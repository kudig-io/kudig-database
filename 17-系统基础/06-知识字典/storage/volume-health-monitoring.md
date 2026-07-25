---
title: Volume Health Monitoring（卷健康监控）
description: '# Volume Health Monitoring（卷健康监控）'
summary: '# Volume Health Monitoring（卷健康监控）'
category: dictionary
tags:
- k8s
- glossary
- terminology
- kubelet
- prometheus
- grafana
- operator
- rag
tier: supporting
created: 2026-05
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Volume Health Monitoring（卷健康监控） 是什么
- 如何 Volume Health Monitoring（卷健康监控）
trigger_keywords:
- Volume
- Health
- Monitoring
- 卷健康监控
- dictionary
prerequisites:
- kubectl-basics
- cloud-provider-basics
- prometheus-basics
- monitoring-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Volume Health Monitoring（卷健康监控）

## 概述

卷健康监控是 [[Kubernetes|Kubernetes]] CSI 实现的一部分，允许 CSI 驱动检测底层存储系统的异常卷状态，并将这些异常作为事件报告到相关的 PersistentVolumeClaim（PVC）或 Pod 上，帮助用户和运维人员及时发现存储问题。

## 核心概念/原理

- **CSI 驱动集成**：卷健康监控依赖于 CSI 驱动在控制器端和/或节点端支持的健康监控功能。
- **异常检测**：驱动通过查询底层存储系统，发现卷的异常状态（如磁盘问题、连接断开、存储阵列告警等）。
- **事件报告**：检测到的异常以 Kubernetes Event 的形式报告到受影响的资源上。

## 关键机制或特性

### 两个核心组件

1. **External Health Monitor Controller**
   - 运行在控制平面，监控 CSI 卷的异常状态。
   - 当检测到异常时，在相关的 PVC 上报告 Event。
   - 支持节点问题监控（`enable-node-watcher=true`）：检测到节点问题时，会在使用该节点上 PVC 的 PVC 对象上报告 Event，提示 Pod 位于问题节点上。

2. **[[kubelet|Kubelet]]（节点端）**
   - 当 CSI 驱动支持节点端的卷健康监控时，kubelet 会在检测到异常后在每个使用该 PVC 的 Pod 上报告 Event。
   - 同时，卷健康信息会暴露为 Kubelet VolumeStats 指标：`kubelet_volume_stats_health_status_abnormal`。
     - 标签：`namespace`、`persistentvolumeclaim`
     - 值：`1` 表示不健康，`0` 表示健康。

### 监控粒度

- **控制器端**：按 PVC 粒度报告异常事件。
- **节点端**：按 Pod 粒度报告异常事件，并提供 [[Prometheus|Prometheus]] 可采集的指标数据。

## 使用场景

- **存储问题告警**：当底层存储阵列或云盘出现异常时，及时在 PVC/Pod 上产生事件，触发告警系统通知运维人员。
- **节点问题感知**：存储节点宕机或网络隔离时，通过节点问题监控快速识别受影响的应用和数据卷。
- **可观测性集成**：将 `kubelet_volume_stats_health_status_abnormal` 指标接入 Prometheus/Grafana，实现存储健康的可视化监控。

## 最佳实践/注意事项

- 卷健康监控功能在 Kubernetes v1.21 中为 Alpha 状态，具体可用性和行为取决于 CSI 驱动的实现，使用前请查阅对应驱动的文档。
- 需要部署 External Health Monitor Controller 组件才能启用控制器端的健康监控。
- 节点端监控需要 CSI 驱动在节点插件中实现相应接口，并且 kubelet 支持暴露相关指标。
- 建议将卷健康事件和指标与现有的监控告警体系集成，形成完整的存储可观测性方案。

## 生产 YAML 示例

### 配置 Prometheus 采集卷健康指标

```yaml
# ServiceMonitor（Prometheus Operator）
apiVersion: monitoring.coreos.com/v1
kind: ServiceMonitor
metadata:
  name: kubelet-volume-health
  namespace: monitoring
spec:
  endpoints:
    - port: https-metrics
      scheme: https
      tlsConfig:
        insecureSkipVerify: true
      metricRelabelings:
        - sourceLabels: [__name__]
          regex: "kubelet_volume_stats_health_status_abnormal"
          action: keep
  namespaceSelector:
    matchNames: ["kube-system"]
  selector:
    matchLabels:
      k8s-app: kubelet
```

### Prometheus 告警规则

```yaml
groups:
  - name: volume-health
    rules:
      - alert: VolumeHealthAbnormal
        expr: kubelet_volume_stats_health_status_abnormal == 1
        for: 5m
        labels:
          severity: critical
        annotations:
          summary: "卷健康异常: {{ $labels.namespace }}/{{ $labels.persistentvolumeclaim }}"
```

## 故障排查

| 症状 | 可能原因 | 排查步骤 |
|------|----------|----------|
| PVC 上出现 VolumeConditionAbnormal 事件 | 底层存储问题 | 检查存储系统告警；检查 CSI 驱动日志 |
| 指标 `kubelet_volume_stats_health_status_abnormal` = 1 | 卷不健康 | `kubectl describe pvc` 查看事件；联系存储管理员 |
| 无健康监控事件产生 | CSI 驱动未实现健康监控 | 确认 CSI 驱动版本支持 Volume Health Monitoring |

## 生产检查清单

- [ ] 部署 External Health Monitor Controller
- [ ] CSI 驱动支持节点端和/或控制器端健康监控
- [ ] 将 `kubelet_volume_stats_health_status_abnormal` 接入 Prometheus
- [ ] 配置告警规则及时响应卷健康异常

## 命令快速参考

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看 PVC 上的健康事件
kubectl get events -n <ns> --field-selector involvedObject.name=<pvc-name>

# 查看 kubelet 卷健康指标
curl -sk https://localhost:10250/metrics | grep kubelet_volume_stats_health_status_abnormal
```
## 交叉引用

- [持久卷](./persistent-volumes.md) — PV/PVC 生命周期
- [存储类](./storage-classes.md) — CSI 驱动配置

## 参考链接

- https://kubernetes.io/docs/concepts/storage/volume-health-monitoring/

## Related

- [[17-系统基础/06-知识字典/storage/persistent-volume.md|Persistent Volume]]
- [[17-系统基础/06-知识字典/storage/persistent-volume-claim.md|Persistent Volume Claim]]
- [[17-系统基础/06-知识字典/storage/storage-class.md|Storage Class]]
- [[17-系统基础/06-知识字典/storage/volume.md|Volume]]
- [[17-系统基础/06-知识字典/storage/emptydir.md|Emptydir]]


<!-- risk-assessed -->
