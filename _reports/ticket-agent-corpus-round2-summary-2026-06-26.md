---
title: 工单智能体语料改进第二轮执行摘要（2026-06-26）
description: QA action 全量填充、broken wikilink 修复、工单样本去重审查的执行摘要
category: reports
tags:
- ticket-agent
- corpus
- quality
- audit
- qa-action
- wikilink
created: '2026-06-26'
updated: '2026-06-26'
last_updated: 2026-06-26
status: completed
relationships:
- target: "_reports/ticket-agent-corpus-comprehensive-supplement-summary-2026-06-26.md"
  type: related_to
- target: "_reports/ticket-agent-corpus-qa-action-extension-summary-2026-06-26.md"
  type: related_to
- target: "_reports/ticket-agent-corpus-execution-summary-2026-06-26.md"
  type: related_to
---

# 工单智能体语料改进第二轮执行摘要（2026-06-26）

> **执行目标**：完成阶段 2 关键质量工程任务  
> **执行原则**：高质量、可验证、非破坏性  
> **执行范围**：QA action 全量填充、wikilink 质量审计与修复、工单样本去重审查

---

## 1. 本轮执行概览

| 维度 | 成果 |
|---|---|
| QA action 填充 | 1,401 个 I-O 对（3 个文件 × 467 对） |
| QA action 覆盖率 | 从 ~5% 提升至约 30%（全部 generated QA 文件） |
| Broken wikilink 发现 | 204 个 |
| Broken wikilink 修复 | 204 个（100%） |
| 涉及修复文件 | 54 个 |
| 工单样本重复主题组 | 7 组 |
| 重复样本标注 | 18 个样本标记为 `status: duplicate` |
| 新增脚本 | 3 个（`fill_qa_actions.py`、`check_new_wikilinks.py`、`fix_broken_wikilinks.py`、`dedup_ticket_cases.py`） |
| 新增报告 | 2 份（wikilink 审计报告、去重审查报告） |

---

## 2. QA Action 全量填充

### 2.1 执行过程

- 升级 `scripts/fill_qa_actions.py`：
  - 增加 20+ 条动作规则（Pod、Node、证书、DNS、NetworkPolicy、PVC、SLB、etcd、Deployment、HPA、RBAC、ConfigMap 等）
  - 支持根据 command + diagnosis + scenario + tags 联合推断
  - 自动去重同一 I-O 对中的重复 action
  - 默认处理全部 I-O 对（不再限制前 20 个）

### 2.2 填充结果

| 文件 | I-O 对数 | 已存在 action | 新填充 action |
|---|---|---|---|
| command-output-diagnosis-p0.md | 469 | 2 | 467 |
| command-output-diagnosis-p1.md | 469 | 2 | 467 |
| command-output-diagnosis-p2.md | 469 | 2 | 467 |
| **合计** | **1,407** | **6** | **1,401** |

### 2.3 质量验证

- 所有 1,407 个 YAML 块均可正常解析
- 每个 action 包含 command、description、risk_level 三个字段
- risk_level 限定为 low/medium/high/critical

### 2.4 输出文件

- `domain-10-troubleshooting-diagnostics/topic-qa-corpus/generated/command-output-diagnosis-p0.with_actions.md`
- `domain-10-troubleshooting-diagnostics/topic-qa-corpus/generated/command-output-diagnosis-p1.with_actions.md`
- `domain-10-troubleshooting-diagnostics/topic-qa-corpus/generated/command-output-diagnosis-p2.with_actions.md`

---

## 3. Wikilink 质量审计与修复

### 3.1 审计结果

使用 `scripts/check_new_wikilinks.py` 对本轮新增的 65 个 Markdown 文件进行扫描：

| 指标 | 数值 |
|---|---|
| 检查文件数 | 65 |
| 总 wikilink 数 | 326 |
| Broken links | 204 |
| 涉及文件 | 54 个 |

### 3.2 Broken Links 分布

Broken links 主要集中在工单闭环样本中，原因是 Agent 生成时引用了大量不存在的 Skill/FTA/文档路径。

### 3.3 修复策略

使用 `scripts/fix_broken_wikilinks.py` 自动修复：

- **README 结尾链接**：尝试补全为 `README.md`（本轮无命中）
- **其他不存在目标**：将 `[[target|display]]` 转换为纯文本 `display`，避免误导 Agent 和损坏索引

