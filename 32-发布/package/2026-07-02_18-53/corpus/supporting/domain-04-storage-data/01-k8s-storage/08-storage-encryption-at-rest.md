---
title: 存储加密与密钥管理
description: 'Kubernetes 存储加密：KMS Provider 集成、Secrets Store CSI Driver、静态加密方案、密钥轮转策略与合规要求'
summary: 'Kubernetes 存储加密：KMS Provider 集成、Secrets Store CSI Driver、静态加密方案、密钥轮转策略与合规要求'
category: storage-data
tags:
- storage
- k8s
- encryption
- kms
- secrets-store
- compliance
- security
tier: supporting
created: '2026-07-02'
last_updated: 2026-07
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 平台工程师
estimated_read_time: 15min
intent_queries:
- 存储加密 是什么
- 如何 KMS Provider 集成
- Secrets Store CSI Driver 配置
- 静态加密方案
- 密钥轮转策略
- SOC2 PCI-DSS 合规
trigger_keywords:
- KMS
- Secrets Store
- CSI Driver
- 加密
- 密钥轮转
- SOC2
- PCI-DSS
- 合规
prerequisites:
- kubectl-basics
- storage-basics
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

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。


# 存储加密与密钥管理

> **适用版本**: Kubernetes v1.28 - v1.32 | **最后更新**: 2026-07
> **文档定位**: 存储加密是生产环境安全基线。本文覆盖 KMS Provider、Secrets Store CSI Driver、云厂商原生加密和合规要求。

## 1. 架构概览

### 1.1 加密层次

```
# 🟢 低风险：只读/信息收集，通常无副作用
┌─────────────────────────────────────────────────────────┐
│              Storage Encryption Layers                   │
│                                                          │
│  Layer 1: etcd 加密 (KMS Provider)                       │
│  ┌─────────────────────────────────────────────────────┐│
│  │  Kubernetes Secrets → etcd → KMS Provider → HSM/KMS ││
│  └─────────────────────────────────────────────────────┘│
│                                                          │
│  Layer 2: Secrets 外部化 (Secrets Store CSI)             │
│  ┌─────────────────────────────────────────────────────┐│
│  │  External Secrets → CSI Driver → Pod Mount           ││
│  │  (Vault/AWS SM/Azure KV/GCP SM)                     ││
│  └─────────────────────────────────────────────────────┘│
│                                                          │
│  Layer 3: PV 静态加密 (Storage Class)                    │
│  ┌─────────────────────────────────────────────────────┐│
│  │  PVC → StorageClass → CSI Driver → Encrypted Volume ││
│  │  (EBS/Azure Disk/PD 原生加密)                        ││
│  └─────────────────────────────────────────────────────┘│
│                                                          │
│  Layer 4: 应用层加密                                     │
│  ┌─────────────────────────────────────────────────────┐│
│  │  Application → 加密库 → 明文写入 → 密文存储         ││
│  └─────────────────────────────────────────────────────┘│
└─────────────────────────────────────────────────────────┘
```
### 1.2 加密方案对比

| 层次 | 方案 | 加密范围 | 性能影响 | 运维复杂度 | 合规覆盖 |
|------|------|---------|---------|-----------|---------|
| etcd 加密 | KMS Provider | Secrets | 低 | 中 | SOC2, PCI-DSS |
| Secrets 外部化 | Secrets Store CSI | Secrets | 低 | 中 | SOC2, PCI-DSS, HIPAA |
| PV 静态加密 | StorageClass 加密 | 卷数据 | 0-5% | 低 | SOC2, PCI-DSS, HIPAA |
| 应用层加密 | 应用自行实现 | 应用数据 | 5-20% | 高 | 全部 |

## 2. KMS Provider 集成

### 2.1 KMS v2 架构

