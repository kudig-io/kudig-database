---
title: CRD × 可观测性
description: '[[实体/crd-custom-resources.md|crd custom resources]] 扩展 K8s API，[[实体/prometheus-grafana.md|prometheus
  grafana]] 是监控栈。两者的交汇点是 **Prometheus Operator**：它通过 CRD（ServiceMonitor、PodMonitor、PrometheusRule）将监控配置从手动脚本维护转变为声明式
  GitOps 工作流。但 wiki '
summary: '[[实体/crd-custom-resources.md|crd custom resources]] 扩展 K8s API，[[实体/prometheus-grafana.md|prometheus
  grafana]] 是监控栈。两者的交汇点是 **Prometheus Operator**：它通过 CRD（ServiceMonitor、PodMonitor、Pro...'
category: synthesis
tags:
- k8s
- crd
- observability
- prometheus
- declarative
- grafana
- cilium
- hpa
- networkpolicy
- operator
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- CRD × 可观测性 是什么
- 如何 CRD × 可观测性
trigger_keywords:
- CRD
- 可观测性
prerequisites:
- kubectl-basics
- prometheus-basics
- monitoring-basics
- ebpf-basics
- cilium-basics
- observability-basics
relationships:
- target: '[[实体/cilium.md]]'
  type: uses
- target: '[[概念/Deployment × Secret 管理.md]]'
  type: uses
- target: '[[实体/kubernetes.md]]'
  type: uses
- target: '[[实体/prometheus.md]]'
  type: uses
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# CRD × 可观测性


## 连接点

[[实体/crd-custom-resources.md|crd custom resources]] 扩展 K8s API，[[实体/prometheus.md|prometheus]]-grafana]] 是监控栈。两者的交汇点是 **Prometheus Operator**：它通过 CRD（ServiceMonitor、PodMonitor、PrometheusRule）将监控配置从手动脚本维护转变为声明式 GitOps 工作流。但 wiki 没有指出一个更深层的范式转变：**CRD 使可观测性配置成为了 [[实体/kubernetes.md|Kubernetes]] 的一等公民**。

## 共现场景

- **ServiceMonitor CRD**：声明式定义 scrape 目标，Prometheus Operator 自动将其转化为 Prometheus 的 scrape_config——消除了手动编辑配置文件的运维负担
- **PrometheusRule CRD**：告警规则以 YAML 形式存储在 Git 中，通过 GitOps 同步——规则的版本历史、审计追踪、回滚能力与业务代码完全一致
- **AlertmanagerConfig CRD**：路由配置、抑制规则、通知渠道全部声明式管理
- **自定义指标 CRD**：HPA 通过 custom.metrics.k8s.io 读取 CRD 定义的自定义指标，实现业务级自动扩缩容

## 交叉洞察

**核心洞察：CRD 使可观测性配置从"运维人员的副业"转变为"平台工程的核心能力"。**

在 CRD 之前，监控配置是运维团队的职责：手动编辑 prometheus.yml、维护 alert.rules、配置 grafana dashboards。这些配置与业务应用分离，导致：
- 应用上线时监控缺失（监控是后期添加的）
- 规则变更与代码发布不同步（告警规则落后于业务变更）
- 监控知识无法版本化（规则散落在各种文档和 wiki 中）

CRD 统一了应用定义和监控定义：

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: my-app
---
apiVersion: monitoring.coreos.com/v1
kind: ServiceMonitor
metadata:
  name: my-app-metrics
spec:
  selector:
    matchLabels:
      app: my-app
```

这两份 YAML 可以共存于同一个 Git 仓库、通过同一个 CI/CD 管道部署、受同一个 GitOps 控制器协调。**监控不再是运维的附属品，而是应用定义的一部分。**

## 张力与权衡

| 张力 | 详情 |
|------|------|
| **CRD 版本兼容性** | Prometheus Operator 升级时，ServiceMonitor CRD 的 schema 可能变更。旧版本规则在新 Operator 中可能无法解析，导致监控中断 |
| **配置漂移的双向性** | GitOps 保证 Git 是单一事实源，但 Prometheus 的 /-/reload 端点允许运行时热加载配置。如果运维人员手动调用了 reload，Git 中的配置与实际运行配置将不一致 |
| **CRD 爆炸** | 完整的可观测性 CRD 生态包括 ServiceMonitor、PodMonitor、Probe、PrometheusRule、AlertmanagerConfig、ThanosRuler、GrafanaDashboard 等 10+ 种 CRD。大型集群中 CRD 实例数可能超过普通工作负载 |

## 开放问题

- **可观测性 CRD 标准化**：Prometheus Operator 的 CRD 是事实标准，但其他监控工具（VictoriaMetrics、Grafana Agent）使用不兼容的 CRD。是否应该有一个跨厂商的可观测性 CRD 标准？
- **CRD 与 OpenTelemetry 的冲突**：OpenTelemetry 推崇供应商中立的配置方式（OTLP、OTel Collector），而 CRD 是 K8s 专属的。未来是可观测性配置 CRD 化，还是去 K8s 化？


## 相关

- [[实体/crd-custom-resources.md|crd-custom-resources]]
- [[实体/prometheus-grafana.md|prometheus-grafana]]
- [[operator-pattern]]

> *This page synthesizes patterns across multiple sources and domains.* ^[inferred]

## See Also

- [[概念/CI-CD 流水线 × Secret 管理.md|CI-CD 流水线 × Secret 管理]]
- [[概念/CNI 插件 × NetworkPolicy.md|CNI 插件 × NetworkPolicy]]
- [[实体/cilium.md|Cilium]] eBPF × 可观测性.md|Cilium eBPF × 可观测性]]
- Deployment × Secret 管理.md|Deployment × Secret 管理]]
## Related

- [[实体/deployment.md|Deployment]]


<!-- risk-assessed -->
