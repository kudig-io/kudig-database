# Kubernetes v1.29-v1.33 工作负载管理新特性指南

> **适用版本**: Kubernetes v1.29 - v1.33  
> **最后更新**: 2026-04-24  
> **用途**: 工作负载管理新特性详解与生产实践

---

## 📋 目录

- [一、Sidecar 容器 (v1.33 GA)](#一sidecar-容器-v133-ga)
- [二、原地 Pod 资源调整 (v1.33 Alpha)](#二原地-pod-资源调整-v133-alpha)
- [三、Job 成功策略 (v1.31 Alpha)](#三job-成功策略-v131-alpha)
- [四、Pod 调度就绪 (v1.30 GA)](#四pod-调度就绪-v130-ga)
- [五、Parallel Image Pulls 优化 (v1.31)](#五parallel-image-pulls-优化-v131)
- [六、AppArmor 安全配置 (v1.31 GA)](#六apparmor-安全配置-v131-ga)
- [七、用户命名空间隔离 (v1.33 GA)](#七用户命名空间隔离-v133-ga)
- [八、Pod 失败策略增强 (v1.32)](#八pod-失败策略增强-v132)
- [九、CRD 与 Operator 新特性](#九crd-与-operator-新特性)

---

## 一、Sidecar 容器 (v1.33 GA)

### 1.1 核心概念

原生 Sidecar 容器通过 `initContainers` 中的 `restartPolicy: Always` 实现，解决了以下问题：

```
问题场景（v1.33 之前）:
├── Envoy Sidecar 需要在主容器前启动
├── 需要复杂的 postStart 生命周期钩子
├── 主容器终止时，Sidecar 无法自动优雅终止
└── Job 完成后，Sidecar 容器阻止 Pod 完成

解决方案（v1.33 GA）:
├── initContainers 中支持 restartPolicy: Always
├── Sidecar 与普通 initContainers 按定义顺序启动
├── 主容器终止后，Sidecar 自动收到 SIGTERM
└── Job 完成后，Sidecar 自动终止
```

### 1.2 完整配置示例

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: webapp-with-sidecar
spec:
  initContainers:
    # 1. Sidecar 容器：先启动并保持运行
    - name: istio-proxy
      image: istio/proxyv2:1.24.0
      restartPolicy: Always
      args:
        - proxy
        - sidecar
      resources:
        requests:
          cpu: 100m
          memory: 128Mi
      volumeMounts:
        - name: istio-envoy
          mountPath: /etc/istio/proxy
      
    # 2. 普通 initContainer：完成后退出
    - name: db-migrate
      image: myapp:v2.0
      command: ["python", "manage.py", "migrate"]
      env:
        - name: DATABASE_URL
          valueFrom:
            secretKeyRef:
              name: db-secret
              key: url
      
  containers:
    # 3. 主容器
    - name: webapp
      image: myapp:v2.0
      ports:
        - containerPort: 8080
      env:
        - name: PROXY_PORT
          value: "15001"
```

### 1.3 Job 中的 Sidecar

```yaml
apiVersion: batch/v1
kind: Job
metadata:
  name: data-processing
spec:
  template:
    spec:
      initContainers:
        - name: metrics-exporter
          image: prometheus/node-exporter:v1.8
          restartPolicy: Always
          # Sidecar 在 Job 完成后自动终止
      containers:
        - name: processor
          image: data-processor:v1.0
          command: ["process", "--input", "/data/input.csv"]
      restartPolicy: Never
```

### 1.4 生命周期顺序

```
Pod 启动:
  Phase: Pending
    ├── 1. istio-proxy (Sidecar) 启动 → Running
    ├── 2. db-migrate 运行 → Completed
    ├── 3. webapp 启动 → Running
    └── Pod Phase → Running

Pod 终止:
  Terminating
    ├── 1. webapp 收到 SIGTERM → 优雅终止 → Completed
    ├── 2. istio-proxy 收到 SIGTERM → 优雅终止 → Completed
    └── Pod Phase → Succeeded/Failed
```

### 1.5 生产检查清单

```bash
# 检查 Sidecar 状态
kubectl get pod webapp-with-sidecar -o jsonpath='{range .status.initContainerStatuses[*]}{.name}{"\t"}{.state}{"\n"}{end}'

# 预期输出:
# istio-proxy     map[running:map[startedAt:2026-04-24T10:00:00Z]]
# db-migrate      map[terminated:map[exitCode:0 reason:Completed]]
```

---

## 二、原地 Pod 资源调整 (v1.33 Alpha)

### 2.1 核心概念

允许在不重启 Pod 的情况下调整 CPU/Memory 的 requests 和 limits。

### 2.2 启用条件

```bash
# 1. Kubelet 启用 Feature Gate
# /var/lib/kubelet/config.yaml
featureGates:
  InPlacePodVerticalScaling: true

# 2. 需要 cgroup v2 (推荐)
stat -fc %T /sys/fs/cgroup/
# 输出: cgroup2fs
```

### 2.3 Pod 配置

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: resize-demo
  annotations:
    resize.kubernetes.io/resources: "true"
spec:
  containers:
    - name: app
      image: nginx:1.25
      resources:
        requests:
          cpu: "100m"
          memory: "128Mi"
        limits:
          cpu: "200m"
          memory: "256Mi"
        resizePolicy:
          - resourceName: cpu
            restartPolicy: NotRequired  # CPU 调整无需重启
          - resourceName: memory
            restartPolicy: NotRequired  # Memory 调整无需重启
```

### 2.4 执行调整

```bash
# 增加资源
kubectl patch pod resize-demo --patch '{
  "spec": {
    "containers": [{
      "name": "app",
      "resources": {
        "requests": {"cpu": "200m", "memory": "256Mi"},
        "limits": {"cpu": "500m", "memory": "512Mi"}
      }
    }]
  }
}'

# 查看调整状态
kubectl get pod resize-demo -o jsonpath='{.status.resizeStatus}'
# Proposed / InProgress / Complete / Deferred / Infeasible

# 查看实际分配
kubectl get pod resize-demo -o jsonpath='{.status.containerStatuses[0].allocatedResources}'
```

### 2.5 限制与注意事项

```
⚠️ 重要限制:
├── 仅支持增加 resources（不支持减少 limits）
├── 内存 limits 增加可能需要容器重启（取决于 resizePolicy）
├── 节点必须有足够可分配资源
├── 不支持降配（limits 只能增加）
├── 与 VPA 配合使用效果更好
└── v1.33 为 Alpha，生产环境需谨慎启用
```

---

## 三、Job 成功策略 (v1.31 Alpha)

### 3.1 核心概念

允许自定义 Job 的"成功"定义，支持"部分成功即整体成功"。

### 3.2 配置示例

```yaml
apiVersion: batch/v1
kind: Job
metadata:
  name: distributed-training
spec:
  parallelism: 10
  completions: 10
  successPolicy:
    rules:
      # 规则 1: 至少 5 个 Pod 成功
      - succeededCount: 5
      # 规则 2: 索引 0 的 Pod（主节点）必须成功
      - succeededIndexes: "0"
        succeededCount: 1
  template:
    spec:
      containers:
        - name: trainer
          image: pytorch-training:v1.0
          command: ["python", "train.py"]
      restartPolicy: Never
```

### 3.3 使用场景

| 场景 | 成功策略 |
|:---|:---|
| 分布式训练 | 主节点成功 + 任意 N 个工作节点成功 |
| 数据处理 | 90% 的批次成功即视为成功 |
| 网格计算 | 至少 1 个节点发现结果 |

---

## 四、Pod 调度就绪 (v1.30 GA)

### 4.1 核心概念

通过 `schedulingGates` 控制 Pod 何时允许被调度。

### 4.2 配置示例

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: gated-pod
spec:
  schedulingGates:
    - name: example.com/gate-a
    - name: example.com/gate-b
  containers:
    - name: app
      image: nginx:1.25
```

### 4.3 调度门控管理

```bash
# Pod 处于 SchedulingGated 状态
kubectl get pod gated-pod
# NAME        READY   STATUS            RESTARTS   AGE
# gated-pod   0/1     SchedulingGated   0          10s

# 移除门控（允许调度）
kubectl patch pod gated-pod --type=merge -p \
  '{"spec":{"schedulingGates":[]}}'

# 验证调度
kubectl get pod gated-pod
# NAME        READY   STATUS    RESTARTS   AGE
# gated-pod   1/1     Running   0          5s
```

### 4.4 使用场景

```
场景 1: 审批工作流
  提交 Pod → 等待审批 → 移除 gate → 调度

场景 2: 资源准备
  创建 Pod → 等待 Storage/NW 就绪 → 移除 gate → 调度

场景 3: 蓝绿部署控制
  创建新版本 Pod → Gate 阻止调度 → 验证通过 → 移除 gate
```

---

## 五、Parallel Image Pulls 优化 (v1.31)

### 5.1 核心概念

v1.31 默认启用并行镜像拉取，加速 Pod 启动。

### 5.2 Kubelet 配置

```yaml
# /var/lib/kubelet/config.yaml
serializeImagePulls: false  # 默认: false (v1.31+)
maxParallelImagePulls: 5    # 最大并行拉取数
```

### 5.3 效果对比

```
串行拉取 (serializeImagePulls: true):
├── 镜像 A: 30s
├── 镜像 B: 45s
├── 镜像 C: 20s
└── 总计: 95s

并行拉取 (serializeImagePulls: false):
├── 镜像 A: 30s (并行)
├── 镜像 B: 45s (并行)
├── 镜像 C: 20s (并行)
└── 总计: 45s (取最大值)
```

---

## 六、AppArmor 安全配置 (v1.31 GA)

### 6.1 核心概念

AppArmor 配置文件配置从注解迁移到 API 字段。

### 6.2 配置示例

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: apparmor-demo
spec:
  securityContext:
    appArmorProfile:
      type: Localhost
      localhostProfile: k8s-default
  containers:
    - name: app
      image: nginx:1.25
      securityContext:
        appArmorProfile:
          type: RuntimeDefault  # 使用运行时默认配置
```

### 6.3 Profile 类型

| 类型 | 说明 |
|:---|:---|
| `RuntimeDefault` | 容器运行时的默认 AppArmor 配置 |
| `Localhost` | 节点上预定义的 profile |
| `Unconfined` | 不限制（不推荐） |

---

## 七、用户命名空间隔离 (v1.33 GA)

### 7.1 核心概念

将容器内的 root (UID 0) 映射到节点上的非特权 UID。

### 7.2 前置条件

```bash
# 1. 内核 >= 5.19
uname -r

# 2. 启用用户命名空间
sysctl user.max_user_namespaces
# 输出: user.max_user_namespaces = 28633

# 3. 容器运行时支持 (containerd 1.7+, CRI-O 1.28+)
```

### 7.3 Pod 配置

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: userns-demo
spec:
  hostUsers: false  # 启用用户命名空间
  containers:
    - name: app
      image: nginx:1.25
      securityContext:
        runAsUser: 0  # 容器内 root
```

### 7.4 安全效果

```
容器内执行:
  $ id
  uid=0(root) gid=0(root)

节点上对应进程:
  $ ps -o pid,uid,comm -p $(pgrep nginx)
  PID   UID  COMM
  1234  65536 nginx
  
逃逸后权限:
  节点 UID 65536 = 无特权用户
```

---

## 八、Pod 失败策略增强 (v1.32)

### 8.1 Job Pod 替换策略

```yaml
apiVersion: batch/v1
kind: Job
metadata:
  name: retry-job
spec:
  podReplacementPolicy: Failed  # 仅在 Pod 失败时替换
  # 或: TerminatingOrFailed (Pod 终止或失败时替换)
  backoffLimit: 3
  template:
    spec:
      containers:
        - name: worker
          image: batch-worker:v1.0
      restartPolicy: Never
```

### 8.2 Pod 失败策略

```yaml
apiVersion: batch/v1
kind: Job
metadata:
  name: training-job
spec:
  podFailurePolicy:
    rules:
      # 规则 1: OOMKilled 不记为失败
      - action: Ignore
        onExitCodes:
          containerName: trainer
          operator: In
          values: [137]
      # 规则 2: 特定错误码终止 Job
      - action: FailJob
        onExitCodes:
          containerName: trainer
          operator: In
          values: [42]
      # 规则 3: 节点不可调度时重试
      - action: Count
        onPodConditions:
          - type: DisruptionTarget
            status: "True"
  template:
    spec:
      containers:
        - name: trainer
          image: pytorch:v2.0
```

---

## 九、CRD 与 Operator 新特性

### 9.1 CRD 字段选择器 (v1.32)

```yaml
# CRD 定义中启用字段选择器
apiVersion: apiextensions.k8s.io/v1
kind: CustomResourceDefinition
spec:
  versions:
    - name: v1
      selectableFields:
        - jsonPath: .spec.tier
        - jsonPath: .spec.region
```

```bash
# 使用字段选择器查询
kubectl get myapps --field-selector spec.tier=premium
```

### 9.2 CRD 默认版本迁移

```bash
# v1.33 支持更平滑的 CRD 版本弃用
kubectl deprecate crd myapps.example.com v1beta1 --to=v1
```

### 9.3 Operator 开发建议

```go
// 使用 controller-runtime v0.19+ 支持的新特性
import ctrl "sigs.k8s.io/controller-runtime"

// Sidecar 感知的控制器
func (r *MyAppReconciler) Reconcile(ctx context.Context, req ctrl.Request) (ctrl.Result, error) {
    // 检查 Pod 中 Sidecar 的就绪状态
    for _, container := range pod.Status.InitContainerStatuses {
        if container.RestartCount > 0 {
            // Sidecar 可能已重启，需要处理
        }
    }
}
```

---

## 附录：工作负载特性快速决策树

```
需要 Sidecar?
├── 是 → 使用 restartPolicy: Always (v1.33 GA)
│   └── 需要 Job 完成时终止 Sidecar?
│       └── 原生支持，无需额外处理
└── 否 → 使用普通 initContainers

需要动态调整资源?
├── 是 → 启用 InPlacePodVerticalScaling (v1.33 Alpha)
│   └── 需要减少 limits?
│       └── 暂不支持，需重建 Pod
└── 否 → 使用 VPA 自动调整（需重建）

需要自定义 Job 成功条件?
├── 是 → 使用 successPolicy (v1.31 Alpha)
└── 否 → 默认所有 Pod 成功

需要更强的安全隔离?
├── 是 → 启用 hostUsers: false (v1.33 GA)
│   └── 需要容器内 root?
│       └── 支持，但映射到节点非特权 UID
└── 否 → 使用传统 securityContext
```

---

## 参考链接

- [Sidecar Containers](https://kubernetes.io/docs/concepts/workloads/pods/sidecar-containers/)
- [Resize CPU and Memory Resources](https://kubernetes.io/docs/tasks/configure-pod-container/resize-container-resources/)
- [Job Success Policy](https://kubernetes.io/docs/concepts/workloads/controllers/job/#success-policy)
- [Pod Scheduling Readiness](https://kubernetes.io/docs/concepts/scheduling-eviction/pod-scheduling-readiness/)
- [User Namespaces](https://kubernetes.io/docs/concepts/workloads/pods/user-namespaces/)