```
# 🟢 低风险：只读/信息收集，通常无副作用
┌─────────────────────────────────────────────────────────┐
│              KMS v2 Provider Architecture                 │
│                                                          │
│  ┌──────────────┐     ┌──────────────────────────────┐  │
│  │ kube-apiserver│     │  KMS Plugin (gRPC)           │  │
│  │              │────►│  ┌──────────────────────────┐│  │
│  │  etcd 加密   │     │  │ Encrypt/Decrypt          ││  │
│  │  配置        │     │  │ DEK 管理                 ││  │
│  └──────────────┘     │  └──────────┬───────────────┘│  │
│                       └─────────────┼────────────────┘  │
│                                     │                    │
│                                     ▼                    │
│                       ┌──────────────────────────────┐  │
│                       │  External KMS/HSM             │  │
│                       │  ┌────────┬────────┬────────┐│  │
│                       │  │ AWS KMS│ Azure  │ HashiCorp││  │
│                       │  │        │ Key V. │ Vault   ││  │
│                       │  └────────┴────────┴────────┘│  │
│                       └──────────────────────────────┘  │
└─────────────────────────────────────────────────────────┘
```
### 2.2 AWS KMS 集成

```yaml
# /etc/kubernetes/encryption-config.yaml
apiVersion: apiserver.config.k8s.io/v1
kind: EncryptionConfiguration
resources:
  - resources:
      - secrets
      - configmaps
    providers:
      - kms:
          apiVersion: v2
          name: aws-kms
          endpoint: unix:///var/run/kms-provider.sock
          cachesize: 1000
          timeout: 3s
      - identity: {}    # 回退到明文（用于解密旧数据）
```

```yaml
# KMS Plugin DaemonSet
apiVersion: apps/v1
kind: DaemonSet
metadata:
  name: aws-kms-provider
  namespace: kube-system
spec:
  selector:
    matchLabels:
      app: aws-kms-provider
  template:
    metadata:
      labels:
        app: aws-kms-provider
    spec:
      nodeSelector:
        node-role.kubernetes.io/control-plane: ""
      tolerations:
        - key: node-role.kubernetes.io/control-plane
          effect: NoSchedule
      containers:
        - name: kms-provider
          image: public.ecr.aws/aws-encryption-provider/aws-encryption-provider:v0.3.0
          command:
            - /aws-encryption-provider
            - --key=arn:aws:kms:us-east-1:123456789012:key/12345678-1234-1234-1234-123456789012
            - --region=us-east-1
            - --listen=/var/run/kms-provider.sock
          volumeMounts:
            - name: socket
              mountPath: /var/run
          resources:
            requests:
              cpu: 50m
              memory: 64Mi
            limits:
              cpu: 100m
              memory: 128Mi
      volumes:
        - name: socket
          hostPath:
            path: /var/run
```

### 2.3 HashiCorp Vault 集成

```yaml
# Vault KMS Provider 配置
apiVersion: apps/v1
kind: DaemonSet
metadata:
  name: vault-kms-provider
  namespace: kube-system
spec:
  selector:
    matchLabels:
      app: vault-kms-provider
  template:
    metadata:
      labels:
        app: vault-kms-provider
    spec:
      nodeSelector:
        node-role.kubernetes.io/control-plane: ""
      tolerations:
        - key: node-role.kubernetes.io/control-plane
          effect: NoSchedule
      containers:
        - name: vault-kms
          image: hashicorp/vault-k8s:latest
          command:
            - vault-k8s
            - -address=https://vault.example.com:8200
            - -tls-skip-verify=false
            - -path=transit/keys/kubernetes
          env:
            - name: VAULT_TOKEN
              valueFrom:
                secretKeyRef:
                  name: vault-kms-token
                  key: token
          volumeMounts:
            - name: socket
              mountPath: /var/run
            - name: vault-tls
              mountPath: /etc/vault/tls
              readOnly: true
      volumes:
        - name: socket
          hostPath:
            path: /var/run
        - name: vault-tls
          secret:
            secretName: vault-tls
```

### 2.4 启用 KMS 加密

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 修改 kube-apiserver 配置
# /etc/kubernetes/manifests/kube-apiserver.yaml
# 添加以下参数:
# - --encryption-provider-config=/etc/kubernetes/encryption-config.yaml

# 验证加密配置
kubectl get secrets -A -o json | \
  jq '.items[0].metadata.annotations'

# 迁移现有 Secrets 到加密存储
kubectl get secrets -A -o json | \
  kubectl replace -f -
