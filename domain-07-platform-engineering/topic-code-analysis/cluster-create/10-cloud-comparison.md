---
title: 云厂商方案与 kubeadm 对比
description: 'title: 云厂商方案与 kubeadm 对比'
category: general
tags:
- reference
- etcd
- cilium
- flannel
- calico
- helm
- gateway
- gpu
- serverless
- rag
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 15min
intent_queries:
- 云厂商方案与 kubeadm 对比 是什么
- 如何 云厂商方案与 kubeadm 对比
- Kubernetes 07 platform engineering 最佳实践
trigger_keywords:
- 云厂商方案与
- kubeadm
- 对比
- platform
- engineering
- code
- analysis
prerequisites:
- kubectl-basics
- platform-engineering-basics
- helm-basics
- cilium-basics
- cni-basics
- etcd-basics
- gpu-scheduling-basics
---

title: 云厂商方案与 kubeadm 对比
description: '# 云厂商方案与 kubeadm 对比'
category: functions
tags:
- k8s
- operations
- cluster-management
- etcd
- cilium
- flannel
- calico
- helm
- gateway
- gpu
last_updated: '2026-05-18'
difficulty: intermediate
reading_level: intermediate
audience:
- DevOps工程师
- Kubernetes管理员
- 云架构师
- SRE
estimated_read_time: 5min
intent_queries:
- kubeadm vs EKS AKS GKE cloud provider managed Kubernetes comparison
- self-hosted Kubernetes vs managed Kubernetes cost and features
- EKS Anywhere vs kubeadm on premises Kubernetes
- Alibaba Cloud ACK Terway CNI vs self-managed Kubernetes
- Kubernetes managed vs self-managed pros cons enterprise
trigger_keywords:
- EKS
- AKS
- GKE
- ACK
- TKE
- EKS Anywhere
- kubeadm
- managed Kubernetes
- cloud Kubernetes
- Terway
- CNI comparison
- cloud provider comparison
related_domains:
- domain-01-cluster-fundamentals
- domain-10-troubleshooting-diagnostics
related_topics:
- kubeadm init
- HA cluster setup
- CNI networking
- cloud integration
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

# 云厂商方案与 kubeadm 对比

## 主要云厂商托管 K8s 方案

| 厂商 | 方案名称 | 底层 | 管理方式 |
|------|---------|------|---------|
| AWS | EKS | 托管 control-plane | AWS Fargate / EC2 |
| AWS | EKS Anywhere | kubeadm 变体 | 自行管理 |
| Azure | AKS | 托管 control-plane | Azure VM |
| GCP | GKE | 托管 control-plane | GCE |
| GCP | GKE Enterprise | Anthos | 混合云 |
| 阿里云 | ACK | 托管 control-plane | ECS |
| 腾讯云 | TKE | 托管 control-plane | CVM |

---

## kubeadm vs 云厂商托管对比

| 维度 | kubeadm | 云厂商托管 (EKS/AKS/GKE) |
|------|---------|------------------------|
| Control Plane | 自行运维 | 厂商托管 (高可用) |
| etcd | 自行运维 | 厂商托管，多副本 |
| 升级 | 手动触发 | 自动升级最新版本 |
| 可用性 | 依赖架构设计 | 原生高可用 |
| 成本 | 节点成本 | 控制面免费，节点付费 |
| 扩展性 | 受限于自身架构 | 云厂商全球扩展能力 |
| 合规 | 完全可控 | 依赖厂商认证 |
| 网络插件 | 自行选择/配置 | 内置 CNI |
| 存储 | 自行对接 CSI | 云厂商块存储/文件存储 |

---

## EKS 架构 (AWS)

```
                    ┌──────────────────┐
                    │   EKS Control Plane │
                    │   (AWS 托管)        │
                    │  API Server        │
                    │  etcd (3副本)      │
                    └──────────────────┘
                           │
        ┌──────────────────┼──────────────────┐
        ▼                  ▼                  ▼
  ┌──────────┐      ┌──────────┐      ┌──────────┐
  │  Node 1  │      │  Node 2  │      │  Node 3  │
  │ (EC2/ASG)│      │ (EC2/ASG)│      │ (EC2/ASG)│
  └──────────┘      └──────────┘      └──────────┘
```

