---
title: 每日一题
description: '- 强化故障排查思维'
category: skills
tags:
- k8s
- skills
- sop
- runbook
- etcd
- apiserver
- kubelet
- rag
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 技术支持
estimated_read_time: 5min
intent_queries:
- 每日一题 是什么
- 如何 每日一题
trigger_keywords:
- 每日一题
- skills
prerequisites:
- kubectl-basics
- troubleshooting-methodology
- etcd-basics
skill_id: SKILL-DAILY_CHECK_QUIZ-001
skill_name: 每日一题
version: 1.0.0
created: "2026-05-23"
---

# 每日一题

> **适用对象**: SRE/Ops 工程师日常学习 | **版本**: K8s 1.28-1.33
> **更新频率**: 每个工作日更新一道题 | **答案见次日更新**

---

## 学习目标

- 巩固 K8s 核心概念
- 强化故障排查思维
- 提高 oncall 响应速度

---

## 第 1 周题目

### Day 1（周一）

**题目**：一个 Pod 处于 `CrashLoopBackOff` 状态，已知容器退出码是 137。以下哪个是最可能的原因？

A. 容器配置错误
B. 内存不足导致 OOM Kill
C. 镜像拉取失败
D. 网络不通
E. 磁盘空间不足

**涉及知识点**：
- Pod 生命周期
- OOM Kill
- 退出码含义

---

### Day 2（周二）

**题目**：你执行 `kubectl get [[Pods|pods]]` 发现某个 Pod 处于 `Pending` 状态超过 5 分钟。执行 `kubectl describe pod` 看到事件 "0/3 nodes are available: 1 Insufficient cpu, 2 node(s) had taint". 以下哪个操作最适合作为第一步？

A. 删除 Pod 重新创建
B. 增加节点资源
C. 检查节点污点和 Pod 容忍配置
D. 重启集群
E. 升级集群版本

**涉及知识点**：
- Pod 调度
- 污点和容忍
- 资源不足

---

### Day 3（周三）

**题目**：一个 Deployment 有 3 个副本，所有 Pod 都处于 Running 状态，但 [[Service|Service]] 的 Endpoints 为空。以下哪个不是可能的原因？

A. Service 的 selector 与 Pod 的标签不匹配
B. 所有 Pod 的 readinessProbe 都失败
C. Pod 处于 Terminating 状态
D. kube-proxy 没有正常运行
E. Pod 使用了 hostNetwork 模式

**涉及知识点**：
- Service Endpoints
- ReadinessProbe
- kube-proxy

---

### Day 4（周四）

**题目**：你执行 `kubectl get nodes` 看到所有节点状态都是 Ready，但执行 `kubectl get pods -A | grep -v Running` 发现大量 Pod 处于 Evicted 状态。Evicted 的原因最可能是？

A. 调度器故障
B. API Server 故障
C. 节点资源压力导致 kubelet 驱逐 Pod
D. 网络分区
E. etcd 故障

**涉及知识点**：
- Kubelet Eviction
- 资源压力
- QoS 优先级

---

### Day 5（周五）

**题目**：你尝试执行 `kubectl logs -f <pod-name>` 但收到错误 "error: persistent volume claim not found"。以下哪个是最可能的原因？

A. PVC 被删除
B. Pod 名称错误
C. 命名空间错误
D. 权限不足
E. API Server 未运行

**涉及知识点**：
- PVC 生命周期
- Volume Mount
- Pod 状态

---

## 第 2 周题目

### Day 6（周一）

**题目**：你在控制平面节点执行 `openssl x509 -in /[[entities/kubernetes|kubernetes]]/pki/apiserver.crt -noout -dates` 发现证书已过期。接下来应该执行什么命令续期？

A. `kubeadm certs renew all`
B. `kubeadm init --skip-certificates`
C. `openssl x509 -req -in apiserver.csr`
D. `etcdctl snapshot save`
E. `kubectl delete pod kube-apiserver`

**涉及知识点**：
- kubeadm 证书管理
- 证书续期
- 控制平面维护

---

### Day 7（周二）

**题目**：一个使用 `hostPath` 类型 PV 的 Pod 调度到节点后无法启动，日志显示 "Unable to mount volumes"。以下哪个不是可能的原因？

A. hostPath 路径不存在
B. 节点上没有对应目录
C. Pod 的 storageClassName 与 PV 不匹配
D. 节点权限问题（无法访问 hostPath）
E. PV 已绑定到其他 PVC

**涉及知识点**：
- hostPath
- Volume Mount
- PV/PVC 绑定

---

## 题目汇总表

| 日期 | 题目类型 | 知识点 | 难度 |
|------|---------|--------|------|
| Day 1 | 选择 | Pod 状态/OOM | 中 |
| Day 2 | 选择 | 调度/污点 | 中 |
| Day 3 | 选择 | Service Endpoints | 中 |
| Day 4 | 选择 | Kubelet Eviction | 中 |
| Day 5 | 选择 | PVC 生命周期 | 中 |
| Day 6 | 选择 | 证书管理 | 中 |
| Day 7 | 选择 | hostPath | 中 |

---

## 答案

答案见 `answer-keys/daily-check-quiz-answers.md`（每周日更新）

---

```yaml
---
id: ASSESSMENT-DAILY-001
topic: assessment
type: daily-quiz
tags: [assessment, daily-quiz, k8s, sre, ops-engineer, k8s-1.28-1.33]
intent_queries:
  - "每日一题"
  - "K8s 练习题"
  - "故障排查练习"
difficulty: intermediate
target_roles: [sre, ops-engineer]
related:
  - domain-10-troubleshooting-diagnostics/topic-skills/assessment/k8s-fundamentals-quiz.md
  - domain-10-troubleshooting-diagnostics/topic-skills/assessment/troubleshooting-lab-exam.md
---
```