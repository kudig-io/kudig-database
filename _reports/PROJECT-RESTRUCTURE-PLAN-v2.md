---
title: KUDIG 项目整体重组行动计划 v2
description: 基于 LLM-Wiki 三层架构，将 topic 融入 domain，同时保留知识图谱层
category: report
tags:
- restructure
- domain
- topic
- llm-wiki
- knowledge-graph
- prometheus
- argocd
- falco
- llm
- rag
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 15min
intent_queries:
- KUDIG 项目整体重组行动计划 v2 是什么
- 如何 KUDIG 项目整体重组行动计划 v2
trigger_keywords:
- KUDIG
- 项目整体重组行动计划
- v2
prerequisites:
- kubectl-basics
- prometheus-basics
- gitops-basics
created: "2026-05-23"
---

# KUDIG 项目整体重组行动计划 v2

> **版本**: 2.0
> **日期**: 2026-05-21
> **总文件数**: 4,648 个 .md 文件
> **架构依据**: LLM-Wiki 三层架构（Raw → Wiki → Schema）

---

## 一、LLM-Wiki 架构审视

本项目本质上是一个 **LLM-Wiki（大模型知识库）**，不是普通的文档分类项目。其核心架构为三层：

```
Raw Sources → Wiki（知识蒸馏层） → Schema（元数据/导航层）
```

当前项目各层分布：

| 层级 | 对应目录 | 作用 |
|------|----------|------|
| **Raw** | `_raw/`, `_staging/` | 原始输入、待处理草稿 |
| **Wiki** | `domain-*/`, `topic-*/`, `concepts/`, `entities/`, `skills/`, `references/`, `synthesis/` | 蒸馏后的知识 |
| **Schema** | `_meta/`, `docs/`, `index.md`, `MOC.md`, `corpus-config/` | 导航、元数据、索引 |

### 核心问题

**Wiki 层内部分类混乱**：
- `domain-*`：知识域（纵向深度）— 合理
- `topic-*`：专题（横向切片）— 与 domain 并列，导致顶层目录爆炸（53 个）
- `concepts/`, `entities/`, `skills/`, `references/`, `synthesis/`：知识图谱节点 — 作为跨域层合理，但增加了顶层复杂度

**LLM-Wiki 的关键洞察**：
> 在 Obsidian/LLM-Wiki 中，**物理位置≠知识连接**。连接靠 `wikilinks` 和 frontmatter tags 实现，目录只负责"存放位置"。

所以重组目标是：**减少物理层级，但保留知识图谱结构**。

---

## 二、重组原则（基于 LLM-Wiki 架构）

### 原则 1：Topic 必须融入 Domain

`topic-*` 是横向切片，应作为 **domain 内部的子目录** 存在，而非与 domain 并列的顶层目录。

**原因**：
- Topic 的内容总是依附于某个知识域（如 "监控" 属于可观测性域，"问题树" 属于排障域）
- Domain 提供纵向学习路径，Topic 提供横向关联，两者是**包含关系**而非**并列关系**

### 原则 2：知识图谱层独立保留

`concepts/`, `entities/`, `skills/`, `references/`, `synthesis/` 作为 **跨域知识图谱节点层**，保留在根目录。

**原因**：
- 这些目录的内容天然跨域（如 `entities/prometheus.md` 同时关联 domain-06 监控、domain-17 基础概念）
- 在 LLM-Wiki 中，它们通过 `wikilinks` 连接到多个 domain，物理上打散会降低知识图谱的完整性
- 保留为独立层，便于 Dataview/Obsidian Query 统一检索

### 原则 3：Schema 层不动

`_meta/`, `docs/`, `_reports/`, `corpus-config/`, `index.md`, `MOC.md` 等属于 Schema 层，完全保留。

### 原则 4：零删除

只用 `mkdir -p` + `mv`，不删除任何文件。

---

## 三、目标架构

