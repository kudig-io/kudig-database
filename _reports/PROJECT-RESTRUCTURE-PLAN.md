---
title: KUDIG 项目整体重组行动计划
description: 将所有 topic-* 目录融入 domain-*，建立单一 domain 分类体系
category: report
tags:
- restructure
- domain
- topic
- architecture
- rag
- agent
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
created: "2026-05-23"
---

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
| 根目录 .md | — | 9 | AGENTS.md, [[domain-07-platform-engineering/topic-code-analysis/deployment-create/README|README]].md 等 |

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
| 1 | `domain-10-troubleshooting-diagnostics/topic-fta/` | 81 | `domain-10-troubleshooting-diagnostics/` | 问题树分析是排障核心方法论 |
| 2 | `domain-10-troubleshooting-diagnostics/topic-febm/` | 11 | `domain-10-troubleshooting-diagnostics/` | 问题排查工程方法论 |
| 3 | `domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/` | 72 | `domain-10-troubleshooting-diagnostics/` | 结构化排查体系 |
| 4 | `domain-10-troubleshooting-diagnostics/topic-skills/` | 42 | `domain-10-troubleshooting-diagnostics/` | 运维技能卡片（排障导向） |
| 5 | `domain-10-troubleshooting-diagnostics/topic-qa-corpus/` | 4 | `domain-10-troubleshooting-diagnostics/` | QA 语料（排障能力评估） |
| 6 | `domain-11-production-operations/topic-best-practices/` | 55 | `domain-11-production-operations/` | 最佳实践属于生产运维 |
| 7 | `domain-11-production-operations/topic-learn/` | 151 | `domain-11-production-operations/` | 学习材料/培训内容 |
| 8 | `domain-11-production-operations/topic-k8s-lecturer/` | 18 | `domain-11-production-operations/` | 讲师培训材料 |
| 9 | `domain-11-production-operations/topic-presentations/` | 13 | `domain-11-production-operations/` | 演示文稿 |
| 10 | `domain-11-production-operations/topic-publish/` | 12 | `domain-11-production-operations/` | 发布与内容运营 |
| 11 | `domain-08-release-change-management/topic-deployment/` | 5 | `domain-08-release-change-management/` | 部署属于发布交付 |
| 12 | `domain-08-release-change-management/topic-migration/` | 11 | `domain-08-release-change-management/` | 迁移属于变更管理 |
| 13 | `domain-17-system-foundation/topic-dictionary/` | 209 | `domain-17-system-foundation/` | 术语词典属于基础概念 |
| 14 | `domain-17-system-foundation/topic-cheat-sheet/` | 15 | `domain-17-system-foundation/` | 速查表属于基础参考 |
| 15 | `domain-02-workloads-applications/topic-functions/` | 80 | `domain-02-workloads-applications/` | 函数/工作负载管理 |
| 16 | `domain-02-workloads-applications/topic-java-kubernetes/` | 7 | `domain-02-workloads-applications/` | Java on K8s 工作负载 |
| 17 | `domain-14-ai-ml-infra/topic-ai-agent/` | 58 | `domain-14-ai-ml-infra/` | AI Agent 基础设施 |
| 18 | `domain-14-ai-ml-infra/topic-ai-coding/` | 25 | `domain-14-ai-ml-infra/` | AI 编码辅助 |
| 19 | `domain-07-platform-engineering/topic-code-analysis/` | 89 | `domain-07-platform-engineering/` | 代码分析属于平台工程 |
| 20 | `domain-20-application-patterns/topic-application-architecture/` | 97 | `domain-20-application-patterns/` | 应用架构模式 |
| 21 | `domain-19-landscape-references/_archived-release-notes/` | 1,323 | `domain-19-landscape-references/` | 发布说明属于生态全景 |
| 22 | `domain-19-landscape-references/topic-index/` | 25 | `domain-19-landscape-references/` | 索引属于生态参考 |
| 23 | `domain-03-networking-traffic/topic-terway/` | 10 | `domain-03-networking-traffic/` | Terway 网络插件 |

### 3.2 迁移后各 Domain 文件规模预估

| Domain | 原有 | 迁入 | 预估总计 |
|--------|------|------|----------|
| domain-10-troubleshooting-diagnostics | 219 | 210 | ~429 |
| domain-11-production-operations | 12 | 249 | ~261 |
| domain-08-release-change-management | 36 | 16 | ~52 |
| domain-17-system-foundation | 53 | 224 | ~277 |
| domain-02-workloads-applications | 41 | 87 | ~128 |
| domain-14-ai-ml-infra | 101 | 83 | ~184 |
| domain-07-platform-engineering | 50 | 89 | ~139 |
| domain-20-application-patterns | 99 | 97 | ~196 |
| domain-19-landscape-references | 266 | 1,348 | ~1,614 |
| domain-03-networking-traffic | 115 | 10 | ~125 |
| 其他 domain | ~443 | 0 | ~443 |

