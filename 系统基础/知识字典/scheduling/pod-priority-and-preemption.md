---
title: Pod Priority and Preemption
description: '## 概述'
summary: '## 概述'
category: dictionary
tags:
- k8s
- glossary
- terminology
- kubelet
- scheduler
- pdb
- ingress
- gateway
tier: supporting
created: '2026-05-23'
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Pod Priority and Preemption 是什么
- 如何 Pod Priority and Preemption
trigger_keywords:
- Pod
- Priority
- and
- Preemption
- dictionary
prerequisites:
- kubectl-basics
- cloud-provider-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Pod Priority and Preemption

## 概述

Pod 优先级和抢占（Pod Priority and Preemption）是 [[Kubernetes|Kubernetes]] v1.14 中达到 stable 的特性。Pod 可以具有优先级，表示该 Pod 相对于其他 Pod 的重要性。如果某个 Pod 无法被调度，调度器会尝试抢占（驱逐）优先级较低的 Pod，以使该 pending Pod 能够被调度。

## 核心概念/原理

### PriorityClass

PriorityClass 是一个非命名空间对象，定义了优先级类名称到整数值的映射。值越高，优先级越高。名称必须是有效的 DNS 子域名，且不能以 `system-` 为前缀。

- 取值范围：-2147483648 到 1000000000（32 位整数）。
- 大于 10 亿的值保留给内置的关键系统 Pod 使用。
- `globalDefault`：表示该 PriorityClass 的值应用于没有指定 `priorityClassName` 的 Pod。系统中只能有一个 `globalDefault` 为 true 的 PriorityClass。
- `preemptionPolicy`：
  - `PreemptLowerPriority`（默认）：允许该 PriorityClass 的 Pod 抢占低优先级 Pod。
  - `Never`：该 PriorityClass 的 Pod 不会抢占其他 Pod，但可能被更高优先级的 Pod 抢占。

### Pod 优先级

创建 Pod 时在 `priorityClassName` 字段指定 PriorityClass 名称。优先级准入控制器会解析并填充整数值。如果找不到对应的 PriorityClass，Pod 会被拒绝。

### 抢占（Preemption）

当调度器无法为 pending Pod 找到满足所有要求的节点时，会触发抢占逻辑。调度器会寻找这样的节点：移除一个或多个优先级低于 pending Pod 的 Pod 后，pending Pod 可以被调度到该节点上。如果找到这样的节点，低优先级 Pod 会被驱逐，然后 pending Pod 被调度到该节点。

## 关键机制或特性

- **nominatedNodeName**：当 Pod P 抢占了一个或多个 Pod 后，Pod P 状态中的 `nominatedNodeName` 字段会被设置为目标节点名称。这有助于调度器跟踪为 Pod P 预留的资源。但 Pod P 最终不一定会调度到 nominated node 上。
- **非抢占式 PriorityClass**（v1.24+ stable）：允许高优先级 Pod 在调度队列中排在低优先级 Pod 前面，但不主动抢占正在运行的 Pod，适用于希望优先但不中断现有工作的数据科学工作负载。
- **PodDisruptionBudget（PDB）支持**：调度器在抢占时尽量尊重 PDB，但这是 best effort。如果找不到不违反 PDB 的受害者，仍然会进行抢占。
- **与 QoS 的交互**：Pod 优先级和 QoS 类是两个正交的特性。调度器的抢占逻辑不考虑 QoS，但 [[kubelet|kubelet]] 的节点压力驱逐会使用优先级来确定驱逐顺序。

## 使用场景

- 关键生产服务需要确保在资源紧张时优先获得调度机会。
- 数据科学批处理作业希望在有空闲资源时优先运行，但不想打断现有工作（使用 `preemptionPolicy: Never`）。
- 集群升级或节点维护后，需要快速重新调度高优先级工作负载。

## 最佳实践/注意事项

- 在不可信用户的多租户集群中，恶意用户可能创建最高优先级的 Pod 导致其他 Pod 被驱逐。管理员应使用 ResourceQuota 限制用户创建高优先级 Pod。
- 抢占受害者的优雅终止期会产生时间差，可以通过将低优先级 Pod 的优雅终止期设置为 0 或较小值来最小化这个间隙。
- 如果 pending Pod 对节点上的低优先级 Pod 有 Pod 间亲和性，调度器不会在该节点上抢占任何 Pod。
- 调度器不支持跨节点抢占（cross node preemption）。
- 现有 Pod 在添加 `globalDefault` PriorityClass 后优先级不会自动改变，只影响之后创建的 Pod。

## 生产 YAML 示例

### PriorityClass 分级体系

