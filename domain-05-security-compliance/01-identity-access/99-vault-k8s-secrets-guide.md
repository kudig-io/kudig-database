---
title: Vault K8s 密钥管理集成深度实践
description: '# Vault K8s 密钥管理集成深度实践'
category: cloud-native-security
tags:
- k8s
- security
- cloud-native
- falco
- opa
- etcd
- prometheus
- helm
- postgresql
- ingress
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- 安全工程师
- SRE
- 架构师
estimated_read_time: 5min
intent_queries:
- Vault K8s 密钥管理集成深度实践 是什么
- 如何 Vault K8s 密钥管理集成深度实践
- Kubernetes 25 cloud native security 最佳实践
trigger_keywords:
- Vault
- K8s
- 密钥管理集成深度实践
- cloud
- native
- security
prerequisites:
- kubectl-basics
- rbac-basics
- helm-basics
- prometheus-basics
- etcd-basics
- policy-basics
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
authors:
- name: KUDIG Team
  role: contributor
cross_refs:
- type: cheatsheet
  path: ../domain-17-system-foundation/topic-cheat-sheet/tls-pki.md
  label: '速查卡: tls-pki'
created: "2026-05-23"
---

# Vault K8s 密钥管理集成深度实践

> **Author**: Cloud Native Security Architect | **Version**: v1.0 | **Update Time**: 2026-05-18
> **Scenario**: HashiCorp Vault integration with [[Kubernetes|Kubernetes]] [[Secrets Management|secrets management]] | **Complexity**: ⭐⭐⭐⭐

<!-- chunk: 概述 -->## 概述

Kubernetes 原生 Secret 资源仅提供 base64 编码，缺乏细粒度的访问控制、审计追踪和自动轮换能力。HashiCorp Vault 作为企业级密钥管理平台，提供了集中式的密钥存储、动态凭证生成、加密即服务和完整的审计追踪功能。本文详细探讨 Vault 与 Kubernetes 的三种集成模式（Agent Sidecar、External Secrets Operator、CSI Driver），涵盖部署架构、认证配置、密钥注入和自动轮换的完整实践。

Vault 与 Kubernetes 的集成解决了原生 Secret 的多个核心问题。首先是安全性——原生 Secret 仅以 base64 编码存储在 etcd 中，而 Vault 提供了加密存储、访问控制列表（ACL）和审计日志。其次是动态性——Vault 的动态密钥引擎可以按需生成短期数据库凭证，过期后自动回收，消除了静态密码泄露的风险。第三是可审计性——Vault 记录了所有密钥访问操作，包括谁在什么时间访问了什么密钥，满足合规审计要求。第四是集中管理——在多集群环境中，Vault 作为集中的密钥管理后端，确保了密钥的一致性和可管理性。

#<!-- chunk: 威胁模型分析 -->## 威胁模型分析

**密钥泄露风险**：Kubernetes Secret 默认以 base64 编码存储在 etcd 中，任何有 etcd 访问权限或 namespace 级别 Secret 读取权限的用户都可以获取密钥明文。更危险的是，开发人员可能将 Secret 以明文形式写入 ConfigMap、环境变量或代码中，导致凭据泄露到 Git 仓库、镜像层或日志系统中。密钥泄露的后果非常严重——攻击者获取了数据库密码后可以直接访问生产数据库，获取了 API Key 后可以冒充应用调用第三方服务，获取了 TLS 私钥后可以解密流量或冒充服务器。

**密钥轮换缺失**：静态密钥长期不变增加了被破解的风险。当密钥泄露时，需要手动轮换所有使用该密钥的应用，操作复杂且容易遗漏。在大型企业中，一个数据库密码可能被数十个应用使用，手动轮换需要协调所有应用团队同时更新配置，这在实践中非常困难。Vault 的动态密钥引擎通过自动生成和回收短期凭证解决了这个问题——每个应用获取的凭证都是唯一的，过期后自动失效，不需要手动轮换。

**访问控制粒度不足**：Kubernetes RBAC 仅能控制谁可以读取 Secret 资源，无法限制密钥的具体用途、使用次数和有效期。一个拥有 namespace 级别 Secret 读取权限的用户可以读取该命名空间下的所有 Secret，无法限制只能读取特定的密钥。Vault 的策略系统提供了细粒度的访问控制——可以限制每个应用只能读取特定的密钥路径、只能执行特定的操作、只能在特定的时间窗口内访问。

**多集群密钥管理**：在多集群环境中，密钥需要在集群之间同步和一致化管理，原生的 Secret 资源无法满足跨集群密钥管理需求。每个集群独立管理 Secret 导致密钥不一致、轮换不同步，增加了运维复杂度和安全风险。Vault 作为集中的密钥管理后端，所有集群共享同一套密钥和策略，确保了一致性。

**攻击向量与 Vault 防御矩阵**：

