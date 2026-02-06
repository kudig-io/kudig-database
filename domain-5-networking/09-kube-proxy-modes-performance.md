# Kube-proxy 实现模式与性能优化 (Kube-proxy Modes & Performance)

> **适用版本**: Kubernetes v1.25 - v1.32  
> **文档版本**: v2.0 | 生产级 kube-proxy 配置参考  
> **最后更新**: 2026-01

## Kube-proxy 整体架构

```
┌─────────────────────────────────────────────────────────────────────────────────────────┐
│                          Kube-proxy Architecture Overview                               │
├─────────────────────────────────────────────────────────────────────────────────────────┤
│                                                                                         │
│  ┌───────────────────────────────────────────────────────────────────────────────────┐ │
│  │                           Kubernetes Control Plane                                 │ │
│  │                                                                                    │ │
│  │  ┌─────────────────────────────────────────────────────────────────────────────┐ │ │
│  │  │                          API Server                                          │ │ │
│  │  │                                                                              │ │ │
│  │  │  ┌─────────────────┐  ┌─────────────────┐  ┌─────────────────┐             │ │ │
│  │  │  │     Service     │  │  EndpointSlice  │  │      Node       │             │ │ │
│  │  │  │    Objects      │  │    Objects      │  │    Objects      │             │ │ │
│  │  │  └─────────────────┘  └─────────────────┘  └─────────────────┘             │ │ │
│  │  └─────────────────────────────────────────────────────────────────────────────┘ │ │
│  └───────────────────────────────────────────────────────────────────────────────────┘ │
│                                          │                                              │
│                                          │ Watch & List                                │
│                                          ▼                                              │
│  ┌───────────────────────────────────────────────────────────────────────────────────┐ │
│  │                              kube-proxy (per Node)                                 │ │
│  │                                                                                    │ │
│  │  ┌─────────────────────────────────────────────────────────────────────────────┐ │ │
│  │  │                           Informer/Reflector                                 │ │ │
│  │  │                                                                              │ │ │
│  │  │  • Watch Service changes                                                    │ │ │
│  │  │  • Watch EndpointSlice changes                                              │ │ │
│  │  │  • Watch Node changes                                                       │ │ │
│  │  │  • Maintain local cache                                                     │ │ │
│  │  └─────────────────────────────────────────────────────────────────────────────┘ │ │
│  │                                          │                                        │ │
│  │                                          │ Sync Rules                            │ │
│  │                                          ▼                                        │ │
│  │  ┌─────────────────────────────────────────────────────────────────────────────┐ │ │
│  │  │                           Proxy Mode Handler                                 │ │ │
│  │  │                                                                              │ │ │
│  │  │  ┌─────────────────┐  ┌─────────────────┐  ┌─────────────────┐             │ │ │
│  │  │  │   iptables      │  │     IPVS        │  │   nftables      │             │ │ │
│  │  │  │   Proxier       │  │    Proxier      │  │   Proxier       │             │ │ │
│  │  │  │                 │  │                 │  │   (Alpha)       │             │ │ │
│  │  │  │  • DNAT rules   │  │  • Virtual      │  │  • nft rules    │             │ │ │
│  │  │  │  • SNAT rules   │  │    Server       │  │  • Modern       │             │ │ │
│  │  │  │  • Chain-based  │  │  • Real Server  │  │    Netfilter    │             │ │ │
│  │  │  │  • O(n) lookup  │  │  • Hash table   │  │  • Improved     │             │ │ │
│  │  │  │                 │  │  • O(1) lookup  │  │    performance  │             │ │ │
│  │  │  └─────────────────┘  └─────────────────┘  └─────────────────┘             │ │ │
│  │  └─────────────────────────────────────────────────────────────────────────────┘ │ │
│  └───────────────────────────────────────────────────────────────────────────────────┘ │
│                                          │                                              │
│                                          │ Configure Kernel                            │
│                                          ▼                                              │
│  ┌───────────────────────────────────────────────────────────────────────────────────┐ │
│  │                              Linux Kernel Network Stack                            │ │
│  │                                                                                    │ │
│  │  ┌─────────────────────────────────────────────────────────────────────────────┐ │ │
│  │  │                              Netfilter Framework                             │ │ │
│  │  │                                                                              │ │ │
│  │  │   Packet Flow (iptables mode):                                              │ │ │
│  │  │   ┌──────────────────────────────────────────────────────────────────────┐ │ │ │
│  │  │   │                                                                       │ │ │ │
│  │  │   │   PREROUTING ──► KUBE-SERVICES ──► KUBE-SVC-xxx ──► KUBE-SEP-xxx     │ │ │ │
│  │  │   │       │                                                     │         │ │ │ │
│  │  │   │       │              (Service VIP match)      (DNAT to Pod IP)       │ │ │ │
│  │  │   │       │                                                     │         │ │ │ │
│  │  │   │       └─────────────────────────────────────────────────────┘         │ │ │ │
│  │  │   │                                                                       │ │ │ │
│  │  │   │   OUTPUT ──► KUBE-SERVICES ──► (same as above)                       │ │ │ │
│  │  │   │                                                                       │ │ │ │
│  │  │   │   POSTROUTING ──► KUBE-POSTROUTING ──► MASQUERADE (SNAT)             │ │ │ │
│  │  │   │                                                                       │ │ │ │
│  │  │   └──────────────────────────────────────────────────────────────────────┘ │ │ │
│  │  │                                                                              │ │ │
│  │  │   Packet Flow (IPVS mode):                                                  │ │ │
│  │  │   ┌──────────────────────────────────────────────────────────────────────┐ │ │ │
│  │  │   │                                                                       │ │ │ │
│  │  │   │   INPUT ──► IPVS Virtual Server (Hash Table Lookup) ──► Real Server  │ │ │ │
│  │  │   │                                                                       │ │ │ │
│  │  │   │   • O(1) complexity for service lookup                               │ │ │ │
│  │  │   │   • Direct routing or NAT mode                                        │ │ │ │
│  │  │   │   • Multiple load balancing algorithms                               │ │ │ │
│  │  │   │                                                                       │ │ │ │
│  │  │   └──────────────────────────────────────────────────────────────────────┘ │ │ │
│  │  └─────────────────────────────────────────────────────────────────────────────┘ │ │
│  └───────────────────────────────────────────────────────────────────────────────────┘ │
│                                                                                         │
└─────────────────────────────────────────────────────────────────────────────────────────┘
```

