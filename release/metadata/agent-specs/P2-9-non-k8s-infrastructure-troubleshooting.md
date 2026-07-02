---
title: 非 Kubernetes 基础设施问题排查
description: '# 非 Kubernetes 基础设施问题排查'
summary: '# 非 Kubernetes 基础设施问题排查'
category: general
tags:
- k8s
- coredns
- gateway
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 非 Kubernetes 基础设施问题排查 是什么
- 如何 非 Kubernetes 基础设施问题排查
- 非 Kubernetes 基础设施问题排查 问题排查
- 非 Kubernetes 基础设施问题排查 排障步骤
trigger_keywords:
- Kubernetes
- 基础设施问题排查
prerequisites:
- kubectl-basics
---



# 非 Kubernetes 基础设施问题排查

> **版本**: v1.0
> **创建日期**: 2026-05-18
> **用途**: DNS (CoreDNS之外)、Load Balancer、VPN/隧道等非 [[entities/kubernetes.md|k8s]] 基础设施的问题排查
> **关联**: domain-14-linux, domain-03-networking-traffic

---

## 1. DNS 服务问题排查

### 1.1 BIND/named 问题排查

| 症状 | 诊断命令 | 根因 | 修复 |
|------|---------|------|------|
| DNS 查询无响应 | `dig @<dns-server> <domain>` | named 未运行 | `systemctl restart named` |
| 递归查询失败 | `dig @<dns-server> <domain> +trace` | 根服务器配置错误 | 检查 forwarders 配置 |
| 区域传输失败 | `dig @<dns-server> axfr <zone>` | allow-transfer 未配置 | 检查 named.conf |
| 解析缓慢 | `dig @<dns-server> <domain>` | 缓存问题/网络延迟 | 调整 max-cache-size |

```bash
# BIND 日志检查
journalctl -u named --since "10 minutes ago"
cat /var/log/named/named.log | tail -100

# 测试配置
named-checkconf /etc/named.conf
named-checkzone <domain> /var/named/zones/<domain>.db

# 常用诊断
dig @localhost www.example.com A +short
dig @localhost . NS +short  # 检查根服务器

# 重载配置
rndc reload
rndc flush  # 清除缓存
```

### 1.2 Dnsmasq 问题排查

| 症状 | 诊断命令 | 根因 | 修复 |
|------|---------|------|------|
| DNS 不解析 | `dig @localhost <domain>` | dnsmasq 未运行 | `systemctl restart dnsmasq` |
| 上游 DNS 不工作 | `cat /etc/resolv.conf` | upstream DNS 配置错误 | 检查 /etc/dnsmasq.conf |
| DHCP 冲突 | `journalctl -u dnsmasq` | IP 分配冲突 | 修复 DHCP 范围 |

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `systemctl stop/restart`：停止/重启系统服务，影响节点上所有容器

```bash
# Dnsmasq 日志
journalctl -u dnsmasq --since "10m" | grep -i error

# 测试配置
dnsmasq --test

# 重启
systemctl restart dnsmasq

# 查看 leases
cat /var/lib/dhcpd/dhcpd.leases
```

### 1.3 企业 DNS 架构问题

```bash
# 多级 DNS 解析检查
dig @<corp-dns> <internal-domain>
# → 失败则检查 corp DNS 配置

# 上游 DNS 无法解析内部域名
# → 配置 split-horizon DNS
# → 内部域名配置在内部 DNS
# → 外部域名转发到公共 DNS

# DNS 传播延迟
# → 区域修改后等待 2-24h 传播
# → 检查 SOA 序列号是否递增
dig @<dns-server> <domain> SOA +short
```

---

## 2. Load Balancer 问题排查

### 2.1 F5 BIG-IP 问题排查

| 症状 | 诊断命令 | 根因 | 修复 |
|------|---------|------|------|
| Virtual Server 不可用 | `tmsh show ltm virtual <name>` | Pool 成员全下线 | 检查 Pool 成员健康 |
| 连接超时 | `tmsh show ltm pool <pool> stats` | 健康检查失败 | 修正健康检查配置 |
| 带宽瓶颈 | `tmsh show sys hardware` | 吞吐量超限 | 扩容或分流 |