| 攻击向量 | 风险等级 | Vault 防御机制 | 集成模式 |
|:---|:---|:---|:---|
| etcd 密钥泄露 | 严重 | 密钥不存储在 etcd | Agent Sidecar / CSI |
| Git 密钥提交 | 高 | 密钥不在代码仓库中 | 所有模式 |
| 容器密钥泄露 | 高 | 内存卷不落盘 | Agent Sidecar |
| 静态密码破解 | 中 | 动态短期凭证 | 动态密钥引擎 |
| 过度权限访问 | 高 | 细粒度策略 | Vault Policy + K8s Auth |
| 密钥未轮换 | 中 | 自动轮换 + TTL | 动态密钥引擎 / ESO |
| 审计追踪缺失 | 中 | 完整审计日志 | Vault Audit |
| 跨集群不一致 | 中 | 集中管理 + 复制 | Performance Replication |

<!-- chunk: 架构设计 -->## 架构设计

#<!-- chunk: 三种集成模式对比 -->## 三种集成模式对比

```mermaid
graph TB
    subgraph "模式 A: Vault Agent Sidecar (推荐)"
        A_POD[Pod]
        A_APP[App Container]
        A_AGENT[Vault Agent Sidecar]
        A_VOL[emptyDir / Memory]
        A_APP -->|读取| A_VOL
        A_AGENT -->|写入| A_VOL
        A_AGENT -->|认证+获取| VAULT1[Vault Server]
    end

    subgraph "模式 B: External Secrets Operator"
        B_ESO[ESO Controller]
        B_SECRET[K8s Secret]
        B_POD[Pod]
        B_ESO -->|同步| B_SECRET
        B_ESO -->|拉取| VAULT2[Vault Server]
        B_POD -->|引用| B_SECRET
    end

    subgraph "模式 C: CSI Driver"
        C_POD[Pod]
        C_CSI[CSI Driver]
        C_MNT[CSI Volume]
        C_POD -->|挂载| C_MNT
        C_CSI -->|认证+获取| VAULT3[Vault Server]
    end
```

| 模式 | 安全性 | 实时性 | 复杂度 | GitOps 兼容 | 适用场景 |
|:---|:---|:---|:---|:---|:---|
| Agent Sidecar | 高（内存卷不落盘） | 实时续租 | 中 | 低 | 生产首选、动态凭证 |
| External Secrets | 中（存入 etcd） | 定期同步 | 低 | 高 | 简单迁移、GitOps 兼容 |
| CSI Driver | 高 | 实时 | 高 | 中 | 特殊合规、已有 CSI 基础设施 |

**模式选择建议**：对于新项目，推荐使用 Vault Agent Sidecar 模式，因为它提供了最高的安全性（密钥仅存在于内存卷中，不写入 etcd）和实时的凭证续租。对于需要与 Argo CD 等 GitOps 工具深度集成的项目，可以使用 External Secrets Operator 模式，因为 ESO 将密钥同步为标准 K8s Secret，Argo CD 可以正常管理。对于有特殊合规要求（如密钥必须通过 CSI 接口注入）或已有 CSI Driver 基础设施的项目，可以使用 CSI Driver 模式。

<!-- chunk: 核心配置 -->## 核心配置

#<!-- chunk: Vault HA 部署 -->## Vault HA 部署

生产环境的 Vault 部署使用 Raft 存储后端的高可用模式，推荐 3 节点集群确保仲裁。Auto-unseal 使用 AWS KMS 等云 KMS 服务，避免手动 unseal 操作。TLS 加密所有 Vault 通信。

```bash
helm repo add hashicorp https://helm.releases.hashicorp.com

helm install vault hashicorp/vault \
  --namespace vault \
  --create-namespace \
  --set server.ha.enabled=true \
  --set server.ha.raft.enabled=true \
  --set server.dataStorage.enabled=true \
  --set server.dataStorage.size=10Gi \
  --set injector.enabled=true \
  --set ui.enabled=true \
  --set server.resources.requests.memory=256Mi \
  --set server.resources.limits.memory=1Gi
```

```yaml
# values-vault-production.yaml
global:
  tlsDisable: false

server:
  ha:
    enabled: true
    raft:
      enabled: true
      setNodeId: true
      config: |
        ui = true
        listener "tcp" {
          tls_disable = 0
          tls_cert_file = "/vault/userconfig/vault-tls/tls.crt"
          tls_key_file  = "/vault/userconfig/vault-tls/tls.key"
          address = "[::]:8200"
          cluster_address = "[::]:8201"
          telemetry {
            unauthenticated_metrics_access = true
          }
        }
        storage "raft" {
          path = "/vault/data"
          retry_join {
            leader_api_addr = "https://vault-0.vault-internal:8200"
          }
          retry_join {
            leader_api_addr = "https://vault-1.vault-internal:8200"
          }
          retry_join {
            leader_api_addr = "https://vault-2.vault-internal:8200"
          }
        }
        service_registration "kubernetes" {}
        telemetry {
          prometheus_retention_time = "30s"
          disable_hostname = true
        }
        seal "awskms" {
          region     = "us-west-2"
          kms_key_id = "arn:aws:kms:us-west-2:123456789012:key/abc123"
        }
    replicas: 3

  ingress:
    enabled: true
    hosts:
      - host: vault.example.com
    tls:
      - secretName: vault-tls
        hosts:
          - vault.example.com

  resources:
    requests:
      memory: 256Mi
      cpu: 250m
    limits:
      memory: 1Gi
      cpu: "1"

  affinity:
    podAntiAffinity:
      requiredDuringSchedulingIgnoredDuringExecution:
        - labelSelector:
            matchLabels:
              app.kubernetes.io/name: vault
          topologyKey: kubernetes.io/hostname

  topologySpreadConstraints:
    - maxSkew: 1
      topologyKey: topology.kubernetes.io/zone
      whenUnsatisfiable: DoNotSchedule
      labelSelector:
        matchLabels:
          app.kubernetes.io/name: vault

  volumes:
    - name: vault-tls
      secret:
        secretName: vault-tls

  volumeMounts:
    - mountPath: /vault/userconfig/vault-tls
      name: vault-tls
      readOnly: true

  standalone:
    enabled: false

  dataStorage:
    enabled: true
    size: 10Gi
    storageClass: fast-ssd

  auditStorage:
    enabled: true
    size: 5Gi

injector:
  enabled: true
  metrics:
    enabled: true
  resources:
    requests:
      memory: 128Mi
      cpu: 100m
    limits:
      memory: 256Mi
      cpu: 500m
  replicaCount: 2
  leaderElector:
    enabled: true

csi:
  enabled: true
  image: hashicorp/vault-csi-provider
  resources:
    requests:
      memory: 64Mi
      cpu: 50m
    limits:
      memory: 128Mi
      cpu: 200m
```

