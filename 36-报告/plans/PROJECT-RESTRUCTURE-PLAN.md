---
title: KUDIG 项目整体重组行动计划
description: 将所有 topic-* 目录融入 domain-*，建立单一 domain 分类体系
summary: 将所有 topic-* 目录融入 domain-*，建立单一 domain 分类体系
category: report
tags:
- restructure
- domain
- topic
- architecture
- rag
- agent
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 15min
intent_queries:
- KUDIG 项目整体重组行动计划 是什么
- 如何 KUDIG 项目整体重组行动计划
trigger_keywords:
- KUDIG
- 项目整体重组行动计划
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# KUDIG 项目整体重组行动计划

> **版本**: 1.0
> **日期**: 2026-05-21
> **总文件数**: 4,648 个 .md 文件
> **核心原则**: 只有 domain 一种顶层分类，所有 topic 融入 domain，不删除任何文件

---

## 一、现状分析

### 1.1 当前目录结构

| 类型 | 数量 | 文件数 | 说明 |
|------|------|--------|------|
| `domain-*` | 20 个 | 1,444 | 按知识域分类 |
| `topic-*` | 23 个 | 2,413 | 按专题/横向切片分类 |
| `concepts/` | 1 个 | 62 | 抽象概念 |
| `entities/` | 1 个 | 265 | 实体（工具/公司/项目） |
| `skills/` | 1 个 | 140 | 技能文档 |
| `references/` | 1 个 | 102 | 参考资料 |
| `docs/` | 1 个 | 35 | 元文档/规范 |
| `synthesis/` | 1 个 | 29 | 综合分析 |
| `_reports/` | 1 个 | 27 | 报告文件 |
| `journal/` | 1 个 | 2 | 日志 |
| `projects/` | 1 个 | 1 | 项目 |
| `corpus-config/` | 1 个 | 2 | 语料配置 |
| 根目录 .md | — | 9 | AGENTS.md, [[10-平台工程/06-代码分析/deployment-create/README.md|README]].md 等 |

### 1.2 核心问题

- **双重分类体系并存**: domain（纵向）+ topic（横向）+ concepts/entities/skills（交叉），导航混乱
- **topic 体量过大**: 2,413 个文件分布在 23 个 topic 中，部分 topic 文件数过多（如 topic-release-notes 1,323 个）
- **根目录层级过多**: 20 个 domain + 23 个 topic + 10 个其他目录 = 53 个顶层目录

---

## 二、重组原则

1. **单一分类**: 只有 `domain-*` 一种顶层目录，彻底消除 `topic-*` 顶层目录
2. **整体迁移**: 每个 topic 目录**整体移入**对应 domain（保留内部子目录结构，不打散重命名）
3. **横向目录归位**: `concepts/`、`entities/`、`skills/`、`references/`、`synthesis/` 按主题分散到相关 domain
4. **元数据保留**: `docs/`、`_reports/`、`corpus-config/` 保留在根目录（不属于知识域内容）
5. **导航文件保留**: 根目录 `README.md`、`AGENTS.md`、`index.md`、`MOC.md` 等保留
6. **零删除**: 只使用 `mkdir` + `mv`，不删除任何文件，不执行任何 git 命令

---

## 三、Topic → Domain 映射方案

### 3.1 映射总表

