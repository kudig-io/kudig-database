# Deployment 故障排查指南

> **适用版本**: Kubernetes v1.25 - v1.32 | **最后更新**: 2026-01 | **难度**: 中级
>
> **版本说明**:
> - v1.25+ 支持 minReadySeconds 与 PodReadinessGate 配合
> - v1.27+ 支持更精细的 Pod 删除策略
> - v1.28+ 支持 SidecarContainers (initContainers restartPolicy: Always)

---

## 0. 10 分钟快速诊断

1. **更新状态**：`kubectl rollout status deployment <name> --timeout=5m`，看是否卡在 Progressing。
2. **副本/RS**：`kubectl get rs -l app=<label> --sort-by=.metadata.creationTimestamp`，确认新 RS 是否扩容。
3. **Pod 事件**：`kubectl describe pod <pod>`，区分 Pending/CrashLoop/Probe 失败。
4. **镜像/配置**：检查镜像版本、`imagePullSecrets`、ConfigMap/Secret 是否更新。
5. **策略参数**：核对 `maxUnavailable/maxSurge/minReadySeconds/progressDeadlineSeconds`。
6. **快速缓解**：
   - 回滚：`kubectl rollout undo deployment <name>`。
   - 降速：临时调低并发，防止健康检查抖动。
7. **证据留存**：保存 Deployment/RS/Pod 描述与 events。

---

## 第一部分：问题现象与影响分析

### 1.1 Deployment 控制器架构

#### 1.1.1 三层控制架构

```
┌──────────────────────────────────────────────────────────────────────────┐
│                        Deployment Controller                              │
│                  (kube-controller-manager 内置)                          │
├──────────────────────────────────────────────────────────────────────────┤
│                                                                          │
│   ┌────────────────────────────────────────────────────────────────┐    │
│   │                      Deployment                                 │    │
│   │  spec.replicas: 3                                              │    │
│   │  spec.strategy.type: RollingUpdate                             │    │
│   │  spec.strategy.rollingUpdate:                                  │    │
│   │    maxUnavailable: 25%  (允许不可用副本上限)                   │    │
│   │    maxSurge: 25%        (允许超出副本上限)                     │    │
│   │  spec.minReadySeconds: 0                                       │    │
│   │  spec.progressDeadlineSeconds: 600                             │    │
│   │  spec.revisionHistoryLimit: 10  (保留 RS 历史数量)             │    │
│   └────────────────────────────┬───────────────────────────────────┘    │
│                                │                                         │
│                    Deployment Controller                                 │
│                    (调协循环 30s/次)                                     │
│                    管理 ReplicaSet 生命周期                              │
│                                │                                         │
│            ┌──────────────────┼──────────────────┐                      │
│            │                  │                  │                       │
│            ▼                  ▼                  ▼                       │
│   ┌─────────────┐    ┌─────────────┐    ┌─────────────┐                 │
│   │ ReplicaSet  │    │ ReplicaSet  │    │ ReplicaSet  │                 │
│   │ (revision 1)│    │ (revision 2)│    │ (revision 3)│                 │
│   │ replicas: 0 │    │ replicas: 0 │    │ replicas: 3 │  ← 当前版本     │
│   │ pod-hash:   │    │ pod-hash:   │    │ pod-hash:   │                 │
│   │  78f9d4c    │    │  a5b7e21    │    │  c8d9f3a    │                 │
│   └─────────────┘    └─────────────┘    └──────┬──────┘                 │
│       ↑                    ↑                   │                         │
│       └────────────────────┴───────────────────┘                         │
│         revisionHistoryLimit 控制保留数量                                │
│                                                │                         │
│                               ReplicaSet Controller                      │
│                               (调协循环 15s/次)                          │
│                               管理 Pod 副本数量                          │
│                                                │                         │
│                      ┌────────────┬────────────┼────────────┐           │
│                      ▼            ▼            ▼            ▼           │
│                 ┌────────┐  ┌────────┐  ┌────────┐  ┌────────┐         │
│                 │ Pod-1  │  │ Pod-2  │  │ Pod-3  │  │ Pod-4  │         │
│                 │ Ready  │  │ Ready  │  │ Ready  │  │Creating│ surge  │
│                 │ Age:5m │  │ Age:5m │  │ Age:3s │  │ Age:1s │         │
│                 └────────┘  └────────┘  └────────┘  └────────┘         │
│                                                                          │
│   滚动更新过程 (Reconcile Loop 核心逻辑)：                              │
│   1. 计算新旧 RS 的目标副本数 (根据 maxSurge/maxUnavailable)            │
│   2. 创建或扩容新 RS (不超过 maxSurge 限制)                             │
│   3. 等待新 Pod Ready + minReadySeconds                                 │
│   4. 缩容旧 RS (保证可用副本不低于 replicas - maxUnavailable)           │
│   5. 重复 2-4 直到新 RS replicas = spec.replicas                        │
│   6. 缩容旧 RS 至 0，保留历史 (受 revisionHistoryLimit 控制)            │
│                                                                          │
└──────────────────────────────────────────────────────────────────────────┘
```

#### 1.1.2 滚动更新核心算法

**控制器调协逻辑 (来自 k8s.io/kubernetes/pkg/controller/deployment)**

```go
// 伪代码：Deployment Controller 的 Reconcile 核心逻辑
func (dc *DeploymentController) reconcile(deployment) {
    // 1. 获取所有 ReplicaSet
    allRS := listReplicaSets(deployment)
    newRS, oldRSs := findNewAndOldReplicaSets(allRS)
    
    // 2. 计算副本数约束
    maxSurge := calculateMaxSurge(deployment)           // 例: 25% of 10 = 3
    maxUnavailable := calculateMaxUnavailable(deployment) // 例: 25% of 10 = 2
    
    // 3. 计算新 RS 的目标副本数
    // newRS 最多 = spec.replicas + maxSurge
    // 但要保证 可用副本数 >= spec.replicas - maxUnavailable
    desiredReplicas := deployment.Spec.Replicas
    
    // 当前总副本数和可用副本数
    totalReplicas := sumReplicas(allRS)
    availableReplicas := countAvailableReplicas(allRS)
    
    // 允许的最大副本数 = desired + maxSurge
    maxReplicasAllowed := desiredReplicas + maxSurge
    
    // 允许的最小可用副本数 = desired - maxUnavailable
    minAvailableReplicas := desiredReplicas - maxUnavailable
    
    // 4. 扩容新 RS
    if newRS.Replicas < desiredReplicas {
        // 可以创建的新 Pod 数量
        scaleUpCount := min(
            desiredReplicas - newRS.Replicas,         // 还需要的副本数
            maxReplicasAllowed - totalReplicas,       // 不超过 maxSurge
        )
        scaleReplicaSet(newRS, newRS.Replicas + scaleUpCount)
    }
    
    // 5. 缩容旧 RS
    if newRS.Replicas == desiredReplicas && allPodsAvailable(newRS) {
        for oldRS in oldRSs {
            // 缩容旧 RS，但保证可用副本数不低于最小值
            scaleDownCount := min(
                oldRS.Replicas,                           // 旧 RS 当前副本数
                availableReplicas - minAvailableReplicas, // 不低于最小可用数
            )
            scaleReplicaSet(oldRS, oldRS.Replicas - scaleDownCount)
        }
    }
    
    // 6. 检查更新进度
    if hasProgressDeadline() && isStuck() {
        setCondition("Progressing", "False", "ProgressDeadlineExceeded")
    }
}

// maxSurge/maxUnavailable 计算示例
// spec.replicas = 10, maxSurge = 25%, maxUnavailable = 25%
//
// maxSurge = ceil(10 * 0.25) = 3
// maxUnavailable = floor(10 * 0.25) = 2
//
// 更新过程示例:
// 初始: 旧RS=10, 新RS=0
// 步骤1: 旧RS=10, 新RS=3 (maxSurge允许总数13)
// 步骤2: 旧RS=8,  新RS=5 (3个新Pod Ready后缩容2个旧Pod)
// 步骤3: 旧RS=5,  新RS=8
// 步骤4: 旧RS=2,  新RS=10
// 步骤5: 旧RS=0,  新RS=10 (完成)
```

**关键参数详解**

| 参数 | 默认值 | 计算方式 | 影响 |
|------|--------|----------|------|
| `maxSurge` | 25% | `ceil(replicas * %)` 或绝对值 | 控制峰值副本数，影响更新速度 |
| `maxUnavailable` | 25% | `floor(replicas * %)` 或绝对值 | 控制最低可用数，影响服务稳定性 |
| `minReadySeconds` | 0 | 秒数 | Pod Ready 后等待时间，防止快速失败 |
| `progressDeadlineSeconds` | 600 | 秒数 | 更新超时判断，不会自动回滚 |
| `revisionHistoryLimit` | 10 | 整数 | 保留的旧 ReplicaSet 数量 |

**特殊配置组合**

```yaml
# 配置 1: 完全蓝绿部署
maxUnavailable: 0     # 不允许副本减少
maxSurge: 100%        # 允许双倍副本
# 结果: 先创建全部新 Pod，再删除旧 Pod (需要 2x 资源)

# 配置 2: 单 Pod 滚动
maxUnavailable: 0
maxSurge: 1
# 结果: 每次只创建 1 个新 Pod，最慢但最安全

# 配置 3: 快速更新
maxUnavailable: 50%
maxSurge: 50%
# 结果: 大量并发更新，需要资源充足

# 配置 4: 无效配置 (会报错)
maxUnavailable: 0
maxSurge: 0
# 结果: 无法进行更新 (既不能删旧也不能加新)
```

#### 1.1.3 版本回滚机制详解

**ReplicaSet 历史管理**

```bash
# Deployment 通过 ReplicaSet 实现版本管理
# 每次 Pod 模板变更都会创建新的 ReplicaSet

# 查看 ReplicaSet 历史
kubectl get rs -l app=myapp --sort-by=.metadata.creationTimestamp

# 输出示例:
# NAME                  DESIRED   CURRENT   READY   AGE
# myapp-78f9d4c         0         0         0       30m   # revision 1
# myapp-a5b7e21         0         0         0       20m   # revision 2
# myapp-c8d9f3a         3         3         3       5m    # revision 3 (当前)

# ReplicaSet 命名规则: <deployment-name>-<pod-template-hash>
# pod-template-hash 由 Pod 模板内容计算而来，确保唯一性
```

**Revision 追踪原理**

```yaml
# Deployment 在 annotation 中记录变更原因
apiVersion: apps/v1
kind: Deployment
metadata:
  annotations:
    deployment.kubernetes.io/revision: "3"  # 当前 revision
    kubernetes.io/change-cause: "kubectl set image deployment/myapp myapp=myapp:v3"
spec:
  revisionHistoryLimit: 10  # 保留最近 10 个 ReplicaSet
  
# 每个 ReplicaSet 也有 revision annotation
---
apiVersion: apps/v1
kind: ReplicaSet
metadata:
  name: myapp-c8d9f3a
  annotations:
    deployment.kubernetes.io/revision: "3"
  labels:
    pod-template-hash: c8d9f3a
  ownerReferences:
  - apiVersion: apps/v1
    kind: Deployment
    name: myapp
    controller: true
```

