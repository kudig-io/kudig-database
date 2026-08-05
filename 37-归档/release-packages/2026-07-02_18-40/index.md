---
title: KUDIG Database — Corpus Package Index
version: 2026.07.02
generated_at: 2026-07-02T18:40:23
profile: rag-full-profile.yaml
total_pages: 3329
total_tokens: 11554636
tier_counts:
  core: 804
  supporting: 1509
  peripheral: 1016
qa_pairs: 15094
domains: 23
tags_unique: 342
summary: AgentScope Agent 挂载 NAS 时的核心索引文件。描述 corpus 包的结构、统计、加载策略与快速定位指南。
category: index
tags:
- index
- corpus
- agentscope
- rag
tier: core
created: '2026-07-02'
last_updated: 2026-07-02
---

# KUDIG Database — Corpus Package Index

> **AgentScope Agent 必读入口**。挂载 NAS 后先读本文件，再决定加载策略。
>
> 本包由 `release/scripts/export_corpus_for_nas.py` 基于 `rag-full-profile.yaml` 自动生成。
> 所有页面严格经 profile 的 `include/exclude` 规则筛选，硬排除 `.git/`, `_archives/`, `_raw/`, `node_modules/` 等非知识目录。

---

## 1. 包概览

| 字段 | 值 |
|---|---|
| 包标识 | `kudig-database-corpus` |
| 版本 | `2026.07.02` |
| 生成时间 | `2026-07-02T18:40:23.063887` |
| Profile | `rag-full-profile.yaml` |
| 总页数 | **3,329** |
| 总 tokens | **~11.5M**（按 `len(text)/4` 估算） |
| 领域数 | 23 个顶层 domain |
| 唯一标签 | 342 |
| QA 对数 | 15,094（去重后） |

### Tier 分层

| Tier | 页数 | Tokens (约) | 加载建议 |
|---|---:|---:|---|
| **core** | 804 | 2.8M | 常驻；权威核心概念、实体、方法论 |
| **supporting** | 1,509 | 5.2M | 按需加载；扩展知识、领域细节 |
| **peripheral** | 1,016 | 3.5M | 仅明确相关时拉取；边缘内容、历史版本 |

---

## 2. 目录结构

```
package/2026-07-02_18-40/
├── index.md                   ← 本文件（Agent 入口）
├── manifest.json              ← 包清单（profile、计数、supplementary）
├── index.json                 ← 全页索引（path/title/summary/tags/tier/tokens）
│
├── corpus/                    ← 语料主体（按 tier 分目录）
│   ├── core/                  ← 804 pages
│   ├── supporting/            ← 1509 pages
│   └── peripheral/            ← 1016 pages
│
├── qa/
│   ├── qa-corpus.jsonl        ← 15,094 QA 对（input/output/source/type/tags）
│   └── raw/                   ← 63 个原始 QA 源文件（YAML/JSON/JSONL）
│       ├── _schema/           ← QA schema 定义
│       ├── benchmark/         ← 评估基准
│       ├── capability-*/      ← 分能力维度 QA（conversation/knowledge/reasoning/safety/tool-use）
│       ├── command-output-diagnosis*.md
│       └── domain-*-qa.yaml   ← 按 domain 分域的 QA 源
│
└── metadata/
    ├── page-stats.json        ← category / tag 分布统计
    ├── intent-corpus/         ← 意图识别语料（1 文件：P0-1-intent-corpus-expanded.jsonl）
    ├── agent-specs/           ← 22 个 Agent 行为规范文档
    └── taxonomy/              ← 5 个分类体系文件（STRUCTURE/AGENTS/schema/taxonomy/README）
```

---

## 3. Agent 加载策略

### 3.1 默认：Index-only 检索（推荐）

**适用**：通用问答、query 路由、RAG 召回。

```
1. 读 manifest.json   (< 1 KB)  →  确认版本与 profile
2. 读 index.json      (~1.8 MB) →  全页元数据索引
3. 对 query 用 title + summary + tags 做语义打分
4. 按得分 + tier 优先级拉取 top-K：corpus/<tier>/<path>.md
```

### 3.2 受限上下文：Tiered 加载

**适用**：Agent 上下文窗口较小，或需控制成本。

- **最小集**：仅 `corpus/core/` (~2.8M tokens)
- **均衡集**：`core/` + 命中相关的 `supporting/` 单页
- **完整集**：不建议整包加载（~11.5M tokens 远超常规上下文）

### 3.3 QA 走独立 embedding

`qa/qa-corpus.jsonl` 单独喂向量库，**不与 corpus 混索引**。

- 用途：few-shot 示例、意图分类、query 改写
- 字段：`input`, `output`, `source`, `type`, `tags`, `skill_ref`, `io_pair_id`

