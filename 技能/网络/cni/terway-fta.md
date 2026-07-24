---
title: Terway CNI 异常诊断技能
description: 阿里云 Terway CNI 网络插件的完整故障诊断技能，覆盖 ENI 分配异常、IP 地址池耗尽、VPC 路由错误、安全组冲突、控制面依赖故障等场景
summary: Terway CNI 故障诊断，覆盖 ENI/IP 池/VPC 路由/安全组/控制面依赖 5 大类 10+ 根因
category: skill
tags:
- k8s
- networking
- cni
- terway
- eni
- troubleshooting
- fta
- ack
- aliyun
sources:
- 故障诊断/FTA故障树/list/terway-fta.md
- 故障诊断/高级排障/structural-03-network-components/
- code/terway-1.17.5/
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
estimated_read_time: 20min
intent_queries:
- Terway Pod 获取 IP 失败怎么排查
- Terway ENI 配额不足如何解决
- 阿里云 ACK 网络异常诊断
- Terway DaemonSet 异常排查
- Pod 跨节点通信失败 Terway
trigger_keywords:
- Terway
- ENI
- ENIIP
- IP 分配失败
- 网络不通
- ACK 网络
- 安全组
- vSwitch
prerequisites:
- kubectl-basics
- vpc-networking-basics
fta_id: FTA-TERWAY-001
component: Terway
severity: high
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。

# Terway CNI 异常诊断技能

## 1. 概述

### 覆盖范围

本技能覆盖阿里云 Terway CNI 网络插件在生产环境中的全部常见故障：

- **ENI 分配异常**：配额耗尽、绑定失败、状态漂移
- **IP 地址池异常**：池耗尽、vSwitch IP 不足、回收延迟
- **VPC 路由异常**：路由表错误、路由条目缺失
- **安全组异常**：规则缺失、规则冲突
- **控制面依赖异常**：Terway DaemonSet 崩溃、API Server 连接失败

### 适用场景

| 适用 | 不适用 |
|------|--------|
| ACK 集群使用 Terway CNI | Flannel/Calico/Cilium 网络问题 |
| Pod 获取 IP 失败 | Service/Ingress 层路由问题 |
| Pod 跨节点通信失败 | 应用层协议错误 |
| ENI/IP 资源异常 | ECS 实例本身网络故障（非 K8s 层面） |

### 前置条件

- 集群使用 Terway CNI（`kubectl get ds -n kube-system terway-eniip` 存在）
- 具备 kube-system 命名空间 Pod 日志读取权限
- 部分诊断需要 aliyun CLI 及对应 RAM 权限

---

## 2. 症状识别

| 症状 ID | 症状描述 | 工单关键词 | 确认命令 |
|---------|---------|-----------|---------|
| S1 | 新 Pod 长时间 ContainerCreating | "IP 分配失败"、"网络未就绪" | `kubectl get events -n <ns> --field-selector reason=FailedCreatePodSandBox` |
| S2 | Pod 跨节点通信超时 | "跨节点不通"、"超时" | `kubectl exec <pod> -- ping <target-pod-ip>` |
| S3 | 同节点 Pod 互通但跨节点不通 | "安全组"、"同节点正常" | `terway-cli mapping` + 安全组检查 |
| S4 | Terway DaemonSet Pod CrashLoopBackOff | "terway 崩溃"、"重启" | `kubectl get pods -n kube-system -l app=terway` |
| S5 | 批量 Pod 无法获取 IP（扩容后） | "扩容失败"、"IP 不够" | `kubectl logs -n kube-system -l app=terway --tail=50` |
| S6 | 固定 IP Pod 重建后 IP 变化 | "IP 变了"、"固定 IP 失效" | `kubectl get pod -o yaml | grep -A5 annotations` |

### 排除标准

- 若 `kubectl get nodes` 显示节点 NotReady → 转 [[技能/节点/node/01-node-notready-diagnosis.md|Node NotReady 诊断]]
- 若仅 Service ClusterIP 不通但 Pod IP 直连正常 → 转 Service/kube-proxy 排查
- 若 DNS 解析失败但 IP 连通正常 → 转 CoreDNS 排查

---

## 3. 快速分级

| 严重性 | 定义 | 响应策略 |
|--------|------|---------|
| P0 | 全集群 Pod 网络中断 | 立即检查 Terway DaemonSet + 安全组，5min 内响应 |
| P1 | 新 Pod 无法获取 IP（业务扩容受阻） | 15min 内检查 ENI 配额/IP 池 |
| P2 | 单节点/少量 Pod 网络异常 | 标准诊断流程，检查节点级 Terway Pod |
| P3 | 偶发网络延迟/抖动 | 检查 VPC 路由表和安全组配置 |

---

