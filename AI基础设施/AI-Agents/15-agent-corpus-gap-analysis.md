---
title: Agent 语料库差距分析：kudig-database 作为 K8s 运维 Agent 语料还缺什么？ [02-ai-agents]
description: 'title: Agent 语料库差距分析：kudig-database 作为 K8s 运维 Agent 语料还缺什么？'
summary: 'title: Agent 语料库差距分析：kudig-database 作为 K8s 运维 Agent 语料还缺什么？'
category: general
tags:
- ai
- ai-agent
- etcd
- apiserver
- kubelet
- scheduler
- prometheus
- cilium
- flannel
- calico
tier: supporting
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 35min
intent_queries:
- Agent 语料库差距分析：kudig-database 作为 K8s 运维 Agent 语料还缺什么？ 是什么
- 如何 Agent 语料库差距分析：kudig-database 作为 K8s 运维 Agent 语料还缺什么？
- Kubernetes 14 ai ml infra 最佳实践
trigger_keywords:
- Agent
- 语料库差距分析：kudig-database
- 作为
- K8s
- 运维
- Agent
- 语料还缺什么？
- ai
prerequisites:
- kubectl-basics
- helm-basics
- prometheus-basics
- gitops-basics
- ebpf-basics
- cilium-basics
- cni-basics
- etcd-basics
- gpu-scheduling-basics
- backup-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




title: Agent 语料库差距分析：kudig-database 作为 K8s 运维 Agent 语料还缺什么？
description: '# Agent 语料库差距分析：kudig-database 作为 K8s 运维 Agent 语料还缺什么？'
category: ai-agent
tags:
- ai
- agent
- llm
- rag
- multi-agent
- [[etcd|etcd]]
- apiserver
- [[kubelet|kubelet]]
- scheduler
- [[Prometheus|prometheus]]
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- AI 工程师
- 架构师
- SRE
estimated_read_time: 15min
intent_queries:
- Agent 语料库差距分析：kudig-database 作为 K8s 运维 Agent 语料还缺什么？ 是什么
- 如何 Agent 语料库差距分析：kudig-database 作为 K8s 运维 Agent 语料还缺什么？
trigger_keywords:
- Agent
- 语料库差距分析：kudig-database
- 作为
- K8s
- 运维
- Agent
- 语料还缺什么？
- ai
authors:
- name: KUDIG Team
  role: contributor
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
---

# Agent 语料库差距分析：kudig-database 作为 K8s 运维 Agent 语料还缺什么？

> **文档类型**: 深度差距分析 | **最后更新**: 2026-03 | **关键词**: Agent 语料, 语料库差距, 结构化知识, K8s 运维 Agent, 知识补全, RAG, SOP, 症状映射, 安全护栏

---

## 概述

kudig-database 已覆盖 39 个知识域、1477 个文件、4300 万字，是目前极其全面的 [[Kubernetes|Kubernetes]] 生产运维知识库。但"人读的知识库"与"Agent 可用的语料库"之间存在**结构性差距**。

本文从 **Agent 视角** 系统审视现有内容，识别出 **10 大类缺失**，对每一类给出：
- 现有资产的逐文件审计
- 缺失内容的精确清单
- Agent 所需的目标格式（含可落地的数据结构）
- 从现有内容改造的具体路径
- 工作量估算

### 方法论

本分析遵循 **Agent Readiness Assessment** 三层模型：

```
Layer 3: 交互层 (Interaction)    → Agent 如何与人对话、确认、教学
Layer 2: 推理层 (Reasoning)      → Agent 如何判断、决策、选择路径
Layer 1: 知识层 (Knowledge)      → Agent 如何检索、理解、引用知识

每一层的缺失都会直接削弱 Agent 的能力上限:
  Layer 1 缺失 → Agent "不知道" → 回答错误或无法回答
  Layer 2 缺失 → Agent "不会判断" → 给出不合适的建议
  Layer 3 缺失 → Agent "不会交流" → 用户体验差、信任度低
```

### 现有资产全景盘点

| 资产类别 | 目录 | 文件数 | 总容量 | Agent 价值评估 |
|---------|------|-------|--------|---------------|
| 故障排查大全 | `故障诊断/` | 42 | ~1.5M 字 | ★★★★★ 排障 Agent 核心语料 |
| 结构化排障 | `故障诊断/高级排障/structural-` | 48+ | ~600K 字 | ★★★★★ 已有决策树结构 |
| FTA 故障树 | `故障诊断/FTA故障树/list/` | 37 | ~1.5M 字 | ★★★★★ Agent 推理链直接输入 |
| YAML 清单手册 | `清单模式/` | 36 | ~1.7M 字 | ★★★★☆ 模板生成 Agent 基础 |
| K8s Events 大全 | `系统基础/` | 15 | ~800K 字 | ★★★★☆ 事件解读 Agent 关键 |
| 运维词典 | `系统基础/topic-dictionary/` | 16 | ~1.4M 字 | ★★★★☆ 术语和最佳实践 |
| FEBM 取证循证 | `故障诊断/FEBM方法论/` | 11 | ~1.0M 字 | ★★★★☆ 诊断方法论 |
| FTA 方法论 | `故障诊断/FTA故障树/` (非 list) | 30 | ~500K 字 | ★★★★☆ Agent 编排理论 |
| 控制平面 | `集群基础/` | 28 | ~1.1M 字 | ★★★★☆ 核心组件深度知识 |
| 网络 | `网络/` | 41 | ~900K 字 | ★★★★☆ 网络排障 Agent |
| 安全 | `安全/` | 21 | ~440K 字 | ★★★☆☆ 安全审计 Agent |
| 可观测性 | `可观测性/` | 27 | ~600K 字 | ★★★☆☆ 监控 Agent |
| 生产运维 | `生产运维/` | 24 | ~500K 字 | ★★★☆☆ SOP 改造基础 |
| 速查卡 | `系统基础/topic-cheat-sheet/` | 3 | ~130K 字 | ★★★☆☆ 可直接检索 |
| 迁移指南 | `发布变更/topic-migration/` | 10 | ~150K 字 | ★★★☆☆ 迁移 Agent 蓝本 |
| 云厂商 | `云厂商/` | 13目录 | ~200K 字 | ★★☆☆☆ ACK 较完整，其余薄弱 |

---

## 差距总览

```
评估维度              现有覆盖度    Agent 可用度    优先级
─────────────────────────────────────────────────────────
1. 结构化元数据        ★☆☆☆☆       ★☆☆☆☆        🔴 P0 - 最高
2. 标准操作规程(SOP)   ★★★☆☆       ★★☆☆☆        🔴 P0 - 最高
3. 症状→原因映射表     ★★★☆☆       ★★☆☆☆        🔴 P0 - 最高
4. 命令输出解读语料     ★★☆☆☆       ★☆☆☆☆        🟡 P1 - 高
5. 真实案例/工单语料    ★★☆☆☆       ★☆☆☆☆        🟡 P1 - 高
6. 对话式交互语料       ☆☆☆☆☆       ☆☆☆☆☆        🟡 P1 - 高
7. 版本差异矩阵        ★★☆☆☆       ★☆☆☆☆        🟢 P2 - 中
8. 判断决策条件         ★★★☆☆       ★★☆☆☆        🟢 P2 - 中
9. 安全护栏规则         ★☆☆☆☆       ☆☆☆☆☆        🟢 P2 - 中
10. 多语言/多云适配     ★★☆☆☆       ★☆☆☆☆        ⚪ P3 - 低
```

