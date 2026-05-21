---
title: 云厂商差异化故障场景
description: '# "Flannel plugin not ready" → 使用 terway 网络插件'
category: general
tags:
- k8s
- scheduler
- flannel
- coredns
- daemonset
- ingress
- networkpolicy
- gpu
- agent
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 云厂商差异化故障场景 是什么
- 如何 云厂商差异化故障场景
- 云厂商差异化故障场景 故障排查
- 云厂商差异化故障场景 排障步骤
trigger_keywords:
- 云厂商差异化故障场景
prerequisites:
- kubectl-basics
- gpu-scheduling-basics
---

# 云厂商差异化故障场景

> **版本**: v1.0
> **创建日期**: 2026-05-18
> **用途**: ACK/EKS/GKE/AKS 特有故障场景的诊断与修复
> **关联**: domain-17-cloud-provider, domain-10-troubleshooting-diagnostics

---

## 1. 阿里云 ACK / ACK (Alibaba Cloud [[entities/kubernetes|kubernetes]])

### 1.1 控制平面故障 (CCE 控制台异常)

| 症状 | 诊断命令 | 根因 | 修复 |
|------|---------|------|------|
| API Server 无响应 | `aliyun cs DescribeClusterDetail --clusterId <id>` | 控制平面异常 | 提交工单联系阿里云 |
| 节点 NotReady | `aliyun cs DescribeClusterNodes --clusterId <id>` | 节点被回收/升级 | 检查节点池状态 |
| 组件异常 | `aliyun cs DescribeTaskInfo --task-id <id>` | 任务执行失败 | 查看任务详情 |

```bash
# ACK 控制平面检查
aliyun cs DescribeClusters --query "clusters[].cluster_id"
aliyun cs DescribeClusterDetail --clusterId <cluster_id>

# 节点状态检查
aliyun cs DescribeClusterNodes --clusterId <cluster_id>
aliyun cs DescribeNodesInCluster --clusterId <cluster_id> --node-pool-id <pool_id>

# 常见问题
# "Master zone is maintenance" → 等待或联系阿里云
# "Resource quota exceeded" → 清理资源或申请配额
# "Flannel plugin not ready" → 使用 terway 网络插件

# 升级集群
aliyun cs InstallClusterAddons --clusterId <id> --addon-name csi-plugin
```

### 1.2 Terway 网络问题

| 症状 | 诊断命令 | 根因 | 修复 |
|------|---------|------|------|
| Pod 无 IP (ENI 模式) | `kubectl describe pod <pod> \| grep -A5 Events` | ENI 资源不足 | 增加 ENI 配额或减少 Pod |
| 跨节点网络不通 | `aliyun vpc DescribeVpcAttribute --VpcId <vpc_id>` | VPC 路由问题 | 检查路由表和安全组 |
| Terway Pod 异常 | `kubectl get pods -n kube-system -l k8s-app=terway` | Terway DaemonSet 问题 | 重启 Terway Pod |

```bash
# Terway 诊断
kubectl logs -n kube-system -l k8s-app=terway --tail=100

# ENI 状态检查
aliyun vpc DescribeNetworkInterfaces --Type eni

# 常见问题
# "ENI IP exhausted" → 申请更多 ENI IP 或改用 VPC-CNI 模式
# "Security group rule limit" → 调整安全组规则
# "Terway daemon not running" → 重启 aliyun-infra DaemonSet
```

### 1.3 存储问题 (云盘/OSS)

| 症状 | 诊断命令 | 根因 | 修复 |
|------|---------|------|------|
| 云盘 PVC Pending | `kubectl describe pvc` | 云盘配额不足 | 清理未使用的云盘 |
| 存储卷只读 | `kubectl describe pod` | 云盘达到配额上限 | 扩容云盘或删除数据 |
| OSS 挂载失败 | `kubectl describe pod` | OSS 权限问题 | 检查 AK/STS 凭证 |

```bash
# ACK 存储检查
aliyun smartag DescribeSagDevices --region <region>
aliyun bss DescribeBill --product="云盘"

# CSI 驱动状态
kubectl get pods -n kube-system | grep csi

# 常见问题
# "Disk quota exceeded" → 在 ACK 控制台清理云盘或扩容
# "AccessKey expired" → 更新阿里云凭证 Secret
```

---

## 2. AWS EKS 故障排查

### 2.1 Fargate 特殊调度问题

| 症状 | 诊断命令 | 根因 | 修复 |
|------|---------|------|------|
| Pod 无法调度到 Fargate | `kubectl describe pod` | 无匹配的 Fargate profile | 创建/修改 Fargate profile |
| Fargate Pod 一直 Pending | `aws eks describe-fargate-profile --cluster-name <name>` | Profile 配置问题 | 检查 profile 标签选择器 |
| 网络策略不生效 | `kubectl get networkpolicy` | Fargate 不支持 network policy | 使用 AWS Security Group |

