# 51 - 容器镜像管理与仓库 (Container Images & Registry)

> **适用版本**: v1.25 - v1.32 | **最后更新**: 2026-01 | **难度**: 中级

## 容器镜像生态架构

```
┌─────────────────────────────────────────────────────────────────────────────────────┐
│                        容器镜像全生命周期管理                                         │
├─────────────────────────────────────────────────────────────────────────────────────┤
│                                                                                      │
│  ┌──────────────────────────────────────────────────────────────────────────────┐   │
│  │                          构建阶段 (Build Phase)                               │   │
│  │                                                                               │   │
│  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐  ┌─────────────────────┐  │   │
│  │  │ Dockerfile  │  │   Kaniko    │  │  Buildah    │  │      BuildKit       │  │   │
│  │  │   (传统)    │  │ (K8s原生)   │  │ (Rootless)  │  │     (并行构建)      │  │   │
│  │  └──────┬──────┘  └──────┬──────┘  └──────┬──────┘  └──────────┬──────────┘  │   │
│  └─────────┼────────────────┼────────────────┼───────────────────┼─────────────┘   │
│            │                │                │                   │                  │
│            └────────────────┴────────────────┴───────────────────┘                  │
│                                      │                                              │
│                                      ▼                                              │
│  ┌──────────────────────────────────────────────────────────────────────────────┐   │
│  │                          扫描阶段 (Scan Phase)                                │   │
│  │                                                                               │   │
│  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐  ┌─────────────────────┐  │   │
│  │  │   Trivy     │  │   Grype     │  │   Snyk      │  │      Anchore        │  │   │
│  │  │  (全栈扫描) │  │  (SBOM)     │  │  (SaaS)     │  │    (企业级)         │  │   │
│  │  └─────────────┘  └─────────────┘  └─────────────┘  └─────────────────────┘  │   │
│  └──────────────────────────────────────────────────────────────────────────────┘   │
│                                      │                                              │
│                                      ▼                                              │
│  ┌──────────────────────────────────────────────────────────────────────────────┐   │
│  │                          签名阶段 (Sign Phase)                                │   │
│  │                                                                               │   │
│  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐  ┌─────────────────────┐  │   │
│  │  │   Cosign    │  │  Notary v2  │  │   Sigstore  │  │       SBOM          │  │   │
│  │  │  (Keyless)  │  │   (OCI)     │  │  (透明日志) │  │   (物料清单)        │  │   │
│  │  └─────────────┘  └─────────────┘  └─────────────┘  └─────────────────────┘  │   │
│  └──────────────────────────────────────────────────────────────────────────────┘   │
│                                      │                                              │
│                                      ▼                                              │
│  ┌──────────────────────────────────────────────────────────────────────────────┐   │
│  │                          存储阶段 (Store Phase)                               │   │
│  │                                                                               │   │
│  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐  ┌─────────────────────┐  │   │
│  │  │   Harbor    │  │    ACR      │  │    ECR      │  │       GCR           │  │   │
│  │  │  (企业级)   │  │  (阿里云)   │  │   (AWS)     │  │      (GCP)          │  │   │
│  │  └─────────────┘  └─────────────┘  └─────────────┘  └─────────────────────┘  │   │
│  └──────────────────────────────────────────────────────────────────────────────┘   │
│                                      │                                              │
│                                      ▼                                              │
│  ┌──────────────────────────────────────────────────────────────────────────────┐   │
│  │                          分发阶段 (Distribute Phase)                          │   │
│  │                                                                               │   │
│  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐  ┌─────────────────────┐  │   │
│  │  │ Dragonfly   │  │   Kraken    │  │   Nydus     │  │     镜像预热        │  │   │
│  │  │  (P2P分发)  │  │  (Uber P2P) │  │  (懒加载)   │  │   (Job预拉取)       │  │   │
│  │  └─────────────┘  └─────────────┘  └─────────────┘  └─────────────────────┘  │   │
│  └──────────────────────────────────────────────────────────────────────────────┘   │
│                                      │                                              │
│                                      ▼                                              │
│  ┌──────────────────────────────────────────────────────────────────────────────┐   │
│  │                          运行阶段 (Run Phase)                                 │   │
│  │                                                                               │   │
│  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐  ┌─────────────────────┐  │   │
│  │  │ containerd  │  │    CRI-O    │  │   Podman    │  │      Kata/gVisor    │  │   │
│  │  │  (Runtime)  │  │  (Runtime)  │  │ (Rootless)  │  │    (安全沙箱)       │  │   │
│  │  └─────────────┘  └─────────────┘  └─────────────┘  └─────────────────────┘  │   │
│  └──────────────────────────────────────────────────────────────────────────────┘   │
│                                                                                      │
└─────────────────────────────────────────────────────────────────────────────────────┘
```

