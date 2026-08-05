---
title: 问题排查决策树 Mermaid 可视化集
description: '# 问题排查决策树 Mermaid 可视化集'
summary: '# 问题排查决策树 Mermaid 可视化集'
category: general
tags:
- k8s
- etcd
- apiserver
- kubelet
- scheduler
- prometheus
- coredns
- elasticsearch
- hpa
- vpa
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 问题排查决策树 Mermaid 可视化集 是什么
- 如何 问题排查决策树 Mermaid 可视化集
trigger_keywords:
- 问题排查决策树
- Mermaid
- 可视化集
prerequisites:
- kubectl-basics
- prometheus-basics
- etcd-basics
- logging-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 问题排查决策树 Mermaid 可视化集

> **版本**: v1.0
> **创建日期**: 2026-05-18
> **用途**: 将 63 篇问题排查文档的决策树转换为 Mermaid 图形化格式，便于 on-call 快速扫描
> **覆盖**: 18 个核心 Skill 对应的决策树

---

## 1. Node NotReady 决策树

```mermaid
flowchart TD
    START(["Node NotReady 告警"]) --> T1{检查节点状态}
    T1 -->|kubectl get nodes| T2{STATUS?}
    T2 -->|NotReady| T3{检查 Conditions}
    T2 -->|Unknown| T3
    T3 -->|DiskPressure=True| P1["RC-004: 磁盘空间不足"]
    T3 -->|MemoryPressure=True| P2["RC-005: 内存压力"]
    T3 -->|PIDPressure=True| P3["RC-006: PID 不足"]
    T3 -->|NetworkUnavailable| P4["RC-007: 网络配置问题"]
    T3 -->|Ready=False 但无特殊压力| T4{检查 kubelet 日志}
    T4 -->|connection refused| P5["RC-001: kubelet 证书过期"]
    T4 -->|kubelet not responding| P6["RC-002: kubelet 进程异常"]
    T4 -->|PLEG unhealthy| P7["RC-003: 容器运行时异常"]
    T4 -->|no error, just timeout| P8["RC-008: 网络分区"]
    T5{检查 Events} -->|NodeNotReady 事件| T6{持续时间?}
    T5 -->|无 Events| T7{检查 Lease}
    T6 -->|>5min| P9["RC-009: 持续性节点问题"]
    T6 -->|<5min| P10["RC-010: 瞬时抖动"]
    T7 -->|Lease 未更新| P5
    T7 -->|Lease 正常| P11["RC-011: 时钟偏差"]

    P1 --> VERIFY["验证恢复"]
    P2 --> VERIFY
    P3 --> VERIFY
    P4 --> VERIFY
    P5 --> VERIFY
    P6 --> VERIFY
    P7 --> VERIFY
    P8 --> VERIFY
    P9 --> ESCALATE["升级人工处理"]
    P10 --> MONITOR["监控等待"]
    P11 --> FIX["检查 NTP 配置"]
```

### 关键命令速查

| 步骤 | 命令 | 预期结果 |
|------|------|---------|
| T1 | `kubectl get nodes -o wide` | 查看 STATUS 和 AGE |
| T3 | `kubectl describe node <node>` | 查看 Conditions |
| T4 | `journalctl -u kubelet --since "10m"` | 查看 kubelet 日志 |
| T7 | `kubectl get lease -n kube-node-lease <node>` | 检查 Lease 更新时间 |

---

## 2. Pod CrashLoopBackOff 决策树