```yaml
# 系统关键组件 — 不可抢占
apiVersion: scheduling.k8s.io/v1
kind: PriorityClass
metadata:
  name: system-critical
value: 1000000
globalDefault: false
preemptionPolicy: PreemptLowerPriority
description: "系统关键组件（monitoring、logging、ingress-controller）"
---
# 生产业务 — 默认优先级
apiVersion: scheduling.k8s.io/v1
kind: PriorityClass
metadata:
  name: production
value: 100000
globalDefault: true                        # 未指定 priorityClassName 的 Pod 默认使用此级别
preemptionPolicy: PreemptLowerPriority
description: "生产业务工作负载"
---
# 批处理作业 — 高优先级但不抢占
apiVersion: scheduling.k8s.io/v1
kind: PriorityClass
metadata:
  name: batch-high-priority
value: 80000
globalDefault: false
preemptionPolicy: Never                    # 排队优先但不驱逐运行中的 Pod
description: "高优先级批处理，排队靠前但不抢占"
---
# 开发/测试 — 最低优先级
apiVersion: scheduling.k8s.io/v1
kind: PriorityClass
metadata:
  name: development
value: 1000
globalDefault: false
preemptionPolicy: PreemptLowerPriority
description: "开发测试环境，资源紧张时优先被抢占"
```

### 使用 PriorityClass 的 Deployment

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: api-gateway
  namespace: production
spec:
  replicas: 3
  selector:
    matchLabels:
      app: api-gateway
  template:
    metadata:
      labels:
        app: api-gateway
    spec:
      priorityClassName: production         # 引用上面的 PriorityClass
      containers:
        - name: gateway
          image: registry.example.com/gateway:v5.1
          resources:
            requests:
              cpu: "500m"
              memory: 512Mi
            limits:
              cpu: "1"
              memory: 1Gi
```

## 故障排查

| 症状 | 可能原因 | 排查步骤 |
|------|----------|----------|
| 高优先级 Pod 仍然 Pending | 即使抢占也无法满足资源需求 | `kubectl describe pod` 查看 FailedScheduling 事件；检查节点总容量是否足够 |
| 低优先级 Pod 被意外驱逐 | 高优先级 Pod 触发抢占 | 检查 Events 中 `Preempted` 信息；审查 PriorityClass 值是否合理 |
| preemptionPolicy: Never 的 Pod 仍触发驱逐 | 误设了 PriorityClass 的 preemptionPolicy | `kubectl get priorityclass -o yaml` 确认 preemptionPolicy 值 |
| PDB 被违反 | 调度器 best-effort 尊重 PDB，但无替代方案时仍会抢占 | 检查 PDB minAvailable 设置是否合理 |
| 多个 PriorityClass 标记为 globalDefault | 只允许一个 globalDefault | `kubectl get priorityclass -o jsonpath='{range .items[?(@.globalDefault==true)]}{.metadata.name}{"\n"}{end}'` |

## 生产检查清单

- [ ] 建立 PriorityClass 分级体系（critical > production > batch > dev）
- [ ] 确保只有一个 PriorityClass 标记为 `globalDefault: true`
- [ ] 使用 ResourceQuota 限制高优先级 PriorityClass 的使用（多租户场景）
- [ ] 为关键服务配置 PodDisruptionBudget，降低抢占影响
- [ ] 数据科学 / 批处理作业使用 `preemptionPolicy: Never` 避免打断生产服务
- [ ] 审查低优先级 Pod 的 terminationGracePeriodSeconds，减少抢占延迟
- [ ] 监控 `scheduler_preemption_attempts_total` 和 `scheduler_preemption_victims` 指标

## 命令快速参考

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看所有 PriorityClass
kubectl get priorityclass

# 查看 PriorityClass 详情
kubectl get priorityclass production -o yaml

# 查看 Pod 的优先级
kubectl get pods -o custom-columns='NAME:.metadata.name,PRIORITY:.spec.priority,CLASS:.spec.priorityClassName'

# 查找被抢占的 Pod 事件
kubectl get events --field-selector reason=Preempted --all-namespaces

# 检查 globalDefault PriorityClass
kubectl get priorityclass -o jsonpath='{range .items[?(@.globalDefault==true)]}{.metadata.name}: {.value}{"\n"}{end}'

# 通过 ResourceQuota 限制高优先级 Pod
kubectl describe resourcequota -n <namespace> | grep -i priority
```
## 交叉引用

- [Kubernetes 调度器](./kubernetes-scheduler.md) — 调度队列排序受优先级影响
- [节点压力驱逐](./node-pressure-eviction.md) — kubelet 驱逐顺序同样考虑 Pod 优先级
- [API 发起驱逐](./api-initiated-eviction.md) — API 驱逐与调度器抢占的区别
- [污点与容忍度](./taints-and-tolerations.md) — 高优先级 Pod 仍需容忍节点污点

## 参考链接

- [Kubernetes 官方文档 - Pod Priority and Preemption](https://kubernetes.io/docs/concepts/scheduling-eviction/pod-priority-preemption/)

## Related

- [[系统基础/知识字典/scheduling/affinity.md|亲和性]]
- [[系统基础/知识字典/scheduling/anti-affinity.md|反亲和性]]
- [[系统基础/知识字典/scheduling/api-initiated-eviction.md|API-initiated Eviction]]


<!-- risk-assessed -->
