# 103 - 容器镜像构建工具 (Container Image Build)

> **适用版本**: v1.25 - v1.32 | **最后更新**: 2026-01 | **难度**: 中级-高级

## 容器镜像构建生态架构

```
┌─────────────────────────────────────────────────────────────────────────────────────┐
│                        容器镜像构建与分发生态                                         │
├─────────────────────────────────────────────────────────────────────────────────────┤
│                                                                                      │
│  ┌──────────────────────────────────────────────────────────────────────────────┐   │
│  │                          源代码层 (Source Code)                               │   │
│  │                                                                               │   │
│  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐  ┌─────────────────────┐  │   │
│  │  │ Dockerfile  │  │ Containerfile│  │   Jib      │  │   Cloud Native      │  │   │
│  │  │   (传统)    │  │  (OCI 标准)  │  │ (无需 DF)  │  │   Buildpacks        │  │   │
│  │  └──────┬──────┘  └──────┬──────┘  └──────┬──────┘  └──────────┬──────────┘  │   │
│  └─────────┼────────────────┼────────────────┼───────────────────┼─────────────┘   │
│            │                │                │                   │                  │
│            └────────────────┴────────────────┴───────────────────┘                  │
│                                      │                                              │
│                                      ▼                                              │
│  ┌──────────────────────────────────────────────────────────────────────────────┐   │
│  │                        构建引擎层 (Build Engines)                             │   │
│  │                                                                               │   │
│  │  ┌─────────────────────────────────────────────────────────────────────────┐ │   │
│  │  │                      Kubernetes 原生构建                                 │ │   │
│  │  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐  ┌───────────────┐  │ │   │
│  │  │  │   Kaniko    │  │   Buildah   │  │    img      │  │    Makisu     │  │ │   │
│  │  │  │ (无 Daemon) │  │ (Rootless)  │  │ (Rootless)  │  │   (Uber)      │  │ │   │
│  │  │  └─────────────┘  └─────────────┘  └─────────────┘  └───────────────┘  │ │   │
│  │  └─────────────────────────────────────────────────────────────────────────┘ │   │
│  │                                                                               │   │
│  │  ┌─────────────────────────────────────────────────────────────────────────┐ │   │
│  │  │                      传统/增强构建                                       │ │   │
│  │  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐  ┌───────────────┐  │ │   │
│  │  │  │  BuildKit   │  │   Docker    │  │   Podman    │  │    nerdctl    │  │ │   │
│  │  │  │  (并行构建) │  │   Build     │  │  (Daemon-   │  │  (containerd) │  │ │   │
│  │  │  │             │  │   (经典)    │  │    less)    │  │               │  │ │   │
│  │  │  └─────────────┘  └─────────────┘  └─────────────┘  └───────────────┘  │ │   │
│  │  └─────────────────────────────────────────────────────────────────────────┘ │   │
│  │                                                                               │   │
│  │  ┌─────────────────────────────────────────────────────────────────────────┐ │   │
│  │  │                      语言特定构建                                        │ │   │
│  │  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐  ┌───────────────┐  │ │   │
│  │  │  │     Jib     │  │    ko       │  │    pack     │  │   Source-to-  │  │ │   │
│  │  │  │   (Java)    │  │    (Go)     │  │(Buildpacks) │  │     Image     │  │ │   │
│  │  │  └─────────────┘  └─────────────┘  └─────────────┘  └───────────────┘  │ │   │
│  │  └─────────────────────────────────────────────────────────────────────────┘ │   │
│  └──────────────────────────────────────────────────────────────────────────────┘   │
│                                      │                                              │
│                                      ▼                                              │
│  ┌──────────────────────────────────────────────────────────────────────────────┐   │
│  │                        镜像仓库层 (Registry)                                  │   │
│  │                                                                               │   │
│  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐  ┌─────────────────────┐  │   │
│  │  │   Harbor    │  │    ACR      │  │    ECR      │  │       GCR           │  │   │
│  │  │  (企业级)   │  │  (阿里云)   │  │   (AWS)     │  │      (GCP)          │  │   │
│  │  └─────────────┘  └─────────────┘  └─────────────┘  └─────────────────────┘  │   │
│  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐  ┌─────────────────────┐  │   │
│  │  │   GHCR      │  │ Docker Hub  │  │   Quay.io   │  │      JFrog          │  │   │
│  │  │  (GitHub)   │  │  (公共)     │  │  (RedHat)   │  │    Artifactory      │  │   │
│  │  └─────────────┘  └─────────────┘  └─────────────┘  └─────────────────────┘  │   │
│  └──────────────────────────────────────────────────────────────────────────────┘   │
│                                      │                                              │
│                                      ▼                                              │
│  ┌──────────────────────────────────────────────────────────────────────────────┐   │
│  │                        安全扫描层 (Security Scanning)                         │   │
│  │                                                                               │   │
│  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐  ┌─────────────────────┐  │   │
│  │  │   Trivy     │  │   Grype     │  │   Snyk      │  │      Anchore        │  │   │
│  │  │  (全栈扫描) │  │  (SBOM)     │  │  (商业)     │  │   (企业安全)        │  │   │
│  │  └─────────────┘  └─────────────┘  └─────────────┘  └─────────────────────┘  │   │
│  └──────────────────────────────────────────────────────────────────────────────┘   │
│                                                                                      │
└─────────────────────────────────────────────────────────────────────────────────────┘
```

## 构建工具技术对比矩阵

| 工具 (Tool) | 运行环境 (Runtime) | 核心特性 (Features) | 安全性 | 生产场景 | ACK 集成 |
|------------|------------------|-------------------|--------|---------|---------|
| **Kaniko** | Kubernetes Pod | 无需 Docker Daemon | 高 | CI/CD 流水线首选 | ✅ 原生 |
| **Buildah** | 任意环境 | OCI 标准、Rootless | 高 | 安全敏感环境 | ✅ Pod |
| **BuildKit** | Docker/Standalone | 并行构建、缓存优化 | 中 | 本地开发、极致性能 | ⚠️ 需DinD |
| **Jib** | Maven/Gradle 插件 | 无需 Dockerfile | 高 | Java 应用快速构建 | ✅ 插件 |
| **ko** | Go 环境 | 无需 Dockerfile | 高 | Go 应用快速构建 | ✅ 工具 |
| **pack** | 任意环境 | Cloud Native Buildpacks | 中 | 标准化构建 | ⚠️ 需配置 |
| **img** | 任意环境 | 无 Daemon、Rootless | 高 | 轻量级场景 | ✅ Pod |
| **Podman** | Linux | Daemonless、Rootless | 高 | 替代 Docker | ✅ 节点 |

## Kaniko - Kubernetes 原生镜像构建

### 1. 架构与工作原理

