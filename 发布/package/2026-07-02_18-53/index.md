---
title: "KUDIG Corpus Package — Agent Entry Point"
version: "2026.07.02"
generated_at: "2026-07-02T18:53:00+08:00"
profile: rag-full-profile.yaml
total_pages: 3903
total_tokens: 12279036
tier_counts:
  core: 1122
  supporting: 1693
  peripheral: 1088
qa_pairs: 15094
domains: 24
tags_unique: 1718
synthesis_pages: 10
topic_dictionary_pages: 564
tier: core
category: index
created: "2026-07-02"
last_updated: "2026-07-02"
---

# KUDIG Corpus Package

> **Agent 入口文件。** 首次加载本包时，先读本文件获取全局导航，再按 `AGENT-USAGE.md` 的加载策略按需拉取页面。
> **严禁整包加载到上下文** — corpus/ 总计 ~12.3M tokens。

---

## 1. 包概览

| 指标 | 值 |
|---|---|
| 版本 | 2026.07.02 |
| Profile | `rag-full-profile.yaml` (全量信噪比优化版 v2) |
| 总页数 | **3,903** |
| 总 Tokens | **~12.3M** |
| Core 页 | 1,122 (~3.1M tokens) |
| Supporting 页 | 1,693 (~4.8M tokens) |
| Peripheral 页 | 1,088 (~4.4M tokens) |
| QA 对 | 15,094 (去重后) |
| 知识域 | 24 个 domain + concepts/entities/skills/synthesis |
| 唯一标签 | 1,718 |

### v2 变更（相对上一版 18-40）

