# 配置与密钥管理工具

> **适用版本**: v1.25 - v1.32 | **最后更新**: 2026-01 | **参考**: [External Secrets](https://external-secrets.io/) | [Sealed Secrets](https://sealed-secrets.netlify.app/)

## 工具对比

| 工具 | 类型 | 安全性 | GitOps友好 | 多云支持 | 生产推荐 |
|------|------|--------|-----------|---------|---------|
| **External Secrets Operator** | 外部同步 | ⭐⭐⭐⭐⭐ | ⭐⭐⭐⭐⭐ | ⭐⭐⭐⭐⭐ | 强烈推荐 |
| **Sealed Secrets** | 加密存储 | ⭐⭐⭐⭐ | ⭐⭐⭐⭐⭐ | ⭐⭐⭐ | 推荐 |
| **HashiCorp Vault** | 企业密钥库 | ⭐⭐⭐⭐⭐ | ⭐⭐⭐⭐ | ⭐⭐⭐⭐⭐ | 大型企业 |
| **SOPS** | 文件加密 | ⭐⭐⭐⭐ | ⭐⭐⭐⭐⭐ | ⭐⭐⭐⭐ | 中小团队 |
| **Kubernetes原生Secret** | 内置 | ⭐⭐ | ⭐⭐ | N/A | 不推荐直接使用 |

---

## External Secrets Operator (ESO)

### 架构优势

```
┌──────────────────┐
│  Git Repository  │  (存储ExternalSecret CRD)
└────────┬─────────┘
         │
         v
┌─────────────────────────────────────┐
│    External Secrets Operator        │
│  ┌─────────────────────────────┐   │
│  │  SecretStore / ClusterStore │   │
│  └──────────┬──────────────────┘   │
└─────────────┼───────────────────────┘
              │
      ┌───────┴────────┐
      v                v
┌──────────┐    ┌──────────────┐
│  Vault   │    │ AWS Secrets  │
│  KV v2   │    │   Manager    │
└──────────┘    └──────────────┘
      │                │
      └────────┬───────┘
               v
        ┌────────────┐
        │ K8s Secret │
        └────────────┘
```

### SecretStore 配置 (Vault)

```yaml
apiVersion: external-secrets.io/v1beta1
kind: SecretStore
metadata:
  name: vault-backend
  namespace: production
spec:
  provider:
    vault:
      server: "https://vault.company.com"
      path: "secret"
      version: "v2"
      auth:
        kubernetes:
          mountPath: "kubernetes"
          role: "production-role"
          serviceAccountRef:
            name: external-secrets-sa
```

### ClusterSecretStore (全局配置)

```yaml
apiVersion: external-secrets.io/v1beta1
kind: ClusterSecretStore
metadata:
  name: aws-secretsmanager
spec:
  provider:
    aws:
      service: SecretsManager
      region: cn-north-1
      auth:
        jwt:
          serviceAccountRef:
            name: external-secrets-sa
            namespace: kube-system
```

### ExternalSecret 使用示例

```yaml
apiVersion: external-secrets.io/v1beta1
kind: ExternalSecret
metadata:
  name: database-credentials
  namespace: production
spec:
  refreshInterval: 1h  # 同步间隔
  secretStoreRef:
    name: vault-backend
    kind: SecretStore
  target:
    name: db-secret  # 生成的K8s Secret名称
    creationPolicy: Owner
    template:
      engineVersion: v2
      data:
        # 模板化生成
        connection-string: "postgresql://{{ .username }}:{{ .password }}@postgres:5432/mydb"
  data:
    - secretKey: username
      remoteRef:
        key: database/prod/credentials
        property: username
    - secretKey: password
      remoteRef:
        key: database/prod/credentials
        property: password
```

### 多源Secret聚合

```yaml
apiVersion: external-secrets.io/v1beta1
kind: ExternalSecret
metadata:
  name: app-config
spec:
  secretStoreRef:
    name: vault-backend
    kind: SecretStore
  target:
    name: app-secret
  dataFrom:
    # 从Vault拉取整个路径
    - extract:
        key: app/production/config
    # 从AWS拉取
    - extract:
        key: arn:aws:secretsmanager:cn-north-1:123456789:secret:prod-config
      rewrite:
        - regexp:
            source: "(.*)_AWS$"
            target: "$1"
```

### Helm 部署 ESO

```bash
helm repo add external-secrets https://charts.external-secrets.io
helm install external-secrets \
  external-secrets/external-secrets \
  -n external-secrets-system \
  --create-namespace \
  --set installCRDs=true \
  --set webhook.port=9443
```

---

## Sealed Secrets

### 工作原理

```
开发者本地                          集群内
┌──────────────┐                 ┌──────────────────┐
│ kubectl      │  加密公钥        │ Sealed Secrets   │
│ + kubeseal   │─────────────────>│   Controller     │
└──────────────┘                 └────────┬─────────┘
        │                                  │ 私钥解密
        │ 生成SealedSecret                 │
        v                                  v
┌──────────────┐  GitOps部署      ┌──────────────────┐
│     Git      │─────────────────>│   K8s Secret     │
└──────────────┘                 └──────────────────┘
```

### 安装 Sealed Secrets

```bash
# 安装Controller
kubectl apply -f https://github.com/bitnami-labs/sealed-secrets/releases/download/v0.24.0/controller.yaml

# 安装kubeseal CLI
wget https://github.com/bitnami-labs/sealed-secrets/releases/download/v0.24.0/kubeseal-linux-amd64
chmod +x kubeseal-linux-amd64
sudo mv kubeseal-linux-amd64 /usr/local/bin/kubeseal
```

### 加密Secret

```bash
# 创建原始Secret (不要直接apply)
kubectl create secret generic db-credentials \
  --from-literal=username=admin \
  --from-literal=password=SecureP@ss123 \
  --dry-run=client -o yaml > secret.yaml

# 使用kubeseal加密
kubeseal -f secret.yaml -w sealed-secret.yaml \
  --controller-name=sealed-secrets-controller \
  --controller-namespace=kube-system

# 安全提交到Git
git add sealed-secret.yaml
git commit -m "Add encrypted database credentials"
```

### SealedSecret YAML 示例

```yaml
apiVersion: bitnami.com/v1alpha1
kind: SealedSecret
metadata:
  name: db-credentials
  namespace: production
spec:
  encryptedData:
    username: AgBy3i4OHN8K...  # 加密后的数据
    password: AgAh8Gq2Jk9P...
  template:
    metadata:
      labels:
        app: myapp
    type: Opaque
```

### Scope 安全模式

```bash
# strict (默认): 命名空间+名称锁定
kubeseal -f secret.yaml --scope strict

# namespace-wide: 命名空间内可重命名
kubeseal -f secret.yaml --scope namespace-wide

# cluster-wide: 集群范围(不推荐生产)
kubeseal -f secret.yaml --scope cluster-wide
```

### 密钥轮换

```bash
# 导出当前密钥(用于灾备)
kubectl get secret -n kube-system \
  -l sealedsecrets.bitnami.com/sealed-secrets-key=active \
  -o yaml > sealed-secrets-key.yaml

# 手动轮换(生成新密钥对)
kubectl delete secret -n kube-system \
  -l sealedsecrets.bitnami.com/sealed-secrets-key=active
kubectl delete pod -n kube-system \
  -l name=sealed-secrets-controller
```

---

## HashiCorp Vault 集成

### Vault Agent Injector

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: myapp
spec:
  template:
    metadata:
      annotations:
        vault.hashicorp.com/agent-inject: "true"
        vault.hashicorp.com/role: "myapp-role"
        vault.hashicorp.com/agent-inject-secret-db: "secret/data/database/config"
        vault.hashicorp.com/agent-inject-template-db: |
          {{- with secret "secret/data/database/config" -}}
          export DB_HOST="{{ .Data.data.host }}"
          export DB_PASSWORD="{{ .Data.data.password }}"
          {{- end }}
    spec:
      serviceAccountName: myapp
      containers:
        - name: app
          image: myapp:latest
          command: ["/bin/sh"]
          args:
            - -c
            - source /vault/secrets/db && ./app
```

### Vault CSI Provider

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: myapp
spec:
  serviceAccountName: myapp
  containers:
    - name: app
      image: myapp:latest
      volumeMounts:
        - name: secrets-store
          mountPath: "/mnt/secrets"
          readOnly: true
      env:
        - name: DB_PASSWORD
          valueFrom:
            secretKeyRef:
              name: vault-db-secret
              key: password
  volumes:
    - name: secrets-store
      csi:
        driver: secrets-store.csi.k8s.io
        readOnly: true
        volumeAttributes:
          secretProviderClass: "vault-database"
---
apiVersion: secrets-store.csi.x-k8s.io/v1
kind: SecretProviderClass
metadata:
  name: vault-database
spec:
  provider: vault
  parameters:
    vaultAddress: "https://vault.company.com"
    roleName: "myapp-role"
    objects: |
      - objectName: "db-password"
        secretPath: "secret/data/database/config"
        secretKey: "password"
  secretObjects:
    - secretName: vault-db-secret
      type: Opaque
      data:
        - objectName: db-password
          key: password
```

---

## SOPS - 文件加密

### 安装与配置

```bash
# 安装SOPS
wget https://github.com/mozilla/sops/releases/download/v3.8.1/sops-v3.8.1.linux
chmod +x sops-v3.8.1.linux
sudo mv sops-v3.8.1.linux /usr/local/bin/sops

# 配置加密密钥(.sops.yaml)
cat > .sops.yaml <<EOF
creation_rules:
  - path_regex: .*/prod/.*\.yaml$
    kms: 'arn:aws:kms:cn-north-1:123456789:key/abc123'
  - path_regex: .*/dev/.*\.yaml$
    age: age1qqqqqqqqqqqqqqqqqq...
EOF
```

### 加密Secret文件

```bash
# 创建明文Secret
cat > secret.yaml <<EOF
apiVersion: v1
kind: Secret
metadata:
  name: db-secret
stringData:
  username: admin
  password: SecureP@ss123
EOF

# 加密文件
sops -e secret.yaml > secret.enc.yaml

# 解密预览
sops -d secret.enc.yaml

# 直接部署(需集成工具)
sops -d secret.enc.yaml | kubectl apply -f -
```

### ArgoCD + SOPS 集成

```yaml
# argocd-cm ConfigMap
apiVersion: v1
kind: ConfigMap
metadata:
  name: argocd-cm
  namespace: argocd
data:
  kustomize.buildOptions: "--enable-alpha-plugins --enable-helm"
  # 配置SOPS插件
  configManagementPlugins: |
    - name: sops
      generate:
        command: ["sh", "-c"]
        args:
          - |
            sops -d $ARGOCD_ENV_FILE > /tmp/decrypted.yaml
            kubectl kustomize . --load-restrictor=LoadRestrictionsNone
```

---

## 工具选型建议

| 场景 | 推荐方案 | 原因 |
|------|---------|------|
| **多云环境** | ESO | 统一接口，支持20+后端 |
| **GitOps团队** | Sealed Secrets + ESO | 加密存储+外部同步 |
| **已有Vault** | Vault Agent/CSI | 复用现有基础设施 |
| **小团队快速启动** | SOPS + Age | 轻量级、易上手 |
| **金融/合规** | Vault + ESO | 审计能力、企业支持 |

---

## 安全最佳实践

```yaml
# 1. RBAC最小权限
apiVersion: rbac.authorization.k8s.io/v1
kind: Role
metadata:
  name: secret-reader
rules:
  - apiGroups: [""]
    resources: ["secrets"]
    resourceNames: ["specific-secret"]  # 指定Secret名称
    verbs: ["get"]

# 2. 审计日志启用
apiVersion: audit.k8s.io/v1
kind: Policy
rules:
  - level: RequestResponse
    resources:
      - group: ""
        resources: ["secrets", "configmaps"]

# 3. 网络隔离
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: deny-secret-access
spec:
  podSelector:
    matchLabels:
      app: untrusted
  policyTypes:
    - Egress
  egress:
    - to:
        - podSelector:
            matchLabels:
              app: public-service
```

---

## 常见问题

**Q: ESO vs Sealed Secrets如何选择?**  
A: ESO适合有外部密钥管理系统(Vault/AWS)，Sealed Secrets适合无外部依赖的GitOps场景。可组合使用。

**Q: 如何处理密钥轮换?**  
A: ESO支持定期刷新(refreshInterval)，Sealed Secrets需要重新加密并替换。

**Q: 密钥泄露如何应急?**  
A: 立即轮换外部密钥→更新ExternalSecret/SealedSecret→删除旧K8s Secret→重启Pod。
