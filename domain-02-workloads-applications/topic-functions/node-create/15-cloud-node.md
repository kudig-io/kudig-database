---
title: 云厂商节点集成 — AWS / GCP / Azure
description: 'description: ''## 概述'''
summary: 'description: ''## 概述'''
category: general
tags:
- reference
- kubelet
- controller-manager
- operator
- gpu
- agent
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 15min
intent_queries:
- 云厂商节点集成 — AWS / GCP / Azure 是什么
- 如何 云厂商节点集成 — AWS / GCP / Azure
- Kubernetes 07 platform engineering 最佳实践
trigger_keywords:
- 云厂商节点集成
- AWS
- GCP
- Azure
- platform
- engineering
- code
- analysis
prerequisites:
- kubectl-basics
- platform-engineering-basics
- gpu-scheduling-basics
---



title: 云厂商节点集成
description: '## 概述'
category: functions
tags:
- k8s
- operations
- cluster-management
- kubelet
- controller-manager
- operator
- gpu
- agent
last_updated: 2026-05-18
difficulty: intermediate
reading_level: intermediate
audience:
- Kubernetes 运维工程师
- 云工程师
estimated_read_time: 5min
intent_queries:
- Kubernetes cloud provider node integration
- AWS EKS GCP GKE Azure AKS node providerID
- cloud node controller providerID
- cloud metadata service IMDS
- node labels cloud provider
trigger_keywords:
- cloud provider
- AWS
- GCP
- Azure
- EKS
- GKE
- AKS
- providerID
- metadata
- IMDS
- node labels
- topology
- region
- zone
- instance-type
- cloud-node-manager
- IRSA
- Workload Identity
- Managed Identity
related_domains:
- domain-01-cluster-fundamentals
- domain-9-orchestration
related_topics:
- node-create/02-registration
- cluster-create/01-overview
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

# 云厂商节点集成 — AWS / GCP / Azure

## 概述

在云环境中运行 Kubernetes 时，节点与云厂商的集成是不可避免的。Kubernetes 通过 Cloud Provider 机制与各大云厂商（AWS、GCP、Azure）进行交互，实现节点自动注册、标签注入、负载均衡集成、存储卷管理等功能。

理解云厂商节点集成的原理对于以下场景至关重要：

- **节点身份标识**：每个云节点都有唯一的 `providerID`，用于关联 Kubernetes Node 对象和云厂商实例
- **自动标签注入**：kubelet 自动从云元数据中获取实例类型、区域、可用区等信息，并以标签形式写入 Node 对象
- **Cluster Autoscaler 集成**：自动伸缩依赖云厂商 API 来创建和删除实例
- **云厂商特有功能**：如 AWS IAM Role、GCP Workload Identity、Azure Managed Identity

本文档详细分析三大云厂商的节点集成机制，包括元数据服务、providerID、标签注入、污点管理等内容。

---

## 源码路径

| 组件 | 源码路径 | 说明 |
|------|---------|------|
| Cloud Provider 接口 | `pkg/cloudprovider/` | 云厂商接口定义 |
| cloud-node-manager | `pkg/controller/cloud/node/` | 云节点控制器 |
| kubelet 云资源 | `pkg/kubelet/cloudresource/` | 云资源管理 |
| AWS Cloud Provider | `staging/src/k8s.io/cloud-provider-aws/` | AWS 实现 |
| GCP Cloud Provider | `staging/src/k8s.io/cloud-provider-gcp/` | GCP 实现 |
| Azure Cloud Provider | `staging/src/k8s.io/legacy-cloud-providers/azure/` | Azure 实现 |

---

## 一、云厂商节点元数据

### 1.1 元数据服务概述

每个云厂商都提供实例元数据服务（Instance Metadata Service, IMDS），运行在实例上的程序可以通过 HTTP 请求获取实例的元数据信息。kubelet 和 cloud-node-manager 使用这些元数据来自动配置节点。

### 1.2 AWS EC2 元数据

