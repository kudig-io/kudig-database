---
title: 报告标题
summary: '报告标题：1. QA Corpus 文件（3个 .md）: 这些是 topic-qa-corpus/generated/ 下的生成文件，原始数据在
  .json 中。建议：'
category: reports
tags:
- reports
- visibility/public
tier: supporting
sources:
- auto-generated
created: 2026-05-24
updated: 2026-05-24
last_updated: 2026-05-24
---



# KUDIG 生产环境 llm-wiki 后续待办评估

**评估日期**: 2026-05-21  
**评估视角**: 生产环境部署（NAS 挂载 + 按需单文件加载 + 远程顾问模式）  
**当前状态**: 5,570 文件，评分 4.8/5

---

## 一、健康度快照

| 指标 | 数值 | 阈值 | 状态 |
|---|---|---|---|
| 总文件数 | 5,570 | — | ✅ |
| Broken Wikilinks（新增） | 40 | 0 | 🔴 **P0** |
| 孤儿页面 | 167 | <500 | ✅ |
| 无入站但有出站的页面 | 1,704 | <1000 | ⚠️ |
| 超大文件（>16K tokens） | 74 | <10 | 🔴 **P0** |
| K8s 版本覆盖 | v1.0 ~ v1.38 | 当前±2 | ✅ |
| Embedding Chunks | 2,223 | 与文件数匹配 | ✅ |
| 新增内容待 Embedding | ~253 chunks | — | ⚠️ |

---

## 二、P0 — 阻塞级（影响生产可用性）

### P0-1: 修复 40 个 Broken Wikilink（全部新增文件受影响）

**问题**: 本次新增的 16 个文件全部包含指向不存在页面的 wikilink。

**影响**: 远程顾问模式的核心机制是「概念 → 诊断 → 修复」的知识链路。当加载 `concepts/ingress-controller.md` 并尝试跳转到 `[[故障诊断/topic-skills/skill-set/k8s-ingress-gateway/SKILL.md|ingress-gateway-troubleshooting]]` 时，文件不存在，链路断裂。

**涉及文件和 broken link 分布**:

| 文件 | Broken Links 数量 | 典型缺失目标 |
|---|---|---|
| `01-production-sre-daily-ops.md` | 6 | `postmortem`, `incident-response-playbook` |
| `01-database-on-kubernetes-guide.md` | 5 | `StatefulSet`, `mysql-operator-guide` |
| `02-change-management-guide.md` | 4 | `gitops-deployment-patterns` |
| `01-containerd-deep-guide.md` | 4 | `container-runtime-security` |
| `03-slo-sli-guide.md` | 3 | `chaos-engineering-guide` |
| 其余 11 个文件 | 各 1-2 个 | troubleshooting/skill 类页面 |

**修复策略**:
- **方案 A**（推荐）: 将不存在的 wikilink 替换为纯文本描述，或创建对应的 stub 页面
- **方案 B**: 创建缺失的 troubleshooting/skill 页面（工作量大，但最完整）
- **方案 C**: 修改 wikilink 指向到现有最接近的页面（如 `[[故障诊断/topic-skills/skill-set/k8s-ingress-gateway/SKILL.md|ingress-gateway-troubleshooting]]` → `[[故障诊断/98-merged-indexes/index.md|故障诊断]]`）

**建议**: 先执行方案 A（纯文本化）确保生产可用，后续再按需创建 stub。

### P0-2: 处理 74 个超大文件（>16K tokens 安全线）

**问题**: 74 个文件超过 16K tokens，其中 3 个 QA corpus 文件达到 **458K tokens**。

**生产影响**: NAS 挂载 + 按需加载方案下，智能体通过 `skill_id` 或文件名路由加载文件。如果不小心加载了一个 458K tokens 的 QA corpus 文件，会瞬间耗尽上下文窗口（甚至超出模型限制），导致会话崩溃。

**最严重文件 TOP 10**:

| 文件 | 估算 Tokens | 类型 | 处理建议 |
|---|---|---|---|
| `command-output-diagnosis-p0.md` | 458,323 | QA Corpus | 🔴 立即拆分或移出主语料 |
| `command-output-diagnosis-p1.md` | 458,323 | QA Corpus | 🔴 立即拆分或移出主语料 |
| `command-output-diagnosis-p2.md` | 458,323 | QA Corpus | 🔴 立即拆分或移出主语料 |
| `CHANGELOG-1.19.md` | 250,930 | Release Notes | 🟡 归档或按版本拆分 |
| `CHANGELOG-1.24.md` | 242,536 | Release Notes | 🟡 归档或按版本拆分 |
| `CHANGELOG-1.32.md` | 240,854 | Release Notes | 🟡 归档或按版本拆分 |
| `CHANGELOG-1.27.md` | 238,843 | Release Notes | 🟡 归档或按版本拆分 |
| `CHANGELOG-1.28.md` | 234,012 | Release Notes | 🟡 归档或按版本拆分 |
| `CHANGELOG-1.22.md` | 232,627 | Release Notes | 🟡 归档或按版本拆分 |
| `CHANGELOG-1.31.md` | 231,447 | Release Notes | 🟡 归档或按版本拆分 |

**处理建议**:

