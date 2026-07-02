---
title: 第三课：Deployment - 应用部署管理器 [fundamentals]
description: 'title: 第三课：Deployment - 应用部署管理器'
summary: 'title: 第三课：Deployment - 应用部署管理器'
category: learning
tags:
- k8s
- training
- hands-on
- hpa
- ingress
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 第三课：Deployment - 应用部署管理器 是什么
- 如何 第三课：Deployment - 应用部署管理器
- Kubernetes 11 production operations 最佳实践
trigger_keywords:
- 第三课：Deployment
- 应用部署管理器
- production
- operations
- learn
prerequisites:
- kubectl-basics
- gpu-ml-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




---
title: 第三课：Deployment - 应用部署管理器
description: '# 第三课：Deployment - 应用部署管理器'
category: learning
tags:
- tutorial
- deployment
- Deployment
- 应用部署
- k8s
- training
- lecturer
- hpa
aliases:
- Deployment
- 应用部署
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
- 第三课：Deployment - 应用部署管理器 是什么
- 如何 第三课：Deployment - 应用部署管理器
trigger_keywords:
- 第三课：Deployment
- 应用部署管理器
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
# 第三课：Deployment - 应用部署管理器

> **章节**: 入门引导 | **难度**: 入门 | **时长**: 25 分钟

---

## 学习目标

1. 理解 Deployment 的作用和优势
2. 掌握 Deployment 的创建、更新、回滚命令
3. 了解滚动更新的原理
4. 学会扩缩容操作

---

## 1. Deployment 的概念

### 1.1 开场白

```
"上节课我们学了 Pod 是 K8s 的最小调度单元。
但你有没有想过一个问题：如果一个 Pod 突然挂了怎么办？

如果只有 Pod，没有其他机制，你需要：
• 手动发现 Pod 挂了
• 手动创建新的 Pod
• 手动确认新 Pod 正常运行

这太麻烦了！

【解决方案】

这就引出了我们今天的主题 —— Deployment

Deployment 是 K8s 中用来管理 Pod 的控制器。
它会：
• 保证 Pod 始终运行（Pod 挂了，自动创建新的）
• 支持滚动更新（新版本慢慢替换旧版本）
• 支持回滚（出了问题可以一键回退）

简单说：Deployment 就是你的'人力资源系统'。"
```

### 1.2 为什么需要 Deployment？

```
【没有 Deployment 的问题】

1. 手动管理 Pod
   - Pod 挂了，需要人工发现并重启
   - 工作量大，容易出错

2. 无法扩缩容
   - 想增加几个副本，需要手动创建
   - 想减少，需要手动删除

3. 更新困难
   - 更新应用版本，需要先停止所有 Pod
   - 然后一个个启动新版本
   - 中间肯定有停机时间

【有了 Deployment】

1. 自动管理
   - Deployment 自动监控 Pod 状态
   - Pod 挂了，立刻创建新的

2. 扩缩容简单
   - 一条命令，增加或减少副本数

3. 滚动更新
   - 逐步替换，零停机
   - 出问题可以一键回滚

这就是 Deployment 的价值！"
```

---

## 2. 创建 Deployment

### 2.1 YAML 方式

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

```
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
【YAML 示例】

apiVersion: apps/v1
kind: Deployment
metadata:
  name: my-deployment
spec:
  replicas: 3
  selector:
    matchLabels:
      app: web
  template:
    metadata:
      labels:
        app: web
    spec:
      containers:
      - name: nginx
        image: nginx:1.21
        ports:
        - containerPort: 80

【解释】

• replicas: 3 → 保持 3 个 Pod 运行
• selector.matchLabels → Deployment 管理哪些 Pod
• template.spec → Pod 的模板配置

【创建命令】

kubectl apply -f deployment.yaml

【验证】

kubectl get deployment
kubectl get pods -l app=web
```
### 2.2 命令行快速创建

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

```
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
【快速创建】

kubectl create deployment my-app --image=nginx:1.21

这会自动：
• 创建 Deployment
• 创建 ReplicaSet（副本管理器）
• 创建 1 个 Pod

【扩展到 3 个副本】

kubectl scale deployment my-app --replicas=3
```
---

## 3. 扩缩容

### 3.1 扩容操作

```
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
【扩容到 5 个副本】

kubectl scale deployment my-app --replicas=5

【自动扩缩容 (HPA)】

kubectl autoscale deployment my-app --cpu-percent=80 --min=2 --max=10

解释：
• cpu-percent=80 → CPU 使用超过 80% 时触发扩容
• min=2 → 最少 2 个副本
• max=10 → 最多 10 个副本
```
---

## 4. 更新与回滚

### 4.1 更新镜像

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

