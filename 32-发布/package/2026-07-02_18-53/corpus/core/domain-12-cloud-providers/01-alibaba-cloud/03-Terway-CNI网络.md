---
title: Terway CNI网络
description: 阿里云Terway网络模式详解、与Flannel对比、常见问题排查与诊断命令
summary: ACK Terway网络模式详解、IPAM管理及常见网络问题排查。
category: cloud-provider
tags:
- alibaba-cloud
- ack
- terway
- cni
- networking
- eni
- ipvlan
- troubleshooting
- ipam
tier: core
sources:
- 阿里云Terway官方文档
- ACK网络最佳实践
created: 2026-05-21
updated: 2026-05-21
last_updated: 2026-05-21
relationships:
- target: '[[entities/cni.md]]'
  type: related_to
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# Terway CNI网络

Terway 是阿里云自研的容器网络接口（[[entities/cni.md|CNI]]）插件，专为 ACK 集群设计。在专有云环境中，Terway 是推荐的网络方案，提供优于 Flannel 的网络性能和功能特性。本文档面向远程顾问，提供 Terway 模式详解、选型建议和排查指南。

---

## 1. Terway 模式详解

Terway 支持三种网络模式：ENI 模式、ENIIP 模式（推荐）和 IPvlan 模式。专有云环境根据网络能力和规模选择合适的模式。

### 1.1 ENI 模式

ENI（Elastic Network Interface）模式为每个 Pod 分配独立的弹性网卡：

```
┌─────────────────────────────────────────┐
│              ECS 节点                    │
│  ┌─────────────────────────────────┐    │
│  │         eth0 (主网卡)            │    │
│  │      172.16.1.10/24             │    │
│  └─────────────────────────────────┘    │
│         │                               │
│  ┌──────┴──────┬────────────┬─────────┐ │
│  ↓             ↓            ↓         │ │
│ ┌───┐      ┌────┐      ┌────┐      │ │
│ │Pod│      │Pod │      │Pod │      │ │
│ │A  │      │B   │      │C   │      │ │
│ └───┘      └────┘      └────┘      │ │
│ 独立ENI    独立ENI    独立ENI      │ │
│ 172.16.1.11 172.16.1.12 172.16.1.13 │ │
│                                      │ │
│  每个Pod独占一个ENI，与VPC IP一一对应   │ │
└─────────────────────────────────────────┘
```

**特点**：

| 特性 | 说明 |
|------|------|
| 网络性能 | 无NAT/Bridge开销，性能接近裸机 |
| IP管理 | 直接从VPC子网分配，无额外CIDR |
| 安全策略 | 支持安全组到Pod级别 |
| 限制 | 单节点ENI数量有限（典型8-15个） |
| 适用 | 低密度、高性能要求的应用 |

```yaml
# Terway ENI 模式配置
apiVersion: v1
kind: ConfigMap
metadata:
  name: eni-config
  namespace: kube-system
data:
  10-terway.conf: |
    {
      "cniVersion": "0.3.0",
      "name": "terway",
      "type": "terway",
      "mode": "ENI",
      "ipam": {
        "type": "terway-eni",
        "eni_conf": {
          "vswitch_id": "vsw-apsara-xxx",
          "security_group": "sg-apsara-xxx"
        }
      }
    }
```

### 1.2 ENIIP 模式（推荐）

ENIIP 模式在 ENI 上绑定多个辅助 IP，一个 ENI 可支持多个 Pod：

