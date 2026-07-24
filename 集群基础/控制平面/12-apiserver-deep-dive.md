---
title: kube-apiserver 深度解析
description: 深入解析 kube-apiserver 的架构设计、请求处理流程、认证授权机制、Admission Control、APF 限流、审计日志与高可用部署
summary: 深入解析 kube-apiserver 的架构设计、请求处理流程、认证授权机制、Admission Control、APF 限流、审计日志与高可用部署
category: 集群基础
tags:
- k8s
- apiserver
- authentication
- authorization
- admission
- apf
- audit
- high-availability
- etcd
- kubelet
tier: supporting
created: '2026-05-23'
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 平台工程师
- 运维工程师
estimated_read_time: 20min
intent_queries:
- kube-apiserver 深度解析 是什么
- 如何 kube-apiserver 深度解析
- Kubernetes 3 control plane 最佳实践
trigger_keywords:
- kube-apiserver
- 深度解析
- control
- plane
prerequisites:
- kubectl-basics
- kubernetes-concepts
- service-mesh-basics
- prometheus-basics
- etcd-basics
- gpu-scheduling-basics
- policy-basics
k8s_versions:
- '1.25'
- '1.26'
- '1.27'
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
authors:
- name: KUDIG Team
  role: contributor
cross_refs:
- type: domain
  path: ../集群基础/
  label: '相关知识域: 集群基础'
- type: domain
  path: ../工作负载/
  label: '相关知识域: 工作负载'
- type: domain
  path: ../网络/
  label: '相关知识域: 网络'
- type: domain
  path: ../存储/
  label: '相关知识域: 存储'
- type: domain
  path: ../安全/
  label: '相关知识域: 安全'
- type: fta
  path: ../故障诊断/FTA故障树/list/apiserver-fta.md
  label: '故障树: apiserver'
- type: cheatsheet
  path: ../系统基础/topic-cheat-sheet/k8s.md
  label: '速查卡: k8s'
- type: cheatsheet
  path: ../系统基础/topic-cheat-sheet/kubectl-scene-cheatsheet.md
  label: '速查卡: kubectl-scene-cheatsheet'
related_docs:
- path: 11-etcd-deep-dive.md
  type: depth
  desc: etcd 深度解析
- path: 13-kube-controller-manager-deep-dive.md
  type: depth
  desc: KCM 深度解析
- path: ../故障诊断/FTA故障树/list/apiserver-fta.md
  type: fta
  desc: API Server 故障树
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# kube-apiserver 深度解析 (kube-apiserver Deep Dive)

> kube-apiserver 是 [[Kubernetes|Kubernetes]] 控制平面的核心组件，提供 RESTful API 接口，是所有组件通信的唯一入口

---

<!-- chunk: 1. 架构概述 (Architecture Overview) -->
## 1. 架构概述 (Architecture Overview)

### 1.1 核心功能模块

| 模块 | 英文名 | 职责 | 关键特性 |
|:---|:---|:---|:---|
| **认证模块** | Authentication | 身份验证 | X509证书、Token、OIDC、Webhook |
| **授权模块** | Authorization | 权限控制 | RBAC、ABAC、Node、Webhook |
| **准入控制** | Admission Control | 请求验证/修改 | Validating、Mutating、动态准入 |
| **API聚合** | API Aggregation | API扩展 | 自定义API Server、CRD |
| **存储层** | Storage Layer | 数据持久化 | [[etcd]]后端、缓存、Watch |
| **审计日志** | Audit Logging | 操作审计 | 请求记录、合规追踪 |
| **限流机制** | Rate Limiting | 流量控制 | APF(API Priority and Fairness) |

### 1.2 请求处理流程

```
# 🟢 低风险：只读/信息收集，通常无副作用
                                   ┌─────────────────────────────────────────────────────┐
                                   │                  kube-apiserver                      │
                                   │                                                      │
┌──────────┐    HTTPS/REST         │  ┌──────────┐   ┌──────────┐   ┌────────────────┐  │
│  Client  │ ─────────────────────▶│  │  认证    │──▶│  授权    │──▶│  准入控制      │  │
│ kubectl  │                       │  │ AuthN    │   │ AuthZ    │   │ Admission      │  │
│ pod/svc  │                       │  └──────────┘   └──────────┘   └───────┬────────┘  │
└──────────┘                       │        │              │                 │           │
                                   │        │              │                 ▼           │
                                   │  ┌─────┴──────────────┴─────────────────────────┐  │
                                   │  │              API Handler (REST)               │  │
                                   │  │  ┌────────┐ ┌────────┐ ┌────────┐ ┌────────┐ │  │
                                   │  │  │  GET   │ │  POST  │ │  PUT   │ │ DELETE │ │  │
                                   │  │  └────────┘ └────────┘ └────────┘ └────────┘ │  │
                                   │  └─────────────────────────┬─────────────────────┘  │
                                   │                            │                        │
                                   │                            ▼                        │
                                   │  ┌─────────────────────────────────────────────┐   │
                                   │  │           Registry / Storage                 │   │
                                   │  │  ┌─────────────┐    ┌──────────────────┐    │   │
                                   │  │  │   Cacher    │    │   etcd Backend   │    │   │
                                   │  │  │  (Watch)    │    │                  │    │   │
                                   │  │  └─────────────┘    └──────────────────┘    │   │
                                   │  └─────────────────────────────────────────────┘   │
                                   └─────────────────────────────────────────────────────┘
                                                              │
                                                              ▼
                                                        ┌──────────┐
                                                        │   etcd   │
                                                        └──────────┘
```
### 1.3 API 组织结构

| API组 | 路径前缀 | 包含资源 | 说明 |
|:---|:---|:---|:---|
| **Core (Legacy)** | /api/v1 | [[Pods|pods]], services, configmaps, secrets, nodes | 核心资源 |
| **apps** | /apis/apps/v1 | deployments, statefulsets, daemonsets, replicasets | 应用工作负载 |
| **batch** | /apis/batch/v1 | jobs, cronjobs | 批处理任务 |
| **networking.k8s.io** | /apis/networking.k8s.io/v1 | ingresses, networkpolicies | 网络资源 |
| **storage.k8s.io** | /apis/storage.k8s.io/v1 | storageclasses, volumeattachments | 存储资源 |
| **rbac.authorization.k8s.io** | /apis/rbac.authorization.k8s.io/v1 | roles, rolebindings, clusterroles | RBAC资源 |
| **autoscaling** | /apis/autoscaling/v2 | hpa | 自动伸缩 |
| **policy** | /apis/policy/v1 | poddisruptionbudgets | 策略资源 |
| **certificates.k8s.io** | /apis/certificates.k8s.io/v1 | certificatesigningrequests | 证书管理 |

---

<!-- chunk: 2. 认证机制 (Authentication) -->
## 2. 认证机制 (Authentication)

### 2.1 认证方式对比

| 认证方式 | 英文名 | 适用场景 | 优点 | 缺点 |
|:---|:---|:---|:---|:---|
| **X509客户端证书** | Client Certificates | 组件间通信、管理员 | 安全性高、无需额外系统 | 证书管理复杂、轮换困难 |
| **Bearer Token** | Static Token | 简单场景、测试 | 配置简单 | 安全性低、无法动态管理 |
| **Bootstrap Token** | Bootstrap Token | 节点加入集群 | 专为节点引导设计 | 临时性、有效期短 |
| **ServiceAccount Token** | SA Token | Pod内访问API | 自动管理、Namespace隔离 | 绑定到ServiceAccount |
| **OIDC** | OpenID Connect | 企业SSO集成 | 标准协议、集成方便 | 需要OIDC Provider |
| **Webhook Token** | Webhook Authentication | 自定义认证 | 灵活性高 | 增加延迟、需维护Webhook |
| **认证代理** | Authenticating Proxy | 前置认证 | 可集成多种认证系统 | 架构复杂 |

### 2.2 X509 证书认证配置

```bash
# API Server 证书参数
--client-ca-file=/etc/kubernetes/pki/ca.crt           # 客户端CA
--tls-cert-file=/etc/kubernetes/pki/apiserver.crt     # 服务器证书
--tls-private-key-file=/etc/kubernetes/pki/apiserver.key  # 服务器私钥

# 证书中的用户信息映射
# Common Name (CN) -> Username
# Organization (O) -> Groups

# 示例: 创建管理员证书
cat > admin-csr.json << EOF
{
  "CN": "admin",
  "key": { "algo": "rsa", "size": 2048 },
  "names": [{ "O": "system:masters" }]
}
EOF

cfssl gencert -ca=ca.pem -ca-key=ca-key.pem \
  -config=ca-config.json -profile=client \
  admin-csr.json | cfssljson -bare admin
```

### 2.3 ServiceAccount Token 配置

```yaml
# ServiceAccount Token 自动挂载
apiVersion: v1
kind: ServiceAccount
metadata:
  name: my-service-account
  namespace: default
automountServiceAccountToken: true  # 默认true

---
# Pod 使用特定 ServiceAccount
apiVersion: v1
kind: Pod
metadata:
  name: my-pod
spec:
  serviceAccountName: my-service-account
  containers:
  - name: app
    image: my-app:latest
    # Token 自动挂载到 /var/run/secrets/kubernetes.io/serviceaccount/
```

```bash
# API Server ServiceAccount 相关参数
--service-account-key-file=/etc/kubernetes/pki/sa.pub      # SA公钥
--service-account-signing-key-file=/etc/kubernetes/pki/sa.key  # SA私钥
--service-account-issuer=https://kubernetes.default.svc    # Token签发者

# Bound ServiceAccount Token (推荐)
--service-account-extend-token-expiration=true
--service-account-max-token-expiration=48h
```

### 2.4 OIDC 配置

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# API Server OIDC 参数
--oidc-issuer-url=https://accounts.google.com  # OIDC Provider URL
--oidc-client-id=kubernetes                     # Client ID
--oidc-username-claim=email                     # 用户名映射字段
--oidc-username-prefix=oidc:                    # 用户名前缀
--oidc-groups-claim=groups                      # 组映射字段
--oidc-groups-prefix=oidc:                      # 组前缀
--oidc-ca-file=/etc/kubernetes/pki/oidc-ca.crt # OIDC Provider CA

