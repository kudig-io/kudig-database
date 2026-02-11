# 云厂商集成故障排查指南

> **适用版本**: Kubernetes v1.25 - v1.32 | **最后更新**: 2026-02 | **文档类型**: 多云环境故障处理

## 0. 10 分钟快速诊断

1. **CCM 存活**：检查各云 Cloud Controller Manager Pod 状态与错误日志。
2. **认证权限**：验证云凭证/实例角色/IAM 权限是否有效。
3. **LB 创建**：`kubectl get svc -A | grep LoadBalancer`，查看事件与云侧资源。
4. **存储供给**：检查 StorageClass 参数与云盘配额是否超限。
5. **网络连通**：确认 VPC/安全组/路由表放通关键端口。
6. **快速缓解**：
   - 降低 API 调用频率或增加配额。
   - 临时切换到备用节点组或区域。
7. **证据留存**：保存 CCM 日志、云 API 错误、Service 事件。

## ☁️ 云厂商集成常见问题与影响分析

### 主要云厂商集成问题

| 问题类型 | 典型现象 | 影响程度 | 紧急级别 |
|---------|---------|---------|---------|
| 云提供商认证失败 | `cloud provider authentication failed` | ⭐⭐⭐ 高 | P0 |
| LoadBalancer 服务创建失败 | `failed to ensure load balancer` | ⭐⭐⭐ 高 | P0 |
| 持久化卷动态供给失败 | `failed to provision volume` | ⭐⭐ 中 | P1 |
| 云厂商特定资源无法创建 | `invalid cloud provider configuration` | ⭐⭐ 中 | P1 |
| 多云环境网络互通问题 | `cross-cloud network connectivity failed` | ⭐⭐⭐ 高 | P0 |
| 云厂商 API 限流/超时 | `cloud provider API rate limited` | ⭐⭐ 中 | P1 |

### 云厂商集成状态检查

```bash
# AWS 云提供商检查
echo "=== AWS Cloud Provider 检查 ==="
kubectl get pods -n kube-system -l k8s-app=aws-cloud-controller-manager
kubectl logs -n kube-system -l k8s-app=aws-cloud-controller-manager --tail=50

# Azure 云提供商检查
echo "=== Azure Cloud Provider 检查 ==="
kubectl get pods -n kube-system -l k8s-app=azure-cloud-controller-manager
kubectl logs -n kube-system -l k8s-app=azure-cloud-controller-manager --tail=50

# GCP 云提供商检查
echo "=== GCP Cloud Provider 检查 ==="
kubectl get pods -n kube-system -l k8s-app=gcp-cloud-controller-manager
kubectl logs -n kube-system -l k8s-app=gcp-cloud-controller-manager --tail=50

# 阿里云检查
echo "=== 阿里云 Cloud Provider 检查 ==="
kubectl get pods -n kube-system -l app=alicloud-cloud-controller-manager
kubectl logs -n kube-system -l app=alicloud-cloud-controller-manager --tail=50
```

## 🔍 云厂商集成问题诊断方法

### 诊断原理说明

云厂商集成故障通常涉及以下几个层面：

1. **认证授权层**：云厂商凭证配置、IAM 权限设置
2. **网络层面**：VPC 配置、安全组规则、路由表设置
3. **存储层面**：块存储、文件存储、对象存储集成
4. **负载均衡层面**：云厂商 LoadBalancer 实现
5. **API 调用层面**：云厂商 API 限流、超时、错误处理

### 云厂商问题诊断决策树

```
云厂商集成故障
    ├── 认证授权问题
    │   ├── 凭证配置检查
    │   ├── IAM 权限验证
    │   ├── 服务账户绑定
    │   └── 区域/可用区配置
    ├── 网络连接问题
    │   ├── VPC 配置检查
    │   ├── 安全组规则
    │   ├── 路由表设置
    │   └── DNS 解析配置
    ├── 存储集成问题
    │   ├── 存储类配置
    │   ├── 动态供给参数
    │   ├── 卷插件状态
    │   └── 存储配额限制
    └── 负载均衡问题
        ├── LoadBalancer 配置
        ├── 健康检查设置
        ├── 监听器规则
        └── SSL/TLS 配置
```

### 详细诊断命令

#### 1. AWS 云提供商故障诊断

