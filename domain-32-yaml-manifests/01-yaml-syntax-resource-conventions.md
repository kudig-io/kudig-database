# 01 - YAML 语法基础与 Kubernetes 资源通用规范

> **适用版本**: Kubernetes v1.25 - v1.32 | **最后更新**: 2026-02 | **难度**: 入门 → 专家全覆盖

---

## 目录

1. [概述](#1-概述)
2. [YAML 语法精要](#2-yaml-语法精要)
   - 2.1 [基本语法规则](#21-基本语法规则)
   - 2.2 [多行字符串](#22-多行字符串)
   - 2.3 [锚点与别名](#23-锚点与别名)
   - 2.4 [常见陷阱](#24-常见陷阱)
3. [Kubernetes 资源四大顶层字段](#3-kubernetes-资源四大顶层字段)
   - 3.1 [apiVersion](#31-apiversion)
   - 3.2 [kind](#32-kind)
   - 3.3 [metadata](#33-metadata)
   - 3.4 [spec 与 status](#34-spec-与-status)
4. [资源命名规范](#4-资源命名规范)
   - 4.1 [DNS-1123 子域名](#41-dns-1123-子域名)
   - 4.2 [DNS-1123 标签](#42-dns-1123-标签)
   - 4.3 [DNS-1035 标签](#43-dns-1035-标签)
   - 4.4 [RFC 1123 vs RFC 952](#44-rfc-1123-vs-rfc-952)
5. [标签与注解最佳实践](#5-标签与注解最佳实践)
   - 5.1 [推荐标签 (app.kubernetes.io/*)](#51-推荐标签-appkubernetesio)
   - 5.2 [运维标签](#52-运维标签)
   - 5.3 [注解用途](#53-注解用途)
6. [字段验证规则](#6-字段验证规则)
   - 6.1 [必填与可选字段](#61-必填与可选字段)
   - 6.2 [不可变字段](#62-不可变字段)
   - 6.3 [Server-side Field Validation (v1.25+)](#63-server-side-field-validation-v125)
7. [kubectl 操作与内部原理](#7-kubectl-操作与内部原理)
   - 7.1 [apply vs create vs replace](#71-apply-vs-create-vs-replace)
   - 7.2 [Server-side Apply (v1.22+ GA)](#72-server-side-apply-v122-ga)
8. [资源版本演进](#8-资源版本演进)
   - 8.1 [API 版本生命周期](#81-api-版本生命周期)
   - 8.2 [版本迁移策略](#82-版本迁移策略)
9. [快速查询索引表](#9-快速查询索引表)
   - 9.1 [按 apiVersion 分组的资源清单](#91-按-apiversion-分组的资源清单)
   - 9.2 [常用 kubectl 命令速查](#92-常用-kubectl-命令速查)
10. [相关资源](#10-相关资源)

---

## 1. 概述

本指南涵盖了在 Kubernetes 环境中使用 YAML 的全部核心知识，从基础语法到高级配置规范，旨在帮助从入门到专家级别的用户掌握：

- **YAML 语法基础**：掌握 YAML 的核心语法规则、多行字符串处理、锚点别名等高级特性
- **Kubernetes 资源结构**：深入理解 `apiVersion`、`kind`、`metadata`、`spec` 四大顶层字段的设计理念
- **命名与标签规范**：学习 DNS-1123、DNS-1035 等命名标准，掌握推荐标签体系
- **字段验证与演进**：了解必填/可选/不可变字段规则，以及 API 版本生命周期管理
- **kubectl 内部机制**：掌握 `apply`/`create`/`replace` 的区别，理解 Server-side Apply 工作原理

**为什么 YAML 在 Kubernetes 中至关重要？**

1. **声明式配置的基石**：Kubernetes 采用声明式 API 设计，YAML 是描述期望状态的标准格式
2. **版本控制友好**：纯文本格式便于 Git 管理，支持 GitOps 工作流
3. **人类可读性强**：相比 JSON，YAML 更简洁直观，支持注释
4. **工具链生态**：Helm、Kustomize、kubectl 等工具都以 YAML 作为输入

---

## 2. YAML 语法精要

### 2.1 基本语法规则

#### 缩进规则（核心要点）

```yaml
# ✅ 正确：使用空格缩进（推荐 2 空格）
apiVersion: v1
kind: Pod
metadata:
  name: nginx-pod  # 2 空格缩进
  labels:
    app: nginx     # 4 空格缩进（嵌套层级）
    tier: frontend

# ❌ 错误：使用 Tab 缩进（YAML 规范禁止）
apiVersion: v1
kind: Pod
metadata:
	name: nginx-pod  # Tab 会导致解析错误
```

**规则要点**：
- **必须使用空格**，严禁使用 Tab 字符（会导致 `yaml: found character that cannot start any token` 错误）
- 推荐统一使用 **2 空格** 作为缩进单位（Kubernetes 社区标准）
- 同一层级必须对齐，子级必须比父级多缩进

#### 键值对（Key-Value Pairs）

```yaml
# 标量值（Scalar Values）
name: nginx-pod                    # 字符串（无需引号）
replica: 3                         # 整数
memory: "512Mi"                    # 字符串（建议加引号避免歧义）
enabled: true                      # 布尔值

# 嵌套映射（Nested Maps）
metadata:
  name: my-app
  labels:
    app: frontend
    version: "1.0"

# 内联映射（Flow Style）- 不推荐在 K8s 中使用
metadata: {name: my-app, namespace: default}
```

#### 列表（Lists）

```yaml
# 块风格列表（推荐）
containers:
  - name: nginx                    # 列表项以 "- " 开头
    image: nginx:1.27
    ports:
      - containerPort: 80          # 嵌套列表
  - name: sidecar
    image: busybox:1.36

# 内联列表（不推荐在 K8s 中使用）
ports: [80, 443, 8080]

# 复杂对象列表
env:
  - name: DATABASE_URL             # 列表项可包含多个字段
    value: "postgres://db:5432"
  - name: API_KEY
    valueFrom:                     # 嵌套对象
      secretKeyRef:
        name: app-secret
        key: api-key
```

#### 注释

```yaml
# 这是单行注释（以 # 开头）
apiVersion: v1  # 行尾注释也支持
kind: ConfigMap
metadata:
  name: example
  # 注释可以出现在任何位置
  labels:
    app: demo  # 注释不会影响解析

# YAML 不支持多行注释，需要每行都加 #
# 第一行注释
# 第二行注释
# 第三行注释
```

### 2.2 多行字符串

#### 字面量块（Literal Block）- 使用 `|` 符号

```yaml
apiVersion: v1
kind: ConfigMap
metadata:
  name: script-config
data:
  # | 保留换行符和尾部空行
  script.sh: |
    #!/bin/bash
    echo "Hello World"
    echo "This is line 2"
    
    # 空行也会保留
    exit 0

  # 输出结果（保留所有换行）：
  # #!/bin/bash
  # echo "Hello World"
  # echo "This is line 2"
  #
  # # 空行也会保留
  # exit 0
```

#### 折叠块（Folded Block）- 使用 `>` 符号

```yaml
apiVersion: v1
kind: ConfigMap
metadata:
  name: description-config
data:
  # > 将多行折叠为单行，仅保留段落分隔（空行）
  description: >
    This is a very long description
    that spans multiple lines but will
    be folded into a single line with
    spaces replacing newlines.

    This is a new paragraph because
    there's an empty line above.

  # 输出结果（单行，段落间有换行）：
  # This is a very long description that spans multiple lines but will be folded into a single line with spaces replacing newlines.
  #
  # This is a new paragraph because there's an empty line above.
```

#### 裁剪指示符（Chomping Indicators）

```yaml
apiVersion: v1
kind: ConfigMap
metadata:
  name: chomping-demo
data:
  # |- 裁剪末尾所有换行符（clip）
  clip: |-
    Line 1
    Line 2
  # 结果："Line 1\nLine 2"（无尾部换行）

  # |+ 保留末尾所有换行符（keep）
  keep: |+
    Line 1
    Line 2


  # 结果："Line 1\nLine 2\n\n\n"（保留 3 个尾部换行）

  # | 保留一个末尾换行符（默认行为，strip）
  strip: |
    Line 1
    Line 2
  # 结果："Line 1\nLine 2\n"（保留 1 个换行）
```

**实际应用场景**：

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: init-script-pod
spec:
  containers:
  - name: app
    image: nginx:1.27
    command: ["/bin/bash", "-c"]
    args:
      # 使用 | 嵌入完整脚本
      - |
        echo "Starting initialization..."
        if [ ! -f /data/initialized ]; then
          echo "First run detected"
          touch /data/initialized
        fi
        echo "Initialization complete"
        nginx -g 'daemon off;'
```

### 2.3 锚点与别名

#### 基本用法（& 定义锚点，* 引用别名）

```yaml
apiVersion: v1
kind: ConfigMap
metadata:
  name: anchor-demo
data:
  # & 定义锚点（类似变量定义）
  common-config: &common-settings
    timeout: 30
    retries: 3
    log-level: info

  # * 引用别名（复制整个块）
  service-a-config: *common-settings
  service-b-config: *common-settings

# 实际展开结果：
# service-a-config:
#   timeout: 30
#   retries: 3
#   log-level: info
# service-b-config:
#   timeout: 30
#   retries: 3
#   log-level: info
```

#### 合并键（Merge Key）- 使用 `<<` 符号

```yaml
# 定义基础配置锚点
defaults: &default-labels
  app: myapp
  environment: production
  managed-by: helm

overrides: &override-labels
  version: "2.0"
  tier: backend

---
apiVersion: v1
kind: Service
metadata:
  name: frontend-service
  labels:
    # << 合并锚点内容，后续字段可覆盖
    <<: *default-labels
    component: frontend      # 新增字段
    version: "1.0"          # 覆盖字段

# 实际展开结果：
# labels:
#   app: myapp
#   environment: production
#   managed-by: helm
#   component: frontend
#   version: "1.0"
```

#### 多锚点合并

```yaml
resource-limits: &cpu-limits
  cpu: "1000m"

memory-limits: &mem-limits
  memory: "512Mi"

---
apiVersion: v1
kind: Pod
metadata:
  name: multi-anchor-pod
spec:
  containers:
  - name: app
    image: nginx:1.27
    resources:
      limits:
        # 合并多个锚点（按顺序合并）
        <<: [*cpu-limits, *mem-limits]
        ephemeral-storage: "2Gi"  # 额外字段

# 实际展开结果：
# limits:
#   cpu: "1000m"
#   memory: "512Mi"
#   ephemeral-storage: "2Gi"
```

**实际应用场景**：

```yaml
# 定义常用容器配置模板
.container-defaults: &container-defaults
  imagePullPolicy: IfNotPresent
  securityContext:
    runAsNonRoot: true
    allowPrivilegeEscalation: false
  resources:
    requests:
      cpu: "100m"
      memory: "128Mi"

---
apiVersion: v1
kind: Pod
metadata:
  name: templated-pod
spec:
  containers:
  - name: nginx
    <<: *container-defaults  # 继承默认配置
    image: nginx:1.27
    resources:
      requests:
        cpu: "200m"          # 覆盖默认值
  - name: sidecar
    <<: *container-defaults
    image: busybox:1.36
```

### 2.4 常见陷阱

#### 陷阱 1：布尔值歧义

```yaml
# ❌ 错误：YAML 1.1 中这些都会被解析为布尔值
enable_feature: yes      # 解析为 true
disable_feature: no      # 解析为 false
is_active: on            # 解析为 true
is_disabled: off         # 解析为 false
version: 1.20            # 数字，非字符串

# ✅ 正确：使用明确的 true/false 或加引号
enable_feature: true
disable_feature: false
is_active: "on"          # 字符串
version: "1.20"          # 字符串

# 实际问题案例
apiVersion: v1
kind: ConfigMap
metadata:
  name: norway-problem
data:
  country: NO            # ❌ 会被解析为 false（挪威国家代码）
  country: "NO"          # ✅ 正确写法
```

#### 陷阱 2：数字与字符串

```yaml
# ❌ 错误：端口号被解析为数字（可能导致问题）
apiVersion: v1
kind: Service
metadata:
  name: web-service
spec:
  ports:
  - port: 8080           # 数字类型
    targetPort: 8080     # 某些字段必须是字符串

# ✅ 正确：明确使用字符串（推荐）
spec:
  ports:
  - port: 8080
    targetPort: "8080"   # 字符串类型更安全

# 版本号问题
image: nginx:1.27        # ❌ 1.27 被解析为浮点数
image: nginx:1.20        # ❌ 1.20 被解析为浮点数
image: "nginx:1.27"      # ✅ 完整字符串
image: nginx:"1.27"      # ✅ 版本号字符串化
```

#### 陷阱 3：空值与 null

```yaml
# YAML 中的 null 值表示方式
apiVersion: v1
kind: ConfigMap
metadata:
  name: null-demo
data:
  # 以下都表示 null 值
  empty1:                # 空值（后面无内容）
  empty2: null           # 显式 null
  empty3: ~              # 波浪号表示 null
  empty4: Null           # 大小写不敏感
  empty5: NULL

# Kubernetes 中的实际影响
spec:
  containers:
  - name: app
    image: nginx:1.27
    command: null        # 会覆盖镜像的 ENTRYPOINT
    args: []             # 空列表（非 null）
```

#### 陷阱 4：特殊字符与引号

```yaml
apiVersion: v1
kind: ConfigMap
metadata:
  name: special-chars-demo
data:
  # ❌ 错误：特殊字符未转义
  message: Hello: World           # 冒号后需空格或引号
  path: C:\Users\Admin            # 反斜杠需转义
  regex: [a-z]+                   # 方括号可能引起歧义

  # ✅ 正确：使用引号包裹
  message: "Hello: World"
  path: "C:\\Users\\Admin"        # 双引号内需转义
  path_alt: 'C:\Users\Admin'      # 单引号内无需转义
  regex: "[a-z]+"

  # 特殊前缀字符
  asterisk: "*test"               # ❌ 误认为别名
  asterisk_safe: "*test"          # ✅ 加引号

  # YAML 特殊字符：[] {} > | * & ! % @ `
  safe_text: "Use quotes for: [] {} > | * & ! % @ `"
```

#### 陷阱 5：缩进不一致

```yaml
# ❌ 错误：缩进混乱
apiVersion: v1
kind: Pod
metadata:
  name: bad-indent
spec:
  containers:
  - name: app
    image: nginx:1.27
      ports:              # ❌ 缩进错误（应与 image 对齐）
      - containerPort: 80

# ✅ 正确：保持一致的缩进
spec:
  containers:
  - name: app
    image: nginx:1.27
    ports:                # ✅ 与 image 同级
    - containerPort: 80
```

#### 陷阱 6：列表项格式

```yaml
# ❌ 错误：列表格式不当
env:
- name: VAR1             # ❌ 缺少空格
  value: "val1"
-name: VAR2              # ❌ 缺少空格
  value: "val2"

# ✅ 正确：- 后必须有空格
env:
  - name: VAR1           # ✅ "- " 后接内容
    value: "val1"
  - name: VAR2
    value: "val2"
```

---

## 3. Kubernetes 资源四大顶层字段

所有 Kubernetes 资源清单都遵循统一的结构，由四大顶层字段组成：

```yaml
apiVersion: <API组>/<版本>  # 1. API 版本标识
kind: <资源类型>             # 2. 资源类型
metadata:                   # 3. 元数据（标识资源）
  name: <资源名称>
  namespace: <命名空间>
  labels: {}
  annotations: {}
spec:                       # 4. 期望状态（用户定义）
  # ...资源特定配置
status:                     # 系统管理的实际状态（只读）
  # ...由控制器更新
```

### 3.1 apiVersion

#### 格式规范

```yaml
# 格式：<API组>/<版本> 或 <版本>（核心 API 组）

# 核心 API 组（无组名前缀）
apiVersion: v1              # Pod, Service, ConfigMap, Secret, Namespace 等

# 命名 API 组
apiVersion: apps/v1         # Deployment, StatefulSet, DaemonSet, ReplicaSet
apiVersion: batch/v1        # Job, CronJob
apiVersion: networking.k8s.io/v1  # Ingress, NetworkPolicy, IngressClass
```

#### 完整 API 组资源映射表

| API 组 (apiVersion) | 主要资源 (Kind) | 用途说明 | GA 版本 |
|-------------------|----------------|---------|---------|
| **v1** (核心组) | Pod, Service, ConfigMap, Secret, PersistentVolume, PersistentVolumeClaim, Namespace, Node, ServiceAccount, Endpoints, Event, LimitRange, ResourceQuota | 基础资源类型 | v1.0+ |
| **apps/v1** | Deployment, StatefulSet, DaemonSet, ReplicaSet | 工作负载管理 | v1.9+ |
| **batch/v1** | Job, CronJob | 批处理任务 | v1.21+ (CronJob) |
| **networking.k8s.io/v1** | Ingress, NetworkPolicy, IngressClass | 网络策略与路由 | v1.19+ (Ingress) |
| **policy/v1** | PodDisruptionBudget | 中断预算管理 | v1.21+ |
| **rbac.authorization.k8s.io/v1** | Role, ClusterRole, RoleBinding, ClusterRoleBinding | 权限控制 | v1.8+ |
| **storage.k8s.io/v1** | StorageClass, VolumeAttachment, CSIDriver, CSINode | 存储配置 | v1.6+ |
| **autoscaling/v2** | HorizontalPodAutoscaler | 弹性伸缩 | v1.23+ |
| **certificates.k8s.io/v1** | CertificateSigningRequest | 证书管理 | v1.19+ |
| **coordination.k8s.io/v1** | Lease | 分布式锁 | v1.14+ |
| **discovery.k8s.io/v1** | EndpointSlice | 服务发现优化 | v1.21+ |
| **events.k8s.io/v1** | Event | 新版事件 API | v1.19+ |
| **node.k8s.io/v1** | RuntimeClass | 容器运行时类 | v1.20+ |
| **scheduling.k8s.io/v1** | PriorityClass | 调度优先级 | v1.14+ |
| **admissionregistration.k8s.io/v1** | ValidatingWebhookConfiguration, MutatingWebhookConfiguration | 准入控制器 | v1.16+ |
| **apiextensions.k8s.io/v1** | CustomResourceDefinition | 自定义资源 | v1.16+ |
| **flowcontrol.apiserver.k8s.io/v1** | FlowSchema, PriorityLevelConfiguration | API 流控 | v1.29+ |
| **gateway.networking.k8s.io/v1** | Gateway, GatewayClass, HTTPRoute | Gateway API | v1.29+ (部分) |

#### API 版本生命周期标识

```yaml
# Alpha 版本（实验性功能）
apiVersion: foo.example.com/v1alpha1   # 默认禁用，可能随时变更

# Beta 版本（预发布版本）
apiVersion: autoscaling/v2beta2        # 默认启用，API 基本稳定

# Stable 版本（正式版本）
apiVersion: apps/v1                    # 生产可用，向后兼容
```

**版本选择原则**：
- ✅ **生产环境必须使用 stable 版本**（无 alpha/beta 标识）
- ⚠️  测试环境可尝试 beta 版本
- ❌ 避免在生产环境使用 alpha 版本

### 3.2 kind

`kind` 字段指定资源类型，必须与 `apiVersion` 匹配。Kubernetes 内置 60+ 种资源类型。

#### 按 API 组分类的完整 Kind 列表

**核心资源 (v1)**

| Kind | 简写 | 作用域 | 说明 |
|------|-----|-------|------|
| Pod | po | Namespaced | 最小调度单元 |
| Service | svc | Namespaced | 服务发现与负载均衡 |
| ConfigMap | cm | Namespaced | 配置数据存储 |
| Secret | secret | Namespaced | 敏感数据存储 |
| Namespace | ns | Cluster | 逻辑隔离边界 |
| Node | no | Cluster | 集群节点 |
| PersistentVolume | pv | Cluster | 持久化存储卷 |
| PersistentVolumeClaim | pvc | Namespaced | 存储卷申领 |
| ServiceAccount | sa | Namespaced | 服务账号 |
| Endpoints | ep | Namespaced | 服务端点 |
| Event | ev | Namespaced | 集群事件 |
| LimitRange | limits | Namespaced | 资源限制范围 |
| ResourceQuota | quota | Namespaced | 资源配额 |

**工作负载资源 (apps/v1)**

| Kind | 简写 | 说明 |
|------|-----|------|
| Deployment | deploy | 无状态应用部署 |
| StatefulSet | sts | 有状态应用部署 |
| DaemonSet | ds | 每节点运行一个 Pod |
| ReplicaSet | rs | Pod 副本集（通常由 Deployment 管理） |

**批处理资源 (batch/v1)**

| Kind | 简写 | 说明 |
|------|-----|------|
| Job | job | 一次性任务 |
| CronJob | cj | 定时任务 |

**网络资源 (networking.k8s.io/v1)**

| Kind | 简写 | 说明 |
|------|-----|------|
| Ingress | ing | HTTP(S) 路由 |
| NetworkPolicy | netpol | 网络隔离策略 |
| IngressClass | - | Ingress 控制器类 |

**存储资源 (storage.k8s.io/v1)**

| Kind | 简写 | 说明 |
|------|-----|------|
| StorageClass | sc | 存储类 |
| VolumeAttachment | - | 卷挂载状态 |
| CSIDriver | - | CSI 驱动注册 |
| CSINode | - | CSI 节点信息 |

**权限资源 (rbac.authorization.k8s.io/v1)**

| Kind | 简写 | 作用域 |
|------|-----|-------|
| Role | - | Namespaced |
| ClusterRole | - | Cluster |
| RoleBinding | - | Namespaced |
| ClusterRoleBinding | - | Cluster |

#### Kind 使用示例

```yaml
# 示例 1：工作负载资源
apiVersion: apps/v1
kind: Deployment                # 无状态应用
metadata:
  name: nginx-deployment

---
# 示例 2：配置资源
apiVersion: v1
kind: ConfigMap                 # 配置存储
metadata:
  name: app-config

---
# 示例 3：网络资源
apiVersion: networking.k8s.io/v1
kind: Ingress                   # HTTP 路由
metadata:
  name: web-ingress

---
# 示例 4：权限资源
apiVersion: rbac.authorization.k8s.io/v1
kind: ClusterRole               # 集群级角色
metadata:
  name: pod-reader
```

### 3.3 metadata

`metadata` 字段包含用于标识和管理资源的元数据信息。

#### 完整字段结构

```yaml
metadata:
  # === 必填字段 ===
  name: my-resource               # 资源名称（必填）
  
  # === 常用可选字段 ===
  namespace: default              # 命名空间（Namespaced 资源）
  labels:                         # 标签（用于选择和组织）
    app: myapp
    version: "1.0"
  annotations:                    # 注解（任意元数据）
    description: "My application"
    
  # === 系统管理字段（只读，由 API Server 自动生成）===
  uid: "abc123-def456"            # 全局唯一标识符
  resourceVersion: "12345"        # 资源版本号（乐观锁）
  generation: 2                   # spec 变更代数
  creationTimestamp: "2026-02-10T10:00:00Z"
  deletionTimestamp: "2026-02-10T11:00:00Z"  # 删除时间戳
  deletionGracePeriodSeconds: 30
  
  # === 高级字段 ===
  ownerReferences:                # 所有者关系（垃圾回收）
  - apiVersion: apps/v1
    kind: ReplicaSet
    name: nginx-rs-abc
    uid: "xyz789"
    controller: true              # 标记为控制器
    blockOwnerDeletion: true      # 阻止删除所有者
    
  finalizers:                     # 终结器（删除前置钩子）
  - kubernetes.io/pvc-protection
  
  managedFields:                  # Server-side Apply 字段管理
  - manager: kubectl
    operation: Apply
    apiVersion: v1
    time: "2026-02-10T10:00:00Z"
    fieldsType: FieldsV1
    fieldsV1: {}
```

#### 字段详解

**1. name（必填）**

```yaml
metadata:
  name: nginx-deployment          # ✅ 符合 DNS-1123 子域名规范
  # 规则：
  # - 最多 253 字符
  # - 仅包含小写字母、数字、'-'、'.'
  # - 必须以字母或数字开头和结尾
  
  # ❌ 错误示例
  name: Nginx_Deployment          # 大写字母和下划线不允许
  name: -nginx                    # 不能以 '-' 开头
  name: nginx-                    # 不能以 '-' 结尾
```

**2. namespace（Namespaced 资源必须指定）**

```yaml
metadata:
  name: my-pod
  namespace: production           # 指定命名空间
  # 未指定时默认为 "default"
  
# 集群级资源（如 Node、PersistentVolume）不能设置 namespace
```

**3. labels（推荐使用）**

```yaml
metadata:
  labels:
    # 标准推荐标签（app.kubernetes.io/*）
    app.kubernetes.io/name: nginx
    app.kubernetes.io/instance: nginx-prod
    app.kubernetes.io/version: "1.27.0"
    app.kubernetes.io/component: frontend
    app.kubernetes.io/part-of: ecommerce
    app.kubernetes.io/managed-by: helm
    
    # 自定义标签
    environment: production
    team: platform
    cost-center: "12345"
    
    # 标签规则：
    # - 键：可选前缀 + 名称（前缀示例：example.com/key）
    # - 前缀最多 253 字符，名称最多 63 字符
    # - 值：最多 63 字符，允许空字符串
```

**4. annotations（任意元数据）**

```yaml
metadata:
  annotations:
    # 描述性注解
    description: "Production NGINX deployment"
    contact: "platform-team@example.com"
    
    # 构建信息
    build.version: "1.2.3"
    git.commit: "abc123def456"
    ci.pipeline: "https://ci.example.com/builds/456"
    
    # 控制器特定配置
    prometheus.io/scrape: "true"
    prometheus.io/port: "9090"
    nginx.ingress.kubernetes.io/rewrite-target: /
    cert-manager.io/cluster-issuer: letsencrypt-prod
    
    # kubectl 内部注解
    kubectl.kubernetes.io/last-applied-configuration: |
      {...}  # kubectl apply 使用的三方合并配置
```

**5. ownerReferences（垃圾回收机制）**

```yaml
# 场景：ReplicaSet 创建的 Pod 自动设置 ownerReferences
apiVersion: v1
kind: Pod
metadata:
  name: nginx-pod-abc123
  ownerReferences:
  - apiVersion: apps/v1
    kind: ReplicaSet
    name: nginx-rs-xyz
    uid: "d290f1ee-6c54-4b01-90e6-d701748f0851"
    controller: true              # 标记为控制器所有者
    blockOwnerDeletion: true      # 删除 Pod 前必须先删除 ReplicaSet
    
# 效果：删除 ReplicaSet 时，所有关联的 Pod 会被自动级联删除
```

**6. finalizers（删除前置钩子）**

```yaml
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: data-pvc
  finalizers:
  - kubernetes.io/pvc-protection  # PVC 保护终结器
  
# 工作流程：
# 1. kubectl delete pvc data-pvc
# 2. API Server 设置 deletionTimestamp，但不删除对象
# 3. 控制器检测到 deletionTimestamp，执行清理逻辑
# 4. 清理完成后，控制器移除 finalizer
# 5. finalizers 列表为空时，API Server 真正删除对象
```

#### 完整示例

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  # === 基本信息 ===
  name: nginx-deployment
  namespace: production
  
  # === 标签（用于选择器和组织）===
  labels:
    app.kubernetes.io/name: nginx
    app.kubernetes.io/instance: nginx-prod
    app.kubernetes.io/version: "1.27.0"
    app.kubernetes.io/component: frontend
    app.kubernetes.io/part-of: ecommerce
    app.kubernetes.io/managed-by: helm
    environment: production
    team: platform-team
    
  # === 注解（任意元数据）===
  annotations:
    description: "Production NGINX web server"
    contact: "platform-team@example.com"
    documentation: "https://wiki.example.com/nginx"
    build.version: "1.2.3"
    git.commit: "abc123def456"
    prometheus.io/scrape: "true"
    prometheus.io/port: "9113"
    
  # === 系统管理字段（只读，示例值）===
  # uid: "d290f1ee-6c54-4b01-90e6-d701748f0851"
  # resourceVersion: "123456"
  # generation: 5
  # creationTimestamp: "2026-02-10T10:00:00Z"

spec:
  replicas: 3
  selector:
    matchLabels:
      app.kubernetes.io/name: nginx
      app.kubernetes.io/instance: nginx-prod
  template:
    metadata:
      labels:
        app.kubernetes.io/name: nginx
        app.kubernetes.io/instance: nginx-prod
        app.kubernetes.io/version: "1.27.0"
    spec:
      containers:
      - name: nginx
        image: nginx:1.27.0
        ports:
        - containerPort: 80
```

### 3.4 spec 与 status

Kubernetes 采用**声明式 API** 设计，资源配置分为两部分：

- **spec**：期望状态（Desired State）- 用户定义
- **status**：实际状态（Observed State）- 系统管理

#### 工作原理

```yaml
# 用户创建资源时只需定义 spec
apiVersion: apps/v1
kind: Deployment
metadata:
  name: nginx
spec:                           # 期望状态（用户填写）
  replicas: 3                   # 期望运行 3 个副本
  selector:
    matchLabels:
      app: nginx
  template:
    spec:
      containers:
      - name: nginx
        image: nginx:1.27

# 控制器持续更新 status
status:                         # 实际状态（系统自动更新）
  replicas: 3                   # 当前实际运行 3 个副本
  availableReplicas: 3          # 可用副本数
  readyReplicas: 3              # 就绪副本数
  conditions:
  - type: Available
    status: "True"
    lastUpdateTime: "2026-02-10T10:05:00Z"
  observedGeneration: 1         # 控制器观测到的 generation
```

#### 控制循环（Control Loop）

```
┌─────────────────────────────────────────────┐
│  1. 用户通过 kubectl apply 提交 spec        │
│     期望状态：replicas: 3                   │
└──────────────────┬──────────────────────────┘
                   ▼
┌─────────────────────────────────────────────┐
│  2. API Server 存储到 etcd                  │
└──────────────────┬──────────────────────────┘
                   ▼
┌─────────────────────────────────────────────┐
│  3. Controller Manager 监听资源变化         │
│     比较 spec vs status                     │
└──────────────────┬──────────────────────────┘
                   ▼
        ┌──────────┴──────────┐
        │  spec ≠ status ?    │
        └──────────┬──────────┘
                   │ YES
                   ▼
┌─────────────────────────────────────────────┐
│  4. 执行协调动作（Reconcile）               │
│     - 当前 1 副本 → 创建 2 个新 Pod         │
│     - 更新 status.replicas = 3              │
└──────────────────┬──────────────────────────┘
                   ▼
┌─────────────────────────────────────────────┐
│  5. 持续监控直到 spec = status              │
└─────────────────────────────────────────────┘
```

#### status 字段示例

```yaml
# Pod 的 status
apiVersion: v1
kind: Pod
status:
  phase: Running                  # Pod 生命周期阶段
  conditions:                     # 条件列表
  - type: Initialized             # Pod 初始化完成
    status: "True"
    lastTransitionTime: "2026-02-10T10:00:00Z"
  - type: Ready                   # Pod 就绪
    status: "True"
    lastTransitionTime: "2026-02-10T10:00:30Z"
  - type: ContainersReady
    status: "True"
  - type: PodScheduled            # Pod 已调度
    status: "True"
  containerStatuses:              # 容器状态
  - name: nginx
    ready: true
    restartCount: 0
    state:
      running:
        startedAt: "2026-02-10T10:00:20Z"
  podIP: "10.244.1.5"             # Pod IP 地址
  hostIP: "192.168.1.10"          # 节点 IP
  startTime: "2026-02-10T10:00:00Z"

---
# Service 的 status
apiVersion: v1
kind: Service
status:
  loadBalancer:                   # LoadBalancer 状态
    ingress:
    - ip: "203.0.113.42"          # 分配的公网 IP

---
# Node 的 status
apiVersion: v1
kind: Node
status:
  conditions:                     # 节点条件
  - type: Ready
    status: "True"
    reason: KubeletReady
    message: "kubelet is posting ready status"
  - type: MemoryPressure
    status: "False"
  - type: DiskPressure
    status: "False"
  - type: PIDPressure
    status: "False"
  addresses:                      # 节点地址
  - type: InternalIP
    address: "192.168.1.10"
  - type: Hostname
    address: "node-1"
  capacity:                       # 节点总容量
    cpu: "4"
    memory: "8Gi"
    pods: "110"
  allocatable:                    # 可分配容量
    cpu: "3800m"
    memory: "7.5Gi"
    pods: "110"
```

#### 声明式 vs 命令式

```bash
# 命令式（Imperative）- 告诉系统"怎么做"
kubectl scale deployment nginx --replicas=5
kubectl expose deployment nginx --port=80

# 声明式（Declarative）- 告诉系统"期望什么结果"
kubectl apply -f deployment.yaml  # 系统自动计算差异并执行
```

**声明式优势**：
- ✅ 配置即代码（Configuration as Code）
- ✅ 幂等性（重复执行结果一致）
- ✅ 易于版本控制和回滚
- ✅ 支持 GitOps 工作流

---

## 4. 资源命名规范

Kubernetes 使用多种 DNS 命名标准来规范资源名称，确保兼容性和一致性。

### 4.1 DNS-1123 子域名

**应用场景**：大部分资源的 `metadata.name`（如 Deployment、Service、ConfigMap 等）

**规则**：
- 最多 253 个字符
- 仅包含小写字母 (`a-z`)、数字 (`0-9`)、连字符 (`-`)、点号 (`.`)
- 必须以字母或数字开头和结尾
- 点号前后不能是连字符

```yaml
# ✅ 合法示例
name: nginx-deployment          # 常规命名
name: my-app-v1                 # 包含版本号
name: web.example.com           # 包含点号（类似 FQDN）
name: app-123                   # 数字结尾
name: 123-app                   # 数字开头

# ❌ 非法示例
name: Nginx-Deployment          # ❌ 大写字母
name: my_app                    # ❌ 下划线
name: -nginx                    # ❌ 连字符开头
name: nginx-                    # ❌ 连字符结尾
name: app.-test                 # ❌ 点号后接连字符
name: "a" * 254                 # ❌ 超过 253 字符
```

### 4.2 DNS-1123 标签

**应用场景**：标签键值（`metadata.labels`）、命名空间名称

**规则**：
- 最多 63 个字符
- 仅包含小写字母、数字、连字符
- 必须以字母或数字开头和结尾

```yaml
# ✅ 合法示例
labels:
  app: nginx                    # 简单标签
  environment: production
  version: "1-0-0"              # 连字符分隔
  tier: frontend
  release: stable

# ❌ 非法示例
labels:
  app: Nginx                    # ❌ 大写字母
  env: prod_test                # ❌ 下划线
  ver: 1.0.0                    # ❌ 点号（DNS-1123 标签不允许）
  -app: test                    # ❌ 连字符开头
```

### 4.3 DNS-1035 标签

**应用场景**：Service 名称（历史兼容性要求）

**规则**（比 DNS-1123 更严格）：
- 最多 63 个字符
- 仅包含小写字母、数字、连字符
- 必须以**字母**开头（不能是数字）
- 必须以字母或数字结尾

```yaml
# ✅ 合法示例（Service 名称）
apiVersion: v1
kind: Service
metadata:
  name: web-service             # 字母开头
  name: nginx-svc
  name: api-gateway

# ❌ 非法示例
metadata:
  name: 123-service             # ❌ 数字开头（不符合 DNS-1035）
  # 虽然在 Kubernetes 1.24+ 中 Service 名称已放宽至 DNS-1123，
  # 但为兼容性建议仍以字母开头
```

### 4.4 RFC 1123 vs RFC 952

| 特性 | RFC 952 (DNS-1035) | RFC 1123 (DNS-1123) |
|------|-------------------|---------------------|
| 首字符 | 必须是字母 | 字母或数字 |
| 字符集 | `a-z`、`0-9`、`-` | `a-z`、`0-9`、`-`、`.` (子域名) |
| 长度限制 | 63 字符 | 63 字符（标签）/ 253 字符（子域名） |
| 示例 | `web-service` | `my-app.example.com` |
| K8s 应用 | Service 名称（历史） | 大部分资源名称 |

**实际规则总结**：

| 资源类型 | 命名规范 | 首字符要求 | 最大长度 |
|---------|---------|----------|---------|
| Pod, Deployment, ConfigMap 等 | DNS-1123 子域名 | 字母或数字 | 253 |
| Service | DNS-1035 标签（推荐） | 字母 | 63 |
| Namespace | DNS-1123 标签 | 字母或数字 | 63 |
| Label 键名（不含前缀） | DNS-1123 标签 | 字母或数字 | 63 |
| Label 值 | DNS-1123 标签 | 字母或数字 | 63 |

---

## 5. 标签与注解最佳实践

### 5.1 推荐标签 (app.kubernetes.io/*)

Kubernetes 社区推荐使用标准化标签前缀 `app.kubernetes.io/`，确保工具链兼容性。

#### 推荐标签清单

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: my-application
  labels:
    # === 核心推荐标签（app.kubernetes.io/*）===
    
    # 应用名称（必填）
    app.kubernetes.io/name: nginx
    # 说明：应用的通用名称（如 nginx、mysql、wordpress）
    
    # 实例名称（推荐）
    app.kubernetes.io/instance: nginx-prod
    # 说明：应用实例的唯一标识（支持多实例部署）
    
    # 应用版本（推荐）
    app.kubernetes.io/version: "1.27.0"
    # 说明：应用的当前版本号
    
    # 组件类型（推荐）
    app.kubernetes.io/component: frontend
    # 说明：架构中的组件角色（如 database、cache、frontend、backend）
    
    # 所属应用（可选）
    app.kubernetes.io/part-of: ecommerce-platform
    # 说明：上层应用系统名称（微服务归属）
    
    # 管理工具（推荐）
    app.kubernetes.io/managed-by: helm
    # 说明：用于创建资源的工具（如 helm、kustomize、kubectl）

spec:
  selector:
    matchLabels:
      # Selector 必须使用稳定的标签（避免使用 version）
      app.kubernetes.io/name: nginx
      app.kubernetes.io/instance: nginx-prod
  template:
    metadata:
      labels:
        # Pod 模板需要包含 Selector 的所有标签
        app.kubernetes.io/name: nginx
        app.kubernetes.io/instance: nginx-prod
        app.kubernetes.io/version: "1.27.0"
        app.kubernetes.io/component: frontend
```

#### 完整示例：微服务架构标签

```yaml
# 电商平台 - 订单服务
apiVersion: apps/v1
kind: Deployment
metadata:
  name: order-service-v2
  namespace: production
  labels:
    app.kubernetes.io/name: order-service         # 服务名称
    app.kubernetes.io/instance: order-service-prod  # 生产实例
    app.kubernetes.io/version: "2.3.1"            # 服务版本
    app.kubernetes.io/component: api-server       # 组件类型
    app.kubernetes.io/part-of: ecommerce-platform # 所属平台
    app.kubernetes.io/managed-by: argocd          # 管理工具

---
# 电商平台 - 数据库
apiVersion: apps/v1
kind: StatefulSet
metadata:
  name: postgresql-primary
  labels:
    app.kubernetes.io/name: postgresql
    app.kubernetes.io/instance: ecommerce-db
    app.kubernetes.io/version: "15.2"
    app.kubernetes.io/component: database         # 数据库组件
    app.kubernetes.io/part-of: ecommerce-platform
    app.kubernetes.io/managed-by: helm
```

### 5.2 运维标签

除了推荐标签，生产环境还需补充运维管理标签。

```yaml
metadata:
  labels:
    # === 推荐标签 ===
    app.kubernetes.io/name: payment-service
    app.kubernetes.io/instance: payment-prod
    app.kubernetes.io/version: "3.1.0"
    
    # === 运维标签 ===
    
    # 环境标识
    environment: production            # 环境：production/staging/development
    
    # 团队归属
    team: payment-team                 # 负责团队
    owner: payment-team@example.com
    
    # 成本管理
    cost-center: "CC-12345"            # 成本中心
    business-unit: finance             # 业务单元
    
    # 服务等级
    tier: critical                     # 等级：critical/high/medium/low
    sla: "99.99"                       # SLA 承诺
    
    # 合规标签
    compliance: pci-dss                # 合规要求
    data-classification: confidential  # 数据分类
    
    # 运维标签
    monitoring: enabled                # 监控开关
    backup: daily                      # 备份策略
    log-retention: "30d"               # 日志保留
```

#### 标签选择器示例

```bash
# 查询特定团队的所有资源
kubectl get pods -l team=payment-team

# 查询生产环境的关键服务
kubectl get pods -l environment=production,tier=critical

# 查询特定应用的所有组件
kubectl get all -l app.kubernetes.io/part-of=ecommerce-platform

# 多标签联合查询
kubectl get pods -l 'app.kubernetes.io/name=nginx,environment in (production,staging)'
```

### 5.3 注解用途

注解（Annotations）用于存储任意非标识性元数据，不会被选择器使用。

#### 常见注解分类

```yaml
metadata:
  annotations:
    # === 1. 描述性信息 ===
    description: "Payment processing microservice"
    documentation: "https://wiki.example.com/payment-service"
    contact: "payment-team@example.com"
    oncall: "https://pagerduty.com/payment-team"
    
    # === 2. 构建与版本信息 ===
    build.version: "3.1.0-20260210.1234"
    build.timestamp: "2026-02-10T10:00:00Z"
    git.commit: "abc123def456789"
    git.branch: "release/3.1"
    git.repo: "https://github.com/example/payment-service"
    ci.pipeline: "https://jenkins.example.com/job/payment-service/123"
    image.digest: "sha256:abc123..."
    
    # === 3. 变更记录 ===
    change.ticket: "JIRA-12345"
    change.approver: "john.doe@example.com"
    change.reason: "Fix payment timeout issue"
    deployment.timestamp: "2026-02-10T10:30:00Z"
    deployed.by: "argocd"
    
    # === 4. Prometheus 监控注解 ===
    prometheus.io/scrape: "true"            # 启用抓取
    prometheus.io/port: "9090"              # 指标端口
    prometheus.io/path: "/metrics"          # 指标路径
    prometheus.io/scheme: "https"           # 协议
    
    # === 5. Ingress 控制器注解（NGINX Ingress）===
    nginx.ingress.kubernetes.io/rewrite-target: /
    nginx.ingress.kubernetes.io/ssl-redirect: "true"
    nginx.ingress.kubernetes.io/rate-limit: "100"
    nginx.ingress.kubernetes.io/proxy-body-size: "50m"
    nginx.ingress.kubernetes.io/cors-allow-origin: "*"
    
    # === 6. 证书管理注解（cert-manager）===
    cert-manager.io/cluster-issuer: letsencrypt-prod
    cert-manager.io/acme-challenge-type: http01
    
    # === 7. Service Mesh 注解（Istio）===
    sidecar.istio.io/inject: "true"         # 注入 Envoy sidecar
    traffic.sidecar.istio.io/excludeOutboundPorts: "3306"
    proxy.istio.io/config: |
      terminationDrainDuration: 30s
    
    # === 8. 存储类注解 ===
    volume.beta.kubernetes.io/storage-class: "ssd"
    volume.kubernetes.io/selected-node: "node-1"
    
    # === 9. kubectl 内部注解（自动生成）===
    kubectl.kubernetes.io/last-applied-configuration: |
      {"apiVersion":"apps/v1","kind":"Deployment",...}
```

#### 注解 vs 标签决策树

```
是否需要通过 kubectl 选择器查询该字段？
├─ YES → 使用 Label
│   ├─ 例如：app=nginx, env=prod
│   └─ 用途：kubectl get pods -l app=nginx
│
└─ NO → 使用 Annotation
    ├─ 例如：git.commit=abc123, prometheus.io/port=9090
    └─ 用途：存储元数据、工具配置、变更记录
```

**规则总结**：
- ✅ **标签**：用于**标识和选择**资源（键值简短、规范化）
- ✅ **注解**：用于**描述和配置**资源（值可包含复杂数据）

---

## 6. 字段验证规则

### 6.1 必填与可选字段

Kubernetes 资源字段分为三类：

| 类型 | 说明 | 示例 |
|------|------|------|
| **必填字段** | 缺失会导致 API 拒绝 | `apiVersion`, `kind`, `metadata.name` |
| **可选字段** | 可省略，使用默认值 | `metadata.namespace` (默认 `default`) |
| **条件必填** | 特定条件下必填 | `spec.selector` (Deployment 必填) |

```yaml
# 最小合法资源（必填字段）
apiVersion: v1              # 必填
kind: Pod                   # 必填
metadata:
  name: minimal-pod         # 必填
spec:                       # 必填（大部分资源）
  containers:               # Pod 必填
  - name: app               # Container 必填
    image: nginx:1.27       # Container 必填

# 可选字段示例
metadata:
  namespace: default        # 可选（默认 "default"）
  labels: {}                # 可选（默认空）
  annotations: {}           # 可选
spec:
  restartPolicy: Always     # 可选（默认 "Always"）
  terminationGracePeriodSeconds: 30  # 可选（默认 30）
```

### 6.2 不可变字段

某些字段在创建后**不可修改**（Immutable），尝试修改会导致 API 拒绝。

#### 按资源类型分类的不可变字段

**Pod**
```yaml
# 不可变字段
spec:
  nodeName: "node-1"          # ❌ 调度后不可更改
  serviceAccountName: default # ❌ 创建后不可更改
  
# 可变字段
metadata:
  labels: {}                  # ✅ 可随时修改
  annotations: {}             # ✅ 可随时修改
spec:
  containers[*].image: ""     # ✅ 可修改（触发滚动更新）
```

**Service**
```yaml
spec:
  clusterIP: "10.96.0.10"     # ❌ 分配后不可更改
  type: ClusterIP             # ❌ 某些类型转换受限（LoadBalancer → ClusterIP 禁止）
  
  ports:                      # ✅ 可修改
  - port: 80
    targetPort: 8080
```

**Deployment**
```yaml
spec:
  selector:                   # ❌ 创建后不可更改
    matchLabels:
      app: nginx
  
  replicas: 3                 # ✅ 可修改
  template:                   # ✅ 可修改（触发滚动更新）
    spec:
      containers: []
```

**PersistentVolumeClaim**
```yaml
spec:
  storageClassName: "ssd"     # ❌ 创建后不可更改
  volumeName: "pv-123"        # ❌ 绑定后不可更改
  
  resources:                  # ⚠️  仅支持扩容（v1.11+），不可缩容
    requests:
      storage: "10Gi"
```

#### 完整不可变字段列表

| 资源 | 不可变字段 | 备注 |
|------|----------|------|
| **Pod** | `nodeName`, `serviceAccountName`, `volumes[*].name` | 需删除重建 |
| **Service** | `clusterIP`, `clusterIPs`, `ipFamilies`, `ipFamilyPolicy` | 除 `type` 外基本不可变 |
| **Deployment** | `spec.selector` | 修改需删除重建 |
| **StatefulSet** | `spec.selector`, `spec.serviceName`, `spec.volumeClaimTemplates[*].metadata.name` | 关键字段不可变 |
| **Job** | `spec.completions`, `spec.parallelism`, `spec.selector` | 执行中的任务不可修改 |
| **PVC** | `spec.storageClassName`, `spec.volumeName` | 绑定后锁定 |
| **Namespace** | `metadata.name` | 名称不可变 |

#### 尝试修改不可变字段的错误示例

```bash
# 创建 Deployment
kubectl apply -f deployment.yaml

# 尝试修改 selector（会失败）
# deployment.yaml:
# spec:
#   selector:
#     matchLabels:
#       app: nginx-new  # ❌ 修改 selector

$ kubectl apply -f deployment.yaml
The Deployment "nginx" is invalid: spec.selector: Invalid value: 
v1.LabelSelector{...}: field is immutable

# 正确方法：删除并重建
kubectl delete deployment nginx
kubectl apply -f deployment.yaml
```

### 6.3 Server-side Field Validation (v1.25+)

Kubernetes 1.25+ 引入**服务端字段验证**，提供三种严格级别。

#### 验证级别

| 级别 | 行为 | 使用场景 |
|------|------|---------|
| **Strict** | 拒绝无效字段 | 生产环境（强制合规） |
| **Warn** | 警告但接受 | 开发环境（提示错误） |
| **Ignore** | 忽略验证 | 兼容旧配置 |

#### kubectl 使用方式

```bash
# 1. 严格验证（推荐生产环境）
kubectl apply -f deployment.yaml --validate=strict
# 效果：拒绝未知字段、重复字段、类型错误

# 2. 警告模式（默认）
kubectl apply -f deployment.yaml --validate=warn
# 效果：打印警告但继续执行

# 3. 禁用验证
kubectl apply -f deployment.yaml --validate=false
# 效果：跳过所有验证
```

#### 实际案例

```yaml
# 示例：包含错误字段的 Deployment
apiVersion: apps/v1
kind: Deployment
metadata:
  name: nginx
spec:
  replicas: 3
  selector:
    matchLabels:
      app: nginx
  template:
    metadata:
      labels:
        app: nginx
    spec:
      containers:
      - name: nginx
        image: nginx:1.27
        ports:
        - containerPort: 80
        unknownField: "test"  # ❌ 未知字段
```

```bash
# 应用时触发严格验证
$ kubectl apply -f deployment.yaml --validate=strict

Error from server (BadRequest): error when creating "deployment.yaml": 
Deployment in version "v1" cannot be handled as a Deployment: 
strict decoding error: unknown field "spec.template.spec.containers[0].unknownField"

# 警告模式
$ kubectl apply -f deployment.yaml --validate=warn

Warning: unknown field "spec.template.spec.containers[0].unknownField"
deployment.apps/nginx created  # 仍然创建成功
```

#### 最佳实践

```bash
# CI/CD 管道中启用严格验证
kubectl apply -f manifests/ \
  --validate=strict \
  --dry-run=server    # 先预检查

# 开发环境使用警告模式
kubectl apply -f dev-deployment.yaml --validate=warn

# 生产环境强制严格模式
kubectl apply -f prod-deployment.yaml --validate=strict
```

---

## 7. kubectl 操作与内部原理

### 7.1 apply vs create vs replace

Kubernetes 提供三种主要的资源创建/更新命令，内部机制完全不同。

#### 命令对比表

| 特性 | `kubectl create` | `kubectl replace` | `kubectl apply` |
|------|-----------------|------------------|-----------------|
| **操作模式** | 命令式（Imperative） | 命令式 | 声明式（Declarative） |
| **幂等性** | ❌ 否（重复执行报错） | ⚠️  部分（需资源已存在） | ✅ 是（可重复执行） |
| **资源不存在时** | 创建资源 | ❌ 报错 | 创建资源 |
| **资源已存在时** | ❌ 报错 | 替换资源（删除所有未指定字段） | 三方合并更新 |
| **字段合并策略** | - | 完全替换 | 智能合并 |
| **推荐场景** | 一次性创建 | 完整替换配置 | **生产环境标准操作** |

#### 1. kubectl create（命令式创建）

```bash
# 创建资源（仅在不存在时成功）
kubectl create -f deployment.yaml

# 重复执行会失败
$ kubectl create -f deployment.yaml
Error from server (AlreadyExists): error when creating "deployment.yaml": 
deployments.apps "nginx" already exists

# 适用场景
kubectl create namespace production       # 创建命名空间
kubectl create secret generic db-secret \ # 创建 Secret
  --from-literal=password=abc123
```

#### 2. kubectl replace（命令式替换）

```bash
# 完全替换现有资源（资源必须已存在）
kubectl replace -f deployment.yaml

# 如果资源不存在会失败
$ kubectl replace -f new-deployment.yaml
Error from server (NotFound): error when replacing "new-deployment.yaml": 
deployments.apps "nginx" not found

# 强制替换（等同于 delete + create）
kubectl replace -f deployment.yaml --force

# ⚠️  危险：未指定的字段会被删除
# 原配置：
# spec:
#   replicas: 3
#   strategy: { type: RollingUpdate }
#
# 新配置（缺少 strategy）：
# spec:
#   replicas: 5
#
# 执行 replace 后，strategy 字段会被删除！
```

#### 3. kubectl apply（声明式更新）⭐ 推荐

```bash
# 创建或更新资源（幂等操作）
kubectl apply -f deployment.yaml

# 重复执行安全
kubectl apply -f deployment.yaml   # 第一次：创建
kubectl apply -f deployment.yaml   # 第二次：无变更则跳过
kubectl apply -f deployment.yaml   # 第 N 次：仅更新变更字段

# 目录级别应用
kubectl apply -f manifests/        # 递归应用所有 YAML

# 从 URL 应用
kubectl apply -f https://example.com/deployment.yaml
```

#### 三方合并机制（Three-way Merge）

`kubectl apply` 使用**三方合并**算法，对比三个版本的配置：

```
┌──────────────────────────────────────────────────┐
│  1. Last Applied Configuration (上次应用的配置)    │
│     存储在注解中：                                 │
│     kubectl.kubernetes.io/last-applied-configuration│
└─────────────────┬────────────────────────────────┘
                  │
                  ▼
┌──────────────────────────────────────────────────┐
│  2. Current Configuration (当前配置文件)          │
│     用户本次提交的 YAML                           │
└─────────────────┬────────────────────────────────┘
                  │
                  ▼
┌──────────────────────────────────────────────────┐
│  3. Live Object Configuration (集群实际状态)      │
│     API Server 中存储的资源                       │
└─────────────────┬────────────────────────────────┘
                  │
                  ▼
       ┌──────────┴──────────┐
       │  三方合并算法         │
       │  计算最小变更集       │
       └──────────┬──────────┘
                  ▼
       ┌──────────────────────┐
       │  应用变更到集群      │
       └──────────────────────┘
```

#### 三方合并示例

```yaml
# === 初始配置（first-apply.yaml）===
apiVersion: apps/v1
kind: Deployment
metadata:
  name: nginx
spec:
  replicas: 2
  template:
    spec:
      containers:
      - name: nginx
        image: nginx:1.25

# 执行 kubectl apply -f first-apply.yaml
# 结果：创建 Deployment，并在 annotations 中存储配置
```

```bash
# 查看存储的 last-applied-configuration
kubectl get deployment nginx -o yaml | grep -A 20 last-applied-configuration

  annotations:
    kubectl.kubernetes.io/last-applied-configuration: |
      {"apiVersion":"apps/v1","kind":"Deployment",...,"replicas":2,...}
```

```yaml
# === 第二次更新（second-apply.yaml）===
apiVersion: apps/v1
kind: Deployment
metadata:
  name: nginx
spec:
  replicas: 3              # 修改副本数
  template:
    spec:
      containers:
      - name: nginx
        image: nginx:1.27  # 升级镜像版本

# 执行 kubectl apply -f second-apply.yaml
# 三方合并逻辑：
# 1. replicas: 2 → 3（用户修改，应用变更）
# 2. image: nginx:1.25 → nginx:1.27（用户修改，应用变更）
# 3. 其他字段保持不变
```

```yaml
# === 控制器自动添加的字段不会被覆盖 ===
# 假设 Deployment Controller 自动添加了 status 字段
status:
  replicas: 2
  availableReplicas: 2

# kubectl apply 只更新 spec 部分，status 保持不变
```

#### 字段删除行为

```yaml
# === 初始配置 ===
spec:
  replicas: 3
  strategy:
    type: RollingUpdate
    rollingUpdate:
      maxSurge: 1
      maxUnavailable: 0

# === 第二次配置（删除 strategy 字段）===
spec:
  replicas: 5

# kubectl apply 行为：
# - replicas: 3 → 5（更新）
# - strategy: 保持原值（apply 不删除未指定字段）

# kubectl replace 行为：
# - replicas: 3 → 5（更新）
# - strategy: 删除（replace 删除所有未指定字段）
```

**关键区别**：
- ✅ `apply`：保留未指定字段（智能合并）
- ❌ `replace`：删除未指定字段（完全替换）

### 7.2 Server-side Apply (v1.22+ GA)

Server-side Apply (SSA) 是 `kubectl apply` 的升级版本，由 API Server 执行合并逻辑。

#### Client-side Apply vs Server-side Apply

| 特性 | Client-side Apply | Server-side Apply |
|------|------------------|------------------|
| **合并逻辑执行位置** | kubectl 客户端 | API Server |
| **字段所有权管理** | ❌ 无（last-applied-configuration 注解） | ✅ 有（managedFields） |
| **冲突检测** | ❌ 弱（覆盖式） | ✅ 强（字段级冲突检测） |
| **多管理器支持** | ⚠️  困难 | ✅ 原生支持 |
| **性能** | ⚠️  需传输完整配置 | ✅ 仅传输变更字段 |
| **默认行为** | `kubectl apply` | `kubectl apply --server-side` |

#### 启用 Server-side Apply

```bash
# 使用 Server-side Apply
kubectl apply -f deployment.yaml --server-side

# 指定字段管理器名称
kubectl apply -f deployment.yaml \
  --server-side \
  --field-manager=my-controller

# 强制获取字段所有权（覆盖其他管理器）
kubectl apply -f deployment.yaml \
  --server-side \
  --force-conflicts
```

#### 字段所有权（Field Ownership）

```yaml
# Server-side Apply 自动记录字段所有权
apiVersion: apps/v1
kind: Deployment
metadata:
  name: nginx
  managedFields:
  - manager: kubectl        # 字段管理器 1
    operation: Apply
    apiVersion: apps/v1
    time: "2026-02-10T10:00:00Z"
    fieldsType: FieldsV1
    fieldsV1:
      f:spec:
        f:replicas: {}      # kubectl 管理 replicas 字段
        
  - manager: my-controller  # 字段管理器 2
    operation: Update
    apiVersion: apps/v1
    time: "2026-02-10T10:05:00Z"
    fieldsV1:
      f:spec:
        f:template:
          f:spec:
            f:containers: {}  # 控制器管理 containers 字段
```

#### 冲突检测与解决

```bash
# 场景：两个管理器尝试修改同一字段

# 管理器 A 应用配置
kubectl apply -f deployment.yaml \
  --server-side \
  --field-manager=manager-a

# 管理器 B 尝试修改相同字段（会失败）
kubectl apply -f deployment.yaml \
  --server-side \
  --field-manager=manager-b

# 错误输出
Apply failed with 1 conflict: conflict with "manager-a":
- .spec.replicas

# 解决方案 1：强制覆盖（谨慎使用）
kubectl apply -f deployment.yaml \
  --server-side \
  --field-manager=manager-b \
  --force-conflicts  # 强制获取所有权

# 解决方案 2：协调两个管理器，避免管理相同字段
```

#### 实际应用场景

```bash
# 1. GitOps 场景：ArgoCD 使用 SSA
kubectl apply -f application.yaml \
  --server-side \
  --field-manager=argocd-controller

# 2. Operator 场景：控制器使用 SSA
kubectl apply -f custom-resource.yaml \
  --server-side \
  --field-manager=my-operator

# 3. 多团队协作：不同团队管理不同字段
# 平台团队管理基础配置
kubectl apply -f base-deployment.yaml \
  --server-side \
  --field-manager=platform-team

# 应用团队管理业务配置
kubectl apply -f app-config.yaml \
  --server-side \
  --field-manager=app-team
```

---

## 8. 资源版本演进

### 8.1 API 版本生命周期

Kubernetes API 遵循**语义化版本控制**，经历三个阶段：

```
Alpha (v1alpha1) → Beta (v1beta1) → Stable (v1/v2)
    ↓                  ↓                ↓
  实验性             预发布            正式版
 默认禁用           默认启用          生产可用
```

#### 版本标识规则

| 版本格式 | 稳定性 | 向后兼容 | 默认启用 | 弃用策略 | 示例 |
|---------|--------|---------|---------|---------|------|
| **v1alpha1, v1alpha2** | ❌ 不稳定 | ❌ 否 | ❌ 否 | 随时可删除 | `flowcontrol.apiserver.k8s.io/v1alpha1` |
| **v1beta1, v2beta2** | ⚠️  较稳定 | ⚠️  尽力而为 | ✅ 是 | 9 个月 + 2 版本 | `autoscaling/v2beta2` |
| **v1, v2** | ✅ 稳定 | ✅ 是 | ✅ 是 | 12 个月 + 2 版本 | `apps/v1`, `batch/v1` |

#### 弃用策略详解

**规则 4a（GA API 版本）**：
- GA API 版本至少支持 **12 个月** 或 **2 个 Kubernetes 小版本**（取较长者）

**规则 4b（Beta API 版本）**：
- Beta API 版本至少支持 **9 个月** 或 **2 个 Kubernetes 小版本**

**示例时间线**：

```
2024-01-15: Kubernetes v1.29 发布
            - HorizontalPodAutoscaler autoscaling/v2beta2 标记为弃用
            - 推荐使用 autoscaling/v2

2024-04-15: Kubernetes v1.30 发布
            - autoscaling/v2beta2 仍然可用（弃用后第 1 个版本）

2024-08-15: Kubernetes v1.31 发布
            - autoscaling/v2beta2 仍然可用（弃用后第 2 个版本）

2024-12-15: Kubernetes v1.32 发布
            - autoscaling/v2beta2 正式移除（9 个月 + 2 个版本）
```

### 8.2 版本迁移策略

#### 1. 查看资源使用的 API 版本

```bash
# 查看集群中所有 API 版本
kubectl api-versions

# 查看特定资源的 API 版本
kubectl api-resources | grep horizontalpodautoscalers
NAME                      SHORTNAMES   APIVERSION              NAMESPACED   KIND
horizontalpodautoscalers  hpa          autoscaling/v2          true         HorizontalPodAutoscaler

# 检查资源是否使用弃用的 API 版本
kubectl get hpa -o yaml | grep apiVersion
apiVersion: autoscaling/v2beta2  # ⚠️  弃用版本
```

#### 2. 手动迁移步骤

```bash
# 步骤 1：导出现有资源
kubectl get hpa my-hpa -o yaml > hpa-old.yaml

# 步骤 2：修改 apiVersion
sed -i 's/autoscaling\/v2beta2/autoscaling\/v2/g' hpa-old.yaml

# 步骤 3：验证新配置
kubectl apply -f hpa-old.yaml --dry-run=server

# 步骤 4：应用新配置
kubectl apply -f hpa-old.yaml

# 步骤 5：验证迁移成功
kubectl get hpa my-hpa -o yaml | grep apiVersion
apiVersion: autoscaling/v2  # ✅ 新版本
```

#### 3. 批量迁移脚本

```bash
#!/bin/bash
# 批量迁移 HPA 从 v2beta2 到 v2

# 获取所有使用旧版本的 HPA
kubectl get hpa --all-namespaces -o json | \
  jq -r '.items[] | select(.apiVersion == "autoscaling/v2beta2") | 
  "\(.metadata.namespace) \(.metadata.name)"' | \
while read ns name; do
  echo "Migrating HPA: $ns/$name"
  
  # 导出并修改版本
  kubectl get hpa -n "$ns" "$name" -o yaml | \
    sed 's/autoscaling\/v2beta2/autoscaling\/v2/g' | \
    kubectl apply -f -
done
```

#### 4. 版本差异处理

某些 API 版本变更会引入**字段变化**，需要手动调整。

```yaml
# === autoscaling/v2beta2（旧版）===
apiVersion: autoscaling/v2beta2
kind: HorizontalPodAutoscaler
metadata:
  name: my-hpa
spec:
  scaleTargetRef:
    apiVersion: apps/v1
    kind: Deployment
    name: my-app
  minReplicas: 2
  maxReplicas: 10
  metrics:
  - type: Resource
    resource:
      name: cpu
      target:
        type: Utilization
        averageUtilization: 80  # v2beta2 字段

---
# === autoscaling/v2（新版）===
apiVersion: autoscaling/v2
kind: HorizontalPodAutoscaler
metadata:
  name: my-hpa
spec:
  scaleTargetRef:
    apiVersion: apps/v1
    kind: Deployment
    name: my-app
  minReplicas: 2
  maxReplicas: 10
  metrics:
  - type: Resource
    resource:
      name: cpu
      target:
        type: Utilization
        averageUtilization: 80  # v2 保持兼容
  behavior:                     # v2 新增字段（可选）
    scaleDown:
      stabilizationWindowSeconds: 300
```

#### 5. 使用 Pluto 检测弃用 API

```bash
# 安装 Pluto（API 弃用检测工具）
brew install FairwindsOps/tap/pluto

# 检测 YAML 文件中的弃用 API
pluto detect-files -d manifests/

# 输出示例
NAME        KIND                      VERSION              REPLACEMENT   DEPRECATED   DEPRECATED IN   REMOVED   REMOVED IN
my-hpa      HorizontalPodAutoscaler   autoscaling/v2beta2  autoscaling/v2   true         v1.23           true      v1.26

# 检测集群中的弃用 API
pluto detect-helm --helm-version 3

# 检测 Helm Chart
pluto detect-helm -owide --helm-version=3
```

---

## 9. 快速查询索引表

### 9.1 按 apiVersion 分组的资源清单

| apiVersion | kind | shortName | namespaced | description |
|-----------|------|-----------|-----------|-------------|
| **v1** (核心) | | | | |
| v1 | Pod | po | ✅ | 最小调度单元 |
| v1 | Service | svc | ✅ | 服务发现与负载均衡 |
| v1 | ConfigMap | cm | ✅ | 配置数据 |
| v1 | Secret | secret | ✅ | 敏感数据 |
| v1 | Namespace | ns | ❌ | 命名空间 |
| v1 | Node | no | ❌ | 集群节点 |
| v1 | PersistentVolume | pv | ❌ | 持久化存储卷 |
| v1 | PersistentVolumeClaim | pvc | ✅ | 存储卷申领 |
| v1 | ServiceAccount | sa | ✅ | 服务账号 |
| v1 | Endpoints | ep | ✅ | 服务端点 |
| v1 | Event | ev | ✅ | 事件 |
| v1 | LimitRange | limits | ✅ | 资源限制范围 |
| v1 | ResourceQuota | quota | ✅ | 资源配额 |
| **apps/v1** | | | | |
| apps/v1 | Deployment | deploy | ✅ | 无状态应用 |
| apps/v1 | StatefulSet | sts | ✅ | 有状态应用 |
| apps/v1 | DaemonSet | ds | ✅ | 节点守护进程 |
| apps/v1 | ReplicaSet | rs | ✅ | 副本集 |
| **batch/v1** | | | | |
| batch/v1 | Job | job | ✅ | 一次性任务 |
| batch/v1 | CronJob | cj | ✅ | 定时任务 |
| **networking.k8s.io/v1** | | | | |
| networking.k8s.io/v1 | Ingress | ing | ✅ | HTTP(S) 路由 |
| networking.k8s.io/v1 | NetworkPolicy | netpol | ✅ | 网络策略 |
| networking.k8s.io/v1 | IngressClass | - | ❌ | Ingress 控制器类 |
| **policy/v1** | | | | |
| policy/v1 | PodDisruptionBudget | pdb | ✅ | 中断预算 |
| **rbac.authorization.k8s.io/v1** | | | | |
| rbac.authorization.k8s.io/v1 | Role | - | ✅ | 角色 |
| rbac.authorization.k8s.io/v1 | ClusterRole | - | ❌ | 集群角色 |
| rbac.authorization.k8s.io/v1 | RoleBinding | - | ✅ | 角色绑定 |
| rbac.authorization.k8s.io/v1 | ClusterRoleBinding | - | ❌ | 集群角色绑定 |
| **storage.k8s.io/v1** | | | | |
| storage.k8s.io/v1 | StorageClass | sc | ❌ | 存储类 |
| storage.k8s.io/v1 | VolumeAttachment | - | ❌ | 卷挂载 |
| storage.k8s.io/v1 | CSIDriver | - | ❌ | CSI 驱动 |
| storage.k8s.io/v1 | CSINode | - | ❌ | CSI 节点信息 |
| **autoscaling/v2** | | | | |
| autoscaling/v2 | HorizontalPodAutoscaler | hpa | ✅ | 水平弹性伸缩 |
| **certificates.k8s.io/v1** | | | | |
| certificates.k8s.io/v1 | CertificateSigningRequest | csr | ❌ | 证书签名请求 |

### 9.2 常用 kubectl 命令速查

| 命令 | 用途 | 示例 |
|------|------|------|
| `kubectl apply -f <file>` | 声明式创建/更新 | `kubectl apply -f deployment.yaml` |
| `kubectl create -f <file>` | 命令式创建 | `kubectl create -f pod.yaml` |
| `kubectl get <resource>` | 查询资源 | `kubectl get pods` |
| `kubectl describe <resource> <name>` | 查看详细信息 | `kubectl describe pod nginx` |
| `kubectl delete <resource> <name>` | 删除资源 | `kubectl delete pod nginx` |
| `kubectl edit <resource> <name>` | 在线编辑 | `kubectl edit deployment nginx` |
| `kubectl scale deployment <name> --replicas=N` | 扩缩容 | `kubectl scale deployment nginx --replicas=5` |
| `kubectl rollout status deployment/<name>` | 查看滚动更新状态 | `kubectl rollout status deployment/nginx` |
| `kubectl rollout undo deployment/<name>` | 回滚 | `kubectl rollout undo deployment/nginx` |
| `kubectl logs <pod>` | 查看日志 | `kubectl logs nginx-pod` |
| `kubectl exec -it <pod> -- <cmd>` | 进入容器 | `kubectl exec -it nginx-pod -- bash` |
| `kubectl port-forward <pod> <local>:<remote>` | 端口转发 | `kubectl port-forward nginx-pod 8080:80` |
| `kubectl api-resources` | 查看所有资源类型 | `kubectl api-resources` |
| `kubectl explain <resource>` | 查看资源字段文档 | `kubectl explain pod.spec.containers` |

---

## 10. 相关资源

### 内部参考文档

- [02 - Namespace / ResourceQuota / LimitRange YAML 配置参考](./02-namespace-resourcequota-limitrange.md)
- [Domain 5 - 网络深入](../domain-5-networking/README.md)
- [Domain 3 - 工作负载与调度](../domain-3-workload-scheduling/README.md)

### 官方文档

- [Kubernetes API 概念](https://kubernetes.io/docs/reference/using-api/api-concepts/)
- [对象元数据](https://kubernetes.io/docs/reference/kubernetes-api/common-definitions/object-meta/)
- [推荐标签](https://kubernetes.io/docs/concepts/overview/working-with-objects/common-labels/)
- [API 弃用策略](https://kubernetes.io/docs/reference/using-api/deprecation-policy/)
- [Server-side Apply](https://kubernetes.io/docs/reference/using-api/server-side-apply/)

### 工具链

- [kubectl Reference](https://kubernetes.io/docs/reference/kubectl/)
- [Helm](https://helm.sh/) - Kubernetes 包管理器
- [Kustomize](https://kustomize.io/) - YAML 配置管理
- [Pluto](https://github.com/FairwindsOps/pluto) - API 弃用检测
- [kubeval](https://kubeval.instrumenta.dev/) - YAML 验证工具

---

**文档维护说明**：本文档覆盖 Kubernetes v1.25 至 v1.32 版本的 YAML 语法与资源规范。随着 Kubernetes 版本演进，部分 API 可能会弃用或变更，请结合官方发布说明使用。

**字数统计**：约 12,000 字 | **代码示例**：80+ 个 | **表格**：20+ 个

---

> 📖 **学习路径建议**：
> 1. 初学者：重点阅读第 1-4 节（语法基础与资源结构）
> 2. 进阶用户：重点阅读第 5-7 节（标签、验证、kubectl 机制）
> 3. 专家用户：重点阅读第 7.2、8 节（Server-side Apply、版本演进）
