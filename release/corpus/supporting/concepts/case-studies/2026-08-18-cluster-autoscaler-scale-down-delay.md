---
title: '[2026-08-18] [P1] Cluster Autoscaler 缩容导致节点驱逐延迟'
summary: '[2026-08-18] [P1] Cluster Autoscaler 缩容导致节点驱逐延迟：03:20，夜间低峰期，Cluster Autoscaler
  决定缩容节点 ip-10-0-9-15.ec2.internal。节点被标记为 ToBeDeletedByClusterAutoscaler，但节点上的 Pod
  迟迟未被驱逐。'
category: case-study
tags:
- production
- incident
- cluster-fundamentals
- autoscaling
- node
- eviction
tier: supporting
created: '2026-05-23'
updated: '2026-05-23'
date: '2026-08-18'
severity: P1
mttr: 33min
status: resolved
last_updated: 2026-05-23
---



# [2026-08-18] Cluster Autoscaler 缩容导致节点上 Pod 驱逐延迟 8 分钟

## 工单信息
- **工单编号**: INC-2026-0818-018
- **发现时间**: 2026-08-18 03:20 UTC
- **恢复时间**: 2026-08-18 03:53 UTC
- **影响范围**: `ip-10-0-9-15.ec2.internal` 节点上的 23 个 Pod
- **业务影响**: 节点被缩容后，Pod 在 8 分钟内未被驱逐，期间新 Pod 无法调度，部分服务降级

## 问题现象
03:20，夜间低峰期，Cluster Autoscaler 决定缩容节点 `ip-10-0-9-15.ec2.internal`。节点被标记为 `ToBeDeletedByClusterAutoscaler`，但节点上的 Pod 迟迟未被驱逐。

```bash
kubectl get node ip-10-0-9-15.ec2.internal
# NAME                        STATUS                     ROLES    AGE
# ip-10-0-9-15.ec2.internal   Ready,SchedulingDisabled   <none>   3d

kubectl get pods -n prod-api --field-selector spec.nodeName=ip-10-0-9-15.ec2.internal
# NAME              READY   STATUS    RESTARTS   AGE
# api-abc12         1/1     Running   0          45m
# worker-def34      1/1     Running   0          45m
# ...（共 23 个 Pod，均未进入 Terminating）
```

## 诊断过程

**03:22** — 检查 Cluster Autoscaler 日志：
```bash
kubectl logs -n kube-system deployment/cluster-autoscaler | grep ip-10-0-9-15
# I0818 03:15:45.112 ... scale_down.go:456] 
#   Scale-down: removing node ip-10-0-9-15.ec2.internal
# I0818 03:15:45.113 ... scale_down.go:460] 
#   Waiting for pod eviction on ip-10-0-9-15.ec2.internal
# I0818 03:23:45.114 ... scale_down.go:460] 
#   Still waiting for pod eviction on ip-10-0-9-15.ec2.internal
```

**03:25** — 检查节点上的 Pod，发现大量 `PodDisruptionBudget` 限制：
```bash
kubectl get pdb -A
# NAMESPACE   NAME        MIN AVAILABLE   MAX UNAVAILABLE   ALLOWED DISRUPTIONS
# prod-api    api-pdb     4               N/A               0
# prod-order  order-pdb   3               N/A               0
```

**03:27** — 检查具体 PDB：
```bash
kubectl get pdb api-pdb -n prod-api -o yaml
# spec:
#   minAvailable: 4
#   selector:
#     matchLabels:
#       app: api
# status:
#   currentHealthy: 4
#   desiredHealthy: 4
#   disruptionsAllowed: 0
```

`api-pdb` 要求 `minAvailable: 4`，当前恰好 4 个 healthy。若驱逐节点上的 1 个 api Pod，`currentHealthy` 将降至 3 < 4，因此 PDB 拒绝驱逐。

**03:29** — 检查 api Deployment 的副本数：
```bash
kubectl get deployment api -n prod-api
# NAME   READY   UP-TO-DATE   AVAILABLE
# api    4/4     4            4
```

Deployment 的 replicas 恰好等于 PDB 的 minAvailable，没有冗余副本。

**03:31** — 检查 Cluster Autoscaler 配置：
```bash
kubectl get deployment cluster-autoscaler -n kube-system -o yaml | grep -A5 args
# args:
# - --scale-down-delay-after-add=10m
# - --scale-down-unneeded-time=5m
# - --skip-nodes-with-system-pods=false
# - --skip-nodes-with-local-storage=false
# （未配置 --scale-down-delay-after-delete）
```

## 根因
1. `api` Deployment 的 replicas=4，PDB minAvailable=4，无冗余
2. Cluster Autoscaler 缩容时，需要驱逐节点上的 Pod
3. 但驱逐任何一个 api Pod 都会违反 PDB（disruptionsAllowed=0）
4. Cluster Autoscaler 等待 PDB 允许驱逐，但 Deployment 没有多余副本
5. 同时，被标记为 `SchedulingDisabled` 的节点无法接受新 Pod，导致其他 namespace 的 Pending Pod 无法调度
6. 缩容过程延迟 8 分钟，期间集群可用容量降低

## 修复动作

**03:35** — 临时增加 api Deployment 副本数：
```bash
kubectl scale deployment api -n prod-api --replicas=6
# 新 Pod 调度到其他节点
kubectl get pods -n prod-api -l app=api
# NAME              READY   STATUS
# api-xxx-abc12     1/1     Running
# api-xxx-def34     1/1     Running
# ...（共 6 个 Running）
```

**03:38** — PDB 允许驱逐：
```bash
kubectl get pdb api-pdb -n prod-api
# NAME     MIN AVAILABLE   ALLOWED DISRUPTIONS
# api-pdb  4               2
```

**03:40** — Cluster Autoscaler 继续缩容：
```bash
kubectl get pods -n prod-api --field-selector spec.nodeName=ip-10-0-9-15.ec2.internal
# （Pod 开始 Terminating）

kubectl get node ip-10-0-9-15.ec2.internal
# （节点已删除）
```

**03:45** — 恢复 api Deployment 到正常副本数：
```bash
kubectl scale deployment api -n prod-api --replicas=4
```

## 验证
- 03:48 — 节点缩容完成，集群节点数恢复正常
- 03:50 — 所有 Pod 正常运行，无 Pending
- 03:53 — 业务指标正常，无服务降级

## 复盘
- **直接原因**: PDB minAvailable = Deployment replicas → 无冗余 → Cluster Autoscaler 无法驱逐 Pod → 缩容延迟
- **根本原因**: PDB 配置过于严格，未考虑缩容场景；Deployment 副本数未预留缩容缓冲
- **改进措施**:
  1. **PDB 配置原则**: `minAvailable < replicas`，或设置 `maxUnavailable: 1`
  2. Cluster Autoscaler 添加 `--scale-down-delay-after-delete=5m`，给驱逐更多时间
  3. 添加告警：`cluster_autoscaler_nodes_count < expected_nodes` 持续 > 10min
  4. 缩容前自动检查 PDB：`kubectl get pdb -A | awk '$4 == 0 {print}'`
  5. 关键服务 Deployment replicas ≥ PDB minAvailable + 2（缩容冗余）
- **相关 Skill**: [[ts-cluster-operations]]
- **相关 FTA**: [[cluster-autoscaler-fta]]
