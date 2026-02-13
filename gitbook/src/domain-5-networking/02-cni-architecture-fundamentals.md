# 141 - CNI 架构与核心原理 (CNI Architecture & Fundamentals)

> **适用版本**: Kubernetes v1.25 - v1.32 | **难度**: 高级 | **最后更新**: 2026-01

---

## 1. Kubernetes 网络模型

### 核心原则

| 原则 | 说明 |
|:---|:---|
| **Pod 间通信** | 所有 Pod 可以直接通信，无需 NAT |
| **Node-Pod 通信** | 节点可以直接与所有 Pod 通信 |
| **Pod IP 一致性** | Pod 看到的自身 IP 与其他 Pod 看到的相同 |

### 网络分层架构

```
┌─────────────────────────────────────────────────────────────────────────┐
│                         Kubernetes 网络栈                                │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                          │
│   ┌─────────────────────────────────────────────────────────────────┐   │
│   │  Layer 4: Service 层                                            │   │
│   │  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐             │   │
│   │  │ ClusterIP   │  │  NodePort   │  │LoadBalancer │             │   │
│   │  └─────────────┘  └─────────────┘  └─────────────┘             │   │
│   └─────────────────────────────────────────────────────────────────┘   │
│                                    │                                     │
│                                    ▼                                     │
│   ┌─────────────────────────────────────────────────────────────────┐   │
│   │  Layer 3: kube-proxy 层                                         │   │
│   │  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐             │   │
│   │  │  iptables   │  │    IPVS     │  │  eBPF/nft   │             │   │
│   │  └─────────────┘  └─────────────┘  └─────────────┘             │   │
│   └─────────────────────────────────────────────────────────────────┘   │
│                                    │                                     │
│                                    ▼                                     │
│   ┌─────────────────────────────────────────────────────────────────┐   │
│   │  Layer 2: CNI 层                                                │   │
│   │  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐             │   │
│   │  │   Flannel   │  │   Calico    │  │   Cilium    │             │   │
│   │  │   Terway    │  │   Antrea    │  │  WeaveNet   │             │   │
│   │  └─────────────┘  └─────────────┘  └─────────────┘             │   │
│   └─────────────────────────────────────────────────────────────────┘   │
│                                    │                                     │
│                                    ▼                                     │
│   ┌─────────────────────────────────────────────────────────────────┐   │
│   │  Layer 1: 物理/虚拟网络                                         │   │
│   │  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐             │   │
│   │  │   VXLAN     │  │    BGP      │  │  VPC/ENI    │             │   │
│   │  └─────────────┘  └─────────────┘  └─────────────┘             │   │
│   └─────────────────────────────────────────────────────────────────┘   │
│                                                                          │
└─────────────────────────────────────────────────────────────────────────┘
```

---

## 2. CNI 规范与接口

### 2.1 CNI 操作类型

| 操作 | 触发时机 | 说明 |
|:---|:---|:---|
| **ADD** | Pod 创建 | 创建网络命名空间、分配 IP、配置路由 |
| **DEL** | Pod 删除 | 清理网络资源、释放 IP |
| **CHECK** | 健康检查 | 验证网络配置正确性 (可选) |
| **VERSION** | 版本查询 | 返回支持的 CNI 版本 |

### 2.2 CNI 配置文件结构

```json
{
  "cniVersion": "1.0.0",
  "name": "k8s-pod-network",
  "plugins": [
    {
      "type": "calico",
      "log_level": "info",
      "datastore_type": "kubernetes",
      "nodename": "__KUBERNETES_NODE_NAME__",
      "mtu": 1440,
      "ipam": {
        "type": "calico-ipam"
      },
      "policy": {
        "type": "k8s"
      },
      "kubernetes": {
        "kubeconfig": "/etc/cni/net.d/calico-kubeconfig"
      }
    },
    {
      "type": "portmap",
      "snat": true,
      "capabilities": {"portMappings": true}
    },
    {
      "type": "bandwidth",
      "capabilities": {"bandwidth": true}
    }
  ]
}
```

