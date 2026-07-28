---
title: 网络抓包与诊断工具
description: tcpdump、Wireshark、eBPF 抓包、ss/netstat、网络故障排查方法论、K8s 网络诊断
summary: 网络诊断工具完整知识，覆盖 tcpdump 实战、Wireshark 分析、eBPF 工具、K8s 网络排查流程
category: knowledge
tags:
- networking
- tcpdump
- wireshark
- ebpf
- troubleshooting
- diagnostics
domain: 系统基础
difficulty: advanced
audience:
- SRE
- 平台工程师
- 网络工程师
tier: supporting
---

# 网络抓包与诊断工具

> 网络抓包是排查复杂网络问题的终极手段。掌握 tcpdump、Wireshark、eBPF 等工具，能在分钟级定位 DNS 超时、TLS 握手失败、TCP 重传等疑难问题。

## 诊断工具全景

### 按层次分类

| 层次 | 工具 | 用途 |
|------|------|------|
| L2 链路 | ping, arping, ethtool | 连通性/链路状态 |
| L3 网络 | traceroute, mtr, ip | 路由/路径 |
| L4 传输 | ss, netstat, nc | 连接/端口 |
| L7 应用 | curl, dig, wget | HTTP/DNS |
| 抓包 | tcpdump, Wireshark | 完整包分析 |
| eBPF | bpftrace, cilium, tcpretrans | 内核级追踪 |
| K8s | kubectl, k9s, kubeshark | 集群网络 |

### 工具选择决策树

```
网络问题
    │
    ├── 完全不通 → ping/traceroute (L2/L3)
    │
    ├── 端口不通 → ss -tlnp / nc -zv (L4)
    │
    ├── 连接建立但无数据 → tcpdump 抓包 (L4/L7)
    │
    ├── 间歇性问题 → 持续抓包 + 时间戳关联
    │
    ├── 性能问题 → mtr / bpftrace / tcpretrans
    │
    └── K8s 特有 → kubectl exec + nsenter + kubeshark
```

## tcpdump 实战

### 基本语法

```bash
tcpdump [选项] [过滤表达式]

# 选项
-i <interface>    # 指定网卡 (any=所有)
-n                # 不解析主机名
-nn               # 不解析主机名和端口名
-v/-vv/-vvv       # 详细程度
-w <file>         # 写入文件
-r <file>         # 读取文件
-c <count>        # 抓包数量
-s <size>         # 抓包长度 (0=全部)
-A                # ASCII 显示
-X                # 十六进制+ASCII
-tt               # 精确时间戳
```

### 常用过滤表达式

```bash
# 🟢 按主机
tcpdump -i any -nn host 10.244.1.5
tcpdump -i any -nn src host 10.244.1.5
tcpdump -i any -nn dst host 10.244.1.5

# 🟢 按端口
tcpdump -i any -nn port 80
tcpdump -i any -nn portrange 8000-9000
tcpdump -i any -nn tcp port 443

# 🟢 按协议
tcpdump -i any -nn tcp
tcpdump -i any -nn udp port 53
tcpdump -i any -nn icmp

# 🟢 组合条件
tcpdump -i any -nn 'host 10.244.1.5 and port 8080'
tcpdump -i any -nn 'src 10.244.1.5 or src 10.244.2.3'
tcpdump -i any -nn 'tcp and (port 80 or port 443)'

# 🟢 TCP 标志
tcpdump -i any -nn 'tcp[tcpflags] & tcp-syn != 0'     # SYN 包
tcpdump -i any -nn 'tcp[tcpflags] & tcp-fin != 0'     # FIN 包
tcpdump -i any -nn 'tcp[tcpflags] & tcp-rst != 0'     # RST 包
tcpdump -i any -nn 'tcp[tcpflags] == tcp-syn'          # 纯 SYN

# 🟢 DNS 查询
tcpdump -i any -nn 'udp port 53 and (dst host 10.96.0.10)'

# 🟢 抓包保存（后续 Wireshark 分析）
tcpdump -i any -nn -w /tmp/capture.pcap -c 10000 host 10.244.1.5
```

### K8s 场景抓包

```bash
# 🟢 Pod 内抓包（需要 tcpdump 工具）
kubectl exec -it <pod> -- tcpdump -i eth0 -nn port 8080

# 🟢 节点上抓 Pod 流量（通过 veth）
# 先找到 Pod 的 veth
kubectl get pod <pod> -o jsonpath='{.status.containerID}'
crictl inspect <container-id> | grep -i ifname
# 或
ip link | grep veth

# 在节点上抓
tcpdump -i veth12345 -nn -w /tmp/pod-capture.pcap

# 🟢 抓 Service 流量
tcpdump -i any -nn 'host <cluster-ip> and port <service-port>'

# 🟢 抓 DNS 查询
tcpdump -i any -nn 'udp port 53' -A

# 🟢 抓特定 Pod 所有流量
POD_IP=$(kubectl get pod <pod> -o jsonpath='{.status.podIP}')
tcpdump -i any -nn host $POD_IP -w /tmp/pod-all.pcap
```

