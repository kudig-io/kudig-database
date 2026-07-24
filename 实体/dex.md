---
title: Dex (entities)
description: '## 概述'
summary: 'Dex 是一个身份联合服务，实现 OpenID Connect (OIDC) 协议。它作为身份代理，连接各种身份提供商（LDAP、SAML、GitHub、Google 等），为 Kubernetes 和其他应用提供统一的认证接口。'
category: entities
tags:
- k8s
- cncf
- observability
- dex
- prometheus
- grafana
- argocd
- postgresql
- crd
- operator
tier: supporting
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Dex 是什么
- 如何 Dex
trigger_keywords:
- Dex
prerequisites:
- kubectl-basics
- prometheus-basics
- monitoring-basics
- gitops-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Dex

> **CNCF 状态**: Sandbox | **类别**: Observability | **主要语言**: Go

## 概述

Dex 是一个身份联合服务，实现 OpenID Connect (OIDC) 协议。它作为身份代理，连接各种身份提供商（LDAP、SAML、GitHub、Google 等），为 Kubernetes 和其他应用提供统一的认证接口。Dex 是 Kubernetes 生态中最流行的轻量级 OIDC Provider，被 ArgoCD、Grafana、Harbor 等广泛集成。

## 核心能力

- **OIDC 提供商**: 标准 OpenID Connect 实现，签发 ID Token 和 Access Token
- **身份联合**: 连接多种上游身份提供商（LDAP、SAML、GitHub、Google、Microsoft）
- **Kubernetes 集成**: 原生支持 K8s API Server 认证（--oidc-issuer-url）
- **轻量级**: 单二进制文件，资源占用小，启动快
- **可扩展**: 支持自定义连接器和 gRPC API
- **静态配置**: YAML 配置文件，易于版本控制和 GitOps

## 架构

Dex 基于 Go 实现，单二进制部署。核心组件包括：OIDC Server（处理 /auth、/token、/keys 端点）、Connector Manager（管理上游身份提供商连接）、Storage Backend（存储 AuthCode、RefreshToken、SigningKeys，支持 Kubernetes CRD、PostgreSQL、SQLite、etcd）。Dex 不存储用户信息，每次认证都转发到上游 IdP，仅缓存 Token 和 Session。

## K8s 集成

Dex 与 Kubernetes API Server 集成作为 OIDC Provider。配置 kube-apiserver 的 `--oidc-issuer-url` 指向 Dex，kubectl 通过 OIDC 流程获取 Token。ArgoCD、Grafana、Harbor 等 CNCF 项目原生支持 Dex 作为 OIDC Provider。通过 Group Claim 映射 K8s RBAC 角色，实现 AD/LDAP 组到 K8s 权限的自动映射。

## 生产部署要点

- **HTTPS**: 生产环境必须启用 HTTPS，配置有效 TLS 证书
- **存储后端**: 使用 Kubernetes CRD 或 PostgreSQL 持久化（避免 SQLite）
- **密钥轮换**: 定期轮换 signing keys，配置 rotationInterval
- **审计日志**: 启用访问日志记录所有认证事件
- **高可用**: 部署多副本 + 共享存储（PostgreSQL/etcd）

## 安装与配置

```bash
# Helm 安装 Dex
helm repo add dex https://charts.dexidp.io
helm install dex dex/dex -n dex --create-namespace \
  --set config.issuer=https://dex.company.com \
  --set config.storage.type=postgres \
  --set config.storage.config.host=pg-primary.database.svc

# 等待就绪
kubectl wait --for=condition=available deployment/dex -n dex --timeout=120s

# 验证 OIDC 端点
curl -s https://dex.company.com/.well-known/openid-configuration | jq .
```

```yaml
# Dex 配置文件示例 (dex-config.yaml)
issuer: https://dex.company.com
storage:
  type: postgres
  config:
    host: pg-primary.database.svc
    port: 5432
    database: dex
    user: dex
    password: $DB_PASSWORD
    ssl:
      mode: require
web:
  https: 0.0.0.0:5556
  tlsCert: /etc/dex/tls/tls.crt
  tlsKey: /etc/dex/tls/tls.key
connectors:
- type: ldap
  id: ldap
  name: Corporate LDAP
  config:
    host: ldap.company.com:636
    insecureNoSSL: false
    bindDN: cn=dex,ou=service,dc=company,dc=com
    bindPW: $LDAP_BIND_PASSWORD
    userSearch:
      baseDN: ou=people,dc=company,dc=com
      filter: "(objectClass=person)"
      username: mail
      idAttr: uid
      emailAttr: mail
      nameAttr: cn
    groupSearch:
      baseDN: ou=groups,dc=company,dc=com
      filter: "(objectClass=groupOfNames)"
      userMatchers:
      - userAttr: DN
        groupAttr: member
      nameAttr: cn
staticClients:
- id: kubernetes
  redirectURIs:
  - http://localhost:8000
  name: Kubernetes
  secret: $K8S_CLIENT_SECRET
- id: argocd
  redirectURIs:
  - https://argocd.company.com/auth/callback
  name: ArgoCD
  secret: $ARGOCD_CLIENT_SECRET
```

## 运维操作

```bash
# 🟢 查看 Dex 状态
kubectl get pods -n dex
kubectl logs -n dex -l app=dex --tail=50

# 🟢 验证 OIDC 端点
curl -s https://dex.company.com/.well-known/openid-configuration
curl -s https://dex.company.com/keys | jq .

# 🟡 更新 Dex 配置（添加新客户端）
kubectl edit configmap dex-config -n dex
kubectl rollout restart deployment/dex -n dex

# 🟢 查看认证日志
kubectl logs -n dex -l app=dex | grep "auth"

# 🟡 轮换 Signing Keys
kubectl delete secret dex-signing-keys -n dex
kubectl rollout restart deployment/dex -n dex

# 🔴 删除所有 AuthCode/RefreshToken（强制重新登录）
kubectl delete authcodes.dex.coreos.com --all -n dex
kubectl delete refreshtokens.dex.coreos.com --all -n dex
```