| # | Topic 目录 | 文件数 | 目标 Domain | 归属理由 |
|---|-----------|--------|-------------|----------|
| 1 | `故障诊断/topic-fta/` | 81 | `故障诊断/` | 问题树分析是排障核心方法论 |
| 2 | `故障诊断/topic-febm/` | 11 | `故障诊断/` | 问题排查工程方法论 |
| 3 | `故障诊断/topic-structural-trouble-shooting/` | 72 | `故障诊断/` | 结构化排查体系 |
| 4 | `故障诊断/topic-skills/` | 42 | `故障诊断/` | 运维技能卡片（排障导向） |
| 5 | `故障诊断/topic-qa-corpus/` | 4 | `故障诊断/` | QA 语料（排障能力评估） |
| 6 | `生产运维/topic-best-practices/` | 55 | `生产运维/` | 最佳实践属于生产运维 |
| 7 | `生产运维/topic-learn/` | 151 | `生产运维/` | 学习材料/培训内容 |
| 8 | `生产运维/topic-k8s-lecturer/` | 18 | `生产运维/` | 讲师培训材料 |
| 9 | `生产运维/topic-presentations/` | 13 | `生产运维/` | 演示文稿 |
| 10 | `生产运维/topic-publish/` | 12 | `生产运维/` | 发布与内容运营 |
| 11 | `发布变更/topic-deployment/` | 5 | `发布变更/` | 部署属于发布交付 |
| 12 | `发布变更/topic-migration/` | 11 | `发布变更/` | 迁移属于变更管理 |
| 13 | `系统基础/topic-dictionary/` | 209 | `系统基础/` | 术语词典属于基础概念 |
| 14 | `系统基础/topic-cheat-sheet/` | 15 | `系统基础/` | 速查表属于基础参考 |
| 15 | `工作负载/topic-functions/` | 80 | `工作负载/` | 函数/工作负载管理 |
| 16 | `工作负载/topic-java-kubernetes/` | 7 | `工作负载/` | Java on K8s 工作负载 |
| 17 | `AI基础设施/02-ai-agents/` | 58 | `AI基础设施/` | AI Agent 基础设施 |
| 18 | `AI基础设施/topic-ai-coding/` | 25 | `AI基础设施/` | AI 编码辅助 |
| 19 | `平台工程/topic-code-analysis/` | 89 | `平台工程/` | 代码分析属于平台工程 |
| 20 | `应用模式/topic-application-architecture/` | 97 | `应用模式/` | 应用架构模式 |
| 21 | `生态参考/_archived-release-notes/` | 1,323 | `生态参考/` | 发布说明属于生态全景 |
| 22 | `生态参考/topic-index/` | 25 | `生态参考/` | 索引属于生态参考 |
| 23 | `网络/topic-terway/` | 10 | `网络/` | Terway 网络插件 |

### 3.2 迁移后各 Domain 文件规模预估

| Domain | 原有 | 迁入 | 预估总计 |
|--------|------|------|----------|
| 故障诊断 | 219 | 210 | ~429 |
| 生产运维 | 12 | 249 | ~261 |
| 发布变更 | 36 | 16 | ~52 |
| 系统基础 | 53 | 224 | ~277 |
| 工作负载 | 41 | 87 | ~128 |
| AI基础设施 | 101 | 83 | ~184 |
| 平台工程 | 50 | 89 | ~139 |
| 应用模式 | 99 | 97 | ~196 |
| 生态参考 | 266 | 1,348 | ~1,614 |
| 网络 | 115 | 10 | ~125 |
| 其他 domain | ~443 | 0 | ~443 |

---

## 四、其他目录处理方案

### 4.1 保留在根目录（元数据/配置/报告）

| 目录 | 文件数 | 处理方案 |
|------|--------|----------|
| `docs/` | 35 | **保留** — 元文档、规范、字典（FRONTMATTER-SPEC.md, TAG-DICTIONARY.md 等） |
| `_reports/` | 27 | **保留** — 报告文件（评估报告、分析报告等） |
| `corpus-config/` | 2 | **保留** — 语料库配置 |
| `journal/` | 2 | **移入** `生产运维/journal/` |
| `projects/` | 1 | **移入** `生产运维/projects/` |

### 4.2 分散到各 Domain（按内容主题）

| 目录 | 文件数 | 分散策略 |
|------|--------|----------|
| `concepts/` | 62 | 按概念主题归入对应 domain（安全概念→domain-05，可观测概念→domain-06，平台概念→domain-07） |
| `entities/` | 265 | 按实体类型归入对应 domain（网络工具→domain-03，监控工具→domain-06，存储工具→domain-04，AI工具→domain-14） |
| `skills/` | 140 | 按技能主题归入对应 domain（排障技能→domain-10，运维技能→domain-11，网络技能→domain-03） |
| `references/` | 102 | 按参考主题归入对应 domain（存储参考→domain-04，网络参考→domain-03，监控参考→domain-06） |
| `synthesis/` | 29 | 按综合分析主题归入对应 domain |

### 4.3 根目录 .md 文件

保留以下导航/元数据文件在根目录：
- `README.md`、`AGENTS.md`、`CHANGELOG.md`、`CONTRIBUTING.md`、`STRUCTURE.md`
- `index.md`、`MOC.md`、`log.md`、`hot.md`

---

## 五、分阶段执行计划

### 5.1 执行原则

- **每阶段只移动 1-2 个 topic**，降低风险
- **每阶段结束后立即验证**：统计文件数，确认无丢失
- **不使用 git 命令**：只用 `mkdir -p` + `mv`
- **链接修复延后**：所有文件移动完成后，统一运行链接修复脚本

### 5.2 阶段划分（共 9 阶段）

