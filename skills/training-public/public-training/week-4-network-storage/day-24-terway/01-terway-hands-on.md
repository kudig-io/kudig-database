---
title: 'Day 24: Terway 网络实操'
description: '# Day 24: Terway 网络实操'
summary: 'kubectl get pod <pod-name> -o jsonpath='{.metadata.annotations.tencentvpc~2Fpod-ip}''
category: learning
tags:
- k8s
- training
- hands-on
- flannel
- calico
- helm
- daemonset
- ingress
- networkpolicy
- rag
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 'Day 24: Terway 网络实操 是什么'
- '如何 Day 24: Terway 网络实操'
trigger_keywords:
- Day
- '24:'
- Terway
- 网络实操
- learn
prerequisites:
- kubectl-basics
- gpu-ml-basics
- helm-basics
- ebpf-basics
- cilium-basics
- cni-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Day 24: Terway 网络实操

> **日期**: Week 4 Day 3 | **主题**: Terway CNI 架构与配置 | **版本**: K8s 1.28-1.33

---

## 1. Terway 核心概念

### 1.1 Terway vs Flannel

| 维度 | Terway | Flannel |
|------|--------|---------|
| 网络模型 | ENI + Trunk ENI | VXLAN / IPIP |
| Pod 数量 | 高密度（200+ Pod/节点） | 中等（100-150 Pod/节点） |
| Pod IP 来源 | 云厂商 ENI | overlay 网络 |
| 性能 | 高（原生网络） | 中等（隧道开销） |
| 安全组支持 | 完全支持 | 不支持 |

### 1.2 Terway 架构

```
Pod → Terway CNI → Veth Pair → Host Bridge → ENI (云网络)
                    ↓
              Terway Agent (daemonset)
                    ↓
              Metadata Service (获取 ENI 信息)
```

---

## 2. Terway 安装与配置

### 2.1 安装 Terway

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `helm upgrade/install`：部署/升级 release
> - `kubectl apply/create/replace`：创建/变更集群资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# Alibaba Cloud ACK
# 集群创建时选择 Terway 网络插件

# 自建集群手动安装
git clone https://github.com/Alibaba/terway.git
cd terway/deploy
kubectl apply -f terway.yaml

# 或使用 Helm
helm install terway -n kube-system ./charts/terway
```
### 2.2 Terway 配置

```yaml
# /etc/terway/config.json
{
  "version": "2",
  "TerwaySubnetCIDR": "10.42.0.0/16",
  "ENIMode": true,
  "ENIIPReserved": "10.42.0.1",
  "MaxPoolSize": 15,
  "MinPoolSize": 5,
  "PodVPPCIDR": "169.254.0.0/16"
}
```

---

## 3. Terway 网络诊断

### 3.1 状态检查

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 检查 Terway Pod
kubectl get pods -n kube-system -l app=terway

# 检查 Terway Agent 日志
kubectl logs -n kube-system -l app=terway --tail=50

# 查看节点网络接口
ip addr | grep -E "veth|eth"
```
### 3.2 Pod 网络问题排查

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 1. 检查 Pod 是否获得 ENI IP
kubectl get pod <pod-name> -o jsonpath='{.metadata.annotations.tencentvpc~2Fpod-ip}'

# 2. 查看 Pod 网络信息
kubectl exec -it <pod-name> -- ip addr
kubectl exec -it <pod-name> -- route -n

# 3. 测试连通性
kubectl exec -it <pod-name> -- ping -c 3 8.8.8.8
kubectl exec -it <pod-name> -- ping -c 3 <other-pod-ip>

# 4. 检查安全组
# 在云控制台检查 Pod 所属安全组是否允许流量
```
### 3.3 Terway 故障排查

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 1. Pod 无法分配 IP
# 检查 Terway Agent 日志
kubectl logs -n kube-system -l app=terway --tail=100 | grep -i "ip allocation"

# 检查 ENI 配额
# 云控制台查看 ENI 剩余数量

# 2. Pod 无法访问外部
# 检查路由表
ip route show
# 检查 NAT 网关配置

# 3. 安全组不生效
# Terway ENI 模式需要正确配置安全组规则
```
---

## 4. Terway 网络策略

### 4.1 [[NetworkPolicy|NetworkPolicy]] 配置

```yaml
# 限制 Pod 入站流量
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: api-network-policy
  namespace: production
spec:
  podSelector:
    matchLabels:
      app: api
  policyTypes:
    - Ingress
    - Egress
  ingress:
    - from:
        - podSelector:
            matchLabels:
              app: frontend
      ports:
        - protocol: TCP
          port: 8080
  egress:
    - to:
        - podSelector:
            matchLabels:
              app: database
      ports:
        - protocol: TCP
          port: 3306
    - to:
        - namespaceSelector: {}
          podSelector:
            matchLabels:
              k8s-app: kube-dns
      ports:
        - protocol: UDP
          port: 53
```

