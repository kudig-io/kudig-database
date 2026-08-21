---
title: FTA故障树模块系统性质量评审与修复记录（2026-08-13）
description: 对 19-故障诊断/06-FTA故障树/ 目录的系统性质量评审报告及高/中/低全部改进建议的执行追踪
summary: FTA 模块质量评审：总体结论、12 条改进建议（H1-H4/M1-M4/L1-L4）的问题描述、改进方案、预期收益与执行状态，及执行中新增登记 N1-N10
category: reports
tags:
- fta
- quality-review
- maintenance
- audit
tier: supporting
created: '2026-08-13'
updated: '2026-08-13'
last_updated: 2026-08-13
---

# FTA故障树模块系统性质量评审与修复记录

> **评审日期**: 2026-08-13
> **评审范围**: `19-故障诊断/06-FTA故障树/` 全目录（37 个顶层文档 + 48 个组件故障树 + 23 个术语卡片）及其跨模块引用
> **审计人**: KUDIG Maintenance (AI-assisted review)
> **执行状态图例**: ✅ 已完成 | 🟡 部分完成（附原因） | ⬜ 未执行（附原因）

---

## 一、总体结论

FTA 专题是项目中最完整的排障知识资产（约 108 个 md 文件、2.2 万行），方法论体系（23 章 + 4 附录）、组件级故障树（48 个）、AI Agent 实践（第 8-13/20 章）三层结构均有扎实内容。但存在 3 个系统性问题：

1. **文档入口与索引严重不一致**：README/MOC/index 统计数字互相矛盾（29 / 79 / 108），6 个工程文档游离于索引之外；
2. **全量分析手册 v1 标题层级损坏**（530 个一级标题）+ v1/v2 双版本并存无版本定位说明；
3. **跨模块引用大量失效**：旧路径名（`故障诊断/FTA故障树/`）、路径夹杂空格（`06-FTA 故障树`、`19-故障诊 断`）、旧英文目录名（`00-core-troubleshooting/`）等。

---

## 二、问题清单总览

| 编号 | 优先级 | 问题摘要 | 执行状态 |
|:---:|:---:|:---|:---:|
| H1 | 🔴 高 | v1 全量手册 530 个一级标题层级损坏 | ✅ 已完成（验证为误报） |
| H2 | 🔴 高 | v1/v2 双版本并存、无版本定位说明 | ✅ 已完成 |
| H3 | 🔴 高 | 6+ 处失效交叉引用（旧路径名+路径空格） | ✅ 已完成（含 H3 漏网 30+ 处） |
| H4 | 🔴 高 | appendix-d frontmatter 损坏、废弃状态未标注 | ✅ 已完成 |
| M1 | 🟡 中 | 4 个入口文档职责重叠、统计口径不一致 | ✅ 已完成 |
| M2 | 🟡 中 | list 索引缺失 12 个组件 FTA，统计过时（36→48） | ✅ 已完成 |
| M3 | 🟡 中 | FTA ↔ 多故障场景/技能/QA 语料无衔接 | ✅ 已完成 |
| M4 | 🟡 中 | 3 个工程文档定位不明、frontmatter 字段为空 | ✅ 已完成 |
| L1 | 🟢 低 | 合集标题层级混入、快照定位未声明 | ✅ 已完成（标题误报，快照已声明） |
| L2 | 🟢 低 | 缺少 FTA-Agent 评测集设计 | ✅ 已完成（新建 24-fta-agent-evaluation.md） |
| L3 | 🟢 低 | 术语体系（appendix-a vs glossary）重复未互链 | ✅ 已完成 |
| L4 | 🟢 低 | 标题层级/死链检查未接入质量 CI | ✅ 已完成 |
| N1 | 🔴 高 | v1 尾部约 110 行重复内容（4.5/五、附录） | ✅ 已完成 |
| N2 | 🟡 中 | `19-故障诊断/index.md` 引用 8 个旧英文目录名 | ✅ 已完成 |
| N3 | 🟡 中 | `09-多故障场景/index.md` wiki 链接含空格 | ✅ 已完成 |
| N4 | 🟢 低 | MOC.md created/last_updated 日期倒挂 | ✅ 已完成 |
| N5 | 🟡 中 | `35-元数据/tags-index.md` 13 处 FTA 旧短路径 | ✅ 已完成 |
| N6 | 🟡 中 | QA 语料 1900+ 处旧路径 + appendix-a/fta-index frontmatter 损坏 | ✅ 已完成 |
| N7 | 🟡 中 | 跨模块 8 文件标题层级异常 + H3 漏网引用 | 🟡 部分完成（引用已修复；8 文件登记 CI 已知清单待专项修复） |
| N8 | 🟡 中 | 执行标注使用行尾 `# 注释`（80 处）污染 Markdown 渲染（标题/表格/列表） | ✅ 已完成（统一转换为 HTML 注释 `<!-- xxx -->`，frontmatter 内 YAML 注释保留） |
| N9 | 🟡 中 | `19-故障诊断/SUMMARY.md` 约 48 处旧目录名/已删除目标死链（自动生成文件遗留） | 🟡 部分完成（可确定性映射的 55 处已修复：09-可观测性 数字偏移 44 处 + ack 实体前缀 11 处；其余目标已删除，登记遗留待重新生成） |
| N10 | 🔴 高 | `08-技能体系` 批量格式污染与引用失效（双重 frontmatter / 序号化 skill_id / 旧路径 / 含空格文件名副本） | ✅ 已完成 |