```bash
# Fargate Profile 检查
aws eks describe-fargate-profile --cluster-name <name> --fargate-profile-name <profile>
aws eks list-fargate-profiles --cluster-name <name>

# 查看 Pod 所在节点
kubectl get pod -o wide

# 常见问题
# "No matching Fargate profile" → 确认 namespace/labels 与 profile 匹配
# "Fargate ENI limit" → 联系 AWS 提高 ENI 限制

# 创建 Fargate Profile
aws eks create-fargate-profile \
  --cluster-name <name> \
  --profile-name <profile> \
  --namespace <ns> \
  --pod-selector-label <label>
```

### 2.2 ALB Ingress 限制

| 症状 | 诊断命令 | 根因 | 修复 |
|------|---------|------|------|
| ALB Ingress 创建失败 | `kubectl describe ingress` | AWS LB Controller 未安装 | 安装 AWS LB Controller |
| ALB 404 错误 | `aws elbv2 describe-load-balancers` | 目标组健康检查失败 | 检查 Pod 健康检查端口 |
| 跨 Namespace 路由失败 | `kubectl get ingress -A` | IAM 权限不足 | 配置 IRSA (IAM Role for Service Account) |

```bash
# AWS LB Controller 检查
kubectl get pods -n kube-system | grep aws-load-balancer-controller

# ALB 状态检查
aws elbv2 describe-load-balancers --names <alb-name>
aws elbv2 describe-target-groups --target-group-arn <arn>

# 常见问题
# "Ingress class not found" → 安装 aws-load-balancer-controller
# "Cross-namespace not allowed" → 需要 annotations: alb.ingress.kubernetes.io/group.name

# IRSA 配置
aws iam create-role --role-name EKS-ALB-controller --assume-role-policy-document file://trust-policy.json
aws iam attach-role-policy --role-name EKS-ALB-controller --policy-arn arn:aws:iam::aws:policy/AmazonEKSClusterPolicy
```

### 2.3 EKS 节点问题

| 症状 | 诊断命令 | 根因 | 修复 |
|------|---------|------|------|
| Managed Node 无法加入 | `aws eks describe-cluster --name <name>` | Node IAM Role 权限不足 | 检查 instance role |
| Bottlerocket 节点异常 | `aws ssm describe-instance-information` | SSM Agent 问题 | 检查 SSM Agent 状态 |

```bash
# EKS 节点健康检查
aws eks describe-cluster --name <name> --query "cluster.resourcesVpcConfig"
aws ec2 describe-instances --filters "Name=tag:eks:cluster-name,Values=<name>"

# 常见问题
# "Node creation failed" → 检查 node IAM role 是否有必要权限
# "Nodes not joining cluster" → 检查 security group 规则

# Managed Node Group 升级
aws eks update-nodegroup-version --cluster-name <name> --nodegroup-name <ng> --kubernetes-version <version>
```

---

## 3. GCP GKE 故障排查

### 3.1 Autopilot 资源限制

| 症状 | 诊断命令 | 根因 | 修复 |
|------|---------|------|------|
| Pod 调度失败 | `kubectl describe pod` | Autopilot 强制资源限制 | 调整 resource requests |
| GPU 不可用 | `kubectl get nodes -l cloud.google.com/gke-accelerator` | Autopilot 不支持 GPU | 使用标准模式集群 |
| 存储无法创建 | `kubectl describe pvc` | Autopilot 限制 ReadWriteOnce | 使用 Cloud SQL 等托管服务 |

```bash
# GKE Autopilot 模式检查
kubectl get nodes -o wide | grep "SCHEDULER"

# 查看资源配额
kubectl describe resourcequota -n <ns>

# 常见问题
# "resource.memory exceeded" → Autopilot 自动限制内存
# "No node matches affinity" → Autopilot 节点不匹配自定义规则

# 切换到标准模式
gcloud container clusters create <cluster> --zone <zone> --no-enable-autopilot
```

### 3.2 Anthos 配置问题

| 症状 | 诊断命令 | 根因 | 修复 |
|------|---------|------|------|
| Anthos Config Management 异常 | `kubectl get pods -n config-management-system` | GitHub 连接问题 | 检查 Connector 配置 |
| Policy Controller 拒绝 | `kubectl get constraints` | 不符合 Policy | 修改资源符合策略 |

```bash
# Anthos 配置检查
kubectl get pods -n anthos-config-management
kubectl describe syncs -n config-management-system

# Config Sync 状态
kubectl get gkerepos -A
kubectl describe root-sync -n config-management-system

# 常见问题
# "RepoSync not found" → 配置 RepoSync CR
# "Policy violation" → 查看 Policy Controller 日志

# 重新同步
kubectl delete syncs -n config-management-system --all
```

---

## 4. Azure AKS 故障排查

### 4.1 Azure CNI 限额问题

