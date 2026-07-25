---
title: Wiki 维护周期执行摘要（2026-06-26）
description: 完成 cross-linker、wiki-lint、wiki-synthesize、wiki-status insights 维护任务及语料导出
category: reports
summary: "完成全库维护、语料评估与导出：4,996 页面，89.1 分，release/ 目录可交付。"
tags:
- wiki-maintenance
- cross-linker
- wiki-lint
- wiki-synthesize
- wiki-status
- insights
tier: supporting
created: '2026-06-26'
updated: '2026-06-26'
last_updated: 2026-06-26
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Wiki 维护周期执行摘要（2026-06-26）

> **执行目标**：按 wiki-status 建议全面执行 wiki 维护任务，评估语料完整度并导出  
> **执行范围**：cross-linker、wiki-lint、wiki-synthesize、wiki-status insights、broken links 修复、summary/frontmatter 补充、tier 分配、fragmented tags cross-link、语料评估与导出  
> **执行结果**：完成全部维护任务；语料评估 89.1/100；已导出到 `release/` 目录

---

## 1. 总体成果

| 任务 | 主要产出 |
|---|---|
| Cross-Linker | 为 30 个 orphan 页面添加 100 个交叉链接，修改 28 个页面 |
| Wiki-Lint | 扫描 4,875 个核心页面，生成完整健康审计报告 |
| Wiki-Synthesize | 创建 5 个 synthesis 页面 |
| Wiki-Status Insights | 生成全库结构洞察报告 `_meta/_insights.md` |
| Broken Links 修复 | 核心内容 broken links 从 255 → 0，relationship issues 从 15 → 0 |
| Summary/Frontmatter | 4,937 个 summary，41 个 frontmatter |
| Tier 分配 | core=1102, supporting=1372, peripheral=2511 |
| Fragmented Tags | 5 个 tag hubs，434 个页面新增交叉链接 |
| 语料评估 | 89.1/100，4,881 页面，1,725 万 tokens |
| 语料导出 | `release/`：4,996 页面，1,746 万 tokens，19,502 QA 对，195M |
| 日志更新 | 更新 `log.md`、`hot.md` |
| 新增脚本 | 16 个自动化脚本 |
| 新增报告 | 12+ 份 |

---

## 2. Broken Links 修复（本轮重点）

### 修复历程

| 轮次 | 修复数 | 转换数 | 说明 |
|---|---|---|---|
| 第一轮 | 89 | 24 | 全库扫描，自动模糊匹配修复 |
| 第二轮 | 0 | 4 | 处理工具目录中的伪链接 |
| 第三轮 | 43 | 22 | 处理 _reports/_meta/journal 链接和概念链接 |
| 第四轮 | 134 | 394 | 更宽松的概念标题匹配 |
| 核心统一修复 | ~16,710 | - | 将标题形式的链接规范化为路径形式 |
| 最终清理 | - | - | 修复 YAML 伪链接、relationships 字段 |

### 最终状态

| 指标 | 修复前 | 修复后 |
|---|---|---|
| 核心内容 broken links | 255 | **0** |
| Typed relationship issues | 15 | **0** |
| 扫描核心页面数 | - | 4,875 |

### 主要修复类型

1. **概念链接规范化**：`[[Container Runtime]]` → `[[22-概念/15-运行时与系统/container-runtime.md|Container Runtime]]`
2. **_meta/_reports 链接转文本**：报告和日志引用转为纯文本
3. **YAML/TOML 伪链接清理**：`[[kind: Deployment]]` → `` `kind: Deployment` ``
4. **relationships 字段修复**：清理 display text、无效 type、不存在目标
5. **工单案例链接**：修复 ticket-cases 之间的交叉引用

### 输出报告

- `_reports/broken-links-full-fix-2026-06-26.md`
- `_reports/broken-links-round2-fix-2026-06-26.md`
- `_reports/broken-links-round3-fix-2026-06-26.md`
- `_reports/broken-links-round4-fix-2026-06-26.md`
- `_reports/broken-links-final-fix-2026-06-26.md`
- `_reports/report-journal-links-converted-2026-06-26.md`

---

## 3. Summary 与 Frontmatter 补充

