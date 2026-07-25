---
title: ORAS (OCI Registry As Storage)
description: 'summary: "ORAS (OCI Registry As Storage) 是一个用于将 OCI 工件推送到和拉取自 OCI 兼容仓库的工具和库。它允许使用容器镜像仓库存储任意类型的工件，如
  Helm Chart、WASM 模块、策略文件、签名等，实现 "anything as OCI artifacts" 的理念。"'
summary: 'summary: "ORAS (OCI Registry As Storage) 是一个用于将 OCI 工件推送到和拉取自 OCI 兼容仓库的工具和库。它允许使用容器镜像仓库存储任意类型的工件，如
  Helm Chart、WASM 模块、策略文件、签名等，实现 "anything as OCI artifacts" 的理念。"'
category: general
tags:
- k8s
- helm
- crd
- operator
- wasm
- rag
tier: supporting
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- ORAS (OCI Registry As Storage) 是什么
- 如何 ORAS (OCI Registry As Storage)
trigger_keywords:
- ORAS
- OCI
- Registry
- As
- Storage
prerequisites:
- kubectl-basics
- helm-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




---
title: "[[ORAS|ORAS]] (OCI Registry As Storage)"
category: entities
summary: "ORAS (OCI Registry As Storage) 是一个用于将 OCI 工件推送到和拉取自 OCI 兼容仓库的工具和库。它允许使用容器镜像仓库存储任意类型的工件，如 Helm Chart、WASM 模块、策略文件、签名等，实现 "anything as OCI artifacts" 的理念。"
tags: k8s, cncf, image, oras]
sources: ["docs/生态参考/sandbox/oras/oras.md", "生态参考/sandbox/oras/oras.md"]
created: 2026-05-21
updated: 2026-05-21
lifecycle: reviewed
lifecycle_changed: "2026-05-21"
tier: reference
base_confidence: 0.7
---

# ORAS (OCI Registry As Storage)

> **CNCF 状态**: Sandbox | **类别**: Image | **主要语言**: Go

## 概述

ORAS (OCI Registry As Storage) 是一个用于将 OCI 工件推送到和拉取自 OCI 兼容仓库的工具和库。它允许使用容器镜像仓库存储任意类型的工件，如 Helm Chart、WASM 模块、策略文件、签名等，实现 "anything as OCI artifacts" 的理念。

## 核心能力

- **任意工件**: 将任意文件存储为 OCI 工件
- **OCI 兼容**: 支持所有 OCI 兼容仓库
- **CLI 和库**: 提供 CLI 工具和 Go/Python 库
- **Manifest 操作**: 查看和管理 OCI manifest
- **多平台**: 支持 Linux、macOS、Windows
- **引用支持**: OCI Reference Types 关联工件

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[kubernetes-architecture-overview|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- **Media Type**: 为工件定义明确的 media type
- **标签管理**: 使用语义化版本标签
- **引用关联**: 使用 OCI Reference Types 关联签名、SBOM
- **仓库兼容**: 确认目标仓库支持 OCI 工件

## 架构定位

在 CNCF 生态中，oras 属于 **Image** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[storage-model]]

## 安装与配置

### ORAS CLI 安装

```bash
# 🟢 安装 ORAS CLI (Linux)
VERSION="1.2.0"
curl -LO "https://github.com/oras-project/oras/releases/download/v${VERSION}/oras_${VERSION}_linux_amd64.tar.gz"
tar -xzf oras_${VERSION}_linux_amd64.tar.gz -C /usr/local/bin oras
oras version

# 🟢 安装 ORAS CLI (macOS)
brew install oras

# 🟢 登录 OCI Registry
oras login registry.example.com -u admin -p <password>
# 或使用 Docker 凭据
oras login registry.example.com --registry-config ~/.docker/config.json
```

### 工件推送与拉取

```bash
# 🟢 推送 Helm Chart 为 OCI 工件
helm package my-chart/
oras push registry.example.com/charts/my-chart:1.0.0 \
  --artifact-type application/vnd.cncf.helm.chart.content.v1.tar+gzip \
  my-chart-1.0.0.tgz

# 🟢 推送 WASM 模块
oras push registry.example.com/wasm/my-module:v1.0 \
  --artifact-type application/vnd.wasm.content.layer.v1+wasm \
  my-module.wasm:application/vnd.wasm.content.layer.v1+wasm

# 🟢 推送策略文件
oras push registry.example.com/policies/opa-bundle:v2.1 \
  --artifact-type application/vnd.cncf.openpolicyagent.policy.layer.v1+rego \
  policy.rego:application/vnd.cncf.openpolicyagent.policy.layer.v1+rego

# 🟢 推送 SBOM 并关联到镜像
oras attach registry.example.com/app:v1.0 \
  --artifact-type application/spdx+json \
  sbom.spdx.json:application/spdx+json

# 🟢 拉取工件
oras pull registry.example.com/charts/my-chart:1.0.0 -o ./output/
oras pull registry.example.com/wasm/my-module:v1.0

# 🟢 查看工件 manifest
oras manifest fetch registry.example.com/charts/my-chart:1.0.0 | jq .
oras manifest fetch-config registry.example.com/charts/my-chart:1.0.0

# 🟢 列出仓库中的 tag
oras repo tags registry.example.com/charts/my-chart

# 🟢 查看工件引用关系
oras discover registry.example.com/app:v1.0 -o tree
```

### Go SDK 使用示例

