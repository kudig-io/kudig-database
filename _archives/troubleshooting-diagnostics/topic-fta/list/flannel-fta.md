---
title: Flannel 网络异常故障树分析
description: Flannel CNI 网络插件异常故障树分析，覆盖 VXLAN/host-gw 模式、跨主机通信、IP 分配等故障路径
category: fta
tags:
- fta
- troubleshooting
- flannel
- cni
- network
- vxlan
- host-gw
- kubelet
- pod
- rag
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 网络工程师
estimated_read_time: 5min
intent_queries:
- Flannel 网络异常故障树分析 是什么
- Flannel VXLAN 故障 根因分析
- Flannel 跨主机通信 故障树
trigger_keywords:
- Flannel
- 网络异常故障树分析
- fta
- VXLAN
- host-gw
- CNI
prerequisites:
- kubectl-basics
- troubleshooting-methodology
- etcd-basics
fta_id: FTA-FLANNEL-001
component: Flannel
severity: critical
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
- type: structural
  path: ../domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/03-networking/08-flannel-troubleshooting.md
  label: '结构化排障: 08-flannel-troubleshooting'
- type: index
  path: ../../domain-19-landscape-references/topic-index/flannel-index.md
  label: '索引文档: flannel-index'
---

# Flannel 网络异常 FTA 树

## 适用范围与说明
- **目标**：覆盖 Flannel CNI 网络插件在生产环境中的网络连通性问题。
- **范围**：VXLAN 封装、host-gw 路由、跨节点通信、IP 分配、子网冲突。
- **符号**：
  - **OR 门**：任一子事件成立即可触发父事件
  - **AND 门**：所有子事件同时成立才触发父事件

---

## Mermaid FTA 树

```mermaid
flowchart TD
  TE[顶事件: Flannel 网络异常]
  OR0{{OR}}
  TE --> OR0

  OR0 --> VNET[VXLAN 封装异常]
  OR0 --> ROUTE[路由异常]
  OR0 --> SUBNET[子网/IP 分配异常]
  OR0 --> BACKEND[后端存储异常]
  OR0 --> CONFIG[配置异常]

  %% VXLAN 分支
  VNET_OR{{OR}}
  VNET --> VNET_OR
  VNET_OR --> VNET1[UDP 端口 8472 被防火墙拦截]
  VNET_OR --> VNET2[flannel.1 接口状态异常]
  VNET_OR --> VNET3[MTU 问题导致分片]
  VNET_OR --> VNET4[overlay 封装丢失]

  %% 路由分支
  ROUTE_OR{{OR}}
  ROUTE --> ROUTE_OR
  ROUTE_OR --> ROUTE1[host-gw 模式下 ARP 表缺失]
  ROUTE_OR --> ROUTE2[节点路由表损坏]
  ROUTE_OR --> ROUTE3[跨网段路由不可达]

  %% 子网分配分支
  SUBNET_OR{{OR}}
  SUBNET --> SUBNET_OR
  SUBNET_OR --> SUBNET1[Pod IP 冲突]
  SUBNET_OR --> SUBNET2[CIDR 耗尽]
  SUBNET_OR --> SUBNET3[分配延迟导致 Pod Pending]

  %% 后端存储分支
  BACKEND_OR{{OR}}
  BACKEND --> BACKEND_OR
  BACKEND_OR --> BACKEND1[etcd 连接失败]
  BACKEND_OR --> BACKEND2[网络信息同步失败]

  %% 配置分支
  CONFIG_OR{{OR}}
  CONFIG --> CONFIG_OR
  CONFIG_OR --> CONFIG1[ConfigMap 配置错误]
  CONFIG_OR --> CONFIG2[版本不兼容]

  style TE fill:#ff6b6b,stroke:#c92a2a,color:#fff
  style VNET fill:#fbbf24,stroke:#d97706,color:#000
  style ROUTE fill:#fbbf24,stroke:#d97706,color:#000
  style SUBNET fill:#fbbf24,stroke:#d97706,color:#000
```

---

## 常见故障场景

### 场景 1: Pod 无法跨节点通信

**顶事件**: Pod A (Node-1) 无法访问 Pod B (Node-2)

```
诊断路径:
1. 检查 flannel.1 接口状态
   - ip addr show flannel.1
   - 状态应为 UP，IP 应为子网网关

2. 检查 VXLAN 端口
   - netstat -ulnp | grep 8472
   - 确认 UDP 端口未被防火墙拦截

3. 检查路由表
   - ip route show
   - 确认到目标 Pod 子网的路由指向 flannel.1

4. 检查 ARP 表 (host-gw 模式)
   - ip neigh show
   - 确认目标节点 MAC 地址存在

5. 测试 MTU
   - ping -M do -s 1400 <target-pod-ip>
   - MTU 问题通常表现为大包丢包
```

### 场景 2: Pod IP 分配失败

**顶事件**: Pod 启动卡在 Pending，Events 显示 "Failed to allocate IP"

```
诊断路径:
1. 检查 Flannel ConfigMap
   kubectl get configmap -n kube-system flannel -o yaml

2. 检查 etcd 中存储的网络信息
   etcdctl get /coreos.com/network/subnets

3. 检查是否有 IP 池耗尽
   kubectl get nodes -o wide
   检查每个节点的 Pod 数量

4. 检查是否有残留的 stale 资源
   kubectl get pods -A | grep -v Running | grep -v Completed
```

### 场景 3: Flannel DaemonSet 不正常

**顶事件**: flannel Pod 持续重启或处于 CrashLoopBackOff

```
诊断路径:
1. 检查 flannel 日志
   kubectl logs -n kube-system -l app=flannel --tail=100

2. 检查 etcd 连接
   kubectl exec -n kube-system <flannel-pod> -- etcdctl endpoint health

3. 检查 kubeconfig 权限
   kubectl auth can-i get pods --as=system:serviceaccount:kube-system:flannel

4. 检查 CNI 配置
   cat /etc/cni/net.d/10-flannel.conflist
```

---

## 故障排查命令速查

```bash
# 1. 检查 flannel 接口状态
ip addr show flannel.1
ip link show flannel.1

# 2. 检查 flannel 路由
ip route show | grep flannel

# 3. 检查 VXLAN 端口
netstat -ulnp | grep 8472

# 4. 检查 etcd 中的子网信息
etcdctl get /coreos.com/network/subnets --prefix

# 5. 检查 flannel ConfigMap
kubectl get configmap -n kube-system flannel -o yaml

# 6. 检查 flannel DaemonSet 状态
kubectl get pods -n kube-system -l app=flannel

# 7. 测试跨节点连通性
ping -I flannel.1 <target-pod-ip>
traceroute -i flannel.1 <target-pod-ip>

# 8. 检查 ARP 表 (host-gw)
ip neigh show | grep flannel

# 9. MTU 测试
ping -M do -s 1400 <target-ip>
```

---

## 相关文档

- [Flannel 完全指南](./domain-03-networking-traffic/04-flannel-complete-guide.md)
- [Flannel 故障排查](./domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/03-networking/08-flannel-troubleshooting.md)
- [CNI 网络故障排查](./domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/03-networking/01-cni-troubleshooting.md)
- [Flannel 全局索引](./domain-19-landscape-references/topic-index/flannel-index.md)

## Related

- [[skills/ts-command-output|命令输出根因解析]] — Cross-reference
- [[skills/skill-22-daemonset-failure|DaemonSet 故障诊断与修复 / DaemonSet Failure Diagnosis & Remediation]] — Cross-reference
- [[domain-19-landscape-references/topic-index/flannel-index|Flannel 知识图谱索引]]
