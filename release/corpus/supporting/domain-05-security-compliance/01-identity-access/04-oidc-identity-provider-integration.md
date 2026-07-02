---
title: OIDC 身份提供商集成
description: 'OIDC 身份集成：Dex/Keycloak 作为 IdP、kubeconfig OIDC 配置、Group-based RBAC 映射、多租户身份隔离'
summary: 'Dex/Keycloak 集成、OIDC kubeconfig、Group RBAC 映射与多租户隔离'
category: security-compliance
tags:
- oidc
- identity-provider
- dex
- keycloak
- authentication
- multi-tenant
tier: supporting
created: '2026-07-02'
last_updated: 2026-07
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 安全工程师
- 平台工程师
estimated_read_time: 15min
intent_queries:
- OIDC 身份提供商集成是什么
- 如何配置 Dex/Keycloak 作为 K8s IdP
trigger_keywords:
- OIDC
- Dex
- Keycloak
- 身份提供商
- kubeconfig
- 多租户
prerequisites:
- kubectl-basics
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

# OIDC 身份提供商集成

## 概述

Kubernetes API Server 支持 OpenID Connect (OIDC) 作为身份认证方式。通过集成 Dex、Keycloak 等 OIDC Provider，可以实现企业级 SSO、集中式身份管理和细粒度 RBAC 控制。

## 1. OIDC 认证流程

```
用户 → kubectl（OIDC 插件）→ 获取 ID Token
  ↓
API Server 验证 ID Token
  ↓
提取 claims: username, groups, email
  ↓
RBAC 授权匹配
```

API Server 关键参数：
- `--oidc-issuer-url`：OIDC Provider 的发行者 URL
- `--oidc-client-id`：API Server 的 Client ID
- `--oidc-username-claim`：用于用户名的 Claim 字段
- `--oidc-groups-claim`：用于用户组的 Claim 字段
- `--oidc-ca-file`：OIDC Provider 的 CA 证书

## 2. Dex 作为 OIDC Provider

### 2.1 Dex 部署配置

```yaml
# dex-config.yaml
issuer: https://dex.example.com
storage:
  type: kubernetes
  config:
    inCluster: true
web:
  http: 0.0.0.0:5556
  tlsCert: /etc/dex/tls/tls.crt
  tlsKey: /etc/dex/tls/tls.key
oauth2:
  skipApprovalScreen: true
  responseTypes: ["code", "token", "id_token"]
staticClients:
- id: kubernetes
  redirectURIs:
  - http://localhost:8000/callback
  - https://kubelogin.example.com/callback
  name: Kubernetes
  secret: <base64-encoded-secret>
connectors:
- type: github
  id: github
  name: GitHub
  config:
    clientID: <github-client-id>
    clientSecret: <github-client-secret>
    orgs:
    - name: my-org
      teams:
      - platform-team
      - sre-team
- type: ldap
  id: ldap
  name: LDAP
  config:
    host: ldap.example.com:636
    rootCA: /etc/dex/ldap-ca.crt
    bindDN: cn=admin,dc=example,dc=com
    bindPW: <ldap-password>
    userSearch:
      baseDN: ou=People,dc=example,dc=com
      filter: "(objectClass=person)"
      username: uid
      idAttr: uid
      emailAttr: mail
      nameAttr: cn
    groupSearch:
      baseDN: ou=Groups,dc=example,dc=com
      filter: "(objectClass=groupOfNames)"
      userMatchers:
      - userAttr: DN
        groupAttr: member
      nameAttr: cn
```

### 2.2 Dex Deployment

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: dex
  namespace: dex
spec:
  replicas: 2
  selector:
    matchLabels:
      app: dex
  template:
    metadata:
      labels:
        app: dex
    spec:
      serviceAccountName: dex
      containers:
      - name: dex
        image: dexidp/dex:v2.37.0
        command: ["dex", "serve", "/etc/dex/config.yaml"]
        ports:
        - containerPort: 5556
          name: https
        volumeMounts:
        - name: config
          mountPath: /etc/dex
        - name: tls
          mountPath: /etc/dex/tls
        livenessProbe:
          httpGet:
            path: /healthz/live
            port: 5556
            scheme: HTTPS
          initialDelaySeconds: 5
        readinessProbe:
          httpGet:
            path: /healthz/ready
            port: 5556
            scheme: HTTPS
          initialDelaySeconds: 5
      volumes:
      - name: config
        configMap:
          name: dex-config
      - name: tls
        secret:
          secretName: dex-tls
---
apiVersion: v1
kind: Service
metadata:
  name: dex
  namespace: dex
spec:
  ports:
  - name: https
    port: 443
    targetPort: 5556
  selector:
    app: dex
```

### 2.3 API Server OIDC 配置

```yaml
# kube-apiserver 配置（静态 Pod 或 kubeadm 配置）
apiVersion: kubeadm.k8s.io/v1beta3
kind: ClusterConfiguration
apiServer:
  extraArgs:
    oidc-issuer-url: https://dex.example.com
    oidc-client-id: kubernetes
    oidc-username-claim: email
    oidc-groups-claim: groups
    oidc-ca-file: /etc/kubernetes/pki/dex-ca.crt
    oidc-signing-algs: RS256
