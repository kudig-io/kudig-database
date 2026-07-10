---
title: Terway（阿里云 CNI）网络故障排查指南 [topic-structural-trouble-shooting]
description: 'title: Terway（阿里云 CNI）网络故障排查指南'
summary: 'title: Terway（阿里云 CNI）网络故障排查指南'
category: structural-troubleshooting
tags:
- troubleshooting
- guide
- kubelet
- prometheus
- istio
- flannel
- calico
- coredns
- docker
- statefulset
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 技术支持
estimated_read_time: 15min
intent_queries:
- Terway（阿里云 CNI）网络故障排查指南 是什么
- 如何 Terway（阿里云 CNI）网络故障排查指南
- Kubernetes 10 troubleshooting diagnostics 最佳实践
- Terway（阿里云 CNI）网络故障排查指南 故障排查
- Terway（阿里云 CNI）网络故障排查指南 排障步骤
trigger_keywords:
- Terway
- 阿里云
- CNI
- 网络故障排查指南
- troubleshooting
- diagnostics
- structural
- trouble
prerequisites:
- kubectl-basics
- troubleshooting-methodology
- service-mesh-basics
- prometheus-basics
- ebpf-basics
- cni-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




title: Terway（阿里云 CNI）网络故障排查指南
description: '# Terway（阿里云 CNI）网络故障排查指南'
category: structural-troubleshooting
tags:
- k8s
- troubleshooting
- decision-tree
- [[kubelet|kubelet]]
- [[Prometheus|prometheus]]
- calico
- coredns
- statefulset
- daemonset
- networkpolicy
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 技术支持
estimated_read_time: 5min
intent_queries:
- Terway（阿里云 CNI）网络故障排查指南 是什么
- 如何 Terway（阿里云 CNI）网络故障排查指南
- Terway（阿里云 CNI）网络故障排查指南 故障排查
- Terway（阿里云 CNI）网络故障排查指南 排障步骤
trigger_keywords:
- Terway
- 阿里云
- CNI
- 网络故障排查指南
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

# Terway（阿里云 CNI）网络故障排查指南

> **适用版本**: Kubernetes v1.25 - v1.32 | Terway v1.2+ | **最后更新**: 2026-04 | **难度**: 高级

---

## 0. 10 分钟快速诊断

1. **Terway Pod 状态**：`kubectl get pods -n kube-system -l app=terway`，确认 terway 和 terway-eniip/terway-eni DaemonSet Pod 均为 Running。
2. **节点弹性网卡信息**：`kubectl describe node <node-name> | grep aliyun.com`，查看已分配/剩余 ENI 和 IP 数量。
3. **Pod IP 归属**：`kubectl get pod <pod-name> -o yaml | grep k8s.aliyun.com`，确认 Pod 使用的是 ENI 模式还是 Veth 模式。
4. **VPC 路由检查**：登录阿里云控制台，确认 VPC 路由表是否包含 Pod CIDR 指向各节点 ECS 实例的路由条目。
5. **安全组规则**：确认节点安全组是否放通 Pod 间通信所需端口（尤其是自定义安全组场景）。
6. **快速缓解**：
   - Pod 无法分配 IP：检查节点 ENI 配额和 IP 池是否耗尽，必要时升级实例规格或释放闲置 Pod。
   - 跨节点通信失败：检查 VPC 路由表和安全组规则，确认无自定义路由冲突。
   - 网络策略不生效：确认是否启用了 Calico 策略引擎且版本兼容。
7. **证据留存**：保存节点 Annotation、terway Pod 日志、弹性网卡控制台截图、VPC 路由表配置。

---

## 1. 问题现象与影响分析

### 1.1 常见问题现象

#### 1.1.1 Pod IP 分配失败

| 现象 | 报错信息 | 报错来源 | 查看方式 |
|------|----------|----------|----------|
| Pod 长期处于 ContainerCreating | `failed to allocate pod IP: no available IP` | terway Pod | `kubectl describe pod` |
| ENI 分配失败 | `failed to allocate eni: exceeded eni quota` | terway Pod | terway Pod 日志 |
| 固定 IP 冲突 | `fixed IP already in use` | terway Pod | terway Pod 日志 |
| IP 资源池耗尽 | `pool is empty` | terway IPAM | terway Pod 日志 |
| 实例规格限制 | `instance type eni limit exceeded` | terway | 节点 Annotation + 控制台 |

