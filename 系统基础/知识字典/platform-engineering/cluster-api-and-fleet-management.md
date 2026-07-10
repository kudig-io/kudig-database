---
title: Cluster API 与集群舰队管理
description: '# Cluster API 与集群舰队管理'
summary: '# Cluster API 与集群舰队管理'
category: dictionary
tags:
- k8s
- glossary
- terminology
- etcd
- kubelet
- istio
- cilium
- flux
- pdb
- rbac
tier: supporting
created: 2026-05
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Cluster API 与集群舰队管理 是什么
- 如何 Cluster API 与集群舰队管理
trigger_keywords:
- Cluster
- API
- 与集群舰队管理
- dictionary
prerequisites:
- kubectl-basics
- cloud-provider-basics
- service-mesh-basics
- iac-basics
- cilium-basics
- etcd-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Cluster API 与集群舰队管理

## 概述

随着企业 [[Kubernetes|Kubernetes]] 集群数量从个位数增长到数十甚至上百个，**集群舰队管理（Fleet Management）** 成为平台工程的核心挑战。**Cluster API（CAPI）** 是 Kubernetes 官方的声明式集群生命周期管理项目，它使用 Kubernetes 的 CRD 机制来创建、配置和管理其他 Kubernetes 集群，实现了"用 Kubernetes 管理 Kubernetes"的 Meta-Cluster 模式。

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
- **Cluster Mesh（[[Cilium|Cilium]]/Istio）**：跨集群的服务发现和流量管理

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
| 自动修复 | MachineHealthCheck 自动检测并替换问题节点 | 提升可用性 |
| 滚动升级 | 控制平面和工作节点支持无中断滚动升级 | 简化版本管理 |
| 多集群 GitOps | 通过 Argo [[entities/flux.md|Flux]] 将应用同步到舰队中的所有集群 | 一致性交付 |
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
3. **灾难恢复与集群重建**：区域级问题后，通过 Git 中存储的 Cluster API 定义在 30 分钟内重建新集群
4. **边缘计算舰队管理**：管理数千个零售门店或工厂边缘的轻量级 K8s 集群

## 最佳实践/注意事项

- **管理集群必须高可用**：管理集群是所有工作负载集群的"大脑"，必须配置多控制平面和 etcd 备份
- **Cluster API 版本兼容性**：确保管理集群的 CAPI 版本支持目标工作负载集群的 Kubernetes 版本
- **节点镜像预构建**：使用预置好容器镜像和 OS 补丁的 Golden Image，减少节点启动时间
- **Secret 管理**：云提供商的 API 凭证、SSH 密钥应使用外部 Secret 管理工具，不直接存储在 Git
- **网络规划统一**：舰队中的集群应使用不重叠的 Pod CIDR 和 Service CIDR，便于未来的 Cluster Mesh 互联
- **漂移检测与治理**：定期检查工作负载集群的实际配置是否与 Git/CAPI 定义一致
- **分批次升级策略**：控制平面升级应按 dev → staging → production 的顺序分批进行，避免全局问题

## 故障排查

| 症状 | 可能原因 | 排查命令/方法 |
|------|---------|-------------|
| Cluster 对象长时间处于 Provisioning | Infrastructure Provider 认证失败或配额不足 | `kubectl describe cluster <name>`；查看 provider 控制器日志 |
| Machine 创建失败 | AMI/镜像不可用或子网配置错误 | `kubectl get machines` 查看状态；`kubectl describe machine <name>` |
| MachineHealthCheck 持续触发替换 | 节点网络不稳定或 kubelet 配置问题 | `kubectl get machinehealthcheck`；降低 unhealthyConditions 的灵敏度 |
| 控制平面升级卡住 | etcd 健康检查失败或证书过期 | `kubectl get kubeadmcontrolplane <name> -o yaml` 检查 status |
| 工作负载集群无法访问 | kubeconfig Secret 未创建或网络不通 | `kubectl get secret <cluster>-kubeconfig`；检查 API endpoint 可达性 |
| 节点滚动升级导致服务中断 | PDB 未配置或 maxUnavailable 过大 | 确保工作负载配置了 PDB；调整 MachineDeployment 的 `maxUnavailable` |
| Provider 控制器 OOM | 管理大量集群时内存不足 | `kubectl top pod -n capi-system`；增加控制器资源限制 |
| 管理集群与工作负载集群版本不兼容 | CAPI 版本不支持目标 K8s 版本 | 查阅 CAPI 兼容性矩阵文档 |

