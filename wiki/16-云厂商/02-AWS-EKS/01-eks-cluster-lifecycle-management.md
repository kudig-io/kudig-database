---
title: EKS 集群生命周期管理
description: 'EKS 集群创建、版本升级、EKS Auto Mode、EKS Anywhere 及节点组管理的生产级实践'
summary: 'EKS 集群创建、版本升级、EKS Auto Mode、EKS Anywhere 及节点组管理的生产级实践'
category: cloud-providers
tags:
- cloud
- k8s
- aws
- eks
- lifecycle
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
- EKS 集群生命周期管理 是什么
- 如何管理 EKS 集群生命周期
trigger_keywords:
- eks
- cluster-lifecycle
- eksctl
- managed-node-group
- eks-auto-mode
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

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。


# EKS 集群生命周期管理

## 1. 集群创建方式对比

### 1.1 eksctl（推荐快速起步）

```yaml
# cluster.yaml
apiVersion: eksctl.io/v1alpha5
kind: ClusterConfig
metadata:
  name: prod-cluster
  region: ap-southeast-1
  version: "1.31"
availabilityZones:
  - ap-southeast-1a
  - ap-southeast-1b
  - ap-southeast-1c
vpc:
  cidr: 10.0.0.0/16
  nat:
    gateway: HighlyAvailable
  clusterEndpoints:
    publicAccess: true
    privateAccess: true
iam:
  withOIDC: true
managedNodeGroups:
  - name: system-ng
    instanceType: m6i.xlarge
    minSize: 3
    maxSize: 10
    desiredCapacity: 3
    labels:
      role: system
    taints:
      - key: CriticalAddonsOnly
        value: "true"
        effect: NoSchedule
    volumeSize: 100
    volumeType: gp3
    ssh:
      allow: false
  - name: app-ng
    instanceType: m6i.2xlarge
    minSize: 5
    maxSize: 50
    desiredCapacity: 5
    labels:
      role: app
    volumeSize: 200
    volumeType: gp3
    spot: false
addons:
  - name: vpc-cni
    version: latest
    attachPolicyARNs:
      - arn:aws:iam::aws:policy/AmazonEKS_CNI_Policy
  - name: coredns
    version: latest
  - name: kube-proxy
    version: latest
  - name: aws-ebs-csi-driver
    version: latest
    attachPolicyARNs:
      - arn:aws:iam::aws:policy/service-role/AmazonEBSCSIDriverPolicy
cloudWatch:
  clusterLogging:
    enableTypes:
      - api
      - audit
      - authenticator
      - controllerManager
      - scheduler
```

```bash
# 创建集群
eksctl create cluster -f cluster.yaml

# 查看集群状态
eksctl get cluster --name prod-cluster --region ap-southeast-1
```

### 1.2 Terraform（生产推荐）

```hcl
# main.tf
module "eks" {
  source  = "terraform-aws-modules/eks/aws"
  version = "~> 20.0"

  cluster_name    = "prod-cluster"
  cluster_version = "1.31"

  cluster_endpoint_public_access  = true
  cluster_endpoint_private_access = true

  vpc_id     = module.vpc.vpc_id
  subnet_ids = module.vpc.private_subnets

  # 集群加密
  cluster_encryption_config = {
    provider_key_arn = aws_kms_key.eks.arn
    resources        = ["secrets"]
  }

  # 集群附加组件
  cluster_addons = {
    vpc-cni = {
      most_recent = true
      configuration_values = jsonencode({
        env = {
          ENABLE_PREFIX_DELEGATION = "true"
          WARM_PREFIX_TARGET       = "1"
        }
      })
    }
    coredns = {
      most_recent = true
    }
    kube-proxy = {
      most_recent = true
    }
    aws-ebs-csi-driver = {
      most_recent = true
    }
  }

  # 托管节点组
  eks_managed_node_groups = {
    system = {
      name            = "system-ng"
      instance_types  = ["m6i.xlarge"]
      min_size        = 3
      max_size        = 10
      desired_size    = 3
      disk_size       = 100
      disk_type       = "gp3"
      labels = { role = "system" }
      taints = [{
        key    = "CriticalAddonsOnly"
        value  = "true"
        effect = "NO_SCHEDULE"
      }]
    }
    app = {
      name            = "app-ng"
      instance_types  = ["m6i.2xlarge", "m6a.2xlarge"]
      min_size        = 5
      max_size        = 50
      desired_size    = 5
      disk_size       = 200
      disk_type       = "gp3"
      labels = { role = "app" }
    }
  }

  # IRSA
  enable_irsa = true

  # 日志
  cluster_enabled_log_types = [
    "api", "audit", "authenticator", "controllerManager", "scheduler"
  ]
}
```

### 1.3 AWS Console

Console 创建适合快速验证，生产环境不推荐。关键步骤：