# 使用示例 (kubectl)
kubectl config set-credentials oidc-user \
  --auth-provider=oidc \
  --auth-provider-arg=idp-issuer-url=https://accounts.google.com \
  --auth-provider-arg=client-id=kubernetes \
  --auth-provider-arg=refresh-token=<refresh_token> \
  --auth-provider-arg=id-token=<id_token>
```
### 2.5 Webhook Token 认证配置

```bash
# API Server Webhook 认证参数
--authentication-token-webhook-config-file=/etc/kubernetes/webhook-auth-config.yaml
--authentication-token-webhook-cache-ttl=2m
--authentication-token-webhook-version=v1beta1
```

```yaml
# webhook-auth-config.yaml
apiVersion: v1
kind: Config
clusters:
- name: webhook-auth
  cluster:
    server: https://auth-webhook.kube-system.svc:443/authenticate
    certificate-authority: /etc/kubernetes/pki/webhook-ca.crt
contexts:
- name: default
  context:
    cluster: webhook-auth
    user: webhook
users:
- name: webhook
  user:
    client-certificate: /etc/kubernetes/pki/webhook-client.crt
    client-key: /etc/kubernetes/pki/webhook-client.key
current-context: default
```

### 2.6 认证代理配置

```bash
# 认证代理参数
--requestheader-username-headers=X-Remote-User
--requestheader-group-headers=X-Remote-Group
--requestheader-extra-headers-prefix=X-Remote-Extra-
--requestheader-client-ca-file=/etc/kubernetes/pki/front-proxy-ca.crt
--requestheader-allowed-names=front-proxy-client
```

### 2.7 结构化认证配置 (Structured Authentication Configuration)

> **适用版本**: Kubernetes 1.30+ (Beta, 默认禁用), 1.32+ (更完善，支持动态重载)

#### 核心概念

结构化认证配置（Structured Authentication Configuration）是 Kubernetes 1.30 引入的新机制，允许通过 **配置文件** 而非启动参数来声明多个认证提供者（Authentication Provider）。其核心优势在于：

- **多 Provider 支持**: 可在单个配置文件中同时声明多个 JWT issuer、Webhook、证书和匿名认证
- **动态重载** (1.32+): 配置文件变更后无需重启 API Server，支持在线热更新
- **声明式管理**: 配置即代码，便于 GitOps 流程和版本控制
- **更灵活的 Claim 映射**: 支持复杂的用户/组/UID 映射规则

#### 与旧方式对比

| 特性 | 旧启动参数方式 (`--oidc-*`) | 结构化认证配置 (`--authentication-config`) |
|:---|:---|:---|
| **Provider 数量** | 仅支持单个 OIDC issuer | 支持多个 JWT issuer + 其他类型 |
| **配置方式** | 命令行参数 | YAML 配置文件 |
| **动态更新** | 必须重启 API Server | 1.32+ 支持动态重载 |
| **Claim 映射** | 简单字段映射 | 支持前缀、表达式、复杂映射 |
| **证书认证** | `--client-ca-file` 参数 | 配置文件内声明 |
| **匿名认证** | `--anonymous-auth` 布尔参数 | 配置文件内声明，可配置匿名用户 |
| **Webhook 认证** | `--authentication-token-webhook-config-file` | 统一在配置文件中管理 |
| **版本状态** | 稳定 (GA) | Beta (1.30+), 逐步替代旧方式 |

#### 完整配置示例

```yaml
# /etc/kubernetes/auth-config.yaml
apiVersion: apiserver.config.k8s.io/v1beta1
kind: AuthenticationConfiguration
jwt:
  # --- 第一个 JWT Provider: 企业 IDP ---
  - issuer:
      url: "https://accounts.enterprise.example.com"
      audiences:
        - "kubernetes"
        - "kubernetes-production"
      certificateAuthority: |
        -----BEGIN CERTIFICATE-----
        MIIDXTCCAkWgAwIBAgIJAJC1HiIAZAiUMA0GCSqGSIb3QaajELMAkGA1UEBhMCU0cx
        ... (企业 CA 证书内容)
        -----END CERTIFICATE-----
      # 或使用文件路径 (1.32+)
      # certificateAuthority: /etc/kubernetes/pki/enterprise-ca.crt
    claimMappings:
      username:
        expression: 'claims.email'
        # 或 prefix: "enterprise:"
      groups:
        expression: 'claims.groups'
        prefix: "enterprise:"
      uid:
        expression: 'claims.sub'
    claimValidationRules:
      - expression: 'claims.iss == "https://accounts.enterprise.example.com"'
        message: "issuer mismatch"
      - expression: 'claims.aud contains "kubernetes"'
        message: "audience mismatch"
    userValidationRules:
      - expression: 'user.name != ""'
        message: "username cannot be empty"
    # 高级选项
    audiences:
      - "kubernetes"
    # 是否跳过 JWT 签名验证 (仅测试环境)
    # skipJWTTokenValidation: false

  # --- 第二个 JWT Provider: 云服务 IDP ---
  - issuer:
      url: "https://cognito-idp.us-west-2.amazonaws.com/us-west-2_xxxxxxxxx"
      audiences:
        - "k8s-cluster-prod"
      certificateAuthority: /etc/kubernetes/pki/aws-cognito-ca.crt
    claimMappings:
      username:
        expression: 'claims.preferred_username'
        prefix: "aws:"
      groups:
        expression: 'claims.cognito:groups'
        prefix: "aws:"
    claimValidationRules:
      - expression: 'claims.token_use == "id"'
        message: "only ID tokens are accepted"

  # --- 第三个 JWT Provider: 内部服务 Token ---
  - issuer:
      url: "https://kubernetes.default.svc.cluster.local"
      audiences:
        - "https://kubernetes.default.svc"
      # 使用 API Server 内置 SA 公钥验证
    claimMappings:
      username:
        expression: 'claims.sub'
        prefix: "serviceaccount:"
      groups:
        expression: 'claims.kubernetes.io/serviceaccount/namespace'
        prefix: "system:serviceaccounts:"

# --- X.509 客户端证书认证 ---
x509:
  # 客户端 CA 证书，用于验证客户端证书
  clientCAData: |
    -----BEGIN CERTIFICATE-----
    MIIDXTCCAkWgAwIBAgIJAJC1HiIAZAiUMA0GCSqGSIb3QaajELMAkGA1UEBhMCU0cx
    ... (集群 CA 证书内容)
    -----END CERTIFICATE-----
  # 或使用文件路径
  # clientCAFile: /etc/kubernetes/pki/ca.crt

# --- Webhook Token 认证 ---
webhook:
  - name: custom-auth-webhook
    connectionInfo:
      type: InClusterConfig
      # 或 Service 配置
      # service:
      #   namespace: kube-system
      #   name: auth-webhook
      #   port: 443
      #   path: /authenticate
      #   caBundle: <base64-encoded-ca>
    cacheTTL: 2m
    # Webhook 超时
    timeout: 10s
    # 失败策略: NoOpinion (默认) / Deny
    failurePolicy: NoOpinion
    # 匹配条件 (可选)
    matchConditions:
      - name: exclude-service-accounts
        expression: 'request.user != "system:serviceaccount"'

# --- 匿名认证 ---
anonymous:
  enabled: true
  # 匿名用户映射
  conditions:
    - name: anonymous-user
      uid: "00000000-0000-0000-0000-000000000000"
      # 匿名用户所属组
      groups:
        - "system:unauthenticated"
      # 额外字段
      extra:
        reason:
          - "anonymous-access"
```

#### API Server 启动参数

```bash
# 启用结构化认证配置 (替代旧参数)
--authentication-config=/etc/kubernetes/auth-config.yaml

# 旧参数对比 (使用结构化配置时，以下参数不应再使用)
# --oidc-issuer-url
# --oidc-client-id
# --oidc-username-claim
# --oidc-groups-claim
# --oidc-ca-file
# --client-ca-file
# --anonymous-auth
# --authentication-token-webhook-config-file
```

#### 关键字段说明

| 字段路径 | 类型 | 必填 | 说明 |
|:---|:---|:---|:---|
| `jwt[].issuer.url` | string | 是 | JWT 签发者 URL，必须与 Token 中的 `iss` claim 完全匹配 |
| `jwt[].issuer.audiences` | []string | 是 | 允许的受众列表，Token 的 `aud` claim 必须包含其中之一 |
| `jwt[].issuer.certificateAuthority` | string | 条件 | issuer 的 CA 证书（PEM 格式内联或文件路径） |
| `jwt[].claimMappings.username` | object | 是 | 用户名映射规则，支持 `expression` 或 `claim` + `prefix` |
| `jwt[].claimMappings.groups` | object | 否 | 用户组映射规则 |
| `jwt[].claimMappings.uid` | object | 否 | 用户 UID 映射规则 |
| `jwt[].claimValidationRules[].expression` | string | 否 | CEL 表达式验证 JWT claim |
| `jwt[].audiences` | []string | 否 | API Server 期望的受众，覆盖 issuer 级别的 audiences |
| `x509.clientCAData` / `clientCAFile` | string | 条件 | 客户端证书 CA（二选一） |
| `webhook[].connectionInfo` | object | 是 | Webhook 连接信息，支持 `InClusterConfig` 或 `Service` |
| `webhook[].cacheTTL` | duration | 否 | 认证结果缓存时间，默认 2m |
| `webhook[].timeout` | duration | 否 | Webhook 调用超时，默认 10s |
| `webhook[].failurePolicy` | string | 否 | Webhook 不可用时的策略：`NoOpinion`(默认) / `Deny` |
| `anonymous.enabled` | bool | 否 | 是否启用匿名认证，默认 false |
| `anonymous.conditions` | []object | 否 | 匿名用户属性映射 |

#### 生产环境最佳实践

**1. 证书轮换管理**

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 1. 准备新 CA 证书
# 更新 auth-config.yaml 中的 certificateAuthority 字段

# 2. 对于 1.32+，配置变更自动生效（无需重启）
# 观察重载状态
kubectl get --raw /metrics | grep apiserver_authentication_config_controller

# 3. 验证新配置生效
kubectl auth can-i get pods --as=user@enterprise.example.com
```
**2. 配置验证**

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 使用 kubectl 验证配置文件格式 (需 1.30+)
kubectl auth reconcile --authentication-config=auth-config.yaml --dry-run=client