```
kudig-database/
│
│  Schema 层（完全保留）
├── _meta/                    # taxonomy, dashboard
├── docs/                     # 元文档、规范（FRONTMATTER-SPEC, TAG-DICTIONARY...）
├── _reports/                 # 评估报告、分析
├── corpus-config/            # 语料配置
├── _raw/                     # 原始输入
├── _staging/                 # 暂存区
├── README.md                 # 项目根导航
├── AGENTS.md                 # Agent 上下文
├── index.md                  # 总索引
├── MOC.md                    # 总知识地图
├── CHANGELOG.md
├── CONTRIBUTING.md
├── STRUCTURE.md
├── hot.md
├── log.md
│
│  知识图谱层（保留，跨域节点库）
├── concepts/                 # 62 个抽象概念（如多租户、零信任、服务网格）
├── entities/                 # 265 个实体（Prometheus, ArgoCD, Falco...）
├── skills/                   # 140 个操作技能（Node NotReady 诊断、存储配置...）
├── references/               # 102 个参考资料（术语表、sysctl 参考、API 文档）
├── synthesis/                # 29 个综合分析（跨域连接、趋势分析）
│
│  知识域层（唯一主分类）
├── domain-01-cluster-fundamentals/
│   ├── MOC.md
│   ├── README.md
│   ├── 00-99-xxx.md          # 核心域文档
│   └── （暂无数入 topic）
│
├── domain-02-workloads-applications/
│   ├── ...核心域文档...
│   ├── domain-02-workloads-applications/topic-functions/              # ← 迁入（80 个文件）
│   └── domain-02-workloads-applications/topic-java-kubernetes/        # ← 迁入（7 个文件）
│
├── domain-03-networking-traffic/
│   ├── ...核心域文档...
│   └── domain-03-networking-traffic/topic-terway/                 # ← 迁入（10 个文件）
│
├── domain-04-storage-data/
│   └── ...核心域文档...
│
├── domain-05-security-compliance/
│   └── ...核心域文档...
│
├── domain-06-observability/
│   └── ...核心域文档...
│
├── domain-07-platform-engineering/
│   ├── ...核心域文档...
│   └── domain-07-platform-engineering/topic-code-analysis/          # ← 迁入（89 个文件）
│
├── domain-08-release-change-management/
│   ├── ...核心域文档...
│   ├── domain-08-release-change-management/topic-deployment/             # ← 迁入（5 个文件）
│   └── domain-08-release-change-management/topic-migration/              # ← 迁入（11 个文件）
│
├── domain-09-reliability-engineering/
│   └── ...核心域文档...
│
├── domain-10-troubleshooting-diagnostics/
│   ├── ...核心域文档...
│   ├── domain-10-troubleshooting-diagnostics/topic-fta/                    # ← 迁入（81 个文件）
│   ├── domain-10-troubleshooting-diagnostics/topic-febm/                   # ← 迁入（11 个文件）
│   ├── domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/  # ← 迁入（72 个文件）
│   ├── domain-10-troubleshooting-diagnostics/topic-skills/                 # ← 迁入（42 个文件）
│   └── domain-10-troubleshooting-diagnostics/topic-qa-corpus/              # ← 迁入（4 个文件）
│
├── domain-11-production-operations/
│   ├── ...核心域文档...
│   ├── domain-11-production-operations/topic-best-practices/         # ← 迁入（55 个文件）
│   ├── domain-11-production-operations/topic-learn/                  # ← 迁入（151 个文件）
│   ├── domain-11-production-operations/topic-k8s-lecturer/           # ← 迁入（18 个文件）
│   ├── domain-11-production-operations/topic-presentations/          # ← 迁入（13 个文件）
│   ├── domain-11-production-operations/topic-publish/                # ← 迁入（12 个文件）
│   ├── journal/                      # ← 迁入（2 个文件）
│   └── projects/                     # ← 迁入（1 个文件）
│
├── domain-12-cloud-providers/
│   └── ...核心域文档...
│
├── domain-13-container-runtime/
│   └── ...核心域文档...
│
├── domain-14-ai-ml-infra/
│   ├── ...核心域文档...
│   ├── domain-14-ai-ml-infra/topic-ai-agent/               # ← 迁入（58 个文件）
│   └── domain-14-ai-ml-infra/topic-ai-coding/              # ← 迁入（25 个文件）
│
├── domain-15-specialized-tech/
│   └── ...核心域文档...
│
├── domain-16-database-middleware/
│   └── ...核心域文档...
│
├── domain-17-system-foundation/
│   ├── ...核心域文档...
│   ├── domain-17-system-foundation/topic-dictionary/             # ← 迁入（209 个文件）
│   └── domain-17-system-foundation/topic-cheat-sheet/            # ← 迁入（15 个文件）
│
├── domain-18-manifests-patterns/
│   └── ...核心域文档...
│
├── domain-19-landscape-references/
│   ├── ...核心域文档...
│   ├── domain-19-landscape-references/_archived-release-notes/          # ← 迁入（1,323 个文件）
│   └── domain-19-landscape-references/topic-index/                  # ← 迁入（25 个文件）
│
└── domain-20-application-patterns/
    ├── ...核心域文档...
    └── domain-20-application-patterns/topic-application-architecture/ # ← 迁入（97 个文件）
```

