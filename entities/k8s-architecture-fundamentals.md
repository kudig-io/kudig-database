---
title: K8s 架构基础与核心组件原理
description: '# K8s 架构基础与核心组件原理'
summary: '# K8s 架构基础与核心组件原理'
category: reference
tags:
- k8s
- architecture
- core-components
- apiserver
- etcd
- scheduler
- controller-manager
- kubelet
- docker
- gateway
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- K8s 架构基础与核心组件原理 是什么
- 如何 K8s 架构基础与核心组件原理
trigger_keywords:
- K8s
- 架构基础与核心组件原理
prerequisites:
- kubectl-basics
- etcd-basics
---



# K8s 架构基础与核心组件原理

## 项目全景

KUDIG-DATABASE（Kubernetes Universal Database & Intelligence Gateway）是一个开源的云原生技术全域知识库，覆盖 950+ 篇文档、41 个知识领域、4300 万+ 字符。以 **Domain（知识域）× Topic（专题）** 二维矩阵组织，具有明确依赖关系和学习路径。

三大差异化定位：
- **生产级**：所有 YAML/Shell 示例经过万级节点生产环境验证
- **AI-Ready**：文档结构天然适配 NotebookLM、RAG 和 Agent 训练场景
- **方法论独创**：内置 FTA 故障树分析、FEBM 取证循证、Skill 诊断-修复闭环

## 分层架构模型

Kubernetes 将系统职责拆解为 7 个正交层次：

| 层次 | 名称 | 职责 | 关键组件 |
|------|------|------|----------|
| Layer 1 | 编排层 | 调度、编排、自动化 | Scheduler, Controllers |
| Layer 2 | API 层 | 统一入口、认证授权、准入控制 | API Server, Admission Controllers |
| Layer 3 | 数据层 | 持久化存储 | etcd |
| Layer 4 | 运行时层 | 容器运行环境 | kubelet, Container Runtime |
| Layer 5 | 网络层 | Pod 网络、Service 负载均衡 | CNI, kube-proxy |
| Layer 6 | 存储层 | 持久化卷管理 | CSI, Volume Plugin |
| Layer 7 | 扩展层 | 自定义功能扩展 | CRD, Operator, Webhook |

**核心推论**：任何请求都必须从 Layer 2（API Server）进入。Scheduler 不直接与 kubelet 通信，Controller Manager 也不直接读写 etcd。这种星型拓扑实现真正松耦合。

## 控制平面核心组件

### API Server
- 唯一的状态入口，所有组件通过 API Server 交互
- 默认端口：:6443（HTTPS）
- 职责：认证、授权、准入控制、API 对象 CRUD

### etcd
- 唯一的持久化后端，默认端口 :2379/:2380
- 基于 Raft 共识协议的分布式 KV 存储
- 生产环境推荐 3 或 5 节点集群

### Scheduler（kube-scheduler）
- 默认端口：:10259
- 两阶段调度：过滤（Filtering）→ 打分（Scoring）
- 支持自定义调度器和调度框架扩展

### Controller Manager（kube-controller-manager）
- 默认端口：:10257
- 核心控制循环：观察实际状态 → 比较期望状态 → 执行调谐动作
- 内置控制器：Deployment、ReplicaSet、Node、Service Account 等

## 控制器模式（Controller Pattern）

Kubernetes 的核心设计模式是**声明式 API + 控制器调谐**：
1. 用户通过 YAML 声明期望状态
2. API Server 将声明持久化到 etcd
3. 控制器 Watch 变更，执行调谐使实际状态匹配期望状态
4. 如果实际状态偏离期望，控制器自动修复

这一模式贯穿 Kubernetes 所有组件，是理解整个系统的基础。

## 学习路径建议

1. Linux 基础 → Docker 容器 → K8s 架构概览
2. 核心组件原理 → API 对象 → 控制器模式
3. 网络模型 → 存储模型 → 安全模型
4. 高级主题：调度、扩缩容、多集群、服务网格

---

> 来源：.zread/wiki/drafts/1-xiang-mu-zong-lan-kudig-database-quan-yu-zhi-shi-ku.md, .zread/wiki/drafts/5-jia-gou-ji-chu-yu-he-xin-zu-jian-yuan-li.md

## Related

- [[entities/kubelet.md|kubelet]] — kubelet
- [[cni]] — CNI (Container Network Interface)
- [[etcd]] — etcd
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[concepts/controller-pattern.md|controller-pattern]] — Controller Pattern (Reconciliation Loop)
