# 11-基础设施即代码

> **适用范围**: Kubernetes v1.25-v1.32 | **维护状态**: 🔧 持续更新中 | **专家级别**: ⭐⭐⭐⭐⭐

## 📋 概述

基础设施即代码(IaC)是现代云原生运维的核心实践。本文档详细介绍使用Terraform和Crossplane实现Kubernetes基础设施自动化的最佳实践。

## 🏗️ Terraform核心实践

### 模块化架构设计

#### 1. 核心模块结构
```
terraform-modules/
├── kubernetes-cluster/
│   ├── main.tf
│   ├── variables.tf
│   ├── outputs.tf
│   └── versions.tf
├── network/
│   ├── main.tf
│   ├── variables.tf
│   └── outputs.tf
├── storage/
│   ├── main.tf
│   └── variables.tf
└── security/
    ├── main.tf
    └── variables.tf
```

#### 2. Kubernetes集群模块
```hcl
# modules/kubernetes-cluster/main.tf
terraform {
  required_version = ">= 1.0"
  required_providers {
    kubernetes = {
      source  = "hashicorp/kubernetes"
      version = "~> 2.20"
    }
    helm = {
      source  = "hashicorp/helm"
      version = "~> 2.10"
    }
  }
}

resource "kubernetes_namespace" "namespaces" {
  for_each = var.namespaces
  
  metadata {
    name = each.key
    labels = merge(
      each.value.labels,
      { managed-by = "terraform" }
    )
  }
}

resource "helm_release" "ingress_controller" {
  count = var.enable_ingress ? 1 : 0
  
  name       = "nginx-ingress"
  repository = "https://kubernetes.github.io/ingress-nginx"
  chart      = "ingress-nginx"
  version    = var.ingress_version
  
  namespace = "ingress-nginx"
  
  set {
    name  = "controller.replicaCount"
    value = var.ingress_replicas
  }
  
  set {
    name  = "controller.service.type"
    value = "LoadBalancer"
  }
  
  depends_on = [kubernetes_namespace.namespaces]
}

# modules/kubernetes-cluster/variables.tf
variable "cluster_name" {
  description = "Name of the Kubernetes cluster"
  type        = string
}

variable "namespaces" {
  description = "Map of namespaces to create"
  type = map(object({
    labels = map(string)
  }))
  default = {}
}

variable "enable_ingress" {
  description = "Enable NGINX ingress controller"
  type        = bool
  default     = true
}

variable "ingress_version" {
  description = "NGINX ingress controller version"
  type        = string
  default     = "4.7.1"
}

variable "ingress_replicas" {
  description = "Number of ingress controller replicas"
  type        = number
  default     = 2
}
```

### 环境分离管理

#### 1. 多环境配置
```hcl
# environments/production/main.tf
module "production_cluster" {
  source = "../../modules/kubernetes-cluster"
  
  cluster_name = "production-cluster"
  
  namespaces = {
    production = {
      labels = {
        environment = "production"
        cost-center = "engineering"
      }
    }
    monitoring = {
      labels = {
        environment = "production"
        purpose     = "monitoring"
      }
    }
  }
  
  enable_ingress     = true
  ingress_replicas   = 3
  ingress_version    = "4.7.1"
}

module "production_network" {
  source = "../../modules/network"
  
  vpc_cidr           = "10.0.0.0/16"
  availability_zones = ["us-west-2a", "us-west-2b", "us-west-2c"]
  private_subnets    = ["10.0.1.0/24", "10.0.2.0/24", "10.0.3.0/24"]
  public_subnets     = ["10.0.101.0/24", "10.0.102.0/24", "10.0.103.0/24"]
}

# Backend配置
terraform {
  backend "s3" {
    bucket         = "terraform-state-production"
    key            = "production/cluster.tfstate"
    region         = "us-west-2"
    dynamodb_table = "terraform-state-lock"
    encrypt        = true
  }
}
```

#### 2. 变量文件管理
```hcl
# terraform.tfvars
# Production环境变量
cluster_name        = "production-cluster"
region             = "us-west-2"
instance_type      = "m5.large"
node_count         = 5
disk_size          = 100

# 网络配置
vpc_cidr           = "10.0.0.0/16"
cluster_ipv4_cidr  = "172.20.0.0/16"

# 安全配置
allowed_ips        = ["203.0.113.0/24", "198.51.100.0/24"]
enable_ssh_access  = false

# 监控配置
enable_monitoring  = true
monitoring_version = "44.3.0"
```

