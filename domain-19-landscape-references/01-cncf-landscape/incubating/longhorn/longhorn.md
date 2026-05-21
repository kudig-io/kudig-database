---
title: Longhorn
description: 'description: ''## 项目概述'''
category: general
tags:
- cncf
- ecosystem
- helm
- rag
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Longhorn 是什么
- 如何 Longhorn
- Kubernetes 19 landscape references 最佳实践
trigger_keywords:
- Longhorn
- landscape
- references
prerequisites:
- kubectl-basics
- cncf-ecosystem
- helm-basics
---

title: Longhorn
description: '## 项目概述'
category: cncf-landscape
tags:
- k8s
- cncf
- cloud-native
- ecosystem
- helm
- rag
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 架构师
- 技术决策者
- SRE
estimated_read_time: 5min
intent_queries:
- Longhorn 是什么
- 如何 Longhorn
- Kubernetes 34 cncf landscape 最佳实践
trigger_keywords:
- Longhorn
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
# Longhorn

> **成熟度**: Incubating | **加入时间**: 2019-10 | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官方网站** | https://longhorn.io |
| **GitHub** | https://github.com/longhorn/longhorn |
| **文档** | https://longhorn.io/docs |
| **许可证** | Apache-2.0 |
| **主要语言** | Go |
| **CNCF 分类** | Storage |

---

## 项目概述

### 简介
Longhorn 是轻量级、可靠、易用的 Kubernetes 分布式块存储系统。它将每个节点的本地存储聚合为分布式存储池，提供高可用的持久化卷。

### 发展历程
| 时间 | 里程碑 |
|:---|:---|
| 2017 | Rancher Labs 创建 |
| 2019-10 | 加入 CNCF Sandbox |
| 2021-04 | 晋升为 CNCF Incubating |

### 核心定位
Longhorn 是边缘和小型集群的理想存储方案，无需专用存储硬件，使用普通磁盘即可构建企业级存储。

---

## 架构设计

```
┌─────────────────────────────────────────────────────────────────┐
│                     Longhorn 架构                                │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  ┌─────────────────────────────────────────────────────────────┐│
│  │                    Longhorn Manager                          ││
│  │  • API Server          • Volume Controller                  ││
│  │  • Replica Controller  • Backup Controller                  ││
│  └─────────────────────────────────────────────────────────────┘│
│                              │                                   │
│         ┌────────────────────┼────────────────────┐             │
│         ▼                    ▼                    ▼             │
│  ┌─────────────┐      ┌─────────────┐      ┌─────────────┐     │
│  │   Node 1    │      │   Node 2    │      │   Node 3    │     │
│  │ ┌─────────┐ │      │ ┌─────────┐ │      │ ┌─────────┐ │     │
│  │ │Longhorn │ │      │ │Longhorn │ │      │ │Longhorn │ │     │
│  │ │ Engine  │ │      │ │ Engine  │ │      │ │ Engine  │ │     │
│  │ └────┬────┘ │      │ └────┬────┘ │      │ └────┬────┘ │     │
│  │      │      │      │      │      │      │      │      │     │
│  │ ┌────┴────┐ │      │ ┌────┴────┐ │      │ ┌────┴────┐ │     │
│  │ │ Replica │ │◄────►│ │ Replica │ │◄────►│ │ Replica │ │     │
│  │ └─────────┘ │ Sync │ └─────────┘ │ Sync │ └─────────┘ │     │
│  │   /data     │      │   /data     │      │   /data     │     │
│  └─────────────┘      └─────────────┘      └─────────────┘     │
│                                                                  │
│  Volume: 3 副本跨节点同步复制                                    │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

---

## 快速开始

### 安装

```bash
# Helm 安装
helm repo add longhorn https://charts.longhorn.io
helm install longhorn longhorn/longhorn \
  --namespace longhorn-system \
  --create-namespace