---

## 🔴 P0 级缺失：Agent 核心能力依赖（知识层 Layer 1）

> P0 缺失直接决定 Agent 能否正确检索和理解知识。这些缺失不解决，Agent 根本无法工作。

### 1. 结构化元数据层（Metadata Layer）

**现状审计**：所有 1477 篇文档均为纯 Markdown 长文，无任何 YAML Front Matter 或机器可读元数据。文档内部无语义分块标记。Agent 在 RAG 检索时只能依赖文本相似度，无法利用元数据过滤。

**逐域影响评估**：

| 受影响域 | 文件数 | 缺失的元数据类型 | 影响程度 |
|---------|-------|----------------|----------|
| `故障诊断/` | 42 | 缺 intent_queries、severity、symptom_tags | 致命 - 排障场景检索准确率低 |
| `清单模式/` | 36 | 缺 resource_type、api_version、use_case | 严重 - YAML 生成 Agent 无法按资源类型过滤 |
| `系统基础/` | 15 | 缺 event_type、source_component、severity | 严重 - 事件解读 Agent 无法精确匹配 |
| `集群基础/` | 28 | 缺 component、layer、failure_mode | 高 - 控制平面诊断检索耗时长 |
| `故障诊断/FTA故障树/list/` | 37 | 缺 component、top_event、gate_count | 中 - FTA 已自带结构但缺查找入口 |

**10 类缺失的元数据字段详述**：

| 缺失字段 | 说明 | Agent 使用场景 | 示例值 |
|---------|------|----------------|--------|
| `id` | 文档唯一标识符 | 交叉引用、关系图谱 | `D12-05` |
| `domain` | 所属知识域 | 缩小检索范围 | `troubleshooting` |
| `tags` | 语义标签 | RAG 向量检索增强 | `[pod, pending, scheduling]` |
| `difficulty` | 难度级别 | 学习 Agent 路径规划 | `intermediate` |
| `target_roles` | 目标角色 | 按角色过滤内容 | `[sre, ops-engineer]` |
| `k8s_versions` | 适用版本 | 版本相关查询过滤 | `[1.28, 1.29, 1.30]` |
| `intent_queries` | 用户可能的提问方式 | 意图识别→文档匹配 | `"Pod Pending 怎么办"` |
| `requires` | 前置知识 | 知识图谱导航 | `[D4-01, D4-19]` |
| `related` | 关联文档 | 推荐进一步阅读 | `[D12-06, D12-07]` |
| `chunk_markers` | 文档内部分块标记 | 精细化检索粒度 | `<!-- chunk: diagnosis-step-3 -->` |

**建议补全方案 —— 完整 Front Matter 规范**：

```yaml
# 以 故障诊断/05-pod-pending-diagnosis.md 为例
---
id: D12-05
domain: troubleshooting
title: Pod Pending 状态深度诊断
tags: [pod, pending, scheduling, resource, node, taint, affinity]
difficulty: intermediate
target_roles: [sre, ops-engineer, developer]
k8s_versions: [1.25, 1.26, 1.27, 1.28, 1.29, 1.30, 1.31, 1.32]
severity_context: P0-P2  # 这篇文档覆盖的问题严重级别范围
intent_queries:
  - "Pod 一直 Pending 怎么办"
  - "Pod stuck in Pending state"
  - "调度失败怎么排查"
  - "Insufficient cpu/memory"
  - "no nodes available to schedule"
requires: [D4-01, D4-19, D1-01]  # 工作负载概览、调度器配置、架构概览
related: [D12-06, D12-07, D12-24, D33-05]  # Node NotReady, OOM, Quota, 调度事件
fta_ref: 故障诊断/FTA故障树/list/pod-fta.md  # 关联故障树
structural_ref: 故障诊断/高级排障/structural-05-workloads/01-pod-troubleshooting.md
---
```

**工作量估算**：
- 批量添加基础 front matter（50% 可脚本自动生成）：~3 天
- 人工补全 intent_queries 和关联关系：~7 天
- 文档内部 chunk_markers 标注（核心 100 篇）：~5 天
- **合计：~15 人天**

### 2. 标准操作规程（SOP / Runbook）

**现状审计**：

现有与 SOP 相关的内容分布在多个位置，但格式偏向“知识讲解”而非“可执行指令序列”：

| 现有资源 | 内容 | Agent SOP 可用度 | 缺失距离 |
|---------|------|------------------|----------|
| `系统基础/topic-dictionary/12-incident-management-runbooks.md` | 3245 行，事故管理框架+流程 | ★★★☆☆ 有框架但缺可执行步骤 | 中 - 需提取为结构化 SOP |
| `生产运维/23-incident-response-handling.md` | 应急响应处理 | ★★☆☆☆ 理论偏重 | 大 - 需重写为分钟级步骤 |
| `生产运维/22-change-management-process.md` | 变更管理流程（62K） | ★★★☆☆ 较完整但非 Agent 格式 | 中 |
| `生产运维/17-disaster-recovery-drills.md` | 灾备演练（45K） | ★★★☆☆ 有步骤但非结构化 | 中 |
| `集群基础/10-plane-backup-disaster-recovery.md` | etcd 备份恢复（58K） | ★★★★☆ 有具体命令 | 小 - 需提取整合 |
| `安全/10-certificate-management.md` | 证书管理（40K） | ★★★☆☆ 有原理和命令 | 中 - 需拆分为按组件的 SOP |
| `集群基础/07-upgrade-paths-strategy.md` | 升级策略 | ★★☆☆☆ 策略为主 | 大 - 缺分版本操作清单 |
| `发布变更/topic-migration/` | 10 篇迁移指南 | ★★★☆☆ 流程清晰但非 SOP 格式 | 中 |

**缺失的 30 个核心 SOP 完整清单**：

