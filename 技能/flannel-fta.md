---
title: Flannel 网络异常故障树分析 (skills)
description: '# Flannel 网络异常故障树分析'
summary: '# Flannel 网络异常故障树分析'
category: skills
tags:
- k8s
- fta
- troubleshooting
- etcd
- flannel
- daemonset
tier: supporting
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Flannel 网络异常故障树分析 是什么
- 如何 Flannel 网络异常故障树分析
trigger_keywords:
- Flannel
- 网络异常故障树分析
prerequisites:
- kubectl-basics
- etcd-basics
fta_id: FTA-FLANNEL-001
component: Flannel
severity: high
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Flannel 网络异常故障树分析

### 故障排查命令速查

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
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

## 生产案例

### 案例 1: Flannel VXLAN 封包导致 MTU 不匹配引发间歇性超时

| 时间 | 事件 |
|------|------|
| 11:00 | 部分大报文请求超时，小报文正常 |
| 11:10 | `ping -s 1472 pod-ip` 失败，`ping -s 1400` 成功 |
| 11:15 | 确认 VXLAN 封包开销 50 bytes，Pod MTU 应为 1450 |
| 11:20 | 🟡 修改 Flannel 配置 `--iface-mtu=1450`，重启 flannel DaemonSet |

**根因**: 节点 MTU 1500，VXLAN 封包后实际 MTU 1450，Pod 未设置 MTU 导致大包被丢弃。

### 案例 2: flannel.1 接口丢失导致节点 Pod 网络全断

**现象**: 单节点上所有 Pod 无法通信，`ip link show flannel.1` 接口不存在。

**诊断**: `journalctl -u kube-flannel` 显示 "failed to create vxlan interface"

**修复**: 🔴 重启 flannel Pod: `kubectl delete pod -n kube-system -l app=flannel --field-selector spec.nodeName=<node>`

## 升级决策点

| 级别 | 条件 | 动作 |
|------|------|------|
| P0 | 多节点 Pod 网络全断 | 立即检查 flannel DaemonSet 状态 |
| P1 | 单节点网络异常 | 重启该节点 flannel Pod |
| P2 | MTU 相关间歇性问题 | 调整 MTU 配置 |

## 面试要点

1. **Q: Flannel 的三种后端模式有何区别？**
   A: VXLAN(默认): 内核态封包，性能好；host-gw: 直接路由，无封包开销但要求二层互通；UDP(已废弃): 用户态封包，性能最差。生产推荐 VXLAN 或 host-gw。

2. **Q: Flannel 的 IP 分配机制是怎样的？**
   A: flanneld 从配置的 Network CIDR(如 10.244.0.0/16) 中为每个节点分配一个 /24 子网，存储在 etcd 或 K8s Subnet CRD 中，确保跨节点不重叠。

3. **Q: Flannel 与 Calico 的主要差异？**
   A: Flannel 纯 Overlay 网络，无网络策略支持；Calico 支持 BGP 路由(无封包) + NetworkPolicy + 安全组，功能更丰富但复杂度更高。

## 相关链接

- [[技能/FTA Methodology and Core Principles.md|FTA 方法论]]
- [[技能/FTA Diagnostic Execution Engine.md|[[FTA 诊断执行引擎|FTA 诊断执行引擎]]]]
- networking.md|网络故障排查]]

## Related

- [[nodepool-fta]] — [[技能/nodepool-fta.md|[[NodePool 异常故障树分析|NodePool 异常故障树分析]]]]
- [[技能/ts-control-plane.md|ts-control-plane]] — 控制平面故障排查
- [[README]] — FTA 故障树清单索引
- [[技能/ts-networking.md|ts-networking]] — 网络故障排查
- [[etcd]] — etcd

- [[故障诊断/FTA故障树/list/flannel-fta.md|Flannel 网络异常故障树分析]]
- [[技能/ts-command-output.md|命令输出根因解析]] — Cross-reference
- [[技能/skill-22-daemonset-failure.md|DaemonSet 故障诊断与修复 / DaemonSet Failure Diagnosis & Remediation]] — Cross-reference
- [[生态参考/领域索引/flannel-index.md|Flannel 知识图谱索引]]


<!-- risk-assessed -->