#### 1.1.2 跨节点 Pod 通信失败

| 现象 | 报错信息 | 报错来源 | 查看方式 |
|------|----------|----------|----------|
| 同 VPC 跨节点 Ping 不通 | `Destination Host Unreachable` | Pod 内 ping | Pod 内执行 |
| 跨节点连接超时 | `dial tcp: i/o timeout` | 应用日志 | 应用 Pod 日志 |
| 安全组阻断 | `connection timed out` | 应用日志 | 安全组规则检查 |
| VPC 路由缺失 | `no route to host` | Pod 内命令 | VPC 控制台路由表 |
| 自定义路由冲突 | `route conflict detected` | terway | terway Pod 日志 |

#### 1.1.3 网络策略与访问控制异常

| 现象 | 报错信息 | 报错来源 | 查看方式 |
|------|----------|----------|----------|
| NetworkPolicy 不生效 | 策略已应用但流量未被拦截/放行 | Calico Felix | Calico Pod 日志 |
| 安全组与策略冲突 | 安全组放通但策略拒绝，或反之 | 网络行为异常 | 对比安全组与策略规则 |
| Pod 无法访问 Service | `connection refused` / `no route` | 应用日志 | kube-proxy + terway 日志 |
| 集群外访问 Pod 失败 | 外部负载均衡无法访问后端 Pod | SLB/ALB | 阿里云控制台 + Pod 状态 |

#### 1.1.4 性能与稳定性问题

| 现象 | 报错信息 | 报错来源 | 查看方式 |
|------|----------|----------|----------|
| 高并发下网络延迟抖动 | 延迟周期性飙升 | 应用监控 | Prometheus + APM |
| ENI 分配延迟高 | Pod 创建耗时 > 30s | kubelet Events | `kubectl describe pod` |
| 节点网络卡顿 | `soft lockup` / `watchdog timeout` | 内核日志 | `dmesg` / `journalctl -k` |
| DNS 解析慢 | CoreDNS 查询延迟高 | CoreDNS 日志 | `kubectl logs -n kube-system -l k8s-app=kube-dns` |

---

## 2. 排查方法与步骤

### 2.1 Terway 架构与模式确认

#### 2.1.1 三种网络模式对比

Terway 支持三种主要网络模式，问题表现和排查方法各不相同：

| 模式 | 网络接口 | IP 分配 | 适用场景 | 排查重点 |
|------|----------|---------|----------|----------|
| **ENI 模式** | 弹性网卡直接挂载到 Pod | 每个 Pod 独占 ENI 或辅助 IP | 高并发、低延迟、需要固定 IP | ENI 配额、VPC 路由、安全组 |
| **Veth 模式** | veth pair（与标准 CNI 类似） | 从节点子网分配 | 兼容性要求高、不依赖 ENI 配额 | CNI 配置、网桥、iptables |
| **IPVlan 模式** | 内核 IPVlan 接口 | 从 ENI 辅助 IP 分配 | 极致性能、ENI 共享 | 内核版本、IPVlan L2/L3 模式 |

#### 2.1.2 确认当前集群使用的 Terway 模式

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看 Terway ConfigMap 配置
kubectl get configmap -n kube-system eni-config -o yaml

# 查看节点使用的网络模式
kubectl get node <node-name> -o yaml | grep -A 20 "aliyun.com"

