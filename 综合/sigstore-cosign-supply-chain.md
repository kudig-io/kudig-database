---
title: "Sigstore/Cosign × 软件供应链安全"
summary: "Sigstore 生态通过无密钥签名、透明日志和自动化验证构建容器镜像供应链信任链，从构建到准入实现端到端可验证"
category: synthesis
tags:
- sigstore
- cosign
- supply-chain-security
- slsa
- in-toto
- sbom
- image-signing
tier: supporting
sources:
- 概念/k8s-security-compliance.md
- 实体/harbor.md
- 实体/trivy.md
- 概念/ci-cd-pipeline-patterns.md
created: '2026-07-19'
updated: '2026-07-19'
last_updated: '2026-07-19'
---

# Sigstore/Cosign × 软件供应链安全

## The Connection（为什么这两个领域交叉）

软件供应链攻击已成为最严重的安全威胁之一——SolarWinds、Log4Shell、Codecov 事件证明，攻击者不再直接攻破生产系统，而是污染构建流水线、篡改依赖包、替换容器镜像。Kubernetes 环境中，容器镜像是部署的基本单元，如果攻击者能将恶意镜像推送到 Registry 并成功部署，所有运行时安全措施（NetworkPolicy、RBAC、mTLS）都将失效——因为恶意代码已经"合法"运行在集群内。

Sigstore 是 Linux Foundation 下的开源项目，提供免费的代码签名和验证基础设施。Cosign 是 Sigstore 的容器签名工具，核心创新是"无密钥签名"（Keyless Signing）——开发者通过 OIDC 身份（GitHub Actions、GitLab CI）获取短期证书签名镜像，签名记录写入不可篡改的透明日志（Rekor）。无需管理长期密钥对，消除了密钥泄露和轮换的运维负担。

供应链安全的完整链条是：源码（Git commit 签名）→ 构建（SLSA 合规的 CI/CD）→ 制品（镜像签名 + SBOM + Attestation）→ 分发（Registry 策略）→ 部署（Admission 验证）→ 运行时（策略执行）。Sigstore/Cosign 覆盖"制品签名"和"部署验证"两个关键环节，与 SLSA 框架、in-toto attestation、SBOM 共同构成端到端供应链信任。

## Where They Co-occur（生产中的交叉场景）

### 场景一：CI/CD 流水线中的镜像签名

GitHub Actions 构建完成后，使用 `cosign sign` 对镜像进行无密钥签名。签名过程：OIDC Token → Fulcio CA 签发短期证书 → 证书 + 签名写入 Rekor 透明日志 → 签名附加到镜像（OCI artifact）。整个流程无需预置密钥，CI 环境的 OIDC 身份即为签名凭证。

### 场景二：Admission 时签名验证

集群部署 Kyverno 或 Gatekeeper 策略，在 Pod 创建时验证镜像签名。Kyverno 的 `verifyImages` 规则调用 Cosign 验证逻辑：检查镜像是否有有效签名、签名证书是否由受信任的 Fulcio CA 签发、签名者身份是否匹配预期（如 `https://github.com/myorg/*`）。验证失败则拒绝部署。

### 场景三：SBOM 生成与绑定

构建阶段使用 Syft 生成 SBOM（Software Bill of Materials），记录镜像内所有软件包及版本。使用 `cosign attest --type spdx` 将 SBOM 作为 in-toto attestation 附加到镜像。下游消费者可用 `cosign verify-attestation` 提取 SBOM，用于漏洞扫描（哪些 CVE 影响哪些包）和合规审计（是否包含 GPL 许可组件）。

### 场景四：SLSA 合规证明

SLSA（Supply-chain Levels for Software Artifacts）定义了供应链安全的成熟度等级（L0-L4）。构建系统生成 SLSA Provenance attestation（记录构建来源、触发方式、构建参数），使用 `cosign attest --type slsaprovenance` 附加到镜像。Admission 策略验证 Provenance：镜像是否由受信任的 CI 系统构建、是否从正确的 Git 仓库和分支构建。

### 场景五：私有 Registry 的签名验证

企业使用 Harbor 私有 Registry。Harbor 原生集成 Cosign 签名验证——配置"仅允许已签名镜像"策略后，未签名镜像无法被拉取。同时 Harbor 的复制策略确保签名随镜像一起复制到灾备 Registry。

### 场景六：多环境镜像晋升

镜像从 dev → staging → production 的晋升过程中，每个环境追加 attestation：dev 环境附加"单元测试通过"attestation，staging 附加"集成测试通过"attestation，production 的 Admission 策略要求所有 attestation 都存在且有效。形成完整的"构建 → 测试 → 部署"信任链。

## Production Patterns（生产模式与架构）

### 模式一：端到端签名验证流水线

