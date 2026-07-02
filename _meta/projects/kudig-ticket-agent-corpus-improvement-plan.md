---
title: KUDIG 工单智能体语料改进规划（阿里云专有云版）
description: 面向阿里云专有云工单智能体的 KUDIG Database 语料改进路线图与执行计划
summary: 面向阿里云专有云工单智能体的 KUDIG Database 语料改进路线图与执行计划
category: project
tags:
- ai-agent
- ticket-agent
- corpus
- alicloud
- apsara-stack
- k8s
- sre
- improvement-plan
tier: supporting
created: '2026-06-26'
updated: '2026-06-26'
last_updated: 2026-06-26
status: active
audience:
- AI 工程师
- SRE
- 知识库维护者
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# KUDIG 工单智能体语料改进规划（阿里云专有云版）

> **目标读者**：负责构建阿里云专有云工单智能体的 AI 工程师、SRE、知识库维护者  
> **适用场景**：阿里云专有云（Apsara Stack）环境下的 K8s 运维工单自动处理  
> **规划周期**：分 3 个阶段执行，预计 4-6 周完成核心改进  
> **核心原则**：云厂商内容以 **阿里云 / 专有云** 为主，非阿里云场景仅作对照补充

---

## 1. 背景与目标

### 1.1 当前状态

KUDIG Database 已具备成为通用 SRE 诊断 Agent 语料的基础：

- 20 个 Domain，约 5,400+ Markdown 文件，95.6 MB
- 44 个 FTA 故障树、17 个 Skill 手册、5,159 对 QA、23 个案例
- 向量化 Pipeline 已就绪（mock / local / OpenAI 三档 provider）
- 评估得分：内容深度 8.7/10，但 SOP Agent 可执行性 6.5/10，命令多样性 8.2%

### 1.2 核心差距

从「阿里云专有云工单智能体」视角看，当前语料存在以下结构性缺口：

| 维度 | 现状 | 目标 |
|---|---|---|
| 工单闭环样本 | 极少，仅有培训文档 | 200+ 条「工单描述 → 分类 → 诊断 → 修复 → 验证 → 回复」完整样本 |
| 阿里云/专有云深度 | Terway、ACK 内容多，但专有云底座、ASO、天基等内容分散 | 系统化的专有云组件与排障知识 |
| Skill 深度 | 平均 122 行 | 核心 Skill 扩充至 400-600 行，覆盖边界条件和专有云场景 |
| 可执行性 | QA action 大量空缺，脚本覆盖率低 | QA action 覆盖率 80%+，每个核心 Skill 配套可执行脚本 |
| 内容密度 | Code 57.9%，Prose 31.1% | Prose 提升至 40%+，增强因果推理链 |
| 存储/发布管理 | 存储 Domain 仅 33 文件，缺少 Helm 深度指南 | 补齐 Velero/Rook/Longhorn、Helm 生产实践 |

### 1.3 目标

构建一套**面向阿里云专有云工单智能体**的高质量语料：

1. Agent 能准确理解工单描述并分类到对应 Skill/FTA
2. Agent 能给出可直接执行的诊断命令（优先阿里云 CLI / ACK 控制台 / 专有云 ASO）
3. Agent 能判断何时需要升级人工，并给出标准交接信息
4. Agent 能生成礼貌、清晰、符合阿里云客服规范的回复话术

---

## 2. 改进框架

### 2.1 语料三层架构

```
# 🟢 低风险：只读/信息收集，通常无副作用
┌─────────────────────────────────────────┐
│  Layer 1: 工单闭环样本（Ticket Cases）   │  ← 新增，最贴近工单场景
│  200+ 条：描述 → 分类 → 诊断 → 修复 → 验证 → 回复  │
├─────────────────────────────────────────┤
│  Layer 2: 提炼知识（concepts/entities/skills）│  ← 增强深度和专有云场景
│  核心 Skill 扩充、prose 增强、阿里云场景注入      │
├─────────────────────────────────────────┤
│  Layer 3: 源文档（domain-*/docs/）        │  ← 补齐缺漏领域
│  存储、Helm、Java on K8s、Cilium 生产落地等      │
└─────────────────────────────────────────┘
```
### 2.2 阿里云 / 专有云优先原则

本规划所有涉及云厂商的内容遵循以下原则：

