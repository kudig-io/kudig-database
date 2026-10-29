---
title: GKE 安全体系 — Workload Identity 与安全加固
description: 'GKE Workload Identity 配置、Binary Authorization、gVisor Sandbox、Shielded Node 及 VPC Service Controls'
summary: 'GKE Workload Identity 配置、Binary Authorization、gVisor Sandbox、Shielded Node 及 VPC Service Controls'
category: cloud-providers
tags:
- cloud
- k8s
- gcp
- gke
- security
- workload-identity
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
- GKE Workload Identity 是什么
- 如何配置 GKE 安全加固
trigger_keywords:
- workload-identity
- binary-authorization
- gvisor
- shielded-node
- vpc-service-controls
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


# GKE 安全体系 — Workload Identity 与安全加固

## 1. Workload Identity

Workload Identity 是 GKE 推荐的方式，将 Kubernetes Service Account (KSA) 映射到 Google Cloud Service Account (GSA)，替代 Node Pool Service Account。

### 1.1 启用 Workload Identity

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 创建集群时启用
gcloud container clusters create prod-cluster \
  --region=asia-southeast1 \
  --workload-pool=my-project.svc.id.goog

# 现有集群启用
gcloud container clusters update prod-cluster \
  --region=asia-southeast1 \
  --workload-pool=my-project.svc.id.goog

# 节点池启用 Workload Identity（现有节点需要迁移）
gcloud container node-pools update default-pool \
  --cluster=prod-cluster \
  --region=asia-southeast1 \
  --workload-metadata=GKE_METADATA
```
```hcl
# Terraform
resource "google_container_cluster" "prod" {
  name     = "prod-cluster"
  location = "asia-southeast1"

  workload_identity_config {
    workload_pool = "my-project.svc.id.goog"
  }

  node_config {
    workload_metadata_config {
      mode = "GKE_METADATA"
    }
  }
}
```

### 1.2 绑定 KSA 到 GSA

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 创建 GCP Service Account
gcloud iam service-accounts create gke-app-sa \
  --display-name="GKE App SA"

# 授权 GSA 访问特定资源
gcloud projects add-iam-policy-binding my-project \
  --member="serviceAccount:gke-app-sa@my-project.iam.gserviceaccount.com" \
  --role="roles/storage.objectViewer"

gcloud projects add-iam-policy-binding my-project \
  --member="serviceAccount:gke-app-sa@my-project.iam.gserviceaccount.com" \
  --role="roles/bigquery.dataViewer"

# 绑定 KSA 到 GSA
gcloud iam service-accounts add-iam-policy-binding \
  gke-app-sa@my-project.iam.gserviceaccount.com \
  --role="roles/iam.workloadIdentityUser" \
  --member="serviceAccount:my-project.svc.id.goog[production/app-ksa]"
```
```yaml
# Kubernetes Service Account
apiVersion: v1
kind: ServiceAccount
metadata:
  name: app-ksa
  namespace: production
  annotations:
    iam.gke.io/gcp-service-account: gke-app-sa@my-project.iam.gserviceaccount.com
```

### 1.3 Pod 使用 Workload Identity

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: data-app
  namespace: production
spec:
  replicas: 3
  selector:
    matchLabels:
      app: data-app
  template:
    metadata:
      labels:
        app: data-app
    spec:
      serviceAccountName: app-ksa  # 使用绑定的 KSA
      containers:
        - name: app
          image: gcr.io/my-project/data-app:v1.2.3
          command: ["/app"]
          env:
            # 自动注入 GOOGLE_APPLICATION_CREDENTIALS
            - name: GOOGLE_CLOUD_PROJECT
              value: my-project
```

### 1.4 Workload Identity vs Node SA

| 特性 | Workload Identity | Node Pool SA |
|------|------------------|-------------|
| 粒度 | Pod/Service Account 级别 | 节点级别 |
| 安全性 | 最小权限 | 节点上所有 Pod 共享权限 |
| 审计 | 可追溯到具体 KSA | 仅追溯到节点 SA |
| 配置复杂度 | 中等 | 简单 |
| 推荐场景 | 生产环境 | 快速测试 |

## 2. Binary Authorization

Binary Authorization 确保只有经过签名的镜像才能部署到 GKE。

### 2.1 启用 Binary Authorization

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 启用 API
gcloud services enable binaryauthorization.googleapis.com

# 创建集群时启用
gcloud container clusters create prod-cluster \
  --region=asia-southeast1 \
  --binauthz-evaluation-mode=PROJECT_SINGLETON_POLICY_ENFORCE
```
### 2.2 创建策略

