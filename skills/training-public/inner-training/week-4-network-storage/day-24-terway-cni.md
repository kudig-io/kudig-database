---
title: 'Day 24: Terway 网络'
description: '# Day 24: Terway 网络'
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
- 'Day 24: Terway 网络 是什么'
- '如何 Day 24: Terway 网络'
trigger_keywords:
- Day
- '24:'
- Terway
- 网络
- learn
prerequisites:
- kubectl-basics
- gpu-ml-basics
created: "2026-05-23"
---

# Day 24: Terway 网络

```yaml
---
title: Day 24: Terway 网络
last_updated: 2026-05-18
difficulty: intermediate
intent_queries:
  - "Terway CNI"
  - "阿里云网络插件"
  - "Terway ENIIP模式"
  - "Kubernetes CNI"
  - "Pod网络配置"
trigger_keywords:
  - "Terway"
  - "Terway CNI"
  - "ENIIP"
  - "ENI模式"
  - "弹性网卡"
  - "VPC网络"
  - "NetworkPolicy"
  - "CNI插件"
reading_level: intermediate
audience:
  - sre工程师
  - ops工程师
  - 运维工程师
estimated_read_time: 45min
related_domains:
  - domain-03-networking-traffic
  - domain-10-troubleshooting-diagnostics
related_topics:
  - domain-11-production-operations/topic-learn/inner-training/week-4-network-storage/day-25-flannel-cni
  - domain-11-production-operations/topic-learn/inner-training/week-4-network-storage/day-23-ingress
  - domain-03-networking-traffic/02-cni-architecture-fundamentals
id: WEEK4-DAY24
topic: training
type: hands-on
tags: [week-4, day-24, terway, cni, networking, k8s, k8s-1.28-1.33]
---
```

> **学习时间**: 4-5 小时 | **主题**: Terway CNI 架构与配置

---

## 今日目标

- [ ] 理解 Terway CNI 的整体架构
- [ ] 掌握 Terway 的三种模式 (VPC / ENI / ENIIP)
- [ ] 能够查看和排查 Terway 网络配置
- [ ] 了解 Terway 与 VPC 网络的集成原理

---

## 理论学习 (2h)

### 必读文档

1. **ACK 网络方案**
   - 文件: `../../../domain-12-cloud-providers/04-alicloud-ack/260-ack-networking.md`
   - 重点: Terway 架构、模式对比

2. **K8S 网络模型**
   - 文件: `../../../domain-06-service-networking/04-cni.md`
   - 重点: CNI 规范、Pod 网络要求

3. **VPC 网络基础**
   - 文件: `../../../domain-12-cloud-providers/04-alicloud-ack/200-ack-overview.md`
   - 重点: VPC/vSwitch CIDR 与集群网络规划

### 阅读要点

- **Terway** 是阿里云自研的 CNI 插件，深度集成 VPC 弹性网卡 (ENI)
- **VPC 模式**: Pod 使用 VPC 路由，类似 Flannel，性能一般
- **ENI 模式**: 每个 Pod 独占一块弹性网卡，性能最优但 ENI 数量有限
- **ENIIP 模式** (推荐): Pod 使用 ENI 的辅助 IP，兼顾性能和密度
- **NetworkPolicy**: Terway 原生支持 K8S NetworkPolicy
- **Pod IP 直通**: Terway ENIIP 模式下 Pod IP 是 VPC 内可路由的
- **IP 规划**: 需要提前规划 Pod vSwitch CIDR，确保 IP 充足

---

## 实践任务 (2.5h)

### 任务 1: 确认 Terway 部署与模式 (30min)

```bash
# 检查 Terway 组件状态
kubectl get ds -n kube-system terway-eniip 2>/dev/null || \
kubectl get ds -n kube-system terway 2>/dev/null || \
echo "未安装 Terway"

# 查看 Terway Pod 状态
kubectl get pods -n kube-system | grep terway

# 查看 Terway 配置 (确认运行模式)
kubectl get configmap -n kube-system eni-config -o yaml 2>/dev/null

# 查看节点上的 ENI 信息
kubectl get nodes -o wide
kubectl describe node <node-name> | grep -A 5 "Allocatable" | grep -i eni
```

### 任务 2: Pod 网络验证 (ENIIP 模式) (40min)

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

