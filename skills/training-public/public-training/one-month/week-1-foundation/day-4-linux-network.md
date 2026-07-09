---
title: 'Day 4: Linux 网络 + 性能调优'
description: '# Day 4: Linux 网络 + 性能调优'
summary: 'docker rm -f iptables-test  # ⚠️ 强制清理，可能杀运行中容器'
category: learning
tags:
- k8s
- training
- hands-on
- docker
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 'Day 4: Linux 网络 + 性能调优 是什么'
- '如何 Day 4: Linux 网络 + 性能调优'
trigger_keywords:
- Day
- '4:'
- Linux
- 网络
- 性能调优
- learn
prerequisites:
- kubectl-basics
- gpu-ml-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Day 4: Linux 网络 + 性能调优

```yaml
---
id: LEARN-ONE-MONTH-W1-DAY4
title: Day 4 - Linux 网络 + 性能调优
topic: linux
type: hands-on-guide
tags: [linux, network, ip, iptables, tcpdump, sysctl, performance, tuning, hands-on, week-1]
last_updated: 2026-05-18
difficulty: intermediate
intent_queries:
  - "Linux 网络命令怎么用"
  - "ip netns 网络命名空间怎么用"
  - "iptables NAT 规则怎么看"
  - "K8s 节点内核参数怎么调优"
trigger_keywords:
  - ip addr
  - ip route
  - ip netns
  - veth
  - iptables
  - NAT
  - tcpdump
  - ss
  - sysctl
  - ip_forward
  - 内核调优
  - 网络排障
reading_level: intermediate
audience:
  - sre
  - ops-engineer
estimated_read_time: 40min
related_domains:
  - 系统基础
  - 网络
related_topics:
  - linux
  - networking
  - performance
related:
  - 生产运维/topic-learn/public-training/one-month/week-1-foundation/day-3-linux-core.md
  - 系统基础/04-linux-networking-configuration.md
---
```

> **学习时间**: 4-5 小时 | **主题**: Linux 网络配置与内核调优

---

## 今日目标

- [ ] 掌握 Linux 网络配置 (ip、iptables、路由)
- [ ] 理解网络命名空间 (K8s 网络基础)
- [ ] 了解内核参数调优 (生产环境必备)

---

## 理论学习 (2h)

### 必读文档

1. **Linux 网络配置**
   - 文件: `../../系统基础/04-linux-networking-configuration.md`
   - 重点: ip 命令、iptables 基础、路由表、网络命名空间

2. **性能调优**
   - 文件: `../../系统基础/06-linux-performance-tuning.md`
   - 重点: 内核参数调优 (K8s 生产必做)

### 补充阅读

3. **运维基础操作**
   - 文件: `../../系统基础/09-linux-operations-basics.md`

---

## 实践任务 (2.5h)

### 任务 1: IP 命令实践 (45min)

```bash
# 查看网络接口
ip addr show
ip link show

# 查看路由表
ip route show
ip route get 8.8.8.8

# 查看 ARP 缓存
ip neigh show

# 临时添加 IP 地址 (测试用)
sudo ip addr add 192.168.100.1/24 dev lo
ip addr show lo
sudo ip addr del 192.168.100.1/24 dev lo

# 查看网络统计
ip -s link show eth0
```

### 任务 2: 网络命名空间实验 (45min)

这是理解 K8s Pod 网络的关键:

```bash
# 创建两个网络命名空间 (模拟两个 Pod)
sudo ip netns add ns1
sudo ip netns add ns2

# 创建 veth pair (虚拟网卡对)
sudo ip link add veth1 type veth peer name veth2

# 将 veth 放入各自的 namespace
sudo ip link set veth1 netns ns1
sudo ip link set veth2 netns ns2

# 配置 IP 地址
sudo ip netns exec ns1 ip addr add 10.0.0.1/24 dev veth1
sudo ip netns exec ns2 ip addr add 10.0.0.2/24 dev veth2

# 启动接口
sudo ip netns exec ns1 ip link set veth1 up
sudo ip netns exec ns2 ip link set veth2 up
sudo ip netns exec ns1 ip link set lo up
sudo ip netns exec ns2 ip link set lo up

# 测试通信
sudo ip netns exec ns1 ping -c 3 10.0.0.2

# 查看各 namespace 的网络配置
sudo ip netns exec ns1 ip addr
sudo ip netns exec ns2 ip addr

# 清理
sudo ip netns delete ns1
sudo ip netns delete ns2
```

