---
title: CNI Plugins
description: CNI Plugins — Kubernetes 生产运维知识库
summary: CNI Plugins — Kubernetes 生产运维知识库
category: entities
tags:
- k8s
- cni
- networking
- calico
- cilium
- flannel
- terway
- kubelet
- networkpolicy
- ebpf
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- CNI Plugins 是什么
- 如何 CNI Plugins
trigger_keywords:
- CNI
- Plugins
prerequisites:
- kubectl-basics
- ebpf-basics
- cilium-basics
- cni-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# CNI Plugins

## What is CNI

CNI (Container Network Interface) is the standard plugin interface Kubernetes uses to configure Pod networking. CNI plugins are invoked by [[kubelet|kubelet]] during Pod creation to set up network namespaces, assign IP addresses, and configure routes.

## Major CNI Plugins

| Plugin | Type | Features | Best For |
|--------|------|----------|----------|
| **Calico** | BGP routing | [[networkpolicy\|NetworkPolicy]] enforcement, BGP peering, IPIP/VXLAN overlay | Enterprise, NetworkPolicy-heavy |
| **Cilium** | eBPF-based | L7 policy, identity-aware security, observability, service mesh replacement | High-performance, security-focused |
| **Flannel** | Overlay (VXLAN/UDP/WireGuard) | Simple, minimal overhead, dual-stack, WireGuard encryption | Small clusters, simplicity |
| **Terway** | Alibaba Cloud ENI | Direct ENI IP allocation, high throughput, VPC-native | Alibaba Cloud environments |

## CNI Requirements

Every CNI plugin must satisfy:
- Each Pod gets a unique IP address
- Pods on the same node can communicate without NAT
- Pods on different nodes can communicate without NAT (cluster-wide flat network)
- No special port mapping needed

## IPAM (IP Address Management)

CNI plugins handle IP allocation through IPAM plugins:
- **host-local**: Node-scoped IP range allocation
- **DHCP**: External DHCP server
- **Static**: Fixed IP assignment
- **Cloud provider**: VPC subnet allocation (Terway)

## Selection Criteria

Choose based on:
- **Scale**: Flannel for small, Calico/Cilium for large
- **Security**: Cilium for eBPF-based L7 policies, Calico for standard NetworkPolicy
- **Cloud integration**: Terway for Alibaba Cloud, AWS VPC CNI for AWS
- **Performance**: Cilium eBPF > Calico BGP > Flannel VXLAN

## CNI 配置文件

CNI 配置位于 `/etc/cni/net.d/`，kubelet 按文件名排序加载第一个配置：

```json
// /etc/cni/net.d/10-calico.conflist (Calico 示例)
{
  "name": "k8s-pod-network",
  "cniVersion": "1.0.0",
  "plugins": [
    {
      "type": "calico",
      "log_level": "info",
      "datastore_type": "kubernetes",
      "mtu": 1440,
      "ipam": { "type": "calico-ipam" },
      "policy": { "type": "k8s" }
    },
    { "type": "portmap", "snat": true, "capabilities": {"portMappings": true} },
    { "type": "bandwidth", "capabilities": {"bandwidth": true} }
  ]
}
```

```json
// /etc/cni/net.d/05-cilium.conflist (Cilium 示例)
{
  "name": "cilium",
  "cniVersion": "0.4.0",
  "plugins": [
    {
      "type": "cilium-cni",
      "enable-debug": false,
      "log-file": "/var/run/cilium/cilium-cni.log"
    }
  ]
}
```

## 安装与部署

```bash
# 🟡 Calico (BGP 模式)
kubectl apply -f https://raw.githubusercontent.com/projectcalico/calico/v3.27.0/manifests/calico.yaml
# 自定义: 修改 CALICO_IPV4POOL_IPIP 为 Never (BGP) 或 Always (IPIP)

# 🟡 Calico (VXLAN 模式 - 无需 BGP)
kubectl apply -f https://raw.githubusercontent.com/projectcalico/calico/v3.27.0/manifests/calico-vxlan.yaml

# 🟡 Cilium (eBPF 模式)
helm repo add cilium https://helm.cilium.io/
helm install cilium cilium/cilium --namespace kube-system \
  --set kubeProxyReplacement=true \
  --set k8sServiceHost=<API_SERVER_IP> \
  --set k8sServicePort=6443

# 🟡 Flannel (VXLAN)
kubectl apply -f https://raw.githubusercontent.com/flannel-io/flannel/v0.24.0/Documentation/kube-flannel.yml

# 🟢 验证 CNI 安装
kubectl get pods -n kube-system -l k8s-app=calico-node
kubectl get pods -n kube-system -l k8s-app=cilium
kubectl get pods -n kube-system -l app=flannel
kubectl get nodes -o wide  # 确认 Pod CIDR 已分配
```

## 运维操作

```bash
# 🟢 检查 CNI 配置
ls -la /etc/cni/net.d/
cat /etc/cni/net.d/*.conflist

# 🟢 Calico 诊断
calicoctl node status  # BGP 对等状态
calicoctl get ippool -o wide
calicoctl get felixconfiguration
calicoctl ipam show

# 🟢 Cilium 诊断
cilium status --all-controllers
cilium endpoint list
cilium bpf tunnel list
cilium monitor --type drop

# 🟢 Flannel 诊断
cat /run/flannel/subnet.env
ip -d link show flannel.1
iptables -t nat -L POSTROUTING -v -n | grep flannel

# 🟢 检查 Pod 网络命名空间
crictl inspect <container-id> | jq '.info.runtimeSpec.linux.namespaces'
nsenter -t <pause-pid> -n ip addr
nsenter -t <pause-pid> -n ip route

# 🟢 检查 kubelet CNI 状态
journalctl -u kubelet | grep -i cni
ls /var/lib/cni/networks/

# 🟡 重启 CNI Pod (网络中断风险)
kubectl rollout restart daemonset/calico-node -n kube-system
kubectl rollout restart daemonset/cilium -n kube-system
kubectl rollout restart daemonset/kube-flannel-ds -n kube-system
```

