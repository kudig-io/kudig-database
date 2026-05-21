---
title: ORAS
description: 'description: ''| **适用场景** | OCI 工件管理 |'''
category: general
tags:
- cncf
- ecosystem
- helm
- wasm
- rag
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- ORAS 是什么
- 如何 ORAS
- Kubernetes 19 landscape references 最佳实践
trigger_keywords:
- ORAS
- landscape
- references
prerequisites:
- kubectl-basics
- cncf-ecosystem
- helm-basics
---

title: ORAS
description: '| **适用场景** | OCI 工件管理 |'
category: cncf-landscape
tags:
- k8s
- cncf
- cloud-native
- ecosystem
- helm
- wasm
- rag
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 架构师
- 技术决策者
- SRE
estimated_read_time: 5min
intent_queries:
- ORAS 是什么
- 如何 ORAS
- Kubernetes 34 cncf landscape 最佳实践
trigger_keywords:
- ORAS
- cncf
- landscape
authors:
- name: KUDIG Team
  role: contributor
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
---
# ORAS

> **成熟度**: Sandbox | **加入时间**: 2021-11 | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官方网站** | https://oras.land |
| **GitHub** | https://github.com/oras-project/oras |
| **许可证** | Apache-2.0 |
| **开发语言** | Go |
| **CNCF 分类** | App Definition & Build |
| **适用场景** | OCI 工件管理 |

---

## 项目概述

ORAS (OCI Registry As Storage) 是一个用于将 OCI 工件推送到和拉取自 OCI 兼容仓库的工具和库。它允许使用容器镜像仓库存储任意类型的工件，如 Helm Chart、WASM 模块、策略文件、签名等，实现 "anything as OCI artifacts" 的理念。

---

## 核心特性

- **任意工件**: 将任意文件存储为 OCI 工件
- **OCI 兼容**: 支持所有 OCI 兼容仓库
- **CLI 和库**: 提供 CLI 工具和 Go/Python 库
- **Manifest 操作**: 查看和管理 OCI manifest
- **多平台**: 支持 Linux、macOS、Windows
- **引用支持**: OCI Reference Types 关联工件

---

## 快速开始

### 安装

```bash
# macOS
brew install oras

# Linux
curl -LO https://github.com/oras-project/oras/releases/latest/download/oras_1.1.0_linux_amd64.tar.gz
tar -xzf oras_1.1.0_linux_amd64.tar.gz
sudo mv oras /usr/local/bin/
```

### 基本使用

```bash
# 推送文件到 OCI 仓库
oras push registry.example.com/myartifact:v1 \
  ./config.yaml:application/vnd.example.config \
  ./data.json:application/json

# 拉取工件
oras pull registry.example.com/myartifact:v1

# 查看 manifest
oras manifest fetch registry.example.com/myartifact:v1

# 列出标签
oras repo tags registry.example.com/myartifact

# 复制工件
oras copy registry-a.example.com/artifact:v1 registry-b.example.com/artifact:v1

# 附加引用 (如签名、SBOM)
oras attach registry.example.com/myimage:v1 \
  --artifact-type application/vnd.example.signature \
  ./signature.sig
```

---

## 使用场景

| 场景 | 说明 |
|:---|:---|
| **Helm Charts** | 存储 Helm Chart 为 OCI 工件 |
| **WASM 模块** | 分发 WebAssembly 策略模块 |
| **SBOM** | 附加软件物料清单 |
| **签名** | 工件签名和验证 |
| **配置文件** | 分发应用配置 |

---

## Go 库使用

```go
import "oras.land/oras-go/v2"

// 推送工件
repo, _ := remote.NewRepository("registry.example.com/myartifact")
_, err := oras.Push(ctx, repo, artifactType, bytes.NewReader(content))

// 拉取工件
_, err := oras.Pull(ctx, repo, tag)
```

---

## 最佳实践

1. **Media Type**: 为工件定义明确的 media type
2. **标签管理**: 使用语义化版本标签
3. **引用关联**: 使用 OCI Reference Types 关联签名、SBOM
4. **仓库兼容**: 确认目标仓库支持 OCI 工件

---

## 参考资源

- [官方文档](https://oras.land/docs/)
- [GitHub Repo](https://github.com/oras-project/oras)
- [Go 库](https://github.com/oras-project/oras-go)
- [OCI Artifacts Spec](https://github.com/opencontainers/image-spec)

---

**维护者**: Kudig Team | **许可证**: MIT

## Related

- [[domain-17-system-foundation/topic-cheat-sheet/go.md|go]]
- [[domain-17-system-foundation/topic-cheat-sheet/helm.md|helm]]
- [[domain-17-system-foundation/topic-cheat-sheet/linux.md|linux]]
- [[domain-17-system-foundation/topic-cheat-sheet/k8s.md|k8s]]
- [[domain-17-system-foundation/topic-cheat-sheet/git.md|git]]
- [[domain-19-landscape-references/topic-index/etcd-index|etcd 知识图谱索引]]
- [[domain-19-landscape-references/topic-index/gitops-cicd-index|GitOps / CI-CD 全局索引]]
