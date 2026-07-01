---
title: Kubernetes v1.33 实战案例集
description: '- [案例六：跨命名空间存储引用](#案例六跨命名空间存储引用)'
summary: '- [案例六：跨命名空间存储引用](#案例六跨命名空间存储引用)'
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
- envoy
- helm
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- 架构师
- SRE
- 平台工程师
estimated_read_time: 5min
intent_queries:
- Kubernetes v1.33 实战案例集 是什么
- 如何 Kubernetes v1.33 实战案例集
- Kubernetes 1 architecture fundamentals 最佳实践
trigger_keywords:
- Kubernetes
- v1.33
- 实战案例集
- architecture
- fundamentals
prerequisites:
- kubectl-basics
- kubernetes-concepts
- helm-basics
- etcd-basics
- gpu-scheduling-basics
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
---



# [[Kubernetes|Kubernetes]] v1.33 实战案例集

> **适用版本**: Kubernetes v1.33  
> **最后更新**: 2026-04-24  
> **用途**: 新特性落地实战，含完整 YAML 与脚本

---

<!-- chunk: 📋 目录 -->
## 📋 目录

- [案例一：Sidecar 容器实现优雅启动/终止](#案例一sidecar-容器实现优雅启动终止)
- [案例二：CEL 准入策略实现资源配额验证](#案例二cel-准入策略实现资源配额验证)
- [案例三：DRA 分配 GPU 资源](#案例三dra-分配-gpu-资源)
- [案例四：nftables kube-proxy 替换 iptables](#案例四nftables-kube-proxy-替换-iptables)
- [案例五：原地 Pod 资源调整](#案例五原地-pod-资源调整)
- [案例六：跨命名空间存储引用](#案例六跨命名空间存储引用)
- [案例七：VolumeAttributesClass 动态调整存储性能](#案例七volumeattributesclass-动态调整存储性能)
- [案例八：SELinux 挂载优化](#案例八selinux-挂载优化)
- [案例九：协调领导者选举](#案例九协调领导者选举)
- [案例十：节点 Swap 支持配置](#案例十节点-swap-支持配置)
- [案例十一：Pod 级 NUMA 拓扑策略](#案例十一pod-级-numa-拓扑策略)
- [案例十二：用户命名空间安全隔离](#案例十二用户命名空间安全隔离)
- [案例十三：Kubectl 节点日志查询](#案例十三kubectl-节点日志查询)
- [案例十四：Queueing Hints 优化调度性能](#案例十四queueing-hints-优化调度性能)

---

<!-- chunk: 案例一：Sidecar 容器实现优雅启动/终止 -->
## 案例一：Sidecar 容器实现优雅启动/终止

### 场景

在 v1.33 之前，Sidecar 容器需要通过复杂的 `initContainers` + `postStart` 生命周期钩子或外部控制器来实现。v1.33 GA 的原生 Sidecar 容器支持，允许在 `initContainers` 中指定 `restartPolicy: Always`，实现真正的 Sidecar 生命周期管理。

### 配置

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: app-with-sidecar
spec:
  initContainers:
    # Sidecar 容器：restartPolicy: Always
    - name: envoy-sidecar
      image: envoyproxy/envoy:v1.31
      restartPolicy: Always  # 🔑 关键：标记为 Sidecar
      volumeMounts:
        - name: shared-logs
          mountPath: /var/log/envoy
    # 普通 initContainer
    - name: migrate-db
      image: myapp:v1.0
      command: ["migrate", "up"]
  containers:
    - name: myapp
      image: myapp:v1.0
      ports:
        - containerPort: 8080
      volumeMounts:
        - name: shared-logs
          mountPath: /var/log/app
  volumes:
    - name: shared-logs
      emptyDir: {}
```

### 行为说明

```
启动顺序:
1. envoy-sidecar (Sidecar) 启动并保持运行
2. migrate-db (普通 initContainer) 运行完成后退出
3. myapp (主容器) 启动
4. envoy-sidecar 在 myapp 终止后自动终止

生命周期:
├── 启动: Sidecar 在普通 initContainers 之前启动
├── 运行: Sidecar 与主容器并行运行
└── 终止: Sidecar 在主容器终止后终止（Graceful shutdown）
```

### 验证

```bash
# 查看 Pod 状态
kubectl get pod app-with-sidecar -o jsonpath='{.status.initContainerStatuses}' | jq

# 查看 Sidecar 容器日志
kubectl logs app-with-sidecar -c envoy-sidecar
```

---

<!-- chunk: 案例二：CEL 准入策略实现资源配额验证 -->
## 案例二：CEL 准入策略实现资源配额验证

### 场景

用 ValidatingAdmissionPolicy (v1.30 GA) 替代 Webhook，实现无依赖的准入验证。

### 配置

```yaml
# ValidatingAdmissionPolicy：禁止无资源限制的 Pod
apiVersion: admissionregistration.k8s.io/v1
kind: ValidatingAdmissionPolicy
metadata:
  name: require-resource-limits
spec:
  failurePolicy: Fail
  matchConstraints:
    resourceRules:
      - apiGroups: [""]
        apiVersions: ["v1"]
        operations: ["CREATE", "UPDATE"]
        resources: ["pods"]
  variables:
    - name: hasCpuLimit
      expression: "object.spec.containers.all(c, has(c.resources) && has(c.resources.limits) && has(c.resources.limits.cpu))"
    - name: hasMemLimit
      expression: "object.spec.containers.all(c, has(c.resources) && has(c.resources.limits) && has(c.resources.limits.memory))"
  validations:
    - expression: "variables.hasCpuLimit"
      message: "所有容器必须设置 CPU limit"
    - expression: "variables.hasMemLimit"
      message: "所有容器必须设置 memory limit"
---
# 绑定到所有命名空间
apiVersion: admissionregistration.k8s.io/v1
kind: ValidatingAdmissionPolicyBinding
metadata:
  name: require-resource-limits-binding
spec:
  policyName: require-resource-limits
  validationActions: [Deny]
```

### 高级 CEL 策略：禁止 latest 标签

```yaml
apiVersion: admissionregistration.k8s.io/v1
kind: ValidatingAdmissionPolicy
metadata:
  name: disallow-latest-tag
spec:
  failurePolicy: Fail
  matchConstraints:
    resourceRules:
      - apiGroups: [""]
        apiVersions: ["v1"]
        operations: ["CREATE", "UPDATE"]
        resources: ["pods"]
  validations:
    - expression: |
        object.spec.containers.all(c, !c.image.endsWith(':latest'))
      message: "禁止使用 :latest 镜像标签"
    - expression: |
        object.spec.initContainers.all(c, !c.image.endsWith(':latest'))
      message: "InitContainers 禁止使用 :latest 镜像标签"
```

### 测试验证

```bash
# 应该被拒绝
kubectl run test --image=nginx:latest --dry-run=server
# Error from server: 禁止使用 :latest 镜像标签

# 应该被允许
kubectl run test --image=nginx:1.25 --dry-run=server
# pod/test created (dry run)
```

---

<!-- chunk: 案例三：DRA 分配 GPU 资源 -->
## 案例三：DRA 分配 GPU 资源

### 场景

[[domain-17-system-foundation/topic-dictionary/scheduling/dynamic-resource-allocation.md|Dynamic Resource Allocation]] (v1.33 GA) 允许 Pod 请求 GPU/FPGA 等外部资源，替代 Device Plugin 方案。

### 前置条件

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `helm upgrade/install`：部署/升级 release

```bash
# 1. 启用 DRA
# kube-apiserver, kube-scheduler, kubelet 都要启用
# --feature-gates=DynamicResourceAllocation=true

# 2. 部署 NVIDIA DRA Driver
helm install nvidia-dra-driver nvidia/dra-driver \
  --namespace nvidia-dra \
  --create-namespace
```

### ResourceClaimTemplate

```yaml
# ResourceClaimTemplate：定义 GPU 资源模板
apiVersion: resource.k8s.io/v1beta1
kind: ResourceClaimTemplate
metadata:
  name: gpu-claim-template
  namespace: default
spec:
  spec:
    resourceClassName: nvidia.com/gpu
    parametersRef:
      apiGroup: resource.nvidia.com
      kind: GpuConfig
      name: gpu-params
---
# GpuConfig：GPU 参数
apiVersion: resource.nvidia.com/v1alpha1
kind: GpuConfig
metadata:
  name: gpu-params
  namespace: default
spec:
  memory: "24Gi"
  multiNodeEnabled: false
```

### Pod 使用 GPU

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: gpu-training
spec:
  containers:
    - name: pytorch
      image: nvcr.io/nvidia/pytorch:24.03-py3
      resources:
        claims:
          - name: gpu
  resourceClaims:
    - name: gpu
      source:
        resourceClaimTemplateName: gpu-claim-template
```

### 验证

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

```bash
# 查看 ResourceClaim
kubectl get resourceclaims

# 查看 DRA 分配
kubectl describe resourceclaim gpu-training-gpu

# 查看 Pod 中的 GPU
kubectl exec gpu-training -- nvidia-smi
```

---

<!-- chunk: 案例四：nftables kube-proxy 替换 iptables -->
## 案例四：nftables kube-proxy 替换 iptables

### 场景

nftables (v1.33 Beta) 是 kube-proxy 的后端替代方案，性能优于传统 iptables，尤其是在大规模集群中。

### 前置条件

```bash
# 1. Linux 内核 >= 5.13（nftables 支持）
uname -r

# 2. 确认 nft 命令可用
which nft

# 3. 确认内核模块加载
lsmod | grep nft
```

### 配置 kube-proxy

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

```bash
# 方式一：修改 kube-proxy ConfigMap
kubectl edit cm kube-proxy -n kube-system
```

```yaml
# kube-proxy ConfigMap 内容
apiVersion: v1
kind: ConfigMap
metadata:
  name: kube-proxy
  namespace: kube-system
data:
  config.conf: |
    mode: "nftables"  # 🔑 从 iptables/ipvs 改为 nftables
    featureGates:
      NFTablesProxyMode: true
    nftables:
      masqueradeAll: false
      syncPeriod: 30s
      minSyncPeriod: 1s
```

```bash
# 方式二：kubeadm 初始化时指定
kubeadm init --config=kubeadm-config.yaml
```

```yaml
# kubeadm-config.yaml
apiVersion: kubeproxy.config.k8s.io/v1alpha1
kind: KubeProxyConfiguration
mode: nftables
featureGates:
  NFTablesProxyMode: true
```

### 重启 kube-proxy

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl delete`：删除资源（可由声明式清单重建）

```bash
# 删除 kube-proxy Pod 触发重建
kubectl delete pod -n kube-system -l k8s-app=kube-proxy

# 验证 nftables 规则
kubectl get pods -n kube-system -l k8s-app=kube-proxy
kubectl logs -n kube-system -l k8s-app=kube-proxy | grep -i nftables
```

### 对比 iptables vs nftables

| 特性 | iptables | nftables |
|:---|:---|:---|
| 内核版本要求 | >= 2.6 | >= 5.13 |
| 规则数量限制 | 有限制 | 无限制 |
| 增量更新 | 慢（全量刷新） | 快（增量更新） |
| IPv6 支持 | 需要 ip6tables | 原生支持 |
| 调试难度 | 较复杂 | 相对简单 |
| 性能（10K [[Service|Service]]） | 慢 | 快 30-50% |

---

<!-- chunk: 案例五：原地 Pod 资源调整 -->
## 案例五：原地 Pod 资源调整

### 场景

In-Place Pod Vertical Scaling (v1.33 Alpha，需显式启用) 允许在不重启 Pod 的情况下调整 CPU/Memory 资源。

### 前置条件

```bash
# kubelet 启用 Feature Gate
# /var/lib/kubelet/config.yaml
# featureGates:
#   InPlacePodVerticalScaling: true
```

### Pod 配置

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: resize-demo
  annotations:
    # 允许原地调整
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
```

### 执行原地调整

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

```bash
# 方法 1：直接 PATCH
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

# 方法 2：编辑
kubectl edit pod resize-demo
```

### 验证调整结果

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

```bash
# 查看 Pod 状态
kubectl get pod resize-demo -o yaml | grep -A 5 "resizeStatus"

# 查看 cgroup 限制
kubectl exec resize-demo -- cat /sys/fs/cgroup/cpu.max
kubectl exec resize-demo -- cat /sys/fs/cgroup/memory.max
```

### 限制说明

```
⚠️ In-Place Resize 限制:
├── 仅支持 CPU 和 Memory 调整
├── 不支持减少 limits（只能增加）
├── 不支持减少 requests（某些场景）
├── 调整时 Pod 不会重启，但应用可能感知到 cgroup 变化
└── 需要 cgroup v2（推荐）或特定 cgroup v1 配置
```

---

<!-- chunk: 案例六：跨命名空间存储引用 -->
## 案例六：跨命名空间存储引用

### 场景

CrossNamespaceVolumeDataSource (v1.33 Alpha) 允许在命名空间 A 中创建 PVC，引用命名空间 B 中的 VolumeSnapshot 或 PVC 作为数据源。

### 前置条件

```bash
# kube-apiserver, kubelet 启用 FG
# --feature-gates=CrossNamespaceVolumeDataSource=true
```

### 创建 VolumeSnapshot（在 source-ns）

```yaml
apiVersion: snapshot.storage.k8s.io/v1
kind: VolumeSnapshot
metadata:
  name: db-snapshot
  namespace: source-ns
spec:
  volumeSnapshotClassName: csi-snapclass
  source:
    persistentVolumeClaimName: db-data
```

### 创建 PVC 引用跨命名空间快照

```yaml
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: restored-db
  namespace: target-ns
spec:
  accessModes:
    - ReadWriteOnce
  storageClassName: standard
  resources:
    requests:
      storage: 10Gi
  dataSourceRef:
    apiGroup: snapshot.storage.k8s.io
    kind: VolumeSnapshot
    name: db-snapshot
    namespace: source-ns  # 🔑 跨命名空间引用
```

### 使用 RBAC 授权

```yaml
apiVersion: rbac.authorization.k8s.io/v1
kind: ClusterRole
metadata:
  name: cross-namespace-snapshot-reader
rules:
  - apiGroups: ["snapshot.storage.k8s.io"]
    resources: ["volumesnapshots"]
    verbs: ["get", "list"]
---
apiVersion: rbac.authorization.k8s.io/v1
kind: ClusterRoleBinding
metadata:
  name: cross-namespace-snapshot-binding
subjects:
  - kind: ServiceAccount
    name: csi-provisioner
    namespace: kube-system
roleRef:
  kind: ClusterRole
  name: cross-namespace-snapshot-reader
  apiGroup: rbac.authorization.k8s.io
```

---

<!-- chunk: 案例七：VolumeAttributesClass 动态调整存储性能 -->
## 案例七：VolumeAttributesClass 动态调整存储性能

### 场景

VolumeAttributesClass (v1.31 Alpha) 允许在不重新创建 PVC 的情况下动态调整存储性能（如 IOPS、吞吐量）。

### 前置条件

```bash
# kube-apiserver, kube-controller-manager 启用 FG
# --feature-gates=VolumeAttributesClass=true
```

### 定义 VolumeAttributesClass

```yaml
apiVersion: storage.k8s.io/v1alpha1
kind: VolumeAttributesClass
metadata:
  name: high-performance
parameters:
  iops: "3000"
  throughput: "125"
---
apiVersion: storage.k8s.io/v1alpha1
kind: VolumeAttributesClass
metadata:
  name: standard-performance
parameters:
  iops: "1000"
  throughput: "50"
```

### 创建带 VolumeAttributesClass 的 PVC

```yaml
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: dynamic-perf-pvc
spec:
  accessModes:
    - ReadWriteOnce
  storageClassName: ebs-sc
  volumeAttributesClassName: standard-performance  # 🔑 指定性能等级
  resources:
    requests:
      storage: 50Gi
```

### 动态调整性能

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

```bash
# 修改 PVC 的性能等级
kubectl patch pvc dynamic-perf-pvc --type=merge -p \
  '{"spec":{"volumeAttributesClassName":"high-performance"}}'
```

---

<!-- chunk: 案例八：SELinux 挂载优化 -->
## 案例八：SELinux 挂载优化

### 场景

SELinuxMount (v1.30 Alpha) 优化了 SELinux 标签处理，减少 `chcon` 开销。

### 前置条件

```bash
# kubelet 启用 FG
# --feature-gates=SELinuxMount=true

# 确认 SELinux 已启用
getenforce
# Enforcing
```

### Pod 配置

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: selinux-demo
spec:
  securityContext:
    seLinuxOptions:
      level: "s0:c123,c456"
  containers:
    - name: app
      image: nginx:1.25
      securityContext:
        seLinuxOptions:
          type: "container_t"
          level: "s0:c123,c456"
      volumeMounts:
        - name: data
          mountPath: /data
  volumes:
    - name: data
      persistentVolumeClaim:
        claimName: data-pvc
```

### 验证

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

```bash
# 查看挂载选项
kubectl exec selinux-demo -- mount | grep /data

# 查看 SELinux 标签
kubectl exec selinux-demo -- ls -Z /data
```

---

<!-- chunk: 案例九：协调领导者选举 -->
## 案例九：协调领导者选举

### 场景

CoordinatedLeaderElection (v1.32 Alpha) 允许多个组件共享领导者选举配置，减少 [[etcd|etcd]] 压力。

### 前置条件

```bash
# kube-apiserver 启用 FG
# --feature-gates=CoordinatedLeaderElection=true
```

### 配置 LeaseCandidate

```yaml
apiVersion: coordination.k8s.io/v1alpha1
kind: LeaseCandidate
metadata:
  name: kube-controller-manager
  namespace: kube-system
spec:
  leaseName: kube-controller-manager
  preferredStrategies:
    - OldestEmulationVersion
  binaryVersion: "1.33.0"
  emulationVersion: "1.33.0"
```

### 效果

```
优化前:
├── kube-scheduler: 独立 Lease (1)
├── kube-controller-manager: 独立 Lease (1)
├── cloud-controller-manager: 独立 Lease (1)
└── 总计: 3 个 Lease 对象 × 副本数

优化后:
├── 统一 Lease 策略
├── 减少 etcd Watch 连接数
└── 降低 APIServer 负载
```

---

<!-- chunk: 案例十：节点 Swap 支持配置 -->
## 案例十：节点 Swap 支持配置

### 场景

NodeSwap (Alpha) 允许在 kubelet 上启用 Swap 支持，适用于内存密集型但可接受性能下降的工作负载。

### 前置条件

```bash
# 1. 创建 Swap 分区/文件
sudo fallocate -l 8G /swapfile
sudo chmod 600 /swapfile
sudo mkswap /swapfile
sudo swapon /swapfile

# 2. 确认 Swap 已启用
free -h
swapon -s
```

### kubelet 配置

```yaml
# /var/lib/kubelet/config.yaml
featureGates:
  NodeSwap: true
memorySwap:
  swapBehavior: LimitedSwap  # 或 UnlimitedSwap（不推荐生产）
```

### Pod 使用 Swap

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: swap-demo
spec:
  containers:
    - name: app
      image: python:3.12
      command: ["python", "-c", "import time; time.sleep(3600)"]
      resources:
        requests:
          memory: "256Mi"
        limits:
          memory: "512Mi"
      # Swap 限制不能超过 memory limit
```

### Swap 行为说明

```
swapBehavior 选项:
├── LimitedSwap: 允许使用 Swap，但限制使用量
│   └── 容器可使用 Swap = memory.limit - memory.request
├── UnlimitedSwap: 无限制使用 Swap（不推荐）
└── 不配置: 默认禁用 Swap
```

---

<!-- chunk: 案例十一：Pod 级 NUMA 拓扑策略 -->
## 案例十一：Pod 级 NUMA 拓扑策略

### 场景

TopologyManagerPolicyOptions (v1.33 GA，需显式启用) 允许在 Pod 级别配置 NUMA 拓扑策略。

### 前置条件

```bash
# kubelet 启用 FG
# --feature-gates=TopologyManagerPolicyOptions=true
```

### kubelet 配置

```yaml
# /var/lib/kubelet/config.yaml
topologyManagerPolicy: best-effort  # 或 restricted/single-numa-node
topologyManagerScope: pod           # pod 级策略
```

### Pod 配置

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: numa-demo
spec:
  containers:
    - name: cpu-intensive
      image: stress-ng:latest
      command: ["stress-ng", "--cpu", "4"]
      resources:
        requests:
          cpu: "4"
          memory: "8Gi"
        limits:
          cpu: "4"
          memory: "8Gi"
```

### 验证 NUMA 亲和性

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

```bash
# 查看 Pod 被分配到的 NUMA 节点
kubectl exec numa-demo -- numactl --show

# 在节点上查看
kubectl get pod numa-demo -o wide
# 在对应节点上执行
cat /sys/fs/cgroup/kubepods/pod-*/cpuset.cpus.effective
```

---

<!-- chunk: 案例十二：用户命名空间安全隔离 -->
## 案例十二：用户命名空间安全隔离

### 场景

UserNamespacesSupport (v1.33 GA) 为 Pod 提供用户命名空间隔离，将容器内的 root (UID 0) 映射到节点上的非特权 UID。

### 前置条件

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `sysctl -w`：实时修改内核参数，全局生效

```bash
# 1. 内核 >= 5.19（推荐 >= 6.3）
uname -r

# 2. 启用用户命名空间
sysctl user.max_user_namespaces
# 如果为 0，启用:
sysctl -w user.max_user_namespaces=28633

# 3. 确认 idmap 挂载支持
cat /proc/filesystems | grep overlay
# 需要内核支持 idmap
```

### Pod 配置

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: userns-demo
spec:
  hostUsers: false  # 🔑 启用用户命名空间
  containers:
    - name: app
      image: nginx:1.25
      securityContext:
        runAsUser: 0  # 容器内 root
        runAsGroup: 0
```

### 验证隔离

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

```bash
# 在容器内查看 UID（显示 root）
kubectl exec userns-demo -- id
# uid=0(root) gid=0(root)

# 在节点上查看实际进程 UID（非 root）
# 找到容器进程
crictl ps | grep userns-demo
# 查看进程 UID
ps -o pid,uid,gid,comm -p $(pgrep nginx)
# UID 会映射到 65536+ 范围
```

### 安全效果

```
用户命名空间隔离:
├── 容器 root (UID 0) → 节点 UID 65536+
├── 容器 UID 1 → 节点 UID 65537
├── 即使容器逃逸，攻击者也无法获得节点 root 权限
└── 建议与 AppArmor/Seccomp 结合使用
```

---

<!-- chunk: 案例十三：Kubectl 节点日志查询 -->
## 案例十三：Kubectl 节点日志查询

### 场景

NodeLogQuery (v1.30 Alpha) 允许通过 kubectl 直接查询节点上的系统日志。

### 前置条件

```bash
# kubelet 启用 FG
# --feature-gates=NodeLogQuery=true
```

### 查询日志

```bash
# 查询所有节点上的 kubelet 日志
kubectl node-logs --all-nodes --query="kubelet"

# 查询特定节点的系统日志
kubectl node-logs node-1 --query="systemd"

# 查询最近 1 小时的日志
kubectl node-logs node-1 --query="kubelet" --since=1h

# 查询特定服务的日志
kubectl node-logs node-1 --service=kubelet

# 查询内核日志
kubectl node-logs node-1 --query="kernel"
```

### 等效配置

```bash
# 也可以通过 API 直接查询
curl -k \
  "https://NODE_IP:10250/logs/?query=kubelet&sinceTime=$(date -u -d '1 hour ago' +%Y-%m-%dT%H:%M:%SZ)" \
  --cert /etc/kubernetes/pki/apiserver-kubelet-client.crt \
  --key /etc/kubernetes/pki/apiserver-kubelet-client.key
```

---

<!-- chunk: 案例十四：Queueing Hints 优化调度性能 -->
## 案例十四：Queueing Hints 优化调度性能

### 场景

SchedulerQueueingHints (v1.33 Beta，默认启用) 优化调度器队列，减少不必要的重试。

### 前置条件

```bash
# kube-scheduler 默认启用
# --feature-gates=SchedulerQueueingHints=true
```

### 验证启用状态

```bash
# 查看调度器日志中的 hints
kubectl logs -n kube-system -l component=kube-scheduler | grep -i "queueing hint"

# 查看调度器指标
curl http://localhost:10259/metrics | grep scheduler_queueing
```

### 效果说明

```
Queueing Hints 优化:
├── 传统方式: Pod 进入不可调度队列 → 所有事件触发重试
├── 优化后: Pod 注册特定 hint → 仅相关事件触发重试
├── 效果: 减少 50-70% 无效调度尝试
└── 适用场景: 大规模集群 (1000+ 节点, 10000+ Pod)
```

---

<!-- chunk: 附录：一键启用所有 v1.33 特性脚本 -->
## 附录：一键启用所有 v1.33 特性脚本

```bash
#!/bin/bash
# enable-v133-features.sh
# 一键启用 Kubernetes v1.33 所有 Alpha/Beta 特性

FEATURE_GATES=(
  "DynamicResourceAllocation=true"
  "InPlacePodVerticalScaling=true"
  "NFTablesProxyMode=true"
  "VolumeAttributesClass=true"
  "NodeSwap=true"
  "NodeLogQuery=true"
  "HonorPVReclaimPolicy=true"
  "PersistentVolumeDeleteProtection=true"
  "ClusterTrustBundle=true"
  "ServiceAccountTokenJTI=true"
  "ServiceAccountTokenNodeBindingValidation=true"
  "CrossNamespaceVolumeDataSource=true"
  "SELinuxMount=true"
  "SELinuxChangePolicy=true"
  "SupplementalGroupsPolicy=true"
  "PodLifecycleSleepAction=true"
  "CPUManagerPolicyAlphaOptions=true"
  "MemoryQoS=true"
  "JobSuccessPolicy=true"
  "CoordinatedLeaderElection=true"
  "OrderedNamespaceDeletion=true"
  "RetryGenerateName=true"
  "WatchListClient=true"
  "BtreeWatchCache=true"
  "PortForwardWebsockets=true"
  "AuthorizeWithSelectors=true"
  "AuthorizeNodeWithSelectors=true"
  "ConsistentListFromCache=true"
  "AnyVolumeDataSource=true"
  "DisableNodeKubeProxyVersion=true"
  "DisableNodeCSIPlugin=true"
)

# 拼接为逗号分隔的字符串
FG_STRING=$(IFS=,; echo "${FEATURE_GATES[*]}")

echo "Feature Gates 字符串:"
echo "$FG_STRING"
echo ""
echo "请将此字符串添加到各组件的 --feature-gates 参数中"
echo ""
echo "示例:"
echo "  kube-apiserver --feature-gates=$FG_STRING"
echo "  kube-scheduler --feature-gates=$FG_STRING"
echo "  kubelet (config.yaml featureGates 节)"
```

---

<!-- chunk: 参考链接 -->
## 参考链接

- [Kubernetes v1.33 发布说明](https://kubernetes.io/blog/2025/04/23/kubernetes-v1-33-release/)
- [Feature Gates 文档](https://kubernetes.io/docs/reference/command-line-tools-reference/feature-gates/)
- [DRA 用户指南](https://kubernetes.io/docs/concepts/scheduling-eviction/dynamic-resource-allocation/)
- [Sidecar 容器](https://kubernetes.io/docs/concepts/workloads/pods/sidecar-containers/)
- [ValidatingAdmissionPolicy](https://kubernetes.io/docs/reference/access-authn-authz/validating-admission-policy/)

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

- 99-kubernetes-v1.33-deprecation-migration-guide
- 99-kubernetes-v1.33-ecosystem-compatibility-matrix
- 99-kubernetes-v1.33-production-best-practices
- 99-kubernetes-v1.33-quick-reference-card
