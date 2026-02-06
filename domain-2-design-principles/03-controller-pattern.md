# 13 - 控制器模式与调谐循环 (Controller Pattern & Reconciliation)

## 生产环境控制器设计最佳实践

### 控制器可靠性设计模式

#### 控制器生命周期管理
```
┌─────────────────────────────────────────────────────────────────────────────┐
│                    控制器生产级生命周期                                       │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                              │
│  初始化阶段 (Initialization)                                                │
│  ┌─────────────────────────────────────────────────────────────────────┐   │
│  │ 1. 配置加载和验证                                                    │   │
│  │ 2. 客户端初始化 (API Server连接)                                     │   │
│  │ 3. Informer启动和缓存同步                                            │   │
│  │ 4. 工作队列初始化                                                    │   │
│  │ 5. 健康检查端点启动                                                  │   │
│  └─────────────────────────────────────────────────────────────────────┘   │
│                                                                              │
│  运行阶段 (Running)                                                         │
│  ┌─────────────────────────────────────────────────────────────────────┐   │
│  │ • 持续调谐循环                                                       │   │
│  │ • 资源变化事件处理                                                   │   │
│  │ • 定期重新排队 (resync)                                              │   │
│  │ • 指标收集和监控                                                     │   │
│  └─────────────────────────────────────────────────────────────────────┘   │
│                                                                              │
│  优雅终止阶段 (Graceful Shutdown)                                           │
│  ┌─────────────────────────────────────────────────────────────────────┐   │
│  │ 1. 停止接收新事件                                                    │   │
│  │ 2. 处理完当前队列中的所有项目                                        │   │
│  │ 3. 清理资源和连接                                                    │   │
│  │ 4. 等待所有工作协程完成                                              │   │
│  └─────────────────────────────────────────────────────────────────────┘   │
│                                                                              │
└─────────────────────────────────────────────────────────────────────────────┘
```

### 控制器性能优化策略

#### 并发控制与限流
```go
// 生产级控制器配置示例
type ControllerConfig struct {
    // 并发工作协程数
    Workers int `json:"workers"`
    
    // 工作队列配置
    QueueConfig struct {
        MaxRetries     int           `json:"maxRetries"`     // 最大重试次数
        BaseDelay      time.Duration `json:"baseDelay"`      // 基础延迟
        MaxDelay       time.Duration `json:"maxDelay"`       // 最大延迟
        RateLimiterQPS float32       `json:"rateLimiterQPS"` // QPS限制
    } `json:"queueConfig"`
    
    // 调谐配置
    ReconcileConfig struct {
        Timeout        time.Duration `json:"timeout"`        // 调谐超时
        ResyncPeriod   time.Duration `json:"resyncPeriod"`   // 重新同步周期
        MaxConcurrent  int           `json:"maxConcurrent"`  // 最大并发调谐
    } `json:"reconcileConfig"`
}

// 限流工作队列实现
func NewRateLimitingQueue(config ControllerConfig) workqueue.RateLimitingInterface {
    return workqueue.NewNamedRateLimitingQueue(
        workqueue.NewItemExponentialFailureRateLimiter(
            config.QueueConfig.BaseDelay,
            config.QueueConfig.MaxDelay,
        ),
        "controller-queue",
    )
}
```

#### 缓存优化策略
```yaml
# Informer缓存优化配置
apiVersion: apps/v1
kind: Deployment
metadata:
  name: custom-controller
spec:
  template:
    spec:
      containers:
        - name: controller
          env:
            # 调整Informer缓存大小
            - name: INFORMER_CACHE_SIZE
              value: "10000"
            
            # 设置List超时时间
            - name: LIST_TIMEOUT
              value: "60s"
            
            # 启用增量同步
            - name: ENABLE_INCREMENTAL_SYNC
              value: "true"
          
          resources:
            requests:
              memory: "512Mi"
              cpu: "500m"
            limits:
              memory: "1Gi"
              cpu: "1000m"
```

### 控制器监控与可观测性