#<!-- chunk: Kubernetes 认证配置 -->## Kubernetes 认证配置

Vault 的 Kubernetes 认证方法允许 Pod 使用其 ServiceAccount JWT Token 向 Vault 认证。Vault 通过 TokenReview API 验证 JWT Token 的有效性，然后根据配置的角色和策略返回对应的密钥。

```bash
#!/bin/bash
# vault_k8s_auth_setup.sh

# 1. 初始化并解封 Vault
kubectl exec -it vault-0 -n vault -- vault operator init -key-shares=5 -key-threshold=3 -format=json > init.json
UNSEAL_KEY_1=$(jq -r '.unseal_keys_b64[0]' init.json)
UNSEAL_KEY_2=$(jq -r '.unseal_keys_b64[1]' init.json)
UNSEAL_KEY_3=$(jq -r '.unseal_keys_b64[2]' init.json)
ROOT_TOKEN=$(jq -r '.root_token' init.json)

kubectl exec -it vault-0 -n vault -- vault operator unseal "$UNSEAL_KEY_1"
kubectl exec -it vault-0 -n vault -- vault operator unseal "$UNSEAL_KEY_2"
kubectl exec -it vault-0 -n vault -- vault operator unseal "$UNSEAL_KEY_3"

# 2. 登录
kubectl exec -it vault-0 -n vault -- vault login "$ROOT_TOKEN"

# 3. 启用 Kubernetes 认证
kubectl exec -it vault-0 -n vault -- vault auth enable kubernetes

# 4. 配置 Kubernetes 认证
kubectl exec -it vault-0 -n vault -- vault write auth/kubernetes/config \
  token_reviewer_jwt="$(kubectl exec -it vault-0 -n vault -- cat /var/run/secrets/kubernetes.io/serviceaccount/token)" \
  kubernetes_host="https://$KUBERNETES_PORT_443_TCP_ADDR:443" \
  kubernetes_ca_cert=@/var/run/secrets/kubernetes.io/serviceaccount/ca.crt \
  issuer="https://kubernetes.default.svc.cluster.local"

# 5. 启用 KV v2 引擎
kubectl exec -it vault-0 -n vault -- vault secrets enable -path=secret kv-v2

# 6. 启用数据库引擎
kubectl exec -it vault-0 -n vault -- vault secrets enable database

# 7. 创建应用密钥
kubectl exec -it vault-0 -n vault -- vault kv put secret/production/myapp \
  db_username="appuser" \
  db_password="$(openssl rand -base64 24)" \
  api_key="$(openssl rand -hex 32)" \
  stripe_key="sk_live_xxx"

# 8. 创建访问策略
cat > /tmp/myapp-policy.hcl << 'EOF'
path "secret/data/production/myapp" {
  capabilities = ["read"]
}
path "database/creds/myapp" {
  capabilities = ["read"]
}
path "database/creds/myapp-readonly" {
  capabilities = ["read"]
}
path "pki/issue/myapp" {
  capabilities = ["update"]
}
EOF

kubectl cp /tmp/myapp-policy.hcl vault-0:/tmp/myapp-policy.hcl -n vault
kubectl exec -it vault-0 -n vault -- vault policy write myapp /tmp/myapp-policy.hcl

# 9. 创建 Kubernetes 角色
kubectl exec -it vault-0 -n vault -- vault write auth/kubernetes/role/myapp \
  bound_service_account_names=myapp-sa \
  bound_service_account_namespaces=production \
  policies=myapp \
  ttl=1h \
  max_ttl=24h

# 10. 创建 ESO 角色（用于 External Secrets Operator）
cat > /tmp/eso-policy.hcl << 'EOF'
path "secret/data/production/*" {
  capabilities = ["read", "list"]
}
path "database/creds/*" {
  capabilities = ["read"]
}
EOF

kubectl cp /tmp/eso-policy.hcl vault-0:/tmp/eso-policy.hcl -n vault
kubectl exec -it vault-0 -n vault -- vault policy write eso-policy /tmp/eso-policy.hcl

kubectl exec -it vault-0 -n vault -- vault write auth/kubernetes/role/eso-global \
  bound_service_account_names=external-secrets-sa \
  bound_service_account_namespaces=external-secrets \
  policies=eso-policy \
  ttl=1h

# 11. 创建 ServiceAccount
kubectl create serviceaccount myapp-sa -n production
kubectl create serviceaccount external-secrets-sa -n external-secrets
```

