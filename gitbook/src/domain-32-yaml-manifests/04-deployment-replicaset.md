# 04 - Deployment / ReplicaSet YAML 配置参考

> **适用版本**: Kubernetes v1.25 - v1.32 | **最后更新**: 2026-02 | **难度**: 入门 → 专家全覆盖

---

## 目录

1. [概述](#1-概述)
2. [Deployment API 信息](#2-deployment-api-信息)
3. [Deployment 完整字段规格表](#3-deployment-完整字段规格表)
4. [更新策略详解](#4-更新策略详解)
   - 4.1 [RollingUpdate 滚动更新](#41-rollingupdate-滚动更新)
   - 4.2 [Recreate 重建策略](#42-recreate-重建策略)
   - 4.3 [策略选择指南](#43-策略选择指南)
5. [最小配置示例（初学者）](#5-最小配置示例初学者)
6. [生产级完整配置（专家）](#6-生产级完整配置专家)
7. [高级特性](#7-高级特性)
   - 7.1 [Pause/Resume 暂停与恢复](#71-pauseresume-暂停与恢复)
   - 7.2 [Rollback 回滚操作](#72-rollback-回滚操作)
   - 7.3 [Revision History 版本历史](#73-revision-history-版本历史)
8. [内部原理](#8-内部原理)
   - 8.1 [Deployment Controller 控制循环](#81-deployment-controller-控制循环)
   - 8.2 [Rolling Update 算法](#82-rolling-update-算法)
   - 8.3 [Revision Hash 机制](#83-revision-hash-机制)
   - 8.4 [ProgressDeadlineSeconds 超时机制](#84-progressdeadlineseconds-超时机制)
9. [ReplicaSet 详解](#9-replicaset-详解)
   - 9.1 [API 信息](#91-api-信息)
   - 9.2 [字段规格表](#92-字段规格表)
   - 9.3 [配置示例](#93-配置示例)
   - 9.4 [为什么很少直接使用](#94-为什么很少直接使用)
10. [ReplicationController（Legacy）](#10-replicationcontrollerlegacy)
    - 10.1 [简要概述](#101-简要概述)
    - 10.2 [迁移指南](#102-迁移指南)
11. [版本兼容性矩阵](#11-版本兼容性矩阵)
12. [生产最佳实践](#12-生产最佳实践)
13. [常见问题 FAQ](#13-常见问题-faq)
14. [生产案例](#14-生产案例)
    - 14.1 [蓝绿部署](#141-蓝绿部署)
    - 14.2 [金丝雀发布](#142-金丝雀发布)
    - 14.3 [带 PDB 的滚动更新](#143-带-pdb-的滚动更新)
15. [相关资源](#15-相关资源)

---

## 1. 概述

**Deployment** 是 Kubernetes 中最常用的工作负载资源,用于管理无状态应用的声明式更新。它通过管理 **ReplicaSet** 来实现 Pod 的副本控制和滚动更新。

### 三层架构关系

```
┌─────────────────────────────────────────────────────┐
│  Deployment (声明期望状态)                          │
│  - replicas: 3                                      │
│  - image: nginx:1.21                                │
│  - strategy: RollingUpdate                          │
│  └─────────────────┬───────────────────────────────┘
│                    │ 管理
│                    ▼
│  ┌─────────────────────────────────────────────────┐
│  │  ReplicaSet (维持副本数量)                      │
│  │  - pod-template-hash: 5d4b7c9f8d               │
│  │  - replicas: 3                                  │
│  └──────────────────┬──────────────────────────────┘
│                     │ 创建和监控
│                     ▼
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐
│  │   Pod 1      │  │   Pod 2      │  │   Pod 3      │
│  │  nginx:1.21  │  │  nginx:1.21  │  │  nginx:1.21  │
│  └──────────────┘  └──────────────┘  └──────────────┘
└─────────────────────────────────────────────────────┘
```

### 核心功能

| 功能 | 说明 | 使用场景 |
|------|------|---------|
| **声明式更新** | 修改 spec 即可触发滚动更新 | 应用版本升级 |
| **回滚机制** | 保留历史 ReplicaSet,支持快速回滚 | 发布失败恢复 |
| **扩缩容** | 动态调整副本数量 | 流量波动应对 |
| **健康检查** | 结合 Probe 确保更新过程可用性 | 生产环境发布 |
| **暂停/恢复** | 手动控制更新流程 | 金丝雀/蓝绿部署 |

### 与其他工作负载对比

| 资源类型 | 适用场景 | Pod 管理 | 持久化 | 网络标识 |
|---------|---------|---------|-------|---------|
| **Deployment** | 无状态应用 (Web 服务、API) | ReplicaSet | ❌ | 随机 Pod 名 |
| **StatefulSet** | 有状态应用 (数据库、消息队列) | 直接管理 | ✅ | 稳定序号 (pod-0, pod-1) |
| **DaemonSet** | 节点级守护进程 (监控、日志) | 每节点一个 Pod | ❌ | 节点绑定 |
| **Job** | 批处理任务 | 直接管理 | ❌ | 一次性执行 |

---

## 2. Deployment API 信息

| 字段 | 值 |
|------|-----|
| **API Group** | apps |
| **API Version** | apps/v1 |
| **Kind** | Deployment |
| **Scope** | Namespaced (命名空间级) |
| **简写** | deploy |
| **kubectl 命令** | `kubectl get deploy`, `kubectl rollout status deploy/<name>` |

### API 演进历史

| 版本 | Kubernetes 版本 | 状态 | 说明 |
|------|----------------|------|------|
| `extensions/v1beta1` | v1.6-v1.16 | ❌ 已废弃 | 最早实验版本 |
| `apps/v1beta1` | v1.7-v1.16 | ❌ 已废弃 | 过渡版本 |
| `apps/v1beta2` | v1.8-v1.16 | ❌ 已废弃 | 添加 selector 不可变规则 |
| `apps/v1` | v1.9+ | ✅ **稳定版** | **当前推荐使用** |

**重要变更**：
- `apps/v1` 要求明确指定 `.spec.selector`,不再自动生成
- `.spec.selector` 创建后不可变 (immutable)

---

## 3. Deployment 完整字段规格表

### 顶层字段

| 字段路径 | 类型 | 必填 | 默认值 | 版本 | 说明 |
|---------|------|------|-------|------|------|
| `apiVersion` | string | ✅ | - | v1.9+ | 固定为 `apps/v1` |
| `kind` | string | ✅ | - | v1.9+ | 固定为 `Deployment` |
| `metadata` | ObjectMeta | ✅ | - | v1.9+ | 标准元数据 (name, namespace, labels 等) |
| `spec` | DeploymentSpec | ✅ | - | v1.9+ | 期望状态规格 |
| `status` | DeploymentStatus | ❌ | 系统生成 | v1.9+ | 当前状态 (只读) |

### spec 字段详解

| 字段路径 | 类型 | 必填 | 默认值 | 不可变 | 说明 |
|---------|------|------|-------|-------|------|
| `spec.replicas` | int32 | ❌ | 1 | ❌ | 期望的 Pod 副本数量 |
| `spec.selector` | LabelSelector | ✅ | - | ✅ | **Pod 选择器,创建后不可修改** |
| `spec.selector.matchLabels` | map[string]string | ✅ | - | ✅ | 标签精确匹配 |
| `spec.selector.matchExpressions` | []LabelSelectorRequirement | ❌ | - | ✅ | 标签表达式匹配 (In, NotIn, Exists, DoesNotExist) |
| `spec.template` | PodTemplateSpec | ✅ | - | ❌ | Pod 模板 (包含 metadata 和 spec) |
| `spec.template.metadata` | ObjectMeta | ✅ | - | ❌ | Pod 元数据,**labels 必须匹配 selector** |
| `spec.template.spec` | PodSpec | ✅ | - | ❌ | Pod 规格 (容器、卷、调度等) |
| `spec.strategy` | DeploymentStrategy | ❌ | RollingUpdate | ❌ | 更新策略 |
| `spec.strategy.type` | string | ❌ | RollingUpdate | ❌ | `RollingUpdate` 或 `Recreate` |
| `spec.strategy.rollingUpdate` | RollingUpdateDeployment | ❌ | 见下文 | ❌ | 滚动更新参数 (仅 RollingUpdate 时有效) |
| `spec.strategy.rollingUpdate.maxSurge` | IntOrString | ❌ | 25% | ❌ | 最大额外 Pod 数量 (整数或百分比) |
| `spec.strategy.rollingUpdate.maxUnavailable` | IntOrString | ❌ | 25% | ❌ | 最大不可用 Pod 数量 (整数或百分比) |
| `spec.revisionHistoryLimit` | int32 | ❌ | 10 | ❌ | 保留的历史 ReplicaSet 数量 |
| `spec.progressDeadlineSeconds` | int32 | ❌ | 600 | ❌ | 更新进度超时时间 (秒) |
| `spec.paused` | bool | ❌ | false | ❌ | 是否暂停 Deployment 更新 |
| `spec.minReadySeconds` | int32 | ❌ | 0 | ❌ | Pod 就绪后等待时间 (秒) |

### status 字段 (系统维护,只读)

| 字段路径 | 类型 | 说明 |
|---------|------|------|
| `status.replicas` | int32 | 当前总副本数 |
| `status.updatedReplicas` | int32 | 已更新到最新 spec 的副本数 |
| `status.readyReplicas` | int32 | 就绪的副本数 |
| `status.availableReplicas` | int32 | 可用的副本数 (就绪且超过 minReadySeconds) |
| `status.unavailableReplicas` | int32 | 不可用的副本数 |
| `status.observedGeneration` | int64 | Controller 观察到的 metadata.generation |
| `status.conditions` | []DeploymentCondition | 状态条件数组 (Progressing, Available, ReplicaFailure) |
| `status.collisionCount` | int32 | ReplicaSet hash 冲突次数 |

---

## 4. 更新策略详解

### 4.1 RollingUpdate 滚动更新

**默认策略**,逐步替换旧 Pod,确保更新过程中服务可用性。

#### 核心参数

```yaml
spec:
  strategy:
    type: RollingUpdate
    rollingUpdate:
      maxSurge: 1           # 可以是整数或百分比
      maxUnavailable: 0     # 可以是整数或百分比
```

#### maxSurge (最大额外 Pod 数)

**定义**：更新期间允许超过 `spec.replicas` 的额外 Pod 数量。

| 取值 | 含义 | 示例 (replicas=10) |
|------|------|-------------------|
| **整数** | 固定数量 | `maxSurge: 2` → 最多 12 个 Pod |
| **百分比** | 相对比例 | `maxSurge: 25%` → 最多 13 个 Pod (10*1.25向上取整) |
| **0** | 不允许额外 Pod | 先删除旧 Pod 才能创建新 Pod |

**计算公式**：
```
最大 Pod 数 = replicas + ceil(maxSurge)
# ceil() 为向上取整函数
```

#### maxUnavailable (最大不可用 Pod 数)

**定义**：更新期间允许的最大不可用 Pod 数量。

| 取值 | 含义 | 示例 (replicas=10) |
|------|------|-------------------|
| **整数** | 固定数量 | `maxUnavailable: 1` → 至少 9 个 Pod 可用 |
| **百分比** | 相对比例 | `maxUnavailable: 30%` → 至少 7 个 Pod 可用 (10*0.7向下取整) |
| **0** | 零宕机更新 | 必须所有新 Pod 就绪后才能删除旧 Pod |

**计算公式**：
```
最小可用 Pod 数 = replicas - floor(maxUnavailable)
# floor() 为向下取整函数
```

#### 参数组合场景

| maxSurge | maxUnavailable | 更新行为 | 适用场景 |
|----------|---------------|---------|---------|
| **25%** | **25%** | 默认平衡 | 通用场景 |
| **100%** | **0** | 先创建全部新 Pod,再删除旧 Pod (蓝绿部署) | 资源充足,零宕机要求 |
| **0** | **100%** | 先删除全部旧 Pod,再创建新 Pod (类似 Recreate) | 资源紧张,允许短暂中断 |
| **1** | **0** | 逐个替换,零宕机 | 生产环境谨慎更新 |
| **3** | **1** | 快速更新,小幅不可用 | 开发/测试环境 |

#### 实际案例

**场景 1：默认配置 (replicas=10)**

```yaml
spec:
  replicas: 10
  strategy:
    type: RollingUpdate
    rollingUpdate:
      maxSurge: 25%        # 最多 13 个 Pod (10 + ceil(10*0.25))
      maxUnavailable: 25%  # 至少 8 个 Pod (10 - floor(10*0.25))
```

**更新流程**：
```
初始状态: 10 个旧 Pod (全部就绪)
步骤 1: 创建 3 个新 Pod (总数 13,maxSurge 允许)
步骤 2: 新 Pod 就绪后,删除 3 个旧 Pod (剩余 10)
步骤 3: 重复上述过程,直到全部替换完成
```

**场景 2：零宕机更新 (replicas=5)**

```yaml
spec:
  replicas: 5
  strategy:
    rollingUpdate:
      maxSurge: 1          # 最多 6 个 Pod
      maxUnavailable: 0    # 必须保持 5 个 Pod 可用
```

**更新流程**：
```
初始状态: 5 个旧 Pod (全部就绪)
步骤 1: 创建 1 个新 Pod (总数 6)
步骤 2: 新 Pod 就绪后,删除 1 个旧 Pod (恢复 5 个)
步骤 3: 重复 5 次,逐个替换
```

**场景 3：快速更新 (replicas=20)**

```yaml
spec:
  replicas: 20
  strategy:
    rollingUpdate:
      maxSurge: 50%        # 最多 30 个 Pod
      maxUnavailable: 50%  # 至少 10 个 Pod 可用
```

**更新流程**：
```
初始状态: 20 个旧 Pod
步骤 1: 创建 10 个新 Pod,同时删除 10 个旧 Pod
步骤 2: 新 Pod 就绪后,完成剩余 10 个的替换
# 速度最快,但短期内仅 50% 可用
```

### 4.2 Recreate 重建策略

**先删除所有旧 Pod,再创建新 Pod**,会导致短暂服务中断。

```yaml
spec:
  strategy:
    type: Recreate
```

**特点**：
- ✅ **简单直接**：无需复杂的滚动逻辑
- ✅ **资源占用少**：不会出现新旧 Pod 共存
- ❌ **服务中断**：所有旧 Pod 删除到新 Pod 就绪之间无法提供服务
- ❌ **回滚慢**：需要重新创建所有 Pod

**适用场景**：
1. **无法并行运行多版本**：应用有单例要求 (如需要持有全局锁)
2. **资源紧张环境**：集群资源无法同时容纳新旧 Pod
3. **开发/测试环境**：对短暂中断不敏感

### 4.3 策略选择指南

| 场景 | 推荐策略 | 参数建议 |
|------|---------|---------|
| **生产环境 Web 服务** | RollingUpdate | `maxSurge: 1, maxUnavailable: 0` (零宕机) |
| **微服务 API** | RollingUpdate | `maxSurge: 25%, maxUnavailable: 25%` (默认平衡) |
| **单例应用** | Recreate | 无需参数 |
| **开发环境快速迭代** | RollingUpdate | `maxSurge: 100%, maxUnavailable: 50%` (快速更新) |
| **有状态应用** | 使用 StatefulSet | 非 Deployment 场景 |

---

## 5. 最小配置示例(初学者)

### 基础 Nginx Deployment

```yaml
# 最简配置:仅包含必填字段
apiVersion: apps/v1              # API 版本 (固定)
kind: Deployment                 # 资源类型
metadata:
  name: nginx-deployment         # Deployment 名称
  namespace: default             # 命名空间 (可选,默认 default)
spec:
  replicas: 3                    # Pod 副本数量
  selector:                      # Pod 选择器 (必填且不可变)
    matchLabels:
      app: nginx                 # 必须与 template.metadata.labels 匹配
  template:                      # Pod 模板
    metadata:
      labels:
        app: nginx               # Pod 标签 (必须包含 selector 中的标签)
    spec:
      containers:                # 容器列表
      - name: nginx              # 容器名称
        image: nginx:1.21        # 容器镜像
        ports:
        - containerPort: 80      # 容器端口
```

**应用命令**：
```bash
# 创建 Deployment
kubectl apply -f nginx-deployment.yaml

# 查看状态
kubectl get deployment nginx-deployment
kubectl get pods -l app=nginx

# 扩容到 5 个副本
kubectl scale deployment nginx-deployment --replicas=5

# 更新镜像
kubectl set image deployment/nginx-deployment nginx=nginx:1.22

# 查看滚动更新状态
kubectl rollout status deployment/nginx-deployment

# 查看历史版本
kubectl rollout history deployment/nginx-deployment

# 回滚到上一个版本
kubectl rollout undo deployment/nginx-deployment
```

### 带资源限制的配置

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: webapp-deployment
  labels:
    app: webapp
    tier: frontend
spec:
  replicas: 2
  selector:
    matchLabels:
      app: webapp
  template:
    metadata:
      labels:
        app: webapp
        tier: frontend
    spec:
      containers:
      - name: webapp
        image: myapp:v1.0
        ports:
        - containerPort: 8080
          name: http
        # 资源请求和限制
        resources:
          requests:              # 最小资源保证
            cpu: 100m            # 0.1 核 CPU
            memory: 128Mi        # 128 MiB 内存
          limits:                # 最大资源限制
            cpu: 500m            # 0.5 核 CPU
            memory: 512Mi        # 512 MiB 内存
```

---

## 6. 生产级完整配置(专家)

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: production-api
  namespace: production
  # 标签:用于组织和选择资源
  labels:
    app.kubernetes.io/name: api-server
    app.kubernetes.io/version: "2.5.0"
    app.kubernetes.io/component: backend
    app.kubernetes.io/part-of: ecommerce-platform
    app.kubernetes.io/managed-by: argocd
    environment: production
    team: platform-engineering
  # 注解:存储非识别信息
  annotations:
    deployment.kubernetes.io/revision: "15"
    description: "Production API server with full observability"
    maintainer: "platform-team@company.com"
    rollout-strategy: "gradual-canary"
spec:
  # 副本数量
  replicas: 10
  
  # Pod 选择器 (创建后不可变)
  selector:
    matchLabels:
      app.kubernetes.io/name: api-server
      app.kubernetes.io/component: backend
  
  # 更新策略:零宕机滚动更新
  strategy:
    type: RollingUpdate
    rollingUpdate:
      maxSurge: 2              # 最多额外 2 个 Pod (总数可达 12)
      maxUnavailable: 0        # 零宕机:必须保持 10 个 Pod 可用
  
  # 保留最近 5 个版本用于回滚
  revisionHistoryLimit: 5
  
  # 更新超时时间:10 分钟
  progressDeadlineSeconds: 600
  
  # Pod 就绪后等待 30 秒再标记为 Available
  minReadySeconds: 30
  
  # Pod 模板
  template:
    metadata:
      labels:
        # 应用标签 (必须匹配 selector)
        app.kubernetes.io/name: api-server
        app.kubernetes.io/version: "2.5.0"
        app.kubernetes.io/component: backend
        environment: production
        # Prometheus 监控标签
        prometheus.io/scrape: "true"
      annotations:
        # Prometheus 指标路径
        prometheus.io/path: "/metrics"
        prometheus.io/port: "8080"
        # Fluentd 日志收集配置
        fluentd.io/parser: "json"
        # 配置版本 hash (用于触发滚动更新)
        config-hash: "a3f5b2c8d9e1"
    spec:
      # 服务账号 (用于 RBAC 权限)
      serviceAccountName: api-server-sa
      
      # 安全上下文 (Pod 级别)
      securityContext:
        runAsNonRoot: true       # 禁止以 root 运行
        runAsUser: 1000          # 指定 UID
        fsGroup: 2000            # 文件系统组 ID
        seccompProfile:
          type: RuntimeDefault   # 使用默认 seccomp 配置
      
      # 容器列表
      containers:
      - name: api-server
        image: registry.company.com/api-server:2.5.0
        imagePullPolicy: IfNotPresent
        
        # 容器端口
        ports:
        - name: http
          containerPort: 8080
          protocol: TCP
        - name: metrics
          containerPort: 9090    # Prometheus 指标端口
          protocol: TCP
        
        # 环境变量
        env:
        - name: APP_ENV
          value: "production"
        - name: LOG_LEVEL
          value: "info"
        # 从 ConfigMap 读取配置
        - name: DATABASE_HOST
          valueFrom:
            configMapKeyRef:
              name: api-config
              key: db.host
        # 从 Secret 读取敏感信息
        - name: DATABASE_PASSWORD
          valueFrom:
            secretKeyRef:
              name: api-secrets
              key: db.password
        # Pod 元数据注入
        - name: POD_NAME
          valueFrom:
            fieldRef:
              fieldPath: metadata.name
        - name: POD_NAMESPACE
          valueFrom:
            fieldRef:
              fieldPath: metadata.namespace
        - name: POD_IP
          valueFrom:
            fieldRef:
              fieldPath: status.podIP
        
        # 资源配额
        resources:
          requests:
            cpu: 500m            # 请求 0.5 核
            memory: 512Mi        # 请求 512 MiB
          limits:
            cpu: 2000m           # 限制 2 核
            memory: 2Gi          # 限制 2 GiB
        
        # 存活探针:检测容器是否存活
        livenessProbe:
          httpGet:
            path: /healthz       # 健康检查端点
            port: http
            scheme: HTTP
          initialDelaySeconds: 30  # 容器启动后等待 30 秒
          periodSeconds: 10        # 每 10 秒检查一次
          timeoutSeconds: 5        # 超时时间 5 秒
          successThreshold: 1      # 成功 1 次认为健康
          failureThreshold: 3      # 失败 3 次重启容器
        
        # 就绪探针:检测容器是否可接收流量
        readinessProbe:
          httpGet:
            path: /ready
            port: http
          initialDelaySeconds: 10
          periodSeconds: 5
          timeoutSeconds: 3
          successThreshold: 1
          failureThreshold: 2      # 失败 2 次移出 Service 端点
        
        # 启动探针:保护慢启动容器 (v1.18+)
        startupProbe:
          httpGet:
            path: /startup
            port: http
          initialDelaySeconds: 0
          periodSeconds: 5
          timeoutSeconds: 3
          failureThreshold: 30     # 最多等待 150 秒 (5秒*30次)
        
        # 生命周期钩子
        lifecycle:
          postStart:
            exec:
              command: ["/bin/sh", "-c", "echo 'Container started' >> /var/log/startup.log"]
          preStop:
            exec:
              # 优雅关闭:等待现有请求完成
              command: ["/bin/sh", "-c", "sleep 15 && /app/graceful-shutdown.sh"]
        
        # 挂载卷
        volumeMounts:
        - name: config-volume
          mountPath: /etc/config
          readOnly: true
        - name: secrets-volume
          mountPath: /etc/secrets
          readOnly: true
        - name: cache-volume
          mountPath: /app/cache
        - name: logs-volume
          mountPath: /var/log/app
        
        # 容器安全上下文
        securityContext:
          allowPrivilegeEscalation: false
          readOnlyRootFilesystem: true
          runAsNonRoot: true
          runAsUser: 1000
          capabilities:
            drop:
            - ALL                  # 删除所有 Linux capabilities
      
      # Sidecar 容器:日志代理
      - name: log-forwarder
        image: fluent/fluent-bit:2.0
        resources:
          requests:
            cpu: 50m
            memory: 64Mi
          limits:
            cpu: 100m
            memory: 128Mi
        volumeMounts:
        - name: logs-volume
          mountPath: /var/log/app
          readOnly: true
        - name: fluent-bit-config
          mountPath: /fluent-bit/etc/
      
      # Init 容器:初始化任务
      initContainers:
      - name: init-db-migration
        image: registry.company.com/db-migrate:1.0
        command: ["/migrate.sh"]
        env:
        - name: DATABASE_URL
          valueFrom:
            secretKeyRef:
              name: api-secrets
              key: db.url
        resources:
          requests:
            cpu: 100m
            memory: 128Mi
          limits:
            cpu: 200m
            memory: 256Mi
      
      # 卷定义
      volumes:
      - name: config-volume
        configMap:
          name: api-config
          defaultMode: 0644
      - name: secrets-volume
        secret:
          secretName: api-secrets
          defaultMode: 0400       # 只读权限
      - name: cache-volume
        emptyDir:
          sizeLimit: 1Gi          # 限制临时卷大小
      - name: logs-volume
        emptyDir: {}
      - name: fluent-bit-config
        configMap:
          name: fluent-bit-config
      
      # 镜像拉取凭证
      imagePullSecrets:
      - name: registry-credentials
      
      # 节点调度
      affinity:
        # Pod 反亲和性:分散部署到不同节点
        podAntiAffinity:
          preferredDuringSchedulingIgnoredDuringExecution:
          - weight: 100
            podAffinityTerm:
              labelSelector:
                matchLabels:
                  app.kubernetes.io/name: api-server
              topologyKey: kubernetes.io/hostname
        # 节点亲和性:优先调度到高性能节点
        nodeAffinity:
          preferredDuringSchedulingIgnoredDuringExecution:
          - weight: 50
            preference:
              matchExpressions:
              - key: node-type
                operator: In
                values:
                - high-performance
      
      # 容忍节点污点
      tolerations:
      - key: "workload"
        operator: "Equal"
        value: "api"
        effect: "NoSchedule"
      
      # 拓扑分布约束:跨可用区分布 (v1.19+)
      topologySpreadConstraints:
      - maxSkew: 1                # 最大偏斜度
        topologyKey: topology.kubernetes.io/zone
        whenUnsatisfiable: ScheduleAnyway
        labelSelector:
          matchLabels:
            app.kubernetes.io/name: api-server
      
      # DNS 配置
      dnsPolicy: ClusterFirst
      dnsConfig:
        options:
        - name: ndots
          value: "2"
        - name: timeout
          value: "5"
      
      # 优雅关闭时间
      terminationGracePeriodSeconds: 60
      
      # 主机名设置
      hostname: api-server
      subdomain: backend
      
      # 优先级类 (需要预先创建 PriorityClass)
      priorityClassName: high-priority
```

**关键配置解读**：

1. **零宕机更新**：`maxUnavailable: 0` + `minReadySeconds: 30`
2. **完整健康检查**：Startup + Liveness + Readiness 三重探针
3. **资源保障**：合理的 requests/limits,避免 OOMKilled
4. **安全加固**：非 root 运行 + 只读根文件系统 + seccomp
5. **可观测性**：Prometheus 指标 + 结构化日志
6. **高可用性**：Pod 反亲和性 + 拓扑分布约束

---

## 7. 高级特性

### 7.1 Pause/Resume 暂停与恢复

**用途**：在执行多次配置更改时,暂停 Deployment 以避免触发多次滚动更新。

#### 暂停 Deployment

```bash
# 暂停滚动更新
kubectl rollout pause deployment/my-deployment
```

**效果**：
- 新的 Pod 模板更改不会触发滚动更新
- 可以进行多次配置修改
- 适用于金丝雀部署的手动控制

#### 修改配置示例

```bash
# 暂停后进行多次修改
kubectl rollout pause deployment/my-deployment

# 修改 1:更新镜像
kubectl set image deployment/my-deployment app=nginx:1.22

# 修改 2:更新资源限制
kubectl set resources deployment/my-deployment -c=app --limits=cpu=500m,memory=512Mi

# 修改 3:更新环境变量
kubectl set env deployment/my-deployment APP_VERSION=v2.0

# 恢复 Deployment,触发一次性滚动更新
kubectl rollout resume deployment/my-deployment
```

#### 在 YAML 中设置

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: my-deployment
spec:
  replicas: 5
  paused: true            # 创建时即暂停
  selector:
    matchLabels:
      app: myapp
  template:
    metadata:
      labels:
        app: myapp
    spec:
      containers:
      - name: app
        image: nginx:1.21
```

**应用场景**：
1. **金丝雀发布**：手动控制流量切换
2. **批量配置更新**：避免多次滚动更新
3. **问题排查**：暂停更新以观察问题

### 7.2 Rollback 回滚操作

**Deployment 自动保留历史版本**,支持快速回滚到之前的稳定版本。

#### 查看历史版本

```bash
# 查看 Deployment 的修订历史
kubectl rollout history deployment/my-deployment

# 输出示例:
REVISION  CHANGE-CAUSE
1         kubectl apply --filename=deploy-v1.yaml --record
2         kubectl set image deployment/my-deployment app=nginx:1.22 --record
3         kubectl set image deployment/my-deployment app=nginx:1.23 --record

# 查看特定版本的详细信息
kubectl rollout history deployment/my-deployment --revision=2
```

**记录变更原因**：
```bash
# 使用 --record 标志记录命令 (已弃用,但仍可用)
kubectl set image deployment/my-deployment app=nginx:1.22 --record

# v1.27+ 推荐使用 annotation
kubectl annotate deployment/my-deployment kubernetes.io/change-cause="升级到 nginx 1.22 修复安全漏洞"
```

#### 回滚到上一版本

```bash
# 回滚到上一个版本 (revision N-1)
kubectl rollout undo deployment/my-deployment

# 查看回滚状态
kubectl rollout status deployment/my-deployment
```

#### 回滚到指定版本

```bash
# 回滚到特定版本 (如 revision 2)
kubectl rollout undo deployment/my-deployment --to-revision=2
```

#### 回滚原理

```
当前状态:
ReplicaSet-v3 (10 个 Pod,当前活跃)
ReplicaSet-v2 (0 个 Pod,历史版本)
ReplicaSet-v1 (0 个 Pod,历史版本)

执行回滚到 v2:
1. Deployment Controller 修改 .spec.template 为 v2 的内容
2. 扩容 ReplicaSet-v2 到 10 个 Pod (滚动更新)
3. 缩容 ReplicaSet-v3 到 0 个 Pod
4. ReplicaSet-v3 被保留在历史中

回滚后状态:
ReplicaSet-v2 (10 个 Pod,当前活跃)   ← 恢复
ReplicaSet-v3 (0 个 Pod,历史版本)   ← 成为新的历史版本
ReplicaSet-v1 (0 个 Pod,历史版本)
```

### 7.3 Revision History 版本历史

#### 历史版本数量控制

```yaml
spec:
  revisionHistoryLimit: 5  # 保留最近 5 个 ReplicaSet (默认 10)
```

**说明**：
- 历史 ReplicaSet 的 `replicas=0`,但定义被保留
- 设置为 `0` 会禁用回滚功能
- 每个 ReplicaSet 约占用 1-2 KB etcd 存储空间

#### 版本 Hash 生成

每个版本的 ReplicaSet 名称包含 **pod-template-hash**:

```bash
$ kubectl get rs
NAME                          DESIRED   CURRENT   READY   AGE
my-deployment-5d4b7c9f8d      10        10        10      5m   # 当前版本
my-deployment-7c8a9b6d5f      0         0         0       15m  # 历史版本
my-deployment-9f3e2d1c4a      0         0         0       30m  # 历史版本
```

**Hash 计算逻辑**：
```go
// pod-template-hash 由 PodTemplate 内容 (不含 labels) 的 hash 生成
hash := ComputeHash(&deployment.Spec.Template, deployment.Status.CollisionCount)
```

#### 清理历史版本

```bash
# 手动删除旧的 ReplicaSet
kubectl delete rs my-deployment-9f3e2d1c4a

# 或调整 revisionHistoryLimit 后,系统自动清理
kubectl patch deployment my-deployment -p '{"spec":{"revisionHistoryLimit":2}}'
```

---

## 8. 内部原理

### 8.1 Deployment Controller 控制循环

**Deployment Controller** 运行在 `kube-controller-manager` 中,负责协调 Deployment 的实际状态与期望状态。

#### 控制循环逻辑

```
┌─────────────────────────────────────────────────────────┐
│  Deployment Controller (持续运行)                       │
└───────────────┬─────────────────────────────────────────┘
                │
                ▼
      ┌─────────────────────┐
      │  监听 Deployment 事件 │
      │  (创建/更新/删除)     │
      └──────────┬────────────┘
                 │
                 ▼
      ┌─────────────────────┐
      │  读取期望状态        │
      │  (Deployment.spec)   │
      └──────────┬────────────┘
                 │
                 ▼
      ┌─────────────────────┐
      │  查询当前 ReplicaSet │
      │  和 Pod 状态         │
      └──────────┬────────────┘
                 │
                 ▼
      ┌─────────────────────────────────────┐
      │  对比期望状态 vs 当前状态            │
      └──────────┬──────────────────────────┘
                 │
        ┌────────┴─────────┐
        │                  │
        ▼                  ▼
 ┌────────────┐      ┌────────────┐
 │  状态一致   │      │  状态不一致 │
 │  (无操作)   │      │  (执行调谐) │
 └────────────┘      └──────┬─────┘
                            │
          ┌─────────────────┴──────────────────┐
          │                                    │
          ▼                                    ▼
  ┌──────────────────┐              ┌──────────────────┐
  │  创建新 ReplicaSet│              │  滚动更新现有    │
  │  (新 Deployment)  │              │  ReplicaSet      │
  └──────────────────┘              └──────────────────┘
          │                                    │
          └──────────────┬─────────────────────┘
                         │
                         ▼
              ┌──────────────────────┐
              │  更新 Deployment     │
              │  .status 字段        │
              └──────────────────────┘
                         │
                         └──────┐
                                │
                                ▼
                         下一次控制循环
```

#### 调谐 (Reconcile) 步骤

```go
// 伪代码:Deployment Controller 核心逻辑
func (dc *DeploymentController) syncDeployment(d *Deployment) error {
    // 1. 获取所有关联的 ReplicaSet
    allRS := dc.getReplicaSetsForDeployment(d)
    
    // 2. 按创建时间排序,找出最新的 ReplicaSet
    newRS := FindNewReplicaSet(d, allRS)
    
    // 3. 判断是否需要创建新 ReplicaSet
    if newRS == nil || d.Spec.Template != newRS.Spec.Template {
        // Pod 模板变化,创建新 ReplicaSet
        newRS = dc.createReplicaSet(d)
    }
    
    // 4. 执行扩缩容或滚动更新
    switch d.Spec.Strategy.Type {
    case RollingUpdate:
        return dc.rolloutRolling(d, newRS, allRS)
    case Recreate:
        return dc.rolloutRecreate(d, newRS, allRS)
    }
    
    // 5. 清理多余的历史 ReplicaSet
    dc.cleanupOldReplicaSets(allRS, d.Spec.RevisionHistoryLimit)
    
    // 6. 更新 Deployment 状态
    dc.updateDeploymentStatus(d)
    
    return nil
}
```

### 8.2 Rolling Update 算法

#### 滚动更新流程

```yaml
# 初始状态:
Deployment:
  replicas: 10
  strategy:
    maxSurge: 2
    maxUnavailable: 1

ReplicaSet-old:
  replicas: 10 (全部 Ready)

ReplicaSet-new:
  replicas: 0
```

**更新步骤**：

```
阶段 1: 计算扩缩容数量
  maxReplicaCount = 10 + 2 = 12        # replicas + maxSurge
  minAvailableCount = 10 - 1 = 9       # replicas - maxUnavailable
  
  newRS 可扩容数量 = min(12 - 10, 2) = 2
  oldRS 可缩容数量 = max(10 - 9, 0) = 1

阶段 2: 扩容新 ReplicaSet
  newRS.replicas = 0 + 2 = 2
  → 创建 2 个新 Pod
  → 等待 Pod Ready

阶段 3: 缩容旧 ReplicaSet
  oldRS.replicas = 10 - 1 = 9
  → 删除 1 个旧 Pod
  
当前状态: 9 个旧 Pod + 2 个新 Pod = 11 个 Pod (符合 maxSurge)

阶段 4: 重复上述过程
  循环执行扩容新 RS、缩容旧 RS
  直到: newRS.replicas = 10, oldRS.replicas = 0

最终状态:
  ReplicaSet-old: replicas=0
  ReplicaSet-new: replicas=10 (全部 Ready)
```

#### 关键判断条件

```go
// 伪代码:滚动更新逻辑
func (dc *DeploymentController) rolloutRolling(d *Deployment, newRS *ReplicaSet, oldRSs []*ReplicaSet) error {
    // 计算最大/最小副本数
    maxReplicaCount := d.Spec.Replicas + MaxSurge(d)
    minAvailable := d.Spec.Replicas - MaxUnavailable(d)
    
    // 获取当前总副本数和可用副本数
    allPodsCount := GetReplicaCountForReplicaSets(append(oldRSs, newRS))
    availablePodsCount := GetAvailableReplicaCountForReplicaSets(append(oldRSs, newRS))
    
    // 1. 扩容新 ReplicaSet (如果未达到 maxReplicaCount)
    if allPodsCount < maxReplicaCount {
        scaleUpCount := min(maxReplicaCount - allPodsCount, maxReplicaCount - newRS.Spec.Replicas)
        dc.scaleReplicaSet(newRS, newRS.Spec.Replicas + scaleUpCount)
        return nil  // 等待下一次调谐
    }
    
    // 2. 缩容旧 ReplicaSet (如果新 Pod 已就绪)
    if newRS.Status.AvailableReplicas < d.Spec.Replicas {
        return nil  // 等待新 Pod 就绪
    }
    
    // 计算可缩容数量 (确保不低于 minAvailable)
    scaleDownCount := allPodsCount - d.Spec.Replicas
    if availablePodsCount - scaleDownCount < minAvailable {
        scaleDownCount = availablePodsCount - minAvailable
    }
    
    // 按比例缩容多个旧 ReplicaSet
    scaledDownCount := dc.scaleDownOldReplicaSets(oldRSs, scaleDownCount)
    
    return nil
}
```

### 8.3 Revision Hash 机制

**pod-template-hash** 用于唯一标识 Pod 模板版本,避免 ReplicaSet 名称冲突。

#### Hash 计算

```go
// Kubernetes 源码:计算 hash
import "hash/fnv"

func ComputeHash(template *PodTemplateSpec, collisionCount *int32) string {
    // 1. 深拷贝模板,移除 labels 中的 pod-template-hash
    templateCopy := template.DeepCopy()
    delete(templateCopy.Labels, "pod-template-hash")
    
    // 2. 序列化为 JSON
    hasher := fnv.New32a()
    DeepHashObject(hasher, templateCopy)
    
    // 3. 加上碰撞计数 (如果存在)
    if collisionCount != nil {
        collisionCountBytes := []byte(strconv.FormatInt(int64(*collisionCount), 10))
        hasher.Write(collisionCountBytes)
    }
    
    // 4. 返回 hash 字符串 (10 位)
    return rand.SafeEncodeString(fmt.Sprint(hasher.Sum32()))
}
```

#### Hash 冲突处理

```yaml
# 极少见的情况:两个不同的 PodTemplate 生成相同 hash
status:
  collisionCount: 1  # 碰撞计数递增,重新计算 hash
```

#### ReplicaSet 命名规则

```
格式: <deployment-name>-<pod-template-hash>

示例:
nginx-deployment-5d4b7c9f8d
│                │
│                └─ pod-template-hash (10 位随机字符串)
└─ Deployment 名称
```

### 8.4 ProgressDeadlineSeconds 超时机制

**作用**：防止滚动更新无限期挂起。

```yaml
spec:
  progressDeadlineSeconds: 600  # 默认 600 秒 (10 分钟)
```

#### 超时判断逻辑

```go
// 伪代码:进度超时检测
func (dc *DeploymentController) checkProgressDeadline(d *Deployment) {
    // 获取最后一次进度更新时间
    progressingCondition := GetDeploymentCondition(d.Status, DeploymentProgressing)
    if progressingCondition == nil {
        return
    }
    
    // 计算是否超时
    timeoutDuration := time.Duration(d.Spec.ProgressDeadlineSeconds) * time.Second
    if time.Since(progressingCondition.LastUpdateTime) > timeoutDuration {
        // 标记为失败
        SetDeploymentCondition(&d.Status, DeploymentCondition{
            Type:   DeploymentProgressing,
            Status: ConditionFalse,
            Reason: "ProgressDeadlineExceeded",
            Message: "Deployment exceeded its progress deadline",
        })
    }
}
```

#### 状态示例

```bash
$ kubectl describe deployment my-deployment

Conditions:
  Type           Status  Reason
  ----           ------  ------
  Available      True    MinimumReplicasAvailable
  Progressing    False   ProgressDeadlineExceeded  # 超时失败
  
Events:
  Warning  ProgressDeadlineExceeded  5s  deployment-controller  
    Deployment "my-deployment" has timed out progressing.
```

**触发超时的常见原因**：
1. **镜像拉取失败**：ImagePullBackOff
2. **资源不足**：节点无足够 CPU/内存
3. **健康检查失败**：Liveness/Readiness Probe 持续失败
4. **Pod 调度失败**：节点亲和性/污点容忍不满足

---

## 9. ReplicaSet 详解

### 9.1 API 信息

| 字段 | 值 |
|------|-----|
| **API Group** | apps |
| **API Version** | apps/v1 |
| **Kind** | ReplicaSet |
| **Scope** | Namespaced |
| **简写** | rs |
| **kubectl 命令** | `kubectl get rs` |

### 9.2 字段规格表

| 字段路径 | 类型 | 必填 | 默认值 | 说明 |
|---------|------|------|-------|------|
| `spec.replicas` | int32 | ❌ | 1 | 期望的 Pod 副本数量 |
| `spec.selector` | LabelSelector | ✅ | - | Pod 选择器 (不可变) |
| `spec.template` | PodTemplateSpec | ✅ | - | Pod 模板 |
| `spec.minReadySeconds` | int32 | ❌ | 0 | Pod 就绪后等待时间 |

**与 Deployment 的区别**：
- ❌ 无 `strategy` 字段 (不支持滚动更新)
- ❌ 无 `revisionHistoryLimit` (不维护版本历史)
- ❌ 无 `paused` (不支持暂停)

### 9.3 配置示例

```yaml
apiVersion: apps/v1
kind: ReplicaSet
metadata:
  name: frontend-rs
  labels:
    app: frontend
    tier: web
spec:
  # 副本数量
  replicas: 3
  
  # Pod 选择器 (创建后不可变)
  selector:
    matchLabels:
      app: frontend
      tier: web
  
  # Pod 模板
  template:
    metadata:
      labels:
        app: frontend
        tier: web
    spec:
      containers:
      - name: nginx
        image: nginx:1.21
        ports:
        - containerPort: 80
        resources:
          requests:
            cpu: 100m
            memory: 128Mi
          limits:
            cpu: 200m
            memory: 256Mi
```

**扩缩容操作**：

```bash
# 创建 ReplicaSet
kubectl apply -f frontend-rs.yaml

# 查看状态
kubectl get rs frontend-rs
kubectl get pods -l app=frontend

# 扩容到 5 个副本
kubectl scale rs frontend-rs --replicas=5

# 删除 ReplicaSet (会删除所有 Pod)
kubectl delete rs frontend-rs

# 删除 ReplicaSet 但保留 Pod (orphan 模式)
kubectl delete rs frontend-rs --cascade=orphan
```

### 9.4 为什么很少直接使用

**ReplicaSet 的局限性**：

| 功能 | ReplicaSet | Deployment | 影响 |
|------|-----------|-----------|------|
| **声明式更新** | ❌ | ✅ | 镜像更新需要手动操作 |
| **滚动更新** | ❌ | ✅ | 更新时所有 Pod 同时替换,导致服务中断 |
| **回滚机制** | ❌ | ✅ | 无法快速恢复到上一版本 |
| **版本历史** | ❌ | ✅ | 无法追溯配置变更历史 |
| **暂停/恢复** | ❌ | ✅ | 无法实现金丝雀部署 |

**手动更新 ReplicaSet 的问题**：

```bash
# 场景:需要更新镜像版本
# ❌ 错误做法:直接修改 ReplicaSet
kubectl set image rs/frontend-rs nginx=nginx:1.22

# 问题:现有 Pod 不会自动更新!需要手动删除
kubectl delete pods -l app=frontend  # 服务中断!

# ✅ 正确做法:使用 Deployment
kubectl set image deployment/frontend nginx=nginx:1.22
# Deployment 自动执行滚动更新,零宕机
```

**推荐使用场景**：

| 场景 | 推荐 | 理由 |
|------|-----|------|
| **生产环境** | Deployment | 需要滚动更新和回滚 |
| **开发测试** | Deployment | 保持一致性,避免环境差异 |
| **学习 K8s** | Deployment | 更符合实际使用场景 |
| **极简场景** | ReplicaSet | 仅需要副本控制,不更新 (极少见) |
| **被其他控制器管理** | ReplicaSet | 如 Deployment 内部使用 |

**总结**：
> ⚠️ **在绝大多数情况下,应该使用 Deployment 而非直接使用 ReplicaSet**。ReplicaSet 主要作为 Deployment 的底层实现,不建议用户直接操作。

---

## 10. ReplicationController(Legacy)

### 10.1 简要概述

**ReplicationController (RC)** 是 Kubernetes 早期的副本控制器,已被 **ReplicaSet** 取代。

| 特性 | ReplicationController | ReplicaSet | Deployment |
|------|---------------------|-----------|-----------|
| **API 版本** | v1 | apps/v1 | apps/v1 |
| **选择器** | 仅支持等值匹配 | 支持集合匹配 | 支持集合匹配 |
| **状态** | ⚠️ 已过时 | ✅ 稳定 | ✅ 推荐 |
| **滚动更新** | ❌ | ❌ | ✅ |

**关键差异**：

```yaml
# ReplicationController (旧)
apiVersion: v1
kind: ReplicationController
spec:
  replicas: 3
  selector:              # 仅支持简单键值对匹配
    app: frontend
    tier: web
  template:
    metadata:
      labels:
        app: frontend
        tier: web
    spec:
      containers:
      - name: nginx
        image: nginx:1.21

---
# ReplicaSet (新)
apiVersion: apps/v1
kind: ReplicaSet
spec:
  replicas: 3
  selector:
    matchLabels:         # 支持精确匹配
      app: frontend
    matchExpressions:    # 支持表达式匹配 (RC 不支持)
    - key: tier
      operator: In
      values:
      - web
      - frontend
  template:
    metadata:
      labels:
        app: frontend
        tier: web
    spec:
      containers:
      - name: nginx
        image: nginx:1.21
```

### 10.2 迁移指南

#### 从 RC 迁移到 Deployment

**步骤 1：导出现有 RC 配置**

```bash
# 导出 ReplicationController
kubectl get rc my-app-rc -o yaml > my-app-rc.yaml
```

**步骤 2：转换为 Deployment**

```yaml
# 手动编辑 my-app-deployment.yaml
apiVersion: apps/v1                    # 修改 apiVersion
kind: Deployment                        # 修改 kind
metadata:
  name: my-app                          # 可以保持或更改名称
spec:
  replicas: 3
  selector:
    matchLabels:                        # 调整 selector 格式
      app: my-app
  template:                             # template 部分保持不变
    metadata:
      labels:
        app: my-app
    spec:
      containers:
      - name: app
        image: myapp:v1.0
```

**步骤 3：零宕机迁移**

```bash
# 1. 创建 Deployment (不会立即删除 RC 管理的 Pod)
kubectl apply -f my-app-deployment.yaml

# 2. 验证 Deployment 正常运行
kubectl get deployment my-app
kubectl get pods -l app=my-app  # 会看到 RC 和 Deployment 的 Pod 共存

# 3. 缩容 ReplicationController 到 0
kubectl scale rc my-app-rc --replicas=0

# 4. 观察 Deployment 是否正常
kubectl rollout status deployment/my-app

# 5. 删除 ReplicationController
kubectl delete rc my-app-rc
```

**自动迁移脚本**：

```bash
#!/bin/bash
# rc-to-deployment.sh

RC_NAME=$1
NAMESPACE=${2:-default}

# 导出 RC 配置
kubectl get rc $RC_NAME -n $NAMESPACE -o json > /tmp/rc.json

# 使用 jq 转换为 Deployment
cat /tmp/rc.json | jq '
  .apiVersion = "apps/v1" |
  .kind = "Deployment" |
  .spec.selector = { matchLabels: .spec.selector } |
  del(.spec.template.metadata.labels["pod-template-hash"]) |
  del(.status)
' > /tmp/deployment.yaml

# 应用 Deployment
kubectl apply -f /tmp/deployment.yaml -n $NAMESPACE

# 缩容 RC
kubectl scale rc $RC_NAME --replicas=0 -n $NAMESPACE

echo "迁移完成,请验证后手动删除 RC: kubectl delete rc $RC_NAME -n $NAMESPACE"
```

---

## 11. 版本兼容性矩阵

### Deployment API 版本

| API 版本 | Kubernetes 版本 | 状态 | 关键变更 |
|---------|----------------|------|---------|
| `extensions/v1beta1` | v1.6 - v1.16 | ❌ v1.16 移除 | 首次引入 Deployment |
| `apps/v1beta1` | v1.7 - v1.16 | ❌ v1.16 移除 | 迁移到 apps API 组 |
| `apps/v1beta2` | v1.8 - v1.16 | ❌ v1.16 移除 | 添加 `.spec.selector` 不可变规则 |
| `apps/v1` | v1.9+ | ✅ 稳定 | **当前推荐版本** |

### 功能特性兼容性

| 特性 | 引入版本 | 稳定版本 | 说明 |
|------|---------|---------|------|
| **基础 Deployment** | v1.2 (RC) → v1.9 (Deployment) | v1.9 | 滚动更新、回滚 |
| **maxSurge/maxUnavailable 百分比** | v1.6 | v1.9 | 支持百分比值 |
| **progressDeadlineSeconds** | v1.7 | v1.9 | 更新超时机制 |
| **Recreate 策略** | v1.9 | v1.9 | 重建式更新 |
| **Startup Probe** | v1.16 (alpha) | v1.20 | 慢启动容器保护 |
| **Pod Topology Spread** | v1.18 (beta) | v1.19 | 拓扑分布约束 |
| **minReadySeconds** | v1.9 | v1.9 | 就绪后等待时间 |
| **Server-side Apply** | v1.16 (beta) | v1.22 | 声明式配置合并 |

### 版本迁移示例

#### 从 apps/v1beta2 迁移到 apps/v1

```yaml
# apps/v1beta2 (旧)
apiVersion: apps/v1beta2
kind: Deployment
spec:
  selector:
    matchLabels:
      app: myapp
  template:
    metadata:
      labels:
        app: myapp
    spec:
      containers:
      - name: app
        image: myapp:v1.0

---
# apps/v1 (新)
apiVersion: apps/v1              # 仅需修改此行
kind: Deployment
spec:
  selector:                      # selector 格式保持不变
    matchLabels:
      app: myapp
  template:
    metadata:
      labels:
        app: myapp
    spec:
      containers:
      - name: app
        image: myapp:v1.0
```

**迁移检查工具**：

```bash
# 检查集群中是否有旧版本 API
kubectl get deployments.v1beta1.apps --all-namespaces
kubectl get deployments.v1beta2.apps --all-namespaces

# 使用 kubectl convert 转换 (需要安装 convert 插件)
kubectl convert -f old-deployment.yaml --output-version apps/v1
```

---

## 12. 生产最佳实践

### 副本数量规划

| 场景 | 推荐副本数 | 理由 |
|------|-----------|------|
| **开发环境** | 1-2 | 节省资源 |
| **测试环境** | 2-3 | 验证多副本行为 |
| **生产环境** | ≥3 | 高可用性,至少跨 2 个可用区 |
| **高流量服务** | ≥10 | 负载均衡,单个 Pod 故障影响小 |
| **单例应用** | 1 | 使用 StatefulSet 或 Deployment + 分布式锁 |

**计算公式**：

```
推荐副本数 = ceil(
    (峰值 QPS * 单请求响应时间) / (单 Pod QPS * 容量规划系数)
)

容量规划系数通常为 0.6-0.7 (留 30-40% 余量)

示例:
  峰值 QPS = 10000
  单请求响应时间 = 50ms
  单 Pod QPS = 500
  
  副本数 = ceil((10000 * 0.05) / (500 * 0.7))
        = ceil(500 / 350)
        = 2 个 Pod (实际应至少 3 个以保证高可用)
```

### 滚动更新策略

| 场景 | maxSurge | maxUnavailable | 说明 |
|------|----------|---------------|------|
| **生产环境标准** | 25% | 25% | Kubernetes 默认值,平衡速度和稳定性 |
| **零宕机更新** | 1 | 0 | 最保守,适合关键服务 |
| **快速更新 (非关键服务)** | 100% | 50% | 更新速度优先 |
| **资源受限环境** | 0 | 1 | 先删除旧 Pod 释放资源 |
| **蓝绿部署** | 100% | 0 | 创建全部新 Pod 后切换 |

### 健康检查配置

```yaml
# 推荐的探针配置模板
containers:
- name: app
  # 1. Startup Probe (v1.20+):保护慢启动容器
  startupProbe:
    httpGet:
      path: /startup         # 专用启动检查端点
      port: 8080
    initialDelaySeconds: 0   # 立即开始检查
    periodSeconds: 5         # 每 5 秒检查一次
    timeoutSeconds: 3
    failureThreshold: 30     # 最多等待 150 秒 (5*30)
    # 启动期间失败不会触发重启,直到 failureThreshold 耗尽
  
  # 2. Liveness Probe:检测死锁/僵尸进程
  livenessProbe:
    httpGet:
      path: /healthz         # 轻量级健康检查
      port: 8080
    initialDelaySeconds: 60  # 给予充分启动时间
    periodSeconds: 10        # 每 10 秒检查一次
    timeoutSeconds: 5
    failureThreshold: 3      # 连续失败 3 次重启容器
  
  # 3. Readiness Probe:控制流量接入
  readinessProbe:
    httpGet:
      path: /ready           # 检查依赖服务可用性
      port: 8080
    initialDelaySeconds: 10
    periodSeconds: 5         # 频繁检查以快速响应
    timeoutSeconds: 3
    failureThreshold: 2      # 失败 2 次移出 Service
    successThreshold: 1      # 成功 1 次恢复流量
```

**探针端点实现示例 (Go)**：

```go
package main

import (
    "net/http"
    "sync/atomic"
    "time"
)

var (
    startupComplete uint32  // 启动是否完成
    isHealthy       uint32  // 服务是否健康
    isReady         uint32  // 是否就绪接收流量
)

func main() {
    // 模拟启动流程
    go func() {
        time.Sleep(20 * time.Second)  // 模拟慢启动
        atomic.StoreUint32(&startupComplete, 1)
        atomic.StoreUint32(&isHealthy, 1)
        atomic.StoreUint32(&isReady, 1)
    }()
    
    // Startup Probe
    http.HandleFunc("/startup", func(w http.ResponseWriter, r *http.Request) {
        if atomic.LoadUint32(&startupComplete) == 1 {
            w.WriteHeader(http.StatusOK)
            w.Write([]byte("Started"))
        } else {
            w.WriteHeader(http.StatusServiceUnavailable)
            w.Write([]byte("Starting..."))
        }
    })
    
    // Liveness Probe
    http.HandleFunc("/healthz", func(w http.ResponseWriter, r *http.Request) {
        if atomic.LoadUint32(&isHealthy) == 1 {
            w.WriteHeader(http.StatusOK)
            w.Write([]byte("OK"))
        } else {
            w.WriteHeader(http.StatusServiceUnavailable)
        }
    })
    
    // Readiness Probe
    http.HandleFunc("/ready", func(w http.ResponseWriter, r *http.Request) {
        // 检查依赖服务 (如数据库连接)
        if atomic.LoadUint32(&isReady) == 1 && checkDependencies() {
            w.WriteHeader(http.StatusOK)
            w.Write([]byte("Ready"))
        } else {
            w.WriteHeader(http.StatusServiceUnavailable)
        }
    })
    
    http.ListenAndServe(":8080", nil)
}

func checkDependencies() bool {
    // 检查数据库、Redis、外部 API 等
    return true
}
```

### 资源配额设置

```yaml
# 生产环境资源配额指南
resources:
  requests:
    cpu: "500m"        # 根据实际负载的 P50 值设置
    memory: "512Mi"    # 根据应用稳定运行的内存需求
  limits:
    cpu: "2000m"       # 限制为 requests 的 2-4 倍
    memory: "2Gi"      # 限制为 requests 的 2-4 倍 (避免 OOM)

# ⚠️ 注意事项:
# 1. requests 不足 → 调度失败 (Pending)
# 2. limits 过高 → 浪费资源
# 3. limits 过低 → 频繁被杀 (OOMKilled/Throttling)
# 4. memory limits 应合理,避免 OOM Killer 触发
```

**推荐配置模板**：

| 应用类型 | CPU requests | CPU limits | Memory requests | Memory limits |
|---------|-------------|-----------|----------------|--------------|
| **轻量级 API** | 100m | 500m | 128Mi | 512Mi |
| **标准 Web 服务** | 500m | 2000m | 512Mi | 2Gi |
| **数据处理** | 1000m | 4000m | 2Gi | 8Gi |
| **Java 应用** | 500m | 2000m | 1Gi | 4Gi (设置 -Xmx 为 limits 的 75%) |

### 标签和注解规范

```yaml
metadata:
  # 标签:用于选择和组织
  labels:
    # 推荐标签 (kubernetes.io 官方规范)
    app.kubernetes.io/name: mysql                    # 应用名称
    app.kubernetes.io/version: "5.7.44"              # 应用版本
    app.kubernetes.io/component: database            # 组件类型
    app.kubernetes.io/part-of: wordpress             # 所属系统
    app.kubernetes.io/managed-by: helm               # 管理工具
    
    # 自定义标签
    environment: production                          # 环境
    team: platform                                   # 负责团队
    cost-center: engineering                         # 成本中心
    
  # 注解:存储非识别信息
  annotations:
    # 变更记录
    kubernetes.io/change-cause: "升级到 MySQL 5.7.44"
    deployed-by: "platform-team@company.com"
    deployment-timestamp: "2026-02-10T10:30:00Z"
    
    # 监控和告警
    prometheus.io/scrape: "true"
    prometheus.io/path: "/metrics"
    prometheus.io/port: "9090"
    
    # 日志和追踪
    fluentd.io/parser: "json"
    jaeger.io/inject: "true"
    
    # 文档链接
    documentation: "https://wiki.company.com/mysql-deployment"
    runbook: "https://runbook.company.com/mysql-troubleshooting"
    
    # 审计信息
    approved-by: "john.doe@company.com"
    ticket: "JIRA-12345"
```

### 版本管理

```yaml
# 1. 镜像标签规范
spec:
  template:
    spec:
      containers:
      - name: app
        # ❌ 避免使用:
        # image: myapp:latest        # 不确定版本,难以回滚
        # image: myapp:v1            # 语义化版本不足
        
        # ✅ 推荐使用:
        image: myapp:v1.2.3          # 完整语义化版本
        # 或
        image: myapp:v1.2.3-sha-a3f5b2c  # 版本 + Git commit hash

# 2. 记录变更原因
metadata:
  annotations:
    kubernetes.io/change-cause: |
      升级到 v1.2.3:
      - 修复内存泄漏问题 (#1234)
      - 优化数据库查询性能 (#1235)
      - 安全更新: CVE-2026-1234

# 3. 使用 revisionHistoryLimit
spec:
  revisionHistoryLimit: 5     # 生产环境保留 5-10 个版本
```

### 安全加固

```yaml
spec:
  template:
    spec:
      # 1. 使用非特权服务账号
      serviceAccountName: app-sa
      automountServiceAccountToken: false  # 如不需要访问 API Server
      
      # 2. Pod 安全上下文
      securityContext:
        runAsNonRoot: true                 # 强制非 root 运行
        runAsUser: 1000                    # 指定 UID
        runAsGroup: 3000                   # 指定 GID
        fsGroup: 2000                      # 文件系统组
        seccompProfile:
          type: RuntimeDefault             # seccomp 配置
      
      containers:
      - name: app
        # 3. 容器安全上下文
        securityContext:
          allowPrivilegeEscalation: false  # 禁止权限提升
          readOnlyRootFilesystem: true     # 只读根文件系统
          runAsNonRoot: true
          runAsUser: 1000
          capabilities:
            drop:
            - ALL                          # 删除所有 Linux capabilities
            add:
            - NET_BIND_SERVICE             # 仅添加必需的 capability
        
        # 4. 挂载可写目录
        volumeMounts:
        - name: tmp-volume
          mountPath: /tmp                  # 为需要写入的目录挂载 emptyDir
        - name: cache-volume
          mountPath: /app/cache
      
      volumes:
      - name: tmp-volume
        emptyDir: {}
      - name: cache-volume
        emptyDir:
          sizeLimit: 1Gi                   # 限制临时卷大小
```

---

## 13. 常见问题 FAQ

### Q1: Deployment 更新卡住 (Progressing 状态)

**症状**：

```bash
$ kubectl rollout status deployment/myapp
Waiting for deployment "myapp" rollout to finish: 2 out of 5 new replicas have been updated...
# 长时间无进展
```

**排查步骤**：

```bash
# 1. 查看 Deployment 事件
kubectl describe deployment myapp

# 2. 查看 ReplicaSet 状态
kubectl get rs -l app=myapp
kubectl describe rs <new-replicaset-name>

# 3. 查看 Pod 状态
kubectl get pods -l app=myapp
kubectl describe pod <pending-pod-name>

# 4. 查看 Pod 日志
kubectl logs <pod-name> -c <container-name>
```

**常见原因及解决方案**：

| 原因 | 症状 | 解决方案 |
|------|------|---------|
| **镜像拉取失败** | `ImagePullBackOff` | 检查镜像名称、tag、registry 凭证 |
| **资源不足** | `Pending` (FailedScheduling) | 增加节点或降低资源 requests |
| **健康检查失败** | `CrashLoopBackOff` | 调整探针参数或修复应用问题 |
| **节点亲和性不满足** | `Pending` (FailedScheduling) | 修改亲和性规则或为节点添加标签 |
| **PVC 绑定失败** | `Pending` (FailedMount) | 检查 StorageClass 和 PV 可用性 |

**快速回滚**：

```bash
# 如果新版本有问题,立即回滚
kubectl rollout undo deployment/myapp
```

### Q2: 如何实现零宕机更新？

**配置要点**：

```yaml
spec:
  replicas: 3                 # 至少 2 个副本
  strategy:
    type: RollingUpdate
    rollingUpdate:
      maxSurge: 1             # 允许额外 1 个 Pod
      maxUnavailable: 0       # 不允许不可用 Pod
  minReadySeconds: 30         # 新 Pod 就绪后等待 30 秒
  template:
    spec:
      containers:
      - name: app
        # 1. 正确配置健康检查
        readinessProbe:
          httpGet:
            path: /ready
            port: 8080
          initialDelaySeconds: 10
          periodSeconds: 5
          failureThreshold: 2
        
        # 2. 优雅关闭
        lifecycle:
          preStop:
            exec:
              command: ["/bin/sh", "-c", "sleep 15"]  # 等待负载均衡器移除
        
        # 3. 充足的终止时间
      terminationGracePeriodSeconds: 60
```

**验证零宕机**：

```bash
# 在更新过程中持续发送请求
while true; do
  curl -s http://myapp.example.com/healthz || echo "FAILED"
  sleep 0.5
done

# 同时执行滚动更新
kubectl set image deployment/myapp app=myapp:v2.0
```

### Q3: Deployment 与 StatefulSet 如何选择？

| 维度 | Deployment | StatefulSet |
|------|-----------|------------|
| **Pod 名称** | 随机 hash (myapp-5d4b7c9f8d-x7k2m) | 有序序号 (myapp-0, myapp-1, myapp-2) |
| **网络标识** | 随机 IP,无固定 DNS | 固定 DNS (myapp-0.myapp-headless.ns.svc.cluster.local) |
| **存储** | 共享存储或无状态 | 每个 Pod 独立 PVC,重启后保留 |
| **启停顺序** | 并行,无序 | 串行,有序 (0→1→2 启动, 2→1→0 停止) |
| **更新策略** | 滚动更新 (默认) | 滚动更新或 OnDelete |
| **适用场景** | Web 服务、API、无状态应用 | 数据库、消息队列、ZooKeeper、Kafka |

**示例**：

```yaml
# 使用 Deployment (无状态 Web 应用)
apiVersion: apps/v1
kind: Deployment
metadata:
  name: nginx-web
spec:
  replicas: 5
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
        image: nginx:1.21
        ports:
        - containerPort: 80

---
# 使用 StatefulSet (MySQL 主从复制)
apiVersion: apps/v1
kind: StatefulSet
metadata:
  name: mysql
spec:
  serviceName: mysql-headless    # 必须指定 Headless Service
  replicas: 3
  selector:
    matchLabels:
      app: mysql
  template:
    metadata:
      labels:
        app: mysql
    spec:
      containers:
      - name: mysql
        image: mysql:5.7
        env:
        - name: MYSQL_ROOT_PASSWORD
          value: password
  volumeClaimTemplates:          # 每个 Pod 独立 PVC
  - metadata:
      name: data
    spec:
      accessModes: ["ReadWriteOnce"]
      resources:
        requests:
          storage: 10Gi
```

### Q4: 如何暂停自动更新 (Pause/Resume)？

**场景**：需要进行多次配置更改,避免每次修改都触发滚动更新。

```bash
# 1. 暂停 Deployment
kubectl rollout pause deployment/myapp

# 2. 进行多次修改
kubectl set image deployment/myapp app=myapp:v2.0
kubectl set env deployment/myapp APP_ENV=production
kubectl set resources deployment/myapp -c=app --limits=cpu=2,memory=2Gi

# 3. 恢复 Deployment (触发一次性滚动更新)
kubectl rollout resume deployment/myapp

# 4. 监控更新进度
kubectl rollout status deployment/myapp
```

### Q5: 如何查看和回滚到历史版本？

```bash
# 1. 查看历史版本
kubectl rollout history deployment/myapp
# 输出:
# REVISION  CHANGE-CAUSE
# 1         kubectl apply --filename=v1.yaml
# 2         kubectl set image deployment/myapp app=myapp:v2.0
# 3         kubectl set image deployment/myapp app=myapp:v3.0

# 2. 查看特定版本详情
kubectl rollout history deployment/myapp --revision=2

# 3. 回滚到上一版本
kubectl rollout undo deployment/myapp

# 4. 回滚到指定版本
kubectl rollout undo deployment/myapp --to-revision=2

# 5. 验证回滚结果
kubectl get pods -l app=myapp -o jsonpath='{.items[0].spec.containers[0].image}'
```

**记录变更原因** (方便追溯)：

```bash
# 方法 1: 使用 --record (已弃用但仍可用)
kubectl set image deployment/myapp app=myapp:v2.0 --record

# 方法 2: 使用 annotation (推荐)
kubectl annotate deployment/myapp kubernetes.io/change-cause="升级到 v2.0 修复安全漏洞 CVE-2026-1234"
```

### Q6: Deployment 更新后 Pod 数量不符合预期

**症状**：

```bash
$ kubectl get deployment myapp
NAME    READY   UP-TO-DATE   AVAILABLE   AGE
myapp   8/10    5            8           10m

# 期望 10 个 Pod,但实际只有 8 个可用
```

**排查步骤**：

```bash
# 1. 查看 ReplicaSet
kubectl get rs -l app=myapp
# 输出:
# NAME              DESIRED   CURRENT   READY   AGE
# myapp-new-abc123  10        10        8       5m   # 新版本
# myapp-old-def456  0         0         0       15m  # 旧版本

# 2. 查看 Pod 状态
kubectl get pods -l app=myapp
# 可能看到一些 Pod 处于 Pending 或 CrashLoopBackOff

# 3. 检查资源配额
kubectl describe deployment myapp | grep -A 5 "Resource Quotas"

# 4. 检查节点资源
kubectl top nodes
kubectl describe node <node-name> | grep -A 10 "Allocated resources"
```

**常见原因**：

1. **节点资源不足**：增加节点或降低资源 requests
2. **PodDisruptionBudget 限制**：检查 PDB 配置
3. **镜像拉取失败**：2 个 Pod 卡在 `ImagePullBackOff`
4. **健康检查失败**：Pod 启动但未通过 Readiness Probe

### Q7: 如何限制 Deployment 更新速度？

**场景**：希望更新过程更加保守,逐步验证新版本。

```yaml
spec:
  replicas: 100
  strategy:
    type: RollingUpdate
    rollingUpdate:
      maxSurge: 1             # 每次仅创建 1 个新 Pod
      maxUnavailable: 1       # 每次仅删除 1 个旧 Pod
  minReadySeconds: 60         # 新 Pod 就绪后等待 60 秒
  progressDeadlineSeconds: 3600  # 允许 1 小时完成更新
```

**效果**：
- 100 个 Pod 的更新大约需要 1 小时 40 分钟 (100 * 60 秒 / 60)
- 每次仅 2% 的 Pod 处于更新状态
- 便于及时发现问题并中断更新

---

## 14. 生产案例

### 14.1 蓝绿部署

**原理**：同时运行新旧两个版本,通过 Service 切换流量。

#### 架构图

```
                  ┌─────────────────────┐
                  │   Service (Label    │
                  │   Selector: version)│
                  └──────────┬───────────┘
                             │
         ┌───────────────────┴────────────────────┐
         │ 1. 初始状态: 流量指向 blue             │
         ▼                                        │
┌─────────────────────┐                          │
│ Deployment: blue    │                          │
│ replicas: 10        │                          │
│ version: blue       │                          │
│ image: app:v1.0     │                          │
└─────────────────────┘                          │
                                                  │
┌─────────────────────┐                          │
│ Deployment: green   │ ← 2. 创建 green 版本    │
│ replicas: 10        │    (流量仍在 blue)      │
│ version: green      │                          │
│ image: app:v2.0     │                          │
└─────────────────────┘                          │
                                                  │
         │ 3. 验证 green 正常后切换流量           │
         └───────────────────────────────────────┘
                             │
                             ▼
                  ┌─────────────────────┐
                  │   Service (Label    │
                  │   Selector: green)  │
                  └─────────────────────┘
```

#### 实施步骤

**步骤 1：部署 Blue 版本 (v1.0)**

```yaml
# blue-deployment.yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: myapp-blue
  labels:
    app: myapp
spec:
  replicas: 10
  selector:
    matchLabels:
      app: myapp
      version: blue
  template:
    metadata:
      labels:
        app: myapp
        version: blue
    spec:
      containers:
      - name: app
        image: myapp:v1.0
        ports:
        - containerPort: 8080

---
# service.yaml
apiVersion: v1
kind: Service
metadata:
  name: myapp-service
spec:
  selector:
    app: myapp
    version: blue      # 流量指向 blue 版本
  ports:
  - protocol: TCP
    port: 80
    targetPort: 8080
  type: LoadBalancer
```

```bash
kubectl apply -f blue-deployment.yaml
kubectl apply -f service.yaml
```

**步骤 2：部署 Green 版本 (v2.0)**

```yaml
# green-deployment.yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: myapp-green
  labels:
    app: myapp
spec:
  replicas: 10
  selector:
    matchLabels:
      app: myapp
      version: green
  template:
    metadata:
      labels:
        app: myapp
        version: green
    spec:
      containers:
      - name: app
        image: myapp:v2.0      # 新版本
        ports:
        - containerPort: 8080
```

```bash
# 部署 green 版本 (流量仍在 blue)
kubectl apply -f green-deployment.yaml

# 等待 green 版本就绪
kubectl rollout status deployment/myapp-green

# 验证 green 版本 (可以通过 Pod IP 访问)
kubectl get pods -l version=green -o wide
curl http://<green-pod-ip>:8080/healthz
```

**步骤 3：切换流量到 Green**

```bash
# 修改 Service selector
kubectl patch service myapp-service -p '{"spec":{"selector":{"version":"green"}}}'

# 验证流量切换
kubectl describe service myapp-service | grep Selector
# 输出: Selector: app=myapp,version=green
```

**步骤 4：验证和清理**

```bash
# 监控新版本运行状态
kubectl top pods -l version=green
kubectl logs -l version=green --tail=100

# 如果新版本正常运行,删除 blue 版本
kubectl delete deployment myapp-blue

# 如果新版本有问题,快速回滚到 blue
kubectl patch service myapp-service -p '{"spec":{"selector":{"version":"blue"}}}'
```

**优点**：
- ✅ **零宕机切换**：瞬时切换流量
- ✅ **快速回滚**：修改 Service selector 即可
- ✅ **充分验证**：新版本可完全启动后再切换

**缺点**：
- ❌ **资源占用翻倍**：需要同时运行两套环境
- ❌ **数据库兼容性**：需确保新旧版本兼容同一数据库 schema

### 14.2 金丝雀发布

**原理**：逐步将一部分流量切换到新版本,观察指标后再全量发布。

#### 架构图

```
                ┌─────────────────────┐
                │   Service           │
                │   (流量按比例分配)   │
                └──────────┬───────────┘
                           │
         ┌─────────────────┴─────────────────┐
         │                                    │
         │ 90% 流量                  10% 流量 │
         ▼                                    ▼
┌──────────────────┐              ┌──────────────────┐
│ Deployment:      │              │ Deployment:      │
│ stable           │              │ canary           │
│ replicas: 9      │              │ replicas: 1      │
│ image: app:v1.0  │              │ image: app:v2.0  │
└──────────────────┘              └──────────────────┘
```

#### 方式 1：基于副本数控制 (简单)

```yaml
# stable-deployment.yaml (v1.0 稳定版本)
apiVersion: apps/v1
kind: Deployment
metadata:
  name: myapp-stable
spec:
  replicas: 9                # 90% 流量
  selector:
    matchLabels:
      app: myapp
  template:
    metadata:
      labels:
        app: myapp
        track: stable
    spec:
      containers:
      - name: app
        image: myapp:v1.0
        ports:
        - containerPort: 8080

---
# canary-deployment.yaml (v2.0 金丝雀版本)
apiVersion: apps/v1
kind: Deployment
metadata:
  name: myapp-canary
spec:
  replicas: 1                # 10% 流量
  selector:
    matchLabels:
      app: myapp
  template:
    metadata:
      labels:
        app: myapp
        track: canary
    spec:
      containers:
      - name: app
        image: myapp:v2.0      # 新版本
        ports:
        - containerPort: 8080

---
# service.yaml (选择所有版本)
apiVersion: v1
kind: Service
metadata:
  name: myapp-service
spec:
  selector:
    app: myapp               # 同时选择 stable 和 canary
  ports:
  - protocol: TCP
    port: 80
    targetPort: 8080
```

**发布流程**：

```bash
# 1. 部署稳定版本
kubectl apply -f stable-deployment.yaml
kubectl apply -f service.yaml

# 2. 部署金丝雀版本 (10% 流量)
kubectl apply -f canary-deployment.yaml

# 3. 监控指标
# - 错误率: kubectl logs -l track=canary | grep ERROR
# - 响应时间: Prometheus 查询 p99 latency
# - 业务指标: 转化率、订单数等

# 4. 逐步增加金丝雀流量
kubectl scale deployment myapp-canary --replicas=2  # 20%
kubectl scale deployment myapp-stable --replicas=8  # 80%

# 观察一段时间...

kubectl scale deployment myapp-canary --replicas=5  # 50%
kubectl scale deployment myapp-stable --replicas=5  # 50%

# 5. 全量发布
kubectl scale deployment myapp-canary --replicas=10  # 100%
kubectl scale deployment myapp-stable --replicas=0   # 0%

# 6. 清理旧版本
kubectl delete deployment myapp-stable
kubectl patch deployment myapp-canary --patch '{"metadata":{"name":"myapp-stable"}}'
```

#### 方式 2：使用 Flagger (自动化)

**Flagger** 是 Weaveworks 开源的渐进式交付工具,支持自动化金丝雀发布。

```yaml
# 安装 Flagger (需要 Istio 或其他服务网格)
kubectl apply -k github.com/fluxcd/flagger//kustomize/istio

---
# canary.yaml
apiVersion: flagger.app/v1beta1
kind: Canary
metadata:
  name: myapp
spec:
  targetRef:
    apiVersion: apps/v1
    kind: Deployment
    name: myapp
  service:
    port: 80
  analysis:
    interval: 1m               # 每 1 分钟分析一次
    threshold: 5               # 5 次成功后进入下一阶段
    maxWeight: 50              # 最大金丝雀流量 50%
    stepWeight: 10             # 每次增加 10% 流量
    metrics:
    - name: request-success-rate
      thresholdRange:
        min: 99                # 成功率 ≥ 99%
      interval: 1m
    - name: request-duration
      thresholdRange:
        max: 500               # P99 延迟 ≤ 500ms
      interval: 1m
    webhooks:
    - name: load-test
      url: http://flagger-loadtester/
      timeout: 5s
      metadata:
        cmd: "hey -z 1m -q 10 -c 2 http://myapp-canary/"
```

**自动化流程**：

```
初始状态: 100% stable (v1.0)

阶段 1: 创建 canary (0% 流量)
  → Flagger 创建 myapp-canary Deployment

阶段 2: 发送 10% 流量到 canary
  → 持续 5 分钟 (5 * interval)
  → 检查成功率 ≥ 99%, P99 延迟 ≤ 500ms
  → 通过 → 进入下一阶段

阶段 3: 发送 20% 流量到 canary
  → 重复监控和验证

...

阶段 N: 发送 50% 流量到 canary
  → 验证通过 → 全量发布
  → stable 更新为 v2.0
  → canary 缩容为 0

如果任何阶段失败:
  → 自动回滚到 stable
  → 发送告警通知
```

### 14.3 带 PDB 的滚动更新

**PodDisruptionBudget (PDB)** 用于限制同时不可用的 Pod 数量,保护关键服务。

#### 场景

在进行节点维护或滚动更新时,防止过多 Pod 同时下线导致服务中断。

```yaml
# pdb.yaml
apiVersion: policy/v1
kind: PodDisruptionBudget
metadata:
  name: myapp-pdb
spec:
  minAvailable: 8            # 至少保持 8 个 Pod 可用
  # 或使用百分比:
  # minAvailable: 80%        # 至少 80% Pod 可用
  # maxUnavailable: 2        # 最多 2 个 Pod 不可用
  selector:
    matchLabels:
      app: myapp

---
# deployment.yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: myapp
spec:
  replicas: 10
  strategy:
    type: RollingUpdate
    rollingUpdate:
      maxSurge: 2
      maxUnavailable: 2      # 与 PDB 协同工作
  selector:
    matchLabels:
      app: myapp
  template:
    metadata:
      labels:
        app: myapp
    spec:
      containers:
      - name: app
        image: myapp:v1.0
        ports:
        - containerPort: 8080
        readinessProbe:
          httpGet:
            path: /ready
            port: 8080
          periodSeconds: 5
```

**PDB 工作原理**：

```
滚动更新时:

步骤 1: Deployment 尝试删除 2 个旧 Pod (maxUnavailable=2)
  → PDB 检查: 10 个 Pod - 2 = 8 个可用 (满足 minAvailable=8)
  → 允许删除

步骤 2: 新 Pod 启动并通过 Readiness Probe
  → 可用 Pod 数量恢复到 10 个

步骤 3: Deployment 再次尝试删除 2 个旧 Pod
  → PDB 检查通过 → 继续更新

节点驱逐时:

场景: kubectl drain node-1 (驱逐节点上的 4 个 myapp Pod)
  → PDB 检查: 10 - 4 = 6 个可用 (不满足 minAvailable=8)
  → 拒绝驱逐全部 4 个 Pod
  → 仅允许驱逐 2 个 Pod (保留 8 个可用)
  → 管理员需等待新 Pod 调度到其他节点后再驱逐剩余 Pod
```

**PDB 配置最佳实践**：

```yaml
# 场景 1: 高可用服务 (replicas=10)
apiVersion: policy/v1
kind: PodDisruptionBudget
metadata:
  name: critical-service-pdb
spec:
  minAvailable: 80%          # 至少 8 个 Pod 可用
  selector:
    matchLabels:
      app: critical-service
      tier: production

---
# 场景 2: 允许少量中断 (replicas=20)
apiVersion: policy/v1
kind: PodDisruptionBudget
metadata:
  name: standard-service-pdb
spec:
  maxUnavailable: 3          # 最多 3 个 Pod 不可用
  selector:
    matchLabels:
      app: standard-service

---
# 场景 3: 单例服务 (replicas=1)
apiVersion: policy/v1
kind: PodDisruptionBudget
metadata:
  name: singleton-pdb
spec:
  minAvailable: 1            # 必须保持 1 个 Pod 可用
  selector:
    matchLabels:
      app: singleton-service
# 注意: 此配置会阻止节点驱逐,需要手动干预
```

**验证 PDB 状态**：

```bash
# 查看 PDB 状态
kubectl get pdb myapp-pdb
# 输出:
# NAME         MIN AVAILABLE   MAX UNAVAILABLE   ALLOWED DISRUPTIONS   AGE
# myapp-pdb    8               N/A               2                     5m

# 查看详细信息
kubectl describe pdb myapp-pdb

# 测试驱逐 (需要足够的 ALLOWED DISRUPTIONS)
kubectl drain node-1 --ignore-daemonsets --delete-emptydir-data
```

---

## 15. 相关资源

### 官方文档

- [Kubernetes Deployment 官方文档](https://kubernetes.io/docs/concepts/workloads/controllers/deployment/)
- [ReplicaSet 官方文档](https://kubernetes.io/docs/concepts/workloads/controllers/replicaset/)
- [Rolling Update 策略详解](https://kubernetes.io/docs/tutorials/kubernetes-basics/update/update-intro/)
- [PodDisruptionBudget 文档](https://kubernetes.io/docs/tasks/run-application/configure-pdb/)

### API 参考

- [Deployment v1 apps API](https://kubernetes.io/docs/reference/kubernetes-api/workload-resources/deployment-v1/)
- [ReplicaSet v1 apps API](https://kubernetes.io/docs/reference/kubernetes-api/workload-resources/replica-set-v1/)
- [Pod v1 core API](https://kubernetes.io/docs/reference/kubernetes-api/workload-resources/pod-v1/)

### 源码

- [Deployment Controller 源码](https://github.com/kubernetes/kubernetes/tree/master/pkg/controller/deployment)
- [ReplicaSet Controller 源码](https://github.com/kubernetes/kubernetes/tree/master/pkg/controller/replicaset)

### 工具

- [kubectl Cheat Sheet](https://kubernetes.io/docs/reference/kubectl/cheatsheet/)
- [Flagger - 渐进式交付工具](https://flagger.app/)
- [Argo Rollouts - 高级部署策略](https://argoproj.github.io/argo-rollouts/)
- [Kustomize - 配置管理](https://kustomize.io/)
- [Helm - 包管理器](https://helm.sh/)

### 社区资源

- [Kubernetes Blog - Deployment 最佳实践](https://kubernetes.io/blog/)
- [CNCF 云原生技术图谱](https://landscape.cncf.io/)
- [Kubernetes Slack 频道](https://kubernetes.slack.com/)

### 推荐阅读

- [《Kubernetes in Action》](https://www.manning.com/books/kubernetes-in-action-second-edition) - 第 9 章 Deployments
- [《Production Kubernetes》](https://www.oreilly.com/library/view/production-kubernetes/9781492092292/) - 第 4 章 Workload Resources
- [Google SRE Book - Rolling Update](https://sre.google/sre-book/release-engineering/)

### kudig-database 相关文档

- [01 - YAML 语法基础与资源通用规范](./01-yaml-syntax-resource-conventions.md)
- [02 - Namespace / ResourceQuota / LimitRange](./02-namespace-resourcequota-limitrange.md)
- [03 - Pod YAML 配置参考](./03-pod.md) *(待创建)*
- [05 - Service / Ingress 配置参考](./05-service-ingress.md) *(待创建)*
- [Kubernetes 故障排查 - Workloads](../topic-structural-trouble-shooting/05-workloads/01-pod-troubleshooting.md)

---

**文档更新日志**

| 版本 | 日期 | 作者 | 变更说明 |
|------|------|------|---------|
| v1.0 | 2026-02-10 | Qoder AI | 初始版本,覆盖 Deployment/ReplicaSet 全部核心内容 |

**反馈与贡献**

如有问题或建议,请通过以下方式反馈:
- GitHub Issues: [kudig-io/kudig-database](https://github.com/kudig-io/kudig-database/issues)
- Email: kudig@example.com

---

**© 2026 kudig.io - Kubernetes 中文知识库**
