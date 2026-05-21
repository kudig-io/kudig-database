---
title: Volcano
description: 'description: ''## 项目概述'''
category: general
tags:
- cncf
- ecosystem
- scheduler
- helm
- job
- operator
- gpu
- nvidia
- kubeflow
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Volcano 是什么
- 如何 Volcano
- Kubernetes 19 landscape references 最佳实践
trigger_keywords:
- Volcano
- landscape
- references
prerequisites:
- kubectl-basics
- cncf-ecosystem
- helm-basics
- gpu-scheduling-basics
---

title: Volcano
description: '## 项目概述'
category: cncf-landscape
tags:
- k8s
- cncf
- cloud-native
- ecosystem
- scheduler
- helm
- job
- operator
- gpu
- nvidia
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 架构师
- 技术决策者
- SRE
estimated_read_time: 5min
intent_queries:
- Volcano 是什么
- 如何 Volcano
- Kubernetes 34 cncf landscape 最佳实践
trigger_keywords:
- Volcano
- cncf
- landscape
authors:
- name: KUDIG Team
  role: contributor
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
---
# Volcano

> **成熟度**: Incubating | **加入时间**: 2020-04 | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官方网站** | https://volcano.sh |
| **GitHub** | https://github.com/volcano-sh/volcano |
| **文档** | https://volcano.sh/docs |
| **许可证** | Apache-2.0 |
| **主要语言** | Go |
| **CNCF 分类** | Orchestration & Management |

---

## 项目概述

### 简介
Volcano 是 Kubernetes 原生的批处理系统，专为高性能计算(HPC)、AI/ML、大数据等需要批量调度的工作负载设计。它扩展了 Kubernetes 调度器，提供 Gang scheduling、公平调度等高级功能。

### 发展历程
| 时间 | 里程碑 |
|:---|:---|
| 2019 | 华为开源 |
| 2020-04 | 加入 CNCF Sandbox |
| 2022-04 | 晋升为 CNCF Incubating |

### 核心定位
Volcano 是运行 AI/ML 和大数据工作负载的首选调度器，被广泛用于分布式训练、Spark、MPI 等场景。

---

## 核心功能

```
┌─────────────────────────────────────────────────────────────────┐
│                    Volcano 调度能力                              │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  Gang Scheduling (组调度)                                        │
│  ┌─────────────────────────────────────────────────────────────┐│
│  │  Job 需要 4 个 Pod 才能运行                                 ││
│  │  ┌─────┐ ┌─────┐ ┌─────┐ ┌─────┐                          ││
│  │  │Pod 1│ │Pod 2│ │Pod 3│ │Pod 4│  ← 全部就绪才调度       ││
│  │  └─────┘ └─────┘ └─────┘ └─────┘                          ││
│  └─────────────────────────────────────────────────────────────┘│
│                                                                  │
│  Queue Management (队列管理)                                     │
│  ┌─────────────────────────────────────────────────────────────┐│
│  │  ┌─────────┐  ┌─────────┐  ┌─────────┐                     ││
│  │  │ Queue A │  │ Queue B │  │ Queue C │                     ││
│  │  │ Weight:3│  │ Weight:2│  │ Weight:1│                     ││
│  │  │ 50% CPU │  │ 33% CPU │  │ 17% CPU │                     ││
│  │  └─────────┘  └─────────┘  └─────────┘                     ││
│  └─────────────────────────────────────────────────────────────┘│
│                                                                  │
│  Preemption (抢占)                                               │
│  ┌─────────────────────────────────────────────────────────────┐│
│  │  高优先级 Job 可抢占低优先级 Job 的资源                     ││
│  └─────────────────────────────────────────────────────────────┘│
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

---

## 使用示例

### Volcano Job

```yaml
apiVersion: batch.volcano.sh/v1alpha1
kind: Job
metadata:
  name: pytorch-training
spec:
  minAvailable: 4  # Gang scheduling
  schedulerName: volcano
  queue: training-queue
  policies:
    - event: PodEvicted
      action: RestartJob
  tasks:
    - replicas: 1
      name: master
      template:
        spec:
          containers:
            - name: pytorch
              image: pytorch/pytorch:latest
              command: ["python", "train.py", "--rank=0"]
              resources:
                limits:
                  nvidia.com/gpu: 1
    - replicas: 3
      name: worker
      template:
        spec:
          containers:
            - name: pytorch
              image: pytorch/pytorch:latest
              command: ["python", "train.py"]
              resources:
                limits:
                  nvidia.com/gpu: 1
```

### Queue 配置

```yaml
apiVersion: scheduling.volcano.sh/v1beta1
kind: Queue
metadata:
  name: training-queue
spec:
  weight: 3
  capability:
    cpu: "100"
    memory: "200Gi"
    nvidia.com/gpu: "16"
  reclaimable: true
```

---

## 集成支持

| 框架 | 集成方式 |
|:---|:---|
| **TensorFlow** | TFJob + Volcano |
| **PyTorch** | PytorchJob + Volcano |
| **Spark** | spark-on-k8s-operator |
| **MPI** | MPIJob |
| **Kubeflow** | 原生集成 |

---

## 安装

```bash
# Helm 安装
helm repo add volcano https://volcano-sh.github.io/charts
helm install volcano volcano/volcano -n volcano-system --create-namespace
```

---

## 参考资源

- [官方文档](https://volcano.sh/docs)
- [GitHub Repo](https://github.com/volcano-sh/volcano)
- [CNCF 项目页面](https://www.cncf.io/projects/volcano/)

---

**维护者**: Kudig Team | **许可证**: MIT

## Related

- [[domain-17-system-foundation/topic-cheat-sheet/go.md|go]]
- [[domain-17-system-foundation/topic-cheat-sheet/helm.md|helm]]
- [[domain-17-system-foundation/topic-cheat-sheet/k8s.md|k8s]]
- [[domain-17-system-foundation/topic-cheat-sheet/git.md|git]]
- [[entities/kubernetes.md|kubernetes]]
- [[entities/cncf-orchestration|CNCF 编排与应用管理项目全景]] — Cross-reference
- [[domain-19-landscape-references/topic-index/ai-gpu-index|AI / GPU 基础设施知识图谱索引]]
- [[domain-19-landscape-references/topic-index/gitops-cicd-index|GitOps / CI-CD 全局索引]]
