---
title: 网络基础知识体系
description: 云原生网络基础知识体系，覆盖 TCP/IP 协议栈、DNS、iptables/nftables、HTTP/HTTPS、负载均衡、网络抓包等核心网络基础
summary: 网络基础知识体系总索引，覆盖 TCP/IP、DNS、iptables/nftables、HTTP/HTTPS、负载均衡、抓包分析
category: index
tags:
- index
- networking
- tcp-ip
- dns
- iptables
- nftables
tier: core
created: '2026-07-02'
last_updated: 2026-07
difficulty: intermediate
audience:
- SRE
- 平台工程师
- 网络工程师
- 开发工程师
---

# 网络基础知识体系

> 本知识体系覆盖云原生工程师必备的网络基础知识，是理解 Kubernetes 网络模型、排查容器网络故障、优化服务通信的权威参考。

## 领域概述

网络是 Kubernetes 的核心基础设施，包括：

- **TCP/IP 协议栈**：四层模型、TCP 状态机、UDP、ICMP
- **DNS**：解析流程、CoreDNS、服务发现、DNS 调优
- **iptables/nftables**：包过滤、NAT、kube-proxy 实现
- **HTTP/HTTPS**：HTTP/1.1、HTTP/2、HTTP/3、TLS 握手
- **负载均衡**：L4/L7、算法、健康检查、连接保持
- **网络抓包**：tcpdump、Wireshark、eBPF 抓包

## 文档索引

| 文档 | 内容 | 难度 |
|------|------|------|
| [[系统基础/网络基础/01-tcp-ip-protocol-stack.md\|TCP/IP 协议栈]] | 四层模型、TCP 状态机、拥塞控制 | 中级 |
| [[系统基础/网络基础/02-dns-resolution-and-servicediscovery.md\|DNS 解析与服务发现]] | DNS 协议、CoreDNS、K8s DNS | 中级 |
| [[系统基础/网络基础/03-iptables-nftables-packet-filtering.md\|iptables/nftables 包过滤]] | 链/表/规则、NAT、kube-proxy | 高级 |
| [[系统基础/网络基础/04-http-https-protocols.md\|HTTP/HTTPS 协议]] | HTTP 版本演进、TLS、证书 | 中级 |
| [[系统基础/网络基础/05-load-balancing-fundamentals.md\|负载均衡基础]] | L4/L7 LB、算法、健康检查 | 中级 |
| [[系统基础/网络基础/06-network-troubleshooting-tools.md\|网络抓包与诊断工具]] | tcpdump、Wireshark、eBPF | 高级 |

## TCP/IP 协议栈核心知识

### 四层模型与数据封装

```
┌─────────────────────────────────────────────┐
│           应用层 (HTTP/DNS/gRPC)             │  ← 数据 (Data)
├─────────────────────────────────────────────┤
│           传输层 (TCP/UDP)                   │  ← 段 (Segment)
├─────────────────────────────────────────────┤
│           网络层 (IP/ICMP/ARP)              │  ← 包 (Packet)
├─────────────────────────────────────────────┤
│           链路层 (Ethernet/WiFi)            │  ← 帧 (Frame)
└─────────────────────────────────────────────┘
```

### TCP 三次握手与四次挥手

```
三次握手 (建立连接):
Client ──── SYN (seq=x) ────────────→ Server
Client ←── SYN+ACK (seq=y,ack=x+1) ── Server
Client ──── ACK (ack=y+1) ──────────→ Server

四次挥手 (断开连接):
Client ──── FIN (seq=u) ────────────→ Server    (FIN_WAIT_1)
Client ←── ACK (ack=u+1) ─────────── Server    (CLOSE_WAIT)
Client ←── FIN (seq=w) ───────────── Server    (LAST_ACK)
Client ──── ACK (ack=w+1) ──────────→ Server    (TIME_WAIT → 2MSL)
```

### TCP 状态机（11 种状态）

