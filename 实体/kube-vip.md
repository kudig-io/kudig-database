---
title: kube-vip (entities)
description: '## 概述'
summary: 'kube-vip 为 Kubernetes 集群提供虚拟 IP (VIP) 和负载均衡功能。它可以作为控制平面的高可用解决方案，提供浮动 VIP 确保 API Server 始终可访问。同时也可以作为 LoadBalancer 类型 [[Service|Service]] 的实现，为裸金属环境提供服务负载均衡。'
category: entities
tags:
- k8s
- cncf
- networking
- kube-vip
- crd
- operator
tier: core
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- kube-vip 是什么
- 如何 kube-vip
trigger_keywords:
- kube-vip
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# kube-vip

> **CNCF 状态**: Sandbox | **类别**: Networking | **主要语言**: Go

## 概述

kube-vip 是由 plunder-app 开源（现由社区维护）的 Kubernetes 虚拟 IP（VIP）和负载均衡工具，2021 年加入 CNCF Sandbox。它为 Kubernetes 集群提供虚拟 IP 管理和负载均衡功能，可作为控制平面的高可用解决方案（提供浮动 VIP 确保 API Server 始终可访问），也可作为 LoadBalancer 类型 [[Service|Service]] 的实现（为裸金属环境提供服务负载均衡）。

## 核心特性

- **控制平面 HA**: 为 [[系统基础/知识字典/fundamentals/the-kubernetes-api.md|Kubernetes API]] Server 提供浮动 VIP
- **Service LoadBalancer**: 裸金属集群的 LoadBalancer 类型 Service 实现
- **ARP/BGP 双模**: Layer 2 (ARP/NDP) 和 Layer 3 (BGP) 两种模式
- **Leader 选举**: 基于 Raft 或 Kubernetes Lease 的分布式选举
- **轻量级**: 单一二进制文件，无外部依赖
- **IPv4/IPv6**: 完整双栈支持

## 架构

kube-vip 以单一 Pod/进程运行在每个节点上。控制平面 HA 模式下，kube-vip 通过 Kubernetes Lease（或 Raft）进行 Leader 选举，持有 VIP 的 Leader 节点通过 ARP（Layer 2）向局域网公告 VIP 的 MAC 地址。当 Leader 节点故障时，新 Leader 接管 VIP 并发送 ARP 广播更新 MAC 映射（ Gratuitous ARP）。BGP 模式下，多个节点同时公告 VIP 到上游路由器，由路由器进行 ECMP 负载均衡。Service LoadBalancer 模式下，kube-vip 通过 Cloud Controller Manager 接口监听 LoadBalancer Service，自动分配和公告 VIP。

## Kubernetes 集成

控制平面 HA 模式下，kube-vip 作为 Static Pod 运行在 Master 节点，为 kube-apiserver 的 6443 端口提供 VIP。配合 keepalived 替代方案。Service LoadBalancer 模式下，kube-vip 通过 `--service-provider` 标志作为 Cloud Provider 运行，监听 `type: LoadBalancer` 的 Service，从 IPAM 池分配 VIP。支持通过 CRD（KubeVIPIPSet）管理 IP 地址池。与 kube-proxy 配合实现完整的南北向流量路由。

## 生产使用场景

1. **裸金属 K8s HA**: 为 kubeadm/k0s/k3s 集群的 API Server 提供浮动 VIP
2. **裸金属 LoadBalancer**: 替代 MetalLB 为 Service 提供外部 IP
3. **BGP 负载均衡**: 大规模集群通过 BGP 实现真正的多节点负载均衡
4. **边缘集群**: 轻量级 VIP 方案适配边缘场景

## 安装与配置

```bash
# 控制平面 HA（Static Pod）
KVVERSION=$(curl -sL https://api.github.com/repos/kube-vip/kube-vip/releases/latest | grep tag_name | cut -d '"' -f 4)
alias kube-vip="ctr image pull ghcr.io/kube-vip/kube-vip:$KVVERSION; \
  ctr run --rm --net-host ghcr.io/kube-vip/kube-vip:$KVVERSION vip /kube-vip"

# 生成控制平面 VIP 清单
kube-vip manifest pod --address 192.168.1.100 --controlplane \
  --services --arp --leaderElection | tee /etc/kubernetes/manifests/kube-vip.yaml

# Service LoadBalancer 模式 (DaemonSet)
kubectl apply -f https://kube-vip.io/manifests/kube-vip-cloud-controller.yaml
kubectl apply -f https://kube-vip.io/manifests/kube-vip.yaml

# BGP 模式
kube-vip manifest pod --address 192.168.1.100 --controlplane \
  --bgp --peerAS 65000 --peerAddress 192.168.1.1 \
  --localAS 65001 --bgpRouterID 192.168.1.10
```

