---
title: 'Day 9: 控制平面 - Scheduler + Controller Manager'
description: '- "Scheduler 调度算法是什么"'
summary: '- "Scheduler 调度算法是什么"'
category: learning
tags:
- k8s
- training
- hands-on
- scheduler
- controller-manager
- statefulset
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
- 'Day 9: 控制平面 - Scheduler + Controller Manager 是什么'
- '如何 Day 9: 控制平面 - Scheduler + Controller Manager'
trigger_keywords:
- Day
- '9:'
- 控制平面
- Scheduler
- Controller
- Manager
- learn
prerequisites:
- kubectl-basics
- gpu-ml-basics
- gpu-scheduling-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Day 9: 控制平面 - Scheduler + Controller Manager

```yaml
---
id: LEARN-ONE-MONTH-W2-DAY9
title: Day 9 - 控制平面 - Scheduler + Controller Manager
topic: [[实体/kubernetes.md|kubernetes]]
type: hands-on-guide
tags: [scheduler, controller-manager, affinity, taint, toleration, nodeelector, hands-on, week-2]
last_updated: 2026-05-18
difficulty: intermediate
intent_queries:
  - "Scheduler 调度算法是什么"
  - "Filter/Score 阶段怎么做"
  - "nodeSelector/nodeAffinity 怎么用"
  - "Taint/Toleration 区别"
  - "Controller Manager 工作原理"
trigger_keywords:
  - Scheduler
  - Filter
  - Score
  - Scheduling
  - nodeSelector
  - nodeAffinity
  - podAffinity
  - podAntiAffinity
  - Taint
  - Toleration
  - Controller Manager
  - Reconcile
  - 调度
  - 亲和性
  - 污点
reading_level: intermediate
audience:
  - sre
  - ops-engineer
estimated_read_time: 45min
related_domains:
  - 集群基础
  - 故障诊断
related_topics:
  - control-plane
  - scheduler
  - scheduling
  - affinity
  - taint
related:
  - 生产运维/topic-learn/public-training/one-month/week-2-core-tech/day-8-control-plane-1.md
  - 集群基础/20-kube-scheduler-deep-dive.md
---
```

> **学习时间**: 4-5 小时 | **主题**: K8s 调度与控制循环

---

## 今日目标

- [ ] 理解 Scheduler 的调度算法 (Filter + Score)
- [ ] 掌握 Controller Manager 的控制循环模式
- [ ] 能够配置调度约束 (nodeSelector, affinity, taints)

---

## 理论学习 (2h)

### 必读文档

1. **Scheduler 深入**
   - 文件: `../../集群基础/20-kube-scheduler-deep-dive.md`
   - 重点: 调度算法、Filter/Score、调度框架

2. **Controller Manager 深入**
   - 文件: `../../集群基础/13-kube-controller-manager-deep-dive.md`
   - 重点: 各种 Controller 的工作原理

3. **控制器模式**
   - 文件: `../../集群基础/03-controller-pattern.md`
   - 重点: Reconcile 循环、声明式管理

---

## 实践任务 (2.5h)

### 任务 1: 调度过程观察 (45min)

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl delete`：删除资源（可由声明式清单重建）

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 创建 Pod 并观察调度事件
kubectl run scheduler-test --image=nginx:alpine
kubectl describe pod scheduler-test | grep -A20 Events

# 查看调度结果
kubectl get pod scheduler-test -o wide

# 查看 Scheduler 日志
kubectl logs -n kube-system -l component=kube-scheduler --tail=100

# 清理
kubectl delete pod scheduler-test
```
### 任务 2: nodeSelector 实践 (30min)

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源
> - `kubectl delete`：删除资源（可由声明式清单重建）
> - `kubectl label/annotate`：改元数据可能影响选择器/控制器

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 查看节点标签
kubectl get nodes --show-labels

# 给节点添加标签
kubectl label node <node-name> disktype=ssd

# 创建使用 nodeSelector 的 Pod
cat > nodeselector-pod.yaml << 'EOF'
apiVersion: v1
kind: Pod
metadata:
  name: nodeselector-test
spec:
  nodeSelector:
    disktype: ssd
  containers:
  - name: nginx
    image: nginx:alpine
EOF

kubectl apply -f nodeselector-pod.yaml

# 验证调度
kubectl get pod nodeselector-test -o wide

