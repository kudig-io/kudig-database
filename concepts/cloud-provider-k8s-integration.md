---
title: 云厂商 K8S 集成
category: concepts
tags: [cloud, eks, gke, aks, ack, multi-cloud, k8s]
created: 2026-05-24
updated: 2026-05-24
last_updated: 2026-05-24
---

# 云厂商 K8S 集成

各主流云厂商对 Kubernetes 的深度集成能力，以及多云抽象层的统一管理方案。

## AWS EKS

### Auto Mode GA
EKS Auto Mode 将集群控制面、节点生命周期、网络、存储全部交由 AWS 托管：
- 自动扩缩节点组，无需手动管理 Node Group
- 内置 Karpenter 引擎处理 Pod 调度与节点供给
- 集成 EBS CSI、VPC CNI、Load Balancer Controller
- 适用场景：团队希望"零运维"K8S 基础设施

### Pod Identity 替代 IRSA
EKS Pod Identity（2024 GA）是 IRSA 的下一代替代方案：
- 无需为每个 ServiceAccount 创建 IAM Role 信任关系
- 通过 `PodIdentityAssociation` 将 Pod 直接绑定 IAM Role
- 支持同一 SA 跨多个 Role 场景
- 简化了多命名空间、多应用的权限管理

### Hybrid Nodes GA
EKS Hybrid Nodes 支持将本地/边缘节点加入 EKS 集群：
- 本地节点作为 EKS 集群的 Worker Node
- 控制面仍运行在 AWS 云上
- 适用于低延迟、数据驻留、混合云场景
- 与 AWS Outposts、Wavelength 互补

### Karpenter v1.12 CNCF
Karpenter 已捐赠 CNCF（Incubating），v1.12 关键特性：
- 多云支持：AWS、Azure、GCP 均有 provider
- NodePool/NodeClaim CRD 声明式节点管理
- 应用感知的 Right Sizing 推荐
- Spot/按需实例混合策略内置
- 与 EKS Auto Mode 深度集成

## GCP GKE

### Autopilot Pay-Per-Pod
GKE Autopilot 以 Pod 为计费单元，无需管理节点：
- 按 Pod 的 CPU/Memory/GPU 请求量计费
- Google 管理节点供应、升级、安全补丁
- 内置 Pod 安全策略、网络策略
- 最低 Pod 资源：0.25 vCPU、512MiB Memory

### GKE Enterprise 多集群 Fleet
GKE Enterprise 提供 Fleet 级多集群管理：
- 统一控制面管理数百个集群
- Config Sync 实现 GitOps 声明式配置
- Policy Controller（基于 OPA/Gatekeeper）合规准入
- Multi Cluster Service / Ingress 跨集群服务发现
- 支持 GKE、混合云、多云集群

### Multi-cluster Gateway
Multi-cluster Gateway 实现跨集群流量管理：
- 全局负载均衡跨多个 GKE 集群
- 支持流量分割、故障转移策略
- 与 Google Cloud CDN、Cloud Armor 集成
- Gateway API 原生支持

## Azure AKS

### AKS Automatic GA
AKS Automatic（2025 GA）是 Azure 的"全自动 K8S"模式：
- 自动管理节点池、升级、扩缩
- 内置网络策略、安全基线
- 与 Azure Monitor、Defender 深度集成
- 支持 Node Autoprovision（NAP，基于 Karpenter）

### KAITO AI 模型推理
KAITO（Kubernetes AI Toolchain Operator）简化 AI 推理部署：
- 自动下载 HuggingFace 模型到 AKS
- 内置 vLLM、TensorRT-LLM 推理引擎
- 自动管理 GPU 节点供应
- 支持 Llama、Mistral、Phi 等主流模型
- CRD 声明式管理模型生命周期

### Node Autoprovision
NAP（Node Autoprovision）是 AKS 的 Karpenter 集成：
- 基于 Pod 需求自动创建最优化节点
- 支持多种 VM SKU 混合
- Spot 实例自动集成
- 与 Azure 瞬时实例回收处理集成

## 阿里云 ACK

### ACK Serverless（ASK/ECI）
ACK Serverless 基于 ECI（Elastic Container Instance）实现无节点：
- Pod 直接运行在安全沙箱实例中
- 按秒计费，无节点空闲浪费
- 启动速度：秒级 Pod 调度
- 适用场景：突发流量、CI/CD、批处理任务

### ACK Pro 企业版
ACK Pro 提供企业级托管 Kubernetes：
- 跨可用区高可用控制面（99.95% SLA）
- 安全加固：运行时安全、网络策略、审计日志
- 与阿里云 RAM、SLS、ARMS 深度集成
- 支持托管节点池、弹性节点池
- ACK Edge 边缘节点管理

## 多云抽象层

### Cluster API（CAPI）30+ Providers
Cluster API 是 Kubernetes 原生的集群生命周期管理框架：
- 声明式定义集群、控制面、Machine 部署
- 支持 30+ 基础设施 Provider（AWS、GCP、Azure、vSphere、OpenStack 等）
- CNCF SIG Cluster Lifecycle 维护
- ClusterClass 模板化集群创建
- 与 Kustomize、Helm 集成 CI/CD

### Rancher（SUSE）
Rancher 是企业级多集群 K8S 管理平台：
- 统一管理任意位置的 K8S 集群
- 集成 Fleet GitOps 引擎
- 内置监控（Prometheus）、日志（Loki）、告警
- 多租户 RBAC、项目/命名空间管理
- 支持 RKE2/k3s 发行版

