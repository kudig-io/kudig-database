---
title: EKS 网络深度 — VPC CNI 与高级网络配置
description: 'EKS VPC CNI 调优、Security Groups for Pods、Prefix Delegation、IPv6 双栈及 Pod 密度规划'
summary: 'EKS VPC CNI 调优、Security Groups for Pods、Prefix Delegation、IPv6 双栈及 Pod 密度规划'
category: cloud-providers
tags:
- cloud
- k8s
- aws
- eks
- networking
- vpc-cni
tier: supporting
created: '2026-07-02'
last_updated: 2026-07
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 网络工程师
estimated_read_time: 15min
intent_queries:
- EKS VPC CNI 是什么
- 如何配置 EKS 网络
trigger_keywords:
- vpc-cni
- prefix-delegation
- security-groups-for-pods
- ipv6-dual-stack
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


# EKS 网络深度 — VPC CNI 与高级网络配置

## 1. VPC CNI 架构

EKS 使用 Amazon VPC CNI 插件为 Pod 分配 VPC 真实 IP 地址。每个 Pod 拥有一个可路由的 VPC IP，无需 NAT 即可与 VPC 内其他资源通信。

### 1.1 IP 分配机制

```
ENI (Elastic Network Interface)
├── Primary IP (节点 IP)
├── Secondary IP 1 → Pod A
├── Secondary IP 2 → Pod B
└── Secondary IP N → Pod N

每个实例可挂载的 ENI 数量和每个 ENI 的 IP 数量由实例类型决定
```

### 1.2 VPC CNI 配置调优

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看当前配置
kubectl describe daemonset aws-node -n kube-system | grep -A 50 "Environment"
```
```yaml
# vpc-cni-config ConfigMap
apiVersion: v1
kind: ConfigMap
metadata:
  name: amazon-vpc-cni
  namespace: kube-system
data:
  # 启用 Prefix Delegation（推荐）
  enable-prefix-delegation: "true"
  # 预热前缀数量
  warm-prefix-target: "1"
  # 每个前缀的 IP 数量（/28 = 16 个 IP）
  warm-prefix-target: "1"
  # 启用 IPv4 前缀委派
  warm-eni-target: "1"
  # 最大 ENI 数量（0 = 自动）
  max-eni: "0"
  # Pod 连接超时
  pod-veth-prefix-regex: "^eni-"
  # IP 温池大小
  warm-ip-target: "5"
  # 最小 IP 数量
  minimum-ip-target: "3"
```

## 2. Prefix Delegation 模式

Prefix Delegation 是 VPC CNI 的重大改进，为每个 ENI 分配 /28 前缀（16 个 IP）而非单个 IP。

### 2.1 启用 Prefix Delegation

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 方式一：通过 Addon 配置
aws eks update-addon \
  --cluster-name prod-cluster \
  --addon-name vpc-cni \
  --configuration-values '{
    "env": {
      "ENABLE_PREFIX_DELEGATION": "true",
      "WARM_PREFIX_TARGET": "1",
      "WARM_IP_TARGET": "5",
      "MINIMUM_IP_TARGET": "3"
    }
  }'

# 方式二：直接修改 DaemonSet
kubectl set env daemonset aws-node \
  -n kube-system \
  ENABLE_PREFIX_DELEGATION=true \
  WARM_PREFIX_TARGET=1 \
  WARM_IP_TARGET=5 \
  MINIMUM_IP_TARGET=3
```
### 2.2 Prefix Delegation vs Secondary IP

| 特性 | Secondary IP | Prefix Delegation |
|------|-------------|-------------------|
| 每 ENI IP 数 | 取决于实例类型 | 16 个（/28 前缀） |
| Pod 密度上限 | 受 ENI 限制 | 大幅提升 |
| IP 地址消耗 | 每 Pod 一个 | 每 16 Pod 一个前缀 |
| 适用场景 | Pod 数量少 | 大规模集群 |
| 子网规划 | 需大子网 | 子网压力小 |

