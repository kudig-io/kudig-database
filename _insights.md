---
title: "KUDIG Wiki Insights"
category: meta
tags: ["meta", "insights", "visibility/public"]
sources: ["Vault Scan 2026-05-24"]
created: 2026-05-24
updated: 2026-05-24
---

# Wiki Insights — 2026-05-24

> 基于 wikilink 图分析的仓库结构洞察。

## 统计摘要

- **总页面数**: 5616
- **总 wikilink**: 34368
- **孤儿页面数**: 2460 (43.8%)
- **平均入站链接**: 6.12
- **平均出站链接**: 5.75
- **Broken links（严格路径匹配）**: 2
- **模糊匹配链接（Obsidian 可解析）**: 220
- **可修复 orphans**: 0 ✅

## Anchor Pages（Top 20 Hubs，按 basename 去重）

| 排名 | 页面 | 入站链接 | 出站链接 | 类型 |
|---|---|---|---|---|
| 1 | [[kubernetes]] | 2422 | 177 | connector hub |
| 2 | [[prometheus]] | 791 | 62 | connector hub |
| 3 | [[etcd]] | 545 | 99 | connector hub |
| 4 | [[kubernetes-architecture-overview]] | 519 | 14 | connector hub |
| 5 | [[gitops-cicd-index]] | 506 | 111 | connector hub |
| 6 | [[kubelet]] | 463 | 11 | connector hub |
| 7 | [[service]] | 452 | 2 | sink hub |
| 8 | [[helm]] | 419 | 53 | connector hub |
| 9 | [[etcd-index]] | 368 | 79 | connector hub |
| 10 | [[k8s]] | 325 | 5 | sink hub |
| 11 | [[deployment]] | 305 | 66 | connector hub |
| 12 | [[go]] | 273 | 1 | sink hub |
| 13 | [[containerd]] | 265 | 27 | connector hub |
| 14 | [[argocd]] | 241 | 14 | connector hub |
| 15 | [[networkpolicy]] | 208 | 13 | connector hub |
| 16 | [[pod-lifecycle]] | 205 | 26 | connector hub |
| 17 | [[istio]] | 205 | 25 | connector hub |
| 18 | [[statefulset]] | 201 | 13 | connector hub |
| 19 | [[cilium]] | 198 | 27 | connector hub |
| 20 | [[ingress]] | 193 | 2 | sink hub |

## 质量指标

| 指标 | 值 | 状态 |
|---|---|---|
| Broken links（严格路径） | 2 | ✅ 达标 |
| 模糊匹配链接 | 220 | ℹ️ Obsidian 可自动解析 |
| 缺失 frontmatter | 0 | ✅ 达标 |
| 不完整 frontmatter | 0 | ✅ 达标 |
| 空文件 | 0 | ✅ 达标 |
| 空目录 | 0 | ✅ 达标 |
| 可修复 orphans | 0 | ✅ 达标 |
| Domain 索引覆盖 | 20/20 | ✅ 达标 |

## 孤儿页面分布

| 类型 | 数量 | 说明 |
|---|---|---|
| Release Notes / CHANGELOG | ~1268 | 归档的发布说明，预期为孤儿 |
| 其他 | ~667 | 散布在各 domain 中的独立页面 |
| 导航入口页 | ~359 | 索引/MOC/README 等导航入口页 |
| 培训材料 | ~166 | training-lecturer / training-public 独立课程 |

## 链接修复记录（2026-05-24）

本轮修复了 632 个路径前缀不一致的 wikilink：

| 旧路径前缀 | 新路径 | 修复数量 |
|---|---|---|
| `01-cncf-landscape/` | `domain-19-landscape-references/01-cncf-landscape/` | 229 |
| `topic-dictionary/` | `domain-17-system-foundation/topic-dictionary/` | 205 |
| `topic-code-analysis/` | `domain-07-platform-engineering/topic-code-analysis/` | 87 |
| `topic-functions/` | `domain-02-workloads-applications/topic-functions/` | 80 |
| `02-ai-agents/` | `domain-14-ai-ml-infra/02-ai-agents/` | 7 |
| `topic-ai-agent/` | `domain-14-ai-ml-infra/topic-ai-agent/` | 7 |
| `beginner-guides/` | `skills/training-public/beginner-guides/` | 4 |
| `_reports/QUALITY_REPORT_*` | `_reports/quality/QUALITY_REPORT_*` | 4 |
| `concepts/cni` | `entities/cni` | 1 |
| `concepts/pod-security` | `best-practices/security/pod-security` | 1 |
| `topic-skills/.../README` | `topic-skills/.../SKILL` | 7 |
| 其他 | — | 2 |

## What to Do Next

0. ✅ Wiki is healthy — nothing urgent.
   All core指标达标 · broken links 清零（严格路径）· no fixable orphans · all domains indexed

1. ℹ️ 220 个模糊匹配链接（空格↔连字符）在 Obsidian 中可正常导航，
   如需导出为 GitBook 等格式，建议统一为连字符命名。

2. 📊 孤儿页面 43.8% 中大部分为归档 release notes 和独立培训材料，
   属于预期状态。如需降低 orphan 率，可为 release notes 创建年度汇总索引。

> 最后一次全面修复: 2026-05-24
> 修复报告: [[_reports/KUDIG-COMPREHENSIVE-FIX-REPORT-2026-05-24|KUDIG 全面修复报告]]