### Tanzu（Broadcom）
VMware Tanzu 提供企业级 K8S 平台：
- Tanzu Kubernetes Grid（TKG）多云 K8S 发行版
- Tanzu Mission Control 多集群管理
- Tanzu Application Platform（TAP）开发者体验
- 与 vSphere 深度集成（Supervisor Cluster）
- NSX 高级网络与安全

### Crossplane
Crossplane 将整个基础设施声明式管理：
- CRD 定义云资源（RDS、S3、VPC 等）
- Composition 将多个资源组合为抽象 API
- Provider 生态：AWS、GCP、Azure、阿里云等
- 与 GitOps 工作流天然集成
- 推动 Platform as Code 理念

### Gardener
SAP 开源的 Gardener 实现 K8S-as-a-Service：
- Shoot 集群声明式定义、Seed 集群管理
- 支持 AWS、GCP、Azure、OpenStack、阿里云等
- 自动管理控制面生命周期（升级、补丁、扩缩）
- CNCF 项目，企业级多租户
- OPA 策略驱动的合规管理

## 各云 CSI/CNI 对比表

### CSI（Container Storage Interface）

| 特性 | AWS EKS | GCP GKE | Azure AKS | 阿里云 ACK |
|------|---------|---------|-----------|------------|
| 默认 CSI | EBS CSI | GCE PD CSI | Azure Disk CSI | Alibaba Cloud CSI |
| 文件存储 | EFS CSI | Filestore CSI | Azure File CSI | NAS CSI |
| 高性能存储 | io2 Block Express | Hyperdisk | Ultra Disk | ESSD PL3 |
| 快照支持 | ✅ EBS Snapshots | ✅ PD Snapshots | ✅ Disk Snapshots | ✅ 快照 |
| 扩容 | 在线 + 离线 | 在线 + 离线 | 在线 + 离线 | 在线 + 离线 |
| 拓扑感知 | ✅ | ✅ | ✅ | ✅ |

### CNI（Container Network Interface）

| 特性 | AWS EKS | GCP GKE | Azure AKS | 阿里云 ACK |
|------|---------|---------|-----------|------------|
| 默认 CNI | VPC CNI | GKE CNI (Dataplane V2) | Azure CNI | Terway / Flannel |
| Pod IP 来源 | VPC 子网 | VPC 子网 (别名 IP) | VPC 子网 | VPC 子网 / Overlay |
| Network Policy | Calico | 内置 (基于 Cilium) | Azure NPF / Calico | Calico / Cilium |
| eBPF 数据面 | 可选 Cilium | ✅ Dataplane V2 | 可选 Cilium | 可选 Cilium |
| 带宽限制 | ✅ Pod 级别 | ✅ Pod 级别 | ✅ Pod 级别 | ✅ Pod 级别 |
| 安全组 Pod | ✅ Security Groups for Pods | ❌ | ❌ | ✅ 安全组 |

## 各云 Spot/抢占式实例策略

### AWS Spot 实例
- **折扣幅度**: 60-90%（相比按需价格）
- **中断通知**: 2 分钟提前通知（通过 SQS/EventBridge/IMDS）
- **Karpenter 集成**: `NodePool` 中设置 `consolidationPolicy` + 多 AZ 分散
- **最佳实践**: 使用 Capacity Optimized 策略选择中断概率最低的池
- **EKS Auto Mode**: 内置 Spot 管理，自动替换中断节点

### GCP 抢占式 VM（Spot VM）
- **折扣幅度**: 60-91%
- **中断通知**: 30 秒提前通知（preemptionNotice）
- **GKE 集成**: `nodeConfig.spot = true`，支持混合节点池
- **GKE Autopilot**: 支持 Spot Pod，按需+Spot 混合调度
- **最佳实践**: 配合 `disruptionBudget` 限制中断影响面

### Azure Spot 虚拟机
- **折扣幅度**: 最高 90%
- **中断通知**: 30 秒提前通知（Scheduled Events API）
- **AKS 集成**: Spot Node Pool，支持 `evictionPolicy=Delete/Deallocate`
- **NAP 集成**: 自动选择最合适的 Spot SKU
- **最佳实践**: 设置 `maxPrice=-1`（按需价格上限）避免价格波动

### 阿里云抢占式实例
- **折扣幅度**: 10-90%（阶梯定价）
- **中断通知**: 5 分钟提前通知（元数据服务）
- **ACK 集成**: 弹性节点池支持抢占式实例
- **ACK Serverless**: ECI 抢占式实例，适合无状态批处理
- **最佳实践**: 使用多规格实例池分散风险，配合 PDB 保护关键服务

### Spot 策略通用建议
1. **混合部署**: 关键服务按需 + 无状态工作负载 Spot
2. **多 AZ 分散**: 避免单可用区中断导致批量丢失
3. **PDB 配置**: 合理设置 `PodDisruptionBudget` 保障可用性
4. **优雅处理**: 实现 SIGTERM 处理、状态持久化、队列回退
5. **成本监控**: 使用 KubeCost/OpenCost 实时监控 Spot vs 按需成本比

## 跨云迁移建议

1. **抽象层先行**: 使用 Crossplane 或 Cluster API 定义基础设施抽象
2. **应用无云依赖**: 通过 Dapr 或服务网格屏蔽云特定 SDK
3. **存储迁移**: 使用 Velero 备份/恢复跨集群迁移
4. **网络互联**: 通过 Service Mesh 多集群实现跨云服务发现
5. **渐进迁移**: 通过 Multi-cluster Gateway 实现流量逐步切换

## 相关概念

- [[concepts/k8s-security-compliance.md|k8s security compliance]] — K8S 安全与合规实践
- [[concepts/gitops-production-operations.md|gitops production operations]] — GitOps 生产运维
- [[concepts/finops-greenops-practices.md|finops greenops practices]] — FinOps 与 GreenOps 实践
