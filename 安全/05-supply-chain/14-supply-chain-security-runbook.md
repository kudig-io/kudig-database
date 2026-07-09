---
title: 供应链安全运行手册
description: Kubernetes 软件供应链安全运行手册，覆盖镜像 SBOM、签名（cosign/notation）、准入校验（Kyverno/OPA/Gatekeeper）、仓库安全与 CI/CD 流水线加固。
summary: 供应链安全运行手册，覆盖 SBOM、镜像签名、准入校验、仓库安全与 CI/CD 加固。
category: security
tags:
- production
- best-practices
- playbook
- supply-chain
- security
- sbom
- cosign
- notation
- kyverno
- admission-control
- slsa
- provenance
tier: core
created: '2026-07-01'
last_updated: '2026-07'
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 平台工程师
- 安全工程师
estimated_read_time: 30min
intent_queries:
- 供应链安全运行手册是什么
- 如何在 Kubernetes 落地镜像签名与 SBOM
- Kyverno OPA 镜像签名校验最佳实践
trigger_keywords:
- 供应链安全
- SBOM
- cosign
- notation
- 镜像签名
- admission control
- Kyverno
- OPA
- SLSA
- provenance
prerequisites:
- kubectl-basics
- container-registry-basics
- kubernetes-policy-basics
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
- '1.33'
authors:
- name: KUDIG Team
  role: contributor
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。


# 供应链安全运行手册

> **适用范围**: 在 Kubernetes 平台落地镜像安全、SBOM、签名验证与 CI/CD 流水线加固的团队。  
> **目标读者**: SRE、平台工程师、安全工程师、DevOps。  
> **最后更新**: 2026-07-01

本手册是 [[安全/99-production-readiness-operations-guide.md|安全与合规生产就绪运维指南]] 在供应链安全方向的落地 runbook，参考 [[_reports/domain-content-gap-analysis-2026-07-01.md|域内容缺口分析]] 中“镜像仓库 / 供应链安全”缺口设计，覆盖镜像 SBOM、签名、准入校验、仓库安全与 CI/CD 加固。软件供应链安全的目标是确保从源代码到生产运行的每一步都可追溯、可验证、可审计。

---

## 1. 适用场景与范围

- 生产环境禁止未签名镜像运行。
- 镜像仓库访问策略、漏洞扫描、异地复制与灾备。
- CI/CD 流水线中生成 SBOM、签名镜像、扫描 CVE。
- 准入控制器（Kyverno / OPA Gatekeeper / Ratify）策略部署与运维。
- 供应链安全事件响应（镜像被篡改、密钥泄露、漏洞爆发）。
- SLSA 成熟度评估与 provenance attestation 落地。
- 依赖管理与私有仓库治理，防止依赖混淆与恶意包注入。

---

## 2. 前置条件与工具

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 必备工具
cosign version                   # Sigstore cosign
notation version                 # Notary v2 / notation
syft version                     # SBOM 生成
trivy version                    # 镜像漏洞扫描
kubectl version --client
helm version
```
- 已部署 Harbor / ACR EE / ECR / GCR 等企业级镜像仓库。
- 已部署 Kyverno 或 OPA Gatekeeper 策略引擎。
- CI/CD 流水线具备 push 镜像与写入仓库的权限。
- 已建立 KMS/HSM 或 Sigstore 私钥管理体系。
- 已制定漏洞分级响应流程与镜像升级策略。

---

## 3. 核心概念/架构

```
CI 构建 → SBOM 生成 → 镜像签名 → 推送到仓库 → 漏洞扫描 → 准入校验 → Pod 创建
   │           │            │            │            │             │
   ▼           ▼            ▼            ▼            ▼             ▼
Dockerfile   Syft/Trivy   cosign/    仓库扫描      Kyverno/OPA/    Pod
加固         生成 SBOM    notation   策略          Ratify          运行
```

- **SBOM**: 记录镜像包含的软件组件与依赖，用于漏洞定位与合规审计。常见格式包括 SPDX、CycloneDX、SWID。
- **镜像签名**: 使用私钥对镜像 digest 签名，验证镜像未被篡改。生产环境应使用 KMS/HSM 或 Sigstore 托管私钥。
- **准入校验**: 在 Pod 创建时校验镜像签名、扫描结果与仓库白名单，阻止不合规镜像运行。
- **Provenance Attestation**: 记录构建来源、构建工具、构建参数，满足 SLSA L2/L3 要求。
- **CI/CD 加固**: 防止恶意依赖、泄露 Secret、使用 latest 标签、未授权镜像拉取。
- **SLSA**: Supply-chain Levels for Software Artifacts，定义从 L1 到 L4 的供应链安全成熟度，L3 及以上要求可复现构建与防篡改 provenance。

---

## 4. 标准操作流程

### 4.1 生成 SBOM

SBOM 应在镜像构建后自动生成，并与镜像一起归档。SPDX 和 CycloneDX 是最常用的格式，可被 Trivy、Grype 等扫描工具解析。生成 SBOM 后应将其作为 OCI artifact 附加到镜像，便于后续审计。

```bash
# 使用 Syft 生成 SPDX JSON
syft registry.example.com/app:v1.0 -o spdx-json=app-v1.0.spdx.json