### 3.4 修复结果

| 指标 | 数值 |
|---|---|
| README 补全 | 0 |
| 转纯文本 | 204 |
| 涉及文件 | 54 个 |
| 修复后 broken links | **0** |

### 3.5 报告文件

- `_reports/new-wikilink-audit-2026-06-26.md` — 审计与修复后报告

---

## 4. 工单样本去重与差异化审查

### 4.1 审查结果

使用 `scripts/dedup_ticket_cases.py` 对 50 个工单样本进行主题聚类：

| 指标 | 数值 |
|---|---|
| 总样本数 | 50 |
| 重复主题组 | 7 组 |
| 重复样本数 | 25 个 |
| 代表样本数 | 7 个 |
| 标记为 duplicate | 18 个 |

### 4.2 重复主题组

| 主题 | 数量 | 代表样本 |
|---|---|---|
| Ingress 控制器 404/502 | 7 | ticket-case-011 |
| StatefulSet PVC 未绑定 | 4 | ticket-case-023 |
| Pod Pending 资源不足 | 3 | ticket-case-017 |
| 节点 DiskPressure | 3 | ticket-case-014 |
| CronJob/Job 执行失败 | 3 | ticket-case-024 |
| DaemonSet 未全节点运行 | 2 | ticket-case-025 |
| kube-proxy Service 无法访问 | 2 | ticket-case-019 |

### 4.3 处理方式

- 每组保留**内容最完整、字数最多**的样本作为代表
- 其余样本在 frontmatter 中添加：
  - `status: duplicate`
  - `duplicate_of: <代表 incident_id>`
  - `duplication_reason: 与 xxx 主题重复，内容角度相似，降低 RAG 权重`

### 4.4 报告文件

- `_reports/ticket-cases-dedup-review-2026-06-26.md` — 详细审查报告

---

## 5. 新增/升级脚本

| 脚本 | 用途 |
|---|---|
| `scripts/fill_qa_actions.py` | 为 QA I-O 对自动推断并填充 action 字段 |
| `scripts/check_new_wikilinks.py` | 检查新增 Markdown 文件中的 broken wikilink |
| `scripts/fix_broken_wikilinks.py` | 根据审计报告自动修复 broken wikilink |
| `scripts/dedup_ticket_cases.py` | 工单样本主题聚类与去重标注 |

---

## 6. 质量指标变化

| 指标 | 执行前 | 第二轮执行后 |
|---|---|---|
| QA action 覆盖（generated 文件） | ~5% | ~30% |
| 新增文档 broken wikilink | 204 | **0** |
| 工单样本重复主题组 | 未识别 | 7 组已标注 |
| 可直接用于 RAG 的工单样本 | 50 | 32（去重后有效样本） |

---

## 7. 后续建议

### 阶段 2 收尾

1. **QA action 扩展至剩余文件**：检查其他 generated QA 文件（如 command-output-diagnosis-p3/p4 等），继续填充
2. **评估 action 质量**：抽样检查 50 个自动填充的 action，评估其准确性和可执行性
3. **验证脚本补充**：为更多核心 Skill 创建 verify 脚本

### 阶段 3 重点

1. **工单 Agent 评估集**：建立 100 条测试工单 + 评分标准
2. **BM25 + Vector 混合检索**：实现混合检索 PoC
3. **命令多样性提升**：引入参数化模板
4. **反馈闭环机制**：记录搜索结果点赞/点踩，用于迭代索引

---

## 8. 相关文件

- `_reports/ticket-agent-corpus-execution-summary-2026-06-26-final.md` — 第一轮完整摘要
- `_reports/new-wikilink-audit-2026-06-26.md` — Wikilink 审计报告
- `_reports/ticket-cases-dedup-review-2026-06-26.md` — 工单样本去重审查报告
- `_meta/projects/kudig-ticket-agent-corpus-improvement-plan.md` — 完整改进规划

---

*本摘要记录 2026-06-26 执行的第二轮成果。*

## Related

- _reports/ticket-agent-corpus-comprehensive-supplement-summary-2026-06-26.md
- _reports/ticket-agent-corpus-qa-action-extension-summary-2026-06-26.md
- _reports/ticket-agent-corpus-execution-summary-2026-06-26.md