```bash
# F5 常用命令
tmsh show ltm virtual         # 查看所有 VS
tmsh show ltm pool             # 查看 Pool
tmsh show sys hardware         # 查看硬件状态
tmsh show sys activity         # 查看活动统计

# 健康检查状态
tmsh show ltm node all-fields | grep -A5 "availability"

# 重启 VS
tmsh modify ltm virtual <name> disabled
tmsh modify ltm virtual <name> enabled

# 常见问题
# SSL 证书过期 → tmsh replace / sys file ssl-cert <cert>
# 连接限制 → tmsh modify ltm virtual <name> connection-limit <num>
```

### 2.2 云负载均衡器问题

| 云厂商 | 诊断命令 | 说明 |
|--------|---------|------|
| AWS ALB/NLB | `aws elbv2 describe-load-balancers --names <name>` | 检查 LB 状态 |
| AWS ALB | `aws elbv2 describe-target-groups --target-group-arn <arn>` | 检查 Target Group |
| GCP LB | `gcloud compute backend-services describe <name> --global` | 检查后端服务 |
| Azure LB | `az network lb show --resource-group <rg> --name <name>` | 检查 LB 配置 |
| 阿里云 SLB | `aliyun slb DescribeLoadBalancers --RegionId <id>` | 检查 SLB 状态 |

```bash
# AWS ALB 健康检查
aws elbv2 describe-target-health --target-group-arn <arn>

# 常见问题
# Target unhealthy → 检查安全组是否允许健康检查 IP
# 跨 AZ 不平衡 → 检查并重新分配 targets
# HTTPS 证书问题 → 检查 cert expiry

# GCP 负载均衡
gcloud compute backend-services get-health <name> --global
gcloud compute forwarding-rules list
```

### 2.3 Nginx/HAProxy 负载均衡

| 症状 | 诊断命令 | 根因 | 修复 |
|------|---------|------|------|
| Nginx upstream 全部失败 | `nginx -t && nginx -s reload` | upstream 配置错误 | 检查 upstream 定义 |
| 502 Bad Gateway | `curl -v http://localhost/<path>` | 后端服务未运行 | 检查后端服务 |
| 连接数满 | `netstat -an | grep :80 | wc -l` | worker_connections 不足 | 增加 worker_connections |

```bash
# Nginx 健康检查
nginx -t
systemctl reload nginx

# 查看 upstream 状态
curl http://localhost/upstream_status

# HAProxy 健康检查
haproxy -c -f /etc/haproxy/haproxy.cfg
systemctl reload haproxy

# 查看 HAProxy 统计
curl -s http://localhost:9000/stats
```

---

## 3. VPN 与隧道问题排查

### 3.1 WireGuard VPN 问题

| 症状 | 诊断命令 | 根因 | 修复 |
|------|---------|------|------|
| 隧道无法建立 | `wg show` | 公钥/私钥不匹配 | 检查 Peer 配置 |
| 无法 ping 对端 | `wg show` 查看 handshake | 网络/防火墙问题 | 检查 UDP 端口 51820 |
| 隧道经常断开 | `journalctl -u wg-quick@<interface>` | Keepalive 配置不当 | 设置 PersistentKeepalive |

```bash
# WireGuard 诊断
wg show
wg show <interface>

# 检查公钥是否匹配
# 在 peer 端查看公钥: wg pubkey < privatekey

# 检查防火墙规则
iptables -L -n | grep wg
ufw status

# 手动添加 peer
wg set wg0 peer <public-key> endpoint <ip>:<port> allowed-ips <cidr>

# 日志
journalctl -u wg-quick@wg0 --since "10m"
```

### 3.2 IPSec VPN 问题

| 症状 | 诊断命令 | 根因 | 修复 |
|------|---------|------|------|
| IKE 握手失败 | `ipsec status` | 预共享密钥错误 | 检查 psk 配置 |
| CHILD SA 未建立 | `ipsec status` | Phase 2 参数不匹配 | 检查 encryption/auth 算法 |
| 隧道不稳定 | `ip xfrm state` | NAT-T 配置问题 | 检查 nat_traversal |

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `systemctl stop/restart`：停止/重启系统服务，影响节点上所有容器

