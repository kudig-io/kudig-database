# HANDOFF.MD

> 踩坑与经验沉淀，执行新任务前先读此文件。

## 2026-11-10 · console M3：实体词表与通用对比矩阵

- 新文件 `console/spec/entities.json`（11 组 / 39 实体，v2026.11.1）：每个实体用 `pageHints` 正则绑定旗舰页，生成器解析成 `console/data/graph/entities.json`（实体 → 页 → 维度小节 → 行号），矩阵不再硬编码产品。
- 矩阵改为"实体组 × 6 个维度组"，可切 11 组；每行带 `n/6` 覆盖徽章。
- `make prototype-verify` 扩到 **98 条锚点**（全部实体×维度的样本），并继续校验词表基线。
- **实测结论**：CNI 组 Cilium 3/6、Flannel 2/6、Terway 2/6、Calico **0/6**（只有 FTA 树、没有成节产品页）。这是语料里最值得被看见的结构性偏斜。
- **踩坑**：实体绑定页不能只靠 pageHints 的第一个命中——`calico-fta` 会先于 `calico-cni` 命中且该页无维度小节；对这类"只有 FTA"的实体要在词表里写 `note` 并允许 `dims` 为空，而不是把它绑到别的产品页上凑数据。
- **踩坑**：验证脚本也要剥 `<!-- chunk: -->`——注释与标题同行时，正则看到的是 `<!-- chunk: -->## X`，层级被算成 H0，误报 14 条 "MISMATCH"。

## 2026-11-10 · console M2：维度词表外置 + 原型跨分区（86 域）

- 新文件 `console/spec/dimensions.json`（21 维，v2026.11.1）：生成器 `load_dimensions()` 与原型 `prototype.js` 都从这里读，不再各自内联；`blocks.json` schema 升到 `kudig-console-blocks/2` 并带 `dimensionsVersion`。
- 词表从 12 维扩到 21 维：新增 排障/最佳实践/适用场景/选型与工具/命令速查/检查清单/调度/配额，理由是 86 个域上实测 `故障排查` 660 次、`最佳实践` 459 次、`面试要点` 342 次等高频章节原本无处归位。
- 原型新增四分区切换（语料正文 / Agent 技能卡 / 维护资产 / 可执行资产）与热力自适应列数（稠密域铺 21 维，稀疏域取前 14 列）。
- `make prototype-verify` 扩成两道门：锚点（11/11）+ 词表回归（07 域监控 58%/备份 43%/高可用 27%，±15 点）+ 未知维度 id 断言。
- **踩坑**：路由同时要支持 `#/wiki%2F05-网络/heat`（域被整体编码成一个 segment）与 `#/skills/04-工作负载/pod/heat`（域跨 3 段）。只按"前缀匹配"会把 `wiki/05-网络` 错配成 `wiki/05-网络/00-总览`（它确实是一个真实子域），必须要求前缀**精确等于**某个域。
- **踩坑**：域键的层级不统一——`wiki/` 下是 2 段、`skills/` 下是 3 段；用 `slice(0,3)` 一把算会漏掉所有 wiki 域，路由静默回落到默认域（表现为"点哪个域都停在 07"）。
- **踩坑**：词表回归基线必须在**它被测量的范围**上断言（07 域 67 页），拿全库做分母会把 58% 算成 27.9%，误报回归。

## 2026-11-10 · console 结构视图原型（M1：blocks.json + 三视图）

- 生成器新增 `console/data/graph/blocks.json`：每页 H2/H3 大纲（`s`/`e` 行号区间、表/码载荷）、表格行、代码块、维度命中（`strength` = section/subsection）。规模 4,817 页 / 100,240 个标题节点 / 14MB，由 `make console-sync` 重建。
- 原型在 `console/prototype/`（不进主界面）：拆解树 / 单页拆解 / 一致性热力 / 对比矩阵 + 证据抽屉 + 定位原文高亮。入口 `make console-serve` → `http://127.0.0.1:8766/prototype/`。
- `make console-serve` 改为运行 `console/prototype/serve.py`：普通静态服务之上加 `/__src/<仓库相对路径>.md` **只读桥**（仅 wiki/ skills/ maintaining/ code/ 的 .md，拒 `code/repo/`，拒路径穿越），供"定位原文"读原始 Markdown。
- 锚点门禁 `make prototype-verify`：矩阵每格必须落在源文件对应行的标题上，当前 11/11 通过。这是 `console/spec/acceptance.md` D2 的窄化版。
- **踩坑（重要）**：`<!-- chunk: … -->` 与 heading 在同一行时，**删除**注释会让该行不再是 `## ` 开头，锚点整体失效；必须把注释替换成等量换行以保持行数。另外行号必须按"被删注释后的 body"计算，否则会与源文件差几行。
- **踩坑**：验证脚本和被验证的生成器必须用同一套 body 切分（`FM = \A---\n(.*?)\n---\n`）；我先用 `split('---',2)[2]` 复算，行号系统性偏移，误报 97% 失配。
- 原型验证出的口径：H2/H3 分层计数、同维度多候选小节的 `PREFER` 偏好、产品页必须显式映射（不能按名称首匹配）。