# 查看 Terway DaemonSet 环境变量
kubectl get ds -n kube-system terway-eniip -o yaml | grep -A 5 "env:"
```
典型配置：
- `ENI_ALLOCATE_MODE`：`eip`（弹性公网 IP）、`eni`（独占 ENI）、`eniip`（共享 ENI 辅助 IP）
- `NETWORK_POLICY_PROVIDER`：`calico` 或空（不使用网络策略）

### 2.2 Pod IP 分配失败排查

#### 2.2.1 排查逻辑决策树

```
Pod 处于 ContainerCreating，事件显示 IP 分配失败
    │
    ├─ 1. 检查 terway Pod 状态
    │       ├─ terway Pod 未 Running → 排查 DaemonSet / 节点资源
    │       └─ terway Pod Running → 进入 2
    │
    ├─ 2. 查看 terway 日志
    │       ├─ "exceeded eni quota" → ENI 配额不足（2.2.2）
    │       ├─ "no available IP" → IP 池耗尽（2.2.3）
    │       ├─ "fixed IP already in use" → 固定 IP 冲突（2.2.4）
    │       └─ 其他错误 → 阿里云 OpenAPI 调用失败（2.2.5）
    │
    └─ 3. 检查节点资源
            ├─ ENI 数量达到实例规格上限 → 升级实例规格或释放 ENI
            └─ 辅助 IP 达到 ENI 上限 → 申请更多 ENI 或调整单 ENI IP 数
```

#### 2.2.2 ENI 配额不足

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 查看节点已分配的 ENI 和 IP 数量
kubectl describe node <node-name> | grep -E "aliyun.com/allocated-eni|aliyun.com/allocated-ip|aliyun.com/eni-max|aliyun.com/ip-max"

# 进入 terway Pod 查看资源池状态
kubectl exec -n kube-system <terway-pod> -- terway-cli show

# 查看实例规格支持的 ENI 和辅助 IP 上限
# 登录阿里云控制台：ECS -> 实例详情 -> 本实例弹性网卡
# 或通过 API
curl "https://ecs.aliyuncs.com/?Action=DescribeInstanceTypes&InstanceTypes.1=<instance-type>"
```
**关键指标**：
- `aliyun.com/allocated-eni`：已分配 ENI 数量
- `aliyun.com/eni-max`：实例规格支持的最大 ENI 数量
- `aliyun.com/allocated-ip`：已分配辅助 IP 数量

**解决方案**：
- 释放不再使用的 Pod（尤其是使用独占 ENI 的 Pod）
- 升级 ECS 实例规格以支持更多 ENI
- 调整 Terway 配置，使用 `eniip` 模式（共享辅助 IP）替代 `eni` 模式（独占 ENI）

#### 2.2.3 IP 资源池耗尽

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 查看 terway 资源池详情
kubectl exec -n kube-system <terway-pod> -- terway-cli show

# 查看节点上所有 Pod 使用的 IP
kubectl get pods --all-namespaces --field-selector spec.nodeName=<node-name> -o wide

# 检查是否存在已删除但 IP 未释放的 Pod（孤儿 IP）
kubectl exec -n kube-system <terway-pod> -- terway-cli garbage-collect --dry-run
```
**常见原因**：
- Pod 频繁创建删除导致 IP 分配/释放速率不匹配
- 固定 IP 的 Pod 数量超过 IP 池容量
- Terway 版本过旧，存在 IP 泄漏 Bug

#### 2.2.4 固定 IP 冲突

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 查看使用固定 IP 的 Pod
kubectl get pods --all-namespaces -o yaml | grep -B 5 "k8s.aliyun.com/allocated-ipv4"

# 查看固定 IP 分配情况
kubectl exec -n kube-system <terway-pod> -- terway-cli show | grep "fixed"

# 检查 Pod Annotation
kubectl get pod <pod-name> -o yaml | grep "k8s.aliyun.com"
```
**关键 Annotation**：
- `k8s.aliyun.com/allocated-ipv4`：已分配的固定 IPv4 地址
- `k8s.aliyun.com/allocated-ipv6`：已分配的固定 IPv6 地址

#### 2.2.5 阿里云 OpenAPI 调用失败

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看 terway 日志中的 API 调用错误
kubectl logs -n kube-system <terway-pod> --tail=500 | grep -iE "api|error|fail|throttle"

