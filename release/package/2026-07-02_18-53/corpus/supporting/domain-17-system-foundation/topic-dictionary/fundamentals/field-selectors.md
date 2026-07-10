---
title: 字段选择器
description: '## 概述'
summary: '## 概述'
category: dictionary
tags:
- k8s
- glossary
- terminology
- scheduler
- statefulset
- job
- crd
tier: supporting
created: '2026-05-23'
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 字段选择器 是什么
- 如何 字段选择器
trigger_keywords:
- 字段选择器
- dictionary
prerequisites:
- kubectl-basics
- cloud-provider-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 字段选择器

## 概述

字段选择器（Field Selectors）允许根据一个或多个资源字段的值来选择 [[entities/kubernetes.md|[[Kubernetes|kubernetes]]]] 对象。与标签选择器不同，字段选择器基于资源的实际字段值进行过滤，是一种更底层的资源筛选机制。

## 核心概念/原理

字段选择器本质上是资源过滤器。默认情况下不应用任何选择器，即选择所有指定类型的资源。以下两个命令是等价的：

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl get pods
kubectl get pods --field-selector ""
```
### 支持的运算符

字段选择器支持 `=`、`==`（两者含义相同）和 `!=` 运算符。

**示例**：选择所有不在 `default` 命名空间中的 [[Service|Service]]：

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl get services --all-namespaces --field-selector metadata.namespace!=default
```
### 链式选择器

字段选择器可以像标签选择器一样用逗号连接，表示逻辑 **AND** 关系：

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl get pods --field-selector=status.phase!=Running,spec.restartPolicy=Always
```
## 关键机制或特性

### 各资源类型支持的字段

所有资源类型都支持 `metadata.name` 和 `metadata.namespace` 字段。使用不支持的字段选择器会产生错误。

| 资源类型 | 支持的字段 |
|---------|-----------|
| Pod | `spec.nodeName`、`spec.restartPolicy`、`spec.schedulerName`、`spec.serviceAccountName`、`spec.hostNetwork`、`status.phase`、`status.podIP`、`status.podIPs`、`status.nominatedNodeName` |
| Event | `involvedObject.kind`、`involvedObject.namespace`、`involvedObject.name`、`involvedObject.uid`、`involvedObject.apiVersion`、`involvedObject.resourceVersion`、`involvedObject.fieldPath`、`reason`、`reportingComponent`、`source`、`type` |
| Secret | `type` |
| Namespace | `status.phase` |
| [[ReplicaSet|ReplicaSet]] | `status.replicas` |
| [[ReplicationController|ReplicationController]] | `status.replicas` |
| Job | `status.successful` |
| Node | `spec.unschedulable` |
| CertificateSigningRequest | `spec.signerName` |

### 自定义资源字段

所有自定义资源类型都支持 `metadata.name` 和 `metadata.namespace` 字段。此外，CustomResourceDefinition 的 `spec.versions[*].selectableFields` 字段可以声明自定义资源中哪些其他字段可用于字段选择器。

### 多资源类型

字段选择器可以跨多种资源类型使用：

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl get statefulsets,services --all-namespaces --field-selector metadata.namespace!=default
```
## 使用场景

- 按 Pod 状态（如 `status.phase=Running`）筛选 Pod。
- 按节点名称（`spec.nodeName`）查找调度到特定节点的 Pod。
- 按命名空间过滤跨命名空间的资源查询。

## 最佳实践/注意事项

- 字段选择器的可用字段因资源类型而异，使用前应先确认目标资源支持的字段列表。
- 字段选择器不支持逻辑 OR 运算符，复杂过滤需求应结合脚本或其他工具处理。
- 对于自定义资源，需要通过 CRD 的 `selectableFields` 显式声明可选择的字段。

## 参考链接

- [Field Selectors - Official Documentation](https://kubernetes.io/docs/concepts/overview/working-with-objects/field-selectors/)

## Related

- [[domain-17-system-foundation/知识字典/fundamentals/about-cgroup-v2.md|About cgroup v2（关于 cgroup v2）]]
- [[domain-17-system-foundation/知识字典/fundamentals/annotations.md|注解]]
- [[domain-17-system-foundation/知识字典/fundamentals/bpfman.md|bpfman eBPF 管理器]]


<!-- risk-assessed -->
