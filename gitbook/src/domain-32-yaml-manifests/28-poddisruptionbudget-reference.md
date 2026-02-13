# 28 - PodDisruptionBudget YAML 配置参考

> **适用版本**: Kubernetes v1.25 - v1.32 | **最后更新**: 2026-02  
> **主题**: PDB 中断预算、集群维护保护、高可用保障

## 目录

- [概述](#概述)
- [完整字段说明](#完整字段说明)
- [minAvailable vs maxUnavailable](#minavailable-vs-maxunavailable)
- [unhealthyPodEvictionPolicy](#unhealthypodeevictionpolicy)
- [内部原理](#内部原理)
- [版本兼容性](#版本兼容性)
- [生产案例](#生产案例)
- [最佳实践](#最佳实践)
- [常见问题](#常见问题)

---

## 概述

### PodDisruptionBudget (PDB)

PDB 用于限制主动驱逐 Pod 的数量，保证应用在集群维护（节点升级、节点排空）期间的高可用性。

**核心概念**:
- **Voluntary Disruption (主动中断)**: 可控的维护操作
  - `kubectl drain` 排空节点
  - 手动删除 Pod
  - Deployment 滚动更新
  - Cluster Autoscaler 缩容节点
  
- **Involuntary Disruption (非主动中断)**: 不可控的故障
  - 硬件故障
  - 内核崩溃
  - 网络分区
  - OOM Kill
  - ⚠️ **PDB 不保护非主动中断**

**工作机制**:
- PDB 不阻止 Pod 创建/删除
- PDB 仅影响 Eviction API 的驱逐决策
- PDB 通过 `minAvailable` 或 `maxUnavailable` 定义保护策略

---

## 完整字段说明

### 基础结构

```yaml
apiVersion: policy/v1
kind: PodDisruptionBudget
metadata:
  name: myapp-pdb
  namespace: default
spec:
  # ========== Pod 选择器 (必需) ==========
  # 选择受保护的 Pod
  selector:
    matchLabels:
      app: myapp
      tier: frontend
  
  # ========== 保护策略 (二选一) ==========
  
  # 选项1: 最少可用 Pod 数
  minAvailable: 2
  # 可以是整数或百分比字符串:
  # - 整数: 2 (至少 2 个 Pod 可用)
  # - 百分比: "50%" (至少 50% Pod 可用)
  
  # 选项2: 最多不可用 Pod 数 (与 minAvailable 互斥)
  # maxUnavailable: 1
  # 可以是整数或百分比字符串:
  # - 整数: 1 (最多 1 个 Pod 不可用)
  # - 百分比: "30%" (最多 30% Pod 不可用)
  
  # ========== v1.26+ 新增字段 ==========
  
  # 不健康 Pod 驱逐策略 (v1.26 Alpha → v1.27 Beta → v1.31 GA)
  unhealthyPodEvictionPolicy: IfHealthyPodCount  # 或 AlwaysAllow
  # - IfHealthyPodCount: 仅当健康 Pod 数满足 PDB 要求时才允许驱逐不健康 Pod (默认, 推荐)
  # - AlwaysAllow: 始终允许驱逐不健康 Pod (旧行为, 兼容性)

# ========== 状态字段 (status) ==========
status:
  # 当前可用的健康 Pod 数
  currentHealthy: 5
  # PDB 要求的最少可用 Pod 数
  desiredHealthy: 3
  # 允许驱逐的 Pod 数
  disruptionsAllowed: 2
  # 期望的健康 Pod 数 (通常等于 desiredHealthy)
  expectedPods: 5
  # 观测到的 Pod 总数 (包括 Terminating)
  observedGeneration: 1
  # 条件状态
  conditions:
  - type: DisruptionAllowed
    status: "True"
    reason: SufficientPods
    lastTransitionTime: "2026-02-10T10:00:00Z"
```

### 字段详解

#### selector

```yaml
# 标签选择器 (与 Service/Deployment 选择器相同语法)
selector:
  matchLabels:
    app: database
  matchExpressions:
  - key: tier
    operator: In
    values: ["backend", "cache"]
```

**注意事项**:
- 必须精确匹配目标 Pod 的标签
- 与 Deployment/StatefulSet 的 selector 保持一致
- 避免选择器过于宽泛（可能误保护其他应用）

#### minAvailable

**整数模式**:
```yaml
minAvailable: 3  # 始终保持至少 3 个 Pod 可用
```

**百分比模式**:
```yaml
minAvailable: "80%"  # 至少 80% Pod 可用
# 计算: desiredHealthy = ceil(totalPods * 0.8)
# 例如: 5 个 Pod → ceil(5 * 0.8) = 4 个必须可用
```

#### maxUnavailable

**整数模式**:
```yaml
maxUnavailable: 1  # 最多 1 个 Pod 不可用
```

**百分比模式**:
```yaml
maxUnavailable: "25%"  # 最多 25% Pod 不可用
# 计算: desiredHealthy = totalPods - floor(totalPods * 0.25)
# 例如: 8 个 Pod → 8 - floor(8 * 0.25) = 8 - 2 = 6 个必须可用
```

---

## minAvailable vs maxUnavailable

### 选择策略

| 维度               | minAvailable                  | maxUnavailable                |
|--------------------|-------------------------------|-------------------------------|
| **语义**           | 正向约束 (最少保留)            | 反向约束 (最多破坏)            |
| **适用场景**       | 高可用系统 (数据库、核心服务)  | 可容忍中断的服务 (批处理)      |
| **副本数变化影响** | 固定值时: 扩容降低保护力度      | 固定值时: 扩容不影响保护力度    |
| **百分比推荐**     | 业务系统常用 (80%)             | 离线任务常用 (30%)             |

### 对比示例

#### 场景1: 固定整数值

```yaml
# 配置A: minAvailable
---
apiVersion: policy/v1
kind: PodDisruptionBudget
metadata:
  name: pdb-min-available
spec:
  minAvailable: 3
  selector:
    matchLabels:
      app: web
---
# 配置B: maxUnavailable
apiVersion: policy/v1
kind: PodDisruptionBudget
metadata:
  name: pdb-max-unavailable
spec:
  maxUnavailable: 1
  selector:
    matchLabels:
      app: web
```

**效果对比**:

| 副本数 | minAvailable: 3<br>(允许驱逐数) | maxUnavailable: 1<br>(允许驱逐数) |
|--------|--------------------------------|----------------------------------|
| 3      | 0 (必须保留 3 个)               | 1 (最多驱逐 1 个)                |
| 5      | 2 (保留 3, 可驱逐 2)            | 1 (保留 4, 可驱逐 1)             |
| 10     | 7 (保留 3, 可驱逐 7)            | 1 (保留 9, 可驱逐 1)             |

**结论**:
- `minAvailable` 固定值在副本数增加时保护力度减弱
- `maxUnavailable` 固定值保持一致的保护力度（**推荐用于生产**）

#### 场景2: 百分比值

```yaml
# 配置A: minAvailable 百分比
---
apiVersion: policy/v1
kind: PodDisruptionBudget
metadata:
  name: pdb-min-percent
spec:
  minAvailable: "75%"
  selector:
    matchLabels:
      app: api
---
# 配置B: maxUnavailable 百分比
apiVersion: policy/v1
kind: PodDisruptionBudget
metadata:
  name: pdb-max-percent
spec:
  maxUnavailable: "25%"
  selector:
    matchLabels:
      app: api
```

**效果对比**:

| 副本数 | minAvailable: 75%<br>(保留/驱逐) | maxUnavailable: 25%<br>(保留/驱逐) |
|--------|----------------------------------|-----------------------------------|
| 4      | ceil(4*0.75)=3 / 1               | 4-floor(4*0.25)=3 / 1             |
| 8      | ceil(8*0.75)=6 / 2               | 8-floor(8*0.25)=6 / 2             |
| 10     | ceil(10*0.75)=8 / 2              | 10-floor(10*0.25)=7 / 3           |

**关键差异**:
- `minAvailable: "75%"` 使用 `ceil` 向上取整（更保守）
- `maxUnavailable: "25%"` 使用 `floor` 向下取整（更激进）

#### 场景3: 极端情况

**单副本应用**:
```yaml
# ❌ 错误配置
apiVersion: policy/v1
kind: PodDisruptionBudget
metadata:
  name: single-replica-pdb
spec:
  minAvailable: 1
  selector:
    matchLabels:
      app: singleton
# 问题: 无法执行任何维护操作 (drain 会被永久阻塞)
```

**解决方案**:
```yaml
# ✅ 正确配置
spec:
  maxUnavailable: 1  # 允许维护时短暂中断
  # 或者使用百分比
  maxUnavailable: "100%"
```

---

## unhealthyPodEvictionPolicy

> **Feature Gate**: `PDBUnhealthyPodEvictionPolicy`  
> **版本**: v1.26 Alpha → v1.27 Beta → v1.31 GA

### 问题背景

**旧行为 (v1.25 及之前)**:
- 不健康 Pod (Not Ready) 不计入 PDB 的 `disruptionsAllowed`
- 可以随时驱逐不健康 Pod，即使违反 PDB 约束
- **问题**: 滚动更新期间，新 Pod 启动失败时，PDB 失效

**示例场景**:
```yaml
# Deployment: 5 个副本
# PDB: minAvailable: 3

# 滚动更新:
# 1. 创建新 Pod (未就绪) → 不计入 PDB
# 2. 删除旧 Pod → PDB 检查通过 (剩余 4 个健康 Pod)
# 3. 再删除 → PDB 通过 (剩余 3 个)
# 4. 再删除 → PDB 通过 (剩余 2 个)
# 结果: 只剩 2 个健康 Pod, 违反 minAvailable: 3!
```

### 新策略详解

#### IfHealthyPodCount (默认, 推荐)

```yaml
apiVersion: policy/v1
kind: PodDisruptionBudget
metadata:
  name: safe-pdb
spec:
  minAvailable: 3
  selector:
    matchLabels:
      app: critical-service
  # 仅当健康 Pod 数满足 PDB 时才允许驱逐不健康 Pod
  unhealthyPodEvictionPolicy: IfHealthyPodCount
```

**行为**:
- 驱逐前检查: `currentHealthy >= desiredHealthy`
- 不健康 Pod **计入** PDB 保护范围
- **优点**: 真正保证可用性
- **缺点**: 可能导致死锁（见下文）

#### AlwaysAllow (旧行为)

```yaml
apiVersion: policy/v1
kind: PodDisruptionBudget
metadata:
  name: legacy-pdb
spec:
  minAvailable: 3
  selector:
    matchLabels:
      app: web
  # 始终允许驱逐不健康 Pod (v1.25 兼容行为)
  unhealthyPodEvictionPolicy: AlwaysAllow
```

**行为**:
- 不健康 Pod 可随时驱逐，不受 PDB 限制
- **适用场景**: 需要快速清理故障 Pod
- **风险**: 滚动更新失败时可能导致服务中断

### 死锁场景与解决方案

#### 死锁案例

```yaml
# Deployment: 3 个副本
apiVersion: apps/v1
kind: Deployment
metadata:
  name: app
spec:
  replicas: 3
  strategy:
    type: RollingUpdate
    rollingUpdate:
      maxUnavailable: 0  # 不允许不可用
      maxSurge: 1        # 最多增加 1 个
---
# PDB: 必须 3 个可用
apiVersion: policy/v1
kind: PodDisruptionBudget
metadata:
  name: app-pdb
spec:
  minAvailable: 3
  unhealthyPodEvictionPolicy: IfHealthyPodCount
  selector:
    matchLabels:
      app: app
```

**死锁流程**:
```
1. 滚动更新开始
2. 创建第 4 个 Pod (新版本) → 启动失败 (CrashLoopBackOff)
3. 尝试删除第 1 个旧 Pod
   ├─ PDB 检查: currentHealthy=3, desiredHealthy=3
   ├─ 删除会导致 currentHealthy=2
   └─ ❌ 驱逐被 PDB 阻止
4. 新 Pod 无法替换旧 Pod → 滚动更新卡住
```

**解决方案**:

**方案1: 调整 Deployment 策略** (推荐)
```yaml
spec:
  replicas: 3
  strategy:
    type: RollingUpdate
    rollingUpdate:
      maxUnavailable: 1  # 允许 1 个不可用
      maxSurge: 1
```

**方案2: 降低 PDB 要求**
```yaml
spec:
  minAvailable: 2  # 从 3 改为 2
  # 或使用百分比
  minAvailable: "60%"
```

**方案3: 使用 maxUnavailable**
```yaml
spec:
  maxUnavailable: 1  # 明确允许 1 个不可用
```

**方案4: 切换到 AlwaysAllow** (临时)
```yaml
spec:
  unhealthyPodEvictionPolicy: AlwaysAllow
  # 仅在紧急情况使用
```

---

## 内部原理

### Eviction API 与 PDB 检查

#### 驱逐流程

```
┌─────────────────────────────────────────────────────────┐
│ kubectl drain node-01                                   │
└────────────────────┬────────────────────────────────────┘
                     │
                     ▼
┌─────────────────────────────────────────────────────────┐
│ Eviction API: POST /api/v1/namespaces/{ns}/pods/{name}/eviction
└────────────────────┬────────────────────────────────────┘
                     │
                     ▼
┌─────────────────────────────────────────────────────────┐
│ API Server: PDB Admission Controller                    │
│ 1. 查找匹配的 PDB 对象                                   │
│ 2. 计算 disruptionsAllowed                              │
│ 3. 检查 unhealthyPodEvictionPolicy                      │
└────────────────────┬────────────────────────────────────┘
                     │
        ┌────────────┼────────────┐
        │            │            │
        ▼            ▼            ▼
   ✅ 允许驱逐   ❌ 拒绝驱逐   ⏳ 等待重试
   └─> 删除 Pod  └─> 返回 429  └─> drain 等待
```

#### disruptionsAllowed 计算

**算法**:
```go
// 伪代码
func calculateDisruptionsAllowed(pdb PDB, pods []Pod) int {
  // 统计健康 Pod 数
  currentHealthy := 0
  for _, pod := range pods {
    if isPodHealthy(pod) {
      currentHealthy++
    }
  }
  
  // 计算期望健康数
  desiredHealthy := calculateDesiredHealthy(pdb, len(pods))
  
  // 计算允许驱逐数
  disruptionsAllowed := currentHealthy - desiredHealthy
  
  // unhealthyPodEvictionPolicy 逻辑
  if pdb.Spec.UnhealthyPodEvictionPolicy == "IfHealthyPodCount" {
    // 新策略: 必须满足健康 Pod 数才允许驱逐不健康 Pod
    if currentHealthy < desiredHealthy {
      return 0  // 阻止所有驱逐
    }
  }
  
  return max(0, disruptionsAllowed)
}
```

**示例计算**:
```yaml
# PDB 配置
minAvailable: 3

# 当前 Pod 状态
总 Pod 数: 5
健康 Pod: 3
不健康 Pod: 2

# 计算
desiredHealthy = 3
currentHealthy = 3
disruptionsAllowed = 3 - 3 = 0

# 结论: 不允许驱逐任何 Pod (包括不健康 Pod, 如果 policy=IfHealthyPodCount)
```

### Voluntary vs Involuntary Disruption

#### Voluntary Disruption (主动中断)

**PDB 生效的场景**:
```yaml
# 1. kubectl drain (排空节点)
kubectl drain node-01 --ignore-daemonsets --delete-emptydir-data

# 2. kubectl delete pod (手动删除)
kubectl delete pod myapp-abc123

# 3. Deployment 滚动更新
kubectl set image deployment/myapp app=myapp:v2

# 4. Cluster Autoscaler 缩容
# 自动选择可安全驱逐的节点

# 5. kubectl cordon + evict
kubectl cordon node-01
kubectl delete pod myapp-abc123 --grace-period=30
```

**API 特征**: 使用 **Eviction API** 删除 Pod
```bash
# Eviction API 请求
curl -X POST \
  https://api-server/api/v1/namespaces/default/pods/myapp-abc123/eviction \
  -H "Content-Type: application/json" \
  -d '{"apiVersion":"policy/v1","kind":"Eviction","metadata":{"name":"myapp-abc123"}}'

# PDB 检查在此拦截
```

#### Involuntary Disruption (非主动中断)

**PDB 不生效的场景**:
```yaml
# 1. 硬件故障
# 节点断电、磁盘损坏

# 2. 内核崩溃
# Kernel Panic、OOM Killer

# 3. 网络分区
# 节点失联 (NotReady)

# 4. 直接删除 (绕过 Eviction API)
kubectl delete pod myapp-abc123 --force --grace-period=0

# 5. 节点对象删除
kubectl delete node node-01
```

**API 特征**: 直接调用 **DELETE API** (不经过 PDB 检查)

### PDB Controller 工作机制

#### Controller 循环

```go
// PDB Controller 主循环 (简化版)
func (c *DisruptionController) Run() {
  for {
    // 1. 监听 PDB 对象变化
    pdb := c.waitForPDBChange()
    
    // 2. 查找匹配的 Pod
    pods := c.listPods(pdb.Spec.Selector)
    
    // 3. 更新 PDB 状态
    status := c.calculatePDBStatus(pdb, pods)
    c.updatePDBStatus(pdb, status)
    
    // 4. 更新 Eviction API 缓存
    c.updateEvictionCache(pdb, status.DisruptionsAllowed)
  }
}
```

#### 状态更新示例

```yaml
# PDB 对象
spec:
  minAvailable: 3
  selector:
    matchLabels:
      app: web

# 当前 Pod 状态
# web-1: Running, Ready
# web-2: Running, Ready
# web-3: Running, Ready
# web-4: Running, Not Ready
# web-5: Terminating

# 计算后的 status
status:
  currentHealthy: 3       # Running + Ready
  desiredHealthy: 3       # minAvailable
  disruptionsAllowed: 0   # 3 - 3 = 0
  expectedPods: 4         # 不包括 Terminating
  observedGeneration: 1
  conditions:
  - type: DisruptionAllowed
    status: "False"       # disruptionsAllowed = 0
    reason: InsufficientPods
```

---

## 版本兼容性

### Feature Timeline

| 版本  | 变化                                              |
|-------|---------------------------------------------------|
| v1.21 | PDB 从 `policy/v1beta1` 升级到 `policy/v1` GA     |
| v1.25 | `policy/v1beta1` 被废弃                           |
| v1.26 | 新增 `unhealthyPodEvictionPolicy` (Alpha)        |
| v1.27 | `unhealthyPodEvictionPolicy` Beta (默认启用)      |
| v1.31 | `unhealthyPodEvictionPolicy` GA                   |
| v1.32 | 所有特性稳定                                       |

### API 版本迁移

**v1.25 之前**:
```yaml
apiVersion: policy/v1beta1  # 已废弃
kind: PodDisruptionBudget
```

**v1.25 之后**:
```yaml
apiVersion: policy/v1  # 使用稳定版本
kind: PodDisruptionBudget
```

**迁移命令**:
```bash
# 批量转换 PDB
kubectl get pdb -o yaml | \
  sed 's/policy\/v1beta1/policy\/v1/g' | \
  kubectl apply -f -
```

### unhealthyPodEvictionPolicy 兼容性

**v1.26 启用 Alpha 特性**:
```bash
# kube-apiserver 参数
--feature-gates=PDBUnhealthyPodEvictionPolicy=true
```

**v1.27+ 默认行为**:
```yaml
# 未指定时的默认值
spec:
  unhealthyPodEvictionPolicy: IfHealthyPodCount  # v1.27+ 默认
```

**向后兼容**:
```yaml
# 保持旧行为 (v1.25 兼容)
spec:
  unhealthyPodEvictionPolicy: AlwaysAllow
```

---

## 生产案例

### 案例1: 集群升级保护

**场景**: Kubernetes 集群升级，需逐个排空节点。

```yaml
# 核心服务 PDB
---
apiVersion: policy/v1
kind: PodDisruptionBudget
metadata:
  name: api-gateway-pdb
  namespace: production
spec:
  maxUnavailable: 1  # 每次最多 1 个 Pod 不可用
  selector:
    matchLabels:
      app: api-gateway
      env: production
  unhealthyPodEvictionPolicy: IfHealthyPodCount
---
# 数据库 PDB (更严格)
apiVersion: policy/v1
kind: PodDisruptionBudget
metadata:
  name: mysql-pdb
  namespace: production
spec:
  minAvailable: 2  # 至少 2 个副本可用 (主从架构)
  selector:
    matchLabels:
      app: mysql
  unhealthyPodEvictionPolicy: IfHealthyPodCount
---
# 批处理任务 PDB (宽松)
apiVersion: policy/v1
kind: PodDisruptionBudget
metadata:
  name: batch-job-pdb
  namespace: production
spec:
  maxUnavailable: "50%"  # 允许 50% Pod 不可用
  selector:
    matchLabels:
      type: batch-job
  unhealthyPodEvictionPolicy: AlwaysAllow  # 快速清理失败任务
```

**升级流程**:

```bash
# 1. 逐个节点排空
for node in $(kubectl get nodes -o name); do
  echo "升级节点: $node"
  
  # 标记节点不可调度
  kubectl cordon $node
  
  # 排空节点 (PDB 自动保护)
  kubectl drain $node \
    --ignore-daemonsets \
    --delete-emptydir-data \
    --timeout=10m
  # 如果 PDB 阻止, drain 会等待或超时
  
  # 执行节点升级
  ssh $node "apt-get update && apt-get upgrade -y kubelet"
  systemctl restart kubelet
  
  # 恢复节点调度
  kubectl uncordon $node
  
  # 等待节点健康
  kubectl wait --for=condition=Ready node/$node --timeout=5m
done
```

**监控脚本**:

```bash
# 检查 PDB 状态
watch 'kubectl get pdb --all-namespaces -o custom-columns=\
NAMESPACE:.metadata.namespace,\
NAME:.metadata.name,\
MIN-AVAILABLE:.spec.minAvailable,\
MAX-UNAVAILABLE:.spec.maxUnavailable,\
ALLOWED-DISRUPTIONS:.status.disruptionsAllowed,\
CURRENT:.status.currentHealthy,\
DESIRED:.status.desiredHealthy'

# 输出示例:
# NAMESPACE    NAME             MIN  MAX  ALLOWED  CURRENT  DESIRED
# production   api-gateway-pdb  -    1    2        5        4
# production   mysql-pdb        2    -    0        2        2  ← 阻止驱逐
```

### 案例2: 节点排空最佳实践

**场景**: 节点维护前安全排空 Pod。

```yaml
# StatefulSet + PDB
---
apiVersion: apps/v1
kind: StatefulSet
metadata:
  name: elasticsearch
  namespace: logging
spec:
  serviceName: es
  replicas: 5
  selector:
    matchLabels:
      app: elasticsearch
  template:
    metadata:
      labels:
        app: elasticsearch
    spec:
      containers:
      - name: es
        image: elasticsearch:8.10.0
        resources:
          requests:
            cpu: "2"
            memory: "8Gi"
---
# 保护 PDB
apiVersion: policy/v1
kind: PodDisruptionBudget
metadata:
  name: es-pdb
  namespace: logging
spec:
  minAvailable: 4  # 5 副本中至少 4 个可用 (确保集群健康)
  selector:
    matchLabels:
      app: elasticsearch
  unhealthyPodEvictionPolicy: IfHealthyPodCount
```

**安全排空流程**:

```bash
#!/bin/bash
# safe-drain.sh

NODE=$1

echo "检查节点上的 Pod..."
kubectl get pods --all-namespaces -o wide --field-selector spec.nodeName=$NODE

echo "检查相关 PDB..."
kubectl get pdb --all-namespaces

echo "开始排空节点 $NODE..."
kubectl drain $NODE \
  --ignore-daemonsets \
  --delete-emptydir-data \
  --grace-period=300 \  # 5 分钟优雅终止
  --timeout=30m \        # 30 分钟超时
  --pod-selector='!app=system' \  # 跳过系统 Pod
  --dry-run=client      # 先模拟执行

# 确认后执行
read -p "确认执行? (y/n) " -n 1 -r
echo
if [[ $REPLY =~ ^[Yy]$ ]]; then
  kubectl drain $NODE \
    --ignore-daemonsets \
    --delete-emptydir-data \
    --grace-period=300 \
    --timeout=30m
fi
```

**Drain 被阻止时的处理**:

```bash
# 查看阻止原因
kubectl get events --sort-by='.lastTimestamp' | grep -i evict

# 输出示例:
# Cannot evict pod as it would violate the pod's disruption budget.

# 检查具体 PDB
kubectl describe pdb es-pdb -n logging

# Status:
#   Allowed Disruptions:  0  ← 当前不允许驱逐
#   Current:              4
#   Desired:              4

# 解决方案:
# 1. 等待其他 Pod 恢复健康
# 2. 临时调整 PDB (生产环境慎用)
kubectl patch pdb es-pdb -n logging -p '{"spec":{"minAvailable":3}}'

# 3. 强制删除 (绕过 PDB, 危险!)
kubectl delete pod es-2 -n logging --force --grace-period=0
```

### 案例3: 高可用保障体系

**场景**: 多层级应用的完整 PDB 配置。

```yaml
# 1. 前端服务 (可容忍中断)
---
apiVersion: policy/v1
kind: PodDisruptionBudget
metadata:
  name: frontend-pdb
  namespace: app
spec:
  maxUnavailable: "30%"  # 允许 30% Pod 不可用
  selector:
    matchLabels:
      tier: frontend
  unhealthyPodEvictionPolicy: AlwaysAllow
---
# 2. 业务逻辑层 (重要)
apiVersion: policy/v1
kind: PodDisruptionBudget
metadata:
  name: backend-pdb
  namespace: app
spec:
  minAvailable: "80%"  # 至少 80% Pod 可用
  selector:
    matchLabels:
      tier: backend
  unhealthyPodEvictionPolicy: IfHealthyPodCount
---
# 3. 缓存层 (关键)
apiVersion: policy/v1
kind: PodDisruptionBudget
metadata:
  name: redis-pdb
  namespace: app
spec:
  minAvailable: 2  # 至少 2 个 Redis 实例 (主从)
  selector:
    matchLabels:
      app: redis
  unhealthyPodEvictionPolicy: IfHealthyPodCount
---
# 4. 数据库 (最高优先级)
apiVersion: policy/v1
kind: PodDisruptionBudget
metadata:
  name: postgres-pdb
  namespace: app
spec:
  minAvailable: 100%  # 不允许任何主动中断
  selector:
    matchLabels:
      app: postgres
      role: master
  unhealthyPodEvictionPolicy: IfHealthyPodCount
```

**与 HPA 联合使用**:

```yaml
# HPA + PDB 配合
---
apiVersion: autoscaling/v2
kind: HorizontalPodAutoscaler
metadata:
  name: backend-hpa
  namespace: app
spec:
  scaleTargetRef:
    apiVersion: apps/v1
    kind: Deployment
    name: backend
  minReplicas: 10
  maxReplicas: 100
  metrics:
  - type: Resource
    resource:
      name: cpu
      target:
        type: Utilization
        averageUtilization: 70
---
# PDB 确保 HPA 缩容时的高可用
apiVersion: policy/v1
kind: PodDisruptionBudget
metadata:
  name: backend-pdb
  namespace: app
spec:
  minAvailable: "80%"  # 即使缩容也保持 80% 可用
  selector:
    matchLabels:
      app: backend
```

---

## 最佳实践

### 1. 选择合适的策略

**推荐配置表**:

| 应用类型         | 推荐策略                  | 说明                          |
|------------------|---------------------------|-------------------------------|
| 无状态 API       | `maxUnavailable: 1`       | 每次最多 1 个 Pod 不可用       |
| 有状态数据库     | `minAvailable: quorum`    | 保证法定人数 (如 3 副本保留 2) |
| 批处理任务       | `maxUnavailable: "50%"`   | 可容忍大规模中断              |
| 单副本服务       | `maxUnavailable: 1`       | 允许维护时短暂中断            |
| 缓存层           | `minAvailable: 1`         | 至少 1 个副本保证服务          |
| 核心系统         | `minAvailable: "90%"`     | 极高可用性要求                |

### 2. 避免过度保护

**错误示例**:
```yaml
# ❌ 过度保护导致无法维护
apiVersion: policy/v1
kind: PodDisruptionBudget
metadata:
  name: bad-pdb
spec:
  minAvailable: 100%  # 不允许任何 Pod 不可用
  selector:
    matchLabels:
      app: myapp
```

**正确示例**:
```yaml
# ✅ 合理平衡可用性和可维护性
apiVersion: policy/v1
kind: PodDisruptionBudget
metadata:
  name: good-pdb
spec:
  minAvailable: "90%"  # 允许 10% Pod 不可用
  # 或使用
  maxUnavailable: 1
  selector:
    matchLabels:
      app: myapp
```

### 3. 配合 Deployment 策略

**协同配置**:

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: web-app
spec:
  replicas: 10
  strategy:
    type: RollingUpdate
    rollingUpdate:
      maxUnavailable: 1  # 与 PDB 协调
      maxSurge: 2
  template:
    spec:
      containers:
      - name: app
        image: myapp:v1
---
apiVersion: policy/v1
kind: PodDisruptionBudget
metadata:
  name: web-app-pdb
spec:
  maxUnavailable: 2  # 大于等于 Deployment.maxUnavailable
  selector:
    matchLabels:
      app: web-app
```

**计算关系**:
```
Deployment.maxUnavailable ≤ PDB.maxUnavailable
或
Deployment.maxUnavailable ≤ (totalReplicas - PDB.minAvailable)
```

### 4. 监控与告警

**Prometheus 告警规则**:

```yaml
groups:
- name: pdb-alerts
  rules:
  # PDB 不允许任何驱逐
  - alert: PDBDisruptionsNotAllowed
    expr: |
      kube_poddisruptionbudget_status_pod_disruptions_allowed == 0
    for: 30m
    labels:
      severity: warning
    annotations:
      summary: "PDB {{ $labels.namespace }}/{{ $labels.poddisruptionbudget }} 不允许驱逐"
      description: "可能阻止节点排空或滚动更新"
  
  # PDB 当前健康数不足
  - alert: PDBCurrentHealthyBelowDesired
    expr: |
      kube_poddisruptionbudget_status_current_healthy
        <
      kube_poddisruptionbudget_status_desired_healthy
    for: 15m
    labels:
      severity: critical
    annotations:
      summary: "PDB 健康 Pod 数不足"
  
  # PDB 长期阻止驱逐
  - alert: PDBBlockingDrain
    expr: |
      kube_poddisruptionbudget_status_pod_disruptions_allowed == 0
        and
      kube_poddisruptionbudget_status_current_healthy
        ==
      kube_poddisruptionbudget_status_desired_healthy
    for: 2h
    labels:
      severity: warning
    annotations:
      summary: "PDB 可能配置过于严格"
```

### 5. 文档和标注

**推荐在 PDB 中添加注释**:

```yaml
apiVersion: policy/v1
kind: PodDisruptionBudget
metadata:
  name: payment-service-pdb
  namespace: production
  annotations:
    description: "支付服务 PDB,保证集群升级期间至少 3 个副本可用"
    owner: "platform-team@example.com"
    runbook: "https://wiki.example.com/runbooks/pdb-payment"
    last-review-date: "2026-02-01"
spec:
  minAvailable: 3
  selector:
    matchLabels:
      app: payment-service
  unhealthyPodEvictionPolicy: IfHealthyPodCount
```

---

## 常见问题

### Q1: drain 一直卡住不动?

**症状**:
```bash
kubectl drain node-01 --ignore-daemonsets
# 卡住,无输出
```

**排查步骤**:

```bash
# 1. 检查 PDB 状态
kubectl get pdb --all-namespaces

# ALLOWED-DISRUPTIONS = 0 说明被 PDB 阻止
# NAMESPACE    NAME        ALLOWED
# default      myapp-pdb   0  ← 问题

# 2. 查看详细信息
kubectl describe pdb myapp-pdb

# Status:
#   Current Healthy: 3
#   Desired Healthy: 3
#   Disruptions Allowed: 0

# 3. 查看节点上的 Pod
kubectl get pods -o wide --all-namespaces --field-selector spec.nodeName=node-01

# 4. 检查 Pod 健康状态
kubectl get pods -l app=myapp -o custom-columns=\
NAME:.metadata.name,\
STATUS:.status.phase,\
READY:.status.conditions[?(@.type==\"Ready\")].status
```

**解决方案**:

```bash
# 方案1: 等待 Pod 恢复健康 (推荐)
kubectl get pods -l app=myapp --watch

# 方案2: 临时调整 PDB
kubectl patch pdb myapp-pdb -p '{"spec":{"minAvailable":2}}'

# 方案3: 删除 PDB (临时,维护后恢复)
kubectl delete pdb myapp-pdb
# 维护完成后重新创建

# 方案4: 强制删除 Pod (危险!)
kubectl delete pod myapp-abc123 --force --grace-period=0
```

### Q2: PDB 与单副本服务的矛盾?

**问题配置**:
```yaml
# Deployment: 1 个副本
apiVersion: apps/v1
kind: Deployment
metadata:
  name: singleton
spec:
  replicas: 1
---
# PDB: 要求 1 个可用
apiVersion: policy/v1
kind: PodDisruptionBudget
metadata:
  name: singleton-pdb
spec:
  minAvailable: 1  # ❌ 导致永远无法 drain
  selector:
    matchLabels:
      app: singleton
```

**解决方案**:

```yaml
# ✅ 使用 maxUnavailable
apiVersion: policy/v1
kind: PodDisruptionBudget
metadata:
  name: singleton-pdb
spec:
  maxUnavailable: 1  # 允许维护时短暂中断
  selector:
    matchLabels:
      app: singleton
```

**或者不使用 PDB**:
```yaml
# 单副本应用通常不需要 PDB
# 接受维护期间的短暂服务中断
```

### Q3: 滚动更新时 PDB 报错?

**症状**:
```bash
kubectl rollout status deployment/myapp
# Waiting for deployment "myapp" rollout to finish: 0 out of 3 new replicas have been updated...
# (长时间卡住)

kubectl get events | grep -i evict
# Cannot evict pod as it would violate the pod's disruption budget.
```

**原因分析**:
```yaml
# 配置冲突
apiVersion: apps/v1
kind: Deployment
spec:
  replicas: 3
  strategy:
    rollingUpdate:
      maxUnavailable: 0  # 不允许不可用
---
apiVersion: policy/v1
kind: PodDisruptionBudget
spec:
  minAvailable: 3  # 必须 3 个可用
  unhealthyPodEvictionPolicy: IfHealthyPodCount
# 问题: 新 Pod 启动失败时, 旧 Pod 无法删除
```

**解决方案**:

```yaml
# 方案1: 调整 Deployment 策略
spec:
  replicas: 3
  strategy:
    rollingUpdate:
      maxUnavailable: 1  # 允许 1 个不可用
      maxSurge: 1

# 方案2: 降低 PDB 要求
spec:
  minAvailable: 2  # 从 3 改为 2

# 方案3: 切换到 AlwaysAllow (临时)
spec:
  unhealthyPodEvictionPolicy: AlwaysAllow
```

### Q4: 如何测试 PDB 是否生效?

**测试脚本**:

```bash
#!/bin/bash
# test-pdb.sh

NAMESPACE=default
APP=myapp

echo "1. 创建测试 Deployment"
kubectl apply -f - <<EOF
apiVersion: apps/v1
kind: Deployment
metadata:
  name: $APP
  namespace: $NAMESPACE
spec:
  replicas: 5
  selector:
    matchLabels:
      app: $APP
  template:
    metadata:
      labels:
        app: $APP
    spec:
      containers:
      - name: nginx
        image: nginx:1.25
        resources:
          requests:
            cpu: 100m
            memory: 128Mi
EOF

echo "2. 等待 Pod 就绪"
kubectl wait --for=condition=Ready pod -l app=$APP -n $NAMESPACE --timeout=60s

echo "3. 创建 PDB"
kubectl apply -f - <<EOF
apiVersion: policy/v1
kind: PodDisruptionBudget
metadata:
  name: $APP-pdb
  namespace: $NAMESPACE
spec:
  maxUnavailable: 1
  selector:
    matchLabels:
      app: $APP
EOF

echo "4. 检查 PDB 状态"
kubectl get pdb $APP-pdb -n $NAMESPACE
kubectl describe pdb $APP-pdb -n $NAMESPACE

echo "5. 测试驱逐 Pod"
POD=$(kubectl get pod -l app=$APP -n $NAMESPACE -o jsonpath='{.items[0].metadata.name}')
echo "尝试驱逐 Pod: $POD"

kubectl delete pod $POD -n $NAMESPACE &
sleep 5

echo "6. 验证 PDB 保护"
kubectl get pods -l app=$APP -n $NAMESPACE
kubectl get pdb $APP-pdb -n $NAMESPACE -o jsonpath='{.status.disruptionsAllowed}'

echo "7. 清理资源"
kubectl delete deployment $APP -n $NAMESPACE
kubectl delete pdb $APP-pdb -n $NAMESPACE
```

### Q5: unhealthyPodEvictionPolicy 何时使用 AlwaysAllow?

**使用场景**:

1. **兼容旧行为** (v1.25 迁移)
   ```yaml
   spec:
     unhealthyPodEvictionPolicy: AlwaysAllow
   ```

2. **批处理任务** (快速清理失败 Pod)
   ```yaml
   # 允许快速驱逐失败的 Job Pod
   spec:
     maxUnavailable: "50%"
     unhealthyPodEvictionPolicy: AlwaysAllow
   ```

3. **开发/测试环境** (降低维护成本)
   ```yaml
   spec:
     maxUnavailable: "100%"
     unhealthyPodEvictionPolicy: AlwaysAllow
   ```

**生产环境推荐**:
```yaml
# 默认使用 IfHealthyPodCount (更安全)
spec:
  unhealthyPodEvictionPolicy: IfHealthyPodCount
```

---

## 参考资料

- [PDB 官方文档](https://kubernetes.io/docs/tasks/run-application/configure-pdb/)
- [Disruptions 概念](https://kubernetes.io/docs/concepts/workloads/pods/disruptions/)
- [Eviction API 参考](https://kubernetes.io/docs/concepts/scheduling-eviction/api-eviction/)
- [unhealthyPodEvictionPolicy KEP](https://github.com/kubernetes/enhancements/tree/master/keps/sig-apps/3017-pod-healthy-policy-for-pdb)

---

**文档维护**: 建议每季度更新，关注 PDB 新特性和社区最佳实践变化。
