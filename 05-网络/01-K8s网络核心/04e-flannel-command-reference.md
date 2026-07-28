---
title: flanneld 启动参数详解
description: flanneld 完整启动参数参考，涵盖网络配置、后端选择、认证、安全等所有命令行选项
summary: flanneld 完整启动参数参考，涵盖网络配置、后端选择、认证、安全等所有命令行选项
category: networking
tags:
- k8s
- networking
- flannel
- flanneld
- command
- reference
- etcd
- apiserver
- daemonset
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
estimated_read_time: 5min
intent_queries:
- flanneld 参数
- flanneld 命令行
- flanneld 配置
trigger_keywords:
- flanneld
- 参数
- 命令行
prerequisites:
- kubectl-basics
- networking-basics
- etcd-basics
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
  path: ../故障诊断/FTA故障树/list/flannel-fta.md
  label: '故障树: flannel'
- type: cheatsheet
  path: ../系统基础/topic-cheat-sheet/networking.md
  label: '速查卡: networking'
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# flanneld 启动参数详解

> **适用版本**: Flannel v0.20+ | **最后更新**: 2026-05

---

<!-- chunk: 1. 概述 -->
## 1. 概述

flanneld 是 Flannel 的核心守护进程，负责子网分配、路由维护和后端配置。

### 1.1 启动方式

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 直接运行
/opt/bin/flanneld [选项]

# 通过 systemd 运行
systemctl start flannel

# Kubernetes DaemonSet 环境
# 参数通过 ConfigMap 或环境变量传入
```
### 1.2 参数来源优先级

1. **命令行参数**（最高优先级）
2. **环境变量**（FLANNEL_ 前缀）
3. **ConfigMap**（net-conf.json）
4. **默认配置**（最低优先级）

---

<!-- chunk: 2. 网络配置参数 -->
## 2. 网络配置参数

### 2.1 基础网络参数

| 参数 | 环境变量 | 默认值 | 说明 |
|:-----|:---------|:------:|:-----|
| `--network` | `FLANNEL_NETWORK` | 10.244.0.0/16 | 整个 Pod 网络 CIDR |
| `--subnet-file` | `FLANNEL_SUBNET_FILE` | /run/flannel/subnet.env | 子网信息文件路径 |
| `--subnet-lease-renew-margin` | `FLANNEL_SUBNET_LEASE_RENEW_MARGIN` | 0 | 子网租约续期边距（小时） |
| `--ip-masq` | `FLANNEL_IP_MASQ` | true | 是否启用 IP 伪装（出站流量） |

**示例**：
```bash
flanneld --network=10.244.0.0/16 --ip-masq=true
```

### 2.2 IPv6 配置

| 参数 | 环境变量 | 说明 |
|:-----|:---------|:-----|
| `--ipv6-network` | `FLANNEL_IPV6_NETWORK` | IPv6 Pod 网络 CIDR |
| `--ipv6-ip-masq` | `FLANNEL_IPV6_IP_MASQ` | IPv6 伪装启用 |

**示例**：
```bash
flanneld --ipv6-network=2001:db8::/64 --ipv6-ip-masq=true
```

---

<!-- chunk: 3. 后端配置参数 -->
## 3. 后端配置参数

### 3.1 后端选择

| 参数 | 环境变量 | 默认值 | 说明 |
|:-----|:---------|:------:|:-----|
| `--backend` | `FLANNEL_BACKEND` | vxlan | 后端类型：vxlan/host-gw/udp/wireguard |

**可用后端**：
- `vxlan` - VXLAN 封装（推荐）
- `host-gw` - 直接路由（需 L2 可达）
- `udp` - 用户态 UDP（已废弃）
- `wireguard` - WireGuard 加密隧道

### 3.2 VXLAN 后端参数

| 参数 | 环境变量 | 默认值 | 说明 |
|:-----|:---------|:------:|:-----|
| `--backend.vxlan.port` | `FLANNEL_VXLAN_PORT` | 4789 | VXLAN UDP 端口 |
| `--backend.vxlan.vni` | `FLANNEL_VXLAN_VNI` | 1 | VXLAN Network Identifier |
| `--backend.vxlan.mtu` | `FLANNEL_VXLAN_MTU` | 自动 | VXLAN MTU |
| `--backend.vxlan.directrouting` | `FLANNEL_VXLAN_DIRECTROUTING` | false | 启用直接路由 |

**示例**：
```bash
flanneld --backend=vxlan --backend.vxlan.port=4789 --backend.vxlan.vni=1
```

### 3.3 host-gw 后端参数

| 参数 | 环境变量 | 说明 |
|:-----|:---------|:-----|
| `--backend.host-gw.interface` | `FLANNEL_HOSTGW_INTERFACES` | 指定物理网卡名称 |

**示例**：
```bash
flanneld --backend=host-gw --backend.host-gw.interface=eth0
```

### 3.4 WireGuard 后端参数

| 参数 | 环境变量 | 默认值 | 说明 |
|:-----|:---------|:------:|:-----|
| `--backend.wireguard.private-key` | `FLANNEL_PRIVATE_KEY` | - | WireGuard 私钥（必需） |
| `--backend.wireguard.listen-port` | `FLANNEL_WG_PORT` | 51820 | WireGuard 监听端口 |
| `--backend.wireguard.fwmark` | `FLANNEL_WG_FWMARK` | 0x20000 | Firewall Mark |

**示例**：
```bash
flanneld --backend=wireguard \
  --backend.wireguard.private-key=/etc/flannel/wg.key \
  --backend.wireguard.listen-port=51820
