---
title: HashiCorp Vault 企业级密钥管理深度实践
description: '# HashiCorp Vault 企业级密钥管理深度实践'
summary: '# HashiCorp Vault 企业级密钥管理深度实践'
category: cloud-native-security
tags:
- k8s
- security
- cloud-native
- falco
- opa
- prometheus
- postgresql
- job
- cronjob
- ingress
tier: supporting
created: '2026-05-23'
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- 安全工程师
- SRE
- 架构师
estimated_read_time: 5min
intent_queries:
- HashiCorp Vault 企业级密钥管理深度实践 是什么
- 如何 HashiCorp Vault 企业级密钥管理深度实践
- Kubernetes 25 cloud native security 最佳实践
trigger_keywords:
- HashiCorp
- Vault
- 企业级密钥管理深度实践
- cloud
- native
- security
prerequisites:
- kubectl-basics
- rbac-basics
- prometheus-basics
- tls-basics
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
---



# HashiCorp Vault 企业级密钥管理深度实践

> **Author**: Enterprise Security Architect | **Version**: v1.0 | **Update Time**: 2026-05-18
> **Scenario**: Enterprise-grade [[Secrets|secrets]]ts Management|secrets management]] and cryptographic operations | **Complexity**: ⭐⭐⭐⭐⭐

<!-- chunk: 概述 -->## 概述

HashiCorp Vault 是企业级密钥管理和加密服务平台，提供集中式的密钥存储、动态凭证生成、加密即服务和完整的审计追踪能力。在云原生环境中，应用程序需要管理数据库凭证、API 密钥、TLS 证书、加密密钥等多种敏感数据，Vault 通过统一的 API 和策略引擎为这些需求提供安全、可审计的解决方案。本文详细探讨 Vault 企业级部署架构、多认证方式集成、密钥引擎使用、加密服务和运维管理，帮助企业在生产环境中构建安全、高可用的密钥管理平台。

## 威胁模型分析

**密钥泄露**：开发人员将数据库密码、API Key 等硬编码在代码或配置文件中，通过代码仓库泄露。更常见的是将密钥以明文形式存储在环境变量或 ConfigMap 中，集群内的任何 Pod 都可能读取。Vault 通过动态凭证和 Agent Sidecar 注入模式，确保密钥仅在内存中存在，不落盘不暴露。

**密钥轮换缺失**：长期使用同一组密钥增加了被破解的风险。当密钥泄露时，需要手动更新所有使用该密钥的服务。Vault 的动态密钥引擎可以自动生成短期凭证，并设置 TTL（Time To Live）和自动回收机制。

**缺乏审计追踪**：没有统一的密钥管理系统时，无法追踪谁在何时访问了哪些密钥。Vault 的审计日志记录所有 API 调用，包括认证、授权和密钥访问，为合规审计提供完整证据链。

**数据加密需求**：应用程序需要对用户敏感数据（如 SSN、信用卡号）进行加密存储。Vault 的 Transit 引擎提供加密即服务，应用无需自行管理加密密钥。

<!-- chunk: 架构设计 -->## 架构设计

## Vault 企业级组件架构

