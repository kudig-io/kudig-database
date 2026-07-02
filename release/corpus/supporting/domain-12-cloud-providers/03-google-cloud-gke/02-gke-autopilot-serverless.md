---
title: GKE Autopilot 模式 — 无节点管理的 Serverless Kubernetes
description: 'GKE Autopilot 自动节点配置、Serverless VPC Access、Pod 资源推断及与 Standard 模式选型对比'
summary: 'GKE Autopilot 自动节点配置、Serverless VPC Access、Pod 资源推断及与 Standard 模式选型对比'
category: cloud-providers
tags:
- cloud
- k8s
- gcp
- gke
- autopilot
- serverless
tier: supporting
created: '2026-07-02'
last_updated: 2026-07
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 平台工程师
estimated_read_time: 15min
intent_queries:
- GKE Autopilot 是什么
- 如何使用 GKE Autopilot
trigger_keywords:
- gke-autopilot
- serverless
- node-auto-provisioning
- standard-vs-autopilot
prerequisites:
- kubectl-basics
- cloud-basics
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
authors:
- name: KUDIG Team
  role: contributor
---

# GKE Autopilot 模式 — 无节点管理的 Serverless Kubernetes

## 1. Autopilot 架构概述

GKE Autopilot 是 Google 完全托管的 Kubernetes 模式，用户无需管理节点、节点池或集群自动扩缩。Google 负责节点配置、安全加固、升级和监控。

```
Standard 模式:
  用户管理: Node Pool → Node → Kubelet → Pod
  Google 管理: Control Plane

Autopilot 模式:
  用户管理: Pod Spec（资源请求/限制）
  Google 管理: Control Plane + Node + Kubelet + 自动扩缩
```

### 1.1 核心特性

- **按 Pod 计费** — 仅对请求的资源付费，非节点级别
- **自动节点管理** — Google 选择实例类型、管理升级
- **安全加固** — 默认启用 Shielded GKE Node、Workload Identity
- **SLA 保障** — 99.95% 控制平面可用性
- **Pod 级别 QoS** — 支持 Guaranteed、Burstable、BestEffort

## 2. 创建 Autopilot 集群

### 2.1 gcloud 命令行

```bash
# 创建 Autopilot 集群
gcloud container clusters create-auto prod-autopilot \
  --region=asia-southeast1 \
  --release-channel=regular \
  --cluster-version=1.31 \
  --network=projects/my-project/global/networks/default \
  --subnetwork=projects/my-project/regions/asia-southeast1/subnetworks/default \
  --cluster-secondary-range-name=pods \
  --services-secondary-range-name=services \
  --enable-private-nodes \
  --master-ipv4-cidr=172.16.0.0/28 \
  --enable-master-authorized-networks \
  --master-authorized-networks=10.0.0.0/8 \
  --logging=SYSTEM,WORKLOAD \
  --monitoring=SYSTEM \
  --labels=env=prod,team=platform

# 查看集群状态
gcloud container clusters describe prod-autopilot \
  --region=asia-southeast1 \
  --format="table(status, currentMasterVersion, currentNodeVersion)"
```

### 2.2 Terraform 配置

```hcl
resource "google_container_cluster" "autopilot" {
  name     = "prod-autopilot"
  location = "asia-southeast1"

  enable_autopilot = true

  release_channel {
    channel = "REGULAR"
  }

  network    = google_compute_network.vpc.id
  subnetwork = google_compute_subnetwork.subnet.id

  ip_allocation_policy {
    cluster_secondary_range_name  = "pods"
    services_secondary_range_name = "services"
  }

  private_cluster_config {
    enable_private_nodes    = true
    enable_private_endpoint = false
    master_ipv4_cidr_block  = "172.16.0.0/28"
  }

  master_authorized_networks_config {
    cidr_blocks {
      cidr_block = "10.0.0.0/8"
      display_name = "internal"
    }
  }

  logging_config {
    enable_components = ["SYSTEM", "WORKLOADS"]
  }

  monitoring_config {
    enable_components = ["SYSTEM"]
    managed_prometheus {
      enabled = true
    }
  }
}
```

## 3. Autopilot 资源管理

### 3.1 Pod 资源请求

