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

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




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

- [[概念/k8s-production-best-practices.md|[[Kubernetes 生产环境最佳实践|Kubernetes 生产环境最佳实践]]]]
- [[概念/observability-pillars.md|[[Observability Pillars|Observability Pillars]]]]
- [[实体/prometheus-grafana.md|Prometheus + Grafana]]
- [[技能/monitor-kubernetes-metrics.md|Monitor Kubernetes Metrics]]
- [[技能/k8s-logging-management-guide.md|Kubernetes 日志管理最佳实践]]
- [[技能/k8s-distributed-tracing-guide.md|Kubernetes 分布式追踪最佳实践]]

## 生产案例

### 案例 1: Prometheus 采集延迟导致告警滞后

| 时间 | 事件 |
|------|------|
| 20:00 | 业务故障 10min 后才收到告警 |
| 20:05 | Prometheus 采集间隔 30s + 告警 for 5min |
| 20:08 | 实际故障发生到告警触发延迟 5.5min |
| 20:10 | 🟢 调整关键指标采集间隔 15s + 告警 for 2min |

**根因**: 默认采集间隔和告警持续时间设置过长。

### 案例 2: Grafana 仪表盘加载超时

**现象**: Grafana 页面加载 30s+，部分面板无数据。

**诊断**: 查询时间范围过大 + 高基数指标

**修复**: 🟢 限制查询时间范围 + 优化 PromQL(使用 recording rules)

## 升级决策点

| 级别 | 条件 | 动作 |
|------|------|------|
| P0 | 监控完全不可用 | 检查 Prometheus + 存储 |
| P1 | 告警延迟 | 调整采集和告警配置 |
| P2 | 性能优化 | 使用 recording rules |

## 面试要点

1. **Q: Kubernetes 监控的四个黄金信号？**
   A: 延迟(Latency)、流量(Traffic)、错误率(Errors)、饱和度(Saturation)。对应指标: P99 延迟、QPS、错误率、资源使用率。

2. **Q: Prometheus 在 K8s 中的服务发现机制？**
   A: 通过 kubernetes_sd_configs 自动发现: ① Pod(annotations) ② Service(endpoints) ③ Node ④ Ingress。配合 ServiceMonitor CRD(Operator) 简化配置。

3. **Q: 如何设计有效的告警规则？**
   A: ① 基于 SLO 而非原始指标 ② 设置合理的 for 持续时间 ③ 分级告警(P0-P3) ④ 避免告警风暴(使用 Alertmanager 分组/抑制) ⑤ 定期审查告警有效性。

## Related

- [[prometheus]] — Prometheus
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[概念/observability-pillars.md|observability-pillars]] — Observability Pillars
- [[概念/k8s-production-best-practices.md|k8s-production-best-practices]] — Kubernetes 生产环境最佳实践
- [[技能/monitor-kubernetes-metrics.md|monitor-kubernetes-metrics]] — Monitor Kubernetes Metrics


<!-- risk-assessed -->
