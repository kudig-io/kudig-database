---
title: OpenKruise
description: 'description: ''## 项目概述'''
category: general
tags:
- cncf
- ecosystem
- helm
- statefulset
- daemonset
- job
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 15min
intent_queries:
- OpenKruise 是什么
- 如何 OpenKruise
- Kubernetes 19 landscape references 最佳实践
trigger_keywords:
- OpenKruise
- landscape
- references
prerequisites:
- kubectl-basics
- cncf-ecosystem
- helm-basics
---

title: OpenKruise
description: '## 项目概述'
category: cncf-landscape
tags:
- k8s
- cncf
- cloud-native
- ecosystem
- helm
- statefulset
- daemonset
- job
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 架构师
- 技术决策者
- SRE
estimated_read_time: 5min
intent_queries:
- OpenKruise 是什么
- 如何 OpenKruise
- Kubernetes 34 cncf landscape 最佳实践
trigger_keywords:
- OpenKruise
- cncf
- landscape
cross_refs:
- type: fta
  path: ../domain-10-troubleshooting-diagnostics/topic-fta/list/openkruise-fta.md
  label: '故障树: openkruise'
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
# OpenKruise

> **成熟度**: Incubating | **加入时间**: 2021-05 | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官方网站** | https://openkruise.io |
| **GitHub** | https://github.com/openkruise/kruise |
| **许可证** | Apache-2.0 |
| **主要语言** | Go |
| **CNCF 分类** | App Definition & Workload |

---

## 项目概述

OpenKruise 是 Kubernetes 的增强工作负载套件，提供高级部署、原地升级、Sidecar 管理等能力。它扩展了 Kubernetes 原生工作负载，解决大规模应用管理的痛点问题。

## 核心特性

- **高级工作负载**: CloneSet、Advanced StatefulSet、Advanced DaemonSet
- **原地升级**: 更新镜像无需重建 Pod
- **Sidecar 管理**: 声明式 Sidecar 注入和独立升级
- **镜像预热**: 提前拉取镜像加速部署
- **容器重启**: 不重建 Pod 的情况下重启容器
- **保护机制**: PodUnavailableBudget 防止误操作

---

## 架构设计

```
┌─────────────────────────────────────────────────────────────────┐
│                    OpenKruise Architecture                       │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  ┌────────────────────────────────────────────────────────────┐ │
│  │                   Kruise Manager                           │ │
│  │                                                            │ │
│  │  ┌─────────────┐ ┌─────────────┐ ┌─────────────────────┐  │ │
│  │  │  CloneSet   │ │ Advanced    │ │   SidecarSet        │  │ │
│  │  │ Controller  │ │ StatefulSet │ │   Controller        │  │ │
│  │  │             │ │ Controller  │ │                     │  │ │
│  │  └─────────────┘ └─────────────┘ └─────────────────────┘  │ │
│  │                                                            │ │
│  │  ┌─────────────┐ ┌─────────────┐ ┌─────────────────────┐  │ │
│  │  │  ImagePull  │ │   BroadJob  │ │   Container         │  │ │
│  │  │   Job       │ │ Controller  │ │   Restart           │  │ │
│  │  │ Controller  │ │             │ │   Controller        │  │ │
│  │  └─────────────┘ └─────────────┘ └─────────────────────┘  │ │
│  └────────────────────────────────────────────────────────────┘ │
│                              │                                   │
│                              ▼                                   │
│  ┌────────────────────────────────────────────────────────────┐ │
│  │                    Kruise Daemon                           │ │
│  │  ┌────────────────────────────────────────────────────┐   │ │
│  │  │  每个节点运行，负责镜像预热、原地升级等底层操作      │   │ │
│  │  └────────────────────────────────────────────────────┘   │ │
│  └────────────────────────────────────────────────────────────┘ │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

---

## 快速开始

### 安装 OpenKruise

```bash
# Helm 安装
helm repo add openkruise https://openkruise.github.io/charts/
helm install kruise openkruise/kruise --namespace kruise-system --create-namespace
```

---

## CloneSet（增强 Deployment）

CloneSet 是 Deployment 的增强版本，支持原地升级、分批发布等高级特性。

```yaml
apiVersion: apps.kruise.io/v1alpha1
kind: CloneSet
metadata:
  name: my-app
spec:
  replicas: 5
  selector:
    matchLabels:
      app: my-app
  template:
    metadata:
      labels:
        app: my-app
    spec:
      containers:
        - name: nginx
          image: nginx:1.20
  updateStrategy:
    type: InPlaceIfPossible  # 优先原地升级
    inPlaceUpdateStrategy:
      gracePeriodSeconds: 30
    partition: 2             # 保留 2 个旧版本
    maxUnavailable: 1
    maxSurge: 0
  scaleStrategy:
    podsToDelete:            # 缩容时指定删除的 Pod
      - my-app-xyz12
```

### 分批发布

```yaml
updateStrategy:
  type: InPlaceIfPossible
  partition: 3  # 逐步减少 partition 实现分批发布
  # partition=3: 更新 2 个 Pod
  # partition=2: 更新 3 个 Pod
  # partition=0: 全部更新
```

---

## Advanced StatefulSet

```yaml
apiVersion: apps.kruise.io/v1beta1
kind: StatefulSet
metadata:
  name: my-statefulset