# 或直接通过 API Server 验证端点
curl -k https://localhost:6443/apis/apiserver.config.k8s.io/v1beta1/authenticationconfigurations \
  --cacert /etc/kubernetes/pki/ca.crt \
  --cert /etc/kubernetes/pki/admin.crt \
  --key /etc/kubernetes/pki/admin.key
```
**3. 多租户场景建议**

| 场景 | 推荐配置 |
|:---|:---|
| **多企业 IDP** | 为每个 IDP 配置独立的 `jwt` 条目，使用不同的 `username.prefix` 避免冲突 |
| **内部服务账号** | 保留内置 ServiceAccount issuer，确保 Pod 访问 API 不受影响 |
| **外部 CI/CD** | 配置独立的 Webhook 认证，与人工用户区分管理 |
| **只读公共访问** | 启用 `anonymous`，并通过 RBAC 严格限制为 `get`, `list`, `watch` 权限 |

**4. 配置热重载监控 (1.32+)**

```yaml
# Prometheus 告警规则
- alert: AuthenticationConfigReloadFailed
  expr: apiserver_authentication_config_controller_automatic_reload_last_timestamp_seconds{status="error"} > 0
  for: 1m
  labels:
    severity: critical
  annotations:
    summary: "Authentication config reload failed"
    description: "API Server failed to reload authentication configuration"
```

#### 故障排查

| 症状 | 可能原因 | 诊断方法 | 解决方案 |
|:---|:---|:---|:---|
| **配置加载失败** | YAML 语法错误/字段类型不匹配 | `journalctl -u kube-apiserver` 查看启动日志 | 使用 `kubectl` 或 JSON Schema 验证配置 |
| **JWT 验证失败 (401)** | issuer URL 不匹配 / CA 证书错误 / audience 不匹配 | 检查 Token 的 `iss` 和 `aud` claim；验证 CA 证书链 | 确保 `issuer.url` 与 Token `iss` 完全一致（包括尾部斜杠） |
| **用户映射错误** | CEL 表达式错误 / Claim 不存在 | 使用 `jwt.io` 解码 Token 检查 claim 结构 | 检查 `claimMappings.expression` 语法和 claim 路径 |
| **Webhook 认证超时** | 网络不通 / Webhook 服务不可用 | `curl` 直接测试 Webhook 端点 | 检查 Service 和网络策略；增加 `timeout` 值 |
| **匿名访问异常** | 配置中 `anonymous.enabled=false` | 检查配置文件和 `--anonymous-auth` 参数冲突 | 确保未混用旧参数和新配置 |
| **配置热重载未生效 (1.32+)** | 文件系统事件未触发 | 检查 `inotify` 限制；查看 `reload` 相关指标 | 手动触发文件 `touch` 或重启 API Server |

**日志分析关键字段：**

```bash
# 查看认证相关日志
journalctl -u kube-apiserver -g "authentication" --no-pager

# 关键日志模式
# - "Loaded authentication configuration from": 配置加载成功
# - "Failed to load authentication configuration": 配置加载失败，会显示具体错误
# - "invalid JWT signature": JWT 签名验证失败，检查 CA 证书
# - "unable to authenticate the request": 认证失败，后续会显示使用的认证方式
# - "Authentication config reload failed": 热重载失败 (1.32+)
```

---

<!-- chunk: 3. 准入控制插件完整参考 (Admission Controllers Complete Reference) -->
## 3. 准入控制插件完整参考 (Admission Controllers Complete Reference)

> 准入控制是 Kubernetes API Server 在认证和授权之后、对象持久化之前，对请求进行拦截处理的最后防线。通过准入控制器，可以修改（Mutating）或验证（Validating）API 请求，确保集群安全策略、资源配额和配置规范得到强制执行。

### 3.1 准入控制概念概述

#### 准入控制执行阶段

准入控制分为两个核心阶段，所有请求按顺序依次通过：

| 阶段 | 英文名 | 执行顺序 | 能力 | 失败影响 |
|:---|:---|:---|:---|:---|
| **变更阶段** | Mutating Admission | 第 1 阶段 | 修改请求对象（填充默认值、注入Sidecar、添加标签等） | 任一插件拒绝则请求终止 |
| **验证阶段** | Validating Admission | 第 2 阶段 | 只读验证，不修改对象（安全策略检查、配额校验等） | 任一插件拒绝则请求终止 |

```
# 🟢 低风险：只读/信息收集，通常无副作用
┌─────────────┐   ┌─────────────┐   ┌─────────────────────────────┐   ┌──────────────────────────────┐   ┌──────────┐
│   客户端    │──▶│   认证      │──▶│      Mutating Admission      │──▶│    Validating Admission      │──▶│  etcd    │
│  (kubectl)  │   │  (AuthN)    │   │  (变更准入: 修改请求对象)     │   │  (验证准入: 只读校验)         │   │ 持久化   │
└─────────────┘   └─────────────┘   └─────────────────────────────┘   └──────────────────────────────┘   └──────────┘
                                              │                                     │
                                              ▼                                     ▼
                                    ┌─────────────────────┐               ┌─────────────────────┐
                                    │  内置变更插件        │               │  内置验证插件        │
                                    │  MutatingWebhook    │               │  ValidatingWebhook  │
                                    │  LimitRanger        │               │  ResourceQuota      │
                                    │  ServiceAccount     │               │  PodSecurity        │
                                    │  DefaultStorageClass│               │  NodeRestriction    │
                                    └─────────────────────┘               └─────────────────────┘
```
#### 关键设计原则

| 原则 | 说明 |
|:---|:---|
| **顺序确定性** | Mutating 插件按固定顺序串行执行，后执行的插件可以看到前面插件的修改结果 |
| **幂等性要求** | Validating 插件应只读检查，不修改对象；同一对象重复验证应返回相同结果 |
| **失败即终止** | 任一准入插件拒绝请求（返回非 2xx），整个请求立即失败，不会进入后续阶段 |
| **Webhook 超时** | 外部 Webhook 必须在 `timeoutSeconds` 内响应，否则按 `failurePolicy` 处理 |
| **重新调用** | Mutating Webhook 可设置 `reinvocationPolicy: IfNeeded`，在后续 Webhook 修改后再次调用 |

---

### 3.2 默认启用插件列表

Kubernetes 各版本默认启用的内置准入控制器如下（以 1.30+ 为准）：

| 插件名称 | 类型 | 核心作用 | 版本状态 |
|:---|:---|:---|:---|
| **NamespaceLifecycle** | Validating | 阻止在终止中的 Namespace 创建资源，阻止删除系统 Namespace | GA，始终启用 |
| **LimitRanger** | Mutating | 为未设置资源限制的 Pod/Container 应用 LimitRange 默认值 | GA，默认启用 |
| **ServiceAccount** | Mutating | 自动为 Pod 挂载 ServiceAccount Token、挂载 Secret | GA，默认启用（1.24+ 自动管理） |
| **DefaultStorageClass** | Mutating | 为未指定 StorageClass 的 PVC 设置默认值 | GA，默认启用 |
| **DefaultTolerationSeconds** | Mutating | 为未设置 tolerationSeconds 的 Pod 设置默认容忍时间 | GA，默认启用 |
| **MutatingAdmissionWebhook** | Mutating | 调用外部 Mutating Webhook 进行动态变更 | GA，默认启用 |
| **ValidatingAdmissionWebhook** | Validating | 调用外部 Validating Webhook 进行动态验证 | GA，默认启用 |
| **ResourceQuota** | Validating | 检查 Namespace 级别的资源配额是否超限 | GA，默认启用 |
| **PodSecurity** | Validating | 强制执行 Pod Security Standards（替代已移除的 PSP） | GA，1.25+ 默认启用 |
| **NodeRestriction** | Validating | 限制 kubelet 只能修改本节点相关的资源 | GA，默认启用 |
| **Priority** | Validating | 验证 Pod 指定的 PriorityClass 是否存在且合法 | GA，默认启用 |
| **StorageObjectInUseProtection** | Mutating | 为正在使用的 PV/PVC 添加删除保护 finalizer | GA，默认启用 |
| **TaintNodesByCondition** | Mutating | 根据节点条件自动添加/移除 Taint | GA，默认启用 |
| **PersistentVolumeClaimResize** | Mutating | 处理 PVC 扩容请求，验证扩容条件 | GA，默认启用 |

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看当前 API Server 启用的准入插件列表（kube-apiserver 启动参数）
grep "enable-admission-plugins" /etc/kubernetes/manifests/kube-apiserver.yaml

# 或使用 API 查看支持的准入插件版本
kubectl get --raw /apis/admissionregistration.k8s.io/v1/
```
---

### 3.3 安全类准入插件

安全类插件是生产环境最核心的准入防线，负责执行安全策略、限制危险操作和防止权限提升。

