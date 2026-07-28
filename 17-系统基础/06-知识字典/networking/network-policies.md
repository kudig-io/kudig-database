---
title: Network Policies
description: '## 概述'
summary: '## 概述'
category: dictionary
tags:
- k8s
- glossary
- terminology
- cilium
- flannel
- calico
- ingress
- networkpolicy
- ebpf
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Network Policies 是什么
- 如何 Network Policies
trigger_keywords:
- Network
- Policies
- dictionary
prerequisites:
- kubectl-basics
- cloud-provider-basics
- ebpf-basics
- cilium-basics
- cni-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Network Policies

## 概述

[[networkpolicy|NetworkPolicy]] 是 [[kubernetes|Kubernetes]] 中用于在 OSI 第 3/4 层（IP 地址和端口级别）控制流量的资源对象。它允许你精确指定 Pod 能够与哪些网络“实体”通信，包括其他 Pod、特定命名空间或特定 IP 网段。要实现 NetworkPolicy，集群必须部署支持该功能的 CNI 网络插件。

## 核心概念/原理

- **Pod 隔离模型**：默认情况下，Pod 对入站和出站流量都是“非隔离”的（即全部放行）。一旦存在某个 NetworkPolicy 同时选中了该 Pod 并包含相应的 `policyTypes`（[[ingress|Ingress]] 和/或 Egress），Pod 即进入隔离状态。此时，只有被显式允许的流量才能通过，其他流量默认被拒绝。
- **规则叠加（Additive）**：多个 NetworkPolicy 之间不会冲突，而是叠加生效。对于某个 Pod 的某个方向（入站或出站），所有适用策略允许流量的并集即为最终允许集合，策略顺序不影响结果。
- **双向放行原则**：从源 Pod 到目标 Pod 的连接，必须同时被源 Pod 的出站策略和目标 Pod 的入站策略允许，连接才能建立。
- **选择器（Selectors）**：
  - `podSelector`：选择同一命名空间内的特定 Pod。
  - `namespaceSelector`：选择特定命名空间内的所有 Pod。
  - `podSelector + namespaceSelector`（同一列表项内）：选择特定命名空间中的特定 Pod。
  - `ipBlock`：基于 CIDR 选择 IP 范围，支持 `except` 排除子网。

## 关键机制或特性

- **端口范围支持（endPort）**：自 v1.25 起稳定，可在规则中指定连续的端口范围（`port` ~ `endPort`），简化多端口服务的策略配置。
- **按命名空间名称选择**：NetworkPolicy 不能直接按名称选择命名空间，但可利用控制平面自动设置的标签 `kubernetes.io/metadata.name=<namespace-name>` 实现。
- **Pod 生命周期与生效延迟**：新创建的网络插件可能需要一定时间处理 NetworkPolicy。如果 Pod 在策略处理完成前启动，可能短暂处于无保护状态。建议通过 init container 等待必要网络连通性，增强启动韧性。
- **hostNetwork Pod**：对 `hostNetwork: true` 的 Pod，NetworkPolicy 行为由具体 CNI 实现定义。大多数实现会将其流量视为节点流量，不应用 `podSelector`/`namespaceSelector`，但可通过 `ipBlock` 规则放行。
- **不支持的能力**：NetworkPolicy 无法做 TLS 处理、L7 控制、节点级策略、按 Service 名称选择、显式拒绝规则、日志记录或策略请求（Policy Request）等。

## 使用场景

- **数据库访问控制**：只允许带有 `role=frontend` 标签的 Pod 访问数据库 Pod 的特定端口。
- **命名空间级默认拒绝（Default Deny）**：为命名空间配置默认拒绝所有入站或出站流量，再按需添加放行规则，构建零信任网络。
- **限制外部访问**：仅允许 Pod 访问特定的外部 IP 网段（如企业内网或第三方 API 地址）。
- **网络分段与合规**：在多租户或受监管环境中，通过网络策略实现工作负载间的最小权限通信。

## 最佳实践/注意事项

- **采用默认拒绝策略**：在生产环境中，建议先为命名空间创建 `default-deny-ingress` 和/或 `default-deny-egress` 策略，再逐步添加必要的放行规则。
- **注意生效时序**：策略变更与 Pod 标签变更对已有连接的影响由实现定义，建议避免在活跃连接期间修改策略或标签。
- **hostNetwork 需谨慎**：由于实现差异大，使用 hostNetwork 的 Pod 不应依赖 NetworkPolicy 进行严格隔离。
- **CNI 兼容性**：并非所有 CNI 都完整支持 `endPort`、SCTP 等功能，使用前需确认插件版本和兼容性。
- **不能替代防火墙/WAF**：NetworkPolicy 仅作用于 L3/L4，对于应用层安全需求，应结合 Ingress Controller、Service Mesh 或外部防火墙。

## 生产 YAML 示例

### 命名空间默认拒绝（零信任基线）