```mermaid
graph TB
    subgraph "Vault Core"
        SERVER[Vault Server]
        STORAGE[Storage Backend<br/>Raft Consensus]
        SEAL[Seal / Unseal<br/>Auto Unseal KMS]
        TOKEN[Token Manager]
        POLICY[Policy Engine]
    end

    subgraph "Enterprise Features"
        NS[Namespaces]
        MFA[MFA Authentication]
        PREF[Performance Replication]
        DR[DR Replication]
        HSM[HSM Integration]
    end

    subgraph "Auth Methods"
        LDAP[LDAP / Active Directory]
        OIDC[OIDC / OAuth2]
        AWS[AWS IAM Auth]
        K8S[Kubernetes Auth]
        APPROLE[AppRole]
        USERPASS[Username / Password]
        JWT[JWT Auth]
    end

    subgraph "Secrets Engines"
        KV[KV v2 Static Secrets]
        DYNDB[Dynamic Database]
        TRANSIT[Transit Encryption]
        PKI[PKI Certificates]
        SSH[SSH OTP]
        TOTP[TOTP MFA]
        AWSSEC[AWS Credentials]
        GCPSEC[GCP Credentials]
    end

    subgraph "Infrastructure"
        LB[Load Balancer]
        AUDIT[Audit Logging]
        MON[Monitoring]
        BACKUP[Backup / Snapshot]
    end

    SERVER --> STORAGE
    SERVER --> SEAL
    SERVER --> TOKEN
    TOKEN --> POLICY
    POLICY --> NS
    SERVER --> MFA
    SERVER --> PREF
    SERVER --> DR
    SERVER --> HSM

    LDAP --> SERVER
    OIDC --> SERVER
    AWS --> SERVER
    K8S --> SERVER
    APPROLE --> SERVER

    KV --> SERVER
    DYNDB --> SERVER
    TRANSIT --> SERVER
    PKI --> SERVER

    LB --> SERVER
    SERVER --> AUDIT
    SERVER --> MON
    STORAGE --> BACKUP
```

## 企业级部署架构

```yaml
vault_enterprise_architecture:
  cluster:
    node_count: 5
    regions: 2
    server_config:
      api_addr: "https://vault.company.com:8200"
      cluster_addr: "https://vault-node-{index}.internal.company.com:8201"
      ui: true
      disable_mlock: false

    listener:
      - tcp:
          address: "0.0.0.0:8200"
          cluster_address: "0.0.0.0:8201"
          tls_cert_file: "/etc/vault/tls/vault.crt"
          tls_key_file: "/etc/vault/tls/vault.key"
          tls_client_ca_file: "/etc/vault/tls/ca.crt"
          telemetry:
            unauthenticated_metrics_access: true

    storage:
      raft:
        path: "/opt/vault/data"
        performance_multiplier: 8
        trailing_logs: 10000
        snapshot_threshold: 8192

    seal:
      awskms:
        region: "us-west-2"
        kms_key_id: "arn:aws:kms:us-west-2:123456789012:key/abcd1234"

    telemetry:
      statsd_address: "statsd.monitoring.svc.cluster.local:8125"
      prometheus_retention_time: "30s"
      disable_hostname: true

  high_availability:
    load_balancer:
      type: "aws_network_load_balancer"
      health_check:
        path: "/v1/sys/health"
        healthy_threshold: 2
        unhealthy_threshold: 3

    auto_unseal:
      provider: "awskms"
      region: "us-west-2"

    replication:
      performance:
        primary: "https://vault.company.com:8201"
        secondary: "https://vault-dr.company.com:8201"
      disaster_recovery:
        primary: "https://vault.company.com:8201"
        secondary: "https://vault-backup.company.com:8201"
```

<!-- chunk: 核心配置 -->## 核心配置

## LDAP / Active Directory 认证

```bash
#!/bin/bash
# vault_ldap_setup.sh

# Enable LDAP auth method
vault auth enable ldap

# Configure LDAP connection
vault write auth/ldap/config \
    url="ldaps://ldap.company.com:636" \
    binddn="cn=vault,ou=service accounts,dc=company,dc=com" \
    bindpass="${LDAP_BIND_PASSWORD}" \
    userdn="ou=users,dc=company,dc=com" \
    userattr="sAMAccountName" \
    groupdn="ou=groups,dc=company,dc=com" \
    groupfilter="(&(objectClass=group)(member:1.2.840.113556.1.4.1941:={{.UserDN}}))" \
    groupattr="cn" \
    insecure_tls=false \
    starttls=true \
    certificate=@/etc/vault/ssl/ldap-ca.crt \
    token_type="default" \
    max_page_size="1000"

# Create LDAP group to policy mappings
vault write auth/ldap/groups/platform-engineering \
    policies="platform-admin,kubernetes-admin,secret-reader"

vault write auth/ldap/groups/security-team \
    policies="security-admin,audit-reader,pki-admin"

vault write auth/ldap/groups/development-team \
    policies="developer,secret-reader"

vault write auth/ldap/groups/dba-team \
    policies="database-admin,secret-reader"

# Configure user-specific overrides
vault write auth/ldap/users/john.doe \
    policies="developer,project-alpha" \
    groups="development-team"

vault write auth/ldap/users/admin.user \
    policies="admin,security-admin" \
    groups="security-team"
```

