---
title: OpenCost [entities]
description: '## 概述'
summary: 'OpenCost 是 Kubernetes 成本监控的开源标准。它提供实时成本分配、多维度成本分析和优化建议，帮助团队了解和优化 Kubernetes 基础设施支出。'
category: entities
tags:
- k8s
- cncf
- cost
- opencost
- prometheus
- grafana
- crd
- operator
tier: supporting
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- OpenCost 是什么
- 如何 OpenCost
trigger_keywords:
- OpenCost
prerequisites:
- kubectl-basics
- prometheus-basics
- monitoring-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# OpenCost

> **CNCF 状态**: Incubating | **类别**: Cost | **主要语言**: Go

## 概述

OpenCost 是由 Kubecost 公司于 2021 年开源的 Kubernetes 成本监控规范和实现，2023 年进入 CNCF Incubating 阶段。它为 Kubernetes 集群提供**实时成本分配（Cost Allocation）**和**成本分析**能力，帮助团队精确了解每个命名空间、标签、服务、工作负载的资源消耗和对应成本。

OpenCost 的核心理念是"**FinOps for Kubernetes**"——将云财务运营（FinOps）实践引入 K8s。它通过采集集群的资源使用指标（CPU、内存、GPU、存储、网络），结合云厂商的定价数据，计算每个工作负载的实际成本。OpenCost 支持 AWS、Azure、GCP、阿里云等主流云厂商的定价，也支持自定义私有云定价。

## Key Features

- **实时成本监控**：分钟级别的成本数据采集，Pod 级别的成本分配
- **多维度分析**：按命名空间、标签（team/app/env）、Service、工作负载分析成本
- **多云定价**：AWS、Azure、GCP、阿里云等主流云厂商定价集成，也支持自定义定价
- **Prometheus 集成**：以 Prometheus 指标格式暴露成本数据，可在 Grafana 可视化
- **闲置资源检测**：识别已申请但未使用的资源（requested > used），量化浪费
- **成本分摊**：支持共享资源（如 namespace 中的基础设施）按比例分摊到团队

## Architecture

OpenCost 由 **Cost Model**（Go 服务，采集资源指标并计算成本）、**Cost Exporter**（Prometheus 指标导出器，暴露 `container_cpu_allocation_seconds` 等指标）、**Pricing Source**（云厂商定价 API 或自定义定价文件）和 **UI/Dashboard**（Web 界面，提供成本仪表盘）组成。Cost Model 通过 K8s API 采集 Pod 资源请求和使用量，结合节点定价计算每 Pod 的成本，结果通过 Prometheus 指标暴露。

## K8s 集成

OpenCost 作为 Deployment 部署在 Kubernetes 集群中。通过 Kubernetes API 采集 Pod、Node、PV 的资源数据。依赖 Prometheus 作为指标存储后端——OpenCost 导出的指标被 Prometheus 抓取，Grafana 通过 PromQL 查询和可视化。支持通过 Helm Chart 或 manifest 一键安装。

## 生产部署要点

- **标签策略**：使用一致的标签（team, app, env）便于成本分配
- **定期审查**：每周检查成本报告，识别异常增长
- **预算告警**：设置成本阈值告警
- **资源配额**：结合成本数据设置命名空间配额
- **闲置资源**：定期清理未使用的 PV 和负载均衡器

## 生产场景

1. **团队成本分摊**：按 namespace/team 标签计算每个团队的实际资源成本，支持内部计费
2. **成本优化**：识别过度申请资源的 Pod，调整 request 节省 30-50% 成本
3. **Spot 实例策略**：分析 Spot/按需实例的成本效益，优化实例类型组合
4. **FinOps 报告**：向管理层定期报告 Kubernetes 基础设施成本趋势

## 安装

```bash
# Helm 安装 OpenCost（依赖 Prometheus）
helm repo add opencost https://opencost.github.io/opencost-helm-chart
helm install opencost opencost/opencost \
  -n opencost --create-namespace \
  --set opencost.exporter.cloudProviderApiKey="" \
  --set opencost.prometheus.url=http://prometheus-server.monitoring.svc

# 或快速安装（含 Prometheus）
kubectl apply -f https://raw.githubusercontent.com/opencost/opencost/develop/kubernetes/opencost.yaml

# 查询成本 API
kubectl port-forward svc/opencost 9003 -n opencost
curl http://localhost:9003/allocation/compute?window=1d&aggregate=namespace
```

## 对比

| 特性 | OpenCost | Kubecost | CloudZero | Vantage |
|------|----------|----------|-----------|---------|
| 开源 | ✅ Apache 2.0 | ⚠️ 部分 | ❌ | ❌ |
| 多云 | ✅ | ✅ | ✅ | ✅ |
| K8s 原生 | ✅ | ✅ | ⚠️ | ⚠️ |
| 自托管 | ✅ | ✅ | ❌ | ❌ |

## 参考链接

- [[实体/prometheus-grafana.md|prometheus-grafana]]
- [[deployment]]
- [[概念/controller-pattern.md|controller-pattern]]
- [[概念/observability-pillars.md|observability-pillars]]
- [[概念/autoscaling-strategies.md|autoscaling-strategies]]

## Related

- [[piraeus-datastore]] — Piraeus Datastore
- [[k8up]] — K8up
- [[parsec]] — Parsec
- [[prometheus]] — Prometheus
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- opencost
- observability|CNCF 可观测性项目全景]] — Cross-reference
- [[生态参考/领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
