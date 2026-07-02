---
title: 网络连通性故障排查
description: '# 25 - 网络连通性故障排查 (Network Connectivity Troubleshooting)'
summary: 'kubectl get pods -n <namespace> -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{.status.conditions[?(@.type=="Ready")].status}{"\n"}{end}' | grep False'
category: troubleshooting
tags:
- network
- connectivity
- cni
- node
- pod-to-pod
- cross-node
- prometheus
- cilium
- flannel
- calico
tier: core
created: '2026-05-23'
last_updated: 2026-03
difficulty: intermediate
reading_level: intermediate
audience:
- SRE
- 运维工程师
- 技术支持
estimated_read_time: 5min
intent_queries:
- Pod 连不通
- 网络不通
- 跨节点通信失败
- ping 不通
trigger_keywords:
- 网络连通性故障排查
- troubleshooting
prerequisites:
- kubectl-basics
- troubleshooting-methodology
- prometheus-basics
- ebpf-basics
- cilium-basics
- cni-basics
- etcd-basics
k8s_versions:
- 1.25
- 1.26
- 1.27
- 1.28
- 1.29
- 1.3
- 1.31
- 1.32
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
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 25 - 网络连通性故障排查 (Network Connectivity Troubleshooting)

---

<!-- chunk: 1. 网络连通性故障诊断总览 (Network Connectivity Diagnosis Overview) -->
## 1. 网络连通性故障诊断总览 (Network Connectivity Diagnosis Overview)

### 1.1 常见问题现象分类

| 问题类型 | 症状表现 | 影响范围 | 紧急程度 |
|---------|---------|---------|---------|
| **Pod间通信失败** | 同集群Pod无法互访 | 微服务调用失败 | P0 - 紧急 |
| **Service访问异常** | ClusterIP服务不可达 | 服务发现失效 | P0 - 紧急 |
| **Pod-to-Node 不通** | Pod无法访问宿主机或其他节点 | 健康检查/监控异常 | P1 - 高 |
| **Node-to-Node 不通** | 节点间网络不可达 | 集群整体瘫痪 | P0 - 紧急 |
| **外部网络阻断** | 无法访问互联网 | 依赖服务中断 | P1 - 高 |
| **DNS解析失败** | 域名无法解析 | 服务调用失败 | P0 - 紧急 |
| **网络策略阻断** | 合法流量被拦截 | 业务功能异常 | P1 - 高 |

### 1.2 网络连通性架构回顾

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                   网络连通性故障诊断架构                                      │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  ┌─────────────────────────────────────────────────────────────────────┐  │
│  │                      应用层通信                                        │  │
│  │  ┌─────────────┐    ┌─────────────┐    ┌─────────────┐              │  │
│  │  │   Pod-A     │    │   Pod-B     │    │   Pod-C     │              │  │
│  │  │  (Client)   │    │  (Server)   │    │  (Database) │              │  │
│  │  └─────────────┘    └─────────────┘    └─────────────┘              │  │
│  └─────────────────────────────────────────────────────────────────────┘  │
│                              │                                              │
│         ┌────────────────────┼────────────────────┐                       │
│         │                    │                    │                       │
│         ▼                    ▼                    ▼                       │
│  ┌─────────────┐    ┌─────────────┐    ┌─────────────┐                   │
│  │   Service   │    │   DNS解析    │   │   网络策略   │                   │
│  │  (ClusterIP)│    │ (CoreDNS)   │   │ (Network    │                   │
│  │   负载均衡   │    │   解析      │   │ Policy)     │                   │
│  └─────────────┘    └─────────────┘    └─────────────┘                   │
│         │                    │                    │                       │
│         ▼                    ▼                    ▼                       │
│  ┌─────────────────────────────────────────────────────────────────────┐  │
│  │                      CNI网络层                                     │  │
│  │  ┌─────────────┐    ┌─────────────┐    ┌─────────────┐              │  │
│  │  │   Calico    │    │   Cilium    │    │   Flannel   │              │  │
│  │  │  (网络插件)  │    │  (eBPF)     │    │  (VXLAN)    │              │  │
│  │  └─────────────┘    └─────────────┘    └─────────────┘              │  │
│  └─────────────────────────────────────────────────────────────────────┘  │
│                              │                                              │
│         ┌────────────────────┼────────────────────┐                       │
│         │                    │                    │                       │
│         ▼                    ▼                    ▼                       │
│  ┌─────────────┐    ┌─────────────┐    ┌─────────────┐                   │
│  │   iptables  │    │    IPVS     │    │    路由     │                   │
│  │   规则链     │    │   负载均衡   │    │   表       │                   │
│  │  (NAT/DNAT) │    │   转发      │    │  (Kernel)   │                   │
│  └─────────────┘    └─────────────┘    └─────────────┘                   │
│                              │                                              │
│                              ▼                                              │
│  ┌─────────────────────────────────────────────────────────────────────┐  │
│  │                      节点网络接口                                    │  │
│  │  ┌─────────────┐    ┌─────────────┐    ┌─────────────┐              │  │
│  │  │   eth0      │    │   tunl0     │    │   vethXXX   │              │  │
│  │  │ (物理网卡)   │    │ (隧道接口)   │    │ (虚拟网卡)   │              │
│  │  └─────────────┘    └─────────────┘    └─────────────┘              │  │
│  └─────────────────────────────────────────────────────────────────────┘  │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