```mermaid
flowchart TD
    START(["Pod CrashLoopBackOff 告警"]) --> T1{检查 Pod 状态}
    T1 -->|kubectl get pods| T2{退出码?}
    T2 -->|137 = OOMKilled| R1["RC-001: 内存限制不足"]
    T2 -->|1 = 应用程序错误| T3{检查日志}
    T2 -->|128+N = 信号导致退出| T4{检查信号}
    T3 -->|OOMKilled in logs| R1
    T3 -->|permission denied| R2["RC-002: 权限问题"]
    T3 -->|command not found| R3["RC-003: 启动命令错误"]
    T3 -->|configuration error| R4["RC-004: 应用配置错误"]
    T3 -->|no specific error| T5{检查 liveness/readiness}
    T4 -->|SIGSEGV/SIGABRT| R5["RC-005: 应用崩溃/段错误"]
    T4 -->|SIGTERM = 正常终止| R6["RC-006: 正常终止但重启"]
    T5 -->|liveness probe failed| R7["RC-007: 存活探针配置错误"]
    T5 -->|readiness probe failed| R8["RC-008: 就绪探针配置错误"]
    T5 -->|both passing| T6{检查资源}
    T6 -->|memory limit 接近限制| R1
    T6 -->|CPU throttle 高| R9["RC-009: CPU 节流"]
    T6 -->|资源正常| T7{检查依赖}
    T7 -->|依赖服务不可达| R10["RC-010: 依赖服务问题"]

    R1 --> VERIFY["验证: kubectl top pod"]
    R2 --> VERIFY
    R3 --> FIX["修正启动命令"]
    R4 --> FIX
    R5 --> ESCALATE["升级应用团队"]
    R6 --> MONITOR["检查重启策略"]
    R7 --> FIX["修正探针配置"]
    R8 --> FIX
    R9 --> FIX["调整 CPU 限制"]
    R10 --> ESCALATE["检查依赖服务"]
```

---

## 3. Pod Pending 决策树

```mermaid
flowchart TD
    START(["Pod Pending 告警"]) --> T1{检查 Events}
    T1 -->|FailedScheduling| T2{错误信息?}
    T1 -->|无 Events| T3{检查调度器}
    T2 -->|Insufficient cpu/memory| R1["RC-001: 资源不足"]
    T2 -->|node(s) had taint| R2["RC-002: 节点污点不容忍"]
    T2 -->|node(s) didn't match PodAffinity| R3["RC-003: 亲和性不匹配"]
    T2 -->|pvc not found| R4["RC-004: PVC 不存在"]
    T2 -->|pvc pending| R5["RC-005: PVC 绑定中"]
    T2 -->|no nodes available| T4{检查节点状态}
    T2 -->|max pending pods reached| R6["RC-006: 调度队列满"]
    T3 -->|scheduler 不工作| R7["RC-007: 调度器问题"]
    T4 -->|存在 NotReady 节点| R8["RC-008: 节点不可用"]
    T4 -->|存在 Ready 但无足够资源| R1
    T4 -->|所有节点 Ready 但有 Taint| R2

    R1 --> FIX1["增加资源或缩减工作负载"]
    R2 --> FIX2["添加 tolerations 或移除 taint"]
    R3 --> FIX3["调整 affinity 规则"]
    R4 --> FIX4["检查 PVC 配置"]
    R5 --> FIX5["检查 StorageClass/CSI"]
    R6 --> FIX6["增加 scheduler 并行度"]
    R7 --> ESCALATE["重启调度器"]
    R8 --> FIX8["恢复节点或迁移 Pod"]
```

---

## 4. DNS 解析问题决策树

```mermaid
flowchart TD
    START(["DNS 解析失败 告警"]) --> T1{确认影响范围}
    T1 -->|仅个别 Pod| T2{检查 Pod DNS 配置}
    T1 -->|整个集群/命名空间| T3{检查 CoreDNS 状态}
    T2 -->|resolv.conf 错误| R1["RC-001: Pod DNS 配置错误"]
    T2 -->|ndots 配置问题| R2["RC-002: ndots 配置不当"]
    T2 -->|配置正常| T4{测试集群内 DNS}
    T3 -->|CoreDNS Pod NotReady| R3["RC-003: CoreDNS 异常"]
    T3 -->|CoreDNS Pod Ready 但不响应| T5{检查 CoreDNS 日志}
    T4 -->|集群内 DNS 失败| T6{检查 Service/Endpoints}
    T4 -->|集群内 DNS 正常| T7{测试外部 DNS}
    T5 -->|forward 失败| R4["RC-004: upstream DNS 问题"]
    T5 -->|health check 失败| R5["RC-005: CoreDNS 健康检查失败"]
    T5 -->|no response from cache| R6["RC-006: CoreDNS 缓存问题"]
    T6 -->|Service 存在但无 Endpoints| R7["RC-007: Service selector 不匹配"]
    T6 -->|Endpoints 存在| T8{检查网络策略}
    T7 -->|外部 DNS 失败| R8["RC-008: 上游 DNS 不可达"]
    T7 -->|外部 DNS 正常| R9["RC-009: 集群 DNS 配置问题"]
    T8 -->|NetworkPolicy 阻断| R10["RC-010: 网络策略阻断"]
    T8 -->|无 NetworkPolicy| R11["RC-011: CNI 问题"]

    R1 --> FIX1["修正 Pod DNS 配置"]
    R2 --> FIX2["调整 ndots 配置"]
    R3 --> FIX3["重启 CoreDNS Pod"]
    R4 --> FIX4["检查 upstream DNS"]
    R5 --> FIX5["检查 CoreDNS health probe"]
    R6 --> FIX6["清除 CoreDNS 缓存"]
    R7 --> FIX7["修正 Service selector"]
    R8 --> FIX8["检查网络到上游 DNS"]
    R9 --> FIX9["检查 kube-dns ConfigMap"]
    R10 --> FIX10["调整 NetworkPolicy"]
    R11 --> ESCALATE["检查 CNI 配置"]
```

