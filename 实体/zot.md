---
title: zot (entities)
description: '## 概述'
summary: 'zot 是一个生产就绪的、OCI 原生的容器镜像注册表，完全基于 OCI Distribution Specification 构建。它以单一二进制文件的形式提供，内置镜像存储、搜索、签名验证、漏洞扫描等功能，无需依赖外部数据库或缓存服务。'
category: entities
tags:
- k8s
- cncf
- image
- zot
- envoy
- opa
- crd
- operator
- rag
tier: supporting
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- zot 是什么
- 如何 zot
trigger_keywords:
- zot
prerequisites:
- kubectl-basics
- tls-basics
- policy-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# zot

> **CNCF 状态**: Sandbox | **类别**: Image | **主要语言**: Go

## 概述

zot 是由 NEC Labs 和 Anthropic 工程师开发的生产级 OCI 原生容器镜像注册表，2022 年进入 CNCF Sandbox。它完全基于 **OCI Distribution Specification** 构建，以单一 Go 二进制文件提供镜像存储、搜索、签名验证（Notary/Cosign）、漏洞扫描（Trivy 集成）等完整功能。与 Harbor 等方案不同，zot **无需外部数据库**（PostgreSQL/Redis）——所有元数据以文件系统方式存储，极大简化部署和运维。

zot 的设计目标是"**简单而生产就绪**"——一个二进制、一个配置文件、一个存储目录即可启动。它支持本地存储和 S3 兼容对象存储后端，适合从开发到生产的全场景使用。

## Key Features

- **零外部依赖**：无需 PostgreSQL、Redis 等外部服务，单二进制即可运行
- **OCI 标准兼容**：完全实现 OCI Distribution Specification v1.1
- **内置搜索**：支持按标签、摘要、CVE 搜索镜像，GraphQL API
- **签名验证**：集成 Notary v2 和 Cosign 签名验证
- **漏洞扫描**：内置 Trivy 扫描引擎，自动扫描镜像 CVE
- **多存储后端**：本地文件系统、S3、Azure Blob、GCS

## Architecture

zot 是单体架构——单个 Go 二进制集成所有功能（API Server、存储引擎、搜索引擎、扫描引擎）。存储层抽象（`storage.StorageDriver`）支持本地文件系统和 S3。搜索引擎使用嵌入式 BoltDB 索引镜像元数据。扫描引擎通过内置的 Trivy DB 扫描镜像层中的漏洞。zot 通过 OCI Distribution API 与 Docker/containerd/crictl 等标准客户端兼容。

## K8s 集成

zot 作为标准 OCI Registry 在 Kubernetes 中运行。容器运行时（containerd/CRI-O）通过配置镜像仓库地址拉取镜像。可以通过 Helm Chart 或 StatefulSet 部署到 K8s，使用 PVC 或 S3 作为存储后端。支持配置 Kubernetes ImagePullSecret 进行私有镜像认证。也支持作为集群内部缓存 Registry（pull-through cache）加速镜像拉取。

## 生产部署要点

- **TLS 加密**：生产环境始终启用 TLS 加密通信
- **垃圾回收**：启用 GC 定期清理未引用的镜像层
- **访问控制**：配置细粒度的仓库级别访问策略
- **镜像同步**：使用 onDemand 模式减少不必要的镜像拉取
- **漏洞扫描**：启用 Trivy 集成，定期扫描镜像漏洞
- **高可用**：使用 S3 等共享存储后端实现多副本部署

## 生产场景

1. **私有镜像仓库**：企业内部镜像分发，无需依赖外部 Registry
2. **边缘镜像缓存**：边缘节点本地 Registry，离线拉取镜像
3. **供应链安全**：签名验证 + 漏洞扫描，只允许安全镜像部署
4. **多租户镜像管理**：按命名空间隔离不同团队的镜像

## 安装

```bash
# 直接下载二进制运行
wget https://github.com/project-zot/zot/releases/latest/download/zot-linux-amd64
chmod +x zot-linux-amd64
./zot-linux-amd64 serve config.json

# Kubernetes Helm 部署
helm repo add zot https://zotregistry.io/charts
helm install zot zot/zot -n zot --create-namespace \
  --set persistence.enabled=true \
  --set persistence.size=100Gi

# 配置文件示例
cat > config.json <<EOF
{
  "distSpecVersion": "1.1.0",
  "storage": { "rootDirectory": "/var/lib/registry" },
  "http": { "address": "0.0.0.0", "port": "5000" },
  "extensions": {
    "search": { "enable": true },
    "lint": { "enable": true },
    "scrub": { "enable": true },
    "mgmt": { "enable": true }
  }
}
EOF
```

## 对比

| 特性 | zot | Harbor | Distribution (Registry) | Quay |
|------|-----|--------|------------------------|------|
| 外部依赖 | ❌ 无 | ✅ PG+Redis | ❌ 无 | ✅ PG+Redis |
| 漏洞扫描 | ✅ Trivy | ✅ Trivy | ❌ | ✅ Clair |
| 镜像搜索 | ✅ GraphQL | ✅ API | ❌ | ✅ |
| 多存储后端 | ✅ | ✅ S3/Azure | ✅ S3 | ✅ |

## 参考链接

- [[实体/trivy.md|trivy]]
- [[deployment]]
- [[概念/storage-model.md|storage-model]]
- [[概念/security-defense-depth.md|security-defense-depth]]

## Related

- [[envoy]] — Envoy
- [[cert-manager]] — cert-manager
- [[实体/trivy.md|trivy]] — Trivy
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[distribution]] — Distribution

- zot
- [[实体/modelpack.md|[[ModelPack|ModelPack]]]]
- [[实体/kitops.md|KitOps]]
- [[实体/copa.md|Copa (Copacetic)]]
- [[实体/stacker.md|Stacker]]
- [[实体/xregistry.md|xRegistry]]
- [[实体/cncf-storage.md|CNCF 存储与数据库项目全景]] — Cross-reference
- [[生态参考/领域索引/etcd-index.md|etcd 知识图谱索引]]


<!-- risk-assessed -->