## Kube-proxy 工作模式对比

### 模式全面对比矩阵

| 特性 | userspace | iptables | IPVS | nftables |
|------|-----------|----------|------|----------|
| **状态** | 已废弃 | 默认 | 推荐 | Alpha (v1.29+) |
| **数据结构** | 用户态代理 | 链表规则 | 哈希表 | 规则集 |
| **查找复杂度** | O(n) | O(n) | O(1) | O(1) |
| **性能** | 很差 | 中等 | 优秀 | 优秀 |
| **大规模支持** | ❌ | < 5000 Svc | > 10000 Svc | > 10000 Svc |
| **负载均衡算法** | 轮询 | 随机 | 多种 | 多种 |
| **规则更新** | 全量 | 全量 | 增量 | 增量 |
| **连接追踪** | 用户态 | conntrack | conntrack | conntrack |
| **内核依赖** | 无 | iptables | ip_vs 模块 | nft |
| **调试难度** | 中 | 易 | 中 | 中 |
| **适用版本** | < v1.2 | 所有版本 | v1.8+ | v1.29+ |

### IPVS 负载均衡算法

| 算法 | 缩写 | 描述 | 适用场景 |
|------|------|------|---------|
| **轮询** | rr | 依次轮流分配 | 后端性能均匀 |
| **加权轮询** | wrr | 按权重轮流分配 | 后端性能不均 |
| **最少连接** | lc | 分配给连接数最少的 | 长连接服务 |
| **加权最少连接** | wlc | 考虑权重的最少连接 | 混合负载 |
| **局部性哈希** | lblc | 基于目标 IP 哈希 | 缓存服务 |
| **局部性最少连接** | lblcr | 结合局部性和最少连接 | 复杂缓存 |
| **源哈希** | sh | 基于源 IP 哈希 | 会话保持 |
| **目标哈希** | dh | 基于目标 IP 哈希 | 缓存代理 |
| **最短期望延迟** | sed | 最小预期延迟 | 低延迟要求 |
| **永不排队** | nq | 避免队列等待 | 实时服务 |

---

## iptables 模式详解

### iptables 规则链结构

