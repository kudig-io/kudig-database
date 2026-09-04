---
version: 1
slug: "gtm-index-html"
primary_target: "GTM/index.html"
related_targets: []
---

# GTM Surface Brief — KUDIG Database

## Scope & Mode

单页静态 Go-to-Market 落地页（GTM/index.html），Persuade 模式。三重职能兼顾：外部获客（K8s/平台工程师与决策者）、GTM 策略展示、开源社区招募。纯静态 HTML/CSS/JS，无构建步骤。

## Audience / Job / Action / Proof

- 受众：生产环境 K8s 运维工程师、SRE Lead、开源贡献者
- 任务：数秒内理解「这是什么、规模多大、为何可信」，然后进入知识库站点或 GitHub
- 动作：进入知识库（主 CTA）/ GitHub Star（次 CTA）/ 按场景换乘进入域文档
- 证明：真实规模数字（4,750 docs / 21 域 / 13 云厂商）、真实 FTA 顶事件索引、真实 Makefile 语料命令、Apache 2.0

## Chosen Direction & Memorable Moment

方向：Solari 机械翻牌出发信息板（seed e7e7f1b0，批准构图 .impeccable/mocks/comp-a.png）。
记忆点：首屏整面翻牌墙逐行翻入，域数据行周期性机械翻动；琥珀发光站台号 CTA。

## Sampled Palette（comp-a 像素采样）

- ground/page: #0E0E0D；nav: #181817；tile face: #1C1A14；seam: #000000
- amber fill/CTA: #EFA206→token #F5A81C；glyph amber max #FDE141→token #F7B32B（hi #FFC53D）
- red status/ticker: #EA340E→token #E23B1E
- cream（层压卡，来自同轮 comp 家族）: #E8E4DC；muted: #8A8578

## Type Ramp

- 拉丁展示/翻牌：Oswald 500-700（condensed grotesque，车站 signage 血统），wide letterspacing 大写
- 数据/等宽：Chivo Mono 400-700
- 中文：Noto Sans SC 400/500/700/900；系统 PingFang SC 回退

## Component Grammar & Mediums

| 元素 | 语法 | 介质 |
|---|---|---|
| 翻牌 tile | 2px 圆角、中缝 1px、内阴影、琥珀字 | semantic HTML/CSS |
| 板框 | 哑光铝框 4px 外缘 + 铆钉 | CSS |
| 数据行 | grid: 编号/域名/计数/状态 | HTML/CSS，JS 生成 tiles |
| 发光 CTA | 6px 圆角琥珀填充 + 外发光脉冲 | CSS |
| ticker | 全宽琥珀滚动条，红色状态段 | CSS marquee |
| 层压站台卡 | cream 面板、琥珀头带、四角铆钉、10px 圆角 | HTML/CSS + 内联 SVG 列车标 |
| FTA 树 | TE-3 → OR → IE-3.1/3.2/3.3，点击高亮路径 | 内联 SVG + JS |
| GTM 时刻表行 | 翻牌行语法复用，状态 DEPARTED/ON TIME/BOARDING | HTML/CSS |

密度承诺：首屏标题行 15 tiles + 8 数据行 × ≈26 tiles ≈ 220 tiles，占首屏 ≥70% 面积。
圆角语言：tiles 2px / 按钮 6px / 卡片 10px。线重：seam 1px、行框 2px、板外框 4px。

## Constraints

- 不虚构客户、benchmark、定价；演示数据（FTA 交互）取自仓库真实索引
- prefers-reduced-motion 全量降级
- 无发货 raster（comp 仅为基准参考，不随页面发货）→ 无 provenance 欠账

## Open Decisions

- 无