```
┌─────────────────────────────────────────┐
│              ECS 节点                    │
│  ┌─────────────────────────────────┐    │
│  │    eth0 (主网卡) + 辅助IP        │    │
│  │   172.16.1.10/24 (Primary)      │    │
│  │   172.16.1.11/24 (Secondary)    │    │
│  │   172.16.1.12/24 (Secondary)    │    │
│  │   172.16.1.13/24 (Secondary)    │    │
│  │   172.16.1.14/24 (Secondary)    │    │
│  └─────────────────────────────────┘    │
│         │                               │
│  ┌──────┴──────┬────────────┬─────────┐ │
│  ↓             ↓            ↓         │ │
│ ┌───┐      ┌────┐      ┌────┐      │ │
│ │Pod│      │Pod │      │Pod │      │ │
│ │A  │      │B   │      │C   │      │ │
│ └───┘      └────┘      └────┘      │ │
│ 172.16.1.11 172.16.1.12 172.16.1.13 │ │
│                                      │ │
│  一个ENI绑定多个Secondary IP分配给Pod  │ │
│  单节点Pod密度 = ENI数 × 每ENI IP数   │ │
└─────────────────────────────────────────┘
```

**特点**：

| 特性 | 说明 |
|------|------|
| 网络性能 | 接近ENI模式，Pod间通过veth pair |
| IP管理 | Secondary IP来自VPC子网 |
| Pod密度 | 单节点支持数十至数百Pod |
| 安全策略 | 安全组粒度到ENI级别 |
| 适用 | 专有云生产环境首选 |

```yaml
# Terway ENIIP 模式配置（专有云推荐）
apiVersion: v1
kind: ConfigMap
metadata:
  name: terway-eniip-config
  namespace: kube-system
data:
  10-terway.conf: |
    {
      "cniVersion": "0.3.0",
      "name": "terway",
      "type": "terway",
      "mode": "ENIIP",
      "ipam": {
        "type": "terway-eniip",
        "eni_conf": {
          "vswitch_id": "vsw-apsara-xxx",
          "security_group": "sg-apsara-xxx",
          "max_pool_size": 10,
          "min_pool_size": 5
        }
      }
    }
  terway_config: |
    {
      "ipam_type": "crds",
      " ENIIP_mode": true,
      " ENI_capacity": 10,
      " ENI_max_ip": 20,
      "pool_size": 10
    }
```

**ENIIP 关键参数说明**：

| 参数 | 默认值 | 说明 |
|------|--------|------|
| `max_pool_size` | 10 | 节点上预分配的IP池上限 |
| `min_pool_size` | 5 | 节点上预分配的IP池下限 |
| `pool_size` | 10 | Terway DaemonSet全局IP池配置 |
| `ENI_capacity` | 按实例规格 | 单节点可绑定的ENI数量 |
| `ENI_max_ip` | 按实例规格 | 每个ENI可绑定的辅助IP数 |

### 1.3 IPvlan 模式

IPvlan 模式使用内核 IPvlan 驱动实现容器网络：

```
┌─────────────────────────────────────────┐
│              ECS 节点                    │
│  ┌─────────────────────────────────┐    │
│  │         eth0 (主网卡)            │    │
│  │      172.16.1.10/24             │    │
│  └─────────────────────────────────┘    │
│         │                               │
│    IPvlan L2/L3                        │
│         │                               │
│  ┌──────┴──────┬────────────┬─────────┐ │
│  ↓             ↓            ↓         │ │
│ ┌───┐      ┌────┐      ┌────┐      │ │
│ │Pod│      │Pod │      │Pod │      │ │
│ │A  │      │B   │      │C   │      │ │
│ └───┘      └────┘      └────┘      │ │
│ 172.16.1.11 172.16.1.12 172.16.1.13 │ │
│                                      │ │
│  Pod与宿主机共享MAC地址               │ │
│  更低的网络开销，但需内核支持          │ │
└─────────────────────────────────────────┘
```

**IPvlan vs ENIIP 对比**：

| 特性 | IPvlan | ENIIP |
|------|--------|-------|
| 网络栈 | 共享宿主机 | 独立veth |
| 性能 | 更优 | 优秀 |
| 内核要求 | Linux 4.2+ | 无特殊要求 |
| 兼容性 | 部分应用不兼容 | 更好 |
| 专有云支持 | 视版本而定 | 完全支持 |