```
┌─────────────────────────────────────────────────────────────────────────────────────────┐
│                         iptables Mode Rule Chain Structure                              │
├─────────────────────────────────────────────────────────────────────────────────────────┤
│                                                                                         │
│  ┌───────────────────────────────────────────────────────────────────────────────────┐ │
│  │                                nat table                                           │ │
│  │                                                                                    │ │
│  │  PREROUTING chain                                                                 │ │
│  │  ┌─────────────────────────────────────────────────────────────────────────────┐ │ │
│  │  │  -A PREROUTING -j KUBE-SERVICES                                              │ │ │
│  │  └─────────────────────────────────────────────────────────────────────────────┘ │ │
│  │                                        │                                          │ │
│  │                                        ▼                                          │ │
│  │  KUBE-SERVICES chain                                                              │ │
│  │  ┌─────────────────────────────────────────────────────────────────────────────┐ │ │
│  │  │  # For each Service ClusterIP:Port                                           │ │ │
│  │  │  -A KUBE-SERVICES -d 10.96.0.100/32 -p tcp --dport 80 -j KUBE-SVC-XXXX      │ │ │
│  │  │  -A KUBE-SERVICES -d 10.96.0.101/32 -p tcp --dport 443 -j KUBE-SVC-YYYY     │ │ │
│  │  │  ...                                                                          │ │ │
│  │  │  # NodePort rules                                                             │ │ │
│  │  │  -A KUBE-SERVICES -p tcp --dport 30080 -j KUBE-NODEPORTS                     │ │ │
│  │  └─────────────────────────────────────────────────────────────────────────────┘ │ │
│  │                                        │                                          │ │
│  │                                        ▼                                          │ │
│  │  KUBE-SVC-XXXX chain (per Service)                                               │ │
│  │  ┌─────────────────────────────────────────────────────────────────────────────┐ │ │
│  │  │  # Load balancing using probability                                          │ │ │
│  │  │  # 3 endpoints: 33.33%, 50%, 100%                                            │ │ │
│  │  │  -A KUBE-SVC-XXXX -m statistic --mode random --probability 0.33333           │ │ │
│  │  │      -j KUBE-SEP-AAAA                                                         │ │ │
│  │  │  -A KUBE-SVC-XXXX -m statistic --mode random --probability 0.50000           │ │ │
│  │  │      -j KUBE-SEP-BBBB                                                         │ │ │
│  │  │  -A KUBE-SVC-XXXX -j KUBE-SEP-CCCC                                           │ │ │
│  │  └─────────────────────────────────────────────────────────────────────────────┘ │ │
│  │                                        │                                          │ │
│  │                                        ▼                                          │ │
│  │  KUBE-SEP-AAAA chain (per Endpoint)                                              │ │
│  │  ┌─────────────────────────────────────────────────────────────────────────────┐ │ │
│  │  │  # DNAT to Pod IP:Port                                                        │ │ │
│  │  │  -A KUBE-SEP-AAAA -p tcp -j DNAT --to-destination 10.244.1.10:8080           │ │ │
│  │  └─────────────────────────────────────────────────────────────────────────────┘ │ │
│  │                                                                                    │ │
│  │  POSTROUTING chain                                                                │ │
│  │  ┌─────────────────────────────────────────────────────────────────────────────┐ │ │
│  │  │  -A POSTROUTING -j KUBE-POSTROUTING                                          │ │ │
│  │  │  -A KUBE-POSTROUTING -m mark --mark 0x4000/0x4000 -j MASQUERADE              │ │ │
│  │  └─────────────────────────────────────────────────────────────────────────────┘ │ │
│  └───────────────────────────────────────────────────────────────────────────────────┘ │
│                                                                                         │
└─────────────────────────────────────────────────────────────────────────────────────────┘
```

### iptables 模式配置

```yaml
# kube-proxy-iptables-config.yaml
apiVersion: kubeproxy.config.k8s.io/v1alpha1
kind: KubeProxyConfiguration
mode: "iptables"

# iptables 特定配置
iptables:
  # 伪装所有流量
  masqueradeAll: false
  # 伪装位
  masqueradeBit: 14
  # 本地流量检测模式
  localhostNodePorts: true
  # 最小同步周期
  minSyncPeriod: 1s
  # 同步周期
  syncPeriod: 30s

# 连接追踪配置
conntrack:
  # 最大连接数 (0 = 系统默认)
  maxPerCore: 32768
  min: 131072
  # TCP 连接超时
  tcpEstablishedTimeout: 24h0m0s
  tcpCloseWaitTimeout: 1h0m0s

# 集群 CIDR
clusterCIDR: "10.244.0.0/16"

# NodePort 地址
nodePortAddresses:
  - "0.0.0.0/0"

# 健康检查绑定地址
healthzBindAddress: "0.0.0.0:10256"

# 指标绑定地址
metricsBindAddress: "0.0.0.0:10249"
```

---

## IPVS 模式详解

### IPVS 架构图

