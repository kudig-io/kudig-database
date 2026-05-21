---
title: Flannel WireGuard 加密后端配置
description: Flannel WireGuard 后端配置指南，涵盖 WireGuard 加密隧道原理、配置步骤、性能对比和故障排查
category: networking
tags:
- k8s
- networking
- flannel
- wireguard
- encryption
- cni
- docker
- opa
- daemonset
- operator
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 网络工程师
- 安全工程师
estimated_read_time: 5min
intent_queries:
- Flannel WireGuard 配置
- WireGuard 加密后端
- Flannel 加密隧道
trigger_keywords:
- Flannel
- WireGuard
- 加密
- backend
prerequisites:
- kubectl-basics
- networking-basics
- policy-basics
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
  path: ../domain-01-cluster-fundamentals/
  label: '相关知识域: domain-01-cluster-fundamentals'
- type: domain
  path: ../domain-03-networking-traffic/
  label: '相关知识域: domain-03-networking-traffic'
- type: domain
  path: ../domain-06-observability/
  label: '相关知识域: domain-06-observability'
- type: fta
  path: ../domain-10-troubleshooting-diagnostics/topic-fta/list/flannel-fta.md
  label: '故障树: flannel'
- type: cheatsheet
  path: ../domain-17-system-foundation/topic-cheat-sheet/networking.md
  label: '速查卡: networking'
---

# Flannel WireGuard 加密后端配置

> **适用版本**: Kubernetes v1.25+ | Flannel v0.20+ | **最后更新**: 2026-05

---

<!-- chunk: 1. WireGuard 后端概述 -->
## 1. WireGuard 后端概述

WireGuard 后端为 Flannel 提供加密的 Pod 间通信隧道，相比 VXLAN 具有：

| 特性 | VXLAN | WireGuard |
|:-----|:-----:|:---------:|
| 封装开销 | 50 bytes | 80 bytes |
| 加密 | 无 | WireGuard 原生加密 |
| 性能 | 中 | 高（硬件加速） |
| MTU 推荐 | 1450 | 1420 |
| 内核要求 | 3.7+ | 5.6+ 或 backport |

---

<!-- chunk: 2. 架构原理 -->
## 2. 架构原理

```
Pod A (10.244.1.10) ──▶ Pod B (10.244.2.20)

Node 1 (192.168.1.10)              Node 2 (192.168.1.20)
┌────────────────────┐             ┌────────────────────┐
│  Pod A ──▶ cni0   │             │  cni0 ──▶ Pod B   │
│       ──▶ wg0     │             │  wg0 ◀──          │
│  (加密) ──▶ eth0 ─┼─────────────┼──▶ eth0 (解密)    │
│                    │  WireGuard  │                   │
└────────────────────┘  UDP:51820  └────────────────────┘
```

---

<!-- chunk: 3. 前置要求 -->
## 3. 前置要求

### 3.1 内核支持

```bash
# 检查 WireGuard 模块是否可用
modprobe wireguard
lsmod | grep wireguard

# 或检查内核版本
uname -r  # 需要 5.6+ 或已安装 wireguard backport
```

### 3.2 安装 wireguard-tools

```bash
# Debian/Ubuntu
apt-get install wireguard

# CentOS/RHEL
yum install epel-release
yum install wireguard-tools

# macOS (开发用)
brew install wireguard-tools
```

---

<!-- chunk: 4. 配置步骤 -->
## 4. 配置步骤

### 4.1 生成密钥对

```bash
# 在每个节点上执行
wg genkey | tee privatekey | wg pubkey > publickey

# 查看生成的密钥
cat privatekey
cat publickey
```

### 4.2 配置 Flannel ConfigMap

```yaml
apiVersion: v1
kind: ConfigMap
metadata:
  name: kube-flannel-cfg
  namespace: kube-flannel
data:
  net-conf.json: |
    {
      "Network": "10.244.0.0/16",
      "Backend": {
        "Type": "wireguard",
        "PrivateKey": "<node1-private-key>",
        "ListenPort": 51820,
        "FwMark": 0x20000
      }
    }
```

### 4.3 节点密钥管理