## 镜像命名规范与最佳实践

### 完整镜像地址格式

| 组成部分 | 示例 | 说明 | 最佳实践 |
|---------|-----|------|----------|
| Registry | docker.io, registry.cn-hangzhou.aliyuncs.com | 镜像仓库地址 | 使用私有仓库 |
| Namespace | library, myproject | 命名空间/项目 | 按团队/环境划分 |
| Repository | nginx, myapp | 镜像名称 | 语义化命名 |
| Tag | v1.0.0, sha256:abc123 | 版本标签 | 使用语义化版本 |
| Digest | sha256:abc123... | 不可变标识 | 生产环境使用 |
| 完整格式 | registry/namespace/repo:tag@digest | 完整镜像地址 | 生产推荐格式 |

### 镜像标签策略

```yaml
# 推荐的标签策略
tags:
  # 语义化版本 (推荐生产使用)
  - v1.2.3                    # 精确版本
  - v1.2                      # 次版本
  - v1                        # 主版本
  
  # Git 相关
  - sha-abc1234               # Git commit SHA
  - main-abc1234              # 分支+commit
  
  # 构建相关
  - build-123                 # CI 构建号
  - 2024.01.15-abc1234        # 日期+commit
  
  # 环境标识
  - latest                    # 最新版本 (仅开发环境)
  - staging                   # 预发布环境
  - production                # 生产环境 (不推荐)

# 标签命名规范
naming_conventions:
  # 好的实践
  - myapp:v1.2.3
  - myapp:v1.2.3-alpine
  - myapp:v1.2.3-amd64
  - myapp:v1.2.3-20240115
  
  # 避免的实践
  - myapp:latest              # 不可复现
  - myapp:stable              # 语义不明确
  - myapp:new                 # 无版本信息
```

## 镜像拉取策略详解

| 策略 | 说明 | 适用场景 | 配置示例 |
|-----|------|---------|---------|
| **Always** | 始终从仓库拉取 | latest 标签、频繁更新 | `imagePullPolicy: Always` |
| **IfNotPresent** | 本地不存在时拉取 | 固定版本标签 (推荐) | `imagePullPolicy: IfNotPresent` |
| **Never** | 从不拉取，仅用本地 | 本地预加载、离线环境 | `imagePullPolicy: Never` |

```yaml
# 完整的镜像拉取配置示例
apiVersion: v1
kind: Pod
metadata:
  name: myapp
  namespace: production
spec:
  # 镜像拉取 Secret (推荐通过 ServiceAccount)
  imagePullSecrets:
    - name: acr-credentials
  
  containers:
    - name: app
      # 使用 digest 确保镜像不可变 (生产推荐)
      image: registry.cn-hangzhou.aliyuncs.com/myns/myapp:v1.2.3@sha256:abc123...
      imagePullPolicy: IfNotPresent
      
      # 安全上下文
      securityContext:
        runAsNonRoot: true
        runAsUser: 1000
        readOnlyRootFilesystem: true
        allowPrivilegeEscalation: false
        capabilities:
          drop:
            - ALL
```

