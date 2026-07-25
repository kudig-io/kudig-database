---
title: Flannel 网络异常诊断技能
description: Flannel CNI 网络插件的完整故障诊断技能，覆盖 VXLAN/host-gw 后端异常、flannel.1 接口丢失、子网分配冲突、MTU 不匹配、etcd/ConfigMap 依赖故障等场景
summary: Flannel CNI 故障诊断，覆盖后端模式/接口/子网/MTU/etcd 依赖 5 大类 10+ 根因
category: skill
tags:
- k8s
- networking
- cni
- flannel
- vxlan
- troubleshooting
- fta
- daemonset
sources:
- 故障诊断/FTA故障树/list/flannel-fta.md
- 故障诊断/高级排障/structural-03-network-components/
- code/flannel-0.28.7/
created: '2026-05-23'
updated: '2026-07-23'
lifecycle: active
tier: core
difficulty: intermediate
reading_level: intermediate
audience:
- SRE
- 运维工程师
- 技术支持
estimated_read_time: 18min
intent_queries:
- Flannel 网络不通怎么排查
- flannel.1 接口丢失如何恢复
- VXLAN MTU 不匹配导致超时
- Flannel DaemonSet 异常排查
- 跨节点 Pod 通信失败 Flannel
trigger_keywords:
- Flannel
- flannel.1
- VXLAN
- host-gw
- MTU
- 子网分配
- 跨节点不通
prerequisites:
- kubectl-basics
- linux-networking-basics
fta_id: FTA-FLANNEL-001
component: Flannel
severity: high
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。

# Flannel 网络异常诊断技能

## 1. 概述

### 覆盖范围

本技能覆盖 Flannel CNI 在生产环境中的全部常见故障：

- **后端模式异常**：VXLAN 封包错误、host-gw 路由失败
- **网络接口异常**：flannel.1 接口丢失、状态 DOWN
- **子网分配异常**：子网冲突、etcd/ConfigMap 数据不一致
- **MTU 不匹配**：VXLAN 封包开销导致大包丢弃
- **DaemonSet 异常**：flannel Pod 崩溃、配置错误

### 适用场景

| 适用 | 不适用 |
|------|--------|
| 使用 Flannel CNI 的集群 | Terway/Calico/Cilium 网络问题 |
| Pod 跨节点通信失败 | Service/Ingress 层路由问题 |
| flannel.1 接口异常 | 物理网卡/交换机硬件故障 |
| MTU 相关间歇性超时 | 应用层协议错误 |

### 前置条件

- 集群使用 Flannel CNI（`kubectl get ds -n kube-system kube-flannel-ds` 存在）
- 具备节点级 SSH 权限（部分诊断需要）
- 了解集群使用的 Flannel 后端模式（VXLAN/host-gw）

---

## 2. 症状识别

| 症状 ID | 症状描述 | 工单关键词 | 确认命令 |
|---------|---------|-----------|---------|
| S1 | 跨节点 Pod 通信完全不通 | "跨节点不通"、"网络隔离" | `kubectl exec <pod> -- ping <other-node-pod-ip>` |
| S2 | 大报文超时、小报文正常 | "间歇性超时"、"大包失败" | `ping -M do -s 1400 <target-ip>` |
| S3 | 单节点所有 Pod 网络中断 | "节点 Pod 全断"、"flannel 接口没了" | `ip link show flannel.1` |
| S4 | Flannel DaemonSet Pod 异常 | "flannel 崩溃"、"CrashLoop" | `kubectl get pods -n kube-system -l app=flannel` |
| S5 | 新节点加入后 Pod 网络不通 | "新节点网络异常"、"子网冲突" | `kubectl get nodes -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{.spec.podCIDR}{"\n"}{end}'` |
| S6 | 偶发性网络抖动/延迟升高 | "网络抖动"、"延迟高" | `ping -c 100 <target-pod-ip>` 观察丢包率 |

