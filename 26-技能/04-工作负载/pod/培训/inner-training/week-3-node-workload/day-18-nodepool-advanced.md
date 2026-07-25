---
title: 'Day 18: 节点池进阶'
description: '## 概述'
summary: '在 Day 17 学习了节点池基础之后，今天将深入节点池的高级特性：自动伸缩策略配置、Spot 实例混合策略、节点池升级流程、以及节点池故障排查。这些进阶能力是生产环境运维的核心技能。'
category: learning
tags:
- k8s
- training
- hands-on
- kubelet
- prometheus
- hpa
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
- 'Day 18: 节点池进阶 是什么'
- '如何 Day 18: 节点池进阶'
trigger_keywords:
- Day
- '18:'
- 节点池进阶
- learn
prerequisites:
- kubectl-basics
- gpu-ml-basics
- prometheus-basics
- gpu-scheduling-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




---
title: Day 18: 节点池进阶
last_updated: 2026-05-18
difficulty: advanced
intent_queries:
  - ACK Cluster Autoscaler configuration
  - Spot instance mixed scaling policy
  - Node pool upgrade management
  - Node pool scaling troubleshooting
  - PDB PodDisruptionBudget configuration
trigger_keywords:
  - Cluster Autoscaler
  - auto scaling
  - Spot
  - 抢占式实例
  - nodepool upgrade
  - 节点池升级
  - PDB
  - PodDisruptionBudget
  - scale up
  - scale down
reading_level: advanced
audience:
  - ACK operators
  - SRE engineers
  - Platform engineers
estimated_read_time: 45min
related_domains:
  - domain-3-node
  - 云厂商
  - 故障诊断
related_topics:
  - nodepool-basics
  - cluster-autoscaler-troubleshooting
  - pod-scheduling
---

# Day 18: 节点池进阶

## 概述

在 Day 17 学习了节点池基础之后，今天将深入节点池的高级特性：自动伸缩策略配置、Spot 实例混合策略、节点池升级流程、以及节点池故障排查。这些进阶能力是生产环境运维的核心技能。

### 学习目标

- 掌握节点池自动伸缩的原理和配置
- 理解 Spot 实例与按量付费混合策略以降低成本
- 掌握节点池升级（K8s 版本和 OS 补丁）的操作流程
- 能够排查节点池相关的常见问题

---

## 核心概念详解

### 自动伸缩原理

ACK 的节点池自动伸缩基于 Cluster Autoscaler（CA）组件。CA 定期检查集群中是否存在因资源不足而无法调度的 Pod，如果有，则触发节点池扩容。当节点上的资源利用率持续低于阈值时，CA 触发缩容。

CA 的工作流程：

```
Pod Pending (资源不足)
    ↓
CA 检测 Pending Pod
    ↓
计算需要的节点数量和规格
    ↓
选择合适的节点池扩容
    ↓
节点池创建新 ECS 实例
    ↓
新节点加入集群
    ↓
Pending Pod 被调度到新节点
```

缩容流程：

```
CA 定期扫描节点资源利用率
    ↓
识别低利用率节点
    ↓
检查节点上的 Pod 是否可以迁移
    ↓
Drain 节点上的 Pod
    ↓
释放 ECS 实例
```

**扩容触发条件**: 存在因资源不足（CPU、内存、GPU 等）而 Pending 的 Pod，且这些 Pod 的调度约束（nodeSelector、affinity、toleration）能被某个节点池满足。

**缩容触发条件**: 节点上的所有 Pod 的 requests 总量低于节点可分配资源的一定比例（默认 50%），且节点上没有以下类型的 Pod：kube-system 命名空间的 Pod、使用 local storage 的 Pod、没有 controller 管理的 Pod、有 PodDisruptionBudget 阻止驱逐的 Pod。

### 自动伸缩配置参数

```json
{
  "auto_scaling": {
    "enable": true,
    "min_instances": 2,
    "max_instances": 20,
    "type": "hpa"
  }
}
```

| 参数 | 说明 | 推荐值 |
|------|------|--------|
| enable | 是否启用自动伸缩 | 生产: true |
| min_instances | 最小节点数 | 至少 2（高可用） |
| max_instances | 最大节点数 | 根据预算设定 |
| type | 伸缩类型 | hpa（基于指标） |

自动伸缩最佳实践：

