---
title: 集群操作函数库
description: '## 概述'
summary: '本主题包含 Kubernetes 集群常见操作函数和流程，提供标准化的操作模板。'
category: general
tags:
- k8s
- daemonset
- gpu
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 集群操作函数库 是什么
- 如何 集群操作函数库
- Kubernetes 07 platform engineering 最佳实践
trigger_keywords:
- 集群操作函数库
- platform
- engineering
- code
- analysis
prerequisites:
- kubectl-basics
- platform-engineering-basics
- gpu-scheduling-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 集群操作函数库

> 领域: topic-functions
> 创建时间: 2026-05-15
> 最后更新: 2026-05-21

## 概述

本主题包含 Kubernetes 集群常见操作函数和流程，提供标准化的操作模板。

## 内容索引

| 文件 | 说明 | 文档数 |
|------|------|--------|
| cluster-cert | 集群证书管理 | 17 |
| cluster-create | 集群创建流程 | 25 |
| cluster-delete | 集群删除流程 | 13 |
| deployment-create | 应用部署流程 | 10 |
| node-create | 节点添加流程 | 17 |
| kubernetes-core | Kubernetes 核心组件源码深度剖析（基于 kubernetes-1.36.2 真实源码树，行号实测，含 kubelet/kube-proxy） | 10 |
| kubernetes-ecosystem | 生态上下游组件集成点源码分析（CRI/CNI/CSI/网格/可观测/CI-CD/仓库-DNS-LB） | 8 |

> kubernetes-core 系列导航见 [[10-平台工程/06-代码分析/kubernetes-core/README.md|kubernetes-core 源码解析系列总览]]，生态集成系列见 [[10-平台工程/06-代码分析/kubernetes-ecosystem/README.md|kubernetes-ecosystem 系列总览]]，源码树位于 [[33-源码/README.md|33-源码]]。

## 相关主题

- 控制平面
- 生命周期管理
- 场景导航

## Related

- Domain-34: CNCF Landscape 开源项目 — Cross-reference
- [[23-实体/15-参考与索引/release-notes-networking.md|发布说明索引 — 网络]] — Cross-reference
- 网络 MOC — Cross-reference
- Topic 应用层架构设计最佳实践 — Cross-reference
- topic-application-architecture MOC — Cross-reference
- [[22-概念/10-最佳实践/bp-common-best-practices.md|Kubernetes 通用最佳实践参考]] — Cross-reference
- [[35-元数据/metadata/KUDIG Knowledge Base Architecture.md|KUDIG Knowledge Base Architecture]] — Cross-reference
- [[15-AI基础设施/01-基础设施/03-gpu-scheduling-management.md|GPU 调度与管理]] — Cross-reference
- [[15-AI基础设施/01-基础设施/05-distributed-training-frameworks.md|分布式训练框架]] — Cross-reference
- 发布变更 MOC — Cross-reference
- [[26-技能/04-工作负载/pod/培训/learn-decision-tree-mermaid.md|故障排查决策树 - Mermaid 可视化版]] — Cross-reference
- [[26-技能/04-工作负载/daemonset/skill-22-daemonset-failure.md|DaemonSet 故障诊断与修复 / DaemonSet Failure Diagnosis & Remediation]] — Cross-reference
- [[10-平台工程/02-运维/04-monitoring-alerting-system.md|监控告警体系]] — Cross-reference
- Domain 30: 企业级灾备与业务连续性 (Enterprise Disaster Recovery & Business Continuity) — Cross-reference
- [[23-实体/15-参考与索引/ecosystem-changelog.md|生态组件变更日志索引]] — Cross-reference
- [[21-生态参考/03-领域索引/cluster-index.md|Cluster 集群知识图谱索引]]
- [[21-生态参考/03-领域索引/pvc-index.md|PVC 知识图谱索引]]
- [[21-生态参考/03-领域索引/terway-index.md|Terway 知识图谱索引]]
- [[21-生态参考/03-领域索引/nginx-ingress-index.md|nginx-ingress-controller 知识图谱索引]]
- [[21-生态参考/03-领域索引/higress-index.md|Higress 知识图谱索引]]
- [[22-概念/09-平台与发布/platform-engineering-idp.md|Platform Engineering and Internal Developer Platforms]]


<!-- risk-assessed -->
