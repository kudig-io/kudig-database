---
title: 重排 P0 盘点报告
description: 全库重复/同题异版盘点（4,711 md）——三类信号量化、五个关键发现、对重排方案的修正输入与人工评审队列
summary: 'P0 阶段产出：reorg-inventory.py 三信号扫描（同名 486 组/内容全同 161 组/同题异版 310 组）、机器可读映射清单，以及 S-1/S-4/S-7 收敛项的实测规模。'
category: references
tags:
- structure
- duplicate
- inventory
- reorg
- corpus
tier: supporting
created: '2026-11-18'
last_updated: '2026-11-18'
---

# 重排 P0 盘点报告

> 阶段：**P0 完成（盘点部分）**；nightly 冻结推迟到 P2 执行窗口前操作
> 关联：`structure-reorg-plan-2026-11-18.md`（方案）、`reorg-inventory.json`（机器清单）、`reorg-mapping-manifest.json`（映射清单）

## 1. 范围与方法

- 扫描：`wiki/`（排除 `31-归档`）+ `skills/`，共 **4,711 md**
- 工具：`code/scripts/maintenance/reorg-inventory.py`（P1 判重复跑、P4 门禁复用；`--check` 只读模式）
- 三类信号（结构性文件 README/index/MOC 等单独分桶，不计入内容重复）：

| 信号 | 含义 | 内容组 | 涉及文件 | 跨目录组 |
|---|---|---:|---:|---:|
| duplicate_basenames | 同名文件 | **486** | 1,114 | 486（100%） |
| identical_content | 正文全同（去 frontmatter 后哈希，抓改名副本） | **161** | 351 | 160 |
| title_collisions | 同题（标题同）但正文不同 = 同题异版 | **310** | 715 | 289 |

## 2. 关键发现

### F1（新发现，方案 S-7 扩大）：`20-最佳实践` 存在系统性镜像子目录

`20/01-best-practices/` 与其兄弟子目录（02-08）成对镜像：`pod-security`、`storage`、`monitoring`、`networking`、`kubernetes-cluster`、`logging`、`tracing`、`disaster-recovery`、`scaling`、`network-security` 等至少 **9 组正文全同**（`01-best-practices/X/` vs `0X-X/`），且**第三份**常驻 `skills/**/最佳实践/`（含 `-alt.md` 变体）。
→ 修正方案 S-7：不是"01 vs 03 局部合并"，而是 **`01-best-practices` 整目录与 02-08 收敛**，skills 侧技能卡保留、wiki 侧副本改链。

### F2：wiki ↔ skills 内容全同 74 组——S-4 的大头可全自动

`identical_content` 中 **74 组**同时横跨 wiki 与 skills（正文全同、仅 frontmatter 体裁不同）。→ P1 判重规则草案：**正文全同 → 按体裁定主场**（知识条目留 wiki、技能卡留 skills），非主场侧替换为 wikilink；无需人工逐对评审，抽检 10% 即可。

### F3：terway 同题异版实测 = 8 对 + 1 对索引异版

`05/01-K8s网络核心/42-49` 与 `23-实体/04/03-10` 标题逐对相同、正文不同（8 对）；`05/06-Terway/terway-index.md` 与 `21-生态参考/03-领域索引/terway-index.md` 同题但正文不同（376 vs 265 行，diff 262 行，未被全同清理覆盖）；`05/01` 与 `05/06` 两套系列标题不同未被自动配对（方案已注明 diff 626 行）——**S-1 人工队列 = 8 对择一 + 索引 1 对 + 05 域内两套系列合并**。

### F4：工单案例双分区复制

`13-生产运维/05-工单案例` 与 `skills/04-工作负载/pod/工单案例` 存在同名同题对（ticket-case-011/036/046 等，标题"ingress 控制器 pod 异常导致业务访问 404/502"一题 5 处）。→ 归入 F2 同规则：工单知识留 wiki、skills 引用。

### F5：标题碰撞的噪声基线

`title_collisions` 高频组含目录级同名（"deployment"/"observability"/"reference"，18 个 index 同题）——属分区命名的合法撞题，P1 判重时按"结构文件跳过 + 跨域标题需带路径上下文"处理，不计入人工队列。

## 3. 对方案的修正输入

| 方案项 | 盘点前 | 盘点后 |
|---|---|---|
| S-4 技能双主场 | 同名 51，人工判重 | 内容全同 74 组可自动（规则见 F2），人工仅抽检 |
| S-7 域内自重 | 01 vs 03 局部 | `01-best-practices` 整目录镜像 02-08（F1），含 skills 第三份 |
| S-1 terway | "8 篇择优" | 精确为 8 对同题异版 + 1 对索引异版 + 05 域内两套系列（F3） |
| S-6 迁移三件套 | 11/20 双份 | 已证实为内容全同（identical 组），**可直接自动判重** |
| 新增 S-9 | — | 工单案例 wiki↔skills 复制（F4） |

## 4. 产物清单

| 产物 | 路径 | 用途 |
|---|---|---|
| 盘点脚本 | `code/scripts/maintenance/reorg-inventory.py` | P1 复跑、P4 `--check` 门禁 |
| 机器清单 | `maintaining/02-元数据/metadata/reorg-inventory.json` | 三信号全量分组（含路径列表） |
| 映射清单 | `maintaining/02-元数据/metadata/reorg-mapping-manifest.json` | P2 改名脚本输入（域级 31 项 + 死桩删除 2 组） |

## 5. P0 状态与下一步

- [x] 全库三信号盘点与机器清单
- [x] 映射清单机器化（域级）
- [x] CI 基线绿（`console-check` 最近一次通过）
- [ ] nightly 冻结 —— **推迟至 P2 执行窗口前**（当前无在途迁移，不需提前停更）
- [ ] 判重规则评审（F2 草案：正文全同按体裁定主场）——P1 启动条件

**下一步 P1**：① ~~按 F2 规则批量处理 161 组全同副本~~ **已完成（2026-11-18）**：`apply-identical-dedup.py` 落桩 190 份、改写 51 处 QA 引用；复核后追加规则 R-MAIN（主题域>提炼层，含 18-云厂商>23-实体/13）、R-FUNC（非 functions-* 优先）、R-TRAIN（培训嵌套>平铺）；复盘 `identical_content` **161 → 0**，QA 悬空 0、指向桩的 source 0，`console-check` 通过。② ~~production-readiness 模板生成器（S-2）~~ **已完成（2026-11-18，形态调整）**：全文生成不可行（正文深度域特异），落地为规范模板 + `check-production-readiness.py` 骨架校验/编号规范化，20/20 指南通过、`--check` 可作 CI 门禁。③ ~~死桩清理（59 个）~~ **已完成（2026-11-18）**：删除前校验（模式匹配 59/59、目标不存在、零入链）后 `git rm`，`console-check` 通过；④ F3/F1 人工队列评审——**分批执行中**：评审单 `title-collision-review-queue.md`（`review-title-collisions.py` 生成，A/B/C/D 四节）；**A 批已执行**（terway 三向：18 节回填 + 旧版 42-49 落桩）；**B 批已裁决执行（2026-11-18）**：B1+B2/B4 按主场落桩 37 份、B3a 工单 12 篇标题加（工单 NNN）、B3b 培训 7 篇加受众限定词、B3① 语料 5 组以 `.with_actions` 为主场落桩；**C 批 94 组整批签认**；title_collisions **160 → 104**（QA 悬空 0）；**余 D 节 B3② 10 组**（细摘要与甲/乙/丙/丁分类建议已出，待裁决）；23-实体侧 8 卡按 S-1「压成 1 张速览卡」仍待办。
