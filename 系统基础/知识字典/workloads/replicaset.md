---
title: ReplicaSet
description: '## 概述'
summary: '## 概述'
category: dictionary
tags:
- k8s
- glossary
- terminology
- hpa
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- ReplicaSet 是什么
- 如何 ReplicaSet
trigger_keywords:
- ReplicaSet
- dictionary
prerequisites:
- kubectl-basics
- pod-lifecycle
- cloud-provider-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# ReplicaSet

## 概述
ReplicaSet 的作用是维护一组稳定运行的 Pod 副本。它通常不直接使用，而是由 Deployment 自动管理，作为 Deployment 实现 Pod 创建、更新和扩缩容的底层机制。

## 核心概念/原理
- **核心字段**：
  - `spec.replicas`：目标副本数，默认 1。
  - `spec.selector`：标签选择器，用于识别和获取受管理的 Pod。必须与 `spec.template.metadata.labels` 匹配，创建后不可变。
  - `spec.template`：Pod 模板，`restartPolicy` 只能为 `Always`。
- **Pod 获取机制**：ReplicaSet 不仅管理自己创建的 Pod，也会立即获取与其选择器匹配且无控制器 OwnerReference（或 OwnerReference 非控制器）的裸 Pod。
- **Pod 替换**：当受管 Pod 被删除或终止时（如节点问题、维护），ReplicaSet 会自动创建替代 Pod。

## 关键机制或特性
- **扩缩容**：通过修改 `spec.replicas` 即可手动扩缩容；也支持作为 HPA 的缩放目标。
- **删除策略**：
  - 默认 `kubectl delete rs` 会级联删除所有 Pod。
  - 使用 `--cascade=orphan` 可仅删除 ReplicaSet 而保留 Pod；后续创建同名选择器的 ReplicaSet 可收养这些 Pod。
- **缩容算法**：缩容时按以下优先级选择要删除的 Pod：
  1. 未调度或 Pending 的 Pod
  2. 节点上该控制器 Pod 密度较高的
  3. 创建时间较新的
  4. `controller.[[实体/kubernetes.md|[[Kubernetes|kubernetes]]]].io/pod-deletion-cost` 注解值较低的（Beta，默认启用）
  5. 随机选择
- **终止副本追踪（Beta）**：`DeploymentReplicaSetTerminatingReplicas` 启用后，可通过 `.status.terminatingReplicas` 查看终止中副本数。

## 使用场景
- 作为 Deployment 的底层实现，绝大多数情况下应通过 Deployment 间接使用。
- 仅在需要自定义更新编排或根本不需要更新时，才考虑直接使用 ReplicaSet。

## 最佳实践/注意事项
- **推荐做法**：日常应用管理应使用 Deployment，而非直接操作 ReplicaSet。
- 避免创建标签与现有 ReplicaSet 选择器重叠的裸 Pod，否则会被意外收养并可能导致终止。
- ReplicaSet 本身不支持滚动更新；如需受控更新，请使用 Deployment。
- 可利用 `pod-deletion-cost` 注解影响缩容时优先保留的 Pod。

## 实战 YAML 示例

```yaml
apiVersion: apps/v1
kind: ReplicaSet
metadata:
  name: web-frontend
  namespace: prod
  labels:
    app: web-frontend
    tier: frontend
spec:
  replicas: 3
  selector:
    matchLabels:
      app: web-frontend
  template:
    metadata:
      labels:
        app: web-frontend
        tier: frontend
      annotations:
        # 设置 pod-deletion-cost，值越高越不容易被缩容时删除
        controller.kubernetes.io/pod-deletion-cost: "100"
    spec:
      containers:
      - name: web
        image: myregistry.com/web-frontend:v1.0
        ports:
        - containerPort: 80
        resources:
          requests:
            cpu: "100m"
            memory: "128Mi"
          limits:
            cpu: "500m"
            memory: "256Mi"
        readinessProbe:
          httpGet:
            path: /healthz
            port: 80
          periodSeconds: 10
```

