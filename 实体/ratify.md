---
title: Ratify (entities)
description: '## 概述'
summary: 'Ratify 是一个容器镜像供应链安全验证框架，用作 Kubernetes 准入控制器，在 Pod 创建时验证容器镜像的签名、SBOM、漏洞扫描报告等供应链工件（Artifacts）。它与 Gatekeeper/OPA 集成，通过可插拔的验证器架构支持 Notary v2 签名、Cosign 签名、SBOM 验证、漏洞报告检查等多种供应链安全策略。'
category: entities
tags:
- k8s
- cncf
- supply-chain
- ratify
- opa
- crd
- operator
- wasm
- agent
tier: supporting
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Ratify 是什么
- 如何 Ratify
trigger_keywords:
- Ratify
prerequisites:
- kubectl-basics
- policy-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# Ratify

> **CNCF 状态**: Sandbox | **类别**: Supply Chain | **主要语言**: Go

## 概述

Ratify 是由 Microsoft（Azure Container Registry 团队）开发的开源**供应链安全验证框架**，2022 年进入 CNCF Sandbox。它作为 Kubernetes 的**动态准入控制器（Admission Controller）**，在 Pod 创建时自动验证容器镜像的供应链安全工件——包括 Notary v2 签名、Cosign 签名、SBOM（Software Bill of Materials）、漏洞扫描报告等。只有通过所有验证策略的镜像才被允许部署。

Ratify 与 **OPA Gatekeeper** 深度集成。Gatekeeper 作为策略引擎，Ratify 作为外部数据提供者（External Data Provider），在准入阶段查询镜像的验证结果。这种分层设计使策略规则（由 Gatekeeper 管理）与验证逻辑（由 Ratify 管理）解耦。Ratify 通过**可插拔的验证器（Verifier）架构**支持多种签名和工件格式。

## Key Features

- **准入控制验证**：Pod 创建时自动验证镜像签名和供应链工件
- **多签名格式**：支持 Notary v2（notation）、Cosign、Notation 签名
- **SBOM 验证**：检查镜像的 Software Bill of Materials（SPDX/CycloneDX）
- **漏洞扫描集成**：验证 Trivy/Grype 漏洞扫描报告
- **Gatekeeper/OPA 集成**：作为外部数据提供者与 Gatekeeper 策略引擎协同
- **可插拔验证器**：自定义验证器通过 Wasm 或 Go 插件实现

## Architecture

Ratify 由 **Ratify Agent**（运行在每个节点的验证服务，缓存验证结果）、**Gatekeeper**（OPA 策略引擎，调用 Ratify 获取验证数据）、**Verifier Store**（存储签名证书和验证配置）和 **Certificate Store**（管理用于验证签名的公钥证书）组成。当 Pod 创建请求到达 API Server 时，Gatekeeper 准入 Webhook 触发，通过 External Data API 向 Ratify 查询镜像验证状态，Ratify 运行配置的验证器检查签名/SBOM/漏洞报告，返回 allow/deny 决策。

## K8s 集成

Ratify 通过 Gatekeeper 的 External Data 机制与 Kubernetes API Server 集成。部署流程：安装 Gatekeeper → 安装 Ratify（Helm）→ 创建 `Store` CRD 配置验证器和证书 → 创建 Gatekeeper `Constraint` 启用镜像验证策略。Pod 创建时自动触发验证，验证失败的请求被 API Server 拒绝。

## 生产部署要点

- **渐进实施**：先用 dryrun 模式观察，确认无误后再切换到 deny 模式
- **多验证器**：组合使用签名验证 + SBOM 验证 + 漏洞检查实现深度防御
- **证书管理**：使用 Kubernetes Secret 管理验证证书，配置自动轮换
- **命名空间隔离**：先在生产命名空间启用强制验证，逐步扩展
- **缓存配置**：合理配置 Ratify 的验证结果缓存，平衡安全性和性能

## 生产场景

1. **镜像签名强制**：生产环境只允许部署经过 Cosign/Notary 签名的镜像
2. **SBOM 合规**：要求所有镜像必须附带有效的 SBOM（SPDX 格式）
3. **漏洞准入**：拒绝部署存在严重（Critical/High）CVE 的镜像
4. **供应链审计**：记录所有镜像验证决策，满足 SLSA/SSDF 合规审计

## 安装

```bash
# 前提：需要已安装 OPA Gatekeeper
helm repo add gatekeeper https://open-policy-agent.github.io/gatekeeper/charts
helm install gatekeeper gatekeeper/gatekeeper -n gatekeeper-system --create-namespace

# 安装 Ratify
helm repo add ratify https://deislabs.github.io/ratify
helm install ratify ratify/ratify -n gatekeeper-system \
  --set featureFlags.AlphaGatekeeperExternalData=true

# 配置签名验证（Cosign）
kubectl apply -f - <<EOF
apiVersion: config.ratify.deislabs.io/v1alpha1
spec:
  store:
    name: oras
    parameters:
      cosignEnabled: true
  verifier:
    - name: cosign
      artifactTypes: application/vnd.dev.cosign.artifact.sig.v1+json
      parameters:
        key: |
          -----BEGIN PUBLIC KEY-----
          MFkwEwYHKoZIzj0CAQYIKoZIzj0DAQcDQgAE...
          -----END PUBLIC KEY-----
EOF

# 创建 Gatekeeper Constraint（拒绝未签名镜像）
kubectl apply -f - <<EOF
apiVersion: constraints.gatekeeper.sh/v1beta1
kind: K8sAllowedImageSignatures
metadata:
  name: require-signed-images
spec:
  enforcementAction: deny
  match:
    kinds:
      - apiGroups: [""]
        kinds: ["Pod"]
EOF
```

## 对比

| 特性 | Ratify | Connaisseur | Kyverno（verifyImages） | Sigstore Policy Controller |
|------|--------|-------------|------------------------|-------------------------|
| 多签名格式 | ✅ Notary+Cosign | ⚠️ Cosign | ✅ Cosign | ✅ Cosign |
| Gatekeeper 集成 | ✅ | ❌ | ❌ | ❌ |
| SBOM 验证 | ✅ | ❌ | ❌ | ❌ |
| 可插拔验证器 | ✅ | ❌ | ❌ | ⚠️ |

## 参考链接

- [[kyverno]]
- [[实体/crd-custom-resources.md|crd-custom-resources]]
- [[概念/controller-pattern.md|controller-pattern]]
- [[概念/secrets-management.md|secrets-management]]
- [[pod-lifecycle]]

## Related

- [[kubevirt]] — KubeVirt
- [[wasmcloud]] — wasmCloud
- [[spiderpool]] — Spiderpool
- [[opa]] — OPA (Open Policy Agent)
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- ratify
- [[实体/cncf-security.md|[[CNCF 安全与合规项目全景|CNCF 安全与合规项目全景]]]] — Cross-reference
- [[生态参考/领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