```bash
# IMDSv1 (不推荐，存在 SSRF 风险)
curl http://169.254.169.254/latest/meta-data/

# IMDSv2 (推荐)
TOKEN=$(curl -X PUT "http://169.254.169.254/latest/api/token" -H "X-aws-ec2-metadata-token-ttl-seconds: 21600")
curl -H "X-aws-ec2-metadata-token: $TOKEN" http://169.254.169.254/latest/meta-data/

# 常用元数据:
curl -H "X-aws-ec2-metadata-token: $TOKEN" http://169.254.169.254/latest/meta-data/ami-id
# ami-0abcdef1234567890

curl -H "X-aws-ec2-metadata-token: $TOKEN" http://169.254.169.254/latest/meta-data/instance-id
# i-0abc123def456

curl -H "X-aws-ec2-metadata-token: $TOKEN" http://169.254.169.254/latest/meta-data/instance-type
# t3.medium

curl -H "X-aws-ec2-metadata-token: $TOKEN" http://169.254.169.254/latest/meta-data/placement/availability-zone
# us-east-1a

curl -H "X-aws-ec2-metadata-token: $TOKEN" http://169.254.169.254/latest/meta-data/local-ipv4
# 10.0.1.100
```

### 1.3 GCP GCE 元数据

```bash
# GCE 元数据需要指定 Metadata-Flavor header
curl -H "Metadata-Flavor: Google" http://metadata.google.internal/computeMetadata/v1/

# 常用元数据:
curl -H "Metadata-Flavor: Google" http://metadata.google.internal/computeMetadata/v1/instance/name
# my-instance

curl -H "Metadata-Flavor: Google" http://metadata.google.internal/computeMetadata/v1/instance/machine-type
# projects/123456/machineTypes/n1-standard-4

curl -H "Metadata-Flavor: Google" http://metadata.google.internal/computeMetadata/v1/instance/zone
# projects/123456/zones/us-central1-a

curl -H "Metadata-Flavor: Google" http://metadata.google.internal/computeMetadata/v1/instance/network-interfaces/0/ip
# 10.128.0.2

# 项目级元数据
curl -H "Metadata-Flavor: Google" http://metadata.google.internal/computeMetadata/v1/project/project-id
# my-gcp-project
```

### 1.4 Azure 元数据

```bash
# Azure Instance Metadata Service (IMDS)
curl -H "Metadata: true" "http://169.254.169.254/metadata/instance?api-version=2021-02-01"

# 常用元数据:
curl -H "Metadata: true" "http://169.254.169.254/metadata/instance/compute/name?api-version=2021-02-01&format=text"
# my-vm

curl -H "Metadata: true" "http://169.254.169.254/metadata/instance/compute/vmSize?api-version=2021-02-01&format=text"
# Standard_D4s_v3

curl -H "Metadata: true" "http://169.254.169.254/metadata/instance/compute/location?api-version=2021-02-01&format=text"
# eastus

curl -H "Metadata: true" "http://169.254.169.254/metadata/instance/compute/resourceGroupName?api-version=2021-02-01&format=text"
# my-resource-group

# Managed Identity Token
curl -H "Metadata: true" "http://169.254.169.254/metadata/identity/oauth2/token?api-version=2018-02-01&resource=https://vault.azure.net"
```

---

## 二、providerID

### 2.1 providerID 格式

providerID 是 Kubernetes Node 对象中用于唯一标识云实例的字段。它的格式因云厂商而异：

| 云厂商 | providerID 格式 | 示例 |
|--------|----------------|------|
| AWS | `aws:///\<zone\>/\<instance-id\>` | `aws:///us-east-1a/i-0abc123def456` |
| GCP | `gce://\<project\>/\<zone\>/\<instance-name\>` | `gce://my-project/us-central1-a/my-instance` |
| Azure | `azure:///subscriptions/\<sub\>/resourceGroups/\<rg\>/providers/Microsoft.Compute/virtualMachines/\<vm\>` | `azure:///subscriptions/xxx/resourceGroups/my-rg/providers/Microsoft.Compute/virtualMachines/my-vm` |
| vSphere | `vsphere://\<uuid\>` | `vsphere://423a3d3a-7e0a-4b9e-8c3a-123456789012` |

### 2.2 providerID 设置与查询

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

```bash
# kubelet 自动设置 providerID (通过 cloud provider)
# 查看 providerID
kubectl get node <node> -o jsonpath='{.spec.providerID}'

# 查看所有节点的 providerID
kubectl get nodes -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{.spec.providerID}{"\n"}{end}'

# 手动设置 providerID (不使用 cloud provider 时)
kubectl patch node <node> -p '{"spec":{"providerID":"aws:///us-east-1a/i-0abc123"}}'
```