```
┌─────────────────────────────────────────────────────────────────────────────────────────┐
│                              IPVS Mode Architecture                                     │
├─────────────────────────────────────────────────────────────────────────────────────────┤
│                                                                                         │
│  ┌───────────────────────────────────────────────────────────────────────────────────┐ │
│  │                              kube-proxy (IPVS mode)                                │ │
│  │                                                                                    │ │
│  │  ┌─────────────────────────────────────────────────────────────────────────────┐ │ │
│  │  │                           IPVS Proxier                                       │ │ │
│  │  │                                                                              │ │ │
│  │  │  1. Create dummy interface (kube-ipvs0)                                     │ │ │
│  │  │  2. Bind Service ClusterIPs to kube-ipvs0                                   │ │ │
│  │  │  3. Create IPVS virtual servers for each Service                            │ │ │
│  │  │  4. Add real servers (Pod IPs) to virtual servers                           │ │ │
│  │  │  5. Use iptables for SNAT/masquerade only                                   │ │ │
│  │  └─────────────────────────────────────────────────────────────────────────────┘ │ │
│  └───────────────────────────────────────────────────────────────────────────────────┘ │
│                                          │                                              │
│                                          ▼                                              │
│  ┌───────────────────────────────────────────────────────────────────────────────────┐ │
│  │                              Linux Kernel                                          │ │
│  │                                                                                    │ │
│  │  ┌─────────────────────────────────────────────────────────────────────────────┐ │ │
│  │  │                          kube-ipvs0 Interface                                │ │ │
│  │  │                                                                              │ │ │
│  │  │  IP Addresses (Service ClusterIPs):                                         │ │ │
│  │  │  • 10.96.0.1 (kubernetes)                                                   │ │ │
│  │  │  • 10.96.0.10 (kube-dns)                                                    │ │ │
│  │  │  • 10.96.0.100 (my-service)                                                 │ │ │
│  │  │  • ...                                                                       │ │ │
│  │  └─────────────────────────────────────────────────────────────────────────────┘ │ │
│  │                                                                                    │ │
│  │  ┌─────────────────────────────────────────────────────────────────────────────┐ │ │
│  │  │                        IPVS Virtual Server Table                             │ │ │
│  │  │                                                                              │ │ │
│  │  │  Virtual Server: 10.96.0.100:80 (TCP)                                       │ │ │
│  │  │  Scheduler: rr (Round Robin)                                                 │ │ │
│  │  │  ┌─────────────────────────────────────────────────────────────────────┐   │ │ │
│  │  │  │  Real Servers:                                                       │   │ │ │
│  │  │  │  • 10.244.1.10:8080  Weight=1  ActiveConn=5   InactConn=10          │   │ │ │
│  │  │  │  • 10.244.2.11:8080  Weight=1  ActiveConn=3   InactConn=8           │   │ │ │
│  │  │  │  • 10.244.3.12:8080  Weight=1  ActiveConn=4   InactConn=12          │   │ │ │
│  │  │  └─────────────────────────────────────────────────────────────────────┘   │ │ │
│  │  │                                                                              │ │ │
│  │  │  Virtual Server: 10.96.0.10:53 (UDP) - kube-dns                             │ │ │
│  │  │  Scheduler: rr                                                               │ │ │
│  │  │  ┌─────────────────────────────────────────────────────────────────────┐   │ │ │
│  │  │  │  Real Servers:                                                       │   │ │ │
│  │  │  │  • 10.244.0.5:53   Weight=1                                          │   │ │ │
│  │  │  │  • 10.244.0.6:53   Weight=1                                          │   │ │ │
│  │  │  └─────────────────────────────────────────────────────────────────────┘   │ │ │
│  │  └─────────────────────────────────────────────────────────────────────────────┘ │ │
│  │                                                                                    │ │
│  │  ┌─────────────────────────────────────────────────────────────────────────────┐ │ │
│  │  │                    Hash Table for O(1) Lookup                                │ │ │
│  │  │                                                                              │ │ │
│  │  │   [Hash(10.96.0.100:80)] ──► Virtual Server Entry                           │ │ │
│  │  │   [Hash(10.96.0.10:53)]  ──► Virtual Server Entry                           │ │ │
│  │  │   ...                                                                        │ │ │
│  │  └─────────────────────────────────────────────────────────────────────────────┘ │ │
│  └───────────────────────────────────────────────────────────────────────────────────┘ │
│                                                                                         │
└─────────────────────────────────────────────────────────────────────────────────────────┘
```

### IPVS 模式配置

```yaml
# kube-proxy-ipvs-config.yaml
apiVersion: kubeproxy.config.k8s.io/v1alpha1
kind: KubeProxyConfiguration
mode: "ipvs"

# IPVS 特定配置
ipvs:
  # 调度算法: rr, lc, dh, sh, sed, nq, wrr, wlc, lblc, lblcr
  scheduler: "rr"
  
  # 同步周期
  syncPeriod: 30s
  minSyncPeriod: 1s
  
  # TCP 超时 (秒)
  tcpTimeout: 0s
  tcpFinTimeout: 0s
  udpTimeout: 0s
  
  # 严格 ARP (推荐启用)
  strictARP: true
  
  # 排除的 CIDR (不通过 IPVS 转发)
  excludeCIDRs: []

# iptables 配置 (IPVS 模式仍需要部分 iptables 规则)
iptables:
  masqueradeAll: false
  masqueradeBit: 14
  minSyncPeriod: 1s
  syncPeriod: 30s

# 连接追踪
conntrack:
  maxPerCore: 32768
  min: 131072
  tcpEstablishedTimeout: 24h0m0s
  tcpCloseWaitTimeout: 1h0m0s

# 集群配置
clusterCIDR: "10.244.0.0/16"
nodePortAddresses:
  - "0.0.0.0/0"

# 监控配置
healthzBindAddress: "0.0.0.0:10256"
metricsBindAddress: "0.0.0.0:10249"

# 日志级别
logging:
  verbosity: 2
```

### 切换到 IPVS 模式