```bash
#!/bin/bash
# AWS 云提供商故障诊断脚本

echo "=== AWS Cloud Provider 故障诊断 ==="

# 1. 检查 AWS 凭证配置
echo "1. AWS 凭证配置检查:"
if [ -f "/etc/kubernetes/aws-credentials" ]; then
  echo "AWS 凭证文件存在"
  # 检查凭证格式
  grep -E "^(aws_access_key_id|aws_secret_access_key)" /etc/kubernetes/aws-credentials
else
  echo "❌ AWS 凭证文件不存在"
fi

# 2. 检查 IAM 权限
echo "2. IAM 权限检查:"
INSTANCE_ROLE=$(curl -s http://169.254.169.254/latest/meta-data/iam/security-credentials/)
if [ -n "$INSTANCE_ROLE" ]; then
  echo "实例角色: $INSTANCE_ROLE"
  # 测试基本 AWS API 访问
  aws sts get-caller-identity 2>/dev/null && echo "✓ IAM 权限正常" || echo "❌ IAM 权限不足"
else
  echo "❌ 未配置实例角色"
fi

# 3. 检查 AWS Cloud Controller Manager 状态
echo "3. AWS Cloud Controller Manager 状态:"
kubectl get pods -n kube-system -l k8s-app=aws-cloud-controller-manager -o wide
kubectl logs -n kube-system -l k8s-app=aws-cloud-controller-manager --tail=100 | grep -i error

# 4. 检查 LoadBalancer 服务状态
echo "4. LoadBalancer 服务检查:"
kubectl get services --all-namespaces -o wide | grep LoadBalancer

# 5. 检查 EBS 卷状态
echo "5. EBS 卷状态检查:"
kubectl get pv -o json | jq -r '.items[] | select(.spec.awsElasticBlockStore != null) | .metadata.name + ": " + .status.phase'

# 6. AWS 特定资源检查
echo "6. AWS 资源配额检查:"
aws ec2 describe-account-attributes --attribute-names max-instances --region us-east-1 2>/dev/null || echo "无法获取 EC2 配额信息"
```

#### 2. Azure 云提供商故障诊断

```bash
#!/bin/bash
# Azure 云提供商故障诊断脚本

echo "=== Azure Cloud Provider 故障诊断 ==="

# 1. 检查 Azure 凭证配置
echo "1. Azure 凭证配置检查:"
if [ -f "/etc/kubernetes/azure.json" ]; then
  echo "Azure 配置文件存在"
  jq '.' /etc/kubernetes/azure.json 2>/dev/null || echo "配置文件格式错误"
else
  echo "❌ Azure 配置文件不存在"
fi

# 2. 检查 MSI 身份验证
echo "2. MSI 身份验证检查:"
MSI_TOKEN=$(curl -s 'http://169.254.169.254/metadata/identity/oauth2/token?api-version=2018-02-01&resource=https%3A%2F%2Fmanagement.azure.com%2F' -H Metadata:true)
if [ -n "$MSI_TOKEN" ]; then
  echo "✓ MSI 身份验证正常"
  echo "$MSI_TOKEN" | jq -r '.access_token' | cut -c1-20
else
  echo "❌ MSI 身份验证失败"
fi

# 3. 检查 Azure Cloud Controller Manager
echo "3. Azure Cloud Controller Manager 状态:"
kubectl get pods -n kube-system -l k8s-app=azure-cloud-controller-manager -o wide
kubectl logs -n kube-system -l k8s-app=azure-cloud-controller-manager --tail=100 | grep -i error

# 4. 检查 Azure 负载均衡器
echo "4. Azure LoadBalancer 检查:"
az account show 2>/dev/null && echo "✓ Azure CLI 认证正常" || echo "❌ Azure CLI 认证失败"

# 5. 检查托管标识权限
echo "5. 托管标识权限检查:"
SUBSCRIPTION_ID=$(jq -r '.subscriptionId' /etc/kubernetes/azure.json 2>/dev/null)
RESOURCE_GROUP=$(jq -r '.resourceGroup' /etc/kubernetes/azure.json 2>/dev/null)

if [ -n "$SUBSCRIPTION_ID" ] && [ -n "$RESOURCE_GROUP" ]; then
  echo "订阅ID: $SUBSCRIPTION_ID"
  echo "资源组: $RESOURCE_GROUP"
  
  # 检查基本权限
  az role assignment list --assignee $(jq -r '.aadClientId' /etc/kubernetes/azure.json 2>/dev/null) \
    --scope "/subscriptions/$SUBSCRIPTION_ID/resourceGroups/$RESOURCE_GROUP" 2>/dev/null || echo "权限检查失败"
fi
```

#### 3. GCP 云提供商故障诊断

```bash
#!/bin/bash
# GCP 云提供商故障诊断脚本

echo "=== GCP Cloud Provider 故障诊断 ==="

# 1. 检查 GCP 凭证配置
echo "1. GCP 凭证配置检查:"
if [ -f "/etc/gcp/service-account.json" ]; then
  echo "Service Account 文件存在"
  PROJECT_ID=$(jq -r '.project_id' /etc/gcp/service-account.json 2>/dev/null)
  echo "项目ID: $PROJECT_ID"
else
  echo "❌ Service Account 文件不存在"
fi

# 2. 检查 Workload Identity
echo "2. Workload Identity 检查:"
WI_ENABLED=$(kubectl get deploy -n kube-system -l k8s-app=gcp-cloud-controller-manager -o jsonpath='{.items[*].spec.template.spec.serviceAccountName}' 2>/dev/null)
if [ -n "$WI_ENABLED" ]; then
  echo "Workload Identity 启用: $WI_ENABLED"
  # 验证 WI 映射
  gcloud iam service-accounts describe $WI_ENABLED@$PROJECT_ID.iam.gserviceaccount.com 2>/dev/null && echo "✓ WI 配置正常" || echo "❌ WI 配置异常"
fi

# 3. 检查 GCP Cloud Controller Manager
echo "3. GCP Cloud Controller Manager 状态:"
kubectl get pods -n kube-system -l k8s-app=gcp-cloud-controller-manager -o wide
kubectl logs -n kube-system -l k8s-app=gcp-cloud-controller-manager --tail=100 | grep -i error

# 4. 检查 GCP API 访问
echo "4. GCP API 访问检查:"
gcloud services list --enabled --project=$PROJECT_ID 2>/dev/null | grep -E "(compute|container)" && echo "✓ 必需 API 已启用" || echo "❌ 必需 API 未启用"

# 5. 检查防火墙规则
echo "5. 防火墙规则检查:"
gcloud compute firewall-rules list --project=$PROJECT_ID --filter="name~k8s" 2>/dev/null || echo "未找到 Kubernetes 相关防火墙规则"
```