| # | SOP 名称 | 触发场景 | 严重级 | 现有可复用内容 | 改造工作量 |
|---|---------|---------|--------|----------------|----------|
| 1 | etcd 紧急备份恢复 | etcd 不可用/数据损坏 | P0 | 集群基础/10, 集群基础/19 | 小 |
| 2 | API Server 不可用应急 | apiserver 无响应 | P0 | 故障诊断/01, 集群基础/12 | 中 |
| 3 | 集群证书过期轮换 | 证书即将/已过期 | P0 | 安全/10, 故障诊断/13 | 中 |
| 4 | 生产环境 Pod 大规模 CrashLoop | 核心服务崩溃 | P0 | 故障诊断/08 | 中 |
| 5 | Node NotReady 应急处置 | 多节点同时 NotReady | P0 | 故障诊断/06, 故障诊断/09 | 中 |
| 6 | 集群网络分区处理 | 跨节点网络不通 | P0 | 故障诊断/03, 故障诊断/25 | 大 |
| 7 | 日常巡检 SOP | 每日/每周巡检 | P1 | 可观测性/13 | 大 |
| 8 | 节点上下线（drain/cordon） | 节点维护 | P1 | 零散 | 大 |
| 9 | 集群版本升级 | 计划升级 | P1 | 集群基础/07, 集群基础/18 | 大 |
| 10 | 应用滚动更新/回滚 | Deployment 更新失败 | P1 | 故障诊断/11, 工作负载/02 | 中 |
| 11 | HPA/VPA 调整 | 扩缩容异常 | P1 | 故障诊断/17, 工作负载/21 | 中 |
| 12 | DNS 问题应急 | 服务发现失败 | P1 | 故障诊断/26, 网络/28 | 中 |
| 13 | PVC 存储故障处理 | PVC Pending/挂载失败 | P1 | 故障诊断/14, 存储/09 | 中 |
| 14 | Ingress/Gateway 故障处理 | 外部访问异常 | P1 | 故障诊断/15, 网络/19-26 | 中 |
| 15 | RBAC 权限问题处理 | 权限不足/过大 | P1 | 故障诊断/12, 安全/07 | 中 |
| 16 | Secret/ConfigMap 热更新 | 配置变更不生效 | P2 | 故障诊断/19 | 小 |
| 17 | CronJob 故障处理 | 定时任务不触发 | P2 | 故障诊断/18 | 小 |
| 18 | DaemonSet 故障处理 | 系统组件异常 | P2 | 故障诊断/20 | 小 |
| 19 | StatefulSet 故障处理 | 有状态服务异常 | P2 | 故障诊断/21 | 小 |
| 20 | 监控告警系统问题 | Prometheus/Alertmanager 异常 | P2 | 故障诊断/30 | 中 |
| 21 | Helm Release 故障处理 | Chart 部署/升级失败 | P2 | 故障诊断/36 | 中 |
| 22 | ArgoCD 同步故障处理 | GitOps 同步失败 | P2 | 故障诊断/38 | 中 |
| 23 | 镜像仓库故障处理 | 镜像拉取失败 | P2 | 故障诊断/27 | 中 |
| 24 | 集群自动扩缩容问题 | Cluster Autoscaler 异常 | P2 | 故障诊断/28 | 中 |
| 25 | NetworkPolicy 故障处理 | 网络策略异常 | P2 | 故障诊断/16 | 小 |
| 26 | GPU 设备故障处理 | GPU 不可见/分配失败 | P2 | AI基础设施/03-04 | 大 |
| 27 | Velero 备份恢复 SOP | 集群级备份恢复 | P2 | 故障诊断/31 | 中 |
| 28 | 多集群故障转移 | 跨集群切换 | P3 | 故障诊断/37 | 大 |
| 29 | 混沌工程演练 SOP | 故障注入测试 | P3 | 故障诊断/42 | 大 |
| 30 | 安全事件应急处置 | 安全漏洞/入侵 | P3 | 安全/20 | 大 |

**Agent 可执行 SOP 目标格式**：

```yaml
sop:
  id: SOP-001
  name: "etcd 紧急备份恢复"
  trigger: "etcd 集群不可用 / 数据损坏 / 多数节点 unhealthy"
  severity: P0
  estimated_time: "30-60min"
  risk_level: high  # Agent 应使用 Level 1 建议模式
  prerequisites:
    - check: "具有 etcd 节点 SSH 访问权限"
      command: "ssh etcd-node-1 'echo ok'"
    - check: "已安装 etcdctl v3.5+"
      command: "etcdctl version"
      expected: "etcdctl version: 3.5"
  steps:
    - id: 1
      action: "确认 etcd 集群状态"
      command: "ETCDCTL_API=3 etcdctl --endpoints=https://127.0.0.1:2379 --cacert=/etc/kubernetes/pki/etcd/ca.crt --cert=/etc/kubernetes/pki/etcd/healthcheck-client.crt --key=/etc/kubernetes/pki/etcd/healthcheck-client.key endpoint health --cluster"
      expected_output: "is healthy: true"
      on_failure:
        action: "跳至步骤 4（单节点恢复）"
        reason: "集群无法正常响应，需从快照恢复"
    - id: 2
      action: "检查成员列表"
      command: "etcdctl member list --write-out=table"
      parse_fields: ["ID", "STATUS", "NAME", "PEER_ADDRS"]
      check: "STATUS 全部为 started"
    - id: 3
      action: "执行快照备份"
      command: "etcdctl snapshot save /backup/etcd-$(date +%Y%m%d-%H%M%S).db"
      validation:
        command: "etcdctl snapshot status /backup/etcd-*.db --write-out=table"
        expected: "revision > 0"
    - id: 4
      action: "从快照恢复（仅当集群不可用时）"
      danger_level: critical
      confirm_required: true
      confirm_message: "即将从快照恢复 etcd，这将丢失快照之后的所有变更。确认继续？"
      command: "etcdctl snapshot restore /backup/etcd-latest.db --data-dir=/var/lib/etcd-restored"
  rollback:
    - "停止恢复操作，保留原始数据目录"
    - "从其他健康节点重建: etcdctl member add"
  knowledge_refs:
    - 集群基础/10-plane-backup-disaster-recovery.md
    - 集群基础/19-etcd-operations.md
    - 故障诊断/02-control-plane-etcd-troubleshooting.md
```

**工作量估算**：
- P0 级 SOP（6 个）：~6 人天（每个 1 天，需从现有文档提取+结构化）
- P1 级 SOP（9 个）：~9 人天
- P2/P3 级 SOP（15 个）：~10 人天
- **合计：~25 人天**

### 3. 症状→原因映射表（Symptom-Cause Matrix）

**现状审计**：

现有知识库中，症状与原因的关系散落在多个层级的文档中：

| 现有数据源 | 结构化程度 | 包含症状数 | Agent 可直接用？ |
|---------|-----------|-----------|---------------|
| `故障诊断/FTA故障树/list/` 37 个故障树 | ★★★★★ 树状结构 | ~200+ | ✗ 需拕平为查找表 |
| `故障诊断/高级排障/structural-` 48+ 篇 | ★★★★☆ 决策树 | ~150+ | ✗ 需提取为映射表 |
| `故障诊断/` 42 篇排障文档 | ★★★☆☆ 长文叙述 | ~300+ | ✗ 需结构化提取 |
| `系统基础/` 15 篇事件文档 | ★★★☆☆ 按事件分类 | ~100+ | ✗ 需映射到症状 |
| `故障诊断/08` Pod 状态速查表 | ★★★★☆ 有表格 | ~10 | △ 接近但缺诊断命令 |

**Agent 所需的目标格式 —— 快速查找表**：

