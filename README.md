---
title: KUDIG Database
summary: 面向生产环境的 Kubernetes + AI Infrastructure 运维全域知识库，既是人类可读的运维手册，也是 AI Agent 的 RAG 语料来源。
category: index
tags:
- readme
- index
- k8s
- ai
- agent
- ops
tier: core
created: '2026-07-01'
last_updated: '2026-08-21'
---

# KUDIG Database

> 面向生产环境的 **Kubernetes + AI Infrastructure** 运维全域知识库。
> 既是人类可读的运维手册，也是 **AI Agent 的 RAG 语料来源**。

[![Deploy to GitHub Pages](https://github.com/kudig-io/kudig-database/actions/workflows/deploy-pages.yml/badge.svg)](https://github.com/kudig-io/kudig-database/actions/workflows/deploy-pages.yml)
[![Quality Check](https://github.com/kudig-io/kudig-database/actions/workflows/quality.yml/badge.svg)](https://github.com/kudig-io/kudig-database/actions/workflows/quality.yml)
[![License](https://img.shields.io/badge/License-Apache%202.0-blue.svg)](LICENSE)

---

## 项目概览

| 维度 | 说明 |
|------|------|
| 定位 | Kubernetes 生产运维 + AI Infrastructure 全域知识库，人类与 AI Agent 双读 |
| 规模 | **~4,750 篇** 活跃知识文档，覆盖 **21 个核心技术域** + 4 层 wiki 提炼知识 |
| 架构 | 双层结构：**源文档层**（深度技术文档） + **提炼知识层**（Agent 优先消费） |
| 语料 | 附带 RAG 语料导出 pipeline，可直接供 AI Agent 检索增强 |
| 站点 | [GitHub Pages](https://kudig-io.github.io/kudig-database/) 自动部署，基于 Astro 构建 |
| 版本 | Kubernetes v1.25 ~ v1.32+，覆盖标准 K8s + ACK/Terway/ASM 扩展 |

---

## 知识架构

### 双层知识体系

```
┌────────────────────────────────────────────────────────────────────────────┐
│  提炼知识层 (Agent 优先读取，Token 高效)                                    │
│                                                                            │
│  22-概念/    核心概念、架构模式、设计原理    306 篇                          │
│  23-实体/    组件实体、CNCF 工具、云产品     407 篇                          │
│  24-综合/    跨领域综合分析                  51 篇                           │
│  26-技能/    诊断排障、FTA 方法、最佳实践    578 篇                          │
│  25-研究/    研究性笔记、调研报告             33 篇                           │
├────────────────────────────────────────────────────────────────────────────┤
│  源文档层 (21 个技术知识域，深度查询兜底)                                    │
│                                                                            │
│  Tier 1 —— 核心技术域                                                      │
│  01-集群基础    02-工作负载    05-网络          06-存储                      │
│  08-安全        09-可观测性    03-清单模式      04-应用模式                  │
│                                                                            │
│  Tier 2 —— 平台与工程域                                                    │
│  10-平台工程    11-发布变更    12-可靠性                                    │
│                                                                            │
│  Tier 3 —— 运维场景域                                                      │
│  13-生产运维    19-故障诊断                                                 │
│                                                                            │
│  Tier 4 —— 部署与生态域                                                    │
│  14-容器运行时  15-AI基础设施  16-专项技术      07-数据库中间件              │
│  18-云厂商      20-最佳实践    21-生态参考      17-系统基础                  │
└────────────────────────────────────────────────────────────────────────────┘
```

### 结构化问题解决引擎

KUDIG 的差异化能力在于其结构化排障引擎：

1. **FTA 故障树分析** — 16 个顶层故障事件的故障树 + 动态概率推理
2. **FEBM 法医证据方法** — 从症状特征向量匹配到根因确认的证据链
3. **诊断技能体系** — 19+ 张诊断技能卡，覆盖常见生产场景
4. **结构化排障** — 症状映射层，支持多维度交叉定位
5. **QA 语料生成** — 脚本化生成 I-O 配对语料，用于 Agent 训练与评估

---

## 核心统计

| 指标 | 数值 |
|------|------|
| 活跃知识文档 | **~4,747 篇** Markdown |
| 核心技术域 | **21 个**（01-21） |
| wiki 提炼层 | 概念 306 + 实体 407 + 技能 578 + 综合 51 |
| 云厂商覆盖 | **13 家**（阿里云 / AWS / GCP / Azure / 腾讯云 / 华为云 / 多云混合） |
| 系统基础 | **646 篇**（Linux / 硬件 / 网络 / 事件 / 速查卡 / 知识字典） |
| 故障诊断 | **491 篇**（FTA / FEBM / 技能体系 / 场景语料） |
| CI GitHub Stars | 5 ★ |
| License | Apache 2.0 |

---

## 生产就绪快速入口

每个知识域目录下均提供运维入口，另有关键跨域 Runbook：

| Runbook | 位置 |
|---------|------|
| 证书 / PKI 生命周期 | `01-集群基础/03-控制平面/38-certificate-pki-lifecycle-runbook.md` |
| 集群升级 | `01-集群基础/03-控制平面/39-cluster-upgrade-runbook.md` |
| 灾难恢复与业务连续性 | `12-可靠性/02-灾难恢复/25-disaster-recovery-bc-runbook-v2.md` |
| Fleet GitOps 操作指南 | `11-发布变更/01-GitOps/10-fleet-gitops-operations-guide.md` |
| 事件响应模板 | `13-生产运维/03-事件响应/11-incident-response-runbook-template.md` |
| FinOps 成本治理 | `13-生产运维/01-成本治理/06-finops-cost-governance-runbook.md` |
| AI/ML 运维 | `15-AI基础设施/01-基础设施/38-ai-ml-ops-runbook.md` |
| 边缘生产运维 | `16-专项技术/01-边缘计算/12-edge-production-runbook.md` |

各域入口级 `99-production-readiness-operations-guide.md` 提供领域就绪指南。

---

## AI Agent 集成

KUDIG 从设计之初即为 AI Agent 优化：

- **Agent 唤醒协议** — 见 [`35-元数据/AGENTS.md`](35-元数据/AGENTS.md)，定义诊断工作流、优先级判定、多 Agent 协作规则
- **RAG 语料导出** — 支持按 profile 分块导出，供向量化 Pipeline 消费
- **诊断工作流** — 五阶段标准化流程：信息采集 → 根因分析 → 方案生成 → 安全评审 → 输出闭环
- **FTA 集成** — 故障树可被 Agent 直接遍历推理，支持贝叶斯概率更新
- **QA 语料** — `19-故障诊断/10-QA语料/` 下生成的结构化 I-O 语料，用于 Agent 评估

---

## 快速开始

### 浏览在线站点

访问自动部署的 GitHub Pages 站点：

<https://kudig-io.github.io/kudig-database/>

支持 Pagefind 客户端搜索、Shiki 语法高亮、Mermaid 图表。

### 本地开发

```bash
# 进入 Astro 站点目录
cd 30-站点

# 安装依赖
npm install

# 启动开发服务器（热重载，默认 http://localhost:4321）
npm run dev

# 构建生产静态产物
npm run build

# 本地预览构建产物
npm run preview
```

### 仅阅读 Markdown 源文件

所有知识内容均为纯 Markdown，可直接在任意编辑器中阅读。推荐使用 [Obsidian](https://obsidian.md/) —— wikilink `[[...]]` 可被原生解析。

```bash
# 直接浏览任意目录下的 .md 文件
open 01-集群基础/README.md
```

### 语料生成（可选）

```bash
# 生成 P0 优先级故障诊断 QA 语料
make corpus-generate-p0

# 生成全量 QA 语料
make corpus-generate-all

# 验证语料覆盖率
make corpus-validate
```

---

## 目录结构

```
.
├── 01-集群基础/ … 21-生态参考/   # 源文档层：21 个技术域文档（NN-中文目录名）
│   └── NN-子目录/                 # 域内二级分类（同样 NN- 前缀有序化）
├── 22-概念/                      # 提炼知识：核心概念、架构模式
├── 23-实体/                      # 提炼知识：组件实体、CNCF 工具、云产品
├── 24-综合/                      # 提炼知识：跨领域综合分析
├── 25-研究/                      # 研究资料、调研报告
├── 26-技能/                      # 提炼知识：诊断排障技能卡、FTA 方法
├── 27-标签/                      # 标签索引页
├── 28-资产/                      # 图片、图表、PDF 附件
├── 29-文档/                      # 项目级说明文档（CHANGELOG、CONTRIBUTING）
├── 30-站点/                      # Astro 静态站点项目（.gitignore 忽略）
├── 31-脚本/                      # 自动化维护脚本（lint、前缀重命名、语料生成）
├── 32-发布/                      # 发布产物（冻结，只增不改）
├── 33-源码/                      # vendor 源码树（Kubernetes、terway 等，.gitignore 忽略）
├── 34-源码分析/                  # 源码分析笔记
├── 35-元数据/                    # 元数据、语料配置、taxonomy、schema、journal
├── 36-报告/                      # 质量报告与评估（冻结）
└── 37-归档/                      # Wiki 归档快照（冻结，重建用）
```

完整目录映射与命名规范详见 [`35-元数据/metadata/domain-mapping.md`](35-元数据/metadata/domain-mapping.md)。

### 命名约定

- **一级目录**：`NN-中文简称`（01-37 有序化前缀）
- **二级目录**（知识域内）：同样 `NN-` 前缀；英文缩写/工具名保留（如 `GitOps`、`IaC`、`eBPF`）
- **文件名**：`kebab-case.md`，ASCII 字符
- **入口文件**：每个域根目录有 `README.md` 或 `index.md`

---

## CI/CD 与质量保障

| 工作流 | 触发条件 | 职责 |
|--------|----------|------|
| **Deploy to GitHub Pages** | 推送 `main` 分支 | 构建 Astro 站点并部署到 GitHub Pages |
| **Quality Check** | 推送 / PR | ruff lint + frontmatter 完整性 + broken wikilink 检查（gating） |
| **Corpus Coverage Check** | 推送 / PR | 语料覆盖率验证（阻断式） |

本地质量检查：

```bash
# Python 脚本 lint
ruff check scripts/

# Frontmatter 完整性验证
python3 scripts/frontmatter-quality-check.py

# Broken wikilink 检查
bash scripts/check-broken-links.sh

# 代码块语法验证
bash scripts/code-example-validation.sh
```

---

## 贡献

欢迎提交 Issue 和 PR。贡献前请阅读：

- [`29-文档/CONTRIBUTING.md`](29-文档/CONTRIBUTING.md) — 贡献流程、提交规范、质量标准
- [`35-元数据/metadata/domain-mapping.md`](35-元数据/metadata/domain-mapping.md) — 目录约定
- [`35-元数据/metadata/schema.md`](35-元数据/metadata/schema.md) — Frontmatter 元数据规范
- [`35-元数据/metadata/taxonomy.md`](35-元数据/metadata/taxonomy.md) — Tag 分类体系

### 提交规范

遵循 [Conventional Commits](https://www.conventionalcommits.org/)：

| 前缀 | 用途 |
|------|------|
| `feat:` | 新增内容 / 功能 |
| `fix:` | 修复错误（技术错误、broken link、frontmatter） |
| `docs:` | 文档变更（README、CHANGELOG） |
| `chore:` | 依赖、清理 |
| `ci:` | CI / 构建链路 |
| `dedup:` | 去重 / 合并 |

---

## 相关链接

- [在线站点](https://kudig-io.github.io/kudig-database/)
- [GitHub 仓库](https://github.com/kudig-io/kudig-database)
- [首页索引](index.md) — 完整的 Wiki 导航入口
- [35-元数据/metadata/KUDIG Knowledge Base Architecture.md](35-元数据/metadata/KUDIG%20Knowledge%20Base%20Architecture.md) — 知识库架构文档

## License

[Apache License 2.0](LICENSE)

版权所有 2026 KUDIG Team