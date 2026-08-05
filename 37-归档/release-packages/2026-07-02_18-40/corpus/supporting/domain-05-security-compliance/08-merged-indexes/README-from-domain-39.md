---
title: 'Domain 39: 供应链安全 (Supply Chain Security)'
description: 'title: ''Domain 39: 供应链安全 (Supply Chain Security)'''
summary: 'title: ''Domain 39: 供应链安全 (Supply Chain Security)'''
category: general
tags:
- k8s
- opa
tier: supporting
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 'Domain 39: 供应链安全 (Supply Chain Security) 是什么'
- '如何 Domain 39: 供应链安全 (Supply Chain Security)'
- Kubernetes 05 security compliance 最佳实践
trigger_keywords:
- Domain
- '39:'
- 供应链安全
- Supply
- Chain
- Security
- security
- compliance
prerequisites:
- kubectl-basics
- rbac-basics
- policy-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




---
title: 'Domain 39: 供应链安全 (Supply Chain Security)'
description: '# Domain 39: 供应链安全 (Supply Chain Security)'
category: supply-chain-security
tags:
- k8s
- supply-chain
- security
- sbom
- slsa
- opa
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- 安全工程师
- SRE
- 架构师
estimated_read_time: 5min
intent_queries:
- 'Domain 39: 供应链安全 (Supply Chain Security) 是什么'
- '如何 Domain 39: 供应链安全 (Supply Chain Security)'
- Kubernetes 39 supply chain security 最佳实践
trigger_keywords:
- Domain
- '39:'
- 供应链安全
- Supply
- Chain
- Security
- supply
- chain

tier: peripheral---

# Domain 39: 供应链安全 (Supply Chain Security)

> **适用范围**: 软件供应链、SBOM、镜像签名、合规自动化 | **维护状态**: 持续更新中 | **专家级别**: ⭐⭐⭐⭐⭐ | **更新时间**: 2026-03-04

## 📋 领域概览

软件供应链安全 (Supply Chain Security) 已成为云原生安全的核心关注点。SolarWinds、Log4Shell 等事件凸显了供应链攻击的严重性。本领域深入探讨 SBOM (软件物料清单)、SLSA (供应链完整性级别)、Sigstore 签名验证等核心技术，帮助企业建立端到端的供应链安全体系。

## 📚 文档目录

### 🎯 供应链安全基础 (01-02)
- **[01-供应链安全概述](32-发布/package/2026-07-02_18-40/corpus/core/domain-05-security-compliance/01-supply-chain/01-supply-chain-security-overview.md)** - 威胁模型、攻击向量、防护体系
- **[02-供应链安全成熟度模型](32-发布/package/2026-07-02_18-40/corpus/supporting/domain-05-security-compliance/05-supply-chain/01-supply-chain-maturity-model.md)** - 成熟度评估、改进路径、合规映射

### 📋 SBOM 软件物料清单 (03-04)
- **[03-SBOM生成与管理](32-发布/package/2026-07-02_18-40/corpus/supporting/domain-05-security-compliance/05-supply-chain/02-sbom-generation-management.md)** - Syft、Trivy SBOM、SPDX/CycloneDX 格式
- **[04-SBOM漏洞分析与治理](32-发布/package/2026-07-02_18-40/corpus/supporting/domain-05-security-compliance/05-supply-chain/03-sbom-vulnerability-analysis.md)** - Grype、依赖分析、风险评估

### 🔐 SLSA 供应链完整性 (05-06)
- **[05-SLSA级别与实施](32-发布/package/2026-07-02_18-40/corpus/supporting/domain-05-security-compliance/05-supply-chain/04-slsa-levels-implementation.md)** - SLSA L1-L4、构建证明、来源验证
- **[06-GitHub Actions SLSA构建](32-发布/package/2026-07-02_18-40/corpus/supporting/domain-05-security-compliance/05-supply-chain/05-github-actions-slsa-build.md)** - SLSA Generator、Provenance、可复现构建

### ✍️ Sigstore 签名验证 (07-08)
- **[07-Sigstore与Cosign签名](32-发布/package/2026-07-02_18-40/corpus/supporting/domain-05-security-compliance/05-supply-chain/06-sigstore-cosign-signing.md)** - Keyless 签名、Cosign 工作流、OIDC 集成
- **[08-Fulcio与Rekor透明日志](32-发布/package/2026-07-02_18-40/corpus/supporting/domain-05-security-compliance/05-supply-chain/07-fulcio-rekor-transparency.md)** - 证书颁发、透明日志、审计追踪