```
┌─────────────────────────────────────────────────────────────────────────────────┐
│                           Kaniko 构建流程                                        │
├─────────────────────────────────────────────────────────────────────────────────┤
│                                                                                  │
│  ┌──────────────┐     ┌──────────────┐     ┌──────────────┐     ┌────────────┐ │
│  │              │     │              │     │              │     │            │ │
│  │  Git 仓库    │────▶│   Kaniko     │────▶│   OCI 镜像   │────▶│  Registry  │ │
│  │  / 构建上下文│     │   Executor   │     │              │     │            │ │
│  │              │     │              │     │              │     │            │ │
│  └──────────────┘     └──────────────┘     └──────────────┘     └────────────┘ │
│                              │                                                  │
│                              ▼                                                  │
│  ┌─────────────────────────────────────────────────────────────────────────┐   │
│  │                        Kaniko 内部工作流程                               │   │
│  │                                                                          │   │
│  │  1. 提取构建上下文                                                        │   │
│  │     ├── Git Clone (git://...)                                            │   │
│  │     ├── GCS Bucket (gs://...)                                            │   │
│  │     ├── S3 Bucket (s3://...)                                             │   │
│  │     └── Local Dir (/workspace)                                           │   │
│  │                                                                          │   │
│  │  2. 解析 Dockerfile                                                       │   │
│  │     ├── FROM 指令 → 拉取基础镜像                                          │   │
│  │     ├── COPY/ADD → 添加文件到层                                           │   │
│  │     └── RUN 指令 → 在用户空间执行                                          │   │
│  │                                                                          │   │
│  │  3. 构建镜像层                                                            │   │
│  │     ├── 每条指令创建新层                                                   │   │
│  │     ├── 计算层 digest                                                     │   │
│  │     └── 检查缓存是否命中                                                   │   │
│  │                                                                          │   │
│  │  4. 推送到 Registry                                                       │   │
│  │     ├── 推送镜像层 (blob)                                                 │   │
│  │     └── 推送镜像 manifest                                                 │   │
│  │                                                                          │   │
│  └─────────────────────────────────────────────────────────────────────────┘   │
│                                                                                  │
│  关键特性:                                                                       │
│  ✓ 无需 Docker Daemon - 完全在用户空间执行                                       │
│  ✓ 无需特权容器 - 不需要 privileged: true                                        │
│  ✓ 支持多阶段构建 - 完整 Dockerfile 支持                                          │
│  ✓ 远程缓存 - 使用 Registry 作为缓存后端                                          │
│                                                                                  │
└─────────────────────────────────────────────────────────────────────────────────┘
```

### 2. 生产级 Kaniko 配置

```yaml
# kaniko-build-pod.yaml - 完整生产配置
apiVersion: v1
kind: Pod
metadata:
  name: kaniko-build-${BUILD_ID}
  namespace: ci-builds
  labels:
    app: kaniko
    build-id: "${BUILD_ID}"
  annotations:
    # 资源配额标记
    scheduler.alpha.kubernetes.io/critical-pod: ""
spec:
  # 使用专用 ServiceAccount
  serviceAccountName: kaniko-builder
  
  # 安全上下文
  securityContext:
    runAsUser: 0
    fsGroup: 0
  
  # 初始化容器 - 准备构建上下文
  initContainers:
    - name: git-clone
      image: alpine/git:latest
      command:
        - /bin/sh
        - -c
        - |
          git clone --depth 1 --branch ${GIT_BRANCH} ${GIT_REPO} /workspace
          cd /workspace
          git checkout ${GIT_COMMIT}
          echo "Build context ready: $(ls -la)"
      env:
        - name: GIT_REPO
          value: "https://github.com/myorg/myapp.git"
        - name: GIT_BRANCH
          value: "main"
        - name: GIT_COMMIT
          value: "${COMMIT_SHA}"
      volumeMounts:
        - name: workspace
          mountPath: /workspace
        - name: git-credentials
          mountPath: /root/.git-credentials
          subPath: .git-credentials
          readOnly: true
  
  containers:
    - name: kaniko
      image: gcr.io/kaniko-project/executor:v1.20.0
      args:
        # 构建上下文
        - "--context=dir:///workspace"
        - "--dockerfile=/workspace/Dockerfile"
        
        # 目标镜像
        - "--destination=${REGISTRY}/${IMAGE_NAME}:${IMAGE_TAG}"
        - "--destination=${REGISTRY}/${IMAGE_NAME}:latest"
        
        # 缓存配置 (强烈推荐)
        - "--cache=true"
        - "--cache-repo=${REGISTRY}/${IMAGE_NAME}/cache"
        - "--cache-ttl=168h"  # 7天缓存
        
        # 构建优化
        - "--snapshotMode=redo"
        - "--use-new-run"
        - "--compressed-caching=true"
        
        # 多平台构建 (可选)
        # - "--customPlatform=linux/amd64"
        # - "--customPlatform=linux/arm64"
        
        # 构建参数
        - "--build-arg=BUILD_DATE=${BUILD_DATE}"
        - "--build-arg=VCS_REF=${COMMIT_SHA}"
        - "--build-arg=VERSION=${IMAGE_TAG}"
        
        # 标签
        - "--label=org.opencontainers.image.created=${BUILD_DATE}"
        - "--label=org.opencontainers.image.revision=${COMMIT_SHA}"
        - "--label=org.opencontainers.image.version=${IMAGE_TAG}"
        
        # 日志级别
        - "--verbosity=info"
        
        # 镜像 Digest 输出
        - "--digest-file=/workspace/image-digest"
        
        # 推送重试
        - "--push-retry=3"
      
      env:
        - name: REGISTRY
          value: "registry.cn-hangzhou.aliyuncs.com/myns"
        - name: IMAGE_NAME
          value: "myapp"
        - name: IMAGE_TAG
          valueFrom:
            fieldRef:
              fieldPath: metadata.labels['build-id']
      
      volumeMounts:
        - name: workspace
          mountPath: /workspace
        - name: docker-config
          mountPath: /kaniko/.docker/
          readOnly: true
      
      resources:
        requests:
          cpu: "1"
          memory: "2Gi"
          ephemeral-storage: "10Gi"
        limits:
          cpu: "4"
          memory: "8Gi"
          ephemeral-storage: "50Gi"
  
  volumes:
    - name: workspace
      emptyDir: {}
    - name: docker-config
      secret:
        secretName: kaniko-docker-config
        items:
          - key: .dockerconfigjson
            path: config.json
    - name: git-credentials
      secret:
        secretName: git-credentials
  
  restartPolicy: Never
  
  # 节点选择 - 使用构建专用节点
  nodeSelector:
    node-type: build
  
  # 容忍构建节点污点
  tolerations:
    - key: "dedicated"
      operator: "Equal"
      value: "build"
      effect: "NoSchedule"
  
  # 构建超时
  activeDeadlineSeconds: 3600  # 1小时
```

