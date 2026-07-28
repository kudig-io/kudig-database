---
title: OVN-Kubernetes (entities)
description: '## 概述'
summary: 'OVN-Kubernetes 是一个基于 OVN (Open Virtual Network) 的 Kubernetes CNI 网络插件，提供企业级的虚拟网络功能。它利用 OVN 的分布式虚拟路由、负载均衡、ACL 和 NAT 能力，为 Kubernetes 提供高性能、可扩展的 L2/L3/L4 网络。'
category: entities
tags:
- k8s
- cncf
- networking
- ovn-kubernetes
- cilium
- argocd
- networkpolicy
- crd
- operator
- ebpf
tier: peripheral
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- OVN-Kubernetes 是什么
- 如何 OVN-Kubernetes
trigger_keywords:
- OVN-Kubernetes
prerequisites:
- kubectl-basics
- gitops-basics
- ebpf-basics
- cilium-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# OVN-Kubernetes

> **CNCF 状态**: Sandbox | **类别**: Networking | **主要语言**: Go

## 概述

OVN-Kubernetes 是基于 OVN（Open Virtual Network）的 Kubernetes CNI 网络插件，由 Red Hat 维护，2022 年加入 CNCF Sandbox。它利用 OVN 的分布式虚拟路由、负载均衡、ACL 和 NAT 能力，为 Kubernetes 提供高性能、可扩展的 L2/L3/L4 网络。OVN-Kubernetes 是 OpenShift 的默认网络插件，已在大规模生产环境中验证。

## 核心特性

- **OVN 数据平面**: 基于 OVN 的高性能虚拟网络（分布式路由/交换/ACL）
- **NetworkPolicy**: 完整支持 K8s NetworkPolicy（通过 OVN ACL 实现）
- **Egress IP**: 为 Pod 出站流量分配固定源 IP
- **Egress Firewall**: 命名空间级别的出站流量控制（CRD）
- **Hybrid SDN**: 支持命名空间级的网络隔离（非全局 Pod 网络）
- **硬件卸载**: 支持 OVS 硬件卸载（SR-IOV、 Mellanox ASAP）

## 架构

OVN-Kubernetes 架构以 OVN 为核心。OVN 北向数据库（NBDB）和南向数据库（SBDB）以 Raft 集群运行（至少 3 节点 HA）。ovnkube-master（每个控制节点一个）监听 K8s API 获取 Pod、Service、NetworkPolicy 变更，将配置写入 NBDB。ovnkube-node（每个工作节点以 DaemonSet 运行）管理本地 OVS 实例，从 SBDB 获取配置更新 OVS flow table。Pod 网络通过 OVS Geneve 隧道或裸网络（hybrid）连接。NetworkPolicy 通过 OVN ACL 实现，比 iptables 性能更高。

## Kubernetes 集成

OVN-Kubernetes 作为标准 CNI 插件集成。通过 CRD（EgressIP、EgressFirewall、AdminNetworkPolicy）提供超越标准 NetworkPolicy 的高级网络控制。EgressIP CRD 为指定命名空间的 Pod 分配固定出站 IP（配合节点 IP 池）。EgressFirewall CRD 限制命名空间出站流量到特定 CIDR/端口。在 OpenShift 中作为默认 CNI，与 OpenShift SDN 无缝集成。

## 生产使用场景

1. **企业网络隔离**: 使用 Hybrid SDN 或 AdminNetworkPolicy 实现多团队网络隔离
2. **Egress IP 控制**: 为合规要求固定 Pod 出站源 IP
3. **出站防火墙**: 限制 Pod 可访问的外部网络范围
4. **高性能 NetworkPolicy**: 使用 OVN ACL 替代 iptables 实现高性能网络策略

## 安装与配置