### 分析抓包结果

```bash
# 🟢 查看 TCP 连接建立
tcpdump -r capture.pcap 'tcp[tcpflags] & tcp-syn != 0'

# 🟢 查看重传
tcpdump -r capture.pcap 'tcp' | grep -i retrans

# 🟢 查看 RST（连接拒绝）
tcpdump -r capture.pcap 'tcp[tcpflags] & tcp-rst != 0'

# 🟢 统计包数量
tcpdump -r capture.pcap | wc -l

# 🟢 按时间范围过滤
tcpdump -r capture.pcap 'tcp' -tt | awk '$1 > 1700000000 && $1 < 1700000060'
```

## Wireshark 分析

### 常用显示过滤器

```
# 按 IP
ip.addr == 10.244.1.5
ip.src == 10.244.1.5 && ip.dst == 10.244.2.3

# 按端口
tcp.port == 8080
tcp.dstport == 443

# TCP 问题
tcp.analysis.retransmission      # 重传
tcp.analysis.duplicate_ack       # 重复 ACK
tcp.analysis.zero_window         # 零窗口
tcp.flags.reset == 1             # RST
tcp.flags.syn == 1 && tcp.flags.ack == 0  # 纯 SYN

# DNS
dns && dns.flags.rcode != 0      # DNS 错误
dns.qry.name contains "cluster.local"

# HTTP
http.response.code >= 500        # 5xx 错误
http.request.method == "POST"

# TLS
tls.handshake.type == 1          # ClientHello
tls.alert_message                # TLS 告警
```

### Wireshark 分析流程

```
1. 打开 pcap 文件
    │
2. 应用过滤器缩小范围
    │
3. 查看 TCP Stream (Follow → TCP Stream)
    │
4. 检查 Statistics:
    ├── Conversations (连接统计)
    ├── Protocol Hierarchy (协议分布)
    ├── TCP Stream Graphs (时序图)
    └── I/O Graphs (流量图)
    │
5. 定位异常包 → 分析根因
```

## eBPF 网络工具

### bpftrace 网络脚本

```bash
# 🟢 追踪 TCP 连接建立
bpftrace -e 'kprobe:tcp_connect { printf("%s:%d -> ", comm, pid); }
kprobe:tcp_set_state /arg1 == 1/ { printf("ESTABLISHED\n"); }'

# 🟢 追踪 TCP 重传
bpftrace -e 'kprobe:tcp_retransmit_skb { printf("%s retransmit\n", comm); }'

# 🟢 追踪 DNS 查询
bpftrace -e 'kprobe:udp_sendmsg /arg2 == 53/ { printf("DNS query from %s\n", comm); }'

# 🟢 追踪连接延迟
bpftrace -e '
kprobe:tcp_connect { @start[tid] = nsecs; }
kretprobe:tcp_connect /@start[tid]/ {
  @connect_us = hist((nsecs - @start[tid]) / 1000);
  delete(@start[tid]);
}'
```

### BCC 网络工具

```bash
# 🟢 tcpretrans - 显示 TCP 重传
tcpretrans -c

# 🟢 tcpconnect - 追踪新建连接
tcpconnect -c -P 80,443

# 🟢 tcpaccept - 追踪接受的连接
tcpaccept -P 8080

# 🟢 tcplife - 追踪连接生命周期
tcplife -T  # 显示时间

# 🟢 tcptracer - 追踪连接事件
tcptracer

# 🟢 dnsdist - DNS 查询追踪
dnstop eth0
```

### Cilium 网络诊断

```bash
# 🟢 查看 Pod 网络策略
cilium policy get

# 🟢 查看连接跟踪
cilium bpf ct list global

# 🟢 监控网络流量
cilium monitor --type trace

# 🟢 连通性测试
cilium connectivity test

# 🟢 查看 eBPF 程序
cilium bpf policy get <endpoint-id>
```

## K8s 网络诊断

### Pod 网络诊断

```bash
# 🟢 进入 Pod 网络命名空间
kubectl exec -it <pod> -- sh

# 🟢 Pod 内诊断
cat /etc/resolv.conf
ip addr show
ip route show
ss -tlnp
nc -zv <target> <port>
curl -vvv http://<service>:<port>/

# 🟢 使用 nsenter 进入 Pod 网络命名空间（节点上）
PID=$(crictl inspect <container-id> | jq .info.pid)
nsenter -t $PID -n ip addr
nsenter -t $PID -n ss -tlnp
nsenter -t $PID -n tcpdump -i eth0 -nn

# 🟢 临时调试 Pod
kubectl run debug --image=nicolaka/netshoot --rm -it -- sh
# netshoot 包含: tcpdump, curl, dig, nmap, ss, iperf3 等
```

