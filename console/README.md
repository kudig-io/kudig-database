---
title: Console 云原生知识体系工作台（全库）
description: console/ 分区说明——把 KUDIG Database 全库 4,815 页、86 个知识域与 2,277 张 Mermaid 图收进一个离线工作台：七层分层谱、S0–S7 结构图、图表库与逐字原文阅读。
summary: console/ 是全库工作台：四分区 86 个域 4,815 页，七层知识栈的分层谱、S0–S7 结构带与库图/层级谱、2,277 张 Mermaid 图的本地渲染与图表库，以及每页的逐字原文。数据由 console-corpus.py 从仓库生成，CI 校验不漂移。
category: references
tags:
- console
- workbench
- full-corpus
- knowledge-management
- frontend
- static-site
- mermaid
- visualization
tier: supporting
created: '2026-11-09'
last_updated: '2026-11-10'
difficulty: beginner
audience:
- 平台工程师
- 知识体系设计者
- AI Agent
estimated_read_time: 8min
intent_queries:
- console 工作台是什么
- 全库有多少页多少图
- 七层知识栈全库覆盖如何
- Mermaid 图在哪里看
- 工作台数据如何更新
---

# Console 云原生知识体系工作台（全库）

> `console/` 是**整个知识库的工作台**：把 `wiki/`、`skills/`、`maintaining/`、`code/` 里的全部正文页收进一个离线可用的界面，既做结构可视化，也做原文阅读。

## 1. 全库口径

| 维度 | 数量 | 说明 |
|------|-----:|------|
| 页面 | **4,815** | 所有内容 Markdown（不含索引跳转与归档） |
| 知识域 | **86** | wiki 30 · skills 43 · maintaining 3 · code 10 |
| 正文行 | **2,288,685** | 不含 frontmatter |
| Mermaid 图 | **2,277** | 其中 2,277 张经引擎校验可渲染；运行期个别失败时原文照常呈现 |
| 表格行 | **123,028** | Markdown 表格行 |
| 库内链接 | **37,113** | `[[wikilink]]` 解析到库内页面的边 |
| 孤儿页 | **1,450** | 没有任何库内页面指向它 |

**排除范围（写死在一处）**：`code/repo/`（上游源码检出，7,845 个文件，由 `make repo-sync` 重建）、`wiki/31-归档`、`03-发布`、`04-报告`、`wiki/27-资产`、点目录。

## 2. 四个视图

| 视图 | 快捷键 | 回答的问题 |
|------|:------:|-----------|
| **总览** | `1` | 四分区与每个域在七层上落锁了多少页；点分区进入该分区的域清单 |
| **结构图** | `2` | S0–S7 结构带（根/总览/深入/未组织/空/0-7/6-7/7-7）、库图（分区→域→页）、层级谱（每个域的七层剖面）与对称性 |
| **图表库** | `3` | 2,277 张 Mermaid 图按类型（流程图 1,898 / 时序图 245 / 状态图 44 / 思维导图 41 …）与域筛选，点一行就地渲染该页全部图 |
| **阅读** | `4` | 任意页的逐字原文：图在正文原位渲染，代码块保留语言，表格可横向滚动，右侧是层级、编码、计数与目录 |

全局：`⌘K` 或 `/` 搜索 4,815 页；`Esc` 关闭；手机端（≤760px）侧栏变抽屉，≤420px 用短标签。

## 3. 七层知识栈（全库覆盖）

判据是**页面自身的一级/二级标题**是否命中该层关键词，所以可逐页复核：

| 层 | 名称 | 判据 | 全库落锁 | 缺口页 |
|----|------|------|--------:|-------:|
| L0 | 总览 | 适用场景 / 核心职责 | 87 | 4,728 |
| L1 | 概念 | 核心概念 / 术语 | 882 | 3,933 |
| L2 | 架构 | 架构 / 拓扑 / 信息流 | 1,387 | 3,428 |
| L3 | 操作 | 标准操作 / 部署 / 步骤 | 1,001 | 3,814 |
| L4 | 检查 | 检查点 / 验证 / 监控 | 632 | 4,183 |
| L5 | 故障 | 故障 / 排查 / 排障 | 1,567 | 3,248 |
| L6 | 参考 | 参考 / Runbook / 阅读 | 1,377 | 3,438 |

全库只有 **20 页七层齐备**，**1,414 页一层都没落锁**——这正是「结构图」里 S7 与 S5 两条带的含义，也是下一步最该动的两块。

