---
title: 'Day 16: Node 节点进阶实操'
description: '- Cluster Autoscaler 配置'
summary: '- Cluster Autoscaler 配置'
category: learning
tags:
- k8s
- training
- hands-on
- scheduler
- hpa
- pdb
- daemonset
- operator
- gpu
- nvidia
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 'Day 16: Node 节点进阶实操 是什么'
- '如何 Day 16: Node 节点进阶实操'
trigger_keywords:
- Day
- '16:'
- Node
- 节点进阶实操
- learn
prerequisites:
- kubectl-basics
- gpu-ml-basics
- gpu-scheduling-basics
---



---
title: Day 16: Node 节点进阶实操
last_updated: 2026-05-18
difficulty: advanced
intent_queries:
  - [[entities/kubernetes.md|[[Kubernetes|kubernetes]]]] 节点池扩缩容
  - Cluster Autoscaler 配置
  - Pod 亲和性反亲和性
  - 拓扑分布约束
trigger_keywords:
  - 节点池
  - node-pool
  - autoscaler
  - 亲和性
  - topology
  - 调度
  - 拓扑
  - PodAntiAffinity
reading_level: advanced
audience:
  - sre-engineer
  - ops-engineer
  - platform-engineer
estimated_read_time: 90min
related_domains:
  - domain-10-troubleshooting-diagnostics
  - domain-07-platform-engineering
related_topics:
  - domain-11-production-operations/topic-learn/public-training/week-3-node-workload/day-15-node-basics/01-node-basics-hands-on
  - domain-11-production-operations/topic-learn/public-training/week-3-node-workload/day-17-nodepool/01-nodepool-basics-hands-on
  - domain-11-production-operations/topic-learn/public-training/week-3-node-workload/day-18-nodepool-advanced/01-nodepool-advanced-hands-on
---

# Day 16: Node 节点进阶实操

> **日期**: Week 3 Day 2 | **主题**: 节点维护、标签与调度约束 | **版本**: K8s 1.28-1.33

---

## 1. 节点池（Node Pool）概念

### 1.1 节点池 vs 手动节点管理

| 维度 | 手动管理 | 节点池 |
|------|---------|--------|
| 扩缩容 | 手动添加/删除 | 自动弹性伸缩 |
| 配置一致性 | 不保证 | 统一配置（标签/污点） |
| 成本 | 高（资源浪费） | 低（按需扩缩） |
| 适用场景 | 少量固定节点 | 生产环境大规模集群 |

### 1.2 节点池标签约定

```bash
# 常用节点池标签
node.kubernetes.io/instance-type=c6.2xlarge  # 实例类型
node-pool=system                              # 系统节点池
node-pool=general-compute                     # 通用计算
node-pool=gpu-compute                         # GPU 计算
node-pool=memory-optimized                    # 内存优化
topology.kubernetes.io/zone=us-east-1a        # 可用区
```

---

## 2. 节点池扩缩容

### 2.1 手动扩缩容

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `kubectl cordon`：标记节点不可调度
> - `kubectl drain`：驱逐节点所有 Pod，业务流量受影响

```bash
# 手动增加节点（使用云厂商 CLI）
# AWS EKS
eksctl scale nodegroup --cluster=my-cluster --name=ng-1 --nodes=5

# 手动减少节点（先 drain）
kubectl cordon <node-name>
kubectl drain <node-name> --ignore-daemonsets
# 然后在云控制台删除节点
```

### 2.2 Cluster Autoscaler 配置

```yaml
# cluster-autoscaler 部署配置
apiVersion: v1
kind: ConfigMap
metadata:
  name: cluster-autoscaler-config
  namespace: kube-system
data:
  config.yaml: |
    scale-down-delay-after-add: 10m
    scale-down-delay-after-delete: 10s
    scale-down-unneeded-time: 10m
    scale-down-utilization-threshold: 0.5
    skip-nodes-with-local-storage: true
    skip-nodes-with-system-pods: true
```

