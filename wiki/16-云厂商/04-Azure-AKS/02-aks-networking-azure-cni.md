---
title: AKS 网络与 Azure CNI 深度解析
description: 'Azure CNI 动态/静态 IP、kubenet、Cilium 数据面、Private Cluster、DNS、NAT Gateway 全面配置指南'
summary: 'Azure CNI 动态/静态 IP、kubenet、Cilium 数据面、Private Cluster、DNS、NAT Gateway 全面配置指南'
category: cloud-providers
tags:
- cloud
- k8s
- aks
- azure
- networking
- cni
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
- Azure CNI 是什么
- 如何配置 AKS 网络
- 如何部署 Private Cluster
trigger_keywords:
- Azure CNI
- kubenet
- Cilium
- Private Cluster
- NAT Gateway
- DNS
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

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。


# AKS 网络与 Azure CNI 深度解析

## 1. 网络模式对比

| 模式 | Pod IP 来源 | 性能 | IP 消耗 | 适用场景 |
|------|------------|------|---------|---------|
| **Azure CNI (Dynamic)** | 子网动态分配 | 最佳 | 中 | 大多数生产环境 |
| **Azure CNI (Static)** | 预分配 IP 空间 | 最佳 | 高 | 需要固定 IP |
| **Azure CNI Overlay** | 私有 CIDR 叠加 | 优 | 低 | 大规模集群 |
| **kubenet** | 节点 NAT | 良 | 低 | 小规模/成本敏感 |
| **Cilium (eBPF)** | 子网或 Overlay | 最佳 | 低 | 高级网络策略 |

## 2. Azure CNI 动态 IP（推荐）

### 2.1 配置

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 创建专用子网
az network vnet create \
  --resource-group rg-aks-net \
  --name vnet-aks-prod \
  --address-prefix 10.0.0.0/16 \
  --subnet-name aks-pod-subnet \
  --subnet-prefix 10.0.0.0/18

# 创建 AKS 集群（Azure CNI 动态模式）
az aks create \
  --resource-group rg-aks-net \
  --name aks-prod-01 \
  --network-plugin azure \
  --network-plugin-mode overlay \
  --vnet-subnet-id /subscriptions/{sub}/resourceGroups/rg-aks-net/providers/Microsoft.Network/virtualNetworks/vnet-aks-prod/subnets/aks-pod-subnet \
  --service-cidr 172.16.0.0/16 \
  --dns-service-ip 172.16.0.10 \
  --max-pods 60
```
### 2.2 IP 规划

```
地址空间规划示例（VNet: 10.0.0.0/16）：

子网划分：
  aks-node-subnet:  10.0.0.0/18    → 16,382 个节点 IP
  aks-pod-subnet:   10.0.64.0/18   → 16,382 个 Pod IP
  aks-svc-subnet:   10.0.128.0/20  → 4,094 个服务 IP
  aks-mgmt-subnet:  10.0.144.0/24  → 管理/跳板机
  aks-db-subnet:    10.0.148.0/22  → 数据库等 PaaS

Service CIDR（集群内部）:
  172.16.0.0/16     → 与 VNet 不重叠

IP 消耗计算：
  每节点 max-pods = 60
  节点池大小 = 50 节点
  总 Pod IP 需求 = 50 × 60 = 3,000
  预留 20% 缓冲 → 至少 /18 子网
```

### 2.3 子网委派

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# AKS 自动委派子网（无需手动操作）
# 但需确保子网足够大，且未被其他资源占用

# 查看子网委派状态
az network vnet subnet show \
  --resource-group rg-aks-net \
  --vnet-name vnet-aks-prod \
  --name aks-pod-subnet \
  --query delegations
```
## 3. Azure CNI Overlay 模式

### 3.1 概述

Overlay 模式下，Pod 使用独立的私有 CIDR（如 100.64.0.0/10），不消耗 VNet 子网 IP。节点 IP 仍来自 VNet 子网。

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 创建 Overlay 集群
az aks create \
  --resource-group rg-aks-overlay \
  --name aks-overlay-01 \
  --network-plugin azure \
  --network-plugin-mode overlay \
  --pod-cidr 100.64.0.0/10 \
  --service-cidr 172.16.0.0/16 \
  --dns-service-ip 172.16.0.10 \
  --vnet-subnet-id $SUBNET_ID
```
### 3.2 Overlay vs 动态 IP 选择

```
决策树：

