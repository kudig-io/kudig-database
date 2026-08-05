---
title: 第二课：Pod - K8s 的最小调度单元 [fundamentals]
description: 'title: 第二课：Pod - K8s 的最小调度单元'
summary: 'title: 第二课：Pod - K8s 的最小调度单元'
category: learning
tags:
- k8s
- training
- hands-on
- docker
- job
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 第二课：Pod - K8s 的最小调度单元 是什么
- 如何 第二课：Pod - K8s 的最小调度单元
- Kubernetes 11 production operations 最佳实践
trigger_keywords:
- 第二课：Pod
- K8s
- 的最小调度单元
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
title: 第二课：Pod - K8s 的最小调度单元
description: '# 第二课：Pod - K8s 的最小调度单元'
category: learning
tags:
- tutorial
- Pod
- 容器组
- k8s
- training
- lecturer
- docker
- job
aliases:
- Pod
- 容器组
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
- 第二课：Pod - K8s 的最小调度单元 是什么
- 如何 第二课：Pod - K8s 的最小调度单元
trigger_keywords:
- 第二课：Pod
- K8s
- 的最小调度单元
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
# 第二课：Pod - K8s 的最小调度单元

> **章节**: 入门引导 | **难度**: 入门 | **时长**: 20 分钟

---

## 学习目标

1. 理解 Pod 的概念和作用
2. 掌握 Pod 的创建、查看、删除命令
3. 了解 Pod 的生命周期
4. 学会排查 Pod 常见问题

---

## 1. Pod 的概念

### 1.1 开场白

```
"上节课我们说了 Kubernetes 是什么。
简单回顾一下：K8s 是一个容器编排系统，让你可以管理大量容器。
那今天我们来聊聊 K8s 里最重要的概念 —— Pod。

【核心要点】

Pod 是 Kubernetes 中的最小调度单元。
你可以理解为一个 Pod 就是'一个运行中的容器'（或者一组紧密相关的容器）。

【类比时间】

想象一下：
• Pod 就像一个快递盒子
• 容器就是盒子里的商品
• 通常一个盒子里装一种商品（一个 Pod 一个容器）
• 但有时候也会放几种相关的商品（sidecar 模式）

在 K8s 里，Pod 是调度的基础单位。
你不会直接调度容器，而是调度 Pod。"
```

### 1.2 Pod 的结构

```
【Pod 内部结构】

一个 Pod 可以包含：
• 主容器 (Main Container) - 你的应用
• 初始化容器 (Init Containers) - 启动前准备
• 边车容器 (Sidecar Containers) - 辅助功能（如日志收集）

【网络】

每个 Pod 有自己的 IP 地址。
同一个 Pod 里的容器共享同一个网络命名空间。
它们可以用 localhost 互相访问。

【存储】

Pod 可以挂载多个 Volume（存储卷）。
同一个 Pod 里的容器可以共享这些存储。
```

---

## 2. 创建 Pod

### 2.1 YAML 方式创建

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

```
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
【YAML 示例】

apiVersion: v1
kind: Pod
metadata:
  name: my-first-pod
  labels:
    app: web
spec:
  containers:
  - name: nginx
    image: nginx:1.21
    ports:
    - containerPort: 80

【创建命令】

kubectl apply -f pod.yaml

【解释】

• apiVersion: v1  → 使用 K8s 核心 API
• kind: Pod       → 创建的是 Pod 资源
• metadata.name   → Pod 的名字
• spec.containers → 容器配置
  - name: nginx   → 容器名字
  - image: nginx   → 镜像地址
  - ports         → 端口映射
```
### 2.2 命令行快速创建

```
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
【快速创建 Pod】

kubectl run my-pod --image=nginx:1.21

【注意】

命令行方式适合快速测试。
生产环境建议使用 YAML 文件，方便管理和版本控制。
```
---

## 3. 查看 Pod

### 3.1 基本查看

```
# 🟢 低风险：只读/信息收集，通常无副作用
【查看所有 Pod】

kubectl get pods

【带更多信息】

kubectl get pods -o wide

输出：
NAME         READY   STATUS    RESTARTS   AGE   IP            NODE
my-pod       1/1     Running   0          5d    10.244.0.15   node-1

解释：
• NAME - Pod 名称
• READY - 1/1 表示 1 个容器运行中，总共 1 个
• STATUS - Running 表示正常运行
• RESTARTS - 重启次数，0 表示没有重启过
• AGE - 运行了多久
• IP - Pod 的 IP 地址
• NODE - 运行在哪个节点
```
### 3.2 详细查看

