---
title: Kubernetes 监控最佳实践
description: '# Kubernetes 监控最佳实践'
summary: '本指南提供生产环境 Kubernetes 监控配置的最佳实践，涵盖从 Prometheus 部署到告警配置的全方位内容 ^[inferred]。'
category: skills
tags:
- k8s
- monitoring
- prometheus
- grafana
- alerting
- etcd
- kubelet
- helm
- ingress
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
- Kubernetes 监控最佳实践 是什么
- 如何 Kubernetes 监控最佳实践
trigger_keywords:
- Kubernetes
- 监控最佳实践
prerequisites:
- kubectl-basics
- helm-basics
- prometheus-basics
- monitoring-basics
- etcd-basics
---



# Kubernetes 监控最佳实践

## 概述

本指南提供生产环境 Kubernetes 监控配置的最佳实践，涵盖从 Prometheus 部署到告警配置的全方位内容 ^[inferred]。

## observability/01-observability-architecture-overview.md|01-observability-architecture-overview]]设计

### 分层监控

- **基础设施层**：节点监控（CPU/内存/磁盘）、网络监控（流量/延迟）、存储监控（IOPS/容量）
- **平台层**：Kubernetes 组件监控（API Server/etcd/kubelet）、容器监控（Pod/容器资源）、服务监控（[[Service|Service]]/Ingress）
- **应用层**：应用指标（QPS/延迟/错误率）、业务指标、自定义指标 ^[inferred]

### 采集层组件

- **Node Exporter**：节点级指标采集
- **cAdvisor**：容器资源指标
- **应用 Exporter**：应用自定义指标
- **Prometheus**：指标采集和存储

## 关键配置

### Prometheus 配置

通过 kube-prometheus-stack [[Helm|Helm]] Chart 部署，建议配置：
- `retention: 30d` — 数据保留 30 天 ^[inferred]
- 存储使用 fast-ssd，至少 100Gi ^[inferred]
- 副本数 >= 2 保证高可用 ^[inferred]

### ServiceMonitor 配置

- `interval: 30s` — 采集间隔 30 秒 ^[inferred]
- 确保 `labels` 与 Prometheus 的 `serviceMonitorSelector` 匹配 ^[inferred]
- `selector.matchLabels` 与 Service 标签匹配 ^[inferred]

### 告警规则设计

- **高错误率告警**：`rate(http_requests_total{status=~"5.."}[5m]) / rate(http_requests_total[5m]) > 0.05`，持续 5 分钟触发 ^[inferred]
- **高延迟告警**：`histogram_quantile(0.95, rate(http_request_duration_seconds_bucket[5m])) > 1`，持续 5 分钟触发 ^[inferred]
- 使用 `for` 字段避免瞬时告警风暴 ^[inferred]

## 实施步骤

1. **安装 Prometheus Operator**：使用 kube-prometheus-stack Helm Chart
2. **配置节点监控**：安装 Node Exporter
3. **配置应用监控**：创建 ServiceMonitor
4. **配置告警**：创建 PrometheusRule 告警规则

## 常见陷阱

### Prometheus 存储空间不足

存储空间不足会导致数据丢失和告警不及时。应根据数据量调整存储大小，配置合理的 retention 策略 ^[inferred]。

### 告警规则配置不当

告警阈值设置不合理会导致告警风暴。应设置合理的 `for` 持续时间，进行分级告警 ^[inferred]。

### ServiceMonitor 标签不匹配

标签不匹配会导致指标采集失败。应确保 ServiceMonitor 的 labels 与 Prometheus 的 serviceMonitorSelector 匹配，selector 与 Service 标签匹配 ^[inferred]。

## 验证方法

- 检查 Prometheus 状态和 ServiceMonitor 列表 ^[inferred]
- 检查告警规则和 Alertmanager 状态
- 测试 Prometheus 查询：`curl http://localhost:9090/api/v1/query?query=up`

## 相关资源

- [[concepts/k8s-production-best-practices.md|[[Kubernetes 生产环境最佳实践|Kubernetes 生产环境最佳实践]]]]
- [[concepts/observability-pillars.md|[[Observability Pillars|Observability Pillars]]]]
- [[entities/prometheus-grafana.md|Prometheus + Grafana]]
- [[skills/monitor-kubernetes-metrics.md|Monitor Kubernetes Metrics]]
- [[skills/k8s-logging-management-guide.md|Kubernetes 日志管理最佳实践]]
- [[skills/k8s-distributed-tracing-guide.md|Kubernetes 分布式追踪最佳实践]]

## Related

- [[prometheus]] — Prometheus
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[concepts/observability-pillars.md|observability-pillars]] — Observability Pillars
- [[concepts/k8s-production-best-practices.md|k8s-production-best-practices]] — Kubernetes 生产环境最佳实践
- [[skills/monitor-kubernetes-metrics.md|monitor-kubernetes-metrics]] — Monitor Kubernetes Metrics
