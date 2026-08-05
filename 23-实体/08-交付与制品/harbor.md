---
title: Harbor (entities)
description: '## 概述'
summary: '该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[22-概念/01-核心架构/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]'
category: entities
tags:
- k8s
- cncf
- storage
- harbor
- helm
- containerd
- docker
- redis
- postgresql
- rbac
tier: peripheral
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Harbor 是什么
- 如何 Harbor
trigger_keywords:
- Harbor
prerequisites:
- kubectl-basics
- helm-basics
- redis-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Harbor

> **CNCF 状态**: Graduated | **类别**: Storage | **主要语言**: Go

## 概述

Harbor 是由 VMware 开源的企业级容器镜像仓库（Registry），2018 年加入 CNCF，2020 年成为 CNCF 毕业项目。Harbor 提供镜像管理、安全扫描、访问控制、镜像复制和企业级治理能力，是目前最广泛使用的开源容器 Registry 之一。它支持 OCI 标准镜像格式，并与 Kubernetes、Helm、Cosign 等云原生工具链深度集成，帮助企业在混合云和多云环境中安全地管理容器镜像生命周期。

## 核心特性

- **镜像管理**: 支持 Docker 和 OCI 镜像格式，提供项目（Project）级隔离
- **安全扫描**: 集成 [[trivy|Trivy]] 漏洞扫描，支持自动化扫描策略和合规报告
- **访问控制**: 基于 RBAC 的权限管理和项目级隔离，支持 OIDC/LDAP 集成
- **镜像复制**: 跨仓库/跨地域镜像同步，支持策略驱动的自动复制
- **内容签名**: 支持 Cosign 和 Notation 镜像签名验证，保障供应链安全
- **Helm Chart**: 内置 Helm Chart 仓库能力，统一管理应用制品

## 架构

Harbor 采用微服务架构，核心组件包括：Core（API 和 Web UI）、JobService（异步任务处理）、Registry（基于 Distribution 的镜像存储）、Portal（前端界面）、Trivy Adapter（安全扫描）、Notary/Sigstore（签名验证）。数据层使用 PostgreSQL 存储元数据，Redis 存储会话和缓存。所有组件容器化部署，支持 Helm Chart 方式在 Kubernetes 上运行。存储后端支持 S3、GCS、Azure Blob、Swift 和本地文件系统。

## Kubernetes 集成

Harbor 通过 Helm Chart 部署到 Kubernetes 集群，各组件以 Deployment 形式运行。它通过 Operator 模式管理配置和升级，支持与 Kubernetes RBAC 集成实现统一身份认证。Harbor 可作为集群内部的私有 Registry，配合镜像拉取凭证（ImagePullSecrets）为工作负载提供安全的镜像分发。同时支持基于策略的镜像自动复制，实现多集群间的镜像同步。

## 生产使用场景

1. **企业私有 Registry**: 在数据中心部署 Harbor 作为统一的镜像管理平台，集成 LDAP/OIDC 身份认证
2. **供应链安全**: 启用 Cosign 签名和 Trivy 扫描，在 CI/CD 流水线中自动拦截包含高危漏洞的镜像
3. **多云镜像同步**: 利用复制策略在多个区域的 Harbor 实例间同步镜像，实现就近拉取加速
4. **合规审计**: 通过操作日志和访问审计满足 SOC2、等保等合规要求

## 安装

```bash
helm repo add harbor https://helm.goharbor.io
helm install harbor harbor/harbor \
  --set expose.ingress.hosts.core=harbor.example.com \
  --set externalURL=https://harbor.example.com \
  --set persistence.persistentVolumeClaim.registry.size=100Gi
```

## 运维操作

### 常用命令

```bash
# 🟢 查看 Harbor Pod
kubectl get pods -n harbor

# 🟢 查看 Harbor 日志
kubectl logs -n harbor -l app=harbor,component=core --tail=50

# 🟢 Docker 登录 Harbor
docker login harbor.example.com -u admin -p <password>

# 🟢 推送镜像
docker tag myapp:v1 harbor.example.com/project/myapp:v1
docker push harbor.example.com/project/myapp:v1

# 🟢 拉取镜像
docker pull harbor.example.com/project/myapp:v1

# 🟢 Helm Chart 推送
helm push mychart-0.1.0.tgz oci://harbor.example.com/charts

# 🟢 查看项目 (API)
curl -u admin:<password> https://harbor.example.com/api/v2.0/projects

# 🟢 查看镜像标签 (API)
curl -u admin:<password> https://harbor.example.com/api/v2.0/projects/library/repositories/myapp/artifacts

# 🟡 触发扫描 (API)
curl -X POST -u admin:<password> https://harbor.example.com/api/v2.0/projects/library/repositories/myapp/artifacts/v1/scan
```

