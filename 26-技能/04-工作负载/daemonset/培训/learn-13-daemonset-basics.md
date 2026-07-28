---
title: 第13课：DaemonSet 与节点守护
description: 2. 掌握 DaemonSet 的创建和配置方法
summary: 2. 掌握 DaemonSet 的创建和配置方法
category: skills
tags:
- k8s
- learn
- fundamentals
- calico
- docker
- ceph
- hpa
- pdb
- statefulset
- daemonset
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 15min
intent_queries:
- 第13课：DaemonSet 与节点守护 是什么
- 如何 第13课：DaemonSet 与节点守护
trigger_keywords:
- 第13课：DaemonSet
- 与节点守护
prerequisites:
- kubectl-basics
- cni-basics
- logging-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 第13课：[[daemonset|DaemonSet]] 与节点守护

> **章节**: 入门引导 | **难度**: 入门 | **时长**: 20 分钟

---

## 学习目标

1. 理解 DaemonSet 的概念和使用场景
2. 掌握 DaemonSet 的创建和配置方法
3. 了解 DaemonSet 与 Deployment 的区别
4. 学会排查 DaemonSet 问题

---

## 1. 问题引入

### 1.1 问题场景

```
【场景】

你需要在每个节点上运行一个日志收集 agent，
用来收集节点的系统日志。

问题：
• 如果有 10 个节点，你需要手动在每个节点上部署
• 如果新增一个节点，又需要手动部署
• 如果节点上的 agent 挂了，需要手动重启

Deployment 可以帮我们自动管理 Pod，但如果我们需要在"每个节点"上都运行一个 Pod 呢？

【解决方案】

DaemonSet！

DaemonSet 会确保集群中每个节点都运行一个 Pod 副本。
新增节点时，它会自动在新节点上创建 Pod。
节点删除时，对应的 Pod 也会被清理。
```

### 1.2 类比说明

```
【学校类比】

Deployment = 辅导员管理学生
• 辅导员确保有 3 个学生在上课
• 不关心具体在哪个教室

DaemonSet = 教室管理员
• 确保每个教室都有一个日光灯
• 每个教室（节点）都必须有一个
• 新建教室，自动安装日光灯
• 教室拆了，日光灯也拆掉

【K8s 类比】

DaemonSet = 确保每个节点都运行一个 Pod
• 日志收集 agent（如 Fluentd）
• 监控 exporter（如 node-exporter）
• 网络插件（如 Calico CNI）
• 存储插件（如 Ceph CSI）
```

---

## 2. DaemonSet 详解

### 2.1 基本配置

```
【YAML 示例】

apiVersion: apps/v1
kind: DaemonSet
metadata:
  name: my-daemonset
spec:
  selector:
    matchLabels:
      app: my-app
  template:
    metadata:
      labels:
        app: my-app
    spec:
      containers:
      - name: my-container
        image: my-app:1.0
        resources:
          limits:
            cpu: 100m
            memory: 128Mi
          requests:
            cpu: 50m
            memory: 64Mi

【与 Deployment 的主要区别】

1. 副本数由 DaemonSet 自动管理
   - Deployment: replicas 指定数量
   - DaemonSet: 自动在每个节点运行一个

2. 不受调度器影响
   - DaemonSet Pod 直接绑定到节点
   - 不经过调度，直接指定节点

3. 不受污点影响（默认情况）
   - 可以配置容忍所有污点
```

### 2.2 选择器与污点容忍

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
【确保 DaemonSet 在所有节点运行】

spec:
  template:
    spec:
      tolerations:
      - operator: Exists   # 容忍所有污点
        effect: NoSchedule

【只在特定节点运行】

方法一：节点选择器
spec:
  template:
    spec:
      nodeSelector:
        disktype: ssd

方法二：污点和容忍
# 给节点添加污点
kubectl taint nodes node1 disk=ssd:NoSchedule

# DaemonSet 添加容忍
spec:
  template:
    spec:
      tolerations:
      - key: "disk"
        operator: "Equal"
        value: "ssd"
        effect: "NoSchedule"

方法三：节点亲和性
spec:
  template:
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
```
---

## 3. 常见使用场景

### 3.1 日志收集

```
# 🟢 低风险：只读/信息收集，通常无副作用
【Fluentd 日志收集 DaemonSet】

apiVersion: apps/v1
kind: DaemonSet
metadata:
  name: fluentd
  namespace: logging