### 2.3 providerID 在控制器中的作用

providerID 是多个 Kubernetes 控制器的关键输入：

```go
// pkg/controller/cloud/node/controller.go
func (c *CloudNodeController) nodeExists(ctx context.Context, node *v1.Node) (bool, error) {
    // 通过 providerID 在云厂商 API 中查询实例是否存在
    // 如果实例不存在，说明节点已被外部删除
    // 控制器会删除对应的 Node 对象
}
```

- **cloud-node-controller**：通过 providerID 检查实例是否存在
- **Cluster Autoscaler**：通过 providerID 关联 Node 和 ASG/VMSS/MIG
- **cloud-controller-manager**：通过 providerID 查询实例信息

---

## 三、云厂商节点标签

### 3.1 通用标签（所有云厂商）

kubelet 自动添加以下通用标签：

```yaml
# Kubernetes 通用标签:
kubernetes.io/arch: amd64                  # CPU 架构
kubernetes.io/os: linux                    # 操作系统
kubernetes.io/hostname: node-1             # 主机名
node.kubernetes.io/instance-type: t3.medium  # 实例类型
topology.kubernetes.io/region: us-east-1    # 区域
topology.kubernetes.io/zone: us-east-1a     # 可用区
```

### 3.2 AWS 特有标签

```yaml
# EKS 自动添加的标签:
topology.kubernetes.io/zone: us-east-1a
topology.kubernetes.io/region: us-east-1
node.kubernetes.io/instance-type: t3.medium
eks.amazonaws.com/nodegroup: my-nodegroup
eks.amazonaws.com/capacityType: ON_DEMAND  # ON_DEMAND 或 SPOT
```

### 3.3 GCP 特有标签

```yaml
# GKE 自动添加的标签:
cloud.google.com/gke-nodepool: default-pool
cloud.google.com/gke-os-distribution: cos
cloud.google.com/machine-family: n1
topology.gke.io/zone: us-central1-a
topology.kubernetes.io/zone: us-central1-a
```

### 3.4 Azure 特有标签

```yaml
# AKS 自动添加的标签:
kubernetes.azure.com/agentpool: default
kubernetes.azure.com/cluster: my-aks-cluster
kubernetes.azure.com/nodepool-mode: System
kubernetes.azure.com/os-sku: Ubuntu
topology.kubernetes.io/zone: eastus-1
topology.kubernetes.io/region: eastus
```

### 3.5 基于标签的调度

```yaml
# 利用云厂商标签进行调度
apiVersion: v1
kind: Pod
metadata:
  name: gpu-workload
spec:
  nodeSelector:
    node.kubernetes.io/instance-type: p3.2xlarge   # AWS GPU 实例
    topology.kubernetes.io/zone: us-east-1a         # 特定可用区
  containers:
  - name: app
    image: tensorflow/serving:latest

# 使用 nodeAffinity 进行更灵活的调度
apiVersion: v1
kind: Pod
metadata:
  name: spot-workload
spec:
  affinity:
    nodeAffinity:
      requiredDuringSchedulingIgnoredDuringExecution:
        nodeSelectorTerms:
        - matchExpressions:
          - key: eks.amazonaws.com/capacityType
            operator: In
            values:
            - SPOT
  containers:
  - name: app
    image: nginx
```

---

## 四、节点污点与容忍

### 4.1 云厂商自动添加的污点

```bash
# AWS EKS 污点 (Spot 实例)
node.kubernetes.io/instance-type=p3.2xlarge:NoSchedule  # 特定实例类型

# GCP GKE 污点
cloud.google.com/gke-preemptible=true:NoSchedule         # 抢占式实例
cloud.google.com/gke-provisioner:NoSchedule               # GKE 自动调配

# Azure AKS 污点
kubernetes.azure.com/scalesetpriority=spot:NoSchedule     # Spot VM
kubernetes.azure.com/agentpool=spot:NoSchedule            # Spot 节点池
```

### 4.2 自定义污点管理

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `kubectl taint nodes`：变更污点影响 Pod 调度

```bash
# 添加污点
kubectl taint nodes <node> dedicated=gpu:NoSchedule

# 删除污点
kubectl taint nodes <node> dedicated=gpu:NoSchedule-

# 查看污点
kubectl describe node <node> | grep Taints
```

### 4.3 Pod 容忍云厂商污点

