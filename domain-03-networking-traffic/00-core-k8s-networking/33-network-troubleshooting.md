---
title: 33 - 网络故障诊断与链路排查 (Network Troubleshooting & Data Path Diagnosis)
description: '# 33 - 网络故障诊断与链路排查 (Network Troubleshooting & Data Path Diagnosis)'
category: networking
tags:
- k8s
- networking
- service
- ingress
- cni
- cilium
- flannel
- calico
- coredns
- networkpolicy
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 网络工程师
- 运维工程师
estimated_read_time: 5min
intent_queries:
- 网络故障诊断与链路排查 (Network Troubleshooting & Data Path Diagnosis) 是什么
- 如何 网络故障诊断与链路排查 (Network Troubleshooting & Data Path Diagnosis)
- Kubernetes 5 networking 最佳实践
- 网络故障诊断与链路排查 (Network Troubleshooting & Data Path Diagnosis) 故障排查
- 网络故障诊断与链路排查 (Network Troubleshooting & Data Path Diagnosis) 排障步骤
trigger_keywords:
- 网络故障诊断与链路排查
- Network
- Troubleshooting
- Data
- Path
- Diagnosis
- networking
prerequisites:
- kubectl-basics
- networking-basics
- ebpf-basics
- cilium-basics
- cni-basics
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
authors:
- name: KUDIG Team
  role: contributor
cross_refs:
- type: domain
  path: ../domain-01-cluster-fundamentals/
  label: '相关知识域: domain-01-cluster-fundamentals'
- type: domain
  path: ../domain-03-networking-traffic/
  label: '相关知识域: domain-03-networking-traffic'
- type: domain
  path: ../domain-06-observability/
  label: '相关知识域: domain-06-observability'
- type: fta
  path: ../domain-10-troubleshooting-diagnostics/topic-fta/list/networkpolicy-fta.md
  label: '故障树: networkpolicy'
- type: cheatsheet
  path: ../domain-17-system-foundation/topic-cheat-sheet/networking.md
  label: '速查卡: networking'
created: "2026-05-23"
---

# 33 - 网络故障诊断与链路排查 (Network Troubleshooting & Data Path Diagnosis)

> **适用版本**: [[Kubernetes|Kubernetes]] v1.25-v1.32 | **最后更新**: 2026-03 | **定位**: 场景化快速参考，适合问题现场速查

---

<!-- chunk: 1. 数据路径概览 (Data Path Overview) -->
## 1. 数据路径概览 (Data Path Overview)

### 1.1 Kubernetes 网络通信全路径

```
Pod A (eth0, 10.244.1.5)
    │
    ▼
veth pair (宿主机侧: caliXXXX / vethXXXX)
    │
    ├─ 同节点 Pod ──▶ bridge(cni0) / 路由表 ──▶ veth ──▶ Pod B
    │
    ├─ 跨节点 Pod ──▶ VXLAN(flannel.1) / IPIP(tunl0) / BGP路由 ──▶ eth0 ──▶ 物理网络
    │                                                                     │
    │                                                              对端节点 eth0
    │                                                                     │
    │                                                              解封装 ──▶ veth ──▶ Pod B
    │
    ├─ Service ──▶ iptables DNAT (KUBE-SVC-*) ──▶ 选择后端 Pod ──▶ 同/跨节点路径
    │
    └─ External ──▶ iptables SNAT (MASQUERADE) ──▶ eth0 ──▶ 默认网关 ──▶ 互联网
```

### 1.2 各 CNI 封装开销与 MTU 计算

| CNI 模式 | 封装协议 | 封装开销 | Pod MTU (物理 1500) | 封装端口/协议 |
|----------|---------|---------|---------------------|--------------|
| Flannel VXLAN | UDP/VXLAN | 50B | **1450** | UDP 4789 |
| Calico VXLAN | UDP/VXLAN | 50B | **1450** | UDP 4789 |
| Calico IPIP | IP-in-IP | 20B | **1480** | IP 协议 4 |
| Calico BGP | 无封装 | 0B | **1500** | TCP 179 |
| [[Cilium|Cilium]] VXLAN | UDP/VXLAN | 50B | **1450** | UDP 8472 |
| Cilium Native | 无封装 | 0B | **1500** | - |
| WireGuard | UDP/WG | 60B | **1440** | UDP 51820 |

