---
title: Software Supply Chain Security
description: '- [[概念/纵深防御 × 供应链安全.md|纵深防御 x 供应链安全]] — synthesis'
summary: '- [[概念/纵深防御 × 供应链安全.md|纵深防御 x 供应链安全]] — synthesis'
category: concepts
tags:
- k8s
- security
- supply-chain
- sbom
- sigstore
- slsa
- cosign
- opa
- agent
- argocd
tier: supporting
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Software Supply Chain Security 是什么
- 如何 Software Supply Chain Security
trigger_keywords:
- Software
- Supply
- Chain
- Security
prerequisites:
- kubectl-basics
- gitops-basics
- policy-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Software Supply Chain Security

## Threat Chain

Supply chain attacks can occur at any stage:
1. **Development**: Developer machine compromised, dependency confusion attack
2. **Build**: CI/CD pipeline hijacked, build tools tampered with, backdoor inserted during compilation
3. **[[Distribution|Distribution]]**: Registry attacked, image replaced, tag mutated (same tag points to different content)
4. **Deployment**: Unsigned image deployed to production without verification

## SBOM (Software Bill of Materials)

SBOM is a formal inventory of all components in a software artifact:

| Tool | Format | Use Case |
|------|--------|----------|
| Syft | CycloneDX, SPDX | Container image SBOM generation |
| [[Trivy|Trivy]] | SPDX-JSON | Image scan + SBOM combined |
| cyclonedx-maven-plugin | CycloneDX | Java application SBOM |

SBOM enables offline vulnerability scanning and dependency tracking without needing the original image.

## Image Signing with Sigstore/Cosign

Cosign signs container images using ephemeral keys bound to OIDC identities (Sigstore keyless signing):
1. Build triggers OIDC authentication (GitHub Actions token, GCP service account)
2. Cosign generates ephemeral key pair, signs the image
3. Signature and certificate uploaded to Sigstore transparency log (Fulcio + Rekor)
4. Deployment admission verifies signature before allowing image to run

## SLSA Framework

| SLSA Level | Requirement | Implementation |
|------------|------------|----------------|
| Level 1 | Documented build process | Tekton Chains / GitHub Actions |
| Level 2 | Hosted build platform | GitHub Actions / Tekton |
| Level 3 | Hardened build platform | Tekton Chains + Cosign + SBOM |
| Level 4 | Two-party review + reproducible | Full chain signing + Hermetic Build |

## Admission Verification

Kyverno or OPA Gatekeeper policies verify image signatures before deployment:
- Block unsigned images
- Block images from untrusted registries
- Require images with no critical vulnerabilities
- Verify SBOM is attached

## 源码实现分析

### Cosign 签名验证流程

```go
// sigstore/cosign/pkg/cosign/verify.go
func VerifyImageSignatures(ctx context.Context, ref name.Reference, co *CheckOpts) ([]ociremote.Signature, error) {
    // 1. 从 Registry 获取镜像 manifest
    ref, err := ociremote.ResolveDigest(ref, co.RegistryClientOpts...)
    // 2. 获取关联的签名列表（存储在 .sig 后缀的 OCI artifact 中）
    sigs, err := ociremote.Signatures(ref, co.RegistryClientOpts...)
    // 3. 验证每个签名的证书链
    for _, sig := range sigs.Sigs {
        cert := sig.Cert // Fulcio 签发的短期证书
        // 4. 验证 OIDC 身份（GitHub Actions / GCP SA / 企业 IdP）
        if err := verifyCertIssuer(cert, co.CertOidcIssuer); err != nil {
            continue
        }
        // 5. 验证证书中的 SAN 匹配预期身份
        if cert.EmailAddresses[0] != co.CertIdentity {
            continue
        }
        // 6. 使用证书公钥验证签名
        if err := cosign.VerifySignature(sig.Payload, sig.Base64Signature, cert); err == nil {
            verified = append(verified, sig)
        }
    }
    // 7. 可选：验证 Rekor 透明日志中的 inclusion proof
    return verified, nil
}
```

### 供应链安全架构

```
┌─────────────────────────────────────────────────────────────┐
│                    供应链安全全景架构                          │
├─────────────────────────────────────────────────────────────┤
│  Developer → Source Code → CI/CD Pipeline → Registry → K8s  │
│     │            │              │              │         │    │
│  [SAST]     [依赖扫描]    [构建签名]     [镜像签名]  [准入验证] │
│  Semgrep    Trivy/Syft   Tekton Chains  Cosign   Kyverno   │
│     │            │              │              │         │    │
│  代码漏洞    SBOM生成     SLSA证明     Sigstore  策略执行    │
│  检测        依赖清单     构建来源     透明日志   签名校验    │
└─────────────────────────────────────────────────────────────┘
```

## 使用场景

### 场景一：CI/CD 中生成 SBOM 并签名镜像

```yaml
# GitHub Actions - 构建 + SBOM + 签名
name: Secure Build Pipeline
on: push
jobs:
  build-sign:
    runs-on: ubuntu-latest
    permissions:
      id-token: write  # OIDC token for keyless signing
      contents: read
    steps:
      - uses: actions/checkout@v4
      - name: Build Image
        run: docker build -t ghcr.io/org/app:${{ github.sha }} .
      - name: Generate SBOM  # 🟢 无副作用
        run: |
          syft ghcr.io/org/app:${{ github.sha }} -o cyclonedx-json > sbom.json
          cosign attach sbom --sbom sbom.json ghcr.io/org/app:${{ github.sha }}
      - name: Sign Image  # 🟢 仅写入签名到 registry
        run: cosign sign --yes ghcr.io/org/app:${{ github.sha }}
```

### 场景二：Kyverno 准入策略验证镜像签名

