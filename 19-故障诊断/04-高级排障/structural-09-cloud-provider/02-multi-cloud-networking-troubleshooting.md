---
title: 多云/混合云网络故障排查指南
description: '# 多云/混合云网络故障排查指南'
category: structural-troubleshooting
tags:
- k8s
- troubleshooting
- decision-tree
- prometheus
- istio
- cilium
- calico
- coredns
- opa
- job
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 技术支持
estimated_read_time: 5min
intent_queries:
- 多云/混合云网络故障排查指南 是什么
- 如何 多云/混合云网络故障排查指南
- 多云/混合云网络故障排查指南 故障排查
- 多云/混合云网络故障排查指南 排障步骤
trigger_keywords:
- 多云
- 混合云网络故障排查指南
- structural
- trouble
- shooting
prerequisites:
- kubectl-basics
- troubleshooting-methodology
- service-mesh-basics
- prometheus-basics
- cilium-basics
- cni-basics
- policy-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。


# 多云/混合云网络故障排查指南

> **适用版本**: [[23-实体/02-K8s核心组件/kubernetes|kubernetes]] v1.25 - v1.32 | **最后更新**: 2026-04 | **难度**: 高级

---

## 0. 10 分钟快速诊断

1. **基础连通性**：在跨集群 Pod 之间执行 `ping`/`curl`，确认网络层是否可达。
2. **Service 解析**：`kubectl exec` 进入 Pod，使用 `nslookup`/`dig` 测试跨集群 Service DNS 是否解析正确。
3. **路由检查**：在节点上执行 `ip route get <remote-pod-ip>`，确认路由路径是否符合预期（VPN/专线/Peering）。
4. **防火墙/安全组**：检查两端集群所在 VPC/子网的安全组、NetworkPolicy、防火墙规则是否放通必要端口。
5. **集群网格组件**：如果使用 Submariner/Linkerd multicluster/Istio multicluster，检查 broker/gateway/connector Pod 状态。
6. **快速缓解**：
   - 临时切换到公共网络通道（如互联网 + TLS）作为逃生通道。
   - 在应用层启用更积极的重试和超时配置。
7. **证据留存**：保存 `tcpdump`、路由表、iptables/nftables 规则、集群网格组件日志、云厂商网络拓扑图。

---

## 1. 问题现象与影响分析

### 1.1 常见问题现象

#### 1.1.1 跨集群 Pod 网络不通

| 现象 | 报错信息 | 报错来源 | 查看方式 |
|------|----------|----------|----------|
| 跨集群 Ping 不通 | `Destination Host Unreachable` | `ping` 命令 | Pod 内执行 |
| 跨集群连接超时 | `dial tcp <ip>:>:<port>: i/o timeout` | 应用日志 | 应用 Pod 日志 |
| DNS 解析失败 | `could not resolve host` | `curl`/`wget` | Pod 内执行 |
| 路由不可达 | `no route to host` | 应用日志 | 应用 Pod 日志 |

#### 1.1.2 跨云 VPC/Peering 连接异常

| 现象 | 报错信息 | 报错来源 | 查看方式 |
|------|----------|----------|----------|
| VPC Peering 未激活 | `VPC peering connection is not active` | 云厂商控制台 | AWS/Azure/GCP 控制台 |
| 路由表未传播 | `route not propagated to route table` | 云厂商控制台 | VPC 路由表 |
| CIDR 重叠冲突 | `peering CIDR overlaps with existing route` | 云厂商事件 | 创建 Peering 事件 |
| VPN 隧道中断 | `IPSec tunnel is down` | VPN Gateway 日志 | 云厂商 VPN 监控 |

#### 1.1.3 集群网格/服务联邦异常

| 现象 | 报错信息 | 报错来源 | 查看方式 |
|------|----------|----------|----------|
| Submariner Gateway 未就绪 | `Submariner gateway not ready` | `subctl` CLI | `subctl show all` |
| Linkerd 多集群镜像失败 | `mirror service not found` | Linkerd CLI | `linkerd multicluster check` |
| Istio 跨集群 Endpoint 缺失 | `cross-cluster endpoint not discovered` | Istiod 日志 | `istioctl proxy-config` |
| Service 导出/导入失败 | `service export failed` | 集群网格控制器 | 对应控制器日志 |

#### 1.1.4 生产环境典型场景

