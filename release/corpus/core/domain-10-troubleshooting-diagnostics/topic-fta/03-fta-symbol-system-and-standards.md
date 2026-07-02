---
title: 第三章：FTA 符号体系与标准规范 [domain-10-troubleshooting-diagnostics]
description: 'description: ''**关联主文档**: [FTA 方法论与 AI Agent 智能运维实践](./fta-methodology-and-agentic-practices.md)'''
summary: 'description: ''**关联主文档**: [FTA 方法论与 AI Agent 智能运维实践](./fta-methodology-and-agentic-practices.md)'''
category: fta
tags:
- fta
- troubleshooting
- etcd
- apiserver
- coredns
- ingress
- agent
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 技术支持
estimated_read_time: 15min
intent_queries:
- 第三章：FTA 符号体系与标准规范 是什么
- 如何 第三章：FTA 符号体系与标准规范
- Kubernetes 10 troubleshooting diagnostics 最佳实践
- 第三章：FTA 符号体系与标准规范 故障排查
- 第三章：FTA 符号体系与标准规范 排障步骤
- 第三章：FTA 符号体系与标准规范 根因分析
trigger_keywords:
- 第三章：FTA
- 符号体系与标准规范
- troubleshooting
- diagnostics
- fta
prerequisites:
- kubectl-basics
- troubleshooting-methodology
- etcd-basics
fta_id: FTA-03_SYMBOL_SYSTEM_AND_STANDARDS-001
component: 03 Symbol System And Standards
severity: critical
---



title: 第三章：FTA 符号体系与标准规范
description: '**关联主文档**: [FTA 方法论与 AI Agent 智能运维实践](./fta-methodology-and-agentic-practices.md)'
category: fta
tags:
- k8s
- fault-tree
- root-cause
- troubleshooting
- [[etcd|etcd]]
- apiserver
- [[CoreDNS|coredns]]
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
- 第三章：FTA 符号体系与标准规范 是什么
- 如何 第三章：FTA 符号体系与标准规范
- 第三章：FTA 符号体系与标准规范 根因分析
- 第三章：FTA 符号体系与标准规范 故障树
trigger_keywords:
- 第三章：FTA
- 符号体系与标准规范
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

# 第三章：FTA 符号体系与标准规范

> **所属部分**: 第一部分 - FTA 方法论理论基础  
> **关联主文档**: [FTA 方法论与 AI Agent 智能运维实践](./fta-methodology-and-agentic-practices.md)  
> **上一章**: [第二章：FTA 数学基础与理论模型](./02-fta-mathematical-foundations.md)  
> **下一章**: [第四章：FTA 方法论核心原则](./[[domain-10-troubleshooting-diagnostics/topic-fta/04-fta-core-principles.md|04-fta-core-principles]].md)

---

## 3.1 标准符号定义

FTA 使用一套标准化的图形符号来表示不同类型的事件和逻辑关系。以下是 IEC 61025 标准定义的核心符号：

**事件符号**：

```
┌───────────────────────────────────────────────────────────────────────┐
│                        FTA 事件符号体系                                │
├───────────────────────────────────────────────────────────────────────┤
│                                                                       │
│  1. 顶事件 / 中间事件 (Top Event / Intermediate Event)               │
│     ┌─────────────────┐                                              │
│     │  系统级问题描述   │    矩形: 可以进一步分解的事件                │
│     │  (可分解事件)     │    顶事件位于树的最顶端                      │
│     └─────────────────┘    中间事件位于中间层                         │
│                                                                       │
│  2. 底事件 / 基本事件 (Basic Event)                                   │
│       ╭─────╮                                                        │
│       │ BE  │              圆形: 不可再分解的基本问题                  │
│       ╰─────╯              需要有明确的问题概率数据                    │
│                             对应具体的可观测/可检测状态                │
│                                                                       │
│  3. 未展开事件 (Undeveloped Event)                                    │
│       ╱─────╲                                                        │
│      ╱  UE   ╲             菱形: 暂未分解到底(信息不足或超出范围)     │
│      ╲       ╱             后续可以进一步展开                         │
│       ╲─────╱                                                        │
│                                                                       │
│  4. 外部事件 / 房屋事件 (House Event)                                 │
│       ┌─────┐                                                        │
│       │ HE  │              梯形/房屋形: 正常预期会发生的事件          │
│      ╱       ╲             如: 系统重启、定期维护                     │
│     ╱─────────╲                                                      │
│                                                                       │
│  5. 条件事件 (Conditioning Event)                                     │
│       ╭─────────╮                                                    │
│       │ 触发条件 │          椭圆形: 逻辑门的附加条件                  │
│       ╰─────────╯          如: "持续超过5分钟"                        │
│                                                                       │
│  6. 转移符号 (Transfer)                                               │
│       △ (转出)  ▽ (转入)    三角形: 故障树分页时的连接标记            │
│                                                                       │
└───────────────────────────────────────────────────────────────────────┘
```