```yaml
# 🟡 中风险：修改集群准入策略，可能阻断部署
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
      resources:
        kinds: ["Pod"]
        namespaces: ["production", "staging"]
    verifyImages:
    - imageReferences:
      - "ghcr.io/org/*"
      attestors:
      - entries:
        - keyless:
            issuer: "https://token.actions.githubusercontent.com"
            subject: "https://github.com/org/app/.github/workflows/build.yml@refs/heads/main"
            rekor:
              url: https://rekor.sigstore.dev
```

### 场景三：离线 SBOM 漏洞扫描

```bash
# 🟢 低风险：只读操作
# 从 registry 下载 SBOM 并扫描已知漏洞
cosign download sbom ghcr.io/org/app:v1.2.3 --output-file sbom.json
grype sbom:sbom.json --fail-on high
# 对比两个版本的依赖变化
cosign download sbom ghcr.io/org/app:v1.2.2 --output-file sbom-old.json
diff <(jq '.components[].name' sbom-old.json | sort) <(jq '.components[].name' sbom.json | sort)
```

## 常见误区

| # | 误区 | 正确理解 |
|---|------|----------|
| 1 | 镜像签名 = 镜像安全 | 签名只证明来源可信，不代表无漏洞；需配合 SBOM 扫描 |
| 2 | 使用长期密钥签名更安全 | 长期密钥泄露风险更高；Sigstore keyless 短期证书 + OIDC 绑定更安全 |
| 3 | 只在 CI 签名就够了 | 必须在部署侧（Admission）验证签名，否则签名形同虚设 |
| 4 | SBOM 生成一次即可 | 每次构建都应重新生成；依赖可能在 patch 版本引入新漏洞 |
| 5 | 私有 registry 不需要签名 | 内部威胁同样存在；registry 被攻破时签名是最后防线 |
| 6 | SLSA Level 4 是必须的 | 大多数团队从 Level 2-3 开始即可；Level 4 成本极高，按需选择 |

## 面试要点

1. **Q: Sigstore keyless 签名如何工作？为什么不使用传统长期密钥？**
   A: Keyless 签名通过 OIDC 身份（GitHub Actions token、GCP SA）向 Fulcio CA 申请短期 X.509 证书（~10min TTL），用该证书签名镜像，签名和证书上传到 Rekor 透明日志。优势：无需管理长期密钥（消除密钥泄露/轮换负担）；身份绑定到 CI 系统（不可伪造）；透明日志提供公开审计能力。

2. **Q: 如何在 Kubernetes 集群中实施完整的供应链安全？**
   A: 四层防线：① CI/CD 中 Trivy 扫描 + Syft 生成 SBOM；② Cosign 签名镜像并附加 SBOM；③ Kyverno/OPA 准入策略验证签名、检查漏洞、限制 registry 来源；④ 运行时 Falco 检测异常行为。配合 SLSA 框架评估构建平台安全等级。

3. **Q: SBOM 的实际用途是什么？仅用于合规吗？**
   A: 三大用途：① 漏洞响应——新 CVE 发布时通过 SBOM 快速定位受影响镜像（无需重新扫描）；② 合规审计——满足美国行政令 14028、EU CRA 等法规要求；③ 依赖治理——检测许可证冲突、过时依赖、未授权组件。格式标准：CycloneDX（安全导向）和 SPDX（许可证导向）。

4. **Q: 依赖混淆攻击（Dependency Confusion）如何防御？**
   A: 攻击原理：在公共 registry 发布与内部包同名的恶意包，利用包管理器优先解析公共源的缺陷。防御：① 配置 scope/namespace 隔离（@org/ 前缀）；② 包管理器锁定内部 registry 优先级；③ 使用 lockfile + checksum 验证；④ CI 中 `npm audit`/`pip-audit` 检测异常包；⑤ 网络策略限制构建环境只能访问内部 registry。

## Related

- [[概念/ci-cd-pipeline-patterns.md|ci-cd-pipeline-patterns]] — CI/CD Pipeline Patterns
- [[opa]] — OPA (Open Policy Agent)
- [[kyverno]] — Kyverno
- [[实体/trivy.md|trivy]] — Trivy
- [[概念/cloud-native-defense-in-depth.md|cloud-native-defense-in-depth]] — Cloud Native Defense in Depth
- [[概念/cloud-native-defense-in-depth.md|Cloud Native Defense in Depth]]
- [[概念/ci-cd-pipeline-patterns.md|CI/CD Pipeline Patterns]]
- [[实体/trivy.md|Trivy]]
- Cosign/Sigstore
- [[kyverno|Kyverno]]
- [[概念/纵深防御 × 供应链安全.md|纵深防御 x 供应链安全]] — synthesis

- 20-kubernetes-supply-chain-security-sbom-slsa-sigstore
- [[安全/00-open-source-projects-index.md|00-open-source-projects-index]]
- 02-supply-chain-maturity-model
- 07-sigstore-cosign-signing
- 01-supply-chain-security-overview
- 03-sbom-generation-management
- 06-github-actions-slsa-build
- 08-fulcio-rekor-transparency
- 10-compliance-automation-audit
- [[安全/README.md|Domain 05: 供应链安全 (Supply Chain Security)]]
- 04-sbom-vulnerability-analysis
- 05-slsa-levels-implementation
- 09-policy-controller-verification
- 安全 MOC
- 99-slsa-supply-chain-security-guide
- [[元数据/KUDIG Knowledge Base Architecture.md|KUDIG Knowledge Base Architecture]] — Cross-reference
- [[实体/argocd.md|ArgoCD]] — Cross-reference
- [[实体/cncf-security.md|CNCF 安全与合规项目全景]] — Cross-reference
- [[生态参考/领域索引/security-index.md|Security 安全知识图谱索引]]
- [[生态参考/领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
