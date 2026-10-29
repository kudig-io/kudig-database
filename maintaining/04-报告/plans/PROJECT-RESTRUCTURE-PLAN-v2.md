---
title: KUDIG 项目整体重组行动计划 v2
description: 基于 LLM-Wiki 三层架构，将 topic 融入 domain，同时保留知识图谱层
summary: 基于 LLM-Wiki 三层架构，将 topic 融入 domain，同时保留知识图谱层
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
tier: peripheral
created: '2026-05-23'
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
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




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
├── 集群基础/
│   ├── MOC.md
│   ├── README.md
│   ├── 00-99-xxx.md          # 核心域文档
│   └── （暂无数入 topic）
│
├── 工作负载/
│   ├── ...核心域文档...
│   ├── 工作负载/topic-functions/              # ← 迁入（80 个文件）
│   └── 工作负载/topic-java-kubernetes/        # ← 迁入（7 个文件）
│
├── 网络/
│   ├── ...核心域文档...
│   └── 网络/topic-terway/                 # ← 迁入（10 个文件）
│
├── 存储/
│   └── ...核心域文档...
│
├── 安全/
│   └── ...核心域文档...
│
├── 可观测性/
│   └── ...核心域文档...
│
├── 平台工程/
│   ├── ...核心域文档...
│   └── 平台工程/topic-code-analysis/          # ← 迁入（89 个文件）
│
├── 发布变更/
│   ├── ...核心域文档...
│   ├── 发布变更/topic-deployment/             # ← 迁入（5 个文件）
│   └── 发布变更/topic-migration/              # ← 迁入（11 个文件）
│
├── 可靠性/
│   └── ...核心域文档...
│
├── 故障诊断/
│   ├── ...核心域文档...
│   ├── 故障诊断/topic-fta/                    # ← 迁入（81 个文件）
│   ├── 故障诊断/topic-febm/                   # ← 迁入（11 个文件）
│   ├── 故障诊断/topic-structural-trouble-shooting/  # ← 迁入（72 个文件）
│   ├── 故障诊断/topic-skills/                 # ← 迁入（42 个文件）
│   └── 故障诊断/topic-qa-corpus/              # ← 迁入（4 个文件）
│
├── 生产运维/
│   ├── ...核心域文档...
│   ├── 生产运维/topic-best-practices/         # ← 迁入（55 个文件）
│   ├── 生产运维/topic-learn/                  # ← 迁入（151 个文件）
│   ├── 生产运维/topic-k8s-lecturer/           # ← 迁入（18 个文件）
│   ├── 生产运维/topic-presentations/          # ← 迁入（13 个文件）
│   ├── 生产运维/topic-publish/                # ← 迁入（12 个文件）
│   ├── journal/                      # ← 迁入（2 个文件）
│   └── projects/                     # ← 迁入（1 个文件）
│
├── 云厂商/
│   └── ...核心域文档...
│
├── 容器运行时/
│   └── ...核心域文档...
│
├── AI基础设施/
│   ├── ...核心域文档...
│   ├── AI基础设施/02-ai-agents/               # ← 迁入（58 个文件）
│   └── AI基础设施/topic-ai-coding/              # ← 迁入（25 个文件）
│
├── 专项技术/
│   └── ...核心域文档...
│
├── 数据库中间件/
│   └── ...核心域文档...
│
├── 系统基础/
│   ├── ...核心域文档...
│   ├── 系统基础/topic-dictionary/             # ← 迁入（209 个文件）
│   └── 系统基础/topic-cheat-sheet/            # ← 迁入（15 个文件）
│
├── 清单模式/
│   └── ...核心域文档...
│
├── 生态参考/
│   ├── ...核心域文档...
│   ├── 生态参考/_archived-release-notes/          # ← 迁入（1,323 个文件）
│   └── 生态参考/topic-index/                  # ← 迁入（25 个文件）
│
└── 应用模式/
    ├── ...核心域文档...
    └── 应用模式/topic-application-architecture/ # ← 迁入（97 个文件）
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
| **1** | Troubleshooting 核心 | 210 | `mv topic-fta topic-febm topic-structural-trouble-shooting topic-skills topic-qa-corpus 故障诊断/` |
| **2** | Production Operations | 249 | `mv topic-best-practices topic-learn topic-k8s-lecturer topic-presentations topic-publish journal projects 生产运维/` |
| **3** | Release & Change | 16 | `mv topic-deployment topic-migration 发布变更/` |
| **4** | System Foundation | 224 | `mv topic-dictionary topic-cheat-sheet 系统基础/` |
| **5** | AI Infra | 83 | `mv 02-ai-agents topic-ai-coding AI基础设施/` |
| **6** | Platform & Patterns | 273 | `mv topic-code-analysis 平台工程/`, `mv topic-application-architecture 应用模式/`, `mv topic-functions topic-java-kubernetes 工作负载/` |
| **7** | Landscape | 1,348 | `mv topic-release-notes topic-index 生态参考/` |
| **8** | Networking | 10 | `mv topic-terway 网络/` |
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


<!-- risk-assessed -->
