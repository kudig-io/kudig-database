---
title: "DevSecOps 流水线自动化"
description: "DevSecOps 流水线：SAST/DAST/SCA 集成、镜像扫描自动化、Policy-as-Code CI 门禁、SBOM 生成"
summary: "面向 SRE 与安全工程师的 DevSecOps 流水线完整实践，覆盖 SAST/DAST/SCA 集成、镜像扫描、Policy-as-Code 门禁、SBOM 生成与签名验证。"
category: 安全
tags:
- devsecops
- ci-cd
- sast
- dast
- sca
- sbom
- supply-chain
tier: core
created: '2026-07-19'
last_updated: 2026-07
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 平台工程师
- 安全工程师
estimated_read_time: 20min
intent_queries:
- "如何在 CI/CD 流水线集成安全扫描"
- "SAST DAST SCA 有什么区别和如何选型"
- "如何生成 SBOM 并做镜像签名"
trigger_keywords:
- devsecops
- sast
- dast
- sca
- sbom
- image scanning
- policy as code
prerequisites:
- kubectl-basics
- ci-cd-basics
- supply-chain-basics
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
authors:
- name: KUDIG Team
  role: contributor
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。

# DevSecOps 流水线自动化

> **适用版本**: 通用 CI/CD（GitHub Actions / GitLab CI / Tekton）
> **最后更新**: 2026-07

---

## 概述

传统的软件安全实践是"事后审计"模式——开发团队完成功能开发并部署到生产环境后，安全团队才介入进行渗透测试和代码审计。这种模式存在根本性问题：发现漏洞的时间越晚，修复成本越高。一个在编码阶段就能发现的 SQL 注入漏洞，修复成本可能只需要 30 分钟；但如果等到上线后才被安全扫描发现，修复可能需要数天（涉及代码修改、测试、重新部署、数据修复等），而如果漏洞在被发现之前已经被攻击者利用，损失更是无法估量。

DevSecOps（Development + Security + Operations）的核心理念是将安全左移（Shift-Left），把安全检查嵌入到 CI/CD 流水线的每个阶段，实现"安全即代码、合规即门禁"。一条成熟的 DevSecOps 流水线应该在代码提交时执行静态分析和密钥检测，在构建时执行依赖分析和 SBOM 生成，在镜像构建后执行漏洞扫描和签名，在部署时通过准入控制验证镜像的完整性和来源可信度。

本文系统覆盖 DevSecOps 流水线的核心环节与工具选型。供应链安全总览见 [[安全/供应链/01-supply-chain-security-overview.md|供应链安全总览]]，SBOM 专题见 [[安全/供应链/03-sbom-generation-management.md|SBOM 生成与管理]]，镜像签名见 [[安全/供应链/07-sigstore-cosign-signing.md|Sigstore Cosign 签名]]。

---

## 核心概念

### 1. 安全测试类型对比

理解不同安全测试类型的检测对象、适用时机和局限性，是设计流水线的基础。

| 类型 | 全称 | 检测对象 | 时机 | 代表工具 |
|------|------|---------|------|---------|
| **SAST** | 静态应用安全测试 | 源代码 | 编码/提交 | Semgrep、CodeQL、SonarQube |
| **DAST** | 动态应用安全测试 | 运行中应用 | 测试环境 | OWASP ZAP、Burp |
| **SCA** | 软件成分分析 | 依赖/开源组件 | 构建 | Trivy、Dependency-Track、Snyk |
| **IaC 扫描** | 基础设施即代码扫描 | YAML/Terraform | 提交 | Checkov、kics、kube-score |
| **镜像扫描** | 容器镜像漏洞扫描 | 镜像 | 构建后 | Trivy、Grype、Clair |
| **Secret 扫描** | 密钥泄露检测 | 代码/历史 | 提交 | gitleaks、trufflehog |

SAST 通过分析源代码的语法结构和数据流来发现安全漏洞（如 SQL 注入、XSS、路径遍历等），它的优势是能在代码编写阶段就发现问题，劣势是误报率较高且无法发现运行时漏洞。DAST 则从外部对运行中的应用发起攻击性测试，能发现 SAST 无法覆盖的运行时问题（如认证绕过、会话管理缺陷），但它只能在应用部署后才能执行。SCA 分析项目依赖的开源组件，检测已知漏洞（CVE）和许可证合规问题，在开源依赖占比极高的现代软件中尤为重要。