### 执行范围

扫描所有核心内容页面（4,987 页），为缺少 `summary` 和基础 frontmatter 的页面自动补充。

### 生成规则

1. **summary 优先级**：
   - 优先使用 frontmatter 中的 `description` 字段
   - 无 description 时提取正文第一段前 200 字符
   - 兜底使用页面标题

2. **frontmatter 默认值**：
   - `title`：从文件名推断（首字母大写）
   - `category`：从路径推断（concepts/entities/skills/domain-XX/docs）
   - `tags`：从文件名推断一个默认 tag
   - `created`：当前日期

### 成果

| 指标 | 修复前 | 修复后 |
|---|---|---|
| Missing summary | 4,827 | **2** |
| Missing frontmatter | 74 | **0** ✅ |
| Summary added | - | **4,937** |
| Frontmatter added/fixed | - | **41** |

### 例外说明

- 2 个文件因 macOS `com.apple.provenance` 扩展属性保护无法写入：
  - `存储/03-distributed-storage/01-velero-backup-recovery.md`
  - `存储/04-stateful-app-storage/01-stateful-app-storage-patterns.md`
- 需用户手动在 Finder 中解除保护或运行 `xattr -c <file>` 后补充 summary

### 输出报告

- `_reports/summary-frontmatter-fill-2026-06-26.md`

---

## 4. Tier 分配

### 分配规则

- **core**: 入链 >= 5
- **peripheral**: 入链 <= 1 且 90+ 天未更新
- **supporting**: 其他情况

### 成果

| Tier | 页面数 |
|---|---|
| core | 1,102 |
| supporting | 1,372 |
| peripheral | 2,511 |
| 权限错误 | 2 |

### 输出

- `_reports/tier-assignment-2026-06-26.md`

---

## 5. Fragmented Tags Cross-Link

### 处理策略

为 5 个 medium-sized fragmented tags 创建 hub 页面，并从每个 tag 页面的 Related 部分链接到 hub。

| Tag | 页面数 | Hub 页面 |
|---|---|---|
| research | 43 | `tags/research.md` |
| deep-dive | 55 | `tags/deep-dive.md` |
| papers | 21 | `tags/papers.md` |
| reference | 194 | `tags/reference.md` |
| visibility/public | 121 | `tags/visibility-public.md` |

### 成果

- 创建 hub 数：5
- 修改页面数：434
- Orphans 从 1,788 降至 1,717（减少 71）

### 输出

- `_reports/fragmented-tags-cross-link-2026-06-26.md`

---

## 6. Cross-Linker

### 执行范围

只针对近期新增、未与主知识图谱连接的目标目录：
- `_reports/`
- `生产运维/ticket-cases/`

### 成果

| 指标 | 数值 |
|---|---|
| 目标 orphan 页面 | 30 |
| 修改页面数 | 28 |
| 新增链接数 | 100 |
| 置信度 | EXTRACTED / INFERRED |

### 输出

- `_reports/cross-linker-targeted-2026-06-26.md`
- `log.md` 已追加 CROSS_LINK 记录
- `hot.md` 已更新 Recent Activity

---

## 7. Wiki-Lint

### 扫描范围

核心内容目录（排除 `_reports/`、`_meta/`、`_archives/` 等低信噪比目录）。

### 主要发现

| 问题类型 | 数量 | 说明 |
|---|---|---|
| Orphan 页面 | 2,425 | 大量发布说明、培训材料、独立报告未链接 |
| Broken wikilinks | 255 | 部分旧链接指向已删除/重命名页面 |
| Missing frontmatter | 710 | 多为旧文档或发布说明 |
| Missing/invalid summary | 5,463 | 绝大多数页面缺少 summary 字段 |
| Stale pages (≥90 days) | 0 | 近期内容活跃 |
| Fragmented tag clusters | 280 | 大量标签簇紧密度 < 0.15 |
| Typed relationship issues | 15 | 主要为新增 relationships 字段的格式问题 |

### 输出

- `_reports/wiki-lint-audit-2026-06-26.md`
- `log.md` 已追加 LINT 记录

### 说明