需要 Pod 直接被 VNet 其他资源访问？
  ├── 是 → Azure CNI 动态 IP
  │         Pod IP 直接分配自子网
  │         VNet 内任何资源可直接访问 Pod
  │
  └── 否 → 是否有大量节点 (>100)？
            ├── 是 → Azure CNI Overlay
            │         Pod CIDR 独立，不消耗子网 IP
            │         通过节点 IP NAT 通信
            │
            └── 否 → kubenet 或 Dynamic IP 均可
```

## 4. kubenet 网络模式

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# kubenet 模式（节省 IP）
az aks create \
  --resource-group rg-aks-kubenet \
  --name aks-kubenet-01 \
  --network-plugin kubenet \
  --pod-cidr 10.244.0.0/16 \
  --service-cidr 10.0.0.0/16 \
  --dns-service-ip 10.0.0.10 \
  --vnet-subnet-id $SUBNET_ID

# kubenet 路由表由 AKS 自动管理
# Pod IP 通过 UDR 路由到节点
```
**kubenet 限制**：
- 不支持 NetworkPolicy（需 Calico 插件）
- Pod 间通信需经过节点 NAT
- 不适用于大规模集群（>500 Pod）

## 5. Cilium 数据面

### 5.1 启用 Cilium

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# AKS 原生支持 Cilium（需 K8s ≥ 1.29）
az aks create \
  --resource-group rg-aks-cilium \
  --name aks-cilium-01 \
  --network-plugin azure \
  --network-plugin-mode overlay \
  --network-policy cilium \
  --pod-cidr 100.64.0.0/10 \
  --service-cidr 172.16.0.0/16 \
  --dns-service-ip 172.16.0.10
```
### 5.2 Cilium NetworkPolicy 示例

```yaml
apiVersion: cilium.io/v2
kind: CiliumNetworkPolicy
metadata:
  name: api-gateway-policy
  namespace: production
spec:
  endpointSelector:
    matchLabels:
      app: api-gateway
  ingress:
  - fromEndpoints:
    - matchLabels:
        app: frontend
    toPorts:
    - ports:
      - port: "8080"
        protocol: TCP
      rules:
        http:
        - method: "GET"
          path: "/api/v1/.*"
        - method: "POST"
          path: "/api/v1/orders"
  egress:
  - toEndpoints:
    - matchLabels:
        app: order-service
    toPorts:
    - ports:
      - port: "9090"
```

### 5.3 Cilium 可观测性

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 启用 Hubble（Cilium 网络可观测性）
az aks update \
  --resource-group rg-aks-cilium \
  --name aks-cilium-01 \
  --enable-cilium-hubble

# 访问 Hubble UI
kubectl port-forward -n kube-system svc/hubble-ui 12000:80

# Hubble CLI 观测流量
hubble observe --namespace production --verdict DROPPED
hubble observe --from-pod production/api-gateway --to-pod production/order-service
```
## 6. Private Cluster

### 6.1 创建 Private Cluster

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
az aks create \
  --resource-group rg-aks-private \
  --name aks-private-01 \
  --private-dns-zone system \
  --enable-private-cluster \
  --api-server-authorized-ip-ranges "203.0.113.0/24,10.0.0.0/8" \
  --network-plugin azure \
  --vnet-subnet-id $SUBNET_ID
```
### 6.2 访问 Private API Server

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 方案 1: 通过 VPN/ExpressRoute 直接访问
# Private DNS 自动解析 api-server 到私有 IP

# 方案 2: 使用 Private Endpoints + 跳板机
az network private-endpoint create \
  --resource-group rg-aks-private \
  --name pe-aks-api \
  --vnet-name vnet-hub \
  --subnet jumpbox-subnet \
  --private-connection-resource-id $AKS_ID \
  --group-id management \
  --connection-name aks-api-connection

# 验证 API Server 解析
nslookup aks-private-01-xxxxxxxx.hcp.eastasia.azmk8s.io
# 应解析到 10.x.x.x 私有地址
```
### 6.3 DNS 配置

```yaml
# 使用自定义 Private DNS Zone
# Terraform 示例：
resource "azurerm_private_dns_zone" "aks" {
  name                = "privatelink.eastasia.azmk8s.io"
  resource_group_name = azurerm_resource_group.aks.name
}

resource "azurerm_private_dns_zone_virtual_network_link" "aks" {
  name                  = "aks-dns-link"
  private_dns_zone_name = azurerm_private_dns_zone.aks.name
  virtual_network_id    = azurerm_virtual_network.hub.id
  resource_group_name   = azurerm_resource_group.aks.name
}
```

## 7. DNS 配置

### 7.1 CoreDNS 自定义