### 排除标准

- 若 `kubectl get nodes` 显示节点 NotReady → 转 [[26-技能/03-节点/node/01-node-notready-diagnosis.md|Node NotReady 诊断]]
- 若仅 Service ClusterIP 不通但 Pod IP 直连正常 → 转 kube-proxy 排查
- 若同节点 Pod 也不通 → 可能是 CNI 配置/容器运行时问题，非纯 Flannel 故障

### 2.1 常见错误消息与事件日志速查

> 以下错误消息和事件日志是 Flannel CNI 故障场景的高频诊断线索。Agent 在采集 Events 和 Flannel 日志后可直接匹配本表快速路由。

#### 关键 Events（`kubectl get events` / `kubectl describe pod`）

| 事件 Reason | 事件 Message 模式 | 含义 | 检测命令 | 路由 |
|-------------|------------------|------|---------|------|
| `FailedCreatePodSandBox` | `Failed to create pod sandbox: rpc error: ... failed to setup network for sandbox: plugin type="flannel" failed (add): ...` | Flannel CNI 插件执行失败 | `kubectl get events -n <ns> --field-selector reason=FailedCreatePodSandBox` | → RC-001 |
| `FailedCreatePodSandBox` | `... NetworkPluginNotReady: network plugin is not ready: CNI plugin not initialized` | Flannel DaemonSet 未就绪 | 同上 | → RC-001 |
| `FailedCreatePodSandBox` | `... plugin type="flannel" failed (add): failed to find plugin "flannel" in path` | flannel 二进制缺失 | 同上 | → RC-001 |
| `FailedCreatePodSandBox` | `... plugin type="flannel" failed (add): no IP addresses available in range` | IP 地址池耗尽 | 同上 | → RC-004 |
| `FailedCreatePodSandBox` | `... context deadline exceeded` | CNI 操作超时 | 同上 | → 检查节点负载 |
| `SandboxChanged` | `Pod sandbox changed, it will be killed and re-created` | 沙箱变更（网络配置更新） | `kubectl get events -n <ns> --field-selector reason=SandboxChanged` | 通常自愈 |
| `NodeNotReady` | `Node <node> status is now: NodeNotReady` + `runtime network plugin is not ready` | 节点因 CNI 未就绪而 NotReady | `kubectl describe node <node>` | → RC-001 |

#### Flannel DaemonSet 日志关键错误（`kubectl logs -n kube-system -l app=flannel`）

```bash
# 🟢 低风险：只读/信息收集
kubectl logs -n kube-system -l app=flannel --tail=100 | grep -iE "error|failed|timeout|conflict|unable"
```

| 日志模式 | 含义 | 对应根因 | 修复方向 |
|---------|------|---------|----------|
| `Error registering network: ... conflict` / `subnet already in use` | 子网分配冲突 | RC-003 | 检查 Pod CIDR 分配 |
| `failed to acquire lease: ...` | 子网租约获取失败 | RC-003/RC-004 | 检查 etcd/ConfigMap 中的子网分配 |
| `VXLAN configured with existing device: flannel.1` | flannel.1 接口已存在（残留） | RC-002 | 删除残留接口后重启 |
| `failed to create vxlan interface: operation not supported` | 内核不支持 VXLAN | RC-002 | 加载 vxlan 内核模块 |
| `MTU mismatch: ...` / `fragmentation needed` | MTU 配置不匹配 | RC-005 | 调整 Flannel MTU 配置 |
| `failed to connect to etcd: ...` / `connection refused` | etcd 连接失败（etcd 后端模式） | RC-001 | 检查 etcd 连接配置 |
| `error creating network interface: ...` | 网络接口创建失败 | RC-002 | 检查节点网络栈 |
| `host-gw: route already exists` / `failed to add route` | host-gw 路由添加失败 | RC-006 | 检查路由表冲突 |
| `watch error: ...` / `list error: ...` | Kubernetes API watch 失败 | RC-001 | 检查 RBAC/API 连接 |
| `panic: runtime error` | Flannel 崩溃 | RC-001 | 重启 Flannel DaemonSet |

