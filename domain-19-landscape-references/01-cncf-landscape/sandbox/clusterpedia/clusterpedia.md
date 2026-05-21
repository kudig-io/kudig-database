---
title: Clusterpedia
description: 'description: ''## 项目概述'''
category: general
tags:
- cncf
- ecosystem
- apiserver
- helm
- mysql
- postgresql
- statefulset
- daemonset
- ingress
- rbac
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Clusterpedia 是什么
- 如何 Clusterpedia
- Kubernetes 19 landscape references 最佳实践
trigger_keywords:
- Clusterpedia
- landscape
- references
prerequisites:
- kubectl-basics
- cncf-ecosystem
- helm-basics
- mysql-basics
---

title: Clusterpedia
description: '## 项目概述'
category: cncf-landscape
tags:
- k8s
- cncf
- cloud-native
- ecosystem
- apiserver
- helm
- mysql
- postgresql
- statefulset
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
- Clusterpedia 是什么
- 如何 Clusterpedia
- Kubernetes 34 cncf landscape 最佳实践
trigger_keywords:
- Clusterpedia
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
# Clusterpedia

> **成熟度**: Sandbox | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官网** | https://clusterpedia.io/ |
| **GitHub** | https://github.com/clusterpedia-io/clusterpedia |
| **许可证** | Apache-2.0 |
| **开发语言** | Go |
| **CNCF 状态** | Sandbox |

---

## 项目概述

Clusterpedia 是一个多集群资源的统一搜索和查询引擎，类似于 Kubernetes 资源的 "百科全书"。它将多个集群的资源同步到统一的存储中，提供与 kubectl 兼容的 API 进行跨集群的资源搜索、过滤和分页查询。

### 核心特性

- **多集群搜索**: 使用 kubectl 跨多个集群搜索资源
- **统一 API**: 兼容 Kubernetes API 的查询接口
- **复杂查询**: 支持字段选择、标签过滤、所有者引用、模糊搜索
- **分页排序**: 支持资源的分页查询和排序
- **多存储后端**: MySQL, PostgreSQL, 内存数据库
- **增量同步**: 使用 Informer 增量同步集群资源变更
- **自定义资源**: 支持同步 CRD 自定义资源

---

## 快速开始

### 安装

```bash
helm repo add clusterpedia https://clusterpedia-io.github.io/clusterpedia-helm/
helm install clusterpedia clusterpedia/clusterpedia \
  --namespace clusterpedia-system \
  --create-namespace \
  --set persistenceMatchNode=None \
  --set installCRDs=true
```

### 导入集群

```yaml
apiVersion: cluster.clusterpedia.io/v1alpha2
kind: PediaCluster
metadata:
  name: cluster-production
spec:
  apiserver: https://production-cluster:6443
  caData: <base64-ca>
  tokenData: <base64-token>
  syncResources:
    - group: ""
      resources:
        - pods
        - services
        - configmaps
        - namespaces
    - group: apps
      resources:
        - deployments
        - statefulsets
        - daemonsets
    - group: networking.k8s.io
      resources:
        - ingresses
```

### 跨集群查询

```bash
# 配置 kubeconfig 使用 Clusterpedia
kubectl api-resources --api-group=clusterpedia.io

# 查询所有集群的 Deployment
kubectl get deployments.clusterpedia.io -A

# 按集群过滤
kubectl get pods.clusterpedia.io -l "search.clusterpedia.io/clusters=cluster-production"

# 按命名空间过滤
kubectl get pods.clusterpedia.io -l "search.clusterpedia.io/namespaces=default,kube-system"

# 模糊搜索
kubectl get deployments.clusterpedia.io -l "search.clusterpedia.io/names=nginx"

# 分页查询
kubectl get pods.clusterpedia.io -l "search.clusterpedia.io/limit=10,search.clusterpedia.io/offset=20"

# 按创建时间排序
kubectl get pods.clusterpedia.io -l "search.clusterpedia.io/orderby=created_at desc"
```

---

## 最佳实践

1. **资源选择**: 只同步需要查询的资源类型，减少存储和同步开销
2. **存储后端**: 大规模场景使用 PostgreSQL，小规模使用内置存储
3. **标签查询**: 利用 search label 实现复杂的跨集群查询
4. **增量同步**: Clusterpedia 使用增量同步，对源集群影响极小
5. **权限控制**: 配置 RBAC 限制用户可查询的集群和资源范围

---

## 参考资源

- [Clusterpedia 官方文档](https://clusterpedia.io/docs/)
- [Clusterpedia GitHub](https://github.com/clusterpedia-io/clusterpedia)
- [CNCF Sandbox Projects](https://www.cncf.io/sandbox-projects/)

---

**维护者**: Kudig Team | **许可证**: MIT

## Related

- [[domain-17-system-foundation/topic-cheat-sheet/go.md|go]]
- [[domain-17-system-foundation/topic-cheat-sheet/helm.md|helm]]
- [[domain-17-system-foundation/topic-cheat-sheet/sql.md|sql]]
- [[domain-17-system-foundation/topic-cheat-sheet/k8s.md|k8s]]
- [[domain-17-system-foundation/topic-cheat-sheet/git.md|git]]
- [[domain-19-landscape-references/topic-index/cluster-index|Cluster 集群知识图谱索引]]
- [[domain-19-landscape-references/topic-index/gitops-cicd-index|GitOps / CI-CD 全局索引]]
