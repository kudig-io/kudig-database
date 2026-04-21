# kube-proxy 与 Service 负载均衡

## 源码路径

`cmd/kubeadm/app/phases/addons/proxy/`
`pkg/proxy/`

---

## kube-proxy 部署

```go
// kubeadm init 完成后:
// 1. 创建 kube-proxy ServiceAccount
// 2. 创建 kube-proxy ConfigMap (mode 配置)
// 3. 创建 kube-proxy DaemonSet (每个节点一个)
```

```bash
# 验证
kubectl get ds -n kube-system -l k8s-app=kube-proxy

# 查看 kube-proxy 配置
kubectl get configmap kube-proxy -n kube-system -o yaml
```

---

## kube-proxy ConfigMap

```yaml
# /var/lib/kubelet/config.yaml 中 kube-proxy 配置
apiVersion: kubeproxy.config.k8s.io/v1alpha1
kind: KubeProxyConfiguration
mode: "iptables"              # 或 "ipvs", "nftables"
iptables:
  masqueradeAll: false
  masqueradeBit: 14
  minSyncPeriod: 0s
  syncPeriod: 30s
ipvs:
  masqueradeAll: true
  minSyncPeriod: 5s
  syncPeriod: 30s
  scheduler: "rr"             # 负载均衡算法
  excludeCIDRs: []
conntrack:
  maxPerCore: 32768
  min: 131072
```

---

## 三种模式对比

| 模式 | 原理 | 性能 | 依赖 |
|------|------|------|------|
| **iptables** | 规则链匹配 | 中 (O(n)) | iptables |
| **ipvs** | 基于哈希表 | 高 (O(1)) | ipvs kernel module |
| **nftables** | nftables 规则 | 高 | nftables |

---

## iptables 模式

```
Service (ClusterIP: 10.96.0.100)
    ↓
KUBE-SERVICES chain (iptables)
    ↓
KUBE-SVC-XXXX chain (概率匹配)
    ↓
KUBE-SEP-XXXX chain (DNAT 到 PodIP)
    ↓
PodIP:Port
```

```bash
# iptables 规则示例
iptables -t nat -L -n | grep KUBE-SVC

# KUBE-SVC-XXXX 链包含:
# - 概率转发到各个 endpoint
# - 首次访问时创建 connection track
# - 后续直接 DNAT
```

**特点**:
- 规则数量随 Service 线性增长
- O(n) 查找复杂度
- 大规模集群性能下降

---

## ipvs 模式

```
Service (ClusterIP: 10.96.0.100)
    ↓
IPVS virtual server (哈希表)
    ↓
real server (PodIP:Port)
```

```bash
# 查看 IPVS 规则
ipvsadm -L -n

# 输出:
# IP Virtual Server version 1.2.1
# Prot LocalAddress:Port Scheduler Flags
#   -> RemoteAddress:Port           Forward  Active  InActice
# TCP  10.96.0.100:80 rr
#   -> 10.244.0.10:80        Masq    1       0
#   -> 10.244.1.10:80        Masq    0       0
```

**负载均衡算法**:

| 算法 | 说明 |
|------|------|
| `rr` | 轮询 (默认) |
| `wrr` | 加权轮询 |
| `lc` | 最少连接 |
| `wlc` | 加权最少连接 |
| `sh` | 源哈希 |
| `dh` | 目标哈希 |

---

## nftables 模式 (K8s 1.28+)

```bash
# 启用 nftables 模式
# kubeadm 不直接支持，在 kubelet config 中配置:
apiVersion: kubelet.config.k8s.io/v1beta1
kind: KubeletConfiguration
mode: nftables  # (1.28+ 支持)
```

**优势**: 比 iptables 更高效的规则更新，更适合大规模集群。

---

## Service 类型与实现

```
ClusterIP (默认)
    ↓ kube-proxy iptables/ipvs
    → PodIP:Port

NodePort (30000-32767)
    ↓ 节点监听 NodePort
    → kube-proxy 转发
    → PodIP:Port

LoadBalancer (云厂商)
    → Cloud LB → NodePort → kube-proxy → PodIP:Port

ExternalName (cnames)
    → 外部服务 cname
```

---

## headless Service

```yaml
# 无 ClusterIP 的 Service
apiVersion: v1
kind: Service
metadata:
  name: headless
spec:
  clusterIP: None  # 关键: 无 ClusterIP
  selector:
    app: nginx
  ports:
  - port: 80
```

```
DNS 查询:
# DNS 返回所有 Pod IP (A 记录)
# 而非单个 ClusterIP
nginx.default.svc.cluster.local → 10.244.0.10, 10.244.1.10
```

---

## External Traffic Policy

```yaml
# 只允许本地 Endpoint
spec:
  externalTrafficPolicy: Local

# 效果:
# - 保留源 IP (不 SNAT)
# - 只转发到本地 Pod
# - 如果本地无 Pod，丢弃
```

| Policy | 源 IP | 跨节点转发 |
|--------|-------|----------|
| Cluster (默认) | SNAT 后 IP | 允许 |
| Local | 原始 IP | 不允许 |

---

## 连接跟踪 (conntrack)

kube-proxy 依赖 conntrack 维护连接状态:

```bash
# 查看 conntrack 表
cat /proc/net/nf_conntrack | grep CLUSTERIP

# conntrack 参数
# /etc/sysctl.conf
net.netfilter.nf_conntrack_max = 131072
net.netfilter.nf_conntrack_tcp_timeout_established = 86400
```

**问题**: 高并发下 conntrack 表满会导致丢包:

```bash
# 查看 conntrack 使用
cat /proc/sys/net/netfilter/nf_conntrack_count

# 到达上限时:
# dmesg | tail
# [UFW BLOCK] IN=eth0 OUT= MAC=... SRC=... DST=...
```

---

## 常见问题

| 问题 | 原因 | 解决 |
|------|------|------|
| Service 无法访问 | kube-proxy 未运行 | 检查 kube-proxy Pod |
| 跨节点 Service 慢 | iptables 规则过多 | 切换 ipvs 模式 |
| 源 IP 丢失 | ExternalTrafficPolicy=Cluster | 改为 Local |
| NodePort 无法访问 | 防火墙阻止 30000-32767 | 开放端口 |
| 连接超时 | conntrack 表满 | 增加 nf_conntrack_max |