```yaml
# binauthz-policy.yaml
admissionWhitelistPatterns:
  - namePattern: "gcr.io/gke-release/*"
  - namePattern: "gke.gcr.io/*"
  - namePattern: "k8s.gcr.io/*"

defaultAdmissionRule:
  requireAttestationsBy:
    - projects/my-project/attestors/prod-attestor
  enforcementMode: ENFORCED_BLOCK_AND_AUDIT_LOG
  evaluationMode: REQUIRE_ATTESTATION

globalPolicyEvaluationMode: ENABLE
```

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 导入策略
gcloud container binauthz policy import binauthz-policy.yaml

# 创建 Attestor
gcloud container binauthz attestors create prod-attestor \
  --attestation-authority-note-project=my-project \
  --attestation-authority-note=prod-note

# 添加 PGP 公钥
gcloud container binauthz attestors public-keys add \
  --attestor=prod-attestor \
  --pgp-public-key-file=attestor.pub
```
### 2.3 CI/CD 集成

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 在 CI/CD 中签名镜像
gcloud container binauthz attestations sign-and-create \
  --artifact-url=gcr.io/my-project/app@sha256:abc123 \
  --attestor=prod-attestor \
  --attestor-project=my-project \
  --pgp-private-key-file=key.asc \
  --pgp-key-fingerprint=AAAA1234

# 验证签名
gcloud container binauthz attestations list \
  --attestor=prod-attestor \
  --artifact-url=gcr.io/my-project/app@sha256:abc123
```
## 3. Sandbox (gVisor)

### 3.1 启用 GKE Sandbox

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 创建使用 gVisor 的节点池
gcloud container node-pools create sandbox-pool \
  --cluster=prod-cluster \
  --region=asia-southeast1 \
  --sandbox=type=gvisor \
  --machine-type=e2-standard-4 \
  --num-nodes=3
```
```yaml
# Pod 使用 gVisor RuntimeClass
apiVersion: node.k8s.io/v1
kind: RuntimeClass
metadata:
  name: gvisor
handler: gvisor
scheduling:
  nodeSelector:
    sandbox.gke.io/runtime: gvisor

---
apiVersion: apps/v1
kind: Deployment
metadata:
  name: untrusted-workload
  namespace: sandbox
spec:
  template:
    spec:
      runtimeClassName: gvisor
      containers:
        - name: worker
          image: gcr.io/my-project/untrusted-worker:latest
          securityContext:
            runAsNonRoot: true
            readOnlyRootFilesystem: true
          resources:
            requests:
              cpu: "500m"
              memory: "512Mi"
```

### 3.2 gVisor 限制

| 特性 | 支持状态 | 说明 |
|------|---------|------|
| 网络 | ✅ 完全支持 | 标准 Kubernetes 网络 |
| 存储 | ✅ 支持 | PD、emptyDir、ConfigMap |
| 特权容器 | ❌ 不支持 | 安全设计限制 |
| hostNetwork | ❌ 不支持 | 安全设计限制 |
| GPU | ❌ 不支持 | gVisor 不支持 GPU 直通 |
| 性能 | ⚠️ 降低约 5-10% | 系统调用开销 |

## 4. Shielded GKE Node

### 4.1 启用 Shielded Node

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 创建集群时启用
gcloud container clusters create prod-cluster \
  --region=asia-southeast1 \
  --shielded-secure-boot \
  --shielded-integrity-monitoring

# 现有节点池启用
gcloud container node-pools update default-pool \
  --cluster=prod-cluster \
  --region=asia-southeast1 \
  --shielded-secure-boot \
  --shielded-integrity-monitoring
```
```hcl
# Terraform
resource "google_container_cluster" "prod" {
  name     = "prod-cluster"
  location = "asia-southeast1"

  node_config {
    shielded_instance_config {
      enable_secure_boot          = true
      enable_integrity_monitoring = true
    }
  }
}
```

### 4.2 Shielded Node 特性

| 特性 | 说明 | 验证方式 |
|------|------|---------|
| Secure Boot | 验证驱动和内核签名 | `mokutil --sb-state` |
| vTPM | 虚拟可信平台模块 | `/dev/tpm0` 存在 |
| Integrity Monitoring | 节点完整性验证 | GKE 安全态势仪表盘 |

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 验证节点安全状态
gcloud container clusters describe prod-cluster \
  --region=asia-southeast1 \
  --format="json(nodeConfig.shieldedInstanceConfig)"