**回滚流程详解**

```bash
# 1. 查看历史版本
kubectl rollout history deployment myapp
# REVISION  CHANGE-CAUSE
# 1         kubectl create --filename=deployment.yaml --record=true
# 2         kubectl set image deployment/myapp myapp=myapp:v2
# 3         kubectl set image deployment/myapp myapp=myapp:v3

# 2. 查看特定版本详情
kubectl rollout history deployment myapp --revision=2
# 显示该 revision 的 Pod 模板

# 3. 回滚到上一版本
kubectl rollout undo deployment myapp
# 实际操作: 
#   - 找到 revision=2 的 ReplicaSet (myapp-a5b7e21)
#   - 更新 Deployment 的 Pod 模板为该 RS 的模板
#   - 触发滚动更新: 扩容 RS myapp-a5b7e21, 缩容 RS myapp-c8d9f3a
#   - 创建新的 revision=4 (内容同 revision=2)

# 4. 回滚到指定版本
kubectl rollout undo deployment myapp --to-revision=1

# 5. 回滚后的 revision 变化
kubectl rollout history deployment myapp
# REVISION  CHANGE-CAUSE
# 2         kubectl set image deployment/myapp myapp=myapp:v2
# 3         kubectl set image deployment/myapp myapp=myapp:v3
# 4         kubectl create --filename=deployment.yaml --record=true
# 注意: revision 1 消失了，因为被回滚后变成了 revision 4
```

**revisionHistoryLimit 清理机制**

```yaml
spec:
  revisionHistoryLimit: 2  # 只保留最近 2 个旧版本

# 清理逻辑:
# - 保留当前活跃的 ReplicaSet (replicas > 0)
# - 按 revision 从新到旧排序，保留前 N 个旧的 RS
# - 删除更老的 ReplicaSet

# 示例: revisionHistoryLimit=2, 当前 revision=5
# 保留: revision 5 (active), revision 4, revision 3
# 删除: revision 2, revision 1

# 设置为 0 会删除所有旧 RS，丧失回滚能力
```

#### 1.1.4 金丝雀发布 (Canary Deployment)

**方案 1: 手动金丝雀 (使用 Pause)**

```bash
# 1. 更新镜像并立即暂停
kubectl set image deployment myapp myapp=myapp:v2
kubectl rollout pause deployment myapp

# 此时状态: 部分 Pod 更新为 v2 (根据 maxSurge)
kubectl get pods -l app=myapp -o custom-columns=NAME:.metadata.name,IMAGE:.spec.containers[0].image

# 2. 验证金丝雀 Pod (v2) 的健康状态
# - 查看日志、监控指标
# - 进行流量测试

# 3. 确认无问题后继续更新
kubectl rollout resume deployment myapp

# 4. 如果发现问题立即回滚
kubectl rollout undo deployment myapp
```

**方案 2: 两套 Deployment (推荐生产环境)**

```yaml
# 主 Deployment (稳定版本)
---
apiVersion: apps/v1
kind: Deployment
metadata:
  name: myapp-stable
spec:
  replicas: 9
  selector:
    matchLabels:
      app: myapp
      version: stable
  template:
    metadata:
      labels:
        app: myapp
        version: stable
    spec:
      containers:
      - name: myapp
        image: myapp:v1

---
# 金丝雀 Deployment (新版本)
apiVersion: apps/v1
kind: Deployment
metadata:
  name: myapp-canary
spec:
  replicas: 1  # 10% 流量
  selector:
    matchLabels:
      app: myapp
      version: canary
  template:
    metadata:
      labels:
        app: myapp
        version: canary
    spec:
      containers:
      - name: myapp
        image: myapp:v2

---
# Service 选择两个 Deployment 的 Pod
apiVersion: v1
kind: Service
metadata:
  name: myapp
spec:
  selector:
    app: myapp  # 同时匹配 stable 和 canary
  ports:
  - port: 80

# 流量分配: stable=90%, canary=10%
# 逐步调整副本数实现流量切换
```

**金丝雀流量切换流程**

```bash
# 阶段 1: 10% 流量到金丝雀
kubectl scale deployment myapp-stable --replicas=9
kubectl scale deployment myapp-canary --replicas=1

# 阶段 2: 50% 流量到金丝雀
kubectl scale deployment myapp-stable --replicas=5
kubectl scale deployment myapp-canary --replicas=5

# 阶段 3: 全部切换到金丝雀
kubectl scale deployment myapp-stable --replicas=0
kubectl scale deployment myapp-canary --replicas=10

# 阶段 4: 清理旧版本 (可选)
kubectl delete deployment myapp-stable

# 如果发现问题快速回滚
kubectl scale deployment myapp-stable --replicas=10
kubectl scale deployment myapp-canary --replicas=0
```

#### 1.1.5 蓝绿部署 (Blue-Green Deployment)

**实现方式: Service Label Selector 切换**

```yaml
# 蓝环境 (当前生产版本)
---
apiVersion: apps/v1
kind: Deployment
metadata:
  name: myapp-blue
spec:
  replicas: 10
  selector:
    matchLabels:
      app: myapp
      env: blue
  template:
    metadata:
      labels:
        app: myapp
        env: blue
    spec:
      containers:
      - name: myapp
        image: myapp:v1

---
# 绿环境 (新版本，预先部署)
apiVersion: apps/v1
kind: Deployment
metadata:
  name: myapp-green
spec:
  replicas: 10
  selector:
    matchLabels:
      app: myapp
      env: green
  template:
    metadata:
      labels:
        app: myapp
        env: green
    spec:
      containers:
      - name: myapp
        image: myapp:v2

---
# Service 初始指向蓝环境
apiVersion: v1
kind: Service
metadata:
  name: myapp
spec:
  selector:
    app: myapp
    env: blue  # 当前指向蓝环境
  ports:
  - port: 80
```

**蓝绿切换流程**

```bash
# 1. 部署绿环境 (不接收流量)
kubectl apply -f myapp-green-deployment.yaml
kubectl rollout status deployment myapp-green

# 2. 验证绿环境健康状态
# - 创建临时 Service 进行内部测试
kubectl expose deployment myapp-green --name=myapp-green-test --port=80

# 3. 确认无问题后，切换 Service 到绿环境 (零停机切换)
kubectl patch service myapp -p '{"spec":{"selector":{"env":"green"}}}'

# 此时流量立即从蓝环境切换到绿环境

# 4. 观察绿环境运行状态
# - 查看监控、日志、错误率

# 5. 如果发现问题，立即切回蓝环境 (秒级回滚)
kubectl patch service myapp -p '{"spec":{"selector":{"env":"blue"}}}'

# 6. 确认稳定后，清理蓝环境
kubectl delete deployment myapp-blue

# 7. 下次发版时，蓝绿角色互换
# 将新版本部署到 myapp-blue (原蓝环境)
# 从 green 切换到 blue
```

**蓝绿部署优缺点**

| 优点 | 缺点 |
|------|------|
| 零停机切换 (毫秒级) | 需要 2x 资源 (两套环境并存) |
| 秒级回滚能力 | 数据库迁移需特殊处理 |
| 新版本充分验证后切换 | 状态同步复杂 (需外部存储) |
| 流量切换可控 | 成本较高 |

### 1.2 常见问题现象

#### 1.2.1 Deployment 创建/更新问题

| 现象 | 报错信息 | 报错来源 | 查看方式 |
|------|----------|----------|----------|
| 创建失败 | `admission webhook denied` | API Server | kubectl apply 输出 |
| 资源配额超限 | `exceeded quota` | API Server | kubectl apply 输出 |
| 副本数不足 | `replicas not ready` | kubectl | `kubectl get deployment` |
| 滚动更新卡住 | `waiting for rollout to finish` | kubectl | `kubectl rollout status` |
| 更新超时 | `deployment exceeded progress deadline` | Events | `kubectl describe deployment` |
| 回滚失败 | `rollback failed` | kubectl | kubectl rollout 输出 |
| 镜像更新未生效 | Pod 仍使用旧镜像 | kubectl | `kubectl get pods -o yaml` |

#### 1.2.2 ReplicaSet 问题

| 现象 | 报错信息 | 报错来源 | 查看方式 |
|------|----------|----------|----------|
| RS 副本数不增 | 新 RS replicas 为 0 | kubectl | `kubectl get rs` |
| 旧 RS 未清理 | revisionHistoryLimit 超限 | kubectl | `kubectl get rs` |
| RS 卡在 0 副本 | 新 RS 无法创建 Pod | kubectl | `kubectl describe rs` |
| 多个 RS 活跃 | 滚动更新未完成 | kubectl | `kubectl get rs` |

#### 1.2.3 Pod 调度/运行问题

| 现象 | 报错信息 | 报错来源 | 查看方式 |
|------|----------|----------|----------|
| Pod Pending | `Insufficient cpu/memory` | Pod Events | `kubectl describe pod` |
| Pod CrashLoop | `CrashLoopBackOff` | kubectl | `kubectl get pods` |
| Pod 不健康 | `Readiness probe failed` | Pod Events | `kubectl describe pod` |
| Pod 反复重启 | restartCount 持续增加 | kubectl | `kubectl get pods` |
| Init 容器失败 | `Init:CrashLoopBackOff` | kubectl | `kubectl get pods` |

### 1.3 报错查看方式汇总

```bash
# 查看 Deployment 状态
kubectl get deployment <name> -o wide
kubectl describe deployment <name>

# 查看 Deployment 条件状态
kubectl get deployment <name> -o jsonpath='{.status.conditions[*].type}'

# 查看 Deployment 事件
kubectl get events --field-selector=involvedObject.name=<deployment-name> --sort-by='.lastTimestamp'

# 查看 ReplicaSet 状态
kubectl get rs -l app=<label> --sort-by='.metadata.creationTimestamp'
kubectl describe rs <rs-name>

# 查看 Pod 状态
kubectl get pods -l app=<label> -o wide --show-labels
kubectl describe pod <pod-name>

# 查看滚动更新状态
kubectl rollout status deployment <name> --timeout=5m
kubectl rollout history deployment <name>

# 查看 Deployment YAML（含 status）
kubectl get deployment <name> -o yaml
```

### 1.4 影响面分析

| 问题类型 | 直接影响 | 间接影响 | 影响范围 |
|----------|----------|----------|----------|
| 创建失败 | 服务无法部署 | 依赖服务不可用 | 应用级 |
| 滚动更新卡住 | 新旧版本并存 | 状态不一致、流量异常 | 应用级 |
| 副本数不足 | 服务能力下降 | 响应延迟增加 | 性能 |
| Pod 反复重启 | 服务不稳定 | 请求失败、数据不一致 | 可用性 |
| progressDeadline 超时 | 更新标记失败 | 告警触发、人工干预 | 运维 |

