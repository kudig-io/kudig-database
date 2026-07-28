---
title: Kube-OVN (entities)
description: '## 概述'
summary: 'Kube-OVN 是一个基于 OVN/OVS 的高级 Kubernetes 网络 CNI 插件，将 SDN（软件定义网络）的能力引入 Kubernetes。它提供子网管理、固定 IP、QoS、网络策略、EIP/SNAT、VPC 多租户等企业级网络功能，是 Kubernetes 网络功能最丰富的 CNI 之一。'
category: entities
tags:
- k8s
- cncf
- networking
- kube-ovn
- statefulset
- networkpolicy
- crd
- operator
tier: supporting
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Kube-OVN 是什么
- 如何 Kube-OVN
trigger_keywords:
- Kube-OVN
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Kube-OVN

> **CNCF 状态**: Sandbox | **类别**: Networking | **主要语言**: Go

## 概述

Kube-OVN 是由灵雀云（Alauda）开源的高级 Kubernetes CNI 网络插件，基于 OVN（Open Virtual Network）/OVS（Open vSwitch）构建。它将 SDN（软件定义网络）的能力引入 Kubernetes，提供子网管理、固定 IP、QoS、网络策略、EIP/SNAT、VPC 多租户等企业级网络功能。Kube-OVN 是 Kubernetes 网络功能最丰富的 CNI 之一，特别适合需要复杂网络拓扑的企业场景。

## 核心特性

- **子网管理**: Namespace 与子网关联，支持自定义 CIDR 和网关
- **固定 IP**: 为 [[statefulset|StatefulSet]] Pod 和普通 Pod 提供固定 IP 分配
- **VPC 多租户**: 自定义 VPC 实现网络级隔离，支持跨子网路由
- **QoS 带宽管理**: 为 Pod 配置入站/出站带宽限制
- **网络策略**: 增强 NetworkPolicy，支持 ICMP、网段级别控制
- **EIP/SNAT/DNAT**: 外部 IP 映射和 NAT 能力

## 架构

Kube-OVN 基于 OVN（Open Virtual Network）构建。核心组件包括：kube-ovn-controller（主控制器，监听 K8s API 管理 OVN 逻辑路由器、交换机和端口）、kube-ovn-cni（节点 CNI 插件，管理 OVS 网桥和接口）、ovn-nb/ovn-sb（OVN 北向/南向数据库）。网络数据平面使用 OVS 内核模块或 DPDK 加速。OVN 提供逻辑路由器、逻辑交换机、ACL 和 LB 能力，Kube-OVN 将这些能力以 CRD（Subnet、VPC、IP、Vip 等）暴露给用户。

## Kubernetes 集成

Kube-OVN 作为标准 CNI 插件与 Kubernetes 集成。通过 CRD（Subnet、VPC、Vip、IP、QoS）声明式管理网络资源。Subnet CRD 将 Namespace 与 OVN 子网关联，Pod 创建时自动分配子网内 IP。VPC CRD 创建隔离的虚拟网络，实现多租户。支持 Kubernetes NetworkPolicy API 和自定义增强策略。通过 kube-ovn-controller 将 OVN 配置同步到每个节点的 OVS 实例。

## 生产使用场景

1. **多租户网络隔离**: 使用自定义 VPC 为不同租户创建隔离网络环境
2. **固定 IP 需求**: 为传统应用（如数据库、中间件）提供固定 Pod IP
3. **混合云网络**: 通过 VPC 互联和 EIP 实现与外部网络的灵活连接
4. **QoS 流量控制**: 对不同优先级应用实施带宽限制

## 安装与配置

```bash
# 一键安装
kubectl apply -f https://raw.githubusercontent.com/kubeovn/kube-ovn/master/dist/images/install.yaml
# 或 Helm
helm repo add kubeovn https://kubeovn.github.io/kube-ovn/
helm install kube-ovn kubeovn/kube-ovn
# 创建子网
kubectl apply -f - <<EOF
apiVersion: kubeovn.io/v1
kind: Subnet
metadata:
  name: prod
spec:
  protocol: IPv4
  cidrBlock: 10.0.1.0/24
  gateway: 10.0.1.1
  namespaces:
    - production
EOF
```

