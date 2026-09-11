---
title: KUDIG Database
summary: 面向生产环境的 Kubernetes + AI Infrastructure 运维全域知识库——既是人类可读的运维手册，也是 AI Agent 的 RAG 语料来源。
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
last_updated: '2026-09-09'
---

# KUDIG Database

> **生产的每一站，都有答案。**
> 面向生产环境的 **Kubernetes + AI Infrastructure** 运维全域知识库——既是人类可读的运维手册，也是 **AI Agent 的 RAG 语料来源**。

[![Quality Gate](https://github.com/kudig-io/kudig-database/actions/workflows/quality.yml/badge.svg)](https://github.com/kudig-io/kudig-database/actions/workflows/quality.yml)
[![Nightly Corpus](https://github.com/kudig-io/kudig-database/actions/workflows/nightly-corpus.yml/badge.svg)](https://github.com/kudig-io/kudig-database/actions/workflows/nightly-corpus.yml)
[![Secret Scan](https://github.com/kudig-io/kudig-database/actions/workflows/secret-scan.yml/badge.svg)](https://github.com/kudig-io/kudig-database/actions/workflows/secret-scan.yml)
[![License](https://img.shields.io/badge/License-Apache%202.0-blue.svg)](LICENSE)

**KUDIG = KUbernetes + DIAGnosis**。整个仓库围绕一个核心命题构建：生产环境里的每一个故障，都应该有一条**可复现、有证据链、可被机器执行**的排查路径。为此它做了三件普通文档库不做的事：

1. **结构化排障引擎** — 50 棵 FTA 故障树 + 179 张诊断技能卡 + 法医证据方法（FEBM），而不是平铺的经验帖
2. **双读架构** — 源文档层服务人类深读，提炼知识层（概念/实体/技能/综合）服务 Agent 低 Token 消费
3. **语料即产品** — QA 语料每日自动再生、覆盖率门禁校验、按 tag 版本化发版，下游（智能体、向量库）按固定版本拉取

## 目录

- [项目速览](#项目速览)
- [双读架构](#双读架构)
- [知识架构](#知识架构)
- [结构化排障引擎](#结构化排障引擎)
- [AI Agent 集成](#ai-agent-集成)
- [快速开始](#快速开始)
- [关键 Runbook](#关键-runbook)
- [面试快问快答索引](#面试快问快答索引)
- [仓库结构](#仓库结构)
- [质量与自动化](#质量与自动化)
- [语料发版](#语料发版)
- [数据口径](#数据口径)
- [贡献](#贡献)

---

## 项目速览

| 维度 | 数值 / 说明 |
|------|-------------|
| 活跃知识文档 | **4,700+ 篇** Markdown（统计口径见[数据口径](#数据口径)） |
| 源文档层 | **21 个技术域**，3,099 篇深度技术文档 |
| wiki 提炼层 | **1,381 篇**：概念 306 · 实体 407 · 技能 582 · 综合 51 · 研究 35 |
| FTA 故障树 | **50 棵**（apiserver / etcd / DNS / CNI / CSI / 证书 / 升级 / GPU…） |
| 诊断技能卡 | **179 张**（19-故障诊断/08-技能体系） |
| QA 语料 | **15,094 对** I-O 语料（全量导出快照口径）+ 每日 nightly 再生 |
| 源码分析 | **5 个项目** 120 篇（Kubernetes / etcd / CNI / flannel / terway） |
| 云厂商覆盖 | 阿里云 / AWS / GCP / Azure / 腾讯云 / 华为云 + 多云混合专章 |
| 版本覆盖 | Kubernetes v1.25 ~ v1.32+，含 ACK / Terway / ASM 扩展 |
| 站点 | Astro 5 + Tailwind + Pagefind + Shiki + Mermaid + D3 知识图谱（本地构建） |
| License | Apache 2.0 |

三类受众，各取所需：

| 受众 | 用法 |
|------|------|
| **K8s / 平台工程师** | 出故障时按[关键 Runbook](#关键-runbook) 与 FTA 故障树走查；日常按域深读 |
| **SRE Lead / 决策者** | 评估作为团队知识基座：看[质量门禁](#质量与自动化)与语料发版机制 |
| **Agent / RAG 工程师** | 从[语料发版](#语料发版)拉取版本化资产，按 [Agent 集成](#ai-agent-集成)接入 |

---

## 双读架构

同一份知识库，两条消费路径：

| | 人类读者 | AI Agent |
|---|---------|----------|
| 入口 | 各域 `README.md` + `index.md` + [关键 Runbook](#关键-runbook) | [`35-元数据/AGENTS.md`](35-元数据/AGENTS.md) 唤醒协议 |
| 优先层 | 源文档层（01-21 域，深度与上下文完整） | 提炼知识层（22-26 目录，Token 高效） |
| 检索方式 | 站点全文搜索（Pagefind）/ Obsidian wikilink | index-only 语义打分 → 按 tier 按需拉取 |
| 兜底 | — | 源文档层仅在提炼层未命中时加载 |
| 产出格式 | Runbook / 排查剧本 / 速查卡 | 现象 → 根因 → 修复 → 验证 → 预防 五段闭环 |

提炼知识层的页面普遍带 `summary`、`intent_queries`、`trigger_keywords`、`tier` 等元数据字段（规范见 [`35-元数据/metadata/schema.md`](35-元数据/metadata/schema.md)），这是 Agent 侧 index-only 检索得以成立的基础。

---

## 知识架构

```
┌──────────────────────────────────────────────────────────────────────┐
│ 提炼知识层 —— Agent 优先读取，Token 高效（1,381 篇）                  │
│                                                                      │
│   22-概念/   核心概念、架构模式、设计原理          306 篇             │
│   23-实体/   组件实体、CNCF 工具、云产品实体       407 篇             │
│   26-技能/   诊断排障技能卡、FTA 方法、培训        582 篇             │
│   24-综合/   跨领域综合分析                         51 篇             │
│   25-研究/   研究笔记、调研报告                     35 篇             │
├──────────────────────────────────────────────────────────────────────┤
│ 源文档层 —— 21 个技术域，深度查询兜底（3,099 篇）                    │
│                                                                      │
│   Tier 1 核心技术域     01 集群基础 · 02 工作负载 · 03 清单模式       │
│                         04 应用模式 · 05 网络 · 06 存储 · 08 安全     │
│                         09 可观测性                                  │
│   Tier 2 平台与工程域   10 平台工程 · 11 发布变更 · 12 可靠性         │
│   Tier 3 运维场景域     13 生产运维 · 19 故障诊断                    │
│   Tier 4 部署与生态域   14 容器运行时 · 15 AI基础设施 · 16 专项技术   │
│                         07 数据库中间件 · 18 云厂商 · 20 最佳实践     │
│                         21 生态参考 · 17 系统基础                     │
└──────────────────────────────────────────────────────────────────────┘
```

### 21 个技术域明细

| 域 | 篇数 | 覆盖内容 |
|----|-----:|----------|
| [01-集群基础](01-集群基础/README.md) | 123 | 架构总览、控制平面、证书 PKI、kubectl、升级路径、性能调优 |
| [02-工作负载](02-工作负载/README.md) | 54 | Deployment/StatefulSet 核心负载、Java/Node.js/多语言运行时上 K8s |
| [03-清单模式](03-清单模式/README.md) | 101 | YAML 参考、Kustomize/Helm/Operator/GitOps/安全/韧性/AI-ML 模式 |
| [04-应用模式](04-应用模式/README.md) | 131 | 应用子模式、行业架构、生产模式 |
| [05-网络](05-网络/README.md) | 140 | Service/Ingress、网络基础、服务网格、API 网关、eBPF、Terway |
| [06-存储](06-存储/README.md) | 72 | PV/PVC/SC、CSI、分布式存储、有状态应用、云存储对比、AI 存储 |
| [07-数据库中间件](07-数据库中间件/README.md) | 69 | 数据库、缓存、消息队列、时序库、搜索引擎、Operator 管理 |
| [08-安全](08-安全/README.md) | 79 | 身份访问、网络安全、运行时安全、策略治理、供应链、零信任 |
| [09-可观测性](09-可观测性/README.md) | 86 | 指标、日志、链路追踪、告警、SLO/SLI、工具链 |
| [10-平台工程](10-平台工程/README.md) | 234 | 构建、运维、治理、开发体验、内部开发者平台、代码分析 |
| [11-发布变更](11-发布变更/README.md) | 75 | GitOps、IaC、渐进式交付、变更管理、部署与迁移方案 |
| [12-可靠性](12-可靠性/README.md) | 82 | 备份/灾难恢复、容量规划、混沌工程、事后复盘、SRE 实践 |
| [13-生产运维](13-生产运维/README.md) | 127 | 成本治理、集群治理、事件响应、工单案例、运维手册与剧本 |
| [14-容器运行时](14-容器运行时/README.md) | 61 | Docker、镜像管理/构建、containerd/CRI-O、沙箱运行时 |
| [15-AI基础设施](15-AI基础设施/README.md) | 176 | GPU 基础设施、AI Agents、Agent 运行时、K8s AI 栈 |
| [16-专项技术](16-专项技术/README.md) | 61 | 边缘计算、WebAssembly、扩展机制、无服务器、多租户 |
| [17-系统基础](17-系统基础/README.md) | 646 | Linux、硬件、网络基础、K8s 事件、速查卡、知识字典 |
| [18-云厂商](18-云厂商/README.md) | 115 | ACK / EKS / GKE / AKS / TKE / CCE、多云混合、阿里云云原生 |
| [19-故障诊断](19-故障诊断/README.md) | 491 | 核心排障、FTA 故障树、FEBM 方法论、技能体系、QA 语料 |
| [20-最佳实践](20-最佳实践/README.md) | 109 | 部署、基础设施、迁移、观测、运营、安全、大规模生产 |
| [21-生态参考](21-生态参考/README.md) | 67 | CNCF 全景、论文、领域索引 |

每域根目录提供 `README.md`（域导航）+ `index.md`（索引）+ `00-总览/`（入门路径）。

---

## 结构化排障引擎

KUDIG 的差异化能力所在——排障知识不是散文，而是可被人和机器共同执行的结构：

| 组件 | 规模 | 位置 |
|------|------|------|
| **FTA 故障树** | 50 棵顶层故障树（apiserver、etcd、DNS、CNI、CSI、证书、升级、GPU、HPA…） | `19-故障诊断/06-FTA故障树/list/` |
| **诊断技能卡** | 179 张场景技能卡（NodeNotReady、CrashLoopBackOff、DNS 解析失败、PVC 挂载失败…） | `19-故障诊断/08-技能体系/` |
| **FEBM 方法论** | 法医证据方法：症状特征向量 → 证据链 → 根因确认 | `19-故障诊断/07-FEBM方法论/` |
| **多故障场景** | 复合故障与级联失效推演 | `19-故障诊断/09-多故障场景/` |
| **QA 语料** | 命令-输出诊断 I-O 配对；五类能力语料（knowledge / reasoning / conversation / safety / tool-use）+ benchmark | `19-故障诊断/10-QA语料/` |

诊断 Agent（KuDig Doctor）按五阶段工作流执行，并有量化质量标准：

```
信息采集 → 根因分析 → 方案生成 → 安全评审 → 输出闭环
（kubectl 数据面）  （排除法+故障树） （命令+风险评估）（红线检查） （五段格式+记忆沉淀）
```

| Agent 质量指标 | 目标值 |
|----------------|--------|
| 平均诊断步骤 | ≤ 5 步 |
| 首次诊断准确率 | ≥ 85% |
| 单次诊断 Token 消耗 | ≤ 30K |
| 无数据支撑断言比例（幻觉率） | < 3% |

完整行为规范（唤醒协议、P0-P3 优先级判定、记忆管理、多 Agent 交接协议、命令风险分级 🔴🟡🟢）见 [`35-元数据/AGENTS.md`](35-元数据/AGENTS.md)。

---

## AI Agent 集成

### 接入路径

1. **版本化语料拉取（推荐）** — 按语义化 tag 拉取 Release 资产，见[语料发版](#语料发版)
2. **全量 NAS 挂载** — 参照 [`32-发布/AGENT-USAGE.md`](32-发布/AGENT-USAGE.md) 的加载策略：**严禁整包载入上下文**，遵循 index-only 检索：
   - 首次加载 `manifest.json`（<1KB）确认版本与 profile
   - 加载 `index.json`，用 `title + summary + tags` 对 query 做语义打分
   - 按需拉取 top-K 单页：`core` 命中优先于 `supporting`，`peripheral` 仅在 query 明确匹配时加载
3. **冷启动协议** — 挂载后先读包内 `index.md`（~400 行），获取统计、domain 分布、top hub、QA schema 与快速定位指南

### 语料切片分层（tier）

| Tier | 定位 | 加载策略 |
|------|------|----------|
| `core` | 高置信核心页 | 建议常驻上下文 |
| `supporting` | 常规支撑页 | 按需检索加载 |
| `peripheral` | 边缘页 | 仅 query 明确匹配时拉取 |

tier 划分、chunk 策略与 embedding 方案见 [`35-元数据/corpus-config/`](35-元数据/corpus-config/README.md)（含 [`rag-chunking-strategy.md`](35-元数据/corpus-config/rag-chunking-strategy.md) 与 [`embedding-guide.md`](35-元数据/corpus-config/embedding-guide.md)）。

### Agent 行为规范资产

`35-元数据/` 同时维护 Agent 侧配套资产：

- [`AGENTS.md`](35-元数据/AGENTS.md) — 唤醒协议、任务路由、五阶段工作流、记忆管理（30 天短期 + 每周提炼长期）、诊断/修复/验证多 Agent 交接协议
- `metadata/` — schema、taxonomy（受控标签词汇）、domain-mapping、知识地图
- `version-watch.json` — 6 个核心组件（kubernetes / cilium / istio / prometheus / argo-cd / terway）的版本监控配置
- `metrics/` — 内容质量指标（nightly 流水线产出）
- `journal/` — 维护日志

### 近期重点：百炼智能体运维

`39-百炼智能体/` 落地阿里云百炼平台的 Agentic Ops：[每日巡检部署配置](39-百炼智能体/01-每日巡检部署配置.md)（智能体定时巡检集群）与[本地集群接入方案](39-百炼智能体/02-本地集群接入方案.md)（MCP 接入）。配套的通用巡检剧本见 `31-脚本/automation/`（证书到期监控、K8s 健康检查、网络策略审计、资源清理、存储审计、GPU 利用率报告）。

---

## 快速开始

### 按问题找答案（人类路径）

```bash
# 示例：Pod 一直 Pending → 先查诊断技能卡，再沿 FTA 故障树深挖
open 19-故障诊断/08-技能体系/03-pod-pending.md
open 19-故障诊断/06-FTA故障树/list/scheduler-fta.md
```

所有内容均为纯 Markdown，推荐用 [Obsidian](https://obsidian.md/) 打开——`[[wikilink]]` 双链可原生跳转。

### 本地构建站点（可视化探索）

```bash
cd 30-站点
npm install
npm run dev        # http://localhost:4321
npm run build && npm run preview
```

站点功能：D3 知识图谱（全库拓扑）、领域探索器、学习方法论动画、自学评估雷达图；搜索基于 Pagefind，代码高亮基于 Shiki，图表基于 Mermaid。

### 生成诊断 QA 语料

```bash
make corpus-generate-p0      # P0 优先级 I-O 语料（核心故障场景）
make corpus-generate-all     # 全量（P0+P1+P2）
make corpus-validate         # 覆盖率校验（技能卡 + FTA 覆盖）
make corpus-stats            # 语料规模统计
make corpus-clean            # 清理生成产物
```

### Agent 拉取语料

```bash
# 按固定版本拉取（下游请勿引用散文件）
gh release download corpus-v2026.09 --repo kudig-io/kudig-database
```

---

## 关键 Runbook

跨域高频运维入口（均为仓库内已验证路径）：

| 主题 | 文件 |
|------|------|
| 证书 / PKI 生命周期 | [`01-集群基础/03-控制平面/38-certificate-pki-lifecycle-runbook.md`](01-集群基础/03-控制平面/38-certificate-pki-lifecycle-runbook.md) |
| 集群升级 | [`01-集群基础/03-控制平面/39-cluster-upgrade-runbook.md`](01-集群基础/03-控制平面/39-cluster-upgrade-runbook.md) |
| 灾难恢复与业务连续性 | [`12-可靠性/02-灾难恢复/25-disaster-recovery-bc-runbook-v2.md`](12-可靠性/02-灾难恢复/25-disaster-recovery-bc-runbook-v2.md) |
| Fleet GitOps 操作指南 | [`11-发布变更/01-GitOps/10-fleet-gitops-operations-guide.md`](11-发布变更/01-GitOps/10-fleet-gitops-operations-guide.md) |
| 事件响应模板 | [`13-生产运维/03-事件响应/11-incident-response-runbook-template.md`](13-生产运维/03-事件响应/11-incident-response-runbook-template.md) |
| FinOps 成本治理 | [`13-生产运维/01-成本治理/06-finops-cost-governance-runbook.md`](13-生产运维/01-成本治理/06-finops-cost-governance-runbook.md) |
| AI/ML 运维 | [`15-AI基础设施/01-基础设施/38-ai-ml-ops-runbook.md`](15-AI基础设施/01-基础设施/38-ai-ml-ops-runbook.md) |
| 边缘生产运维 | [`16-专项技术/01-边缘计算/12-edge-production-runbook.md`](16-专项技术/01-边缘计算/12-edge-production-runbook.md) |
| 智能体每日巡检 | [`39-百炼智能体/01-每日巡检部署配置.md`](39-百炼智能体/01-每日巡检部署配置.md) |

---

## 面试快问快答索引

面试向 quick-qa 系列：先遮住答案口述，再对照关键词补全，最后沿"深挖"链接回语料源文件精读。同一知识域配套理论面经（姊妹篇见文末行）。

| 知识域 | 文件 | 题量 | 侧重 |
|--------|------|-----:|------|
| 网络（Service/Ingress/CNI/DNS） | `05-网络/01-K8s网络核心/58-kubernetes-network-quick-qa.md` | 56 | 快问快答 + conntrack/数据面进阶 + 12 大高级课题 + 面试模拟对话 |
| DNS/CoreDNS | `05-网络/01-K8s网络核心/59-kubernetes-dns-coredns-quick-qa.md` | 19 | 解析链路、插件链、ndots/超时竞态、NodeLocal、dnsPolicy、排障与外部 DNS |
| 存储入门 | `06-存储/01-K8s存储/22-k8s-storage-quick-qa.md` | 61 | PV/PVC/SC 基础速查 |
| 存储高级 | `06-存储/01-K8s存储/23-k8s-storage-advanced-quick-qa.md` | 15 | CSI 两阶段、快照、拓扑、容灾、加密、删除保护 |
| Deployment | `02-工作负载/01-核心工作负载/26-kubernetes-deployment-quick-qa.md` | 21 | 三层模型、滚动更新、探针、终止竞态、发布模式、progressDeadline、排障 |
| 调度与资源 | `02-工作负载/01-核心工作负载/27-kubernetes-scheduling-resource-quick-qa.md` | 21 | requests/limits、QoS/OOM、调度两阶段、污点拓扑、HPA/VPA、LimitRange、容量水位 |
| 架构与配置安全 | `01-集群基础/01-架构总览/26-kubernetes-architecture-config-quick-qa.md` | 21 | 控制平面组件、list-watch、创建 Pod 全流程、Secret/RBAC、三阶段准入、CRD |

理论面经姊妹篇：网络 [`57-kubernetes-service-ingress-interview.md`](05-网络/01-K8s网络核心/57-kubernetes-service-ingress-interview.md)、存储 [`21-storageclass-pvc-pv-interview-notes.md`](06-存储/01-K8s存储/21-storageclass-pvc-pv-interview-notes.md)（与上述文件同目录）。

---

## 仓库结构

```
.
├── 01-集群基础/              # 控制平面、证书 PKI、升级、性能调优
├── 02-工作负载/              # 核心工作负载、多语言运行时上 K8s
├── 03-清单模式/              # YAML/Kustomize/Helm/Operator/GitOps 等清单范式
├── 04-应用模式/              # 应用架构子模式、行业架构、生产模式
├── 05-网络/                  # Service/Ingress、服务网格、eBPF、Terway
├── 06-存储/                  # PV/PVC/CSI、分布式存储、AI 存储
├── 07-数据库中间件/          # 数据库/缓存/消息队列/时序库 on K8s
├── 08-安全/                  # RBAC、运行时安全、供应链、零信任
├── 09-可观测性/              # 指标/日志/追踪/告警/SLO
├── 10-平台工程/              # 构建、治理、开发体验、内部开发者平台
├── 11-发布变更/              # GitOps、IaC、渐进式交付、迁移方案
├── 12-可靠性/                # 备份恢复、灾难恢复、混沌工程、SRE
├── 13-生产运维/              # 成本治理、事件响应、工单案例、运维剧本
├── 14-容器运行时/            # Docker/containerd/沙箱、镜像管理
├── 15-AI基础设施/            # GPU、AI Agents、Agent 运行时
├── 16-专项技术/              # 边缘、WebAssembly、无服务器、多租户
├── 17-系统基础/              # Linux、硬件、网络基础、速查卡、知识字典
├── 18-云厂商/                # ACK/EKS/GKE/AKS/TKE/CCE、多云混合
├── 19-故障诊断/              # FTA 故障树、FEBM、技能体系、QA 语料
├── 20-最佳实践/              # 生产清单、大规模生产、安全运营
├── 21-生态参考/              # CNCF 全景、论文、领域索引
├── 22-概念/                  # 提炼层：核心概念、架构模式
├── 23-实体/                  # 提炼层：组件实体、CNCF 工具、云产品
├── 24-综合/                  # 提炼层：跨领域综合分析
├── 25-研究/                  # 研究笔记、调研报告
├── 26-技能/                  # 提炼层：诊断技能卡、FTA 方法、培训
├── 27-标签/                  # 标签索引页（受控 taxonomy 的人类视图）
├── 28-资产/                  # 图片、图表、PDF 附件
├── 29-文档/                  # CONTRIBUTING、CHANGELOG、agent-specs
├── 30-站点/                  # Astro 站点项目（构建产物 gitignore）
├── 31-脚本/                  # 维护/质量/语料脚本（约百个）+ automation 巡检剧本
├── 32-发布/                  # 发布说明与 Agent 消费指南（冻结；产物改 tag 发版）
├── 33-源码/                  # vendor 源码树（kubernetes/etcd/cni 等，gitignore）
├── 34-源码分析/              # kubernetes/etcd/cni/flannel/terway 源码分析（120 篇）
├── 35-元数据/                # AGENTS.md、schema、taxonomy、corpus-config、metrics
├── 36-报告/                  # 质量报告与评估（冻结）
├── 37-归档/                  # wiki 归档快照（冻结，仅重建用）
├── 38-消化/                  # 知识消化产物（播客等二次创作）
├── 39-百炼智能体/            # 百炼平台 Agentic Ops（巡检、MCP 接入）
└── kubernetes-hardware/      # K8s 硬件调研报告与生成脚本（研究产物）
```

### 命名约定

- **一级目录**：`NN-中文简称`，01-39 有序化前缀；知识域与技术支撑目录共用一套编号
- **二级目录**（域内）：同样 `NN-` 前缀；英文专名保留（如 `GitOps`、`eBPF`、`FEBM方法论`）
- **文件名**：`kebab-case.md`，ASCII 字符
- **域入口**：每域根目录 `README.md` + `index.md` + `00-总览/`

完整目录映射见 [`35-元数据/metadata/domain-mapping.md`](35-元数据/metadata/domain-mapping.md)，域间知识地图见 [`35-元数据/metadata/knowledge-map.md`](35-元数据/metadata/knowledge-map.md)。

---

## 质量与自动化

### CI 工作流（6 个）

| 工作流 | 触发 | 职责 |
|--------|------|------|
| **Quality Gate** | push main / PR / 手动 | 5 道门禁（见下） |
| **Corpus Coverage Check** | push / PR | QA 语料覆盖率校验（阻断式） |
| **Nightly Corpus** | 每日 05:30（北京时间） | 重生成全量 QA 语料 + 覆盖率校验，有变更才回写 main |
| **Release Corpus** | push `corpus-v*` tag | 打包语料快照并发布 GitHub Release 资产 |
| **Secret Scan** | push（gitleaks） | 只扫新增提交，不做历史翻账；手动可触发全量历史扫描 |
| **Version Watch** | 每周一 10:00（北京时间） | 检查 6 个核心组件新 release，检索库内受影响页面并自动开 issue 提醒复核 |

### Quality Gate 五道门禁

1. **Ruff Lint** — `31-脚本/maintenance/` Python 脚本静态检查
2. **README Sync** — 根 README 目录图与实际顶层目录双向一致性
3. **Frontmatter Integrity** — 全部内容页 YAML 可解析 + 必填字段（`title` 等）
4. **Broken Wikilink Gate** — 断链清零后启用的强制门禁，任何新增 `[[断链]]` 阻断合并
5. **Heading Integrity** — 一级标题阈值检查（防止排查命令 `#` 注释污染标题层级）

### 本地质量检查（与 CI 同步）

```bash
ruff check 31-脚本/maintenance/                    # Python lint
python3 31-脚本/readme-sync-check.py               # README 与目录一致性
python3 31-脚本/frontmatter-quality-check.py       # frontmatter 完整性
bash 31-脚本/check-broken-links.sh                 # broken wikilink
bash 31-脚本/code-example-validation.sh            # 代码块语法
```

---

## 语料发版

语料按 tag 版本化发布（发布产物已停止入库 git）：

```bash
git tag corpus-v2026.09
git push origin corpus-v2026.09
# → CI 自动打包 19-故障诊断/10-QA语料/generated 全量快照为 tar.gz
#   并创建 GitHub Release（自动生成 release notes）
```

下游约定（百炼智能体知识库、向量库 pipeline 等）：

- 按**固定 tag** 拉取 Release 资产，勿引用散文件或 main 分支路径
- 加载策略遵循 [`32-发布/AGENT-USAGE.md`](32-发布/AGENT-USAGE.md)（index-only 检索、tier 优先级、冷启动协议）
- 切片与向量化参数见 [`35-元数据/corpus-config/`](35-元数据/corpus-config/README.md)

---

## 数据口径

本 README 的数字可复现，避免"虚假繁荣"：

| 口径 | 范围 |
|------|------|
| 活跃知识文档（4,700+） | `01-27` 全部域 + `29-文档` + `34-源码分析` + `35-元数据` + `38-消化` + `39-百炼智能体` 的 `*.md`；不含 `28-资产`、`30-站点`、`32-发布`、`33-源码`、`36-报告`、`37-归档` 与根目录散页 |
| 源文档层（3,099） | `01-集群基础` ~ `21-生态参考` 共 21 域 |
| 提炼层（1,381） | 22-概念 306 + 23-实体 407 + 26-技能 582 + 24-综合 51 + 25-研究 35 |
| FTA 50 棵 / 技能卡 179 张 | `19-故障诊断/06-FTA故障树/list/` 与 `08-技能体系/` 下的 `.md` 文件计数 |
| QA 语料 15,094 对 | 2026-07 全量导出快照（`qa-corpus.jsonl` 去重后），nightly 每日再生 |

复算命令：

```bash
find 0[1-9]-*/ 1[0-9]-*/ 2[0-7]-*/ 29-文档 34-源码分析 35-元数据 38-消化 39-百炼智能体 \
  -name '*.md' -type f | wc -l
```

---

## 贡献

欢迎提交 Issue 和 PR。贡献前请阅读：

- [`29-文档/CONTRIBUTING.md`](29-文档/CONTRIBUTING.md) — 贡献流程、提交规范、质量标准
- [`35-元数据/metadata/schema.md`](35-元数据/metadata/schema.md) — Frontmatter 元数据规范
- [`35-元数据/metadata/taxonomy.md`](35-元数据/metadata/taxonomy.md) — Tag 受控词表
- [`35-元数据/metadata/domain-mapping.md`](35-元数据/metadata/domain-mapping.md) — 目录与命名约定

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

> CI 会对新增断链与 frontmatter 缺陷执行阻断——PR 提交前建议先在本地跑一遍上面的质量检查命令。

---

## License

[Apache License 2.0](LICENSE) · 版权所有 2026 KUDIG Team

## Related

- [[02-工作负载/04-多语言运行时/01-go-on-kubernetes-production|Go 应用 Kubernetes 生产实践]]
- [[02-工作负载/04-多语言运行时/02-python-on-kubernetes-production|Python 应用 Kubernetes 生产实践]]
- [[02-工作负载/04-多语言运行时/03-rust-on-kubernetes-production|Rust 应用 Kubernetes 生产实践]]
- [[02-工作负载/04-多语言运行时/04-gpu-workload-management|GPU 工作负载管理]]
- [[09-可观测性/00-总览/13-ebpf-observability-deep-dive|eBPF 可观测性深度实践]]
- [[12-可靠性/06-SRE实践/13-ai-workload-reliability|AI 工作负载可靠性]]
- [[19-故障诊断/04-高级排障/structural-README|Kubernetes 结构化故障排查知识库 [故障诊断]]]
- [[26-技能/04-工作负载/pod/方法论/FTA Diagnostic Execution Engine|FTA Diagnostic Execution Engine]]
- [[27-标签/06-AI与专项/research|research]]