# 使用 Trivy 生成 CycloneDX
trivy image --format cyclonedx --output app-v1.0.cdx.json registry.example.com/app:v1.0

# 将 SBOM 作为 artifact 上传到仓库或 OCI 附件
oras attach --artifact-type application/spdx+json \
  registry.example.com/app:v1.0 \
  app-v1.0.spdx.json
```

### 4.2 镜像签名（cosign）

cosign 支持密钥对签名与 KMS 签名，并可将签名上传到 Sigstore 的 Rekor 透明度日志。生产环境推荐 KMS 或 HSM 托管私钥，避免私钥泄露导致签名体系失效。

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 生成密钥对（生产环境推荐 KMS/HSM）
cosign generate-key-pair

# 签名镜像（按 digest，不可变）
cosign sign --key cosign.key \
  registry.example.com/app@sha256:<digest>

# 验证签名
cosign verify --key cosign.pub \
  registry.example.com/app@sha256:<digest>

# 使用 KMS（AWS KMS 示例）
cosign sign --key awskms:///arn:aws:kms:us-east-1:<account-id>:alias/cosign-key \
  registry.example.com/app@sha256:<digest>
```
### 4.3 镜像签名（notation）

notation 是 Notary v2 的 CLI 工具，支持基于 X.509 证书的签名与验证。与 cosign 不同，notation 更贴近传统 PKI 体系，适合已有内部 CA 的组织。

```bash
# 添加证书
cat cert.pem | notation cert add --store example --type ca

# 签名
notation sign registry.example.com/app@sha256:<digest>

# 验证
notation verify registry.example.com/app@sha256:<digest>
```

### 4.4 准入策略（Kyverno）

Kyverno 通过 `verifyImages` 规则校验镜像签名。策略必须指定镜像引用与公钥，建议按项目或仓库维度拆分策略，避免一条策略覆盖过广导致误拦截。同时应配置 `failurePolicy` 与副本数，防止策略引擎故障阻塞所有 Pod 创建。

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
helm upgrade --install kyverno kyverno/kyverno -n kyverno --create-namespace

kubectl apply -f - <<EOF
apiVersion: kyverno.io/v1
kind: ClusterPolicy
metadata:
  name: verify-image-signature
spec:
  validationFailureAction: Enforce
  background: false
  rules:
  - name: verify-cosign-signature
    match:
      any:
      - resources:
          kinds:
          - Pod
    verifyImages:
    - imageReferences:
      - "registry.example.com/app*"
      attestors:
      - entries:
        - keys:
            publicKeys: |
              -----BEGIN PUBLIC KEY-----
              MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEA...
              -----END PUBLIC KEY-----
EOF
```
### 4.5 OPA Gatekeeper 镜像签名校验（备选）

OPA Gatekeeper 通过 Rego 策略实现更灵活的校验逻辑，例如要求镜像必须来自允许仓库、必须使用 digest、必须附带特定标签等。对于需要复杂策略组合的场景，Gatekeeper 更具扩展性。

```yaml
# ConstraintTemplate 示例：要求镜像必须来自允许仓库且使用 digest
apiVersion: templates.gatekeeper.sh/v1
kind: ConstraintTemplate
metadata:
  name: k8sallowedrepos
spec:
  crd:
    spec:
      names:
        kind: K8sAllowedRepos
  targets:
    - target: admission.k8s.gatekeeper.sh
      rego: |
        package k8sallowedrepos
        violation[{"msg": msg}] {
          container := input.review.object.spec.containers[_]
          not startswith(container.image, "registry.example.com/")
          msg := sprintf("镜像 %v 不在允许仓库列表中", [container.image])
        }
```

### 4.6 Ratify 与 Notation 集成

Ratify 可作为 Gatekeeper 的外部数据提供器，支持 notation 签名验证与 SBOM 校验。部署后配置 `CertificateStore` 与 `Policy`，即可在准入阶段统一校验签名、SBOM 与漏洞扫描结果。

### 4.7 仓库安全配置

镜像仓库是供应链安全的核心节点，必须从访问控制、扫描、审计、备份四个维度进行加固。启用 HTTPS/TLS，禁用 HTTP；配置机器人账户与细粒度权限；开启镜像扫描；配置复制规则与保留策略；启用操作审计日志并接入 SIEM；限制公网访问，使用 VPC Endpoint 或白名单。

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# Harbor 查看复制与 GC 任务
kubectl logs -n harbor deploy/harbor-jobservice --tail=200

# ACR EE 查看实例与安全扫描
aliyun cr GET /instances
aliyun cr GET /instances/<instance-id>/scanResults

# ECR 启用基本扫描
aws ecr put-image-scanning-configuration \
  --repository-name app \
  --image-scanning-configuration scanOnPush=true
```
### 4.8 CI/CD 流水线加固

