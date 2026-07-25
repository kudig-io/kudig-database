# Release Package — Agent 消费指南

> 给 AgentScope 实例化 Agent 挂载 NAS 时的加载策略参考。

## 本包结构

```
package/<DATE_TIME>/
├── index.md             ★★ Agent 必读入口：核心索引、统计、加载策略、快速定位
├── AGENT-USAGE.md       ← 本文件：简明加载参考
├── manifest.json        ← 包清单（version/profile/计数/supplementary）
├── index.json           ← 全页索引（path/title/summary/tags/tier/tokens）
├── corpus/              ← 语料主体（按 tier 分目录，68 MB）
│   ├── core/            ← 1,122 页（高置信核心页，建议常驻）
│   ├── supporting/      ← 1,693 页（常规支撑页，按需加载）
│   └── peripheral/      ← 1,088 页（边缘页，仅在明确相关时拉取）
├── qa/
│   ├── qa-corpus.jsonl  ← 15,094 个 QA 对（去重后，可直接喂 embedding）
│   └── raw/             ← 63 个原始 QA 源文件
├── metadata/
│   ├── page-stats.json  ← category/tag 分布统计
│   ├── intent-corpus/   ← 意图识别语料（P0 级冷启动必读）
│   ├── agent-specs/     ← 22 个 Agent 行为规范文档
│   └── taxonomy/        ← 5 个受控标签词汇文件
└── profiles/            ← 同次导出的其他 profile 子集（可选）
    └── sre/             ← rag-sre-profile.yaml 产出
```

> **冷启动协议**：Agent 挂载 NAS 后 **先读 `index.md`**（~400 行 / 15 KB），获取完整统计、domain 分布、top hub、synthesis 连接页、QA schema、快速定位指南。本文件仅作为简明参考。

## 加载策略（CRITICAL）

本包 `index.json` 约 2.1 MB，`corpus/` 总计 ~12.3M tokens（3,903 页）。**严禁整包加载到上下文。**

### 默认：Index-only 检索
1. **首次加载** `manifest.json`（<1KB）确认版本与 profile
2. **加载 `index.json`**，用 `title + summary + tags` 对 query 做语义打分
3. **按需拉取** top-K 页：读 `corpus/<tier>/<path>.md` 单文件
4. **优先 core**：core 命中优先于 supporting；peripheral 仅在 query 明确匹配时加载

### 受限上下文：Tiered 加载
- 只加载 `corpus/core/`（1,122 页 / ~3.1M tokens）
- 按需取 supporting；peripheral 默认不加载

### QA 走独立 embedding
- `qa/qa-corpus.jsonl` 单独喂向量库
- 字段：`input/output/source/type/tags/skill_ref/io_pair_id`
- 用于 few-shot 示例、意图分类、query 改写

### v2 新增内容加载建议
- **`synthesis/`**（10 页）：跨域合成页，处理涉及多个知识域的 query 时优先检索
- **`topic-dictionary/`**（564 页）：结构化术语定义，术语归一化和概念 grounding 场景下检索

## 重新生成

```bash
# 默认：写入 release/package/<YYYY-MM-DD_HH-MM>/
python3 scripts/export_corpus_for_nas.py

# 切换 profile
python3 scripts/export_corpus_for_nas.py -p rag-sre-profile.yaml

# 自定义输出（跳过自动时间戳）
python3 scripts/export_corpus_for_nas.py -o /tmp/export
```

## 安全约束

- 脚本拒绝清理以下路径（防止误删兄弟内容）：
  - vault 根目录
  - `release/`（会误删 scripts + package）
  - `release/scripts/`
  - `release/package/`
- 每次导出写入**新时间戳目录**，历史包不会被覆盖
- macOS `uchg` 标志已在导出前清理（`chflags -R nouchg`），复制/删除无阻塞

## 完整度评估

```bash
python3 scripts/evaluate_corpus_completeness.py
# 输出: _reports/corpus-completeness-evaluation-<DATE>.md + .json
```

## 历史归档

`release/package/` 下按时间戳保留所有历史导出；如需清理，由人工决定保留策略（建议至少保留最近 3 个）。