---

## 5. Service 无 Endpoints 决策树

```mermaid
flowchart TD
    START(["Service 无 Endpoints 告警"]) --> T1{检查 Service 状态}
    T1 -->|kubectl get svc <name> -n <ns>| T2{selector 检查}
    T2 -->|selector 正确| T3{检查 Pod 状态}
    T2 -->|selector 错误| R1["RC-001: Service selector 配置错误"]
    T3 -->|Pod Running| T4{检查 Pod labels}
    T3 -->|Pod 非 Running| R2["RC-002: Pod 未就绪"]
    T4 -->|labels 匹配 selector| T5{检查 EndpointsSlice}
    T4 -->|labels 不匹配| R3["RC-003: Pod labels 不匹配"]
    T5 -->|EndpointsSlice 存在但为空| T6{检查 kube-proxy}
    T5 -->|EndpointsSlice 正常| R4["RC-004: 网络层问题"]
    T6 -->|kube-proxy 正常| T7{检查 iptables/IPVS}
    T6 -->|kube-proxy 异常| R5["RC-005: kube-proxy 问题"]
    T7 -->|规则缺失| R6["RC-006: kube-proxy 规则未同步"]
    T7 -->|规则存在但不通| R7["RC-007: 网络策略阻断"]

    R1 --> FIX1["修正 Service selector"]
    R2 --> FIX2["排查 Pod 未就绪原因"]
    R3 --> FIX3["修正 Pod labels"]
    R4 --> ESCALATE["检查 Ingress/网关"]
    R5 --> FIX5["重启 kube-proxy"]
    R6 --> FIX6["重启 kube-proxy"]
    R7 --> FIX7["检查 NetworkPolicy"]
```

---

## 6. 证书过期决策树

```mermaid
flowchart TD
    START(["证书过期告警/x509 错误"]) --> T1{确认证书类型}
    T1 -->|kubelet 证书| T2{检查 kubelet 日志}
    T1 -->|API Server 证书| T3{检查 API Server 状态}
    T1 -->|etcd 证书| T4{检查 etcd 状态}
    T1 -->|用户证书/kubeconfig| T5{检查 kubeconfig 有效期}
    T2 -->|kubelet 无法连接 apiserver| R1["RC-001: kubelet 证书过期"]
    T2 -->|kubelet 正常| T6{检查证书轮换配置}
    T3 -->|API Server 无法访问| R2["RC-002: API Server 证书过期"]
    T3 -->|API Server 正常| T7{检查认证配置}
    T4 -->|etcd 无法启动| R3["RC-003: etcd 证书过期"]
    T4 -->|etcd 正常| T8{检查证书分发}
    T5 -->|kubeconfig 过期| R4["RC-004: kubeconfig 过期"]
    T5 -->|kubeconfig 正常| R5["RC-005: 证书签名问题"]
    T6 -->|自动轮换未启用| R6["RC-006: 证书自动轮换未配置"]
    T6 -->|自动轮换失败| R7["RC-007: 证书轮换失败"]
    T7 -->|认证配置错误| R8["RC-008: API Server 认证配置错误"]
    T8 -->|证书分发问题| R9["RC-009: 证书分发问题"]

    R1 --> FIX1["重启 kubelet 触发轮换"]
    R2 --> FIX2["更新 API Server 证书"]
    R3 --> FIX3["更新 etcd 证书"]
    R4 --> FIX4["重新生成 kubeconfig"]
    R5 --> FIX5["检查 CA 签名"]
    R6 --> FIX6["启用证书自动轮换"]
    R7 --> FIX7["手动触发证书轮换"]
    R8 --> FIX8["修正认证配置"]
    R9 --> FIX9["检查证书分发机制"]
```