| 场景 | 典型现象 | 根本原因 | 解决方向 |
|------|----------|----------|----------|
| **灾备切换后服务发现中断** | 主集群故障切换到备集群后，客户端无法解析服务 | 全局 DNS/ServiceImport 未同步 | 检查 ExternalDNS + 多集群服务联邦 |
| **跨云微服务调用高延迟** | 微服务 A（AWS）调用微服务 B（Azure）延迟 200ms+ | 流量绕行公共互联网 | 使用云厂商专线/ExpressRoute/Direct Connect |
| **多集群 Ingress 路由混乱** | 同一域名在不同集群解析到不同后端 | 全局负载均衡配置冲突 | 统一使用多云 Ingress Controller（如 MCI） |
| **网络策略阻断跨集群流量** | 启用 NetworkPolicy 后跨集群通信中断 | Calico/Cilium 未配置远程集群 CIDR | 在 NetworkPolicy 中放通 remoteClusterCIDRs |

### 1.2 报错查看方式汇总

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 跨集群 Pod 连通性测试
# 在集群 A 的 Pod 中
ping <cluster-b-pod-ip>
curl -v http://://<cluster-b-service-ip>:>:<port>/health

# DNS 解析测试
dig @ @<coredns-ip> <cross-cluster-service>.svc.clusterset.local

# Submariner 诊断
subctl show all
subctl diagnose all

# Linkerd 多集群诊断
linkerd multicluster check
linkerd multicluster gateways

# Istio 多集群诊断
istioctl remote-clusters
istioctl proxy-config endpoint <pod> --cluster <service>.>.<ns>.svc.clusterset.local

# 节点路由表
ip route show table all | grep <remote-cidr>

# 云厂商 VPN/Peering 状态
# AWS: aws ec2 describe-vpc-peering-connections
# Azure: az network vnet-peering list
# GCP: gcloud compute networks peerings list
```
---

## 2. 排查方法与步骤

### 2.1 诊断原理说明

多云/混合云网络架构通常由以下层次组成：

```
# 🟢 低风险：只读/信息收集，通常无副作用
┌─────────────────────────────────────────────────────────────────┐
│                        应用层 (Pod)                              │
│  Service Discovery (DNS/ServiceImport) / mTLS (Istio/Linkerd)  │
├─────────────────────────────────────────────────────────────────┤
│                      集群网格层 (可选)                            │
│  Submariner / Linkerd Multicluster / Istio Multicluster        │
├─────────────────────────────────────────────────────────────────┤
│                      负载均衡层 (可选)                            │
│  Multi-Cluster Ingress (MCI) / Global Load Balancer            │
├─────────────────────────────────────────────────────────────────┤
│                      网络互联层                                   │
│  VPC Peering / VPN (IPSec/SSL) / 专线 (Direct Connect/ExpressRoute)│
├─────────────────────────────────────────────────────────────────┤
│                      云厂商网络层                                 │
│  AWS VPC / Azure VNet / GCP VPC / 私有云网络                    │
└─────────────────────────────────────────────────────────────────┘
```
**关键设计约束**：
- **CIDR 不重叠**：所有参与互联的集群 Pod CIDR、Service CIDR、VPC CIDR 必须互不重叠
- **双向路由**：互联双方的路由表必须互相宣告对方 CIDR
- **防火墙一致性**：安全组、NetworkPolicy、防火墙规则需在两端同步放通

### 2.2 排查逻辑决策树

```
多云网络问题
    ├── 跨集群 Pod 无法连通
    │   ├── 基础网络层不可达？
    │   │   ├── VPC Peering/VPN 状态异常？──► 修复云厂商网络互联
    │   │   ├── 路由表缺少对端 CIDR？──► 添加/传播路由
    │   │   └── 安全组/ACL 阻断？──► 放通对端 CIDR + 端口
    │   ├── 集群网格层异常？
    │   │   ├── Gateway/Broker Pod 未就绪？──► 重启/重新部署
    │   │   ├── 集群间认证令牌失效？──► 轮换 ServiceAccount token
    │   │   └── CNI 未配置远程集群 CIDR？──► 更新 CNI 配置
    │   └── DNS/服务发现失败？
    │       ├── CoreDNS 缺少联邦插件？──► 配置 multicluster DNS
    │       └── ServiceImport 未同步？──► 检查 mcs-controller
    ├── 跨集群延迟过高
    │   ├── 流量绕行公共互联网？──► 配置专线/VPN 替代公网
    │   ├── MTU 不匹配导致分片？──► 统一隧道/网卡 MTU
    │   └── DNS 解析跨地域？──► 部署就近 DNS 缓存
    └── 服务联邦不稳定
        ├── Endpoint 漂移频繁？──► 调整 health check 参数
        ├── 跨集群证书过期？──► 轮换 Istio/Linkerd 证书
        └── 控制平面网络分区？──► 增强控制平面网络冗余
