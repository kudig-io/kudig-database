---
title: Koordinator (entities)
description: '## 概述'
summary: 'Koordinator 是一个基于 QoS 的 Kubernetes 混合调度系统，专为提高集群资源利用率而设计。它通过精细化的资源管理和混部（co-location）技术，在保证延迟敏感型（LS）工作负载 SLO 的同时，充分利用空闲资源运行尽力而为型（BE）任务，实现 60%+ 的集群利用率。'
category: entities
tags:
- k8s
- cncf
- orchestration
- koordinator
- scheduler
- crd
- operator
- gpu
tier: peripheral
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Koordinator 是什么
- 如何 Koordinator
trigger_keywords:
- Koordinator
prerequisites:
- kubectl-basics
- gpu-scheduling-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# Koordinator

> **CNCF 状态**: Sandbox | **类别**: Orchestration | **主要语言**: Go

## 概述

Koordinator 是由阿里巴巴（阿里云）开源的 Kubernetes 混合调度系统，2022 年进入 CNCF Sandbox。它解决的核心问题是**集群资源利用率低下**——传统 K8s 集群中，在线服务（Deployment）平均 CPU 利用率通常仅 10-20%，大量资源处于"已申请但未使用"状态。Koordinator 通过**混部（Co-location）**技术，将这些空闲资源分配给可容忍资源抢占的批处理任务（Best-Effort，如大数据、AI 训练），在保证在线服务 SLO 的前提下将集群利用率提升到 60%+。

Koordinator 扩展了 Kubernetes 调度器，引入了 **QoS（Quality of Service）分级模型**：LSE（Latency Sensitive Exclusive）、LSR（Latency Sensitive Reserved）、LS（Latency Sensitive）、BE（Best Effort）。在线服务标记为 LS/LSR，批处理标记为 BE，调度器优先保障 LS 资源，BE 任务使用超卖（overcommit）资源。

## Key Features

- **QoS 分级调度**：LSE/LSR/LS/BE 四级 QoS，精细控制工作负载优先级
- **CPU/内存混部**：在线服务空闲的 CPU 和内存资源超卖给 BE 任务
- **GPU 共享调度**：多 Pod 共享同一 GPU（时间分片或显存分区），提升 GPU 利用率
- **CPU Burst**：允许在线服务突发使用超过 limit 的 CPU，减少延迟抖动
- **弹性 Quota**：跨团队/命名空间的弹性资源配额，允许借用空闲配额
- **Koordlet**：节点级守护进程，采集真实资源使用画像，驱动重调度和抢占

## Architecture

Koordinator 由 **Koordinator Scheduler**（扩展的 K8s 调度器，支持 QoS 感知调度）、**Koordlet**（节点级 DaemonSet，采集资源指标、执行 CPU/内存隔离策略）、**Descheduler**（重调度器，检测资源热点和 QoS 违规并迁移 Pod）和 **CRD 控制器**（管理 PodMigrationJob、Device 等）组成。Koordlet 通过 cgroup 和 EDF（Earliest Deadline First）算法实现 CPU 调度隔离，确保 LS 任务优先于 BE 任务获得 CPU 时间。

## K8s 集成

Koordinator 作为标准 Kubernetes 调度器插件运行。通过 Pod 的 label/annotation 标记 QoS 级别（`koordinator.sh/qosClass: BE`），调度器自动应用对应的调度策略。也提供自定义 CRD：`PodMigrationJob`（安全迁移 Pod）、`Device`（GPU 设备管理）、`ElasticQuota`（弹性配额）。与标准 K8s PriorityClass 和 ResourceQuota 机制兼容。

## 生产部署要点

- **渐进混部**：从低资源利用率的集群开始，逐步提高 BE 工作负载比例
- **QoS 分级**：严格按业务重要性配置 QoS 级别，确保核心服务 SLO
- **CPU Burst**：为突发流量的在线服务启用 CPU Burst，减少延迟抖动
- **资源画像**：利用 Koordlet 收集的实际资源使用数据优化资源 request
- **GPU 共享**：推理服务使用 GPU 共享调度，提升 GPU 利用率
- **弹性 Quota**：跨团队使用弹性 Quota 允许资源借用，提高整体效率

## 生产场景

1. **在线+离线混部**：电商交易（LS）与数据分析（BE）混部，利用交易服务空闲时段跑分析
2. **GPU 共享推理**：多个低 QPS AI 推理服务共享 GPU，降低 GPU 成本 50%+
3. **CI/CD 批处理混部**：构建任务（BE）利用集群空闲资源，不额外采购机器
4. **弹性配额管理**：多团队共享集群，弹性 Quota 自动借用/归还资源

## 安装

```bash
# Helm 安装 Koordinator
helm repo add koordinator-sh https://koordinator-sh.github.io/charts/
helm repo update
helm install koordinator koordinator-sh/koordinator -n koordinator-system --create-namespace

# 标记 Pod 为 BE QoS（批处理任务）
kubectl apply -f - <<EOF
apiVersion: v1
kind: Pod
metadata:
  name: spark-worker
  labels:
    koordinator.sh/qos-class: BE
spec:
  schedulerName: koordinator-scheduler
  containers:
  - name: worker
    image: spark:latest
    resources:
      limits:
        kubernetes.io/batch-cpu: "4"
        kubernetes.io/batch-memory: "8Gi"
EOF
```

## 对比

| 特性 | Koordinator | Volcano | YuniKorn | 默认 Scheduler |
|------|------------|---------|---------|---------------|
| 混部（Co-location） | ✅ 核心能力 | ⚠️ | ⚠️ | ❌ |
| GPU 共享 | ✅ | ⚠️ | ❌ | ❌ |
| QoS 分级 | ✅ 4 级 | ❌ | ✅ 3 级 | ❌ |
| 批处理队列 | ⚠️ | ✅ | ✅ | ❌ |

## 参考链接

- [[deployment]]
- [[实体/crd-custom-resources.md|crd-custom-resources]]
- [[概念/controller-pattern.md|controller-pattern]]
- [[pod-lifecycle]]
- [[实体/kube-scheduler.md|kube-scheduler]]

## Related

- [[eraser]] — Eraser
- [[kubewarden]] — Kubewarden
- [[devfile]] — Devfile
- [[cohdi]] — Cohdi
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- koordinator
- index/gitops-cicd-index|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
