# kube-scheduler 调度详解

## 源码路径

`pkg/scheduler/`
`cmd/kubeadm/app/phases/controlplane/` (scheduler manifest)

---

## 调度流程

```
Pod 创建请求
    ↓
API Server 接收并写入 etcd
    ↓
调度队列 (SchedulingQueue)
    ↓
预选阶段 (Filtering)
    ↓
打分阶段 (Scoring)
    ↓
优选阶段 (Prioritizing)
    ↓
选择最佳节点 (Selecting)
    ↓
绑定阶段 (Binding)
    ↓
Pod 调度完成
```

---

## 预选阶段 (Predicates/Filtering)

```go
// 过滤不满足条件的节点
type Predicates map[string]PredicateFunction

// 内置 Predicates:
func GeneralPredicates(pod *v1.Pod, nodeInfo *NodeInfo) bool {
    // 1. PodFitsHostPorts - 端口冲突检查
    // 2. HostNamePred - NodeName 匹配
    // 3. MatchNodeSelector - NodeSelector/NodeAffinity
    // 4. PodFitsResources - CPU/内存是否足够
    // 5. NoDiskConflict - PVC 挂载无冲突
    // 6. CheckNodeMemoryPressure - 节点是否内存紧张
    // 7. CheckNodeDiskPressure - 节点是否磁盘紧张
    // 8. PodToleratesNodeTaints - Pod 容忍节点污点
    // 9. CheckVolumeBinding - PVC 是否能绑定到节点
}
```

---

## 打分阶段 (Priorities/Scoring)

```go
// 各节点打分
type PriorityConfig struct {
    Name   string
    Score  int32
    Weight int32
}

// 内置 Priorities:
func CalculateNodeAffinityPriority(pod *v1.Pod, nodeInfo *NodeInfo) int {
    // 节点亲和性匹配度
}

func CalculateResourceAllocationPriority(pod *v1.Pod, nodeInfo *NodeInfo) int {
    // 资源分配率 (空闲资源越多分数越高)
}

func CalculateImageLocalityPriority(pod *v1.Pod, nodeInfo *NodeInfo) int {
    // 镜像是否已在节点上 (无需拉取)
}

func CalculateTaintTolerationPriority(pod *v1.Pod, nodeInfo *NodeInfo) int {
    // Pod 容忍与节点污点匹配度
}
```

---

## kube-scheduler 配置

```yaml
# /etc/kubernetes/manifests/kube-scheduler.yaml
apiVersion: v1
kind: Pod
spec:
  containers:
  - name: kube-scheduler
    command:
    - kube-scheduler
    - --authentication-kubeconfig=/etc/kubernetes/scheduler.conf
    - --authorization-kubeconfig=/etc/kubernetes/scheduler.conf
    - --leader-elect=true
    - --scheduler-name=default-scheduler
    - --profiling=true
```

---

## kube-scheduler ConfigMap (K8s 1.22+)

```yaml
# KubeSchedulerConfiguration (1.22+ 推荐)
apiVersion: kubescheduler.config.k8s.io/v1beta3
kind: KubeSchedulerConfiguration
profiles:
- pluginConfig:
  - name: NodeResourcesFit
    args:
      scoringStrategy:
        resources:
        - name: cpu
          weight: 1
        - name: memory
          weight: 1
        strategy: Least # Least/Most/Requested
  - name: InterPodAffinity
    args:
      hardTopologyKey: topology.kubernetes.io/zone
      softWeight: 1
  - name: ImageLocality
    args:
      minFeasibleImagePercentage: 50
      minFeasibleNodes: 10
```

---

## 调度算法详解

### 资源请求 vs 限制

```yaml
# Pod 资源请求 (调度依据)
spec:
  containers:
  - name: app
    resources:
      requests:
        cpu: "500m"
        memory: "128Mi"
      limits:
        cpu: "1000m"
        memory: "256Mi"

# 调度依据: requests (不是 limits)
# 节点可用: allocatable - 已调度 Pod requests
```

---

## 污点与容忍

```bash
# 节点污点
kubectl taint nodes <node> key=value:NoSchedule

# 污点效果:
# - NoSchedule: 不调度 (除非容忍)
# - PreferNoSchedule: 尽量避免调度
# - NoExecute: 驱逐已有 Pod (可选)

# Pod 容忍
spec:
  tolerations:
  - key: "key"
    operator: "Equal"  # Exists/Equal
    value: "value"
    effect: "NoSchedule"
    tolerationSeconds: 300  # NoExecute 时有效
```

