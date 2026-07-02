---
title: Azure AKS 生产环境运行手册
description: 面向 Azure Kubernetes Service（AKS）的生产运维运行手册，覆盖托管身份、Azure CNI、集群升级、节点池、备份/容灾、Azure Monitor、成本治理与故障排查。
summary: 面向 AKS 的生产运维运行手册，覆盖 AKS 身份、网络、升级、备份/容灾、监控、成本与故障排查。
category: cloud-providers
tags:
- production
- best-practices
- playbook
- cloud-providers
- azure
- aks
- managed-identity
- azure-cni
tier: core
created: '2026-07-01'
last_updated: '2026-07'
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 平台工程师
estimated_read_time: 30min
intent_queries:
- AKS 生产环境如何运维
- AKS Managed Identity 与 Workload Identity 配置
- Azure CNI 网络模式选择
- AKS 升级与备份最佳实践
trigger_keywords:
- AKS
- Azure Kubernetes Service
- Managed Identity
- Workload Identity
- Azure CNI
- Azure Monitor
prerequisites:
- kubectl-basics
- azure-cli
- aks-networking-basics
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
- '1.33'
authors:
- name: KUDIG Team
  role: contributor
---

# Azure AKS 生产环境运行手册

本手册面向在 Microsoft Azure 上运行 AKS 的 SRE 与平台工程师，聚焦 AKS 托管身份、Azure CNI 网络、升级、节点池、备份/容灾、Azure Monitor、成本治理与常见故障排查。AKS 作为托管 Kubernetes 服务，虽然接管了控制面的可用性、升级与部分安全基线，但网络架构设计、身份与访问管理、节点生命周期、可观测性配置以及成本控制仍然是运维团队的核心职责。手册中的命令可直接在配置了 Azure CLI 与 kubectl 的环境中执行，所有变更操作应在非生产环境验证后再应用到生产集群。通过遵循本手册，团队可以将 AKS 特定操作纳入组织统一的 [[domain-11-production-operations/99-production-readiness-operations-guide.md|生产就绪运维框架]]，提升运维一致性与故障响应效率，同时降低因配置不当导致的安全风险与成本失控。

## 1. 适用场景与范围

本手册适用于以下场景：

- 新建或接管 AKS 生产集群，需要制定运维基线、变更流程与回滚预案。
- 执行控制面/节点升级、节点池扩缩容、网络配置变更、安全加固等操作。
- 建立基于 Azure Monitor、Container Insights、Prometheus 与 Grafana 的可观测性体系。
- 排查 AKS 相关的身份鉴权失败、网络连通异常、节点 NotReady、存储挂载失败等问题。
- 理解 AKS 托管特性与 Azure 平台服务（如 Key Vault、Managed Identity、Azure AD、Azure DNS、Azure Load Balancer）的集成方式。
- 需要将 AKS 成本治理、配额管理与 FinOps 实践结合起来。

## 2. 前置条件与工具

```bash
# 安装 Azure CLI
curl -sL https://aka.ms/InstallAzureCLIDeb | sudo bash
az login
az account set --subscription <SUBSCRIPTION_ID>

# 安装 aks-preview（如需预览功能）
az extension add --name aks-preview

# 获取集群凭证
az aks get-credentials --resource-group <RG> --name <CLUSTER_NAME> --overwrite-existing

# 验证
kubectl version
az aks show --resource-group <RG> --name <CLUSTER_NAME> --query provisioningState
```

生产环境建议为 SRE 团队配置最小权限 RBAC 角色，例如 `Azure Kubernetes Service Cluster User Role`、`Contributor`（限资源组范围），避免使用 Subscription 级别的 Owner。所有生产变更应通过 Terraform、ARM/Bicep 或 GitOps 工作流管理，禁止在 Azure Portal 进行未记录的手动修改。变更窗口、回滚方案与影响范围应在变更管理工具中登记，确保事后可审计。同时，应为不同的环境（开发、测试、生产）使用独立的订阅或资源组，并通过 Azure Policy 强制实施命名规范与标签策略。

## 3. 核心概念与架构

### 3.1 托管身份（Managed Identity）

AKS 控制面默认使用 **System-Assigned Managed Identity**。工作负载可通过以下两种方式访问 Azure 资源：

