---
title: Distribution (entities)
description: '## 概述'
category: entities
tags:
- k8s
- cncf
- observability
- distribution
- prometheus
- grafana
- containerd
- docker
- crd
- operator
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Distribution 是什么
- 如何 Distribution
trigger_keywords:
- Distribution
prerequisites:
- kubectl-basics
- prometheus-basics
- monitoring-basics
- etcd-basics
created: "2026-05-23"
---

# Distribution

> **CNCF 状态**: Sandbox | **类别**: Observability | **主要语言**: Go

## 概述

Distribution (原 Docker Registry) 是 OCI 容器镜像分发的参考实现。它提供了一个符合 OCI Distribution Specification 的镜像仓库服务器，用于存储和分发容器镜像及其他 OCI 工件。Distribution 是 Docker Hub、GitHub Container Registry 等大型容器仓库的底层实现。

## 核心能力

- **OCI 兼容**: 完整支持 OCI Distribution Spec
- **多存储后端**: 文件系统、S3、Azure Blob、GCS
- **镜像代理**: 作为上游仓库的 pull-through 缓存
- **Webhook 通知**: 镜像推送/拉取事件通知
- **垃圾回收**: 清理未使用的镜像层
- **认证集成**: Bearer Token、Basic Auth、LDAP

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[concepts/kubernetes-architecture-overview|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- **TLS 加密**: 生产环境必须启用 TLS
- **认证授权**: 配置 Token 或 htpasswd 认证
- **存储选择**: 生产环境使用对象存储 (S3/GCS/Azure)
- **垃圾回收**: 定期执行垃圾回收释放空间
- **高可用**: 使用共享存储后端部署多副本
- **缓存代理**: 使用 pull-through cache 减少外部流量

## 架构定位

在 CNCF 生态中，distribution 属于 **Observability** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[entities/prometheus-grafana|prometheus-grafana]]
- [[containerd]]
- [[deployment]]
- [[concepts/storage-model|storage-model]]
- [[concepts/secrets-management|secrets-management]]

## Related

- [[werf]] — werf
- [[dalec]] — Dalec
- [[vineyard]] — Vineyard
- [[docker]] — Docker
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- 02-docker-registry-enterprise-distribution
- distribution
- [[synthesis/etcd x 高可用模式|[[etcd × 高可用模式|etcd × 高可用模式]]]] — Cross-reference
- [[entities/cncf-storage|CNCF 存储与数据库项目全景]] — Cross-reference