### 2. DevSecOps 流水线阶段

一条完整的 DevSecOps 流水线将安全检查分布在软件交付的每个阶段，形成层层防线。

```
代码提交 → SAST + Secret 扫描 + IaC 扫描
   ↓
构建     → SCA（依赖分析）+ SBOM 生成
   ↓
镜像     → 镜像扫描 + 签名（Cosign）
   ↓
准入     → Policy-as-Code 门禁（Kyverno/OPA）
   ↓
部署     → 准入控制验证签名 + SBOM 关联
   ↓
运行时   → DAST + 运行时防护
```

这种分层设计的核心思想是"尽早发现、逐层过滤"：越早期的检查成本越低、反馈越快，应该覆盖尽可能多的问题类型；越后期的检查越接近生产环境，应该聚焦于前期无法覆盖的运行时问题。每一层都是一道门禁，只有通过所有门禁的制品才能最终部署到生产环境。

### 3. 门禁策略

门禁的严格程度需要平衡安全性和开发效率。过于宽松的门禁形同虚设，过于严格的门禁则会严重拖慢开发速度，导致团队寻找绕过手段。

| 门禁级别 | 行为 | 适用 |
|---------|------|------|
| 阻断（Block） | 高危漏洞直接失败 | 生产关键路径 |
| 警告（Warn） | 记录但不阻断 | 中低危、过渡期 |
| 报告（Report） | 仅生成报告 | 观察期 |

我们的经验是：新引入的安全检查应该先以 Report 模式运行一到两周，让团队了解扫描结果和误报情况；然后切换到 Warn 模式，在 CI 输出中显示警告但不阻断；最后对确认无误的高危检查项切换到 Block 模式。这种渐进策略既保证了安全性，又给了团队适应的时间。

---

## 生产部署/实现

### 1. GitHub Actions 完整流水线 🟡

以下是一个覆盖 SAST、SCA、镜像扫描、SBOM 生成和签名的完整流水线示例。

```yaml
# 🟡 中风险：CI 流水线配置，门禁会阻断不合规构建
name: devsecops-pipeline
on:
  push:
    branches: [main]
  pull_request:

jobs:
  # ===== 阶段 1：代码安全 =====
  sast:
    runs-on: ubuntu-latest
    steps:
    - uses: actions/checkout@v4
      with:
        fetch-depth: 0
    - name: Semgrep SAST
      run: |
        docker run --rm -v "${PWD}:/src" returntocorp/semgrep \
          semgrep scan --config=auto --error --severity ERROR
    - name: Gitleaks Secret Scan
      uses: gitleaks/gitleaks-action@v2
      env:
        GITHUB_TOKEN: ${{ secrets.GITHUB_TOKEN }}

  # ===== 阶段 2：依赖与构建 =====
  sca-and-build:
    needs: sast
    runs-on: ubuntu-latest
    steps:
    - uses: actions/checkout@v4
    - name: Trivy SCA (filesystem)
      run: |
        trivy fs --scanners vuln,secret \
          --severity HIGH,CRITICAL \
          --exit-code 1 .
    - name: Build Image
      run: docker build -t ${{ secrets.REGISTRY }}/app:${{ github.sha }} .
    - name: Generate SBOM
      run: |
        syft ${{ secrets.REGISTRY }}/app:${{ github.sha }} \
          -o spdx-json=sbom.spdx.json
    - name: Image Scan
      run: |
        trivy image --severity HIGH,CRITICAL \
          --exit-code 1 \
          --ignore-unfixed \
          ${{ secrets.REGISTRY }}/app:${{ github.sha }}

  # ===== 阶段 3：签名与推送 =====
  sign-and-push:
    needs: sca-and-build
    runs-on: ubuntu-latest
    permissions:
      id-token: write
      packages: write
    steps:
    - name: Cosign Sign (keyless)
      run: |
        cosign sign --yes \
          ${{ secrets.REGISTRY }}/app:${{ github.sha }}
```

这个流水线的设计有几个值得注意的细节。fetch-depth: 0 确保 gitleaks 能扫描完整的 Git 历史，而不仅仅是最新提交——很多密钥泄露发生在历史提交中，即使当前代码已经删除了密钥，历史记录中仍然存在。Trivy 的 --ignore-unfixed 参数只报告有已知修复方案的漏洞，避免因为无法修复的漏洞而持续阻断构建。Cosign 的 keyless 签名模式使用 OIDC 身份（GitHub Actions 的 id-token）而非静态密钥，消除了密钥管理的负担，同时通过 Rekor 透明日志提供不可抵赖性。