## Kubernetes 认证

```yaml
apiVersion: v1
kind: ServiceAccount
metadata:
  name: vault-auth
  namespace: vault
---
apiVersion: rbac.authorization.k8s.io/v1
kind: ClusterRoleBinding
metadata:
  name: vault-auth-rolebinding
roleRef:
  apiGroup: rbac.authorization.k8s.io
  kind: ClusterRole
  name: system:auth-delegator
subjects:
  - kind: ServiceAccount
    name: vault-auth
    namespace: vault
```

```bash
#!/bin/bash
# vault_k8s_auth.sh

# Enable Kubernetes auth method
vault auth enable kubernetes

# Configure Kubernetes auth
vault write auth/kubernetes/config \
    kubernetes_host="https://kubernetes.default.svc.cluster.local" \
    kubernetes_ca_cert=@/var/run/secrets/kubernetes.io/serviceaccount/ca.crt \
    token_reviewer_jwt=$(cat /var/run/secrets/kubernetes.io/serviceaccount/token) \
    issuer="https://kubernetes.default.svc.cluster.local"

# Create roles for different service accounts
vault write auth/kubernetes/role/frontend-app \
    bound_service_account_names="frontend-sa" \
    bound_service_account_namespaces="production" \
    policies="frontend-policy" \
    ttl="1h" \
    max_ttl="24h"

vault write auth/kubernetes/role/backend-app \
    bound_service_account_names="backend-sa" \
    bound_service_account_namespaces="production" \
    policies="backend-policy,database-reader" \
    ttl="1h" \
    max_ttl="24h"

vault write auth/kubernetes/role/cronjob-app \
    bound_service_account_names="cronjob-sa" \
    bound_service_account_namespaces="production" \
    policies="database-reader,secret-reader" \
    ttl="30m" \
    max_ttl="1h"

# Create policies for K8s workloads
cat > frontend-policy.hcl << 'EOF'
path "secret/data/production/frontend" {
    capabilities = ["read"]
}
path "pki/issue/frontend" {
    capabilities = ["update"]
}
EOF

cat > backend-policy.hcl << 'EOF'
path "secret/data/production/backend" {
    capabilities = ["read"]
}
path "database/creds/backend-role" {
    capabilities = ["read"]
}
path "transit/encrypt/backend-key" {
    capabilities = ["update"]
}
path "transit/decrypt/backend-key" {
    capabilities = ["update"]
}
path "pki/issue/backend" {
    capabilities = ["update"]
}
EOF

cat > database-reader.hcl << 'EOF'
path "database/creds/read-only" {
    capabilities = ["read"]
}
path "database/creds/app-role" {
    capabilities = ["read"]
}
EOF

vault policy write frontend-policy frontend-policy.hcl
vault policy write backend-policy backend-policy.hcl
vault policy write database-reader database-reader.hcl
```

## AppRole 认证（机器对机器）

