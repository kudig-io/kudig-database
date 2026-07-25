---
title: 供应链准入控制
description: 镜像签名验证、准入策略与供应链安全
summary: 使用 Cosign/Sigstore 验证镜像签名，通过准入策略只允许受信任镜像部署
category: manifests-patterns
tags:
- k8s
- manifests
- security
- supply-chain
- cosign
- sigstore
- image-signing
tier: core
created: '2026-07-11'
last_updated: 2026-07
difficulty: advanced
reading_level: advanced
audience:
- 安全工程师
- 平台工程师
- SRE
estimated_read_time: 12min
intent_queries:
- 镜像签名验证
- Cosign Kubernetes 准入
- 供应链安全
trigger_keywords:
- cosign
- sigstore
- supply-chain
- image-signing
- admission
prerequisites:
- admission-webhook-basics
- container-registry-basics
authors:
- name: KUDIG Team
  role: contributor
---

# 供应链准入控制

## 1. 供应链安全威胁

| 威胁 | 说明 | 防御 |
|------|------|------|
| 镜像篡改 | 中间人攻击替换镜像 | 镜像签名验证 |
| 恶意镜像 | 上游投毒 | 白名单仓库 |
| 过时镜像 | 已知漏洞 | 漏洞扫描准入 |
| 未授权镜像 | 任意来源 | 仅允许受信任仓库 |

## 2. Sigstore/Cosign 镜像签名

### 2.1 CI/CD 中签名

```bash
# 🟢 低风险：签名操作
# 在 CI 流水线中签名镜像
cosign sign --key cosign.key \
  registry.example.com/app:v1.0.0

# 使用 keyless 签名（基于 OIDC）
cosign sign --identity-token \
  registry.example.com/app:v1.0.0

# 附带 SBOM
cosign attach sbom --sbom sbom.spdx \
  registry.example.com/app:v1.0.0
cosign sign --attachment sbom \
  registry.example.com/app:v1.0.0
```

### 2.2 验证签名

```bash
# 🟢 低风险：验证操作
cosign verify --key cosign.pub \
  registry.example.com/app:v1.0.0

# 验证特定 issuer/subject
cosign verify \
  --certificate-identity "https://github.com/example/repo/.github/workflows/deploy.yml@refs/heads/main" \
  --certificate-oidc-issuer "https://token.actions.githubusercontent.com" \
  registry.example.com/app:v1.0.0
```

## 3. Kyverno 镜像签名验证策略

```yaml
apiVersion: kyverno.io/v1
kind: ClusterPolicy
metadata:
  name: verify-image-signatures
spec:
  validationFailureAction: Enforce
  background: true
  rules:
    - name: verify-registry-signature
      match:
        any:
          - resources:
              kinds: ["Pod"]
      verifyImages:
        - imageReferences:
            - "registry.example.com/*"   # 仅验证内部仓库
          attestors:
            - entries:
                - keys:
                    publicKeys: |
                      -----BEGIN PUBLIC KEY-----
                      MFkwEwYHKoZIzj0CAQYIKoZIzj0DAQcDQgAE...
                      -----END PUBLIC KEY-----
    - name: verify-keyless-signature
      match:
        any:
          - resources:
              kinds: ["Pod"]
      verifyImages:
        - imageReferences:
            - "ghcr.io/example/*"
          attestors:
            - count: 1
              entries:
                - keyless:
                    subject: "https://github.com/example/*"
                    issuer: "https://token.actions.githubusercontent.com"
                    rekor:
                      url: https://rekor.sigstore.dev
```

## 4. 限制镜像仓库白名单

```yaml
apiVersion: kyverno.io/v1
kind: ClusterPolicy
metadata:
  name: allowed-registries
spec:
  validationFailureAction: Enforce
  rules:
    - name: only-allowed-registries
      match:
        any:
          - resources:
              kinds: ["Pod"]
      validate:
        message: "镜像必须来自受信任的仓库"
        foreach:
          - list: "request.object.spec.containers"
            pattern:
              image: "registry.example.com/* | ghcr.io/example/* | docker.io/library/*"
          - list: "request.object.spec.initContainers"
            pattern:
              image: "registry.example.com/* | ghcr.io/example/*"
```

## 5. 漏洞扫描准入（Trivy + Kyverno）

