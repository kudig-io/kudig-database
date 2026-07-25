---
title: 发布会演示脚本 (完整版)
description: '"今天我们要发布的是 kudig-database, 一个专门为 AI 智能体打造的 Kubernetes 生产运维知识库。'
summary: '"今天我们要发布的是 kudig-database, 一个专门为 AI 智能体打造的 Kubernetes 生产运维知识库。'
category: general
tags:
- k8s
- etcd
- networkpolicy
- rag
- agent
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 发布会演示脚本 (完整版) 是什么
- 如何 发布会演示脚本 (完整版)
trigger_keywords:
- 发布会演示脚本
- 完整版
prerequisites:
- kubectl-basics
- etcd-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 发布会演示脚本 (完整版)

> **总时长**: 15 分钟
> **演示环境**: 接入 kudig-database RAG 的 AI Agent

---

## 开场 (1 分钟)

**主持人**:

"今天我们要发布的是 kudig-database, 一个专门为 AI 智能体打造的 [[23-实体/02-K8s核心组件/kubernetes.md|kubernetes]] 生产运维知识库。

它不是一份文档, 不是一个 Wiki, 而是 3,346 篇经过结构化处理的专家级知识, 可以直接被智能体检索和使用。

接下来我用 4 个真实场景, 演示它能做什么。"

---

## 场景 1: 深度研究 (3 分钟)

**主持人**: "假设你刚接手 K8s 集群的 etcd 运维, 老板让你一周内建立系统性认知, 你会怎么做?"

**操作**: 向 Agent 提问

```
我要深度研究 etcd 的生产运维, 包括架构原理、Raft 共识、备份恢复和性能调优
```

**Agent 输出要点** (主持人边展示边讲解):

1. "Agent 命中了 集群基础 的 etcd 深度文档, 1,042 行"
2. "它给出了 Raft 共识协议的完整解析, 包括 Leader 选举和日志复制"
3. "MVCC 数据模型, 解释了多版本并发控制的原理"
4. "备份恢复的完整操作流程, 含 etcdctl 命令"
5. "性能调优参数表, 磁盘/网络/CPU 的推荐配置"

**主持人**: "这相当于一个资深 SRE 花 3 天整理的知识体系, Agent 30 秒就输出了。"

---

## 场景 2: 问题排查 (3 分钟)

**主持人**: "线上出问题了, Pod 一直在重启, 你怎么快速定位?"

**操作**: 向 Agent 提问

```
线上 Pod CrashLoopBackOff, RESTARTS 一直在涨, 帮我排查
```

**Agent 输出要点**:

1. "Agent 调用了 diagnose-pod-crashloop.sh 诊断脚本"
2. "输出了上次崩溃日志, 显示 Connection refused"
3. "检查了 OOMKilled 状态, 排除内存问题"
4. "分析了 livenessProbe 配置, 发现超时设置过短"
5. "给出了修复建议: 增大 initialDelaySeconds 或优化应用启动速度"

**主持人**: "从发现问题到定位根因, 不到 1 分钟。而且 Agent 给出的是可执行的命令, 不是泛泛的建议。"

---

## 场景 3: 架构设计 (4 分钟)

**主持人**: "老板让你设计一套金融支付系统的 K8s 架构, 要满足 PCI-DSS 合规要求, 你怎么交出一份专业方案?"

**操作**: 向 Agent 提问

```
帮我设计一套金融支付系统的 Kubernetes 生产架构, 要满足 PCI-DSS 合规要求
```

**Agent 输出要点**:

1. "Agent 命中了 应用模式/topic-application-architecture/06-fintech-architecture.md"
2. "给出了完整的微服务拆分: 支付网关/风控引擎/账务系统/清算系统"
3. "安全架构: HSM 密钥管理 + KMS + TDE 加密 + 审计日志"
4. "多可用区部署方案, 含 Mermaid 架构图"
5. "多云对照表: 阿里云/ AWS / GCP / Azure 的对应服务映射"
6. "完整的 Deployment + NetworkPolicy YAML 配置"

**主持人**: "这份方案如果人工编写, 至少需要一个架构师花一周。Agent 给出的是可以直接拿去做技术评审的水准。"

---

## 场景 4: 命令解读 (2 分钟)

**主持人**: "最后一个场景, 看到一个报错, 不确定什么意思, 直接问 Agent。"

**操作**: 向 Agent 提问

```
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl describe pod 显示 OOMKilled, Exit Code 137, 这是什么意思? 怎么处理?
```
**Agent 输出要点**:

1. "Exit Code 137 = 128 + 9, 即被 SIGKILL 信号终止"
2. "OOMKilled 表示容器内存使用超过 limits.memory 限制, 被内核 OOM Killer 强制终止"
3. "检查命令: kubectl top pod 查看实际内存使用"
4. "修复方案: 增大 resources.limits.memory 或排查应用内存泄漏"
5. "预防措施: 设置合理的 requests 保证 QoS 级别"

**主持人**: "对于新手来说, 一个 Exit Code 137 可能要查半天。Agent 直接告诉你 137 = 128 + 9, 这就是专家级知识的价值。"

---

## 收尾 (2 分钟)

**主持人**:

"刚才演示的 4 个场景, 覆盖了 kudig-database 的核心能力:

| 场景 | 命中知识 | 价值 |
|------|---------|------|
| 深度研究 | 1,042 行 etcd 深度文档 | 3 天 → 30 秒 |
| 问题排查 | 18 个可执行 SOP + 17 个脚本 | 结构化诊断 |
| 架构设计 | 97 个行业场景 | 1 周 → 5 分钟 |
| 命令解读 | 23 种诊断模式 | 精准解读 |

这背后是 3,346 篇文档、40 个知识域、218 个 CNCF 项目的结构化知识。

kudig-database 的目标很简单: **让每一个 K8s 问题都有答案, 让每一个智能体都拥有专家级知识。**

感谢大家。"


<!-- risk-assessed -->
