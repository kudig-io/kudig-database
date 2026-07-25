---
title: TUF
description: '## 概述'
summary: '该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[22-概念/01-核心架构/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]'
category: entities
tags:
- k8s
- cncf
- supply-chain
- tuf
- crd
- operator
- rag
tier: peripheral
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- TUF 是什么
- 如何 TUF
trigger_keywords:
- TUF
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# TUF

> **CNCF 状态**: Graduated | **类别**: Supply Chain | **主要语言**: Python, Go, Rust

## 概述

The Update Framework（TUF）是一个 CNCF 毕业项目，最初由 NYU 安全研究团队开发，灵感来源于 Tor 项目中的软件更新安全研究。TUF 是一个用于保护软件更新系统安全的框架，解决镜像仓库被入侵后的降级攻击、恶意软件分发等安全威胁。它是 Notary 项目的底层规范基础，也是容器镜像签名验证体系的核心组件。TUF 规范已被 Datadog、Docker、Cloudflare、Python PyPI、RubyGems 等组织采用。

## Key Features（核心能力）

- **防降级攻击**：通过版本号和时间戳机制防止攻击者分发旧版本软件
- **防无限数据攻击**：限制元数据大小，防止客户端资源耗尽
- **防快速攻击**：使用时间戳和自动化验证防止恶意镜像快速分发
- **防混合攻击**：组合多种防御机制，覆盖已知攻击向量
- **密钥分离架构**：将根密钥、目标密钥、快照密钥、时间戳密钥分离，降低密钥泄露风险
- **委托机制**：支持多层级角色委派，实现细粒度仓库管理

## 架构与工作原理

TUF 采用分层元数据架构：Root Role 管理顶级密钥；Targets Role 定义目标文件的哈希和大小；Snapshot Role 冻结各角色元数据版本；Timestamp Role 提供最新的仓库快照引用。所有元数据通过密钥签名链式验证。客户端更新流程：获取并验证 Root 元数据 -> Timestamp 元数据 -> Snapshot 元数据 -> Targets 元数据，逐层确认文件完整性和新鲜度。

## K8s 集成

TUF 在 Kubernetes 生态中的主要实现是 Notary / Notation，用于容器镜像签名和验证。K8s 通过 Admission Controller（如 Connaisseur、Sigstore Policy Controller）在 Pod 创建时验证镜像 TUF 签名，确保仅部署经过签名的可信镜像。Containerd 和 CRI-O 均支持基于 TUF/Notary 的镜像签名验证策略。

## 生产用例

- **容器镜像签名**：在 CI/CD 流水线中对构建产物签名，部署时验证
- **软件供应链安全**：防止镜像仓库被入侵后分发恶意镜像
- **IoT 设备更新**：保护物联网设备的 OTA 固件更新安全
- **包管理器安全**：保护 PyPI、NPM 等包管理器的软件分发安全

## 安装与配置

```bash
# 🟢 安装 Notation CLI（基于 TUF 规范）
brew install notation
# 或 Linux
curl -Lo notation.tar.gz https://github.com/notaryproject/notation/releases/download/v1.1.0/notation_1.1.0_linux_amd64.tar.gz
tar -xzf notation.tar.gz && mv notation /usr/local/bin/

# 🟢 安装 Python TUF 参考实现
pip install tuf

# 🟢 验证 Notation 安装
notation version

# 🟢 生成签名密钥
notation cert generate-test --default "wabbit-networks.io"

# 🟢 签名镜像
notation sign registry.example.com/myapp:v1

# 🟢 验证镜像签名
notation verify registry.example.com/myapp:v1

# 🟢 查看信任策略
notation policy show
```

### TUF 元数据结构

```
仓库元数据层次:
┌─────────────────────────────────────────────────────┐
│  root.json (根角色 - 管理所有密钥)              │
│  ├── targets.json (目标角色 - 定义文件哈希)       │
│  │   └── delegated roles (委托角色)             │
│  ├── snapshot.json (快照角色 - 冻结版本)         │
│  └── timestamp.json (时间戳角色 - 新鲜度保证)   │
└─────────────────────────────────────────────────────┘

客户端验证流程:
1. 获取并验证 root.json (密钥信任链)
2. 获取并验证 timestamp.json (新鲜度)
3. 获取并验证 snapshot.json (一致性)
4. 获取并验证 targets.json (文件完整性)
5. 下载并验证目标文件
```

### K8s Admission Controller 集成