| 插件名称 | 类型 | 作用 | 启用建议 | 注意事项 |
|:---|:---|:---|:---|:---|
| **AlwaysPullImages** | Mutating | 强制将所有 Pod 的 `imagePullPolicy` 修改为 `Always` | ⚠️ 按需启用 | 增加镜像拉取延迟和 Registry 负载；适用于多租户环境防止镜像泄露 |
| **CertificateApproval** | Validating | 验证 CertificateSigningRequest 的批准操作是否合法 | ✅ 默认启用 | 仅允许具有 `approve` 权限的用户批准 CSR；防止未授权证书签发 |
| **CertificateSigning** | Validating | 验证 CSR 的签名请求内容格式和主体合法性 | ✅ 默认启用 | 确保证书请求中的 CN/O 等字段符合预期 |
| **ClusterTrustBundleAttestation** | Validating | 验证 ClusterTrustBundle 资源的声明和签名 | ✅ 1.32+ | 防止恶意信任包注入；需配合签名者身份验证 |
| **DenyServiceExternalIPs** | Validating | 阻止创建或更新使用 `externalIPs` 字段的 Service | ✅ 推荐启用 | `externalIPs` 可被用于中间人攻击；除非明确需要负载均衡外部 IP，否则应禁用 |
| **EventRateLimit** | Validating | 限制单个 Namespace 或用户产生 Event 的速率 | ⚠️ 按需启用 | **需要额外配置文件**；防止 Event 风暴导致 etcd 和监控过载 |
| **ImagePolicyWebhook** | Validating | 通过外部 Webhook 审核镜像是否允许部署 | ⚠️ 按需启用 | 需部署独立的镜像扫描/策略服务；延迟敏感场景慎用 |
| **NamespaceLifecycle** | Validating | 阻止在终止中的 Namespace 创建资源；阻止删除 `kube-system` 等系统 NS | ✅ **必须启用** | 几乎所有集群都默认启用；删除系统 Namespace 将导致集群不可用 |
| **NodeRestriction** | Validating | 限制 kubelet 只能修改绑定到本节点的 Pod、本节点对象和本节点租约 | ✅ **必须启用** | 防止节点凭据泄露后横向移动；多租户集群核心安全插件 |
| **PodSecurity** | Validating | 强制执行 Pod Security Standards（Privileged / Baseline / Restricted） | ✅ **必须启用** | 1.25+ 替代 PSP；建议大多数 Namespace 使用 `restricted` 或 `baseline` |
| **PodTolerationRestriction** | Validating | 限制 Pod 可以使用的 Toleration，防止绕过节点污点的 Pod 调度 | ⚠️ 按需启用 | 需配合 Namespace 注解配置允许/禁止的容忍规则；多租户场景有用 |
| **SecurityContextDeny** | Validating | 拒绝设置特定安全上下文（如 privileged、hostPath 等）的 Pod | ❌ **已弃用** | 1.24+ 被 PodSecurity 取代；不建议使用，功能过于简单 |
| **ValidatingAdmissionPolicy** | Validating | 使用内置 CEL 表达式定义验证策略，无需外部 Webhook | ✅ 1.30+ 推荐 | 性能优于 Webhook；适合轻量级策略检查；复杂逻辑仍需 Webhook |

#### EventRateLimit 配置示例

```yaml
# /etc/kubernetes/admission/event-config.yaml
apiVersion: eventratelimit.admission.k8s.io/v1alpha1
kind: Configuration
limits:
- type: Namespace
  qps: 50
  burst: 100
  cacheSize: 2000
- type: User
  qps: 10
  burst: 50
  cacheSize: 1000
- type: SourceAndObject
  qps: 25
  burst: 50
  cacheSize: 5000
```

```bash
# API Server 启动参数
--enable-admission-plugins=EventRateLimit
--admission-control-config-file=/etc/kubernetes/admission/event-config.yaml
```

---

### 3.4 配置类准入插件

配置类插件负责资源默认值填充、配额管理、存储配置和调度相关辅助功能，确保集群资源使用规范化和自动化。

| 插件名称 | 类型 | 作用 | 启用建议 | 注意事项 |
|:---|:---|:---|:---|:---|
| **DefaultIngressClass** | Mutating | 为未指定 IngressClass 的 Ingress 资源设置默认值 | ✅ 默认启用 | 需确保集群已定义默认 IngressClass；多 Ingress Controller 场景需明确指定 |
| **DefaultStorageClass** | Mutating | 为未指定 StorageClass 的 PVC 设置默认 StorageClass | ✅ **必须启用** | 确保有且仅有一个 StorageClass 标记为 `default`; 否则 PVC 创建会失败 |
| **DefaultTolerationSeconds** | Mutating | 为 Pod 的 `node.kubernetes.io/not-ready` 和 `node.kubernetes.io/unreachable` 容忍设置默认的 `tolerationSeconds`（默认 300s） | ✅ 默认启用 | 生产环境建议配合 PodDisruptionBudget 使用，避免默认驱逐时间过长 |
| **ExtendedResourceToleration** | Mutating | 当 Pod 请求扩展资源（如 GPU、FPGA）时，自动为 Pod 添加对应设备插件设置的污点容忍 | ✅ 推荐启用 | 简化 GPU/特殊硬件调度配置；确保设备插件正确设置了污点 |
| **LimitRanger** | Mutating | 为 Namespace 中未设置资源限制的 Pod/Container 应用 LimitRange 默认值和最小/最大限制 | ✅ **必须启用** | 需提前在 Namespace 中创建 LimitRange 对象；否则无限制效果 |
| **MutatingAdmissionWebhook** | Mutating | 调用外部 Mutating Webhook（如 Istio 注入、Secret 注入）进行动态变更 | ✅ **必须启用** | 外部 Webhook 问题可能导致所有 Pod 创建失败；建议设置 `failurePolicy: Ignore` 用于非关键注入 |
| **NamespaceAutoProvision** | Mutating | 当在不存在 Namespace 中创建资源时，自动创建该 Namespace | ❌ **已弃用** | 1.22+ 被移除；建议通过 CI/CD 或 Namespace 管理策略显式创建 Namespace |
| **PersistentVolumeClaimResize** | Mutating | 处理 PVC 扩容请求，验证 StorageClass 是否允许扩容、目标大小是否合法 | ✅ 默认启用 | 需 StorageClass 设置 `allowVolumeExpansion: true`；不支持缩容 |
| **ResourceQuota** | Validating | 检查 Namespace 级别的资源配额（CPU/内存/Pod/Service 等）是否超限 | ✅ **必须启用** | 需提前在 Namespace 中创建 ResourceQuota；注意配额更新非原子性 |
| **StorageObjectInUseProtection** | Mutating | 为正在使用的 PV 和 PVC 添加 `kubernetes.io/pv-protection` 和 `kubernetes.io/pvc-protection` finalizer，阻止误删除 | ✅ 默认启用 | 确保控制器在删除前正确处理 finalizer；强制删除可能导致数据丢失 |
| **TaintNodesByCondition** | Mutating | 根据节点条件（MemoryPressure、DiskPressure、PIDPressure、NetworkUnavailable 等）自动添加/移除对应 Taint | ✅ 默认启用 | 与 DefaultTolerationSeconds 配合工作；自定义调度器需考虑这些 Taint |
| **ValidatingAdmissionWebhook** | Validating | 调用外部 Validating Webhook（如 OPA Gatekeeper、Kyverno）进行策略验证 | ✅ **必须启用** | 生产环境建议设置 `failurePolicy: Fail` 确保安全策略不被绕过；注意超时配置 |

#### LimitRanger 配置示例

```yaml
# 为 Namespace 设置资源限制范围
apiVersion: v1
kind: LimitRange
metadata:
  name: resource-limits
  namespace: production
spec:
  limits:
  - default:
      cpu: "500m"
      memory: "512Mi"
    defaultRequest:
      cpu: "100m"
      memory: "128Mi"
    max:
      cpu: "4"
      memory: "8Gi"
    min:
      cpu: "50m"
      memory: "64Mi"
    type: Container
  - max:
      storage: "100Gi"
    min:
      storage: "1Gi"
    type: PersistentVolumeClaim
```

#### ResourceQuota 配置示例

```yaml
apiVersion: v1
kind: ResourceQuota
metadata:
  name: compute-quota
  namespace: production
spec:
  hard:
    requests.cpu: "20"
    requests.memory: 40Gi
    limits.cpu: "40"
    limits.memory: 80Gi
    pods: "50"
    services: "10"
    persistentvolumeclaims: "20"
    secrets: "50"
    configmaps: "50"
```

---

### 3.5 弃用/移除类准入插件

以下插件已被官方弃用或移除，生产环境不应再依赖这些插件，应迁移到推荐的替代方案。

| 插件名称 | 类型 | 历史作用 | 状态 | 替代方案 | 迁移建议 |
|:---|:---|:---|:---|:---|:---|
| **AlwaysAdmit** | Validating | 允许所有请求通过，无任何检查 | ❌ **1.14 已移除** | 无（不再需要） | 无需替代，直接移除启动参数中的配置 |
| **AlwaysDeny** | Validating | 拒绝所有请求 | ❌ **1.14 已移除** | RBAC + NetworkPolicy | 使用 RBAC 精确控制权限，而非全局拒绝 |
| **BootstrapToken** | Mutating | 处理 Bootstrap Token 相关的 Secret 和 ConfigMap 创建 | ❌ **已弃用** | kubeadm 内置引导流程 | 现代集群使用 kubeadm 或云厂商自动化引导，无需此插件 |
| **PodNodeSelector** | Mutating | 根据 Namespace 注解自动为 Pod 添加 nodeSelector | ❌ **1.22 已移除** | PodPreset / MutatingWebhook | 使用自定义 Mutating Webhook 或 ValidatingAdmissionPolicy 实现相同功能 |
| **PodPreset** | Mutating | 为 Pod 自动注入环境变量、Volume、VolumeMount 等配置 | ❌ **1.20 已移除** | MutatingAdmissionWebhook | 使用 Kyverno、OPA Gatekeeper 或自定义 Webhook 实现配置注入 |
| **PodSecurityPolicy** | Validating + Mutating | 控制 Pod 安全上下文（privileged、hostNetwork、volumes 等） | ❌ **1.25 已移除** | PodSecurity + ValidatingAdmissionWebhook | 迁移至内置 PodSecurity（简单场景）或 OPA/Kyverno（复杂策略） |
| **SecurityContextDeny** | Validating | 拒绝包含特定安全上下文的 Pod | ❌ **1.27 已弃用** | PodSecurity | 直接使用 PodSecurity 的 `restricted` 级别 |
| **ServiceAccount** | Mutating | 自动为 Pod 挂载 ServiceAccount Token 和 ImagePullSecret | ✅ **已内置化** | 无需配置 | 1.24+ 此功能已内置于 API Server，不再作为独立插件暴露，始终启用 |

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 检查是否还在使用已弃用的插件
kubectl logs -n kube-system kube-apiserver-<node> | grep -i "deprecated|removed|unrecognized"

