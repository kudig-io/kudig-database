---
title: Distribution (entities)
description: '## 概述'
summary: 'Distribution (原 Docker Registry) 是 OCI 容器镜像分发的参考实现。它提供了一个符合 OCI Distribution Specification 的镜像仓库服务器，用于存储和分发容器镜像及其他 OCI 工件。'
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
tier: core
created: '2026-05-23'
last_updated: 2026-07
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
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Distribution

> **CNCF 状态**: Sandbox | **类别**: Observability | **主要语言**: Go

## 概述

Distribution（原 Docker Distribution/Docker Registry）是 OCI 容器镜像分发的参考实现，由 Docker 公司开发，现由 CNCF 维护，是 CNCF 毕业项目。它提供了符合 OCI Distribution Specification 的镜像仓库服务器，用于存储和分发容器镜像及其他 OCI 工件。Distribution 是 Docker Hub、GitHub Container Registry、Harbor 等大型容器仓库的底层实现基础。

## 核心特性

- **OCI 兼容**: 完整实现 OCI Distribution Specification
- **多存储后端**: 文件系统、S3、Azure Blob Storage、GCS、Swift、Aliyun OSS
- **Pull-Through 缓存**: 作为上游仓库的代理缓存，加速镜像拉取
- **Webhook 通知**: 镜像 push/pull 事件通知外部系统
- **垃圾回收**: 清理未被引用的镜像层释放空间
- **认证集成**: Bearer Token、Basic Auth、htpasswd

## 架构

Distribution 是一个用 Go 编写的 HTTP 服务器。核心组件包括：Registry API Handler（处理 OCI Distribution API 请求）、Storage Driver（抽象存储后端，支持 S3/GCS/Azure/本地文件系统等）、Manifest Store（管理镜像清单和层）、Blob Store（存储镜像层数据）。认证通过外部 Token 服务实现（如 Docker Auth、JWT）。推送（push）时，客户端先上传层（blob），再上传清单（manifest）。拉取（pull）时反向操作。

## Kubernetes 集成

Distribution 可作为集群内的私有 Registry 部署。通过 Helm Chart 或 Kubernetes Manifest 部署为 Deployment + Service + PVC（本地存储模式）。镜像推送到 Distribution 后，通过 ImagePullSecret 为 Pod 提供认证。Distribution 也可作为 Pull-Through Cache 运行——当本地无镜像时自动从 Docker Hub 拉取并缓存，减少外部流量。支持作为 Helm Chart、OCI Artifact、Cosign 签名等 OCI 工件的存储后端。

## 生产使用场景

1. **私有镜像仓库**: 在数据中心部署 Distribution 作为内部镜像分发平台
2. **镜像缓存代理**: 在离线/受限网络环境中缓存上游镜像
3. **CI/CD 制品仓库**: 存储 CI/CD 构建的 OCI 镜像和 Helm Chart
4. **Harbor 后端**: Distribution 是 Harbor 的底层 Registry 引擎

## 安装

```bash
# Docker 快速启动
docker run -d -p 5000:5000 --name registry \
  -v registry_data:/var/lib/registry registry:2
# Kubernetes 部署
kubectl create deployment registry --image=registry:2 --port=5000
kubectl expose deployment registry --port=5000
# 或使用 Helm
helm repo add twuni https://helm.twun.io
helm install registry twuni/docker-registry
```

## 替代方案

| 项目 | 优势 | 劣势 |
|------|------|------|
| **Distribution** | CNCF 毕业、参考实现、轻量 | 无 UI、无安全扫描 |
| Harbor | 企业级、安全扫描、RBAC | 资源占用大、部署复杂 |
| Quay | 与 OpenShift 深度集成 | 商业产品 |
| zot | 轻量级、OCI 原生 | 社区较小 |

## 架构定位

在 CNCF 生态中，Distribution 属于 **Storage / Supply Chain** 类别，是容器镜像分发的标准参考实现。它是 Harbor、GitHub Container Registry 等产品的底层引擎。

## 参考链接

- [[实体/prometheus-grafana.md|prometheus-grafana]]
- [[containerd]]
- [[deployment]]
- [[概念/storage-model.md|storage-model]]
- [[概念/secrets-management.md|secrets-management]]

## Related

- [[werf]] — werf
- [[dalec]] — Dalec
- [[vineyard]] — Vineyard
- [[docker]] — Docker
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- 02-docker-registry-enterprise-distribution
- distribution
- [[概念/etcd x 高可用模式.md|[[etcd × 高可用模式|etcd × 高可用模式]]]] — Cross-reference
- [[实体/cncf-storage.md|CNCF 存储与数据库项目全景]] — Cross-reference


<!-- risk-assessed -->
