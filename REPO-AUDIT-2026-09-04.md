# 仓库工程审计 — kudig-database

- **日期**: 2026-09-04
- **范围**: 仓库健康、体积治理、CI 流水线、脚本债、安全
- **基线数据**: git pack 1018.90 MiB;全库 21,417 个 md 文件,其中约 77% 为归档(37-归档, 8,851)、发布快照(32-发布, 3,957)与 vendor 源码(33-源码, 3,678, 已 gitignore);知识核心(01–26 领域 + 内容目录)约 3,000–4,000 篇。

## 总体结论

工程化基础好(已有质量门禁、语料覆盖率校验、发布流水线、RAG 向量化管线),**主要问题是 git 仓库被发布产物撑爆且持续增长**,其次是死流水线、工作区卫生与一次性脚本债。

## 核实事实

| 事实 | 数据 |
|---|---|
| 仓库 pack 体积 | 1018.90 MiB |
| `32-发布/package/` | 115 MB / 4,006 个文件,时间戳快照反复入库 |
| `37-归档/release-packages/` | 227 MB / 7,423 个文件,同上 |
| 最大 tracked 文件 | 4 份相同 8.3MB `qa-corpus.jsonl` + 8MB pptx 附件 |
| `deploy-pages.yml` | 从 `30-站点/` 构建,但该目录 0 个跟踪文件 → CI 必然失败(死流水线) |
| `30-站点` / `33-源码` | 已 gitignore 且整体迁出(pages/源码树),`readme-sync-check.py` 标为 TREE_EXEMPT |
| `32-发布` / `37-归档` | `readme-sync-check.py` 标为 FROZEN(只增不改),仅按目录存在性校验,不依赖 git 跟踪状态 |
| `.claude/scripts/` | 8 个脚本,其中 4 个 backlink wave 变体互为迭代,约 80% 代码重复,硬编码绝对路径 |
| wave3 hub 链接表 | 指向旧版目录结构(`entities/kubernetes.md` 等),当前目录树中不存在 → 原样运行会制造断链 |
| 工作区噪音 | `.obsidian/workspace.json` 被跟踪;`.impeccable/`、`.mimosa/`、`.video_agent/`、`GTM/dist` 未 ignore |
| embedding-pipeline.py | 输出到本地 gitignored `.vector-cache`,消费端在本机(百炼/RAG)→ 不适合进 CI |

## 执行项(P0 → P3)

### P0 仓库止血
1. `.gitignore` 增加 `32-发布/package/` 与 `37-归档/release-packages/`;
2. `git rm -r --cached` 解除两目录跟踪(本地文件保留),发布产物今后不再入库。
   > 可选后续:用 `git filter-repo` 重写历史清除旧 blob(可再省约数百 MB clone 体积)。**破坏性操作,需单独决策,本次未执行。**

### P1 工作区卫生
3. `.gitignore` 补:`.impeccable/`、`.mimosa/`、`.video_agent/`、`GTM/dist/`;
4. `.obsidian/workspace.json` 解除跟踪并 ignore(机器本地 UI 状态);
5. `GTM/`(营销页 index.html + og.png)与 `DESIGN.md` 为有意的成品产物 → 入库;
6. `.claude/scripts/` 的 `backlink_fix.py` + `wave2/3/4.py` 合并为单一参数化脚本 `31-脚本/maintenance/backlink-orphans.py`(ruff 门禁覆盖),旧文件删除。合并版相对原脚本的关键改进:**所有策略先校验链接目标存在,杜绝制造断链**;
7. post-commit 钩子迁入 `.githooks/` 并 `core.hooksPath` 生效,随库分发。

### P3 流水线闭环
8. 删除死流水线 `deploy-pages.yml`(pages 部署职责归 kudig-webui 仓库);
9. 新增 `.github/workflows/nightly-corpus.yml`:定时(北京时间 05:30)+ 手动触发,语料生成 → 覆盖率校验 → 有变更则自动提交回 main;向量化**有意不进 CI**(无 CI 内消费端、重依赖,见上表);
10. 新增 `.github/workflows/secret-scan.yml`:gitleaks(固定版本)只扫新增提交,不对存量 2.1 万文件翻旧账,避免遗产误报阻塞。

## 延后项(需人工决策)

- **历史重写**:`git filter-repo` 清除 `32-发布/package`、`37-归档/release-packages` 的历史 blob。收益(clone 体积降 ~2/3)明确,但重写全部 commit hash,需协调远端协作者,单独安排执行。
- **归档目录整体外迁**:37-归档/32-发布 中更早的历史快照可进一步迁至 GitHub Releases,属 P2 结构调整,未在本次范围。

---

## 执行记录（2026-09-04）

| # | 动作 | 结果 |
|---|---|---|
| 1 | `.gitignore` 增补发布产物/Obsidian 状态/GTM dist/Agent 会话产物 6 条规则 | 完成 |
| 2 | `git rm --cached` `32-发布/package` + `37-归档/release-packages` | 11,379 项解除跟踪，本地文件保留；今后发布快照不再入库 |
| 3 | `.obsidian/workspace.json` 解除跟踪 | 完成（发现 `.mimosa/`、`.video_agent/`、`.impeccable/` 共 17 个文件被此前 `update` 提交误扫入，一并解除跟踪并 ignore） |
| 4 | `GTM/`、`DESIGN.md` 处置 | 核实后已在提交 `28fa721b21` 中入库，无需操作 |
| 5 | 4 个 wave 脚本合并为 `31-脚本/maintenance/backlink-orphans.py` | 完成；ruff 通过（quality.yml 门禁覆盖）、py_compile 通过；真实库 dry-run：扫描 4,343 页 / 孤儿 2,056 / emitters 待补 5,168 链接（未写盘）；hub 策略旧路径全部被存在性守卫拦截，0 断链 |
| 6 | post-commit 钩子迁入 `.githooks/` + `core.hooksPath` | 完成（Qoder tracker，`|| true` 跨机器安全） |
| 7 | 删除 `deploy-pages.yml` | 完成（从 gitignored 且 0 跟踪文件的 `30-站点/` 构建，CI 必然失败；pages 部署职责归 kudig-webui） |
| 8 | 新增 `nightly-corpus.yml` | 完成；YAML 校验通过；北京时间每日 05:30 生成语料→覆盖率校验→有变更才提交回 main |
| 9 | 新增 `secret-scan.yml` | 完成；YAML 校验通过；gitleaks v8.18.4 固定版本，push 只扫新增提交，手动可选全量 |

**预期收益**：今后发布产物不再进入 git（此前累计约 342MB 工作区、pack 大头的来源即此类快照）；工作区状态干净（无工具目录噪音）；nightly 自动保持语料新鲜并在覆盖率退化时红灯；新增提交含秘密即在 CI 被拦截。

**验证方式**：合并脚本经 ruff + py_compile + 真实库 dry-run 三重验证；workflow YAML 经 yaml.safe_load 解析验证；其余为 git 配置/ignore 规则，`git status` 可直接复核。本次全部改动以单条 commit 提交，可整体 revert。