# kubeadm 集群检查启动参数
grep "enable-admission-plugins|disable-admission-plugins" /etc/kubernetes/manifests/kube-apiserver.yaml
```
---

### 3.6 生产环境推荐启用插件组合

不同场景下的准入插件推荐配置：

#### 3.6.1 通用生产集群（推荐基线）

```bash
# 最小安全基线（适用于绝大多数生产集群）
--enable-admission-plugins=NodeRestriction,PodSecurity,DenyServiceExternalIPs
```

此基线配合默认启用的插件（NamespaceLifecycle, LimitRanger, ResourceQuota, ServiceAccount, DefaultStorageClass 等），可满足大多数安全需求。

#### 3.6.2 多租户隔离集群

```bash
# 多租户场景：强化隔离和限速
--enable-admission-plugins=NodeRestriction,PodSecurity,DenyServiceExternalIPs,AlwaysPullImages,EventRateLimit,PodTolerationRestriction
--admission-control-config-file=/etc/kubernetes/admission/multi-tenant-config.yaml
```

| 额外插件 | 作用 |
|:---|:---|
| AlwaysPullImages | 防止镜像缓存泄露（多租户共用节点时尤其重要） |
| EventRateLimit | 防止单个租户 Event 风暴影响整体集群 |
| PodTolerationRestriction | 防止租户通过 toleration 绕过节点隔离 |

#### 3.6.3 金融/高安全合规集群

```bash
# 金融级安全：配合外部策略引擎
--enable-admission-plugins=NodeRestriction,PodSecurity,DenyServiceExternalIPs,ImagePolicyWebhook,ValidatingAdmissionPolicy
```

| 额外插件 | 作用 |
|:---|:---|
| ImagePolicyWebhook | 强制所有镜像通过漏洞扫描和签名验证 |
| ValidatingAdmissionPolicy | 内置 CEL 策略，实现轻量级、高性能的自定义规则验证 |

#### 3.6.4 大规模 GPU/AI 训练集群

```bash
# AI 场景：配合资源调度优化
--enable-admission-plugins=NodeRestriction,PodSecurity,ExtendedResourceToleration,ResourceQuota
```

| 额外插件 | 作用 |
|:---|:---|
| ExtendedResourceToleration | 自动为请求 GPU 的 Pod 添加污点容忍，简化调度配置 |

#### 3.6.5 插件组合速查表

| 场景 | 核心安全插件 | 推荐额外插件 | 禁用建议 |
|:---|:---|:---|:---|
| **标准生产** | NamespaceLifecycle, NodeRestriction, PodSecurity, ResourceQuota, LimitRanger | DenyServiceExternalIPs | SecurityContextDeny |
| **多租户 SaaS** | 标准生产 + | AlwaysPullImages, EventRateLimit, PodTolerationRestriction | 无 |
| **金融合规** | 标准生产 + | ImagePolicyWebhook, ValidatingAdmissionPolicy | 无 |
| **AI/GPU 集群** | 标准生产 + | ExtendedResourceToleration | 无 |
| **边缘/IoT** | 标准生产 | PersistentVolumeClaimResize | EventRateLimit（如 Event 量小） |
| **开发测试** | NamespaceLifecycle, LimitRanger | 无 | DenyServiceExternalIPs（如需调试） |

---

### 3.7 自定义准入插件顺序和禁用策略

#### 3.7.1 启动参数配置

```bash
# kube-apiserver 启动参数
--enable-admission-plugins=NodeRestriction,PodSecurity,DenyServiceExternalIPs,EventRateLimit
--disable-admission-plugins=SecurityContextDeny  # 显式禁用已弃用插件
--admission-control-config-file=/etc/kubernetes/admission/admission-configuration.yaml
```

**重要规则：**

| 规则 | 说明 |
|:---|:---|
| 顺序不可自定义 | 内置准入插件的 Mutating 和 Validating 顺序由 API Server 硬编码，启动参数仅控制启用/禁用 |
| Webhook 顺序 | MutatingWebhook 和 ValidatingWebhook 内部的多个 Webhook 按 `name` 字典序执行 |
| 默认即启用 | 未在 `--enable-admission-plugins` 中显式列出但属于默认启用的插件，仍然会自动启用 |
| 显式禁用优先 | `--disable-admission-plugins` 优先级高于 `--enable-admission-plugins` |

#### 3.7.2 插件配置文件

```yaml
# /etc/kubernetes/admission/admission-configuration.yaml
apiVersion: apiserver.config.k8s.io/v1
kind: AdmissionConfiguration
plugins:
- name: EventRateLimit
  path: /etc/kubernetes/admission/eventratelimit.yaml
- name: PodSecurity
  configuration:
    apiVersion: pod-security.admission.config.k8s.io/v1
    kind: PodSecurityConfiguration
    defaults:
      enforce: "restricted"
      audit: "restricted"
      warn: "restricted"
    exemptions:
      usernames: []
      runtimeClasses: ["gvisor"]
      namespaces: ["kube-system", "istio-system", "monitoring"]
```

---

### 3.8 准入插件性能影响分析

#### 3.8.1 性能基准

| 插件类型 | 典型延迟 | 并发影响 | 主要开销来源 |
|:---|:---|:---|:---|
| **内置 Mutating 插件** | 1-5ms | 低 | 内存操作、对象序列化 |
| **内置 Validating 插件** | 1-3ms | 低 | 内存中的规则匹配 |
| **Mutating Webhook** | 10-500ms | 中-高 | 网络 RTT + Webhook 处理时间 |
| **Validating Webhook** | 10-500ms | 中-高 | 网络 RTT + Webhook 处理时间 |
| **ValidatingAdmissionPolicy** | 1-10ms | 低 | CEL 表达式求值 |

#### 3.8.2 性能优化建议

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 监控准入控制延迟
kubectl get --raw /metrics | grep apiserver_admission_controller_admission_duration_seconds
kubectl get --raw /metrics | grep apiserver_admission_webhook_admission_duration_seconds
```
| 优化策略 | 具体操作 |
|:---|:---|
| **缩短 Webhook 超时** | 非关键 Webhook 设置 `timeoutSeconds: 5`；关键 Webhook 不超过 `10s` |
| **就近部署 Webhook** | Webhook 服务应与 API Server 同可用区部署，减少网络延迟 |
| **启用 Webhook 缓存** | 对幂等性验证结果实施缓存，减少重复计算 |
| **使用 ValidatingAdmissionPolicy 替代轻量级 Webhook** | CEL 策略在 API Server 内部执行，无网络开销 |
| **限制 Webhook 作用范围** | 使用 `namespaceSelector` 和 `objectSelector` 缩小 Webhook 拦截范围 |
| **水平扩展 Webhook** | Webhook 服务应部署多个副本，配合 HPA 应对高峰流量 |

#### 3.8.3 Prometheus 监控指标

```yaml
# 准入控制延迟告警
- alert: AdmissionControllerLatencyHigh
  expr: histogram_quantile(0.99, rate(apiserver_admission_controller_admission_duration_seconds_bucket[5m])) > 0.05
  for: 5m
  labels:
    severity: warning
  annotations:
    summary: "内置准入控制器延迟过高"
    description: "99分位延迟: {{ $value }}s"

- alert: AdmissionWebhookLatencyHigh
  expr: histogram_quantile(0.99, rate(apiserver_admission_webhook_admission_duration_seconds_bucket[5m])) > 0.5
  for: 5m
  labels:
    severity: critical
  annotations:
    summary: "准入 Webhook 延迟过高"
    description: "Webhook 99分位延迟: {{ $value }}s"

- alert: AdmissionWebhookRejectionRateHigh
  expr: sum(rate(apiserver_admission_webhook_rejection_count[5m])) / sum(rate(apiserver_admission_webhook_admission_duration_seconds_count[5m])) > 0.1
  for: 5m
  labels:
    severity: warning
  annotations:
    summary: "准入 Webhook 拒绝率过高"
    description: "Webhook 拒绝率超过 10%"
```

---

### 3.9 故障排查：插件导致的请求被拒绝

#### 3.9.1 诊断流程

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl label/annotate`：改元数据可能影响选择器/控制器

```
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
请求被拒绝 (403/500)
    │
    ├── 检查 API Server 日志 ───────────────────────────┐
    │   journalctl -u kube-apiserver -g "admission"      │
    │   # 查找 "admission webhook denied" 或插件名      │
    │                                                    │
    ├── 检查 Webhook 状态 ──────────────────────────────┤
    │   kubectl get validatingwebhookconfigurations      │
    │   kubectl get mutatingwebhookconfigurations        │
    │   # 查看 WEBHOOK 是否 Ready、CA 证书是否过期      │
    │                                                    │
    ├── 检查 PodSecurity 冲突 ──────────────────────────┤
    │   kubectl label ns <ns> pod-security.kubernetes.io/enforce=restricted --dry-run=server
    │   # 查看具体违反了哪些安全标准                    │
    │                                                    │
    ├── 检查 ResourceQuota ─────────────────────────────┤
    │   kubectl describe resourcequota -n <ns>           │
    │   # 查看当前用量和限制                            │
    │                                                    │
    └── 测试请求详情 ───────────────────────────────────┘
        kubectl -v=8 <command>  # 查看完整请求/响应