### 2. Policy-as-Code 准入（Kyverno 验证签名） 🔴

部署阶段的最后一道防线是准入控制——在 Pod 创建时验证镜像的签名和来源。

```yaml
# 🔴 高风险：准入策略拒绝未签名镜像，配置错误会阻断部署
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
          kinds: ["Pod"]
          namespaces: ["production"]
    verifyImages:
    - imageReferences:
      - "registry.example.com/*"
      attestors:
      - count: 1
        entries:
        - keys:
            rekor:
              url: https://rekor.sigstore.dev
            tuf:
              root: ""
      conditions:
      - all:
        - key: "{{ issuer }}"
          operator: Equals
          value: "https://token.actions.githubusercontent.com"
```

这个策略确保只有经过 CI 流水线签名的镜像才能部署到 production 命名空间。验证逻辑不仅检查签名是否存在，还验证签名者的身份（certificate-oidc-issuer 必须是 GitHub Actions），防止攻击者用自己的密钥签名恶意镜像。validationFailureAction: Enforce 表示验证失败时直接拒绝 Pod 创建。这个策略应该先在非生产环境以 Audit 模式运行验证，确认不会误拦合法镜像后再切换到 Enforce。

### 3. SBOM 生成与存储 🟢

SBOM（Software Bill of Materials，软件物料清单）是软件供应链安全的基石，它记录了镜像中包含的所有软件组件及其版本，是漏洞追踪和合规审计的基础。

```bash
# 🟢 低风险：生成 SBOM
# 使用 syft 生成 SPDX 格式 SBOM
syft registry.example.com/app:v1.0 -o spdx-json=sbom.spdx.json
syft registry.example.com/app:v1.0 -o cyclonedx-json=sbom.cdx.json

# 使用 cosign 将 SBOM 附加到镜像
cosign attach sbom --sbom sbom.spdx.json \
  --type spdxjson registry.example.com/app:v1.0

# 验证 SBOM
cosign verify-attachment --type sbom registry.example.com/app:v1.0
```

SBOM 有两种主流格式：SPDX（Linux 基金会标准）和 CycloneDX（OWASP 标准），两者在功能上基本等价，选择哪种取决于下游工具的支持情况。将 SBOM 通过 cosign attach 附加到镜像是最推荐的做法——它与镜像绑定存储，不会被意外丢失，且可以通过 cosign 验证其完整性。

---

## 运维操作

### 1. 流水线安全门禁管理 🟡

门禁的严格度需要根据团队成熟度和漏洞实际情况动态调整。

```bash
# 🟡 中风险：调整 Trivy 门禁严格度
# 仅阻断 CRITICAL，HIGH 警告
trivy image --severity CRITICAL --exit-code 1 registry.example.com/app:v1.0
trivy image --severity HIGH --exit-code 0 registry.example.com/app:v1.0

# 维护漏洞白名单（.trivyignore）
echo "CVE-2024-XXXX" >> .trivyignore   # 附注释说明原因与到期日
```

漏洞白名单（.trivyignore）是一个必要但需要严格管控的机制。某些漏洞可能因为依赖链无法升级、或者经评估实际不可利用而需要临时忽略。但每一个白名单条目都应该包含：CVE 编号、忽略原因、负责人、到期复审日期。我们建议每月对白名单进行一次复审，删除已修复的条目，对到期未处理的条目升级告警。

### 2. 供应链验证 🟢

```bash
# 🟢 低风险：验证镜像签名与来源
cosign verify --certificate-identity-regexp=".*" \
  --certificate-oidc-issuer="https://token.actions.githubusercontent.com" \
  registry.example.com/app:v1.0 | jq

# 查看 SBOM 中的组件
cosign download sbom registry.example.com/app:v1.0 | syft convert -o table
```

### 3. 持续漏洞监控 🟢

```bash
# 🟢 低风险：将 SBOM 导入 Dependency-Track 持续监控
curl -X "PUT" "https://dtrack.example.com/api/v1/bom" \
  -H "X-Api-Key: $DTRACK_TOKEN" \
  -F "project=app-uuid" \
  -F "bom=@sbom.cdx.json"
```

