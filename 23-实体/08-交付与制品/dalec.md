---
title: Dalec
description: '## 概述'
summary: 'Dalec 是一个声明式的 Linux 系统包构建工具，通过简洁的 YAML 规范定义如何构建 RPM、DEB 等 Linux 包，而无需手动编写 spec 文件或 debian/rules。它基于 BuildKit 构建，能够交叉编译多架构包，支持自动依赖管理、补丁应用、签名等功能。Dalec 特别适合需要将软件打包为多个发行版格式的场景。'
category: entities
tags:
- k8s
- cncf
- ci-cd
- dalec
- crd
- operator
tier: core
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Dalec 是什么
- 如何 Dalec
trigger_keywords:
- Dalec
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# Dalec

> **CNCF 状态**: Sandbox | **类别**: CI/CD | **主要语言**: Go

## 概述

Dalec（Declarative Artifact Lifecycle Engine for Containers）是由 Microsoft（Azure Linux/SBL 团队）开发的开源声明式 Linux 包构建工具，2024 年进入 CNCF Sandbox。它通过简洁的 YAML 规范文件定义如何从源码构建 RPM（RHEL/Fedora/CentOS）、DEB（Debian/Ubuntu）等 Linux 发行版包，而无需手动编写复杂的 RPM spec 文件或 `debian/rules` 脚本。

Dalec 基于 **BuildKit** 构建引擎，利用 Dockerfile 的前端扩展机制（`#syntax` 指令）实现声明式构建。它支持交叉编译多架构包（x86_64、aarch64）、自动依赖管理、补丁应用、GPG 签名等企业级打包需求。Dalec 特别适合需要将同一软件同时打包为多个发行版格式的场景（如开源软件的 CI/CD 多发行版发布）。

## Key Features

- **声明式 YAML 规范**：用 `dalec.spec.yml` 描述包元数据、源码、依赖、构建步骤和测试
- **多格式输出**：一次定义生成 RPM 和 DEB 包，无需维护两套构建脚本
- **BuildKit 引擎**：利用 BuildKit 的并行构建、缓存和远程执行能力
- **交叉编译**：支持在 x86 主机上构建 ARM 包，多架构包一次生成
- **自动依赖管理**：声明 build-time 和 runtime 依赖，Dalec 自动安装
- **签名与验证**：支持 GPG 签名和包完整性验证

## Architecture

Dalec 作为 BuildKit 的自定义前端工作。开发者创建 `dalec.spec.yml` 文件，然后通过 `docker build -f dalec.spec.yml .` 命令触发构建。Dalec 前端解析 YAML 规范，将其翻译为一系列 BuildKit 步骤（拉取基础镜像、安装构建依赖、编译源码、生成包文件）。每个目标格式（RPM/DEB）有对应的构建模板，最终输出构建好的包文件。

## K8s 集成

Dalec 本身不直接依赖 Kubernetes，但它非常适合在 Kubernetes CI/CD 管道中运行。通过 Tekton、Argo Workflows 或 GitHub Actions 在 K8s Pod 中执行 Dalec 构建，利用 BuildKit 的分布式缓存能力加速构建。也支持通过 OCI 镜像形式分发构建好的包，与 Kubernetes 节点镜像更新流程（如 bootc）配合。

## 生产部署要点

- **源码固定**：使用 Git commit hash 而非分支名固定源码版本
- **依赖最小化**：只声明实际需要的运行时依赖
- **测试覆盖**：为每个包编写基本测试确保安装和运行正常
- **CI/CD 集成**：将 Dalec 构建集成到流水线，自动构建多发行版包
- **版本策略**：使用 revision 字段区分同一上游版本的不同打包版本

## 生产场景

1. **多发行版软件发布**：开源软件 CI 中一次性生成 RPM 和 DEB 包
2. **内部工具打包**：企业内部工具打包为系统包分发到生产服务器
3. **K8s 节点 OS 包管理**：为 bootc/Talos 等不可变 OS 构建自定义包
4. **安全补丁分发**：快速构建和分发安全补丁包到多发行版环境

