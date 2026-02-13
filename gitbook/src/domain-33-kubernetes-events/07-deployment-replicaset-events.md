# 07 - Deployment 与 ReplicaSet 控制器事件

> **适用版本**: Kubernetes v1.25 - v1.32 | **最后更新**: 2026-02 | **作者**: Allen Galler

---

## 📋 文档概述

本文档全面记录 Kubernetes Deployment Controller 和 ReplicaSet Controller 产生的所有事件类型，涵盖滚动更新、扩缩容、回滚、进度监控等核心场景。这些控制器负责管理应用的声明式更新和副本保证，是生产环境中最常用的工作负载资源。

**核心职责**：
- **Deployment Controller**: 管理 ReplicaSet 版本、滚动更新策略、回滚机制
- **ReplicaSet Controller**: 确保指定数量的 Pod 副本运行，直接创建和删除 Pod

**事件总数**: 14 个核心事件

---

## 📊 事件分类总览

### Deployment Controller 事件 (12 个)

| 事件原因 | 类型 | 频率 | 关键程度 | 典型场景 |
|:---|:---:|:---:|:---:|:---|
| ScalingReplicaSet | Normal | 高频 | ⭐⭐⭐ | 滚动更新、扩缩容 |
| NewReplicaSetCreated | Normal | 中频 | ⭐⭐⭐ | 首次部署、更新 Pod 模板 |
| NewReplicaSetAvailable | Normal | 中频 | ⭐⭐⭐ | 新版本可用 |
| MinimumReplicasAvailable | Normal | 中频 | ⭐⭐ | 达到最小可用副本数 |
| ProgressDeadlineExceeded | Warning | 中频 | ⭐⭐⭐⭐⭐ | **生产高危** - 更新超时 |
| MinimumReplicasUnavailable | Warning | 中频 | ⭐⭐⭐⭐ | 可用副本数不足 |
| DeploymentRollback | Normal | 低频 | ⭐⭐⭐ | 手动回滚 |
| DeploymentPaused | Normal | 低频 | ⭐⭐ | 暂停更新 |
| DeploymentResumed | Normal | 低频 | ⭐⭐ | 恢复更新 |
| FoundNewReplicaSet | Normal | 中频 | ⭐ | 发现已存在的 RS |
| ReplicaSetUpdated | Normal | 中频 | ⭐ | 更新 RS 配置 |
| DeploymentRollbackRevisionNotFound | Warning | 罕见 | ⭐⭐⭐ | 回滚版本不存在 |

### ReplicaSet Controller 事件 (2 个)

| 事件原因 | 类型 | 频率 | 关键程度 | 典型场景 |
|:---|:---:|:---:|:---:|:---|
| SuccessfulCreate | Normal | 高频 | ⭐⭐⭐ | 创建 Pod 成功 |
| SuccessfulDelete | Normal | 中频 | ⭐⭐ | 删除 Pod 成功 |
| FailedCreate | Warning | 中频 | ⭐⭐⭐⭐⭐ | **生产高危** - Pod 创建失败 |
| SelectingAll | Warning | 罕见 | ⭐⭐⭐⭐ | 选择器错误 |

---

## 🎯 Deployment Controller 事件详解

### `ScalingReplicaSet` - 扩缩容 ReplicaSet

| 属性 | 说明 |
|:---|:---|
| **事件类型** | Normal |
| **来源组件** | deployment-controller |
| **关联资源** | Deployment |
| **适用版本** | v1.0+ |
| **生产频率** | 高频 |

#### 事件含义

Deployment Controller 调整 ReplicaSet 的副本数时产生。这是滚动更新过程中最常见的事件，反映了新旧 ReplicaSet 之间的副本迁移过程。

#### 典型事件消息

```bash
# 扩容新版本 ReplicaSet
Scaled up replica set myapp-7d4f8c9b5d to 3 from 1

# 缩容旧版本 ReplicaSet
Scaled down replica set myapp-6c8b7a4f3e to 1 from 3

# 直接扩容（非滚动更新）
Scaled up replica set myapp-7d4f8c9b5d to 5 from 3
```

#### 影响面说明

- **性能影响**: 扩容会触发新 Pod 调度和启动，消耗集群资源
- **可用性影响**: 配合 `maxSurge` 和 `maxUnavailable` 控制滚动更新速度
- **成本影响**: 扩容期间可能超出声明副本数（maxSurge > 0 时）

#### 排查建议

```bash
# 1. 查看 Deployment 扩缩容事件序列
kubectl describe deployment <deployment-name>

# 2. 查看所有 ReplicaSet 及其副本数
kubectl get rs -l app=<app-name> --show-labels

# 3. 查看滚动更新配置
kubectl get deployment <deployment-name> -o jsonpath='{.spec.strategy}'

# 4. 实时监控扩缩容过程
kubectl get pods -w -l app=<app-name>
```

#### 解决建议

**正常场景**：无需处理，这是 Deployment 正常工作流程

**异常场景**：

```yaml
# 场景 1: 扩容速度过快导致资源不足
# 调整滚动更新策略
spec:
  strategy:
    type: RollingUpdate
    rollingUpdate:
      maxSurge: 1        # 降低最大超出数量
      maxUnavailable: 0  # 保证可用性

# 场景 2: 扩容卡住不继续
# 检查 Deployment 状态
kubectl rollout status deployment/<deployment-name>

# 检查 Pod 创建失败原因（查看 FailedCreate 事件）
kubectl describe rs <replicaset-name>

# 场景 3: 旧版本 RS 缩容到 0 太慢
# 检查 Pod 终止配置
spec:
  template:
    spec:
      terminationGracePeriodSeconds: 30  # 调整优雅终止时间
```

---

### `NewReplicaSetCreated` - 创建新 ReplicaSet

| 属性 | 说明 |
|:---|:---|
| **事件类型** | Normal |
| **来源组件** | deployment-controller |
| **关联资源** | Deployment |
| **适用版本** | v1.0+ |
| **生产频率** | 中频 |

#### 事件含义

Deployment Controller 检测到 Pod 模板变更，创建新的 ReplicaSet 来管理新版本 Pod。这是滚动更新的第一步，标志着新版本部署的开始。

#### 典型事件消息

```bash
Created new replica set "myapp-7d4f8c9b5d"
```

#### 影响面说明

- **版本管理**: 每次 Pod 模板变更都会创建新 ReplicaSet，旧 RS 保留用于回滚
- **资源占用**: ReplicaSet 对象本身占用 etcd 空间（虽然很小）
- **历史限制**: 默认保留 10 个历史 RS（`spec.revisionHistoryLimit`）

#### 排查建议

```bash
# 1. 查看 ReplicaSet 创建事件
kubectl describe deployment <deployment-name> | grep "Created new replica set"

# 2. 查看所有 ReplicaSet 历史版本
kubectl get rs -l app=<app-name> --sort-by=.metadata.creationTimestamp

# 3. 查看 ReplicaSet 的 Pod 模板 hash
kubectl get rs -l app=<app-name> -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{.metadata.labels.pod-template-hash}{"\n"}{end}'

# 4. 对比新旧 ReplicaSet 的差异
kubectl diff -f deployment.yaml
```

#### 解决建议

**正常场景**：这是正常的更新流程

**异常场景**：

```yaml
# 场景 1: 频繁创建新 ReplicaSet（配置抖动）
# 问题：短时间内多次修改 Deployment 导致创建大量 RS
# 解决：使用 pause/resume 机制批量修改

kubectl rollout pause deployment/<deployment-name>
kubectl set image deployment/<deployment-name> app=myapp:v2
kubectl set resources deployment/<deployment-name> -c=app --limits=cpu=200m
kubectl rollout resume deployment/<deployment-name>

# 场景 2: ReplicaSet 历史版本过多
# 调整保留数量
spec:
  revisionHistoryLimit: 3  # 只保留最近 3 个版本

# 场景 3: 新 ReplicaSet 创建但未扩容
# 检查是否被暂停
kubectl rollout status deployment/<deployment-name>
kubectl rollout resume deployment/<deployment-name>  # 如果被暂停
```

---

### `ProgressDeadlineExceeded` - ⚠️ 更新进度超时

| 属性 | 说明 |
|:---|:---|
| **事件类型** | Warning |
| **来源组件** | deployment-controller |
| **关联资源** | Deployment |
| **适用版本** | v1.6+ |
| **生产频率** | 中频 |

#### 事件含义

**生产环境最常见的 Deployment 故障事件**。当滚动更新在 `spec.progressDeadlineSeconds` 时间内未取得进展时触发。这是 Kubernetes 自动检测更新失败的核心机制。

#### 典型事件消息

```bash
# 标准超时消息
Deployment "myapp" has timed out progressing.

# 详细消息示例
ReplicaSet "myapp-7d4f8c9b5d" has timed out progressing.
```

#### 影响面说明

- **可用性影响**: 更新被阻塞，新版本 Pod 无法达到期望副本数
- **回滚决策**: 触发此事件后 Deployment 不会自动回滚，需要人工介入
- **监控告警**: 应作为 **P0 级生产告警**，立即响应