## 镜像仓库认证配置

### 1. Docker Registry Secret 创建

```bash
# ==================== 命令行创建 ====================

# 基础创建
kubectl create secret docker-registry regcred \
  --docker-server=registry.cn-hangzhou.aliyuncs.com \
  --docker-username=user@example.com \
  --docker-password='your-password' \
  --docker-email=user@example.com \
  -n production

# 从已有 Docker 配置创建
kubectl create secret generic regcred \
  --from-file=.dockerconfigjson=$HOME/.docker/config.json \
  --type=kubernetes.io/dockerconfigjson \
  -n production

# ==================== 多仓库认证 ====================

# 创建包含多个仓库的认证
cat <<EOF | kubectl apply -f -
apiVersion: v1
kind: Secret
metadata:
  name: multi-registry-credentials
  namespace: production
type: kubernetes.io/dockerconfigjson
stringData:
  .dockerconfigjson: |
    {
      "auths": {
        "registry.cn-hangzhou.aliyuncs.com": {
          "username": "aliyun-user",
          "password": "aliyun-password",
          "auth": "$(echo -n 'aliyun-user:aliyun-password' | base64)"
        },
        "ghcr.io": {
          "username": "github-user",
          "password": "ghp_xxxxxxxxxxxx",
          "auth": "$(echo -n 'github-user:ghp_xxxxxxxxxxxx' | base64)"
        },
        "docker.io": {
          "username": "dockerhub-user",
          "password": "dockerhub-token",
          "auth": "$(echo -n 'dockerhub-user:dockerhub-token' | base64)"
        }
      }
    }
EOF
```

### 2. ServiceAccount 关联 (推荐)

```yaml
# 创建带镜像拉取凭证的 ServiceAccount
apiVersion: v1
kind: ServiceAccount
metadata:
  name: myapp-sa
  namespace: production
  annotations:
    # 阿里云 ACK 特定注解
    ack.aliyun.com/image-pull-secrets: "acr-credentials"
imagePullSecrets:
  - name: acr-credentials
  - name: ghcr-credentials
---
# Deployment 使用 ServiceAccount
apiVersion: apps/v1
kind: Deployment
metadata:
  name: myapp
  namespace: production
spec:
  template:
    spec:
      serviceAccountName: myapp-sa
      containers:
        - name: app
          image: registry.cn-hangzhou.aliyuncs.com/myns/myapp:v1.2.3
```

### 3. 命名空间级别默认凭证

```bash
# 为命名空间设置默认 imagePullSecrets
kubectl patch serviceaccount default \
  -n production \
  -p '{"imagePullSecrets": [{"name": "acr-credentials"}]}'

# 验证配置
kubectl get serviceaccount default -n production -o yaml
```

## 多架构镜像 (Multi-arch Images)

### 架构支持矩阵

| 架构 | 平台标识 | 说明 | 典型场景 |
|-----|---------|------|----------|
| AMD64 | linux/amd64 | x86_64 服务器 | 通用服务器 |
| ARM64 | linux/arm64 | ARM 服务器/Mac M系列 | 云原生/边缘计算 |
| ARMv7 | linux/arm/v7 | 32位 ARM | 树莓派/IoT |
| s390x | linux/s390x | IBM 大型机 | 金融/大企业 |
| ppc64le | linux/ppc64le | PowerPC | IBM Power |

### 多架构镜像构建