#### 4. 阿里云故障诊断

```bash
#!/bin/bash
# 阿里云故障诊断脚本

echo "=== 阿里云故障诊断 ==="

# 1. 检查阿里云凭证配置
echo "1. 阿里云凭证配置检查:"
if [ -f "/etc/kubernetes/cloud-config" ]; then
  echo "阿里云配置文件存在"
  grep -E "^(accessKeyID|accessKeySecret)" /etc/kubernetes/cloud-config
else
  echo "❌ 阿里云配置文件不存在"
fi

# 2. 检查 RAM 权限
echo "2. RAM 权限检查:"
ROLE_NAME=$(curl -s http://100.100.100.200/latest/meta-data/ram/security-credentials/ 2>/dev/null)
if [ -n "$ROLE_NAME" ]; then
  echo "RAM 角色: $ROLE_NAME"
  # 获取临时凭证
  TEMP_CREDS=$(curl -s http://100.100.100.200/latest/meta-data/ram/security-credentials/$ROLE_NAME)
  if [ -n "$TEMP_CREDS" ]; then
    echo "✓ RAM 权限正常"
    echo "$TEMP_CREDS" | jq -r '.AccessKeyId' | cut -c1-10
  else
    echo "❌ RAM 权限获取失败"
  fi
else
  echo "❌ 未配置 RAM 角色"
fi

# 3. 检查阿里云 Cloud Controller Manager
echo "3. 阿里云 Cloud Controller Manager 状态:"
kubectl get pods -n kube-system -l app=alicloud-cloud-controller-manager -o wide
kubectl logs -n kube-system -l app=alicloud-cloud-controller-manager --tail=100 | grep -i error

# 4. 检查 SLB 状态
echo "4. SLB 负载均衡检查:"
kubectl get services --all-namespaces -o json | jq -r '.items[] | select(.spec.type=="LoadBalancer") | .metadata.name + " (" + .status.loadBalancer.ingress[0].ip + ")"'

# 5. 检查云盘状态
echo "5. 云盘状态检查:"
kubectl get pv -o json | jq -r '.items[] | select(.spec.csi.driver=="diskplugin.csi.alibabacloud.com") | .metadata.name + ": " + .status.phase'
```

## 🔧 云厂商集成问题解决方案

### AWS 集成问题解决

#### 方案一：AWS 凭证和权限修复

```yaml
# AWS Cloud Provider 配置示例
apiVersion: v1
kind: Secret
metadata:
  name: aws-credentials
  namespace: kube-system
type: Opaque
data:
  # base64 encoded credentials
  aws_access_key_id: <base64-encoded-access-key>
  aws_secret_access_key: <base64-encoded-secret-key>

---
# AWS Cloud Controller Manager 部署配置
apiVersion: apps/v1
kind: Deployment
metadata:
  name: aws-cloud-controller-manager
  namespace: kube-system
spec:
  template:
    spec:
      containers:
      - name: aws-cloud-controller-manager
        image: registry.k8s.io/provider-aws/cloud-controller-manager:v1.32.0
        args:
        - --cloud-provider=aws
        - --cluster-name=my-cluster
        - --allocate-node-cidrs=true
        - --configure-cloud-routes=true
        env:
        - name: AWS_ACCESS_KEY_ID
          valueFrom:
            secretKeyRef:
              name: aws-credentials
              key: aws_access_key_id
        - name: AWS_SECRET_ACCESS_KEY
          valueFrom:
            secretKeyRef:
              name: aws-credentials
              key: aws_secret_access_key
```

#### 方案二：IAM 权限策略配置

