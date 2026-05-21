---
title: containerd 多租户与共享集群配置
description: 'title: containerd 多租户与共享集群配置'
category: general
tags:
- cncf
- ecosystem
- kubelet
- prometheus
- coredns
- containerd
- docker
- hpa
- daemonset
- job
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 15min
intent_queries:
- containerd 多租户与共享集群配置 是什么
- 如何 containerd 多租户与共享集群配置
- Kubernetes 19 landscape references 最佳实践
trigger_keywords:
- containerd
- 多租户与共享集群配置
- landscape
- references
prerequisites:
- kubectl-basics
- cncf-ecosystem
- prometheus-basics
- gpu-scheduling-basics
---

title: containerd 多租户与共享集群配置
description: '## 1. 多租户概述'
category: cncf-landscape
tags:
- k8s
- containerd
- multi-tenant
- namespace
- resource-quota
- security
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- 平台工程师
- 运维工程师
- 安全工程师
estimated_read_time: 10min
intent_queries:
- containerd 多租户 配置
- containerd 命名空间 隔离
- containerd 资源配额 配置
trigger_keywords:
- containerd 多租户
- containerd 命名空间
- containerd 隔离
authors:
- name: KUDIG Team
  role: contributor
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
---
# containerd 多租户与共享集群配置

> **版本**: v1.0 | **适用版本**: containerd 1.6+ / 2.0 | **最后更新**: 2026-05

---

## 1. 多租户概述

### 1.1 多租户挑战

在共享 Kubernetes 集群中，多个租户（团队、项目、部门）共享同一组节点资源。containerd 作为运行时要支持：

| 挑战 | 说明 | containerd 支持 |
|------|------|----------------|
| **资源隔离** | 防止某个租户耗尽所有资源 | Cgroups, Resource Limits |
| **命名空间隔离** | 租户间容器和网络隔离 | CNI, NetworkNamespace |
| **安全隔离** | 防止权限提升 | Capabilities, Seccomp, SELinux |
| **配额管理** | 限制租户资源使用 | Kubernetes ResourceQuota |
| **存储隔离** | 租户数据不互相访问 | Volume, Snapshot 隔离 |

### 1.2 多租户架构

```
┌─────────────────────────────────────────────────────────────────────────────────┐
│                         多租户 containerd 架构                                   │
├─────────────────────────────────────────────────────────────────────────────────┤
│                                                                                  │
│  ┌─────────────────────────────────────────────────────────────────────────┐    │
│  │                    Shared Node (containerd)                               │    │
│  │                                                                          │    │
│  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐  ┌─────────────────┐  │    │
│  │  │  Tenant A   │  │  Tenant B   │  │  Tenant C   │  │   System        │  │    │
│  │  │  Namespace  │  │  Namespace  │  │  Namespace  │  │   Pods          │  │    │
│  │  │  (k8s.io)   │  │  (k8s.io)   │  │  (k8s.io)   │  │   (kube-system) │  │    │
│  │  │             │  │             │  │             │  │                 │  │    │
│  │  │ Pod-1       │  │ Pod-3       │  │ Pod-5       │  │   coredns       │  │    │
│  │  │ Pod-2       │  │ Pod-4       │  │ Pod-6       │  │   metrics-server│  │    │
│  │  │             │  │             │  │             │  │                 │  │    │
│  │  └─────────────┘  └─────────────┘  └─────────────┘  └─────────────────┘  │    │
│  │                                                                          │    │
│  │  ┌───────────────────────────────────────────────────────────────────┐   │    │
│  │  │                    containerd daemon                              │   │    │
│  │  │  ┌───────────┐ ┌───────────┐ ┌───────────┐ ┌─────────────────────┐│   │    │
│  │  │  │  Content  │ │ Snapshot  │ │ Container │ │       Task          ││   │    │
│  │  │  │  Store    │ │  Service  │ │  Service  │ │      Service        ││   │    │
│  │  │  └───────────┘ └───────────┘ └───────────┘ └─────────────────────┘│   │    │
│  │  └───────────────────────────────────────────────────────────────────┘   │    │
│  │                                                                          │    │
│  └─────────────────────────────────────────────────────────────────────────┘    │
│                                                                                  │
└─────────────────────────────────────────────────────────────────────────────────┘
```

---

## 2. Kubernetes 命名空间隔离

### 2.1 containerd 命名空间

containerd 通过命名空间隔离不同租户的资源和元数据：

```bash
# 列出 containerd 命名空间
ctr namespaces ls

# 输出示例
NAME                LABELS
k8s.io              []
default             []
tenant-a            [{"tenant":"a"}]
tenant-b            [{"tenant":"b"}]

# 在特定命名空间操作
ctr -n tenant-a images ls
ctr -n tenant-a containers ls
```

