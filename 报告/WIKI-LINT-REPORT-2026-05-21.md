---
title: Wiki Lint Report — 2026-05-21
description: '- `AGENTS` — Agent 配置文件，不需要入链'
summary: '- `AGENTS` — Agent 配置文件，不需要入链'
category: general
tags:
- k8s
- prometheus
- rag
- agent
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Wiki Lint Report — 2026-05-21 是什么
- 如何 Wiki Lint Report — 2026-05-21
trigger_keywords:
- Wiki
- Lint
- Report
- '2026-05-21'
prerequisites:
- kubectl-basics
- prometheus-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




---
title: Wiki Lint Report 2026-05-21
category: references
tags: [maintenance, lint, wiki-health]
sources: []
summary: Wiki health audit covering 4,090 pages across 49 directories. Found 6,436 issues: 2,659 orphans, 262 broken links, 3,041 missing frontmatter fields, 31 fragmented tag clusters.
lifecycle: reviewed
tier: peripheral
created: 2026-05-21T16:30:00Z
updated: 2026-05-21T16:30:00Z
---

# Wiki Lint Report — 2026-05-21

## 概览

- **Wiki 页面总数：** 4,090
- **已摄入源文件：** 3,523
- **上次摄入：** 2026-05-21（全量模式）
- **问题总数：** 6,436

---

## 检查结果汇总

| 检查项 | 问题数 | 严重度 | 备注 |
|---|---|---|---|
| 孤儿页面 | 2,659 | 🔴 高 | 65% 页面无人引用，主要是 topic-code-analysis 源文件 |
| 破损链接 | 262 → 已修复 ~1,200+ (含嵌套wikilink 940 + 其他 ~260) | 🟡 中 | fta-template、cheat-sheet-template、中文链接等 |
| 缺少前置元数据 | 3,041 | 🟡 中 | 大量来自非 wiki 文件 |
| 缺少摘要 | 3,031 | 🟢 低 | soft warning |
| 碎片化标签群组 | 31 | 🟡 中 | 所有标签群组 cohesion=0.00 |
| 可见性误报 | 407 | 🟢 低 | K8s 技术文档中的 token/secret 示例被误判 |
| 综合缺口 | 36 | 🟡 中 | 跨领域概念对缺少 synthesis 页面 |
| 内容过期 | 0 | ✅ | 今天刚完成大规模更新 |
| 生命周期/置信度 | 0 | ✅ | 全部合规 |
| 关系类型 | 0 | ✅ | 使用 with: 格式，合法 |
| 溯源问题 | 0 | ✅ | 无超出阈值标记 |

---

## 孤儿页面（2,659 个）

### 典型示例（前 10 个）

- `STRUCTURE` — 项目结构文件，不需要入链
- `AGENTS` — Agent 配置文件，不需要入链
- `平台工程/topic-code-analysis/node-create/11-eviction` — 源分析文件
- `平台工程/topic-code-analysis/node-create/08-troubleshooting` — 源分析文件
- `平台工程/topic-code-analysis/node-create/09-cni-node` — 源分析文件
- `平台工程/topic-code-analysis/node-create/06-certificate` — 源分析文件
- `平台工程/topic-code-analysis/node-create/13-security` — 源分析文件
- `平台工程/topic-code-analysis/node-create/07-autoscaling` — 源分析文件
- `平台工程/topic-code-analysis/node-create/12-monitoring` — 源分析文件
- `平台工程/topic-code-analysis/node-create/05-upgrade` — 源分析文件

### 分类

- **非 wiki 页面**（不需要修复）：`STRUCTURE`, `AGENTS`, `CHANGELOG`, `CONTRIBUTING`, `.codebuddy/` 等
- **平台工程/topic-code-analysis/ 源文件**（低优先级）：代码分析输出，应由 wiki-ingest 转化为 wiki 页面后自动解决
- **核心 wiki 孤儿**（需要修复）：少数 concepts/skills/entities 页面缺少入链

---

## 破损链接（262 个）

### 高优先级（核心 wiki 页面中的链接）

| 页面 | 行号 | 破损链接 | 建议修复 |
|---|---|---|---|
| CONTRIBUTING.md | 23 | `[[脚本/templates/fta-template.md|fta template]]` | 移除 .md 后缀 |
| CONTRIBUTING.md | 306 | `[[fta-template]]` | 指向 `[[脚本/templates/fta-template.md|fta template]]` |
| MOC.md | 40 | `[[平台工程/代码分析/MOC` | 修复嵌套 wikilink 语法 |
| AGENTS.md | 40,102,125 | `wikilinks` 等 | 这些是说明性文本，可转为代码格式 |
| 平台工程/topic-code-analysis/README.md | 15-19 | `[[entities/k8s-cluster-cert.md|k8s cluster cert]]` 等 5 个 | 应指向 references/ 下的分组页面 |
| 多个 README.md | 多处 | `[[脚本/templates/cheat-sheet-template.md|cheat sheet template]]` | 应指向 `[[脚本/templates/cheat-sheet-template.md|cheat sheet template]]` |
| _reports/OBSIDIAN-WIKI-AGENT... | 125,130,132,425 | 中文链接/相对路径 | 这些是分析报告，应转为纯文本 |

### 修复计划

1. CONTRIBUTING.md — 修复 fta-template 链接
2. MOC.md — 修复嵌套 wikilink 语法错误
3. 平台工程/topic-code-analysis/ 目录 — 批量修复 cheat-sheet-template 和分组页面链接
4. _reports/ 目录 — 将破损链接转为纯文本
5. AGENTS.md — 将 wikilinks 说明转为代码格式