- **cool-down period**: 扩容后等待一段时间（默认 10 分钟）再考虑缩容，避免频繁抖动
- **scale-down-utilization-threshold**: 节点资源利用率低于此值时考虑缩容，默认 0.5（50%）
- **skip-nodes-with-system-[[Pods|pods]]**: 不要缩容运行 kube-system Pod 的节点，默认 true
- **balance-similar-node-groups**: 保持相似节点组的节点数均衡，推荐开启

### Spot 实例混合策略

Spot 实例（抢占式实例）可以大幅降低计算成本（通常比按量付费便宜 50%-90%），但存在被回收的风险。ACK 节点池支持多实例规格配置，可以在一个节点池中混合使用按量付费和 Spot 实例。

Spot 实例策略：

| 策略 | 说明 | 适用场景 |
|------|------|---------|
| 最低价格 | 选择当前价格最低的规格 | 成本优先 |
| 容量优化 | 选择库存最充足的规格 | 稳定性优先 |
| 多规格组合 | 配置多个备选规格 | 平衡成本和稳定性 |

混合实例配置：

```json
{
  "scaling_group": {
    "instance_types": [
      "ecs.g6.xlarge",
      "ecs.g6a.xlarge",
      "ecs.g7.xlarge"
    ],
    "spot_strategy": "SpotAsPriceGo",
    "spot_budget": 0.5,
    "multi_az_policy": "BALANCE"
  }
}
```

| 参数 | 说明 | 推荐值 |
|------|------|--------|
| spot_strategy | Spot 策略 | SpotAsPriceGo（出价跟随市场价） |
| spot_budget | Spot 实例预算比例 | 0.5-0.7（50%-70% 为 Spot） |
| instance_types | 多实例规格 | 至少配置 3 个备选规格 |
| multi_az_policy | 多可用区策略 | BALANCE（均衡分布） |

Spot 实例最佳实践：

- 在弹性节点池中使用 Spot 实例，系统节点池使用按量付费
- 配置至少 3 个备选实例规格，避免单一规格库存不足
- 应用层做好优雅终止处理，响应 Spot 回收通知
- 使用 PodDisruptionBudget 保障最小可用副本数

### 节点池升级

节点池升级包含两类：K8s 版本升级和操作系统补丁升级。

**K8s 版本升级流程**：

```
1. 升级控制平面（Master）
   ↓
2. 逐个升级节点池中的节点
   ├── Cordon 节点（标记不可调度）
   ├── Drain 节点（驱逐 Pod）
   ├── 升级 kubelet 和 kube-proxy
   ├── Uncordon 节点（恢复调度）
   └── 验证节点健康
```

**托管节点池自动升级配置**：

```json
{
  "management": {
    "auto_upgrade": true,
    "upgrade_config": {
      "auto_upgrade_kubelet": true,
      "maintenance_window": {
        "enable": true,
        "maintenance_time": "02:00:00",
        "duration": "4h",
        "weekly_period": "Sat"
      }
    }
  }
}
```

升级注意事项：

- 升级前确保所有 Deployment 有足够的 replicas 和 PDB
- 升级前在测试环境验证应用兼容性
- 升级过程中监控应用可用性和错误率
- 预留回滚方案（升级失败的节点可以替换）

### 节点池监控指标

| 指标 | 说明 | 告警阈值 |
|------|------|---------|
| 节点数变化 | 节点池节点数增减 | 缩容超过 50% |
| 节点 NotReady | 节点不可用 | 持续 5 分钟 |
| CPU 使用率 | 节点 CPU 利用率 | > 85% 持续 10 分钟 |
| 内存使用率 | 节点内存利用率 | > 90% 持续 10 分钟 |
| 磁盘使用率 | 节点磁盘利用率 | > 85% 持续 15 分钟 |
| Pod 调度失败 | Pending Pod 数 | > 0 持续 10 分钟 |
| 自动伸缩触发 | CA 扩缩容事件 | 频繁触发（> 5 次/小时） |

---

## 实战演练

