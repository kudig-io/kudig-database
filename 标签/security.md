---
title: security
description: 安全治理标签枢纽 — 涵盖 RBAC、NetworkPolicy、Pod Security、供应链安全、运行时安全、合规审计、零信任架构等全部安全领域知识
category: tag-index
tags:
- security
- rbac
- pod-security
- supply-chain
- runtime-security
- compliance
- zero-trust
tier: core
difficulty: intermediate-to-advanced
domain: security
k8s_versions: ["1.28", "1.30", "1.32", "1.34"]
created: '2026-07-11'
last_updated: '2026-07-21'
---

# security Tag Hub

> 安全领域页面 — RBAC、NetworkPolicy、Pod Security、供应链安全、运行时安全、合规审计等。

## 核心定义

**Kubernetes 安全**是保护容器化应用和基础设施免受威胁的系统化实践，覆盖身份认证、访问控制、网络隔离、运行时防护、供应链安全、合规审计等多个层面。它遵循纵深防御（Defense in Depth）原则，在每一层都建立安全屏障。

### 安全分层模型

| 层级 | 关注点 | 关键技术 |
|------|--------|----------|
| 云平台/基础设施 | 节点安全、网络隔离 | 安全组、VPC、节点加固 |
| 集群控制平面 | API 安全、etcd 加密 | TLS、审计日志、加密存储 |
| 身份与访问 | 认证、授权、密钥管理 | RBAC、OIDC、Vault |
| 网络隔离 | 微分段、加密通信 | NetworkPolicy、mTLS |
| Pod 安全 | 容器权限、沙箱 | Pod Security Standards、gVisor |
| 运行时防护 | 威胁检测、响应 | Falco、Tetragon |
| 供应链 | 镜像签名、漏洞扫描 | Sigstore、Trivy、SBOM |
| 合规审计 | 策略执行、审计报告 | Kyverno、OPA、CIS Benchmark |

### 4C 安全模型

```
Cloud (云平台) → Cluster (集群) → Container (容器) → Code (代码)
    │                  │                  │                │
    └── 基础设施安全 ──┴── 集群安全 ────┴── 容器安全 ──┴── 应用安全
```

## 生产实践要点

### 安全检查清单

| 检查项 | 状态 | 工具 |
|--------|------|------|
| RBAC 最小权限 | 必须 | `kubectl auth can-i --list` |
| Pod Security Standards | 必须 | restricted 级别 |
| NetworkPolicy 默认拒绝 | 必须 | default-deny-all |
| 镜像漏洞扫描 | 必须 | Trivy / Grype |
| Secret 加密存储 | 必须 | etcd encryption / Vault |
| 审计日志开启 | 必须 | --audit-log-path |
| 镜像签名验证 | 建议 | Cosign + admission |
| 运行时威胁检测 | 建议 | Falco / Tetragon |
| CIS Benchmark 合规 | 建议 | kube-bench |

### 常见安全反模式

| 反模式 | 风险 | 正确做法 |
|---------|------|----------|
| 使用 privileged 容器 | 完全控制节点 | 使用最小权限 + capabilities |
| 明文存储 Secret | 泄露敏感数据 | etcd 加密 + 外部密钥管理 |
| cluster-admin 滥用 | 权限过大 | 按角色分配最小权限 |
| 忽略镜像漏洞 | 已知漏洞被利用 | CI/CD 中集成扫描 |
| 无网络策略 | 横向移动无限制 | 默认拒绝 + 白名单 |
| 使用 hostNetwork | 绕过网络隔离 | 使用 Pod 网络 |

## 身份与访问 (Identity & Access)

- [[安全/身份与访问/01-authentication-authorization-system|认证授权体系]]
- [[安全/身份与访问/05-vault-enterprise-secrets-management|Vault 企业级密钥管理]]
- [[安全/身份与访问/07-rbac-matrix-configuration|RBAC 矩阵配置]]
- [[安全/身份与访问/11-secret-management-tools|密钥管理工具]]
- [[安全/身份与访问/99-vault-k8s-secrets-guide|Vault K8s 密钥指南]]

## 策略治理 (Policy Governance)

- [[安全/策略治理/04-kyverno-enterprise-policy-management|Kyverno 企业级策略管理]]
- [[安全/策略治理/05-policy-validation-tools|策略验证工具]]
- [[安全/策略治理/06-pod-security-standards|Pod 安全标准]]
- [[安全/策略治理/09-opa-gatekeeper-policy|OPA Gatekeeper 策略]]
- [[安全/策略治理/14-policy-engines-opa-kyverno|策略引擎 OPA/Kyverno]]
- [[安全/策略治理/99-kyverno-policy-guide|Kyverno 策略指南]]
- [[安全/策略治理/99-opa-gatekeeper-policy-guide|OPA Gatekeeper 策略指南]]

## 供应链安全 (Supply Chain Security)

- [[安全/供应链/01-supply-chain-security-overview|供应链安全概览]]
- [[安全/供应链/02-supply-chain-maturity-model|供应链成熟度模型]]
- [[安全/供应链/03-sbom-generation-management|SBOM 生成与管理]]
- [[安全/供应链/05-slsa-levels-implementation|SLSA 级别实施]]
- [[安全/供应链/07-sigstore-cosign-signing|Sigstore Cosign 签名]]
- [[安全/供应链/10-image-security-scanning|镜像安全扫描]]
- [[安全/供应链/14-supply-chain-security-runbook|供应链安全 Runbook]]
- [[安全/供应链/99-slsa-supply-chain-security-guide|SLSA 供应链安全指南]]

## 合规审计 (Compliance & Audit)