```yaml
# kaniko-docker-config-secret.yaml
apiVersion: v1
kind: Secret
metadata:
  name: kaniko-docker-config
  namespace: ci-builds
type: kubernetes.io/dockerconfigjson
stringData:
  .dockerconfigjson: |
    {
      "auths": {
        "registry.cn-hangzhou.aliyuncs.com": {
          "auth": "${BASE64_ENCODED_AUTH}"
        },
        "ghcr.io": {
          "auth": "${GITHUB_TOKEN_BASE64}"
        }
      },
      "credHelpers": {
        "gcr.io": "gcr",
        "us.gcr.io": "gcr",
        "eu.gcr.io": "gcr",
        "asia.gcr.io": "gcr"
      }
    }
```

### 3. CI/CD 集成示例

```yaml
# .gitlab-ci.yml - GitLab CI 集成
stages:
  - build
  - scan
  - deploy

variables:
  REGISTRY: registry.cn-hangzhou.aliyuncs.com/myns
  IMAGE_NAME: myapp
  KANIKO_IMAGE: gcr.io/kaniko-project/executor:v1.20.0

build:
  stage: build
  image:
    name: $KANIKO_IMAGE
    entrypoint: [""]
  script:
    - |
      /kaniko/executor \
        --context "${CI_PROJECT_DIR}" \
        --dockerfile "${CI_PROJECT_DIR}/Dockerfile" \
        --destination "${REGISTRY}/${IMAGE_NAME}:${CI_COMMIT_SHA}" \
        --destination "${REGISTRY}/${IMAGE_NAME}:${CI_COMMIT_REF_SLUG}" \
        --cache=true \
        --cache-repo="${REGISTRY}/${IMAGE_NAME}/cache" \
        --build-arg "BUILD_DATE=$(date -u +%Y-%m-%dT%H:%M:%SZ)" \
        --build-arg "VCS_REF=${CI_COMMIT_SHA}" \
        --build-arg "VERSION=${CI_COMMIT_REF_NAME}" \
        --digest-file /workspace/image-digest
    - echo "IMAGE_DIGEST=$(cat /workspace/image-digest)" >> build.env
  artifacts:
    reports:
      dotenv: build.env
  rules:
    - if: $CI_COMMIT_BRANCH
    - if: $CI_COMMIT_TAG

scan:
  stage: scan
  image: aquasec/trivy:latest
  script:
    - trivy image --exit-code 1 --severity HIGH,CRITICAL "${REGISTRY}/${IMAGE_NAME}@${IMAGE_DIGEST}"
  needs:
    - build
  rules:
    - if: $CI_COMMIT_BRANCH == "main"
    - if: $CI_COMMIT_TAG
```

```yaml
# .github/workflows/build.yaml - GitHub Actions 集成
name: Build and Push

on:
  push:
    branches: [main]
    tags: ['v*']
  pull_request:
    branches: [main]

env:
  REGISTRY: ghcr.io
  IMAGE_NAME: ${{ github.repository }}

jobs:
  build:
    runs-on: ubuntu-latest
    permissions:
      contents: read
      packages: write
    
    steps:
      - name: Checkout
        uses: actions/checkout@v4
      
      - name: Set up Docker Buildx
        uses: docker/setup-buildx-action@v3
      
      - name: Log in to Container Registry
        uses: docker/login-action@v3
        with:
          registry: ${{ env.REGISTRY }}
          username: ${{ github.actor }}
          password: ${{ secrets.GITHUB_TOKEN }}
      
      - name: Extract metadata
        id: meta
        uses: docker/metadata-action@v5
        with:
          images: ${{ env.REGISTRY }}/${{ env.IMAGE_NAME }}
          tags: |
            type=ref,event=branch
            type=ref,event=pr
            type=semver,pattern={{version}}
            type=semver,pattern={{major}}.{{minor}}
            type=sha
      
      - name: Build and push
        uses: docker/build-push-action@v5
        with:
          context: .
          push: ${{ github.event_name != 'pull_request' }}
          tags: ${{ steps.meta.outputs.tags }}
          labels: ${{ steps.meta.outputs.labels }}
          cache-from: type=gha
          cache-to: type=gha,mode=max
          build-args: |
            BUILD_DATE=${{ github.event.head_commit.timestamp }}
            VCS_REF=${{ github.sha }}
            VERSION=${{ github.ref_name }}
      
      - name: Scan image
        uses: aquasecurity/trivy-action@master
        with:
          image-ref: ${{ env.REGISTRY }}/${{ env.IMAGE_NAME }}:sha-${{ github.sha }}
          format: 'sarif'
          output: 'trivy-results.sarif'
          severity: 'CRITICAL,HIGH'
      
      - name: Upload scan results
        uses: github/codeql-action/upload-sarif@v2
        with:
          sarif_file: 'trivy-results.sarif'
```

```groovy
// Jenkinsfile - Jenkins Pipeline 集成
pipeline {
    agent {
        kubernetes {
            yaml '''
apiVersion: v1
kind: Pod
spec:
  containers:
  - name: kaniko
    image: gcr.io/kaniko-project/executor:debug
    command:
    - sleep
    args:
    - infinity
    volumeMounts:
    - name: docker-config
      mountPath: /kaniko/.docker/
  - name: trivy
    image: aquasec/trivy:latest
    command:
    - sleep
    args:
    - infinity
  volumes:
  - name: docker-config
    secret:
      secretName: kaniko-docker-config
'''
        }
    }
    
    environment {
        REGISTRY = 'registry.cn-hangzhou.aliyuncs.com/myns'
        IMAGE_NAME = 'myapp'
    }
    
    stages {
        stage('Build') {
            steps {
                container('kaniko') {
                    sh '''
                        /kaniko/executor \
                            --context ${WORKSPACE} \
                            --dockerfile ${WORKSPACE}/Dockerfile \
                            --destination ${REGISTRY}/${IMAGE_NAME}:${BUILD_NUMBER} \
                            --destination ${REGISTRY}/${IMAGE_NAME}:${GIT_COMMIT} \
                            --cache=true \
                            --cache-repo=${REGISTRY}/${IMAGE_NAME}/cache \
                            --digest-file /tmp/image-digest
                    '''
                    script {
                        env.IMAGE_DIGEST = sh(script: 'cat /tmp/image-digest', returnStdout: true).trim()
                    }
                }
            }
        }
        
        stage('Scan') {
            steps {
                container('trivy') {
                    sh """
                        trivy image \
                            --exit-code 1 \
                            --severity HIGH,CRITICAL \
                            --format table \
                            ${REGISTRY}/${IMAGE_NAME}@${IMAGE_DIGEST}
                    """
                }
            }
        }
    }
}
```

## BuildKit - 高性能构建引擎

### 1. BuildKit 架构