<!-- chunk: 安全策略实战 -->## 安全策略实战

#<!-- chunk: Vault Agent Injector 密钥注入 -->## Vault Agent Injector 密钥注入

Vault Agent Injector 通过 Mutating Admission Webhook 自动向 Pod 注入 Vault Agent Sidecar 容器，Sidecar 使用 Kubernetes ServiceAccount Token 向 Vault 认证，获取密钥后写入共享内存卷供应用容器读取。这是生产环境推荐的集成模式，因为密钥仅存在于内存卷中，不会写入 etcd。

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: myapp
  namespace: production
spec:
  replicas: 3
  selector:
    matchLabels:
      app: myapp
  template:
    metadata:
      labels:
        app: myapp
      annotations:
        vault.hashicorp.com/agent-inject: "true"
        vault.hashicorp.com/role: "myapp"
        vault.hashicorp.com/agent-pre-populate: "true"
        vault.hashicorp.com/agent-pre-populate-only: "false"
        vault.hashicorp.com/agent-run-as-user: "65534"
        vault.hashicorp.com/agent-run-as-group: "65534"
        vault.hashicorp.com/secret-volume-path: "/vault/secrets"

        vault.hashicorp.com/agent-inject-secret-db-creds: "database/creds/myapp"
        vault.hashicorp.com/agent-inject-template-db-creds: |
          {{- with secret "database/creds/myapp" }}
          DB_USERNAME="{{ .Data.username }}"
          DB_PASSWORD="{{ .Data.password }}"
          DB_URL="postgres.postgres.svc.cluster.local:5432/myapp?sslmode=require"
          {{- end }}

        vault.hashicorp.com/agent-inject-secret-config: "secret/data/production/myapp"
        vault.hashicorp.com/agent-inject-template-config: |
          {{- with secret "secret/data/production/myapp" }}
          {
            "api_key": "{{ .Data.data.api_key }}",
            "stripe_key": "{{ .Data.data.stripe_key }}"
          }
          {{- end }}

        vault.hashicorp.com/agent-inject-secret-tls-cert: "pki/issue/myapp"
        vault.hashicorp.com/agent-inject-template-tls-cert: |
          {{- with secret "pki/issue/myapp" "common_name=myapp.production.svc.cluster.local" "ttl=24h" }}
          {{ .Data.certificate }}
          {{- end }}

        vault.hashicorp.com/agent-inject-secret-tls-key: "pki/issue/myapp"
        vault.hashicorp.com/agent-inject-template-tls-key: |
          {{- with secret "pki/issue/myapp" "common_name=myapp.production.svc.cluster.local" "ttl=24h" }}
          {{ .Data.private_key }}
          {{- end }}

        vault.hashicorp.com/address: "https://vault.vault.svc.cluster.local:8200"
        vault.hashicorp.com/tls-skip-verify: "false"
        vault.hashicorp.com/ca-cert: "/vault/tls/ca.crt"
    spec:
      serviceAccountName: myapp-sa
      securityContext:
        runAsNonRoot: true
        runAsUser: 1001
        fsGroup: 1001
      containers:
        - name: myapp
          image: registry.company.com/myapp:v1.2.3
          env:
            - name: DB_CREDS_FILE
              value: "/vault/secrets/db-creds"
            - name: CONFIG_FILE
              value: "/vault/secrets/config"
            - name: TLS_CERT_FILE
              value: "/vault/secrets/tls-cert"
            - name: TLS_KEY_FILE
              value: "/vault/secrets/tls-key"
          volumeMounts:
            - name: vault-secrets
              mountPath: /vault/secrets
          resources:
            requests:
              cpu: 100m
              memory: 128Mi
            limits:
              cpu: 500m
              memory: 512Mi
          livenessProbe:
            httpGet:
              path: /actuator/health/liveness
              port: 8080
            initialDelaySeconds: 30
            periodSeconds: 15
          readinessProbe:
            httpGet:
              path: /actuator/health/readiness
              port: 8080
            initialDelaySeconds: 10
            periodSeconds: 10
      volumes:
        - name: vault-secrets
          emptyDir:
            medium: Memory
```

#<!-- chunk: External Secrets Operator -->## External Secrets Operator

External Secrets Operator（ESO）将 Vault 密钥同步为标准 Kubernetes Secret 资源，与 GitOps 工具兼容性更好。ESO 的优势在于：密钥同步为标准 K8s Secret 后，Argo CD 可以正常管理和追踪这些 Secret；应用不需要修改代码即可使用——原来通过环境变量引用 Secret 的应用无需改动；支持模板化密钥转换——可以将 Vault 中的密钥格式转换为应用需要的格式。

```yaml
helm repo add external-secrets https://charts.external-secrets.io
helm install external-secrets external-secrets/external-secrets \
  --namespace external-secrets \
  --create-namespace \
  --set replicaCount=2 \
  --set serviceMonitor.enabled=true