```bash
#!/bin/bash
# vault_approle_setup.sh

# Enable AppRole auth
vault auth enable approle

# Create policy for CI/CD pipeline
cat > cicd-policy.hcl << 'EOF'
path "secret/data/cicd/*" {
    capabilities = ["read"]
}
path "database/creds/cicd-role" {
    capabilities = ["read"]
}
path "pki/issue/cicd" {
    capabilities = ["update"]
}
path "transit/encrypt/cicd-key" {
    capabilities = ["update"]
}
EOF

vault policy write cicd-policy cicd-policy.hcl

# Create AppRole
vault write auth/approle/role/cicd-pipeline \
    token_policies="cicd-policy" \
    token_ttl="1h" \
    token_max_ttl="4h" \
    secret_id_bound_cidrs="10.0.0.0/8,172.16.0.0/12" \
    token_bound_cidrs="10.0.0.0/8,172.16.0.0/12" \
    secret_id_num_uses=100

# Get Role ID
ROLE_ID=$(vault read -field=role_id auth/approle/role/cicd-pipeline/role-id)
echo "Role ID: $ROLE_ID"

# Generate Secret ID
SECRET_ID=$(vault write -field=secret_id -f auth/approle/role/cicd-pipeline/secret-id)
echo "Secret ID: $SECRET_ID"

# Login with AppRole
vault write auth/approle/login \
    role_id="$ROLE_ID" \
    secret_id="$SECRET_ID"
```

## KV Secrets Engine

```bash
#!/bin/bash
# kv_secrets_management.sh

# Enable KV v2 engine
vault secrets enable -path=secret kv-v2

# Configure versioning and max versions
vault kv tune -max-versions=20 secret/

# Create application secrets
vault kv put secret/production/database \
    username="appuser" \
    password="$(openssl rand -base64 24)" \
    host="postgres.production.svc.cluster.local" \
    port="5432" \
    database="myapp"

vault kv put secret/production/api-keys \
    stripe_key="sk_live_abc123" \
    sendgrid_key="SG.def456" \
    google_maps_key="AIza ghi789"

vault kv put secret/production/jwt \
    secret_key="$(openssl rand -hex 64)" \
    issuer="myapp.company.com" \
    access_token_ttl="15m" \
    refresh_token_ttl="7d"

# Read secrets
vault kv get -format=json secret/production/database
vault kv get -field=password secret/production/database

# Version management
vault kv metadata get secret/production/database
vault kv rollback -version=2 secret/production/database

# Delete and undelete
vault kv delete secret/production/api-keys
vault kv undelete -versions=1 secret/production/api-keys

# Permanently destroy
vault kv destroy -versions=3 secret/production/api-keys
```

## Dynamic Database Credentials

```bash
#!/bin/bash
# dynamic_database_secrets.sh

# Enable database engine
vault secrets enable database

# Configure PostgreSQL connection
vault write database/config/production-postgres \
    plugin_name="postgresql-database-plugin" \
    allowed_roles="app-role,read-only,admin-role" \
    connection_url="postgresql://{{username}}:{{password}}@postgres.production.svc.cluster.local:5432/myapp?sslmode=require" \
    username="vault_admin" \
    password="${VAULT_DB_ADMIN_PASSWORD}" \
    max_open_connections=10 \
    max_idle_connections=5 \
    max_connection_lifetime="0"

# Rotate the admin password (one-time operation)
vault write -force database/rotate-root/production-postgres

# Create application role (read/write)
vault write database/roles/app-role \
    db_name="production-postgres" \
    creation_statements="CREATE ROLE \"{{name}}\" WITH LOGIN PASSWORD '{{password}}' VALID UNTIL '{{expiration}}'; GRANT SELECT, INSERT, UPDATE, DELETE ON ALL TABLES IN SCHEMA public TO \"{{name}}\"; GRANT USAGE, SELECT ON ALL SEQUENCES IN SCHEMA public TO \"{{name}}\";" \
    revocation_statements="DROP ROLE IF EXISTS \"{{name}}\";" \
    renew_statements="ALTER ROLE \"{{name}}\" VALID UNTIL '{{expiration}}';" \
    rollback_statements="DROP ROLE IF EXISTS \"{{name}}\";" \
    default_ttl="1h" \
    max_ttl="24h"

# Create read-only role
vault write database/roles/read-only \
    db_name="production-postgres" \
    creation_statements="CREATE ROLE \"{{name}}\" WITH LOGIN PASSWORD '{{password}}' VALID UNTIL '{{expiration}}'; GRANT SELECT ON ALL TABLES IN SCHEMA public TO \"{{name}}\";" \
    default_ttl="1h" \
    max_ttl="24h"

# Create admin role (short TTL)
vault write database/roles/admin-role \
    db_name="production-postgres" \
    creation_statements="CREATE ROLE \"{{name}}\" WITH LOGIN PASSWORD '{{password}}' SUPERUSER VALID UNTIL '{{expiration}}';" \
    default_ttl="30m" \
    max_ttl="1h"

# Test dynamic credential generation
vault read database/creds/app-role
vault read database/creds/read-only
```