### 1.3 网络问题分类速查

| 类型 | 症状 | 常见原因 | 首选排查手段 |
|-----|------|---------|-------------|
| DNS解析失败 | 无法解析服务名 | CoreDNS问题/策略阻断 | nslookup + [[CoreDNS|CoreDNS]] logs |
| Pod间不通 | 跨节点通信失败 | CNI问题/NetworkPolicy | 多跳 tcpdump |
| Service不通 | ClusterIP无响应 | Endpoints为空/kube-proxy | iptables/IPVS 检查 |
| 外部访问失败 | 无法访问外网 | NAT/防火墙/策略 | SNAT 规则 + 安全组 |
| 延迟高 | 响应慢 | 网络拥塞/MTU问题 | iperf3 + MTU 测试 |
| 随机丢包 | 间歇性超时 | conntrack表满/ARP表满 | conntrack -S + dmesg |
| 大包丢失 | HTTPS 握手后卡死 | MTU 不匹配 | ping -M do -s 测试 |

---

<!-- chunk: 2. 诊断工具箱 (Diagnostic Toolkit) -->
## 2. 诊断工具箱 (Diagnostic Toolkit)

### 2.1 netshoot 工具 Pod

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: netshoot
spec:
  containers:
  - name: debug
    image: nicolaka/netshoot:latest
    command: ["sleep", "infinity"]
    securityContext:
      capabilities:
        add: ["NET_ADMIN", "NET_RAW"]
  # 指定在特定节点排查
  # nodeName: <target-node>
```

### 2.2 常用诊断工具速查

| 工具 | 用途 | 关键命令 |
|------|------|---------|
| **ping** | L3 连通性 | `ping -c 3 <ip>` |
| **traceroute/mtr** | 路由跟踪 | `mtr -c 10 -r <ip>` |
| **curl** | L7 HTTP 测试 | `curl -so /dev/null -w "%{http_code} %{time_total}s" <url>` |
| **nc/ncat** | 端口连通性 | `nc -zv <host> <port>` |
| **tcpdump** | 抓包分析 | `tcpdump -i <iface> -nn host <ip>` |
| **nsenter** | 进入网络命名空间 | `nsenter -t <pid> -n <cmd>` |
| **conntrack** | 连接跟踪 | `conntrack -L -s <ip>` |
| **iptables** | 规则检查 | `iptables -t nat -L -n -v` |
| **ipvsadm** | IPVS 检查 | `ipvsadm -Ln` |
| **nslookup/dig** | DNS 测试 | `nslookup <name> <dns-ip>` |
| **iperf3** | 带宽测试 | `iperf3 -c <server>` |
| **ethtool** | 网卡状态 | `ethtool -S <iface>` |

---

<!-- chunk: 3. 场景化排查速查 (Scenario-Based Quick Reference) -->
## 3. 场景化排查速查 (Scenario-Based Quick Reference)

### 3.1 DNS 诊断

```bash
# 测试DNS解析
kubectl exec -it netshoot -- nslookup kubernetes.default
kubectl exec -it netshoot -- nslookup google.com

# 检查DNS配置
kubectl exec -it netshoot -- cat /etc/resolv.conf

# 查看CoreDNS状态和日志
kubectl get pods -n kube-system -l k8s-app=kube-dns -o wide
kubectl logs -n kube-system -l k8s-app=kube-dns --tail=50

# DNS性能测试（100次解析）
kubectl exec -it netshoot -- bash -c \
  "for i in {1..100}; do nslookup kubernetes.default > /dev/null 2>&1; done; echo Done"