---

## 第二部分：排查原理与方法

### 2.1 排查决策树

```
Deployment 问题
    │
    ├─► 检查 Deployment 状态
    │       │
    │       ├─► Deployment 不存在 ──► 检查创建命令和 YAML
    │       │
    │       ├─► Available=False ──► 检查 Pod 状态
    │       │
    │       └─► Progressing=False ──► 检查更新策略和资源
    │
    ├─► 检查 ReplicaSet 状态
    │       │
    │       ├─► 新 RS replicas=0 ──► 检查 Webhook/Admission
    │       │
    │       ├─► 新 RS 创建 Pod 失败 ──► 检查 Pod Events
    │       │
    │       └─► 旧 RS 未缩容 ──► 检查 maxUnavailable 和 Pod 健康
    │
    ├─► 检查 Pod 状态
    │       │
    │       ├─► Pending ──► 检查资源、调度、亲和性
    │       │       │
    │       │       ├─► Insufficient resources ──► 扩容节点或减少 requests
    │       │       ├─► Node selector mismatch ──► 修正标签或选择器
    │       │       ├─► Taints not tolerated ──► 添加 tolerations
    │       │       └─► PVC not bound ──► 检查存储类和 PV
    │       │
    │       ├─► ImagePullBackOff ──► 检查镜像和凭证
    │       │
    │       ├─► CrashLoopBackOff ──► 检查容器日志和配置
    │       │       │
    │       │       ├─► 应用启动失败 ──► 修复应用代码/配置
    │       │       ├─► 健康检查失败 ──► 调整探针配置
    │       │       └─► 依赖服务不可用 ──► 添加 init container 或重试
    │       │
    │       ├─► Running but Not Ready ──► 检查 readinessProbe
    │       │
    │       └─► OOMKilled ──► 增加内存限制
    │
    └─► 检查滚动更新
            │
            ├─► 更新卡住 ──► 检查 progressDeadlineSeconds
            │
            ├─► 新旧 Pod 并存过久 ──► 检查 minReadySeconds
            │
            └─► 更新后回滚 ──► 检查新版本问题
```

### 2.2 排查命令集

#### 2.2.1 Deployment 状态检查

```bash
# 查看 Deployment 概览
kubectl get deployment <name> -o wide

# 查看详细状态条件
kubectl get deployment <name> -o jsonpath='{range .status.conditions[*]}{.type}: {.status} - {.reason}{"\n"}{end}'

# 预期输出：
# Available: True - MinimumReplicasAvailable
# Progressing: True - NewReplicaSetAvailable

# 查看副本状态
kubectl get deployment <name> -o jsonpath='
  desired: {.spec.replicas}
  current: {.status.replicas}
  ready: {.status.readyReplicas}
  available: {.status.availableReplicas}
  updated: {.status.updatedReplicas}
'

# 查看更新策略
kubectl get deployment <name> -o jsonpath='{.spec.strategy}'

# 查看 revision 历史
kubectl rollout history deployment <name>
kubectl rollout history deployment <name> --revision=<n>
```

#### 2.2.2 ReplicaSet 检查

```bash
# 列出所有 RS，按时间排序
kubectl get rs -l app=<label> --sort-by='.metadata.creationTimestamp'

# 查看 RS 详细信息
kubectl describe rs <rs-name>

# 查看当前活跃的 RS
kubectl get rs -l app=<label> -o jsonpath='{range .items[?(@.spec.replicas>0)]}{.metadata.name}: {.spec.replicas}/{.status.readyReplicas}{"\n"}{end}'

# 检查 RS 的 Pod 模板哈希
kubectl get rs -l app=<label> -o jsonpath='{range .items[*]}{.metadata.name}: {.metadata.labels.pod-template-hash}{"\n"}{end}'
```

#### 2.2.3 Pod 状态检查

```bash
# 查看 Pod 列表和状态
kubectl get pods -l app=<label> -o wide

# 查看 Pod 事件
kubectl describe pod <pod-name> | tail -20

# 查看容器日志
kubectl logs <pod-name> --tail=100
kubectl logs <pod-name> --previous  # 崩溃前的日志
kubectl logs <pod-name> -c <init-container-name>  # init 容器日志

# 查看 Pod 资源使用
kubectl top pod <pod-name> --containers

# 检查 Pod 的 owner reference
kubectl get pod <pod-name> -o jsonpath='{.metadata.ownerReferences[*].name}'
```

#### 2.2.4 滚动更新监控

```bash
# 实时监控更新状态
kubectl rollout status deployment <name> -w

# 查看更新事件
kubectl get events --field-selector=involvedObject.kind=Deployment,involvedObject.name=<name> -w

# 监控 Pod 变化
kubectl get pods -l app=<label> -w

# 检查更新是否卡住
kubectl get deployment <name> -o jsonpath='{.status.conditions[?(@.type=="Progressing")].status}'
```

### 2.3 排查注意事项

| 注意项 | 说明 | 风险 |
|--------|------|------|
| 不要随意删除 RS | RS 保留用于回滚 | 丢失回滚能力 |
| 谨慎调整 progressDeadlineSeconds | 过短会误报失败 | 触发不必要告警 |
| 注意 minReadySeconds | 影响更新速度 | 更新时间过长 |
| 检查 PDB | 可能阻止 Pod 删除 | 更新卡住 |
| 注意资源配额 | 新 Pod 可能超限 | 创建失败 |

---

## 第三部分：解决方案与风险控制

### 3.1 滚动更新卡住

#### 3.1.1 诊断原因

```bash
# 检查 Deployment 条件
kubectl get deployment <name> -o jsonpath='{.status.conditions[?(@.type=="Progressing")]}'

# 检查是否是 progressDeadline 超时
# reason: ProgressDeadlineExceeded 表示超时

# 检查新 RS 的 Pod 状态
NEW_RS=$(kubectl get rs -l app=<label> --sort-by='.metadata.creationTimestamp' -o jsonpath='{.items[-1].metadata.name}')
kubectl describe rs $NEW_RS

# 检查 Pod 为什么没有 Ready
kubectl get pods -l app=<label> -o jsonpath='{range .items[?(@.status.phase!="Running")]}{.metadata.name}: {.status.phase}{"\n"}{end}'
```

#### 3.1.2 解决方案

```bash
# 方案 1：暂停更新，排查问题
kubectl rollout pause deployment <name>
# 排查并修复问题后恢复
kubectl rollout resume deployment <name>

# 方案 2：回滚到上一版本
kubectl rollout undo deployment <name>

# 方案 3：回滚到特定版本
kubectl rollout history deployment <name>
kubectl rollout undo deployment <name> --to-revision=<n>

# 方案 4：增加 progressDeadlineSeconds（如果只是启动慢）
kubectl patch deployment <name> -p '{"spec":{"progressDeadlineSeconds":900}}'

# 方案 5：调整更新策略（减少并发）
kubectl patch deployment <name> -p '{
  "spec": {
    "strategy": {
      "type": "RollingUpdate",
      "rollingUpdate": {
        "maxUnavailable": 0,
        "maxSurge": 1
      }
    }
  }
}'

# 方案 6：强制重建所有 Pod (Recreate 策略)
kubectl patch deployment <name> -p '{"spec":{"strategy":{"type":"Recreate"}}}'
# 注意：会导致服务中断
```

### 3.2 Pod 健康检查失败导致更新卡住

#### 3.2.1 诊断

```bash
# 检查 readinessProbe 配置
kubectl get deployment <name> -o jsonpath='{.spec.template.spec.containers[*].readinessProbe}' | jq

# 检查 Pod 的 Ready 条件
kubectl get pod <pod-name> -o jsonpath='{.status.conditions[?(@.type=="Ready")]}'

# 检查探针失败详情
kubectl describe pod <pod-name> | grep -A5 "Readiness"
```

#### 3.2.2 解决方案

```bash
# 调整探针参数
kubectl patch deployment <name> --type='json' -p='[
  {
    "op": "replace",
    "path": "/spec/template/spec/containers/0/readinessProbe",
    "value": {
      "httpGet": {
        "path": "/health",
        "port": 8080
      },
      "initialDelaySeconds": 30,
      "periodSeconds": 10,
      "timeoutSeconds": 5,
      "successThreshold": 1,
      "failureThreshold": 3
    }
  }
]'

# 或者临时移除探针（仅用于紧急恢复）
kubectl patch deployment <name> --type='json' -p='[
  {"op": "remove", "path": "/spec/template/spec/containers/0/readinessProbe"}
]'
```

### 3.3 资源不足导致 Pod Pending

#### 3.3.1 解决方案

```bash
# 方案 1：减少资源请求
kubectl patch deployment <name> --type='json' -p='[
  {
    "op": "replace",
    "path": "/spec/template/spec/containers/0/resources/requests",
    "value": {"cpu": "100m", "memory": "128Mi"}
  }
]'

# 方案 2：清理无用资源
kubectl delete pods --field-selector=status.phase=Failed -A
kubectl delete jobs --field-selector=status.successful=1 -A

# 方案 3：使用优先级抢占
cat << EOF | kubectl apply -f -
apiVersion: scheduling.k8s.io/v1
kind: PriorityClass
metadata:
  name: high-priority
value: 1000000
globalDefault: false
preemptionPolicy: PreemptLowerPriority
EOF

kubectl patch deployment <name> -p '{"spec":{"template":{"spec":{"priorityClassName":"high-priority"}}}}'
```

### 3.4 镜像拉取失败

#### 3.4.1 解决方案

```bash
# 检查镜像名称是否正确
kubectl get deployment <name> -o jsonpath='{.spec.template.spec.containers[*].image}'

# 创建镜像拉取凭证
kubectl create secret docker-registry regcred \
  --docker-server=<registry> \
  --docker-username=<user> \
  --docker-password=<password> \
  -n <namespace>

# 关联凭证到 Deployment
kubectl patch deployment <name> -p '{"spec":{"template":{"spec":{"imagePullSecrets":[{"name":"regcred"}]}}}}'

# 强制重新拉取镜像
kubectl patch deployment <name> -p '{"spec":{"template":{"spec":{"containers":[{"name":"<container>","imagePullPolicy":"Always"}]}}}}'
kubectl rollout restart deployment <name>
```

### 3.5 常用更新策略配置

```yaml
# 保守策略：最小化风险
spec:
  strategy:
    type: RollingUpdate
    rollingUpdate:
      maxUnavailable: 0       # 始终保持完整副本数
      maxSurge: 1             # 每次只多创建 1 个
  minReadySeconds: 30         # Pod Ready 后等待 30s 才继续
  progressDeadlineSeconds: 600

# 快速策略：快速完成更新
spec:
  strategy:
    type: RollingUpdate
    rollingUpdate:
      maxUnavailable: 25%
      maxSurge: 50%
  minReadySeconds: 5
  progressDeadlineSeconds: 300

# 蓝绿部署：先创建所有新 Pod
spec:
  strategy:
    type: RollingUpdate
    rollingUpdate:
      maxUnavailable: 0
      maxSurge: 100%          # 先创建同等数量新 Pod
```