spec:
  selector:
    matchLabels:
      app: fluentd
  template:
    metadata:
      labels:
        app: fluentd
    spec:
      serviceAccount: fluentd
      tolerations:
      - key: node-role.kubernetes.io/master
        effect: NoSchedule
      containers:
      - name: fluentd
        image: fluent/fluentd:v1.16
        env:
        - name: FLUENTD_CONF
          value: fluent.conf
        volumeMounts:
        - name: varlog
          mountPath: /var/log
        - name: varlibdockercontainers
          mountPath: /var/lib/docker/containers
          readOnly: true
      volumes:
      - name: varlog
        hostPath:
          path: /var/log
      - name: varlibdockercontainers
        hostPath:
          path: /var/lib/docker/containers
```
### 3.2 监控 exporter

```
# 🟢 低风险：只读/信息收集，通常无副作用
【node-exporter 监控 DaemonSet】

apiVersion: apps/v1
kind: DaemonSet
metadata:
  name: node-exporter
  namespace: monitoring
spec:
  selector:
    matchLabels:
      app: node-exporter
  template:
    metadata:
      labels:
        app: node-exporter
    spec:
      hostNetwork: true
      hostPID: true
      containers:
      - name: node-exporter
        image: prom/node-exporter:v1.6.1
        args:
        - --path.procfs=/host/proc
        - --path.sysfs=/host/sys
        - --collector.filesystem.mount-points-exclude=^/(dev|proc|sys|var/lib/docker/.+)($|/)
        securityContext:
          privileged: true
        volumeMounts:
        - name: proc
          mountPath: /host/proc
        - name: sys
          mountPath: /host/sys
      volumes:
      - name: proc
        hostPath:
          path: /proc
      - name: sys
        hostPath:
          path: /sys
```
### 3.3 存储插件

```
【Local PV provisioner DaemonSet】

apiVersion: apps/v1
kind: DaemonSet
metadata:
  name: local-storage
spec:
  selector:
    matchLabels:
      app: local-storage
  template:
    metadata:
      labels:
        app: local-storage
    spec:
      containers:
      - name: local-storage
        image: local-storage:v1.0
        env:
        - name: NODE_NAME
          valueFrom:
            fieldRef:
              fieldPath: spec.nodeName
        volumeMounts:
        - name: storage
          mountPath: /storage
      volumes:
      - name: storage
        hostPath:
          path: /data/storage
```

---

## 4. DaemonSet 与 Deployment 对比

### 4.1 核心区别

```
| 特性 | Deployment | DaemonSet |
|------|-----------|-----------|
| 副本数 | 由 replicas 控制 | 每个节点一个 |
| 调度 | 调度器决定 | 直接指定节点 |
| 扩容 | 手动或 HPA | 跟随节点数 |
| 用途 | 无状态应用 | 节点守护进程 |
| 污点容忍 | 依赖调度器 | 默认容忍所有污点 |

【使用场景对比】

Deployment 适用：
• Web 应用（3 个副本随便跑在哪）
• API 服务（不需要关心节点）
• 批处理任务（Job 更合适）

DaemonSet 适用：
• 日志收集（每个节点都要有）
• 监控 agent（每个节点都要有）
• 网络插件（CNI 每个节点都要有）
• 存储插件（CSI 每个节点都要有）
```

---

## 5. 常见问题

### 5.1 DaemonSet Pod 没有在所有节点运行

```
# 🟢 低风险：只读/信息收集，通常无副作用
【排查步骤】

1. 检查 DaemonSet 状态
   kubectl get daemonset -n <namespace>
   kubectl describe daemonset <name> -n <namespace>

2. 查看 Pod 分布
   kubectl get pods -n <namespace> -l app=<label> -o wide

3. 检查节点选择器/污点容忍
   kubectl describe daemonset <name> | grep -A10 "Node Terms"

4. 检查节点是否有问题
   kubectl get nodes
   kubectl describe node <node-name> | grep -A5 "Taints"

5. 查看 Pod 详情
   kubectl describe pod <pod-name> -n <namespace>

   看 Events 部分是否有调度失败的记录。
```
### 5.2 DaemonSet 更新

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl rollout undo/restart`：触发滚动变更，影响副本

```
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
【滚动更新策略】

spec:
  updateStrategy:
    type: RollingUpdate
    rollingUpdate:
      maxUnavailable: 1   # 最多 1 个不可用

【手动更新镜像】

kubectl set image daemonset/<name> <container-name>=<new-image>

【查看更新状态】

kubectl rollout status daemonset/<name>

【回滚】

kubectl rollout undo daemonset/<name>
```
---