### 🛡️ 策略与合规 (09-10)
- **[09-Policy Controller镜像验证](32-发布/package/2026-07-02_18-40/corpus/supporting/domain-05-security-compliance/05-supply-chain/08-policy-controller-verification.md)** - Kyverno/Gatekeeper 签名验证、准入控制
- **[10-合规自动化与审计](11-compliance-automation-audit.md)** - SOC 2、PCI-DSS、FedRAMP 合规自动化

## 🎯 学习路径建议

### 🔰 供应链安全入门
1. **01-供应链安全概述** → 理解威胁与防护
2. **03-SBOM生成** → 建立软件清单
3. **07-Sigstore签名** → 实施镜像签名

### ⭐ 安全工程师
1. **05-SLSA实施** → 提升供应链完整性
2. **04-漏洞分析** → 依赖风险管理
3. **09-策略验证** → 准入控制自动化

### 🏗️ 合规架构师
1. **02-成熟度模型** → 评估与规划
2. **08-透明日志** → 审计追踪体系
3. **10-合规自动化** → 自动化合规验证

## 📊 技术深度对比

| 文档 | 技术深度 | 实践价值 | 适用场景 | 复杂度 |
|------|----------|----------|----------|--------|
| 01-供应链概述 | ⭐⭐⭐⭐ | 很高 | 战略规划 | 中 |
| 02-成熟度模型 | ⭐⭐⭐⭐ | 高 | 评估改进 | 中 |
| 03-SBOM生成 | ⭐⭐⭐⭐⭐ | 很高 | 清单管理 | 中 |
| 04-漏洞分析 | ⭐⭐⭐⭐⭐ | 很高 | 风险管理 | 中高 |
| 05-SLSA实施 | ⭐⭐⭐⭐⭐ | 很高 | 完整性保障 | 高 |
| 06-GitHub SLSA | ⭐⭐⭐⭐ | 很高 | CI/CD 集成 | 中 |
| 07-Sigstore签名 | ⭐⭐⭐⭐⭐ | 很高 | 签名验证 | 中高 |
| 08-透明日志 | ⭐⭐⭐⭐ | 高 | 审计追踪 | 中高 |
| 09-策略验证 | ⭐⭐⭐⭐⭐ | 很高 | 准入控制 | 中高 |
| 10-合规自动化 | ⭐⭐⭐⭐⭐ | 很高 | 合规审计 | 高 |

## 🔧 核心技术栈

```bash
# SBOM 工具
Syft (Anchore)                  # SBOM 生成
Trivy (Aqua Security)           # 扫描与 SBOM
SPDX/CycloneDX                  # SBOM 格式标准

# 签名与验证
Sigstore                        # 签名生态系统
Cosign                          # 容器签名工具
Fulcio                          # 无密钥证书颁发
Rekor                           # 透明日志

# 构建完整性
SLSA Framework                  # 供应链完整性级别
in-toto                         # 软件证明框架
Tekton Chains                   # Tekton 签名集成

# 策略引擎
Kyverno (CNCF Graduated)        # Kubernetes 策略
OPA/Gatekeeper                  # 通用策略引擎
```

## 📚 相关领域链接

- **[Domain-19: 高级论文](../domain-19-papers)** - 供应链安全深度实践
- **[Domain-25: 云原生安全](../domain-05-security-compliance)** - 运行时安全
- **[Domain-23: GitOps CI/CD](../domain-08-release-change-management)** - CI/CD 安全集成

---
*本文档由云原生技术专家团队维护，内容基于 2026 年供应链安全最新实践。*

## See Also

- [[domain-05-security-compliance/98-merged-indexes/MOC-from-domain-7.md|MOC-from-domain-05-security-compliance]]
- [[domain-05-security-compliance/98-merged-indexes/README-from-domain-25.md|README-from-domain-05-security-compliance]]
- [[domain-05-security-compliance/98-merged-indexes/README-from-domain-7.md|README-from-domain-05-security-compliance]]
- [[domain-05-security-compliance/身份与访问/01-authentication-authorization-system.md|01-authentication-authorization-system]]

- [[domain-05-security-compliance/README.md|返回目录]]

<!-- risk-assessed -->
