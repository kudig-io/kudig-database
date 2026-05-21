---
title: 操作教训记录
description: 2026-05-21 重组操作中发生的错误与教训，永久记录以避免重复
category: report
tags:
- lessons-learned
- operations
- git
- safety
- llm
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 操作教训记录 是什么
- 如何 操作教训记录
trigger_keywords:
- 操作教训记录
prerequisites:
- kubectl-basics
---

# 操作教训记录 — 2026-05-21

> **事件**: domain-18-production-operations topic 重组及后续全库重组
> **后果**: 多次险些导致数据丢失，git 状态混乱，用户信任受损
> **记录目的**: 永久铭记，绝不再犯

---

## 错误 1：误执行 `git restore --staged --worktree`

**场景**: 第一轮重组后，文件已用 `git mv` 移入 topic 子目录，工作目录和暂存区状态正确。

**误操作**:
```bash
git restore --staged --worktree domain-18-production-operations/
```

**后果**:
- 暂存区中 domain-18 的 rename 记录被撤销
- 工作目录从暂存区（HEAD 状态）恢复，所有移动操作被撤销
- 第一轮重组成果完全丢失

**根因**: 不了解 `git restore --worktree` 的行为——它会从暂存区强制恢复工作目录，覆盖现有文件。

**铁律**: ❌ **永远不在重组/迁移场景中使用 `git restore`**

---

## 错误 2：误执行 `git checkout-index -a -f`

**场景**: 第二轮重组后，工作目录中的文件莫名消失（暂存区有 rename 记录，工作目录为空）。

**误操作**:
```bash
git checkout-index -a -f
```

**后果**:
- 从暂存区提取所有文件到工作目录
- 由于暂存区记录的是删除状态（D），工作目录中大量文件被清空
- 导致 1043 个文件被标记为删除，涉及 20+ 个 domain

**根因**: `checkout-index -a -f` 是底层命令，在暂存区混乱时极其危险。`-f` 强制覆盖，不检查状态。

**铁律**: ❌ **永远不使用 `git checkout-index`，尤其不能加 `-a -f`**

---

## 错误 3：操作前未全景扫描 git 状态

**场景**: 开始重组前，未执行完整的 `git status` 检查。

**后果**:
- 未发现工作目录中已存在大量文件缺失（best-practices/ 等目录的删除记录）
- 将项目已有的问题误认为是自己造成的
-  panic 状态下做出更多错误决策

**铁律**: ✅ **任何 git 操作前，必须先执行 `git status` 全景扫描**

---

## 错误 4：使用 `git mv` 而非普通 `mv`

**场景**: 重组文件移动时使用了 `git mv`。

**后果**:
- git 状态变得复杂（rename 记录、暂存区 vs 工作目录不一致）
- 出现问题后难以理解和回滚
- 用户对 git 操作的恐惧加剧

**铁律**: ✅ **文件重组只用普通 `mkdir + mv`，绝不使用 `git mv`**

---

## 错误 5：未检查目标目录冲突

**场景**: `mv topic-fta domain-10-troubleshooting-diagnostics/` 时，domain-10 下已有同名 `topic-fta/` 目录。

**后果**:
- `mv` 命令报错 "Directory not empty"
- 部分文件未移动，需要后续手动处理
- 产生冗余的 `-archive/` 子目录

**铁律**: ✅ **批量 `mv` 前，先检查目标 domain 下是否已有同名 topic 目录**

---

## 安全操作清单（重组场景）

```bash
# 1. 操作前：全景扫描
git status --short | head -100

# 2. 统计文件数（基准）
find topic-xxx -type f | wc -l

# 3. 检查冲突
ls domain-yyy/topic-xxx 2>/dev/null && echo "CONFLICT" || echo "OK"

# 4. 执行移动（只用普通 mv）
mkdir -p domain-yyy/topic-xxx
mv topic-xxx/* domain-yyy/topic-xxx/

# 5. 验证
find domain-yyy/topic-xxx -type f | wc -l

# 6. 清理空目录
rmdir topic-xxx 2>/dev/null || true
```

---

## 绝对禁止的命令（重组场景）

| 命令 | 危险等级 | 原因 |
|------|----------|------|
| `git restore --staged --worktree <path>` | 🔴 致命 | 撤销暂存区并覆盖工作目录 |
| `git checkout-index -a -f` | 🔴 致命 | 底层命令，强制覆盖，无视状态 |
| `git reset --hard HEAD` | 🟡 高危 | 丢弃所有未提交变更（除非用户明确要求回滚） |
| `git mv` | 🟡 高危 | 使 git 状态复杂化，难以理解和回滚 |
| `rm -rf` | 🔴 致命 | 永久删除文件 |
| `rmdir <non-empty-dir>` | 🟢 安全 | 仅删除空目录，不会删除文件 |

---

## 用户核心诉求（必须铭记）

1. **不删除任何 md 文件** — 宁可保留冗余副本，也不删除
2. **不执行 git 命令** — 重组只用 `mkdir + mv`
3. **每步验证** — 移动前后统计文件数，确保一致
4. **先给计划再执行** — 让用户确认后再操作
5. **考虑 LLM-Wiki 架构** — 不只是物理分类，还要保留知识图谱结构

---

*本记录作为永久性教训，写入项目历史。绝不再犯。*
