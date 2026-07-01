---
title: Kubernetes 核心组件 v1.29 - v1.33 新特性速查
description: '# 替代大部分 ValidatingWebhook 的零延迟方案'
category: architecture-fundamentals
tags:
- k8s
- architecture
- kubernetes
- etcd
- apiserver
- kubelet
- scheduler
- controller-manager
- istio
- statefulset
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- 架构师
- SRE
- 平台工程师
estimated_read_time: 5min
intent_queries:
- Kubernetes 核心组件 v1.29 - v1.33 新特性速查 是什么
- 如何 Kubernetes 核心组件 v1.29 - v1.33 新特性速查
- Kubernetes 1 architecture fundamentals 最佳实践
trigger_keywords:
- Kubernetes
- 核心组件
- v1.29
- v1.33
- 新特性速查
- architecture
- fundamentals
prerequisites:
- kubectl-basics
- kubernetes-concepts
- service-mesh-basics
- etcd-basics
- gpu-scheduling-basics
- observability-basics
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
authors:
- name: KUDIG Team
  role: contributor
cross_refs:
- type: domain
  path: ../domain-13-container-runtime/
  label: '相关知识域: domain-13-container-runtime'
- type: domain
  path: ../domain-01-cluster-fundamentals/
  label: '相关知识域: domain-01-cluster-fundamentals'
- type: cheatsheet
  path: ../domain-17-system-foundation/topic-cheat-sheet/k8s.md
  label: '速查卡: k8s'
- type: cheatsheet
  path: ../domain-17-system-foundation/topic-cheat-sheet/kubectl-scene-cheatsheet.md
  label: '速查卡: kubectl-scene-cheatsheet'
created: "2026-05-23"
---

# [[Kubernetes|Kubernetes]] 核心组件 v1.29 - v1.33 新特性速查

> **适用版本**: Kubernetes v1.29 - v1.33  
> **最后更新**: 2026-04-24  
> **用途**: 按核心组件快速查阅最新版本变更

---

<!-- chunk: 📋 目录 -->
## 📋 目录