## 故障排查

| 症状 | 可能原因 | 排查命令 | 修复方案 |
|------|----------|----------|----------|
| 登录失败 500 | LDAP/上游 IdP 不可达 | `kubectl logs -n dex -l app=dex` | 检查 LDAP 连接和凭据 |
| Token 验证失败 | Signing keys 轮换或时钟不同步 | `curl dex/keys` | 同步 NTP，检查 keys 轮换 |
| kubectl 认证失败 | issuer-url 或 client-id 不匹配 | `kubectl config view` | 检查 kubeconfig OIDC 配置 |
| 存储后端连接失败 | PostgreSQL 不可达或凭据错误 | `kubectl logs dex-xxx` | 检查 DB 连接配置 |
| 回调 URL 错误 | redirectURI 未在白名单中 | 查看 Dex 日志 | 添加正确的 redirectURI |

```
排查流程：
├── 认证失败
│   ├── 检查 Dex 日志中的错误信息
│   ├── 验证上游 IdP 连接（LDAP/SAML/GitHub）
│   ├── 确认 client_id 和 client_secret 正确
│   └── 检查 redirectURI 是否在白名单
├── K8s 集成问题
│   ├── 确认 kube-apiserver --oidc-issuer-url 配置
│   ├── 检查 issuer URL 是否可访问
│   ├── 验证 Group Claim 映射 RBAC
│   └── 检查时钟同步
└── 存储问题
    ├── 检查 PostgreSQL/etcd 连接
    ├── 确认 CRD 存储后端 RBAC 权限
    └── 查看 Dex 启动日志
```

## 生产案例

### 案例 1：K8s 集群统一认证网关

- **场景**：多个 K8s 集群和 CNCF 工具（ArgoCD/Grafana/Harbor）需要统一认证，之前各自配置 LDAP
- **排查**：各工具 LDAP 配置不一致，权限管理分散，无法统一审计
- **方案**：部署 Dex 作为统一 OIDC Provider，所有工具通过 Dex 认证，Dex 后端连接企业 LDAP
- **效果**：统一认证入口，权限管理集中化，新工具接入认证从 1 天降至 10 分钟

### 案例 2：GitOps 工具链 SSO

- **场景**：ArgoCD + Grafana + Harbor 工具链，开发者需要记住多套账号密码
- **排查**：多套认证体验差，密码管理混乱，离职员工账号清理困难
- **方案**：Dex 集成 GitHub OAuth，所有工具通过 Dex SSO，GitHub 组织成员自动获得访问权限
- **效果**：开发者一次登录访问所有工具，离职员工 GitHub 移除后自动失效，审计日志完整

## 架构定位

在 CNCF 生态中，dex 属于 **Observability** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[实体/prometheus-grafana.md|prometheus-grafana]]
- [[实体/argocd.md|[[ArgoCD|argocd]]]]
- [[实体/crd-custom-resources.md|crd-custom-resources]]
- [[概念/storage-model.md|storage-model]]
- [[概念/secrets-management.md|secrets-management]]

## Related

- [[kubefleet]] — KubeFleet
- [[kuma]] — Kuma
- [[kuberhealthy]] — Kuberhealthy
- [[tokenetes]] — Tokenetes
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- 00-open-source-projects-index
- 00-open-source-projects-index
- 00-open-source-projects-index
- 00-open-source-projects-index
- 00-open-source-projects-index
- 00-open-source-projects-index
- 00-open-source-projects-index
- 00-open-source-projects-index
- [[可观测性/00-open-source-projects-index.md|00-open-source-projects-index]]
- 00-open-source-projects-index
- 04-cncf-fta-index
- 00-open-source-projects-index
- 00-open-source-projects-index
- 00-open-source-projects-index
- 00-open-source-projects-index
- [[可观测性/00-open-source-projects-index.md|00-open-source-projects-index]]
- [[可观测性/00-open-source-projects-index.md|00-open-source-projects-index]]
- 00-open-source-projects-index
- 00-open-source-projects-index
- 00-open-source-projects-index
- 00-open-source-projects-index
- 00-open-source-projects-index
- 00-open-source-projects-index
- 00-open-source-projects-index
- 00-open-source-projects-index
- 00-open-source-projects-index
- [[安全/00-open-source-projects-index.md|00-open-source-projects-index]]
- 00-open-source-projects-index
- 00-open-source-projects-index
- [[安全/00-open-source-projects-index.md|00-open-source-projects-index]]
- 00-open-source-projects-index
- [[安全/00-open-source-projects-index.md|00-open-source-projects-index]]
- 00-open-source-projects-index
- 00-open-source-projects-index
- 00-open-source-projects-index
- 00-open-source-projects-index
- 00-open-source-projects-index
- 00-open-source-projects-index
- 00-open-source-projects-index
- 00-open-source-projects-index
- 00-open-source-projects-index
- [[故障诊断/FTA故障树/fta-index.md|fta-index]]
- dex
- [[技能/节点/gpu/诊断排障/ts-ai-ml-workloads.md|AI/ML 工作负载排查]] — Cross-reference
- [[实体/cncf-security.md|CNCF 安全与合规项目全景]] — Cross-reference
- [[生态参考/领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
