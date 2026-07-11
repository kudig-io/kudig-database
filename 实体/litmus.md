---
title: LitmusChaos
description: '## 概述'
summary: '该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[概念/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]'
category: entities
tags:
- k8s
- cncf
- observability
- litmus
- prometheus
- grafana
- istio
- argocd
- crd
- operator
tier: peripheral
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- LitmusChaos 是什么
- 如何 LitmusChaos
trigger_keywords:
- LitmusChaos
prerequisites:
- kubectl-basics
- service-mesh-basics
- prometheus-basics
- monitoring-basics
- gitops-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# LitmusChaos

> **CNCF 状态**: Incubating | **类别**: Observability | **主要语言**: Go

## 概述

LitmusChaos 是由 Harness（原 MayaData）开源的云原生混沌工程平台，2020 年加入 CNCF Sandbox，后晋升为 Incubating。它提供完整的混沌实验编排和管理能力，帮助团队在受控环境中主动注入故障，测试系统弹性，发现潜在问题。LitmusChaos 提供预置的 ChaosHub 实验库，支持 Pod 杀死、网络延迟、CPU 压力等数十种故障注入场景，并通过 GitOps 方式管理混沌实验。

## 核心特性

- **ChaosHub**: 50+ 预置混沌实验（Pod Kill、Network Delay、CPU Hog、Disk Fill 等）
- **CRD 原生**: ChaosEngine、ChaosExperiment、ChaosResult CRD 声明式管理
- **GitOps 支持**: 混沌实验以 YAML 定义，通过 Git 版本控制管理
- **稳态假设**: 实验前后验证系统稳态指标（Hypothesis CRD）
- **多调度模式**: 支持 Cron、Manual、Automated 触发方式
- **可观测性**: Prometheus 指标导出和 Grafana 仪表盘

## 架构

LitmusChaos 采用控制平面-执行平面分离架构。控制平面（ChaosCenter）提供 Web UI 和 API，管理项目、用户和实验编排。执行平面由 ChaosOperator 和 ChaosRunner 组成——Operator 监听 ChaosEngine CRD，为每个实验创建 ChaosRunner Pod。ChaosRunner 注入混沌故障到目标 Pod/Node。实验执行使用 LitmusProbes（探针）验证稳态假设，结果写入 ChaosResult CRD。ChaosHub 提供实验模板，可克隆和自定义。

## Kubernetes 集成

LitmusChaos 完全基于 Kubernetes CRD 构建。ChaosEngine 定义实验目标和参数，ChaosExperiment 定义实验步骤，ChaosResult 记录执行结果。通过 ServiceAccount 和 RBAC 控制实验权限范围。支持命名空间级别的混沌隔离。实验可通过 ArgoCD 或 FluxCD 以 GitOps 方式部署。ChaosScheduler 支持 CronJob 式的定期实验。

## 生产使用场景

1. **弹性验证**: 在游戏日（Game Day）中注入 Pod 故障，验证自动恢复能力
2. **CI/CD 集成**: 在部署后自动运行混沌实验，确保新版本的弹性不退化
3. **多区域容灾**: 注入网络分区，验证跨区域故障切换
4. **容量规划**: 注入 CPU/内存压力，验证系统在高负载下的表现

## 安装

```bash
# Helm 安装
helm repo add litmuschaos https://litmuschaos.github.io/litmus-helm/
helm install litmus litmuschaos/litmus --namespace litmus --create-namespace
# 安装混沌实验
kubectl apply -f https://hub.litmuschaos.io/api/chaos/2.15.0?file=charts/generic/pod-delete/experiment.yaml
# 运行实验
kubectl apply -f - <<EOF
apiVersion: litmuschaos.io/v1alpha1
kind: ChaosEngine
metadata: { name: pod-kill-test }
spec:
  appinfo: { appns: default, applabel: app=test }
  chaosServiceAccount: litmus-admin
  experiments:
  - name: pod-delete
EOF
```

## 替代方案

| 项目 | 优势 | 劣势 |
|------|------|------|
| **LitmusChaos** | CNCF Incubating、UI 完善 | 资源开销中等 |
| Chaos Mesh | CNCF Incubating、中文社区强 | 架构较重 |
| Gremlin | 企业级、商业支持 | 商业产品 |
| Pumba | 轻量级、Docker 原生 | 功能有限、无 UI |

## 架构定位

在 CNCF 生态中，LitmusChaos 属于 **Observability / Reliability Engineering** 类别，是云原生混沌工程的两大主流平台之一（与 Chaos Mesh 并列）。

## 参考链接

- [[实体/prometheus-grafana.md|prometheus-grafana]]
- [[实体/argocd.md|argocd]]
- [[deployment]]
- [[实体/crd-custom-resources.md|crd-custom-resources]]
- [[operator-pattern]]

## Related

- [[openkruise]] — OpenKruise
- [[02-istio-advanced-traffic-management]] — Istio 高级流量管理
- [[vscode-kubernetes-tools]] — VS Code Kubernetes Tools
- [[prometheus]] — Prometheus
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- litmus
- [[实体/k8s-observability-ecosystem.md|可观测性体系：指标、日志、链路追踪与混沌工程]] — Cross-reference
- [[实体/operations-terms.md|K8s 运维运营术语参考]] — Cross-reference
- [[实体/cncf-infrastructure.md|CNCF 基础设施与混沌工程项目全景]] — Cross-reference
- [[生态参考/领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