- **AKS Managed Identity（控制面/节点）**：用于 AKS 自身访问 Azure 资源（如负载均衡器、托管磁盘、NSG、路由表）。该身份由 AKS 自动管理，用户无需手动轮换凭据。
- **Azure AD Workload Identity**：Pod 内 ServiceAccount 通过 OIDC 联邦映射到 Azure AD 应用/托管身份，无需在 Pod 中存放任何密钥。这是生产环境推荐的工作负载身份方案。

生产建议：工作负载统一使用 Azure AD Workload Identity，禁止在 Secret 中存放 Service Principal 密码。对于遗留应用，应制定明确的迁移计划，逐步淘汰 Service Principal 并将密钥替换为 Managed Identity 联邦。迁移过程中应审计所有 `imagePullSecrets` 与自定义 Secret，确保无硬编码凭据。

### 3.2 Azure CNI 网络模式

| 模式 | 特点 | 适用场景 |
|---|---|---|
| Azure CNI Overlay | Pod 使用 overlay 网段，不消耗 VNet IP，支持大规模集群 | 大规模集群、IP 受限 |
| Azure CNI（动态 IP）| Pod 直接从 VNet 子网获取 IP，支持 Pod 级网络安全组 | 需要 Pod 与 VNet 资源直接通信 |
| kubenet | 基本网络，性能与功能有限，不支持 Azure Network Policy | 不推荐生产使用 |

生产建议：优先使用 Azure CNI Overlay 或动态 IP，结合 Network Policy（Calico 或 Cilium）实现微分段。如果 Pod 需要直接通过 VNet IP 被其他 Azure 服务访问，或需要使用 Azure Network Policy Manager，应选择 Azure CNI 动态 IP 模式。无论选择哪种模式，都应提前规划 Pod CIDR、Service CIDR 与 VNet 子网，避免后期因 IP 耗尽导致无法扩容。

### 3.3 IP 地址规划

IP 地址规划是 AKS 生产部署中最容易忽视但影响最大的问题之一。规划不足会导致 Pod 无法调度、Service 无法创建或节点池无法扩容。建议遵循以下原则：

- **VNet 子网**：根据节点数量与预留扩容空间选择掩码，例如 `/24` 支持约 250 个节点。如果计划使用多个节点池或未来扩容，建议预留更大空间。
- **Pod CIDR**：Overlay 模式下可独立规划，建议使用 `/16` 以满足大规模 Pod 需求。每个节点默认可分配 250 个 Pod IP，因此 Pod CIDR 大小直接决定集群最大节点数。
- **Service CIDR**：用于 Kubernetes Service ClusterIP，需与 VNet 不重叠，通常 `/16` 足够。Service IP 一旦分配后难以更改，应预留足够空间。
- **Docker Bridge CIDR**：仅在需要时配置，避免与现有网络冲突。大多数场景可使用默认值。

### 3.4 节点池设计

- 使用 **system node pool** 承载 kube-system、monitoring、ingress 等核心组件，并设置 `CriticalAddonsOnly=true:NoSchedule` 污点，防止业务 Pod 抢占核心组件资源。
- 按业务/环境划分 **user node pools**，并设置标签、污点、VM 大小与 OS 磁盘类型。例如 `general`、`spot`、`gpu`、`memory-optimized` 等。
- 关键工作负载使用 **Availability Zones** 部署，避免单点故障。节点池应跨至少两个可用区。
- 使用 Ephemeral OS disk 提升节点性能并降低存储成本，但需注意 OS 盘数据不会持久化，仅适合容器镜像与临时文件。

## 4. 标准操作流程

### 4.1 创建生产集群

```bash
az group create --name prod-aks-rg --location eastasia

az aks create \
  --resource-group prod-aks-rg \
  --name prod-aks \
  --location eastasia \
  --zones 1 2 3 \
  --node-count 3 \
  --node-vm-size Standard_D4s_v5 \
  --node-osdisk-size 128 \
  --enable-cluster-autoscaler \
  --min-count 3 \
  --max-count 10 \
  --network-plugin azure \
  --network-plugin-mode overlay \
  --pod-cidr 10.244.0.0/16 \
  --service-cidr 10.0.0.0/16 \
  --dns-service-ip 10.0.0.10 \
  --enable-managed-identity \
  --enable-workload-identity \
  --enable-oidc-issuer \
  --enable-addons monitoring \
  --generate-ssh-keys
```

