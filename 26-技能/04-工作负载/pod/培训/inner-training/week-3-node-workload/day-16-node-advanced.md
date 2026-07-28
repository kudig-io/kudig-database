---
title: 'Day 16: Node 节点进阶'
description: '**学习时间**: 4-5 小时 | **主题**: 节点维护、标签与调度约束'
summary: '**学习时间**: 4-5 小时 | **主题**: 节点维护、标签与调度约束'
category: learning
tags:
- k8s
- training
- hands-on
- controller-manager
- containerd
- docker
- pdb
- daemonset
- operator
- gpu
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 'Day 16: Node 节点进阶 是什么'
- '如何 Day 16: Node 节点进阶'
trigger_keywords:
- Day
- '16:'
- Node
- 节点进阶
- learn
prerequisites:
- kubectl-basics
- gpu-ml-basics
- gpu-scheduling-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




---
title: Day 16: Node 节点进阶
last_updated: 2026-05-18
difficulty: intermediate
intent_queries:
  - [[23-实体/kubernetes.md|[[kubernetes|kubernetes]]]] node labels management
  - Node taint toleration mechanism
  - Node maintenance cordon drain uncordon
  - Kubernetes node scheduling constraints
  - ACK node maintenance operations
trigger_keywords:
  - node labels
  - 节点标签
  - taint
  - 污点
  - toleration
  - 容忍
  - cordon
  - drain
  - uncordon
  - maintenance
  - 维护
  - nodeSelector
  - nodeAffinity
reading_level: intermediate
audience:
  - ACK operators
  - SRE engineers
  - Platform engineers
estimated_read_time: 45min
related_domains:
  - domain-3-node
  - 故障诊断
  - 云厂商
related_topics:
  - node-basics
  - node-management
  - pod-scheduling-strategies
---

# Day 16: Node 节点进阶

> **学习时间**: 4-5 小时 | **主题**: 节点维护、标签与调度约束

---

## 概述

节点（Node）是 Kubernetes 集群中运行工作负载的基础设施单元。在生产环境中，节点的日常维护、标签管理和调度约束配置是运维工程师最频繁操作的任务之一。无论是进行节点硬件升级、内核补丁安装，还是实现工作负载的精细化调度，都需要深入理解节点的各种管理机制。

本课程将深入探讨节点标签（Labels）的管理和用途、污点（Taints）和容忍（Tolerations）的协作机制、节点维护的标准操作流程（cordon/drain/uncordon），以及 ACK 平台的节点运维操作。通过大量实践操作，你将掌握节点管理的核心技能。

**学习目标**：
- 掌握节点标签（Labels）的管理和用途
- 理解污点（Taints）和容忍（Tolerations）机制
- 掌握 cordon/drain/uncordon 节点维护操作
- 了解节点维护的最佳实践

**前置条件**：
- 已完成 Day 15 的节点基础学习
- 了解 Pod 调度基本概念
- 有 kubectl 操作集群的能力

---

## 核心概念

### 节点标签 (Labels)

节点标签是键值对形式的元数据，用于标识节点的属性特征。Kubernetes 调度器可以使用这些标签来决定 Pod 的调度位置。

#### Kubernetes 内置节点标签

| 标签 | 说明 | 示例值 |
|------|------|--------|
| `kubernetes.io/os` | 操作系统 | linux, windows |
| `kubernetes.io/arch` | CPU 架构 | amd64, arm64 |
| `kubernetes.io/hostname` | 主机名 | node-worker-1 |
| `topology.kubernetes.io/zone` | 可用区 | cn-hangzhou-a |
| `topology.kubernetes.io/region` | 地域 | cn-hangzhou |
| `node.kubernetes.io/instance-type` | 实例规格 | ecs.g7.xlarge |
| `node.kubernetes.io/os` | 操作系统类型 | linux |

#### 自定义标签命名规范

| 标签前缀 | 用途 | 示例 |
|----------|------|------|
| `env` | 环境标识 | `env=production`, `env=staging` |
| `team` | 团队归属 | `team=backend`, `team=data` |
| `tier` | 应用层级 | `tier=frontend`, `tier=backend` |
| `hardware` | 硬件特征 | `hardware=gpu`, `hardware=high-mem` |
| `role` | 节点角色 | `role=worker`, `role=compute` |

### 污点与容忍 (Taints & Tolerations)

污点和容忍是 Kubernetes 中实现节点专用化的核心机制。污点附加在节点上"排斥" Pod，容忍附加在 Pod 上"容忍"污点。