```yaml
symptom_cause_map:
  # === Pod 异常状态类（15 种） ===
  - symptom: "Pod 状态 CrashLoopBackOff"
    category: pod_status
    urgency: high
    possible_causes:
      - cause: "应用启动失败"
        probability: high
        diagnosis:
          commands:
            - "kubectl logs <pod> --previous"
            - "kubectl logs <pod> -c <container>"
          indicators: ["Exit Code 1", "Error in logs", "panic", "fatal"]
          next_step: "检查应用日志中的具体错误"
        fix_pattern: "修复应用代码或配置"
        knowledge_ref: "故障诊断/08-pod-comprehensive-troubleshooting.md#crashloopbackoff"
      - cause: "OOMKilled"
        probability: medium
        diagnosis:
          commands:
            - "kubectl describe pod <pod> | grep -A5 'Last State'"
            - "kubectl describe pod <pod> | grep -i oom"
          indicators: ["OOMKilled", "Exit Code 137", "reason: OOMKilled"]
          next_step: "检查容器 memory limits 和应用实际内存使用"
        fix_pattern: "调大 limits.memory 或优化应用内存"
        knowledge_ref: "故障诊断/07-oom-memory-diagnosis.md"
        fta_ref: "故障诊断/FTA故障树/list/pod-fta.md#oomkilled"
      - cause: "Liveness Probe 失败"
        probability: medium
        diagnosis:
          commands:
            - "kubectl describe pod <pod> | grep -A10 'Liveness'"
            - "kubectl get events --field-selector involvedObject.name=<pod>"
          indicators: ["Liveness probe failed", "Unhealthy"]
          next_step: "检查 Probe 配置和应用健康检查端点"
        fix_pattern: "调整 Probe 参数或修复健康检查端点"
        knowledge_ref: "系统基础/04-probe-health-check-events.md"
      - cause: "配置错误（ConfigMap/Secret 缺失）"
        probability: low
        diagnosis:
          commands:
            - "kubectl describe pod <pod> | grep -A5 'Events'"
            - "kubectl get configmap <cm> -n <ns>"
          indicators: ["CreateContainerConfigError", "configmap not found"]
        knowledge_ref: "故障诊断/19-configmap-secret-troubleshooting.md"

  - symptom: "Pod 状态 Pending"
    category: pod_status
    urgency: varies  # P0(生产核心) 到 P2(测试环境)
    possible_causes:
      - cause: "集群资源不足（CPU/内存）"
        probability: high
        diagnosis:
          commands:
            - "kubectl describe pod <pod> | grep -A20 'Events'"
            - "kubectl describe nodes | grep -A5 'Allocated resources'"
            - "kubectl top nodes"
          indicators: ["Insufficient cpu", "Insufficient memory", "0/N nodes are available"]
        knowledge_ref: "故障诊断/05-pod-pending-diagnosis.md#3"
      - cause: "节点污点/亲和性不匹配"
        probability: high
        diagnosis:
          commands:
            - "kubectl get nodes --show-labels"
            - "kubectl describe nodes | grep Taints"
            - "kubectl get pod <pod> -o yaml | grep -A10 'nodeSelector|affinity|tolerations'"
          indicators: ["didn't match Pod's node affinity", "had taint", "node(s) didn't match"]
        knowledge_ref: "故障诊断/05-pod-pending-diagnosis.md#4"
      - cause: "PVC 未绑定"
        probability: medium
        diagnosis:
          commands:
            - "kubectl get pvc -n <ns>"
            - "kubectl describe pvc <pvc>"
          indicators: ["Pending", "waiting for a volume", "no persistent volumes available"]
        knowledge_ref: "故障诊断/14-pvc-storage-troubleshooting.md"

  # === Service/网络异常类（12 种） ===
  - symptom: "Service 无法访问"
    category: network
    urgency: high
    possible_causes:
      - cause: "Endpoint 为空"
        probability: high
        diagnosis:
          commands:
            - "kubectl get endpoints <svc> -n <ns>"
            - "kubectl get pods -l <selector> -n <ns>"
          indicators: ["<none>", "ENDPOINTS: "]
        knowledge_ref: "故障诊断/10-service-comprehensive-troubleshooting.md"
      - cause: "Selector 不匹配"
        probability: high
        diagnosis:
          commands:
            - "kubectl get svc <svc> -o yaml | grep -A5 selector"
            - "kubectl get pods --show-labels -n <ns>"
          indicators: ["标签不一致"]
      - cause: "Pod 未就绪"
        probability: medium
        diagnosis:
          commands: ["kubectl get pods -l <selector> -o wide"]
          indicators: ["0/1 Running", "CrashLoopBackOff"]
      - cause: "NetworkPolicy 阻断"
        probability: low
        diagnosis:
          commands: ["kubectl get networkpolicy -n <ns> -o yaml"]
        knowledge_ref: "故障诊断/16-networkpolicy-troubleshooting.md"
```

**需覆盖的 70+ 症状完整清单**：

| 症状类别 | 具体症状 | 数量 | 来源文档 |
|---------|---------|------|----------|
| **Pod 异常状态** | CrashLoopBackOff, Pending, ImagePullBackOff, OOMKilled, Evicted, ContainerCreating, Init:Error, Terminating, Unknown, RunContainerError, CreateContainerConfigError, PreemptionByScheduler, BackOff, ErrImageNeverPull, InvalidImageName | 15 | 故障诊断/05,07,08 |
| **Node 异常** | NotReady, MemoryPressure, DiskPressure, PIDPressure, NetworkUnavailable, CordonedUnexpectedly, KubeletDown, ContainerRuntimeDown, ClockSkew, KernelDeadlock | 10 | 故障诊断/06,09,35 |
| **Service/网络** | ServiceUnavailable, EmptyEndpoints, DNSResolutionFailed, IngressRouteError, TLSHandshakeFailed, ConnectionRefused, ConnectionTimeout, LoadBalancerPending, ExternalIPNotAssigned, GatewayRouteNotMatched, CrossNodeNetworkFailure, MTLSCertificateError | 12 | 故障诊断/10,15,16,25,26 |
| **存储异常** | PVCPending, VolumeAttachFailed, VolumeMountFailed, CSIDriverError, StorageClassNotFound, VolumeResizeFailed, SnapshotFailed, DataCorruption | 8 | 故障诊断/04,14 |
| **控制平面** | APIServerUnresponsive, APIServer5xx, etcdLeaderLost, etcdHighLatency, SchedulerBacklog, ControllerManagerStuck, WebhookTimeout, APFThrottling, etcdDiskSpaceLow, CertificateExpired | 10 | 故障诊断/01,02 |
| **调度异常** | Unschedulable, PreemptionFailure, PriorityClassMissing, TopologySpreadViolation, ResourceQuotaExceeded, LimitRangeViolation | 6 | 故障诊断/05,17,24 |
| **安全/权限** | Forbidden, Unauthorized, CertificateExpired, PodSecurityViolation, NetworkPolicyDrop, AuditPolicyError, SecretNotFound, ServiceAccountTokenExpired | 8 | 故障诊断/12,13,32 |
| **运维工具** | HelmReleaseFailed, ArgoCDOutOfSync, VeleroBackupFailed, ClusterAutoscalerNoScale | 4 | 故障诊断/28,36,38 |
| **合计** | | **73** | |

**工作量估算**：
- 从 FTA 故障树自动提取映射（50%）：~3 天
- 从 故障诊断 文档人工提取补充：~5 天
- 从 系统基础 事件文档关联映射：~2 天
- 编写诊断命令和 indicators：~5 天
- **合计：~15 人天**

---

## 🟡 P1 级缺失：Agent 质量提升依赖（推理层 Layer 2 + 交互层 Layer 3）

> P1 缺失不会让 Agent 完全失能，但会显著降低回答质量和用户信任度。

### 4. 命令输出解读语料（Command Output Interpretation）

**现状审计**：

文档中包含大量 kubectl 命令（`domain-12` 就有 3000+ 个可执行命令），但**极少包含命令输出的逐字段解读**。这意味着 Agent 可以告诉用户“运行这个命令”，但无法帮助用户“解读输出结果”。

**逐类审计**：