---
apiVersion: external-secrets.io/v1beta1
kind: SecretStore
metadata:
  name: vault-backend
  namespace: production
spec:
  provider:
    vault:
      server: "https://vault.vault.svc.cluster.local:8200"
      path: "secret"
      version: "v2"
      auth:
        kubernetes:
          mountPath: "kubernetes"
          role: "myapp"
          serviceAccountRef:
            name: myapp-sa
---
apiVersion: external-secrets.io/v1beta1
kind: ClusterSecretStore
metadata:
  name: vault-global
spec:
  provider:
    vault:
      server: "https://vault.vault.svc.cluster.local:8200"
      path: "secret"
      version: "v2"
      auth:
        kubernetes:
          mountPath: "kubernetes"
          role: "eso-global"
          serviceAccountRef:
            name: external-secrets-sa
            namespace: external-secrets
---
apiVersion: external-secrets.io/v1beta1
kind: ExternalSecret
metadata:
  name: myapp-secrets
  namespace: production
spec:
  refreshInterval: 1h
  secretStoreRef:
    kind: SecretStore
    name: vault-backend
  target:
    name: myapp-k8s-secret
    creationPolicy: Owner
    template:
      type: Opaque
      data:
        DB_USERNAME: "{{ .db_username }}"
        DB_PASSWORD: "{{ .db_password }}"
        API_KEY: "{{ .api_key }}"
  data:
    - secretKey: db_username
      remoteRef:
        key: secret/data/production/myapp
        property: db_username
    - secretKey: db_password
      remoteRef:
        key: secret/data/production/myapp
        property: db_password
    - secretKey: api_key
      remoteRef:
        key: secret/data/production/myapp
        property: api_key
---
apiVersion: external-secrets.io/v1beta1
kind: ExternalSecret
metadata:
  name: myapp-db-dynamic
  namespace: production
spec:
  refreshInterval: 55m
  secretStoreRef:
    kind: SecretStore
    name: vault-backend
  target:
    name: myapp-db-creds
    creationPolicy: Owner
    template:
      type: Opaque
      data:
        DATABASE_URL: "postgresql://{{ .username }}:{{ .password }}@postgres.postgres.svc.cluster.local:5432/myapp?sslmode=require"
  dataFrom:
    - extract:
        key: database/creds/myapp
```

#<!-- chunk: 动态数据库凭证 -->## 动态数据库凭证

Vault 的动态密钥引擎是区分于其他密钥管理工具的核心功能。它不存储静态的数据库密码，而是在应用请求时临时创建数据库用户，分配有限权限，设置 TTL。凭证过期后自动回收（删除数据库用户）。这消除了静态密码泄露的风险，每个应用实例获取的凭证都是唯一且短期的。

```bash
# 启用数据库引擎
kubectl exec -it vault-0 -n vault -- vault secrets enable database

# 配置 PostgreSQL 连接
kubectl exec -it vault-0 -n vault -- vault write database/config/myapp-postgres \
  plugin_name=postgresql-database-plugin \
  allowed_roles="myapp,myapp-readonly" \
  connection_url="postgresql://{{username}}:{{password}}@postgres.postgres.svc.cluster.local:5432/myapp?sslmode=require" \
  username="vault_admin" \
  password="vault_admin_password"

# 创建读写角色
kubectl exec -it vault-0 -n vault -- vault write database/roles/myapp \
  db_name=myapp-postgres \
  creation_statements="CREATE ROLE \"{{name}}\" WITH LOGIN PASSWORD '{{password}}' VALID UNTIL '{{expiration}}'; GRANT SELECT, INSERT, UPDATE, DELETE ON ALL TABLES IN SCHEMA public TO \"{{name}}\"; GRANT USAGE, SELECT ON ALL SEQUENCES IN SCHEMA public TO \"{{name}}\";" \
  revocation_statements="DROP ROLE IF EXISTS \"{{name}}\";" \
  default_ttl=1h \
  max_ttl=24h

# 创建只读角色
kubectl exec -it vault-0 -n vault -- vault write database/roles/myapp-readonly \
  db_name=myapp-postgres \
  creation_statements="CREATE ROLE \"{{name}}\" WITH LOGIN PASSWORD '{{password}}' VALID UNTIL '{{expiration}}'; GRANT SELECT ON ALL TABLES IN SCHEMA public TO \"{{name}}\";" \
  revocation_statements="DROP ROLE IF EXISTS \"{{name}}\";" \
  default_ttl=1h \
  max_ttl=24h

# 创建审计角色（只读 + 连接限制）
kubectl exec -it vault-0 -n vault -- vault write database/roles/myapp-audit \
  db_name=myapp-postgres \
  creation_statements="CREATE ROLE \"{{name}}\" WITH LOGIN PASSWORD '{{password}}' VALID UNTIL '{{expiration}}' CONNECTION LIMIT 5; GRANT SELECT ON ALL TABLES IN SCHEMA public TO \"{{name}}\";" \
  default_ttl=30m \
  max_ttl=2h

# 测试获取动态凭证
kubectl exec -it vault-0 -n vault -- vault read database/creds/myapp
```

#<!-- chunk: PKI 引擎自动 TLS -->## PKI 引擎自动 TLS

```bash
# 启用 PKI 引擎
kubectl exec -it vault-0 -n vault -- vault secrets enable pki
kubectl exec -it vault-0 -n vault -- vault secrets tune -max-lease-ttl=8760h pki

