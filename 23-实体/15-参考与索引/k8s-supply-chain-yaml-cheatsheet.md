---
title: 供应链安全、YAML 配置清单与速查表
description: '# 供应链安全、YAML 配置清单与速查表'
summary: '1. **SBOM（Software Bill of Materials）**：软件物料清单'
category: reference
tags:
- k8s
- supply-chain-security
- sbom
- slsa
- sigstore
- yaml
- cheat-sheet
- docker
- ingress
- rbac
tier: core
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 供应链安全、YAML 配置清单与速查表 是什么
- 如何 供应链安全、YAML 配置清单与速查表
trigger_keywords:
- 供应链安全
- YAML
- 配置清单与速查表
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# 供应链安全、YAML 配置清单与速查表

> **CNCF 状态**: 参考文档 | **类别**: Supply Chain Security | **主要语言**: YAML

## 概述

Kubernetes 供应链安全 YAML 速查表是一份涵盖 K8s 供应链安全各环节配置的快速参考文档。它整合了 SLSA 框架、Sigstore 签名验证、SBOM 生成、镜像策略准入、GitOps 安全配置等关键供应链安全实践的 YAML 配置示例。该文档为 DevSecOps 团队提供从代码提交到生产部署全链条的安全配置参考，帮助实施 SLSA Level 1-4 的供应链安全控制。

## Key Features（核心能力）

- **SLSA 合规配置**：SLSA Build Level 1-4 的构建来源验证和完整性证明配置
- **镜像签名验证**：Cosign/Notation 的镜像签名和 Admission Controller 验证策略
- **SBOM 生成**：Synergy/CycloneDX 的 SBOM 生成和存储配置
- **策略准入**：Kyverno/Cosign Gatekeeper 的镜像安全策略 YAML
- **GitOps 安全**：ArgoCD/Flux 的安全配置和签名验证策略
- **密钥管理**：Sealed Secrets/SOPS 的加密配置 YAML

## 架构与工作原理

供应链安全配置覆盖四个阶段：Source（代码来源验证、提交签名）、Build（构建来源证明、SBOM 生成、镜像签名）、Package（Registry 策略、镜像扫描）、Deploy（准入验证、策略执行）。每个阶段的 YAML 配置通过 K8s CRD 或控制器配置实施，形成从代码到生产的安全链条。

## K8s 集成

在 K8s 中，供应链安全通过多种 API 对象实施：ValidatingWebhookConfiguration 执行镜像签名验证策略；ClusterPolicy（Kyverno CRD）定义镜像来源限制；ConfigMap 承载 Cosign 公钥和验证规则；Secret 存储签名密钥。CI/CD 流水线中的 Cosign 签名步骤和 K8s 部署时的验证策略通过共享的公钥/策略配置关联。

## 生产用例

- **DevSecOps 流水线**：在 CI/CD 中实施镜像签名和安全扫描
- **合规要求实施**：满足 SLSA、NIST SSDF 供应链安全框架要求
- **镜像来源控制**：限制集群仅部署经过签名的可信镜像
- **安全审计准备**：快速配置和验证供应链安全控制措施

## 安装与配置

```bash
# 🟢 安装 Cosign
GOFLAGS="-tags=e2e" go install github.com/sigstore/cosign/v2/cmd/cosign@latest
# 或
curl -LO https://github.com/sigstore/cosign/releases/latest/download/cosign-linux-amd64
chmod +x cosign-linux-amd64 && mv cosign-linux-amd64 /usr/local/bin/cosign

# 🟢 生成签名密钥对
cosign generate-key-pair

# 🟢 签名镜像
cosign sign --key cosign.key my-registry/app:v1

# 🟢 验证签名
cosign verify --key cosign.pub my-registry/app:v1

# 🟢 安装 Kyverno
helm repo add kyverno https://kyverno.github.io/kyverno/
helm install kyverno kyverno/kyverno -n kyverno --create-namespace

# 🟢 应用镜像签名验证策略
kubectl apply -f verify-image-policy.yaml

# 🟢 生成 SBOM
syft my-registry/app:v1 -o cyclonedx-json > sbom.json
cosign attach sbom --sbom sbom.json my-registry/app:v1
```

### Kyverno 镜像签名验证策略

```yaml
apiVersion: kyverno.io/v1
kind: ClusterPolicy
metadata:
  name: verify-image-signatures
spec:
  validationFailureAction: Enforce
  background: false
  webhookTimeoutSeconds: 30
  rules:
    - name: verify-cosign-signature
      match:
        any:
          - resources:
              kinds:
                - Pod
      verifyImages:
        - imageReferences:
            - "my-registry.com/*"
          attestors:
            - count: 1
              entries:
                - keys:
                    publicKeys: |-
                      -----BEGIN PUBLIC KEY-----
                      MFkwEwYHKoZIzj0CAQYIKoZIzj0DAQcDQgAE...
                      -----END PUBLIC KEY-----
---
# 镜像来源限制策略
apiVersion: kyverno.io/v1
kind: ClusterPolicy
metadata:
  name: restrict-image-registries
spec:
  validationFailureAction: Enforce
  rules:
    - name: validate-registries
      match:
        any:
          - resources:
              kinds:
                - Pod
      validate:
        message: "Images must come from approved registries only"
        pattern:
          spec:
            containers:
              - image: "my-registry.com/* | ghcr.io/myorg/*"
```

