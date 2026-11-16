---
title: Console 结构视图原型
summary: M1 原型——blocks.json 块级抽取 + 拆解树 / 一致性热力 / 对比矩阵三个视图 + 可回溯证据抽屉，附锚点门禁脚本。
category: references
tags:
- console
- prototype
- m1
audience:
- 前端
- 数据工程
- 产品
---

# Console 结构视图原型（M1）

> 按 [路线图](../spec/roadmap.md) 的 M1 范围做的可运行原型：**块级抽取 → 三个结构视图 → 点任意结构元素回到原文那一行**。它不接入主界面（主界面 `console/index.html` 未改动），作为设计验证与数据可信度的证据。

## 1. 怎么跑

```bash
make console-serve            # 起 http://127.0.0.1:8766/
open http://127.0.0.1:8766/prototype/
```

`make console-serve` 现在运行的是 `console/prototype/serve.py`，它比普通静态服务多一个只读桥：

- `/prototype/`、`/data/`、`/content/` — 静态文件
- `/__src/<仓库相对路径>.md` — **只读**返回仓库原文，仅允许 `wiki/` `skills/` `maintaining/` `code/` 下的 `.md`，显式拒绝 `code/repo/`、`.env`、路径穿越
- 用途：证据抽屉与"定位原文"需要读原始 Markdown 并高亮对应行

## 2. 三个视图

| 视图 | 路由 | 它证明什么 |
|------|------|-----------|
| 拆解树 | `#/<域>/tree` | 每页的骨架可被抽取：H2/H3 数、表与代码载荷、七层落锁、首节锚点；点页名进单页 |
| 单页拆解 | `#/<域>/page/<页 id>` | 一页拆成几节、每节跨度多少行、载荷几表几码，全部带 `L起–L止` |
| 一致性热力 | `#/<域>/heat` | 12 个维度 × 域内页面：实底 = H2 成节、浅底 = 仅 H3、虚线 = 未成节；列底部给命中页数 |
| 对比矩阵 | `#/<域>/matrix` | 4 个产品 × 3 个运维维度：每格 = 命中小节 + 行号锚点，空格为语料未写 |

**证据抽屉**：点任意格子/行 → 右侧抽屉给出页面、路径、锚点、小节、来源，并直接显示原文该行起的 8 行；「定位原文」打开全文并高亮该行。

## 2.1 M2：词表外置与全库可用

| 能力 | 状态 | 说明 |
|---|---|---|
| 维度词表外置 | ✅ | `console/spec/dimensions.json`（21 维，v2026.11.1）；生成器与原型都从这里读 |
| H2 / H3 分层 | ✅ | JSON 里 `h2` / `h3` 控制允许的命中层级 |
| 四分区切换 | ✅ | 侧栏顶部可在 语料正文 / Agent 技能卡 / 维护资产 / 可执行资产 之间切换（86 域） |
| 热力自适应列数 | ✅ | 域内稠密（≥35% 页面命中 ≥4 维）时铺全部 21 维，否则取前 14 列 |
| 词表回归门禁 | ✅ | `make prototype-verify` 同时校验锚点与 07 域三行基线 |
| 跨域对比矩阵 | ✅ | `console/spec/entities.json`：11 个实体组、39 个实体，矩阵可切组（数据库 / 缓存 / 消息队列 / CNI / 服务网格 / 网关 / 可观测性 / GitOps / 分布式存储 / 弹性伸缩） |

## 2.2 M3：实体词表与通用对比矩阵

`console/spec/entities.json` 定义 **11 个实体组 / 39 个实体**。每个实体用 `pageHints` 绑定到"它的旗舰页"，生成器据此把该页的维度小节挂到实体上，矩阵不再硬编码产品。

| 实体组 | 成员（括号内为该实体在语料里覆盖的维度组数 / 6） |
|---|---|
| 数据库 | MySQL 6 · PostgreSQL 6 · TiDB 5 · MongoDB 6 |
| 容器网络插件 | Cilium 3 · Calico 0 · Flannel 2 · Terway 2 |
| 分布式存储 | Ceph 5 · OpenEBS 6 · Longhorn 5 · JuiceFS 5 |