---

<!-- chunk: 2. 数据路径基础与场景化排查 (Data Path Fundamentals & Scenario-Based Diagnosis) -->
## 2. 数据路径基础与场景化排查 (Data Path Fundamentals & Scenario-Based Diagnosis)

### 2.1 [[Kubernetes|Kubernetes]] 数据路径全景

理解数据包经过的完整路径，是精准定位网络问题的基础。

```
Pod A (eth0)
    │
    ▼
veth pair (宿主机侧: caliXXXX / vethXXXX)
    │
    ├── [Flannel] ──▶ cni0 bridge ──▶ VXLAN (flannel.1) ──▶ eth0 ──▶ 物理网络
    │
    ├── [Calico BGP] ──▶ 宿主机路由表 ──▶ eth0 ──▶ BGP路由 ──▶ 物理网络
    │
    ├── [Calico IPIP] ──▶ 宿主机路由表 ──▶ tunl0 ──▶ IPIP封装 ──▶ eth0 ──▶ 物理网络
    │
    └── [Cilium eBPF] ──▶ eBPF datapath ──▶ eth0 ──▶ 物理网络
                │
                ▼
          对端节点 eth0
                │
                ▼
          解封装 (VXLAN/IPIP/BGP)
                │
                ▼
          veth pair (宿主机侧)
                │
                ▼
          Pod B (eth0)
```

**关键排查点**: 数据包在上述路径的每一跳都可能被丢弃。排查时需要从源到目的逐跳抓包确认。

### 2.2 Pod 网络状态验证

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# ========== 1. Pod网络基本信息检查 ==========
# 检查Pod IP分配和所在节点
kubectl get pods -n <namespace> -o wide

# 验证Pod网络就绪状态
kubectl get pods -n <namespace> -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{.status.conditions[?(@.type=="Ready")].status}{"\n"}{end}' | grep False

# 检查节点网络插件状态
kubectl get pods -n kube-system | grep -E "(calico|cilium|flannel|weave)"

# ========== 2. 网络接口检查 ==========
# 进入Pod检查网络接口
kubectl exec -n <namespace> <pod-name> -- ip addr show

# 查看路由表
kubectl exec -n <namespace> <pod-name> -- ip route show

# 检查DNS配置
kubectl exec -n <namespace> <pod-name> -- cat /etc/resolv.conf
```
### 2.3 同节点 Pod-to-Pod 排查

同节点 Pod 通过 veth pair → bridge/路由表 → veth pair 通信，不经过物理网络。

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# ========== 1. 确认两个 Pod 在同一节点 ==========
kubectl get pod <pod-a> <pod-b> -n <namespace> -o wide
# 确认 NODE 列相同

# ========== 2. 从 Pod A 测试连通性 ==========
kubectl exec -it <pod-a> -n <namespace> -- ping -c 3 <pod-b-ip>
kubectl exec -it <pod-a> -n <namespace> -- wget -qO- --timeout=5 http://<pod-b-ip>:<port>

# ========== 3. 在宿主机上抓包验证 ==========
# 找到 Pod A 对应的 veth
POD_A_IFINDEX=$(kubectl exec -it <pod-a> -n <namespace> -- cat /sys/class/net/eth0/iflink 2>/dev/null | tr -d '\r')
VETH_A=$(ip link show | grep "^${POD_A_IFINDEX}:" | awk '{print $2}' | tr -d ':@')

# 同时在 Pod A 的 veth 和 Pod B 的 veth 抓包
tcpdump -i $VETH_A -nn icmp &
# 如果在 Pod A 的 veth 能看到出包，但 Pod B 的 veth 看不到，问题在 bridge/路由

# ========== 4. 检查 bridge（Flannel 模式）或路由（Calico 模式）==========
# Flannel: 检查 cni0 bridge 转发表
bridge fdb show br cni0

# Calico: 检查到 Pod B IP 的路由
ip route get <pod-b-ip>
# 应返回类似: <pod-b-ip> dev caliXXXX scope link
```
### 2.4 跨节点 Pod-to-Pod 排查

