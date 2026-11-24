---
name: KUDIG Console (full library)
description: 全库知识体系工作台——抽样册世界 + 三种结构图语法（分层谱 / S0–S7 结构带 / 库图），白纸 + IBM 蓝 + 一色琥珀
colors:
  paper: "#FFFFFF"
  vellum: "#FAFAF8"
  mist: "#F2F1EC"
  ink: "#0D0D0D"
  ink-2: "#2B2B2E"
  muted: "#55555C"
  muted-2: "#6E6E75"
  rule: "#D6D5CD"
  hairline: "#E8E7E0"
  blue: "#1E40AF"
  blue-deep: "#16348F"
  blue-hi: "#1D4ED8"
  blue-wash: "#EFF3FE"
  blue-line: "#C9D6F8"
  amber: "#B86E0A"
  amber-text: "#7D4A05"
  amber-wash: "#FCF5EA"
  amber-line: "#E6D3B0"
  and-gate: "#93A9E8"
  on-ink-muted: "#C9C9CF"
typography:
  display:
    fontFamily: "Libre Franklin, Helvetica Neue, PingFang SC, sans-serif"
    fontWeight: 600
    fontSize: "25px"
    lineHeight: 1.2
    letterSpacing: "-0.02em"
  body:
    fontFamily: "PingFang SC, Hiragino Sans GB, Noto Sans SC, sans-serif"
    fontWeight: 400
    fontSize: "13px"
    lineHeight: 1.6
  reader:
    fontFamily: "PingFang SC, Hiragino Sans GB, Noto Sans SC, sans-serif"
    fontWeight: 400
    fontSize: "14px"
    lineHeight: 1.78
  title:
    fontFamily: "Libre Franklin, Noto Sans SC, sans-serif"
    fontWeight: 600
    fontSize: "22px"
    lineHeight: 1.3
    letterSpacing: "-0.015em"
  panel:
    fontFamily: "Libre Franklin, Noto Sans SC, sans-serif"
    fontWeight: 600
    fontSize: "15px"
    lineHeight: 1.35
  note:
    fontFamily: "Noto Sans SC, PingFang SC, sans-serif"
    fontWeight: 400
    fontSize: "12px"
    lineHeight: 1.6
  data:
    fontFamily: "IBM Plex Mono, SF Mono, Menlo, monospace"
    fontWeight: 500
    fontSize: "11px"
    lineHeight: 1
    letterSpacing: "0.06em"
  chart-label:
    fontFamily: "IBM Plex Mono, SF Mono, Menlo, monospace"
    fontWeight: 500
    fontSize: "10px"
    lineHeight: 1
rounded:
  none: "0"
  hairline: "1px"
  max: "2px"
spacing:
  rail: "300px"
  topbar: "48px"
  gutter: "30px"
  panel-pad: "14px"
  grid-gap: "14px"
components:
  topbar-view-active:
    backgroundColor: "{colors.paper}"
    textColor: "{colors.blue}"
    padding: "0 15px"
  nav-part:
    backgroundColor: "{colors.vellum}"
    textColor: "{colors.ink}"
    padding: "7px 16px"
  nav-domain-active:
    backgroundColor: "{colors.blue-wash}"
    textColor: "{colors.blue}"
    padding: "5px 16px"
  spectrum-cell-full:
    backgroundColor: "{colors.blue}"
    textColor: "{colors.paper}"
    size: "1fr x auto"
  spectrum-cell-partial:
    backgroundColor: "{colors.blue-wash}"
    textColor: "{colors.blue}"
    size: "1fr x auto"
  spectrum-cell-gap:
    backgroundColor: "{colors.amber-wash}"
    textColor: "{colors.amber-text}"
    rounded: "{rounded.hairline}"
  band:
    backgroundColor: "{colors.paper}"
    textColor: "{colors.ink}"
    rounded: "{rounded.none}"
    padding: "0"
  diagram-frame:
    backgroundColor: "{colors.paper}"
    textColor: "{colors.ink}"
    rounded: "{rounded.none}"
    padding: "12px"
  chip-btn-active:
    backgroundColor: "{colors.ink}"
    textColor: "{colors.paper}"
    rounded: "{rounded.max}"
    padding: "6px 10px"
---

# Design System: KUDIG Console (full library)

## Overview

工作台的视觉世界仍是**抽样册**：白纸、墨线框、IBM 蓝、发丝线、圆角上限 2px。全库版新增的是一套**结构图语法**——把 4,815 页读成一棵树、一条谱和一个带状分布，而不是一份列表。

三种语法各自对应一个真实问题：

1. **分层谱（Spectrum）**：一个单元格 = 一个层在一个分区/域上的覆盖。实蓝 = 全库该层对齐，浅蓝 = 部分页对齐，琥珀虚框 = 零对齐。回答「这份库厚在哪一层」。
2. **S0–S7 结构带（Structural Bands）**：按页在库里的位置与七层落锁数分八条带，每条带用一根比例轨 + 一列页名 chip。回答「哪些页其实还没成文」。
3. **库图与层级谱（Library Tree / Layer Spectrum）**：纯 SVG。库图是分区→域→页的三级树，页面方块按落锁数着色；层级谱把每个域的七层剖面画成一行色块。两者都不引入新色，只复用蓝/琥珀。

**Key Characteristics**

- 数据一律**等宽字体 + tabular-nums**；叙述一律 PingFang/思源黑；标题轻，框线重。
- 蓝色只表示「已对齐/已落锁/已选中」；琥珀只表示「缺口/未组织」。没有任何红色。
- 0 条网格阴影、0 个圆角卡片堆叠；分隔靠 1px 线与底色差。
- 图表本身（Mermaid）是**内容**，不进品牌色：用 Mermaid `base` 主题，只把主色映射到蓝、注释映射到琥珀。