```
## 3. Secrets Store CSI Driver

### 3.1 安装 Secrets Store CSI Driver

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# Helm 安装
helm repo add secrets-store-csi-driver https://kubernetes-sigs.github.io/secrets-store-csi-driver/charts
helm install csi-secrets-store secrets-store-csi-driver/secrets-store-csi-driver \
  --namespace kube-system \
  --set syncSecret.enabled=true \
  --set enableSecretRotation=true \
  --set rotationPollInterval=3600 \
  --wait

# 验证安装
kubectl get pods -n kube-system -l app=csi-secrets-store
```
### 3.2 AWS Secrets Manager Provider

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 安装 AWS Provider
helm install aws-secrets-manager secrets-store-csi-driver/secrets-store-csi-driver-provider-aws \
  --namespace kube-system
```
```yaml
# SecretProviderClass (AWS)
apiVersion: secrets-store.csi.x-k8s.io/v1
kind: SecretProviderClass
metadata:
  name: aws-secrets-manager
  namespace: production
spec:
  provider: aws
  parameters:
    objects: |
      - objectName: "prod/database/credentials"
        objectType: "secretsmanager"
        jmesPath:
          - path: username
            objectAlias: db-username
          - path: password
            objectAlias: db-password
  secretObjects:
    - secretName: db-credentials-sync
      type: Opaque
      data:
        - objectName: db-username
          key: username
        - objectName: db-password
          key: password
```

### 3.3 HashiCorp Vault Provider

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 安装 Vault Provider
kubectl apply -f https://raw.githubusercontent.com/hashicorp/vault-csi-provider/main/deployment/vault-csi-provider.yaml
```
```yaml
# SecretProviderClass (Vault)
apiVersion: secrets-store.csi.x-k8s.io/v1
kind: SecretProviderClass
metadata:
  name: vault-secrets
  namespace: production
spec:
  provider: vault
  parameters:
    roleName: "production"
    vaultAddress: "https://vault.example.com:8200"
    vaultCACertPath: "/vault/tls/ca.crt"
    objects: |
      - objectName: "db-password"
        secretPath: "secret/data/production/database"
        secretKey: "password"
      - objectName: "api-key"
        secretPath: "secret/data/production/api"
        secretKey: "key"
  secretObjects:
    - secretName: app-secrets
      type: Opaque
      data:
        - objectName: db-password
          key: db-password
        - objectName: api-key
          key: api-key
```

### 3.4 使用 Secrets Store 卷

```yaml
# pod-with-secrets.yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: my-app
  namespace: production
spec:
  replicas: 3
  selector:
    matchLabels:
      app: my-app
  template:
    metadata:
      labels:
        app: my-app
    spec:
      containers:
        - name: app
          image: my-app:latest
          volumeMounts:
            - name: secrets-store
              mountPath: "/mnt/secrets"
              readOnly: true
          env:
            - name: DB_PASSWORD
              valueFrom:
                secretKeyRef:
                  name: app-secrets
                  key: db-password
      volumes:
        - name: secrets-store
          csi:
            driver: secrets-store.csi.k8s.io
            readOnly: true
            volumeAttributes:
              secretProviderClass: vault-secrets
```

## 4. PV 静态加密

### 4.1 AWS EBS 加密

```yaml
# 加密的 EBS StorageClass
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: ebs-gp3-encrypted
provisioner: ebs.csi.aws.com
parameters:
  type: gp3
  iops: "3000"
  throughput: "125"
  encrypted: "true"
  kmsKeyId: "arn:aws:kms:us-east-1:123456789012:key/12345678-1234-1234-1234-123456789012"
reclaimPolicy: Delete
volumeBindingMode: WaitForFirstConsumer
allowVolumeExpansion: true
```

### 4.2 Azure Disk 加密

```yaml
# 加密的 Azure Disk StorageClass
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: azure-disk-encrypted
provisioner: disk.csi.azure.com
parameters:
  skuName: Premium_LRS
  kind: Managed
  # 使用平台管理的加密密钥
  encryptionSetID: "/subscriptions/xxx/resourceGroups/xxx/providers/Microsoft.Compute/diskEncryptionSets/xxx"
reclaimPolicy: Delete
volumeBindingMode: WaitForFirstConsumer
allowVolumeExpansion: true
```