<!-- chunk: 安全策略实战 -->## 安全策略实战

## Transit 加密即服务

Transit 引擎提供加密即服务（Encryption as a Service），应用程序无需自行管理加密密钥。密钥存储在 Vault 中，永远不暴露给应用。适合加密用户敏感数据如 SSN、信用卡号、健康记录等。

```bash
#!/bin/bash
# transit_encryption.sh

# Enable Transit engine
vault secrets enable transit

# Create encryption keys for different data types
vault write -f transit/keys/customer-pii
vault write -f transit/keys/payment-data
vault write -f transit/keys/health-records
vault write -f transit/keys/session-tokens

# Configure key policies
vault write transit/keys/customer-pii/config \
    exportable=false \
    allow_plaintext_backup=false \
    deletion_allowed=false \
    min_decryption_version=1 \
    min_encryption_version=1 \
    auto_rotate_period="720h"

vault write transit/keys/payment-data/config \
    exportable=false \
    allow_plaintext_backup=false \
    deletion_allowed=false \
    auto_rotate_period="168h"

# Create encryption policy
cat > transit-encrypt-policy.hcl << 'EOF'
path "transit/encrypt/customer-pii" {
    capabilities = ["update"]
}
path "transit/decrypt/customer-pii" {
    capabilities = ["update"]
}
path "transit/encrypt/payment-data" {
    capabilities = ["update"]
}
path "transit/decrypt/payment-data" {
    capabilities = ["update"]
}
path "transit/rewrap/*" {
    capabilities = ["update"]
}
EOF

vault policy write transit-app transit-encrypt-policy.hcl

# Rotate encryption key
vault write -f transit/keys/customer-pii/rotate

# Rewrap existing ciphertext with new key version
vault write transit/rewrap/customer-pii \
    ciphertext="vault:v1:abc123..."
```

## PKI 证书管理

```bash
#!/bin/bash
# pki_management.sh

# === Root CA Setup ===
vault secrets enable -path=pki-root pki
vault secrets tune -max-lease-ttl=87600h pki-root

vault write pki-root/root/generate/internal \
    common_name="Company Root CA" \
    ttl=87600h \
    key_type="rsa" \
    key_bits=4096 \
    exclude_cn_from_sans=true \
    issuer_name="root-2026"

vault write pki-root/config/urls \
    issuing_certificates="https://vault.company.com:8200/v1/pki-root/ca" \
    crl_distribution_points="https://vault.company.com:8200/v1/pki-root/crl" \
    ocsp_servers="https://vault.company.com:8200/v1/pki-root/ocsp"

# === Intermediate CA Setup ===
vault secrets enable -path=pki-intermediate pki
vault secrets tune -max-lease-ttl=43800h pki-intermediate

vault write pki-intermediate/intermediate/generate/internal \
    common_name="Company Intermediate CA" \
    ttl=43800h \
    key_type="ec" \
    key_bits=256 \
    issuer_name="intermediate-2026"

CSR=$(vault write -field=csr pki-intermediate/intermediate/generate/internal)

vault write pki-root/root/sign-intermediate \
    csr=@<(echo "$CSR") \
    format=pem_bundle \
    ttl=43800h

vault write pki-intermediate/intermediate/set-signed \
    certificate=@signed_intermediate.crt

# === Issue Server Certificates ===
vault write pki-intermediate/roles/server-certs \
    allowed_domains="company.com,internal.company.com,svc.cluster.local" \
    allow_subdomains=true \
    allow_bare_domains=false \
    max_ttl="720h" \
    key_type="ec" \
    key_bits=256 \
    server_flag=true \
    client_flag=false \
    no_store=false

vault write pki-intermediate/roles/client-certs \
    allowed_domains="company.com" \
    allow_bare_domains=true \
    allow_subdomains=true \
    max_ttl="168h" \
    client_flag=true \
    server_flag=false

# Issue a certificate
vault write pki-intermediate/issue/server-certs \
    common_name="webapp.company.com" \
    ttl="24h" \
    alt_names="webapp.production.svc.cluster.local" \
    ip_sans="10.0.1.100"

# === Certificate Revocation ===
vault write pki-intermediate/revoke \
    serial_number="12:34:56:78:90:ab:cd:ef"

# === Tidy / Cleanup ===
vault write pki-intermediate/tidy \
    tidy_cert_store=true \
    tidy_revoked_certs=true \
    safety_buffer="72h"
```