#### 排查建议

```bash
# ==========================================
# 阶段 1: 快速诊断 - 确定卡在哪个阶段
# ==========================================

# 1. 查看 Deployment 状态（最重要）
kubectl get deployment <deployment-name> -o yaml | grep -A 10 "conditions:"

# 关键字段解读：
# - type: Progressing, status: False, reason: ProgressDeadlineExceeded
# - message: 会显示具体超时原因

# 2. 查看新旧 ReplicaSet 副本数
kubectl get rs -l app=<app-name>
# 分析：
# - 新 RS 副本数是否增长到期望值？
# - 旧 RS 副本数是否正常缩容？

# 3. 查看新 ReplicaSet 的 Pod 状态
NEW_RS=$(kubectl get rs -l app=<app-name> --sort-by=.metadata.creationTimestamp | tail -1 | awk '{print $1}')
kubectl get pods -l pod-template-hash=${NEW_RS##*-}

# ==========================================
# 阶段 2: 深度排查 - 找到根本原因
# ==========================================

# 4. 查看 Pod 创建失败原因
kubectl describe rs $NEW_RS | grep -A 5 "Events:"

# 5. 检查 Pod 启动失败原因
kubectl get pods -l pod-template-hash=${NEW_RS##*-} -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{.status.phase}{"\t"}{.status.containerStatuses[0].state}{"\n"}{end}'

# 6. 查看 Pod 详细错误信息
kubectl describe pod <pod-name>
kubectl logs <pod-name> --previous  # 如果是 CrashLoopBackOff

# ==========================================
# 阶段 3: 环境检查 - 排查集群资源
# ==========================================

# 7. 检查节点资源是否充足
kubectl top nodes
kubectl describe nodes | grep -A 5 "Allocated resources:"

# 8. 检查是否有 Pod 卡在 Pending
kubectl get pods -l pod-template-hash=${NEW_RS##*-} -o wide

# 9. 查看调度失败事件
kubectl get events --field-selector involvedObject.kind=Pod,reason=FailedScheduling

# ==========================================
# 阶段 4: 配置检查
# ==========================================

# 10. 检查 Readiness Probe 配置是否合理
kubectl get deployment <deployment-name> -o jsonpath='{.spec.template.spec.containers[0].readinessProbe}'

# 11. 检查 progressDeadlineSeconds 配置
kubectl get deployment <deployment-name> -o jsonpath='{.spec.progressDeadlineSeconds}'
```

#### 解决建议

#### **根本原因分类与解决方案**

##### 原因 1: Pod 启动时间过长（最常见 - 占 40%）

**症状**：Pod 状态为 Running，但 Readiness Probe 一直失败

```bash
# 诊断命令
kubectl get pods -l app=<app-name> -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{.status.conditions[?(@.type=="Ready")].status}{"\t"}{.status.containerStatuses[0].ready}{"\n"}{end}'

# 查看 Readiness Probe 失败原因
kubectl describe pod <pod-name> | grep -A 10 "Readiness"
```

**解决方案**：

```yaml
# 方案 A: 延长进度超时时间
spec:
  progressDeadlineSeconds: 600  # 从默认 600s 延长到 900s（根据应用实际启动时间）
  
  template:
    spec:
      containers:
      - name: app
        # 方案 B: 调整 Readiness Probe 参数
        readinessProbe:
          httpGet:
            path: /health
            port: 8080
          initialDelaySeconds: 30   # 增加初始延迟（应用启动需要时间）
          periodSeconds: 10          # 探测间隔
          timeoutSeconds: 5          # 单次探测超时
          successThreshold: 1        # 成功 1 次即认为就绪
          failureThreshold: 3        # 失败 3 次才认为失败

# 方案 C: 优化应用启动速度
# - 减少容器镜像大小
# - 优化应用初始化逻辑
# - 使用 StartupProbe 处理慢启动应用
```

##### 原因 2: 资源不足导致 Pod 无法调度（占 25%）

**症状**：Pod 状态一直是 Pending

```bash
# 诊断
kubectl describe pod <pod-name> | grep -A 5 "Events:"
# 错误示例：
# 0/5 nodes are available: 3 Insufficient cpu, 2 Insufficient memory.
```

**解决方案**：

```yaml
# 方案 A: 降低资源请求
spec:
  template:
    spec:
      containers:
      - name: app
        resources:
          requests:
            cpu: 100m      # 从 500m 降低
            memory: 128Mi  # 从 512Mi 降低
          limits:
            cpu: 500m
            memory: 512Mi

# 方案 B: 调整滚动更新策略（减少并发 Pod 数量）
spec:
  strategy:
    rollingUpdate:
      maxSurge: 0          # 不允许超出副本数
      maxUnavailable: 1    # 一次只更新 1 个 Pod

# 方案 C: 扩容集群节点或清理资源
```

##### 原因 3: 镜像拉取失败（占 15%）

**症状**：Pod 状态为 ImagePullBackOff 或 ErrImagePull

```bash
# 诊断
kubectl describe pod <pod-name> | grep "Image"
# 错误示例：
# Failed to pull image "myapp:v2": rpc error: code = NotFound desc = failed to pull and unpack image
```

**解决方案**：

```bash
# 方案 A: 检查镜像是否存在
docker pull myapp:v2

# 方案 B: 检查 ImagePullSecrets
kubectl get deployment <deployment-name> -o jsonpath='{.spec.template.spec.imagePullSecrets}'

# 方案 C: 使用正确的镜像标签
kubectl set image deployment/<deployment-name> app=myapp:v2-correct

# 方案 D: 配置镜像拉取策略
spec:
  template:
    spec:
      containers:
      - name: app
        image: myapp:v2
        imagePullPolicy: IfNotPresent  # 或 Always
```

##### 原因 4: 应用启动后立即崩溃（占 10%）

**症状**：Pod 状态为 CrashLoopBackOff

```bash
# 诊断
kubectl logs <pod-name> --previous
kubectl describe pod <pod-name>
```

**解决方案**：

```bash
# 1. 立即回滚到稳定版本
kubectl rollout undo deployment/<deployment-name>

# 2. 修复应用代码后重新部署
# 3. 使用 Canary 部署策略逐步验证
```

##### 原因 5: PreStop Hook 或优雅终止时间过长（占 5%）

**症状**：旧 Pod 终止缓慢，阻塞新 Pod 扩容

```bash
# 诊断
kubectl get pods -l app=<app-name> -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{.metadata.deletionTimestamp}{"\t"}{.status.phase}{"\n"}{end}'
```

**解决方案**：

```yaml
spec:
  template:
    spec:
      terminationGracePeriodSeconds: 30  # 从 300s 缩短到 30s
      
      containers:
      - name: app
        lifecycle:
          preStop:
            exec:
              command: ["/bin/sh", "-c", "sleep 10"]  # 减少 preStop 时间
```

##### 原因 6: PodDisruptionBudget (PDB) 阻塞（占 3%）

**症状**：旧 Pod 无法被驱逐

```bash
# 诊断
kubectl get pdb
kubectl describe pdb <pdb-name>
```

**解决方案**：

```yaml
# 调整 PDB 配置
apiVersion: policy/v1
kind: PodDisruptionBudget
metadata:
  name: myapp-pdb
spec:
  minAvailable: 1    # 从 2 降低到 1（如果总副本数较小）
  selector:
    matchLabels:
      app: myapp
```

##### 原因 7: Admission Webhook 拒绝（占 2%）

**症状**：Pod 创建请求被拦截

```bash
# 诊断
kubectl describe rs $NEW_RS | grep "admission webhook"
```

**解决方案**：

```bash
# 检查并修复 Webhook 策略
kubectl get validatingwebhookconfiguration
kubectl get mutatingwebhookconfiguration

# 临时禁用有问题的 Webhook（谨慎操作）
kubectl delete validatingwebhookconfiguration <webhook-name>
```

---

#### 生产最佳实践

