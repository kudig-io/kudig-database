---
title: 84 - 网络性能调优
description: '# 84 - 网络性能调优'
summary: 'net.netfilter.nf_conntrack_tcp_timeout_established = 86400'
category: networking
tags:
- k8s
- networking
- service
- ingress
- cni
- kubelet
- scheduler
- cilium
- helm
- flannel
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 网络工程师
- 运维工程师
estimated_read_time: 5min
intent_queries:
- 网络性能调优 是什么
- 如何 网络性能调优
- Kubernetes 5 networking 最佳实践
trigger_keywords:
- 网络性能调优
- networking
prerequisites:
- kubectl-basics
- networking-basics
- helm-basics
- cilium-basics
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
  path: ../集群基础/
  label: '相关知识域: 集群基础'
- type: domain
  path: ../网络/
  label: '相关知识域: 网络'
- type: domain
  path: ../可观测性/
  label: '相关知识域: 可观测性'
- type: cheatsheet
  path: ../系统基础/topic-cheat-sheet/networking.md
  label: '速查卡: networking'
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 84 - 网络性能调优

<!-- chunk: 网络性能瓶颈 -->
## 网络性能瓶颈

| 瓶颈类型 | 症状 | 诊断方法 |
|---------|-----|---------|
| 带宽不足 | 吞吐量低 | iperf3测试 |
| 延迟高 | 响应慢 | ping/hping |
| 丢包 | 连接不稳定 | netstat/ss |
| conntrack满 | 新连接失败 | conntrack -L |
| 队列溢出 | 间歇性问题 | ethtool -S |

<!-- chunk: 内核网络参数 -->
## 内核网络参数

```yaml
apiVersion: v1
kind: ConfigMap
metadata:
  name: network-tuning
data:
  sysctl.conf: |
    # conntrack优化
    net.netfilter.nf_conntrack_max = 1048576
    net.netfilter.nf_conntrack_tcp_timeout_established = 86400
    net.netfilter.nf_conntrack_tcp_timeout_close_wait = 3600
    
    # 网络缓冲区
    net.core.rmem_max = 134217728
    net.core.wmem_max = 134217728
    net.core.rmem_default = 16777216
    net.core.wmem_default = 16777216
    net.core.netdev_max_backlog = 65536
    net.core.somaxconn = 65535
    
    # TCP优化
    net.ipv4.tcp_rmem = 4096 87380 134217728
    net.ipv4.tcp_wmem = 4096 65536 134217728
    net.ipv4.tcp_max_syn_backlog = 65536
    net.ipv4.tcp_slow_start_after_idle = 0
    net.ipv4.tcp_tw_reuse = 1
    net.ipv4.tcp_fin_timeout = 30
    net.ipv4.tcp_keepalive_time = 600
    net.ipv4.tcp_keepalive_probes = 5
    net.ipv4.tcp_keepalive_intvl = 15
    
    # 本地端口范围
    net.ipv4.ip_local_port_range = 1024 65535
    
    # MTU发现
    net.ipv4.tcp_mtu_probing = 1
```

<!-- chunk: kubelet网络配置 -->
## kubelet网络配置

```yaml
apiVersion: kubelet.config.k8s.io/v1beta1
kind: KubeletConfiguration
maxPods: 250
podPidsLimit: 4096
```

<!-- chunk: kube-proxy性能优化 -->
## kube-proxy性能优化

```yaml
apiVersion: kubeproxy.config.k8s.io/v1alpha1
kind: KubeProxyConfiguration
mode: ipvs  # 大规模使用IPVS
ipvs:
  scheduler: rr
  syncPeriod: 30s
  minSyncPeriod: 2s
conntrack:
  maxPerCore: 65536
  min: 524288
  tcpEstablishedTimeout: 86400s
  tcpCloseWaitTimeout: 3600s
```

<!-- chunk: CNI带宽限制 -->
## CNI带宽限制

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: bandwidth-limited
  annotations:
    kubernetes.io/ingress-bandwidth: "10M"
    kubernetes.io/egress-bandwidth: "10M"
spec:
  containers:
  - name: app
    image: myapp
```

<!-- chunk: Cilium带宽管理 -->
## Cilium带宽管理

```yaml
# Helm values
apiVersion: v1
kind: ConfigMap
metadata:
  name: cilium-bandwidth