### 3.4 意图识别优先

`metadata/intent-corpus/P0-1-intent-corpus-expanded.jsonl` 是高优先级意图样本（P0 级）。
**Agent 冷启动时先加载此文件**，建立 query → intent 的映射。

---

## 4. Corpus 域分布（23 个 domain）

按 page 数排序：

| 排名 | Domain | Pages | Core | Sup | Per | 主题 |
|---:|---|---:|---:|---:|---:|---|
| 1 | `domain-10-troubleshooting-diagnostics` | 464 | 210 | 211 | 43 | 故障诊断方法论、FTA/FEBM、工单案例 |
| 2 | `skills` | 458 | 92 | 155 | 211 | 运维操作、最佳实践、SOP |
| 3 | `entities` | 385 | 134 | 148 | 103 | 工具、组件、产品、人物 |
| 4 | `concepts` | 280 | 129 | 147 | 4 | 抽象概念、架构模式 |
| 5 | `domain-14-ai-ml-infra` | 158 | 6 | 91 | 61 | AI/ML 基础设施、Agent 编排 |
| 6 | `domain-07-platform-engineering` | 155 | 12 | 59 | 84 | 平台工程、IaC、CI/CD |
| 7 | `domain-02-workloads-applications` | 139 | 15 | 81 | 43 | 工作负载、应用模式 |
| 8 | `domain-03-networking-traffic` | 135 | 14 | 27 | 94 | K8s 网络、Service Mesh、eBPF |
| 9 | `domain-20-application-patterns` | 120 | 22 | 98 | 0 | 应用架构模式、Saga、CQRS |
| 10 | `domain-01-cluster-fundamentals` | 116 | 9 | 27 | 80 | 集群基础、控制面、数据面 |
| 11 | `domain-11-production-operations` | 104 | 20 | 50 | 34 | 生产运维、on-call、工单 |
| 12 | `domain-12-cloud-providers` | 97 | 27 | 55 | 15 | ACK/GKE/AKS/TKE/CCE |
| 13 | `domain-06-observability` | 87 | 9 | 77 | 1 | Prometheus / Grafana / Loki |
| 14 | `domain-05-security-compliance` | 81 | 8 | 73 | 0 | RBAC、网络策略、运行时安全 |
| 15 | `domain-17-system-foundation` | 77 | 16 | 14 | 47 | 系统基础、速查表、词典 |
| 16 | `domain-08-release-change-management` | 70 | 21 | 24 | 25 | 发布管理、变更管理 |
| 17 | `domain-15-specialized-tech` | 69 | 4 | 20 | 45 | Wasm、eBPF、Specialized |
| 18 | `domain-09-reliability-engineering` | 68 | 10 | 42 | 16 | SLO/SLA、容灾、备份 |
| 19 | `domain-19-landscape-references` | 60 | 25 | 13 | 22 | 领域景观、版本索引、Release Notes |
| 20 | `domain-18-manifests-patterns` | 52 | 2 | 12 | 38 | YAML Manifests、配置模式 |
| 21 | `domain-16-database-middleware` | 52 | 8 | 35 | 9 | MySQL/PG/Mongo/Kafka Operator |
| 22 | `domain-04-storage-data` | 52 | 5 | 28 | 19 | CSI、分布式存储、StatefulSet |
| 23 | `domain-13-container-runtime` | 50 | 6 | 22 | 22 | containerd、CRI-O、Kata |

---

## 5. Top Hub Pages（按入链数）

| 入链 | Tier | Path | 主题 |
|---:|---|---|---|
| 1601 | core | `entities/kubernetes.md` | Kubernetes 实体（最核心枢纽） |
| 534 | core | `entities/prometheus.md` | Prometheus 实体 |
| 421 | core | `entities/etcd.md` | etcd 状态存储 |
| 366 | core | `entities/kubelet.md` | kubelet 节点代理 |
| 335 | core | `concepts/kubernetes-architecture-overview.md` | K8s 架构总览 |
| 309 | core | `domain-17-system-foundation/topic-cheat-sheet/k8s.md` | K8s 速查表 |
| 274 | sup | `concepts/service.md` | Service 概念 |
| 268 | core | `domain-17-system-foundation/topic-cheat-sheet/go.md` | Go 速查表 |
| 264 | core | `domain-19-landscape-references/topic-index/gitops-cicd-index.md` | GitOps CI/CD 索引 |
| 232 | core | `domain-19-landscape-references/topic-index/etcd-index.md` | etcd 主题索引 |
| 200 | core | `entities/argocd.md` | ArgoCD |
| 186 | core | `concepts/ingress.md` | Ingress |
| 180 | core | `entities/networkpolicy.md` | NetworkPolicy |
| 172 | core | `entities/cilium.md` | Cilium / eBPF |
| 171 | core | `domain-05-security-compliance/README.md` | 安全合规总览 |
| 169 | core | `entities/containerd.md` | containerd |
| 168 | core | `domain-17-system-foundation/topic-cheat-sheet/helm.md` | Helm 速查表 |
| 165 | core | `entities/statefulset.md` | StatefulSet |