```yaml
# kube-vip Static Pod 清单示例 (/etc/kubernetes/manifests/kube-vip.yaml)
apiVersion: v1
kind: Pod
metadata:
  name: kube-vip
  namespace: kube-system
spec:
  containers:
    - name: kube-vip
      image: ghcr.io/kube-vip/kube-vip:v0.7.0
      args:
        - manager
      env:
        - name: vip_arp
          value: "true"
        - name: address
          value: "192.168.1.100"
        - name: port
          value: "6443"
        - name: vip_leaderelection
          value: "true"
        - name: vip_leasename
          value: "plndr-cp-lock"
        - name: vip_leaseduration
          value: "15"
        - name: vip_renewdeadline
          value: "10"
        - name: vip_retryperiod
          value: "2"
      securityContext:
        capabilities:
          add: ["NET_ADMIN", "NET_RAW", "SYS_TIME"]
      volumeMounts:
        - name: kubeconfig
          mountPath: /etc/kubernetes/admin.conf
  hostNetwork: true
  volumes:
    - name: kubeconfig
      hostPath:
        path: /etc/kubernetes/admin.conf
```

```yaml
# Service LoadBalancer IP 地址池 (KubeVIPIPSet CRD)
apiVersion: kube-vip.io/v1alpha1
kind: KubeVIPIPSet
metadata:
  name: production-pool
spec:
  addresses:
    - 192.168.1.200-192.168.1.220
    - 10.10.10.50
---
# 使用特定 IP 的 Service
apiVersion: v1
kind: Service
metadata:
  name: web-lb
  annotations:
    kube-vip.io/loadbalancerIPs: "192.168.1.200"
spec:
  type: LoadBalancer
  selector:
    app: web
  ports:
    - port: 80
      targetPort: 8080
```

## 运维操作

```bash
# 🟢 检查 kube-vip Pod 状态
kubectl get pods -n kube-system -l app.kubernetes.io/name=kube-vip
kubectl logs -n kube-system -l app.kubernetes.io/name=kube-vip --tail=30

# 🟢 检查 VIP 分配
kubectl get svc -A -o wide | grep LoadBalancer
kubectl get kubevipippool -A  # 查看 IP 池

# 🟢 检查 Leader 选举状态
kubectl get lease -n kube-system plndr-cp-lock -o yaml
kubectl get lease -n kube-system plndr-svcs-lock -o yaml

# 🟢 检查 ARP 公告 (Layer 2)
arping -I eth0 192.168.1.100  # 验证 VIP MAC 地址
ip addr show | grep 192.168.1.100  # 确认 VIP 在哪个节点

# 🟢 检查 BGP 状态 (Layer 3)
kubectl logs -n kube-system -l app.kubernetes.io/name=kube-vip | grep -i bgp

# 🟡 重启 kube-vip DaemonSet
kubectl rollout restart daemonset/kube-vip-ds -n kube-system

# 🔴 删除 VIP Service (外部 IP 将不可用)
kubectl delete svc web-lb -n production
```

## 故障排查

| 症状 | 可能原因 | 诊断命令 | 修复方案 |
|------|----------|----------|----------|
| API Server 不可达 | VIP Leader 丢失 | `kubectl get lease plndr-cp-lock` | 检查 Master 节点状态 |
| VIP 未分配给 Service | IP 池耗尽 | `kubectl get kubevipippool -o yaml` | 扩大 IP 池范围 |
| ARP 公告失败 | 缺少 NET_ADMIN 权限 | 检查 Pod SecurityContext | 添加 NET_ADMIN capability |
| Leader 频繁切换 | 网络报动/Lease 超时 | 检查 kube-vip 日志 | 调整 leaseduration/retryperiod |
| BGP 对等失败 | AS 号/密码配置错误 | 检查 BGP 日志 | 核对路由器配置 |
| Service ExternalIP 不生效 | Cloud Controller 未运行 | `kubectl get pods -n kube-system` | 部署 kube-vip-cloud-controller |