跨节点 Pod 通信涉及 overlay/underlay 网络，排查需要在两个节点同时进行。

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# ========== 1. 确认 Pod 在不同节点 ==========
kubectl get pod <pod-a> <pod-b> -n <namespace> -o wide
# 记录: Pod A IP, Pod A Node, Pod B IP, Pod B Node

# ========== 2. 多跳 tcpdump 并行抓包 ==========
# 在 Pod A 所在节点 (Node A) 上同时开 3 个抓包:

# (Terminal 1) Pod A 的 veth pair 上抓包
tcpdump -i <veth-a> -nn host <pod-b-ip> -c 20

# (Terminal 2) 隧道接口或物理网卡上抓包
# VXLAN 模式:
tcpdump -i flannel.1 -nn host <pod-b-ip> -c 20
# 或 IPIP 模式:
tcpdump -i tunl0 -nn host <pod-b-ip> -c 20
# 或 BGP 模式:
tcpdump -i eth0 -nn host <pod-b-ip> -c 20

# (Terminal 3) 物理网卡出口
tcpdump -i eth0 -nn host <node-b-ip> -c 20

# 同时在 Pod B 所在节点 (Node B) 上抓包:
# (Terminal 4) 物理网卡入口
tcpdump -i eth0 -nn host <node-a-ip> -c 20

# (Terminal 5) Pod B 的 veth pair
tcpdump -i <veth-b> -nn host <pod-a-ip> -c 20

# ========== 3. 分析抓包结果 ==========
# 对比各跳点的包数量和内容:
# - veth-a 有出包但 flannel.1/tunl0 没有 → CNI 路由问题
# - flannel.1 有出包但 eth0 没有 → 封装或路由问题
# - Node A eth0 有出包但 Node B eth0 没有 → 底层网络/安全组/防火墙问题
# - Node B eth0 有入包但 veth-b 没有 → 解封装或 CNI 转发问题
```
### 2.5 Pod-to-Node 排查

Pod 访问自身所在节点或其他节点的 IP。

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `sysctl -w`：实时修改内核参数，全局生效
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# ========== 1. Pod 访问自身所在节点 ==========
NODE_IP=$(kubectl get pod <pod-name> -n <namespace> -o jsonpath='{.status.hostIP}')
kubectl exec -it <pod-name> -n <namespace> -- ping -c 3 $NODE_IP

# 如果不通，检查 rp_filter
# Pod 出包: eth0 → veth → host
# Node 回包: 直接从 eth0 发（非 veth），rp_filter=1 会因为回包路径不同而丢弃
ssh <node> "sysctl net.ipv4.conf.all.rp_filter"
# 若为 1，改为 0 或 2:
ssh <node> "sysctl -w net.ipv4.conf.all.rp_filter=0"
ssh <node> "sysctl -w net.ipv4.conf.cali+.rp_filter=0"

# ========== 2. Pod 访问其他节点 ==========
kubectl exec -it <pod-name> -n <namespace> -- ping -c 3 <other-node-ip>

# 如果不通:
# 1) 检查 Pod 默认网关是否指向宿主机
kubectl exec -it <pod-name> -n <namespace> -- ip route show
# 应有: default via 169.254.1.1 dev eth0 (Calico) 或 default via 10.244.x.1 dev eth0 (Flannel)

# 2) 在宿主机检查 FORWARD 链是否允许 Pod 流量
iptables -t filter -L FORWARD -n -v | head -20

# 3) 检查 ip_forward 是否开启
sysctl net.ipv4.ip_forward
```
### 2.6 Node-to-Node 排查

节点间通信问题直接影响所有跨节点的 Pod 通信。