### 2.3 Pod 密度规划

```
实例类型        ENI数   Secondary IP/PD   最大Pod数(PD)
m6i.large       3       10/48              48
m6i.xlarge      4       15/64              64
m6i.2xlarge     4       15/64              64
m6i.4xlarge     8       30/128             128
m6i.8xlarge     8       30/128             128
m6i.16xlarge    15      50/240             240
m6i.24xlarge    15      50/240             240
```

## 3. Security Groups for Pods (SGP)

SGP 允许将 VPC Security Group 直接关联到 Pod，实现细粒度网络隔离。

### 3.1 启用 SGP

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 确认 VPC CNI 版本 >= 1.11
kubectl describe daemonset aws-node -n kube-system \
  | grep Image

# 启用 SGP
kubectl set env daemonset aws-node \
  -n kube-system \
  ENABLE_POD_ENI=true

# 确认节点支持
kubectl get nodes -o json \
  | jq '.items[].metadata.labels["vpc.amazonaws.com/has-trunk-attached"]'
```
### 3.2 创建 SecurityGroupPolicy

```yaml
apiVersion: vpcresources.k8s.aws/v1beta1
kind: SecurityGroupPolicy
metadata:
  name: app-sgp
  namespace: production
spec:
  podSelector:
    matchLabels:
      app: my-app
      tier: web
  securityGroups:
    groupIds:
      - sg-0123456789abcdef0  # 允许出站到 RDS
      - sg-0fedcba9876543210  # 允许入站 8080
```

```yaml
# Service Account 级别 SGP
apiVersion: vpcresources.k8s.aws/v1beta1
kind: SecurityGroupPolicy
metadata:
  name: backend-sa-sgp
  namespace: production
spec:
  serviceAccountSelector:
    matchLabels:
      app: backend
  securityGroups:
    groupIds:
      - sg-backend-allow-redis
      - sg-backend-allow-rds
```

### 3.3 SGP 限制

- 最多 5 个 Security Group 附加到一个 Pod
- 不兼容 Security Group 有 stateless 规则时的某些场景
- 需要节点有 trunk ENI，会占用一个 ENI 槽位
- 不支持 Windows 节点

## 4. 自定义网络配置

### 4.1 Secondary CIDR

当主 CIDR IP 不足时，可为 VPC 添加 Secondary CIDR。

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 添加 Secondary CIDR（必须与主 CIDR 不重叠）
aws ec2 associate-vpc-cidr-block \
  --vpc-id vpc-0123456789abcdef0 \
  --cidr-block 100.64.0.0/16

# 创建新子网使用 Secondary CIDR
aws ec2 create-subnet \
  --vpc-id vpc-0123456789abcdef0 \
  --cidr-block 100.64.0.0/18 \
  --availability-zone ap-southeast-1a \
  --tag-specifications 'ResourceType=subnet,Tags=[{Key=Name,Value=pod-subnet-1a}]'
```
### 4.2 CNI Custom Networking

```yaml
# eniconfig.yaml — 指定 Pod 使用的子网和安全组
apiVersion: crd.k8s.amazonaws.com/v1alpha1
kind: ENIConfig
metadata:
  name: ap-southeast-1a
  namespace: kube-system
spec:
  securityGroups:
    - sg-pod-security-group
  subnet: subnet-pod-1a-100-64

---
apiVersion: crd.k8s.amazonaws.com/v1alpha1
kind: ENIConfig
metadata:
  name: ap-southeast-1b
  namespace: kube-system
spec:
  securityGroups:
    - sg-pod-security-group
  subnet: subnet-pod-1b-100-64
```

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 启用自定义网络
kubectl set env daemonset aws-node \
  -n kube-system \
  AWS_VPC_K8S_CNI_CUSTOM_NETWORK_CFG=true \
  ENI_CONFIG_LABEL_DEF=topology.kubernetes.io/zone
