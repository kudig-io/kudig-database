---
title: 'Day 25: Flannel 网络实操'
description: '# Day 25: Flannel 网络实操'
summary: 'kubectl apply -f https://raw.githubusercontent.com/flannel-io/flannel/master/Documentation/kube-flannel.yml'
category: learning
tags:
- k8s
- training
- hands-on
- etcd
- apiserver
- kubelet
- flannel
- calico
- daemonset
- networkpolicy
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 'Day 25: Flannel 网络实操 是什么'
- '如何 Day 25: Flannel 网络实操'
trigger_keywords:
- Day
- '25:'
- Flannel
- 网络实操
- learn
prerequisites:
- kubectl-basics
- gpu-ml-basics
- ebpf-basics
- cilium-basics
- cni-basics
- etcd-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Day 25: Flannel 网络实操

> **日期**: Week 4 Day 4 | **主题**: Flannel 网络模型与故障排查 | **版本**: K8s 1.28-1.33

---

## 1. Flannel 核心概念

### 1.1 Flannel 模式

| 模式 | 说明 | 适用场景 |
|------|------|---------|
| VXLAN | 二层隧道（推荐） | 跨节点 Pod 通信 |
| host-gw | 主机网关（需二层可达） | 同机房低延迟 |
| IPIP | IP 隧道 | 跨网络通信 |
| WireGuard | 加密隧道 | 安全要求高 |

### 1.2 Flannel 架构

```
Pod A (10.244.1.2) → cni0 (10.244.1.1) → flannel.1 (VXLAN) → eth0 → Node B
                    ↓
              flanneld (daemonset)
                    ↓
              etcd (网络分配存储)
```

---

## 2. 安装 Flannel

### 2.1 kubeadm 集群安装 Flannel

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 安装 CNI 插件
kubectl apply -f https://raw.githubusercontent.com/flannel-io/flannel/master/Documentation/kube-flannel.yml

# 检查 Pod
kubectl get pods -n kube-flannel

# 确认节点有 flannel 接口
ip addr | grep flannel
```
### 2.2 自定义 CIDR

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

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
# 通过 kubeadm 配置 Pod CIDR
kubeadm init --pod-network-cidr=10.244.0.0/16

# 或通过 ConfigMap 修改
kubectl edit configmap -n kube-system kubelet-config

# 修改 Flannel 配置
kubectl edit configmap -n kube-flannel kube-flannel-cfg
```
---

## 3. Flannel 网络诊断

### 3.1 状态检查

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 检查 Flannel Pod
kubectl get pods -n kube-flannel -o wide

# 查看 Flannel 日志
kubectl logs -n kube-flannel -l app=flannel --tail=50

# 检查 Flannel 接口
ip addr | grep flannel

# 检查 Flannel 网络信息
cat /var/run/flannel/subnet.env
```
### 3.2 Pod 间通信故障排查

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 1. 检查 Flannel 接口是否存在
ip addr show flannel.1

# 2. 检查 Flannel 路由
ip route show

# 3. 查看节点 subnet 分配
kubectl get nodes -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{.spec.podCIDR}{"\n"}'

# 4. 检查 etcd 中的网络配置
etcdctl get /coreos.com/network/subnets
```
---

## 4. Flannel 问题场景

### 4.1 Pod 无法跨节点通信

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl delete`：删除资源（可由声明式清单重建）

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 1. 检查 Flannel 是否在所有节点运行
kubectl get pods -n kube-flannel -o wide

# 2. 检查 VXLAN 端口
ss -tlnp | grep 8472

# 3. 测试节点间连通性
ping -c 3 <other-node-ip>

# 4. 检查 iptables 规则
iptables -L -n -t nat | grep flannel

# 5. 重启 Flannel
kubectl delete pod -n kube-flannel -l app=flannel
```
### 4.2 Flannel 启动失败

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 1. 检查 Flannel 日志
kubectl logs -n kube-flannel -l app=flannel --previous

# 2. 检查 etcd 连接
kubectl exec -it -n kube-flannel flannel-xxx -- etcdctl get /coreos.com/network/config