#### 阶段 1: Troubleshooting 核心专题（低复杂度）
**目标**: 将 5 个排障相关 topic 移入 domain-10
**操作**:
```bash
mkdir -p 故障诊断/topic-fta
mkdir -p 故障诊断/topic-febm
mkdir -p 故障诊断/topic-structural-trouble-shooting
mkdir -p 故障诊断/topic-skills
mkdir -p 故障诊断/topic-qa-corpus
mv 故障诊断/topic-fta/* 故障诊断/topic-fta/
mv 故障诊断/topic-febm/* 故障诊断/topic-febm/
mv 故障诊断/topic-structural-trouble-shooting/* 故障诊断/topic-structural-trouble-shooting/
mv 故障诊断/topic-skills/* 故障诊断/topic-skills/
mv 故障诊断/topic-qa-corpus/* 故障诊断/topic-qa-corpus/
```
**文件数**: 210 个
**预计时间**: 5 分钟

#### 阶段 2: Production Operations 专题（中复杂度）
**目标**: 将 5 个运维/培训 topic + journal + projects 移入 domain-11
**操作**:
```bash
mv 生产运维/topic-best-practices/ 生产运维/
mv 生产运维/topic-learn/ 生产运维/
mv 生产运维/topic-k8s-lecturer/ 生产运维/
mv 生产运维/topic-presentations/ 生产运维/
mv 生产运维/topic-publish/ 生产运维/
mv journal/ 生产运维/
mv projects/ 生产运维/
```
**文件数**: 249 个
**预计时间**: 5 分钟

#### 阶段 3: Release & Change Management（低复杂度）
**目标**: deployment + migration → domain-08
**操作**:
```bash
mv 发布变更/topic-deployment/ 发布变更/
mv 发布变更/topic-migration/ 发布变更/
```
**文件数**: 16 个
**预计时间**: 2 分钟

#### 阶段 4: System Foundation（中复杂度）
**目标**: dictionary + cheat-sheet → domain-17
**操作**:
```bash
mv 系统基础/topic-dictionary/ 系统基础/
mv 系统基础/topic-cheat-sheet/ 系统基础/
```
**文件数**: 224 个
**预计时间**: 5 分钟

#### 阶段 5: AI & ML Infra（低复杂度）
**目标**: ai-agent + ai-coding → domain-14
**操作**:
```bash
mv AI基础设施/02-ai-agents/ AI基础设施/
mv AI基础设施/topic-ai-coding/ AI基础设施/
```
**文件数**: 83 个
**预计时间**: 2 分钟

#### 阶段 6: Platform & Patterns（中复杂度）
**目标**: code-analysis → domain-07, application-architecture → domain-20, functions + java-kubernetes → domain-02
**操作**:
```bash
mv 平台工程/topic-code-analysis/ 平台工程/
mv 应用模式/topic-application-architecture/ 应用模式/
mv 工作负载/topic-functions/ 工作负载/
mv 工作负载/topic-java-kubernetes/ 工作负载/
```
**文件数**: 273 个
**预计时间**: 5 分钟

#### 阶段 7: Landscape & References（高复杂度）
**目标**: release-notes + index → domain-19
**操作**:
```bash
mv 生态参考/_archived-release-notes/ 生态参考/
mv 生态参考/topic-index/ 生态参考/
```
**文件数**: 1,348 个
**预计时间**: 10 分钟

#### 阶段 8: Networking（低复杂度）
**目标**: terway → domain-03
**操作**:
```bash
mv 网络/topic-terway/ 网络/
```
**文件数**: 10 个
**预计时间**: 1 分钟

#### 阶段 9: 横向目录分散（高复杂度）
**目标**: concepts, entities, skills, references, synthesis 按主题分散
**策略**: 不一次性完成，而是按子目录分批迁移
```bash
# 示例：concepts/ 中的安全概念 → domain-05
mv concepts/security-* 安全/concepts/
mv concepts/observability-* 可观测性/concepts/
# ...（需逐文件分析）
```
**文件数**: 598 个
**预计时间**: 30 分钟（需人工判断每个文件归属）

---

## 六、安全操作规范

### 6.1 禁止操作

❌ **绝对禁止**:
- `git restore`、`git checkout-index`、`git reset --hard`
- `git mv`（改用普通 `mv`）
- `rm -rf`（任何删除操作）
- `rmdir`（空目录验证后手动删除）

### 6.2 允许操作

✅ **只允许**:
- `mkdir -p` — 创建目标目录
- `mv <source> <dest>/` — 移动文件/目录
- `ls`、`find`、`wc` — 验证文件完整性
- `grep` — 检查链接

### 6.3 每阶段验证清单

