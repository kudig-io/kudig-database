# 05 - Informer 架构与工作队列 (Informer & Workqueue)

## 架构师洞察：SharedInformerFactory 的并发陷阱

虽然 `SharedInformerFactory` 极大节省了资源，但在生产环境下使用时需注意以下几点：

### 关键陷阱与对策
* **Handler 阻塞**: `AddEventHandler` 中的逻辑必须轻量。如果在 Handler 中执行耗时操作，会阻塞整个 Factory 的事件分发管道，导致其他控制器观察到“陈旧”数据。
* **Indexer 竞态**: 避免直接修改从 Indexer 中获取的对象。始终使用 `DeepCopy()`，否则可能意外污染本地缓存，导致难以排查的状态不一致。

## 生产环境 Informer 与 WorkQueue 最佳实践

### 企业级Informer架构优化

#### 多租户Informer管理
```
┌─────────────────────────────────────────────────────────────────────────────┐
│                    企业级多租户Informer架构                                  │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                              │
│  ┌─────────────────────────────────────────────────────────────────────┐   │
│  │                        Informer工厂模式                              │   │
│  │  ┌─────────────┐    ┌─────────────┐    ┌─────────────┐              │   │
│  │  │ Tenant-A    │    │ Tenant-B    │    │ Tenant-C    │              │   │
│  │  │ Informer    │    │ Informer    │    │ Informer    │              │   │
│  │  └─────────────┘    └─────────────┘    └─────────────┘              │   │
│  └─────────────────────────────────────────────────────────────────────┘   │
│                                      │                                       │
│  ┌─────────────────────────────────────────────────────────────────────┐   │
│  │                    共享Informer优化层                                │   │
│  │  • 资源复用: 相同资源类型共享Informer                               │   │
│  │  • 命名空间隔离: 按命名空间过滤事件                                 │   │
│  │  • 标签选择器优化: 精确控制监听范围                                 │   │
│  └─────────────────────────────────────────────────────────────────────┘   │
│                                      │                                       │
│  ┌─────────────────────────────────────────────────────────────────────┐   │
│  │                      缓存管理层                                      │   │
│  │  • LRU缓存策略: 控制内存使用                                        │   │
│  │  • TTL过期机制: 自动清理陈旧数据                                    │   │
│  │  • 压缩存储: 减少内存占用                                           │   │
│  └─────────────────────────────────────────────────────────────────────┘   │
│                                                                              │
└─────────────────────────────────────────────────────────────────────────────┘
```

#### Informer性能调优配置
```go
// 生产级SharedInformerFactory配置
type InformerConfig struct {
    // 同步周期配置
    ResyncPeriod time.Duration `json:"resyncPeriod"`
    
    // 缓存配置
    CacheConfig struct {
        SizeLimit     int           `json:"sizeLimit"`     // 缓存大小限制
        TTL           time.Duration `json:"ttl"`           // 缓存过期时间
        Compression   bool          `json:"compression"`   // 是否启用压缩
    } `json:"cacheConfig"`
    
    // 选择器配置
    Namespace     string        `json:"namespace"`       // 命名空间过滤
    LabelSelector string        `json:"labelSelector"`   // 标签选择器
    FieldSelector string        `json:"fieldSelector"`   // 字段选择器
}

// Informer工厂初始化
func NewSharedInformerFactory(client kubernetes.Interface, config InformerConfig) informers.SharedInformerFactory {
    return informers.NewSharedInformerFactoryWithOptions(
        client,
        config.ResyncPeriod,
        informers.WithNamespace(config.Namespace),
        informers.WithTweakListOptions(func(opts *metav1.ListOptions) {
            opts.LabelSelector = config.LabelSelector
            opts.FieldSelector = config.FieldSelector
        }),
    )
}
```

### WorkQueue生产级实现

#### 高级队列类型和配置
```go
// 生产环境WorkQueue配置
type WorkQueueConfig struct {
    // 基础队列配置
    Name          string        `json:"name"`
    MaxRetries    int           `json:"maxRetries"`    // 最大重试次数
    
    // 限速配置
    RateLimiterConfig struct {
        BaseDelay   time.Duration `json:"baseDelay"`     // 基础延迟
        MaxDelay    time.Duration `json:"maxDelay"`      // 最大延迟
        QPS         float32       `json:"qps"`           // 每秒请求数限制
        Burst       int           `json:"burst"`         // 突发请求数
    } `json:"rateLimiterConfig"`
    
    // 去重配置
    Deduplication struct {
        Enabled   bool          `json:"enabled"`       // 是否启用去重
        TTL       time.Duration `json:"ttl"`           // 去重记录过期时间
    } `json:"deduplication"`
}

// 创建生产级限速队列
func NewProductionRateLimitingQueue(config WorkQueueConfig) workqueue.RateLimitingInterface {
    // 选择合适的限速算法
    var rateLimiter workqueue.RateLimiter
    
    if config.RateLimiterConfig.QPS > 0 {
        // 带QPS限制的指数退避
        rateLimiter = workqueue.NewMaxOfRateLimiter(
            workqueue.NewItemExponentialFailureRateLimiter(
                config.RateLimiterConfig.BaseDelay,
                config.RateLimiterConfig.MaxDelay,
            ),
            &workqueue.BucketRateLimiter{
                Limiter: rate.NewLimiter(
                    rate.Limit(config.RateLimiterConfig.QPS),
                    config.RateLimiterConfig.Burst,
                ),
            },
        )
    } else {
        // 纯指数退避
        rateLimiter = workqueue.NewItemExponentialFailureRateLimiter(
            config.RateLimiterConfig.BaseDelay,
            config.RateLimiterConfig.MaxDelay,
        )
    }
    
    return workqueue.NewNamedRateLimitingQueue(rateLimiter, config.Name)
}
```

