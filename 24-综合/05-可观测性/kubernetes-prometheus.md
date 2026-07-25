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
- 系统基础/topic-dictionary/fundamentals/kubernetes.md
- 系统基础/topic-dictionary/observability/prometheus.md
- concepts/Kubernetes Fault Distribution and MTTR.md
- concepts/Structural Troubleshooting Framework.md
- concepts/bp-common-best-practices.md
- concepts/bp-observability.md
created: '2026-07-02'
updated: '2026-07-02'
last_updated: 2026-07-11
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

Kubernetes 提供了容器化应用的编排和运行时管理，但其动态性（Pod 频繁创建销毁、节点弹性伸缩、控制器自动修复）使得传统推模式监控难以跟上——IP 不固定、服务实例增减不可预测、短生命周期 Job 来不及注册。Prometheus 采用 Pull 模型和基于服务发现的动态采集目标管理，天然契合 Kubernetes 的服务发现机制：Prometheus 通过 Kubernetes SD 配置自动发现带有 `prometheus.io/scrape` 注解的 Service 和 Pod，将其加入采集目标列表。当 Pod 被创建或销毁时，采集目标自动增减，无需手动注册或注销。二者结合，Prometheus 成为 Kubernetes 集群可观测性的事实标准。更关键的是，Prometheus 的多维标签模型（metric name + labels）与 Kubernetes 的资源模型（kind + namespace + name + labels）天然对齐，使得每一个指标数据点都能追溯到具体的 Pod/Service/Node，实现了"观测数据 → K8s 资源"的无缝映射。^[inferred]

## Where They Co-occur

- **kube-prometheus-stack** 是生产环境部署 Prometheus 的标准方式，通过 Helm Chart 一键部署 Prometheus、Grafana、Alertmanager 及 kube-state-metrics，提供开箱即用的集群监控。
- **Prometheus Operator** 通过 ServiceMonitor/PodMonitor CRD 声明式管理采集目标，与 Kubernetes RBAC 和 Namespace 隔离模型对齐——开发者只需创建 ServiceMonitor 即可将自己的服务纳入监控。
- 控制面组件（kube-apiserver、etcd、kube-scheduler、kube-controller-manager）均暴露 `/metrics` 端点供 Prometheus 抓取，提供 API 延迟、etcd WAL fsync、调度延迟等关键指标。
- kubelet 内置 cAdvisor 采集容器级 CPU、内存、网络、文件系统指标，通过 kubelet 的 `/metrics/cadvisor` 端点暴露。
- node-exporter 以 DaemonSet 方式运行，采集节点级硬件与 OS 指标（CPU、内存、磁盘 I/O、网络），是节点层监控的基础。
- 告警规则（AlertingRule CRD）驱动 Alertmanager 进行分级通知（critical → PagerDuty、warning → Slack），与 SLO 错误预算策略联动。
- **kube-state-metrics (KSM)**：暴露 Kubernetes 对象状态指标（Deployment replicas、Pod phase、PVC capacity），是"集群状态可观测"的核心数据源。
- **Recording Rules 预计算**：高频查询的 PromQL（如 SLO 计算公式）通过 Recording Rules 预计算为持久化的时间序列，降低查询延迟和 Prometheus CPU 负载。
- **Prometheus relabeling**：通过 `relabel_configs` 和 `metric_relabel_configs` 对采集目标和指标做标签过滤/重命名——如丢弃高基数 label（`request_id`、`user_id`）避免 TSDB 膨胀。
- **Alertmanager 分级路由**：Alertmanager 的 `route` 配置支持按 label（severity、team）做告警分级——critical → PagerDuty，warning → Slack，info → 静默归档。
- **Prometheus remote_write 多后端**：Prometheus 的 `remote_write` 可同时推送指标到多个后端（Thanos Receive、Mimir、Cortex、Datadog），实现"采集一次、多后端消费"的弹性架构。
- **Prometheus TSDB 压缩与 retention**：Prometheus 本地 TSDB 默认 15 天 retention（`--storage.tsdb.retention.time`），超期自动压缩删除——长期存储依赖 Thanos/Mimir + 对象存储。