---

## 6. Top Tags（受控词汇）

| Tag | Count | 主题 |
|---|---:|---|
| k8s | 1785 | Kubernetes 全局 |
| prometheus | 991 | 可观测性 |
| etcd | 742 | 控制面存储 |
| kubelet | 723 | 节点代理 |
| scheduler | 519 | 调度器 |
| apiserver | 512 | API Server |
| grafana | 483 | 可视化 |
| rag | 448 | 检索增强生成 |
| operator | 434 | K8s Operator |
| docker | 421 | 容器引擎 |
| helm | 372 | 包管理 |
| crd | 335 | Custom Resource |
| ingress | 317 | 入口网关 |
| istio | 316 | Service Mesh |
| controller-manager | 313 | 控制器 |
| troubleshooting | 310 | 故障诊断 |
| rbac | 301 | 权限 |
| cilium | 289 | eBPF 网络 |
| containerd | 289 | CRI 运行时 |

完整 tag 分布见 `metadata/page-stats.json` 的 `tag_counts` 字段。

---

## 7. QA Corpus

### 7.1 类型分布

| Type | Count | 用途 |
|---|---:|---|
| concept | 8184 | 概念解释、原理问答 |
| operation | 2714 | 操作指南、命令示例 |
| diagnosis | 1650 | 故障诊断、根因分析 |
| comparison | 1223 | 技术选型、方案对比 |
| troubleshooting | 1065 | 排障流程、troubleshooting |
| best_practice | 177 | 最佳实践、规范 |
| fault_tree | 81 | 故障树、FTA 节点 |
| **Total** | **15,094** | |

### 7.2 Top Sources（按 QA 对数）

| Source | Pairs |
|---|---:|
| `domain-19-landscape-references-qa.yaml` | 3377 |
| `domain-10-troubleshooting-diagnostics-qa.yaml` | 2545 |
| `domain-11-production-operations-qa.yaml` | 1219 |
| `domain-17-system-foundation-qa.yaml` | 1218 |
| `command-output-diagnosis-p0.json` | 727 |
| `domain-07-platform-engineering-qa.yaml` | 694 |
| `domain-14-ai-ml-infra-qa.yaml` | 680 |
| `domain-02-workloads-applications-qa.yaml` | 624 |
| `domain-03-networking-traffic-qa.yaml` | 564 |
| `command-output-diagnosis-all.yaml` | 469 |

### 7.3 字段 Schema

```json
{
  "input":        "用户 query / 命令 / 异常描述",
  "output":       "标准答案 / 诊断 / 操作",
  "source":       "源文件相对路径（vault 根）",
  "type":         "concept | operation | diagnosis | comparison | troubleshooting | best_practice | fault_tree",
  "tags":         ["kubectl", "pod", "net", ...],
  "skill_ref":    "关联的 skill 引用（可选）",
  "io_pair_id":   "稳定 ID（可选，用于跨版本对齐）"
}
```

---

## 8. Metadata Files

### 8.1 `metadata/page-stats.json`

```json
{
  "total_pages": 3329,
  "total_tokens": 11554636,
  "tier_counts": {"core": 804, "supporting": 1509, "peripheral": 1016},
  "category_counts": {...},
  "tag_counts": {...}
}
```

### 8.2 `metadata/agent-specs/`（22 个文件）

Agent 行为规范与协议：