```
┌─────────────────────────────────────────────────────────────────────────────────┐
│                           BuildKit 架构                                          │
├─────────────────────────────────────────────────────────────────────────────────┤
│                                                                                  │
│  ┌──────────────────────────────────────────────────────────────────────────┐   │
│  │                         客户端层 (Client)                                 │   │
│  │                                                                           │   │
│  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐  ┌───────────────┐   │   │
│  │  │   docker    │  │   buildctl  │  │   nerdctl   │  │    buildx     │   │   │
│  │  │   build     │  │    (CLI)    │  │    build    │  │    (multi)    │   │   │
│  │  └──────┬──────┘  └──────┬──────┘  └──────┬──────┘  └───────┬───────┘   │   │
│  └─────────┼────────────────┼────────────────┼─────────────────┼───────────┘   │
│            │                │                │                 │               │
│            └────────────────┴────────────────┴─────────────────┘               │
│                                      │                                          │
│                                      ▼                                          │
│  ┌──────────────────────────────────────────────────────────────────────────┐   │
│  │                       BuildKit Daemon (buildkitd)                         │   │
│  │                                                                           │   │
│  │  ┌─────────────────────────────────────────────────────────────────────┐ │   │
│  │  │                      前端 (Frontend)                                 │ │   │
│  │  │  ┌───────────────┐  ┌───────────────┐  ┌───────────────────────┐   │ │   │
│  │  │  │  Dockerfile   │  │    LLB       │  │    Buildpacks         │   │ │   │
│  │  │  │   Frontend    │  │   Frontend   │  │    Frontend           │   │ │   │
│  │  │  └───────────────┘  └───────────────┘  └───────────────────────┘   │ │   │
│  │  └─────────────────────────────────────────────────────────────────────┘ │   │
│  │                                 │                                         │   │
│  │                                 ▼                                         │   │
│  │  ┌─────────────────────────────────────────────────────────────────────┐ │   │
│  │  │                      LLB (Low-Level Build)                          │ │   │
│  │  │                    构建图 DAG 表示                                   │ │   │
│  │  └─────────────────────────────────────────────────────────────────────┘ │   │
│  │                                 │                                         │   │
│  │                                 ▼                                         │   │
│  │  ┌─────────────────────────────────────────────────────────────────────┐ │   │
│  │  │                      求解器 (Solver)                                 │ │   │
│  │  │  ┌───────────────┐  ┌───────────────┐  ┌───────────────────────┐   │ │   │
│  │  │  │   并行执行    │  │   缓存检查    │  │     依赖解析          │   │ │   │
│  │  │  │   Scheduler   │  │   Cache       │  │     Graph             │   │ │   │
│  │  │  └───────────────┘  └───────────────┘  └───────────────────────┘   │ │   │
│  │  └─────────────────────────────────────────────────────────────────────┘ │   │
│  │                                 │                                         │   │
│  │                                 ▼                                         │   │
│  │  ┌─────────────────────────────────────────────────────────────────────┐ │   │
│  │  │                      工作器 (Worker)                                 │ │   │
│  │  │  ┌───────────────┐  ┌───────────────┐  ┌───────────────────────┐   │ │   │
│  │  │  │  containerd   │  │     OCI       │  │      runc             │   │ │   │
│  │  │  │   Worker      │  │   Worker      │  │     (执行)            │   │ │   │
│  │  │  └───────────────┘  └───────────────┘  └───────────────────────┘   │ │   │
│  │  └─────────────────────────────────────────────────────────────────────┘ │   │
│  └──────────────────────────────────────────────────────────────────────────┘   │
│                                                                                  │
│  核心优势:                                                                       │
│  ✓ DAG 并行执行 - 独立构建步骤并行化                                              │
│  ✓ 分布式缓存 - 跨构建共享缓存层                                                  │
│  ✓ 多平台构建 - 单次构建多架构镜像                                                │
│  ✓ Secret 挂载 - 安全处理敏感信息                                                 │
│                                                                                  │
└─────────────────────────────────────────────────────────────────────────────────┘
```

### 2. BuildKit 高级特性示例

```dockerfile
# syntax=docker/dockerfile:1.7

# ==================== 多阶段并行构建 ====================

# 阶段 1: 构建后端 (Go)
FROM golang:1.22-alpine AS build-backend
WORKDIR /app

# 利用缓存挂载加速依赖下载
RUN --mount=type=cache,target=/go/pkg/mod \
    --mount=type=cache,target=/root/.cache/go-build \
    --mount=type=bind,source=go.mod,target=go.mod \
    --mount=type=bind,source=go.sum,target=go.sum \
    go mod download -x

# 复制源码并构建
COPY . .
RUN --mount=type=cache,target=/go/pkg/mod \
    --mount=type=cache,target=/root/.cache/go-build \
    CGO_ENABLED=0 GOOS=linux go build -ldflags="-s -w" -o /server ./cmd/server

# 阶段 2: 构建前端 (Node.js) - 与后端并行
FROM node:20-alpine AS build-frontend
WORKDIR /app

# npm 缓存挂载
RUN --mount=type=cache,target=/root/.npm \
    --mount=type=bind,source=package.json,target=package.json \
    --mount=type=bind,source=package-lock.json,target=package-lock.json \
    npm ci --prefer-offline

COPY . .
RUN npm run build

# 阶段 3: 安全处理 Secret
FROM alpine:latest AS secrets-handler
RUN --mount=type=secret,id=npmrc,target=/root/.npmrc \
    --mount=type=secret,id=aws_credentials,target=/root/.aws/credentials \
    # 使用 secret 但不会保留在镜像层中
    echo "Secrets processed"

# 阶段 4: 生产镜像
FROM gcr.io/distroless/static-debian12:nonroot AS production

# 标签
LABEL org.opencontainers.image.source="https://github.com/myorg/myapp"
LABEL org.opencontainers.image.description="MyApp Production Image"
LABEL org.opencontainers.image.licenses="MIT"

# 从构建阶段复制产物
COPY --from=build-backend /server /server
COPY --from=build-frontend /app/dist /static

# 非 root 用户
USER nonroot:nonroot

# 健康检查
HEALTHCHECK --interval=30s --timeout=3s --start-period=5s --retries=3 \
    CMD ["/server", "healthcheck"]

EXPOSE 8080
ENTRYPOINT ["/server"]
```

### 3. BuildKit 配置与使用