## Cross-cutting Insight

Kubernetes 解决"如何运行应用"，Prometheus 解决"运行得怎么样"。没有 Prometheus，Kubernetes 的自动修复（如 liveness probe 重启、HPA 弹性伸缩）只能基于局部信号；有了 Prometheus 的全局指标视图，运维团队可以从故障统计（如 etcd 延迟占故障的 ~30%、网络问题占 ~20%）出发，制定数据驱动的可靠性策略。更深层地看，Prometheus 在 Kubernetes 环境中的价值不仅是"采集指标"，更是为 K8s 的自动化决策提供反馈回路：HPA 依赖 Prometheus Adapter 的自定义指标做扩缩决策，SLO 管理器依赖 Prometheus 的 burn rate 告警驱动发布冻结，甚至 ArgoCD 的渐进式发布也可以用 Prometheus 指标作为 promotion gate。当 Prometheus 从"被动记录"升级为"主动驱动自动化"的数据源时，它的可用性和延迟直接影响 K8s 控制面的决策质量——Prometheus 不再只是观测工具，而是基础设施的关键依赖。这也意味着 Prometheus 自身需要被监控（meta-monitoring），其不可用不应成为 SLO 违约的隐藏根因。^[inferred]

## Tensions and Trade-offs

| 维度 | Kubernetes 原生 | Prometheus 增强 | 结合注意事项 |
|---|---|---|---|
| 健康检查 | liveness/readiness probe | 指标 + 告警规则 | probe 只判断存活，指标判断质量 |
| 弹性伸缩 | HPA 基于 CPU/内存 | HPA 基于自定义指标（Prometheus Adapter） | 自定义指标延迟可能影响扩缩决策 |
| 故障发现 | Event + 控制器状态 | 告警规则 + Grafana 仪表盘 | Event 是点状的，指标是连续的 |
| 资源开销 | 无额外开销 | 存储时间序列数据消耗内存和磁盘 | 大规模集群需 Federation 或 Thanos |
| 多租户 | Namespace 隔离 | Prometheus 实例按租户拆分或 Thanos | 指标隔离与聚合的平衡 |
| 数据持久化 | K8s 不存历史指标 | Prometheus 默认本地 TSDB | 长期存储需 Thanos/Mimir + 对象存储 |

## Open Questions

- 在千节点级集群中，单一 Prometheus 实例的内存瓶颈如何通过 Thanos/Cortex/Mimir 解决？分片策略按 namespace 还是按 metric name？
- Prometheus 的 Pull 模型在 Service Mesh 环境下是否需要适配 mTLS？sidecar 是否会阻断 scrape？
- 如何基于 Prometheus 指标构建自动化的故障自愈（如 etcd compaction 自动触发、节点 NotReady 自动 drain）？告警 → 自动化 action 的边界在哪？
- 当 Prometheus 自身不可用时，K8s 自动化决策（HPA、SLO gate）如何优雅降级而非级联失败？

## Related

- [[17-系统基础/06-知识字典/fundamentals/kubernetes.md|Kubernetes]]
- [[17-系统基础/06-知识字典/observability/prometheus.md|Prometheus]]
- [[37-归档/troubleshooting-diagnostics/kubernetes-fault-distribution-and-mttr-en.md|Kubernetes Fault Distribution and MTTR]]
- [[22-概念/08-可靠性与运维/Structural Troubleshooting Framework.md|Structural Troubleshooting Framework]]
- [[22-概念/10-最佳实践/bp-observability.md|最佳实践：Observability]]
- [[22-概念/10-最佳实践/bp-common-best-practices.md|Kubernetes 通用最佳实践参考]]
- [[24-综合/05-可观测性/slo-observability.md|SLO × 可观测性]]


<!-- risk-assessed -->