### 3.6 安全生产风险提示

| 操作 | 风险等级 | 风险描述 | 防护措施 |
|------|----------|----------|----------|
| 回滚 (rollout undo) | 中 | 立即触发回滚 | 确认目标版本稳定 |
| 修改 strategy | 中 | 影响后续更新行为 | 了解策略含义 |
| 删除 RS | 高 | 丢失回滚能力 | 通常不要手动删除 |
| Recreate 策略 | 高 | 服务完全中断 | 仅用于可中断服务 |
| 强制删除 Pod | 中 | 可能丢失数据 | 确认 Pod 无状态 |
| 移除探针 | 中 | 失去健康检测 | 仅临时措施 |
| 调整 progressDeadline | 低 | 影响超时判断 | 根据实际启动时间设置 |

```
⚠️  安全生产风险提示：
1. 回滚前确认目标版本是稳定的，检查 rollout history
2. 大规模 Deployment 更新需要足够的集群资源余量
3. 修改 maxUnavailable=0 + maxSurge=0 会导致更新无法进行
4. progressDeadlineSeconds 只影响状态报告，不会自动回滚
5. 如果配置了 PDB，确保 PDB 允许足够的 Pod 不可用
6. 有状态应用的 Deployment 更新需要特别注意数据一致性
```

---

## 附录

### A. Deployment 状态字段说明

| 字段 | 说明 | 健康值 |
|------|------|--------|
| replicas | 当前副本总数 | 等于或接近 spec.replicas |
| readyReplicas | 就绪副本数 | 等于 spec.replicas |
| availableReplicas | 可用副本数 | 等于 spec.replicas |
| unavailableReplicas | 不可用副本数 | 0 |
| updatedReplicas | 已更新副本数 | 等于 spec.replicas |
| observedGeneration | 已观察到的版本 | 等于 metadata.generation |

### B. Deployment 条件状态

| 条件 | 状态 | 原因 | 说明 |
|------|------|------|------|
| Available | True | MinimumReplicasAvailable | 至少有最小副本数可用 |
| Available | False | MinimumReplicasUnavailable | 可用副本数不足 |
| Progressing | True | NewReplicaSetAvailable | 新 RS 已就绪 |
| Progressing | True | ReplicaSetUpdated | RS 正在更新 |
| Progressing | False | ProgressDeadlineExceeded | 更新超时 |
| ReplicaFailure | True | FailedCreate | Pod 创建失败 |

### C. 常用命令速查

```bash
# 更新镜像
kubectl set image deployment/<name> <container>=<image>:<tag>

# 扩缩容
kubectl scale deployment <name> --replicas=<n>

# 查看状态
kubectl rollout status deployment <name>

# 查看历史
kubectl rollout history deployment <name>

# 回滚
kubectl rollout undo deployment <name>
kubectl rollout undo deployment <name> --to-revision=<n>

# 暂停/恢复
kubectl rollout pause deployment <name>
kubectl rollout resume deployment <name>

# 重启所有 Pod
kubectl rollout restart deployment <name>

# 查看更新原因
kubectl describe deployment <name> | grep -A10 Conditions
```

### D. 排查清单

- [ ] Deployment 状态条件 (Available, Progressing)
- [ ] ReplicaSet 副本数和状态
- [ ] Pod 状态 (Pending, CrashLoop, Ready)
- [ ] Pod 事件和日志
- [ ] 资源配额和限制
- [ ] 镜像和镜像拉取凭证
- [ ] 更新策略参数
- [ ] 探针配置
- [ ] PDB 约束
- [ ] 节点资源可用性


### 1.5 专家级故障矩阵

#### 1.5.1 按更新阶段分类的故障场景

**阶段 1: 更新触发 (0-5s)**

| 故障现象 | 根因分析 | 排查路径 | 典型场景 |
|----------|----------|----------|----------|
| Webhook 拒绝更新 | Admission Controller 校验失败 | `kubectl get validatingwebhookconfigurations` 检查规则 | Istio/OPA 策略阻止 |
| 资源配额超限 | ResourceQuota 限制创建 | `kubectl describe quota -n <ns>` | 命名空间资源已满 |
| 镜像策略违规 | ImagePolicyWebhook 拒绝 | 检查镜像仓库白名单 | 使用未授权镜像 |
| RBAC 权限不足 | ServiceAccount 无权限 | `kubectl auth can-i update deployment` | CI/CD 账号权限不足 |

**阶段 2: ReplicaSet 创建 (5-10s)**

| 故障现象 | 根因分析 | 排查路径 | 典型场景 |
|----------|----------|----------|----------|
| 新 RS replicas=0 | Deployment Controller 计算错误 | 检查 `maxSurge`/`maxUnavailable` 配置 | 配置为 `0/0` 导致无法更新 |
| RS 创建失败 | API Server 拒绝 | `kubectl get events` 查看失败原因 | 名称冲突、标签错误 |
| Pod 模板无效 | 容器镜像/配置错误 | `kubectl describe rs <rs>` 查看错误 | 镜像不存在、参数错误 |
| Owner Reference 错误 | Deployment UID 变更 | 检查 Deployment 是否被重建 | 删除重建导致 RS 孤立 |

**阶段 3: Pod 创建与调度 (10-60s)**

| 故障现象 | 根因分析 | 排查路径 | 典型场景 |
|----------|----------|----------|----------|
| Pod Pending | 资源不足/节点不匹配 | `kubectl describe pod` 查看调度失败原因 | CPU/内存不足、污点阻止 |
| PVC 未绑定 | 存储类/PV 不可用 | `kubectl get pvc` 检查 PVC 状态 | 动态供应失败 |
| 节点亲和性不满足 | NodeSelector/Affinity 配置错误 | 检查节点标签与 Pod 要求 | 标签拼写错误 |
| 污点阻止调度 | Node Taint 未容忍 | `kubectl get nodes -o json \| jq '.items[].spec.taints'` | GPU 节点需要 toleration |

**阶段 4: 容器启动 (1-5min)**

| 故障现象 | 根因分析 | 排查路径 | 典型场景 |
|----------|----------|----------|----------|
| ImagePullBackOff | 镜像拉取失败 | `kubectl describe pod` 查看镜像拉取事件 | 凭证错误、镜像不存在 |
| CrashLoopBackOff | 容器启动即崩溃 | `kubectl logs <pod> --previous` 查看崩溃日志 | 配置错误、依赖不可用 |
| Init 容器失败 | Init Container 退出非 0 | `kubectl logs <pod> -c <init-container>` | 数据库连接失败 |
| PostStart Hook 失败 | 生命周期钩子执行失败 | 查看 Pod Events 中的 FailedPostStartHook | 脚本执行超时/错误 |

**阶段 5: 健康检查 (minReadySeconds 内)**

| 故障现象 | 根因分析 | 排查路径 | 典型场景 |
|----------|----------|----------|----------|
| Readiness Probe 失败 | 应用未就绪 | `kubectl describe pod` 查看探针失败详情 | 启动时间超过 initialDelaySeconds |
| Liveness Probe 失败 | 应用响应超时 | 检查探针超时配置 | 数据库慢查询阻塞主线程 |
| Pod Ready 但流量异常 | Service Endpoints 未更新 | `kubectl get endpoints <svc>` | kube-proxy 延迟 |
| minReadySeconds 未满足 | Pod 在等待期内重启 | 查看 Pod 重启历史 | 内存泄漏导致 OOM |

**阶段 6: 旧 Pod 缩容 (5-10min)**

| 故障现象 | 根因分析 | 排查路径 | 典型场景 |
|----------|----------|----------|----------|
| 旧 Pod 不缩容 | 新 Pod 未达到 Ready | 检查新 Pod 健康状态 | 健康检查配置错误 |
| Pod Terminating 卡住 | PreStop Hook 超时 | `kubectl describe pod` 查看终止事件 | PreStop 脚本死循环 |
| PDB 阻止删除 | PodDisruptionBudget 限制 | `kubectl get pdb` 检查预算 | minAvailable 设置过高 |
| 长连接未断开 | terminationGracePeriodSeconds 不足 | 增加优雅终止时间 | WebSocket 连接未关闭 |

**阶段 7: 更新完成校验 (10-15min)**

| 故障现象 | 根因分析 | 排查路径 | 典型场景 |
|----------|----------|----------|----------|
| progressDeadlineExceeded | 更新超时 | 检查 `progressDeadlineSeconds` 配置 | 启动时间超过 10 分钟 |
| 新旧版本持续并存 | 滚动更新卡在中间状态 | 查看 Deployment Conditions | 健康检查间歇性失败 |
| observedGeneration 不匹配 | Deployment 规范再次变更 | 比对 `metadata.generation` 和 `status.observedGeneration` | 更新过程中再次触发更新 |
| 副本数不一致 | ReplicaSet 控制器异常 | 检查 kube-controller-manager 日志 | 控制器重启导致状态不同步 |

#### 1.5.2 复合故障场景

**场景 1: 资源争抢导致滚动更新雪崩**

```
触发条件:
  - 集群资源使用率 > 80%
  - 多个 Deployment 同时更新
  - maxSurge > 0 导致峰值资源需求增加

故障链:
  1. Deployment A 创建 surge Pod 消耗剩余资源
  2. Deployment B 的新 Pod 无法调度 → Pending
  3. Deployment B 的旧 Pod 因 maxUnavailable 无法删除
  4. 触发 progressDeadlineExceeded，更新失败
  5. 人工介入清理 Pending Pod，影响服务稳定性

排查路径:
  kubectl top nodes  # 查看节点资源使用
  kubectl get pods --field-selector=status.phase=Pending -A  # 查看所有 Pending Pod
  kubectl describe pod <pending-pod> | grep "Insufficient"

解决方案:
  - 降低 maxSurge 或增加集群资源
  - 使用 PriorityClass 控制更新优先级
  - 分批次更新 Deployment
```

**场景 2: PDB + 节点维护导致更新阻塞**

```
触发条件:
  - 配置了严格的 PodDisruptionBudget (minAvailable=100%)
  - 部分节点进入维护模式 (kubectl drain)
  - Deployment 执行滚动更新

故障链:
  1. 滚动更新尝试删除旧 Pod
  2. PDB 检查发现不满足 minAvailable 条件 → 阻止删除
  3. 新 Pod 创建但旧 Pod 无法删除 → 总副本数超过预期
  4. 节点维护等待 Pod 驱逐，但 PDB 阻止
  5. 更新卡住，节点也无法维护

排查路径:
  kubectl get pdb -A  # 检查所有 PDB
  kubectl describe pdb <pdb-name>  # 查看 DisruptionsAllowed
  kubectl get pods -o wide | grep <node>  # 查看节点上的 Pod

解决方案:
  - 临时调整 PDB: kubectl patch pdb <pdb> -p '{"spec":{"minAvailable":1}}'
  - 或先完成节点维护，再更新 Deployment
```