# 生成 Root CA
kubectl exec -it vault-0 -n vault -- vault write pki/root/generate/internal \
  common_name="MyOrg Root CA" \
  ttl=87600h \
  key_type=ec \
  key_bits=256

# 配置 CA 和 CRL URL
kubectl exec -it vault-0 -n vault -- vault write pki/config/urls \
  issuing_certificates="https://vault.example.com/v1/pki/ca" \
  crl_distribution_points="https://vault.example.com/v1/pki/crl"

# 创建中间 CA
kubectl exec -it vault-0 -n vault -- vault secrets enable -path=pki-int pki
kubectl exec -it vault-0 -n vault -- vault write pki-int/intermediate/generate/internal \
  common_name="MyOrg Intermediate CA" \
  ttl=43800h

# 创建服务端证书角色
kubectl exec -it vault-0 -n vault -- vault write pki/roles/myapp \
  allowed_domains="production.svc.cluster.local,svc.cluster.local" \
  allow_subdomains=true \
  max_ttl=720h \
  key_type=ec \
  key_bits=256 \
  server_flag=true \
  client_flag=true

# 测试签发证书
kubectl exec -it vault-0 -n vault -- vault write pki/issue/myapp \
  common_name="myapp.production.svc.cluster.local" \
  ttl=24h
```

#<!-- chunk: Transit 引擎加密即服务 -->## Transit 引擎加密即服务

```bash
# 启用 Transit 引擎
kubectl exec -it vault-0 -n vault -- vault secrets enable transit

# 创建加密密钥
kubectl exec -it vault-0 -n vault -- vault write -f transit/keys/myapp-encryption \
  type=aes256-gcm96 \
  exportable=false \
  convergent_encryption=false

# 加密数据
kubectl exec -it vault-0 -n vault -- vault write transit/encrypt/myapp-encryption \
  plaintext=$(echo -n "sensitive data" | base64)

# 解密数据
kubectl exec -it vault-0 -n vault -- vault write transit/decrypt/myapp-encryption \
  ciphertext="vault:v1:..."

# 轮换密钥
kubectl exec -it vault-0 -n vault -- vault write -f transit/keys/myapp-encryption/rotate
```

<!-- chunk: 合规与审计 -->## 合规与审计

#<!-- chunk: Vault 审计日志 -->## Vault 审计日志

Vault 的审计日志记录了所有操作，包括认证请求、密钥访问、策略变更和管理操作。审计日志是合规审计的重要证据来源。

```bash
# 启用文件审计
kubectl exec -it vault-0 -n vault -- vault audit enable file file_path=/vault/logs/audit.log

# 启用 Syslog 审计（发送到外部 SIEM）
kubectl exec -it vault-0 -n vault -- vault audit enable syslog \
  facility="AUTH" \
  tag="vault" \
  address="syslog.monitoring.svc.cluster.local:514"

# 查看审计设备
kubectl exec -it vault-0 -n vault -- vault audit list -detailed
```

#<!-- chunk: 审计日志分析 -->## 审计日志分析

```bash
#!/bin/bash
# vault_audit_analysis.sh

AUDIT_LOG="/var/log/vault/audit.log"

echo "=== Vault Audit Log Analysis ==="
echo "Date: $(date)"
echo ""

echo "1. Top 10 Most Active Users"
jq -r '.auth.accessor // "anonymous"' "$AUDIT_LOG" | sort | uniq -c | sort -rn | head -10
echo ""

echo "2. Failed Authentication Attempts"
jq 'select(.type == "request" and .request.path | test("login")) |
    select(.response.auth == null or .response.auth.lease_duration == 0)' \
  "$AUDIT_LOG" | jq -r '"\(.time) | \(.request.path) | \(.request.data | keys)"'
echo ""

echo "3. Secret Access by Path"
jq -r 'select(.type == "request") | .request.path' "$AUDIT_LOG" | \
  grep -E "^secret/" | sort | uniq -c | sort -rn | head -20
echo ""

echo "4. Policy Changes"
jq 'select(.request.path | test("sys/policies"))' "$AUDIT_LOG" | \
  jq -r '"\(.time) | \(.request.operation) | \(.request.path)"'
echo ""

echo "5. Root Token Usage"
jq 'select(.auth.policies | contains(["root"]))' "$AUDIT_LOG" | \
  jq -r '"\(.time) | \(.request.operation) | \(.request.path)"' | head -20
echo ""

echo "6. Dynamic Credential Creation"
jq 'select(.request.path | test("database/creds"))' "$AUDIT_LOG" | \
  jq -r '"\(.time) | \(.request.path) | \(.auth.entity_id)"'
echo ""

echo "7. PKI Certificate Issuance"
jq 'select(.request.path | test("pki/issue"))' "$AUDIT_LOG" | \
  jq -r '"\(.time) | \(.request.path) | \(.request.data.common_name)"'
```

#<!-- chunk: 合规检查脚本 -->## 合规检查脚本

```bash
#!/bin/bash
# vault_compliance_check.sh

echo "=== Vault Compliance Check ==="
echo ""

echo "1. Audit Devices Enabled"
kubectl exec -it vault-0 -n vault -- vault audit list -detailed
echo ""