```yaml
# Autopilot 要求每个容器必须指定资源请求
apiVersion: apps/v1
kind: Deployment
metadata:
  name: web-app
  namespace: production
spec:
  replicas: 3
  selector:
    matchLabels:
      app: web-app
  template:
    metadata:
      labels:
        app: web-app
    spec:
      containers:
        - name: app
          image: gcr.io/my-project/web-app:v1.2.3
          resources:
            requests:
              cpu: "500m"
              memory: "512Mi"
              ephemeral-storage: "1Gi"
            limits:
              cpu: "1000m"
              memory: "1Gi"
          ports:
            - containerPort: 8080
```

### 3.2 资源规格限制

| 资源类型 | 最小请求 | 最大请求 | 说明 |
|---------|---------|---------|------|
| CPU | 250m | 32 vCPU | 每容器 |
| Memory | 512Mi | 128 GiB | 每容器 |
| Ephemeral Storage | 1Gi | 179 GiB | 每容器 |
| Pod 总 CPU | - | 64 vCPU | 每 Pod |
| Pod 总 Memory | - | 256 GiB | 每 Pod |
| GPU | 1 | 8 | 支持 T4、A100、L4 |

### 3.3 资源推断配置

```yaml
# Vertical Pod Autoscaler（Autopilot 内置推荐）
apiVersion: autoscaling.k8s.io/v1
kind: VerticalPodAutoscaler
metadata:
  name: web-app-vpa
  namespace: production
spec:
  targetRef:
    apiVersion: apps/v1
    kind: Deployment
    name: web-app
  updatePolicy:
    updateMode: "Auto"  # 自动调整资源
  resourcePolicy:
    containerPolicies:
      - containerName: app
        minAllowed:
          cpu: "250m"
          memory: "256Mi"
        maxAllowed:
          cpu: "4"
          memory: "8Gi"
        controlledResources: ["cpu", "memory"]
```

## 4. Autopilot 网络配置

### 4.1 VPC 原生集群

Autopilot 强制使用 VPC 原生模式，Pod 和 Service 使用独立的 IP 范围。

```bash
# 查看 IP 分配
gcloud container clusters describe prod-autopilot \
  --region=asia-southeast1 \
  --format="json(ipAllocationPolicy)"
```

### 4.2 Serverless VPC Access

```bash
# 创建 Serverless VPC Access Connector
gcloud compute networks vpc-access connectors create prod-connector \
  --region=asia-southeast1 \
  --subnet=connector-subnet \
  --subnet-project=my-project \
  --min-instances=2 \
  --max-instances=10 \
  --machine-type=e2-micro
```

```yaml
# Cloud Run 连接 GKE 集群内服务
apiVersion: v1
kind: Service
metadata:
  name: internal-api
  namespace: production
spec:
  type: ClusterIP
  selector:
    app: internal-api
  ports:
    - port: 8080
      targetPort: 8080
```

### 4.3 Network Policy

```yaml
# Autopilot 支持 Google 实现的 Network Policy
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: api-server-policy
  namespace: production
spec:
  podSelector:
    matchLabels:
      app: api-server
  policyTypes:
    - Ingress
    - Egress
  ingress:
    - from:
        - namespaceSelector:
            matchLabels:
              role: frontend
        - podSelector:
            matchLabels:
              app: gateway
      ports:
        - protocol: TCP
          port: 8080
  egress:
    - to:
        - namespaceSelector:
            matchLabels:
              role: database
      ports:
        - protocol: TCP
          port: 5432
    - to:  # 允许 DNS
        - namespaceSelector: {}
      ports:
        - protocol: UDP
          port: 53
```

## 5. Autopilot 存储

### 5.1 支持的存储类型

| 存储类型 | ReadWriteOnce | ReadOnlyMany | ReadWriteMany |
|---------|:---:|:---:|:---:|
| Persistent Disk | ✅ | ✅ | ❌ |
| Filestore | ✅ | ✅ | ✅ |
| Cloud Storage FUSE | ✅ | ✅ | ✅ |

### 5.2 StorageClass 配置

```yaml
# 标准 PD（默认）
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: standard-rwo
provisioner: pd.csi.storage.gke.io
volumeBindingMode: WaitForFirstConsumer
allowVolumeExpansion: true
parameters:
  type: pd-standard
  replication-type: regional-pd

---
# SSD PD
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: ssd-rwo
provisioner: pd.csi.storage.gke.io
volumeBindingMode: WaitForFirstConsumer
allowVolumeExpansion: true
parameters:
  type: pd-ssd
  replication-type: regional-pd

---
# PVC 示例
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: app-data
  namespace: production
spec:
  accessModes:
    - ReadWriteOnce
  storageClassName: ssd-rwo
  resources:
    requests:
      storage: 100Gi
```

## 6. Autopilot 安全特性

