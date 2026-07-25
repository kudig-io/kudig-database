---
title: 144 - CNI 故障排查与优化 (CNI Troubleshooting & Optimization)
description: '# 144 - CNI 故障排查与优化 (CNI Troubleshooting & Optimization)'
summary: 'Warning  FailedCreatePodSandBox  kubelet  Failed to create pod sandbox:'
category: networking
tags:
- k8s
- networking
- service
- ingress
- cni
- kubelet
- scheduler
- prometheus
- cilium
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
- CNI 故障排查与优化 (CNI Troubleshooting & Optimization) 是什么
- 如何 CNI 故障排查与优化 (CNI Troubleshooting & Optimization)
- Kubernetes 5 networking 最佳实践
- CNI 故障排查与优化 (CNI Troubleshooting & Optimization) 故障排查
- CNI 故障排查与优化 (CNI Troubleshooting & Optimization) 排障步骤
trigger_keywords:
- CNI
- 故障排查与优化
- CNI
- Troubleshooting
- Optimization
- networking
prerequisites:
- kubectl-basics
- networking-basics
- prometheus-basics
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




# 144 - CNI 故障排查与优化 (CNI Troubleshooting & Optimization)

> **适用版本**: [[Kubernetes|Kubernetes]] v1.25 - v1.32 | **难度**: 高级 | **最后更新**: 2026-03

---

<!-- chunk: 1. CNI 问题分级 -->
## 1. CNI 问题分级

| 级别 | 类型 | 影响 | 优先级 |
|:---|:---|:---|:---|
| **P0** | Pod 网络初始化失败 | Pod 无法启动 | 紧急 |
| **P1** | Pod 间通信问题 | 服务不可用 | 高 |
| **P2** | [[Service|Service]] 访问异常 | 部分功能异常 | 中 |
| **P3** | 网络性能问题 | 延迟/丢包 | 低 |

---

<!-- chunk: 2. 系统化排查流程 -->
## 2. 系统化排查流程

```
┌─────────────────────────────────────────────────────────────────────────┐
│                       CNI 故障排查流程图                                 │
└─────────────────────────────────────────────────────────────────────────┘

                    ┌───────────────┐
                    │  问题发现     │
                    └───────┬───────┘
                            │
                            ▼
              ┌─────────────────────────┐
              │ Step 1: CNI 组件检查    │
              │ - CNI Pod 状态          │
              │ - CNI 配置文件          │
              │ - CNI 二进制文件        │
              └───────────┬─────────────┘
                          │
            ┌─────────────┼─────────────┐
            │ 正常        │             │ 异常
            ▼             │             ▼
┌───────────────────┐     │    ┌───────────────────┐
│ Step 2: 节点网络  │     │    │ 修复 CNI 组件    │
│ - 网络接口        │     │    │ - 重启 Pod       │
│ - 路由表          │     │    │ - 重新部署       │
│ - iptables        │     │    │ - 检查权限       │
└─────────┬─────────┘     │    └───────────────────┘
          │               │
    ┌─────┼─────┐         │
    │正常 │     │异常      │
    ▼     │     ▼         │
┌─────────┴───────────┐   │
│ Step 3: Pod 网络    │   │
│ - 网络命名空间      │   │
│ - veth 接口         │   │
│ - IP 配置           │   │
└─────────┬───────────┘   │
          │               │
    ┌─────┼─────┐         │
    │正常 │     │异常      │
    ▼     │     ▼         │
┌─────────┴───────────┐   │
│ Step 4: 连通性测试  │   │
│ - 同节点 Pod        │   │
│ - 跨节点 Pod        │   │
│ - Service/DNS       │   │
└─────────────────────┘   │
```

---

<!-- chunk: 3. 诊断命令速查表 -->
## 3. 诊断命令速查表

| 检查项 | 命令 |
|:---|:---|
| **CNI Pod 状态** | `kubectl get [[Pods|pods]] -n kube-system -l k8s-app=calico-node` |
| **CNI 配置** | `cat /etc/cni/net.d/*.conflist` |
| **CNI 插件** | `ls -la /opt/cni/bin/` |
| **网络接口** | `ip link show` |
| **路由表** | `ip route show` |
| **iptables** | `iptables -t nat -L -n -v` |
| **IPVS** | `ipvsadm -Ln` |
| **Pod 网络** | `kubectl exec <pod> -- ip addr` |
| **DNS 测试** | `kubectl exec <pod> -- nslookup kubernetes` |
| **连通性** | `kubectl exec <pod> -- ping <target-ip>` |

