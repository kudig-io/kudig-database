---
title: Cloud Native Buildpacks 生产指南
description: 面向阿里云专有云 Java/Go/Node.js 应用的 Cloud Native Buildpacks 实战：pack CLI、builder、分层镜像、可复现构建与
  CI/CD 集成。
summary: 面向阿里云专有云 Java/Go/Node.js 应用的 Cloud Native Buildpacks 实战：pack CLI、builder、分层镜像、可复现构建与
  CI/CD 集成。
category: container-runtime
tags:
- buildpacks
- pack
- builder
- layered-image
- reproducible-build
- java
- go
- cicd
tier: supporting
created: '2026-06-29'
updated: '2026-06-29'
last_updated: 2026-06-29
difficulty: intermediate
audience:
- 平台工程师
- 应用开发者
- DevOps 工程师
estimated_read_time: 19min
intent_queries:
- Cloud Native Buildpacks 怎么用
- pack builder 分层镜像原理
- Buildpacks 与 Dockerfile 区别
trigger_keywords:
- buildpacks
- pack
- builder
- lifecycle
- layered image
- reproducible build
prerequisites:
- 容器运行时/04-image-build/01-buildkit-production-guide.md
- 容器运行时/02-image-management/01-harbor-enterprise-image-registry.md
- 发布变更/01-gitops/08-cicd-pipeline-patterns.md
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




# Cloud Native Buildpacks 生产指南

> 适用场景：希望减少 Dockerfile 维护成本、统一应用构建基线、利用分层缓存加速镜像构建的阿里云专有云研发团队与平台工程团队。

## 目录