```bash
# ========== 1. L3 连通性 ==========
ping -c 5 -W 2 <other-node-ip>
traceroute -n <other-node-ip>
mtr -c 10 -r <other-node-ip>

# ========== 2. L2 ARP 解析 ==========
ip neigh show | grep <other-node-ip>
# 若状态为 FAILED → ARP 解析失败，可能是:
# - VLAN 不同
# - 安全组阻断 ARP
# - ARP 表满 (大集群)

# 检查 ARP 表容量
ip neigh show | wc -l
sysctl net.ipv4.neigh.default.gc_thresh3

# ========== 3. CNI 隧道/Overlay 端口 ==========
# 检查 VXLAN 端口 (4789/8472)
nc -uzv <other-node-ip> 4789

# 检查 BGP 端口 (179)
nc -zv <other-node-ip> 179

# 检查 IPIP (协议号 4, 需用 tcpdump)
tcpdump -i eth0 -nn proto 4 -c 5

# ========== 4. 物理/云平台层 ==========
# 网卡状态
ethtool eth0 | grep -E "Speed|Duplex|Link detected"
ethtool -S eth0 | grep -E "error|drop"

# 云平台安全组须放行:
# VXLAN: UDP 4789 | Cilium: UDP 8472 | IPIP: 协议4 | BGP: TCP 179
```

### 2.7 Pod-to-External 排查

Pod 访问集群外部网络的排查。

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# ========== 1. 测试外网连通性 ==========
kubectl exec -it <pod-name> -n <namespace> -- ping -c 3 8.8.8.8
kubectl exec -it <pod-name> -n <namespace> -- wget -qO- --timeout=5 http://httpbin.org/ip

# ========== 2. 检查 SNAT/Masquerade 配置 ==========
# Pod 访问外网需要 SNAT
iptables -t nat -L POSTROUTING -n -v | grep -i masq

# Calico natOutgoing 配置
calicoctl get ippool -o yaml | grep natOutgoing

# ========== 3. 检查默认路由 ==========
# 在宿主机上确认默认路由
ip route show default

# 在 Pod 内确认默认路由
kubectl exec -it <pod-name> -- ip route show default

# ========== 4. DNS 外网解析 ==========
kubectl exec -it <pod-name> -- nslookup google.com
# 若 DNS 失败，检查 CoreDNS 上游 forward 配置
kubectl get configmap coredns -n kube-system -o yaml | grep -A3 forward
```
---

<!-- chunk: 3. Pod间通信问题排查 (Inter-Pod Communication Issues) -->
## 3. Pod间通信问题排查 (Inter-Pod Communication Issues)

### 3.1 同Namespace通信测试

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# ========== 1. 基础连通性测试 ==========
# 使用 netshoot 工具箱
kubectl run netshoot --image=nicolaka/netshoot -n <namespace> -it --rm -- sh

# 在 netshoot 内:
ping -c 3 <target-pod-ip>
curl -s -o /dev/null -w "%{http_code} %{time_total}s" http://<target-pod-ip>:<port>
traceroute -n <target-pod-ip>

# ========== 2. Service访问测试 ==========
kubectl exec -n <namespace> <pod-name> -- wget -qO- http://<service-name>.<namespace>.svc.cluster.local

# ========== 3. DNS解析测试 ==========
kubectl exec -n <namespace> <pod-name> -- nslookup <service-name>.<namespace>.svc.cluster.local
```
### 3.2 跨Namespace通信测试

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 测试跨Namespace Service访问
kubectl exec -n <ns-a> <pod-name> -- wget -qO- http://<service-name>.<ns-b>.svc.cluster.local