---

<!-- chunk: 4. Pod 网络初始化失败 -->
## 4. Pod 网络初始化失败

### 4.1 现象

```
Events:
  Warning  FailedCreatePodSandBox  kubelet  Failed to create pod sandbox: 
    rpc error: code = Unknown desc = failed to setup network for sandbox: 
    plugin type="calico" failed: error getting ClusterInformation: 
    connection refused
```

### 4.2 排查步骤

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 1. 检查 CNI 配置文件
ls -la /etc/cni/net.d/
cat /etc/cni/net.d/10-calico.conflist

# 2. 检查 CNI 插件
ls -la /opt/cni/bin/
/opt/cni/bin/calico --version

# 3. 检查 CNI Pod 状态
kubectl get pods -n kube-system -l k8s-app=calico-node
kubectl describe pod -n kube-system calico-node-xxxxx

# 4. 检查 CNI 日志
kubectl logs -n kube-system calico-node-xxxxx -c calico-node --tail=100
journalctl -u kubelet | grep -i cni

# 5. 检查 kubelet 日志
journalctl -u kubelet | grep -i "network plugin"
```
### 4.3 常见原因与解决

| 原因 | 解决方案 |
|:---|:---|
| CNI 配置文件缺失 | 重新部署 CNI |
| CNI 二进制缺失 | 复制或重新安装 CNI 插件 |
| CNI Pod 未就绪 | 检查 CNI Pod 状态和日志 |
| IPAM 分配失败 | 检查 IP 池配置和可用 IP |

---

<!-- chunk: 5. IPAM 故障排查 -->
## 5. IPAM 故障排查

### 5.1 Calico IPAM

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看 IP 池
kubectl get ippools -o yaml

# 查看 IP 分配
calicoctl ipam show

# 查看节点 IP 块
calicoctl ipam show --show-blocks

# 检查 IP 使用情况
calicoctl ipam check

# 释放未使用的 IP
calicoctl ipam release --ip=10.244.1.100
```
### 5.2 Flannel IPAM

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看子网分配
cat /run/flannel/subnet.env

# 检查节点子网注解
kubectl get nodes -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{.spec.podCIDR}{"\n"}{end}'
```
### 5.3 Terway IPAM

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 查看 ENI IP 分配
kubectl exec -n kube-system <terway-pod> -c terway -- terway-cli show

# 查看节点 ENI 状态
kubectl get nodes -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{.metadata.annotations.k8s\.aliyun\.com/allocated-eniips}{"\n"}{end}'
```
---

<!-- chunk: 6. 同节点 Pod 通信问题 -->
## 6. 同节点 Pod 通信问题

### 6.1 排查路径

```
Pod A ──▶ veth ──▶ Bridge/路由 ──▶ veth ──▶ Pod B
         │              │              │
         检查点1       检查点2        检查点3
```

### 6.2 veth pair 深度诊断

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# ========== 定位 Pod 对应的 veth ==========
POD_IFINDEX=$(kubectl exec -it <pod-a> -- cat /sys/class/net/eth0/iflink | tr -d '\r')
VETH_A=$(ip link show | grep "^${POD_IFINDEX}:" | awk '{print $2}' | tr -d ':@')
echo "Pod A 的 veth: $VETH_A"

# ========== 检查 veth 状态 ==========
ip -s link show $VETH_A
# 关注: RX/TX errors, dropped

ethtool -S $VETH_A
# 关注: tx_dropped, rx_dropped

# ========== 检查 veth 连接的 bridge/路由 ==========
# Flannel:
bridge link show | grep $VETH_A

# Calico:
ip route show | grep $VETH_A
```
### 6.3 诊断命令

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 1. 查看 Pod 网络配置
kubectl exec pod-a -- ip addr
kubectl exec pod-a -- ip route

# 2. 查看 veth pair
ip link show | grep veth
bridge link show

# 3. 检查 bridge/cni0
ip addr show cni0
bridge fdb show br cni0

# 4. 抓包分析
tcpdump -i cni0 -n host <pod-a-ip> and host <pod-b-ip>

# 5. 检查 iptables
iptables -t filter -L FORWARD -n -v
```
---