---

## 四、重组后顶层目录数量对比

| 类型 | 重组前 | 重组后 | 变化 |
|------|--------|--------|------|
| domain-* | 20 | 20 | — |
| topic-* | 23 | **0** | ✅ 全部融入 domain |
| 知识图谱层 | 5 | 5 | — |
| Schema 层 | 8 | 8 | — |
| 根目录 .md | 9 | 9 | — |
| **顶层目录总计** | **~65** | **~42** | **-35%** |

---

## 五、分阶段执行计划

### 安全铁律
- ✅ 只用 `mkdir -p` + `mv`
- ❌ 禁止任何 git 命令、禁止 `rm`、禁止 `rmdir`

### 阶段划分

| 阶段 | 内容 | 文件数 | 操作 |
|------|------|--------|------|
| **1** | Troubleshooting 核心 | 210 | `mv topic-fta topic-febm topic-structural-trouble-shooting topic-skills topic-qa-corpus domain-10-troubleshooting-diagnostics/` |
| **2** | Production Operations | 249 | `mv topic-best-practices topic-learn topic-k8s-lecturer topic-presentations topic-publish journal projects domain-11-production-operations/` |
| **3** | Release & Change | 16 | `mv topic-deployment topic-migration domain-08-release-change-management/` |
| **4** | System Foundation | 224 | `mv topic-dictionary topic-cheat-sheet domain-17-system-foundation/` |
| **5** | AI Infra | 83 | `mv topic-ai-agent topic-ai-coding domain-14-ai-ml-infra/` |
| **6** | Platform & Patterns | 273 | `mv topic-code-analysis domain-07-platform-engineering/`, `mv topic-application-architecture domain-20-application-patterns/`, `mv topic-functions topic-java-kubernetes domain-02-workloads-applications/` |
| **7** | Landscape | 1,348 | `mv topic-release-notes topic-index domain-19-landscape-references/` |
| **8** | Networking | 10 | `mv topic-terway domain-03-networking-traffic/` |
| **9** | 链接修复 | — | 全库脚本替换旧 topic 路径为新的 domain/topic 路径 |

---

## 六、知识图谱层的处理说明

`concepts/`、`entities/`、`skills/`、`references/`、`synthesis/` **保留在根目录**，理由：

1. **跨域特性**：`entities/prometheus.md` 同时连接 domain-06（监控）、domain-17（基础概念）、domain-20（架构模式），无法归入单一 domain
2. **Obsidian 查询便利**：保留为独立层后，可通过 Dataview 查询 `FROM "entities"` 获取所有实体
3. **LLM-Wiki 标准结构**：Karpathy 的 LLM-Wiki 本身就将 concepts/entities 作为与 domain 并存的层
4. **数量可控**：5 个目录共 598 个文件，作为根目录的知识图谱层清晰可管理

如果未来需要进一步精简，可通过 **frontmatter 标签** + **Dataview 查询** 实现虚拟分类，无需物理移动。

---

*本计划作为 LLM-Wiki 架构重组的执行依据，分阶段执行，每阶段需用户确认后进行。*