### K8s ImagePullSecret 配置

```yaml
# 创建 Docker Registry Secret
kubectl create secret docker-registry harbor-creds \
  --docker-server=harbor.example.com \
  --docker-username=robot$pull \
  --docker-password=<token> \
  -n default
---
# 在 Pod 中使用
spec:
  imagePullSecrets:
  - name: harbor-creds
  containers:
  - name: app
    image: harbor.example.com/project/myapp:v1
```

## 故障排查

| 问题 | 原因 | 解决方案 |
|------|------|----------|
| 推送失败 401 | 认证失败 | 检查用户名/密码/Robot 账户 |
| 拉取失败 | ImagePullSecret 缺失 | 配置 imagePullSecrets |
| 磁盘空间不足 | 镜像堆积 | 配置 GC 策略 |
| 扫描失败 | Trivy 更新失败 | 检查网络/离线库 |
| 复制失败 | 目标不可达 | 检查网络和凭据 |
| 性能下降 | 并发拉取高 | 扩容/使用 CDN |

### 垃圾回收 (GC)

```bash
# 通过 API 触发 GC
curl -X POST -u admin:<password> \
  -H "Content-Type: application/json" \
  -d '{"schedule":{"type":"Weekly","cron":"0 0 * * 6"},"parameters":{"delete_untagged":true}}' \
  https://harbor.example.com/api/v2.0/system/gc/schedule
```

## 生产最佳实践

1. **使用 Robot 账户** - CI/CD 拉取/推送用 Robot，不用 admin
2. **启用镜像签名** - Cosign 签名 + 准入控制验证
3. **配置自动扫描** - 推送时自动扫描，阻断高危漏洞
4. **定期 GC** - 清理未打标签的镜像
5. **多副本复制** - 跨区域镜像同步
6. **RBAC 最小权限** - 项目级权限控制
7. **监控告警** - 磁盘、扫描状态、复制状态

## 检查清单

- [ ] 理解 Harbor 架构 (Core/Registry/JobService)
- [ ] 掌握 Docker/Helm 推送拉取
- [ ] 能配置 ImagePullSecret
- [ ] 理解安全扫描和签名
- [ ] 掌握故障排查流程
- [ ] 了解 GC 和复制策略

## 替代方案

| 项目 | 优势 | 劣势 |
|------|------|------|
| **Harbor** | 企业级功能全面、CNCF 毕业、社区活跃 | 资源占用较大、部署复杂 |
| Distribution | 轻量级、简单易用 | 缺乏安全扫描和访问控制 |
| Quay (Red Hat) | 与 OpenShift 深度集成 | 商业产品为主、社区版功能有限 |
| JFrog Artifactory | 多语言制品管理 | 商业产品、成本高 |

## 架构定位

在 CNCF 生态中，Harbor 属于 **Storage / Supply Chain** 类别，是云原生供应链安全的核心组件。它与 Trivy、Cosign、OPA 等项目协同工作，构建从镜像构建到部署的完整安全链路。

## 参考链接

- [[23-实体/06-安全/trivy.md|trivy]]
- [[22-概念/05-安全/secrets-management.md|secrets-management]]
- [[22-概念/09-平台与发布/ci-cd-pipeline-patterns.md|ci-cd-pipeline-patterns]]

## Related

- [[07-containerd-multi-tenant]] — containerd 多租户
- [[docker]] — Docker
- [[helm]] — Helm
- [[23-实体/06-安全/trivy.md|trivy]] — Trivy
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- 04-harbor-enterprise-security-scanning
- 99-harbor-enterprise-guide
- 01-harbor-enterprise-image-registry
- harbor
- [[23-实体/15-参考与索引/cncf-storage.md|CNCF 存储与数据库项目全景]] — Cross-reference
- [[23-实体/15-参考与索引/cncf-security.md|CNCF 安全与合规项目全景]] — Cross-reference
- [[21-生态参考/03-领域索引/etcd-index.md|etcd 知识图谱索引]]
- [[21-生态参考/03-领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