| 文件 | 主题 |
|---|---|
| `CLAUDE.md` / `GEMINI.md` | Agent 通用身份与约束 |
| `obsidian-wiki-agent-context.md` | Obsidian Wiki 框架 Agent 上下文 |
| `P0-1-ticket-classification-intent-recognition.md` | P0：工单分类与意图识别 |
| `P0-2-multi-skill-coordination-protocol.md` | P0：多 Skill 协调协议 |
| `P0-3-session-context-management.md` | P0：会话上下文管理 |
| `P0-Knowledge-Graph-RDF-Model.md` | P0：知识图谱 RDF 模型 |
| `P0-Tool-Schema-Definition.md` | P0：工具 Schema 定义 |
| `P1-4-decision-tree-mermaid-visualization.md` | P1：决策树 Mermaid 可视化 |
| `P1-5-oncall-quick-reference-card.md` | P1：on-call 速查卡 |
| `P1-6-alert-to-ticket-resolution-loop.md` | P1：告警→工单→解决闭环 |
| `P1-7-Reflection-Mechanism.md` | P1：反思机制 |
| `P1-8-Agent-Diagnostic-Benchmark.md` | P1：Agent 诊断基准 |
| `P2-7-ai-ml-workloads-troubleshooting.md` | P2：AI/ML 工作负载排障 |
| `P2-8-database-middleware-troubleshooting.md` | P2：数据库中间件排障 |
| `P2-9-non-k8s-infrastructure-troubleshooting.md` | P2：非 K8s 基础设施排障 |
| `P3-10-cloud-vendor-specific-troubleshooting.md` | P3：云厂商特定排障 |
| `P3-11-security-incident-sop-compliance-checklist.md` | P3：安全事件 SOP |
| `P3-12-multi-cluster-federation-troubleshooting.md` | P3：多集群联邦排障 |

优先级：P0 > P1 > P2 > P3。Agent 冷启动按优先级顺序加载。

### 8.3 `metadata/taxonomy/`（5 个文件）

| 文件 | 作用 |
|---|---|
| `STRUCTURE.md` | 23 个 domain 的总体结构说明 |
| `AGENTS.md` | 行为规范与工作流 |
| `schema.md` | frontmatter 字段 schema |
| `taxonomy.md` | 受控标签词汇与别名 |
| `README.md` | 分类体系总览 |

### 8.4 `metadata/intent-corpus/`

- `P0-1-intent-corpus-expanded.jsonl`：P0 级意图识别语料（扩展版）。**Agent 冷启动必读。**

---

## 9. 快速定位指南

### 9.1 给定 query，如何找答案？

1. **关键词匹配**：扫 `index.json.pages[].title` + `tags` + `summary`
2. **Tier 优先级**：core > supporting > peripheral
3. **入链加分**：`incoming_links` 高的页面更权威
4. **domain 聚焦**：按 query 主题选 domain（见 §4）
   - 故障诊断 → `domain-10-*`
   - 网络问题 → `domain-03-*`
   - AI/ML → `domain-14-*`
   - 安全 → `domain-05-*`
   - 云平台 → `domain-12-*`

### 9.2 给定命令输出，如何诊断？

1. 命中 `qa/raw/command-output-diagnosis*.md` → 直接返回匹配的诊断条目
2. 否则查 `corpus/core/domain-10-troubleshooting-diagnostics/topic-fta/`（FTA 方法论）
3. 再查 `topic-febm/`（FEBM 智能体工单处理）

### 9.3 给定概念解释需求？

1. 先查 `concepts/<concept>.md`（129 core 概念页）
2. 补充查 `entities/<tool>.md`（134 core 实体页）
3. 深度需求查 `domain-*/README.md` 或 `topic-index/*.md`

---

## 10. 重新生成与归档

### 重新生成

```bash
# 默认：写入新时间戳目录
python release/scripts/export_corpus_for_nas.py

# 切换 profile（如 SRE 子集）
python release/scripts/export_corpus_for_nas.py -p rag-sre-profile.yaml

# 自定义路径（绕过时间戳）
python release/scripts/export_corpus_for_nas.py -o /tmp/export
```

### 归档策略

- `release/package/` 下按时间戳保留所有历史导出
- 人工决定保留策略（建议至少保留最近 3 个）
- 脚本内置安全守卫：拒绝清理 vault 根、`release/`、`release/scripts/`、`release/package/`

### 完整度评估

```bash
python release/scripts/evaluate_corpus_completeness.py
# 输出: _reports/corpus-completeness-evaluation-<DATE>.md + .json
```

---

## 11. 维护者注意事项

1. **不要修改 corpus 文件**：它们是源 vault 的精确副本，任何修改会在下次导出时被覆盖。
2. **不要直接编辑 `index.json` / `manifest.json`**：这些是脚本自动生成的，请修改源 vault 或 profile 后重跑脚本。
3. **新增 QA 对**：在源 vault 的 `domain-10-troubleshooting-diagnostics/topic-qa-corpus/` 目录下添加 YAML/JSON 源文件，重跑脚本自动聚合。
4. **新增 domain**：编辑 `_meta/corpus-config/profiles/rag-full-profile.yaml` 的 `include` 列表。
5. **Token 估算方法**：`len(text) // 4`（粗略估算，非真实 tokenizer）。如需精确 token 数，用 `tiktoken` 或 Agent 侧 tokenizer 重新统计。

---

*本文件为 corpus package 的顶层索引。挂载 NAS 的 Agent 必须先读此文件，再决定加载策略。*