| 状态 | 含义 | K8s 关联 |
|------|------|----------|
| CLOSED | 初始/终态 | 连接未建立 |
| LISTEN | 服务端等待 | Service 监听 |
| SYN_SENT | 客户端发起连接 | Pod 发起请求 |
| SYN_RECEIVED | 服务端收到 SYN | 半连接队列 |
| ESTABLISHED | 连接已建立 | 正常通信 |
| FIN_WAIT_1 | 主动关闭方 | 优雅终止开始 |
| FIN_WAIT_2 | 等待对方 FIN | 优雅终止中间态 |
| CLOSE_WAIT | 被动关闭方 | **泄漏风险** |
| LAST_ACK | 等待最后 ACK | 连接即将关闭 |
| TIME_WAIT | 等待 2MSL | **端口耗尽风险** |
| CLOSING | 双方同时关闭 | 罕见场景 |

### TCP 关键参数（Linux sysctl）

```bash
# /etc/sysctl.d/99-tcp-tuning.conf
# 连接队列
net.core.somaxconn = 65535              # 全连接队列上限
net.ipv4.tcp_max_syn_backlog = 65535    # 半连接队列上限
net.core.netdev_max_backlog = 65535     # 网卡接收队列

# TIME_WAIT 优化
net.ipv4.tcp_tw_reuse = 1              # 允许复用 TIME_WAIT 连接
net.ipv4.tcp_max_tw_buckets = 1048576  # TIME_WAIT 上限
net.ipv4.tcp_fin_timeout = 15          # FIN_WAIT_2 超时

# 保活检测
net.ipv4.tcp_keepalive_time = 600      # 空闲多久发探测
net.ipv4.tcp_keepalive_intvl = 30      # 探测间隔
net.ipv4.tcp_keepalive_probes = 3      # 探测次数

# 拥塞控制
net.ipv4.tcp_congestion_control = bbr  # 使用 BBR 算法
net.core.default_qdisc = fq            # BBR 需要 fq 调度

# 缓冲区
net.core.rmem_max = 16777216           # 接收缓冲区上限
net.core.wmem_max = 16777216           # 发送缓冲区上限
net.ipv4.tcp_rmem = 4096 87380 16777216
net.ipv4.tcp_wmem = 4096 65536 16777216
```

### UDP vs TCP 对比

| 特性 | TCP | UDP |
|------|-----|-----|
| 连接 | 面向连接 | 无连接 |
| 可靠性 | 可靠传输 | 尽最大努力 |
| 顺序 | 保证有序 | 不保证 |
| 流控 | 滑动窗口 | 无 |
| 拥塞控制 | 有 | 无 |
| 头部开销 | 20-60 字节 | 8 字节 |
| K8s 用途 | HTTP/gRPC/etcd | DNS/Service/日志 |

## DNS 核心知识

### DNS 解析流程

```
Pod 内应用发起 DNS 查询
    │
    ▼
/etc/resolv.conf → nameserver (CoreDNS ClusterIP)
    │
    ▼
CoreDNS (kube-dns Service, 通常 10.96.0.10)
    │
    ├── 集群内域名 (*.cluster.local) → 直接解析
    ├── 外部域名 → 转发到上游 DNS
    └── 缓存命中 → 直接返回
```

### K8s DNS 记录类型

| 记录类型 | 格式 | 示例 |
|----------|------|------|
| Service A | `<svc>.<ns>.svc.cluster.local` | `nginx.default.svc.cluster.local` |
| Pod A | `<pod-ip-dashed>.<ns>.pod.cluster.local` | `10-244-1-5.default.pod.cluster.local` |
| Headless Service | 返回所有 Pod IP | 多条 A 记录 |
| StatefulSet Pod | `<pod-name>.<svc>.<ns>.svc.cluster.local` | `web-0.nginx.default.svc.cluster.local` |
| ExternalName | CNAME 到外部域名 | `db.example.com` |

### CoreDNS 配置（Corefile）

```yaml
apiVersion: v1
kind: ConfigMap
metadata:
  name: coredns
  namespace: kube-system
data:
  Corefile: |
    .:53 {
        errors
        health {
            lameduck 5s
        }
        ready
        kubernetes cluster.local in-addr.arpa ip6.arpa {
            pods insecure
            fallthrough in-addr.arpa ip6.arpa
            ttl 30
        }
        prometheus :9153
        forward . /etc/resolv.conf {
            max_concurrent 1000
        }
        cache 30 {
            success 9984 30
            denial 9984 5
        }
        loop
        reload
        loadbalance
    }
```

