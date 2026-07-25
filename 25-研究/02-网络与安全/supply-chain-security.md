---
title: K8s 供应链安全深度研究
summary: 深入研究 Kubernetes 软件供应链安全体系，覆盖 SBOM 生成、镜像签名（Sigstore/Cosign）、Admission 策略、SLSA 等级实践。
category: research
tags:
- research
- security
- supply-chain
- sbom
- sigstore
- slsa
- cosign
tier: supporting
created: '2026-07-11'
updated: '2026-07-11'
last_updated: '2026-07-11'
status: done
---

# K8s 供应链安全深度研究

## 研究背景

软件供应链攻击（SolarWinds、CodeCov、xz-utils）已成为最具破坏力的攻击向量。Kubernetes 的供应链攻击面极为广泛：

- **基础镜像污染**：Docker Hub 公共镜像被植入恶意代码
- **依赖投毒**：PyPI/NPM 包被劫持（typosquatting、dependency confusion）
- **CI/CD 管道入侵**：构建管道被入侵后植入后门
- **镜像篡改**：镜像仓库被入侵后替换镜像
- **配置注入**：Helm Chart/Manifest 中植入恶意配置

## 核心问题

1. SLSA（Supply-chain Levels for Software Artifacts）框架如何指导供应链安全实践？
2. Sigstore/Cosign 镜像签名体系如何与 Kubernetes Admission Controller 集成？
3. SBOM（Software Bill of Materials）的生成、存储和消费工具链是什么？
4. 如何构建从代码到集群的端到端供应链安全管道？

## 调研发现

### 发现一：SLSA 框架与 K8s 对应实践

| SLSA 级别 | 要求 | K8s 供应链实践 |
|-----------|------|---------------|
| **L1** | 构建过程有文档 | CI/CD 管道可追溯，构建脚本版本化 |
| **L2** | 托管构建 + 签名 | 使用 GitHub Actions/GitLab CI，构建结果签名 |
| **L3** | 加强构建隔离 + 不可篡改 | 专用隔离构建环境（Tekton Chains），Provenance 存储防篡改 |
| **L4** | 双人审核 + 可重现构建 | 代码变更需两人审核，构建可独立复现验证 |

### 发现二：Sigstore/Cosign 镜像签名管道

```
构建阶段:
  源码 → CI Pipeline → 构建镜像
         │              │
         │              ↓
         │    Cosign 签名（使用 OIDC 身份 Keyless 模式）
         │              │
         ↓              ↓
  生成 SBOM     推送到 Registry（Image + Signature + SBOM）
  (Syft)              │
                      ↓
  部署阶段:
  Admission Controller（policy-controller / Kyverno）
    → 验证镜像签名（Cosign Verify）
    → 检查 SBOM 漏洞（Rekor 透明日志）
    → 放行或拒绝
```

**Cosign Keyless 签名（OIDC 身份）**：

```bash
# 🟢 在 CI 中使用 Keyless 模式签名（无需管理密钥）
cosign sign --yes \
  --identity-token="$OIDC_TOKEN" \
  registry.example.com/app:v1.2.3

# 🟢 同时签发 SBOM
syft registry.example.com/app:v1.2.3 -o cyclonedx-json > sbom.json
cosign attach sbom --sbom sbom.json registry.example.com/app:v1.2.3
cosign sign --yes registry.example.com/app:v1.2.3

# 🟢 签名记录写入 Rekor 透明日志（可公开审计）
cosign verify registry.example.com/app:v1.2.3 \
  --certificate-identity "https://github.com/org/repo/.github/workflows/build.yml@refs/heads/main" \
  --certificate-oidc-issuer "https://token.actions.githubusercontent.com"
```

### 发现三：Admission 策略强制

**Kyverno 策略：拒绝未签名镜像**：

```yaml
apiVersion: kyverno.io/v1
kind: ClusterPolicy
metadata:
  name: verify-image-signatures
spec:
  validationFailureAction: Enforce
  rules:
  - name: check-signature
    match:
      resources:
        kinds:
        - Pod
    verifyImages:
    - imageReferences:
      - "registry.example.com/*"
      attestors:
      - count: 1
        entries:
        - keyless:
            identity:
              issuer: "https://token.actions.githubusercontent.com"
              subject: "https://github.com/org/repo/.github/workflows/*"
```

### 发现四：SBOM 工具链

| 工具 | 功能 | 格式 | 集成 |
|------|------|------|------|
| **Syft** | 镜像 SBOM 生成 | SPDX/CycloneDX | CLI + CI 集成 |
| **Trivy** | 漏洞扫描 + SBOM | SPDX/CycloneDX | CLI + Operator |
| **Grype** | SBOM 漏洞匹配 | 任意 SBOM | 与 Syft 配套 |
| **Dependency-Track** | SBOM 管理+持续扫描 | CycloneDX | Web 平台 |
| **Polarys** | SBOM 可视化+合规 | CycloneDX | UI |

## 结论与建议

1. **立即实施镜像签名**：Cosign Keyless 模式零成本，配合 GitHub Actions OIDC 即可。
2. **SBOM 是合规基石**：生成 SBOM 并持续扫描，满足监管要求。
3. **Admission Controller 是强制层**：通过 Kyverno 策略阻止未签名镜像部署。
4. **SLSA L3 是生产基线**：隔离构建 + 签名 + Provenance 是防止供应链攻击的最低要求。
5. **透明日志（Rekor）是信任根**：签名记录写入公开可审计的透明日志，防篡改。

## 参考资料

- SLSA Framework: https://slsa.dev/
- Sigstore: https://www.sigstore.dev/
- Syft: https://github.com/anchore/syft
- [[08-安全/index.md|安全目录]]
- [[14-容器运行时/04-镜像构建/index.md|镜像构建目录]]

## Related

- [[24-综合/04-安全与合规/container-runtime-image-security.md|容器运行时 × 镜像安全]]
- [[25-研究/02-网络与安全/zero-trust-k8s-security.md|零信任安全架构]]