---

## 亲和性与反亲和性

```yaml
# Pod 与 Node 亲和性
spec:
  affinity:
    nodeAffinity:
      requiredDuringSchedulingIgnoredDuringExecution:
        nodeSelectorTerms:
        - matchExpressions:
          - key: disktype
            operator: In
            values:
            - ssd
      preferredDuringSchedulingIgnoredDuringExecution:
      - weight: 1
        preference:
          matchExpressions:
          - key: workload-type
            operator: In
            values:
            - interactive

# Pod 与 Pod 亲和性 (同域调度)
spec:
  affinity:
    podAffinity:
      requiredDuringSchedulingIgnoredDuringExecution:
      - labelSelector:
          matchLabels:
            app: frontend
        topologyKey: topology.kubernetes.io/zone

# Pod 反亲和性 (分散调度)
spec:
  affinity:
    podAntiAffinity:
      requiredDuringSchedulingIgnoredDuringExecution:
      - labelSelector:
          matchLabels:
            app: redis
        topologyKey: kubernetes.io/hostname
```

---

## TopologyKey

```bash
# 常用拓扑域:
topology.kubernetes.io/hostname     # 节点级别
topology.kubernetes.io/zone         # 可用区级别
topology.kubernetes.io/region       # 区域级别
topology.io/hostname                # legacy
failure-domain.beta.kubernetes.io/zone  # legacy (deprecated)
failure-domain.beta.kubernetes.io/region # legacy (deprecated)
```

---

## 调度重试

```go
// 当调度失败时:
if !scheduled {
    // 1. 放入调度队列尾部 (backoff)
    // 2. 记录调度失败原因
    // 3. 等待节点资源释放或新节点加入
}
```

```bash
# 查看调度失败原因
kubectl describe pod <pod-name> | grep -A 5 "Events"

# 输出:
# Events:
#   Type     Reason            From                  Message
#   Warning  FailedScheduling  default-scheduler     0/3 nodes are available: 1 Insufficient memory, 2 node(s) had taints that the pod didn't tolerate.
```

---

## Preemption (抢占调度)

```yaml
# Pod 优先级
apiVersion: v1
kind: Pod
metadata:
  name: high-priority
spec:
  priorityClassName: high-priority
  # 或者直接指定:
  # priority: 1000
```

```go
// 当高优先级 Pod 无法调度时:
// 1. 查找低优先级 Pod
// 2. 驱逐低优先级 Pod
// 3. 调度高优先级 Pod
// 4. 被驱逐 Pod 回到调度队列重新调度
```

**注意**: 抢占调度由 kube-scheduler 的 `Preempt` plugin 处理 (1.22+ 默认开启)。

---

## 调度插件 (1.22+ Framework)

```go
// K8s 1.22+ 使用插件架构:
type Plugin struct {
    Name string
    PreScore  func(ctx context.Context, state *CycleState, pod *v1.Pod, nodes []*v1.Node) *Status
    Score     func(ctx context.Context, state *CycleState, pod *v1.Pod, nodeName string) (Score, *Status)
    Reserve   func(ctx context.Context, state *CycleState, pod *v1.Pod, nodeName string) *Status
    Permit    func(ctx context.Context, state *CycleState, pod *v1.Pod, nodeName string) *Status
    PostBind  func(ctx context.Context, state *CycleState, pod *v1.Pod, nodeName string)
}

// 默认启用的插件:
DefaultPlugins = []Plugin{
    NodeName{},
    NodeResourcesFit{},
    ImageLocality{},
    InterPodAffinity{},
    NodeResourcesBalancedAllocation{},
    PodTopologySpread{},
    SelectorSpread{},
    TaintToleration{},
    NodeAffinity{},
    PrioritySort{},
}
```

---

## 常见问题

| 问题 | 原因 | 解决 |
|------|------|------|
| Pod 一直 Pending | 无节点满足条件 | `kubectl describe pod` 查看原因 |
| 节点有资源但调度失败 | Predicate 过滤 | 检查污点/亲和性/端口冲突 |
| 高优先级 Pod 抢占低优先级 | 正常调度行为 | 配置 PodDisruptionBudget 保护 |
| 调度太慢 | 大规模集群/算法复杂 | 启用 VolumeScheduling |