```yaml
# 容忍 Spot 实例污点
spec:
  tolerations:
  - key: cloud.google.com/gke-preemptible
    operator: Equal
    value: "true"
    effect: NoSchedule
  - key: kubernetes.azure.com/scalesetpriority
    operator: Equal
    value: "spot"
    effect: NoSchedule
```

---

## 五、云厂商特有集成

### 5.1 AWS IAM Role for Service Account (IRSA)

```yaml
# EKS IRSA 配置
apiVersion: v1
kind: ServiceAccount
metadata:
  name: my-sa
  annotations:
    eks.amazonaws.com/role-arn: arn:aws:iam::123456789012:role/my-role
```

### 5.2 GCP Workload Identity

```yaml
# GKE Workload Identity
apiVersion: v1
kind: ServiceAccount
metadata:
  name: my-sa
  annotations:
    iam.gke.io/gcp-service-account: my-gsa@my-project.iam.gserviceaccount.com
```

### 5.3 Azure Managed Identity

```yaml
# AKS Managed Identity
apiVersion: v1
kind: ServiceAccount
metadata:
  name: my-sa
  annotations:
    azure.workload.identity/client-id: xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx
```

---

## 六、常见错误与排查

| 错误 | 原因 | 排查命令 | 解决方案 |
|------|------|---------|---------|
| providerID 为空 | cloud-controller-manager 未运行 | `kubectl get nodes -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{.spec.providerID}{"\n"}{end}'` | 部署 cloud-controller-manager |
| 节点无法注册 | providerID 冲突（重复使用 instance-id） | `kubectl get nodes | grep <id>` | 删除旧 Node 对象后重新注册 |
| 元数据获取失败 | 网络隔离或 IMDS 被阻止 | `curl http://169.254.169.254/` | 配置 NAT/安全组允许访问 IMDS |
| 标签缺失 | kubelet 未检测到云环境 | `kubectl describe node <node> | grep Labels` | 检查 `--cloud-provider` 参数 |
| AWS IMDSv2 超时 | hop limit 不足 | `aws ec2 modify-instance-metadata-options --instance-id i-xxx --http-put-response-hop-limit 2` | 增加 hop limit |
| GCP 元数据 403 | 缺少 Metadata-Flavor header | `curl -H "Metadata-Flavor: Google" ...` | 始终添加 header |

### 调试命令

```bash
# 检查 cloud-controller-manager 状态
kubectl get pods -n kube-system -l component=cloud-controller-manager

# 检查节点云标签
kubectl get node <node> -o jsonpath='{.metadata.labels}' | jq .

# 检查 providerID
kubectl get node <node> -o jsonpath='{.spec.providerID}'

# AWS: 检查实例元数据
TOKEN=$(curl -s -X PUT "http://169.254.169.254/latest/api/token" -H "X-aws-ec2-metadata-token-ttl-seconds: 30")
curl -s -H "X-aws-ec2-metadata-token: $TOKEN" http://169.254.169.254/latest/meta-data/instance-id

# GCP: 检查实例元数据
curl -s -H "Metadata-Flavor: Google" http://metadata.google.internal/computeMetadata/v1/instance/id

# Azure: 检查实例元数据
curl -s -H "Metadata: true" "http://169.254.169.254/metadata/instance?api-version=2021-02-01" | jq .
```

---

## 相关函数

| 函数/组件 | 源码位置 | 说明 |
|----------|---------|------|
| `CloudNodeController` | `pkg/controller/cloud/node/controller.go` | 云节点控制器 |
| `InitializeCloudNode` | `pkg/controller/cloud/node/controller.go` | 节点云初始化 |
| `providerID` | `pkg/api/v1/node/types.go` | providerID 字段定义 |
| `Instances` | `pkg/cloudprovider/instances.go` | 实例接口 |
| `InstanceMetadata` | `pkg/cloudprovider/instances.go` | 实例元数据接口 |

## Related

- [[reference|#reference Hub]] — tag hub

- [[log|log]]
- [[domain-17-system-foundation/topic-cheat-sheet/go.md|go]]
- [[domain-17-system-foundation/topic-cheat-sheet/k8s.md|k8s]]
- [[entities/kubernetes.md|kubernetes]]
- [[domain-17-system-foundation/topic-dictionary/fundamentals/cloud-controller-manager.md|cloud-controller-manager]]

```