# 查看节点完整性报告
kubectl get nodes -o json | jq '.items[] | {
  name: .metadata.name,
  labels: .metadata.labels | to_entries[] | select(.key | startswith("node.gke.io"))
}'
```
## 5. VPC Service Controls

### 5.1 配置 VPC SC

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 创建 Access Policy
gcloud access-context-manager policies create \
  --title="GKE Security Policy" \
  --scopes="projects/123456789012"

# 创建 Service Perimeter
gcloud access-context-manager perimeters create prod-perimeter \
  --title="Production Perimeter" \
  --resources="projects/123456789012" \
  --restricted-services="container.googleapis.com,storage.googleapis.com,bigquery.googleapis.com" \
  --policy=123456789

# 配置 Ingress 规则（允许外部 CI/CD 访问）
gcloud access-context-manager levels create trusted-ci \
  --title="Trusted CI Access" \
  --basic-level-spec='{"conditions":[{"ipSubnetworks":["10.0.0.0/8"]}]}'

gcloud access-context-manager perimeters add-ingress-policy prod-perimeter \
  --ingress-from-sources="access_level:trusted-ci" \
  --ingress-to-services="container.googleapis.com" \
  --policy=123456789
```
### 5.2 GKE 与 VPC SC 集成

```yaml
# 通过 Private Cluster + VPC SC 实现完全私有
# 控制平面只能通过 VPC 内部访问
# API 调用必须来自 VPC SC 内部
```

## 6. Pod Security Standards

### 6.1 GKE 内置 Pod Security

```yaml
# GKE 自动应用 Pod Security Standards
# 集群级别标签控制
apiVersion: v1
kind: Namespace
metadata:
  name: production
  labels:
    pod-security.kubernetes.io/enforce: restricted
    pod-security.kubernetes.io/audit: restricted
    pod-security.kubernetes.io/warn: restricted
```

### 6.2 自定义 OPA/Gatekeeper 策略

```yaml
# 禁止特权容器
apiVersion: constraints.gatekeeper.sh/v1beta1
kind: K8sPSPPrivilegedContainer
metadata:
  name: deny-privileged
spec:
  match:
    kinds:
      - apiGroups: [""]
        kinds: ["Pod"]
    namespaces:
      - production
      - staging
  parameters:
    exemptImages:
      - "gcr.io/google-containers/pause:*"

---
# 强制资源限制
apiVersion: constraints.gatekeeper.sh/v1beta1
kind: K8sContainerLimits
metadata:
  name: require-resource-limits
spec:
  match:
    kinds:
      - apiGroups: [""]
        kinds: ["Pod"]
  parameters:
    cpu: "8"
    memory: "16Gi"
```

## 7. 安全审计

### 7.1 Admin Activity 日志

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看 GKE 审计日志
gcloud logging read \
  'resource.type="k8s_cluster" AND resource.labels.cluster_name="prod-cluster"' \
  --limit=50 \
  --format="table(timestamp, protoPayload.methodName, protoPayload.authenticationInfo.principalEmail)"
```
### 7.2 Data Access 日志

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 启用 Data Access 审计日志
# 通过 GKE Enterprise 或手动配置 Audit Policy

# 查看 API 访问日志
gcloud logging read \
  'resource.type="k8s_cluster" AND protoPayload.methodName=~"get|list|watch"' \
  --limit=100 \
  --format=json
```
### 7.3 安全态势仪表盘

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 启用 Security Posture（GKE Enterprise）
gcloud container clusters update prod-cluster \
  --region=asia-southeast1 \
  --security-posture=standard

# 查看安全发现
gcloud container clusters describe prod-cluster \
  --region=asia-southeast1 \
  --format="json(securityPostureConfig)"
```
## 8. 安全最佳实践清单

```
GKE 安全加固清单:

□ 启用 Workload Identity（替代 Node SA）
□ 启用 Private Cluster（禁用公共端点）
□ 启用 Dataplane V2（eBPF Network Policy）
□ 启用 Shielded Node（Secure Boot + Integrity Monitoring）
□ 配置 Binary Authorization（镜像签名验证）
□ 启用 Pod Security Standards（restricted 级别）
□ 配置 Network Policy（默认拒绝）
□ 启用 VPC Service Controls（API 安全边界）
□ 启用审计日志（Admin Activity + Data Access）
□ 禁用基本认证和客户端证书
□ 配置 Master Authorized Networks
□ 使用 GKE Sandbox 处理不可信工作负载
```

## Related

- [[01-gke-autopilot-serverless]]
- [[02-gke-networking-dataplane-v2]]

## See Also

- GKE Security 文档
- Workload Identity
- Binary Authorization


<!-- risk-assessed -->
