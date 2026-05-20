---
title: SOPS
description: '| **适用场景** | 秘密信息加密管理 |'
category: cncf-landscape
tags:
- k8s
- cncf
- cloud-native
- ecosystem
- helm
- argocd
- flux
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 架构师
- 技术决策者
- SRE
estimated_read_time: 5min
intent_queries:
- SOPS 是什么
- 如何 SOPS
- Kubernetes 34 cncf landscape 最佳实践
trigger_keywords:
- SOPS
- cncf
- landscape
---


# SOPS

> **成熟度**: Sandbox | **加入时间**: 2023-04 | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官方网站** | https://getsops.io |
| **GitHub** | https://github.com/getsops/sops |
| **许可证** | MPL-2.0 |
| **开发语言** | Go |
| **CNCF 分类** | Security & Compliance |
| **适用场景** | 秘密信息加密管理 |

---

## 项目概述

SOPS (Secrets OPerationS) 是一个加密文件编辑器，支持 YAML、JSON、ENV 和 BINARY 格式。它使用 AWS KMS、GCP KMS、Azure Key Vault、HashiCorp Vault 或 PGP 密钥对文件中的值进行加密，而保持键名明文，便于版本控制和代码审查。SOPS 是 GitOps 工作流中管理敏感信息的核心工具。

---

## 核心特性

- **多格式支持**: YAML、JSON、ENV、INI、BINARY
- **键值分离**: 只加密值，保留键名可读
- **多 KMS 后端**: AWS KMS、GCP KMS、Azure、Vault、age、PGP
- **多密钥加密**: 同时使用多个密钥加密
- **审计日志**: 加密/解密操作审计
- **GitOps 友好**: 加密文件可安全提交到 Git
- **Flux/ArgoCD 集成**: 原生支持 GitOps 工作流

---

## 架构设计

```
┌─────────────────────────────────────────────────────────────────┐
│                       SOPS Architecture                          │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  ┌─────────────────────────────────────────────────────────┐   │
│  │                    Developer Workflow                      │   │
│  │                                                           │   │
│  │  secrets.yaml (plaintext) ──► sops encrypt ──► secrets.enc.yaml│
│  │                                                           │   │
│  │  secrets.enc.yaml ──► sops decrypt ──► secrets.yaml       │   │
│  │                                                           │   │
│  │  sops secrets.enc.yaml ──► (in-place edit) ──► save       │   │
│  └──────────────────────────────┬──────────────────────────┘   │
│                                 │                               │
│  ┌──────────────────────────────▼──────────────────────────┐   │
│  │                      SOPS Engine                          │   │
│  │  ┌─────────────────────────────────────────────────────┐ │   │
│  │  │              Key Management Backends                 │ │   │
│  │  │  ┌─────────┐ ┌─────────┐ ┌─────────┐ ┌─────────┐  │ │   │
│  │  │  │ AWS KMS │ │ GCP KMS │ │ Azure   │ │  age    │  │ │   │
│  │  │  │         │ │         │ │KeyVault │ │         │  │ │   │
│  │  │  └─────────┘ └─────────┘ └─────────┘ └─────────┘  │ │   │
│  │  │  ┌─────────┐ ┌─────────┐                           │ │   │
│  │  │  │  PGP    │ │ Vault   │                           │ │   │
│  │  │  │         │ │ Transit │                           │ │   │
│  │  │  └─────────┘ └─────────┘                           │ │   │
│  │  └─────────────────────────────────────────────────────┘ │   │
│  └──────────────────────────────────────────────────────────┘  │
│                                 │                               │
│  ┌──────────────────────────────▼──────────────────────────┐   │
│  │                    Git Repository                         │   │
│  │  ┌─────────────────────────────────────────────────────┐ │   │
│  │  │  secrets.enc.yaml (encrypted values, plain keys)    │ │   │
│  │  │  .sops.yaml (SOPS configuration)                    │ │   │
│  │  └─────────────────────────────────────────────────────┘ │   │
│  └──────────────────────────────┬──────────────────────────┘   │
│                                 │                               │
│  ┌──────────────────────────────▼──────────────────────────┐   │
│  │                  GitOps Deployment                        │   │
│  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────────┐  │   │
│  │  │ Flux SOPS   │  │  ArgoCD     │  │   Helm Secrets  │  │   │
│  │  │ Controller  │  │  SOPS Plugin│  │   Plugin        │  │   │
│  │  └─────────────┘  └─────────────┘  └─────────────────┘  │   │
│  └──────────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────────┘
```