**逻辑门符号**：

```
┌───────────────────────────────────────────────────────────────────────┐
│                        FTA 逻辑门符号体系                              │
├───────────────────────────────────────────────────────────────────────┤
│                                                                       │
│  1. OR 门 (或门)                                                      │
│        ╭──────╮                                                      │
│       ╱   OR   ╲     任一输入发生 → 输出发生                         │
│      ╱──────────╲    P(out) = 1 - ∏(1-P(inᵢ))                       │
│     ╱            ╲                                                   │
│                                                                       │
│  2. AND 门 (与门)                                                     │
│      ┌──────────┐                                                    │
│      │   AND    │     全部输入发生 → 输出发生                         │
│      ╰──────────╯     P(out) = ∏P(inᵢ)                               │
│                                                                       │
│  3. k/n 投票门 (Voting Gate)                                          │
│        ╭──────╮                                                      │
│       ╱  k/n   ╲     n 个输入中至少 k 个发生 → 输出发生              │
│      ╱──────────╲    如: 2/3 表示 3 个中至少 2 个                    │
│                                                                       │
│  4. 抑制门 (Inhibit Gate)                                             │
│      ┌──────────┐                                                    │
│      │ INHIBIT  │     输入事件发生 AND 条件事件成立 → 输出发生        │
│      │   ◇      │     ◇ 表示附加条件                                 │
│      └──────────┘                                                    │
│                                                                       │
│  5. 优先 AND 门 (Priority AND)                                        │
│      ┌──────────┐                                                    │
│      │  PAND    │     所有输入按特定顺序发生 → 输出发生               │
│      │   →      │     考虑时间先后顺序                                │
│      └──────────┘                                                    │
│                                                                       │
│  6. 异或门 (XOR)                                                      │
│        ╭──────╮                                                      │
│       ╱  XOR   ╲     恰好一个输入发生 → 输出发生                     │
│      ╱──────────╲    互斥事件                                        │
│                                                                       │
└───────────────────────────────────────────────────────────────────────┘
```

## 3.2 事件编号与命名规范

在企业级 FTA 实践中，统一的编号和命名规范是团队协作和工具化的基础。推荐以下规范（与本知识库 [kubernetes-fta-full-analysis.md](./kubernetes-fta-full-analysis.md) 一致）：

**编号体系**：

```
┌────────────────────────────────────────────────────────────────────┐
│                     事件编号规范                                     │
├────────────────────────────────────────────────────────────────────┤
│                                                                    │
│  顶事件:   TE-{序号}                                               │
│            例: TE-1 (集群完全不可用)                                │
│            例: TE-2 (应用服务不可用)                                │
│                                                                    │
│  中间事件: IE-{顶事件序号}.{中间事件序号}                           │
│            例: IE-1.1 (控制平面问题)                                │
│            例: IE-2.3 (Ingress访问异常)                             │
│                                                                    │
│  底事件:   BE-{顶事件序号}.{底事件序号}                             │
│            例: BE-1.1 (API Server问题)                              │
│            例: BE-2.10 (负载均衡器问题)                             │
│                                                                    │
│  修复动作: HA-{关联底事件编号}.{动作序号}                           │
│            例: HA-1.1.1 (重启API Server)                            │
│            例: HA-1.1.2 (恢复etcd数据)                              │
│                                                                    │
│  监控指标: MT-{关联底事件编号}                                      │
│            例: MT-1.1 (apiserver_request_duration_seconds)          │
│                                                                    │
└────────────────────────────────────────────────────────────────────┘
```

**命名规范**：

