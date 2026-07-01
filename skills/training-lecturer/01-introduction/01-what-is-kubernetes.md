---
title: 第一课：Kubernetes 入门 [fundamentals]
description: 'description: • 如果你有 100 台服务器，容器该部署到哪台？'
category: learning
tags:
- k8s
- training
- hands-on
- docker
- mysql
- ingress
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 第一课：Kubernetes 入门 是什么
- 如何 第一课：Kubernetes 入门
- Kubernetes 11 production operations 最佳实践
trigger_keywords:
- 第一课：Kubernetes
- 入门
- production
- operations
- learn
prerequisites:
- kubectl-basics
- gpu-ml-basics
- mysql-basics
created: "2026-05-23"
---

---
title: 第一课：[[Kubernetes|Kubernetes]] 入门
description: • 如果你有 100 台服务器，容器该部署到哪台？
category: learning
tags:
- tutorial
- k8s
- training
- lecturer
- docker
- mysql
- [[Ingress|ingress]]
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
- 第一课：Kubernetes 入门 是什么
- 如何 第一课：Kubernetes 入门
trigger_keywords:
- 第一课：Kubernetes
- 入门
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
# 第一课：Kubernetes 入门

> **章节**: 入门引导 | **难度**: 入门 | **时长**: 15 分钟

---

## 学习目标

1. 理解 Kubernetes 的基本概念
2. 了解 K8s 能解决什么问题
3. 掌握 K8s 的核心优势

---

## 1. 什么是 Kubernetes？

### 1.1 开场白

```
"大家好，欢迎来到 K8s 学习课堂！我是 K8S 小博士。

今天我们要聊一个很有意思的话题 —— 什么是 Kubernetes？

你们有没有遇到过这种情况：
电脑用久了，应用越装越多，然后电脑变得很慢、很卡？
或者说，想同时跑好几个程序，但它们之间互相抢资源？

如果是这样，你可能需要了解一下容器和 Kubernetes 了。"
```

### 1.2 容器是什么？

```
"在说 K8s 之前，我们先说说容器。

【生活类比】

想象你要搬家。你需要把所有的衣服、书籍、杂物打包。
如果直接搬，可能丢失、损坏、乱成一团。
但如果你用箱子把东西分类打包，贴上标签，那就整齐多了。

容器就像这个箱子：
• 把应用和它的依赖打包在一起
• 无论在哪里运行，环境都一样
• 互不干扰，不会'这个应用在我电脑上能跑，在你电脑上就不行'

【核心技术】

Docker 是最流行的容器技术。
它可以：
• 创建容器镜像（就像给箱子拍照，记录里面有什么）
• 启动容器（就像把箱子打开，拿出东西用）
• 停止/删除容器（就像把箱子合上、扔掉）

这就是容器：一个轻量级、独立、可移植的软件打包方式。"
```

### 1.3 Kubernetes 登场

```
"好，现在我们有了容器。

但问题来了：
• 如果你有 100 台服务器，容器该部署到哪台？
• 如果一个容器挂了，怎么自动重启？
• 如果访问量突然增加，怎么自动扩容？

这就需要 Kubernetes 了！

【核心比喻】

想象你是乐队的指挥：
• 每一件乐器是一个容器
• 指挥家（K8s）协调所有乐器
• 确保大家按正确的节奏演奏
• 如果有人出错，指挥会调整

这就是 Kubernetes：
它是容器的'指挥家'，负责管理、协调、自动修复。"
```

---

## 2. Kubernetes 能解决什么问题？

### 2.1 传统部署 vs 容器 vs K8s

```
【传统部署的问题】

假设你要部署一个网站：
• 需要安装 nginx、PHP、MySQL...
• 每台服务器都要手动配置
• 如果服务器挂了，需要人工迁移
• 扩展时要买新服务器，配置很久

【容器部署的优势】

使用 Docker 容器：
• 环境一致："在我机器上能跑，生产也能跑"
• 快速部署：镜像拉下来就能跑
• 资源隔离：不同应用不会互相影响

【K8s 的最终形态】

加入 Kubernetes：
• 自动调度：容器该去哪台机器，不用你管
• 自动修复：容器挂了，K8s 自动启动新的
• 自动扩缩容：访问量增加，自动加容器
• 滚动更新：更新版本时，零停机

这就是 K8s 的魅力！"
```