echo "2. Auth Methods Enabled"
kubectl exec -it vault-0 -n vault -- vault auth list -detailed
echo ""

echo "3. Secrets Engines Enabled"
kubectl exec -it vault-0 -n vault -- vault secrets list -detailed
echo ""

echo "4. Policies"
kubectl exec -it vault-0 -n vault -- vault policy list
echo ""

echo "5. Root Token Check"
echo "WARNING: If root token is still in use, create a new token and revoke root"
echo ""

echo "6. Seal Status"
kubectl exec -it vault-0 -n vault -- vault status | grep -E "Sealed|Total Shares|Threshold"
echo ""

echo "7. Lease Count"
kubectl exec -it vault-0 -n vault -- vault list sys/leases/count 2>/dev/null
echo ""

echo "8. HA Status"
kubectl exec -it vault-0 -n vault -- vault operator raft list-peers
```

<!-- chunk: 监控与告警 -->## 监控与告警

#<!-- chunk: Prometheus 监控 -->## Prometheus 监控

```yaml
apiVersion: monitoring.coreos.com/v1
kind: ServiceMonitor
metadata:
  name: vault-metrics
  namespace: vault
spec:
  selector:
    matchLabels:
      app.kubernetes.io/name: vault
  endpoints:
    - port: http
      path: /v1/sys/metrics
      params:
        format:
          - prometheus
      interval: 30s
---
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: vault-alerts
  namespace: vault
spec:
  groups:
    - name: vault.rules
      rules:
        - alert: VaultSealed
          expr: vault_core_unsealed == 0
          for: 2m
          labels:
            severity: critical
          annotations:
            summary: "Vault 实例已密封"
            description: "Vault 实例 {{ $labels.instance }} 处于密封状态"

        - alert: VaultHighSealEntropy
          expr: rate(vault_runtime_alloc_bytes[5m]) > 1e8
          for: 10m
          labels:
            severity: warning
          annotations:
            summary: "Vault 内存分配速率异常"

        - alert: VaultTokenRenewalFailure
          expr: rate(vault.core.leadership.setup_cluster[5m]) > 0
          for: 5m
          labels:
            severity: warning
          annotations:
            summary: "Vault 令牌续租失败"

        - alert: VaultRaftQuorumLost
          expr: vault.raft.state == 0
          for: 1m
          labels:
            severity: critical
          annotations:
            summary: "Vault Raft 集群失去仲裁"

        - alert: VaultAuditLoggingFailed
          expr: rate(vault.audit.log.request.failure[5m]) > 0
          for: 2m
          labels:
            severity: critical
          annotations:
            summary: "Vault 审计日志写入失败"

        - alert: VaultHighRequestRate
          expr: rate(vault.core.handle_request_count[5m]) > 1000
          for: 5m
          labels:
            severity: warning
          annotations:
            summary: "Vault 请求速率异常高: {{ $value }}/s"

        - alert: VaultLeaseExhaustion
          expr: vault_core_leases_count > 100000
          for: 10m
          labels:
            severity: warning
          annotations:
            summary: "Vault 租约数量过多: {{ $value }}"

        - alert: VaultInjectorErrors
          expr: rate(vault_injector_injections_total{status="error"}[5m]) > 0
          for: 5m
          labels:
            severity: warning
          annotations:
            summary: "Vault Agent Injector 注入错误"
```

<!-- chunk: 最佳实践 -->## 最佳实践

| 实践 | 说明 | 详情 |
|:---|:---|:---|
| 最小权限策略 | 每个应用独立策略 | 仅授予必要的密钥路径读取权限 |
| 动态凭证优先 | 使用动态密钥引擎 | 数据库、云 IAM、RabbitMQ 等 |
| 内存卷存储 | Agent Sidecar 使用 emptyDir Memory | 密钥不写入 etcd |
| 审计日志 | 启用至少一个审计设备 | 文件 + Syslog 双重审计 |
| Auto-unseal | 使用云 KMS 自动解封 | AWS KMS / GCP KMS / Azure Key Vault |
| TLS 加密 | 所有 Vault 通信使用 TLS | 包括集群内部通信 |
| 密钥轮换 | 定期轮换静态密钥 | Vault KV v2 支持版本管理 |
| 多集群复制 | Performance Replication | 主集群写，DR 集群读 |
| 策略审计 | 定期审查策略和角色 | 删除不再使用的权限 |
| HA 部署 | 至少 3 节点 Raft 集群 | 确保 Raft 仲裁 |

#<!-- chunk: 多集群密钥管理 -->## 多集群密钥管理

```yaml
# 主集群 Vault 配置（读写）
apiVersion: secrets-store.csi.x-k8s.io/v1
kind: SecretProviderClass
metadata:
  name: vault-primary
  namespace: production
spec:
  provider: vault
  parameters:
    vaultAddress: "https://vault.primary.example.com:8200"
    roleName: "myapp"
    objects: |
      - objectName: "db-password"
        secretPath: "secret/data/production/database"
        secretKey: "password"
---
# DR 集群 Vault 配置（只读）
apiVersion: secrets-store.csi.x-k8s.io/v1
kind: SecretProviderClass
metadata:
  name: vault-dr
  namespace: production