#### 污点效果 (Taint Effect)

| Effect | 行为 | 调度器 | 已有 Pod | 使用场景 |
|--------|------|--------|---------|---------|
| **NoSchedule** | 不调度新 Pod | 不调度 | 不影响 | GPU 节点、专用节点 |
| **PreferNoSchedule** | 尽量不调度 | 尽量避免 | 不影响 | 软性隔离 |
| **NoExecute** | 不调度 + 驱逐 | 不调度 | 驱逐不能容忍的 Pod | 节点维护、故障隔离 |

#### 调度约束对比

| 约束类型 | 方向 | 作用 | 灵活性 |
|----------|------|------|--------|
| **nodeSelector** | Pod → Node | 简单的标签匹配 | 低（精确匹配） |
| **nodeAffinity** | Pod → Node | 支持表达式和权重 | 中（In/NotIn/Exists） |
| **Taint/Toleration** | Node → Pod | 排斥不能容忍的 Pod | 中 |
| **PodAntiAffinity** | Pod → Pod | Pod 之间的排斥关系 | 高 |

### 节点维护流程

节点维护的标准流程分为四个阶段，每个阶段有明确的目标和操作：

```
cordon → drain → 维护 → uncordon
  │         │        │         │
  ▼         ▼        ▼         ▼
禁止调度  驱逐Pod   执行维护   恢复调度
```

---

## 实战演练

### 任务 1: 节点标签管理 (45min)

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源
> - `kubectl label/annotate`：改元数据可能影响选择器/控制器

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# Step 1: 查看所有节点标签
kubectl get nodes --show-labels

# 预期输出:
# NAME            STATUS   ROLES    AGE   VERSION   LABELS
# node-worker-1   Ready    worker   30d   v1.30.1   beta.kubernetes.io/arch=amd64,beta.kubernetes.io/os=linux,...
# node-worker-2   Ready    worker   30d   v1.30.1   ...

# Step 2: 按标签过滤节点
kubectl get nodes -l kubernetes.io/os=linux
kubectl get nodes -l topology.kubernetes.io/zone=cn-hangzhou-a

# Step 3: 添加自定义标签
kubectl label node node-worker-1 env=production
kubectl label node node-worker-1 team=backend
kubectl label node node-worker-1 tier=compute

# 预期输出:
# node/node-worker-1 labeled

# Step 4: 批量添加标签
kubectl label node node-worker-2 env=staging tier=compute

# Step 5: 查看特定节点的标签
kubectl describe node node-worker-1 | grep -A 20 "Labels"

# 预期输出:
# Labels:             beta.kubernetes.io/arch=amd64
#                     env=production
#                     team=backend
#                     tier=compute
#                     topology.kubernetes.io/zone=cn-hangzhou-a

# Step 6: 使用 nodeSelector 调度
cat > label-pod.yaml << 'EOF'
apiVersion: v1
kind: Pod
metadata:
  name: label-test
  namespace: default
spec:
  nodeSelector:
    env: production
  containers:
  - name: nginx
    image: nginx:1.25-alpine
    resources:
      requests:
        cpu: 50m
        memory: 64Mi
      limits:
        cpu: 100m
        memory: 128Mi
EOF

kubectl apply -f label-pod.yaml

# 预期输出:
# pod/label-test created

# Step 7: 验证 Pod 调度到了正确的节点
kubectl get pod label-test -o wide

# 预期输出:
# NAME         READY   STATUS    RESTARTS   AGE   IP            NODE
# label-test   1/1     Running   0          30s   10.0.1.100   node-worker-1

# Step 8: 使用 nodeAffinity（更灵活的调度）
cat > affinity-pod.yaml << 'EOF'
apiVersion: v1
kind: Pod
metadata:
  name: affinity-test
spec:
  affinity:
    nodeAffinity:
      requiredDuringSchedulingIgnoredDuringExecution:
        nodeSelectorTerms:
        - matchExpressions:
          - key: topology.kubernetes.io/zone
            operator: In
            values:
            - cn-hangzhou-a
            - cn-hangzhou-b
      preferredDuringSchedulingIgnoredDuringExecution:
      - weight: 80
        preference:
          matchExpressions:
          - key: env
            operator: In
            values:
            - production
  containers:
  - name: nginx
    image: nginx:1.25-alpine
EOF

kubectl apply -f affinity-pod.yaml

# Step 9: 删除标签
kubectl label node node-worker-1 env-
kubectl label node node-worker-1 team-

