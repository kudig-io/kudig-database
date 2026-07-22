---
title: Spiderpool (entities)
description: '## 概述'
summary: 'Spiderpool 是一个 Kubernetes 的 Underlay 网络 IPAM (IP Address Management) 解决方案，专为数据中心和云原生环境设计。它支持固定 IP、多网卡、双栈网络等高级特性，能够与多种 CNI 插件无缝集成，特别适合需要 Pod 与物理网络直接通信的场景。'
category: entities
tags:
- k8s
- cncf
- networking
- spiderpool
- cilium
- crd
- operator
- wasm
tier: supporting
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Spiderpool 是什么
- 如何 Spiderpool
trigger_keywords:
- Spiderpool
prerequisites:
- kubectl-basics
- cilium-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# Spiderpool

> **CNCF 状态**: Sandbox | **类别**: Networking | **主要语言**: Go

## 概述

SpiderPool 是一个 CNCF 沙箱项目，由 DaoCloud 开源，是 Kubernetes 下的 Underlay 网络和 IPAM（IP Address Management）解决方案。它为 K8s Pod 提供固定的 Underlay IP 地址分配能力，支持多网卡、多 CNI 协同工作。SpiderPool 特别适合需要固定 IP 的场景（如传统应用迁移、网络设备对接、跨子网通信），支持 VLAN、BGP、SR-IOV 等多种网络模式。它与 Calico、Cilium、Multus 等主流 CNI 配合使用。

## Key Features（核心能力）

- **固定 IP 分配**：为 StatefulSet 和 Deployment 的 Pod 分配固定的 Underlay IP
- **多网卡支持**：支持 Pod 多网卡，每张网卡独立 IPAM
- **多 CNI 协同**：与 Calico、Cilium、Macvlan、SR-IOV 等 CNI 协同
- **IP 预留和回收**：支持 IP 预留（不分配给新 Pod）和自动回收
- **Subnet CRD**：通过 SpiderSubnet CRD 声明式管理 IP 子网
- **VLAN/RDMA 支持**：支持 VLAN 网络和 RDMA 网络配置

## 架构与工作原理

SpiderPool 由 Spiderpool Controller 和 IPAM 插件组成。Controller 管理 SpiderSubnet 和 SpiderReservedIP CRD，协调 IP 池的分配和回收。IPAM CNI 插件在 Pod 创建时从对应的 SpiderSubnet 分配 IP，记录到 SpiderIPPool CRD 中。多 CNI 场景下，通过 Multus 编排多个 CNI 插件，SpiderPool 作为 IPAM 插件为每个接口分配 Underlay IP。

## K8s 集成

SpiderPool 通过丰富的 CRD 与 Kubernetes 集成。SpiderSubnet CRD 定义 IP 子网范围和网关配置。SpiderIPPool CRD 记录已分配的 IP 和关联的 Pod。SpiderMultusConfig CRD 管理 Multus 网络附件配置。通过 DaemonSet 部署 Spiderpool Agent 到每个节点执行 CNI 插件逻辑。与 K8s StatefulSet 集成时，Pod 重建后获得相同 IP。

## 生产用例

- **传统应用迁移**：需要固定 IP 的遗留应用迁移到 K8s
- **多 CNI 网络**：Underlay + Overlay 混合网络（如 Calico Overlay + Macvlan Underlay）
- **网络策略合规**：防火墙规则需要固定 IP 的安全合规场景
- **RDMA/GPU 网络**：AI 训练集群的 RDMA 网络配置

## 安装与配置

```bash
# 🟢 Helm 安装
helm repo add spiderpool https://spidernet-io.github.io/spiderpool
helm install spiderpool spiderpool/spiderpool \
  -n kube-system \
  --set spiderpoolAgent.mode=overlay+underlay

# 🟢 验证安装
kubectl get pods -n kube-system -l app=spiderpool
kubectl get crd | grep spiderpool

# 🟢 查看 SpiderSubnet
kubectl get spidersubnet

# 🟢 查看 SpiderIPPool
kubectl get spiderippool -A
```

### SpiderSubnet CRD 示例

```yaml
apiVersion: spiderpool.spidernet.io/v2beta1
kind: SpiderSubnet
metadata:
  name: vlan100-subnet
spec:
  subnet: 192.168.100.0/24
  gateway: 192.168.100.1
  vlan: 100
  ips:
  - 192.168.100.10-192.168.100.200
  excludeIPs:
  - 192.168.100.100-192.168.100.110  # 预留 IP
  routes:
  - dst: 10.0.0.0/8
    gw: 192.168.100.1
```

### Pod 固定 IP 配置

```yaml
apiVersion: apps/v1
kind: StatefulSet
metadata:
  name: database
spec:
  serviceName: database-headless
  replicas: 3
  selector:
    matchLabels:
      app: database
  template:
    metadata:
      labels:
        app: database
      annotations:
        # 指定使用 Spiderpool IPAM
        ipam.spidernet.io/ippool: '["vlan100-subnet"]'
        # 固定 IP (StatefulSet 默认行为)
        ipam.spidernet.io/ip-retention: "true"
    spec:
      containers:
      - name: postgres
        image: postgres:16
        ports:
        - containerPort: 5432
```