**方式一：手动配置（仅推荐测试环境）**

```bash
# 在每个节点上设置密钥
kubectl create secret generic flannel-keys \
  --from-literal=private-key=<base64-encoded-private-key> \
  --namespace=kube-flannel
```

**方式二：使用 Kubernetes Secret（生产推荐）**

```yaml
apiVersion: v1
kind: Secret
metadata:
  name: flannel-wireguard-keys
  namespace: kube-flannel
type: Opaque
stringData:
  privatekey: |
    <node-private-key-base64>
---
# 在 DaemonSet 中引用
env:
  - name: FLANNEL_PRIVATE_KEY
    valueFrom:
      secretKeyRef:
        name: flannel-wireguard-keys
        key: privatekey
```

### 4.4 完整 DaemonSet 配置

```yaml
apiVersion: apps/v1
kind: DaemonSet
metadata:
  name: kube-flannel-ds
  namespace: kube-flannel
spec:
  selector:
    matchLabels:
      app: flannel
  template:
    metadata:
      labels:
        app: flannel
    spec:
      serviceAccountName: flannel
      hostNetwork: true
      tolerations:
        - operator: Exists
      containers:
        - name: kube-flannel
          image: docker.io/flannel/flannel:v0.24.2
          command: ["/opt/bin/flanneld"]
          args:
            - --ip-masq
            - --kube-subnet-mgr
            - --iface=eth0
            - --backend=wireguard
          securityContext:
            capabilities:
              add: ["NET_ADMIN", "SYS_MODULE"]
          env:
            - name: FLANNEL_PRIVATE_KEY
              value: "<node-private-key>"
            - name: FLANNEL_WG_PORT
              value: "51820"
          volumeMounts:
            - name: run
              mountPath: /run/flannel
            - name: flannel-cfg
              mountPath: /etc/kube-flannel/
          resources:
            requests:
              cpu: "100m"
              memory: "50Mi"
            limits:
              memory: "100Mi"
      volumes:
        - name: run
          hostPath:
            path: /run/flannel
        - name: flannel-cfg
          configMap:
            name: kube-flannel-cfg
```

---

<!-- chunk: 5. WireGuard 接口验证 -->
## 5. WireGuard 接口验证

### 5.1 检查接口状态

```bash
# 查看 wg0 接口
ip link show wg0

# 查看 WireGuard 详细信息
ip -d link show wg0

# 查看 WireGuard 状态和 Peer
wg show wg0
```

**预期输出示例**：
```
interface: wg0
  public key: <node-public-key>
  private key: (hidden)
  listening port: 51820

peer: <peer-public-key>
  endpoint: 192.168.1.20:51820
  allowed ips: 10.244.2.0/24
  latest handshake: 45 seconds ago
  transfer: 1.23 MiB received, 2.45 MiB sent
```

### 5.2 检查路由表

```bash
# 查看 Flannel 添加的路由
ip route | grep 10.244

# 预期：流量通过 wg0 发送到远端子网
# 10.244.2.0/24 via <peer-node-ip> dev wg0
```

---

<!-- chunk: 6. 性能对比 -->
## 6. 性能对比

### 6.1 吞吐量测试

```bash
# 使用 iperf3 测试（需在两个 Pod 中运行）
kubectl exec -it <pod-a> -- iperf3 -s -D
kubectl exec -it <pod-b> -- iperf3 -c <pod-a-ip> -P 4

# 参考数据（单连接）
# VXLAN: ~2.8 Gbps
# WireGuard: ~3.5 Gbps (受益于加密硬件加速)
```

### 6.2 延迟对比

```bash
# 使用 qperf 测试
kubectl exec -it <pod-a> -- qperf <pod-b-ip> tcp_lat

# 参考数据
# VXLAN: ~0.15 ms
# WireGuard: ~0.12 ms
```

---

<!-- chunk: 7. 故障排查 -->
## 7. 故障排查

### 7.1 常见问题