```bash
# Helm 安装
helm repo add ovn-kubernetes https://ovn-kubernetes.github.io/ovn-kubernetes
helm install ovn-kubernetes ovn-kubernetes/ovn-kubernetes \
  -n ovn-kubernetes --create-namespace \
  --set nbdb.replicas=3 \
  --set sbdb.replicas=3

# 等待 OVN 数据库集群就绪
kubectl wait --for=condition=available statefulset/ovnkube-db -n ovn-kubernetes --timeout=180s
kubectl get pods -n ovn-kubernetes

# 验证 OVN 状态
kubectl exec -n ovn-kubernetes ovnkube-db-0 -- ovn-nbctl show
kubectl exec -n ovn-kubernetes ovnkube-db-0 -- ovn-sbctl show
```

```yaml
# EgressIP 配置（固定 Pod 出站源 IP）
apiVersion: k8s.ovn.org/v1
kind: EgressIP
metadata:
  name: egress-prod
spec:
  egressIPs:
    - "203.0.113.10"
    - "203.0.113.11"
  namespaceSelector:
    matchLabels:
      env: production
---
# EgressFirewall 配置（限制出站流量）
apiVersion: k8s.ovn.org/v1
kind: EgressFirewall
metadata:
  name: default
  namespace: production
spec:
  egress:
  - type: Allow
    to:
      cidrSelector: 10.0.0.0/8
  - type: Allow
    to:
      dnsName: api.company.com
    ports:
    - protocol: TCP
      port: 443
  - type: Deny
    to:
      cidrSelector: 0.0.0.0/0
---
# AdminNetworkPolicy（集群级网络策略）
apiVersion: policy.networking.k8s.io/v1alpha1
kind: AdminNetworkPolicy
metadata:
  name: isolate-tenants
spec:
  priority: 10
  subject:
    namespaces:
      matchLabels:
        tenant: team-a
  ingress:
  - name: deny-cross-tenant
    action: Deny
    from:
    - namespaces:
        matchLabels:
          tenant: team-b
```

## 运维操作

```bash
# 🟢 查看 OVN 网络拓扑
kubectl exec -n ovn-kubernetes ovnkube-db-0 -- ovn-nbctl show

# 🟢 查看 OVS 流表
kubectl exec -n ovn-kubernetes -l app=ovnkube-node -- ovs-ofctl dump-flows br-int

# 🟢 查看 EgressIP 分配
kubectl get egressip
kubectl get egressip -o yaml

# 🟡 添加节点到 EgressIP 池
kubectl label node worker-1 k8s.ovn.org/egress-assignable=""

# 🟢 查看 NetworkPolicy 对应的 OVN ACL
kubectl exec -n ovn-kubernetes ovnkube-db-0 -- ovn-nbctl acl-list <logical-switch>

# 🟡 重启 OVN 控制平面（紧急场景）
kubectl rollout restart statefulset/ovnkube-db -n ovn-kubernetes

# 🔴 重置 OVN 数据库（破坏性操作，仅灾难恢复）
kubectl delete statefulset/ovnkube-db -n ovn-kubernetes
kubectl apply -f ovn-setup.yaml
```

## 故障排查

| 症状 | 可能原因 | 排查命令 | 修复方案 |
|------|----------|----------|----------|
| Pod 网络不通 | OVS 流表异常或 Geneve 隧道断开 | `ovs-ofctl dump-flows br-int` | 重启 ovnkube-node Pod |
| EgressIP 未生效 | 节点未标记 egress-assignable | `kubectl get egressip -o yaml` | 添加节点标签 |
| NetworkPolicy 不生效 | OVN ACL 未同步 | `ovn-nbctl acl-list` | 检查 ovnkube-master 日志 |
| OVN DB 集群异常 | Raft 选举失败或节点不可达 | `kubectl logs ovnkube-db-0` | 检查节点间 6641/6642 端口 |
| Service 访问失败 | 负载均衡器配置错误 | `ovn-nbctl lb-list` | 检查 Service 和 Endpoint 状态 |

