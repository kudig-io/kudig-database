---
title: Security & Compliance Open Source Projects Index
description: '# 安全合规开源项目索引'
summary: '# 安全合规开源项目索引'
category: reference
tags:
- security
- compliance
- open-source
- index
- opa
- falco
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Security & Compliance Open Source Projects Index 是什么
- 如何 Security & Compliance Open Source Projects Index
- Kubernetes 05 security compliance 最佳实践
trigger_keywords:
- Security
- Compliance
- Open
- Source
- Projects
- Index
- security
- compliance
prerequisites:
- kubectl-basics
- rbac-basics
- tls-basics
- policy-basics
---



# 安全合规开源项目索引

> 本索引合并了原 `domain-7-security`、`domain-25-cloud-native-security`、`domain-39-supply-chain-security` 三个域的开源项目信息。

## 身份与访问

| 项目 | 类型 | 说明 | 文档位置 |
|------|------|------|----------|
| Vault | Secret 管理 | HashiCorp 企业级 Secret 管理 | `01-identity-access/05-vault-enterprise-secrets-management.md` |
| cert-manager | 证书管理 | K8s 自动 TLS 证书 | `06-compliance/99-cert-manager-tls-guide.md` |

## 运行时安全

| 项目 | 类型 | 说明 | 文档位置 |
|------|------|------|----------|
| [[Falco|Falco]] | 威胁检测 | 云原生运行时安全 | `03-runtime-security/01-falco-cloud-native-security.md` |
| Sysdig | 安全监控 | 容器与系统监控 | `03-runtime-security/02-sysdig-enterprise-container-security.md` |
| Aqua | 容器安全 | 企业级容器安全平台 | `03-runtime-security/03-aqua-enterprise-container-security.md` |
| gVisor | 容器沙箱 | 用户空间内核沙箱 | `03-runtime-security/17-gvisor-container-sandbox.md` |

## 策略治理

| 项目 | 类型 | 说明 | 文档位置 |
|------|------|------|----------|
| OPA / Gatekeeper | 策略引擎 | 通用策略执行 | `04-policy-governance/09-opa-gatekeeper-policy.md` |
| Kyverno | K8s 策略 | 原生 K8s 策略管理 | `04-policy-governance/04-kyverno-enterprise-policy-management.md` |

## 供应链安全

| 项目 | 类型 | 说明 | 文档位置 |
|------|------|------|----------|
| Sigstore | 签名验证 | 开源软件签名生态 | `05-supply-chain/07-sigstore-cosign-signing.md` |
| Cosign | 容器签名 | 容器镜像签名工具 | `05-supply-chain/07-sigstore-cosign-signing.md` |
| SLSA | 标准 | 软件供应链安全级别 | `05-supply-chain/05-slsa-levels-implementation.md` |
| Syft / Grype | SBOM/扫描 | 生成 SBOM 并扫描漏洞 | `05-supply-chain/03-sbom-generation-management.md` |

## 原始索引保留

更详细的索引见：
- `98-merged-indexes/00-open-source-projects-index-from-domain-7.md`
- `98-merged-indexes/00-open-source-projects-index-from-domain-25.md`
- `98-merged-indexes/00-open-source-projects-index-from-domain-39.md`

## Related

- [[domain-05-security-compliance/README.md|返回目录]]
- [[domain-19-landscape-references/topic-index/pvc-index.md|PVC 知识图谱索引]]
- [[domain-19-landscape-references/topic-index/etcd-index.md|etcd 知识图谱索引]]
- [[domain-19-landscape-references/topic-index/observability-index.md|Observability 可观测性知识图谱索引]]
- [[domain-19-landscape-references/topic-index/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]