| 命令类别 | 现有覆盖 | Agent 需要 | 差距 | 优先级 |
|---------|---------|---------|------|--------|
| `kubectl describe pod` | 有命令但无输出解读 | 逐字段解析 Conditions、Events、Container Status | **大** | 🔴 最高 |
| `kubectl describe node` | 有命令但无解读 | Conditions 各状态含义、Allocated resources 判断 | **大** | 🔴 最高 |
| `kubectl get events` | 系统基础 有事件分类 | 异常模式识别、时间序列分析 | **中** | 🔴 最高 |
| `kubectl top node/pod` | 无 | 性能判断阈值（CPU >80%、Memory >85%） | **大** | 🟡 高 |
| `etcdctl endpoint status` | 集群基础/19 有部分 | 健康判断标准、Raft 状态解读 | **中** | 🟡 高 |
| `etcdctl endpoint health` | 集群基础/19 有 | 异常模式识别 | **小** | 🟡 高 |
| `crictl ps/inspect` | 集群基础/21 有部分 | 运行时状态解读 | **中** | 🟢 中 |
| `helm status/list` | 故障诊断/36 有部分 | Release 状态解读、异常模式 | **中** | 🟢 中 |
| `argocd app get` | 故障诊断/38 有部分 | 同步状态解读、Health/Sync 判断 | **中** | 🟢 中 |
| Prometheus 告警规则 | 可观测性 有规则但无解读 | 告警含义映射、严重级判断 | **大** | 🟢 中 |

**Agent 所需的目标格式 —— 输出解读模板**：

```yaml
command_output_interpretation:
  command: "kubectl describe pod <pod>"
  sections:
    - section: "Conditions"
      fields:
        - field: "PodScheduled"
          normal: "True"
          abnormal_patterns:
            - value: "False"
              meaning: "Pod 未被调度，检查资源/节点/污点"
              next_action: "查看 Events 中的 FailedScheduling 信息"
        - field: "Ready"
          normal: "True"
          abnormal_patterns:
            - value: "False"
              meaning: "Pod 未就绪，可能是 Readiness Probe 失败"
              next_action: "检查 Readiness Probe 配置和应用状态"
    - section: "Events"
      pattern_matching:
        - pattern: "FailedScheduling.*Insufficient (cpu|memory)"
          meaning: "集群资源不足"
          severity: high
          action: "检查节点资源使用情况，考虑扩容"
        - pattern: "FailedMount.*timeout"
          meaning: "存储卷挂载超时"
          severity: high
          action: "检查 CSI 驱动、存储后端状态"
        - pattern: "Unhealthy.*Liveness probe failed"
          meaning: "存活探针失败，容器将被重启"
          severity: medium
          action: "检查探针配置和应用健康状态"
    - section: "Container Status"
      fields:
        - field: "State.Waiting.Reason"
          patterns:
            - value: "CrashLoopBackOff"
              meaning: "容器反复崩溃"
              next: "查看 --previous 日志"
            - value: "ImagePullBackOff"
              meaning: "镜像拉取失败"
              next: "检查镜像名、凭证、网络"
            - value: "CreateContainerConfigError"
              meaning: "ConfigMap/Secret 配置错误"
              next: "检查引用的 CM/Secret 是否存在"

  command: "kubectl top nodes"
  interpretation:
    thresholds:
      cpu_warning: "80%"   # CPU 使用率 > 80% 告警
      cpu_critical: "90%"  # CPU 使用率 > 90% 严重
      memory_warning: "85%"
      memory_critical: "95%"
    output_format: "NAME  CPU(cores)  CPU%  MEMORY(bytes)  MEMORY%"
    abnormal_patterns:
      - pattern: "CPU% > 90"
        meaning: "节点 CPU 严重超载，可能影响新 Pod 调度"
      - pattern: "MEMORY% > 95"
        meaning: "内存即将耗尽，可能触发驱逐"
```

**工作量估算**：~12 人天（Top 20 高频命令 × 每个 0.5 天 + 模式库编写）

### 5. 真实案例/工单语料（Real-world Cases / Ticket Corpus）

**现状审计**：

| 现有案例源 | 案例数 | 格式质量 | Agent 可用度 |
|---------|-------|---------|----------|
| `发布变更/topic-migration/10-real-world-case-study.md` | 1 个完整迁移案例 | ★★★★☆ | 中 - 仅覆盖迁移场景 |
| `故障诊断/` 各篇文档中散落的案例 | ~20 个片段 | ★★☆☆☆ | 低 - 非结构化 |
| `系统基础/topic-dictionary/02-failure-patterns-analysis.md` | ~15 个模式 | ★★★☆☆ | 中 - 模式而非工单 |
| `可观测性/22-best-practices-case-studies.md` | ~5 个案例 | ★★★☆☆ | 中 - 偏监控场景 |

**Agent 所需的工单格式（50-100 个）**：

```yaml
incident:
  id: INC-2025-001
  severity: P1
  reported_at: "2025-12-15 14:32"
  reported_by: "开发工程师 A"
  symptom: "生产环境订单服务 Pod 频繁重启，过去 1 小时重启了 12 次"
  environment:
    cluster_version: "v1.28.3"
    node_count: 50
    cni: "Calico 3.26"
    affected_service: "order-service (Deployment, 6 replicas)"
  diagnosis_process:
    - step: 1
      action: "kubectl get pods -n production | grep order"
      finding: "RestartCount 持续增长，当前值 12"
    - step: 2
      action: "kubectl logs order-service-xxx --previous"
      finding: "java.lang.OutOfMemoryError: Java heap space"
    - step: 3
      action: "kubectl describe pod order-service-xxx"
      finding: "Last State: Terminated, Reason: OOMKilled, Exit Code: 137, limits.memory=512Mi"
    - step: 4
      action: "检查应用 JVM 参数"
      finding: "JVM -Xmx 未设置，默认使用容器内存的 1/4 = 128Mi，但应用实际需 400Mi+"
  root_cause: "JVM 默认堆大小超过容器内存 limits，触发 OOMKilled"
  fix:
    immediate: "设置 -Xmx=384m，调整 limits.memory=1Gi"
    permanent: "CI/CD 流水线强制检查 JVM 参数与容器 limits 的匹配"
  lesson_learned:
    rule: "Java 应用必须显式设置 JVM 堆大小 ≤ 容器 memory limits 的 75%"
    category: "resource-management"
  knowledge_refs:
    - 故障诊断/07-oom-memory-diagnosis.md
    - 故障诊断/08-pod-comprehensive-troubleshooting.md#oomkilled
  tags: [java, oom, memory, crashloop, production, jvm]
  resolution_time: "35min"
```

**需覆盖的工单场景分布**：

| 场景类别 | 需要的工单数 | 典型场景 | 来源 |
|---------|-----------|---------|------|
| Pod 异常 | 15 | CrashLoop/OOM/Pending/Evicted/ImagePull | 故障诊断/05,07,08 |
| 网络问题 | 10 | DNS失败/Service不通/Ingress异常/跨节点不通 | 故障诊断/03,10,15,25,26 |
| 存储问题 | 8 | PVC Pending/挂载失败/性能下降 | 故障诊断/04,14 |
| 控制平面 | 8 | apiserver 慢/etcd 不健康/证书过期 | 故障诊断/01,02,13 |
| 安全权限 | 5 | RBAC 拒绝/ServiceAccount 失效 | 故障诊断/12,32 |
| 扩缩容 | 5 | HPA 不触发/CA 不扩容 | 故障诊断/17,28 |
| 升级迁移 | 5 | 版本不兼容/滚动升级卡住 | 故障诊断/34 |
| 工具链 | 5 | Helm 失败/ArgoCD 不同步 | 故障诊断/36,38 |
| 性能问题 | 5 | CPU 瓶颈/IO 延迟/网络延迟 | 故障诊断/33 |
| **合计** | **66** | | |

**工作量估算**：~20 人天（66 个工单 × 每个 0.3 天，可从 故障诊断 现有内容提取结构化）

### 6. 对话式交互语料（Conversational Corpus）