### 2.3 CNI 环境变量

| 变量 | 说明 |
|:---|:---|
| `CNI_COMMAND` | ADD/DEL/CHECK/VERSION |
| `CNI_CONTAINERID` | 容器 ID |
| `CNI_NETNS` | 网络命名空间路径 |
| `CNI_IFNAME` | 接口名称 (通常为 eth0) |
| `CNI_PATH` | CNI 插件搜索路径 |
| `CNI_ARGS` | 额外参数 |

---

## 3. Pod 网络创建流程

```
┌─────────────────────────────────────────────────────────────────────────┐
│                        Pod 网络初始化流程                                │
└─────────────────────────────────────────────────────────────────────────┘

  API Server              kubelet                CNI Plugin           IPAM
      │                      │                      │                  │
      │  1. Pod 调度到节点   │                      │                  │
      ├─────────────────────▶│                      │                  │
      │                      │                      │                  │
      │                      │  2. 创建 Sandbox     │                  │
      │                      ├────────┐             │                  │
      │                      │        │             │                  │
      │                      │◀───────┘             │                  │
      │                      │                      │                  │
      │                      │  3. 调用 CNI ADD     │                  │
      │                      ├─────────────────────▶│                  │
      │                      │                      │                  │
      │                      │                      │  4. 请求 IP      │
      │                      │                      ├─────────────────▶│
      │                      │                      │                  │
      │                      │                      │  5. 分配 IP      │
      │                      │                      │◀─────────────────┤
      │                      │                      │                  │
      │                      │                      │  6. 创建 veth    │
      │                      │                      ├────────┐         │
      │                      │                      │        │         │
      │                      │                      │◀───────┘         │
      │                      │                      │                  │
      │                      │                      │  7. 配置路由     │
      │                      │                      ├────────┐         │
      │                      │                      │        │         │
      │                      │                      │◀───────┘         │
      │                      │                      │                  │
      │                      │  8. 返回 IP 配置     │                  │
      │                      │◀─────────────────────┤                  │
      │                      │                      │                  │
      │                      │  9. 启动容器         │                  │
      │                      ├────────┐             │                  │
      │                      │        │             │                  │
      │                      │◀───────┘             │                  │
      │                      │                      │                  │
```

---

## 4. CNI 插件对比

| 特性 | Flannel | Calico | Cilium | Terway | Antrea |
|:---|:---:|:---:|:---:|:---:|:---:|
| **网络模式** | Overlay | Overlay/BGP | Overlay/Native | VPC | Overlay/NoEncap |
| **NetworkPolicy** | ❌ | ✅ | ✅ | ✅ | ✅ |
| **eBPF 支持** | ❌ | ✅ (部分) | ✅ (原生) | ✅ | ✅ |
| **加密** | ❌ | ✅ (WireGuard) | ✅ | ❌ | ✅ (IPSec) |
| **多集群** | ❌ | ✅ | ✅ | ❌ | ✅ |
| **Service Mesh** | ❌ | ❌ | ✅ | ❌ | ❌ |
| **Windows** | ✅ | ✅ | ❌ | ❌ | ✅ |
| **复杂度** | 低 | 中 | 高 | 中 | 中 |
| **性能** | 中 | 高 | 高 | 高 | 高 |

---

## 5. Overlay 网络原理

### 5.1 VXLAN 封装