CI/CD 是供应链攻击的主要入口，加固措施包括：禁止 latest 标签、流水线最小权限、预提交 Secret 扫描、依赖校验、使用私有 runner、provenance attestation、镜像扫描阻断等。

```yaml
# GitHub Actions 示例片段
- name: Build and push
  uses: docker/build-push-action@v6
  with:
    push: true
    tags: registry.example.com/app:${{ github.sha }}

- name: Generate SBOM
  uses: anchore/sbom-action@v0
  with:
    image: registry.example.com/app:${{ github.sha }}

- name: Sign image
  uses: sigstore/cosign-installer@v3
- run: cosign sign --key env://COSIGN_PRIVATE_KEY registry.example.com/app:${{ github.sha }}

- name: Scan image
  uses: aquasecurity/trivy-action@master
  with:
    image-ref: registry.example.com/app:${{ github.sha }}
    severity: CRITICAL,HIGH
    exit-code: 1
```

### 4.9 SLSA 与 Provenance

通过 cosign attest 生成 provenance attestation，记录构建来源与参数。SLSA L3 要求构建过程运行在受控环境中，并且 provenance 由受信任的构建服务签名。建议结合 GitHub Actions OIDC 与 Sigstore 实现无密钥签名。

```bash
# 生成并附加 provenance attestation
cosign attest --predicate ./provenance.json \
  --type slsaprovenance \
  --key cosign.key \
  registry.example.com/app@sha256:<digest>

# 验证 provenance
cosign verify-attestation --key cosign.pub \
  --type slsaprovenance \
  registry.example.com/app@sha256:<digest>
```

### 4.10 供应链事件响应

当发现镜像被篡改、密钥泄露或 CVE 爆发时，应立即执行以下步骤：隔离受影响镜像、撤销签名密钥或证书、通知相关团队、重新构建并签名镜像、更新准入策略公钥、审计拉取记录、复盘并改进流程。事件响应团队应预先制定联系人名单、升级路径与沟通模板，确保在紧急情况下能够快速止损。

### 4.11 SLSA 成熟度等级参考

| 等级 | 要求 | 落地要点 |
|---|---|---|
| L1 | 构建过程自动化 | 使用 CI/CD 自动构建，禁止本地手动构建 |
| L2 | 生成来源级 provenance | 使用 cosign/notation 记录构建来源 |
| L3 | 构建环境受信任 | 使用私有 runner、HSM 签名、可复现构建 |
| L4 | 完全可复现与防篡改 | 构建过程完全隔离，所有输入输出可验证 |

### 4.12 CI/CD 加固检查清单

| 检查项 | 要求 | 验证方法 |
|---|---|---|
| 分支保护 | 主分支禁止直接推送 | GitHub/GitLab 分支规则 |
| 代码审查 | 所有合并需至少一名 reviewer | PR/MR 记录 |
| Secret 扫描 | 提交前运行 gitleaks/git-secrets | 预提交 hook |
| 依赖校验 | 使用 lockfile 与 checksum | CI 依赖安装步骤 |
| 镜像扫描 | 阻断 HIGH/CRITICAL 漏洞 | trivy exit-code |
| 签名与 SBOM | 每次构建生成并归档 | CI artifact |
| Runner 安全 | 使用私有或 hardened runner | Runner 配置 |

---

## 5. 关键检查点与验证命令

| 检查项 | 验证命令 | 通过标准 |
|---|---|---|
| 镜像已签名 | `cosign verify --key cosign.pub <image>@sha256:<digest>` | 验证通过 |
| SBOM 已生成 | `oras manifest fetch <image>:sbom` 或查看 CI artifact | 存在且可解析 |
| 漏洞扫描无高危 | `trivy image --severity HIGH,CRITICAL <image>` | 0 HIGH/CRITICAL 或已批准例外 |
| 准入策略生效 | `kubectl get clusterpolicy verify-image-signature` | READY，无失败事件 |
| 仓库 TLS/审计 | 浏览器访问仓库 / 查看审计日志 | HTTPS、审计开启 |
| 未使用 latest | `kubectl get pods -A -o jsonpath='{..image}' \| grep ':latest'` | 生产环境无结果 |
| 签名密钥安全 | KMS/HSM 审计日志 | 无私钥泄露事件 |
| Provenance | `cosign verify-attestation ...` | 构建来源可验证 |
| CI 权限 | 检查 CI service account 权限 | 仅允许 push 指定仓库 |