- [[安全/合规审计/04-audit-logging-compliance|审计日志合规]]
- [[安全/合规审计/08-cis-benchmark-compliance-audit|CIS Benchmark 合规审计]]
- [[安全/合规审计/08-security-best-practices|安全最佳实践]]
- [[安全/合规审计/09-security-hardening-production|生产环境安全加固]]
- [[安全/合规审计/11-kubernetes-security-hardening|Kubernetes 安全加固]]
- [[安全/合规审计/12-compliance-certification|合规认证]]
- [[安全/合规审计/99-cert-manager-tls-guide|cert-manager TLS 指南]]

## 网络安全 (Network Security)

- [[安全/网络安全/02-network-security-policies|网络安全策略]]
- [[安全/网络安全/07-zero-trust-security-architecture|零信任安全架构]]
- [[安全/网络安全/18-network-defense-depth|网络纵深防御]]
- [[安全/网络安全/19-zero-trust-architecture|零信任架构]]
- [[安全/网络安全/21-multicluster-security|多集群安全]]

## 运行时安全 (Runtime Security)

- [[安全/运行时安全/01-falco-cloud-native-security|Falco 云原生安全]]
- [[安全/运行时安全/02-sysdig-enterprise-container-security|Sysdig 企业级容器安全]]
- [[安全/运行时安全/03-runtime-security-defense|运行时安全防御]]
- [[安全/运行时安全/15-runtime-security-detection|运行时安全检测]]
- [[安全/运行时安全/17-gvisor-container-sandbox|gVisor 容器沙箱]]
- [[安全/运行时安全/99-falco-runtime-security-guide|Falco 运行时安全指南]]

## AI 安全 (AI Security)

- [[AI基础设施/基础设施/11-ai-security-model-protection|AI 安全与模型保护]]
- [[AI基础设施/基础设施/22-llm-privacy-security|LLM 隐私安全]]
- [[AI基础设施/基础设施/37-agent-sandbox-security|Agent 沙箱安全]]
- [[AI基础设施/AI-Agents/10-security-guardrails|AI Agent 安全护栏]]

## 概念 (Concepts)

- [[概念/k8s-security-compliance|K8s 安全合规]]
- [[概念/cloud-native-defense-in-depth|云原生纵深防御]]
- [[概念/secrets-management|密钥管理]]
- [[概念/security-defense-depth|安全纵深防御]]
- [[概念/supply-chain-security|供应链安全]]
- [[概念/multi-cluster-security|多集群安全]]
- [[概念/service-mesh-zero-trust-security|服务网格零信任安全]]

## 清单模式 (Manifest Patterns)

- [[清单模式/05-security-patterns/01-pod-security-standards-reference|Pod 安全标准参考]]
- [[清单模式/05-security-patterns/02-networkpolicy-default-deny|NetworkPolicy 默认拒绝]]
- [[清单模式/05-security-patterns/03-networkpolicy-tiered-isolation|NetworkPolicy 分层隔离]]
- [[清单模式/05-security-patterns/04-rbac-least-privilege|RBAC 最小权限]]
- [[清单模式/05-security-patterns/06-secret-external-management|Secret 外部管理]]
- [[清单模式/05-security-patterns/08-supply-chain-admission|供应链准入控制]]

## 故障诊断 (Troubleshooting)

- [[故障诊断/基础设施排障/32-security-troubleshooting|安全故障排查]]
- [[故障诊断/高级排障/structural-06-security-auth/03-pod-security-troubleshooting|Pod 安全排障]]
- [[故障诊断/技能体系/skill-set/k8s-rbac-quota/DIALOGUE|RBAC 权限问题对话]]
- [[故障诊断/技能体系/skill-set/k8s-security-incident/DIALOGUE|安全事件响应对话]]
- [[故障诊断/技能体系/skill-set/k8s-certificate-expiry/DIALOGUE|证书过期对话]]

## 研究 (Research)

- [[研究/zero-trust-k8s-security|零信任 K8s 安全]]
- [[研究/supply-chain-security|供应链安全]]
- [[研究/multi-tenancy-isolation|多租户隔离]]
- [[研究/policy-as-code-security|策略即代码安全]]

## 实体 (Entities)

- [[实体/falco|Falco]]
- [[实体/kyverno|Kyverno]]
- [[实体/opa|OPA]]
- [[实体/cert-manager|cert-manager]]
- [[实体/vault|Vault]]
- [[实体/trivy|Trivy]]
- [[实体/tetragon|Tetragon]]
- [[实体/k8s-security-compliance|Kubernetes Security Compliance]]

## 安全标签全景

### K8s 安全分层

| 层次 | 内容 |
|---|---|
| 集群 | API Server、etcd、RBAC |
| 节点 | kubelet、容器运行时 |
| 应用 | Pod Security、网络策略 |
| 数据 | Secret 加密、备份 |
| 供应链 | 镜像签名、漏洞扫描 |

### 安全工具链

| 类别 | 工具 |
|---|---|
| 策略 | OPA Gatekeeper, Kyverno |
| 运行时 | Falco, Sysdig |
| 扫描 | Trivy, Clair |
| 密钥 | Vault, Sealed Secrets |

## 面试要点

1. **Q：K8s 安全的核心原则？**
   A：最小权限、纵深防御、零信任、持续验证、安全左移。

2. **Q：Pod Security Standards 的级别？**
   A：Privileged(无限制)、Baseline(基本防护)、Restricted(严格限制)。

3. **Q：供应链安全如何保障？**
   A：镜像签名、漏洞扫描、SBOM、私有仓库、构建可重现。

## Related Tags

- [[标签/k8s|k8s]]
- [[标签/networking|networking]]
- [[标签/production|production]]
- [[标签/best-practices|best-practices]]