## 4. 诊断工作流

### Phase 1：快速检查（< 2 分钟）

#### D1.1 确认 Terway DaemonSet 状态

```bash
# 🟢 低风险：只读/信息收集
kubectl get ds -n kube-system terway-eniip -o wide
kubectl get pods -n kube-system -l app=terway -o wide | grep -v Running
```

**判断逻辑**：
- DESIRED ≠ READY → Terway DaemonSet 异常，转 RC-001
- 特定节点无 Terway Pod → 节点调度/污点问题

#### D1.2 检查异常 Pod 事件

```bash
# 🟢 低风险：只读/信息收集
kubectl get events -n ${NAMESPACE} --field-selector reason=FailedCreatePodSandBox --sort-by='.lastTimestamp' | tail -20
```

**判断逻辑**：
- Events 含 `ENI`/`bindquota`/`AttachNetworkInterface` → 转 ENI 子树（RC-002~004）
- Events 含 `IP`/`pool`/`address` → 转 IP 池子树（RC-005~006）
- Events 含 `timeout`/`context deadline` → 转控制面依赖（RC-009）

#### D1.3 检查 Terway 日志

```bash
# 🟢 低风险：只读/信息收集
kubectl logs -n kube-system -l app=terway --tail=50 | grep -E "error|failed|exceeded|timeout"
```

### Phase 2：深度检查（< 10 分钟）

#### D2.1 ENI 配额与状态检查

```bash
# 🟢 低风险：只读/信息收集（需要 aliyun CLI）
aliyun ecs DescribeInstances --InstanceIds '["${INSTANCE_ID}"]' | jq '.Instances.Instance[0].NetworkInterfaces.NetworkInterface | length'
aliyun ecs DescribeNetworkInterfaces --InstanceId ${INSTANCE_ID} | jq '.NetworkInterfaceSets.NetworkInterfaceSet[] | {id: .NetworkInterfaceId, status: .Status}'
```

#### D2.2 IP 池与 vSwitch 检查

```bash
# 🟢 低风险：只读/信息收集
kubectl exec -n kube-system $(kubectl get pods -n kube-system -l app=terway -o jsonpath='{.items[0].metadata.name}') -- terway-cli show
kubectl exec -n kube-system $(kubectl get pods -n kube-system -l app=terway -o jsonpath='{.items[0].metadata.name}') -- terway-cli mapping
```

```bash
# 🟢 低风险：只读（需要 aliyun CLI）
aliyun vpc DescribeVSwitchAttributes --VSwitchId ${VSWITCH_ID} | jq '{AvailableIpAddressCount}'
```

#### D2.3 安全组规则验证

```bash
# 🟢 低风险：只读（需要 aliyun CLI）
aliyun ecs DescribeSecurityGroupAttribute --SecurityGroupId ${SG_ID} --Direction ingress | jq '.Permissions.Permission[] | select(.SourceCidrIp != "")'
```

**关键检查**：Pod CIDR 段是否在安全组入方向规则中被放行。

#### D2.4 VPC 路由表检查

```bash
# 🟢 低风险：只读（需要 aliyun CLI）
aliyun vpc DescribeRouteTableList --VpcId ${VPC_ID} | jq '.RouterTableList.RouterTableListType[].RouteTableId'
aliyun vpc DescribeRouteEntryList --RouteTableId ${RTB_ID} | jq '.RouteEntrys.RouteEntry[] | select(.DestinationCidrBlock | test("172\\."))'
```

### Phase 3：主动探测（需审批）

#### D3.1 节点网络连通性测试

```bash
# 🟢 低风险：只读
kubectl exec -n kube-system <terway-pod> -- terway-cli check
# 或手动测试
kubectl run net-test --rm -it --image=nicolaka/netshoot -- bash
ping <target-pod-ip>
traceroute <target-pod-ip>
```

#### D3.2 Terway Pod 重启

```bash
# 🟡 中风险：会短暂中断该节点新 Pod 网络分配
kubectl delete pod -n kube-system <terway-pod-on-target-node>
```

---

## 5. 根因分类