```
排查流程：
├── Pod 网络异常
│   ├── ovs-vsctl show 检查 OVS 端口
│   ├── ovs-ofctl dump-flows br-int 检查流表
│   ├── 检查 Geneve 隧道状态
│   └── 重启 ovnkube-node DaemonSet Pod
├── EgressIP/Firewall 问题
│   ├── kubectl get egressip 查看状态
│   ├── 确认节点有 egress-assignable 标签
│   ├── 检查 EgressFirewall 规则顺序
│   └── 查看 ovnkube-master 日志
└── OVN 集群问题
    ├── kubectl get pods -n ovn-kubernetes
    ├── 检查 OVN DB Raft 状态
    ├── 确认节点间 6641/6642 端口连通
    └── 查看 ovnkube-db 日志
```

## 生产案例

### 案例 1：企业多租户网络隔离

- **场景**：金融企业 K8s 集群，多团队共享，合规要求团队间网络完全隔离，且 Pod 出站需固定 IP
- **排查**：之前使用 Calico NetworkPolicy，但无法实现 EgressIP 和出站防火墙，合规审计不通过
- **方案**：迁移到 OVN-Kubernetes，使用 AdminNetworkPolicy 隔离租户，EgressIP 固定出站 IP，EgressFirewall 限制出站范围
- **效果**：合规审计通过，多租户网络隔离完整，Pod 出站 IP 固定可审计

### 案例 2：OpenShift 大规模生产网络

- **场景**：200 节点 OpenShift 集群，10000+ Pod，需要高性能 NetworkPolicy 和稳定的网络控制平面
- **排查**：之前使用 iptables 实现 NetworkPolicy，规则数量多时性能下降明显
- **方案**：使用 OVN-Kubernetes 作为默认 CNI，NetworkPolicy 通过 OVN ACL 实现，分布式路由减少网络延迟
- **效果**：NetworkPolicy 性能提升 5x，网络控制平面稳定运行 99.99% SLA

## 替代方案

| 项目 | 优势 | 劣势 | 适用场景 |
|------|------|------|----------|
| **OVN-Kubernetes** | OVN 高性能、OpenShift 验证、EgressIP | OVN 运维复杂 | 企业级网络 |
| Calico | BGP 原生、简单 | 无 Egress IP/Firewall | 简单集群网络 |
| Cilium | eBPF 高性能、可观测强 | 企业网络功能较少 | 可观测性优先 |
| Kube-OVN | VPC 多租户 | 社区较小 | 多租户 VPC |

## 架构定位

在 CNCF 生态中，OVN-Kubernetes 属于 **Networking** 类别，是 OVN 技术在 Kubernetes 上的官方实现。OpenShift 默认 CNI，已在大规模生产中验证。

## 参考链接

- [[cilium]]
- [[22-概念/03-网络/cilium-ebpf-networking.md|cilium-ebpf-networking]]
- [[23-实体/02-K8s核心组件/cni-plugins.md|cni-plugins]]
- [[23-实体/networkpolicy.md|[[networkpolicy|networkpolicy]]]]
- [[22-概念/04-存储/storage-model.md|storage-model]]

## Related

- [[43-terway-crd-operations]] — Terway CRD 资源操作
- [[sops]] — SOPS (Secrets OPerationS)
- [[23-实体/08-交付与制品/argocd.md|argocd]] — ArgoCD
- [[cni]] — CNI (Container Network Interface)
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- ovn-kubernetes
- [[23-实体/04-网络/antrea.md|Antrea]]
- [[23-实体/04-网络/kubeslice.md|KubeSlice]]
- [[23-实体/04-网络/kuadrant.md|Kuadrant]]
- [[23-实体/04-网络/kube-ovn.md|Kube-OVN]]
- [[23-实体/04-网络/easegress.md|Easegress]]
- [[23-实体/10-平台与开发工具/bpfman.md|bpfman]]
- [[23-实体/10-平台与开发工具/telepresence.md|Telepresence]]
- [[23-实体/04-网络/spiderpool.md|Spiderpool]]
- [[23-实体/04-网络/k8gb.md|K8GB]]
- [[23-实体/15-参考与索引/cncf-networking.md|CNCF 网络与服务网格项目全景]] — Cross-reference
- [[21-生态参考/03-领域索引/etcd-index.md|etcd 知识图谱索引]]
- [[21-生态参考/03-领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