1. **Networking** — 选择 VPC、Subnet（至少 2 个 AZ）、Security Group
2. **Logging** — 开启所有控制平面日志类型
3. **Add-ons** — 安装 vpc-cni、coredns、kube-proxy、ebs-csi-driver
4. **Node Group** — 配置实例类型、容量、标签和污点

## 2. 版本升级策略

### 2.1 升级顺序（必须遵守）

```
控制平面 → CoreDNS / kube-proxy / vpc-cni → 节点组 → 工作负载适配
```

### 2.2 控制平面升级

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看可用版本
aws eks describe-cluster-versions \
  --query 'clusterVersions[?clusterVersionStatus==`ACTIVE`]'

# 升级控制平面（每次只能升一个小版本）
aws eks update-cluster-version \
  --name prod-cluster \
  --kubernetes-version 1.31 \
  --region ap-southeast-1

# 监控升级状态
aws eks describe-cluster --name prod-cluster \
  --query 'cluster.status'
```
升级通常需要 10-20 分钟，期间 API Server 短暂不可用（约 1-2 分钟）。

### 2.3 Addon 升级

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看当前 addon 版本
aws eks list-addons --cluster-name prod-cluster

# 升级 vpc-cni
aws eks update-addon \
  --cluster-name prod-cluster \
  --addon-name vpc-cni \
  --addon-version v1.18.1-eksbuild.1 \
  --resolve-conflicts OVERWRITE

# 升级 CoreDNS
aws eks update-addon \
  --cluster-name prod-cluster \
  --addon-name coredns \
  --addon-version v1.11.3-eksbuild.1 \
  --resolve-conflicts OVERWRITE
```
### 2.4 节点组滚动升级

> **🔴 高风险操作警告**
>
> 下方命令属于不可逆或高影响操作，执行前请确认：
> - 已备份关键数据与配置
> - 处于批准的变更窗口期
> - 已获得相关责任人授权
> - 已准备回滚或恢复方案
> - 目标集群、Namespace、节点/资源名称正确无误

``` bash
# 🔴 高风险：可能造成数据丢失或服务中断，执行前需备份、变更审批与回滚方案
# Terraform 方式 — 更新 instance_types 或 version 触发滚动
terraform apply -target=module.eks

# eksctl 方式
eksctl upgrade nodegroup \
  --name app-ng \
  --cluster prod-cluster \
  --kubernetes-version 1.31

# 手动 Drain 节点
kubectl drain <node-name> --ignore-daemonsets --delete-emptydir-data
```
### 2.5 Pod Disruption Budget 保障升级安全

```yaml
apiVersion: policy/v1
kind: PodDisruptionBudget
metadata:
  name: app-pdb
  namespace: production
spec:
  minAvailable: 80%
  selector:
    matchLabels:
      app: my-app
```

## 3. EKS Auto Mode

EKS Auto Mode（2024 年发布）将节点管理完全交给 AWS，不再需要手动管理 Node Group。

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 创建 Auto Mode 集群
aws eks create-cluster \
  --name auto-cluster \
  --kubernetes-version 1.31 \
  --compute-config '{"nodePools":["system","general-purpose"]}' \
  --storage-config '{"blockStorage":{"enabled":true}}' \
  --kubernetes-network-config '{"serviceIpv4Cidr":"172.20.0.0/16"}' \
  --access-config '{"authenticationMode":"API_AND_CONFIG_MAP"}'
```
```yaml
# 节点池配置
apiVersion: karpenter.sh/v1beta1
kind: NodePool
metadata:
  name: general-purpose
spec:
  template:
    spec:
      requirements:
        - key: karpenter.sh/capacity-type
          operator: In
          values: ["on-demand"]
        - key: node.kubernetes.io/instance-type
          operator: In
          values: ["m6i.xlarge", "m6i.2xlarge", "m6a.xlarge", "m6a.2xlarge"]
      nodeClassRef:
        name: default
  limits:
    cpu: "1000"
    memory: 2000Gi
  disruption:
    consolidationPolicy: WhenUnderutilized
    expireAfter: 720h
