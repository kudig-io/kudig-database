---
title: 05 - 调度与抢占事件
description: '# 05 - 调度与抢占事件'
summary: '表示 Pod 已成功通过调度器的所有过滤和打分阶段,并被绑定到特定的节点上。这是 Pod 生命周期中最关键的事件之一,标志着 Pod 从 Pending 状态进入 Running 状态的转折点。调度器会记录调度决策的节点名称和调度耗时。'
category: kubernetes-events
tags:
- k8s
- events
- troubleshooting
- apiserver
- kubelet
- scheduler
- controller-manager
- coredns
- docker
- vpa
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 技术支持
estimated_read_time: 10min
intent_queries:
- 调度与抢占事件 是什么
- 如何 调度与抢占事件
- Kubernetes 33 kubernetes events 最佳实践
trigger_keywords:
- 调度与抢占事件
- kubernetes
- events
prerequisites:
- kubectl-basics
- cloud-provider-basics
- gpu-scheduling-basics
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
authors:
- name: KUDIG Team
  role: contributor
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 05 - 调度与抢占事件

> **适用版本**: [[kubernetes|Kubernetes]] v1.25 - v1.32 | **最后更新**: 2026-02 | **作者**: Allen Galler

> **本文档详细记录 kube-scheduler 和 node-controller 产生的所有调度与抢占相关事件,帮助运维人员快速定位和解决调度问题。**

---

<!-- chunk: 📋 事件速查表 -->## 📋 事件速查表

| 事件原因 | 中文名 | 类型 | 来源组件 | 生产频率 | 版本 | 典型场景 |
|:--------|:------|:-----|:--------|:--------|:-----|:--------|
| `Scheduled` | 调度成功 | Normal | scheduler | 高频 | v1.0+ | Pod 成功绑定到节点 |
| `FailedScheduling` | 调度失败 | Warning | scheduler | 中频 | v1.0+ | 无可用节点/资源不足 |
| `Preempted` | 被抢占 | Normal | scheduler | 低频 | v1.11+ | 高优先级 Pod 抢占 |
| `WaitingForGates` | 等待调度门 | Normal | scheduler | 低频 | v1.26+ | 自定义调度控制 |
| `TaintManagerEviction` | 污点驱逐 | Normal | node-controller | 低频 | v1.13+ | 节点状态异常驱逐 |
| `FailedBinding` | 绑定失败 | Warning | scheduler | 罕见 | v1.0+ | 调度绑定冲突 |

---

<!-- chunk: 🔄 调度器工作流程 -->## 🔄 调度器工作流程

```
┌─────────────────────────────────────────────────────────────────┐
│                    kube-scheduler 调度流程                        │
└─────────────────────────────────────────────────────────────────┘
                              │
                    ┌─────────▼─────────┐
                    │  Pod 创建(Pending) │
                    └─────────┬─────────┘
                              │
                    ┌─────────▼──────────┐
                    │  调度前检查         │
                    │  - SchedulingGates │
                    │  - PriorityClass   │
                    └─────────┬──────────┘
                              │
                              ├─[有 Gates]──> [WaitingForGates]
                              │
                    ┌─────────▼──────────┐
                    │  过滤阶段 (Filter)  │
                    │  - Resource Fit    │
                    │  - Node Affinity   │
                    │  - Taints/Tolerations│
                    │  - Topology Spread │
                    │  - Volume Binding  │
                    └─────────┬──────────┘
                              │
                    ┌─────────▼──────────┐
                    │  打分阶段 (Score)   │
                    │  - 负载均衡         │
                    │  - 资源分布         │
                    └─────────┬──────────┘
                              │
                    ┌─────────▼──────────┐
                    │  选择最优节点       │
                    └─────────┬──────────┘
                              │
                    ┌─────────▼──────────┐
                    │  抢占检查(若失败)   │
                    │  - 优先级比较       │
                    │  - 抢占可行性       │
                    └─────────┬──────────┘
                              │
                ┌─────────────┼─────────────┐
                │             │             │
        [无可用节点]      [抢占成功]      [调度成功]
                │             │             │
                ▼             ▼             ▼
      [FailedScheduling] [Preempted]  [Scheduled]
                                           │
                              ┌────────────▼────────────┐
                              │  绑定到节点 (Bind)       │
                              └────────────┬────────────┘
                                           │
                                    [Pod Running]
```

---

<!-- chunk: 📌 事件详细说明 -->## 📌 事件详细说明

## `Scheduled` - 调度成功

| 属性 | 说明 |
|:---|:---|
| **事件类型** | Normal |
| **来源组件** | default-scheduler |
| **关联资源** | Pod |
| **适用版本** | v1.0+ |
| **生产频率** | 高频 |

## 事件含义
表示 Pod 已成功通过调度器的所有过滤和打分阶段,并被绑定到特定的节点上。这是 Pod 生命周期中最关键的事件之一,标志着 Pod 从 Pending 状态进入 Running 状态的转折点。调度器会记录调度决策的节点名称和调度耗时。

此事件产生后,[[kubelet|kubelet]] 将接管 Pod 的后续生命周期管理,包括镜像拉取、容器创建、健康检查等。调度成功并不代表 Pod 一定能成功运行,后续仍可能因镜像拉取失败、资源限制等问题导致启动失败。

## 典型事件消息
```yaml
Type:    Normal
Reason:  Scheduled
Message: Successfully assigned default/nginx-deployment-7d4c8c6d9f-xkj2m to node-10-0-1-15
Source:  default-scheduler
```

## 影响面说明
- **资源预留**: 节点资源已被预留,影响后续 Pod 调度
- **调度延迟**: 调度耗时影响 Pod 启动速度(正常 < 100ms)
- **绑定不可逆**: 调度绑定后无法更改,除非删除 Pod 重建