## 命名空间隔离

```bash
#!/bin/bash
# namespace_setup.sh

# Create namespaces for different environments
vault namespace create production
vault namespace create staging
vault namespace create development
vault namespace create security

# Configure secrets engines in each namespace
vault secrets enable -namespace=production -path=secret kv-v2
vault secrets enable -namespace=production -path=database database
vault secrets enable -namespace=production -path=transit transit

vault secrets enable -namespace=staging -path=secret kv-v2
vault secrets enable -namespace=staging -path=database database

vault secrets enable -namespace=development -path=secret kv-v2

# Create namespace-specific policies
cat > prod-admin-policy.hcl << 'EOF'
path "secret/data/*" {
    capabilities = ["create", "read", "update", "delete", "list"]
}
path "database/creds/*" {
    capabilities = ["read"]
}
path "transit/*" {
    capabilities = ["update"]
}
path "pki/issue/*" {
    capabilities = ["update"]
}
EOF

vault policy write -namespace=production prod-admin prod-admin-policy.hcl

cat > dev-admin-policy.hcl << 'EOF'
path "secret/data/*" {
    capabilities = ["create", "read", "update", "delete", "list"]
}
path "database/creds/*" {
    capabilities = ["read"]
}
EOF

vault policy write -namespace=development dev-admin dev-admin-policy.hcl

# Configure auth per namespace
vault auth enable -namespace=production userpass
vault auth enable -namespace=production kubernetes

vault write -namespace=production auth/userpass/users/prod-admin \
    password="${PROD_ADMIN_PASSWORD}" \
    policies="prod-admin"

vault auth enable -namespace=development userpass
vault write -namespace=development auth/userpass/users/dev-admin \
    password="${DEV_ADMIN_PASSWORD}" \
    policies="dev-admin"
```

<!-- chunk: 合规与审计 -->## 合规与审计

## 审计日志配置

```bash
# Enable file audit
vault audit enable file file_path=/vault/logs/audit.log log_raw=false

# Enable syslog audit
vault audit enable syslog \
    facility="AUTH" \
    tag="vault" \
    address="syslog.monitoring.svc.cluster.local:514"

# Enable socket audit (for external SIEM)
vault audit enable socket \
    address="siem.company.com:9000" \
    socket_type="tcp" \
    format="json"
```

## 审计日志分析