**场景 3: 配置漂移导致回滚失败**

```
触发条件:
  - 通过 kubectl edit 直接修改 ConfigMap/Secret
  - Deployment 回滚到旧版本
  - 旧版本依赖的配置已被删除或修改

故障链:
  1. kubectl rollout undo 回滚 Deployment
  2. 旧版本 Pod 启动，读取 ConfigMap
  3. ConfigMap 内容已变更，导致应用启动失败
  4. Pod CrashLoopBackOff
  5. 回滚失败，新旧版本都不可用

排查路径:
  kubectl rollout history deployment <name> --revision=<n>  # 查看旧版本配置
  kubectl get configmap <cm> -o yaml  # 检查当前 ConfigMap
  kubectl logs <pod> --previous  # 查看崩溃日志

解决方案:
  - 使用 ConfigMap/Secret 版本化命名 (app-config-v1)
  - 在 Deployment 中引用特定版本: configMapRef: name: app-config-v1
  - 保留 ConfigMap 历史版本
```

---

## 第二部分(补充)：深度排查方法

### 2.4 progressDeadlineSeconds 深度解析

#### 2.4.1 超时判断逻辑

```go
// Deployment Controller 中的超时检查逻辑 (简化版)
func checkProgressDeadline(deployment, newRS) {
    // 计算最后更新时间
    lastUpdateTime := getLastProgressTimestamp(deployment)
    
    // 如果没有进展时间记录，使用创建时间
    if lastUpdateTime == nil {
        lastUpdateTime = deployment.CreationTimestamp
    }
    
    // 计算超时时间
    deadline := lastUpdateTime.Add(deployment.Spec.ProgressDeadlineSeconds)
    
    // 检查是否超时
    if now().After(deadline) {
        // 检查是否有新的副本变为可用
        if newRS.Status.AvailableReplicas < deployment.Spec.Replicas {
            setCondition("Progressing", "False", "ProgressDeadlineExceeded")
        }
    }
}

// 触发进展时间更新的事件:
// 1. 新 ReplicaSet 创建
// 2. 新 RS 的副本数增加
// 3. 新 Pod 变为 Ready
// 4. 旧 RS 的副本数减少
```

**关键要点**

```yaml
spec:
  progressDeadlineSeconds: 600  # 默认 10 分钟
  minReadySeconds: 30           # Pod Ready 后等待 30s

# 实际更新超时 = progressDeadlineSeconds
# minReadySeconds 只是延迟 Pod 被视为可用的时间，不影响超时计算

# 例如: replicas=10, maxSurge=50%, maxUnavailable=25%
# 如果每个 Pod 启动需要 2 分钟
# 预计总时间 = 2min * (10/7.5 批次) ≈ 3 分钟
# 应设置 progressDeadlineSeconds >= 180s
```

#### 2.4.2 超时后的行为

```bash
# 超时后 Deployment 状态
kubectl get deployment <name> -o jsonpath='{.status.conditions[?(@.type=="Progressing")]}'

# 输出:
{
  "type": "Progressing",
  "status": "False",
  "reason": "ProgressDeadlineExceeded",
  "message": "ReplicaSet \"myapp-c8d9f3a\" has timed out progressing."
}

# 重要提示:
# 1. 超时不会自动回滚 (需要手动处理)
# 2. 超时后 Deployment Controller 仍会继续尝试调协
# 3. 如果后续 Pod 变为 Ready，状态会自动恢复
```

### 2.5 PodDisruptionBudget 交互深度分析

#### 2.5.1 PDB 检查逻辑

```yaml
# PDB 配置示例
apiVersion: policy/v1
kind: PodDisruptionBudget
metadata:
  name: myapp-pdb
spec:
  minAvailable: 80%  # 至少 80% 副本可用
  # 或者使用 maxUnavailable: 20%
  selector:
    matchLabels:
      app: myapp

# Deployment 滚动更新时的 PDB 检查流程:
# 1. Deployment Controller 尝试删除旧 Pod
# 2. API Server 调用 Eviction API (而非直接删除)
# 3. PDB Controller 检查是否允许驱逐
# 4. 计算: currentAvailable - 1 >= minAvailable
# 5. 如果不满足，返回 429 Too Many Requests
# 6. Deployment Controller 等待并重试
```

**PDB 阻塞滚动更新的诊断**

```bash
# 检查 PDB 状态
kubectl get pdb <pdb-name> -o yaml

# 关键字段:
status:
  currentHealthy: 8      # 当前健康 Pod 数
  desiredHealthy: 8      # 期望健康 Pod 数
  disruptionsAllowed: 0  # 允许中断的 Pod 数 (0 表示不能删除任何 Pod)
  expectedPods: 10       # 总 Pod 数

# 如果 disruptionsAllowed=0，滚动更新会被阻塞

# 查看 PDB 阻止的驱逐请求
kubectl get events --field-selector=reason=EvictionBlocked

# 临时绕过 PDB (紧急情况)
kubectl delete pdb <pdb-name>
# 或直接强制删除 Pod (跳过 Eviction API)
kubectl delete pod <pod> --grace-period=0 --force
```

#### 2.5.2 PDB 与滚动更新策略的冲突

```yaml
# 冲突配置示例
---
apiVersion: apps/v1
kind: Deployment
spec:
  replicas: 10
  strategy:
    rollingUpdate:
      maxUnavailable: 3  # 允许 3 个不可用
      
---
apiVersion: policy/v1
kind: PodDisruptionBudget
spec:
  minAvailable: 9  # 要求至少 9 个可用
  
# 冲突分析:
# - maxUnavailable=3 意味着可能有 7 个可用 Pod
# - minAvailable=9 要求至少 9 个可用
# - 滚动更新会被 PDB 持续阻塞

# 正确配置:
# 方案 1: 调整 maxUnavailable
maxUnavailable: 1  # 最多 1 个不可用 (9 个可用)

# 方案 2: 调整 PDB
minAvailable: 7  # 或 maxUnavailable: 3

# 方案 3: 使用百分比
maxUnavailable: 10%  # 允许 1 个不可用
minAvailable: 80%    # 要求 8 个可用
```

### 2.6 深度排查脚本集

#### 2.6.1 滚动更新时间线重建

```bash
#!/bin/bash
# 文件: deployment-timeline.sh
# 用途: 重建 Deployment 滚动更新的完整时间线

DEPLOY=$1
NS=${2:-default}

echo "=== Deployment Update Timeline ==="
echo "Deployment: $DEPLOY"
echo "Namespace: $NS"
echo

# 1. Deployment 变更历史
echo "--- Deployment Revisions ---"
kubectl rollout history deployment/$DEPLOY -n $NS

# 2. ReplicaSet 创建时间线
echo -e "\n--- ReplicaSet Timeline ---"
kubectl get rs -n $NS -l app=$DEPLOY \
  --sort-by=.metadata.creationTimestamp \
  -o custom-columns=\
NAME:.metadata.name,\
REVISION:.metadata.annotations.deployment\\.kubernetes\\.io/revision,\
CREATED:.metadata.creationTimestamp,\
DESIRED:.spec.replicas,\
CURRENT:.status.replicas,\
READY:.status.readyReplicas

# 3. Pod 创建和状态变更时间线
echo -e "\n--- Pod Events Timeline ---"
kubectl get events -n $NS \
  --field-selector involvedObject.kind=Pod \
  --sort-by='.lastTimestamp' \
  | grep -E "$DEPLOY|NAME"

# 4. Deployment 条件变更历史
echo -e "\n--- Deployment Conditions ---"
kubectl get deployment/$DEPLOY -n $NS -o json | \
  jq -r '.status.conditions[] | "\(.type): \(.status) - \(.reason) (\(.lastUpdateTime))"'

# 5. 计算更新耗时
echo -e "\n--- Update Duration Analysis ---"
START_TIME=$(kubectl get rs -n $NS -l app=$DEPLOY \
  --sort-by=.metadata.creationTimestamp \
  -o jsonpath='{.items[-1].metadata.creationTimestamp}')
  
COMPLETE_TIME=$(kubectl get deployment/$DEPLOY -n $NS \
  -o jsonpath='{.status.conditions[?(@.type=="Progressing")].lastUpdateTime}')

echo "Update started: $START_TIME"
echo "Last progress: $COMPLETE_TIME"

# 6. 检查是否卡住
PROGRESSING=$(kubectl get deployment/$DEPLOY -n $NS \
  -o jsonpath='{.status.conditions[?(@.type=="Progressing")].status}')

if [ "$PROGRESSING" == "False" ]; then
  echo "⚠️  WARNING: Deployment update has FAILED or TIMED OUT"
  REASON=$(kubectl get deployment/$DEPLOY -n $NS \
    -o jsonpath='{.status.conditions[?(@.type=="Progressing")].reason}')
  echo "Reason: $REASON"
fi
```

#### 2.6.2 滚动更新并发分析

