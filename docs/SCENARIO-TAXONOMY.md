---
title: KUDIG 场景分类体系
description: '| SC-01 | 集群部署 | `topic-scenarios/cluster-deployment.md` | domain-1,
  domain-4, domain-07-platform-engineering | ~20 |'
summary: '| SC-01 | 集群部署 | `topic-scenarios/cluster-deployment.md` | domain-1, domain-4,
  domain-07-platform-engineering | ~20 |'
category: general
tags:
- k8s
- etcd
- rag
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
- KUDIG 场景分类体系 是什么
- 如何 KUDIG 场景分类体系
trigger_keywords:
- KUDIG
- 场景分类体系
prerequisites:
- kubectl-basics
- etcd-basics
---



---
title: KUDIG 场景分类体系
description: KUDIG 场景分类体系
category: docs
tags:
- k8s
- scenario
- taxonomy
relationships:
- target: "[[docs/TAG-DICTIONARY.md|KUDIG 全局标签字典]]"
  type: related_to
- target: "[[docs/FRONTMATTER-SPEC.md|KUDIG Frontmatter 规范]]"
  type: related_to
- target: "[[docs/SYNONYM-DICTIONARY.md|KUDIG 同义词与别名词典]]"
  type: related_to
difficulty: intermediate
reading_level: intermediate
audience:
- SRE
- DevOps
estimated_read_time: 10min
last_updated: 2026-05
authors:
- name: KUDIG Team
  role: contributor
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'

tier: peripheral---

# KUDIG 场景分类体系

> 创建时间: 2026-05-20
> 用途: 按"生产场景"而非"文档结构"组织知识入口，支持 Agent 意图路由

---

## 场景分类总览

| ID | 场景名称 | 入口 | 覆盖领域 | 文档预估 |
|---|---|---|---|---|
| SC-01 | 集群部署 | `topic-scenarios/cluster-deployment.md` | domain-1, domain-4, domain-07-platform-engineering | ~20 |
| SC-02 | 应用部署 | `topic-scenarios/app-deployment.md` | domain-4, domain-18-manifests-patterns | ~30 |
| SC-03 | 问题排查 | `topic-scenarios/troubleshooting.md` | domain-12, topic-fta, topic-skills | ~100 |
| SC-04 | 性能调优 | `topic-scenarios/performance-tuning.md` | domain-1, domain-13, domain-11-production-operations | ~25 |
| SC-05 | 安全加固 | `topic-scenarios/security-hardening.md` | domain-7, domain-25, domain-05-security-compliance | ~30 |
| SC-06 | 监控告警 | `topic-scenarios/monitoring-alerting.md` | domain-8, domain-20, domain-06-observability | ~30 |
| SC-07 | 备份恢复 | `topic-scenarios/backup-restore.md` | domain-30, domain-3, topic-fta | ~20 |
| SC-08 | 升级迁移 | `topic-scenarios/upgrade-migration.md` | domain-1, topic-migration | ~25 |
| SC-09 | 日常运维 | `topic-scenarios/daily-ops.md` | domain-9, topic-skills | ~40 |
| SC-10 | AI 基础设施 | `topic-scenarios/ai-infra-ops.md` | domain-11, topic-ai-agent | ~30 |
| SC-11 | 网络诊断 | `topic-scenarios/network-diagnosis.md` | domain-5, domain-03-networking-traffic | ~25 |
| SC-12 | 存储问题 | `topic-scenarios/storage-issues.md` | domain-6, domain-04-storage-data | ~20 |
| SC-13 | 安全事件响应 | `topic-scenarios/security-incident.md` | domain-7, domain-25, domain-05-security-compliance | ~15 |
| SC-14 | 容量规划 | `topic-scenarios/capacity-planning.md` | domain-18, domain-07-platform-engineering | ~15 |
| SC-15 | GitOps 工作流 | `topic-scenarios/gitops-workflow.md` | domain-23, domain-08-release-change-management | ~20 |
| SC-16 | Service Mesh 运维 | `topic-scenarios/mesh-ops.md` | domain-03-networking-traffic | ~15 |
| SC-17 | 多集群管理 | `topic-scenarios/multi-cluster.md` | domain-9, domain-12-cloud-providers | ~15 |
| SC-18 | 边缘计算运维 | `topic-scenarios/edge-ops.md` | domain-15-specialized-tech | ~10 |
| SC-19 | 成本优化 | `topic-scenarios/cost-optimization.md` | domain-18, domain-07-platform-engineering | ~10 |
| SC-20 | 合规审计 | `topic-scenarios/compliance-audit.md` | domain-25, domain-05-security-compliance | ~10 |

---

## 场景定义规范

每个场景页应包含:

```yaml
---
title: "场景: {{场景名称}}"
category: scenario
scenario_id: "SC-{{ID}}"
tags: [k8s, scenario, {{primary-tag}}]
last_updated: "YYYY-MM-DD"
---
```

### 场景页结构

1. **场景概述** — 该场景的目标、触发条件、影响范围
2. **快速决策树** — Mermaid 决策图，3 步内定位问题
3. **相关文档索引** — 按排查优先级排列的文档链接
4. **操作手册** — 可直接执行的命令/步骤
5. **升级路径** — 何时需要升级、升级到谁

---

## 场景 → 文档映射规则

| 场景 | 优先文档 | 辅助文档 |
|---|---|---|
| 集群部署 | domain-01-cluster-fundamentals/12-cluster-deployment-patterns.md | domain-07-platform-engineering/*, domain-08-release-change-management/topic-deployment/* |
| 应用部署 | domain-02-workloads-applications/* | domain-18-manifests-patterns/* |
| 问题排查 | domain-10-troubleshooting-diagnostics/* | domain-10-troubleshooting-diagnostics/topic-fta/list/*, domain-10-troubleshooting-diagnostics/topic-skills/* |
| 性能调优 | domain-01-cluster-fundamentals/13-performance-tuning-guide.md | domain-11-production-operations/* |
| 安全加固 | domain-05-security-compliance/* | domain-05-security-compliance/*, domain-05-security-compliance/* |
| 监控告警 | domain-06-observability/* | domain-06-observability/*, domain-06-observability/* |
| 备份恢复 | domain-01-cluster-fundamentals/* (etcd) | domain-09-reliability-engineering/* |
| 升级迁移 | domain-01-cluster-fundamentals/07,18-upgrade* | domain-08-release-change-management/topic-migration/* |
| 日常运维 | domain-07-platform-engineering/* | domain-10-troubleshooting-diagnostics/topic-skills/* |
| AI 基础设施 | domain-14-ai-ml-infra/* | domain-14-ai-ml-infra/topic-ai-agent/* |

---

*本文档定义了场景分类体系，场景页由脚本自动生成。*

---

## Related

- [[entities/KUDIG Tag Dictionary.md|KUDIG Tag Dictionary]]
- [[entities/KUDIG Frontmatter Spec.md|KUDIG Frontmatter Spec]]
- [[docs/TAG-DICTIONARY.md|KUDIG 全局标签字典]]
- [[docs/FRONTMATTER-SPEC.md|KUDIG Frontmatter 规范]]