```yaml
apiVersion: kyverno.io/v1
kind: ClusterPolicy
metadata:
  name: disallow-high-severity
spec:
  validationFailureAction: Enforce
  webhooks:
    failurePolicy: Fail
  rules:
    - name: block-critical-vulnerabilities
      match:
        any:
          - resources:
              kinds: ["Pod"]
      preconditions:
        all:
          - key: "{{ request.operation || 'BACKGROUND' }}"
            operator: Equals
            value: "CREATE"
      context:
        - name: scanResult
          apiCall:
            method: GET
            urlPath: "/apis/aquasecurity.github.io/v1alpha1/configauditreports"
            queryParameters:
              - name: resource
                value: "{{ request.object.kind }}/{{ request.object.metadata.name }}"
      validate:
        message: "镜像包含 CRITICAL 级别漏洞，不允许部署"
        deny:
          conditions:
            any:
              - key: "{{ scanResult.report.summary.criticalCount || 0 }}"
                operator: GreaterThan
                value: 0
```

## 6. Connaisseur（专用签名验证 Admission Controller）

```yaml
# Connaisseur 配置
apiVersion: v1
kind: ConfigMap
metadata:
  name: connaisseur-config
  namespace: connaisseur
data:
  config.yaml: |
    validators:
      - name: cosign
        type: cosign
        trust_roots:
          - name: default
            key: |
              -----BEGIN PUBLIC KEY-----
              MFkwEwYHKoZIzj0CAQYIKoZIzj0DAQcDQgAE...
              -----END PUBLIC KEY-----
      - name: keyless
        type: cosign
        trust_roots:
          - name: github-actions
            keyless:
              issuer: "https://token.actions.githubusercontent.com"
              identity: "https://github.com/example/*"
    policy:
      - pattern: "registry.example.com/*:*"
        validator: cosign
      - pattern: "ghcr.io/example/*:*"
        validator: keyless
      - pattern: "docker.io/library/*:*"
        validator: default_allow
```

## 7. Sigstore Policy Controller（基于 Cosign）

```yaml
apiVersion: policy.sigstore.dev/v1beta1
kind: ClusterImagePolicy
metadata:
  name: require-signature
spec:
  images:
    - glob: "registry.example.com/**"
  authorities:
    - name: cosign-key
      key:
        data: |
          -----BEGIN PUBLIC KEY-----
          MFkwEwYHKoZIzj0CAQYIKoZIzj0DAQcDQgAE...
          -----END PUBLIC KEY-----
    - name: keyless
      keyless:
        url: https://fulcio.sigstore.dev
        identities:
          - issuer: "https://token.actions.githubusercontent.com"
            subject: "https://github.com/example/*"
```

## 8. SBOM 与溯源

```yaml
# 验证 SBOM 存在
apiVersion: kyverno.io/v1
kind: ClusterPolicy
metadata:
  name: require-sbom
spec:
  validationFailureAction: Audit
  rules:
    - name: verify-sbom
      match:
        any:
          - resources:
              kinds: ["Pod"]
      verifyImages:
        - imageReferences: ["registry.example.com/*"]
          attestations:
            - type: spdxjson
              attestors:
                - entries:
                    - keys:
                        publicKeys: |
                          -----BEGIN PUBLIC KEY-----
                          ...
                          -----END PUBLIC KEY-----
```

## 9. 生产实践

| 实践 | 说明 |
|------|------|
| 先 Audit 后 Enforce | 观察哪些镜像未签名 |
| CI 强制签名 | 构建后立即签名 |
| 使用 Keyless 签名 | 避免 key 管理 |
| 白名单注册表 | 只允许受信任仓库 |
| 定期漏洞扫描 | 阻止高危漏洞镜像 |
| 生成 SBOM | 软件物料清单溯源 |

## Related

- [[03-清单模式/06-安全模式/07-opa-kyverno-policy-examples|OPA/Kyverno 策略]]
- [[03-清单模式/06-安全模式/01-pod-security-standards-reference|Pod Security Standards]]

## See Also

- [Sigstore/Cosign 文档](https://docs.sigstore.dev/)
- [Kyverno 镜像验证](https://kyverno.io/docs/writing-policies/verify-images/)
- [Connaisseur](https://sse-secure-systems.github.io/connaisseur/)

<!-- risk-assessed -->