#### 关键指标定义
```yaml
# Controller指标配置
apiVersion: monitoring.coreos.com/v1
kind: ServiceMonitor
metadata:
  name: controller-metrics
spec:
  selector:
    matchLabels:
      app: custom-controller
  endpoints:
    - port: metrics
      interval: 30s
      path: /metrics

---
# Prometheus指标定义
# controller_workqueue_depth - 队列深度
# controller_workqueue_adds_total - 入队总数
# controller_workqueue_retries_total - 重试总数
# controller_reconcile_errors_total - 调谐错误总数
# controller_reconcile_time_seconds - 调谐耗时分布
```

#### 健康检查和就绪探针
```go
// 控制器健康检查实现
func (c *Controller) setupHealthChecks() {
    // 就绪探针 - 缓存是否已同步
    http.HandleFunc("/readyz", func(w http.ResponseWriter, r *http.Request) {
        if c.informer.HasSynced() {
            w.WriteHeader(http.StatusOK)
            w.Write([]byte("ok"))
        } else {
            w.WriteHeader(http.StatusServiceUnavailable)
            w.Write([]byte("not synced"))
        }
    })
    
    // 健康探针 - 控制器是否正常运行
    http.HandleFunc("/healthz", func(w http.ResponseWriter, r *http.Request) {
        select {
        case <-c.stopCh:
            w.WriteHeader(http.StatusServiceUnavailable)
            w.Write([]byte("stopping"))
        default:
            w.WriteHeader(http.StatusOK)
            w.Write([]byte("ok"))
        }
    })
}
```

### 故障处理与恢复机制

#### 错误处理策略
```go
// 生产级错误处理模式
func (r *Reconciler) Reconcile(ctx context.Context, req ctrl.Request) (ctrl.Result, error) {
    log := r.Log.WithValues("resource", req.NamespacedName)
    
    // 获取资源对象
    obj := &MyCustomResource{}
    if err := r.Get(ctx, req.NamespacedName, obj); err != nil {
        // 如果资源不存在，从队列中移除
        return ctrl.Result{}, client.IgnoreNotFound(err)
    }
    
    // 检查删除时间戳
    if obj.DeletionTimestamp != nil {
        return r.handleFinalizer(ctx, obj)
    }
    
    // 添加finalizer
    if !controllerutil.ContainsFinalizer(obj, myFinalizerName) {
        controllerutil.AddFinalizer(obj, myFinalizerName)
        return ctrl.Result{Requeue: true}, r.Update(ctx, obj)
    }
    
    // 执行主要调谐逻辑
    result, err := r.reconcileLogic(ctx, obj)
    if err != nil {
        log.Error(err, "调谐失败")
        
        // 记录错误指标
        reconcileErrors.WithLabelValues(obj.Kind, obj.Name).Inc()
        
        // 根据错误类型决定重试策略
        switch {
        case isTransientError(err):
            // 临时错误，指数退避重试
            return ctrl.Result{RequeueAfter: calculateBackoff(retryCount)}, nil
        case isPermanentError(err):
            // 永久错误，记录但不重试
            log.Error(err, "永久错误，停止重试")
            return ctrl.Result{}, nil
        default:
            // 未知错误，正常重试
            return ctrl.Result{}, err
        }
    }
    
    return result, nil
}
```

#### 资源清理和Finalizers
```go
// Finalizer处理模式
const myFinalizerName = "mycontroller.example.com/finalizer"

func (r *Reconciler) handleFinalizer(ctx context.Context, obj *MyCustomResource) (ctrl.Result, error) {
    // 检查是否还有finalizer
    if !controllerutil.ContainsFinalizer(obj, myFinalizerName) {
        return ctrl.Result{}, nil
    }
    
    // 执行清理逻辑
    if err := r.cleanupExternalResources(ctx, obj); err != nil {
        return ctrl.Result{}, fmt.Errorf("清理外部资源失败: %w", err)
    }
    
    // 移除finalizer
    controllerutil.RemoveFinalizer(obj, myFinalizerName)
    if err := r.Update(ctx, obj); err != nil {
        return ctrl.Result{}, err
    }
    
    return ctrl.Result{}, nil
}
```

### 多控制器协调

