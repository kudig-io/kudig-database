---
title: Consolidation Report 2026-05-23
summary: Auto-generated consolidation report from wiki-lint --consolidate run on 2026-05-23.
category: synthesis
tags:
- maintenance
- consolidation
tier: supporting
sources: []
created: '2026-05-23'
updated: '2026-05-23'
lifecycle: draft
lifecycle_changed: 2026-05-23
last_updated: 2026-05-23
relationships:
- target: '[[domain-17-system-foundation/知识字典/networking/ingress.md]]'
  type: uses
- target: '[[entities/istio.md]]'
  type: uses
- target: '[[domain-17-system-foundation/知识字典/networking/service.md]]'
  type: uses
- target: '[[entities/argo.md]]'
  type: related_to
---


# Consolidation Report — 2026-05-23

## Summary
- Broken links fixed: 3
- Cross-references added: 3 (orphan rescue)
- Lifecycle states updated: 0
- Tier demotions: 0
- Tags normalized: 0
- Contradiction callouts added: 0

## Broken Link Fixes
- `domain-12-cloud-providers/01-alibaba-cloud/01-专有云架构概述.md` — Nexus → `Nexus`
- `domain-12-cloud-providers/01-alibaba-cloud/01-专有云架构概述.md` — ELK → `ELK`
- `domain-12-cloud-providers/01-alibaba-cloud/README.md` — [[domain-17-system-foundation/知识字典/networking/ingress.md|ingress]]-gateway-failure.md]] → plain text

## Cross-References Added (orphan rescue)
- `synthesis/gitops-sre-release-gate.md` — now linked from: [[entities/argo.md|argo]]-cd-enterprise-gitops.md]], [[32-发布/package/2026-07-02_18-53/corpus/core/domain-09-reliability-engineering/04-sre-practices/01-release-gate-slo-based|02 release gate slo based]]
- `[[domain-17-system-foundation/知识字典/networking/service.md|service]]-mesh-zero-trust-security.md` — now linked from: [[entities/istio.md|istio]]-enterprise-service-mesh.md]], [[32-发布/package/2026-07-02_18-53/corpus/supporting/domain-05-security-compliance/02-network-security/02-zero-trust-security-architecture|07 zero trust security architecture]]
- `synthesis/finops-resource-governance.md` — already linked (previously fixed)

## Scope
本次 consolidate 聚焦新增内容范围（阿里云文档 + synthesis），未处理全 vault 历史遗留的 ~25,465 个 broken links（主要来自 domain-19-landscape-references 的 CNCF 文件）。

## 全 Vault Broken Links 清理（追加）

### Batch Fix — Round 1
- 扫描范围: 全 vault (5,550 个 Markdown 文件)
- 原始 broken links: 25,771 个
- 唯一目标: 5,016 个
- 模糊匹配修复: 558 个链接（155 个目标）
- 纯文本化: 26,748 个链接（4,861 个目标）
- 修改文件: 3,885 个

### Batch Fix — Round 2（清理残留）
- 剩余 broken links: 719 个
- 模糊匹配修复: 212 个链接
- 纯文本化: 507 个链接
- 修改文件: 461 个

### Final Cleanup
- 剩余: 3 个（截断的 wikilink target）
- 处理: 转为纯文本

### 最终结果
- **Broken links: 25,771 → 0 ✅**
- 修复方式: 模糊匹配重写 + 无匹配纯文本化
- 覆盖文件: ~4,300+ 个 Markdown
## Related

- [[domain-17-system-foundation/速查卡/git.md|Git 速查卡]]

## Cross-Linker — 知识图谱织密

### 链接添加统计
- 扫描页面: 124 个（新增内容范围）
- 候选链接: 61,779 个（去重后 587 个）
- 实际添加: 587 个链接
- 修改页面: 123 个
- 关系类型写入: 587 个（uses, related_to, extends）

### 链接类型分布
- 内联链接（Inline）: 大部分
- Related 章节: 部分（无法内联时）
- 关系类型: uses (实体/概念引用), related_to (默认), extends (扩展)

### 覆盖范围
- 阿里云文档: 6 篇全部增强
- 对话脚本: 17 篇全部增强
- 合成分析: 100+ 篇增强

