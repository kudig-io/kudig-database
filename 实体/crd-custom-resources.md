---
title: CRD (Custom Resource Definition)
description: CRD (Custom Resource Definition) — Kubernetes 生产运维知识库
summary: CRD (Custom Resource Definition) — Kubernetes 生产运维知识库
category: entities
tags:
- k8s
- crd
- extension
- custom-resource
- api
- etcd
- rbac
- operator
- webhook
- rag
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- CRD (Custom Resource Definition) 是什么
- 如何 CRD (Custom Resource Definition)
trigger_keywords:
- CRD
- Custom
- Resource
- Definition
prerequisites:
- kubectl-basics
- etcd-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# CRD (Custom Resource Definition)

> CRD 是 Kubernetes 的核心扩展机制，允许用户定义新的资源类型，使其成为 K8s API 的一等公民，支持 kubectl、RBAC、审计日志、GitOps 等所有原生工具。

## 基本信息

| 属性 | 值 |
|------|------|
| API 版本 | apiextensions.k8s.io/v1 |
| 作用域 | Cluster (CRD 本身) |
| 存储 | etcd |
| 验证 | OpenAPI v3 Schema |
| 扩展 | Conversion Webhook, Admission Webhook |

## CRD 规范详解

### 完整 CRD 示例

```yaml
apiVersion: apiextensions.k8s.io/v1
kind: CustomResourceDefinition
metadata:
  name: webapps.example.com
spec:
  group: example.com
  names:
    kind: WebApp
    plural: webapps
    singular: webapp
    shortNames:
    - wa
    categories:
    - all
  scope: Namespaced  # 或 Cluster
  versions:
  - name: v1
    served: true
    storage: true
    schema:
      openAPIV3Schema:
        type: object
        properties:
          spec:
            type: object
            properties:
              replicas:
                type: integer
                minimum: 1
                maximum: 100
                default: 1
              image:
                type: string
              port:
                type: integer
                default: 8080
              env:
                type: object
                additionalProperties:
                  type: string
            required:
            - image
          status:
            type: object
            properties:
              readyReplicas:
                type: integer
              conditions:
                type: array
                items:
                  type: object
                  properties:
                    type:
                      type: string
                    status:
                      type: string
                    reason:
                      type: string
                    message:
                      type: string
    subresources:
      status: {}   # 启用 /status 端点
      scale:       # 启用 /scale 端点
        specReplicasPath: .spec.replicas
        statusReplicasPath: .status.readyReplicas
    additionalPrinterColumns:
    - name: Replicas
      type: integer
      jsonPath: .spec.replicas
    - name: Ready
      type: integer
      jsonPath: .status.readyReplicas
    - name: Age
      type: date
      jsonPath: .metadata.creationTimestamp
  - name: v1alpha1
    served: true
    storage: false
    deprecated: true
    deprecationWarning: "v1alpha1 已废弃，请使用 v1"
    schema:
      openAPIV3Schema:
        type: object
        x-kubernetes-preserve-unknown-fields: true
```

### 关键字段说明

| 字段 | 说明 | 示例 |
|------|------|------|
| group | API 组名 | example.com |
| names.kind | 资源类型名 | WebApp |
| names.plural | 复数名 (URL 用) | webapps |
| names.shortNames | 缩写 | wa |
| scope | 作用域 | Namespaced / Cluster |
| versions[].served | 是否提供服务 | true/false |
| versions[].storage | 存储版本 (etcd) | true (只能一个) |
| subresources.status | 状态子资源 | 分离 spec/status 更新 |
| subresources.scale | 缩放子资源 | 支持 HPA |

## 版本管理

### 多版本共存

```
v1alpha1 (served=true, storage=false, deprecated=true)
v1beta1  (served=true, storage=false)
v1       (served=true, storage=true)  ← etcd 存储版本
```

### Conversion Webhook

```yaml
spec:
  conversion:
    strategy: Webhook
    webhook:
      clientConfig:
        service:
          namespace: default
          name: conversion-webhook
          path: /convert
      conversionReviewVersions:
      - v1
```

