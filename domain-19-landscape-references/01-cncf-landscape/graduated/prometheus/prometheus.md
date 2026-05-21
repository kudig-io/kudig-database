---
title: Prometheus
description: 'description: ''## 项目概述'''
category: general
tags:
- cncf
- ecosystem
- prometheus
- grafana
- helm
- docker
- job
- gateway
- operator
- rag
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Prometheus 是什么
- 如何 Prometheus
- Kubernetes 19 landscape references 最佳实践
trigger_keywords:
- Prometheus
- landscape
- references
prerequisites:
- kubectl-basics
- cncf-ecosystem
- helm-basics
- prometheus-basics
- monitoring-basics
- ebpf-basics
- observability-basics
---

title: Prometheus
description: '## 项目概述'
category: cncf-landscape
tags:
- k8s
- cncf
- cloud-native
- ecosystem
- prometheus
- grafana
- helm
- docker
- job
- gateway
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 架构师
- 技术决策者
- SRE
estimated_read_time: 5min
intent_queries:
- Prometheus 是什么
- 如何 Prometheus
- Kubernetes 34 cncf landscape 最佳实践
trigger_keywords:
- Prometheus
- cncf
- landscape
authors:
- name: KUDIG Team
  role: contributor
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
---
# Prometheus

> **成熟度**: Graduated | **加入时间**: 2016-05 | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官方网站** | https://prometheus.io |
| **GitHub** | https://github.com/prometheus/prometheus |
| **文档** | https://prometheus.io/docs |
| **许可证** | Apache-2.0 |
| **主要语言** | Go |
| **CNCF 分类** | Observability |

---

## 项目概述

### 简介
Prometheus 是一个开源的系统监控和告警工具包，最初由 SoundCloud 开发，现已成为云原生监控的事实标准。

### 核心定位
Prometheus 专为可靠性和可扩展性设计，提供多维数据模型、灵活的查询语言 PromQL、高效的时序数据存储，是云原生可观测性的核心组件。

### 发展历程
- **2012**: SoundCloud 开始开发 Prometheus
- **2015**: 对外发布 v0.1
- **2016-05**: 成为 CNCF 第二个托管项目
- **2018-08**: 成为 CNCF 第二个毕业项目
- **2024**: Prometheus v2.50+ 持续演进

---

## 核心功能

### 主要特性
- **多维数据模型**: 基于指标名和键值对标签的时序数据
- **PromQL**: 强大灵活的查询语言
- **拉取模式**: 主动从目标拉取指标数据
- **服务发现**: 自动发现监控目标
- **告警管理**: 灵活的告警规则和通知
- **可视化**: 内置表达式浏览器，集成 Grafana

### 功能架构
```
┌─────────────────────────────────────────────────────────────┐
│                     Prometheus Server                       │
│  ┌──────────────┐ ┌──────────────┐ ┌──────────────────────┐│
│  │   Retrieval  │ │    TSDB      │ │    HTTP Server       ││
│  │   (Scraping) │ │   (Storage)  │ │    (PromQL API)      ││
│  └──────────────┘ └──────────────┘ └──────────────────────┘│
│  ┌──────────────────────────────────────────────────────── ┐│
│  │                   Service Discovery                     ││
│  └─────────────────────────────────────────────────────────┘│
└─────────────────────────────────────────────────────────────┘
         │                    │                    │
         ▼                    ▼                    ▼
┌──────────────┐    ┌──────────────┐    ┌──────────────────┐
│   Targets    │    │ Alertmanager │    │     Grafana      │
│ (Ex[[domain-19-landscape-references/sandbox/porter/porter.md|porter]]s)  │    │  (Alerting)  │    │ (Visualization)  │
└──────────────┘    └──────────────┘    └──────────────────┘
```

---

## 技术架构

### 整体架构
Prometheus 采用拉取（Pull）模式采集指标，通过内置的时序数据库（TSDB）存储数据，支持 PromQL 查询和 Alertmanager 告警。

### 核心组件
| 组件 | 功能 | 说明 |
|:---|:---|:---|
| Prometheus Server | 核心服务 | 数据采集、存储、查询 |
| Alertmanager | 告警管理 | 告警分组、抑制、路由 |
| Pushgateway | 推送网关 | 支持短生命周期任务推送指标 |
| Exporters | 指标导出器 | 将第三方系统指标转换为 Prometheus 格式 |
| Client Libraries | 客户端库 | 应用程序埋点 SDK |