```

### 2.3 详细诊断命令

#### 基础网络连通性诊断

```bash
#!/bin/bash
# 跨集群网络连通性诊断脚本
# 在集群 A 的节点/Pod 上运行

REMOTE_POD_IP=${1:-""}
REMOTE_SVC_IP=${2:-""}
REMOTE_DNS_NAME=${3:-""}

if [ -z "$REMOTE_POD_IP" ]; then
  echo "用法: $0 <remote-pod-ip> [remote-svc-ip] [remote-dns-name]"
  exit 1
fi

echo "=== 跨集群网络连通性诊断 ==="

# 1. 本地路由检查
echo "1. 到 $REMOTE_POD_IP 的路由:"
ip route get $REMOTE_POD_IP

# 2. Ping 测试
echo ""
echo "2. Ping 测试 (10 次):"
ping -c 10 -W 2 $REMOTE_POD_IP

# 3. 端口连通性测试 (TCP)
echo ""
echo "3. TCP 端口连通性:"
for port in 80 443 8080 9090; do
  timeout 3 bash -c "echo >/dev/tcp/$REMOTE_POD_IP/$port" 2>/dev/null
  if [ $? -eq 0 ]; then
    echo "  ✓ $REMOTE_POD_IP:$port 可达"
  else
    echo "  ✗ $REMOTE_POD_IP:$port 不可达"
  fi
done

# 4. MTU 测试
echo ""
echo "4. MTU 路径测试:"
for size in 1500 1472 1400 1300; do
  ping -c 2 -M do -s $size $REMOTE_POD_IP >/dev/null 2>&1
  if [ $? -eq 0 ]; then
    echo "  ✓ MTU $(($size + 28)) 可达 (payload=$size)"
  else
    echo "  ✗ MTU $(($size + 28)) 不可达 (payload=$size)"
  fi
done

# 5. DNS 解析测试（如提供）
if [ -n "$REMOTE_DNS_NAME" ]; then
  echo ""
  echo "5. DNS 解析测试:"
  dig +short $REMOTE_DNS_NAME
fi

# 6. 本地 iptables/nftables 检查
echo ""
echo "6. 本地防火墙规则检查:"
echo "  iptables 中到 $REMOTE_POD_IP 的规则:"
iptables -L -n -v | grep $REMOTE_POD_IP | head -5
echo "  (如使用 nftables，请执行: nft list ruleset | grep $REMOTE_POD_IP)"
```

#### Submariner 深度诊断

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
#!/bin/bash
# Submariner 深度诊断脚本

echo "=== Submariner 深度诊断 ==="

# 1. 基础状态
echo "1. Submariner 整体状态:"
subctl show all 2>/dev/null || echo "  ⚠ subctl 未安装或无法连接"

# 2. Gateway 节点状态
echo ""
echo "2. Gateway 节点状态:"
kubectl get pods -n submariner-operator -l app=submariner-gateway
kubectl get pods -n submariner-operator -l app=submariner-routeagent

# 3. Gateway 日志
echo ""
echo "3. Gateway 错误日志:"
for pod in $(kubectl get pods -n submariner-operator -l app=submariner-gateway -o name); do
  echo "=== $pod ==="
  kubectl logs -n submariner-operator $pod --tail=50 | grep -iE "error|fail|timeout" | tail -5
done

# 4. 隧道状态 (VXLAN/IPSec) 
echo ""
echo "4. 隧道接口检查:"
ip link show | grep -E "submariner|vx-submariner"

# 5. Submariner 路由
echo ""
echo "5. Submariner 注入的路由:"
ip route show | grep submariner

# 6. 集群 CIDR 和 Service CIDR 配置
echo ""
echo "6. Submariner 集群配置:"
kubectl get configmap submariner-clusters -n submariner-operator -o yaml 2>/dev/null | \
  grep -E "clusterCIDR|serviceCIDR" || echo "  未找到 submariner-clusters ConfigMap"

# 7. 全局net（GlobalNet）状态（如启用）
echo ""
echo "7. GlobalNet 状态（如启用）:"
kubectl get pods -n submariner-operator -l app=submariner-globalnet 2>/dev/null || \
  echo "  GlobalNet 未启用"
```
#### Istio 多集群诊断

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
#!/bin/bash
# Istio 多集群诊断脚本