# 预期输出:
# node/node-worker-1 labeled
# node/node-worker-1 unlabeled
```
### 任务 2: 污点与容忍 (45min)

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `kubectl taint nodes`：变更污点影响 Pod 调度
> - `kubectl apply/create/replace`：创建/变更集群资源

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
# Step 1: 查看节点污点
kubectl describe node node-worker-1 | grep -A 5 Taints

# 预期输出:
# Taints: <none>
# 或:
# Taints: node-role.kubernetes.io/control-plane:NoSchedule

# Step 2: 添加污点（模拟 GPU 节点）
kubectl taint nodes node-worker-1 dedicated=gpu:NoSchedule

# 预期输出:
# node/node-worker-1 tainted

# Step 3: 尝试创建普通 Pod（应该无法调度）
kubectl run normal-pod --image=nginx:1.25-alpine

# Step 4: 查看调度状态
kubectl get pod normal-pod -o wide
# 预期输出: Pending（如果所有节点都有污点）

# Step 5: 创建带容忍的 Pod
cat > toleration-pod.yaml << 'EOF'
apiVersion: v1
kind: Pod
metadata:
  name: toleration-test
spec:
  tolerations:
  - key: "dedicated"
    operator: "Equal"
    value: "gpu"
    effect: "NoSchedule"
  containers:
  - name: nginx
    image: nginx:1.25-alpine
    resources:
      requests:
        cpu: 50m
        memory: 64Mi
      limits:
        cpu: 100m
        memory: 128Mi
EOF

kubectl apply -f toleration-pod.yaml

# 预期输出:
# pod/toleration-test created

# Step 6: 验证 Pod 调度到了有污点的节点
kubectl get pod toleration-test -o wide

# 预期输出:
# NAME              READY   STATUS    RESTARTS   AGE   IP            NODE
# toleration-test   1/1     Running   0          30s   10.0.1.101   node-worker-1

# Step 7: 测试 NoExecute 效果
kubectl taint nodes node-worker-2 test=noexecute:NoExecute

# 观察没有容忍的 Pod 会被驱逐
kubectl get pods -A -o wide | grep node-worker-2
# 不容忍此污点的 Pod 将被终止

# Step 8: 清理
kubectl taint nodes node-worker-1 dedicated=gpu:NoSchedule-
kubectl taint nodes node-worker-2 test=noexecute:NoExecute-

# 预期输出:
# node/node-worker-1 untainted
# node/node-worker-2 untainted
```
### 任务 3: 节点维护操作 (45min)

> ⚠️ **🔴 灾难性操作** — 含不可逆命令，执行前必须满足变更窗口+双人复核+事前备份+回滚方案
> - `docker prune/rm -f`：强制清理镜像/容器/卷，运行中容器会被杀
> - `kubectl cordon`：标记节点不可调度
> - `kubectl drain`：驱逐节点所有 Pod，业务流量受影响

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
# Step 1: 标记节点不可调度 (cordon)
kubectl cordon node-worker-1

# 预期输出:
# node/node-worker-1 cordoned

# Step 2: 验证节点状态
kubectl get nodes

# 预期输出:
# NAME            STATUS                     ROLES    AGE   VERSION
# node-worker-1   Ready,SchedulingDisabled   worker   30d   v1.30.1
# node-worker-2   Ready                      worker   30d   v1.30.1

# Step 3: 尝试在新节点调度 Pod（应该失败）
kubectl run test-cordon --image=nginx:1.25-alpine
# 如果所有其他节点资源不足，Pod 会 Pending

# Step 4: 排水节点 (drain)
kubectl drain node-worker-1 \
  --ignore-daemonsets \
  --delete-emptydir-data \
  --timeout=300s \
  --grace-period=60

# 预期输出:
# node/node-worker-1 already cordoned
# WARNING: ignoring DaemonSet-managed Pods...
# evicting pod default/label-test
# evicting pod default/toleration-test
# pod/label-test evicted
# pod/toleration-test evicted
# node/node-worker-1 drained

# Step 5: 确认 Pod 已迁移
kubectl get pods -A -o wide | grep node-worker-1
# 应该只剩 DaemonSet 管理的 Pod (kube-proxy, terway 等)

# Step 6: 执行维护操作 (模拟)
echo "=== 执行节点维护 ==="
# 常见维护操作:
# - 内核升级: yum update kernel
# - Docker/Containerd 升级
# - 磁盘清理: docker system prune  # ⚠️ 强制清理，可能杀运行中容器
# - 硬件更换