### 任务 1: 配置弹性节点池 (45min)

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 创建弹性节点池（Spot + 按量混合）
cat > elastic-nodepool.json << 'EOF'
{
  "nodepool_info": {
    "name": "elastic-spot-pool"
  },
  "scaling_group": {
    "vswitch_ids": ["<vsw-id-az-a>", "<vsw-id-az-b>"],
    "instance_types": [
      "ecs.g6.xlarge",
      "ecs.g6a.xlarge",
      "ecs.g7.xlarge"
    ],
    "system_disk_category": "cloud_essd",
    "system_disk_size": 120,
    "spot_strategy": "SpotAsPriceGo",
    "multi_az_policy": "BALANCE",
    "key_pair": "<your-key-pair>"
  },
  "kubernetes_config": {
    "labels": [
      {"key": "workload", "value": "elastic"},
      {"key": "spot", "value": "true"}
    ],
    "taints": [
      {"key": "spot", "value": "true", "effect": "NoSchedule"}
    ]
  },
  "management": {
    "auto_repair": true
  },
  "auto_scaling": {
    "enable": true,
    "min_instances": 0,
    "max_instances": 20
  },
  "count": 0
}
EOF

aliyun cs POST /clusters/<cluster_id>/nodepools \
  --body "$(cat elastic-nodepool.json)"
```
### 任务 2: 配置应用使用弹性节点池 (30min)

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 部署能容忍 Spot 污点的应用
cat > spot-deployment.yaml << 'EOF'
apiVersion: apps/v1
kind: Deployment
metadata:
  name: batch-processor
  namespace: default
spec:
  replicas: 5
  selector:
    matchLabels:
      app: batch-processor
  template:
    metadata:
      labels:
        app: batch-processor
    spec:
      tolerations:
      - key: spot
        value: "true"
        effect: NoSchedule
      affinity:
        nodeAffinity:
          preferredDuringSchedulingIgnoredDuringExecution:
          - weight: 100
            preference:
              matchExpressions:
              - key: spot
                operator: In
                values: ["true"]
      containers:
      - name: worker
        image: busybox:1.36
        command: ['sh', '-c', 'echo "Processing batch..." && sleep 3600']
        resources:
          requests:
            cpu: 500m
            memory: 512Mi
          limits:
            cpu: "1"
            memory: 1Gi
      terminationGracePeriodSeconds: 60
---
apiVersion: policy/v1
kind: PodDisruptionBudget
metadata:
  name: batch-processor-pdb
  namespace: default
spec:
  minAvailable: 3
  selector:
    matchLabels:
      app: batch-processor
EOF

kubectl apply -f spot-deployment.yaml

# 验证 Pod 调度到弹性节点池
kubectl get pods -l app=batch-processor -o wide

# 查看 CA 伸缩日志
kubectl logs -n kube-system -l app=cluster-autoscaler --tail=50
```
### 任务 3: 监控和排查自动伸缩问题 (30min)

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看 CA 状态
kubectl get configmap cluster-autoscaler-status -n kube-system -o yaml

# 示例输出:
# Cluster-autoscaler status at 2024-01-15 10:30:00:
# Cluster-wide:
#   Health:      Healthy
#   ScaleUp:     Needed 3 nodes, scaled up 3 nodes
#   ScaleDown:   No candidates for scale down
# NodeGroups:
#   Name:        elastic-spot-pool
#   Health:      Healthy
#   Min:         0, Max: 20, Current: 5
#   ScaleUp:     3 nodes in last 30 minutes

# 查看 Pending Pod 原因
kubectl get pods -A --field-selector=status.phase=Pending

# 详细查看 Pending 原因
kubectl describe pod <pending-pod-name> | grep -A10 Events

# 常见 Pending 原因和解决方案:
# 1. Insufficient cpu/memory → 扩容节点池或增加节点
# 2. MatchNodeSelector → 检查 nodeSelector/affinity 配置
# 3. Node(s) had taints that the pod didn't tolerate → 添加 toleration
# 4. PersistentVolumeClaim not bound → 检查 StorageClass 和 PV

# 查看伸缩活动历史
aliyun cs GET /clusters/<cluster_id>/nodepools/<nodepool_id> | jq '.status'
```
### 任务 4: 节点池升级演练 (30min)

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
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
# 查看当前集群版本
kubectl version --short

# 查看可升级版本
aliyun cs GET /clusters/<cluster_id>/upgradestatus

# 升级节点池中的节点（手动方式）
# 先 cordon 节点
kubectl cordon <node-name>

# Drain 节点
kubectl drain <node-name> --ignore-daemonsets --delete-emptydir-data --grace-period=60

# 验证 Pod 已迁移
kubectl get pods -A -o wide | grep <node-name>

# 通过 API 升级节点
aliyun cs POST /clusters/<cluster_id>/nodepools/<nodepool_id>/upgrade \
  --body '{"version": "1.28.3-aliyun.1"}'

# 查看升级进度
aliyun cs GET /clusters/<cluster_id>/upgradestatus | jq '.nodepool_status'

# 升级完成后 uncordon
kubectl uncordon <node-name>

# 验证节点版本
kubectl get nodes -o wide
```
---

