---
title: "平台模式索引"
description: "08-platform-patterns 目录索引，汇总 Crossplane、CUE、Jsonnet/Tanka 等平台配置模式"
summary: "平台模式系列文章索引，涵盖基础设施即代码、配置语言、平台抽象等模式"
category: 清单模式
tags:
- platform-patterns
- index
- iac
- configuration
tier: supporting
created: '2026-07-19'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- SRE
- 平台工程师
estimated_read_time: 5min
intent_queries:
- "平台模式有哪些"
- "配置管理工具怎么选"
trigger_keywords:
- platform-patterns
- index
- configuration
prerequisites:
- kubectl-basics
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
authors:
- name: KUDIG Team
  role: contributor
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。

# 平台模式索引

本目录收录 Kubernetes 平台工程中的配置管理与基础设施即代码（IaC）模式。这些模式帮助平台团队构建可复用、可验证、可扩展的配置管理体系。

## 文章列表

| 编号 | 文章 | 主题 | 适用场景 |
|------|------|------|---------|
| 01 | [[清单模式/08-platform-patterns/01-crossplane-compositions-patterns\|Crossplane 组合模式]] | XRD/Composition 设计、Provider 配置、平台抽象 | 云资源自助服务、平台 API |
| 02 | [[清单模式/08-platform-patterns/02-cue-language-configuration\|CUE 语言配置]] | 类型安全配置、约束验证、模块化 | 配置标准化、合规检查 |
| 03 | [[清单模式/08-platform-patterns/03-jsonnet-tanka-patterns\|Jsonnet/Tanka 模式]] | 函数式配置、多环境管理、大型项目 | 复杂配置逻辑、多环境 |

## 选型指南

```
你的需求是什么？
│
├── 需要管理云资源（RDS/S3/VPC）？
│   └── → Crossplane（K8s 原生 IaC）
│
├── 需要强类型配置验证？
│   └── → CUE（编译时类型检查）
│
├── 需要复杂条件逻辑和函数复用？
│   └── → Jsonnet/Tanka（函数式编程）
│
├── 需要简单的环境差异化？
│   └── → Kustomize（补丁叠加）
│
└── 需要应用打包和分发？
    └── → Helm（Chart 生态）
```

## 相关主题

- [[平台工程/构建/01-platform-engineering-overview|平台工程概述]]
- [[平台工程/构建/07-crossplane-platform-composition|Crossplane 平台组合]]
- [[平台工程/构建/08-golden-paths-design|Golden Path 设计]]
- [[综合/helm-gitops|Helm GitOps]]
- [[综合/argocd-gitops|ArgoCD GitOps]]
- [[综合/crossplane-iac|Crossplane IaC]]

## Related

- [[清单模式/08-platform-patterns/01-crossplane-compositions-patterns|Crossplane 组合模式]]
- [[清单模式/08-platform-patterns/02-cue-language-configuration|CUE 语言配置]]
- [[清单模式/08-platform-patterns/03-jsonnet-tanka-patterns|Jsonnet/Tanka 模式]]
- [[平台工程/构建/01-platform-engineering-overview|平台工程概述]]
- [[综合/helm-gitops|Helm GitOps 综合]]