# Step 7: 恢复节点调度 (uncordon)
kubectl uncordon node-worker-1

# 预期输出:
# node/node-worker-1 uncordoned

# Step 8: 验证节点恢复
kubectl get nodes

# 预期输出:
# NAME            STATUS   ROLES    AGE   VERSION
# node-worker-1   Ready    worker   30d   v1.30.1
# node-worker-2   Ready    worker   30d   v1.30.1

# Step 9: 验证 Pod 自动重新调度到恢复的节点
# Deployment 管理的 Pod 会在节点恢复后自动调度
kubectl get pods -A -o wide
```
### 任务 4: ACK 节点运维操作 (30min)

```bash
# Step 1: 通过 aliyun CLI 查看节点池
aliyun cs GET /clusters/<cluster_id>/nodepools

# 预期输出 (部分):
# {
#   "nodepools": [{
#     "nodepool_id": "np-abc123",
#     "name": "default-pool",
#     "type": "ess",
#     "status": "active",
#     "node_count": 3
#   }]
# }

# Step 2: 通过 ACK API 排水节点
aliyun cs POST /clusters/<cluster_id>/nodes/drain \
  --body '{"nodes":["node-worker-1"],"drain_timeout":300}'

# Step 3: 查看节点维护历史
aliyun cs GET /clusters/<cluster_id>/nodes/maintenance

# Step 4: 节点池扩容
aliyun cs PUT /clusters/<cluster_id>/nodepools/<nodepool_id> \
  --body '{"desired_size": 5}'

# 预期输出:
# {
#   "task_id": "task-xxx",
#   "nodepool_id": "np-abc123",
#   "state": "scaling"
# }

# Step 5: 查看扩容状态
aliyun cs GET /clusters/<cluster_id>/nodepools/<nodepool_id>

# ACK 控制台操作:
# 控制台 → 容器服务 → 集群 → 节目管理 → 节目列表
# 操作选项:
# - 排水: 等同于 cordon + drain
# - 移除: 从集群中移除节点
# - 停止调度: 等同于 cordon
# - 恢复调度: 等同于 uncordon
# - 标签管理: 批量添加/删除标签
# - 污点管理: 批量添加/删除污点
```

---

## 配置参考

### 完整的节点维护 Runbook

```yaml
apiVersion: v1
kind: ConfigMap
metadata:
  name: node-maintenance-runbook
  namespace: ops
data:
  maintenance-steps: |
    ## 节点维护标准流程

    ### 前置检查
    1. 确认集群有足够的冗余节点接收迁移的 Pod
    2. 确认 PDB (PodDisruptionBudget) 配置正确
    3. 通知相关团队维护计划

    ### 执行维护
    1. kubectl cordon <node>
    2. kubectl drain <node> --ignore-daemonsets --delete-emptydir-data
    3. 执行维护操作
    4. kubectl uncordon <node>

    ### 后置验证
    1. kubectl get nodes (确认 Ready)
    2. kubectl get pods -A -o wide (确认 Pod 重新调度)
    3. 检查监控指标是否正常