# ndots 问题: 如果非 FQDN 解析慢（5s延迟），检查 ndots
kubectl exec -it netshoot -- cat /etc/resolv.conf | grep ndots
# ndots:5 会导致先尝试 search 域（最多4次超时后才查外部域名）
# 解决: 使用 FQDN（末尾加 .）或设置 dnsConfig ndots:2
```

### 3.2 连通性诊断 - 按场景

```bash
# ========== Pod-to-Pod（同节点）==========
kubectl exec -it netshoot -- ping -c 3 <target-pod-ip>
# 不通 → 检查 veth pair 状态和 bridge/路由

# ========== Pod-to-Pod（跨节点）==========
kubectl exec -it netshoot -- ping -c 3 <cross-node-pod-ip>
kubectl exec -it netshoot -- traceroute -n <cross-node-pod-ip>
# 不通 → 多跳抓包（见 Section 4）

# ========== Pod-to-Node ==========
kubectl exec -it netshoot -- ping -c 3 <node-ip>
# 不通 → 检查 rp_filter（见 Section 5 内核参数）

# ========== Pod-to-Service ==========
kubectl exec -it netshoot -- curl -v <service-name>:<port>
kubectl exec -it netshoot -- curl -v <cluster-ip>:<port>
# 不通 → 检查 Endpoints 和 iptables/IPVS

# ========== Pod-to-External ==========
kubectl exec -it netshoot -- curl -v https://www.aliyun.com
# 不通 → 检查 SNAT/masquerade 和默认路由

# ========== Node-to-Node ==========
ping -c 5 <other-node-ip>
traceroute -n <other-node-ip>
# 不通 → 检查安全组和物理网络
```

### 3.3 CNI 诊断

```bash
# Calico 诊断
calicoctl node status                # BGP 邻居状态
calicoctl get ippool -o wide         # IP 池
calicoctl get workloadEndpoint -o wide  # 工作负载端点

# Cilium 诊断
cilium status                        # 整体状态
cilium connectivity test             # 连通性测试
cilium monitor --type drop           # 丢包监控
cilium bpf endpoint list             # eBPF 端点

# 通用检查
ls /etc/cni/net.d/
cat /etc/cni/net.d/10-*.conflist
ls /opt/cni/bin/
```

### 3.4 kube-proxy 与 Service 诊断

```bash
# kube-proxy 模式
kubectl get cm kube-proxy -n kube-system -o yaml | grep mode

# iptables 模式
iptables -t nat -L KUBE-SERVICES -n -v | head -30
iptables -t nat -L KUBE-SVC-XXXX -n -v    # 特定 Service
iptables -t nat -L KUBE-SEP-XXXX -n -v    # 特定 Endpoint

# IPVS 模式
ipvsadm -Ln
ipvsadm -Ln --stats

# Endpoints 检查
kubectl get endpoints <svc-name> -n <namespace>
```

### 3.5 NetworkPolicy 诊断

```bash
# 查看策略
kubectl get networkpolicy -A -o wide
kubectl describe networkpolicy <name> -n <namespace>

# Calico 策略
calicoctl get networkpolicy -A -o wide
calicoctl get globalnetworkpolicy -o wide

# Cilium 策略（最佳）
cilium policy get
cilium monitor --type policy-verdict   # 实时策略判定
hubble observe --verdict DROPPED       # 被策略丢弃的流量
```

---

<!-- chunk: 4. 多跳抓包与 iptables TRACE (Multi-Hop Capture & TRACE) -->
## 4. 多跳抓包与 iptables TRACE (Multi-Hop Capture & TRACE)

### 4.1 多跳 tcpdump 并行抓包

跨节点通信失败时，需要在路径上每个接口同时抓包。

```bash
# ========== 源节点 ==========
# 跳1: Pod 的 veth pair
tcpdump -i <src-veth> -nn host <dst-pod-ip> -c 20 &

# 跳2: 隧道接口
tcpdump -i flannel.1 -nn host <dst-pod-ip> -c 20 &  # VXLAN
# 或
tcpdump -i eth0 -nn host <dst-node-ip> and udp port 4789 -c 20 &

