---
title: Kubernetes Cloud Provider Comparison — EKS vs GKE vs AKS vs Self-Managed
description: K8s 云厂商对比 — EKS/GKE/AKS/自建集群的功能对比、成本分析、选型决策矩阵、迁移策略
summary: 全面对比主流 Kubernetes 托管服务的功能、成本与适用场景，提供选型决策框架
category: reference
tags:
- cloud-provider
- eks
- gke
- aks
- comparison
- decision-matrix
tier: supporting
created: '2026-07-21'
last_updated: '2026-07-21'
difficulty: intermediate
domain: entity-research
---
# Kubernetes 云厂商对比与选型

> EKS、GKE、AKS 与自建集群的全面对比与决策框架。

## 核心功能对比

| 维度 | EKS (AWS) | GKE (Google) | AKS (Azure) | 自建 |
|------|-----------|--------------|-------------|------|
| 控制平面费用 | $0.10/h (~$73/月) | 免费(Standard) | 免费 | 硬件/VM |
| 版本支持 | N-2 | N-3 | N-2 | 任意 |
| 自动升级 | ✅ | ✅ | ✅ | 手动 |
| 节点自动缩放 | Karpenter/CA | CA/Node Auto-Provisioning | CA/KEDA | CA |
| GPU 支持 | ✅ 全类型 | ✅ TPU+GPU | ✅ | ✅ |
| ARM 节点 | Graviton | Tau T2A | Cobalt | 自选 |
| Spot/抢占 | ✅ Spot | ✅ Preemptible | ✅ Spot | 无 |
| 多 AZ | ✅ | ✅ | ✅ | 自建 |
| 服务网格 | App Mesh/Istio | Anthos/Istio | Istio | Istio |
| Serverless | Fargate | Autopilot/GKE | Virtual Nodes | Knative |
| Windows 节点 | ✅ | ✅ | ✅ | ✅ |
| 私有集群 | ✅ | ✅ | ✅ | 默认 |
| 合规认证 | SOC/HIPAA/PCI | SOC/HIPAA/PCI | SOC/HIPAA/PCI | 自证 |

## 网络对比

| 功能 | EKS | GKE | AKS |
|------|-----|-----|-----|
| CNI | VPC CNI/Calico/Cilium | GKE CNI/Calico | Azure CNI/Calico |
| Pod CIDR | VPC 子网/自定义 | 自定义/共享 | 自定义 |
| Service CIDR | 自定义 | 自定义 | 自定义 |
| 负载均衡 | NLB/ALB (LBC) | ILB/GLB | Azure LB/App GW |
| Ingress | ALB/Nginx | GCE/Nginx | App GW/Nginx |
| 服务网格 | Istio/App Mesh | Istio/ASM | Istio/ASM |
| 私有端点 | ✅ | ✅ | ✅ |
| IPv6 | ✅ | ✅ | ✅ |

## 存储对比

| 功能 | EKS | GKE | AKS |
|------|-----|-----|-----|
| 块存储 CSI | EBS CSI | PD CSI | Disk CSI |
| 文件存储 CSI | EFS CSI | Filestore CSI | Files CSI |
| 快照 | ✅ | ✅ | ✅ |
| 扩容 | ✅ | ✅ | ✅ |
| 加密 | KMS | CMEK | Key Vault |
| 高性能 | io2 Block Express | Hyperdisk | Ultra Disk |

## 可观测性对比

| 功能 | EKS | GKE | AKS |
|------|-----|-----|-----|
| 托管监控 | CloudWatch | Cloud Monitoring | Azure Monitor |
| 托管日志 | CloudWatch Logs | Cloud Logging | Log Analytics |
| Prometheus | AMP (托管) | GMP (托管) | Monitor Metrics |
| 追踪 | X-Ray | Cloud Trace | App Insights |
| 成本分析 | CUR + Kubecost | 内置 | Cost Management |

## 成本分析（100 节点集群）

