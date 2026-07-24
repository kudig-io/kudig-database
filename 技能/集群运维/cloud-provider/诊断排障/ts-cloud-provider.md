---
title: 云服务商集成排查
description: '# 云服务商集成排查'
summary: '1. **CCM 存活**：检查各云 Cloud Controller Manager Pod 状态与错误日志。'
category: skills
tags:
- k8s
- troubleshooting
- structural
- cloud-provider
- controller-manager
- istio
- ingress
- gateway
- networkpolicy
- gpu
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 云服务商集成排查 是什么
- 如何 云服务商集成排查
trigger_keywords:
- 云服务商集成排查
prerequisites:
- kubectl-basics
- service-mesh-basics
- gpu-scheduling-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 云服务商集成排查

### 01 Cloud Provider Integration Troubleshootingompt 模板|Troubleshooting]]

#### 0. 10 分钟快速诊断

1. **CCM 存活**：检查各云 Cloud Controller Manager Pod 状态与错误日志。
2. **认证权限**：验证云凭证/实例角色/IAM 权限是否有效。
3. **LB 创建**：`kubectl get svc -A | grep LoadBalancer`，查看事件与云侧资源。
4. **存储供给**：检查 StorageClass 参数与云盘配额是否超限。
5. **网络连通**：确认 VPC/安全组/路由表放通关键端口。
6. **快速缓解**：
   - 降低 API 调用频率或增加配额。
   - 临时切换到备用节点组或区域。
7. **证据留存**：保存 CCM 日志、云 API 错误、[[Service|Service]] 事件。

#### 排查方法与步骤

1. **确认 CCM 状态**：检查各云控制器 Pod 与关键错误日志。
2. **验证凭证与权限**：核对 IAM/实例角色/服务账户与配额。
3. **排查网络与路由**：确认安全组/路由表/VPC 互通规则。
4. **核对云侧资源**：在云控制台确认 LB/磁盘/网络资源状态。
5. **修复验证**：Service 状态恢复、PVC 绑定成功、关键链路可达。

#### 常见修复策略

- **权限问题**：修复 IAM/角色策略与凭证配置。
- **LB/存储异常**：提升配额并排查云侧资源冲突。
- **网络问题**：修复路由与安全组规则，必要时启用临时旁路。

---

### 02 Multi Cloud Networking Troubleshooting

#### 0. 10 分钟快速诊断

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

#### 2. 排查方法与步骤



#### 2.1 诊断原理说明

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

---

### 03 Cloud Resource Quota Troubleshooting

#### 0. 10 分钟快速诊断

1. **CCM 日志扫描**：`kubectl logs -n kube-system -l app=cloud-controller-manager --tail=200 | grep -iE "quota|rate|limit|throttle"`。
2. **云 CLI 配额检查**：使用对应云厂商 CLI（`aws ec2 describe-account-attributes`、`az vm list-usage`）查看当前配额使用率。
3. **事件检查**：`kubectl get events --field-selector reason=FailedScheduling` 查看是否因配额不足导致节点无法创建。
4. **Service 状态**：`kubectl get svc -A | grep Pending`，确认 LoadBalancer 是否因配额卡住。
5. **节点组状态**：检查 Cluster Autoscaler 日志中的 `InsufficientInstanceCapacity` 或配额相关错误。
6. **快速缓解**：
   - 立即申请临时配额提升（多数云厂商支持紧急配额申请）。
   - 释放闲置资源（未绑定的 EIP、过期的快照、闲置的磁盘）。
7. **证据留存**：保存 CCM 日志、云厂商配额页面截图、受影响的资源列表、配额提升工单号。

---

#### 2. 排查方法与步骤



#### 2.1 诊断原理说明

云资源配额管理是一个多层级体系：

```
# 🟢 低风险：只读/信息收集，通常无副作用
┌─────────────────────────────────────────────┐
│          组织/账户级配额 (Organization)        │
│  总 vCPU | 总内存 | 总存储 | 总网络资源        │
├─────────────────────────────────────────────┤
│          区域级配额 (Region)                   │
│  区域 vCPU | 区域实例数 | 区域 EIP | 区域 LB   │
├─────────────────────────────────────────────┤
│          可用区级配额 (Availability Zone)       │
│  AZ 实例容量 | AZ 网络资源 | AZ GPU             │
├─────────────────────────────────────────────┤
│          API 级限流 (API Rate Limit)           │
│  每秒请求数 (QPS) | 并发请求数 | 令牌桶速率     │
└─────────────────────────────────────────────┘
```
**关键概念**：
- **On-Demand Quota**：标准实例配额，适用于大多数场景
- **Spot/Preemptible Quota**：竞价实例独立配额，通常更高
- **API Rate Limit**：云厂商对 API 调用频率的限制，通常按账户+区域维度计算
- **Burst Limit**：部分云厂商允许短时突破基线限流，但持续超限会被惩罚性限流

## 相关链接

- [[实体/k8s-knowledge-map.md|K8s 知识图谱]]

## 云厂商故障排查指南

### 云厂商差异对比

| 特性 | 阿里云 ACK | AWS EKS | Azure AKS | GCP GKE |
|---|---|---|---|---|
| 控制平面 | 托管/自管 | 托管 | 托管 | 托管 |
| 网络插件 | Terway/Flannel | VPC CNI | Azure CNI | GKE CNI |
| 存储 | 云盘/NAS | EBS/EFS | Azure Disk | PD |
| 负载均衡 | SLB | ALB/NLB | Azure LB | GCP LB |

### 云厂商常见问题

| 问题 | 可能原因 | 排查方向 |
|---|---|---|
| LoadBalancer 无外部 IP | 配额不足/权限问题 | 检查云控制台配额 |
| PV 挂载失败 | 可用区不匹配 | 检查 PV/Pod 可用区 |
| 节点无法加入 | 安全组/子网配置 | 检查网络配置 |

### 云厂商排查命令

```bash
# 🟢 检查云厂商组件状态
kubectl get pods -n kube-system | grep -E 'cloud|csi|cni'
# 🟢 检查 LoadBalancer 状态
kubectl get svc -A -o wide | grep LoadBalancer
# 🟢 检查 PV 状态
kubectl get pv -o wide
```

## 面试要点

1. **Q：云厂商托管集群与自建集群的差异？**
   A：托管：控制平面免运维、集成云服务、成本优化。自建：完全控制、多云一致、定制性强。

2. **Q：云厂商网络插件的特点？**
   A：Terway(阿里云ENI)、VPC CNI(AWS原生)、Azure CNI(集成VNet)、GKE CNI(自动管理)。

3. **Q：云厂商故障排查的特殊性？**
   A：需结合云控制台、检查配额/权限、关注可用区、理解云服务依赖关系。

## Related

- [[技能/工作负载/pod/培训/测验/assessment-k8s-fundamentals-quiz.md|assessment-k8s-fundamentals-quiz]] — K8S Fundamentals Quiz
- [[实体/networkpolicy.md|networkpolicy]] — NetworkPolicy
- [[submariner]] — Submariner
- [[istio]] — Istio
- [[linkerd]] — Linkerd


<!-- risk-assessed -->
