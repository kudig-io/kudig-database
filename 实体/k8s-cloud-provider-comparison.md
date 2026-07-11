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

---

> 来源：.zread/wiki/drafts/22-yun-han-shang-tuo-guan-*.md

## Related

- [[volcengine-vek-overview]] — volcengine-vek-overview
- [[tencent-tke-overview]] — tencent-tke-overview
- [[alicloud-ack-overview]] — alicloud-ack-overview
- [[kubeedge]] — KubeEdge
- [[cni]] — CNI (Container Network Interface)


<!-- risk-assessed -->
