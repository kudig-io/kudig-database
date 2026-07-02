---
title: Kubernetes × Prometheus
summary: Kubernetes 与 Prometheus 的交叉：Prometheus 作为集群可观测性基础设施，如何采集控制面、工作负载和节点的指标并驱动告警与自动化决策。
category: synthesis
tags:
- k8s
- prometheus
- observability
- monitoring
- metrics
- alerting
tier: supporting
sources:
- domain-17-system-foundation/topic-dictionary/fundamentals/kubernetes.md
- domain-17-system-foundation/topic-dictionary/observability/prometheus.md
- concepts/Kubernetes Fault Distribution and MTTR.md
- concepts/Structural Troubleshooting Framework.md
- concepts/bp-common-best-practices.md
- concepts/bp-observability.md
created: '2026-07-02'
updated: '2026-07-02'
last_updated: 2026-07-02
provenance:
  extracted: 0.3
  inferred: 0.6
  ambiguous: 0.1
base_confidence: 0.76
lifecycle: draft
lifecycle_changed: '2026-07-02'
---


# Kubernetes × Prometheus

## The Connection

Kubernetes 提供了容器化应用的编排和运行时管理，但其动态性（Pod 频繁创建销毁、节点弹性伸缩、控制器自动修复）使得传统推模式监控难以跟上。Prometheus 采用 Pull 模型和基于服务发现的动态采集目标管理，天然契合 Kubernetes 的服务发现机制。二者结合，Prometheus 成为 Kubernetes 集群可观测性的事实标准。^[inferred]

## Where They Co-occur

- **kube-prometheus-stack** 是生产环境部署 Prometheus 的标准方式，通过 Helm Chart 一键部署 Prometheus、Grafana、Alertmanager 及 kube-state-metrics
- **Prometheus Operator** 通过 ServiceMonitor/PodMonitor CRD 声明式管理采集目标，与 Kubernetes RBAC 和 Namespace 隔离模型对齐
- 控制面组件（kube-apiserver、etcd、kube-scheduler、kube-controller-manager）均暴露 `/metrics` 端点供 Prometheus 抓取
- kubelet 内置 cAdvisor 采集容器级 CPU、内存、网络、文件系统指标
- node-exporter 以 DaemonSet 方式运行，采集节点级硬件与 OS 指标
- 告警规则（AlertingRule CRD）驱动 Alertmanager 进行分级通知，与 SLO 错误预算策略联动

## Cross-cutting Insight

Kubernetes 解决"如何运行应用"，Prometheus 解决"运行得怎么样"。没有 Prometheus，Kubernetes 的自动修复（如 liveness probe 重启、HPA 弹性伸缩）只能基于局部信号；有了 Prometheus 的全局指标视图，运维团队可以从故障统计（如 etcd 延迟占故障的 ~30%、网络问题占 ~20%）出发，制定数据驱动的可靠性策略。^[inferred]

## Tensions and Trade-offs

| 维度 | Kubernetes 原生 | Prometheus 增强 | 结合注意事项 |
|---|---|---|---|
| 健康检查 | liveness/readiness probe | 指标 + 告警规则 | probe 只判断存活，指标判断质量 |
| 弹性伸缩 | HPA 基于 CPU/内存 | HPA 基于自定义指标（Prometheus Adapter） | 自定义指标延迟可能影响扩缩决策 |
| 故障发现 | Event + 控制器状态 | 告警规则 + Grafana 仪表盘 | Event 是点状的，指标是连续的 |
| 资源开销 | 无额外开销 | 存储时间序列数据消耗内存和磁盘 | 大规模集群需 Federation 或 Thanos |
| 多租户 | Namespace 隔离 | Prometheus 实例按租户拆分或 Thanos | 指标隔离与聚合的平衡 |

## Open Questions

- 在千节点级集群中，单一 Prometheus 实例的内存瓶颈如何通过 Thanos/Cortex 解决？
- Prometheus 的 Pull 模型在 Service Mesh 环境下是否需要适配 mTLS？
- 如何基于 Prometheus 指标构建自动化的故障自愈（如 etcd compaction 自动触发）？

## Related

- [[domain-17-system-foundation/topic-dictionary/fundamentals/kubernetes.md|Kubernetes]]
- [[domain-17-system-foundation/topic-dictionary/observability/prometheus.md|Prometheus]]
- [[concepts/Kubernetes Fault Distribution and MTTR.md|Kubernetes Fault Distribution and MTTR]]
- [[concepts/Structural Troubleshooting Framework.md|Structural Troubleshooting Framework]]
- [[concepts/bp-observability.md|最佳实践：Observability]]
- [[concepts/bp-common-best-practices.md|Kubernetes 通用最佳实践参考]]
- [[synthesis/slo-observability.md|SLO × 可观测性]]


<!-- risk-assessed -->