### 4.3 GCE PD 加密

```yaml
# 加密的 GCE PD StorageClass
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: gce-pd-encrypted
provisioner: pd.csi.storage.gke.io
parameters:
  type: pd-ssd
  replication-type: regional-pd
  disk-encryption-kms-key: "projects/my-project/locations/us-central1/keyRings/my-ring/cryptoKeys/my-key"
reclaimPolicy: Delete
volumeBindingMode: WaitForFirstConsumer
allowVolumeExpansion: true
```

### 4.4 Ceph RBD 加密

```yaml
# 加密的 Ceph RBD StorageClass
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: ceph-rbd-encrypted
provisioner: rbd.csi.ceph.com
parameters:
  clusterID: my-ceph-cluster
  pool: rbd
  imageFormat: "2"
  imageFeatures: layering
  
  # 加密配置
  encryptionPassphraseSecretName: csi-rbd-encryption
  encryptionPassphraseSecretNamespace: ceph-csi
  
  csi.storage.k8s.io/provisioner-secret-name: csi-rbd-secret
  csi.storage.k8s.io/provisioner-secret-namespace: ceph-csi
reclaimPolicy: Delete
volumeBindingMode: Immediate
allowVolumeExpansion: true
```

```yaml
# 加密密钥 Secret
apiVersion: v1
kind: Secret
metadata:
  name: csi-rbd-encryption
  namespace: ceph-csi
type: Opaque
stringData:
  encryptionPassphrase: "my-secure-passphrase-1234567890"
```

## 5. 密钥轮转策略

### 5.1 KMS 密钥轮转

```yaml
# KMS 密钥轮转 CronJob
apiVersion: batch/v1
kind: CronJob
metadata:
  name: kms-key-rotation
  namespace: kube-system
spec:
  schedule: "0 3 1 * *"    # 每月 1 日凌晨 3 点
  jobTemplate:
    spec:
      template:
        spec:
          serviceAccountName: kms-rotation
          containers:
            - name: rotation
              image: amazon/aws-cli:latest
              command:
                - /bin/sh
                - -c
                - |
                  # AWS KMS 密钥轮转
                  KEY_ID="arn:aws:kms:us-east-1:123456789012:key/12345678-1234-1234-1234-123456789012"
                  
                  # 启用自动密钥轮转
                  aws kms enable-key-rotation \
                    --key-id ${KEY_ID}
                  
                  # 验证轮转状态
                  aws kms get-key-rotation-status \
                    --key-id ${KEY_ID}
                  
                  echo "KMS key rotation enabled for ${KEY_ID}"
          restartPolicy: OnFailure
```

### 5.2 Secrets 自动轮转

```yaml
# Secrets Store CSI 自动轮转配置
# 在 CSI Driver Helm values 中启用
# secrets-store-csi-driver:
#   enableSecretRotation: true
#   rotationPollInterval: 3600  # 每小时检查一次

# 或通过 SecretProviderClass 注解
apiVersion: secrets-store.csi.x-k8s.io/v1
kind: SecretProviderClass
metadata:
  name: auto-rotating-secrets
  namespace: production
  annotations:
    secrets-store.csi.k8s.io/rotation-enabled: "true"
    secrets-store.csi.k8s.io/rotation-interval: "1h"
spec:
  provider: aws
  parameters:
    objects: |
      - objectName: "prod/rotating-secret"
        objectType: "secretsmanager"
```

### 5.3 应用级密钥轮转