```bash
# ==================== Docker Buildx ====================

# 创建多平台 builder
docker buildx create --name multiarch-builder \
  --driver docker-container \
  --platform linux/amd64,linux/arm64 \
  --use

# 查看 builder 信息
docker buildx inspect --bootstrap

# 构建多架构镜像并推送
docker buildx build \
  --platform linux/amd64,linux/arm64,linux/arm/v7 \
  --tag registry.cn-hangzhou.aliyuncs.com/myns/myapp:v1.0.0 \
  --push \
  --cache-from type=registry,ref=registry.cn-hangzhou.aliyuncs.com/myns/myapp:cache \
  --cache-to type=registry,ref=registry.cn-hangzhou.aliyuncs.com/myns/myapp:cache,mode=max \
  .

# ==================== Manifest 操作 ====================

# 创建 manifest 列表
docker manifest create registry.cn-hangzhou.aliyuncs.com/myns/myapp:v1.0.0 \
  registry.cn-hangzhou.aliyuncs.com/myns/myapp:v1.0.0-amd64 \
  registry.cn-hangzhou.aliyuncs.com/myns/myapp:v1.0.0-arm64

# 注解架构信息
docker manifest annotate registry.cn-hangzhou.aliyuncs.com/myns/myapp:v1.0.0 \
  registry.cn-hangzhou.aliyuncs.com/myns/myapp:v1.0.0-arm64 \
  --os linux --arch arm64

# 推送 manifest
docker manifest push registry.cn-hangzhou.aliyuncs.com/myns/myapp:v1.0.0

# 检查 manifest
docker manifest inspect registry.cn-hangzhou.aliyuncs.com/myns/myapp:v1.0.0
```

### 多架构 Dockerfile 示例

```dockerfile
# syntax=docker/dockerfile:1.7

# 基础镜像自动选择正确架构
FROM --platform=$TARGETPLATFORM golang:1.22-alpine AS builder

# 构建参数
ARG TARGETPLATFORM
ARG TARGETOS
ARG TARGETARCH

WORKDIR /app
COPY go.* ./
RUN go mod download
COPY . .

# 交叉编译
RUN CGO_ENABLED=0 GOOS=${TARGETOS} GOARCH=${TARGETARCH} \
    go build -ldflags="-s -w" -o /server ./cmd/server

# 运行时镜像
FROM --platform=$TARGETPLATFORM gcr.io/distroless/static-debian12:nonroot
COPY --from=builder /server /server
USER nonroot:nonroot
ENTRYPOINT ["/server"]
```

## 镜像安全扫描工具对比

| 工具 | 类型 | 集成方式 | 特点 | 推荐场景 |
|-----|-----|---------|------|---------|
| **Trivy** | 开源 | CLI/CI/Operator | 全面、快速、SBOM支持 | 通用推荐 |
| **Grype** | 开源 | CLI/CI | Anchore 出品、SBOM 分析 | SBOM 场景 |
| **Clair** | 开源 | 服务端 | 静态分析、Quay 集成 | Registry 集成 |
| **ACR 扫描** | 商业 | 托管服务 | ACK 原生集成 | 阿里云环境 |
| **Snyk** | 商业 | SaaS | 开发者友好、修复建议 | 企业级 |
| **Sysdig** | 商业 | 运行时 | 运行时保护、合规 | 安全敏感 |
| **Anchore** | 商业/开源 | 服务端 | 策略引擎、企业级 | 合规要求 |

## Trivy 完整使用指南

### 基础扫描命令

