---
title: 云厂商托管 Kubernetes 服务全景对比（13 家）
description: '| 厂商 | 产品 | K8s 版本支持 | 网络模型 | 亮点 |'
summary: '| 厂商 | 产品 | K8s 版本支持 | 网络模型 | 亮点 |'
category: reference
tags:
- k8s
- cloud-provider
- managed-k8s
- aws
- azure
- gcp
- alicloud
- huawei
- tencent
- flannel
tier: core
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 云厂商托管 Kubernetes 服务全景对比（13 家） 是什么
- 如何 云厂商托管 Kubernetes 服务全景对比（13 家）
trigger_keywords:
- 云厂商托管
- Kubernetes
- 服务全景对比
- '13'
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 云厂商托管 K8s 服务全景对比

## 概述

云厂商托管 Kubernetes 服务是企业上云的核心选择。主流云厂商都提供托管的 Kubernetes 控制平面管理（Control Plane Managed），用户只需管理工作节点。选型需要考虑生态成熟度、网络模型、价格、区域覆盖和技术支持等因素。

## 主流厂商对比

| 厂商 | 产品 | K8s 版本支持 | 网络模型 | 亮点 | 定价模型 |
|------|------|-------------|----------|------|----------|
| AWS | EKS | v1.25-v1.33 | VPC CNI | 最成熟生态、IRSA | $0.10/h/control plane |
| Azure | AKS | v1.26-v1.33 | Azure CNI | 混合云集成、Azure Arc | 免费 control plane |
| Google | GKE | v1.27-v1.33 | GKE Networking | Autopilot、最佳 K8s 体验 | $0.10/h (Standard) |
| 阿里云 | ACK | v1.24-v1.32 | Terway/Flannel | ACK Edge、国内最大 | 按 Pro/标准版 |
| 华为云 | CCE | v1.25-v1.32 | CCE Turbo | 鲲鹏 ARM 生态 | 按节点数 |
| 腾讯云 | TKE | v1.24-v1.32 | VPC-CNI | 游戏场景优化 | 免费 control plane |
| 火山引擎 | VEK | v1.26-v1.32 | VPC CNI | 字节跳动经验 | 免费 control plane |

## 网络模型差异

- **VPC CNI（AWS/阿里 Terway）**：Pod 直接获得 VPC IP，无 NAT，网络性能最优但 IP 消耗大
- **Azure CNI**：类似 VPC CNI，Pod 获得 Azure VNet IP
- **Overlay（Flannel/Calico VXLAN）**：Pod 使用 overlay CIDR，通过隧道通信，IP 消耗小但性能略低
- **GKE VPC-native**：Alias IP 模式，兼顾 VPC 原生和 IP 效率

## 特性差异

| 特性 | EKS | AKS | GKE | ACK |
|------|-----|-----|-----|-----|
| Serverless 节点 | Fargate | ACI | Autopilot | Virtual Kubelet |
| GPU 支持 | 完整 | 完整 | 完整 | 完整 |
| Spot 实例 | 支持 | 支持 | 支持 | 支持 |
| IRSA/Workload Identity | IRSA | OIDC | Workload Identity | RRSA |
| Ingress Controller | ALB | App Gateway | GCE Ingress | ALB |

## 选型建议

- **全球化业务**：AWS EKS（最成熟生态）/ Google GKE（最佳 K8s 体验和 Autopilot）
- **国内公有云**：阿里云 ACK（市场份额最大）/ 华为云 CCE（鲲鹏 ARM 优势）/ 腾讯云 TKE（游戏场景优化）
- **混合云**：Azure AKS（Azure Arc）/ Anthos GKE / ACKAnywhere
- **边缘场景**：阿里云 ACK Edge / KubeEdge / SuperEdge
- **成本敏感**：AKS/TKE（免费 control plane）/ GKE Autopilot（按 Pod 计费）

## 迁移考量

跨云迁移注意事项：
1. **网络模型**：VPC-native 到 Overlay 迁移需要重新规划 CIDR
2. **存储 CSI**：云厂商存储接口不兼容，需使用 CSI 标准接口
3. **IAM 认证**：IRSA → Workload Identity 需调整 Pod 配置
4. **Ingress/Gateway**：云厂商 Ingress 实现不同，建议使用标准 Ingress/Gateway API

## 运维操作

### 通用集群检查命令

```bash
# 🟢 检查集群版本和健康状态
kubectl version --short 2>/dev/null || kubectl version
kubectl get nodes -o wide
kubectl get componentstatuses 2>/dev/null || kubectl get --raw /healthz?verbose

# 🟢 检查云厂商 CSI 驱动
kubectl get csidrivers
kubectl get storageclass

# 🟢 检查云厂商 LoadBalancer 集成
kubectl get svc -A --field-selector spec.type=LoadBalancer

# 🟢 检查节点实例类型
kubectl get nodes -o custom-columns=NAME:.metadata.name,INSTANCE:.metadata.labels.'node\.kubernetes\.io/instance-type',ZONE:.metadata.labels.'topology\.kubernetes\.io/zone'
```

