---
title: "Wiki Dedup 审计报告 — 2026-05-24"
category: reports
tags: ["reports", "dedup", "quality", "visibility/public"]
sources: ["Vault Scan 2026-05-24"]
created: 2026-05-24
updated: 2026-05-24
---

# Wiki Dedup 审计报告 — 2026-05-24

> 扫描 4,762 页面，发现 795 个候选重复对。

---

## 总览

| 类别 | 候选对数 | 置信度 | 建议 |
|------|---------|--------|------|
| domain-20 目录重构 | 96 | HIGH | 合并（保留 topic-application-architecture） |
| CNCF 实体重复 | ~50 | HIGH | 合并（保留 entities/ 版本） |
| training 讲师/公开 | 19 | MEDIUM | 审查后决定 |
| 跨目录 README/MOC | ~500 | LOW | 保持现状（不同上下文） |
| 其他跨目录重复 | ~130 | MEDIUM | 逐案审查 |

---

## 1. domain-20 目录重构（96 对）

`01-reference-architectures/` 与 `topic-application-architecture/` 完全重复。

**建议**: 保留 `topic-application-architecture/`（新结构），删除 `01-reference-architectures/`。

**示例**:
- `01-reference-architectures/63-industrial-visual-inspection` ↔ `topic-application-architecture/63-industrial-visual-inspection`
- `01-reference-architectures/45-smart-port-shipping` ↔ `topic-application-architecture/45-smart-port-shipping`
- ... 共 96 对

## 2. CNCF 实体重复（~50 对）

`domain-19-landscape-references/01-cncf-landscape/graduated/*/` 与 `entities/` 重复。

**示例**:
- `domain-19-.../graduated/tikv/tikv` ↔ `entities/tikv`
- `domain-19-.../graduated/dragonfly/dragonfly` ↔ `entities/dragonfly`
- `domain-19-.../graduated/cloudevents/cloudevents` ↔ `entities/cloudevents`
- `domain-19-.../graduated/fluentd/fluentd` ↔ `entities/fluentd`
- `domain-19-.../graduated/harbor/harbor` ↔ `entities/harbor`

**建议**: 保留 `entities/` 版本（核心知识库），landscape 版本改为引用。

## 3. training 讲师/公开（19 对）

`skills/training-lecturer/` 与 `skills/training-public/` 共享 19 个同名文件。

**建议**: 保留两套（讲师版有额外注释），但添加交叉引用。

## 4. 跨目录 README/MOC（~500 对）

这些是导航文件，每个目录都有自己的 README.md，不是真正的内容重复。

**建议**: 保持现状。

---

## 待执行操作

| 操作 | 数量 | 风险 |
|------|------|------|
| 删除 domain-20 旧目录 | 96 文件 | 低（内容完全相同） |
| CNCF 实体 → redirect stub | ~50 文件 | 低（entities/ 版本更完整） |
| training 交叉引用 | 19 文件 | 无（添加链接） |

---

> 审计时间: 2026-05-24
> 扫描页面: 4,762
> 工具: wiki-dedup skill
