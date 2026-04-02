# DaemonSet

## 概述
DaemonSet 确保所有（或部分）节点上都运行一个 Pod 副本。当节点加入集群时，Pod 会被自动创建；当节点从集群移除时，Pod 会被垃圾回收。删除 DaemonSet 会清理其创建的所有 Pod。

## 核心概念/原理
- **节点级服务**：DaemonSet 用于提供节点本地设施，类似于传统 Unix 服务器上的系统守护进程。
- **Pod 模板**：与 Deployment 类似，`spec.template` 是必需的，且 `restartPolicy` 必须为 `Always`（或未指定，默认即 Always）。
- **选择器**：`spec.selector` 用于匹配 Pod 标签，创建后不可变。
- **节点筛选**：可通过 `nodeSelector` 或 `nodeAffinity` 限制 DaemonSet 仅在符合条件的节点上创建 Pod。

## 关键机制或特性
- **调度方式**：DaemonSet 控制器会为每个目标节点设置 `spec.affinity.nodeAffinity`，将 Pod 绑定到特定节点。默认调度器随后会处理实际的节点绑定，必要时可基于 Pod 优先级抢占现有 Pod。
- **自动容忍（Tolerations）**：DaemonSet 控制器会自动为 Pod 添加一组容忍，使其能在不健康的节点上运行：
  - `node.kubernetes.io/not-ready`（NoExecute）
  - `node.kubernetes.io/unreachable`（NoExecute）
  - `node.kubernetes.io/disk-pressure`（NoSchedule）
  - `node.kubernetes.io/memory-pressure`（NoSchedule）
  - `node.kubernetes.io/pid-pressure`（NoSchedule）
  - `node.kubernetes.io/unschedulable`（NoSchedule）
  - `node.kubernetes.io/network-unavailable`（NoSchedule，仅对 `hostNetwork: true` 的 Pod）
- **更新策略**：支持滚动更新（RollingUpdate），可配置 `maxUnavailable` 和 `maxSurge`。
- **高优先级**：建议为关键 DaemonSet 设置较高的 PriorityClass，以确保在资源竞争时能成功调度。

## 使用场景
- 集群网络插件（如 Calico、Flannel、Cilium）。
- 节点监控代理（如 Prometheus Node Exporter）。
- 日志收集代理（如 Fluentd、Fluent Bit）。
- 存储驱动或设备插件（如 CSI 节点插件）。

## 最佳实践/注意事项
- 如果 DaemonSet 提供集群网络等关键功能，请确保其具有足够的优先级和 tolerations，以避免与节点就绪状态形成死锁。
- 可通过 `hostPort`、DNS 或 Service 等方式与 DaemonSet Pod 通信。
- 修改 DaemonSet 的 Pod 后，控制器下次在节点上创建 Pod 时仍会使用原始模板；某些字段不支持原地更新。
- 删除 DaemonSet 时使用 `--cascade=orphan` 可保留节点上的 Pod；后续创建相同选择器的 DaemonSet 会收养这些 Pod。

## 参考链接
- https://kubernetes.io/docs/concepts/workloads/controllers/daemonset/