- **+574 页** (3,329 → 3,903)
- **+724K tokens** (11.5M → 12.3M)
- **新增 synthesis/**：10 个跨域合成页（共现分析产出，Agent 高价值连接页）
- **新增 topic-dictionary/**：564 个结构化术语文件（domain-17，平均 199 行/文件，~450K tokens）

---

## 2. 目录结构

```
package/2026-07-02_18-53/
├── index.md              ← 本文件：Agent 入口，全局导航
├── AGENT-USAGE.md        ← 加载策略参考（冷启动协议）
├── manifest.json         ← 包清单（版本/profile/tier/qa 计数）
├── index.json            ← 全页索引（path/title/summary/tags/tier/tokens）
├── corpus/               ← 语料主体（按 tier 分目录，68 MB）
│   ├── core/             ← 1,122 页（17 MB）
│   ├── supporting/       ← 1,693 页（25 MB）
│   └── peripheral/       ← 1,088 页（26 MB）
├── qa/
│   ├── qa-corpus.jsonl   ← 15,094 个 QA 对（去重后）
│   └── raw/              ← 63 个原始 QA 源文件
└── metadata/
    ├── page-stats.json   ← category/tag 分布统计
    ├── intent-corpus/    ← 意图识别语料
    ├── agent-specs/      ← 22 个 Agent 行为规范文档
    └── taxonomy/         ← 5 个受控标签词汇文件
```

---

## 3. Agent 加载策略

### 默认：Index-only 检索（推荐）

```
1. 读 manifest.json（<1KB）→ 确认版本
2. 读 index.json（2.1MB）→ 用 title + summary + tags 对 query 语义打分
3. 按需拉取 top-K 页 → 读 corpus/<tier>/<path>.md 单文件
4. 优先 core → supporting → peripheral
```

### 受限上下文

- 只加载 `corpus/core/`（1,122 页 / ~3.1M tokens）
- 按需取 supporting；peripheral 默认不加载

### QA 走独立 Embedding

- `qa/qa-corpus.jsonl` 单独喂向量库
- 字段：`input/output/source/type/tags/skill_ref/io_pair_id`
- 用途：few-shot 示例、意图分类、query 改写

---

## 4. Corpus 域分布（24 个域）

| 域 | 页数 | Tokens | 优先级 | 说明 |
|---|---|---|---|---|
| domain-17-system-foundation | 641 | 1.16M | medium | Linux 基础 + topic-dictionary 术语语料（564 文件） |
| domain-10-troubleshooting-diagnostics | 464 | 1.91M | high | 运维诊断核心：FTA/FEBM 方法论、QA 语料、工单案例 |
| skills | 458 | 953K | high | 操作技能与最佳实践 |
| entities | 385 | 420K | medium | 工具/项目/产品实体 |
| concepts | 280 | — | high | 抽象概念 |
| domain-14-ai-ml-infra | 158 | 795K | medium | AI/ML 基础设施 |
| domain-07-platform-engineering | 155 | 636K | high | 平台工程 |
| domain-02-workloads-applications | 139 | 579K | high | 工作负载与应用 |
| domain-03-networking-traffic | 135 | 768K | high | 网络与流量（eBPF、Service Mesh） |
| domain-20-application-patterns | 120 | 378K | low | 应用模式 |
| domain-01-cluster-fundamentals | 116 | 748K | high | 集群基础架构 |
| domain-11-production-operations | 104 | — | high | 生产运维（工单/模板/on-call） |
| domain-12-cloud-providers | 97 | — | medium | 云厂商 |
| domain-06-observability | 87 | 390K | high | 可观测性 |
| domain-05-security-compliance | 81 | 453K | high | 安全与合规 |
| domain-08-release-change-management | 70 | 318K | high | 发布与变更管理 |
| domain-15-specialized-tech | 69 | 474K | medium | 专项技术 |
| domain-09-reliability-engineering | 68 | — | high | 可靠性工程 |
| domain-19-landscape-references | 60 | — | low | CNCF 全景/Papers/知识图谱聚合页 |
| domain-18-manifests-patterns | 52 | 440K | medium | Manifest 模板与模式 |
| domain-16-database-middleware | 52 | — | medium | 数据库与中间件 |
| domain-04-storage-data | 52 | — | high | 存储与数据 |
| domain-13-container-runtime | 50 | — | medium | 容器运行时 |
| **synthesis** | **10** | — | **high** | **跨域合成页（共现分析产出）** |

---

## 5. Top Hub Pages（高连接度页面）

这些页面被最多其他页面引用，是知识图谱的锚点。Agent 在处理相关 query 时应优先加载。

| 页面路径 | 域 | 说明 |
|---|---|---|
| `domain-10-troubleshooting-diagnostics/SUMMARY.md` | 诊断 | 诊断域总索引，涵盖 FTA/FEBM/工单全链路 |
| `domain-10-troubleshooting-diagnostics/topic-fta/fta-methodology-and-agentic-prac...` | 诊断 | FTA 故障树方法论 + Agent 实践（33K tokens） |
| `domain-03-networking-traffic/00-core-k8s-networking/06-service-concepts-types.md` | 网络 | K8s Service 概念与类型详解 |
| `domain-01-cluster-fundamentals/` 各 SUMMARY | 集群 | 集群架构核心聚合页 |

---

## 6. Top Tags（高频标签，Top 20）

```
k8s: 2,341    prometheus: 1,044    kubelet: 804      etcd: 792
scheduler: 567    apiserver: 550    glossary: 548    grafana: 509
rag: 487      operator: 472      docker: 431      helm: 388
index: 376    crd: 352           rbac: 338        istio: 337
ingress: 337  controller-manager: 336    job: 320    cilium: 312
```

**全量唯一标签：1,718 个**

---

## 7. Synthesis 跨域合成页（10 页）

本轮新增的高价值页面，由共现分析自动生成，连接多个知识域：

| 页面 | 共现对 | 连接域 |
|---|---|---|
| `synthesis/kubernetes-prometheus.md` | K8s × Prometheus | 集群/可观测性 |
| `synthesis/kubernetes-etcd.md` | K8s × etcd | 集群/存储 |
| `synthesis/kubernetes-service.md` | K8s × Service | 集群/网络 |
| `synthesis/service-ingress.md` | Service × Ingress | 网络 |
| `synthesis/helm-gitops.md` | Helm × GitOps | 平台/发布 |
| `synthesis/slo-observability.md` | SLO × 可观测性 | 可靠性/可观测性 |
| `synthesis/statefulset-cloud-native-storage.md` | StatefulSet × 云存储 | 工作负载/存储 |
| `synthesis/container-runtime-image-security.md` | 运行时 × 镜像安全 | 运行时/安全 |
| `synthesis/ticket-agent-rag.md` | 工单智能体 × RAG | 运维/AI |
| `synthesis/index.md` | 合成域索引 | — |

---

## 8. Topic Dictionary 术语语料（564 页）

本轮新增，来自 `domain-17-system-foundation/topic-dictionary/`：

- **总计**：564 个结构化术语定义文件
- **平均大小**：199 行/文件
- **覆盖子域**：fundamentals, networking, security, operations, storage, tooling, multi-cloud, observability, scheduling, workloads, configuration, platform-engineering, specialized-workloads
- **最大文件**：`multi-cloud-operations.md` (4,658 行), `cli-commands.md` (3,757 行), `cloud-native-security-practices.md` (3,528 行)
- **用途**：术语归一化、few-shot 定义注入、Agent 概念 grounding

---

## 9. QA 语料

- **去重后**：15,094 个 QA 对
- **格式**：JSONL，字段 `input/output/source/type/tags/skill_ref/io_pair_id`
- **原始源**：63 个文件（`qa/raw/`）
- **用途**：embedding 检索 → few-shot 示例、意图分类、query 改写

---

## 10. 快速定位指南

### 按问题类型定位

| 问题类型 | 首选路径 | Tier |
|---|---|---|
| Pod 调度/Pending | `domain-10-troubleshooting-diagnostics/topic-fta/` | core |
| 网络不通/DNS | `domain-03-networking-traffic/` | core |
| 存储/PVC | `domain-04-storage-data/` | core |
| RBAC/权限 | `domain-05-security-compliance/` | core |
| 监控/告警 | `domain-06-observability/` | core |
| Helm/GitOps | `domain-07-platform-engineering/` | core |
| 发布/回滚 | `domain-08-release-change-management/` | core |
| SLO/SLA | `domain-09-reliability-engineering/` | core |
| 工单处理 | `domain-11-production-operations/` | core |
| 术语定义 | `domain-17-system-foundation/topic-dictionary/` | supporting |
| 跨域连接 | `synthesis/` | supporting |

### 按 Tier 优先级

1. **Core** (1,122 页)：高频命中场景，建议常驻或优先加载
2. **Supporting** (1,693 页)：常规支撑，按需拉取
3. **Peripheral** (1,088 页)：边缘知识，仅在 query 明确匹配时加载

---

## 11. 重新生成与归档

```bash
# 默认：写入 release/package/<YYYY-MM-DD_HH-MM>/
python3 scripts/export_corpus_for_nas.py

# 切换 profile
python3 scripts/export_corpus_for_nas.py -p rag-sre-profile.yaml

# 自定义输出（跳过自动时间戳）
python3 scripts/export_corpus_for_nas.py -o /tmp/export
```

**完整度评估**：

```bash
python3 scripts/evaluate_corpus_completeness.py
# 输出: _reports/corpus-completeness-evaluation-<DATE>.md + .json
```

---

## 12. 维护者注意事项

- 每次导出写入**新时间戳目录**，历史包不会被覆盖
- macOS `uchg` 标志已在导出前清理（`chflags -R nouchg`）
- 脚本拒绝清理 `release/`、`release/scripts/`、`release/package/`（防误删）
- `release/package/` 下保留所有历史导出，建议至少保留最近 3 个
- Profile 变更（如新增 include 路径）后需重新运行导出脚本