```bash
#!/bin/bash
# switch-to-ipvs.sh - 切换 kube-proxy 到 IPVS 模式

set -euo pipefail

log() {
    echo "[$(date '+%Y-%m-%d %H:%M:%S')] $1"
}

# Step 1: 检查并加载内核模块
load_ipvs_modules() {
    log "Loading IPVS kernel modules..."
    
    local modules=(
        ip_vs
        ip_vs_rr
        ip_vs_wrr
        ip_vs_sh
        nf_conntrack
    )
    
    for mod in "${modules[@]}"; do
        if ! lsmod | grep -q "^${mod}"; then
            modprobe "$mod"
            log "Loaded module: $mod"
        else
            log "Module already loaded: $mod"
        fi
    done
    
    # 持久化模块加载
    cat > /etc/modules-load.d/ipvs.conf << EOF
ip_vs
ip_vs_rr
ip_vs_wrr
ip_vs_sh
nf_conntrack
EOF
    
    log "IPVS modules loaded and persisted"
}

# Step 2: 安装 ipvsadm 工具
install_ipvsadm() {
    log "Installing ipvsadm..."
    
    if command -v apt-get &>/dev/null; then
        apt-get update && apt-get install -y ipvsadm
    elif command -v yum &>/dev/null; then
        yum install -y ipvsadm
    elif command -v dnf &>/dev/null; then
        dnf install -y ipvsadm
    fi
    
    log "ipvsadm installed"
}

# Step 3: 配置 strictARP (用于 MetalLB 等场景)
configure_strict_arp() {
    log "Configuring strict ARP..."
    
    # 启用严格 ARP
    cat > /etc/sysctl.d/99-ipvs.conf << EOF
net.ipv4.conf.all.arp_ignore = 1
net.ipv4.conf.all.arp_announce = 2
EOF
    
    sysctl --system
    
    log "Strict ARP configured"
}

# Step 4: 更新 kube-proxy ConfigMap
update_kube_proxy_config() {
    log "Updating kube-proxy ConfigMap..."
    
    kubectl get configmap kube-proxy -n kube-system -o yaml | \
        sed 's/mode: ""/mode: "ipvs"/' | \
        sed 's/mode: "iptables"/mode: "ipvs"/' | \
        kubectl apply -f -
    
    log "kube-proxy ConfigMap updated"
}

# Step 5: 重启 kube-proxy Pods
restart_kube_proxy() {
    log "Restarting kube-proxy pods..."
    
    kubectl delete pod -n kube-system -l k8s-app=kube-proxy
    
    # 等待 Pods 就绪
    kubectl wait --for=condition=ready pod -n kube-system -l k8s-app=kube-proxy --timeout=60s
    
    log "kube-proxy pods restarted"
}

# Step 6: 验证 IPVS 模式
verify_ipvs() {
    log "Verifying IPVS mode..."
    
    # 检查 kube-proxy 日志
    if kubectl logs -n kube-system -l k8s-app=kube-proxy --tail=20 | grep -q "Using ipvs Proxier"; then
        log "SUCCESS: kube-proxy is running in IPVS mode"
    else
        log "WARNING: Could not confirm IPVS mode from logs"
    fi
    
    # 检查 IPVS 规则
    log "IPVS rules:"
    ipvsadm -Ln | head -30
    
    # 检查 kube-ipvs0 接口
    log "kube-ipvs0 interface:"
    ip addr show kube-ipvs0 2>/dev/null || log "kube-ipvs0 not found"
}

# 清理旧的 iptables 规则
cleanup_iptables() {
    log "Cleaning up old iptables rules..."
    
    # 注意: 这会删除所有 KUBE-* 规则，确保在维护窗口执行
    iptables -t nat -F KUBE-SERVICES 2>/dev/null || true
    iptables -t nat -F KUBE-POSTROUTING 2>/dev/null || true
    iptables -t filter -F KUBE-FORWARD 2>/dev/null || true
    
    log "Old iptables rules cleaned up"
}

# Main
main() {
    log "Starting IPVS mode switch..."
    
    # 在所有节点上执行模块加载
    load_ipvs_modules
    install_ipvsadm
    configure_strict_arp
    
    # 更新配置并重启
    update_kube_proxy_config
    restart_kube_proxy
    
    # 验证
    sleep 10
    verify_ipvs
    
    log "IPVS mode switch completed!"
}

main "$@"
```

---

## 性能对比与基准测试

### 性能对比图

```
┌─────────────────────────────────────────────────────────────────────────────────────────┐
│                    Performance Comparison: iptables vs IPVS                             │
├─────────────────────────────────────────────────────────────────────────────────────────┤
│                                                                                         │
│  Rule Update Latency (ms) vs Number of Services                                        │
│                                                                                         │
│       │                                                                                 │
│  5000 ┤                                               iptables                         │
│       │                                           ___/                                  │
│  4000 ┤                                      ___/                                       │
│       │                                 ___/                                            │
│  3000 ┤                            ___/                                                 │
│       │                       ___/                                                      │
│  2000 ┤                  ___/                                                           │
│       │             ___/                                                                │
│  1000 ┤        ___/                                                                     │
│       │   ___/                                                                          │
│   500 ┤──────────────────────────────────────────────── IPVS                           │
│       │                                                                                 │
│     0 ┼─────────┬─────────┬─────────┬─────────┬─────────┬─────────┬─────────           │
│       0       1000      2000      3000      4000      5000      6000                    │
│                              Number of Services                                         │
│                                                                                         │
├─────────────────────────────────────────────────────────────────────────────────────────┤
│                                                                                         │
│  Connection Setup Latency (μs) vs Number of Services                                   │
│                                                                                         │
│       │                                                                                 │
│   200 ┤                                              iptables                          │
│       │                                          ___/                                   │
│   150 ┤                                     ___/                                        │
│       │                                ___/                                             │
│   100 ┤                           ___/                                                  │
│       │                      ___/                                                       │
│    50 ┤─────────────────────────────────────────────── IPVS                            │
│       │                                                                                 │
│     0 ┼─────────┬─────────┬─────────┬─────────┬─────────┬─────────                     │
│       0       2000      4000      6000      8000     10000                              │
│                              Number of Services                                         │
│                                                                                         │
└─────────────────────────────────────────────────────────────────────────────────────────┘
```

### 性能基准测试脚本

