---
title: kube-scheduler
summary: kube-scheduler 是 Kubernetes 控制面中负责 Pod 调度的核心组件。它监听未分配节点的 Pod（.spec.nodeName
  为空），通过一系列过滤与打分算法，为其选择最合适的运行节点。调度决策的质量直接影响集群资源利用率、应用性能与节点负载均衡。
category: concepts
tags:
- core-concept
- domain-01
- visibility/public
tier: supporting
sources:
- KUDIG Gap Analysis 2026-05-21
created: 2026-05-21
updated: 2026-05-21
last_updated: 2026-05-21
status: reviewed
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# kube-scheduler

kube-scheduler 是 Kubernetes 控制面中负责 Pod 调度的核心组件。它监听未分配节点的 Pod（`.spec.nodeName` 为空），通过一系列过滤与打分算法，为其选择最合适的运行节点。调度决策的质量直接影响集群资源利用率、应用性能与节点负载均衡。

## 调度器架构

一个 Pod 的调度过程分为四个阶段：

1. **调度队列（Scheduling Queue）**：Pod 进入活跃队列（activeQ）或不可调度队列（backoff/unschedulableQ）。优先级与抢占机制在此阶段生效。高优先级 Pod 可抢占低优先级 Pod 的资源。
2. **过滤（Predicates/Filter）**：筛选出满足 Pod 所有硬性约束的节点。任一 Filter 失败，该节点即被淘汰。此阶段通常可淘汰 80% 以上的候选节点。
3. **打分（Priorities/Score）**：对通过过滤的节点进行评分，选择得分最高的节点。若多个节点同分，则随机选择一个。打分插件可自定义权重。
4. **绑定（Bind）**：将 Pod 的 `nodeName` 更新为选中节点，由 kubelet 接管后续创建容器。绑定操作是异步的，调度器继续处理下一个 Pod。

## 关键 Predicates

- **PodFitsResources**：检查节点是否有足够的 CPU、内存、存储等资源满足 Pod 的 request。这是最常见的 Pod 无法调度原因。注意调度器只看 request，不看 limit。
- **PodFitsHost**：检查 `nodeName` 或 `nodeSelector` 是否严格匹配目标节点。若 Pod 指定了 `nodeName`，调度器会直接跳过其他节点。
- **PodToleratesNodeTaints**：检查 Pod 的 tolerations 是否能容忍节点的 taints。若节点被打了 `NoSchedule` 污点而 Pod 没有对应容忍，将被过滤掉。`PreferNoSchedule` 为软约束，不影响过滤阶段。

## 关键 Priorities

- **LeastRequestedPriority**：优先选择资源利用率最低的节点，实现负载均衡。适合通用工作负载，避免单节点过载。
- **BalancedResourceAllocation**：优先选择 CPU 与内存使用率接近的节点，避免单一资源耗尽导致节点不可用。此策略与 LeastRequested 结合使用效果更佳。

## 调度框架（Scheduling Framework）

Kubernetes 1.15+ 引入 Scheduling Framework，将调度流程拆分为多个扩展点（Extension Points），使自定义调度逻辑无需重新编译 kube-scheduler：

- **QueueSort**：自定义 Pod 排序策略。
- **PreFilter / Filter**：在过滤阶段插入自定义逻辑。
- **PostFilter**：过滤失败后执行（如抢占逻辑）。
- **Score**：自定义打分算法。
- **Reserve / Permit / PreBind / Bind / PostBind**：在绑定前后执行资源预留、审批等操作。

通过开发**调度插件（Scheduler Plugins）**并配置 `KubeSchedulerConfiguration`，平台团队可以扩展 kube-scheduler 的能力，例如实现 GPU 共享调度、拓扑感知调度或容量调度（Coscheduling）。

## Node Affinity / Pod Affinity / Taints & Tolerations

- **Node Affinity**：Pod 对节点的偏好或硬性要求，基于节点标签匹配，比 `nodeSelector` 更灵活（支持软偏好与操作符，如 `In`、`NotIn`、`Exists`、`Gt`、`Lt`）。
- **Pod Affinity/Anti-Affinity**：要求 Pod 与某些 Pod 同处（或不同处）一个拓扑域（如节点、机架、可用区），用于高可用或数据局部性。例如同一 Deployment 的副本分散在不同可用区（Anti-Affinity）。
- **Taints & Tolerations**：节点主动排斥 Pod 的机制。Taint 打在节点上，Toleration 声明在 Pod 中。二者组合实现专用节点、驱逐准备（`NoExecute`）、GPU 专用节点等场景。

示例：为节点打上专用污点

```yaml
apiVersion: v1
kind: Node
metadata:
  name: gpu-node-1
spec:
  taints:
  - key: nvidia.com/gpu
    value: "true"
    effect: NoSchedule
```

## 远程顾问诊断要点

Pod 长期处于 Pending 状态是远程顾问模式下的经典问题。排查应围绕"资源够不够、约束匹不匹配、有没有污点"展开：

1. **资源不足**：指导用户执行 `kubectl describe pod <pod>` 查看 Events。若出现 `Insufficient cpu` 或 `Insufficient memory`，说明集群整体或目标节点资源 request 已耗尽。此时需要扩容节点（Cluster Autoscaler）或降低 Pod 的 request。注意调度器只看 request，limit 再高也不影响调度。
2. **Taint 不匹配**：若 Events 中出现 `node(s) had taint {key: value}`，说明节点存在该 Pod 无法容忍的污点。请用户核对 Pod 的 `tolerations` 与节点的 `taints` 是否对应，特别注意 `NoSchedule` 与 `NoExecute` 的区别。`NoExecute` 还会驱逐已运行 Pod。
3. **亲和性冲突**：若使用了 Pod Anti-Affinity，而副本数大于可用节点数（或拓扑域数量），则必然有部分 Pod 无法调度。指导用户检查 `podAffinity` / `podAntiAffinity` 的 `topologyKey` 与集群拓扑分布是否匹配。Pod Affinity 要求拓扑域内已存在匹配 Pod，新集群可能因此无法调度。
4. **调度框架插件拦截**：某些自定义调度插件（如 GPU 调度、容量调度）可能在 Filter 或 Permit 阶段拒绝 Pod。若标准排查无异常，需确认集群是否启用了第三方调度插件，并查看调度器日志中的插件名称与拒绝原因。
5. **持久化卷绑定延迟**：若 Pod 依赖 PVC 且使用 WaitForFirstConsumer 模式，调度器会等待卷创建完成后再绑定节点。存储后端（如 NAS、云盘）的延迟或配额耗尽会导致 Pod 长期 Pending，此时 Events 中通常会有卷相关的提示。

更多排查细节可参考 [[故障诊断/topic-structural-trouble-shooting/01-control-plane/03-scheduler-troubleshooting.md|scheduler-troubleshooting]] 与 [[node-notready]]。

## 相关概念

- [[scheduling-algorithm]] — 调度算法详解
- [[node-lifecycle-management]] — 节点生命周期管理
- [[resource-management]] — Pod 资源管理机制

## Related

- [[visibility-public|#visibility/public Hub]] — tag hub


<!-- risk-assessed -->