```

---

<!-- chunk: 4. Kubernetes 集成参数 -->
## 4. Kubernetes 集成参数

### 4.1 API 连接参数

| 参数 | 环境变量 | 默认值 | 说明 |
|:-----|:---------|:------:|:-----|
| `--kube-subnet-mgr` | - | false | 使用 [[17-系统基础/06-知识字典/fundamentals/the-kubernetes-api.md|Kubernetes API]] 管理子网（推荐） |
| `--kube-api-url` | `FLANNEL_KUBE_API_URL` | - | Kubernetes API Server URL |
| `--kubeconfig` | `FLANNEL_KUBECONFIG` | ~/.kube/config | kubeconfig 文件路径 |
| `--kubeconfig-file` | `FLANNEL_KUBECONFIG_FILE` | - | kubeconfig 文件路径（备选） |

### 4.2 认证参数

| 参数 | 环境变量 | 说明 |
|:-----|:---------|:-----|
| `--kube-tls-insecure` | `FLANNEL_KUBE_TLS_INSECURE` | 跳过 TLS 验证 |
| `--kube-tls-server-name` | `FLANNEL_KUBE_TLS_SERVER_NAME` | TLS Server Name |
| `--kube-ca-file` | `FLANNEL_KUBE_CA_FILE` | CA 证书文件 |
| `--kube-token-file` | `FLANNEL_KUBE_TOKEN_FILE` | ServiceAccount Token 文件 |

### 4.3 节点注解参数

| 参数 | 环境变量 | 默认值 | 说明 |
|:-----|:---------|:------:|:-----|
| `--node-refresh-interval` | `FLANNEL_NODE_REFRESH_INTERVAL` | 5m | 节点信息刷新间隔 |
| `--pod-cidr` | `FLANNEL_POD_CIDR` | - | 指定节点的 Pod CIDR（覆盖 kube-apiserver） |

---

<!-- chunk: 5. [[etcd|etcd]] 配置参数 -->
## 5. etcd 配置参数

### 5.1 etcd 连接参数

| 参数 | 环境变量 | 默认值 | 说明 |
|:-----|:---------|:------:|:-----|
| `--etcd-endpoints` | `FLANNEL_ETCD_ENDPOINTS` | http://127.0.0.1:2379 | etcd 端点列表 |
| `--etcd-prefix` | `FLANNEL_ETCD_PREFIX` | /coreos.com/network | etcd 键前缀 |
| `--etcd-quorum-read` | `FLANNEL_ETCD_QUORUM_READ` | false | etcd 读 quorum |

### 5.2 etcd TLS 参数

| 参数 | 环境变量 | 说明 |
|:-----|:---------|:-----|
| `--etcd-cafile` | `FLANNEL_ETCD_CAFILE` | etcd CA 证书 |
| `--etcd-certfile` | `FLANNEL_ETCD_CERTFILE` | etcd 客户端证书 |
| `--etcd-keyfile` | `FLANNEL_ETCD_KEYFILE` | etcd 客户端私钥 |

**示例**：
```bash
flanneld \
  --etcd-endpoints=https://etcd1:2379,https://etcd2:2379 \
  --etcd-prefix=/my-cluster/network \
  --etcd-cafile=/etc/kubernetes/pki/etcd/ca.crt \
  --etcd-certfile=/etc/kubernetes/pki/etcd/client.crt \
  --etcd-keyfile=/etc/kubernetes/pki/etcd/client.key
```

---

<!-- chunk: 6. 接口选择参数 -->
## 6. 接口选择参数

### 6.1 网卡选择

| 参数 | 环境变量 | 说明 |
|:-----|:---------|:-----|
| `--iface` | `FLANNEL_IFACE` | 指定用于 Pod 通信的网卡 |
| `--iface-ipv6` | `FLANNEL_IFACE_IPV6` | 指定 IPv6 通信的网卡 |
| `--iface-label` | `FLANNEL_IFACE_LABEL` | 使用正则匹配网卡 |

**示例**：
```bash
# 指定单一网卡
flanneld --iface=eth0

# 指定多个网卡（逗号分隔）
flanneld --iface=eth0,enp0s3