```yaml
# 密钥轮转 Job
apiVersion: batch/v1
kind: Job
metadata:
  name: rotate-app-secrets
  namespace: production
spec:
  template:
    spec:
      serviceAccountName: secret-rotator
      containers:
        - name: rotator
          image: bitnami/kubectl:latest
          command:
            - /bin/sh
            - -c
            - |
              # 生成新密码
              NEW_PASSWORD=$(openssl rand -base64 32)
              
              # 更新 Secret
              kubectl patch secret app-credentials -n production \
                --type='json' \
                -p="[{\"op\": \"replace\", \"path\": \"/data/password\", \"value\": \"$(echo -n ${NEW_PASSWORD} | base64)\"}]"
              
              # 滚动重启使用该 Secret 的 Deployment
              kubectl rollout restart deployment/my-app -n production
              
              echo "Secret rotated successfully"
      restartPolicy: Never
  backoffLimit: 3
```

### 5.4 etcd 数据重新加密

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 当 KMS 密钥轮转后，需要重新加密 etcd 中的数据
# 使用 kubectl 触发所有 Secrets 更新

# 获取所有 Secrets 并重新写入
kubectl get secrets -A -o json | \
  kubectl replace -f -

# 或使用脚本批量处理
for ns in $(kubectl get ns -o jsonpath='{.items[*].metadata.name}'); do
  for secret in $(kubectl get secrets -n $ns -o jsonpath='{.items[*].metadata.name}'); do
    kubectl get secret $secret -n $ns -o yaml | kubectl apply -f -
  done
done
```
## 6. 合规要求

### 6.1 SOC 2 合规

```yaml
soc2_requirements:
  # CC6.1 - 逻辑和物理访问控制
  access_control:
    - 实施最小权限原则
    - 定期审查访问权限
    - 多因素认证
  
  # CC6.6 - 系统边界保护
  data_protection:
    - 静态数据加密 (Encryption at Rest)
    - 传输数据加密 (Encryption in Transit)
    - 密钥管理流程
  
  # CC6.7 - 数据传输和处置
  data_handling:
    - 安全删除策略
    - 数据保留策略
    - 审计日志

# K8s 实现
kubernetes_implementation:
  etcd_encryption:
    enabled: true
    provider: "KMS v2"
  
  pv_encryption:
    enabled: true
    storage_classes: "all"
  
  network_policies:
    enabled: true
    default_deny: true
  
  rbac:
    enabled: true
    audit_logging: true
```

### 6.2 PCI-DSS 合规

```yaml
pci_dss_requirements:
  # Requirement 3: 保护存储的账户数据
  stored_data:
    - 3.4: 渲染 PAN 不可读
    - 3.5: 保护加密密钥
    - 3.6: 密钥管理程序
  
  # Requirement 7: 按业务需求限制访问
  access_restriction:
    - 最小权限原则
    - 基于角色的访问控制
  
  # Requirement 10: 跟踪和监控访问
  monitoring:
    - 审计日志
    - 入侵检测
    - 日志保留 1 年

# K8s 实现
kubernetes_implementation:
  secret_encryption:
    method: "KMS v2 with HSM"
    key_rotation: "90 days"
  
  pv_encryption:
    method: "Cloud KMS"
    key_rotation: "365 days"
  
  audit_logging:
    enabled: true
    retention: "365 days"
  
  network_isolation:
    enabled: true
    microsegmentation: true
```

### 6.3 HIPAA 合规

```yaml
hipaa_requirements:
  # §164.312(a)(2)(iv) - 加密和解密
  encryption:
    - 静态加密 (AES-256)
    - 传输加密 (TLS 1.2+)
    - 密钥管理
  
  # §164.312(b) - 审计控制
  audit:
    - 访问日志
    - 修改日志
    - 日志保留 6 年
  
  # §164.312(c)(1) - 完整性
  integrity:
    - 数据完整性校验
    - 备份验证

# K8s 实现
kubernetes_implementation:
  encryption:
    etcd: "KMS v2 with FIPS 140-2 validated module"
    pv: "AES-256 with customer-managed keys"
    transit: "TLS 1.3"
  
  audit:
    enabled: true
    retention: "6 years"
    immutable: true
```

## 7. 监控与审计

### 7.1 加密状态监控

```yaml
# encryption-monitoring.yaml
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: encryption-alerts
  namespace: monitoring
