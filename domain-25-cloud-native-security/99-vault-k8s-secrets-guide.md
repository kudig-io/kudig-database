# Vault K8s 密钥管理集成指南

> **适用版本**: Vault v1.19.0 / Vault Agent Injector v1.19.0  
> **最后更新**: 2026-04-24  
> **难度**: 中级 → 高级

---

## 📋 目录

- [一、架构模式选择](#一架构模式选择)
- [二、Helm 部署](#二helm-部署)
- [三、K8s 认证方法](#三k8s-认证方法)
- [四、Vault Agent Injector (推荐)](#四vault-agent-injector-推荐)
- [五、External Secrets Operator (开源替代)](#五external-secrets-operator-开源替代)
- [六、PKI 引擎与自动 TLS](#六pki-引擎与自动-tls)
- [七、动态数据库凭证](#七动态数据库凭证)
- [八、监控与审计](#八监控与审计)

---

## 一、架构模式选择

```
模式 A: Vault Agent Sidecar (推荐)
Pod
├── App Container ──► 从共享卷读取 /vault/secrets/config.json
└── Vault Agent Sidecar ──► 自动认证、续租、模板渲染
    └── 将密钥注入共享 emptyDir 卷

模式 B: External Secrets Operator
External Secrets Controller ──► 同步 Vault 密钥 → K8s Secret
Pod ──► 引用标准 K8s Secret

模式 C: CSI Driver
Vault CSI Driver ──► Pod 通过 CSI 卷挂载密钥
```

| 模式 | 安全性 | 实时性 | 复杂度 | 推荐场景 |
|:---|:---|:---|:---|:---|
| Agent Sidecar | 高 (内存不落盘) | 实时续租 | 中 | 生产环境首选 |
| External Secrets | 中 (Secret 存在 etcd) | 定期同步 | 低 | 简单迁移、GitOps |
| CSI Driver | 高 | 实时 | 高 | 特殊合规要求 |

---

## 二、Helm 部署

```bash
helm repo add hashicorp https://helm.releases.hashicorp.com

# 开发/测试 (Dev mode - 不安全，仅测试)
helm install vault hashicorp/vault \
  --set "server.dev.enabled=true"

# 生产环境 (HA with Raft)
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

### 生产级 values

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
        }
        storage "raft" {
          path = "/vault/data"
        }
        service_registration "kubernetes" {}
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
      cpu: 1000m

  volumes:
    - name: vault-tls
      secret:
        secretName: vault-tls

  volumeMounts:
    - mountPath: /vault/userconfig/vault-tls
      name: vault-tls
      readOnly: true

injector:
  enabled: true
  metrics:
    enabled: true
```

---

## 三、K8s 认证方法

### 3.1 启用 K8s Auth 后端

```bash
# 初始化并解封 Vault
kubectl exec -it vault-0 -- vault operator init -key-shares=5 -key-threshold=3
# 保存 Unseal Keys 和 Root Token

kubectl exec -it vault-0 -- vault operator unseal <UNSEAL_KEY_1>
kubectl exec -it vault-0 -- vault operator unseal <UNSEAL_KEY_2>
kubectl exec -it vault-0 -- vault operator unseal <UNSEAL_KEY_3>

# 启用 K8s 认证
kubectl exec -it vault-0 -- vault auth enable kubernetes

# 配置 K8s 认证
kubectl exec -it vault-0 -- vault write auth/kubernetes/config \
  token_reviewer_jwt="$(cat /var/run/secrets/kubernetes.io/serviceaccount/token)" \
  kubernetes_host="https://$KUBERNETES_PORT_443_TCP_ADDR:443" \
  kubernetes_ca_cert="@/var/run/secrets/kubernetes.io/serviceaccount/ca.crt" \
  issuer="https://kubernetes.default.svc.cluster.local"
```

### 3.2 创建策略与角色

```bash
# 创建策略
cat > app-policy.hcl <<EOF
path "secret/data/myapp/*" {
  capabilities = ["read"]
}
path "database/creds/myapp" {
  capabilities = ["read"]
}
EOF

kubectl cp app-policy.hcl vault-0:/tmp/app-policy.hcl
kubectl exec -it vault-0 -- vault policy write myapp /tmp/app-policy.hcl

# 创建 K8s 角色
kubectl exec -it vault-0 -- vault write auth/kubernetes/role/myapp \
  bound_service_account_names=myapp-sa \
  bound_service_account_namespaces=production \
  policies=myapp \
  ttl=1h
```

---

## 四、Vault Agent Injector (推荐)

### 4.1 部署带注入的 Pod

```yaml
apiVersion: v1
kind: ServiceAccount
metadata:
  name: myapp-sa
  namespace: production
---
apiVersion: apps/v1
kind: Deployment
metadata:
  name: myapp
  namespace: production
  annotations:
    vault.hashicorp.com/agent-inject: "true"
    vault.hashicorp.com/role: "myapp"
    vault.hashicorp.com/agent-pre-populate: "true"
    vault.hashicorp.com/agent-pre-populate-only: "false"
    vault.hashicorp.com/agent-run-as-user: "65534"
    
    # 注入数据库凭证
    vault.hashicorp.com/agent-inject-secret-db-creds: "database/creds/myapp"
    vault.hashicorp.com/agent-inject-template-db-creds: |
      {{ with secret "database/creds/myapp" -}}
      export DB_USER="{{ .Data.username }}"
      export DB_PASS="{{ .Data.password }}"
      {{- end }}
    
    # 注入应用配置
    vault.hashicorp.com/agent-inject-secret-config: "secret/data/myapp/config"
    vault.hashicorp.com/agent-inject-template-config: |
      {{ with secret "secret/data/myapp/config" -}}
      {
        "api_key": "{{ .Data.data.api_key }}",
        "stripe_key": "{{ .Data.data.stripe_key }}"
      }
      {{- end }}
    
    # 模板输出格式
    vault.hashicorp.com/secret-volume-path: "/vault/secrets"
spec:
  replicas: 3
  selector:
    matchLabels:
      app: myapp
  template:
    metadata:
      labels:
        app: myapp
    spec:
      serviceAccountName: myapp-sa
      containers:
      - name: myapp
        image: myapp:v1.0.0
        env:
        - name: DB_CREDS_FILE
          value: "/vault/secrets/db-creds"
        - name: CONFIG_FILE
          value: "/vault/secrets/config"
        volumeMounts:
        - name: vault-secrets
          mountPath: /vault/secrets
      volumes:
      - name: vault-secrets
        emptyDir:
          medium: Memory  # 内存卷，不落盘
```

### 4.2 应用读取注入的密钥

```python
# Python 示例: 读取注入的配置
import json
import os

config_path = os.environ.get('CONFIG_FILE', '/vault/secrets/config')
with open(config_path) as f:
    config = json.load(f)

api_key = config['api_key']
stripe_key = config['stripe_key']

# 数据库凭证 (通过 source 文件)
import subprocess
env_vars = subprocess.check_output(['cat', os.environ['DB_CREDS_FILE']]).decode()
# 解析 export DB_USER=... DB_PASS=...
```

---

## 五、External Secrets Operator (开源替代)

```yaml
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
  data:
  - secretKey: api_key
    remoteRef:
      key: secret/data/myapp/config
      property: api_key
  - secretKey: stripe_key
    remoteRef:
      key: secret/data/myapp/config
      property: stripe_key
```

---

## 六、PKI 引擎与自动 TLS

```bash
# 启用 PKI
vault secrets enable pki
vault secrets tune -max-lease-ttl=8760h pki

# 生成根 CA
vault write pki/root/generate/internal \
  common_name="MyOrg Root CA" \
  ttl=8760h

# 创建角色
vault write pki/roles/myapp \
  allowed_domains="myapp.svc.cluster.local" \
  allow_subdomains=true \
  max_ttl=720h
```

```yaml
# Pod 注解自动获取 TLS 证书
annotations:
  vault.hashicorp.com/agent-inject: "true"
  vault.hashicorp.com/role: "myapp"
  vault.hashicorp.com/agent-inject-secret-tls: "pki/issue/myapp"
  vault.hashicorp.com/agent-inject-template-tls: |
    {{ with secret "pki/issue/myapp" "common_name=myapp.production.svc.cluster.local" "ttl=24h" -}}
    {{ .Data.certificate }}
    {{ .Data.ca_chain }}
    {{ .Data.private_key }}
    {{- end }}
  vault.hashicorp.com/agent-inject-secret-tls-key: "pki/issue/myapp"
  vault.hashicorp.com/agent-inject-template-tls-key: |
    {{ with secret "pki/issue/myapp" "common_name=myapp.production.svc.cluster.local" "ttl=24h" -}}
    {{ .Data.private_key }}
    {{- end }}
```

---

## 七、动态数据库凭证

```bash
# 启用数据库引擎
vault secrets enable database

# 配置 PostgreSQL 连接
vault write database/config/myapp-postgres \
  plugin_name=postgresql-database-plugin \
  allowed_roles="myapp" \
  connection_url="postgresql://{{username}}:{{password}}@postgres:5432/myapp" \
  username="vaultadmin" \
  password="vaultadmin-password"

# 创建动态角色
vault write database/roles/myapp \
  db_name=myapp-postgres \
  creation_statements="CREATE ROLE \"{{name}}\" WITH LOGIN PASSWORD '{{password}}' VALID UNTIL '{{expiration}}'; \
    GRANT SELECT ON ALL TABLES IN SCHEMA public TO \"{{name}}\";" \
  default_ttl=1h \
  max_ttl=24h
```

---

## 八、监控与审计

### 8.1 启用审计日志

```bash
vault audit enable file file_path=/vault/logs/audit.log
```

### 8.2 Prometheus 指标

```yaml
# values-vault-production.yaml
server:
  ha:
    config: |
      telemetry {
        prometheus_retention_time = "30s"
        disable_hostname = true
      }
```

### 8.3 关键告警

| 告警 | 表达式 |
|:---|:---|
| Vault 密封 | `vault_core_unsealed == 0` |
| 证书即将过期 | `vault_pki_cert_expiry - time() < 86400 * 7` |
| 认证失败率 | `rate(vault_audit_log_failure_count[5m]) > 0.1` |

---

## 参考链接

- [Vault 官方文档](https://developer.hashicorp.com/vault/docs)
- [Vault Agent Injector](https://developer.hashicorp.com/vault/docs/platform/k8s/injector)
- [External Secrets Operator](https://external-secrets.io/latest/)
- [Vault PKI 引擎](https://developer.hashicorp.com/vault/docs/secrets/pki)
- [Vault Database 动态凭证](https://developer.hashicorp.com/vault/docs/secrets/databases)