### 2.3 扩缩容监控

```bash
# 监控集群节点数变化
kubectl get nodes -w &
watch -n 5 'kubectl get nodes -l node-pool=gpu-compute'

# 监控 HPA 触发条件
kubectl get hpa -A
kubectl describe hpa <hpa-name>

# 监控 Cluster Autoscaler 日志
kubectl logs -n kube-system -l app=cluster-autoscaler --tail=50
```

---

## 3. 节点调度策略

### 3.1 节点亲和性（Node Affinity）

```yaml
# 必须调度到 GPU 节点（硬亲和）
apiVersion: v1
kind: Pod
metadata:
  name: ml-workload
spec:
  affinity:
    nodeAffinity:
      requiredDuringSchedulingIgnoredDuringExecution:
        nodeSelectorTerms:
          - matchExpressions:
              - key: "nvidia.com/gpu"
                operator: In
                values:
                  - "true"
      preferredDuringSchedulingIgnoredDuringExecution:
        - weight: 10
          preference:
            matchExpressions:
              - key: "node-pool"
                operator: In
                values:
                  - gpu-compute
  containers:
    - name: ml-container
      image: pytorch:latest
      resources:
        limits:
          nvidia.com/gpu: 1
```

### 3.2 Pod 亲和性与反亲和性

```yaml
# 分散 Pod 到不同可用区（高可用）
apiVersion: apps/v1
kind: Deployment
metadata:
  name: web-backend
spec:
  replicas: 3
  strategy:
    type: RollingUpdate
  selector:
    matchLabels:
      app: web-backend
  template:
    spec:
      affinity:
        podAntiAffinity:
          requiredDuringSchedulingIgnoredDuringExecution:
            - labelSelector:
                matchLabels:
                  app: web-backend
              topologyKey: topology.kubernetes.io/zone
      containers:
        - name: web
          image: nginx:latest
          resources:
            requests:
              cpu: "500m"
              memory: "512Mi"
```

### 3.3 拓扑散布约束（Topology Spread）

```yaml
# 跨可用区均匀分布
apiVersion: apps/v1
kind: Deployment
metadata:
  name: api-server
spec:
  replicas: 5
  selector:
    matchLabels:
      app: api-server
  template:
    spec:
      topologySpreadConstraints:
        - maxSkew: 1
          topologyKey: topology.kubernetes.io/zone
          whenUnsatisfiable: DoNotSchedule
          labelSelector:
            matchLabels:
              app: api-server
        - maxSkew: 1
          topologyKey: kubernetes.io/hostname
          whenUnsatisfiable: ScheduleAnyway
          labelSelector:
            matchLabels:
              app: api-server
      containers:
        - name: api
          image: api:v1
```

---

## 4. 节点调度故障排查

### 4.1 Pod Pending（调度失败）

```bash
# 查看 Pod 调度失败原因
kubectl describe pod <pod-name> | grep -A20 "Events:"

# 常见错误：
# - "node(s) had taint(s) that the pod didn't tolerate"
# - "Insufficient cpu/memory"
# - "node(s) didn't match node selector"

# 查看节点资源
kubectl describe node | grep -E "cpu|memory|allocatable"

# 查看污点
kubectl get nodes -o jsonpath='{.items[*].spec.taints}'
```

### 4.2 调度器故障排查

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

```bash
# 检查调度器是否运行
kubectl get pods -n kube-system -l component=kube-scheduler

# 查看调度器日志
kubectl logs -n kube-system kube-scheduler-<pod> --tail=50

# 测试调度器（模拟 Pod 调度）
kubectl create -f pod.yaml --dry-run=client -o json | \
  kubectl alpha scheduling probe --namespace=default
```

### 4.3 自定义调度器

