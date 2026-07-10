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
tier: core
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

## 相关链接

- [[skills/FTA Methodology and Core Principles.md|FTA 方法论]]
- [[skills/FTA Diagnostic Execution Engine.md|[[FTA 诊断执行引擎|FTA 诊断执行引擎]]]]
- networking.md|网络故障排查]]

## Related

- [[nodepool-fta]] — [[skills/nodepool-fta.md|[[NodePool 异常故障树分析|NodePool 异常故障树分析]]]]
- [[skills/ts-control-plane.md|ts-control-plane]] — 控制平面故障排查
- [[README]] — FTA 故障树清单索引
- [[skills/ts-networking.md|ts-networking]] — 网络故障排查
- [[etcd]] — etcd

- [[故障诊断/topic-fta/list/flannel-fta.md|Flannel 网络异常故障树分析]]
- [[skills/ts-command-output.md|命令输出根因解析]] — Cross-reference
- [[skills/skill-22-daemonset-failure.md|DaemonSet 故障诊断与修复 / DaemonSet Failure Diagnosis & Remediation]] — Cross-reference
- [[生态参考/topic-index/flannel-index.md|Flannel 知识图谱索引]]


<!-- risk-assessed -->