echo "=== Istio 多集群诊断 ==="

# 1. 远程集群连接状态
echo "1. 远程集群连接状态:"
istioctl remote-clusters 2>/dev/null || echo "  ⚠ istioctl 未配置或无法连接"

# 2. Istiod 多集群日志
echo ""
echo "2. Istiod 多集群相关日志:"
kubectl logs -n istio-system deployment/istiod --tail=200 | \
  grep -iE "remote|cluster|endpoint|multicluster" | tail -20

# 3. 跨集群 Endpoint 检查
echo ""
echo "3. 跨集群 Endpoint 检查:"
# 获取一个多集群服务的 endpoint
SAMPLE_POD=$(kubectl get pods -n default -o name | head -1)
if [ -n "$SAMPLE_POD" ]; then
  echo "  检查 Pod $SAMPLE_POD 的 cluster 配置:"
  istioctl proxy-config cluster $SAMPLE_POD -n default | grep -E "outbound|clusterset" | head -10
fi

# 4. ServiceEntry 检查
echo ""
echo "4. ServiceEntry 配置检查:"
kubectl get serviceentry -A -o json | jq -r '
  .items[] | select(.spec.location == "MESH_INTERNAL") |
  "  \(.metadata.namespace)/\(.metadata.name): hosts=\(.spec.hosts | join(","))"
'

# 5. 跨集群证书状态
echo ""
echo "5. 跨集群证书状态:"
kubectl get secret cacerts -n istio-system -o jsonpath='{.data.ca-cert\.pem}' 2>/dev/null | \
  base64 -d | openssl x509 -noout -dates -subject 2>/dev/null || \
  echo "  未找到自定义 cacerts 或使用自签名证书"
```
---

## 3. 解决方案与风险控制

### 3.1 基础网络互联修复

#### 方案一：AWS-Azure 跨云 VPC/VNet Peering + VPN 备份

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
#!/bin/bash
# AWS-Azure 跨云网络配置脚本（概念示例）

# ===== AWS 侧配置 =====
# 1. 创建 VPC Peering（仅适用于同账号/组织，跨云需使用 VPN）
# AWS 与 Azure 之间通常使用 VPN Gateway 或专线

echo "=== AWS 侧 VPN 配置 ==="
# 创建 Customer Gateway（指向 Azure VPN Gateway 公网 IP）
aws ec2 create-customer-gateway \
  --type ipsec.1 \
  --public-ip <azure-vpn-gateway-public-ip> \
  --bgp-asn 65001 \
  --tag-specifications 'ResourceType=customer-gateway,Tags=[{Key=Name,Value=azure-cgw}]'

# 创建 VPN Connection
aws ec2 create-vpn-connection \
  --type ipsec.1 \
  --customer-gateway-id <cgw-id> \
  --vpn-gateway-id <vgw-id> \
  --options "StaticRoutesOnly=true"

# 添加静态路由（指向 Azure VNet CIDR）
aws ec2 create-vpn-connection-route \
  --vpn-connection-id <vpn-connection-id> \
  --destination-cidr-block 10.2.0.0/16  # Azure VNet CIDR

# ===== Azure 侧配置 =====
echo "=== Azure 侧 VPN 配置 ==="
# 创建 Local Network Gateway（指向 AWS VPC CIDR）
az network local-gateway create \
  --name aws-lgw \
  --resource-group my-rg \
  --gateway-ip-address <aws-vpn-gateway-public-ip> \
  --local-address-prefixes 10.1.0.0/16  # AWS VPC CIDR

# 创建 VPN Connection
az network vpn-connection create \
  --name aws-vpn-connection \
  --resource-group my-rg \
  --vnet-gateway1 <azure-vnet-gateway> \
  --local-gateway2 aws-lgw \
  --shared-key <pre-shared-key> \
  --connection-type IPsec
```
#### 方案二：GCP Cloud Interconnect + Router 动态路由

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
#!/bin/bash
# GCP Cloud Interconnect + Cloud Router 配置（概念示例）

