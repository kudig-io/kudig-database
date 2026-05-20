---
title: 'Day 25: Flannel 网络'
description: '# Day 25: Flannel 网络'
category: learning
tags:
- k8s
- training
- hands-on
- flannel
- ingress
- networkpolicy
- rag
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 'Day 25: Flannel 网络 是什么'
- '如何 Day 25: Flannel 网络'
trigger_keywords:
- Day
- '25:'
- Flannel
- 网络
- learn
---

# Day 25: Flannel 网络

```yaml
---
title: Day 25: Flannel 网络
last_updated: 2026-05-18
difficulty: intermediate
intent_queries:
  - "Flannel CNI"
  - "VxLAN"
  - "Kubernetes网络"
  - "Flannel配置"
  - "Pod CIDR"
trigger_keywords:
  - "Flannel"
  - "Flannel CNI"
  - "VxLAN"
  - "Overlay网络"
  - "Pod CIDR"
  - "网络插件"
  - "Flannel配置"
reading_level: intermediate
audience:
  - sre工程师
  - ops工程师
  - 运维工程师
estimated_read_time: 45min
related_domains:
  - domain-5-networking
  - domain-12-troubleshooting
related_topics:
  - topic-learn/inner-training/week-4-network-storage/day-24-terway-cni
  - topic-learn/inner-training/week-4-network-storage/day-23-ingress
  - domain-5-networking/02-cni-architecture-fundamentals
id: WEEK4-DAY25
topic: training
type: hands-on
tags: [week-4, day-25, flannel, cni, networking, k8s, k8s-1.28-1.33]
---
```

> **学习时间**: 4-5 小时 | **主题**: Flannel 网络模型与故障排查

---

## 今日目标

- [ ] 理解 Flannel CNI 的架构与 VxLAN 模式
- [ ] 掌握 Flannel 网络下的 Pod CIDR 分配
- [ ] 能排查 Flannel 网络常见问题
- [ ] 对比 Terway 与 Flannel 的适用场景

---

## 理论学习 (2h)

### 必读文档

1. **ACK 网络方案对比**
   - 文件: `../../../domain-17-cloud-provider/04-alicloud-ack/260-ack-networking.md`
   - 重点: Flannel vs Terway 架构差异

2. **Flannel 网络原理**
   - 文件: `../../../domain-06-service-networking/04-cni.md`
   - 重点: VxLAN 封装、Pod CIDR 分配

3. **网络故障排查**
   - 文件: `../../../domain-12-troubleshooting/10-network-troubleshooting.md`
   - 重点: 网络分层排查方法

### 阅读要点

- **Flannel** 使用 Overlay 网络 (VxLAN)，在节点间建立隧道
- **Pod CIDR**: 每个节点分配一个子网 (如 /25 或 /26)，Pod 从中获取 IP
- **VxLAN 封装**: Pod 跨节点通信时，数据包会被封装后转发
- **性能**: 由于封装开销，Flannel 性能略低于 Terway ENIIP
- **不支持 NetworkPolicy**: Flannel 不原生支持 K8S NetworkPolicy
- **优势**: 部署简单、IP 消耗少 (Pod IP 不占用 VPC 地址段)
- **限制**: Pod IP 在 VPC 外不可直接路由

---

## 实践任务 (2.5h)

### 任务 1: Flannel 组件检查 (30min)

```bash
# 检查 Flannel 组件 (如集群使用 Flannel)
kubectl get ds -n kube-system kube-flannel-ds 2>/dev/null || echo "未安装 Flannel"

# 查看 Flannel Pod 状态
kubectl get pods -n kube-system | grep flannel

# 查看 Flannel 配置
kubectl get configmap -n kube-system kube-flannel-cfg -o yaml 2>/dev/null

# 查看节点分配的 Pod CIDR
kubectl get nodes -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{.spec.podCIDR}{"\n"}{end}'

# 查看集群 CIDR 配置
kubectl cluster-info dump | grep -m 1 "cluster-cidr" 2>/dev/null
```

### 任务 2: Flannel 网络连通性验证 (40min)