### VPC 多租户配置

```yaml
apiVersion: kubeovn.io/v1
kind: Vpc
metadata:
  name: tenant-a-vpc
spec:
  namespaces:
    - tenant-a
  staticRoutes:
    - cidr: 10.0.0.0/16
      nextHopIP: 10.0.1.1
---
apiVersion: kubeovn.io/v1
kind: Subnet
metadata:
  name: tenant-a-subnet
spec:
  vpc: tenant-a-vpc
  protocol: IPv4
  cidrBlock: 10.1.0.0/24
  gateway: 10.1.0.1
  namespaces:
    - tenant-a
```

### 固定 IP 和 QoS

```yaml
# 固定 IP Pod
apiVersion: v1
kind: Pod
metadata:
  name: db-pod
  annotations:
    ovn.kubernetes.io/ip_address: "10.0.1.100"
    ovn.kubernetes.io/mac_address: "00:00:00:53:6B:B6"
spec:
  containers:
    - name: db
      image: postgres:16
---
# QoS 带宽限制
apiVersion: kubeovn.io/v1
kind: QoSPolicy
metadata:
  name: app-qos
spec:
  bandwidthLimitRules:
    - name: ingress-limit
      direction: ingress
      rate: 100
      burst: 20
    - name: egress-limit
      direction: egress
      rate: 50
      burst: 10
```

## 运维操作

```bash
# 🟢 查看子网状态
kubectl get subnets
kubectl describe subnet prod

# 🟢 查看 VPC 列表
kubectl get vpcs

# 🟢 查看 IP 分配情况
kubectl get ips
kubectl get ips -o custom-columns=NAME:.metadata.name,IP:.spec.ipAddress,SUBNET:.spec.subnet

# 🟢 检查 OVN 状态
kubectl exec -n kube-system deploy/ovn-central -- ovn-nbctl show

# 🟢 查看 OVS 流表
kubectl exec -n kube-system ds/ovs-ovn -- ovs-ofctl dump-flows br-int

# 🟡 创建 EIP
kubectl apply -f - <<EOF
apiVersion: kubeovn.io/v1
kind: IptablesEIP
metadata:
  name: eip-web
spec:
  natGwDp: gw-external
  v4ip: 203.0.113.10
EOF

# 🟢 网络连通性测试
kubectl exec -it pod-a -- ping 10.0.1.100
```

## 故障排查

| 症状 | 可能原因 | 诊断命令 | 修复方案 |
|------|----------|----------|----------|
| Pod 无 IP | 子网 CIDR 耗尽 | `kubectl describe subnet` | 扩大子网 CIDR |
| 跨节点不通 | OVS 隧道异常 | `kubectl exec ds/ovs-ovn -- ovs-vsctl show` | 检查隧道端口状态 |
| DNS 解析失败 | CoreDNS Pod 网络异常 | `kubectl logs -n kube-system coredns-*` | 检查 CoreDNS Service IP |
| VPC 隔离失效 | 路由配置错误 | `ovn-nbctl lr-route-list <vpc>` | 检查静态路由配置 |
| QoS 不生效 | 策略未关联 | `kubectl get qospolicies` | 检查 QoS 策略绑定 |

### 排查流程

```
Kube-OVN 网络异常
├─ Pod 无法获取 IP？
│  ├─ 子网耗尽 → 扩大 CIDR 或清理 IP
│  ├─ Controller 异常 → 检查 kube-ovn-controller 日志
│  └─ OVN DB 异常 → 检查 ovn-nb/ovn-sb 状态
├─ 网络不通？
│  ├─ 同节点 → 检查 OVS 流表和端口
│  ├─ 跨节点 → 检查隧道（Geneve/VXLAN）
│  └─ 跨 VPC → 检查路由和 ACL
└─ 性能问题？
   ├─ 带宽低 → 检查 QoS 策略和 MTU
   └─ 延迟高 → 检查 OVS 流表复杂度
```