```go
package main

import (
    "context"
    "fmt"
    "os"

    oras "oras.land/oras-go/v2"
    "oras.land/oras-go/v2/content/file"
    "oras.land/oras-go/v2/registry/remote"
    "oras.land/oras-go/v2/registry/remote/auth"
)

func main() {
    ctx := context.Background()

    // 创建仓库客户端
    repo, err := remote.NewRepository("registry.example.com/artifacts/my-app")
    if err != nil {
        panic(err)
    }
    repo.Client = &auth.Client{
        Credential: auth.StaticCredential("registry.example.com", auth.Credential{
            Username: os.Getenv("REGISTRY_USER"),
            Password: os.Getenv("REGISTRY_PASS"),
        }),
    }

    // 推送工件
    fs, err := file.New("./artifacts")
    if err != nil {
        panic(err)
    }
    defer fs.Close()

    manifestDesc, err := oras.Copy(ctx, fs, "artifact.tar.gz", repo, "v1.0.0",
        oras.DefaultCopyOptions)
    if err != nil {
        panic(err)
    }
    fmt.Printf("Pushed: %s\n", manifestDesc.Digest)
}
```

### CI/CD 集成示例

```yaml
# GitHub Actions 中使用 ORAS
name: Push OCI Artifacts
on:
  push:
    tags: ['v*']
jobs:
  push-artifacts:
    runs-on: ubuntu-latest
    steps:
    - uses: actions/checkout@v4
    - name: Install ORAS
      uses: oras-project/setup-oras@v1
    - name: Login to Registry
      run: oras login ${{ secrets.REGISTRY }} -u ${{ secrets.REGISTRY_USER }} -p ${{ secrets.REGISTRY_PASS }}
    - name: Push Helm Chart
      run: |
        helm package charts/my-app/
        oras push ${{ secrets.REGISTRY }}/charts/my-app:${{ github.ref_name }} \
          --artifact-type application/vnd.cncf.helm.chart.content.v1.tar+gzip \
          my-app-*.tgz
    - name: Push SBOM
      run: |
        syft packages dir:. -o spdx-json=sbom.json
        oras attach ${{ secrets.REGISTRY }}/app:${{ github.ref_name }} \
          --artifact-type application/spdx+json \
          sbom.json:application/spdx+json
```

## 运维操作

```bash
# 🟢 检查工件是否存在
oras manifest fetch registry.example.com/artifacts/my-app:v1.0 > /dev/null 2>&1 && echo "exists" || echo "not found"

# 🟢 删除工件 tag
oras manifest delete registry.example.com/artifacts/my-app:v1.0-old

# 🟢 复制工件到另一个仓库
oras cp registry.example.com/staging/app:v1.0 registry.example.com/production/app:v1.0

# 🟢 查看工件引用关系（签名、SBOM）
oras discover registry.example.com/app:v1.0 -o tree

# 🟢 批量拉取所有引用工件
oras discover registry.example.com/app:v1.0 -o json | jq -r '.manifests[].digest' | \
  xargs -I{} oras pull registry.example.com/app@{}
```

## 故障排查

| 症状 | 可能原因 | 诊断命令 | 修复方案 |
|------|----------|----------|----------|
| push 失败 401 | 认证失败/token过期 | `oras login`; 检查凭据 | 重新登录/更新 token |
| pull 失败 404 | 工件不存在/tag错误 | `oras repo tags <repo>` | 检查仓库名和 tag |
| media type 不支持 | Registry 不支持 OCI artifact | 检查 Registry 版本 | 升级 Registry/使用支持 OCI 1.1 的仓库 |
| attach 失败 | 目标 manifest 不存在 | `oras manifest fetch <target>` | 先推送目标镜像 |
| 大文件推送超时 | 网络/仓库限制 | 检查网络/仓库配置 | 分块上传/调整超时 |

## 生产案例

### 案例1：统一工件管理平台

- **场景**：团队使用多个工具（Helm、OPA、Wasm），工件分散在不同存储
- **方案**：统一使用 ORAS 将所有工件推送到同一 OCI Registry；通过 artifact-type 区分类型；通过 OCI Reference 关联签名和 SBOM
- **效果**：单一仓库管理所有工件，统一认证和访问控制，简化 CI/CD

### 案例2：供应链安全与 SBOM 关联

- **场景**：合规要求每个发布镜像必须附带 SBOM 和签名
- **方案**：CI 中用 `oras attach` 将 SBOM 和 Cosign 签名关联到镜像；部署时用 Kyverno 验证引用工件存在
- **效果**：满足 SLSA Level 3 要求，所有工件可追溯

## 对比替代方案

| 方案 | 优势 | 劣势 | 适用场景 |
|------|------|------|----------|
| ORAS | OCI标准、任意工件、生态广 | 较新、部分Registry支持不完整 | 统一工件管理 |
| Helm OCI | Helm原生支持 | 仅限Helm Chart | Helm Chart 分发 |
| ChartMuseum | 成熟Helm仓库 | 仅Helm、额外组件 | 纯 Helm 环境 |
| 云存储 (S3/GCS) | 简单、无限存储 | 无版本管理、无签名 | 简单文件存储 |
| Git Repository | 版本控制、审计 | 大文件不友好、无内容寻址 | 小型配置文件 |

## 检查清单

- [ ] ORAS CLI 已安装且版本 >= 1.2
- [ ] Registry 支持 OCI Distribution Spec 1.1
- [ ] 工件定义了明确的 artifact-type
- [ ] 使用语义化版本标签
- [ ] 重要工件已关联签名和 SBOM
- [ ] CI/CD 中集成了 ORAS 推送步骤
- [ ] Registry 认证凭据已安全存储
- [ ] 工件清理策略已配置（避免无限增长）

## Related

- [[helm]] — Helm
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[21-生态参考/03-领域索引/etcd-index.md|etcd 知识图谱索引]]
- [[21-生态参考/03-领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
