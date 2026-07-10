---
title: 自定义资源
description: '## 概述'
summary: '## 概述'
category: dictionary
tags:
- k8s
- glossary
- terminology
- etcd
- mysql
- rbac
- crd
- operator
- webhook
tier: supporting
created: '2026-05-23'
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 自定义资源 是什么
- 如何 自定义资源
trigger_keywords:
- 自定义资源
- dictionary
prerequisites:
- kubectl-basics
- cloud-provider-basics
- etcd-basics
- mysql-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 自定义资源

## 概述

自定义资源（Custom Resources）是 [[Kubernetes|Kubernetes]]es API|Kubernetes API]] 的扩展，允许用户在不修改 Kubernetes 核心代码的情况下，为集群添加新的资源类型。自定义资源可以通过动态注册在运行的集群中出现或消失，安装后用户可以像操作内置资源（如 Pod）一样使用 `kubectl` 来创建和访问它们。

## 核心概念/原理

- **资源（Resource）**：Kubernetes API 中的一个端点，存储特定类型的 API 对象集合（例如内置的 [[Pods|pods]] 资源存储 Pod 对象集合）。
- **自定义资源（Custom Resource）**：默认 Kubernetes 安装中不一定可用的 API 扩展，代表对特定 Kubernetes 安装的定制。如今，许多核心 Kubernetes 功能也是基于自定义资源构建的，使 Kubernetes 更加模块化。
- **自定义控制器（Custom Controller）**：单独使用自定义资源只能存储和检索结构化数据；当与自定义控制器结合时，自定义资源提供了真正的声明式 API。控制器负责将当前状态与声明的期望状态保持同步。
- **Operator 模式**：将自定义资源与自定义控制器相结合，用于将特定应用的领域知识编码到 Kubernetes API 扩展中。

## 关键机制或特性

- **动态注册**：自定义资源可在集群运行时动态注册和更新，独立于集群生命周期。
- **声明式 API**：用户声明期望状态，控制器负责持续调谐（reconcile）。这与命令式 API（直接指示服务器执行操作）形成对比。
- **字段选择器（Field Selectors）**：自 Kubernetes v1.32 起稳定支持，允许客户端根据自定义资源字段的值进行筛选。除 `metadata.name` 和 `metadata.namespace` 外，还可在 CRD 的 `spec.versions[*].selectableFields` 中声明其他可选字段。
- **两种实现方式**：
  - **CRD**：无需编程，由 API server 直接服务和存储。
  - **API Aggregation**：需要编写自定义 API server，提供更大的灵活性。

## 使用场景

- 需要为集群引入新的抽象层来管理应用或基础设施配置。
- 希望通过 `kubectl` 读写新类型资源，并在 Kubernetes UI 中展示。
- 需要构建自动化工具，监听新资源的更新并相应地创建、修改其他资源。
- 希望使用 Kubernetes API 约定（如 `.spec`、`.status`、`.metadata`）来封装一组受控资源。

## 最佳实践/注意事项

- **CRD vs. ConfigMap**：如果配置已有明确的文件格式（如 `mysql.cnf`）、主要用于 Pod 内程序消费、希望通过 Deployment 滚动更新文件时，优先使用 ConfigMap（敏感数据用 Secret）。如果需要 `kubectl` 顶级支持、构建自动化、使用 Kubernetes API 约定，则优先使用自定义资源。
- **是否聚合 API 的决策**：如果 API 是声明式的、需要 `kubectl` 和 UI 支持、资源自然按集群或命名空间范围划分，则考虑聚合到 Kubernetes API；否则可保持为独立 API。
- **避免数据存储滥用**：不要将自定义资源用作应用数据、终端用户数据或监控数据的存储，这会导致与 Kubernetes 过度耦合。
- **存储影响**：自定义资源占用 API server 存储空间，创建过多会压垮存储。资源按当前存储版本存入 etcd，更新时会使用定义的存储版本。
- **RBAC 授权**：新资源默认不会被现有 RBAC 角色授予访问权限（除 cluster-admin 或通配符规则外），需显式授权。

## 故障排查

| 症状 | 可能原因 | 排查步骤 |
|------|----------|----------|
| CRD 创建后 kubectl get 报错 | CRD spec 中 names/group 不正确 | `kubectl get crd <name> -o yaml` 检查 spec |
| CR 创建被拒绝 | CRD validation schema 不匹配 | 检查 CR YAML 与 CRD openAPIV3Schema |
| Controller 未处理 CR 事件 | Controller 未 watch 正确的 GVR | 检查 Controller 的 informer 配置和 RBAC |
| CRD 版本升级后旧 CR 不兼容 | 缺少 conversion webhook | 配置 CRD conversion strategy |

## 生产检查清单

- [ ] CRD 配置完整的 openAPIV3Schema validation
- [ ] 配置 additionalPrinterColumns 提升 kubectl 可读性
- [ ] 多版本 CRD 配置 conversion webhook
- [ ] Controller 配置正确的 RBAC
- [ ] 为 CRD 创建 RBAC ClusterRole 供用户使用

## 命令快速参考

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl delete`：删除资源（可由声明式清单重建）

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看 CRD
kubectl get crd

# 查看 CRD 详情
kubectl describe crd <crd-name>

# 查看自定义资源实例
kubectl get <resource-name> -A

# 删除 CRD（会级联删除所有 CR 实例）
kubectl delete crd <crd-name>
```
## 交叉引用

- [扩展 Kubernetes API](./extending-[[系统基础/知识字典/fundamentals/the-kubernetes-api.md|the-kubernetes-api]].md) — API 扩展总览
- [Operator 模式](./operator-pattern.md) — CRD + Controller 最佳实践
- [Admission Webhook](./admission-webhook-good-practices.md) — CR 验证与变更

## 参考链接

- https://kubernetes.io/docs/concepts/extend-kubernetes/api-extension/custom-resources/

## Related

- [[系统基础/知识字典/platform-engineering/api-group.md|Api Group]]
- [[系统基础/知识字典/platform-engineering/api-version.md|Api Version]]
- [[系统基础/知识字典/platform-engineering/kind.md|Kind]]
- [[系统基础/知识字典/platform-engineering/manifest.md|Manifest]]
- [[系统基础/知识字典/platform-engineering/custom-resource.md|Custom Resource]]


<!-- risk-assessed -->
