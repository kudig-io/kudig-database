# Ratify

> **成熟度**: Sandbox | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官网** | https://ratify.dev/ |
| **GitHub** | https://github.com/ratify-project/ratify |
| **许可证** | Apache-2.0 |
| **开发语言** | Go |
| **CNCF 状态** | Sandbox |

---

## 项目概述

Ratify 是一个容器镜像供应链安全验证框架，用作 Kubernetes 准入控制器，在 Pod 创建时验证容器镜像的签名、SBOM、漏洞扫描报告等供应链工件（Artifacts）。它与 Gatekeeper/OPA 集成，通过可插拔的验证器架构支持 Notary v2 签名、Cosign 签名、SBOM 验证、漏洞报告检查等多种供应链安全策略。

### 核心特性

- **签名验证**: 支持 Notary v2 和 Cosign 镜像签名验证
- **SBOM 验证**: 验证镜像附带的 SPDX/CycloneDX SBOM
- **漏洞策略**: 基于漏洞扫描报告阻止含高危漏洞的镜像
- **Gatekeeper 集成**: 作为 Gatekeeper External Data Provider 运行
- **可插拔架构**: 通过 Verifier 和 Store 插件扩展验证能力
- **OCI 标准**: 基于 OCI Reference Types 发现和验证供应链工件

---

## 架构设计

```
┌──────────────────────────────────────────────────┐
│            Kubernetes API Server                   │
│  ┌────────────────────────────────────────────┐   │
│  │     Admission Webhook (Gatekeeper)          │   │
│  │  ┌──────────────────────────────────┐      │   │
│  │  │ ConstraintTemplate (Rego)        │      │   │
│  │  │ "所有镜像必须有有效签名"          │      │   │
│  │  └──────────────┬───────────────────┘      │   │
│  └─────────────────┼──────────────────────────┘   │
└────────────────────┼──────────────────────────────┘
                     │ External Data
┌────────────────────▼──────────────────────────────┐
│               Ratify Server                         │
│                                                     │
│  ┌──────────────────────────────────────────┐      │
│  │         Executor                          │      │
│  │  (协调 Store 和 Verifier)                 │      │
│  └──────────────┬───────────────────────────┘      │
│                 │                                    │
│  ┌──────────────▼────┐  ┌───────────────────┐      │
│  │   Referrer Store  │  │    Verifiers       │      │
│  │  ┌─────────────┐  │  │  ┌──────────────┐ │      │
│  │  │ OCI Registry│  │  │  │ Notary v2    │ │      │
│  │  │ (ORAS)      │  │  │  │ Cosign       │ │      │
│  │  └─────────────┘  │  │  │ SBOM         │ │      │
│  └────────────────────┘  │  │ Vulnerability│ │      │
│                           │  └──────────────┘ │      │
│                           └───────────────────┘      │
└──────────────────────────────────────────────────────┘
```

---

## 快速开始

### 安装

```bash
# 安装 Gatekeeper
helm repo add gatekeeper https://open-policy-agent.github.io/gatekeeper/charts
helm install gatekeeper gatekeeper/gatekeeper \
  --namespace gatekeeper-system \
  --create-namespace \
  --set enableExternalData=true

# 安装 Ratify
helm repo add ratify https://ratify-project.github.io/ratify
helm install ratify ratify/ratify \
  --namespace gatekeeper-system \
  --set featureFlags.RATIFY_CERT_ROTATION=true
```

### 配置签名验证

```yaml
# 配置 Notary v2 验证器
apiVersion: config.ratify.deislabs.io/v1beta1
kind: Verifier
metadata:
  name: verifier-notary
spec:
  name: notaryv2
  artifactTypes: application/vnd.cncf.notary.signature
  parameters:
    verificationCertStores:
      ca:
        - ratify-notation-inline-cert
    trustPolicyDoc:
      version: "1.0"
      trustPolicies:
        - name: default
          registryScopes:
            - "*"
          signatureVerification:
            level: strict
          trustStores:
            - ca:ratify-notation-inline-cert
          trustedIdentities:
            - "*"

---
# 配置 Cosign 验证器
apiVersion: config.ratify.deislabs.io/v1beta1
kind: Verifier
metadata:
  name: verifier-cosign
spec:
  name: cosign
  artifactTypes: application/vnd.dev.cosign.artifact.sig.v1+json
  parameters:
    key: /usr/local/ratify-certs/cosign/cosign.pub
```

### Gatekeeper 约束

```yaml
apiVersion: templates.gatekeeper.sh/v1
kind: ConstraintTemplate
metadata:
  name: ratifyverification
spec:
  crd:
    spec:
      names:
        kind: RatifyVerification
  targets:
    - target: admission.k8s.gatekeeper.sh
      rego: |
        package ratifyverification
        violation[{"msg": msg}] {
          input.review.object.kind == "Pod"
          images := [img | img = input.review.object.spec.containers[_].image]
          image := images[_]
          response := external_data({"provider": "ratify-provider", "keys": [image]})
          response.system_error != ""
          msg := sprintf("Ratify error: %v", [response.system_error])
        }
        violation[{"msg": msg}] {
          input.review.object.kind == "Pod"
          images := [img | img = input.review.object.spec.containers[_].image]
          image := images[_]
          response := external_data({"provider": "ratify-provider", "keys": [image]})
          result := response.items[_]
          result.error != ""
          msg := sprintf("Image %v failed verification: %v", [image, result.error])
        }

---
apiVersion: constraints.gatekeeper.sh/v1beta1
kind: RatifyVerification
metadata:
  name: require-signed-images
spec:
  enforcementAction: deny
  match:
    kinds:
      - apiGroups: [""]
        kinds: ["Pod"]
    namespaces: ["production"]
```

---

## 与其他方案对比

| 特性 | Ratify | Kyverno | Connaisseur | Sigstore policy-controller |
|:---|:---|:---|:---|:---|
| 签名验证 | Notary v2 + Cosign | Cosign/Notary | Notary v1 | Cosign |
| SBOM 验证 | 支持 | 不支持 | 不支持 | 不支持 |
| 漏洞策略 | 支持 | 不支持 | 不支持 | 不支持 |
| 策略引擎 | Gatekeeper/OPA | 内置 | 内置 | 内置 |
| OCI Reference | 支持 | 有限 | 不支持 | 有限 |
| 可扩展性 | 插件架构 | 策略规则 | 有限 | 有限 |

---

## 最佳实践

1. **渐进实施**: 先用 dryrun 模式观察，确认无误后再切换到 deny 模式
2. **多验证器**: 组合使用签名验证 + SBOM 验证 + 漏洞检查实现深度防御
3. **证书管理**: 使用 Kubernetes Secret 管理验证证书，配置自动轮换
4. **命名空间隔离**: 先在生产命名空间启用强制验证，逐步扩展
5. **缓存配置**: 合理配置 Ratify 的验证结果缓存，平衡安全性和性能

---

## 参考资源

- [Ratify 官方文档](https://ratify.dev/docs/)
- [Ratify GitHub](https://github.com/ratify-project/ratify)
- [Notary v2 项目](https://github.com/notaryproject/notation)
- [CNCF Sandbox Projects](https://www.cncf.io/sandbox-projects/)

---

**维护者**: Kudig Team | **许可证**: MIT
