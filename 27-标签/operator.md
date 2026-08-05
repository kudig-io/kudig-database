---
title: operator
description: Operator 模式标签枢纽 — 涵盖 CRD、控制器模式、调和循环、Finalizer、Webhook、Leader Election、Operator SDK 等全部 Operator 领域知识
category: tag-index
tags:
- operator
- crd
- controller
- reconciliation
- kubebuilder
tier: core
difficulty: advanced
domain: platform-engineering
created: '2026-07-11'
last_updated: '2026-07-21'
---

# operator Tag Hub

> Operator 模式页面 — CRD、控制器模式、调和循环、Finalizer、Webhook、Leader Election 等。

## 核心定义

**Operator 模式**是 Kubernetes 的扩展机制，通过 CRD（Custom Resource Definition）+ 自定义控制器将领域知识编码为软件，实现复杂有状态应用的自动化运维。Operator 封装了安装、升级、备份、故障恢复等运维操作。

### Operator 核心组件

| 组件 | 职责 |
|------|------|
| CRD | 定义自定义资源的 Schema |
| Controller | Watch → Compare → Act 调和循环 |
| Webhook | 准入控制（Validating/Mutating） |
| Finalizer | 资源删除前的清理逻辑 |
| Status | 报告资源实际状态 |
| Owner Reference | 级联删除与 GC |


## Operator 清单模式 (Operator Manifest Patterns)

- [[03-清单模式/04-Operator模式/01-operator-cr-design-patterns|Operator CR 设计模式]]
- [[03-清单模式/04-Operator模式/03-operator-reconciliation-patterns|Operator 调和模式]]
- [[03-清单模式/04-Operator模式/04-operator-finalizer-cleanup|Operator Finalizer 清理]]
- [[03-清单模式/04-Operator模式/05-operator-leader-election|Operator Leader Election]]
- [[03-清单模式/04-Operator模式/06-operator-webhook-patterns|Operator Webhook 模式]]
- [[03-清单模式/04-Operator模式/07-operator-status-conditions|Operator 状态条件]]
- [[03-清单模式/04-Operator模式/08-operator-metrics-observability|Operator 指标可观测性]]
- [[03-清单模式/04-Operator模式/09-operator-testing-strategies|Operator 测试策略]]

## 扩展机制 (Extension Mechanisms)

- [[16-专项技术/03-扩展机制/01-crd-development-guide|CRD 开发指南]]
- [[16-专项技术/03-扩展机制/02-operator-development-patterns|Operator 开发模式]]
- [[16-专项技术/03-扩展机制/03-admission-webhook-configuration|Admission Webhook 配置]]
- [[16-专项技术/03-扩展机制/04-api-aggregation-extension|API 聚合扩展]]

## 概念 (Concepts)

- [[22-概念/01-核心架构/operator-pattern|Operator 模式]]
- [[22-概念/01-核心架构/controller-pattern|控制器模式]]
- [[22-概念/01-核心架构/eventual-consistency|最终一致性]]
- [[22-概念/02-工作负载/pod-lifecycle|Pod 生命周期]]
- [[22-概念/11-交叉分析/控制器模式 × Operator 模式|控制器模式与 Operator 模式]]
- [[22-概念/11-交叉分析/声明式 API × 控制器模式|声明式 API 与控制器模式]]
- [[22-概念/08-可靠性与运维/high-availability-patterns|高可用模式]]
- [[22-概念/11-交叉分析/etcd × Operator 模式|etcd 与 Operator 模式]]
- [[22-概念/11-交叉分析/CRD × 可观测性|CRD 与可观测性]]

## 集群基础 (Cluster Fundamentals)

- [[01-集群基础/02-设计原则/04-controller-pattern|控制器模式]]
- [[01-集群基础/02-设计原则/06-informer-workqueue|Informer/WorkQueue]]
- [[01-集群基础/02-设计原则/11-cap-theorem-distributed-systems|CAP 定理分布式系统]]
- [[01-集群基础/02-设计原则/13-operator-development-guide|Operator 开发指南]]

## 平台工程 (Platform Engineering)

- [[10-平台工程/01-构建/10-crd-operator-development|CRD/Operator 开发]]
- [[10-平台工程/01-构建/11-api-aggregation|API 聚合]]
- [[10-平台工程/01-构建/12-client-libraries|客户端库]]
- [[10-平台工程/01-构建/16-java-k8s-client-operator-guide|Java K8s 客户端/Operator 指南]]

## 工作负载 (Workloads)

- [[02-工作负载/02-Java-on-K8s/03-java-operator-sdk-development|Java Operator SDK 开发]]
- [[02-工作负载/01-核心工作负载/12-advanced-pod-patterns|高级 Pod 模式]]

## 技能 (Skills)

- [[26-技能/02-控制面/crd-operator/运维操作/develop-crd-operator|CRD/Operator 开发]]
- [[26-技能/02-控制面/crd-operator/crd-operator-fta|CRD/Operator FTA]]
- [[26-技能/02-控制面/controller-manager/controller-manager-fta|Controller Manager FTA]]
- [[26-技能/04-工作负载/daemonset/daemonset-fta|DaemonSet FTA]]
- [[26-技能/08-可观测性/monitoring/monitoring-fta|Monitoring FTA]]

