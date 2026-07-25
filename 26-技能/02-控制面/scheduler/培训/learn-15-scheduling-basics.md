---
title: 第15课：调度与亲和性
description: '# 第15课：调度与亲和性'
summary: 'kubectl describe pod <pod-name> | grep -A10 "Tolerations"'
category: skills
tags:
- k8s
- learn
- fundamentals
- redis
- pdb
- statefulset
- daemonset
- operator
- gpu
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 15min
intent_queries:
- 第15课：调度与亲和性 是什么
- 如何 第15课：调度与亲和性
trigger_keywords:
- 第15课：调度与亲和性
prerequisites:
- kubectl-basics
- redis-basics
- gpu-scheduling-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 第15课：调度与亲和性

> **章节**: 入门引导 | **难度**: 入门 | **时长**: 25 分钟

---

## 学习目标

1. 理解调度器的工作原理
2. 掌握污点（Taints）和容忍（Tolerations）的用法
3. 学会使用节点亲和性控制 Pod 调度
4. 了解 Pod 亲和性和反亲和性

---

## 1. 问题引入

### 1.1 问题场景

```
【场景】

你的集群有 3 类节点：
• 普通节点 - 运行普通应用
• GPU 节点 - 运行机器学习任务
• 内存优化节点 - 运行大内存数据库

问题：
• 如何让 Web 应用只跑在普通节点？
• 如何让机器学习任务只跑在 GPU 节点？
• 如何让数据库只跑在内存优化节点？

【解决方案】

调度策略！

K8s 调度器可以：
• 通过污点（Taints）标记节点"不欢迎某些 Pod"
• 通过容忍（Tolerations）让 Pod "接受某些污点"
• 通过亲和性（Affinity）让 Pod "喜欢某些节点"
```

### 1.2 调度器工作原理

```
# 🟢 低风险：只读/信息收集，通常无副作用
【调度流程】

Pod 创建请求 → API Server → 调度器 → 选择节点 → 绑定 Pod

【调度器考虑因素】

1. 资源需求
   CPU、内存是否足够

2. 亲和性/反亲和性
   Pod 是否要求在特定节点运行

3. 污点和容忍
   节点是否排斥 Pod，Pod 是否接受排斥

4. 拓扑约束
   Pod 是否需要分布在不同可用区

5. 优先级和抢占
   高优先级 Pod 可以抢占低优先级 Pod

【查看调度决策】

kubectl describe pod <pod-name> | grep -A10 "Node Selectors"
kubectl describe pod <pod-name> | grep -A10 "Tolerations"
```
---

## 2. 污点（Taints）与容忍（Tolerations）

### 2.1 污点的概念

```
【概念】

污点是节点的"排斥"机制：
• 节点可以标记"我只接受有特定容忍的 Pod"
• 没有匹配容忍的 Pod 不会被调度到该节点

【效果】

NoSchedule：
• 不调度不匹配的 Pod 到该节点
• 不影响已运行的 Pod

NoExecute：
• 不调度不匹配的 Pod
• 驱逐已运行的不匹配 Pod（如果没有对应容忍）

 PreferNoSchedule：
• 尽量不调度，但如果没有其他选择还是会调度

【类比】

节点污点 = 宿舍楼的门禁
• 门禁只允许特定人员进入
• 没有门禁卡的人不能进
• 除非你申请了临时通行证（容忍）
```

### 2.2 添加和删除污点

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `kubectl taint nodes`：变更污点影响 Pod 调度

> **🔴 高风险操作警告**
>
> 下方命令属于不可逆或高影响操作，执行前请确认：
> - 已备份关键数据与配置
> - 处于批准的变更窗口期
> - 已获得相关责任人授权
> - 已准备回滚或恢复方案
> - 目标集群、Namespace、节点/资源名称正确无误