#### 事件去重和批处理机制
```go
// 生产级事件处理器
type ProductionEventHandler struct {
    queue         workqueue.RateLimitingInterface
    deduplicator  *EventDeduplicator
    batcher       *EventBatcher
    metrics       *EventHandlerMetrics
}

// 事件去重器
type EventDeduplicator struct {
    cache     map[string]EventRecord
    mutex     sync.RWMutex
    ttl       time.Duration
    gcTicker  *time.Ticker
}

type EventRecord struct {
    ResourceVersion string
    Timestamp       time.Time
    ProcessCount    int
}

func (ed *EventDeduplicator) ShouldProcess(key string, rv string) bool {
    ed.mutex.Lock()
    defer ed.mutex.Unlock()
    
    if record, exists := ed.cache[key]; exists {
        // 如果资源版本相同或更旧，则跳过
        if record.ResourceVersion >= rv {
            ed.metrics.IncrementDuplicateEvents()
            return false
        }
    }
    
    // 更新记录
    ed.cache[key] = EventRecord{
        ResourceVersion: rv,
        Timestamp:       time.Now(),
        ProcessCount:    ed.cache[key].ProcessCount + 1,
    }
    
    return true
}

// 事件批处理器
type EventBatcher struct {
    batchSize    int
    flushTimeout time.Duration
    buffer       []interface{}
    mutex        sync.Mutex
    flushTimer   *time.Timer
    processor    BatchProcessor
}

type BatchProcessor func([]interface{}) error

func (eb *EventBatcher) Add(item interface{}) {
    eb.mutex.Lock()
    defer eb.mutex.Unlock()
    
    eb.buffer = append(eb.buffer, item)
    
    // 达到批次大小立即处理
    if len(eb.buffer) >= eb.batchSize {
        eb.flush()
        return
    }
    
    // 重置刷新定时器
    if eb.flushTimer != nil {
        eb.flushTimer.Stop()
    }
    eb.flushTimer = time.AfterFunc(eb.flushTimeout, eb.flush)
}

func (eb *EventBatcher) flush() {
    eb.mutex.Lock()
    items := make([]interface{}, len(eb.buffer))
    copy(items, eb.buffer)
    eb.buffer = eb.buffer[:0]
    eb.mutex.Unlock()
    
    if len(items) > 0 {
        if err := eb.processor(items); err != nil {
            log.Printf("批量处理失败: %v", err)
            // 将失败的项目重新入队
            for _, item := range items {
                // 重新入队逻辑
            }
        }
    }
}
```

### 监控和故障诊断

#### 关键监控指标
```yaml
# Informer/WorkQueue监控指标
apiVersion: monitoring.coreos.com/v1
kind: ServiceMonitor
metadata:
  name: controller-informer-metrics
spec:
  selector:
    matchLabels:
      app: controller-manager
  endpoints:
    - port: metrics
      interval: 30s
      path: /metrics

---
# Prometheus指标定义
# workqueue_depth - 队列深度
# workqueue_adds_total - 入队总数
# workqueue_retries_total - 重试总数
# workqueue_queue_duration_seconds - 队列等待时间
# workqueue_work_duration_seconds - 处理耗时
# cache_size - 缓存大小
# cache_hits_total - 缓存命中数
# cache_misses_total - 缓存未命中数
```