# 检查 terway 使用的 RAM 角色权限
# 登录阿里云控制台：RAM -> 角色 -> <集群名称>-worker-role
# 确认策略包含：AliyunECSNetworkInterfaceManagementAccess、AliyunVPCReadOnlyAccess
```
**常见 API 错误**：
- `Throttling.User`：API 调用频率超限，需降低 Pod 创建速率或申请提高限流阈值
- `InvalidVSwitchId.NotFound`：VSwitch ID 不存在或已删除
- `InvalidSecurityGroupId.NotFound`：安全组 ID 不存在

### 2.3 跨节点通信失败排查

#### 2.3.1 VPC 路由检查

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 在 Pod 内测试跨节点连通性
kubectl exec -it <pod-a> -- ping <pod-b-ip>

# 在节点上查看路由表
ip route get <pod-b-ip>

# 查看节点上的网络接口
ip addr show
```
**阿里云控制台检查**：
1. 登录 VPC 控制台 -> 路由表
2. 确认存在指向各节点 ECS 实例的系统路由（由 Terway 自动维护）
3. 检查是否存在自定义路由与 Pod CIDR 冲突

```bash
# 使用阿里云 CLI 查看路由表
aliyun vpc DescribeRouteTableList --RegionId <region-id> --RouteTableId <route-table-id>

# 检查路由条目
aliyun vpc DescribeRouteEntryList --RouteTableId <route-table-id>
```

#### 2.3.2 安全组规则检查

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看节点绑定的安全组
kubectl describe node <node-name> | grep "SecurityGroup"

# 阿里云 CLI 查看安全组规则
aliyun ecs DescribeSecurityGroupAttribute --SecurityGroupId <sg-id> --RegionId <region-id>
```
**关键规则**：
- 入方向：放通 Pod CIDR 网段的所有端口（或至少放通业务所需端口）
- 出方向：通常默认放通，但自定义安全组需确认
- **注意**：如果 Pod 使用独立安全组（Terway 高级特性），需额外检查 Pod 级安全组规则

#### 2.3.3 Terway 路由同步问题

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 检查 terway 是否成功同步路由
kubectl logs -n kube-system <terway-pod> --tail=200 | grep -i "route"

# 手动触发路由同步（谨慎操作）
kubectl exec -n kube-system <terway-pod> -- terway-cli sync

# 检查节点上的路由表
ip route | grep <pod-cidr>
```
### 2.4 网络策略问题排查

#### 2.4.1 Calico 与 Terway 集成检查

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 确认 Calico 组件状态
kubectl get pods -n kube-system -l k8s-app=calico-node
kubectl get pods -n kube-system -l k8s-app=calico-kube-controllers

# 查看 Calico Felix 配置
kubectl get configmap -n kube-system calico-config -o yaml

# 检查 felix 日志
kubectl logs -n kube-system <calico-node-pod> -c calico-node | grep -i "policy"
```
**已知兼容性问题**：
- Terway ENI 模式下，Calico NetworkPolicy 可能无法拦截同节点 Pod 间流量（绕过宿主机协议栈）
- 解决方案：升级到 Terway v1.4+ 和 Calico v3.24+，或启用 eBPF 数据面

#### 2.4.2 安全组与网络策略的优先级

在阿里云环境中，安全组规则和网络策略同时生效时的优先级：

| 流量方向 | 安全组 | NetworkPolicy | 实际效果 |
|----------|--------|---------------|----------|
| 入站 | 拒绝 | 允许 | **拒绝**（安全组优先） |
| 入站 | 允许 | 拒绝 | **拒绝**（策略生效） |
| 出站 | 拒绝 | 允许 | **拒绝**（安全组优先） |

**排查建议**：同时检查安全组规则和 NetworkPolicy 规则，避免遗漏。

### 2.5 性能问题排查

#### 2.5.1 ENI 分配延迟

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 监控 Pod 创建到 Running 的时间
kubectl get events --field-selector reason=Scheduled,reason=Created

# 查看 terway 分配 IP 的耗时
kubectl logs -n kube-system <terway-pod> | grep -i "allocate.*cost|duration"

# 检查阿里云 OpenAPI 延迟
kubectl logs -n kube-system <terway-pod> | grep -i "api.*latency|api.*duration"
```
**优化方向**：
- 启用 Terway 的预分配（Pre-allocation）机制，提前准备 ENI/IP
- 调整 `terway-eniip` DaemonSet 的资源限制
- 使用 IPVlan 模式替代 Veth 模式，减少协议栈开销