> N1-N4 为执行过程中新发现并登记的问题（N1 与 H1 同文件一并修复，N2-N3 随 H3/M3 修复，N4 随 M1 修复）；N5-N7 为本次执行新增登记（N5 tags-index 旧短路径、N6 QA 语料与 frontmatter 损坏、N7 跨模块遗留）。N8 为二次验证发现（行尾注释污染渲染，随全局验证修复）；N9 为全局死链扫描发现（SUMMARY.md 自动生成文件陈旧内容，已修可映射部分并登记遗留）；N10 为技能模块专项扫描发现（08-技能体系 批量格式污染，含双重 frontmatter、skill_id 双重编号、含空格文件名副本等，已全量修复）。

---

## 三、改进建议明细与执行状态

### 🔴 H1: 修复 `kubernetes-fta-full-analysis.md`（v1）标题层级灾难

- **问题描述**: v1 共 4317 行，其中 530 个一级标题（`# 检查API Server Pod状态`、`# 检查etcd日志` 等排查命令步骤全部错误使用 `#`），仅 7 个二级、28 个三级标题。文档目录导航完全失效，严重影响阅读与 Agent 检索。
- **改进方案**: 将 v1 中 529 个一级标题批量降级为 `####`（顶层标题保留唯一 `#`），跳过 fenced code block 内的 `#` 注释行；同步删除尾部约 110 行重复内容（重复的 ASCII 决策树、4.5 阈值表、五、附录）。
- **预期收益**: 恢复文档目录导航；修复基于标题层级的知识检索与 QA 语料生成质量；消除标题噪音对 Agent 分块检索的干扰。
- **涉及文件**: `19-故障诊断/06-FTA故障树/kubernetes-fta-full-analysis.md`
- **执行状态**: ✅ 已完成（围栏感知验证：530 个一级标题均在代码块内，属误报无需降级；同文件尾部重复内容随 N1 清理）

### 🔴 H2: 消除 v1/v2 双版本并存歧义，明确版本定位

- **问题描述**: `kubernetes-fta-full-analysis.md`（8 TE / 63 BE）与 `-v2.md`（16 TE / ~300 BE）并存；README 只索引 v1 并称"8 个顶事件、63 个底事件"（与 v2 自述 v1"~90 底事件"矛盾），而 fta-index.md 全部引用 v2。用户无法判断以哪个为准。
- **改进方案**: ① README"快速导航"与"主文档"处标注 v2 为现行版本、v1 为历史版本（v2 已取代），修正 v1 数字描述；② v1 文件头部加 deprecated 横幅指向 v2；③ README 最近更新区补充 v2 说明。
- **预期收益**: 消除"两个全量手册"的认知冲突；使 fta-index 的链接目标与 README 入口一致。
- **涉及文件**: `06-FTA故障树/README.md`、`06-FTA故障树/kubernetes-fta-full-analysis.md`
- **执行状态**: ✅ 已完成（README 主文档表/快速导航/最近更新标注 v2 现行、v1 历史；v1 头部 deprecated 横幅；SUMMARY 双条目）

### 🔴 H3: 修复全部失效的交叉引用（旧目录名 + 路径空格）

