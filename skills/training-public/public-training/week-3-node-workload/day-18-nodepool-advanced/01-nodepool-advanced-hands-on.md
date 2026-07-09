---
title: 'Day 18: 节点池进阶实操'
description: '- Cluster Autoscaler 配置'
summary: '- Cluster Autoscaler 配置'
category: learning
tags:
- k8s
- training
- hands-on
- kubelet
- pdb
- daemonset
- operator
- gpu
- nvidia
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
- 'Day 18: 节点池进阶实操 是什么'
- '如何 Day 18: 节点池进阶实操'
trigger_keywords:
- Day
- '18:'
- 节点池进阶实操
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
title: Day 18: 节点池进阶实操
last_updated: 2026-05-18
difficulty: advanced
intent_queries:
  - [[entities/kubernetes.md|[[Kubernetes|kubernetes]]]] 节点池弹性伸缩
  - Cluster Autoscaler 配置
  - 节点池生命周期管理
  - PDB Pod 中断预算
trigger_keywords:
  - 节点池
  - Cluster Autoscaler
  - 弹性伸缩
  - 节点池升级
  - PDB
  - 成本优化
  - Spot 实例
reading_level: advanced
audience:
  - sre-engineer
  - ops-engineer
  - platform-engineer
estimated_read_time: 90min
related_domains:
  - 平台工程
  - 故障诊断
related_topics:
  - 生产运维/topic-learn/public-training/week-3-node-workload/day-17-nodepool/01-nodepool-basics-hands-on
  - 生产运维/topic-learn/public-training/week-3-node-workload/day-20-pod-advanced/01-pod-advanced-hands-on
---

# Day 18: 节点池进阶实操

> **日期**: Week 3 Day 4 | **主题**: 节点池扩缩容与生命周期管理 | **版本**: K8s 1.28-1.33

---

## 1. 节点池弹性伸缩

### 1.1 Cluster Autoscaler 原理

```
触发条件: Pod 无法调度（Pending）
    ↓
检测: 调度失败原因 = 资源不足
    ↓
决策: 计算需要新增节点数量
    ↓
执行: 调用云厂商 API 创建节点
    ↓
等待: 节点 Ready → 调度器分配 Pod
    ↓
冷却: scale-down-delay 避免频繁扩缩
```

### 1.2 Cluster Autoscaler 配置

```yaml
# kube-system/cluster-autoscaler-config ConfigMap
apiVersion: v1
kind: ConfigMap
metadata:
  name: cluster-autoscaler-config
  namespace: kube-system
data:
  config.yaml: |
    # 扩容配置
    scale-out-utilization-threshold: 0.5  # 资源使用 > 50% 时扩容
    scale-out-delay: 10s                  # 触发后延迟 10s 再扩容

    # 缩容配置
    scale-down-delay-after-add: 10m        # 扩容后 10 分钟内不缩容
    scale-down-delay-after-delete: 10s     # 删除节点后 10 秒可再次缩容
    scale-down-unneeded-time: 10m           # 节点空闲 10 分钟开始缩容
    scale-down-utilization-threshold: 0.3  # 资源使用 < 30% 时尝试缩容

    # 安全配置
    skip-nodes-with-system-pods: true     # 有 system Pod 的节点不缩容
    skip-nodes-with-local-storage: true   # 有本地存储的节点不缩容
    best-effort-scale-down: true           # 优先缩容低优先级 Pod

    # 节点池配置
    aws-use-static-instance-list: false
    azure-use-managed-identity: true
```

### 1.3 节点池扩缩容触发条件

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 手动触发扩容（无 ASG 的集群）
kubectl scale deployment <name> --replicas=20
kubectl get events --sort-by='.lastTimestamp' | grep "Scaling"

# 监控扩容行为
kubectl logs -n kube-system cluster-autoscaler-xxx --tail=20 -f

