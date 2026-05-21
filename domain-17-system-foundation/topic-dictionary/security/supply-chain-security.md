---
title: 软件供应链安全
description: '# 软件供应链安全'
category: dictionary
tags:
- k8s
- glossary
- terminology
- docker
- harbor
- opa
- falco
- operator
- ebpf
- argocd
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 软件供应链安全 是什么
- 如何 软件供应链安全
trigger_keywords:
- 软件供应链安全
- dictionary
prerequisites:
- kubectl-basics
- cloud-provider-basics
- gitops-basics
- ebpf-basics
- policy-basics
---

# 软件供应链安全

## 概述

软件供应链攻击（如 SolarWinds、Log4j 事件）已成为云原生环境的首要威胁。2026 年的 Kubernetes 安全最佳实践要求将**供应链安全**纳入整个应用生命周期，从镜像构建、签名、扫描到准入控制和运行时验证，形成端到端的可信交付链。核心能力包括 **SBOM（软件物料清单）、镜像签名（Sigstore/Cosign）、漏洞扫描和 SLSA 合规**。

## 核心概念/原理

### 1. 软件供应链攻击面

Kubernetes 软件供应链包含多个潜在攻击环节：
- **源代码**：依赖库被植入恶意代码
- **构建系统**：CI/CD Pipeline 被篡改，生成带后门的镜像
- **镜像仓库**：镜像被替换或覆盖
- **部署阶段**：未经验证的镜像被允许进入集群
- **运行时**：容器内存在已知 CVE 或被动态注入恶意进程

### 2. SBOM（Software Bill of Materials）

SBOM 是软件的"成分表"，详细列出镜像中包含的所有组件、库、许可证和版本信息：
- **生成工具**：Syft、Trivy、Docker SBOM
- **格式标准**：SPDX（Linux 基金会）、CycloneDX（OWASP）
- **价值**：在漏洞爆发时（如 Log4j），可快速定位受影响的镜像和集群

2026 年，SBOM 生成和验证正在成为容器交付的标准实践，Docker 也于 2025 年将 Hardened Images 的 SBOM 作为开源默认项。

### 3. 镜像签名与验证（Sigstore / Cosign）

**Sigstore** 是 CNCF 项目，提供无需维护私钥基础设施的免费镜像签名服务：
- **Cosign**：用于对容器镜像进行签名和验证的命令行工具
- **Rekor**：透明的签名日志，记录所有签名操作的不可篡改历史
- **Fulcio**：短期的 OIDC 身份证书颁发机构

使用 Cosign 签名后，可在 Kubernetes 集群中通过 **Kyverno** 或 **OPA Gatekeeper** 策略强制：
- 仅允许带有有效签名的镜像被部署
- 签名者身份必须来自受信任的 CI/CD 系统

### 4. SLSA（Supply-chain Levels for Software Artifacts）

SLSA 是由 Google 主导的开源框架，定义了软件供应链安全的四个级别：
| 级别 | 要求 | 说明 |
|------|------|------|
| **L1** | 生成 provenance 文档 | 记录构建来源 |
| **L2** | 使用版本控制和托管构建服务 | 防止手动篡改 |
| **L3** | 构建环境隔离、防篡改 | 高安全要求 |
| **L4** | 可复现构建、双人审查 | 最高安全级别 |

Docker Hardened Images 已达到 **SLSA Build Level 3**。

## 关键机制或特性

### 端到端供应链安全流水线

```
源代码提交
    ↓
[CI Pipeline] → 依赖扫描（Snyk/Trivy）
    ↓
[镜像构建] → 生成 SBOM + 漏洞扫描
    ↓
[镜像签名] → Cosign 签名 + Rekor 日志记录
    ↓
[推送镜像仓库] → Harbor / ECR / ACR
    ↓
[准入控制] → Kyverno/OPA 验证签名和 SBOM
    ↓
[Kubernetes 集群] → 运行时扫描（Trivy Operator / Falco）
```

### 镜像扫描集成

- **Trivy**：轻量级漏洞扫描器，支持镜像、文件系统、Git 仓库
- **Snyk**：企业级漏洞数据库和修复建议
- **Grype**：Anchore 出品的开源漏洞扫描器
- **Harbor**：内置 Trivy 扫描，可配置"仅允许无 Critical 漏洞的镜像"

### 运行时保护

即使镜像通过了准入控制，运行时仍需持续监控：
- **Falco**：基于 eBPF 检测异常进程启动、敏感文件访问
- **Trivy Operator**：持续扫描集群中运行的镜像，发现新 CVE 时告警

## 使用场景

1. **金融/医疗合规部署**：必须通过 SBOM 和镜像签名验证，满足监管机构对软件溯源的要求
2. **开源依赖风险管理**：在 CI 阶段扫描依赖库的已知漏洞，阻断存在 Critical CVE 的构建
3. **零信任镜像准入**：集群禁止部署任何未经过内部 CI 系统签名的镜像，防止供应链投毒
4. **漏洞应急响应**：新 CVE 公布后，通过 SBOM 快速定位所有运行受影响版本的集群和命名空间

## 最佳实践/注意事项

- **将 SBOM 生成纳入 CI 强制步骤**：每次构建都必须生成 SPDX 或 CycloneDX 格式的 SBOM，并随镜像 artifact 存储
- **使用 Cosign 密钥less 签名**：结合 OIDC（如 GitHub Actions/GitLab CI）实现无需长期维护私钥的签名流程
- **镜像只读策略**：运行中的容器应禁止写入根文件系统（`readOnlyRootFilesystem: true`），防止运行时注入
- **最小权限基础镜像**：优先使用 Distroless、Alpine 或 Docker Hardened Images，减少攻击面
- **定期轮换镜像和重建**：即使应用代码未变更，也应定期重建镜像以获取最新的 OS 安全补丁
- **准入控制双保险**：同时使用镜像签名验证 + 漏洞扫描结果验证，缺一不可
- **追踪 Provenance**：记录每个镜像的构建来源（Git commit、CI Pipeline ID），支持安全审计和溯源

## 参考链接

- [Sigstore / Cosign Documentation](https://docs.sigstore.dev/)
- [SLSA Framework](https://slsa.dev/)
- [Trivy Documentation](https://aquasecurity.github.io/trivy/)
- [Docker Hardened Images](https://www.docker.com/blog/docker-hardened-images/)
- [Ajeet Singh Raina - Top 5 Trends Shaping Kubernetes in 2026](https://www.ajeetraina.com/top-5-trends-shaping-kubernetes-in-2026/)

## Related

- [[synthesis/纵深防御 x 供应链安全|纵深防御 x 供应链安全]] — Cross-reference
- [[concepts/cloud-native-defense-in-depth|Cloud Native Defense in Depth]] — Cross-reference
- [[concepts/KUDIG Knowledge Base Architecture|KUDIG Knowledge Base Architecture]] — Cross-reference
- [[entities/argocd|ArgoCD]] — Cross-reference
- [[entities/trivy|Trivy]] — Cross-reference
- [[entities/cncf-security|CNCF 安全与合规项目全景]] — Cross-reference
- [[domain-19-landscape-references/topic-index/security-index|Security 安全知识图谱索引]]
- [[domain-19-landscape-references/topic-index/gitops-cicd-index|GitOps / CI-CD 全局索引]]