### 2.2 Kubernetes 命名空间映射

```yaml
# Kubernetes Namespace 与 containerd 命名空间对应
apiVersion: v1
kind: Namespace
metadata:
  name: tenant-a
  labels:
    tenant: a
---
# Pod 自动使用 default containerd 命名空间 (k8s.io)
apiVersion: v1
kind: Pod
metadata:
  name: app-a
  namespace: tenant-a
spec:
  containers:
  - name: app
    image: nginx:latest
```

### 2.3 跨命名空间隔离配置

```toml
# /etc/containerd/config.toml
version = 2

[plugins]
  # 启用命名空间追踪
  [plugins."io.containerd.grpc.v1.cri"]
    # 容器只在所属 Kubernetes namespace 内可见
    enforce_governing_sequence_number = ""
    
    # Network namespace 隔离
    [plugins."io.containerd.grpc.v1.cri".cni]
      bin_dir = "/opt/cni/bin"
      conf_dir = "/etc/cni/net.d"
      max_conf_num = 1

# 命名空间隔离通过 Kubernetes RBAC 实现
# containerd 本身不直接做租户隔离，而是通过 kubelet 层面控制
```

---

## 3. 资源配额管理

### 3.1 Kubernetes ResourceQuota

```yaml
# 为租户配置 ResourceQuota
apiVersion: v1
kind: ResourceQuota
metadata:
  name: tenant-a-quota
  namespace: tenant-a
spec:
  hard:
    # Pod 数量限制
    pods: "50"
    # 计算资源
    requests.cpu: "20"
    requests.memory: "40Gi"
    limits.cpu: "40"
    limits.memory: "80Gi"
    # 存储资源
    requests.storage: "100Gi"
    # 镜像数量
    persistentvolumeclaims: "10"
    # 拉取镜像速率
    .storageclass.storage.k8s.io/requests.storage: "50Gi"
```

### 3.2 LimitRange

```yaml
# 设置默认资源限制和限制范围
apiVersion: v1
kind: LimitRange
metadata:
  name: tenant-a-limits
  namespace: tenant-a
spec:
  limits:
  - type: Container
    max:
      cpu: "4"
      memory: "8Gi"
    min:
      cpu: "100m"
      memory: "128Mi"
    default:
      cpu: "500m"
      memory: "512Mi"
    defaultRequest:
      cpu: "200m"
      memory: "256Mi"
    maxLimitRequestRatio:
      cpu: "10"
      memory: "10"
```

### 3.3 cgroup v2 配额

```toml
# /etc/containerd/config.toml (多租户资源配置)
[plugins."io.containerd.grpc.v1.cri"]
  # Pod 级别的资源限制通过 Kubernetes 实现
  # containerd 负责将限制传递给 runc
  
  [plugins."io.containerd.grpc.v1.cri".containerd.runtimes.runc]
    runtime_type = "io.containerd.runc.v2"
    [plugins."io.containerd.grpc.v1.cri".containerd.runtimes.runc.options]
      SystemdCgroup = true
      # Cgroup v2 路径由 kubelet 管理
```

### 3.4 运行时资源监控

```bash
# 查看租户资源使用
kubectl describe resourcequota tenant-a-quota -n tenant-a

# 查看 LimitRange
kubectl describe limitrange tenant-a-limits -n tenant-a

# 查看节点资源分配
kubectl describe node | grep -A 20 "Allocated resources"
```

---

## 4. 安全隔离

### 4.1 Pod Security Standards

```yaml
# 强制租户使用 restricted PSP
apiVersion: v1
kind: Namespace
metadata:
  name: tenant-a
  labels:
    # 强制 Pod Security Standards
    pod-security.kubernetes.io/enforce: restricted
    pod-security.kubernetes.io/audit: restricted
    pod-security.kubernetes.io/warn: restricted
---
# 或者使用 Pod Security Admission
apiVersion: v1
kind: Namespace
metadata:
  name: privileged-tenant
  labels:
    pod-security.kubernetes.io/enforce: baseline
```

### 4.2 NetworkPolicy 隔离

```yaml
# 租户网络隔离
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: tenant-a-network-policy
  namespace: tenant-a
spec:
  podSelector: {}  # 选择所有 Pod
  policyTypes:
  - Ingress
  - Egress
  
  ingress:
  - from:
    - namespaceSelector:
        matchLabels:
          tenant: a
    - podSelector: {}
  
  egress:
  - to:
    - podSelector: {}
    - namespaceSelector: {}
```

### 4.3 服务账户权限控制

