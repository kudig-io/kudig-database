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

- [[清单模式/Operator模式/01-operator-cr-design-patterns|Operator CR 设计模式]]
- [[清单模式/Operator模式/02-operator-reconciliation-patterns|Operator 调和模式]]
- [[清单模式/Operator模式/03-operator-finalizer-cleanup|Operator Finalizer 清理]]
- [[清单模式/Operator模式/04-operator-leader-election|Operator Leader Election]]
- [[清单模式/Operator模式/05-operator-webhook-patterns|Operator Webhook 模式]]
- [[清单模式/Operator模式/06-operator-status-conditions|Operator 状态条件]]
- [[清单模式/Operator模式/07-operator-metrics-observability|Operator 指标可观测性]]
- [[清单模式/Operator模式/08-operator-testing-strategies|Operator 测试策略]]

## 扩展机制 (Extension Mechanisms)

- [[专项技术/扩展机制/01-crd-development-guide|CRD 开发指南]]
- [[专项技术/扩展机制/02-operator-development-patterns|Operator 开发模式]]
- [[专项技术/扩展机制/03-admission-webhook-configuration|Admission Webhook 配置]]
- [[专项技术/扩展机制/04-api-aggregation-extension|API 聚合扩展]]

## 概念 (Concepts)

- [[概念/operator-pattern|Operator 模式]]
- [[概念/controller-pattern|控制器模式]]
- [[概念/eventual-consistency|最终一致性]]
- [[概念/pod-lifecycle|Pod 生命周期]]
- [[概念/控制器模式 × Operator 模式|控制器模式与 Operator 模式]]
- [[概念/声明式 API × 控制器模式|声明式 API 与控制器模式]]
- [[概念/high-availability-patterns|高可用模式]]
- [[概念/etcd × Operator 模式|etcd 与 Operator 模式]]
- [[概念/CRD × 可观测性|CRD 与可观测性]]

## 集群基础 (Cluster Fundamentals)

- [[集群基础/设计原则/03-controller-pattern|控制器模式]]
- [[集群基础/设计原则/05-informer-workqueue|Informer/WorkQueue]]
- [[集群基础/设计原则/10-cap-theorem-distributed-systems|CAP 定理分布式系统]]
- [[集群基础/设计原则/12-operator-development-guide|Operator 开发指南]]

## 平台工程 (Platform Engineering)

- [[平台工程/构建/20-crd-operator-development|CRD/Operator 开发]]
- [[平台工程/构建/21-api-aggregation|API 聚合]]
- [[平台工程/构建/22-client-libraries|客户端库]]
- [[平台工程/构建/99-java-k8s-client-operator-guide|Java K8s 客户端/Operator 指南]]

## 工作负载 (Workloads)

- [[工作负载/04-java-operator-sdk-development|Java Operator SDK 开发]]
- [[工作负载/核心工作负载/12-advanced-pod-patterns|高级 Pod 模式]]

## 技能 (Skills)

- [[技能/develop-crd-operator|CRD/Operator 开发]]
- [[技能/crd-operator-fta|CRD/Operator FTA]]
- [[技能/controller-manager-fta|Controller Manager FTA]]
- [[技能/daemonset-fta|DaemonSet FTA]]
- [[技能/monitoring-fta|Monitoring FTA]]

## 故障诊断 (Troubleshooting)

- [[故障诊断/高级排障/structural-08-cluster-operations/05-crd-operator-troubleshooting|CRD/Operator 排障]]
- [[故障诊断/FTA故障树/list/crd-operator-fta|CRD/Operator 故障树]]
- [[故障诊断/FTA故障树/list/controller-manager-fta|Controller Manager 故障树]]

## 数据库中间件 Operator (Database Operators)

- [[数据库中间件/Operator管理/01-database-operator-patterns|数据库 Operator 模式]]
- [[数据库中间件/Operator管理/02-operator-comparison-mysql-postgres-redis|Operator 对比]]
- [[数据库中间件/Operator管理/03-operator-lifecycle-management|Operator 生命周期管理]]
- [[数据库中间件/数据库/07-redis-kubernetes-operator|Redis K8s Operator]]
- [[数据库中间件/数据库/08-kafka-kubernetes-strimzi|Kafka Strimzi]]

## 可观测性 (Observability)

- [[技能/best-practices/best-practices/observability/monitoring|监控最佳实践]]
- [[技能/best-practices/best-practices/observability/tracing|追踪最佳实践]]
- [[清单模式/Operator模式/07-operator-metrics-observability|Operator 指标可观测性]]

## 研究 (Research)

- [[研究/operator-development-patterns|Operator 开发模式]]

## 知识字典 (Knowledge Dictionary)

- [[系统基础/知识字典/platform-engineering/custom-resources|自定义资源]]
- [[系统基础/知识字典/platform-engineering/operator-pattern|Operator 模式]]
- [[系统基础/知识字典/platform-engineering/operator-framework|Operator Framework]]
- [[系统基础/知识字典/platform-engineering/extending-the-kubernetes-api|扩展 K8s API]]
- [[系统基础/知识字典/platform-engineering/admission-webhook-good-practices|Admission Webhook 最佳实践]]
- [[系统基础/知识字典/platform-engineering/coordinated-leader-election|协调 Leader Election]]
- [[系统基础/知识字典/fundamentals/controllers|控制器]]
- [[系统基础/知识字典/fundamentals/the-kubernetes-api|Kubernetes API]]

## 实体 (Entities)

- [[实体/operator-framework|Operator Framework]]
- [[实体/crd-custom-resources|CRD 自定义资源]]
- [[实体/strimzi|Strimzi]]
- [[实体/cloudnativepg|CloudNativePG]]
- [[实体/openkruise|OpenKruise]]
- [[实体/knative|Knative]]
- [[实体/crossplane|Crossplane]]
- [[实体/kubeflow|Kubeflow]]
- [[实体/kserve|KServe]]
- [[实体/keda|KEDA]]
- [[实体/cncf-orchestration|CNCF Orchestration]]

## 应用架构 (Application Architecture)

- [[应用模式/行业架构/07-iot-platform-architecture|IoT 平台架构]]
- [[应用模式/行业架构/08-ai-ml-inference-architecture|AI/ML 推理架构]]
- [[应用模式/行业架构/09-gaming-backend-architecture|游戏后端架构]]

## Related Tags

- [[标签/k8s|k8s]]
- [[标签/helm|helm]]
- [[标签/gitops|gitops]]
- [[标签/production|production]]