```json
{
  "Version": "2012-10-17",
  "Statement": [
    {
      "Effect": "Allow",
      "Action": [
        "ec2:DescribeInstances",
        "ec2:DescribeRegions",
        "ec2:DescribeRouteTables",
        "ec2:DescribeSecurityGroups",
        "ec2:DescribeSubnets",
        "ec2:DescribeVolumes",
        "ec2:CreateSecurityGroup",
        "ec2:CreateTags",
        "ec2:CreateVolume",
        "ec2:ModifyInstanceAttribute",
        "ec2:ModifyVolume",
        "ec2:AttachVolume",
        "ec2:AuthorizeSecurityGroupIngress",
        "ec2:CreateRoute",
        "ec2:DeleteRoute",
        "ec2:DeleteSecurityGroup",
        "ec2:DeleteVolume",
        "ec2:DetachVolume",
        "ec2:RevokeSecurityGroupIngress",
        "ec2:DescribeVpcs",
        "elasticloadbalancing:AddTags",
        "elasticloadbalancing:AttachLoadBalancerToSubnets",
        "elasticloadbalancing:ApplySecurityGroupsToLoadBalancer",
        "elasticloadbalancing:CreateLoadBalancer",
        "elasticloadbalancing:CreateLoadBalancerPolicy",
        "elasticloadbalancing:CreateLoadBalancerListeners",
        "elasticloadbalancing:ConfigureHealthCheck",
        "elasticloadbalancing:DeleteLoadBalancer",
        "elasticloadbalancing:DeleteLoadBalancerListeners",
        "elasticloadbalancing:DescribeLoadBalancers",
        "elasticloadbalancing:DescribeLoadBalancerAttributes",
        "elasticloadbalancing:DetachLoadBalancerFromSubnets",
        "elasticloadbalancing:DeregisterInstancesFromLoadBalancer",
        "elasticloadbalancing:ModifyLoadBalancerAttributes",
        "elasticloadbalancing:RegisterInstancesWithLoadBalancer",
        "elasticloadbalancing:SetLoadBalancerPoliciesForBackendServer",
        "elasticloadbalancing:AddTags",
        "elasticloadbalancing:CreateListener",
        "elasticloadbalancing:CreateRule",
        "elasticloadbalancing:DeleteListener",
        "elasticloadbalancing:DeleteRule",
        "elasticloadbalancing:DeleteTargetGroup",
        "elasticloadbalancing:DeregisterTargets",
        "elasticloadbalancing:DescribeListeners",
        "elasticloadbalancing:DescribeLoadBalancerPolicies",
        "elasticloadbalancing:DescribeTargetGroups",
        "elasticloadbalancing:DescribeTargetHealth",
        "elasticloadbalancing:ModifyListener",
        "elasticloadbalancing:ModifyRule",
        "elasticloadbalancing:RegisterTargets",
        "elasticloadbalancing:SetIpAddressType",
        "elasticloadbalancing:SetSecurityGroups",
        "elasticloadbalancing:SetSubnets",
        "elasticloadbalancing:SetWebAcl"
      ],
      "Resource": "*"
    }
  ]
}
```

### Azure 集成问题解决

#### 方案一：Azure 服务主体配置

```bash
#!/bin/bash
# Azure 服务主体创建和配置脚本

echo "=== Azure 服务主体配置 ==="

# 1. 创建服务主体
AZURE_SUBSCRIPTION_ID=$(az account show --query id -o tsv)
AZURE_TENANT_ID=$(az account show --query tenantId -o tsv)

echo "订阅ID: $AZURE_SUBSCRIPTION_ID"
echo "租户ID: $AZURE_TENANT_ID"

# 创建服务主体
SP_NAME="k8s-cluster-sp-$(date +%Y%m%d)"
echo "创建服务主体: $SP_NAME"

SP_RESULT=$(az ad sp create-for-rbac --name $SP_NAME --role Contributor --scopes /subscriptions/$AZURE_SUBSCRIPTION_ID 2>/dev/null)

if [ $? -eq 0 ]; then
  echo "✓ 服务主体创建成功"
  echo "$SP_RESULT" | jq '.'
  
  # 提取凭据
  CLIENT_ID=$(echo "$SP_RESULT" | jq -r '.appId')
  CLIENT_SECRET=$(echo "$SP_RESULT" | jq -r '.password')
  
  echo "客户端ID: $CLIENT_ID"
  echo "客户端密钥: $(echo $CLIENT_SECRET | cut -c1-10)..."
  
  # 生成 Azure 配置文件
  cat > /etc/kubernetes/azure.json << EOF
{
  "cloud": "AzurePublicCloud",
  "tenantId": "$AZURE_TENANT_ID",
  "subscriptionId": "$AZURE_SUBSCRIPTION_ID",
  "aadClientId": "$CLIENT_ID",
  "aadClientSecret": "$CLIENT_SECRET",
  "resourceGroup": "my-k8s-rg",
  "location": "eastus",
  "vmType": "standard",
  "subnetName": "k8s-subnet",
  "securityGroupName": "k8s-nsg",
  "vnetName": "k8s-vnet",
  "vnetResourceGroup": "my-k8s-rg",
  "routeTableName": "k8s-routetable",
  "primaryScaleSetName": "",
  "primaryAvailabilitySetName": "",
  "cloudProviderBackoff": true,
  "cloudProviderBackoffRetries": 6,
  "cloudProviderBackoffExponent": 1.5,
  "cloudProviderBackoffDuration": 5,
  "cloudProviderBackoffJitter": 1,
  "cloudProviderRateLimit": true,
  "cloudProviderRateLimitQPS": 3,
  "cloudProviderRateLimitBucket": 10,
  "useManagedIdentityExtension": false,
  "userAssignedIdentityID": "",
  "useInstanceMetadata": true,
  "loadBalancerSku": "Standard",
  "disableOutboundSNAT": false,
  "excludeMasterFromStandardLB": true
}
EOF

  echo "✓ Azure 配置文件已生成: /etc/kubernetes/azure.json"
else
  echo "❌ 服务主体创建失败"
fi
```

#### 方案二：Azure 托管标识配置