```
# 🔴 高风险：可能造成数据丢失或服务中断，执行前需备份、变更审批与回滚方案
【给节点添加污点】

kubectl taint nodes node1 dedicated=gpu:NoSchedule

格式：key=value:effect
• key = dedicated
• value = gpu
• effect = NoSchedule

【添加多个污点】

kubectl taint nodes node1 dedicated=gpu:NoSchedule
kubectl taint nodes node1 disk=ssd:NoExecute

【删除污点】

kubectl taint nodes node1 dedicated=gpu:NoSchedule-

【查看节点的污点】

kubectl describe node node1 | grep Taints

输出示例：
Taints: dedicated=gpu:NoSchedule, disk=ssd:NoExecute
```
### 2.3 Pod 添加容忍

```
【基础容忍】

spec:
  tolerations:
  - key: "dedicated"
    operator: "Equal"
    value: "gpu"
    effect: "NoSchedule"

【匹配任意值】

spec:
  tolerations:
  - key: "dedicated"
    operator: "Exists"     # 存在这个 key 就行，value 任意
    effect: "NoSchedule"

【容忍所有污点】

spec:
  tolerations:
  - operator: "Exists"     # 匹配所有污点
    effect: NoSchedule

【容忍特定 key 的所有 effect】

spec:
  tolerations:
  - key: "dedicated"
    operator: "Exists"     # 存在 dedicated key 就行
    # 不需要指定 effect，默认容忍所有

【容忍并允许驱逐（NoExecute）】

spec:
  tolerations:
  - key: "dedicated"
    operator: "Equal"
    value: "gpu"
    effect: "NoExecute"
    tolerationSeconds: 300  # 300 秒后才驱逐
```

### 2.4 常见场景

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `kubectl taint nodes`：变更污点影响 Pod 调度

> **🔴 高风险操作警告**
>
> 下方命令属于不可逆或高影响操作，执行前请确认：
> - 已备份关键数据与配置
> - 处于批准的变更窗口期
> - 已获得相关责任人授权
> - 已准备回滚或恢复方案
> - 目标集群、Namespace、节点/资源名称正确无误

```
# 🔴 高风险：可能造成数据丢失或服务中断，执行前需备份、变更审批与回滚方案
【场景一：专用 GPU 节点】

# 节点添加污点
kubectl taint nodes gpu-node dedicated=gpu:NoSchedule

# Pod 添加容忍
spec:
  tolerations:
  - key: "dedicated"
    operator: "Equal"
    value: "gpu"
    effect: "NoSchedule"
  containers:
  - name: tensorflow
    image: tensorflow:latest

【场景二：维护期间的临时容忍】

spec:
  tolerations:
  - key: "node.kubernetes.io/unreachable"
    operator: "Exists"
    effect: "NoExecute"
    tolerationSeconds: 300
```
---

## 3. 节点亲和性

### 3.1 亲和性类型

```
【节点亲和性】

nodeAffinity = 我（Pod）喜欢/不喜欢某些节点

【类型】

requiredDuringSchedulingIgnoredDuringExecution：
• 硬亲和性，必须满足
• 调度时必须满足，节点运行时忽略

preferredDuringSchedulingIgnoredDuringExecution：
• 软亲和性，尽量满足
• 调度时尽量满足，不强求

【未来支持】
• requiredDuringSchedulingRequiredDuringExecution
  节点运行时也要满足，不满足就驱逐
```

### 3.2 nodeSelector（简单方式）

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl label/annotate`：改元数据可能影响选择器/控制器

```
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
【最简单的节点选择】

spec:
  nodeSelector:
    disktype: ssd

【给节点添加标签】

kubectl label nodes node1 disktype=ssd

【查看标签】

kubectl get nodes --show-labels | grep disktype
```
### 3.3 nodeAffinity 详细配置

```
【硬亲和性 - 必须满足】

spec:
  affinity:
    nodeAffinity:
      requiredDuringSchedulingIgnoredDuringExecution:
        nodeSelectorTerms:
        - matchExpressions:
          - key: "kubernetes.io/os"
            operator: In
            values:
            - linux
          - key: "disktype"
            operator: NotIn
            values:
            - ssd

【软亲和性 - 尽量满足】

spec:
  affinity:
    nodeAffinity:
      preferredDuringSchedulingIgnoredDuringExecution:
      - weight: 1
        preference:
          matchExpressions:
          - key: "disktype"
            operator: In
            values:
            - ssd
      - weight: 50
        preference:
          matchExpressions:
          - key: "memory-size"
            operator: In
            values:
            - large

