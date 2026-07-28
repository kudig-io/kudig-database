---
title: ACK 关联产品 - VPC 网络
description: ACK 集群 VPC 网络规划实践：地址段设计、vSwitch 多可用区策略、Terway CNI 集成、NAT 出口与混合云互联
summary: ACK 集群 VPC 网络规划实践指南，覆盖 VPC/Pod/Service 三大网段设计、多可用区 vSwitch 策略、Terway CNI 模式选择、NAT 网关出口管理与 CEN/专线混合云互联，附网络验证命令。
category: general
tags:
- cloud
- multi-cloud
- networking
- docker
- gateway
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- ACK 关联产品 - VPC 网络 是什么
- 如何 ACK 关联产品 - VPC 网络
- Kubernetes 12 cloud providers 最佳实践
trigger_keywords:
- ACK
- 关联产品
- VPC
- 网络
- cloud
- providers
prerequisites:
- kubectl-basics
- troubleshooting-methodology
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。


# ACK 关联产品 - VPC 网络

> **适用版本**: ACK v1.25 - v1.32 | **最后更新**: 2026-01

---

## 目录

- [VPC 网络规划](#vpc-网络规划)
- [子网 (vSwitch) 设计策略](#子网-vswitch-设计策略)
- [Terway CNI 与 VPC 集成](#terway-cni-与-vpc-集成)
- [网络配置验证方法](#网络配置验证方法)
- [出口流量管理 (NAT Gateway)](#出口流量管理-nat-gateway)
- [多集群与混合云互联](#多集群与混合云互联)

---

## VPC 网络规划

### 核心地址段建议

| 网段类型 | 建议范围 | 覆盖范围 | 约束 |
|:---|:---|:---|:---|
| **VPC 网段** | `192.168.0.0/12` | 整个专有网络 | 后续不可修改，需预留充足空间 |
| **Pod 网段** | `172.20.0.0/16` | 集群 Pod 使用 | 不能与 VPC/Service 网段冲突 |
| **[[service\|Service]] 网段** | `172.21.0.0/20` | 集群内部 Service | 必须是私网地址段 |

### IP 地址分配估算公式

```bash
所需 IP 总数 = 节点数 × (每一个 ENI 的 IP 数 × ENI 密度) + 节点本身 IP数
# 建议为业务预留 2-3 倍的弹性空间。
```

---

## 子网 (vSwitch) 设计策略

### 多可用区设计 (HA)

生产环境建议跨至少 **3 个可用区 (AZ)** 分配子网：

- **vSwitch-AZ1 (10.0.1.0/24)**: 用于核心节点。
- **vSwitch-AZ2 (10.0.2.0/24)**: 用于核心节点。
- **vSwitch-AZ3 (10.0.3.0/24)**: 用于弹性节点 (Spot)。

### 虚拟交换机用途划分

| 用途 | 设计建议 |
|:---|:---|
| **Pod 专属子网** | 使用 Terway 时，建议为 Pod 分配独立子网，避免与 ECS 混用 |
| **SLB 专属子网** | 为内网负载均衡器预留小段子网范围 |

---

## Terway CNI 与 VPC 集成

### Terway 优势

- **真·VPC IP**: Pod IP 直接属于 VPC，网络延迟几乎等同于 ECS 到 ECS。
- **安全组集成**: Pod 级别可以直接设置阿里云安全组。
- **无 VXLAN 损耗**: 避免了传统叠加网络 (Overlay) 的封装性能损耗。

### 模式选择

| 模式 | 描述 | 适用规模 |
|:---|:---|:---|
| **ENI 多 IP** | 为 ENI 绑定辅助内网 IP | 推荐，最通用 |
| **Trunk ENI** | 中继 ENI 模式，高密度 | 高密度 Pod 部署场景 |
| **IPv4/IPv6 双栈** | 支持双栈协议 | 全球性业务 |

---

## 网络配置验证方法

### 验证 Terway 与 Pod IP 分配

```bash
# 🟢 低风险：确认 Terway DaemonSet 全部 Ready
kubectl -n kube-system get ds terway-eniip

# 🟢 低风险：Pod IP 应落在规划的 Pod 网段内
kubectl get pods -A -o wide | head -20

# 🟢 低风险：查看节点 ENI 配额使用情况（ENI 耗尽会导致 Pod 卡 ContainerCreating）
kubectl get nodes -o custom-columns='NAME:.metadata.name,MAX-POD:.status.allocatable.pods'
kubectl -n kube-system logs ds/terway-eniip -c terway --tail=30 | grep -i 'eni\|ip pool'
```

### 验证跨可用区与出口连通性

```bash
# 🟢 低风险：确认节点分布在多个可用区
kubectl get nodes -L topology.kubernetes.io/zone

# 🟢 低风险：在集群内验证 SNAT 出口公网 IP（应为 NAT 网关绑定的 EIP）
kubectl run nettest --rm -it --image=registry.cn-hangzhou.aliyuncs.com/acs/busybox:latest --restart=Never -- wget -qO- http://ifconfig.me

# 🟢 低风险：Pod 间跨节点连通性抽样
kubectl exec <pod-a> -- ping -c 3 <pod-b-ip>
```

---

## 出口流量管理 (NAT Gateway)

### 典型架构

```mermaid
graph TD
    A[Pod] --> B[ECS Node]
    B --> C[VPC 路由表]
    C --> D[NAT Gateway]
    D --> E((Internet))
```

### 关键配置

1. **SNAT 规则**: 确保私网 Pod 可以访问外网 (如下载拉取外部镜像)。
2. **固定 EIP**: 许多外部 API 接口需要白名单，通过 NAT Gateway 绑定固定 EIP 实现。
3. **共享带宽包**: 多个 EIP 共享带宽，节省成本。

---

## 多集群与混合云互联

### 互联方案对比

| 方案 | 特点 | 场景 |
|:---|:---|:---|
| **CEN (云企业网)** | 自动路由传播，全球互联 | 跨地域集群互通 |
| **VPC Peering** | 简单对等连接 | 同地域两个 VPC 互通 |
| **VPN 网关** | 加密隧道，成本低 | 办公网与 ACK 集群互联 |
| **高速通道 (Express Connect)** | 物理专线，低延迟 | IDC 机房与 ACK 混合云 |

### 选型决策要点

1. **跨地域多集群**：优先 CEN，自动路由传播避免手工维护路由表；注意各集群 Pod/Service 网段不可重叠。
2. **办公网访问集群 API**：VPN 网关成本最低；需长期稳定大带宽则上专线。
3. **混合云节点接入（ACK One/注册集群）**：专线 + CEN 组合，保证 kubelet 到控制面的延迟 < 100ms。

---

## 相关文档

- [[05-网络/06-Terway/index|Terway 专题]]
- [[05-网络/02-网络基础/index|网络协议基础]]
- [[18-云厂商/01-阿里云/index|阿里云域索引]]

## Related

- [[17-系统基础/05-速查卡/networking.md|networking]]
- [[17-系统基础/05-速查卡/k8s.md|k8s]]
- [[23-实体/02-K8s核心组件/kubernetes.md|kubernetes]]
- [[21-生态参考/03-领域索引/terway-index.md|Terway 知识图谱索引]]
- [[13-生产运维/05-工单案例/ticket-case-001-terway-eni-exhaustion|工单案例：Terway ENI 耗尽]]

## See Also

- [[18-云厂商/01-阿里云/公有云-ACK/240-ack-ecs-compute.md|240-ack-ecs-compute]]
- [[18-云厂商/01-阿里云/公有云-ACK/241-ack-slb-nlb-alb.md|241-ack-slb-nlb-alb]]
- [[18-云厂商/01-阿里云/公有云-ACK/243-ack-ram-authorization.md|243-ack-ram-authorization]]
- [[18-云厂商/01-阿里云/公有云-ACK/244-ack-ros-iac.md|244-ack-ros-iac]]


<!-- risk-assessed -->