```yaml
# 使用托管标识的 Azure Cloud Provider 配置
apiVersion: v1
kind: ServiceAccount
metadata:
  name: azure-cloud-provider
  namespace: kube-system
  annotations:
    azure.workload.identity/client-id: "YOUR_USER_ASSIGNED_IDENTITY_CLIENT_ID"

---
apiVersion: apps/v1
kind: Deployment
metadata:
  name: azure-cloud-controller-manager
  namespace: kube-system
spec:
  template:
    spec:
      serviceAccountName: azure-cloud-provider
      containers:
      - name: azure-cloud-controller-manager
        image: mcr.microsoft.com/oss/kubernetes/azure-cloud-controller-manager:v1.28.2
        args:
        - --cloud-config=/etc/kubernetes/azure.json
        - --cloud-provider=azure
        - --leader-elect=true
        - --use-managed-identity-extension=true
        volumeMounts:
        - name: cloud-config
          mountPath: /etc/kubernetes/azure.json
          subPath: azure.json
          readOnly: true
      volumes:
      - name: cloud-config
        configMap:
          name: azure-cloud-provider-config
```

### GCP 集成问题解决

#### 方案一：Workload Identity 配置

```bash
#!/bin/bash
# GCP Workload Identity 配置脚本

PROJECT_ID="your-project-id"
CLUSTER_NAME="your-cluster-name"
REGION="us-central1"

echo "=== GCP Workload Identity 配置 ==="

# 1. 启用必要的 API
echo "1. 启用必要 API:"
gcloud services enable container.googleapis.com iam.googleapis.com

# 2. 创建 Google Service Account
GSA_NAME="k8s-gcp-provider"
GSA_EMAIL="$GSA_NAME@$PROJECT_ID.iam.gserviceaccount.com"

echo "创建 Google Service Account: $GSA_EMAIL"
gcloud iam service-accounts create $GSA_NAME \
  --display-name="Kubernetes GCP Provider"

# 3. 分配必要权限
echo "3. 分配权限:"
gcloud projects add-iam-policy-binding $PROJECT_ID \
  --member="serviceAccount:$GSA_EMAIL" \
  --role="roles/compute.admin"

gcloud projects add-iam-policy-binding $PROJECT_ID \
  --member="serviceAccount:$GSA_EMAIL" \
  --role="roles/container.admin"

gcloud projects add-iam-policy-binding $PROJECT_ID \
  --member="serviceAccount:$GSA_EMAIL" \
  --role="roles/iam.serviceAccountUser"

# 4. 创建 Kubernetes Service Account
echo "4. 创建 Kubernetes Service Account:"
kubectl create serviceaccount gcp-cloud-provider -n kube-system

# 5. 建立 Workload Identity 映射
echo "5. 建立 Workload Identity 映射:"
gcloud iam service-accounts add-iam-policy-binding $GSA_EMAIL \
  --role roles/iam.workloadIdentityUser \
  --member "serviceAccount:$PROJECT_ID.svc.id.goog[kube-system/gcp-cloud-provider]"

# 6. 添加注解到 Kubernetes Service Account
kubectl annotate serviceaccount gcp-cloud-provider \
  -n kube-system \
  iam.gke.io/gcp-service-account=$GSA_EMAIL

echo "✓ Workload Identity 配置完成"
```

#### 方案二：传统服务账户密钥配置

```yaml
# GCP 服务账户密钥配置
apiVersion: v1
kind: Secret
metadata:
  name: gcp-service-account-key
  namespace: kube-system
type: Opaque
data:
  service-account.json: <base64-encoded-service-account-key>

---
apiVersion: apps/v1
kind: Deployment
metadata:
  name: gcp-cloud-controller-manager
  namespace: kube-system
spec:
  template:
    spec:
      containers:
      - name: gcp-cloud-controller-manager
        image: gke.gcr.io/cloud-controller-manager:v1.32.0-gke.0
        args:
        - --cloud-provider=gce
        - --cluster-name=$(CLUSTER_NAME)
        - --configure-cloud-routes=true
        env:
        - name: GOOGLE_APPLICATION_CREDENTIALS
          value: /etc/gcp/service-account.json
        volumeMounts:
        - name: gcp-credentials
          mountPath: /etc/gcp
          readOnly: true
      volumes:
      - name: gcp-credentials
        secret:
          secretName: gcp-service-account-key
```

### 阿里云集成问题解决

#### 方案一：RAM 角色权限配置