---

## 7. PVC 存储问题决策树

```mermaid
flowchart TD
    START(["PVC Pending 告警"]) --> T1{检查 PVC 状态}
    T1 -->|kubectl describe pvc <name>| T2{Pending 原因?}
    T2 -->|waiting for cache| R1["RC-001: CSI 驱动未就绪"]
    T2 -->|waiting for volumes| R2["RC-002: 存储类不存在"]
    T2 -->|waiting for backend| T3{检查存储后端}
    T2 -->|no storage class| R3["RC-003: StorageClass 未定义"]
    T3 -->|云盘存储| T4{检查云盘配额}
    T3 -->|NFS/其他| T5{检查 NFS 连接}
    T4 -->|配额超限| R4["RC-004: 云盘配额超限"]
    T4 -->|配额正常| R5["RC-005: 云盘存储驱动异常"]
    T5 -->|NFS 不可达| R6["RC-006: NFS 服务器不可达"]
    T5 -->|NFS 正常| R7["RC-007: CSI 驱动问题"]

    R1 --> FIX1["等待 CSI 驱动就绪"]
    R2 --> FIX2["创建缺失的 StorageClass"]
    R3 --> FIX3["定义 StorageClass"]
    R4 --> FIX4["增加云盘配额或清理"]
    R5 --> FIX5["检查云盘驱动 Pod"]
    R6 --> FIX6["恢复 NFS 连接"]
    R7 --> FIX7["检查 CSI driver 日志"]
```

---

## 8. Deployment 滚动更新卡住决策树

```mermaid
flowchart TD
    START(["Deployment 滚动更新卡住"]) --> T1{检查 Deployment 状态}
    T1 -->|kubectl rollout status| T2{状态?}
    T2 -->|Waiting for minReadySeconds| R1["RC-001: 最小就绪时间未到"]
    T2 -->|Waiting for rollout finish| T3{检查 ReplicaSet}
    T2 -->|error: deployment timeout| T4{检查 Pod 状态}
    T3 -->|Old ReplicaSet 未停止| R2["RC-002: 滚动更新策略问题"]
    T3 -->|新 ReplicaSet 未创建| R3["RC-003: 镜像拉取失败"]
    T4 -->|Pod 处于 CrashLoop| R4["RC-004: 新版本应用错误"]
    T4 -->|Pod 处于 Pending| R5["RC-005: 资源不足"]
    T4 -->|Pod 就绪但探针失败| R6["RC-006: 探针配置问题"]
    T4 -->|ImagePullBackOff| R3

    R1 --> MONITOR["等待 minReadySeconds"]
    R2 --> FIX2["调整 RollingUpdate 策略"]
    R3 --> FIX3["检查镜像配置"]
    R4 --> FIX4["回滚到上一版本"]
    R5 --> FIX5["增加资源或缩减"]
    R6 --> FIX6["修正探针配置"]
```

---

## 9. RBAC/Quota 问题决策树

```mermaid
flowchart TD
    START(["Forbidden/Quota exceeded 错误"]) --> T1{确认错误类型}
    T1 -->|Forbidden| T2{检查权限}
    T1 -->|ResourceQuota exceeded| T3{检查配额}
    T2 -->|ServiceAccount 权限不足| R1["RC-001: RBAC 规则缺失"]
    T2 -->|用户权限不足| R2["RC-002: User RBAC 问题"]
    T2 -->|Webhook 拒绝| R3["RC-003: 准入控制器拒绝"]
    T3 -->|CPU quota exceeded| R4["RC-004: CPU 配额不足"]
    T3 -->|Memory quota exceeded| R5["RC-005: Memory 配额不足"]
    T3 -->|Pod 数量配额 exceeded| R6["RC-006: Pod 数量配额不足"]

    R1 --> FIX1["创建/更新 Role/ClusterRole"]
    R2 --> FIX2["调整用户 RBAC 绑定"]
    R3 --> FIX3["检查准入控制器配置"]
    R4 --> FIX4["申请更多 CPU 配额"]
    R5 --> FIX5["申请更多 Memory 配额"]
    R6 --> FIX6["申请更多 Pod 配额"]
```