echo "=== GCP 专线 + 动态路由配置 ==="

# 1. 创建 VLAN Attachment（已有机柜/端口）
gcloud compute interconnects attachments dedicated create my-attachment \
  --region us-central1 \
  --interconnect <interconnect-name> \
  --router my-cloud-router \
  --bandwidth BPS_10G

# 2. 配置 Cloud Router BGP（宣告 GCP VPC CIDR，学习对端 CIDR）
gcloud compute routers add-bgp-peer my-cloud-router \
  --region us-central1 \
  --peer-name on-prem-peer \
  --peer-asn 65002 \
  --interface my-attachment-interface \
  --advertised-route-priority 100

# 3. 宣告 GCP 子网
gcloud compute routers update my-cloud-router \
  --region us-central1 \
  --advertised-routes-mode custom \
  --set-advertised-ranges=10.3.0.0/16  # GCP VPC CIDR

# 4. 检查 BGP 会话状态
gcloud compute routers get-status my-cloud-router --region us-central1
```
### 3.2 集群网格部署与修复

#### 方案一：Submariner 跨集群网络

```yaml
# Submariner Broker 集群部署
---
# broker-info.subm 文件生成（在 broker 集群）
# subctl deploy-broker --globalnet
---

# 成员集群加入
# subctl join broker-info.subm --clusterid cluster-east --clustercidr 10.10.0.0/16 --servicecidr 10.11.0.0/16

# Submariner 关键资源检查清单
apiVersion: v1
kind: ConfigMap
metadata:
  name: submariner-checklist
  namespace: submariner-operator
data:
  checklist: |
    Pre-deployment:
      - [ ] 所有集群的 Pod CIDR 互不重叠
      - [ ] 所有集群的 Service CIDR 互不重叠
      - [ ] Gateway 节点拥有公网 IP 或可路由的内网 IP
      - [ ] 必要端口已放通 (4500/UDP for IPSec, 4800/UDP for VXLAN)
      
    Post-deployment:
      - [ ] subctl show all 显示所有集群 Connected
      - [ ] Gateway Pod 状态 Running
      - [ ] RouteAgent Pod 在每个节点上 Running
      - [ ] 跨集群 Service 可解析（GlobalNet IP 或直接路由）
```

#### 方案二：Linkerd 多集群服务镜像

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
#!/bin/bash
# Linkerd 多集群部署与修复脚本

# 1. 检查多集群安装状态
echo "1. Linkerd 多集群检查:"
linkerd multicluster check

# 2. 重新链接集群（如 token 过期）
echo ""
echo "2. 重新链接集群:"
# 在目标集群生成新凭证
linkerd multicluster link --cluster-name target-cluster > linkerd-multicluster-link.yaml

# 在源集群应用
# kubectl apply -f linkerd-multicluster-link.yaml

# 3. 检查 Gateway 状态
echo ""
echo "3. Linkerd Gateway 状态:"
kubectl get pods -n linkerd-multicluster -l app=linkerd-gateway

# 4. 检查 ServiceMirror 状态
echo ""
echo "4. ServiceMirror 状态:"
kubectl get pods -n linkerd-multicluster -l app=linkerd-service-mirror
for pod in $(kubectl get pods -n linkerd-multicluster -l app=linkerd-service-mirror -o name); do
  echo "=== $pod 日志 ==="
  kubectl logs -n linkerd-multicluster $pod --tail=20 | grep -iE "error|mirroring|fail"
done

# 5. 验证跨集群服务镜像
echo ""
echo "5. 已镜像的远程服务:"
kubectl get services -n linkerd-multicluster  # 查看 mirrored services
```
### 3.3 多集群 DNS 与服务发现

```yaml
# CoreDNS 多集群 DNS 配置
apiVersion: v1
kind: ConfigMap
metadata:
  name: coredns
  namespace: kube-system
data:
  Corefile: |
    .:53 {
        errors
        health {
            lameduck 5s
        }
        ready
        kubernetes cluster.local in-addr.arpa ip6.arpa {
            pods insecure
            fallthrough in-addr.arpa ip6.arpa
            ttl 30
        }
        # 多集群服务发现：使用 clusterset.local 域名
        # 需要配合 Kubernetes Multi-Cluster Services (MCS) API
        template IN A clusterset.local {
            match "^[^.]+\.([^\.]+)\.svc\.clusterset\.local$"
            answer "{{ .Match 1 }} 60 IN A 10.255.255.1"  # 由 Global Controller 动态更新
            fallthrough
        }
        prometheus :9153
        forward . /etc/resolv.conf {
            max_concurrent 1000
        }
        cache 30
        loop
        reload
        loadbalance
    }
---
# Kubernetes Multi-Cluster Service Export 示例
apiVersion: multicluster.x-k8s.io/v1alpha1
kind: ServiceExport
metadata:
  name: my-service
  namespace: default
# 导出后，其他集群可通过 my-service.default.svc.clusterset.local 访问
```