<!-- chunk: 7. 跨节点 Pod 通信问题 -->
## 7. 跨节点 Pod 通信问题

### 7.1 VXLAN 模式排查

```bash
# 1. 检查 VXLAN 接口
ip -d link show flannel.1

# 2. 检查 FDB 表
bridge fdb show dev flannel.1

# 3. 检查路由
ip route | grep <target-pod-cidr>

# 4. 检查防火墙 (UDP 8472/4789)
iptables -t filter -L INPUT -n | grep -E "8472|4789"
ss -ulnp | grep -E "8472|4789"

# 5. 抓包 (外层封装)
tcpdump -i eth0 -n udp port 8472

# 6. 验证 MTU
ping -M do -s 1422 <target-pod-ip>  # VXLAN: 1500-78=1422
```

### 7.2 BGP/路由模式排查

```bash
# 1. 检查 BGP 状态 (Calico)
calicoctl node status

# 2. 检查路由表
ip route | grep <target-node-ip>

# 3. 检查 BGP 邻居
calicoctl get bgpPeer -o yaml

# 4. 路由跟踪
traceroute <target-pod-ip>
```

### 7.3 多跳 tcpdump 并行抓包

跨节点通信失败时，需要在数据路径的每一跳同时抓包。

```bash
# 源节点:
tcpdump -i <src-veth> -nn host <dst-pod-ip> -c 20 &   # 跳1: Pod veth
tcpdump -i flannel.1 -nn host <dst-pod-ip> -c 20 &    # 跳2: 隧道口
tcpdump -i eth0 -nn host <dst-node-ip> and udp port 4789 -c 20 &  # 跳3: 出口

# 目的节点:
tcpdump -i eth0 -nn host <src-node-ip> and udp port 4789 -c 20 &  # 跳4: 入口
tcpdump -i <dst-veth> -nn host <src-pod-ip> -c 20 &   # 跳5: Pod veth

# 对比各跳包数量，确定丢包位置
```

### 7.4 Node-to-Node 底层网络诊断

跨节点 Pod 不通时，先确认节点间底层网络是否正常。

```bash
# L3 连通性
ping -c 5 -W 2 <other-node-ip>
traceroute -n <other-node-ip>

# L2 ARP 解析
ip neigh show | grep <other-node-ip>
# FAILED → ARP 解析失败，检查安全组/VLAN

# ARP 表容量（大集群关注）
ip neigh show | wc -l
sysctl net.ipv4.neigh.default.gc_thresh3  # 默认 1024

# 网卡状态
ethtool eth0 | grep -E "Speed|Duplex|Link detected"
ethtool -S eth0 | grep -E "error|drop"

# 云平台安全组必须放行的端口:
# VXLAN: UDP 4789 | Cilium: UDP 8472 | IPIP: 协议4 | BGP: TCP 179
```

---

<!-- chunk: 8. DNS 解析问题 -->
## 8. DNS 解析问题

### 8.1 排查步骤

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 1. 检查 CoreDNS 状态
kubectl get pods -n kube-system -l k8s-app=kube-dns

# 2. 检查 DNS Service
kubectl get svc -n kube-system kube-dns

# 3. 检查 Pod DNS 配置
kubectl exec <pod> -- cat /etc/resolv.conf

# 4. DNS 解析测试
kubectl exec <pod> -- nslookup kubernetes.default
kubectl exec <pod> -- nslookup <service-name>.<namespace>

# 5. CoreDNS 日志
kubectl logs -n kube-system -l k8s-app=kube-dns --tail=100
```
### 8.2 常见问题

| 问题 | 原因 | 解决方案 |
|:---|:---|:---|
| DNS 超时 | CoreDNS Pod 异常 | 重启 CoreDNS |
| NXDOMAIN | Service 不存在 | 检查 Service 名称和命名空间 |
| 解析到错误 IP | DNS 缓存 | 清理 CoreDNS 缓存 |

---

<!-- chunk: 9. 网络性能优化 -->
## 9. 网络性能优化

### 9.1 MTU 优化

```yaml
# Calico MTU 配置
apiVersion: v1
kind: ConfigMap
metadata:
  name: calico-config
  namespace: kube-system
