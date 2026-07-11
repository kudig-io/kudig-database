---
title: OpenKruise [entities]
description: '## 概述'
summary: 'OpenKruise 是 Kubernetes 的增强工作负载套件，提供高级部署、原地升级、Sidecar 管理等能力。'
category: entities
tags:
- k8s
- cncf
- orchestration
- openkruise
- statefulset
- daemonset
- job
- crd
- operator
tier: peripheral
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- OpenKruise 是什么
- 如何 OpenKruise
trigger_keywords:
- OpenKruise
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# OpenKruise

> **CNCF 状态**: Incubating | **类别**: Orchestration | **主要语言**: Go

## 概述

OpenKruise 是 Kubernetes 的增强工作负载套件，由阿里巴巴开源，2020 年加入 CNCF 沙箱，2022 年晋升孵化项目。它提供高级部署、原地升级（In-place Update）、Sidecar 热管理、容器重启等能力，扩展了 Kubernetes 原生工作负载，解决大规模应用管理的痛点。OpenKruise 在阿里巴巴内部经过多年双十一验证，管理数十万 Pod。与原生 Deployment/StatefulSet 相比，OpenKruise 的 CloneSet 提供了原地镜像升级（只更新容器镜像，不重建 Pod）、分批灰度发布、maxUnavailable 等高级特性。Advanced StatefulSet 支持原地升级和指定 Pod 删除，SidecarSet 实现了声明式 Sidecar 注入和独立升级。

## 核心能力

- **CloneSet**: 无状态工作负载，支持原地升级、分批灰度、maxUnavailable 控制
- **Advanced StatefulSet**: 增强版 StatefulSet，支持原地升级和指定 Pod 删除
- **Advanced DaemonSet**: 增强版 DaemonSet，支持分批发布和灰度
- **SidecarSet**: 声明式 Sidecar 注入和独立升级，无需重建 Pod
- **ImagePullJob**: 集中式镜像预热，加速大规模发布
- **PodUnavailableBudget**: 防止误操作驱逐关键 Pod

## 架构

OpenKruise 基于标准 Kubernetes Controller 模式：

- **Kruise Manager**: 核心 Controller Manager，管理所有增强工作负载 CRD
- **CloneSet Controller**: 调谐 CloneSet，执行原地升级和分批发布
- **SidecarSet Controller**: 管理 Sidecar 定义，通过 Webhook 注入到 Pod
- **ImagePullJob Controller**: 管理镜像预拉取任务
- **PodUnavailableBudget Controller**: 防止关键 Pod 被驱逐
- **Kruise-daemon (可选)**: 节点级守护进程，支持容器级别的原地升级操作

原地升级流程：`CloneSet (镜像变更) → Controller → Pod (更新容器镜像) → kubelet (重启容器，不重建 Pod)`

## K8s 集成

OpenKruise 以标准 Kubernetes CRD + Controller 方式部署。Kruise Manager 作为 Deployment 运行在集群中，监听 CloneSet、Advanced StatefulSet 等 CRD。原地升级通过修改 Pod 的 `image` 字段实现——OpenKruise 利用 Kubernetes API 的 Pod update 能力，在不重建 Pod 的情况下更新容器镜像，保留 Pod 的 IP、Volume、网络配置。SidecarSet 通过 Mutating Webhook 在 Pod 创建时自动注入 Sidecar 容器。与 [[概念/kubernetes-architecture-overview.md|Kubernetes 架构]] 中的标准工作负载 API 和调度器完全兼容。

## 生产场景

1. **大规模应用原地升级**: 数千 Pod 的应用版本更新，原地升级节省调度和 IP 分配时间
2. **Sidecar 统一管理**: 通过 SidecarSet 统一管理所有 Pod 的日志 Agent、监控 Agent 版本
3. **镜像预热加速发布**: 大规模发布前使用 ImagePullJob 预拉取镜像到所有节点
4. **有状态应用灰度发布**: Advanced StatefulSet 支持按序号灰度升级指定 Pod

## 安装

```bash
# Helm 安装 OpenKruise
helm repo add kruise https://openkruise.github.io/charts/
helm install kruise kruise/kruise -n kruise-system --create-namespace

# 创建 CloneSet（原地升级）
kubectl apply -f - <<EOF
apiVersion: apps.kruise.io/v1alpha1
kind: CloneSet
metadata:
  name: my-app
spec:
  replicas: 3
  updateStrategy:
    type: InPlaceIfPossible
    maxUnavailable: 1
    partition: 0
  selector:
    matchLabels:
      app: my-app
  template:
    metadata:
      labels:
        app: my-app
    spec:
      containers:
      - name: main
        image: my-app:v1
EOF

# 创建 SidecarSet（自动注入 Sidecar）
kubectl apply -f - <<EOF
apiVersion: apps.kruise.io/v1alpha1
kind: SidecarSet
metadata:
  name: log-agent
spec:
  selector:
    matchLabels:
      app: my-app
  containers:
  - name: log-agent
    image: fluent-bit:latest
    volumeMounts:
    - mountPath: /var/log
      name: host-log
  volumes:
  - name: host-log
    hostPath:
      path: /var/log
EOF
```

## 对比

| 特性 | OpenKruise | 原生 K8s | Argo Rollouts | Flagger |
|------|-----------|----------|---------------|---------|
| 原地升级 | ✅ | ❌ | ❌ | ❌ |
| Sidecar 管理 | ✅ SidecarSet | ❌ | ❌ | ❌ |
| 镜像预热 | ✅ | ❌ | ❌ | ❌ |
| CNCF 状态 | Incubating | Graduated | 非 CNCF | 非 CNCF |

## 架构定位

在 CNCF 生态中，OpenKruise 属于 **Orchestration** 类别，为云原生应用提供增强工作负载管理能力。

## 参考链接

- [[deployment]]
- [[概念/controller-pattern.md|controller-pattern]]
- [[pod-lifecycle]]

## Related

- [[krkn]] — Krkn
- [[opengitops]] — OpenGitOps
- [[cadence]] — Cadence
- [[实体/statefulset.md|statefulset]] — StatefulSet
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- [[故障诊断/FTA故障树/list/openkruise-fta.md|OpenKruise 工作负载异常故障树分析]]
- openkruise
- [[实体/cncf-cicd.md|CNCF CI/CD 与发布管理项目全景]] — Cross-reference
- [[生态参考/领域索引/openkruise-index.md|OpenKruise 全局索引]]
- [[生态参考/领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