**现状审计**：**完全空白**。所有 1477 篇文档均为单向叙述体，Agent 缺少"如何与人交互"的参考。

**6 类缺失的对话模式详述**：

| 对话类型 | 说明 | Agent 能力影响 | 需要的模板数 |
|---------|------|--------------|----------|
| **多轮排障对话** | Agent 如何逐步追问定位问题 | 排障效率 | 15 |
| **信息不足追问** | 缺少什么信息时如何追问 | 诊断准确性 | 5 |
| **危险操作确认** | 执行破坏性操作前的确认流程 | 安全性 | 5 |
| **解释与教学** | 解释原理的对话模式 | 学习体验 | 5 |
| **操作指导** | 手把手指导操作的对话 | 新手体验 | 5 |
| **方案对比决策** | 多方案对比时如何引导用户决策 | 决策支持 | 5 |

**对话模板示例 —— 多轮排障对话**：

```yaml
conversation_template:
  id: CONV-TS-001
  scenario: "Pod Pending 排障"
  trigger: "用户报告 Pod Pending"
  target_outcome: "定位 Pending 原因并给出修复建议"
  knowledge_refs: [D12-05, D12-24, D33-05]
  dialogue:
    - turn: 1
      user: "Pod 一直卡在 Pending 状态，怎么办？"
      agent_intent: "收集基础信息"
      agent: |
        了解。我需要一些信息来帮你诊断：
        1. 请运行: `kubectl describe pod <pod-name> -n <namespace>`
        2. 请告诉我 Events 部分的输出
        3. 这是生产环境还是测试环境？
      required_info: ["describe 输出", "环境类型"]

    - turn: 2
      user: "生产环境，Events 显示: 0/10 nodes are available: 4 Insufficient cpu, 6 had taint"
      agent_intent: "解析 Events 并缩小范围"
      agent: |
        根据 Events 信息，问题很清晰：
        - **4 个节点 CPU 不足** → 资源瓶颈
        - **6 个节点有污点** → Pod 未配置对应的 tolerations

        让我确认一下：
        1. `kubectl top nodes` → 看节点实际资源使用
        2. `kubectl describe nodes | grep Taints` → 看污点详情
        3. `kubectl get pod <pod> -o yaml | grep -A5 resources` → 看 Pod 资源请求
      reasoning: "结合 D12-05 诊断决策树，资源+污点双重问题"

    - turn: 3
      user: "top nodes 显示 4 个节点 CPU 都在 92%，Taints 显示 6 个节点有 gpu=true:NoSchedule"
      agent_intent: "给出建议"
      agent: |
        明确了！你的集群有两个问题：

        **问题 1: CPU 资源不足（40% 节点已超载）**
        - 短期方案: 检查是否有可回收的低优先级 Pod
        - 中期方案: 扩容普通节点
        ```bash
        kubectl get pods -A --sort-by='.spec.containers[0].resources.requests.cpu' | tail -20
        ```

        **问题 2: 6 个 GPU 节点有污点，普通 Pod 无法调度**
        - 如果这个 Pod 不需要 GPU，这是正确行为
        - 如果需要 GPU，添加 toleration:
        ```yaml
        tolerations:
        - key: "gpu"
          operator: "Equal"
          value: "true"
          effect: "NoSchedule"
        ```

        你想先解决哪个问题？
```

**工作量估算**：~10 人天（40 个对话模板 × 0.25 天）

---

## 🟢 P2 级缺失：Agent 精度和安全依赖（推理层 Layer 2）

> P2 缺失影响 Agent 的决策精度和安全边界，在生产环境中尤为关键。

### 7. 版本差异矩阵（Version Compatibility Matrix）

**现状审计**：文档头部标注了适用版本范围（如 v1.25-v1.32），但缺少**版本间 API 和行为差异的精确映射**。

现有版本相关内容分布：

| 文档 | 版本相关内容 | Agent 可用度 |
|------|-----------|----------|
| `集群基础/03-api-versions-features.md` | API 版本演进 | ★★★☆☆ 有但不够精确 |
| `集群基础/07-upgrade-paths-strategy.md` | 升级策略 | ★★☆☆☆ 偏策略 |
| `故障诊断/05` 第 13 节 | Pod Pending 版本特定变更 | ★★★☆☆ 局部覆盖 |
| `清单模式/` | YAML 示例中的 apiVersion | ★★☆☆☆ 隐含但未整理 |

**Agent 所需的目标格式**：

```yaml
version_diff_matrix:
  - feature: "Pod Security Standards"
    changes:
      - version: "1.25"
        status: "GA"
        breaking: true

        note: "PodSecurityPolicy 移除，PSS 成为默认"
        migration: "从 PSP 迁移到 PSS"
        ref: "安全/06-pod-security-standards.md"
      - version: "1.28"
        note: "新增 AppArmor 字段"
      - version: "1.30"
        note: "UserNamespace 支持 beta"

  - feature: "Gateway API"
    changes:
      - version: "1.26"
        status: "beta"
        api: "gateway.networking.k8s.io/v1beta1"
      - version: "1.29"
        status: "GA"
        api: "gateway.networking.k8s.io/v1"
        breaking: true
        note: "v1beta1 部分字段废弃"
        ref: "网络/35-gateway-api-overview.md"

  - feature: "Sidecar Containers"
    changes:
      - version: "1.28"
        status: "alpha"
        note: "restartPolicy: Always for init containers"
      - version: "1.29"
        status: "beta"
        note: "默认启用 SidecarContainers feature gate"
        ref: "工作负载/14-sidecar-containers-patterns.md"
```

**需覆盖的版本差异类别**：

| 类别 | 估计条目数 | 优先级 | 现有可复用 |
|------|-----------|--------|----------|
| API 废弃/移除时间线 | ~15 | 🔴 高 | 集群基础/03 可提取 |
| 默认行为变更 | ~20 | 🔴 高 | 零散 |
| Feature Gate 状态 | ~15 | 🟡 中 | K8s 官方文档 |
| 组件参数变更 | ~10 | 🟡 中 | 集群基础 可提取 |
| **合计** | **~60** | | |

**工作量估算**：~8 人天

### 8. 判断决策条件（Decision Criteria）

**现状审计**：文档给出了多种方案选择，但缺少**量化的决策条件**供 Agent 自动判断。

| 现有文档 | 决策内容 | 缺失 |
|---------|---------|------|
| `网络/03-cni-plugins-comparison.md` | CNI 插件对比 | 缺量化选择条件 |
| `存储/04-storageclass-dynamic-provisioning.md` | 存储类选择 | 缺性能/成本判断指标 |
| `工作负载/22-cluster-capacity-planning.md` | 容量规划 | 缺自动化计算公式 |
| `集群基础/12-cluster-deployment-patterns.md` | 部署模式 | 缺场景→模式映射 |

**Agent 所需的目标格式**：

