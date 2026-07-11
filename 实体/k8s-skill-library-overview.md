---
title: 运维 Skill 库：AI Agent 可执行的工单诊断-修复闭环
description: '# 运维 Skill 库'
summary: 'Skill 是 KUDIG 的核心知识单元，将运维经验编码为结构化的诊断-修复流程：'
category: reference
tags:
- k8s
- skill
- ai-agent
- runbook
- diagnostic
- remediation
- rbac
- rag
- agent
tier: peripheral
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 运维 Skill 库：AI Agent 可执行的工单诊断-修复闭环 是什么
- 如何 运维 Skill 库：AI Agent 可执行的工单诊断-修复闭环
trigger_keywords:
- 运维
- Skill
- 库：AI
- Agent
- 可执行的工单诊断-修复闭环
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# 运维 Skill 库

## 概述

运维 Skill 库是 KUDIG（Kubernetes Universal Diagnostic & Intelligent Governance）平台的**核心知识单元体系**。它将资深 SRE 和运维工程师的经验编码为结构化、可执行的**诊断-修复流程（Diagnostic-Remediation Workflow）**，使 AI Agent 能够自动化执行运维工单的诊断和修复闭环。

每个 Skill 是一份标准化文档，定义了从**问题现象识别 → 诊断工作流 → 根因匹配 → 修复方案 → 验证确认**的完整闭环。Skill 库不是简单的运维文档集合，而是可以被 AI Agent 解析和执行的**可操作知识（Actionable Knowledge）**——每一步都有明确的命令、预期输出和判断逻辑。

## Skill 设计理念

Skill 是 KUDIG 的核心知识单元，将运维经验编码为结构化的诊断-修复流程：

```
问题现象（Symptom）
    ↓
诊断工作流（Diagnostic Workflow）
    ↓
根因匹配（Root Cause Catalog）
    ↓
修复方案（Remediation Playbook）
    ↓
验证确认（Verification）
```

## Skill 文档结构

每个 Skill 文档包含：
- **skill_id**：唯一标识（如 SKILL-NODE-001）
- **skill_name**：双语名称（中文 + 英文）
- **触发条件**：问题现象描述和匹配关键词
- **诊断步骤**：具体的诊断命令 + 预期输出 + 判断逻辑
- **修复方案**：操作步骤 + 风险等级（🟢/🟡/🔴）+ 回滚方案
- **关联文档**：FTA 故障树、最佳实践、参考文档
- **RBAC 要求**：执行该 Skill 所需的 Kubernetes 权限

## 覆盖领域

Skill 库已覆盖 20+ 常见问题场景：
- **节点问题**：Node NotReady、资源压力（CPU/Memory/Disk）、kubelet 异常
- **Pod 问题**：CrashLoopBackOff、OOMKilled、ImagePullBackOff
- **网络问题**：Service 不通、DNS 解析失败、NetworkPolicy 阻断
- **存储问题**：PV 挂载失败、存储容量不足、PVC Pending
- **调度问题**：调度失败、资源配额超限、亲和性冲突
- **安全问题**：RBAC 权限拒绝、ServiceAccount 异常、证书过期

## Architecture

Skill 库由三层构成：**Skill 文档层**（Markdown 格式的结构化运维知识）、**Skill 引擎**（AI Agent 解析和执行 Skill 的引擎）和**执行沙箱**（安全执行诊断命令和修复操作的环境）。AI Agent 接收工单 → 匹配 Skill → 在安全沙箱中执行诊断步骤 → 根据输出匹配根因 → 执行修复方案 → 验证结果。整个过程有完整的审计日志和人工审批门控（针对高风险操作）。

## K8s 集成

Skill 库深度集成 Kubernetes 运维。诊断步骤使用 `kubectl`、`crictl`、`nerdctl` 等标准 K8s 工具。修复方案通过 Kubernetes API 执行（如重启 Pod、调整 HPA、清理资源）。RBAC 权限控制每个 Skill 可执行的操作范围。RAG（Retrieval-Augmented Generation）机制让 AI Agent 能根据工单描述检索最相关的 Skill。

## 生产部署要点

- **Skill 审计**：所有 Skill 变更经过 SRE 团队审核，确保安全性和准确性
- **分级权限**：🟢 只读诊断 Skill 可自动执行，🟡 修复 Skill 需人工确认，🔴 高风险需 SRE 审批
- **回滚方案**：每个修复 Skill 必须包含回滚步骤
- **持续更新**：每次生产故障后创建/更新对应的 Skill

## 生产场景

1. **AI 自动诊断**：值班 AI Agent 接收 Pod CrashLoopBackOff 工单，自动执行诊断 Skill
2. **半自动修复**：AI 匹配到 OOMKilled 根因，建议调整内存限制，人工确认后执行
3. **知识沉淀**：将资深 SRE 的排查经验固化为 Skill，供 AI 和初级工程师复用
4. **故障自愈**：常见故障（如 Pod 重启、HPA 扩容）通过 Skill 实现自动修复

---

> 来源：.zread/wiki/drafts/16-yun-wei-skill-ku-*.md

## Related

- [[实体/k8s-ai-agent-engineering.md|k8s-ai-agent-engineering]] — AI Agent 工程：RAG、多 Agent 编排、安全护栏与生产部署
- [[技能/skill-reference-root-cause-catalog.md|skill-reference-root-cause-catalog]] — Root Cause Catalog
- [[技能/skill-reference-remediation-playbook.md|skill-reference-remediation-playbook]] — Remediation Playbook
- [[技能/skill-reference-diagnostic-workflow.md|skill-reference-diagnostic-workflow]] — Diagnostic Workflow
- [[技能/skill-k8s-node-notready-SKILL.md|skill-k8s-node-notready-SKILL]] — Skill


<!-- risk-assessed -->