# 3. 确认 kube-apiserver 可达
kubectl exec -it -n kube-flannel flannel-xxx -- curl -sk https://kubernetes.default.svc.cluster.local/healthz
```
### 4.3 网络分段问题

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 1. 检查 Pod CIDR 分配
kubectl get pods -o wide | awk '{print $1, $2, $NF}' | head -20

# 2. 查看 Flannel subnet 列表
kubectl get nodes -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{.status.addresses[?(@.type=="InternalIP")].address}{"\t"}{.spec.podCIDR}{"\n"}'

# 3. 检查路由表
route -n | grep flannel
```
---

## 5. Flannel 与 [[NetworkPolicy|NetworkPolicy]]

### 5.1 Calico 安装（替代 Flannel）

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 如需 NetworkPolicy，使用 Calico
kubectl apply -f https://docs.projectcalico.org/manifests/calico.yaml

# 检查 Calico 组件
kubectl get pods -n kube-system -l k8s-app=calico-node
```
### 5.2 Flannel + Calico 混合

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 安装 Flannel（基础网络）
kubectl apply -f https://raw.githubusercontent.com/flannel-io/flannel/master/Documentation/kube-flannel.yml

# 安装 Calico（策略）
kubectl apply -f https://docs.projectcalico.org/v3.25/manifests/calico-policy-only.yaml
```
---

## 6. Flannel 性能调优

### 6.1 VXLAN 参数调优

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

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
# 检查当前 MTU
ip link show flannel.1 | grep mtu

# 降低 MTU（避免分片）
ip link set flannel.1 mtu 1400

# 持久化 MTU 配置
# 在 Flannel ConfigMap 中设置
kubectl edit configmap -n kube-flannel kube-flannel-cfg
# 添加: "Backend": {"Type": "vxlan", "VNI": 1, "MTU": 1400}
```
### 6.2 网络延迟优化

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 检查 VXLAN 性能
# 在两个节点上分别运行
kubectl exec -it <pod-a> -- iperf3 -s &
kubectl exec -it <pod-b> -- iperf3 -c <pod-a-ip>
```
---

## 7. 实战练习

**练习 1**: 安装 Flannel，验证 Pod 跨节点通信正常

**练习 2**: 使用 tcpdump 抓包分析 VXLAN 隧道流量

**练习 3**: 模拟 Flannel Pod 无法启动，排查 etcd 连接问题

**练习 4**: 配置 Flannel MTU 优化，减少网络分片

---

```yaml
---
id: LEARN-WEEK4-DAY25
title: Day 25 - Flannel 网络实操
topic: network-storage
type: hands-on-guide
tags: [flannel, cni, vxlan, network, troubleshooting, hands-on, k8s-1.28-1.33]
last_updated: 2026-05-18
difficulty: intermediate
intent_queries:
  - "Flannel 网络怎么安装"
  - "VXLAN 隧道怎么工作"
  - "Flannel 问题怎么排查"
  - "Pod 跨节点通信失败怎么办"
  - "Flannel 网络原理"
trigger_keywords:
  - Flannel
  - CNI
  - VXLAN
  - host-gw
  - IPIP
  - WireGuard
  - 网络插件
  - Pod CIDR
  - etcd
  - flanneld
  - cni0
  - 网络诊断
reading_level: intermediate
audience:
  - sre
  - ops-engineer
estimated_read_time: 35min
related_domains:
  - domain-10-troubleshooting-diagnostics
  - domain-03-networking-traffic
related_topics:
  - networking
  - cni
  - flannel
  - vxlan
related:
  - domain-11-production-operations/topic-learn/public-training/week-4-network-storage/day-24-terway/01-terway-hands-on.md
  - domain-10-troubleshooting-diagnostics/topic-fta/list/calico-fta.md
---
```

---

## 自测题 (Self-Check)

**1. ClusterIP 如何实现?**

<details><summary>答案</summary>

kube-proxy 通过 iptables/IPVS 将 ClusterIP DNAT 到后端 PodIP:TargetPort。

</details>

**2. [[Ingress|Ingress]] vs Gateway API?**

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


## Related

- [[domain-19-landscape-references/topic-index/terway-index.md|Terway 知识图谱索引]]
- [[domain-19-landscape-references/topic-index/flannel-index.md|Flannel 知识图谱索引]]

```

<!-- risk-assessed -->