---

## 四、其他目录处理方案

### 4.1 保留在根目录（元数据/配置/报告）

| 目录 | 文件数 | 处理方案 |
|------|--------|----------|
| `docs/` | 35 | **保留** — 元文档、规范、字典（FRONTMATTER-SPEC.md, TAG-DICTIONARY.md 等） |
| `_reports/` | 27 | **保留** — 报告文件（评估报告、分析报告等） |
| `corpus-config/` | 2 | **保留** — 语料库配置 |
| `journal/` | 2 | **移入** `domain-11-production-operations/journal/` |
| `projects/` | 1 | **移入** `domain-11-production-operations/projects/` |

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
mkdir -p domain-10-troubleshooting-diagnostics/topic-fta
mkdir -p domain-10-troubleshooting-diagnostics/topic-febm
mkdir -p domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting
mkdir -p domain-10-troubleshooting-diagnostics/topic-skills
mkdir -p domain-10-troubleshooting-diagnostics/topic-qa-corpus
mv domain-10-troubleshooting-diagnostics/topic-fta/* domain-10-troubleshooting-diagnostics/topic-fta/
mv domain-10-troubleshooting-diagnostics/topic-febm/* domain-10-troubleshooting-diagnostics/topic-febm/
mv domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/* domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/
mv domain-10-troubleshooting-diagnostics/topic-skills/* domain-10-troubleshooting-diagnostics/topic-skills/
mv domain-10-troubleshooting-diagnostics/topic-qa-corpus/* domain-10-troubleshooting-diagnostics/topic-qa-corpus/
```
**文件数**: 210 个
**预计时间**: 5 分钟

#### 阶段 2: Production Operations 专题（中复杂度）
**目标**: 将 5 个运维/培训 topic + journal + projects 移入 domain-11
**操作**:
```bash
mv domain-11-production-operations/topic-best-practices/ domain-11-production-operations/
mv domain-11-production-operations/topic-learn/ domain-11-production-operations/
mv domain-11-production-operations/topic-k8s-lecturer/ domain-11-production-operations/
mv domain-11-production-operations/topic-presentations/ domain-11-production-operations/
mv domain-11-production-operations/topic-publish/ domain-11-production-operations/
mv journal/ domain-11-production-operations/
mv projects/ domain-11-production-operations/
```
**文件数**: 249 个
**预计时间**: 5 分钟

#### 阶段 3: Release & Change Management（低复杂度）
**目标**: deployment + migration → domain-08
**操作**:
```bash
mv domain-08-release-change-management/topic-deployment/ domain-08-release-change-management/
mv domain-08-release-change-management/topic-migration/ domain-08-release-change-management/
```
**文件数**: 16 个
**预计时间**: 2 分钟

#### 阶段 4: System Foundation（中复杂度）
**目标**: dictionary + cheat-sheet → domain-17
**操作**:
```bash
mv domain-17-system-foundation/topic-dictionary/ domain-17-system-foundation/
mv domain-17-system-foundation/topic-cheat-sheet/ domain-17-system-foundation/
```
**文件数**: 224 个
**预计时间**: 5 分钟

#### 阶段 5: AI & ML Infra（低复杂度）
**目标**: ai-agent + ai-coding → domain-14
**操作**:
```bash
mv domain-14-ai-ml-infra/topic-ai-agent/ domain-14-ai-ml-infra/
mv domain-14-ai-ml-infra/topic-ai-coding/ domain-14-ai-ml-infra/
```
**文件数**: 83 个
**预计时间**: 2 分钟

#### 阶段 6: Platform & Patterns（中复杂度）
**目标**: code-analysis → domain-07, application-architecture → domain-20, functions + java-kubernetes → domain-02
**操作**:
```bash
mv domain-07-platform-engineering/topic-code-analysis/ domain-07-platform-engineering/
mv domain-20-application-patterns/topic-application-architecture/ domain-20-application-patterns/
mv domain-02-workloads-applications/topic-functions/ domain-02-workloads-applications/
mv domain-02-workloads-applications/topic-java-kubernetes/ domain-02-workloads-applications/
```
**文件数**: 273 个
**预计时间**: 5 分钟

#### 阶段 7: Landscape & References（高复杂度）
**目标**: release-notes + index → domain-19
**操作**:
```bash
mv domain-19-landscape-references/_archived-release-notes/ domain-19-landscape-references/
mv domain-19-landscape-references/topic-index/ domain-19-landscape-references/
```
**文件数**: 1,348 个
**预计时间**: 10 分钟

#### 阶段 8: Networking（低复杂度）
**目标**: terway → domain-03
**操作**:
```bash
mv domain-03-networking-traffic/topic-terway/ domain-03-networking-traffic/
```
**文件数**: 10 个
**预计时间**: 1 分钟

#### 阶段 9: 横向目录分散（高复杂度）
**目标**: concepts, entities, skills, references, synthesis 按主题分散
**策略**: 不一次性完成，而是按子目录分批迁移
```bash
# 示例：concepts/ 中的安全概念 → domain-05
mv concepts/security-* domain-05-security-compliance/concepts/
mv concepts/observability-* domain-06-observability/concepts/
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
   mv domain-10-troubleshooting-diagnostics/topic-fta/ ./domain-10-troubleshooting-diagnostics/topic-fta/
   ```
3. 最坏情况：从 git HEAD 恢复（`git checkout HEAD -- <file>`），但尽量避免

---

## 八、链接修复计划

所有文件移动完成后，统一执行链接修复：

```python
# 批量替换旧路径为新路径
# 示例：
# "domain-10-troubleshooting-diagnostics/topic-fta/01-xxx.md" → "domain-10-troubleshooting-diagnostics/topic-fta/01-xxx.md"
# "domain-11-production-operations/topic-best-practices/security/pod-security.md" → "domain-11-production-operations/domain-11-production-operations/topic-best-practices/security/pod-security.md"
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
├── domain-01-cluster-fundamentals/
│   ├── 00-open-source-projects-index.md
│   ├── 01-kubernetes-architecture-overview.md
│   └── ...
├── domain-02-workloads-applications/
│   ├── ...原有文件...
│   ├── domain-02-workloads-applications/topic-functions/          # ← 迁入
│   └── domain-02-workloads-applications/topic-java-kubernetes/    # ← 迁入
├── domain-03-networking-traffic/
│   ├── ...原有文件...
│   └── domain-03-networking-traffic/topic-terway/             # ← 迁入
├── domain-04-storage-data/
├── domain-05-security-compliance/
│   └── concepts/                 # ← 分散迁入
├── domain-06-observability/
│   └── concepts/                 # ← 分散迁入
├── domain-07-platform-engineering/
│   ├── ...原有文件...
│   └── domain-07-platform-engineering/topic-code-analysis/      # ← 迁入
├── domain-08-release-change-management/
│   ├── ...原有文件...
│   ├── domain-08-release-change-management/topic-deployment/         # ← 迁入
│   └── domain-08-release-change-management/topic-migration/          # ← 迁入
├── domain-09-reliability-engineering/
├── domain-10-troubleshooting-diagnostics/
│   ├── ...原有文件...
│   ├── domain-10-troubleshooting-diagnostics/topic-fta/                # ← 迁入
│   ├── domain-10-troubleshooting-diagnostics/topic-febm/               # ← 迁入
│   ├── domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/  # ← 迁入
│   ├── domain-10-troubleshooting-diagnostics/topic-skills/             # ← 迁入
│   └── domain-10-troubleshooting-diagnostics/topic-qa-corpus/          # ← 迁入
├── domain-11-production-operations/
│   ├── ...原有文件...
│   ├── domain-11-production-operations/topic-best-practices/     # ← 迁入
│   ├── domain-11-production-operations/topic-learn/              # ← 迁入
│   ├── domain-11-production-operations/topic-k8s-lecturer/       # ← 迁入
│   ├── domain-11-production-operations/topic-presentations/      # ← 迁入
│   ├── domain-11-production-operations/topic-publish/            # ← 迁入
│   ├── journal/                  # ← 迁入
│   └── projects/                 # ← 迁入
├── domain-12-cloud-providers/
├── domain-13-container-runtime/
├── domain-14-ai-ml-infra/
│   ├── ...原有文件...
│   ├── domain-14-ai-ml-infra/topic-ai-agent/           # ← 迁入
│   └── domain-14-ai-ml-infra/topic-ai-coding/          # ← 迁入
├── domain-15-specialized-tech/
├── domain-16-database-middleware/
├── domain-17-system-foundation/
│   ├── ...原有文件...
│   ├── domain-17-system-foundation/topic-dictionary/         # ← 迁入
│   └── domain-17-system-foundation/topic-cheat-sheet/        # ← 迁入
├── domain-18-manifests-patterns/
├── domain-19-landscape-references/
│   ├── ...原有文件...
│   ├── domain-19-landscape-references/_archived-release-notes/      # ← 迁入
│   └── domain-19-landscape-references/topic-index/              # ← 迁入
└── domain-20-application-patterns/
    ├── ...原有文件...
    └── domain-20-application-patterns/topic-application-architecture/  # ← 迁入
```

---

*本计划作为项目重组的执行依据，分阶段执行，每阶段需用户确认后进行。*