## 排查建议
1. **检查调度延迟**
   ```bash
   # 查看调度耗时(CreationTimestamp -> Scheduled Event)
   kubectl get events --field-selector involvedObject.name=<pod-name> \
     --sort-by='.lastTimestamp' | grep Scheduled
   
   # 调度耗时分析
   kubectl describe pod <pod-name> | grep -A5 Events
   ```

2. **验证节点选择合理性**
   ```bash
   # 查看 Pod 调度到的节点
   kubectl get pod <pod-name> -o wide
   
   # 检查节点资源使用率
   kubectl top node <node-name>
   kubectl describe node <node-name> | grep -A5 "Allocated resources"
   ```

3. **检查调度约束是否生效**
   ```bash
   # 查看 Pod 的 nodeSelector/affinity/tolerations
   kubectl get pod <pod-name> -o yaml | grep -A20 "nodeSelector|affinity|tolerations"
   ```

## 解决建议
| 原因 | 解决方案 | 优先级 |
|:----|:--------|:------|
| 调度延迟过高 (>1s) | 检查调度器性能、减少节点数量、优化调度策略 | P2 |
| 调度到不期望节点 | 添加 nodeSelector/affinity 约束、更新节点标签 | P3 |
| 节点资源不均衡 | 调整 Scheduler 打分策略、使用 Descheduler 重平衡 | P3 |
| 调度器配置问题 | 检查调度器配置文件、插件启用状态 | P1 |

---

## `FailedScheduling` - 调度失败

| 属性 | 说明 |
|:---|:---|
| **事件类型** | Warning |
| **来源组件** | default-scheduler |
| **关联资源** | Pod |
| **适用版本** | v1.0+ |
| **生产频率** | 中频 |

## 事件含义
表示调度器无法为 Pod 找到满足所有调度约束的节点。这是生产环境中最常见的调度问题,原因多样且复杂,包括资源不足、节点污点、亲和性约束、拓扑分布约束、存储卷绑定失败、端口冲突等。Pod 将保持在 Pending 状态,调度器会持续重试(默认退避间隔 1s-60s)。

调度失败的根本原因是**所有节点都无法通过 Filter 阶段的某个或多个过滤插件**。生产环境中需要根据失败原因快速定位问题,避免影响业务部署。调度器会记录每个节点的失败原因,格式为 `0/N nodes are available: <reason1>, <reason2>...`。

## 典型事件消息
```yaml
# 资源不足示例
Type:    Warning
Reason:  FailedScheduling
Message: 0/3 nodes are available: 1 Insufficient cpu, 2 Insufficient memory. preemption: 0/3 nodes are available: 3 No preemption victims found for incoming pod.

# 污点容忍示例
Type:    Warning
Reason:  FailedScheduling
Message: 0/5 nodes are available: 5 node(s) had untolerated taint {node-role.kubernetes.io/master: }.

# 亲和性约束示例
Type:    Warning
Reason:  FailedScheduling
Message: 0/4 nodes are available: 4 node(s) didn't match Pod's node affinity/selector.

# 拓扑分布约束示例
Type:    Warning
Reason:  FailedScheduling
Message: 0/6 nodes are available: 6 node(s) didn't match pod topology spread constraints.

# PVC 绑定失败示例
Type:    Warning
Reason:  FailedScheduling
Message: 0/3 nodes are available: 3 node(s) had volume node affinity conflict.

# 端口冲突示例
Type:    Warning
Reason:  FailedScheduling
Message: 0/2 nodes are available: 2 node(s) didn't have free ports for the requested pod ports.
```

## FailedScheduling 失败原因分类表

| 失败原因关键字 | 中文说明 | 原因分类 | 生产频率 | 解决难度 |
|:-------------|:--------|:--------|:--------|:--------|
| `Insufficient cpu` | CPU 资源不足 | 资源不足 | 高频 | 中 |
| `Insufficient memory` | 内存资源不足 | 资源不足 | 高频 | 中 |
| `Insufficient [[Pods|pods]]` | 节点 Pod 数量超限 | 资源不足 | 中频 | 低 |
| `Insufficient ephemeral-storage` | 临时存储不足 | 资源不足 | 中频 | 中 |
| `Insufficient nvidia.com/gpu` | GPU 资源不足 | 扩展资源不足 | 中频 | 高 |
| `untolerated taint` | 节点污点不容忍 | 污点约束 | 高频 | 低 |
| `didn't match Pod's node affinity/selector` | 节点亲和性不匹配 | 亲和性约束 | 中频 | 低 |
| `didn't match pod affinity rules` | Pod 亲和性不满足 | 亲和性约束 | 低频 | 中 |
| `didn't match pod anti-affinity rules` | Pod 反亲和性冲突 | 反亲和性约束 | 中频 | 中 |
| `didn't match pod topology spread constraints` | 拓扑分布约束不满足 | 拓扑约束 | 中频 | 中 |
| `had volume node affinity conflict` | 存储卷节点亲和性冲突 | 存储绑定 | 中频 | 中 |
| `persistentvolumeclaim "xxx" not found` | PVC 不存在 | 存储绑定 | 低频 | 低 |
| `didn't have free ports for the requested pod ports` | 端口冲突 | 端口占用 | 低频 | 中 |
| `node(s) had taint {key: value}, that the pod didn't tolerate` | 特定污点不容忍 | 污点约束 | 高频 | 低 |
| `node(s) not ready` | 节点未就绪 | 节点状态 | 中频 | 高 |
| `node(s) were unschedulable` | 节点不可调度 | 节点状态 | 中频 | 中 |
| `No preemption victims found` | 无法抢占低优先级 Pod | 抢占失败 | 低频 | 高 |
| `didn't find available persistent volumes to bind` | 无可用 PV 绑定 | 存储绑定 | 中频 | 中 |

