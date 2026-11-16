---
title: Console 数据契约（Data Contract）
version: 2.0
status: draft
created: '2026-11-10'
last_updated: '2026-11-10'
category: spec
tags:
- console
- data-contract
- extraction
audience:
- 数据工程
- 前端
- AI Agent
---

# Console 数据契约 v2

> 结构视图里出现的一切，都必须能回答"这条结论来自哪一页的第几节"。本文定义四层派生数据的**格式、生成规则、回溯方式与校验方法**。任何一条无法回溯的数据，视为缺陷而非数据。

## 1. 分层总览

```
原始语料（Markdown）
   │  L0 页级（v1 已有）
   ▼
index.json     4,815 页：title / partition / domain / layers / tier / 计数 / out-links
   │  L1 块级（新增）
   ▼
blocks.json    每页的 H2/H3 大纲 + 表格 + 代码块 + 图，全部带行号区间
   │  L2 实体 / 维度（新增）
   ▼
entities.json  dimensions.json    受控词表
   │  L3 关系级（新增）
   ▼
triples.json   {page, entity, dimension, anchor, evidence}
   │  L4 视图级（前端计算）
   ▼
构成剖面 / 拆解树 / 一致性热力 / 对比矩阵 / 关系邻域 / 演化时间线
```

**分层原则**：下层只依赖上层；视图永远不直接读原文，只读 `triples.json` 与 `blocks.json`。

---

## 2. L0 页级：`data/index.json`（已存在，向后兼容）

```json
{
  "schema": "kudig-console/3",
  "pages": [{
    "id": "wiki/07-数据库中间件/01-数据库/17-mysql-kubernetes-production-guide",
    "t": "MySQL on Kubernetes 生产指南",
    "p": "wiki", "d": "wiki/07-数据库中间件", "dn": "07 数据库中间件",
    "layers": "1111111",        // L0..L6 落锁位串
    "tier": "core", "cat": "database-middleware", "diff": "advanced",
    "date": "2026-07", "n": 402, "sec": 8,
    "dm": 2, "tb": 41, "fence": {"bash": 12, "yaml": 9}, "links": 13, "bad": 0,
    "out": ["wiki/07-数据库中间件/...", "..."], "in": 3
  }]
}
```

| 字段 | 类型 | 含义 | 稳定性 |
|------|------|------|--------|
| `id` | string | 相对仓库根的无扩展名路径，全局唯一 | 不变 |
| `layers` | 7 位 string | L0–L6 是否落锁 | 随判据版本变化，判据版本见 `coverage.json.meta` |
| `out` / `in` | string[] / int | 库内 wikilink 出链（已解析为页面 id）与入链数 | 每次重建重算 |
| `dm` `tb` `links` | int | 图 / 表格行 / 出链计数 | 重建重算 |

**禁用**：视图不得把 `layers` 当作"质量分"展示，它只表示"结构段落是否成节"。

---

## 3. L1 块级：`data/blocks.json`（新增）

### 3.1 目标

让"一页的骨架"成为一等数据：章节树、每节承载的表格/代码/图、以及每节的**行号区间**（回溯锚点）。

### 3.2 格式

```json
{
  "schema": "kudig-console-blocks/1",
  "generatedFrom": {"indexSha": "c2df8d7b…", "builder": "console-corpus/3"},
  "pages": {
    "wiki/07-数据库中间件/01-数据库/17-mysql-kubernetes-production-guide": {
      "lineCount": 402,
      "shape": "ops-heavy",
      "sections": [
        { "level": 2, "index": 1, "title": "适用场景与范围", "dimension": "l0-scope",
          "start": 41, "end": 63, "tables": 0, "codeBlocks": 0, "diagrams": [], "bullets": 7 },
        { "level": 2, "index": 4, "title": "标准操作流程", "dimension": "l3-operation",
          "start": 120, "end": 260, "tables": 1, "codeBlocks": 9, "diagrams": ["…/page#0"], "bullets": 22,
          "children": [
            { "level": 3, "index": 1, "title": "3.1 高可用拓扑", "start": 126, "end": 168,
              "tables": 0, "codeBlocks": 2, "diagrams": [], "bullets": 4 }
          ] }
      ],
      "tables": [
        { "sectionIndex": 4, "line": 48, "headers": ["维度", "MySQL", "TiDB"], "rows": 6,
          "kind": "comparison", "anchor": "L48" }
      ],
      "codeBlocks": [
        { "sectionIndex": 4, "line": 132, "lang": "bash", "lines": 18, "risk": "unknown" }
      ]
    }
  }
}
```

