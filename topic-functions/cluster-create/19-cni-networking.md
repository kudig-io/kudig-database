# CNI 网络插件与集群网络

## 源码路径

`cmd/kubeadm/app/phases/addons/` (CNI/Proxy/DNS)
`pkg/probe/` (健康检查)

---

## CNI 安装顺序

**关键**: kubeadm init 完成后，节点网络不通，必须安装 CNI 插件:

```
kubeadm init 完成
    ↓
CoreDNS Pod 处于 Pending (因为节点无网络)
    ↓
安装 CNI 插件 (Calico/Cilium/Flannel)
    ↓
节点网络连通
    ↓
CoreDNS Pod 变为 Running
    ↓
集群可正常工作
```

---

## CNI 插件对比

| 插件 | 模式 | 数据面 | 性能 | 网络模型 |
|------|------|--------|------|---------|
| Calico |Overlay+BGP | eBPF/Linux Routing | 高 | IP-in-IP/VXLAN |
| Cilium |Overlay+BGP | eBPF | 最高 | VXLAN/Geneve |
| Flannel |Overlay | VXLAN | 中 | VXLAN |
| Weave |Overlay | sleeve/fastdp | 中 | VXLAN |
| kube-router |路由 | eBPF | 高 | BGP |

---

## kubeadm --pod-network-cidr

```bash
# kubeadm init 时指定 Pod CIDR
kubeadm init --pod-network-cidr=10.244.0.0/16

# 对应 CNI 配置:
# Calico: ippool yaml 中指定
# Cilium: CiliumPolicy
# Flannel: net-conf.json 中指定
```

---

## CNI 配置文件

```bash
# kubelet 读取 CNI 配置目录:
# /etc/cni/net.d/ (kubeadm 不写入，由 CNI 插件写入)

ls /etc/cni/net.d/
# 输出:
# 10-calico.conflist   (Calico)
# 99-loopback.conf     (回环设备)

# kubelet config.yaml 中的 CNI 配置:
# cniBinDir: /opt/cni/bin
# cniConfDir: /etc/cni/net.d
# networkPluginMTU: 1500
```

---

## CoreDNS 详解

CoreDNS 替代 kube-dns (K8s 1.10+) 成为默认 DNS 服务:

```bash
# CoreDNS 部署
kubectl get deployment -n kube-system coredns
kubectl get service -n kube-system kube-dns

# Service: kube-dns (ClusterIP: 10.96.0.10)
# 包含两个端口:
# - 53/UDP (DNS 解析)
# - 53/TCP (DNS 解析)
# - 9153/TCP (健康检查)
```

---

## DNS 解析流程

```
Pod 内应用发起 DNS 查询:
1. 应用调用 gethostbyname("nginx.default.svc.cluster.local")
    ↓
2. glibc 将查询发送到 /etc/resolv.conf 中的 nameserver
    nameserver 10.96.0.10
    ↓
3. kubelet 配置 Pod 的 resolv.conf 指向 CoreDNS:
    nameserver 10.96.0.10
    search default.svc.cluster.local svc.cluster.local cluster.local
    options ndots:5
    ↓
4. CoreDNS 接收 UDP 53 端口请求
    ↓
5. CoreDNS 查询 etcd/本地缓存获取 Service Endpoints
    ↓
6. 返回 ClusterIP (10.96.0.x)
    ↓
7. kube-proxy iptables/ipvs 将 ClusterIP DNAT 到具体 PodIP
```

---

## Corefile 解析

```yaml
# CoreDNS Corefile (存在 ConfigMap: kube-system/coredns)
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
        cache 30
        loop
        reload
        loadbalance
    }
```

---

## Pod 网络模型

```
节点 A                              节点 B
┌──────────────────┐               ┌──────────────────┐
│  Pod A           │               │  Pod B           │
│  10.244.0.10     │               │  10.244.1.10     │
│       ↓ eth0      │               │       ↓ eth0      │
│  ┌─────┴─────┐    │               │  ┌─────┴─────┐    │
│  │  veth0    │    │               │  │  veth0    │    │
│  └─────┬─────┘    │               │  └─────┬─────┘    │
│        ↓          │               │        ↓          │
│  ┌─────┴─────┐    │               │  ┌─────┴─────┐    │
│  │  bridge   │    │               │  │  bridge   │    │
│  │  cni0     │    │               │  │  cni0     │    │
│  └─────┬─────┘    │               │  └─────┬─────┘    │
│        ↓          │               │        ↓          │
│  ┌─────┴──────────┴─────────────────────────┴─────┐    │
│  │              VXLAN Tunnel (overlay)           │    │
│  └────────────────────┬──────────────────────────┘    │
└──────────────────────┼────────────────────────────────┘
                       ↓
                 物理网卡 (eth0)
                 节点 IP: 192.168.1.x
```

---

## Service 与 Endpoints

```
Service (ClusterIP: 10.96.0.100)
    ↓ (通过 iptables/ipvs)
    ↓ DNAT
Endpoints (PodIP: 10.244.0.10:80, 10.244.1.10:80)
```

```bash
# 查看 Service
kubectl get svc nginx -o yaml

# 查看 Endpoints
kubectl get endpoints nginx -o yaml

# kube-proxy 规则
# iptables -t nat -L -n | grep KUBE-SERVICES
# ipvsadm -L -n | grep :80
```

---

## NodePort / LoadBalancer / Ingress

```
Ingress/LoadBalancer → Service (ClusterIP) → Endpoints (PodIP)
                        ↓
                  NodePort (30000-32767)
                        ↓
              直接访问 Pod (HostPort)
```

---

## CNI 安装后验证

```bash
# 1. 检查 CNI pods 运行状态
kubectl get pods -n kube-system -l k8s-app=calico-node
kubectl get pods -n kube-system -l k8s-app=cilium

# 2. 检查 CNI 配置
cat /etc/cni/net.d/10-calico.conflist

# 3. 测试节点间连通
kubectl run -it --rm debug --image=busybox -- sh
# 在容器内: wget http://nginx.default.svc.cluster.local

# 4. 检查 CoreDNS
kubectl get pods -n kube-system -l k8s-app=kube-dns
kubectl logs -n kube-system -l k8s-app=kube-dns
```

---

## 常见问题

| 问题 | 原因 | 解决 |
|------|------|------|
| CoreDNS 一直 Pending | CNI 未安装/配置错误 | 安装 CNI |
| Pod 无法跨节点通信 | 防火墙阻止 VXLAN | 开放 4789/8472 端口 |
| Service 无法访问 | kube-proxy iptables 规则错误 | 检查 iptables -t nat -L |
| DNS 解析失败 | CoreDNS 未就绪 | `kubectl rollout restart deployment/coredns -n kube-system` |
| NodePort 无法访问 | 防火墙阻止 NodePort | 开放 30000-32767 端口 |
