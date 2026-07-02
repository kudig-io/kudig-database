---
title: AKS 身份认证与 Workload Identity
description: 'Azure Workload Identity OIDC 集成、Key Vault CSI Provider、AAD Pod Identity 迁移、Managed Identity 最佳实践'
summary: 'Azure Workload Identity OIDC 集成、Key Vault CSI Provider、AAD Pod Identity 迁移、Managed Identity 最佳实践'
category: cloud-providers
tags:
- cloud
- k8s
- aks
- azure
- identity
- workload-identity
- oidc
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
- Azure Workload Identity 是什么
- 如何配置 AKS Workload Identity
- 如何从 AAD Pod Identity 迁移
trigger_keywords:
- Workload Identity
- OIDC
- Key Vault
- Managed Identity
- AAD Pod Identity
prerequisites:
- kubectl-basics
- cloud-basics
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


# AKS 身份认证与 Workload Identity

## 1. 认证架构演进

```
AKS 身份认证演进路径：

Legacy（已废弃）:
  AAD Pod Identity → 使用 CRD + DaemonSet 拦截 IMDS 请求
  问题：延迟高、维护复杂、安全边界模糊

Current（推荐）:
  Azure Workload Identity → 基于 OIDC 的原生集成
  优势：零额外组件、Token 直接获取、审计友好

Integration:
  K8s ServiceAccount ← OIDC Trust → Azure Managed Identity
  Pod 使用标准 K8s SA → 直接获取 Azure AD Token
```

## 2. Azure Workload Identity 配置

### 2.1 前置条件

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 确认 AKS 集群已启用 OIDC Issuer
az aks show \
  --resource-group rg-aks-prod \
  --name aks-prod-01 \
  --query oidcIssuerProfile

# 如果未启用，更新集群
az aks update \
  --resource-group rg-aks-prod \
  --name aks-prod-01 \
  --enable-oidc-issuer \
  --enable-workload-identity

# 获取 OIDC Issuer URL
OIDC_ISSUER=$(az aks show \
  --resource-group rg-aks-prod \
  --name aks-prod-01 \
  --query oidcIssuerProfile.issuerUrl \
  --output tsv)
echo $OIDC_ISSUER
```
### 2.2 创建 User-Assigned Managed Identity

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 创建 Managed Identity
az identity create \
  --resource-group rg-aks-prod \
  --name mi-app-prod \
  --location eastasia \
  --subscription $SUBSCRIPTION_ID

# 获取 Identity 信息
CLIENT_ID=$(az identity show \
  --resource-group rg-aks-prod \
  --name mi-app-prod \
  --query clientId \
  --output tsv)

OBJECT_ID=$(az identity show \
  --resource-group rg-aks-prod \
  --name mi-app-prod \
  --query principalId \
  --output tsv)

# 赋予 Identity 访问 Azure 资源的权限
# 示例：允许读取 Storage Account
az role assignment create \
  --assignee $CLIENT_ID \
  --role "Storage Blob Data Reader" \
  --scope /subscriptions/$SUBSCRIPTION_ID/resourceGroups/rg-aks-prod/providers/Microsoft.Storage/storageAccounts/saprod01
```
### 2.3 建立 Federated Identity Credential

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 创建 Federated Identity（关联 K8s SA 到 Azure MI）
az identity federated-credential create \
  --name fic-app-prod \
  --identity-name mi-app-prod \
  --resource-group rg-aks-prod \
  --issuer $OIDC_ISSUER \
  --subject system:serviceaccount:production:app-sa \
  --audience api://AzureADTokenExchange

# 一个 Managed Identity 可关联多个 Federated Credential
# 适用于多命名空间/多服务共享 Identity
```
### 2.4 Kubernetes 配置

```yaml
apiVersion: v1
kind: ServiceAccount
metadata:
  name: app-sa
  namespace: production
  annotations:
    azure.workload.identity/client-id: "${CLIENT_ID}"
  labels:
    azure.workload.identity/use: "true"

---
apiVersion: apps/v1
kind: Deployment
metadata:
  name: app-backend
  namespace: production