创建后应立即配置授权 IP 范围（authorized IP ranges）或私有集群（private cluster），限制控制面访问源。授权 IP 范围适用于控制面仍有公共 endpoint 但需要限制访问源的场景；私有集群则完全关闭公共 endpoint，仅通过私有网络访问。同时启用维护窗口（maintenance window），避免 Azure 自动执行的补丁与升级影响业务高峰。对于生产环境，建议购买 AKS Uptime SLA，以获得更高的控制面可用性保障。

### 4.2 配置 Azure AD Workload Identity

Azure AD Workload Identity 的完整配置涉及 Azure AD Managed Identity、Federated Identity Credentials、K8s ServiceAccount 注解以及 Namespace 标签。任何一步配置错误都会导致 Pod 无法获取 Azure AD 访问令牌。

```bash
# 1. 创建 Managed Identity
az identity create --name aks-app-mi --resource-group prod-aks-rg
export MANAGED_IDENTITY_CLIENT_ID=$(az identity show --name aks-app-mi --resource-group prod-aks-rg --query clientId -o tsv)

# 2. 为 Identity 授权（示例：Key Vault 读取）
az role assignment create \
  --assignee $MANAGED_IDENTITY_CLIENT_ID \
  --role "Key Vault Secrets User" \
  --scope /subscriptions/<SUB>/resourceGroups/prod-aks-rg/providers/Microsoft.KeyVault/vaults/<VAULT_NAME>

# 3. 创建 K8s ServiceAccount 并添加注解
kubectl create serviceaccount app-sa -n prod
kubectl annotate serviceaccount app-sa -n prod \
  azure.workload.identity/client-id=$MANAGED_IDENTITY_CLIENT_ID

# 4. 为 Namespace 开启 Workload Identity
kubectl label namespace prod azure.workload.identity/use=true

# 5. 创建 Federated Identity Credential
az identity federated-credential create \
  --name prod-federated-credential \
  --identity-name aks-app-mi \
  --resource-group prod-aks-rg \
  --issuer $(az aks show -n prod-aks -g prod-aks-rg --query "oidcIssuerProfile.issuerUrl" -o tsv) \
  --subject system:serviceaccount:prod:app-sa \
  --audiences api://AzureADTokenExchange
```

验证：在 Pod 中执行 `az login --identity` 或访问目标 Azure 资源，应成功获取令牌。如果失败，请按以下顺序排查：检查 Pod 是否使用了正确的 ServiceAccount、ServiceAccount 注解是否正确、Namespace 是否带有 `azure.workload.identity/use=true` 标签、Federated Identity Credential 的 issuer 与 subject 是否与集群 OIDC issuer 和 K8s SA 完全匹配、Managed Identity 是否被赋予了所需 Azure RBAC 角色。

### 4.3 升级 AKS

AKS 控制面升级不可逆，必须先在 staging 验证应用兼容性。节点升级会导致节点重建，因此需要确保工作负载能够平滑迁移。

```bash
# 查看可用版本
az aks get-upgrades --resource-group prod-aks-rg --name prod-aks --output table

# 升级控制面
az aks upgrade \
  --resource-group prod-aks-rg \
  --name prod-aks \
  --kubernetes-version 1.30.0 \
  --control-plane-only

# 升级所有节点池
az aks nodepool upgrade \
  --cluster-name prod-aks \
  --resource-group prod-aks-rg \
  --name nodepool1 \
  --kubernetes-version 1.30.0

# 验证
kubectl get nodes -o wide
```

升级前确认：
- 已阅读目标版本的发布说明与已知问题。
- 关键工作负载配置 PodDisruptionBudget。
- 业务低峰期执行，变更窗口已通知。
- 已准备回滚方案（节点池可重建，控制面不可回滚）。
- 已检查目标版本中的弃用 API 与行为变更。

### 4.4 节点池扩缩容与自动缩放

```bash
# 手动扩缩容
az aks nodepool scale \
  --cluster-name prod-aks \
  --resource-group prod-aks-rg \
  --name gpupool \
  --node-count 2

# 启用 cluster autoscaler
az aks nodepool update \
  --cluster-name prod-aks \
  --resource-group prod-aks-rg \
  --name nodepool1 \
  --enable-cluster-autoscaler \
  --min-count 3 \
  --max-count 20
```