```bash
#!/bin/bash
# kube-proxy-benchmark.sh - kube-proxy 性能基准测试

set -euo pipefail

# 配置
NUM_SERVICES=${1:-100}
TEST_DURATION=${2:-60}
OUTPUT_DIR="/tmp/kube-proxy-benchmark"

mkdir -p "$OUTPUT_DIR"

log() {
    echo "[$(date '+%Y-%m-%d %H:%M:%S')] $1"
}

# 创建测试 Services
create_test_services() {
    local count=$1
    log "Creating $count test services..."
    
    for i in $(seq 1 "$count"); do
        cat <<EOF | kubectl apply -f -
apiVersion: v1
kind: Service
metadata:
  name: bench-svc-${i}
  namespace: default
spec:
  type: ClusterIP
  selector:
    app: bench-app-${i}
  ports:
    - port: 80
      targetPort: 8080
---
apiVersion: apps/v1
kind: Deployment
metadata:
  name: bench-app-${i}
  namespace: default
spec:
  replicas: 2
  selector:
    matchLabels:
      app: bench-app-${i}
  template:
    metadata:
      labels:
        app: bench-app-${i}
    spec:
      containers:
        - name: nginx
          image: nginx:alpine
          ports:
            - containerPort: 80
          resources:
            requests:
              cpu: 10m
              memory: 16Mi
            limits:
              cpu: 50m
              memory: 32Mi
EOF
    done
    
    log "Test services created"
}

# 测量规则更新延迟
measure_rule_update_latency() {
    log "Measuring rule update latency..."
    
    local start_time=$(date +%s%N)
    
    # 触发规则更新
    kubectl patch svc bench-svc-1 -p '{"spec":{"ports":[{"port":81,"targetPort":8080}]}}'
    
    # 等待规则同步
    sleep 2
    
    local end_time=$(date +%s%N)
    local latency=$(( (end_time - start_time) / 1000000 ))
    
    echo "Rule update latency: ${latency}ms"
    
    # 恢复
    kubectl patch svc bench-svc-1 -p '{"spec":{"ports":[{"port":80,"targetPort":8080}]}}'
}

# 测量连接延迟
measure_connection_latency() {
    log "Measuring connection latency..."
    
    # 部署测试客户端
    kubectl run bench-client --image=curlimages/curl --restart=Never -- sleep 3600 2>/dev/null || true
    kubectl wait --for=condition=ready pod/bench-client --timeout=60s
    
    # 测试连接延迟
    local results=""
    for i in $(seq 1 10); do
        local svc_ip=$(kubectl get svc bench-svc-${i} -o jsonpath='{.spec.clusterIP}')
        local latency=$(kubectl exec bench-client -- curl -o /dev/null -s -w '%{time_connect}\n' "http://${svc_ip}" 2>/dev/null)
        results+="${latency}\n"
    done
    
    echo -e "$results" | awk '{ sum += $1; n++ } END { printf "Average connection latency: %.6f seconds\n", sum/n }'
    
    kubectl delete pod bench-client --ignore-not-found
}

# 测量吞吐量
measure_throughput() {
    log "Measuring throughput..."
    
    local svc_ip=$(kubectl get svc bench-svc-1 -o jsonpath='{.spec.clusterIP}')
    
    # 使用 wrk 或 ab 进行压力测试
    kubectl run bench-wrk --image=skandyla/wrk --restart=Never -- -t2 -c100 -d${TEST_DURATION}s "http://${svc_ip}/" 2>/dev/null || true
    kubectl wait --for=condition=ready pod/bench-wrk --timeout=60s || true
    
    sleep $((TEST_DURATION + 10))
    
    kubectl logs bench-wrk
    kubectl delete pod bench-wrk --ignore-not-found
}

# 收集 kube-proxy 指标
collect_metrics() {
    log "Collecting kube-proxy metrics..."
    
    local node=$(kubectl get nodes -o jsonpath='{.items[0].metadata.name}')
    local proxy_pod=$(kubectl get pods -n kube-system -l k8s-app=kube-proxy -o jsonpath='{.items[0].metadata.name}')
    
    kubectl exec -n kube-system "$proxy_pod" -- curl -s http://localhost:10249/metrics > "$OUTPUT_DIR/kube-proxy-metrics.txt"
    
    # 解析关键指标
    grep "kubeproxy_sync_proxy_rules_duration_seconds" "$OUTPUT_DIR/kube-proxy-metrics.txt"
    grep "kubeproxy_sync_proxy_rules_last_timestamp_seconds" "$OUTPUT_DIR/kube-proxy-metrics.txt"
    grep "kubeproxy_network_programming_duration_seconds" "$OUTPUT_DIR/kube-proxy-metrics.txt"
}

# 清理测试资源
cleanup() {
    log "Cleaning up test resources..."
    
    kubectl delete deployment -l app --field-selector metadata.name!=kube-dns --ignore-not-found
    kubectl delete svc -l kubernetes.io/name!=kubernetes --ignore-not-found
    
    log "Cleanup completed"
}

# 生成报告
generate_report() {
    log "Generating report..."
    
    cat > "$OUTPUT_DIR/benchmark-report.md" <<EOF
# Kube-proxy Benchmark Report

## Test Configuration
- Number of Services: $NUM_SERVICES
- Test Duration: ${TEST_DURATION}s
- Timestamp: $(date)

## Results

### Rule Update Latency
$(cat "$OUTPUT_DIR/rule-update-latency.txt" 2>/dev/null || echo "N/A")

### Connection Latency
$(cat "$OUTPUT_DIR/connection-latency.txt" 2>/dev/null || echo "N/A")

### Throughput
$(cat "$OUTPUT_DIR/throughput.txt" 2>/dev/null || echo "N/A")

### Key Metrics
\`\`\`
$(cat "$OUTPUT_DIR/kube-proxy-metrics.txt" | grep -E "sync_proxy_rules|network_programming" | head -20)
\`\`\`
EOF

    log "Report generated: $OUTPUT_DIR/benchmark-report.md"
}

# Main
main() {
    log "Starting kube-proxy benchmark..."
    
    create_test_services "$NUM_SERVICES"
    
    sleep 30  # 等待所有 Pod 就绪
    
    measure_rule_update_latency > "$OUTPUT_DIR/rule-update-latency.txt"
    measure_connection_latency > "$OUTPUT_DIR/connection-latency.txt"
    measure_throughput > "$OUTPUT_DIR/throughput.txt"
    collect_metrics
    generate_report
    
    # cleanup  # 取消注释以自动清理
    
    log "Benchmark completed!"
}

main "$@"
```

