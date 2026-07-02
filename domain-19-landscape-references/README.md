---
title: Landscape & References
description: Landscape & References — Kubernetes 生产运维知识库
summary: Landscape & References — Kubernetes 生产运维知识库
category: domain
tags:
- cncf
- papers
- reference
- landscape
- ecosystem
- daemonset
- gpu
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Landscape & References 是什么
- 如何 Landscape & References
- Kubernetes 19 landscape references 最佳实践
trigger_keywords:
- Landscape
- References
- landscape
- references
prerequisites:
- kubectl-basics
- cncf-ecosystem
- gpu-scheduling-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Landscape & References

整合原 domain-19-landscape-references/34 的参考资料，涵盖 CNCF 全景图和学术论文。

## 目录结构

| 子目录 | 内容 |
|---|---|
| 01-cncf-landscape/ | CNCF 项目全景（graduated/incubating/sandbox 目录待填充） |
| 02-papers/ | [[Kubernetes|Kubernetes]] 相关学术论文 |
| topic-index/ | 按主题的知识图谱索引（cluster/pvc/networking 等） |
| topic-release-notes/ | Kubernetes 及生态组件版本发布说明归档 |
| _archived-release-notes/ | 历史发布说明归档（安全/存储/CLI 等） |

## 与其他 Domain 的关系

- 所有 Tier 1-4 Domain — 生态参考

## Related

- [[reference|#reference Hub]] — tag hub

- [[papers|#papers Hub]] — tag hub

- Domain-34: CNCF Landscape 开源项目 — Cross-reference
- networking|发布说明索引 — 网络]] — Cross-reference
- domain-03-networking-traffic KUDIG Database — Global MOC — Cross-reference
- Topic 应用层架构设计最佳实践]] — Cross-reference
- topic-application-architecture MOC — Cross-reference
- [[concepts/bp-common-best-practices.md|[[Kubernetes 通用最佳实践参考|Kubernetes 通用最佳实践参考]]]] — Cross-reference
- KUDIG Knowledge Base Architecture|KUDIG Knowledge Base Architecture]] — Cross-reference
- [[domain-14-ai-ml-infra/01-ai-infra/03-gpu-scheduling-management.md|GPU 调度与管理]] — Cross-reference
- [[domain-14-ai-ml-infra/01-ai-infra/05-distributed-training-frameworks.md|分布式训练框架]] — Cross-reference
- domain-08-release-change-management MOC — Cross-reference
- [[skills/learn-decision-tree-mermaid.md|故障排查决策树 - Mermaid 可视化版]] — Cross-reference
- [[skills/skill-22-daemonset-failure.md|[[DaemonSet 故障诊断与修复 / DaemonSet Failure Diagnosis & Remediation|DaemonSet 故障诊断与修复 / DaemonSet Failure Diagnosis & Remediation]]]] — Cross-reference
- [[domain-07-platform-engineering/operate/06-monitoring-alerting-system.md|监控告警体系]] — Cross-reference
- Domain 30: 企业级灾备与业务连续性 (Enterprise Kubernetes 灾难恢复最佳实践 & Business Continuity) — Cross-reference
- [[entities/ecosystem-changelog.md|生态组件变更日志索引]] — Cross-reference
- [[domain-19-landscape-references/topic-index/cluster-index.md|Cluster 集群知识图谱索引]]
- [[domain-19-landscape-references/topic-index/pvc-index.md|PVC 知识图谱索引]]
- [[domain-19-landscape-references/topic-index/terway-index.md|Terway 知识图谱索引]]
- [[domain-19-landscape-references/topic-index/nginx-ingress-index.md|nginx-ingress-controller 知识图谱索引]]
- [[domain-19-landscape-references/topic-index/higress-index.md|Higress 知识图谱索引]]

## 相关概念

- [[concepts/etcd Operational Reference.md|etcd Operational Reference]]
- [[skills/skill-reference-version-matrix.md|Version Matrix]]
- [[skills/skill-reference-remediation-playbook.md|Remediation Playbook]]
- [[skills/skill-reference-root-cause-catalog.md|Root Cause Catalog]]
- [[entities/kubernetes.md|Kubernetes (CNCF Graduated)]]
- [[entities/cncf-orchestration.md|CNCF 编排与应用管理项目全景]]
- [[entities/cncf-infrastructure.md|CNCF 基础设施与混沌工程项目全景]]
- [[entities/cncf-runtime.md|CNCF 容器运行时与工具链项目全景]]


<!-- risk-assessed -->