spec:
  provider: vault
  parameters:
    vaultAddress: "https://vault.dr.example.com:8200"
    roleName: "myapp"
    objects: |
      - objectName: "db-password"
        secretPath: "secret/data/production/database"
        secretKey: "password"
```

<!-- chunk: 事件响应流程 -->## 事件响应流程

| 事件 | 严重程度 | 响应时间 | 操作 |
|:---|:---|:---|:---|
| Vault 密封 | Critical | < 15min | 检查 Auto-unseal，手动 unseal |
| Raft 仲裁丢失 | Critical | < 15min | 检查节点状态，替换故障节点 |
| 审计日志失败 | High | < 1h | 检查审计设备配置，恢复日志 |
| 密钥泄露 | Critical | < 30min | 轮换密钥，撤销相关 Token |
| Token 续租失败 | Medium | < 4h | 检查 Vault 状态和应用日志 |
| Injector 注入失败 | Medium | < 2h | 检查 Webhook 和 SA 配置 |

<!-- chunk: 故障排查 -->## 故障排查

#<!-- chunk: 常见问题 -->## 常见问题

**Agent 注入不生效**：检查 Pod 是否有正确的注解 `vault.hashicorp.com/agent-inject: "true"`。确认 Vault Injector 的 Mutating Webhook 是否正常运行。查看 Injector 日志是否有错误。检查 namespace 是否被 Webhook 排除。

**认证失败**：确认 ServiceAccount 名称与 Vault 角色的 `bound_service_account_names` 匹配。检查命名空间是否在 `bound_service_account_namespaces` 中。验证 Kubernetes 认证配置的 issuer 是否与集群的 OIDC issuer 匹配。

**密钥获取为空**：检查 KV 引擎路径和版本（v1/v2）。确认策略中的路径是否正确（v2 需要使用 `secret/data/` 前缀）。验证密钥确实存在于指定路径。

**动态凭证创建失败**：检查数据库连接配置是否正确。确认 Vault 管理员账号是否有足够的权限创建数据库用户。查看 Vault 日志中的数据库错误信息。

```bash
#!/bin/bash
# vault_k8s_diagnostics.sh

echo "=== Vault Cluster Health ==="
kubectl exec -it vault-0 -n vault -- vault status
echo ""

echo "=== Vault Pods ==="
kubectl get pods -n vault -o wide
echo ""

echo "=== Resource Usage ==="
kubectl top pods -n vault
echo ""

echo "=== Raft Cluster Status ==="
kubectl exec -it vault-0 -n vault -- vault operator raft list-peers
echo ""

echo "=== Kubernetes Auth Config ==="
kubectl exec -it vault-0 -n vault -- vault read auth/kubernetes/config
echo ""

echo "=== Registered Roles ==="
kubectl exec -it vault-0 -n vault -- vault list auth/kubernetes/roles
echo ""

echo "=== Policies ==="
kubectl exec -it vault-0 -n vault -- vault policy list
echo ""

echo "=== Secrets Engines ==="
kubectl exec -it vault-0 -n vault -- vault secrets list
echo ""

echo "=== Audit Devices ==="
kubectl exec -it vault-0 -n vault -- vault audit list
echo ""

echo "=== Injector Logs (last 20 lines) ==="
kubectl logs -n vault -l app.kubernetes.io/name=vault-agent-injector --tail=20
echo ""

echo "=== Sidecar Injection Events ==="
kubectl get events -n production --field-selector reason=Injected --sort-by='.lastTimestamp' | tail -10
echo ""

echo "=== Test Authentication ==="
SA_TOKEN=$(kubectl create token myapp-sa -n production --duration=1h)
kubectl exec -it vault-0 -n vault -- vault write auth/kubernetes/login \
  jwt="$SA_TOKEN" role="myapp"
echo ""

echo "=== Active Leases ==="
kubectl exec -it vault-0 -n vault -- vault list sys/leases/lookup/auth/kubernetes/login 2>/dev/null | head -20
echo ""

echo "=== Certificate Expiry ==="
kubectl exec -it vault-0 -n vault -- vault read pki/cert/ca-chain | grep -E "Not Before|Not After"
```

---

*本文档基于 Vault 与 Kubernetes 密钥管理集成实践经验编写，持续更新最新技术和最佳实践。*

---

<!-- chunk: Obsidian 相关文档 -->## Obsidian 相关文档

- domain-05-security-compliance MOC
- [[domain-05-security-compliance/README.md|Domain 25: 云原生安全 (Cloud Native Security)]]
- [[domain-05-security-compliance/00-open-source-projects-index.md|Domain-25 云原生安全 — 开源项目索引]]
- Falco 云原生安全监控深度实践
- Sysdig企业级容器安全深度实践
- Aqua Security 企业级容器安全平台深度实践
- Kyverno 企业级策略管理深度实践
- HashiCorp Vault 企业级密钥管理深度实践
- OPA Gatekeeper 策略即代码深度实践
- 容器镜像安全扫描深度实践
- Kubernetes 安全加固深度实践
- gVisor 容器沙箱深度解析

## See Also

- 99-kyverno-policy-guide
- 99-opa-gatekeeper-policy-guide
- 01-falco-cloud-native-security
- 02-sysdig-enterprise-container-security

- [[domain-05-security-compliance/README.md|返回目录]]