### 工作原理
1. Prometheus Server 通过服务发现找到监控目标
2. 定期从目标的 /metrics 端点拉取指标
3. 将指标数据存储到本地 TSDB
4. 根据告警规则评估并发送告警到 Alertmanager
5. 用户通过 PromQL 查询数据或在 Grafana 可视化

---

## 使用场景

### 典型应用
- **基础设施监控**: 服务器、网络设备、存储系统
- **Kubernetes 监控**: 集群、节点、Pod、容器
- **应用性能监控**: 微服务延迟、吞吐量、错误率
- **业务指标监控**: 订单量、用户活跃度等业务指标

### 适用条件
- 需要灵活的多维度指标查询
- 基于拉取模式的监控架构
- 云原生和 Kubernetes 环境
- 需要与 Grafana 等工具集成

### 不适用场景
- 需要 100% 精确计费的场景
- 长期历史数据存储（需要配合 Thanos/Cortex）
- 日志和链路追踪（应使用专门工具）

---

## 快速开始

### 安装部署
```bash
# Docker 运行
docker run -p 9090:9090 prom/prometheus

# Kubernetes 部署（使用 kube-prometheus-stack）
helm repo add prometheus-community https://prometheus-community.github.io/helm-charts
helm install prometheus prometheus-community/kube-prometheus-stack
```

### 基础配置
```yaml
# prometheus.yml
global:
  scrape_interval: 15s
  evaluation_interval: 15s

alerting:
  alertmanagers:
    - static_configs:
        - targets:
          - alertmanager:9093

rule_files:
  - "alert_rules.yml"

scrape_configs:
  - job_name: 'prometheus'
    static_configs:
      - targets: ['localhost:9090']
  
  - job_name: 'kubernetes-pods'
    kubernetes_sd_configs:
      - role: pod
    relabel_configs:
      - source_labels: [__meta_kubernetes_pod_annotation_prometheus_io_scrape]
        action: keep
        regex: true
```

### 验证测试
```bash
# 访问 Prometheus UI
curl http://localhost:9090

# 查询指标
curl 'http://localhost:9090/api/v1/query?query=up'

# PromQL 查询示例
rate(http_requests_total[5m])
```

---

## 最佳实践

### 生产环境建议
- 配置数据持久化存储
- 使用联邦集群处理大规模环境
- 合理设置数据保留期限
- 配置告警规则和通知渠道

### 性能优化
- 控制标签基数，避免高基数标签
- 合理配置采集间隔
- 使用录制规则预计算常用查询
- 监控 Prometheus 自身资源使用

### 安全加固
- 启用 TLS 加密通信
- 配置基本认证或 OAuth
- 限制 API 访问权限
- 定期备份配置和数据

---

## 生态集成

### 相关 CNCF 项目
- **Thanos**: 高可用长期存储方案
- **Cortex**: 多租户 Prometheus 即服务
- **OpenTelemetry**: 可观测性数据采集
- **Alertmanager**: 告警管理和路由

### 常见集成方案
- Prometheus + Grafana 可视化
- Prometheus + Alertmanager + PagerDuty 告警
- Prometheus + Thanos 长期存储
- Prometheus Operator 在 Kubernetes 中的部署

---

## 社区与支持

### 社区资源
- Slack: https://slack.cncf.io #prometheus
- 邮件列表: prometheus-users@googlegroups.com
- 论坛: https://groups.google.com/g/prometheus-users

### 贡献指南
访问 https://prometheus.io/community/ 了解参与方式

---

## 参考资源