### CI/CD 签名流水线示例

```yaml
# GitHub Actions 示例
name: Build and Sign
on: [push]
jobs:
  build-sign:
    runs-on: ubuntu-latest
    permissions:
      id-token: write
      contents: read
    steps:
      - uses: actions/checkout@v4
      - name: Build Image
        run: docker build -t my-registry.com/app:${{ github.sha }} .
      - name: Push Image
        run: docker push my-registry.com/app:${{ github.sha }}
      - name: Install Cosign
        uses: sigstore/cosign-installer@v3
      - name: Sign with Keyless
        run: cosign sign --yes my-registry.com/app:${{ github.sha }}
      - name: Generate SBOM
        run: |
          syft my-registry.com/app:${{ github.sha }} -o cyclonedx-json > sbom.json
          cosign attach sbom --sbom sbom.json my-registry.com/app:${{ github.sha }}
```

## 运维操作

```bash
# 🟢 验证集群中所有镜像签名
kubectl get pods -A -o jsonpath='{range .items[*]}{range .spec.containers[*]}{.image}{"\n"}{end}{end}' | sort -u | while read img; do
  echo "Verifying: $img"
  cosign verify --key cosign.pub $img 2>/dev/null && echo "  ✅ Signed" || echo "  ❌ Unsigned"
done

# 🟢 查看 Kyverno 策略状态
kubectl get clusterpolicy
kubectl get policyreport -A

# 🟢 查看被拒绝的部署
kubectl get events -A --field-selector reason=PolicyViolation

# 🟡 临时切换策略为 Audit 模式
kubectl patch clusterpolicy verify-image-signatures --type=merge -p \
  '{"spec":{"validationFailureAction":"Audit"}}'

# 🔴 删除策略（允许未签名镜像部署）
kubectl delete clusterpolicy verify-image-signatures
```

## 故障排查

| 症状 | 可能原因 | 诊断命令 | 修复方法 |
|------|----------|----------|----------|
| Pod 被拒绝部署 | 镜像未签名 | `kubectl get events` | 签名镜像或调整策略 |
| 签名验证失败 | 公钥不匹配 | `cosign verify --key` | 更新策略中的公钥 |
| Kyverno 未生效 | Webhook 未注册 | `kubectl get validatingwebhookconfigurations` | 重启 Kyverno |
| SBOM 缺失 | CI 未生成 | `cosign download sbom <img>` | 修复 CI 流水线 |

```bash
# 排查流程
# 1. 检查 Kyverno 状态
kubectl get pods -n kyverno
kubectl logs -n kyverno -l app.kubernetes.io/name=kyverno --tail=50

# 2. 检查策略违规事件
kubectl get events -A --field-selector reason=PolicyViolation --sort-by='.lastTimestamp'

# 3. 验证镜像签名
cosign verify --key cosign.pub <image>:<tag>

# 4. 检查 Webhook 配置
kubectl get validatingwebhookconfigurations | grep kyverno
```

## 生产案例

### 案例1：全链路供应链安全
- **场景**：金融企业需要确保生产集群只部署经过签名和扫描的镜像
- **方案**：CI 中 Cosign 签名 + Trivy 扫描 + SBOM 生成；K8s Kyverno 强制验证签名和扫描结果；未签名镜像自动拒绝
- **效果**：未授权镜像部署事件降为 0，通过 SLSA Level 3 审计

### 案例2：紧急漏洞响应
- **场景**：发现基础镜像 CVE 漏洞，需要快速识别受影响工作负载
- **方案**：通过 SBOM 查询包含漏洞组件的镜像；Kyverno 策略禁止包含漏洞镜像的新部署；批量更新受影响工作负载
- **效果**：受影响工作负载识别时间从 2天 缩短到 10分钟

## 对比替代方案

| 维度 | Sigstore+Kyverno | Notation+Gatekeeper | 商业方案(Sysdig) | 无控制 |
|------|-----------------|--------------------|--------------|--------|
| 开源 | 是 | 是 | 否 | - |
| K8s 原生 | 强 | 强 | 中 | - |
| Keyless 签名 | 支持 | 不支持 | 支持 | - |
| SBOM | 支持 | 部分 | 支持 | - |
| 学习曲线 | 中 | 中 | 低 | - |

## 检查清单

- [ ] 镜像签名已在 CI/CD 中配置
- [ ] Kyverno/Gatekeeper 验证策略已部署
- [ ] 策略已设为 Enforce 模式（生产）
- [ ] SBOM 生成已集成到 CI
- [ ] 镜像来源限制策略已配置
- [ ] 公钥已安全存储和分发
- [ ] 策略违规告警已配置

## Related

- [[22-概念/11-交叉分析/纵深防御 × 供应链安全.md|纵深防御 x 供应链安全]] — 纵深防御 x 供应链安全
- [[docker]] — Docker
- [[23-实体/02-K8s核心组件/networkpolicy.md|networkpolicy]] — NetworkPolicy
- [[deployment]] — Deployment
- [[23-实体/06-安全/trivy.md|trivy]] — Trivy


<!-- risk-assessed -->