---

## 10. HPA/VPA 弹性伸缩问题决策树

```mermaid
flowchart TD
    START(["HPA 不触发扩容告警"]) --> T1{检查 HPA 状态}
    T1 -->|kubectl describe hpa <name>| T2{HPA 状态?}
    T2 -->|ScalingActive=False| T3{检查原因}
    T2 -->|ScalingActive=True 但无扩容| T4{检查指标}
    T3 -->|Unable to read metrics| R1["RC-001: Metrics Server 异常"]
    T3 -->|backoff off| R2["RC-002: HPA backoff"]
    T3 -->|min replicas reached| R3["RC-003: 已达最小副本"]
    T4 -->|指标正常但低于阈值| R4["RC-004: 实际负载低于扩容阈值"]
    T4 -->|指标异常| R1
    T4 -->|指标正常且高于阈值| T5{检查 Pod 配置}

    R1 --> FIX1["检查 Metrics Server 部署"]
    R2 --> FIX2["等待 backoff 结束或调整"]
    R3 --> FIX3["调整 min replicas"]
    R4 --> MONITOR["正常行为，监控"]
    R5 --> FIX5["检查副本数配置"]
    T5 -->|资源请求未设置| R5["RC-005: Pod 未设置资源请求"]
    T5 -->|资源请求已设置| R6["RC-006: HPA 配置问题"]

    R5 --> FIX5["为 Pod 添加资源请求"]
    R6 --> FIX6["修正 HPA 阈值配置"]
```

---

## 11. Ingress/Gateway 问题决策树

```mermaid
flowchart TD
    START(["Ingress 4xx/5xx 错误"]) --> T1{确认错误码}
    T1 -->|404| T2{检查路由配置}
    T1 -->|502/503| T3{检查后端服务}
    T1 -->|timeout| T4{检查网络连通}
    T2 -->|路径不匹配| R1["RC-001: Ingress 路径配置错误"]
    T2 -->|后端 service 不存在| R2["RC-002: Service 名称错误"]
    T2 -->|路径匹配但无 backend| R3["RC-003: Ingress backend 配置错误"]
    T3 -->|Service 无 Endpoints| R4["RC-004: Service 无后端 Pod"]
    T3 -->|Service 有 Endpoints 但不通| T5{检查 kube-proxy}
    T3 -->|后端 Pod 健康但响应慢| R5["RC-005: 后端应用问题"]
    T4 -->|Ingress Controller Pod 不健康| R6["RC-006: Ingress Controller 问题"]
    T4 -->|网络策略阻断| R7["RC-007: 网络策略阻断"]
    T5 -->|kube-proxy 正常| T6{检查安全组/防火墙}
    T5 -->|kube-proxy 异常| R8["RC-008: kube-proxy 问题"]
    T6 -->|安全组问题| R9["RC-009: 云安全组限制"]
    T6 -->|防火墙正常| R10["RC-010: 跨节点网络问题"]

    R1 --> FIX1["修正 Ingress 路径"]
    R2 --> FIX2["修正 Service 名称"]
    R3 --> FIX3["修正 Ingress backend"]
    R4 --> FIX4["检查后端 Pod"]
    R5 --> ESCALATE["检查后端应用"]
    R6 --> FIX6["重启 Ingress Controller"]
    R7 --> FIX7["调整 NetworkPolicy"]
    R8 --> FIX8["重启 kube-proxy"]
    R9 --> FIX9["调整安全组规则"]
    R10 --> ESCALATE["检查 CNI/网络"]
```

---

## 12. 镜像拉取失败决策树