#### 2.5.2 网络延迟与丢包

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# Pod 间延迟测试
kubectl exec -it <pod-a> -- ping -c 100 -i 0.01 <pod-b-ip>

# 查看网络接口统计
ip -s link show <interface>

# 检查是否有丢包或错误
ethtool -S <interface> | grep -iE "error|drop|discard"

# 检查内核网络参数
sysctl -a | grep -E "net.core.netdev_max_backlog|net.ipv4.tcp_congestion_control"
```
---

## 3. 解决方案与风险控制

### 3.1 Pod IP 分配失败修复

#### 方案一：释放 ENI/IP 资源

> ⚠️ **🔴 灾难性操作** — 含不可逆命令，执行前必须满足变更窗口+双人复核+事前备份+回滚方案
> - `kubectl delete --all`：批量删除某类全部资源，波及面巨大
> - `kubectl edit/patch`：修改运行中的资源

> **🔴 高风险操作警告**
>
> 下方命令属于不可逆或高影响操作，执行前请确认：
> - 已备份关键数据与配置
> - 处于批准的变更窗口期
> - 已获得相关责任人授权
> - 已准备回滚或恢复方案
> - 目标集群、Namespace、节点/资源名称正确无误

``` bash
# 🔴 高风险：可能造成数据丢失或服务中断，执行前需备份、变更审批与回滚方案
# 查找并删除已终止但未释放资源的 Pod
kubectl get pods --all-namespaces --field-selector spec.nodeName=<node-name>,status.phase=Failed
kubectl delete pods --all-namespaces --field-selector spec.nodeName=<node-name>,status.phase=Failed  # ⚠️ 批量删除，波及面大

# 对于使用独占 ENI 的 StatefulSet，考虑调整为共享模式
# 修改 Terway ConfigMap
kubectl edit configmap -n kube-system eni-config
# 将 ENI_ALLOCATE_MODE 从 "eni" 改为 "eniip"
```
**风险**：修改 Terway 配置后，新创建的 Pod 会使用新模式，已运行的 Pod 不受影响。建议在低峰期操作。

#### 方案二：升级实例规格

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 通过阿里云 CLI 升级实例规格（需先停止实例）
aliyun ecs ModifyInstanceSpec --InstanceId <instance-id> --InstanceType <new-type>

# 或使用 ACK 节点池自动升级
kubectl patch nodepool <nodepool-name> --type merge -p '{"spec":{"instanceTypes":["["<new-type>"]}}'
```
**风险**：升级实例规格会导致节点短暂不可用（需重启），建议通过新增节点池并迁移业务的方式平滑升级。

#### 方案三：调整 ENI 辅助 IP 数量

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

> **🔴 高风险操作警告**
>
> 下方命令属于不可逆或高影响操作，执行前请确认：
> - 已备份关键数据与配置
> - 处于批准的变更窗口期
> - 已获得相关责任人授权
> - 已准备回滚或恢复方案
> - 目标集群、Namespace、节点/资源名称正确无误

``` bash
# 🔴 高风险：可能造成数据丢失或服务中断，执行前需备份、变更审批与回滚方案
# 编辑 Terway ConfigMap，调整单 ENI 的最大辅助 IP 数
kubectl edit configmap -n kube-system eni-config
# 修改 max-eni-ip 参数（需根据实例规格支持的辅助 IP 数设置）
```
### 3.2 跨节点通信修复

#### 方案一：修复 VPC 路由

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl delete`：删除资源（可由声明式清单重建）

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 如果路由缺失，重启 terway Pod 触发路由同步
kubectl delete pod -n kube-system -l app=terway --field-selector spec.nodeName=<node-name>

# 或手动添加路由（临时措施）
aliyun vpc CreateRouteEntry --RouteTableId <rt-id> --DestinationCidrBlock <pod-cidr> --NextHopId <ecs-id> --NextHopType Instance
```
#### 方案二：调整安全组规则

