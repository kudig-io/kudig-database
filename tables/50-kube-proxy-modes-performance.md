# 123 - Kube-proxy 实现模式与性能

`kube-proxy` 是 Kubernetes Service 机制背后的核心实现者。它是一个运行在每个节点上的守护进程，负责监视 `Service` 和 `EndpointSlice` 对象的变化，并将网络规则写入节点，从而实现 Service 的虚拟 IP (VIP) 路由和负载均衡。

理解 `kube-proxy` 的不同工作模式对于诊断网络问题、进行性能调优以及为大规模集群选择正确的技术栈至关重要。

---

## 1. `kube-proxy` 工作模式概览

`kube-proxy` 支持多种后端模式，通过 `--proxy-mode` 标志来指定。

| 模式 | 英文名称 | 状态 | 核心技术 | 适用场景 |
|:---:|:---|:---:|:---|:---|
| **用户空间** | `userspace` | 已废弃 | 内核态到用户态的代理 | 仅用于非常古老的 K8s 版本，**生产环境严禁使用**。 |
| **IPTables** | `iptables` | 长期默认 | 内核 Netfilter `iptables` 规则 | 通用场景，功能稳定，但大规模下有性能瓶颈。 |
| **IPVS** | `ipvs` | 推荐 | 内核 Netfilter `IPVS`/`LVS` | 大规模、高性能场景，提供更优的吞吐量和扩展性。 |
| **内核空间** | `kernelspace` | 未来/部分实现 | eBPF 等内核原生技术 | 下一代数据平面技术，例如 Cilium 的实现。 |
| **Windows** | `winkernel` | 特定平台 | Windows HNS | 用于 Windows 节点。 |

---

## 2. 深入对比 `iptables` 和 `IPVS` 模式

`userspace` 模式因性能过差早已被淘汰，现代 Kubernetes 集群主要在 `iptables` 和 `IPVS` 之间选择。

### 2.1 `iptables` 模式

**工作原理**:
`iptables` 模式完全依赖内核的 `iptables` 规则来实现 Service 的路由和负载均衡。对于每个 Service，`kube-proxy` 会创建一系列 `iptables` 规则。

1.  **DNAT 规则**: 在 `PREROUTING` 链（用于外部流量）和 `OUTPUT` 链（用于集群内部流量）中，创建规则将发往 Service VIP 的流量进行目标地址转换 (DNAT)，使其指向后端的某个 Pod IP。
2.  **负载均衡**: 通过 `statistic` 模块，为每个后端 Pod 创建等概率的转发规则，从而实现随机的负载均衡。
3.  **规则同步**: `kube-proxy` 监视 `EndpointSlice` 的变化。当 Pod 增加或减少时，它会**全量刷新**相关的 `iptables` 规则链。

**优点**:
- **成熟稳定**: `iptables` 是 Linux 内核的基石，极其稳定可靠。
- **功能全面**: 兼容所有 Service 的特性。

**缺点**:
- **性能瓶颈**: 当 Service 或后端 Pod 数量巨大时（例如，超过 10,000 个 Service），`iptables` 规则链会变得非常长。
- **更新效率低**: 规则更新是**非增量**的，每次更新都需要遍历和刷新整个规则链，这在大型集群中会导致明显的延迟和 CPU 消耗。其时间复杂度为 `O(n)`，其中 `n` 是规则数量。
- **负载均衡算法有限**: 默认只支持随机负载均衡。

### 2.2 `IPVS` 模式

**工作原理**:
`IPVS` (IP Virtual Server) 是 Linux 内核中内置的高性能L4层负载均衡器，通常用于构建大家熟知的 `LVS` (Linux Virtual Server)。`kube-proxy` 的 `IPVS` 模式利用 `IPVS` 来代替 `iptables` 的大部分负载均衡功能。

1.  **IPVS Service 创建**: 对于每个 Service，`kube-proxy` 会在内核中创建一个 `IPVS` 虚拟服务器。
2.  **IPVS Real Server 创建**: 对于每个后端的 Endpoint (Pod)，`kube-proxy` 会创建一个 `IPVS` 真实服务器 (Real Server)。
3.  **高效查找**: `IPVS` 使用哈希表来存储其服务和后端映射关系，而不是像 `iptables` 那样使用线性规则链。这使得在大量 Service 和 Pod 的情况下，查找和更新的效率极高。
4.  **`iptables` 协同**: `IPVS` 模式仍然需要 `iptables` 来处理一些特定任务，例如数据包过滤、SNAT 以及处理 `NodePort` 的流量。

**优点**:
- **高性能与高扩展性**: 使用哈希表进行查找，其时间复杂度为 `O(1)`，即使在 Service 数量巨大时也能保持高性能。
- **更高的吞吐量**: 内核处理路径更短，网络吞吐量更高。
- **丰富的负载均衡算法**: 支持多种算法，如轮询 (Round Robin)、最少连接 (Least Connection)、多种哈希算法等。可以通过 `service.beta.kubernetes.io/masquerade-aware` 注解进行配置。

**缺点**:
- **依赖内核模块**: 节点必须预先加载 `IPVS` 相关的内核模块（如 `ip_vs`, `ip_vs_rr` 等）。
- **监控和调试工具链**: 相对于 `iptables`，`IPVS` 的调试工具（如 `ipvsadm`）对一些运维人员来说可能不那么熟悉。

---

## 3. 如何选择和配置

| 对比维度 | `iptables` | `IPVS` |
|:---:|:---|:---|
| **数据结构** | 线性链表 | 哈希表 |
| **时间复杂度** | `O(n)` | `O(1)` |
| **性能** | 中等，大规模下衰减 | 高 |
| **负载均衡算法** | 随机 | 轮询, 最少连接, 哈希等 |
| **适用规模** | 中小规模 (< 5000 Services) | 大规模 (>= 5000 Services) |
| **内核依赖** | 无 | 需要加载 IPVS 模块 |

### 实操：切换到 `IPVS` 模式

1.  **检查节点内核支持**:
    确保所有节点都加载了 `IPVS` 模块。
    ```bash
    # 在每个节点上执行
    lsmod | grep ip_vs
    # 如果没有输出，尝试手动加载
    modprobe -- ip_vs ip_vs_rr ip_vs_wrr ip_vs_sh
    ```
    为了持久化，需要将这些模块添加到 `/etc/modules-load.d/ipvs.conf`。

2.  **修改 `kube-proxy` 配置**:
    编辑 `kube-proxy` 的 `ConfigMap`。
    ```bash
    kubectl edit configmap kube-proxy -n kube-system
    ```
    找到 `mode` 字段，将其值从 `iptables` 修改为 `ipvs`。
    ```yaml
    # ...
    apiVersion: kubeproxy.config.k8s.io/v1alpha1
    kind: KubeProxyConfiguration
    mode: "ipvs" # <-- 修改这里
    # ...
    ```

3.  **重启 `kube-proxy` Pods**:
    删除 `kube-proxy` 的 Pods，让它们使用新的 `ConfigMap` 重启。
    ```bash
    kubectl delete pod -n kube-system -l k8s-app=kube-proxy
    ```

4.  **验证模式**:
    查看任一 `kube-proxy` Pod 的日志，确认它已在 `IPVS` 模式下运行。
    ```bash
    kubectl logs -n kube-system -l k8s-app=kube-proxy | grep "Using ipvs Proxier"
    ```
    或者使用 `ipvsadm` 查看规则。
    ```bash
    # 在任一节点上执行
    ipvsadm -Ln
    ```
    如果能看到 Service VIP 和后端 Pod IP 的列表，说明切换成功。