# 验证网络策略是否阻断跨 Namespace 流量
kubectl get networkpolicy --all-namespaces -o wide
```
---

<!-- chunk: 4. Service网络问题排查 (Service Network Issues) -->
## 4. Service网络问题排查 (Service Network Issues)

### 4.1 ClusterIP服务问题

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# ========== 1. Service配置检查 ==========
kubectl describe service <service-name> -n <namespace>

# 检查Endpoints（关键：确认有后端 Pod）
kubectl get endpoints <service-name> -n <namespace>
# 若 Endpoints 为空 → Selector 不匹配或无 Ready Pod

# 验证Selector匹配
SVC_SELECTOR=$(kubectl get service <service-name> -n <namespace> -o jsonpath='{.spec.selector}')
echo "Service selector: $SVC_SELECTOR"
kubectl get pods -n <namespace> -l <key>=<value> -o wide

# ========== 2. iptables/IPVS规则检查 ==========
# iptables 模式:
iptables -t nat -L KUBE-SERVICES -n -v | grep <service-cluster-ip>
# 找到对应的 KUBE-SVC-XXXX 链
iptables -t nat -L KUBE-SVC-XXXX -n -v
# 检查后端 KUBE-SEP-XXXX
iptables -t nat -L KUBE-SEP-XXXX -n -v

# IPVS 模式:
ipvsadm -Ln | grep -A5 <service-cluster-ip>

# ========== 3. kube-proxy状态检查 ==========
kubectl get pods -n kube-system -l k8s-app=kube-proxy
kubectl logs -n kube-system -l k8s-app=kube-proxy --tail=50 | grep -i "error|warn"
```
### 4.2 NodePort / LoadBalancer 问题

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# NodePort 检查
iptables -t nat -L KUBE-NODEPORTS -n -v | grep <node-port>

# 从外部测试 NodePort
curl -s -o /dev/null -w "%{http_code}" http://<node-ip>:<node-port>

# 检查 externalTrafficPolicy
kubectl get svc <service-name> -n <namespace> -o jsonpath='{.spec.externalTrafficPolicy}'
# Local: 只转发到本节点 Pod（源 IP 保留，但可能返回空）
# Cluster: 可转发到任意节点 Pod（默认）
```
---

<!-- chunk: 5. DNS解析问题排查 (DNS Resolution Issues) -->
## 5. DNS解析问题排查 (DNS Resolution Issues)

### 5.1 CoreDNS状态检查

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 检查CoreDNS Pod状态
kubectl get pods -n kube-system -l k8s-app=kube-dns -o wide

# 查看CoreDNS配置
kubectl get configmap -n kube-system coredns -o yaml

# 检查CoreDNS日志
kubectl logs -n kube-system -l k8s-app=kube-dns --tail=100

# 测试集群内部域名解析
kubectl exec -n <namespace> <pod-name> -- nslookup kubernetes.default.svc.cluster.local

# 测试外部域名解析
kubectl exec -n <namespace> <pod-name> -- nslookup google.com
```
### 5.2 DNS性能与常见问题

| 问题 | 症状 | 排查 | 解决 |
|------|------|------|------|
| CoreDNS CrashLoop | DNS 完全不可用 | `kubectl logs -n kube-system -l k8s-app=kube-dns` | 调整资源 limits，修复配置错误 |
| 解析超时 | 5s 延迟后返回 | 检查 /etc/resolv.conf 中 ndots 配置 | 设置 ndots:2 或使用 FQDN |
| 上游 DNS 不通 | 外部域名解析失败 | CoreDNS forward 配置，安全组 UDP 53 | 确认上游 DNS 可达 |
| conntrack 竞争 | DNS 间歇性失败 (SERVFAIL) | `conntrack -S` 检查 insert_failed | 升级到 Cilium eBPF 或调大 conntrack |

---

<!-- chunk: 6. 网络策略影响排查 (Network Policy Impact) -->
## 6. 网络策略影响排查 (Network Policy Impact)

### 6.1 网络策略验证

> ⚠️ **🔴 灾难性操作** — 含不可逆命令，执行前必须满足变更窗口+双人复核+事前备份+回滚方案
> - `kubectl delete --all`：批量删除某类全部资源，波及面巨大

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
# 查看所有网络策略
kubectl get networkpolicy --all-namespaces -o wide

# 检查特定 Pod 受影响的策略
kubectl get networkpolicy -n <namespace> -o yaml

# 临时测试: 删除所有策略看是否恢复通信
# ⚠️ 仅在测试环境执行
kubectl delete networkpolicy --all -n <namespace>  # ⚠️ 批量删除，波及面大
```
### 6.2 策略调试工具

```bash
# Calico 策略调试
calicoctl get networkpolicy --all-namespaces -o wide
calicoctl get workloadendpoint -n <namespace>