data:
  veth_mtu: "1440"  # VXLAN: 1500-60=1440
```

**MTU 计算参考**:
| CNI 模式 | 封装开销 | 推荐 Pod MTU (物理 1500) |
|----------|----------|---------------------------|
| VXLAN | 50B | 1450 |
| IPIP | 20B | 1480 |
| BGP | 0B | 1500 |
| WireGuard | 60B | 1440 |

### 9.2 启用 eBPF (Cilium)

```yaml
# cilium-config ConfigMap
apiVersion: v1
kind: ConfigMap
metadata:
  name: cilium-config
  namespace: kube-system
data:
  enable-bpf-masquerade: "true"
  kube-proxy-replacement: "strict"
  enable-bandwidth-manager: "true"
```

### 9.3 IPVS 模式

```yaml
# kube-proxy ConfigMap
apiVersion: v1
kind: ConfigMap
metadata:
  name: kube-proxy
  namespace: kube-system
data:
  config.conf: |
    mode: "ipvs"
    ipvs:
      scheduler: "rr"
      strictARP: true
```

---

<!-- chunk: 10. 关键内核网络参数 -->
## 10. 关键内核网络参数

以下参数直接影响 Kubernetes 网络连通性。

| 参数 | K8s 推荐值 | 错误配置影响 |
|------|-----------|---------------|
| `net.ipv4.ip_forward` | **1** | Pod 无法跨节点通信 |
| `net.bridge.bridge-nf-call-iptables` | **1** | Service ClusterIP 不通 |
| `net.ipv4.conf.all.rp_filter` | **0/2** | Pod-to-Node 回包被丢弃 |
| `net.netfilter.nf_conntrack_max` | **262144+** | conntrack 表满，随机丢包 |
| `net.ipv4.neigh.default.gc_thresh3` | **8192** | 大集群 ARP 溢出 |

```bash
# 一键检查
for p in net.ipv4.ip_forward net.bridge.bridge-nf-call-iptables \
    net.ipv4.conf.all.rp_filter net.netfilter.nf_conntrack_max \
    net.netfilter.nf_conntrack_count net.ipv4.neigh.default.gc_thresh3; do
    printf "%-50s %s\n" "$p" "$(sysctl -n $p 2>/dev/null || echo N/A)"
done
```

---

<!-- chunk: 11. conntrack 诊断 -->
## 11. conntrack 诊断

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `sysctl -w`：实时修改内核参数，全局生效

```bash
# 使用率
CT_COUNT=$(sysctl -n net.netfilter.nf_conntrack_count)
CT_MAX=$(sysctl -n net.netfilter.nf_conntrack_max)
echo "conntrack: $CT_COUNT / $CT_MAX ($((CT_COUNT*100/CT_MAX))%)"

# 统计信息
conntrack -S
# insert_failed > 0 → 表满丢包

# 内核报错
dmesg | grep "nf_conntrack: table full"

# 按状态统计
conntrack -L 2>/dev/null | awk '{print $4}' | sort | uniq -c | sort -rn

# 调优
sysctl -w net.netfilter.nf_conntrack_max=262144
sysctl -w net.netfilter.nf_conntrack_tcp_timeout_time_wait=30
```

---

<!-- chunk: 12. iptables TRACE 链路追踪 -->
## 12. iptables TRACE 链路追踪

```bash
# 启用追踪
modprobe nf_log_ipv4
iptables -t raw -A PREROUTING -s <src-ip> -d <dst-ip> -j TRACE
iptables -t raw -A OUTPUT -s <src-ip> -d <dst-ip> -j TRACE

# 查看输出
dmesg -w | grep TRACE
# 格式: TRACE: <table>:<chain>:<rule|policy>:<num> IN=<iface> ...
# 最后一条 TRACE 后停止 → 该规则为 DROP/REJECT 位置

