# 25 - ValidatingAdmissionPolicy YAML 配置参考

> **适用版本**: Kubernetes v1.26+ (Beta), v1.30+ (GA) | **最后更新**: 2026-02 | **难度**: 入门 → 专家全覆盖

## 📋 目录

- [概述](#概述)
- [核心概念](#核心概念)
- [ValidatingAdmissionPolicy 字段详解](#validatingadmissionpolicy-字段详解)
- [ValidatingAdmissionPolicyBinding 字段详解](#validatingadmissionpolicybinding-字段详解)
- [CEL 表达式详解](#cel-表达式详解)
- [内部原理](#内部原理)
- [版本兼容性](#版本兼容性)
- [最佳实践](#最佳实践)
- [生产案例](#生产案例)
- [常见问题 FAQ](#常见问题-faq)

---

## 概述

### 什么是 ValidatingAdmissionPolicy

ValidatingAdmissionPolicy 是 Kubernetes v1.26 引入的声明式准入控制机制，使用 CEL (Common Expression Language) 表达式定义验证规则，无需编写和维护 Webhook。

**核心优势**:
- ✅ **声明式配置**: 使用 YAML + CEL 表达式，无需编写代码
- ✅ **高性能**: 进程内执行，比 Webhook 快 10-100 倍
- ✅ **高可靠性**: 无网络依赖，无外部服务故障风险
- ✅ **安全性**: CEL 表达式沙箱隔离，防止恶意代码执行
- ✅ **灵活性**: 支持参数化配置，一个策略多种绑定
- ✅ **可观测性**: 内置审计注解和指标

**与 Webhook 对比**:
| 特性 | ValidatingAdmissionPolicy | ValidatingWebhook |
|------|---------------------------|-------------------|
| 配置方式 | 声明式 YAML + CEL | 代码 + 外部服务 |
| 性能 | 进程内，微秒级 | 网络调用，毫秒级 |
| 可靠性 | 无外部依赖 | 依赖外部服务和网络 |
| 维护成本 | 低 | 高（需要开发、部署、监控） |
| 灵活性 | 中等（CEL 限制） | 高（任意逻辑） |
| 适用场景 | 标准验证规则 | 复杂业务逻辑 |

---

## 核心概念

### 两个核心资源

1. **ValidatingAdmissionPolicy**: 定义验证规则和匹配条件
2. **ValidatingAdmissionPolicyBinding**: 将策略绑定到特定资源并提供参数

```
┌─────────────────────────────────────────────────────────────────┐
│                   ValidatingAdmissionPolicy                      │
│  ┌───────────────────────────────────────────────────────────┐  │
│  │ matchConstraints: 匹配哪些资源（GVK）                       │  │
│  │ validations[]:    验证规则（CEL 表达式）                    │  │
│  │ paramKind:        参数类型（可选）                          │  │
│  │ auditAnnotations: 审计注解                                  │  │
│  │ failurePolicy:    失败处理策略                              │  │
│  │ matchConditions:  前置条件（v1.27+）                        │  │
│  └───────────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────────┘
                              ▼ 绑定
┌─────────────────────────────────────────────────────────────────┐
│              ValidatingAdmissionPolicyBinding                    │
│  ┌───────────────────────────────────────────────────────────┐  │
│  │ policyName:        引用的策略名称                           │  │
│  │ paramRef:          参数对象引用                             │  │
│  │ matchResources:    匹配哪些命名空间/对象                    │  │
│  │ validationActions: 验证动作（Deny/Warn/Audit）             │  │
│  └───────────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────────┘
```

### 工作流程

```
API 请求
   │
   ├─► [1] 匹配 Binding 的 matchResources
   │      └─► 检查命名空间标签、资源选择器
   │
   ├─► [2] 匹配 Policy 的 matchConstraints
   │      └─► 检查资源 GVK
   │
   ├─► [3] 评估 matchConditions（可选）
   │      └─► 执行前置条件 CEL 表达式
   │
   ├─► [4] 加载参数对象（paramRef）
   │      └─► 获取配置参数
   │
   ├─► [5] 执行 validations[] 表达式
   │      └─► 评估每个验证规则
   │
   ├─► [6] 根据 validationActions 处理结果
   │      ├─► Deny: 拒绝请求
   │      ├─► Warn: 返回警告
   │      └─► Audit: 记录审计事件
   │
   └─► [7] 添加 auditAnnotations（可选）
```

---

## ValidatingAdmissionPolicy 字段详解

### 基础结构

```yaml
apiVersion: admissionregistration.k8s.io/v1  # v1.26+ beta, v1.30+ GA
kind: ValidatingAdmissionPolicy
metadata:
  name: policy-name
spec:
  # 匹配约束：定义应用到哪些资源类型
  matchConstraints:
    resourceRules: []
    excludeResourceRules: []
    namespaceSelector: {}
    objectSelector: {}
    matchPolicy: Exact|Equivalent
  
  # 验证规则：CEL 表达式列表
  validations:
    - expression: ""           # CEL 表达式，返回 true = 通过
      message: ""              # 静态错误消息
      messageExpression: ""    # 动态错误消息（CEL）
      reason: ""               # 失败原因代码
  
  # 参数类型（可选）：支持参数化配置
  paramKind:
    apiVersion: ""
    kind: ""
  
  # 审计注解：在审计日志中添加自定义字段
  auditAnnotations:
    - key: ""
      valueExpression: ""
  
  # 失败策略：验证失败时的处理方式
  failurePolicy: Fail|Ignore
  
  # 匹配条件：前置过滤条件（v1.27+）
  matchConditions:
    - name: ""
      expression: ""
  
  # 变量定义：可复用的 CEL 表达式（v1.28+）
  variables:
    - name: ""
      expression: ""
```

---

### 1. matchConstraints 字段

定义策略应用到哪些资源类型。

```yaml
spec:
  matchConstraints:
    # 包含规则：匹配哪些资源
    resourceRules:
      - apiGroups: ["apps"]              # API 组
        apiVersions: ["v1"]              # API 版本
        resources: ["deployments"]       # 资源类型
        operations: ["CREATE", "UPDATE"] # 操作类型
        scope: "Namespaced"              # 作用域: Namespaced|Cluster|*
    
    # 排除规则：排除哪些资源
    excludeResourceRules:
      - apiGroups: [""]
        apiVersions: ["v1"]
        resources: ["pods"]
        operations: ["DELETE"]
    
    # 命名空间选择器：匹配哪些命名空间
    namespaceSelector:
      matchLabels:
        environment: production
      matchExpressions:
        - key: team
          operator: In
          values: ["platform", "infrastructure"]
    
    # 对象选择器：匹配哪些对象
    objectSelector:
      matchLabels:
        app.kubernetes.io/managed-by: helm
      matchExpressions:
        - key: security-level
          operator: Exists
    
    # 匹配策略
    # - Exact: 精确匹配指定的 GVK
    # - Equivalent: 匹配等价的 GVK（例如 apps/v1 和 apps/v1beta2）
    matchPolicy: Equivalent
```

**operations 可选值**:
- `CREATE`: 创建资源
- `UPDATE`: 更新资源
- `DELETE`: 删除资源
- `CONNECT`: 连接资源（如 exec、port-forward）
- `*`: 所有操作

**scope 可选值**:
- `Namespaced`: 命名空间资源
- `Cluster`: 集群资源
- `*`: 所有作用域

---

### 2. validations 字段

定义验证规则列表，每个规则包含 CEL 表达式和错误处理。

```yaml
spec:
  validations:
    # 规则 1: 基础验证
    - expression: "object.spec.replicas <= 10"
      message: "副本数不能超过 10"
      reason: Invalid
    
    # 规则 2: 复杂条件
    - expression: |
        object.spec.template.spec.containers.all(c,
          c.resources.requests.has('memory') &&
          c.resources.requests.has('cpu')
        )
      message: "所有容器必须设置 CPU 和内存请求"
      reason: Required
    
    # 规则 3: 动态错误消息
    - expression: "object.spec.replicas <= params.maxReplicas"
      messageExpression: |
        "副本数 " + string(object.spec.replicas) + 
        " 超过最大限制 " + string(params.maxReplicas)
      reason: Invalid
    
    # 规则 4: 使用变量
    - expression: "variables.hasResourceLimits"
      message: "所有容器必须设置资源限制"
      reason: Required
    
    # 规则 5: 更新验证（使用 oldObject）
    - expression: |
        !has(oldObject) ||
        object.metadata.labels['immutable-label'] == oldObject.metadata.labels['immutable-label']
      message: "标签 'immutable-label' 不可修改"
      reason: Forbidden
```

**reason 可选值** (与 Pod Status Reasons 对齐):
- `Unauthorized`: 未授权
- `Forbidden`: 禁止操作
- `Invalid`: 无效配置
- `Required`: 必需字段缺失
- `FieldValueInvalid`: 字段值无效
- `FieldValueDuplicate`: 字段值重复
- `FieldValueNotSupported`: 字段值不支持
- `TooLong`: 值过长
- `TooMany`: 数量过多

**expression vs messageExpression**:
- `expression`: 验证逻辑，返回 `true` 表示通过，`false` 表示失败
- `messageExpression`: 动态生成错误消息，可以引用对象字段和变量

---

### 3. paramKind 字段

定义参数类型，支持参数化配置。

```yaml
spec:
  # 参数类型定义
  paramKind:
    apiVersion: v1
    kind: ConfigMap
```

**完整示例**:

```yaml
# 1. 定义参数对象
---
apiVersion: v1
kind: ConfigMap
metadata:
  name: deployment-limits-dev
  namespace: default
data:
  maxReplicas: "5"
  maxMemory: "2Gi"
  maxCPU: "2"
---
apiVersion: v1
kind: ConfigMap
metadata:
  name: deployment-limits-prod
  namespace: default
data:
  maxReplicas: "20"
  maxMemory: "16Gi"
  maxCPU: "8"

---
# 2. 定义策略（引用参数类型）
apiVersion: admissionregistration.k8s.io/v1
kind: ValidatingAdmissionPolicy
metadata:
  name: deployment-resource-limits
spec:
  paramKind:
    apiVersion: v1
    kind: ConfigMap  # 参数类型为 ConfigMap
  
  matchConstraints:
    resourceRules:
      - apiGroups: ["apps"]
        apiVersions: ["v1"]
        resources: ["deployments"]
        operations: ["CREATE", "UPDATE"]
  
  validations:
    # 使用 params 引用参数对象的字段
    - expression: "object.spec.replicas <= int(params.data.maxReplicas)"
      messageExpression: |
        "副本数 " + string(object.spec.replicas) + 
        " 超过最大限制 " + params.data.maxReplicas
    
    - expression: |
        object.spec.template.spec.containers.all(c,
          !has(c.resources.limits.memory) ||
          resource.quantity(c.resources.limits.memory) <= resource.quantity(params.data.maxMemory)
        )
      message: "容器内存限制超过配置的最大值"

---
# 3. 绑定到开发环境（使用 dev 参数）
apiVersion: admissionregistration.k8s.io/v1
kind: ValidatingAdmissionPolicyBinding
metadata:
  name: deployment-limits-dev-binding
spec:
  policyName: deployment-resource-limits
  
  # 引用开发环境参数
  paramRef:
    name: deployment-limits-dev
    namespace: default
  
  matchResources:
    namespaceSelector:
      matchLabels:
        environment: development

---
# 4. 绑定到生产环境（使用 prod 参数）
apiVersion: admissionregistration.k8s.io/v1
kind: ValidatingAdmissionPolicyBinding
metadata:
  name: deployment-limits-prod-binding
spec:
  policyName: deployment-resource-limits
  
  # 引用生产环境参数
  paramRef:
    name: deployment-limits-prod
    namespace: default
  
  matchResources:
    namespaceSelector:
      matchLabels:
        environment: production
```

**支持的参数类型**:
- 任何集群内的 Kubernetes 资源（通常使用 ConfigMap 或自定义 CRD）
- 参数对象必须在绑定时存在
- 参数对象更新会触发策略重新评估

---

### 4. auditAnnotations 字段

在审计日志中添加自定义注解，用于记录验证上下文。

```yaml
spec:
  auditAnnotations:
    # 注解 1: 记录副本数
    - key: "validated-replicas"
      valueExpression: "string(object.spec.replicas)"
    
    # 注解 2: 记录用户信息
    - key: "validated-by-user"
      valueExpression: "request.userInfo.username"
    
    # 注解 3: 记录违规原因
    - key: "validation-reason"
      valueExpression: |
        object.spec.replicas > params.maxReplicas 
        ? "replica-count-exceeded" 
        : "valid"
    
    # 注解 4: 记录资源使用情况
    - key: "total-memory-requests"
      valueExpression: |
        object.spec.template.spec.containers.map(c, 
          has(c.resources.requests.memory) 
          ? resource.quantity(c.resources.requests.memory) 
          : resource.quantity("0")
        ).sum().asInteger()
```

**审计注解特性**:
- 不会影响验证结果
- 记录在 API Server 审计日志中
- 可用于监控、告警、合规性分析
- valueExpression 必须返回字符串类型

**审计日志示例**:
```json
{
  "kind": "Event",
  "apiVersion": "audit.k8s.io/v1",
  "annotations": {
    "validated-replicas": "15",
    "validated-by-user": "alice",
    "validation-reason": "replica-count-exceeded"
  }
}
```

---

### 5. failurePolicy 字段

定义当策略评估失败（如 CEL 表达式错误）时的处理方式。

```yaml
spec:
  # 失败策略
  # - Fail: 评估失败时拒绝请求（默认）
  # - Ignore: 评估失败时忽略策略
  failurePolicy: Fail
```

**Fail vs Ignore**:

| 场景 | Fail | Ignore |
|------|------|--------|
| CEL 表达式语法错误 | ❌ 拒绝请求 | ✅ 忽略策略 |
| 引用不存在的字段 | ❌ 拒绝请求 | ✅ 忽略策略 |
| 参数对象不存在 | ❌ 拒绝请求 | ✅ 忽略策略 |
| 超时 | ❌ 拒绝请求 | ✅ 忽略策略 |
| 表达式返回 false | ❌ 拒绝请求（正常行为） | ❌ 拒绝请求（正常行为） |

**最佳实践**:
- 生产环境初期使用 `Ignore`，观察指标后切换到 `Fail`
- 对关键安全策略使用 `Fail`
- 对可选性策略使用 `Ignore`

---

### 6. matchConditions 字段

前置过滤条件，用于在执行 validations 前快速过滤请求（v1.27+）。

```yaml
spec:
  matchConditions:
    # 条件 1: 仅对特定操作生效
    - name: "is-create-or-update"
      expression: "request.operation in ['CREATE', 'UPDATE']"
    
    # 条件 2: 排除系统命名空间
    - name: "exclude-system-namespaces"
      expression: "!namespaceObject.metadata.name.startsWith('kube-')"
    
    # 条件 3: 仅对特定用户生效
    - name: "non-admin-users"
      expression: |
        !request.userInfo.username.startsWith('system:') &&
        !'cluster-admin' in request.userInfo.groups
    
    # 条件 4: 检查对象标签
    - name: "has-enforce-label"
      expression: |
        has(object.metadata.labels) &&
        'policy.kubernetes.io/enforce' in object.metadata.labels &&
        object.metadata.labels['policy.kubernetes.io/enforce'] == 'true'
```

**matchConditions vs validations**:

| 特性 | matchConditions | validations |
|------|-----------------|-------------|
| 用途 | 快速过滤 | 验证逻辑 |
| 失败行为 | 跳过策略 | 拒绝请求 |
| 性能影响 | 优先执行，减少不必要的验证 | 主要验证逻辑 |
| 错误处理 | 失败时跳过策略 | 失败时根据 validationActions 处理 |

**使用场景**:
- 性能优化：减少不必要的验证计算
- 条件性策略：仅在特定场景下生效
- 灵活控制：动态启用/禁用策略

---

### 7. variables 字段

定义可复用的 CEL 表达式变量（v1.28+）。

```yaml
spec:
  variables:
    # 变量 1: 检查是否所有容器都设置了资源限制
    - name: hasResourceLimits
      expression: |
        object.spec.template.spec.containers.all(c,
          has(c.resources.limits) &&
          has(c.resources.limits.cpu) &&
          has(c.resources.limits.memory)
        )
    
    # 变量 2: 计算总 CPU 请求
    - name: totalCPURequests
      expression: |
        object.spec.template.spec.containers
        .filter(c, has(c.resources.requests.cpu))
        .map(c, resource.quantity(c.resources.requests.cpu))
        .sum()
    
    # 变量 3: 检查镜像来源
    - name: allImagesFromTrustedRegistry
      expression: |
        object.spec.template.spec.containers.all(c,
          c.image.startsWith('registry.company.com/') ||
          c.image.startsWith('gcr.io/company/')
        )
    
    # 变量 4: 复杂条件组合
    - name: isProductionReady
      expression: |
        variables.hasResourceLimits &&
        variables.allImagesFromTrustedRegistry &&
        object.spec.replicas >= 2
  
  validations:
    # 使用变量
    - expression: "variables.hasResourceLimits"
      message: "生产环境 Deployment 必须设置资源限制"
    
    - expression: "variables.allImagesFromTrustedRegistry"
      message: "仅允许使用受信任的镜像仓库"
    
    - expression: "variables.totalCPURequests <= resource.quantity('100')"
      message: "总 CPU 请求不能超过 100 核"
    
    - expression: "variables.isProductionReady"
      message: "Deployment 未满足生产环境要求"
```

**变量特性**:
- 变量按定义顺序计算
- 后续变量可以引用前面的变量
- 变量可以在 validations 和 auditAnnotations 中使用
- 提高可读性和可维护性

---

## ValidatingAdmissionPolicyBinding 字段详解

### 基础结构

```yaml
apiVersion: admissionregistration.k8s.io/v1
kind: ValidatingAdmissionPolicyBinding
metadata:
  name: binding-name
spec:
  # 引用的策略名称
  policyName: policy-name
  
  # 参数对象引用（可选）
  paramRef:
    name: ""
    namespace: ""
    selector: {}
    parameterNotFoundAction: Allow|Deny
  
  # 匹配资源：定义绑定到哪些命名空间/对象
  matchResources:
    namespaceSelector: {}
    objectSelector: {}
    resourceRules: []
    excludeResourceRules: []
    matchPolicy: Exact|Equivalent
  
  # 验证动作：验证失败时的行为（v1.27+）
  validationActions:
    - Deny    # 拒绝请求
    - Warn    # 返回警告
    - Audit   # 仅记录审计日志
```

---

### 1. policyName 字段

```yaml
spec:
  # 引用的 ValidatingAdmissionPolicy 名称
  policyName: require-resource-limits
```

---

### 2. paramRef 字段

引用参数对象，为策略提供配置。

```yaml
spec:
  paramRef:
    # 参数对象名称
    name: deployment-limits-prod
    
    # 参数对象命名空间
    namespace: config-namespace
    
    # 标签选择器（可选，用于动态选择参数）
    selector:
      matchLabels:
        environment: production
        team: platform
    
    # 参数未找到时的处理策略（v1.28+）
    # - Allow: 允许请求通过（默认）
    # - Deny: 拒绝请求
    parameterNotFoundAction: Deny
```

**参数解析规则**:
1. 如果提供了 `name` 和 `namespace`：直接引用该对象
2. 如果提供了 `selector`：
   - 在 `namespace` 中查找匹配的对象
   - 如果匹配多个，选择名称字母序最小的
3. 如果未找到参数且 `parameterNotFoundAction: Deny`：拒绝请求

---

### 3. matchResources 字段

定义绑定应用到哪些资源。

```yaml
spec:
  matchResources:
    # 命名空间选择器：匹配哪些命名空间
    namespaceSelector:
      matchLabels:
        environment: production
      matchExpressions:
        - key: team
          operator: In
          values: ["backend", "frontend"]
    
    # 对象选择器：匹配哪些对象
    objectSelector:
      matchLabels:
        security-tier: high
      matchExpressions:
        - key: managed-by
          operator: NotIn
          values: ["legacy-system"]
    
    # 资源规则：细粒度控制（可选）
    resourceRules:
      - apiGroups: ["apps"]
        apiVersions: ["v1"]
        resources: ["deployments", "statefulsets"]
        operations: ["CREATE", "UPDATE"]
        scope: "Namespaced"
    
    # 排除规则：排除特定资源（可选）
    excludeResourceRules:
      - apiGroups: ["apps"]
        apiVersions: ["v1"]
        resources: ["deployments"]
        operations: ["DELETE"]
    
    # 匹配策略
    matchPolicy: Equivalent
```

**matchResources vs Policy.matchConstraints**:

| 维度 | matchResources | matchConstraints |
|------|----------------|------------------|
| 位置 | Binding | Policy |
| 用途 | 定义绑定范围 | 定义策略适用的资源类型 |
| 选择器 | 支持 namespaceSelector + objectSelector | 支持 namespaceSelector + objectSelector |
| 资源规则 | 可选（进一步细化） | 必需（定义资源类型） |
| 优先级 | 两者都必须匹配才会应用策略 | - |

**最佳实践**:
- 在 Policy 中定义资源类型（GVK）
- 在 Binding 中定义部署范围（命名空间、环境）

---

### 4. validationActions 字段

定义验证失败时的行为（v1.27+）。

```yaml
spec:
  # 验证动作列表（可以组合多个）
  validationActions:
    - Deny   # 拒绝请求（默认）
    - Warn   # 返回警告消息
    - Audit  # 仅记录审计日志
```

**动作详解**:

| 动作 | 行为 | 用途 | API 响应 |
|------|------|------|----------|
| **Deny** | 拒绝请求 | 强制执行策略 | 返回 403 Forbidden |
| **Warn** | 允许请求，返回警告 | 渐进式推广策略 | 返回 200 + Warning header |
| **Audit** | 允许请求，记录审计日志 | 观察模式，收集违规数据 | 返回 200，审计日志有记录 |

**组合使用**:

```yaml
# 场景 1: 仅拒绝
validationActions:
  - Deny

# 场景 2: 拒绝 + 审计
validationActions:
  - Deny
  - Audit   # 拒绝请求，同时记录审计日志

# 场景 3: 警告 + 审计（渐进式推广）
validationActions:
  - Warn    # 允许请求但返回警告
  - Audit   # 记录违规行为

# 场景 4: 仅审计（观察模式）
validationActions:
  - Audit   # 不影响请求，仅收集数据
```

**渐进式推广策略**:

```
阶段 1: Audit Only
  ↓ 收集违规数据，评估影响范围
阶段 2: Warn + Audit
  ↓ 通知用户即将强制执行
阶段 3: Deny + Audit
  ↓ 强制执行，持续监控
阶段 4: Deny Only
  ✓ 完全执行
```

---

### 完整示例：多环境绑定

```yaml
# 1. 定义策略（适用于所有 Deployment）
---
apiVersion: admissionregistration.k8s.io/v1
kind: ValidatingAdmissionPolicy
metadata:
  name: deployment-best-practices
spec:
  paramKind:
    apiVersion: v1
    kind: ConfigMap
  
  matchConstraints:
    resourceRules:
      - apiGroups: ["apps"]
        apiVersions: ["v1"]
        resources: ["deployments"]
        operations: ["CREATE", "UPDATE"]
  
  validations:
    - expression: "object.spec.replicas <= int(params.data.maxReplicas)"
      message: "副本数超过限制"
    
    - expression: |
        object.spec.template.spec.containers.all(c,
          c.image.startsWith(params.data.allowedRegistry)
        )
      message: "仅允许使用指定的镜像仓库"

---
# 2. 开发环境参数
apiVersion: v1
kind: ConfigMap
metadata:
  name: limits-dev
  namespace: default
data:
  maxReplicas: "3"
  allowedRegistry: "registry.dev.company.com/"

---
# 3. 生产环境参数
apiVersion: v1
kind: ConfigMap
metadata:
  name: limits-prod
  namespace: default
data:
  maxReplicas: "10"
  allowedRegistry: "registry.prod.company.com/"

---
# 4. 开发环境绑定（Warn + Audit，宽松模式）
apiVersion: admissionregistration.k8s.io/v1
kind: ValidatingAdmissionPolicyBinding
metadata:
  name: best-practices-dev
spec:
  policyName: deployment-best-practices
  paramRef:
    name: limits-dev
    namespace: default
  
  matchResources:
    namespaceSelector:
      matchLabels:
        environment: development
  
  validationActions:
    - Warn    # 开发环境仅警告
    - Audit

---
# 5. 生产环境绑定（Deny，严格模式）
apiVersion: admissionregistration.k8s.io/v1
kind: ValidatingAdmissionPolicyBinding
metadata:
  name: best-practices-prod
spec:
  policyName: deployment-best-practices
  paramRef:
    name: limits-prod
    namespace: default
  
  matchResources:
    namespaceSelector:
      matchLabels:
        environment: production
  
  validationActions:
    - Deny     # 生产环境强制拒绝
    - Audit
```

---

## CEL 表达式详解

### CEL 基础

CEL (Common Expression Language) 是 Google 开发的表达式语言，用于安全的数据验证。

**语法特点**:
- ✅ 类型安全：编译时类型检查
- ✅ 沙箱隔离：无副作用，无网络/IO 操作
- ✅ 性能高：编译后缓存，执行快速
- ✅ 易读易写：类 C/Java 语法

**基础语法**:

```cel
// 比较运算
object.spec.replicas > 5
object.spec.replicas >= 5
object.spec.replicas < 5
object.spec.replicas <= 5
object.spec.replicas == 5
object.spec.replicas != 5

// 逻辑运算
expr1 && expr2              // 逻辑与
expr1 || expr2              // 逻辑或
!expr                       // 逻辑非

// 三元运算
condition ? trueValue : falseValue

// 字符串操作
str.startsWith('prefix')
str.endsWith('suffix')
str.contains('substring')
str.matches('^regex$')      // 正则匹配

// 集合操作
item in list                // 成员检查
list.size()                 // 长度
list.all(x, condition)      // 所有元素满足条件
list.exists(x, condition)   // 存在元素满足条件
list.map(x, expression)     // 映射
list.filter(x, condition)   // 过滤
```

---

### 内置变量

ValidatingAdmissionPolicy 中可用的 CEL 变量：

| 变量 | 类型 | 描述 | 示例 |
|------|------|------|------|
| `object` | Object | 当前请求的对象 | `object.spec.replicas` |
| `oldObject` | Object | 更新前的对象（仅 UPDATE） | `oldObject.spec.replicas` |
| `request` | AdmissionRequest | 准入请求信息 | `request.operation` |
| `params` | Object | 参数对象（paramRef） | `params.data.maxReplicas` |
| `namespaceObject` | Namespace | 对象所属的命名空间 | `namespaceObject.metadata.labels` |
| `authorizer` | Authorizer | 授权检查器（v1.28+） | `authorizer.allowed(...)` |
| `variables` | Map | 自定义变量 | `variables.hasResourceLimits` |

---

#### 1. object 变量

表示当前请求的 Kubernetes 对象。

```yaml
validations:
  # 访问基础字段
  - expression: "object.metadata.name.startsWith('prod-')"
    message: "生产环境资源名称必须以 'prod-' 开头"
  
  # 访问 spec 字段
  - expression: "object.spec.replicas >= 2"
    message: "生产环境副本数至少为 2"
  
  # 访问嵌套字段
  - expression: |
      object.spec.template.spec.containers[0].image.startsWith('registry.company.com/')
    message: "第一个容器必须使用公司镜像仓库"
  
  # 检查字段是否存在
  - expression: "has(object.metadata.labels) && 'app' in object.metadata.labels"
    message: "必须设置 'app' 标签"
  
  # 遍历数组
  - expression: |
      object.spec.template.spec.containers.all(c,
        has(c.resources.limits)
      )
    message: "所有容器必须设置资源限制"
```

---

#### 2. oldObject 变量

表示更新前的对象，仅在 UPDATE 操作时可用。

```yaml
validations:
  # 检查是否为创建操作
  - expression: "!has(oldObject)"
    message: "此策略仅应用于新创建的资源"
  
  # 检查是否为更新操作
  - expression: "has(oldObject)"
    message: "此策略仅应用于更新操作"
  
  # 防止字段修改（不可变字段）
  - expression: |
      !has(oldObject) ||
      object.metadata.labels['immutable-label'] == oldObject.metadata.labels['immutable-label']
    message: "标签 'immutable-label' 不可修改"
  
  # 防止副本数减少
  - expression: |
      !has(oldObject) ||
      object.spec.replicas >= oldObject.spec.replicas
    message: "不允许减少副本数"
  
  # 检查特定字段是否变化
  - expression: |
      !has(oldObject) ||
      object.spec.template.spec.containers[0].image != oldObject.spec.template.spec.containers[0].image
    message: "检测到镜像变更，需要审批"
```

**常见模式：不可变字段**

```yaml
# 模式：如果是创建操作 OR 字段未变化，则通过
expression: |
  !has(oldObject) ||
  object.spec.fieldName == oldObject.spec.fieldName
```

---

#### 3. request 变量

包含准入请求的元数据。

```yaml
validations:
  # 检查操作类型
  - expression: "request.operation == 'CREATE'"
    message: "此策略仅应用于创建操作"
  
  - expression: "request.operation in ['CREATE', 'UPDATE']"
    message: "此策略应用于创建和更新操作"
  
  # 检查用户信息
  - expression: |
      !request.userInfo.username.startsWith('system:')
    message: "系统用户不受此策略限制"
  
  - expression: |
      'platform-team' in request.userInfo.groups
    message: "仅平台团队成员可以执行此操作"
  
  # 检查请求来源
  - expression: |
      has(request.userInfo.extra) &&
      'client-type' in request.userInfo.extra &&
      'kubectl' in request.userInfo.extra['client-type']
    message: "仅允许通过 kubectl 创建"
  
  # 检查 DryRun
  - expression: "!request.dryRun"
    message: "DryRun 请求不触发此策略"
```

**request 字段详解**:

```go
// AdmissionRequest 结构（Go 定义）
type AdmissionRequest struct {
    // 操作类型: CREATE, UPDATE, DELETE, CONNECT
    Operation string
    
    // 用户信息
    UserInfo struct {
        Username string              // 用户名
        UID      string              // 用户 UID
        Groups   []string            // 用户组列表
        Extra    map[string][]string // 额外信息
    }
    
    // 资源信息
    Kind struct {
        Group   string
        Version string
        Kind    string
    }
    
    // 命名空间
    Namespace string
    
    // 资源名称
    Name string
    
    // DryRun 标志
    DryRun bool
    
    // 子资源（如 status, scale）
    SubResource string
}
```

---

#### 4. params 变量

引用 paramRef 指定的参数对象。

```yaml
# 参数对象示例（ConfigMap）
apiVersion: v1
kind: ConfigMap
metadata:
  name: policy-config
data:
  maxReplicas: "10"
  allowedRegistries: "registry1.io,registry2.io"
  enforceResourceLimits: "true"

---
# 在 CEL 中使用 params
validations:
  # 访问字符串字段
  - expression: "object.spec.replicas <= int(params.data.maxReplicas)"
    message: "副本数超过限制"
  
  # 类型转换
  - expression: "params.data.enforceResourceLimits == 'true'"
    message: "必须启用资源限制"
  
  # 处理逗号分隔的列表
  - expression: |
      params.data.allowedRegistries.split(',').exists(r,
        object.spec.template.spec.containers[0].image.startsWith(r)
      )
    message: "镜像必须来自允许的仓库"
  
  # 使用自定义 CRD 参数
  - expression: "object.spec.replicas <= params.spec.limits.maxReplicas"
    message: "副本数超过配置限制"
  
  - expression: |
      object.spec.template.spec.containers.all(c,
        resource.quantity(c.resources.limits.memory) <= resource.quantity(params.spec.limits.maxMemoryPerContainer)
      )
    message: "容器内存限制超过配置"
```

**自定义 CRD 参数示例**:

```yaml
# 1. 定义参数 CRD
---
apiVersion: apiextensions.k8s.io/v1
kind: CustomResourceDefinition
metadata:
  name: deploymentpolicies.policy.company.com
spec:
  group: policy.company.com
  names:
    kind: DeploymentPolicy
    plural: deploymentpolicies
  scope: Namespaced
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
                limits:
                  type: object
                  properties:
                    maxReplicas:
                      type: integer
                    maxMemoryPerContainer:
                      type: string
                    allowedRegistries:
                      type: array
                      items:
                        type: string

---
# 2. 创建参数对象
apiVersion: policy.company.com/v1
kind: DeploymentPolicy
metadata:
  name: prod-limits
  namespace: default
spec:
  limits:
    maxReplicas: 20
    maxMemoryPerContainer: "8Gi"
    allowedRegistries:
      - "registry.prod.company.com"
      - "gcr.io/company-prod"

---
# 3. 在策略中使用
apiVersion: admissionregistration.k8s.io/v1
kind: ValidatingAdmissionPolicy
metadata:
  name: deployment-policy
spec:
  paramKind:
    apiVersion: policy.company.com/v1
    kind: DeploymentPolicy
  
  validations:
    # 使用强类型字段（无需类型转换）
    - expression: "object.spec.replicas <= params.spec.limits.maxReplicas"
    
    - expression: |
        params.spec.limits.allowedRegistries.exists(r,
          object.spec.template.spec.containers[0].image.startsWith(r)
        )
```

---

#### 5. namespaceObject 变量

表示对象所属的命名空间（Namespace 资源）。

```yaml
validations:
  # 检查命名空间标签
  - expression: |
      has(namespaceObject.metadata.labels) &&
      'environment' in namespaceObject.metadata.labels
    message: "命名空间必须设置 'environment' 标签"
  
  # 根据命名空间标签调整策略
  - expression: |
      namespaceObject.metadata.labels['environment'] != 'production' ||
      object.spec.replicas >= 2
    message: "生产命名空间中副本数至少为 2"
  
  # 检查命名空间注解
  - expression: |
      has(namespaceObject.metadata.annotations) &&
      'policy.company.com/max-replicas' in namespaceObject.metadata.annotations &&
      object.spec.replicas <= int(namespaceObject.metadata.annotations['policy.company.com/max-replicas'])
    message: "副本数超过命名空间限制"
  
  # 命名空间资源配额检查
  - expression: |
      has(namespaceObject.metadata.annotations['quota.company.com/cpu']) &&
      variables.totalCPURequests <= resource.quantity(namespaceObject.metadata.annotations['quota.company.com/cpu'])
    message: "命名空间 CPU 配额不足"
```

**使用场景**:
- 基于命名空间标签的策略
- 命名空间级别的配额和限制
- 多租户环境的隔离策略

---

#### 6. authorizer 变量

用于检查用户权限（v1.28+）。

```yaml
validations:
  # 检查用户是否有特定权限
  - expression: |
      authorizer.allowed(
        request.userInfo,
        object.metadata.namespace,
        "apps",
        "v1",
        "deployments",
        "delete"
      )
    message: "用户没有删除 Deployment 的权限"
  
  # 检查是否为集群管理员
  - expression: |
      authorizer.allowed(
        request.userInfo,
        "",
        "",
        "v1",
        "namespaces",
        "create"
      )
    message: "仅集群管理员可以执行此操作"
  
  # 条件性权限检查
  - expression: |
      object.spec.replicas <= 10 ||
      authorizer.allowed(
        request.userInfo,
        object.metadata.namespace,
        "policy.company.com",
        "v1",
        "scalerequests",
        "create"
      )
    message: "超过 10 个副本需要提交扩容申请"
```

**authorizer.allowed() 方法签名**:

```cel
authorizer.allowed(
  userInfo,        // UserInfo 对象
  namespace,       // 命名空间（集群资源为空字符串）
  apiGroup,        // API 组
  apiVersion,      // API 版本
  resource,        // 资源类型
  verb            // 操作动词: get, list, create, update, delete, etc.
) -> bool
```

---

#### 7. variables 变量

访问自定义变量（v1.28+）。

```yaml
spec:
  variables:
    - name: hasResourceLimits
      expression: |
        object.spec.template.spec.containers.all(c,
          has(c.resources.limits)
        )
    
    - name: totalMemoryRequests
      expression: |
        object.spec.template.spec.containers
        .map(c, has(c.resources.requests.memory) ? resource.quantity(c.resources.requests.memory) : resource.quantity("0"))
        .sum()
  
  validations:
    # 使用变量
    - expression: "variables.hasResourceLimits"
      message: "必须设置资源限制"
    
    - expression: "variables.totalMemoryRequests <= resource.quantity('100Gi')"
      message: "总内存请求超过限制"
```

---

### 常用 CEL 函数

#### 字符串函数

```yaml
validations:
  # 前缀/后缀检查
  - expression: "object.metadata.name.startsWith('prod-')"
  - expression: "object.metadata.name.endsWith('-v1')"
  
  # 包含检查
  - expression: "object.spec.template.spec.containers[0].image.contains('redis')"
  
  # 正则匹配
  - expression: "object.metadata.name.matches('^[a-z0-9-]+$')"
  - expression: "object.spec.template.spec.containers[0].image.matches('^registry\\.company\\.com/[^:]+:v\\d+\\.\\d+\\.\\d+$')"
  
  # 字符串转换
  - expression: "object.metadata.name.toLowerCase() == 'production'"
  - expression: "object.metadata.name.toUpperCase() == 'PROD'"
  
  # 字符串分割
  - expression: |
      params.data.allowedRegistries.split(',').exists(r,
        object.spec.template.spec.containers[0].image.startsWith(r.trim())
      )
  
  # 字符串替换
  - expression: "object.metadata.name.replace('-', '_') == 'prod_app'"
  
  # 字符串拼接
  - expression: "object.metadata.namespace + '-' + object.metadata.name == 'default-myapp'"
```

---

#### 集合函数

```yaml
validations:
  # all(): 所有元素满足条件
  - expression: |
      object.spec.template.spec.containers.all(c,
        has(c.resources.limits)
      )
  
  # exists(): 存在元素满足条件
  - expression: |
      object.spec.template.spec.containers.exists(c,
        c.name == 'nginx'
      )
  
  # exists_one(): 仅有一个元素满足条件
  - expression: |
      object.spec.template.spec.containers.exists_one(c,
        c.name == 'main'
      )
  
  # map(): 映射转换
  - expression: |
      object.spec.template.spec.containers.map(c, c.name).size() == 3
  
  - expression: |
      object.spec.template.spec.containers.map(c,
        has(c.resources.requests.memory) ? resource.quantity(c.resources.requests.memory) : resource.quantity("0")
      ).sum() <= resource.quantity("10Gi")
  
  # filter(): 过滤元素
  - expression: |
      object.spec.template.spec.containers.filter(c,
        has(c.resources.limits)
      ).size() == object.spec.template.spec.containers.size()
  
  # size(): 集合大小
  - expression: "object.spec.template.spec.containers.size() <= 5"
  
  # sum(): 求和（需要配合 map）
  - expression: |
      object.spec.template.spec.containers.map(c, 1).sum() <= 10
```

---

#### 类型检查和转换

```yaml
validations:
  # has(): 字段是否存在
  - expression: "has(object.metadata.labels)"
  - expression: "has(object.spec.template.spec.containers[0].resources.limits.memory)"
  
  # in: 成员检查
  - expression: "'app' in object.metadata.labels"
  - expression: "request.operation in ['CREATE', 'UPDATE']"
  
  # type(): 获取类型
  - expression: "type(object.spec.replicas) == int"
  
  # 类型转换
  - expression: "int(params.data.maxReplicas) > 0"
  - expression: "double(object.spec.progressDeadlineSeconds) / 60.0 > 5.0"
  - expression: "string(object.spec.replicas) == '3'"
  - expression: "bool(params.data.enforcePolicy) == true"
  
  # 可选值处理（三元运算）
  - expression: |
      has(object.spec.replicas) ? object.spec.replicas <= 10 : true
```

---

#### 资源数量函数

Kubernetes 资源数量（如 CPU、内存）有特殊的处理函数。

```yaml
validations:
  # resource.quantity(): 解析资源数量
  - expression: |
      resource.quantity(object.spec.template.spec.containers[0].resources.limits.memory) <= resource.quantity("8Gi")
  
  # 数量比较
  - expression: |
      resource.quantity("1Gi") < resource.quantity("1024Mi")  // false（相等）
  
  - expression: |
      resource.quantity("1000m") == resource.quantity("1")    // true（1000 millicores = 1 core）
  
  # 数量加法
  - expression: |
      object.spec.template.spec.containers.map(c,
        has(c.resources.requests.cpu) ? resource.quantity(c.resources.requests.cpu) : resource.quantity("0")
      ).sum() <= resource.quantity("16")
  
  # asInteger(): 转换为整数（最小单位）
  - expression: |
      resource.quantity("1Gi").asInteger() == 1073741824
  
  - expression: |
      resource.quantity("1").asInteger() == 1000  // 1 CPU = 1000 millicores
```

**资源数量单位**:

| 类型 | 单位 | 示例 |
|------|------|------|
| CPU | millicores (m) | `100m`, `1`, `2.5` |
| 内存 | Ki, Mi, Gi, Ti | `128Mi`, `1Gi`, `10Ti` |
| 存储 | Ki, Mi, Gi, Ti, Pi | `10Gi`, `1Ti` |

---

### 完整 CEL 示例：生产级验证

```yaml
apiVersion: admissionregistration.k8s.io/v1
kind: ValidatingAdmissionPolicy
metadata:
  name: production-deployment-policy
spec:
  matchConstraints:
    resourceRules:
      - apiGroups: ["apps"]
        apiVersions: ["v1"]
        resources: ["deployments"]
        operations: ["CREATE", "UPDATE"]
  
  paramKind:
    apiVersion: v1
    kind: ConfigMap
  
  # 定义可复用变量
  variables:
    # 是否为生产环境
    - name: isProduction
      expression: |
        has(namespaceObject.metadata.labels) &&
        'environment' in namespaceObject.metadata.labels &&
        namespaceObject.metadata.labels['environment'] == 'production'
    
    # 所有容器
    - name: allContainers
      expression: "object.spec.template.spec.containers"
    
    # 是否所有容器都设置了资源限制
    - name: hasResourceLimits
      expression: |
        variables.allContainers.all(c,
          has(c.resources.limits) &&
          has(c.resources.limits.cpu) &&
          has(c.resources.limits.memory)
        )
    
    # 是否所有镜像来自受信任的仓库
    - name: allImagesFromTrustedRegistry
      expression: |
        params.data.trustedRegistries.split(',').exists(r,
          variables.allContainers.all(c,
            c.image.startsWith(r.trim())
          )
        )
    
    # 总 CPU 请求
    - name: totalCPURequests
      expression: |
        variables.allContainers.map(c,
          has(c.resources.requests.cpu) ? resource.quantity(c.resources.requests.cpu) : resource.quantity("0")
        ).sum()
    
    # 总内存请求
    - name: totalMemoryRequests
      expression: |
        variables.allContainers.map(c,
          has(c.resources.requests.memory) ? resource.quantity(c.resources.requests.memory) : resource.quantity("0")
        ).sum()
  
  validations:
    # 验证 1: 生产环境副本数至少为 2
    - expression: |
        !variables.isProduction ||
        object.spec.replicas >= 2
      message: "生产环境 Deployment 副本数至少为 2"
      reason: Invalid
    
    # 验证 2: 副本数不超过配置限制
    - expression: |
        object.spec.replicas <= int(params.data.maxReplicas)
      messageExpression: |
        "副本数 " + string(object.spec.replicas) + 
        " 超过最大限制 " + params.data.maxReplicas
      reason: Invalid
    
    # 验证 3: 生产环境必须设置资源限制
    - expression: |
        !variables.isProduction ||
        variables.hasResourceLimits
      message: "生产环境所有容器必须设置 CPU 和内存限制"
      reason: Required
    
    # 验证 4: 镜像必须来自受信任的仓库
    - expression: "variables.allImagesFromTrustedRegistry"
      messageExpression: |
        "镜像必须来自以下仓库之一: " + params.data.trustedRegistries
      reason: Forbidden
    
    # 验证 5: 总 CPU 请求不超过限制
    - expression: |
        variables.totalCPURequests <= resource.quantity(params.data.maxTotalCPU)
      messageExpression: |
        "总 CPU 请求 " + string(variables.totalCPURequests.asInteger()) + 
        "m 超过限制 " + params.data.maxTotalCPU
      reason: Invalid
    
    # 验证 6: 总内存请求不超过限制
    - expression: |
        variables.totalMemoryRequests <= resource.quantity(params.data.maxTotalMemory)
      messageExpression: |
        "总内存请求超过限制 " + params.data.maxTotalMemory
      reason: Invalid
    
    # 验证 7: 必须设置 app 和 version 标签
    - expression: |
        has(object.metadata.labels) &&
        'app' in object.metadata.labels &&
        'version' in object.metadata.labels
      message: "必须设置 'app' 和 'version' 标签"
      reason: Required
    
    # 验证 8: 镜像必须使用特定标签格式（不允许 latest）
    - expression: |
        variables.allContainers.all(c,
          c.image.contains(':') &&
          !c.image.endsWith(':latest')
        )
      message: "所有镜像必须指定版本标签，不允许使用 'latest'"
      reason: Invalid
    
    # 验证 9: 不可变标签（仅更新时检查）
    - expression: |
        !has(oldObject) ||
        !has(oldObject.metadata.labels) ||
        !'immutable-id' in oldObject.metadata.labels ||
        object.metadata.labels['immutable-id'] == oldObject.metadata.labels['immutable-id']
      message: "标签 'immutable-id' 创建后不可修改"
      reason: Forbidden
    
    # 验证 10: 生产环境不允许特权容器
    - expression: |
        !variables.isProduction ||
        variables.allContainers.all(c,
          !has(c.securityContext) ||
          !has(c.securityContext.privileged) ||
          c.securityContext.privileged == false
        )
      message: "生产环境不允许使用特权容器"
      reason: Forbidden
  
  # 审计注解
  auditAnnotations:
    - key: "validated-replicas"
      valueExpression: "string(object.spec.replicas)"
    
    - key: "total-cpu-requests"
      valueExpression: "string(variables.totalCPURequests.asInteger()) + 'm'"
    
    - key: "total-memory-requests"
      valueExpression: "string(variables.totalMemoryRequests.asInteger() / 1048576) + 'Mi'"
    
    - key: "is-production"
      valueExpression: "string(variables.isProduction)"
    
    - key: "validated-by-user"
      valueExpression: "request.userInfo.username"
  
  failurePolicy: Fail
```

---

## 内部原理

### CEL 编译和缓存

```
策略加载
   │
   ├─► [1] 解析 YAML 配置
   │
   ├─► [2] 编译 CEL 表达式
   │      ├─► 语法检查
   │      ├─► 类型推断
   │      ├─► 生成执行计划
   │      └─► 优化表达式
   │
   ├─► [3] 缓存编译结果
   │      └─► 按策略名称缓存
   │
   └─► [4] 请求时执行
          ├─► 加载缓存的编译结果
          ├─► 绑定变量（object, params, etc.）
          ├─► 执行表达式（微秒级）
          └─► 返回结果
```

**性能优化机制**:
- ✅ **编译缓存**: 表达式编译后缓存，避免重复编译
- ✅ **短路求值**: `&&` 和 `||` 支持短路
- ✅ **懒加载**: 仅在需要时加载参数对象
- ✅ **并发执行**: 多个策略并行评估
- ✅ **超时保护**: 单个表达式默认超时 3 秒

---

### 与 Webhook 对比

| 维度 | ValidatingAdmissionPolicy | ValidatingWebhook |
|------|---------------------------|-------------------|
| **部署复杂度** | 低（仅 YAML） | 高（代码 + 服务 + 证书） |
| **性能** | 微秒级（进程内） | 毫秒级（网络调用） |
| **可靠性** | 高（无外部依赖） | 中（依赖网络和外部服务） |
| **故障影响** | 无外部故障风险 | 外部服务故障可能阻塞 API |
| **网络延迟** | 无 | 有（1-100ms） |
| **TLS 证书** | 不需要 | 需要 |
| **服务监控** | 内置指标 | 需要自建监控 |
| **灵活性** | 中（CEL 限制） | 高（任意代码） |
| **安全性** | 高（沙箱隔离） | 取决于实现 |
| **维护成本** | 低 | 高 |
| **适用场景** | 标准验证规则 | 复杂业务逻辑 |

**性能对比（典型场景）**:

```
ValidatingAdmissionPolicy:
  编译时间:    1-5ms（仅首次）
  执行时间:    10-100μs
  端到端延迟:  < 1ms

ValidatingWebhook:
  网络延迟:    1-10ms
  服务处理:    5-50ms
  TLS 握手:    1-5ms
  端到端延迟:  10-100ms

性能提升: 10-100 倍
```

---

### 错误处理和失败模式

```
表达式执行
   │
   ├─► 情况 1: 表达式返回 true
   │      └─► ✅ 验证通过
   │
   ├─► 情况 2: 表达式返回 false
   │      └─► ❌ 验证失败
   │          ├─► Deny: 拒绝请求
   │          ├─► Warn: 返回警告
   │          └─► Audit: 记录审计日志
   │
   ├─► 情况 3: 表达式执行出错（语法错误、类型错误、超时）
   │      └─► 根据 failurePolicy 处理
   │          ├─► Fail: 拒绝请求
   │          └─► Ignore: 忽略策略
   │
   └─► 情况 4: 参数对象未找到
          └─► 根据 parameterNotFoundAction 处理
              ├─► Allow: 跳过策略
              └─► Deny: 拒绝请求
```

---

## 版本兼容性

| 版本 | 状态 | 特性 |
|------|------|------|
| **v1.26** | Beta | 首次引入 |
| **v1.27** | Beta | 新增 `validationActions`（Deny/Warn/Audit）<br/>新增 `matchConditions` |
| **v1.28** | Beta | 新增 `variables`<br/>新增 `authorizer` 变量<br/>新增 `parameterNotFoundAction` |
| **v1.29** | Beta | 性能优化<br/>改进错误消息 |
| **v1.30** | **GA** | 正式版，API 稳定 |

### Feature Gate

| 版本 | Feature Gate | 默认值 |
|------|--------------|--------|
| v1.26-v1.27 | `ValidatingAdmissionPolicy` | `false` |
| v1.28-v1.29 | `ValidatingAdmissionPolicy` | `true` |
| v1.30+ | N/A | GA，无需 Feature Gate |

**启用 Feature Gate（v1.26-v1.29）**:

```yaml
# kube-apiserver 配置
apiVersion: v1
kind: Pod
metadata:
  name: kube-apiserver
spec:
  containers:
    - name: kube-apiserver
      command:
        - kube-apiserver
        - --feature-gates=ValidatingAdmissionPolicy=true
```

---

## 最佳实践

### 1. 策略设计原则

**DO ✅**:
- ✅ 使用描述性的策略和绑定名称
- ✅ 将通用验证逻辑提取为变量
- ✅ 使用 `messageExpression` 提供详细的错误上下文
- ✅ 在 `auditAnnotations` 中记录关键验证信息
- ✅ 使用参数化配置支持多环境
- ✅ 渐进式推广：Audit → Warn → Deny

**DON'T ❌**:
- ❌ 避免过于复杂的 CEL 表达式（超过 50 行）
- ❌ 避免在 CEL 中硬编码配置值
- ❌ 避免重复的验证逻辑
- ❌ 避免在生产环境直接使用 `Deny`（未经测试）

---

### 2. 性能优化

```yaml
spec:
  # 优化 1: 使用 matchConditions 快速过滤
  matchConditions:
    - name: "skip-system-users"
      expression: "!request.userInfo.username.startsWith('system:')"
    
    - name: "skip-delete-operations"
      expression: "request.operation != 'DELETE'"
  
  # 优化 2: 提取复杂计算为变量（仅计算一次）
  variables:
    - name: totalCPU
      expression: |
        object.spec.template.spec.containers.map(c,
          resource.quantity(c.resources.requests.cpu)
        ).sum()
  
  validations:
    # 优化 3: 使用短路求值
    - expression: |
        !has(object.metadata.labels) ||
        'skip-validation' in object.metadata.labels ||
        object.spec.replicas <= 10
    
    # 优化 4: 避免嵌套循环
    # ❌ 不推荐
    - expression: |
        object.spec.template.spec.containers.all(c,
          object.spec.template.spec.initContainers.all(ic,
            c.name != ic.name
          )
        )
    
    # ✅ 推荐
    - expression: |
        variables.containerNames.size() + variables.initContainerNames.size() ==
        (variables.containerNames + variables.initContainerNames).unique().size()
```

---

### 3. 安全性最佳实践

```yaml
# 安全策略示例
---
apiVersion: admissionregistration.k8s.io/v1
kind: ValidatingAdmissionPolicy
metadata:
  name: security-hardening
spec:
  matchConstraints:
    resourceRules:
      - apiGroups: ["apps", ""]
        apiVersions: ["v1"]
        resources: ["deployments", "pods"]
        operations: ["CREATE", "UPDATE"]
  
  variables:
    - name: allContainers
      expression: |
        object.kind == 'Pod' 
        ? object.spec.containers 
        : object.spec.template.spec.containers
  
  validations:
    # 1. 禁止特权容器
    - expression: |
        variables.allContainers.all(c,
          !has(c.securityContext) ||
          !has(c.securityContext.privileged) ||
          c.securityContext.privileged == false
        )
      message: "禁止使用特权容器"
      reason: Forbidden
    
    # 2. 禁止 hostNetwork
    - expression: |
        !has(object.spec.hostNetwork) ||
        object.spec.hostNetwork == false
      message: "禁止使用 hostNetwork"
      reason: Forbidden
    
    # 3. 禁止 hostPID/hostIPC
    - expression: |
        (!has(object.spec.hostPID) || object.spec.hostPID == false) &&
        (!has(object.spec.hostIPC) || object.spec.hostIPC == false)
      message: "禁止使用 hostPID 或 hostIPC"
      reason: Forbidden
    
    # 4. 要求非 root 用户运行
    - expression: |
        variables.allContainers.all(c,
          has(c.securityContext) &&
          has(c.securityContext.runAsNonRoot) &&
          c.securityContext.runAsNonRoot == true
        )
      message: "容器必须以非 root 用户运行"
      reason: Forbidden
    
    # 5. 禁止挂载敏感主机路径
    - expression: |
        !has(object.spec.volumes) ||
        object.spec.volumes.all(v,
          !has(v.hostPath) ||
          !v.hostPath.path.startsWith('/') ||
          (!v.hostPath.path.startsWith('/etc') &&
           !v.hostPath.path.startsWith('/var/run/docker.sock') &&
           !v.hostPath.path.startsWith('/proc'))
        )
      message: "禁止挂载敏感主机路径"
      reason: Forbidden
    
    # 6. 镜像必须来自受信任的仓库
    - expression: |
        variables.allContainers.all(c,
          c.image.startsWith('registry.company.com/') ||
          c.image.startsWith('gcr.io/company-')
        )
      message: "仅允许使用受信任的镜像仓库"
      reason: Forbidden
    
    # 7. 必须设置 seccomp profile
    - expression: |
        has(object.spec.securityContext) &&
        has(object.spec.securityContext.seccompProfile) &&
        object.spec.securityContext.seccompProfile.type in ['RuntimeDefault', 'Localhost']
      message: "必须设置 seccomp profile"
      reason: Required
  
  failurePolicy: Fail
```

---

### 4. 多环境管理

```yaml
# 环境配置模式
---
# 开发环境参数
apiVersion: v1
kind: ConfigMap
metadata:
  name: policy-config
  namespace: dev-config
  labels:
    environment: development
data:
  maxReplicas: "3"
  enforceResourceLimits: "false"
  trustedRegistries: "registry.dev.company.com,docker.io"

---
# 生产环境参数
apiVersion: v1
kind: ConfigMap
metadata:
  name: policy-config
  namespace: prod-config
  labels:
    environment: production
data:
  maxReplicas: "20"
  enforceResourceLimits: "true"
  trustedRegistries: "registry.prod.company.com"

---
# 通用策略（适用所有环境）
apiVersion: admissionregistration.k8s.io/v1
kind: ValidatingAdmissionPolicy
metadata:
  name: deployment-policy
spec:
  paramKind:
    apiVersion: v1
    kind: ConfigMap
  
  matchConstraints:
    resourceRules:
      - apiGroups: ["apps"]
        apiVersions: ["v1"]
        resources: ["deployments"]
        operations: ["CREATE", "UPDATE"]
  
  validations:
    - expression: "object.spec.replicas <= int(params.data.maxReplicas)"
      message: "副本数超过限制"
    
    - expression: |
        params.data.enforceResourceLimits == 'false' ||
        object.spec.template.spec.containers.all(c, has(c.resources.limits))
      message: "必须设置资源限制"

---
# 开发环境绑定
apiVersion: admissionregistration.k8s.io/v1
kind: ValidatingAdmissionPolicyBinding
metadata:
  name: deployment-policy-dev
spec:
  policyName: deployment-policy
  paramRef:
    name: policy-config
    namespace: dev-config
  matchResources:
    namespaceSelector:
      matchLabels:
        environment: development
  validationActions:
    - Warn
    - Audit

---
# 生产环境绑定
apiVersion: admissionregistration.k8s.io/v1
kind: ValidatingAdmissionPolicyBinding
metadata:
  name: deployment-policy-prod
spec:
  policyName: deployment-policy
  paramRef:
    name: policy-config
    namespace: prod-config
  matchResources:
    namespaceSelector:
      matchLabels:
        environment: production
  validationActions:
    - Deny
    - Audit
```

---

### 5. 测试和验证

```bash
# 1. 测试策略（DryRun 模式）
kubectl apply -f deployment.yaml --dry-run=server

# 2. 查看策略状态
kubectl get validatingadmissionpolicies
kubectl describe validatingadmissionpolicy <policy-name>

# 3. 查看绑定状态
kubectl get validatingadmissionpolicybindings
kubectl describe validatingadmissionpolicybinding <binding-name>

# 4. 查看审计日志
kubectl logs -n kube-system kube-apiserver-xxx | grep -i "policy.k8s.io"

# 5. 查看指标
kubectl get --raw /metrics | grep admission_policy

# 6. 测试参数更新
kubectl edit configmap policy-config

# 7. 测试策略禁用（临时）
kubectl label namespace my-namespace policy.kubernetes.io/exempt=true
```

---

## 生产案例

### 案例 1: 镜像来源限制

**需求**: 所有容器镜像必须来自公司内部镜像仓库。

```yaml
---
apiVersion: admissionregistration.k8s.io/v1
kind: ValidatingAdmissionPolicy
metadata:
  name: trusted-image-registry
spec:
  matchConstraints:
    resourceRules:
      - apiGroups: ["apps", "batch", ""]
        apiVersions: ["v1"]
        resources: ["deployments", "statefulsets", "daemonsets", "jobs", "cronjobs", "pods"]
        operations: ["CREATE", "UPDATE"]
  
  paramKind:
    apiVersion: v1
    kind: ConfigMap
  
  variables:
    # 提取所有容器（包括 init containers）
    - name: allContainers
      expression: |
        (object.kind == 'Pod' ? object.spec.containers : object.spec.template.spec.containers) +
        (object.kind == 'Pod' && has(object.spec.initContainers) ? object.spec.initContainers : 
         has(object.spec.template.spec.initContainers) ? object.spec.template.spec.initContainers : [])
    
    # 提取所有镜像
    - name: allImages
      expression: "variables.allContainers.map(c, c.image)"
    
    # 受信任的仓库列表
    - name: trustedRegistries
      expression: "params.data.registries.split(',')"
  
  validations:
    # 验证所有镜像来自受信任的仓库
    - expression: |
        variables.allImages.all(img,
          variables.trustedRegistries.exists(reg,
            img.startsWith(reg.trim())
          )
        )
      messageExpression: |
        "镜像必须来自以下仓库之一: " + params.data.registries + "。" +
        "当前镜像: " + variables.allImages.join(", ")
      reason: Forbidden
    
    # 验证镜像不使用 latest 标签
    - expression: |
        variables.allImages.all(img,
          img.contains(':') && !img.endsWith(':latest')
        )
      message: "镜像必须指定具体版本标签，不允许使用 'latest'"
      reason: Invalid
  
  auditAnnotations:
    - key: "validated-images"
      valueExpression: "variables.allImages.join(',')"
    
    - key: "image-count"
      valueExpression: "string(variables.allImages.size())"
  
  failurePolicy: Fail

---
# 参数配置
apiVersion: v1
kind: ConfigMap
metadata:
  name: trusted-registries
  namespace: default
data:
  # 逗号分隔的受信任仓库列表
  registries: "registry.company.com/,gcr.io/company-,quay.io/company/"

---
# 全局绑定（排除 kube-system）
apiVersion: admissionregistration.k8s.io/v1
kind: ValidatingAdmissionPolicyBinding
metadata:
  name: trusted-image-registry-global
spec:
  policyName: trusted-image-registry
  paramRef:
    name: trusted-registries
    namespace: default
  
  matchResources:
    namespaceSelector:
      matchExpressions:
        # 排除系统命名空间
        - key: kubernetes.io/metadata.name
          operator: NotIn
          values: ["kube-system", "kube-public", "kube-node-lease"]
  
  validationActions:
    - Deny
    - Audit
```

---

### 案例 2: 强制标签规范

**需求**: 生产环境 Deployment 必须设置特定标签。

```yaml
---
apiVersion: admissionregistration.k8s.io/v1
kind: ValidatingAdmissionPolicy
metadata:
  name: required-labels
spec:
  matchConstraints:
    resourceRules:
      - apiGroups: ["apps"]
        apiVersions: ["v1"]
        resources: ["deployments", "statefulsets"]
        operations: ["CREATE", "UPDATE"]
  
  paramKind:
    apiVersion: v1
    kind: ConfigMap
  
  variables:
    # 检查命名空间是否为生产环境
    - name: isProduction
      expression: |
        has(namespaceObject.metadata.labels) &&
        'environment' in namespaceObject.metadata.labels &&
        namespaceObject.metadata.labels['environment'] == 'production'
    
    # 必需的标签列表
    - name: requiredLabels
      expression: "params.data.labels.split(',')"
    
    # 缺失的标签
    - name: missingLabels
      expression: |
        variables.requiredLabels.filter(label,
          !has(object.metadata.labels) ||
          !label.trim() in object.metadata.labels
        )
  
  validations:
    # 验证必需标签存在
    - expression: |
        !variables.isProduction ||
        variables.missingLabels.size() == 0
      messageExpression: |
        "生产环境缺少以下必需标签: " + variables.missingLabels.join(", ")
      reason: Required
    
    # 验证 app 标签格式
    - expression: |
        !variables.isProduction ||
        !has(object.metadata.labels) ||
        !'app' in object.metadata.labels ||
        object.metadata.labels['app'].matches('^[a-z0-9-]+$')
      message: "标签 'app' 格式无效，仅允许小写字母、数字和连字符"
      reason: Invalid
    
    # 验证 version 标签格式（语义化版本）
    - expression: |
        !variables.isProduction ||
        !has(object.metadata.labels) ||
        !'version' in object.metadata.labels ||
        object.metadata.labels['version'].matches('^v\\d+\\.\\d+\\.\\d+$')
      message: "标签 'version' 必须遵循语义化版本格式 (vX.Y.Z)"
      reason: Invalid
    
    # 验证 team 标签值
    - expression: |
        !variables.isProduction ||
        !has(object.metadata.labels) ||
        !'team' in object.metadata.labels ||
        object.metadata.labels['team'] in params.data.validTeams.split(',')
      messageExpression: |
        "标签 'team' 值无效。允许的值: " + params.data.validTeams
      reason: Invalid
  
  auditAnnotations:
    - key: "is-production"
      valueExpression: "string(variables.isProduction)"
    
    - key: "missing-labels"
      valueExpression: |
        variables.missingLabels.size() > 0 
        ? variables.missingLabels.join(",") 
        : "none"
  
  failurePolicy: Fail

---
# 参数配置
apiVersion: v1
kind: ConfigMap
metadata:
  name: label-requirements
  namespace: default
data:
  # 必需的标签（逗号分隔）
  labels: "app,version,team,environment"
  
  # 有效的团队名称
  validTeams: "platform,backend,frontend,data,infra"

---
# 绑定
apiVersion: admissionregistration.k8s.io/v1
kind: ValidatingAdmissionPolicyBinding
metadata:
  name: required-labels-binding
spec:
  policyName: required-labels
  paramRef:
    name: label-requirements
    namespace: default
  
  validationActions:
    - Deny
    - Audit
```

---

### 案例 3: 副本数上限控制

**需求**: 根据命名空间配额动态限制 Deployment 副本数。

```yaml
---
# 自定义 CRD: 命名空间配额
apiVersion: apiextensions.k8s.io/v1
kind: CustomResourceDefinition
metadata:
  name: namespacequotas.policy.company.com
spec:
  group: policy.company.com
  names:
    kind: NamespaceQuota
    plural: namespacequotas
    singular: namespacequota
  scope: Namespaced
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
                maxReplicasPerDeployment:
                  type: integer
                maxTotalReplicas:
                  type: integer
                maxCPUPerPod:
                  type: string
                maxMemoryPerPod:
                  type: string

---
# 创建命名空间配额
apiVersion: policy.company.com/v1
kind: NamespaceQuota
metadata:
  name: quota
  namespace: production-app
spec:
  maxReplicasPerDeployment: 10
  maxTotalReplicas: 50
  maxCPUPerPod: "4"
  maxMemoryPerPod: "8Gi"

---
# 策略定义
apiVersion: admissionregistration.k8s.io/v1
kind: ValidatingAdmissionPolicy
metadata:
  name: replica-count-limits
spec:
  matchConstraints:
    resourceRules:
      - apiGroups: ["apps"]
        apiVersions: ["v1"]
        resources: ["deployments"]
        operations: ["CREATE", "UPDATE"]
  
  paramKind:
    apiVersion: policy.company.com/v1
    kind: NamespaceQuota
  
  variables:
    # 单个 Pod 的资源请求
    - name: podCPURequest
      expression: |
        object.spec.template.spec.containers.map(c,
          has(c.resources.requests.cpu) ? resource.quantity(c.resources.requests.cpu) : resource.quantity("0")
        ).sum()
    
    - name: podMemoryRequest
      expression: |
        object.spec.template.spec.containers.map(c,
          has(c.resources.requests.memory) ? resource.quantity(c.resources.requests.memory) : resource.quantity("0")
        ).sum()
    
    # 总资源请求（副本数 × 单 Pod 请求）
    - name: totalCPURequest
      expression: |
        variables.podCPURequest.asInteger() * object.spec.replicas
    
    - name: totalMemoryRequest
      expression: |
        variables.podMemoryRequest.asInteger() * object.spec.replicas
  
  validations:
    # 验证单个 Deployment 副本数上限
    - expression: |
        object.spec.replicas <= params.spec.maxReplicasPerDeployment
      messageExpression: |
        "Deployment 副本数 " + string(object.spec.replicas) + 
        " 超过命名空间限制 " + string(params.spec.maxReplicasPerDeployment)
      reason: Invalid
    
    # 验证单个 Pod CPU 上限
    - expression: |
        variables.podCPURequest <= resource.quantity(params.spec.maxCPUPerPod)
      messageExpression: |
        "Pod CPU 请求 " + string(variables.podCPURequest.asInteger()) + 
        "m 超过上限 " + params.spec.maxCPUPerPod
      reason: Invalid
    
    # 验证单个 Pod 内存上限
    - expression: |
        variables.podMemoryRequest <= resource.quantity(params.spec.maxMemoryPerPod)
      messageExpression: |
        "Pod 内存请求超过上限 " + params.spec.maxMemoryPerPod
      reason: Invalid
  
  auditAnnotations:
    - key: "deployment-replicas"
      valueExpression: "string(object.spec.replicas)"
    
    - key: "total-cpu-request"
      valueExpression: "string(variables.totalCPURequest) + 'm'"
    
    - key: "total-memory-request"
      valueExpression: "string(variables.totalMemoryRequest / 1048576) + 'Mi'"
    
    - key: "namespace-quota-used"
      valueExpression: |
        string(object.spec.replicas) + "/" + string(params.spec.maxReplicasPerDeployment)
  
  failurePolicy: Fail

---
# 绑定（自动选择命名空间内的 NamespaceQuota）
apiVersion: admissionregistration.k8s.io/v1
kind: ValidatingAdmissionPolicyBinding
metadata:
  name: replica-limits-binding
spec:
  policyName: replica-count-limits
  
  # 使用选择器自动匹配命名空间内的 NamespaceQuota
  paramRef:
    selector:
      matchLabels: {}
    parameterNotFoundAction: Deny  # 没有配额则拒绝
  
  validationActions:
    - Deny
    - Audit
```

---

## 常见问题 FAQ

### Q1: ValidatingAdmissionPolicy 与 OPA/Gatekeeper 如何选择？

**对比**:

| 维度 | ValidatingAdmissionPolicy | OPA/Gatekeeper |
|------|---------------------------|----------------|
| 安装 | 无需安装（内置） | 需要安装 Operator |
| 语言 | CEL | Rego |
| 性能 | 极高（进程内） | 高（进程内） |
| 生态 | 较新（2022+） | 成熟（2017+） |
| 社区策略库 | 较少 | 丰富 |
| 复杂逻辑 | 中等 | 强大 |
| 学习曲线 | 平缓 | 陡峭 |

**选择建议**:
- **使用 ValidatingAdmissionPolicy**: 标准验证场景、简单策略、追求性能
- **使用 OPA/Gatekeeper**: 复杂策略、需要丰富的策略库、团队熟悉 Rego

---

### Q2: 如何调试 CEL 表达式错误？

**方法 1: 使用 DryRun 测试**

```bash
# 测试部署（不实际创建）
kubectl apply -f deployment.yaml --dry-run=server -v=8
```

**方法 2: 查看 API Server 日志**

```bash
# 查看详细错误信息
kubectl logs -n kube-system kube-apiserver-xxx | grep -i cel
```

**方法 3: 使用 Audit 模式先观察**

```yaml
validationActions:
  - Audit  # 不拒绝请求，仅记录日志
```

**方法 4: 分步调试表达式**

```yaml
# 从简单表达式开始
- expression: "object.spec.replicas > 0"  # ✅ 通过
- expression: "object.spec.replicas <= 10"  # ✅ 通过
- expression: "has(object.spec.template.spec.containers)"  # ✅ 通过
# 逐步增加复杂度
```

**常见错误**:

| 错误 | 原因 | 解决方案 |
|------|------|----------|
| `no such key: field` | 字段不存在 | 使用 `has(object.field)` 检查 |
| `type mismatch` | 类型错误 | 使用 `int()`, `string()` 转换 |
| `invalid syntax` | 语法错误 | 检查括号、引号匹配 |
| `deadline exceeded` | 表达式超时 | 简化表达式，使用变量 |

---

### Q3: 如何实现渐进式推广？

**推荐流程**:

```yaml
# 阶段 1: 仅审计（1-2 周）
validationActions:
  - Audit

# 收集数据，评估影响范围
kubectl logs -n kube-system kube-apiserver-xxx | grep "policy.k8s.io"

---
# 阶段 2: 警告 + 审计（1-2 周）
validationActions:
  - Warn
  - Audit

# 通知用户，收集反馈

---
# 阶段 3: 拒绝 + 审计（长期运行）
validationActions:
  - Deny
  - Audit

# 强制执行，持续监控

---
# 阶段 4: 仅拒绝（可选）
validationActions:
  - Deny
```

---

### Q4: 参数对象更新后策略何时生效？

**答案**: 立即生效（无需重启 API Server）。

**原理**:
- API Server 会监听参数对象变化
- 参数更新触发策略缓存刷新
- 下一个请求使用新参数

**验证方法**:

```bash
# 1. 更新参数
kubectl edit configmap policy-config

# 2. 立即测试（应使用新参数）
kubectl apply -f deployment.yaml --dry-run=server
```

---

### Q5: 如何处理参数对象不存在的情况？

**配置 `parameterNotFoundAction`** (v1.28+):

```yaml
spec:
  paramRef:
    name: policy-config
    namespace: default
    
    # 参数未找到时的处理策略
    parameterNotFoundAction: Deny  # 或 Allow（默认）
```

**推荐**:
- 开发环境: `Allow`（容错）
- 生产环境: `Deny`（严格）

---

### Q6: 如何排除特定命名空间或对象？

**方法 1: 在 Binding 中使用 namespaceSelector**

```yaml
spec:
  matchResources:
    namespaceSelector:
      matchExpressions:
        - key: kubernetes.io/metadata.name
          operator: NotIn
          values: ["kube-system", "kube-public"]
```

**方法 2: 在 Policy 中使用 matchConditions**

```yaml
spec:
  matchConditions:
    - name: "exclude-exempt-objects"
      expression: |
        !has(object.metadata.labels) ||
        !'policy.company.com/exempt' in object.metadata.labels ||
        object.metadata.labels['policy.company.com/exempt'] != 'true'
```

**方法 3: 在 Validation 中添加豁免逻辑**

```yaml
validations:
  - expression: |
      (has(object.metadata.labels) && 
       'policy.company.com/exempt' in object.metadata.labels &&
       object.metadata.labels['policy.company.com/exempt'] == 'true') ||
      object.spec.replicas <= 10
```

---

### Q7: CEL 表达式有哪些限制？

**限制**:

| 限制项 | 说明 |
|--------|------|
| 表达式长度 | 建议 < 1000 字符，硬限制 64KB |
| 执行时间 | 默认超时 3 秒 |
| 副作用 | 不允许（无网络、无 IO） |
| 递归 | 不支持 |
| 自定义函数 | 不支持（仅内置函数） |
| 外部数据 | 仅能访问 params 和集群对象 |

**解决方案**:
- 复杂逻辑 → 使用 Webhook
- 需要外部数据 → 预先写入 ConfigMap/CRD
- 需要递归 → 改为迭代

---

### Q8: 如何监控策略执行情况？

**方法 1: Prometheus 指标**

```promql
# 策略评估次数
apiserver_validating_admission_policy_check_total

# 策略评估耗时
apiserver_validating_admission_policy_check_duration_seconds

# 策略失败次数
apiserver_validating_admission_policy_check_total{result="deny"}
```

**方法 2: 审计日志**

```bash
# 查看策略审计日志
kubectl logs -n kube-system kube-apiserver-xxx | grep "policy.k8s.io"
```

**方法 3: 事件日志**

```bash
# 查看验证失败事件
kubectl get events --all-namespaces | grep -i "admission policy"
```

---

## 总结

### 核心要点

1. **声明式验证**: 使用 YAML + CEL 表达式，无需编写代码
2. **高性能**: 进程内执行，比 Webhook 快 10-100 倍
3. **参数化配置**: 一个策略，多种绑定，支持多环境
4. **渐进式推广**: Audit → Warn → Deny
5. **版本支持**: v1.26 Beta, v1.30 GA

### 适用场景

✅ **适合使用 ValidatingAdmissionPolicy**:
- 标准验证规则（副本数、资源限制、标签检查）
- 镜像仓库限制
- 安全基线加固
- 命名规范
- 配置规范检查

❌ **不适合使用 ValidatingAdmissionPolicy**:
- 需要外部 API 调用（如查询数据库）
- 复杂业务逻辑（如多资源关联验证）
- 需要修改对象（应使用 MutatingAdmissionPolicy）
- 需要异步处理

### 快速上手

```bash
# 1. 检查集群版本（需要 v1.26+）
kubectl version --short

# 2. 检查 Feature Gate（v1.26-v1.29）
kubectl get --raw /api | grep admissionregistration.k8s.io

# 3. 创建策略
kubectl apply -f policy.yaml

# 4. 创建绑定
kubectl apply -f binding.yaml

# 5. 测试
kubectl apply -f deployment.yaml --dry-run=server

# 6. 查看状态
kubectl get validatingadmissionpolicies
kubectl describe validatingadmissionpolicy <name>
```

---

## 参考资源

- [Kubernetes 官方文档 - Validating Admission Policy](https://kubernetes.io/docs/reference/access-authn-authz/validating-admission-policy/)
- [CEL Language Specification](https://github.com/google/cel-spec)
- [KEP-3488: CEL for Admission Control](https://github.com/kubernetes/enhancements/tree/master/keps/sig-api-machinery/3488-cel-admission-control)
- [Kubernetes API Reference - ValidatingAdmissionPolicy](https://kubernetes.io/docs/reference/kubernetes-api/extend-resources/validating-admission-policy-v1/)

---

> 💡 **提示**: 本文档基于 Kubernetes v1.30+，部分特性在早期版本可能不可用或处于 Beta 阶段。生产环境使用前请验证集群版本和 Feature Gate 状态。

> 📝 **更新日期**: 2026-02 | **文档版本**: v1.0 | **维护者**: Kubernetes SIG API Machinery
