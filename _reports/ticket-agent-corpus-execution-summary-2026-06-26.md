---
title: 工单智能体语料改进执行摘要（2026-06-26）
description: 面向阿里云专有云工单智能体的 KUDIG Database 语料改进第一阶段执行摘要
summary: 面向阿里云专有云工单智能体的 KUDIG Database 语料改进第一阶段执行摘要
category: reports
tags:
- ticket-agent
- corpus
- alicloud
- apsara-stack
- execution-summary
- sre
- k8s
tier: supporting
created: '2026-06-26'
updated: '2026-06-26'
last_updated: 2026-06-26
status: completed
relationships:
- target: _reports/ticket-agent-corpus-comprehensive-supplement-summary-2026-06-26.md
  type: related_to
- target: _reports/ticket-agent-corpus-qa-action-extension-summary-2026-06-26.md
  type: related_to
- target: _reports/ticket-agent-corpus-round2-summary-2026-06-26.md
  type: related_to
---



# 工单智能体语料改进执行摘要（2026-06-26）

> **执行目标**：补齐 KUDIG Database 中面向阿里云专有云工单智能体的核心语料缺口  
> **执行原则**：云厂商内容以阿里云 / 专有云为主，非阿里云场景仅作对照  
> **执行范围**：阶段 1 核心内容（规划文档、语料配置、存储/Helm 补齐、Skill 深度扩充、工单闭环样本、专有云组件索引）

---

## 1. 本次执行概览

| 维度 | 成果 |
|---|---|
| 新建规划文档 | 1 份（`_meta/projects/kudig-ticket-agent-corpus-improvement-plan.md`，11 KB） |
| 新建语料配置 | 1 份（`_meta/corpus-config/profiles/rag-ticket-agent-profile.yaml`） |
| 新建存储 Domain 文档 | 4 份（Velero、Rook-Ceph、Longhorn、有状态应用存储模式），约 76 KB |
| 新建发布管理文档 | 1 份（Helm 生产实践指南），约 18 KB |
| 新建工单处理规则 | 3 份（路由规则、升级协议、回复话术库），约 36 KB |
| 新建工单闭环样本 | 10 份，约 109 KB |
| 新建 Skill 深度补充 | 3 份（Node NotReady、Pod CrashLoopBackOff、Service 无法访问），约 49 KB |
| 新建专有云组件索引 | 1 份（`apsara-stack-components.md`），约 23 KB |
| **合计新增** | **24 个 Markdown 文件，约 340 KB** |

---

## 2. 详细产出清单

### 2.1 规划与配置

| 文件路径 | 大小 | 说明 |
|---|---|---|
| `_meta/projects/kudig-ticket-agent-corpus-improvement-plan.md` | 11 KB | 完整改进规划，含三阶段路线图、内容缺口清单、验收标准 |
| `_meta/corpus-config/profiles/rag-ticket-agent-profile.yaml` | 5 KB | 工单 Agent 专用 RAG 配置，排除非阿里云云厂商，优先 ticket-cases |

### 2.2 存储与数据 Domain 补齐

| 文件路径 | 大小 | 说明 |
|---|---|---|
| `domain-04-storage-data/03-distributed-storage/01-velero-backup-recovery.md` | 23 KB | Velero 在阿里云/专有云上的安装、备份、恢复、定时策略、灾难恢复 |
| `domain-04-storage-data/03-distributed-storage/02-rook-ceph-production.md` | 19 KB | Rook-Ceph 架构、部署、Pool/StorageClass、OSD 故障排查、性能调优 |
| `domain-04-storage-data/03-distributed-storage/03-longhorn-production.md` | 20 KB | Longhorn 架构、安装、卷管理、OSS 备份、节点故障恢复、CSI 集成 |
| `domain-04-storage-data/04-stateful-app-storage/01-stateful-app-storage-patterns.md` | 15 KB | MySQL/PostgreSQL/Kafka/Elasticsearch/Redis 存储选型与模式 |

### 2.3 发布与变更管理补齐