- [一、API Server](#一api-server)
- [二、Scheduler](#二scheduler)
- [三、[[kubelet|Kubelet]]](#三kubelet)
- [四、Controller Manager](#四controller-manager)
- [五、Kube-proxy](#五kube-proxy)
- [六、[[etcd|etcd]]](#六etcd)
- [七、Workloads (Pod/Deployment/StatefulSet)](#七workloads-poddeploymentstatefulset)
- [八、Network](#八network)
- [九、Storage](#九storage)
- [十、Security](#十security)

---

<!-- chunk: 一、API Server -->
## 一、API Server

### ValidatingAdmissionPolicy GA (v1.30)

```yaml
# 替代大部分 ValidatingWebhook 的零延迟方案
apiVersion: admissionregistration.k8s.io/v1
kind: ValidatingAdmissionPolicy
metadata:
  name: require-resources
spec:
  failurePolicy: Fail
  matchConstraints:
    resourceRules:
    - apiGroups: ["apps"]
      apiVersions: ["v1"]
      operations: ["CREATE", "UPDATE"]
      resources: ["deployments"]
  validations:
  - expression: |
      object.spec.template.spec.containers.all(
        c, has(c.resources.limits) && has(c.resources.requests)
      )
    message: "所有容器必须设置 resources.limits 和 resources.requests"
  - expression: |
      object.spec.template.spec.containers.all(
        c, c.resources.limits.memory == c.resources.requests.memory
      )
    message: "内存 limits 必须等于 requests"
```

| 版本 | 状态 | 说明 |
|:---|:---|:---|
| v1.26 | Alpha | CEL 表达式验证 |
| v1.28 | Beta | 参数化策略 |
| v1.30 | **GA** | 生产可用，替代 80%+ ValidatingWebhook |

### API 优先级和公平性 (APF) 增强

```yaml
# v1.29+ FlowSchema v1 为唯一版本
apiVersion: flowcontrol.apiserver.k8s.io/v1
kind: FlowSchema
metadata:
  name: workload-high
spec:
  priorityLevelConfiguration:
    name: workload-high
  rules:
  - subjects:
    - kind: ServiceAccount
      serviceAccount:
        name: default
        namespace: production
    resourceRules:
    - verbs: ["*"]
      apiGroups: ["apps"]
      resources: ["deployments"]
```

### v1.33 API 变更

| 变更 | 影响 |
|:---|:---|
| `PodLevelResources` Alpha | Pod 级别资源限制 |
| `CrossNamespaceVolumeDataSource` Alpha | PVC 跨命名空间引用 |
| `StorageVersionAPI` 改进 | 存储版本迁移 |

---

<!-- chunk: 二、Scheduler -->
## 二、Scheduler

### Queueing Hints (v1.33 Beta)

```bash
# 启用调度器队列提示
kube-scheduler --feature-gates=SchedulerQueueingHints=true
```

**效果**: 
- 调度器性能提升 10-30%
- 减少不必要的重试和抢占
- 更智能的 Pod 排队策略

### DRA ([[domain-17-system-foundation/topic-dictionary/scheduling/dynamic-resource-allocation.md|Dynamic Resource Allocation]]) 集成 (v1.33 GA)

```yaml
# v1.33: DRA 控制平面 GA
apiVersion: resource.k8s.io/v1alpha3
kind: ResourceSlice
metadata:
  name: gpu-node-1
spec:
  driver: gpu.nvidia.com
  pool:
    name: node-1-gpu
    generation: 1
    resourceSlices:
    - namedResources:
        instances:
        - name: gpu-0
          attributes:
          - name: memory
            quantity: 80Gi
          - name: product
            string: NVIDIA-A100
```

### TopologyManager Per Pod (v1.33 GA)

```yaml
apiVersion: v1
kind: Pod
spec:
  topologySpreadConstraints:
  - maxSkew: 1
    topologyKey: topology.kubernetes.io/zone
    whenUnsatisfiable: DoNotSchedule
  containers:
  - name: app
    resources:
      limits:
        cpu: "4"
        memory: 8Gi
```

---

<!-- chunk: 三、Kubelet -->
## 三、Kubelet

### Parallel Image Pulls 默认启用 (v1.31)

```yaml
# /var/lib/kubelet/config.yaml
serializeImagePulls: false       # v1.31 起默认 false
maxParallelImagePulls: 5         # 默认 5 个并行拉取
```

### OpenTelemetry Tracing GA (v1.31)

```yaml
# kubelet 配置
tracing:
  endpoint: "otel-collector.monitoring.svc.cluster.local:4317"
  samplingRatePerMillion: 100000
```

### Kubelet Resource Metrics Endpoint (v1.33 Beta)

```bash
# 新端点: /metrics/resource
kubectl get --raw /api/v1/nodes/NODE_NAME/proxy/metrics/resource

# 输出示例:
# resource_scrape_error 0
# node_cpu_usage_seconds_total 12345.67
# node_memory_working_set_bytes 8589934592
```

### In-Place Pod Vertical Scaling (v1.33 Alpha)

```bash
# 启用 Feature Gate
# kubelet 启动参数
--feature-gates=InPlacePodVerticalScaling=true
```

```yaml
# Pod 注解声明可调整
metadata:
  annotations:
    resize.policy/container.app: "RestartNotRequired"
```

### 弃用 --cloud-provider flag (v1.31)

```bash
# 旧方式 (已弃用)
kubelet --cloud-provider=aws

# 新方式: 使用外部云控制器管理器 (CCM)
# kubelet 无需 --cloud-provider 参数
# 单独部署 CCM
kubectl get pods -n kube-system | grep cloud-controller
```

---

<!-- chunk: 四、Controller Manager -->
## 四、Controller Manager

### StatefulSet PodIndexLabel (v1.33 GA)

```yaml
# v1.33: StatefulSet 自动为每个 Pod 添加索引标签
apiVersion: apps/v1
kind: StatefulSet
metadata:
  name: web
spec:
  serviceName: "nginx"
  replicas: 3
  template:
    metadata:
      labels:
        app: nginx
    spec:
      containers:
      - name: nginx
        image: nginx
        env:
        - name: POD_INDEX
          valueFrom:
            fieldRef:
              fieldPath: metadata.labels['apps.kubernetes.io/pod-index']   # ← v1.33 GA
```

### CronJob 时区支持 (v1.25+ Stable)

```yaml
apiVersion: batch/v1
kind: CronJob
metadata:
  name: timezone-cron
spec:
  timeZone: "Asia/Shanghai"        # v1.25+ 支持
  schedule: "0 2 * * *"
  jobTemplate:
    spec:
      template:
        spec:
          containers:
          - name: job
            image: busybox
          restartPolicy: OnFailure
```

---

<!-- chunk: 五、Kube-proxy -->
## 五、Kube-proxy

### nftables 后端 (v1.31 Alpha → v1.33 Beta)

```bash
# kube-proxy ConfigMap
apiVersion: v1
kind: ConfigMap
metadata:
  name: kube-proxy
  namespace: kube-system
data:
  config.conf: |
    mode: "nftables"              # ← v1.33 Beta
    nftables:
      masqueradeAll: false
```

| 后端 | 性能 | 复杂度 | 推荐场景 |
|:---|:---|:---|:---|
| iptables | 中 | 低 | 小集群 (<100 节点) |
| IPVS | 高 | 中 | 大集群 (100+ 节点) |
| nftables | 高 | 低 | 新集群 / Linux 5.13+ |

---

<!-- chunk: 六、etcd -->
## 六、etcd

### 版本兼容性

| K8s 版本 | etcd 推荐版本 | 变更 |
|:---|:---|:---|
| v1.29 | 3.5.9+ | 基础版本 |
| v1.30 | 3.5.10+ | 性能优化 |
| v1.31 | 3.5.12+ | 快照改进 |
| v1.32 | 3.5.13+ | 存储优化 |
| v1.33 | 3.5.15+ | 最新稳定 |

### etcd 加密 (KMS v2 GA v1.29)

```yaml
apiVersion: apiserver.config.k8s.io/v1
kind: EncryptionConfiguration
resources:
  - resources:
    - secrets
    - configmaps
    providers:
    - kms:
        apiVersion: v2              # v1.29 GA
        name: myKMSPlugin
        endpoint: unix:///var/run/k8s-kms-plugin/socket.sock
        timeout: 3s
```

---

<!-- chunk: 七、Workloads (Pod/Deployment/StatefulSet) -->
## 七、Workloads (Pod/Deployment/StatefulSet)

### Sidecar 容器 GA (v1.33)

```yaml
apiVersion: v1
kind: Pod
spec:
  initContainers:
  - name: istio-proxy
    image: istio/proxyv2:1.24.0
    restartPolicy: Always           # ← v1.33 GA: 标识为 Sidecar
    lifecycle:
      preStop:
        exec:
          command: ["pilot-agent", "wait"]
  containers:
  - name: app
    image: myapp:v1.0
```

**生命周期对比**:

```
传统 Init 容器:
  init-1 → init-2 → app容器启动 → (init容器已退出)

Sidecar 容器 (v1.33):
  init-1 → sidecar启动 → app容器启动 → app终止 → sidecar终止
              ↑_________并行运行_________↑
```

### Pod Scheduling Readiness GA (v1.30)

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: gated-pod
spec:
  schedulingGates:
  - name: example.com/network-ready
  containers:
  - name: app
    image: nginx
```

### ReadWriteOncePod GA (v1.29)

```yaml
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: exclusive-pvc
spec:
  accessModes:
  - ReadWriteOncePod              # ← 仅单个 Pod 可挂载
  resources:
    requests:
      storage: 10Gi
```

---

<!-- chunk: 八、Network -->
## 八、Network

### Gateway API v1 (v1.31 Stable)

```yaml
apiVersion: gateway.networking.k8s.io/v1
kind: Gateway
metadata:
  name: example-gateway
spec:
  gatewayClassName: nginx
  listeners:
  - name: https
    protocol: HTTPS
    port: 443
    tls:
      mode: Terminate
      certificateRefs:
      - kind: Secret
        name: example-cert
```

### Service Traffic Distribution (v1.31 Alpha)

```yaml
apiVersion: v1
kind: Service
metadata:
  name: my-service
spec:
  selector:
    app: my-app
  ports:
  - port: 80
  trafficDistribution:
    preferences:
    - zone: us-east-1a              # ← v1.31 Alpha: 拓扑感知路由
      weight: 70
    - zone: us-east-1b
      weight: 30
```

---

<!-- chunk: 九、Storage -->
## 九、Storage

### VolumeAttributesClass (v1.33 Alpha)

```yaml
apiVersion: storage.k8s.io/v1alpha1
kind: VolumeAttributesClass
metadata:
  name: premium-io
driverName: ebs.csi.aws.com
parameters:
  iops: "16000"
  throughput: "1000"
---
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: my-pvc
spec:
  volumeAttributesClassName: premium-io    # ← v1.33 Alpha
  resources:
    requests:
      storage: 100Gi
```

### Cross-Namespace Volume References (v1.33 Alpha)

```yaml
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: clone-pvc
  namespace: team-a
spec:
  dataSource:
    kind: PersistentVolumeClaim
    name: source-pvc
    namespace: storage-team          # ← v1.33 Alpha
  resources:
    requests:
      storage: 10Gi
```

### PersistentVolume Last Phase Transition Time (v1.31 GA)

```bash
# 查看 PV 最后状态转换时间
kubectl get pv PV_NAME -o jsonpath='{.status.lastPhaseTransitionTime}'
```

---

<!-- chunk: 十、Security -->
## 十、Security

### AppArmor GA (v1.31)

```yaml
apiVersion: v1
kind: Pod
spec:
  containers:
  - name: app
    image: nginx
    securityContext:
      appArmorProfile:
        type: Localhost
        localhostProfile: k8s-apparmor-deny-write
```

### BoundServiceAccountTokenVolume GA (v1.30)

```bash
# v1.30 起 ServiceAccount Token 默认 1 小时过期
# 检查 Token 绑定
kubectl get pod POD_NAME -o jsonpath='{.spec.volumes[?(@.name=="kube-api-access")].projected.sources[0].serviceAccountToken.expirationSeconds}'

# 输出: 3607 (约1小时)
```

### Pod Security Admission (稳定)

```yaml
apiVersion: v1
kind: Namespace
metadata:
  name: restricted-ns
  labels:
    pod-security.kubernetes.io/enforce: restricted
    pod-security.kubernetes.io/enforce-version: v1.33
    pod-security.kubernetes.io/audit: restricted
    pod-security.kubernetes.io/warn: restricted
```

### 匿名用户安全加固 (v1.30)

```bash
# v1.30 起禁止 system:anonymous 绑定 cluster-admin
# 检查现有绑定
kubectl get clusterrolebindings -o json | \
  jq '.items[] | select(.subjects[]?.name == "system:anonymous") | .metadata.name'
```

---

<!-- chunk: 快速命令参考 -->
## 快速命令参考

```bash
# 检查当前 K8s 版本
kubectl version

# 查看所有 Feature Gates
kubectl get --raw /api/v1/nodes/NODE_NAME/proxy/configz | jq '.kubeletconfig.featureGates'

# 检查已弃用 API
kubectl get --raw /metrics | grep apiserver_requested_deprecated_apis

# 查看 API 版本
kubectl api-versions | grep -E "v1beta|v1alpha"

# 检查 ValidatingAdmissionPolicy
kubectl get validatingadmissionpolicies

# 检查节点支持的 AppArmor
kubectl get nodes -o jsonpath='{.items[0].status.nodeInfo.operatingSystem}'
cat /sys/kernel/security/apparmor/profiles | head

# 检查 Sidecar 容器
kubectl get pods -A -o json | jq '.items[].spec.initContainers[]? | select(.restartPolicy == "Always") | .name'
```

---

<!-- chunk: 参考链接 -->
## 参考链接

- [K8s Feature Gates](https://kubernetes.io/docs/reference/command-line-tools-reference/feature-gates/)
- [K8s API 变更日志](https://kubernetes.io/docs/reference/using-api/deprecation-guide/)
- [K8s 版本发布说明](https://kubernetes.io/releases/)
- [Gateway API v1](https://gateway-api.sigs.k8s.io/)
- [DRA 文档](https://kubernetes.io/docs/concepts/scheduling-eviction/dynamic-resource-allocation/)

---

<!-- chunk: Obsidian 相关文档 -->
## Obsidian 相关文档

- domain-01-cluster-fundamentals MOC
- [[domain-01-cluster-fundamentals/README.md|Domain-1: Kubernetes架构基础]]
- Domain-1 架构基础 — 开源项目索引
- Kubernetes 架构全景图
- Kubernetes 核心组件深度剖析
- 03 - 功能和API表
- 04 - Kubernetes 源码结构深度解析
- kubectl 命令完整参考
- 06 - 集群配置参数完全参考
- 07 - 升级路径与策略指南
- 08 - 多租户架构设计 (Multi-Tenancy Architecture)
- 09 - 边缘计算集成架构 (KubeEdge/OpenYurt)

## See Also

- 99-kubectl-v1.29-v1.33-new-commands-guide
- 99-kubernetes-api-version-matrix
- 99-kubernetes-core-features-mermaid-diagrams
- 99-kubernetes-v1.25-v1.33-feature-comparison-table
