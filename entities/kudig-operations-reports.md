---
title: KUDIG 项目运维报告：Comate 操作记录与计划
description: '# KUDIG 项目运维报告'
summary: '网络 和 存储 的内容增强计划。'
category: reference
tags:
- k8s
- operations
- comate
- cleanup
- enhancement
- code-review
- rag
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- KUDIG 项目运维报告：Comate 操作记录与计划 是什么
- 如何 KUDIG 项目运维报告：Comate 操作记录与计划
trigger_keywords:
- KUDIG
- 项目运维报告：Comate
- 操作记录与计划
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# KUDIG 项目运维报告

## 清理重复文件

### 操作摘要

| 项目 | 数量 |
|------|------|
| 删除的重复文件 | 853 |
| 删除的重复目录 | 106 |
| 总计删除项 | 959 |
| 误删文件 | 0 |

重复判断依据：文件名/目录名中包含 " 2"（空格+数字2）后缀。

### 涉及范围
- 工作负载 ~ domain-40（大部分域目录）
- topic-*（14 个专题目录）
- gitbook/、man/、_reports/

## Topic-Skills 代码审查

对 topic-skills 目录的代码质量和文档一致性进行审查。

## 网络存储增强

网络 和 存储 的内容增强计划。

## 结构化排查扩展

topic-structural-trouble-shooting 的扩展计划，覆盖更多问题场景。

## Topic-Skills 增强

topic-skills 的质量提升和内容扩展。

---

> 来源：.comate/specs/*.md（共 15 篇）

## Related

- [[entities/k8s-node-create.md|k8s-node-create]] — Kubernetes 节点管理操作指南
- [[entities/k8s-cluster-cert.md|k8s-cluster-cert]] — Kubernetes 集群证书管理操作指南
- [[entities/k8s-cluster-create.md|k8s-cluster-create]] — Kubernetes 集群创建操作指南
- [[entities/k8s-cluster-delete.md|k8s-cluster-delete]] — Kubernetes 集群删除操作指南
- [[entities/k8s-deployment-create.md|k8s-deployment-create]] — Kubernetes Deployment 创建操作指南


<!-- risk-assessed -->
