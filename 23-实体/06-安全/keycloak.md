---
title: Keycloak [entities]
description: '## 概述'
summary: 'Keycloak 是开源的身份和访问管理（IAM）解决方案，提供单点登录（SSO）、身份联合、用户管理和细粒度授权功能。它支持 OpenID Connect、OAuth 2.0 和 SAML 2.0 标准协议。'
category: entities
tags:
- k8s
- cncf
- observability
- keycloak
- prometheus
- grafana
- argocd
- containerd
- crd
- operator
tier: peripheral
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Keycloak 是什么
- 如何 Keycloak
trigger_keywords:
- Keycloak
prerequisites:
- kubectl-basics
- prometheus-basics
- monitoring-basics
- gitops-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Keycloak

> **CNCF 状态**: Incubating | **类别**: Observability | **主要语言**: Java

## 概述

Keycloak 是由 Red Hat 开源的身份和访问管理（IAM）解决方案，2023 年加入 CNCF Incubating。它提供单点登录（SSO）、身份联合、用户管理和细粒度授权功能，支持 OpenID Connect（OIDC）、OAuth 2.0 和 SAML 2.0 标准协议。Keycloak 是企业级 IAM 领域最流行的开源方案之一，被广泛应用于微服务、API 网关和 Kubernetes 集群的身份认证场景。

## 核心特性

- **单点登录 (SSO)**: 一次登录访问多个应用，支持 Web 和移动端
- **身份联合**: 集成 LDAP、Active Directory、社交登录（Google/GitHub/Microsoft）
- **标准协议**: OpenID Connect 1.0、OAuth 2.0、SAML 2.0
- **多租户**: Realm 隔离的多租户架构，每个租户独立管理
- **细粒度授权**: 基于角色（RBAC）、资源（UMA）和策略的访问控制
- **用户管理**: 用户注册、密码策略、多因素认证（OTP/WebAuthn）

## 架构

Keycloak 基于 Java（Quarkus）构建。核心组件包括：Auth Server（处理认证和授权请求）、Realm Manager（管理多租户 Realm）、Identity Brokering（OIDC/SAML 身份联合）、User Federation（LDAP/AD 用户同步）、Token Generator（签发 JWT/OAuth Token）。数据层使用关系型数据库（PostgreSQL、MySQL、MariaDB）存储用户和配置。Keycloak 可以集群部署，通过分布式 Infinispan 缓存实现 Session 和 Token 共享。

## Kubernetes 鿟成

Keycloak 通过 Keycloak Operator 或 Helm Chart 部署到 Kubernetes。Operator 通过 Keycloak CRD 管理实例生命周期。在 K8s 场景中，Keycloak 常作为 OIDC Provider 与 API Server 集成——配置 `--oidc-issuer-url` 使 kubectl 通过 Keycloak 认证。Ingress/API Gateway（如 Envoy、Traefik）可集成 Keycloak 实现集群入口的统一认证。支持通过 ProtocolMapper 为 K8s ServiceAccount 映射 RBAC 角色。

## 生产使用场景

1. **统一身份认证**: 为所有内部应用提供 SSO 和集中式用户管理
2. **K8s API 认证**: 作为 Kubernetes API Server 的 OIDC Provider
3. **API 网关认证**: 在 API Gateway 层集成 Keycloak 进行请求认证
4. **多租户 SaaS**: 使用 Realm 隔离为不同租户提供独立的身份管理

## 安装与配置

```bash
# Helm 安装（Bitnami Chart）
helm repo add bitnami https://charts.bitnami.com/bitnami
helm install keycloak bitnami/keycloak -n keycloak --create-namespace \
  --set auth.adminUser=admin \
  --set auth.adminPassword=secure-password \
  --set global.postgresql.auth.postgresPassword=pg-password \
  --set replicaCount=2 \
  --set production=true \
  --set proxy=edge

# 等待就绪
kubectl wait --for=condition=available statefulset/keycloak -n keycloak --timeout=180s

# 或使用 Keycloak Operator
kubectl apply -f https://raw.githubusercontent.com/keycloak/keycloak-k8s-resources/latest/kubernetes/keycloaks.k8s.keycloak.org-v1.yml
kubectl apply -f - <<EOF
apiVersion: k8s.keycloak.org/v2alpha1
kind: Keycloak
metadata:
  name: prod-keycloak
  namespace: keycloak
spec:
  instances: 3
  db:
    vendor: postgres
    host: pg-primary.database.svc
    port: 5432
    database: keycloak
    usernameSecret:
      name: keycloak-db-secret
      key: username
    passwordSecret:
      name: keycloak-db-secret
      key: password
  http:
    tlsSecret: keycloak-tls
  hostname:
    hostname: auth.company.com
  features:
    enabled:
      - token-exchange
      - admin-fine-grained-authz
EOF
```