```bash
# 在不同节点创建测试 Pod
cat <<EOF | kubectl apply -f -
apiVersion: v1
kind: Pod
metadata:
  name: flannel-test-1
spec:
  containers:
  - name: test
    image: registry.cn-hangzhou.aliyuncs.com/acs-sample/busybox:1.36
    command: ['sleep', '3600']
---
apiVersion: v1
kind: Pod
metadata:
  name: flannel-test-2
  labels:
    test: flannel
spec:
  containers:
  - name: test
    image: registry.cn-hangzhou.aliyuncs.com/acs-sample/busybox:1.36
    command: ['sleep', '3600']
EOF

# 查看 Pod 所在节点
kubectl get pod -o wide

# 同节点通信测试
POD1_IP=$(kubectl get pod flannel-test-1 -o jsonpath='{.status.podIP}')
POD2_IP=$(kubectl get pod flannel-test-2 -o jsonpath='{.status.podIP}')

kubectl exec flannel-test-1 -- ping -c 3 ${POD2_IP}

# 查看 Pod 内的网络信息
kubectl exec flannel-test-1 -- ip addr
kubectl exec flannel-test-1 -- ip route

# Pod 到 Service 通信
kubectl exec flannel-test-1 -- wget -qO- --timeout=5 http://kubernetes.default.svc.cluster.local/version 2>/dev/null || echo "需要认证"

# DNS 解析测试
kubectl exec flannel-test-1 -- nslookup kubernetes.default
```

### 任务 3: VxLAN 封装分析 (40min)

```bash
# 查看节点上的 flannel.1 接口 (需要 exec 到节点或使用特权 Pod)
cat <<EOF | kubectl apply -f -
apiVersion: v1
kind: Pod
metadata:
  name: net-debug
spec:
  hostNetwork: true
  containers:
  - name: debug
    image: registry.cn-hangzhou.aliyuncs.com/acs-sample/busybox:1.36
    command: ['sleep', '3600']
    securityContext:
      privileged: true
EOF

# 在节点网络查看 flannel 接口
kubectl exec net-debug -- ip addr show flannel.1 2>/dev/null || echo "非 Flannel 集群"

# 查看 ARP 表 (VxLAN 对端)
kubectl exec net-debug -- ip neigh show dev flannel.1 2>/dev/null

# 查看 FDB 转发表
kubectl exec net-debug -- bridge fdb show dev flannel.1 2>/dev/null | head -10

# 路由表分析
kubectl exec net-debug -- ip route | grep flannel 2>/dev/null
```

### 任务 4: Terway vs Flannel 对比实验 (30min)

```bash
# 网络性能对比参考 (概念理解)
# Flannel 特点:
echo "=== Flannel 网络特点 ==="
echo "1. Pod CIDR: 集群独立网段，不消耗 VPC IP"
echo "2. 跨节点: VxLAN 封装，有 50 字节头部开销"
echo "3. NetworkPolicy: 不支持"
echo "4. Pod IP 可路由性: 仅集群内可路由"
echo ""
echo "=== Terway ENIIP 网络特点 ==="
echo "1. Pod CIDR: 使用 VPC vSwitch IP，消耗 VPC 地址"
echo "2. 跨节点: VPC 原生路由，无封装开销"
echo "3. NetworkPolicy: 原生支持"
echo "4. Pod IP 可路由性: VPC 内全局可路由"

# 清理
kubectl delete pod flannel-test-1 flannel-test-2 net-debug
```

---

## 费曼复述 (0.5h)

1. **Flannel VxLAN 模式中，跨节点的 Pod 通信数据包经过了哪些路径？**
2. **Flannel 的 Pod CIDR 分配机制是怎样的？每个节点分到的网段如何确定？**
3. **什么场景下应该选择 Flannel 而不是 Terway？**

---

## 今日检验

- [ ] 能查看 Flannel 组件状态和配置
- [ ] 理解 VxLAN 封装的基本原理
- [ ] 能验证 Flannel 网络下的 Pod 连通性
- [ ] 能清晰对比 Terway 和 Flannel 的适用场景

---

## 核心概念总结

| 对比项 | Terway (ENIIP) | Flannel (VxLAN) |
|--------|---------------|----------------|
| Pod IP 来源 | VPC vSwitch IP | 集群独立 CIDR |
| 跨节点通信 | VPC 原生路由 | VxLAN 封装 |
| 性能 | 优 | 一般 (封装开销) |
| NetworkPolicy | 支持 | 不支持 |
| IP 消耗 | 消耗 VPC 地址 | 不消耗 VPC 地址 |
| 复杂度 | 需规划 Pod vSwitch | 简单 |
| 适用集群 | 中大型生产集群 | 小型/测试集群 |
| VPC 内直通 | 支持 | 不支持 |

---

## 明日预告

Day 26 将学习存储卷 (PV/PVC) 的创建与删除。