```yaml
# ============================================
# 生产级 Deployment 配置模板
# ============================================
apiVersion: apps/v1
kind: Deployment
metadata:
  name: myapp
spec:
  replicas: 3
  
  # 关键配置 1: 进度超时设置
  progressDeadlineSeconds: 600  # 根据应用启动时间调整（默认 600s）
  
  # 关键配置 2: 历史版本保留
  revisionHistoryLimit: 10
  
  # 关键配置 3: 滚动更新策略
  strategy:
    type: RollingUpdate
    rollingUpdate:
      maxSurge: 1              # 最多超出 1 个 Pod（25%）
      maxUnavailable: 0        # 保证可用性，不允许低于期望副本数
  
  selector:
    matchLabels:
      app: myapp
  
  template:
    metadata:
      labels:
        app: myapp
        version: v2
    spec:
      # 关键配置 4: 优雅终止时间
      terminationGracePeriodSeconds: 30
      
      containers:
      - name: app
        image: myapp:v2
        imagePullPolicy: IfNotPresent
        
        # 关键配置 5: 资源配置
        resources:
          requests:
            cpu: 100m
            memory: 128Mi
          limits:
            cpu: 500m
            memory: 512Mi
        
        # 关键配置 6: 健康检查（最重要）
        startupProbe:  # v1.18+ 推荐使用
          httpGet:
            path: /health
            port: 8080
          initialDelaySeconds: 0
          periodSeconds: 10
          timeoutSeconds: 3
          successThreshold: 1
          failureThreshold: 30  # 最多等待 300s（30 * 10s）
        
        readinessProbe:
          httpGet:
            path: /ready
            port: 8080
          initialDelaySeconds: 0  # 使用 startupProbe 后可设为 0
          periodSeconds: 10
          timeoutSeconds: 3
          successThreshold: 1
          failureThreshold: 3
        
        livenessProbe:
          httpGet:
            path: /health
            port: 8080
          initialDelaySeconds: 30
          periodSeconds: 10
          timeoutSeconds: 3
          successThreshold: 1
          failureThreshold: 3
        
        # 关键配置 7: 生命周期钩子
        lifecycle:
          preStop:
            exec:
              command: ["/bin/sh", "-c", "sleep 5 && kill -SIGTERM 1"]
```

---

#### 监控与告警配置

```yaml
# Prometheus AlertManager 规则
groups:
- name: deployment-alerts
  interval: 30s
  rules:
  
  # 告警 1: ProgressDeadlineExceeded 检测（P0 级）
  - alert: DeploymentProgressDeadlineExceeded
    expr: |
      kube_deployment_status_condition{condition="Progressing",status="false",reason="ProgressDeadlineExceeded"} == 1
    for: 1m
    labels:
      severity: critical
      team: sre
    annotations:
      summary: "Deployment {{ $labels.namespace }}/{{ $labels.deployment }} 更新超时"
      description: "滚动更新在 progressDeadlineSeconds 时间内未完成，需要立即排查"
      runbook: "https://wiki.company.com/runbook/deployment-timeout"
  
  # 告警 2: 新版本 Pod 创建失败（P0 级）
  - alert: DeploymentReplicaSetCreateFailed
    expr: |
      increase(kube_replicaset_status_replicas{namespace!="kube-system"}[5m]) == 0
      and
      kube_deployment_spec_replicas > kube_deployment_status_replicas_available
    for: 5m
    labels:
      severity: critical
    annotations:
      summary: "Deployment {{ $labels.namespace }}/{{ $labels.deployment }} Pod 创建停滞"
  
  # 告警 3: 可用副本数低于期望值（P1 级）
  - alert: DeploymentReplicasMismatch
    expr: |
      kube_deployment_spec_replicas != kube_deployment_status_replicas_available
    for: 15m
    labels:
      severity: warning
    annotations:
      summary: "Deployment {{ $labels.namespace }}/{{ $labels.deployment }} 副本数不匹配"
      description: "期望: {{ $value }} 副本，当前可用: {{ $labels.replicas_available }}"
```

---

### `MinimumReplicasAvailable` - 达到最小可用副本数

| 属性 | 说明 |
|:---|:---|
| **事件类型** | Normal |
| **来源组件** | deployment-controller |
| **关联资源** | Deployment |
| **适用版本** | v1.2+ |
| **生产频率** | 中频 |

#### 事件含义

Deployment 的可用副本数达到或超过 `.spec.replicas - maxUnavailable` 时触发。这是滚动更新过程中的关键检查点，确保应用始终保持最低服务能力。

#### 典型事件消息

```bash
Deployment has minimum availability.

# 详细消息
Deployment "myapp" has minimum availability (2/3 replicas available).
```

#### 影响面说明

- **可用性保证**: 标志着 Deployment 满足最低 SLA 要求
- **更新进度**: 在达到此状态后，才会继续缩容旧版本 ReplicaSet
- **监控指标**: 可用于判断滚动更新是否健康进行

#### 排查建议

```bash
# 1. 查看当前可用副本数
kubectl get deployment <deployment-name> -o jsonpath='{.status.availableReplicas}/{.spec.replicas}'

# 2. 查看 Pod 就绪状态
kubectl get pods -l app=<app-name> -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{.status.conditions[?(@.type=="Ready")].status}{"\n"}{end}'

# 3. 查看滚动更新进度
kubectl rollout status deployment/<deployment-name>
```

#### 解决建议

**正常场景**：这是健康的滚动更新信号，无需处理

**注意事项**：

```yaml
# 确保 maxUnavailable 配置合理
spec:
  replicas: 3
  strategy:
    rollingUpdate:
      maxSurge: 1
      maxUnavailable: 1  # 最小可用副本数 = 3 - 1 = 2

# 高可用服务建议配置
spec:
  replicas: 5
  strategy:
    rollingUpdate:
      maxSurge: 2
      maxUnavailable: 1  # 确保至少 4 个副本可用
```

---

### `MinimumReplicasUnavailable` - 可用副本数不足

| 属性 | 说明 |
|:---|:---|
| **事件类型** | Warning |
| **来源组件** | deployment-controller |
| **关联资源** | Deployment |
| **适用版本** | v1.2+ |
| **生产频率** | 中频 |

#### 事件含义

Deployment 的可用副本数低于最小要求（`.spec.replicas - maxUnavailable`），标志着服务可用性受损，可能影响业务。

#### 典型事件消息

```bash
Deployment does not have minimum availability.

# 详细消息
Deployment "myapp" does not have minimum availability (1/3 replicas available, need 2).
```

#### 影响面说明

- **可用性告警**: 服务容量低于预期，可能无法处理正常流量
- **更新阻塞**: Deployment Controller 会暂停进一步缩容，等待 Pod 恢复
- **业务影响**: 可能导致请求超时、限流或服务降级

#### 排查建议

```bash
# 1. 快速查看问题 Pod
kubectl get pods -l app=<app-name> -o wide | grep -v Running

# 2. 查看不可用原因
kubectl describe deployment <deployment-name>

# 3. 检查 Pod 事件
kubectl get events --field-selector involvedObject.kind=Pod --sort-by='.lastTimestamp'

# 4. 查看 ReplicaSet 状态
kubectl get rs -l app=<app-name>
```

#### 解决建议

```bash
# 场景 1: 新版本 Pod 启动失败
# 立即回滚
kubectl rollout undo deployment/<deployment-name>

# 场景 2: 节点资源不足
# 检查节点资源
kubectl top nodes
kubectl describe nodes | grep -A 5 "Non-terminated Pods"

# 临时降低资源请求或扩容节点

# 场景 3: 健康检查配置过严
# 临时调整探测参数
kubectl set probe deployment/<deployment-name> --readiness --failure-threshold=5

# 场景 4: 外部依赖故障（数据库、Redis 等）
# 检查应用日志
kubectl logs -l app=<app-name> --tail=100
```

---

### `DeploymentRollback` - 回滚部署

| 属性 | 说明 |
|:---|:---|
| **事件类型** | Normal |
| **来源组件** | deployment-controller |
| **关联资源** | Deployment |
| **适用版本** | v1.2+ |
| **生产频率** | 低频 |

#### 事件含义

执行 `kubectl rollout undo` 命令或通过 API 触发回滚操作时产生。Deployment Controller 将 Pod 模板恢复到历史版本。

#### 典型事件消息

```bash
Rolled back deployment "myapp" to revision 2

# 手动指定版本回滚
Rolled back deployment "myapp" to revision 3
```

#### 影响面说明

- **版本恢复**: 将 Deployment 恢复到之前的稳定版本
- **滚动回滚**: 回滚过程遵循相同的滚动更新策略（maxSurge/maxUnavailable）
- **事件触发**: 会产生新的 `ScalingReplicaSet` 事件序列

#### 排查建议

```bash
# 1. 查看回滚历史
kubectl rollout history deployment/<deployment-name>

# 2. 查看具体版本的配置
kubectl rollout history deployment/<deployment-name> --revision=2

# 3. 对比当前版本与历史版本差异
kubectl rollout history deployment/<deployment-name> --revision=3 > /tmp/rev3.yaml
kubectl rollout history deployment/<deployment-name> --revision=2 > /tmp/rev2.yaml
diff /tmp/rev3.yaml /tmp/rev2.yaml

# 4. 查看回滚进度
kubectl rollout status deployment/<deployment-name>
```

#### 解决建议

```bash
# 场景 1: 回滚到上一个版本（最常用）
kubectl rollout undo deployment/<deployment-name>

# 场景 2: 回滚到指定版本
kubectl rollout undo deployment/<deployment-name> --to-revision=2

# 场景 3: 回滚后仍有问题，继续回滚
kubectl rollout undo deployment/<deployment-name> --to-revision=1

# 场景 4: 防止误回滚，查看回滚预期
kubectl rollout history deployment/<deployment-name> --revision=2

# 场景 5: 回滚版本已被清理
# 错误: "unable to find specified revision"
# 解决: 只能手动修改 Deployment 配置或重新部署

# 增加历史版本保留数量
kubectl patch deployment <deployment-name> -p '{"spec":{"revisionHistoryLimit":20}}'
```