```bash
#!/bin/bash
# 阿里云 RAM 角色配置脚本

echo "=== 阿里云 RAM 角色配置 ==="

# 1. 创建 RAM 角色策略
cat > k8s-provider-policy.json << 'EOF'
{
  "Version": "1",
  "Statement": [
    {
      "Action": [
        "ecs:DescribeInstances",
        "ecs:CreateSecurityGroup",
        "ecs:DeleteSecurityGroup",
        "ecs:AuthorizeSecurityGroup",
        "ecs:RevokeSecurityGroup",
        "ecs:CreateSecurityGroupAttribute",
        "ecs:DescribeSecurityGroupAttribute",
        "ecs:DescribeSecurityGroups",
        "ecs:DescribeAvailableResource",
        "ecs:AllocatePublicIpAddress",
        "ecs:CreateDisk",
        "ecs:AttachDisk",
        "ecs:DetachDisk",
        "ecs:DeleteDisk",
        "ecs:DescribeDisks",
        "ecs:CreateSnapshot",
        "ecs:DeleteSnapshot",
        "ecs:DescribeSnapshots",
        "ecs:ModifyDiskAttribute",
        "ecs:ResizeDisk",
        "ecs:ResetDisk",
        "ecs:ReplaceSystemDisk",
        "ecs:RenewInstance",
        "ecs:StopInstance",
        "ecs:StartInstance",
        "ecs:RebootInstance",
        "ecs:ModifyInstanceAttribute",
        "ecs:ModifyInstanceNetworkSpec",
        "ecs:ModifyInstanceSpec",
        "ecs:DescribeInstanceTypes",
        "ecs:DescribeZones",
        "ecs:DescribeRegions",
        "ecs:ImportImage",
        "ecs:ExportImage",
        "ecs:CopyImage",
        "ecs:CancelCopyImage",
        "ecs:DescribeImages",
        "ecs:DeleteImage",
        "ecs:CreateImage",
        "ecs:ModifyImageAttribute",
        "ecs:ModifyImageSharePermission",
        "ecs:DescribeImageSharePermission",
        "ecs:TagResource",
        "ecs:UntagResource",
        "ecs:ListTagResources",
        "slb:CreateLoadBalancer",
        "slb:DeleteLoadBalancer",
        "slb:SetLoadBalancerStatus",
        "slb:SetLoadBalancerName",
        "slb:ModifyLoadBalancerInternetSpec",
        "slb:DescribeLoadBalancers",
        "slb:DescribeLoadBalancerAttribute",
        "slb:CreateLoadBalancerHTTPListener",
        "slb:CreateLoadBalancerHTTPSListener",
        "slb:CreateLoadBalancerTCPListener",
        "slb:CreateLoadBalancerUDPListener",
        "slb:DeleteLoadBalancerListener",
        "slb:StartLoadBalancerListener",
        "slb:StopLoadBalancerListener",
        "slb:DescribeLoadBalancerHTTPListenerAttribute",
        "slb:DescribeLoadBalancerHTTPSListenerAttribute",
        "slb:DescribeLoadBalancerTCPListenerAttribute",
        "slb:DescribeLoadBalancerUDPListenerAttribute",
        "slb:SetLoadBalancerHTTPListenerAttribute",
        "slb:SetLoadBalancerHTTPSListenerAttribute",
        "slb:SetLoadBalancerTCPListenerAttribute",
        "slb:SetLoadBalancerUDPListenerAttribute",
        "slb:AddBackendServers",
        "slb:RemoveBackendServers",
        "slb:SetBackendServers",
        "slb:DescribeHealthStatus",
        "slb:CreateVServerGroup",
        "slb:SetVServerGroupAttribute",
        "slb:DeleteVServerGroup",
        "slb:DescribeVServerGroups",
        "slb:DescribeVServerGroupAttribute",
        "slb:AddVServerGroupBackendServers",
        "slb:RemoveVServerGroupBackendServers",
        "slb:ModifyVServerGroupBackendServers",
        "slb:UploadServerCertificate",
        "slb:DeleteServerCertificate",
        "slb:SetDomainExtensionAttribute",
        "slb:CreateDomainExtension",
        "slb:DeleteDomainExtension",
        "slb:DescribeDomainExtensions",
        "slb:DescribeServerCertificates",
        "vpc:DescribeVpcs",
        "vpc:DescribeVSwitches",
        "vpc:DescribeRouteTableList",
        "vpc:DescribeRouteEntryList",
        "vpc:CreateRouteEntry",
        "vpc:DeleteRouteEntry",
        "vpc:DescribeNatGateways",
        "vpc:CreateNatGateway",
        "vpc:DeleteNatGateway",
        "vpc:DescribeSnatTableEntries",
        "vpc:CreateSnatEntry",
        "vpc:DeleteSnatEntry",
        "vpc:DescribeForwardTableEntries",
        "vpc:CreateForwardEntry",
        "vpc:DeleteForwardEntry"
      ],
      "Resource": "*",
      "Effect": "Allow"
    }
  ]
}
EOF

# 2. 创建 RAM 策略
POLICY_NAME="K8sCloudProviderPolicy"
aliyun ram CreatePolicy \
  --PolicyName $POLICY_NAME \
  --PolicyDocument "$(cat k8s-provider-policy.json)"

# 3. 创建 RAM 角色
ROLE_NAME="K8sCloudProviderRole"
aliyun ram CreateRole \
  --RoleName $ROLE_NAME \
  --AssumeRolePolicyDocument '{
    "Statement": [
      {
        "Action": "sts:AssumeRole",
        "Effect": "Allow",
        "Principal": {
          "Service": [
            "ecs.aliyuncs.com"
          ]
        }
      }
    ],
    "Version": "1"
  }'

# 4. 绑定策略到角色
aliyun ram AttachPolicyToRole \
  --PolicyType Custom \
  --PolicyName $POLICY_NAME \
  --RoleName $ROLE_NAME

echo "✓ RAM 角色配置完成: $ROLE_NAME"
```

#### 方案二：阿里云 Cloud Provider 配置