## 影响面说明
- **业务中断**: Pod 无法启动,影响服务可用性
- **调度重试**: 调度器持续重试,消耗 API Server 资源
- **资源浪费**: 资源配置不合理导致碎片化
- **级联失败**: 亲和性约束可能导致关联 Pod 全部失败

## 排查建议

1. **快速定位失败原因**
   ```bash
   # 查看 Pod 事件获取失败详情
   kubectl describe pod <pod-name> | grep -A10 "Events:"
   
   # 过滤调度失败事件
   kubectl get events --field-selector reason=FailedScheduling,involvedObject.name=<pod-name>
   
   # 查看所有 Pending Pod 的调度失败原因
   kubectl get pods --field-selector status.phase=Pending -A
   kubectl describe pod -A | grep -B5 "FailedScheduling"
   ```

2. **资源不足排查**
   ```bash
   # 检查集群资源使用情况
   kubectl top nodes
   kubectl describe nodes | grep -A5 "Allocated resources"
   
   # 查看节点可分配资源
   kubectl get nodes -o json | jq '.items[] | {name: .metadata.name, allocatable: .status.allocatable}'
   
   # 计算集群总可用资源
   kubectl get nodes -o json | jq '[.items[] | .status.allocatable | {cpu: .cpu, memory: .memory}]'
   
   # 查看 Pod 资源请求
   kubectl get pod <pod-name> -o json | jq '.spec.containers[] | {name: .name, resources: .resources.requests}'
   ```

3. **污点容忍排查**
   ```bash
   # 查看所有节点污点
   kubectl get nodes -o json | jq '.items[] | {name: .metadata.name, taints: .spec.taints}'
   
   # 查看 Pod 污点容忍配置
   kubectl get pod <pod-name> -o yaml | grep -A10 tolerations
   
   # 检查特定污点的节点
   kubectl get nodes -o json | jq '.items[] | select(.spec.taints != null) | select(.spec.taints[] | .key == "node-role.kubernetes.io/master") | .metadata.name'
   ```

4. **亲和性约束排查**
   ```bash
   # 查看 Pod 亲和性配置
   kubectl get pod <pod-name> -o yaml | grep -A20 affinity
   
   # 检查节点标签是否匹配
   kubectl get nodes --show-labels
   kubectl get pod <pod-name> -o yaml | grep -A5 nodeSelector
   
   # 检查 Pod 反亲和性冲突
   kubectl get pods -o wide --all-namespaces | grep <node-name>
   kubectl get pod <pod-name> -o yaml | grep -A15 podAntiAffinity
   ```

5. **拓扑分布约束排查**
   ```bash
   # 查看 Pod 拓扑分布约束
   kubectl get pod <pod-name> -o yaml | grep -A10 topologySpreadConstraints
   
   # 检查拓扑域分布情况
   kubectl get pods -o wide --all-namespaces | grep <app-label>
   kubectl get nodes -o json | jq '.items[] | {name: .metadata.name, zone: .metadata.labels["topology.kubernetes.io/zone"]}'
   ```

6. **存储卷绑定排查**
   ```bash
   # 检查 PVC 状态
   kubectl get pvc -A
   kubectl describe pvc <pvc-name>
   
   # 查看 PV 的节点亲和性
   kubectl get pv -o yaml | grep -A10 nodeAffinity
   
   # 检查 StorageClass 配置
   kubectl get storageclass
   kubectl describe storageclass <storageclass-name>
   ```

7. **端口冲突排查**
   ```bash
   # 查看 Pod 请求的端口
   kubectl get pod <pod-name> -o yaml | grep -A5 "hostPort"
   
   # 检查节点上已占用的端口
   kubectl get pods -o wide --all-namespaces | grep <node-name>
   kubectl get pods -A -o json | jq '.items[] | select(.spec.nodeName == "<node-name>") | .spec.containers[] | select(.ports != null) | .ports[] | select(.hostPort != null) | .hostPort'
   ```

8. **调度器日志分析**
   ```bash
   # 查看调度器日志(kubeadm 集群)
   kubectl logs -n kube-system -l component=kube-scheduler --tail=100 | grep <pod-name>
   
   # 查看详细调度决策(需启用 --v=5 日志级别)
   kubectl logs -n kube-system kube-scheduler-<node> --tail=500 | grep -A20 "Attempting to schedule pod"
   ```

## 解决建议