| 症状 | 诊断命令 | 根因 | 修复 |
|------|---------|------|------|
| Pod 无法获取 IP | `kubectl describe pod` | Azure CNI IP 池耗尽 | 增加 IP 池或减少 Pod |
| 节点网络配置错误 | `kubectl get nodes -o wide` | CNI 配置问题 | 重启 aks-npm (Azure policy) |

```bash
# Azure CNI 检查
kubectl get pods -n kube-system -l k8s-app=azure-cni
kubectl logs -n kube-system -l k8s-app=azure-cni --tail=100

# 查看 VNet 配置
az network vnet show --resource-group <rg> --name <vnet>

# 常见问题
# "IP address exhaustion" → 分割更多子网或启用 Azure CNI 覆盖
# "Azure firewall blocking traffic" → 配置 Azure Firewall 规则

# Azure CNI 网络诊断
az aks show --resource-group <rg> --name <cluster> --query networkProfile
```

### 4.2 AKS 升级的特殊卡点

| 症状 | 诊断命令 | 根因 | 修复 |
|------|---------|------|------|
| 升级卡在 "InProgress" | `az aks show --resource-group <rg> --name <cluster>` | 节点池升级失败 | 检查虚拟机规模集状态 |
| 系统组件不兼容 | `kubectl get pods -n kube-system` | 新版本 API 不兼容 | 降级或等待组件更新 |

```bash
# AKS 升级状态
az aks show --resource-group <rg> --name <cluster> --query "agentPoolProfiles"
az aks list-upgrades --resource-group <rg> --name <cluster>

# 升级控制平面
az aks upgrade --resource-group <rg> --name <cluster> --kubernetes-version <version>

# 升级节点池
az aks nodepool upgrade --resource-group <rg> --cluster-name <cluster> --name <pool> --kubernetes-version <version>

# 常见问题
# "Node pool upgrade timeout" → 手动 drain 节点后重试
# "System addons incompatible" → 先升级 addons 再升级控制平面
```

---

## 5. 多云网络故障

### 5.1 跨云 VPC Peering / Transit

| 症状 | 诊断命令 | 根因 | 修复 |
|------|---------|------|------|
| 跨 VPC 网络不通 | ping/traceroute 测试 | Peering 路由未配置 | 配置路由表 |
| DNS 解析失败 | `nslookup cross-cloud-service` | 私有 DNS 未同步 | 配置 Cloud DNS 或 hosts |
| VPN 隧道不稳定 | `wg show` / `ipsec status` | MTU 问题或 NAT 问题 | 调整 MTU 或重启隧道 |

```bash
# AWS-阿里云跨云
# AWS 端配置
aws directconnect describe-connections

# 阿里云端配置
aliyun vpc DescribeRouterInterfaces --region <region>

# 检查路由传播
aws ec2 describe-route-tables --filters "Name=route-table-id,Values=<rt-id>"

# GCP-AWS 跨云
gcloud compute networks peerings list --router <router>
```

### 5.2 跨云 DNS 解析

```bash
# 方案 1: 私有 DNS 区域同步
# AWS Route53 → 阿里云 Private Zone (通过 DNS 同步服务)

# 方案 2: CoreDNS 条件转发
# 在 CoreDNS 配置 conditional forwarder 到各云 DNS

# 方案 3: 手动 /etc/hosts
# 在 Pod 中配置 hosts 文件

# 检查跨云 DNS 解析
kubectl exec -it <pod> -- nslookup <cross-cloud-domain>
```

---

## 6. 云厂商特有诊断命令汇总

| 云厂商 | 控制平面检查 | 节点检查 | 网络检查 |
|--------|------------|---------|---------|
| ACK | `aliyun cs DescribeClusterDetail` | `aliyun cs DescribeClusterNodes` | `aliyun vpc DescribeVpcs` |
| AWS | `aws eks describe-cluster` | `aws ec2 describe-instances` | `aws elbv2 describe-load-balancers` |
| GCP | `gcloud container clusters describe` | `gcloud compute instances list` | `gcloud compute networks list` |
| Azure | `az aks show` | `az vm list` | `az network lb show` |

---

## 7. 快速检查清单

### 云厂商 on-call 速查

```bash
# ACK
aliyun cs DescribeClusters && aliyun cs DescribeClusterDetail --clusterId <id>
aliyun cs DescribeClusterNodes --clusterId <id>

# AWS EKS
aws eks describe-cluster --name <cluster> --query "cluster.status"
aws ec2 describe-instances --filters "Name=tag:eks:cluster-name,Values=<cluster>"

# GCP GKE
gcloud container clusters describe <cluster> --zone <zone>
gcloud compute instances list --filter "labels.goog-gke-node"

# Azure AKS
az aks show --resource-group <rg> --name <cluster>
az vm list --resource-group <rg>
```

---

**关联文档**:
- [domain-12-cloud-providers/](../domain-12-cloud-providers/) — 云厂商选型对比
- [domain-03-networking-traffic/](../domain-03-networking-traffic/) — Kubernetes 网络
- [P1-5: On-call 快速参考卡](./P1-5-oncall-quick-reference-card.md)