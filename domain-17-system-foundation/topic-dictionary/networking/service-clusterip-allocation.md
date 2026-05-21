---
title: Service ClusterIP allocation
description: '## 概述'
category: dictionary
tags:
- k8s
- glossary
- terminology
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Service ClusterIP allocation 是什么
- 如何 Service ClusterIP allocation
trigger_keywords:
- Service
- ClusterIP
- allocation
- dictionary
prerequisites:
- kubectl-basics
- cloud-provider-basics
---

# Service ClusterIP allocation

## 概述

在 [[entities/kubernetes|kubernetes]] 中，`ClusterIP` 类型的 Service 会被分配一个集群范围内的虚拟 IP 地址，客户端通过该 IP 访问 Service，再由 Kubernetes 将流量负载均衡到后端 Pod。整个集群中，每个 Service 的 ClusterIP 必须唯一。Kubernetes 采用了一种分带（banding）分配策略，以降低用户手动指定静态 IP 与系统自动动态分配发生冲突的风险。

## 核心概念/原理

- **动态分配与静态指定**：
  - 如果不指定 `.spec.clusterIP`，Kubernetes 会从 Service IP 范围（`--service-cluster-ip-range`）中自动分配一个可用地址。
  - 用户也可以在创建 Service 时显式设置 `.spec.clusterIP`，以确保关键服务（如集群 DNS）使用固定的知名 IP。
- **冲突风险**：由于动态分配和静态指定共享同一个 IP 池，如果其他 Service 在 DNS Service 之前被动态创建，可能会占用计划中的静态 IP，导致后续创建失败。

## 关键机制或特性

- **分带策略（Banding Strategy）**：Kubernetes 将 Service IP 范围划分为**静态带（static band，低位段）**和**动态带（dynamic band，高位段）**，以公式计算分界点：
  ```
  Band Offset = min(max(16, cidrSize / 16), 256)
  ```
  即偏移量永远不会小于 16，也不会大于 256。
- **分配顺序**：
  - **动态分配默认使用 upper band**（高位段），当高位段耗尽后，才会回退使用 lower band（低位段）。
  - **静态分配建议使用 lower band**（低位段），因为动态分配优先使用高位段，从而大大降低了冲突概率。
- **示例**：
  - 对于 `10.96.0.0/24`（254 个可用 IP）：偏移量为 `min(max(16, 256/16), 256) = 16`，静态带为 `10.96.0.1` ~ `10.96.0.16`。
  - 对于 `10.96.0.0/20`（4094 个可用 IP）：偏移量为 `min(max(16, 4096/16), 256) = 256`，静态带为 `10.96.0.1` ~ `10.96.1.0`。
  - 对于 `10.96.0.0/16`（65534 个可用 IP）：偏移量为 `min(max(16, 65536/16), 256) = 256`，静态带为 `10.96.0.1` ~ `10.96.1.0`。

## 使用场景

- **为核心服务预留知名 IP**：例如将集群 DNS Service 固定为 Service IP 范围的第 10 个地址（如 `10.96.0.10`），方便其他组件和配置文件直接引用。
- **Legacy 系统集成**：某些遗留系统或外部防火墙规则已硬编码了特定 Service IP，需要 Kubernetes Service 保持该地址不变。
- **减少静态/动态分配冲突**：了解分带策略后，可以有意识地将静态分配请求放在 lower band，而将常规动态分配交给 upper band。

## 最佳实践/注意事项

- **为关键 Service 选择 lower band 地址**：若需要手动指定 ClusterIP，尽量选择静态带（lower band）内的地址，以最大限度避免与动态分配冲突。
- **提前计算静态带范围**：根据集群配置的 `--service-cluster-ip-range` CIDR 大小，使用公式 `min(max(16, cidrSize / 16), 256)` 计算出可用的静态带范围，合理规划 IP。
- **并发创建仍可能冲突**：虽然分带策略显著降低了冲突概率，但在极高并发或动态带耗尽回退到静态带时，仍可能发生冲突。若创建失败，可尝试使用其他 IP 重新创建。
- **不能修改已有 Service 的 ClusterIP**：ClusterIP 在 Service 创建后不可变更，如需更换必须删除并重建 Service。

## 生产 YAML 示例

### 静态 ClusterIP 分配（关键服务）