```bash
#!/bin/bash
# 文件: deployment-concurrency-analyzer.sh
# 用途: 分析滚动更新的并发行为

DEPLOY=$1
NS=${2:-default}

echo "=== Deployment Concurrency Analysis ==="

# 获取配置
REPLICAS=$(kubectl get deployment/$DEPLOY -n $NS -o jsonpath='{.spec.replicas}')
MAX_SURGE=$(kubectl get deployment/$DEPLOY -n $NS -o jsonpath='{.spec.strategy.rollingUpdate.maxSurge}')
MAX_UNAVAIL=$(kubectl get deployment/$DEPLOY -n $NS -o jsonpath='{.spec.strategy.rollingUpdate.maxUnavailable}')

echo "Replicas: $REPLICAS"
echo "maxSurge: $MAX_SURGE"
echo "maxUnavailable: $MAX_UNAVAIL"
echo

# 计算数值 (处理百分比)
if [[ $MAX_SURGE == *%* ]]; then
  PCT=${MAX_SURGE%\%}
  MAX_SURGE_NUM=$(echo "scale=0; ($REPLICAS * $PCT + 99) / 100" | bc)
else
  MAX_SURGE_NUM=$MAX_SURGE
fi

if [[ $MAX_UNAVAIL == *%* ]]; then
  PCT=${MAX_UNAVAIL%\%}
  MAX_UNAVAIL_NUM=$(echo "scale=0; ($REPLICAS * $PCT) / 100" | bc)
else
  MAX_UNAVAIL_NUM=$MAX_UNAVAIL
fi

echo "Calculated maxSurge: $MAX_SURGE_NUM pods"
echo "Calculated maxUnavailable: $MAX_UNAVAIL_NUM pods"
echo

# 计算更新策略
MAX_TOTAL=$((REPLICAS + MAX_SURGE_NUM))
MIN_AVAILABLE=$((REPLICAS - MAX_UNAVAIL_NUM))

echo "Maximum total pods during update: $MAX_TOTAL"
echo "Minimum available pods: $MIN_AVAILABLE"
echo

# 当前状态
CURRENT=$(kubectl get deployment/$DEPLOY -n $NS -o jsonpath='{.status.replicas}')
READY=$(kubectl get deployment/$DEPLOY -n $NS -o jsonpath='{.status.readyReplicas}')
UPDATED=$(kubectl get deployment/$DEPLOY -n $NS -o jsonpath='{.status.updatedReplicas}')
AVAILABLE=$(kubectl get deployment/$DEPLOY -n $NS -o jsonpath='{.status.availableReplicas}')

echo "Current status:"
echo "  Total pods: $CURRENT"
echo "  Ready pods: $READY"
echo "  Updated pods: $UPDATED"
echo "  Available pods: $AVAILABLE"
echo

# 检查约束
if [ $CURRENT -gt $MAX_TOTAL ]; then
  echo "⚠️  WARNING: Total pods ($CURRENT) exceeds maximum allowed ($MAX_TOTAL)"
fi

if [ $AVAILABLE -lt $MIN_AVAILABLE ]; then
  echo "⚠️  WARNING: Available pods ($AVAILABLE) below minimum required ($MIN_AVAILABLE)"
fi

# 预估更新时间
AVG_POD_START_TIME=120  # 假设每个 Pod 启动需要 2 分钟
CONCURRENT_UPDATES=$(echo "scale=0; $MAX_SURGE_NUM + $MAX_UNAVAIL_NUM" | bc)
BATCHES=$(echo "scale=0; ($REPLICAS + $CONCURRENT_UPDATES - 1) / $CONCURRENT_UPDATES" | bc)
ESTIMATED_TIME=$(echo "scale=0; $BATCHES * $AVG_POD_START_TIME / 60" | bc)

echo -e "\nEstimated update time: ~$ESTIMATED_TIME minutes"
echo "(assuming 2min per pod start time)"
```

---

## 第三部分(补充)：生产案例深度剖析

### 案例 1: progressDeadlineSeconds 超时导致发布中断

**故障背景**

- **集群**: 阿里云 ACK 1.28，50 节点
- **应用**: 电商订单服务 (order-service)
- **配置**: replicas=30, maxSurge=50%, maxUnavailable=0, progressDeadlineSeconds=600

**故障过程**

```
时间线:
14:00:00 - 触发滚动更新 (镜像从 v2.1 更新到 v2.2)
14:00:05 - 创建新 RS (order-service-abc123)
14:00:10 - 第一批 15 个新 Pod 开始创建 (maxSurge=50%)
14:02:30 - 前 10 个 Pod 启动完成，进入 Running 状态
14:02:30 - Readiness Probe 开始检查 (initialDelaySeconds=30s)
14:03:00 - 前 10 个 Pod 通过健康检查，变为 Ready
14:03:00 - 开始创建第二批 5 个 Pod (保持总数不超过 45)
14:05:00 - 第二批 Pod 中有 3 个卡在 CrashLoopBackOff
14:05:00 - 排查发现: 新版本依赖的 Redis 连接参数错误
14:08:00 - 紧急修复配置，重新打包镜像 v2.2.1
14:10:00 - progressDeadlineSeconds 超时 (600s)
14:10:00 - Deployment 状态变为 Progressing=False
14:10:00 - 更新停止，30 个副本中只有 13 个是新版本
```

**详细排查**

```bash
# 1. 检查 Deployment 状态
$ kubectl get deployment order-service -o jsonpath='{.status.conditions[?(@.type=="Progressing")]}'
{
  "type": "Progressing",
  "status": "False",
  "reason": "ProgressDeadlineExceeded",
  "message": "ReplicaSet \"order-service-abc123\" has timed out progressing."
}

# 2. 查看 Pod 状态分布
$ kubectl get pods -l app=order-service -o wide
NAME                              READY   STATUS             RESTARTS   AGE
order-service-abc123-1            1/1     Running            0          10m  # 新版本 v2.2
order-service-abc123-2            1/1     Running            0          10m
...
order-service-abc123-13           1/1     Running            0          7m
order-service-abc123-14           0/1     CrashLoopBackOff   5          5m   # 失败的新版本
order-service-abc123-15           0/1     CrashLoopBackOff   5          5m
order-service-xyz789-1            1/1     Running            0          3h   # 旧版本 v2.1
...
order-service-xyz789-17           1/1     Running            0          3h

# 3. 查看失败 Pod 日志
$ kubectl logs order-service-abc123-14
Error: Cannot connect to Redis at redis-cluster:6380
Configuration: REDIS_HOST=redis-cluster, REDIS_PORT=6380
...

# 4. 对比配置差异
$ kubectl rollout history deployment order-service --revision=2 | grep REDIS
    REDIS_PORT: 6380  # 新版本配置错误

$ kubectl rollout history deployment order-service --revision=1 | grep REDIS
    REDIS_PORT: 6379  # 旧版本配置正确
```

**根因分析**

1. **直接原因**: 新版本镜像的 Redis 端口配置错误 (6380 → 6379)
2. **触发条件**: progressDeadlineSeconds=600s, 更新过程超过 10 分钟
3. **加剧因素**:
   - maxSurge=50% 导致第一批就创建了 15 个 Pod，消耗 5 分钟
   - readinessProbe initialDelaySeconds=30s 延迟了问题发现
   - 修复镜像花费了 5 分钟，但更新已经超时

**修复方案**

```bash
# 方案 1: 立即回滚 (选择此方案)
$ kubectl rollout undo deployment order-service
deployment.apps/order-service rolled back

# 滚动更新立即恢复为旧版本
# 耗时: 约 3 分钟完成回滚

# 方案 2: 修复配置并重新更新
# 1. 修复 ConfigMap
$ kubectl patch configmap order-service-config -p '{"data":{"REDIS_PORT":"6379"}}'

# 2. 更新镜像 (已修复的 v2.2.1)
$ kubectl set image deployment/order-service order-service=order-service:v2.2.1

# 3. 延长超时时间 (避免再次超时)
$ kubectl patch deployment order-service -p '{"spec":{"progressDeadlineSeconds":1200}}'
```

**业务影响**

- **影响时间**: 14:05 - 14:13 (8 分钟)
- **影响范围**: 3 个 Pod 反复崩溃，但因 maxUnavailable=0，旧版本 Pod 未减少
- **服务可用性**: 27/30 副本可用 (90%)，未触发告警阈值 (80%)
- **错误率**: 新版本 Pod 的请求全部失败 (~10% 总流量)

**防护措施**

```yaml
# 1. 增加 progressDeadlineSeconds (给修复更多时间)
spec:
  progressDeadlineSeconds: 1200  # 从 600s 增加到 1200s (20 分钟)

# 2. 调整金丝雀策略 (减少初始爆炸半径)
spec:
  strategy:
    rollingUpdate:
      maxSurge: 1      # 从 50% 降低到 1 个
      maxUnavailable: 0
  # 先创建 1 个 Pod，验证成功后再扩容

# 3. 添加启动探针 (快速失败)
spec:
  containers:
  - name: order-service
    startupProbe:      # 启动探针比 readiness 更快失败
      httpGet:
        path: /health/startup
        port: 8080
      failureThreshold: 3
      periodSeconds: 10  # 30s 内失败 3 次则标记为失败
      
# 4. 使用 Helm 钩子进行配置校验
# 在 Helm Chart 的 pre-upgrade hook 中校验配置
```

### 案例 2: PDB 阻塞导致滚动更新停滞 48 小时

**故障背景**

- **集群**: Google GKE 1.27，100 节点
- **应用**: 推荐算法服务 (recommendation-engine)
- **配置**: replicas=20, PDB minAvailable=19, maxUnavailable=1

**故障过程**

```
时间线:
周一 10:00 - 触发滚动更新 (添加新的特征工程模块)
周一 10:05 - 创建 1 个新 Pod (maxSurge=1)
周一 10:10 - 新 Pod 启动完成，尝试删除旧 Pod
周一 10:10 - PDB 检查: currentHealthy=20, minAvailable=19, 允许删除 1 个
周一 10:10 - 旧 Pod 进入 Terminating 状态
周一 10:15 - 旧 Pod 卡在 Terminating (PreStop Hook 执行缓存刷新)
周一 10:20 - PreStop Hook 超时 (terminationGracePeriodSeconds=300s)
周一 10:20 - Pod 强制终止，但状态未从 Endpoints 移除 (kubelet 网络问题)
周一 10:20 - PDB 检查: currentHealthy=19 (认为被删除的 Pod 仍计入)
周一 10:20 - 尝试删除下一个旧 Pod → PDB 阻止 (19-1=18 < minAvailable)
周一 10:20 - 更新卡住，状态: 新 RS=1, 旧 RS=19, Terminating=1

周一 12:00 - 运维发现更新停滞，开始排查
周一 12:30 - 手动清理 Terminating Pod，更新继续
周二 10:00 - 更新再次卡住 (发现共有 5 个节点 kubelet 网络异常)
周二 14:00 - 修复所有异常节点
周三 10:00 - 更新终于完成 (总耗时 48 小时)
```

**详细排查**

```bash
# 1. 检查 Pod 状态
$ kubectl get pods -l app=recommendation-engine
NAME                                   READY   STATUS        RESTARTS   AGE
recommendation-engine-new-abc-1        1/1     Running       0          2h   # 新版本
recommendation-engine-old-xyz-1        1/1     Running       0          5d   # 旧版本
...
recommendation-engine-old-xyz-19       1/1     Running       0          5d
recommendation-engine-old-xyz-20       0/1     Terminating   0          5d   # 卡住

# 2. 检查 PDB 状态
$ kubectl get pdb recommendation-engine-pdb -o yaml
apiVersion: policy/v1
kind: PodDisruptionBudget
metadata:
  name: recommendation-engine-pdb
spec:
  minAvailable: 19
  selector:
    matchLabels:
      app: recommendation-engine
status:
  currentHealthy: 19       # 当前健康 Pod (不包括 Terminating)
  desiredHealthy: 19
  disruptionsAllowed: 0    # 不允许任何中断!
  expectedPods: 21         # 总 Pod 数 (包括 Terminating)
  observedGeneration: 1

# 问题: disruptionsAllowed=0 导致无法删除更多旧 Pod

# 3. 查看 Terminating Pod 详情
$ kubectl describe pod recommendation-engine-old-xyz-20
...
Events:
  Type     Reason        Age   From               Message
  ----     ------        ----  ----               -------
  Normal   Killing       2h    kubelet            Stopping container recommendation-engine
  Warning  FailedSync    2h    kubelet            Error syncing pod, skipping: failed to "KillPodSandbox"...
  
# PreStop Hook 日志
$ kubectl logs recommendation-engine-old-xyz-20
[INFO] Received SIGTERM, starting graceful shutdown...
[INFO] Flushing cache to Redis (estimated 5 minutes)...
[INFO] Cache flush progress: 20%...
[WARN] Redis connection timeout, retrying...
# 卡在这里，永远无法完成

# 4. 检查节点状态
$ kubectl get nodes -o custom-columns=NAME:.metadata.name,STATUS:.status.conditions[-1].type
NAME           STATUS
gke-node-1     Ready
gke-node-2     Ready
gke-node-3     Ready
gke-node-4     NotReady   # 问题节点!
gke-node-5     NotReady   # 问题节点!

# 5. 检查 kubelet 日志 (在问题节点上)
$ journalctl -u kubelet | tail -50
... E1201 10:20:15 ... Failed to remove pod from network: CNI plugin error
... E1201 10:20:20 ... Pod sandbox stuck in removing state
```

