---
title: Notary Project
description: 'description: ''## 项目概述'''
category: general
tags:
- cncf
- ecosystem
- helm
- docker
- job
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 15min
intent_queries:
- Notary Project 是什么
- 如何 Notary Project
- Kubernetes 19 landscape references 最佳实践
trigger_keywords:
- Notary
- Project
- landscape
- references
prerequisites:
- kubectl-basics
- cncf-ecosystem
- helm-basics
- policy-basics
---

title: Notary Project
description: '## 项目概述'
category: cncf-landscape
tags:
- k8s
- cncf
- cloud-native
- ecosystem
- helm
- docker
- job
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 架构师
- 技术决策者
- SRE
estimated_read_time: 5min
intent_queries:
- Notary Project 是什么
- 如何 Notary Project
- Kubernetes 34 cncf landscape 最佳实践
trigger_keywords:
- Notary
- Project
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

# Notary Project

> **成熟度**: Incubating | **加入时间**: 2017-10 | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官方网站** | https://notaryproject.dev |
| **GitHub** | https://github.com/notaryproject |
| **许可证** | Apache-2.0 |
| **主要语言** | Go |
| **CNCF 分类** | Security & Supply Chain |

---

## 项目概述

Notary Project 提供容器镜像和 OCI 制品的签名、验证规范与工具。它是软件供应链安全的关键组件，通过数字签名确保制品的完整性和来源可信。

## 核心特性

- **标准规范**: OCI 兼容的签名规范
- **Notation CLI**: 签名和验证的命令行工具
- **多种签名方式**: 本地密钥、KMS、硬件令牌
- **信任策略**: 灵活的签名验证策略配置
- **插件架构**: 支持第三方 KMS 和签名服务
- **无侵入**: 签名存储在 OCI 清单，不修改原始镜像

---

## 架构设计

```
┌─────────────────────────────────────────────────────────────────┐
│                    Notary Project Architecture                   │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  ┌────────────────────────────────────────────────────────────┐ │
│  │                     Signing Flow                           │ │
│  │                                                            │ │
│  │  ┌─────────┐     ┌───────────┐     ┌─────────────────┐   │ │
│  │  │ OCI     │────▶│  Notation │────▶│   Signature     │   │ │
│  │  │ Artifact│     │   Sign    │     │   (OCI Manifest)│   │ │
│  │  └─────────┘     └─────┬─────┘     └─────────────────┘   │ │
│  │                        │                                   │ │
│  │              ┌─────────┼─────────┐                        │ │
│  │              ▼         ▼         ▼                        │ │
│  │        ┌──────────┐ ┌─────┐ ┌────────────┐               │ │
│  │        │   AWS    │ │Azure│ │  Hardware  │               │ │
│  │        │   KMS    │ │ KV  │ │  Token     │               │ │
│  │        └──────────┘ └─────┘ └────────────┘               │ │
│  └────────────────────────────────────────────────────────────┘ │
│                                                                  │
│  ┌────────────────────────────────────────────────────────────┐ │
│  │                   Verification Flow                        │ │
│  │                                                            │ │
│  │  ┌─────────────┐    ┌───────────┐    ┌────────────────┐  │ │
│  │  │  Signature  │───▶│  Notation │───▶│ Trust Policy   │  │ │
│  │  │  (Registry) │    │  Verify   │    │ Evaluation     │  │ │
│  │  └─────────────┘    └─────┬─────┘    └────────────────┘  │ │
│  │                           │                               │ │
│  │                           ▼                               │ │
│  │                    ┌─────────────┐                       │ │
│  │                    │ Trust Store │                       │ │
│  │                    │(Certificates)│                       │ │
│  │                    └─────────────┘                       │ │
│  └────────────────────────────────────────────────────────────┘ │
│                                                                  │
│  ┌────────────────────────────────────────────────────────────┐ │
│  │                   OCI Registry                             │ │
│  │  ┌─────────────────────────────────────────────────────┐  │ │
│  │  │                   Artifact                          │  │ │
│  │  │  ┌──────────────┐        ┌──────────────────────┐  │  │ │
│  │  │  │    Image     │◀──ref──│  Signature Manifest  │  │  │ │
│  │  │  │   Manifest   │        │  (application/vnd.   │  │  │ │
│  │  │  └──────────────┘        │   cncf.notary.sig)   │  │  │ │
│  │  │                          └──────────────────────┘  │  │ │
│  │  └─────────────────────────────────────────────────────┘  │ │
│  └────────────────────────────────────────────────────────────┘ │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

---

## 快速开始

### 安装 Notation CLI

```bash
# macOS
brew install notation

# Linux
curl -Lo notation.tar.gz https://github.com/notaryproject/notation/releases/download/v1.1.0/notation_1.1.0_linux_amd64.tar.gz
tar xzf notation.tar.gz
sudo mv notation /usr/local/bin/

# 验证安装
notation version
```

### 生成测试证书

```bash
# 生成自签名证书用于测试
notation cert generate-test --default "wabbit-networks.io"

# 查看证书
notation cert list
```

### 签名镜像

```bash
# 签名镜像
notation sign myregistry.io/myapp:v1.0

# 使用特定密钥签名
notation sign --key mykey myregistry.io/myapp:v1.0

# 签名并添加自定义注解
notation sign \
  --key mykey \
  --signature-manifest annotations.io.cncf.notary.x509chain.thumbprint.sha256='{"critical":{}}' \
  myregistry.io/myapp:v1.0
```

### 验证镜像

```bash
# 验证签名
notation verify myregistry.io/myapp:v1.0

# 详细输出
notation verify -v myregistry.io/myapp:v1.0

