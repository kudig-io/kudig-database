---
title: K8s 基础知识考核
description: A. 调度器无法找到合适的节点
summary: A. 调度器无法找到合适的节点
category: skills
tags:
- k8s
- skills
- sop
- runbook
- etcd
- apiserver
- kubelet
- scheduler
- docker
- mysql
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 技术支持
estimated_read_time: 5min
intent_queries:
- K8s 基础知识考核 是什么
- 如何 K8s 基础知识考核
trigger_keywords:
- K8s
- 基础知识考核
- skills
prerequisites:
- kubectl-basics
- troubleshooting-methodology
- etcd-basics
- mysql-basics
- gpu-scheduling-basics
skill_id: SKILL-K8S_FUNDAMENTALS_QUIZ-001
skill_name: K8s 基础知识考核
version: 1.0.0
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# K8s 基础知识考核

> **适用对象**: SRE/Ops 工程师能力考核 | **版本**: K8s 1.28-1.33 | **时间**: 60 分钟

---

## 一、选择题（每题 2 分，共 40 分）

### 1. 以下哪个不是 [[entities/kubernetes.md|[[Kubernetes|kubernetes]]]] 的核心组件？

A. kube-apiserver
B. [[etcd|etcd]]
C. [[kubelet|kubelet]]
D. docker
E. kube-scheduler

### 2. Pod 处于 `Pending` 状态的可能原因不包括：

A. 调度器无法找到合适的节点
B. 镜像拉取失败
C. 资源不足（CPU/内存）
D. Pod 已经 Running
E. 节点有污点但 Pod 没有容忍

### 3. 创建一个 Deployment 后，Pod 调度到节点之前需要经历哪些阶段？

A. Pending → ContainerCreating → Running
B. Pending → Running
C. Pending → Initializing → Running
D. ContainerCreating → Running
E. Pending → Scheduled → Running

### 4. Service `type: ClusterIP` 的含义是：

A. 通过节点端口暴露服务
B. 通过负载均衡器暴露服务
C. 仅集群内部可访问的虚拟 IP
D. 通过外部名称暴露服务
E. 无头服务（无 ClusterIP）

### 5. 以下哪个命令可以查看 Pod 的资源使用？

A. `kubectl get pods`
B. `kubectl describe pod`
C. `kubectl top pods`
D. `kubectl logs`
E. `kubectl exec`

### 6. 在 K8s 中，以下哪种方式可以保证 Pod 最多只有 1 个副本在维护期间不可用？

A. HPA
B. PDB（PodDisruptionBudget）
C. ReplicaSet
D. Deployment
E. Service

### 7. 当容器探针配置错误导致应用无法启动时，应如何修复？

A. 删除 Pod 让 Deployment 重建
B. 修改 Pod 的探针配置并重新 apply
C. 重启 kubelet
D. 修改 Deployment 的探针配置
E. 创建新的 Deployment

### 8. 如果 Pod 需要调度到 GPU 节点，应该配置什么？

A. nodeSelector
B. 环境变量
C. ConfigMap
D. Secret
E. Volume

### 9. 使用 `kubeadm init` 初始化集群后，集群证书在哪里？

A. `/etc/kubernetes/pki`
B. `/var/lib/etcd`
C. `/var/lib/kubelet`
D. `/etc/kubernetes/manifests`
E. `/root/.kube`

### 10. 以下哪个命令可以查看 API Server 的健康状态？

A. `kubectl get nodes`
B. `curl -sk https://localhost:6443/healthz`
C. `kubectl get pods -n kube-system`
D. `kubectl cluster-info`
E. Both B and D

### 11. etcd 的默认端口是：

A. 443
B. 6443
C. 2379
D. 10250
E. 8472

### 12. 使用 `kubectl exec` 进入容器后，如何查看容器内进程？

A. `ps aux`
B. `top`
C. `ctr tasks`
D. Both A and B
E. Both A and C

### 13. 在 K8s 中，RBAC 的 Role 和 ClusterRole 区别是：

A. Role 作用于 namespace，ClusterRole 作用于集群
B. Role 用于 Pod，ClusterRole 用于 Node
C. 两者没有区别
D. ClusterRole 只能绑定到 ServiceAccount
E. Role 只能绑定到 User

### 14. 使用 `kubectl scale` 命令扩缩容 Deployment 后，新 Pod 的调度依据是什么？

A. 随机选择节点
B. kube-scheduler 根据资源情况调度
C. 总是调度到同一节点
D. 手动指定节点
E. 使用默认调度器

### 15. PVC 的 `accessModes` 中 `ReadWriteOnce` 表示：

A. 只能单节点读写
B. 多节点只读
C. 单节点只读
D. 多节点读写
E. 只允许一个 Pod 读

### 16. 在 K8s 中，什么是 Init Container？

A. 第一个启动的容器
B. 在主容器启动前执行的容器
C. 初始化失败的容器
D. 系统自动创建的容器
E. 用于日志收集的容器

### 17. 如果要查看 Pod 的详细事件，应该使用哪个命令？

A. `kubectl get pods`
B. `kubectl logs`
C. `kubectl describe pod`
D. `kubectl get events`
E. Both C and D

### 18. kube-proxy 在 iptables 模式下，负责什么功能？

A. 容器运行时管理
B. Pod 网络通信
C. Service 到 Endpoints 的代理
D. 节点间网络隧道
E. 镜像拉取

### 19. 当节点的内存压力达到阈值时，kubelet 会：

A. 自动重启节点
B. 驱逐低优先级 Pod
C. 停止 kubelet
D. 通知用户
E. 忽略压力继续运行

### 20. 在使用 `kubectl apply` 时，如果想先预览 YAML 实际效果而不真正执行，应该使用什么参数？

A. `--dry-run`
B. `--preview`
C. `--validate`
D. `--check`
E. `--diff`

---

## 二、简答题（每题 10 分，共 40 分）

### 1. 请描述 Pod 从创建到 Running 的完整生命周期，包括各个阶段和触发条件。

### 2. 在生产环境中，发现多个 Pod 出现 `CrashLoopBackOff`，请列出你的排查步骤和可能原因。

### 3. 假设你需要在集群中部署一个有状态 MySQL 数据库，请说明你会使用哪些 K8s 资源（Deployment/StatefulSet/Service/PVC 等），以及它们各自的作用。

### 4. 解释什么是 `Ingress`，以及 Ingress Controller 的作用。如果配置 Ingress 后访问返回 404，请列出至少 3 个可能的原因。

---

## 三、实操题（20 分）

### 场景

你接到一个工单：生产环境的 `production` 命名空间下的 `web-backend` Deployment 的所有 Pod 处于 `Running` 状态，但外部无法访问，返回 502 错误。

### 要求

请写出你的排查步骤（伪代码/命令序列），包括：

1. 如何确认问题（2 分）
2. 如何收集信息（4 分）
3. 如何定位根因（6 分）
4. 如何修复并验证（8 分）

---

## 答案

答案见 `answer-keys/k8s-fundamentals-quiz-answers.md`

---

```yaml
---
id: ASSESSMENT-K8S-001
topic: assessment
type: quiz
tags: [assessment, quiz, k8s-fundamentals, sre, ops-engineer, k8s-1.28-1.33]
intent_queries:
  - "K8s 基础知识考核"
  - "选择题题库"
  - "简答题题库"
difficulty: intermediate
target_roles: [sre, ops-engineer]
related:
  - 故障诊断/topic-skills/assessment/troubleshooting-lab-exam.md
  - 故障诊断/topic-skills/assessment/daily-check-quiz.md
---
```

<!-- risk-assessed -->