# kubectl 安装
kubectl apply -f https://raw.githubusercontent.com/longhorn/longhorn/master/deploy/longhorn.yaml
```

### 使用 PVC

```yaml
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: longhorn-pvc
spec:
  accessModes:
    - ReadWriteOnce
  storageClassName: longhorn
  resources:
    requests:
      storage: 10Gi
```

---

## 核心功能

| 功能 | 说明 |
|:---|:---|
| **副本复制** | 数据自动跨节点复制 |
| **快照** | 增量快照，空间高效 |
| **备份** | S3/NFS 备份支持 |
| **灾难恢复** | 跨集群 DR |
| **卷克隆** | 快速创建卷副本 |
| **卷扩容** | 在线扩容 |

---

## 参考资源

- [官方文档](https://longhorn.io/docs)
- [GitHub Repo](https://github.com/longhorn/longhorn)
- [CNCF 项目页面](https://www.cncf.io/projects/longhorn/)

---

**维护者**: Kudig Team | **许可证**: MIT

## Related

- [[domain-17-system-foundation/topic-cheat-sheet/go.md|go]]
- [[domain-17-system-foundation/topic-cheat-sheet/helm.md|helm]]
- [[domain-17-system-foundation/topic-cheat-sheet/k8s.md|k8s]]
- [[domain-17-system-foundation/topic-cheat-sheet/git.md|git]]
- [[entities/kubernetes.md|kubernetes]]
- [[references/release-notes-storage|发布说明索引 — 存储]] — Cross-reference
- [[references/release-notes-reading-guide|发布说明阅读指南]] — Cross-reference
- [[concepts/storage-tool-evolution|存储工具演进]] — Cross-reference
- [[entities/cncf-storage|CNCF 存储与数据库项目全景]] — Cross-reference
- [[domain-19-landscape-references/topic-index/backup-dr-index|Backup & DR 备份与灾备知识图谱索引]]
- [[domain-19-landscape-references/topic-index/pvc-index|PVC 知识图谱索引]]
- [[domain-19-landscape-references/topic-index/storage-index|Storage 存储知识图谱索引]]
- [[domain-19-landscape-references/topic-index/gitops-cicd-index|GitOps / CI-CD 全局索引]]
- [[domain-19-landscape-references/topic-index/csi-index|CSI (Container Storage Interface) 知识图谱索引]]
- [[domain-19-landscape-references/topic-release-notes/storage/longhorn/RELEASE-NOTES-1.9|longhorn v1.9 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/storage/longhorn/RELEASE-NOTES-0.8|longhorn v0.8 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/storage/longhorn/RELEASE-NOTES-1.8|longhorn v1.8 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/storage/longhorn/RELEASE-NOTES-1.3|longhorn v1.3 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/storage/longhorn/RELEASE-NOTES-0.2|longhorn v0.2 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/storage/longhorn/RELEASE-NOTES-1.7|longhorn v1.7 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/storage/longhorn/RELEASE-NOTES-0.6|longhorn v0.6 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/storage/longhorn/RELEASE-NOTES-1.6|longhorn v1.6 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/storage/longhorn/RELEASE-NOTES-0.7|longhorn v0.7 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/storage/longhorn/RELEASE-NOTES-1.2|longhorn v1.2 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/storage/longhorn/RELEASE-NOTES-0.3|longhorn v0.3 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/storage/longhorn/RELEASE-NOTES-1.5|longhorn v1.5 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/storage/longhorn/RELEASE-NOTES-1.10|longhorn v1.10 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/storage/longhorn/RELEASE-NOTES-0.4|longhorn v0.4 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/storage/longhorn/RELEASE-NOTES-1.0|longhorn v1.0 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/storage/longhorn/RELEASE-NOTES-1.4|longhorn v1.4 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/storage/longhorn/RELEASE-NOTES-1.11|longhorn v1.11 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/storage/longhorn/RELEASE-NOTES-0.5|longhorn v0.5 Release Notes]]