- [1. 什么是 Cloud Native Buildpacks](#1-什么是-cloud-native-buildpacks)
- [2. 核心概念](#2-核心概念)
- [3. 安装 pack CLI](#3-安装-pack-cli)
- [4. 构建第一个应用镜像](#4-构建第一个应用镜像)
- [5. Builder 与 Stack 选择](#5-builder-与-stack-选择)
- [6. 分层镜像与缓存机制](#6-分层镜像与缓存机制)
- [7. 可复现构建与 SBOM](#7-可复现构建与-sbom)
- [8. CI/CD 集成](#8-cicd-集成)
- [9. 阿里云 Dragonwell 与 Java 构建](#9-阿里云-dragonwell-与-java-构建)
- [10. 从 Dockerfile 迁移到 Buildpacks](#10-从-dockerfile-迁移到-buildpacks)
- [11. 多平台构建实践](#11-多平台构建实践)
- [12. Buildpacks 与 ACR 镜像扫描](#12-buildpacks-与-acr-镜像扫描)
- [13. Buildpacks 与 GitOps 结合](#13-buildpacks-与-gitops-结合)
- [14. 生产检查清单](#14-生产检查清单)
- [15. 相关文档](#15-相关文档)

## 1. 什么是 Cloud Native Buildpacks

Cloud Native Buildpacks（CNB）是一种将源代码转换为符合 OCI 标准的容器镜像的技术。它最早由 Heroku 提出，后来由 Pivotal 与 Cloud Foundry 共同推动进入 CNCF。与 Dockerfile 不同，Buildpacks 不需要开发者手写镜像构建指令，而是由平台根据检测到的语言、框架自动选择合适的 buildpack，完成依赖安装、编译、打包。

在阿里云专有云场景中，Buildpacks 的价值在于：

- **降低开发者心智负担**：无需维护 Dockerfile；
- **统一基线**：由平台团队维护 builder，控制 JDK/Go/Node 版本与安全补丁；
- **分层缓存**：依赖层与代码层分离，代码变更时只重新构建应用层；
- **供应链安全**：原生支持 SBOM、provenance，便于合规审计。

## 2. 核心概念

| 概念 | 说明 |
| --- | --- |
| Buildpack | 检测应用类型并执行构建的最小单元，例如 Java Maven buildpack、Go buildpack |
| Builder | 一组 buildpack + stack + lifecycle 的集合，是构建的入口 |
| Stack | 运行 buildpack 所需的基础镜像，包含 build 镜像与 run 镜像 |
| Lifecycle | 执行 detect、analyze、restore、build、export、rebase 等阶段的二进制集合 |
| Platform | 调用 lifecycle 的环境，例如 pack CLI、Tekton、kpack |
| Image | 最终输出的 OCI 镜像，由多个可复用层组成 |

## 3. 安装 pack CLI

pack 是 Cloud Native Buildpacks 官方 CLI，支持 Linux/macOS/Windows。以下命令在 Alibaba Cloud Linux 3 上安装最新版。

```bash
# 下载并安装 pack CLI，用于本地或 CI 中触发 Buildpacks 构建
sudo yum install -y wget
wget https://github.com/buildpacks/pack/releases/download/v0.33.2/pack-v0.33.2-linux.tgz
tar -xzf pack-v0.33.2-linux.tgz
sudo mv pack /usr/local/bin/
pack version
```

安装完成后，建议设置默认 builder，减少每次构建时的参数输入。

```bash
# 设置 Paketo 基础 builder 为默认，支持 Java/Go/Node.js/Python 等主流语言
pack config default-builder paketobuildpacks/builder-jammy-base:latest
```

## 4. 构建第一个应用镜像

假设有一个 Spring Boot 项目，目录下包含 `pom.xml` 与 `src/`。使用 pack 构建时，buildpack 会自动检测 Maven 项目并执行 `mvn clean package`。

```bash
# 进入项目根目录，使用 pack 构建并推送到 ACR
pack build registry.cn-hangzhou.aliyuncs.com/demo/spring-app:v1.0.0 \
  --builder paketobuildpacks/builder-jammy-base:latest \
  --publish
```

构建过程会依次执行：

1. **Detect**：所有 buildpack 检测项目特征；
2. **Analyze**：分析之前构建的镜像层，决定哪些层可复用；
3. **Restore**：从缓存中恢复依赖；
4. **Build**：安装依赖、编译、打包；
5. **Export**：将应用层、依赖层、运行时层导出为 OCI 镜像；
6. **Cache**：更新缓存层供下次使用。

## 5. Builder 与 Stack 选择

### 5.1 官方 Builder

| Builder | 适用场景 | 特点 |
| --- | --- | --- |
| paketobuildpacks/builder-jammy-base | 通用生产环境 | 基于 Ubuntu Jammy，依赖成熟 |
| paketobuildpacks/builder-jammy-full | 需要完整工具链 | 包含更多系统库与编译工具 |
| paketobuildpacks/builder-jammy-tiny | 静态编译语言（Go/Rust） | 运行镜像极小，攻击面低 |
| heroku/builder:22 | Heroku 生态迁移 | 与 Heroku 构建行为一致 |

### 5.2 自定义 Builder

在专有云环境中，企业通常需要内网依赖源、统一 JDK 版本、内置公司 CA 证书。此时应创建自定义 builder 与 buildpack。

```bash
# 创建自定义 builder 目录结构
mkdir -p ~/cnb-builder/buildpacks ~/cnb-builder/stacks
cd ~/cnb-builder
```

自定义 builder 通过 `builder.toml` 描述：

```toml
# builder.toml
[[buildpacks]]
  id = "example/java-maven"
  version = "1.0.0"
  uri = "./buildpacks/java-maven"

[[order]]
  [[order.group]]
    id = "example/java-maven"
    version = "1.0.0"

[stack]
  id = "io.buildpacks.stacks.jammy"
  build-image = "registry-vpc.cn-hangzhou.aliyuncs.com/cnb/build:jammy"
  run-image = "registry-vpc.cn-hangzhou.aliyuncs.com/cnb/run:jammy"
  run-image-mirrors = ["registry-vpc.cn-hangzhou.aliyuncs.com/cnb/run:jammy"]
```

```bash
# 使用自定义 builder.toml 创建 builder 镜像并推送到 ACR
pack builder create registry.cn-hangzhou.aliyuncs.com/cnb/custom-builder:latest \
  --config builder.toml \
  --publish
```

## 6. 分层镜像与缓存机制

Buildpacks 生成的镜像通常包含以下层：

1. **运行层（run layer）**：来自 stack 的 run 镜像，包含 OS 与基础库；
2. **依赖层（dependencies）**：例如 JDK、Node.js 运行时；
3. **应用依赖层（app dependencies）**：例如 Maven 下载的 jar 包、node_modules；
4. **应用代码层（app code）**：编译后的可执行文件或字节码；
5. **启动层（launcher）**：Buildpacks 注入的启动脚本与环境变量。

这种分层结构使得：

- 仅修改业务代码时，只有最后一两层重新构建；
- 平台升级 JDK 补丁时，可以只替换依赖层；
- 多个应用共享同一基础层时，registry 存储与节点拉取更高效。

```bash
# 查看 Buildpacks 生成的镜像层与大小
pack inspect-image registry.cn-hangzhou.aliyuncs.com/demo/spring-app:v1.0.0
dive registry.cn-hangzhou.aliyuncs.com/demo/spring-app:v1.0.0
```

## 7. 可复现构建与 SBOM

### 7.1 可复现构建

Buildpacks 通过固定 builder、buildpack、stack 版本，以及使用 lock 文件（如 `package-lock.json`、`go.sum`）实现高度可复现的构建。生产环境应避免使用 `latest` tag，防止构建结果漂移。

```bash
# 使用固定版本的 builder 与 buildpack 构建，确保可复现性
pack build registry.cn-hangzhou.aliyuncs.com/demo/spring-app:v1.0.0 \
  --builder paketobuildpacks/builder-jammy-base:0.0.219 \
  --buildpack paketobuildpacks/java-maven:9.17.0 \
  --buildpack paketobuildpacks/spring-boot:5.27.0 \
  --publish
```

### 7.2 SBOM 导出

pack 可以直接导出镜像的 SBOM，便于安全审计与漏洞追踪。

```bash
# 导出镜像的 SBOM 为 SPDX JSON 格式
pack sbom download registry.cn-hangzhou.aliyuncs.com/demo/spring-app:v1.0.0 \
  --format spdx-json \
  --output-dir ./sbom
```

## 8. CI/CD 集成

### 8.1 Tekton 集成

在 [[发布变更/01-gitops/05-tekton-cloud-native-cicd.md|Tekton]] Pipeline 中，可使用 `buildpacks-io/pack` Task 触发构建。

```yaml
# tekton-buildpacks-taskrun.yaml
apiVersion: tekton.dev/v1
kind: TaskRun
metadata:
  name: buildpacks-build
spec:
  taskRef:
    name: buildpacks
  params:
    - name: IMAGE
      value: registry.cn-hangzhou.aliyuncs.com/demo/spring-app:v1.0.0
    - name: BUILDER_IMAGE
      value: paketobuildpacks/builder-jammy-base:latest
    - name: SOURCE_URL
      value: https://github.com/example/spring-app.git
  workspaces:
    - name: source
      emptyDir: {}
```

### 8.2 GitHub Actions / GitLab CI

```yaml
# .github/workflows/buildpacks.yml 片段
- name: Build image with pack
  uses: buildpacks/github-actions/setup-pack@v5.0.0
- run: |
    pack build registry.cn-hangzhou.aliyuncs.com/demo/app:${{ github.sha }} \
      --builder paketobuildpacks/builder-jammy-base:latest \
      --publish
```

### 8.3 kpack 平台化

对于需要大规模平台化的团队，可使用 VMware 开源的 kpack。它在 Kubernetes 中实现 image CRD，当源代码或 builder 更新时自动触发镜像构建与推送。

```yaml
# kpack-image.yaml
apiVersion: kpack.io/v1alpha2
kind: Image
metadata:
  name: spring-app
spec:
  tag: registry.cn-hangzhou.aliyuncs.com/demo/spring-app:latest
  builder:
    name: custom-builder
    kind: Builder
  source:
    git:
      url: https://github.com/example/spring-app.git
      revision: main
```

## 9. 阿里云 Dragonwell 与 Java 构建

对于 Java 应用，除了 Temurin，还可以选择阿里云 Dragonwell 作为 JDK 基础镜像，以获得更好的 Alibaba Cloud Linux 兼容性与性能优化。

```toml
# builder.toml 片段：使用 Dragonwell 作为 Java 运行时
[[buildpacks]]
  id = "alibabacloud/dragonwell"
  version = "1.0.0"
  uri = "docker://registry.cn-hangzhou.aliyuncs.com/cnb/dragonwell-buildpack:latest"

[[order]]
  [[order.group]]
    id = "alibabacloud/dragonwell"
```

```bash
# 使用自定义 Dragonwell builder 构建 Java 应用
pack build registry.cn-hangzhou.aliyuncs.com/demo/java-app:v1.0.0 \
  --builder registry.cn-hangzhou.aliyuncs.com/cnb/dragonwell-builder:latest \
  --env BP_JVM_VERSION=17 \
  --publish
```

## 10. 从 Dockerfile 迁移到 Buildpacks

迁移通常分为四个阶段：

1. **评估阶段**：确认项目是否依赖 Dockerfile 中的特殊指令（如 `RUN --mount=type=ssh`），Buildpacks 可能不支持；
2. **试点阶段**：选择 1-2 个无状态服务，使用 pack 本地构建并验证功能；
3. **平台化阶段**：定义企业级 builder，内置 ACR 仓库、Dragonwell JDK、公司 CA；
4. **推广阶段**：将 CI/CD 中的 `docker build` 替换为 `pack build`，并保留 Dockerfile 作为降级方案。

迁移过程中，建议保留 `project.toml` 以声明构建设置：

```toml
# project.toml
[project]
id = "demo.spring-app"
version = "1.0.0"
name = "Spring Boot Application"

[[build.buildpacks]]
uri = "paketobuildpacks/java-maven"
```

## 11. 多平台构建实践

ACK 集群可能同时包含 x86 与 ARM 节点。Buildpacks 通过 `--target` 参数支持多平台输出，但需要 builder 的 stack 也提供对应架构的 run 镜像。

```bash
# 构建同时支持 amd64 与 arm64 的镜像，并推送到 ACR
pack build registry.cn-hangzhou.aliyuncs.com/demo/app:v1.0.0 \
  --builder paketobuildpacks/builder-jammy-base:latest \
  --target linux/amd64 \
  --target linux/arm64 \
  --publish
```

多平台构建会显著增加构建时间与缓存复杂度。生产环境建议：

- 为不同架构准备独立的 builder 镜像，避免单 builder 体积过大；
- 在 CI 中使用 matrix job 并行构建不同架构；
- 通过 ACR 的多平台 manifest 统一引用。

## 12. Buildpacks 与 ACR 镜像扫描

构建完成后，应立刻触发镜像安全扫描。ACR 企业版支持自动扫描，也可在 CI 中调用 API。

```bash
# 推送后触发 ACR 镜像扫描
aliyun cr ScanRepoImage \
  --RepoNamespace demo \
  --RepoName app \
  --Tag v1.0.0 \
  --RegionId cn-hangzhou
```

Buildpacks 的 SBOM 可与扫描结果互补，帮助快速定位漏洞所在的依赖版本，例如某个 Log4j 版本来自哪一层 buildpack。

## 13. Buildpacks 与 GitOps 结合

在 GitOps 工作流中，可将 kpack Image 资源或 Tekton TaskRun 提交到 Git 仓库，由 Argo CD/Flux 同步到集群。当业务代码更新时，GitOps 触发构建并自动更新部署清单中的镜像 tag。

```yaml
# argocd-application-buildpacks.yaml 片段
source:
  repoURL: https://github.com/example/gitops.git
  targetRevision: main
  path: buildpacks
syncPolicy:
  automated:
    prune: true
    selfHeal: true
```

这种模式下，镜像构建与部署解耦，平台团队负责 builder 与 pipeline，应用团队只需关注源码与业务配置。建议为不同环境（开发、测试、生产）维护独立的 builder 版本与镜像 tag 策略，避免测试环境的不稳定构建影响生产发布。

## 14. 生产检查清单

- [ ] 已选择或创建符合企业基线的 builder；
- [ ] builder/buildpack/stack 使用固定版本，避免 latest 漂移；
- [ ] 自定义 builder 已推送至 ACR/Harbor 内网仓库；
- [ ] 镜像分层结构已使用 `pack inspect-image` 或 dive 验证；
- [ ] 构建缓存已配置并命中（可通过 `pack build --verbose` 观察 analyze/restore 阶段）；
- [ ] SBOM 已导出并归档，便于漏洞响应；
- [ ] CI/CD 中已集成镜像扫描（Trivy/ACR 扫描）；
- [ ] 多平台构建目标（amd64/arm64）已与 ACK 节点架构对齐。

## 15. 相关文档

- [[容器运行时/04-image-build/01-buildkit-production-guide.md|BuildKit 生产指南]]
- [[容器运行时/04-image-build/03-kaniko-ko-build-guide.md|Kaniko 与 ko 构建指南]]
- [[容器运行时/02-image-management/01-harbor-enterprise-image-registry.md|Harbor 企业镜像仓库]]
- [[发布变更/01-gitops/05-tekton-cloud-native-cicd.md|Tekton 云原生 CI/CD]]
- [[发布变更/01-gitops/08-cicd-pipeline-patterns.md|CI/CD 流水线模式]]


<!-- risk-assessed -->
