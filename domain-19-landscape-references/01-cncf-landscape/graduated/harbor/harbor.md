---
title: Harbor
description: 'description: ''## 项目概述'''
category: general
tags:
- cncf
- ecosystem
- helm
- docker
- harbor
- redis
- postgresql
- job
- ingress
- rbac
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Harbor 是什么
- 如何 Harbor
- Kubernetes 19 landscape references 最佳实践
trigger_keywords:
- Harbor
- landscape
- references
prerequisites:
- kubectl-basics
- cncf-ecosystem
- helm-basics
- redis-basics
---

title: Harbor
description: '## 项目概述'
category: cncf-landscape
tags:
- k8s
- cncf
- cloud-native
- ecosystem
- helm
- docker
- harbor
- redis
- postgresql
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
- Harbor 是什么
- 如何 Harbor
- Kubernetes 34 cncf landscape 最佳实践
trigger_keywords:
- Harbor
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
# Harbor

> **成熟度**: Graduated | **加入时间**: 2018-07 | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官方网站** | https://goharbor.io |
| **GitHub** | https://github.com/goharbor/harbor |
| **文档** | https://goharbor.io/docs |
| **许可证** | Apache-2.0 |
| **主要语言** | Go |
| **CNCF 分类** | Provisioning |

---

## 项目概述

### 简介
Harbor 是一个开源的企业级容器镜像仓库，提供镜像管理、安全扫描、访问控制和镜像复制等功能。

### 核心定位
Harbor 扩展了开源 Docker Distribution，增加了企业级功能如安全、身份认证、管理等，是企业私有容器镜像仓库的首选方案。

### 发展历程
- **2016**: VMware 开源 Harbor 项目
- **2018-07**: 加入 CNCF 作为孵化项目
- **2020-06**: 成为 CNCF 毕业项目
- **2024**: Harbor v2.10+ 持续演进

---

## 核心功能

### 主要特性
- **镜像管理**: 支持 Docker 和 OCI 镜像格式
- **安全扫描**: 集成 Trivy 漏洞扫描
- **访问控制**: RBAC 和项目级权限
- **镜像复制**: 跨仓库镜像同步
- **内容签名**: Cosign/Notation 镜像签名
- **Helm Chart**: Helm Chart 仓库支持

### 功能架构
```
┌─────────────────────────────────────────────────────────────┐
│                         Harbor                              │
│  ┌─────────────────┐ ┌─────────────────┐ ┌───────────────┐ │
│  │      Core       │ │     Portal      │ │   Registry    │ │
│  │   (API/Auth)    │ │     (Web UI)    │ │  (Distribution)│ │
│  └─────────────────┘ └─────────────────┘ └───────────────┘ │
│  ┌─────────────────┐ ┌─────────────────┐ ┌───────────────┐ │
│  │    JobService   │ │     Trivy       │ │    Notary     │ │
│  │  (Async Tasks)  │ │   (Scanning)    │ │  (Signing)    │ │
│  └─────────────────┘ └─────────────────┘ └───────────────┘ │
│  ┌─────────────────────────────────────────────────────────┐│
│  │                     PostgreSQL / Redis                  ││
│  └─────────────────────────────────────────────────────────┘│
└─────────────────────────────────────────────────────────────┘
```

---

## 技术架构

### 核心组件
| 组件 | 功能 | 说明 |
|:---|:---|:---|
| Core | 核心服务 | API、认证、项目管理 |
| Registry | 镜像存储 | 基于 Distribution 的镜像存储 |
| Portal | Web 界面 | 用户管理界面 |
| JobService | 任务服务 | 扫描、复制等异步任务 |
| Trivy | 漏洞扫描 | 镜像安全扫描 |

### 工作原理
1. 用户通过 Web UI 或 CLI 操作
2. Core 服务处理认证和授权
3. Registry 存储和分发镜像
4. JobService 执行异步任务（扫描、复制）
5. 扫描结果和元数据存储在 PostgreSQL

