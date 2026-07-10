---
title: SLSA 软件供应链安全实践指南
description: '# SLSA 软件供应链安全实践指南'
summary: 'kubectl apply -f https://storage.googleapis.com/tekton-releases/chains/latest/release.yaml'
category: supply-chain-security
tags:
- k8s
- supply-chain
- security
- sbom
- slsa
- harbor
- opa
- falco
- crd
- operator
tier: supporting
created: '2026-05-23'
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- 安全工程师
- SRE
- 架构师
estimated_read_time: 5min
intent_queries:
- SLSA 软件供应链安全实践指南 是什么
- 如何 SLSA 软件供应链安全实践指南
- Kubernetes 39 supply chain security 最佳实践
trigger_keywords:
- SLSA
- 软件供应链安全实践指南
- supply
- chain
- security
prerequisites:
- kubectl-basics
- rbac-basics
- policy-basics
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




# SLSA 软件供应链安全实践指南

> **适用版本**: SLSA v1.0 / Sigstore / Tekton Chains v0.24  
> **最后更新**: 2026-04-24  
> **难度**: 高级

---

<!-- chunk: 📋 目录 -->## 📋 目录

- [一、供应链攻击面](#一供应链攻击面)
- [二、SLSA 框架概述](#二slsa-框架概述)
- [三、Sigstore 无密钥签名](#三sigstore-无密钥签名)
- [四、Tekton Chains 构建证明](#四tekton-chains-构建证明)
- [五、cosign 镜像签名与验证](#五cosign-镜像签名与验证)
- [六、SBOM 生成与管理](#六sbom-生成与管理)
- [七、策略引擎验证 ([[Kyverno|Kyverno]]/OPA)](#七策略引擎验证-kyvernoopa)
- [八、SLSA 等级提升路径](#八slsa-等级提升路径)
- [九、参考架构](#九参考架构)

---

<!-- chunk: 一、供应链攻击面 -->## 一、供应链攻击面

```
软件供应链攻击向量
├── 源代码
│   ├── 开发者账户泄露
│   ├── 恶意代码注入
│   └── 依赖投毒 (dependency confusion)
│
├── 构建系统
│   ├── 构建环境篡改
│   ├── 缓存投毒
│   └── 非确定性构建
│
├── 制品仓库
│   ├── 镜像替换
│   ├── 标签可变 (mutable tags)
│   └── 签名伪造
│
└── 部署环节
    ├── 配置漂移
    ├── 密钥泄露
    └── 中间人攻击
```

## 著名供应链攻击

| 事件 | 年份 | 影响 | 教训 |
|:---|:---|:---|:---|
| SolarWinds Orion | 2020 | 18,000+ 组织 | 构建系统被入侵 |
| Codecov Bash Uploader | 2021 | 数千 CI 环境 | 脚本供应链攻击 |
| Log4j (Log4Shell) | 2021 | 全球影响 | 依赖库漏洞 |
| PyPI 依赖投毒 | 2022+ | 多个包 | 命名空间混淆 |
| xz Utils 后门 | 2024 | 几乎影响所有 Linux | 长期社会工程 |

---

<!-- chunk: 二、SLSA 框架概述 -->## 二、SLSA 框架概述

```
SLSA (Supply-chain Levels for Software Artifacts)
├── 来源 (Provenance)
│   └── 谁、什么、何时、如何构建了制品
│
├── 等级定义
│   ├── SLSA 1: 来源记录 (基础)
│   ├── SLSA 2: 签名来源 + 托管构建
│   ├── SLSA 3: 防篡改构建 + 审计日志
│   └── SLSA 4: 双人审核 + 可复现构建
│
└── 核心原则
    ├── 不可伪造 (Non-falsifiable)
    ├── 可审计 (Auditable)
    └── 可验证 (Verifiable)
```

## SLSA 等级要求

| 等级 | 来源 | 构建 | 依赖 | 典型实现 |
|:---|:---|:---|:---|:---|
| **SLSA 1** | 记录来源 | 无要求 | 无要求 | 手动记录构建信息 |
| **SLSA 2** | 签名来源 | 托管构建 | 无要求 | Tekton + Tekton Chains |
| **SLSA 3** | 签名来源 | 防篡改 | 无要求 | 隔离构建 + 审计日志 |
| **SLSA 4** | 签名来源 | 可复现 | 完整追踪 | 双人审核 + 确定性构建 |

---

<!-- chunk: 三、Sigstore 无密钥签名 -->## 三、Sigstore 无密钥签名

## 3.1 架构

```
Sigstore 生态
├── cosign          ← 容器镜像签名/验证
├── rekor           ← 透明日志 (Transparency Log)
├── fulcio          ← 短期证书颁发 (OIDC 身份)
└── gitsign         ← Git 提交签名

工作流程 (无密钥)
1. 开发者通过 OIDC 认证 (GitHub/GitLab/Google)
2. Fulcio 颁发短期证书 (10 分钟)
3. 使用证书签名制品
4. 签名记录到 Rekor 透明日志
5. 证书自动过期，无需管理密钥
```

## 3.2 安装 cosign

```bash
# 安装
curl -O -L https://github.com/sigstore/cosign/releases/latest/download/cosign-linux-amd64
chmod +x cosign-linux-amd64
sudo mv cosign-linux-amd64 /usr/local/bin/cosign

# 验证版本
cosign version
```

---

<!-- chunk: 四、Tekton Chains 构建证明 -->## 四、Tekton Chains 构建证明

## 4.1 安装

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
kubectl apply -f https://storage.googleapis.com/tekton-releases/chains/latest/release.yaml
```
## 4.2 配置

```yaml
apiVersion: v1
kind: ConfigMap
metadata:
  name: chains-config
  namespace: tekton-chains
data:
  # 签名格式
  artifacts.taskrun.format: in-toto
  artifacts.taskrun.storage: oci
  artifacts.oci.format: simplesigning
  artifacts.oci.storage: oci
  
  # 签名方式 (keyless)
  signers.x509.fulcio.enabled: "true"
  signers.x509.fulcio.address: https://fulcio.sigstore.dev
  signers.x509.fulcio.issuer: https://token.actions.githubusercontent.com
  
  # Rekor 透明日志
  transparency.enabled: "true"
  transparency.url: https://rekor.sigstore.dev
```

## 4.3 Pipeline 中的 SBOM + 签名

```yaml
apiVersion: tekton.dev/v1
kind: Pipeline
metadata:
  name: secure-build
  namespace: cicd
spec:
  tasks:
  - name: build
    taskRef:
      name: kaniko-build
  
  - name: generate-sbom
    runAfter: [build]
    taskRef:
      name: syft-generate-sbom
    params:
    - name: image
      value: $(tasks.build.results.image-digest)
  
  - name: sign-image
    runAfter: [generate-sbom]
    taskRef:
      name: cosign-sign
    params:
    - name: image
      value: $(tasks.build.results.image-digest)
    - name: keyless
      value: "true"
```

---

<!-- chunk: 五、cosign 镜像签名与验证 -->## 五、cosign 镜像签名与验证

## 5.1 Keyless 签名 (推荐)

```bash
# 签名镜像 (使用 OIDC 身份)
cosign sign --yes myregistry/app:v1.0.0

# 签名并附加 SBOM
cosign attest --predicate sbom.json \
  --type spdxjson \
  --yes myregistry/app:v1.0.0

# 验证签名
cosign verify myregistry/app:v1.0.0 \
  --certificate-identity=user@example.com \
  --certificate-oidc-issuer=https://accounts.google.com
```

## 5.2 在 K8s 中验证 (Kyverno)

```yaml
apiVersion: kyverno.io/v1
kind: ClusterPolicy
metadata:
  name: verify-image-signature
spec:
  validationFailureAction: Enforce
  background: false
  webhookTimeoutSeconds: 30
  failurePolicy: Fail
  rules:
  - name: verify-cosign-signature
    match:
      resources:
        kinds:
        - Pod
    verifyImages:
    - imageReferences:
      - "myregistry/*"
      attestors:
      - entries:
        - keyless:
            issuer: "https://token.actions.githubusercontent.com"
            subject: "https://github.com/myorg/.github/workflows/build.yml@refs/heads/main"
      attestations:
      - predicateType: https://spdx.dev/Document
        conditions:
        - all:
          - key: "$.packages[0].name"
            operator: Equals
            value: "myapp"
```

## 5.3 在 K8s 中验证 (OPA Gatekeeper)

```yaml
apiVersion: templates.gatekeeper.sh/v1
kind: ConstraintTemplate
metadata:
  name: k8srequiredcosignsignature
spec:
  crd:
    spec:
      names:
        kind: K8sRequiredCosignSignature
  targets:
  - target: admission.k8s.gatekeeper.sh
    rego: |
      package k8srequiredcosignsignature
      
      violation[{"msg": msg}] {
        container := input.review.object.spec.containers[_]
        image := container.image
        not cosign_verified(image)
        msg := sprintf("镜像 %v 缺少有效的 cosign 签名", [image])
      }
      
      cosign_verified(image) {
        # 调用外部数据进行验证
        data.external.cosign.verify(image)
      }
```

---

<!-- chunk: 六、SBOM 生成与管理 -->## 六、SBOM 生成与管理

## 6.1 生成 SBOM

```bash
# Syft (Anchore)
syft packages myregistry/app:v1.0.0 -o spdx-json > sbom.spdx.json

# Trivy
 trivy image --format spdx-json --output sbom.spdx.json myregistry/app:v1.0.0

# 生成 CycloneDX 格式
syft packages myregistry/app:v1.0.0 -o cyclonedx-json > sbom.cyclonedx.json
```

## 6.2 存储与分发

```bash
# 将 SBOM 附加到镜像
cosign attach sbom --sbom sbom.spdx.json myregistry/app:v1.0.0

# 下载 SBOM
cosign download sbom myregistry/app:v1.0.0
```

## 6.3 SBOM 格式对比

| 格式 | 标准组织 | 特点 | 推荐场景 |
|:---|:---|:---|:---|
| SPDX | Linux Foundation | 最广泛支持 | 通用合规 |
| CycloneDX | OWASP | 轻量级，安全聚焦 | 安全审计 |
| SWID | ISO/IEC | 软件标识 | 企业资产管理 |

---

<!-- chunk: 七、策略引擎验证 -->## 七、策略引擎验证

## 7.1 Kyverno 镜像签名验证 (完整示例)

```yaml
apiVersion: kyverno.io/v1
kind: ClusterPolicy
metadata:
  name: supply-chain-security
spec:
  validationFailureAction: Enforce
  rules:
  # 规则 1: 必须签名
  - name: require-signed-images
    match:
      resources:
        kinds: [Pod]
    verifyImages:
    - imageReferences: ["*"]
      required: true
      mutateDigest: true
      attestors:
      - entries:
        - keyless:
            issuer: "https://token.actions.githubusercontent.com"
            subject: "*"
      attestations:
      - predicateType: https://slsa.dev/provenance/v0.2
  
  # 规则 2: 禁止 latest 标签
  - name: disallow-latest-tag
    match:
      resources:
        kinds: [Pod]
    validate:
      message: "禁止使用 latest 标签"
      pattern:
        spec:
          containers:
          - image: "!*:latest"
  
  # 规则 3: 必须包含 SBOM
  - name: require-sbom-attestation
    match:
      resources:
        kinds: [Pod]
    verifyImages:
    - imageReferences: ["myregistry/*"]
      attestations:
      - predicateType: https://spdx.dev/Document
        conditions:
        - all:
          - key: "$.packages"
            operator: Exists
```

---

<!-- chunk: 八、SLSA 等级提升路径 -->## 八、SLSA 等级提升路径

```
当前状态
    |
    ▼
SLSA Level 1 (基础)
├── 记录构建来源
└── 工具: 手动文档 + CI/CD 日志
    |
    ▼
SLSA Level 2 (推荐起点)
├── 托管构建 + 签名来源
├── 工具: Tekton + Tekton Chains + Sigstore
└── 工作量: 1-2 周
    |
    ▼
SLSA Level 3 (企业目标)
├── 防篡改构建 + 审计日志
├── 工具: 隔离构建环境 + 不可变日志
└── 工作量: 1-2 月
    |
    ▼
SLSA Level 4 (最高标准)
├── 双人审核 + 可复现构建
├── 工具: 强制 Code Review + 确定性构建
└── 工作量: 3-6 月
```

---

<!-- chunk: 九、参考架构 -->## 九、参考架构

```
安全供应链完整流水线

Developer Push
    |
    ▼
Source Code (Git)
    ├── Branch Protection (强制 PR + Review)
    ├── CODEOWNERS
    └── Signed Commits (gitsign)
    |
    ▼
CI/CD Pipeline (Tekton)
    ├── 隔离构建环境 (隔离 Namespace)
    ├── 确定性构建 (锁定依赖版本)
    ├── 生成 SBOM (Syft/Trivy)
    ├── 漏洞扫描 (Trivy/Snyk)
    ├── 签名镜像 (cosign keyless)
    ├── 生成 SLSA Provenance (Tekton Chains)
    └── 推送至镜像仓库
    |
    ▼
镜像仓库 (Harbor + cosign)
    ├── 签名验证
    ├── 漏洞扫描 (Trivy)
    └── 不可变标签
    |
    ▼
K8s 准入控制
    ├── Kyverno/OPA 验证签名
    ├── 验证 SBOM 存在
    ├── 拒绝未签名镜像
    └── 审计日志
    |
    ▼
运行时安全
    ├── Falco 异常检测
    └── 持续监控
```

---

<!-- chunk: 参考链接 -->## 参考链接

- [SLSA 官方](https://slsa.dev/)
- [Sigstore 文档](https://docs.sigstore.dev/)
- [cosign GitHub](https://github.com/sigstore/cosign)
- [Tekton Chains](https://tekton.dev/docs/chains/)
- [SPDX 标准](https://spdx.dev/)
- [CycloneDX](https://cyclonedx.org/)
- [OpenSSF](https://openssf.org/)

---

<!-- chunk: Obsidian 相关文档 -->## Obsidian 相关文档

- 安全 KUDIG Database — Global MOC
- [[安全/README.md|Domain 05: 供应链安全 (Supply Chain Security)]]
- [[安全/00-open-source-projects-index.md|Domain-39 供应链安全 — 开源项目索引]]
- 供应链安全概述 (Supply Chain Security Overview)
- 供应链安全成熟度模型 (Supply Chain Security Maturity Model)
- SBOM 生成与管理 (SBOM Generation and Management)
- SBOM 漏洞分析与治理 (SBOM Vulnerability Analysis and Governance)
- SLSA 级别与实施 (SLSA Levels and Implementation)
- GitHub Actions SLSA 构建 (GitHub Actions SLSA Build)
- Sigstore 与 Cosign 签名 (Sigstore and Cosign Signing)
- Fulcio 与 Rekor 透明日志 (Fulcio and Rekor Transparency Logs)
- Policy Controller 镜像验证 (Policy Controller Image Verification...

## See Also

- 09-policy-controller-verification
- 10-compliance-automation-audit
- 01-supply-chain-security-overview
- 02-supply-chain-maturity-model

- [[安全/README.md|返回目录]]
```

<!-- risk-assessed -->