- **首选阿里云/专有云**：命令、控制台路径、CLI 以 `aliyun` / `ack-ctl` / ASO / 天基 为主
- **次选开源/通用方案**：当阿里云无专属方案时，使用开源通用方案
- **对照补充**：必要时在附录中简要提及 AWS/GCP/Azure 差异，但正文不展开
- **专有云特有组件**：重点覆盖 ASO（Apsara Stack Operation）、天基、专有云 OSS/SLB/RDS/VPC 等底座

---

## 3. 分阶段执行计划

### 阶段 1：基础补齐（1-2 周）

**目标**：建立工单 Agent 语料骨架，补齐最紧迫的内容缺漏。

| # | 任务 | 产出 | 优先级 |
|---|---|---|---|
| 1.1 | 创建 ticket-agent 专用 profile | `_meta/corpus-config/profiles/rag-ticket-agent-profile.yaml` | P0 |
| 1.2 | 创建工单闭环样本库 | `domain-11-production-operations/ticket-cases/` 50+ 条 | P0 |
| 1.3 | 补齐存储 Domain 缺漏 | Velero / Rook / Longhorn / 有状态应用存储指南 | P0 |
| 1.4 | 补齐 Helm 生产实践 | `domain-08-release-change-management/01-gitops/99-helm-production-guide.md` | P0 |
| 1.5 | 提升 3-5 个核心 Skill 深度 | node-notready、pod-crashloop、service-unreachable 等 | P0 |
| 1.6 | 接入真实 Embedding 模型 | pipeline 默认从 mock 切至 local (bge-m3) | P1 |

### 阶段 2：专有云深化（2-3 周）

**目标**：将阿里云专有云组件、场景、话术深度注入语料。

| # | 任务 | 产出 | 优先级 |
|---|---|---|---|
| 2.1 | 专有云组件知识库 | ASO、天基、专有云 SLB/OSS/RDS/VPC 在 K8s 中的集成与排障 | P1 |
| 2.2 | 阿里云 CLI / ACK 控制台操作集 | 每个 Skill 增加阿里云专属诊断路径 | P1 |
| 2.3 | 工单回复话术模板 | 确认/请求信息/方案/升级/闭环五类话术 | P1 |
| 2.4 | 工单分类与路由规则 | `domain-11-production-operations/ticket-routing-rules.md` | P1 |
| 2.5 | 升级与交接协议 | `domain-11-production-operations/escalation-playbook.md` | P1 |
| 2.6 | 扩充工单样本至 200+ | 覆盖 P0/P1/P2/P3 各优先级 | P1 |

### 阶段 3：质量工程与评估（1-2 周）

**目标**：建立可持续迭代的质量机制。

| # | 任务 | 产出 | 优先级 |
|---|---|---|---|
| 3.1 | 命令多样性提升 | 参数化模板，目标唯一命令比例 25%+ | P2 |
| 3.2 | QA action 填充 | 覆盖率 80%+ | P2 |
| 3.3 | 验证脚本补充 | 每个核心 Skill 配套 verify-*.sh | P2 |
| 3.4 | 建立工单 Agent 评估集 | 100 条测试工单 + 评分标准 | P2 |
| 3.5 | 混合检索增强 | BM25 + Vector 混合检索 PoC | P2 |
| 3.6 | 反馈闭环机制 | 搜索结果点赞/点踩 → 索引迭代 | P3 |

---

## 4. 内容缺口详细清单

### 4.1 工单 Agent 专属语料（新增）

| 内容 | 路径 | 说明 |
|---|---|---|
| 工单闭环样本 | `domain-11-production-operations/ticket-cases/` | 每条包含 incident_id、priority、description、diagnosis_steps、fix_commands、verification、reply_template |
| 工单分类规则 | `domain-11-production-operations/ticket-routing-rules.md` | 关键词 → Domain/Skill/FTA 映射 |
| 升级标准 | `domain-11-production-operations/escalation-playbook.md` | P0/P1/P2 升级条件与交接信息模板 |
| 回复话术库 | `domain-11-production-operations/reply-templates/` | 五类标准话术 |
| 专有云组件索引 | `domain-12-cloud-providers/01-alibaba-cloud/apsara-stack-components.md` | ASO、天基、专有云底座组件 |

### 4.2 技术内容缺漏（补齐）