# 检查签名
notation inspect myregistry.io/myapp:v1.0
```

---

## 信任策略配置

```json
// ~/.config/notation/trustpolicy.json
{
  "version": "1.0",
  "trustPolicies": [
    {
      "name": "production-images",
      "registryScopes": [
        "myregistry.io/production/*"
      ],
      "signatureVerification": {
        "level": "strict"
      },
      "trustStores": [
        "ca:production-certs"
      ],
      "trustedIdentities": [
        "x509.subject: C=US, ST=WA, O=MyOrg, CN=production-signer"
      ]
    },
    {
      "name": "development-images",
      "registryScopes": [
        "myregistry.io/dev/*"
      ],
      "signatureVerification": {
        "level": "permissive"
      },
      "trustStores": [
        "ca:dev-certs"
      ],
      "trustedIdentities": [
        "*"
      ]
    }
  ]
}
```

### 验证级别

| 级别 | 说明 |
|------|------|
| strict | 必须有有效签名，完全匹配策略 |
| permissive | 有签名时验证，无签名时通过 |
| audit | 记录但不阻止 |
| skip | 跳过验证 |

---

## KMS 插件集成

### AWS KMS

```bash
# 安装插件
notation plugin install aws-signer

# 配置密钥
notation key add aws-key \
  --plugin aws-signer \
  --id "arn:aws:kms:us-west-2:123456789:key/abc123" \
  --default

# 签名
notation sign --key aws-key myregistry.io/myapp:v1.0
```

### Azure Key Vault

```bash
# 安装插件
notation plugin install azure-kv

# 配置密钥
notation key add azure-key \
  --plugin azure-kv \
  --id "https://myvault.vault.azure.net/certificates/mycert" \
  --default

# 签名
notation sign --key azure-key myregistry.io/myapp:v1.0
```

### HashiCorp Vault

```bash
# 安装插件
notation plugin install hashicorp-vault

# 配置密钥
notation key add vault-key \
  --plugin hashicorp-vault \
  --id "transit/keys/notation-key" \
  --default
```

---

## Kubernetes 集成

### Gatekeeper + Ratify

```yaml
# 安装 Ratify
helm repo add ratify https://ratify.dev/charts
helm install ratify ratify/ratify \
  --namespace gatekeeper-system

# 验证策略
apiVersion: config.ratify.dev/v1beta1
kind: Verifier
metadata:
  name: notation-verifier
spec:
  name: notation
  artifactTypes: application/vnd.cncf.notary.signature
  parameters:
    verificationCertStores:
      ca:
        - ratify-notation-certs
    trustPolicyDoc:
      version: "1.0"
      trustPolicies:
        - name: default
          registryScopes:
            - "*"
          signatureVerification:
            level: strict
          trustStores:
            - ca:ratify-notation-certs
          trustedIdentities:
            - "*"
```

### Kyverno 策略

```yaml
apiVersion: kyverno.io/v1
kind: ClusterPolicy
metadata:
  name: verify-image-signature
spec:
  validationFailureAction: Enforce
  background: true
  rules:
    - name: verify-notation-signature
      match:
        any:
        - resources:
            kinds:
            - Pod
      verifyImages:
      - imageReferences:
        - "myregistry.io/*"
        attestors:
        - entries:
          - keys:
              publicKeys: |-
                -----BEGIN PUBLIC KEY-----
                ...
                -----END PUBLIC KEY-----
```

---

## CI/CD 集成

```yaml
# GitHub Actions
name: Sign and Push Image
on:
  push:
    branches: [main]

jobs:
  build-sign-push:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v3
      
      - name: Build Image
        run: docker build -t myregistry.io/myapp:${{ github.sha }} .
      
      - name: Push Image
        run: docker push myregistry.io/myapp:${{ github.sha }}
      
      - name: Setup Notation
        uses: notaryproject/notation-action/setup@v1
        with:
          version: "1.1.0"
      
      - name: Sign Image
        uses: notaryproject/notation-action/sign@v1
        with:
          plugin_name: azure-kv
          plugin_url: https://github.com/Azure/notation-azure-kv/releases/download/v1.0.0/notation-azure-kv_1.0.0_linux_amd64.tar.gz
          key_id: ${{ secrets.AZURE_KEY_ID }}
          target_artifact_reference: myregistry.io/myapp:${{ github.sha }}
```

---

## 最佳实践

1. **密钥管理**: 使用 KMS 服务管理签名密钥，避免本地存储
2. **证书轮换**: 定期轮换签名证书，保持短期有效期
3. **分层策略**: 生产环境使用 strict，开发环境可用 permissive
4. **审计日志**: 记录所有签名和验证操作
5. **供应链完整**: 结合 SBOM 和漏洞扫描构建完整供应链安全

---

## 参考资源

- [官方文档](https://notaryproject.dev/docs)
- [GitHub Repo](https://github.com/notaryproject)
- [Notation CLI](https://github.com/notaryproject/notation)
- [签名规范](https://github.com/notaryproject/specifications)
- [Ratify 项目](https://ratify.dev/)

---

**维护者**: Kudig Team | **许可证**: MIT

## Related

- [[domain-17-system-foundation/topic-cheat-sheet/go.md|go]]
- [[domain-17-system-foundation/topic-cheat-sheet/helm.md|helm]]
- [[domain-17-system-foundation/topic-cheat-sheet/k8s.md|k8s]]
- [[domain-17-system-foundation/topic-cheat-sheet/git.md|git]]
- [[domain-17-system-foundation/topic-cheat-sheet/docker.md|docker]]
- [[entities/cncf-security|CNCF 安全与合规项目全景]] — Cross-reference
- [[domain-19-landscape-references/topic-index/etcd-index|etcd 知识图谱索引]]
- [[domain-19-landscape-references/topic-index/gitops-cicd-index|GitOps / CI-CD 全局索引]]