---

## 快速开始

### 安装

```bash
# macOS
brew install sops

# Linux
curl -LO https://github.com/getsops/sops/releases/latest/download/sops-v3.8.1.linux.amd64
chmod +x sops-v3.8.1.linux.amd64
sudo mv sops-v3.8.1.linux.amd64 /usr/local/bin/sops

# 安装 age (推荐的加密后端)
brew install age  # macOS
# 或 apt install age  # Debian/Ubuntu
```

### 生成密钥 (age)

```bash
# 生成 age 密钥对
age-keygen -o ~/.config/sops/age/keys.txt

# 查看公钥
age-keygen -y ~/.config/sops/age/keys.txt
# age1xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
```

---

## 基本使用

### 加密文件

```bash
# 使用 age 加密
sops --encrypt --age age1xxxxxxxxxx secrets.yaml > secrets.enc.yaml

# 使用 AWS KMS 加密
sops --encrypt --kms arn:aws:kms:us-east-1:123:key/xxx secrets.yaml > secrets.enc.yaml

# 使用 GCP KMS
sops --encrypt --gcp-kms projects/my-project/locations/global/keyRings/my-ring/cryptoKeys/my-key secrets.yaml > secrets.enc.yaml

# 原地加密
sops --encrypt --in-place --age age1xxxxxxxxxx secrets.yaml
```

### 解密文件

```bash
# 解密到标准输出
sops --decrypt secrets.enc.yaml

# 解密到文件
sops --decrypt secrets.enc.yaml > secrets.yaml

# 原地解密
sops --decrypt --in-place secrets.enc.yaml

# 提取特定值
sops --decrypt --extract '["database"]["password"]' secrets.enc.yaml
```

### 编辑加密文件

```bash
# 使用默认编辑器打开 (自动解密/重新加密)
sops secrets.enc.yaml

# 指定编辑器
EDITOR=vim sops secrets.enc.yaml
```

---

## 配置文件 (.sops.yaml)

### 基本配置

```yaml
# .sops.yaml (放在仓库根目录)
creation_rules:
  # 默认规则
  - age: age1xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx

  # 按路径匹配不同密钥
  - path_regex: production/.*\.yaml$
    age: age1prod_xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
    
  - path_regex: staging/.*\.yaml$
    age: age1staging_xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx

  # 多密钥加密 (任一密钥都可解密)
  - path_regex: shared/.*\.yaml$
    age: >-
      age1team_a_xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx,
      age1team_b_xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
```

### 多后端配置

```yaml
creation_rules:
  - path_regex: aws/.*\.yaml$
    kms: arn:aws:kms:us-east-1:123456789:key/xxx-yyy-zzz
    
  - path_regex: gcp/.*\.yaml$
    gcp_kms: projects/my-project/locations/global/keyRings/sops/cryptoKeys/sops-key
    
  - path_regex: azure/.*\.yaml$
    azure_keyvault: https://my-vault.vault.azure.net/keys/sops-key/xxx

  - path_regex: .*\.yaml$
    age: age1xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
```

### 选择性加密

```yaml
creation_rules:
  - path_regex: .*\.yaml$
    age: age1xxxxxxxxxx
    # 只加密特定键
    encrypted_regex: "^(password|secret|token|key|apiKey)$"
    
  # 或排除特定键
  - path_regex: config/.*\.yaml$
    age: age1xxxxxxxxxx
    encrypted_suffix: _encrypted
```

---

## 加密文件示例

### 加密前 (明文)