- **问题描述**: 经查证存在 6+ 处失效引用：`01-核心排障/02-control-plane-etcd-troubleshooting.md`（3 处：`../故障诊断/FTA 故障树/...`、`"../19-故障诊 断/..."`、`[[19-故障诊断/06-FTA 故障树/...]]`）、`01-核心排障/04-storage-csi-troubleshooting.md`（`../故障诊断/FTA故障树/...`）、`06-FTA故障树/list/README.md`（`../故障诊断/topic-skills/README.md` 应为 `../08-技能体系/README.md`）、`19-故障诊断/index.md`（8 个旧英文目录名）。
- **改进方案**: 全局替换旧路径模式（`FTA 故障树`/`FTA故障树`/`19-故障诊 断`/`故障诊断/topic-`）为 `06-FTA故障树`；重建 `19-故障诊断/index.md` 子目录引用。
- **预期收益**: 消除死链；恢复跨模块导航（01-核心排障 → FTA 树跳转可用）。
- **涉及文件**: `01-核心排障/02-control-plane-etcd-troubleshooting.md`、`01-核心排障/04-storage-csi-troubleshooting.md`、`06-FTA故障树/list/README.md`、`19-故障诊断/index.md`
- **执行状态**: ✅ 已完成（含执行中发现的 H3 漏网 30+ 处：list 组件树 topic-structural/topic-skills 残留、03-基础设施排障 5 文件、problem-solving-architecture 8 处、QA 语料 index 说明等，全部映射新路径并验证目标存在）

### 🔴 H4: 修复 `appendix-d-templates.md` frontmatter 损坏与废弃状态未标注

- **问题描述**: appendix-d 在 52-91 行存在第二个 YAML frontmatter 块（文件中间出现纯文本 `title:`/`description:` 块），顶部 description 截断（`description: — FTA 方法论与 AI Agent 实践'`）；且文件已声明"⚠️ 已废弃"（内容迁移至 `31-脚本/templates/fta-template.md`，已确认存在），但 README/MOC 仍将其列为有效附录。
- **改进方案**: ① 删除 52-91 行重复 frontmatter，修复顶部 description；② README 附录 D 条目标注 `(已废弃 → 31-脚本/templates/fta-template.md)`。
- **预期收益**: 修复 YAML 解析错误（影响 frontmatter 校验 CI）；杜绝用户使用已废弃模板。
- **涉及文件**: `06-FTA故障树/appendix-d-templates.md`、`06-FTA故障树/README.md`
- **执行状态**: ✅ 已完成（重复 frontmatter 已删、description 修复；README/SUMMARY 标注已废弃 → 31-脚本/templates/fta-template.md）

### 🟡 M1: 统一入口文档职责，消除 README/MOC/index/fta-index 四重冗余

- **问题描述**: 目录内存在 4 个入口文档：README（人工维护）、MOC.md（脚本生成）、index.md（自动目录索引）、fta-index.md（v2 故障树专用索引）。统计口径互不一致：README"29 篇"、MOC"79 篇"、实际 108 个 md。index.md 自动索引缺失 18-typical-scenarios.md 且未含 glossary 子目录；MOC 链接格式损坏（`[[path|[[path|名称]]]]` 双嵌套）；MOC created/last_updated 日期倒挂（N4）。
- **改进方案**: ① 分工声明写入 README；② README 更新文档数量为实时口径，新增"工具与工程文档"小节补齐 6 个游离文档（ack-fta-generator-v2、fta-diagnosis-improvement、fta-execution-engine、fta-index、problem-solving-architecture、symptom-vector-matcher）；③ index.md 补 18-typical-scenarios.md 与 glossary 子目录；④ MOC.md 修复双嵌套链接与日期倒挂、更新文档数量。
- **预期收益**: 入口一致、数字可信；6 个游离文档获得导航入口。
- **涉及文件**: `06-FTA故障树/README.md`、`06-FTA故障树/MOC.md`、`06-FTA故障树/index.md`
- **执行状态**: ✅ 已完成（README 入口分工声明 + 110 篇统计；MOC 双嵌套链接修复、数量 79→110、N4 日期倒挂修复、收录 24 章；index 补 18/22/24 章、appendix-a~d、list/glossary 子目录）

### 🟡 M2: 修正 `list/README.md` 与 `19-故障诊断/README.md` 的"36 个故障树"过时统计

- **问题描述**: `list/` 实际含 48 个 FTA 文件，另有 12 个（calico、cilium、cni、containerd、flannel、higress、kube-proxy、kubeadm、kubelet、nginx-ingress、openkruise、pod-creation-end-to-end）未收录进 list/README 索引，也未进 `19-故障诊断/SUMMARY.md`；两处 README 均写"36 个"。
- **改进方案**: ① 12 个缺失文件按领域补入 list/README 分类表；② 同步更新 `19-故障诊断/README.md` 描述与 `SUMMARY.md` 子章节。
- **预期收益**: 48 个组件 FTA 在导航中完整呈现，检索可达。
- **涉及文件**: `06-FTA故障树/list/README.md`、`19-故障诊断/README.md`、`19-故障诊断/SUMMARY.md`
- **执行状态**: ✅ 已完成（12 个 FTA 补录分类表，统计 36→48；README L31 与 SUMMARY 子章节同步）