```mermaid
flowchart TD
    START(["ImagePullBackOff/ErrImagePull"]) --> T1{确认错误类型}
    T1 -->|ImagePullBackOff| T2{检查镜像名称}
    T1 -->|ErrImagePull| T2
    T2 -->|镜像名称正确| T3{检查凭证}
    T2 -->|镜像名称错误| R1["RC-001: 镜像名称拼写错误"]
    T3 -->|需要认证| T4{检查 imagePullSecrets}
    T3 -->|公开镜像| T5{检查网络}
    T4 -->|Secret 不存在| R2["RC-002: imagePullSecrets 未配置"]
    T4 -->|Secret 存在但无效| R3["RC-003: 镜像仓库凭证过期"]
    T5 -->|registry 不存在| R4["RC-004: 镜像仓库地址错误"]
    T5 -->|registry 存在但拉取超时| T6{检查网络}
    T6 -->|DNS 解析失败| R5["RC-005: DNS 问题"]
    T6 -->|TCP 连接超时| R6["RC-006: 网络隔离/防火墙"]
    T6 -->|TLS 握手失败| R7["RC-007: 证书问题"]

    R1 --> FIX1["修正镜像名称"]
    R2 --> FIX2["配置 imagePullSecrets"]
    R3 --> FIX3["更新镜像仓库凭证"]
    R4 --> FIX4["使用正确的仓库地址"]
    R5 --> FIX5["检查集群 DNS 配置"]
    R6 --> FIX6["检查网络策略/防火墙"]
    R7 --> FIX7["检查仓库证书"]
```

---

## 13. 控制平面问题决策树

```mermaid
flowchart TD
    START(["API Server/etcd 异常"]) --> T1{确认影响范围}
    T1 -->|仅部分组件异常| T2{检查组件日志}
    T1 -->|所有组件异常| T3{检查 etcd 状态}
    T2 -->|API Server 日志异常| R1["RC-001: API Server 异常"]
    T2 -->|Scheduler 日志异常| R2["RC-002: Scheduler 异常"]
    T2 -->|Controller Manager 异常| R3["RC-003: Controller Manager 异常"]
    T3 -->|etcd 无法连接| T4{检查 etcd 健康}
    T3 -->|etcd 响应慢| T5{检查 etcd 性能}
    T4 -->|etcd 未运行| R4["RC-004: etcd 未运行"]
    T4 -->|etcd 无法选主| R5["RC-005: etcd leader 选举失败"]
    T5 -->|磁盘 I/O 高| R6["RC-006: etcd 磁盘延迟"]
    T5 -->|网络延迟高| R7["RC-007: etcd 网络问题"]
    T5 -->|请求超时| R8["RC-008: etcd 资源不足"]

    R1 --> FIX1["重启 API Server Pod"]
    R2 --> FIX2["重启 Scheduler"]
    R3 --> FIX3["重启 Controller Manager"]
    R4 --> FIX4["检查 etcd 进程和配置"]
    R5 --> FIX5["检查 etcd 日志解决选举问题"]
    R6 --> FIX6["优化 etcd 磁盘或增加资源"]
    R7 --> FIX7["检查 etcd 网络配置"]
    R8 --> FIX8["增加 etcd 资源"]
```

---

## 14. 性能瓶颈决策树

```mermaid
flowchart TD
    START(["响应延迟高/资源使用率高"]) --> T1{确认瓶颈位置}
    T1 -->|API Server 延迟高| T2{检查 API Server}
    T1 -->|应用响应慢| T3{检查应用层}
    T1 -->|数据库响应慢| T4{检查数据库}
    T2 -->|etcd 延迟高| R1["RC-001: etcd 性能问题"]
    T2 -->|请求积压| R2["RC-002: API Server 请求过多"]
    T2 -->|CPU/内存瓶颈| R3["RC-003: API Server 资源不足"]
    T3 -->|Pod CPU 高| T5{检查应用 CPU}
    T3 -->|Pod 内存高| T6{检查应用内存}
    T3 -->|网络延迟高| T7{检查网络}
    T4 -->|数据库连接池满| R4["RC-004: DB 连接池耗尽"]
    T4 -->|慢查询| R5["RC-005: 数据库慢查询"]
    T5 -->|CPU 节流| R6["RC-006: CPU limit 过低"]
    T5 -->|实际计算量大| R7["RC-007: 应用算法问题"]
    T6 -->|内存泄漏| R8["RC-008: 内存泄漏"]
    T6 -->|GC 频繁| R9["RC-009: GC 问题"]
    T7 -->|跨节点延迟高| R10["RC-010: 网络问题"]
    T7 -->|DNS 解析慢| R11["RC-011: DNS 问题"]

    R1 --> FIX1["优化 etcd 性能"]
    R2 --> FIX2["优化客户端 LIST 请求"]
    R3 --> FIX3["增加 API Server 资源"]
    R4 --> FIX4["增加连接池或优化查询"]
    R5 --> FIX5["优化查询或增加资源"]
    R6 --> FIX6["增加 CPU limit"]
    R7 --> ESCALATE["优化应用算法"]
    R8 --> ESCALATE["排查内存泄漏"]
    R9 --> FIX9["优化 GC 配置"]
    R10 --> ESCALATE["检查网络 CNI"]
    R11 --> FIX11["检查 CoreDNS 配置"]
```