### 各厂商 CLI 工具

```bash
# AWS EKS
aws eks describe-cluster --name my-cluster --region us-east-1
aws eks update-kubeconfig --name my-cluster --region us-east-1

# Azure AKS
az aks show --resource-group my-rg --name my-cluster
az aks get-credentials --resource-group my-rg --name my-cluster

# Google GKE
gcloud container clusters describe my-cluster --zone us-central1-a
gcloud container clusters get-credentials my-cluster --zone us-central1-a

# 阿里云 ACK
aliyun cs DescribeClusterDetail --ClusterId <id>
aliyun cs GET /k8s/<id>/user_config
```

## 故障排查

| 症状 | 可能原因 | 诊断命令 | 修复方案 |
|------|----------|----------|----------|
| LB Service 无外部 IP | 云配额不足/子网无可用IP | `kubectl describe svc` | 检查云控制台配额 |
| 节点无法加入 | 安全组/子网配置 | 检查云控制台节点组 | 修正安全组规则 |
| PV 挂载失败 | CSI 驱动异常/AZ不匹配 | `kubectl get pods -n kube-system` | 检查 CSI Pod/存储类 |
| 集群升级失败 | 版本跳跃/插件不兼容 | 检查云控制台升级日志 | 逐版本升级/更新插件 |
| Pod 网络不通 | VPC CIDR 冲突/安全组 | 检查 VPC 配置 | 修正 CIDR/安全组规则 |

### 跨云迁移检查清单

```
跨云迁移流程
├── 评估阶段
│   ├── 盘点云厂商特有 API 依赖
│   ├── 检查存储 CSI 兼容性
│   ├── 检查 IAM/认证方式差异
│   └── 检查网络模型差异
├── 改造阶段
│   ├── 替换云厂商特有 Ingress 为 Gateway API
│   ├── 使用标准 CSI StorageClass
│   ├── 替换 IRSA/Workload Identity 为目标云方案
│   └── 移除云厂商特有 annotation
├── 验证阶段
│   ├── 功能测试（LB/存储/DNS）
│   ├── 性能基准测试
│   └── 故障转移测试
└── 切换阶段
    ├── DNS 切换/流量迁移
    ├── 监控确认
    └── 回滚方案就绪
```

## 生产案例

### 案例1：AWS EKS 迁移到阿里云 ACK

- **场景**：业务进入中国市场，需从 AWS EKS 迁移到阿里云 ACK
- **挑战**：IRSA → RRSA、ALB Ingress → ACK ALB、EBS CSI → 云盘 CSI
- **方案**：使用 KubeVela 抽象应用层；逐步替换云厂商特有组件；DNS 权重切换
- **效果**：2 周内完成迁移，零停机

### 案例2：多云容灾架构

- **场景**：金融客户要求双云容灾，RPO < 1min，RTO < 5min
- **方案**：AWS EKS + Azure AKS 双活；使用 Submariner 跨云网络互通；Velero 备份到共享 S3；全局 DNS 故障转移
- **效果**：单云故障时 3 分钟内自动切换，满足金融合规

## 成本优化建议

| 策略 | 适用厂商 | 节省比例 | 注意事项 |
|------|----------|----------|----------|
| Spot/抢占式实例 | 所有 | 60-90% | 需处理中断 |
| 预留实例/承诺使用 | AWS/Azure/GCP | 30-50% | 1-3年承诺 |
| Autopilot/Serverless | GKE/EKS Fargate | 变动 | 按实际使用计费 |
| 自动伸缩 (Cluster Autoscaler) | 所有 | 20-40% | 缩容延迟 |
| 资源右 sizing | 所有 | 20-30% | 需监控数据支撑 |

## 检查清单

- [ ] 已评估业务对云厂商特有服务的依赖
- [ ] 网络模型已规划（CIDR 不冲突）
- [ ] 存储使用标准 CSI 接口
- [ ] 认证方式使用标准 OIDC（便于迁移）
- [ ] Ingress 使用标准 Gateway API
- [ ] 备份和容灾策略已配置
- [ ] 成本优化策略已实施
- [ ] 集群升级策略已制定

---

> 来源：.zread/wiki/drafts/22-yun-han-shang-tuo-guan-*.md

## Related

- [[volcengine-vek-overview]] — volcengine-vek-overview
- [[tencent-tke-overview]] — tencent-tke-overview
- [[alicloud-ack-overview]] — alicloud-ack-overview
- [[kubeedge]] — KubeEdge
- [[cni]] — CNI (Container Network Interface)


<!-- risk-assessed -->
