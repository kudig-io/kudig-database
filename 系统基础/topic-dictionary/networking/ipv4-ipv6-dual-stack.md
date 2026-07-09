---
title: IPv4/IPv6 dual-stack
description: '## 概述'
summary: '## 概述'
category: dictionary
tags:
- k8s
- glossary
- terminology
- apiserver
- kubelet
- controller-manager
- cilium
- calico
- agent
tier: supporting
created: 2026-05
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- IPv4/IPv6 dual-stack 是什么
- 如何 IPv4/IPv6 dual-stack
trigger_keywords:
- IPv4
- IPv6
- dual-stack
- dictionary
prerequisites:
- kubectl-basics
- cloud-provider-basics
- cilium-basics
- cni-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# IPv4/IPv6 dual-stack

## 概述

[[entities/kubernetes.md|[[Kubernetes|kubernetes]]]] 支持为 Pod 和 [[Service|Service]] 同时分配 IPv4 与 IPv6 地址，实现双栈（Dual-Stack）网络。自 v1.21 起，IPv4/IPv6 双栈默认启用，允许集群中的工作负载通过两种协议族同时进行通信，包括集群内部 Service 访问和 Pod 的集群外出网流量。

## 核心概念/原理

- **双栈 CIDR 配置**：需要在集群各核心组件中同时指定 IPv4 和 IPv6 的 CIDR 范围：
  - `kube-apiserver`：`--service-cluster-ip-range=<IPv4 CIDR>,<IPv6 CIDR>`
  - `kube-controller-manager`：`--cluster-cidr=<IPv4 CIDR>,<IPv6 CIDR>`、`--service-cluster-ip-range=<IPv4 CIDR>,<IPv6 CIDR>`，以及 `--node-cidr-mask-size-ipv4`（默认 /24）和 `--node-cidr-mask-size-ipv6`（默认 /64）
  - `kube-proxy`：`--cluster-cidr=<IPv4 CIDR>,<IPv6 CIDR>`
  - `[[kubelet|kubelet]]`：`--node-ip=<IPv4 IP>,<IPv6 IP>`（裸金属节点必需）
- **Service 地址族策略（ipFamilyPolicy）**：
  - `SingleStack`：单栈，仅分配第一个配置的 service-cluster-ip-range 的地址。
  - `PreferDualStack`：优先双栈，在双栈启用时分配 IPv4 和 IPv6 地址；若不支持则回退到单栈。
  - `RequireDualStack`：强制双栈，若无法分配两种地址则 Service 创建失败。
- **ipFamilies 字段**：显式指定 Service 的地址族顺序，如 `["IPv4"]`, `["IPv6"]`, `["IPv4","IPv6"]` 或 `["IPv6","IPv4"]`。第一个元素决定 `.spec.clusterIP` 的值。该字段对已有 Service 是条件可变的：可增删次要地址族，但不能更改主地址族。

## 关键机制或特性

- **双栈 Pod 网络**：每个 Pod 可同时获得一个 IPv4 和一个 IPv6 地址。
- **双栈 Service**：普通 Service、Headless Service 和 LoadBalancer 类型 Service 均可配置为双栈。使用 LoadBalancer 时，需确保云厂商支持 IPv4/IPv6 负载均衡器。
- **已有 Service 的默认行为**：在现有集群上启用双栈后，已有 Service 的控制平面会自动将其 `ipFamilyPolicy` 设为 `SingleStack`，`ipFamilies` 设为其现有地址族，保持向后兼容。
- **单栈与双栈切换**：可通过修改 Service 的 `ipFamilyPolicy` 字段，在 `SingleStack` 与 `PreferDualStack`/`RequireDualStack` 之间切换，系统会自动分配或回收相应地址族的 ClusterIP。
- **Headless Service（无 selector）**：若未显式设置 `ipFamilyPolicy`，默认策略为 `RequireDualStack`。
- **Windows 支持**：Windows 节点不支持 IPv6-only 单栈，但支持 IPv4/IPv6 双栈（仅 `l2bridge` 网络模式）。Windows 的 Overlay (VXLAN) 网络不支持双栈。

## 使用场景

- **同时支持 IPv4 和 IPv6 客户端**：面向公网的服务需要同时兼容传统 IPv4 用户和新兴 IPv6 用户。
- **特定合规与网络要求**：部分企业或政府机构要求内部网络具备原生 IPv6 支持。
- **未来网络演进**：为应用提前布局双栈能力，避免未来大规模迁移改造。

## 最佳实践/注意事项

- **确保全栈兼容性**：在启用双栈前，需确认 CNI 插件、云厂商、操作系统及负载均衡器均支持 IPv6 和双栈配置。
- **升级现有集群**：升级到支持双栈的版本后，已有 Service 默认保持单栈。如需双栈能力，需手动将 `ipFamilyPolicy` 改为 `PreferDualStack` 或 `RequireDualStack`。
- **IPv6 出网注意**：若 Pod 使用非公网路由的 IPv6 地址，需配置透明代理或 IP 伪装（如 ip-masq-agent）才能访问外部 IPv6 互联网。
- **LoadBalancer 双栈限制**：云厂商必须同时支持 IPv4 和 IPv6 的外部负载均衡器，否则双栈 LoadBalancer Service 可能无法正确创建。
- **避免随意更改主地址族**：修改 `ipFamilies` 时只能增删次要地址族，无法更改第一个元素（主地址族），规划时需提前确定。