# 清理
kubectl delete pod nodeselector-test
kubectl label node <node-name> disktype-
```
### 任务 3: Affinity 实践 (45min)

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源
> - `kubectl delete`：删除资源（可由声明式清单重建）

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# Node Affinity
cat > node-affinity.yaml << 'EOF'
apiVersion: v1
kind: Pod
metadata:
  name: node-affinity-test
spec:
  affinity:
    nodeAffinity:
      requiredDuringSchedulingIgnoredDuringExecution:
        nodeSelectorTerms:
        - matchExpressions:
          - key: kubernetes.io/os
            operator: In
            values:
            - linux
      preferredDuringSchedulingIgnoredDuringExecution:
      - weight: 1
        preference:
          matchExpressions:
          - key: disktype
            operator: In
            values:
            - ssd
  containers:
  - name: nginx
    image: nginx:alpine
EOF

kubectl apply -f node-affinity.yaml
kubectl describe pod node-affinity-test | grep -A10 "Node-Selectors"

# Pod Anti-Affinity (让 Pod 分散到不同节点)
cat > pod-antiaffinity.yaml << 'EOF'
apiVersion: apps/v1
kind: Deployment
metadata:
  name: web-spread
spec:
  replicas: 3
  selector:
    matchLabels:
      app: web-spread
  template:
    metadata:
      labels:
        app: web-spread
    spec:
      affinity:
        podAntiAffinity:
          preferredDuringSchedulingIgnoredDuringExecution:
          - weight: 100
            podAffinityTerm:
              labelSelector:
                matchExpressions:
                - key: app
                  operator: In
                  values:
                  - web-spread
              topologyKey: kubernetes.io/hostname
      containers:
      - name: nginx
        image: nginx:alpine
EOF

kubectl apply -f pod-antiaffinity.yaml
kubectl get pods -l app=web-spread -o wide

# 清理
kubectl delete -f node-affinity.yaml
kubectl delete -f pod-antiaffinity.yaml
```
### 任务 4: Taints 和 Tolerations (30min)

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `kubectl taint nodes`：变更污点影响 Pod 调度
> - `kubectl apply/create/replace`：创建/变更集群资源
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
# 查看节点的 Taints
kubectl describe nodes | grep Taints

# 给节点添加 Taint
kubectl taint nodes <node-name> dedicated=special:NoSchedule

# 创建普通 Pod (会被 Taint 阻止)
kubectl run taint-test --image=nginx:alpine
kubectl describe pod taint-test | grep -A5 Events  # 观察 Pending

# 创建带 Toleration 的 Pod
cat > toleration-pod.yaml << 'EOF'
apiVersion: v1
kind: Pod
metadata:
  name: toleration-test
spec:
  tolerations:
  - key: "dedicated"
    operator: "Equal"
    value: "special"
    effect: "NoSchedule"
  containers:
  - name: nginx
    image: nginx:alpine
EOF

kubectl apply -f toleration-pod.yaml
kubectl get pod toleration-test -o wide

# 清理
kubectl delete pod taint-test toleration-test
kubectl taint nodes <node-name> dedicated-
```
### 任务 5: Controller 行为观察 (30min)

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源
> - `kubectl delete`：删除资源（可由声明式清单重建）

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 创建 Deployment
kubectl create deployment controller-test --image=nginx:alpine --replicas=3

# 观察 ReplicaSet Controller 行为
kubectl get replicaset -w &

# 手动删除一个 Pod
kubectl delete pod $(kubectl get pods -l app=controller-test -o jsonpath='{.items[0].metadata.name}')

# 观察 Controller 自动创建新 Pod

# 修改 Deployment 镜像，观察滚动更新
kubectl set image deployment/controller-test nginx=nginx:1.25

# 观察多个 ReplicaSet
kubectl get replicaset -l app=controller-test

# 清理
kubectl delete deployment controller-test
```
---

## 费曼复述 (0.5h)

用自己的语言回答:

1. **Scheduler 的 Filter 和 Score 阶段分别做什么？**
   - Filter: 过滤不满足条件的节点
   - Score: 对剩余节点打分，选择最高分

2. **什么是 Reconcile 循环？Controller 如何工作？**
   - 持续观察当前状态
   - 与期望状态对比
   - 执行操作使当前状态趋向期望状态

3. **Taint 和 Toleration 的使用场景是什么？**
   - 专用节点 (GPU、高内存)
   - 驱逐 Pod
   - 节点维护

---

## 今日检验

- [ ] 能够配置 nodeSelector 控制调度
- [ ] 能够使用 affinity 实现高级调度策略
- [ ] 能够配置 taints/tolerations
- [ ] 理解 Controller 的 Reconcile 循环

---

## 调度约束对比

| 方式 | 硬性/软性 | 使用场景 |
|------|----------|----------|
| nodeSelector | 硬性 | 简单节点选择 |
| nodeAffinity required | 硬性 | 复杂节点选择 |
| nodeAffinity preferred | 软性 | 偏好但非必须 |
| podAffinity | 软/硬 | Pod 共置 |
| podAntiAffinity | 软/硬 | Pod 分散 |
| taints/tolerations | 硬性 | 专用节点、驱逐 |

---

## 明日预告

Day 10 将学习工作负载资源: Deployment、[[StatefulSet|StatefulSet]]、[[DaemonSet|DaemonSet]]，理解不同应用类型的管理方式。


<!-- risk-assessed -->
