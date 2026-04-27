# Domain-17 云厂商 — 开源项目索引

> **最后更新**: 2026-04-24

---

## 核心项目与托管服务

### AWS

| 项目/服务 | 作用 | 开源性 | 备注 |
|:---|:---|:---|:---|
| **Amazon EKS** | 托管 K8s | 商业 | 最成熟的云托管 K8s |
| **EKS Distro** | K8s 发行版 | 开源 | 与 EKS 同源 |
| **EKS Anywhere** | 本地部署 EKS | 部分开源 | 基于 Cluster API |
| **Karpenter** | 节点自动伸缩 | 开源 | AWS 官方 |
| **AWS Load Balancer Controller** | ALB/NLB 管理 | 开源 | Ingress/Service 集成 |
| **Amazon ECR** | 托管镜像仓库 | 商业 | 与 IAM 集成 |
| **AWS CDK** | 基础设施即代码 | 开源 | 编程式 IaC |
| **kro** | Kube Resource Orchestrator | 开源 | AWS 2024 发布 |
| **Hyperlight** | 轻量级 hypervisor | 申请 Sandbox | 函数级隔离 |

### Google Cloud

| 项目/服务 | 作用 | 开源性 | 备注 |
|:---|:---|:---|:---|
| **Google GKE** | 托管 K8s | 商业 | Autopilot 模式无节点管理 |
| **GKE Enterprise** | 多集群管理 | 商业 | Fleet/Config Sync |
| **Config Connector** | K8s 原生 GCP 资源 | 开源 | 类似 Crossplane |
| **Knative** | Serverless on K8s | CNCF Graduated | Google 发起 |
| **Tekton** | 云原生 CI/CD | CDF | Google 发起 |
| **Skaffold** | 本地 K8s 开发 | 开源 | Google 维护 |

### Azure

| 项目/服务 | 作用 | 开源性 | 备注 |
|:---|:---|:---|:---|
| **Azure AKS** | 托管 K8s | 商业 | 与 Azure 生态深度集成 |
| **AKS Engine** | K8s 部署工具 | 开源 | 逐步被 Cluster API 替代 |
| **Azure Service Operator** | K8s 管理 Azure 资源 | 开源 | 类似 Config Connector |
| **Dapr** | 分布式运行时 | CNCF Graduated | 微软发起 |
| **Helm** | 包管理器 | CNCF Graduated | 微软 Deis 发起 |
| **Open Service Mesh** | 服务网格 (已归档) | 微软 | 已停止开发 |
| **Hyperlight** | 轻量级 hypervisor | 申请 Sandbox | Azure Core Upstream 团队 |

### 阿里云

| 项目/服务 | 作用 | 开源性 | 备注 |
|:---|:---|:---|:---|
| **ACK** | 托管 K8s | 商业 | 国内最广泛 |
| **OpenYurt** | 边缘 K8s 扩展 | CNCF Incubating | 阿里云开源 |
| **Fluid** | 数据集缓存 | CNCF Incubating | 阿里云开源 |
| **OpenKruise** | 高级工作负载 | CNCF Incubating | 阿里云开源 |
| **Koordinator** | QoS 调度与混部 | 开源 | 阿里云开源 |
| **KubeSphere** | 多集群管理平台 | 开源 | 青云/社区 |

### 腾讯云

| 项目/服务 | 作用 | 开源性 | 备注 |
|:---|:---|:---|:---|
| **TKE** | 托管 K8s | 商业 | 腾讯云容器服务 |
| **TKEStack** | 开源容器平台 | 开源 | 基于 K8s |
| **SuperEdge** | 边缘容器 | 开源 | 腾讯开源 |

### 火山引擎 (字节跳动)

| 项目/服务 | 作用 | 开源性 | 备注 |
|:---|:---|:---|:---|
| **veStack** | 企业级 K8s | 商业 | 火山引擎 |
| **KubeWharf** | K8s 增强套件 | 开源 | 字节跳动开源 |

---

## 参考链接

- [AWS EKS 文档](https://docs.aws.amazon.com/eks/)
- [GKE 文档](https://cloud.google.com/kubernetes-engine/docs)
- [Azure AKS 文档](https://docs.microsoft.com/azure/aks/)
- [阿里云 ACK](https://www.aliyun.com/product/kubernetes)
- [腾讯云 TKE](https://cloud.tencent.com/product/tke)