```bash
# ==================== 镜像漏洞扫描 ====================

# 基础扫描
trivy image myapp:v1.0

# 仅显示高危漏洞
trivy image --severity HIGH,CRITICAL myapp:v1.0

# 忽略未修复的漏洞
trivy image --ignore-unfixed myapp:v1.0

# 输出 JSON 格式
trivy image -f json -o results.json myapp:v1.0

# 输出 SARIF 格式 (GitHub Security)
trivy image -f sarif -o results.sarif myapp:v1.0

# CI/CD 使用 (设置退出码)
trivy image --exit-code 1 --severity CRITICAL myapp:v1.0

# 扫描私有仓库
trivy image --username $USER --password $PASS registry.example.com/myapp:v1.0

# ==================== 配置扫描 ====================

# 扫描 Kubernetes 配置
trivy config ./kubernetes/

# 扫描 Dockerfile
trivy config --file-patterns 'dockerfile:Dockerfile*' .

# 扫描 Helm Chart
trivy config ./charts/myapp/

# ==================== 文件系统扫描 ====================

# 扫描项目目录
trivy fs --security-checks vuln,config,secret ./

# 扫描并生成 SBOM
trivy fs --format cyclonedx -o sbom.json ./

# ==================== SBOM 生成与扫描 ====================

# 生成 SBOM (CycloneDX 格式)
trivy image --format cyclonedx -o sbom.json myapp:v1.0

# 生成 SBOM (SPDX 格式)
trivy image --format spdx-json -o sbom.spdx.json myapp:v1.0

# 扫描已有 SBOM
trivy sbom sbom.json

# ==================== 高级选项 ====================

# 使用数据库缓存
trivy image --cache-dir ~/.cache/trivy myapp:v1.0

# 离线模式 (使用本地数据库)
trivy image --offline-scan myapp:v1.0

# 自定义忽略规则
trivy image --ignorefile .trivyignore myapp:v1.0

# 扫描特定层
trivy image --list-all-pkgs myapp:v1.0
```

### Trivy Operator 部署

```yaml
# trivy-operator-values.yaml
trivy:
  # 扫描配置
  severity: HIGH,CRITICAL
  ignoreUnfixed: true
  
  # 数据库更新
  dbRepository: ghcr.io/aquasecurity/trivy-db
  dbRepositoryInsecure: false
  
  # 资源限制
  resources:
    requests:
      cpu: 100m
      memory: 100Mi
    limits:
      cpu: 500m
      memory: 500Mi

operator:
  # 扫描频率
  vulnerabilityScannerScanOnlyCurrentRevisions: true
  configAuditScannerScanOnlyCurrentRevisions: true
  
  # 并发扫描
  scanJobsConcurrentLimit: 10
  
  # 扫描超时
  scanJobTimeout: 5m
  
  # 报告保留
  vulnerabilityReportsPlugin: Trivy
  configAuditReportsPlugin: Trivy
  
serviceMonitor:
  enabled: true
```

```bash
# 部署 Trivy Operator
helm repo add aqua https://aquasecurity.github.io/helm-charts/
helm repo update

helm install trivy-operator aqua/trivy-operator \
  --namespace trivy-system \
  --create-namespace \
  -f trivy-operator-values.yaml

# 查看漏洞报告
kubectl get vulnerabilityreports -A
kubectl get configauditreports -A

# 查看特定报告
kubectl get vulnerabilityreport -n production -o yaml
```

## 镜像签名与验证 (Cosign)

### Cosign 完整工作流