### 3.3 字段规则

| 字段 | 规则 |
|------|------|
| `shape` | 页形分类，规则见 §3.4，**必须在页面上展示**，因为它决定了该页适合哪种视图 |
| `sections[i].dimension` | 由 §4 维度词表匹配得到；匹配不到写 `null`，**不猜** |
| `tables[].kind` | `comparison`（首列是实体/维度且 ≥2 数据列）/ `lookup`（键值）/ `checklist`（✅❌ 为主）/ `prose-table`（兜底） |
| `tables[].anchor` | `L<line>`，指向该表第一行在源文件中的行号（**1-based，含 frontmatter 偏移后的正文行号**） |
| `diagrams[]` | 复用 L3 图表 id（`<page>#<n>`），与 `content/*.json` 对齐 |
| `risk` | 代码块风险占位，暂为 `unknown`；后续可接 `annotate-command-risk.py` |

### 3.4 页形分类规则（可解释、无模型）

按优先级匹配，命中即停：

| 页形 | 规则 | 实测（`07 数据库中间件`，60 页） |
|------|------|------|
| `index-like` | 落锁层 = 0 且出链 > 10 | 1 |
| `prose` | H3 = 0 且代码块 < 3 | 9 |
| `table-heavy` | 表格 ≥ 2 且代码块 < 6 | 1 |
| `ops-heavy` | 代码块 ≥ 8 | **49** |
| `structured` | 其余 | 0 |

> 这个分布说明该域几乎全是"操作手册"——这本身就是一条可展示的结构结论，而不是缺陷。

---

## 4. L2 受控词表

### 4.1 `data/dimensions.json`

维度 = 页面里反复出现的"论题轴"。它让不同页面可比。

```json
{
  "schema": "kudig-console-dimensions/1",
  "version": "2026.11",
  "dimensions": [
    { "id": "l0-scope",   "name": "适用场景", "patterns": ["适用场景", "范围", "概述"], "class": "layer" },
    { "id": "l2-arch",    "name": "架构",     "patterns": ["架构", "拓扑", "组件总览"], "class": "layer" },
    { "id": "l3-operation","name": "操作流程","patterns": ["标准操作", "部署", "操作流程", "实施"], "class": "layer" },
    { "id": "l4-check",   "name": "检查点",   "patterns": ["检查点", "验证", "巡检"], "class": "layer" },
    { "id": "l5-fault",   "name": "故障排查", "patterns": ["故障", "排查", "排障", "troubleshoot"], "class": "layer" },
    { "id": "l6-ref",     "name": "参考",     "patterns": ["参考", "runbook", "延伸"], "class": "layer" },
    { "id": "d-monitor",  "name": "监控告警", "patterns": ["监控", "告警", "指标"], "class": "operation" },
    { "id": "d-backup",   "name": "备份恢复", "patterns": ["备份", "恢复", "pitr", "wal-g"], "class": "operation" },
    { "id": "d-ha",       "name": "高可用",   "patterns": ["高可用", "故障转移", "failover", "副本集", "节点替换"], "class": "operation" },
    { "id": "d-pool",     "name": "连接池",   "patterns": ["连接池", "pgbouncer", "proxysql"], "class": "operation" },
    { "id": "d-perf",     "name": "性能调优", "patterns": ["性能", "调优", "慢查询"], "class": "operation" },
    { "id": "d-scale",    "name": "扩缩容",   "patterns": ["扩缩容", "扩容", "scale"], "class": "operation" },
    { "id": "d-storage",  "name": "存储",     "patterns": ["存储", "持久化", "pvc", "容量规划"], "class": "operation" }
  ]
}
```

