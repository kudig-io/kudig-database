---
title: AKS 集群生命周期与升级策略
description: '涵盖 AKS 集群创建、版本升级、Node Image Upgrade、Blue-Green 升级及 AKS Automatic 全生命周期管理'
summary: '涵盖 AKS 集群创建、版本升级、Node Image Upgrade、Blue-Green 升级及 AKS Automatic 全生命周期管理'
category: cloud-providers
tags:
- cloud
- k8s
- aks
- azure
- lifecycle
- upgrade
tier: supporting
created: '2026-07-02'
last_updated: 2026-07
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 平台工程师
estimated_read_time: 15min
intent_queries:
- AKS 集群生命周期管理是什么
- 如何升级 AKS 集群版本
- 如何执行 Blue-Green 升级
trigger_keywords:
- AKS
- cluster upgrade
- node image
- blue-green
- AKS Automatic
- Terraform
prerequisites:
- kubectl-basics
- cloud-basics
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
authors:
- name: KUDIG Team
  role: contributor
---

# AKS 集群生命周期与升级策略

## 1. 集群创建

### 1.1 Terraform 创建生产级集群

```hcl
resource "azurerm_resource_group" "aks" {
  name     = "rg-aks-prod-eastasia"
  location = "eastasia"
}

resource "azurerm_kubernetes_cluster" "main" {
  name                = "aks-prod-01"
  location            = azurerm_resource_group.aks.location
  resource_group_name = azurerm_resource_group.aks.name
  dns_prefix          = "aks-prod-01"
  kubernetes_version  = "1.30.3"
  node_resource_group = "MC_rg-aks-prod-eastasia_aks-prod-01_eastasia"

  # 系统节点池（必须）
  default_node_pool {
    name                = "system"
    vm_size             = "Standard_D4s_v5"
    node_count          = 3
    min_count           = 3
    max_count           = 10
    enable_auto_scaling = true
    os_disk_size_gb     = 128
    os_disk_type        = "Managed"
    max_pods            = 60
    vnet_subnet_id      = azurerm_subnet.aks.id
    zones               = ["1", "2", "3"]

    upgrade_settings {
      max_surge                     = "33%"
      drain_timeout_in_minutes      = 30
      node_soak_duration_in_minutes = 0
    }

    node_labels = {
      "nodepool-type" = "system"
      "environment"   = "production"
    }
  }

  # 身份认证
  identity {
    type = "SystemAssigned"
  }

  # 网络配置
  network_profile {
    network_plugin    = "azure"
    network_policy    = "cilium"
    load_balancer_sku = "standard"
    service_cidr      = "10.0.0.0/16"
    dns_service_ip    = "10.0.0.10"
    outbound_type     = "managedNATGateway"

    nat_gateway_profile {
      managed_outbound_ip_count = 2
      idle_timeout_in_minutes   = 4
    }
  }

  # Azure AD 集成
  azure_active_directory_role_based_access_control {
    managed                = true
    azure_rbac_enabled     = true
    admin_group_object_ids = [var.aks_admin_group_id]
  }

  # 监控
  oms_agent {
    log_analytics_workspace_id = azurerm_log_analytics_workspace.aks.id
  }

  # 密钥管理
  key_vault_secrets_provider {
    secret_rotation_enabled = true
  }

  # 维护窗口
  maintenance_window {
    allowed {
      day   = "Sunday"
      hours = [2, 3, 4]
    }
  }

  auto_scaler_profile {
    balance_similar_node_groups      = true
    expander                         = "random"
    max_graceful_termination_sec     = 600
    scale_down_delay_after_add       = "10m"
    scale_down_unneeded              = "10m"
    scale_down_utilization_threshold = "0.5"
  }

  tags = {
    environment = "production"
    managed-by  = "terraform"
  }
}

# 用户节点池
resource "azurerm_kubernetes_cluster_node_pool" "worker" {
  name                  = "worker"
  kubernetes_cluster_id = azurerm_kubernetes_cluster.main.id
  vm_size               = "Standard_D8s_v5"
  node_count            = 5
  min_count             = 5
  max_count             = 50
  enable_auto_scaling   = true
  os_disk_size_gb       = 256
  vnet_subnet_id        = azurerm_subnet.aks.id
  zones                 = ["1", "2", "3"]
  mode                  = "User"
  max_pods              = 60

  node_labels = {
    "nodepool-type" = "worker"
    "workload"      = "general"
  }

  node_taints = []

  upgrade_settings {
    max_surge                     = "50%"
    drain_timeout_in_minutes      = 30
    node_soak_duration_in_minutes = 5
  }
}
```