```bash
# ==================== 密钥管理 ====================

# 生成本地密钥对
cosign generate-key-pair

# 使用 KMS 密钥 (推荐生产环境)
cosign generate-key-pair --kms awskms:///alias/cosign-key
cosign generate-key-pair --kms gcpkms://projects/PROJECT/locations/LOCATION/keyRings/KEYRING/cryptoKeys/KEY
cosign generate-key-pair --kms azurekms://VAULT_NAME/KEY_NAME

# 使用 Vault
cosign generate-key-pair --kms hashivault://KEY_NAME

# ==================== 镜像签名 ====================

# 使用私钥签名
cosign sign --key cosign.key registry.example.com/myapp:v1.0

# Keyless 签名 (OIDC，推荐)
COSIGN_EXPERIMENTAL=1 cosign sign registry.example.com/myapp:v1.0

# 添加自定义注解
cosign sign --key cosign.key \
  --annotations "build.id=${BUILD_ID}" \
  --annotations "git.commit=${GIT_SHA}" \
  --annotations "git.repo=${GIT_REPO}" \
  registry.example.com/myapp:v1.0

# 签名并附加证书
cosign sign --key cosign.key \
  --certificate cert.pem \
  --certificate-chain chain.pem \
  registry.example.com/myapp:v1.0

# ==================== 签名验证 ====================

# 使用公钥验证
cosign verify --key cosign.pub registry.example.com/myapp:v1.0

# Keyless 验证
COSIGN_EXPERIMENTAL=1 cosign verify registry.example.com/myapp:v1.0

# 验证特定身份
cosign verify \
  --certificate-identity "builder@example.com" \
  --certificate-oidc-issuer "https://accounts.google.com" \
  registry.example.com/myapp:v1.0

# 验证 GitHub Actions 签名
cosign verify \
  --certificate-identity "https://github.com/myorg/myrepo/.github/workflows/build.yaml@refs/heads/main" \
  --certificate-oidc-issuer "https://token.actions.githubusercontent.com" \
  registry.example.com/myapp:v1.0

# ==================== SBOM 附加与验证 ====================

# 生成 SBOM
syft registry.example.com/myapp:v1.0 -o spdx-json > sbom.spdx.json

# 附加 SBOM 到镜像
cosign attach sbom --sbom sbom.spdx.json registry.example.com/myapp:v1.0

# 签名 SBOM
cosign sign --key cosign.key --attachment sbom registry.example.com/myapp:v1.0

# 验证 SBOM 签名
cosign verify --key cosign.pub --attachment sbom registry.example.com/myapp:v1.0

# ==================== 证明 (Attestation) ====================

# 创建 In-toto 证明
cosign attest --key cosign.key \
  --predicate predicate.json \
  --type slsaprovenance \
  registry.example.com/myapp:v1.0

# 验证证明
cosign verify-attestation --key cosign.pub \
  --type slsaprovenance \
  registry.example.com/myapp:v1.0
```

### Kubernetes 策略强制签名验证

```yaml
# Kyverno 策略 - 强制镜像签名验证
apiVersion: kyverno.io/v1
kind: ClusterPolicy
metadata:
  name: verify-image-signatures
spec:
  validationFailureAction: Enforce
  background: false
  rules:
    - name: verify-signature
      match:
        any:
          - resources:
              kinds:
                - Pod
      verifyImages:
        - imageReferences:
            - "registry.example.com/*"
          attestors:
            - entries:
                - keys:
                    publicKeys: |-
                      -----BEGIN PUBLIC KEY-----
                      MFkwEwYHKoZIzj0CAQYIKoZIzj0DAQcDQgAE...
                      -----END PUBLIC KEY-----
          mutateDigest: true
          verifyDigest: true
```

## 镜像分发加速方案

### 方案对比

| 方案 | 说明 | 适用场景 | 复杂度 |
|-----|------|---------|--------|
| **镜像预热** | 提前拉取到节点 | 大规模部署、确定性调度 | 低 |
| **P2P 分发 (Dragonfly)** | 节点间 P2P 传输 | 大镜像、大规模集群 | 中 |
| **镜像懒加载 (Nydus)** | 按需加载镜像层 | 快速启动、大镜像 | 高 |
| **Registry 代理** | 就近缓存 | 跨地域访问 | 低 |

### Dragonfly P2P 分发配置

```yaml
# dragonfly-values.yaml
manager:
  replicas: 1
  resources:
    requests:
      cpu: 100m
      memory: 256Mi

scheduler:
  replicas: 3
  resources:
    requests:
      cpu: 200m
      memory: 512Mi

seedPeer:
  replicas: 3
  resources:
    requests:
      cpu: 500m
      memory: 1Gi
  config:
    download:
      prefetch: true

dfdaemon:
  resources:
    requests:
      cpu: 100m
      memory: 256Mi
  config:
    proxy:
      registryMirror:
        url: https://registry-vpc.cn-hangzhou.aliyuncs.com
      proxies:
        - regx: blobs/sha256.*
```