#### 控制器依赖管理
```yaml
# 控制器依赖关系定义
apiVersion: apps/v1
kind: Deployment
metadata:
  name: primary-controller
spec:
  template:
    spec:
      initContainers:
        # 等待依赖控制器就绪
        - name: wait-for-dependencies
          image: busybox:1.35
          command:
            - sh
            - -c
            - |
              until nc -z dependency-controller-service 8080; do
                echo "等待依赖控制器就绪..."
                sleep 5
              done
              echo "依赖控制器已就绪"
```

#### 资源所有权管理
```go
// 设置控制器引用
func (r *Reconciler) setOwnerReference(owner, owned client.Object) error {
    return ctrl.SetControllerReference(owner, owned, r.Scheme)
}

// 创建子资源时设置所有权
func (r *Reconciler) createOwnedResource(ctx context.Context, owner *MyCustomResource) error {
    deployment := &appsv1.Deployment{
        ObjectMeta: metav1.ObjectMeta{
            Name:      fmt.Sprintf("%s-worker", owner.Name),
            Namespace: owner.Namespace,
        },
        Spec: appsv1.DeploymentSpec{
            // ... deployment spec
        },
    }
    
    // 设置所有权关系
    if err := r.setOwnerReference(owner, deployment); err != nil {
        return err
    }
    
    return r.Create(ctx, deployment)
}
```

## 控制器组成部分

| 组件 | 英文 | 职责 |
|-----|-----|------|
| Informer | 信息器 | 监听API Server变化，维护本地缓存 |
| Lister | 列表器 | 从本地缓存读取资源 |
| WorkQueue | 工作队列 | 存储待处理的资源key |
| Reconciler | 调谐器 | 执行实际的调谐逻辑 |

## Informer工作机制

| 阶段 | 说明 |
|-----|------|
| List | 启动时全量获取资源列表 |
| Watch | 持续监听增量变化事件 |
| 缓存同步 | 将变化同步到本地缓存(Indexer) |
| 事件分发 | 通过EventHandler分发到WorkQueue |

### Informer架构

```
API Server
    │
    │ List & Watch
    ▼
┌─────────────────────────────────────────────┐
│                 Informer                     │
│  ┌─────────┐    ┌─────────┐    ┌─────────┐ │
│  │ Reflector│───▶│  Store  │───▶│ Indexer │ │
│  └─────────┘    └─────────┘    └─────────┘ │
│       │                              │      │
│       ▼                              ▼      │
│  ┌─────────────────────────────────────┐   │
│  │         EventHandler                 │   │
│  │  OnAdd() / OnUpdate() / OnDelete()   │   │
│  └─────────────────────────────────────┘   │
└─────────────────────────────────────────────┘
         │
         ▼
    WorkQueue
         │
         ▼
    Controller (Reconcile)
```

## WorkQueue特性

| 特性 | 说明 |
|-----|------|
| 去重(De-dup) | 相同key只保留一个 |
| 限速(Rate Limiting) | 失败重试带退避 |
| 公平(Fair) | 不同key公平处理 |
| 关机(Shutdown) | 优雅关闭支持 |

### WorkQueue类型

| 类型 | 用途 |
|-----|------|
| FIFO Queue | 基础队列 |
| Delaying Queue | 延迟入队 |
| Rate Limiting Queue | 限速重试 |

## 调谐循环代码模式

```go
// 控制器主循环
func (c *Controller) Run(workers int, stopCh <-chan struct{}) error {
    defer c.workqueue.ShutDown()
    
    // 等待缓存同步
    if !cache.WaitForCacheSync(stopCh, c.informer.HasSynced) {
        return fmt.Errorf("failed to sync caches")
    }
    
    // 启动工作协程
    for i := 0; i < workers; i++ {
        go wait.Until(c.runWorker, time.Second, stopCh)
    }
    
    <-stopCh
    return nil
}

// 工作协程
func (c *Controller) runWorker() {
    for c.processNextItem() {
    }
}

// 处理单个项目
func (c *Controller) processNextItem() bool {
    key, quit := c.workqueue.Get()
    if quit {
        return false
    }
    defer c.workqueue.Done(key)
    
    err := c.syncHandler(key.(string))
    c.handleErr(err, key)
    return true
}

// 错误处理
func (c *Controller) handleErr(err error, key interface{}) {
    if err == nil {
        c.workqueue.Forget(key)
        return
    }
    
    if c.workqueue.NumRequeues(key) < maxRetries {
        c.workqueue.AddRateLimited(key)
        return
    }
    
    c.workqueue.Forget(key)
    runtime.HandleError(err)
}
```

