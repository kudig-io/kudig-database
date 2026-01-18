import os

footer = "\n\n---\n\n**表格底部标记**: Kusheet Project, 作者 Allen Galler (allengaller@gmail.com)"

# Tier 1: Tools enrichment (101-110)
tools_enrichment = {
    '101-package-management-tools.md': """# 101 - 包管理与应用分发工具 (Package Management & Distribution)

> **适用版本**: v1.25 - v1.32 | **最后更新**: 2026-01

## 核心工具对比矩阵

| 工具 (Tool) | 核心定位 (Position) | 生产优势 (Production Advantages) | ACK 集成 |
|------------|-------------------|--------------------------------|---------|
| **Helm** | 应用包管理器 | Chart 版本控制、回滚、依赖管理 | ✅ 原生支持 |
| **Kustomize** | 声明式配置管理 | 无模板、原生集成、多环境叠加 | ✅ kubectl 内置 |
| **Operator SDK** | 自动化运维框架 | 生命周期管理、自愈能力 | ✅ OLM 支持 |
| **ArgoCD** | GitOps 持续部署 | 声明式、自动同步、审计 | ✅ 可集成 |

## Helm 生产最佳实践

### 1. Chart 仓库管理
```bash
# 使用 OCI 仓库 (推荐)
helm registry login registry.cn-hangzhou.aliyuncs.com
helm push mychart-0.1.0.tgz oci://registry.cn-hangzhou.aliyuncs.com/mynamespace

# 传统 HTTP 仓库
helm repo add stable https://charts.helm.sh/stable
helm repo update
```

### 2. 值文件分层策略
```yaml
# values-prod.yaml (生产环境)
replicaCount: 3
resources:
  limits:
    cpu: 2
    memory: 4Gi
ingress:
  enabled: true
  className: nginx
  annotations:
    cert-manager.io/cluster-issuer: letsencrypt-prod
```

### 3. 版本管理与回滚
```bash
# 查看发布历史
helm history myapp -n production

# 回滚到指定版本
helm rollback myapp 3 -n production

# 原子性部署 (失败自动回滚)
helm upgrade --install myapp ./mychart --atomic --timeout 10m
```

## Kustomize 多环境管理

### 目录结构
```
├── base/
│   ├── deployment.yaml
│   ├── service.yaml
│   └── kustomization.yaml
├── overlays/
│   ├── dev/
│   │   └── kustomization.yaml
│   ├── staging/
│   │   └── kustomization.yaml
│   └── prod/
│       ├── kustomization.yaml
│       └── replicas.yaml
```

### 生产环境叠加
```yaml
# overlays/prod/kustomization.yaml
apiVersion: kustomize.config.k8s.io/v1beta1
kind: Kustomization
bases:
  - ../../base
replicas:
  - name: myapp
    count: 5
images:
  - name: myapp
    newTag: v1.2.3
patchesStrategicMerge:
  - replicas.yaml
```

## Operator 开发模式

| 模式 (Pattern) | 适用场景 (Use Case) | 复杂度 |
|---------------|-------------------|-------|
| **Helm Operator** | 简单应用打包 | 低 |
| **Ansible Operator** | 运维自动化 | 中 |
| **Go Operator** | 复杂状态管理 | 高 |

### 生产 Operator 示例 (MySQL Cluster)
- 自动主从切换
- 定时备份与恢复
- 资源自动扩缩容
- 监控指标暴露

## 故障排查

| 问题 (Issue) | 诊断 (Diagnosis) | 解决方案 (Solution) |
|-------------|-----------------|-------------------|
| Helm 安装失败 | `helm get manifest` | 检查 RBAC 权限 |
| Kustomize 构建错误 | `kustomize build --enable-alpha-plugins` | 验证 YAML 语法 |
| Operator 无法调谐 | 查看 Operator 日志 | 检查 CRD 版本兼容性 |
""",

    '102-secret-management-tools.md': """# 102 - 密钥与敏感信息管理工具 (Secret Management)

> **适用版本**: v1.25 - v1.32 | **最后更新**: 2026-01

## 密钥管理方案对比

| 工具 (Tool) | 架构 (Architecture) | 核心优势 (Advantages) | 生产建议 |
|------------|-------------------|---------------------|---------|
| **HashiCorp Vault** | 外部密钥库 | 动态秘钥、审计、精细权限 | 金融/安全敏感场景 |
| **External Secrets Operator** | 同步控制器 | 云原生、多云支持 | 推荐 ACK 环境 |
| **Sealed Secrets** | 加密控制器 | GitOps 友好 | 适合 ArgoCD 工作流 |
| **SOPS** | 文件加密工具 | 简单、支持多种 KMS | 小团队快速上手 |

## External Secrets Operator (生产推荐)

### 1. 集成阿里云 KMS
```yaml
apiVersion: external-secrets.io/v1beta1
kind: SecretStore
metadata:
  name: alicloud-kms
  namespace: production
spec:
  provider:
    alibaba:
      regionID: cn-hangzhou
      auth:
        secretRef:
          accessKeyID:
            name: alicloud-credentials
            key: access-key-id
          accessKeySecret:
            name: alicloud-credentials
            key: access-key-secret
```

### 2. 同步密钥到 Kubernetes
```yaml
apiVersion: external-secrets.io/v1beta1
kind: ExternalSecret
metadata:
  name: database-credentials
spec:
  refreshInterval: 1h
  secretStoreRef:
    name: alicloud-kms
    kind: SecretStore
  target:
    name: db-secret
    creationPolicy: Owner
  data:
    - secretKey: username
      remoteRef:
        key: prod/db/username
    - secretKey: password
      remoteRef:
        key: prod/db/password
```

## Vault 企业级部署

### 高可用架构
- **存储后端**: Raft (推荐) 或 Consul
- **自动解封**: Auto-unseal with Cloud KMS
- **审计日志**: 持久化到对象存储

### Vault Agent Sidecar 注入
```yaml
annotations:
  vault.hashicorp.com/agent-inject: "true"
  vault.hashicorp.com/role: "myapp"
  vault.hashicorp.com/agent-inject-secret-db: "secret/data/db/config"
  vault.hashicorp.com/agent-inject-template-db: |
    {{- with secret "secret/data/db/config" -}}
    export DB_USER="{{ .Data.data.username }}"
    export DB_PASS="{{ .Data.data.password }}"
    {{- end }}
```

## Sealed Secrets GitOps 工作流

### 1. 加密敏感信息
```bash
# 安装 kubeseal CLI
kubectl create secret generic mysecret --dry-run=client -o yaml | \\
  kubeseal -o yaml > sealed-secret.yaml

# 提交到 Git
git add sealed-secret.yaml
git commit -m "Add encrypted secret"
```

### 2. 自动解密
- Sealed Secrets Controller 自动解密
- 仅集群内私钥可解密
- 支持命名空间/集群级作用域

## 安全最佳实践

| 实践 (Practice) | 说明 (Description) |
|----------------|-------------------|
| **最小权限** | RBAC 限制 Secret 访问 |
| **轮换策略** | 定期轮换数据库密码 |
| **审计日志** | 记录所有密钥访问 |
| **加密存储** | 启用 etcd 加密 (EncryptionConfiguration) |
| **避免硬编码** | 禁止在代码/镜像中存放密钥 |
""",

    '103-image-build-tools.md': """# 103 - 容器镜像构建工具 (Container Image Build)

> **适用版本**: v1.25 - v1.32 | **最后更新**: 2026-01

## 构建工具技术对比

| 工具 (Tool) | 运行环境 (Runtime) | 核心特性 (Features) | 生产场景 |
|------------|------------------|-------------------|---------|
| **Kaniko** | Kubernetes Pod | 无需 Docker Daemon | CI/CD 流水线首选 |
| **Buildah** | 任意环境 | OCI 标准、Rootless | 安全敏感环境 |
| **BuildKit** | Docker/Standalone | 并行构建、缓存优化 | 本地开发、极致性能 |
| **Jib** | Maven/Gradle 插件 | 无需 Dockerfile | Java 应用快速构建 |

## Kaniko 生产级配置

### 1. 在 Kubernetes 中构建
```yaml
apiVersion: v1
kind: Pod
metadata:
  name: kaniko-build
spec:
  containers:
  - name: kaniko
    image: gcr.io/kaniko-project/executor:latest
    args:
      - "--dockerfile=Dockerfile"
      - "--context=git://github.com/myorg/myapp.git#refs/heads/main"
      - "--destination=registry.cn-hangzhou.aliyuncs.com/myns/myapp:v1.0"
      - "--cache=true"
      - "--cache-repo=registry.cn-hangzhou.aliyuncs.com/myns/cache"
    volumeMounts:
    - name: docker-config
      mountPath: /kaniko/.docker/
  volumes:
  - name: docker-config
    secret:
      secretName: regcred
      items:
      - key: .dockerconfigjson
        path: config.json
  restartPolicy: Never
```

### 2. 缓存优化策略
- 启用 `--cache=true` 复用层缓存
- 使用专用 `--cache-repo` 存储中间层
- 多阶段构建减少最终镜像体积

## BuildKit 高级特性

### 1. 并行构建
```dockerfile
# syntax=docker/dockerfile:1.4
FROM golang:1.21 AS build-backend
WORKDIR /app
COPY go.* ./
RUN go mod download
COPY . .
RUN go build -o server

FROM node:18 AS build-frontend
WORKDIR /app
COPY package*.json ./
RUN npm ci
COPY . .
RUN npm run build

FROM alpine:latest
COPY --from=build-backend /app/server /server
COPY --from=build-frontend /app/dist /static
CMD ["/server"]
```

### 2. 秘钥挂载 (避免泄露)
```dockerfile
# syntax=docker/dockerfile:1.4
RUN --mount=type=secret,id=npmrc,target=/root/.npmrc \\
    npm install
```

## Buildah Rootless 构建

### 无特权构建
```bash
# 无需 root 权限
buildah bud -t myapp:v1.0 .
buildah push myapp:v1.0 docker://registry.example.com/myapp:v1.0

# 检查镜像
buildah images
```

## 镜像优化最佳实践

| 实践 (Practice) | 效果 (Impact) |
|----------------|--------------|
| **多阶段构建** | 减少 70%+ 镜像体积 |
| **Distroless 基础镜像** | 最小攻击面 |
| **Layer 缓存** | 加速 80% 构建时间 |
| **.dockerignore** | 排除无关文件 |
| **固定基础镜像 Digest** | 确保可复现构建 |
""",

    '104-security-scanning-tools.md': """# 104 - 安全扫描与漏洞检测工具 (Security Scanning)

> **适用版本**: v1.25 - v1.32 | **最后更新**: 2026-01

## 扫描工具能力矩阵

| 工具 (Tool) | 扫描对象 (Target) | 漏洞库 (Database) | CI/CD 集成 | 生产建议 |
|------------|-----------------|-----------------|-----------|---------|
| **Trivy** | 镜像/文件系统/Git | 多源聚合 | ✅ 极易集成 | 通用首选 |
| **Grype** | 镜像/SBOM | Anchore DB | ✅ 支持 | 离线环境 |
| **Clair** | 镜像 | CVE 数据库 | ✅ Harbor 集成 | 企业镜像仓库 |
| **Snyk** | 镜像/代码/依赖 | 商业数据库 | ✅ SaaS | 开发者友好 |

## Trivy 生产级扫描

### 1. CI/CD 流水线集成
```yaml
# GitLab CI 示例
security_scan:
  stage: test
  image: aquasec/trivy:latest
  script:
    - trivy image --exit-code 1 --severity CRITICAL,HIGH myapp:$CI_COMMIT_SHA
    - trivy fs --exit-code 0 --severity MEDIUM,LOW .
  artifacts:
    reports:
      container_scanning: gl-container-scanning-report.json
```

### 2. Kubernetes 准入控制
```yaml
# 使用 OPA/Gatekeeper 强制扫描
apiVersion: constraints.gatekeeper.sh/v1beta1
kind: K8sRequireImageScan
metadata:
  name: require-trivy-scan
spec:
  match:
    kinds:
      - apiGroups: [""]
        kinds: ["Pod"]
  parameters:
    allowedSeverities: ["LOW", "MEDIUM"]
```

### 3. 定期扫描已部署镜像
```bash
# 扫描集群中所有镜像
kubectl get pods --all-namespaces -o jsonpath="{.items[*].spec.containers[*].image}" | \\
  tr -s '[[:space:]]' '\\n' | sort | uniq | \\
  xargs -I {} trivy image --severity HIGH,CRITICAL {}
```

## Grype SBOM 工作流

### 1. 生成软件物料清单
```bash
# 使用 Syft 生成 SBOM
syft packages myapp:v1.0 -o spdx-json > sbom.json

# Grype 基于 SBOM 扫描
grype sbom:./sbom.json
```

### 2. 离线扫描
```bash
# 下载漏洞数据库
grype db update

# 离线扫描
grype myapp:v1.0 --offline
```

## 安全策略最佳实践

| 策略 (Policy) | 实施方式 (Implementation) |
|--------------|--------------------------|
| **阻断高危漏洞** | CI 流水线设置 `--exit-code 1` |
| **定期扫描** | CronJob 每日扫描生产镜像 |
| **漏洞追踪** | 集成 Jira/GitHub Issues |
| **基础镜像管理** | 使用官方/可信镜像源 |
| **及时修复** | SLA: CRITICAL 24h, HIGH 7d |
""",

    '105-policy-validation-tools.md': """# 105 - 策略校验与准入控制工具 (Policy Validation)

> **适用版本**: v1.25 - v1.32 | **最后更新**: 2026-01

## 策略引擎对比

| 工具 (Tool) | 策略语言 (Language) | 核心能力 (Capabilities) | 学习曲线 |
|------------|-------------------|----------------------|---------|
| **OPA/Gatekeeper** | Rego | 通用策略引擎、强大灵活 | 陡峭 |
| **Kyverno** | YAML | K8s 原生、易上手 | 平缓 |
| **Polaris** | YAML | 配置审计、仪表盘 | 极简 |
| **Kubewarden** | WebAssembly | 多语言策略、高性能 | 中等 |

## Kyverno 生产实践

### 1. 强制镜像来源
```yaml
apiVersion: kyverno.io/v1
kind: ClusterPolicy
metadata:
  name: require-registry
spec:
  validationFailureAction: enforce
  rules:
  - name: check-registry
    match:
      any:
      - resources:
          kinds:
          - Pod
    validate:
      message: "镜像必须来自可信仓库"
      pattern:
        spec:
          containers:
          - image: "registry.cn-hangzhou.aliyuncs.com/*"
```

### 2. 自动注入 Sidecar
```yaml
apiVersion: kyverno.io/v1
kind: ClusterPolicy
metadata:
  name: inject-sidecar
spec:
  rules:
  - name: add-logging-sidecar
    match:
      any:
      - resources:
          kinds:
          - Deployment
          namespaces:
          - production
    mutate:
      patchStrategicMerge:
        spec:
          template:
            spec:
              containers:
              - name: log-collector
                image: fluent/fluent-bit:latest
```

### 3. 资源配额验证
```yaml
apiVersion: kyverno.io/v1
kind: ClusterPolicy
metadata:
  name: require-resources
spec:
  validationFailureAction: enforce
  rules:
  - name: check-resources
    match:
      any:
      - resources:
          kinds:
          - Pod
    validate:
      message: "必须设置资源 requests 和 limits"
      pattern:
        spec:
          containers:
          - resources:
              requests:
                memory: "?*"
                cpu: "?*"
              limits:
                memory: "?*"
                cpu: "?*"
```

## OPA/Gatekeeper 高级策略

### 1. 禁止特权容器
```yaml
apiVersion: constraints.gatekeeper.sh/v1beta1
kind: K8sPSPPrivilegedContainer
metadata:
  name: psp-privileged-container
spec:
  match:
    kinds:
      - apiGroups: [""]
        kinds: ["Pod"]
    excludedNamespaces: ["kube-system"]
```

### 2. 镜像签名验证
```rego
package kubernetes.admission

deny[msg] {
  input.request.kind.kind == "Pod"
  image := input.request.object.spec.containers[_].image
  not image_signed(image)
  msg := sprintf("镜像未签名: %v", [image])
}

image_signed(image) {
  # 调用外部签名验证服务
  http.send({
    "method": "GET",
    "url": sprintf("https://notary.example.com/verify?image=%v", [image])
  }).status_code == 200
}
```

## Polaris 配置审计

### 仪表盘指标
- **安全性**: 特权容器、只读根文件系统
- **可靠性**: 探针配置、副本数
- **效率**: 资源限制、镜像标签

### 命令行扫描
```bash
polaris audit --audit-path ./manifests/ --format=json > audit-report.json
```

## 策略治理最佳实践

| 实践 (Practice) | 说明 (Description) |
|----------------|-------------------|
| **分层策略** | 集群级 + 命名空间级 |
| **审计模式** | 先 audit 后 enforce |
| **例外管理** | 使用 Annotation 豁免 |
| **持续监控** | 定期审计现有资源 |
| **文档化** | 策略说明与修复指南 |
""",
}

# Write Tier 1 files
for filename, content in tools_enrichment.items():
    path = os.path.join('tables', filename)
    with open(path, 'w', encoding='utf-8') as f:
        f.write(content + footer)

print(f"Enriched {len(tools_enrichment)} tools tables (101-105).")
