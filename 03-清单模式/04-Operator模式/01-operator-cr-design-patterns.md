---
title: Operator CRD 设计模式与最佳实践
description: CustomResourceDefinition 设计规范、字段建模、OpenAPI Schema 验证与 CEL 表达式
summary: CRD 设计最佳实践，包括 spec/status 分离、CEL 验证、版本管理及生产级 CRD 模式
category: manifests-patterns
tags:
- k8s
- manifests
- operator
- crd
- api-design
tier: core
created: '2026-07-11'
last_updated: 2026-07
difficulty: advanced
reading_level: advanced
audience:
- 平台工程师
- 开发工程师
- SRE
estimated_read_time: 12min
intent_queries:
- 如何设计 CRD
- CRD spec status 分离
- CRD CEL 验证规则
trigger_keywords:
- crd
- operator
- customresourcedefinition
- apidesign
prerequisites:
- kubectl-basics
- crd-basics
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
authors:
- name: KUDIG Team
  role: contributor
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。

# Operator CRD 设计模式与最佳实践

## 1. Spec/Status 分离原则

CRD 设计的黄金法则是 **spec 由用户写入、status 由控制器写出**，两者严格分离：

```yaml
apiVersion: apiextensions.k8s.io/v1
kind: CustomResourceDefinition
metadata:
  name: webapps.platform.example.com
spec:
  group: platform.example.com
  names:
    kind: WebApp
    plural: webapps
    singular: webapp
    shortNames: [wa]
  scope: Namespaced
  versions:
    - name: v1
      served: true
      storage: true
      schema:
        openAPIV3Schema:
          type: object
          required: [spec]
          properties:
            spec:
              type: object
              required: [image, replicas]
              properties:
                image:
                  type: string
                  description: "容器镜像地址"
                replicas:
                  type: integer
                  minimum: 1
                  maximum: 100
                  default: 1
                env:
                  type: array
                  x-kubernetes-list-type: map
                  x-kubernetes-list-map-keys: [name]
                  items:
                    type: object
                    required: [name, value]
                    properties:
                      name:
                        type: string
                      value:
                        type: string
            status:
              type: object
              properties:
                readyReplicas:
                  type: integer
                phase:
                  type: string
                  enum: [Pending, Running, Failed]
                observedGeneration:
                  type: integer
                  format: int64
      subresources:
        status: {}
        scale:
          specReplicasPath: .spec.replicas
          statusReplicasPath: .status.readyReplicas
```

## 2. CEL 验证规则模式

使用 CEL 实现跨字段验证，避免引入 ValidatingAdmissionWebhook：

```yaml
properties:
  spec:
    properties:
      minReplicas:
        type: integer
        minimum: 1
      maxReplicas:
        type: integer
        minimum: 1
      strategy:
        type: string
        enum: [RollingUpdate, BlueGreen]
      blueGreen:
        type: object
        properties:
          activeColor:
            type: string
            enum: [blue, green]
    x-kubernetes-validations:
      - rule: "self.maxReplicas >= self.minReplicas"
        message: "maxReplicas 必须大于等于 minReplicas"
      - rule: "self.strategy != 'BlueGreen' || has(self.blueGreen)"
        message: "BlueGreen 策略必须配置 blueGreen 参数"
      - rule: "!has(self.blueGreen) || self.blueGreen.activeColor in ['blue', 'green']"
        message: "activeColor 只能是 blue 或 green"
```

## 3. 不可变字段模式

对创建后不应修改的字段使用 transition rule：

```yaml
x-kubernetes-validations:
  - rule: "self.image == oldSelf.image"
    message: "image 字段创建后不可修改，请重新创建资源"
    fieldPath: ".spec.image"
```

## 4. 附加打印列

为 `kubectl get` 提供有意义的输出：

```yaml
additionalPrinterColumns:
  - name: Image
    type: string
    jsonPath: .spec.image
  - name: Replicas
    type: integer
    jsonPath: .spec.replicas
  - name: Ready
    type: integer
    jsonPath: .status.readyReplicas
  - name: Phase
    type: string
    jsonPath: .status.phase
  - name: Age
    type: date
    jsonPath: .metadata.creationTimestamp
```

## 5. 多版本共存模式

```yaml
versions:
  - name: v1
    served: true
    storage: true
  - name: v1beta1
    served: true
    storage: false
    deprecated: true
    deprecationWarning: "platform.example.com/v1beta1 已弃用，请使用 v1"
conversion:
  strategy: None  # 如果版本间字段完全兼容
```

## 6. 生产实践清单

| 实践 | 说明 |
|------|------|
| 始终定义 `status` 子资源 | 允许控制器独立更新状态 |
| 使用 `observedGeneration` | 标记控制器已处理到哪一代 |
| 设置 `preserveUnknownFields: false` | 强制 Schema 验证 |
| 为列表字段定义 list-type | atomic（整体替换）或 map（按键合并） |
| 启用 `scale` 子资源 | 支持 `kubectl scale` 命令 |

## Related

- [[03-清单模式/01-YAML参考/29-customresourcedefinition|CRD 完整参考]]
- [[03-清单模式/04-Operator模式/02-operator-reconciliation-patterns|调谐循环模式]]

## See Also

- [Kubebuilder CRD 设计指南](https://book.kubebuilder.io/reference/markers)
- [CEL 验证规则文档](https://kubernetes.io/docs/tasks/extend-kubernetes/custom-resources/custom-resource-definitions/#validation-rules)

<!-- risk-assessed -->
