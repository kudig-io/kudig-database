---
title: Paralus
description: '## 项目概述'
category: cncf-landscape
tags:
- k8s
- cncf
- cloud-native
- ecosystem
- helm
- rbac
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 架构师
- 技术决策者
- SRE
estimated_read_time: 5min
intent_queries:
- Paralus 是什么
- 如何 Paralus
- Kubernetes 34 cncf landscape 最佳实践
trigger_keywords:
- Paralus
- cncf
- landscape
---

# Paralus

> **成熟度**: Sandbox | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官网** | https://paralus.io/ |
| **GitHub** | https://github.com/paralus/paralus |
| **许可证** | Apache-2.0 |
| **开发语言** | Go |
| **CNCF 状态** | Sandbox |

---

## 项目概述

Paralus 是一个 Kubernetes 零信任访问管理平台，为多集群环境提供统一的身份认证、授权和审计能力。它作为 kubectl 和 Kubernetes API 之间的安全代理层，实现基于身份的细粒度访问控制和完整的操作审计日志。

### 核心特性

- **零信任访问**: 基于身份的 Kubernetes 集群访问控制
- **多集群管理**: 统一管理多个 Kubernetes 集群的访问策略
- **即时访问 (JIT)**: 支持临时访问权限的即时授予和自动回收
- **RBAC 集成**: 与 Kubernetes RBAC 深度集成的角色管理
- **OIDC/SAML**: 集成 Okta, Azure AD, Google 等 IdP
- **审计日志**: 完整的 kubectl 命令和 API 操作审计追踪
- **kubectl 代理**: 无需直连集群 API Server

---

## 架构设计

```
┌──────────┐    ┌──────────────────────┐    ┌──────────────┐
│  kubectl  │──► │     Paralus Proxy    │──► │  K8s Cluster │
│  (User)   │    │                      │    │  API Server  │
└──────────┘    │  Authentication      │    └──────────────┘
                │  Authorization        │
                │  Audit Logging        │    ┌──────────────┐
                │                      │──► │  K8s Cluster │
                └──────────────────────┘    │  API Server  │
                         │                  └──────────────┘
                ┌────────┴────────┐
                │  Paralus Core    │
                │  (OIDC, RBAC,    │
                │   Org, Project)  │
                └─────────────────┘
```

---

## 快速开始

### 安装

```bash
helm repo add paralus https://paralus.github.io/helm-charts
helm install paralus paralus/ztka \
  --namespace paralus \
  --create-namespace \
  --set fqdn.domain=paralus.example.com
```

### 导入集群

```bash
# 在 Paralus UI 中创建集群配置后获取 bootstrap YAML
kubectl apply -f cluster-bootstrap.yaml

# 或使用 pctl CLI
pctl create cluster my-cluster \
  --description "Production cluster"
```

### 配置访问策略

```yaml
# 创建项目级角色
apiVersion: paralus.dev/v1
kind: ProjectRole
metadata:
  name: developer-readonly
spec:
  project: my-project
  roles:
    - namespace: "default"
      role: "view"
    - namespace: "staging"
      role: "edit"
---
# 分配用户角色
apiVersion: paralus.dev/v1
kind: UserRoleBinding
metadata:
  name: dev-team-binding
spec:
  user: developer@example.com
  projectRole: developer-readonly
  cluster: production-cluster
```

### 使用 kubectl 访问

```bash
# 配置 kubeconfig（通过 Paralus 代理）
pctl kubeconfig download --cluster my-cluster -o ~/.kube/paralus-config
export KUBECONFIG=~/.kube/paralus-config

# 正常使用 kubectl（所有操作经过 Paralus 代理）
kubectl get pods  # 经过认证、授权和审计
```

---

## 最佳实践

1. **零信任**: 所有集群访问通过 Paralus 代理，不直接暴露 API Server
2. **JIT 访问**: 为生产集群使用即时访问，避免长期权限授予
3. **审计合规**: 利用审计日志满足合规要求，记录所有 kubectl 操作
4. **IdP 集成**: 使用企业 IdP (Okta/Azure AD) 统一身份管理
5. **最小权限**: 基于项目和命名空间配置最小权限角色

---

## 参考资源

- [Paralus 官方文档](https://paralus.io/docs/)
- [Paralus GitHub](https://github.com/paralus/paralus)
- [CNCF Sandbox Projects](https://www.cncf.io/sandbox-projects/)

---

**维护者**: Kudig Team | **许可证**: MIT