---

## 监控与告警

### Prometheus 监控规则

```yaml
# kube-proxy-monitoring-rules.yaml
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: kube-proxy-rules
  namespace: monitoring
spec:
  groups:
    - name: kube-proxy.rules
      interval: 30s
      rules:
        # Recording rules
        - record: kube_proxy:sync_rules_duration:avg
          expr: |
            avg(rate(kubeproxy_sync_proxy_rules_duration_seconds_sum[5m]) / 
                rate(kubeproxy_sync_proxy_rules_duration_seconds_count[5m]))
        
        - record: kube_proxy:sync_rules_duration:p99
          expr: |
            histogram_quantile(0.99, sum(rate(kubeproxy_sync_proxy_rules_duration_seconds_bucket[5m])) by (le))
        
        - record: kube_proxy:network_programming_duration:avg
          expr: |
            avg(rate(kubeproxy_network_programming_duration_seconds_sum[5m]) /
                rate(kubeproxy_network_programming_duration_seconds_count[5m]))

    - name: kube-proxy.alerts
      rules:
        - alert: KubeProxyDown
          expr: |
            absent(up{job="kube-proxy"}) or up{job="kube-proxy"} == 0
          for: 5m
          labels:
            severity: critical
          annotations:
            summary: "kube-proxy is down"
            description: "kube-proxy on node {{ $labels.instance }} is not responding"
        
        - alert: KubeProxySyncRulesSlowIPTables
          expr: |
            kube_proxy:sync_rules_duration:p99 > 5
          for: 10m
          labels:
            severity: warning
          annotations:
            summary: "kube-proxy rule sync is slow"
            description: "kube-proxy P99 rule sync duration is {{ $value }}s. Consider switching to IPVS mode."
        
        - alert: KubeProxySyncRulesVerySlow
          expr: |
            kube_proxy:sync_rules_duration:p99 > 30
          for: 5m
          labels:
            severity: critical
          annotations:
            summary: "kube-proxy rule sync is very slow"
            description: "kube-proxy P99 rule sync duration is {{ $value }}s. Service routing may be affected."
        
        - alert: KubeProxyIPVSServicesSyncError
          expr: |
            increase(kubeproxy_sync_proxy_rules_iptables_restore_failures_total[5m]) > 0
          for: 5m
          labels:
            severity: warning
          annotations:
            summary: "kube-proxy iptables restore failures"
            description: "kube-proxy is experiencing iptables restore failures"
        
        - alert: KubeProxyEndpointChangesHigh
          expr: |
            sum(rate(kubeproxy_sync_proxy_rules_endpoint_changes_total[5m])) > 100
          for: 10m
          labels:
            severity: warning
          annotations:
            summary: "High rate of endpoint changes"
            description: "Endpoint changes rate is {{ $value }}/s. This may cause excessive kube-proxy activity."
```

### Grafana Dashboard JSON