### 4.2 跨节点通信问题

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 检查 Terway ENI 模式
cat /etc/terway/config.json | grep ENIMode

# 检查 Trunk ENI 配置
kubectl get node <node-name> -o jsonpath='{.metadata.annotations.cni~2F Alibaba~2Ftrunk-eni}'

# 测试跨节点 Pod 通信
kubectl exec -it <pod-a> -- ping -c 3 <pod-b-ip>
```
---

## 5. Terway 与 Service

### 5.1 Terway 模式下 Service 访问

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# Terway ENI 模式下 Service 流量路径
# Pod → Terway CNI → Host Bridge → ENI → 云负载均衡 → Service

# 查看 Service Endpoints
kubectl get endpoints <service-name>

# 检查 kube-proxy 状态
kubectl get pods -n kube-system -l k8s-app=kube-proxy
```
---

## 6. Terway 性能优化

### 6.1 ENI 池管理

```bash
# 配置 ENI 池预分配（减少 Pod 创建延迟）
# 编辑 Terway 配置
{
  "PreFilter": true,
  "ENIPoolCrossAZ": false,
  "ENIIPPrefix": true
}
```

### 6.2 带宽控制

```yaml
# 通过 annotation 设置 Pod 带宽
apiVersion: v1
kind: Pod
metadata:
  name: bandwidth-limited-pod
  annotations:
    [[entities/kubernetes.md|kubernetes]].io/ingress-bandwidth: "10M"
    kubernetes.io/egress-bandwidth: "10M"
spec:
  containers:
    - name: app
      image: app:v1
```

---

## 7. 实战练习

**练习 1**: 检查 Terway Agent 日志，验证 Pod IP 分配成功

**练习 2**: 配置 NetworkPolicy，限制 API Pod 只允许前端 Pod 访问

**练习 3**: 模拟 Pod 无法分配 IP 的场景，排查 ENI 配额问题

**练习 4**: 验证跨节点 Pod 通信，测试 Terway ENI 模式网络性能

---

```yaml
---
id: LEARN-WEEK4-DAY24
title: Day 24 - Terway 网络实操
topic: network-storage
type: hands-on-guide
tags: [terway, cni, eni, networkpolicy, troubleshooting, hands-on, k8s-1.28-1.33]
last_updated: 2026-05-18
difficulty: advanced
intent_queries:
  - "Terway 网络怎么配置"
  - "Terway Pod IP 分配失败怎么排查"
  - "Terway NetworkPolicy 怎么用"
  - "ENI 模式怎么工作"
  - "阿里云 Terway CNI"
trigger_keywords:
  - Terway
  - ENI
  - CNI
  - Trunk ENI
  - Pod IP
  - 安全组
  - 带宽控制
  - NetworkPolicy
  - ACK 网络
  - 阿里云容器服务
reading_level: advanced
audience:
  - sre
  - ops-engineer
estimated_read_time: 35min
related_domains:
  - 故障诊断
  - 网络
related_topics:
  - networking
  - cni
  - terway
  - networkpolicy
related:
  - 生产运维/topic-learn/public-training/week-4-network-storage/day-25-flannel/01-flannel-hands-on.md
  - 故障诊断/topic-fta/list/calico-fta.md
---
```

---

## 自测题 (Self-Check)

**1. ClusterIP 如何实现?**

<details><summary>答案</summary>

kube-proxy 通过 iptables/IPVS 将 ClusterIP DNAT 到后端 PodIP:TargetPort。

</details>

**2. Ingress vs Gateway API?**

<details><summary>答案</summary>

Ingress 仅 HTTP, 需注解扩展; Gateway API 支持 HTTP/gRPC/TCP, 原生流量分割, 角色分离。

</details>

**3. StatefulSet 稳定网络标识原理?**

<details><summary>答案</summary>

Pod 名 <sts>-<ordinal> + Headless Service → DNS <pod>.<svc>.<ns>.svc.cluster.local。

</details>

**4. 如何选 CNI?**

<details><summary>答案</summary>

Calico (通用 BGP/VXLAN) / Cilium (eBPF 高性能) / Flannel (简单无 Policy)。生产推荐 Cilium 或 Calico。

</details>

**5. PVC 三种访问模式?**

<details><summary>答案</summary>

ReadWriteOnce (单节点 RW) / ReadOnlyMany (多节点 RO) / ReadWriteMany (多节点 RW)。

</details>


```

<!-- risk-assessed -->
