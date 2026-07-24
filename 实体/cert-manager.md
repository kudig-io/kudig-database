---
title: cert-manager (entities)
description: '## 概述'
summary: '该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[概念/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]'
category: entities
tags:
- k8s
- cncf
- security
- cert-manager
- envoy
- crd
- operator
- agent
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- cert-manager 是什么
- 如何 cert-manager
trigger_keywords:
- cert-manager
prerequisites:
- kubectl-basics
- tls-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# cert-manager

> **CNCF 状态**: Graduated | **类别**: Security | **主要语言**: Go

## 概述

description: '## 项目概述'

## 核心能力

- 详见源文档获取完整信息 ^[inferred]

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[概念/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- 建议参考官方文档获取最新部署指南 ^[inferred]

## 架构定位

cert-manager 是 Kubernetes 原生的证书管理控制器，自动签发、续期、管理 TLS 证书，支持 Let's Encrypt、Vault、自签 CA 等多种 Issuer。

## 架构设计

```
┌─────────────────────────────────────────────────────┐
│              cert-manager Controller                │
│                                                      │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐    │
│  │Certificate│  │  Order   │  │Challenge │    │
│  │Controller│→│Controller│→│Controller│    │
│  └──────────┘  └──────────┘  └──────────┘    │
│       │              │              │            │
│       ▼              ▼              ▼            │
│  ┌────────────────────────────────────────┐  │
│  │           Issuer / ClusterIssuer        │  │
│  │  (Let's Encrypt / Vault / CA / Self)   │  │
│  └────────────────────────────────────────┘  │
│       │                                         │
│       ▼                                         │
│  ┌────────────────────────────────────────┐  │
│  │        Secret (TLS 证书)              │  │
│  └────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────┘
```

## 核心资源

| 资源 | 作用域 | 说明 |
|------|--------|------|
| Issuer | Namespace | 命名空间级证书签发者 |
| ClusterIssuer | Cluster | 集群级证书签发者 |
| Certificate | Namespace | 证书请求 |
| Order | Namespace | ACME 订单 |
| Challenge | Namespace | ACME 挑战 |

## 安装与配置

```bash
# 🟢 Helm 安装
helm repo add jetstack https://charts.jetstack.io
helm install cert-manager jetstack/cert-manager \
  --namespace cert-manager \
  --create-namespace \
  --set crds.enabled=true

# 🟢 验证安装
kubectl get pods -n cert-manager
kubectl get crd | grep cert-manager.io
```

### Let's Encrypt ClusterIssuer

```yaml
apiVersion: cert-manager.io/v1
kind: ClusterIssuer
metadata:
  name: letsencrypt-prod
spec:
  acme:
    server: https://acme-v02.api.letsencrypt.org/directory
    email: admin@example.com
    privateKeySecretRef:
      name: letsencrypt-prod-key
    solvers:
    - http01:
        ingress:
          ingressClassName: nginx
    # 或使用 DNS01 验证 (支持通配符)
    # - dns01:
    #     route53:
    #       region: us-east-1
```

### Certificate 资源

```yaml
apiVersion: cert-manager.io/v1
kind: Certificate
metadata:
  name: app-cert
  namespace: default
spec:
  secretName: app-tls
  dnsNames:
  - app.example.com
  - "*.app.example.com"
  issuerRef:
    name: letsencrypt-prod
    kind: ClusterIssuer
  renewBefore: 720h  # 到期前 30 天续期
  duration: 2160h    # 有效期 90 天
  privateKey:
    algorithm: ECDSA
    size: 256
```

### 自签 CA (内部服务)

```yaml
# 根 CA
apiVersion: cert-manager.io/v1
kind: ClusterIssuer
metadata:
  name: selfsigned-issuer
spec:
  selfSigned: {}
---
# CA 证书
apiVersion: cert-manager.io/v1
kind: Certificate
metadata:
  name: internal-ca
  namespace: cert-manager
spec:
  isCA: true
  commonName: internal-ca
  secretName: internal-ca-secret
  issuerRef:
    name: selfsigned-issuer
    kind: ClusterIssuer
---
# CA Issuer
apiVersion: cert-manager.io/v1
kind: ClusterIssuer
metadata:
  name: internal-ca-issuer
spec:
  ca:
    secretName: internal-ca-secret
```

## 运维操作

### 常用命令

```bash
# 🟢 查看证书状态
kubectl get certificates -A
kubectl get certificaterequests -A
kubectl get orders -A
kubectl get challenges -A

# 🟢 查看证书详情
kubectl describe certificate app-cert -n default

# 🟢 查看 Issuer 状态
kubectl get clusterissuers
kubectl describe clusterissuer letsencrypt-prod

# 🟢 查看证书 Secret
kubectl get secret app-tls -o yaml
kubectl get secret app-tls -o jsonpath='{.data.tls\.crt}' | base64 -d | openssl x509 -noout -dates

# 🟢 查看 cert-manager 日志
kubectl logs -n cert-manager -l app=cert-manager --tail=50

# 🟡 强制续期
kubectl cert-manager renew app-cert -n default

# 🟡 删除并重建证书
kubectl delete certificate app-cert -n default
kubectl apply -f certificate.yaml
```

## 故障排查

| 问题 | 原因 | 解决方案 |
|------|------|----------|
| Certificate Pending | Issuer 未就绪 | 检查 Issuer 状态 |
| Order Failed | ACME 验证失败 | 检查 Challenge |
| Challenge Failed | DNS/HTTP 验证不通过 | 检查 DNS/Ingress |
| 续期失败 | Let's Encrypt 限流 | 等待/使用 staging |
| Secret 未创建 | 权限不足 | 检查 RBAC |
| 证书过期 | renewBefore 太短 | 调整 renewBefore |

### 排查流程

```
1. 检查 Certificate 状态
   kubectl describe certificate <name>
       │
2. 检查 CertificateRequest
   kubectl get certificaterequests
       │
3. 检查 Order
   kubectl describe order <name>
       │
4. 检查 Challenge
   kubectl describe challenge <name>
       │
5. 检查 Issuer
   kubectl describe clusterissuer <name>
```

## 检查清单

- [ ] 理解 cert-manager 架构
- [ ] 能配置 Let's Encrypt Issuer
- [ ] 掌握 Certificate 资源配置
- [ ] 能排查证书签发失败
- [ ] 理解自动续期机制
- [ ] 了解自签 CA 配置

## 参考链接

- [[实体/vault.md|HashiCorp Vault]]
- [[实体/crd-custom-resources.md|CRD]]
- [[概念/controller-pattern.md|Controller Pattern]]
- [[概念/secrets-management.md|Secrets Management]]

## Related

- [[litmus]] — LitmusChaos
- [[pixie]] — Pixie
- [[dapr]] — Dapr
- [[envoy]] — Envoy
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- 99-cert-manager-tls-guide
- cert-manager
- RELEASE-NOTES-0.12
- RELEASE-NOTES-1.9
- RELEASE-NOTES-0.8
- RELEASE-NOTES-0.16
- RELEASE-NOTES-1.18
- RELEASE-NOTES-1.19
- RELEASE-NOTES-0.13
- RELEASE-NOTES-1.8
- RELEASE-NOTES-0.9
- RELEASE-NOTES-1.16
- RELEASE-NOTES-1.3
- RELEASE-NOTES-0.2
- RELEASE-NOTES-1.7
- RELEASE-NOTES-1.12
- RELEASE-NOTES-0.6
- RELEASE-NOTES-1.6
- RELEASE-NOTES-1.13
- RELEASE-NOTES-0.7
- RELEASE-NOTES-1.17
- RELEASE-NOTES-1.2
- RELEASE-NOTES-0.3
- RELEASE-NOTES-1.5
- RELEASE-NOTES-1.10
- RELEASE-NOTES-0.4
- RELEASE-NOTES-1.14
- RELEASE-NOTES-1.1
- RELEASE-NOTES-1.20
- RELEASE-NOTES-1.15
- RELEASE-NOTES-1.0
- RELEASE-NOTES-0.1
- RELEASE-NOTES-1.4
- RELEASE-NOTES-1.11
- RELEASE-NOTES-0.5
- RELEASE-NOTES-0.14
- RELEASE-NOTES-0.10
- RELEASE-NOTES-0.11
- RELEASE-NOTES-0.15
- [[实体/kubearmor.md|KubeArmor]]
- [[实体/openfga.md|OpenFGA]]
- [[实体/release-notes-security.md|发布说明索引 — 安全]] — Cross-reference
- [[实体/kudig-man-pages-index.md|KUDIG Man Pages Index]] — Cross-reference
- [[概念/控制器模式 × Operator 模式.md|控制器模式 × Operator 模式]] — Cross-reference
- [[概念/cloud-native-defense-in-depth.md|Cloud Native Defense in Depth]] — Cross-reference
- [[概念/security-tool-evolution.md|安全工具演进]] — Cross-reference
- [[技能/节点/gpu/诊断排障/ts-ai-ml-workloads.md|AI/ML 工作负载排查]] — Cross-reference
- [[技能/安全/rbac/诊断排障/ts-security-auth.md|安全认证故障排查]] — Cross-reference
- [[技能/工作负载/pod/方法论/Kubernetes Diagnostic Skills Overview.md|Kubernetes Diagnostic Skills Overview]] — Cross-reference
- [[技能/控制面/apiserver/诊断排障/ts-control-plane.md|控制平面故障排查]] — Cross-reference
- [[技能/工作负载/pod/方法论/agent/Agent Orchestration Patterns.md|Agent Orchestration Patterns for FTA]] — Cross-reference
- [[实体/cncf-security.md|CNCF 安全与合规项目全景]] — Cross-reference
- [[生态参考/领域索引/etcd-index.md|etcd 知识图谱索引]]
- [[生态参考/领域索引/cert-index.md|Certificate / TLS 证书知识图谱索引]]
- [[生态参考/领域索引/security-index.md|Security 安全知识图谱索引]]
- [[生态参考/领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