```

### 调度约束配置对比

| 配置 | 类型 | 示例 | 复杂度 | 灵活性 |
|------|------|------|--------|--------|
| `nodeSelector` | Pod Spec | `env: production` | 低 | 精确匹配 |
| `nodeAffinity (required)` | Pod Spec | `In [zone-a, zone-b]` | 中 | 表达式匹配 |
| `nodeAffinity (preferred)` | Pod Spec | `weight: 80` | 中 | 软性偏好 |
| `Taint + Toleration` | Node + Pod | `dedicated=gpu:NoSchedule` | 中 | 排斥/容忍 |
| `PodAffinity` | Pod Spec | `topologyKey: zone` | 高 | Pod 间吸引 |
| `PodAntiAffinity` | Pod Spec | 均匀分布 | 高 | Pod 间排斥 |

### Taint 参数说明

| 参数 | 说明 | 示例 |
|------|------|------|
| `key` | 污点键名 | `dedicated`, `node.kubernetes.io/not-ready` |
| `value` | 污点值（可选） | `gpu`, `high-mem` |
| `effect` | 污点效果 | `NoSchedule`, `PreferNoSchedule`, `NoExecute` |

### 常用系统污点

| 污点 | 自动添加时机 | 说明 |
|------|-------------|------|
| `node.kubernetes.io/not-ready` | 节点 NotReady | NodeProblemDetector 检测到问题 |
| `node.kubernetes.io/unreachable` | 节点不可达 | controller-manager 无法访问节点 |
| `node.kubernetes.io/memory-pressure` | 内存压力 | 节点内存不足 |
| `node.kubernetes.io/disk-pressure` | 磁盘压力 | 节点磁盘不足 |
| `node.kubernetes.io/network-unavailable` | 网络不可用 | 节点网络未配置 |

---

## 常见问题

### Q1: Taints 和 Tolerations 的工作机制是什么？

**A**: 
1. **Taint** 附加在节点上，表示"排斥不能容忍此条件的 Pod"
2. **Toleration** 附加在 Pod 上，表示"我可以容忍这种条件"
3. 调度器在调度时检查：如果 Pod 没有匹配节点污点的容忍，则不会调度到该节点
4. `NoExecute` 效果还会驱逐已经在运行的不能容忍的 Pod

### Q2: drain 和 cordon 的区别是什么？什么时候用哪个？

**A**: 
- **cordon**: 只禁止新 Pod 调度，不影响已有 Pod。用于计划维护前的准备
- **drain**: cordon + 驱逐已有 Pod。用于需要清空节点进行维护的场景
- **uncordon**: 恢复节点调度。维护完成后使用
- **最佳实践**: `cordon → drain → 维护 → uncordon` 完整流程

### Q3: 节点维护时如何保证业务不中断？

**A**: 确保业务连续性的关键措施：
1. **PDB (PodDisruptionBudget)**: 确保始终有足够的 Pod 在线
2. **多副本部署**: Deployment 至少 3 副本，分布在多个节点
3. **拓扑分布**: 使用 topologySpreadConstraints 跨可用区分布
4. **逐个维护**: 一次只维护一个节点，等待上一个恢复后再进行下一个
5. **提前验证**: 维护前确认有足够的冗余资源

### Q4: nodeSelector 和 nodeAffinity 怎么选？

**A**: 
- **nodeSelector**: 简单场景，只需要精确匹配标签。如 `env=production`
- **nodeAffinity**: 复杂场景，需要表达式匹配或权重偏好。如 `zone in [a, b]` 优先 `zone=a`
- **建议**: 简单需求用 nodeSelector，复杂需求用 nodeAffinity

### Q5: drain 时某些 Pod 无法驱逐怎么办？

**A**: 常见原因和解决方法：
1. **PDB 阻止**: 检查 `kubectl get pdb -n <ns>`，临时调整 PDB
2. **Pod 有 Local Storage**: 使用 `--delete-emptydir-data` 参数
3. **DaemonSet Pod**: 使用 `--ignore-daemonsets` 参数
4. **Pod 超时**: 使用 `--timeout` 和 `--grace-period` 调整超时
5. **强制删除**: 最后手段 `kubectl delete pod <pod> --force --grace-period=0`

---

## 要点总结

- **节点标签**用于标识节点属性，配合 nodeSelector/nodeAffinity 实现精准调度
- **Taint/Toleration** 实现节点专用化，三种 Effect: NoSchedule / PreferNoSchedule / NoExecute
- **cordon → drain → 维护 → uncordon** 是标准的节点维护流程
- **nodeSelector** 适合简单匹配，**nodeAffinity** 适合复杂表达式
- **PDB** 确保维护期间业务不中断
- **ACK API** 提供了控制台等价的节点管理操作

---

## 延伸阅读

- [节点管理文档](https://kubernetes.io/docs/concepts/architecture/nodes/)
- [Taint 和 Toleration](https://kubernetes.io/docs/concepts/scheduling-eviction/taint-and-toleration/)
- [节点维护](https://kubernetes.io/docs/tasks/administer-cluster/safely-drain-node/)
- [文件: `../../../故障诊断/09-node-comprehensive-troubleshooting.md`](../../../../../../19-%E6%95%85%E9%9A%9C%E8%AF%8A%E6%96%AD/02-%E8%B5%84%E6%BA%90%E6%8E%92%E9%9A%9C/09-node-comprehensive-troubleshooting.md)
- [文件: `../../../工作负载/02-deployment-production-patterns.md`](../../../../../../02-%E5%B7%A5%E4%BD%9C%E8%B4%9F%E8%BD%BD/01-%E6%A0%B8%E5%BF%83%E5%B7%A5%E4%BD%9C%E8%B4%9F%E8%BD%BD/02-deployment-production-patterns.md)

---

## 明日预告

Day 17 将学习 ACK 节点池的基础概念与创建配置。

```

<!-- risk-assessed -->