### 排查流程

```
VIP 异常
├── 控制平面 VIP 不可达
│   ├── 检查所有 Master 节点 kube-vip Pod 状态
│   ├── 检查 Lease (plndr-cp-lock) 持有者
│   ├── 在 Leader 节点: ip addr | grep <VIP>
│   ├── 检查 ARP: arping -I <iface> <VIP>
│   └── 检查防火墙: iptables -L | grep 6443
├── Service LoadBalancer 无 ExternalIP
│   ├── 检查 kube-vip-cloud-controller Pod
│   ├── 检查 IP 池是否有可用地址
│   ├── 检查 Service annotations
│   └── 检查 kube-vip DaemonSet 日志
└── VIP 频繁漂移
    ├── 检查 Lease renewdeadline 配置
    ├── 检查节点间网络延迟
    └── 检查节点资源压力 (CPU/内存)
```

## 生产案例

### 案例 1: 裸金属集群 API Server HA

- **场景**: 3 Master 裸金属集群，kubeadm 部署，需要 API Server 高可用
- **排查**: 单 Master 故障时 kubectl 无法连接，因为 kubeconfig 指向固定 IP
- **方案**: 部署 kube-vip 控制平面模式，VIP 192.168.1.100；kubeconfig 指向 VIP；Leader 选举基于 K8s Lease
- **效果**: Master 故障后 VIP 在 5 秒内漂移到新 Leader，kubectl 无感知切换

### 案例 2: 裸金属 LoadBalancer 替代 MetalLB

- **场景**: 边缘集群需要 LoadBalancer Service，但 MetalLB BGP 与现有网络设备不兼容
- **排查**: MetalLB BGP 会话无法与旧型号交换机建立
- **方案**: 使用 kube-vip ARP 模式替代；配置 IP 池 192.168.1.200-220；Service 自动获取 ExternalIP
- **效果**: LoadBalancer Service 正常工作，无需 BGP 支持，运维复杂度降低

## 对比与替代方案

| 维度 | kube-vip | MetalLB | Keepalived | HAProxy+KA |
|------|----------|---------|------------|------------|
| 控制平面 HA | ✅ | ❌ | ✅ | ✅ |
| Service LB | ✅ | ✅ | ❌ | ✅ |
| ARP (L2) | ✅ | ✅ | ✅ | ❌ |
| BGP (L3) | ✅ | ✅ | ❌ | ❌ |
| K8s 原生 | ✅ | ✅ | ❌ | ❌ |
| 资源占用 | 极低 (~20MB) | 低 | 低 | 中 |
| 成熟度 | 中 | 高 | 高 | 高 |
| 适用场景 | 裸金属一体化 | 裸金属 LB | 传统 VIP | 复杂 LB |

## 检查清单

- [ ] kube-vip Pod 在所有目标节点 Running
- [ ] 控制平面 VIP 可从所有客户端访问
- [ ] Leader 选举稳定（无频繁切换）
- [ ] ARP/BGP 公告正常（网络可达）
- [ ] IP 池地址充足且无冲突
- [ ] 防火墙允许 VRRP/ARP 协议
- [ ] kubeconfig 指向 VIP 而非单节点 IP
- [ ] 监控覆盖 kube-vip Pod 状态和 Leader 切换事件
- [ ] 故障切换时间 < 15 秒 (leaseduration 配置)

## 参考链接

- [[deployment]]
- [[operator-pattern]]
- [[概念/controller-pattern.md|controller-pattern]]
- [[pod-lifecycle]]
- [[概念/security-defense-depth.md|security-defense-depth]]

## Related

- [[opencost]] — OpenCost
- [[slimfaas]] — SlimFaas
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[实体/k8s-cluster-delete.md|Kubernetes 集群删除操作指南]] — Cross-reference
- [[技能/集群运维/kubeadm/kubeadm-ha-cluster-setup.md|kubeadm 高可用集群搭建]] — Cross-reference
- [[实体/cncf-networking.md|CNCF 网络与服务网格项目全景]] — Cross-reference

<!-- risk-assessed -->