# ⭐ 完成后必须清理
iptables -t raw -D PREROUTING -s <src-ip> -d <dst-ip> -j TRACE
iptables -t raw -D OUTPUT -s <src-ip> -d <dst-ip> -j TRACE
```

---

<!-- chunk: 13. 监控告警 -->
## 13. 监控告警

### 10.1 关键指标

```yaml
# Prometheus 告警规则
groups:
  - name: cni-alerts
    rules:
      - alert: CNIPodNotReady
        expr: |
          kube_pod_status_ready{namespace="kube-system", pod=~"calico.*|flannel.*|cilium.*"} == 0
        for: 5m
        labels:
          severity: critical
        annotations:
          summary: "CNI Pod 未就绪"
          
      - alert: PodNetworkLatencyHigh
        expr: |
          histogram_quantile(0.99, rate(container_network_receive_packets_total[5m])) > 100
        for: 10m
        labels:
          severity: warning
        annotations:
          summary: "Pod 网络延迟过高"
          
      - alert: ConntrackTableNearFull
        expr: |
          node_nf_conntrack_entries / node_nf_conntrack_entries_limit > 0.7
        for: 5m
        labels:
          severity: warning
        annotations:
          summary: "conntrack 表使用率超过 70%"
          
      - alert: IPPoolExhausted
        expr: |
          (calico_ipam_blocks_per_node - calico_ipam_blocks_per_node_free) / calico_ipam_blocks_per_node > 0.9
        for: 5m
        labels:
          severity: warning
        annotations:
          summary: "IP 池即将耗尽"

```

---

<!-- chunk: 14. 生产案例 -->
## 14. 生产案例

### 案例 1: 大促期间 conntrack 表满导致随机丢包

**现象**: 约 2% HTTP 请求超时，重试成功

**排查**: `dmesg` 发现 `nf_conntrack: table full`，`conntrack -S` 显示 `insert_failed > 0`

**解决**: `sysctl -w net.netfilter.nf_conntrack_max=524288`，长期迁移到 Cilium eBPF

### 案例 2: rp_filter 导致 Pod 无法访问宿主机

**现象**: `ping <host-ip>` 超时，但 `ping <pod-ip>` 正常

**排查**: `sysctl net.ipv4.conf.all.rp_filter` = 1，回包路径不对称被丢弃

**解决**: `sysctl -w net.ipv4.conf.all.rp_filter=0`

### 案例 3: 500+ 节点集群 ARP 表溢出

**现象**: 新节点上的 Pod 无法跨节点通信

**排查**: `dmesg` 显示 `neighbour: arp_cache: neighbor table overflow!`

**解决**: `sysctl -w net.ipv4.neigh.default.gc_thresh3=8192`

---

<!-- chunk: 15. 最佳实践清单 -->
## 15. 最佳实践清单

| 类别 | 建议 |
|:---|:---|
| **预防** | 定期检查 CNI Pod 状态和日志 |
| **监控** | 配置网络延迟、丢包、IP 池告警 |
| **文档** | 记录网络架构和 CNI 配置 |
| **测试** | 定期执行网络连通性测试 |
| **备份** | 备份 CNI 配置和 IPAM 数据 |
| **内核参数** | 确保 ip_forward、bridge-nf-call-iptables、rp_filter、conntrack 配置正确 |
| **conntrack** | 监控使用率，配置 Prometheus 告警（阈值 70%）|
| **ARP** | 大集群 (200+ 节点) 调大 gc_thresh 参数 |

---

**表格底部标记**: Kusheet Project | 作者: Allen Galler (allengaller@gmail.com)

---

<!-- chunk: Obsidian 相关文档 -->
## Obsidian 相关文档

- 网络 MOC
- [[05-网络/README.md|Domain 03: Networking 网络]]
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
- [[19-故障诊断/06-FTA故障树/list/apiserver-fta.md|API Server 异常故障树分析]]
- [[19-故障诊断/06-FTA故障树/list/backup-restore-fta.md|备份/恢复异常故障树分析]]
- [[19-故障诊断/06-FTA故障树/list/calico-fta.md|calico FTA 树：Calico CNI 故障诊断]]

## See Also

- 25-ingress-monitoring-troubleshooting
- 26-ingress-production-best-practices
- 28-coredns-troubleshooting-optimization
- 29-egress-traffic-management

## Related

- [[21-生态参考/03-领域索引/terway-index.md|Terway 知识图谱索引]]

```

<!-- risk-assessed -->