---

### `DeploymentRollbackRevisionNotFound` - 回滚版本不存在

| 属性 | 说明 |
|:---|:---|
| **事件类型** | Warning |
| **来源组件** | deployment-controller |
| **关联资源** | Deployment |
| **适用版本** | v1.2+ |
| **生产频率** | 罕见 |

#### 事件含义

尝试回滚到不存在的历史版本时触发。通常是因为版本已被清理（超过 `revisionHistoryLimit`）或指定了无效的版本号。

#### 典型事件消息

```bash
Unable to find the revision 5 for deployment "myapp"
```

#### 影响面说明

- **回滚失败**: Deployment 保持当前状态，不会执行任何操作
- **数据丢失**: 历史版本配置已被永久删除，无法恢复

#### 排查建议

```bash
# 1. 查看当前可用的历史版本
kubectl rollout history deployment/<deployment-name>

# 2. 查看 revisionHistoryLimit 配置
kubectl get deployment <deployment-name> -o jsonpath='{.spec.revisionHistoryLimit}'

# 3. 查看所有 ReplicaSet（包括已删除的）
kubectl get rs -l app=<app-name> --show-labels
```

#### 解决建议

```bash
# 场景 1: 回滚到最近的可用版本
kubectl rollout history deployment/<deployment-name>  # 查看可用版本
kubectl rollout undo deployment/<deployment-name> --to-revision=<available-revision>

# 场景 2: 手动恢复配置
# 如果有 Git 版本控制，从代码仓库恢复
git checkout <commit-hash> -- deployment.yaml
kubectl apply -f deployment.yaml

# 场景 3: 预防措施 - 增加历史版本保留数量
kubectl patch deployment <deployment-name> -p '{"spec":{"revisionHistoryLimit":20}}'

# 场景 4: 使用外部配置管理工具（推荐）
# Helm: helm rollback <release-name> <revision>
# ArgoCD: 自动保留完整历史
# Flux: Git 仓库即历史记录
```

---

### `DeploymentPaused` - 部署已暂停

| 属性 | 说明 |
|:---|:---|
| **事件类型** | Normal |
| **来源组件** | deployment-controller |
| **关联资源** | Deployment |
| **适用版本** | v1.2+ |
| **生产频率** | 低频 |

#### 事件含义

执行 `kubectl rollout pause` 命令后产生。暂停状态下，Deployment Controller 不会响应 Pod 模板的任何修改，适合批量修改配置。

#### 典型事件消息

```bash
Deployment myapp paused
```

#### 影响面说明

- **更新冻结**: 对 `.spec.template` 的任何修改都不会触发滚动更新
- **扩缩容正常**: 修改 `.spec.replicas` 仍然生效
- **手动恢复**: 必须显式执行 `resume` 才能恢复更新

#### 排查建议

```bash
# 1. 检查 Deployment 是否被暂停
kubectl get deployment <deployment-name> -o jsonpath='{.spec.paused}'

# 2. 查看暂停时间和原因
kubectl describe deployment <deployment-name> | grep -A 5 "Conditions:"

# 3. 查看暂停期间累积的配置变更
kubectl diff -f deployment.yaml
```

#### 解决建议

```bash
# 使用场景: 批量修改配置，一次性部署

# 步骤 1: 暂停 Deployment
kubectl rollout pause deployment/<deployment-name>

# 步骤 2: 批量修改（不会触发滚动更新）
kubectl set image deployment/<deployment-name> app=myapp:v2
kubectl set resources deployment/<deployment-name> -c=app --limits=cpu=200m,memory=512Mi
kubectl set env deployment/<deployment-name> ENV=production

# 步骤 3: 确认修改无误
kubectl diff -f deployment.yaml

# 步骤 4: 恢复并一次性部署所有变更
kubectl rollout resume deployment/<deployment-name>

# 注意事项：
# - 暂停状态会持久化，重启 controller 后依然有效
# - 如果忘记 resume，应用将无法更新（常见生产事故）
# - 建议设置自动化检查，防止长期暂停
```

---

### `DeploymentResumed` - 部署已恢复

| 属性 | 说明 |
|:---|:---|
| **事件类型** | Normal |
| **来源组件** | deployment-controller |
| **关联资源** | Deployment |
| **适用版本** | v1.2+ |
| **生产频率** | 低频 |

#### 事件含义

执行 `kubectl rollout resume` 命令后产生。Deployment Controller 恢复工作，立即应用暂停期间累积的所有配置变更。

#### 典型事件消息

```bash
Deployment myapp resumed
```

#### 影响面说明

- **滚动更新触发**: 如果暂停期间修改了 Pod 模板，会立即开始滚动更新
- **批量生效**: 暂停期间的多次修改会合并为一次更新（创建一个新 ReplicaSet）

#### 排查建议

```bash
# 1. 确认 Deployment 已恢复
kubectl get deployment <deployment-name> -o jsonpath='{.spec.paused}'

# 2. 查看恢复后的滚动更新进度
kubectl rollout status deployment/<deployment-name>

# 3. 查看新创建的 ReplicaSet
kubectl get rs -l app=<app-name> --sort-by=.metadata.creationTimestamp
```

#### 解决建议

**正常场景**：这是 pause/resume 工作流的正常结束，无需处理

---

### `FoundNewReplicaSet` - 发现已存在的 ReplicaSet

| 属性 | 说明 |
|:---|:---|
| **事件类型** | Normal |
| **来源组件** | deployment-controller |
| **关联资源** | Deployment |
| **适用版本** | v1.0+ |
| **生产频率** | 中频 |

#### 事件含义

Deployment Controller 检测到与当前 Pod 模板匹配的 ReplicaSet 已存在（通过 `pod-template-hash` 标签匹配），会复用该 ReplicaSet 而不是创建新的。

#### 典型事件消息

```bash
Found new replica set "myapp-7d4f8c9b5d"
```

#### 影响面说明

- **版本复用**: 回滚到历史版本时会触发，复用旧的 ReplicaSet
- **资源优化**: 避免创建重复的 ReplicaSet 对象
- **历史追溯**: 保持 ReplicaSet 与历史版本的连续性

#### 排查建议

```bash
# 1. 查看 ReplicaSet 的创建时间（判断是新建还是复用）
kubectl get rs -l app=<app-name> --sort-by=.metadata.creationTimestamp

# 2. 查看 ReplicaSet 的 pod-template-hash
kubectl get rs -l app=<app-name> -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{.metadata.labels.pod-template-hash}{"\n"}{end}'

# 3. 对比 Deployment 的 Pod 模板 hash
kubectl get deployment <deployment-name> -o jsonpath='{.spec.template.metadata.labels.pod-template-hash}'
```

#### 解决建议

**正常场景**：这是正常的版本管理机制，无需处理

**使用案例**：

```bash
# 场景: 回滚后再次更新到同一版本
kubectl set image deployment/myapp app=myapp:v2  # 创建新 RS: myapp-abc123
kubectl set image deployment/myapp app=myapp:v1  # 回滚，复用旧 RS: myapp-def456
kubectl set image deployment/myapp app=myapp:v2  # 再次更新，复用 RS: myapp-abc123 (触发 FoundNewReplicaSet)
```

---

### `NewReplicaSetAvailable` - 新 ReplicaSet 可用

| 属性 | 说明 |
|:---|:---|
| **事件类型** | Normal |
| **来源组件** | deployment-controller |
| **关联资源** | Deployment |
| **适用版本** | v1.0+ |
| **生产频率** | 中频 |

#### 事件含义

新 ReplicaSet 的可用副本数达到期望值，且所有旧 ReplicaSet 副本数已缩容到 0，标志着滚动更新成功完成。

#### 典型事件消息

```bash
ReplicaSet "myapp-7d4f8c9b5d" has successfully progressed.

# 详细消息
Deployment "myapp" has successfully progressed.
```

#### 影响面说明

- **更新完成**: 滚动更新流程结束，所有 Pod 已替换为新版本
- **可用性恢复**: 新版本副本数达到期望值，服务恢复满载能力
- **监控指标**: 可用于计算更新成功率和更新耗时

#### 排查建议

```bash
# 1. 确认所有 Pod 都是新版本
kubectl get pods -l app=<app-name> -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{.metadata.labels.pod-template-hash}{"\n"}{end}'

# 2. 查看旧 ReplicaSet 是否已完全缩容
kubectl get rs -l app=<app-name>

# 3. 查看更新耗时
kubectl describe deployment <deployment-name> | grep "NewReplicaSetAvailable"
```

#### 解决建议

**正常场景**：这是滚动更新成功的标志，无需处理

**后续建议**：

```bash
# 1. 验证新版本功能
curl http://<service-endpoint>/health

# 2. 监控新版本指标
kubectl top pods -l app=<app-name>

# 3. 清理过多的历史 ReplicaSet（可选）
kubectl get rs -l app=<app-name> --sort-by=.metadata.creationTimestamp | head -n -10 | awk '{print $1}' | xargs kubectl delete rs
```

