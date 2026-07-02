---
title: OpenKruise 全局索引
description: OpenKruise Kubernetes 增强工作负载套件全局索引，聚合 CloneSet、原地升级、Sidecar 管理等所有相关内容
summary: OpenKruise Kubernetes 增强工作负载套件全局索引，聚合 CloneSet、原地升级、Sidecar 管理等所有相关内容
category: index
tags:
- k8s
- index
- catalog
- openkruise
- kruise
- workload
- cloneset
- statefulset
- sidecar
- rag
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- OpenKruise 全局索引 是什么
- OpenKruise 增强工作负载相关内容
trigger_keywords:
- OpenKruise
- kruise
- CloneSet
- 原地升级
- Sidecar
prerequisites:
- kubectl-basics
- cncf-ecosystem
- helm-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# [[OpenKruise|OpenKruise]] 全局索引

> 全局索引：按关键字 **OpenKruise** 聚合项目内所有相关内容。

## 架构基础

- OpenKruise CNCF Landscape ← 核心文档

## 高级工作负载

- CloneSet 高级部署
- StatefulSet|statefulset]]|Advanced StatefulSet]]
- daemonset|Advanced DaemonSet]]

## 核心特性

- 原地升级 (In-place Update)
- Sidecar 管理
- 镜像预热
- 容器重启
- PodUnavailableBudget

## 结构化故障排查 - 工作负载

- troubleshooting-diagnostics/topic-structural-trouble-shooting/05-workloads/|CloneSet 故障排查]]
- troubleshooting|[[entities/kubernetes.md|Kubernetes]] 部署策略最佳实践|Deployment]] 故障排查]]
- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/05-workloads/03-statefulset-troubleshooting.md|[[StatefulSet 故障排查|StatefulSet 故障排查]]]]

## FTA 故障树

- [[domain-10-troubleshooting-diagnostics/topic-fta/list/openkruise-fta.md|[[OpenKruise 工作负载异常故障树分析|OpenKruise FTA]] 故障树]]

## 云厂商集成

- 阿里云 ACK 增强工作负载
- 华为云 CCE 工作负载

## 生产运维

- 工作负载性能调优
- Kubernetes 多租户与资源隔离

## 学习培训

- 工作负载管理学习路径
- Day 16: 工作负载控制器

## CNCF 生态

- Kubernetes
- Helm
- Argo

<!-- risk-assessed -->
