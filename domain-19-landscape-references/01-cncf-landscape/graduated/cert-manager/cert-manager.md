---
title: cert-manager
description: 'description: ''## 项目概述'''
category: general
tags:
- cncf
- ecosystem
- helm
- ingress
- crd
- webhook
- operator
- agent
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 15min
intent_queries:
- cert-manager 是什么
- 如何 cert-manager
- Kubernetes 19 landscape references 最佳实践
trigger_keywords:
- cert-manager
- landscape
- references
prerequisites:
- kubectl-basics
- cncf-ecosystem
- helm-basics
- tls-basics
---

title: cert-manager
description: '## 项目概述'
category: cncf-landscape
tags:
- k8s
- cncf
- cloud-native
- ecosystem
- helm
- ingress
- crd
- webhook
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 架构师
- 技术决策者
- SRE
estimated_read_time: 5min
intent_queries:
- cert-manager 是什么
- 如何 cert-manager
- Kubernetes 34 cncf landscape 最佳实践
trigger_keywords:
- cert-manager
- cncf
- landscape
authors:
- name: KUDIG Team
  role: contributor
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
---

# cert-manager

> **成熟度**: Graduated | **加入时间**: 2020-11 | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官方网站** | https://cert-manager.io |
| **GitHub** | https://github.com/cert-manager/cert-manager |
| **文档** | https://cert-manager.io/docs |
| **许可证** | Apache-2.0 |
| **主要语言** | Go |
| **CNCF 分类** | Security |

---

## 项目概述

### 简介
cert-manager 是 Kubernetes 原生的证书管理控制器，自动化 X.509 证书的颁发、续期和管理。它支持多种证书颁发机构(CA)，包括 Let's Encrypt、HashiCorp Vault、Venafi 以及自签名证书。

### 发展历程
| 时间 | 里程碑 |
|:---|:---|
| 2017 | Jetstack 创建 cert-manager |
| 2020-11 | 加入 CNCF Sandbox |
| 2022-10 | 晋升为 CNCF Incubating |
| 2024-10 | 晋升为 CNCF Graduated |

### 核心定位
cert-manager 是 Kubernetes 生态中 TLS 证书管理的事实标准，使 HTTPS 配置变得简单自动化，是保护集群内外通信安全的关键组件。

---

## 架构设计

### 整体架构