```
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
【更新镜像版本】

kubectl set image deployment/my-app nginx=nginx:1.22

【查看更新状态】

kubectl rollout status deployment/my-app

【查看历史版本】

kubectl rollout history deployment/my-app

输出：
REVISION  CHANGE-CAUSE
1        kubectl create deployment my-app --image=nginx:1.21
2        kubectl set image deployment my-app nginx=nginx:1.22
```
### 4.2 回滚操作

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl rollout undo/restart`：触发滚动变更，影响副本

```
# 🟢 低风险：只读/信息收集，通常无副作用
【回滚到上一个版本】

kubectl rollout undo deployment/my-app

【回滚到指定版本】

kubectl rollout undo deployment/my-app --to-revision=1

【查看当前版本】

kubectl rollout history deployment/my-app
```
---

## 5. 滚动更新原理

### 5.1 更新策略

```
【RollingUpdate 策略】

K8s 默认使用 RollingUpdate 策略：
• 逐步替换旧版本 Pod
• 始终保持一定数量的可用 Pod
• 零停机更新

【参数配置】

spec:
  strategy:
    type: RollingUpdate
    rollingUpdate:
      maxSurge: 1        # 最多超出期望多少
      maxUnavailable: 0  # 最多有多少不可用

解释：
• maxSurge: 1 → 额外最多 1 个 Pod
• maxUnavailable: 0 → 始终保持所有 Pod 可用

这意味着：更新过程中，最多有 replicas+maxSurge 个 Pod 同时存在。
```

---

## 6. 删除 Deployment

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl delete`：删除资源（可由声明式清单重建）

```
# 🟢 低风险：只读/信息收集，通常无副作用
【删除 Deployment】

kubectl delete deployment my-app

这会删除：
• Deployment 本身
• ReplicaSet（副本控制器）
• 由它创建的 Pod

【保留 Pod，只删除 Deployment】

kubectl delete deployment my-app --cascade=orphan

但注意：这样 Pod 就没有管理器了，需要手动管理。
```
---

## 7. 常见问题

### 7.1 Deployment 卡住不动

```
# 🟢 低风险：只读/信息收集，通常无副作用
【原因】

1. 镜像拉取失败
2. 健康检查失败（readinessProbe 配置问题）
3. 资源不足，新 Pod 无法调度

【排查】

kubectl describe deployment my-app
kubectl get pods

看 Events 部分和 Pod 状态。
```
### 7.2 回滚失败

```
# 🟢 低风险：只读/信息收集，通常无副作用
【原因】

通常是版本历史丢失。

【解决方案】

1. 每次更新前记录版本
2. 保存 YAML 配置，方便追溯
3. 使用 GitOps 管理配置

【建议】

"养成好习惯：每次重要更新前，先 kubectl rollout history 查看历史，
确认没有问题后再继续。"
```
---

## 8. 总结

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源
> - `kubectl delete`：删除资源（可由声明式清单重建）
> - `kubectl rollout undo/restart`：触发滚动变更，影响副本

```
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
【命令速查】

创建 Deployment：
kubectl create deployment my-app --image=nginx:1.21

扩缩容：
kubectl scale deployment my-app --replicas=5

自动扩缩容：
kubectl autoscale deployment my-app --cpu-percent=80 --min=2 --max=10

更新镜像：
kubectl set image deployment/my-app nginx=nginx:1.22

查看状态：
kubectl get deployment
kubectl rollout status deployment/my-app

回滚：
kubectl rollout undo deployment/my-app

删除：
kubectl delete deployment my-app

【核心要点】

1. Deployment 管理 Pod，保证指定数量运行
2. 滚动更新：零停机更新，自动回滚
3. 扩缩容：一条命令搞定
4. 使用 YAML 管理，方便版本控制

【下节课预告】

下节课我们会学习 Service：
• Service 是什么
• 如何让应用可以被访问
• ClusterIP、NodePort、LoadBalancer 的区别

有问题吗？"
```
---

**关联文档**:
- [../03-networking/03-service-basics.md](../03-networking/03-service-basics.md) — Service 基础
- [../../domain-10-troubleshooting-diagnostics/topic-skills/08-deployment-rollout-failure.md](../../domain-10-troubleshooting-diagnostics/topic-skills/08-deployment-rollout-failure.md) — 滚动更新问题 [[SKILL|Skill]]
- [../../domain-02-workloads-applications/](../../domain-02-workloads-applications/) — 工作负载文档

## See Also

- kubernetes.md|01-what-is-kubernetes]]
- 02-pod-basics
- 04-service-basics
- 05-ingress-basics


## 参见

- [[skills/training-public/fundamentals/03-deployment-basics.md|公开版]]


<!-- risk-assessed -->