对于 KEDA 驱动的应用，可以结合 HPA 与 cluster autoscaler 实现事件驱动的弹性伸缩。配置 autoscaler 时，应根据业务峰值合理设置 `max-count`，防止因异常流量或配置错误导致成本失控。同时，应为不同工作负载使用不同节点池，避免批处理任务抢占在线服务资源。

### 4.5 备份与容灾

```bash
# 启用 Azure Backup for AKS（需先注册 Provider）
az provider register --namespace Microsoft.DataProtection

# 使用 Velero 作为跨云/灵活备份方案
velero install \
  --provider azure \
  --plugins velero/velero-plugin-for-microsoft-azure:v1.10.0 \
  --bucket <BACKUP_CONTAINER> \
  --secret-file ./credentials-velero \
  --backup-location-config resourceGroup=prod-aks-rg,storageAccount=<SA_NAME> \
  --snapshot-location-config apiTimeout=5m,resourceGroup=prod-aks-rg

velero backup create prod-daily --include-namespaces prod,monitoring
```

建议每月执行一次恢复演练，验证命名空间级恢复与 PVC 快照恢复。对于关键数据库，还需执行应用级备份验证。备份数据应存放在与生产集群不同的区域或订阅中，并启用版本控制与加密，防止单区域故障导致备份不可用。

### 4.6 Azure Monitor 与 Container Insights

```bash
# 启用 Container Insights（如创建时未启用）
az aks enable-addons \
  --resource-group prod-aks-rg \
  --name prod-aks \
  --addons monitoring \
  --workspace-resource-id /subscriptions/<SUB>/resourceGroups/<RG>/providers/Microsoft.OperationalInsights/workspaces/<WS_NAME>

# 关键告警（Azure Monitor Alert Rules）
az monitor metrics alert create \
  --name "AKS Node Not Ready" \
  --resource-group prod-aks-rg \
  --scopes /subscriptions/<SUB>/resourceGroups/prod-aks-rg/providers/Microsoft.ContainerService/managedClusters/prod-aks \
  --condition "avg kube_node_status_condition > 0" \
  --evaluation-frequency 1m \
  --window-size 5m \
  --severity 2
```

建议同时部署 Prometheus/Grafana 或 Azure Managed Prometheus 收集自定义业务指标，并将告警统一路由到 PagerDuty/OpsGenie。Container Insights 提供了容器日志、性能指标与集群健康状况的统一视图，是 AKS 生产监控的基础组件。应为其配置合理的日志保留策略与成本预算，避免日志量突增导致费用失控。

### 4.7 网络策略示例

```yaml
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: default-deny-ingress
  namespace: prod
spec:
  podSelector: {}
  policyTypes:
  - Ingress
---
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: allow-frontend-to-backend
  namespace: prod
spec:
  podSelector:
    matchLabels:
      app: backend
  ingress:
  - from:
    - podSelector:
        matchLabels:
          app: frontend
    ports:
    - protocol: TCP
      port: 8080
```

### 4.8 成本治理

```bash
# 使用 Spot 节点池运行批处理/可中断工作负载
az aks nodepool add \
  --cluster-name prod-aks \
  --resource-group prod-aks-rg \
  --name spotpool \
  --priority Spot \
  --eviction-policy Delete \
  --node-vm-size Standard_D4s_v5 \
  --node-count 0 \
  --enable-cluster-autoscaler \
  --min-count 0 \
  --max-count 50 \
  --spot-max-price -1
```

成本优化建议：
- 为节点与命名空间添加 team/env/cost-center 标签。
- 设置 Azure Budget 与 Cost Alert。
- 定期审查闲置 Public IP、LoadBalancer、Disk 与 Snapshot。
- 对开发/测试环境使用 Spot 与 B 系列 VM。
- 分析 Azure Cost Management 数据，识别长期低利用率节点池。
- 使用预留实例（Reserved Instances）或节省计划（Savings Plans）降低长期稳定工作负载成本。

## 5. 关键检查点与验证命令

