---
title: 第二章：FTA 数学基础与理论模型 [domain-10-troubleshooting-diagnostics]
description: 'description: ''**关联主文档**: [FTA 方法论与 AI Agent 智能运维实践](./fta-methodology-and-agentic-practices.md)'''
category: fta
tags:
- fta
- troubleshooting
- etcd
- coredns
- pdb
- ingress
- agent
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 技术支持
estimated_read_time: 15min
intent_queries:
- 第二章：FTA 数学基础与理论模型 是什么
- 如何 第二章：FTA 数学基础与理论模型
- Kubernetes 10 troubleshooting diagnostics 最佳实践
- 第二章：FTA 数学基础与理论模型 故障排查
- 第二章：FTA 数学基础与理论模型 排障步骤
- 第二章：FTA 数学基础与理论模型 根因分析
trigger_keywords:
- 第二章：FTA
- 数学基础与理论模型
- troubleshooting
- diagnostics
- fta
prerequisites:
- kubectl-basics
- troubleshooting-methodology
- etcd-basics
fta_id: FTA-02_MATHEMATICAL_FOUNDATIONS-001
component: 02 Mathematical Foundations
severity: critical
created: "2026-05-23"
---

title: 第二章：FTA 数学基础与理论模型
description: '**关联主文档**: [FTA 方法论与 AI Agent 智能运维实践](./fta-methodology-and-agentic-practices.md)'
category: fta
tags:
- k8s
- fault-tree
- root-cause
- troubleshooting
- [[etcd|etcd]]
- [[CoreDNS|coredns]]
- pdb
- [[Ingress|ingress]]
- agent
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 技术支持
estimated_read_time: 5min
intent_queries:
- 第二章：FTA 数学基础与理论模型 是什么
- 如何 第二章：FTA 数学基础与理论模型
- 第二章：FTA 数学基础与理论模型 根因分析
- 第二章：FTA 数学基础与理论模型 故障树
trigger_keywords:
- 第二章：FTA
- 数学基础与理论模型
- fta
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
# 第二章：FTA 数学基础与理论模型

> **所属部分**: 第一部分 - FTA 方法论理论基础  
> **关联主文档**: [FTA 方法论与 AI Agent 智能运维实践](./fta-methodology-and-agentic-practices.md)  
> **上一章**: [第一章：FTA 起源与发展史](./[[domain-10-troubleshooting-diagnostics/topic-fta/01-fta-origin-and-evolution.md|01-fta-origin-and-evolution]].md)  
> **下一章**: [第三章：FTA 符号体系与标准规范](./03-fta-symbol-system-and-standards.md)

---

## 2.1 布尔代数基础

FTA 的核心数学工具是**布尔代数**。故障树中的每个事件可被视为一个布尔变量（发生 = 1，未发生 = 0），逻辑门则对应布尔运算。

**基本逻辑门的布尔表达**：

| 逻辑门 | 布尔表达式 | 含义 | 故障树语义 |
|--------|-----------|------|-----------|
| **OR 门** | Q = A + B = A ∨ B | 任一输入事件发生，输出事件即发生 | 多个独立原因中的**任意一个**即可导致问题 |
| **AND 门** | Q = A · B = A ∧ B | 所有输入事件同时发生，输出事件才发生 | 需要**所有条件同时满足**才会导致问题 |
| **NOT 门** | Q = Ā = ¬A | 输入事件未发生时，输出事件发生 | 问题由正常条件的**缺失**导致 |
| **k/n 投票门** | Q = C(n,k) 组合 | n 个输入中至少 k 个发生 | 如"3 节点集群中至少 2 个节点问题" |
| **异或门（XOR）** | Q = A ⊕ B | 恰好一个输入发生 | 互斥故障模式 |
| **优先 AND 门** | Q = A · B (A 先于 B) | 按时序同时发生 | 有时间依赖关系的问题组合 |

**布尔代数化简公式**（用于优化故障树结构）：

```
幂等律:      A + A = A          A · A = A
互补律:      A + Ā = 1          A · Ā = 0
吸收律:      A + A·B = A        A · (A+B) = A
德摩根律:    ¬(A+B) = Ā·B̄       ¬(A·B) = Ā+B̄
分配律:      A·(B+C) = A·B+A·C  A+(B·C) = (A+B)·(A+C)
```

**实际应用示例**：