**规则**：

1. `patterns` 只做小写包含匹配。
2. `class` 决定它属于"层维度"（L0–L6，与落锁直接对应）还是"运维维度"（跨层，用于对比矩阵）。
3. **H2 与 H3 都要匹配**（实测教训：`07` 域"备份"在 H2 命中 0、在 H3 命中 4/4）；命中 H2 记 `strength: section`，只命中 H3 记 `strength: subsection`，视图必须可见地区分两者。
4. 同义异写不合并（如"故障排查" vs "故障与 remediation"），各自保留，由 `group` 在视图层归组。

### 4.2 `data/entities.json`

> **已落库**：词表源文件是 `console/spec/entities.json`（11 组 / 39 实体），生成器解析成 `console/data/graph/entities.json`（每个实体挂上它旗舰页的维度小节）。下面是格式说明。

```json
{
  "schema": "kudig-console-entities/1",
  "version": "2026.11",
  "entities": [
    { "id": "mysql",  "name": "MySQL",  "group": "database", "aliases": ["mysql", "innodb cluster", "pxc", "percona xtradb"] },
    { "id": "postgresql", "name": "PostgreSQL", "group": "database", "aliases": ["postgresql", "postgres", "pg", "cloudnativepg", "patroni"] },
    { "id": "tidb",   "name": "TiDB",   "group": "database", "aliases": ["tidb", "tikv", "pd", "tiflash"] },
    { "id": "mongodb","name": "MongoDB","group": "database", "aliases": ["mongodb", "mongo", "replicaset"] },
    { "id": "redis",  "name": "Redis",  "group": "cache",    "aliases": ["redis", "sentinel"] }
  ]
}
```

**规则**：

1. 同义词必须来自实测（在语料中出现过的写法），不得凭空扩展。
1a. **每个实体必须能解析到一页**（`pageHints` 按顺序取第一个命中的 page id）。解析不到的实体保留但标 `page: null`，UI 显示"仅 FTA"，不得静默指向别页。
1b. **实体覆盖度要可见**：`entities.json` 同时给出 `dimensionIndex`（实体 → 维度 → 行号），矩阵据此计算 `n/维度组数` 徽章。
2. `group` 用于"同组才能对比"的约束——不允许把 MySQL 与 Kubernetes 放进同一对比矩阵。
3. 实体 id 一旦发布不得改名（视图与 URL 依赖）。

---

## 5. L3 关系级：`data/triples.json`（新增）

```json
{
  "schema": "kudig-console-triples/1",
  "version": 1,
  "triples": [
    { "page": "wiki/07-…/17-mysql-kubernetes-production-guide", "entity": "mysql",
      "dimension": "d-backup", "anchor": "L226", "sectionIndex": 4, "sectionTitle": "4.3 备份",
      "evidenceLines": 14, "source": "heading" }
  ]
}
```

| 字段 | 说明 |
|------|------|
| `anchor` | `L<n>`：该维度在原文中的起始行 |
| `source` | `heading`（标题命中）/ `table`（表格里出现）/ `strong`（正文加粗行命中）；**只有这三种**，保证可解释 |
| `group` | 维度组 id（对比矩阵按组对齐，如"拓扑 + 故障转移 → `ha`"）；见可视化规范 §4.1 |
| `mappingNote` | 该小节与维度组名不一致时的说明（如"TiKV 节点替换 ⇒ 高可用"）；一致时为 `null` |
| `strength` | `section`（H2 命中）或 `subsection`（仅 H3 命中）；视图用徽章区分，不可混同 |
| `evidenceLines` | 命中段落行数；用于热力格"专节 vs 提及"的判定（≥3 行为"专节"） |

### 5.1 回溯协议