矩阵头部每个实体带一个 `n/6` 覆盖徽章（蓝=≥4、灰=1–3、琥珀虚线=0），一眼看出"这家实体在语料里说清了几件事"。

**CNI 组本身就是一条结论**：Cilium 覆盖 3/6，Calico 只有 FTA 树没有任何成节产品页（0/6），Flannel 与 Terway 各 2/6 —— 语料对容器网络的知识结构是**严重偏斜**的，这比"有 101 页提到 Cilium"有用得多。

**两个实体只有 FTA 树**（Calico、Cluster Autoscaler），`entities.json` 里显式写了 `note`，矩阵显示为"仅 FTA"而不是静默错指。

## 3. 数据

原型读 `console/data/graph/blocks.json`（由 `make console-sync` 生成）：

```json
{ "pages": { "<page-id>": {
  "lineBase": "body", "lines": 261, "shape": "table-heavy",
  "outline": [{ "l": 2, "n": 1, "t": "适用场景与范围", "s": 11, "e": 20, "tb": 0, "cd": 0 }],
  "tables": [{ "l": 48, "h2": 3, "h3": 2, "rows": 0 }],
  "codes": [{ "l": 21, "lang": "bash", "n": 18 }],
  "dims": [{ "id": "d-backup", "name": "备份恢复", "strength": "subsection", "line": 135, "hits": 1 }],
  "h2": 8, "h3": 8 } } }
```

- 规模：**4,817 页 · 100,240 个标题节点 · 14MB**（只存锚点与计数，不存正文）
- `lineBase: "body"` 表示行号是**去掉 frontmatter 后的正文行号（1-based）**，与 `console/spec/data-contract.md` §3 一致

## 4. 锚点门禁

```bash
python3 console/prototype/verify-anchors.py
```

做两件事：

1. **锚点**：逐条检查矩阵引用的每个锚点在**源文件对应行**确实是那个标题（H2/H3 均可）——当前 **11/11 通过**（`acceptance.md` 的 D2）。
2. **词表回归**：07 域三行基线（监控 58% / 备份 43% / 高可用 27%，容差 ±15 点）不得偏离；`blocks.json` 中不得出现词表外的维度 id（D4）。

`make console-serve` 与主界面共用同一个端口：原型在 `/prototype/`，主界面在 `/`。

## 5. 原型验证出的口径（已回流到规范）

| 发现 | 证据 | 去向 |
|------|------|------|
| H2 与 H3 必须分开计 | 备份在 07 域 H2 命中 0、H3 命中 4/4 | 热力实底/浅底两态；`strength` 字段 |
| 同维度有多个候选小节 | MySQL 同时有「3.1 高可用拓扑」与「4.5 故障转移」，矩阵要的是后者 | `PREFER` 偏好表 + `+1` 角标 |
| 产品页不能"随便挑" | 初版按名称匹配误取了「MySQL 企业级数据库运维管理」 | 显式 `PRODUCT_PAGE` 映射 |
| 行号基准必须是正文 | 该页 frontmatter 占 50 行，用原始行号会整体偏移 | `lineBase: "body"` |

## 6. 已知边界

- 只做了 M1（拆解树/热力/矩阵的数据已具备，构成剖面与关系邻域未做）。
- 矩阵的产品页映射目前硬编码 4 个产品；扩到全库需要 `entities.json`（M3）。
- 热力的维度词表是 M2 的 `dimensions.json` 的雏形，仍写在生成器里。
- 原型不写主界面的路由与样式，验证通过后再并入。

## Related

- [[console/spec/roadmap|路线图]]
- [[console/spec/data-contract|数据契约]]
- [[console/spec/visualization-spec|可视化规范]]
- [[console/spec/walkthrough|走查样例]]