## 生产检查清单

- [ ] 管理集群配置多控制平面和 etcd 备份（管理集群是所有集群的"大脑"）
- [ ] MachineHealthCheck 已为所有工作负载集群配置自动修复
- [ ] 节点使用预构建的 Golden Image（OS 补丁 + 容器镜像预拉取）
- [ ] 云 Provider API 凭证使用 External Secrets 管理，定期轮换
- [ ] 舰队中所有集群的 Pod CIDR / Service CIDR 不重叠（为 Cluster Mesh 做准备）
- [ ] 控制平面和工作节点升级策略按 dev → staging → production 分批执行
- [ ] MachineDeployment 配置了合理的 maxSurge/maxUnavailable
- [ ] 管理集群的 CAPI 控制器资源限制根据管理集群数量调优
- [ ] 集群配置漂移检测已启用（定期对比 Git 定义与实际状态）
- [ ] 灾难恢复计划已演练：管理集群不可用时的 pivot 和重建流程

## 命令快速参考

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl label/annotate`：改元数据可能影响选择器/控制器

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 查看所有管理的集群
kubectl get clusters -A

# 查看集群详情和状态
kubectl describe cluster <name>

# 查看所有 Machine 及其状态
kubectl get machines -A

# 查看 MachineDeployment（类似 Deployment）
kubectl get machinedeployment -A

# 查看控制平面状态
kubectl get kubeadmcontrolplane -A

# 获取工作负载集群的 kubeconfig
kubectl get secret <cluster-name>-kubeconfig -o jsonpath='{.data.value}' | base64 -d > kubeconfig.yaml

# 查看 MachineHealthCheck 状态
kubectl get machinehealthcheck -A

# 查看 CAPI 控制器日志
kubectl -n capi-system logs -l cluster.x-k8s.io/provider=cluster-api --tail=100

# 查看 Infrastructure Provider 控制器日志
kubectl -n capa-system logs -l cluster.x-k8s.io/provider=infrastructure-aws --tail=100

# 暂停集群协调（维护窗口）
kubectl annotate cluster <name> cluster.x-k8s.io/paused=""

# 恢复集群协调
kubectl annotate cluster <name> cluster.x-k8s.io/paused-
```
## 交叉引用

- [infrastructure-as-code-for-kubernetes.md](./infrastructure-as-code-for-kubernetes.md) — Terraform/Pulumi/Crossplane 与 CAPI 的分层 IaC
- [gitops-and-continuous-delivery.md](./gitops-and-continuous-delivery.md) — 多集群 GitOps 交付策略
- [operator-pattern.md](./operator-pattern.md) — CAPI 控制器的 Operator 模式
- [custom-resources.md](./custom-resources.md) — Cluster API 使用的 CRD 体系
- [compatibility-version-for-control-plane.md](./compatibility-version-for-control-plane.md) — 控制平面版本管理

## 参考链接

- [Cluster API Documentation](https://cluster-api.sigs.k8s.io/)
- [Rancher Multi-Cluster Management](https://ranchermanager.docs.rancher.com/)
- [Red Hat Advanced Cluster Management](https://access.redhat.com/documentation/en-us/red_hat_advanced_cluster_management_for_kubernetes/)
- [Google Anthos](https://cloud.google.com/anthos)
- [Ajeet Singh Raina - Top 5 Trends Shaping Kubernetes in 2026](https://www.ajeetraina.com/top-5-trends-shaping-kubernetes-in-2026/)

## Related
- [[生态参考/topic-index/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
