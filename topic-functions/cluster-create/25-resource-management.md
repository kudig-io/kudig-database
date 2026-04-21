# 资源管理与配额

## 源码路径

`pkg/apis/core/`
`pkg/controller/resourcequota/`
`pkg/controller/limitrange/`

---

## ResourceQuota

限制命名空间资源总量:

```yaml
apiVersion: v1
kind: ResourceQuota
metadata:
  name: quota
  namespace: default
spec:
  hard:
    # 计数类型
    pods: "20"
    services: "10"
    replicationcontrollers: "5"
    resourcequotas: "1"
    # 计算资源
    requests.cpu: "4"
    requests.memory: 8Gi
    limits.cpu: "8"
    limits.memory: 16Gi
    # 存储
    persistentvolumeclaims: "5"
    requests.storage: "100Gi"
    # 子资源
    services.nodeport: "2"
    services.loadbalancer: "1"
```

---

## LimitRange

限制单个 Pod/容器的资源:

```yaml
apiVersion: v1
kind: LimitRange
metadata:
  name: limits
  namespace: default
spec:
  limits:
  # 默认容器限制
  - type: Container
    default:
      cpu: 500m
      memory: 256Mi
    defaultRequest:
      cpu: 100m
      memory: 128Mi
    max:
      cpu: "2"
      memory: 1Gi
    min:
      cpu: 50m
      memory: 32Mi
    maxLimitRequestRatio:
      cpu: "10"
      memory: "4"
  # Pod 级别限制
  - type: Pod
    max:
      cpu: "4"
      memory: 2Gi
```

---

## PriorityClass

Pod 优先级用于抢占调度:

```yaml
# 高优先级
apiVersion: scheduling.k8s.io/v1
kind: PriorityClass
metadata:
  name: high-priority
value: 100000
globalDefault: false  # 是否为默认优先级
description: "生产环境高优先级工作负载"

# 低优先级
apiVersion: scheduling.k8s.io/v1
kind: PriorityClass
metadata:
  name: low-priority
value: 100
globalDefault: true   # 无指定时使用此值
description: "测试环境工作负载"
```

```yaml
# Pod 使用优先级
spec:
  priorityClassName: high-priority
```

---

## 资源请求与限制

```yaml
spec:
  containers:
  - name: app
    resources:
      requests:
        cpu: "1"
        memory: "1Gi"
      limits:
        cpu: "2"
        memory: "2Gi"

# requests: 调度依据，决定 Pod 应该调度到哪个节点
# limits: 运行时上限，超限会被 OOM Kill 或 CPU 节流
```

---

## kubelet Eviction Thresholds

kubelet 主动驱逐 Pod 以保护节点稳定性:

```yaml
# /var/lib/kubelet/config.yaml
evictionHard:
  memory.available: "100Mi"      # 内存小于 100Mi 时驱逐
  nodefs.available: "10%"        # 磁盘小于 10% 时驱逐
  imagefs.available: "15%"        # 镜像磁盘小于 15% 时驱逐
  nodefs.inodesFree: "5%"         # inode 不足时驱逐

evictionSoft:
  memory.available: "200Mi"
  nodefs.available: "15%"

evictionSoftGracePeriod:
  memory.available: "1m30s"
  nodefs.available: "1m30s"

evictionPressureTransitionPeriod: 5m

evictionMinimumReclaim:
  memory.available: "50Mi"
  nodefs.available: "5%"
```

---

## Eviction 优先级

当资源不足时，kubelet 按以下顺序驱逐:

```
1. BestEffort Pod (最低优先级)
    ↓
2. Burstable Pod
    ↓
3. Guaranteed Pod (不会被驱逐，除非资源枯竭)
```

**Guaranteed Pod**: 所有容器都设置了 CPU/内存 limits，且 requests == limits。

---

## PodDisruptionBudget (PDB)

保护 Pod 在自愿中断时不被全部驱逐:

```yaml
apiVersion: policy/v1
kind: PodDisruptionBudget
metadata:
  name: app-pdb
spec:
  # 最少保持可用数
  minAvailable: 2
  # 或最多不可用数
  # maxUnavailable: 1
  selector:
    matchLabels:
      app: frontend
```

```bash
# 查看 PDB
kubectl get pdb

# 允许的驱逐:
# - kubectl drain --ignore-daemonsets --grace-period=X
# - Deployment滚动更新
# - 节点维护 (kubectl drain)
```

---

## EndpointSlice

EndpointSlice (1.16+) 替代 Endpoints，减少 API Server 负载:

```bash
# 查看 EndpointSlice
kubectl get endpointslices

# Endpoint vs EndpointSlice:
# Endpoints: 单个资源，限制 1000 endpoints
# EndpointSlice: 分布式，多个资源，每个含 100 endpoints
```

```yaml
# EndpointSlice 示例
apiVersion: discovery.k8s.io/v1
kind: EndpointSlice
metadata:
  name: nginx
  labels:
    kubernetes.io/service-name: nginx
addressType: IPv4
ports:
- port: 80
  name: http
  protocol: TCP
endpoints:
- addresses:
  - "10.244.0.10"
  conditions:
    ready: true
  topology:
    kubernetes.io/hostname: node-1
```

---

## Topology Aware Routing (ServiceTopology)

优先使用同拓扑域的 Endpoint:

```yaml
apiVersion: v1
kind: Service
metadata:
  name: nginx
spec:
  topologyKeys:
  - kubernetes.io/hostname      # 优先同节点
  - topology.kubernetes.io/zone  # 其次同可用区
  - topology.kubernetes.io/region # 最后同区域
  selector:
    app: nginx
```

---

## 常见问题

| 问题 | 原因 | 解决 |
|------|------|------|
| Pod 被 OOM Kill | 内存超 limit | 增加 limit 或减少工作负载 |
| Pod 驱逐 | 节点资源不足 | 添加节点或减少调度 |
| 调度失败 | 资源不足 | 检查 requests 是否合理 |
| PDB 阻止 drain | PDB 配置太严格 | 调整 minAvailable/maxUnavailable |
