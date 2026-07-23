---
title: SOPS (Secrets OPerationS)
description: '## 概述'
summary: 'SOPS (Secrets OPerationS) 是一个加密文件编辑器，支持 YAML、JSON、ENV 和 BINARY 格式。它使用 AWS KMS、GCP KMS、Azure Key Vault、HashiCorp Vault 或 PGP 密钥对文件中的值进行加密，而保持键名明文，便于版本控制和代码审查。'
category: entities
tags:
- k8s
- cncf
- supply-chain
- sops
- prometheus
- argocd
- flux
- crd
- operator
tier: supporting
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- SOPS (Secrets OPerationS) 是什么
- 如何 SOPS (Secrets OPerationS)
trigger_keywords:
- SOPS
- Secrets
- OPerationS
prerequisites:
- kubectl-basics
- prometheus-basics
- gitops-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# [[SOPS|SOPS]] ([[Secrets|Secrets]] OPerationS)

> **CNCF 状态**: Sandbox | **类别**: Supply Chain | **主要语言**: Go

## 概述

SOPS (Secrets OPerationS) 是一个加密文件编辑器，支持 YAML、JSON、ENV 和 BINARY 格式。它使用 AWS KMS、GCP KMS、Azure Key Vault、HashiCorp Vault 或 PGP 密钥对文件中的值进行加密，而保持键名明文，便于版本控制和代码审查。SOPS 是 GitOps 工作流中管理敏感信息的核心工具。

## 核心能力

- **多格式支持**: YAML、JSON、ENV、INI、BINARY
- **键值分离**: 只加密值，保留键名可读
- **多 KMS 后端**: AWS KMS、GCP KMS、Azure、Vault、age、PGP
- **多密钥加密**: 同时使用多个密钥加密
- **审计日志**: 加密/解密操作审计
- **GitOps 友好**: 加密文件可安全提交到 Git

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[概念/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- **age 优先**: 新项目推荐使用 age 而非 PGP
- **密钥轮换**: 定期轮换加密密钥
- **.sops.yaml**: 始终配置 .sops.yaml 简化使用
- **Git 集成**: 使用 git-diff 配置显示加密差异
- **CI/CD**: 在 pipeline 中使用 KMS 而非本地密钥
- **最小权限**: 按环境和团队划分加密密钥

## 架构定位

在 CNCF 生态中，sops 属于 **Supply Chain** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[flux]]
- [[实体/argocd.md|argocd]]
- [[实体/vault.md|vault]]
- [[deployment]]
- [[概念/controller-pattern.md|controller-pattern]]

## 安装与配置

### SOPS CLI 安装

```bash
# 🟢 安装 SOPS (Linux)
VERSION="3.9.0"
curl -LO "https://github.com/getsops/sops/releases/download/v${VERSION}/sops-v${VERSION}.linux.amd64"
chmod +x sops-v${VERSION}.linux.amd64
mv sops-v${VERSION}.linux.amd64 /usr/local/bin/sops
sops --version

# 🟢 安装 age 密钥管理工具
go install filippo.io/age/cmd/...@latest
# 生成 age 密钥对
age-keygen -o ~/.config/sops/age/keys.txt
# 公钥: age1ql3z7hjy54pw3hyww5ayyfg7zqgvc7w3j2elw8zmrj2kg5sfn9aqmcac8p

# 🟢 配置 git diff 显示加密差异
git config diff.sops.textconv 'sops -d'
echo '*.enc.yaml diff=sops' >> .gitattributes
```

### .sops.yaml 配置