## 🛠️ Crossplane实践

### Crossplane基础配置

#### 1. Crossplane安装
```yaml
# Crossplane安装配置
apiVersion: v1
kind: Namespace
metadata:
  name: crossplane-system
---
apiVersion: helm.crossplane.io/v1beta1
kind: Provider
metadata:
  name: provider-aws
spec:
  package: xpkg.upbound.io/upbound/provider-aws:v0.38.0
  controllerConfigRef:
    name: aws-config
---
apiVersion: helm.crossplane.io/v1alpha1
kind: ControllerConfig
metadata:
  name: aws-config
spec:
  args:
  - --debug
  metadata:
    annotations:
      iam.amazonaws.com/role: crossplane-role
```

#### 2. 云资源声明
```yaml
# S3存储桶配置
apiVersion: s3.aws.upbound.io/v1beta1
kind: Bucket
metadata:
  name: app-storage-bucket
  namespace: crossplane-system
spec:
  forProvider:
    region: us-west-2
    acl: private
    versioning:
    - enabled: true
    serverSideEncryptionConfiguration:
    - rule:
      - applyServerSideEncryptionByDefault:
        - sseAlgorithm: AES256
  providerConfigRef:
    name: aws-provider-config
---
# RDS数据库实例
apiVersion: rds.aws.upbound.io/v1beta1
kind: Instance
metadata:
  name: app-database
  namespace: crossplane-system
spec:
  forProvider:
    region: us-west-2
    instanceClass: db.t3.medium
    engine: postgres
    engineVersion: "15.3"
    allocatedStorage: 20
    dbName: appdb
    username: admin
    passwordSecretRef:
      name: db-password
      namespace: crossplane-system
      key: password
    publiclyAccessible: false
    skipFinalSnapshot: true
    backupRetentionPeriod: 7
    backupWindow: "03:00-04:00"
    maintenanceWindow: "sun:04:00-sun:05:00"
  providerConfigRef:
    name: aws-provider-config
```

### Composition组合模式

#### 1. 基础设施组合
```yaml
# Kubernetes集群组合
apiVersion: apiextensions.crossplane.io/v1
kind: CompositeResourceDefinition
metadata:
  name: xeksclusters.example.org
spec:
  group: example.org
  names:
    kind: XEKSCluster
    plural: xeksclusters
  claimNames:
    kind: EKSCluster
    plural: eksclusters
  versions:
  - name: v1alpha1
    served: true
    referenceable: true
    schema:
      openAPIV3Schema:
        type: object
        properties:
          spec:
            type: object
            properties:
              parameters:
                type: object
                properties:
                  region:
                    type: string
                  version:
                    type: string
                  nodeSize:
                    type: string
                  nodeCount:
                    type: integer
                required:
                - region
                - version
---
apiVersion: apiextensions.crossplane.io/v1
kind: Composition
metadata:
  name: eks-cluster-composition
  labels:
    provider: aws
spec:
  compositeTypeRef:
    apiVersion: example.org/v1alpha1
    kind: XEKSCluster
  resources:
  - name: eks-cluster
    base:
      apiVersion: eks.aws.upbound.io/v1beta1
      kind: Cluster
      spec:
        forProvider:
          roleArnSelector:
            matchControllerRef: true
          vpcConfig:
          - endpointPrivateAccess: true
          - endpointPublicAccess: true
    patches:
    - type: FromCompositeFieldPath
      fromFieldPath: spec.parameters.region
      toFieldPath: spec.forProvider.region
    - type: FromCompositeFieldPath
      fromFieldPath: spec.parameters.version
      toFieldPath: spec.forProvider.version
```