```yaml
# secrets.yaml
database:
  host: db.example.com
  port: 5432
  username: admin
  password: super-secret-password
  
api:
  key: sk-1234567890abcdef
  endpoint: https://api.example.com
```

### 加密后

```yaml
# secrets.enc.yaml
database:
  host: ENC[AES256_GCM,data:xxx,iv:yyy,tag:zzz,type:str]
  port: ENC[AES256_GCM,data:xxx,iv:yyy,tag:zzz,type:int]
  username: ENC[AES256_GCM,data:xxx,iv:yyy,tag:zzz,type:str]
  password: ENC[AES256_GCM,data:xxx,iv:yyy,tag:zzz,type:str]
api:
  key: ENC[AES256_GCM,data:xxx,iv:yyy,tag:zzz,type:str]
  endpoint: ENC[AES256_GCM,data:xxx,iv:yyy,tag:zzz,type:str]
sops:
  age:
    - recipient: age1xxxxxxxxxx
      enc: |
        -----BEGIN AGE ENCRYPTED FILE-----
        ...
        -----END AGE ENCRYPTED FILE-----
  lastmodified: "2026-03-01T00:00:00Z"
  version: 3.8.1
```

---

## GitOps 集成

### Flux CD 集成

```yaml
# 创建 SOPS Secret
kubectl create secret generic sops-age \
  --namespace flux-system \
  --from-file=age.agekey=/path/to/keys.txt

# Flux Kustomization 配置
apiVersion: kustomize.toolkit.fluxcd.io/v1
kind: Kustomization
metadata:
  name: my-app
  namespace: flux-system
spec:
  sourceRef:
    kind: GitRepository
    name: my-app
  decryption:
    provider: sops
    secretRef:
      name: sops-age
  path: ./deploy
  prune: true
```

### ArgoCD 集成

```bash
# 安装 KSOPS 插件
# 在 ArgoCD repo-server 中添加 KSOPS

# ksops-generator.yaml
apiVersion: viaduct.ai/v1
kind: ksops
metadata:
  name: secrets-generator
files:
  - secrets.enc.yaml
```

### Helm Secrets 插件

```bash
# 安装 helm-secrets 插件
helm plugin install https://github.com/jkroepke/helm-secrets

# 使用加密 values 文件
helm secrets install my-app ./chart \
  -f secrets.enc.yaml

# 编辑加密 values
helm secrets edit secrets.enc.yaml
```

---

## AWS KMS 配置

```bash
# 使用 AWS KMS 加密
export SOPS_KMS_ARN="arn:aws:kms:us-east-1:123456789:key/xxx-yyy-zzz"
sops --encrypt secrets.yaml > secrets.enc.yaml

# 多区域 KMS
sops --encrypt \
  --kms "arn:aws:kms:us-east-1:123:key/xxx,arn:aws:kms:eu-west-1:123:key/yyy" \
  secrets.yaml
```

---

## HashiCorp Vault Transit

```bash
# 配置 Vault 后端
export VAULT_ADDR=https://vault.example.com
export VAULT_TOKEN=hvs.xxx

sops --encrypt \
  --hc-vault-transit https://vault.example.com/v1/transit/keys/sops-key \
  secrets.yaml
```

---

## 最佳实践

1. **age 优先**: 新项目推荐使用 age 而非 PGP
2. **密钥轮换**: 定期轮换加密密钥
3. **.sops.yaml**: 始终配置 .sops.yaml 简化使用
4. **Git 集成**: 使用 git-diff 配置显示加密差异
5. **CI/CD**: 在 pipeline 中使用 KMS 而非本地密钥
6. **最小权限**: 按环境和团队划分加密密钥

---

## 参考资源

- [官方文档](https://getsops.io)
- [GitHub Repo](https://github.com/getsops/sops)
- [age 加密](https://github.com/FiloSottile/age)
- [Flux SOPS](https://fluxcd.io/flux/guides/mozilla-sops/)
- [Helm Secrets](https://github.com/jkroepke/helm-secrets)

---

**维护者**: Kudig Team | **许可证**: MIT