## 生产 YAML 示例

### 双栈 Service 配置

```yaml
# PreferDualStack：优先双栈，不支持时回退单栈
apiVersion: v1
kind: Service
metadata:
  name: web-app
  namespace: production
spec:
  ipFamilyPolicy: PreferDualStack
  ipFamilies:
  - IPv4                           # 主地址族
  - IPv6                           # 次要地址族
  selector:
    app: web
  ports:
  - port: 80
    targetPort: 8080
# 结果：分配一个 IPv4 和一个 IPv6 ClusterIP
---
# RequireDualStack：强制双栈
apiVersion: v1
kind: Service
metadata:
  name: api-server
  namespace: production
spec:
  ipFamilyPolicy: RequireDualStack
  ipFamilies:
  - IPv6                           # 主地址族为 IPv6
  - IPv4
  type: LoadBalancer
  selector:
    app: api
  ports:
  - port: 443
    targetPort: 8443
```

### 集群组件双栈配置参考

```bash
# kube-apiserver
--service-cluster-ip-range=10.96.0.0/16,fd00:10:96::/112

# kube-controller-manager
--cluster-cidr=10.244.0.0/16,fd00:10:244::/48
--service-cluster-ip-range=10.96.0.0/16,fd00:10:96::/112
--node-cidr-mask-size-ipv4=24
--node-cidr-mask-size-ipv6=64

# kube-proxy
--cluster-cidr=10.244.0.0/16,fd00:10:244::/48

# kubelet（裸金属节点必需）
--node-ip=192.168.1.10,fd00:192:168:1::10
```

## 故障排查

| 症状 | 可能原因 | 排查步骤 |
|------|----------|----------|
| Service 创建失败：RequireDualStack 不满足 | 集群未配置双栈 CIDR | 检查 apiserver 的 `--service-cluster-ip-range` 是否包含两个 CIDR |
| Pod 只有一个 IP | CNI 不支持双栈 | `kubectl get pod -o jsonpath='{.status.podIPs}'` 确认；升级 CNI |
| IPv6 出站不通 | 缺少 IPv6 NAT/伪装配置 | 部署 ip-masq-agent 或配置透明代理 |
| 双栈 LoadBalancer 失败 | 云厂商不支持 IPv6 LB | 确认云厂商 LB 的双栈支持；考虑拆分为两个 Service |
| 修改 ipFamilies 报错 | 尝试更改主地址族 | 只能增删次要地址族，不能更改第一个元素 |

## 生产检查清单

- [ ] 所有组件（apiserver、controller-manager、kube-proxy、kubelet）配置双栈 CIDR
- [ ] CNI 插件支持双栈（Calico、Cilium 等）
- [ ] 云 LB 支持 IPv4/IPv6（使用 LoadBalancer 时）
- [ ] IPv6 出站配置 ip-masq-agent（非公网路由地址时）
- [ ] Windows 节点仅使用 l2bridge 网络模式（Overlay 不支持双栈）
- [ ] 升级现有集群后手动将需要双栈的 Service 修改为 `PreferDualStack`

## 命令快速参考

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 查看 Pod 双栈 IP
kubectl get pod <name> -o jsonpath='{.status.podIPs}'

# 查看 Service 的 ClusterIPs
kubectl get svc <name> -o jsonpath='{.spec.clusterIPs}'

# 查看 Service ipFamilyPolicy
kubectl get svc <name> -o jsonpath='{.spec.ipFamilyPolicy} {.spec.ipFamilies}'

# 修改现有 Service 为双栈
kubectl patch svc <name> -p '{"spec":{"ipFamilyPolicy":"PreferDualStack","ipFamilies":["IPv4","IPv6"]}}'

# 测试 IPv6 连通性
kubectl exec <pod> -- curl -6 http://[<ipv6-addr>]:80
```
## 交叉引用

- [Service](service.md) — Service 类型和 ClusterIP 分配
- [Service ClusterIP Allocation](service-clusterip-allocation.md) — 双栈下的 IP 分配策略
- [Cluster Networking](cluster-networking.md) — 集群网络类型和 CIDR 规划
- [Networking on Windows](networking-on-windows.md) — Windows 双栈限制

## 参考链接

- https://kubernetes.io/docs/concepts/services-networking/dual-stack/

## Related

- [[系统基础/topic-dictionary/networking/aeraki-mesh.md|Aeraki Mesh 七层网格]]
- [[系统基础/topic-dictionary/networking/akri.md|Akri 边缘设备发现]]
- [[系统基础/topic-dictionary/networking/antrea.md|Antrea 网络方案]]


<!-- risk-assessed -->
