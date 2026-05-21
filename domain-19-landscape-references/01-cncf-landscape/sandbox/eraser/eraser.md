---
title: Eraser
description: 'description: ''## 项目概述'''
category: general
tags:
- cncf
- ecosystem
- kubelet
- coredns
- helm
- containerd
- docker
- daemonset
- job
- cronjob
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Eraser 是什么
- 如何 Eraser
- Kubernetes 19 landscape references 最佳实践
trigger_keywords:
- Eraser
- landscape
- references
prerequisites:
- kubectl-basics
- cncf-ecosystem
- helm-basics
---

title: Eraser
description: '## 项目概述'
category: cncf-landscape
tags:
- k8s
- cncf
- cloud-native
- ecosystem
- kubelet
- coredns
- helm
- containerd
- docker
- daemonset
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 架构师
- 技术决策者
- SRE
estimated_read_time: 5min
intent_queries:
- Eraser 是什么
- 如何 Eraser
- Kubernetes 34 cncf landscape 最佳实践
trigger_keywords:
- Eraser
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
# Eraser

> **成熟度**: Sandbox | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **GitHub** | https://github.com/eraser-dev/eraser |
| **许可证** | Apache-2.0 |
| **开发语言** | Go |
| **CNCF 状态** | Sandbox |

---

## 项目概述

Eraser 是一个 Kubernetes 原生的镜像清理工具，用于自动从集群节点中删除存在漏洞的和未使用的容器镜像。它通过与漏洞扫描器（如 Trivy）集成，定期扫描节点上的镜像，自动移除包含高危漏洞的镜像，减小节点的攻击面并释放磁盘空间。

### 核心特性

- **漏洞镜像清理**: 自动移除包含指定严重级别漏洞的镜像
- **未使用镜像清理**: 清理节点上不再被任何 Pod 引用的陈旧镜像
- **排除列表**: 支持白名单保护关键镜像不被误删
- **定时扫描**: 可配置定时任务周期性执行清理
- **Trivy 集成**: 内置 Trivy 扫描器识别漏洞镜像
- **DaemonSet 模式**: 在每个节点上运行清理 Agent

---

## 架构设计

```
┌──────────────────────────────────────────────────┐
│          Eraser Controller Manager                 │
│                                                    │
│  ┌──────────────────────────────────┐             │
│  │    ImageJob Controller            │             │
│  │  (调度扫描/清理任务)              │             │
│  └──────────────┬───────────────────┘             │
└─────────────────┼─────────────────────────────────┘
                  │ 创建 ImageJob
┌─────────────────▼─────────────────────────────────┐
│              每个 Node                              │
│                                                     │
│  ┌──────────────────────┐                          │
│  │  Collector Pod        │  收集节点上所有镜像列表   │
│  └──────────┬───────────┘                          │
│             │                                       │
│  ┌──────────▼───────────┐                          │
│  │  Scanner Pod (Trivy)  │  扫描镜像漏洞            │
│  └──────────┬───────────┘                          │
│             │                                       │
│  ┌──────────▼───────────┐                          │
│  │  Remover Pod          │  删除漏洞/未使用镜像     │
│  └──────────────────────┘                          │
└─────────────────────────────────────────────────────┘
```

---

## 快速开始

### 安装

```bash
# 使用 Helm 安装
helm repo add eraser https://eraser-dev.github.io/eraser/
helm install eraser eraser/eraser \
  --namespace eraser-system \
  --create-namespace
```

### 配置漏洞清理

```yaml
# eraser-config ConfigMap
apiVersion: v1
kind: ConfigMap
metadata:
  name: eraser-manager-config
  namespace: eraser-system
data:
  controller_manager_config.yaml: |
    manager:
      runtime: containerd
      profile:
        enabled: true
        port: 6060
      imageJob:
        cleanup:
          delayAfterSuccess: "0s"
          delayAfterFailure: "24h"
      schedule: "0 2 * * *"   # 每天凌晨 2 点执行

    components:
      scanner:
        enabled: true
        config:
          vulnerabilities:
            enabled: true
            securityContext:
              # Critical 和 High 漏洞的镜像会被清理
              severities:
                - CRITICAL
                - HIGH
            ignoreUnfixed: true

      collector:
        enabled: true

      remover:
        enabled: true
```

### 配置排除列表

```yaml
apiVersion: eraser.sh/v1alpha3
kind: ImageList
metadata:
  name: excluded
spec:
  images:
    # 保护这些镜像不被清理
    - docker.io/library/nginx:*
    - registry.k8s.io/pause:*
    - quay.io/myorg/critical-app:*
```

### 手动触发清理

```yaml
# 手动创建 ImageJob 立即执行
apiVersion: eraser.sh/v1alpha3
kind: ImageJob
metadata:
  name: manual-cleanup
spec:
  jobTemplate:
    spec:
      template:
        spec:
          containers:
            - name: collector
              image: ghcr.io/eraser-dev/collector:latest
            - name: scanner
              image: ghcr.io/eraser-dev/eraser-trivy-scanner:latest
            - name: remover
              image: ghcr.io/eraser-dev/remover:latest
```

---

## 高级配置

### 仅清理未使用的镜像（不扫描漏洞）

```yaml
components:
  scanner:
    enabled: false
  collector:
    enabled: true
  remover:
    enabled: true
    config:
      # 清理超过 7 天未使用的镜像
      unusedImageAge: "168h"
```

### 节点选择器

```yaml
manager:
  nodeFilter:
    selectors:
      - matchExpressions:
          - key: node-role.kubernetes.io/worker
            operator: Exists
```

---

## 与其他方案对比

| 特性 | Eraser | kubelet GC | kube-image-keeper | 手动清理 |
|:---|:---|:---|:---|:---|
| 漏洞清理 | 支持 (Trivy) | 不支持 | 不支持 | 不支持 |
| 未使用清理 | 支持 | 支持 | 镜像缓存 | 手动 |
| 排除列表 | CRD 配置 | 无 | 无 | 无 |
| 调度方式 | CronJob/手动 | 自动 | 持续运行 | 手动 |
| K8s 原生 | CRD/Operator | 内置 | Operator | 脚本 |

---

## 最佳实践

1. **排除列表**: 将关键系统镜像（pause、coredns 等）加入排除列表
2. **渐进部署**: 先在非生产集群测试清理策略，确认不会误删关键镜像
3. **执行时间**: 将清理任务安排在低峰时段执行，减少对节点的影响
4. **严重级别**: 根据组织安全策略选择需要清理的漏洞严重级别
5. **磁盘监控**: 配合节点磁盘使用率监控，动态调整清理频率

---

## 参考资源

- [Eraser GitHub](https://github.com/eraser-dev/eraser)
- [Eraser 文档](https://eraser-dev.github.io/eraser/)
- [CNCF Sandbox Projects](https://www.cncf.io/sandbox-projects/)

---

**维护者**: Kudig Team | **许可证**: MIT

## Related

- [[domain-17-system-foundation/topic-cheat-sheet/go.md|go]]
- [[domain-17-system-foundation/topic-cheat-sheet/helm.md|helm]]
- [[domain-17-system-foundation/topic-cheat-sheet/k8s.md|k8s]]
- [[domain-17-system-foundation/topic-cheat-sheet/git.md|git]]
- [[domain-17-system-foundation/topic-cheat-sheet/docker.md|docker]]
- [[entities/cncf-security|CNCF 安全与合规项目全景]] — Cross-reference
- [[domain-19-landscape-references/topic-index/gitops-cicd-index|GitOps / CI-CD 全局索引]]