data:
  values.yaml: |
    bandwidthManager:
      enabled: true
      bbr: true  # 启用BBR拥塞控制
```

<!-- chunk: 网卡多队列 -->
## 网卡多队列

```bash
# 检查网卡队列
ethtool -l eth0

# 设置网卡队列数
ethtool -L eth0 combined 8

# 配置IRQ亲和性
# 将网卡中断分散到多个CPU
for i in /proc/irq/*/eth0*/smp_affinity_list; do
  echo "0-7" > $i
done
```

<!-- chunk: MTU优化 -->
## MTU优化

| 环境 | 推荐MTU | 说明 |
|-----|--------|------|
| 物理网络 | 1500 | 默认 |
| VXLAN | 1450 | 50字节开销 |
| WireGuard | 1420 | 80字节开销 |
| Jumbo Frame | 9000 | 需网络支持 |

<!-- chunk: 性能测试命令 -->
## 性能测试命令

```bash
# 带宽测试
iperf3 -s  # 服务端
iperf3 -c <server-ip> -t 30 -P 4  # 客户端,4并发

# 延迟测试
ping -c 100 <target>
hping3 -S -p 80 -c 100 <target>

# 连接数测试
wrk -t4 -c400 -d30s http://<service>/

# TCP连接状态
ss -s
netstat -an | awk '/^tcp/ {++state[$NF]} END {for(k in state) print k,state[k]}'

# conntrack使用
conntrack -L | wc -l
cat /proc/sys/net/netfilter/nf_conntrack_count
cat /proc/sys/net/netfilter/nf_conntrack_max
```

<!-- chunk: 网络监控指标 -->
## 网络监控指标

| 指标 | 类型 | 告警阈值 |
|-----|-----|---------|
| `node_network_receive_bytes_total` | Counter | - |
| `node_network_transmit_bytes_total` | Counter | - |
| `node_network_receive_drop_total` | Counter | >0持续 |
| `node_network_transmit_drop_total` | Counter | >0持续 |
| `node_netstat_Tcp_CurrEstab` | Gauge | 接近端口范围 |
| `node_nf_conntrack_entries` | Gauge | >80%max |

<!-- chunk: 性能告警规则 -->
## 性能告警规则

```yaml
groups:
- name: network-performance
  rules:
  - alert: HighNetworkDrops
    expr: rate(node_network_receive_drop_total[5m]) > 0
    for: 5m
    labels:
      severity: warning
      
  - alert: ConntrackNearFull
    expr: node_nf_conntrack_entries / node_nf_conntrack_entries_limit > 0.8
    for: 5m
    labels:
      severity: critical
      
  - alert: HighNetworkLatency
    expr: histogram_quantile(0.99, rate(http_request_duration_seconds_bucket[5m])) > 1
    for: 10m
    labels:
      severity: warning
```

<!-- chunk: ACK网络优化 -->
## ACK网络优化

| 功能 | 说明 |
|-----|------|
| Terway ENIIP | 零网络损耗 |
| eRDMA | 高性能RDMA |
| 智能网卡 | 硬件卸载 |
| 网络优化镜像 | 预优化内核参数 |

---

**表格底部标记**: Kusheet Project, 作者 Allen Galler (allengaller@gmail.com)

---

<!-- chunk: Obsidian 相关文档 -->
## Obsidian 相关文档

- 网络 KUDIG Database — Global MOC
- [[网络/README.md|[[Domain 5: Networking 网络|Domain 5: Networking 网络]]working]] 网络]]
- [[网络/K8s网络核心/00-network-in-nutshell.md|00 network in nutshell]]
- index.md|Domain-5 网络 — 开源项目索引]]
- FAQ 文档
- 网络核心组件
- CNI 架构与核心原理
- 76 - CNI插件深度对比
- 142 - Flannel 完整指南 (Flannel Complete Guide)
- Flannel WireGuard 加密后端配置
- Flannel IPv6 Dual Stack 支持
- Flannel Windows 节点支持

## See Also

- 32-multi-cluster-networking
- 33-network-troubleshooting
- 35-gateway-api-overview
- 36-api-gateway-patterns

## Related

- [[生态参考/领域索引/terway-index.md|Terway 知识图谱索引]]


<!-- risk-assessed -->