CI 流水线中的漏洞扫描只能发现构建时已知的漏洞，对于构建后新披露的漏洞无能为力。Dependency-Track 等持续监控平台通过定期比对 SBOM 中的组件与最新漏洞数据库，能在漏洞披露的第一时间发出告警，实现"构建时扫描 + 运行时监控"的双重保障。

---

## 故障排查

### 症状 1：CI 因漏洞扫描失败

```bash
# 🟢 低风险：本地复现
trivy fs --severity HIGH,CRITICAL .
trivy image --severity HIGH,CRITICAL registry.example.com/app:v1.0
```

根因是项目依赖中包含高危 CVE，或者基础镜像存在已知漏洞。处置方法是升级含漏洞的依赖到修复版本、更换或更新基础镜像（如从 debian:bullseye 升级到 debian:bookworm）、对于确认不可利用的漏洞在审批后加入白名单。

### 症状 2：Kyverno 拒绝部署（签名验证失败）

根因可能是镜像未经过 CI 签名（如手动 docker push）、签名者身份与策略中的 certificate-oidc-issuer 不匹配、或者 Rekor 透明日志服务不可达。处置方法是确认镜像通过 CI 流水线构建并签名、核对策略中的 issuer 配置、检查 Rekor 服务的连通性。

### 症状 3：SBOM 生成失败

根因通常是镜像无法拉取（认证问题或网络问题），或者 syft 版本过旧不支持某些镜像层格式。处置方法是确认镜像仓库的访问凭证有效、更新 syft 到最新版本。

### 症状 4：流水线执行过慢

根因是每次构建都全量扫描所有依赖、Trivy 漏洞数据库每次重新下载、或者安全扫描 job 串行执行。处置方法是启用增量扫描（只扫描变更的依赖）、缓存 Trivy 漏洞数据库（CI 缓存或本地 mirror）、将独立的扫描 job 并行执行。

### 排查决策树

```
流水线失败
├── 扫描失败?     → 升级依赖/换基础镜像/白名单
├── 签名验证失败? → 确认签名/身份/Rekor
├── SBOM 失败?    → 镜像访问/syft 版本
└── 执行慢?       → 缓存/增量/并行
```

---

## 最佳实践

第一，分层门禁设计，SAST 和 Secret 扫描在提交阶段执行（反馈最快），SCA 和镜像扫描在构建阶段执行，签名验证在准入阶段执行。第二，严格度渐进，新引入的扫描先用 warn 模式让团队适应，确认无误报后再切 block。第三，SBOM 是必备项，每个镜像生成并附加 SBOM，导入 Dependency-Track 进行持续漏洞监控。第四，镜像签名使用 Cosign keyless 模式加 Rekor 透明日志，准入阶段验证签名和来源。第五，漏洞白名单需要严格治理，每个条目附注释、负责人和到期日，定期复审。第六，统一受信任基础镜像，减少漏洞面并简化扫描，参考 [[安全/供应链/10-image-security-scanning.md|镜像安全扫描]]。第七，向 SLSA Level 3 演进，建立完整的供应链可信度，见 [[安全/供应链/05-slsa-levels-implementation.md|SLSA 等级实现]]。第八，建立安全度量体系，跟踪漏洞修复时长（MTTR）、门禁通过率、白名单数量等指标，持续改进。

```yaml
# 🟢 低风险：Tekton 流水线中的扫描 Task 示例
apiVersion: tekton.dev/v1
kind: Task
metadata:
  name: trivy-scan
spec:
  params:
  - name: IMAGE
    type: string
  steps:
  - name: scan
    image: aquasec/trivy:latest
    script: |
      #!/bin/sh
      trivy image --severity HIGH,CRITICAL \
        --exit-code 1 --ignore-unfixed $(params.IMAGE)
```

---

## Related

- [[安全/供应链/01-supply-chain-security-overview.md|供应链安全总览]]
- [[安全/供应链/03-sbom-generation-management.md|SBOM 生成与管理]]
- [[安全/供应链/07-sigstore-cosign-signing.md|Sigstore Cosign 签名]]
- [[安全/供应链/10-image-security-scanning.md|镜像安全扫描]]
- [[安全/供应链/05-slsa-levels-implementation.md|SLSA 等级实现]]
- [[安全/策略治理/04-kyverno-enterprise-policy-management.md|Kyverno 企业策略管理]]