---

### `ReplicaSetUpdated` - 更新 ReplicaSet

| 属性 | 说明 |
|:---|:---|
| **事件类型** | Normal |
| **来源组件** | deployment-controller |
| **关联资源** | Deployment |
| **适用版本** | v1.0+ |
| **生产频率** | 中频 |

#### 事件含义

Deployment Controller 更新 ReplicaSet 的配置（通常是副本数以外的字段，如 annotations）。

#### 典型事件消息

```bash
Updated replica set "myapp-7d4f8c9b5d"
```

#### 影响面说明

- **元数据更新**: 通常涉及 annotations、labels 等元数据字段
- **不触发 Pod 重建**: 不影响现有 Pod，仅更新 ReplicaSet 对象本身

#### 排查建议

```bash
# 1. 查看 ReplicaSet 最近的修改
kubectl describe rs <replicaset-name>

# 2. 对比 ReplicaSet 与 Deployment 的配置
kubectl get deployment <deployment-name> -o yaml > /tmp/deploy.yaml
kubectl get rs <replicaset-name> -o yaml > /tmp/rs.yaml
diff /tmp/deploy.yaml /tmp/rs.yaml
```

#### 解决建议

**正常场景**：这是正常的配置同步，无需处理

---

## 🔄 ReplicaSet Controller 事件详解

### `SuccessfulCreate` - 创建 Pod 成功

| 属性 | 说明 |
|:---|:---|
| **事件类型** | Normal |
| **来源组件** | replicaset-controller |
| **关联资源** | ReplicaSet |
| **适用版本** | v1.0+ |
| **生产频率** | 高频 |

#### 事件含义

ReplicaSet Controller 成功向 API Server 提交 Pod 创建请求。注意：此时 Pod 对象已创建，但未必已调度或运行。

#### 典型事件消息

```bash
Created pod: myapp-7d4f8c9b5d-x8k2l
```

#### 影响面说明

- **副本保证**: ReplicaSet 正在执行副本数调谐，增加 Pod 数量
- **调度前置**: Pod 对象已创建，等待 Scheduler 分配节点
- **资源消耗**: Pod 对象占用 etcd 空间，等待实际资源分配

#### 排查建议

```bash
# 1. 查看新创建的 Pod 状态
kubectl get pods <pod-name> -o wide

# 2. 查看 Pod 是否被调度
kubectl get pod <pod-name> -o jsonpath='{.spec.nodeName}'

# 3. 如果 Pod 长时间 Pending，查看调度失败原因
kubectl describe pod <pod-name> | grep -A 10 "Events:"

# 4. 查看 ReplicaSet 的扩容事件序列
kubectl describe rs <replicaset-name>
```

#### 解决建议

**正常场景**：Pod 创建成功后会经历以下阶段：

```bash
# Pod 生命周期
SuccessfulCreate -> Pending -> Scheduled -> ContainerCreating -> Running -> Ready

# 监控整个流程
kubectl get pods -w -l pod-template-hash=<hash>
```

**异常场景**：

```bash
# 场景 1: Pod 创建后一直 Pending
# 原因: 资源不足、节点选择器不匹配、污点容忍度不匹配
kubectl describe pod <pod-name>

# 场景 2: Pod 创建速率过快
# 如果短时间创建大量 Pod，可能导致 API Server 负载过高
# 检查 ReplicaSet 副本数是否异常
kubectl get rs <replicaset-name> -o jsonpath='{.spec.replicas}'

# 场景 3: Pod 名称冲突（极罕见）
# 错误: "pods xxx already exists"
# 通常是 etcd 数据不一致或 controller 重复操作
# 检查 etcd 健康状态
```

---

### `SuccessfulDelete` - 删除 Pod 成功

| 属性 | 说明 |
|:---|:---|
| **事件类型** | Normal |
| **来源组件** | replicaset-controller |
| **关联资源** | ReplicaSet |
| **适用版本** | v1.0+ |
| **生产频率** | 中频 |

#### 事件含义

ReplicaSet Controller 成功向 API Server 提交 Pod 删除请求。Pod 进入 Terminating 状态，开始优雅终止流程。

#### 典型事件消息

```bash
Deleted pod: myapp-7d4f8c9b5d-x8k2l
```

#### 影响面说明

- **副本缩容**: ReplicaSet 正在减少副本数，通常发生在滚动更新或手动缩容时
- **优雅终止**: Pod 会执行 preStop Hook 和优雅终止流程（terminationGracePeriodSeconds）
- **服务摘除**: Pod 会从 Service Endpoints 中移除，停止接收新流量

#### 排查建议

```bash
# 1. 查看 Pod 删除原因（查看 ReplicaSet 事件）
kubectl describe rs <replicaset-name>

# 2. 查看 Pod 终止状态
kubectl get pods <pod-name> -o jsonpath='{.metadata.deletionTimestamp}'

# 3. 如果 Pod 长时间处于 Terminating，检查终止卡点
kubectl describe pod <pod-name>

# 4. 查看 Pod 终止日志
kubectl logs <pod-name> --previous
```

#### 解决建议

**正常场景**：Pod 删除后会经历以下阶段：

```bash
# Pod 终止流程
SuccessfulDelete -> Terminating -> PreStop Hook -> SIGTERM -> SIGKILL -> Deleted

# 监控终止过程
kubectl get pods -w -l pod-template-hash=<hash>
```

**异常场景**：

```bash
# 场景 1: Pod 长时间 Terminating（超过 terminationGracePeriodSeconds）
# 原因: preStop Hook 执行超时、进程不响应 SIGTERM、finalizer 阻塞

# 检查 Pod 的 finalizers
kubectl get pod <pod-name> -o jsonpath='{.metadata.finalizers}'

# 强制删除（谨慎操作）
kubectl delete pod <pod-name> --force --grace-period=0

# 场景 2: Pod 被删除后立即重建
# 原因: ReplicaSet 副本数未减少，controller 会重新创建
kubectl get rs <replicaset-name> -o jsonpath='{.spec.replicas}'

# 场景 3: 大量 Pod 同时被删除
# 可能导致服务短时间内可用性下降
# 检查是否是 Node NotReady 或 Deployment 配置错误
kubectl get nodes
kubectl describe deployment <deployment-name>
```

---

### `FailedCreate` - ⚠️ 创建 Pod 失败

| 属性 | 说明 |
|:---|:---|
| **事件类型** | Warning |
| **来源组件** | replicaset-controller |
| **关联资源** | ReplicaSet |
| **适用版本** | v1.0+ |
| **生产频率** | 中频 |

#### 事件含义

**生产环境最常见的 ReplicaSet 故障事件**。ReplicaSet Controller 向 API Server 提交 Pod 创建请求时被拒绝，导致副本数无法达到期望值。

#### 典型事件消息

```bash
# 1. ResourceQuota 限额不足
Error creating: pods "myapp-7d4f8c9b5d-x8k2l" is forbidden: exceeded quota: compute-quota, requested: limits.cpu=500m, used: limits.cpu=4, limited: limits.cpu=4

# 2. LimitRange 限制
Error creating: pods "myapp-7d4f8c9b5d-x8k2l" is forbidden: maximum cpu usage per Container is 2, but limit is 4

# 3. Admission Webhook 拒绝
Error creating: pods "myapp-7d4f8c9b5d-x8k2l" is forbidden: admission webhook "validator.example.com" denied the request: image tag must not be 'latest'

# 4. RBAC 权限不足
Error creating: pods "myapp-7d4f8c9b5d-x8k2l" is forbidden: User "system:serviceaccount:default:default" cannot create resource "pods" in API group "" in the namespace "production"

# 5. PodSecurityPolicy 违规（v1.21-v1.25）
Error creating: pods "myapp-7d4f8c9b5d-x8k2l" is forbidden: unable to validate against any pod security policy: [spec.containers[0].securityContext.privileged: Invalid value: true: Privileged containers are not allowed]

# 6. PodSecurity Admission 违规（v1.23+）
Error creating: pods "myapp-7d4f8c9b5d-x8k2l" is forbidden: violates PodSecurity "restricted:latest": privileged containers not allowed

# 7. 命名空间不存在
Error creating: namespaces "test" not found

# 8. ServiceAccount 不存在
Error creating: serviceaccounts "myapp-sa" not found
```

#### 影响面说明

- **可用性危机**: Pod 无法创建，副本数不足，直接影响服务容量
- **更新阻塞**: 如果是滚动更新场景，会触发 `ProgressDeadlineExceeded`
- **持续重试**: ReplicaSet Controller 会持续重试创建 Pod，产生大量事件

#### 排查建议

