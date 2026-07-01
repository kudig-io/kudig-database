---
title: Trivy
description: Trivy 是 Aqua Security 开源的全方位安全扫描工具，现为 CNCF 孵化项目。它可以扫描容器镜像、文件系统、Git 仓库中的漏洞、错误配置和敏...
summary: Trivy 是 Aqua Security 开源的全方位安全扫描工具，现为 CNCF 孵化项目。它可以扫描容器镜像、文件系统、Git 仓库中的漏洞、错误配置和敏...
category: dictionary
tags:
- k8s
- glossary
- trivy
- security
- scanning
- cncf
tier: core
created: '2026-06-24'
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Trivy 是什么
- Trivy 详解
trigger_keywords:
- Trivy
- dictionary
prerequisites:
- kubectl-basics
---



# Trivy

> **英文名**: Trivy

## 概述

Trivy 是 Aqua Security 开源的全方位安全扫描工具，现为 CNCF 孵化项目。它可以扫描容器镜像、文件系统、Git 仓库中的漏洞、错误配置和敏感信息，是 Kubernetes 安全扫描的事实标准工具。

## 核心概念/原理

### 扫描能力

| 扫描目标 | 内容 |
|----------|------|
| Container Image | CVE 漏洞、OS 包、应用依赖 |
| Filesystem | IaC 错误配置、密钥泄露 |
| Git Repository | 代码中的安全问题和密钥 |
| Kubernetes Cluster | 集群配置错误和权限风险 |
| SBOM | 软件物料清单生成 |

### 支持的漏洞数据库

NVD、Alpine、Debian、Ubuntu、Red Hat、Amazon Linux、GitHub Advisory 等。

## 关键机制或特性

- **Trivy Operator**：Kubernetes 原生部署，自动扫描集群中的镜像和配置。
- **CI/CD 集成**：作为 GitHub Action、GitLab CI 步骤扫描镜像。
- **SBOM 生成**：输出 SPDX/CycloneDX 格式的软件物料清单。
- **Misconfiguration**：扫描 Terraform、Kubernetes YAML、Dockerfile。
- 支持 JSON/Table/SARIF 多种输出格式。

## 使用场景与最佳实践

- CI/CD 流水线中集成 `trivy image` 扫描构建的镜像。
- 部署 Trivy Operator 持续扫描集群中的运行镜像。
- 使用 `trivy config` 检查 Kubernetes YAML 的安全配置。
- 将 Trivy 结果集成到 GitHub Security Advisory。
- 定期生成 SBOM 满足合规要求。

## 参考链接

- [Trivy Official](https://aquasecurity.github.io/trivy/)

## Related

- [[domain-17-system-foundation/topic-dictionary/security/pod-security-policy.md|Pod Security Policy]]
- [[domain-17-system-foundation/topic-dictionary/security/security-context.md|Security Context]]
- [[domain-17-system-foundation/topic-dictionary/security/rbac.md|RBAC]]
- [[domain-17-system-foundation/topic-dictionary/security/certificate.md|Certificate]]
- [[domain-17-system-foundation/topic-dictionary/security/admission-controller.md|Admission Controller]]