```
#### 3.9.2 常见问题场景

| 症状 | 可能原因 | 诊断命令 | 解决方案 |
|:---|:---|:---|:---|
| **Pod 创建失败：admission webhook denied** | Validating Webhook 策略拒绝 | `kubectl -v=8 create -f pod.yaml` 查看响应体 | 检查 Webhook 的拒绝原因；临时设置 `failurePolicy: Ignore` 恢复 |
| **Pod 创建失败：PodSecurity restricted** | Pod 违反 PodSecurity 标准 | `kubectl label ns default pod-security.kubernetes.io/enforce=restricted` | 修复 Pod 的安全上下文（runAsNonRoot, seccompProfile 等）或放宽 Namespace 策略 |
| **Pod 创建失败：exceeded quota** | ResourceQuota 已满 | `kubectl describe resourcequota -n <ns>` | 清理无用资源或申请增加配额 |
| **Pod 创建失败：No API token found** | ServiceAccount 插件异常 | `kubectl get sa default -n <ns>` | 确保 ServiceAccount 存在且 `automountServiceAccountToken: true` |
| **PVC 创建失败：no storage class is set** | DefaultStorageClass 未生效 | `kubectl get sc` | 确保有一个 StorageClass 标记为 `storageclass.kubernetes.io/is-default-class: true` |
| **API Server 启动失败：unrecognized admission controller** | 配置了已移除的插件 | `journalctl -u kube-apiserver` | 从 `--enable-admission-plugins` 中移除废弃插件名称 |
| **所有请求超时** | Webhook 服务不可达 | `kubectl get pods -n <webhook-ns>`, `kubectl logs <webhook-pod>` | 检查 Webhook Pod 状态和网络连通性；必要时禁用问题 Webhook |
| **Namespace 删除卡住** | NamespaceLifecycle 或 finalizer 问题 | `kubectl get ns <ns> -o yaml` 查看 finalizers | 手动清理 finalizer（谨慎操作）；检查是否有正在运行的 Pod |

#### 3.9.3 日志关键字段速查

```bash
# 实时跟踪准入控制相关日志
journalctl -u kube-apiserver -f | grep -E "admission|webhook|denied|rejected"

# 关键日志模式
# "admission webhook \"<name>\" denied the request: <reason>"  → Webhook 拒绝
# "PodSecurity <level>: <violation>"                            → PodSecurity 违规
# "Forbidden: exceeded quota"                                    → 资源配额超限
# "namespace \"<ns>\" is terminating"                            → NamespaceLifecycle 拦截
# "unrecognized admission controller: <name>"                    → 插件名称错误
```

#### 3.9.4 紧急恢复操作

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl delete`：删除资源（可由声明式清单重建）
> - `kubectl edit/patch`：修改运行中的资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 场景：某个 Mutating Webhook 问题导致所有 Pod 无法创建

# 1. 查看当前 Webhook 配置
kubectl get mutatingwebhookconfigurations

# 2. 备份后删除问题 Webhook（谨慎！）
kubectl get mutatingwebhookconfigurations <faulty-webhook> -o yaml > /tmp/webhook-backup.yaml
kubectl delete mutatingwebhookconfigurations <faulty-webhook>

# 3. 或将 failurePolicy 改为 Ignore（仅用于恢复）
kubectl patch mutatingwebhookconfigurations <faulty-webhook> \
  --type='json' \
  -p='[{"op": "replace", "path": "/webhooks/0/failurePolicy", "value": "Ignore"}]'

# 4. 如果 API Server 因配置错误无法启动，直接编辑静态 Pod 清单
vim /etc/kubernetes/manifests/kube-apiserver.yaml
# 修改 --enable-admission-plugins 参数，移除问题插件
# kubelet 会自动重启 API Server
```
---

**准入控制插件总结**: 准入控制是 Kubernetes 安全架构的最后一道关卡。生产环境应始终启用 `NamespaceLifecycle`, `NodeRestriction`, `PodSecurity`, `ResourceQuota`, `LimitRanger` 等核心插件，并根据场景补充 `DenyServiceExternalIPs`, `EventRateLimit`, `ValidatingAdmissionPolicy` 等高级插件。定期检查废弃插件列表，及时迁移到官方推荐的替代方案。

<!-- chunk: 4. 授权机制 (Authorization) -->
## 4. 授权机制 (Authorization)

### 4.1 授权模式对比

| 模式 | 英文名 | 说明 | 适用场景 |
|:---|:---|:---|:---|
| **AlwaysAllow** | 始终允许 | 跳过授权检查 | 仅开发/测试 |
| **AlwaysDeny** | 始终拒绝 | 拒绝所有请求 | 维护模式 |
| **ABAC** | Attribute-Based | 基于属性的访问控制 | 已弃用 |
| **RBAC** | Role-Based | 基于角色的访问控制 | 生产环境标准 |
| **Node** | 节点授权 | kubelet专用授权 | 节点访问控制 |
| **Webhook** | Webhook授权 | 外部授权服务 | 自定义授权逻辑 |

```bash
# API Server 授权配置
--authorization-mode=Node,RBAC  # 推荐配置
```

### 4.2 RBAC 核心资源

| 资源类型 | 作用域 | 说明 | 示例 |
|:---|:---|:---|:---|
| **Role** | Namespace | 命名空间级别角色 | 开发者角色 |
| **ClusterRole** | Cluster | 集群级别角色 | 管理员角色、聚合角色 |
| **RoleBinding** | Namespace | 绑定Role/ClusterRole到主体 | 绑定用户到角色 |
| **ClusterRoleBinding** | Cluster | 集群级别绑定 | 集群管理员绑定 |

```yaml
# ClusterRole 示例: 只读访问所有资源
apiVersion: rbac.authorization.k8s.io/v1
kind: ClusterRole
metadata:
  name: cluster-reader
rules:
- apiGroups: ["*"]
  resources: ["*"]
  verbs: ["get", "list", "watch"]
- nonResourceURLs: ["/healthz", "/metrics"]
  verbs: ["get"]

---
# Role 示例: 特定Namespace的Deployment管理
apiVersion: rbac.authorization.k8s.io/v1
kind: Role
metadata:
  namespace: production
  name: deployment-manager
rules:
- apiGroups: ["apps"]
  resources: ["deployments", "replicasets"]
  verbs: ["get", "list", "watch", "create", "update", "patch", "delete"]
- apiGroups: [""]
  resources: ["pods", "pods/log"]
  verbs: ["get", "list", "watch"]
- apiGroups: [""]
  resources: ["configmaps", "secrets"]
  verbs: ["get", "list"]

---
# RoleBinding 示例
apiVersion: rbac.authorization.k8s.io/v1
kind: RoleBinding
metadata:
  name: dev-deployment-manager
  namespace: production
subjects:
- kind: User
  name: developer@example.com
  apiGroup: rbac.authorization.k8s.io
- kind: Group
  name: developers
  apiGroup: rbac.authorization.k8s.io
- kind: ServiceAccount
  name: ci-cd-sa
  namespace: ci-cd
roleRef:
  kind: Role
  name: deployment-manager
  apiGroup: rbac.authorization.k8s.io
```

### 4.3 内置 ClusterRole

| ClusterRole | 说明 | 权限范围 |
|:---|:---|:---|
| `cluster-admin` | 超级管理员 | 所有资源的所有操作 |
| `admin` | 管理员 | 命名空间内大部分资源的管理权限 |
| `edit` | 编辑者 | 读写大部分资源，不含RBAC |
| `view` | 查看者 | 只读访问大部分资源 |
| `system:node` | 节点角色 | kubelet所需权限 |
| `system:kube-scheduler` | 调度器角色 | 调度器所需权限 |
| `system:kube-controller-manager` | 控制器角色 | KCM所需权限 |

---

<!-- chunk: 5. 准入控制 (Admission Control) -->
## 5. 准入控制 (Admission Control)

### 5.1 准入控制器类型

| 类型 | 英文名 | 执行时机 | 功能 |
|:---|:---|:---|:---|
| **变更准入** | Mutating Admission | 授权后、验证前 | 修改请求对象 |
| **验证准入** | Validating Admission | 变更准入后 | 验证请求合法性 |

### 5.2 内置准入控制器

| 控制器名称 | 类型 | 功能 | 默认启用 |
|:---|:---|:---|:---|
| **NamespaceLifecycle** | Validating | 阻止在终止中的NS创建资源 | Yes |
| **LimitRanger** | Mutating | 应用默认资源限制 | Yes |
| **ServiceAccount** | Mutating | 自动挂载SA Token | Yes |
| **DefaultStorageClass** | Mutating | 设置默认StorageClass | Yes |
| **DefaultTolerationSeconds** | Mutating | 设置默认容忍时间 | Yes |
| **MutatingAdmissionWebhook** | Mutating | 调用外部Webhook | Yes |
| **ValidatingAdmissionWebhook** | Validating | 调用外部Webhook | Yes |
| **ResourceQuota** | Validating | 检查资源配额 | Yes |
| **PodSecurity** | Validating | Pod安全标准 (替代PSP) | Yes (1.25+) |
| **NodeRestriction** | Validating | 限制kubelet修改范围 | Yes |
| **PriorityClass** | Validating | 验证PriorityClass | Yes |

```bash
# API Server 准入控制配置
--enable-admission-plugins=NodeRestriction,PodSecurity
--disable-admission-plugins=PodSecurityPolicy  # PSP已弃用
```

### 5.3 动态准入 Webhook

```yaml
# MutatingWebhookConfiguration 示例
apiVersion: admissionregistration.k8s.io/v1
kind: MutatingWebhookConfiguration
metadata:
  name: pod-injector
webhooks:
- name: pod-injector.example.com
  admissionReviewVersions: ["v1", "v1beta1"]
  sideEffects: None
  timeoutSeconds: 5
  failurePolicy: Fail  # Fail/Ignore
  matchPolicy: Equivalent
  reinvocationPolicy: IfNeeded
  clientConfig:
    service:
      name: pod-injector
      namespace: kube-system
      path: "/mutate"
      port: 443
    caBundle: <base64-encoded-ca-cert>
  rules:
  - operations: ["CREATE"]
    apiGroups: [""]
    apiVersions: ["v1"]
    resources: ["pods"]
    scope: "Namespaced"
  namespaceSelector:
    matchLabels:
      injection: enabled
  objectSelector:
    matchLabels:
      inject: "true"