## 使用 CRD

### 创建自定义资源

```yaml
apiVersion: example.com/v1
kind: WebApp
metadata:
  name: my-app
  namespace: default
spec:
  replicas: 3
  image: nginx:1.25
  port: 80
  env:
    ENV: production
```

### 常用命令

```bash
# 🟢 查看 CRD
kubectl get crd
kubectl get crd webapps.example.com -o yaml

# 🟢 查看自定义资源
kubectl get webapps -A
kubectl get wa my-app -o yaml

# 🟡 创建/更新 CR
kubectl apply -f webapp.yaml

# 🔴 删除 CRD (会删除所有 CR!)
kubectl delete crd webapps.example.com

# 🟢 查看 CRD 状态
kubectl get crd webapps.example.com -o jsonpath='{.status.conditions}'

# 🟢 查看 CR 事件
kubectl describe wa my-app
```

## CRD 与 Operator 模式

```
CRD (定义资源类型)
    │
    ▼
Controller/Operator (监听 CR 变化)
    │
    ▼
Reconcile Loop (确保实际状态 = 期望状态)
    │
    ▼
更新 CR Status (报告当前状态)
```

### 何时使用 CRD

| 适合 | 不适合 |
|------|--------|
| 领域特定配置 | 简单配置 (用 ConfigMap) |
| 需要 K8s 工具链 | 不需要 K8s 生命周期管理 |
| 需要 RBAC 控制 | 临时数据 |
| 需要 GitOps 管理 | 大量二进制数据 |
| 需要审计日志 | 简单键值对 |
| 需要 Operator 自动化 | 一次性配置 |

## 故障排查

| 问题 | 原因 | 解决方案 |
|------|------|----------|
| CRD 未就绪 | Schema 错误 | 检查 openAPIV3Schema |
| CR 创建失败 | 验证不通过 | 检查字段类型/必填 |
| Conversion 失败 | Webhook 异常 | 检查 conversion webhook |
| CRD 删除卡住 | Finalizer 未清理 | 移除 finalizer |
| 版本迁移失败 | 存储版本冲突 | 检查 storage 标记 |

### 排查命令

```bash
# 🟢 检查 CRD 状态
kubectl get crd <name> -o jsonpath='{.status.conditions[*]}'

# 🟢 检查是否有 Finalizer 阻塞删除
kubectl get crd <name> -o jsonpath='{.metadata.finalizers}'

# 🟡 移除 Finalizer (危险!)
kubectl patch crd <name> -p '{"metadata":{"finalizers":[]}}' --type=merge

# 🟢 查看 API 资源发现
kubectl api-resources | grep example.com
```

## 生产最佳实践

1. **始终启用 status 子资源** - 分离 spec/status 更新权限
2. **使用 additionalPrinterColumns** - 提升 kubectl get 可读性
3. **设置 validation schema** - 防止无效配置
4. **版本演进策略** - v1alpha1 → v1beta1 → v1
5. **设置 default 值** - 减少用户配置负担
6. **使用 categories** - 方便 `kubectl get all` 查看
7. **CRD 版本管理** - 用 Git 管理 CRD 变更

## 检查清单

- [ ] 理解 CRD 规范关键字段
- [ ] 能编写带 Schema 验证的 CRD
- [ ] 理解多版本管理和 Conversion
- [ ] 掌握 status/scale 子资源
- [ ] 能排查 CRD 常见问题
- [ ] 理解 CRD 与 Operator 的关系
- [ ] 掌握生产最佳实践

## Related

- [[概念/控制器模式 × Operator 模式.md|控制器模式 × Operator 模式]]
- [[实体/kubernetes-api-versions-reference.md|Kubernetes API Versions Reference]]
- [[etcd]] — etcd
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[技能/develop-crd-operator.md|Develop CRD Operator]]
- [[概念/declarative-api.md|Declarative API]]
- [[概念/CRD × 可观测性.md|CRD × 可观测性]]

<!-- risk-assessed -->