### 1.2 Azure CLI 快速创建

```bash
# 创建资源组
az group create --name rg-aks-prod --location eastasia

# 创建 VNet 和子网
az network vnet create \
  --resource-group rg-aks-prod \
  --name vnet-aks-prod \
  --address-prefix 10.240.0.0/16 \
  --subnet-name aks-subnet \
  --subnet-prefix 10.240.0.0/20

# 创建 AKS 集群
az aks create \
  --resource-group rg-aks-prod \
  --name aks-prod-01 \
  --kubernetes-version 1.30.3 \
  --node-count 3 \
  --node-vm-size Standard_D4s_v5 \
  --network-plugin azure \
  --network-policy cilium \
  --load-balancer-sku standard \
  --enable-managed-identity \
  --enable-addons monitoring \
  --enable-msi-auth-for-monitoring \
  --enable-azure-rbac \
  --enable-aad \
  --aad-admin-group-object-ids <group-id> \
  --zones 1 2 3 \
  --auto-scaler-profile balance-similar-node-groups=true \
  --ssh-key-value ~/.ssh/id_rsa.pub

# 添加用户节点池
az aks nodepool add \
  --resource-group rg-aks-prod \
  --cluster-name aks-prod-01 \
  --name worker \
  --node-count 5 \
  --min-count 5 \
  --max-count 50 \
  --enable-cluster-autoscaler \
  --node-vm-size Standard_D8s_v5 \
  --zones 1 2 3 \
  --mode User \
  --max-pods 60
```

## 2. Kubernetes 版本升级策略

### 2.1 版本支持矩阵

| 支持类型 | 说明 | 时间窗口 |
|---------|------|---------|
| **GA 版本** | Azure 官方支持的稳定版本 | 发布后 ~12 个月 |
| **预览版本** | 最新功能，不建议生产使用 | 可能随时变化 |
| **停止支持** | 已过期版本，需尽快升级 | 30 天宽限期 |

### 2.2 控制面升级

```bash
# 查看可用版本
az aks get-upgrades \
  --resource-group rg-aks-prod \
  --name aks-prod-01 \
  --output table

# 升级控制面（不含节点池）
az aks upgrade \
  --resource-group rg-aks-prod \
  --name aks-prod-01 \
  --kubernetes-version 1.31.0 \
  --control-plane-only

# 验证控制面版本
az aks show \
  --resource-group rg-aks-prod \
  --name aks-prod-01 \
  --query kubernetesVersion \
  --output tsv
```

### 2.3 节点池逐级升级

```bash
# 先升级系统节点池
az aks nodepool upgrade \
  --resource-group rg-aks-prod \
  --cluster-name aks-prod-01 \
  --name system \
  --kubernetes-version 1.31.0 \
  --max-surge 33%

# 再升级用户节点池
az aks nodepool upgrade \
  --resource-group rg-aks-prod \
  --cluster-name aks-prod-01 \
  --name worker \
  --kubernetes-version 1.31.0 \
  --max-surge 50%
```

### 2.4 滚动升级参数调优

```bash
# 设置升级策略
az aks nodepool update \
  --resource-group rg-aks-prod \
  --cluster-name aks-prod-01 \
  --name worker \
  --max-surge 50% \
  --drain-timeout 30 \
  --node-soak-duration 5

# max-surge 说明：
# - 百分比：按节点池大小计算额外节点数
# - 绝对值：如 "5" 表示最多额外 5 个节点
# - 生产建议：33%-50%
```

## 3. Node Image Upgrade