```bash
#!/bin/bash
# vault_audit_analysis.sh

AUDIT_LOG="/var/log/vault/audit.log"

echo "=== Vault Audit Analysis Report ==="
echo "Date: $(date)"
echo ""

echo "1. Top 10 Most Active Users"
jq -r '.auth.accessor // "anonymous"' "$AUDIT_LOG" | sort | uniq -c | sort -rn | head -10
echo ""

echo "2. Failed Authentication Attempts"
jq 'select(.type == "request" and .request.path | test("login")) |
    select(.response.auth == null or .response.auth.lease_duration == 0)' \
  "$AUDIT_LOG" | \
  jq -r '"\(.time) | \(.request.path) | \(.request.data | keys)"'
echo ""

echo "3. Secret Access by Path"
jq -r 'select(.type == "request") | .request.path' "$AUDIT_LOG" | \
  grep -E "^secret/" | sort | uniq -c | sort -rn | head -20
echo ""

echo "4. Database Dynamic Credentials Generated"
jq 'select(.request.path | startswith("database/creds/"))' "$AUDIT_LOG" | \
  jq -r '"\(.time) | \(.auth.accessor) | \(.request.path)"' | sort | uniq -c | sort -rn
echo ""

echo "5. Policy Changes"
jq 'select(.request.path | test("sys/policies"))' "$AUDIT_LOG" | \
  jq -r '"\(.time) | \(.request.operation) | \(.request.path)"'
echo ""

echo "6. Root Token Usage (should be minimal)"
jq 'select(.auth.policies | contains(["root"]))' "$AUDIT_LOG" | \
  jq -r '"\(.time) | \(.request.operation) | \(.request.path)"' | head -20
echo ""

echo "7. PKI Certificate Issuance"
jq 'select(.request.path | test("pki.*issue"))' "$AUDIT_LOG" | \
  jq -r '"\(.time) | \(.auth.accessor) | \(.request.data.common_name // "N/A")"'
echo ""

echo "8. Encryption/Decryption Operations"
jq 'select(.request.path | test("transit/(encrypt|decrypt)"))' "$AUDIT_LOG" | \
  jq -r '"\(.time) | \(.request.path) | \(.auth.accessor)"' | sort | uniq -c | sort -rn
```

<!-- chunk: 监控与告警 -->## 监控与告警

## Prometheus 监控

```bash
# Configure Vault telemetry in server config
# listener "tcp" {
#   telemetry { unauthenticated_metrics_access = true }
# }
# telemetry {
#   prometheus_retention_time = "30s"
#   disable_hostname = true
# }
```

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
            summary: "Vault instance {{ $labels.instance }} is sealed"

        - alert: VaultRaftQuorumLost
          expr: vault.raft.state == 0
          for: 1m
          labels:
            severity: critical
          annotations:
            summary: "Vault Raft cluster lost quorum"

        - alert: VaultHighTokenCreation
          expr: rate(vault.token.creation[5m]) > 100
          for: 5m
          labels:
            severity: warning
          annotations:
            summary: "High token creation rate: {{ $value }}/s"

        - alert: VaultLeaseCreationError
          expr: rate(vault.core.lease.creation_error[5m]) > 0
          for: 2m
          labels:
            severity: warning
          annotations:
            summary: "Lease creation errors detected"

        - alert: VaultAuditLoggingFailed
          expr: rate(vault.audit.log.request.failure[5m]) > 0
          for: 2m
          labels:
            severity: critical
          annotations:
            summary: "Audit logging failure detected"

        - alert: VaultAutoUnsealFailed
          expr: vault.core.unsealed == 0 and vault.core.sealed == 1
          for: 5m
          labels:
            severity: critical
          annotations:
            summary: "Auto-unseal failed for {{ $labels.instance }}"

        - alert: VaultReplicationIngressWALHung
          expr: vault.replication.performance.ingress_wal != 0
          for: 10m
          labels:
            severity: warning
          annotations:
            summary: "Performance replication WAL is hung"
```

<!-- chunk: 最佳实践 -->## 最佳实践

## 最小权限原则

为每个应用创建独立的 Vault 策略，仅授予必要的路径和操作权限。使用命名空间隔离不同环境的密钥。定期审查策略和角色配置，删除不再使用的权限。

## 密钥自动轮换

使用动态密钥引擎为数据库和云服务生成短期凭证。对于静态密钥，使用 External Secrets Operator 定期同步。加密密钥通过 Transit 引擎的 `auto_rotate_period` 自动轮换。

## 备份与灾难恢复

```bash
#!/bin/bash
# vault_backup.sh

BACKUP_DIR="/backup/vault"
DATE=$(date +%Y%m%d_%H%M%S)
mkdir -p "$BACKUP_DIR/$DATE"