## 故障诊断 (Troubleshooting)

- [[19-故障诊断/04-高级排障/structural-08-cluster-operations/05-crd-operator-troubleshooting|CRD/Operator 排障]]
- [[19-故障诊断/06-FTA故障树/list/crd-operator-fta|CRD/Operator 故障树]]
- [[19-故障诊断/06-FTA故障树/list/controller-manager-fta|Controller Manager 故障树]]

## 数据库中间件 Operator (Database Operators)

- [[07-数据库中间件/05-Operator管理/01-database-operator-patterns|数据库 Operator 模式]]
- [[07-数据库中间件/05-Operator管理/02-operator-comparison-mysql-postgres-redis|Operator 对比]]
- [[07-数据库中间件/05-Operator管理/03-operator-lifecycle-management|Operator 生命周期管理]]
- [[07-数据库中间件/01-数据库/07-redis-kubernetes-operator|Redis K8s Operator]]
- [[07-数据库中间件/01-数据库/08-kafka-kubernetes-strimzi|Kafka Strimzi]]

## 可观测性 (Observability)

- [[20-最佳实践/01-best-practices/observability/monitoring|监控最佳实践]]
- [[20-最佳实践/01-best-practices/observability/tracing|追踪最佳实践]]
- [[03-清单模式/04-Operator模式/08-operator-metrics-observability|Operator 指标可观测性]]

## 研究 (Research)

- [[25-研究/03-平台与交付/operator-development-patterns|Operator 开发模式]]

## 知识字典 (Knowledge Dictionary)

- [[17-系统基础/06-知识字典/platform-engineering/custom-resources|自定义资源]]
- [[17-系统基础/06-知识字典/platform-engineering/operator-pattern|Operator 模式]]
- [[17-系统基础/06-知识字典/platform-engineering/operator-framework|Operator Framework]]
- [[17-系统基础/06-知识字典/platform-engineering/extending-the-kubernetes-api|扩展 K8s API]]
- [[17-系统基础/06-知识字典/platform-engineering/admission-webhook-good-practices|Admission Webhook 最佳实践]]
- [[17-系统基础/06-知识字典/platform-engineering/coordinated-leader-election|协调 Leader Election]]
- [[17-系统基础/06-知识字典/fundamentals/controllers|控制器]]
- [[17-系统基础/06-知识字典/fundamentals/the-kubernetes-api|Kubernetes API]]

## 实体 (Entities)

- [[23-实体/10-平台与开发工具/operator-framework|Operator Framework]]
- [[23-实体/02-K8s核心组件/crd-custom-resources|CRD 自定义资源]]
- [[23-实体/12-数据与消息/strimzi|Strimzi]]
- [[23-实体/12-数据与消息/cloudnativepg|CloudNativePG]]
- [[23-实体/09-编排调度/openkruise|OpenKruise]]
- [[23-实体/09-编排调度/knative|Knative]]
- [[23-实体/08-交付与制品/crossplane|Crossplane]]
- [[23-实体/11-AI与边缘/kubeflow|Kubeflow]]
- [[23-实体/11-AI与边缘/kserve|KServe]]
- [[23-实体/09-编排调度/keda|KEDA]]
- [[23-实体/15-参考与索引/cncf-orchestration|CNCF Orchestration]]

## 应用架构 (Application Architecture)

- [[04-应用模式/02-行业架构/07-iot-platform-architecture|IoT 平台架构]]
- [[04-应用模式/02-行业架构/08-ai-ml-inference-architecture|AI/ML 推理架构]]
- [[04-应用模式/02-行业架构/09-gaming-backend-architecture|游戏后端架构]]

## Operator 技术全景

### Operator 设计模式

| 模式 | 说明 | 示例 |
|---|---|---|
| 基础 Operator | CRUD + Reconcile | etcd-operator |
| 有状态 Operator | 备份/恢复/升级 | prometheus-operator |
| 多集群 Operator | 跨集群管理 | cluster-api |
| Day-2 Operator | 运维自动化 | strimzi-kafka |

### Operator 开发框架

| 框架 | 语言 | 特点 |
|---|---|---|
| Operator SDK | Go/Ansible/Helm | 官方推荐 |
| Kubebuilder | Go | 轻量级 |
| Metacontroller | 任意语言 | Webhook 方式 |
| KEDA | YAML | 事件驱动 |

## 面试要点

1. **Q：Operator 的核心价值？**
   A：将运维知识代码化、自动化复杂操作、提供声明式 API、实现自愈能力。

2. **Q：Operator vs Helm 的区别？**
   A：Helm：包管理、一次性部署。Operator：持续调和、Day-2 运维、状态管理。

3. **Q：如何设计高质量的 Operator？**
   A：明确 CRD 语义、实现幂等 Reconcile、完善状态报告、支持升级回滚、充分测试。

## Related Tags

- [[27-标签/k8s|k8s]]
- [[27-标签/helm|helm]]
- [[27-标签/gitops|gitops]]
- [[27-标签/production|production]]