> **注意**: 上述示例仅用于理解 ReplicaSet 结构。生产环境中应使用 Deployment 代替直接创建 ReplicaSet。

## 故障排查

### ReplicaSet 副本数不符合预期
- **症状**: `kubectl get rs` 显示 `DESIRED` 与 `READY` 不一致。
- **常见原因**: Pod 调度失败（资源不足）；Pod 启动失败（镜像错误、探针失败）。
- **诊断命令**:
  ```bash
  # 查看 ReplicaSet 状态
  kubectl get rs -n prod -l app=web-frontend
  
  # 查看 ReplicaSet 事件
  kubectl describe rs web-frontend -n prod | tail -20
  
  # 查看相关 Pod 状态
  kubectl get pods -n prod -l app=web-frontend -o wide
  ```

### 裸 Pod 被 ReplicaSet 意外收养
- **症状**: 手动创建的 Pod 被某个 ReplicaSet 获取并管理。
- **常见原因**: Pod 标签与 ReplicaSet 选择器匹配，且 Pod 没有 OwnerReference。
- **诊断命令**:
  ```bash
  # 查看 Pod 的 ownerReferences
  kubectl get pod <pod-name> -n prod -o jsonpath='{.metadata.ownerReferences}'
  ```
- **解决方案**: 为手动创建的 Pod 使用不与任何控制器重叠的标签。

### Deployment 下有多个 ReplicaSet
- **症状**: 一个 Deployment 关联了多个 ReplicaSet，其中旧的 RS 副本数为 0。
- **说明**: 这是正常行为。Deployment 每次更新 Pod 模板都会创建新的 ReplicaSet，旧的 RS 保留用于回滚。数量受 `revisionHistoryLimit` 控制。

## 生产就绪检查清单

- [ ] 通过 Deployment 间接管理 ReplicaSet（而非直接创建）
- [ ] Pod 模板配置了 `resources.requests/limits`
- [ ] 标签选择器不与其他控制器重叠
- [ ] 了解缩容算法，合理使用 `pod-deletion-cost` 注解
- [ ] Deployment 的 `revisionHistoryLimit` 设置合理，控制旧 ReplicaSet 数量

## 命令快速参考

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 查看 ReplicaSet 列表
kubectl get rs -n prod

# 查看特定 Deployment 关联的所有 ReplicaSet
kubectl get rs -n prod -l app=web-api --sort-by=.metadata.creationTimestamp

# 手动扩缩容 ReplicaSet
kubectl scale rs web-frontend -n prod --replicas=5

# 查看 ReplicaSet 详情
kubectl describe rs <rs-name> -n prod

# 查看 Pod 的 OwnerReference（确认由哪个 RS 管理）
kubectl get pod <pod-name> -n prod -o jsonpath='{.metadata.ownerReferences[0].name}'
```
## 交叉引用

- [[系统基础/知识字典/workloads/deployments.md|Deployments]]](./deployments.md)
- [工作负载概览与架构](../../工作负载/01-workload-overview-architecture.md)
- [HPA 水平自动扩缩](./horizontal-pod-autoscaling.md)
- [工作负载管理总览](./workload-management.md)
- [[系统基础/知识字典/workloads/replicationcontroller.md|ReplicationController]]（旧版）](./replicationcontroller.md)

## 参考链接
- https://kubernetes.io/docs/concepts/workloads/controllers/replicaset/

## Related

- [[系统基础/知识字典/workloads/advanced-pod-configuration.md|Advanced Pod Configuration]]
- [[系统基础/知识字典/workloads/automatic-cleanup-for-finished-jobs.md|Automatic Cleanup for Finished Jobs]]
- [[系统基础/知识字典/workloads/autoscaling-workloads.md|Autoscaling Workloads]]


<!-- risk-assessed -->