spec:
  template:
    metadata:
      labels:
        azure.workload.identity/use: "true"
    spec:
      serviceAccountName: app-sa
      containers:
      - name: app
        image: app:v1.0
        env:
        - name: AZURE_CLIENT_ID
          value: "${CLIENT_ID}"
        - name: AZURE_TENANT_ID
          value: "${TENANT_ID}"
        - name: AZURE_FEDERATED_TOKEN_FILE
          value: "/var/run/secrets/azure/tokens/azure-identity-token"
        volumeMounts:
        - name: azure-identity-token
          mountPath: /var/run/secrets/azure/tokens
          readOnly: true
      volumes:
      - name: azure-identity-token
        projected:
          sources:
          - serviceAccountToken:
              path: azure-identity-token
              audience: api://AzureADTokenExchange
              expirationSeconds: 3600
```

### 2.5 应用代码中获取 Token

```python
# Python 示例（使用 azure-identity）
from azure.identity import DefaultAzureCredential
from azure.storage.blob import BlobServiceClient

# Workload Identity 会自动使用 SA Token 换取 Azure Token
credential = DefaultAzureCredential()

# 访问 Azure 资源
blob_client = BlobServiceClient(
    account_url="https://saprod01.blob.core.windows.net",
    credential=credential
)

# 列出容器中的 Blob
container = blob_client.get_container_client("data")
for blob in container.list_blobs():
    print(blob.name)
```

```go
// Go 示例
import (
    "github.com/Azure/azure-sdk-for-go/sdk/azidentity"
    "github.com/Azure/azure-sdk-for-go/sdk/storage/azblob"
)

cred, err := azidentity.NewDefaultAzureCredential(nil)
if err != nil {
    log.Fatal(err)
}

client, err := azblob.NewClient("https://saprod01.blob.core.windows.net", cred, nil)
```

## 3. Key Vault CSI Provider

### 3.1 安装

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# AKS 已内置 Key Vault CSI Provider
# 确认已启用
az aks show \
  --resource-group rg-aks-prod \
  --name aks-prod-01 \
  --query addonProfiles.azureKeyvaultSecretsProvider.enabled

# 如果未启用
az aks enable-addons \
  --resource-group rg-aks-prod \
  --name aks-prod-01 \
  --addons azure-keyvault-secrets-provider
```
### 3.2 SecretProviderClass 配置

```yaml
apiVersion: secrets-store.csi.x-k8s.io/v1
kind: SecretProviderClass
metadata:
  name: app-secrets
  namespace: production
spec:
  provider: azure
  parameters:
    useVMManagedIdentity: "false"
    usePodIdentity: "false"
    clientID: "${CLIENT_ID}"
    keyvaultName: "kv-prod-eastasia"
    tenantId: "${TENANT_ID}"
    objects: |
      array:
        - |
          objectName: db-connection-string
          objectType: secret
        - |
          objectName: redis-password
          objectType: secret
        - |
          objectName: tls-cert
          objectType: cert
        - |
          objectName: signing-key
          objectType: key
  # 可选：同步到 K8s Secret
  secretObjects:
  - secretName: app-k8s-secret
    type: Opaque
    data:
    - objectName: db-connection-string
      key: connection-string
    - objectName: redis-password
      key: redis-password
```

### 3.3 Pod 挂载 Key Vault Secret

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: app-backend
  namespace: production
spec:
  template:
    metadata:
      labels:
        azure.workload.identity/use: "true"
    spec:
      serviceAccountName: app-sa
      containers:
      - name: app
        image: app:v1.0
        volumeMounts:
        - name: secrets-store
          mountPath: "/mnt/secrets"
          readOnly: true
        env:
        - name: DB_CONNECTION_STRING
          valueFrom:
            secretKeyRef:
              name: app-k8s-secret
              key: connection-string
      volumes:
      - name: secrets-store
        csi:
          driver: secrets-store.csi.k8s.io
          readOnly: true
          volumeAttributes:
            secretProviderClass: app-secrets
```

### 3.4 Secret 自动轮换

```yaml
# 在 SecretProviderClass 中启用自动轮换
apiVersion: secrets-store.csi.x-k8s.io/v1
kind: SecretProviderClass
metadata:
  name: app-secrets
spec:
  provider: azure
  parameters:
    # ... 其他参数同上
  secretObjects:
  - secretName: app-k8s-secret
    type: Opaque
    data:
    - objectName: db-connection-string
      key: connection-string

---
# DaemonSet 配置轮换间隔
# 在 AKS 中默认 2 分钟轮询一次
# 可通过 Helm values 自定义：
# secrets-store-csi-driver.rotationPollInterval: 2m
```

## 4. AAD Pod Identity 迁移指南

### 4.1 迁移策略

```
迁移步骤（零停机）：