## 安装与配置

```bash
# Dalec 作为 BuildKit 前端，无需单独安装
# 确保 Docker BuildKit 已启用
export DOCKER_BUILDKIT=1

# 创建 dalec.spec.yml
cat > dalec.spec.yml <<EOF
syntax: ghcr.io/azure/dalec/frontend:latest
---
name: myapp
version: 1.0.0
revision: 1
description: My application
license: Apache-2.0
sources:
  -:
    ref: https://github.com/myorg/myapp/archive/v1.0.0.tar.gz
build:
  steps:
    - command: make
      dest_dir: /usr/bin
dependencies:
  runtime:
    - glibc
tests:
  - name: binary-exists
    files:
      - /usr/bin/myapp
EOF

# 构建 RPM 包
DOCKER_BUILDKIT=1 docker build -f dalec.spec.yml --target rpm -t myapp-rpm .

# 构建 DEB 包
DOCKER_BUILDKIT=1 docker build -f dalec.spec.yml --target deb -t myapp-deb .

# 交叉编译 ARM64 包
DOCKER_BUILDKIT=1 docker buildx build --platform linux/arm64 \
  -f dalec.spec.yml --target rpm -t myapp-rpm-arm64 .

# 提取构建产物
docker create --name tmp myapp-rpm
docker cp tmp:/output/myapp-1.0.0-1.x86_64.rpm ./
docker rm tmp
```

```yaml
# 完整 dalec.spec.yml 示例（含补丁、签名、多目标）
syntax: ghcr.io/azure/dalec/frontend:latest
---
name: nginx-custom
version: 1.25.3
revision: 2
description: Custom Nginx with additional modules
license: BSD-2-Clause
sources:
  -:
    ref: https://nginx.org/download/nginx-1.25.3.tar.gz
    sha256: abc123...
patches:
  - name: fix-cve-2024-xxxx.patch
    source: ./patches/
build:
  steps:
    - command: |
        ./configure --prefix=/usr/share/nginx --with-http_ssl_module
        make -j$(nproc)
      dest_dir: /usr/share/nginx
dependencies:
  build:
    - gcc
    - make
    - openssl-devel
    - pcre-devel
  runtime:
    - openssl
    - pcre
    - zlib
tests:
  - name: binary-runs
    exec: /usr/share/nginx/sbin/nginx -v
  - name: config-valid
    exec: /usr/share/nginx/sbin/nginx -t
```

## 运维操作

```bash
# 🟢 查看构建缓存状态
docker buildx du --filter type=exec

# 🟢 列出已构建的包镜像
docker images | grep -E '(rpm|deb)$'

# 🟡 清理 Dalec 构建缓存（释放磁盘空间）
docker buildx prune --filter type=exec

# 🟡 更新 Dalec 前端版本
# 修改 dalec.spec.yml 第一行 syntax 指向新版本
sed -i 's|ghcr.io/azure/dalec/frontend:.*|ghcr.io/azure/dalec/frontend:v0.6.0|' dalec.spec.yml

# 🟢 验证包完整性
rpm -K myapp-1.0.0-1.x86_64.rpm   # RPM GPG 验证
dpkg --info myapp_1.0.0-1_amd64.deb  # DEB 信息检查

# 🔴 批量删除旧版本包镜像（谨慎操作）
docker rmi $(docker images --filter 'reference=myapp-*' -q)
```

## 故障排查

| 症状 | 可能原因 | 排查命令 | 修复方案 |
|------|----------|----------|----------|
| 构建失败：source download error | 源码 URL 不可达或 SHA 不匹配 | `curl -sI <source-url>` | 更新 ref URL 或修正 sha256 |
| 依赖安装失败 | 基础镜像缺少包仓库配置 | `docker build --progress=plain` 查看详细日志 | 添加正确的 repo 配置 |
| 交叉编译失败 | buildx 未配置多平台 | `docker buildx ls` | `docker buildx create --use` |
| 包签名验证失败 | GPG key 未导入或已过期 | `rpm --import pubkey.asc` | 重新导入有效 GPG 公钥 |
| 构建缓存无效 | 源码变更但缓存未失效 | `docker buildx du` | 添加 `--no-cache` 或修改 spec |