```yaml
# 部署自定义调度器
apiVersion: v1
kind: Pod
metadata:
  name: nginx
spec:
  schedulerName: my-custom-scheduler  # 指定调度器
  containers:
    - name: nginx
      image: nginx:latest
```

---

## 5. 节点指标与监控

### 5.1 节点资源预留

```bash
# 查看节点 allocatable 资源
kubectl get node <node-name> -o jsonpath='{.status.allocatable}'

# 典型配置（生产环境）
# 节点 4C8G，K8s 预留约 0.5C 0.5G 给系统
# allocatable: cpu=3.5, memory=7.5Gi

# 查看节点资源使用分布
kubectl top nodes
kubectl describe nodes | grep -A10 "Allocated resources"
```

### 5.2 节点健康检查

```bash
# 综合健康检查脚本
cat > node-health-check.sh <<'EOF'
#!/bin/bash
NODE=$1

echo "=== 节点健康检查: $NODE ==="

# 1. 节点状态
echo "[1] 节点状态"
kubectl get node $NODE

# 2. 节点条件
echo "[2] 节点条件"
kubectl get node $NODE -o jsonpath='{.status.conditions[*].type}'

# 3. 资源使用
echo "[3] 资源使用"
kubectl top node $NODE

# 4. 污点
echo "[4] 污点"
kubectl describe node $NODE | grep -i taint

# 5. 事件
echo "[5] 近期事件"
kubectl get events --field-selector involvedObject.name=$NODE --sort-by='.lastTimestamp' | tail -10

echo "=== 检查完成 ==="
EOF
chmod +x node-health-check.sh
./node-health-check.sh <node-name>
```

---

## 6. 节点维护深度实践

### 6.1 批量节点维护流程

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `kubectl cordon`：标记节点不可调度
> - `kubectl drain`：驱逐节点所有 Pod，业务流量受影响

```bash
# ========== 批量节点维护 SOP ==========
CLUSTER="production"
MAX_UNAVAILABLE=2  # 最大同时不可用节点数

# 1. 确认维护窗口
date "+%Y-%m-%d %H:%M"

# 2. 获取所有 worker 节点
NODES=$(kubectl get nodes -l node-pool!=$POOL --no-headers | awk '{print $1}')

# 3. 逐节点维护（滚动方式）
for NODE in $NODES; do
  echo ">>> 维护节点: $NODE"

  # cordon
  kubectl cordon $NODE
  echo "[1/5] cordon 完成"

  # drain（最多容忍 5 分钟）
  kubectl drain $NODE --ignore-daemonsets --grace-period=60 --timeout=300s
  echo "[2/5] drain 完成"

  # 执行维护操作
  # ... ssh $NODE "sudo apt-get update && sudo reboot" ...

  # 等待节点恢复
  echo "[3/5] 等待节点恢复..."
  until kubectl get node $NODE | grep -q "Ready"; do
    sleep 10
  done

  # uncordon
  kubectl uncordon $NODE
  echo "[4/5] uncordon 完成"

  # 验证 Pod 恢复
  kubectl get pods -A -o wide | grep $NODE
  echo "[5/5] 验证完成"
done

echo ">>> 批量维护完成"
# ========== SOP 结束 ==========
```

### 6.2 维护期间服务保障

```bash
# 检查 Deployment 副本数是否满足可用性
kubectl get deploy -A | grep -v "1/1"

# 确认 HPA 可用
kubectl get hpa -A

# 确认 PDB（Pod 中断预算）
kubectl get pdb -A
```

---

## 7. 实战练习

**练习 1**: 配置 Cluster Autoscaler，当 Pod Pending 超过 3 分钟时自动扩容

**练习 2**: 创建 3 副本 Deployment，使用 topologySpreadConstraints 实现跨可用区分布

**练习 3**: 编写脚本批量维护 5 个节点，每次最多 1 个节点下线

**练习 4**: 配置 PDB（Pod 中断预算）保护核心服务，确保维护期间至少保留 2 个副本

---


```