### DNS 故障排查命令

```bash
# 🟢 检查 Pod DNS 配置
kubectl exec -it <pod> -- cat /etc/resolv.conf

# 🟢 测试 DNS 解析
kubectl exec -it <pod> -- nslookup kubernetes.default.svc.cluster.local

# 🟢 检查 CoreDNS Pod 状态
kubectl get pods -n kube-system -l k8s-app=kube-dns -o wide

# 🟢 查看 CoreDNS 日志
kubectl logs -n kube-system -l k8s-app=kube-dns --tail=50

# 🟢 检查 CoreDNS Service
kubectl get svc -n kube-system kube-dns

# 🟡 重启 CoreDNS（滚动）
kubectl rollout restart deployment coredns -n kube-system

# 🟢 检查 ndots 配置影响
kubectl exec -it <pod> -- cat /etc/resolv.conf | grep ndots
```

## iptables/nftables 核心知识

### iptables 五表五链

```
表 (Table)          链 (Chain)              用途
─────────────────────────────────────────────────────
filter              INPUT                   入站包过滤
                    FORWARD                 转发包过滤
                    OUTPUT                  出站包过滤
nat                 PREROUTING              DNAT（目的地址转换）
                    INPUT                   入站 NAT
                    OUTPUT                  出站 NAT
                    POSTROUTING             SNAT/MASQUERADE
mangle              所有链                   修改包头
raw                 PREROUTING/OUTPUT       连接跟踪豁免
security            INPUT/FORWARD/OUTPUT    SELinux 标记
```

### kube-proxy iptables 模式工作原理

```
Pod 访问 Service ClusterIP:Port
    │
    ▼
PREROUTING/OUTPUT → KUBE-SERVICES 链
    │
    ▼
匹配 Service → KUBE-SVC-XXXX 链
    │
    ▼
概率选择后端 → KUBE-SEP-YYYY 链
    │
    ▼
DNAT 到 Pod IP:TargetPort
    │
    ▼
POSTROUTING → MASQUERADE（跨节点时）
```

### 关键 iptables 命令

```bash
# 🟢 查看 NAT 表规则（kube-proxy 生成）
iptables -t nat -L KUBE-SERVICES -n --line-numbers

# 🟢 查看 filter 表规则
iptables -L -n -v --line-numbers

# 🟢 查看特定 Service 的转发规则
iptables -t nat -L KUBE-SVC-XXXX -n -v

# 🟢 统计包计数
iptables -t nat -L -n -v -x

# 🟡 清空 NAT 表（危险！kube-proxy 会重建）
iptables -t nat -F

# 🟢 查看连接跟踪表
conntrack -L -p tcp --dport 80

# 🟢 连接跟踪统计
conntrack -C
cat /proc/sys/net/netfilter/nf_conntrack_count
cat /proc/sys/net/netfilter/nf_conntrack_max
```

### nftables vs iptables 对比

| 特性 | iptables | nftables |
|------|----------|----------|
| 内核接口 | 每规则一次系统调用 | 批量更新 |
| 规则查找 | 线性遍历 O(n) | 集合/映射 O(1) |
| 规则数量 | 大量规则性能差 | 万级规则无压力 |
| 语法 | 多命令分散 | 统一声明式 |
| K8s 支持 | kube-proxy 默认 | kube-proxy 1.31+ |
| 原子更新 | 不支持 | 支持 |

### 连接跟踪（conntrack）调优

```bash
# /etc/sysctl.d/99-conntrack.conf
net.netfilter.nf_conntrack_max = 1048576        # 最大连接数
net.netfilter.nf_conntrack_tcp_timeout_established = 3600  # ESTABLISHED 超时
net.netfilter.nf_conntrack_tcp_timeout_time_wait = 30     # TIME_WAIT 超时
net.netfilter.nf_conntrack_tcp_timeout_close_wait = 15    # CLOSE_WAIT 超时
net.netfilter.nf_conntrack_tcp_timeout_fin_wait = 30      # FIN_WAIT 超时

# 查看当前连接数
cat /proc/sys/net/netfilter/nf_conntrack_count

# 查看连接跟踪表满导致的丢包
dmesg | grep "nf_conntrack: table full"
netstat -s | grep "conntrack"
```

