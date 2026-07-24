---
title: KUDIG Templates and Agent Prompts
description: KUDIG Templates and Agent Prompts — Kubernetes 生产运维知识库
summary: KUDIG Templates and Agent Prompts — Kubernetes 生产运维知识库
category: reference
tags:
- k8s
- templates
- prompts
- agents
- agent
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- KUDIG Templates and Agent Prompts 是什么
- 如何 KUDIG Templates and Agent Prompts
trigger_keywords:
- KUDIG
- Templates
- and
- Agent
- Prompts
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# KUDIG Templates and Agent Prompts

## Document Templates

Available in `templates/`:

| Template | Purpose | Structure |
|----------|---------|-----------|
| `fta-template.md` | Fault Tree Analysis documents | Top events, intermediate events, bottom events with observability, diagnosis commands, healing actions |
| `skill-template.md` | Diagnostic skill cards | Trigger conditions, decision tree, diagnostic commands, root cause catalog, remediation playbook, escalation path |
| `cheat-sheet-template.md` | Quick reference cards | Organized by scenario or topic, commands with explanations |
| `moc-template.md` | Map of Contents / navigation pages | Scope definition, document list with difficulty/tags, statistics |
| `decision-tree-template.md` | Decision tree documents | Input conditions, branching logic, leaf outcomes |
| `best-practice-template.md` | Best practice guides | Context, recommendation, rationale, examples, anti-patterns |
| `domain-article-template.md` | Domain knowledge articles | Standard knowledge base article format |
| `febm-template.md` | Forensic Evidence-Based Method | Evidence collection, forensic analysis patterns |
| `presentation-template.md` | Presentation/slides | Structured slide deck format |
| `PROJECT-INDEX-TEMPLATE.md` | Project index | Project-level navigation and overview |

## Agent Prompt Templates

Available in `prompts/`:

| Prompt | Purpose |
|--------|---------|
| `troubleshooting.md` | AI Agent troubleshooting workflow - guides agent through symptom input, FTA navigation, diagnosis, and remediation |
| `architecture-review.md` | Architecture review prompt - evaluates K8s architecture against best practices |
| `config-generator.md` | Configuration generator prompt - generates K8s manifests from requirements |
| `learning-path.md` | Learning path prompt - creates structured learning paths from knowledge base content |

## FTA Template Key Structure

The FTA template defines the standard structure for fault tree documents:

```yaml
# Each FTA document contains:
severity: P0|P1|P2

# Fault tree structure:
top_event:
  id: "TE-X"
  name: "Description"
  gate: OR|AND
  intermediate_events: [...]

# Each bottom event contains:
bottom_event:
  id: "BE-X.Y"
  name: "Description"
  observable:
    metrics: [...]
    logs: [...]
    events: [...]
  root_causes: [...]
  diagnosis_commands: [...]
  healing_actions:
    - id: "HA-X.Y.Z"
      description: "..."
      risk: low|medium|high|critical
      auto_healable: true|false
      command: "..."
```

## Skill Template Key Structure

```yaml
# Each skill document contains:
tags: [k8s, skills, sop, ...]

# Skill structure:
1. Overview - trigger conditions, impact, severity
2. Quick Decision Tree - 3-step diagnostic flow
3. Diagnostic Commands - specific commands with expected output
4. Root Cause Catalog - common causes ranked by probability
5. Remediation Playbook - step-by-step fixes with risk assessment
6. Escalation Path - when and how to escalate
7. Version Matrix - K8s compatibility
```

## Related

- [[INDEX]] — Wiki Index
- [[技能/工作负载/pod/方法论/skill-reference-root-cause-catalog.md|skill-reference-root-cause-catalog]] — Root Cause Catalog
- [[技能/工作负载/pod/方法论/skill-reference-remediation-playbook.md|skill-reference-remediation-playbook]] — Remediation Playbook
- [[技能/集群运维/cluster-upgrade/reference/skill-reference-version-matrix.md|skill-reference-version-matrix]] — Version Matrix
- [[技能/节点/node/skill-notready/skill-k8s-node-notready-SKILL.md|skill-k8s-node-notready-SKILL]] — Skill
- [[实体/KUDIG Frontmatter Spec.md|KUDIG Frontmatter Spec]]
- [[元数据/KUDIG Knowledge Base Architecture.md|KUDIG Knowledge Base Architecture]]
- [[技能/工作负载/pod/方法论/FTA Methodology and Core Principles.md|FTA Methodology and Core Principles]]

- [[kudig-templates-catalog]]

<!-- risk-assessed -->