# Create Raft snapshot
vault operator raft snapshot save "$BACKUP_DIR/$DATE/vault-snapshot.snap"

# Backup configuration
cp -r /etc/vault "$BACKUP_DIR/$DATE/config"

# Create manifest
cat > "$BACKUP_DIR/$DATE/manifest.json" << EOF
{
    "backup_id": "$DATE",
    "timestamp": "$(date -Iseconds)",
    "vault_version": "$(vault version | awk '{print $2}')",
    "files": {
        "snapshot": "vault-snapshot.snap",
        "config": "config/"
    }
}
EOF

# Compress and upload to S3
tar czf "$BACKUP_DIR/$DATE.tar.gz" -C "$BACKUP_DIR" "$DATE"
aws s3 cp "$BACKUP_DIR/$DATE.tar.gz" s3://company-vault-backups/

# Cleanup local backups older than 30 days
find "$BACKUP_DIR" -type d -mtime +30 -exec rm -rf {} \;
```

<!-- chunk: 故障排查 -->## 故障排查

## 常见问题

**Vault Sealed**：检查 Auto Unseal KMS 配置。确认 KMS 密钥存在且有权限。查看 Vault 日志中的 unseal 错误信息。手动使用 unseal key 解封。

**Raft 集群不一致**：使用 `vault operator raft list-peers` 检查集群成员。检查网络连接和 TLS 证书。使用 `vault operator raft remove-peer` 移除问题节点后重新加入。

**认证失败**：检查 LDAP/K8s 配置是否正确。验证 ServiceAccount Token 是否有效。确认角色绑定的策略存在。

**性能问题**：增大 `performance_multiplier` 参数。检查存储后端 I/O 性能。使用 Prometheus 监控 `vault.core.handle_request` 延迟指标。

```bash
#!/bin/bash
# vault_diagnostics.sh

echo "=== Vault Status ==="
vault status
echo ""

echo "=== Raft Cluster Peers ==="
vault operator raft list-peers
echo ""

echo "=== Raft Cluster Configuration ==="
vault operator raft configuration
echo ""

echo "=== Auth Methods ==="
vault auth list
echo ""

echo "=== Secrets Engines ==="
vault secrets list
echo ""

echo "=== Active Tokens Count ==="
vault list auth/token/accessors | wc -l
echo ""

echo "=== Lease Count ==="
vault list sys/leases | head -20
echo ""

echo "=== Namespace List ==="
vault namespace list
echo ""

echo "=== Replication Status ==="
vault read -format=json sys/replication/performance/status
vault read -format=json sys/replication/dr/status
echo ""

echo "=== Audit Devices ==="
vault audit list -detailed
```

---

*本文档基于企业级 HashiCorp Vault 密钥管理实践经验编写，持续更新最新技术和最佳实践。*

---

<!-- chunk: Obsidian 相关文档 -->## Obsidian 相关文档

- domain-05-security-compliance MOC
- [[domain-05-security-compliance/README.md|Domain 05: 云原生安全 (Cloud Native Security)]]
- [[domain-05-security-compliance/00-open-source-projects-index.md|Domain-25 云原生安全 — 开源项目索引]]
- Falco 云原生安全监控深度实践
- Sysdig企业级容器安全深度实践
- Aqua Security 企业级容器安全平台深度实践
- Kyverno 企业级策略管理深度实践
- OPA Gatekeeper 策略即代码深度实践
- 容器镜像安全扫描深度实践
- Kubernetes 安全加固深度实践
- gVisor 容器沙箱深度解析
- cert-manager 自动证书管理深度实践

## See Also

- 03-aqua-enterprise-container-security
- 04-kyverno-enterprise-policy-management
- 09-opa-gatekeeper-policy
- 10-image-security-scanning

- [[domain-05-security-compliance/README.md|返回目录]]

## Related

- [[domain-19-landscape-references/topic-index/cert-index.md|Certificate / TLS 证书知识图谱索引]]
- [[domain-19-landscape-references/topic-index/security-index.md|Security 安全知识图谱索引]]