```bash
# ==================== 启用 BuildKit ====================

# Docker Desktop 默认已启用 BuildKit
# 手动启用
export DOCKER_BUILDKIT=1

# 或在 /etc/docker/daemon.json 配置
# {
#   "features": {
#     "buildkit": true
#   }
# }

# ==================== 使用 buildx (多平台构建) ====================

# 创建 builder 实例
docker buildx create --name mybuilder --driver docker-container --use

# 检查 builder
docker buildx inspect --bootstrap

# 多平台构建并推送
docker buildx build \
  --platform linux/amd64,linux/arm64 \
  --tag myregistry.com/myapp:latest \
  --push \
  .

# ==================== 缓存配置 ====================

# 使用 Registry 缓存
docker buildx build \
  --cache-from type=registry,ref=myregistry.com/myapp:buildcache \
  --cache-to type=registry,ref=myregistry.com/myapp:buildcache,mode=max \
  --tag myregistry.com/myapp:latest \
  --push \
  .

# 使用本地缓存
docker buildx build \
  --cache-from type=local,src=/tmp/buildcache \
  --cache-to type=local,dest=/tmp/buildcache,mode=max \
  --tag myapp:latest \
  .

# 使用 GitHub Actions 缓存
docker buildx build \
  --cache-from type=gha \
  --cache-to type=gha,mode=max \
  --tag myapp:latest \
  .

# ==================== Secret 挂载 ====================

# 构建时挂载 Secret
docker buildx build \
  --secret id=npmrc,src=$HOME/.npmrc \
  --secret id=aws_credentials,src=$HOME/.aws/credentials \
  --tag myapp:latest \
  .

# SSH 挂载 (访问私有仓库)
docker buildx build \
  --ssh default=$SSH_AUTH_SOCK \
  --tag myapp:latest \
  .

# ==================== 输出配置 ====================

# 输出到本地目录 (不推送)
docker buildx build \
  --output type=local,dest=./output \
  .

# 输出为 tar 文件
docker buildx build \
  --output type=tar,dest=./image.tar \
  .

# 输出 OCI 格式
docker buildx build \
  --output type=oci,dest=./image-oci.tar \
  .

# ==================== 构建参数与标签 ====================

docker buildx build \
  --build-arg BUILD_DATE=$(date -u +%Y-%m-%dT%H:%M:%SZ) \
  --build-arg VCS_REF=$(git rev-parse HEAD) \
  --build-arg VERSION=$(git describe --tags --always) \
  --label org.opencontainers.image.created=$(date -u +%Y-%m-%dT%H:%M:%SZ) \
  --label org.opencontainers.image.revision=$(git rev-parse HEAD) \
  --tag myapp:latest \
  .
```

### 4. Kubernetes 中运行 BuildKit

```yaml
# buildkit-deployment.yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: buildkitd
  namespace: ci-builds
spec:
  replicas: 3
  selector:
    matchLabels:
      app: buildkitd
  template:
    metadata:
      labels:
        app: buildkitd
    spec:
      containers:
        - name: buildkitd
          image: moby/buildkit:v0.13.0-rootless
          args:
            - --addr
            - unix:///run/user/1000/buildkit/buildkitd.sock
            - --addr
            - tcp://0.0.0.0:1234
            - --oci-worker-gc=true
            - --oci-worker-gc-keepstorage=20000
          ports:
            - containerPort: 1234
              protocol: TCP
          securityContext:
            seccompProfile:
              type: Unconfined
            apparmor.security.beta.kubernetes.io/container: unconfined
          volumeMounts:
            - name: buildkit-state
              mountPath: /home/user/.local/share/buildkit
          resources:
            requests:
              cpu: "1"
              memory: "2Gi"
            limits:
              cpu: "4"
              memory: "8Gi"
      volumes:
        - name: buildkit-state
          persistentVolumeClaim:
            claimName: buildkit-state
---
apiVersion: v1
kind: Service
metadata:
  name: buildkitd
  namespace: ci-builds
spec:
  selector:
    app: buildkitd
  ports:
    - port: 1234
      targetPort: 1234
---
# 使用 buildctl 连接
# buildctl --addr tcp://buildkitd.ci-builds:1234 build \
#   --frontend dockerfile.v0 \
#   --local context=. \
#   --local dockerfile=. \
#   --output type=image,name=myregistry.com/myapp:latest,push=true
```

## Buildah - Rootless OCI 构建

### 1. Buildah 完整使用指南

```bash
# ==================== 基础构建 ====================

# 从 Dockerfile 构建 (类似 docker build)
buildah bud -t myapp:v1.0 .

# 指定 Dockerfile
buildah bud -f Dockerfile.prod -t myapp:v1.0 .

# 构建参数
buildah bud \
  --build-arg VERSION=1.0.0 \
  --build-arg BUILD_DATE=$(date -u +%Y-%m-%dT%H:%M:%SZ) \
  -t myapp:v1.0 \
  .

# 多平台构建
buildah bud \
  --platform linux/amd64,linux/arm64 \
  --manifest myapp:v1.0 \
  .

# ==================== 交互式构建 (无需 Dockerfile) ====================

# 从基础镜像创建容器
container=$(buildah from alpine:latest)

# 运行命令
buildah run $container -- apk add --no-cache nginx

# 复制文件
buildah copy $container ./config/nginx.conf /etc/nginx/nginx.conf
buildah copy $container ./html /usr/share/nginx/html

# 设置环境变量
buildah config --env NGINX_VERSION=1.24 $container

# 设置工作目录
buildah config --workingdir /usr/share/nginx/html $container

# 设置入口点和命令
buildah config --entrypoint '["nginx"]' $container
buildah config --cmd '["-g", "daemon off;"]' $container

# 设置端口
buildah config --port 80 $container

# 设置标签
buildah config --label maintainer="admin@example.com" $container
buildah config --label version="1.0" $container

# 设置用户
buildah config --user nginx $container

# 提交为镜像
buildah commit $container myapp:v1.0

# 清理容器
buildah rm $container

# ==================== 镜像管理 ====================

# 列出镜像
buildah images

# 检查镜像
buildah inspect myapp:v1.0

# 打标签
buildah tag myapp:v1.0 myregistry.com/myapp:v1.0

# 推送镜像
buildah push myapp:v1.0 docker://myregistry.com/myapp:v1.0

# 推送到不同格式
buildah push myapp:v1.0 oci:./myapp-oci:v1.0
buildah push myapp:v1.0 docker-archive:./myapp.tar:myapp:v1.0
buildah push myapp:v1.0 dir:./myapp-dir

# 删除镜像
buildah rmi myapp:v1.0

# ==================== Manifest (多架构镜像) ====================

# 创建 manifest 列表
buildah manifest create myapp:v1.0

# 添加不同架构镜像
buildah manifest add myapp:v1.0 myapp:v1.0-amd64
buildah manifest add myapp:v1.0 myapp:v1.0-arm64

# 推送 manifest
buildah manifest push --all myapp:v1.0 docker://myregistry.com/myapp:v1.0

# ==================== 存储和缓存 ====================

# 清理构建缓存
buildah prune

# 清理所有未使用的数据
buildah prune -a

# 指定存储位置
export BUILDAH_ROOT=/var/lib/containers/storage
```

### 2. Buildah 在 Kubernetes 中使用

```yaml
# buildah-pod.yaml
apiVersion: v1
kind: Pod
metadata:
  name: buildah-build
  namespace: ci-builds
spec:
  containers:
    - name: buildah
      image: quay.io/buildah/stable:latest
      command:
        - /bin/bash
        - -c
        - |
          # 配置存储
          sed -i 's/^driver = .*/driver = "vfs"/' /etc/containers/storage.conf
          
          # 构建镜像
          buildah bud \
            --isolation chroot \
            --format docker \
            --layers \
            --cache-from ${CACHE_REPO} \
            --cache-to ${CACHE_REPO} \
            -t ${DESTINATION} \
            /workspace
          
          # 推送镜像
          buildah push \
            --creds ${REGISTRY_USER}:${REGISTRY_PASSWORD} \
            ${DESTINATION}
      env:
        - name: DESTINATION
          value: "registry.example.com/myapp:v1.0"
        - name: CACHE_REPO
          value: "registry.example.com/myapp/cache"
        - name: REGISTRY_USER
          valueFrom:
            secretKeyRef:
              name: registry-credentials
              key: username
        - name: REGISTRY_PASSWORD
          valueFrom:
            secretKeyRef:
              name: registry-credentials
              key: password
      securityContext:
        privileged: false
        capabilities:
          add:
            - SETUID
            - SETGID
      volumeMounts:
        - name: workspace
          mountPath: /workspace
        - name: storage
          mountPath: /var/lib/containers
      resources:
        requests:
          cpu: "500m"
          memory: "1Gi"
        limits:
          cpu: "2"
          memory: "4Gi"
  volumes:
    - name: workspace
      emptyDir: {}
    - name: storage
      emptyDir: {}
  restartPolicy: Never
```