#### 节点级网络诊断命令

```bash
# 🟢 低风险：只读/信息收集（需节点 SSH 权限）
# 检查 flannel.1 接口状态
ssh <node-ip> "ip link show flannel.1"
ssh <node-ip> "ip addr show flannel.1"

# 检查 Flannel 路由规则
ssh <node-ip> "ip route show | grep flannel"
ssh <node-ip> "cat /run/flannel/subnet.env"

# 检查 VXLAN FDB 表（跨节点转发）
ssh <node-ip> "bridge fdb show dev flannel.1"

# 检查 MTU 配置
ssh <node-ip> "ip link show flannel.1 | grep mtu"
ssh <node-ip> "ip link show cni0 | grep mtu"

# 测试 VXLAN 封装路径（大包测试）
ssh <node-ip> "ping -M do -s 1400 -c 5 <other-node-pod-ip>"
```

| 节点级现象 | 含义 | 对应根因 |
|-----------|------|----------|
| `ip link show flannel.1` 返回 `Device "flannel.1" does not exist` | flannel.1 接口丢失 | RC-002 |
| flannel.1 状态为 `DOWN` | 接口未启用 | RC-002 |
| `/run/flannel/subnet.env` 不存在 | Flannel 未成功初始化 | RC-001 |
| `subnet.env` 中 FLANNEL_SUBNET 与其他节点重叠 | 子网冲突 | RC-003 |
| `bridge fdb show` 无远端节点 MAC 条目 | VXLAN FDB 未学习 | RC-002/RC-007 |
| MTU 不一致（flannel.1 vs cni0 vs eth0） | MTU 不匹配导致大包丢失 | RC-005 |
| `ip route` 无其他节点 Pod CIDR 路由 | 路由缺失 | RC-006 |

---

## 3. 快速分级

| 严重性 | 定义 | 响应策略 |
|--------|------|---------|
| P0 | 多节点 Pod 网络全断 | 立即检查 flannel DaemonSet 状态，5min 内响应 |
| P1 | 单节点网络异常 | 15min 内重启该节点 flannel Pod |
| P2 | MTU 相关间歇性问题 | 调整 MTU 配置，计划变更窗口 |
| P3 | 偶发延迟/抖动 | 检查网络基础设施和 VXLAN 隧道状态 |

---

## 4. 诊断工作流

### Phase 1：快速检查（< 2 分钟）

#### D1.1 检查 Flannel DaemonSet 状态

```bash
# 🟢 低风险：只读/信息收集
kubectl get ds -n kube-system kube-flannel-ds -o wide
kubectl get pods -n kube-system -l app=flannel -o wide | grep -v Running
```

**判断逻辑**：
- DESIRED ≠ READY → Flannel DaemonSet 异常，转 RC-001
- 特定节点无 flannel Pod → 节点调度/污点问题

#### D1.2 检查 flannel.1 接口状态

```bash
# 🟢 低风险：只读（需在目标节点执行）
ip addr show flannel.1
ip link show flannel.1
ip route show | grep flannel
```

**判断逻辑**：
- 接口不存在 → 转 RC-002
- 接口状态 DOWN → 转 RC-003
- 无 flannel 路由 → 转 RC-004

#### D1.3 检查 Flannel ConfigMap

```bash
# 🟢 低风险：只读/信息收集
kubectl get configmap -n kube-system kube-flannel-cfg -o yaml
```

### Phase 2：深度检查（< 10 分钟）

#### D2.1 VXLAN 隧道检查

```bash
# 🟢 低风险：只读（需在节点执行）
# 检查 VXLAN 端口监听
netstat -ulnp | grep 8472
# 检查 FDB 转发表
bridge fdb show dev flannel.1
# 检查 ARP/邻居表
ip neigh show dev flannel.1
```