```yaml
# 项目根目录 .sops.yaml
creation_rules:
  # 生产环境密钥
  - path_regex: secrets/production/.*
    age: age1ql3z7hjy54pw3hyww5ayyfg7zqgvc7w3j2elw8zmrj2kg5sfn9aqmcac8p
    # 或使用云 KMS:
    # kms: 'arn:aws:kms:us-east-1:123456789:key/xxx'
    # gcp_kms: 'projects/my-project/locations/global/keyRings/sops/cryptoKeys/sops-key'
    # azure_kv: 'https://my-vault.vault.azure.net/keys/sops-key/version'
    # hc_vault_transit: 'http://vault:8200/v1/transit/keys/sops'
  # 开发环境密钥
  - path_regex: secrets/development/.*
    age: age1dev-key-here...
  # 默认规则
  - age: age1ql3z7hjy54pw3hyww5ayyfg7zqgvc7w3j2elw8zmrj2kg5sfn9aqmcac8p
```

### 加密/解密操作

```bash
# 🟢 加密文件
sops -e secrets.yaml > secrets.enc.yaml
# 或就地加密
sops -e -i secrets.yaml

# 🟢 解密文件
sops -d secrets.enc.yaml
# 解密到文件
sops -d secrets.enc.yaml > /tmp/secrets.yaml

# 🟢 编辑加密文件（打开编辑器）
sops secrets.enc.yaml

# 🟢 仅加密特定字段
sops -e --encrypted-regex '^(password|token|secret|key)$' config.yaml

# 🟢 密钥轮换
sops updatekeys secrets.enc.yaml

# 🟢 查看文件元数据（不解密）
sops filestatus secrets.enc.yaml
```

### 加密文件示例

```yaml
# secrets.enc.yaml - 加密后的文件（可安全提交到 Git）
apiVersion: v1
kind: Secret
metadata:
    name: app-credentials
    namespace: production
type: Opaque
stringData:
    # 键名明文，值加密
    database-url: ENC[AES256_GCM,data:abc123...,iv:xyz...,tag:def...,type:str]
    api-key: ENC[AES256_GCM,data:ghi456...,iv:uvw...,tag:jkl...,type:str]
    redis-password: ENC[AES256_GCM,data:mno789...,iv:rst...,tag:pqr...,type:str]
sops:
    kms: []
    gcp_kms: []
    azure_kv: []
    hc_vault: []
    age:
        - recipient: age1ql3z7hjy54pw3hyww5ayyfg7zqgvc7w3j2elw8zmrj2kg5sfn9aqmcac8p
          enc: |
            -----BEGIN AGE ENCRYPTED FILE-----
            ...
            -----END AGE ENCRYPTED FILE-----
    lastmodified: "2026-07-01T10:00:00Z"
    mac: ENC[AES256_GCM,data:...,iv:...,tag:...,type:str]
    version: 3.9.0
```

### Flux 集成 (SOPS Provider)

```yaml
# Flux Kustomization 解密 SOPS 加密的 Secret
apiVersion: kustomize.toolkit.fluxcd.io/v1
kind: Kustomization
metadata:
  name: app-secrets
  namespace: flux-system
spec:
  interval: 10m
  path: ./secrets/production
  prune: true
  sourceRef:
    kind: GitRepository
    name: flux-system
  decryption:
    provider: sops
    secretRef:
      name: sops-age  # 包含 age 私钥的 Secret
---
# age 私钥 Secret
apiVersion: v1
kind: Secret
metadata:
  name: sops-age
  namespace: flux-system
stringData:
  age.agekey: "AGE-SECRET-KEY-1QFP0R8..."
```

### ArgoCD 集成 (Helm Secrets)

```yaml
# ArgoCD Application 使用 helm-secrets 插件
apiVersion: argoproj.io/v1alpha1
kind: Application
metadata:
  name: my-app
spec:
  source:
    repoURL: https://github.com/org/repo
    path: charts/my-app
    plugin:
      name: helm-secrets
      env:
      - name: HELM_SECRETS_SOPS_PATH
        value: /usr/local/bin/sops
  destination:
    server: https://kubernetes.default.svc
    namespace: production
```

## 运维操作