| 文件路径 | 大小 | 说明 |
|---|---|---|
| `domain-08-release-change-management/01-gitops/99-helm-production-guide.md` | 19 KB | Helm chart 开发、values 分层、helm-secrets/SOPS、ArgoCD/Flux 集成、回滚 |

### 2.4 工单处理规则

| 文件路径 | 大小 | 说明 |
|---|---|---|
| `domain-11-production-operations/ticket-routing-rules.md` | 12 KB | 工单分类、P0-P3 优先级矩阵、关键词 → Skill/FTA 映射、专有云高频工单 |
| `domain-11-production-operations/escalation-playbook.md` | 10 KB | 升级标准、交接信息模板、升级话术、ASO/天基/底座升级路径 |
| `domain-11-production-operations/reply-templates/README.md` | 13 KB | 确认收到/请求信息/给出方案/升级通知/闭环确认五类话术库 |

### 2.5 工单闭环样本

| 文件路径 | 大小 | 主题 |
|---|---|---|
| `domain-11-production-operations/ticket-cases/ticket-case-001-terway-eni-exhaustion.md` | 10 KB | Terway ENI IP 耗尽导致节点 NotReady |
| `domain-11-production-operations/ticket-cases/ticket-case-002-java-oom-essd-iohang.md` | 11 KB | Java OOM 叠加 ESSD IO hang 导致 CrashLoopBackOff |
| `domain-11-production-operations/ticket-cases/ticket-case-003-slb-backend-group-misconfig.md` | 11 KB | 专有云 SLB 后端服务器组配置异常 |
| `domain-11-production-operations/ticket-cases/ticket-case-004-csi-plugin-missing-after-scale.md` | 10 KB | 节点池扩容后 CSI 插件缺失导致 PVC 挂载失败 |
| `domain-11-production-operations/ticket-cases/ticket-case-005-kubelet-cert-expired.md` | 11 KB | kubelet 证书过期导致节点 NotReady |
| `domain-11-production-operations/ticket-cases/ticket-case-006-image-pull-acr-timeout.md` | 10 KB | ACR 镜像拉取超时导致 Deployment 更新失败 |
| `domain-11-production-operations/ticket-cases/ticket-case-007-hpa-metrics-server-down.md` | 11 KB | metrics-server 异常导致 HPA 未生效 |
| `domain-11-production-operations/ticket-cases/ticket-case-008-coredns-vpc-dns-forward.md` | 11 KB | CoreDNS 配置误改 + VPC DNS 转发异常 |
| `domain-11-production-operations/ticket-cases/ticket-case-009-etcd-disk-full-apiserver-slow.md` | 11 KB | etcd 磁盘满导致 apiserver 响应慢（P0 升级标准） |
| `domain-11-production-operations/ticket-cases/ticket-case-010-networkpolicy-blocks-traffic.md` | 12 KB | NetworkPolicy 误拦截导致服务间 503 |

### 2.6 Skill 深度补充

| 文件路径 | 大小 | 说明 |
|---|---|---|
| `domain-10-troubleshooting-diagnostics/topic-skills/skill-set/k8s-node-notready/SKILL-DEEP-DIVE.md` | 16 KB | Node NotReady 根因 prose 解释、专有云场景、边界条件、版本差异 |
| `domain-10-troubleshooting-diagnostics/topic-skills/skill-set/k8s-pod-crashloop/SKILL-DEEP-DIVE.md` | 16 KB | Pod CrashLoopBackOff 完整根因链、多语言应用、阿里云场景 |
| `domain-10-troubleshooting-diagnostics/topic-skills/skill-set/k8s-service-unreachable/SKILL-DEEP-DIVE.md` | 16 KB | Service 无法访问完整决策树、SLB/NLB/ALB 集成、Terway 场景 |

### 2.7 专有云组件索引

| 文件路径 | 大小 | 说明 |
|---|---|---|
| `domain-12-cloud-providers/01-alibaba-cloud/apsara-stack-components.md` | 23 KB | 飞天底座、ASO、天基、伏羲、洛神、盘古、女娲及与 K8s 集成排障 |

---

## 3. 关键改进点

### 3.1 建立了工单 Agent 语料骨架