### 3.1 概述

Node Image Upgrade 只更新 OS 和运行时镜像，不变更 K8s 版本。适用于安全补丁和 OS 修复。

```bash
# 查看当前节点镜像版本
az aks nodepool show \
  --resource-group rg-aks-prod \
  --cluster-name aks-prod-01 \
  --name system \
  --query nodeImageVersion

# 触发节点镜像升级
az aks nodepool upgrade \
  --resource-group rg-aks-prod \
  --cluster-name aks-prod-01 \
  --name system \
  --node-image-only

# 配置自动节点镜像升级
az aks nodepool update \
  --resource-group rg-aks-prod \
  --cluster-name aks-prod-01 \
  --name system \
  --upgrade-channel NodeImage
```

### 3.2 自动升级通道

| 通道 | 行为 | 适用场景 |
|------|------|---------|
| `none` | 不自动升级 | 完全手动控制 |
| `patch` | 仅补丁版本 | 保守生产环境 |
| `stable` | 推荐的稳定版本 | 大多数生产环境 |
| `rapid` | 最新 GA 版本 | 测试/预发布环境 |
| `node-image` | 仅 OS 镜像 | K8s 版本不变 |

```bash
# 设置节点池自动升级通道
az aks nodepool update \
  --resource-group rg-aks-prod \
  --cluster-name aks-prod-01 \
  --name worker \
  --upgrade-channel stable

# 设置集群级自动升级（K8s 版本）
az aks update \
  --resource-group rg-aks-prod \
  --name aks-prod-01 \
  --auto-upgrade-channel stable
```

## 4. Blue-Green 升级策略

### 4.1 架构设计

```
Blue-Green 升级流程：

阶段 1: 准备 Green 环境
  ├── 创建新集群（Green）或新节点池
  ├── 部署相同工作负载
  └── 验证健康检查通过

阶段 2: 流量切换
  ├── 更新 DNS / Traffic Manager / Ingress 权重
  ├── 逐步将流量从 Blue 切到 Green
  └── 监控错误率和延迟

阶段 3: 清理 Blue 环境
  ├── 确认 Green 稳定运行（观察窗口 ≥ 24h）
  ├── 保留 Blue 环境作为回滚点
  └── 定期清理旧环境
```

### 4.2 节点池级 Blue-Green（推荐）

```bash
# 1. 创建新的节点池（Green）
az aks nodepool add \
  --resource-group rg-aks-prod \
  --cluster-name aks-prod-01 \
  --name worker-green \
  --kubernetes-version 1.31.0 \
  --node-count 5 \
  --enable-cluster-autoscaler \
  --min-count 5 \
  --max-count 50 \
  --node-vm-size Standard_D8s_v5 \
  --zones 1 2 3 \
  --mode User \
  --labels pool=green

# 2. 驱逐 Blue 节点池上的 Pod
kubectl cordon aks-worker-<blue-suffix> -l agentpool=worker
kubectl drain aks-worker-<blue-suffix> \
  --ignore-daemonsets \
  --delete-emptydir-data \
  --grace-period=120 \
  --timeout=300s

# 3. 验证新节点池工作正常
kubectl get nodes -l pool=green
kubectl get pods -o wide | grep green

# 4. 删除旧节点池
az aks nodepool delete \
  --resource-group rg-aks-prod \
  --cluster-name aks-prod-01 \
  --name worker
```

### 4.3 集群级 Blue-Green

```bash
# 使用 Terraform 管理双集群
# blue 集群：aks-prod-blue
# green 集群：aks-prod-green

# 流量切换使用 Azure Traffic Manager
az network traffic-manager endpoint update \
  --resource-group rg-aks-prod \
  --profile-name tm-aks-prod \
  --name green-endpoint \
  --type azureEndpoints \
  --target-resource-id $GREEN_CLUSTER_LB_ID \
  --priority 1

# 降级 Blue 优先级
az network traffic-manager endpoint update \
  --resource-group rg-aks-prod \
  --profile-name tm-aks-prod \
  --name blue-endpoint \
  --type azureEndpoints \
  --target-resource-id $BLUE_CLUSTER_LB_ID \
  --priority 2
```

