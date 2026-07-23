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

## 生产案例

### 案例1: Flannel VXLAN 跨节点不通

**时间线**:
- 16:00 新节点加入集群，Flannel Pod 启动正常
- 16:05 新节点上的 Pod 无法与其他节点 Pod 通信
- 16:10 检查发现 flannel.1 接口 MTU 为 1450，但节点网卡 MTU 为 1500
- 16:15 确认根因：云厂商 VPC 已封装 VXLAN，叠加 Flannel VXLAN 导致双重封装
- 16:20 切换为 host-gw 模式，跨节点通信恢复

**根因链**:
```
云VPC已启用VXLAN封装 → Flannel再次VXLAN封装 → 双重封装超过MTU
→ 大包被丢弃 → 跨节点Pod通信失败(小包正常)
```

**修复**:
```bash
# 🟡 切换 Flannel 后端模式为 host-gw
kubectl edit configmap kube-flannel-cfg -n kube-system
# 修改 "Type": "vxlan" → "Type": "host-gw"
# 🟡 重启 Flannel
kubectl rollout restart daemonset kube-flannel-ds -n kube-system
# 🟢 验证跨节点连通性
kubectl run test --rm -it --image=busybox -- ping ${REMOTE_POD_IP}
```

### 案例2: Flannel Pod CrashLoopBackOff

**现象**: Flannel Pod 反复重启，日志显示 `failed to acquire lease: etcdserver: request timed out`

**根因**: etcd 集群负载过高，Flannel 无法获取子网 lease

**修复**:
```bash
# 🟢 检查 etcd 健康状态
kubectl exec -n kube-system etcd-master-0 -- etcdctl endpoint health
# 🟡 如 etcd 压力大，考虑切换 Flannel 后端为 kubernetes (使用 K8s API 而非 etcd)
kubectl edit configmap kube-flannel-cfg -n kube-system
# "Backend": {"Type": "kubernetes"}
```

## 预防与监控

### 告警规则

```yaml
groups:
- name: flannel-alerts
  rules:
  - alert: FlannelPodNotReady
    expr: kube_pod_status_ready{condition="true", namespace="kube-system"} * on(pod) group_left() (kube_pod_labels{label_app="flannel"}) == 0
    for: 5m
    labels:
      severity: critical
  - alert: FlannelSubnetExhausted
    expr: flannel_subnet_count / flannel_subnet_max > 0.9
    for: 10m
    labels:
      severity: warning
```

### 预防措施

| 措施 | 说明 | 优先级 |
|------|------|--------|
| MTU 规划 | 云环境预留封装开销，VXLAN 减 50 | P0 |
| 后端模式选择 | 同子网用 host-gw，跨子网用 VXLAN | P0 |
| 子网容量规划 | Pod CIDR 足够大，避免子网耗尽 | P1 |
| Flannel 资源限制 | 设置合理的 CPU/Memory requests | P1 |

## 面试要点

1. **Q: Flannel 三种后端模式的区别？**
   A: UDP(已废弃，用户态转发慢) / VXLAN(内核态封装，跨子网) / host-gw(直接路由，同子网性能最佳)

2. **Q: Flannel 与 Calico 的核心差异？**
   A: Flannel 纯 Overlay 无网络策略；Calico 支持 NetworkPolicy + BGP 路由；大规模集群 Calico BGP 性能更优

3. **Q: Flannel 跨节点不通的排查步骤？**
   A: 检查 flannel.1 接口状态 → 验证 FDB/ARP 表 → 测试 MTU(ping -M do -s 1400) → 检查节点防火墙(UDP 8472) → 确认子网分配无冲突

## 相关链接

- [[技能/fta-方法论/methodology/FTA Methodology and Core Principles.md|FTA 方法论]]
- [[技能/fta-方法论/execution-engine/FTA Diagnostic Execution Engine.md|[[FTA 诊断执行引擎|FTA 诊断执行引擎]]]]
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