```
┌─────────────────────────────────────────────────────────────────────────┐
│                         VXLAN 封装结构                                   │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                          │
│   原始数据包:                                                            │
│   ┌────────────┬────────────┬────────────┬──────────────────────────┐   │
│   │  Pod Eth   │   Pod IP   │    TCP     │        Payload           │   │
│   │   Header   │   Header   │   Header   │                          │   │
│   └────────────┴────────────┴────────────┴──────────────────────────┘   │
│                                                                          │
│   VXLAN 封装后:                                                          │
│   ┌──────────┬──────────┬────────┬────────┬────────────────────────┐    │
│   │ Outer    │ Outer IP │  UDP   │ VXLAN  │    原始数据包          │    │
│   │ Ethernet │  Header  │ Header │ Header │    (上面的内容)        │    │
│   │ 14 bytes │ 20 bytes │8 bytes │8 bytes │                        │    │
│   └──────────┴──────────┴────────┴────────┴────────────────────────┘    │
│                                                                          │
│   额外开销: 50 bytes (MTU: 1500 → 实际 1450)                            │
│                                                                          │
└─────────────────────────────────────────────────────────────────────────┘
```

### 5.2 VXLAN 跨节点通信

```
┌─────────────────────────────────────────────────────────────────────────┐
│  Node 1 (10.0.1.10)                  Node 2 (10.0.1.20)                 │
│  ┌─────────────────────┐             ┌─────────────────────┐            │
│  │  Pod A              │             │  Pod B              │            │
│  │  IP: 10.244.1.10    │             │  IP: 10.244.2.20    │            │
│  └─────────┬───────────┘             └───────────┬─────────┘            │
│            │ veth                                 │ veth                │
│            ▼                                      ▼                     │
│  ┌─────────────────────┐             ┌─────────────────────┐            │
│  │     cni0 Bridge     │             │     cni0 Bridge     │            │
│  │  IP: 10.244.1.1     │             │  IP: 10.244.2.1     │            │
│  └─────────┬───────────┘             └───────────┬─────────┘            │
│            │                                      │                     │
│            ▼                                      ▼                     │
│  ┌─────────────────────┐             ┌─────────────────────┐            │
│  │  flannel.1 (VTEP)   │────VXLAN───▶│  flannel.1 (VTEP)   │            │
│  │  IP: 10.244.1.0     │   隧道      │  IP: 10.244.2.0     │            │
│  └─────────┬───────────┘             └───────────┬─────────┘            │
│            │                                      │                     │
│            ▼                                      ▼                     │
│  ┌─────────────────────┐             ┌─────────────────────┐            │
│  │       eth0          │             │       eth0          │            │
│  │  IP: 10.0.1.10      │─────────────│  IP: 10.0.1.20      │            │
│  └─────────────────────┘   物理网络   └─────────────────────┘            │
└─────────────────────────────────────────────────────────────────────────┘
```

---

## 6. 路由模式原理 (BGP/host-gw)

```
┌─────────────────────────────────────────────────────────────────────────┐
│  Node 1 (10.0.1.10)                  Node 2 (10.0.1.20)                 │
│  ┌─────────────────────┐             ┌─────────────────────┐            │
│  │  Pod A              │             │  Pod B              │            │
│  │  IP: 10.244.1.10    │             │  IP: 10.244.2.20    │            │
│  └─────────┬───────────┘             └───────────┬─────────┘            │
│            │ veth                                 │ veth                │
│            ▼                                      ▼                     │
│  ┌─────────────────────┐             ┌─────────────────────┐            │
│  │      cali*          │             │      cali*          │            │
│  │   路由到 eth0       │             │   路由到 eth0       │            │
│  └─────────┬───────────┘             └───────────┬─────────┘            │
│            │                                      │                     │
│   路由表:                               路由表:                          │
│   10.244.2.0/24 via 10.0.1.20          10.244.1.0/24 via 10.0.1.10     │
│            │                                      │                     │
│            ▼                                      ▼                     │
│  ┌─────────────────────┐             ┌─────────────────────┐            │
│  │       eth0          │             │       eth0          │            │
│  │  IP: 10.0.1.10      │─────────────│  IP: 10.0.1.20      │            │
│  └─────────────────────┘   直接路由   └─────────────────────┘            │
│                         (无封装开销)                                     │
└─────────────────────────────────────────────────────────────────────────┘
```

### 路由模式优势