## Colors

白纸 `#FFFFFF` 为内容面，`#FAFAF8` 为侧栏与面板底。墨 `#0D0D0D` 画框。IBM 蓝 `#1E40AF` 表「已对齐/已落锁/当前选中」，浅蓝 `#EFF3FE` 是它的底色版本。琥珀 `#B86E0A` 只表缺口，文字版本 `#7D4A05` 保证 4.5:1 以上。

### Named Rules

**The One Gap Colour Rule.** 琥珀只标缺口；任何「警告/错误」都用文字说清楚，不另引颜色。

**The Measured Blue Rule.** 蓝色出现处都意味着「已经对上」：命中、落锁、选中、覆盖。蓝色不是装饰。

## Typography

系统字体栈：`Libre Franklin`（若已安装）→ `Helvetica Neue` → `PingFang SC`；等宽 `IBM Plex Mono` → `SF Mono` → `Menlo`。不加载任何 Web 字体——全库 2,277 张图 + 4,815 页的渲染不应依赖网络。

层级：视图标题 25px/600 → 面板标题 12.5px/600 + 面板名 11px Mono → 数据 11px Mono → 正文 13–14px。层级谱 SVG 里的百分比标签允许 10px（`chart-label`），这是图形内标注，不是正文。

### Named Rules

**The Measurement Voice Rule.** Mono 只给数据、编号、路径、命令与度量。叙述文字永远不打等宽。

## Layout

顶栏 48px（品牌 300px 对齐 + 四个视图 + 搜索）。左 300px 侧栏：分区折叠组 → 域（页数 + 图数）。主区：面包屑 → 标题 + 元信息 → 视图标签 → 内容 → 底部索引条（页/域/图/行/链）。

- 分层谱：`200px repeat(7, 1fr) 88px 76px`；≤760px 收为 `104px repeat(7, minmax(22px,1fr)) 44px`。
- 结构带：竖排 8 条带，每条 = 头（编号/标题/页数）+ 比例轨 + chip 墙（最多 60 个 + 余量）。
- 库图：SVG 宽 1000，横向滚动；层级谱：SVG 宽 1000，7 列。
- 阅读：`minmax(0,1fr) 300px`，右栏 `position:sticky`，≤1180px 折到正文下方两列。

## Elevation & Depth

**只有一种抬升**：命令面板 `0 12px 32px rgba(13,13,13,.22)`；抽屉 `0 0 24px rgba(13,13,13,.14)`。内容面、面板、带、谱、树**一律不使用阴影**——墨框就是布尔层级。

### Named Rules

**The Inked Frame Rule.** 一个面只声明一次边界：要么墨框，要么投影，永不叠加。

## Shapes

圆角上限 2px（刻度格 1px）。所有度量都是矩形：谱格、带轨、chip、色块。分隔线两种：1px 发丝（内部）与 1px 墨线（面板外框）。

## Components

### 分层谱 Spectrum
表头是七个层（编号 + 名称），每行是一个分区或域：七个覆盖格 + 落锁合计 + 覆盖率条。格内数字是该层在该行已落锁的页数；`–` 表示零。

### 结构带 Bands
`S0 根与索引 / S1 总览页 / S2 深入页（≥3 层）/ S3 未组织正文（0–2 层）/ S4 空目录 / S5 0/7 / S6 6/7 / S7 7/7`。每条带有页数、占比、比例轨与可点击 chip。

### 库图 Library Tree
三级 SVG 树：分区（蓝实框）→ 域（墨框，页数右对齐）→ 页（12px 方块，七层齐备=蓝实、≥3 层=墨框、其余=琥珀虚框）。每个方块带 `<title>`，点击进阅读。

### 层级谱 Layer Spectrum
每行一个域，七列色块按覆盖率深浅着色并标百分比；行按覆盖率排序，最稠密在上。

### 图表库 Diagram Gallery
类型 chip 行 + 域 chip 行 + 可点击的页列表（图数、标题、域、主要类型）；点页后就地渲染该页全部 Mermaid 图。

### 阅读 Reader
逐字原文：图原位渲染（带类型与「源码写作」标注）、代码块保留语言、表格可横滚、`[[wikilink]]` 解析为站内链接。右栏：七层小方阵、编码、计数、目录（吸顶）。

## Do's and Don'ts

### Do
- **Do** 每个数字都由 `console-corpus.py` 生成；界面不手写计数。
- **Do** 用层格/带/谱表达结构，用列表表达检索。
- **Do** Mermaid 图用引擎渲染；渲染失败时显示源码与原因。
- **Do** 维持全键盘可达、`prefers-reduced-motion` 关动效、打印只留阅读区。

### Don't
- **Don't** 用深色底、状态灯、大数字英雄区或卡片+图标网格。
- **Don't** 让琥珀变成告警色或第二装饰色。
- **Don't** 用零模糊偏移阴影；一个面只声明一次边界。
- **Don't** 把「部分落锁」读成「缺失」——界面文案必须写清。

## Constraints & known deviations

- 字体走系统栈（不加载 Web 字体），理由是同架构下 CDN 样式表会拖住首屏；`DESIGN.md` 的字体声明以实现为准。
- S4（空目录/仅索引）在当前数据中没有独立可枚举的目录条目，页面按实际内容归入 S0/S3，界面上如实为 0。
- 图表库的类型筛选按**域级**类型分布近似（`diagram.json` 记录的是域内各类型数量，不是逐页）；逐页精确类型在阅读页里显示。
- 单页内容按需加载（`console/content/<page-id>.json` 分片）；曾有的 78MB 单体调试索引 `content/index.json` 运行时从不被加载，2026-11-24 起生成器停止产出且不再入库。