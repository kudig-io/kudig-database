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
│ (Exporters)  │    │  (Alerting)  │    │ (Visualization)  │
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