| 特性 | Overlay (VXLAN) | 路由模式 (BGP) |
|:---|:---|:---|
| MTU 开销 | ~50 bytes | 0 |
| 延迟 | 较高 | 较低 |
| 网络要求 | 无特殊要求 | L2 互通或 BGP 支持 |
| 调试难度 | 较高 | 较低 |

---

## 7. IPAM (IP 地址管理)

### 7.1 IPAM 类型

| 类型 | 说明 | 适用场景 |
|:---|:---|:---|
| **host-local** | 每节点独立 IP 段 | Flannel |
| **calico-ipam** | 全局 IP 池，按块分配 | Calico |
| **whereabouts** | 跨节点 IP 池 | 多租户 |
| **VPC IPAM** | 云厂商 VPC IP 直接分配 | Terway、AWS VPC CNI |

### 7.2 Calico IPPool 配置

```yaml
apiVersion: crd.projectcalico.org/v1
kind: IPPool
metadata:
  name: default-ippool
spec:
  cidr: 10.244.0.0/16
  ipipMode: CrossSubnet      # 跨子网使用 IPIP
  vxlanMode: Never
  natOutgoing: true
  nodeSelector: all()
  blockSize: 26              # 每节点 64 个 IP (/26)
---
# 特定节点池
apiVersion: crd.projectcalico.org/v1
kind: IPPool
metadata:
  name: gpu-node-pool
spec:
  cidr: 10.245.0.0/16
  ipipMode: Always
  natOutgoing: true
  nodeSelector: hardware == 'gpu'
```

---

## 8. 故障排查

### 8.1 常用诊断命令

```bash
# 查看 CNI 配置
ls -la /etc/cni/net.d/
cat /etc/cni/net.d/10-calico.conflist

# 查看 CNI 插件
ls -la /opt/cni/bin/

# 查看 Pod 网络命名空间
crictl pods
crictl inspectp <pod-id> | jq '.info.runtimeSpec.linux.namespaces'

# 进入 Pod 网络命名空间
nsenter -t $(crictl inspect <container-id> | jq '.info.pid') -n ip addr

# 检查路由表
ip route show
ip route get <pod-ip>

# 检查 VXLAN 接口
ip -d link show flannel.1
bridge fdb show dev flannel.1

# 抓包分析
tcpdump -i eth0 -n port 4789  # VXLAN
tcpdump -i cali+ -n           # Calico 接口
```

### 8.2 常见问题

| 问题 | 可能原因 | 解决方案 |
|:---|:---|:---|
| Pod 无法获取 IP | CNI 插件未安装/配置错误 | 检查 /etc/cni/net.d/ |
| Pod 间无法通信 | 网络策略/路由问题 | 检查 NetworkPolicy、路由表 |
| 跨节点通信失败 | 防火墙/VXLAN 端口 | 检查 UDP 4789/8472 |
| DNS 解析失败 | CoreDNS 问题 | 检查 CoreDNS Pod 状态 |
| 网络性能差 | MTU 配置不当 | 调整 MTU (VXLAN: 1450) |

---

## 9. 最佳实践

| 类别 | 建议 |
|:---|:---|
| **CNI 选择** | 简单场景用 Flannel，需要 NetworkPolicy 用 Calico/Cilium |
| **网络模式** | 同子网用 host-gw/BGP，跨子网用 VXLAN |
| **MTU** | VXLAN 设置为 1450，避免分片 |
| **IP 规划** | 预留足够的 Pod CIDR，避免与 VPC 冲突 |
| **NetworkPolicy** | 默认拒绝，显式允许 |
| **监控** | 监控 CNI Pod 状态、网络延迟 |

---

---

## 10. 生产环境CNI运维实践

### 10.1 CNI插件选型指南