## 2026-11-10 · console/spec 设计方案定稿（文档，未编码）

- `console/spec/` 新增 10 篇设计文档（PRD / 数据契约 / 可视化规范 / 交互无障碍 / 验收 / 路线图 / 走查 / 决策记录 / 维度词表 / 总览），约 1,650 行。定位从"读语料"改为"拆解语料"：结构视图为主，阅读页降为证据页。
- 三条来自实测、必须守住的口径：
  1. **H2 与 H3 都要匹配**：`07-数据库中间件` 的"备份"在 H2 命中 0、在 H3 命中 4/4——只看 H2 会把成熟维度判成缺失。
  2. **维度组对齐**：TiDB 没有"故障转移"小节，高可用由 `5.1 扩缩容 + 5.3 节点替换` 承担；矩阵必须记录 `mappingNote`，否则会误判"TiDB 缺高可用"。
  3. **锚点落在正文行号**（去 frontmatter 后的 1-based）：该页 frontmatter 占 11 行，用原始行号会系统性偏移。
- 踩坑：`pathlib.Path.rglob('*.md')` 在 macOS 上**不匹配大写扩展名**（`maintaining/HANDOFF.MD` 不在结果里），CI 的断链门禁因此也看不见它。给 `HANDOFF.MD` 建 wikilink 会在自定义检查里报"断链"，而 CI 反而不报——两边口径不一致。规范文档里一律不要链接 `HANDOFF.MD`，改为引用 `console/spec/decisions#D-004`。
- 下一步是 M1（`blocks.json` + 拆解树），在此之前不写业务代码：结构视图的一切正确性都建立在锚点准确性上。

## 2026-11-10 · console/ 升级为全库工作台（4,815 页 / 86 域 / 2,277 张 Mermaid 图）

- 生成器 `code/scripts/maintenance/console-corpus.py` 改为全库口径：`wiki/`（30 域）+ `skills/`（43）+ `maintaining/`（3）+ `code/`（10），**排除 `code/repo/`（7,845 个上游文件）、31-归档、03-发布、04-报告、27-资产**。产出 `console/data/{index,domains,graph/*}.json` 与 `console/content/<page-id>.json`（按需加载的单页切片），`make console-sync` 重建、`make console-check` 在 CI 校验（4821 个文件逐一比对，不要手改）。
- 内容切片格式（改格式要同时改 `console/assets/console.js`）：`["t",文本]` / `["c",语言,代码]` / `["m",图id,类型,代码,"源码写法"]` / `["d",图id,类型,原因,源码]`。
- Mermaid 用 `console/vendor/mermaid.min.js`（11.14，Apache-2.0）**本地渲染**，离线可用；构建期的引擎校验结果在 `console/data/mermaid-validation.json`。
- 踩坑：`fetch` 的 `cache: 'no-cache'` 对 CSS/JS 无效，截图或改前端后必须绕缓存（本轮用 URL 的 `?cb=Date.now()`），否则会拿着旧渲染下结论。
- 踩坑：headless Chrome 的 `--dump-dom` 对带 hash 的 URL 会挂死；交互断言一律走 CDP（`Runtime.evaluate` + `Input.dispatchMouseEvent`）。
- 口径：七层判据沿用（标题命中），全库 20 页七层齐备、1,414 页 0 层；L4 只有 632 页是判据严格，不是内容缺失。

## 2026-11-09 · console/ 知识体系工作台（新顶层分区）