#### 故障诊断工具集
```bash
#!/bin/bash
# Informer/WorkQueue诊断脚本

echo "=== Informer/WorkQueue诊断报告 ==="

# 1. 检查控制器Pod状态
echo "1. 控制器Pod状态:"
kubectl get pods -n kube-system -l tier=control-plane
echo

# 2. 检查WorkQueue指标
echo "2. WorkQueue深度分析:"
kubectl get --raw=/metrics | grep "workqueue_depth" | head -10
echo

# 3. 检查缓存同步状态
echo "3. 缓存同步延迟:"
kubectl get --raw=/metrics | grep "cache_sync_lag" | head -5
echo

# 4. 检查事件处理速率
echo "4. 事件处理速率:"
kubectl get --raw=/metrics | grep "workqueue_queue_duration" | head -5
echo

# 5. 检查内存使用情况
echo "5. 控制器内存使用:"
kubectl top pods -n kube-system -l tier=control-plane
echo

# 6. 生成性能报告
echo "6. 性能优化建议:"
cat << EOF
优化建议:
1. 调整ResyncPeriod: 根据资源变化频率设置合适的同步周期
2. 优化选择器: 使用精确的LabelSelector和FieldSelector
3. 控制缓存大小: 根据集群规模设置合理的缓存限制
4. 启用批处理: 对高频事件启用批处理机制
5. 监控队列深度: 设置告警阈值防止队列堆积
EOF
```

### 最佳实践配置模板

#### 生产环境Deployment配置
```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: production-controller
spec:
  replicas: 3
  template:
    spec:
      containers:
        - name: controller
          image: controller:latest
          env:
            # Informer配置
            - name: INFORMER_RESYNC_PERIOD
              value: "600s"  # 10分钟同步一次
            - name: INFORMER_NAMESPACE
              value: ""      # 空表示所有命名空间
            - name: INFORMER_LABEL_SELECTOR
              value: "app.kubernetes.io/managed-by=my-controller"
            
            # WorkQueue配置
            - name: WORKQUEUE_MAX_RETRIES
              value: "10"
            - name: WORKQUEUE_BASE_DELAY
              value: "1s"
            - name: WORKQUEUE_MAX_DELAY
              value: "300s"
            - name: WORKQUEUE_QPS_LIMIT
              value: "20"
            
            # 缓存配置
            - name: CACHE_SIZE_LIMIT
              value: "10000"
            - name: CACHE_TTL
              value: "1h"
            - name: ENABLE_COMPRESSION
              value: "true"
          
          resources:
            requests:
              memory: "512Mi"
              cpu: "500m"
            limits:
              memory: "1Gi"
              cpu: "1000m"
          
          # 健康检查
          livenessProbe:
            httpGet:
              path: /healthz
              port: 8080
            initialDelaySeconds: 60
            periodSeconds: 10
          
          readinessProbe:
            httpGet:
              path: /readyz
              port: 8080
            initialDelaySeconds: 30
            periodSeconds: 5
```

#### 高可用配置
```yaml
# 多副本控制器配置
apiVersion: apps/v1
kind: Deployment
metadata:
  name: ha-controller
spec:
  replicas: 3
  selector:
    matchLabels:
      app: ha-controller
  template:
    metadata:
      labels:
        app: ha-controller
    spec:
      affinity:
        # Pod反亲和性确保分散到不同节点
        podAntiAffinity:
          preferredDuringSchedulingIgnoredDuringExecution:
            - weight: 100
              podAffinityTerm:
                labelSelector:
                  matchExpressions:
                    - key: app
                      operator: In
                      values:
                        - ha-controller
                topologyKey: kubernetes.io/hostname
      
      containers:
        - name: controller
          # 启用领导者选举
          env:
            - name: LEADER_ELECTION_ENABLED
              value: "true"
            - name: LEASE_DURATION
              value: "15s"
            - name: RENEW_DEADLINE
              value: "10s"
            - name: RETRY_PERIOD
              value: "2s"
```

## DeltaFIFO详解

| 概念 | 说明 |
|-----|------|
| Delta | 变更记录(对象+类型) |
| FIFO | 先进先出队列 |
| 去重 | 相同key的Delta合并 |
| 类型 | Added, Updated, Deleted, Replaced, Sync |

### Delta类型

| 类型 | 触发时机 |
|-----|---------|
| Added | 新资源创建 |
| Updated | 资源更新 |
| Deleted | 资源删除 |
| Replaced | 重新List替换 |
| Sync | 周期性重同步 |

## Indexer索引机制

| 概念 | 说明 |
|-----|------|
| IndexFunc | 索引函数,提取索引key |
| Indices | 索引名->Index的映射 |
| Index | 索引值->对象key集合的映射 |

### 内置索引函数

| 索引名 | IndexFunc | 用途 |
|-------|----------|------|
| namespace | MetaNamespaceIndexFunc | 按命名空间索引 |

### 自定义索引示例

```go
// 按标签索引
func LabelIndexFunc(obj interface{}) ([]string, error) {
    pod := obj.(*v1.Pod)
    labels := []string{}
    for k, v := range pod.Labels {
        labels = append(labels, fmt.Sprintf("%s=%s", k, v))
    }
    return labels, nil
}

// 注册索引
informer.AddIndexers(cache.Indexers{
    "byLabel": LabelIndexFunc,
})

// 使用索引查询
pods, _ := indexer.ByIndex("byLabel", "app=nginx")
```