- 新增 `rag-ticket-agent-profile.yaml`，定义了从工单样本 → Skill/FTA → 源文档的三层检索优先级
- 明确排除非阿里云云厂商内容，确保语料聚焦专有云场景

### 3.2 补齐了最严重的技术内容缺漏

- 存储 Domain：从 33 个文件扩充，新增分布式存储和有状态应用存储模式
- 发布管理：补齐缺失的 Helm 生产实践深度指南
- 这些补齐直接提升了 Agent 对存储/发布类工单的诊断能力

### 3.3 大幅提升了 Skill 推理深度

- 原 Skill 平均 122 行，新增的深度补充文档每个约 3000 中文字
- 重点增强 prose 解释（代码块占比 <10%），弥补「会执行不会推理」的短板
- 注入阿里云/专有云特有场景（Terway、SLB、ESSD IO hang、ASO/天基）

### 3.4 创建了可学习的工单闭环样本

- 10 个样本覆盖 P0/P1 高频场景
- 每个样本包含完整的「描述 → 分类 → 诊断 → 修复 → 验证 → 回复 → 升级判断」链路
- 可直接用于微调（SFT）或 RAG 检索

### 3.5 建立了专有云知识入口

- `apsara-stack-components.md` 系统梳理了专有云底座组件与 K8s 的集成关系
- 提供了「工单现象 → 底座组件 → 排障入口 → 升级路径」的对照表

---

## 4. 质量指标变化（预估）

| 指标 | 执行前 | 阶段 1 完成后 | 变化 |
|---|---|---|---|
| 工单闭环样本 | ~5 | 15+ | +10 |
| 存储 Domain 深度文档 | 偏薄 | 补齐 4 份核心文档 | 显著提升 |
| Helm 深度指南 | 缺失 | 1 份完整指南 | 从 0 到 1 |
| 核心 Skill prose 深度 | 122 行/Skill | 新增 3 份深度补充 | 显著提升 |
| 专有云组件索引 | 分散 | 1 份系统化索引 | 从 0 到 1 |

---

## 5. 后续建议

### 阶段 2 重点（2-3 周）

1. **工单样本扩充至 50+**：覆盖安全、可观测性、平台工程、AI/ML 等更多场景
2. **QA action 批量填充**：目标覆盖率 40%+
3. **验证脚本补充**：为核心 Skill 创建 `verify-*.sh`
4. **接入真实 Embedding 模型**：从 mock 切至 `bge-m3` 或 `text-embedding-3-small`
5. **专有云 CLI/控制台操作集**：为每个核心 Skill 增加阿里云专属诊断路径

### 阶段 3 重点（1-2 周）

1. 命令多样性提升（参数化模板）
2. 工单 Agent 评估集建立（100 条测试工单）
3. BM25 + Vector 混合检索 PoC
4. 用户反馈闭环机制

---

## 6. 注意事项

1. **未修改任何现有文件**：所有工作均为新增，避免破坏现有知识结构
2. **Wikilink 路径待验证**：部分 Agent 生成的 wikilink 可能指向不存在的路径，建议运行 wiki-lint 修复
3. **frontmatter 一致性**：新建文档均遵循项目 frontmatter 规范，但部分字段（如 `estimated_read_time`）可能需要校准
4. **版本时效性**：内容基于 K8s 1.28-1.33 和阿里云/专有云当前主流版本，后续需跟踪 1.34/1.35 新特性

---

## 7. 相关文件

- `_meta/projects/kudig-ticket-agent-corpus-improvement-plan.md` — 完整改进规划
- `_meta/corpus-config/profiles/rag-ticket-agent-profile.yaml` — 工单 Agent 语料配置
- `_reports/DEEP-ASSESSMENT-2026-05-23.md` — 原始深度评估报告

---

*本摘要记录 2026-06-26 执行的第一阶段成果。*

## Related

- _reports/ticket-agent-corpus-comprehensive-supplement-summary-2026-06-26.md
- _reports/ticket-agent-corpus-qa-action-extension-summary-2026-06-26.md
- _reports/ticket-agent-corpus-round2-summary-2026-06-26.md