**kubeadm 在 AWS**: 可用 EKS Anywhere 或手动 kubeadm 搭建

---

## EKS Anywhere 架构

EKS Anywhere 是 AWS 官方的本地 kubeadm 发行版:

```go
// EKS Anywhere 使用 kubeadm 作为底层
// 但添加了:
type EKSAnywhereConfig struct {
    Bundles     string  // 组件镜像包
    Cilium      bool    // 使用 Cilium CNI
    Bottlerocket string // 节点 OS
}
```

**关键差异**:
- 使用 Bottlerocket OS (容器专用)
- 内置 Cilium CNI
- 提供 EKS 管理工具 `eksctl`

---

## GKE 架构 (Google Cloud)

```
                    ┌──────────────────┐
                    │  GKE Control Plane │
                    │   (Google 托管)     │
                    │  API Server        │
                    │  etcd (多区域)      │
                    └──────────────────┘
                           │
        ┌──────────────────┼──────────────────┐
        ▼                  ▼                  ▼
  ┌──────────┐      ┌──────────┐      ┌──────────┐
  │  Node    │      │  Node    │      │  Node    │
  │ (GCE/GAE)│      │ (GCE/GAE)│      │ (GCE/GAE)│
  └──────────┘      └──────────┘      └──────────┘
```

---

## AKS 架构 (Azure)

```
                    ┌──────────────────┐
                    │  AKS Control Plane │
                    │   (Azure 托管)     │
                    │  API Server        │
                    │  etcd              │
                    └──────────────────┘
                           │
        ┌──────────────────┼──────────────────┐
        ▼                  ▼                  ▼
  ┌──────────┐      ┌──────────┐      ┌──────────┐
  │  Agent   │      │  Agent   │      │  Agent   │
  │ (VM/ASG) │      │ (VM/ASG) │      │ (VM/ASG) │
  └──────────┘      └──────────┘      └──────────┘
```

**关键特性**:
- Azure CNI 网络插件
- Azure Monitor / Log Analytics 集成
- AAD (Azure Active Directory) 集成

---

## ACK 架构 (阿里云)

```
                    ┌──────────────────┐
                    │  ACK Control Plane│
                    │   (阿里云托管)     │
                    │  API Server      │
                    │  etcd            │
                    └──────────────────┘
                           │
        ┌──────────────────┼──────────────────┐
        ▼                  ▼                  ▼
  ┌──────────┐      ┌──────────┐      ┌──────────┐
  │  Node    │      │  Node    │      │  Node    │
  │ (ECS/ECI)│      │ (ECS/ECI)│      │ (ECS/ECI)│
  └──────────┘      └──────────┘      └──────────┘
```

**关键定制**:
- **Terway CNI**: 阿里云自研网络插件，支持 Pod 独立 VPC ENI/IPvlan
- **AliCloud Controller Manager**: 阿里云资源管理 (SLB/OSS/ESS)
- **CSI-Plugin**: 阿里云云盘/NAS/OSS 存储
- **Logtail**: 日志收集
- **Agent**: 阿里云节点管理组件

**Terway vs Flannel**:
- Terway: 每个 Pod 有独立 ENI/IP，性能更好，但受 VPC IP 数量限制
- Flannel: VXLAN 隧道，所有 Pod 共享节点 IP

---

## TKE 架构 (腾讯云)

```
                    ┌──────────────────┐
                    │  TKE Control Plane│
                    │   (腾讯云托管)     │
                    │  API Server      │
                    │  etcd            │
                    └──────────────────┘
                           │
        ┌──────────────────┼──────────────────┐
        ▼                  ▼                  ▼
  ┌──────────┐      ┌──────────┐      ┌──────────┐
  │  Node    │      │  Node    │      │  Node    │
  │ (CVM/PEK)│      │ (CVM/PEK)│      │ (CVM/PEK)│
  └──────────┘      └──────────┘      └──────────┘
```

**关键定制**:
- **CCN (Cloud Container Network)**: 跨可用区容器网络
- **CBS (Cloud Block Storage)**: 腾讯云云盘 CSI
- **CLB**: 负载均衡器集成
- **TKE Gateway**: 集群 API Server 入口
- **YunAPI Controller**: 腾讯云资源管理

