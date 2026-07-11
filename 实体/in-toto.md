---
title: in-toto (entities)
description: '## 概述'
summary: '该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[概念/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]'
category: entities
tags:
- k8s
- cncf
- supply-chain
- in-toto
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
- in-toto 是什么
- 如何 in-toto
trigger_keywords:
- in-toto
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# in-toto

> **CNCF 状态**: Graduated | **类别**: Supply Chain | **主要语言**: Python, Go

## 概述

in-toto 是一个 CNCF 毕业项目，由 NYU 安全研究团队开发，是软件供应链安全验证框架。它定义了一种标准化的方式来验证软件从源代码到发布制品的全链路完整性。in-toto 通过元数据（Layout）定义供应链中的每个步骤及其执行者，并通过链接元数据（Link）记录每一步的实际执行情况。只有当所有步骤都按预期执行且未被篡改时，制品才被认为是安全的。项目被 Datadog、Google、Apache 等采用。

## Key Features（核心能力）

- **Layout 规范**：通过 Layout 文件定义供应链中的每个步骤、执行者和预期产物
- **Link 元数据**：每个步骤生成 Link 文件记录执行的命令、产物材料（Materials）和产物（Products）
- **签名验证**：所有元数据通过 GPG 密钥签名，验证身份和完整性
- **步骤隔离**：每个步骤的执行者密钥独立，实现职责分离
- **子 Layout**：支持 Layout 嵌套，实现供应链层级化验证
- **与 SLSA 兼容**：为 SLSA 框架提供具体的实现方式

## 架构与工作原理

in-toto 工作流分为三个阶段：Layout 定义阶段——项目所有者定义供应链 Layout（步骤序列、执行者公钥、预期命令和产物）；执行阶段——每个步骤的执行者运行 in-toto-run 生成 Link 元数据（记录材料哈希、命令、产物哈希）；验证阶段——in-toto-verify 收集 Layout 和所有 Link 文件，验证每个步骤是否按预期执行，产物是否未被篡改。

## K8s 集成

in-toto 在 Kubernetes 供应链安全中与 Sigstore/Cosign 配合使用。CI/CD 流水线中，每个构建步骤生成 in-toto Link 文件（attestation），Cosign 将这些 attestation 附加到容器镜像。部署时，K8s Admission Controller（如 Policy Controller）验证镜像上的 in-toto attestation，确保构建过程符合预期 Layout。

## 生产用例

- **软件供应链验证**：验证从代码到发布制品的全链路完整性
- **SLSA 合规**：满足 SLSA Level 2-4 的构建来源验证要求
- **安全审计**：为每次构建提供可追溯的审计链
- **防篡改保护**：防止构建过程中的恶意代码注入

## 安装与快速开始

```bash
pip3 install in-toto
# 定义 Layout
in-toto-layout
# 执行步骤
in-toto-run --step-name build --command make
# 验证
in-toto-verify --layout root.layout --verification-keys owner.pub
```

## 对比替代方案

相比 TUF（保护仓库级完整性），in-toto 关注供应链过程（每一步的执行验证）。相比 SLSA 框架（指导性规范），in-toto 提供具体的验证工具和格式。

## Related

- [[opentofu]] — OpenTofu
- [[cartography]] — Cartography
- [[volcano]] — Volcano
- [[bpfman]] — bpfman
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- in-toto
- [[实体/cncf-security.md|[[CNCF 安全与合规项目全景|CNCF 安全与合规项目全景]]]] — Cross-reference
- index/gitops-cicd-index|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