```
排查流程：
├── 构建失败
│   ├── 检查 syntax 行指向的 frontend 镜像是否可拉取
│   ├── 检查 sources 中的 URL 和 SHA256
│   ├── 检查 build dependencies 是否在目标发行版仓库中存在
│   └── 使用 --progress=plain 查看完整构建日志
├── 包安装失败
│   ├── rpm -qpR package.rpm 检查依赖列表
│   ├── dpkg-deb -I package.deb 检查依赖
│   └── 确认目标系统版本与包兼容
└── 交叉编译问题
    ├── docker buildx inspect 确认 builder 配置
    ├── 确认 QEMU 用户态模拟已安装
    └── 检查目标架构的基础镜像是否存在
```

## 生产案例

### 案例 1：多发行版 CI/CD 发布管道

- **场景**：开源项目需要同时为 RHEL 9、Ubuntu 22.04、Fedora 39 发布安装包，之前维护 3 套构建脚本
- **排查**：构建脚本不一致导致各发行版行为差异，用户频繁报告特定发行版安装失败
- **方案**：迁移到 Dalec 统一 YAML 规范，CI 中并行构建 3 个目标格式，GitHub Actions 矩阵策略
- **效果**：构建脚本从 3 套减为 1 个 YAML，发布周期从 2 天缩短到 30 分钟，跨发行版一致性问题归零

### 案例 2：Azure Linux 节点自定义包构建

- **场景**：企业使用 Azure Linux (CBL-Mariner) 作为 AKS 节点 OS，需要构建内部安全 Agent 包
- **排查**：内部 Agent 之前以二进制分发，缺少包管理（无版本追踪、无依赖管理、无自动更新）
- **方案**：使用 Dalec 将 Agent 打包为 RPM，集成到节点镜像构建流程（bootc/ostree），通过包管理器自动更新
- **效果**：Agent 部署从手动 scp 升级为包管理器自动更新，版本回滚时间从 30 分钟降至 2 分钟

## 对比

| 特性 | Dalec | fpm | rpmbuild | dpkg-buildpackage | 适用场景 |
|------|-------|-----|----------|-------------------|----------|
| 声明式 YAML | ✅ | ✅ Ruby DSL | ❌ spec 文件 | ❌ debian/rules | 多格式统一构建 |
| 多格式输出 | ✅ RPM+DEB | ✅ 多格式 | ❌ RPM only | ❌ DEB only | 跨发行版发布 |
| BuildKit 缓存 | ✅ | ❌ | ❌ | ❌ | CI/CD 加速 |
| 交叉编译 | ✅ | ⚠️ | ⚠️ | ⚠️ | 多架构支持 |
| 学习曲线 | 低 | 中 | 高 | 高 | 团队快速上手 |
| 生产成熟度 | 中（新项目） | 高 | 高 | 高 | 企业级稳定性 |

## 参考链接

- [[22-概念/01-核心架构/declarative-api.md|declarative-api]]
- [[22-概念/09-平台与发布/ci-cd-pipeline-patterns.md|ci-cd-pipeline-patterns]]

## Related

- [[pipecd]] — PipeCD
- [[hami]] — HAMI
- [[open-policy-containers]] — [[23-实体/06-安全/open-policy-containers.md|Open Policy Containers (OPCR)]]
- [[werf]] — werf
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- dalec
- [[23-实体/15-参考与索引/cncf-runtime.md|CNCF 容器运行时与工具链项目全景]] — Cross-reference
- [[21-生态参考/03-领域索引/etcd-index.md|etcd 知识图谱索引]]
- [[21-生态参考/03-领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