| 编号 | 根因 | 概率 | 关键证据 | FTA 映射 |
|------|------|------|----------|---------|
| RC-001 | Terway DaemonSet 未部署/崩溃 | 高 | DaemonSet DESIRED≠READY | TE→IE-1→BE-1.1 |
| RC-002 | ENI 配额耗尽 | 高 | 日志 `bindquota exceeded`，ENI 数达实例上限 | TE→IE-2→BE-2.1 |
| RC-003 | ENI 绑定失败（云平台 API 错误） | 中 | 日志 `AttachNetworkInterface failed` | TE→IE-2→BE-2.2 |
| RC-004 | ENI 状态漂移（Detaching 残留） | 中 | `terway-cli show` 与云平台不一致 | TE→IE-2→BE-2.3 |
| RC-005 | vSwitch 可用 IP 耗尽 | 高 | `AvailableIpAddressCount = 0` | TE→IE-3→BE-3.1 |
| RC-006 | IP 池回收延迟（Terway 缓存不一致） | 中 | `terway-cli mapping` 显示已释放 IP 仍被占用 | TE→IE-3→BE-3.2 |
| RC-007 | 安全组未放行 Pod CIDR | 高 | 同节点通、跨节点不通 | TE→IE-4→BE-4.1 |
| RC-008 | VPC 路由表缺失/错误 | 中 | 路由表无 Pod CIDR 条目 | TE→IE-4→BE-4.2 |
| RC-009 | Terway 与 API Server 连接超时 | 中 | 日志 `context deadline exceeded` | TE→IE-5→BE-5.1 |
| RC-010 | Terway 配置错误（ENI 模式/共享模式不匹配） | 低 | ConfigMap 配置与实际模式不一致 | TE→IE-5→BE-5.2 |

---

## 6. 修复操作

| 编号 | 对应根因 | 修复操作 | 风险等级 | 审批要求 |
|------|---------|---------|:--------:|---------|
| REM-001 | RC-001 | 检查 DaemonSet 调度约束，修复 nodeSelector/tolerations，重新部署 | 🟡 | 无需 |
| REM-002 | RC-002 | 阿里云控制台提升 ENI 配额，或切换为 ENIIP 共享模式 | 🟡 | 变更审批 |
| REM-003 | RC-003 | 等待云平台 API 恢复；若持续失败，联系阿里云工单 | 🟢 | 无需 |
| REM-004 | RC-004 | 重启该节点 Terway Pod 触发 ENI 状态同步 | 🟡 | 无需 |
| REM-005 | RC-005 | 扩展 vSwitch CIDR 或添加新 vSwitch 到 Terway 配置 | 🔴 | 高级审批 |
| REM-006 | RC-006 | 重启 Terway Pod 清理本地缓存 | 🟡 | 无需 |
| REM-007 | RC-007 | 安全组添加入方向规则：允许 Pod CIDR 全端口互通 | 🟡 | 变更审批 |
| REM-008 | RC-008 | 修正 VPC 路由表，添加 Pod CIDR → 节点 ENI 路由条目 | 🔴 | 高级审批 |
| REM-009 | RC-009 | 检查 API Server 健康状态和网络连通性 | 🟢 | 无需 |
| REM-010 | RC-010 | 修正 Terway ConfigMap（`eni_conf`），确保模式一致 | 🟡 | 变更审批 |

---

## 7. 验证确认

### 即时验证（修复后 1 分钟）

```bash
# 🟢 低风险
kubectl get pods -n kube-system -l app=terway -o wide  # 全部 Running
kubectl run verify-net --rm -it --image=busybox -- ping -c 3 <target-pod-ip>  # 连通性
```

### 短期监控（15-30 分钟）

- 观察 Terway Pod 日志无新增 error
- 新创建的 Pod 能在 30s 内获取 IP
- 监控 `terway_eni_bindquota_usage` 指标

### 解决标准

| 条件 | 判定 |
|------|------|
| Terway DaemonSet 全部 READY | ✅ |
| 新 Pod 创建无 FailedCreatePodSandBox 事件 | ✅ |
| 跨节点 Pod ping 延迟 < 1ms（同 VPC） | ✅ |
| 30 分钟内无新增网络相关工单 | ✅ |

---

## 8. 升级协议

| 级别 | 自动升级条件 | 消息模板 | 交接信息 |
|------|------------|---------|---------|
| P0→专家 | 全集群网络中断 > 5min | "【P0】Terway 全集群网络中断，影响 {N} 个 Pod" | DaemonSet 状态 + 最近变更 + 安全组快照 |
| P1→SME | ENI/IP 资源耗尽且无法自助扩容 | "【P1】{节点池} ENI/IP 资源耗尽，业务扩容受阻" | 配额使用量 + vSwitch 剩余 IP + 实例规格 |
| P2→二线 | 单节点问题 > 30min 未解决 | "【P2】节点 {node} Terway 异常" | 节点 Terway 日志 + terway-cli show 输出 |

---

## 9. 版本兼容矩阵

| Terway 版本 | K8s 版本 | 关键差异 |
|------------|---------|---------|
| terway-eniip v1.2+ | ACK 1.22+ | 默认 ENIIP 共享模式，支持固定 IP |
| terway-eniip v1.5+ | ACK 1.26+ | 支持 NetworkPolicy（基于 eBPF/iptables） |
| terway v1.7+ | ACK 1.28+ | 支持 IPvlan 模式、RDMA |
| terway v1.9+ | ACK 1.30+ | 支持 Pod 级别安全组（独立 ENI 安全组） |