```yaml
# 1. 默认拒绝所有入站流量
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: default-deny-ingress
  namespace: production
spec:
  podSelector: {}                  # 选中命名空间内所有 Pod
  policyTypes:
  - Ingress                        # 所有入站流量被拒绝
---
# 2. 默认拒绝所有出站流量
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: default-deny-egress
  namespace: production
spec:
  podSelector: {}
  policyTypes:
  - Egress
---
# 3. 允许 DNS 出站（必须，否则服务发现失效）
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: allow-dns-egress
  namespace: production
spec:
  podSelector: {}
  policyTypes:
  - Egress
  egress:
  - to:
    - namespaceSelector:
        matchLabels:
          kubernetes.io/metadata.name: kube-system
      podSelector:
        matchLabels:
          k8s-app: kube-dns
    ports:
    - protocol: UDP
      port: 53
    - protocol: TCP
      port: 53
```

### 精细化访问控制

```yaml
# 允许 frontend Pod 访问 backend Pod 的 8080 端口
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: allow-frontend-to-backend
  namespace: production
spec:
  podSelector:
    matchLabels:
      role: backend
  policyTypes:
  - Ingress
  ingress:
  - from:
    - podSelector:
        matchLabels:
          role: frontend
    # 同一命名空间内
    ports:
    - protocol: TCP
      port: 8080
---
# 允许 backend Pod 访问 database Pod（跨命名空间）
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: allow-backend-to-db
  namespace: database
spec:
  podSelector:
    matchLabels:
      role: database
  policyTypes:
  - Ingress
  ingress:
  - from:
    - namespaceSelector:
        matchLabels:
          kubernetes.io/metadata.name: production
      podSelector:
        matchLabels:
          role: backend
    ports:
    - protocol: TCP
      port: 5432
---
# 限制出站到特定外部 IP 范围
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: restrict-egress-external
  namespace: production
spec:
  podSelector:
    matchLabels:
      role: backend
  policyTypes:
  - Egress
  egress:
  - to:
    - ipBlock:
        cidr: 10.0.0.0/8           # 内网
    - ipBlock:
        cidr: 203.0.113.0/24       # 特定外部 API
        except:
        - 203.0.113.128/25         # 排除部分子网
    ports:
    - protocol: TCP
      port: 443
```

## 策略叠加规则图解

```
Pod "backend" 被以下策略选中：

Policy A: 允许来自 frontend 的 TCP:8080
Policy B: 允许来自 monitoring 的 TCP:9090

最终入站允许集合 = Policy A ∪ Policy B
= 允许 frontend:8080 OR monitoring:9090
（两个策略的允许规则取并集）
```

## 故障排查

| 症状 | 可能原因 | 排查步骤 |
|------|----------|----------|
| 所有 Pod 通信正常（策略未生效） | CNI 不支持 NetworkPolicy | 确认 CNI 支持（Calico/Cilium 支持，Flannel 不支持） |
| 部署策略后 Pod 间通信中断 | default-deny 策略生效但缺少必要的放行规则 | 检查是否遗漏了 DNS 出站放行规则 |
| 跨命名空间访问失败 | `namespaceSelector` 标签不匹配 | `kubectl get ns --show-labels` 确认目标命名空间标签 |
| ipBlock 规则不生效 | CIDR 范围不正确或被更高优先级规则覆盖 | 策略是叠加的，检查所有适用策略的并集 |
| hostNetwork Pod 不受策略控制 | 大多数 CNI 对 hostNetwork Pod 的策略支持有限 | 使用 ipBlock 替代 podSelector |

## 生产检查清单

- [ ] CNI 插件支持 NetworkPolicy（Calico、Cilium、Antrea 等）
- [ ] 生产命名空间部署 default-deny-ingress 和 default-deny-egress
- [ ] DNS 出站规则已添加（否则服务发现失效）
- [ ] 策略经过 audit/dry-run 模式验证后再启用
- [ ] 不依赖 NetworkPolicy 隔离 hostNetwork Pod
- [ ] 定期审查策略规则，清理过时条目

## 命令快速参考

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 查看命名空间内的 NetworkPolicy
kubectl get networkpolicies -n production

# 查看策略详情
kubectl describe networkpolicy allow-frontend-to-backend -n production

# 检查命名空间标签（用于 namespaceSelector）
kubectl get ns --show-labels

# 测试连通性（从 frontend Pod 到 backend）
kubectl exec -n production <frontend-pod> -- curl -s --connect-timeout 3 http://backend:8080

# 使用 Cilium 查看策略是否生效
cilium policy get -n production
cilium monitor --type drop         # 查看被丢弃的流量
```
## 交叉引用

- [Service](service.md) — Service 的端口映射和流量路由
- [eBPF 与 Cilium](ebpf-and-cilium-networking.md) — Cilium NetworkPolicy 和身份微分段
- [Cluster Networking](cluster-networking.md) — CNI 插件对 NetworkPolicy 的支持情况
- [Service Mesh](service-mesh.md) — L7 级别的访问控制和 mTLS

## 参考链接

- https://kubernetes.io/docs/concepts/services-networking/network-policies/

## Related

- [[21-生态参考/03-领域索引/service-mesh-index.md|Service Mesh 服务网格知识图谱索引]]
- [[21-生态参考/03-领域索引/security-index.md|Security 安全知识图谱索引]]


<!-- risk-assessed -->
