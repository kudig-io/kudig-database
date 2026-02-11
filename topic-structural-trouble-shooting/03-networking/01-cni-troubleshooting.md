# CNI 网络插件故障排查指南

> **适用版本**: Kubernetes v1.25 - v1.32 | **最后更新**: 2026-01 | **难度**: 高级

---

## 目录

1. [问题现象与影响分析](#1-问题现象与影响分析)
2. [排查方法与步骤](#2-排查方法与步骤)
3. [解决方案与风险控制](#3-解决方案与风险控制)

---

## 🎯 本文档价值

| 读者对象 | 价值体现 |
| :--- | :--- |
| **初学者** | 理解 Pod 网络是如何从无到有构建的，学会检查 CNI 配置文件和二进制插件，掌握解决“Pod 无 IP”或“Pod 间无法 Ping 通”的基础技能。 |
| **资深专家** | 深入剖析 VXLAN/IPIP 封装原理、BGP 路由分发机制、eBPF（Cilium）对内核协议栈的加速优化，以及在大规模集群下的 IPAM 地址池管理和跨可用区网络延迟调优。 |

---

## 0. 10 分钟快速诊断

1. **组件存活**：`kubectl get pods -n kube-system -l k8s-app=calico-node`/`-l app=flannel`/`-l k8s-app=cilium`，若异常先看对应日志。
2. **CNI 配置完整性**：节点上检查 `/etc/cni/net.d/` 与 `/opt/cni/bin/` 是否匹配版本、文件未损坏。
3. **Pod IP 分配**：`kubectl get pods -A -o wide | head` 查看是否出现无 IP/重复 IP；CNI 日志搜索 `IPAM`/`no available IPs`。
4. **路由/封装**：`ip route`、`bridge fdb show`、`tcpdump -i eth0 udp port 4789` 验证 VXLAN；BGP 场景检查 `bird`/`calico-node` 路由。
5. **MTU 与分片**：对大包探测 `ping -M do -s 1472 <pod-ip>`，若不通需调小 Pod MTU。
6. **跨节点连通**：在不同节点 Pod 之间 `ping`/`curl`，结合 `ip route get` 确认路径正确。
7. **快速缓解**：
   - IPAM 耗尽：扩展地址池或回收泄露 IP。
   - 组件异常：滚动重启 CNI DaemonSet，避免单节点规则不同步。
   - 网络抖动：先降低变更频率，避免大量 Pod 同时创建/删除导致 FDB/ARP 抖动。
8. **证据留存**：保存 CNI 日志、节点路由/ARP/FDB 快照、失败的连通性测试结果。

---

## 1. 核心原理解析：CNI 的生命周期

### 1.1 Pod 网络的“诞生”过程

当 kubelet 创建 Pod 时，它通过 CNI 接口调用网络插件：
1. **ADD 操作**：kubelet 调用 CNI 二进制文件，传入 Pod 的命名空间路径和容器 ID。
2. **接口创建**：CNI 插件在宿主机创建 `veth pair` 或 `ipvlan` 接口，将一端塞入 Pod 命名空间。
3. **地址分配 (IPAM)**：调用 IPAM 插件（如 host-local）从预设的子网池中划拨一个 IP 给 Pod。
4. **路由设置**：在宿主机和 Pod 内配置路由表，确保流量能送达目标子网。

### 1.2 生产环境典型“断网坑”

1. **MTU 不匹配导致的大包丢弃**：
   - **现象**：`ping` 小包通，但 `curl` 大网页或传输大文件时连接超时。
   - **深层原因**：Overlay 网络（VXLAN/IPIP）增加了报文头，若 Pod 内 MTU 与宿主机一致，会导致报文超过物理链路 MTU 且设置了不分片位，从而被中间路由器丢弃。
   - **对策**：根据封装类型调小 Pod MTU（如 VXLAN 设为 1450）。
2. **ARP 表爆满或 FDB 同步延迟**：
   - **现象**：跨节点通信间歇性中断，重启 CNI 插件瞬间恢复。
   - **对策**：在大规模集群中调优内核参数 `net.ipv4.neigh.default.gc_thresh3`。

### 1.3 专家观测工具链（Expert's Toolbox）

```bash
# 专家级：在不进入 Pod 的情况下抓取 Pod 网卡流量
# 先通过 crictl 找到 PID，再使用 nsenter
pid=$(crictl inspect <container-id> | jq '.info.pid')
nsenter -t $pid -n tcpdump -i eth0 -nn

# 专家级：检查内核路由决策路径
ip route get <TargetPodIP> from <SourcePodIP> iif <VethName>

# 专家级：验证 VXLAN 封装报文
tcpdump -i eth0 udp port 4789 -vv -X
```

---

## 第一部分：问题现象与影响分析

### 1.1 常见问题现象

#### 1.1.1 CNI 插件不可用

| 现象 | 报错信息 | 报错来源 | 查看方式 |
|------|----------|----------|----------|
| CNI 插件未安装 | `network plugin is not ready: cni config uninitialized` | kubelet | kubelet 日志 |
| CNI 配置错误 | `error parsing CNI config` | kubelet | kubelet 日志 |
| CNI 二进制缺失 | `failed to find plugin "xxx" in path` | kubelet | kubelet 日志 |
| CNI DaemonSet 异常 | CrashLoopBackOff | kubectl | `kubectl get pods -n kube-system` |

#### 1.1.2 Pod 网络问题

| 现象 | 报错信息 | 报错来源 | 查看方式 |
|------|----------|----------|----------|
| Pod 无 IP 地址 | `failed to allocate IP address` | CNI 日志 | CNI 日志 |
| Pod 间无法通信 | `connection timeout` | 应用日志 | 应用日志/ping |
| 跨节点通信失败 | `no route to host` | 应用日志 | 应用日志/ping |
| Pod 到外网不通 | `network is unreachable` | 应用日志 | 应用日志/curl |
| IPAM 地址耗尽 | `no available IPs` | CNI 日志 | CNI 日志 |

#### 1.1.3 CNI 组件问题

| 现象 | 报错信息 | 报错来源 | 查看方式 |
|------|----------|----------|----------|
| Calico 节点未就绪 | `calico/node is not ready` | kubectl | `kubectl get pods -n kube-system` |
| Flannel 后端故障 | `failed to initialize VXLAN backend` | flannel 日志 | flannel Pod 日志 |
| Cilium 异常 | `cilium-agent unhealthy` | kubectl | `kubectl get pods -n kube-system` |
| 网络策略不生效 | 流量未被阻止 | 测试 | 网络测试 |

### 1.2 报错查看方式汇总

```bash
# 查看 CNI 配置目录
ls -la /etc/cni/net.d/
cat /etc/cni/net.d/*.conf*

# 查看 CNI 插件目录
ls -la /opt/cni/bin/

# 查看 kubelet CNI 相关日志
journalctl -u kubelet | grep -i cni | tail -50

# 查看 CNI 组件 Pod 状态
kubectl get pods -n kube-system -l k8s-app=calico-node
kubectl get pods -n kube-system -l app=flannel
kubectl get pods -n kube-system -l k8s-app=cilium

# 查看 CNI 组件日志
# Calico
kubectl logs -n kube-system -l k8s-app=calico-node -c calico-node --tail=200

# Flannel
kubectl logs -n kube-system -l app=flannel --tail=200

# Cilium
kubectl logs -n kube-system -l k8s-app=cilium --tail=200

# 查看节点网络状态
ip addr
ip route
bridge fdb show
```

### 1.3 影响面分析

#### 1.3.1 直接影响

| 影响范围 | 影响程度 | 影响描述 |
|----------|----------|----------|
| **Pod 创建** | 可能失败 | Pod 无法获取 IP 地址 |
| **Pod 网络** | 不可用 | Pod 间无法通信 |
| **Service** | 部分影响 | 依赖 Pod 网络的 Service 不可用 |
| **DNS** | 部分影响 | CoreDNS Pod 可能受影响 |

#### 1.3.2 间接影响

| 影响范围 | 影响程度 | 影响描述 |
|----------|----------|----------|
| **应用服务** | 高 | 服务间调用失败 |
| **外部访问** | 部分影响 | 通过 NodePort/LoadBalancer 可能受影响 |
| **监控** | 部分影响 | 监控数据采集可能失败 |
| **日志** | 部分影响 | 日志采集可能失败 |

---

## 2. 排查方法与步骤

### 2.1 排查原理：CNI 架构与数据平面

CNI（Container Network Interface）负责为 Pod 配置网络。深入理解其架构是高效排查的关键：

#### 2.1.1 CNI 标准接口
- **CNI 规范版本**：当前主流 v0.4.0 / v1.0.0，定义了标准化的网络配置接口
- **调用时机**：
  - **ADD**：Pod 创建时，kubelet 调用 CNI 插件创建网络命名空间并配置网络
  - **DEL**：Pod 删除时，kubelet 调用 CNI 插件清理网络资源
  - **CHECK**（v0.4.0+）：检查网络配置是否符合预期
  - **VERSION**：查询插件支持的 CNI 版本
- **调用参数**：
  ```json
  {
    "cniVersion": "1.0.0",
    "name": "k8s-pod-network",
    "type": "calico",
    "ipam": {
      "type": "calico-ipam"
    },
    "containerID": "abc123...",
    "netns": "/var/run/netns/cni-xxx",
    "ifname": "eth0"
  }
  ```
- **返回结果**：包含分配的 IP 地址、路由、DNS 配置等

#### 2.1.2 CNI 插件分类与职责

##### 1. 主插件（Main Plugin）
负责创建网络接口和配置路由：

**Calico**：
- **数据平面**：纯三层路由（默认）或 VXLAN/IPIP 封装
- **控制平面**：BGP 协议分发路由（bird）或 kube-apiserver 存储路由
- **网络策略**：通过 iptables 或 eBPF（Calico-eBPF）实现
- **优势**：性能好（无封装）、支持网络策略、大规模集群稳定
- **组件**：
  - `calico-node`（DaemonSet）：运行 BIRD BGP、Felix（路由/策略管理）
  - `calico-kube-controllers`（Deployment）：监听 API Server 同步网络配置
  - `calico-typha`（可选）：缓存 API Server 数据，减少 API 压力

**Flannel**：
- **数据平面**：VXLAN（默认）、Host-GW（纯路由）、UDP（已废弃）
- **控制平面**：etcd 或 Kubernetes API 存储网络配置
- **网络策略**：不支持（需配合 Calico Policy Controller）
- **优势**：简单易部署、社区成熟
- **后端模式**：
  - **VXLAN**：三层网络隧道，兼容性好但有性能开销（5-10%）
  - **Host-GW**：纯路由，要求节点在同一二层网络，性能最优
  - **WireGuard**：加密隧道，安全但性能开销较大

**Cilium**：
- **数据平面**：eBPF 内核加速（绕过 netfilter/iptables）
- **控制平面**：Key-Value Store（etcd）或 CRD
- **网络策略**：L3-L7 策略（HTTP/gRPC/Kafka 协议感知）
- **优势**：高性能、可观测性强、支持服务网格
- **组件**：
  - `cilium-agent`（DaemonSet）：加载 eBPF 程序、管理网络
  - `cilium-operator`（Deployment）：IP 地址管理、CRD 控制器
  - `hubble`（可选）：网络流量可观测性

**Weave Net**：
- **数据平面**：UDP 封装（默认）或 VXLAN
- **特点**：自动加密、无需外部存储、内置 DNS
- **劣势**：性能较差、社区活跃度下降

##### 2. IPAM 插件（IP Address Management）
负责分配和管理 IP 地址：

**host-local**：
- **原理**：在每个节点本地存储 IP 分配状态（`/var/lib/cni/networks/`）
- **配置**：预先划分子网池，静态分配给节点
- **优势**：简单、无依赖
- **劣势**：子网固定，无法动态调整

**calico-ipam**：
- **原理**：从全局 IP 池动态分配 CIDR 给节点，节点内再分配给 Pod
- **配置**：通过 IPPool CRD 定义地址池
- **优势**：灵活、支持多 IP 池、IP 回收自动化
- **高级特性**：
  - IP 池亲和性（特定 Pod 从特定池分配）
  - IP 保留（StatefulSet 固定 IP）
  - CIDR 动态扩容

**cilium-ipam**：
- **模式**：
  - **Cluster Scope**：全局统一 CIDR
  - **Kubernetes**：复用 Node.spec.podCIDR
  - **CRD**：通过 CiliumNode CRD 管理

**whereabouts**（第三方）：
- **原理**：跨节点协调 IP 分配（通过 etcd 或 Kubernetes API）
- **场景**：多 CNI、动态 IP 池

##### 3. Meta 插件（辅助插件）
提供额外功能：

**portmap**：
- **功能**：实现容器端口映射到宿主机（类似 Docker `-p`）
- **实现**：配置 iptables DNAT 规则

**bandwidth**：
- **功能**：限制 Pod 带宽（入站/出站）
- **实现**：使用 Linux tc（Traffic Control）

**tuning**：
- **功能**：调整网络接口参数（MTU、队列长度等）

**firewall**：
- **功能**：配置 iptables 规则（基础防火墙）

#### 2.1.3 数据平面技术详解

##### 1. VXLAN（Virtual eXtensible LAN）
- **原理**：二层报文封装在 UDP 中跨三层网络传输
- **封装开销**：50 字节（外层 IP 20B + UDP 8B + VXLAN 8B + 内层 Ethernet 14B）
- **MTU 计算**：物理链路 MTU 1500 - 50 = 1450（Pod MTU）
- **端口**：UDP 4789（IANA 标准）
- **Linux 实现**：通过 `vxlan` 类型 netdevice，内核自动封装/解封装
- **FDB（Forwarding Database）**：存储 MAC 地址 → VTEP（VXLAN Tunnel Endpoint）映射
  ```bash
  bridge fdb show dev vxlan.calico
  # 00:00:00:00:00:00 dst 10.0.1.10 via flannel.1  # 远程节点 VTEP
  ```
- **性能影响**：CPU 开销 5-10%，适用于跨子网场景

##### 2. IPIP（IP-in-IP）
- **原理**：IP 报文封装在另一个 IP 报文中
- **封装开销**：20 字节（外层 IP 头）
- **MTU 计算**：1500 - 20 = 1480
- **模式**：
  - **Always**：所有流量封装
  - **CrossSubnet**：仅跨子网流量封装（推荐）
- **优势**：开销小于 VXLAN
- **劣势**：不支持 IPv6、部分云厂商限制 IPIP 协议

##### 3. Host-GW（Host Gateway）
- **原理**：纯三层路由，无封装
- **要求**：所有节点在同一二层网络（或云厂商打通路由）
- **路由示例**：
  ```bash
  ip route
  # 10.244.1.0/24 via 10.0.1.10 dev eth0  # 通过节点 IP 路由到 Pod 子网
  ```
- **优势**：性能最优（无封装开销）
- **劣势**：对网络拓扑要求高

##### 4. BGP（Border Gateway Protocol）
- **角色**：控制平面协议，分发路由信息
- **Calico 实现**：
  - **Full Mesh**：每个节点与其他所有节点建立 BGP 连接（适用 < 100 节点）
  - **Route Reflector**：指定部分节点作为 RR，其他节点仅与 RR 建立连接（适用大规模集群）
  - **ToR（Top-of-Rack）集成**：与物理交换机建立 BGP，将 Pod 路由注入数据中心网络
- **Bird 配置**：Calico-node 内置 BIRD BGP daemon
  ```bash
  # 进入 calico-node 容器
  kubectl exec -it -n kube-system calico-node-xxx -c calico-node -- /bin/bash
  
  # 查看 BGP 对等体状态
  calicoctl node status
  # IPv4 BGP status
  # +--------------+-------------------+-------+----------+
  # | PEER ADDRESS |     PEER TYPE     | STATE |  SINCE   |
  # +--------------+-------------------+-------+----------+
  # | 10.0.1.11    | node-to-node mesh | up    | 08:15:23 |
  # | 10.0.1.12    | node-to-node mesh | up    | 08:15:23 |
  ```

##### 5. eBPF（Cilium）
- **原理**：在内核态执行高性能数据包处理，绕过 netfilter/iptables
- **加速效果**：相比 iptables，网络吞吐提升 20-50%，延迟降低 30-60%
- **实现**：
  - 在网卡 TC（Traffic Control）层挂载 eBPF 程序
  - 直接修改数据包并转发，无需经过完整协议栈
- **可观测性**：通过 Hubble 捕获每个数据包的元数据（源/目标、协议、策略决策）

#### 2.1.4 常见故障模式与根因

##### 1. IP 地址耗尽
- **现象**：新 Pod Pending，事件显示 `failed to allocate IP address`
- **根因**：
  - 节点 PodCIDR 过小（默认 /24 = 254 IP）
  - IP 泄露（Pod 删除后 IP 未回收）
  - 节点过多但地址池总量不足
- **排查**：
  ```bash
  # 查看节点已分配 CIDR
  kubectl get nodes -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{.spec.podCIDR}{"\n"}{end}'
  
  # Calico：查看 IP 池使用情况
  calicoctl ipam show --show-blocks
  ```

##### 2. MTU 不匹配导致大包丢弃
- **现象**：小包（ping）通，大包（HTTP 传输）超时
- **根因**：
  - Pod MTU 与物理链路不匹配
  - Overlay 封装增加头部，导致报文超过 MTU 且设置 DF（Don't Fragment）位
- **排查**：
  ```bash
  # 测试路径 MTU
  ping -M do -s 1472 <pod-ip>  # 1472 + 28(IP+ICMP) = 1500
  # 若失败，逐步减小 -s 值找到最大 MTU
  
  # 检查 Pod MTU
  kubectl exec -it <pod> -- ip link show eth0
  # mtu 1500  # ❌ Overlay 网络应为 1450 或更小
  ```

##### 3. ARP/FDB 表溢出
- **现象**：跨节点通信间歇性丢包，重启 CNI 后短暂恢复
- **根因**：
  - 内核 ARP 表/FDB 表大小限制（默认 1024/4096）
  - 大规模集群 Pod 数量超过限制
- **排查**：
  ```bash
  # 查看 ARP 表使用情况
  ip -s neigh show | grep -c REACHABLE
  
  # 查看 FDB 表
  bridge fdb show | wc -l
  
  # 检查内核参数
  sysctl net.ipv4.neigh.default.gc_thresh3
  # net.ipv4.neigh.default.gc_thresh3 = 1024  # ❌ 过小
  ```

##### 4. iptables 规则过多导致性能下降
- **现象**：Service 访问延迟高，kube-proxy CPU 高
- **根因**：
  - iptables 线性匹配，规则数 O(n) 影响性能
  - 1000+ Service 时明显卡顿
- **解决**：
  - 切换到 IPVS 模式（O(1) 查找）
  - 或使用 Cilium eBPF 替代 kube-proxy

#### 2.1.5 跨节点网络排查工具链

```bash
# 1. 路径追踪
# 查看数据包如何从源 Pod 到目标 Pod
ip route get <dst-pod-ip> from <src-pod-ip>

# 2. 网络命名空间调试
# 进入 Pod 网络命名空间
nsenter -t $(crictl inspect <container-id> | jq .info.pid) -n

# 3. 抓包分析
# 抓取 VXLAN 封装报文
tcpdump -i eth0 udp port 4789 -vv -w /tmp/vxlan.pcap

# 4. 连通性矩阵测试
# 使用 kubenetbench 或自定义脚本测试所有 Pod 对
for src in $(kubectl get pods -A -o jsonpath='{.items[*].status.podIP}'); do
  for dst in $(kubectl get pods -A -o jsonpath='{.items[*].status.podIP}'); do
    kubectl run test --image=busybox --rm -it --restart=Never -- \
      wget -T 5 -O- http://$dst:80 || echo "$src -> $dst FAILED"
  done
done
```

### 2.2 排查步骤和具体命令

#### 2.2.1 第一步：检查 CNI 安装

```bash
# 检查 CNI 配置文件
ls -la /etc/cni/net.d/
cat /etc/cni/net.d/10-calico.conflist  # Calico
cat /etc/cni/net.d/10-flannel.conflist  # Flannel
cat /etc/cni/net.d/05-cilium.conf  # Cilium

# 检查 CNI 二进制文件
ls -la /opt/cni/bin/

# 检查必需的 CNI 插件
ls /opt/cni/bin/ | grep -E "(calico|flannel|cilium|portmap|bandwidth)"

# 检查 kubelet CNI 配置
cat /var/lib/kubelet/config.yaml | grep -A5 cni

# 或者检查 kubelet 启动参数
ps aux | grep kubelet | grep cni
```

#### 2.2.2 第二步：检查 CNI 组件状态

```bash
# Calico
kubectl get pods -n kube-system -l k8s-app=calico-node -o wide
kubectl get pods -n kube-system -l k8s-app=calico-kube-controllers -o wide
calicoctl node status

# Flannel
kubectl get pods -n kube-system -l app=flannel -o wide

# Cilium
kubectl get pods -n kube-system -l k8s-app=cilium -o wide
cilium status

# 检查 DaemonSet 状态
kubectl get daemonset -n kube-system
```

#### 2.2.3 第三步：检查 Pod 网络

```bash
# 检查 Pod IP 分配
kubectl get pods -A -o wide

# 进入 Pod 检查网络配置
kubectl exec -it <pod-name> -- sh
# 在 Pod 内执行
ip addr
ip route
cat /etc/resolv.conf

# 测试 Pod 间连通性
kubectl exec -it <pod-a> -- ping <pod-b-ip>

# 测试跨节点连通性
# 找到不同节点的 Pod
kubectl get pods -o wide -A | grep -v <current-node>
kubectl exec -it <pod-a> -- ping <pod-on-other-node-ip>
```

#### 2.2.4 第四步：检查网络底层

```bash
# 检查网络接口
ip link show

# 检查 VXLAN 接口（Flannel VXLAN 模式）
ip -d link show flannel.1

# 检查 IPIP 隧道（Calico IPIP 模式）
ip -d link show tunl0

# 检查 BGP 状态（Calico BGP 模式）
calicoctl node status

# 检查路由表
ip route

# 检查 ARP 表
arp -n

# 检查 FDB 表（VXLAN）
bridge fdb show dev flannel.1
```

#### 2.2.5 第五步：检查 IPAM

```bash
# Calico IPAM
calicoctl ipam show
calicoctl ipam check

# 查看 IP Pool
calicoctl get ippool -o wide

# 查看节点 IP 分配
calicoctl get workloadendpoint -A

# Flannel 检查子网分配
cat /run/flannel/subnet.env
etcdctl get /coreos.com/network/subnets --prefix
```

#### 2.2.6 第六步：抓包分析

```bash
# 在节点上抓包
tcpdump -i any host <pod-ip> -nn

# 抓取 VXLAN 流量
tcpdump -i flannel.1 -nn

# 抓取特定端口流量
tcpdump -i any port 4789 -nn  # VXLAN 端口

# 使用 nsenter 进入 Pod 网络命名空间抓包
# 获取 Pod 的 PID
pid=$(crictl inspect <container-id> | jq '.info.pid')
nsenter -t $pid -n tcpdump -i eth0 -nn
```

### 2.3 排查注意事项

| 注意项 | 说明 | 建议 |
|--------|------|------|
| **CNI 版本兼容** | CNI 版本需与 Kubernetes 兼容 | 查看兼容矩阵 |
| **节点防火墙** | 防火墙可能阻止隧道流量 | 检查 iptables 规则 |
| **MTU 设置** | MTU 不匹配导致分片问题 | 检查 MTU 配置 |
| **IP 地址冲突** | Pod CIDR 与节点网络冲突 | 规划网络地址 |

---

## 3. 解决方案与风险控制

### 3.1 CNI 配置未初始化

#### 3.1.1 解决步骤

```bash
# 步骤 1：检查 CNI 配置目录
ls -la /etc/cni/net.d/

# 步骤 2：如果目录为空，检查 CNI DaemonSet
kubectl get pods -n kube-system | grep -E "(calico|flannel|cilium)"

# 步骤 3：检查 DaemonSet 日志
kubectl logs -n kube-system <cni-pod> --tail=100

# 步骤 4：如果 DaemonSet 未部署，安装 CNI
# Calico
kubectl apply -f https://raw.githubusercontent.com/projectcalico/calico/v3.26.1/manifests/calico.yaml

# Flannel
kubectl apply -f https://raw.githubusercontent.com/flannel-io/flannel/master/Documentation/kube-flannel.yml

# Cilium
helm install cilium cilium/cilium --namespace kube-system

# 步骤 5：等待 CNI Pod 就绪
kubectl rollout status daemonset -n kube-system calico-node

# 步骤 6：验证配置生成
ls -la /etc/cni/net.d/
```

#### 3.1.2 执行风险

| 风险等级 | 风险描述 | 缓解措施 |
|----------|----------|----------|
| **高** | 初次安装 CNI 可能导致已有 Pod 网络中断 | 在新集群操作 |
| **中** | CNI 版本选择影响功能 | 选择稳定版本 |

#### 3.1.3 安全生产风险提示

```
⚠️  安全生产风险提示：
1. 生产集群不建议更换 CNI 插件
2. 安装 CNI 前确认 Pod CIDR 配置正确
3. 确保所有节点都能访问 CNI 镜像
4. 安装后验证所有节点 CNI Pod 正常
5. 测试 Pod 间连通性
```

### 3.2 Pod 无法获取 IP 地址

#### 3.2.1 解决步骤

```bash
# 步骤 1：检查 CNI 日志
kubectl logs -n kube-system -l k8s-app=calico-node -c calico-node | grep -i "ip"

# 步骤 2：检查 IP Pool 配置
calicoctl get ippool -o yaml

# 步骤 3：检查 IP Pool 是否有可用 IP
calicoctl ipam show

# 步骤 4：如果 IP 耗尽，扩展 IP Pool
calicoctl apply -f - << EOF
apiVersion: projectcalico.org/v3
kind: IPPool
metadata:
  name: new-pool
spec:
  cidr: 10.245.0.0/16
  ipipMode: Always
  natOutgoing: true
EOF

# 步骤 5：或者清理未使用的 IP
calicoctl ipam release --ip=<unused-ip>

# 步骤 6：验证 IP 分配
kubectl get pods -A -o wide | grep Pending
```

#### 3.2.2 执行风险

| 风险等级 | 风险描述 | 缓解措施 |
|----------|----------|----------|
| **中** | 新 IP Pool 可能与现有网络冲突 | 规划地址空间 |
| **低** | 释放 IP 不影响运行 Pod | 确认 IP 未使用 |

#### 3.2.3 安全生产风险提示

```
⚠️  安全生产风险提示：
1. 新增 IP Pool 前确认不与现有网络冲突
2. 不要释放正在使用的 IP
3. IP 耗尽是容量问题，考虑扩容
4. 监控 IP Pool 使用率
5. 预留足够的 IP 地址空间
```

### 3.3 跨节点通信失败

#### 3.3.1 解决步骤

```bash
# 步骤 1：确认问题范围
kubectl get pods -o wide -A
# 找到不同节点的 Pod 测试

# 步骤 2：检查节点间网络
# 在节点 A 上
ping <node-b-ip>

# 步骤 3：检查隧道接口
# VXLAN 模式
ip -d link show flannel.1
# IPIP 模式
ip -d link show tunl0

# 步骤 4：检查路由
ip route | grep <other-node-pod-cidr>

# 步骤 5：检查防火墙规则
# VXLAN 需要 UDP 4789
iptables -L -n | grep 4789
# IPIP 需要协议 4
iptables -L -n | grep ipencap

# 步骤 6：如果防火墙阻止，添加规则
# VXLAN
iptables -A INPUT -p udp --dport 4789 -j ACCEPT
# IPIP
iptables -A INPUT -p 4 -j ACCEPT
# BGP
iptables -A INPUT -p tcp --dport 179 -j ACCEPT

# 步骤 7：检查云平台安全组（如果适用）
# 确保安全组允许节点间通信

# 步骤 8：验证修复
kubectl exec -it <pod-a> -- ping <pod-on-other-node-ip>
```

#### 3.3.2 执行风险

| 风险等级 | 风险描述 | 缓解措施 |
|----------|----------|----------|
| **中** | 防火墙规则变更影响安全 | 仅开放必要端口 |
| **低** | 网络检查无风险 | - |

#### 3.3.3 安全生产风险提示

```
⚠️  安全生产风险提示：
1. 不要禁用所有防火墙规则
2. 只开放 CNI 需要的端口
3. 云平台安全组需要同步配置
4. 考虑使用 NetworkPolicy 进行细粒度控制
5. 记录所有防火墙变更
```

### 3.4 MTU 问题导致大包丢失

#### 3.4.1 解决步骤

```bash
# 步骤 1：确认 MTU 问题
# 大包测试
kubectl exec -it <pod-a> -- ping -s 1400 <pod-b-ip>
kubectl exec -it <pod-a> -- ping -s 1472 <pod-b-ip>

# 步骤 2：检查各接口 MTU
ip link show eth0
ip link show flannel.1
ip link show tunl0

# 步骤 3：计算正确的 MTU
# VXLAN: 节点 MTU - 50
# IPIP: 节点 MTU - 20

# 步骤 4：修改 CNI MTU 配置
# Calico
calicoctl patch felixconfiguration default -p '{"spec":{"mtu": 1440}}'

# Flannel（修改 ConfigMap）
kubectl edit configmap -n kube-system kube-flannel-cfg
# 修改 net-conf.json 中的 Backend.MTU

# 步骤 5：重启 CNI Pod 应用配置
kubectl rollout restart daemonset -n kube-system calico-node

# 步骤 6：验证修复
kubectl exec -it <pod-a> -- ping -s 1400 <pod-b-ip>
```

#### 3.4.2 执行风险

| 风险等级 | 风险描述 | 缓解措施 |
|----------|----------|----------|
| **中** | MTU 变更需要重启 CNI Pod | 在维护窗口操作 |
| **低** | MTU 检测无风险 | - |

#### 3.4.3 安全生产风险提示

```
⚠️  安全生产风险提示：
1. MTU 设置过大会导致分片
2. MTU 设置过小影响性能
3. 所有节点 MTU 应该一致
4. 云环境检查网络 MTU 限制
5. 变更后全面测试大数据传输
```

---

## 附录

### A. 常见 CNI 端口

| CNI | 协议 | 端口 | 用途 |
|-----|------|------|------|
| Calico VXLAN | UDP | 4789 | VXLAN 封装 |
| Calico IPIP | IP | 4 | IPIP 隧道 |
| Calico BGP | TCP | 179 | BGP 路由 |
| Flannel VXLAN | UDP | 4789 | VXLAN 封装 |
| Flannel UDP | UDP | 8285 | UDP 封装 |
| Cilium VXLAN | UDP | 8472 | VXLAN 封装 |

### B. CNI 模式对比

| 模式 | 优点 | 缺点 | 适用场景 |
|------|------|------|----------|
| Overlay (VXLAN) | 跨子网、易部署 | 性能开销 | 云环境 |
| IPIP | 比 VXLAN 轻量 | 需要路由支持 | 私有云 |
| BGP | 原生路由、高性能 | 需要网络支持 | 裸金属 |
| Direct | 最高性能 | 网络要求高 | 特定环境 |

### C. 常用诊断命令

```bash
# Calico
calicoctl node status
calicoctl get node -o wide
calicoctl get ippool -o wide
calicoctl ipam check

# Cilium
cilium status
cilium connectivity test
cilium bpf endpoint list

# Flannel
cat /run/flannel/subnet.env

# 通用
ip route
ip link
bridge fdb show
conntrack -L
```

---

## 📚 D. 生产环境实战案例精选

### 案例 1：Calico IPAM 地址池耗尽导致大规模 Pod 创建失败

#### 🎯 故障场景
某互联网公司在业务高峰期自动扩容，10 分钟内需创建 2000+ Pod，但发现只有 500 个 Pod 成功启动，其余全部 Pending，报错 `failed to allocate IP address`，业务扩容失败。

#### 🔍 排查过程
1. **现象确认**：
   ```bash
   # 大量 Pod Pending
   kubectl get pods -A | grep Pending | wc -l
   # 1500  # ❌ 75% Pod 无法创建
   
   # 查看失败 Pod 事件
   kubectl describe pod myapp-abc123 | grep -A5 Events
   # Warning  FailedCreatePodSandBox  Failed to create pod sandbox: 
   #          rpc error: code = Unknown desc = failed to setup network for sandbox: 
   #          plugin type="calico" failed (add): error getting ClusterInformation: 
   #          failed to allocate IP address: no IPs available in block
   ```

2. **Calico IP 池检查**：
   ```bash
   # 查看 IP 池配置
   calicoctl get ippool -o wide
   # NAME           CIDR            NAT      IPIPMODE   VXLANMODE   DISABLED
   # default-pool   10.244.0.0/16   true     Never      Always      false
   # 总量：65536 个 IP（/16）
   
   # 查看 IP 分配情况
   calicoctl ipam show --show-blocks
   # +----------+------------------+------------+------------+-------------------+
   # | GROUPING |      CIDR        | IPS TOTAL  | IPS IN USE | IPS AVAILABLE     |
   # +----------+------------------+------------+------------+-------------------+
   # | IP Pool  | 10.244.0.0/16    | 65536      | 65534      | 2                 | ❌
   # +----------+------------------+------------+------------+-------------------+
   
   # 查看详细分配块
   calicoctl ipam show --show-blocks | head -50
   # Block 10.244.0.0/26     Node: node-01   IPs: 64/64   ✅ 已满
   # Block 10.244.0.64/26    Node: node-02   IPs: 64/64   ✅ 已满
   # Block 10.244.0.128/26   Node: node-03   IPs: 64/64   ✅ 已满
   # ...
   # 发现：1024 个 Block 全部分配完毕
   ```

3. **IP 泄露检查**：
   ```bash
   # 统计 Pod 实际数量
   kubectl get pods -A --field-selector=status.phase=Running | wc -l
   # 15000  # 实际运行 15000 个 Pod
   
   # 统计已分配 IP 数
   calicoctl ipam show | grep "IPS IN USE"
   # IPS IN USE: 65534  # ❌ 分配了 65534 个 IP
   
   # 差值：65534 - 15000 = 50534 个 IP 泄露！
   
   # 查找僵尸 WorkloadEndpoint
   calicoctl get workloadendpoint -A | wc -l
   # 65534  # WorkloadEndpoint 数量与分配 IP 一致
   
   kubectl get pods -A -o json | jq -r '.items[] | .metadata.name' | wc -l
   # 15000  # 但 Pod 实际只有 15000
   
   # 根因：历史删除的 Pod 对应的 WorkloadEndpoint 未清理
   ```

4. **根因分析**：
   - **直接原因**：IP 池 /16（65536 IP）耗尽
   - **深层原因**：
     1. 大规模 Pod 创建/删除（每天数万次滚动发布）
     2. Calico Controller 清理 WorkloadEndpoint 有延迟/失败
     3. 累积 50000+ 僵尸 WorkloadEndpoint，占用 IP 未释放
   - **为什么突然爆发**：高峰期扩容触发最后 2 个 IP 被占用，新 Pod 无 IP 可分配

#### ⚡ 应急措施
1. **立即清理僵尸 WorkloadEndpoint**：
   ```bash
   # 获取所有 WorkloadEndpoint
   calicoctl get workloadendpoint -A -o yaml > /tmp/wep-backup.yaml
   
   # 获取实际存在的 Pod 列表
   kubectl get pods -A -o jsonpath='{range .items[*]}{.metadata.namespace}/{.metadata.name}{"\n"}{end}' | sort > /tmp/pods.txt
   
   # 获取 WorkloadEndpoint 对应的 Pod
   calicoctl get workloadendpoint -A -o json | \
     jq -r '.items[] | "\(.metadata.namespace)/\(.metadata.labels.projectcalico_org_workload)"' | \
     sort > /tmp/weps.txt
   
   # 找出僵尸 WorkloadEndpoint（存在于 weps 但不在 pods 中）
   comm -13 /tmp/pods.txt /tmp/weps.txt > /tmp/zombie-weps.txt
   wc -l /tmp/zombie-weps.txt
   # 50534  # ❌ 确认 50000+ 僵尸
   
   # 批量删除僵尸 WorkloadEndpoint
   calicoctl get workloadendpoint -A -o json | \
     jq -r '.items[] | 
       select(.metadata.namespace + "/" + .metadata.labels.projectcalico_org_workload as $key | 
       [$key] | inside(["僵尸列表"])) | 
       "\(.metadata.namespace) \(.metadata.name)"' | \
     xargs -n2 calicoctl delete workloadendpoint --namespace
   
   # 或使用更安全的方式：标记为删除的 Pod 对应的 WEP
   for ns_pod in $(cat /tmp/zombie-weps.txt); do
     ns=$(echo $ns_pod | cut -d/ -f1)
     pod=$(echo $ns_pod | cut -d/ -f2)
     calicoctl get workloadendpoint -n $ns -l projectcalico.org/workload=$pod -o name | \
       xargs -r calicoctl delete
   done
   ```

2. **验证 IP 释放**：
   ```bash
   # 再次检查 IP 池
   calicoctl ipam show
   # IPS IN USE: 15000  # ✅ 恢复正常
   # IPS AVAILABLE: 50536  # ✅ 释放 50000+ IP
   ```

3. **触发 Pod 重建**：
   ```bash
   # 删除 Pending Pod 触发重新调度
   kubectl get pods -A --field-selector=status.phase=Pending -o name | xargs kubectl delete --wait=false
   
   # 等待 5 分钟
   kubectl get pods -A | grep Pending | wc -l
   # 0  # ✅ 全部成功创建
   ```

#### 🛡️ 长期优化
1. **扩大 IP 池**：
   ```bash
   # 方案 1：扩展现有 IP 池（需确保 CIDR 不与其他网络冲突）
   calicoctl patch ippool default-pool -p '{"spec":{"cidr":"10.244.0.0/12"}}'
   # /12 = 1048576 IP（从 65536 扩展至 100 万+）
   
   # 方案 2：添加额外 IP 池
   cat <<EOF | calicoctl apply -f -
   apiVersion: projectcalico.org/v3
   kind: IPPool
   metadata:
     name: secondary-pool
   spec:
     cidr: 10.245.0.0/16
     ipipMode: Never
     vxlanMode: Always
     natOutgoing: true
   EOF
   ```

2. **启用 WorkloadEndpoint 自动清理**：
   ```yaml
   # Calico Controller 配置
   apiVersion: v1
   kind: ConfigMap
   metadata:
     name: calico-config
     namespace: kube-system
   data:
     # 启用自动清理孤儿 WorkloadEndpoint
     typha_enabled: "true"
     
   # 或者通过 calico-kube-controllers Deployment 配置
   apiVersion: apps/v1
   kind: Deployment
   metadata:
     name: calico-kube-controllers
     namespace: kube-system
   spec:
     template:
       spec:
         containers:
         - name: calico-kube-controllers
           env:
           - name: ENABLED_CONTROLLERS
             value: "node,policy,workloadendpoint"  # ✅ 确保启用 workloadendpoint 控制器
           - name: LOG_LEVEL
             value: "info"
   ```

3. **定期清理脚本**：
   ```bash
   # CronJob 每小时清理一次
   cat <<EOF | kubectl apply -f -
   apiVersion: batch/v1
   kind: CronJob
   metadata:
     name: calico-wep-cleaner
     namespace: kube-system
   spec:
     schedule: "0 * * * *"  # 每小时
     jobTemplate:
       spec:
         template:
           spec:
             serviceAccountName: calico-kube-controllers
             containers:
             - name: cleaner
               image: calico/ctl:v3.26.0
               command:
               - /bin/sh
               - -c
               - |
                 # 获取所有 Pod
                 kubectl get pods -A -o json | \
                   jq -r '.items[] | "\(.metadata.namespace)/\(.metadata.name)"' | \
                   sort > /tmp/pods.txt
                 
                 # 获取所有 WorkloadEndpoint
                 calicoctl get workloadendpoint -A -o json | \
                   jq -r '.items[] | 
                     "\(.metadata.namespace)/\(.metadata.labels."projectcalico.org/workload")"' | \
                   sort > /tmp/weps.txt
                 
                 # 找出僵尸并删除
                 comm -13 /tmp/pods.txt /tmp/weps.txt | while read ns_pod; do
                   ns=$(echo $ns_pod | cut -d/ -f1)
                   pod=$(echo $ns_pod | cut -d/ -f2)
                   echo "Deleting zombie WEP: $ns/$pod"
                   calicoctl delete workloadendpoint -n $ns -l projectcalico.org/workload=$pod
                 done
             restartPolicy: OnFailure
   EOF
   ```

4. **监控 IP 池使用率**：
   ```yaml
   # Prometheus 告警规则
   groups:
   - name: calico-ipam
     rules:
     - alert: CalicoIPPoolLowAvailability
       expr: |
         (calico_ipam_allocations_per_node - calico_ipam_blocks_per_node * 64) / 
         (calico_ipam_blocks_per_node * 64) > 0.8
       for: 10m
       labels:
         severity: warning
       annotations:
         summary: "Calico IP 池使用率高"
         description: "节点 {{ $labels.node }} IP 使用率 {{ $value | humanizePercentage }}，可能即将耗尽"
     
     - alert: CalicoIPPoolExhausted
       expr: calico_ipam_blocks_per_node * 64 - calico_ipam_allocations_per_node < 10
       for: 5m
       labels:
         severity: critical
       annotations:
         summary: "Calico IP 池即将耗尽"
         description: "节点 {{ $labels.node }} 仅剩 {{ $value }} 个可用 IP"
   
   # 使用 Calico 自带的 Prometheus exporter
   kubectl apply -f https://docs.projectcalico.org/manifests/prometheus-calico.yaml
   ```

5. **容量规划**：
   ```bash
   # 计算 IP 需求
   # 节点数：1000
   # 每节点平均 Pod 数：100
   # 预留 20% 余量
   # 总需求：1000 × 100 × 1.2 = 120000 IP
   # 推荐 CIDR：/15（131072 IP）
   
   # 评估现有 IP 池
   current_cidr="10.244.0.0/16"  # 65536 IP
   required_ips=120000
   if [ 65536 -lt $required_ips ]; then
     echo "需要扩展 IP 池至 /15 或更大"
   fi
   ```

#### 💡 经验总结
- **容量规划不足**：集群扩容未评估 IP 池容量
- **清理机制失效**：WorkloadEndpoint 控制器未启用或有 Bug
- **监控盲区**：未监控 IP 池使用率和僵尸资源
- **改进方向**：扩大 IP 池、自动化清理、监控告警、定期审计

---

### 案例 2：MTU 不匹配导致 HTTP 大文件传输超时

#### 🎯 故障场景
某视频公司部署了视频转码服务，用户上传小文件（< 1MB）正常，但上传大文件（> 10MB）时连接总是中途超时，前端报 `ERR_CONNECTION_RESET`。

#### 🔍 排查过程
1. **现象确认**：
   ```bash
   # 测试小文件上传（成功）
   kubectl exec -it test-client -- curl -F "file=@/tmp/small.txt" http://upload-service/upload
   # {"status":"ok"}  ✅
   
   # 测试大文件上传（失败）
   kubectl exec -it test-client -- curl -F "file=@/tmp/large.mp4" http://upload-service/upload
   # curl: (56) Recv failure: Connection reset by peer  ❌
   
   # 测试 ping（成功）
   kubectl exec -it test-client -- ping -c 5 <upload-pod-ip>
   # 5 packets transmitted, 5 received, 0% packet loss  ✅
   
   # 结论：小包通，大包不通
   ```

2. **MTU 测试**：
   ```bash
   # 测试不同大小的 ping
   kubectl exec -it test-client -- ping -M do -s 1400 <upload-pod-ip>
   # 1400 bytes: icmp_seq=1 ttl=64 time=0.5 ms  ✅
   
   kubectl exec -it test-client -- ping -M do -s 1450 <upload-pod-ip>
   # 1450 bytes: icmp_seq=1 ttl=64 time=0.6 ms  ✅
   
   kubectl exec -it test-client -- ping -M do -s 1472 <upload-pod-ip>
   # ping: sendmsg: Message too long  ❌
   
   # 结论：1450 通，1472 不通，MTU 问题！
   # 1472 + 28(IP+ICMP头) = 1500，正好是标准 MTU
   ```

3. **网络配置检查**：
   ```bash
   # 检查 Pod 网卡 MTU
   kubectl exec -it upload-pod -- ip link show eth0
   # eth0: <BROADCAST,MULTICAST,UP,LOWER_UP> mtu 1500 qdisc noqueue state UP
   # ❌ MTU 1500
   
   # 检查宿主机 VXLAN 接口 MTU
   ip link show vxlan.calico
   # vxlan.calico: mtu 1450 qdisc noqueue state UNKNOWN
   # ✅ 宿主机正确配置为 1450
   
   # 检查物理网卡 MTU
   ip link show eth0
   # eth0: mtu 1500 qdisc mq state UP
   ```

4. **封装开销分析**：
   ```
   数据包路径：
   1. Pod eth0 发送：Payload + 20B(IP) + 8B(TCP) = 1500B（MTU 限制）
   2. 宿主机 veth 接收：1500B
   3. Calico VXLAN 封装：1500B + 50B(VXLAN头) = 1550B  ❌ 超过物理网卡 MTU 1500！
   4. 物理网卡 eth0 发送：由于设置了 DF（Don't Fragment）位，无法分片
   5. 内核丢弃报文，返回 ICMP Fragmentation Needed
   6. TCP 超时重传，最终连接失败
   ```

5. **根因分析**：
   - **配置错误**：Pod MTU 设置为 1500，未考虑 VXLAN 封装开销 50B
   - **正确配置**：Pod MTU 应为 1500 - 50 = 1450
   - **为什么小包通**：< 1450B 的包未超过 MTU，可以正常发送
   - **为什么大包不通**：> 1450B 的包经 VXLAN 封装后 > 1500B，触发 DF 位导致丢包

#### ⚡ 应急措施
1. **立即修复 Calico MTU 配置**：
   ```bash
   # 修改 Calico 配置
   kubectl edit cm calico-config -n kube-system
   
   # 添加 MTU 配置
   data:
     veth_mtu: "1450"  # ✅ 设置 Pod MTU 为 1450
   
   # 或者通过 Installation CRD（Calico Operator）
   kubectl edit installation default
   
   spec:
     calicoNetwork:
       mtu: 1450  # ✅ 全局 MTU 配置
   ```

2. **重启 Calico DaemonSet**：
   ```bash
   # 滚动重启 calico-node
   kubectl rollout restart daemonset calico-node -n kube-system
   
   # 等待所有 Pod 重启完成
   kubectl rollout status daemonset calico-node -n kube-system
   ```

3. **重建测试 Pod**：
   ```bash
   # 删除现有 Pod（触发重建以应用新 MTU）
   kubectl delete pod upload-pod test-client
   
   # 等待重建
   kubectl wait --for=condition=Ready pod/upload-pod --timeout=60s
   
   # 验证新 Pod MTU
   kubectl exec -it upload-pod -- ip link show eth0
   # eth0: mtu 1450  ✅ 修复成功
   ```

4. **验证大文件传输**：
   ```bash
   # 再次测试大文件上传
   kubectl exec -it test-client -- curl -F "file=@/tmp/large.mp4" http://upload-service/upload
   # {"status":"ok","size":"52428800"}  ✅ 成功！
   
   # 测试 MTU
   kubectl exec -it test-client -- ping -M do -s 1422 <upload-pod-ip>
   # 1422 + 28 = 1450，应该通过
   # 1422 bytes: icmp_seq=1 ttl=64 time=0.5 ms  ✅
   ```

#### 🛡️ 长期优化
1. **自动检测 MTU**：
   ```yaml
   # Calico 自动检测配置
   apiVersion: operator.tigera.io/v1
   kind: Installation
   metadata:
     name: default
   spec:
     calicoNetwork:
       mtu: 0  # ✅ 设置为 0 启用自动检测
       nodeAddressAutodetectionV4:
         interface: "eth0"  # 检测物理网卡 MTU
   
   # Calico 会自动：
   # 1. 检测物理网卡 MTU（如 1500）
   # 2. 根据封装类型减去开销（VXLAN -50，IPIP -20）
   # 3. 设置 Pod MTU
   ```

2. **不同环境的 MTU 配置**：
   ```yaml
   # AWS：EC2 实例默认 MTU 9001（Jumbo Frame）
   # Pod MTU: 9001 - 50 = 8951
   calicoNetwork:
     mtu: 8951
   
   # Azure：VM 默认 MTU 1500
   # Pod MTU: 1500 - 50 = 1450
   calicoNetwork:
     mtu: 1450
   
   # Google Cloud：VM 默认 MTU 1460
   # Pod MTU: 1460 - 50 = 1410
   calicoNetwork:
     mtu: 1410
   
   # 裸金属（Host-GW 模式）：无封装开销
   # Pod MTU: 1500
   calicoNetwork:
     mtu: 1500
   ```

3. **验证 MTU 配置的自动化测试**：
   ```bash
   # 部署 MTU 测试 DaemonSet
   cat <<EOF | kubectl apply -f -
   apiVersion: apps/v1
   kind: DaemonSet
   metadata:
     name: mtu-test
     namespace: kube-system
   spec:
     selector:
       matchLabels:
         app: mtu-test
     template:
       metadata:
         labels:
           app: mtu-test
       spec:
         hostNetwork: true
         containers:
         - name: test
           image: busybox
           command:
           - /bin/sh
           - -c
           - |
             while true; do
               # 获取 Pod CIDR
               pod_cidr=\$(ip route | grep -oP '10\.244\.\d+\.\d+/\d+' | head -1)
               if [ -n "\$pod_cidr" ]; then
                 # 测试到其他节点 Pod 的 MTU
                 for target in \$(ip route | grep -oP '10\.244\.\d+\.\d+/\d+'); do
                   target_ip=\$(echo \$target | cut -d/ -f1)
                   max_mtu=1450
                   if ! ping -M do -s \$((max_mtu - 28)) -c 1 -W 1 \$target_ip > /dev/null 2>&1; then
                     echo "❌ MTU test FAILED to \$target_ip (expected \$max_mtu)"
                   fi
                 done
               fi
               sleep 300  # 每 5 分钟测试一次
             done
   EOF
   ```

4. **监控 MTU 相关问题**：
   ```yaml
   # Prometheus 告警规则
   - alert: MTUFragmentationDetected
     expr: increase(node_netstat_IpExt_InMcastPkts{type="FragFails"}[5m]) > 100
     for: 5m
     labels:
       severity: warning
     annotations:
       summary: "检测到 IP 分片失败"
       description: "节点 {{ $labels.node }} 检测到大量 IP 分片失败，可能是 MTU 配置问题"
   ```

#### 💡 经验总结
- **配置错误**：未根据封装类型调整 Pod MTU
- **测试不足**：仅测试小包连通性，未测试大包传输
- **文档缺失**：运维人员不了解 Overlay 网络的 MTU 影响
- **改进方向**：自动检测 MTU、环境适配配置、自动化测试、监控告警
```