```yaml
# 限制租户的服务账户权限
apiVersion: rbac.authorization.k8s.io/v1
kind: Role
metadata:
  name: tenant-a-role
  namespace: tenant-a
rules:
- apiGroups: [""]
  resources: ["pods", "services", "configmaps"]
  verbs: ["get", "list", "watch"]
- apiGroups: ["apps"]
  resources: ["deployments", "replicasets"]
  verbs: ["get", "list", "watch"]
---
apiVersion: rbac.authorization.k8s.io/v1
kind: RoleBinding
metadata:
  name: tenant-a-binding
  namespace: tenant-a
subjects:
- kind: ServiceAccount
  name: default
  namespace: tenant-a
roleRef:
  kind: Role
  name: tenant-a-role
```

---

## 5. 存储隔离

### 5.1 快照存储隔离

```bash
# containerd 默认按命名空间隔离快照
# 查看租户命名空间的快照
ctr -n tenant-a snapshots ls

# 查看系统命名空间的快照
ctr -n k8s.io snapshots ls

# 快照存储路径
# /var/lib/containerd/io.containerd.snapshotter.v1.overlayfs/snapshots/
# 每个命名空间有独立的快照目录
```

### 5.2 PersistentVolume 隔离

```yaml
# 租户独占 PV
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: tenant-a-data
  namespace: tenant-a
spec:
  accessModes:
  - ReadWriteOnce
  storageClassName: tenant-a-storage
  resources:
    requests:
      storage: 10Gi
---
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: tenant-a-storage
provisioner: kubernetes.io/gce-pd
parameters:
  type: pd-ssd
  replication-type: regional
volumeBindingMode: WaitForFirstConsumer
allowed topologies:
- matchLabelExpressions:
  - key: topology.kubernetes.io/zone
    values:
    - us-east1-a
    - us-east1-b
```

### 5.3 镜像存储隔离

```bash
# containerd 内容存储是共享的，但元数据按命名空间隔离
# 租户只能看到自己命名空间内的镜像

# 列出特定租户可见的镜像
ctr -n tenant-a images ls

# 跨租户镜像共享需要通过 ImagePullSecrets
kubectl create secret docker-registry tenant-a-registry \
  --docker-server=my-registry.example.com \
  --docker-username=tenant-a \
  --docker-password=xxx \
  -n tenant-a
```

---

## 6. 配额强制执行

### 6.1 Kubernetes 准入控制

```yaml
# LimitRanger 准入控制器自动设置资源限制
apiVersion: v1
kind: LimitRange
metadata:
  name: default-limits
  namespace: tenant-a
spec:
  limits:
  - type: Container
    default:
      memory: 512Mi
      cpu: 500m
    defaultRequest:
      memory: 256Mi
      cpu: 200m
---
# Pod 创建时自动应用默认限制
apiVersion: v1
kind: Pod
metadata:
  name: app
  namespace: tenant-a
spec:
  containers:
  - name: app
    image: nginx:latest
    # 自动应用 LimitRange 的默认值
```

### 6.2 资源配额强制

```bash
# 检查配额使用情况
kubectl describe resourcequota -n tenant-a

# 查看具体资源分配
kubectl top pods -n tenant-a

# 配额耗尽时创建 Pod 会被拒绝
# kubectl create 会返回:
# Error from server (Forbidden): pods "app" is forbidden:
# exceeded quota: tenant-a-quota, requested: pods=1, used: 50, limited: 50
```

### 6.3 运行时配额监控

```bash
# 监控 containerd 资源配额
curl -s http://127.0.0.1:1338/v1/metrics | grep containerd

# 查看容器数量
containerd_container_count

# 查看镜像数量
containerd_image_count

# 按命名空间统计
ctr -n k8s.io containers ls | wc -l
```

---

## 7. 共享集群最佳实践

### 7.1 节点池隔离

```yaml
# 为不同租户使用不同的节点池
apiVersion: v1
kind: NodePool
metadata:
  name: tenant-a-pool
spec:
  taints:
  - key: tenant
    value: a
    effect: NoSchedule
  nodeSelector:
    tenant: a
---
# 系统 Pod 使用 tolerations
apiVersion: v1
kind: DaemonSet
metadata:
  name: log-collector
  namespace: kube-system
spec:
  template:
    spec:
      tolerations:
      - key: "tenant"
        operator: "Exists"
        effect: "NoSchedule"
      nodeSelector:
        kubernetes.io/os: linux
```

### 7.2 公平调度

```yaml
# Pod 优先级配置
apiVersion: v1
kind: Pod
metadata:
  name: critical-app
  namespace: tenant-a
spec:
  priorityClassName: high-priority
  containers:
  - name: app
    image: nginx:latest
---
# PriorityClass 定义
apiVersion: scheduling.k8s.io/v1
kind: PriorityClass
metadata:
  name: high-priority
value: 100000
globalDefault: false
description: "High priority tenant-a workloads"
```

### 7.3 资源公平共享

