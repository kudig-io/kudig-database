---
title: domain-05-security-compliance MOC [98-merged-indexes]
description: domain-05-security-compliance 知识域导航页，覆盖 12 篇文档
summary: domain-05-security-compliance 知识域导航页，覆盖 12 篇文档
category: moc
tags:
- k8s
- moc
- security
tier: supporting
created: '2026-05-23'
last_updated: '2026-05-21'
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- domain-05-security-compliance MOC 是什么
- 如何 domain-05-security-compliance MOC
- Kubernetes 05 security compliance 最佳实践
trigger_keywords:
- domain-05-security-compliance
- MOC
- security
- compliance
prerequisites:
- kubectl-basics
- rbac-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# domain-05-security-compliance MOC

> **MOC 版本**: 1.0
> **知识域**: domain-05-security-compliance
> **文档数量**: 12 篇
> **最后更新**: 2026-05-21
> **用途**: 本知识域的导航入口，汇总所有相关文档、关联领域、和场景入口

---

## 领域概述

供应链安全 — SBOM、签名、验证、镜像安全

### 知识域定位

| 维度 | 说明 |
|---|---|
| **知识域** | domain-05-security-compliance |
| **文档数量** | 12 篇 |
| **难度分布** | 入门 0 / 进阶 0 / 高级 0 / 专家 0 |

---

## 文档清单

| # | 文档 | 难度 | 标签 | 估计阅读时间 |
|---|---|---|---|---|
| 1 | [[domain-05-security-compliance/00-open-source-projects-index.md|Domain-39 供应链安全 — 开源项目索引]] |  | security, supply-chain |  |
| 2 | 供应链安全概述 (Supply Chain Security Overview) |  | security, supply-chain, deep-dive |  |
| 3 | 供应链安全成熟度模型 (Supply Chain Security Maturity Model) |  | security, supply-chain |  |
| 4 | SBOM 生成与管理 (SBOM Generation and Management) |  | security, supply-chain |  |
| 5 | SBOM 漏洞分析与治理 (SBOM Vulnerability Analysis and Governance) |  | security, supply-chain |  |
| 6 | SLSA 级别与实施 (SLSA Levels and Implementation) |  | security, supply-chain |  |
| 7 | GitHub Actions SLSA 构建 (GitHub Actions SLSA Build) |  | security, supply-chain |  |
| 8 | Sigstore 与 Cosign 签名 (Sigstore and Cosign Signing) |  | security, supply-chain |  |
| 9 | Fulcio 与 Rekor 透明日志 (Fulcio and Rekor Transparency Logs) |  | security, supply-chain |  |
| 10 | Policy Controller 镜像验证 (Policy Controller Image Verification) |  | security, supply-chain |  |
| 11 | 合规自动化与审计 (Compliance Automation and Audit) |  | security, supply-chain, compliance |  |
| 12 | SLSA 软件供应链安全实践指南 |  | security, supply-chain, guide |  |

---

## 知识图谱

```mermaid
graph TD
    subgraph domain-05-security-compliance
        A["Domain-39 供应链安全 — 开源项目索引"]
    B["供应链安全概述 (Supply Chain Security Overview)"]
    C["供应链安全成熟度模型 (Supply Chain Security Maturity Model)"]
    D["SBOM 生成与管理 (SBOM Generation and Management)"]
    E["SBOM 漏洞分析与治理 (SBOM Vulnerability Analysis and Governance)"]
    F["SLSA 级别与实施 (SLSA Levels and Implementation)"]
    end

    A --> B
    A --> C
    A --> D
    A --> E
    A --> F

    style A fill:#3b82f6,stroke:#1d4ed8,color:#fff
    style B fill:#22c55e,stroke:#166534,color:#fff
```

---

## 关联入口

| 入口 | 说明 |
|---|---|
| FTA 故障树 | domain-05-security-compliance 相关故障树分析 |
| Skills 技能 | domain-05-security-compliance 相关操作技能 |
| 深度研究入口 | 语料库索引与向量检索 |

---

## 统计信息

| 指标 | 数值 |
|---|---|
| 文档总数 | 12 |
| 覆盖 K8s 版本 | v1.25 - v1.32 |

---

*本文档由 scripts/generate-mocs.py 自动生成，最后更新 2026-05-21。*

## See Also

- [[domain-05-security-compliance/98-merged-indexes/00-open-source-projects-index-from-domain-7.md|00-open-source-projects-index-from-domain-05-security-compliance]]
- [[domain-05-security-compliance/98-merged-indexes/MOC-from-domain-25.md|MOC-from-domain-05-security-compliance]]
- [[domain-05-security-compliance/98-merged-indexes/MOC-from-domain-7.md|MOC-from-domain-05-security-compliance]]
- [[domain-05-security-compliance/98-merged-indexes/README-from-domain-25.md|README-from-domain-05-security-compliance]]

- [[domain-05-security-compliance/README.md|返回目录]]

<!-- risk-assessed -->