## Jib - Java 应用快速构建

### 1. Maven 集成

```xml
<!-- pom.xml -->
<project>
    <build>
        <plugins>
            <plugin>
                <groupId>com.google.cloud.tools</groupId>
                <artifactId>jib-maven-plugin</artifactId>
                <version>3.4.0</version>
                <configuration>
                    <!-- 基础镜像 -->
                    <from>
                        <image>eclipse-temurin:21-jre-alpine</image>
                        <platforms>
                            <platform>
                                <architecture>amd64</architecture>
                                <os>linux</os>
                            </platform>
                            <platform>
                                <architecture>arm64</architecture>
                                <os>linux</os>
                            </platform>
                        </platforms>
                    </from>
                    
                    <!-- 目标镜像 -->
                    <to>
                        <image>registry.cn-hangzhou.aliyuncs.com/myns/myapp</image>
                        <tags>
                            <tag>${project.version}</tag>
                            <tag>latest</tag>
                        </tags>
                        <auth>
                            <username>${env.REGISTRY_USER}</username>
                            <password>${env.REGISTRY_PASSWORD}</password>
                        </auth>
                    </to>
                    
                    <!-- 容器配置 -->
                    <container>
                        <!-- JVM 参数 -->
                        <jvmFlags>
                            <jvmFlag>-Xms512m</jvmFlag>
                            <jvmFlag>-Xmx2g</jvmFlag>
                            <jvmFlag>-XX:+UseG1GC</jvmFlag>
                            <jvmFlag>-XX:+UseContainerSupport</jvmFlag>
                            <jvmFlag>-XX:MaxRAMPercentage=75.0</jvmFlag>
                            <jvmFlag>-Djava.security.egd=file:/dev/./urandom</jvmFlag>
                        </jvmFlags>
                        
                        <!-- 主类 -->
                        <mainClass>com.example.Application</mainClass>
                        
                        <!-- 端口 -->
                        <ports>
                            <port>8080</port>
                            <port>8081</port>
                        </ports>
                        
                        <!-- 环境变量 -->
                        <environment>
                            <SPRING_PROFILES_ACTIVE>production</SPRING_PROFILES_ACTIVE>
                            <TZ>Asia/Shanghai</TZ>
                        </environment>
                        
                        <!-- 标签 -->
                        <labels>
                            <org.opencontainers.image.title>${project.name}</org.opencontainers.image.title>
                            <org.opencontainers.image.version>${project.version}</org.opencontainers.image.version>
                            <org.opencontainers.image.vendor>MyCompany</org.opencontainers.image.vendor>
                        </labels>
                        
                        <!-- 用户 -->
                        <user>1000:1000</user>
                        
                        <!-- 工作目录 -->
                        <workingDirectory>/app</workingDirectory>
                        
                        <!-- 时间戳 (可复现构建) -->
                        <creationTime>USE_CURRENT_TIMESTAMP</creationTime>
                        <filesModificationTime>USE_CURRENT_TIMESTAMP</filesModificationTime>
                    </container>
                    
                    <!-- 额外目录 -->
                    <extraDirectories>
                        <paths>
                            <path>
                                <from>src/main/jib</from>
                                <into>/app</into>
                            </path>
                        </paths>
                        <permissions>
                            <permission>
                                <file>/app/entrypoint.sh</file>
                                <mode>755</mode>
                            </permission>
                        </permissions>
                    </extraDirectories>
                    
                    <!-- 允许不安全的 Registry -->
                    <allowInsecureRegistries>false</allowInsecureRegistries>
                </configuration>
                
                <!-- 绑定到 package 阶段 -->
                <executions>
                    <execution>
                        <phase>package</phase>
                        <goals>
                            <goal>build</goal>
                        </goals>
                    </execution>
                </executions>
            </plugin>
        </plugins>
    </build>
</project>
```

```bash
# Maven 构建命令
# 构建并推送到 Registry
mvn compile jib:build

# 构建到本地 Docker daemon
mvn compile jib:dockerBuild

# 构建到 tar 文件
mvn compile jib:buildTar

# 跳过测试快速构建
mvn compile jib:build -DskipTests

# 指定目标镜像
mvn compile jib:build -Dimage=myregistry.com/myapp:v1.0
```

### 2. Gradle 集成

```groovy
// build.gradle.kts
plugins {
    id("java")
    id("org.springframework.boot") version "3.2.0"
    id("com.google.cloud.tools.jib") version "3.4.0"
}

jib {
    from {
        image = "eclipse-temurin:21-jre-alpine"
        platforms {
            platform {
                architecture = "amd64"
                os = "linux"
            }
            platform {
                architecture = "arm64"
                os = "linux"
            }
        }
    }
    
    to {
        image = "registry.cn-hangzhou.aliyuncs.com/myns/${project.name}"
        tags = setOf(project.version.toString(), "latest")
        auth {
            username = System.getenv("REGISTRY_USER")
            password = System.getenv("REGISTRY_PASSWORD")
        }
    }
    
    container {
        jvmFlags = listOf(
            "-Xms512m",
            "-Xmx2g",
            "-XX:+UseG1GC",
            "-XX:+UseContainerSupport",
            "-XX:MaxRAMPercentage=75.0"
        )
        mainClass = "com.example.Application"
        ports = listOf("8080", "8081")
        environment = mapOf(
            "SPRING_PROFILES_ACTIVE" to "production",
            "TZ" to "Asia/Shanghai"
        )
        labels.set(mapOf(
            "org.opencontainers.image.title" to project.name,
            "org.opencontainers.image.version" to project.version.toString()
        ))
        user = "1000:1000"
        workingDirectory = "/app"
        creationTime.set("USE_CURRENT_TIMESTAMP")
    }
    
    extraDirectories {
        paths {
            path {
                setFrom("src/main/jib")
                into = "/app"
            }
        }
        permissions.set(mapOf(
            "/app/entrypoint.sh" to "755"
        ))
    }
}
```

## ko - Go 应用快速构建

### 1. ko 配置与使用