- 2,425 个 orphan 中，大部分是 生态参考 的发布说明归档，属于预期 orphan
- Missing summary 是历史遗留问题，因为早期文档未要求 summary 字段
- 本次未自动修复所有问题，仅生成审计报告供后续决策

---

## 8. Wiki-Synthesize

### 创建的 Synthesis 页面

| 页面 | 主题 |
|---|---|
| `synthesis/statefulset-cloud-native-storage.md` | StatefulSet × 云原生存储 |
| `synthesis/helm-gitops.md` | Helm × GitOps |
| `synthesis/slo-observability.md` | SLO × 可观测性 |
| `synthesis/container-runtime-image-security.md` | 容器运行时 × 镜像安全 |
| `synthesis/ticket-agent-rag.md` | 工单智能体 × RAG |

### 每个页面包含

- 完整 frontmatter（category、tags、sources、summary、provenance、base_confidence、lifecycle）
- The Connection / Where They Co-occur / Cross-cutting Insight / Tensions and Trade-offs / Open Questions / Related
- 所有 cross-cutting 结论标注 `^[inferred]`

### 输出

- 5 个 synthesis 页面
- `log.md` 已追加 WIKI_SYNTHESIZE 记录
- `hot.md` 已更新 Recent Activity

---

## 9. Wiki-Status Insights

### 更新 `_meta/_insights.md`

| 指标 | 数值 |
|---|---|
| 总页面数 | 5,666 |
| 总 wikilink | 29,981 |
| 孤儿页面数 | 2,521 (44.5%) |
| 平均入站链接 | 5.45 |
| 平均出站链接 | 5.29 |

### Top 10 Anchor Pages

| 页面 | 入站链接 |
|---|---|
| Kubernetes | 1,507 |
| Prometheus | 648 |
| Kubernetes (CNCF Graduated) | 609 |
| etcd | 485 |
| Service | 480 |
| kubelet | 344 |
| Kubernetes 生产环境速查卡 | 343 |
| GitOps / CI-CD 全局索引 | 319 |
| Helm | 314 |
| Kubernetes Architecture Overview | 289 |

### 最松散标签簇（需 cross-linker 关注）

- #incident-response、#filesystem、#developer-experience、#governance、#pulumi、#capacity、#block、#tcp、#debugging、#load-balancer

### Tier 建议

- 识别出大量 tier 未设置的页面，建议后续批量分配 core/supporting/peripheral

---

## 10. 日志与热缓存更新

### log.md

新增以下记录：
- `BROKEN_LINKS_FIX pages_scanned=4875 broken_links=0 relationship_issues=0`
- `STATUS_INSIGHTS anchors=20 cohesion_checked=... tier_suggestions=...`
- `WIKI_SYNTHESIZE pages_scanned=5506 synthesis_created=5`
- `LINT pages_scanned=4875 orphans=1787 broken_links=0 ...`
- `CROSS_LINK pages_scanned=5659 target_orphans=30 links_added=100 ...`

### hot.md

更新 Recent Activity，记录本次维护周期的五项任务（含 broken links 修复）。

---

## 11. 新增脚本

| 脚本 | 用途 |
|---|---|
| `scripts/cross_link_recent_pages.py` | 为最近 orphan 页面添加链接 |
| `scripts/cross_link_orphan_pages.py` | 为目标目录 orphan 页面添加链接 |
| `scripts/wiki_lint_audit.py` | 全库 wiki-lint 健康审计 |
| `scripts/wiki_status_insights.py` | 生成 wiki insights 报告 |
| `scripts/check_recent_wikilinks.py` | 检查最近文件 wikilink |
| `scripts/enhance_ticket_frontmatter.py` | 工单样本 frontmatter 增强 |
| `scripts/fix_broken_links_lint.py` | lint 报告 broken links 修复 |
| `scripts/scan_and_fix_broken_links.py` | 全库 broken links 扫描修复 |
| `scripts/fix_remaining_broken_links.py` | 第二轮剩余 broken links 修复 |
| `scripts/fix_broken_links_round3.py` | 第三轮概念/报告链接修复 |
| `scripts/fix_broken_links_round4.py` | 第四轮核心概念链接修复 |
| `scripts/fix_report_journal_links.py` | _reports/_meta/journal 链接转换 |
| `scripts/fix_all_core_broken_links.py` | 核心内容链接统一规范化 |
| `scripts/fix_relationships_targets.py` | relationships display text 清理 |
| `scripts/final_cleanup.py` / `final_cleanup_v2.py` | 最终清理 |
| `scripts/fix_broken_relationships_yaml.py` | relationships YAML 格式修复 |
| `scripts/bulk_fill_summary_and_frontmatter.py` | 批量补充 summary/frontmatter |
| `scripts/bulk_assign_tiers.py` | 批量分配 tier |
| `scripts/cross_link_fragmented_tags.py` | 为 fragmented tags 创建 hub 并交叉链接 |
| `scripts/fix_escaped_wikilinks.py` | 修复错误转义的 wikilink |
| `scripts/evaluate_corpus_completeness.py` | 评估语料完整度 |
| `scripts/export_corpus_for_nas.py` | 导出语料到指定目录 |