weight: 权重，越大越优先
```

### 3.4 运算符

```
【In - 匹配值列表】

operator: In
values: ["linux", "windows"]

【NotIn - 排除值列表】

operator: NotIn
values: ["windows"]

【Exists - key 存在即可】

operator: Exists
# 不需要指定 values

【DoesNotExist - key 不存在】

operator: DoesNotExist
# 不需要指定 values

【Gt/Lt - 数值比较】

operator: Gt
values: ["2"]  # 大于 2

operator: Lt
values: ["4"]  # 小于 4
```

---

## 4. Pod 亲和性与反亲和性

### 4.1 Pod 亲和性概念

```
【概念】

Pod 亲和性 = Pod 想要和某些 Pod 调度到同一区域
Pod 反亲和性 = Pod 不想和某些 Pod 调度到同一区域

【应用场景】

1. 将 Web 应用和缓存放在一起（减少延迟）
   → Pod 亲和性

2. 将多个副本分散到不同节点（提高可用性）
   → Pod 反亲和性

3. 将 Web 应用和数据库放不同可用区（防单点问题）
   → Pod 反亲和性
```

### 4.2 Pod 反亲和性配置

```
【将 Pod 分散到不同节点】

spec:
  affinity:
    podAntiAffinity:
      requiredDuringSchedulingIgnoredDuringExecution:
      - labelSelector:
          matchExpressions:
          - key: app
            operator: In
            values:
            - web
        topologyKey: kubernetes.io/hostname

解释：
• app=web 的 Pod 尽量不要调度到同一节点
• kubernetes.io/hostname 表示按节点划分

【软反亲和性 - 尽量分散】

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
              - web
          topologyKey: kubernetes.io/hostname
```

### 4.3 Pod 亲和性配置

```
【将 Web 和 Redis 调度到一起】

spec:
  affinity:
    podAffinity:
      requiredDuringSchedulingIgnoredDuringExecution:
      - labelSelector:
          matchExpressions:
          - key: app
            operator: In
            values:
            - redis
        topologyKey: kubernetes.io/hostname

【按区域分散】

topologyKey 可以是：
• kubernetes.io/hostname - 同一节点
• topology.kubernetes.io/zone - 同一可用区
• topology.kubernetes.io/region - 同一地域
```

---

## 5. 实际应用场景

### 5.1 场景一：GPU 训练任务

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `kubectl taint nodes`：变更污点影响 Pod 调度
> - `kubectl label/annotate`：改元数据可能影响选择器/控制器

> **🔴 高风险操作警告**
>
> 下方命令属于不可逆或高影响操作，执行前请确认：
> - 已备份关键数据与配置
> - 处于批准的变更窗口期
> - 已获得相关责任人授权
> - 已准备回滚或恢复方案
> - 目标集群、Namespace、节点/资源名称正确无误

```
# 🔴 高风险：可能造成数据丢失或服务中断，执行前需备份、变更审批与回滚方案
【节点配置】

# GPU 节点标记标签
kubectl label nodes gpu-node gpu=true

# GPU 节点添加污点（可选，让普通 Pod 不调度过来）
kubectl taint nodes gpu-node dedicated=gpu:NoSchedule

【Pod 配置】

spec:
  containers:
  - name: tensorflow
    image: tensorflow:latest
  tolerations:
  - key: "dedicated"
    operator: "Equal"
    value: "gpu"
    effect: "NoSchedule"
  nodeSelector:
    gpu: "true"
```
### 5.2 场景二：多副本高可用

```
【Deployment 配置】

spec:
  replicas: 3
  affinity:
    podAntiAffinity:
      requiredDuringSchedulingIgnoredDuringExecution:
      - labelSelector:
          matchExpressions:
          - key: app
            operator: In
            values:
            - my-app
        topologyKey: kubernetes.io/hostname

确保 3 个副本分布在 3 个不同节点上。
```

### 5.3 场景三：灰度发布

```
【新版本只调度到特定节点】

