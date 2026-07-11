---
title: K8sGPT (entities)
description: '## 概述'
summary: 'K8sGPT 是一款 AI 驱动的 Kubernetes 诊断工具，利用大语言模型 (LLM) 自动分析集群问题并提供人类可读的解释和建议。它扫描 Kubernetes 集群中的问题，结合 AI 能力生成诊断报告，帮助 SRE 和开发者快速定位和解决问题。'
category: entities
tags:
- k8s
- cncf
- platform
- k8sgpt
- prometheus
- grafana
- job
- cronjob
- ingress
- networkpolicy
tier: peripheral
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- K8sGPT 是什么
- 如何 K8sGPT
trigger_keywords:
- K8sGPT
prerequisites:
- kubectl-basics
- prometheus-basics
- monitoring-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# K8sGPT

> **CNCF 状态**: Sandbox | **类别**: Platform | **主要语言**: Go

## 概述

K8sGPT 是一款 AI 驱动的 Kubernetes 诊断工具，由 Alex Jones 创建，2023 年加入 CNCF Sandbox。它利用大语言模型（LLM）自动分析集群问题并提供人类可读的解释和建议。K8sGPT 扫描 Kubernetes 集群中的问题，结合 AI 能力生成诊断报告，帮助 SRE 和开发者快速定位和解决问题。K8sGPT 将 SRE 知识编码为分析器（Analyzer），覆盖 Pod、Service、Ingress、Deployment 等核心资源。

## 核心特性

- **多 LLM 后端**: 支持 OpenAI、Azure OpenAI、Amazon Bedrock、LocalAI、Ollama 等
- **内置分析器**: 覆盖 Pod 崩溃、Service 配置、Ingress 后端、网络策略等 30+ 场景
- **AI 解释**: 使用 LLM 将技术错误转化为人类可读的因果分析和修复建议
- **Operator 模式**: 作为 K8s Operator 持续监控集群健康状态
- **多语言**: 支持中文、英文、日文等多种语言输出
- **自定义分析器**: 通过插件架构扩展内置分析能力

## 架构

K8sGPT 分为 CLI 和 Operator 两种运行模式。CLI 模式下，它通过 Kubernetes API 收集资源状态，经分析器（Analyzer）识别问题模式，再将问题发送给 LLM 后端生成自然语言解释。Operator 模式下，K8sGPT 以 Deployment 彐式运行在集群内，通过 CRD（K8sGPT / Result）管理扫描任务和结果。结果对象包含问题描述、严重级别和建议操作，可集成到告警系统。

## Kubernetes 集成

K8sGPT 通过 Kubernetes API Server 读取集群资源状态（Pod、Service、Ingress、Node 等），无需在节点上部署代理。Operator 模式通过 CRD 声明式管理扫描配置和结果，支持与 Prometheus AlertManager 集成实现自动化告警。CLI 模式使用 kubeconfig 连接集群，可作为 kubectl 插件使用。扫描结果以 Kubernetes 自定义资源（Result CRD）存储，便于 GitOps 管理。

## 生产使用场景

1. **日常巡检**: 配合 CronJob 定期运行 K8sGPT 扫描，生成集群健康报告
2. **故障诊断**: 快速分析 Pod CrashLoopBackOff、Service 无端点等问题
3. **告警增强**: 将 K8sGPT 结果集成到 AlertManager，在告警中附加 AI 解释
4. **新人培训**: 利用 AI 生成的解释帮助新 SRE 理解集群问题

## 安装

```bash
# 安装 CLI
brew install k8sgpt
# 初始化（配置 AI 后端）
k8sgpt auth
# 扫描集群
k8sgpt analyze --explain
# Operator 模式
helm repo add k8sgpt https://charts.k8sgpt.ai/
helm install k8sgpt k8sgpt/k8sgpt-operator -n k8sgpt-system --create-namespace
```

## 替代方案

| 项目 | 优势 | 劣势 |
|------|------|------|
| **K8sGPT** | AI 驱动、解释易懂、社区活跃 | 依赖外部 LLM API、成本考量 |
| HolmesGPT | 与 Robusta 集成更深 | 生态较小 |
| Robusta | 全面的自动化诊断平台 | 架构更重、部署复杂 |
| Prometheus Alerts | 无 AI 依赖、成熟稳定 | 仅提供指标告警，无根因分析 |

## 架构定位

在 CNCF 生态中，K8sGPT 属于 **Platform / AIOps** 类别，代表了 AI 与 Kubernetes 运维结合的趋势。它与 Prometheus、Grafana 等监控工具互补，为可观测性增加智能诊断维度。

## 参考链接

- [[实体/prometheus-grafana.md|prometheus-grafana]]
- [[实体/trivy.md|trivy]]
- [[实体/networkpolicy.md|networkpolicy]]
- [[deployment]]
- [[实体/crd-custom-resources.md|crd-custom-resources]]

## Related

- [[spire]] — SPIRE
- [[akri]] — Akri
- [[实体/cncf-edge-ai.md|cncf-edge-ai]] — CNCF 边缘计算与 AI/ML 项目全景
- [[confidential-containers]] — Confidential Containers (CoCo)
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- k8sgpt
- [[生态参考/领域索引/etcd-index.md|etcd 知识图谱索引]]
- [[生态参考/领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