# 扩容失败常见原因
# 1. 云厂商配额不足（limit exceeded）
# 2. ASG 最大节点数限制
# 3. 可用区资源不足
# 4. 节点池标签过滤导致无可用节点
```
---

## 2. 节点池生命周期管理

### 2.1 节点池升级

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `kubectl cordon`：标记节点不可调度
> - `kubectl drain`：驱逐节点所有 Pod，业务流量受影响
> - `systemctl stop/restart`：停止/重启系统服务，影响节点上所有容器

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
# 查看当前节点池版本
kubectl get nodes -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{.status.nodeInfo.kubeletVersion}{"\n"}'

# 滚动升级节点池（替换旧版本节点）
for node in $(kubectl get nodes -l node-pool=general-compute --no-headers | awk '{print $1}'); do
  echo "升级节点: $node"
  kubectl cordon $node
  kubectl drain $node --ignore-daemonsets --grace-period=60

  # 在节点上执行升级
  ssh $node "sudo apt-get update && sudo apt-get install -y kubelet=1.30.0-1.30*"
  ssh $node "sudo systemctl restart kubelet"

  # 等待节点恢复
  until kubectl get node $node | grep -q "Ready"; do sleep 10; done
  kubectl uncordon $node
  echo "节点 $node 升级完成"
done
```
### 2.2 节点池回滚

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 记录节点池状态快照
kubectl get nodes -l node-pool=gpu-compute -o yaml > node-pool-snapshot.yaml

# 缩容问题节点
kubectl scale nodegroup --cluster=my-cluster --name=gpu-pool --nodes=0

# 扩容新节点池
kubectl scale nodegroup --cluster=my-cluster --name=gpu-pool --nodes=3
```
### 2.3 节点池销毁

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `kubectl cordon`：标记节点不可调度
> - `kubectl drain`：驱逐节点所有 Pod，业务流量受影响
> - `kubectl delete`：删除资源（可由声明式清单重建）

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
# 安全销毁节点池步骤
# 1. 迁移工作负载
kubectl cordon <node-name>
kubectl drain <node-name> --ignore-daemonsets

# 2. 移除节点
kubectl delete node <node-name>

# 3. 在云控制台删除节点池（销毁所有节点）

# 4. 验证清理
kubectl get nodes | grep <node-pool>
```
---

## 3. 节点池调度优化

### 3.1 资源配额控制

```yaml
# ResourceQuota 限制节点池资源使用
apiVersion: v1
kind: ResourceQuota
metadata:
  name: compute-quota
  namespace: production
spec:
  hard:
    requests.cpu: "40"
    requests.memory: "80Gi"
    pods: "100"
    nvidia.com/gpu: "8"
  scopeSelector:
    matchExpressions:
      - operator: In
        scopeName: PriorityClass
        values: ["production-high"]
```

### 3.2 LimitRange 强制资源限制

```yaml
# 默认资源限制（防止 Pod 未设置 limits）
apiVersion: v1
kind: LimitRange
metadata:
  name: default-limits
  namespace: production
spec:
  limits:
    - type: Container
      default:
        cpu: "500m"
        memory: "512Mi"
      defaultRequest:
        cpu: "200m"
        memory: "256Mi"
      max:
        cpu: "4"
        memory: "8Gi"
      min:
        cpu: "50m"
        memory: "64Mi"
```

### 3.3 Pod 中断预算（PDB）

```yaml
# 保护核心 Deployment 最小可用副本
apiVersion: policy/v1beta1
kind: PodDisruptionBudget
metadata:
  name: web-backend-pdb
  namespace: production
spec:
  minAvailable: 2   # 至少保持 2 个可用副本
  # 或使用百分比
  # maxUnavailable: 50%
  selector:
    matchLabels:
      app: web-backend
```

---

## 4. 节点池成本优化

### 4.1 Spot/Preemptible 节点池

```yaml
# 使用 Spot 实例降低成本
# AWS: 在 nodegroup 配置中添加 --instance-types=m5,m5a --spot
# GKE: 创建节点池时指定 --enable-spot

# Pod 容忍 Spot 中断
spec:
  tolerations:
    - key: "cloud.google.com/gke-spot"
      operator: "Equal"
      value: "true"
      effect: "NoSchedule"
  podPriorityClassName: spot-tolerant
```

