---
title: ChaosBlade [entities]
description: '## 概述'
category: entities
tags:
- k8s
- cncf
- chaos
- chaosblade
- containerd
- docker
- mysql
- crd
- operator
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- ChaosBlade 是什么
- 如何 ChaosBlade
trigger_keywords:
- ChaosBlade
prerequisites:
- kubectl-basics
- mysql-basics
created: "2026-05-23"
---

# ChaosBlade

> **CNCF 状态**: Sandbox | **类别**: Chaos | **主要语言**: Go, Java

## 概述

ChaosBlade 是阿里巴巴开源的混沌工程实验工具，用于模拟各种问题场景以测试系统的韧性。它支持对主机、容器、Kubernetes 和各种中间件 (Dubbo、RocketMQ、MySQL) 进行故障注入。ChaosBlade 提供统一的 CLI 和 Kubernetes Operator 两种使用方式。

## 核心能力

- **多平台支持**: 主机、Docker、Kubernetes 环境
- **丰富场景**: CPU、内存、网络、磁盘、进程问题
- **中间件问题**: Java 应用、Dubbo、RocketMQ、MySQL 等
- **Kubernetes 原生**: Operator 模式，CRD 声明式实验
- **安全机制**: 实验自动恢复和销毁
- **统一 CLI**: 一致的命令行接口

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[concepts/kubernetes-architecture-overview|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- **渐进式注入**: 从小规模开始，逐步扩大实验范围
- **超时设置**: 始终设置 timeout 参数防止永久问题
- **非生产先行**: 先在测试环境验证实验效果
- **监控配合**: 结合监控系统观察系统响应
- **团队协作**: 通知相关人员后再执行实验
- **回滚计划**: 确保可以快速销毁实验

## 架构定位

在 CNCF 生态中，chaosblade 属于 **Chaos** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[deployment]]
- [[entities/crd-custom-resources|crd-custom-resources]]
- [[operator-pattern]]
- [[pod-lifecycle]]

## Related

- [[kcp]] — kcp
- [[entities/cncf-security|cncf-security]] — CNCF 安全与合规项目全景
- [[07-containerd-disaster-recovery]] — containerd 灾难恢复
- [[docker]] — Docker
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- chaosblade
- [[entities/krkn|Krkn]]
- [[entities/cncf-infrastructure|CNCF 基础设施与混沌工程项目全景]] — Cross-reference
- [[domain-19-landscape-references/topic-index/etcd-index|etcd 知识图谱索引]]
- [[domain-19-landscape-references/topic-index/gitops-cicd-index|GitOps / CI-CD 全局索引]]
