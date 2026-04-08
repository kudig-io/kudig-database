# 网络诊断速查表

> TCP/IP 网络故障排查与诊断工具快速参考

---

## 目录

- [常用诊断命令](#常用诊断命令)
- [DNS 诊断](#dns-诊断)
- [TCP/UDP 调试](#tcpudp-调试)
- [HTTP/HTTPS 诊断](#httphttps-诊断)
- [路由与防火墙](#路由与防火墙)
- [抓包分析](#抓包分析)
- [Kubernetes 网络诊断](#kubernetes-网络诊断)
- [性能测试](#性能测试)

---

## 常用诊断命令

### 连通性测试

```bash
# ping - 基础连通性
ping google.com
ping -c 4 8.8.8.8                    # 发送4次
ping -i 0.2 192.168.1.1              # 间隔0.2秒
ping -s 1472 google.com              # 大包测试（MTU）

# hping3 - 高级 ping
hping3 -S -p 80 google.com           # SYN ping 端口80
hping3 -A -p 443 192.168.1.1         # ACK ping
hping3 -2 -p 53 8.8.8.8              # UDP ping
hping3 --flood -S -p 80 192.168.1.1  # 压力测试（慎用）

# fping - 批量 ping
fping -g 192.168.1.0/24              # ping 整个网段
fping -f hosts.txt                   # 从文件读取
```

### 查看网络配置

```bash
# IP 地址
ip addr
ip -4 addr                           # 仅 IPv4
ip -6 addr                           # 仅 IPv6
ip addr show eth0                    # 指定接口

# 传统命令（已弃用但常用）
ifconfig
ifconfig eth0

# 路由表
ip route
ip route show default
route -n

# 网络接口统计
ip -s link                           # 所有接口统计
ip -s link show eth0                 # 指定接口
ifconfig eth0

# ARP 表
ip neigh                             # 显示 ARP/邻居表
arp -a
cat /proc/net/arp

# 网络连接统计
ss -s                                # 连接统计摘要
netstat -s                           # 详细统计
```

---

## DNS 诊断

### dig - DNS 查询

```bash
# 基础查询
dig google.com
dig @8.8.8.8 google.com              # 指定 DNS 服务器
dig +short google.com                # 简短输出
dig +trace google.com                # 完整解析追踪

# 特定记录类型
dig google.com A                     # A 记录（IPv4）
dig google.com AAAA                  # AAAA 记录（IPv6）
dig google.com MX                    # 邮件交换记录
dig google.com NS                    # 域名服务器
dig google.com TXT                   # 文本记录
dig google.com SOA                   # 授权起始
dig google.com CNAME                 # 别名记录
dig google.com SRV                   # 服务记录
dig google.com PTR 8.8.8.8.in-addr.arpa  # 反向解析

# 高级选项
dig +tcp google.com                  # 使用 TCP
dig +norecurse google.com            # 禁用递归
dig +time=5 +tries=3 google.com      # 超时和重试
dig +dnssec google.com               # DNSSEC 验证

# 批量查询
for host in google.com baidu.com github.com; do
  dig +short $host
done
```

### nslookup

```bash
# 交互模式
nslookup
> server 8.8.8.8
> set type=MX
> google.com

# 非交互模式
nslookup google.com
nslookup -type=MX google.com 8.8.8.8
```

### host

```bash
host google.com
host -t A google.com
host -t MX google.com
host -a google.com                   # 所有记录
```

### systemd-resolve / resolvectl

```bash
# Ubuntu 18.04+ / systemd
resolvectl query google.com
resolvectl status                    # 解析器状态
systemd-resolve --status

# 刷新 DNS 缓存
sudo systemd-resolve --flush-caches
```

### DNS 故障排查流程

```bash
# 1. 检查本地 hosts
cat /etc/hosts

# 2. 检查 resolv.conf
cat /etc/resolv.conf

# 3. 直接测试 DNS 服务器
dig @127.0.0.53 google.com           # systemd-resolved
dig @8.8.8.8 google.com              # Google DNS
dig @223.5.5.5 google.com            # 阿里云 DNS

# 4. 检查 DNS 解析时间
dig +stats google.com | grep "Query time"

# 5. 追踪完整解析链
dig +trace google.com
```

---

## TCP/UDP 调试

### 端口扫描

```bash
# nc (netcat) - 端口测试
nc -zv google.com 80                 # TCP 端口测试
nc -zv -w 5 google.com 443           # 5秒超时
nc -zvu 8.8.8.8 53                   # UDP 端口测试

# nmap - 端口扫描
nmap -p 80,443 google.com            # 扫描指定端口
nmap -p 1-1000 192.168.1.1           # 扫描端口范围
nmap -sT -O 192.168.1.0/24           # TCP 扫描 + OS 检测
nmap -sU -p 53,161 192.168.1.1       # UDP 扫描
nmap -Pn 192.168.1.1                 # 跳过 ping 检测

# ss / netstat - 查看监听端口
ss -tlnp                             # TCP 监听端口 + 进程
ss -ulnp                             # UDP 监听端口
ss -tlnp | grep :80                  # 查找特定端口
netstat -tlnp                        # 传统方式
```

### 连接跟踪

```bash
# ss - socket 统计
ss -t                                # TCP 连接
ss -u                                # UDP 连接
ss -ta                               # 所有 TCP（包括监听）
ss -tn state established            # 仅 established 连接
ss -tn state time-wait               # TIME_WAIT 连接

# 过滤特定状态
ss -t state established '( dport = :ssh or sport = :ssh )'

# 查看进程
ss -tp
ss -tlp | grep nginx                 # 查找 nginx 端口

# 连接数统计
ss -s
ss -tan | awk '{print $1}' | sort | uniq -c | sort -rn
```

### telnet 测试

```bash
# 测试 TCP 连接
telnet google.com 80

telnet 192.168.1.1 22
Trying 192.168.1.1...
Connected to 192.168.1.1.            # 连接成功
Escape character is '^]'.

# 手动发送 HTTP 请求
$ telnet google.com 80
Trying 142.250.185.78...
Connected to google.com.
Escape character is '^]'.
GET / HTTP/1.1
Host: google.com

```

---

## HTTP/HTTPS 诊断

### curl

```bash
# 基础请求
curl https://api.example.com
curl -I https://example.com          # 仅显示响应头
curl -L https://bit.ly/xxx           # 跟随重定向

# 请求方法
curl -X POST https://api.example.com/users
curl -X PUT https://api.example.com/users/1
curl -X DELETE https://api.example.com/users/1

# 请求头和数据
curl -H "Content-Type: application/json" \\
     -H "Authorization: Bearer TOKEN" \\
     -d '{"name":"test"}' \\
     https://api.example.com/users

# 输出控制
curl -s https://api.example.com      # 静默模式
curl -v https://api.example.com      # 详细模式（调试）
curl -w "@curl-format.txt" -o /dev/null -s https://example.com

# 性能测试
curl -o /dev/null -s -w "%{time_total}\\n" https://example.com
curl -o /dev/null -s -w "DNS: %{time_namelookup}\\nConnect: %{time_connect}\\nTotal: %{time_total}\\n" https://example.com

# 证书信息
curl -vI https://example.com 2>&1 | grep -E "(subject|issuer|expire)"
```

**curl 时间变量**:
| 变量 | 含义 |
|:---|:---|
| `time_namelookup` | DNS 解析时间 |
| `time_connect` | TCP 连接时间 |
| `time_appconnect` | SSL/SSH 握手时间 |
| `time_pretransfer` | 请求发送前时间 |
| `time_redirect` | 重定向时间 |
| `time_starttransfer` | 首字节时间 (TTFB) |
| `time_total` | 总时间 |

### wget

```bash
# 简单下载
wget https://example.com/file.zip

# 后台下载
wget -b https://example.com/large-file.zip

# 断点续传
wget -c https://example.com/large-file.zip

# 指定输出名
wget -O myfile.zip https://example.com/file.zip

# 递归下载
wget -r -l 2 https://example.com/docs/
```

### HTTPie

```bash
# 友好格式的 HTTP 客户端
http GET api.example.com/users
http POST api.example.com/users name=test email=test@example.com
http -v GET api.example.com/users    # 详细输出
http --form POST api.example.com/upload file@photo.jpg
```

### OpenSSL 测试

```bash
# 查看证书链
echo | openssl s_client -connect google.com:443 -showcerts

# 查看证书信息
echo | openssl s_client -connect google.com:443 2>/dev/null | \
  openssl x509 -noout -text

# 查看过期时间
echo | openssl s_client -connect google.com:443 2>/dev/null | \
  openssl x509 -noout -dates

# 测试 SNI
echo | openssl s_client -connect 142.250.185.78:443 -servername google.com

# 测试特定 TLS 版本
echo | openssl s_client -connect google.com:443 -tls1_3
echo | openssl s_client -connect google.com:443 -tls1_2
```

---

## 路由与防火墙

### 路由表

```bash
# 查看路由
ip route
ip route show table main
route -n

# 追踪路由
traceroute google.com
traceroute -I google.com             # ICMP 模式（绕过防火墙）
mtr google.com                       # 持续 traceroute
mtr -r -c 100 google.com             # 报告模式，100次

# 添加/删除路由
sudo ip route add 192.168.10.0/24 via 192.168.1.1
sudo ip route add default via 192.168.1.1
sudo ip route del 192.168.10.0/24

# 策略路由
ip rule list                         # 路由策略
ip route show table 100              # 查看特定表
```

### 防火墙 (iptables/nftables)

```bash
# iptables - 查看规则
sudo iptables -L -n -v               # 列出所有规则
sudo iptables -L INPUT -n -v         # 查看 INPUT 链
sudo iptables -t nat -L -n -v        # NAT 表
sudo iptables -t mangle -L -n -v     # Mangle 表

# 计数器清零
sudo iptables -Z

# 查找规则行号
sudo iptables -L INPUT --line-numbers

# 删除规则
sudo iptables -D INPUT 5             # 删除第5条规则
sudo iptables -D INPUT -p tcp --dport 80 -j ACCEPT

# 临时允许端口
sudo iptables -I INPUT -p tcp --dport 8080 -j ACCEPT

# 保存规则（不同发行版不同）
sudo iptables-save > /etc/iptables/rules.v4  # Debian/Ubuntu
sudo service iptables save                      # CentOS/RHEL

# nftables
sudo nft list ruleset
sudo nft list table inet filter
```

### conntrack

```bash
# 查看连接追踪表
sudo conntrack -L
sudo conntrack -L -p tcp --dport 443

# 统计
sudo conntrack -C                    # 连接数
sudo conntrack -S                    # 统计信息

# 清空
sudo conntrack -F
```

---

## 抓包分析

### tcpdump

```bash
# 基础抓包
sudo tcpdump -i eth0
sudo tcpdump -i any                  # 所有接口

# 过滤条件
sudo tcpdump -i eth0 port 80         # 端口 80
sudo tcpdump -i eth0 tcp port 443    # TCP 443
sudo tcpdump -i eth0 host 8.8.8.8    # 特定主机
sudo tcpdump -i eth0 net 192.168.1.0/24  # 网段
sudo tcpdump -i eth0 icmp            # ICMP

# 高级过滤
sudo tcpdump -i eth0 'port 80 and host 192.168.1.100'
sudo tcpdump -i eth0 'tcp[tcpflags] & tcp-syn != 0'    # SYN 包
sudo tcpdump -i eth0 'tcp port 80 and (((ip[2:2] - ((ip[0]&0xf)<<2)) - ((tcp[12]&0xf0)>>2)) != 0)'  # HTTP 数据

# 输出选项
sudo tcpdump -i eth0 -w capture.pcap        # 写入文件
sudo tcpdump -r capture.pcap                # 读取文件
sudo tcpdump -i eth0 -nn -v                 # 不解析域名，详细输出
sudo tcpdump -i eth0 -c 100                 # 抓100个包后停止
sudo tcpdump -i eth0 -s 0                   # 抓取完整数据包
sudo tcpdump -i eth0 -A                     # ASCII 输出
sudo tcpdump -i eth0 -X                     # 十六进制输出

# 常用组合
sudo tcpdump -i eth0 port 80 -w http.pcap -nn -v
```

### Wireshark tshark

```bash
# 命令行抓包
tshark -i eth0
tshark -i eth0 -f "tcp port 80"
tshark -r capture.pcap

# 过滤并输出
tshark -r capture.pcap -Y "http.request.method == GET"
tshark -r capture.pcap -T fields -e http.host -e http.request.uri
```

### 抓包场景

```bash
# 场景1: 抓取 HTTP 请求
sudo tcpdump -i eth0 'tcp port 80' -w http.pcap -nn

# 场景2: 抓取 DNS 查询
sudo tcpdump -i eth0 'udp port 53' -nn

# 场景3: 抓取特定 IP 的所有流量
sudo tcpdump -i eth0 host 192.168.1.100 -w target.pcap

# 场景4: 抓取 SSH 流量并分析
sudo tcpdump -i eth0 'tcp port 22' -nn -A | grep -i password

# 场景5: 检测 SYN 洪水攻击
sudo tcpdump -i eth0 'tcp[tcpflags] & tcp-syn != 0 and tcp[tcpflags] & tcp-ack == 0'
```

---

## Kubernetes 网络诊断

### Pod 网络调试

```bash
# 进入 Pod 网络命名空间
kubectl debug -it podname --image=nicolaka/netshoot -- /bin/bash

# 临时调试 Pod
kubectl run -it --rm debug --image=nicolaka/netshoot --restart=Never -- bash

# 测试 Service 连通性
kubectl run -it --rm debug --image=busybox:1.28 --restart=Never -- wget -O- http://myservice:8080

# 查看 Pod IP
kubectl get pod -o wide
kubectl get pod -o jsonpath='{.items[*].status.podIP}'
```

### Service 诊断

```bash
# 查看 Service 端点
kubectl get endpoints myservice
kubectl get endpointslices

# 测试 Service DNS
kubectl run -it --rm debug --image=busybox:1.28 --restart=Never -- nslookup kubernetes.default

# 检查 kube-proxy
kubectl get pods -n kube-system -l k8s-app=kube-proxy
kubectl logs -n kube-system -l k8s-app=kube-proxy
```

### CNI 诊断

```bash
# 查看 CNI 配置
cat /etc/cni/net.d/10-calico.conflist
cat /etc/cni/net.d/10-flannel.conflist

# Calico 诊断
calicoctl node status
calicoctl get ippool

# Cilium 诊断
cilium status
cilium endpoint list
cilium monitor

# 查看 iptables NAT 规则（Kube-proxy）
sudo iptables -t nat -L KUBE-SERVICES -n -v
sudo iptables -t nat -L KUBE-POSTROUTING -n -v
```

### CoreDNS 诊断

```bash
# 查看 CoreDNS Pod
kubectl get pods -n kube-system -l k8s-app=kube-dns

# 查看 CoreDNS 日志
kubectl logs -n kube-system -l k8s-app=kube-dns

# 测试 DNS 解析
kubectl run -it --rm debug --image=busybox:1.28 --restart=Never -- nslookup kubernetes.default.svc.cluster.local

# 检查 CoreDNS 配置
kubectl get configmap coredns -n kube-system -o yaml
```

---

## 性能测试

### 带宽测试

```bash
# iperf3 - 带宽测试
# 服务端
iperf3 -s

# 客户端
iperf3 -c server_ip
iperf3 -c server_ip -t 30              # 30秒测试
iperf3 -c server_ip -P 10              # 10个并行流
iperf3 -c server_ip -u -b 1G           # UDP 测试，1Gbps

# 反向测试
iperf3 -c server_ip -R
```

### HTTP 压力测试

```bash
# ab (Apache Bench)
ab -n 10000 -c 100 http://localhost:8080/

# wrk
wrk -t12 -c400 -d30s http://localhost:8080/

# hey
go install github.com/rakyll/hey@latest
hey -n 10000 -c 100 http://localhost:8080/

# vegeta
echo "GET http://localhost:8080/" | vegeta attack -duration=30s -rate=100 | vegeta report
```

### 延迟测试

```bash
# 检查 RTT
ping -c 100 target | tail -2

# tcpping
tcpping -x 100 target port

# httping
httping -g http://example.com -c 100
```

---

## 相关文档

- [domain-15-network-fundamentals/](../domain-15-network-fundamentals/) - 网络基础
- [domain-5-networking/](../domain-5-networking/) - Kubernetes 网络
- [domain-3-control-plane/23-container-network-deep-dive.md](../domain-3-control-plane/23-container-network-deep-dive.md) - CNI 深度解析
