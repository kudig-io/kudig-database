---
title: Workloads & Applications
description: 整合原 工作负载 和 工作负载
  的工作负载与应用知识，涵盖 K8s 原生工作负载、Java on K8s 和应用架构模式。
summary: 整合原 工作负载 和 工作负载 的工作负载与应用知识，涵盖
  K8s 原生工作负载、Java on K8s 和应用架构模式。
category: domain
tags:
- workloads
- deployment
- statefulset
- daemonset
- java
- applications
- job
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
- Workloads & Applications 是什么
- 如何 Workloads & Applications
- Kubernetes 02 workloads applications 最佳实践
trigger_keywords:
- Workloads
- Applications
- workloads
- applications
prerequisites:
- kubectl-basics
- pod-lifecycle
- gpu-scheduling-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Workloads & Applications

整合原 工作负载 和 工作负载 的工作负载与应用知识，涵盖 K8s 原生工作负载、Java on K8s 和应用架构模式。

## 目录结构

| 子目录 | 内容 |
|---|---|
| 00-core-workloads/ | Deployment、[[StatefulSet|StatefulSet]]、[[DaemonSet|DaemonSet]]、Job |
| topic-functions/ | 集群创建函数库与场景化脚本 |
| topic-java-kubernetes/ | Java/Kubernetes 应用专项（JVM 调优、Spring Boot on K8s） |
| 98-merged-indexes/ | 原始域元数据保留 |

## 与其他 Domain 的关系

- [[集群基础/README.md|集群基础]] — 集群基础架构
- [[平台工程/README.md|平台工程]] — 平台工程

## Related

- Domain-34: CNCF Landscape 开源项目 — Cross-reference
- networking|发布说明索引 — 网络]] — Cross-reference
- 网络 MOC — Cross-reference
- Topic 应用层架构设计最佳实践 — Cross-reference
- topic-application-architecture MOC — Cross-reference
- [[concepts/bp-common-best-practices.md|Kubernetes 通用最佳实践参考]] — Cross-reference
- [[concepts/KUDIG Knowledge Base Architecture.md|KUDIG Knowledge Base Architecture]] — Cross-reference
- [[AI基础设施/01-ai-infra/03-gpu-scheduling-management.md|GPU 调度与管理]] — Cross-reference
- [[AI基础设施/01-ai-infra/05-distributed-training-frameworks.md|分布式训练框架]] — Cross-reference
- 发布变更 MOC — Cross-reference
- [[skills/learn-decision-tree-mermaid.md|故障排查决策树 - Mermaid 可视化版]] — Cross-reference
- [[skills/skill-22-daemonset-failure.md|DaemonSet 故障诊断与修复 / DaemonSet Failure Diagnosis & Remediation]] — Cross-reference
- [[平台工程/operate/06-monitoring-alerting-system.md|监控告警体系]] — Cross-reference
- Domain 30: 企业级灾备与业务连续性 (Enterprise Disaster Recovery & Business Continuity) — Cross-reference
- [[entities/ecosystem-changelog.md|生态组件变更日志索引]] — Cross-reference
- [[生态参考/topic-index/cluster-index.md|Cluster 集群知识图谱索引]]
- [[生态参考/topic-index/pvc-index.md|PVC 知识图谱索引]]
- [[生态参考/topic-index/terway-index.md|Terway 知识图谱索引]]
- [[生态参考/topic-index/nginx-ingress-index.md|nginx-ingress-controller 知识图谱索引]]
- [[生态参考/topic-index/higress-index.md|Higress 知识图谱索引]]


<!-- risk-assessed -->