> [存疑：Terway 各版本与 ACK 集群版本的精确对应关系需参照阿里云官方发布说明确认]

**通用提示**：排障前先确认 Terway 版本：
```bash
# 🟢 低风险
kubectl get ds -n kube-system terway-eniip -o jsonpath='{.spec.template.spec.containers[0].image}'
```

---

## 10. 知识进化

### 常见误诊模式

| 误诊模式 | 表现 | 正确做法 |
|---------|------|---------|
| 将安全组问题误判为 Terway Bug | 同节点通、跨节点不通 | 先检查安全组规则是否放行 Pod CIDR |
| 将 vSwitch IP 耗尽误判为 ENI 配额 | 日志含 "no available IP" | 区分 ENI 配额（实例级）和 IP 配额（子网级） |
| 将节点 NotReady 误判为网络问题 | 多 Pod 同时异常 | 先 `kubectl get nodes` 排除节点级故障 |

### 变更记录

| 版本 | 日期 | 变更内容 | 触发原因 |
|------|------|---------|---------|
| 1.0.0 | 2026-05-23 | 初版 FTA 故障树 | 技能库初始化 |
| 2.0.0 | 2026-07-23 | 重构为 12 章节标准结构，补全根因/修复/验证 | 技能建设最佳实践对标 |

---

## 11. 云厂商特异性（阿里云 ACK）

### Terway 模式选型

| 模式 | 原理 | 优势 | 限制 | 适用场景 |
|------|------|------|------|---------|
| ENI 独占 | 每 Pod 一个独立 ENI | 网络性能最佳 | 受 ENI 数量限制（4-8/节点） | 高性能/低延迟 |
| ENIIP 共享 | 多 Pod 共享 ENI 辅助 IP | 密度高 | 性能略低 | **生产推荐** |
| ENI 固定 IP | StatefulSet Pod 固定 IP | IP 不变 | 占用 IP 资源 | 有状态服务 |

### ACK 特有排查命令

```bash
# 🟢 低风险：terway-cli 诊断（在 Terway Pod 内执行）
kubectl exec -n kube-system <terway-pod> -- terway-cli show       # 查看 ENI/IP 分配
kubectl exec -n kube-system <terway-pod> -- terway-cli mapping    # 查看 Pod-IP 映射
kubectl exec -n kube-system <terway-pod> -- terway-cli check      # 健康检查
```

---

## 生产案例

### 案例 1: Terway ENI 配额耗尽导致新 Pod 无法获取 IP

| 时间 | 事件 |
|------|------|
| 10:00 | 扩容 Deployment 50→80 副本 |
| 10:02 | 新 Pod 全部 Pending，Events 显示 "failed to allocate IP" |
| 10:05 | `kubectl logs -n kube-system -l app=terway` 显示 ENI quota exceeded |
| 10:10 | 阿里云控制台提升 ENI 配额，或切换为 ENIIP 共享模式 |
| 10:15 | Pod 获取 IP 成功，业务恢复 |

**根因**: RC-002。每节点独立 ENI 模式受 ECS 实例规格 ENI 数量限制(通常 4-8 个)，未提前规划容量。

### 案例 2: Terway 安全组规则缺失导致跨节点 Pod 通信失败

**现象**: 同节点 Pod 互通，跨节点 Pod 访问超时。

**诊断**: `terway-cli mapping` 检查 IP 分配正常 → 安全组未放行 Pod CIDR 段

**修复**: 🟡 REM-007 安全组添加入方向规则: 允许 Pod CIDR 全部端口互通

### 案例 3: vSwitch IP 耗尽导致整个节点池无法扩容

**现象**: 节点池所有新 Pod ContainerCreating > 5min

**诊断**: `aliyun vpc DescribeVSwitchAttributes` 显示 `AvailableIpAddressCount: 0`

**修复**: 🔴 REM-005 添加新 vSwitch 到 Terway ConfigMap 的 `vswitches` 配置

---

## 相关链接

- [[技能/工作负载/pod/方法论/FTA Methodology and Core Principles.md|FTA 方法论]] — 方法论基础
- [[技能/工作负载/pod/方法论/FTA Diagnostic Execution Engine.md|FTA 诊断执行引擎]] — 执行引擎
- [[技能/网络/cni/flannel-fta.md|Flannel 网络异常诊断]] — 同域技能
- [[技能/节点/node/01-node-notready-diagnosis.md|Node NotReady 诊断]] — 跨域关联
- [[生态参考/领域索引/terway-index.md|Terway 知识图谱索引]] — 知识索引

<!-- risk-assessed -->
