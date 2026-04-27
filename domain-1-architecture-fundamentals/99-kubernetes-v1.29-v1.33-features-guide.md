# Kubernetes v1.29 - v1.33 版本特性深度指南

> **适用版本**: Kubernetes v1.29 - v1.33  
> **最后更新**: 2026-04-24  
> **难度**: 中级 → 高级

---

## 📋 目录

- [一、版本演进总览](#一版本演进总览)
- [二、v1.29 核心特性](#二v1.29-核心特性)
- [三、v1.30 核心特性](#三v1.30-核心特性)
- [四、v1.31 核心特性](#四v1.31-核心特性)
- [五、v1.32 核心特性](#五v1.32-核心特性)
- [六、v1.33 核心特性](#六v1.33-核心特性)
- [七、渐进式升级路径](#七渐进式升级路径)
- [八、Feature Gate 速查](#八feature-gate-速查)
- [九、版本兼容性矩阵](#九版本兼容性矩阵)

---

## 一、版本演进总览

```
Kubernetes v1.29 → v1.33 演进路线
    │
    ├── v1.29 (2023.12) — Sidecar Beta, ReadWriteOncePod GA
    ├── v1.30 (2024.04) — CEL Admission GA, SchedulingGates GA
    ├── v1.31 (2024.08) — AppArmor GA, nftables Alpha, DRA 改进
    ├── v1.32 (2024.12) — DRA Beta, TopologyManager Per Pod Beta
    └── v1.33 (2025.04) — Sidecar GA, DRA GA, In-Place Resize Alpha

五大演进主题:
1. Sidecar 原生生命周期管理 (v1.29 Beta → v1.33 GA)
2. CEL 准入控制替代 Webhook (v1.30 GA)
3. 动态资源分配 DRA (v1.31~v1.33 Beta→GA)
4. 就地 Pod 资源调整 (v1.33 Alpha)
5. 云原生解耦 (in-tree 驱动/云厂商全面弃用)
```

---

## 二、v1.29 核心特性

### 2.1 Sidecar 容器 (Beta, 默认启用)

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: myapp
spec:
  initContainers:
  - name: istio-init
    image: istio/proxyv2:1.20.0
    restartPolicy: Always          # ← v1.29 新增: 标识为 Sidecar
    securityContext:
      allowPrivilegeEscalation: false
  containers:
  - name: myapp
    image: myapp:v1.0
```

**行为变化**:
- `restartPolicy: Always` 的 init 容器成为 Sidecar
- Sidecar 在普通容器启动前启动，在所有普通容器终止后终止
- Sidecar 崩溃时会自动重启，不影响 Pod 生命周期

### 2.2 ReadWriteOncePod 访问模式 (GA)

```yaml
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: exclusive-pvc
spec:
  accessModes:
  - ReadWriteOncePod        # ← 仅允许单个 Pod 读写
  resources:
    requests:
      storage: 10Gi
```

### 2.3 KMS v2 加密 (GA)

```yaml
apiVersion: apiserver.config.k8s.io/v1
kind: EncryptionConfiguration
resources:
  - resources:
    - secrets
    providers:
    - kms:
        name: myKMSPlugin
        endpoint: unix:///var/run/k8s-kms-plugin/socket.sock
        cachesize: 1000
        timeout: 3s
        apiVersion: v2              # ← v1.29: v2 为 GA
```

### 2.4 弃用清单

| 弃用项 | 替代方案 | 操作 |
|:---|:---|:---|
| Node v1beta1 metrics | Node v1 metrics | 更新 Prometheus 查询 |
| in-tree cloud providers | 外部云控制器管理器 (CCM) | 迁移至 CCM |
| flowcontrol.apiserver.k8s.io/v1beta2 | v1 | 更新 FlowSchema |

---

## 三、v1.30 核心特性

### 3.1 ValidatingAdmissionPolicy GA (CEL 准入控制)

```yaml
apiVersion: admissionregistration.k8s.io/v1
kind: ValidatingAdmissionPolicy
metadata:
  name: require-labels-policy
spec:
  failurePolicy: Fail
  matchConstraints:
    resourceRules:
    - apiGroups: [""]
      apiVersions: ["v1"]
      operations: ["CREATE", "UPDATE"]
      resources: ["pods"]
  validations:
  - expression: "has(object.metadata.labels) && has(object.metadata.labels.app)"
    message: "Pod 必须包含 app 标签"
  - expression: "object.spec.containers.all(c, has(c.resources.limits) && has(c.resources.requests))"
    message: "所有容器必须设置资源 limits 和 requests"
  - expression: "object.spec.containers.all(c, c.resources.limits.memory == c.resources.requests.memory)"
    message: "内存 limits 必须等于 requests (Guaranteed QoS)"
```

**优势**: 替代大部分 ValidatingWebhook，零延迟、高可用、无需维护 Webhook 服务。

### 3.2 Pod Scheduling Readiness GA (SchedulingGates)

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: gated-pod
spec:
  schedulingGates:
  - name: example.com/network-ready      # ← Pod 被阻塞，直到 gate 被移除
  containers:
  - name: app
    image: nginx
```

```bash
# 控制器确认条件满足后移除 gate
kubectl patch pod gated-pod --type=json \
  -p='[{"op": "remove", "path": "/spec/schedulingGates/0"}]'
```

**场景**: 外部依赖就绪后才调度 Pod (如网络配置、存储准备)。

### 3.3 安全加固

```bash
# v1.30 起禁止匿名用户绑定 cluster-admin
# 检查现有绑定
kubectl get clusterrolebindings -o json | \
  jq '.items[] | select(.subjects[]?.name == "system:anonymous")'
```

---

## 四、v1.31 核心特性

### 4.1 AppArmor Support GA

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: apparmor-pod
  annotations:
    container.apparmor.security.beta.kubernetes.io/nginx: localhost/k8s-apparmor-example-deny-write
spec:
  containers:
  - name: nginx
    image: nginx
    securityContext:
      appArmorProfile:
        type: Localhost
        localhostProfile: k8s-apparmor-example-deny-write    # ← v1.31 GA
```

### 4.2 nftables kube-proxy (Alpha)

```bash
# kube-proxy 启动参数
kube-proxy --proxy-mode nftables

# 或 ConfigMap
apiVersion: v1
kind: ConfigMap
metadata:
  name: kube-proxy
  namespace: kube-system
data:
  config.conf: |
    mode: "nftables"          # ← v1.31 Alpha
```

**优势**: 比 iptables 更快，比 IPVS 更简洁，Linux 内核原生支持。

### 4.3 kubelet OpenTelemetry Tracing (GA)

```yaml
apiVersion: v1
kind: ConfigMap
metadata:
  name: kubelet-config
  namespace: kube-system
data:
  kubelet: |
    tracing:
      endpoint: "otel-collector.monitoring.svc.cluster.local:4317"
      samplingRatePerMillion: 100000    # 10% 采样
```

---

## 五、v1.32 核心特性

### 5.1 Dynamic Resource Allocation (DRA) Beta

```yaml
apiVersion: resource.k8s.io/v1alpha3
kind: ResourceClaim
metadata:
  name: gpu-claim
spec:
  resourceClassName: gpu.nvidia.com
  parametersRef:
    apiGroup: gpu.resource.nvidia.com
    kind: GpuClaimParameters
    name: gpu-params
---
apiVersion: resource.k8s.io/v1alpha3
kind: ResourceClaimTemplate
metadata:
  name: gpu-template
spec:
  spec:
    resourceClassName: gpu.nvidia.com
---
apiVersion: v1
kind: Pod
metadata:
  name: gpu-workload
spec:
  containers:
  - name: trainer
    image: pytorch:latest
    resources:
      claims:
      - name: gpu
  resourceClaims:
  - name: gpu
    source:
      resourceClaimTemplateName: gpu-template    # ← v1.32 DRA Beta
```

### 5.2 TopologyManager Per Pod (Beta)

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: numa-pod
spec:
  topologySpreadConstraints:
  - maxSkew: 1
    topologyKey: topology.kubernetes.io/zone
    whenUnsatisfiable: DoNotSchedule
  containers:
  - name: app
    image: myapp
    resources:
      limits:
        cpu: "4"
        memory: 8Gi
      requests:
        cpu: "4"
        memory: 8Gi
```

---

## 六、v1.33 核心特性

### 6.1 Sidecar 容器 GA

```yaml
apiVersion: v1
kind: Pod
spec:
  initContainers:
  - name: istio-proxy
    image: istio/proxyv2:1.24.0
    restartPolicy: Always              # ← v1.33 GA，生产可用
    lifecycle:
      preStop:
        exec:
          command: ["pilot-agent", "wait", "--timeout", "10s"]
  containers:
  - name: app
    image: myapp:v1.0
```

**Sidecar vs Init 容器对比**:

| 特性 | Init 容器 (传统) | Sidecar 容器 (v1.33 GA) |
|:---|:---|:---|
| 启动顺序 | 严格串行 | 与普通容器并行启动 |
| 终止顺序 | 先终止 | 最后终止 |
| 崩溃行为 | Pod 失败 | 自动重启 |
| 生命周期 | 一次性 | 持续运行 |
| 适用场景 | 初始化任务 | 代理/监控/日志 |

### 6.2 In-Place Pod Vertical Scaling (Alpha)

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: resizable-pod
  annotations:
    resize.policy/container.app: "RestartNotRequired"    # ← v1.33 Alpha
spec:
  containers:
  - name: app
    image: myapp
    resources:
      requests:
        cpu: "500m"
        memory: "512Mi"
      limits:
        cpu: "1000m"
        memory: "1Gi"
```

```bash
# 原地调整资源 (无需重启 Pod)
kubectl patch pod resizable-pod --patch '
{
  "spec": {
    "containers": [{
      "name": "app",
      "resources": {
        "requests": {"cpu": "1000m", "memory": "1Gi"},
        "limits": {"cpu": "2000m", "memory": "2Gi"}
      }
    }]
  }
}'
```

**限制**: Alpha 阶段，需启用 `InPlacePodVerticalScaling` Feature Gate。

### 6.3 Scheduler Queueing Hints (Beta)

```bash
# 启用调度器队列提示
kube-scheduler --feature-gates=SchedulerQueueingHints=true
```

**效果**: 调度器性能提升 10-30%，减少不必要的重试。

### 6.4 Cross-Namespace Volume References (Alpha)

```yaml
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: cross-ns-pvc
  namespace: app-team
spec:
  dataSource:
    kind: PersistentVolumeClaim
    name: source-pvc
    namespace: storage-team          # ← v1.33 Alpha: 跨命名空间引用
  resources:
    requests:
      storage: 10Gi
```

---

## 七、渐进式升级路径

```
v1.29 → v1.30 → v1.31 → v1.32 → v1.33
    │       │       │       │       │
    │       │       │       │       └── Sidecar GA, DRA GA
    │       │       │       └── DRA Beta, TopologyManager Beta
    │       │       └── AppArmor GA, nftables Alpha
    │       └── CEL Admission GA, SchedulingGates GA
    └── Sidecar Beta, ReadWriteOncePod GA

升级检查清单:
1. 检查已弃用 API 使用
2. 验证 CSI 驱动就绪 (v1.30+)
3. 确认 CCM 已部署 (v1.31+)
4. 测试 Sidecar 兼容性 (v1.33)
5. 评估 DRA 需求 (v1.33 GA)
6. 更新监控和告警规则
```

---

## 八、Feature Gate 速查

| Feature Gate | 默认值 v1.33 | 说明 |
|:---|:---|:---|
| `SidecarContainers` | true (GA) | 原生 Sidecar 容器 |
| `DynamicResourceAllocation` | false (GA, 需显式启用) | 动态资源分配 |
| `InPlacePodVerticalScaling` | false (Alpha) | 原地 Pod 资源调整 |
| `NFTablesProxyMode` | false (Beta) | nftables kube-proxy |
| `SchedulerQueueingHints` | true (Beta) | 调度器队列提示 |
| `CrossNamespaceVolumeDataSource` | false (Alpha) | 跨命名空间存储引用 |
| `NodeLogQuery` | false (Alpha) | 节点日志查询 |
| `PodLevelResources` | false (Alpha) | Pod 级别资源限制 |

```bash
# 启用 Feature Gate (kubelet 示例)
# /var/lib/kubelet/config.yaml
featureGates:
  DynamicResourceAllocation: true
  SchedulerQueueingHints: true
```

---

## 九、版本兼容性矩阵

| 组件 | v1.29 | v1.30 | v1.31 | v1.32 | v1.33 |
|:---|:---|:---|:---|:---|:---|
| **kubectl** | ±1 版本 | ±1 版本 | ±1 版本 | ±1 版本 | ±1 版本 |
| **kubelet** | 同版本或-1 | 同版本或-1 | 同版本或-1 | 同版本或-1 | 同版本或-1 |
| **API Server** | 领先 ≤1 | 领先 ≤1 | 领先 ≤1 | 领先 ≤1 | 领先 ≤1 |
| **etcd** | 3.5.9+ | 3.5.10+ | 3.5.12+ | 3.5.13+ | 3.5.15+ |
| **containerd** | 1.7.8+ | 1.7.11+ | 1.7.13+ | 1.7.15+ | 1.7.18+ |
| **CNI** | 0.9.1+ | 1.0.0+ | 1.1.0+ | 1.2.0+ | 1.3.0+ |

---

## 参考链接

- [K8s v1.29 Release Notes](https://kubernetes.io/blog/2023/12/13/kubernetes-v1-29-release/)
- [K8s v1.30 Release Notes](https://kubernetes.io/blog/2024/04/17/kubernetes-v1-30-release/)
- [K8s v1.31 Release Notes](https://kubernetes.io/blog/2024/08/13/kubernetes-v1-31-release/)
- [K8s v1.32 Release Notes](https://kubernetes.io/blog/2024/12/11/kubernetes-v1-32-release/)
- [K8s v1.33 Release Notes](https://kubernetes.io/blog/2025/04/23/kubernetes-v1-33-release/)
- [Feature Gates 文档](https://kubernetes.io/docs/reference/command-line-tools-reference/feature-gates/)
- [K8s 升级指南](https://kubernetes.io/docs/tasks/administer-cluster/cluster-upgrade/)