# ========== 目的节点 ==========
# 跳3: 物理口
tcpdump -i eth0 -nn host <src-node-ip> and udp port 4789 -c 20 &

# 跳4: Pod 的 veth pair
tcpdump -i <dst-veth> -nn host <src-pod-ip> -c 20 &

# ===== 触发流量 =====
kubectl exec -it <src-pod> -- ping -c 5 <dst-pod-ip>

# 对比: 哪一跳丢失 → 定位问题层
```

### 4.2 定位 Pod 对应的 veth

```bash
# 方法1: 通过 Pod 内 ifindex
POD_IFINDEX=$(kubectl exec -it <pod> -- cat /sys/class/net/eth0/iflink 2>/dev/null | tr -d '\r')
ip link show | grep "^${POD_IFINDEX}:"

# 方法2: 通过 crictl
CONTAINER_ID=$(crictl ps --name <container> -q)
PID=$(crictl inspect $CONTAINER_ID | jq '.info.pid')
nsenter -t $PID -n ip link show eth0
```

### 4.3 iptables TRACE

```bash
# 启用追踪
modprobe nf_log_ipv4
iptables -t raw -A PREROUTING -s <src-ip> -d <dst-ip> -j TRACE
iptables -t raw -A OUTPUT -s <src-ip> -d <dst-ip> -j TRACE

# 查看输出
dmesg -w | grep TRACE
# 格式: TRACE: <table>:<chain>:<rule|policy>:<num> IN=<iface> OUT=<iface> SRC=... DST=...
# 最后一条 TRACE 后的 DROP/REJECT 即为丢包位置

# ⚠️ 清理
iptables -t raw -D PREROUTING -s <src-ip> -d <dst-ip> -j TRACE
iptables -t raw -D OUTPUT -s <src-ip> -d <dst-ip> -j TRACE
```

---

<!-- chunk: 5. 关键内核参数 (Critical Kernel Parameters) -->
## 5. 关键内核参数 (Critical Kernel Parameters)

| 参数 | K8s 推荐 | 错误影响 | 检查 |
|------|---------|---------|------|
| `net.ipv4.ip_forward` | **1** | Pod 跨节点不通 | `sysctl net.ipv4.ip_forward` |
| `net.bridge.bridge-nf-call-iptables` | **1** | Service ClusterIP 不通 | `sysctl net.bridge.bridge-nf-call-iptables` |
| `net.ipv4.conf.all.rp_filter` | **0/2** | Pod→Node 回包丢弃 | `sysctl net.ipv4.conf.all.rp_filter` |
| `net.netfilter.nf_conntrack_max` | **262144+** | conntrack 满，随机丢包 | `sysctl net.netfilter.nf_conntrack_max` |
| `net.ipv4.neigh.default.gc_thresh3` | **8192** | 大集群 ARP 溢出 | `sysctl net.ipv4.neigh.default.gc_thresh3` |

```bash
# 一键检查
for p in net.ipv4.ip_forward net.bridge.bridge-nf-call-iptables \
    net.ipv4.conf.all.rp_filter net.netfilter.nf_conntrack_max \
    net.netfilter.nf_conntrack_count net.ipv4.neigh.default.gc_thresh3; do
    printf "%-50s %s\n" "$p" "$(sysctl -n $p 2>/dev/null || echo N/A)"
done

# conntrack 使用率
CT_C=$(sysctl -n net.netfilter.nf_conntrack_count 2>/dev/null)
CT_M=$(sysctl -n net.netfilter.nf_conntrack_max 2>/dev/null)
echo "conntrack: $CT_C / $CT_M ($((CT_C*100/CT_M))%)"
```

---

<!-- chunk: 6. MTU 与性能诊断 (MTU & Performance) -->
## 6. MTU 与性能诊断 (MTU & Performance)

### 6.1 MTU 问题诊断

```bash
# 检查各接口 MTU
ip link show | grep mtu

# Pod 内 MTU
kubectl exec -it netshoot -- ip link show eth0

