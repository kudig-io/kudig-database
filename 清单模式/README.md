---
title: Manifests & Patterns
description: 清单模式知识域 — YAML 参考、Kustomize/Helm/Operator 高级模式、GitOps/安全/AI/弹性/平台清单模式
summary: 清单模式知识域入口，涵盖 YAML 资源规范、Kustomize 覆盖策略、Helm Chart 工程化、Operator 开发模式、GitOps 声明式清单、安全与弹性模式

category: domain
tags:
- yaml
- manifests
- kustomize
- helm
- operator
- gitops
- patterns
tier: core
created: '2026-05-23'
last_updated: '2026-07-21'
difficulty: intermediate
audience:
- 所有工程师
- 架构师
estimated_read_time: 5min
---
# 清单模式 Manifests & Patterns

> YAML 参考、Kustomize 模式、Helm 值模式、Operator 模式与领域专用清单模式集。

## 二级子目录

| 子目录 | 内容 | 核心话题 |
|--------|------|----------|
| [[清单模式/YAML参考/README.md\|YAML参考/]] | 资源规范 | Pod/Deployment/Service YAML 字段详解 |
| [[清单模式/Kustomize模式/README.md\|Kustomize模式/]] | Kustomize | Base/Overlay、patchesStrategicMerge、Generator |
| [[清单模式/Helm值模式/README.md\|Helm值模式/]] | Helm | Chart 结构、values 覆盖、模板函数、依赖管理 |
| [[清单模式/Operator模式/README.md\|Operator模式/]] | Operator | CRD 设计、Reconcile 循环、Kubebuilder/Operator SDK |
| [[清单模式/04-gitops-patterns/README.md\|04-gitops-patterns/]] | GitOps 清单 | ArgoCD Application、Sync Policy、App-of-Apps |
| [[清单模式/05-security-patterns/README.md\|05-security-patterns/]] | 安全清单 | NetworkPolicy、PodSecurity、RBAC 模板 |
| [[清单模式/06-ai-ml-patterns/README.md\|06-ai-ml-patterns/]] | AI/ML 清单 | GPU 调度、训练 Job、推理服务模板 |
| [[清单模式/07-resilience-patterns/README.md\|07-resilience-patterns/]] | 弹性清单 | PDB、HPA/VPA、拓扑分布、优先级 |
| [[清单模式/08-platform-patterns/README.md\|08-platform-patterns/]] | 平台清单 | Namespace 治理、ResourceQuota、LimitRange |

## 跨域导航

- [[AI基础设施/README.md|AI基础设施]]
- [[专项技术/README.md|专项技术]]
- [[云厂商/README.md|云厂商]]
- [[发布变更/README.md|发布变更]]
- [[可观测性/README.md|可观测性]]
- [[可靠性/README.md|可靠性]]
- [[存储/README.md|存储]]
- [[安全/README.md|安全]]
- [[容器运行时/README.md|容器运行时]]
- [[工作负载/README.md|工作负载]]
- [[平台工程/README.md|平台工程]]
- [[应用模式/README.md|应用模式]]
- [[故障诊断/README.md|故障诊断]]
- [[数据库中间件/README.md|数据库中间件]]
- [[生产运维/README.md|生产运维]]
- [[生态参考/README.md|生态参考]]
- [[系统基础/README.md|系统基础]]
- [[网络/README.md|网络]]
- [[集群基础/README.md|集群基础]]
