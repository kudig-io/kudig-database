---
title: 故障树分析（FTA）与取证循证方法论（FEBM）
description: 1. **证据收集**：日志、指标、事件、命令输出
category: reference
tags:
- k8s
- fta
- febm
- troubleshooting
- methodology
- root-cause-analysis
- ingress
- agent
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 故障树分析（FTA）与取证循证方法论（FEBM） 是什么
- 如何 故障树分析（FTA）与取证循证方法论（FEBM）
trigger_keywords:
- 故障树分析
- FTA
- 与取证循证方法论
- FEBM
prerequisites:
- kubectl-basics
---

# 故障树分析（FTA）与取证循证方法论（FEBM）

## FTA：故障树分析

FTA 是一种**演绎推理**方法，从顶层故障事件出发，逐层分解到基本原因：

```
顶层事件（故障现象）
    ├── 逻辑门 AND/OR
    │   ├── 中间事件 1
    │   │   ├── 基本原因 A
    │   │   └── 基本原因 B
    │   └── 中间事件 2
    │       └── 基本原因 C
    └── ...
```

核心要素：
- **故障事件**：可观测的异常状态
- **逻辑门**：AND（全部满足才触发）、OR（任一满足即触发）
- **基本原因**：不可再分的根因
- **最小割集**：导致顶层事件的最小原因组合

## FEBM：取证循证方法论

FEBM 是一种**归纳取证**方法，从证据出发推理结论：

1. **证据收集**：日志、指标、事件、命令输出
2. **证据分类**：时间线、因果关系、相关性
3. **假设生成**：基于证据提出可能的故障原因
4. **假设验证**：通过额外检查排除/确认假设
5. **结论输出**：确定根因并给出修复方案

## FTA + FEBM 联合使用

- **FTA**用于建立知识库中的故障因果图谱
- **FEBM**用于实际排障时的证据推理流程
- AI Agent 结合两者：FTA 提供候选根因，FEBM 提供验证路径

---

> 来源：.zread/wiki/drafts/13-fta-gu-zhang-shu-fen-xi-*.md, .zread/wiki/drafts/14-febm-*.md

## Related

- [[synthesis/Structural Troubleshooting Framework.md|Structural Troubleshooting Framework]] — Structural Troubleshooting Framework
- [[synthesis/Production Troubleshooting Playbook.md|Production Troubleshooting Playbook]] — Production Troubleshooting Playbook

- [[README]]
- [[nginx-ingress-fta]]