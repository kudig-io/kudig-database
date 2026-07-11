---
title: OSCAL Compass (entities)
description: '## 概述'
summary: 'OSCAL Compass 是一套基于 NIST OSCAL (Open Security Controls Assessment Language) 标准的合规自动化工具集。'
category: entities
tags:
- k8s
- cncf
- security
- oscal-compass
- crd
- operator
tier: core
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- OSCAL Compass 是什么
- 如何 OSCAL Compass
trigger_keywords:
- OSCAL
- Compass
prerequisites:
- kubectl-basics
- policy-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# OSCAL Compass

> **CNCF 状态**: Sandbox | **类别**: Security | **主要语言**: Python

## 概述

OSCAL Compass 是一套基于 NIST OSCAL (Open Security Controls Assessment Language) 标准的合规自动化工具集，由 Red Hat 推动开发，2024 年加入 CNCF 沙箱。它包括 Trestle（合规即代码框架）、C2P（Compliance to Policy，合规到策略转换）等组件，帮助组织将安全合规要求转换为可执行的代码和策略。OSCAL Compass 实现了从合规框架（如 FedRAMP、SOC 2、ISO 27001、NIST 800-53）到实际控制实施（如 Kyverno 策略、OPA 规则）的自动化转换闭环，让合规审计从手工文档操作升级为可编程、可验证的自动化流程。

## 核心能力

- **OSCAL 标准**: 完全兼容 NIST OSCAL JSON/XML 格式（Catalog、Profile、Component、SSP、AP、AR）
- **合规即代码**: 将合规文档（如 FedRAMP SSP）转化为 Git 管理的代码资产
- **C2P 转换**: 自动将 OSCAL 合规控制映射到 Kubernetes 策略（Kyverno、OPA）
- **Trestle 框架**: Python 工具链，提供 OSCAL 文档的创建、编辑、验证和转换能力
- **持续监控**: 通过 OSCAL Assessment Results 持续收集和记录合规证据
- **多框架支持**: FedRAMP、SOC 2、ISO 27001、NIST 800-53、CIS Benchmark

## 架构

OSCAL Compass 围绕 NIST OSCAL 数据模型构建：

- **OSCAL Catalog**: 安全控制目录（如 NIST 800-53 的 1000+ 控制项定义）
- **OSCAL Profile**: 从 Catalog 中选取适用的控制子集（基线）
- **OSCAL Component Definition**: 组件的控制实施声明（如 Kyverno 策略如何满足某控制）
- **OSCAL SSP**: System Security Plan，描述系统如何满足合规要求
- **Trestle CLI**: 操作 OSCAL 文档的命令行工具，支持 assemble/validate/split
- **C2P Engine**: 将 Component Definition 中的实施声明转换为 Kubernetes 策略 CRD

合规流程：`OSCAL Catalog → Profile → Component Definition → C2P → Kyverno/OPA 策略 → 集群执行`

## K8s 集成

OSCAL Compass 的 C2P 组件将 OSCAL 合规定义转换为 Kubernetes 策略资源。例如，将 NIST 800-53 的 "AC-2 账户管理" 控制映射为 Kyverno ClusterPolicy，要求所有 Pod 必须设置特定的 SecurityContext。C2P 以 Operator 或 CLI 方式运行，读取 OSCAL 格式的合规定义，生成 Kyverno 或 OPA 策略 CRD 并应用到集群。结合 OSCAL Assessment Results，可以持续验证策略执行状态并生成合规报告。与 [[概念/kubernetes-architecture-overview.md|Kubernetes 架构]] 中的准入控制（Admission Webhook）机制配合，实现策略的强制执行。

## 生产场景

1. **FedRAMP 合规**: 为政府云服务自动化生成和验证 FedRAMP SSP 文档
2. **多框架合规**: 同时满足 NIST 800-53、SOC 2、ISO 27001 要求，避免重复工作
3. **策略即代码**: 将安全控制自动转化为 Kyverno/OPA 策略，在集群中强制执行
4. **持续合规审计**: 定期运行评估，生成 OSCAL Assessment Results 供审计使用

## 安装

```bash
# 安装 Trestle CLI
pip install trestle

# 初始化 Trestle 项目
trestle init -v

# 导入 NIST 800-53 Catalog
trestle import -f nist_800-53_catalog.json -o nist80053

# 创建合规 Profile
trestle profile create -n fedramp-low -c nist80053

# 安装 C2P (Compliance to Policy)
pip install c2p
# 将 OSCAL 定义转换为 Kyverno 策略
c2p convert --input component-definition.json --engine kyverno --output policies/
```

## 对比

| 特性 | OSCAL Compass | Compliance-as-Code | Chef InSpec | OpenSCAP |
|------|--------------|-------------------|-------------|----------|
| OSCAL 标准 | ✅ 原生 | ❌ | ❌ | ⚠️ 部分 |
| K8s 策略生成 | ✅ Kyverno/OPA | ⚠️ 需手动 | ❌ | ❌ |
| 合规框架 | 多框架 | 单一 | 多框架 | 多框架 |
| 持续监控 | ✅ | ⚠️ | ✅ | ✅ |

## 架构定位

在 CNCF 生态中，OSCAL Compass 属于 **Security** 类别，为云原生应用提供合规自动化和策略转换能力。

## 参考链接

- [[kyverno]]
- [[概念/gitops-principles.md|gitops-principles]]
- [[概念/security-defense-depth.md|security-defense-depth]]
- [[概念/ci-cd-pipeline-patterns.md|ci-cd-pipeline-patterns]]

## Related

- [[urunc]] — urunc
- [[connect-rpc]] — Connect RPC
- [[antrea]] — Antrea
- [[linkerd]] — Linkerd
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- oscal-compass
- [[实体/openfga.md|[[OpenFGA|OpenFGA]]]]
- [[实体/cncf-security.md|CNCF 安全与合规项目全景]] — Cross-reference
- [[生态参考/领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