```json
{
  "dashboard": {
    "title": "Kube-proxy Performance",
    "uid": "kube-proxy-perf",
    "panels": [
      {
        "title": "Rule Sync Duration (P99)",
        "type": "timeseries",
        "gridPos": {"h": 8, "w": 12, "x": 0, "y": 0},
        "targets": [
          {
            "expr": "histogram_quantile(0.99, sum(rate(kubeproxy_sync_proxy_rules_duration_seconds_bucket[5m])) by (le, instance))",
            "legendFormat": "{{ instance }}"
          }
        ],
        "fieldConfig": {
          "defaults": {
            "unit": "s"
          }
        }
      },
      {
        "title": "Sync Rules Rate",
        "type": "timeseries",
        "gridPos": {"h": 8, "w": 12, "x": 12, "y": 0},
        "targets": [
          {
            "expr": "sum(rate(kubeproxy_sync_proxy_rules_duration_seconds_count[5m])) by (instance)",
            "legendFormat": "{{ instance }}"
          }
        ]
      },
      {
        "title": "Network Programming Duration",
        "type": "timeseries",
        "gridPos": {"h": 8, "w": 12, "x": 0, "y": 8},
        "targets": [
          {
            "expr": "histogram_quantile(0.99, sum(rate(kubeproxy_network_programming_duration_seconds_bucket[5m])) by (le))",
            "legendFormat": "P99"
          },
          {
            "expr": "histogram_quantile(0.50, sum(rate(kubeproxy_network_programming_duration_seconds_bucket[5m])) by (le))",
            "legendFormat": "P50"
          }
        ],
        "fieldConfig": {
          "defaults": {
            "unit": "s"
          }
        }
      },
      {
        "title": "Service/Endpoint Count",
        "type": "stat",
        "gridPos": {"h": 4, "w": 6, "x": 12, "y": 8},
        "targets": [
          {
            "expr": "kubeproxy_sync_proxy_rules_service_changes_total",
            "legendFormat": "Services"
          }
        ]
      },
      {
        "title": "iptables Restore Failures",
        "type": "stat",
        "gridPos": {"h": 4, "w": 6, "x": 18, "y": 8},
        "targets": [
          {
            "expr": "sum(increase(kubeproxy_sync_proxy_rules_iptables_restore_failures_total[1h]))",
            "legendFormat": "Failures (1h)"
          }
        ],
        "fieldConfig": {
          "defaults": {
            "thresholds": {
              "mode": "absolute",
              "steps": [
                {"color": "green", "value": 0},
                {"color": "yellow", "value": 1},
                {"color": "red", "value": 10}
              ]
            }
          }
        }
      }
    ]
  }
}
```

---

## 故障排除

### 常用诊断命令

```bash
#!/bin/bash
# kube-proxy-debug.sh

# 检查 kube-proxy 状态
kubectl get pods -n kube-system -l k8s-app=kube-proxy -o wide

# 查看 kube-proxy 日志
kubectl logs -n kube-system -l k8s-app=kube-proxy --tail=100

# 检查当前模式
kubectl logs -n kube-system -l k8s-app=kube-proxy | grep -i "using.*proxier"

# 查看 kube-proxy 配置
kubectl get configmap kube-proxy -n kube-system -o yaml

# 检查 iptables 规则 (iptables 模式)
iptables -t nat -L KUBE-SERVICES -n -v | head -50
iptables -t nat -L -n | grep -c KUBE-  # 规则数量

# 检查 IPVS 规则 (IPVS 模式)
ipvsadm -Ln
ipvsadm -Ln --stats  # 带统计信息

# 检查 kube-ipvs0 接口
ip addr show kube-ipvs0

# 检查连接追踪
cat /proc/sys/net/netfilter/nf_conntrack_count
cat /proc/sys/net/netfilter/nf_conntrack_max
conntrack -L | head -20

# 检查 kube-proxy 指标
kubectl exec -n kube-system $(kubectl get pods -n kube-system -l k8s-app=kube-proxy -o jsonpath='{.items[0].metadata.name}') -- curl -s http://localhost:10249/metrics | grep kubeproxy

# 测试 Service 连通性
kubectl run test-curl --rm -it --image=curlimages/curl --restart=Never -- curl -v http://<service-ip>:<port>
```

### 常见问题和解决方案

| 问题 | 原因 | 解决方案 |
|------|------|---------|
| Service 无法访问 | kube-proxy 未运行 | 检查 DaemonSet 状态 |
| 规则同步慢 | Service 数量过多 | 切换到 IPVS 模式 |
| IPVS 模式启动失败 | 内核模块未加载 | `modprobe ip_vs ip_vs_rr ip_vs_sh` |
| conntrack 表满 | 连接数过多 | 增加 `nf_conntrack_max` |
| 源 IP 丢失 | SNAT 配置 | 使用 `externalTrafficPolicy: Local` |
| NodePort 不通 | 防火墙阻止 | 开放 30000-32767 端口 |
| 负载不均 | 算法选择不当 | 调整 IPVS scheduler |

---

## 版本变更记录

| K8s版本 | 变更内容 | 影响 |
|--------|---------|------|
| v1.32 | nftables 模式改进 | 更好的 nftables 支持 |
| v1.31 | IPVS 调度器增强 | 更多负载均衡选项 |
| v1.30 | EndpointSlice 性能优化 | 更快的规则同步 |
| v1.29 | nftables 模式 Alpha | 新的后端选项 |
| v1.28 | kube-proxy 指标改进 | 更详细的监控 |
| v1.27 | IPVS strictARP 默认启用 | 更好的 LB 兼容性 |
| v1.26 | Windows IPVS 支持 | 跨平台一致性 |
| v1.25 | 拓扑感知路由改进 | 更好的区域路由 |

---

> **参考文档**:  
> - [kube-proxy 官方文档](https://kubernetes.io/docs/reference/command-line-tools-reference/kube-proxy/)
> - [IPVS 模式](https://kubernetes.io/docs/concepts/services-networking/service/#proxy-mode-ipvs)
> - [Linux IPVS](http://www.linuxvirtualserver.org/Documents.html)

---

*Kusheet - Kubernetes 知识速查表项目*