---

## 15. 配置管理问题决策树

```mermaid
flowchart TD
    START(["ConfigMap/Secret 未生效"]) --> T1{检查挂载状态}
    T1 -->|kubectl describe pod| T2{挂载情况?}
    T2 -->|Volume not mounted| R1["RC-001: 未挂载 Volume"]
    T2 -->|Volume mounted 但文件不存在| T3{检查 Volume 来源}
    T2 -->|环境变量未设置| T4{检查环境变量配置}
    T3 -->|ConfigMap 不存在| R2["RC-002: ConfigMap 不存在"]
    T3 -->|ConfigMap 存在但不匹配| R3["RC-003: ConfigMap 键名错误"]
    T4 -->|envFrom 未正确引用| R4["RC-004: envFrom 配置错误"]
    T4 -->|env 数值未正确引用| R5["RC-005: env 配置错误"]

    R1 --> FIX1["添加 Volume 挂载"]
    R2 --> FIX2["创建 ConfigMap"]
    R3 --> FIX3["修正 ConfigMap 键名"]
    R4 --> FIX4["修正 envFrom 配置"]
    R5 --> FIX5["修正 env 配置"]
```

---

## 16. 日志收集问题决策树

```mermaid
flowchart TD
    START(["日志缺失/收集中断"]) --> T1{检查日志收集状态}
    T1 -->|[[fluentd|Fluentd]]/Fluent Bit Pod| T2{Pod 状态?}
    T2 -->|Pod 未运行| R1["RC-001: 日志收集 Pod 未运行"]
    T2 -->|Pod 运行但不发送| T3{检查配置}
    T3 -->|Input 配置错误| R2["RC-002: 日志输入配置错误"]
    T3 -->|Output 无法到达| T4{检查 Output}
    T4 -->|Elasticsearch 不可达| R3["RC-003: 输出端不可达"]
    T4 -->|权限不足| R4["RC-004: 输出端认证失败"]
    T3 -->|Filter 配置错误| R5["RC-005: 日志过滤配置错误"]

    R1 --> FIX1["重启日志收集 Pod"]
    R2 --> FIX2["修正 Input 配置"]
    R3 --> FIX3["检查 Elasticsearch 连接"]
    R4 --> FIX4["更新认证信息"]
    R5 --> FIX5["修正 Filter 配置"]
```

---

## 17. 监控告警问题决策树

```mermaid
flowchart TD
    START(["Prometheus 指标缺失/告警不触发"]) --> T1{确认影响范围}
    T1 -->|单个服务指标缺失| T2{检查服务暴露}
    T1 -->|所有指标缺失| T3{检查 Prometheus}
    T2 -->|metrics endpoint 不可用| R1["RC-001: 应用未暴露指标"]
    T2 -->|metrics endpoint 可用| T4{检查 ServiceMonitor}
    T3 -->|Prometheus Pod 不健康| R2["RC-002: Prometheus 异常"]
    T3 -->|Prometheus 正常但无数据| T5{检查存储}
    T4 -->|ServiceMonitor 未选择服务| R3["RC-003: ServiceMonitor selector 错误"]
    T4 -->|ServiceMonitor 正确| R4["RC-004: Prometheus 无法访问服务"]
    T5 -->|存储卷满| R5["RC-005: Prometheus 存储满"]
    T5 -->|存储正常| R6["RC-006: scrape 配置问题"]

    R1 --> FIX1["在应用中暴露 /metrics"]
    R2 --> FIX2["重启 Prometheus Pod"]
    R3 --> FIX3["修正 ServiceMonitor selector"]
    R4 --> FIX4["检查网络/认证"]
    R5 --> FIX5["清理存储或扩容"]
    R6 --> FIX6["修正 scrape 配置"]
```