| 原因 | 解决方案 | 优先级 | 影响范围 |
|:----|:--------|:------|:--------|
| **资源不足 - CPU** | 1. 扩容节点增加 CPU 资源<br>2. 降低 Pod CPU requests<br>3. 清理闲置 Pod<br>4. 启用 Cluster Autoscaler | P0 | 高 |
| **资源不足 - 内存** | 1. 扩容节点增加内存资源<br>2. 降低 Pod memory requests<br>3. 优化应用内存使用<br>4. 启用垂直扩缩容(VPA) | P0 | 高 |
| **资源不足 - Pod 数量** | 1. 增加节点 `--max-pods` 参数<br>2. 扩容节点分散 Pod<br>3. 减少 DaemonSet 数量 | P1 | 中 |
| **资源不足 - 临时存储** | 1. 清理节点临时文件<br>2. 增加节点磁盘空间<br>3. 降低 Pod ephemeral-storage requests | P1 | 中 |
| **资源不足 - GPU** | 1. 添加 GPU 节点<br>2. 优化 GPU 共享策略<br>3. 使用 GPU 切片技术 | P1 | 中 |
| **污点不容忍** | 1. 为 Pod 添加对应的 tolerations<br>2. 移除节点不必要的污点<br>3. 调整污点效果(NoSchedule -> PreferNoSchedule) | P1 | 低 |
| **节点亲和性不匹配** | 1. 修改 Pod nodeSelector 或 nodeAffinity<br>2. 为节点添加匹配的标签<br>3. 使用 preferredDuringScheduling 软约束 | P2 | 低 |
| **Pod 反亲和性冲突** | 1. 调整反亲和性拓扑域(node -> zone)<br>2. 使用软反亲和性(preferred)<br>3. 增加节点以满足分布需求 | P2 | 中 |
| **拓扑分布约束不满足** | 1. 调整 maxSkew 容忍度<br>2. 修改 whenUnsatisfiable 为 ScheduleAnyway<br>3. 增加拓扑域(zone/region)节点数量 | P2 | 中 |
| **存储卷节点亲和性冲突** | 1. 检查 PV 的 nodeAffinity 配置<br>2. 使用 WaitForFirstConsumer 绑定模式<br>3. 迁移 PV 到正确的拓扑域 | P1 | 高 |
| **PVC 不存在或未绑定** | 1. 创建缺失的 PVC<br>2. 检查 StorageClass 是否存在<br>3. 确认 PV 供应正常 | P0 | 高 |
| **端口冲突** | 1. 移除 hostPort 配置<br>2. 使用 Service 代替 hostPort<br>3. 调度到其他节点 | P2 | 低 |
| **节点未就绪** | 1. 修复节点问题(kubelet/docker/网络)<br>2. 标记节点为 Unschedulable<br>3. 从集群移除问题节点 | P0 | 高 |
| **节点不可调度** | 1. 移除节点 Unschedulable 标记<br>2. 检查节点是否在维护中<br>3. 确认节点是否被 drain | P1 | 中 |
| **无法抢占低优先级 Pod** | 1. 检查 PriorityClass 配置<br>2. 确认是否有可抢占的 Pod<br>3. 扩容节点避免抢占 | P2 | 中 |

---

## `Preempted` - 被抢占

| 属性 | 说明 |
|:---|:---|
| **事件类型** | Normal |
| **来源组件** | default-scheduler |
| **关联资源** | Pod |
| **适用版本** | v1.11+ |
| **生产频率** | 低频 |

## 事件含义
表示当前 Pod 因集群资源不足,被更高优先级的 Pod 抢占而被驱逐。这是 Kubernetes 优先级调度机制的核心体现,确保高优先级工作负载(如生产业务)能够优先获得资源,牺牲低优先级工作负载(如批处理任务)。

抢占流程:当高优先级 Pod 无法调度时,调度器会评估是否可以通过删除低优先级 Pod 来腾出资源。如果抢占可行,调度器会选择影响最小的节点和 Pod 组合,然后向 API Server 发送删除请求,被抢占的 Pod 会收到 Preempted 事件并进入 Terminating 状态。

## 典型事件消息
```yaml
Type:    Normal
Reason:  Preempted
Message: Preempted by default/high-priority-pod on node node-10-0-1-20

# 详细消息包含优先级信息
Type:    Normal
Reason:  Preempted
Message: Preempted in order to admit pod "default/critical-app-abc123" on node "worker-node-5". This pod has priority 1000 while preempting pod has priority 10000.
```

## 影响面说明
- **服务中断**: 被抢占 Pod 立即终止,影响低优先级服务
- **资源浪费**: 频繁抢占导致 Pod 重启,浪费计算资源
- **调度延迟**: 被抢占 Pod 重新调度需要等待资源
- **级联影响**: 可能触发多个 Pod 被抢占

## PriorityClass 说明

Kubernetes 通过 PriorityClass 资源定义 Pod 优先级:

```yaml
apiVersion: scheduling.k8s.io/v1
kind: PriorityClass
metadata:
  name: high-priority
value: 10000              # 优先级值,越大越高
globalDefault: false       # 是否为默认优先级
description: "High priority for production workloads"
preemptionPolicy: PreemptLowerPriority  # 抢占策略
```

**系统预定义 PriorityClass**:
- `system-cluster-critical`: 优先级 2000000000(集群核心组件,如 coredns)
- `system-node-critical`: 优先级 2000001000(节点核心组件,如 kube-proxy)

**抢占策略 (preemptionPolicy)**:
- `PreemptLowerPriority` (默认): 可以抢占低优先级 Pod
- `Never` (v1.19+): 高优先级但不抢占,仅排队等待

**Pod 使用 PriorityClass**:
```yaml
apiVersion: v1
kind: Pod
metadata:
  name: critical-app
spec:
  priorityClassName: high-priority
  containers:
  - name: app
    image: nginx
```

## 排查建议

1. **确认抢占详情**
   ```bash
   # 查看被抢占 Pod 的事件
   kubectl describe pod <preempted-pod-name> | grep -A5 Preempted
   
   # 查看抢占者 Pod 信息
   kubectl get pod <preemptor-pod-name> -o yaml | grep priorityClassName
   kubectl get priorityclass <priority-class-name>
   ```

2. **检查优先级配置**
   ```bash
   # 查看所有 PriorityClass
   kubectl get priorityclass
   kubectl describe priorityclass <priority-class-name>
   
   # 查看被抢占 Pod 的优先级
   kubectl get pod <pod-name> -o json | jq '.spec.priority, .spec.priorityClassName'
   
   # 查看集群中高优先级 Pod
   kubectl get pods -A -o json | jq '.items[] | select(.spec.priority > 1000) | {name: .metadata.name, namespace: .metadata.namespace, priority: .spec.priority}'
   ```

