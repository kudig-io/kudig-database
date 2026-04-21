# 节点网络：CN 配置

## 源码路径

`pkg/kubelet/network/`
`plugins/meta/`

---

## CNI 架构

```
Pod 网络创建流程:
  ┌─────────────────────────────────────────────────────────────┐
  │  1. kubelet 通过 CRI 调用 CNI ADD                           │
  │     - 创建 netns (网络命名空间)                              │
  │     - 创建 veth pair                                        │
  │     - 配置 IP 地址                                          │
  │     - 调用 CNI IPAM 分配 IP                                 │
  │     - 配置网络 (网桥/路由)                                   │
  └─────────────────────────────────────────────────────────────┘
```

---

## Pod 网络命名空间

```bash
# 查看 Pod netns
ip netns list

# 查看网络接口
ip link

# 在容器内查看网络
nsenter -t <pid> -n ip addr

# 或使用 crictl
crictl exec -i <container-id> ip addr
```

---

## veth pair

```
节点网络:
  ┌─────────────────────────────────────────────────────────────┐
  │  eth0 (节点物理网卡)                                         │
  │      ↓                                                        │
  │  cni0 (网桥)                                                 │
  │      ↓                                                        │
  │  veth1 ──────────── eth0 (Pod 网络命名空间)                    │
  │                        ↓                                      │
  │                   Pod: 10.244.0.10/24                        │
  └─────────────────────────────────────────────────────────────┘
```

---

## CNI 插件

```bash
# CNI 配置文件
ls /etc/cni/net.d/

# 常用 CNI 插件:
# - bridge
# - host-local
# - ptp
# - calico
# - cilium
# - flannel

# CNI 二进制文件
ls /opt/cni/bin/
```

---

## Pod IP 分配

```bash
# CNI IPAM (IP Address Management)
# 分配给每个 Pod 唯一 IP

# 查看已分配的 Pod IP
kubectl get pods -o wide

# 查看节点上 CNI 分配的 IP
ip addr show
```

---

## 常见问题

| 问题 | 原因 | 解决 |
|------|------|------|
| Pod 无法跨节点通信 | CNI 配置错误 | 检查 CNI 配置 |
| IP 冲突 | CNI IPAM 冲突 | 重启 CNI/IPAM |
| DNS 不通 | CoreDNS 问题 | 检查 CoreDNS |