#### D2.2 子网分配检查

```bash
# 🟢 低风险：只读/信息收集
# K8s 模式（使用 Node spec.podCIDR）
kubectl get nodes -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{.spec.podCIDR}{"\n"}{end}'

# etcd 模式（旧版 Flannel）
etcdctl get /coreos.com/network/subnets --prefix
```

**判断逻辑**：
- 两节点 podCIDR 相同 → 子网冲突，转 RC-005
- etcd 中子网信息与 Node 不一致 → 转 RC-006

#### D2.3 MTU 验证

```bash
# 🟢 低风险：只读
# 测试不同包大小
ping -M do -s 1450 <target-pod-ip>   # VXLAN 模式应通过
ping -M do -s 1472 <target-pod-ip>   # 应失败（VXLAN 开销 50 bytes）
ping -M do -s 1400 <target-pod-ip>   # 应通过
# 检查接口 MTU
ip link show flannel.1 | grep mtu
```

#### D2.4 Flannel 日志分析

```bash
# 🟢 低风险：只读/信息收集
kubectl logs -n kube-system -l app=flannel --tail=100 | grep -E "error|failed|cannot"
journalctl -u kube-flannel --since "10 min ago"  # systemd 模式
```

### Phase 3：主动探测（需审批）

#### D3.1 跨节点抓包

```bash
# 🟡 中风险：可能影响网络性能
tcpdump -i eth0 udp port 8472 -nn -c 50  # VXLAN 封包
tcpdump -i flannel.1 -nn -c 50           # 解封装后的包
```

#### D3.2 重启 Flannel Pod

```bash
# 🟡 中风险：短暂中断该节点 Pod 网络
kubectl delete pod -n kube-system -l app=flannel --field-selector spec.nodeName=<node>
```

---

## 5. 根因分类

| 编号 | 根因 | 概率 | 关键证据 | FTA 映射 |
|------|------|------|----------|---------|
| RC-001 | Flannel DaemonSet 崩溃/未部署 | 高 | DaemonSet DESIRED≠READY | TE→IE-1→BE-1.1 |
| RC-002 | flannel.1 接口丢失 | 高 | `ip link show flannel.1` 不存在 | TE→IE-2→BE-2.1 |
| RC-003 | flannel.1 接口 DOWN | 中 | 接口状态 DOWN | TE→IE-2→BE-2.2 |
| RC-004 | 路由表缺失 flannel 路由 | 中 | `ip route` 无 flannel 条目 | TE→IE-2→BE-2.3 |
| RC-005 | 子网分配冲突 | 中 | 两节点 podCIDR 重叠 | TE→IE-3→BE-3.1 |
| RC-006 | etcd/ConfigMap 子网数据不一致 | 低 | etcd 与 Node spec 不匹配 | TE→IE-3→BE-3.2 |
| RC-007 | MTU 不匹配（VXLAN 封包开销） | 高 | 大包失败、小包正常 | TE→IE-4→BE-4.1 |
| RC-008 | VXLAN UDP 端口被防火墙阻断 | 中 | 8472 端口不通 | TE→IE-5→BE-5.1 |
| RC-009 | host-gw 模式节点非二层互通 | 中 | 仅 host-gw 后端，跨子网不通 | TE→IE-5→BE-5.2 |
| RC-010 | Flannel 配置错误（Network/Backend 不匹配） | 低 | ConfigMap 与实际网络不一致 | TE→IE-6→BE-6.1 |

---

## 6. 修复操作