---

## 2. Terway vs Flannel 选择

| 对比维度 | Terway (ENIIP) | Flannel (VXLAN) |
|----------|----------------|-----------------|
| **网络模型** | 与VPC网络平面一体 | 叠加网络（Overlay） |
| **IP来源** | VPC子网IP | 独立的Pod CIDR |
| **跨节点通信** | VPC路由直达 | VXLAN封装 |
| **网络性能** | 接近线速 | 有VXLAN封装开销 |
| **Pod密度** | 受限于ENI/IP配额 | 理论上无限制 |
| **安全组** | 支持Pod级安全组 | 仅节点级安全组 |
| **SLB直通** | 支持Pod直接挂载SLB | 需通过NodePort |
| **网络策略** | 支持K8s NetworkPolicy | 需额外组件 |
| **专有云支持** | 推荐 | 功能受限 |
| **与外部通信** | 无需NAT | 需SNAT |

**选择建议**：

- **专有云生产环境**：优先选择 Terway ENIIP 模式
- **小型测试环境**：Flannel 也可使用，但需注意跨VPC限制
- **高性能网络场景**：Terway ENI 或 IPvlan 模式
- **大规模集群**：Terway ENIIP，提前规划 IP 资源

---

## 3. Terway 常见问题

### 3.1 IP不足问题

**症状**：Pod 创建失败，事件显示 IP 分配失败

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 远程诊断：检查Pod事件
kubectl describe pod <pending-pod>
# Events: "Failed to allocate IP address: no available IP"

# 检查Terway IP池状态
kubectl get pods -n kube-system -l app=terway-eniip -o wide
kubectl logs -n kube-system -l app=terway-eniip --tail=200
```
**根因与解决方案**：

| 根因 | 检查方法 | 解决方案 |
|------|----------|----------|
| VSwitch IP耗尽 | 检查VSwitch可用IP数 | 扩容VSwitch CIDR或添加新VSwitch |
| ENI配额不足 | 检查实例规格ENI上限 | 升级ECS规格或添加节点 |
| IP池分配不均 | 检查各节点IP使用率 | 调整max_pool_size参数 |
| 大量Terminated Pod占IP | 检查Terminating状态Pod | 强制删除或调优gc阈值 |

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 检查VSwitch IP余量
aliyun vpc DescribeVSwitchAttributes --VSwitchId vsw-apsara-xxx --RegionId cn-apsara-local
# 检查Terway IP分配
kubectl exec -n kube-system terway-eniip-xxxx -- terway-cli show
```
### 3.2 ENI配额问题

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 检查ECS实例规格的ENI配额
# 典型规格配额参考：
# ecs.g7.large: 2 ENI, 10 IPs/ENI = 20 Pod/节点
# ecs.g7.xlarge: 3 ENI, 10 IPs/ENI = 30 Pod/节点
# ecs.g7.2xlarge: 4 ENI, 15 IPs/ENI = 60 Pod/节点

# 查看节点当前ENI使用情况
kubectl exec -n kube-system terway-eniip-xxxx -- terway-cli show | grep ENI

# 计算节点Pod容量
# 公式：Pod容量 = (ENI数量 - 1) × 每个ENI的IP数 + 1
# 减1是因为主ENI占用一个，加1是主ENI本身也可分配IP
```
**ENI配额计算公式**：

| 实例规格 | ENI数 | IP数/ENI | 理论Pod上限 | 实际可用（保留1ENI） |
|----------|-------|----------|-------------|---------------------|
| ecs.g7.large | 2 | 10 | 20 | 10 |
| ecs.g7.xlarge | 3 | 10 | 30 | 20 |
| ecs.g7.2xlarge | 4 | 15 | 60 | 45 |
| ecs.g7.4xlarge | 8 | 30 | 240 | 210 |
| ecs.g7.8xlarge | 8 | 30 | 240 | 210 |

### 3.3 跨VPC通信问题

专有云环境中，ACK集群Pod可能需要与其他VPC或传统网络通信：

```
┌─────────────┐         ┌─────────────┐         ┌─────────────┐
│   VPC-A     │         │   VPC-B     │         │  传统网络    │
│  ACK集群     │ ←─────→ │  数据库VPC   │ ←─────→ │  核心系统    │
│ 172.16.0.0/16│  对等连接 │ 172.17.0.0/16│  专线/VPN │ 10.0.0.0/8  │
└─────────────┘         └─────────────┘         └─────────────┘
       ↑
   Pod 172.16.1.10