```yaml
# .ko.yaml
defaultBaseImage: gcr.io/distroless/static-debian12:nonroot

builds:
  - id: server
    main: ./cmd/server
    ldflags:
      - -s -w
      - -X main.version={{.Version}}
      - -X main.commit={{.Commit}}
      - -X main.date={{.Date}}
    env:
      - CGO_ENABLED=0

baseImageOverrides:
  # 不同模块使用不同基础镜像
  github.com/myorg/myapp/cmd/server: gcr.io/distroless/static-debian12:nonroot
  github.com/myorg/myapp/cmd/worker: gcr.io/distroless/base-debian12:nonroot
```

```bash
# ==================== 基础使用 ====================

# 设置默认 Registry
export KO_DOCKER_REPO=registry.cn-hangzhou.aliyuncs.com/myns

# 构建并推送单个应用
ko build ./cmd/server

# 构建并获取镜像引用
ko build ./cmd/server --bare

# 构建到本地 Docker
ko build ./cmd/server --local

# ==================== 多平台构建 ====================

# 构建多架构镜像
ko build ./cmd/server --platform=linux/amd64,linux/arm64

# ==================== 与 Kubernetes 集成 ====================

# 替换 YAML 中的镜像引用
ko resolve -f deployment.yaml

# 应用到集群
ko apply -f deployment.yaml

# 删除
ko delete -f deployment.yaml

# ==================== 发布 ====================

# 发布到 Registry (使用 Git tag 作为版本)
ko publish ./cmd/server --tags=$(git describe --tags --always)

# 带基础镜像摘要
ko publish ./cmd/server --image-refs=images.txt
```

```yaml
# deployment.yaml - ko 自动替换镜像
apiVersion: apps/v1
kind: Deployment
metadata:
  name: myapp
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
      containers:
        - name: server
          # ko 会自动替换这个为实际镜像
          image: ko://github.com/myorg/myapp/cmd/server
          ports:
            - containerPort: 8080
```

## 镜像优化最佳实践

### 1. 多阶段构建模式

```dockerfile
# 模式 1: 基础多阶段构建
# syntax=docker/dockerfile:1.7

# 构建阶段
FROM golang:1.22-alpine AS builder
WORKDIR /app
COPY go.* ./
RUN go mod download
COPY . .
RUN CGO_ENABLED=0 go build -ldflags="-s -w" -o /server

# 运行阶段
FROM gcr.io/distroless/static-debian12:nonroot
COPY --from=builder /server /server
USER nonroot:nonroot
ENTRYPOINT ["/server"]
```

```dockerfile
# 模式 2: 测试-构建-运行分离
# syntax=docker/dockerfile:1.7

# 基础依赖
FROM golang:1.22-alpine AS base
WORKDIR /app
COPY go.* ./
RUN --mount=type=cache,target=/go/pkg/mod \
    go mod download

# 测试阶段
FROM base AS test
COPY . .
RUN --mount=type=cache,target=/go/pkg/mod \
    --mount=type=cache,target=/root/.cache/go-build \
    go test -v ./...

# 构建阶段
FROM base AS builder
COPY . .
RUN --mount=type=cache,target=/go/pkg/mod \
    --mount=type=cache,target=/root/.cache/go-build \
    CGO_ENABLED=0 go build -ldflags="-s -w" -o /server

# 运行阶段
FROM gcr.io/distroless/static-debian12:nonroot AS production
COPY --from=builder /server /server
USER nonroot:nonroot
ENTRYPOINT ["/server"]
```

```dockerfile
# 模式 3: 开发-生产分离
# syntax=docker/dockerfile:1.7

# 共享基础
FROM node:20-alpine AS base
WORKDIR /app
COPY package*.json ./

# 开发阶段
FROM base AS development
RUN npm install
COPY . .
CMD ["npm", "run", "dev"]

# 构建阶段
FROM base AS builder
RUN npm ci --only=production
COPY . .
RUN npm run build

# 生产阶段
FROM nginx:alpine AS production
COPY --from=builder /app/dist /usr/share/nginx/html
COPY nginx.conf /etc/nginx/nginx.conf
EXPOSE 80
CMD ["nginx", "-g", "daemon off;"]
```

### 2. 基础镜像选择指南

| 镜像类型 | 大小 | 安全性 | 适用场景 |
|---------|------|--------|---------|
| **gcr.io/distroless/static** | ~2MB | 最高 | Go 静态二进制 |
| **gcr.io/distroless/base** | ~20MB | 高 | 需要 glibc 的应用 |
| **gcr.io/distroless/cc** | ~25MB | 高 | 需要 libgcc 的应用 |
| **gcr.io/distroless/java** | ~200MB | 高 | Java 应用 |
| **alpine** | ~5MB | 中 | 需要 shell 的场景 |
| **scratch** | 0MB | 最高 | 完全静态二进制 |
| **chainguard/static** | ~2MB | 最高 | Wolfi 基础，签名验证 |

### 3. .dockerignore 最佳配置

```dockerignore
# .dockerignore

# 版本控制
.git
.gitignore
.gitattributes

# IDE 和编辑器
.idea/
.vscode/
*.swp
*.swo
*~

# 依赖目录 (在容器内安装)
node_modules/
vendor/
.npm/
.yarn/

# 构建产物
dist/
build/
out/
target/
bin/
*.exe
*.dll
*.so
*.dylib

# 测试和覆盖率
coverage/
.nyc_output/
*.test
*_test.go
__tests__/
*.spec.ts
*.spec.js

# 文档
docs/
*.md
!README.md
LICENSE
CHANGELOG*

# Docker 相关
Dockerfile*
docker-compose*.yml
.docker/
.dockerignore

# CI/CD
.github/
.gitlab-ci.yml
.travis.yml
Jenkinsfile
azure-pipelines.yml

# 环境和配置
.env
.env.*
*.local
secrets/
*.pem
*.key

# 日志和临时文件
logs/
*.log
tmp/
temp/
.cache/

# macOS
.DS_Store
.AppleDouble
.LSOverride

# Windows
Thumbs.db
ehthumbs.db
```

## 安全最佳实践

### 1. 镜像安全扫描流程

```yaml
# security-scan-pipeline.yaml
apiVersion: tekton.dev/v1beta1
kind: Pipeline
metadata:
  name: secure-build-pipeline
spec:
  params:
    - name: image-url
      type: string
  
  tasks:
    - name: build
      taskRef:
        name: kaniko-build
      params:
        - name: IMAGE
          value: $(params.image-url)
    
    - name: scan-vulnerabilities
      taskRef:
        name: trivy-scan
      runAfter:
        - build
      params:
        - name: IMAGE
          value: $(params.image-url)
        - name: SEVERITY
          value: "HIGH,CRITICAL"
        - name: EXIT_CODE
          value: "1"
    
    - name: sign-image
      taskRef:
        name: cosign-sign
      runAfter:
        - scan-vulnerabilities
      params:
        - name: IMAGE
          value: $(params.image-url)
    
    - name: generate-sbom
      taskRef:
        name: syft-sbom
      runAfter:
        - build
      params:
        - name: IMAGE
          value: $(params.image-url)
    
    - name: verify-signature
      taskRef:
        name: cosign-verify
      runAfter:
        - sign-image
      params:
        - name: IMAGE
          value: $(params.image-url)
```