---
# ValidatingWebhookConfiguration 示例
apiVersion: admissionregistration.k8s.io/v1
kind: ValidatingWebhookConfiguration
metadata:
  name: pod-policy
webhooks:
- name: pod-policy.example.com
  admissionReviewVersions: ["v1"]
  sideEffects: None
  timeoutSeconds: 10
  failurePolicy: Fail
  clientConfig:
    service:
      name: pod-policy
      namespace: kube-system
      path: "/validate"
      port: 443
    caBundle: <base64-encoded-ca-cert>
  rules:
  - operations: ["CREATE", "UPDATE"]
    apiGroups: [""]
    apiVersions: ["v1"]
    resources: ["pods"]
```

---

<!-- chunk: 6. API Priority and Fairness (APF) -->
## 6. API Priority and Fairness (APF)

### 6.1 APF 核心概念

| 概念 | 英文名 | 说明 |
|:---|:---|:---|
| **优先级级别** | PriorityLevel | 定义请求队列和并发限制 |
| **流Schema** | FlowSchema | 将请求分类到PriorityLevel |
| **队列** | Queue | 请求等待队列 |
| **座位** | Seat | 并发执行槽位 |

### 6.2 内置 PriorityLevel

| 名称 | 类型 | 并发份额 | 说明 |
|:---|:---|:---|:---|
| `system` | Exempt | - | 系统关键请求，不排队 |
| `leader-election` | Limited | 10 | Leader选举相关 |
| `node-high` | Limited | 40 | 节点高优先级请求 |
| `workload-high` | Limited | 40 | 工作负载高优先级 |
| `workload-low` | Limited | 100 | 普通工作负载请求 |
| `global-default` | Limited | 20 | 默认级别 |
| `exempt` | Exempt | - | 豁免流量控制 |
| `catch-all` | Limited | 5 | 兜底级别 |

### 6.3 APF 配置示例

```yaml
# 自定义 PriorityLevel
apiVersion: flowcontrol.apiserver.k8s.io/v1beta3
kind: PriorityLevelConfiguration
metadata:
  name: custom-high-priority
spec:
  type: Limited
  limited:
    nominalConcurrencyShares: 50
    limitResponse:
      type: Queue
      queuing:
        queues: 64
        handSize: 6
        queueLengthLimit: 50

---
# 自定义 FlowSchema
apiVersion: flowcontrol.apiserver.k8s.io/v1beta3
kind: FlowSchema
metadata:
  name: critical-service-requests
spec:
  priorityLevelConfiguration:
    name: custom-high-priority
  matchingPrecedence: 100
  distinguisherMethod:
    type: ByUser
  rules:
  - subjects:
    - kind: ServiceAccount
      serviceAccount:
        name: critical-app
        namespace: production
    resourceRules:
    - verbs: ["*"]
      apiGroups: ["*"]
      resources: ["*"]
      namespaces: ["production"]
```

---

<!-- chunk: 7. 关键配置参数 (Configuration Parameters) -->
## 7. 关键配置参数 (Configuration Parameters)

### 7.1 核心参数

| 参数 | 默认值 | 推荐值 | 说明 |
|:---|:---|:---|:---|
| `--bind-address` | 0.0.0.0 | 0.0.0.0 | 监听地址 |
| `--secure-port` | 6443 | 6443 | HTTPS端口 |
| `--advertise-address` | 自动检测 | 节点IP | 广播地址 |
| `--etcd-servers` | - | etcd集群地址 | etcd连接地址 |
| `--etcd-cafile` | - | /etc/kubernetes/pki/etcd/ca.crt | etcd CA |
| `--etcd-certfile` | - | /etc/kubernetes/pki/apiserver-etcd-client.crt | etcd客户端证书 |
| `--etcd-keyfile` | - | /etc/kubernetes/pki/apiserver-etcd-client.key | etcd客户端私钥 |
| `--service-cluster-ip-range` | 10.0.0.0/24 | 10.96.0.0/12 | Service IP范围 |
| `--service-node-port-range` | 30000-32767 | 30000-32767 | NodePort范围 |

### 7.2 性能调优参数

| 参数 | 默认值 | 推荐值(大集群) | 说明 |
|:---|:---|:---|:---|
| `--max-requests-inflight` | 400 | 800-1600 | 非变更请求最大并发 |
| `--max-mutating-requests-inflight` | 200 | 400-800 | 变更请求最大并发 |
| `--target-ram-mb` | - | 根据集群规模设置 | 目标内存(用于缓存) |
| `--watch-cache-sizes` | - | 根据资源调整 | Watch缓存大小 |
| `--default-watch-cache-size` | 100 | 100-1000 | 默认Watch缓存 |
| `--etcd-count-metric-poll-period` | 1m | 1m | etcd计数指标轮询周期 |
| `--request-timeout` | 60s | 60s | 请求超时时间 |
| `--min-request-timeout` | 1800s | 1800s | 最小请求超时(用于Watch) |

### 7.3 安全参数

| 参数 | 说明 | 推荐配置 |
|:---|:---|:---|
| `--anonymous-auth` | 匿名访问 | false |
| `--enable-admission-plugins` | 启用的准入控制器 | NodeRestriction,PodSecurity |
| `--audit-log-path` | 审计日志路径 | /var/log/kubernetes/audit.log |
| `--audit-policy-file` | 审计策略文件 | /etc/kubernetes/audit-policy.yaml |
| `--audit-log-maxage` | 审计日志保留天数 | 30 |
| `--audit-log-maxbackup` | 审计日志备份数 | 10 |
| `--audit-log-maxsize` | 审计日志最大大小(MB) | 100 |
| `--profiling` | 性能分析端点 | false (生产环境) |
| `--enable-swagger-ui` | Swagger UI | false |

---

<!-- chunk: 8. 审计日志 (Audit Logging) -->
## 8. 审计日志 (Audit Logging)

### 8.1 审计级别

| 级别 | 英文 | 记录内容 |
|:---|:---|:---|
| **None** | 无 | 不记录 |
| **Metadata** | 元数据 | 请求元数据(用户、时间、资源、动作) |
| **Request** | 请求 | 元数据 + 请求体 |
| **RequestResponse** | 请求响应 | 元数据 + 请求体 + 响应体 |

### 8.2 审计策略示例

```yaml
# /etc/kubernetes/audit-policy.yaml
apiVersion: audit.k8s.io/v1
kind: Policy
rules:
# 不记录的请求
- level: None
  users: ["system:kube-proxy"]
  verbs: ["watch"]
  resources:
  - group: ""
    resources: ["endpoints", "services", "services/status"]

# 不记录健康检查
- level: None
  nonResourceURLs:
  - /healthz*
  - /version
  - /swagger*
  - /readyz*
  - /livez*

# 不记录高频只读操作
- level: None
  resources:
  - group: ""
    resources: ["events"]

# Secrets 只记录元数据(不记录内容)
- level: Metadata
  resources:
  - group: ""
    resources: ["secrets", "configmaps"]

# 记录所有删除操作的请求和响应
- level: RequestResponse
  verbs: ["delete", "deletecollection"]

# 记录写操作的请求体
- level: Request
  verbs: ["create", "update", "patch"]
  resources:
  - group: ""
    resources: ["pods", "services", "deployments"]

# 默认记录元数据
- level: Metadata
  omitStages:
  - "RequestReceived"
```

### 8.3 审计后端配置

```bash
# 日志文件后端
--audit-log-path=/var/log/kubernetes/audit.log
--audit-log-maxage=30
--audit-log-maxbackup=10
--audit-log-maxsize=100

# Webhook 后端
--audit-webhook-config-file=/etc/kubernetes/audit-webhook-config.yaml
--audit-webhook-initial-backoff=10s
--audit-webhook-batch-max-size=400
--audit-webhook-batch-max-wait=30s
```

```yaml
# audit-webhook-config.yaml
apiVersion: v1
kind: Config
clusters:
- name: audit-webhook
  cluster:
    server: https://audit-service.kube-system.svc:443/audit
    certificate-authority: /etc/kubernetes/pki/audit-ca.crt
contexts:
- name: default
  context:
    cluster: audit-webhook
current-context: default
```

---

<!-- chunk: 9. 监控指标 (Monitoring Metrics) -->
## 9. 监控指标 (Monitoring Metrics)

### 9.1 关键指标表

| 指标名称 | 类型 | 说明 | 告警阈值 |
|:---|:---|:---|:---|
| `apiserver_request_total` | Counter | 请求总数(按verb、resource、code) | - |
| `apiserver_request_duration_seconds` | Histogram | 请求延迟 | p99 > 1s |
| `apiserver_current_inflight_requests` | Gauge | 当前并发请求数 | > max * 0.8 |
| `apiserver_response_sizes` | Histogram | 响应大小分布 | - |
| `apiserver_admission_controller_admission_duration_seconds` | Histogram | 准入控制延迟 | p99 > 100ms |
| `apiserver_admission_webhook_admission_duration_seconds` | Histogram | Webhook延迟 | p99 > 500ms |
| `etcd_request_duration_seconds` | Histogram | etcd请求延迟 | p99 > 200ms |
| `apiserver_storage_objects` | Gauge | 存储对象数 | - |
| `apiserver_watch_events_total` | Counter | Watch事件数 | - |
| `apiserver_longrunning_requests` | Gauge | 长连接请求数(Watch) | - |
| `process_resident_memory_bytes` | Gauge | 内存使用 | > 16GB |
| `process_cpu_seconds_total` | Counter | CPU使用 | - |

### 9.2 Prometheus 告警规则

```yaml
groups:
- name: kube-apiserver
  rules:
  - alert: KubeAPIServerDown
    expr: absent(up{job="kube-apiserver"} == 1)
    for: 5m
    labels:
      severity: critical
    annotations:
      summary: "kube-apiserver is down"

  - alert: KubeAPIServerLatencyHigh
    expr: histogram_quantile(0.99, rate(apiserver_request_duration_seconds_bucket{verb!="WATCH"}[5m])) > 1
    for: 10m
    labels:
      severity: warning
    annotations:
      summary: "kube-apiserver latency is high"
      description: "API server p99 latency is {{ $value }}s"

  - alert: KubeAPIServerErrorsHigh
    expr: sum(rate(apiserver_request_total{code=~"5.."}[5m])) / sum(rate(apiserver_request_total[5m])) > 0.01
    for: 5m
    labels:
      severity: warning
    annotations:
      summary: "kube-apiserver error rate is high"

  - alert: KubeAPIServerSaturated
    expr: apiserver_current_inflight_requests / apiserver_current_inflight_requests_limit > 0.8
    for: 5m
    labels:
      severity: warning
    annotations:
      summary: "kube-apiserver is saturated"

  - alert: KubeAPIServerAdmissionWebhookLatency
    expr: histogram_quantile(0.99, rate(apiserver_admission_webhook_admission_duration_seconds_bucket[5m])) > 0.5
    for: 10m
    labels:
      severity: warning
    annotations:
      summary: "Admission webhook latency is high"

  - alert: KubeAPIServerEtcdLatencyHigh
    expr: histogram_quantile(0.99, rate(etcd_request_duration_seconds_bucket[5m])) > 0.2
    for: 10m
    labels:
      severity: warning
    annotations:
      summary: "etcd request latency from apiserver is high"