---

## 使用场景

### 典型应用
- **企业镜像仓库**: 私有容器镜像存储
- **镜像安全**: 漏洞扫描和策略控制
- **多站点部署**: 跨数据中心镜像复制
- **CI/CD 集成**: 流水线镜像存储

### 适用条件
- 需要企业级镜像仓库
- 需要镜像安全扫描
- 需要细粒度访问控制
- 需要多仓库同步

### 不适用场景
- 简单的开发环境
- 公共镜像分发

---

## 快速开始

### 安装部署
```bash
# 使用 Helm 安装
helm repo add harbor https://helm.goharbor.io
helm install harbor harbor/harbor --namespace harbor --create-namespace \
  --set expose.type=ingress \
  --set expose.ingress.hosts.core=harbor.example.com \
  --set externalURL=https://harbor.example.com

# 使用 docker-compose（开发测试）
wget https://github.com/goharbor/harbor/releases/download/v2.10.0/harbor-offline-installer-v2.10.0.tgz
tar xvf harbor-offline-installer-v2.10.0.tgz
cd harbor
./install.sh
```

### 基础配置
```yaml
# harbor.yml
hostname: harbor.example.com
http:
  port: 80
https:
  port: 443
  certificate: /path/to/cert.pem
  private_key: /path/to/key.pem
harbor_admin_password: Harbor12345
database:
  password: root123
data_volume: /data
trivy:
  ignore_unfixed: false
  skip_update: false
  insecure: false
```

### 验证测试
```bash
# 登录 Harbor
docker login harbor.example.com

# 推送镜像
docker tag nginx:latest harbor.example.com/library/nginx:latest
docker push harbor.example.com/library/nginx:latest

# 拉取镜像
docker pull harbor.example.com/library/nginx:latest
```

---

## 最佳实践

### 生产环境建议
- 使用外部 PostgreSQL 和 Redis
- 配置对象存储后端（S3、GCS）
- 启用 HTTPS 和证书
- 配置高可用部署

### 性能优化
- 使用 CDN 加速镜像分发
- 配置镜像缓存代理
- 合理设置 GC 策略
- 优化存储性能

### 安全加固
- 启用漏洞扫描策略
- 配置镜像签名验证
- 使用 OIDC/LDAP 认证
- 审计操作日志

---

## 生态集成

### 相关 CNCF 项目
- **Kubernetes**: 集群镜像仓库
- **Helm**: Chart 仓库
- **Trivy**: 安全扫描
- **Notary/Cosign**: 镜像签名

### 常见集成方案
- Harbor + Kubernetes ImagePullSecret
- Harbor + CI/CD 流水线
- Harbor + Trivy 安全扫描
- Harbor + Helm Chart 仓库

---

## 参考资源

- [官方文档](https://goharbor.io/docs)
- [GitHub Repo](https://github.com/goharbor/harbor)
- [CNCF 项目页面](https://www.cncf.io/projects/harbor/)
- [Harbor 博客](https://goharbor.io/blog/)

---

**维护者**: Kudig Team | **许可证**: MIT

## Related

- [[log.md|log]]
- [[domain-17-system-foundation/topic-cheat-sheet/go.md|go]]
- [[domain-17-system-foundation/topic-cheat-sheet/helm.md|helm]]
- [[domain-17-system-foundation/topic-cheat-sheet/sql.md|sql]]
- [[domain-17-system-foundation/topic-cheat-sheet/k8s.md|k8s]]
- [[entities/cncf-storage|CNCF 存储与数据库项目全景]] — Cross-reference
- [[entities/cncf-security|CNCF 安全与合规项目全景]] — Cross-reference
- [[domain-19-landscape-references/topic-index/etcd-index|etcd 知识图谱索引]]
- [[domain-19-landscape-references/topic-index/gitops-cicd-index|GitOps / CI-CD 全局索引]]