```
# 🟢 低风险：只读/信息收集，通常无副作用
【查看 Pod 详情】

kubectl describe pod my-pod

这会显示：
• 基本信息（名称、命名空间、状态等）
• 容器信息（镜像、端口、资源等）
• 事件信息（调度、启动、错误等）
• 条件信息（是否就绪、是否有问题等）

【查看日志】

# 查看当前日志
kubectl logs my-pod

# 查看上一个容器的日志（如果重启过）
kubectl logs my-pod --previous

# 实时查看日志
kubectl logs -f my-pod
```
---

## 4. Pod 的生命周期

### 4.1 状态说明

```
【Pod 的状态】

1. Pending（待处理）
   Pod 已被 K8s 接受，但容器还没创建完成。
   可能原因：镜像正在下载、调度等待、资源不足

2. Running（运行中）
   容器已经创建完成，正在运行。

3. Succeeded（成功）
   容器正常退出（退出码为 0），
   通常用于 Job 或一次性任务。

4. Failed（失败）
   容器异常退出（退出码非 0），
   或者被系统终止。

5. Unknown（未知）
   不知道 Pod 的状态，
   通常是因为节点通信问题。
```

### 4.2 容器状态

```
【容器内部状态】

• Waiting - 等待中（镜像拉取、依赖等待等）
• Running - 运行中
• Terminated - 已终止

【Terminated 的原因】

• Completed - 正常完成（退出码 0）
• OOMKilled - 内存不足被杀掉（退出码 137）
• Error - 出错退出（退出码非 0）
```

---

## 5. 删除 Pod

### 5.1 基本删除

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl delete`：删除资源（可由声明式清单重建）

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
【删除 Pod】

kubectl delete pod <pod-name>

【⚠️ 高危命令：删除所有 Pod】
```bash
# ⚠️ 危险！这会删除 namespace 下所有 Pod
kubectl delete pods --all

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl delete`：删除资源（可由声明式清单重建）

```
⚠️ 风险：会导致所有 Pod 重建，服务中断。

【按标签删除】

kubectl delete pods -l app=web
```

---

## 6. 常见问题

### 6.1 Pod 一直 Pending

```
【原因】

1. 资源不足 - 没有足够的 CPU/内存
2. 节点有污点 - Pod 不允许调度到某些节点
3. 镜像拉取失败 - 镜像不存在或网络问题
4. 调度失败 - 没有匹配的节点

【排查命令】

kubectl describe pod <pod-name>

看 Events 部分，那里会显示具体原因。
```

### 6.2 Pod 一直 CrashLoopBackOff

```
【原因】

容器一直崩溃、重启、再崩溃...

常见原因：
1. 应用启动命令错误
2. 应用依赖的服务不可用
3. 配置文件错误
4. 内存不足（OOMKilled）

【排查命令】

kubectl logs <pod-name> --previous

这会显示上一个（崩溃的）容器的日志。
```
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
### 6.3 Pod 处于 ImagePullBackOff

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

```
【原因】

镜像拉取失败。

常见原因：
1. 镜像名称拼写错误
2. 私有仓库需要认证，但没有配置 secret
3. 镜像不存在
4. 网络不通

【解决方案】

1. 检查镜像名称是否正确
2. 如果是私有镜像，创建 docker-registry secret：
   kubectl create secret docker-registry my-secret \
     --docker-server=registry.example.com \
     --docker-username=user \
     --docker-password=pass
3. 在 Pod 中引用 secret：
   ```yaml
   imagePullSecrets:
   - name: my-secret
   ```
```
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
---

## 7. 总结

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源
> - `kubectl delete`：删除资源（可由声明式清单重建）

```
【要点回顾】

1. Pod 是 K8s 的最小调度单位
2. 一个 Pod 通常包含一个容器
3. Pod 有独立 IP，容器之间共享网络
4. Pod 状态：Pending → Running → Succeeded/Failed
5. 排查问题用 kubectl describe pod 和 kubectl logs

【命令速查】

创建：kubectl apply -f pod.yaml
查看：kubectl get pods
详情：kubectl describe pod <name>
日志：kubectl logs <name>
删除：kubectl delete pod <name>

【下节课预告】

下节课我们会学习 Deployment：
• Deployment 如何管理 Pod
• 如何滚动更新和回滚
• 如何扩缩容

有问题吗？"
```

---

**关联文档**:
- [../02-getting-started/](../02-getting-started/) — 快速入门
- [../../domain-10-troubleshooting-diagnostics/topic-skills/01-node-notready.md](../../domain-10-troubleshooting-diagnostics/技能体系/01-node-notready.md) — 节点问题 [[SKILL|Skill]]
- [../../domain-02-workloads-applications/](../../domain-02-workloads-applications/) — 工作负载文档

## See Also

- decision-tree-mermaid
- 01-what-is-kubernetes
- 03-deployment-basics
- 04-service-basics


## 参见

- [[32-发布/package/2026-07-02_18-40/corpus/peripheral/skills/training-lecturer/01-getting-started/01-pod-basics|讲师版]]


<!-- risk-assessed -->