# Cilium 策略调试（推荐）
cilium monitor --type drop         # 查看被策略丢弃的流量
cilium monitor --type policy-verdict  # 查看策略判定
cilium connectivity test           # 自动化连通性测试
hubble observe --verdict DROPPED   # Hubble 流量审计
```

---

<!-- chunk: 7. 多跳抓包与 iptables TRACE (Multi-Hop Capture & iptables TRACE) -->
## 7. 多跳抓包与 iptables TRACE (Multi-Hop Capture & iptables TRACE)

### 7.1 多节点并行抓包方案

当跨节点通信失败时，需要在数据路径的每一跳同时抓包，通过对比确认丢包位置。

```bash
# ========== 在源节点上抓包 ==========
# 跳 1: Pod veth pair
tcpdump -i <src-veth> -nn host <dst-pod-ip> -w /tmp/src-veth.pcap &

# 跳 2: 隧道/出口网卡
tcpdump -i flannel.1 -nn host <dst-pod-ip> -w /tmp/src-tunnel.pcap &  # VXLAN
# 或
tcpdump -i eth0 -nn host <dst-node-ip> and udp port 4789 -w /tmp/src-eth0.pcap &  # 物理口

# ========== 在目的节点上抓包 ==========
# 跳 3: 物理口入口
tcpdump -i eth0 -nn host <src-node-ip> and udp port 4789 -w /tmp/dst-eth0.pcap &

# 跳 4: Pod veth pair
tcpdump -i <dst-veth> -nn host <src-pod-ip> -w /tmp/dst-veth.pcap &

# 触发流量后停止抓包, 然后对比各 pcap 文件中的包数量
```

### 7.2 iptables TRACE 详细排查

```bash
# 启用 TRACE
modprobe nf_log_ipv4
iptables -t raw -A PREROUTING -s <src-ip> -d <dst-ip> -j TRACE
iptables -t raw -A OUTPUT -s <src-ip> -d <dst-ip> -j TRACE

# 查看 TRACE 输出
dmesg -w | grep TRACE

# 分析 TRACE 输出:
# 每行格式: TRACE: <table>:<chain>:<rule|policy>:<rule-number> ...
# 例: TRACE: filter:FORWARD:rule:3 IN=cali1234 OUT=cali5678 ...
# → 表示数据包在 filter 表的 FORWARD 链第 3 条规则匹配
# 如果 TRACE 在某条 DROP/REJECT 规则后停止，即为丢包位置

# ⚠️ 调试完成后必须清理
iptables -t raw -D PREROUTING -s <src-ip> -d <dst-ip> -j TRACE
iptables -t raw -D OUTPUT -s <src-ip> -d <dst-ip> -j TRACE
```

---

<!-- chunk: 8. 关键内核参数与 conntrack (Kernel Parameters & Conntrack) -->
## 8. 关键内核参数与 conntrack (Kernel Parameters & Conntrack)

### 8.1 关键内核网络参数

| 参数 | K8s 推荐值 | 错误配置影响 |
|------|-----------|-------------|
| `net.ipv4.ip_forward` | **1** | Pod 无法跨节点通信 |
| `net.bridge.bridge-nf-call-iptables` | **1** | Service ClusterIP 不通 |
| `net.ipv4.conf.all.rp_filter` | **0** 或 **2** | Pod-to-Node 回包被丢弃 |
| `net.netfilter.nf_conntrack_max` | **262144+** | conntrack 表满，随机丢包 |
| `net.ipv4.neigh.default.gc_thresh3` | **8192** | 大集群 ARP 解析失败 |

```bash
# 一键检查
for p in net.ipv4.ip_forward net.bridge.bridge-nf-call-iptables \
    net.ipv4.conf.all.rp_filter net.netfilter.nf_conntrack_max \
    net.netfilter.nf_conntrack_count net.ipv4.neigh.default.gc_thresh3; do
    echo "$p = $(sysctl -n $p 2>/dev/null || echo N/A)"
done
```

### 8.2 conntrack 诊断

```bash
# 使用率
CT_COUNT=$(sysctl -n net.netfilter.nf_conntrack_count)
CT_MAX=$(sysctl -n net.netfilter.nf_conntrack_max)
echo "conntrack: $CT_COUNT / $CT_MAX ($((CT_COUNT*100/CT_MAX))%)"

# 统计信息
conntrack -S
# 关注: insert_failed > 0 → 表满丢包

# 查看特定连接
conntrack -L -s <pod-ip> 2>/dev/null | head -10
conntrack -L -d <service-cluster-ip> 2>/dev/null | head -10