```bash
# StrongSwan 诊断
ipsec status
ipsec statusall
ipsec logs

# 检查 IKE SA
ipsec status | grep -E "ESTABLISHED|IKE_SA"

# 检查 CHILD SA
ipsec status | grep -E "CHILD_SA|IPsec"

# 常见问题
# "no matching CHILD_SA config found" → 检查 local/remote subnet 配置
# "IKE_AUTH failed" → 检查 ID / EAP 认证

# 重新启动
systemctl restart strongswan
ipsec restart
```

### 3.3 GRE/IPSec 隧道问题

```bash
# 检查 GRE 隧道
ip tunnel show
ip tunnel add gre1 mode gre remote <remote_ip> local <local_ip>
ip link set gre1 up

# 检查隧道 MTU
ip link show gre1
# GRE MTU 问题: 设置 mtu 1400 避免分片

# 丢包检测
ping -M do -s 1400 <remote_ip>
# 如果大包丢包 → 检查 MTU 配置或启用 PMTUDiscovery

# 隧道流量监控
ip -s tunnel show
```

---

## 4. 网络基础问题排查

### 4.1 网络连通性

```bash
# 基础连通性
ping -c 4 <host>
traceroute -m 10 <host>
mtr -c 10 <host>

# TCP 连通性
nc -zv <host> <port>
telnet <host> <port>

# HTTP/HTTPS 连通性
curl -v --connect-timeout 5 http://<host>:<port>/health

# 路由检查
ip route
route -n
ip route get <ip>

# ARP 表
arp -a
ip neigh show
```

### 4.2 交换机/VLAN 问题

| 症状 | 诊断命令 | 说明 |
|------|---------|------|
| VLAN 间不通 | 检查交换机端口配置 | 确认 trunk/access 模式 |
| MAC 地址漂移 | `show mac address-table` (交换机) | 检查 STP 状态 |
| 端口 down | `show interface status` (交换机) | 检查协商模式 |

```bash
# 常见交换机命令 (Cisco)
show interfaces status
show vlan brief
show spanning-tree
show mac address-table

# 查看端口错误统计
show interfaces GigabitEthernet0/0/1 counters errors
```

### 4.3 防火墙规则问题

```bash
# iptables 检查
iptables -L -n -v --line-numbers
iptables -L INPUT -n --line-numbers

# 查看 NAT 规则
iptables -t nat -L -n -v

# 查看 NAT 转换
conntrack -L | head -20

# 常见问题
# 端口无法访问 → 检查 iptables INPUT/OUTPUT
# SNAT/DNAT 不工作 → 检查 nat 表规则
# 丢包 → 检查 DROP 规则日志

# 排查顺序
# 1. 检查物理连接 (link state)
# 2. 检查 IP 配置 (ip addr)
# 3. 检查路由 (ip route)
# 4. 检查防火墙 (iptables)
# 5. 检查应用监听 (netstat/ss)
```

---

## 5. 快速检查清单

### 网络基础设施 on-call 速查

```bash
# DNS 解析测试
dig @<dns> <domain> +short
nslookup <domain>
host <domain>

# 连通性测试
ping -c 3 <gateway>
traceroute <destination>
mtr -c 5 <destination>

# 负载均衡检查
curl -s http://<lb-vip>/health
openssl s_client -connect <lb-vip>:443 -servername <domain> 2>/dev/null | openssl x509 -noout -dates

# VPN 隧道状态
wg show
ipsec status
netstat -an | grep :500 | grep ESTABLISHED

# SSL 证书检查
echo | openssl s_client -connect <host>:443 2>/dev/null | openssl x509 -noout -dates
```

---

## 6. 升级条件

| 条件 | 操作 |
|------|------|
| 核心路由器/交换机问题 | 立即升级网络团队 |
| VPN 隧道无法建立且无法临时解决 | 升级网络团队 |
| 负载均衡器完全不可用 | 立即升级网络团队 |
| 多站点网络中断 | 立即升级网络团队 + 管理层 |

---

**关联文档**:
- [domain-17-system-foundation/](../domain-17-system-foundation/) — Linux 系统基础
- [domain-03-networking-traffic/](../domain-03-networking-traffic/) — 网络基础
- [domain-03-networking-traffic/](../domain-03-networking-traffic/) — Kubernetes 网络