对于 Kubernetes 集群中 "API Server 不可用" 的故障树：

```
TE = BE₁ + BE₂ + (BE₃ · BE₄)

其中:
  TE  = API Server 不可用（顶事件）
  BE₁ = API Server 进程崩溃（OR 分支）
  BE₂ = etcd 集群不可用（OR 分支）
  BE₃ = 证书过期（AND 分支条件 1）
  BE₄ = 自动续期机制失效（AND 分支条件 2）

含义: API Server 不可用 = 进程崩溃 OR etcd 问题 OR (证书过期 AND 自动续期失效)
```

## 2.2 概率论在 FTA 中的应用

当每个底事件被赋予问题概率后，可以通过布尔代数计算顶事件的发生概率。

**基本概率公式**：

```
OR 门 (独立事件):
  P(Q) = 1 - ∏(1 - P(Aᵢ))
  
  近似（低概率时）:
  P(Q) ≈ ΣP(Aᵢ)

AND 门 (独立事件):
  P(Q) = ∏P(Aᵢ)

k/n 投票门:
  P(Q) = Σ C(n,i) · P^i · (1-P)^(n-i)   (i = k to n)
```

**Kubernetes 问题概率计算示例**：

```
场景: 计算 "集群完全不可用" 的概率

已知底事件年问题概率:
  P(API Server 崩溃)        = 0.001  (年问题率)
  P(etcd 集群问题)           = 0.0005
  P(网络基础设施问题)        = 0.002
  P(批量节点问题)            = 0.0003
  P(CNI 插件问题)            = 0.001

故障树结构:
  TE-1 [OR门]
  ├── IE-1.1 控制平面问题 [OR门]
  │   ├── BE: API Server 崩溃: P = 0.001
  │   └── BE: etcd 集群问题: P = 0.0005
  ├── IE-1.2 批量节点问题: P = 0.0003
  └── IE-1.3 网络基础设施问题 [OR门]
      ├── BE: 网络设备问题: P = 0.002
      └── BE: CNI 插件问题: P = 0.001

计算过程:
  P(IE-1.1) = 1 - (1-0.001)(1-0.0005) = 1 - 0.999 × 0.9995 ≈ 0.001499
  P(IE-1.3) = 1 - (1-0.002)(1-0.001) = 1 - 0.998 × 0.999 ≈ 0.002998

  P(TE-1) = 1 - (1-0.001499)(1-0.0003)(1-0.002998)
           = 1 - 0.998501 × 0.9997 × 0.997002
           ≈ 0.004789

结论: 集群完全不可用的年概率约为 0.48%
       对应年化可用性约 99.52%（未达到 99.9% SLA 目标）
       
加固建议: 
  - 网络基础设施问题概率最高(0.002998)，是最大风险因素
  - 建议优先加固网络冗余，引入双CNI或网络多平面架构
```

## 2.3 最小割集理论

**最小割集（Minimal Cut Sets, MCS）** 是 FTA 定量分析的核心概念。

**定义**：
- **割集（Cut Set）**：使顶事件发生的底事件组合
- **最小割集**：不含冗余事件的割集，即移除任何一个事件后，该组合不再导致顶事件发生

**最小割集的工程意义**：

| 最小割集阶数 | 含义 | 风险等级 | 运维策略 |
|-------------|------|---------|---------|
| **1 阶**（单点问题） | 单个底事件即可导致顶事件 | 极高 | 必须消除，引入冗余 |
| **2 阶** | 需要 2 个底事件同时发生 | 高 | 确保两个事件不相关联 |
| **3 阶** | 需要 3 个底事件同时发生 | 中 | 常规监控即可 |
| **4 阶及以上** | 需要多个底事件同时发生 | 低 | 接受残余风险 |

**最小割集算法（MOCUS 算法）**：

```
MOCUS (Method of Obtaining Cut Sets) 算法流程:

输入: 故障树的布尔表达式
输出: 所有最小割集

步骤:
1. 将故障树转换为布尔表达式
2. 展开所有 OR 门为行（每条路径独立一行）
3. 展开所有 AND 门为列（同行事件全部发生）
4. 应用吸收律消除冗余割集
5. 输出最小割集列表

示例:
  TE = A + (B · C) + (D · (E + F))
  
  展开:
  = A + B·C + D·E + D·F
  
  最小割集:
  MCS₁ = {A}        → 1 阶（单点问题!）
  MCS₂ = {B, C}     → 2 阶
  MCS₃ = {D, E}     → 2 阶
  MCS₄ = {D, F}     → 2 阶
  
  诊断: 事件 A 是单点问题，必须优先消除
```