## 故障排查

| 症状 | 可能原因 | 诊断命令 | 修复方案 |
|------|----------|----------|----------|
| Pod 卡在 ContainerCreating | CNI 插件未就绪/IP 耗尽 | `kubectl describe pod` 查看 Events | 检查 CNI Pod 状态，扩容 IPAM 池 |
| 跨节点 Pod 不通 | 路由/隧道未建立 | `calicoctl node status` / `ip route` | 检查 BGP 对等/防火墙规则 |
| DNS 解析失败 | CoreDNS Pod 网络异常 | `kubectl exec pod -- nslookup kubernetes.default` | 检查 CoreDNS Endpoint |
| Service 不可达 | kube-proxy/iptables 规则异常 | `iptables-save \| grep <svc-ip>` | 重启 kube-proxy |
| 网络策略不生效 | CNI 不支持 NetworkPolicy | `kubectl get networkpolicy` | 使用 Calico/Cilium 替代 Flannel |
| MTU 不匹配导致大包丢失 | CNI MTU > 物理网络 MTU | `ping -M do -s 1400 <target>` | 调整 CNI MTU 配置 |
| IP 地址泄漏 | Pod 删除后 IP 未回收 | `calicoctl ipam show` / `cilium ip list` | 运行 IPAM GC |

### 排查流程

```
Pod 网络异常
├── kubectl describe pod → 查看 Events
│   ├── "failed to set up sandbox" → CNI 插件问题
│   │   ├── 检查 CNI DaemonSet Pod 状态
│   │   ├── 检查 /etc/cni/net.d/ 配置文件
│   │   └── 检查 /opt/cni/bin/ 二进制文件
│   └── "no IP addresses available" → IPAM 耗尽
│       ├── calicoctl ipam show / cilium ip list
│       └── 扩容 IPPool 或清理泄漏 IP
├── Pod 已 Running 但网络不通
│   ├── 同节点不通 → 检查 veth pair / bridge
│   ├── 跨节点不通 → 检查路由/隧道/BGP
│   └── Service 不通 → 检查 kube-proxy/iptables
└── 间歇性网络问题
    ├── 检查 MTU 配置
    ├── 检查节点资源 (CPU/内存压力)
    └── 检查 conntrack 表是否满
```

## 生产案例

### 案例 1: Calico BGP 对等失败导致跨节点通信中断

- **场景**: 3 节点集群扩容后，新节点 Pod 无法与其他节点通信
- **排查**: `calicoctl node status` 显示新节点 BGP 状态为 Idle；检查发现新节点防火墙阻止了 179 端口（BGP）
- **方案**: 开放节点间 TCP 179 端口；`calicoctl node status` 确认 Established
- **效果**: 跨节点通信恢复，BGP 路由正常收敛

### 案例 2: Cilium IPAM 地址泄漏导致 Pod 无法调度

- **场景**: 运行 2 周后，新 Pod 报 "no IP addresses available in range"
- **排查**: `cilium ip list` 显示已分配 IP 远超实际 Pod 数；发现是 Job Pod 异常终止后 IP 未释放
- **方案**: `cilium ip release` 清理泄漏 IP；升级 Cilium 到修复版本；配置 `ipam.operator.clusterPoolIPv4PodCIDRList` 扩大地址池
- **效果**: IP 泄漏问题解决，Pod 正常调度

## 对比与选型

| 维度 | Calico | Cilium | Flannel | Terway |
|------|--------|--------|---------|--------|
| 数据面 | BGP/IPIP/VXLAN | eBPF | VXLAN/WireGuard | ENI 直通 |
| NetworkPolicy | ✅ 完整 | ✅ L7 级别 | ❌ 不支持 | ✅ |
| 性能 | 高 (BGP) | 最高 (eBPF) | 中 (Overlay) | 最高 (VPC) |
| 复杂度 | 中 | 高 | 低 | 低 (云托管) |
| 内核要求 | 无特殊 | >= 4.19 | 无特殊 | 无特殊 |
| 适用规模 | 中大型 | 大型 | 小型 | 阿里云 |
| 可观测性 | 基础 | Hubble (丰富) | 无 | 云监控 |

## 检查清单

- [ ] CNI DaemonSet 所有 Pod Running 且 Ready
- [ ] 所有节点 BGP/隧道对等正常（如适用）
- [ ] IPAM 地址池使用率 < 80%
- [ ] MTU 配置与底层网络一致
- [ ] NetworkPolicy 功能验证通过
- [ ] 跨节点 Pod-to-Pod 连通性测试通过
- [ ] DNS 解析正常（CoreDNS 可达）
- [ ] CNI 二进制和配置文件在所有节点一致
- [ ] 防火墙规则允许 CNI 所需端口（BGP 179, VXLAN 4789, IPIP proto 4）
- [ ] 监控告警覆盖 CNI Pod 状态和 IPAM 使用率

## Related
- [[22-概念/11-交叉分析/CNI 插件 × NetworkPolicy.md|CNI 插件 × NetworkPolicy]] — 综合
- [[cilium]] — Cilium
- [[23-实体/02-K8s核心组件/kubelet.md|kubelet]] — kubelet
- [[cni]] — CNI (Container Network Interface)
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[22-概念/03-网络/service-networking.md|service-networking]] — Service Networking
- [[23-实体/02-K8s核心组件/networkpolicy.md|NetworkPolicy]]

<!-- risk-assessed -->