```markdown
## CNI插件选型决策矩阵

| 场景 | 推荐插件 | 理由 | 注意事项 |
|------|----------|------|----------|
| **公有云环境** | Calico | 性能优秀，支持NetworkPolicy | 需要BGP配置优化 |
| **混合云环境** | Cilium | eBPF强大，安全功能丰富 | 内核版本要求≥4.19 |
| **阿里云ACK** | Terway | 阿里云原生优化 | 仅适用于ACK环境 |
| **简单网络需求** | Flannel | 配置简单，易于维护 | 不支持NetworkPolicy |
| **企业级安全** | Calico | 企业级安全策略 | 需要额外学习成本 |

### 性能基准测试结果
- **Calico VXLAN模式**: 8.2 Gbps 吞吐量
- **Cilium eBPF模式**: 12.1 Gbps 吞吐量  
- **Flannel VXLAN模式**: 6.8 Gbps 吞吐量
- **Terway ENI模式**: 25 Gbps 吞吐量（接近物理网卡极限）
```

### 10.2 CNI高可用部署配置

```yaml
# Calico生产环境高可用配置
apiVersion: operator.tigera.io/v1
kind: Installation
metadata:
  name: default
spec:
  # 高可用配置
  calicoNetwork:
    ipPools:
    - cidr: 10.244.0.0/16
      encapsulation: VXLANCrossSubnet
      natOutgoing: Enabled
      blockSize: 26
    nodeAddressAutodetectionV4:
      # 自动检测主网络接口
      interface: "eth.*"
  
  # 组件资源配置
  componentResources:
  - componentName: Node
    resourceRequirements:
      requests:
        cpu: 100m
        memory: 128Mi
      limits:
        cpu: 500m
        memory: 512Mi
  
  # 高可用设置
  nodeMetricsPort: 9091
  typhaMetricsPort: 9093
  
---
# Typha高可用部署
apiVersion: apps/v1
kind: Deployment
metadata:
  name: calico-typha
  namespace: calico-system
spec:
  replicas: 3  # 至少3个副本保证高可用
  selector:
    matchLabels:
      k8s-app: calico-typha
  template:
    metadata:
      labels:
        k8s-app: calico-typha
    spec:
      affinity:
        podAntiAffinity:
          preferredDuringSchedulingIgnoredDuringExecution:
          - weight: 100
            podAffinityTerm:
              labelSelector:
                matchLabels:
                  k8s-app: calico-typha
              topologyKey: kubernetes.io/hostname
      containers:
      - name: calico-typha
        image: docker.io/calico/typha:v3.26.1
        env:
        - name: TYPHA_LOGSEVERITYSCREEN
          value: "info"
        - name: TYPHA_HEALTHENABLED
          value: "true"
        - name: TYPHA_HEALTHPORT
          value: "9098"
        readinessProbe:
          httpGet:
            path: /readiness
            port: 9098
          periodSeconds: 10
        livenessProbe:
          httpGet:
            path: /liveness
            port: 9098
          initialDelaySeconds: 30
          periodSeconds: 10
```

### 10.3 CNI故障诊断与恢复