3. **分析抢占历史**
   ```bash
   # 查看集群中所有抢占事件
   kubectl get events -A --field-selector reason=Preempted --sort-by='.lastTimestamp'
   
   # 统计抢占频率
   kubectl get events -A --field-selector reason=Preempted -o json | jq '[.items[] | {pod: .involvedObject.name, time: .lastTimestamp}]'
   ```

4. **评估资源压力**
   ```bash
   # 检查集群资源使用率
   kubectl top nodes
   kubectl describe nodes | grep -A5 "Allocated resources"
   
   # 检查高优先级 Pod 的资源请求
   kubectl get pods -A -o json | jq '.items[] | select(.spec.priority > 5000) | {name: .metadata.name, resources: .spec.containers[].resources.requests}'
   ```

## 解决建议

| 原因 | 解决方案 | 优先级 |
|:----|:--------|:------|
| 集群资源不足导致频繁抢占 | 扩容集群,增加节点资源 | P0 |
| 优先级配置不合理 | 重新设计 PriorityClass 体系,避免过大差距 | P1 |
| 低优先级 Pod 被频繁抢占 | 提升关键业务优先级,降低批处理优先级 | P2 |
| 抢占策略过于激进 | 使用 PreemptionPolicy: Never 禁用抢占 | P2 |
| 资源预留不足 | 为关键业务预留节点资源(taints + tolerations) | P1 |
| 突发高优先级 Pod 过多 | 限制高优先级 Pod 的并发数量(ResourceQuota) | P2 |

---

## `WaitingForGates` - 等待调度门

| 属性 | 说明 |
|:---|:---|
| **事件类型** | Normal |
| **来源组件** | default-scheduler |
| **关联资源** | Pod |
| **适用版本** | v1.26+ (Beta), v1.30 (GA) |
| **生产频率** | 低频 |

## 事件含义
表示 Pod 因配置了 SchedulingGates 而暂停调度,等待外部控制器移除调度门后才会进入正常调度流程。这是 Kubernetes v1.26 引入的新特性,允许自定义调度控制逻辑,实现复杂的编排场景,如批量任务调度、多集群协调、配额检查等。

SchedulingGates 提供了一种声明式的调度暂停机制,相比 PodScheduled Condition 更加简洁和灵活。Pod 创建时如果 `spec.schedulingGates` 非空,调度器会跳过该 Pod,直到所有 Gate 被移除。这允许外部系统在 Pod 调度前进行预处理,如资源预留、配额验证、依赖检查等。

## SchedulingGates 说明

**基本用法**:
```yaml
apiVersion: v1
kind: Pod
metadata:
  name: gated-pod
spec:
  schedulingGates:
  - name: "example.com/resource-quota-check"
  - name: "example.com/multi-cluster-placement"
  containers:
  - name: nginx
    image: nginx
```

**典型应用场景**:
1. **批量调度**: 等待所有 Pod 创建完成后统一调度(Gang Scheduling)
2. **资源配额**: 外部系统检查配额后再允许调度
3. **多集群协调**: 跨集群资源预留和调度协调
4. **依赖检查**: 等待依赖服务就绪后再调度
5. **审批流程**: 需要人工或自动审批后才调度

**移除 SchedulingGate**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 使用 kubectl patch 移除特定 gate
kubectl patch pod gated-pod --type=json -p='[{"op": "remove", "path": "/spec/schedulingGates/0"}]'

# 使用 client-go 或自定义 controller 自动化管理
```
## 典型事件消息
```yaml
Type:    Normal
Reason:  WaitingForGates
Message: Scheduling is blocked on 2 gates: [example.com/resource-quota-check example.com/multi-cluster-placement]

# Gate 被移除后产生的事件
Type:    Normal
Reason:  SchedulingGatesRemoved
Message: All scheduling gates have been removed, proceeding with scheduling.
```

## 影响面说明
- **调度延迟**: Pod 处于 Pending 状态直到 Gate 移除
- **控制器依赖**: 依赖外部控制器正确移除 Gate
- **故障排查**: Gate 未移除会导致 Pod 永久 Pending

## 排查建议

1. **检查调度门状态**
   ```bash
   # 查看 Pod 的调度门配置
   kubectl get pod <pod-name> -o yaml | grep -A5 schedulingGates
   
   # 查看等待调度门的所有 Pod
   kubectl get pods -A -o json | jq '.items[] | select(.spec.schedulingGates != null) | {name: .metadata.name, namespace: .metadata.namespace, gates: .spec.schedulingGates}'
   
   # 查看 WaitingForGates 事件
   kubectl get events --field-selector reason=WaitingForGates -A
   ```

2. **检查控制器状态**
   ```bash
   # 查看负责移除 Gate 的控制器日志
   kubectl logs -n <controller-namespace> <controller-pod> --tail=100
   
   # 检查控制器是否正常运行
   kubectl get pods -n <controller-namespace>
   ```

3. **手动移除调度门**

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

   ```bash
   # 查看当前 Gate 列表
   kubectl get pod <pod-name> -o json | jq '.spec.schedulingGates'
   
   # 移除第一个 Gate
   kubectl patch pod <pod-name> --type=json -p='[{"op": "remove", "path": "/spec/schedulingGates/0"}]'
   
   # 移除所有 Gates
   kubectl patch pod <pod-name> --type=json -p='[{"op": "remove", "path": "/spec/schedulingGates"}]'
   ```

4. **检查 API Server 版本支持**
   ```bash
   # 确认集群版本支持 SchedulingGates (v1.26+)
   kubectl version --short
   
   # 检查 API 版本
   kubectl api-resources | grep schedulinggates
   ```

## 解决建议

| 原因 | 解决方案 | 优先级 |
|:----|:--------|:------|
| 控制器未正常工作 | 重启控制器,检查日志排查问题 | P0 |
| Gate 名称拼写错误 | 修正 Gate 名称,确保与控制器匹配 | P1 |
| 控制器逻辑错误 | 修复控制器逻辑,确保正确移除 Gate | P0 |
| Gate 永久未移除 | 手动移除 Gate,允许调度继续 | P1 |
| 集群版本不支持 | 升级集群到 v1.26+,或移除 SchedulingGates 配置 | P2 |
| 配额检查失败 | 调整资源配额或 Pod 资源请求 | P2 |

---

## `TaintManagerEviction` - 污点驱逐

| 属性 | 说明 |
|:---|:---|
| **事件类型** | Normal |
| **来源组件** | node-controller |
| **关联资源** | Pod |
| **适用版本** | v1.13+ |
| **生产频率** | 低频 |

## 事件含义
表示 Pod 因节点污点(Taint)导致不再容忍(Tolerate)而被 Taint Manager 驱逐。这是 Kubernetes 节点状态异常处理机制的关键部分,当节点出现问题(如 NotReady、磁盘压力、内存压力)时,kube-controller-manager 的 node-controller 组件会为节点添加污点,并根据 Pod 的容忍配置决定是否驱逐。

Taint Manager 是 node-controller 的一部分,负责监控节点污点变化,评估每个 Pod 的容忍时间(tolerationSeconds),并在超时后删除 Pod。这种机制比传统的节点故障检测更加灵活和可控,允许不同的 Pod 有不同的容忍策略。

## 典型事件消息
```yaml
# 节点 NotReady 驱逐
Type:    Normal
Reason:  TaintManagerEviction
Message: Marking for deletion Pod default/nginx-7d4c8c6d9f-xkj2m due to NoExecute taint node.kubernetes.io/not-ready:NoExecute on node node-10-0-1-15