spec:
  groups:
    - name: encryption.rules
      rules:
        # KMS 可用性
        - alert: KMSProviderDown
          expr: up{job="kms-provider"} == 0
          for: 5m
          labels:
            severity: critical
          annotations:
            summary: "KMS Provider 不可用，可能导致 Secret 读取失败"

        # Secrets Store CSI 同步失败
        - alert: SecretsStoreSyncFailed
          expr: |
            increase(secrets_store_csi_sync_errors_total[1h]) > 0
          for: 10m
          labels:
            severity: warning
          annotations:
            summary: "Secrets Store CSI 同步失败"

        # 密钥轮转过期
        - alert: KeyRotationOverdue
          expr: |
            (time() - secrets_store_csi_last_rotation_timestamp) > 90 * 24 * 3600
          for: 1h
          labels:
            severity: warning
          annotations:
            summary: "密钥轮转超过 90 天未执行"
```

### 7.2 审计日志配置

```yaml
# audit-policy.yaml
apiVersion: audit.k8s.io/v1
kind: Policy
rules:
  # 记录所有 Secret 访问
  - level: RequestResponse
    resources:
      - group: ""
        resources: ["secrets"]
    verbs: ["create", "update", "patch", "delete", "get", "list"]
  
  # 记录加密配置变更
  - level: RequestResponse
    resources:
      - group: "storage.k8s.io"
        resources: ["storageclasses"]
    verbs: ["create", "update", "patch", "delete"]
  
  # 记录 PV/PVC 操作
  - level: Metadata
    resources:
      - group: ""
        resources: ["persistentvolumes", "persistentvolumeclaims"]
```

```bash
# 启用审计日志
# kube-apiserver 参数:
# - --audit-policy-file=/etc/kubernetes/audit-policy.yaml
# - --audit-log-path=/var/log/kubernetes/audit.log
# - --audit-log-maxage=365
# - --audit-log-maxbackup=10
# - --audit-log-maxsize=100
```

## 8. 生产最佳实践

### 8.1 加密策略建议

| 数据类型 | 加密方案 | 密钥管理 | 轮转周期 |
|---------|---------|---------|---------|
| **K8s Secrets** | KMS v2 | HSM/Cloud KMS | 90 天 |
| **应用凭据** | Secrets Store CSI | Vault/Secrets Manager | 30 天 |
| **PV 数据** | StorageClass 加密 | Cloud KMS | 365 天 |
| **备份数据** | 备份工具加密 | 独立密钥 | 按需 |

### 8.2 密钥管理清单

```yaml
key_management_checklist:
  creation:
    - [ ] 使用强随机密钥
    - [ ] 最小化密钥权限
    - [ ] 记录密钥元数据
  
  storage:
    - [ ] 使用专用密钥管理系统 (HSM/KMS)
    - [ ] 实施密钥分离
    - [ ] 备份密钥
  
  rotation:
    - [ ] 自动化密钥轮转
    - [ ] 轮转后重新加密数据
    - [ ] 验证轮转成功
  
  revocation:
    - [ ] 密钥吊销流程
    - [ ] 紧急响应计划
    - [ ] 审计日志
```

### 8.3 安全加固

```yaml
security_hardening:
  # etcd 加密
  etcd:
    - 启用 KMS v2 Provider
    - 定期轮转加密密钥
    - 监控 KMS 可用性
  
  # Secrets 管理
  secrets:
    - 外部化到专用系统
    - 启用自动轮转
    - 实施访问审计
  
  # PV 加密
  pv:
    - 所有 StorageClass 启用加密
    - 使用客户管理密钥
    - 跨区域加密密钥复制
  
  # 网络安全
  network:
    - etcd 通信加密
    - KMS 通信加密
    - CSI 通信加密
```

---

## Related

- [[32-发布/package/2026-07-02_18-53/corpus/peripheral/domain-04-storage-data/01-k8s-storage/05-storage-security-compliance|存储安全与合规]]

## See Also

- [Kubernetes Encryption at Rest](https://kubernetes.io/docs/tasks/administer-cluster/encrypt-data/)
- [Secrets Store CSI Driver](https://secrets-store-csi-driver.sigs.k8s.io/)
- [AWS KMS Provider](https://github.com/kubernetes-sigs/aws-encryption-provider)


<!-- risk-assessed -->