```

**排查步骤**：

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 跨VPC排查
aliyun vpc DescribeRouteTableList --RouteTableId vtb-apsara-xxx --RegionId cn-apsara-local
kubectl run -it --rm debug --image=busybox:1.36 --restart=Never -- traceroute <target-ip>
aliyun ecs DescribeSecurityGroupAttribute --SecurityGroupId sg-apsara-xxx --RegionId cn-apsara-local
```
---

## 4. Terway 排查命令

### 4.1 ack-terway-cli 工具

`ack-terway-cli` 是 Terway 诊断的核心工具，需在 Terway Pod 内或节点上执行：

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 进入Terway Pod执行诊断
kubectl exec -it -n kube-system terway-eniip-xxxx -- /bin/sh

# 查看Terway状态（典型输出：IP池、ENI、Pod映射）
terway-cli show
terway-cli show --detail
terway-cli show --pod <namespace>/<pod-name>
terway-cli show --eni

# 手动释放IP（Pod已删除但IP未释放时）
terway-cli release --ip <ip-address>
```
### 4.2 IPAM 诊断

IP 地址管理（IPAM）是 Terway 的核心组件，负责 IP 分配与回收：

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 检查Terway IPAM CRD
kubectl get crd | grep terway
# 输出：
# podenis.network.aliyun.com
# nodes.network.aliyun.com

# 查看Pod ENI分配记录
kubectl get podeni -A
kubectl get podeni -n default <pod-name> -o yaml

# 典型PodENI结构：
# apiVersion: network.aliyun.com/v1beta1
# kind: PodENI
# metadata:
#   name: app-1
#   namespace: default
# spec:
#   allocation:
#     eni:
#       id: "eni-apsara-xxx"
#       macAddress: "00:16:3e:xx:xx:xx"
#     ipv4: "172.16.1.101"
#     status: "Allocated"

# 查看节点网络配置
kubectl get node <node-name> -o yaml | grep -A 20 allocatable
# 检查：
#   allocatable:
#     aliyun/eni: "10"
#     aliyun/eniip: "100"
```
### 4.3 网络连通性诊断

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# === Pod网络诊断 ===
kubectl run -it --rm network-debug --image=nicolaka/netshoot --restart=Never
ip addr show; cat /etc/resolv.conf; ip route
ping <target-pod-ip>
curl http://<target-pod-ip>:<port>
nslookup <service-name>.default.svc.cluster.local
iptables -t nat -L -n -v | grep KUBE