## HTTP/HTTPS 核心知识

### HTTP 版本对比

| 特性 | HTTP/1.1 | HTTP/2 | HTTP/3 |
|------|----------|--------|--------|
| 传输层 | TCP | TCP | QUIC (UDP) |
| 多路复用 | 无（管线化） | 有（流） | 有（流） |
| 队头阻塞 | 应用层+传输层 | 传输层 | 无 |
| 头部压缩 | 无 | HPACK | QPACK |
| 服务器推送 | 无 | 有 | 有 |
| 连接迁移 | 不支持 | 不支持 | 支持 |
| K8s 场景 | 传统服务 | Ingress/gRPC | 前沿 |

### TLS 1.3 握手流程

```
Client ──── ClientHello (supported_versions, key_share) ──→ Server
Client ←── ServerHello (key_share) + EncryptedExtensions ── Server
Client ←── Certificate + CertificateVerify ─────────────── Server
Client ←── Finished ────────────────────────────────────── Server
Client ──── Finished ─────────────────────────────────────→ Server
         [1-RTT 完成，开始传输应用数据]

0-RTT (会话恢复):
Client ──── ClientHello + EarlyData ──→ Server  [立即发送数据]
```

### 常见 HTTP 状态码（K8s 场景）

| 状态码 | 含义 | K8s 常见原因 |
|--------|------|-------------|
| 400 | 请求错误 | API 参数错误 |
| 401 | 未认证 | Token 过期/无效 |
| 403 | 无权限 | RBAC 配置错误 |
| 404 | 未找到 | Service/路由不存在 |
| 429 | 限流 | API Priority and Fairness |
| 500 | 服务器错误 | 应用异常 |
| 502 | 网关错误 | 后端 Pod 未就绪 |
| 503 | 服务不可用 | 所有 Pod 不健康 |
| 504 | 网关超时 | 后端响应超时 |

## 负载均衡核心知识

### L4 vs L7 负载均衡

| 特性 | L4 (传输层) | L7 (应用层) |
|------|------------|------------|
| 工作层 | TCP/UDP | HTTP/gRPC |
| 性能 | 极高（百万 CPS） | 中等 |
| 路由 | IP:Port | URL/Header/Cookie |
| TLS | 透传或终止 | 终止+重加密 |
| K8s 实现 | kube-proxy/MetalLB | Ingress/Gateway API |
| 典型产品 | LVS/F5 | Nginx/Envoy/HAProxy |

### 负载均衡算法

| 算法 | 原理 | 适用场景 |
|------|------|----------|
| 轮询 (RR) | 依次分配 | 后端性能均匀 |
| 加权轮询 (WRR) | 按权重分配 | 后端性能不均 |
| 最少连接 (LC) | 选连接最少的 | 长连接场景 |
| IP Hash | 源 IP 哈希 | 会话保持 |
| 一致性哈希 | 环形哈希 | 缓存/分片 |
| 随机 | 随机选择 | 简单场景 |

## 网络故障排查

### 排查流程

```
1. 确认症状 → 超时/拒绝/丢包/慢
    │
2. 分层排查
    ├── L2: 链路是否通 (ping/arping)
    ├── L3: 路由是否正确 (traceroute/ip route)
    ├── L4: 端口是否监听 (ss -tlnp/nc -zv)
    ├── L7: 应用是否正常 (curl -v)
    └── DNS: 解析是否正确 (nslookup/dig)
    │
3. 抓包分析
    └── tcpdump -i any -nn host <ip> and port <port>
    │
4. 定位根因 → 修复 → 验证
```

### 常用诊断命令