### 网络排查完整流程

```
Step 1: 确认症状
    ├── 完全不通 (timeout)
    ├── 连接拒绝 (connection refused)
    ├── 间歇性失败
    └── 延迟高
        │
Step 2: 分层排查
    L2: ping <pod-ip> / arping
    L3: traceroute / ip route
    L4: nc -zv / ss -tlnp
    L7: curl -vvv / dig
        │
Step 3: 定位范围
    ├── Pod 内问题 → 检查应用/配置
    ├── 节点问题 → 检查 iptables/路由
    ├── 集群问题 → 检查 CNI/CoreDNS
    └── 外部问题 → 检查 LB/防火墙
        │
Step 4: 抓包确认
    tcpdump -i any -nn -w /tmp/debug.pcap <filter>
        │
Step 5: 修复验证
    修复 → 验证 → 记录 → 监控
```

### 常见网络问题速查

| 症状 | 工具 | 命令 |
|------|------|------|
| Pod 无法解析 DNS | dig/nslookup | `kubectl exec -- dig @10.96.0.10 <svc>` |
| Service 无响应 | kubectl | `kubectl get endpoints <svc>` |
| 跨节点不通 | ping/traceroute | `ping <remote-pod-ip>` |
| 连接超时 | nc/tcpdump | `nc -zv <ip> <port>` |
| 高延迟 | mtr/tcpdump | `mtr -n <target>` |
| 丢包 | netstat/ss | `netstat -s \| grep -i drop` |
| TLS 失败 | openssl | `openssl s_client -connect <ip>:443` |
| 带宽不足 | iperf3 | `iperf3 -c <target> -t 30` |

## 生产案例

### 案例1：DNS 超时导致服务启动失败

**症状：** Pod 启动时 CrashLoopBackOff，日志显示 DNS 解析超时

**排查：**
```bash
tcpdump -i any -nn 'udp port 53' -A
# 发现 DNS 查询无响应
kubectl get pods -n kube-system -l k8s-app=kube-dns
# CoreDNS Pod OOMKilled
```

**解决：** 增加 CoreDNS 资源限制，部署 NodeLocal DNSCache

### 案例2：TCP 重传导致延迟飙升

**症状：** 服务 P99 延迟从 50ms 飙升到 2s

**排查：**
```bash
tcpretrans -c
# 大量重传到特定节点
mtr -n <target-node>
# 发现某跳丢包 30%
ethtool -S eth0 | grep -i error
# 网卡 CRC 错误
```

**解决：** 更换故障网线/网卡

### 案例3：NetworkPolicy 阻断合法流量

**症状：** 部署 NetworkPolicy 后部分服务不通

**排查：**
```bash
kubectl get networkpolicy -n <ns> -o yaml
# 检查 ingress/egress 规则
cilium monitor --type drop
# 查看被丢弃的包
```

**解决：** 补充缺失的 egress 规则（DNS、依赖服务）

## 工具安装

```bash
# 节点工具
yum install -y tcpdump wireshark-cli mtr traceroute
apt install -y tcpdump tshark mtr-tiny traceroute

# K8s 调试 Pod
kubectl run netshoot --image=nicolaka/netshoot --rm -it -- bash

# eBPF 工具
yum install -y bcc-tools bpftrace
apt install -y bpfcc-tools bpftrace

# Cilium CLI
curl -L --remote-name-all https://github.com/cilium/cilium-cli/releases/latest/download/cilium-linux-amd64.tar.gz
tar xzvf cilium-linux-amd64.tar.gz
mv cilium /usr/local/bin/
```

## 检查清单

- [ ] 掌握 tcpdump 过滤表达式
- [ ] 能在 K8s Pod/节点上抓包
- [ ] 掌握 Wireshark 显示过滤器
- [ ] 能分析 TCP 重传/RST/零窗口
- [ ] 了解 eBPF 网络工具 (bpftrace/BCC)
- [ ] 掌握 K8s 网络分层排查流程
- [ ] 能使用 nsenter 进入 Pod 网络命名空间
- [ ] 掌握常见网络问题速查表

## 参考链接

- [[17-系统基础/03-网络基础/index.md|网络基础总索引]]
- [[17-系统基础/03-网络基础/01-tcp-ip-protocol-stack.md|TCP/IP 协议栈]]
- [[17-系统基础/05-速查卡/networking.md|网络速查卡]]
- [[17-系统基础/05-速查卡/perf-bpftrace-cheat-sheet.md|bpftrace 速查卡]]