| 编号 | 对应根因 | 修复操作 | 风险等级 | 审批要求 |
|------|---------|---------|:--------:|---------|
| REM-001 | RC-001 | 检查 DaemonSet 配置，修复 RBAC/镜像拉取问题，重新部署 | 🟡 | 无需 |
| REM-002 | RC-002 | 重启该节点 flannel Pod 重建接口 | 🟡 | 无需 |
| REM-003 | RC-003 | `ip link set flannel.1 up` 或重启 flannel Pod | 🟡 | 无需 |
| REM-004 | RC-004 | 重启 flannel Pod 重建路由 | 🟡 | 无需 |
| REM-005 | RC-005 | 删除冲突节点 Pod 触发重新分配，或手动修正 Node podCIDR | 🔴 | 高级审批 |
| REM-006 | RC-006 | 清理 etcd 中过期子网记录，重启 flannel | 🟡 | 变更审批 |
| REM-007 | RC-007 | 修改 Flannel 配置 `--iface-mtu=1450`，重启 DaemonSet | 🟡 | 变更审批 |
| REM-008 | RC-008 | 防火墙放行 UDP 8472（VXLAN）端口 | 🟡 | 变更审批 |
| REM-009 | RC-009 | 切换为 VXLAN 模式或确保节点二层互通 | 🔴 | 高级审批 |
| REM-010 | RC-010 | 修正 kube-flannel-cfg ConfigMap，重启 DaemonSet | 🟡 | 变更审批 |

---

## 7. 验证确认

### 即时验证（修复后 1 分钟）

```bash
# 🟢 低风险
kubectl get pods -n kube-system -l app=flannel -o wide  # 全部 Running
ip link show flannel.1                                   # 接口 UP
kubectl exec <pod> -- ping -c 3 <other-node-pod-ip>     # 跨节点连通
```

### 短期监控（15-30 分钟）

- 观察 flannel Pod 日志无新增 error
- 跨节点 ping 丢包率 = 0%
- 大报文测试：`ping -M do -s 1400 <target>` 成功

### 解决标准

| 条件 | 判定 |
|------|------|
| Flannel DaemonSet 全部 READY | ✅ |
| flannel.1 接口 UP 且 MTU 正确 | ✅ |
| 跨节点 Pod ping 延迟 < 1ms、丢包 0% | ✅ |
| 大报文（1400 bytes）ping 成功 | ✅ |

---

## 8. 升级协议

| 级别 | 自动升级条件 | 消息模板 | 交接信息 |
|------|------------|---------|---------|
| P0→专家 | 多节点网络全断 > 5min | "【P0】Flannel 多节点网络中断，影响 {N} 节点" | DaemonSet 状态 + 最近变更 + 接口状态 |
| P1→SME | 单节点问题 > 15min 未恢复 | "【P1】节点 {node} Flannel 网络异常" | 接口状态 + 路由表 + flannel 日志 |
| P2→二线 | MTU 问题需变更窗口 | "【P2】MTU 配置需调整" | 当前 MTU + 测试结果 |

---

## 9. 版本兼容矩阵

| Flannel 版本 | K8s 版本 | 关键差异 |
|-------------|---------|---------|
| v0.12-v0.15 | 1.16-1.21 | 使用 etcd 或 K8s API 存储子网；ConfigMap 名 `kube-flannel-cfg` |
| v0.16-v0.20 | 1.22-1.25 | 默认后端 VXLAN；支持 WireGuard 后端（实验性） |
| v0.21-v0.24 | 1.26-1.28 | 改进 IPv6 双栈支持；ConfigMap 挂载路径变更 |
| v0.25+ (0.28.7) | 1.29-1.36 | 支持 nftables；性能优化；修复多网卡环境路由问题 |

> [存疑：Flannel 0.22+ 是否默认启用 nftables 替代 iptables，需确认各发行版配置]

**通用提示**：排障前先确认 Flannel 版本与后端模式：
```bash
# 🟢 低风险
kubectl get ds -n kube-system kube-flannel-ds -o jsonpath='{.spec.template.spec.containers[0].image}'
kubectl get configmap -n kube-system kube-flannel-cfg -o jsonpath='{.data.net-conf\.json}'
```

---

## 10. 知识进化

### 常见误诊模式

