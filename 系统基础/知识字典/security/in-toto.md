---
title: in-toto 供应链安全
description: in-toto 是 CNCF 孵化项目，为软件供应链提供端到端的完整性验证框架，通过记录供应链中每个步骤的元数据（layout + link），确保软件制品在构...
summary: in-toto 是 CNCF 孵化项目，为软件供应链提供端到端的完整性验证框架，通过记录供应链中每个步骤的元数据（layout + link），确保软件制品在构...
category: dictionary
tags:
- k8s
- glossary
- security
- supply-chain
- verification
tier: core
created: 2026-06
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- in-toto 供应链安全 是什么
- in-toto 详解
trigger_keywords:
- in-toto 供应链安全
- in-toto
- dictionary
prerequisites:
- kubernetes
---



# in-toto 供应链安全（in-toto）

## 概述

in-toto 是 CNCF 孵化项目，为软件供应链提供端到端的完整性验证框架，通过记录供应链中每个步骤的元数据（layout + link），确保软件制品在构建和分发过程中未被篡改。

## 核心概念/原理

- **完整性框架**：定义供应链步骤（Steps）和检查（Inspections）的完整布局
- **元数据记录**：每个步骤的输入/输出哈希、命令、执行者签名
- **验证链**：从源代码到最终制品的端到端验证
- **CNCF 孵化**：与 TUF/Sigstore 构成供应链安全三件套

## 关键机制或特性

- Layout 定义：供应链步骤序列和验证规则
- Link 元数据：每个步骤的材料（materials）和产品（products）
- 函数签名验证（Functionary verification）
- 子布局（Sublayouts）支持嵌套供应链
- ITE-5/ITE-6 规范标准化
- `in-toto-run` / `in-toto-verify` CLI 工具

## 使用场景与最佳实践

- CI/CD Pipeline 的完整性验证
- 软件发布流程的审计追踪
- 第三方依赖的来源验证
- SLSA 合规的供应链证明
- 与 Sigstore/TUF 集成的综合安全方案

## 参考链接

- https://in-toto.io/
- https://github.com/in-toto/in-toto

## Related

- [[系统基础/知识字典/security/notary-project.md|Notary Project]]
- [[系统基础/知识字典/security/ratify.md|Ratify]]
- [[系统基础/知识字典/security/supply-chain-security.md|供应链安全]]