- 新增第五分区 `console/`：纯静态工作台（`index.html` + `assets/console.{css,js}`），数据 `console/data/corpus.json`（结构，76 KB）与 `console/data/content.json`（57 篇原文逐字，903 KB）**均由脚本生成，勿手改**。
- 生成器 `code/scripts/maintenance/console-corpus.py`：入口 `make console-sync`，CI 门禁 `make console-check`（quality.yml 新增 Console Corpus Check job）。改了 `wiki/07-数据库中间件` 下的页面标题后必须重跑，否则 CI 报 STALE。
- 口径（2026-11-09）：8 知识域 57 页 / 26,067 行；七层落锁 L0 7·L1 20·L2 48·L3 46·L4 8·L5 43·L6 8（共 180/399）；FTA 48 棵（615 OR / 35 AND / 2,280 叶）；技能卡 73 张唯一（95 份副本），其中 49 张与知识域相连。**都以此脚本输出为准，不手抄 README 数字。**
- 分层足迹判据 = 页面 H1/H2 标题命中关键词（见 `LAYERS`）；新增层或改判据只改脚本 `LAYERS`，界面自动跟随。技能卡连接 = 卡片正文前 3,000 字符命中 `ATOM_KEYWORDS`，是关键词连接而非人工审定，界面上如实标注。
- 踩坑：仓库 `README.md` 第 94–150 行左右的顶层架构图等宽框按**全角字符计宽**，插入 `console/` 行时要按显示宽度补空格，否则框线错位；`readme-sync-check.py` 只校验目录名，不校验空格。
- 踩坑：`file://` 直接打开 `console/index.html` 会被浏览器拦截 `fetch('./data/...')`，界面进入样本模式并提示；本地务必 `make console-serve`（8766 端口）。
- 踩坑：headless Chrome 对带 hash 的 URL 用 `--dump-dom` 会挂起；验证用 `--screenshot`。工作台 deep-link 里域 id 是 `db/cache/mq/tsdb/search/stream/operator/newdb`，不是目录名 `01-数据库`。
- 设计：抽样册页式（沿用 GTM 设计语言），白纸 + IBM 蓝 + 一色琥珀只标缺口；实现后的设计系统见 **`console/DESIGN.md`**（含字体策略、契约漂移记录）。
- 踩坑（重要）：**不要在静态页里挂 Google Fonts 的阻塞样式表**——受限网络下它会挂住 `DOMContentLoaded`，JS 不初始化，页面永远停在「载入语料」。console 已改为纯系统字体栈；其他静态页若出现同样症状先查这里。
- 踩坑：手机端隐藏的抽屉侧栏若只用 `transform: translateX(-102%)` 移出视口，仍会参与命中测试，吞掉左侧约 250px 的点击（表现为点头部完全没反应）。必须同时 `visibility:hidden`，打开时再恢复。
- 覆盖矩阵不要做成 8×7 的零墙（清一色 0 的格子等于没有信息）；按「完整层数」分 ≥4 / 1–2 / 0 三段，零完整层段内按已落锁层数排序，底部按缺口页数给出「下一批工作面」。L4 判据从严时只有 8 页落锁，是判据严格、不是内容缺失——放宽会把顺带提一句的页面也算进来。
- 侧栏 `nav a` 是 4 列栅格（图标 / 名称 / 0-7 徽 / 页数）；新增元素必须同步改列模板，否则页数会被挤到下一行（亚秒级小屏最容易漏看）。
- 改代码后**必须做真实交互冒烟**，不能只看截图：本轮截图长期看着正常，但 `twinMarkup` 里一个未定义变量（`domainFta`）让双标尺整块抛错、渲染为空，截图看不出来。工具：`.impeccable/tools/capture.mjs`（CDP，断言后才落盘 + 真实鼠标事件）。
- 踩坑：headless Chrome 在 1× 设备像素比、320–390 宽的小窗口里会把后续绘制裁剪到约 340px（DOM 几何仍是 390），移动端截图一律加 `--force-device-scale-factor=2 --window-size=780,1688` 后按 2× 审图，别把裁剪当成 CSS 缺陷。
- 口径纪律：工作台的「连接」是页面级关键词命中（主题词 = uid + 标题去掉通用词），不是人工审定的映射；57 页中 29 页命中，MySQL/PostgreSQL/TiDB 等无专名故障树的页面如实显示 0。改这条规则时同步改 `console/README.md` 与 `console/assets/console.js` 的 `GENERIC`。

## 2026-11-06 · ⚠️ 仓库三分重构已落地——旧路径全部失效

- 仓库已按「根目录归拢评估」方案重构：`code/` + `wiki/` + `skills/` + `maintaining/` 四分。原根目录的 `01-集群基础`…`39-*` 数字目录全部移入 **`wiki/`** 前缀下；元数据/报告/发布移入 **`maintaining/`**（如 `maintaining/02-元数据/`、`maintaining/04-报告/`、`maintaining/35-元数据/`——注意 35-元数据 在 maintaining 下）。
- 沉淀文件新路径：`maintaining/04-报告/k8s-高级面试快问快答-沉淀.md`；journal：`maintaining/02-元数据/journal/log.md`；本文件（HANDOFF.MD）：`maintaining/HANDOFF.MD`。
- **wiki-link 引用旧路径会断链**（nightly 门禁断链必须保持 0）：给既有笔记加链接时，一律用 `[[wiki/...]]` 或 `[[skills/...]]` / `[[maintaining/...]]` 新前缀。本库早前笔记里的 `[[wiki/…]]` 前缀写法在重构后恰好与目录名对齐，存量链接未断。
- 2026-10-25 两条评估记录中引用的旧相对路径（`15-AI基础设施`、`35-元数据/...` 等）按上述规则加 `wiki/` 或 `maintaining/` 前缀即可定位。

## 2026-11-06 · BMCP 深度沉淀（Q128–Q138 + 010 深挖笔记）

- 009（FAQ 速查层）与新建 010（机制深挖层）双层结构：009 答「现象→处理」，010 答「机制→为什么→怎么防」。010 的机制推断（数据盘征用 kubelet 视角、KGPU×HAMi×DRA 对照）已逐处标注「推断」，与官方文档事实（V3.20.0，20260423）区分——后续引用时保持这个标注纪律。
- 六张 009 §11 面试卡已迁入沉淀文件 Q128–Q133（009 §11 原文保留并加了迁卡指针，作 FAQ 层自洽阅读）；新深挖卡 Q134–Q138。010 §12 的 Q 号引用与实际落号已核对一致。
- 专有云-ACK/index.md 是**手工维护**表格（与域级自动生成 index 不同），加新文档要手动加行。


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