## 5. AKS Automatic

### 5.1 概述

AKS Automatic 是 Azure 的全托管 Kubernetes 体验，自动处理节点管理、升级、缩放等操作。

```bash
# 创建 AKS Automatic 集群
az aks create \
  --resource-group rg-aks-auto \
  --name aks-auto-01 \
  --sku automatic \
  --kubernetes-version 1.31.0 \
  --network-plugin azure \
  --network-plugin-mode overlay \
  --network-policy cilium \
  --enable-msi-auth-for-monitoring \
  --enable-azure-rbac
```

### 5.2 AKS Automatic 特性

| 特性 | 说明 |
|------|------|
| 自动节点管理 | 节点选择、配置、升级全自动 |
| 自动缩放 | 基于工作负载实时扩缩 |
| Node Autoprovisioning | 根据 Pod 需求自动创建最优节点池 |
| 安全默认值 | 默认启用安全基线配置 |
| 自动升级 | K8s 版本和节点镜像自动更新 |

### 5.3 Node Autoprovisioning (NAP)

```yaml
# AKS Automatic 使用 Karpenter (NAP) 替代传统 Cluster Autoscaler
# Pod 上通过 nodeSelector 或 affinity 指定需求
apiVersion: apps/v1
kind: Deployment
metadata:
  name: gpu-workload
spec:
  template:
    spec:
      nodeSelector:
        node.kubernetes.io/instance-type: Standard_NC6s_v3
      tolerations:
      - key: "sku"
        operator: "Equal"
        value: "gpu"
        effect: "NoSchedule"
      containers:
      - name: inference
        resources:
          requests:
            nvidia.com/gpu: "1"
```

## 6. 维护窗口与发布控制

### 6.1 配置维护窗口

```bash
# 设置维护窗口（每周日凌晨 2-6 点 UTC）
az aks maintenanceconfiguration add \
  --resource-group rg-aks-prod \
  --cluster-name aks-prod-01 \
  --name weeklyMaintenance \
  --schedule-type Weekly \
  --day-of-week Sunday \
  --start-hour 2 \
  --duration 4

# 排除特定日期
az aks maintenanceconfiguration add \
  --resource-group rg-aks-prod \
  --cluster-name aks-prod-01 \
  --name not-during-holidays \
  --schedule-type Weekly \
  --day-of-week Sunday \
  --start-hour 2 \
  --duration 4 \
  --interval-weeks 1
```

### 6.2 发布排期策略

```
升级排期建议（生产环境）：

Week 1: 测试集群升级
  - 升级 dev/staging 环境
  - 运行全量回归测试
  - 观察 72 小时

Week 2: 生产灰度
  - 升级系统节点池
  - 升级 1 个用户节点池（20% 流量）
  - 监控 48 小时

Week 3: 全量生产
  - 升级剩余节点池
  - 验证所有工作负载
  - 更新 CI/CD 中的 K8s 版本引用

Week 4: 清理
  - 移除旧版本兼容性代码
  - 更新文档和 Runbook
```

## 7. 监控与告警

```bash
# 创建升级相关告警
az monitor metrics alert create \
  --name aks-upgrade-failure \
  --resource-group rg-aks-prod \
  --scopes /subscriptions/{sub}/resourceGroups/rg-aks-prod/providers/Microsoft.ContainerService/managedClusters/aks-prod-01 \
  --condition "count nodepool_upgrade_count > 0 and nodepool_upgrade_status == 'Failed'" \
  --window-size 5m \
  --evaluation-frequency 1m \
  --severity 1 \
  --action-group ag-sre-oncall

# Prometheus 查询升级指标
# kube_node_status_condition{condition="Ready",status="true"}
# kube_node_info{node_image_version}
```

## Related

- [[03-aks-networking-azure-cni|AKS 网络与 Azure CNI]]
- [[06-aks-troubleshooting-playbook|AKS 故障排查手册]]

## See Also

- Azure AKS 官方文档：版本支持策略
- AKS 发布跟踪器