## 配置参考

### Cluster Autoscaler 配置

```yaml
# CA ConfigMap 配置
apiVersion: v1
kind: ConfigMap
metadata:
  name: cluster-autoscaler-status
  namespace: kube-system
data:
  cluster-autoscaler.[[23-实体/02-K8s核心组件/kubernetes.md|kubernetes]].io/cluster-autoscaler-status: |
    Cluster-wide:
      Health: Healthy
      ScaleUp: Needed 3 nodes, scaled up 3 nodes
      ScaleDown: No candidates
    NodeGroups:
      - Name: app-pool
        Health: Healthy (ready: 5/5)
        Min: 2, Max: 10
---
# CA 部署参数
apiVersion: apps/v1
kind: Deployment
metadata:
  name: cluster-autoscaler
  namespace: kube-system
spec:
  replicas: 1
  selector:
    matchLabels:
      app: cluster-autoscaler
  template:
    spec:
      containers:
      - name: cluster-autoscaler
        image: registry.cn-hangzhou.aliyuncs.com/acs/cluster-autoscaler:v1.28.0
        command:
        - ./cluster-autoscaler
        - --alsologtostderr
        - --v=4
        - --stderrthreshold=info
        - --cloud-provider=alicloud
        - --skip-nodes-with-system-pods=false
        - --expander=priority
        - --scale-down-delay-after-add=10m
        - --scale-down-delay-after-delete=10m
        - --scale-down-delay-after-failure=3m
        - --scale-down-unneeded-time=10m
        - --scale-down-utilization-threshold=0.5
        - --balance-similar-node-groups=true
        - --node-group-auto-discovery=asg:tag:k8s.io/cluster-autoscaler/enabled
        resources:
          requests:
            cpu: 100m
            memory: 300Mi
          limits:
            cpu: "1"
            memory: 1Gi
```

### PDB 配置模板

```yaml
# 确保关键应用在节点维护时的可用性
apiVersion: policy/v1
kind: PodDisruptionBudget
metadata:
  name: critical-app-pdb
  namespace: production
spec:
  minAvailable: "66%"
  selector:
    matchLabels:
      app: critical-app
---
# 批处理应用的 PDB（更宽松）
apiVersion: policy/v1
kind: PodDisruptionBudget
metadata:
  name: batch-app-pdb
  namespace: default
spec:
  maxUnavailable: "50%"
  selector:
    matchLabels:
      app: batch-processor
```

### 节点池监控 Prometheus 规则

```yaml
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: nodepool-alerts
  namespace: monitoring
spec:
  groups:
  - name: nodepool
    rules:
    - alert: NodePoolScalingTooFrequent
      expr: sum(increase(kube_node_created[1h])) > 10
      for: 5m
      labels:
        severity: warning
      annotations:
        summary: "Node pool scaling too frequently"
        description: "More than 10 nodes created in the last hour"
    - alert: NodePoolUnhealthyNodes
      expr: sum(kube_node_status_condition{condition="Ready",status!="true"}) by (node) > 0
      for: 10m
      labels:
        severity: critical
      annotations:
        summary: "Node {{ $labels.node }} is unhealthy"
    - alert: SpotInstanceReclaimed
      expr: sum(increase(kube_node_deleted{label_spot="true"}[5m])) > 0
      for: 1m
      labels:
        severity: info
      annotations:
        summary: "Spot instance reclaimed"
```

---

## 延伸阅读

- [集群自动伸缩排障](../../故障诊断/28-cluster-autoscaler-troubleshooting.md)
- [ECS 计算资源](../../云厂商/04-alicloud-ack/240-ack-ecs-compute.md)
- [集群升级策略](../../云厂商/04-alicloud-ack/220-ack-upgrade.md)
- [K8s 调度策略](../../domain-09-workload/05-pod-scheduling.md)


<!-- risk-assessed -->