# === 节点网络诊断 ===
ip link show | grep veth
ip route show table all | grep 172.16
ip link show | grep terway
```
### 4.4 远程诊断检查清单

远程顾问通过工单指导客户执行以下检查：

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
#!/bin/bash
# Terway 远程诊断脚本

echo "=== 1. Terway Pod 状态 ==="
kubectl get pods -n kube-system -l app=terway-eniip -o wide

echo "=== 2. Terway 日志 ==="
kubectl logs -n kube-system -l app=terway-eniip --tail=100

echo "=== 3. Pod IP 分配 ==="
kubectl get podeni -A

echo "=== 4. 节点IP容量 ==="
kubectl get nodes -o custom-columns=\
  NAME:.metadata.name,\
  ENI_CAPACITY:.status.allocatable.\alibabacloud/eni,\
  ENIIP_CAPACITY:.status.allocatable.\alibabacloud/eniip

echo "=== 5. VSwitch IP 余量（通过API）==="
# aliyun vpc DescribeVSwitches --VpcId vpc-apsara-xxx

echo "=== 6. 异常Pod检查 ==="
kubectl get pods -A --field-selector status.phase!=Running,status.phase!=Succeeded

echo "=== 7. 网络策略检查 ==="
kubectl get networkpolicies -A

echo "=== 8. Terway CLI 状态 ==="
for pod in $(kubectl get pods -n kube-system -l app=terway-eniip -o name); do
  echo "--- $pod ---"
  kubectl exec -n kube-system $pod -- terway-cli show 2>/dev/null || echo "terway-cli 不可用"
done
```
---

## 5. Terway 配置调优

### 5.1 大规模集群调优

```yaml
# Terway DaemonSet 大规模调优
apiVersion: apps/v1
kind: DaemonSet
metadata:
  name: terway-eniip
  namespace: kube-system
spec:
  template:
    spec:
      containers:
        - name: terway
          env:
            # IP池预热，减少冷启动延迟
            - name: ENIIP_POOL_SIZE
              value: "20"
            # 加速IP释放
            - name: ENIIP_GC_INTERVAL
              value: "30"
            # 最大并发分配
            - name: MAX_IP_ALLOCATION_BATCH
              value: "10"
          resources:
            limits:
              cpu: "2"
              memory: "1Gi"
            requests:
              cpu: "500m"
              memory: "512Mi"
```

### 5.2 IP 泄漏防护

```yaml
# 配置kubelet垃圾回收，加速Pod清理
apiVersion: kubelet.config.k8s.io/v1beta1
kind: KubeletConfiguration
evictionHard:
  memory.available: "500Mi"
  nodefs.available: "5%"
evictionSoft:
  memory.available: "1Gi"
  nodefs.available: "10%"
evictionSoftGracePeriod:
  memory.available: "1m"
  nodefs.available: "1m30s"
# 容器清理相关
containerLogMaxSize: "50Mi"
containerLogMaxFiles: 5
```

---

## 相关文档

- [[domain-12-cloud-providers/阿里云/01-专有云架构概述.md|专有云架构概述]]
- [[domain-12-cloud-providers/阿里云/02-ACK集群运维.md|ACK集群运维]]
- [[domain-12-cloud-providers/阿里云/04-阿里云存储集成.md|阿里云存储集成]]
- [[domain-12-cloud-providers/阿里云/05-阿里云SLB与Ingress.md|阿里云SLB与Ingress]]
- [[domain-12-cloud-providers/阿里云/06-阿里云专有云远程顾问指南.md|阿里云专有云远程顾问指南]]
- [[32-发布/package/2026-07-02_18-53/corpus/core/domain-12-cloud-providers/05-alicloud-ack/004-ack-vpc-network|ACK VPC网络]]
- [[alicloud-ack-overview|阿里云ACK概述]]
## Related

- [[domain-17-system-foundation/知识字典/networking/ingress.md|Ingress]]
- [[32-发布/package/2026-07-02_18-53/corpus/supporting/domain-03-networking-traffic/00-core-k8s-networking/05-ingress-fundamentals|Kubernetes Ingress 基础概念与核心原理 (Ingress Fundamentals)]]
- [[32-发布/package/2026-07-02_18-53/corpus/supporting/domain-03-networking-traffic/00-core-k8s-networking/06-ingress-controller-deep-dive|128 - Ingress Controller 深入剖析]]
- [[32-发布/package/2026-07-02_18-53/corpus/supporting/domain-03-networking-traffic/00-core-k8s-networking/07-nginx-ingress-complete-guide|129 - NGINX Ingress 完整配置指南]]


<!-- risk-assessed -->