#### 2. 应用环境组合
```yaml
# 应用环境Composition
apiVersion: apiextensions.crossplane.io/v1
kind: Composition
metadata:
  name: application-environment
spec:
  compositeTypeRef:
    apiVersion: example.org/v1alpha1
    kind: XApplicationEnvironment
  resources:
  - name: namespace
    base:
      apiVersion: kubernetes.crossplane.io/v1alpha1
      kind: Object
      spec:
        forProvider:
          manifest:
            apiVersion: v1
            kind: Namespace
    patches:
    - type: FromCompositeFieldPath
      fromFieldPath: spec.parameters.environmentName
      toFieldPath: spec.forProvider.manifest.metadata.name
      
  - name: resource-quota
    base:
      apiVersion: kubernetes.crossplane.io/v1alpha1
      kind: Object
      spec:
        forProvider:
          manifest:
            apiVersion: v1
            kind: ResourceQuota
            spec:
              hard:
                requests.cpu: "4"
                requests.memory: 8Gi
                limits.cpu: "8"
                limits.memory: 16Gi
    patches:
    - type: FromCompositeFieldPath
      fromFieldPath: spec.parameters.environmentName
      toFieldPath: spec.forProvider.manifest.metadata.namespace
      
  - name: limit-range
    base:
      apiVersion: kubernetes.crossplane.io/v1alpha1
      kind: Object
      spec:
        forProvider:
          manifest:
            apiVersion: v1
            kind: LimitRange
            spec:
              limits:
              - default:
                  cpu: 500m
                  memory: 512Mi
                defaultRequest:
                  cpu: 100m
                  memory: 128Mi
                type: Container
    patches:
    - type: FromCompositeFieldPath
      fromFieldPath: spec.parameters.environmentName
      toFieldPath: spec.forProvider.manifest.metadata.namespace
```

## 🔧 自动化流水线

### CI/CD集成

#### 1. GitHub Actions配置
```yaml
# .github/workflows/terraform-plan.yml
name: Terraform Plan
on:
  pull_request:
    branches: [ main ]
    paths:
    - 'terraform/**'

jobs:
  terraform-plan:
    runs-on: ubuntu-latest
    steps:
    - name: Checkout
      uses: actions/checkout@v3
      
    - name: Setup Terraform
      uses: hashicorp/setup-terraform@v2
      with:
        terraform_version: 1.5.7
        
    - name: Terraform Init
      run: terraform init
      working-directory: terraform/environments/staging
      
    - name: Terraform Validate
      run: terraform validate
      working-directory: terraform/environments/staging
      
    - name: Terraform Plan
      run: terraform plan -no-color
      working-directory: terraform/environments/staging
      env:
        AWS_ACCESS_KEY_ID: ${{ secrets.AWS_ACCESS_KEY_ID }}
        AWS_SECRET_ACCESS_KEY: ${{ secrets.AWS_SECRET_ACCESS_KEY }}
```

#### 2. Atlantis自动化
```yaml
# atlantis.yaml
version: 3
projects:
- name: staging
  dir: terraform/environments/staging
  workflow: terragrunt
  autoplan:
    when_modified: ["../modules/**/*.tf", "*.tf*"]
  apply_requirements: [approved]
  
- name: production
  dir: terraform/environments/production
  workflow: terragrunt
  autoplan:
    when_modified: ["../modules/**/*.tf", "*.tf*"]
  apply_requirements: [mergeable, approved]
  
workflows:
  terragrunt:
    plan:
      steps:
      - env:
          name: TERRAGRUNT_TFPATH
          value: terraform
      - run: terragrunt plan -no-color -out $PLANFILE
    apply:
      steps:
      - env:
          name: TERRAGRUNT_TFPATH
          value: terraform
      - run: terragrunt apply -no-color $PLANFILE
```

### 状态管理策略

#### 1. 远程状态配置
```hcl
# backend.tf
terraform {
  backend "s3" {
    bucket         = "company-terraform-state"
    key            = "kubernetes-clusters/production.tfstate"
    region         = "us-west-2"
    dynamodb_table = "terraform-state-lock"
    encrypt        = true
    
    # 状态文件加密
    kms_key_id = "arn:aws:kms:us-west-2:123456789012:key/abcd1234-a123-456a-a12b-a123b4cd56ef"
  }
}

# 状态锁定配置
resource "aws_dynamodb_table" "terraform_state_lock" {
  name           = "terraform-state-lock"
  billing_mode   = "PAY_PER_REQUEST"
  hash_key       = "LockID"
  
  attribute {
    name = "LockID"
    type = "S"
  }
  
  server_side_encryption {
    enabled = true
  }
  
  point_in_time_recovery {
    enabled = true
  }
}
```