任何结构元素点击后，打开证据需带：

```
#read/<page-id>?at=L226&hl=d-backup
```

阅读页据此滚动到该行并高亮。**没有 `at` 参数的证据视为无效。**

---

## 6. 一致性热力的判定（示例，可逐格复核）

以 `07 数据库中间件` 的 4 个产品页为准（实测）：

| 维度 | MySQL | PostgreSQL | TiDB | MongoDB | 判定依据 |
|------|:-----:|:----------:|:----:|:-------:|----------|
| 备份恢复 | ● | ● | ● | ● | 4 页均有专节 |
| 监控告警 | ● | ● | ● | ● | 4 页均有 `监控` 节 |
| 高可用 | ● | ● | ● | ○ | MongoDB 写在 3.4 子节（`提及`） |
| 故障转移 | ● | ● | ● | ○ | 同上 |
| 连接池 | ● | ● | ● | ○ | MongoDB 未成节 |
| 性能调优 | ● | ○ | ● | ○ | 只 2 页成节 |
| 扩缩容 | ○ | ○ | ● | ○ | 只 TiDB 成节 |

> ● 专节（≥3 行）  ○ 提及（1–2 行）  – 未出现（琥珀虚线）
> 这张表就是视图要渲染的东西；它不是手工画的，而是 `triples.json` 的一个投影。

**维度组映射示例（实测，必须记录）**：TiDB 页 `12-tidb-kubernetes-production` 没有名为"故障转移"的 H3，其高可用由 `5.1 水平扩缩容` 与 `5.3 TiKV 节点替换` 承担：

```json
{ "page": "wiki/07-…/12-tidb-kubernetes-production", "entity": "tidb", "group": "ha",
  "dimension": "d-ha", "anchor": "L5.3", "sectionTitle": "5.3 TiKV 节点替换",
  "mappingNote": "TiDB 无独立故障转移节；节点替换 + 扩缩容承担该职责", "source": "heading" }
```

不记录这条映射，矩阵会把 TiDB 误判为"缺失高可用"——**这是最容易产生错误结论的位置，必须在数据层显式记录**。

---

## 7. 生成与校验

### 7.1 生成

```bash
make console-sync        # 重建 index → blocks → triples
make console-check       # CI：逐字节比对所有派生文件
```

**顺序约定**：`index.json` → `blocks.json` → `triples.json`；每层生成后计算 `sha256` 写入下一层的 `generatedFrom`，形成可追溯链。

### 7.2 校验（每次 CI）

| 校验 | 规则 | 失败后果 |
|------|------|----------|
| V1 锚点存在性 | 每条 triple 的 `anchor` 行必须存在且非空 | 阻断合并 |
| V2 章节归属 | `sectionIndex` 对应的 H2 必须存在 | 阻断合并 |
| V3 实体合法 | `entity` 必须在 `entities.json` 中 | 阻断合并 |
| V4 维度合法 | `dimension` 必须在 `dimensions.json` 中 | 阻断合并 |
| V5 无孤儿图 | 视图引用的每个结构 id 必须能在 `blocks.json` 找到 | 阻断合并 |
| V6 体积阈值 | `blocks.json` ≤ 40MB，`triples.json` ≤ 15MB | 告警 |

### 7.3 手工抽检

每次发布前，随机抽 20 条 triple 人工核对原文，一致率必须 **100%**；低于则回退该批抽取。

---

## 8. 版本与兼容

- `schema` 字段标示格式版本；视图必须**拒绝**未知 schema，而不是降级猜。
- `dimensions.json` / `entities.json` 的 `version` 独立演进（词表变更是常态），变更需在 PR 中列出"新增/合并/删除"三类并给出理由。
- `layers` 判据变更会改变历史数据，需在 `coverage.json.meta.criteriaVersion` 记录。

## Related

- [[console/spec/PRD|PRD]]
- [[console/spec/visualization-spec|可视化规范]]
- [[console/spec/acceptance|验收与度量]]
- [[console/README|Console 说明]]