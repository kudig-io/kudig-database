---
title: 附录 A：FTA 术语表 [故障诊断]
description: 'description: ''**关联主文档**: [FTA 方法论与 AI Agent 智能运维实践](./fta-methodology-and-agentic-practices.md)'''
summary: 'description: ''**关联主文档**: [FTA 方法论与 AI Agent 智能运维实践](./fta-methodology-and-agentic-practices.md)'''
category: fta
tags:
- fta
- troubleshooting
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
estimated_read_time: 5min
intent_queries:
- 附录 A：FTA 术语表 是什么
- 如何 附录 A：FTA 术语表
- Kubernetes 10 troubleshooting diagnostics 最佳实践
- 附录 A：FTA 术语表 故障排查
- 附录 A：FTA 术语表 排障步骤
- 附录 A：FTA 术语表 根因分析
trigger_keywords:
- 附录
- A：FTA
- 术语表
- troubleshooting
- diagnostics
- fta
prerequisites:
- kubectl-basics
- troubleshooting-methodology
fta_id: FTA-APPENDIX_A_GLOSSARY-001
component: Appendix A Glossary
severity: high
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




title: 附录 A：FTA 术语表
description: '**关联主文档**: [FTA 方法论与 AI Agent 智能运维实践](./fta-methodology-and-agentic-practices.md)'
category: fta
tags:
- k8s
- fault-tree
- root-cause
- troubleshooting
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
- 附录 A：FTA 术语表 是什么
- 如何 附录 A：FTA 术语表
- 附录 A：FTA 术语表 根因分析
- 附录 A：FTA 术语表 故障树
trigger_keywords:
- 附录
- A：FTA
- 术语表
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
# 附录 A：FTA 术语表

> **关联主文档**: [FTA 方法论与 AI Agent 智能运维实践](./fta-methodology-and-agentic-practices.md)  
> **上一章**: 第二十二章：行业标准化建议](./22-industry-standardization.md)  
> **下一附录**: 附录 B：工具与资源清单](./[[故障诊断/topic-fta/appendix-b-tools-and-resources.md|appendix-b-tools-and-resources]].md)

---

| 中文术语 | 英文术语 | 缩写 | 定义 |
|---------|---------|------|------|
| 故障树分析 | Fault Tree Analysis | FTA | 自顶向下的演绎式系统安全分析方法 |
| 顶事件 | Top Event | TE | 故障树最顶层的不期望事件 |
| 中间事件 | Intermediate Event | IE | 问题传播链中的中间层事件 |
| 底事件/基本事件 | Basic Event | BE | 不可再分解的最底层问题事件 |
| 或门 | OR Gate | - | 任一输入发生则输出发生 |
| 与门 | AND Gate | - | 全部输入发生则输出发生 |
| 最小割集 | Minimal Cut Set | MCS | 使顶事件发生的最小底事件集合 |
| 割集阶数 | Cut Set Order | - | 最小割集中底事件的数量 |
| 重要度 | Importance Measure | - | 底事件对顶事件的影响程度 |
| 平均问题间隔 | Mean Time Between Failures | MTBF | 系统两次问题之间的平均时间 |
| 平均修复时间 | Mean Time To Repair | MTTR | 从问题发生到恢复的平均时间 |
| 平均检测时间 | Mean Time To Detect | MTTD | 从问题发生到被检测到的平均时间 |
| 可用性 | Availability | A | 系统正常运行的时间比例 |
| 可靠度 | Reliability | R(t) | 系统在时间 t 内无问题运行的概率 |
| 问题率 | Failure Rate | λ | 单位时间内发生问题的概率 |
| 风险优先级数 | Risk Priority Number | RPN | 严重度 x 发生频率 x 可检测性 |
| 故障模式与影响分析 | Failure Mode and Effects Analysis | FMEA | 自底向上的归纳式分析方法 |
| 共因问题 | Common Cause Failure | CCF | 由同一根因导致的多个组件问题 |
| 外部事件/房屋事件 | House Event | HE | 正常预期会发生的事件 |
| 未展开事件 | Undeveloped Event | UE | 暂未分解到底的事件 |
| 投票门 | Voting Gate | k/n | n 个输入中至少 k 个发生 |
| 抑制门 | Inhibit Gate | - | 带条件约束的 AND 门 |
| 优先与门 | Priority AND Gate | PAND | 按时序发生的 AND 门 |
| 转移符号 | Transfer Symbol | - | 故障树跨页连接标记 |

---

> **导航**: [<< 第二十二章 - 行业标准化建议](./22-industry-standardization.md) | [附录 B - 工具与资源清单 >>](./appendix-b-tools-and-resources.md)

---

## Obsidian 相关文档

- [[故障诊断/topic-fta/MOC.md|topic-fta [[KUDIG Database — Global MOC|MOC]]]]
- [[故障诊断/topic-fta/README.md|topic-fta: 故障树分析（FTA）方法论与 AI Agent 智能运维实践]]
- [[故障诊断/topic-fta/01-fta-origin-and-evolution.md|第一章：FTA 起源与发展史]]
- [[故障诊断/topic-fta/02-fta-mathematical-foundations.md|第二章：FTA 数学基础与理论模型]]
- [[故障诊断/topic-fta/03-fta-symbol-system-and-standards.md|第三章：FTA 符号体系与标准规范]]
- [[故障诊断/topic-fta/04-fta-core-principles.md|第四章：FTA 方法论核心原则]]
- [[故障诊断/topic-fta/05-fta-construction-process.md|第五章：FTA 构建完整流程]]
- [[故障诊断/topic-fta/06-fta-verification-and-quality.md|第六章：FTA 验证与质量保证]]
- [[故障诊断/topic-fta/07-fta-maintenance-and-evolution.md|第七章：FTA 维护与演进策略]]
- [[故障诊断/topic-fta/08-ai-agent-ops-revolution.md|第八章：AI Agent 时代的运维范式革命]]
- [[故障诊断/topic-fta/09-fta-as-agent-knowledge-skeleton.md|第九章：FTA 作为 AI Agent 的知识骨架]]
- [[故障诊断/topic-fta/10-agent-orchestration-patterns.md|第十章：Agent 编排模式与 FTA 逻辑门映射]]

## See Also

- [[故障诊断/topic-fta/23-fta-production-quick-start.md|23-fta-production-quick-start]]
- [[故障诊断/topic-fta/ack-fta-generator-v2.md|ack-fta-generator-v2]]
- [[故障诊断/topic-fta/appendix-b-tools-and-resources.md|appendix-b-tools-and-resources]]
- [[故障诊断/topic-fta/appendix-c-references.md|appendix-c-references]]


<!-- risk-assessed -->
