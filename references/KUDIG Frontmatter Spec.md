---
title: KUDIG Frontmatter Specification
description: '| `audience` | array | Target audience | `[SRE, DevOps, 运维工程师]` |'
category: reference
tags:
- k8s
- frontmatter
- metadata
- spec
- etcd
- kubelet
- rag
- agent
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- KUDIG Frontmatter Specification 是什么
- 如何 KUDIG Frontmatter Specification
trigger_keywords:
- KUDIG
- Frontmatter
- Specification
prerequisites:
- kubectl-basics
- etcd-basics
created: "2026-05-23"
---

# KUDIG Frontmatter Specification

## Purpose

Defines the standardized YAML frontmatter format for all 3,337+ documents in the KUDIG knowledge base. Ensures consistent metadata for document classification, search indexing, and Agent/RAG retrieval.

## Required Fields

| Field | Type | Description | Example |
|-------|------|-------------|---------|
| `title` | string | Document title | "Kubernetes Production Environment Quick Reference" |
| `description` | string | Brief document description | "Covers 90%+ of production commands" |
| `category` | enum | Document category | `docs`, `fta`, `skills`, `cheatsheet`, `scenario`, `moc` |
| `tags` | array | Document tags from [[references/KUDIG Tag Dictionary.md|KUDIG Tag Dictionary]] | `[k8s, troubleshooting, etcd]` |

## Recommended Fields

| Field | Type | Description | Example |
|-------|------|-------------|---------|
| `difficulty` | enum | Content difficulty | `beginner`, `intermediate`, `advanced`, `expert` |
| `reading_level` | enum | Required reading level | Same as difficulty |
| `audience` | array | Target audience | `[SRE, DevOps, 运维工程师]` |
| `estimated_read_time` | string | Estimated reading time | `5min`, `10min`, `30min` |
| `last_updated` | string | Last update date (YYYY-MM) | `2026-05` |
| `authors` | array | Author information | `{name: KUDIG Team, role: contributor}` |
| `k8s_versions` | array | Applicable K8s versions | `["1.28", "1.29", "1.30", "1.31", "1.32"]` |
| `related_docs` | array | Related document references | `{path: ..., desc: ...}` |

## Optional Fields

| Field | Type | Description |
|-------|------|-------------|
| `aliases` | array | Alternative names/synonyms |
| `intent_queries` | array | Expected user queries that match this doc |
| `trigger_keywords` | array | Keywords that trigger this doc in search |

## Category Values

| Category | Description | Source Topics |
|----------|-------------|--------------|
| `docs` | Spec and dictionary documents | docs/ |
| `fta` | Fault Tree Analysis documents | domain-10-troubleshooting-diagnostics/topic-fta/ |
| `skills` | Diagnostic skill documents | domain-10-troubleshooting-diagnostics/topic-skills/ |
| `cheatsheet` | Quick reference cards | domain-17-system-foundation/topic-cheat-sheet/ |
| `scenario` | Production scenario guides | topic-scenarios/ |
| `learning` | Learning path documents | domain-11-production-operations/topic-learn/ |
| `moc` | Map of Contents / navigation pages | All MOC.md files |
| `template` | Document templates | templates/ |
| `prompt` | Agent prompt templates | prompts/ |
| `man` | Man pages | man/ |

## Example Frontmatter

```yaml
---
tags: [k8s, skills, sop, troubleshooting, kubelet, node]
audience: [SRE, 运维工程师]  - name: KUDIG Team
    role: contributor  - "1.28"
  - "1.29"
  - "1.30"
  - "1.31"
  - "1.32"  - node-notready
  - 节点NotReady  - "node not ready how to fix"
  - "节点显示 NotReady 怎么办"  - node notready
  - node unknown
  - kubelet down
---
```

## Related

- [[skills/skill-k8s-node-notready-SKILL.md|skill-k8s-node-notready-SKILL]] — Skill
- [[entities/kubelet.md|kubelet]] — kubelet
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[references/KUDIG Tag Dictionary.md|KUDIG Tag Dictionary]]
- [[references/KUDIG Scenario Taxonomy.md|KUDIG Scenario Taxonomy]]
- [[docs/SYNONYM-DICTIONARY.md|SYNONYM-DICTIONARY]]
- [[docs/FRONTMATTER-SPEC.md|KUDIG Frontmatter 规范]]