> L4 偏低是判据严格造成的（标题里要成段出现「检查点 / 验证 / 监控指标」）。工作台对判据前后一致、不做宽窄切换。

## 4. Mermaid 怎么渲染

- 用随仓库分发的 `console/vendor/mermaid.min.js`（Mermaid 11.14，Apache-2.0，见 `vendor/LICENSE-mermaid`）在浏览器里**本地渲染**，不请求任何 CDN，离线可用。
- 构建时对每个 ```` ```mermaid ```` 块做一次引擎校验，结果写进 `data/mermaid-validation.json`；可渲染的块以「代码 + 类型」进 `content/`，不可渲染的块保留原文并在页面上标为「源码」。
- 源码里写作 `xychart-beta` / `venn-beta` 的图，页面标签会注明「源码写作 …」，渲染用引擎支持的写法。
- 两种渲染证据：阅读页里图在正文原位出现；图表库里点任一页可就地看该页全部图。

## 5. 数据与目录

```text
console/
├── index.html                # 单页工作台
├── assets/console.{css,js}   # 设计令牌、渲染与路由
├── vendor/mermaid.min.js     # 本地 Mermaid 引擎
├── data/
│   ├── index.json            # 4,815 页：标题、层、编码、计数、出链
│   ├── domains.json          # 分区 → 域 → 页数
│   ├── graph/diagram.json    # 图类型、域分布、高产页、破损清单
│   ├── graph/coverage.json   # 域 × 层矩阵与每层缺口清单（截断 200）
│   ├── graph/structure.json  # 分区、tier、类别、枢纽、孤儿
│   └── mermaid-validation.json  # 每个 Mermaid 块的引擎校验结果
├── content/
│   ├── index.json            # 页 → 段落切片（仅供脚本与调试）
│   ├── <page-id>.json        # 单页切片：t 段落 / c 代码 / m 图 / d 降级图
│   └── …
└── README.md
```

`data/` 与 `content/` 都是**派生产物**，由 `make console-sync` 重新生成，不要手改。

## 6. 本地运行

```bash
make console-serve     # http://127.0.0.1:8766/（推荐）
make console-sync      # wiki 变更后重建 data/ 与 content/
make console-check     # CI：校验派生物未漂移
```

`file://` 直接打开会被浏览器拦截 `fetch`，必须经 `console-serve`。

## 7. 设计方案（未实现）

`console/spec/` 记录从"读语料"转向"拆解语料"的设计方案；其中 M1 已有可运行原型（`prototype/`），其余仍为文档：

| 文档 | 内容 |
|------|------|
| [设计方案总览](spec/index.md) | 文档地图、与现有实现的关系、阅读顺序 |
| [PRD](spec/PRD.md) | 定位、用户、六个结构视图、数据契约摘要、分期与度量 |
| [数据契约](spec/data-contract.md) | 四层派生数据格式、锚点规则、校验方法 |
| [可视化规范](spec/visualization-spec.md) | 六个视图的画法、读法、禁忌、文字规范 |
| [交互与无障碍](spec/accessibility.md) | 抽屉模型、键盘、图形等价、性能预算 |
| [验收与度量](spec/acceptance.md) | 三层验收、回归策略、发布门禁 |
| [路线图](spec/roadmap.md) | M0–M4 里程碑、工作分解、回退条件 |
| [走查样例](spec/walkthrough.md) | 以 07 域把六个视图走一遍 |
| [决策记录](spec/decisions.md) | 12 条已定口径与 4 个未决问题 |
| [维度词表](spec/labels.md) | 维度定义、标注流程、实测基线 |
| [结构视图原型](prototype/README.md) | **已可运行**：拆解树 / 热力 / 矩阵 + 锚点门禁 |

## 8. 已知边界

- 解析是**结构级**而非语义级：七层只看标题判据，不判断内容质量；「未组织正文」不等于「无用」。
- 单页内容按需加载；图表库里一次只渲染选中页的所有图，避免 2,277 张同时排队。
- 运行期个别 Mermaid 图仍可能因引擎版本差异失败，失败处显示源码与原因，不伪造渲染结果。
- `code/repo/` 不属于正文，不在工作台内。

## Related

- [[README]]
- [[maintaining/PRODUCT|Product]]
- [[maintaining/DESIGN|Design System]]
- [[wiki/17-故障诊断/README|故障诊断]]
- [[wiki/05-网络/README|网络]]