```yaml
decision_rules:
  - id: DR-001
    decision: "选择 CNI 插件"
    conditions:
      - if: "节点数 > 500 AND 需要 NetworkPolicy"
        then: "Cilium (eBPF 模式)"
        reason: "大规模场景下 kube-proxy 性能瓶颈，Cilium eBPF 无需 iptables"
        ref: "网络/03-cni-plugins-comparison.md"
      - if: "阿里云 ACK 环境"
        then: "Terway"
        reason: "原生 ENI 集成，性能最优，支持 VPC 原生网络"
        ref: "网络/05-terway-advanced-guide.md"
      - if: "节点数 < 100 AND 无复杂网络需求"
        then: "Flannel (VXLAN)"
        reason: "简单稳定，运维成本低"
      - default: "Calico"
        reason: "广泛使用，功能全面，社区支持好"

  - id: DR-002
    decision: "Pod 内存 limits 设置"
    conditions:
      - if: "Java 应用"
        formula: "limits.memory = JVM_Xmx * 1.3 + 200Mi (non-heap + metaspace)"
        example: "Xmx=512m → limits=896Mi ≈ 1Gi"
        warning: "必须显式设置 -Xmx，否则 JVM 可能用尽容器内存"
        ref: "故障诊断/07-oom-memory-diagnosis.md"
      - if: "Go 应用"
        formula: "limits.memory = 观测 P99 内存 * 1.5"
        note: "Go GC 会在 GOMEMLIMIT 附近触发，建议设置 GOMEMLIMIT"
      - if: "Node.js 应用"
        formula: "limits.memory = --max-old-space-size * 1.4 + 100Mi"
      - if: "Python 应用"
        formula: "limits.memory = 观测峰值 * 2.0"
        note: "Python 内存管理较不可预测，留更大余量"

  - id: DR-003
    decision: "选择数据备份方案"
    conditions:
      - if: "K8s 资源备份（无状态）"
        then: "Velero"
      - if: "etcd 数据备份"
        then: "etcdctl snapshot"
      - if: "有状态服务数据"
        then: "应用原生备份 + Velero 编排"
        ref: "平台工程/12-backup-recovery-strategy.md"
```

**需覆盖的决策场景**：

| 决策场景 | 估计规则数 | 来源 |
|---------|-----------|------|
| CNI/网络插件选择 | 5 | 网络/03 |
| 存储类/CSI 驱动选择 | 5 | 存储/04-05 |
| 资源 requests/limits 设置 | 5 | 工作负载/23, 故障诊断/07 |
| 集群规模/架构选型 | 5 | 集群基础/12 |
| 监控方案选择 | 3 | 可观测性/01 |
| Ingress/Gateway 选型 | 5 | 网络/19,35 |
| 备份策略选择 | 3 | 平台工程/12 |
| 安全策略选型 | 5 | 安全 |
| 扩缩容策略选择 | 3 | 工作负载/21 |
| **合计** | **~39** | |

**工作量估算**：~8 人天

### 9. 安全护栏规则（Safety Guardrails）

**现状审计**：`安全/` 和 `安全/` 有安全知识，但缺少 **Agent 执行前的安全检查规则集**。这是 Agent 进入生产环境的**必要条件**。

**Agent 所需的安全护栏体系**：

```yaml
safety_guardrails:
  # === 等级 1: 绝对禁止（Agent 永远不应该执行） ===
  forbidden_actions:
    - pattern: "kubectl delete namespace (kube-system|kube-public|kube-node-lease)"
      reason: "删除系统命名空间将导致集群不可用"
      severity: catastrophic
    - pattern: "kubectl delete node"
      reason: "直接删除节点可能导致数据丢失"
    - pattern: "etcdctl del .* --prefix"
      reason: "清空 etcd 将销毁整个集群"
    - pattern: "kubectl delete pv"
      reason: "删除 PV 可能导致数据永久丢失"
    - pattern: "kubectl.*--force --grace-period=0.*-n (production|prod)"
      reason: "强制删除生产 Pod 可能中断服务"
    - pattern: "kubectl apply.*--server-side --force-conflicts"
      reason: "强制服务端应用可能覆盖关键配置"

  # === 等级 2: 需二次确认（Agent 必须显示警告并等待用户确认） ===
  confirm_required:
    - pattern: "kubectl delete pod .* -n (production|prod|prd)"
      warning: "↠️ 即将删除生产环境 Pod，请确认影响范围"
      show_info: ["kubectl get pod <pod> -o wide", "kubectl get endpoints"]
    - pattern: "kubectl drain"
      warning: "↠️ 节点排水将迁移所有工作负载"
      pre_check: ["kubectl get pods -o wide --field-selector spec.nodeName=<node>"]
    - pattern: "kubectl scale.*replicas=0"
      warning: "↠️ 将副本数设为 0 会停止所有实例"
    - pattern: "kubectl edit.*-n (production|prod)"
      warning: "↠️ 直接编辑生产资源，建议通过 GitOps 变更"
    - pattern: "kubectl rollout undo"
      warning: "↠️ 即将回滚 Deployment，请确认目标版本"
    - pattern: "helm uninstall|helm delete"
      warning: "↠️ 卸载 Helm Release 将删除所有相关资源"

  # === 等级 3: 时间窗口约束 ===
  time_constraints:
    - rule: "不在工作时间 (9:00-18:00 工作日) 执行集群升级"
      reason: "需要充足的回滚窗口"
    - rule: "大促期间（双11、61818）冻结变更"
      reason: "业务高峰期最小化风险"

  # === 等级 4: 前置检查 ===
  pre_checks:
    - before: "删除 PVC"
      checks:
        - "确认无 Pod 挂载: kubectl get pods --all-namespaces -o json | jq '.items[] | select(.spec.volumes[]?.persistentVolumeClaim.claimName==\"<pvc>\")'"
        - "确认数据已备份"
    - before: "修改 RBAC"
      checks:
        - "检查受影响的 ServiceAccount: kubectl get rolebinding,clusterrolebinding -A -o json | jq '.items[] | select(.roleRef.name==\"<role>\")'"
    - before: "删除 Namespace"
      checks:
        - "确认 namespace 内无运行中的工作负载"
        - "确认无绑定的 PVC/PV"
```

**工作量估算**：~5 人天

---

## ⚪ P3 级缺失：Agent 生态拓展

### 10. 多云适配语料

**现状**：`云厂商/` 中阿里云 ACK 有 8 个文件覆盖较好，但 AWS EKS、GKE、AKS 各只有 1 个文件。

**缺失**：
- 各云厂商的 K8s 服务差异对比表
- 云厂商特有的 annotation/label 映射
- 跨云部署的配置转换规则

---

## 补全路线图

```
# 🟢 低风险：只读/信息收集，通常无副作用
Phase 1 (4-6 周) - 知识层基础设施                          总工作量: ~30 人天
├── 为全量文档添加 YAML front matter 元数据         ~15 人天
│   ├── 脚本自动生成基础字段 (id, domain, title, tags)   ~3 天
│   ├── 人工补全 intent_queries 和关联关系          ~7 天
│   └── 核心 100 篇文档 chunk_markers 标注          ~5 天
└── 提取症状→原因映射表 (73 条)                    ~15 人天
    ├── 从 FTA 故障树自动提取                         ~3 天
    ├── 从 故障诊断 人工提取补充                      ~5 天
    ├── 从 系统基础 事件文档关联                       ~2 天
    └── 编写诊断命令和 indicators                      ~5 天

Phase 2 (4-6 周) - SOP 和命令语料                          总工作量: ~57 人天
├── 编写 30 个核心运维 SOP（结构化可执行格式）       ~25 人天
│   ├── P0 级 SOP (6 个)                                ~6 天
│   ├── P1 级 SOP (9 个)                                ~9 天
│   └── P2/P3 级 SOP (15 个)                            ~10 天
├── 补全 Top 20 高频 kubectl 命令输出解读            ~12 人天
└── 收集/编写 66+ 真实问题工单案例                  ~20 人天

Phase 3 (3-4 周) - 交互和安全层                          总工作量: ~23 人天
├── 编写 40 个高频场景对话模板                      ~10 人天
├── 建立安全护栏规则集                               ~5 人天
└── 补全版本差异矩阵 (v1.25-v1.32, ~60 条)          ~8 人天

Phase 4 (2-3 周) - 决策和验证层                          总工作量: ~16 人天
├── 提取量化决策条件 (~39 条)                        ~8 人天
├── 建立 Agent 评测基准 (Benchmark)                   ~5 人天
└── 补全多云适配语料                                  ~3 人天

总计: ~126 人天 ≈ 1 人 6 个月 / 2 人 3 个月 / 3 人 2 个月
```
---