**Kubernetes 场景最小割集分析**：

```
TE-2: 应用服务不可用

最小割集分析结果:
┌──────┬───────────────────────────────────┬──────┬──────────────────────┐
│ 编号 │ 最小割集                           │ 阶数 │ 加固建议              │
├──────┼───────────────────────────────────┼──────┼──────────────────────┤
│ MCS1 │ {Ingress Controller 单实例问题}   │  1   │ 部署多副本            │
│ MCS2 │ {所有 Pod 同时 OOMKilled}         │  1   │ 设置合理 Resource     │
│ MCS3 │ {DNS 服务问题}                    │  1   │ CoreDNS 多副本+节点   │
│ MCS4 │ {kube-proxy 问题, iptables 损坏}  │  2   │ 节点级冗余            │
│ MCS5 │ {镜像仓库不可达, 本地缓存过期}    │  2   │ 镜像预拉取策略        │
│ MCS6 │ {PDB 配置错误, 节点驱逐, 滚动更新}│  3   │ PDB 审计策略          │
└──────┴───────────────────────────────────┴──────┴──────────────────────┘

关键发现:
  - 存在 3 个 1 阶最小割集（单点问题），需立即消除
  - Ingress Controller 和 CoreDNS 是最大风险点
  - 建议所有关键组件至少部署 2 副本，跨可用区分布
```

## 2.4 重要度分析

重要度分析用于量化每个底事件对顶事件的影响程度，指导运维资源的优先分配。

**Fussell-Vesely 重要度（FV）**：

```
定义: 包含底事件 i 的所有最小割集的概率之和 / 顶事件概率

        Σ P(MCSⱼ)   (j 为所有包含事件 i 的最小割集)
I_FV = ──────────────────────────────────────────────
                    P(TE)

含义: 底事件 i 对顶事件贡献了多少概率
范围: 0 ≤ I_FV ≤ 1
      I_FV → 1: 该事件是关键风险因素
      I_FV → 0: 该事件影响可忽略
```

**Birnbaum 重要度（结构重要度）**：

```
定义: 底事件 i 从"未发生"变为"发生"时，顶事件概率的增量

I_B(i) = P(TE | Aᵢ = 1) - P(TE | Aᵢ = 0)

含义: 底事件 i 的状态变化对系统可靠性的敏感度
      独立于底事件自身的问题概率
```

**风险优先级数（Risk Priority Number, RPN）**：

```
RPN = 严重度(S) × 发生频率(O) × 可检测性(D)

评分标准 (1-10分):
  严重度(S):    1(无影响) → 10(灾难性)
  发生频率(O):  1(极罕见) → 10(频繁)
  可检测性(D):  1(必定检测到) → 10(无法检测)

RPN 范围: 1 ~ 1000
  RPN > 200: 高风险，必须立即采取措施
  RPN 100-200: 中风险，计划内改进
  RPN < 100: 低风险，可接受
```

**Kubernetes 底事件重要度排序示例**：

| 底事件 | FV 重要度 | Birnbaum | RPN | 综合排名 | 加固优先级 |
|--------|----------|----------|-----|---------|-----------|
| etcd 集群问题 | 0.82 | 0.95 | 720 | 1 | 最高 |
| API Server 崩溃 | 0.78 | 0.92 | 630 | 2 | 最高 |
| CNI 插件问题 | 0.45 | 0.60 | 420 | 3 | 高 |
| CoreDNS 问题 | 0.42 | 0.55 | 350 | 4 | 高 |
| 节点资源耗尽 | 0.30 | 0.40 | 280 | 5 | 高 |
| 镜像拉取失败 | 0.15 | 0.20 | 180 | 6 | 中 |
| 证书过期 | 0.10 | 0.85 | 240 | 7 | 中 |
| 配额超限 | 0.05 | 0.10 | 90 | 8 | 低 |

> **关键洞察**：证书过期的 FV 重要度较低（因为有自动续期），但 Birnbaum 重要度极高（一旦发生则全集群影响），说明应重点保障自动续期机制的可靠性。

## 2.5 可靠性核心指标

