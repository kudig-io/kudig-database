---
title: Dex
description: '## 项目概述'
category: cncf-landscape
tags:
- k8s
- cncf
- cloud-native
- ecosystem
- apiserver
- grafana
- helm
- argocd
- postgresql
- crd
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 架构师
- 技术决策者
- SRE
estimated_read_time: 5min
intent_queries:
- Dex 是什么
- 如何 Dex
- Kubernetes 34 cncf landscape 最佳实践
trigger_keywords:
- Dex
- cncf
- landscape
---

# Dex

> **成熟度**: Sandbox | **加入时间**: 2018-11 | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官方网站** | https://dexidp.io |
| **GitHub** | https://github.com/dexidp/dex |
| **许可证** | Apache-2.0 |
| **主要语言** | Go |
| **CNCF 分类** | Security & Identity |

---

## 项目概述

Dex 是一个身份联合服务，实现 OpenID Connect (OIDC) 协议。它作为身份代理，连接各种身份提供商（LDAP、SAML、GitHub、Google 等），为 Kubernetes 和其他应用提供统一的认证接口。

## 核心特性

- **OIDC 提供商**: 标准 OpenID Connect 实现
- **身份联合**: 连接多种上游身份提供商
- **Kubernetes 集成**: 原生支持 K8s API Server 认证
- **轻量级**: 单二进制文件，资源占用小
- **可扩展**: 支持自定义连接器
- **静态配置**: YAML 配置文件，易于版本控制

---

## 架构设计

```
┌─────────────────────────────────────────────────────────────────┐
│                      Dex Architecture                            │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  ┌────────────────────────────────────────────────────────────┐ │
│  │                     Applications                           │ │
│  │  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌───────────┐ │ │
│  │  │ Kubernetes│  │  Argo CD │  │  Grafana │  │  Custom   │ │ │
│  │  │ API Server│  │          │  │          │  │   App     │ │ │
│  │  └─────┬─────┘  └────┬─────┘  └────┬─────┘  └─────┬─────┘ │ │
│  │        │             │             │              │        │ │
│  │        └─────────────┴──────┬──────┴──────────────┘        │ │
│  └────────────────────────────┬┴──────────────────────────────┘ │
│                               │                                  │
│                         OIDC/OAuth2                              │
│                               │                                  │
│  ┌────────────────────────────┼─────────────────────────────────┐│
│  │                            ▼                                  ││
│  │  ┌─────────────────────────────────────────────────────────┐││
│  │  │                       Dex                               │││
│  │  │  ┌─────────────────────────────────────────────────┐   │││
│  │  │  │              OIDC Provider                      │   │││
│  │  │  │  - Authorization Endpoint                       │   │││
│  │  │  │  - Token Endpoint                               │   │││
│  │  │  │  - UserInfo Endpoint                            │   │││
│  │  │  │  - JWKS Endpoint                                │   │││
│  │  │  └─────────────────────────────────────────────────┘   │││
│  │  │                                                         │││
│  │  │  ┌─────────────────────────────────────────────────┐   │││
│  │  │  │              Connector Interface                │   │││
│  │  │  └─────────────────────────────────────────────────┘   │││
│  │  └──────────────────────────┬──────────────────────────────┘││
│  └─────────────────────────────┼────────────────────────────────┘│
│                                │                                  │
│          ┌─────────────────────┼─────────────────────┐           │
│          ▼                     ▼                     ▼           │
│  ┌──────────────┐     ┌──────────────┐     ┌──────────────────┐ │
│  │    LDAP      │     │    SAML      │     │  Social Login    │ │
│  │   (AD)       │     │   (Okta)     │     │ GitHub/Google    │ │
│  └──────────────┘     └──────────────┘     └──────────────────┘ │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

---

## 快速开始

### 安装 Dex

```bash
# Helm 安装
helm repo add dex https://charts.dexidp.io
helm install dex dex/dex --namespace dex --create-namespace -f values.yaml
```

### 配置文件

```yaml
# config.yaml
issuer: https://dex.example.com
storage:
  type: kubernetes
  config:
    inCluster: true

