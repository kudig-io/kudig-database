---
title: 故障树分析（FTA）与取证循证方法论（FEBM）
description: 1. **证据收集**：日志、指标、事件、命令输出
summary: 1. **证据收集**：日志、指标、事件、命令输出
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
tier: supporting
created: '2026-05-23'
last_updated: 2026-07
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

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# 故障树分析（FTA）与取证循证方法论（FEBM）

> **CNCF 状态**: 方法论 | **类别**: Troubleshooting | **主要语言**: Markdown, Mermaid

## 概述

故障树分析（FTA, Fault Tree Analysis）与取证循证方法论（FEBM, Forensic Evidence-Based Methodology）是两种互补的 Kubernetes 生产环境故障诊断方法。FTA 是一种自顶向下的演绎推理方法，从问题现象出发，通过逻辑门（AND/OR）分解为基本原因；FEBM 是一种自底向上的归纳取证方法，从收集的证据出发推理故障原因。两者结合使用——FTA 提供系统化的候选根因框架，FEBM 提供基于证据的验证路径——能显著提高复杂分布式系统的故障定位效率。

## Key Features（核心能力）

- **FTA 故障树**：从顶层事件（问题现象）通过 AND/OR 逻辑门分解到基本原因
- **FEBM 证据链**：证据收集 → 证据分类 → 假设生成 → 假设验证 → 结论输出
- **最小割集分析**：识别导致顶层事件的最小原因组合
- **AI Agent 结合**：FTA 提供候选根因知识库，FEBM 提供验证执行路径
- **可复用的因果图谱**：积累故障案例构建组织级知识库
- **与 K8s 事件集成**：自动从 K8s Events 和指标中收集证据

## 架构与工作原理

FTA + FEBM 联合方法论的工作流：当故障发生时，首先使用 FTA 根据问题类型从预定义的故障树中选择候选根因路径；然后启动 FEBM 流程，收集相关证据（日志、指标、Events、命令输出）；基于证据生成或排除假设；通过额外的诊断命令验证假设；最终确定根因并输出修复方案。AI Agent 可自动化这个过程——FTA 作为知识库提供推理路径，FEBM 作为执行框架驱动证据收集和验证。

## K8s 集成

在 K8s 环境中，FTA 故障树覆盖 Pod 启动失败、服务不可达、性能下降等常见场景，每棵故障树的叶子节点对应可检查的 K8s 资源状态（Pod Status、Events、Node Conditions、Service Endpoints 等）。FEBM 证据收集通过 kubectl 命令、Prometheus 查询和 K8s Events API 自动化执行。

## 生产用例

- **Pod 启动失败诊断**：从 ImagePullBackOff 到 OOMKilled 的系统化排查
- **网络问题定位**：DNS 解析失败、Service 不可达的因果分析
- **性能降级排查**：从应用延迟指标到资源争用的证据链推理
- **AI 运维知识库**：积累故障树和证据模式构建自动化运维 Agent

## 安装与快速开始

```bash
# K8s 证据收集命令模板
kubectl describe pod <name> -n <ns>     # Pod 状态和 Events
kubectl get events -n <ns> --sort-by=.lastTimestamp  # 事件时间线
kubectl top pod -n <ns>               # 资源使用
kubectl logs <pod> -n <ns> --previous # 崩溃前日志
```

## 对比替代方案

相比传统的「试错式」排障，FTA+FEBM 方法论提供系统化、可追溯的推理框架。相比五为什么（5 Whys），FTA 提供更完整的因果图谱覆盖。

## Related

- [[概念/Structural Troubleshooting Framework.md|Structural Troubleshooting Framework]] — Structural Troubleshooting Framework
- [[概念/Production Troubleshooting Playbook.md|Production Troubleshooting Playbook]] — Production Troubleshooting Playbook

- [[README]]
- [[nginx-ingress-fta]]


<!-- risk-assessed -->