```yaml
apiVersion: v1
kind: ConfigMap
metadata:
  name: coredns-custom
  namespace: kube-system
data:
  custom.server: |
    # 转发公司内部域名到内部 DNS
    corporate.internal:53 {
        forward . 10.0.0.53 10.0.0.54
    }
    # 转发特定域名到 Azure Private DNS
    privatelink.blob.core.windows.net:53 {
        forward . 168.63.129.16
    }
```

### 7.2 Pod DNS 策略

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: dns-test
spec:
  dnsPolicy: "None"
  dnsConfig:
    nameservers:
    - 10.0.0.53
    searches:
    - production.svc.cluster.local
    - svc.cluster.local
    - cluster.local
    options:
    - name: ndots
      value: "5"
```

## 8. NAT Gateway 配置

### 8.1 集群级出站 NAT

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 创建 NAT Gateway
az network nat gateway create \
  --resource-group rg-aks-net \
  --name natgw-aks-prod \
  --public-ip-prefix pip-prefix-natgw \
  --idle-timeout 4

# 关联到 AKS 子网
az network vnet subnet update \
  --resource-group rg-aks-net \
  --vnet-name vnet-aks-prod \
  --name aks-node-subnet \
  --nat-gateway natgw-aks-prod

# 使用 Terraform 创建
resource "azurerm_nat_gateway" "aks" {
  name                = "natgw-aks-prod"
  location            = azurerm_resource_group.aks.location
  resource_group_name = azurerm_resource_group.aks.name
  sku_name            = "Standard"
  idle_timeout_in_minutes = 4
}

resource "azurerm_public_ip_prefix" "natgw" {
  name                = "pip-prefix-natgw"
  location            = azurerm_resource_group.aks.location
  resource_group_name = azurerm_resource_group.aks.name
  prefix_length       = 28
  sku                 = "Standard"
}

resource "azurerm_nat_gateway_public_ip_prefix_association" "aks" {
  nat_gateway_id      = azurerm_nat_gateway.aks.id
  public_ip_prefix_id = azurerm_public_ip_prefix.natgw.id
}
```
### 8.2 出站类型选择

| 出站类型 | 说明 | 场景 |
|---------|------|------|
| `loadBalancer` | 默认，使用 SLB SNAT | 小规模集群 |
| `managedNATGateway` | AKS 托管 NAT GW | 中大规模生产 |
| `userAssignedNATGateway` | 用户自建 NAT GW | 精细控制 |
| `userDefinedRouting` | 自定义 UDR | 需要防火墙 |

### 8.3 SNAT 端口管理

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 监控 SNAT 端口使用
az monitor metrics list \
  --resource $SLB_ID \
  --metric SnatConnectionCount \
  --aggregation Maximum \
  --interval PT1M

# 告警：SNAT 端口使用率 > 80%
az monitor metrics alert create \
  --name snat-port-warning \
  --condition "max SnatConnectionCount > 80" \
  --window-size 5m
```
## 9. 网络策略

### 9.1 Calico NetworkPolicy

```yaml
apiVersion: projectcalico.org/v3
kind: NetworkPolicy
metadata:
  name: deny-all-ingress
  namespace: production
spec:
  selector: all()
  types:
  - Ingress
  - Egress
  egress:
  - action: Allow
    destination:
      selector: k8s-app == "kube-dns"
    protocol: UDP
    destination:
      ports:
      - 53
```

### 9.2 Azure NetworkPolicy

```yaml
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: allow-frontend-to-api
  namespace: production
spec:
  podSelector:
    matchLabels:
      app: api
  policyTypes:
  - Ingress
  ingress:
  - from:
    - podSelector:
        matchLabels:
          app: frontend
    ports:
    - port: 8080
```

## 10. 故障排查

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 检查 CNI 配置
kubectl get configmap -n kube-system azure-cni-networkconfig -o yaml

# 查看节点网络状态
kubectl get nodes -o custom-columns=NAME:.metadata.name,INTERNAL-IP:.status.addresses[0].address,POD-CIDR:.spec.podCIDR

# 检查 Pod 网络连通性
kubectl exec -it debug-pod -- ping <pod-ip>
kubectl exec -it debug-pod -- nslookup kubernetes.default

# Azure CNI 日志
kubectl logs -n kube-system -l app=azure-cns --tail=100

# 检查 Service 连通性
kubectl get svc -A
kubectl describe svc <service-name>
```
## Related

- [[01-aks-cluster-lifecycle-upgrades|AKS 集群生命周期与升级]]
- [[05-aks-troubleshooting-playbook|AKS 故障排查手册]]

## See Also

- Azure CNI 官方文档
- Cilium on AKS 文档


<!-- risk-assessed -->