web:
  http: 0.0.0.0:5556

connectors:
  # GitHub 连接器
  - type: github
    id: github
    name: GitHub
    config:
      clientID: $GITHUB_CLIENT_ID
      clientSecret: $GITHUB_CLIENT_SECRET
      redirectURI: https://dex.example.com/callback
      orgs:
        - name: my-org
          teams:
            - dev-team

  # LDAP 连接器
  - type: ldap
    id: ldap
    name: LDAP
    config:
      host: ldap.example.com:636
      insecureNoSSL: false
      bindDN: cn=admin,dc=example,dc=com
      bindPW: admin-password
      userSearch:
        baseDN: ou=users,dc=example,dc=com
        filter: "(objectClass=person)"
        username: uid
        idAttr: uid
        emailAttr: mail
        nameAttr: cn
      groupSearch:
        baseDN: ou=groups,dc=example,dc=com
        filter: "(objectClass=groupOfNames)"
        userMatchers:
          - userAttr: DN
            groupAttr: member
        nameAttr: cn

staticClients:
  - id: kubernetes
    name: Kubernetes
    secret: kubernetes-secret
    redirectURIs:
      - http://localhost:8000/callback
      
  - id: argocd
    name: Argo CD
    secret: argocd-secret
    redirectURIs:
      - https://argocd.example.com/auth/callback
```

---

## Kubernetes API Server 集成

```yaml
# kube-apiserver 配置
apiVersion: v1
kind: Pod
metadata:
  name: kube-apiserver
spec:
  containers:
    - name: kube-apiserver
      command:
        - kube-apiserver
        - --oidc-issuer-url=https://dex.example.com
        - --oidc-client-id=kubernetes
        - --oidc-username-claim=email
        - --oidc-groups-claim=groups
        - --oidc-ca-file=/etc/kubernetes/pki/dex-ca.crt
```

### kubectl 登录

```bash
# 使用 kubelogin
kubectl oidc-login setup \
  --oidc-issuer-url=https://dex.example.com \
  --oidc-client-id=kubernetes \
  --oidc-client-secret=kubernetes-secret

# kubeconfig
users:
  - name: oidc-user
    user:
      exec:
        apiVersion: client.authentication.k8s.io/v1beta1
        command: kubectl
        args:
          - oidc-login
          - get-token
          - --oidc-issuer-url=https://dex.example.com
          - --oidc-client-id=kubernetes
          - --oidc-client-secret=kubernetes-secret
```

---

## 连接器配置

### Google

```yaml
connectors:
  - type: google
    id: google
    name: Google
    config:
      clientID: xxx.apps.googleusercontent.com
      clientSecret: xxx
      redirectURI: https://dex.example.com/callback
      hostedDomains:
        - example.com
```

### SAML (Okta)

```yaml
connectors:
  - type: saml
    id: okta
    name: Okta
    config:
      ssoURL: https://xxx.okta.com/app/xxx/sso/saml
      ca: /path/to/ca.crt
      redirectURI: https://dex.example.com/callback
      usernameAttr: name
      emailAttr: email
      groupsAttr: groups
```

---

## 最佳实践

1. **HTTPS**: 生产环境必须启用 HTTPS
2. **存储后端**: 使用 Kubernetes CRD 或 PostgreSQL 持久化
3. **密钥轮换**: 定期轮换 signing keys
4. **审计日志**: 启用访问日志记录
5. **高可用**: 部署多副本 + 共享存储

---

## 参考资源

- [官方文档](https://dexidp.io/docs/)
- [GitHub Repo](https://github.com/dexidp/dex)
- [连接器列表](https://dexidp.io/docs/connectors/)

---

**维护者**: Kudig Team | **许可证**: MIT