阶段 1: 准备
  ├── 确认 AKS 版本 ≥ 1.22
  ├── 启用 OIDC Issuer 和 Workload Identity
  ├── 记录所有 AzureIdentity 和 Binding
  └── 为每个 Identity 创建 Federated Credential

阶段 2: 并行运行
  ├── 创建新的 ServiceAccount（带 WI 注解）
  ├── 更新 Deployment 使用新 SA
  ├── 验证应用可正常获取 Token
  └── 监控两种方式并行运行 1 周

阶段 3: 切换
  ├── 移除 Pod 中的 aadpodidbinding label
  ├── 删除 AzureIdentityBinding / AzureIdentity
  └── 卸载 AAD Pod Identity 组件
```

### 4.2 迁移对比

```yaml
# 旧方式：AAD Pod Identity
apiVersion: aadpodidentity.k8s.io/v1
kind: AzureIdentity
metadata:
  name: app-identity
  namespace: production
spec:
  type: 0
  resourceID: /subscriptions/.../providers/Microsoft.ManagedIdentity/userAssignedIdentities/mi-app
  clientID: xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx
---
apiVersion: aadpodidentity.k8s.io/v1
kind: AzureIdentityBinding
metadata:
  name: app-identity-binding
spec:
  azureIdentity: app-identity
  selector: app-identity-label
---
# Deployment 使用 label 选择
spec:
  template:
    metadata:
      labels:
        aadpodidbinding: app-identity-label

---
# 新方式：Azure Workload Identity
apiVersion: v1
kind: ServiceAccount
metadata:
  name: app-sa
  namespace: production
  annotations:
    azure.workload.identity/client-id: "xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx"
  labels:
    azure.workload.identity/use: "true"
---
# Deployment 使用 serviceAccountName
spec:
  template:
    metadata:
      labels:
        azure.workload.identity/use: "true"
    spec:
      serviceAccountName: app-sa
```

## 5. Managed Identity 最佳实践

### 5.1 Identity 粒度设计

```
推荐的 Identity 粒度：

方案 A：每服务一个 Identity（推荐）
  优势：最小权限、审计清晰、隔离性好
  劣势：管理对象多
  适用：生产环境、安全敏感场景

方案 B：每命名空间一个 Identity
  优势：管理简单
  劣势：命名空间内权限粒度粗
  适用：团队级隔离

方案 C：每环境一个 Identity
  优势：最简管理
  劣势：权限过于宽泛
  仅适用：dev/staging 环境
```

### 5.2 权限最小化

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 只读 Storage Blob
az role assignment create \
  --assignee $CLIENT_ID \
  --role "Storage Blob Data Reader" \
  --scope /subscriptions/$SUB/resourceGroups/rg-prod/providers/Microsoft.Storage/storageAccounts/saprod01/containers/data

# 而非宽泛的
# az role assignment create --role "Contributor" --scope /subscriptions/$SUB
```
### 5.3 审计与监控

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看 Workload Identity Token 请求日志
# 在 Azure AD 日志中查询
az monitor log-analytics query \
  --workspace $WORKSPACE_ID \
  --analytics-query "
    AADServicePrincipalSignInLogs
    | where ServicePrincipalName startswith 'mi-app-prod'
    | project TimeGenerated, ServicePrincipalName, ResourceDisplayName, IPAddress, Status
    | order by TimeGenerated desc
  "

# 告警：异常 Token 请求
# 在 Azure Monitor 中配置 Alert Rule
```
## 6. 故障排查

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 检查 OIDC Issuer 状态
az aks show -g rg-aks-prod -n aks-prod-01 --query oidcIssuerProfile

# 检查 Federated Credential
az identity federated-credential list \
  --identity-name mi-app-prod \
  --resource-group rg-aks-prod

# 验证 ServiceAccount 注解
kubectl get sa app-sa -n production -o yaml

# 检查 Token 投影卷
kubectl describe pod -n production -l app=app-backend | grep -A5 "azure-identity-token"

# 测试 Token 获取
kubectl exec -it -n production deploy/app-backend -- cat /var/run/secrets/azure/tokens/azure-identity-token

# 检查 Key Vault CSI Provider 日志
kubectl logs -n kube-system -l app=csi-secrets-store-provider-azure --tail=50
```
## Related

- [[04-aks-storage-managed-disk|AKS 存储与 Managed Disk]]
- [[06-aks-troubleshooting-playbook|AKS 故障排查手册]]

## See Also

- Azure Workload Identity 官方文档
- Key Vault CSI Provider 文档
- AAD Pod Identity 迁移指南


<!-- risk-assessed -->