spec:
  replicas: 3
  selector:
    matchLabels:
      app: my-app
  serviceName: my-service
  podManagementPolicy: Parallel  # 并行创建/删除
  template:
    metadata:
      labels:
        app: my-app
    spec:
      containers:
        - name: app
          image: myapp:v1
  updateStrategy:
    type: RollingUpdate
    rollingUpdate:
      podUpdatePolicy: InPlaceIfPossible  # 原地升级
      partition: 1
      maxUnavailable: 1
      inPlaceUpdateStrategy:
        gracePeriodSeconds: 30
```

---

## SidecarSet（Sidecar 管理）

```yaml
apiVersion: apps.kruise.io/v1alpha1
kind: SidecarSet
metadata:
  name: logging-sidecar
spec:
  selector:
    matchLabels:
      inject-sidecar: "true"
  containers:
    - name: fluentbit
      image: fluent/fluent-bit:latest
      volumeMounts:
        - name: logs
          mountPath: /var/log/app
      resources:
        requests:
          cpu: 100m
          memory: 128Mi
  volumes:
    - name: logs
      emptyDir: {}
  updateStrategy:
    type: RollingUpdate
    partition: 0
```

### 独立升级 Sidecar

```yaml
# 只升级 sidecar 不影响主容器
updateStrategy:
  type: RollingUpdate
  rollingUpdate:
    maxUnavailable: 1
```

---

## ImagePullJob（镜像预热）

```yaml
apiVersion: apps.kruise.io/v1alpha1
kind: ImagePullJob
metadata:
  name: preheat-nginx
spec:
  image: nginx:1.21
  parallelism: 5           # 并行拉取节点数
  completionPolicy:
    type: Always
    ttlSecondsAfterFinished: 300
  selector:
    matchLabels:
      node-type: worker    # 指定节点
```

### 使用 NodeImage（节点级镜像管理）

```yaml
apiVersion: apps.kruise.io/v1alpha1
kind: NodeImage
metadata:
  name: node-1
spec:
  images:
    nginx:
      tags:
        - tag: "1.21"
          pullPolicy:
            ttlSecondsAfterFinished: 600
```

---

## ContainerRecreateRequest（容器重启）

```yaml
apiVersion: apps.kruise.io/v1alpha1
kind: ContainerRecreateRequest
metadata:
  name: restart-nginx
spec:
  podName: my-pod-xyz12
  containers:
    - name: nginx
  strategy:
    terminationGracePeriodSeconds: 30
    unreadyGracePeriodSeconds: 5
```

---

## PodUnavailableBudget（保护机制）

```yaml
apiVersion: policy.kruise.io/v1alpha1
kind: PodUnavailableBudget
metadata:
  name: my-app-pub
spec:
  selector:
    matchLabels:
      app: my-app
  maxUnavailable: 1       # 最多不可用数量
  # 或使用百分比
  # maxUnavailablePercent: 20
```

---

## BroadcastJob（广播任务）

在所有节点上运行一次性任务：

```yaml
apiVersion: apps.kruise.io/v1alpha1
kind: BroadcastJob
metadata:
  name: cleanup-job
spec:
  template:
    spec:
      containers:
        - name: cleanup
          image: busybox
          command: ["sh", "-c", "rm -rf /tmp/cache/*"]
      restartPolicy: Never
  completionPolicy:
    type: Always
    ttlSecondsAfterFinished: 300
```

---

## 与原生工作负载对比

| 特性 | Deployment | CloneSet |
|------|------------|----------|
| 原地升级 | 不支持 | 支持 |
| 分批发布 | 基础 | 精细控制 |
| 指定缩容 Pod | 不支持 | 支持 |
| 扩容顺序控制 | 不支持 | 支持 |
| 最大不可用 + 最大 Surge | 支持 | 支持 |

---

## 最佳实践

1. **原地升级**: 对于无状态应用优先使用原地升级减少调度开销
2. **镜像预热**: 大规模发布前使用 ImagePullJob 预热镜像
3. **Sidecar 管理**: 使用 SidecarSet 统一管理 Sidecar 版本
4. **保护机制**: 配置 PodUnavailableBudget 防止误操作
5. **分批发布**: 使用 partition 实现灰度发布

---

## 参考资源

- [官方文档](https://openkruise.io/docs)
- [GitHub Repo](https://github.com/openkruise/kruise)
- [最佳实践](https://openkruise.io/docs/best-practices/)
- [用户案例](https://openkruise.io/docs/user-stories/)

---

**维护者**: Kudig Team | **许可证**: MIT

## Related

- [[domain-19-landscape-references/04-cncf-fta-index.md|04-cncf-fta-index]]
- [[domain-17-system-foundation/topic-cheat-sheet/go.md|go]]
- [[domain-17-system-foundation/topic-cheat-sheet/helm.md|helm]]
- [[domain-17-system-foundation/topic-cheat-sheet/k8s.md|k8s]]
- [[domain-17-system-foundation/topic-cheat-sheet/git.md|git]]
- [[entities/cncf-cicd|CNCF CI/CD 与发布管理项目全景]] — Cross-reference
- [[domain-19-landscape-references/topic-index/openkruise-index|OpenKruise 全局索引]]
- [[domain-19-landscape-references/topic-index/gitops-cicd-index|GitOps / CI-CD 全局索引]]