---

## 18. 安全事件决策树

```mermaid
flowchart TD
    START(["安全告警/异常访问"]) --> T1{确认告警类型}
    T1 -->|未授权访问告警| T2{检查访问来源}
    T1 -->|异常行为告警| T3{检查 Pod 行为}
    T1 -->|审计日志异常| T4{检查审计日志}
    T2 -->|来自外部 IP| R1["RC-001: 外部攻击"]
    T2 -->|来自集群内部| T5{检查 ServiceAccount}
    T3 -->|异常网络连接| R2["RC-002: 容器异常网络活动"]
    T3 -->|异常进程行为| R3["RC-003: 容器内异常进程"]
    T4 -->|异常 API 调用| R4["RC-004: 异常 API 操作"]
    T4 -->|凭据使用异常| R5["RC-005: 凭据滥用"]
    T5 -->|ServiceAccount 异常| R6["RC-006: ServiceAccount 被盗用"]
    T5 -->|正常 ServiceAccount| R7["RC-007: 误报或测试流量"]

    R1 --> ESCALATE["封锁来源 IP，升级安全团队"]
    R2 --> ESCALATE["隔离可疑 Pod，进行取证"]
    R3 --> ESCALATE["隔离 Pod，检查容器进程"]
    R4 --> FIX4["审查审计日志，识别攻击者"]
    R5 --> FIX5["轮换凭据，检查泄露源"]
    R6 --> ESCALATE["禁用可疑 ServiceAccount，轮换令牌"]
    R7 --> MONITOR["标记为误报，监控后续"]
```

---

## 使用说明

### 如何使用本决策树

1. **定位入口**: 根据告警/症状找到对应的决策树
2. **按图索骥**: 沿决策树路径执行检查
3. **快速定位**: 每个分支对应一个根因 (RC-xxx)
4. **修复验证**: 修复后使用对应命令验证

### 与 Skills 的对应关系

| 决策树 | 主要 Skill | 辅助 Skill |
|--------|-----------|-----------|
| Node NotReady | SKILL-NODE-001 | SKILL-CP-001, SKILL-SEC-001 |
| Pod CrashLoop | SKILL-POD-001 | SKILL-SEC-001 |
| Pod Pending | SKILL-POD-002 | SKILL-NET-001, SKILL-STORE-001 |
| DNS 解析问题 | SKILL-NET-001 | SKILL-NET-002 |
| Service 无 Endpoints | SKILL-NET-002 | SKILL-NET-001 |
| 证书过期 | SKILL-SEC-001 | SKILL-NODE-001 |
| PVC 存储问题 | SKILL-STORE-001 | SKILL-CP-001 |
| Deployment 卡住 | SKILL-WORK-001 | SKILL-POD-001 |
| RBAC/Quota | SKILL-SEC-002 | SKILL-POD-002 |
| HPA/VPA | SKILL-SCALE-001 | SKILL-POD-001 |
| Ingress/Gateway | SKILL-NET-003 | SKILL-SEC-001 |
| 镜像拉取失败 | SKILL-IMAGE-001 | SKILL-POD-002 |
| 控制平面问题 | SKILL-CP-001 | SKILL-SEC-001 |
| 性能瓶颈 | SKILL-PERF-001 | - |
| 配置管理 | SKILL-CONFIG-001 | SKILL-POD-001 |
| 日志收集 | SKILL-LOG-001 | SKILL-MONITOR-001 |
| 监控告警 | SKILL-MONITOR-001 | - |
| 安全事件 | SKILL-SECURITY-001 | SKILL-NODE-001 |

---

**关联文档**:
- [domain-10-troubleshooting-diagnostics/topic-skills/](../domain-10-troubleshooting-diagnostics/技能体系/) — 18 个 GA Skill
- [domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/](../domain-10-troubleshooting-diagnostics/高级排障/) — 63 篇问题排查文档
- [P0-1: 工单分类体系](./P0-1-ticket-classification-intent-recognition.md)
- [P0-2: 多技能协同协议](./P0-2-multi-skill-coordination-protocol.md)

<!-- risk-assessed -->
