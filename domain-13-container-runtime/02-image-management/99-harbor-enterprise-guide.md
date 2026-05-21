---
title: Harbor 企业级镜像仓库部署指南
description: '# Harbor 企业级镜像仓库部署指南'
category: container-image-management
tags:
- k8s
- container
- image
- registry
- harbor
- helm
- docker
- redis
- postgresql
- job
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- SRE
- 运维工程师
- 开发工程师
estimated_read_time: 5min
intent_queries:
- Harbor 企业级镜像仓库部署指南 是什么
- 如何 Harbor 企业级镜像仓库部署指南
- Kubernetes 22 container image management 最佳实践
trigger_keywords:
- Harbor
- 企业级镜像仓库部署指南
- container
- image
- management
prerequisites:
- kubectl-basics
- helm-basics
- redis-basics
- tls-basics
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
authors:
- name: KUDIG Team
  role: contributor
---

# Harbor 企业级镜像仓库部署指南

> **适用版本**: Harbor v2.13.0 / Helm Chart v1.16.0  
> **最后更新**: 2026-04-24  
> **难度**: 中级

---

## 📋 目录

- [一、架构设计](#一架构设计)
- [二、Helm 部署](#二helm-部署)
- [三、高可用配置](#三高可用配置)
- [四、复制与灾备](#四复制与灾备)
- [五、漏洞扫描集成](#五漏洞扫描集成)
- [六、镜像签名与信任](#六镜像签名与信任)
- [七、CI/CD 集成](#七cicd-集成)
- [八、监控与告警](#八监控与告警)

---

## 一、架构设计

```
Harbor 核心组件:
- Core (API / UI)
- Registry (Distribution 后端)
- Jobservice (异步任务)
- PostgreSQL (元数据)
- Redis (缓存 / 队列)
- Trivy (漏洞扫描)
- Notary (内容信任)
- Chartmuseum (Helm Chart)
```

---

## 二、Helm 部署

```yaml
# values-harbor-production.yaml
expose:
  type: ingress
  tls:
    enabled: true
    certSource: auto
  ingress:
    hosts:
      core: harbor.example.com
    className: nginx
    annotations:
      cert-manager.io/cluster-issuer: "letsencrypt-prod"
      nginx.ingress.kubernetes.io/proxy-body-size: "0"

externalURL: https://harbor.example.com

persistence:
  enabled: true
  persistentVolumeClaim:
    registry:
      storageClass: "standard"
      size: 500Gi
    chartmuseum:
      size: 10Gi
    jobservice:
      size: 10Gi
    database:
      size: 10Gi
    redis:
      size: 5Gi
    trivy:
      size: 10Gi

harborAdminPassword: "ChangeMe-Strong-Passw0rd!"

core:
  replicas: 2
  resources:
    requests:
      memory: 256Mi
      cpu: 100m
    limits:
      memory: 1Gi
      cpu: 1000m

jobservice:
  replicas: 2

registry:
  replicas: 2
  resources:
    requests:
      memory: 256Mi
    limits:
      memory: 2Gi
      cpu: 2000m

trivy:
  enabled: true

notary:
  enabled: true

chartmuseum:
  enabled: true
```

```bash
helm repo add harbor https://helm.goharbor.io
helm install harbor harbor/harbor \
  --namespace harbor --create-namespace \
  --values values-harbor-production.yaml \
  --version 1.16.0
```

---

## 三、高可用配置

### 外部依赖解耦

```yaml
# 生产建议外部 PostgreSQL + Redis
database:
  type: external
  external:
    host: "harbor-db.example.com"
    port: "5432"
    username: "harbor"
    password: "external-password"
    coreDatabase: "registry"
    sslmode: "require"

redis:
  type: external
  external:
    addr: "harbor-redis.example.com:6379"
    coreDatabaseIndex: "0"

# S3 存储后端
persistence:
  imageChartStorage:
    type: s3
    s3:
      region: us-east-1
      bucket: harbor-registry
      accesskey: AKIA...
      secretkey: ...
```

---

## 四、复制与灾备

Harbor 支持跨实例复制:
- 目标: Harbor / Docker Hub / ECR / ACR / GCR / AliACR
- 触发: 事件驱动 + 定时任务
- 过滤: 按仓库、标签、资源类型

---

## 五、漏洞扫描集成

内置 Trivy:
- 自动扫描推送的镜像
- 按 CVE 严重度分级
- 可配置策略阻止漏洞镜像运行
- 支持 SBOM 生成

---

## 六、镜像签名与信任

```bash
# cosign 签名 (推荐)
cosign generate-key-pair
cosign sign --key cosign.key harbor.example.com/project/image:tag

# Harbor 中配置策略阻止未签名镜像
# Projects -> Configuration -> Deploy security
```

---

## 七、CI/CD 集成

```yaml
# GitHub Actions 示例
- name: Login to Harbor
  uses: docker/login-action@v3
  with:
    registry: harbor.example.com
    username: ${{ secrets.HARBOR_USERNAME }}
    password: ${{ secrets.HARBOR_PASSWORD }}

- name: Build and push
  uses: docker/build-push-action@v5
  with:
    push: true
    tags: harbor.example.com/project/app:${{ github.sha }}
```

---

## 八、监控与告警

```yaml
# ServiceMonitor
apiVersion: monitoring.coreos.com/v1
kind: ServiceMonitor
metadata:
  name: harbor-metrics
spec:
  selector:
    matchLabels:
      app: harbor
  endpoints:
  - port: metrics
    interval: 30s
```

| 关键告警 | 表达式 |
|:---|:---|
| 磁盘使用率 | harbor_project_quota_usage_byte / harbor_project_quota_byte > 0.85 |
| 复制失败 | harbor_replication_status{status="fail"} > 0 |

---

## 参考链接

- [Harbor 官方文档](https://goharbor.io/docs/)
- [Harbor Helm Chart](https://github.com/goharbor/harbor-helm)
- [cosign 文档](https://docs.sigstore.dev/)

---

## Obsidian 相关文档

- [[domain-13-container-runtime/MOC.md|domain-22-container-image-management MOC]]
- [[domain-13-container-runtime/README.md|Domain 22: 容器镜像管理 (Container Image Management)]]
- [[domain-13-container-runtime/00-open-source-projects-index.md|Domain-22 容器镜像管理 — 开源项目索引]]
- [[domain-13-container-runtime/01-harbor-enterprise-image-registry.md|Harbor企业级容器镜像仓库深度实践]]
- [[domain-13-container-runtime/02-docker-registry-enterprise-distribution.md|Docker Registry企业级镜像分发深度实践]]
- [[domain-13-container-runtime/03-jfrog-artifactory-enterprise.md|JFrog Artifactory Enterprise Container Registry Platform]]
- [[domain-13-container-runtime/04-harbor-enterprise-security-scanning.md|Harbor企业级镜像安全扫描深度实践]]
- [[domain-13-container-runtime/04-quay-enterprise-registry.md|Quay Container Registry 企业级镜像管理深度实践]]
- [[domain-13-container-runtime/05-gitlab-container-registry-enterprise.md|GitLab Container Registry Enterprise 深度实践]]
- [[domain-13-container-runtime/06-amazon-ecr-enterprise.md|Amazon ECR (Elastic Container Registry) Enterprise 深度实践]]

## See Also

- [[domain-13-container-runtime/05-gitlab-container-registry-enterprise.md|05-gitlab-container-registry-enterprise]]
- [[domain-13-container-runtime/06-amazon-ecr-enterprise.md|06-amazon-ecr-enterprise]]
- [[domain-13-container-runtime/01-harbor-enterprise-image-registry.md|01-harbor-enterprise-image-registry]]
- [[domain-13-container-runtime/02-docker-registry-enterprise-distribution.md|02-docker-registry-enterprise-distribution]]