```yaml
# 阿里云 Cloud Provider 配置文件
apiVersion: v1
kind: ConfigMap
metadata:
  name: cloud-config
  namespace: kube-system
data:
  cloud-config: |
    {
      "Global": {
        "accessKeyID": "your-access-key-id",
        "accessKeySecret": "your-access-key-secret",
        "region": "cn-hangzhou",
        "zone": "cn-hangzhou-a",
        "vpcid": "vpc-xxxxxx",
        "routerid": "vtb-xxxxxx",
        "securityGroupID": "sg-xxxxxx"
      },
      "LoadBalancer": {
        "slbNetworkType": "internet",
        "chargeType": "PayByTraffic",
        "masterZoneID": "cn-hangzhou-a",
        "slaveZoneID": "cn-hangzhou-b"
      }
    }

---
apiVersion: apps/v1
kind: DaemonSet
metadata:
  name: alicloud-cloud-controller-manager
  namespace: kube-system
spec:
  selector:
    matchLabels:
      app: alicloud-cloud-controller-manager
  template:
    metadata:
      labels:
        app: alicloud-cloud-controller-manager
    spec:
      serviceAccountName: cloud-controller-manager
      containers:
      - name: alicloud-cloud-controller-manager
        image: registry.cn-hangzhou.aliyuncs.com/acs/cloud-controller-manager-amd64:v1.9.3
        args:
        - --cloud-provider=alicloud
        - --cluster-name=kubernetes
        - --allocate-node-cidrs=true
        - --configure-cloud-routes=true
        - --cloud-config=/etc/kubernetes/cloud-config
        volumeMounts:
        - name: cloud-config
          mountPath: /etc/kubernetes/cloud-config
          subPath: cloud-config
          readOnly: true
      volumes:
      - name: cloud-config
        configMap:
          name: cloud-config
```

## ⚠️ 执行风险评估

| 操作 | 风险等级 | 影响评估 | 回滚方案 |
|------|---------|---------|---------|
| 云厂商凭证更新 | ⭐⭐ 中 | 可能影响云资源访问 | 恢复原凭证配置 |
| IAM/权限策略修改 | ⭐⭐⭐ 高 | 可能导致权限过大或过小 | 恢复原权限策略 |
| 云提供商组件升级 | ⭐⭐ 中 | 可能影响云资源管理 | 回滚到旧版本组件 |
| 网络配置变更 | ⭐⭐⭐ 高 | 可能影响网络连通性 | 恢复原网络配置 |

## 📊 云厂商集成验证与监控

### 集成验证脚本

```bash
#!/bin/bash
# 云厂商集成验证脚本

echo "=== 云厂商集成验证 ==="

# 1. 通用验证函数
verify_cloud_provider() {
  local provider=$1
  local check_command=$2
  
  echo "验证 $provider 集成:"
  if eval $check_command; then
    echo "✓ $provider 集成正常"
  else
    echo "❌ $provider 集成异常"
  fi
  echo ""
}

# 2. AWS 验证
verify_cloud_provider "AWS" "
  kubectl get pods -n kube-system -l k8s-app=aws-cloud-controller-manager 2>/dev/null | grep -q Running &&
  kubectl logs -n kube-system -l k8s-app=aws-cloud-controller-manager --tail=10 2>/dev/null | grep -q 'starting workers'
"

# 3. Azure 验证
verify_cloud_provider "Azure" "
  kubectl get pods -n kube-system -l k8s-app=azure-cloud-controller-manager 2>/dev/null | grep -q Running &&
  kubectl logs -n kube-system -l k8s-app=azure-cloud-controller-manager --tail=10 2>/dev/null | grep -q 'starting workers'
"

# 4. GCP 验证
verify_cloud_provider "GCP" "
  kubectl get pods -n kube-system -l k8s-app=gcp-cloud-controller-manager 2>/dev/null | grep -q Running &&
  kubectl logs -n kube-system -l k8s-app=gcp-cloud-controller-manager --tail=10 2>/dev/null | grep -q 'starting workers'
"

# 5. 阿里云验证
verify_cloud_provider "阿里云" "
  kubectl get pods -n kube-system -l app=alicloud-cloud-controller-manager 2>/dev/null | grep -q Running &&
  kubectl logs -n kube-system -l app=alicloud-cloud-controller-manager --tail=10 2>/dev/null | grep -q 'starting workers'
"

# 6. 功能测试
echo "功能测试:"
echo "创建测试 LoadBalancer 服务..."
kubectl apply -f - << EOF
apiVersion: v1
kind: Service
metadata:
  name: test-lb-service
spec:
  type: LoadBalancer
  ports:
  - port: 80
    targetPort: 80
  selector:
    app: test-app
EOF

sleep 30

LB_STATUS=$(kubectl get service test-lb-service -o jsonpath='{.status.loadBalancer.ingress[0].ip}' 2>/dev/null)
if [ -n "$LB_STATUS" ] && [ "$LB_STATUS" != "<pending>" ]; then
  echo "✓ LoadBalancer 服务创建成功: $LB_STATUS"
else
  echo "⚠ LoadBalancer 服务仍在创建中或失败"
fi

# 清理测试资源
kubectl delete service test-lb-service 2>/dev/null
```

### 云厂商监控告警配置

