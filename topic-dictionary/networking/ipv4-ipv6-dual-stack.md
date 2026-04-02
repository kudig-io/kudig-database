# IPv4/IPv6 dual-stack

## 概述

Kubernetes 支持为 Pod 和 Service 同时分配 IPv4 与 IPv6 地址，实现双栈（Dual-Stack）网络。自 v1.21 起，IPv4/IPv6 双栈默认启用，允许集群中的工作负载通过两种协议族同时进行通信，包括集群内部 Service 访问和 Pod 的集群外出网流量。

## 核心概念/原理

- **双栈 CIDR 配置**：需要在集群各核心组件中同时指定 IPv4 和 IPv6 的 CIDR 范围：
  - `kube-apiserver`：`--service-cluster-ip-range=<IPv4 CIDR>,<IPv6 CIDR>`
  - `kube-controller-manager`：`--cluster-cidr=<IPv4 CIDR>,<IPv6 CIDR>`、`--service-cluster-ip-range=<IPv4 CIDR>,<IPv6 CIDR>`，以及 `--node-cidr-mask-size-ipv4`（默认 /24）和 `--node-cidr-mask-size-ipv6`（默认 /64）
  - `kube-proxy`：`--cluster-cidr=<IPv4 CIDR>,<IPv6 CIDR>`
  - `kubelet`：`--node-ip=<IPv4 IP>,<IPv6 IP>`（裸金属节点必需）
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

## 参考链接

- https://kubernetes.io/docs/concepts/services-networking/dual-stack/