## 现有资产复用矩阵

| 现有资产 | 文件数 | 可复用于 | 改造工作量 |
|---------|-------|---------|-----------|
| `故障诊断/` | 42 | 症状→原因映射表、SOP | 中 - 需结构化提取 |
| `故障诊断/高级排障/structural-` | 48+ | 决策树、症状映射 | 低 - 已半结构化 |
| `故障诊断/FTA故障树/list/` | 37 | 故障树推理链 | 低 - 已结构化 |
| `清单模式/` | 36 | YAML 生成模板 | 低 - 已标准化 |
| `系统基础/` | 15 | 事件解读语料 | 中 - 需提取映射 |
| `系统基础/topic-dictionary/` | 16 | 术语表、最佳实践 | 中 - 需分块 |
| `系统基础/topic-cheat-sheet/` | 3 | 命令速查 | 低 - 可直接用 |
| `发布变更/topic-migration/` | 10 | 迁移 SOP | 中 - 需 SOP 化 |
| `故障诊断/FEBM方法论/` | 11 | 诊断方法论 | 中 - 需模板化 |

---

## 定量差距估算

| 维度 | 当前数量 | Agent 所需 | 差距 | 估算工作量 | 优先级 |
|------|---------|----------|------|-----------|--------|
| 结构化元数据覆盖文档数 | 0 | 1477 | **1477 篇** | ~15 人天 | P0 |
| 可执行 SOP 数 | ~5 (零散) | 30 | **~25 个** | ~25 人天 | P0 |
| 症状→原因映射条目 | 0 (散落在文档) | 73+ | **~73 条** | ~15 人天 | P0 |
| 命令输出解读模板 | ~20 (零散) | 50+ | **~30 个** | ~12 人天 | P1 |
| 问题工单案例 | ~3 | 66+ | **~63 个** | ~20 人天 | P1 |
| 对话模板 | 0 | 40+ | **40+ 组** | ~10 人天 | P1 |
| 版本差异矩阵条目 | ~10 (零散) | 60+ | **~50 条** | ~8 人天 | P2 |
| 量化决策条件 | ~15 (散落) | 39+ | **~24 条** | ~8 人天 | P2 |
| 安全护栏规则 | 0 | 30+ | **30+ 条** | ~5 人天 | P2 |
| **合计** | | | | **~126 人天** | |

---

## 结论

kudig-database 作为**人类阅读的知识库**已经非常完善（★★★★★），但作为 **Agent 可用的语料库**（★★★☆☆）还存在结构性差距。核心问题不是内容缺失，而是：

> **内容丰富但结构不够 Agent 友好** —— 需要从"人读长文"转化为"机器可检索、可推理、可执行"的结构化语料。

**关键洞察**：

| 维度 | 现状 | 目标 |
|------|------|------|
| 知识层 (Layer 1) | 内容丰富但无元数据 | 结构化、可检索、有关联 |
| 推理层 (Layer 2) | 有知识但缺决策规则 | 量化条件、安全护栏、版本矩阵 |
| 交互层 (Layer 3) | 完全空白 | 对话模板、工单语料、操作确认 |

**好消息**：**~80% 的差距可以通过对现有内容的结构化改造来弥补**，真正需要新增的内容仅占 20%（主要是对话语料、工单案例和安全护栏）。

**总投入估算：~126 人天**，可以分 4 个阶段 16 周完成。建议从 Phase 1（结构化元数据 + 症状映射）开始，这是 Agent 能力的基础。

---

## 关联文档

| 文档 | 说明 |
|------|------|
| [Agent 设计思路与落地路径](./14-agent-kudig-design-strategy.md) | Agent 赋能的整体设计思路 |
| [故障诊断/FTA故障树/09-fta-as-agent-knowledge-skeleton.md](../故障诊断/FTA故障树/09-fta-as-agent-knowledge-skeleton.md) | FTA 作为 Agent 知识骨架 |
| [故障诊断/FTA故障树/10-agent-orchestration-patterns.md](../故障诊断/FTA故障树/10-agent-orchestration-patterns.md) | Agent 编排模式 |
| [故障诊断/FEBM方法论/04-febm-agent-ticket-processing.md](../故障诊断/FEBM方法论/04-febm-agent-ticket-processing.md) | FEBM Agent 工单处理 |

---

*本文档为 kudig-database 项目 02-ai-agents 专题的差距分析报告，原 topic-agent 专题已整合至此。*

---

## Obsidian 相关文档

- 02-ai-agents MOC
- [[AI基础设施/AI-Agents/README.md|AI Agent 工程专题]]
- [[AI基础设施/AI-Agents/01-ai-agent-fundamentals.md|AI Agent 基础与核心架构]]
- [[AI基础设施/AI-Agents/02-llm-foundation-models.md|LLM 基座模型选型与评估]]
- [[AI基础设施/AI-Agents/03-agent-frameworks-comparison.md|主流 Agent 框架深度对比]]
- [[AI基础设施/AI-Agents/04-rag-knowledge-retrieval.md|RAG 检索增强生成深度指南]]
- [[AI基础设施/AI-Agents/05-tool-use-function-calling.md|Tool Use & Function Calling 设计规范]]
- [[AI基础设施/AI-Agents/06-multi-agent-orchestration.md|多 Agent 编排与协作架构]]
- [[AI基础设施/AI-Agents/07-memory-context-management.md|记忆管理与上下文窗口工程]]
- [[AI基础设施/AI-Agents/08-agent-evaluation-observability.md|Agent 评测体系与可观测性]]
- [[AI基础设施/AI-Agents/09-production-deployment-guide.md|生产部署指南：K8s 上运行 Agent 服务]]
- [[AI基础设施/AI-Agents/10-security-guardrails.md|安全护栏、提示注入防护与合规]]

## Related

- 13-trusted-agent-system-fiscal-plan
- 39-agent-harness-testing-benchmark
- 42-model-harness-compatibility-matrix
- 12-enterprise-case-studies
- 02-llm-foundation-models
- 23-agent-cli-fundamentals
- 50-openclaw-identity-mechanism
- 01-ai-agent-fundamentals
- 03-agent-frameworks-comparison
- 47-openclaw-tools-mechanism
- 37-agent-harness-multi-agent
- 20-agentscope-multi-agent-orchestration
- 25-agent-cli-mcp-integration
- 26-agent-cli-development-workflow
- 07-memory-context-management
- 11-cost-latency-optimization
- 44-openclaw-soul-mechanism
- 45-openclaw-user-mechanism
- 31-agent-harness-loop-execution
- 06-multi-agent-orchestration

## See Also

- 13-trusted-agent-system-fiscal-plan
- 14-agent-kudig-design-strategy
- 16-agentscope-overview-installation
- 17-agentscope-core-concepts


<!-- risk-assessed -->