```

## 3. Keycloak 作为 OIDC Provider

### 3.1 Keycloak Realm 配置

```json
{
  "realm": "kubernetes",
  "enabled": true,
  "sslRequired": "external",
  "registrationAllowed": false,
  "loginWithEmailAllowed": true,
  "duplicateEmailsAllowed": false,
  "resetPasswordAllowed": true,
  "editUsernameAllowed": false,
  "bruteForceProtected": true,
  "permanentLockout": false,
  "maxFailureWaitSeconds": 900,
  "failureFactor": 5
}
```

### 3.2 Keycloak Client 配置

```json
{
  "clientId": "kubernetes",
  "name": "Kubernetes Cluster",
  "enabled": true,
  "protocol": "openid-connect",
  "publicClient": false,
  "secret": "<client-secret>",
  "redirectUris": [
    "http://localhost:8000/callback",
    "https://kubelogin.example.com/*"
  ],
  "webOrigins": ["+"],
  "standardFlowEnabled": true,
  "directAccessGrantsEnabled": true,
  "defaultClientScopes": [
    "openid",
    "profile",
    "email",
    "groups"
  ],
  "protocolMappers": [
    {
      "name": "groups",
      "protocol": "openid-connect",
      "protocolMapper": "oidc-usermodel-realm-role-list-mapper",
      "config": {
        "id.token.claim": "true",
        "access.token.claim": "true",
        "claim.name": "groups",
        "multivalued": "true",
        "jsonType.label": "String"
      }
    }
  ]
}
```

### 3.3 Keycloak Group 到 RBAC 映射

```yaml
# Keycloak Groups 结构
# /kubernetes/platform-admin    → cluster-admin
# /kubernetes/namespace-admin   → namespace admin
# /kubernetes/developers        → namespace developer
# /kubernetes/viewers           → read-only

# ClusterRoleBinding 示例
apiVersion: rbac.authorization.k8s.io/v1
kind: ClusterRoleBinding
metadata:
  name: oidc-platform-admin
roleRef:
  apiGroup: rbac.authorization.k8s.io
  kind: ClusterRole
  name: cluster-admin
subjects:
- apiGroup: rbac.authorization.k8s.io
  kind: Group
  name: "kubernetes/platform-admin"
---
apiVersion: rbac.authorization.k8s.io/v1
kind: ClusterRoleBinding
metadata:
  name: oidc-viewers
roleRef:
  apiGroup: rbac.authorization.k8s.io
  kind: ClusterRole
  name: view
subjects:
- apiGroup: rbac.authorization.k8s.io
  kind: Group
  name: "kubernetes/viewers"
```

## 4. kubeconfig OIDC 配置

### 4.1 kubelogin（kubectl oidc-login）

```bash
# 安装 kubelogin
# brew install int128/kubelogin/kubelogin

# 配置 kubeconfig
kubectl config set-credentials oidc-user \
  --exec-api-version=client.authentication.k8s.io/v1beta1 \
  --exec-command=kubectl \
  --exec-arg=oidc-login \
  --exec-arg=get-token \
  --exec-arg=--oidc-issuer-url=https://dex.example.com \
  --exec-arg=--oidc-client-id=kubernetes \
  --exec-arg=--oidc-client-secret=<secret> \
  --exec-arg=--oidc-extra-scope="openid profile email groups"

kubectl config set-context my-cluster \
  --cluster=my-cluster \
  --user=oidc-user
```

### 4.2 自动刷新 Token 的 kubeconfig

```yaml
# ~/.kube/config 片段
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
      - --oidc-client-secret=<secret>
      - --oidc-extra-scope=openid profile email groups
      - --grant-type=authcode
      installHint: |
        Install kubelogin:
        brew install int128/kubelogin/kubelogin
      provideClusterInfo: true
```

## 5. Group-based RBAC 映射

### 5.1 多租户 RBAC 模板

```yaml
# 通用的命名空间 RBAC 模板
apiVersion: rbac.authorization.k8s.io/v1
kind: Role
metadata:
  name: namespace-admin
  namespace: ${NAMESPACE}
rules:
- apiGroups: ["", "apps", "batch", "extensions"]
  resources: ["*"]
  verbs: ["*"]
- apiGroups: ["rbac.authorization.k8s.io"]
  resources: ["roles", "rolebindings"]
  verbs: ["*"]
---
apiVersion: rbac.authorization.k8s.io/v1
kind: Role
metadata:
  name: namespace-developer
  namespace: ${NAMESPACE}
rules:
- apiGroups: ["", "apps", "batch"]
  resources: ["pods", "deployments", "services", "configmaps", "secrets", "jobs", "cronjobs"]
  verbs: ["get", "list", "watch", "create", "update", "patch"]
- apiGroups: [""]
  resources: ["pods/log", "pods/exec"]
  verbs: ["get", "create"]
---
apiVersion: rbac.authorization.k8s.io/v1
kind: Role
metadata:
  name: namespace-viewer
  namespace: ${NAMESPACE}