```

Auto Mode 核心优势：
- 自动选择实例类型和大小
- 自动伸缩和整合（consolidation）
- 内置 EBS 存储和 ALB/NLB 负载均衡
- 按 Pod 计费模式（Pay-per-pod pricing preview）

## 4. EKS Anywhere

EKS Anywhere 将 EKS 运行在自有基础设施上。

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 安装 EKS Anywhere CLI
curl "https://anywhere-assets.eks.amazonaws.com/releases/eks-a/manifest.yaml" \
  | sed 's/v0.0.0/v0.21.0/' \
  | kubectl apply -f -

# 生成集群配置
eksctl anywhere generate clusterconfig prod-onprem \
  --provider vsphere > cluster.yaml

# 创建集群
eksctl anywhere create cluster -f cluster.yaml
```
```yaml
# cluster.yaml 核心配置
apiVersion: anywhere.eks.amazonaws.com/v1alpha1
kind: Cluster
metadata:
  name: prod-onprem
spec:
  clusterNetwork:
    pods:
      cidrBlocks:
        - 10.244.0.0/16
    services:
      cidrBlocks:
        - 10.96.0.0/16
  controlPlaneConfiguration:
    count: 3
    endpoint:
      host: "10.0.50.100"
    machineGroupRef:
      kind: VSphereMachineConfig
      name: prod-onprem-cp
  workerNodeGroupConfigurations:
    - count: 5
      machineGroupRef:
        kind: VSphereMachineConfig
        name: prod-onprem-worker
```

## 5. Managed Node Group 管理

### 5.1 节点组策略

| 场景 | 实例类型 | 容量策略 | 适用场景 |
|------|---------|---------|---------|
| System | m6i.xlarge | On-Demand | 系统组件，必须稳定 |
| General | m6i/m6a 混合 | On-Demand + Spot | 通用工作负载 |
| Compute | c6i/c6a | On-Demand | CPU 密集型 |
| Memory | r6i/r6a | On-Demand | 内存密集型 |
| GPU | p4d/g5 | On-Demand | ML 训练/推理 |
| Spot | 多类型混合 | Spot | 容错批处理 |

### 5.2 节点组扩缩容

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 手动扩缩
aws eks update-nodegroup-config \
  --cluster-name prod-cluster \
  --nodegroup-name app-ng \
  --scaling-config minSize=3,maxSize=100,desiredSize=10

# 配置 Cluster Autoscaler
kubectl apply -f cluster-autoscaler-autodiscover.yaml
```
```yaml
# Cluster Autoscaler Deployment
apiVersion: apps/v1
kind: Deployment
metadata:
  name: cluster-autoscaler
  namespace: kube-system
spec:
  template:
    spec:
      containers:
        - name: cluster-autoscaler
          image: registry.k8s.io/autoscaling/cluster-autoscaler:v1.31.0
          command:
            - ./cluster-autoscaler
            - --v=4
            - --stderrthreshold=info
            - --cloud-provider=aws
            - --skip-nodes-with-local-storage=false
            - --expander=least-waste
            - --node-group-auto-discovery=asg:tag=k8s.io/cluster-autoscaler/enabled,k8s.io/cluster-autoscaler/prod-cluster
            - --balance-similar-node-groups
            - --skip-nodes-with-system-pods=false
```

## 6. Fargate Profile 配置

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 创建 Fargate Profile
aws eks create-fargate-profile \
  --cluster-name prod-cluster \
  --fargate-profile-name batch-jobs \
  --pod-execution-role-arn arn:aws:iam::123456789012:role/eks-fargate-role \
  --selectors '[{"namespace":"batch","labels":{"compute":"fargate"}}]' \
  --subnets subnet-aaaa subnet-bbbb subnet-cccc
```
```yaml
# eksctl Fargate Profile
fargateProfiles:
  - name: batch-jobs
    selectors:
      - namespace: batch
        labels:
          compute: fargate
      - namespace: batch
        labels:
          compute: fargate-spot
  - name: kubernetes-dashboard
    selectors:
      - namespace: kubernetes-dashboard
```

Fargate 限制：
- 不支持 DaemonSet
- 不支持 hostNetwork
- 最大 4 vCPU / 30 GB 内存
- 仅支持 EFS（不支持 EBS）

## 7. 集群删除

> **🔴 高风险操作警告**
>
> 下方命令属于不可逆或高影响操作，执行前请确认：
> - 已备份关键数据与配置
> - 处于批准的变更窗口期
> - 已获得相关责任人授权
> - 已准备回滚或恢复方案
> - 目标集群、Namespace、节点/资源名称正确无误

``` bash
# 🔴 高风险：可能造成数据丢失或服务中断，执行前需备份、变更审批与回滚方案
# eksctl 删除（自动清理关联资源）
eksctl delete cluster --name prod-cluster --region ap-southeast-1

# Terraform 删除
terraform destroy -target=module.eks

# 手动清理残留资源
# 1. 删除 Load Balancer
# 2. 删除 EBS 卷
# 3. 删除 Security Group
# 4. 删除 CloudWatch Log Group
# 5. 删除 IAM Role 和 Policy
```
> **警告**：删除集群前必须确认所有持久化数据已备份，PVC 对应的 EBS 卷默认保留。

## Related

- [[02-eks-networking-vpc-cni]]
- [[03-eks-storage-efs-fsx]]
- [[04-eks-iam-irsa-pod-identity]]

## See Also

- AWS EKS 官方文档
- eksctl 文档
- Terraform EKS 模块


<!-- risk-assessed -->
