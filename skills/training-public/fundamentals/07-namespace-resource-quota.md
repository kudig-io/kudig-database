---
title: 第七课：Namespace 与资源隔离 [fundamentals]
description: 'description: 2. 掌握 Namespace 的创建和管理'
category: learning
tags:
- k8s
- training
- hands-on
- hpa
- ingress
- rbac
- networkpolicy
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 第七课：Namespace 与资源隔离 是什么
- 如何 第七课：Namespace 与资源隔离
- Kubernetes 11 production operations 最佳实践
trigger_keywords:
- 第七课：Namespace
- 与资源隔离
- production
- operations
- learn
prerequisites:
- kubectl-basics
- gpu-ml-basics
created: "2026-05-23"
---

---
title: 第七课：Namespace 与资源隔离
description: 2. 掌握 Namespace 的创建和管理
category: learning
tags:
- tutorial
- k8s
- training
- lecturer
- rbac
- [[NetworkPolicy|networkpolicy]]
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 初学者
- 运维工程师
- 培训师
- 技术经理
estimated_read_time: 5min
intent_queries:
- 第七课：Namespace 与资源隔离 是什么
- 如何 第七课：Namespace 与资源隔离
trigger_keywords:
- 第七课：Namespace
- 与资源隔离
- k8s
- learning
authors:
- name: KUDIG Team
  role: contributor
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'

tier: peripheral---
# 第七课：Namespace 与资源隔离

> **章节**: 入门引导 | **难度**: 入门 | **时长**: 20 分钟

---

## 学习目标

1. 理解 Namespace 的概念和作用
2. 掌握 Namespace 的创建和管理
3. 了解资源配额 (ResourceQuota) 和限制范围 (LimitRange)
4. 学会多环境管理策略

---

## 1. Namespace 概念

### 1.1 问题引入

```
【场景】

你是公司 IT 管理员，需要：
• 开发团队使用集群
• 测试团队使用集群
• 生产环境使用集群

但你只有一个 K8s 集群！

问题：如何让不同团队互不干扰？

【解决方案】

这就引出了 Namespace（命名空间）的概念！

Namespace 是 K8s 中用于隔离资源的逻辑分组。
就好像一个大楼里有很多办公室，每个办公室是一个 Namespace。
不同办公室的人看不到彼此的东西，但都共享大楼的基础设施。
```

### 1.2 类比说明

```
【大楼类比】

Namespace = 办公室
Node = 大楼
Cluster = 整栋大楼

• 每个办公室（Namespace）有独立的员工（资源）
• 办公室之间是隔离的，看不到彼此的东西
• 但都共享大楼的基础设施（网络、存储等）
• 可以设置每个办公室的使用配额（资源限制）

【K8s 类比】

Namespace = 命名空间
Node = 服务器
Cluster = K8s 集群

• 每个命名空间有独立的工作负载（Deployment、Pod 等）
• 命名空间之间是隔离的
• 系统组件运行在 kube-system 命名空间
• 可以设置每个命名空间的资源配额
```

---

## 2. 使用 Namespace

### 2.1 查看 Namespace

```
【查看所有 Namespace】

kubectl get namespaces

输出示例：
NAME              STATUS   AGE
default           Active   30d     # 默认命名空间
kube-system       Active   30d     # 系统组件
kube-public      Active   30d     # 公开资源
development       Active   15d    # 开发环境
staging           Active   15d    # 测试环境
production        Active   15d    # 生产环境

【查看特定 Namespace 的资源】

kubectl get pods -n development
kubectl get services -n staging
kubectl get all -n production
```

### 2.2 创建 Namespace

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

```
【方式一：命令行】

kubectl create namespace development

【方式二：YAML】

apiVersion: v1
kind: Namespace
metadata:
  name: development

【创建资源时指定 Namespace】

kubectl create deployment my-app --image=nginx -n development

【或在 YAML 中指定】

apiVersion: v1
kind: Pod
metadata:
  name: my-pod
  namespace: development  # 指定命名空间
spec:
  containers:
  - name: nginx
    image: nginx:1.21
```

### 2.3 设置默认 Namespace

```
【切换默认 Namespace】

kubectl config set-context --current --namespace=development

【验证】

kubectl config view --minify | grep namespace

【快速切换】

使用 kubecontext 插件或 zsh 插件可以快速切换命名空间。
```

---

## 3. 资源配额 (ResourceQuota)

### 3.1 什么是 ResourceQuota？

```
【概念】

ResourceQuota 用于限制每个 Namespace 的资源总量。
防止某个团队耗尽整个集群的资源。

【示例场景】

• 开发环境：最多使用 10 个 CPU、20GB 内存
• 测试环境：最多使用 20 个 CPU、40GB 内存
• 生产环境：最多使用 100 个 CPU、200GB 内存
```

### 3.2 创建 ResourceQuota

```
【YAML 示例】

apiVersion: v1
kind: ResourceQuota
metadata:
  name: dev-quota
spec:
  hard:
    requests.cpu: "10"      # 最多 10 个 CPU 请求
    requests.memory: 20Gi   # 最多 20GB 内存请求
    limits.cpu: "20"        # 最多 20 个 CPU 限制
    limits.memory: 40Gi     # 最多 40GB 内存限制
    pods: "50"              # 最多 50 个 Pod
    services: "20"          # 最多 20 个 Service
```