### 多网卡配置 (Multus)

```yaml
apiVersion: spiderpool.spidernet.io/v2beta1
kind: SpiderMultusConfig
metadata:
  name: underlay-net
  namespace: kube-system
spec:
  cniType: macvlan
  enableIsolatedDevice: true
  master:
    name: eth0
    type: auto
  ipamType: spiderpool
  ipam:
    ipv4:
    - subnet: vlan100-subnet
---
# Pod 多网卡注解
apiVersion: v1
kind: Pod
metadata:
  annotations:
    k8s.v1.cni.cncf.io/networks: underlay-net
spec:
  containers:
  - name: app
    image: myapp:latest
```

## 运维操作

### 常用命令

```bash
# 🟢 查看子网状态
kubectl get spidersubnet
kubectl describe spidersubnet vlan100-subnet

# 🟢 查看 IP 分配状态
kubectl get spiderippool -A
kubectl describe spiderippool <pool-name> -n <namespace>

# 🟢 查看预留 IP
kubectl get spiderreservedip -A

# 🟢 查看 Controller 日志
kubectl logs -n kube-system -l app=spiderpool-controller --tail=50

# 🟢 查看 Agent 日志
kubectl logs -n kube-system -l app=spiderpool-agent --tail=50

# 🟡 释放指定 IP
kubectl delete spiderippool <pool-name> -n <namespace>

# 🟡 创建预留 IP
kubectl apply -f - <<EOF
apiVersion: spiderpool.spidernet.io/v2beta1
kind: SpiderReservedIP
metadata:
  name: reserve-for-vip
spec:
  ips:
  - 192.168.100.200
  - 192.168.100.201
EOF
```

## 故障排查

| 症状 | 可能原因 | 排查命令 | 修复方案 |
|------|----------|----------|----------|
| Pod 无法获取 IP | 子网 IP 耗尽 | `kubectl describe spidersubnet <name>` | 扩展子网或回收 IP |
| 固定 IP 未生效 | 注解配置错误 | `kubectl get pod -o yaml` | 检查 ipam 注解 |
| 多网卡不工作 | Multus 未安装/配置错误 | `kubectl get spidermultusconfig` | 检查 Multus 和 CNI 配置 |
| IP 冲突 | 子网重叠/手动分配 | `kubectl get spiderippool -A` | 检查子网范围不重叠 |
| Agent 未就绪 | 节点 CNI 配置错误 | `kubectl logs -l app=spiderpool-agent` | 检查 /etc/cni/net.d 配置 |

### 排查流程

```
1. kubectl get spidersubnet → 确认子网状态和可用 IP
2. kubectl get spiderippool -A → 查看 IP 分配情况
3. kubectl describe pod <name> → 查看网络相关 Events
4. kubectl logs -l app=spiderpool-agent → 查看节点 Agent 日志
5. 检查节点 CNI 配置文件
```

## 生产案例

### 案例1: 传统应用固定 IP 迁移
- **场景**: 50+ 传统应用依赖固定 IP，防火墙规则绑定 IP
- **方案**: Spiderpool 为 StatefulSet Pod 分配固定 Underlay IP
- **效果**: 无需修改防火墙规则，应用无缝迁移到 K8s

### 案例2: AI 训练 RDMA 网络
- **场景**: GPU 训练集群需要 RDMA 网络加速
- **方案**: Spiderpool + SR-IOV + Multus，为训练 Pod 分配 RDMA IP
- **效果**: 训练通信延迟降低 80%，GPU 利用率提升 25%

## 对比替代方案

| 维度 | Spiderpool | Whereabouts | Calico IPAM | 手动分配 |
|------|-----------|-------------|-------------|----------|
| 固定 IP | 支持 | 支持 | 不支持 | 支持 |
| 多网卡 | 支持 | 有限 | 不支持 | 复杂 |
| Underlay | 支持 | 支持 | 不支持 | 支持 |
| VLAN | 支持 | 有限 | 不支持 | 支持 |
| CRD 管理 | 丰富 | 基本 | 基本 | 无 |
| CNCF | Sandbox | 非 CNCF | Graduated | N/A |

## 检查清单

- [ ] SpiderSubnet IP 范围充足
- [ ] 排除已使用的 IP (excludeIPs)
- [ ] 网关和路由配置正确
- [ ] Multus 已安装 (多网卡场景)
- [ ] 节点 CNI 配置正确
- [ ] 监控 IP 池使用率
- [ ] 配置了 IP 回收策略

## Related

- [[openfunction]] — OpenFunction
- [[kubevirt]] — KubeVirt
- [[wasmcloud]] — wasmCloud
- [[cni]] — CNI (Container Network Interface)
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- spiderpool
- [[实体/cncf-networking.md|CNCF 网络与服务网格项目全景]] — Cross-reference
- [[生态参考/领域索引/etcd-index.md|etcd 知识图谱索引]]
- [[生态参考/领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