```yaml
# K8s API Server OIDC 集成配置
# kube-apiserver 启动参数：
# --oidc-issuer-url=https://auth.company.com/realms/kubernetes
# --oidc-client-id=kubernetes
# --oidc-username-claim=preferred_username
# --oidc-groups-claim=groups
---
# kubectl OIDC 客户端配置 (kubelogin)
apiVersion: client.authentication.k8s.io/v1beta1
kind: ExecCredential
spec:
  exec:
    apiVersion: client.authentication.k8s.io/v1beta1
    command: kubectl-oidc_login
    args:
      - get-token
      - --oidc-issuer-url=https://auth.company.com/realms/kubernetes
      - --oidc-client-id=kubernetes
      - --oidc-client-secret=<secret>
```

## 运维操作

```bash
# 🟢 查看 Keycloak 实例状态
kubectl get keycloak -n keycloak
kubectl get pods -n keycloak -l app=keycloak

# 🟢 查看 Realm 列表
kubectl exec -n keycloak keycloak-0 -- /opt/keycloak/bin/kcadm.sh get realms \
  --server http://localhost:8080 --realm master --user admin --password $ADMIN_PASS

# 🟡 导出 Realm 配置（备份）
kubectl exec -n keycloak keycloak-0 -- /opt/keycloak/bin/kc.sh export \
  --dir /tmp/realm-export --realm production
kubectl cp keycloak/keycloak-0:/tmp/realm-export ./keycloak-backup/

# 🟡 导入 Realm 配置（恢复）
kubectl cp ./keycloak-backup/ keycloak/keycloak-0:/tmp/realm-import/
kubectl exec -n keycloak keycloak-0 -- /opt/keycloak/bin/kc.sh import --dir /tmp/realm-import

# 🔴 强制重置管理员密码（紧急场景）
kubectl exec -n keycloak keycloak-0 -- /opt/keycloak/bin/kcadm.sh update \
  users/<user-id>/reset-password -r master \
  -s type=password -s value=new-password -s temporary=false

# 🟢 查看活跃会话
kubectl exec -n keycloak keycloak-0 -- /opt/keycloak/bin/kcadm.sh get \
  realms/production/sessions --server http://localhost:8080 --realm master --user admin
```

## 故障排查

| 症状 | 可能原因 | 排查命令 | 修复方案 |
|------|----------|----------|----------|
| Pod CrashLoopBackOff | 数据库连接失败或内存不足 | `kubectl logs keycloak-0 -n keycloak` | 检查 DB 连接串和 JVM 内存参数 |
| 登录失败 502 | 反向代理配置错误或 TLS 证书问题 | `curl -vk https://auth.company.com` | 检查 Ingress TLS 和 proxy 设置 |
| Token 验证失败 | 时钟不同步或 Realm 配置错误 | `date -u` 对比各节点时间 | 同步 NTP，检查 issuer-url |
| 集群 Session 丢失 | Infinispan 缓存未同步 | `kubectl logs keycloak-0` 查看 JGroups | 检查节点间 7800 端口连通性 |
| LDAP 同步失败 | LDAP 服务器不可达或凭据过期 | `kubectl exec keycloak-0 -- ldapsearch -H ldap://...` | 检查 LDAP 连接和 Bind DN 密码 |