| Domain | 缺漏内容 | 说明 |
|---|---|---|
| domain-04-storage-data | Velero 生产指南 | 备份恢复、灾难恢复 |
| domain-04-storage-data | Rook-Ceph / Longhorn / OpenEBS | 分布式存储系统 |
| domain-04-storage-data | 有状态应用存储模式 | MySQL/PostgreSQL/Kafka/Elasticsearch |
| domain-08-release-change-management | Helm 生产实践 | chart 开发、values 管理、helm-secrets |
| domain-08-release-change-management | 渐进式交付 | Flagger、Argo Rollouts |
| domain-02-workloads-applications | Java on K8s | JVM 调优、Spring Boot/Quarkus |
| domain-02-workloads-applications | Serverless / Knative | 无服务器工作负载 |
| domain-03-networking-traffic | Cilium 生产落地 | 替换迁移、ClusterMesh |
| domain-03-networking-traffic | Gateway API 实战 | 从 Ingress 迁移、多租户路由 |
| domain-13-container-runtime | CRI-O / BuildKit | 容器运行时与镜像构建 |
| domain-14-ai-ml-infra | vLLM / TGI / KServe | LLM 推理服务引擎 |

### 4.3 质量增强（全 Domain）

| 问题 | 改进措施 | 目标 |
|---|---|---|
| Prose 密度 31.1% | 为代码块补充「为什么执行」的解释 | 40%+ |
| Skill 平均 122 行 | 扩充边界条件、版本差异、专有云场景 | 400-600 行 |
| 命令多样性 8.2% | 引入参数化模板 | 25%+ |
| QA action 空缺 | 批量填充修复命令 | 80%+ 覆盖率 |
| 重复 title 948 个 | 添加唯一标识后缀 | <100 个 |
| 版本差异标注不足 | 在命令和方案中标注 K8s 版本 | 核心文档 100% 覆盖 |

---

## 5. 专有云重点场景

以下场景是阿里云专有云工单高频场景，需优先覆盖：

| 场景 | 涉及组件 | 优先级 |
|---|---|---|
| 节点 NotReady | ECS、Terway、kubelet、containerd、ASO | P0 |
| Pod 调度失败 | 资源配额、污点容忍、节点池、调度器 | P0 |
| 服务无法访问 | 专有云 SLB、Terway、CoreDNS、安全组 | P0 |
| PVC 挂载失败 | 云盘 CSI、NAS/OSS、存储类 | P0 |
| 证书过期 | kubelet 证书、apiserver 证书、专有云底座证书 | P0 |
| 镜像拉取失败 | ACR/专有云镜像仓库、Harbor、imagePullSecret | P1 |
| 应用发布失败 | ArgoCD、Helm、Deployment 滚动更新 | P1 |
| 监控告警异常 | Prometheus、ARMS、SLS、Grafana | P1 |
| 集群升级失败 | kubeadm、ACK/专有云版本矩阵 | P1 |
| 安全策略拦截 | Kyverno、OPA、Pod Security Standards | P2 |

---

## 6. 验收标准

### 6.1 数量指标

| 指标 | 当前 | 阶段 1 目标 | 阶段 2 目标 | 阶段 3 目标 |
|---|---|---|---|---|
| 工单闭环样本 | ~5 | 50+ | 200+ | 200+ |
| Skill 平均行数 | 122 | 250+ | 400+ | 500+ |
| QA action 覆盖率 | ~5% | 40% | 80% | 90% |
| 命令多样性 | 8.2% | 15% | 25% | 30% |
| Prose 密度 | 31.1% | 35% | 40% | 42% |
| 验证脚本覆盖 | 38 个 | 60 个 | 100 个 | 120 个 |

### 6.2 质量指标

| 指标 | 目标 |
|---|---|
| 工单分类准确率 | ≥ 85% |
| 根因定位准确率 | ≥ 80% |
| 命令可执行率 | ≥ 90% |
| 回复话术合规率 | ≥ 90% |
| 升级判断准确率 | ≥ 85% |

---

## 7. 相关文件

- `_meta/corpus-config/profiles/rag-ticket-agent-profile.yaml` — Ticket Agent 语料配置
- `domain-11-production-operations/ticket-cases/` — 工单闭环样本库
- `domain-11-production-operations/ticket-routing-rules.md` — 工单分类与路由规则
- `domain-11-production-operations/escalation-playbook.md` — 升级与交接协议
- `domain-11-production-operations/reply-templates/` — 回复话术库
- `STRUCTURE.md` — 目录结构规范
- `_reports/DEEP-ASSESSMENT-2026-05-23.md` — 深度评估报告

---

## 8. 变更记录

| 日期 | 变更 | 作者 |
|---|---|---|
| 2026-06-26 | 创建本规划 | KUDIG Team |

---

*本规划会根据执行进展和评估反馈持续更新。*


<!-- risk-assessed -->
