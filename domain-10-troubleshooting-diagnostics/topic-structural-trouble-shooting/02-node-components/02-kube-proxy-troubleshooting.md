---
title: kube-proxy 故障排查指南 [topic-structural-trouble-shooting]
description: 'title: kube-proxy 故障排查指南'
category: structural-troubleshooting
tags:
- troubleshooting
- guide
- kubelet
- scheduler
- prometheus
- docker
- daemonset
- networkpolicy
- gpu
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 技术支持
estimated_read_time: 15min
intent_queries:
- kube-proxy 故障排查指南 是什么
- 如何 kube-proxy 故障排查指南
- Kubernetes 10 troubleshooting diagnostics 最佳实践
- kube-proxy 故障排查指南 故障排查
- kube-proxy 故障排查指南 排障步骤
trigger_keywords:
- kube-proxy
- 故障排查指南
- troubleshooting
- diagnostics
- structural
- trouble
- shooting
prerequisites:
- kubectl-basics
- troubleshooting-methodology
- prometheus-basics
- gpu-scheduling-basics
created: "2026-05-23"
---

title: kube-proxy 故障排查指南
description: '# kube-proxy 故障排查指南'
category: structural-troubleshooting
tags:
- k8s
- troubleshooting
- decision-tree
- scheduler
- [[Prometheus|prometheus]]
- [[DaemonSet|daemonset]]
- [[NetworkPolicy|networkpolicy]]
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 技术支持
estimated_read_time: 5min
intent_queries:
- kube-proxy 故障排查指南 是什么
- 如何 kube-proxy 故障排查指南
- kube-proxy 故障排查指南 故障排查
- kube-proxy 故障排查指南 排障步骤
trigger_keywords:
- kube-proxy
- 故障排查指南
- structural
- trouble
- shooting
authors:
- name: KUDIG Team
  role: contributor
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
---

# kube-proxy 故障排查指南

> **适用版本**: Kubernetes v1.25 - v1.32 | **最后更新**: 2026-01 | **难度**: 中级-高级

---

## 🎯 本文档价值

| 读者对象 | 价值体现 |
| :--- | :--- |
| **初学者** | 理解 Service 负载均衡的“幕后英雄”，掌握 iptables 与 IPVS 的基本概念，学会通过日志和规则命令排查 ClusterIP 不通等基础网络问题。 |
| **资深专家** | 深入剖析 kube-proxy 的内核交互机制（conntrack、ipset、NFQUEUE）、在大规模集群下的性能瓶颈优化、IPVS 模式的深度调优，以及网络分区下的容错处理。 |

---

## 目录

