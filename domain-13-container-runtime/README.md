---
title: Container Runtime
description: 整合原 domain-13-container-runtime/22 的容器运行时知识，涵盖 Docker、镜像管理和镜像仓库。
category: domain
tags:
- docker
- container
- image
- registry
- harbor
- supply-chain
- daemonset
- gpu
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Container Runtime 是什么
- 如何 Container Runtime
- Kubernetes 13 container runtime 最佳实践
trigger_keywords:
- Container
- Runtime
- container
- runtime
prerequisites:
- kubectl-basics
- gpu-scheduling-basics
created: "2026-05-23"
---

# Container Runtime

整合原 domain-13-container-runtime/22 的容器运行时知识，涵盖 Docker、镜像管理和镜像仓库。

## 目录结构

| 子目录 | 内容 |
|---|---|
| 01-docker/ | Docker 架构、容器生命周期、网络、存储 |
| 02-image-management/ | [[Harbor|Harbor]]、Registry、镜像安全扫描 |
| 03-supply-chain-runtime/ | 容器运行时供应链安全 |

## 与其他 Domain 的关系

- [[domain-05-security-compliance/README.md|domain-05-security-compliance]] — 镜像安全
- [[domain-17-system-foundation/README.md|domain-90-system-foundation]] — Linux 容器基础

## Related

- Domain-34: CNCF Landscape 开源项目 — Cross-reference
- networking|发布说明索引 — 网络]] — Cross-reference
- domain-03-networking-traffic MOC — Cross-reference
- Topic 应用层架构设计最佳实践 — Cross-reference
- topic-application-architecture MOC — Cross-reference
- [[concepts/bp-common-best-practices.md|Kubernetes 通用最佳实践参考]] — Cross-reference
- [[concepts/KUDIG Knowledge Base Architecture.md|KUDIG Knowledge Base Architecture]] — Cross-reference
- [[domain-14-ai-ml-infra/01-ai-infra/03-gpu-scheduling-management.md|GPU 调度与管理]] — Cross-reference
- [[domain-14-ai-ml-infra/01-ai-infra/05-distributed-training-frameworks.md|分布式训练框架]] — Cross-reference
- domain-08-release-change-management MOC — Cross-reference
- [[skills/learn-decision-tree-mermaid.md|故障排查决策树 - Mermaid 可视化版]] — Cross-reference
- [[skills/skill-22-daemonset-failure.md|DaemonSet 故障诊断与修复 / DaemonSet Failure Diagnosis & Remediation]] — Cross-reference
- [[domain-07-platform-engineering/operate/06-monitoring-alerting-system.md|监控告警体系]] — Cross-reference
- Domain 30: 企业级灾备与业务连续性 (Enterprise Disaster Recovery & Business Continuity) — Cross-reference
- [[entities/ecosystem-changelog.md|生态组件变更日志索引]] — Cross-reference
- [[domain-19-landscape-references/topic-index/cluster-index.md|Cluster 集群知识图谱索引]]
- [[domain-19-landscape-references/topic-index/pvc-index.md|PVC 知识图谱索引]]
- [[domain-19-landscape-references/topic-index/terway-index.md|Terway 知识图谱索引]]
- [[domain-19-landscape-references/topic-index/nginx-ingress-index.md|nginx-ingress-controller 知识图谱索引]]
- [[domain-19-landscape-references/topic-index/higress-index.md|Higress 知识图谱索引]]