#### 2. 状态分割策略
```hcl
# foundation/main.tf - 基础设施层
terraform {
  backend "s3" {
    bucket = "terraform-state"
    key    = "foundation/us-west-2.tfstate"
    region = "us-west-2"
  }
}

module "network" {
  source = "../modules/network"
  # 网络配置
}

module "security" {
  source = "../modules/security"
  # 安全配置
}

# applications/main.tf - 应用层
terraform {
  backend "s3" {
    bucket = "terraform-state"
    key    = "applications/production.tfstate"
    region = "us-west-2"
  }
}

data "terraform_remote_state" "foundation" {
  backend = "s3"
  config = {
    bucket = "terraform-state"
    key    = "foundation/us-west-2.tfstate"
    region = "us-west-2"
  }
}

module "kubernetes_apps" {
  source = "../modules/kubernetes-apps"
  
  vpc_id     = data.terraform_remote_state.foundation.outputs.vpc_id
  subnet_ids = data.terraform_remote_state.foundation.outputs.private_subnet_ids
}
```

## 📊 监控与合规

### 基础设施监控

#### 1. Terraform监控指标
```yaml
# Prometheus指标收集
apiVersion: monitoring.coreos.com/v1
kind: ServiceMonitor
metadata:
  name: terraform-execution-monitor
  namespace: monitoring
spec:
  selector:
    matchLabels:
      app: terraform-executor
  endpoints:
  - port: metrics
    path: /metrics
    interval: 30s
---
# 自定义指标收集器
apiVersion: apps/v1
kind: Deployment
metadata:
  name: tf-metrics-collector
  namespace: monitoring
spec:
  replicas: 1
  selector:
    matchLabels:
      app: tf-metrics-collector
  template:
    metadata:
      labels:
        app: tf-metrics-collector
    spec:
      containers:
      - name: collector
        image: custom/tf-metrics-collector:latest
        ports:
        - containerPort: 8080
          name: metrics
        env:
        - name: TERRAFORM_STATE_BUCKET
          value: "company-terraform-state"
        - name: AWS_REGION
          value: "us-west-2"
        resources:
          requests:
            cpu: 100m
            memory: 128Mi
          limits:
            cpu: 200m
            memory: 256Mi
```

#### 2. 合规性检查
```yaml
# Terrascan配置
apiVersion: batch/v1
kind: CronJob
metadata:
  name: terraform-compliance-check
  namespace: security
spec:
  schedule: "0 2 * * *"  # 每天凌晨2点执行
  jobTemplate:
    spec:
      template:
        spec:
          containers:
          - name: terrascan
            image: tenable/terrascan:latest
            command:
            - /bin/sh
            - -c
            - |
              terrascan scan -i terraform -t aws -f /terraform -o json > /results/compliance-report.json
              
              # 检查严重违规
              if jq '.results | length > 0' /results/compliance-report.json; then
                echo "COMPLIANCE VIOLATIONS DETECTED"
                exit 1
              fi
            volumeMounts:
            - name: terraform-code
              mountPath: /terraform
            - name: results
              mountPath: /results
          volumes:
          - name: terraform-code
            gitRepo:
              repository: "https://github.com/company/terraform-modules.git"
              revision: "main"
          - name: results
            emptyDir: {}
          restartPolicy: Never
```

### 成本优化监控

#### 1. 资源利用率分析
```python
#!/usr/bin/env python3
# 资源利用率分析脚本

import boto3
import json
from datetime import datetime, timedelta

def analyze_ec2_utilization():
    """分析EC2实例利用率"""
    ec2 = boto3.client('ec2')
    cloudwatch = boto3.client('cloudwatch')
    
    # 获取所有运行中的实例
    instances = ec2.describe_instances(
        Filters=[{'Name': 'instance-state-name', 'Values': ['running']}]
    )
    
    utilization_data = []
    
    for reservation in instances['Reservations']:
        for instance in reservation['Instances']:
            instance_id = instance['InstanceId']
            
            # 获取CPU利用率
            cpu_metrics = cloudwatch.get_metric_statistics(
                Namespace='AWS/EC2',
                MetricName='CPUUtilization',
                Dimensions=[{'Name': 'InstanceId', 'Value': instance_id}],
                StartTime=datetime.utcnow() - timedelta(days=7),
                EndTime=datetime.utcnow(),
                Period=3600,
                Statistics=['Average']
            )
            
            avg_cpu = sum(point['Average'] for point in cpu_metrics['Datapoints']) / len(cpu_metrics['Datapoints']) if cpu_metrics['Datapoints'] else 0
            
            utilization_data.append({
                'instance_id': instance_id,
                'instance_type': instance['InstanceType'],
                'avg_cpu_utilization': avg_cpu,
                'recommendation': get_recommendation(avg_cpu, instance['InstanceType'])
            })
    
    return utilization_data

def get_recommendation(cpu_utilization, instance_type):
    """根据利用率提供建议"""
    if cpu_utilization < 10:
        return f"Consider downsizing from {instance_type}"
    elif cpu_utilization > 80:
        return f"Consider upsizing from {instance_type}"
    else:
        return "Instance sizing appropriate"

# 使用示例
if __name__ == "__main__":
    utilization_report = analyze_ec2_utilization()
    
    print("EC2 Utilization Report:")
    print(json.dumps(utilization_report, indent=2))
    
    # 生成优化建议
    recommendations = [item for item in utilization_report if item['recommendation'] != "Instance sizing appropriate"]
    if recommendations:
        print("\nOptimization Recommendations:")
        for rec in recommendations:
            print(f"- {rec['instance_id']}: {rec['recommendation']}")
```