```bash
# 添加安全组规则放通 Pod CIDR
aliyun ecs AuthorizeSecurityGroup \
  --RegionId <region-id> \
  --SecurityGroupId <sg-id> \
  --IpProtocol all \
  --SourceCidrIp <pod-cidr> \
  --PortRange "-1/-1" \
  --Priority 1
```

### 3.3 网络策略修复

#### 方案：升级 Terway 与 Calico 版本

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 查看当前版本
kubectl get ds -n kube-system terway-eniip -o yaml | grep image
kubectl get ds -n kube-system calico-node -o yaml | grep image

# ACK 集群可通过控制台升级 Terway 插件
# 或手动更新镜像版本
kubectl set image ds/terway-eniip -n kube-system terway=registry-vpc.cn-hangzhou.aliyuncs.com/acs/terway:v1.4.0
kubectl set image ds/calico-node -n kube-system calico-node=registry-vpc.cn-hangzhou.aliyuncs.com/acs/calico-node:v3.24.0
```
**风险**：网络插件升级可能导致短暂网络中断，建议在维护窗口执行，并确保有回滚方案。

### 3.4 性能优化

#### 方案一：启用 IPVlan 模式

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源
> - `kubectl rollout undo/restart`：触发滚动变更，影响副本

> **🔴 高风险操作警告**
>
> 下方命令属于不可逆或高影响操作，执行前请确认：
> - 已备份关键数据与配置
> - 处于批准的变更窗口期
> - 已获得相关责任人授权
> - 已准备回滚或恢复方案
> - 目标集群、Namespace、节点/资源名称正确无误

``` bash
# 🔴 高风险：可能造成数据丢失或服务中断，执行前需备份、变更审批与回滚方案
# 修改 Terway ConfigMap 启用 IPVlan
kubectl edit configmap -n kube-system eni-config
# 添加或修改：
# NETWORK_POLICY_PROVIDER: calico
# ENABLE_IP_VLAN: "true"

# 重启 Terway DaemonSet 使配置生效
kubectl rollout restart ds/terway-eniip -n kube-system
```
**前置条件**：
- 内核版本 >= 4.19（推荐 >= 5.4）
- 实例支持 IPVlan
- 不与其他依赖 macvlan 的应用共存

#### 方案二：启用 ENI 预分配

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

> **🔴 高风险操作警告**
>
> 下方命令属于不可逆或高影响操作，执行前请确认：
> - 已备份关键数据与配置
> - 处于批准的变更窗口期
> - 已获得相关责任人授权
> - 已准备回滚或恢复方案
> - 目标集群、Namespace、节点/资源名称正确无误

``` bash
# 🔴 高风险：可能造成数据丢失或服务中断，执行前需备份、变更审批与回滚方案
# 在 Terway ConfigMap 中配置预分配参数
kubectl edit configmap -n kube-system eni-config
# 添加：
# ENI_PRE_ALLOCATE: "2"
# IP_PRE_ALLOCATE: "10"
```
---

## 4. 预防与最佳实践

### 4.1 监控告警配置

```yaml
# PrometheusRule: Terway 关键指标告警
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: terway-alerts
  namespace: monitoring
spec:
  groups:
    - name: terway
      rules:
        - alert: TerwayENIQuotaExhausted
          expr: |
            (
              aliyun_terway_allocated_eni / aliyun_terway_eni_max
            ) > 0.85
          for: 5m
          labels:
            severity: warning
          annotations:
            summary: "Terway ENI 配额即将耗尽"
            description: "节点 {{ $labels.node }} ENI 使用率 {{ $value | humanizePercentage }}"

        - alert: TerwayIPPoolExhausted
          expr: |
            (
              aliyun_terway_allocated_ip / aliyun_terway_ip_max
            ) > 0.9
          for: 3m
          labels:
            severity: critical
          annotations:
            summary: "Terway IP 池即将耗尽"
            description: "节点 {{ $labels.node }} IP 使用率 {{ $value | humanizePercentage }}"

        - alert: TerwayPodAllocationSlow
          expr: |
            histogram_quantile(0.99,
              rate(terway_pod_allocate_duration_seconds_bucket[5m])
            ) > 30
          for: 5m
          labels:
            severity: warning
          annotations:
            summary: "Terway Pod IP 分配延迟过高"
            description: "P99 分配延迟 {{ $value }}s"
