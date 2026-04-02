# Network Policies

## 概述

NetworkPolicy 是 Kubernetes 中用于在 OSI 第 3/4 层（IP 地址和端口级别）控制流量的资源对象。它允许你精确指定 Pod 能够与哪些网络“实体”通信，包括其他 Pod、特定命名空间或特定 IP 网段。要实现 NetworkPolicy，集群必须部署支持该功能的 CNI 网络插件。

## 核心概念/原理

- **Pod 隔离模型**：默认情况下，Pod 对入站和出站流量都是“非隔离”的（即全部放行）。一旦存在某个 NetworkPolicy 同时选中了该 Pod 并包含相应的 `policyTypes`（Ingress 和/或 Egress），Pod 即进入隔离状态。此时，只有被显式允许的流量才能通过，其他流量默认被拒绝。
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

## 参考链接

- https://kubernetes.io/docs/concepts/services-networking/network-policies/