# 节点磁盘压力驱逐
Type:    Normal
Reason:  TaintManagerEviction
Message: Marking for deletion Pod default/app-5b9c7d-8xm9q due to NoExecute taint node.kubernetes.io/disk-pressure:NoExecute on node worker-3

# 节点内存压力驱逐
Type:    Normal
Reason:  TaintManagerEviction
Message: Marking for deletion Pod default/web-6f8d5c-4nk2p due to NoExecute taint node.kubernetes.io/memory-pressure:NoExecute on node worker-5
```

## 常见节点污点类型

| 污点键名 | 污点效果 | 触发条件 | 默认容忍时间 | 生产频率 |
|:--------|:--------|:--------|:-----------|:--------|
| `node.kubernetes.io/not-ready` | NoExecute | 节点 NotReady 状态 | 300s | 高频 |
| `node.kubernetes.io/unreachable` | NoExecute | 节点失联(网络不可达) | 300s | 中频 |
| `node.kubernetes.io/disk-pressure` | NoSchedule/NoExecute | 节点磁盘压力 | 无自动驱逐 | 中频 |
| `node.kubernetes.io/memory-pressure` | NoSchedule/NoExecute | 节点内存压力 | 无自动驱逐 | 中频 |
| `node.kubernetes.io/pid-pressure` | NoSchedule/NoExecute | 节点 PID 压力 | 无自动驱逐 | 低频 |
| `node.kubernetes.io/network-unavailable` | NoSchedule/NoExecute | 节点网络不可用 | 无自动驱逐 | 低频 |
| `node.kubernetes.io/unschedulable` | NoSchedule | 节点被标记为不可调度 | N/A | 中频 |

**Pod 默认容忍配置**:
```yaml
# Kubernetes 自动为 Pod 添加的默认容忍
tolerations:
- key: node.kubernetes.io/not-ready
  operator: Exists
  effect: NoExecute
  tolerationSeconds: 300
- key: node.kubernetes.io/unreachable
  operator: Exists
  effect: NoExecute
  tolerationSeconds: 300
```

**自定义容忍配置**:
```yaml
apiVersion: v1
kind: Pod
metadata:
  name: tolerant-pod
spec:
  tolerations:
  - key: node.kubernetes.io/not-ready
    operator: Exists
    effect: NoExecute
    tolerationSeconds: 600  # 容忍 10 分钟
  - key: node.kubernetes.io/disk-pressure
    operator: Exists
    effect: NoExecute
    tolerationSeconds: 0    # 立即驱逐
  - key: custom-taint
    operator: Equal
    value: "true"
    effect: NoExecute       # 永久容忍(无 tolerationSeconds)
  containers:
  - name: nginx
    image: nginx