# 内核报错
dmesg | grep "nf_conntrack: table full"
```

---

<!-- chunk: 9. 生产案例 (Production Case Studies) -->
## 9. 生产案例 (Production Case Studies)

### 案例 1: 跨节点 Pod 通信间歇性超时

**现象**: 微服务 A 调用微服务 B 偶尔超时 (5-10s)，重试后成功

**排查过程**:
1. `ping` 跨节点 Pod 偶尔丢包，延迟波动大
2. `dmesg` 发现 `nf_conntrack: table full, dropping packet`
3. `conntrack -S` 显示 `insert_failed: 23456`
4. `sysctl net.netfilter.nf_conntrack_max` = 65536（默认值）

**根因**: 高并发微服务导致 conntrack 表满，新连接被随机丢弃

**修复**:

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `sysctl -w`：实时修改内核参数，全局生效

```bash
sysctl -w net.netfilter.nf_conntrack_max=262144
sysctl -w net.netfilter.nf_conntrack_buckets=65536
# 持久化到 /etc/sysctl.d/99-conntrack.conf
```

### 案例 2: Pod 无法访问自身所在节点 IP

**现象**: Pod 内 `ping <host-ip>` 超时，但 `ping <other-pod-ip>` 正常

**排查过程**:
1. 在节点上 `tcpdump -i <veth>` 看到 Pod 出包
2. 但 `tcpdump -i eth0` 没有回包
3. 检查 `sysctl net.ipv4.conf.all.rp_filter` = 1
4. Pod 请求从 veth 进入宿主机，宿主机从 eth0 回包，rp_filter 认为回包路径不对称

**根因**: rp_filter (反向路径过滤) 丢弃了非对称路径的回包

**修复**:

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `sysctl -w`：实时修改内核参数，全局生效

```bash
sysctl -w net.ipv4.conf.all.rp_filter=0
sysctl -w net.ipv4.conf.default.rp_filter=0
# 对 Calico veth 接口:
for i in /proc/sys/net/ipv4/conf/cali*/rp_filter; do echo 0 > $i; done
```

### 案例 3: 大集群 (500+ 节点) 新节点加入后 Pod 网络不通

**现象**: 新加入集群的节点上 Pod 无法与老节点 Pod 通信

**排查过程**:
1. `ip neigh show` 发现 ARP 表中很多 FAILED 条目
2. `dmesg` 显示 `neighbour: arp_cache: neighbor table overflow!`
3. `sysctl net.ipv4.neigh.default.gc_thresh3` = 1024（默认）
4. 500+ 节点集群 ARP 条目远超 1024

**根因**: 内核 ARP 表容量不足，无法缓存所有节点的 MAC 地址

**修复**:

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `sysctl -w`：实时修改内核参数，全局生效

```bash
sysctl -w net.ipv4.neigh.default.gc_thresh1=1024
sysctl -w net.ipv4.neigh.default.gc_thresh2=4096
sysctl -w net.ipv4.neigh.default.gc_thresh3=8192
```

---

<!-- chunk: 10. 监控告警与健康检查 (Monitoring & Health Check) -->
## 10. 监控告警与健康检查 (Monitoring & Health Check)

### 10.1 网络连通性告警

```yaml
# Prometheus 告警规则
groups:
- name: network-connectivity.rules
  rules:
  - alert: PodNetworkUnreachable
    expr: kube_pod_status_ready{condition="false"} == 1
    for: 2m
    labels:
      severity: critical
    annotations:
      summary: "Pod 网络不可达 (namespace {{ $labels.namespace }})"

  - alert: ConntrackTableNearFull
    expr: node_nf_conntrack_entries / node_nf_conntrack_entries_limit > 0.7
    for: 5m
    labels:
      severity: warning
    annotations:
      summary: "conntrack 表使用率超过 70%"

  - alert: CoreDNSDown
    expr: up{job="coredns"} == 0
    for: 1m
    labels:
      severity: critical
    annotations:
      summary: "CoreDNS 不可用"