```bash
#!/bin/bash
# cni-troubleshooting.sh - CNI故障诊断脚本

set -euo pipefail

echo "=== CNI故障诊断工具 ==="
echo "诊断时间: $(date)"
echo

# 1. CNI插件识别
echo "[1/7] 识别CNI插件..."
CNI_PLUGIN=""
if kubectl get pods -n kube-system | grep -q calico; then
  CNI_PLUGIN="calico"
  echo "检测到 Calico CNI"
elif kubectl get pods -n kube-system | grep -q cilium; then
  CNI_PLUGIN="cilium"
  echo "检测到 Cilium CNI"
elif kubectl get pods -n kube-system | grep -q flannel; then
  CNI_PLUGIN="flannel"
  echo "检测到 Flannel CNI"
elif kubectl get pods -n kube-system | grep -q terway; then
  CNI_PLUGIN="terway"
  echo "检测到 Terway CNI"
else
  echo "未识别到标准CNI插件"
  exit 1
fi

# 2. CNI配置检查
echo "[2/7] 检查CNI配置..."
{
  echo "=== CNI配置检查 ==="
  echo "CNI插件: ${CNI_PLUGIN}"
  echo "配置文件列表:"
  kubectl get nodes -o jsonpath='{range .items[*]}{.metadata.name}{"\n"}{end}' | \
    head -3 | while read node; do
      echo "--- Node: $node ---"
      kubectl debug node/$node -it --image=nicolaka/netshoot -- ls /etc/cni/net.d/ 2>/dev/null || echo "无法访问"
    done
} > /tmp/cni-config-check.log

# 3. CNI Pod状态检查
echo "[3/7] 检查CNI Pod状态..."
{
  echo "=== CNI Pod状态 ==="
  case $CNI_PLUGIN in
    "calico")
      kubectl get pods -n calico-system -o wide
      echo -e "\nCalico节点状态:"
      kubectl exec -n calico-system -l k8s-app=calico-node -- calicoctl node status 2>/dev/null || echo "calicoctl不可用"
      ;;
    "cilium")
      kubectl get pods -n kube-system -l k8s-app=cilium -o wide
      echo -e "\nCilium状态:"
      kubectl exec -n kube-system -l k8s-app=cilium -- cilium status 2>/dev/null || echo "cilium命令不可用"
      ;;
    "flannel")
      kubectl get pods -n kube-system -l app=flannel -o wide
      ;;
    "terway")
      kubectl get pods -n kube-system -l app=terway -o wide
      echo -e "\nTerway诊断:"
      kubectl exec -n kube-system -l app=terway -- terway-cli version 2>/dev/null || echo "terway-cli不可用"
      ;;
  esac
} > /tmp/cni-pod-status.log

# 4. 网络连通性测试
echo "[4/7] 测试网络连通性..."
{
  echo "=== 网络连通性测试 ==="
  
  # 创建测试Pod
  cat <<EOF | kubectl apply -f -
apiVersion: v1
kind: Pod
metadata:
  name: cni-test-pod-1
  labels:
    app: cni-test
spec:
  containers:
  - name: tester
    image: nicolaka/netshoot
    command: ["sleep", "3600"]
---
apiVersion: v1
kind: Pod
metadata:
  name: cni-test-pod-2
  labels:
    app: cni-test
spec:
  containers:
  - name: tester
    image: nicolaka/netshoot
    command: ["sleep", "3600"]
EOF

  sleep 10
  
  # 等待Pod就绪
  kubectl wait --for=condition=Ready pod/cni-test-pod-1 pod/cni-test-pod-2 --timeout=60s
  
  POD1_IP=$(kubectl get pod cni-test-pod-1 -o jsonpath='{.status.podIP}')
  POD2_IP=$(kubectl get pod cni-test-pod-2 -o jsonpath='{.status.podIP}')
  
  echo "测试Pod1 IP: $POD1_IP"
  echo "测试Pod2 IP: $POD2_IP"
  
  echo -e "\n跨Pod连通性测试:"
  kubectl exec cni-test-pod-1 -- ping -c 3 $POD2_IP
  
  echo -e "\nDNS解析测试:"
  kubectl exec cni-test-pod-1 -- nslookup kubernetes.default
  
  echo -e "\n外部网络测试:"
  kubectl exec cni-test-pod-1 -- ping -c 3 8.8.8.8
  
  # 清理测试Pod
  kubectl delete pod cni-test-pod-1 cni-test-pod-2 --force --grace-period=0
} > /tmp/cni-connectivity-test.log

# 5. 日志分析
echo "[5/7] 收集CNI日志..."
{
  echo "=== CNI日志分析 ==="
  case $CNI_PLUGIN in
    "calico")
      echo "Calico节点日志采样:"
      kubectl logs -n calico-system -l k8s-app=calico-node --tail=100 | grep -E "(error|warn|failed)" || echo "未发现明显错误"
      ;;
    "cilium")
      echo "Cilium代理日志采样:"
      kubectl logs -n kube-system -l k8s-app=cilium --tail=100 | grep -E "(level=error|level=warning)" || echo "未发现明显错误"
      ;;
    "flannel")
      echo "Flannel日志采样:"
      kubectl logs -n kube-system -l app=flannel --tail=100 | grep -E "(E[0-9]|W[0-9])" || echo "未发现明显错误"
      ;;
    "terway")
      echo "Terway日志采样:"
      kubectl logs -n kube-system -l app=terway --tail=100 | grep -E "(error|Error)" || echo "未发现明显错误"
      ;;
  esac
} > /tmp/cni-logs-analysis.log

# 6. 性能指标检查
echo "[6/7] 检查网络性能指标..."
{
  echo "=== 网络性能检查 ==="
  
  echo "节点网络接口状态:"
  kubectl get nodes -o jsonpath='{range .items[*]}{.metadata.name}{"\n"}{end}' | \
    head -3 | while read node; do
      echo "--- Node: $node ---"
      kubectl debug node/$node -it --image=nicolaka/netshoot -- \
        ss -i | grep -E "(eth|ens|eno)" | head -10
    done
  
  echo -e "\nconntrack使用情况:"
  kubectl get nodes -o jsonpath='{range .items[*]}{.metadata.name}{"\n"}{end}' | \
    head -3 | while read node; do
      echo "--- Node: $node ---"
      kubectl debug node/$node -it --image=nicolaka/netshoot -- \
        sh -c 'echo "current: $(cat /proc/sys/net/netfilter/nf_conntrack_count 2>/dev/null || echo N/A)" && echo "max: $(cat /proc/sys/net/netfilter/nf_conntrack_max 2>/dev/null || echo N/A)"'
    done
} > /tmp/cni-performance-check.log

# 7. 生成诊断报告
echo "[7/7] 生成诊断报告..."
{
  echo "=========================================="
  echo "           CNI插件诊断报告"
  echo "=========================================="
  echo "CNI插件类型: ${CNI_PLUGIN}"
  echo "诊断时间: $(date)"
  echo
  
  echo "=== 关键指标 ==="
  case $CNI_PLUGIN in
    "calico")
      echo "Calico节点数: $(kubectl get pods -n calico-system -l k8s-app=calico-node --no-headers | wc -l)"
      echo "Calico运行状态: $(kubectl get pods -n calico-system -l k8s-app=calico-node --no-headers | grep Running | wc -l)/$(kubectl get pods -n calico-system -l k8s-app=calico-node --no-headers | wc -l)"
      ;;
    "cilium")
      echo "Cilium节点数: $(kubectl get pods -n kube-system -l k8s-app=cilium --no-headers | wc -l)"
      echo "Cilium运行状态: $(kubectl get pods -n kube-system -l k8s-app=cilium --no-headers | grep Running | wc -l)/$(kubectl get pods -n kube-system -l k8s-app=cilium --no-headers | wc -l)"
      ;;
  esac
  
  echo
  echo "=== 发现的问题 ==="
  if grep -q "error\|Error\|failed\|Failed" /tmp/cni-logs-analysis.log; then
    echo "⚠️  发现CNI错误日志"
  fi
  
  if ! kubectl exec cni-test-pod-1 -- ping -c 1 $POD2_IP >/dev/null 2>&1; then
    echo "⚠️  Pod间网络连通性异常"
  fi
  
  echo
  echo "=== 建议操作 ==="
  echo "1. 详细日志已保存至临时文件"
  echo "2. 如需进一步分析，请查看对应日志文件"
  echo "3. 常见解决方案:"
  case $CNI_PLUGIN in
    "calico")
      echo "   - 重启calico-node Pod: kubectl delete pod -n calico-system -l k8s-app=calico-node"
      echo "   - 检查BGP peer状态: calicoctl node status"
      ;;
    "cilium")
      echo "   - 重启cilium-agent: kubectl delete pod -n kube-system -l k8s-app=cilium"
      echo "   - 检查eBPF状态: cilium status"
      ;;
    "flannel")
      echo "   - 重启kube-flannel-ds: kubectl delete pod -n kube-system -l app=flannel"
      echo "   - 检查子网分配: kubectl logs -n kube-system -l app=flannel"
      ;;
  esac
} > /tmp/cni-diagnosis-report.txt

echo
echo "=== 诊断完成 ==="
echo "诊断报告: /tmp/cni-diagnosis-report.txt"
echo "各检查项日志已分别保存"
```