| 问题 | 原因 | 解决方案 |
|:-----|:-----|:--------|
| wg0 接口不存在 | 内核不支持 WireGuard | 升级内核或安装 backport |
| handshake 超时 | UDP 51820 端口被阻断 | 检查防火墙规则 |
| Peer 连接失败 | 密钥不匹配 | 确认公钥配置正确 |
| 性能下降 | MTU 配置不当 | 设置 MTU=1420 |

### 7.2 排查命令

```bash
# 1. 检查 WireGuard 模块
lsmod | grep wireguard

# 2. 检查 UDP 51820 端口
ss -ulnp | grep 51820

# 3. 检查 wg0 接口
ip link show wg0

# 4. 查看详细错误
journalctl -u flanneld -n 100

# 5. 抓包分析
tcpdump -i eth0 udp port 51820 -nn

# 6. 测试连通性
ping -I wg0 <remote-pod-ip>
```

### 7.3 防火墙规则

```bash
# 放通 WireGuard 端口
iptables -A INPUT -p udp --dport 51820 -j ACCEPT

# 保存规则
iptables-save > /etc/iptables/rules.v4
```

---

<!-- chunk: 8. 与其他后端对比 -->
## 8. 与其他后端对比

| 后端 | 封装开销 | 性能 | 加密 | 适用场景 |
|:-----|:-------:|:----:|:----:|:---------|
| VXLAN | 50 bytes | 中 | 无 | 通用场景 |
| host-gw | 0 | 高 | 无 | 同二层网络 |
| WireGuard | 80 bytes | 高 | WireGuard 原生 | 需要加密 |
| UDP | 较大 | 低 | 无 | 调试/旧内核 |

---

<!-- chunk: 9. 回滚到 VXLAN -->
## 9. 回滚到 VXLAN

如需回滚，修改 ConfigMap：

```bash
kubectl edit configmap -n kube-flannel kube-flannel-cfg

# 修改 net-conf.json
{
  "Network": "10.244.0.0/16",
  "Backend": {
    "Type": "vxlan",
    "DirectRouting": true
  }
}

# 重启 flannel
kubectl rollout restart ds/kube-flannel-ds -n kube-flannel
```

---

<!-- chunk: Obsidian 相关文档 -->
## Obsidian 相关文档

- [[domain-03-networking-traffic/MOC.md|domain-03-networking-traffic MOC]]
- [[domain-03-networking-traffic/README.md|Domain 5: Networking 网络]]
- [[domain-03-networking-traffic/00-network-in-nutshell.md|Kubernetes 网络基础 Network in a Nutshell]]
- [[domain-03-networking-traffic/00-open-source-projects-index.md|Domain-5 网络 — 开源项目索引]]
- [[domain-03-networking-traffic/01-network-architecture-overview-faq.md|FAQ 文档]]
- [[domain-03-networking-traffic/01-network-architecture-overview.md|网络核心组件]]
- [[domain-03-networking-traffic/02-cni-architecture-fundamentals.md|CNI 架构与核心原理]]
- [[domain-03-networking-traffic/03-cni-plugins-comparison.md|76 - CNI插件深度对比]]
- [[domain-03-networking-traffic/04-flannel-complete-guide.md|142 - Flannel 完整指南 (Flannel Complete Guide)]]
- [[domain-03-networking-traffic/04b-flannel-ipv6-dual-stack.md|Flannel IPv6 Dual Stack 支持]]
- [[domain-03-networking-traffic/04c-flannel-windows-support.md|Flannel Windows 节点支持]]
- [[domain-03-networking-traffic/04d-flannel-multi-cluster.md|Flannel 多集群场景与子网冲突处理]]

## See Also

- [[domain-03-networking-traffic/03-cni-plugins-comparison.md|03-cni-plugins-comparison]]
- [[domain-03-networking-traffic/04-flannel-complete-guide.md|04-flannel-complete-guide]]
- [[domain-03-networking-traffic/04b-flannel-ipv6-dual-stack.md|04b-flannel-ipv6-dual-stack]]
- [[domain-03-networking-traffic/04c-flannel-windows-support.md|04c-flannel-windows-support]]

## Related

- [[domain-19-landscape-references/topic-index/flannel-index|Flannel 知识图谱索引]]