节点标签：version=stable

spec:
  affinity:
    nodeAffinity:
      requiredDuringSchedulingIgnoredDuringExecution:
        nodeSelectorTerms:
        - matchExpressions:
          - key: "version"
            operator: In
            values:
            - stable
            - canary

这样可以让新版本 Pod 调度到预发布节点。
```

---

## 6. 常见问题

### 6.1 Pod 无法调度到任何节点

```
# 🟢 低风险：只读/信息收集，通常无副作用
【排查步骤】

1. 检查节点状态
   kubectl get nodes

2. 检查资源是否足够
   kubectl describe nodes | grep -A5 "Allocated resources"

3. 检查污点
   kubectl describe nodes | grep Taints

4. 检查亲和性
   kubectl describe pod <pod-name> | grep -A10 "Affinity"

5. 查看调度失败原因
   kubectl get events --sort-by=.lastTimestamp | tail -20
```
### 6.2 污点容忍不生效

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `kubectl taint nodes`：变更污点影响 Pod 调度

> **🔴 高风险操作警告**
>
> 下方命令属于不可逆或高影响操作，执行前请确认：
> - 已备份关键数据与配置
> - 处于批准的变更窗口期
> - 已获得相关责任人授权
> - 已准备回滚或恢复方案
> - 目标集群、Namespace、节点/资源名称正确无误

```
# 🔴 高风险：可能造成数据丢失或服务中断，执行前需备份、变更审批与回滚方案
【排查步骤】

1. 检查污点名称是否正确
   kubectl describe node <node-name> | grep Taints

2. 检查 Pod 容忍配置
   kubectl describe pod <pod-name> | grep -A10 "Tolerations"

3. 确认 key、value、effect 完全匹配
   kubectl taint nodes node1 dedicated=gpu:NoSchedule
   Pod 容忍需要 exact match！

4. 注意 operator 区别
   operator: Equal → key=value 必须完全匹配
   operator: Exists → 只检查 key 是否存在
```
### 6.3 亲和性规则冲突

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl label/annotate`：改元数据可能影响选择器/控制器

```
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
【场景】

Pod 同时有 nodeSelector 和 nodeAffinity：
• nodeSelector: disktype=ssd
• nodeAffinity: 硬亲和要求 memory=large

如果没有任何节点同时满足这两个条件，Pod 会 Pending。

【解决方案】

1. 检查节点标签
   kubectl get nodes --show-labels

2. 给需要的节点添加标签
   kubectl label node <node-name> disktype=ssd memory=large

3. 或者放宽亲和性要求
   从 required 改为 preferred
```
---

## 7. 数字人 Q&A 场景

### 7.1 用户问：污点和节点选择器有什么区别？

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `kubectl taint nodes`：变更污点影响 Pod 调度

> **🔴 高风险操作警告**
>
> 下方命令属于不可逆或高影响操作，执行前请确认：
> - 已备份关键数据与配置
> - 处于批准的变更窗口期
> - 已获得相关责任人授权
> - 已准备回滚或恢复方案
> - 目标集群、Namespace、节点/资源名称正确无误

```
# 🔴 高风险：可能造成数据丢失或服务中断，执行前需备份、变更审批与回滚方案
【回复】

"好问题！让我来解释一下区别：

【节点选择器（nodeSelector）】

Pod 主动说："我想去有这个标签的节点"
节点被动接受。

例子：
nodeSelector:
  disktype: ssd

效果：Pod 只调度到有 disktype=ssd 标签的节点。

【污点（Taints）】

节点主动说："我不欢迎这些 Pod"
Pod 被动接受（通过容忍）。

例子：
kubectl taint nodes node1 dedicated=gpu:NoSchedule

效果：普通 Pod 不能调度到 node1，除非有对应的容忍。

【组合使用】

场景：GPU 集群
1. GPU 节点添加污点：kubectl taint nodes gpu-node dedicated=gpu:NoSchedule
2. GPU 任务 Pod 添加容忍：tolerations + nodeSelector

这样：
• 普通 Pod 不会调度到 GPU 节点（被污点阻止）
• GPU 任务 Pod 可以调度到 GPU 节点（有容忍 + 节点选择）

有其他问题吗？"
```
### 7.2 用户问：如何让 Pod 分散到不同节点？