```

### 4.2 日常巡检清单

- [ ] **ENI 配额**：各节点 ENI 和 IP 使用率是否低于 80%
- [ ] **Terway Pod 健康**：`kubectl get pods -n kube-system -l app=terway` 是否全部 Running
- [ ] **VPC 路由**：Pod CIDR 路由是否完整指向各节点
- [ ] **安全组**：是否无意收紧了 Pod 间通信规则
- [ ] **内核版本**：是否满足 Terway 最低要求（>= 4.19）
- [ ] **Terway 版本**：是否为最新稳定版，是否存在已知 CVE

### 4.3 容量规划建议

| 指标 | 警戒线 | 严重线 | 建议措施 |
|------|--------|--------|----------|
| 节点 ENI 使用率 | 80% | 95% | 升级实例规格或启用 eniip 模式 |
| 节点 IP 使用率 | 85% | 95% | 增加 ENI 数量或调整辅助 IP 数 |
| Pod 分配延迟 P99 | 5s | 15s | 启用预分配或排查 API 限流 |
| 跨节点丢包率 | 0.1% | 1% | 检查安全组、VPC 路由、物理网络 |

### 4.4 自动化诊断脚本

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
#!/bin/bash
# terway-health-check.sh - Terway 健康检查脚本

NAMESPACE="kube-system"
FAILED=0

echo "=== Terway 健康检查 ==="

# 1. 检查 Terway Pod 状态
echo "[1/5] 检查 Terway Pod 状态..."
NOT_RUNNING=$(kubectl get pods -n $NAMESPACE -l app=terway -o json | \
  jq -r '.items[] | select(.status.phase != "Running") | .metadata.name')
if [ -n "$NOT_RUNNING" ]; then
  echo "  ✗ 异常 Pod: $NOT_RUNNING"
  FAILED=1
else
  echo "  ✓ 所有 Terway Pod 运行正常"
fi

# 2. 检查节点 ENI 配额
echo "[2/5] 检查节点 ENI/IP 配额..."
kubectl get nodes -o json | jq -r '
  .items[] |
  {
    name: .metadata.name,
    eni_used: (.metadata.annotations["k8s.aliyun.com/allocated-eni"] // "0"),
    eni_max: (.metadata.annotations["k8s.aliyun.com/eni-max"] // "0"),
    ip_used: (.metadata.annotations["k8s.aliyun.com/allocated-ip"] // "0"),
    ip_max: (.metadata.annotations["k8s.aliyun.com/ip-max"] // "0")
  } |
  select((.eni_max | tonumber) > 0) |
  "  \(.name): ENI \(.eni_used)/\(.eni_max), IP \(.ip_used)/\(.ip_max)"
'

# 3. 检查固定 IP 冲突
echo "[3/5] 检查固定 IP 冲突..."
DUPLICATES=$(kubectl get pods --all-namespaces -o json | \
  jq -r '.items[] | select(.metadata.annotations["k8s.aliyun.com/allocated-ipv4"]) | 
    .metadata.annotations["k8s.aliyun.com/allocated-ipv4"]' | \
  sort | uniq -d)
if [ -n "$DUPLICATES" ]; then
  echo "  ✗ 发现重复固定 IP: $DUPLICATES"
  FAILED=1
else
  echo "  ✓ 无固定 IP 冲突"
fi

# 4. 检查 VPC 路由（需配置阿里云 CLI）
echo "[4/5] 检查 VPC 路由（需 aliyun CLI）..."
if command -v aliyun &> /dev/null; then
  ROUTE_TABLE_ID=$(kubectl get configmap -n $NAMESPACE eni-config -o json | \
    jq -r '.data.vpc_id // empty')
  if [ -n "$ROUTE_TABLE_ID" ]; then
    echo "  ℹ VPC ID: $ROUTE_TABLE_ID，请手动检查路由表完整性"
  fi
else
  echo "  ⚠ 未安装 aliyun CLI，跳过自动路由检查"
fi

# 5. 检查最近 10 分钟内的 terway 错误日志
echo "[5/5] 检查 Terway 错误日志..."
ERRORS=$(kubectl logs -n $NAMESPACE -l app=terway --since=10m 2>/dev/null | \
  grep -icE "error|fail|unable")
if [ "$ERRORS" -gt 0 ]; then
  echo "  ✗ 最近 10 分钟发现 $ERRORS 条错误日志"
  FAILED=1
else
  echo "  ✓ 最近 10 分钟无错误日志"
fi

echo ""
if [ $FAILED -eq 1 ]; then
  echo "检查结果: 存在异常，请进一步排查"
  exit 1
else
  echo "检查结果: 健康"
  exit 0
fi
```
---

