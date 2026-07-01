---
title: 节点驱逐与维护
description: '## 概述'
category: skills
tags:
- k8s
- drain
- cordon
- uncordon
- eviction
- pdb
- node-maintenance
- etcd
- kubelet
- redis
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 节点驱逐与维护 是什么
- 如何 节点驱逐与维护
trigger_keywords:
- 节点驱逐与维护
prerequisites:
- kubectl-basics
- etcd-basics
- redis-basics
created: "2026-05-23"
---

# 节点驱逐与维护

## 概述

`kubectl drain` 是安全驱逐节点上所有 Pod 的标准操作，用于节点维护、升级或下线。drain 操作分为两个阶段：先将节点标记为不可调度（cordon），然后驱逐该节点上的 Pod。

## drain 完整流程

```
步骤 1: 获取目标节点 → 根据节点名或 label selector 查找
步骤 2: Cordon 节点  → 设置 node.spec.unschedulable = true
步骤 3: 获取节点上所有 Pod → fieldSelector: spec.nodeName=<node>
步骤 4: 过滤 Pod
  → 跳过 DaemonSet Pod（如果 --ignore-daemonsets）
  → 跳过 mirror Pod（静态 Pod 镜像）
  → 跳过已终止 Pod（Succeeded/Failed）
  → 检查 emptyDir 卷（需 --delete-emptydir-data）
步骤 5: 驱逐 Pod（eviction API）
  → API Server 检查 PodDisruptionBudget
  → 如果 PDB 允许，发送 SIGTERM 给容器
  → 等待 gracePeriodSeconds 后强制终止
步骤 6: 等待所有 Pod 终止 → 如果有 Pod 驱逐失败（PDB），等待 5 秒重试
步骤 7: 返回结果 → 打印 "node/<node> drained"
```

## Eviction API vs Delete API

| 维度 | Eviction API（推荐） | Delete API（不推荐） |
|------|---------------------|-------------------|
| 是否检查 PDB | 是 | 否 |
| 优雅终止 | 是（SIGTERM） | 是（但不尊重 PDB） |
| PDB 拒绝时 | 返回 429，等待重试 | 直接删除 |
| 安全性 | 高 | 低 |

## 关键参数

| 参数 | 说明 | 默认值 |
|------|------|--------|
| `--ignore-daemonsets` | 忽略 DaemonSet Pod | 必须设置，否则拒绝 drain |
| `--delete-emptydir-data` | 允许删除 emptyDir 数据 | 必须显式设置 |
| `--grace-period` | 优雅终止宽限期（秒） | -1（使用 Pod 默认值） |
| `--timeout` | drain 超时时间 | 0（无限等待） |
| `--disable-eviction` | 使用 delete 而非 eviction | false |
| `--force` | 强制驱逐无控制器的 Pod | false |

## 标准操作流程

### 节点维护（内核升级）

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `kubectl drain`：驱逐节点所有 Pod，业务流量受影响

```bash
# 1. 驱逐 Pod
kubectl drain node-1 \
  --delete-emptydir-data \
  --ignore-daemonsets

# 2. 执行维护
ssh node-1 "apt-get update && apt-get upgrade -y linux-image-generic"
ssh node-1 "reboot"

# 3. 等待节点恢复
kubectl wait --for=condition=Ready node/node-1 --timeout=300s

# 4. 恢复调度
kubectl uncordon node-1
```

### 集群升级时逐个 drain

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `kubectl cordon`：标记节点不可调度
> - `kubectl drain`：驱逐节点所有 Pod，业务流量受影响
> - `systemctl stop/restart`：停止/重启系统服务，影响节点上所有容器

```bash
for node in $(kubectl get nodes -l node-role.kubernetes.io/worker -o name); do
    kubectl cordon $node
    kubectl drain $node \
      --delete-emptydir-data \
      --ignore-daemonsets \
      --timeout=120s
    # 升级 kubelet
    ssh ${node#node/} "apt-get install -y kubelet=1.32.0"
    ssh ${node#node/} "systemctl restart kubelet"
    kubectl uncordon $node
    kubectl wait --for=condition=Ready $node --timeout=120s
done
```

### PodDisruptionBudget 配置

```yaml
# 最少可用副本数
apiVersion: policy/v1
kind: PodDisruptionBudget
metadata:
  name: api-server-pdb
spec:
  minAvailable: 2
  selector:
    matchLabels:
      app: api-server

# 最大不可用副本数
apiVersion: policy/v1
kind: PodDisruptionBudget
spec:
  maxUnavailable: 1
  selector:
    matchLabels:
      app: redis-cache
```

## 常见错误

| 错误 | 原因 | 解决方案 |
|------|------|---------|
| `DaemonSet Pod found but --ignore-daemonsets not set` | 未设置忽略 DaemonSet | 添加 `--ignore-daemonsets` |
| `Cannot evict pod as it would violate PDB` | PDB 阻止驱逐 | 增加 Pod 副本数，等待新 Pod 就绪后重试 |
| `pod has emptyDir volume but --delete-emptydir-data not set` | Pod 使用 emptyDir 卷 | 添加 `--delete-emptydir-data` |
| `[[Pods|pods]] not managed by RC/RS/Job/StatefulSet` | Pod 无控制器管理 | 添加 `--force` 强制删除 |
| `drain hung` | Pod 容器忽略 SIGTERM | 检查应用信号处理，使用 `--grace-period=0` |

## 相关技能

- [[skills/kubeadm-cluster-deletion.md|[[kubeadm 集群删除操作|kubeadm 集群删除操作]]]]
- [[skills/kubelet-eviction-mechanism.md|[[kubelet 资源驱逐机制|kubelet 资源驱逐机制]]]]
- [[skills/backup-restore-etcd.md|备份和恢复 etcd]]
- [[concepts/resource-management.md|资源管理]]

## Related

- [[skills/kubeadm-cluster-deletion.md|kubeadm-cluster-deletion]] — kubeadm 集群删除操作
- [[entities/statefulset.md|[[StatefulSet|statefulset]]]] — StatefulSet
- [[entities/kubelet.md|kubelet]] — kubelet
- [[etcd]] — etcd
- [[kubernetes]] — Kubernetes (CNCF Graduated)

```