### 4.2 资源利用率分析

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 分析节点池资源使用
kubectl top nodes -l node-pool=general-compute

# 统计 Pod 资源请求 vs 限制
kubectl get pods -n production -o json | jq -r '
.items[] | "\(.metadata.name): req=\(.spec.containers[].resources.requests.cpu) lim=\(.spec.containers[].resources.limits.cpu)"
'

# 找出资源浪费的 Pod
# - 设置 limits 但实际使用 < 30%
# - 设置 requests 过高导致调度效率低
```
### 4.3 节点池成本可视化

```promql
# 节点池成本估算（基于 AWS EC2 pricing）
# CPU cost: $0.0236/vCPU-hour (m5.xlarge)
# Memory cost: $0.0059/GiB-hour

# 节点池月度成本
sum by (node_pool) (
  count by (node_pool) (
    kube_node_labels{label_node_pool!=""}
  ) * 0.0236 * 24 * 30  # CPU
) + sum by (node_pool) (
  sum by (node_pool, memory) (
    kube_node_status_allocatable{resource="memory"} / 1024
  ) * 0.0059 * 24 * 30  # Memory
)
```

---

## 5. 节点池高可用设计

### 5.1 跨可用区分布

```yaml
# 多可用区节点池
# AWS EKS
eksctl create nodegroup --cluster=my-cluster \
  --name=multi-az \
  --zones=us-east-1a,us-east-1b,us-east-1c \
  --nodes=3

# GKE
gcloud container clusters create my-cluster \
  --zone us-central1-a \
  --node-pool multi-az \
  --num-nodes=3

# 验证 Pod 分布
kubectl get pods -o wide -A | awk '{print $NF}' | sort | uniq -c
```

### 5.2 多节点池容灾

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl label/annotate`：改元数据可能影响选择器/控制器

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 主节点池问题时切换到备用
kubectl label node <standby-node> failover-pool=standby

# 紧急扩容备用节点池
kubectl scale deployment web-backend --replicas=20

# 快速迁移脚本
cat > failover.sh <<'EOF'
#!/bin/bash
SOURCE_POOL="general-compute"
TARGET_POOL="general-compute-backup"

echo ">>> 开始故障转移"

# 1. 标记备用节点池
kubectl label nodes -l node-pool=$TARGET_POOL failover=true --overwrite

# 2. 紧急扩容目标节点池
for deploy in $(kubectl get deploy -n production -o jsonpath='{.items[*].metadata.name}'); do
  CURRENT=$(kubectl get deploy $deploy -n production -o jsonpath='{.spec.replicas}')
  NEW=$((CURRENT * 2))
  echo "扩容 $deploy: $CURRENT -> $NEW"
  kubectl scale deployment $deploy -n production --replicas=$NEW
done

echo ">>> 故障转移完成"
EOF
```
---

## 6. 节点池监控与告警

### 6.1 节点池状态告警

```yaml
# 节点池健康监控
- alert: NodePoolNotHealthy
  expr: |
    count by (node_pool) (
      kube_node_status_condition{condition="Ready",status="false"}
    ) > 0
  for: 5m
  labels:
    severity: critical

- alert: NodePoolAtCapacity
  expr: |
    (count by (node_pool) (kube_node_labels) /
     max by (node_pool) (kube_node_labels)) > 0.9
  for: 10m
  labels:
    severity: warning
```

### 6.2 节点池成本告警

```yaml
# 月度成本超限告警
- alert: NodePoolCostExceeded
  expr: |
    node_pool_daily_cost > 1000  # 设置阈值
  for: 1h
  labels:
    severity: warning
```

---

## 7. 实战练习

**练习 1**: 配置 Cluster Autoscaler，在节点池资源使用率 > 70% 时自动扩容

**练习 2**: 创建 PDB 保护核心 Deployment，确保任何时候至少 2 个副本可用

**练习 3**: 使用 Spot 实例创建成本优化节点池，配置中断容忍

**练习 4**: 配置跨可用区节点池，使用 topologySpreadConstraints 确保 Pod 均匀分布

---



<!-- risk-assessed -->