```yaml
# 使用 Connaisseur 验证镜像签名
apiVersion: apps/v1
kind: Deployment
metadata:
  name: connaisseur
  namespace: connaisseur
spec:
  template:
    spec:
      containers:
      - name: connaisseur
        image: connaisseur:latest
        env:
        - name: NOTATION_TRUST_POLICY
          value: |
            {
              "version": "1.0",
              "trustPolicies": [{
                "name": "default",
                "registryScopes": ["*"],
                "signatureVerification": {"level": "strict"},
                "trustStores": ["ca:my-ca"]
              }]
            }
---
# 或使用 Sigstore Policy Controller
apiVersion: policy.sigstore.dev/v1beta1
kind: ClusterImagePolicy
metadata:
  name: require-signed-images
spec:
  images:
  - glob: "registry.example.com/**"
  authorities:
  - keyless:
      url: https://fulcio.sigstore.dev
      identities:
      - issuer: https://token.actions.githubusercontent.com
```

## 运维操作

### 常用命令

```bash
# 🟢 查看信任策略
notation policy show

# 🟢 查看证书存储
notation cert ls

# 🟢 添加信任证书
notation cert add --type ca --name my-ca /path/to/ca.crt

# 🟢 签名镜像
notation sign registry.example.com/myapp:v1 \
  --signature-format cose

# 🟢 验证镜像
notation verify registry.example.com/myapp:v1

# 🟢 查看镜像签名信息
notation manifest registry.example.com/myapp:v1

# 🟡 更新信任策略
notation policy import /path/to/trustpolicy.json

# 🟢 检查 TUF 元数据新鲜度
curl -s https://tuf-repo.example.com/timestamp.json | jq .signed.expires
```

## 故障排查

| 症状 | 可能原因 | 排查命令 | 修复方案 |
|------|----------|----------|----------|
| 签名验证失败 | 证书不匹配/过期 | `notation verify <image>` | 更新信任证书 |
| 元数据过期 | TUF 仓库未更新 | 检查 timestamp.json expires | 刷新 TUF 元数据 |
| Pod 被拒绝 | Admission 策略拒绝 | `kubectl describe pod <name>` | 检查镜像签名和策略 |
| 密钥泄露 | 根密钥被盗 | 审计 TUF 仓库访问日志 | 轮换根密钥 (root rotation) |
| 降级攻击检测 | 版本号回退 | 对比 snapshot 版本 | 拒绝旧版本元数据 |

### 排查流程

```
1. notation verify <image> → 验证镜像签名
2. notation policy show → 检查信任策略
3. notation cert ls → 确认证书配置
4. kubectl describe pod → 查看 Admission 拒绝原因
5. 检查 TUF 仓库元数据新鲜度
```

## 生产案例

### 案例1: 容器镜像供应链安全
- **场景**: 镜像仓库被入侵，攻击者替换了合法镜像
- **方案**: TUF/Notation 签名 + Admission Controller 验证
- **效果**: 未签名镜像无法部署，攻击被拦截

### 案例2: CI/CD 签名流水线
- **场景**: 需要确保只有 CI 构建的镜像才能部署到生产
- **方案**: CI 流水线中 Notation 签名，部署时验证
- **效果**: 实现完整的软件供应链可追溯性

## 对比替代方案

| 维度 | TUF/Notation | Sigstore/Cosign | Docker Content Trust | 无签名 |
|------|-------------|----------------|---------------------|--------|
| 防降级攻击 | 支持 | 不支持 | 支持 | 无 |
| 密钥管理 | 分层密钥 | 无密钥 (keyless) | 分层密钥 | 无 |
| 透明度日志 | 无 | Rekor | 无 | 无 |
| 仓库级保护 | 支持 | 个体签名 | 支持 | 无 |
| 复杂度 | 中 | 低 | 高 | 无 |
| CNCF | Graduated | Graduated | 非 CNCF | N/A |

## 检查清单

- [ ] 信任策略已配置并测试
- [ ] 签名证书已添加到信任存储
- [ ] Admission Controller 已部署并配置
- [ ] CI/CD 流水线集成签名步骤
- [ ] TUF 元数据定期刷新
- [ ] 根密钥安全存储 (HSM/离线)
- [ ] 监控签名验证失败事件
- [ ] 制定密钥轮换和应急响应计划

## Related

- [[k8up]] — K8up
- [[parsec]] — Parsec
- [[opencost]] — OpenCost
- [[slimfaas]] — SlimFaas
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- tuf
- [[23-实体/cncf-security.md|[[CNCF 安全与合规项目全景|CNCF 安全与合规项目全景]]]] — Cross-reference
- [[21-生态参考/03-领域索引/etcd-index.md|etcd 知识图谱索引]]


<!-- risk-assessed -->