rules:
- apiGroups: ["*"]
  resources: ["*"]
  verbs: ["get", "list", "watch"]
```

### 5.2 自动化 RBAC 绑定

```yaml
# 使用 Kyverno 自动为 OIDC Group 创建 RBAC
apiVersion: kyverno.io/v1
kind: ClusterPolicy
metadata:
  name: oidc-namespace-rbac
spec:
  rules:
  - name: create-namespace-rbac
    match:
      any:
      - resources:
          kinds:
          - Namespace
    generate:
      kind: RoleBinding
      name: "oidc-admin-binding"
      namespace: "{{ request.object.metadata.name }}"
      synchronize: true
      data:
        roleRef:
          apiGroup: rbac.authorization.k8s.io
          kind: ClusterRole
          name: admin
        subjects:
        - apiGroup: rbac.authorization.k8s.io
          kind: Group
          name: "kubernetes/{{ request.object.metadata.name }}-admin"
```

## 6. 多租户身份隔离

### 6.1 租户隔离架构

```
Tenant A                    Tenant B
├── namespace: tenant-a     ├── namespace: tenant-b
│   ├── RoleBinding         │   ├── RoleBinding
│   │   └── Group:          │   │   └── Group:
│   │       tenant-a-admin  │   │       tenant-b-admin
│   └── NetworkPolicy       │   └── NetworkPolicy
│       └── 隔离流量         │       └── 隔离流量
├── ResourceQuota           ├── ResourceQuota
└── LimitRange              └── LimitRange
```

### 6.2 租户模板自动化

```yaml
# 租户创建模板
apiVersion: batch/v1
kind: Job
metadata:
  name: create-tenant
  namespace: kube-system
spec:
  template:
    spec:
      serviceAccountName: tenant-manager
      containers:
      - name: create-tenant
        image: bitnami/kubectl:latest
        command:
        - /bin/bash
        - -c
        - |
          TENANT=$1

          # 创建命名空间
          kubectl create namespace "tenant-${TENANT}" --dry-run=client -o yaml | \
            kubectl apply -f -

          # 设置 PSA 标签
          kubectl label namespace "tenant-${TENANT}" \
            pod-security.kubernetes.io/enforce=baseline \
            pod-security.kubernetes.io/enforce-version=latest

          # 创建 ResourceQuota
          cat <<EOF | kubectl apply -f -
          apiVersion: v1
          kind: ResourceQuota
          metadata:
            name: tenant-quota
            namespace: tenant-${TENANT}
          spec:
            hard:
              requests.cpu: "10"
              requests.memory: 20Gi
              limits.cpu: "20"
              limits.memory: 40Gi
              pods: "50"
          EOF

          # 创建 RBAC
          cat <<EOF | kubectl apply -f -
          apiVersion: rbac.authorization.k8s.io/v1
          kind: RoleBinding
          metadata:
            name: tenant-admin
            namespace: tenant-${TENANT}
          roleRef:
            apiGroup: rbac.authorization.k8s.io
            kind: ClusterRole
            name: admin
          subjects:
          - apiGroup: rbac.authorization.k8s.io
            kind: Group
            name: "kubernetes/${TENANT}-admin"
          EOF

          # 创建 NetworkPolicy
          cat <<EOF | kubectl apply -f -
          apiVersion: networking.k8s.io/v1
          kind: NetworkPolicy
          metadata:
            name: tenant-isolation
            namespace: tenant-${TENANT}
          spec:
            podSelector: {}
            policyTypes:
            - Ingress
            - Egress
            ingress:
            - from:
              - namespaceSelector:
                  matchLabels:
                    kubernetes.io/metadata.name: "tenant-${TENANT}"
              - namespaceSelector:
                  matchLabels:
                    kubernetes.io/metadata.name: kube-system
            egress:
            - to:
              - namespaceSelector:
                  matchLabels:
                    kubernetes.io/metadata.name: "tenant-${TENANT}"
              - namespaceSelector:
                  matchLabels:
                    kubernetes.io/metadata.name: kube-system
              - to:
                - ipBlock:
                    cidr: 0.0.0.0/0
                  ports:
                  - port: 443
                  - port: 53
          EOF
        args: ["${TENANT}"]
      restartPolicy: Never
```

## 7. 最佳实践

```
OIDC 集成检查清单：

□ 选择合适的 OIDC Provider（Dex 轻量级 / Keycloak 功能全面）
□ 配置 API Server OIDC 参数
□ 定义清晰的 Group 到 RBAC 映射策略
□ 使用 kubelogin 简化用户认证流程
□ 实施多租户身份隔离
□ 定期轮转 OIDC Client Secret
□ 监控认证失败和异常登录
□ 配置 Token 过期和刷新策略
□ 记录所有认证事件用于审计
```

## Related

- [[domain-05-security-compliance/01-identity-access/01-rbac-best-practices|RBAC 最佳实践]]
- [[domain-05-security-compliance/01-identity-access/03-service-account-token-management|SA Token 管理]]

## See Also

- [Dex 文档](https://dexidp.io/docs/)
- [Keycloak 文档](https://www.keycloak.org/documentation)
- [kubelogin](https://github.com/int128/kubelogin)