### 3.3 查看配额使用

```
【查看配额】

kubectl describe resourcequota -n development

【输出示例】

Name:       dev-quota
Namespace:   development
Resource     Used  Hard
--------     ---   ---
limits.cpu   4     20
limits.memory   8Gi    40Gi
pods         15    50
requests.cpu 2     10
requests.memory   4Gi    20Gi
```

---

## 4. 限制范围 (LimitRange)

### 4.1 什么是 LimitRange？

```
【概念】

LimitRange 为 Namespace 内的 Pod 和容器设置默认资源限制。
如果没有手动设置资源请求/限制，LimitRange 会自动注入默认值。

【作用】

• 防止 Pod 没有设置资源限制，导致资源耗尽
• 统一团队的资源使用标准
• 方便成本核算
```

### 4.2 创建 LimitRange

```
【YAML 示例】

apiVersion: v1
kind: LimitRange
metadata:
  name: dev-limits
spec:
  limits:
  - type: Container
    default:
      cpu: 500m           # 默认 CPU 限制
      memory: 256Mi        # 默认内存限制
    defaultRequest:
      cpu: 200m           # 默认 CPU 请求
      memory: 128Mi       # 默认内存请求
    max:
      cpu: "2"            # 单容器最大 CPU
      memory: 1Gi          # 单容器最大内存
    min:
      cpu: 100m            # 单容器最小 CPU
      memory: 64Mi          # 单容器最小内存
```

---

## 5. 多环境管理

### 5.1 环境分离策略

```
【推荐结构】

clusters/
├── production/
│   ├── namespace.yaml
│   ├── resource-quota.yaml
│   ├── limit-range.yaml
│   └── network-policy.yaml
├── staging/
│   ├── namespace.yaml
│   ├── resource-quota.yaml
│   └── limit-range.yaml
└── development/
    ├── namespace.yaml
    ├── resource-quota.yaml
    └── limit-range.yaml
```

### 5.2 NetworkPolicy 隔离

```
【场景】

生产环境的 Pod 不应该被开发环境的 Pod 访问。
可以通过 NetworkPolicy 实现命名空间级别的网络隔离。

【命名空间默认拒绝】

apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: default-deny-all
  namespace: production
spec:
  podSelector: {}

这会禁止所有进入 production 命名空间的流量。
然后可以根据需要添加允许规则。
```

---

## 6. 常见问题

### 6.1 资源配额超限

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl delete`：删除资源（可由声明式清单重建）
> - `kubectl edit/patch`：修改运行中的资源

```
【错误信息】

"exceeded quota" 或 "Cannot create <resource> in namespace"

【原因】

该 Namespace 的资源配额已用完。

【解决方案】

1. 查看配额使用情况
   kubectl describe resourcequota -n <namespace>

2. 增加配额
   kubectl edit resourcequota <name> -n <namespace>

3. 或者清理不需要的资源
   kubectl delete pod --field-selector=status.phase!=Running -n <namespace>
   kubectl delete deployment --field-selector=status.phase!=Running -n <namespace>
```

### 6.2 无法创建资源

```
【错误信息】

"pods quota exceeded" 或 "Unable to create due to maximum object limit"

【原因】

超过了 Namespace 的对象数量限制。

【解决方案】

清理不需要的资源，或申请增加配额。
```

---

## 7. 总结

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

```
【命令速查】

查看 Namespace：
kubectl get namespaces

创建 Namespace：
kubectl create namespace <name>

创建 ResourceQuota：
kubectl create resourcequota <name> -n <namespace> \
  --hard=requests.cpu=10,limits.cpu=20,pods=50

创建 LimitRange：
kubectl create limitrange <name> -n <namespace> \
  --default-cpu=500m --default-memory=256Mi

查看配额使用：
kubectl describe resourcequota -n <namespace>

【核心要点】

1. Namespace 用于隔离资源，类似于办公室隔间
2. 系统组件运行在 kube-system 命名空间
3. ResourceQuota 限制 Namespace 的资源总量
4. LimitRange 为容器设置默认资源限制
5. 可以结合 NetworkPolicy 实现网络隔离

【下节课预告】

下节课我们会学习故障排查：
• 常见 Pod 问题及解决方案
• 快速定位问题的方法
• 常用诊断命令

有问题吗？"
```

---

**关联文档**:
- [../09-troubleshooting/09-common-problems.md](../09-troubleshooting/09-common-problems.md) — 常见问题
- [../../domain-10-troubleshooting-diagnostics/topic-skills/09-rbac-quota-failure.md](../../domain-10-troubleshooting-diagnostics/topic-skills/09-rbac-quota-failure.md) — RBAC/配额问题
- [../../domain-10-troubleshooting-diagnostics/](../../domain-10-troubleshooting-diagnostics/) — 故障排查文档

## See Also

- 05-ingress-basics
- 06-configmap-secret
- 08-pv-pvc-basics
- 09-hpa-basics