### 🟡 M3: 建立 FTA ↔ 多故障场景/技能/QA 语料的双向衔接

- **问题描述**: `09-多故障场景/`（级联故障、雪崩、复合问题）零 FTA 引用——而 FTA 的 AND 门、共因失效正是多故障分析的天然工具；`26-技能/` 下已有技能形态 FTA（node-fta.md 等，含 `fta_id: FTA-NODE-001` 元数据）与 `list/` 同主题内容并存但互不链接；`10-QA语料/generated/command-output-diagnosis-*` 明显由 FTA 生成却无来源标注。
- **改进方案**: ① `09-多故障场景/index.md` 增加"方法论衔接"小节链接 FTA 的 AND 门/共因失效章节；② `list/README.md` 增加"技能映射"列；③ QA 语料生成文件补 frontmatter 来源字段。
- **预期收益**: FTA 从"排障知识库"升级为"多故障诊断方法工具"；消除技能域与知识域的同题重复。
- **涉及文件**: `09-多故障场景/index.md`、`06-FTA故障树/list/README.md`、`10-QA语料/generated/command-output-diagnosis-all.yaml`
- **执行状态**: ✅ 已完成（09 章方法论衔接小节；list/README 技能映射表；QA 语料 8 文件 source: fta-skills 字段）

### 🟡 M4: 明确"执行引擎/症状匹配/排查架构"三个工程文档的定位与分工

- **问题描述**: `fta-execution-engine.md`（873 行）、`symptom-vector-matcher.md`（788 行）、`problem-solving-architecture.md`（427 行）三文档互不引用、未在 README 索引，与 09 章 9.2"执行引擎架构"、fta-index"快速查询算法"存在概念重叠；frontmatter 的 audience/intent_queries/trigger_keywords/prerequisites 全部为空。
- **改进方案**: ① README 新增"工程化工具文档"小节并写定位说明；② fta-execution-engine 与 09 章 9.2 互链；③ 补齐三文档 frontmatter 空字段。
- **预期收益**: 工程文档从"孤儿"变为可发现的实现参考。
- **涉及文件**: `06-FTA故障树/README.md`、`fta-execution-engine.md`、`symptom-vector-matcher.md`、`problem-solving-architecture.md`
- **执行状态**: ✅ 已完成（README 工具与工程文档小节含定位说明；3 文档 description/summary 补齐 + 相关文档互链；09 章 9.2 互链）

### 🟢 L1: 修复合集文档标题层级与漂移风险

- **问题描述**: `fta-methodology-and-agentic-practices.md`（4269 行）与 23 个分章内容重复（合集=快照，分章=现行），存在漂移风险；合集内混入 `# 系统边界定义模板`、`# 🟢 低风险...` 等内容级一级标题（29 个 `# `，其中 7 个为"第 X 部分"分隔标题，约 22 个需降级）。
- **改进方案**: ① 批量修正合集内内容级一级标题为 `###`（跳过代码块与部分分隔标题）；② README 标注"合集为离线快照，分章为权威来源"。
- **预期收益**: 消除双份维护漂移隐患。
- **涉及文件**: `06-FTA故障树/fta-methodology-and-agentic-practices.md`、`06-FTA故障树/README.md`
- **执行状态**: ✅ 已完成（29 个一级标题经验证均在代码块内属误报；快照声明横幅已加；README 标注合集为离线快照）

### 🟢 L2: 补充 Agent 评测闭环（FTA 诊断正确性评测集）

- **问题描述**: 第 8-13 章落地路径清晰，但缺少"如何评测 Agent 按 FTA 诊断是否正确"的闭环章节——15 章仅有 A/B 测试概念，无评测集/指标/基准线。
- **改进方案**: 新建 `24-fta-agent-evaluation.md`：定义诊断准确率（TE 命中率）、路径完整率（TE→IE→BE）、误报率三个指标，设计 20-30 条评测基准（引用 10-QA语料 工单）。
- **预期收益**: 使"FTA 作为 Agent 知识骨架"从方法论变为可度量、可迭代的工程实践。
- **涉及文件**: 新建 `06-FTA故障树/24-fta-agent-evaluation.md`
- **执行状态**: ✅ 已完成（新建 24-fta-agent-evaluation.md：三指标 + 20 条评测基准（16 工单正样本 + 2 QA 对齐 + 2 负样本）+ 评分规则 + 迭代闭环；SUMMARY/README/MOC/index 同步收录）

### 🟢 L3: 术语体系去重（appendix-a 与 glossary 互链）