## 调谐函数模式

```go
func (c *Controller) syncHandler(key string) error {
    namespace, name, err := cache.SplitMetaNamespaceKey(key)
    if err != nil {
        return err
    }
    
    // 1. 获取资源
    obj, err := c.lister.Get(name)
    if errors.IsNotFound(err) {
        // 资源已删除，执行清理
        return nil
    }
    if err != nil {
        return err
    }
    
    // 2. 检查是否需要处理
    if !c.needsReconcile(obj) {
        return nil
    }
    
    // 3. 执行调谐逻辑
    result, err := c.reconcile(obj)
    if err != nil {
        return err
    }
    
    // 4. 更新状态
    if result.StatusChanged {
        _, err = c.client.UpdateStatus(obj)
    }
    
    return err
}
```

## 内置控制器详解

| 控制器 | 监听资源 | 管理资源 | 核心逻辑 |
|-------|---------|---------|---------|
| Deployment | Deployment | ReplicaSet | 滚动更新、版本管理 |
| ReplicaSet | ReplicaSet, Pod | Pod | 维护副本数 |
| StatefulSet | StatefulSet, Pod | Pod, PVC | 有序部署、持久存储 |
| DaemonSet | DaemonSet, Node, Pod | Pod | 每节点一个Pod |
| Job | Job, Pod | Pod | 完成后清理 |
| CronJob | CronJob | Job | 定时创建Job |
| Endpoints | Service, Pod | Endpoints | 维护端点列表 |
| Namespace | Namespace | 所有ns资源 | 级联删除 |
| Node | Node | Pod | 节点健康管理 |
| GC | 所有资源 | 所有资源 | 孤儿资源清理 |

## ReplicaSet控制器逻辑

| 步骤 | 操作 |
|-----|------|
| 1 | 获取ReplicaSet对象 |
| 2 | 列出所有匹配selector的Pod |
| 3 | 过滤掉正在删除的Pod |
| 4 | 计算当前副本数与期望值差异 |
| 5 | 差异>0: 创建新Pod |
| 6 | 差异<0: 删除多余Pod |
| 7 | 更新ReplicaSet status |

## Deployment控制器逻辑

| 步骤 | 操作 |
|-----|------|
| 1 | 获取Deployment对象 |
| 2 | 列出所有关联的ReplicaSet |
| 3 | 计算新旧ReplicaSet |
| 4 | 根据更新策略(RollingUpdate/Recreate)执行 |
| 5 | 调整新RS副本数(scale up) |
| 6 | 调整旧RS副本数(scale down) |
| 7 | 更新Deployment status |

## 并发控制最佳实践

| 实践 | 说明 |
|-----|------|
| 使用resourceVersion | 乐观锁避免冲突 |
| 处理409冲突 | 重新获取后重试 |
| 单一所有者 | 避免多控制器管理同一资源 |
| 使用OwnerReferences | 明确资源所属关系 |
| 幂等操作 | 确保重复执行结果一致 |

## 控制器开发框架

| 框架 | 特点 | 适用场景 |
|-----|------|---------|
| client-go | 官方底层库 | 深度定制 |
| controller-runtime | 高级抽象 | 快速开发 |
| Kubebuilder | 脚手架+controller-runtime | 标准Operator |
| Operator SDK | 多语言支持 | Go/Ansible/Helm |

## 调试技巧

| 方法 | 说明 |
|-----|------|
| 日志级别 | 调整klog verbosity |
| 事件记录 | 使用EventRecorder |
| 指标暴露 | Prometheus metrics |
| 健康检查 | /healthz, /readyz |
| pprof | 性能分析 |

---

**表格底部标记**: Kusheet Project, 作者 Allen Galler (allengaller@gmail.com)