### 10.4 CNI安全加固配置

```yaml
# CNI网络安全加固配置
apiVersion: v1
kind: ConfigMap
metadata:
  name: cni-security-config
  namespace: kube-system
data:
  # Calico安全配置
  calico-security.yaml: |
    apiVersion: projectcalico.org/v3
    kind: FelixConfiguration
    metadata:
      name: default
    spec:
      # 启用日志记录
      logSeverityScreen: Info
      
      # 网络策略强制执行
      policySyncPathPrefix: /var/run/nodeagent
      
      # 安全特性
      reportingInterval: 0s  # 禁用定期报告
      endpointReportingEnabled: true
      endpointReportingDelay: 1s
      
      # 资源限制
      healthPort: 9099
      prometheusMetricsEnabled: true
      
      # 安全扫描
      iptablesLockTimeoutSecs: 30
      iptablesRefreshInterval: 90
      
      # 防御配置
      failsafeInboundHostPorts:
      - protocol: TCP
        port: 22    # SSH
        net: 0.0.0.0/0
      - protocol: TCP
        port: 10250 # Kubelet
        net: 0.0.0.0/0
      failsafeOutboundHostPorts:
      - protocol: UDP
        port: 53    # DNS
        net: 0.0.0.0/0
      - protocol: TCP
        port: 53    # DNS
        net: 0.0.0.0/0
  
  # Cilium安全配置
  cilium-security.yaml: |
    apiVersion: v2
    kind: CiliumConfig
    metadata:
      name: cilium
    spec:
      # 安全选项
      enable-ipv4: true
      enable-ipv6: false
      tunnel: vxlan
      
      # 策略执行模式
      policy-enforcement-mode: "always"  # 强制执行策略
      
      # 监控和日志
      monitor-aggregation: "medium"
      monitor-aggregation-flags: "all"
      log-driver: "fluentd"
      
      # 资源限制
      bpf-map-dynamic-size-ratio: "0.0025"
      preallocate-bpf-maps: "false"
      
      # 安全特性
      enable-l7-proxy: "true"
      enable-remote-node-identity: "true"
      enable-bandwidth-manager: "true"
```