1. [核心原理解析](#11-核心原理解析kube-proxy-的两种面孔)
2. [专家观测工具链](#13-专家观测工具链experts-toolbox)
3. [高级排查工作流](#2-排查方法与步骤-高级工作流)
4. [性能调优与深度治理](#4-性能调优与预防)
5. [基础排查步骤（初学者）](#5-排查方法与步骤-基础版)
6. [基础解决方案（初学者）](#6-解决方案与风险控制-基础版)

---

## 0. 10 分钟快速诊断

1. **组件存活与模式**：`kubectl -n kube-system get pods -l k8s-app=kube-proxy -o wide`；`kubectl -n kube-system get cm kube-proxy -o yaml | grep mode` 确认 iptables/IPVS。
2. **服务/后端核对**：`kubectl get svc <ns>/<svc> -o wide && kubectl get endpoints <ns>/<svc>`，确认 Endpoints 是否为空或不均衡。
3. **数据面规则**：
   - iptables：`iptables-save | grep KUBE-SERVICES | wc -l`，`iptables -t nat -L KUBE-SERVICES -n | head`。
   - IPVS：`ipvsadm -Ln --stats | head`，关注缺失/未同步的虚拟服务。
4. **conntrack/内核健康**：`conntrack -S` 观察表使用率；`dmesg | grep conntrack | tail`；`sysctl net.netfilter.nf_conntrack_max`。
5. **NodePort/外访**：若 NodePort 不通，检查宿主机防火墙/云安全组；`nc -zv <node> <nodePort>` 与 `tcpdump -i eth0 port <nodePort>`。
6. **快速缓解**：
   - 单节点问题：重启该节点 kube-proxy Pod；若规则缺失，删除 Pod 触发重建规则。
   - 大规模性能：切换 IPVS、开启 `strictARP`，调高 conntrack 表并开启连接回收参数；限制 Service 爆炸增长。
   - Endpoints 空：修复上游工作负载或健康检查，避免空代理。
7. **证据留存**：保存规则导出、kube-proxy 日志、conntrack 统计、问题节点的 iptables/ipvs 快照。

---

## 1. 问题现象与影响分析

### 1.1 核心原理解析：kube-proxy 的“两种面孔”

kube-proxy 并不是一个真正的 Proxy（如 Nginx），而是一个**规则控制器**。它监听 API Server 的 Service 和 Endpoints 变化，并将其转化为内核转发规则：

1. **iptables 模式（默认）**：
   - **原理**：利用内核 Netfilter 的 `NAT` 表进行流量转发。
   - **特点**：规则是链式顺序匹配。当 Service 数量超过 5000 时，内核匹配延迟显著增加。
2. **IPVS 模式（高性能）**：
   - **原理**：利用 Linux 内核的 LVS (IP Virtual Server) 进行负载均衡，匹配复杂度为 $O(1)$。
   - **特点**：支持多种调度算法（rr, lc, dh, sh），在大规模集群（1W+ Service）下性能极其优越。

### 1.2 常见问题现象

#### 1.2.1 kube-proxy 服务状态异常

| 现象 | 报错信息关键字 | 根本原因方向 |
| :--- | :--- | :--- |
| **规则同步停滞** | `SyncProxyRules took too long` | iptables 规则过多（> 10W 条）、内核锁竞争 |
| **IPVS 降级** | `IPVS proxier not available; falling back to iptables` | 宿主机缺失 `ip_vs` 相关内核模块 |
| **权限报错** | `Failed to set sysctl ... read-only file system` | Pod 安全上下文（SecurityContext）配置不当 |

#### 1.2.2 Service 转发异常

| 问题场景 | 典型表现 | 排查方向 |
| :--- | :--- | :--- |
| **Service 闪断** | 间歇性 `Connection Refused` | conntrack 表满、部分 Endpoints 探针失败 |
| **固定节点不通** | 仅在某节点访问 Service 报错 | 该节点 kube-proxy Pod 僵死、iptables 规则丢失 |
| **NodePort 无法访问** | 外部访问超时，内部 OK | 宿主机防火墙（firewalld/ufw）、云平台安全组 |

#### 1.2.3 生产环境典型“性能杀手”

1. **iptables 规则爆炸导致 CPU 飙升**：
   - **背景**：随着 Service 增加，iptables 每次同步需要刷新全量规则，导致 `kube-proxy` 长期占用单核 100% CPU。
   - **对策**：必须切换到 IPVS 模式。
2. **Conntrack Race Condition (UDP)**：
   - **背景**：在多核节点上，高并发 UDP 请求可能触发 Netfilter 的 conntrack 竞态。
   - **现象**：DNS 解析偶尔出现 5s 延迟。
   - **对策**：升级内核版本，或者配置 `single-request-reopen`。

### 1.3 专家观测工具链（Expert's Toolbox）

```bash
# 专家级：深度检查 IPVS 转发链（IPVS 模式）
ipvsadm -Ln --stats            # 查看转发统计，定位是否有流量倾斜
ipvsadm -Ln --rate             # 查看实时速率

# 专家级：追踪流量匹配 iptables 规则
# 在规则中添加 TRACE（需谨慎，仅在测试环境使用）
iptables -t raw -A PREROUTING -p tcp --destination <ClusterIP> -j TRACE
# 然后在 dmesg 或 /var/log/messages 查看匹配路径

# 专家级：检查 conntrack 连接状态
conntrack -L -p tcp --dport <ServicePort> | grep <PodIP>
```

---

## 4. 性能调优与预防

### 4.1 监控核心指标（Prometheus）
| 指标 | 含义 | 风险点 |
| :--- | :--- | :--- |
| `kubeproxy_sync_proxy_rules_duration_seconds` | 规则同步延迟 | P99 > 1s |
| `kubeproxy_sync_proxy_rules_last_timestamp_seconds` | 最后同步时间 | > 60s 未更新 |
| `kubeproxy_network_programming_duration_seconds` | 网络编程延迟 | P99 > 2s |

### 4.2 最佳实践方案
- **大规模集群推荐 IPVS**：规则匹配时间恒定，且支持更复杂的负载均衡算法。
- **内核参数优化**：
  ```bash
  # 增加 conntrack 表大小
  net.netfilter.nf_conntrack_max = 1048576
  # 降低 TIME_WAIT 连接回收时间
  net.ipv4.tcp_tw_reuse = 1
  ```
- **配置 strictARP**：在使用 MetalLB 或 BGP 模式的 CNI 时，必须在 kube-proxy 配置中开启 `strictARP` 以保证 ARP 响应正确。

---

## 排查方法与步骤（高级工作流）

### 排查模型：三级跳

1. **Control Plane**：确认 API Server 中 Service 和 Endpoints 是否正确对齐。
2. **Data Plane (Kernel)**：确认内核规则（iptables/IPVS）是否已生成并正确映射到 Endpoints。
3. **Environment Layer**：确认内核参数（conntrack_max）、宿主机防火墙、CNI 网络连通性。

### 专家级排查步骤

```
开始排查
    │
    ├─► 检查 kube-proxy Pod 状态
    │       │
    │       ├─► Pod 未运行 ──► 检查 Pod 启动失败原因
    │       │
    │       └─► Pod 运行中 ──► 继续下一步
    │
    ├─► 检查代理模式
    │       │
    │       ├─► iptables 模式 ──► 检查 iptables 规则
    │       │
    │       └─► IPVS 模式 ──► 检查 IPVS 规则
    │
    ├─► 检查 Service 和 Endpoints
    │       │
    │       ├─► Endpoints 为空 ──► 检查 Pod 选择器
    │       │
    │       └─► Endpoints 存在 ──► 继续下一步
    │
    ├─► 检查规则是否正确
    │       │
    │       ├─► 规则不存在 ──► kube-proxy 同步问题
    │       │
    │       └─► 规则存在 ──► 继续下一步
    │
    └─► 检查实际连通性
            │
            ├─► 连接失败 ──► 检查网络策略、防火墙
            │
            └─► 连接成功 ──► 问题可能在应用层
```

### 排查步骤和具体命令

#### 2.3.1 第一步：检查 kube-proxy Pod 状态

```bash
# 查看所有 kube-proxy Pod
kubectl get pods -n kube-system -l k8s-app=kube-proxy -o wide

# 查看 Pod 详情
kubectl describe pod -n kube-system -l k8s-app=kube-proxy

# 查看 kube-proxy 日志
kubectl logs -n kube-system -l k8s-app=kube-proxy --tail=200

# 检查健康状态
curl http://localhost:10256/healthz

# 检查 kube-proxy 配置
kubectl get configmap -n kube-system kube-proxy -o yaml
```

#### 2.3.2 第二步：确认代理模式

```bash
# 从 ConfigMap 查看模式
kubectl get configmap -n kube-system kube-proxy -o yaml | grep mode

# 从日志确认
kubectl logs -n kube-system -l k8s-app=kube-proxy | grep -i "using.*mode"

# 检查 IPVS 模块是否加载（IPVS 模式）
lsmod | grep -E "ip_vs|nf_conntrack"

# 检查 iptables 版本
iptables --version
```

#### 2.3.3 第三步：检查 Service 和 Endpoints

```bash
# 查看 Service 详情
kubectl get svc <service-name> -n <namespace> -o yaml

# 查看 Endpoints
kubectl get endpoints <service-name> -n <namespace> -o yaml

# 查看 EndpointSlice（v1.21+）
kubectl get endpointslices -n <namespace> -l kubernetes.io/service-name=<service-name>

# 验证 Service selector 匹配的 Pod
kubectl get pods -n <namespace> -l <selector-key>=<selector-value>

# 检查 Pod 是否 Ready
kubectl get pods -n <namespace> -o wide | grep <service-related>
```

#### 2.3.4 第四步：检查 iptables 规则（iptables 模式）

```bash
# 查看所有 KUBE-SERVICES 规则
iptables -t nat -L KUBE-SERVICES -n --line-numbers

# 查看特定 Service 的规则
iptables -t nat -L -n | grep <cluster-ip>
iptables -t nat -L -n | grep <service-name>

# 查看 KUBE-SVC 链
iptables -t nat -L KUBE-SVC-<hash> -n

# 查看 KUBE-SEP 链（Service Endpoint）
iptables -t nat -L KUBE-SEP-<hash> -n

# 统计规则数量
iptables -t nat -L -n | wc -l

# 查看 NodePort 规则
iptables -t nat -L KUBE-NODEPORTS -n
```

#### 2.3.5 第五步：检查 IPVS 规则（IPVS 模式）

```bash
# 查看所有 IPVS 规则
ipvsadm -Ln

# 查看特定 Service 的规则
ipvsadm -Ln -t <cluster-ip>:<port>

# 查看连接统计
ipvsadm -Ln --stats

# 查看速率统计
ipvsadm -Ln --rate

# 检查 IPVS 超时设置
ipvsadm -L --timeout

# 查看 kube-ipvs0 接口
ip addr show kube-ipvs0
```

#### 2.3.6 第六步：测试连通性

```bash
# 从 Pod 内测试 ClusterIP
kubectl run test-pod --rm -it --image=busybox -- sh
# 在 Pod 内执行
wget -qO- http://<cluster-ip>:<port>
nc -zv <cluster-ip> <port>

# 测试 NodePort
curl http://<node-ip>:<node-port>

# 测试 DNS 解析
kubectl run test-pod --rm -it --image=busybox -- nslookup <service-name>.<namespace>.svc.cluster.local

# 检查 conntrack
conntrack -L -d <cluster-ip>

# 抓包分析
tcpdump -i any host <cluster-ip> -nn
```

#### 2.3.7 第七步：检查日志和指标

```bash
# 查看 kube-proxy 同步日志
kubectl logs -n kube-system -l k8s-app=kube-proxy | grep -i "sync"

# 查看错误日志
kubectl logs -n kube-system -l k8s-app=kube-proxy | grep -iE "(error|failed)"

# 查看 kube-proxy 指标
curl http://localhost:10249/metrics | grep kubeproxy

# 关键指标
# kubeproxy_sync_proxy_rules_duration_seconds - 规则同步延迟
# kubeproxy_sync_proxy_rules_last_timestamp_seconds - 最后同步时间
# kubeproxy_network_programming_duration_seconds - 网络编程延迟
```

### 2.4 排查注意事项

#### 2.4.1 安全注意事项

| 注意项 | 说明 | 建议 |
|--------|------|------|
| **iptables 操作** | 错误的规则可能导致网络中断 | 先查看再修改 |
| **IPVS 操作** | 影响所有 Service 流量 | 谨慎操作 |
| **conntrack 清理** | 可能导致连接中断 | 评估影响 |

#### 2.4.2 操作注意事项

| 注意项 | 说明 | 建议 |
|--------|------|------|
| **重启影响** | 重启 kube-proxy 会短暂影响规则同步 | 在维护窗口操作 |
| **模式切换** | iptables 和 IPVS 切换需要清理旧规则 | 规划切换步骤 |
| **大规模集群** | 规则数量大时同步较慢 | 监控同步时间 |

---

## 6. 解决方案与风险控制 (基础版)

### 3.1 kube-proxy Pod 未运行

#### 3.1.1 解决步骤

```bash
# 步骤 1：检查 Pod 状态
kubectl get pods -n kube-system -l k8s-app=kube-proxy -o wide
kubectl describe pod -n kube-system <kube-proxy-pod>

# 步骤 2：查看失败原因
kubectl logs -n kube-system <kube-proxy-pod> --previous

# 步骤 3：检查 DaemonSet 配置
kubectl get daemonset -n kube-system kube-proxy -o yaml

# 步骤 4：检查节点是否有污点阻止调度
kubectl get nodes -o custom-columns='NAME:.metadata.name,TAINTS:.spec.taints'

# 步骤 5：如果是配置问题，修复 ConfigMap
kubectl edit configmap -n kube-system kube-proxy

# 步骤 6：重启 kube-proxy Pod
kubectl rollout restart daemonset -n kube-system kube-proxy

# 步骤 7：验证恢复
kubectl get pods -n kube-system -l k8s-app=kube-proxy
```

#### 3.1.2 执行风险

| 风险等级 | 风险描述 | 缓解措施 |
|----------|----------|----------|
| **中** | 重启期间规则不更新 | 在低峰期操作 |
| **中** | 配置错误可能导致启动失败 | 修改前备份 |
| **低** | 查看状态无风险 | - |

#### 3.1.3 安全生产风险提示

```
⚠️  安全生产风险提示：
1. kube-proxy 重启期间 Service 规则不更新
2. 已有的连接和规则不受影响
3. ConfigMap 修改会影响所有节点
4. 大规模集群逐批重启
5. 验证恢复后测试服务连通性
```

### 3.2 Service 访问不通

#### 3.2.1 解决步骤

```bash
# 步骤 1：确认 Service 和 Endpoints 存在
kubectl get svc <service-name> -n <namespace>
kubectl get endpoints <service-name> -n <namespace>

# 步骤 2：如果 Endpoints 为空，检查 Pod 选择器
kubectl get svc <service-name> -n <namespace> -o yaml | grep -A5 selector
kubectl get pods -n <namespace> -l <selector>

# 步骤 3：检查 Pod 是否 Ready
kubectl get pods -n <namespace> -o wide

# 步骤 4：检查规则是否存在
# iptables 模式
iptables -t nat -L -n | grep <cluster-ip>

# IPVS 模式
ipvsadm -Ln -t <cluster-ip>:<port>

# 步骤 5：如果规则不存在，检查 kube-proxy 日志
kubectl logs -n kube-system -l k8s-app=kube-proxy | grep <service-name>

# 步骤 6：强制同步规则
# 重启该节点的 kube-proxy
kubectl delete pod -n kube-system <kube-proxy-pod-on-node>

# 步骤 7：测试连通性
kubectl run test --rm -it --image=busybox -- wget -qO- http://<cluster-ip>:<port>
```

#### 3.2.2 执行风险

| 风险等级 | 风险描述 | 缓解措施 |
|----------|----------|----------|
| **低** | 检查操作无风险 | - |
| **中** | 重启 kube-proxy 会短暂影响规则同步 | 单节点重启 |
| **低** | 测试 Pod 无风险 | - |

#### 3.2.3 安全生产风险提示

```
⚠️  安全生产风险提示：
1. 确认 Endpoints 不为空是第一步
2. Pod 未 Ready 不会加入 Endpoints
3. 规则同步有一定延迟（通常几秒）
4. 使用 headless Service 排除 kube-proxy 问题
5. 检查 NetworkPolicy 是否阻止了访问
```

### 3.3 iptables 规则问题

#### 3.3.1 解决步骤

```bash
# 步骤 1：检查规则是否存在
iptables -t nat -L KUBE-SERVICES -n | grep <cluster-ip>

# 步骤 2：检查完整转发链
CLUSTER_IP=<cluster-ip>
PORT=<port>

# 找到 KUBE-SVC 链
iptables -t nat -L -n | grep -A2 "$CLUSTER_IP.*$PORT"

# 查看后端 Pod 规则
iptables -t nat -L KUBE-SVC-<hash> -n

# 步骤 3：如果规则错误或缺失，检查 kube-proxy
kubectl logs -n kube-system -l k8s-app=kube-proxy --tail=200 | grep -iE "(error|failed)"

# 步骤 4：清理并重建规则（谨慎操作）
# 方法 1：重启 kube-proxy
kubectl rollout restart daemonset -n kube-system kube-proxy

# 方法 2：手动清理（仅在必要时）
# ⚠️ 危险操作
iptables -t nat -F KUBE-SERVICES
iptables -t nat -F KUBE-NODEPORTS
# 等待 kube-proxy 重建规则

# 步骤 5：验证规则恢复
iptables -t nat -L KUBE-SERVICES -n --line-numbers
```

#### 3.3.2 执行风险

| 风险等级 | 风险描述 | 缓解措施 |
|----------|----------|----------|
| **高** | 清理 iptables 规则会中断所有 Service | 仅在必要时使用 |
| **中** | 重启 kube-proxy 有短暂影响 | 在维护窗口操作 |
| **低** | 查看规则无风险 | - |

#### 3.3.3 安全生产风险提示

```
⚠️  安全生产风险提示：
1. 不要手动修改 KUBE-* 开头的 iptables 规则
2. 手动清理规则会导致所有 Service 短暂不可用
3. 优先通过重启 kube-proxy 解决
4. 规则重建通常在几秒内完成
5. 大规模 Service 环境规则重建时间较长
```

### 3.4 IPVS 模式问题

#### 3.4.1 解决步骤

```bash
# 步骤 1：确认 IPVS 模式生效
kubectl logs -n kube-system -l k8s-app=kube-proxy | grep "Using ipvs"

# 步骤 2：检查 IPVS 内核模块
lsmod | grep ip_vs
# 如果未加载，加载模块
modprobe ip_vs
modprobe ip_vs_rr
modprobe ip_vs_wrr
modprobe ip_vs_sh
modprobe nf_conntrack

# 步骤 3：检查 IPVS 规则
ipvsadm -Ln -t <cluster-ip>:<port>

# 步骤 4：检查 kube-ipvs0 接口
ip addr show kube-ipvs0

# 步骤 5：如果 ClusterIP 未绑定到 kube-ipvs0
# 检查 kube-proxy 日志
kubectl logs -n kube-system -l k8s-app=kube-proxy | grep -i "ipvs"

# 步骤 6：检查 strictARP 配置（对于 MetalLB 等）
kubectl get configmap -n kube-system kube-proxy -o yaml | grep strictARP

# 步骤 7：如果需要启用 strictARP
kubectl edit configmap -n kube-system kube-proxy
# 设置 ipvs.strictARP: true

# 步骤 8：重启 kube-proxy
kubectl rollout restart daemonset -n kube-system kube-proxy
```

#### 3.4.2 执行风险

| 风险等级 | 风险描述 | 缓解措施 |
|----------|----------|----------|
| **中** | 模块加载一般无风险 | 生产环境预先配置 |
| **中** | strictARP 变更影响 ARP 响应 | 评估 LB 方案需求 |
| **低** | 查看规则无风险 | - |

#### 3.4.3 安全生产风险提示

```
⚠️  安全生产风险提示：
1. IPVS 模块需要在所有节点加载
2. 持久化模块加载配置到 /etc/modules-load.d/
3. strictARP 对某些负载均衡方案是必需的
4. IPVS 模式比 iptables 性能更好，适合大规模集群
5. 模式切换需要清理旧规则，建议在维护窗口进行
```

### 3.5 从 iptables 模式切换到 IPVS 模式

#### 3.5.1 解决步骤

```bash
# 步骤 1：加载必要的内核模块
cat > /etc/modules-load.d/ipvs.conf << EOF
ip_vs
ip_vs_rr
ip_vs_wrr
ip_vs_sh
nf_conntrack
EOF

# 加载模块
modprobe ip_vs
modprobe ip_vs_rr
modprobe ip_vs_wrr
modprobe ip_vs_sh
modprobe nf_conntrack

# 步骤 2：安装 ipvsadm 工具（如未安装）
apt-get install -y ipvsadm  # Debian/Ubuntu
yum install -y ipvsadm      # CentOS/RHEL

# 步骤 3：修改 kube-proxy ConfigMap
kubectl edit configmap -n kube-system kube-proxy
# 修改 mode 为 ipvs：
# mode: "ipvs"
# ipvs:
#   scheduler: "rr"  # 调度算法

# 步骤 4：重启 kube-proxy
kubectl rollout restart daemonset -n kube-system kube-proxy

# 步骤 5：清理旧的 iptables 规则（可选）
# kube-proxy 会自动清理，但可以手动加速
iptables -t nat -F KUBE-SERVICES
iptables -t nat -F KUBE-NODEPORTS
iptables -t nat -F KUBE-POSTROUTING

# 步骤 6：验证切换成功
kubectl logs -n kube-system -l k8s-app=kube-proxy | grep "Using ipvs"
ipvsadm -Ln

# 步骤 7：测试 Service 连通性
kubectl run test --rm -it --image=busybox -- wget -qO- http://<cluster-ip>:<port>
```

#### 3.5.2 执行风险

| 风险等级 | 风险描述 | 缓解措施 |
|----------|----------|----------|
| **高** | 模式切换期间 Service 可能短暂不可用 | 在维护窗口操作 |
| **中** | 模块未加载会导致切换失败 | 预先验证模块 |
| **中** | 旧规则清理不完全可能有残留 | 验证规则状态 |

#### 3.5.3 安全生产风险提示

```
⚠️  安全生产风险提示：
1. 在测试环境充分验证后再在生产执行
2. 模式切换期间避免业务高峰
3. 分批在节点上执行，观察效果
4. 准备回滚方案（改回 iptables 模式）
5. 切换后全面测试 Service 连通性
6. 某些 CNI 插件对 IPVS 模式有特殊要求
```

### 3.6 conntrack 表问题

#### 3.6.1 解决步骤

```bash
# 步骤 1：检查 conntrack 表状态
conntrack -C  # 当前连接数
cat /proc/sys/net/netfilter/nf_conntrack_max  # 最大值

# 步骤 2：如果 conntrack 表满
# 临时增加限制
sysctl -w net.netfilter.nf_conntrack_max=262144

# 持久化配置
echo "net.netfilter.nf_conntrack_max=262144" >> /etc/sysctl.conf
sysctl -p

# 步骤 3：清理过期连接
conntrack -F

# 步骤 4：检查特定 Service 的连接
conntrack -L -d <cluster-ip>

# 步骤 5：如果连接状态异常，清理特定连接
conntrack -D -d <cluster-ip>

# 步骤 6：调整连接超时
sysctl -w net.netfilter.nf_conntrack_tcp_timeout_established=3600
sysctl -w net.netfilter.nf_conntrack_tcp_timeout_time_wait=30

# 步骤 7：验证连通性恢复
kubectl run test --rm -it --image=busybox -- wget -qO- http://<cluster-ip>:<port>
```

#### 3.6.2 执行风险

| 风险等级 | 风险描述 | 缓解措施 |
|----------|----------|----------|
| **中** | 清理 conntrack 可能断开活跃连接 | 评估影响 |
| **低** | 增加 conntrack_max 一般无风险 | 确保内存充足 |
| **中** | 超时调整影响连接保持 | 根据业务调整 |

#### 3.6.3 安全生产风险提示

```
⚠️  安全生产风险提示：
1. conntrack 表满会导致新连接建立失败
2. 清理 conntrack 可能中断现有连接
3. 增大 conntrack_max 会增加内存使用
4. 监控 conntrack 使用率，设置告警
5. 长连接服务注意超时配置
```

---

## 附录

### A. kube-proxy 关键指标

| 指标名称 | 说明 | 告警阈值建议 |
|----------|------|--------------|
| `kubeproxy_sync_proxy_rules_duration_seconds` | 规则同步延迟 | P99 > 1s |
| `kubeproxy_sync_proxy_rules_last_timestamp_seconds` | 最后同步时间 | > 60s 未更新 |
| `kubeproxy_network_programming_duration_seconds` | 网络编程延迟 | P99 > 2s |

### B. 常见配置参数

| 参数 | 默认值 | 说明 |
|------|--------|------|
| `mode` | iptables | 代理模式 (iptables/ipvs) |
| `ipvs.scheduler` | rr | IPVS 调度算法 |
| `ipvs.strictARP` | false | 严格 ARP 模式 |
| `iptables.masqueradeAll` | false | 是否对所有流量做 SNAT |
| `conntrack.maxPerCore` | 32768 | 每核心 conntrack 限制 |

### C. IPVS 调度算法

| 算法 | 说明 | 适用场景 |
|------|------|----------|
| `rr` | 轮询 | 通用场景 |
| `lc` | 最少连接 | 长连接服务 |
| `dh` | 目标地址哈希 | 会话保持 |
| `sh` | 源地址哈希 | 会话保持 |
| `sed` | 最短期望延迟 | 权重不等的场景 |
| `nq` | 不排队 | 实时性要求高 |

## Related

- 08-docker-troubleshooting-guide
- 16-troubleshooting-guide
- [[domain-17-system-foundation/topic-cheat-sheet/go|go]]
- [[domain-17-system-foundation/topic-cheat-sheet/linux|linux]]
- [[domain-17-system-foundation/topic-cheat-sheet/k8s|k8s]]
- [[domain-19-landscape-references/topic-index/node-index|Node 知识图谱索引]]
- [[domain-19-landscape-references/topic-index/gitops-cicd-index|GitOps / CI-CD 全局索引]]

## See Also

- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/02-node-components/06-gpu-device-plugin-troubleshooting|06-gpu-device-plugin-troubleshooting]]
- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/02-node-components/01-kubelet-troubleshooting|01-kubelet-troubleshooting]]
- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/02-node-components/03-container-runtime-troubleshooting|03-container-runtime-troubleshooting]]
- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/02-node-components/04-node-troubleshooting|04-node-troubleshooting]]