```
┌─────────────────────────────────────────────────────────┐
│  Supply Chain Security Pipeline                          │
├─────────────────────────────────────────────────────────┤
│                                                         │
│  1. Source (Git)                                        │
│     └── Git commit 签名 (GPG/SSH)                      │
│                                                         │
│  2. Build (CI/CD)                                       │
│     ├── SLSA Provenance 生成                            │
│     ├── SBOM 生成 (Syft/Trivy)                         │
│     ├── 漏洞扫描 (Trivy/Grype)                         │
│     └── 镜像构建 (BuildKit/Buildpacks)                  │
│                                                         │
│  3. Sign (Cosign)                                       │
│     ├── cosign sign (Keyless / Key-based)               │
│     ├── cosign attest --type spdx (SBOM)               │
│     ├── cosign attest --type slsaprovenance            │
│     └── 签名写入 Rekor 透明日志                         │
│                                                         │
│  4. Push (Registry)                                     │
│     ├── Harbor/ECR/GCR 存储镜像 + 签名                 │
│     └── Registry 策略：拒绝未签名镜像                   │
│                                                         │
│  5. Deploy (Admission)                                  │
│     ├── Kyverno verifyImages / Gatekeeper              │
│     ├── 验证签名 + 证书链 + 签名者身份                  │
│     ├── 验证 SBOM attestation                          │
│     ├── 验证 SLSA Provenance                           │
│     └── 验证失败 → 拒绝部署                            │
│                                                         │
│  6. Runtime (Monitoring)                                │
│     └── 持续扫描运行中镜像的新 CVE                     │
│                                                         │
└─────────────────────────────────────────────────────────┘
```

### 模式二：Keyless vs Key-based 签名

```yaml
# Keyless 签名 (GitHub Actions)
- name: Sign image
  uses: sigstore/cosign-installer@v3
- run: cosign sign ${{ env.IMAGE }}:${{ env.TAG }}
  env:
    COSIGN_EXPERIMENTAL: "1"  # 使用 Fulcio + Rekor

# Key-based 签名 (自管密钥)
- name: Sign with key
  run: |
    cosign sign --key cosign.key $IMAGE:$TAG
  env:
    COSIGN_PASSWORD: ${{ secrets.COSIGN_PASSWORD }}
```

Keyless 适合公有 CI（GitHub/GitLab），Key-based 适合私有 CI 或需要长期验证的场景（Keyless 证书有效期短，依赖 Rekor 日志持久性）。

### 模式三：Kyverno 镜像验证策略

```yaml
apiVersion: kyverno.io/v1
kind: ClusterPolicy
metadata:
  name: verify-image-signature
spec:
  validationFailureAction: Enforce
  webhookTimeoutSeconds: 30
  rules:
  - name: verify-cosign-signature
    match:
      any:
      - resources:
          kinds:
          - Pod
          namespaces:
          - production
          - staging
    verifyImages:
    - imageReferences:
      - "harbor.internal.com/*"
      attestors:
      - count: 1
        entries:
        - keys:
            publicKeys: |-
              -----BEGIN PUBLIC KEY-----
              MFkwEwYHKoZIzj0CAQYIKoZIzj0DAQcDQgAE...
              -----END PUBLIC KEY-----
      attestations:
      - type: https://slsa.dev/provenance/v0.2
        conditions:
          all:
          - key: builder.id
            operator: Equals
            value: "https://github.com/myorg/.github/workflows"
```

### 模式四：SBOM 驱动的漏洞管理

```
构建时: syft packages $IMAGE -o spdx-json | cosign attest --type spdx
查询时: cosign verify-attestation --type spdx $IMAGE | jq '.predicate'
扫描时: 提取 SBOM → grype sbom:sbom.json → 生成 CVE 报告
准入时: 策略检查 SBOM 中无 Critical CVE（或已知例外列表）
```

### 模式五：Rekor 透明日志审计

Rekor 是不可篡改的透明日志（类似 Certificate Transparency）。所有签名记录公开可查，任何人可以监控是否有未授权的签名行为。生产环境可部署私有 Rekor 实例（`rekor-server`），或监控公共 Rekor 中与自己组织相关的条目（`rekor-cli search`）。

## Trade-offs & Decision Matrix（权衡与决策）

| 维度 | Keyless (Fulcio+Rekor) | Key-based (自管密钥) | Notary v1 (Docker) | Harbor 内置签名 |
|------|----------------------|---------------------|-------------------|----------------|
| 密钥管理 | 无需（OIDC 身份） | 需安全存储和轮换 | 需 TUF 密钥管理 | 需管理密钥 |
| 签名者验证 | OIDC 提供者验证 | 公钥验证 | TUF 信任链 | 公钥验证 |
| 透明日志 | Rekor（公开可审计） | 无 | 无 | 无 |
| 长期验证 | 依赖 Rekor 持久性 | 密钥存在即可验证 | TUF 元数据 | 密钥存在即可 |
| CI 集成 | GitHub/GitLab 原生 | 需注入密钥 | 需 Docker CLI | Harbor API |
| 离线验证 | 需访问 Rekor | 本地公钥即可 | 需 TUF 仓库 | 本地公钥即可 |
| 适用场景 | 公有 CI、开源项目 | 私有 CI、合规要求 | 遗留 Docker 环境 | Harbor 用户 |
| 运维成本 | 极低（托管服务） | 中（密钥轮换） | 高（TUF 复杂） | 低 |

### 决策矩阵