## SharedInformerFactory

| 特性 | 说明 |
|-----|------|
| 资源复用 | 同类型资源共享一个Informer |
| 启动管理 | 统一启动所有Informer |
| 缓存同步 | 统一等待缓存同步 |

### 使用示例

```go
// 创建Factory
factory := informers.NewSharedInformerFactory(clientset, 30*time.Second)

// 获取特定资源的Informer
podInformer := factory.Core().V1().Pods()
deployInformer := factory.Apps().V1().Deployments()

// 添加事件处理器
podInformer.Informer().AddEventHandler(cache.ResourceEventHandlerFuncs{
    AddFunc:    onAdd,
    UpdateFunc: onUpdate,
    DeleteFunc: onDelete,
})

// 启动所有Informer
factory.Start(stopCh)

// 等待缓存同步
factory.WaitForCacheSync(stopCh)
```

## WorkQueue类型

| 类型 | 特性 | 用途 |
|-----|------|------|
| Interface | 基础队列 | 简单场景 |
| DelayingInterface | 延迟入队 | 延迟处理 |
| RateLimitingInterface | 限速重试 | 失败重试 |

## WorkQueue操作

| 方法 | 说明 |
|-----|------|
| Add(item) | 添加项目 |
| Get() | 获取项目(阻塞) |
| Done(item) | 标记处理完成 |
| Forget(item) | 清除重试计数 |
| AddAfter(item, d) | 延迟添加 |
| AddRateLimited(item) | 限速添加 |
| NumRequeues(item) | 获取重试次数 |
| ShutDown() | 关闭队列 |

### WorkQueue使用模式

```go
// 1. 创建限速队列
queue := workqueue.NewRateLimitingQueue(
    workqueue.DefaultControllerRateLimiter(),
)

// 2. 事件处理器入队
handler := cache.ResourceEventHandlerFuncs{
    AddFunc: func(obj interface{}) {
        key, _ := cache.MetaNamespaceKeyFunc(obj)
        queue.Add(key)
    },
    UpdateFunc: func(old, new interface{}) {
        key, _ := cache.MetaNamespaceKeyFunc(new)
        queue.Add(key)
    },
    DeleteFunc: func(obj interface{}) {
        key, _ := cache.DeletionHandlingMetaNamespaceKeyFunc(obj)
        queue.Add(key)
    },
}

// 3. 工作循环处理
for {
    key, quit := queue.Get()
    if quit {
        return
    }
    
    err := processItem(key)
    if err == nil {
        queue.Forget(key)
    } else {
        queue.AddRateLimited(key)
    }
    queue.Done(key)
}
```

## 限速器类型

| 限速器 | 算法 | 参数 |
|-------|-----|------|
| BucketRateLimiter | 令牌桶 | rate, burst |
| ItemExponentialFailureRateLimiter | 指数退避 | baseDelay, maxDelay |
| ItemFastSlowRateLimiter | 快慢切换 | fastDelay, slowDelay, maxFastAttempts |
| MaxOfRateLimiter | 取最大 | 组合多个限速器 |

### 默认限速器

```go
// DefaultControllerRateLimiter组合:
// 1. 指数退避: 5ms起始, 最大1000秒
// 2. 令牌桶: 10 QPS, burst=100
func DefaultControllerRateLimiter() RateLimiter {
    return NewMaxOfRateLimiter(
        NewItemExponentialFailureRateLimiter(5*time.Millisecond, 1000*time.Second),
        &BucketRateLimiter{Limiter: rate.NewLimiter(rate.Limit(10), 100)},
    )
}
```

## Resync机制

| 概念 | 说明 |
|-----|------|
| ResyncPeriod | 周期性重同步间隔 |
| 作用 | 确保控制器处理所有对象 |
| 触发 | 生成Sync类型Delta |

### Resync配置

```go
// 30秒重同步
factory := informers.NewSharedInformerFactory(clientset, 30*time.Second)

// 禁用重同步
factory := informers.NewSharedInformerFactory(clientset, 0)
```

## 性能优化建议

| 优化 | 说明 |
|-----|------|
| 共享Informer | 使用SharedInformerFactory |
| 合理Resync | 不要设置过短 |
| 索引优化 | 添加常用索引 |
| 过滤事件 | EventHandler中过滤无关事件 |
| 控制并发 | 合理设置worker数量 |

## 调试技巧

| 方法 | 说明 |
|-----|------|
| 查看缓存 | informer.GetStore().List() |
| 查看索引 | indexer.IndexKeys(indexName, value) |
| 队列长度 | queue.Len() |
| 重试次数 | queue.NumRequeues(key) |

---

**表格底部标记**: Kusheet Project, 作者 Allen Galler (allengaller@gmail.com)