```bash
# 🟢 检查加密文件状态
sops filestatus secrets.enc.yaml

# 🟢 查看加密文件的密钥信息
sops --show-master-keys secrets.enc.yaml

# 🟢 添加新的加密密钥
sops --add-age age1new-key... secrets.enc.yaml

# 🟢 移除旧密钥
sops --rm-age age1old-key... secrets.enc.yaml

# 🟡 密钥轮换（重新加密所有值）
sops updatekeys secrets.enc.yaml

# 🟢 在 CI/CD 中解密（使用环境变量）
export SOPS_AGE_KEY_FILE=/run/secrets/age-key
sops -d secrets.enc.yaml | kubectl apply -f -
```

## 故障排查

| 症状 | 可能原因 | 诊断命令 | 修复方案 |
|------|----------|----------|----------|
| 解密失败 | 密钥不匹配/缺失 | `sops -d file` 查看错误 | 检查 age key/KMS 权限 |
| MAC 验证失败 | 文件被篡改 | `sops filestatus` | 从 Git 恢复/重新加密 |
| Flux 解密失败 | Secret 未配置 | `kubectl logs kustomize-controller` | 检查 sops-age Secret |
| 加密后格式错误 | YAML 缩进问题 | `sops -d` 验证 | 检查原始文件格式 |
| KMS 访问拒绝 | IAM 权限不足 | 检查云控制台 | 更新 IAM 策略 |

## 生产案例

### 案例1：GitOps 工作流中的密钥管理

- **场景**：团队需要在 Git 中管理 50+ 个 K8s Secret，但不能明文存储
- **方案**：所有 Secret 用 SOPS+age 加密后提交 Git；Flux 通过 decryption provider 自动解密部署；按环境划分不同 age 密钥
- **效果**：密钥安全存储在 Git，支持审计追踪和回滚，无需额外 Vault 基础设施

### 案例2：密钥泄露后紧急轮换

- **场景**：开发人员离职，其 age 私钥可能泄露
- **方案**：生成新 age 密钥对；`sops updatekeys` 重新加密所有文件；更新 Flux sops-age Secret；从所有环境移除旧密钥
- **效果**：1小时内完成全部密钥轮换，旧密钥无法解密任何文件

## 对比替代方案

| 方案 | 优势 | 劣势 | 适用场景 |
|------|------|------|----------|
| SOPS + age | 简单、Git友好、无服务器 | 密钥分发需额外处理 | GitOps/小团队 |
| SOPS + KMS | 企业级密钥管理、审计 | 云依赖、成本 | 企业/合规要求 |
| HashiCorp Vault | 功能全面、动态密钥 | 运维复杂、额外组件 | 大规模/动态密钥 |
| Sealed Secrets | K8s原生、无需外部工具 | 仅K8s Secret、集群绑定 | 纯 K8s 环境 |
| External Secrets Operator | 多后端支持、同步 | 需要外部密钥服务 | 混合环境 |

## 检查清单

- [ ] .sops.yaml 已配置且路径规则正确
- [ ] age 密钥已安全存储（不在 Git 中）
- [ ] 加密文件可安全提交到 Git
- [ ] CI/CD 中密钥通过 Secret/环境变量注入
- [ ] 密钥轮换流程已制定
- [ ] 按环境/团队划分了不同加密密钥
- [ ] Flux/ArgoCD 解密配置已验证
- [ ] git diff 配置为显示加密差异

## Related

- [[prometheus]] — Prometheus
- [[interlink]] — InterLink
- [[tinkerbell]] — Tinkerbell
- [[实体/vault.md|vault]] — HashiCorp Vault
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- sops
- [[实体/ratify.md|Ratify]]
- [[概念/IaC × 多集群管理.md|基础设施即代码 x 多集群管理]] — Cross-reference
- [[概念/GitOps × 平台工程.md|GitOps x 平台工程]] — Cross-reference
- [[实体/cncf-security.md|CNCF 安全与合规项目全景]] — Cross-reference
- [[生态参考/领域索引/etcd-index.md|etcd 知识图谱索引]]
- [[生态参考/领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
