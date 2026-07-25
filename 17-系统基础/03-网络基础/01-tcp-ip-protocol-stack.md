---
title: TCP/IP 协议栈深度解析
description: TCP/IP 四层模型、TCP 状态机、拥塞控制、UDP 特性、ICMP 协议、Socket 编程模型、K8s 网络中的 TCP 行为
summary: TCP/IP 协议栈完整知识，覆盖四层模型、TCP 状态机、拥塞控制算法、Socket 模型、K8s 场景实践
category: knowledge
tags:
- networking
- tcp
- udp
- protocol
- kernel
domain: 系统基础
difficulty: advanced
audience:
- SRE
- 平台工程师
- 网络工程师
---

# TCP/IP 协议栈深度解析

> TCP/IP 是互联网和 Kubernetes 网络的基石协议族。深入理解其工作原理是排查网络故障、优化服务性能的前提。

## 四层模型详解

### 各层职责与协议

| 层次 | 名称 | 核心协议 | 数据单元 | K8s 对应 |
|------|------|----------|----------|----------|
| 4 | 应用层 | HTTP, DNS, gRPC, etcd | 消息 | Service/Ingress |
| 3 | 传输层 | TCP, UDP, SCTP | 段(Segment) | kube-proxy |
| 2 | 网络层 | IP, ICMP, ARP | 包(Packet) | CNI/路由 |
| 1 | 链路层 | Ethernet, WiFi | 帧(Frame) | veth/bridge |

### 数据封装与解封装

```
发送方:
  应用数据 → [TCP头 + 数据] → [IP头 + TCP段] → [Eth头 + IP包 + FCS]
              段(Segment)      包(Packet)        帧(Frame)

接收方:
  帧 → 去Eth头 → 包 → 去IP头 → 段 → 去TCP头 → 应用数据
```

### 各层头部大小

| 协议 | 头部大小 | 关键字段 |
|------|----------|----------|
| Ethernet | 14 字节 | src/dst MAC, EtherType |
| IPv4 | 20-60 字节 | src/dst IP, TTL, Protocol |
| IPv6 | 40 字节 | src/dst IP, Hop Limit |
| TCP | 20-60 字节 | src/dst Port, Seq, Ack, Flags |
| UDP | 8 字节 | src/dst Port, Length |
| ICMP | 8 字节 | Type, Code, Checksum |

## TCP 协议深度

### TCP 头部结构

```
 0                   1                   2                   3
 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|          Source Port          |       Destination Port        |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                        Sequence Number                        |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                    Acknowledgment Number                      |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|  Data |           |U|A|P|R|S|F|                               |
| Offset| Reserved  |R|C|S|S|Y|I|            Window             |
|       |           |G|K|H|T|N|N|                               |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|           Checksum            |         Urgent Pointer        |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                    Options (0-40 bytes)                       |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
```

### TCP 标志位详解

| 标志 | 名称 | 含义 | 使用场景 |
|------|------|------|----------|
| SYN | Synchronize | 同步序列号 | 三次握手第1、2步 |
| ACK | Acknowledge | 确认收到 | 几乎所有包 |
| FIN | Finish | 发送完毕 | 四次挥手 |
| RST | Reset | 重置连接 | 异常断开/端口未监听 |
| PSH | Push | 立即交付应用 | 小数据包 |
| URG | Urgent | 紧急数据 | 几乎不用 |

### TCP 三次握手详解

```
步骤1: Client → Server
  SYN=1, seq=x (随机ISN)
  状态: Client=SYN_SENT, Server=LISTEN→SYN_RCVD

步骤2: Server → Client
  SYN=1, ACK=1, seq=y, ack=x+1
  状态: Server=SYN_RCVD

步骤3: Client → Server
  ACK=1, seq=x+1, ack=y+1
  状态: Client=ESTABLISHED, Server=ESTABLISHED
```

**为什么需要三次？**
- 防止历史重复连接（旧 SYN 到达）
- 双方确认收发能力正常
- 同步初始序列号（ISN）

**SYN Flood 攻击与防御：**
```bash
# 启用 SYN Cookies（不依赖半连接队列）
net.ipv4.tcp_syncookies = 1

# 增大半连接队列
net.ipv4.tcp_max_syn_backlog = 65535

# 减少 SYN-ACK 重试次数
net.ipv4.tcp_synack_retries = 2
```

### TCP 四次挥手详解