# MTU 路径发现（DF 标志，不分片）
kubectl exec -it netshoot -- ping -M do -s 1472 <target>  # 1500 MTU 物理网络
kubectl exec -it netshoot -- ping -M do -s 1422 <target>  # 1450 MTU VXLAN

# 如果 1472 超时但 1422 正常 → 说明路径上有 VXLAN 封装导致 MTU 减小
# 二分法定位精确 MTU:
kubectl exec -it netshoot -- ping -M do -s 1450 <target>
kubectl exec -it netshoot -- ping -M do -s 1440 <target>
```

### 6.2 性能测试

```bash
# 带宽测试 (iperf3)
# 服务端:
kubectl exec -it iperf-server -- iperf3 -s
# 客户端:
kubectl exec -it iperf-client -- iperf3 -c <server-ip>
# UDP 测试:
kubectl exec -it iperf-client -- iperf3 -c <server-ip> -u -b 1G

# 延迟测试
kubectl exec -it netshoot -- hping3 -S -p 80 -c 10 <target>

# 并发连接测试
kubectl exec -it netshoot -- ab -n 1000 -c 100 http://<service>/
```

---

<!-- chunk: 7. 常见问题速查表 (Quick Reference) -->
## 7. 常见问题速查表 (Quick Reference)

| 问题 | 首选诊断 | 可能原因 | 解决 |
|-----|---------|---------|------|
| DNS超时(5s) | `cat /etc/resolv.conf` | ndots:5 + 非 FQDN | 设 ndots:2 或用 FQDN |
| 跨节点不通 | 多跳 tcpdump | CNI隧道/安全组 | 放行 CNI 端口 |
| Service不通 | `kubectl get ep` | Endpoints为空 | 修复 selector |
| 随机超时 | `conntrack -S` | conntrack表满 | 调大 nf_conntrack_max |
| 性能差 | `iperf3` | MTU/带宽限制 | 修正 MTU |
| HTTPS 握手卡 | `ping -M do -s` | MTU 不匹配 | 统一 Pod MTU |
| Pod→Node 不通 | `sysctl rp_filter` | 反向路径过滤 | rp_filter=0 或 2 |
| 新节点不通 | `ip neigh show` | ARP 表满 | 调大 gc_thresh |
| 间歇性重连 | `dmesg \| grep conntrack` | conntrack 竞争 | 升级 eBPF CNI |

---

<!-- chunk: 8. ACK 网络诊断 (ACK-Specific) -->
## 8. ACK 网络诊断 (ACK-Specific)

| 工具 | 说明 |
|-----|------|
| 节点诊断 | 控制台一键诊断 |
| Terway诊断 | `terway-cli` |
| 网络拓扑 | 可视化网络拓扑 |
| 日志服务 | 网络日志分析 |

---

<!-- chunk: Obsidian 相关文档 -->
## Obsidian 相关文档

- domain-03-networking-traffic MOC
- [[domain-03-networking-traffic/README.md|Domain 5: Networking 网络]]
- Kubernetes 网络基础 Network in a Nutshell
- Domain-5 网络 — 开源项目索引
- FAQ 文档
- 网络核心组件
- CNI 架构与核心原理
- 76 - CNI插件深度对比
- 142 - Flannel 完整指南 (Flannel Complete Guide)
- Flannel WireGuard 加密后端配置
- Flannel IPv6 Dual Stack 支持
- Flannel Windows 节点支持
- [[domain-10-troubleshooting-diagnostics/topic-fta/list/apiserver-fta.md|API Server 异常故障树分析]]
- [[domain-10-troubleshooting-diagnostics/topic-fta/list/backup-restore-fta.md|备份/恢复异常故障树分析]]
- [[domain-10-troubleshooting-diagnostics/topic-fta/list/calico-fta.md|calico FTA 树：Calico CNI 故障诊断]]

## See Also

- 31-multi-cluster-federation
- 32-multi-cluster-networking
- 34-network-performance-tuning
- 35-gateway-api-overview

## Related

- [[domain-19-landscape-references/topic-index/terway-index|Terway 知识图谱索引]]
