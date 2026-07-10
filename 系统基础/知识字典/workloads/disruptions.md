---
title: Disruptions
description: '## 概述'
summary: '## 概述'
category: dictionary
tags:
- k8s
- glossary
- terminology
- etcd
- kubelet
- scheduler
- redis
- pdb
- statefulset
- daemonset
tier: supporting
created: '2026-05-23'
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Disruptions 是什么
- 如何 Disruptions
trigger_keywords:
- Disruptions
- dictionary
prerequisites:
- kubectl-basics
- pod-lifecycle
- cloud-provider-basics
- etcd-basics
- redis-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Disruptions

## 概述
本页介绍影响 Pod 可用性的中断类型，以及如何通过 Pod Disruption Budget（PDB）等机制来管理自愿中断，帮助应用所有者和集群管理员维护高可用性。

## 核心概念/原理
- **非自愿中断（Involuntary Disruptions）**：无法避免的中断，例如硬件问题、节点内核崩溃、节点网络分区、资源不足导致的驱逐等。
- **自愿中断（Voluntary Disruptions）**：由人或控制器主动发起的中断，例如：
  - 删除 Deployment 或直接删除 Pod
  - 更新 Deployment 的 Pod 模板导致滚动重启
  - `kubectl drain` 节点进行维护或缩容
  - 调度器抢占（preemption）低优先级 Pod
- **Pod Disruption Budget（PDB）**：限制因自愿中断而同时不可用的 Pod 数量，确保应用始终维持最低可用副本数。

## 关键机制或特性
- **PDB 工作原理**：通过标签选择器指定受保护的 Pod 组，并设置 `minAvailable` 或 `maxUnavailable`。使用 Eviction API（如 `kubectl drain`）时，调度器会尊重 PDB 约束。
- **PDB 不限制的情况**：直接删除 Deployment 或 Pod 会绕过 PDB；滚动更新不受 PDB 限制（由工作负载控制器自行管理）。
- **DisruptionTarget 条件（Stable）**：Pod 即将因中断被删除时，会添加 `DisruptionTarget` 条件，并附带具体原因：
  - `PreemptionByScheduler`
  - `DeletionByTaintManager`
  - `EvictionByEvictionAPI`
  - `DeletionByPodGC`
  - `TerminationByKubelet`
- **Unhealthy Pod Eviction Policy**：建议设置为 `AlwaysAllow`，以便在节点维护期间允许驱逐不健康的 Pod。

## 使用场景
- 运行基于仲裁的应用（如 [[系统基础/知识字典/fundamentals/etcd.md|etcd]]、ZooKeeper），需要保证最低副本数。
- 集群管理员进行节点维护、升级或缩容时，确保业务不中断。
- 多租户环境中，应用团队通过 PDB 声明可用性需求。

## 最佳实践/注意事项
- 为高可用应用配置 PDB，但不要依赖 PDB 防止所有中断（特别是直接删除操作）。
- 复制应用并跨机架/可用区分布，以进一步降低中断影响。
- 若集群未启用任何自动自愿中断源，可暂时跳过 PDB 配置。
- 集群管理员应使用遵守 Eviction API 的工具执行维护操作。
- 为 PDB 设置 `AlwaysAllow` 不健康 Pod 驱逐策略，避免节点 drain 被卡住。

## 实战 YAML 示例

### 使用 minAvailable 的 PDB

```yaml
apiVersion: policy/v1
kind: PodDisruptionBudget
metadata:
  name: web-api-pdb
  namespace: prod
spec:
  minAvailable: 2                            # 任何时候至少保持 2 个可用副本
  selector:
    matchLabels:
      app: web-api
  unhealthyPodEvictionPolicy: AlwaysAllow    # 允许驱逐不健康 Pod，避免 drain 卡住
```

### 使用 maxUnavailable 的 PDB

```yaml
apiVersion: policy/v1
kind: PodDisruptionBudget
metadata:
  name: cache-pdb
  namespace: prod
spec:
  maxUnavailable: 1                          # 最多允许 1 个副本不可用
  selector:
    matchLabels:
      app: redis-cache
  unhealthyPodEvictionPolicy: AlwaysAllow
```

### 使用百分比的 PDB

```yaml
apiVersion: policy/v1
kind: PodDisruptionBudget
metadata:
  name: worker-pdb
  namespace: prod
spec:
  maxUnavailable: "25%"                      # 最多允许 25% 副本不可用
  selector:
    matchLabels:
      app: worker

```

### minAvailable vs maxUnavailable 对比

| 参数 | 3 副本场景 | 5 副本场景 | 适用场景 |
|------|-----------|-----------|---------|
| `minAvailable: 2` | 允许 1 个不可用 | 允许 3 个不可用 | 有固定仲裁要求的应用 |
| `maxUnavailable: 1` | 允许 1 个不可用 | 允许 1 个不可用 | 保守策略，每次最多中断 1 个 |
| `maxUnavailable: "25%"` | 允许 0 个不可用（向上取整） | 允许 1 个不可用 | 大规模 Deployment |