```
排查流程：
├── Pod 无法启动
│   ├── kubectl logs 查看启动日志
│   ├── 检查 PostgreSQL 连接（host/port/credentials）
│   ├── 检查 JVM 内存配置（-Xmx/-Xms）
│   └── 确认 PVC 存储充足
├── 认证流程异常
│   ├── 检查 Realm 的 issuer URL 是否可访问
│   ├── 确认客户端 redirect_uri 配置正确
│   ├── 检查时钟同步（NTP）
│   └── 查看 Keycloak 事件日志（Events）
└── 集群问题
    ├── 检查 JGroups 集群成员（jgroups-7800 端口）
    ├── 确认所有实例使用相同数据库
    └── 检查负载均衡器 sticky session 配置
```

## 生产案例

### 案例 1：K8s 多集群统一身份认证

- **场景**：企业 5 个 K8s 集群，之前使用静态 Token 认证，无法审计、无法撤销、无 MFA
- **排查**：安全审计发现 Token 泄露风险，无法实现 RBAC 与 AD 组映射，离职员工 Token 未清理
- **方案**：部署 Keycloak 作为 OIDC Provider，集成 AD，配置 --oidc-issuer-url，通过 Group Claim 映射 K8s RBAC
- **效果**：实现 SSO + MFA，离职员工自动失效，RBAC 与 AD 组自动同步，安全审计合规

### 案例 2：微服务 API 网关统一认证

- **场景**：50+ 微服务通过 API Gateway 暴露，每个服务自行实现认证逻辑，不一致且难维护
- **排查**：各服务认证实现不统一，部分服务存在认证绕过漏洞，Token 验证逻辑分散
- **方案**：Keycloak 作为统一 IdP，API Gateway (Envoy) 集成 ext_authz，服务只验证 JWT 签名
- **效果**：认证逻辑集中管理，安全漏洞修复从周级降至分钟级，新服务接入认证从 2 天降至 10 分钟

## 替代方案

| 项目 | 优势 | 劣势 | 适用场景 |
|------|------|------|----------|
| **Keycloak** | CNCF Incubating、Red Hat 支持、功能全面 | Java 资源开销大 | 企业级全功能 IAM |
| Dex | 轻量级、K8s 原生、Go 实现 | 功能少（仅 OIDC 桥接） | K8s 集群认证 |
| Authentik | Python、灵活、UI 友好 | 社区较小 | 中小企业 SSO |
| Auth0 | SaaS、零运维 | 商业产品、数据出境 | 快速上线无运维团队 |

## 架构定位

在 CNCF 生态中，Keycloak 属于 **Security / IAM** 类别，是开源 IAM 领域的标杆项目。它在 Kubernetes 身份认证生态中扮演 OIDC Provider 的核心角色。

## 参考链接

- [[23-实体/07-可观测性/prometheus-grafana.md|prometheus-grafana]]
- [[operator-pattern]]
- [[22-概念/08-可靠性与运维/microservice-resilience-patterns.md|microservice-resilience-patterns]]
- [[22-概念/05-安全/secrets-management.md|secrets-management]]
- [[22-概念/05-安全/security-defense-depth.md|security-defense-depth]]

## Related

- [[23-实体/argocd.md|[[ArgoCD|argocd]]]] — ArgoCD
- [[ovn-kubernetes]] — OVN-Kubernetes
- [[vitess]] — Vitess
- [[argo]] — Argo Workflows
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- keycloak
- [[23-实体/07-可观测性/pixie.md|Pixie]]
- [[23-实体/07-可观测性/kuberhealthy.md|Kuberhealthy]]
- [[23-实体/06-安全/kubescape.md|Kubescape]]
- [[23-实体/07-可观测性/perses.md|Perses]]
- [[23-实体/07-可观测性/03-prometheus-ha-deployment.md|Prometheus 高可用部署]]
- [[23-实体/07-可观测性/trickster.md|Trickster]]
- [[23-实体/08-交付与制品/distribution.md|Distribution]]
- [[23-实体/11-AI与边缘/hami.md|HAMI]]
- [[23-实体/03-运行时/06-containerd-observability.md|containerd 可观测性]]
- [[23-实体/09-编排调度/kubeelasti.md|KubeElastic]]
- [[23-实体/15-参考与索引/cncf-security.md|CNCF 安全与合规项目全景]] — Cross-reference


<!-- risk-assessed -->