| 项目 | EKS | GKE | AKS | 自建 |
|------|-----|-----|-----|------|
| 控制平面 | $73/月 | $0 (Standard) | $0 | ~$500/月 |
| 节点 (m5.xlarge) | ~$14,000/月 | ~$13,500/月 | ~$13,800/月 | ~$10,000/月 |
| 网络 (跨 AZ) | 含 | 含 | 含 | 含 |
| LB | ~$200/月 | ~$150/月 | ~$180/月 | 自建 |
| 存储 (10TB) | ~$1,000/月 | ~$850/月 | ~$900/月 | ~$500/月 |
| 运维人力 | 0.5 FTE | 0.3 FTE | 0.5 FTE | 2 FTE |
| **月度总计** | **~$16,000** | **~$15,000** | **~$15,500** | **~$25,000** |

*注: 不含数据传输费用和实际折扣*

## 选型决策矩阵

```
选择 EKS 如果:
├── 已深度使用 AWS 生态（Lambda、DynamoDB、SQS）
├── 需要 Graviton ARM 节点（性价比）
├── 需要 Fargate 无节点模式
└── 合规要求 AWS 特定认证

选择 GKE 如果:
├── 已深度使用 GCP 生态（BigQuery、Pub/Sub）
├── 需要 Autopilot 全托管（最少运维）
├── 需要 TPU 支持（AI/ML）
├── 追求最低控制平面成本
└── 需要最成熟的 K8s 体验（Google 发明）

选择 AKS 如果:
├── 已深度使用 Azure 生态（AD、Office 365）
├── 需要 Windows 容器（.NET 遗留）
├── 混合云（Azure Arc）
└── 企业已有 Azure EA 协议

选择自建如果:
├── 数据主权/合规（不能上公有云）
├── 超大规模（> 1000 节点，成本优势）
├── 特殊硬件需求（GPU 集群/FPGA）
├── 边缘计算场景
└── 多云/混合云统一管控
```

## 迁移策略

### 跨云迁移清单

```markdown
## 迁移前评估
- [ ] 应用依赖梳理（云服务 API）
- [ ] 数据迁移方案（对象存储/数据库）
- [ ] 网络架构重设计（VPC/子网）
- [ ] IAM 映射（RBAC 转换）
- [ ] 密钥迁移（Vault/KMS）
- [ ] 监控告警重建
- [ ] CI/CD 流水线适配

## 迁移执行
- [ ] 基础设施即代码（Terraform 多云）
- [ ] 容器镜像迁移（Registry 同步）
- [ ] DNS 切换方案（权重渐进）
- [ ] 数据同步验证
- [ ] 性能基准对比
- [ ] 回滚预案

## 迁移后验证
- [ ] 功能测试通过
- [ ] 性能达标（延迟/吞吐）
- [ ] 成本符合预期
- [ ] 监控告警正常
- [ ] 安全扫描通过
- [ ] DR 演练通过
```

### 多云抽象层

```yaml
# Terraform 多云抽象
# modules/k8s-cluster/main.tf
variable "cloud_provider" {
  type    = string
  default = "aws"  # aws / gcp / azure
}

module "cluster" {
  source = "./${var.cloud_provider}"
  
  cluster_name    = var.cluster_name
  kubernetes_version = var.k8s_version
  node_count      = var.node_count
  node_type       = var.node_type
  region          = var.region
}
```

## 版本支持时间线

| 版本 | EKS EOL | GKE EOL | AKS EOL |
|------|---------|---------|---------|
| 1.28 | 2025-01 | 2024-12 | 2025-01 |
| 1.29 | 2025-05 | 2025-04 | 2025-05 |
| 1.30 | 2025-09 | 2025-08 | 2025-09 |
| 1.31 | 2026-01 | 2025-12 | 2026-01 |
| 1.32 | 2026-05 | 2026-04 | 2026-05 |
| 1.33 | 2026-09 | 2026-08 | 2026-09 |

## Related

- [[23-实体/01-research/k8s-cloud-providers/index.md|云厂商研究]]
- [[23-实体/01-research/k8s-cloud-providers/kubernetes-cloud-providers-2025-2026.md|2025-2026 云厂商]]
- [[01-集群基础/index.md|集群基础]]
- [[13-生产运维/01-成本治理/index.md|成本治理]]