```

### 10.2 全链路网络健康检查脚本

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
#!/bin/bash
# network-connectivity-full-check.sh

echo "=== 网络连通性全链路检查 ==="
echo "时间: $(date)"

# 1. CNI 状态
echo -e "\n--- 1. CNI 组件 ---"
kubectl get pods -n kube-system | grep -E "(calico|cilium|flannel)" | awk '{printf "  %-45s %s\n", $1, $3}'

# 2. 节点间连通性
echo -e "\n--- 2. 节点间 ICMP ---"
for ip in $(kubectl get nodes -o jsonpath='{.items[*].status.addresses[?(@.type=="InternalIP")].address}'); do
    result=$(timeout 3 ping -c 1 -W 2 $ip 2>/dev/null && echo "OK" || echo "FAIL")
    printf "  %-16s %s\n" "$ip" "$result"
done

# 3. 内核参数
echo -e "\n--- 3. 关键内核参数 ---"
for p in net.ipv4.ip_forward net.bridge.bridge-nf-call-iptables net.ipv4.conf.all.rp_filter; do
    printf "  %-45s %s\n" "$p" "$(sysctl -n $p 2>/dev/null || echo N/A)"
done

# 4. conntrack
echo -e "\n--- 4. conntrack ---"
CT_C=$(sysctl -n net.netfilter.nf_conntrack_count 2>/dev/null || echo 0)
CT_M=$(sysctl -n net.netfilter.nf_conntrack_max 2>/dev/null || echo 1)
echo "  使用率: $CT_C / $CT_M ($((CT_C*100/CT_M))%)"

# 5. CoreDNS
echo -e "\n--- 5. CoreDNS ---"
kubectl get pods -n kube-system -l k8s-app=kube-dns --no-headers | awk '{printf "  %-45s %s\n", $1, $3}'

echo -e "\n=== 检查完成 ==="
```
---

<!-- chunk: Obsidian 相关文档 -->
## Obsidian 相关文档

- domain-10-troubleshooting-diagnostics KUDIG Database — Global MOC
- [[domain-10-troubleshooting-diagnostics/README.md|Domain-12 故障排查 (Troubleshooting)]]
- Domain-12 故障排查 — 开源项目索引
- [[domain-10-troubleshooting-diagnostics/00-core-troubleshooting/01-control-plane-apiserver-troubleshooting.md|API Server 故障排查]]
- [[domain-10-troubleshooting-diagnostics/00-core-troubleshooting/02-control-plane-etcd-troubleshooting.md|etcd 故障排查]]
- [[domain-10-troubleshooting-diagnostics/00-core-troubleshooting/03-networking-cni-troubleshooting.md|CNI 网络插件故障排查]]
- [[domain-10-troubleshooting-diagnostics/00-core-troubleshooting/04-storage-csi-troubleshooting.md|CSI 存储驱动故障排查]]
- [[domain-10-troubleshooting-diagnostics/00-core-troubleshooting/05-pod-pending-diagnosis.md|Pod Pending 状态深度诊断]]
- [[domain-10-troubleshooting-diagnostics/00-core-troubleshooting/06-node-notready-diagnosis.md|Node NotReady 状态深度诊断]]
- [[domain-10-troubleshooting-diagnostics/00-core-troubleshooting/07-oom-memory-diagnosis.md|OOM 和内存问题诊断]]
- [[domain-10-troubleshooting-diagnostics/00-core-troubleshooting/08-pod-comprehensive-troubleshooting.md|Pod 全面故障排查]]
- [[domain-10-troubleshooting-diagnostics/01-resource-troubleshooting/09-node-comprehensive-troubleshooting.md|Node 全面故障排查]]
- [[domain-10-troubleshooting-diagnostics/topic-fta/list/apiserver-fta.md|API Server 异常故障树分析]]
- [[domain-10-troubleshooting-diagnostics/topic-fta/list/backup-restore-fta.md|备份/恢复异常故障树分析]]
- [[domain-10-troubleshooting-diagnostics/topic-fta/list/calico-fta.md|calico FTA 树：Calico CNI 故障诊断]]

## See Also

- [[domain-10-troubleshooting-diagnostics/01-resource-troubleshooting/23-namespace-troubleshooting.md|23-namespace-troubleshooting]]
- [[domain-10-troubleshooting-diagnostics/01-resource-troubleshooting/24-quota-limitrange-troubleshooting.md|24-quota-limitrange-troubleshooting]]
- [[domain-10-troubleshooting-diagnostics/02-infrastructure-troubleshooting/26-dns-troubleshooting.md|26-dns-troubleshooting]]
- [[domain-10-troubleshooting-diagnostics/02-infrastructure-troubleshooting/27-image-registry-troubleshooting.md|27-image-registry-troubleshooting]]

## Related

- [[domain-19-landscape-references/topic-index/network-index.md|Network 网络知识图谱索引]]
- [[domain-19-landscape-references/topic-index/dns-index.md|DNS 知识图谱索引]]

```

<!-- risk-assessed -->