### 任务 3: iptables 基础 (30min)

> ⚠️ **🔴 灾难性操作** — 含不可逆命令，执行前必须满足变更窗口+双人复核+事前备份+回滚方案
> - `docker prune/rm -f`：强制清理镜像/容器/卷，运行中容器会被杀

> **🔴 高风险操作警告**
>
> 下方命令属于不可逆或高影响操作，执行前请确认：
> - 已备份关键数据与配置
> - 处于批准的变更窗口期
> - 已获得相关责任人授权
> - 已准备回滚或恢复方案
> - 目标集群、Namespace、节点/资源名称正确无误

``` bash
# 🔴 高风险：可能造成数据丢失或服务中断，执行前需备份、变更审批与回滚方案
# 查看 iptables 规则
sudo iptables -L -n -v
sudo iptables -t nat -L -n -v

# 查看 Docker 创建的规则
docker run -d --name iptables-test -p 8888:80 nginx:alpine
sudo iptables -t nat -L -n | grep 8888

# 理解 DNAT 规则 (端口映射原理)
sudo iptables -t nat -L DOCKER -n -v

# 清理
docker rm -f iptables-test  # ⚠️ 强制清理，可能杀运行中容器
```
### 任务 4: 内核参数调优 (30min)

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `sysctl -w`：实时修改内核参数，全局生效

```bash
# 查看当前内核参数
sysctl -a | head -50

# 常用网络参数
sysctl net.ipv4.ip_forward                    # IP 转发 (K8s 必须开启)
sysctl net.bridge.bridge-nf-call-iptables     # 桥接流量经过 iptables
sysctl net.core.somaxconn                     # 监听队列大小

# 临时修改参数
sudo sysctl -w net.ipv4.ip_forward=1

# 查看文件描述符限制
ulimit -n
cat /proc/sys/fs/file-max

# K8s 节点常用调优参数
cat << 'EOF'
# /etc/sysctl.d/k8s.conf 示例
net.ipv4.ip_forward = 1
net.bridge.bridge-nf-call-iptables = 1
net.bridge.bridge-nf-call-ip6tables = 1
vm.swappiness = 0
vm.overcommit_memory = 1
net.ipv4.tcp_max_syn_backlog = 8096
net.core.somaxconn = 32768
fs.file-max = 1000000
fs.inotify.max_user_watches = 524288
EOF
```

### 任务 5: 网络排障工具 (30min)

```bash
# ping - 连通性测试
ping -c 3 8.8.8.8

# traceroute - 路由追踪
traceroute 8.8.8.8

# dig/nslookup - DNS 查询
dig google.com
nslookup google.com

# curl - HTTP 测试
curl -v http://localhost:8080

# tcpdump - 抓包 (排障利器)
sudo tcpdump -i any port 80 -n

# ss - Socket 统计
ss -tuln                  # TCP/UDP 监听端口
ss -s                     # 统计信息

# 查看连接状态统计
ss -ant | awk '{print $1}' | sort | uniq -c | sort -rn
```

---

## 费曼复述 (0.5h)

用自己的语言回答:

1. **网络命名空间是什么？K8s 中 Pod 的网络是如何隔离的？**

2. **iptables 的 NAT 表在 K8s 中有什么作用？**
   - 提示: [[Service|Service]] 的 ClusterIP 实现

3. **为什么 K8s 节点需要开启 ip_forward？**

---

## 今日检验

- [ ] 能够使用 ip 命令查看和配置网络
- [ ] 能够创建网络命名空间并实现通信
- [ ] 理解 iptables NAT 规则的基本原理
- [ ] 知道 K8s 节点需要调优的关键内核参数

---

## 重要命令速查

| 命令 | 用途 | 示例 |
|------|------|------|
| `ip addr` | 查看 IP 地址 | `ip addr show eth0` |
| `ip route` | 查看路由表 | `ip route get 10.0.0.1` |
| `ip netns` | 网络命名空间 | `ip netns exec ns1 ip addr` |
| `iptables` | 防火墙规则 | `iptables -t nat -L -n` |
| `ss` | Socket 统计 | `ss -tuln` |
| `tcpdump` | 网络抓包 | `tcpdump -i eth0 port 80` |

---

## 明日预告

Day 5 将正式进入 [[entities/kubernetes.md|[[Kubernetes|kubernetes]]]] 架构学习，理解 Master/Node 组件及其交互。


<!-- risk-assessed -->