```bash
# ==========================================
# 阶段 1: 快速定位失败原因
# ==========================================

# 1. 查看 ReplicaSet 事件（最重要）
kubectl describe rs <replicaset-name> | grep -A 10 "FailedCreate"

# 2. 查看当前副本状态
kubectl get rs <replicaset-name> -o jsonpath='{.status.replicas}/{.spec.replicas} (Ready: {.status.readyReplicas})'

# 3. 查看 Deployment 状态
kubectl get deployment <deployment-name> -o jsonpath='{.status.conditions[?(@.type=="Progressing")]}'

# ==========================================
# 阶段 2: 根据错误类型深度排查
# ==========================================

# 针对 ResourceQuota 错误
kubectl describe quota -n <namespace>
kubectl get resourcequota -n <namespace> -o yaml

# 针对 LimitRange 错误
kubectl describe limitrange -n <namespace>

# 针对 Admission Webhook 错误
kubectl get validatingwebhookconfigurations
kubectl get mutatingwebhookconfigurations
kubectl describe validatingwebhookconfiguration <webhook-name>

# 针对 RBAC 错误
kubectl auth can-i create pods --as=system:serviceaccount:<namespace>:<serviceaccount>
kubectl get rolebinding,clusterrolebinding -n <namespace>

# 针对 PodSecurity 错误（v1.23+）
kubectl label namespace <namespace> pod-security.kubernetes.io/enforce=baseline --overwrite --dry-run=server
kubectl get ns <namespace> -o jsonpath='{.metadata.labels}'

# 针对资源不存在错误
kubectl get ns <namespace>
kubectl get sa <serviceaccount> -n <namespace>
```

#### 解决建议

#### **根据错误类型分类解决**

##### 错误类型 1: ResourceQuota 限额不足（占 35%）

**症状**：`exceeded quota: compute-quota`

```bash
# 诊断
kubectl describe quota -n <namespace>

# 查看当前使用情况
kubectl get resourcequota -n <namespace> -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{.status.used}{"\t"}{.status.hard}{"\n"}{end}'
```

**解决方案**：

```yaml
# 方案 A: 提高 Quota 限额（需要管理员权限）
apiVersion: v1
kind: ResourceQuota
metadata:
  name: compute-quota
  namespace: production
spec:
  hard:
    requests.cpu: "10"      # 从 5 增加到 10
    requests.memory: 20Gi   # 从 10Gi 增加到 20Gi
    limits.cpu: "20"        # 从 10 增加到 20
    limits.memory: 40Gi     # 从 20Gi 增加到 40Gi
    pods: "50"              # 从 30 增加到 50

# 方案 B: 降低 Pod 资源请求
spec:
  template:
    spec:
      containers:
      - name: app
        resources:
          requests:
            cpu: 100m       # 从 500m 降低
            memory: 128Mi   # 从 512Mi 降低

# 方案 C: 清理不必要的工作负载
kubectl get pods -n <namespace> --sort-by=.spec.containers[0].resources.requests.cpu
kubectl delete deployment <unused-deployment>
```

##### 错误类型 2: LimitRange 限制（占 15%）

**症状**：`maximum cpu usage per Container is 2, but limit is 4`

```bash
# 诊断
kubectl describe limitrange -n <namespace>
```

**解决方案**：

```yaml
# 方案 A: 调整 LimitRange 配置（需要管理员权限）
apiVersion: v1
kind: LimitRange
metadata:
  name: compute-limitrange
  namespace: production
spec:
  limits:
  - max:
      cpu: "4"        # 提高最大限制
      memory: 8Gi
    min:
      cpu: 50m
      memory: 64Mi
    default:          # 默认 limit
      cpu: 500m
      memory: 512Mi
    defaultRequest:   # 默认 request
      cpu: 100m
      memory: 128Mi
    type: Container

# 方案 B: 降低 Pod 资源 limits
spec:
  template:
    spec:
      containers:
      - name: app
        resources:
          limits:
            cpu: 2        # 从 4 降低到 2（符合 LimitRange 要求）
            memory: 2Gi
```

##### 错误类型 3: Admission Webhook 拒绝（占 20%）

**症状**：`admission webhook "xxx" denied the request`

```bash
# 诊断
kubectl get validatingwebhookconfigurations
kubectl describe validatingwebhookconfiguration <webhook-name>

# 查看 Webhook 具体拒绝原因
kubectl get events --field-selector reason=FailedCreate -n <namespace>
```

**解决方案**：

```bash
# 方案 A: 修复 Pod 配置以符合 Webhook 策略
# 例如: Webhook 拒绝使用 latest 标签
kubectl set image deployment/<deployment-name> app=myapp:v1.2.3  # 使用明确版本号

# 方案 B: 临时绕过 Webhook（仅测试环境，生产谨慎）
kubectl label namespace <namespace> admission.example.com/ignore=true

# 方案 C: 修复或禁用有问题的 Webhook（需要管理员权限）
# 检查 Webhook 服务是否健康
kubectl get svc -n <webhook-namespace>

# 临时禁用 Webhook（生产高危操作）
kubectl delete validatingwebhookconfiguration <webhook-name>

# 方案 D: 调整 Webhook 的 failurePolicy
kubectl patch validatingwebhookconfiguration <webhook-name> -p '{"webhooks":[{"name":"webhook.example.com","failurePolicy":"Ignore"}]}'
```

##### 错误类型 4: RBAC 权限不足（占 10%）

**症状**：`User "system:serviceaccount:default:default" cannot create resource "pods"`

```bash
# 诊断
kubectl auth can-i create pods --as=system:serviceaccount:<namespace>:<serviceaccount>

# 查看 ServiceAccount 的权限
kubectl get rolebinding,clusterrolebinding -n <namespace> -o json | jq '.items[] | select(.subjects[]?.name=="<serviceaccount>")'
```

**解决方案**：

```yaml
# 方案 A: 为 ServiceAccount 授予权限
apiVersion: rbac.authorization.k8s.io/v1
kind: Role
metadata:
  name: pod-creator
  namespace: production
rules:
- apiGroups: [""]
  resources: ["pods"]
  verbs: ["create", "get", "list", "delete"]

---
apiVersion: rbac.authorization.k8s.io/v1
kind: RoleBinding
metadata:
  name: myapp-pod-creator
  namespace: production
subjects:
- kind: ServiceAccount
  name: myapp-sa
  namespace: production
roleRef:
  kind: Role
  name: pod-creator
  apiGroup: rbac.authorization.k8s.io

# 方案 B: 使用具有足够权限的 ServiceAccount
spec:
  template:
    spec:
      serviceAccountName: admin-sa  # 切换到有权限的 SA
```

##### 错误类型 5: PodSecurity Admission 违规（v1.23+，占 10%）

**症状**：`violates PodSecurity "restricted:latest"`

```bash
# 诊断
kubectl get ns <namespace> -o jsonpath='{.metadata.labels}'

# 查看命名空间的 PodSecurity 配置
kubectl label namespace <namespace> --list | grep pod-security
```

**解决方案**：

```yaml
# 方案 A: 调整命名空间 PodSecurity 级别（需要管理员权限）
kubectl label namespace <namespace> \
  pod-security.kubernetes.io/enforce=baseline \
  pod-security.kubernetes.io/audit=restricted \
  pod-security.kubernetes.io/warn=restricted

# 方案 B: 修复 Pod 配置以符合 restricted 策略
spec:
  template:
    spec:
      securityContext:
        runAsNonRoot: true
        runAsUser: 1000
        fsGroup: 1000
        seccompProfile:
          type: RuntimeDefault
      
      containers:
      - name: app
        securityContext:
          allowPrivilegeEscalation: false
          capabilities:
            drop:
            - ALL
          readOnlyRootFilesystem: true

# 方案 C: 使用 audit/warn 模式（仅记录违规，不阻止）
kubectl label namespace <namespace> \
  pod-security.kubernetes.io/enforce=privileged \
  pod-security.kubernetes.io/audit=restricted \
  pod-security.kubernetes.io/warn=restricted
```

##### 错误类型 6: 依赖资源不存在（占 5%）

**症状**：`serviceaccounts "myapp-sa" not found`

```bash
# 诊断
kubectl get sa <serviceaccount> -n <namespace>
kubectl get secret <secret-name> -n <namespace>
kubectl get configmap <configmap-name> -n <namespace>
```

**解决方案**：

```bash
# 方案 A: 创建缺失的资源
kubectl create serviceaccount myapp-sa -n <namespace>

# 方案 B: 使用已存在的资源
kubectl patch deployment <deployment-name> -p '{"spec":{"template":{"spec":{"serviceAccountName":"default"}}}}'

# 方案 C: 确保资源创建顺序（使用 Helm、Kustomize 等工具）
```

##### 错误类型 7: API Server 限流或故障（占 3%）

**症状**：`too many requests` 或 `connection refused`

```bash
# 诊断
kubectl get --raw /metrics | grep apiserver_request_total

# 查看 API Server 日志
kubectl logs -n kube-system kube-apiserver-<node-name>
```

**解决方案**：

```bash
# 方案 A: 降低 ReplicaSet Controller 的并发操作
# 调整 kube-controller-manager 的 --concurrent-replicaset-syncs 参数（需要管理员权限）

# 方案 B: 减缓部署速度
spec:
  strategy:
    rollingUpdate:
      maxSurge: 1
      maxUnavailable: 0
  minReadySeconds: 10  # 每个 Pod 就绪后等待 10s 再继续

# 方案 C: 检查 API Server 健康状态
kubectl get componentstatus
```