```
【回复】

"想让 Pod 分散到不同节点，用 Pod 反亲和性！

【配置】

spec:
  affinity:
    podAntiAffinity:
      requiredDuringSchedulingIgnoredDuringExecution:
      - labelSelector:
          matchExpressions:
          - key: app
            operator: In
            values:
            - my-app
        topologyKey: kubernetes.io/hostname

【解释】

• app=my-app 的 Pod 不要调度到同一节点
• topologyKey: kubernetes.io/hostname 表示按节点划分

【效果】

如果有 3 个节点：
• my-app-0 → node1
• my-app-1 → node2
• my-app-2 → node3

每个副本在不同节点，提高可用性。

【软分散（推荐）】

如果要求不那么严格，可以用 preferred：
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
              - my-app
          topologyKey: kubernetes.io/hostname

这样如果节点不够，Pod 还是能调度，只是尽量分散。

有其他问题吗？"
```

---

## 8. 总结

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `kubectl taint nodes`：变更污点影响 Pod 调度
> - `kubectl label/annotate`：改元数据可能影响选择器/控制器

> **🔴 高风险操作警告**
>
> 下方命令属于不可逆或高影响操作，执行前请确认：
> - 已备份关键数据与配置
> - 处于批准的变更窗口期
> - 已获得相关责任人授权
> - 已准备回滚或恢复方案
> - 目标集群、Namespace、节点/资源名称正确无误

```
# 🔴 高风险：可能造成数据丢失或服务中断，执行前需备份、变更审批与回滚方案
【命令速查】

添加污点：
kubectl taint nodes <node> key=value:effect

删除污点：
kubectl taint nodes <node> key=value:effect-

查看污点：
kubectl describe node <node> | grep Taints

查看标签：
kubectl get nodes --show-labels

添加标签：
kubectl label nodes <node> key=value

【调度策略对比】

| 策略 | 作用对象 | 方向 |
|------|---------|------|
| nodeSelector | Pod | Pod 选择节点 |
| 污点 (Taints) | 节点 | 节点排斥 Pod |
| 节点亲和性 | Pod | Pod 选择节点 |
| Pod 亲和性 | Pod | Pod 选择其他 Pod |

【核心要点】

1. 污点让节点"排斥"不匹配的 Pod
2. 容忍让 Pod"接受"被排斥的节点
3. 节点亲和性让 Pod"喜欢"特定节点
4. Pod 反亲和性让 Pod"远离"特定 Pod
5. topologyKey 控制分布范围（节点/可用区/地域）

【下节课预告】

下节课我们会学习 PodDisruptionBudget：
• 什么是 PDB
• 如何保护关键应用
• 如何进行安全维护

有问题吗？"
```
---

**关联文档**:
- [../10-advanced-workloads/10-daemonset-basics.md](../10-advanced-workloads/10-daemonset-basics.md) — [[DaemonSet|DaemonSet]]
- [../10-advanced-workloads/11-statefulset-basics.md](../10-advanced-workloads/11-statefulset-basics.md) — [[StatefulSet|StatefulSet]]
- [../../故障诊断/topic-skills/16-scheduling-pdb-failure.md](../../故障诊断/技能体系/16-scheduling-pdb-failure.md) — 调度与 PDB 问题 [[SKILL|Skill]]
- [../../工作负载/](../../工作负载/) — 工作负载文档

## Related

- [[26-技能/07-安全/resource-quota/培训/learn-07-namespace-resource-quota.md|learn-07-namespace-resource-quota]] — 第七课：Namespace 与资源隔离
- [[26-技能/03-节点/node/skill-notready/skill-k8s-node-notready-SKILL.md|skill-k8s-node-notready-SKILL]] — Skill
- [[23-实体/02-K8s核心组件/statefulset.md|statefulset]] — StatefulSet
- [[deployment]] — Deployment
- [[kubernetes]] — Kubernetes (CNCF Graduated)


<!-- risk-assessed -->