```

## 影响面说明
- **服务中断**: Pod 被驱逐导致服务中断
- **迁移延迟**: Pod 重新调度到其他节点需要时间
- **级联影响**: 节点问题可能导致大量 Pod 同时驱逐
- **数据丢失**: StatefulSet Pod 驱逐可能导致数据访问中断

## 排查建议

1. **确认驱逐原因**
   ```bash
   # 查看 Pod 驱逐事件
   kubectl describe pod <pod-name> | grep -A5 TaintManagerEviction
   
   # 查看节点污点状态
   kubectl describe node <node-name> | grep -A5 Taints
   kubectl get node <node-name> -o json | jq '.spec.taints'
   ```

2. **检查节点状态**
   ```bash
   # 查看节点状态和条件
   kubectl get nodes
   kubectl describe node <node-name> | grep -A10 Conditions
   
   # 检查节点资源压力
   kubectl top node <node-name>
   kubectl describe node <node-name> | grep -A5 "Allocated resources"
   ```

3. **检查 Pod 容忍配置**
   ```bash
   # 查看 Pod 容忍配置
   kubectl get pod <pod-name> -o yaml | grep -A20 tolerations
   
   # 检查驱逐时间计算
   kubectl get events --field-selector involvedObject.name=<pod-name> --sort-by='.lastTimestamp'
   ```

4. **查看节点污点历史**
   ```bash
   # 查看节点事件历史
   kubectl describe node <node-name> | grep -A20 Events
   
   # 查看所有 TaintManagerEviction 事件
   kubectl get events -A --field-selector reason=TaintManagerEviction --sort-by='.lastTimestamp'
   ```

5. **检查 node-controller 配置**
   ```bash
   # 查看 kube-controller-manager 配置
   kubectl get pods -n kube-system kube-controller-manager-<node> -o yaml | grep -A5 "pod-eviction-timeout|node-monitor-grace-period"
   
   # 默认配置:
   # --pod-eviction-timeout=5m0s (v1.13+ 已废弃,由 taint 容忍时间控制)
   # --node-monitor-grace-period=40s
   ```

## 解决建议

| 原因 | 解决方案 | 优先级 |
|:----|:--------|:------|
| 节点问题(NotReady/Unreachable) | 修复节点问题(kubelet/网络/硬件),恢复节点就绪状态 | P0 |
| 节点磁盘压力 | 清理节点磁盘空间,增加磁盘容量,配置日志轮转 | P0 |
| 节点内存压力 | 驱逐非关键 Pod,增加节点内存,优化 Pod 内存使用 | P0 |
| 容忍时间过短 | 增加 Pod tolerationSeconds,给予更长恢复时间 | P2 |
| 关键 Pod 被误驱逐 | 为关键 Pod 配置永久容忍(无 tolerationSeconds) | P1 |
| 节点频繁抖动 | 调整 node-monitor-grace-period,增加节点稳定性检测时间 | P2 |
| 自定义污点驱逐 | 检查自定义污点逻辑,确保符合业务需求 | P2 |

---

## `FailedBinding` - 绑定失败

| 属性 | 说明 |
|:---|:---|
| **事件类型** | Warning |
| **来源组件** | default-scheduler |
| **关联资源** | Pod |
| **适用版本** | v1.0+ |
| **生产频率** | 罕见 |

## 事件含义
表示调度器已为 Pod 选择了目标节点,但在执行绑定操作(Binding)时失败。这是一个罕见但严重的错误,通常由于并发冲突、API Server 问题、权限问题或调度器内部错误导致。与 FailedScheduling 不同,FailedBinding 发生在调度决策完成后的绑定阶段。

绑定流程:调度器通过 POST 请求向 API Server 发送 Binding 对象,将 Pod.spec.nodeName 设置为目标节点。如果绑定失败,Pod 会回退到 Pending 状态,调度器会重新调度。常见失败原因包括 Pod 已被其他调度器绑定、节点已被删除、API Server 不可达等。

## 典型事件消息
```yaml
# 绑定冲突示例
Type:    Warning
Reason:  FailedBinding
Message: Binding rejected: Pod "default/nginx-7d4c8c6d9f-xkj2m" is already bound to node "node-10-0-1-20"

# API Server 不可达示例
Type:    Warning
Reason:  FailedBinding
Message: Failed to bind pod: Post "https://apiserver:6443/api/v1/namespaces/default/pods/nginx/binding": dial tcp: lookup apiserver: no such host

# 节点不存在示例
Type:    Warning
Reason:  FailedBinding
Message: Failed to bind pod: node "worker-node-99" not found
```

## 影响面说明
- **调度失败**: Pod 无法启动,持续 Pending
- **资源浪费**: 调度决策完成但无法绑定,浪费调度资源
- **并发冲突**: 可能反映集群并发控制问题
- **API Server 问题**: 可能是 API Server 不稳定的信号

## 排查建议

1. **确认绑定失败详情**
   ```bash
   # 查看 Pod 事件获取详细错误
   kubectl describe pod <pod-name> | grep -A10 FailedBinding
   
   # 查看 Pod 当前绑定状态
   kubectl get pod <pod-name> -o yaml | grep nodeName
   kubectl get pod <pod-name> -o json | jq '.spec.nodeName, .status.phase'
   ```

2. **检查调度器状态**
   ```bash
   # 查看调度器日志
   kubectl logs -n kube-system -l component=kube-scheduler --tail=100 | grep -i "binding|error"
   
   # 检查调度器运行状态
   kubectl get pods -n kube-system -l component=kube-scheduler
   
   # 检查是否有多个调度器实例
   kubectl get pods -n kube-system --field-selector status.phase=Running | grep scheduler
   ```

3. **检查 API Server 连通性**
   ```bash
   # 测试 API Server 连接
   kubectl cluster-info
   kubectl get --raw /healthz
   
   # 查看 API Server 日志
   kubectl logs -n kube-system kube-apiserver-<node> --tail=100 | grep -i "error|binding"
   ```

4. **检查节点状态**
   ```bash
   # 确认目标节点存在
   kubectl get node <node-name>
   
   # 查看节点是否可调度
   kubectl describe node <node-name> | grep "Unschedulable|Taints"
   ```

5. **检查并发调度器**
   ```bash
   # 查看集群中所有调度器
   kubectl get pods -A | grep scheduler
   
   # 检查 Pod schedulerName 配置
   kubectl get pod <pod-name> -o yaml | grep schedulerName
   ```

6. **检查 RBAC 权限**
   ```bash
   # 查看调度器 ServiceAccount 权限
   kubectl get clusterrolebinding | grep scheduler
   kubectl describe clusterrole system:kube-scheduler
   
   # 测试调度器权限
   kubectl auth can-i create pods/binding --as=system:kube-scheduler -n default
   ```

## 解决建议

| 原因 | 解决方案 | 优先级 |
|:----|:--------|:------|
| Pod 已绑定到其他节点 | 删除 Pod 重建,或检查是否有多个调度器冲突 | P1 |
| 目标节点不存在 | 清理已删除节点的遗留数据,重新调度 | P1 |
| API Server 不可达 | 检查网络连接,修复 API Server 问题 | P0 |
| 调度器权限不足 | 修复 RBAC 配置,确保调度器有 pods/binding 权限 | P0 |
| 并发调度冲突 | 确保只有一个默认调度器实例,或使用 Leader Election | P1 |
| 调度器内部错误 | 重启调度器,升级调度器版本,检查日志排查 Bug | P1 |
| API Server 限流 | 调整 API Priority and Fairness 配置,增加调度器优先级 | P2 |

---

<!-- chunk: 🔍 跨场景排查建议 -->## 🔍 跨场景排查建议

## 1. 大规模 Pod 调度失败排查
``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 统计所有 Pending Pod 的失败原因分布
kubectl get pods -A --field-selector status.phase=Pending -o json | \
  jq -r '.items[] | [.metadata.namespace, .metadata.name] | @tsv' | \
  while read ns name; do
    kubectl describe pod -n $ns $name | grep "FailedScheduling" | tail -1
  done | sort | uniq -c | sort -rn

# 快速识别集群瓶颈
kubectl describe nodes | grep -A5 "Allocated resources" | grep -E "cpu|memory" | \
  awk '{print $2, $3}' | sed 's/[()]//g' | \
  awk '{sum+=$1; count++} END {print "平均资源使用率:", sum/count "%"}'
```
## 2. 调度延迟分析
``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 计算 Pod 调度耗时(创建到调度成功)
kubectl get events --field-selector involvedObject.name=<pod-name> \
  --sort-by='.firstTimestamp' -o json | \
  jq -r '.items | map(select(.reason == "Scheduled" or .reason == "FailedScheduling")) | 
    .[0].firstTimestamp as $start | 
    .[-1].lastTimestamp as $end | 
    "\($start) -> \($end)"'