**TKE 网络模式**:
- Global Router: VXLAN 隧道，跨可用区
- VPC-CNI: Pod 使用独立 ENI，VPC 网络

---

## kubeadm 适用场景

```
推荐使用 kubeadm:
✅ 需要完全控制集群行为
✅ 在私有环境/裸金属/VMWare
✅ 需要定制 API Server 参数
✅ 离线环境
✅ 边缘计算场景
✅ 实验/学习/开发环境

推荐云厂商托管:
✅ 希望减少运维工作
✅ 需要快速扩缩容
✅ 需要多可用区高可用
✅ 使用云厂商其他服务 (存储/负载均衡/安全)
✅ 不想运维 control plane
```

---

## 混合架构: 云厂商 + kubeadm 节点

很多企业采用:

```
云厂商托管 Control Plane (EKS/AKS/GKE)
    +
kubeadm 管理的 Worker 节点 (特殊需求节点/GPU 节点)
```

```bash
# 添加 kubeadm 节点到 EKS
aws eks update-kubeconfig --name my-cluster
# 生成 worker join 命令
kubeadm token create --print-join-command
# 在 worker 节点执行 join
```

---

## 迁移: kubeadm → 云厂商托管

| 步骤 | 操作 |
|------|------|
| 1 | 在云厂商创建新托管集群 |
| 2 | 部署相同的应用 (Helm/Kustomize) |
| 3 | 验证应用正常运行 |
| 4 | 流量切换到新集群 |
| 5 | 销毁旧 kubeadm 集群 |

**注意**: 不能原地升级 kubeadm 集群为 EKS，两者 control-plane 实现不同。

---

## 云厂商 kubeadm 变体对比

| 方案 | 基于 | 特殊定制 |
|------|------|---------|
| EKS Anywhere | kubeadm | Bottlerocket OS, Cilium, eksctl |
| ACK (阿里云) | kubeadm 变体 | Terway CNI, 阿里云 CSI |
| TKE (腾讯云) | kubeadm 变体 | CCN/CNI 插件, CBS CSI |
| DOKS (DigitalOcean) | kubeadm | 托管 control-plane |

---

---

## 四大云厂商功能对比

| 功能 | EKS | AKS | GKE | ACK |
|------|-----|-----|-----|-----|
| 托管 Control Plane | ✅ | ✅ | ✅ | ✅ |
| 多可用区 HA | ✅ | ✅ | ✅ | ✅ |
| 自动升级 | ✅ (1个月内) | ✅ (手动) | ✅ (自动) | ✅ (手动) |
| Fargate/Serverless | ✅ | ❌ | ✅ (GKE Autopilot) | ✅ (ECI) |
| GPU 节点 | ✅ (EKS on EC2) | ✅ | ✅ (GKE Autopilot) | ✅ (ECS GPU) |
| 裸金属节点 | ❌ | ❌ | ✅ (GKE on Bare Metal) | ❌ |
| 私有集群 | ✅ (PrivateLink) | ✅ (Private Link) | ✅ (私有集群) | ✅ (PrivateLink) |
| OIDC 联邦 | ✅ | ✅ | ✅ | ✅ |
| 网络策略 | ✅ (Calico) | ✅ (Calico) | ✅ (Calico) | ✅ (Terway) |
| 存储 CSI | ✅ (EBS/EFS/FSx) | ✅ (Azure Disk/Blob) | ✅ (PD/S Filestore) | ✅ (云盘/NAS/OSS) |
| 免费额度 | 750h/month | 免 Control Plane | 首次 $300 | 每月 1 节点免费 |

---

## 总结

- **kubeadm**: 通用、底层、标准化，适合私有化/定制场景
- **云厂商托管**: 开箱即用，免运维，适合大多数生产场景
- **混合**: 利用各自优势，托管 control-plane + 自管理特殊节点

## Related

- [[log.md|log]]
- [[domain-17-system-foundation/topic-cheat-sheet/go.md|go]]
- [[domain-17-system-foundation/topic-cheat-sheet/networking.md|networking]]
- [[domain-17-system-foundation/topic-cheat-sheet/helm.md|helm]]
- [[domain-17-system-foundation/topic-cheat-sheet/k8s.md|k8s]]
