# Cluster API 与集群舰队管理

## 概述

随着企业 Kubernetes 集群数量从个位数增长到数十甚至上百个，**集群舰队管理（Fleet Management）** 成为平台工程的核心挑战。**Cluster API（CAPI）** 是 Kubernetes 官方的声明式集群生命周期管理项目，它使用 Kubernetes 的 CRD 机制来创建、配置和管理其他 Kubernetes 集群，实现了"用 Kubernetes 管理 Kubernetes"的 Meta-Cluster 模式。

## 核心概念/原理

### 1. Cluster API 架构

Cluster API 通过以下 CRD 抽象集群生命周期：
- **Cluster**：定义目标集群的整体配置（如 CNI、Control Plane 端点）
- **Machine / MachineDeployment**：定义工作节点的规格、数量和升级策略，类似 Deployment 管理 Pod
- **KubeadmControlPlane**：定义控制平面的配置和高可用拓扑
- **Infrastructure Provider**：对接底层 IaaS（AWS、Azure、GCP、vSphere、OpenStack 等）
- **Bootstrap Provider**：负责节点初始化（通常使用 kubeadm）

### 2. 管理集群与工作负载集群

- **管理集群（Management Cluster）**：运行 Cluster API 控制器的 Kubernetes 集群
- **工作负载集群（Workload Cluster）**：由 Cluster API 创建和管理的业务集群
- 一个管理集群可以管理数百个工作负载集群，实现统一的集群运维平面

### 3. 集群舰队管理（Fleet Management）

企业通常使用以下工具构建舰队管理能力：
- **Rancher**：全功能的多集群管理平台，提供 UI、RBAC、监控和应用商店
- **Red Hat Advanced Cluster Management（ACM）**：基于 OpenShift 的企业级舰队管理
- **Google Anthos**：多云/混合云统一控制平面
- **Azure Fleet Manager**：Azure 原生的集群舰队管理
- **Cluster Mesh（Cilium/Istio）**：跨集群的服务发现和流量管理

### 4. 声明式集群生命周期

```yaml
apiVersion: cluster.x-k8s.io/v1beta1
kind: Cluster
metadata:
  name: production-cluster
  namespace: default
spec:
  controlPlaneRef:
    apiVersion: controlplane.cluster.x-k8s.io/v1beta1
    kind: KubeadmControlPlane
    name: production-cp
  infrastructureRef:
    apiVersion: infrastructure.cluster.x-k8s.io/v1beta1
    kind: AWSCluster
    name: production-infra
```

通过修改上述 CRD，即可触发：
- 集群创建/删除
- 控制平面扩缩容
- 工作节点滚动升级
- 控制平面版本升级

## 关键机制或特性

| 能力 | 说明 | 价值 |
|------|------|------|
| 声明式集群创建 | 通过 YAML 定义即可在任意云上创建 K8s 集群 | 基础设施即代码 |
| 自动修复 | MachineHealthCheck 自动检测并替换故障节点 | 提升可用性 |
| 滚动升级 | 控制平面和工作节点支持无中断滚动升级 | 简化版本管理 |
| 多集群 GitOps | 通过 Argo CD/Flux 将应用同步到舰队中的所有集群 | 一致性交付 |
| 集群自动扩缩 | 根据负载自动调整工作节点数量 | 成本优化 |

### 集群分类策略

2026 年的最佳实践建议按用途和 SLA 对集群进行分类管理：
- **Hub 集群**：运行 Cluster API、GitOps、监控等管理平台
- **生产集群**：运行核心业务负载，高可用、严格变更控制
- **预发集群**：用于集成测试和发布验证
- **开发/实验集群**：供开发团队快速验证，使用 Spot 实例降低成本
- **边缘集群**：运行 K3s/MicroK8s，靠近数据源或用户

## 使用场景

1. **多云统一底座**：在 AWS、Azure、GCP 和私有数据中心使用统一的 Cluster API 模板创建集群
2. **大规模节点升级**： fleet 中 50 个集群的 1000+ 节点需要在 1 周内完成安全补丁升级
3. **灾难恢复与集群重建**：区域级故障后，通过 Git 中存储的 Cluster API 定义在 30 分钟内重建新集群
4. **边缘计算舰队管理**：管理数千个零售门店或工厂边缘的轻量级 K8s 集群

## 最佳实践/注意事项

- **管理集群必须高可用**：管理集群是所有工作负载集群的"大脑"，必须配置多控制平面和 etcd 备份
- **Cluster API 版本兼容性**：确保管理集群的 CAPI 版本支持目标工作负载集群的 Kubernetes 版本
- **节点镜像预构建**：使用预置好容器镜像和 OS 补丁的 Golden Image，减少节点启动时间
- **Secret 管理**：云提供商的 API 凭证、SSH 密钥应使用外部 Secret 管理工具，不直接存储在 Git
- **网络规划统一**：舰队中的集群应使用不重叠的 Pod CIDR 和 Service CIDR，便于未来的 Cluster Mesh 互联
- **漂移检测与治理**：定期检查工作负载集群的实际配置是否与 Git/CAPI 定义一致
- **分批次升级策略**：控制平面升级应按 dev → staging → production 的顺序分批进行，避免全局故障

## 参考链接

- [Cluster API Documentation](https://cluster-api.sigs.k8s.io/)
- [Rancher Multi-Cluster Management](https://ranchermanager.docs.rancher.com/)
- [Red Hat Advanced Cluster Management](https://access.redhat.com/documentation/en-us/red_hat_advanced_cluster_management_for_kubernetes/)
- [Google Anthos](https://cloud.google.com/anthos)
- [Ajeet Singh Raina - Top 5 Trends Shaping Kubernetes in 2026](https://www.ajeetraina.com/top-5-trends-shaping-kubernetes-in-2026/)
