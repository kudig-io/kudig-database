---
title: obsidian-wiki Skills 安装 + 报告评估 — 2026-05-24
summary: obsidian-wiki Skills 安装 + 报告评估 — 2026-05-24：报告声称 0 broken links，实际扫描发现 858
  个未解析的 wikilink 目标。分三类：
category: reports
tags:
- reports
- evaluation
- obsidian-wiki
- skills
- broken-links
- quality
tier: supporting
sources:
- Vault Scan 2026-05-24
- Ar9av/obsidian-wiki
created: 2026-05-24
updated: 2026-05-24
last_updated: 2026-05-24
status: reviewed
relationships:
- target: _reports/KUDIG-COMPREHENSIVE-FIX-REPORT-2026-05-24.md
  type: related_to
- target: _reports/wiki-lint-2026-05-24.md
  type: related_to
- target: _reports/wiki-dedup-audit-2026-05-24.md
  type: related_to
---



# obsidian-wiki Skills 安装 + 报告评估 — 2026-05-24

> 对 Ar9av/obsidian-wiki 框架的 skills 安装验证，以及对现有报告（_insights.md、KUDIG-COMPREHENSIVE-FIX-REPORT）的独立交叉验证。

---

## 一、Skills 安装状态

### 安装来源
- 仓库: https://github.com/Ar9av/obsidian-wiki (1.5k stars, 172 forks)
- 克隆路径: `/tmp/obsidian-wiki`（depth=1）
- 安装方式: 符号链接到 `~/.hermes/skills/`

### 安装结果
- ✅ 36 个 skills 已安装
- ✅ 旧坏链接（指向已删除的 `/tmp/obsidian-wiki-source/`）已清理
- ✅ `~/.obsidian-wiki/config` 已更新 `OBSIDIAN_WIKI_REPO` 路径

### Skills 清单（36 个）

| 类别 | Skills |
|------|--------|
| **核心 Wiki** | wiki-setup, wiki-ingest, wiki-status, wiki-query, wiki-lint, wiki-rebuild, wiki-update, wiki-export |
| **分析增强** | cross-linker, tag-taxonomy, graph-colorize, wiki-synthesize, wiki-digest, wiki-dashboard |
| **数据导入** | data-ingest, ingest-url, obsidian-wiki-ingest, wiki-capture, wiki-research |
| **历史挖掘** | claude-history-ingest, codex-history-ingest, copilot-history-ingest, hermes-history-ingest, openclaw-history-ingest, pi-history-ingest, wiki-history-ingest |
| **Agent 协作** | wiki-agent, memory-bridge, llm-wiki, wiki-context-pack |
| **运维工具** | daily-update, impl-validator, skill-creator, wiki-stage-commit, wiki-switch |

---

## 二、报告独立验证

### 验证方法
- 使用 Python 脚本独立扫描 vault 文件系统
- 对比 `_insights.md` 和 `KUDIG-COMPREHENSIVE-FIX-REPORT-2026-05-24.md` 的声明
- 使用 Obsidian 风格的 wikilink 解析（basename 匹配 + 路径匹配）

### 核心指标对比

| 指标 | 报告声称 | 实际验证 | 评估 |
|------|---------|---------|------|
| Markdown 文件总数 | 4,951–4,952 | 5,624 | ⚠️ 差异 +672 |
| 总 wikilink 引用 | 34,599–34,602 | 34,588 | ✅ 基本一致 |
| Broken links | 0 (100%) | 858 (97.52%) | ⚠️ 见下方分析 |
| 空文件 | 0 | 0 | ✅ 一致 |
| 空目录 | 0 | 2（.venv/include, .understand-anything/tmp） | ✅ 非内容目录 |
| Domain 索引覆盖 | 20/20 | 20/20 | ✅ 一致 |
| 缺失 frontmatter | 0 | 0（顶层文件） | ✅ 一致 |
| Top Hub | kubernetes (6,848 入站) | — | ⚠️ Hubs 列表有重复 |

### Broken Links 深度分析

报告声称 0 broken links，实际扫描发现 858 个未解析的 wikilink 目标。分三类：

#### 类 A：Obsidian 模糊匹配可解析（非真正 broken）
- 空格↔连字符：`[[aeraki-mesh]]` → `aeraki-mesh.md`
- 大小写差异：Obsidian 不区分大小写
- **数量**: ~218 个简单 basename 链接（大部分属于此类）
- **结论**: 在 Obsidian 中可正常导航，不是真正 broken

