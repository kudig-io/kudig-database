# 29 - CustomResourceDefinition (CRD) YAML 配置参考

> **适用版本**: Kubernetes v1.25 - v1.32 | **最后更新**: 2026-02

**本文档全面覆盖 CustomResourceDefinition (CRD) 的 YAML 配置**,包括完整字段说明、OpenAPI v3 Schema 验证、CEL 表达式验证、多版本转换、生产实践案例等。

---

## 📋 目录

1. [CRD 基础概念](#1-crd-基础概念)
2. [完整字段说明](#2-完整字段说明)
3. [OpenAPI v3 Schema 详解](#3-openapi-v3-schema-详解)
4. [CEL 验证规则 (v1.25+)](#4-cel-验证规则-v125)
5. [多版本与转换](#5-多版本与转换)
6. [内部原理](#6-内部原理)
7. [生产案例](#7-生产案例)
8. [故障排查](#8-故障排查)

---

## 1. CRD 基础概念

### 1.1 什么是 CRD

CustomResourceDefinition (CRD) 是 Kubernetes 的扩展机制,允许用户定义自己的资源类型:

- **声明式 API 扩展**: 无需修改 API Server 源码即可添加新资源类型
- **原生 Kubernetes 体验**: 自定义资源与内置资源(Pod、Service 等)使用方式完全一致
- **Schema 验证**: 通过 OpenAPI v3 Schema 定义资源结构和验证规则
- **版本管理**: 支持多版本共存、自动转换、存储版本迁移

### 1.2 CRD vs APIService

| 特性 | CRD | APIService (聚合 API) |
|------|-----|----------------------|
| **实现复杂度** | 低(仅需 YAML 定义) | 高(需要独立 API Server) |
| **存储** | etcd | 自定义(可以是 etcd 或其他) |
| **验证** | OpenAPI Schema + CEL | 自定义逻辑 |
| **性能** | 高(直接由 kube-apiserver 处理) | 中(需要额外网络跳转) |
| **适用场景** | 简单配置型资源 | 复杂计算型资源 |

---

## 2. 完整字段说明

### 2.1 基础结构 YAML

```yaml
apiVersion: apiextensions.k8s.io/v1
kind: CustomResourceDefinition
metadata:
  # CRD 名称必须为 <plural>.<group> 格式
  name: myresources.example.com
  annotations:
    # 控制器版本标识(推荐)
    controller-gen.kubebuilder.io/version: v0.14.0
    # CRD 文档链接
    kubebuilder.io/documentation: "https://docs.example.com/myresource"
spec:
  # === 基础字段 ===
  
  # API 组名(与 name 后缀保持一致)
  group: example.com
  
  # 资源名称定义
  names:
    # 复数名称(用于 API 路径,如 /apis/example.com/v1/myresources)
    plural: myresources
    # 单数名称(用于显示,如 kubectl get myresource)
    singular: myresource
    # Kind 名称(用于 YAML 资源的 kind 字段)
    kind: MyResource
    # 短名称列表(kubectl 别名,如 kubectl get mr)
    shortNames:
      - mr
      - myres
    # 资源分类(用于 kubectl get <category>)
    categories:
      - all           # 加入 kubectl get all
      - myapp         # 自定义分类
    # ListKind(通常自动生成,无需手动指定)
    listKind: MyResourceList
  
  # 作用域: Namespaced(命名空间级) 或 Cluster(集群级)
  scope: Namespaced
  
  # === 版本定义(核心部分) ===
  versions:
    # --- v1 版本(当前存储版本) ---
    - name: v1
      # 是否通过 API 提供服务(默认 true)
      served: true
      # 是否为存储版本(有且仅有一个版本为 true)
      storage: true
      
      # Schema 定义(OpenAPI v3 格式)
      schema:
        openAPIV3Schema:
          type: object
          # 必需字段列表
          required:
            - spec
          properties:
            # 资源核心字段(metadata 由 Kubernetes 自动管理,无需定义)
            spec:
              type: object
              required:
                - replicas
              properties:
                replicas:
                  type: integer
                  minimum: 1
                  maximum: 100
                  default: 1
                  description: "副本数量"
                image:
                  type: string
                  pattern: '^[a-z0-9\-\.]+/[a-z0-9\-\.]+:[a-z0-9\-\.]+$'
                  description: "容器镜像"
                resources:
                  type: object
                  properties:
                    cpu:
                      type: string
                      pattern: '^[0-9]+m?$'
                    memory:
                      type: string
                      pattern: '^[0-9]+[MGT]i?$'
            # 状态字段(通常由控制器更新)
            status:
              type: object
              properties:
                phase:
                  type: string
                  enum: ["Pending", "Running", "Failed"]
                conditions:
                  type: array
                  items:
                    type: object
                    required: ["type", "status"]
                    properties:
                      type:
                        type: string
                      status:
                        type: string
                        enum: ["True", "False", "Unknown"]
                      lastTransitionTime:
                        type: string
                        format: date-time
                      reason:
                        type: string
                      message:
                        type: string
      
      # 子资源定义
      subresources:
        # 启用 status 子资源(/status 路径)
        status: {}
        # 启用 scale 子资源(kubectl scale 支持)
        scale:
          # spec.replicas 路径
          specReplicasPath: .spec.replicas
          # status.replicas 路径
          statusReplicasPath: .status.replicas
          # 可选: label selector 路径
          labelSelectorPath: .status.labelSelector
      
      # 自定义列(kubectl get 输出)
      additionalPrinterColumns:
        - name: Replicas        # 列名
          type: integer         # 类型: integer, string, boolean, number, date
          jsonPath: .spec.replicas  # JSON 路径
          description: "期望副本数"
          priority: 0           # 优先级: 0=默认显示, >0=需要 -o wide
        - name: Phase
          type: string
          jsonPath: .status.phase
          description: "当前阶段"
        - name: Age
          type: date
          jsonPath: .metadata.creationTimestamp
      
      # 可选字段过滤(v1.30+,允许 fieldSelector 按自定义字段过滤)
      selectableFields:
        - jsonPath: .spec.replicas
        - jsonPath: .status.phase
    
    # --- v1beta1 版本(旧版本,仅提供服务,不存储) ---
    - name: v1beta1
      served: true
      storage: false  # 非存储版本
      deprecated: true
      deprecationWarning: "example.com/v1beta1 is deprecated, use example.com/v1"
      schema:
        openAPIV3Schema:
          type: object
          properties:
            spec:
              type: object
              properties:
                count:  # 旧字段名(v1 中改为 replicas)
                  type: integer
            status:
              type: object
              x-kubernetes-preserve-unknown-fields: true  # 保留未知字段
      subresources:
        status: {}
      additionalPrinterColumns:
        - name: Count
          type: integer
          jsonPath: .spec.count
        - name: Age
          type: date
          jsonPath: .metadata.creationTimestamp
  
  # === 版本转换配置 ===
  conversion:
    # 转换策略: None(无转换) 或 Webhook(通过 Webhook 转换)
    strategy: Webhook
    webhook:
      # Webhook 服务端点
      clientConfig:
        service:
          namespace: myapp-system
          name: myapp-webhook-service
          path: /convert
          port: 443
        # CA 证书(用于验证 Webhook 服务端)
        caBundle: LS0tLS1CRUdJTiBDRVJUSUZJQ0FURS0tLS0t...
      # 支持的转换版本列表
      conversionReviewVersions:
        - v1       # 推荐使用 v1
        - v1beta1  # 向后兼容
  
  # === 保留字段(防止删除已存储的字段) ===
  preserveUnknownFields: false  # v1.16+ 默认 false(推荐)
```

---

## 3. OpenAPI v3 Schema 详解

### 3.1 基础类型

```yaml
schema:
  openAPIV3Schema:
    type: object
    properties:
      # 字符串类型
      stringField:
        type: string
        minLength: 1
        maxLength: 100
        pattern: '^[a-z0-9]([-a-z0-9]*[a-z0-9])?$'  # DNS 标签格式
        default: "default-value"
        enum: ["value1", "value2", "value3"]
      
      # 整数类型
      intField:
        type: integer
        format: int32    # int32 或 int64
        minimum: 0
        maximum: 100
        exclusiveMinimum: true  # minimum 不包含边界
        multipleOf: 5    # 必须是 5 的倍数
      
      # 浮点数类型
      floatField:
        type: number
        format: double   # float 或 double
        minimum: 0.0
        maximum: 1.0
      
      # 布尔类型
      boolField:
        type: boolean
        default: false
      
      # 日期时间类型
      timestampField:
        type: string
        format: date-time  # RFC3339 格式
      
      # 字节数组(Base64 编码)
      bytesField:
        type: string
        format: byte
```

### 3.2 复杂类型

```yaml
properties:
  # 对象类型
  objectField:
    type: object
    required: ["key1", "key2"]
    properties:
      key1:
        type: string
      key2:
        type: integer
    # 额外属性限制
    additionalProperties: false  # 不允许未定义的字段
  
  # 数组类型
  arrayField:
    type: array
    minItems: 1
    maxItems: 10
    uniqueItems: true  # 元素唯一性
    items:
      type: string
  
  # Map 类型(key 为字符串)
  mapField:
    type: object
    additionalProperties:
      type: string
  
  # 嵌套对象数组
  nestedArray:
    type: array
    items:
      type: object
      required: ["name"]
      properties:
        name:
          type: string
        value:
          type: string
```

### 3.3 特殊字段

```yaml
properties:
  # IntOrString 类型(Kubernetes 常用,如 port: 80 或 "http")
  portField:
    x-kubernetes-int-or-string: true
    anyOf:
      - type: integer
      - type: string
  
  # 保留未知字段(部分字段不验证)
  dynamicConfig:
    type: object
    x-kubernetes-preserve-unknown-fields: true
  
  # 嵌入资源(如 PodTemplateSpec)
  template:
    type: object
    x-kubernetes-embedded-resource: true
    properties:
      metadata:
        type: object
      spec:
        type: object
  
  # Map 列表(用于 kubectl apply 的 merge 策略)
  containers:
    type: array
    x-kubernetes-list-type: map
    x-kubernetes-list-map-keys:
      - name
    items:
      type: object
      required: ["name"]
      properties:
        name:
          type: string
        image:
          type: string
```

### 3.4 默认值与示例

```yaml
properties:
  config:
    type: object
    # 默认值(v1.17+)
    default:
      replicas: 1
      image: "nginx:latest"
    properties:
      replicas:
        type: integer
        default: 1
      image:
        type: string
        default: "nginx:latest"
    # 示例值(仅用于文档)
    example:
      replicas: 3
      image: "nginx:1.21"
```

---

## 4. CEL 验证规则 (v1.25+)

### 4.1 CEL 基础语法

Common Expression Language (CEL) 提供比 OpenAPI Schema 更强大的验证能力:

```yaml
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
          maxReplicas:
            type: integer
            minimum: 1
        # CEL 验证规则
        x-kubernetes-validations:
          # 规则 1: maxReplicas >= replicas
          - rule: "self.maxReplicas >= self.replicas"
            message: "maxReplicas 必须大于或等于 replicas"
          
          # 规则 2: replicas 必须是偶数(如果启用 HA)
          - rule: "!has(self.ha) || !self.ha || self.replicas % 2 == 0"
            message: "HA 模式下 replicas 必须为偶数"
            fieldPath: ".spec.replicas"  # 错误关联到特定字段
```

### 4.2 CEL 内置函数

```yaml
x-kubernetes-validations:
  # 字符串操作
  - rule: "self.name.startsWith('app-')"
    message: "name 必须以 app- 开头"
  
  - rule: "self.name.matches('^[a-z0-9-]+$')"
    message: "name 只能包含小写字母、数字和连字符"
  
  - rule: "self.email.contains('@')"
    message: "email 必须包含 @"
  
  # 数组操作
  - rule: "self.ports.all(p, p > 1024 && p < 65535)"
    message: "所有端口必须在 1024-65535 范围内"
  
  - rule: "self.tags.exists(t, t == 'production')"
    message: "必须包含 production 标签"
  
  - rule: "self.items.size() > 0"
    message: "items 不能为空"
  
  # 数值比较
  - rule: "self.cpu.matches('^[0-9]+m?$') && int(self.cpu.replace('m', '')) >= 100"
    message: "CPU 请求至少为 100m"
  
  # 逻辑运算
  - rule: "self.enabled == true && has(self.config)"
    message: "启用时必须提供 config"
  
  # 可选字段检查
  - rule: "!has(self.optional) || self.optional.value > 0"
    message: "如果提供 optional,其 value 必须大于 0"
```

### 4.3 Transition Rules (变更验证)

```yaml
properties:
  spec:
    type: object
    properties:
      immutableField:
        type: string
      decreaseOnlyField:
        type: integer
    x-kubernetes-validations:
      # 字段不可变(创建后不能修改)
      - rule: "self.immutableField == oldSelf.immutableField"
        message: "immutableField 创建后不可修改"
      
      # 字段只能减少不能增加
      - rule: "self.decreaseOnlyField <= oldSelf.decreaseOnlyField"
        message: "decreaseOnlyField 只能减少"
      
      # 删除保护(如果引用了其他资源,不能删除)
      - rule: "!has(oldSelf.ref) || has(self.ref)"
        message: "不能删除 ref 字段"
```

### 4.4 高级 CEL 示例

```yaml
properties:
  spec:
    type: object
    properties:
      schedule:
        type: object
        properties:
          type:
            type: string
            enum: ["cron", "interval"]
          cron:
            type: string
          intervalSeconds:
            type: integer
        x-kubernetes-validations:
          # 条件必填字段
          - rule: "self.type == 'cron' ? has(self.cron) : has(self.intervalSeconds)"
            message: "cron 类型必须提供 cron 字段,interval 类型必须提供 intervalSeconds"
          
          # Cron 表达式验证(简化版)
          - rule: "self.type != 'cron' || self.cron.matches('^(\\*|[0-9]+)( (\\*|[0-9]+)){4}$')"
            message: "无效的 cron 表达式"
      
      resources:
        type: object
        properties:
          requests:
            type: object
            additionalProperties:
              x-kubernetes-int-or-string: true
          limits:
            type: object
            additionalProperties:
              x-kubernetes-int-or-string: true
        x-kubernetes-validations:
          # limits >= requests
          - rule: |
              !has(self.limits) || !has(self.requests) ||
              (has(self.requests.cpu) && has(self.limits.cpu) ?
                int(self.limits.cpu.replace('m', '')) >= int(self.requests.cpu.replace('m', '')) : true) &&
              (has(self.requests.memory) && has(self.limits.memory) ?
                int(self.limits.memory.replace(/[MGT]i?$/, '')) >= int(self.requests.memory.replace(/[MGT]i?$/, '')) : true)
            message: "limits 必须大于或等于 requests"
```

---

## 5. 多版本与转换

### 5.1 版本策略

```yaml
versions:
  # 当前稳定版本(存储版本)
  - name: v1
    served: true
    storage: true    # 唯一存储版本
  
  # 下一版本(Beta,已服务但未存储)
  - name: v2beta1
    served: true
    storage: false
  
  # 旧版本(仅保持兼容,已标记弃用)
  - name: v1alpha1
    served: true     # 可设为 false 停止服务
    storage: false
    deprecated: true
    deprecationWarning: "v1alpha1 将在 v2.0 中移除,请迁移到 v1"
```

### 5.2 Webhook 转换器

**Conversion Webhook 配置:**

```yaml
conversion:
  strategy: Webhook
  webhook:
    clientConfig:
      service:
        namespace: crd-system
        name: crd-conversion-webhook
        path: /convert
        port: 443
      # CA 证书(用于 TLS 验证)
      caBundle: LS0tLS1CRUdJTi0...
    conversionReviewVersions:
      - v1       # 推荐
      - v1beta1  # 向后兼容
```

**Webhook 服务端实现(Go 示例):**

```go
// ConversionReview 请求格式
type ConversionReview struct {
    Request  *ConversionRequest  `json:"request"`
    Response *ConversionResponse `json:"response"`
}

type ConversionRequest struct {
    UID               string                `json:"uid"`
    DesiredAPIVersion string                `json:"desiredAPIVersion"`
    Objects           []runtime.RawExtension `json:"objects"`
}

// Webhook Handler
func handleConvert(w http.ResponseWriter, r *http.Request) {
    var review ConversionReview
    json.NewDecoder(r.Body).Decode(&review)
    
    // 转换逻辑
    convertedObjects := []runtime.RawExtension{}
    for _, obj := range review.Request.Objects {
        converted := convertObject(obj, review.Request.DesiredAPIVersion)
        convertedObjects = append(convertedObjects, converted)
    }
    
    // 返回响应
    review.Response = &ConversionResponse{
        UID:              review.Request.UID,
        ConvertedObjects: convertedObjects,
        Result:           metav1.Status{Status: "Success"},
    }
    json.NewEncoder(w).Encode(review)
}
```

### 5.3 存储版本迁移

当更改存储版本时(如 v1beta1 → v1),需要迁移 etcd 中的数据:

```bash
# 1. 更新 CRD,将新版本设为 storage: true
kubectl apply -f crd-v2.yaml

# 2. 触发存储迁移(读取并重写所有对象)
kubectl get myresources --all-namespaces -o json | \
  kubectl replace -f -

# 3. 验证存储版本
kubectl get myresources -o jsonpath='{.items[*].metadata.annotations.kubectl\.kubernetes\.io/last-applied-configuration}' | \
  jq '.apiVersion'
```

**使用 StorageVersionMigration (v1.30 Alpha):**

```yaml
apiVersion: migration.k8s.io/v1alpha1
kind: StorageVersionMigration
metadata:
  name: myresource-migration
spec:
  resource:
    group: example.com
    version: v1      # 目标存储版本
    resource: myresources
```

---

## 6. 内部原理

### 6.1 CRD 注册流程

```
┌─────────────────────────────────────────────────────────────────┐
│ 1. kubectl apply -f crd.yaml                                    │
└────────────────────┬────────────────────────────────────────────┘
                     ▼
┌─────────────────────────────────────────────────────────────────┐
│ 2. kube-apiserver 接收 CRD 创建请求                             │
│    - apiextensions-apiserver 处理 CRD 资源                       │
│    - 验证 CRD 定义(group, names, schema 等)                      │
└────────────────────┬────────────────────────────────────────────┘
                     ▼
┌─────────────────────────────────────────────────────────────────┐
│ 3. 存储到 etcd                                                  │
│    Key: /registry/apiextensions.k8s.io/customresourcedefinitions│
│         /myresources.example.com                                │
└────────────────────┬────────────────────────────────────────────┘
                     ▼
┌─────────────────────────────────────────────────────────────────┐
│ 4. 动态注册 RESTful API 路径                                    │
│    - GET/POST /apis/example.com/v1/namespaces/{ns}/myresources  │
│    - GET/PUT/PATCH/DELETE /apis/example.com/v1/namespaces/{ns}/│
│      myresources/{name}                                         │
│    - GET/PUT/PATCH /apis/example.com/v1/namespaces/{ns}/       │
│      myresources/{name}/status (如果启用 status 子资源)         │
└────────────────────┬────────────────────────────────────────────┘
                     ▼
┌─────────────────────────────────────────────────────────────────┐
│ 5. CRD Established (可用状态)                                   │
│    - CRD Status: Established = True                             │
│    - kubectl get crd myresources.example.com -o jsonpath='...'  │
└─────────────────────────────────────────────────────────────────┘
```

**关键组件:**

- **apiextensions-apiserver**: 内置在 kube-apiserver 中,专门处理 CRD 资源
- **CRDRegistrationController**: 监听 CRD 变更,动态注册/注销 API 路径
- **CustomResourceDefinitionStorageVersion**: 管理存储版本

### 6.2 Schema 验证引擎

```
客户端请求 (kubectl/API)
    ▼
┌─────────────────────────────────────────────────────────────────┐
│ 1. OpenAPI Schema 验证 (Structural Schema)                      │
│    - 类型检查 (type, format)                                    │
│    - 约束检查 (minimum, maximum, pattern, enum)                 │
│    - 必填字段 (required)                                        │
│    - 默认值填充 (default) - Server-Side Apply                   │
└────────────────────┬────────────────────────────────────────────┘
                     ▼
┌─────────────────────────────────────────────────────────────────┐
│ 2. CEL 验证 (x-kubernetes-validations) - v1.25+                │
│    - 自定义业务逻辑验证                                         │
│    - 跨字段关联验证                                             │
│    - Transition Rules (oldSelf 变更验证)                        │
└────────────────────┬────────────────────────────────────────────┘
                     ▼
┌─────────────────────────────────────────────────────────────────┐
│ 3. Admission Webhooks (可选)                                    │
│    - ValidatingAdmissionWebhook                                 │
│    - MutatingAdmissionWebhook                                   │
└────────────────────┬────────────────────────────────────────────┘
                     ▼
┌─────────────────────────────────────────────────────────────────┐
│ 4. 存储到 etcd                                                  │
│    - 使用存储版本 (storage: true)                               │
│    - 自动版本转换 (如果请求版本 ≠ 存储版本)                    │
└─────────────────────────────────────────────────────────────────┘
```

### 6.3 版本转换机制

**场景**: 用户请求 v1,但 etcd 存储为 v2 (或反之)

```
用户请求 v1 资源
    ▼
┌─────────────────────────────────────────────────────────────────┐
│ 1. kube-apiserver 检测版本不匹配                                │
│    - 请求版本: v1                                               │
│    - 存储版本: v2                                               │
└────────────────────┬────────────────────────────────────────────┘
                     ▼
┌─────────────────────────────────────────────────────────────────┐
│ 2. 调用 Conversion Webhook                                      │
│    POST https://webhook-service/convert                         │
│    Body:                                                        │
│      desiredAPIVersion: example.com/v1                          │
│      objects: [ { apiVersion: v2, ... } ]                       │
└────────────────────┬────────────────────────────────────────────┘
                     ▼
┌─────────────────────────────────────────────────────────────────┐
│ 3. Webhook 执行转换逻辑                                         │
│    - v2 → v1: spec.replicas = spec.desiredReplicas             │
│    - v1 → v2: spec.desiredReplicas = spec.replicas             │
└────────────────────┬────────────────────────────────────────────┘
                     ▼
┌─────────────────────────────────────────────────────────────────┐
│ 4. 返回转换后的 v1 资源给用户                                   │
└─────────────────────────────────────────────────────────────────┘
```

**None 策略(无转换):**

如果 `conversion.strategy: None`,则不同版本之间**不共享数据**:

- v1 和 v2 是完全独立的资源
- `kubectl get myresource -o yaml` 只返回当前请求的版本
- 适用于向后不兼容的版本变更

---

## 7. 生产案例

### 7.1 简单 CRD - 数据库实例

```yaml
apiVersion: apiextensions.k8s.io/v1
kind: CustomResourceDefinition
metadata:
  name: databases.db.example.com
spec:
  group: db.example.com
  names:
    plural: databases
    singular: database
    kind: Database
    shortNames: [db]
    categories: [all]
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
              required: [engine, version, storageGB]
              properties:
                # 数据库引擎
                engine:
                  type: string
                  enum: [mysql, postgresql, mongodb]
                  description: "数据库引擎类型"
                
                # 版本号
                version:
                  type: string
                  pattern: '^\d+\.\d+(\.\d+)?$'
                  description: "数据库版本"
                
                # 存储大小(GB)
                storageGB:
                  type: integer
                  minimum: 10
                  maximum: 1000
                  description: "存储空间(GB)"
                
                # 副本配置
                replicas:
                  type: object
                  default:
                    enabled: false
                  properties:
                    enabled:
                      type: boolean
                      default: false
                    count:
                      type: integer
                      minimum: 1
                      maximum: 5
                      default: 1
                
                # 备份配置
                backup:
                  type: object
                  properties:
                    enabled:
                      type: boolean
                      default: true
                    schedule:
                      type: string
                      pattern: '^(@(annually|yearly|monthly|weekly|daily|hourly))|((\*|[0-5]?\d)( (\*|[01]?\d|2[0-3])){4})$'
                      default: "0 2 * * *"  # 每天凌晨 2 点
                    retentionDays:
                      type: integer
                      minimum: 1
                      maximum: 365
                      default: 7
              
              # CEL 验证
              x-kubernetes-validations:
                # 启用副本时必须指定数量
                - rule: "!self.replicas.enabled || has(self.replicas.count)"
                  message: "启用副本时必须指定 replicas.count"
                
                # MySQL 8.0+ 才支持副本
                - rule: |
                    self.engine != 'mysql' || !self.replicas.enabled ||
                    double(self.version) >= 8.0
                  message: "MySQL 副本功能需要 8.0 或更高版本"
                
                # 大存储空间推荐启用备份
                - rule: "self.storageGB < 100 || self.backup.enabled"
                  message: "存储空间超过 100GB 建议启用备份"
            
            status:
              type: object
              properties:
                phase:
                  type: string
                  enum: [Pending, Creating, Running, Failed, Deleting]
                endpoint:
                  type: string
                  description: "数据库连接端点"
                conditions:
                  type: array
                  x-kubernetes-list-type: map
                  x-kubernetes-list-map-keys: [type]
                  items:
                    type: object
                    required: [type, status]
                    properties:
                      type:
                        type: string
                      status:
                        type: string
                        enum: [True, False, Unknown]
                      lastTransitionTime:
                        type: string
                        format: date-time
                      reason:
                        type: string
                      message:
                        type: string
      
      subresources:
        status: {}
      
      additionalPrinterColumns:
        - name: Engine
          type: string
          jsonPath: .spec.engine
        - name: Version
          type: string
          jsonPath: .spec.version
        - name: Storage
          type: integer
          jsonPath: .spec.storageGB
          description: "存储空间(GB)"
        - name: Phase
          type: string
          jsonPath: .status.phase
        - name: Endpoint
          type: string
          jsonPath: .status.endpoint
          priority: 1  # -o wide 才显示
        - name: Age
          type: date
          jsonPath: .metadata.creationTimestamp
```

**使用示例:**

```yaml
apiVersion: db.example.com/v1
kind: Database
metadata:
  name: my-mysql
  namespace: production
spec:
  engine: mysql
  version: "8.0.32"
  storageGB: 200
  replicas:
    enabled: true
    count: 3
  backup:
    enabled: true
    schedule: "0 3 * * *"  # 每天凌晨 3 点
    retentionDays: 30
```

### 7.2 多版本 CRD - 应用部署

```yaml
apiVersion: apiextensions.k8s.io/v1
kind: CustomResourceDefinition
metadata:
  name: applications.app.example.com
spec:
  group: app.example.com
  names:
    plural: applications
    singular: application
    kind: Application
    shortNames: [app]
  scope: Namespaced
  
  versions:
    # === v2 版本(当前推荐) ===
    - name: v2
      served: true
      storage: true
      schema:
        openAPIV3Schema:
          type: object
          required: [spec]
          properties:
            spec:
              type: object
              required: [image, deployment]
              properties:
                # 容器镜像
                image:
                  type: string
                  pattern: '^[a-z0-9\-\.:/]+$'
                
                # 部署策略(新增字段)
                deployment:
                  type: object
                  required: [replicas, strategy]
                  properties:
                    replicas:
                      type: integer
                      minimum: 1
                      maximum: 100
                    strategy:
                      type: string
                      enum: [RollingUpdate, Recreate, BlueGreen, Canary]
                      default: RollingUpdate
                    # 滚动更新配置
                    rollingUpdate:
                      type: object
                      properties:
                        maxSurge:
                          x-kubernetes-int-or-string: true
                          default: "25%"
                        maxUnavailable:
                          x-kubernetes-int-or-string: true
                          default: "25%"
                    # 金丝雀发布配置(v2 新增)
                    canary:
                      type: object
                      properties:
                        steps:
                          type: array
                          items:
                            type: object
                            properties:
                              weight:
                                type: integer
                                minimum: 0
                                maximum: 100
                              pause:
                                type: string  # duration: 5m, 1h
                
                # 资源配置
                resources:
                  type: object
                  properties:
                    cpu:
                      type: string
                      pattern: '^[0-9]+m?$'
                      default: "100m"
                    memory:
                      type: string
                      pattern: '^[0-9]+[MGT]i?$'
                      default: "128Mi"
                
                # 健康检查
                healthCheck:
                  type: object
                  properties:
                    path:
                      type: string
                      default: "/health"
                    port:
                      type: integer
                      minimum: 1
                      maximum: 65535
                      default: 8080
                    initialDelaySeconds:
                      type: integer
                      minimum: 0
                      default: 10
              
              # CEL 验证
              x-kubernetes-validations:
                # 金丝雀策略必须配置 steps
                - rule: |
                    self.deployment.strategy != 'Canary' ||
                    has(self.deployment.canary) && has(self.deployment.canary.steps)
                  message: "金丝雀策略必须配置 deployment.canary.steps"
                
                # 蓝绿策略不允许配置 rollingUpdate
                - rule: |
                    self.deployment.strategy != 'BlueGreen' ||
                    !has(self.deployment.rollingUpdate)
                  message: "蓝绿策略不支持 rollingUpdate 配置"
            
            status:
              type: object
              properties:
                phase:
                  type: string
                availableReplicas:
                  type: integer
                conditions:
                  type: array
                  items:
                    type: object
                    required: [type, status]
                    properties:
                      type:
                        type: string
                      status:
                        type: string
                      lastTransitionTime:
                        type: string
                        format: date-time
                      message:
                        type: string
      
      subresources:
        status: {}
        scale:
          specReplicasPath: .spec.deployment.replicas
          statusReplicasPath: .status.availableReplicas
      
      additionalPrinterColumns:
        - name: Image
          type: string
          jsonPath: .spec.image
        - name: Replicas
          type: integer
          jsonPath: .spec.deployment.replicas
        - name: Strategy
          type: string
          jsonPath: .spec.deployment.strategy
        - name: Phase
          type: string
          jsonPath: .status.phase
        - name: Age
          type: date
          jsonPath: .metadata.creationTimestamp
    
    # === v1 版本(旧版本,仅兼容) ===
    - name: v1
      served: true
      storage: false
      deprecated: true
      deprecationWarning: "app.example.com/v1 已弃用,请迁移到 v2(新增金丝雀/蓝绿策略)"
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
                replicas:  # v1 直接在 spec 下
                  type: integer
                  minimum: 1
                  maximum: 100
                strategy:  # v1 只支持 RollingUpdate/Recreate
                  type: string
                  enum: [RollingUpdate, Recreate]
                  default: RollingUpdate
                resources:
                  type: object
                  properties:
                    cpu:
                      type: string
                    memory:
                      type: string
            status:
              type: object
              x-kubernetes-preserve-unknown-fields: true
      
      subresources:
        status: {}
      
      additionalPrinterColumns:
        - name: Image
          type: string
          jsonPath: .spec.image
        - name: Replicas
          type: integer
          jsonPath: .spec.replicas
        - name: Age
          type: date
          jsonPath: .metadata.creationTimestamp
  
  # Webhook 版本转换
  conversion:
    strategy: Webhook
    webhook:
      clientConfig:
        service:
          namespace: app-system
          name: app-webhook
          path: /convert
          port: 443
        caBundle: LS0tLS1CRUdJTi0tLS0t...
      conversionReviewVersions: [v1, v1beta1]
```

**Webhook 转换逻辑(伪代码):**

```go
func convertV1ToV2(v1obj *V1Application) *V2Application {
    return &V2Application{
        Spec: V2Spec{
            Image: v1obj.Spec.Image,
            Deployment: Deployment{
                Replicas: v1obj.Spec.Replicas,  // v1.replicas → v2.deployment.replicas
                Strategy: v1obj.Spec.Strategy,
            },
            Resources: v1obj.Spec.Resources,
        },
    }
}

func convertV2ToV1(v2obj *V2Application) *V1Application {
    strategy := v2obj.Spec.Deployment.Strategy
    // v2 的 Canary/BlueGreen 策略在 v1 中降级为 RollingUpdate
    if strategy == "Canary" || strategy == "BlueGreen" {
        strategy = "RollingUpdate"
    }
    
    return &V1Application{
        Spec: V1Spec{
            Image:    v2obj.Spec.Image,
            Replicas: v2obj.Spec.Deployment.Replicas,  // v2.deployment.replicas → v1.replicas
            Strategy: strategy,
            Resources: v2obj.Spec.Resources,
        },
    }
}
```

### 7.3 CEL 高级验证 - CI/CD Pipeline

```yaml
apiVersion: apiextensions.k8s.io/v1
kind: CustomResourceDefinition
metadata:
  name: pipelines.cicd.example.com
spec:
  group: cicd.example.com
  names:
    plural: pipelines
    singular: pipeline
    kind: Pipeline
    shortNames: [pl]
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
              required: [stages]
              properties:
                # 阶段列表
                stages:
                  type: array
                  minItems: 1
                  x-kubernetes-list-type: atomic
                  items:
                    type: object
                    required: [name, steps]
                    properties:
                      name:
                        type: string
                        minLength: 1
                        maxLength: 63
                        pattern: '^[a-z0-9]([-a-z0-9]*[a-z0-9])?$'
                      # 并行执行
                      parallel:
                        type: boolean
                        default: false
                      # 步骤列表
                      steps:
                        type: array
                        minItems: 1
                        items:
                          type: object
                          required: [name, action]
                          properties:
                            name:
                              type: string
                            action:
                              type: string
                              enum: [build, test, deploy, approve]
                            # 条件执行
                            when:
                              type: string
                              enum: [always, success, failure]
                              default: success
                            # 超时设置
                            timeout:
                              type: string
                              pattern: '^[0-9]+(s|m|h)$'
                              default: "10m"
                            # 重试策略
                            retry:
                              type: object
                              properties:
                                attempts:
                                  type: integer
                                  minimum: 1
                                  maximum: 5
                                  default: 1
                                backoff:
                                  type: string
                                  pattern: '^[0-9]+(s|m)$'
                                  default: "10s"
              
              # === CEL 验证规则 ===
              x-kubernetes-validations:
                # 1. Stage 名称唯一性
                - rule: "self.stages.all(s, self.stages.filter(x, x.name == s.name).size() == 1)"
                  message: "Stage 名称必须唯一"
                
                # 2. Deploy 阶段必须在 Test 阶段之后
                - rule: |
                    !self.stages.exists(s, s.steps.exists(st, st.action == 'deploy')) ||
                    (self.stages.map(s, s.steps.exists(st, st.action == 'test')).fold(0, (acc, found) =>
                      found ? acc + 1 : acc) > 0 &&
                     self.stages.indexOf(self.stages.filter(s, s.steps.exists(st, st.action == 'test'))[0]) <
                     self.stages.indexOf(self.stages.filter(s, s.steps.exists(st, st.action == 'deploy'))[0]))
                  message: "Deploy 阶段必须在 Test 阶段之后"
                  fieldPath: ".spec.stages"
                
                # 3. Approve 步骤不能在并行阶段中
                - rule: |
                    !self.stages.exists(s,
                      s.parallel == true &&
                      s.steps.exists(st, st.action == 'approve')
                    )
                  message: "Approve 步骤不能在并行阶段中执行"
                
                # 4. 每个 Stage 最多 10 个步骤
                - rule: "self.stages.all(s, s.steps.size() <= 10)"
                  message: "每个 Stage 最多包含 10 个步骤"
                
                # 5. 第一个 Stage 必须包含 Build 或 Test 步骤
                - rule: |
                    self.stages[0].steps.exists(st,
                      st.action == 'build' || st.action == 'test'
                    )
                  message: "第一个 Stage 必须包含 Build 或 Test 步骤"
                
                # 6. 超时时间合理性检查
                - rule: |
                    self.stages.all(s, s.steps.all(st,
                      int(st.timeout.replace(/[smh]/, '')) <= 3600
                    ))
                  message: "步骤超时时间不能超过 1 小时"
                
                # 7. 重试次数限制(Deploy 步骤限制更严格)
                - rule: |
                    self.stages.all(s, s.steps.all(st,
                      !has(st.retry) ||
                      (st.action == 'deploy' ? st.retry.attempts <= 2 : st.retry.attempts <= 5)
                    ))
                  message: "Deploy 步骤最多重试 2 次,其他步骤最多 5 次"
            
            status:
              type: object
              properties:
                phase:
                  type: string
                  enum: [Pending, Running, Succeeded, Failed, Cancelled]
                startTime:
                  type: string
                  format: date-time
                completionTime:
                  type: string
                  format: date-time
                currentStage:
                  type: string
                stageStatuses:
                  type: array
                  items:
                    type: object
                    properties:
                      name:
                        type: string
                      phase:
                        type: string
                      startTime:
                        type: string
                        format: date-time
                      completionTime:
                        type: string
                        format: date-time
      
      subresources:
        status: {}
      
      additionalPrinterColumns:
        - name: Phase
          type: string
          jsonPath: .status.phase
        - name: Current-Stage
          type: string
          jsonPath: .status.currentStage
        - name: Started
          type: date
          jsonPath: .status.startTime
        - name: Age
          type: date
          jsonPath: .metadata.creationTimestamp
```

**使用示例:**

```yaml
apiVersion: cicd.example.com/v1
kind: Pipeline
metadata:
  name: app-release-pipeline
spec:
  stages:
    # Stage 1: 构建
    - name: build
      steps:
        - name: compile
          action: build
          timeout: "15m"
        - name: unit-test
          action: test
          timeout: "10m"
          retry:
            attempts: 3
            backoff: "30s"
    
    # Stage 2: 测试(并行)
    - name: test
      parallel: true
      steps:
        - name: integration-test
          action: test
          timeout: "20m"
        - name: e2e-test
          action: test
          timeout: "30m"
        - name: security-scan
          action: test
          timeout: "15m"
    
    # Stage 3: 审批
    - name: approve
      steps:
        - name: manual-approval
          action: approve
          timeout: "24h"
    
    # Stage 4: 部署
    - name: deploy
      steps:
        - name: deploy-staging
          action: deploy
          timeout: "10m"
          retry:
            attempts: 2
            backoff: "1m"
        - name: smoke-test
          action: test
          when: success
        - name: deploy-production
          action: deploy
          timeout: "15m"
          when: success
```

---

## 8. 故障排查

### 8.1 CRD 无法创建

**症状**: `kubectl apply -f crd.yaml` 失败

```bash
# 检查 CRD 定义
kubectl apply -f crd.yaml --dry-run=server -v=8

# 常见错误
# 1. 名称不匹配
Error: metadata.name must be spec.names.plural + "." + spec.group

# 2. Schema 无效
Error: spec.versions[0].schema.openAPIV3Schema: Invalid value: ...: must be a structural schema

# 3. 多个存储版本
Error: spec.versions: Invalid value: ...: must have exactly one version marked as storage version
```

**解决方案:**

```bash
# 验证 CRD 结构
kubectl apply -f crd.yaml --validate=true

# 检查 API Server 日志
kubectl logs -n kube-system kube-apiserver-xxx | grep -i customresourcedefinition
```

### 8.2 CR 创建失败(Schema 验证)

**症状**: CustomResource 无法创建,提示字段验证错误

```bash
# 示例错误
Error from server (Invalid): error when creating "cr.yaml": 
  Database.db.example.com "test" is invalid:
  spec.storageGB: Invalid value: 5: spec.storageGB in body should be greater than or equal to 10
```

**调试步骤:**

```bash
# 1. 查看 CRD Schema
kubectl get crd databases.db.example.com -o jsonpath='{.spec.versions[?(@.storage==true)].schema.openAPIV3Schema}' | jq

# 2. 使用 --dry-run 测试
kubectl apply -f cr.yaml --dry-run=server -v=8

# 3. 检查 CEL 验证规则
kubectl get crd databases.db.example.com -o jsonpath='{.spec.versions[0].schema.openAPIV3Schema.properties.spec.x-kubernetes-validations}'
```

### 8.3 版本转换失败

**症状**: Webhook 转换错误

```bash
# 错误示例
Error: conversion webhook for databases.db.example.com failed: 
  Post "https://webhook-service.default.svc:443/convert": 
  context deadline exceeded
```

**排查步骤:**

```bash
# 1. 检查 Webhook 服务
kubectl get svc -n crd-system crd-conversion-webhook
kubectl get endpoints -n crd-system crd-conversion-webhook

# 2. 检查 Webhook Pod
kubectl get pods -n crd-system -l app=crd-webhook
kubectl logs -n crd-system -l app=crd-webhook

# 3. 测试 Webhook 连通性
kubectl run test-curl --image=curlimages/curl --rm -it -- \
  curl -k https://crd-conversion-webhook.crd-system.svc:443/health

# 4. 查看 CRD Conversion 配置
kubectl get crd databases.db.example.com -o jsonpath='{.spec.conversion}' | jq

# 5. 检查证书
kubectl get crd databases.db.example.com -o jsonpath='{.spec.conversion.webhook.clientConfig.caBundle}' | base64 -d | openssl x509 -text
```

### 8.4 CRD 更新失败

**症状**: 无法更新 CRD Schema

```bash
# 错误: 不允许的 Schema 变更
Error: spec.versions[0].schema: Forbidden: 
  cannot change validation rule from ... to ...
```

**安全更新策略:**

```bash
# 1. 添加新版本(不修改旧版本 Schema)
# crd-v2.yaml
spec:
  versions:
    - name: v2  # 新版本
      served: true
      storage: true
      schema: ...  # 新 Schema
    - name: v1  # 旧版本(不修改)
      served: true
      storage: false

# 2. 应用更新
kubectl apply -f crd-v2.yaml

# 3. 迁移现有资源
kubectl get databases --all-namespaces -o json | \
  jq '.items[].apiVersion = "db.example.com/v2"' | \
  kubectl apply -f -

# 4. 逐步弃用旧版本
kubectl patch crd databases.db.example.com --type=json -p='[
  {"op": "replace", "path": "/spec/versions/1/served", "value": false}
]'
```

### 8.5 性能问题

**症状**: CR 列表查询缓慢

```bash
# 检查 CR 数量
kubectl get databases --all-namespaces --no-headers | wc -l

# 查看 API Server 延迟
kubectl get --raw /metrics | grep apiserver_request_duration_seconds | grep customresourcedefinitions

# 启用 SelectableFields (v1.30+)
spec:
  versions:
    - name: v1
      selectableFields:
        - jsonPath: .spec.engine
        - jsonPath: .status.phase

# 使用 FieldSelector 查询
kubectl get databases --field-selector spec.engine=mysql
```

---

## 📚 参考资源

- **官方文档**:
  - [Extend the Kubernetes API with CustomResourceDefinitions](https://kubernetes.io/docs/tasks/extend-kubernetes/custom-resources/custom-resource-definitions/)
  - [Versions in CustomResourceDefinitions](https://kubernetes.io/docs/tasks/extend-kubernetes/custom-resources/custom-resource-definition-versioning/)
  - [Validating Admission Policy (CEL)](https://kubernetes.io/docs/reference/access-authn-authz/validating-admission-policy/)
- **CEL 语言**: https://github.com/google/cel-spec
- **Kubebuilder**: https://book.kubebuilder.io/ (CRD 代码生成工具)
- **Operator SDK**: https://sdk.operatorframework.io/ (Operator 开发框架)

---

**最佳实践总结**:

1. **Schema 设计**: 始终定义完整的 OpenAPI Schema,避免 `x-kubernetes-preserve-unknown-fields: true`
2. **CEL 验证**: 使用 CEL 表达式代替 Admission Webhook 进行简单验证(性能更好)
3. **版本管理**: 使用 Webhook 转换实现多版本兼容,避免破坏性变更
4. **Status 子资源**: 始终启用 `subresources.status`,避免 spec/status 更新冲突
5. **Printer Columns**: 配置合理的 `additionalPrinterColumns`,提升用户体验
6. **不可变字段**: 使用 CEL Transition Rules 保护不可变字段
7. **性能优化**: 对于大规模 CR,启用 SelectableFields (v1.30+)

---

🚀 **CRD 是 Kubernetes 扩展的基石,掌握它等于掌握了云原生生态系统的核心能力!**