---

## 缺少前置元数据（3,041 个）

### 分布

- **非 wiki 文件**（可忽略）：CHANGELOG.md, CONTRIBUTING.md, MOC.md, README.md 等
- **.codebuddy/ 缓存文件**（可忽略）：AI 工具生成的临时文件
- **平台工程/topic-code-analysis/ 源文件**（低优先级）：代码分析输出
- **_meta/ 元数据文件**（建议修复）：wiki 自身的元数据配置
- **_reports/ 报告文件**（建议修复）：本报告等

### 建议

核心 wiki 页面（concepts/, entities/, skills/, domain-*/）的元数据完整性较高，缺失主要来自非标准页面。

---

## 碎片化标签群组（31 个）

所有 ≥5 页面的标签群组 cohesion 均为 0.00。

| 标签 | 页面数 | cohesion | 建议 |
|---|---|---|---|
| #k8s | 407 | 0.00 | 运行 /cross-linker |
| #moc | 44 | 0.00 | 运行 /cross-linker |
| #networking | 29 | 0.00 | 运行 /cross-linker |
| #cloud | 25 | 0.00 | 运行 /cross-linker |
| #platform | 15 | 0.00 | 运行 /cross-linker |

### 根因

标签群组内聚度为 0 是因为 wiki 的链接模式是通过 cross-linker 批量添加的，页面间的手动 wikilink 较少。需要依赖 cross-linker 技能来增加群组内链接。

---

## 可见性误报（407 个）

页面中包含 password/token/secret 等模式但未标记 `visibility/pii`。

**分析：** 这些 407 个页面全部位于 `平台工程/topic-code-analysis/` 目录下，内容是 Kubernetes 技术文档中的示例命令和配置片段（如 `kubeadm join --token xxx`），不是真正的 PII 数据。**标记为已知误报，无需修复。**

---

## 综合缺口（36 对）

概念对高频共现但缺少专门的 synthesis 页面。

| 概念对 | 共现页面数 | 建议 |
|---|---|---|
| [[实体/kubernetes.md|kubernetes]] × [[实体/fta-febm-methodology.md|fta febm methodology]] | 3,190 | 已有相关 synthesis 覆盖 |
| [[实体/fta-febm-methodology.md|fta febm methodology]] × [[prometheus]] | 3,000 | 建议创建 synthesis |
| [[实体/kubernetes.md|kubernetes]] × [[prometheus]] | 2,986 | 已有相关 synthesis 覆盖 |
| [[pod-lifecycle]] × [[实体/fta-febm-methodology.md|fta febm methodology]] | 2,972 | 建议创建 synthesis |
| [[实体/kubernetes.md|kubernetes]] × [[pod-lifecycle]] | 2,914 | 已有相关 synthesis 覆盖 |

---

---

## 修复结果

本次 lint 修复执行以下操作：

### 修复 1：嵌套 wikilink 模式（940 个）

模式：`path/[[basename` → `path/basename`
- AI基础设施/ — 修复 ~150 个文件中的 150+ 嵌套链接
- 应用模式/ — 修复 ~90 个文件中的 450+ 嵌套链接
- 故障诊断/FTA故障树/ — 修复 ~30 个文件中的 140+ 嵌套链接
- 故障诊断/FEBM方法论/ — 修复 ~10 个文件中的 45+ 嵌套链接
- 生产运维/topic-best-practices/migration/ — 修复 ~10 个文件中的 40+ 嵌套链接
- 其他 domain 文件 — 修复 ~50 个文件中的 115+ 嵌套链接

### 修复 2：cheat-sheet-template 链接（15 个）

模式：`[[scripts/templates/cheat-sheet-template.md|cheat sheet template]]` 和 `[[cheat-sheet-template]]` → `[[脚本/templates/cheat-sheet-template.md|cheat sheet template]]`

### 修复 3：CONTRIBUTING.md 链接（1 个）

`[[fta-template]]` → `[[脚本/templates/fta-template.md|fta template]]`

### 修复 4：MOC.md 嵌套 wikilink（1 个）

`平台工程/[[平台工程/代码分析/MOC.md|MOC]]` → `[[平台工程/代码分析/MOC.md|MOC]]`

### 修复 5：平台工程/topic-code-analysis/README.md 分组链接（5 个）

将目录名 wikilink（如 `[[实体/k8s-cluster-cert.md|k8s cluster cert]]`）转为纯文本

### 修复 6：转义 wikilink（48 个）

生产运维/MOC.md 中的 `display` → `display`

### 修复 7：_reports/ 中文 wikilink（3 个）

将中文和相对路径 wikilink 转为纯文本

### 修复 8：占位 wikilink（1 个）

`domain-X/...` → 纯文本

### 未修复（均为误报）

- bash `-f ...` 条件判断
- 构建工具配置引用（`trigger.http`, `stacks` 等）
- 模板占位符（`{{component}}`）
- 数组/列表表示（`1, 2, 3`）
- 代码块内的示例 wikilink（AGENTS.md 中的 ``wikilinks``）


## 后续行动

1. **修复破损链接**（262 个）— 最高优先级，影响用户体验
2. **孤儿页面救援** — 针对核心 wiki 孤儿页面添加入链
3. **核心页面元数据补全** — 针对 _meta/ 和 _reports/ 页面
4. **标签群组内聚** — 运行 /cross-linker 增加群组内链接
5. **综合缺口填充** — 运行 /wiki-synthesize 创建缺失的 synthesis 页面


<!-- risk-assessed -->