FTA 分析的最终目标是量化和提升系统可靠性。以下是核心可靠性指标及其在 Kubernetes 运维中的映射：

| 指标 | 公式 | Kubernetes 语义 | 典型目标 |
|------|------|----------------|---------|
| **可靠度 R(t)** | R(t) = P(T > t) = e^(-λt) | 集群在时间 t 内无问题的概率 | R(8760h) > 0.999 |
| **问题率 λ** | λ = 1/MTBF | 单位时间内发生问题的概率 | λ < 0.001/h |
| **MTBF** | MTBF = ∫₀^∞ R(t) dt | 平均问题间隔时间 | > 720h (30天) |
| **MTTR** | MTTR = 修复时间总和 / 问题次数 | 平均修复时间 | P0 < 15min, P1 < 60min |
| **MTTD** | MTTD = 检测时间总和 / 问题次数 | 平均检测时间 | < 5min |
| **可用性 A** | A = MTBF / (MTBF + MTTR) | 系统正常运行时间比例 | ≥ 99.95% (four nines) |

**可用性等级与允许停机时间**：

```
可用性等级      年停机时间      月停机时间      Kubernetes SLA 参考
─────────────────────────────────────────────────────────────────
99%    (two 9s)   3.65 天        7.31 小时     开发/测试环境
99.9%  (three 9s) 8.77 小时      43.83 分钟    非关键生产服务
99.95% (3.5 9s)   4.38 小时      21.92 分钟    标准生产服务
99.99% (four 9s)  52.60 分钟     4.38 分钟     核心业务服务
99.999%(five 9s)  5.26 分钟      26.30 秒      金融/医疗关键服务
```

---

> **导航**: [<< 上一章 - FTA 起源与发展史](./01-fta-origin-and-evolution.md) | [下一章 - FTA 符号体系与标准规范 >>](./03-fta-symbol-system-and-standards.md)

---

## Obsidian 相关文档

- [[domain-10-troubleshooting-diagnostics/topic-fta/MOC.md|topic-fta MOC]]
- [[domain-10-troubleshooting-diagnostics/topic-fta/README.md|topic-fta: 故障树分析（FTA）方法论与 AI Agent 智能运维实践]]
- [[domain-10-troubleshooting-diagnostics/topic-fta/01-fta-origin-and-evolution.md|第一章：FTA 起源与发展史]]
- [[domain-10-troubleshooting-diagnostics/topic-fta/03-fta-symbol-system-and-standards.md|第三章：FTA 符号体系与标准规范]]
- [[domain-10-troubleshooting-diagnostics/topic-fta/04-fta-core-principles.md|第四章：FTA 方法论核心原则]]
- [[domain-10-troubleshooting-diagnostics/topic-fta/05-fta-construction-process.md|第五章：FTA 构建完整流程]]
- [[domain-10-troubleshooting-diagnostics/topic-fta/06-fta-verification-and-quality.md|第六章：FTA 验证与质量保证]]
- [[domain-10-troubleshooting-diagnostics/topic-fta/07-fta-maintenance-and-evolution.md|第七章：FTA 维护与演进策略]]
- [[domain-10-troubleshooting-diagnostics/topic-fta/08-ai-agent-ops-revolution.md|第八章：AI Agent 时代的运维范式革命]]
- [[domain-10-troubleshooting-diagnostics/topic-fta/09-fta-as-agent-knowledge-skeleton.md|第九章：FTA 作为 AI Agent 的知识骨架]]
- [[domain-10-troubleshooting-diagnostics/topic-fta/10-agent-orchestration-patterns.md|第十章：Agent 编排模式与 FTA 逻辑门映射]]
- [[domain-10-troubleshooting-diagnostics/topic-fta/11-fta-driven-runbook-automation.md|第十一章：FTA 驱动的 Runbook 自动化]]

## See Also

- [[domain-10-troubleshooting-diagnostics/topic-fta/symptom-vector-matcher.md|symptom-vector-matcher]]
- [[domain-10-troubleshooting-diagnostics/topic-fta/01-fta-origin-and-evolution.md|01-fta-origin-and-evolution]]
- [[domain-10-troubleshooting-diagnostics/topic-fta/03-fta-symbol-system-and-standards.md|03-fta-symbol-system-and-standards]]
- [[domain-10-troubleshooting-diagnostics/topic-fta/04-fta-core-principles.md|04-fta-core-principles]]