```
## 5. IPv6 双栈网络

### 5.1 创建 IPv6 集群

```bash
# eksctl IPv6 集群
eksctl create cluster \
  --name ipv6-cluster \
  --version 1.31 \
  --vpc-cidr 10.0.0.0/16 \
  --ip-family ipv6
```

```yaml
# Terraform IPv6 配置
module "eks" {
  source = "terraform-aws-modules/eks/aws"
  
  cluster_name    = "ipv6-cluster"
  cluster_version = "1.31"
  
  cluster_ip_family = "ipv6"
  # service_ipv4_cidr 和 service_ipv6_cidr 自动配置
  
  vpc_id     = module.vpc.vpc_id
  subnet_ids = module.vpc.private_subnets
}
```

### 5.2 IPv6 注意事项

- Pod 同时获得 IPv4 和 IPv6 地址（双栈）
- Service 支持 IPv4 或 IPv6（不能同时）
- `ip-family: ipv6` 创建时指定，不可更改
- CoreDNS 默认监听 IPv6 地址
- 部分 AWS 服务尚不支持 IPv6 端点

## 6. VPC Flow Logs 集成

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 为 VPC 启用 Flow Logs
aws ec2 create-flow-logs \
  --resource-type VPC \
  --resource-ids vpc-0123456789abcdef0 \
  --traffic-type ALL \
  --log-destination-type cloud-watch-logs \
  --log-group-name /aws/vpc/flowlogs \
  --deliver-logs-permission-arn arn:aws:iam::123456789012:role/vpc-flowlog-role \
  --max-aggregation-interval 60
```
```yaml
# CloudWatch Insights 查询 — 查找异常流量
fields @timestamp, srcAddr, dstAddr, dstPort, action, bytes
| filter dstPort = 443 and action = "REJECT"
| stats sum(bytes) as totalBytes by srcAddr, dstAddr
| sort totalBytes desc
| limit 20
```

## 7. 网络性能调优

### 7.1 ENI 参数调优

```yaml
# vpc-cni ConfigMap 关键参数
apiVersion: v1
kind: ConfigMap
metadata:
  name: amazon-vpc-cni
  namespace: kube-system
data:
  # 启用 Pod 级别的网络带宽控制
  ENABLE_BANDWIDTH_PLUGIN: "true"
  # 默认带宽限制（入/出，单位 KBPS）
  POD_ENI_PLACEMENT: "true"
```

### 7.2 MTU 配置

```bash
# 检查节点 MTU
ip link show eth0

# VPC MTU 默认 9001（Jumbo Frame）
# 如果使用 VPN/专线连接，可能需要降低到 1500
```

### 7.3 网络诊断命令

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 检查 VPC CNI 状态
kubectl get pods -n kube-system -l k8s-app=aws-node -o wide

# 查看节点 ENI 分配
kubectl get nodes -o json | jq '.items[] | {
  name: .metadata.name,
  enis: .status.allocatable["vpc.amazonaws.com/eni"],
  prefixes: .status.allocatable["vpc.amazonaws.com/prefix-eni"]
}'

# 检查 Pod IP 分配
kubectl get pods -A -o json | jq '.items[] | {
  name: .metadata.name,
  node: .spec.nodeName,
  podIP: .status.podIP,
  podIPs: .status.podIPs
}'

# 测试 Pod 间网络连通性
kubectl run nettest --image=nicolaka/netshoot --rm -it -- bash
# 容器内执行：
# ping <target-pod-ip>
# traceroute <target-pod-ip>
# curl -s http://<service-name>.<namespace>.svc.cluster.local
```
## Related

- [[02-eks-cluster-lifecycle-management]]
- [[06-eks-troubleshooting-playbook]]

## See Also

- AWS VPC CNI GitHub
- EKS Best Practices Guide — Networking


<!-- risk-assessed -->