```
步骤1: Client → Server: FIN, seq=u
  Client: ESTABLISHED → FIN_WAIT_1

步骤2: Server → Client: ACK, ack=u+1
  Client: FIN_WAIT_1 → FIN_WAIT_2
  Server: ESTABLISHED → CLOSE_WAIT

步骤3: Server → Client: FIN, seq=w
  Server: CLOSE_WAIT → LAST_ACK

步骤4: Client → Server: ACK, ack=w+1
  Client: FIN_WAIT_2 → TIME_WAIT (等待 2MSL)
  Server: LAST_ACK → CLOSED
```

**为什么 TIME_WAIT 要等 2MSL？**
- 确保最后的 ACK 到达对方（丢失时对方重发 FIN）
- 让旧连接的迟到包在网络中消亡
- MSL 通常为 30s 或 60s，所以 TIME_WAIT 持续 60-120s

**TIME_WAIT 过多的处理：**
```bash
# 查看 TIME_WAIT 数量
ss -s | grep TIME-WAIT
netstat -n | grep TIME_WAIT | wc -l

# 允许复用（安全，仅对客户端有效）
net.ipv4.tcp_tw_reuse = 1

# 缩短 FIN_WAIT_2 超时
net.ipv4.tcp_fin_timeout = 15

# 增大本地端口范围
net.ipv4.ip_local_port_range = 1024 65535
```

### TCP 可靠传输机制

| 机制 | 原理 | 参数 |
|------|------|------|
| 序列号 | 每字节编号，保证有序 | ISN 随机生成 |
| 确认应答 | 接收方回复 ACK | 累积确认 |
| 超时重传 | RTO 内未收到 ACK 则重传 | 初始 200ms，指数退避 |
| 快速重传 | 3 个重复 ACK 立即重传 | 不等 RTO |
| 滑动窗口 | 流控，控制发送速率 | 接收方通告窗口 |
| 拥塞控制 | 避免网络过载 | cwnd/ssthresh |

### TCP 拥塞控制算法

```
        cwnd
         │
         │         ╱──── 拥塞避免（线性增）
         │        ╱
         │       ╱
         │      ╱ ← ssthresh
         │     ╱
         │    ╱ 慢启动（指数增）
         │   ╱
         │  ╱
         │ ╱
         │╱
         └─────────────────────────────── time
```

| 算法 | 特点 | 适用场景 |
|------|------|----------|
| Reno | 经典 AIMD | 通用 |
| CUBIC | Linux 默认，高带宽友好 | 数据中心 |
| BBR | 基于带宽和 RTT 建模 | 高延迟/有丢包 |
| DCTCP | 数据中心 TCP，ECN | 数据中心内部 |

```bash
# 查看当前拥塞控制算法
sysctl net.ipv4.tcp_congestion_control

# 切换到 BBR
modprobe tcp_bbr
sysctl -w net.ipv4.tcp_congestion_control=bbr
sysctl -w net.core.default_qdisc=fq

# 验证
lsmod | grep bbr
```

## UDP 协议

### UDP 特点与使用场景

| 特点 | 说明 |
|------|------|
| 无连接 | 无需握手，直接发送 |
| 不可靠 | 不保证到达、不保证顺序 |
| 低开销 | 头部仅 8 字节 |
| 无流控 | 发送速率不受限制 |
| 支持广播/组播 | 一对多通信 |

### K8s 中 UDP 的使用

| 场景 | 说明 |
|------|------|
| DNS (CoreDNS) | 53/UDP，查询通常 < 512B |
| Service (UDP 类型) | 游戏/视频/IoT |
| VXLAN (CNI) | Flannel/Calico 封装 |
| 日志 (syslog) | 514/UDP |
| NTP | 123/UDP 时间同步 |

### UDP 注意事项

```bash
# UDP 接收缓冲区（DNS 高并发时重要）
net.core.rmem_max = 16777216
net.core.rmem_default = 262144

# 查看 UDP 丢包
netstat -su | grep "packet receive errors"
ss -ulnp  # 查看 UDP 监听

# CoreDNS UDP 丢包排查
kubectl logs -n kube-system -l k8s-app=kube-dns | grep -i "timeout\|drop"
```

## ICMP 协议

### 常用 ICMP 类型

