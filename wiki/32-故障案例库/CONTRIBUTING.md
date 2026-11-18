# 贡献指南与维护机制

本文档定义 kuba-database 的数据录入规范、命名约定、版本控制与更新机制。**所有贡献必须遵循本规范**，以保证数据库长期可检索、可统计、可演进。

## 1. 文件命名规范

```
incidents/<领域目录>/<YYYY-MM-DD>-<公司slug>-<事件slug>.md
```

- 日期为**故障开始日期（UTC）**。
- `公司slug` / `事件slug` 全小写，单词以 `-` 连接，只允许 `[a-z0-9-]`。
- 示例：`incidents/cdn-edge/2019-07-02-cloudflare-waf-regex.md`
- 领域目录必须是 [docs/severity-and-taxonomy.md](docs/severity-and-taxonomy.md) 中已定义的领域；新增领域需先在该文档登记，并在 `incidents/README.md` 补充说明。

## 2. 元数据（Frontmatter）规范

所有字段定义与合法枚举值以 [templates/incident-report.md](templates/incident-report.md) 为准。硬性要求：

| 字段 | 要求 |
|---|---|
| `id` | 全库唯一，格式 `INC-<YYYY><MMDD>-<公司大写slug>`，如 `INC-20190702-CLOUDFLARE` |
| `severity` | 必须是 `SEV-1`~`SEV-4` 之一（定义见分级标准） |
| `root_cause_category` | 必须是根因分类表中的枚举值（单选主根因） |
| `root_cause_tags` | 细粒度标签，小写-连字符，2~6 个 |
| `sources` | 至少 1 条可访问的公开来源 URL，官方复盘优先 |
| `status` | `draft`（信息不全）/ `published`（复盘完整）/ `updated`（官方披露新信息后修订） |
| `last_updated` | 每次内容修改必须更新（YYYY-MM-DD） |

## 3. 录入流程

1. **建档**：`./tools/new-incident.sh <domain> <date> <company> <slug>` 生成骨架，`status: draft`。
2. **收集**：优先级 官方 Postmortem > 官方状态页/博客 > 监管报告 > 权威第三方分析（须标注）。
3. **复盘**：按模板完成全部章节；根因分析至少完成一条 5 Whys 链或贡献因素清单。
4. **定级**：对照分级标准确定 `severity`，不确定时就低不就高，并在正文说明定级依据。
5. **发布**：内容完整后改为 `status: published`，运行 `python3 tools/build-index.py` 重建索引。
6. **提交**：索引文件与案例文件在同一 commit 中提交。

## 4. 版本控制与更新机制

- **Git 即版本机制**：案例的每次演进都是一次 commit；历史通过 `git log --follow <file>` 追溯，不在文档内维护修订表。
- **Commit message 规范**：
  - `add(<domain>): <incident-id> 新增案例`
  - `update(<domain>): <incident-id> 补充官方复盘细节`
  - `fix(<domain>): <incident-id> 修正事实性错误`
  - `docs: ...` / `tools: ...` 用于方法论与工具变更。
- **更新触发条件**：官方发布正式 RCA、披露新细节、纠正早期说法时，必须更新案例正文与 `last_updated`，并将 `status` 置为 `updated`。
- **索引一致性**：`INDEX.md` 是生成物，禁止手工编辑；任何案例变更后必须重跑 `build-index.py`。

## 5. 内容红线

- ❌ 不收录仅有传闻、无公开来源的事件。
- ❌ 不点名批评个人（无指责原则）；官方报告中的"操作失误"表述为"操作层面的系统性防护缺失"。
- ❌ 不收录未公开的内部信息、泄露材料。
- ❌ 事实与观点混写；推断性内容必须以"（第三方推断）"显式标注。

## 6. 评审清单（Review Checklist）

- [ ] 文件名、目录、`id` 三者一致且符合规范
- [ ] Frontmatter 字段完整、枚举值合法
- [ ] 时间线含时区标注（统一 UTC，可括注当地时间）
- [ ] 根因分析区分"触发因素 / 根本原因 / 使影响扩大的因素"
- [ ] 每条经验教训可操作（能转化为工程改进项）
- [ ] 所有来源链接可访问
- [ ] 已重跑索引且无告警