**根因分析**

1. **直接原因**: 节点 kubelet 网络插件故障，导致 Pod 无法正常终止
2. **PDB 计算错误**: Terminating 状态的 Pod 仍被计入 expectedPods，但不计入 currentHealthy
   - expectedPods = 21 (新 1 + 旧 19 + Terminating 1)
   - currentHealthy = 19 (不包括 Terminating)
   - disruptionsAllowed = currentHealthy - minAvailable = 19 - 19 = 0
3. **配置过严**: minAvailable=19/20 (95%) 几乎不允许任何中断
4. **PreStop Hook 缺陷**: 缓存刷新逻辑依赖 Redis，网络故障时会卡死

**修复方案**

```bash
# 紧急修复: 强制删除 Terminating Pod
$ kubectl delete pod recommendation-engine-old-xyz-20 --grace-period=0 --force
Warning: Immediate deletion does not wait for confirmation...
pod "recommendation-engine-old-xyz-20" force deleted

# 手动清理残留的网络资源 (在节点上执行)
$ crictl rmp <pod-id>  # 清理 Pod sandbox
$ ip netns delete <netns-id>  # 清理网络命名空间

# 临时放宽 PDB (允许更新继续)
$ kubectl patch pdb recommendation-engine-pdb -p '{"spec":{"minAvailable":18}}'

# 修复 kubelet 网络问题
$ systemctl restart kubelet
$ systemctl restart containerd

# 长期优化: 调整 PDB 配置
$ kubectl patch pdb recommendation-engine-pdb -p '{"spec":{"minAvailable":"85%"}}'
# minAvailable=85% → 17/20，允许最多 3 个 Pod 不可用
```

**防护措施**

```yaml
# 1. 调整 PDB 配置 (留出缓冲空间)
apiVersion: policy/v1
kind: PodDisruptionBudget
metadata:
  name: recommendation-engine-pdb
spec:
  minAvailable: 85%  # 从 95% 降低到 85% (17/20)
  # 或使用 maxUnavailable: 15% (允许 3 个不可用)
  
# 2. 优化 PreStop Hook (添加超时和降级逻辑)
spec:
  containers:
  - name: recommendation-engine
    lifecycle:
      preStop:
        exec:
          command: ["/bin/sh", "-c", "
            timeout 60s /app/flush-cache.sh || {
              echo 'Cache flush timeout, skipping...';
              exit 0;
            }
          "]
    # 缩短 terminationGracePeriodSeconds
  terminationGracePeriodSeconds: 90  # 从 300s 降低到 90s

# 3. 添加节点健康监控
# 使用 Node Problem Detector 自动标记异常节点
apiVersion: v1
kind: DaemonSet
metadata:
  name: node-problem-detector
...

# 4. 滚动更新策略调整
spec:
  strategy:
    rollingUpdate:
      maxSurge: 2          # 允许同时创建 2 个新 Pod
      maxUnavailable: 1    # 允许 1 个旧 Pod 不可用
  # 配合 PDB minAvailable=85%，确保更新不被阻塞
```

**业务影响**

- **影响时间**: 48 小时 (间歇性停滞)
- **影响范围**: 更新停滞，但服务未中断 (旧版本持续运行)
- **潜在风险**: 新版本特性无法上线，影响推荐效果
- **人力成本**: 3 名 SRE 投入 12 小时排查修复

**经验总结**

1. **PDB 配置要留缓冲**: 不要设置 100% 或 95% 的 minAvailable
2. **PreStop Hook 要有超时**: 使用 `timeout` 命令包裹，避免无限等待
3. **监控 Terminating Pod**: 告警触发条件: Pod Terminating > 5min
4. **定期检查节点健康**: 使用 Node Problem Detector 自动标记异常节点

### 案例 3: 镜像层共享导致滚动更新雪崩

**故障背景**

- **集群**: AWS EKS 1.29，200 节点
- **应用**: 微服务网关 (api-gateway)
- **配置**: replicas=100, maxSurge=50%, maxUnavailable=25%
- **镜像**: 基于 alpine:3.18 (1.5GB 压缩后)

**故障过程**

```
时间线:
09:00:00 - 触发滚动更新 (镜像从 v3.5 更新到 v3.6)
09:00:05 - 第一批 50 个新 Pod 开始创建 (maxSurge=50%)
09:00:10 - 所有 50 个 Pod 同时开始拉取镜像
09:00:10 - 镜像仓库 (ECR) 带宽打满 (10Gbps)
09:02:00 - 前 20 个节点完成镜像拉取，Pod 启动
09:05:00 - 后 30 个节点镜像拉取超时 (timeout: 5min)
09:05:00 - 30 个 Pod 进入 ImagePullBackOff
09:05:00 - 退避算法: 10s, 20s, 40s, 80s, 160s, 300s (最大 5min)
09:10:00 - 重试拉取镜像，再次打满带宽
09:15:00 - 又一批节点拉取超时
09:15:00 - 已成功启动的 20 个新 Pod 通过健康检查
09:15:00 - 开始删除 20 个旧 Pod (maxUnavailable=25)
09:20:00 - 镜像拉取问题级联扩散，更新停滞
09:30:00 - 手动暂停更新，排查根因
10:00:00 - 使用镜像预拉取 DaemonSet 解决问题
11:00:00 - 恢复更新，2 小时后完成
```

**详细排查**

```bash
# 1. 检查 Pod 状态分布
$ kubectl get pods -l app=api-gateway -o wide | grep ImagePullBackOff | wc -l
42  # 42 个 Pod 卡在镜像拉取

# 2. 查看镜像拉取事件
$ kubectl describe pod api-gateway-new-abc-1
Events:
  Type     Reason           Age                  From             Message
  ----     ------           ----                 ----             -------
  Normal   Scheduled        10m                  default-scheduler Successfully assigned default/api-gateway-new-abc-1 to node-42
  Normal   Pulling          10m (x3 over 15m)    kubelet          Pulling image "012345678.dkr.ecr.us-west-2.amazonaws.com/api-gateway:v3.6"
  Warning  Failed           10m (x3 over 15m)    kubelet          Failed to pull image: rpc error: code = DeadlineExceeded desc = failed to pull and unpack image: failed to copy: httpReadSeeker: failed open: unexpected status code 429 Too Many Requests
  Warning  Failed           10m (x3 over 15m)    kubelet          Error: ErrImagePull
  Normal   BackOff          2m (x20 over 10m)    kubelet          Back-off pulling image

# 3. 检查镜像仓库限流
$ aws ecr describe-registry --region us-west-2
{
  "registryId": "012345678",
  "replicationConfiguration": {...},
  "rateLimits": {
    "readRateLimit": "1000 per second",    # 读取限流
    "writeRateLimit": "10 per second",
    "burstReadRateLimit": "1500 per second"  # 突发限流
  }
}

# 4. 计算并发拉取数量
$ kubectl get events -A --field-selector reason=Pulling | grep api-gateway | wc -l
50  # 50 个节点同时拉取镜像

# 镜像大小: 1.5GB * 50 nodes = 75GB 数据
# 时间: 75GB / 10Gbps = 60s (理论值)
# 实际: 因 429 限流和重试，耗时 > 5min

# 5. 检查镜像层共享情况
$ crictl images | grep api-gateway
012345678.dkr.ecr.us-west-2.amazonaws.com/api-gateway   v3.5   abc123   1.5GB
012345678.dkr.ecr.us-west-2.amazonaws.com/api-gateway   v3.6   def456   1.5GB

# 问题: v3.5 和 v3.6 几乎没有共享层 (都基于 alpine:3.18，但重新构建)
$ docker history 012345678.dkr.ecr.us-west-2.amazonaws.com/api-gateway:v3.5
$ docker history 012345678.dkr.ecr.us-west-2.amazonaws.com/api-gateway:v3.6
# 发现: 基础层 SHA256 不同，无法复用缓存
```

**根因分析**

1. **直接原因**: 镜像构建未复用层，导致每次更新都是全新镜像
2. **触发条件**: maxSurge=50% → 50 个节点同时拉取 1.5GB 镜像
3. **加剧因素**:
   - ECR 限流: 1000 req/s, burst 1500 req/s
   - 镜像大小过大: 1.5GB (包含不必要的调试工具)
   - 节点镜像缓存未预热
4. **级联效应**: ImagePullBackOff 导致退避等待，更新停滞

**修复方案**

```bash
# 方案 1: 镜像预拉取 (立即生效)
cat << 'EOF' | kubectl apply -f -
apiVersion: apps/v1
kind: DaemonSet
metadata:
  name: image-pre-puller
spec:
  selector:
    matchLabels:
      app: image-pre-puller
  template:
    metadata:
      labels:
        app: image-pre-puller
    spec:
      initContainers:
      - name: pre-pull
        image: 012345678.dkr.ecr.us-west-2.amazonaws.com/api-gateway:v3.6
        command: ['sh', '-c', 'echo Image pulled successfully']
      containers:
      - name: pause
        image: gcr.io/google_containers/pause:3.1
EOF

# 等待 DaemonSet 在所有节点拉取镜像
$ kubectl rollout status daemonset image-pre-puller

# 恢复 Deployment 更新
$ kubectl rollout resume deployment api-gateway

# 清理 DaemonSet
$ kubectl delete daemonset image-pre-puller
```

**长期优化**

```dockerfile
# 优化 1: 多阶段构建减少镜像大小
FROM golang:1.21-alpine AS builder
WORKDIR /app
COPY . .
RUN go build -ldflags="-s -w" -o api-gateway

FROM alpine:3.18
# 固定基础镜像版本和 SHA，确保层复用
RUN apk add --no-cache ca-certificates
COPY --from=builder /app/api-gateway /usr/local/bin/
ENTRYPOINT ["/usr/local/bin/api-gateway"]

# 优化后镜像大小: 1.5GB → 50MB (减少 30 倍)
```