| Type | Code | 含义 | 场景 |
|------|------|------|------|
| 0 | 0 | Echo Reply | ping 响应 |
| 3 | 0 | 网络不可达 | 路由错误 |
| 3 | 1 | 主机不可达 | 目标下线 |
| 3 | 3 | 端口不可达 | UDP 无监听 |
| 3 | 4 | 需要分片(DF) | MTU 问题 |
| 5 | 0 | 重定向 | 路由优化 |
| 8 | 0 | Echo Request | ping 请求 |
| 11 | 0 | TTL 超时 | traceroute |

### ICMP 在 K8s 中的重要性

```bash
# 测试 Pod 连通性
kubectl exec -it <pod> -- ping <target-pod-ip>

# 检查 MTU（DF 位）
ping -M do -s 1472 <target>  # 1472 + 28(IP+ICMP) = 1500

# MTU 问题排查（VXLAN 封装减少 MTU）
ip link show | grep mtu
# VXLAN: 外部 1500 → 内部 1450（50字节封装开销）
```

## Socket 编程模型

### TCP Server 流程

```
socket() → bind() → listen() → accept() → read()/write() → close()
                                    ↑
                              阻塞等待连接
```

### 关键 Socket 参数

| 参数 | 含义 | K8s 影响 |
|------|------|----------|
| SO_REUSEADDR | 端口复用 | Pod 快速重启 |
| SO_KEEPALIVE | TCP 保活 | 检测死连接 |
| TCP_NODELAY | 禁用 Nagle | 低延迟 |
| SO_LINGER | 关闭行为 | 优雅终止 |
| SO_RCVBUF | 接收缓冲 | 高吞吐 |
| SO_SNDBUF | 发送缓冲 | 高吞吐 |

### 连接队列（Backlog）

```
                    ┌─────────────────────┐
  SYN 到达 ──────→ │   半连接队列 (SYN Queue)  │ ← tcp_max_syn_backlog
                    └──────────┬──────────┘
                               │ 三次握手完成
                               ▼
                    ┌─────────────────────┐
  accept() ←────── │   全连接队列 (Accept Queue) │ ← somaxconn / backlog
                    └─────────────────────┘
```

```bash
# 查看全连接队列溢出
netstat -s | grep "overflowed"
# 或
nstat -az | grep TcpExtListenOverflows

# 查看特定端口的队列状态
ss -lnt | grep :80
# Recv-Q: 当前全连接队列长度
# Send-Q: 全连接队列上限 (min(backlog, somaxconn))
```

## K8s 中的 TCP 行为

### Service 连接建立流程

```
Pod A (Client) → Service ClusterIP:Port
    │
    ▼ (iptables DNAT)
Pod B (Server): TargetPort
    │
    ▼
TCP 三次握手 (Pod A ↔ Pod B)
    │
    ▼
数据传输
    │
    ▼
连接关闭 (四次挥手)
```

### 优雅终止与 TCP

```yaml
# Pod 优雅终止配置
spec:
  terminationGracePeriodSeconds: 60
  containers:
  - name: app
    lifecycle:
      preStop:
        exec:
          command: ["/bin/sh", "-c", "sleep 10"]  # 等待 endpoints 更新
```

**优雅终止流程：**
1. Pod 标记为 Terminating
2. 从 Service Endpoints 移除（异步）
3. 执行 preStop hook
4. 发送 SIGTERM
5. 等待 terminationGracePeriodSeconds
6. 发送 SIGKILL

**常见问题：** preStop 未等待足够时间，导致已移除的 Endpoints 仍有 in-flight 请求。

### 生产案例

#### 案例1：TIME_WAIT 导致端口耗尽

**症状：** 高并发服务报 `cannot assign requested address`

**根因：** 短连接客户端产生大量 TIME_WAIT，本地端口耗尽

**解决：**
```bash
# 临时
sysctl -w net.ipv4.tcp_tw_reuse=1
sysctl -w net.ipv4.ip_local_port_range="1024 65535"

# 根本：使用连接池/长连接
```

#### 案例2：全连接队列溢出

**症状：** 间歇性连接超时，`ss -lnt` 显示 Recv-Q 接近 Send-Q

**根因：** 突发流量超过 accept 速率，全连接队列满

**解决：**
```bash
sysctl -w net.core.somaxconn=65535
# 应用侧增大 listen backlog
```

#### 案例3：TCP 重传率高

**症状：** 服务延迟 P99 异常高