| 误诊模式 | 表现 | 正确做法 |
|---------|------|---------|
| 将 MTU 问题误判为网络中断 | 小文件传输正常、大文件超时 | 先做 `ping -M do -s 1400` 测试 |
| 将节点防火墙误判为 Flannel Bug | 特定节点对不通 | 检查 UDP 8472 端口是否放行 |
| 将 kube-proxy/iptables 问题误判为 Flannel | Service IP 不通但 Pod IP 通 | 区分 Pod 网络（Flannel）和 Service 网络（kube-proxy） |

### 变更记录

| 版本 | 日期 | 变更内容 | 触发原因 |
|------|------|---------|---------|
| 1.0.0 | 2026-05-23 | 初版 FTA 故障树 | 技能库初始化 |
| 2.0.0 | 2026-07-23 | 重构为 12 章节标准结构，补全根因/修复/验证 | 技能建设最佳实践对标 |

---

## 11. 云厂商特异性

### 阿里云 ACK（Flannel 模式）

| 差异点 | 说明 |
|--------|------|
| 默认后端 | VXLAN（`--backend-type=vxlan`） |
| 安全组 | 需放行 UDP 8472 + Pod CIDR 互通 |
| 与 Terway 对比 | Flannel 为 Overlay 有封包开销；Terway 基于 ENI 原生性能 |
| 网络策略 | Flannel 不支持 NetworkPolicy，需额外部署 Calico Policy |

### AWS EKS（Flannel 替代方案）

> EKS 默认使用 AWS VPC CNI（类似 Terway ENI 模式），Flannel 多用于 Kops 自建集群。

---

## 生产案例

### 案例 1: Flannel VXLAN 封包导致 MTU 不匹配引发间歇性超时

| 时间 | 事件 |
|------|------|
| 11:00 | 部分大报文请求超时，小报文正常 |
| 11:10 | `ping -s 1472 pod-ip` 失败，`ping -s 1400` 成功 |
| 11:15 | 确认 VXLAN 封包开销 50 bytes，Pod MTU 应为 1450 |
| 11:20 | 🟡 REM-007 修改 Flannel 配置 `--iface-mtu=1450`，重启 flannel DaemonSet |

**根因**: RC-007。节点 MTU 1500，VXLAN 封包后实际 MTU 1450，Pod 未设置 MTU 导致大包被丢弃。

### 案例 2: flannel.1 接口丢失导致节点 Pod 网络全断

**现象**: 单节点上所有 Pod 无法通信，`ip link show flannel.1` 接口不存在。

**诊断**: `journalctl -u kube-flannel` 显示 "failed to create vxlan interface"

**修复**: 🟡 REM-002 重启 flannel Pod: `kubectl delete pod -n kube-system -l app=flannel --field-selector spec.nodeName=<node>`

### 案例 3: 防火墙升级后 UDP 8472 被阻断

**现象**: 集群升级安全基线后，跨节点 Pod 全部不通

**诊断**: `tcpdump -i eth0 udp port 8472` 无封包到达对端

**修复**: 🟡 REM-008 iptables/firewalld 放行 UDP 8472

---

## 相关链接

- [[26-技能/04-工作负载/pod/方法论/FTA Methodology and Core Principles.md|FTA 方法论]] — 方法论基础
- [[26-技能/04-工作负载/pod/方法论/FTA Diagnostic Execution Engine.md|FTA 诊断执行引擎]] — 执行引擎
- [[26-技能/05-网络/cni/terway-fta.md|Terway CNI 异常诊断]] — 同域技能
- [[26-技能/05-网络/cni/calico-fta.md|Calico 网络异常诊断]] — 同域技能
- [[26-技能/03-节点/node/01-node-notready-diagnosis.md|Node NotReady 诊断]] — 跨域关联
- [[21-生态参考/03-领域索引/flannel-index.md|Flannel 知识图谱索引]] — 知识索引

<!-- risk-assessed -->