---

## 6. 常见故障与 remediation

| 现象 | 可能根因 | 确认命令 | 修复措施 |
|---|---|---|---|
| Pod 无法创建，事件提示 signature verification failed | 镜像未签名或公钥不匹配 | `kubectl get events`；`cosign verify` | 重新签名镜像、更新 Kyverno publicKeys |
| Kyverno 拦截所有 Pod 创建 | 策略匹配过于宽泛、publicKeys 配置错误 | `kubectl logs -n kyverno deploy/kyverno-admission-controller` | 收窄 imageReferences、修正密钥 |
| 漏洞扫描误报阻塞发布 | 基础镜像 CVE 无修复版本 | `trivy image --vuln-type os <image>` | 使用 `.trivyignore` 审批流程、升级基础镜像 |
| cosign 验证失败 | KMS 密钥轮换、rekor 不可达 | `cosign verify --rekor-url` | 指定内部 rekor、检查网络策略 |
| 仓库复制失败 | 目标仓库凭证过期或配额满 | `kubectl logs -n harbor deploy/harbor-jobservice` | 更新凭证、清理旧镜像 |
| CI 流水线泄露 Secret | 环境变量未加密或日志未屏蔽 | `git-secrets --scan-history` | 轮换 Secret、启用 masking、审计流水线 |
| Ratify 验证失败 | CertificateStore 证书过期 | `kubectl get certificatestore -n gatekeeper-system` | 更新 CA 证书、重启 Ratify |
| 镜像被篡改 | 仓库凭证泄露或构建环境被入侵 | 仓库审计日志 / SBOM 比对 | 轮换凭证、重新构建、通知安全团队 |
| 依赖混淆攻击 | 私有包与公共包同名 | 检查依赖来源与 checksum | 使用内部私有仓库、锁定依赖版本 |

---

## 7. 风险与注意事项

- **签名私钥安全**: 私钥应存储于 KMS/HSM，禁止明文保存在 CI secret 或 Git 仓库。
- **镜像 digest 校验**: 策略必须校验 digest，而非 tag，防止 tag 被篡改后绕过签名。
- **准入策略故障影响**: Kyverno/OPA webhook 异常会阻塞所有 Pod 创建，建议配置 `failurePolicy=Ignore` 的兜底策略或部署高可用副本。
- **SBOM 更新频率**: 每次镜像重建都应重新生成 SBOM，旧 SBOM 与镜像 digest 不一致会失去意义。
- **合规保留期**: SBOM、签名、扫描报告保留期应满足行业合规要求（如软件供应链安全要求保留 ≥ 3 年）。
- **私有仓库可用性**: 生产镜像拉取高度依赖仓库，需配置镜像缓存、仓库多活或离线备份。
- **依赖混淆攻击**: 使用内部私有仓库时，需防止攻击者上传同名包到公共仓库。
- **策略误报**: 过于严格的策略可能导致正常业务无法发布，应建立例外审批流程与策略灰度机制。
- **密钥轮换**: 定期轮换签名密钥与证书，制定旧签名验证兼容期，避免一次性轮换导致历史镜像无法验证。

---

## 8. 相关 Runbook / 推荐阅读

### 同域核心文档

- [[安全/99-production-readiness-operations-guide.md|安全与合规生产就绪运维指南]]
- [[安全/05-supply-chain/01-supply-chain-security-overview.md|供应链安全概览]]
- [[安全/05-supply-chain/03-sbom-generation-management.md|SBOM 生成与管理]]
- [[安全/05-supply-chain/07-sigstore-cosign-signing.md|Sigstore Cosign 镜像签名]]
- [[安全/05-supply-chain/08-fulcio-rekor-transparency.md|Fulcio / Rekor 透明度日志]]
- [[安全/05-supply-chain/09-policy-controller-verification.md|镜像签名校验策略控制器]]
- [[安全/05-supply-chain/99-slsa-supply-chain-security-guide.md|SLSA 供应链安全指南]]
- [[安全/05-supply-chain/13-image-security-scanning.md|镜像安全扫描]]

### 跨域参考

- [[_reports/domain-content-gap-analysis-2026-07-01.md|域内容缺口分析]]
- [[容器运行时/99-production-readiness-operations-guide.md|容器运行时生产就绪运维指南]]
- [[容器运行时/03-containerd-cri-o/01-containerd-production-operations.md|containerd 生产运维指南]]
- [[容器运行时/02-image-management/01-harbor-enterprise-image-registry.md|Harbor 企业级镜像仓库]]
- [[发布变更/README.md|发布与变更管理]]
- [[生产运维/README.md|生产运维]]

---

*本手册应随镜像仓库、签名工具、策略引擎版本更新而迭代。建议每次安全演练后 review 策略有效性与误报率。*


<!-- risk-assessed -->