```bash
# 创建测试 Pod
kubectl run terway-test-1 --image=registry.cn-hangzhou.aliyuncs.com/acs-sample/busybox:1.36 \
  --command -- sleep 3600
kubectl run terway-test-2 --image=registry.cn-hangzhou.aliyuncs.com/acs-sample/busybox:1.36 \
  --command -- sleep 3600

# 等待 Pod 就绪
kubectl wait --for=condition=Ready pod/terway-test-1 pod/terway-test-2 --timeout=60s

# 查看 Pod IP (ENIIP 模式下应为 VPC 子网 IP)
kubectl get pod -o wide

# Pod 间通信测试
POD2_IP=$(kubectl get pod terway-test-2 -o jsonpath='{.status.podIP}')
kubectl exec terway-test-1 -- ping -c 3 ${POD2_IP}

# 验证 Pod IP 属于 VPC 网段
echo "Pod IP: ${POD2_IP}"
echo "检查此 IP 是否属于 Pod vSwitch CIDR"

# 从 Pod 内访问外部
kubectl exec terway-test-1 -- wget -qO- --timeout=5 http://100.100.100.200/latest/meta-data/instance-id 2>/dev/null || echo "非 ECS 环境"
```

### 任务 3: NetworkPolicy 实践 (40min)

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

```bash
# 创建命名空间隔离
kubectl create namespace np-test
kubectl run web --namespace=np-test \
  --image=registry.cn-hangzhou.aliyuncs.com/acs-sample/nginx:1.24 \
  --labels="app=web" --expose --port=80

# 测试默认可访问
kubectl run test --namespace=np-test \
  --image=registry.cn-hangzhou.aliyuncs.com/acs-sample/busybox:1.36 \
  --rm -it --restart=Never -- wget -qO- --timeout=5 http://web

# 创建 deny-all NetworkPolicy
cat <<EOF | kubectl apply -f -
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: deny-all
  namespace: np-test
spec:
  podSelector: {}
  policyTypes:
  - Ingress
EOF

# 测试: 应该无法访问
kubectl run test2 --namespace=np-test \
  --image=registry.cn-hangzhou.aliyuncs.com/acs-sample/busybox:1.36 \
  --rm -it --restart=Never -- wget -qO- --timeout=5 http://web 2>&1 || echo "连接超时 - NetworkPolicy 生效"

# 允许特定标签访问
cat <<EOF | kubectl apply -f -
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: allow-app
  namespace: np-test
spec:
  podSelector:
    matchLabels:
      app: web
  ingress:
  - from:
    - podSelector:
        matchLabels:
          access: "true"
  policyTypes:
  - Ingress
EOF

# 带标签的 Pod 可以访问
kubectl run test3 --namespace=np-test --labels="access=true" \
  --image=registry.cn-hangzhou.aliyuncs.com/acs-sample/busybox:1.36 \
  --rm -it --restart=Never -- wget -qO- --timeout=5 http://web
```

### 任务 4: Terway 排障 (30min)

> ⚠️ **🔴 灾难性操作** — 含不可逆命令，执行前必须满足变更窗口+双人复核+事前备份+回滚方案
> - `kubectl delete namespace`：永久删除命名空间及全部资源，不可恢复
> - `kubectl delete`：删除资源（可由声明式清单重建）
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

```bash
# 查看 Terway 日志
kubectl logs -n kube-system $(kubectl get pod -n kube-system -l app=terway-eniip -o jsonpath='{.items[0].metadata.name}') -c terway --tail=30

# 检查节点 ENI 使用量
aliyun ecs DescribeNetworkInterfaces --RegionId cn-hangzhou \
  --InstanceId <instance-id> 2>/dev/null | head -30

# 查看 Pod 的网络接口信息
kubectl exec terway-test-1 -- ip addr
kubectl exec terway-test-1 -- ip route

# 清理
kubectl delete pod terway-test-1 terway-test-2
kubectl delete namespace np-test  # ⚠️ 不可逆：永久删除命名空间及全部资源
```

---

## 费曼复述 (0.5h)

1. **Terway ENIIP 模式的工作原理是什么？Pod 如何获取 IP？**
2. **Terway 相比 Flannel 最大的优势是什么？又有哪些限制？**
3. **NetworkPolicy 只在 Terway 下支持，Flannel 不支持 — 这是为什么？**

---

## 今日检验

- [ ] 能查看 Terway 运行模式和组件状态
- [ ] 理解 ENIIP 模式的 Pod 网络原理
- [ ] 能使用 NetworkPolicy 实现网络隔离
- [ ] 了解 Terway 常见问题排查方法

---

## 核心概念总结

| Terway 模式 | Pod IP 来源 | 性能 | Pod 密度 | 适用场景 |
|-------------|------------|------|---------|---------|
| VPC 模式 | VPC 路由分配 | 一般 | 高 | 低性能要求场景 |
| ENI 模式 | 独占弹性网卡 | 最优 | 低 (受 ENI 配额限制) | 高性能/安全隔离 |
| ENIIP 模式 | ENI 辅助 IP | 优 | 中 | **推荐默认选择** |

---

## 明日预告

Day 25 将学习 Flannel CNI 方案，与 Terway 进行对比。

## Related

- index/terway-index|Terway 知识图谱索引]]
