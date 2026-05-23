---
title: 运维 Skill 库：AI Agent 可执行的工单诊断-修复闭环
description: '# 运维 Skill 库'
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
last_updated: 2026-05
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
created: "2026-05-23"
---

# 运维 Skill 库

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
- **skill_name**：双语名称
- **触发条件**：问题现象描述
- **诊断步骤**：命令 + 预期输出 + 判断逻辑
- **修复方案**：操作步骤 + 风险等级 + 回滚方案
- **关联文档**：FTA 故障树、最佳实践、参考文档

## 覆盖领域

Skill 库已覆盖 20+ 常见问题场景：
- Node NotReady / 资源压力
- Pod CrashLoopBackOff / OOMKilled
- Service 不通 / DNS 解析失败
- PV 挂载失败 / 存储容量不足
- 调度失败 / 资源配额超限
- RBAC 权限拒绝 / 网络策略阻断

---

> 来源：.zread/wiki/drafts/16-yun-wei-skill-ku-*.md

## Related

- [[references/k8s-ai-agent-engineering.md|k8s-ai-agent-engineering]] — AI Agent 工程：RAG、多 Agent 编排、安全护栏与生产部署
- [[skills/skill-reference-root-cause-catalog.md|skill-reference-root-cause-catalog]] — Root Cause Catalog
- [[skills/skill-reference-remediation-playbook.md|skill-reference-remediation-playbook]] — Remediation Playbook
- [[skills/skill-reference-diagnostic-workflow.md|skill-reference-diagnostic-workflow]] — Diagnostic Workflow
- [[skills/skill-k8s-node-notready-SKILL.md|skill-k8s-node-notready-SKILL]] — Skill