```

---

<!-- chunk: 10. 高可用部署 (High Availability) -->
## 10. 高可用部署 (High Availability)

### 10.1 HA 架构模式

| 模式 | 说明 | 适用场景 |
|:---|:---|:---|
| **堆叠模式** | etcd与控制平面部署在同一节点 | 中小规模集群 |
| **外部etcd模式** | etcd独立部署 | 大规模集群、高可用要求高 |

### 10.2 负载均衡配置

```yaml
# HAProxy 配置示例
frontend kube-apiserver
    bind *:6443
    mode tcp
    option tcplog
    default_backend kube-apiserver

backend kube-apiserver
    mode tcp
    option tcp-check
    balance roundrobin
    default-server inter 10s downinter 5s rise 2 fall 2 slowstart 60s maxconn 250 maxqueue 256 weight 100
    server master1 10.0.0.1:6443 check
    server master2 10.0.0.2:6443 check
    server master3 10.0.0.3:6443 check
```

```yaml
# Nginx 配置示例
stream {
    upstream kube-apiserver {
        least_conn;
        server 10.0.0.1:6443 max_fails=3 fail_timeout=30s;
        server 10.0.0.2:6443 max_fails=3 fail_timeout=30s;
        server 10.0.0.3:6443 max_fails=3 fail_timeout=30s;
    }
    
    server {
        listen 6443;
        proxy_pass kube-apiserver;
        proxy_timeout 10m;
        proxy_connect_timeout 1s;
    }
}
```

### 10.3 健康检查端点

| 端点 | 用途 | 检查内容 |
|:---|:---|:---|
| `/healthz` | 整体健康检查 | 所有健康检查的聚合结果 |
| `/livez` | 存活检查 | 进程是否正常运行 |
| `/readyz` | 就绪检查 | 是否可以接收请求 |
| `/healthz/etcd` | etcd连接检查 | etcd是否可访问 |
| `/healthz/poststarthook/*` | 启动钩子检查 | 各启动钩子状态 |

```bash
# 健康检查命令
curl -k https://localhost:6443/healthz
curl -k https://localhost:6443/livez
curl -k https://localhost:6443/readyz
curl -k https://localhost:6443/healthz?verbose
```

---

<!-- chunk: 11. 故障排查 (Troubleshooting) -->
## 11. 故障排查 (Troubleshooting)

### 11.1 常见问题诊断

| 症状 | 可能原因 | 诊断方法 | 解决方案 |
|:---|:---|:---|:---|
| **连接超时** | 网络问题/服务未启动 | telnet检查端口/systemctl status | 检查网络配置/启动服务 |
| **认证失败 (401)** | 证书错误/Token无效 | 检查证书有效期和配置 | 更新证书/Token |
| **授权失败 (403)** | RBAC配置不足 | kubectl auth can-i | 添加适当的RBAC权限 |
| **etcd连接失败** | etcd不可用/证书问题 | etcdctl endpoint health | 检查etcd集群状态 |
| **请求超时** | 负载过高/etcd慢 | 检查指标和日志 | 扩容/优化性能 |
| **OOM** | 内存不足 | dmesg/检查内存使用 | 增加内存/优化配置 |
| **证书过期** | 证书未轮换 | openssl检查有效期 | kubeadm certs renew |

### 11.2 诊断命令

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 检查 API Server 状态
systemctl status kube-apiserver
journalctl -u kube-apiserver -f --no-pager

# 检查证书有效期
kubeadm certs check-expiration
openssl x509 -in /etc/kubernetes/pki/apiserver.crt -noout -dates

# 检查 API 可访问性
kubectl get --raw /healthz
kubectl get --raw /livez
kubectl get --raw /readyz

# 检查 RBAC 权限
kubectl auth can-i create pods --as=system:serviceaccount:default:default
kubectl auth can-i --list --as=developer@example.com

# 检查准入控制器
kubectl get validatingwebhookconfigurations
kubectl get mutatingwebhookconfigurations

# 查看请求详情
kubectl -v=9 get pods  # 详细输出

# 检查 API 资源
kubectl api-resources
kubectl api-versions
```
### 11.3 证书轮换

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `systemctl stop/restart`：停止/重启系统服务，影响节点上所有容器

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# kubeadm 管理的集群
kubeadm certs renew all

# 手动轮换 (非kubeadm)
# 1. 生成新证书
cfssl gencert ... | cfssljson -bare apiserver

# 2. 备份旧证书
cp /etc/kubernetes/pki/apiserver.{crt,key} /etc/kubernetes/pki/backup/

# 3. 替换证书
cp apiserver.pem /etc/kubernetes/pki/apiserver.crt
cp apiserver-key.pem /etc/kubernetes/pki/apiserver.key

# 4. 重启 API Server
systemctl restart kube-apiserver
```
---

<!-- chunk: 12. 生产环境 Checklist -->
## 12. 生产环境 Checklist

### 12.1 部署检查

| 检查项 | 状态 | 说明 |
|:---|:---|:---|
| [ ] 多实例部署 (3+) | | 高可用保证 |
| [ ] 负载均衡配置 | | 流量分发 |
| [ ] TLS配置完整 | | 通信加密 |
| [ ] 证书有效期充足 | | 避免过期中断 |
| [ ] 审计日志启用 | | 合规要求 |
| [ ] RBAC配置完善 | | 最小权限原则 |
| [ ] 监控告警配置 | | 运维保障 |
| [ ] 资源限制配置 | | 防止资源耗尽 |
| [ ] 网络策略配置 | | 网络安全 |
| [ ] 定期备份etcd | | 数据保护 |

### 12.2 安全加固

| 加固项 | 推荐配置 |
|:---|:---|
| 匿名访问 | --anonymous-auth=false |
| 不安全端口 | --insecure-port=0 (已在1.24+移除) |
| 性能分析 | --profiling=false |
| AlwaysAllow授权 | 不使用，使用RBAC |
| 审计日志 | 启用并配置合理的保留策略 |
| 准入控制 | 启用NodeRestriction,PodSecurity |
| 加密存储 | --encryption-provider-config |

---

<!-- chunk: 附录: 常用 API 端点 -->
## 附录: 常用 API 端点

```bash
# 核心 API
/api/v1/namespaces
/api/v1/pods
/api/v1/services
/api/v1/nodes

# 扩展 API
/apis/apps/v1/deployments
/apis/batch/v1/jobs
/apis/networking.k8s.io/v1/ingresses

# 集群信息
/version                    # 版本信息
/api                        # 核心API组
/apis                       # 所有API组
/openapi/v2                 # OpenAPI规范

# 健康检查
/healthz
/livez
/readyz

# 指标
/metrics
/metrics/cadvisor

# 调试 (需要启用profiling)
/debug/pprof/
```

---

**表格底部标记**: Kusheet Project, 作者 Allen Galler (allengaller@gmail.com)

---

<!-- chunk: Obsidian 相关文档 -->
## Obsidian 相关文档

- 集群基础 MOC
- [[集群基础/README.md|Domain-3: Kubernetes控制平面]]
- Domain-3 控制平面 — 开源项目索引
- Kubernetes 控制平面架构总览 (Control Plane Architecture Overview)
- 控制平面组件交互详解 (Control Plane Components Interaction Deep Dive)
- 控制平面高可用部署模式 (Control Plane High Availability Deployment Patt...
- 控制平面安全加固指南 (Control Plane Security Hardening Guide)
- 控制平面监控与可观测性 (Control Plane Monitoring & Observability)
- 控制平面故障排查手册 (Control Plane Troubleshooting Handbook)
- 控制平面升级与迁移策略 (Control Plane Upgrade & Migration Strategy)
- 控制平面性能基准测试 (Control Plane Performance Benchmarking)
- 控制平面扩缩容指南 (Control Plane Scalability Guide)

## Related

- [[etcd]]

- etcd 深度解析
- KCM 深度解析
- 相关知识域: 集群基础
- 相关知识域: 工作负载
- 相关知识域: 网络
- 相关知识域: 存储
- 相关知识域: 安全
- [[系统基础/速查卡/k8s.md|速查卡: k8s]]
- [[系统基础/速查卡/kubectl-scene-cheatsheet.md|速查卡: kubectl-scene-cheatsheet]]
- [[生态参考/领域索引/etcd-index.md|etcd 知识图谱索引]]

## See Also

- 10-plane-backup-disaster-recovery
- 11-etcd-deep-dive
- 13-kube-controller-manager-deep-dive
- 14-cloud-controller-manager-deep-dive

```

<!-- risk-assessed -->