- **开源项目 / 公有 CI** → Keyless Cosign（零密钥管理）
- **企业私有 CI + 合规要求** → Key-based Cosign + 私有 Fulcio/Rekor
- **已用 Harbor** → Harbor 内置 Cosign 集成（最简单）
- **需要长期离线验证** → Key-based（不依赖外部服务）
- **SLSA L3+ 合规** → Keyless + SLSA Provenance Attestation

## Anti-patterns & Pitfalls（反模式）

### 反模式一：签名但不验证

CI 流水线中签名了镜像，但集群 Admission 未配置验证策略。签名沦为"安全剧场"——攻击者仍可推送未签名镜像并成功部署。**正确做法**：签名和验证必须成对部署，验证策略从 Audit 模式开始，确认无误后切换 Enforce。

### 反模式二：密钥硬编码在 CI 配置中

Key-based 签名时，将私钥直接写入 `.github/workflows/deploy.yaml` 或 Dockerfile。密钥随代码仓库泄露。**正确做法**：密钥存储在 Vault/KMS 中，CI 运行时动态获取；或使用 Keyless 模式彻底消除密钥管理。

### 反模式三：忽略 Attestation 验证

只验证镜像签名（证明"这个镜像没被篡改"），不验证 Attestation（证明"这个镜像是从正确的源码正确构建的"）。攻击者可以用合法密钥签名恶意镜像。**正确做法**：验证 SLSA Provenance（构建来源）+ SBOM（内容清单）+ 签名（完整性），三者缺一不可。

### 反模式四：Rekor 单点依赖

Keyless 签名依赖 Rekor 透明日志的可用性。Rekor 不可用时，新签名无法完成，验证也可能失败（需查询日志）。**正确做法**：关键环境使用私有 Rekor 实例或 Key-based 签名作为降级方案；配置 `--timeout` 避免验证阻塞部署。

### 反模式五：SBOM 生成后不更新

镜像构建时生成 SBOM，但基础镜像更新（如 Alpine 安全补丁）后 SBOM 未重新生成。SBOM 与实际内容不一致，漏洞扫描结果不可信。**正确做法**：每次构建都重新生成 SBOM；使用 `cosign attest --replace` 更新已有 attestation。

### 反模式六：过度信任 Registry 安全

认为"镜像在私有 Registry 中就是安全的"，不做签名验证。实际上 Registry 凭证泄露、内部人员恶意操作、Registry 漏洞都可能导致镜像被替换。**正确做法**：Registry 安全（访问控制、网络隔离）+ 镜像签名验证（密码学证明）双重保障。

## Operational Checklist（运维检查清单）

### 基础设施部署

- [ ] 安装 Cosign CLI（CI 环境和运维跳板机）
- [ ] 评估 Keyless vs Key-based：公有 CI 用 Keyless，私有 CI 评估私有 Fulcio
- [ ] 如用 Key-based：密钥存储在 Vault/KMS，配置自动轮换
- [ ] 部署 Kyverno/Gatekeeper 镜像验证策略（先 Audit 后 Enforce）
- [ ] 配置 Registry 策略：Harbor 启用"仅允许签名镜像"
- [ ] 验证 Webhook 超时设置（建议 30s，签名验证可能较慢）

### CI/CD 集成

- [ ] 构建阶段：生成 SBOM（Syft）+ 漏洞扫描（Trivy）
- [ ] 签名阶段：`cosign sign` + `cosign attest`（SBOM + Provenance）
- [ ] 验证阶段：CI 中 `cosign verify` 确认签名成功
- [ ] 晋升阶段：每环境追加 attestation（测试结果、审批记录）
- [ ] 失败处理：签名失败不阻塞构建但告警（避免流水线脆弱）

### 运行监控

- [ ] 监控 Admission 拒绝率：`kyverno_policy_results_total{result="fail"}`
- [ ] 监控签名验证延迟：Webhook P99 < 5s
- [ ] 定期扫描运行中镜像：Trivy Operator 持续扫描
- [ ] 监控 Rekor/Fulcio 可用性（如用 Keyless）
- [ ] 告警：未签名镜像部署尝试、签名验证失败突增

### 定期审计

- [ ] 每月：审查 Registry 中未签名镜像清单
- [ ] 每月：验证 SBOM 覆盖率（所有生产镜像都有 SBOM）
- [ ] 每季度：审查签名者身份列表（是否有离职人员的 OIDC）
- [ ] 每季度：演练"恶意镜像注入"场景（验证 Admission 拦截）
- [ ] 每年：审查密钥轮换记录（Key-based 模式）

## Related

- [[实体/harbor.md|Harbor]]
- [[实体/trivy.md|Trivy]]
- [[实体/kyverno.md|Kyverno]]
- [[概念/k8s-security-compliance.md|K8s 安全合规]]
- [[概念/ci-cd-pipeline-patterns.md|CI/CD 流水线模式]]
- [[综合/opa-kyverno-policy-as-code.md|OPA × Kyverno × Policy-as-Code]]
- [[综合/container-registry-image-scanning.md|容器镜像仓库 × 镜像扫描]]
- [[综合/compliance-k8s-soc2-hipaa.md|合规 × K8s × SOC2/HIPAA]]