### 3.4 风险控制与回滚

| 操作 | 风险等级 | 影响评估 | 回滚方案 |
|------|---------|---------|---------|
| 修改 VPC Peering 路由表 | ⭐⭐⭐ 高 | 可能导致整个 VPC 网络中断 | 提前备份路由表，错误时恢复 |
| 更换 VPN Pre-Shared Key | ⭐⭐ 中 | VPN 隧道中断，跨集群通信失效 | 恢复旧 PSK 或证书 |
| 升级 Submariner/Linkerd | ⭐⭐ 中 | 跨集群网络可能暂时中断 | 使用 subctl/linkerd 降级命令 |
| 调整 CNI 远程 CIDR 配置 | ⭐⭐⭐ 高 | Pod 网络路由错误，流量丢失 | 恢复原始 CNI ConfigMap |
| 启用 GlobalNet | ⭐⭐ 中 | 现有跨集群连接需重新建立 | 禁用 GlobalNet 并恢复直接路由 |
| 修改 Istio 多集群证书 | ⭐⭐⭐ 高 | 跨集群 mTLS 握手失败 | 恢复旧 cacerts Secret |

### 3.5 验证与监控

#### 跨集群网络健康检查脚本

```bash
#!/bin/bash
# 跨集群网络健康检查脚本
# 建议在监控 CronJob 中定期执行

REPORT_FILE="/var/log/kubernetes/multicluster-health-$(date +%Y%m%d-%H%M%S).log"
REMOTE_CLUSTERS=("cluster-b" "cluster-c")  # 配置远程集群地址/端口

echo "=== 跨集群网络健康检查 $(date) ===" | tee $REPORT_FILE

for cluster in "${REMOTE_CLUSTERS[@]}"; do
  echo "" | tee -a $REPORT_FILE
  echo "--- 检查远程集群: $cluster ---" | tee -a $REPORT_FILE
  
  # 1. 解析远程网关 IP
  GATEWAY_IP=$(dig +short $cluster-gateway.example.com)
  if [ -z "$GATEWAY_IP" ]; then
    echo "✗ 无法解析 $cluster 网关 DNS" | tee -a $REPORT_FILE
    continue
  fi
  echo "网关 IP: $GATEWAY_IP" | tee -a $REPORT_FILE
  
  # 2. Ping 测试
  ping -c 5 -W 2 $GATEWAY_IP >/dev/null 2>&1
  if [ $? -eq 0 ]; then
    echo "✓ 网关可达" | tee -a $REPORT_FILE
  else
    echo "✗ 网关不可达" | tee -a $REPORT_FILE
  fi
  
  # 3. TCP 端口测试
  for port in 4500 8080 6443; do
    timeout 3 bash -c "echo >/dev/tcp/$GATEWAY_IP/$port" 2>/dev/null
    if [ $? -eq 0 ]; then
      echo "✓ 端口 $port 可达" | tee -a $REPORT_FILE
    else
      echo "✗ 端口 $port 不可达" | tee -a $REPORT_FILE
    fi
  done
  
  # 4. HTTP 健康检查（如适用）
  HTTP_STATUS=$(curl -s -o /dev/null -w "%{http_code}" --connect-timeout 5 http://$GATEWAY_IP:8080/health 2>/dev/null)
  if [ "$HTTP_STATUS" = "200" ]; then
    echo "✓ HTTP 健康检查通过" | tee -a $REPORT_FILE
  else
    echo "✗ HTTP 健康检查失败 (状态码: $HTTP_STATUS)" | tee -a $REPORT_FILE
  fi
done

echo "" | tee -a $REPORT_FILE
echo "检查报告已保存: $REPORT_FILE" | tee -a $REPORT_FILE
```

#### Prometheus 多集群网络监控告警