**排查：**
```bash
netstat -s | grep retrans
# 计算重传率 = retrans / total segments

# 检查是否 MTU 不匹配
ping -M do -s 1400 <target>
```

## 监控指标

### 关键 TCP 指标

| 指标 | 含义 | 告警阈值 |
|------|------|----------|
| TcpRetransSegs | 重传段数 | 重传率 > 1% |
| TcpExtListenOverflows | 全连接溢出 | > 0 |
| TcpExtListenDrops | 监听丢弃 | > 0 |
| TcpAttemptFails | 连接失败 | 突增 |
| TcpCurrEstab | 当前连接数 | 接近上限 |
| TcpExtTCPTimeouts | 超时重传 | 突增 |

### Prometheus 采集

```yaml
# node_exporter 已包含 TCP 指标
- job_name: 'node'
  static_configs:
    - targets: ['node:9100']

# 告警规则
groups:
- name: tcp-alerts
  rules:
  - alert: HighTCPRetransmitRate
    expr: rate(node_netstat_Tcp_RetransSegs[5m]) / rate(node_netstat_Tcp_OutSegs[5m]) > 0.01
    for: 5m
    labels:
      severity: warning
    annotations:
      summary: "TCP 重传率超过 1%"
```

## 版本兼容矩阵

| 组件 | 版本 | TCP 相关变化 |
|------|------|-------------|
| Linux Kernel | 4.9+ | BBR 可用 |
| Linux Kernel | 5.6+ | MPTCP 支持 |
| Linux Kernel | 5.19+ | BIG TCP (数据中心) |
| Kubernetes | 1.25+ | 支持 Minimize IPTables |
| Kubernetes | 1.31+ | nftables kube-proxy GA |
| containerd | 1.7+ | 网络命名空间优化 |
| Cilium | 1.14+ | eBPF 替代 iptables |

## 常见问题 FAQ

**Q1: 为什么 K8s Pod 间通信不需要 NAT？**
A: CNI 通过 veth pair + 路由/隧道实现 Pod 直接通信，每个 Pod 有独立 IP，包在 Pod 间直接路由，无需地址转换。只有 Pod 访问外部或 Service ClusterIP 时才经过 NAT。

**Q2: tcp_tw_reuse 和 tcp_tw_recycle 的区别？**
A: `tcp_tw_reuse` 允许客户端复用 TIME_WAIT 连接（安全），`tcp_tw_recycle` 快速回收 TIME_WAIT（NAT 环境不安全，Linux 4.12 已移除）。生产环境只用 `tcp_tw_reuse`。

**Q3: 如何判断是 TCP 层还是应用层问题？**
A: 使用 `tcpdump` 抓包：如果三次握手成功但应用无响应，是应用层问题；如果 SYN 无回复，是网络/防火墙问题；如果频繁重传，是网络质量问题。

**Q4: K8s 中 conntrack 表满会怎样？**
A: 新连接无法建立，`dmesg` 显示 `nf_conntrack: table full, dropping packet`。解决：增大 `nf_conntrack_max`，减少超时时间，或使用 IPVS/eBPF 模式减少 conntrack 依赖。

**Q5: BBR 和 CUBIC 如何选择？**
A: 数据中心内部（低延迟、低丢包）用 CUBIC 即可；跨地域/公网（高延迟、有丢包）用 BBR 可显著提升吞吐。注意 BBR 需要 `fq` 调度器。

## 检查清单

- [ ] 理解 TCP 三次握手和四次挥手
- [ ] 掌握 TCP 状态机 11 种状态
- [ ] 能排查 TIME_WAIT/CLOSE_WAIT 问题
- [ ] 理解拥塞控制（慢启动/拥塞避免/快重传/快恢复）
- [ ] 掌握 conntrack 对 TCP 的影响
- [ ] 能使用 ss/netstat 诊断连接问题
- [ ] 理解 K8s Service 的 TCP 连接流程
- [ ] 掌握优雅终止对 TCP 连接的影响
- [ ] 了解 BBR/CUBIC 选择策略
- [ ] 掌握 Socket 参数调优

## 参考链接

- [[17-系统基础/03-网络基础/index.md|网络基础总索引]]
- [[17-系统基础/03-网络基础/03-iptables-nftables-packet-filtering.md|iptables/nftables]]
- [[17-系统基础/01-Linux/04-linux-networking-configuration.md|Linux 网络配置]]
- [[17-系统基础/05-速查卡/networking.md|网络速查卡]]