- **问题描述**: `appendix-a-glossary.md`（150 行术语表）与 `glossary/`（23 个术语卡片）内容重叠但互不引用。
- **改进方案**: appendix-a 增加"详细词条见 glossary"链接表；glossary/index.md 反向链接 appendix-a。
- **预期收益**: 术语检索路径统一。
- **涉及文件**: `06-FTA故障树/appendix-a-glossary.md`、`06-FTA故障树/glossary/index.md`
- **执行状态**: ✅ 已完成（appendix-a 新增 20 词条 glossary 链接表；glossary/index 反向链接 appendix-a；随 N6 修复 appendix-a frontmatter 嵌套与损坏链接）

### 🟢 L4: FTA 质量评估接入 CI

- **问题描述**: 标题层级损坏（H1/L1）与死链（H3）问题无自动化拦截，存在复发风险；项目已有 `quality.yml`（Ruff/README 同步/Frontmatter 校验）。
- **改进方案**: 在 `.github/workflows/quality.yml` 增加 `heading-integrity` job：扫描 corpus 内容目录，一级标题数量超过阈值（20）的文件报错（README/MOC/index 等入口文档除外）。
- **预期收益**: 防止标题层级与死链问题复发。
- **涉及文件**: `.github/workflows/quality.yml`
- **执行状态**: ✅ 已完成（新增 heading-integrity job：fence 感知扫描一级标题阈值 20，入口文档豁免，8 个已知问题文件登记 KNOWN 清单防复发；本地模拟验证通过）

### 🔢 新发现 N8: 行尾 `# 注释` 污染 Markdown 渲染

- **问题描述**: 执行改进建议时为满足可追溯性要求，在正文行尾以 `  # Hx: ...` 形式标注建议编号（共 80 处、18 个文件）。但 Markdown 中行尾 `#` 不是注释语法：表格行尾注释会作为单元格内容渲染、标题行尾注释会污染标题文本与锚点（如 `## 相关文档  # M4: ...` 导致锚点失效，而 `problem-solving-architecture.md` 恰好引用 `fta-execution-engine.md#修复执行控制器` 锚点）、列表项行尾注释同样可见。
- **改进方案**: 正文行尾注释统一转换为 HTML 注释 `<!-- Hx: ... -->`（不渲染、保留可追溯性）；frontmatter（YAML）内的 `# 注释` 为合法 YAML 注释，予以保留；同时修复注释文本内异常空格与嵌套 wiki 链接损坏（`03-fta-symbol-system-and-standards.md` L144）。
- **预期收益**: 恢复标题锚点与表格/列表渲染；保留逐条可追溯标注；消除对 GitHub/网站渲染与 QA 语料生成的污染。
- **涉及文件**: 18 个（06-FTA故障树 目录 15 个 + 19-故障诊断/index.md、README.md、SUMMARY.md + 09-多故障场景/index.md）
- **执行状态**: ✅ 已完成（80 处正文注释转换为 HTML 注释，闭合校验通过；frontmatter 内 YAML 注释保留）

### 🔢 新发现 N9: `19-故障诊断/SUMMARY.md` 陈旧自动生成死链

- **问题描述**: 全局死链扫描发现 SUMMARY.md（Obsidian 自动生成文件，created 2026-05-23）约 97 处失效链接：① 09-可观测性 子目录数字偏移（`01-总览`→`00-总览`、`02-指标`→`01-指标` 等，44 处）；② ack 实体裸文件名缺路径（`001-ack-ecs-compute.md` → `23-实体/13-云厂商与发行版/`，11 处）；③ 全局重命名前的旧目录名（`云厂商/`、`生产运维/`、`可观测性/`、`工作负载/`、`故障诊断/`、`系统基础/`、`集群基础/`）；④ 已删除目标（`32-发布/package/2026-07-02_18-29/`、`topic-presentations/`、`QUALITY-REPORT.md`、`26-技能 pod 清单规范/生命周期与事件` 等）。
- **改进方案**: 对确定性映射（①②）逐条验证目标存在后批量替换；③④ 目标文件已删除或映射不确定（如 `云厂商/02-google-cloud-gke/` 对应 `18-云厂商/03-Google-GKE/` 但文件名已变），不宜人工臆测。
- **预期收益**: SUMMARY.md 恢复可导航性；确定性死链清零。
- **涉及文件**: `19-故障诊断/SUMMARY.md`
- **执行状态**: 🟡 部分完成（55 处确定性映射已修复并验证目标存在；剩余约 42 处目标已删除/映射不确定，登记遗留：建议重新运行 SUMMARY 生成器或专项清理 ③④ 类条目）

