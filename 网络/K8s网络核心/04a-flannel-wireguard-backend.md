---
title: Flannel WireGuard 加密后端配置
description: Flannel WireGuard 后端配置指南，涵盖 WireGuard 加密隧道原理、配置步骤、性能对比和故障排查
summary: Flannel WireGuard 后端配置指南，涵盖 WireGuard 加密隧道原理、配置步骤、性能对比和故障排查
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
tier: supporting
created: '2026-05-23'
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
  path: ../集群基础/
  label: '相关知识域: 集群基础'
- type: domain
  path: ../网络/
  label: '相关知识域: 网络'
- type: domain
  path: ../可观测性/
  label: '相关知识域: 可观测性'
- type: fta
  path: ../故障诊断/topic-fta/list/flannel-fta.md
  label: '故障树: flannel'
- type: cheatsheet
  path: ../系统基础/topic-cheat-sheet/networking.md
  label: '速查卡: networking'
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Flannel WireGuard 加密后端配置

> **适用版本**: [[Kubernetes|Kubernetes]] v1.25+ | Flannel v0.20+ | **最后更新**: 2026-05

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

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
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

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 使用 iperf3 测试（需在两个 Pod 中运行）
kubectl exec -it <pod-a> -- iperf3 -s -D
kubectl exec -it <pod-b> -- iperf3 -c <pod-a-ip> -P 4

# 参考数据（单连接）
# VXLAN: ~2.8 Gbps
# WireGuard: ~3.5 Gbps (受益于加密硬件加速)
```
### 6.2 延迟对比

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
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

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源
> - `kubectl rollout undo/restart`：触发滚动变更，影响副本

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

- 网络 KUDIG Database — Global MOC
- [[网络/README.md|[[Domain 5: Networking 网络|Domain 5: Networking 网络]]working]] 网络]]
- Kubernetes 网络基础 Network in a Nutshell
- Domain-5 网络 — 开源项目索引
- FAQ 文档
- 网络核心组件
- CNI 架构与核心原理
- 76 - CNI插件深度对比
- 142 - Flannel 完整指南 (Flannel Complete Guide)
- Flannel IPv6 Dual Stack 支持
- Flannel Windows 节点支持
- Flannel 多集群场景与子网冲突处理

## See Also

- 03-cni-plugins-comparison
- 04-flannel-complete-guide
- 04b-flannel-ipv6-dual-stack
- 04c-flannel-windows-support

## Related

- [[生态参考/领域索引/flannel-index.md|Flannel 知识图谱索引]]


<!-- risk-assessed -->
