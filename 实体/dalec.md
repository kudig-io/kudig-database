---
title: Dalec
description: '## 概述'
summary: 'Dalec 是一个声明式的 Linux 系统包构建工具，通过简洁的 YAML 规范定义如何构建 RPM、DEB 等 Linux 包，而无需手动编写 spec 文件或 debian/rules。它基于 BuildKit 构建，能够交叉编译多架构包，支持自动依赖管理、补丁应用、签名等功能。Dalec 特别适合需要将软件打包为多个发行版格式的场景。'
category: entities
tags:
- k8s
- cncf
- ci-cd
- dalec
- crd
- operator
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Dalec 是什么
- 如何 Dalec
trigger_keywords:
- Dalec
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Dalec

> **CNCF 状态**: Sandbox | **类别**: CI/CD | **主要语言**: Go

## 概述

Dalec 是一个声明式的 Linux 系统包构建工具，通过简洁的 YAML 规范定义如何构建 RPM、DEB 等 Linux 包，而无需手动编写 spec 文件或 debian/rules。它基于 BuildKit 构建，能够交叉编译多架构包，支持自动依赖管理、补丁应用、签名等功能。Dalec 特别适合需要将软件打包为多个发行版格式的场景。

## 核心能力

- 详见源文档获取完整信息 ^[inferred]

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[概念/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- **源码固定**: 使用 Git commit hash 而非分支名固定源码版本
- **依赖最小化**: 只声明实际需要的运行时依赖
- **测试覆盖**: 为每个包编写基本测试确保安装和运行正常
- **CI/CD 集成**: 将 Dalec 构建集成到流水线，自动构建多发行版包
- **版本策略**: 使用 revision 字段区分同一上游版本的不同打包版本

## 架构定位

在 CNCF 生态中，dalec 属于 **CI/CD** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[概念/declarative-api.md|declarative-api]]
- [[概念/ci-cd-pipeline-patterns.md|ci-cd-pipeline-patterns]]

## Related

- [[pipecd]] — PipeCD
- [[hami]] — HAMI
- [[open-policy-containers]] — [[实体/open-policy-containers.md|Open Policy Containers (OPCR)]]
- [[werf]] — werf
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- dalec
- [[实体/cncf-runtime.md|CNCF 容器运行时与工具链项目全景]] — Cross-reference
- [[生态参考/领域索引/etcd-index.md|etcd 知识图谱索引]]
- [[生态参考/领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
