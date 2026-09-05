---
title: Helm
description: Helm 是 Kubernetes 的包管理器，被称为「K8s 的 apt/yum」。它通过 Chart（模板化的 YAML 集合）简化应用的打包、分发和部署，...
summary: Helm 是 Kubernetes 的包管理器，被称为「K8s 的 apt/yum」。它通过 Chart（模板化的 YAML 集合）简化应用的打包、分发和部署，...
category: dictionary
tags:
- k8s
- glossary
- helm
- package-manager
- gitops
tier: core
created: '2026-06-24'
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Helm 是什么
- Helm 详解
trigger_keywords:
- Helm
- dictionary
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Helm

> **英文名**: Helm

## 概述

Helm 是 Kubernetes 的包管理器，被称为「K8s 的 apt/yum」。它通过 Chart（模板化的 YAML 集合）简化应用的打包、分发和部署，是 Kubernetes 生态中最广泛使用的工具之一。

## 核心概念/原理

### 核心概念

- **Chart**：Helm 包，包含模板化的 K8s 资源定义。
- **Release**：Chart 的一次部署实例。
- **Repository**：Chart 仓库（如 Artifact Hub）。

### Chart 结构

```
mychart/
├── Chart.yaml       # 元数据
├── values.yaml      # 默认配置值
├── templates/       # Go 模板文件
│   ├── deployment.yaml
│   ├── service.yaml
│   └── _helpers.tpl
└── charts/          # 依赖 Chart
```

## 关键机制或特性

- **Helm v3**：移除了 Tiller 服务端组件，直接通过 kubeconfig 与 apiserver 交互。
- **Release 版本管理**：每次 `helm upgrade` 自动生成新版本，支持 `helm rollback`。
- **模板引擎**：基于 Go text/template，支持 values 注入和条件渲染。
- **Hook 机制**：在 install/upgrade/delete 前后执行 Job/Pod。
- **OCI Registry**：Helm v3.8+ 支持将 Chart 推送到 OCI 兼容的容器仓库。

## 使用场景与最佳实践

- 使用 `helm template` 本地渲染 Chart 检查生成的 YAML。
- 使用 `helm lint` 验证 Chart 语法和最佳实践。
- 生产环境推荐配合 Helmfile 或 ArgoCD 进行声明式管理。
- 避免在 Chart 中硬编码镜像版本，使用 values 注入。
- 使用 `helm diff` 插件预览 upgrade 变更。

## 参考链接

- [Helm Official Documentation](https://helm.sh/docs/)

## Related

- [[17-系统基础/06-知识字典/tooling/kubectl.md|Kubectl]]
- [[17-系统基础/06-知识字典/tooling/kustomize.md|Kustomize]]
- [[17-系统基础/06-知识字典/workloads/deployment.md|Deployment]]
- [[17-系统基础/06-知识字典/platform-engineering/manifest.md|Manifest]]
- [[17-系统基础/06-知识字典/operations/rolling-update.md|Rolling Update]]


<!-- risk-assessed -->
- [[04-应用模式/02-行业架构/19-cloudnative-devops-architecture|云原生 DevOps 平台 Kubernetes 生产架构设计]]
- [[04-应用模式/02-行业架构/17-saas-multitenant-architecture|SaaS 多租户平台 Kubernetes 生产架构设计]]
- [[05-网络/03-服务网格/03-consul-connect-enterprise|Consul Connect 企业级服务网格管理]]
- [[08-安全/05-供应链/11-compliance-automation-audit|合规自动化与审计 (Compliance Automation and Audit)]]
- [[08-安全/05-供应链/09-policy-controller-verification|Policy Controller 镜像验证 (Policy Controller Image Verification)]]
- [[08-安全/06-合规审计/07-certificate-management|证书管理与 TLS 配置]]
- [[15-AI基础设施/02-AI-Agents/30-agent-harness-engineering|Agent Harness 工程：从模型包装到生产级 Agent 系统设计 (AI基础设施)]]
- [[15-AI基础设施/02-AI-Agents/47-openclaw-tools-mechanism|OpenClaw TOOLS.md 机制深度解析 (AI基础设施)]]
- [[15-AI基础设施/02-AI-Agents/37-agent-harness-multi-agent|Agent Harness 多 Agent 编排 (AI基础设施)]]
- [[15-AI基础设施/02-AI-Agents/44-openclaw-soul-mechanism|OpenClaw SOUL.md 机制深度解析 (AI基础设施)]]
- [[15-AI基础设施/02-AI-Agents/openclaw-workspace/SOUL|KuDig Doctor — 角色人格与绝对红线 (02-ai-agents)]]
- [[16-专项技术/02-WebAssembly/04-wasmcloud-platform|wasmCloud 平台]]
- [[17-系统基础/01-Linux/15-linux-commands-reference|Linux 命令大全参考]]
- [[17-系统基础/04-K8s事件/11-storage-volume-events|11 - 存储与卷事件]]
- [[17-系统基础/06-知识字典/fundamentals/recommended-labels|推荐标签]]
- [[17-系统基础/06-知识字典/platform-engineering/infrastructure-as-code-for-kubernetes|Kubernetes 基础设施即代码（IaC）]]
- [[19-故障诊断/01-核心排障/00-open-source-projects-index-from-domain-12|Domain-12 故障排查 — 开源项目索引]]
- [[19-故障诊断/04-高级排障/07-event-driven-architecture-troubleshooting|事件驱动架构故障排查]]
- [[23-实体/08-交付与制品/porter|Porter (entities)]]
- [[23-实体/15-参考与索引/k8s-advanced-ecosystem|硬件知识体系、CNCF 全景生态与 eBPF 平台工程]]
- [[23-实体/15-参考与索引/k8s-cluster-create|Kubernetes 集群创建操作指南]]
- [[23-实体/15-参考与索引/k8s-platform-extensions|平台运维与扩展生态：Helm、CI/CD、Operator 开发与服务网格]]
- [[26-技能/01-集群运维/gitops-argocd/诊断排障/ts-gitops-devops|GitOps/DevOps 排查]]
- [[26-技能/04-工作负载/pod/培训/learn-04-debug-tools-setup|Day 4: 调试工具全家桶安装]]
- [[26-技能/04-工作负载/pod/培训/beginner-guides/03-end-to-end-project|端到端项目实战——从代码到生产完整流水线]]
- [[26-技能/08-可观测性/monitoring/最佳实践/k8s-monitoring-guide|Kubernetes 监控最佳实践]]
- [[26-技能/08-可观测性/monitoring/最佳实践/k8s-distributed-tracing-guide|Kubernetes 分布式追踪最佳实践]]