- [官方文档](https://prometheus.io/docs)
- [GitHub Repo](https://github.com/prometheus/prometheus)
- [CNCF 项目页面](https://www.cncf.io/projects/prometheus/)
- [PromQL 教程](https://prometheus.io/docs/prometheus/latest/querying/basics/)

---

**维护者**: Kudig Team | **许可证**: MIT

## Related

- [[CONTRIBUTING.md|CONTRIBUTING]]
- [[domain-17-system-foundation/topic-cheat-sheet/go.md|go]]
- [[domain-17-system-foundation/topic-cheat-sheet/helm.md|helm]]
- [[domain-17-system-foundation/topic-cheat-sheet/promql.md|promql]]
- [[_reports/WIKI-LINT-REPORT-2026-05-21|Wiki Lint Report — 2026-05-21]] — Cross-reference
- [[references/KUDIG Cheat Sheet Index|KUDIG Cheat Sheet Index]] — Cross-reference
- [[references/specialized-workloads-terms|K8s 专用工作负载术语参考]] — Cross-reference
- [[references/release-notes-observability|发布说明索引 — 可观测性]] — Cross-reference
- [[references/k8s-observability-ecosystem|可观测性体系：指标、日志、链路追踪与混沌工程]] — Cross-reference
- [[references/fundamentals-terms|K8s 基础概念术语参考]] — Cross-reference
- [[references/kudig-ecosystem-guide|KUDIG 开源生态指南与深度研究指南]] — Cross-reference
- [[references/k8s-advanced-ecosystem|硬件知识体系、CNCF 全景生态与 eBPF 平台工程]] — Cross-reference
- [[references/observability-terms|K8s 可观测性术语参考]] — Cross-reference
- [[references/k8s-deployment-create|Kubernetes Deployment 创建操作指南]] — Cross-reference
- [[references/tooling-terms|K8s 工具链术语参考]] — Cross-reference
- [[references/multi-cloud-terms|K8s 多云架构术语参考]] — Cross-reference
- [[references/kudig-man-pages-index|KUDIG Man Pages Index]] — Cross-reference
- [[references/version-upgrade-guide|版本升级指南]] — Cross-reference
- [[references/operations-terms|K8s 运维运营术语参考]] — Cross-reference
- [[synthesis/kubeadm-cluster-operations|kubeadm 集群运维全景]] — Cross-reference
- [[synthesis/K8s 故障分布与 MTTR 基准|K8s 故障分布与 MTTR 基准]] — Cross-reference
- [[synthesis/控制器模式 × Operator 模式|控制器模式 × Operator 模式]] — Cross-reference
- [[synthesis/Structural Troubleshooting Framework|Structural Troubleshooting Framework]] — Cross-reference
- [[concepts/bp-common-best-practices|Kubernetes 通用最佳实践参考]] — Cross-reference
- [[concepts/kubernetes-pki-certificate-system|Kubernetes PKI 证书体系]] — Cross-reference
- [[concepts/bp-observability|最佳实践：Observability]] — Cross-reference
- [[concepts/autoscaling-strategies|Autoscaling Strategies]] — Cross-reference
- [[concepts/ai-agent-openclaw-workspace|OpenClaw 工作空间配置]] — Cross-reference
- [[concepts/ai-agent-README|AI Agent 工程专题]] — Cross-reference
- [[concepts/cni-networking-model|CNI 网络模型与插件对比]] — Cross-reference
- [[concepts/storage-tool-evolution|存储工具演进]] — Cross-reference
- [[concepts/KUDIG Knowledge Base Architecture|KUDIG Knowledge Base Architecture]] — Cross-reference
- [[concepts/bp-README|Kubernetes 最佳实践指南]] — Cross-reference
- [[concepts/production-operations-best-practices|Production Operations Best Practices]] — Cross-reference
- [[concepts/observability-stack-evolution|可观测性栈演进]] — Cross-reference
- [[skills/learn-01-day-one-checklist|Day 1: 新人首日检查清单]] — Cross-reference
- [[skills/learn-README|新人上手快速路径（Quick Start）]] — Cross-reference
- [[skills/skill-20-networkpolicy-connectivity|NetworkPolicy 连通性故障诊断 / NetworkPolicy Connectivity Troubleshooting]] — Cross-reference
- [[skills/k8s-monitoring-guide|Kubernetes 监控最佳实践]] — Cross-reference
- [[skills/Kubernetes Diagnostic Skills Overview|Kubernetes Diagnostic Skills Overview]] — Cross-reference
- [[skills/learn-03-oncall-handoff|Day 3: 值班交接 SOP]] — Cross-reference
- [[skills/monitoring-fta|监控与告警异常故障树分析]] — Cross-reference
- [[skills/ts-control-plane|控制平面故障排查]] — Cross-reference
- [[skills/learn-inner-training|Kubernetes 培训：Inner Training]] — Cross-reference
- [[skills/kubelet-eviction-mechanism|kubelet 资源驱逐机制]] — Cross-reference
- [[skills/monitor-kubernetes-metrics|Monitor Kubernetes Metrics]] — Cross-reference
- [[skills/learn-public-training|Kubernetes 培训：Public Training]] — Cross-reference
- [[skills/learn-02-first-ticket-guide|Day 2: 第一个工单处理指南]] — Cross-reference
- [[skills/FTA-Driven Runbook Automation|FTA-Driven Runbook Automation]] — Cross-reference
- [[skills/ts-storage|存储故障排查]] — Cross-reference
- [[skills/ts-cluster-operations|集群运维故障排查]] — Cross-reference
- [[entities/cncf-cicd|CNCF CI/CD 与发布管理项目全景]] — Cross-reference
- [[entities/cncf-orchestration|CNCF 编排与应用管理项目全景]] — Cross-reference
- [[entities/cncf-security|CNCF 安全与合规项目全景]] — Cross-reference
- [[entities/cncf-observability|CNCF 可观测性项目全景]] — Cross-reference
- [[domain-19-landscape-references/topic-index/observability-index|Observability 可观测性知识图谱索引]]
- [[domain-19-landscape-references/topic-index/gitops-cicd-index|GitOps / CI-CD 全局索引]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-0.12|prometheus v0.12 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.32|prometheus v2.32 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.22|prometheus v2.22 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.47|prometheus v2.47 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.16|prometheus v2.16 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.36|prometheus v2.36 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.53|prometheus v2.53 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-0.16|prometheus v0.16 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.12|prometheus v2.12 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.43|prometheus v2.43 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.26|prometheus v2.26 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.37|prometheus v2.37 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.52|prometheus v2.52 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-0.17|prometheus v0.17 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.13|prometheus v2.13 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.42|prometheus v2.42 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.27|prometheus v2.27 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-0.13|prometheus v0.13 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-1.8|prometheus v1.8 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.33|prometheus v2.33 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.23|prometheus v2.23 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.46|prometheus v2.46 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.17|prometheus v2.17 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-0.18|prometheus v0.18 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-1.3|prometheus v1.3 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.38|prometheus v2.38 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.28|prometheus v2.28 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-3.5|prometheus v3.5 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-1.7|prometheus v1.7 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.0|prometheus v2.0 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.18|prometheus v2.18 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.49|prometheus v2.49 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-3.1|prometheus v3.1 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-1.6|prometheus v1.6 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.19|prometheus v2.19 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.48|prometheus v2.48 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-3.0|prometheus v3.0 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-0.19|prometheus v0.19 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-1.2|prometheus v1.2 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.39|prometheus v2.39 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.29|prometheus v2.29 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-3.4|prometheus v3.4 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-1.5|prometheus v1.5 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-3.3|prometheus v3.3 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.6|prometheus v2.6 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-1.1|prometheus v1.1 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-3.7|prometheus v3.7 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-3.10|prometheus v3.10 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.7|prometheus v2.7 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-1.0|prometheus v1.0 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-3.6|prometheus v3.6 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-3.11|prometheus v3.11 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-1.4|prometheus v1.4 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-3.2|prometheus v3.2 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-0.20|prometheus v0.20 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.34|prometheus v2.34 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.8|prometheus v2.8 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.51|prometheus v2.51 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-0.14|prometheus v0.14 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.41|prometheus v2.41 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.10|prometheus v2.10 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-3.9|prometheus v3.9 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.24|prometheus v2.24 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.55|prometheus v2.55 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.30|prometheus v2.30 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.20|prometheus v2.20 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.14|prometheus v2.14 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.45|prometheus v2.45 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.54|prometheus v2.54 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-0.11|prometheus v0.11 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.31|prometheus v2.31 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.21|prometheus v2.21 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.15|prometheus v2.15 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.44|prometheus v2.44 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.35|prometheus v2.35 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.9|prometheus v2.9 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.50|prometheus v2.50 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-0.15|prometheus v0.15 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.40|prometheus v2.40 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.11|prometheus v2.11 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-3.8|prometheus v3.8 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/prometheus/RELEASE-NOTES-2.25|prometheus v2.25 Release Notes]]

## See Also

- [[domain-19-landscape-references/graduated/prometheus/02-prometheus-promql-advanced.md|02-prometheus-promql-advanced]]
- [[domain-19-landscape-references/graduated/prometheus/03-prometheus-ha-deployment.md|03-prometheus-ha-deployment]]
- [[domain-19-landscape-references/graduated/prometheus/02-prometheus-promql-advanced.md|02-prometheus-promql-advanced]]
- [[domain-19-landscape-references/graduated/prometheus/03-prometheus-ha-deployment.md|03-prometheus-ha-deployment]]