```bash
# 部署 Dragonfly
helm repo add dragonfly https://dragonflyoss.github.io/helm-charts/
helm install dragonfly dragonfly/dragonfly \
  --namespace dragonfly-system \
  --create-namespace \
  -f dragonfly-values.yaml

# 配置 containerd 使用 Dragonfly
# /etc/containerd/config.toml
# [plugins."io.containerd.grpc.v1.cri".registry.mirrors."docker.io"]
#   endpoint = ["http://127.0.0.1:65001"]
```

### 镜像预热 Job

```yaml
# image-warm-up-job.yaml
apiVersion: batch/v1
kind: Job
metadata:
  name: image-warmup
  namespace: kube-system
spec:
  parallelism: 10  # 并行节点数
  completions: 10  # 总节点数
  template:
    spec:
      containers:
        - name: warmup
          image: docker.io/library/docker:24-cli
          command:
            - /bin/sh
            - -c
            - |
              # 预热镜像列表
              images=(
                "registry.cn-hangzhou.aliyuncs.com/myns/myapp:v1.0.0"
                "registry.cn-hangzhou.aliyuncs.com/myns/sidecar:v2.0.0"
              )
              for img in "${images[@]}"; do
                echo "Warming up: $img"
                docker pull $img || true
              done
          volumeMounts:
            - name: docker-sock
              mountPath: /var/run/docker.sock
      volumes:
        - name: docker-sock
          hostPath:
            path: /var/run/docker.sock
      restartPolicy: Never
      affinity:
        podAntiAffinity:
          requiredDuringSchedulingIgnoredDuringExecution:
            - labelSelector:
                matchLabels:
                  job-name: image-warmup
              topologyKey: kubernetes.io/hostname
  backoffLimit: 3
```

## 高效 Dockerfile 最佳实践

```dockerfile
# syntax=docker/dockerfile:1.7

# ==================== 多阶段构建示例 ====================

# 阶段 1: 依赖安装
FROM golang:1.22-alpine AS deps
WORKDIR /app
COPY go.mod go.sum ./
RUN --mount=type=cache,target=/go/pkg/mod \
    go mod download

# 阶段 2: 构建
FROM deps AS builder
COPY . .
ARG VERSION=dev
ARG COMMIT=unknown
ARG BUILD_DATE=unknown
RUN --mount=type=cache,target=/go/pkg/mod \
    --mount=type=cache,target=/root/.cache/go-build \
    CGO_ENABLED=0 GOOS=linux go build \
    -ldflags="-s -w -X main.version=${VERSION} -X main.commit=${COMMIT} -X main.buildDate=${BUILD_DATE}" \
    -o /server ./cmd/server

# 阶段 3: 安全扫描 (可选)
FROM aquasec/trivy:latest AS scanner
COPY --from=builder /server /server
RUN trivy rootfs --exit-code 1 --severity HIGH,CRITICAL /

# 阶段 4: 最终运行镜像
FROM gcr.io/distroless/static-debian12:nonroot AS production

# OCI 标准标签
LABEL org.opencontainers.image.title="MyApp"
LABEL org.opencontainers.image.description="Production-ready application"
LABEL org.opencontainers.image.version="${VERSION}"
LABEL org.opencontainers.image.revision="${COMMIT}"
LABEL org.opencontainers.image.created="${BUILD_DATE}"
LABEL org.opencontainers.image.source="https://github.com/myorg/myapp"
LABEL org.opencontainers.image.licenses="MIT"

# 复制二进制文件
COPY --from=builder /server /server

# 非 root 用户运行
USER nonroot:nonroot

# 健康检查
HEALTHCHECK --interval=30s --timeout=3s --start-period=5s --retries=3 \
    CMD ["/server", "health"]

# 暴露端口
EXPOSE 8080 9090

# 入口点
ENTRYPOINT ["/server"]
CMD ["serve"]
```