### 🔢 新发现 N10: 08-技能体系 批量格式污染与引用失效（双重 frontmatter / 序号化 skill_id / 旧路径 / 含空格文件名副本）

- **问题描述**: `19-故障诊断/08-技能体系/`（28 个编号文档 + 索引）存在 6 类问题：① 6 个文件（20/22/23/24/25/26 号）正文中误存第二个 YAML frontmatter 块（Agent 元数据：severity_range/trigger_events/trigger_metrics/related_skills/fta_refs/knowledge_refs/cross_refs/authors），污染 Markdown 渲染且与顶部 frontmatter 冲突（tier 不一致）；② 28 个文件 description/summary 从正文复制含 wiki 链接并截断（如 `# Skill 本地运行 Demo 指南`、`│ Layer 2: topic-structural-...`），README 的 description/summary 为 ASCII 图片段；③ skill_id 双重编号体系并存——序号化（`SKILL-02_POD_CRASHLOOP_OOMKILLED-001`，由 31-脚本/batch-fix-quality.py 生成）与语义化（`SKILL-POD-001`，README/26-技能 引用方为权威），且 27 号 `SKILL-25_` 与 25 号文件序号冲突、28 号 frontmatter `SKILL-26_` 与正文声明 `SKILL-HELM-001` 矛盾；④ 路径引用大量失效：双重前缀 `../19-19-故障诊断/`、旧路径 `故障诊断/topic-skills/`、无效片段 `02-工作负载/`、`存储/`、`集群基础/`、`网络/`，死引用 `./23-job-cronjob-failure.md`（实为 statefulset 文件）、`./11-control-plane-failure.md`（实为 12 号）；⑤ 嵌套 wiki 链接损坏（21 号 summary/正文 `README.md](./[[10-平台工程/...|README]].md)`、25 号 `[[23-实体/kubernetes.md|[[kubernetes|kubernetes]]]]`、28 号 `[[helm|Helm]]`、README 标题 `[[SKILL|Skill]]`）；⑥ 含空格文件名损坏副本 `07-pvc-storage-failure 2.md`（title 带"(故障诊断)"后缀、skill_id 含空格、与 08 权威文件重复），QA 语料 14 文件 900+ 处 skill_ref/tags/source 引用旧文件名，README 全景表序号偏移 1 且缺 20-28 号条目、`04-高级排障/structural-` 截断、孤立围栏、过时"待开发 Skill"表（所列 5 项已全部开发完成）。
- **改进方案**: ① 6 文件第二个 frontmatter 合并进顶部（删除正文块、字段并入、以顶部 frontmatter 为准）；② 全部 description/summary 重写为语义化风格（"Kubernetes XXX 的完整诊断-修复-验证工单处理 Skill"），skill_id 统一为语义化（SKILL-NODE-001～SKILL-HELM-001；27→SKILL-CP-002、28→SKILL-HELM-001 与正文声明对齐）；③ 路径统一根相对（`19-故障诊断/...`），修复双重前缀/旧路径/死引用；④ 嵌套 wiki 链接清理（标题与正文）；⑤ 删除 `07-pvc-storage-failure 2.md` 副本，QA 语料全部旧引用更新为 08 权威（先带" 2"后普通，防子串误伤）；⑥ README 全景表重建（01-27 连续编号 + 27/28 补录 + 成熟度标注 + 编号说明）、快速导航/症状表链接修复（序号偏移 20 处）、待开发表更新、孤立围栏删除；⑦ 联动修复 index.md 副本条目、26-技能 2 个镜像文件 skill_id、SUMMARY.md/ENHANCEMENT-RECORD/QUICK-REFERENCE/22-概念案例/17-系统基础 cross_refs/02-资源排障 无效域路径（`../网络/`→`05-网络/` 等）/09-多故障场景/35-元数据（metadata-enhanced.json 键合并、_insights 图快照）。
- **预期收益**: 08-技能体系 渲染纯净（无双重 frontmatter）、skill_id 单一权威体系、跨模块引用全部有效、含空格文件名清零、README 索引与物理文件一一对应。
- **涉及文件**: 08-技能体系 27 个 md + README/index/ENHANCEMENT-RECORD + 26-技能 2 个镜像 + QA 语料 14 文件（10 generated md/yaml + 4 json + 2 语料 yaml）+ 35-元数据 2 文件 + 17-系统基础/22-概念/31-脚本 3 文件 + 02-资源排障 + 09-多故障场景 + SUMMARY.md
- **执行状态**: ✅ 已完成（全量验证：07 引用活跃内容清零、topic-skills 旧路径清零、`19-19-` 双重前缀清零、含空格文件名引用清零；仅 `.claude/scripts/output` 生成产物与历史评估报告保留旧引用，登记遗留）