---

#### 监控与告警

```yaml
# Prometheus 告警规则
groups:
- name: replicaset-alerts
  interval: 30s
  rules:
  
  # 告警: FailedCreate 持续发生（P0 级）
  - alert: ReplicaSetPodCreateFailed
    expr: |
      increase(kube_replicaset_status_replicas{namespace!="kube-system"}[5m]) == 0
      and
      kube_replicaset_spec_replicas > kube_replicaset_status_replicas
    for: 5m
    labels:
      severity: critical
      team: sre
    annotations:
      summary: "ReplicaSet {{ $labels.namespace }}/{{ $labels.replicaset }} Pod 创建失败"
      description: "持续 5 分钟无法创建 Pod，当前副本: {{ $value }}, 期望副本: {{ $labels.spec_replicas }}"
      runbook: "检查 ReplicaSet 事件: kubectl describe rs {{ $labels.replicaset }} -n {{ $labels.namespace }}"
  
  # 告警: ResourceQuota 接近限额（P1 级）
  - alert: ResourceQuotaNearLimit
    expr: |
      kube_resourcequota{type="used"} / kube_resourcequota{type="hard"} > 0.9
    for: 5m
    labels:
      severity: warning
    annotations:
      summary: "命名空间 {{ $labels.namespace }} ResourceQuota 接近限额"
      description: "资源 {{ $labels.resource }} 使用率: {{ $value | humanizePercentage }}"
```

---

### `SelectingAll` - ⚠️ 选择器匹配所有 Pod

| 属性 | 说明 |
|:---|:---|
| **事件类型** | Warning |
| **来源组件** | replicaset-controller |
| **关联资源** | ReplicaSet |
| **适用版本** | v1.0+ |
| **生产频率** | 罕见 |

#### 事件含义

ReplicaSet 的 selector 为空或配置错误，会匹配命名空间内的所有 Pod，这是非常危险的配置错误。

#### 典型事件消息

```bash
This replica set is selecting all pods. A non-empty selector is required.
```

#### 影响面说明

- **高危配置**: ReplicaSet 会尝试管理命名空间内所有 Pod，可能误删其他应用的 Pod
- **副本混乱**: 副本数计算错误，可能大量创建或删除 Pod
- **生产事故**: 可能导致整个命名空间内的应用大规模中断

#### 排查建议

```bash
# 1. 查看 ReplicaSet 的 selector
kubectl get rs <replicaset-name> -o jsonpath='{.spec.selector}'

# 2. 查看 ReplicaSet 当前管理的 Pod 数量
kubectl get pods -n <namespace> --selector=<selector> --show-labels

# 3. 查看 ReplicaSet 的完整配置
kubectl get rs <replicaset-name> -o yaml
```

#### 解决建议

```bash
# 立即删除有问题的 ReplicaSet（高危操作，需谨慎）
kubectl delete rs <replicaset-name> --cascade=false  # 不删除 Pod

# 检查并修复 Deployment 配置
kubectl get deployment <deployment-name> -o yaml

# 正确的 selector 配置示例
spec:
  selector:
    matchLabels:
      app: myapp
      version: v1
  template:
    metadata:
      labels:
        app: myapp        # 必须与 selector 匹配
        version: v1       # 必须与 selector 匹配
```

---

## 📈 滚动更新事件流程图

### 完整滚动更新事件序列（RollingUpdate 策略）

```
时间轴    Deployment Controller 事件           ReplicaSet Controller 事件          Pod 状态
─────────────────────────────────────────────────────────────────────────────────────────────

T0        用户执行: kubectl set image deployment/myapp app=myapp:v2
          │
          ▼
T1        NewReplicaSetCreated                                                   
          "Created new replica set myapp-v2-abc123"
          │
          ▼
T2        ScalingReplicaSet                                                      
          "Scaled up replica set myapp-v2-abc123 to 1 from 0"
          │                                       ▼
          │                                  SuccessfulCreate                     Pod myapp-v2-abc123-pod1
          │                                  "Created pod: myapp-v2-abc123-pod1"  └─> Pending
          │                                                                       └─> ContainerCreating
          │                                                                       └─> Running (未 Ready)
          ▼                                                                       
T3        等待新 Pod Ready (Readiness Probe 通过)                                 └─> Running + Ready ✓
          │
          ▼
T4        MinimumReplicasAvailable                                                
          "Deployment has minimum availability"
          (可用副本数 = 3, 满足最小要求 = 3 - 0 = 3)
          │
          ▼
T5        ScalingReplicaSet                                                      
          "Scaled up replica set myapp-v2-abc123 to 2 from 1"
          │                                       ▼
          │                                  SuccessfulCreate                     Pod myapp-v2-abc123-pod2
          │                                  "Created pod: myapp-v2-abc123-pod2"  └─> Pending -> Running + Ready ✓
          │
          │                                                                       旧版本 Pod 总数: 3
          │                                                                       新版本 Pod 总数: 2
          │                                                                       总 Pod 数: 5 (maxSurge=2 允许)
          ▼
T6        ScalingReplicaSet                                                      
          "Scaled down replica set myapp-v1-def456 to 2 from 3"
          │                                       ▼
          │                                  SuccessfulDelete                     Pod myapp-v1-def456-pod1
          │                                  "Deleted pod: myapp-v1-def456-pod1"  └─> Terminating
          │                                                                       └─> PreStop Hook
          │                                                                       └─> SIGTERM
          │                                                                       └─> Deleted ✓
          ▼
T7        ScalingReplicaSet                                                      
          "Scaled up replica set myapp-v2-abc123 to 3 from 2"
          │                                       ▼
          │                                  SuccessfulCreate                     Pod myapp-v2-abc123-pod3
          │                                  "Created pod: myapp-v2-abc123-pod3"  └─> Pending -> Running + Ready ✓
          │
          ▼                                                                       
T8        ScalingReplicaSet                                                      
          "Scaled down replica set myapp-v1-def456 to 1 from 2"
          │                                       ▼
          │                                  SuccessfulDelete                     Pod myapp-v1-def456-pod2
          │                                  "Deleted pod: myapp-v1-def456-pod2"  └─> Terminating -> Deleted ✓
          │
          ▼
T9        ScalingReplicaSet                                                      
          "Scaled down replica set myapp-v1-def456 to 0 from 1"
          │                                       ▼
          │                                  SuccessfulDelete                     Pod myapp-v1-def456-pod3
          │                                  "Deleted pod: myapp-v1-def456-pod3"  └─> Terminating -> Deleted ✓
          │
          ▼                                                                       所有 Pod 均为新版本 ✓
T10       NewReplicaSetAvailable                                                  
          "Deployment has successfully progressed"
          │
          ▼
          ✅ 滚动更新完成
          - 新 RS (myapp-v2-abc123): 3 副本 (全部 Ready)
          - 旧 RS (myapp-v1-def456): 0 副本
```

---

### 滚动更新配置对事件流程的影响

#### 配置 1: maxSurge=1, maxUnavailable=0 (保证可用性)

```yaml
spec:
  replicas: 3
  strategy:
    rollingUpdate:
      maxSurge: 1           # 最多 4 个 Pod (3 + 1)
      maxUnavailable: 0     # 至少 3 个 Pod 可用
```

**事件序列特点**：
- 先扩容新版本 Pod (Scaled up to 1)
- 等待新 Pod Ready
- 再缩容旧版本 Pod (Scaled down to 2)
- 循环直至完成

**优点**: 始终保持满足期望副本数，无服务降级  
**缺点**: 需要额外资源（峰值 4 个 Pod）

---

#### 配置 2: maxSurge=0, maxUnavailable=1 (节省资源)

```yaml
spec:
  replicas: 3
  strategy:
    rollingUpdate:
      maxSurge: 0           # 最多 3 个 Pod (不超出)
      maxUnavailable: 1     # 至少 2 个 Pod 可用
```

**事件序列特点**：
- 先缩容旧版本 Pod (Scaled down to 2)
- 再扩容新版本 Pod (Scaled up to 1)
- 等待新 Pod Ready
- 循环直至完成

**优点**: 不需要额外资源（始终 3 个 Pod）  
**缺点**: 更新期间可用 Pod 数减少（2 个），可能影响性能

---

#### 配置 3: maxSurge=2, maxUnavailable=1 (快速更新)

```yaml
spec:
  replicas: 3
  strategy:
    rollingUpdate:
      maxSurge: 2           # 最多 5 个 Pod (3 + 2)
      maxUnavailable: 1     # 至少 2 个 Pod 可用
```

**事件序列特点**：
- 快速扩容 2 个新 Pod (Scaled up to 2)
- 同时缩容 1 个旧 Pod (Scaled down to 2)
- 更新速度快

**优点**: 更新速度最快  
**缺点**: 需要更多资源（峰值 5 个 Pod）

---

### 异常场景事件序列

#### 场景 A: 新 Pod 启动失败（ProgressDeadlineExceeded）