```yaml
# Prometheus 多集群网络监控告警
groups:
- name: multicluster-network
  rules:
  - alert: CrossClusterPingFailed
    expr: |
      probe_success{job="multicluster-ping"} == 0
    for: 2m
    labels:
      severity: critical
    annotations:
      summary: "跨集群网络连通性中断"
      description: "到集群 {{ $labels.target_cluster }} 的 ping 探测失败"

  - alert: CrossClusterLatencyHigh
    expr: |
      probe_duration_seconds{job="multicluster-ping"} > 0.5
    for: 5m
    labels:
      severity: warning
    annotations:
      summary: "跨集群网络延迟过高"
      description: "到集群 {{ $labels.target_cluster }} 的平均延迟超过 500ms"

  - alert: SubmarinerGatewayNotReady
    expr: |
      submariner_gateway_active_connections < 1
    for: 2m
    labels:
      severity: critical
    annotations:
      summary: "Submariner Gateway 未就绪"
      description: "Submariner Gateway 在 {{ $labels.instance }} 上没有活跃连接"

  - alert: IstioRemoteClusterDisconnected
    expr: |
      istio_remote_clusters{status="Connected"} < 1
    for: 2m
    labels:
      severity: critical
    annotations:
      summary: "Istio 远程集群连接断开"
      description: "Istiod 无法连接到远程集群 {{ $labels.cluster }}"
```

### 3.6 最佳实践

1. **CIDR 规划先行**：在集群创建前规划全局唯一的 Pod/Service/VPC CIDR，使用 10.x.0.0/16 或更大段避免重叠
2. **分层互联策略**：
   - 同云厂商多区域：使用云厂商原生 VPC Peering/Transit Gateway
   - 跨云厂商：使用 VPN (IPSec) 作为基础，专线 (Direct Connect/ExpressRoute) 作为高性能通道
   - 公私混合：使用 VPN/专线连接私有数据中心和公有云
3. **集群网格选型**：
   - 需要 Pod IP 直达：Submariner（CNI 级别互联）
   - 仅服务级互联：Linkerd Multicluster（Service 镜像）
   - 完整服务网格 + 安全：Istio Multicluster（mTLS + 流量管理）
4. **逃生通道**：始终保留基于公共互联网 + TLS 的备用通信路径，防止专线/Peering 问题时业务完全中断
5. **监控覆盖**：对跨集群网络延迟、丢包率、VPN 隧道状态配置独立监控和告警
6. **安全组最小权限**：仅放通必要的 CIDR 和端口，避免使用 0.0.0.0/0

### 典型问题案例

#### 案例一：Submariner 启用后跨集群 DNS 解析失败

**问题描述**：Submariner 部署后跨集群 Pod IP 可达，但 Service DNS 无法解析。

**根本原因**：Submariner GlobalNet 分配了全局 IP（如 242.x.x.x），但 CoreDNS 未配置 `clusterset.local` 域的解析。

**解决方案**：
1. 部署 Kubernetes MCS (Multi-Cluster Services) Controller
2. 导出服务：`kubectl apply -f serviceexport.yaml`
3. 在 CoreDNS 中配置 `clusterset.local` 域的 forward 到 MCS DNS

#### 案例二：VPC Peering 创建成功但路由不互通

**问题描述**：AWS VPC Peering 状态为 `active`，但两端 EC2 实例无法互 ping。

**根本原因**：VPC Peering 不会自动修改路由表，需要手动在两端路由表中添加对端 CIDR 的路由条目。

**解决方案**：
1. 在 VPC A 的路由表中添加目标为 VPC B CIDR、下一跳为 Peering Connection 的路由
2. 在 VPC B 的路由表中添加目标为 VPC A CIDR、下一跳为 Peering Connection 的路由
3. 确保安全组放通对端 CIDR 的 ICMP/必要端口

#### 案例三：跨云微服务调用间歇性 503

**问题描述**：部署 Istio 多集群后，跨集群服务调用间歇性返回 503。

**根本原因**：Istio 的跨集群 Endpoint 发现依赖控制平面网络，控制平面网络偶发丢包导致 Endpoint 被错误摘除。

**解决方案**：
1. 调大 Istio 的 `OUTLIER_DETECTION` 阈值，避免短暂网络抖动导致熔断
2. 为控制平面网络配置更高的 QoS 优先级
3. 在 DestinationRule 中配置更宽容的连接池设置


<!-- risk-assessed -->