```yaml
# 使用 ResourceClaims 进行资源管理
apiVersion: resource.k8s.io/v1alpha3
kind: ResourceClaim
metadata:
  name: gpu-resources
  namespace: tenant-a
spec:
  requests:
  - name: gpu
    resource:
      device: nvidia.com/gpu
      count: 1
```

---

## 8. 监控与告警

### 8.1 租户资源监控

```yaml
# Prometheus 配置按租户抓取指标
- job_name: 'kubernetes-nodes'
  kubernetes_sd_configs:
  - role: node
  relabel_configs:
  - source_labels: [__address__]
    regex: '(.*):10250'
    replacement: '${1}:1338'
    target_label: __metrics_path__
  
  # 按节点分组
  - action: labelmap
    regex: __meta_kubernetes_node_label_(.+)
```

### 8.2 租户配额告警

```yaml
# 当租户接近配额时告警
groups:
- name: tenant-quota-alerts
  rules:
  - alert: TenantQuotaNearlyExhausted
    expr: |
      kube_resourcequota{type="used"} / kube_resourcequota{type="hard"} > 0.8
    for: 10m
    labels:
      severity: warning
    annotations:
      summary: "Tenant {{ $labels.namespace }} quota nearly exhausted"
      description: "Used {{ $value | humanizePercentage }} of quota"
      
  - alert: TenantQuotaExceeded
    expr: |
      kube_resourcequota{type="used"} >= kube_resourcequota{type="hard"}
    for: 1m
    labels:
      severity: critical
    annotations:
      summary: "Tenant {{ $labels.namespace }} quota exceeded"
```

---

## 9. 故障排查

### 9.1 配额相关问题

```bash
# 问题: Pod 创建失败 "exceeded quota"
kubectl describe namespace tenant-a | grep -A 10 "ResourceQuota"

# 解决方案: 增加配额或清理资源
kubectl patch resourcequota tenant-a-quota -n tenant-a \
  --type=merge -p '{"spec":{"hard":{"pods":"100"}}}'

# 问题: 资源限制不足
kubectl describe limitrange -n tenant-a

# 解决方案: 调整 LimitRange
kubectl patch limitrange tenant-a-limits -n tenant-a \
  --type=merge -p '{"spec":{"limits":[{"type":"Container","max":{"memory":"16Gi"}}]}}'
```

### 9.2 命名空间隔离问题

```bash
# 问题: 跨租户访问
# 检查 NetworkPolicy
kubectl get networkpolicy -A

# 问题: 镜像拉取失败
# 检查 ImagePullSecrets
kubectl get secrets -n tenant-a | grep docker-registry

# 解决方案: 创建 registry secret
kubectl create secret docker-registry tenant-a-registry \
  --docker-server=my-registry.example.com \
  --docker-username=tenant-a \
  --docker-password=xxx \
  -n tenant-a

# 问题: 存储卷挂载失败
# 检查 PVC 状态
kubectl get pvc -n tenant-a

# 查看 Events
kubectl describe pvc tenant-a-data -n tenant-a
```

---

## 10. 成本优化

### 10.1 资源超卖

```yaml
# 在低优先级租户中启用资源超卖
apiVersion: v1
kind: ResourceQuota
metadata:
  name: tenant-burst-quota
  namespace: tenant-b
spec:
  hard:
    pods: "100"  # 超过实际节点容量
    requests.cpu: "40"
    limits.cpu: "80"
```

### 10.2 弹性资源池

```yaml
# 配置弹性资源池
apiVersion: autoscaling/v2
kind: HorizontalPodAutoscaler
metadata:
  name: tenant-a-hpa
  namespace: tenant-a
spec:
  scaleTargetRef:
    apiVersion: apps/v1
    kind: Deployment
    name: app
  minReplicas: 2
  maxReplicas: 20
  metrics:
  - type: Resource
    resource:
      name: cpu
      target:
        type: Utilization
        averageUtilization: 70
```

---

**维护者**: Kudig Team | **许可证**: MIT

## Related

- [[hot.md|hot]]
- [[domain-17-system-foundation/topic-cheat-sheet/go.md|go]]
- [[domain-17-system-foundation/topic-cheat-sheet/linux.md|linux]]
- [[domain-17-system-foundation/topic-cheat-sheet/k8s.md|k8s]]
- [[entities/kubernetes.md|kubernetes]]

## See Also

- [[domain-19-landscape-references/graduated/containerd/06-containerd-observability.md|06-containerd-observability]]
- [[domain-19-landscape-references/graduated/containerd/07-containerd-disaster-recovery.md|07-containerd-disaster-recovery]]
- [[domain-19-landscape-references/graduated/containerd/containerd.md|containerd]]
- [[domain-19-landscape-references/graduated/containerd/02-containerd-v2-features.md|02-containerd-v2-features]]