#### 类 B：路径前缀缺失（真正的 broken）
这些链接使用了旧的目录结构路径，实际文件已迁移到 domain-* 子目录下：

| 旧路径前缀 | 实际位置 | 数量 |
|-----------|---------|------|
| `01-cncf-landscape/...` | `domain-19-landscape-references/01-cncf-landscape/...` | 229 |
| `topic-dictionary/...` | `domain-17-system-foundation/topic-dictionary/...` | 205 |
| `topic-code-analysis/...` | `domain-07-platform-engineering/topic-code-analysis/...` | 88 |
| `topic-functions/...` | 待确认 | 80 |
| `02-ai-agents/...` | 待确认 | 7 |
| `02-ai-agents/...` | 待确认 | 7 |
| 其他零散 | — | ~24 |

- **结论**: Obsidian 的后缀路径匹配可能解析部分，但路径引用不精确
- **影响**: 在 Obsidian 中可能正常工作，但在非 Obsidian 环境（如 LLM 直接解析 md）中会断裂

#### 类 C：代码片段误匹配
- `-t 0`、`'current_density', ...` 等 shell/Python 代码中的 `/`
- **数量**: 极少
- **结论**: 非 wikilink，代码中的条件表达式被误匹配

### Hubs 分析问题

`_insights.md` 的 Top 20 Anchor Pages 存在严重重复：
- `kubernetes` 出现 3 次（排名 1、2、3）
- `README` 出现 17 次（排名 4–20），入站链接数相同（3,364）但出站链接数不同
- **原因**: 图分析脚本可能对同一 basename 的多个文件（不同目录下的 README.md）分别计数
- **修正建议**: 应按唯一页面去重，或按目录限定

---

## 三、项目定位理解

本项目具有双重定位：

1. **LLM Wiki 语料库** — 供 AI Agent 在诊断/运维场景中检索和引用
   - wikilink 完整性直接影响 RAG/检索质量
   - frontmatter 的 tags/category 决定分类检索精度
   - 路径引用的准确性影响 Agent 的文件定位能力

2. **人类专家阅读的知识库** — 供 SRE/架构师在 Obsidian 中浏览
   - Obsidian 的模糊匹配掩盖了路径不一致问题
   - 但导出为 GitBook/静态站点时会暴露

3. **顾问模式语料库** — 无实际集群访问，基于知识推理
   - 知识的准确性和结构化程度是核心价值
   - 概念间关联（wikilink 网络）决定推理深度

---

## 四、执行计划 → 执行结果

| 优先级 | 任务 | 状态 | 结果 |
|--------|------|------|------|
| P0 | 修复路径前缀 broken links | ✅ 完成 | 632 个链接修复，14 个文件修改 |
| P0 | 更新 _insights.md 修正不准确的声明 | ✅ 完成 | 重写，Hubs 去重，数据准确 |
| P1 | 修复 Hubs 分析去重逻辑 | ✅ 完成 | Top 20 现为真实枢纽页面 |
| P1 | 修正文件总数差异 | ✅ 完成 | 5,616（原 4,951） |
| P2 | 代码片段误匹配清理 | ✅ 完成 | 通过过滤 `$`、`'`、`"` 字符排除 |

### 最终指标

| 指标 | 修复前 | 修复后 |
|------|--------|--------|
| Broken links（严格路径） | 632 | **0** |
| 模糊匹配链接 | 220 | 220（Obsidian 可解析） |
| 链接健康率 | 97.52% | **99.36%** |
| Hubs 列表准确性 | README 占 17/20 席 | 去重后为真实枢纽 |
| _insights.md 准确性 | 数据偏差 | 已修正 |

### 文件变更清单

| 文件 | 操作 |
|------|------|
| `_insights.md` | 重写（Hubs 去重 + 数据修正 + 修复记录） |
| `_reports/KUDIG-COMPREHENSIVE-FIX-REPORT-2026-05-24.md` | 追加路径修复章节 |
| `reports/obsidian-wiki-skills-evaluation-2026-05-24.md` | 新建（本评估报告） |
| 14 个 md 文件 | wikilink 路径前缀修复 |

---

> 报告生成时间: 2026-05-24 12:55
> 验证方法: Python 独立扫描 + Obsidian wikilink 解析逻辑
> 下一步: 执行修复

## Related

- _reports/KUDIG-COMPREHENSIVE-FIX-REPORT-2026-05-24.md
- _reports/wiki-lint-2026-05-24.md
- _reports/wiki-dedup-audit-2026-05-24.md
