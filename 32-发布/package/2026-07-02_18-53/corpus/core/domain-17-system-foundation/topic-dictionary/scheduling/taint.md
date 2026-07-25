---
title: 污点
description: Taint（污点）是应用在节点上的标记，表示该节点不应接受没有对应容忍度（Toleration）的 Pod。污点与容忍度配合工作，实现节点的调度控制。...
summary: Taint（污点）是应用在节点上的标记，表示该节点不应接受没有对应容忍度（Toleration）的 Pod。污点与容忍度配合工作，实现节点的调度控制。...
category: dictionary
tags:
- k8s
- glossary
- scheduling
- taint
tier: core
created: '2026-06-24'
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 污点 是什么
- Taint 详解
trigger_keywords:
- 污点
- Taint
- dictionary
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 污点

> **英文名**: Taint

## 概述

Taint（污点）是应用在节点上的标记，表示该节点不应接受没有对应容忍度（Toleration）的 Pod。污点与容忍度配合工作，实现节点的调度控制。

## 核心概念/原理

### 污点效果

- **NoSchedule**：新 Pod 不会调度到该节点（已运行的 Pod 不受影响）。
- **PreferNoSchedule**：尽量不调度新 Pod（软性约束）。
- **NoExecute**：驱逐已运行但没有对应容忍度的 Pod。

### 设置污点

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `kubectl taint nodes`：变更污点影响 Pod 调度

> **🔴 高风险操作警告**
>
> 下方命令属于不可逆或高影响操作，执行前请确认：
> - 已备份关键数据与配置
> - 处于批准的变更窗口期
> - 已获得相关责任人授权
> - 已准备回滚或恢复方案
> - 目标集群、Namespace、节点/资源名称正确无误

``` bash
# 🔴 高风险：可能造成数据丢失或服务中断，执行前需备份、变更审批与回滚方案
kubectl taint nodes node1 key1=value1:NoSchedule
```
### 常见系统污点

- `node.kubernetes.io/not-ready`：节点未就绪。
- `node.kubernetes.io/unreachable`：节点不可达。
- `node.kubernetes.io/memory-pressure`：内存压力。
- `node.kubernetes.io/disk-pressure`：磁盘压力。

## 关键机制或特性

- 控制平面节点默认有 `node-role.kubernetes.io/control-plane:NoSchedule` 污点。
- 节点问题（NotReady、内存/磁盘压力）会自动添加系统污点。
- `NoExecute` 支持 `tolerationSeconds` 设置驱逐延迟。

## 使用场景与最佳实践

- 为专用节点（如 GPU 节点）添加污点，只有特定工作负载才能调度。
- 使用 `PreferNoSchedule` 作为软性约束。
- 监控系统污点的添加和移除情况。

## 参考链接

- [Taint - Official Documentation](https://kubernetes.io/docs/concepts/scheduling-eviction/taint-and-toleration/)

## Related

- [[domain-17-system-foundation/知识字典/scheduling/affinity.md|Affinity]]
- [[domain-17-system-foundation/知识字典/scheduling/anti-affinity.md|Anti Affinity]]
- [[domain-17-system-foundation/知识字典/scheduling/toleration.md|Toleration]]
- [[domain-17-system-foundation/知识字典/scheduling/node-selector.md|Node Selector]]
- [[domain-17-system-foundation/知识字典/scheduling/resource-request.md|Resource Request]]


<!-- risk-assessed -->
