# Volume Health Monitoring（卷健康监控）

## 概述

卷健康监控是 Kubernetes CSI 实现的一部分，允许 CSI 驱动检测底层存储系统的异常卷状态，并将这些异常作为事件报告到相关的 PersistentVolumeClaim（PVC）或 Pod 上，帮助用户和运维人员及时发现存储问题。

## 核心概念/原理

- **CSI 驱动集成**：卷健康监控依赖于 CSI 驱动在控制器端和/或节点端支持的健康监控功能。
- **异常检测**：驱动通过查询底层存储系统，发现卷的异常状态（如磁盘故障、连接断开、存储阵列告警等）。
- **事件报告**：检测到的异常以 Kubernetes Event 的形式报告到受影响的资源上。

## 关键机制或特性

### 两个核心组件

1. **External Health Monitor Controller**
   - 运行在控制平面，监控 CSI 卷的异常状态。
   - 当检测到异常时，在相关的 PVC 上报告 Event。
   - 支持节点故障监控（`enable-node-watcher=true`）：检测到节点故障时，会在使用该节点上 PVC 的 PVC 对象上报告 Event，提示 Pod 位于故障节点上。

2. **Kubelet（节点端）**
   - 当 CSI 驱动支持节点端的卷健康监控时，kubelet 会在检测到异常后在每个使用该 PVC 的 Pod 上报告 Event。
   - 同时，卷健康信息会暴露为 Kubelet VolumeStats 指标：`kubelet_volume_stats_health_status_abnormal`。
     - 标签：`namespace`、`persistentvolumeclaim`
     - 值：`1` 表示不健康，`0` 表示健康。

### 监控粒度

- **控制器端**：按 PVC 粒度报告异常事件。
- **节点端**：按 Pod 粒度报告异常事件，并提供 Prometheus 可采集的指标数据。

## 使用场景

- **存储故障告警**：当底层存储阵列或云盘出现异常时，及时在 PVC/Pod 上产生事件，触发告警系统通知运维人员。
- **节点故障感知**：存储节点宕机或网络隔离时，通过节点故障监控快速识别受影响的应用和数据卷。
- **可观测性集成**：将 `kubelet_volume_stats_health_status_abnormal` 指标接入 Prometheus/Grafana，实现存储健康的可视化监控。

## 最佳实践/注意事项

- 卷健康监控功能在 Kubernetes v1.21 中为 Alpha 状态，具体可用性和行为取决于 CSI 驱动的实现，使用前请查阅对应驱动的文档。
- 需要部署 External Health Monitor Controller 组件才能启用控制器端的健康监控。
- 节点端监控需要 CSI 驱动在节点插件中实现相应接口，并且 kubelet 支持暴露相关指标。
- 建议将卷健康事件和指标与现有的监控告警体系集成，形成完整的存储可观测性方案。

## 参考链接

- https://kubernetes.io/docs/concepts/storage/volume-health-monitoring/
