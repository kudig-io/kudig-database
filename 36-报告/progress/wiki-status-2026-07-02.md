# Wiki Status — 2026-07-02

> 维护周期完成后的基线快照。Release 刚于 16:39 重新生成。

## Overview

- **Corpus 页面 (profile-filtered):** 3329 across 20 domains
  - core: 804
  - supporting: 1509
  - peripheral: 1016
- **Vault 总页面 (hard-excluded only):** 5553（含 _reports / _meta / docs / 顶层辅助 2224 页，不进入 corpus）
- **Page visibility:** 5553 public · 0 internal · 0 pii
- **Release 输出:** `release/` 于 2026-07-02T16:39:25 生成
- **QA pairs:** 15,094（去重后；原始 18,520 → 移除 3,426 重复）
- **Release tokens:** 11,553,184
- **Profile:** `rag-full-profile.yaml`
- **最近一次 ingest:** 维护周期（2026-07-02）—— 非新增 ingest，而是结构修复
- **Staged writes:** 0

## Delta（本次维护相对上次 ingest 的变化）

本次为**结构修复周期**，不是 ingest 周期。无新增源文件进入 corpus；所有变化均为已有页面的元数据与链接修复。

| 变化类别 | 数量 | 说明 |
|---|---|---|
| 修复断链 | 935 → 0 | 目录-as-link（441）+ topic-index 嵌套（158）+ 中文标题（24）+ 其他（312） |
| 孤岛页减少 | 406 → 305 | 新增 20 条 cross-link，覆盖 19 个孤岛 |
| 陈旧核心页刷新 | 37 | `last_updated` 提升至 2026-07-02 |
| Lifecycle 修复 | 3 | 设为 `reviewed` |
| PII `sources:` 修复 | 2 | 补齐空 sources |
| 新增 corpus 页面 | 0 | 无新 ingest |
| 修改 corpus 页面 | ~560 | 链接 / 元数据修复（仅正文，不改知识） |

## Token Footprint（release/ 实际计数）

| Scope | Pages | Tokens |
|---|---|---|
| core tier | 804 | ~2.8M |
| supporting tier | 1509 | ~5.2M |
| peripheral tier | 1016 | ~3.5M |
| **Full release (all)** | **3329** | **11,553,184** |

Index-only pass (`index.json` frontmatter + summaries): ~1.8M tokens
Typical query (index + 5 full pages): ~1.8M + 5 × ~3500 ≈ 1.82M tokens

⚠️  Full release 远超 100K token 阈值。建议：
  - 按 tier 分层加载（仅加载 core = 2.8M；按需取 supporting）
  - 在 Agent 侧启用 index-only 检索（只读 `index.json` 的 summary/tags）
  - QA 语料单独走 embedding pipeline，不要与 corpus 混索引

## What to Do Next

0. ⚠️  **Release 已就绪但超阈值** — 11.5M tokens 需要分层策略
   → 配置 Agent 只加载 `release/index.json` 做首次检索，再按 `path` 拉取单页
   → 或使用 `rag-sre-profile.yaml` 导出 SRE 专用子集（更小）

1. 🔗  **305 个孤岛页仍待链接**
   → 本轮只处理了 19 个高价值孤岛；剩余多为 `index.md` 或通用标题
   run: `/cross-linker`（第二轮，针对 domain-04 / domain-12 / skills 目录）

2. 🧩  **Synthesis 扫描超期**（上次运行 2026-05-24，距今 39 天）
   → 上次新增 19 概念 + 11 合成页的跨域链接；本轮修复后可能暴露新的高共现对
   run: `/wiki-synthesize`

3. 🩺  **Lint 刚完成**（2026-07-02，本日），无待修复项
   → 下一轮建议 30 天后：2026-08-01

4. 📊  **Page-stats 与 index.json 已同步**
   → `release/metadata/page-stats.json` 反映修复后的 category/tag 分布
   → `release/qa/qa-corpus.jsonl` 已去重，可直接喂给 embedding

5. 🔒  **无 `_raw/` 文件、无 staged writes**
   → 输入队列干净；下次 ingest 前可先整理 sources

6. ✅  **Vault 结构健康**
   - 断链 = 0
   - 陈旧核心 = 0（全部刷新）
   - Release 输出幂等（`chmod + chflags + rm` 已验证）

## Methodology

- Token 估算：release 直接按 `len(text) // 4` 求和（非 tokenizer，粗略估算）
- 可见性统计：扫描 frontmatter 的 `tags:` 段（支持 inline `[a,b]` 与 block list）
- 陈旧判定：`last_updated` 早于 `today - 90d` 且 incoming ≥ 5
- Profile 过滤：复用 `scripts/export_corpus_for_nas.py::should_include`（单一事实来源）