## ACR (阿里云容器镜像服务) 功能对比

| 功能 | 企业版 | 个人版 | 说明 |
|-----|-------|-------|------|
| 镜像托管 | ✅ | ✅ | 基础镜像存储 |
| 漏洞扫描 | ✅ 深度扫描 | ✅ 基础扫描 | 安全检测 |
| 镜像签名 | ✅ | ❌ | Notary v2 |
| 地域复制 | ✅ | ❌ | 跨地域同步 |
| P2P 加速 | ✅ | ❌ | Dragonfly 集成 |
| Helm Chart | ✅ | ✅ | OCI 格式 |
| 构建服务 | ✅ | ✅ | 自动构建 |
| 清理策略 | ✅ | ✅ | 标签清理 |
| VPC 访问 | ✅ | ✅ | 内网拉取 |
| RAM 授权 | ✅ | ✅ | 细粒度权限 |
| 配额限制 | 无限制 | 有限制 | 仓库数/流量 |

## 镜像管理监控指标

```yaml
# Prometheus 告警规则
groups:
  - name: image-registry-alerts
    rules:
      - alert: ImagePullFailure
        expr: |
          increase(kubelet_runtime_operations_errors_total{operation_type="pull_image"}[5m]) > 5
        for: 5m
        labels:
          severity: warning
        annotations:
          summary: "节点 {{ $labels.node }} 镜像拉取失败"
          description: "过去5分钟内镜像拉取失败超过5次"
      
      - alert: ImagePullSlow
        expr: |
          histogram_quantile(0.99, 
            rate(kubelet_runtime_operations_duration_seconds_bucket{operation_type="pull_image"}[5m])
          ) > 300
        for: 10m
        labels:
          severity: warning
        annotations:
          summary: "镜像拉取延迟过高"
          description: "P99 镜像拉取时间超过5分钟"
      
      - alert: RegistryUnreachable
        expr: |
          probe_success{job="registry-probe"} == 0
        for: 5m
        labels:
          severity: critical
        annotations:
          summary: "镜像仓库不可达"
          description: "{{ $labels.instance }} 连接失败"
```

## 镜像管理最佳实践清单

### 构建最佳实践

- [ ] **使用多阶段构建**: 分离构建和运行环境
- [ ] **最小基础镜像**: distroless/alpine/scratch
- [ ] **固定基础镜像版本**: 使用 digest 或精确版本
- [ ] **清理构建缓存**: 不保留 apt/npm/pip 缓存
- [ ] **非 root 用户**: USER 指令指定运行用户
- [ ] **只读文件系统**: readOnlyRootFilesystem: true

### 安全最佳实践

- [ ] **漏洞扫描**: CI/CD 集成 Trivy 扫描
- [ ] **镜像签名**: Cosign 签名并验证
- [ ] **SBOM 生成**: 生成软件物料清单
- [ ] **策略强制**: Kyverno/OPA 签名验证
- [ ] **私有仓库**: 使用企业私有仓库
- [ ] **最小权限**: 镜像内只包含必要组件

### 运维最佳实践

- [ ] **标签策略**: 语义化版本 + Git SHA
- [ ] **镜像清理**: 定期清理旧版本镜像
- [ ] **镜像预热**: 大规模部署前预拉取
- [ ] **P2P 分发**: 大镜像使用 Dragonfly
- [ ] **监控告警**: 镜像拉取失败/延迟告警

## 版本变更记录

| 版本 | 变更内容 |
|------|---------|
| v1.25 | CRI v1 API 稳定 |
| v1.27 | 镜像 Volume 支持改进 |
| v1.28 | 镜像拉取并行化优化 |
| v1.29 | 镜像拉取进度报告 |
| v1.30 | containerd 2.0 支持 |
| v1.31 | OCI 镜像布局支持增强 |

---

**表格底部标记**: Kusheet Project, 作者 Allen Galler (allengaller@gmail.com)