```bash
# 🟢 连通性测试
ping -c 4 <target>
traceroute -n <target>
mtr -n <target>

# 🟢 端口检查
ss -tlnp                    # 本机监听端口
ss -tnp state established   # 已建立连接
nc -zv <host> <port>        # 远程端口探测

# 🟢 DNS 诊断
dig <domain> A
dig @10.96.0.10 <svc>.default.svc.cluster.local
nslookup <domain>

# 🟢 HTTP 诊断
curl -vvv http://<svc>:<port>/
curl -o /dev/null -s -w '%{time_total}\n' http://<svc>/
curl --resolve <host>:443:<ip> https://<host>/

# 🟢 抓包
tcpdump -i any -nn -w /tmp/capture.pcap host <ip>
tcpdump -i eth0 -nn port 80 -A
tcpdump -i any -nn 'tcp[tcpflags] & (tcp-syn|tcp-fin) != 0'

# 🟢 路由检查
ip route show
ip route get <destination>
ip neigh show

# 🟢 连接跟踪
conntrack -L -p tcp --dport <port>
conntrack -D -p tcp --dport <port>  # 🟡 删除特定连接
```

### K8s 网络常见问题速查

| 症状 | 可能原因 | 排查命令 |
|------|----------|----------|
| Pod 无法解析 DNS | CoreDNS 异常/ndots 配置 | `kubectl exec -- nslookup kubernetes.default` |
| Service 无法访问 | Endpoints 为空 | `kubectl get endpoints <svc>` |
| 跨节点 Pod 不通 | CNI 路由/MTU 问题 | `ip route`, `ping <pod-ip>` |
| 连接超时 | NetworkPolicy 阻断 | `kubectl get networkpolicy -A` |
| 502 Bad Gateway | 后端 Pod 未就绪 | `kubectl get endpoints`, `kubectl logs` |
| 连接重置 | conntrack 表满 | `dmesg | grep conntrack` |
| 间歇性失败 | DNS 超时/竞态 | `kubectl exec -- dig +time=5 <svc>` |
| NodePort 不通 | 防火墙/kube-proxy | `iptables -L KUBE-NODEPORTS -n` |

## 检查清单

### 节点网络就绪检查

- [ ] 网卡 UP 且速率正确 (`ethtool eth0`)
- [ ] MTU 配置一致 (`ip link show`)
- [ ] 路由表正确 (`ip route show`)
- [ ] DNS 可达 (`dig @<dns-ip> example.com`)
- [ ] conntrack 未满 (`conntrack -C` vs `nf_conntrack_max`)
- [ ] 无丢包 (`netstat -s | grep -i drop`)
- [ ] 无 TCP 重传异常 (`netstat -s | grep -i retrans`)
- [ ] iptables 规则数合理 (`iptables -t nat -L | wc -l`)

### 生产网络优化检查

- [ ] TCP 参数已调优（somaxconn, tw_reuse, keepalive）
- [ ] conntrack 表大小充足（max > 当前 * 2）
- [ ] DNS 缓存已启用（NodeLocal DNSCache）
- [ ] 大文件传输使用合适 MTU（Jumbo Frame）
- [ ] BBR 拥塞控制已启用
- [ ] 网络监控已部署（CNI metrics, conntrack exporter）

## 缩略语表

| 缩写 | 全称 | 说明 |
|------|------|------|
| TCP | Transmission Control Protocol | 传输控制协议 |
| UDP | User Datagram Protocol | 用户数据报协议 |
| DNS | Domain Name System | 域名系统 |
| NAT | Network Address Translation | 网络地址转换 |
| SNAT | Source NAT | 源地址转换 |
| DNAT | Destination NAT | 目的地址转换 |
| MTU | Maximum Transmission Unit | 最大传输单元 |
| TTL | Time To Live | 生存时间 |
| RTT | Round Trip Time | 往返时间 |
| MSS | Maximum Segment Size | 最大段大小 |
| CNI | Container Network Interface | 容器网络接口 |
| BGP | Border Gateway Protocol | 边界网关协议 |
| ARP | Address Resolution Protocol | 地址解析协议 |
| ICMP | Internet Control Message Protocol | 互联网控制消息协议 |
| TLS | Transport Layer Security | 传输层安全 |
| QUIC | Quick UDP Internet Connections | 快速 UDP 连接 |

## 参考链接

- [[系统基础/速查卡/networking.md|网络速查卡]]
- [[系统基础/知识字典/networking/index.md|网络知识字典]]
- [[系统基础/Linux/04-linux-networking-configuration.md|Linux 网络配置]]
- [[系统基础/硬件/08-network-hardware-technology.md|网络硬件技术]]