| 规则 | 正确示例 | 错误示例 | 原因 |
|------|---------|---------|------|
| 使用名词短语描述状态 | "API Server 进程崩溃" | "API Server 挂了" | 专业术语，避免口语化 |
| 明确问题主体 | "etcd 集群仲裁丢失" | "数据库出问题" | 主体明确，定位清晰 |
| 指明可观测特征 | "Pod 状态为 CrashLoopBackOff" | "Pod 有问题" | 对应可检测的具体状态 |
| 避免模糊描述 | "内存使用率超过 95%" | "内存不够" | 可量化，可设告警阈值 |
| 包含影响范围 | "全集群 CoreDNS 解析超时" | "DNS 慢" | 影响范围明确 |

## 3.3 故障树绘制规范

**层次结构规范**：

```
推荐层数: 3-5 层

层级过少 (< 3层):
  ❌ 分解粒度不足，底事件仍包含多个故障模式
  ❌ 无法精确定位根因

层级过多 (> 5层):
  ❌ 维护成本指数增长
  ❌ 分析路径过长，Agent推理延迟增加
  ❌ 底事件可能超出可观测边界

推荐结构:

  第 1 层: 顶事件 (1个)
           │
  第 2 层: 中间事件 - 故障域/子系统 (3-8个)
           │
  第 3 层: 中间事件 - 问题类别 (5-20个)
           │
  第 4 层: 底事件 - 具体故障模式 (15-60个)
           │
  第 5 层: (可选) 底事件 - 根本原因 (仅复杂问题需要)
```

**布局规范**：

| 规则 | 说明 |
|------|------|
| 顶事件居顶部中央 | 便于一目了然地看到分析目标 |
| 同层事件水平排列 | 保持视觉层次清晰 |
| OR 门优先展开高概率分支 | 左侧放置高风险分支（便于优先阅读） |
| AND 门所有分支等权重展示 | 强调"缺一不可" |
| 转移符号标注目标页 | 跨页引用必须双向标注 |
| 每棵子树不超过 15 个底事件 | 超过则拆分为独立子树 |

---

> **导航**: [<< 上一章 - FTA 数学基础与理论模型](./02-fta-mathematical-foundations.md) | [下一章 - FTA 方法论核心原则 >>](./04-fta-core-principles.md)

---

## Obsidian 相关文档

- [[domain-10-troubleshooting-diagnostics/topic-fta/MOC.md|topic-fta MOC]]
- [[domain-10-troubleshooting-diagnostics/topic-fta/README.md|topic-fta: 故障树分析（FTA）方法论与 AI Agent 智能运维实践]]
- [[domain-10-troubleshooting-diagnostics/topic-fta/01-fta-origin-and-evolution.md|第一章：FTA 起源与发展史]]
- [[domain-10-troubleshooting-diagnostics/topic-fta/02-fta-mathematical-foundations.md|第二章：FTA 数学基础与理论模型]]
- [[domain-10-troubleshooting-diagnostics/topic-fta/04-fta-core-principles.md|第四章：FTA 方法论核心原则]]
- [[domain-10-troubleshooting-diagnostics/topic-fta/05-fta-construction-process.md|第五章：FTA 构建完整流程]]
- [[domain-10-troubleshooting-diagnostics/topic-fta/06-fta-verification-and-quality.md|第六章：FTA 验证与质量保证]]
- [[domain-10-troubleshooting-diagnostics/topic-fta/07-fta-maintenance-and-evolution.md|第七章：FTA 维护与演进策略]]
- [[domain-10-troubleshooting-diagnostics/topic-fta/08-ai-agent-ops-revolution.md|第八章：AI Agent 时代的运维范式革命]]
- [[domain-10-troubleshooting-diagnostics/topic-fta/09-fta-as-agent-knowledge-skeleton.md|第九章：FTA 作为 AI Agent 的知识骨架]]
- [[domain-10-troubleshooting-diagnostics/topic-fta/10-agent-orchestration-patterns.md|第十章：Agent 编排模式与 FTA 逻辑门映射]]
- [[domain-10-troubleshooting-diagnostics/topic-fta/11-fta-driven-runbook-automation.md|第十一章：FTA 驱动的 Runbook 自动化]]

## See Also

- [[domain-10-troubleshooting-diagnostics/topic-fta/01-fta-origin-and-evolution.md|01-fta-origin-and-evolution]]
- [[domain-10-troubleshooting-diagnostics/topic-fta/02-fta-mathematical-foundations.md|02-fta-mathematical-foundations]]
- [[domain-10-troubleshooting-diagnostics/topic-fta/04-fta-core-principles.md|04-fta-core-principles]]
- [[domain-10-troubleshooting-diagnostics/topic-fta/05-fta-construction-process.md|05-fta-construction-process]]