## 🔐 安全最佳实践

### 权限管理

#### 1. 最小权限原则
```hcl
# IAM策略配置
resource "aws_iam_policy" "terraform_deployer" {
  name        = "terraform-deployer-policy"
  description = "Minimal permissions for Terraform deployments"
  
  policy = jsonencode({
    Version = "2012-10-17"
    Statement = [
      {
        Effect = "Allow"
        Action = [
          "ec2:*",
          "eks:*",
          "iam:PassRole",
          "iam:GetRole",
          "iam:ListAttachedRolePolicies"
        ]
        Resource = "*"
        Condition = {
          "StringEquals": {
            "aws:RequestedRegion": "us-west-2"
          }
        }
      }
    ]
  })
}

# 假设角色配置
resource "aws_iam_role" "terraform_executor" {
  name = "terraform-executor-role"
  
  assume_role_policy = jsonencode({
    Version = "2012-10-17"
    Statement = [
      {
        Effect = "Allow"
        Principal = {
          Service = "ec2.amazonaws.com"
        }
        Action = "sts:AssumeRole"
      }
    ]
  })
  
  managed_policy_arns = [aws_iam_policy.terraform_deployer.arn]
}
```

#### 2. 敏感信息管理
```hcl
# Vault集成配置
provider "vault" {
  address = "https://vault.example.com:8200"
  token   = var.vault_token
}

data "vault_generic_secret" "aws_credentials" {
  path = "secret/aws/terraform"
}

provider "aws" {
  region     = var.aws_region
  access_key = data.vault_generic_secret.aws_credentials.data.access_key
  secret_key = data.vault_generic_secret.aws_credentials.data.secret_key
}

# SOPS加密配置
# .sops.yaml
creation_rules:
  - path_regex: \.yaml$
    kms: 'arn:aws:kms:us-west-2:123456789012:key/abcd1234-a123-456a-a12b-a123b4cd56ef'
    pgp: 'your-email@example.com'
```

## 🔧 实施检查清单

### 基础设施代码化
- [ ] 设计模块化Terraform架构
- [ ] 建立多环境配置管理
- [ ] 配置远程状态存储和锁定
- [ ] 实施Crossplane云资源管理
- [ ] 建立基础设施组合模式
- [ ] 配置自动化部署流水线

### 安全与合规
- [ ] 实施最小权限访问控制
- [ ] 配置敏感信息加密存储
- [ ] 建立安全合规检查机制
- [ ] 实施基础设施审计日志
- [ ] 配置资源访问策略
- [ ] 建立安全基线检查

### 监控与优化
- [ ] 部署基础设施监控系统
- [ ] 建立成本优化分析机制
- [ ] 配置资源利用率监控
- [ ] 实施性能基准测试
- [ ] 建立变更影响评估
- [ ] 维护基础设施文档

### 运营维护
- [ ] 建立版本控制和变更管理
- [ ] 配置自动化测试和验证
- [ ] 实施故障恢复和回滚机制
- [ ] 建立运维操作手册
- [ ] 定期进行架构评审
- [ ] 持续优化基础设施代码

---

*本文档为企业级基础设施即代码实践提供完整的技术方案和实施指导*