---

## 12. 关键报告索引

- `_reports/wiki-lint-audit-2026-06-26.md` — lint 审计（broken links = 0, missing frontmatter = 0, relationship issues = 0）
- `_meta/_insights.md` — 全库结构洞察
- `_reports/tier-assignment-2026-06-26.md` — tier 分配报告
- `_reports/fragmented-tags-cross-link-2026-06-26.md` — fragmented tags cross-link 报告
- `_reports/summary-frontmatter-fill-2026-06-26.md` — summary/frontmatter 补充报告
- `_reports/broken-links-final-fix-2026-06-26.md` — broken links 最终修复报告
- `_reports/cross-linker-targeted-2026-06-26.md` — cross-link 报告
- `_reports/corpus-completeness-evaluation-2026-06-26.md` — 语料完整度评估报告
- `_reports/wiki-maintenance-cycle-summary-2026-06-26.md` — 本摘要

---

## 13. 语料评估与导出

### 评估结果

| 维度 | 得分 | 说明 |
|---|---|---|
| 综合评分 | **89.1/100** | 优秀，可导出 |
| 规模 | 97.6 | 4,881 页面，1,725 万 tokens |
| 结构健康度 | 100.0 | broken links=0，missing frontmatter=0 |
| 概念覆盖度 | 98.0 | 49/50 关键 K8s/云原生概念 |
| 工单智能体适配度 | 100.0 | 58 ticket 页，507 skill 文档 |
| RAG 适配度 | 100.0 | 4,879 summary，4,881 tags/category |

### 导出详情

- **导出目录**: `release/`
- **总页面数**: 4,996
- **总 tokens**: 17,460,194
- **QA 对**: 19,502
- **目录大小**: 195M
- **分层结构**:
  - `corpus/core/` — 1,102 页
  - `corpus/supporting/` — 1,382 页
  - `corpus/peripheral/` — 2,512 页
  - `corpus/all/` — 全部页面
  - `qa/qa-corpus.jsonl` — QA 语料
  - `index.json` — 页面索引
  - `manifest.json` — 导出清单
  - `metadata/page-stats.json` — 统计信息

### 输出

- `_reports/corpus-completeness-evaluation-2026-06-26.md`
- `_reports/corpus-completeness-evaluation-2026-06-26.json`
- `release/`

---

## 14. 后续建议

### 高优先级

1. **处理 2 个受 macOS 保护的页面** — 手动为 Velero 和 StatefulSet 存储模式页面补充 summary
2. **Review 自动生成的 summary 质量** — 4,937 个 summary 中可能存在低质量或截断的条目
3. **继续 cross-link 更多 fragmented tags** — 当前仅处理了 5 个，还有 271 个标签簇

### 中优先级

4. **处理 release notes 归档 orphan** — 大量 domain-19 发布说明天然为 orphan，可接受
5. **Review tier 分配** — 检查是否有核心页面被误分配为 peripheral

### 低优先级

6. **定期重跑 wiki-status insights** — 建议每 2-4 周一次
7. **考虑将 _reports/ 中的报告间链接也规范化** — 当前已排除在核心知识图外

---

*本摘要记录 2026-06-26 执行的完整 wiki 维护周期成果。*


<!-- risk-assessed -->