### 6.1 默认安全加固

```bash
# Autopilot 自动启用:
# - Workload Identity
# - Shielded GKE Node (Secure Boot, vTPM, Integrity Monitoring)
# - 漏洞扫描 (GKE Enterprise)
# - 节点只读文件系统
# - 禁止特权容器
# - 禁止 hostNetwork/hostPID/hostIPC

# 查看安全配置
gcloud container clusters describe prod-autopilot \
  --region=asia-southeast1 \
  --format="json(shieldedNodes, workloadIdentityConfig)"
```

### 6.2 Workload Identity 配置

```bash
# 创建 GCP Service Account
gcloud iam service-accounts create gke-app-sa \
  --display-name="GKE App Service Account"

# 绑定 IAM 角色
gcloud projects add-iam-policy-binding my-project \
  --member="serviceAccount:gke-app-sa@my-project.iam.gserviceaccount.com" \
  --role="roles/storage.objectViewer"

# 创建 KSA 到 GSA 绑定
gcloud iam service-accounts add-iam-policy-binding \
  gke-app-sa@my-project.iam.gserviceaccount.com \
  --role="roles/iam.workloadIdentityUser" \
  --member="serviceAccount:my-project.svc.id.goog[production/app-sa]"
```

```yaml
# Kubernetes Service Account
apiVersion: v1
kind: ServiceAccount
metadata:
  name: app-sa
  namespace: production
  annotations:
    iam.gke.io/gcp-service-account: gke-app-sa@my-project.iam.gserviceaccount.com
```

## 7. Autopilot vs Standard 选型

| 维度 | Autopilot | Standard |
|------|----------|----------|
| 节点管理 | 完全自动 | 用户自管 |
| 计费模式 | 按 Pod 资源请求 | 按节点实例 |
| 最低成本 | 零（无 Pod 时无费用） | 最少 1 个节点 |
| 高密度场景 | 成本可能更高 | 成本更可控 |
| DaemonSet | 不支持 | 支持 |
| 特权容器 | 不支持 | 可配置 |
| GPU 节点 | 支持 | 支持 |
| 裸金属节点 | 不支持 | 支持 |
| 控制平面 | Google 全托管 | Google 全托管 |
| 适用场景 | 微服务、CI/CD、批处理 | 需要自定义节点、特殊硬件 |

### 7.1 选型决策树

```
是否需要 DaemonSet？
├── 是 → Standard
└── 否 → 是否需要特权容器？
    ├── 是 → Standard
    └── 否 → 是否需要自定义节点配置？
        ├── 是 → Standard
        └── 否 → 是否对成本敏感且 Pod 数量波动大？
            ├── 是 → Autopilot
            └── 否 → 是否有 GPU/HPC 需求？
                ├── 是 → 两者均可，评估成本
                └── 否 → Autopilot（推荐）
```

## 8. Autopilot 监控与日志

### 8.1 Cloud Monitoring 集成

```yaml
# Managed Prometheus（推荐）
# Autopilot 默认启用
# 查询示例:
# container_cpu_usage_seconds_total
# container_memory_working_set_bytes

# 自定义 Dashboard
apiVersion: monitoring.googleapis.com/v1
kind: Dashboard
metadata:
  name: autopilot-overview
spec:
  displayName: "Autopilot Cluster Overview"
  gridLayout:
    columns: 2
    widgets:
      - title: "Pod CPU Usage"
        xyChart:
          dataSets:
            - timeSeriesQuery:
                timeSeriesFilter:
                  filter: 'metric.type="kubernetes.io/container/cpu/core_usage_time"'
      - title: "Pod Memory Usage"
        xyChart:
          dataSets:
            - timeSeriesQuery:
                timeSeriesFilter:
                  filter: 'metric.type="kubernetes.io/container/memory/used_bytes"'
```

### 8.2 日志查询

```bash
# 通过 gcloud 查看日志
gcloud logging read \
  'resource.type="k8s_container" AND resource.labels.cluster_name="prod-autopilot"' \
  --limit=50 \
  --format="table(timestamp, resource.labels.namespace_name, resource.labels.pod_name, textPayload)"

# 查找 OOMKilled 事件
gcloud logging read \
  'resource.type="k8s_container" AND textPayload=~"OOMKilled"' \
  --limit=20 \
  --format=json
```

## Related

- [[03-gke-networking-dataplane-v2]]
- [[05-gke-workload-identity-security]]

## See Also

- GKE Autopilot 文档
- Autopilot 定价