## 6. 数字人 Q&A 场景

### 6.1 用户问：DaemonSet 和 Deployment 有什么区别？

```
【回复】

"好问题！让我来解释一下：

【核心区别】

Deployment = 管理 Pod 的数量
• 你说需要 3 个副本，Deployment 就在集群里找 3 个节点跑 Pod
• 不关心具体在哪，只要够数就行

DaemonSet = 管理 Pod 的分布
• 确保每个节点都有一个 Pod
• 新增节点，自动在新节点上创建 Pod
• 节点删除，对应的 Pod 也删除

【生活类比】

Deployment = 招聘 3 个服务员
• 不关心在哪个餐厅，只要够 3 个就行

DaemonSet = 每个教室一个日光灯
• 每个教室必须有灯
• 新建教室，自动装灯
• 教室拆了，灯也拆掉

【使用场景】

用 Deployment：
• Web 应用、API 服务
• 不关心运行在哪个节点

用 DaemonSet：
• 日志收集（fluentd）
• 监控 agent（node-exporter）
• 网络插件（CNI）
• 存储插件（CSI）

有其他问题吗？"
```

### 6.2 用户问：DaemonSet 不在某个节点上运行怎么办？

```
# 🟢 低风险：只读/信息收集，通常无副作用
【回复】

"DaemonSet 在某个节点没运行，按以下步骤排查：

第一步：检查 DaemonSet 状态
kubectl get daemonset -n <namespace>
kubectl describe daemonset <name> -n <namespace>

看 Desired Number Scheduled 和 Current Number Scheduled 对比。

第二步：检查节点污点
kubectl describe node <node-name> | grep Taints

如果节点有污点，DaemonSet 可能没有配置对应的容忍。

第三步：检查节点选择器
kubectl describe daemonset <name> | grep Node Selector

如果 DaemonSet 有 nodeSelector，节点必须匹配才能运行。

第四步：查看 Pod 详情
kubectl get pods -n <namespace> -l app=<label> -o wide
kubectl describe pod <pod-name> -n <namespace>

看 Events 部分，找调度失败的原因。

【解决方案】

如果有污点，添加容忍：
spec:
  template:
    spec:
      tolerations:
      - key: "node.kubernetes.io/not-ready"
        operator: "Exists"
        effect: "NoSchedule"

有其他问题吗？"
```
---

## 7. 总结

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源
> - `kubectl delete`：删除资源（可由声明式清单重建）
> - `kubectl rollout undo/restart`：触发滚动变更，影响副本

```
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
【命令速查】

创建 DaemonSet：
kubectl apply -f daemonset.yaml

查看 DaemonSet：
kubectl get daemonset -n <namespace>
kubectl describe daemonset <name> -n <namespace>

更新镜像：
kubectl set image daemonset/<name> <container>=<image>

滚动更新：
kubectl rollout status daemonset/<name>

回滚：
kubectl rollout undo daemonset/<name>

删除 DaemonSet：
kubectl delete daemonset <name> -n <namespace>

【核心要点】

1. DaemonSet 确保每个节点都运行一个 Pod
2. 新增节点会自动在新节点上创建 Pod
3. 常用于日志收集、监控、网络插件等
4. 可以通过 nodeSelector、污点容忍控制部署节点

【下节课预告】

下节课我们会学习 StatefulSet：
• 什么是有状态应用
• StatefulSet 与 Deployment 的区别
• 如何管理有状态应用（如数据库）

有问题吗？"
```
---

**关联文档**:
- [../11-scheduling/11-scheduling-basics.md](../11-scheduling/11-scheduling-basics.md) — 调度与亲和性
- [../../故障诊断/topic-skills/17-daemonset-pdb-failure.md](../../故障诊断/技能体系/17-daemonset-pdb-failure.md) — DaemonSet 问题 [[SKILL|Skill]]
- [../../工作负载/](../../工作负载/) — 工作负载文档

## Related

- [[docker]] — Docker
- [[23-实体/02-K8s核心组件/statefulset.md|statefulset]] — StatefulSet
- [[deployment]] — Deployment
- [[cni]] — CNI (Container Network Interface)
- [[kubernetes]] — Kubernetes (CNCF Graduated)


<!-- risk-assessed -->