# 使用正则表达式
flanneld --iface-regex=^eth[0-9]+$
```

### 6.2 接口检测逻辑

```
1. 如果指定 --iface，使用指定网卡
2. 如果指定 --iface-regex，匹配第一个符合的网卡
3. 否则使用默认路由的网卡
```

---

<!-- chunk: 7. 日志与监控参数 -->
## 7. 日志与监控参数

### 7.1 日志参数

| 参数 | 环境变量 | 默认值 | 说明 |
|:-----|:---------|:------:|:-----|
| `--v` | `FLANNEL_V` | 0 | 日志级别（0-5） |
| `--logtostderr` | - | true | 输出到 stderr |
| `--log-file` | `FLANNEL_LOG_FILE` | - | 日志文件路径 |
| `--syslog` | - | false | 输出到 syslog |

**日志级别**：
- 0 - Debug 及以上
- 1 - Info 及以上
- 2 - Warning
- 3 - Error
- 4 - Fatal
- 5 - Panic

**示例**：
```bash
flanneld --v=2 --log-file=/var/log/flannel.log
```

### 7.2 监控参数

| 参数 | 环境变量 | 默认值 | 说明 |
|:-----|:---------|:------:|:-----|
| `--healthz-ip` | `FLANNEL_HEALTHZ_IP` | 0.0.0.0 | healthz 服务 IP |
| `--healthz-port` | `FLANNEL_HEALTHZ_PORT` | 0 | healthz 服务端口（0 表示禁用） |

**示例**：
```bash
flanneld --healthz-port=10251
```

---

<!-- chunk: 8. 性能与资源参数 -->
## 8. 性能与资源参数

### 8.1 队列参数

| 参数 | 环境变量 | 默认值 | 说明 |
|:-----|:---------|:------:|:-----|
| `--event-queue-depth` | `FLANNEL_EVENT_QUEUE_DEPTH` | 5000 | 事件队列深度 |

**示例**：
```bash
# 大规模集群建议增加队列深度
flanneld --event-queue-depth=10000
```

### 8.2 子网租约参数

| 参数 | 环境变量 | 默认值 | 说明 |
|:-----|:---------|:------:|:-----|
| `--subnet-lease-ttl` | `FLANNEL_SUBNET_LEASE_TTL` | 24h | 子网租约 TTL |

**示例**：
```bash
# 延长租约时间（减少 etcd 压力）
flanneld --subnet-lease-ttl=48h
```

---

<!-- chunk: 9. 安全参数 -->
## 9. 安全参数

### 9.1 Capabilities

```yaml
securityContext:
  capabilities:
    add: ["NET_ADMIN", "NET_RAW"]
```

| Capability | 说明 |
|:-----------|:----|
| NET_ADMIN | 启用网络管理（路由、接口配置） |
| NET_RAW | 允许使用 RAW socket |

### 9.2 Seccomp

```yaml
securityContext:
  seccompProfile:
    type: RuntimeDefault
```

---

<!-- chunk: 10. 完整启动示例 -->
## 10. 完整启动示例

### 10.1 Kubernetes DaemonSet 完整配置

```yaml
args:
  - --ip-masq
  - --kube-subnet-mgr
  - --iface=eth0
  - --backend=vxlan
  - --backend.vxlan.port=4789
  - --backend.vxlan.directrouting=true
  - --event-queue-depth=5000
  - --healthz-port=10251
  - --v=2
env:
  - name: POD_NAME
    valueFrom:
      fieldRef:
        fieldPath: metadata.name
  - name: POD_NAMESPACE
    valueFrom:
      fieldRef:
        fieldPath: metadata.namespace
```

### 10.2 WireGuard 加密配置

```yaml
args:
  - --ip-masq
  - --kube-subnet-mgr
  - --iface=eth0
  - --backend=wireguard
  - --backend.wireguard.listen-port=51820
env:
  - name: FLANNEL_PRIVATE_KEY
    valueFrom:
      secretKeyRef:
        name: flannel-wireguard-keys
        key: privatekey
```

---

<!-- chunk: 11. 故障排查参数 -->
## 11. 故障排查参数

### 11.1 调试模式

```bash
# 启用详细日志
flanneld --v=5 --logtostderr

# 保存完整日志
flanneld --v=5 --log-file=/var/log/flanneld.log
```

### 11.2 网络调试

```bash
# 测试 etcd 连接
flanneld --etcd-endpoints=https://etcd:2379 --v=2

# 测试 Kubernetes API 连接
flanneld --kube-subnet-mgr --kubeconfig=/path/to/kubeconfig --v=2
```

---

<!-- chunk: 12. 弃用参数 -->
## 12. 弃用参数

| 参数 | 替代方案 | 弃用版本 |
|:-----|:---------|:--------:|
| `--public-ip` | 使用节点注解 | v0.14 |
| `--etcd-quorum-read` | `--kube-subnet-mgr` | v0.14 |
| `--networks` | `--network` | v0.12 |

---

<!-- chunk: Obsidian 相关文档 -->
## Obsidian 相关文档

- 网络 KUDIG Database — Global MOC
- [[05-网络/README.md|[[37-归档/domain-indexes/network/README-from-domain-5|Domain 5: Networking 网络]]working]] 网络]]
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

## See Also

- 04c-flannel-windows-support
- 04d-flannel-multi-cluster
- 05-terway-advanced-guide
- 06-service-concepts-types

## Related

- [[reference|#reference Hub]] — tag hub

- [[21-生态参考/03-领域索引/flannel-index.md|Flannel 知识图谱索引]]


<!-- risk-assessed -->