```bash
# 1. 移动前记录文件数
find topic-xxx -name "*.md" | wc -l

# 2. 执行移动
mv topic-xxx/ domain-yyy/

# 3. 移动后验证
find domain-yyy/topic-xxx -name "*.md" | wc -l
# 数值应与移动前一致

# 4. 确认原目录为空
ls -A topic-xxx/ 2>/dev/null || echo "Source dir removed or empty"
```

---

## 七、回滚方案

由于不使用 git 命令，回滚只能通过反向 `mv` 操作完成。

**回滚策略**:
1. 每阶段完成后，记录迁移映射（已包含在本计划中）
2. 如需回滚某阶段：
   ```bash
   mv 故障诊断/topic-fta/ ./故障诊断/topic-fta/
   ```
3. 最坏情况：从 git HEAD 恢复（`git checkout HEAD -- <file>`），但尽量避免

---

## 八、链接修复计划

所有文件移动完成后，统一执行链接修复：

```python
# 批量替换旧路径为新路径
# 示例：
# "故障诊断/topic-fta/01-xxx.md" → "故障诊断/topic-fta/01-xxx.md"
# "生产运维/topic-best-practices/security/pod-security.md" → "生产运维/生产运维/topic-best-practices/security/pod-security.md"
```

**修复范围**:
- 全库所有 .md 文件中的内部链接
- `mkdocs.yml` 导航配置
- `docs/indexes/` 索引文件
- `domain-*/README.md` 和 `MOC.md`

---

## 九、预期最终结构

```
kudig-database/
├── README.md, AGENTS.md, index.md, MOC.md ...  # 导航保留
├── docs/                                         # 元文档保留
├── _reports/                                     # 报告保留
├── corpus-config/                                # 配置保留
├── 集群基础/
│   ├── 00-open-source-projects-index.md
│   ├── 01-kubernetes-architecture-overview.md
│   └── ...
├── 工作负载/
│   ├── ...原有文件...
│   ├── 工作负载/topic-functions/          # ← 迁入
│   └── 工作负载/topic-java-kubernetes/    # ← 迁入
├── 网络/
│   ├── ...原有文件...
│   └── 网络/topic-terway/             # ← 迁入
├── 存储/
├── 安全/
│   └── concepts/                 # ← 分散迁入
├── 可观测性/
│   └── concepts/                 # ← 分散迁入
├── 平台工程/
│   ├── ...原有文件...
│   └── 平台工程/topic-code-analysis/      # ← 迁入
├── 发布变更/
│   ├── ...原有文件...
│   ├── 发布变更/topic-deployment/         # ← 迁入
│   └── 发布变更/topic-migration/          # ← 迁入
├── 可靠性/
├── 故障诊断/
│   ├── ...原有文件...
│   ├── 故障诊断/topic-fta/                # ← 迁入
│   ├── 故障诊断/topic-febm/               # ← 迁入
│   ├── 故障诊断/topic-structural-trouble-shooting/  # ← 迁入
│   ├── 故障诊断/topic-skills/             # ← 迁入
│   └── 故障诊断/topic-qa-corpus/          # ← 迁入
├── 生产运维/
│   ├── ...原有文件...
│   ├── 生产运维/topic-best-practices/     # ← 迁入
│   ├── 生产运维/topic-learn/              # ← 迁入
│   ├── 生产运维/topic-k8s-lecturer/       # ← 迁入
│   ├── 生产运维/topic-presentations/      # ← 迁入
│   ├── 生产运维/topic-publish/            # ← 迁入
│   ├── journal/                  # ← 迁入
│   └── projects/                 # ← 迁入
├── 云厂商/
├── 容器运行时/
├── AI基础设施/
│   ├── ...原有文件...
│   ├── AI基础设施/02-ai-agents/           # ← 迁入
│   └── AI基础设施/topic-ai-coding/          # ← 迁入
├── 专项技术/
├── 数据库中间件/
├── 系统基础/
│   ├── ...原有文件...
│   ├── 系统基础/topic-dictionary/         # ← 迁入
│   └── 系统基础/topic-cheat-sheet/        # ← 迁入
├── 清单模式/
├── 生态参考/
│   ├── ...原有文件...
│   ├── 生态参考/_archived-release-notes/      # ← 迁入
│   └── 生态参考/topic-index/              # ← 迁入
└── 应用模式/
    ├── ...原有文件...
    └── 应用模式/topic-application-architecture/  # ← 迁入
```

---

*本计划作为项目重组的执行依据，分阶段执行，每阶段需用户确认后进行。*


<!-- risk-assessed -->