```yaml
# 为集群 DNS 固定 IP（通常在集群初始化时配置）
apiVersion: v1
kind: Service
metadata:
  name: kube-dns
  namespace: kube-system
spec:
  clusterIP: 10.96.0.10           # 固定在 lower band（静态带）
  selector:
    k8s-app: kube-dns
  ports:
  - name: dns
    port: 53
    protocol: UDP
  - name: dns-tcp
    port: 53
    protocol: TCP
```

### 动态 ClusterIP 分配（默认行为）

```yaml
# 不指定 clusterIP，系统自动从 upper band 分配
apiVersion: v1
kind: Service
metadata:
  name: my-app
  namespace: production
spec:
  # clusterIP 省略 → 自动分配（优先使用 upper band）
  selector:
    app: my-app
  ports:
  - port: 80
    targetPort: 8080
```

## 分带策略计算示例

```
Service CIDR: --service-cluster-ip-range

┌────────────────────────────────────────────────────────────────┐
│            Service IP Range (CIDR)                              │
│                                                                │
│  ┌─── Lower Band (静态带) ───┐  ┌─── Upper Band (动态带) ───┐  │
│  │ 静态分配优先使用此区间     │  │ 动态分配优先使用此区间     │  │
│  │ 如: DNS=10.96.0.10       │  │ 系统自动分配的 ClusterIP   │  │
│  └──────────────────────────┘  └──────────────────────────────┘  │
└────────────────────────────────────────────────────────────────┘

Band Offset = min(max(16, cidrSize / 16), 256)

CIDR 大小        可用 IP    Offset  静态带范围              动态带起始
10.96.0.0/24    254        16      10.96.0.1~10.96.0.16    10.96.0.17
10.96.0.0/20    4094       256     10.96.0.1~10.96.1.0     10.96.1.1
10.96.0.0/16    65534      256     10.96.0.1~10.96.1.0     10.96.1.1
10.96.0.0/12    1048574    256     10.96.0.1~10.96.1.0     10.96.1.1
```

## 故障排查

| 症状 | 可能原因 | 排查步骤 |
|------|----------|----------|
| Service 创建失败：ClusterIP 冲突 | 指定的 IP 已被占用 | `kubectl get svc -A -o jsonpath='{.items[*].spec.clusterIP}' \| tr ' ' '\n' \| sort` 查看已分配 IP |
| 动态带 IP 耗尽 | Service 数量过多或 CIDR 范围过小 | 评估扩大 `--service-cluster-ip-range` |
| 手动 IP 在 upper band 被占用 | 静态分配使用了动态带地址 | 静态分配应使用 lower band 地址 |
| ClusterIP 需要修改 | ClusterIP 创建后不可变 | 删除 Service 重建（注意 DNS 缓存） |

## 生产检查清单

- [ ] 关键服务（DNS、监控）使用 lower band 的固定 ClusterIP
- [ ] 常规服务使用默认动态分配
- [ ] Service CIDR 大小为集群预期 Service 数量的 4 倍以上
- [ ] 记录已使用的静态 ClusterIP 分配，避免冲突
- [ ] 双栈集群为 IPv4 和 IPv6 分别规划 CIDR

## 命令快速参考

```bash
# 查看集群 Service CIDR 配置
kubectl cluster-info dump | grep service-cluster-ip-range

# 列出所有已分配的 ClusterIP
kubectl get svc -A -o custom-columns='NAME:.metadata.name,NS:.metadata.namespace,CLUSTER-IP:.spec.clusterIP'

# 计算 lower band 范围（手动推算）
# CIDR=10.96.0.0/20 → 4094 IPs → offset=min(max(16,4096/16),256)=256
# 静态带: 10.96.0.1 ~ 10.96.1.0

# 查看 Service 的 ClusterIP
kubectl get svc <name> -o jsonpath='{.spec.clusterIP}'
```

## 交叉引用

- [Service](service.md) — Service 类型和 ClusterIP 的使用
- [DNS for Services](dns-for-services-and-pods.md) — ClusterIP 与 DNS 记录的关系
- [IPv4/IPv6 Dual Stack](ipv4-ipv6-dual-stack.md) — 双栈 Service 的 IP 分配

## 参考链接

- https://kubernetes.io/docs/concepts/services-networking/cluster-ip-allocation/