```
T1   NewReplicaSetCreated "Created new replica set myapp-v2-abc123"
T2   ScalingReplicaSet "Scaled up replica set myapp-v2-abc123 to 1"
T3   SuccessfulCreate "Created pod: myapp-v2-abc123-pod1"
T4   Pod 状态: Pending -> ImagePullBackOff (镜像拉取失败)
T5   (600s progressDeadlineSeconds 超时)
T6   ProgressDeadlineExceeded ⚠️
     "Deployment has timed out progressing"
T7   更新阻塞，等待人工干预
```

**处理**：
```bash
# 立即回滚
kubectl rollout undo deployment/myapp

# 或修复问题后继续
kubectl set image deployment/myapp app=myapp:v2-fixed
```

---

#### 场景 B: Pod 创建失败（FailedCreate）

```
T1   NewReplicaSetCreated "Created new replica set myapp-v2-abc123"
T2   ScalingReplicaSet "Scaled up replica set myapp-v2-abc123 to 1"
T3   FailedCreate ⚠️
     "Error creating: exceeded quota: compute-quota"
T4   ReplicaSet Controller 持续重试（每 15s 一次）
T5   FailedCreate (重试失败)
T6   FailedCreate (重试失败)
...  持续失败直到修复配置或触发 ProgressDeadlineExceeded
```

**处理**：
```bash
# 查看详细错误
kubectl describe rs myapp-v2-abc123

# 修复 Quota 或降低资源请求
kubectl edit deployment myapp
```

---

## 🔀 部署策略对比

### RollingUpdate vs Recreate

| 特性 | RollingUpdate (默认) | Recreate |
|:---|:---|:---|
| **更新方式** | 逐步替换 Pod | 先删除所有旧 Pod，再创建新 Pod |
| **服务可用性** | 无中断（配置合理时） | **有中断**（所有 Pod 同时停止） |
| **资源占用** | 需要额外资源（maxSurge > 0 时） | 不需要额外资源 |
| **更新速度** | 较慢（逐步替换） | 快速（并发创建所有 Pod） |
| **版本共存** | 新旧版本短暂共存 | 不会共存 |
| **适用场景** | 大部分生产应用 | 单例应用、数据库迁移 |
| **事件序列** | 多次 ScalingReplicaSet | 一次 Scaled down to 0 + 一次 Scaled up to N |

---

### Recreate 策略事件序列

```yaml
spec:
  replicas: 3
  strategy:
    type: Recreate  # 非滚动更新
```

**事件流程**：

```
T1   用户执行: kubectl set image deployment/myapp app=myapp:v2
T2   ScalingReplicaSet "Scaled down replica set myapp-v1-def456 to 0 from 3"
     ├─> SuccessfulDelete "Deleted pod: myapp-v1-def456-pod1"
     ├─> SuccessfulDelete "Deleted pod: myapp-v1-def456-pod2"
     └─> SuccessfulDelete "Deleted pod: myapp-v1-def456-pod3"

T3   ⚠️ 服务完全不可用（所有 Pod 已删除）

T4   NewReplicaSetCreated "Created new replica set myapp-v2-abc123"
T5   ScalingReplicaSet "Scaled up replica set myapp-v2-abc123 to 3 from 0"
     ├─> SuccessfulCreate "Created pod: myapp-v2-abc123-pod1"
     ├─> SuccessfulCreate "Created pod: myapp-v2-abc123-pod2"
     └─> SuccessfulCreate "Created pod: myapp-v2-abc123-pod3"

T6   等待所有新 Pod Ready

T7   NewReplicaSetAvailable "Deployment has successfully progressed"
T8   ✅ 更新完成，服务恢复
```

**中断时间**：T3 到 T8（通常 30s - 2min）

---

## 🎯 生产环境最佳实践

### 1. 滚动更新配置推荐

```yaml
# 高可用服务（推荐）
spec:
  replicas: 5
  strategy:
    type: RollingUpdate
    rollingUpdate:
      maxSurge: 1           # 25% (保守策略，避免资源爆炸)
      maxUnavailable: 0     # 0% (保证可用性)
  
  # 关键: 进度超时配置
  progressDeadlineSeconds: 600  # 10 分钟（根据应用启动时间调整）
  
  # 关键: 最小就绪时间
  minReadySeconds: 10  # 新 Pod Ready 后等待 10s 再继续（防止启动即崩溃）
  
  template:
    spec:
      # 关键: 健康检查配置
      containers:
      - name: app
        startupProbe:      # v1.18+ 强烈推荐
          httpGet:
            path: /health
            port: 8080
          initialDelaySeconds: 0
          periodSeconds: 10
          failureThreshold: 30  # 最多等待 300s
        
        readinessProbe:    # 控制流量切换
          httpGet:
            path: /ready
            port: 8080
          initialDelaySeconds: 0
          periodSeconds: 5
          failureThreshold: 3
        
        livenessProbe:     # 防止僵尸进程
          httpGet:
            path: /health
            port: 8080
          initialDelaySeconds: 60
          periodSeconds: 10
          failureThreshold: 3
```

---

### 2. 监控告警配置

```yaml
# Prometheus 监控指标

# 指标 1: Deployment 副本数不匹配
kube_deployment_spec_replicas != kube_deployment_status_replicas_available

# 指标 2: Deployment 更新超时
kube_deployment_status_condition{condition="Progressing",status="false",reason="ProgressDeadlineExceeded"}

# 指标 3: ReplicaSet Pod 创建失败
rate(kube_pod_container_status_restarts_total[5m]) > 0

# 指标 4: 滚动更新耗时（自定义指标）
histogram_quantile(0.95, rate(deployment_rollout_duration_seconds_bucket[5m]))
```

---

### 3. 回滚策略

```bash
# 自动回滚（使用 CI/CD 工具实现）
# 示例: 如果新版本 5 分钟内错误率 > 5%，自动回滚

kubectl rollout status deployment/myapp --timeout=5m
if [ $? -ne 0 ]; then
  echo "Deployment failed, rolling back..."
  kubectl rollout undo deployment/myapp
fi

# 金丝雀部署（Canary Deployment）
# 使用 Flagger、Argo Rollouts 等工具实现自动渐进式发布
```

---

### 4. 常见问题排查清单

#### ✅ Deployment 更新卡住

```bash
# 1. 查看 Deployment 状态
kubectl get deployment <name> -o jsonpath='{.status.conditions[?(@.type=="Progressing")]}'

# 2. 查看 ReplicaSet 副本数
kubectl get rs -l app=<app-name>

# 3. 查看 Pod 状态
kubectl get pods -l app=<app-name> -o wide

# 4. 查看事件
kubectl describe deployment <name>
kubectl describe rs <replicaset-name>

# 5. 查看 Pod 日志
kubectl logs <pod-name>
```

#### ✅ Pod 创建失败

```bash
# 1. 查看 FailedCreate 事件
kubectl describe rs <replicaset-name> | grep FailedCreate

# 2. 根据错误类型排查
# - ResourceQuota: kubectl describe quota
# - LimitRange: kubectl describe limitrange
# - Admission Webhook: kubectl get validatingwebhookconfigurations
# - RBAC: kubectl auth can-i create pods --as=system:serviceaccount:ns:sa
```

#### ✅ Pod 启动缓慢

```bash
# 1. 检查镜像拉取时间
kubectl describe pod <pod-name> | grep "Pulling image"

# 2. 检查 Readiness Probe
kubectl get pod <pod-name> -o jsonpath='{.spec.containers[0].readinessProbe}'

# 3. 查看应用日志
kubectl logs <pod-name>

# 4. 调整配置
# - 增加 progressDeadlineSeconds
# - 增加 readinessProbe.initialDelaySeconds
# - 使用 startupProbe
```

---

## 📚 相关文档

- **[05 - Controller Manager 事件](./05-controller-manager-events.md)** - Deployment/ReplicaSet Controller 原理
- **[06 - Scheduler 事件](./06-scheduler-events.md)** - Pod 调度失败排查
- **[08 - StatefulSet 事件](./08-statefulset-events.md)** - 有状态应用控制器事件
- **[09 - DaemonSet 事件](./09-daemonset-events.md)** - 守护进程控制器事件
- **[10 - Job/CronJob 事件](./10-job-cronjob-events.md)** - 任务控制器事件

---

## 🔗 外部资源

- **Kubernetes 官方文档**: [Deployments](https://kubernetes.io/docs/concepts/workloads/controllers/deployment/)
- **Kubernetes 官方文档**: [ReplicaSet](https://kubernetes.io/docs/concepts/workloads/controllers/replicaset/)
- **源码参考**: [pkg/controller/deployment](https://github.com/kubernetes/kubernetes/tree/master/pkg/controller/deployment)
- **源码参考**: [pkg/controller/replicaset](https://github.com/kubernetes/kubernetes/tree/master/pkg/controller/replicaset)

---

> **KUDIG-DATABASE** | Domain-33: Kubernetes Events 全域事件大全 | 文档 07/15