---

## 四、执行变更摘要

> 本部分在全部建议执行完毕后更新（2026-08-13 完成）。

### 新建/修改文件清单

**新建（2）**：

| 文件 | 建议 | 说明 |
|:---|:---:|:---|
| `19-故障诊断/06-FTA故障树/24-fta-agent-evaluation.md` | L2 | Agent 评测集设计（三指标 + 20 条基准） |
| `36-报告/assessments/fta-module-quality-review-2026-08-13.md` | P1 | 本维护记录 |

**修改（按建议编号）**：

| 建议 | 涉及文件（主要） |
|:---:|:---|
| H1/N1 | `kubernetes-fta-full-analysis.md`（验证误报；尾部重复清理） |
| H2 | `README.md`、`kubernetes-fta-full-analysis.md`（deprecated 横幅） |
| H3/N2 | `01-核心排障/02-control-plane-etcd-troubleshooting.md`、`04-storage-csi-troubleshooting.md`、`03-基础设施排障/*`（5 文件）、`list/README.md`、`problem-solving-architecture.md`、`18-typical-scenarios.md`、`19-故障诊断/index.md`、QA 语料 index 等（含 H3 漏网 30+ 处） |
| H4 | `appendix-d-templates.md`、`README.md`、`SUMMARY.md` |
| M1/N4 | `README.md`、`MOC.md`、`index.md` |
| M2 | `list/README.md`、`19-故障诊断/README.md`、`SUMMARY.md` |
| M3/N3 | `09-多故障场景/index.md`、`list/README.md`、`10-QA语料/generated/*`（8 文件 source 字段） |
| M4 | `README.md`、`fta-execution-engine.md`、`symptom-vector-matcher.md`、`problem-solving-architecture.md`、`09-fta-as-agent-knowledge-skeleton.md` |
| L1 | `fta-methodology-and-agentic-practices.md`（快照声明）、`README.md` |
| L2 | 新建 `24-fta-agent-evaluation.md`；`SUMMARY.md`/`README.md`/`MOC.md`/`index.md` 同步收录 |
| L3 | `appendix-a-glossary.md`、`glossary/index.md` |
| L4 | `.github/workflows/quality.yml`（heading-integrity job + N7 KNOWN 清单） |
| N5 | `35-元数据/tags-index.md`（13 处旧短路径） |
| N6 | `10-QA语料` 旧路径 1900+ 处、`appendix-a-glossary.md`/`fta-index.md` frontmatter |
| N7 | 跨模块 8 文件登记 CI KNOWN（引用部分已随 H3 修复） |
| N8 | 18 个文件行尾注释→HTML 注释（80 处）；`03-fta-symbol-system-and-standards.md` 嵌套链接、`24-fta-agent-evaluation.md` 4 处 QA 路径、`list/openkruise-fta.md` 3 处生态参考路径、`MOC.md` 旧目录名 |
| N9 | `19-故障诊断/SUMMARY.md` 55 处确定性死链修复 |
| N10 | 08-技能体系 27 文件（双重 frontmatter 合并 6、description/summary 重写 28、skill_id 语义化 29、嵌套链接清理 4、路径修复）；README 全景表重建/快速导航/症状表；删除 `07-pvc-storage-failure 2.md`；QA 语料 14 文件 900+ 处；26-技能 2 镜像；35-元数据 2；17-系统基础/22-概念/31-脚本/02-资源排障/09-多故障场景/SUMMARY.md 联动 |
| N10-补漏 | 最终验证新增：`skill-set/k8s-pvc-storage/SKILL.md` 知识库关联表 6 行旧路径（根级旧编号 + `19-19-` 双重前缀 + 反引号格式）→ 子目录实际位置；`11-image-pull-failure.md`/`assessment/k8s-fundamentals-quiz.md`/`assessment/troubleshooting-lab-exam.md` 3 处 `[[23-实体/kubernetes.md|...]]` 嵌套链接 → `23-实体/02-K8s核心组件/kubernetes.md`；`MOC.md` L144 `09-可观测性/01-总览` → `00-总览`（数字偏移死链）；`dialogue/DIALOGUE-PSP-001.md` L240 已删除发布快照 `32-发布/package/2026-07-02_18-29/...` → `08-安全/README.md` |
| 收尾 | `fta-index.md` 变更记录、`36-报告/assessments/index.md` 补录本维护记录 |

### 遗留事项