| 检查项 | 命令 | 通过标准 |
|---|---|---|
| 集群状态 | `az aks show -g prod-aks-rg -n prod-aks --query provisioningState` | Succeeded |
| 节点健康 | `kubectl get nodes -o wide` | 所有节点 Ready |
| Workload Identity | Pod 内 `az login --identity` 或访问目标资源 | 成功获取令牌/访问资源 |
| 网络策略 | `kubectl get networkpolicies -A` | 核心命名空间存在默认拒绝策略 |
| 备份状态 | `velero backup get` | 最近 24h 有 Completed 备份 |
| 证书/Secret | `kubectl get certificates -A` | 无 Expired/Failed |
| 成本标签 | `kubectl get nodes --show-labels` | 节点带有 team/env/cost-center 标签 |
| Container Insights | `az aks show -g prod-aks-rg -n prod-aks --query addonProfiles.omsagent.enabled` | true |

## 6. 常见故障与 remediation

| 现象 | 根因 | 处理命令/步骤 |
|---|---|---|
| Pod 持续 Pending | 资源不足、污点/亲和性、Spot 驱逐 | `kubectl describe pod <pod> -n <ns>`；检查节点池余量与 taint/toleration |
| Workload Identity 失败 | federation 配置错误、SA 注解缺失 | 检查 OIDC issuer URL、SA 注解、Azure AD app federation |
| 节点 NotReady | 磁盘压力、kubelet 异常、网络中断 | `kubectl describe node <node>`；SSH 到节点查看 `journalctl -u kubelet` |
| 服务访问不通 | Azure Load Balancer 健康探测失败、NSG 阻断 | 检查 Service `type=LoadBalancer` 注解、NSG 规则、后端池健康状态 |
| PVC 挂载失败 | 磁盘类型/可用区不匹配、CSI 异常 | `kubectl describe pvc <pvc> -n <ns>`；查看 csi-azuredisk-node 日志 |
| 升级失败 | PDB 阻塞 drain、节点镜像拉取失败 | `kubectl get pdb -A`；查看节点事件与镜像仓库连通性 |
| 成本突增 | 自动缩放上限过高、预留实例到期 | 分析 Cost Analysis 标签；调整 max-count 与 VM 大小 |
| 网络策略不生效 | CNI 与 Network Policy 实现不匹配 | 确认启用 Calico/Cilium；检查策略选择器与端口 |
| 应用无法访问 Azure PaaS | 出站 NSG 或路由限制 | 检查子网 NSG 规则与 UDR 路由 |

## 7. 风险与注意事项

1. **控制面升级不可逆**：AKS 控制面升级后无法回滚，先在非生产环境验证兼容性。
2. **节点池 OS 升级会重启节点**：使用 `az aks nodepool upgrade --node-image-only` 时，确保 PDB 配置合理。
3. **Azure AD Workload Identity 依赖 OIDC issuer**：删除/重建集群会改变 issuer URL，需同步更新 Azure AD 应用 federation。
4. **Private Cluster 访问受限**：建议使用 Azure Bastion、Jump Box 或已加入 VNet 的 CI/CD agent 访问。
5. **Network Policy 选择**：Calico 与 Azure Network Policy 各有特性差异，生产切换前需充分测试。
6. **备份需验证可恢复性**：定期演练 Velero 恢复，确认 StatefulSet、PVC 与 Secret 完整恢复。
7. **Spot 节点不适合有状态服务**：为关键 StatefulSet 配置反亲和性，避免同时被回收。
8. **IP 地址规划不足会导致无法调度**：定期监控 Pod/Service IP 使用率，提前扩容网络范围。

## 8. 相关 Runbook / 推荐阅读

- [[domain-11-production-operations/99-production-readiness-operations-guide.md|生产运维域生产就绪运维指南]]
- [[domain-12-cloud-providers/04-azure-aks/azure-aks-overview.md|AKS 概览]]
- [[domain-12-cloud-providers/04-azure-aks/02-aks-cluster-lifecycle-upgrades.md|AKS 集群生命周期与升级]]
- [[domain-12-cloud-providers/04-azure-aks/03-aks-networking-azure-cni.md|AKS Azure CNI 网络]]
- [[domain-12-cloud-providers/04-azure-aks/05-aks-identity-workload-identity.md|AKS 身份与 Workload Identity]]
- [[domain-12-cloud-providers/04-azure-aks/06-aks-troubleshooting-playbook.md|AKS 故障排查手册]]
- [[domain-05-security-compliance/README.md|安全合规域]]
- [[domain-06-observability/README.md|可观测性域]]
