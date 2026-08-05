---
title: Domain-39 供应链安全 — 开源项目索引
description: '# Domain-39 供应链安全 — 开源项目索引'
summary: '# Domain-39 供应链安全 — 开源项目索引'
category: supply-chain-security
tags:
- k8s
- supply-chain
- security
- sbom
- slsa
tier: supporting
created: '2026-05-23'
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- 安全工程师
- SRE
- 架构师
estimated_read_time: 5min
intent_queries:
- Domain-39 供应链安全 — 开源项目索引 是什么
- 如何 Domain-39 供应链安全 — 开源项目索引
- Kubernetes 39 supply chain security 最佳实践
trigger_keywords:
- Domain-39
- 供应链安全
- 开源项目索引
- supply
- chain
- security
prerequisites:
- kubectl-basics
- rbac-basics
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




# Domain-39 供应链安全 — 开源项目索引

> **最后更新**: 2026-04-24

---

## 核心项目

| 项目 | 作用 | 归属 | 最新版本 | Stars | License |
|:---|:---|:---|:---|:---|:---|
| **Sigstore** | 软件签名生态 | OpenSSF | - | - | Apache-2.0 |
| **cosign** | 容器镜像签名 | OpenSSF | v2.4.0 | 4k+ | Apache-2.0 |
| **Fulcio** | 免费代码签名 CA | OpenSSF | v1.6.0 | 1k+ | Apache-2.0 |
| **Rekor** | 签名透明日志 | OpenSSF | v1.3.0 | 1.5k+ | Apache-2.0 |
| **gitsign** | Git 提交签名 | OpenSSF | v0.12.0 | 1k+ | Apache-2.0 |
| **in-toto** | 供应链完整性 | CNCF Graduated | v3.0.0 | 1k+ | Apache-2.0 |
| **TUF** | 安全更新框架 | CNCF Graduated | v4.0.0 | 3k+ | MIT |
| **Notary** | 镜像内容信任 | CNCF Incubating | v2.0.0 | 3k+ | Apache-2.0 |
| **Syft** | SBOM 生成 | Anchore | v1.22.0 | 6k+ | Apache-2.0 |
| **Grype** | 漏洞扫描 (Syft 配套) | Anchore | v0.87.0 | 8k+ | Apache-2.0 |
| **SPDX** | SBOM 标准格式 | Linux 基金会 | v2.3.0 | - | 标准 |
| **CycloneDX** | SBOM 标准格式 | OWASP | v1.6.0 | - | Apache-2.0 |
| **SLSA** | 供应链安全框架 | OpenSSF | v1.1.0 | - | 标准 |
| **Scorecard** | 开源项目安全评分 | OpenSSF | v5.0.0 | 4k+ | Apache-2.0 |
| **Allstar** | GitHub 安全策略自动化 | OpenSSF | v3.0.0 | 1k+ | Apache-2.0 |
| **GUAC** | 软件供应链知识图谱 | OpenSSF | v0.13.0 | 1k+ | Apache-2.0 |
| **Trivy** | 漏洞/SBOM/许可证扫描 | Aqua | v0.61.0 | 24k+ | Apache-2.0 |
| **Snyk** | 安全扫描 | Snyk | - | - | 商业 |
| **Checkmarx** | SAST/DAST/SCA | Checkmarx | - | - | 商业 |
| **FOSSA** | 许可证合规 | FOSSA | - | - | 商业 |
| **Sigstore policy-controller** | K8s 签名策略验证 | Sigstore | v0.11.0 | 1k+ | Apache-2.0 |
| **Tekton Chains** | CI/CD 供应链安全 | CDF | v0.24.0 | 500+ | Apache-2.0 |
| **SOPS** | YAML/JSON 加密 | Mozilla | v3.9.0 | 17k+ | MPL-2.0 |
| **Sigstore** | 软件签名生态 | OpenSSF | - | - | Apache-2.0 |
| **Fulcio** | 免费代码签名 CA | OpenSSF | v1.6.0 | 1k+ | Apache-2.0 |
| **Rekor** | 签名透明日志 | OpenSSF | v1.3.0 | 1.5k+ | Apache-2.0 |
| **gitsign** | Git 提交签名 | OpenSSF | v0.12.0 | 1k+ | Apache-2.0 |

---

## 参考链接

- [Sigstore 文档](https://docs.sigstore.dev/)
- [SLSA 规范](https://slsa.dev/)
- [OpenSSF](https://openssf.org/)
- [SPDX](https://spdx.dev/)
- [CycloneDX](https://cyclonedx.org/)

---

## Obsidian 相关文档

- 安全 MOC
- [[08-安全/README.md|Domain 05: 供应链安全 (Supply Chain Security)]]
- 供应链安全概述 (Supply Chain Security Overview)
- 供应链安全成熟度模型 (Supply Chain Security Maturity Model)
- SBOM 生成与管理 (SBOM Generation and Management)
- SBOM 漏洞分析与治理 (SBOM Vulnerability Analysis and Governance)
- SLSA 级别与实施 (SLSA Levels and Implementation)
- GitHub Actions SLSA 构建 (GitHub Actions SLSA Build)
- Sigstore 与 Cosign 签名 (Sigstore and Cosign Signing)
- Fulcio 与 Rekor 透明日志 (Fulcio and Rekor Transparency Logs)
- Policy Controller 镜像验证 (Policy Controller Image Verification...
- 合规自动化与审计 (Compliance Automation and Audit)

## See Also

- [[32-发布/package/2026-07-02_18-29/corpus/core/domain-05-security-compliance/02-incident-response/01-incident-response-process|20-incident-response-process]]
- [[37-归档/domain-indexes/security/00-open-source-projects-index-from-domain-25.md|00-open-source-projects-index-from-安全]]
- [[37-归档/domain-indexes/security/02-open-source-projects-index-from-domain-7.md|00-open-source-projects-index-from-安全]]
- [[37-归档/domain-indexes/security/MOC-from-domain-25.md|MOC-from-安全]]

- [[08-安全/README.md|返回目录]]

<!-- risk-assessed -->