## 附录 A: Terway 关键 Annotation 速查

| Annotation | 说明 | 示例值 |
|------------|------|--------|
| `k8s.aliyun.com/allocated-ipv4` | Pod 分配的 IPv4 地址 | `192.168.1.100` |
| `k8s.aliyun.com/allocated-ipv6` | Pod 分配的 IPv6 地址 | `2408:4002:10c0:8200::1` |
| `k8s.aliyun.com/allocated-eni` | 分配的弹性网卡 ID | `eni-bp1xxxxxxxxxxxxx` |
| `k8s.aliyun.com/allocated-eip` | 分配的弹性公网 IP | `47.100.x.x` |
| `k8s.aliyun.com/eni-max` | 节点最大 ENI 数 | `6` |
| `k8s.aliyun.com/ip-max` | 节点最大 IP 数 | `30` |
| `k8s.aliyun.com/allocated-eni` | 已分配 ENI 数 | `4` |
| `k8s.aliyun.com/allocated-ip` | 已分配 IP 数 | `20` |

## 附录 B: Terway 常用 CLI 命令

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 进入 terway Pod 执行诊断
kubectl exec -it -n kube-system <terway-pod> -- /bin/sh

# 查看资源池状态
terway-cli show

# 查看弹性网卡详细信息
terway-cli show eni

# 手动触发垃圾回收
terway-cli garbage-collect

# 同步网络配置
terway-cli sync

# 查看帮助
terway-cli --help
```
## 附录 C: 阿里云 ECS 实例规格 ENI 限制速查

| 实例规格族 | 最大 ENI 数 | 单 ENI 最大辅助 IP | 总 IP 容量 |
|------------|-------------|-------------------|------------|
| ecs.g7/c7/r7 | 10 | 20 | 210 |
| ecs.g6/c6/r6 | 10 | 20 | 210 |
| ecs.g5/c5/r5 | 8 | 20 | 168 |
| ecs.g6e/c6e | 15 | 20 | 300 |
| ecs.u1 | 3 | 10 | 30 |

> **注意**：具体数值以阿里云官方文档为准，不同地域和可用区可能有差异。

## Related

- 08-docker-troubleshooting-guide
- 16-troubleshooting-guide
- [[系统基础/速查卡/go.md|go]]
- [[系统基础/速查卡/k8s.md|k8s]]
- [[skills/ts-networking.md|ts-networking]]
- [[生态参考/领域索引/terway-index.md|Terway 知识图谱索引]]
- [[生态参考/领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]

## See Also

- [[故障诊断/高级排障/03-networking/05-service-mesh-istio-troubleshooting.md|05-service-mesh-istio-troubleshooting]]
- [[故障诊断/高级排障/03-networking/06-gateway-api-troubleshooting.md|06-gateway-api-troubleshooting]]
- [[故障诊断/高级排障/03-networking/08-flannel-troubleshooting.md|08-flannel-troubleshooting]]
- [[故障诊断/高级排障/03-networking/09-higress-troubleshooting.md|09-higress-troubleshooting]]

```

<!-- risk-assessed -->