### 2. Cosign 镜像签名

```bash
# ==================== 生成密钥对 ====================

# 生成签名密钥
cosign generate-key-pair

# 或使用 KMS
cosign generate-key-pair --kms awskms:///alias/cosign-key

# ==================== 签名镜像 ====================

# 使用私钥签名
cosign sign --key cosign.key myregistry.com/myapp:v1.0

# 使用 Keyless 签名 (推荐)
COSIGN_EXPERIMENTAL=1 cosign sign myregistry.com/myapp:v1.0

# 添加自定义注解
cosign sign \
  --key cosign.key \
  --annotations "build.id=${BUILD_ID}" \
  --annotations "git.commit=${GIT_COMMIT}" \
  myregistry.com/myapp:v1.0

# ==================== 验证签名 ====================

# 使用公钥验证
cosign verify --key cosign.pub myregistry.com/myapp:v1.0

# Keyless 验证
COSIGN_EXPERIMENTAL=1 cosign verify myregistry.com/myapp:v1.0

# 验证特定身份
cosign verify \
  --certificate-identity "builder@example.com" \
  --certificate-oidc-issuer "https://accounts.google.com" \
  myregistry.com/myapp:v1.0

# ==================== 附加 SBOM ====================

# 生成 SBOM
syft myregistry.com/myapp:v1.0 -o spdx-json > sbom.json

# 附加 SBOM 到镜像
cosign attach sbom --sbom sbom.json myregistry.com/myapp:v1.0

# 验证 SBOM
cosign verify-attestation \
  --type spdxjson \
  --key cosign.pub \
  myregistry.com/myapp:v1.0
```

### 3. Dockerfile 安全检查

```bash
# 使用 hadolint 检查 Dockerfile
hadolint Dockerfile

# 使用 trivy 检查配置
trivy config .

# 使用 dockle 检查最佳实践
dockle myregistry.com/myapp:v1.0
```

```dockerfile
# 安全 Dockerfile 模板
# syntax=docker/dockerfile:1.7

# 1. 使用特定版本标签
FROM golang:1.22.1-alpine3.19 AS builder

# 2. 不使用 root
USER 1000:1000

# 3. 设置工作目录
WORKDIR /app

# 4. 最小化层数，合并 RUN 命令
RUN --mount=type=cache,target=/go/pkg/mod \
    --mount=type=bind,source=go.mod,target=go.mod \
    --mount=type=bind,source=go.sum,target=go.sum \
    go mod download

COPY --chown=1000:1000 . .

# 5. 静态编译，禁用 CGO
RUN CGO_ENABLED=0 GOOS=linux go build \
    -ldflags="-s -w -extldflags '-static'" \
    -o /server ./cmd/server

# 6. 使用 distroless 或 scratch
FROM gcr.io/distroless/static-debian12:nonroot

# 7. 添加标签
LABEL org.opencontainers.image.source="https://github.com/myorg/myapp"
LABEL org.opencontainers.image.description="Secure Application"
LABEL org.opencontainers.image.licenses="MIT"

# 8. 从构建阶段复制
COPY --from=builder /server /server

# 9. 使用非 root 用户
USER nonroot:nonroot

# 10. 只暴露必要端口
EXPOSE 8080

# 11. 健康检查
HEALTHCHECK --interval=30s --timeout=3s --start-period=5s --retries=3 \
    CMD ["/server", "health"]

# 12. 使用 ENTRYPOINT
ENTRYPOINT ["/server"]
```

## 故障排查指南

### 常见问题与解决方案

| 问题 (Issue) | 诊断 (Diagnosis) | 解决方案 (Solution) |
|-------------|-----------------|-------------------|
| Kaniko 权限不足 | 检查 Secret 挂载 | 验证 docker config.json 格式 |
| 构建上下文过大 | `du -sh .` 检查大小 | 优化 .dockerignore |
| 缓存未命中 | 检查 Dockerfile 层顺序 | 将变化少的层放前面 |
| 多平台构建失败 | 检查 QEMU 模拟器 | `docker run --rm --privileged multiarch/qemu-user-static --reset -p yes` |
| 推送超时 | 检查网络和 Registry | 增加超时时间，检查代理设置 |
| 镜像层过大 | `docker history` 分析 | 多阶段构建，清理缓存 |
| OOM Killed | 检查资源限制 | 增加内存限制 |

### 诊断脚本

```bash
#!/bin/bash
# image-build-diagnose.sh

set -euo pipefail

IMAGE_NAME=${1:-"myapp:latest"}

echo "=== 镜像信息 ==="
docker inspect $IMAGE_NAME --format '{{.Id}}'
docker inspect $IMAGE_NAME --format '{{.Size}}' | numfmt --to=iec

echo ""
echo "=== 镜像历史 ==="
docker history $IMAGE_NAME --no-trunc

echo ""
echo "=== 层大小分析 ==="
docker history $IMAGE_NAME --format "{{.Size}}\t{{.CreatedBy}}" | head -20

echo ""
echo "=== 漏洞扫描 ==="
trivy image --severity HIGH,CRITICAL $IMAGE_NAME

echo ""
echo "=== 最佳实践检查 ==="
dockle $IMAGE_NAME 2>/dev/null || echo "dockle not installed"

echo ""
echo "=== SBOM 生成 ==="
syft $IMAGE_NAME -o table | head -30
```

## 速查表

### Kaniko 参数速查

```bash
# 常用参数
--context          # 构建上下文 (dir://, git://, s3://, gs://)
--dockerfile       # Dockerfile 路径
--destination      # 目标镜像 (可多次指定)
--cache            # 启用缓存
--cache-repo       # 缓存仓库
--cache-ttl        # 缓存 TTL (默认 2 周)
--build-arg        # 构建参数
--label            # 镜像标签
--target           # 目标阶段 (多阶段构建)
--snapshot-mode    # 快照模式 (full, redo, time)
--verbosity        # 日志级别 (panic, fatal, error, warn, info, debug, trace)
--push-retry       # 推送重试次数
--digest-file      # 输出 digest 到文件
```

### BuildKit/buildx 参数速查

```bash
# 构建
docker buildx build
  --platform        # 目标平台
  --cache-from      # 缓存来源
  --cache-to        # 缓存目标
  --secret          # 挂载 Secret
  --ssh             # 挂载 SSH agent
  --output          # 输出配置
  --push            # 推送到 Registry
  --load            # 加载到本地 Docker
```

### 镜像大小优化清单

- [ ] 使用多阶段构建
- [ ] 选择最小基础镜像 (distroless/alpine)
- [ ] 合并 RUN 命令
- [ ] 清理缓存和临时文件
- [ ] 使用 .dockerignore
- [ ] 静态编译 (CGO_ENABLED=0)
- [ ] 压缩二进制 (upx)
- [ ] 移除调试符号 (-ldflags="-s -w")

---

**表格底部标记**: Kusheet Project, 作者 Allen Galler (allengaller@gmail.com)