1. **N7**：8 个标题层级异常文件已登记 `quality.yml` heading-integrity KNOWN 清单（仅拦截新增），待专项修复后移出；
2. **N9**：`SUMMARY.md` 约 42 处死链目标已删除或映射不确定（`云厂商/`、`生产运维/`、`32-发布/package/2026-07-02_18-29/` 等旧路径/已删除内容），建议重新运行 SUMMARY 生成器或专项清理；
3. **N10-生成产物**：`.claude/scripts/output/*.json`（registry/hub_candidates/page_stats）仍引用旧 domain 路径（`domain-10-troubleshooting-diagnostics/topic-skills/...`），属脚本输出缓存，下次运行自动覆盖，未人工修改；
4. **N10-历史报告**：`36-报告/assessments/domain-content-gap-analysis-2026-07-01.md` L401 与 `TRI-DIMENSION-DEEP-ASSESSMENT-2026-05-23.md` L100 关于 `07-pvc-storage-failure 2` 的记载为历史审计描述，保留原文（本次删除副本后已与维护记录形成闭环）；
5. **跨模块穷举**：`SUMMARY.md` 之外的自动生成索引（如 36-报告、35-元数据 等）未做逐目录死链穷举，后续可复用 `31-脚本/` 检查工具专项扫描。
6. **N10-补漏（已闭环）**：最终验证阶段发现并修复 08-技能体系 skill-set/assessment/dialogue 子目录 5 处死链（k8s-pvc-storage/SKILL.md 知识库关联表旧路径、3 处 `23-实体/kubernetes.md` 嵌套链接、MOC.md `09-可观测性/01-总览` 数字偏移、DIALOGUE-PSP-001.md 已删除发布快照引用），修复后目标全部验证存在。

### 验证结论

1. **标题层级**：v1 与合集的 530/29 个一级标题经围栏感知验证均在代码块内（误报）；heading-integrity CI 本地模拟通过（`checked 1442 markdown files`，0 异常，8 个 KNOWN 放行）；
2. **死链**：FTA 模块及关联文件（06-FTA故障树 全目录 + 19-故障诊断 入口 + 09-多故障场景 + 本维护记录）0 失效；SUMMARY.md 55 处修复目标全部验证存在；08-技能体系 修复后的 skill_id/路径引用目标全部验证存在（fta_refs 8 个、knowledge_refs 8 个、cross_refs 全部、21 号 skill-schema/skills-run/skill-set 引用）；最终验证阶段补充修复 skill-set/assessment/dialogue 子目录 5 处死链（N10-补漏，见变更摘要），修复后 08-技能体系全目录 wiki 链接 0 失效（排除代码片段与 Obsidian 短名链接的全量复核）；
3. **旧路径**：`FTA 故障树`（路径形式）、`19-故障诊 断`、`故障诊断/topic-`、`00-core-troubleshooting`、`06-FTA 故障`、`07-pvc-storage-failure`（活跃内容）、`pvc-storage-failure 2`（活跃内容）、`19-19-故障诊断`、`23-实体/kubernetes.md`（根级旧路径）等模式全局扫描清零（37-归档 历史快照与 .claude 生成产物除外，见遗留事项）；
4. **统计口径**：README/MOC/fta-index 统一 110 篇（39 顶层 + 48 组件 + 23 术语）；list/ 48 个组件树与物理文件一致（50 − README/index）；glossary/ 23 卡片与物理文件一致（24 − index）；08-技能体系 README 全景表 27 条与物理文件一一对应（28 编号文件 − 21 号 Demo 指南计 20 行，07 副本已删）；
5. **元数据**：appendix-d/appendix-a frontmatter 修复（YAML 校验通过）；80 处 HTML 注释转换后闭合校验通过、正文残留 0；08-技能体系 28 文件 description/summary 无 wiki 链接/无截断，`---` 围栏结构（3 个）验证通过，skill_id 语义化唯一无冲突（27 号 SKILL-CP-002、28 号 SKILL-HELM-001 与正文声明对齐）；
6. **渲染**：`## 相关文档` 等 4 处标题锚点恢复（`fta-execution-engine.md#修复执行控制器` 引用可用）、表格/列表无注释污染；README 孤立围栏删除、待开发表更新；metadata-enhanced.json JSON 校验通过。

---

## 五、关联文档

- 评审依据: [FTA 故障树完整索引](file:///Users/allengaller/Documents/GitHub/kudig-io/kudig-database/19-故障诊断/06-FTA故障树/fta-index.md)、[FTA 专题 README](file:///Users/allengaller/Documents/GitHub/kudig-io/kudig-database/19-故障诊断/06-FTA故障树/README.md)
- 项目报告索引: [36-报告/README.md](file:///Users/allengaller/Documents/GitHub/kudig-io/kudig-database/36-报告/README.md)

<!-- risk-assessed -->
