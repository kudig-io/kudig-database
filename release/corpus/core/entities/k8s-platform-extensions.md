---
title: 平台运维与扩展生态：Helm、CI/CD、Operator 开发与服务网格
description: '# 平台运维与扩展生态'
summary: '# 平台运维与扩展生态'
category: reference
tags:
- k8s
- platform
- helm
- ci-cd
- operator
- service-mesh
- istio
- envoy
- crd
- webhook
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 平台运维与扩展生态：Helm、CI/CD、Operator 开发与服务网格 是什么
- 如何 平台运维与扩展生态：Helm、CI/CD、Operator 开发与服务网格
trigger_keywords:
- 平台运维与扩展生态：Helm
- CI
- CD
- Operator
- 开发与服务网格
prerequisites:
- kubectl-basics
- helm-basics
- service-mesh-basics
---



# 平台运维与扩展生态

## [[helm]] 包管理

Helm 三大概念：
- **Chart**：应用包模板
- **Release**：Chart 的部署实例
- **Repository**：Chart 存储仓库

最佳实践：使用 Helmfile 管理多环境部署。

## CI/CD 流水线

| 工具 | 特点 | 适用场景 |
|------|------|----------|
| Tekton | K8s 原生 | 云原生 CI |
| GitHub Actions | 托管式 | 开源项目 |
| Argo Workflow | DAG 编排 | 复杂流水线 |
| Jenkins X | GitOps 集成 | 企业级 |

## Operator 开发

- **kubebuilder**：Go SDK，生成 CRD + Controller 脚手架
- **operator-sdk**：支持 Go/Ansible/Helm 多语言
- 核心组件：CRD 定义 + Reconciler 调谐逻辑 + Webhook 准入控制

## 服务网格

Istio 架构：
- **控制平面**：istiod（Pilot + Citadel + Galley）
- **数据平面**：Envoy sidecar 代理
- 功能：流量管理、安全（mTLS）、可观测性

---

> 来源：.zread/wiki/drafts/21-ping-tai-yun-wei-yu-kuo-zhan-sheng-tai-*.md

## Related

- [[concepts/服务网格 x 零信任安全.md|服务网格 x 零信任安全]] — 服务网格 x 零信任安全
- [[istio]] — Istio
- [[helm]] — Helm
- [[envoy]] — Envoy
- [[argo]] — Argo Workflows
