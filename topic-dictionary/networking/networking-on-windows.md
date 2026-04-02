# Networking on Windows

## 概述

Kubernetes 支持在 Windows 节点上运行工作负载，并允许与 Linux 节点混合部署在同一个集群中。Windows 容器网络通过 CNI 插件暴露，其网络模型与 Linux 有显著差异：每个容器拥有一个虚拟网卡（vNIC），连接到 Hyper-V 虚拟交换机（vSwitch），由 Host Networking Service（HNS）和 Host Compute Service（HCS）协同管理。

## 核心概念/原理

- **HNS 与 HCS**：HCS 负责容器生命周期管理，HNS 负责网络资源管理。HNS 与 vSwitch 实现了命名空间隔离，并能按需为 Pod 或容器创建虚拟网卡。
- **配置存储差异**：Linux 将 DNS、路由等网络配置存储在 `/etc` 下的文件中，而 Windows 将这些配置存储在容器独立的注册表中。因此，CNI 实现不能直接依赖文件映射（如挂载 `/etc/resolv.conf`），而必须调用 Windows API 或 HNS 来配置网络。
- **支持的 Service 类型**：在包含 Windows 节点的集群中，Service 支持 `NodePort`、`ClusterIP`、`LoadBalancer` 和 `ExternalName`。

## 关键机制或特性

- **Windows 网络驱动/模式**：
  - **L2bridge**：容器连接到外部 vSwitch，MAC 地址在出入时被重写为主机 MAC，性能最佳。需要用户定义路由（UDR）实现跨节点通信。适用于 `win-bridge`、Azure-CNI、Flannel host-gateway。
  - **L2tunnel**：Azure 专用模式，数据包发送到虚拟化主机后应用 SDN 策略。
  - **Overlay (VXLAN)**：容器通过 VXLAN 封装在隔离的 overlay 网络中通信，支持 IP 复用。适用于 `win-overlay`、Flannel VXLAN。Windows Server 2019 需安装补丁 KB4489899。
  - **Transparent**：供 `ovn-kubernetes` 使用，基于逻辑交换机和路由器实现分布式 ACL、IPAM 和负载均衡。
  - **NAT**：容器连接内部 vSwitch，由 WinNAT 提供 DHCP/DNS，通常不用于 Kubernetes。
- **Flannel on Windows**：Flannel 在 Windows 上通过 VXLAN（Beta，委托给 `win-overlay`）或 host-gateway（稳定，委托给 `win-bridge`）后端运行，配合 Flanneld 自动分配节点子网并创建 HNS 网络。
- **特性支持矩阵**：
  - **Session affinity**：Windows Server 2022 支持，通过 `service.spec.sessionAffinity: ClientIP` 启用。
  - **Direct Server Return (DSR)**：Windows Server 2019+ 支持，在容器 vSwitch 端口直接做 NAT，使回包绕过负载均衡器。通过 kube-proxy 的 `--enable-dsr=true` 启用。
  - **Client IP preservation**：Windows Server 2019+ 支持，通过 `externalTrafficPolicy: Local` 配合 DSR 保留源 IP。
  - **IPv4/IPv6 dual-stack**：Windows Server 2019+ 支持，但仅 `l2bridge` 网络模式支持双栈，Overlay 不支持。
- **已知限制**：
  - Windows 数据平面（VFP）不支持 ICMP 包转换，因此 `ping` 命令可能无法用于网络调试，建议使用 `curl` 等基于 TCP/UDP 的工具。
  - Windows 不支持 IPv6-only 单栈网络。

## 使用场景

- **混合 OS 集群**：企业现有 .NET Framework 等 Windows 应用容器化后，与 Linux 应用共存于同一 Kubernetes 集群。
- **Azure 云原生集成**：利用 Azure-CNI 的 L2tunnel 模式，使 Windows 容器直接集成 Azure 虚拟网络（vNET），使用 NSG 等 Azure 网络功能。
- **隔离网络需求**：使用 Overlay 模式将容器网络与主机底层网络隔离，提高安全性并解决数据中心 IP 不足的问题。

## 最佳实践/注意事项

- **选择合适的网络模式**：追求最佳性能时优先使用 `l2bridge`；需要网络隔离或 IP 复用时使用 Overlay（但注意 Overlay 不支持双栈）。
- **调试避免使用 ping**：由于 Windows 容器网络对 ICMP 支持有限，排查外部连通性时建议使用 `curl` 或 `Resolve-DNSName`（PowerShell）。
- **确认操作系统版本**：DSR、Session Affinity、双栈等功能对 Windows Server 版本有最低要求，部署前需核对版本和补丁。
- **注意 CNI 插件兼容性**：确保选用的 CNI 插件在 Windows 上有稳定支持，并了解其对 Service、NetworkPolicy 等功能的实现程度。

## 参考链接

- https://kubernetes.io/docs/concepts/services-networking/windows-networking/