### 10.5 CNI升级与回滚策略

```markdown
## CNI插件升级最佳实践

### 升级前准备
1. **备份配置**: 备份当前CNI配置和网络策略
2. **容量评估**: 确保集群有足够的资源
3. **兼容性检查**: 验证新版本与Kubernetes版本兼容性
4. **测试环境验证**: 在测试环境先行验证

### 滚动升级步骤
```bash
# 1. 检查当前版本
kubectl get pods -n kube-system -l k8s-app=calico-node -o jsonpath='{.items[0].spec.containers[0].image}'

# 2. 应用新配置
kubectl apply -f calico-new-version.yaml

# 3. 监控升级过程
watch kubectl get pods -n kube-system -l k8s-app=calico-node

# 4. 验证网络功能
./network-validation-script.sh

# 5. 回滚机制（如有问题）
kubectl apply -f calico-old-version.yaml
```

### 回滚条件
- 升级过程中出现大量Pod网络异常
- 核心业务服务不可达
- 监控指标异常（CPU/内存使用率激增）
- 日志中出现严重错误信息

### 验证清单
- [ ] Pod间网络连通性正常
- [ ] Service访问正常
- [ ] DNS解析正常
- [ ] NetworkPolicy策略生效
- [ ] 监控指标稳定
- [ ] 无新增错误日志
```