```
┌─────────────────────────────────────────────────────────────────┐
│                   cert-manager 架构                              │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  ┌─────────────────────────────────────────────────────────────┐│
│  │                    Kubernetes Cluster                        ││
│  │                                                              ││
│  │  ┌──────────────────────────────────────────────────────┐   ││
│  │  │                 cert-manager                          │   ││
│  │  │  ┌─────────────┐ ┌─────────────┐ ┌─────────────┐     │   ││
│  │  │  │ Controller  │ │  Webhook    │ │ CA Injector │     │   ││
│  │  │  │ Manager     │ │ (验证/转换) │ │ (CA 注入)   │     │   ││
│  │  │  └──────┬──────┘ └─────────────┘ └─────────────┘     │   ││
│  │  │         │                                             │   ││
│  │  │         ▼                                             │   ││
│  │  │  ┌─────────────────────────────────────────────────┐ │   ││
│  │  │  │              CRD Resources                       │ │   ││
│  │  │  │ ┌───────────┐ ┌───────────┐ ┌───────────┐      │ │   ││
│  │  │  │ │Certificate│ │  Issuer   │ │Certificate│      │ │   ││
│  │  │  │ │           │ │(Cluster)  │ │  Request  │      │ │   ││
│  │  │  │ └───────────┘ └───────────┘ └───────────┘      │ │   ││
│  │  │  └─────────────────────────────────────────────────┘ │   ││
│  │  └──────────────────────────────────────────────────────┘   ││
│  │                            │                                 ││
│  │                            │                                 ││
│  │         ┌──────────────────┼──────────────────┐             ││
│  │         ▼                  ▼                  ▼             ││
│  │  ┌─────────────┐    ┌─────────────┐    ┌─────────────┐     ││
│  │  │Let's Encrypt│    │   Vault     │    │ Self-Signed │     ││
│  │  │   (ACME)    │    │   (PKI)     │    │     CA      │     ││
│  │  └─────────────┘    └─────────────┘    └─────────────┘     ││
│  │                                                              ││
│  │  ┌─────────────────────────────────────────────────────────┐││
│  │  │                     使用证书                             │││
│  │  │  ┌───────────┐  ┌───────────┐  ┌───────────┐           │││
│  │  │  │  Ingress  │  │  Service  │  │   Pod     │           │││
│  │  │  │   (TLS)   │  │   Mesh    │  │  (mTLS)   │           │││
│  │  │  └───────────┘  └───────────┘  └───────────┘           │││
│  │  └─────────────────────────────────────────────────────────┘││
│  └─────────────────────────────────────────────────────────────┘│
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

### 证书颁发流程

```
┌─────────────────────────────────────────────────────────────────┐
│                   证书颁发流程                                   │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  1. 用户创建                   2. cert-manager 处理             │
│  ┌───────────────┐            ┌───────────────┐                │
│  │  Certificate  │ ────────►  │ Certificate   │                │
│  │  (期望状态)   │            │ Request (CR)  │                │
│  └───────────────┘            └───────┬───────┘                │
│                                       │                         │
│  3. 与 CA 交互                        ▼                         │
│  ┌───────────────┐            ┌───────────────┐                │
│  │    Issuer     │ ◄────────  │   Issuer      │                │
│  │ (Let's Encrypt)│           │   Controller  │                │
│  └───────┬───────┘            └───────────────┘                │
│          │                                                      │
│  4. 验证域名所有权                                              │
│          ▼                                                      │
│  ┌───────────────┐  DNS-01    ┌───────────────┐                │
│  │ ACME Challenge│ ─────────► │   DNS Record  │                │
│  │               │  HTTP-01   │   或 HTTP     │                │
│  └───────┬───────┘            └───────────────┘                │
│          │                                                      │
│  5. 颁发证书                                                    │
│          ▼                                                      │
│  ┌───────────────┐            ┌───────────────┐                │
│  │  TLS Secret   │ ────────►  │   Ingress     │                │
│  │ (tls.crt/key) │            │   使用证书    │                │
│  └───────────────┘            └───────────────┘                │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

---

## 安装部署

```bash
# 使用 kubectl 安装
kubectl apply -f https://github.com/cert-manager/cert-manager/releases/download/v1.14.0/cert-manager.yaml

# 或使用 Helm
helm repo add jetstack https://charts.jetstack.io
helm repo update

helm install cert-manager jetstack/cert-manager \
  --namespace cert-manager \
  --create-namespace \
  --set installCRDs=true

# 验证安装
kubectl get pods -n cert-manager
```

---

## 核心资源

### Issuer / ClusterIssuer

```yaml
# Let's Encrypt 生产环境
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
      # HTTP-01 验证 (需要 80 端口可访问)
      - http01:
          ingress:
            class: nginx
      
      # DNS-01 验证 (支持通配符证书)
      - dns01:
          cloudflare:
            email: admin@example.com
            apiTokenSecretRef:
              name: cloudflare-api-token
              key: api-token
        selector:
          dnsZones:
            - "example.com"

---
# 自签名 CA
apiVersion: cert-manager.io/v1
kind: ClusterIssuer
metadata:
  name: selfsigned-ca
spec:
  selfSigned: {}

---
# HashiCorp Vault
apiVersion: cert-manager.io/v1
kind: ClusterIssuer
metadata:
  name: vault-issuer
spec:
  vault:
    server: https://vault.example.com
    path: pki/sign/my-role
    auth:
      kubernetes:
        role: cert-manager
        mountPath: /v1/auth/kubernetes
        secretRef:
          name: vault-token
          key: token
```

### Certificate

```yaml
apiVersion: cert-manager.io/v1
kind: Certificate
metadata:
  name: example-com
  namespace: default
spec:
  # 证书存储的 Secret 名称
  secretName: example-com-tls
  
  # 证书有效期
  duration: 2160h    # 90 天
  renewBefore: 360h  # 到期前 15 天续期
  
  # 证书主题
  subject:
    organizations:
      - Example Inc
  
  # 通用名称 (已弃用，建议用 dnsNames)
  commonName: example.com
  
  # 是否为 CA 证书
  isCA: false
  
  # 私钥配置
  privateKey:
    algorithm: RSA
    encoding: PKCS1
    size: 2048
  
  # 密钥用途
  usages:
    - server auth
    - client auth
  
  # DNS 名称
  dnsNames:
    - example.com
    - www.example.com
    - "*.example.com"  # 通配符 (需要 DNS-01)
  
  # IP 地址 (可选)
  ipAddresses:
    - 192.168.1.1
  
  # 使用的 Issuer
  issuerRef:
    name: letsencrypt-prod
    kind: ClusterIssuer
    group: cert-manager.io
```

---

## 使用场景

### 1. Ingress 自动 TLS

```yaml
# 方式 1: 通过 Ingress 注解
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: my-app
  annotations:
    # 自动创建 Certificate
    cert-manager.io/cluster-issuer: letsencrypt-prod
spec:
  ingressClassName: nginx
  tls:
    - hosts:
        - myapp.example.com
      secretName: myapp-tls
  rules:
    - host: myapp.example.com
      http:
        paths:
          - path: /
            pathType: Prefix
            backend:
              service:
                name: my-app
                port:
                  number: 80
```

### 2. 服务网格 mTLS

```yaml
# 为服务间通信创建客户端证书
apiVersion: cert-manager.io/v1
kind: Certificate
metadata:
  name: api-client
spec:
  secretName: api-client-tls
  issuerRef:
    name: internal-ca
    kind: ClusterIssuer
  commonName: api-client
  usages:
    - client auth
  dnsNames:
    - api-client.default.svc.cluster.local
```

### 3. Webhook 证书

```yaml
# 为 Admission Webhook 自动管理证书
apiVersion: cert-manager.io/v1
kind: Certificate
metadata:
  name: webhook-cert
  namespace: my-system
spec:
  secretName: webhook-tls
  issuerRef:
    name: selfsigned-ca
    kind: ClusterIssuer
  dnsNames:
    - my-webhook.my-system.svc
    - my-webhook.my-system.svc.cluster.local
```

---

## DNS-01 提供商

| 提供商 | 配置 |
|:---|:---|
| **Cloudflare** | `dns01.cloudflare` |
| **AWS Route53** | `dns01.route53` |
| **Google Cloud DNS** | `dns01.cloudDNS` |
| **Azure DNS** | `dns01.azureDNS` |
| **DigitalOcean** | `dns01.digitalocean` |
| **Webhook** | 自定义 DNS 提供商 |

```yaml
# AWS Route53 示例
solvers:
  - dns01:
      route53:
        region: us-west-2
        hostedZoneID: Z1234567890
        accessKeyIDSecretRef:
          name: aws-credentials
          key: access-key-id
        secretAccessKeySecretRef:
          name: aws-credentials
          key: secret-access-key
```

---

## 监控和故障排查

```bash
# 查看 Certificate 状态
kubectl get certificate -A
kubectl describe certificate example-com

# 查看 CertificateRequest
kubectl get certificaterequest -A

# 查看 Challenge (ACME 验证)
kubectl get challenges -A

# 查看 Order (ACME 订单)
kubectl get orders -A

# 检查 cert-manager 日志
kubectl logs -n cert-manager deploy/cert-manager

# 手动触发续期
kubectl annotate certificate example-com \
  cert-manager.io/issuer-name=letsencrypt-prod --overwrite
```

---

## 参考资源

- [官方文档](https://cert-manager.io/docs)
- [GitHub Repo](https://github.com/cert-manager/cert-manager)
- [CNCF 项目页面](https://www.cncf.io/projects/cert-manager/)
- [Let's Encrypt](https://letsencrypt.org/)
- [ACME 协议](https://datatracker.ietf.org/doc/html/rfc8555)

---

**维护者**: Kudig Team | **许可证**: MIT

## Related

- [[domain-17-system-foundation/topic-cheat-sheet/go.md|go]]
- [[domain-17-system-foundation/topic-cheat-sheet/helm.md|helm]]
- [[domain-17-system-foundation/topic-cheat-sheet/k8s.md|k8s]]
- [[domain-17-system-foundation/topic-cheat-sheet/git.md|git]]
- [[entities/kubernetes.md|kubernetes]]
- [[references/release-notes-security|发布说明索引 — 安全]] — Cross-reference
- [[references/kudig-man-pages-index|KUDIG Man Pages Index]] — Cross-reference
- [[synthesis/控制器模式 × Operator 模式|控制器模式 × Operator 模式]] — Cross-reference
- [[concepts/cloud-native-defense-in-depth|Cloud Native Defense in Depth]] — Cross-reference
- [[concepts/security-tool-evolution|安全工具演进]] — Cross-reference
- [[skills/ts-ai-ml-workloads|AI/ML 工作负载排查]] — Cross-reference
- [[skills/ts-security-auth|安全认证故障排查]] — Cross-reference
- [[skills/Kubernetes Diagnostic Skills Overview|Kubernetes Diagnostic Skills Overview]] — Cross-reference
- [[skills/ts-control-plane|控制平面故障排查]] — Cross-reference
- [[skills/Agent Orchestration Patterns|Agent Orchestration Patterns for FTA]] — Cross-reference
- [[entities/cncf-security|CNCF 安全与合规项目全景]] — Cross-reference
- [[domain-19-landscape-references/topic-index/etcd-index|etcd 知识图谱索引]]
- [[domain-19-landscape-references/topic-index/cert-index|Certificate / TLS 证书知识图谱索引]]
- [[domain-19-landscape-references/topic-index/security-index|Security 安全知识图谱索引]]
- [[domain-19-landscape-references/topic-index/gitops-cicd-index|GitOps / CI-CD 全局索引]]
- [[domain-19-landscape-references/topic-release-notes/security/cert-manager/RELEASE-NOTES-0.12|cert-manager v0.12 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/security/cert-manager/RELEASE-NOTES-1.9|cert-manager v1.9 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/security/cert-manager/RELEASE-NOTES-0.8|cert-manager v0.8 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/security/cert-manager/RELEASE-NOTES-0.16|cert-manager v0.16 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/security/cert-manager/RELEASE-NOTES-1.18|cert-manager v1.18 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/security/cert-manager/RELEASE-NOTES-1.19|cert-manager v1.19 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/security/cert-manager/RELEASE-NOTES-0.13|cert-manager v0.13 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/security/cert-manager/RELEASE-NOTES-1.8|cert-manager v1.8 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/security/cert-manager/RELEASE-NOTES-0.9|cert-manager v0.9 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/security/cert-manager/RELEASE-NOTES-1.16|cert-manager v1.16 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/security/cert-manager/RELEASE-NOTES-1.3|cert-manager v1.3 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/security/cert-manager/RELEASE-NOTES-0.2|cert-manager v0.2 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/security/cert-manager/RELEASE-NOTES-1.7|cert-manager v1.7 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/security/cert-manager/RELEASE-NOTES-1.12|cert-manager v1.12 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/security/cert-manager/RELEASE-NOTES-0.6|cert-manager v0.6 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/security/cert-manager/RELEASE-NOTES-1.6|cert-manager v1.6 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/security/cert-manager/RELEASE-NOTES-1.13|cert-manager v1.13 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/security/cert-manager/RELEASE-NOTES-0.7|cert-manager v0.7 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/security/cert-manager/RELEASE-NOTES-1.17|cert-manager v1.17 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/security/cert-manager/RELEASE-NOTES-1.2|cert-manager v1.2 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/security/cert-manager/RELEASE-NOTES-0.3|cert-manager v0.3 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/security/cert-manager/RELEASE-NOTES-1.5|cert-manager v1.5 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/security/cert-manager/RELEASE-NOTES-1.10|cert-manager v1.10 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/security/cert-manager/RELEASE-NOTES-0.4|cert-manager v0.4 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/security/cert-manager/RELEASE-NOTES-1.14|cert-manager v1.14 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/security/cert-manager/RELEASE-NOTES-1.20|cert-manager v1.20 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/security/cert-manager/RELEASE-NOTES-1.15|cert-manager v1.15 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/security/cert-manager/RELEASE-NOTES-1.0|cert-manager v1.0 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/security/cert-manager/RELEASE-NOTES-1.4|cert-manager v1.4 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/security/cert-manager/RELEASE-NOTES-1.11|cert-manager v1.11 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/security/cert-manager/RELEASE-NOTES-0.5|cert-manager v0.5 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/security/cert-manager/RELEASE-NOTES-0.14|cert-manager v0.14 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/security/cert-manager/RELEASE-NOTES-0.10|cert-manager v0.10 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/security/cert-manager/RELEASE-NOTES-0.11|cert-manager v0.11 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/security/cert-manager/RELEASE-NOTES-0.15|cert-manager v0.15 Release Notes]]