## 故障排查

### kubectl drain 被 PDB 阻塞
- **症状**: `kubectl drain` 长时间卡住，提示 `Cannot evict pod as it would violate the pod's disruption budget`。
- **常见原因**: PDB 设置过于严格（如 `minAvailable` 等于副本数）；有 Pod 不健康导致可用副本不足。
- **诊断命令**:
  ```bash
  # 查看 PDB 状态
  kubectl get pdb -n prod
  # 查看 PDB 详情（当前可用/期望/中断允许数）
  kubectl describe pdb web-api-pdb -n prod
  # 查看受 PDB 保护的 Pod 状态
  kubectl get pods -n prod -l app=web-api -o wide
  # 检查是否有不健康的 Pod
  kubectl get pods -n prod -l app=web-api -o jsonpath='{range .items[*]}{.metadata.name}: Ready={.status.conditions[?(@.type=="Ready")].status}{"\n"}{end}'

  ```
- **解决方案**:
  - 确认不健康 Pod 并修复，或设置 `unhealthyPodEvictionPolicy: AlwaysAllow`
  - 降低 `minAvailable` 或增大 `maxUnavailable`
  - 紧急情况可临时删除 PDB：`kubectl delete pdb web-api-pdb -n prod`

### 滚动更新时 PDB 导致更新缓慢
- **症状**: Deployment 滚动更新每次只能终止一个 Pod，非常缓慢。
- **原因**: 这通常是正常行为。滚动更新不受 PDB 限制（由 Deployment 控制器管理），但如果 `maxUnavailable` 设置为 0 且 `maxSurge` 为 1，更新确实会很慢。
- **解决方案**: 调整 Deployment 的 `strategy.rollingUpdate.maxSurge` 和 `maxUnavailable`，而非 PDB。

### PDB 状态显示 disruptionsAllowed: 0
- **症状**: PDB 的 `disruptionsAllowed` 始终为 0，阻止所有驱逐。
- **诊断命令**:
  ```bash
  kubectl get pdb web-api-pdb -n prod -o yaml

  ```
- **常见原因**: 当前可用 Pod 数恰好等于 `minAvailable`，或有 Pod 不健康导致可用数不足。

## 生产检查清单

- [ ] 所有高可用应用（>= 2 副本）都配置了 PDB
- [ ] PDB 的 `minAvailable` 满足应用仲裁/可用性要求（如 etcd 至少 N/2+1）
- [ ] `unhealthyPodEvictionPolicy: AlwaysAllow` 已设置，防止 drain 卡死
- [ ] PDB 选择器与工作负载标签正确匹配
- [ ] 集群升级前验证所有 PDB 的 `disruptionsAllowed` > 0
- [ ] 应用副本跨可用区分布，降低单点问题影响
- [ ] 运维团队使用 `kubectl drain`（Eviction API）而非 `kubectl delete pod`

## 命令快速参考

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `kubectl drain`：驱逐节点所有 Pod，业务流量受影响

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
# 查看所有 PDB 状态
kubectl get pdb -A

# 查看 PDB 详细信息（含 disruptionsAllowed）
kubectl describe pdb <pdb-name> -n <namespace>

# 安全驱逐节点上的 Pod（遵守 PDB）
kubectl drain <node-name> --ignore-daemonsets --delete-emptydir-data

# 强制驱逐（忽略 PDB，慎用！）
kubectl drain <node-name> --ignore-daemonsets --delete-emptydir-data --force --disable-eviction

# 检查特定 Pod 是否受 PDB 保护
kubectl get pdb -n <namespace> -o wide
```
## 交叉引用

- [[系统基础/知识字典/workloads/deployments.md|Deployments]] 中断管理](./deployments.md)
- [[entities/statefulset.md|StatefulSet]] 有序管理](./statefulsets.md)
- [工作负载概览与架构](../../工作负载/01-workload-overview-architecture.md)
- [节点 NotReady 诊断](../../故障诊断/06-node-notready-diagnosis.md)
- [Pod Pending 诊断](../../故障诊断/05-pod-pending-diagnosis.md)

## 参考链接
- https://[[entities/kubernetes.md|[[Kubernetes|kubernetes]]]].io/docs/concepts/workloads/pods/disruptions/

## Related

- [[系统基础/知识字典/workloads/advanced-pod-configuration.md|Advanced Pod Configuration]]
- [[系统基础/知识字典/workloads/automatic-cleanup-for-finished-jobs.md|Automatic Cleanup for Finished Jobs]]
- [[系统基础/知识字典/workloads/autoscaling-workloads.md|Autoscaling Workloads]]

```

<!-- risk-assessed -->