## 生产案例

### 案例 1: 电信多租户网络隔离

**场景**: 电信运营商在 K8s 上为多个企业客户提供隔离的网络环境。

**方案**:
1. 每个客户分配独立 VPC
2. VPC 内创建多个子网对应不同业务
3. 通过 EIP 提供外部访问
4. QoS 策略保证 SLA

**效果**: 单集群支持 100+ 租户，网络完全隔离，满足电信级 SLA。

### 案例 2: 数据库固定 IP 迁移

**场景**: 传统数据库集群迁移到 K8s，客户端配置了固定 IP 无法修改。

**方案**:
1. 使用 Kube-OVN 固定 IP 功能
2. 为数据库 Pod 指定原有 IP 地址
3. StatefulSet 确保 Pod 重建后 IP 不变

**效果**: 数据库迁移零客户端修改，业务无感知。

## 对比与替代方案

| 维度 | Kube-OVN | Calico | Cilium | Antrea |
|------|----------|--------|--------|--------|
| VPC 多租户 | ✅ | ❌ | ❌ | ❌ |
| 固定 IP | ✅ | ❌ | ❌ | ❌ |
| QoS | ✅ | 有限 | ✅ | ✅ |
| EIP/NAT | ✅ | ❌ | ❌ | ❌ |
| 性能 | 中 (OVS) | 高 (BGP) | 高 (eBPF) | 中 (OVS) |
| 运维复杂度 | 高 | 低 | 中 | 中 |

## 检查清单

- [ ] OVN 中央组件高可用部署（3 副本）
- [ ] 子网 CIDR 规划合理，避免冲突
- [ ] VPC 隔离策略已测试验证
- [ ] QoS 策略已配置并测试
- [ ] 网络监控已配置（OVS 指标 + Prometheus）
- [ ] 备份策略：OVN NB/SB 数据库定期备份
- [ ] 升级路径已验证（OVS 版本兼容性）
- [ ] 故障恢复演练已完成

## 替代方案

| 项目 | 优势 | 劣势 |
|------|------|------|
| **Kube-OVN** | 功能最全面、VPC 多租户 | OVS 运维复杂、性能开销 |
| Calico | 高性能、BGP 原生 | 无 VPC/固定 IP 功能 |
| Cilium | eBPF 高性能、可观测性强 | 企业网络功能较少 |
| Antrea | OVS 基础、策略丰富 | 无 VPC 多租户 |

## 架构定位

在 CNCF 生态中，Kube-OVN 属于 **Networking** 类别，是将 SDN 能力引入 Kubernetes 的代表性项目。适合需要复杂网络功能的企业场景。

## 参考链接

- [[23-实体/02-K8s核心组件/cni-plugins.md|cni-plugins]]
- [[23-实体/networkpolicy.md|[[networkpolicy|networkpolicy]]]]
- [[23-实体/02-K8s核心组件/crd-custom-resources.md|crd-custom-resources]]
- [[22-概念/01-核心架构/controller-pattern.md|controller-pattern]]
- [[pod-lifecycle]]

## Related

- [[sermant]] — Sermant
- [[loxilb]] — LoxiLB
- [[23-实体/02-K8s核心组件/statefulset.md|statefulset]] — StatefulSet
- [[cni]] — CNI (Container Network Interface)
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- kube-ovn
- [[23-实体/15-参考与索引/cncf-networking.md|CNCF 网络与服务网格项目全景]] — Cross-reference
- [[21-生态参考/03-领域索引/etcd-index.md|etcd 知识图谱索引]]
- [[21-生态参考/03-领域索引/network-index.md|Network 网络知识图谱索引]]
- [[21-生态参考/03-领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