1. **QA Corpus 文件（3个 .md）**: 这些是 `topic-qa-corpus/generated/` 下的生成文件，原始数据在 `.json` 中。建议：
   - 将 `.md` 文件从主语料中移出（保留 `.json` 供程序消费）
   - 或按 skill 拆分为每个 skill 一个 `.md` 文件（每个约 500-2000 tokens）

2. **CHANGELOG 文件（14个 >200KB）**: 共 136 个 CHANGELOG 文件。建议：
   - 将 `生态参考/` 下的 CHANGELOG 移入 `_archives/`
   - 或仅保留最近 3 个版本的 CHANGELOG 在主语料中

3. **建立加载黑名单**: 在加载逻辑中明确排除 `topic-qa-corpus/generated/*.md` 和 `CHANGELOG-*.md`

---

## 三、P1 — 重要级（影响质量和效率）

### P1-1: Embedding 增量更新

**问题**: 新增 16 个文件 + 1 个报告 ≈ 253 个 chunks 未加入向量索引。

**影响**: 语义搜索时，新增内容无法被检索到。远程顾问查询「SRE 日常巡检」时，`生产运维/01-production-sre-daily-ops.md` 不会出现在搜索结果中。

**执行**: 运行 `embedding-pipeline.py` 的增量模式，预计 5-10 分钟。

### P1-2: 建立自动化 Wiki-Lint（防回归）

**问题**: 本次修复引入了 40 个 broken link。如果没有自动化检查，后续每次内容新增都会重复此问题。

**建议**: 在 CI 或本地 hook 中加入 wiki-lint 检查：
- 每次 commit 前运行 broken link 扫描
- 每次 commit 前运行 frontmatter 合规检查
- 每次 commit 前运行大文件检查（>100KB 警告，>200KB 报错）

### P1-3: 完善 Skill-Concept 双向链路

**问题**: 当前 concepts/ 有 67 个文件（原有 62 + 新增 5），但 skills 目录中的技能很少直接引用 concept 文件。概念和诊断之间是单向连接（concept → troubleshooting），缺少 troubleshooting → concept 的回链。

**建议**: 在主要 skill 文件中添加 `相关概念：概念名` 章节，建立双向链路。

### P1-4: 处理 1,704 个"无入站但有出站"页面

**问题**: 这些页面引用了其他页面，但自身未被引用。虽然比孤儿页面好（它们有知识输出），但表明知识图谱的引用方向单一。

**建议**: 低优先级，可通过 cross-linker 的「反向链接发现」功能逐步改善。

---

## 四、P2 — 优化级（提升体验）

### P2-1: Domain-11 继续增强

当前 生产运维 从 12 文件增至 14 文件，但仍是所有 domain 中最少的。建议补充：
- 值班手册模板
- 告警分级响应流程
- 生产事故复盘模板

### P2-2: 定期 Digest 和 Insights 更新

当前 `_insights.md` 和 `journal/digest-2026-05-23.md` 是基于 5,494 文件的。新增 17 个文件后，insights 已过时。

建议: 每月更新一次 insights，每周更新一次 digest。

### P2-3: 大文件拆分策略文档化

将超大文件处理规则写入 `AGENTS.md` 或 `docs/CONTRIBUTING.md`：
- 单文件不超过 100KB（约 5K tokens）
- 超过阈值必须拆分或使用索引文件
- QA corpus 生成文件不进入主语料

### P2-4: K8s 1.33+ 内容跟进

当前语料已覆盖到 v1.38（主要是 CHANGELOG），但核心概念文档中 K8s 1.33+ 的新特性（如 SidecarContainers GA、PodLifecycleSleepAction 等）可能未充分覆盖。

建议: 每季度做一次「版本内容审计」，检查核心概念是否包含最新版本特性。

---

## 五、执行建议

### 立即执行（本周）
1. **P0-1**: 修复 40 个 broken wikilink → 纯文本化或创建 stub
2. **P0-2**: 将 3 个 QA corpus .md 移出主语料或拆分
3. **P1-1**: 运行 embedding 增量更新

### 短期（2周内）
4. **P0-2 续**: 处理 14 个超大 CHANGELOG
5. **P1-2**: 建立本地 wiki-lint 自动化脚本
6. 重新运行 wiki-lint，确认 broken link 归零

### 中期（1个月内）
7. **P1-3**: Skill-Concept 双向链路补全
8. **P2-1**: Domain-11 再增强 3-5 个文件
9. **P2-2**: 更新 insights 和 digest

---

## 六、风险评估

| 风险 | 概率 | 影响 | 缓解措施 |
|---|---|---|---|
| 加载超大文件导致 Token 溢出 | 高 | 会话崩溃 | 建立加载黑名单 |
| Broken link 导致知识链路断裂 | 中 | 回答不完整 | 立即修复 + 自动化检查 |
| Embedding 未更新导致新内容检索不到 | 中 | 回答质量下降 | 立即增量更新 |
| 内容版本过时 | 低 | 回答不准确 | 季度版本审计 |

---

*评估完成: 2026-05-21 | 下次评估: 建议 P0 修复完成后*

## Related

- [[visibility-public|#visibility/public Hub]] — tag hub