```yaml
# 优化 2: 调整滚动更新策略
spec:
  strategy:
    rollingUpdate:
      maxSurge: 10%      # 从 50% 降低到 10% (10 个 Pod)
      maxUnavailable: 10%
  # 减少并发拉取，避免限流

# 优化 3: 使用 imagePullPolicy: IfNotPresent
spec:
  containers:
  - name: api-gateway
    image: 012345678.dkr.ecr.us-west-2.amazonaws.com/api-gateway:v3.6
    imagePullPolicy: IfNotPresent  # 复用节点缓存

# 优化 4: 配置镜像拉取并发限制
# 在 kubelet 配置中添加:
apiVersion: kubelet.config.k8s.io/v1beta1
kind: KubeletConfiguration
serializeImagePulls: false        # 允许并行拉取不同镜像
maxParallelImagePulls: 3          # 每个节点最多 3 个并发拉取
registryPullQPS: 5                # 限制镜像仓库请求速率
registryBurst: 10
```

**防护措施**

```bash
# 1. 镜像构建流水线检查
# CI/CD 中添加镜像大小检查
- name: Check image size
  run: |
    SIZE=$(docker images --format "{{.Size}}" myimage:latest | sed 's/MB//')
    if [ "$SIZE" -gt 100 ]; then
      echo "Image size $SIZE MB exceeds limit 100MB"
      exit 1
    fi

# 2. 镜像层复用率检查
- name: Check layer reuse
  run: |
    PREV_LAYERS=$(docker history myimage:prev | wc -l)
    NEW_LAYERS=$(docker history myimage:latest | wc -l)
    SHARED=$(comm -12 <(docker history myimage:prev | awk '{print $1}' | sort) \
                      <(docker history myimage:latest | awk '{print $1}' | sort) | wc -l)
    REUSE_RATE=$(echo "scale=2; $SHARED / $PREV_LAYERS * 100" | bc)
    echo "Layer reuse rate: $REUSE_RATE%"
    if [ $(echo "$REUSE_RATE < 50" | bc) -eq 1 ]; then
      echo "WARNING: Low layer reuse rate"
    fi

# 3. 滚动更新前预热镜像
# 使用 kbld + imgpkg 预加载镜像到节点
$ imgpkg copy -i myimage:v3.6 --to-repo myregistry.com/cache

# 4. 配置镜像仓库缓存代理
# 使用 Harbor/Dragonfly P2P 加速分发
```

**业务影响**

- **影响时间**: 2 小时
- **影响范围**: 更新停滞，42/100 节点镜像拉取失败
- **服务可用性**: 未影响 (旧版本持续运行)
- **成本**: ECR 数据传输费用增加 (75GB * 重试次数)

---

## 附录(补充)

### E. Deployment 巡检清单

#### 每日巡检 (自动化)

```bash
#!/bin/bash
# 文件: deployment-daily-check.sh

echo "=== Deployment Daily Health Check ==="
echo "Date: $(date)"
echo

# 1. 检查所有 Deployment 状态
echo "--- Deployments with Issues ---"
kubectl get deployments -A -o json | jq -r '
  .items[] |
  select(
    .status.replicas != .status.readyReplicas or
    .status.replicas != .status.availableReplicas or
    (.status.conditions[] | select(.type=="Available" and .status=="False"))
  ) |
  "\(.metadata.namespace)/\(.metadata.name): \(.status.replicas) desired, \(.status.readyReplicas) ready, \(.status.availableReplicas) available"
'

# 2. 检查正在更新的 Deployment
echo -e "\n--- Deployments in Progress ---"
kubectl get deployments -A -o json | jq -r '
  .items[] |
  select(.status.updatedReplicas != .status.replicas) |
  "\(.metadata.namespace)/\(.metadata.name): \(.status.updatedReplicas)/\(.status.replicas) updated"
'

# 3. 检查超时的更新
echo -e "\n--- Deployments with Exceeded Deadline ---"
kubectl get deployments -A -o json | jq -r '
  .items[] |
  select(.status.conditions[] | select(.type=="Progressing" and .status=="False" and .reason=="ProgressDeadlineExceeded")) |
  "\(.metadata.namespace)/\(.metadata.name): UPDATE TIMED OUT"
'

# 4. 检查过多的 ReplicaSet
echo -e "\n--- Deployments with Too Many ReplicaSets ---"
for ns in $(kubectl get ns -o jsonpath='{.items[*].metadata.name}'); do
  kubectl get rs -n $ns -o json | jq -r --arg ns "$ns" '
    .items |
    group_by(.metadata.ownerReferences[0].name) |
    map(select(length > 15)) |
    .[] |
    "\($ns)/\(.[0].metadata.ownerReferences[0].name): \(length) ReplicaSets (limit: 10)"
  '
done

# 5. 检查资源使用率
echo -e "\n--- Resource Usage ---"
kubectl top nodes --no-headers | awk '{
  cpu_pct = substr($3, 1, length($3)-1);
  mem_pct = substr($5, 1, length($5)-1);
  if (cpu_pct > 80 || mem_pct > 80) {
    print "⚠️  " $1 ": CPU=" $3 " MEM=" $5
  }
}'

echo -e "\n=== Check Complete ==="
```

#### 每周巡检 (手动)

- [ ] **检查滚动更新策略配置合理性**
  - maxSurge 和 maxUnavailable 是否合理
  - progressDeadlineSeconds 是否足够
  - minReadySeconds 是否设置

- [ ] **检查 PDB 配置与 Deployment 兼容性**
  - PDB minAvailable 不应过高 (建议 < 90%)
  - PDB + maxUnavailable 不应冲突

- [ ] **检查镜像拉取策略**
  - 是否使用固定 tag (避免 latest)
  - imagePullPolicy 是否合理
  - 镜像大小是否过大 (建议 < 500MB)

- [ ] **检查探针配置**
  - initialDelaySeconds 是否足够
  - timeoutSeconds 和 periodSeconds 是否合理
  - failureThreshold 是否过于宽松

- [ ] **检查资源配置**
  - requests 和 limits 是否合理
  - 是否设置了 QoS (推荐 Guaranteed 或 Burstable)

- [ ] **检查历史版本清理**
  - revisionHistoryLimit 是否合理 (默认 10)
  - 是否有孤立的 ReplicaSet (无 owner reference)

### F. Deployment 最佳实践总结

#### 滚动更新策略

```yaml
# 保守策略 (生产关键服务)
spec:
  replicas: 10
  strategy:
    type: RollingUpdate
    rollingUpdate:
      maxUnavailable: 0    # 不允许副本减少
      maxSurge: 1          # 每次只增加 1 个
  minReadySeconds: 30      # 等待 30s 确认稳定
  progressDeadlineSeconds: 1200  # 20 分钟超时

# 快速策略 (开发/测试环境)
spec:
  replicas: 10
  strategy:
    rollingUpdate:
      maxUnavailable: 50%
      maxSurge: 50%
  minReadySeconds: 5
  progressDeadlineSeconds: 300

# 蓝绿策略 (需要快速切换)
spec:
  replicas: 10
  strategy:
    rollingUpdate:
      maxUnavailable: 0
      maxSurge: 100%   # 先创建全部新 Pod
```

#### 健康检查配置

```yaml
spec:
  containers:
  - name: myapp
    # 1. 启动探针 (快速失败，避免长时间等待)
    startupProbe:
      httpGet:
        path: /health/startup
        port: 8080
      failureThreshold: 30    # 30 * 10s = 5min 启动超时
      periodSeconds: 10
      
    # 2. 就绪探针 (流量切换)
    readinessProbe:
      httpGet:
        path: /health/ready
        port: 8080
      initialDelaySeconds: 10
      periodSeconds: 5
      timeoutSeconds: 3
      successThreshold: 1     # 通过 1 次即可
      failureThreshold: 3     # 失败 3 次才标记为 NotReady
      
    # 3. 存活探针 (重启容器)
    livenessProbe:
      httpGet:
        path: /health/live
        port: 8080
      initialDelaySeconds: 30
      periodSeconds: 10
      timeoutSeconds: 5
      failureThreshold: 3     # 失败 3 次才重启
```

#### 优雅终止配置

```yaml
spec:
  terminationGracePeriodSeconds: 60  # 优雅终止时间
  containers:
  - name: myapp
    lifecycle:
      preStop:
        exec:
          command: ["/bin/sh", "-c", "
            # 1. 停止接收新流量 (从负载均衡器摘除)
            curl -X POST localhost:8080/admin/shutdown;
            
            # 2. 等待现有请求处理完成
            sleep 15;
            
            # 3. 清理资源 (可选)
            /app/cleanup.sh || true;
          "]

# 终止流程:
# 1. Pod 状态变为 Terminating
# 2. Endpoint Controller 从 Service Endpoints 中移除 Pod
# 3. PreStop Hook 开始执行
# 4. 同时发送 SIGTERM 信号给容器主进程
# 5. 等待 terminationGracePeriodSeconds
# 6. 发送 SIGKILL 强制终止
```

#### 版本管理最佳实践

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: myapp
  labels:
    app: myapp
  annotations:
    # 记录变更原因 (显示在 rollout history 中)
    kubernetes.io/change-cause: "Upgrade to v2.3 with new feature X"
spec:
  revisionHistoryLimit: 10  # 保留 10 个历史版本
  selector:
    matchLabels:
      app: myapp
  template:
    metadata:
      labels:
        app: myapp
        version: v2.3  # 明确标记版本
    spec:
      containers:
      - name: myapp
        image: myapp:v2.3-20260211-abc123  # 使用明确的镜像 tag (不使用 latest)
        # tag 格式: <version>-<date>-<commit-sha>
```

#### 配置管理最佳实践

```yaml
# 方案: ConfigMap 版本化命名
---
apiVersion: v1
kind: ConfigMap
metadata:
  name: myapp-config-v1  # 版本化命名
data:
  app.yaml: |
    database:
      host: db.example.com
      port: 5432

---
apiVersion: apps/v1
kind: Deployment
metadata:
  name: myapp
spec:
  template:
    spec:
      containers:
      - name: myapp
        envFrom:
        - configMapRef:
            name: myapp-config-v1  # 引用特定版本
        # 更新配置时:
        # 1. 创建新的 ConfigMap (myapp-config-v2)
        # 2. 更新 Deployment 引用新版本
        # 3. 触发滚动更新
        # 4. 回滚时自动回到旧 ConfigMap
```

---

**文档补强完成统计**

- **原始行数**: 576 行
- **补充内容**: ~1100 行
- **预计最终**: ~1600 行
- **新增章节**:
  - 滚动更新核心算法 (含伪代码)
  - 版本回滚机制详解
  - 金丝雀发布与蓝绿部署实战
  - 专家级故障矩阵 (7 阶段分类)
  - 复合故障场景 (3 个)
  - progressDeadlineSeconds 深度解析
  - PDB 交互分析
  - 深度排查脚本 (时间线重建、并发分析)
  - 3 个生产案例 (超时、PDB 阻塞、镜像雪崩)
  - 完整巡检清单
  - 最佳实践总结