```
## 3. 节点调度能力评估
``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 计算每个节点还能调度多少 Pod(基于 CPU)
kubectl get nodes -o json | jq -r '.items[] | 
  .metadata.name as $name | 
  (.status.allocatable.cpu | tonumber) as $allocatable | 
  (.status.capacity.cpu | tonumber) as $capacity | 
  "\($name): 可用 CPU \($allocatable) cores, 容量 \($capacity) cores"'
```
---

<!-- chunk: 📚 相关文档交叉引用 -->## 📚 相关文档交叉引用

## 相关事件文档
- **[01-pod-lifecycle-events.md](01-pod-lifecycle-events.md)** - Pod 创建、启动、删除事件
- **[02-resource-management-events.md](02-resource-management-events.md)** - OOMKilled、Evicted 事件
- **[03-volume-storage-events.md](03-volume-storage-events.md)** - PVC 绑定、挂载失败事件
- **[06-node-lifecycle-events.md](06-node-lifecycle-events.md)** - 节点 NotReady、污点管理事件

## 相关技术主题
- **[../domain-2-workload/10-pod-scheduling.md](../domain-2-workload/10-pod-scheduling.md)** - Pod 调度机制详解
- **[../domain-4-storage/20-pv-pvc-dynamic-provisioning.md](../domain-4-storage/20-pv-pvc-dynamic-provisioning.md)** - 存储动态供应与绑定
- **[../domain-3-cluster/15-node-management.md](../domain-3-cluster/15-node-management.md)** - 节点管理与污点配置
- **[../故障诊断/高级排障/structural-01-control-plane/03-scheduler-troubleshooting.md](../../19-%E6%95%85%E9%9A%9C%E8%AF%8A%E6%96%AD/04-%E9%AB%98%E7%BA%A7%E6%8E%92%E9%9A%9C/structural-01-control-plane/03-scheduler-troubleshooting.md)** - 调度器故障排查

## 相关最佳实践
- **[../系统基础/topic-dictionary/01-operations-best-practices.md](../系统基础/知识字典/01-operations-best-practices.md)** - 调度策略最佳实践
- **[../系统基础/topic-dictionary/03-performance-tuning-expert.md](../系统基础/知识字典/03-performance-tuning-expert.md)** - 调度性能优化

---

> **KUDIG-DATABASE** | Domain-33: Kubernetes Events 全域事件大全 | 文档 05/15

---

<!-- chunk: Obsidian 相关文档 -->## Obsidian 相关文档

- domain-33-kubernetes-events MOC
- [[17-系统基础/README.md|Domain-33: Kubernetes Events 全域事件大全]]
- Domain-33 K8s 事件 — 开源项目索引
- 01 - Kubernetes 事件系统架构与 API 参考
- 02 - Pod 与容器生命周期事件
- 03 - 镜像拉取事件
- 04 - 探针与健康检查事件
- 06 - 节点生命周期与状态事件
- 07 - Deployment 与 ReplicaSet 控制器事件
- 08 - StatefulSet 与 DaemonSet 控制器事件
- 09 - Job 与 CronJob 批处理事件
- 10 - Service 与网络事件

## See Also

- 03-image-pull-events
- 04-probe-health-check-events
- 06-node-lifecycle-condition-events
- 07-deployment-replicaset-events

## Related

- [[21-生态参考/03-领域索引/observability-index.md|Observability 可观测性知识图谱索引]]
- [[21-生态参考/03-领域索引/scheduler-index.md|Scheduler 调度与弹性伸缩知识图谱索引]]


<!-- risk-assessed -->
