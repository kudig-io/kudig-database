# HANDOFF.MD

> 踩坑与经验沉淀，执行新任务前先读此文件。

## 2026-10-25 · AI 内容发展评估盘点（口径与教训）

- 质量指标快照在 `maintaining/35-元数据/metrics/content-quality-latest.json`（nightly 更新），字段含 orphans/avg_inbound/broken_wikilinks/verified_upon。2026-10-22 口径：4,386 pages、orphan 33.9%、verified 覆盖 0.11%、断链 0。
- QA 语料按域分文件在 `wiki/19-故障诊断/10-QA语料/*-qa.yaml`；统计条目数用 `grep -c 'question:'`，不要用 `^- id:`（匹配不到）。2026-10 口径：全库 14,080 问，AI 域（`AI基础设施-qa.yaml`）692 问 = 4.9%。
- FTA 树清单在 `wiki/19-故障诊断/06-FTA故障树/list/`，50 棵中 AI 相关仅 `gpu-fta.md`（720 行）。
- 各域文档规模统计：`for d in 0*/ 1*/ 2*/ 3*/; do find "$d" -name "*.md" | wc -l; done`。15-AI基础设施 = 180 篇（占活跃层 4.1%）。
- 评估结论（战略）：K8s 侧"最全"成立；AI 侧走"AI Infra 运维"生态位（FTA/FEBM/QA 引擎复制），不做算法/榜单向内容。缺口证据（grep 提及数）：PD 分离 1 / MoE 2 / MLA 1 / SGLang 5 / RLHF·GRPO 13 / 昇腾 26 / Claude Code·Cursor 12 处提及无专文。
- 无新踩坑；本次为只读评估，未改动知识内容。

## 2026-10-25 · 两库划界评估补充（kudig K8s 切入 vs ai-guru AI 原生）

- 划界规则：ai-guru（13,759 篇）拥有「AI 原生·是什么」层（词条/架构分析：MoE 317·RLHF 553·vLLM 645·MCP 339·Claude Code 142·DRA/MIG/HAMi/Kueue/Volcano/KServe/KubeRay/KAI 均有专门词条）；kudig 拥有「K8s 生产·怎么活」层（FTA/FEBM/技能卡/Runbook/配额治理/升级排障）。判据实证：ai-guru 的 Kueue_深入分析.md 702 行中"故障/告警"仅 1 次、runbook 0 次——词条深但运维层空。
- 上一轮"评估记忆"中的 P0 建议（推理深水区词条：PD 分离/MoE/MLA/KV Cache 解读文）已作废：那是 ai-guru 领地；kudig 只写这些引擎/架构「跑在 K8s 上」的部署·伸缩·配比·排障半区。
- kudig AI 域 180 篇人工归类：K8s 视角核心 ≈105 / 移交候选 ≈75（04-AI编码 全部 24、02 框架教程 ≈33、03 框架选型 ≈9、01 AI 原生篇 ≈10）。
- K8s 侧真缺口（grep 提及数）：HAMi 3 · Fluid 3 · Koordinator 混部 6 · 断点续训 4 · RoCE/PFC/ECN 10 · DRA 41（散布 8 篇无专文）· KAI/KAITO/llmaz/AIBrix/LWS 全 0。
- ai-guru 本地路径：`~/Documents/GitHub/ai-guru-global/ai-guru-database`（注意不是同名 org 目录）；其 12_架构基建/05_CNCF云原生AI + 07_硬件与算力 是两库交界地带。
