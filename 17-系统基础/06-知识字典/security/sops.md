---
title: SOPS（Secrets OPerationS）
description: SOPS 是 Mozilla 开发的加密文件编辑器，支持 YAML/JSON/ENV 等格式，使用 KMS、GCP KMS、Azure Key
  Vault、ag...
summary: SOPS 是 Mozilla 开发的加密文件编辑器，支持 YAML/JSON/ENV 等格式，使用 KMS、GCP KMS、Azure Key Vault、ag...
category: dictionary
tags:
- k8s
- glossary
- security
- secrets
- encryption
tier: core
created: 2026-06
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- SOPS（Secrets OPerationS） 是什么
- SOPS 详解
trigger_keywords:
- SOPS（Secrets OPerationS）
- SOPS
- dictionary
prerequisites:
- kubernetes
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# SOPS（Secrets OPerationS）（SOPS）

## 概述

SOPS 是 Mozilla 开发的加密文件编辑器，支持 YAML/JSON/ENV 等格式，使用 KMS、GCP KMS、Azure Key Vault、age 或 PGP 作为密钥后端，实现 GitOps 友好的密钥管理。

## 核心概念/原理

- **文件级加密**：对值（value）加密，保留键（key）和结构不变，便于 diff 和 review
- **多密钥后端**：同时支持 AWS KMS、GCP KMS、Azure Key Vault、age、PGP
- **审计与权限**：通过 .sops.yaml 配置加密规则（creation rules），按路径匹配密钥
- **GitOps 集成**：加密后的文件可安全提交到 Git，配合 External Secrets 或 Sealed Secrets 使用

## 关键机制或特性

- 支持加密/解密/原地编辑（in-place edit）操作
- `sops --encrypt --in-place secrets.yaml` 加密文件
- `sops --decrypt secrets.yaml` 解密到标准输出
- 支持 SOPS + age 轻量方案，无需云 KMS
- 与 External Secrets Operator 配合实现自动注入

## 使用场景与最佳实践

- GitOps 仓库中的 Secret/ConfigMap 加密存储
- CI/CD pipeline 中的敏感配置管理
- 多环境（dev/staging/prod）密钥分离
- 合规要求下的密钥轮转与审计

## 参考链接

- https://github.com/getsops/sops
- https://fluxcd.io/flux/guides/mozilla-sops/

## Related

- [[17-系统基础/06-知识字典/security/external-secrets.md|External Secrets]]
- [[17-系统基础/06-知识字典/security/vault.md|Vault]]
- [[17-系统基础/06-知识字典/security/opa.md|OPA]]


<!-- risk-assessed -->