```yaml
# Prometheus 云厂商监控告警
groups:
- name: cloud-provider
  rules:
  - alert: CloudControllerManagerDown
    expr: absent(up{job="cloud-controller-manager"}) == 1
    for: 5m
    labels:
      severity: critical
    annotations:
      summary: "云控制器管理器宕机"
      description: "{{ $labels.job }} 云控制器管理器不可用"

  - alert: CloudProviderAPIErrors
    expr: sum(rate(rest_client_requests_total{code=~"5.."}[5m])) by (job) > 0
    for: 10m
    labels:
      severity: warning
    annotations:
      summary: "云提供商 API 错误"
      description: "{{ $labels.job }} 云提供商 API 返回 5xx 错误"

  - alert: LoadBalancerProvisioningFailed
    expr: kube_service_status_load_balancer_ingress == 0
    for: 15m
    labels:
      severity: warning
    annotations:
      summary: "LoadBalancer 配置失败"
      description: "服务 {{ $labels.service }} 的 LoadBalancer 配置失败"

  - alert: PersistentVolumeProvisioningFailed
    expr: kube_persistentvolumeclaim_status_phase{phase="Pending"} == 1
    for: 10m
    labels:
      severity: warning
    annotations:
      summary: "持久化卷配置失败"
      description: "PVC {{ $labels.persistentvolumeclaim }} 配置失败"

  - alert: CloudProviderRateLimited
    expr: rate(rest_client_rate_limiter_duration_seconds_count[5m]) > 10
    for: 5m
    labels:
      severity: warning
    annotations:
      summary: "云提供商 API 限流"
      description: "{{ $labels.job }} 云提供商 API 调用被限流"
```

## 📚 云厂商集成最佳实践

### 多云环境配置管理

```yaml
# 多云环境配置示例
multiCloudConfig:
  providers:
    aws:
      enabled: true
      region: us-east-1
      credentials:
        secretName: aws-credentials
      
    azure:
      enabled: true
      subscriptionId: "your-subscription-id"
      tenantId: "your-tenant-id"
      
    gcp:
      enabled: true
      projectId: "your-project-id"
      useWorkloadIdentity: true
      
    alibaba:
      enabled: true
      region: cn-hangzhou
      useRamRole: true
  
  crossCloudNetworking:
    vpnConnections:
      - name: aws-to-azure
        from: aws-us-east-1
        to: azure-eastus
        type: ipsec
        
      - name: gcp-to-alibaba
        from: gcp-us-central1
        to: alibaba-cn-hangzhou
        type: express-connect
  
  disasterRecovery:
    primaryRegion: aws-us-east-1
    secondaryRegion: azure-eastus
    failoverThreshold: 5m
```

### 云厂商集成安全基线

```bash
#!/bin/bash
# 云厂商集成安全检查脚本

SECURITY_REPORT="/var/log/kubernetes/cloud-security-report-$(date +%Y%m%d).log"

{
  echo "=== 云厂商集成安全检查报告 $(date) ==="
  
  # 1. 凭证安全检查
  echo "1. 凭证安全检查:"
  
  # 检查硬编码凭证
  if grep -r "access_key\|secret_key" /etc/kubernetes/ 2>/dev/null; then
    echo "⚠ 发现可能的硬编码凭证"
  else
    echo "✓ 未发现硬编码凭证"
  fi
  
  # 检查凭证权限范围
  echo "2. 凭证权限范围检查:"
  # 这里可以添加具体的权限检查逻辑
  
  # 3. 网络安全检查
  echo "3. 网络安全检查:"
  kubectl get networkpolicies --all-namespaces 2>/dev/null | wc -l
  
  # 4. 加密传输检查
  echo "4. 加密传输检查:"
  kubectl get secrets -n kube-system | grep -E "(tls|certificate)" | wc -l
  
} >> "$SECURITY_REPORT"

echo "安全检查报告已生成: $SECURITY_REPORT"
```

## 🔄 典型云厂商集成案例

### 案例一：AWS LoadBalancer 服务创建失败

**问题描述**：在 AWS EKS 集群中创建 LoadBalancer 类型的服务时，一直显示 `<pending>` 状态。

**根本原因**：EC2 实例缺少必要的 IAM 权限，无法创建 ELB 负载均衡器。

**解决方案**：
1. 为节点组附加正确的 IAM 策略
2. 确保安全组允许相关端口通信
3. 验证子网配置支持负载均衡器

### 案例二：Azure 托管标识权限不足

**问题描述**：使用 Workload Identity 的 Azure AKS 集群中，云控制器管理器无法创建公共 IP 地址。

**根本原因**：用户分配的托管标识缺少 Network Contributor 角色权限。

**解决方案**：
1. 为托管标识分配 Network Contributor 角色
2. 等待权限传播完成（通常需要几分钟）
3. 重启云控制器管理器 Pod

## 📞 云厂商支持

**官方文档**：
- AWS: https://docs.aws.amazon.com/eks/
- Azure: https://learn.microsoft.com/azure/aks/
- GCP: https://cloud.google.com/kubernetes-engine/docs
- 阿里云: https://help.aliyun.com/document_detail/86987.html

**社区支持**：
- Kubernetes Slack #cloud-provider 频道
- 各云厂商技术社区论坛
- CNCF 认证的云原生服务提供商