### 2.2 K8s 的核心能力

| 能力 | 说明 | 类比 |
|------|------|------|
| **自动调度** | 根据资源情况自动分配容器到节点 | 派单系统 |
| **自动修复** | 容器挂了自动重启，始终保持预期数量 | 生命维持系统 |
| **水平扩缩容** | 根据负载自动增减容器数量 | 弹性橡皮筋 |
| **服务发现** | 容器可以相互找到，不需要记 IP | 自动寻人 |
| **负载均衡** | 请求均匀分布到各个容器 | 交通指挥 |
| **滚动更新** | 逐步替换容器，零停机 | 无缝换装 |
| **回滚** | 出问题可以一键回退 | 后悔药 |

---

## 3. Kubernetes 基本概念

### 3.1 核心概念一览

```
【重要概念】

1. Pod - 最小调度单位
   一个 Pod 就是一个（或一组）容器

2. Node - 工作机器
   服务器，就是一个 Node

3. Cluster - 集群
   多台 Node 组成一个集群

4. Control Plane - 控制平面
   管理整个集群的大脑

5. Deployment - 部署
   管理 Pod 的控制器，保证 Pod 始终运行

6. Service - 服务
   给 Pod 提供固定访问入口

7. Ingress - 入口
   管理外部 HTTP/HTTPS 访问

8. ConfigMap/Secret - 配置
   存储配置和敏感信息

【架构图】

       Control Plane (大脑)
              │
    ┌─────────┼─────────┐
    │         │         │
   Node1    Node2     Node3
    │         │         │
   Pod      Pod      Pod
   Pod      Pod
   (工作)   (工作)    (工作)
```

### 3.2 名词解释

```
K8s - Kubernetes 的缩写
K - K
8  - 8 个字母 (ubernetes)
s  - s

所以 Kubernetes = K8s
就像 "internationalization" = "i18n" 一样。
```

---

## 4. 为什么要学 Kubernetes？

### 4.1 应用场景

```
【典型使用场景】

1. 微服务架构
   把大应用拆成小服务，每个服务独立部署
   K8s 是微服务的最佳运行环境

2. 持续集成/持续部署 (CI/CD)
   代码提交 → 自动构建 → 自动部署到 K8s
   整个流程自动化

3. 云原生应用
   为云设计的应用天然适合在 K8s 运行
   AWS、阿里云、腾讯云都支持 K8s

4. 大规模容器编排
   成百上千个容器需要统一管理
   只有 K8s 能胜任
```

### 4.2 职业价值

```
【为什么学习 K8s】

1. 市场需求大
   几乎所有科技公司都在用 K8s
   运维、开发、SRE 都要求掌握

2. 技术趋势
   云原生是未来
   K8s 是云原生的核心

3. 薪资水平
   掌握 K8s 的工程师薪资普遍较高
   是 IT 界的硬通货

【适合人群】

• 后端开发工程师
• 运维工程师 / SRE
• 技术负责人 / 架构师
• 对云原生感兴趣的学生
```

---

## 5. 下一步

```
【课后作业】

1. 安装 kubectl 客户端
2. 连接到你的第一个 K8s 集群（可以用 minikube 本地体验）
3. 运行 kubectl get nodes 看看集群里有哪些节点

【下节课预告】

下节课我们会学习：
• Pod - K8s 的最小调度单元
• 如何创建、查看、删除 Pod
• Pod 的生命周期

有问题吗？记得多实践！"
```

---

**关联文档**:
- [../02-getting-started/](./02-getting-started/) — 快速入门
- [../../domain-01-cluster-fundamentals/01-kubernetes-architecture-overview.md](../../domain-01-cluster-fundamentals/01-kubernetes-architecture-overview.md) — 架构详解
- [../../domain-17-system-foundation/](../../domain-17-system-foundation/) — Linux 基础

## See Also

- presentation-template
- decision-tree-mermaid
- 02-pod-basics
- 03-deployment-basics


## 参见

- [[skills/training-public/fundamentals/01-what-is-kubernetes.md|公开版]]
