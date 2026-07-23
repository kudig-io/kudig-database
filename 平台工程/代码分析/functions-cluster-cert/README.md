---
title: Cluster Cert — Kubernetes 集群证书体系源码分析
description: '## 概述'
summary: 'Kubernetes 集群的认证与授权体系高度依赖 PKI（Public Key Infrastructure，公钥基础设施）。一个标准的 kubeadm 部署的集群包含超过 10 组证书/密钥对，'
category: functions
tags:
- k8s
- operations
- cluster-management
- etcd
- apiserver
- kubelet
- scheduler
- controller-manager
- prometheus
- rbac
tier: supporting
created: '2026-05-23'
last_updated: '2026-05-18'
difficulty: expert
reading_level: expert
audience:
- Kubernetes 管理员
- 安全工程师
- 集群运维人员
- DevOps 工程师
- SRE 工程师
estimated_read_time: 5min
intent_queries:
- Kubernetes 集群 PKI 证书体系总览 kubeadm 源码分析
- Kubernetes 三组 CA 架构 kubernetes-ca etcd-ca front-proxy-ca
- Kubernetes 14 组证书密钥对完整列表 路径 签发者 用途
- kubeadm init 证书阶段 CreatePKIAssets 源码
- Kubernetes 证书信任链 验证关系
trigger_keywords:
- PKI
- CA
- 证书
- kubeadm
- kubernetes-ca
- etcd-ca
- front-proxy-ca
- 证书体系
- 证书生成
- 证书轮换
prerequisites:
- kubectl-basics
- pod-lifecycle
- prometheus-basics
- etcd-basics
- gpu-scheduling-basics
related_domains:
- 集群基础
- 安全
related_topics:
- cluster-cert/ca-generation
- cluster-cert/apiserver-cert
- cluster-cert/etcd-cert
- cluster-cert/kubelet-cert
- cluster-cert/cert-rotation
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Cluster Cert — [[Kubernetes|Kubernetes]] 集群证书体系源码分析

## 概述

Kubernetes 集群的认证与授权体系高度依赖 PKI（Public Key Infrastructure，公钥基础设施）。一个标准的 kubeadm 部署的集群包含超过 10 组证书/密钥对，涵盖 API Server、[[etcd|etcd]]、[[kubelet|kubelet]]、Controller Manager、Scheduler、Front Proxy 等所有组件的身份认证。理解这些证书的生成逻辑、信任链关系和轮换机制，对于集群安全管理和故障排查至关重要。

本文档基于 Kubernetes 官方源码（`kubernetes/kubernetes`），系统梳理集群 PKI 证书的生成逻辑、证书链架构、轮换机制及安全设计。从 kubeadm 证书阶段的源码出发，深入分析每一组证书的生成过程、用途、有效期、依赖关系，以及证书轮换和续期的完整工作流。

---

## 文档索引

| 文档 | 内容 | 核心源码路径 | 建议阅读顺序 |
|------|------|-------------|-------------|
| [01-pki-architecture](01-pki-architecture.md) | PKI 架构总览：三组 CA、证书依赖关系、信任链 | `cmd/kubeadm/app/phases/certs/` | 1 |
| [02-ca-generation](02-ca-generation.md) | CA 证书生成源码分析：kubeadm CA、etcd CA、Front Proxy CA | `cmd/kubeadm/app/phases/certs/ca.go` | 2 |
| [03-apiserver-cert](03-apiserver-cert.md) | API Server 证书：SAN 生成逻辑、扩展属性、证书用途 | `cmd/kubeadm/app/phases/certs/apiserver.go` | 3 |
| [04-etcd-cert](04-etcd-cert.md) | etcd 证书体系：Server/Peer/Client 证书及健康检查证书 | `cmd/kubeadm/app/phases/certs/etcd.go` | 4 |
| [05-kubelet-cert](05-kubelet-cert.md) | kubelet 证书：引导证书、CSR 机制、自动轮换源码 | `pkg/kubelet/certificate/` | 5 |
| [06-cert-rotation](06-cert-rotation.md) | 证书轮换机制：kubeadm renew、kubelet 自动轮换、Controller | `cmd/kubeadm/app/phases/certs/renew/` | 6 |
| [07-service-account-keys](07-service-account-keys.md) | ServiceAccount 密钥对：JWT 签名、Token 验证、密钥轮换 | `pkg/serviceaccount/` | 7 |
| [08-rbac-mapping](08-rbac-mapping.md) | 证书身份到 RBAC 的映射：CommonName/Organization、front-proxy | `pkg/kube-apiserver/authorizer/rbac/` | 8 |
| [09-join-cert-flow](09-join-cert-flow.md) | kubeadm join 证书分发：Bootstrap Token、CSR、HA 证书复制 | `cmd/kubeadm/app/phases/kubeconfig/` | 9 |
| [10-front-proxy-workflow](10-front-proxy-workflow.md) | Front Proxy 聚合层完整工作流：APIService、metrics-server、安全边界 | `vendor/k8s.io/apiserver/pkg/endpoints/handlers/` | 10 |
| [11-apiserver-cert-flags](11-apiserver-cert-flags.md) | API Server 证书启动参数汇总：全量标志、验证脚本、配置陷阱 | `cmd/kube-apiserver/app/options/` | 11 |
| [12-kubeconfig-certs](12-kubeconfig-certs.md) | kubeconfig 证书嵌入逻辑：admin/controller-manager/scheduler、Base64 编码 | `cmd/kubeadm/app/phases/kubeconfig/` | 12 |
| [13-cert-config](13-cert-config.md) | kubeadm 配置对证书的影响：certSANs、CertificatesDir、controlPlaneEndpoint | `cmd/kubeadm/app/util/pkiutil/` | 13 |
| [14-admission-webhook-certs](14-admission-webhook-certs.md) | Webhook 证书体系：caBundle、cainjector、证书轮换、故障排查 | `cmd/kubeadm/app/phases/certs/` | 14 |
| [15-cert-format-encoding](15-cert-format-encoding.md) | 证书格式与编码：PEM/DER/ASN.1、X.509v3 扩展字段 | `staging/src/k8s.io/client-go/util/cert/` | 15 |
| [16-openssl-cookbook](16-openssl-cookbook.md) | OpenSSL 速查手册：查看/验证/生成/转换/调试 | 外部工具 | 16 |
| [17-pki-security-best-practices](17-pki-security-best-practices.md) | PKI 安全最佳实践：私钥保护、监控告警、CIS 合规 | 最佳实践 | 17 |

---

## PKI 架构总览

### 三组 CA

kubeadm 生成的 PKI 体系包含三组独立的 CA（Certificate Authority，证书颁发机构），每组 CA 服务于不同的安全域：

```
Kubernetes PKI 架构:
  ┌─────────────────────────────────────────────────────────────────┐
  │  kubernetes CA (ca.crt / ca.key)                                │
  │  ├── API Server 证书 (apiserver.crt / apiserver.key)           │
  │  ├── API Server kubelet 客户端证书                              │
  │  ├── API Server etcd 客户端证书                                 │
  │  ├── Controller Manager 证书                                   │
  │  ├── Scheduler 证书                                             │
  │  ├── admin 证书 (system:masters)                               │
  │  ├── kubelet 证书 (system:nodes, 通过 CSR 签发)                 │
  │  └── Front Proxy 客户端证书                                     │
  ├─────────────────────────────────────────────────────────────────┤
  │  etcd CA (etcd/ca.crt / etcd/ca.key)                            │
  │  ├── etcd Server 证书 (etcd/server.crt)                        │
  │  ├── etcd Peer 证书 (etcd/peer.crt)                             │
  │  ├── etcd Healthcheck 客户端证书 (etcd/healthcheck-client.crt)  │
  │  └── API Server etcd 客户端证书 (apiserver-etcd-client.crt)     │
  ├─────────────────────────────────────────────────────────────────┤
  │  Front Proxy CA (front-proxy-ca.crt / front-proxy-ca.key)        │
  │  ├── Front Proxy 客户端证书 (front-proxy-client.crt)             │
  │  └── 用于 API Server 聚合层 (metrics-server 等)                 │
  └─────────────────────────────────────────────────────────────────┘
```

### 证书列表

| 证书/密钥 | 路径 | 签发者 | 有效期 | 用途 |
|-----------|------|--------|--------|------|
| ca.crt/ca.key | `/etc/kubernetes/pki/` | 自签名 | 10 年 | Kubernetes CA |
| etcd/ca.crt/ca.key | `/etc/kubernetes/pki/etcd/` | 自签名 | 10 年 | etcd CA |
| front-proxy-ca.crt/ca.key | `/etc/kubernetes/pki/` | 自签名 | 10 年 | Front Proxy CA |
| apiserver.crt/key | `/etc/kubernetes/pki/` | kubernetes CA | 1 年 | API Server 服务端证书 |
| apiserver-kubelet-client.crt/key | `/etc/kubernetes/pki/` | kubernetes CA | 1 年 | API Server - kubelet |
| apiserver-etcd-client.crt/key | `/etc/kubernetes/pki/` | etcd CA | 1 年 | API Server - etcd |
| etcd/server.crt/key | `/etc/kubernetes/pki/etcd/` | etcd CA | 1 年 | etcd 服务端 |
| etcd/peer.crt/key | `/etc/kubernetes/pki/etcd/` | etcd CA | 1 年 | etcd 集群通信 |
| etcd/healthcheck-client.crt/key | `/etc/kubernetes/pki/etcd/` | etcd CA | 1 年 | etcd 健康检查 |
| front-proxy-client.crt/key | `/etc/kubernetes/pki/` | Front Proxy CA | 1 年 | 聚合层客户端 |
| sa.key/sa.pub | `/etc/kubernetes/pki/` | N/A | N/A | ServiceAccount 签名 |

---

### 核心文档链接

| 文档 | 内容 | 建议阅读顺序 |
|------|------|-------------|
| [01-pki-architecture](01-pki-architecture.md) | PKI 架构总览 | 1 |
| [02-ca-generation](02-ca-generation.md) | CA 证书生成 | 2 |
| [03-apiserver-cert](03-apiserver-cert.md) | API Server 证书 | 3 |
| [04-etcd-cert](04-etcd-cert.md) | etcd 证书 | 4 |
| [05-kubelet-cert](05-kubelet-cert.md) | kubelet 证书 | 5 |
| [06-cert-rotation](06-cert-rotation.md) | 证书轮换 | 6 |
| [07-service-account-keys](07-service-account-keys.md) | SA 密钥对 | 7 |
| [08-rbac-mapping](08-rbac-mapping.md) | 证书 RBAC 映射 | 8 |
| [09-join-cert-flow](09-join-cert-flow.md) | join 证书流程 | 9 |
| [10-front-proxy-workflow](10-front-proxy-workflow.md) | Front Proxy | 10 |
| [11-apiserver-cert-flags](11-apiserver-cert-flags.md) | API Server 启动参数 | 11 |
| [12-kubeconfig-certs](12-kubeconfig-certs.md) | kubeconfig 证书 | 12 |
| [13-cert-config](13-cert-config.md) | kubeadm 配置影响 | 13 |
| [14-admission-webhook-certs](14-admission-webhook-certs.md) | Webhook 证书 | 14 |
| [15-cert-format-encoding](15-cert-format-encoding.md) | 证书格式编码 | 15 |
| [16-openssl-cookbook](16-openssl-cookbook.md) | OpenSSL 速查 | 16 |
| [17-pki-security-best-practices](17-pki-security-best-practices.md) | 安全最佳实践 | 17 |

---

## 证书生成流程源码分析

### kubeadm init 证书阶段

```go
// cmd/kubeadm/app/cmd/phases/init/certs.go
func NewCertsPhase() workflow.Phase {
    return workflow.Phase{
        Name:  "certs",
        Short: "Certificate generation",
        Phases: []workflow.Phase{
            {Name: "ca", Run: runCACerts},                          // 生成三组 CA
            {Name: "apiserver", Run: runAPIServerCerts},            // 生成 API Server 证书
            {Name: "apiserver-kubelet-client", Run: runAPIServerKubeletClientCert},
            {Name: "front-proxy-ca", Run: runFrontProxyCACert},
            {Name: "front-proxy-client", Run: runFrontProxyClientCert},
            {Name: "etcd-ca", Run: runEtcdCACert},
            {Name: "etcd-server", Run: runEtcdServerCert},
            {Name: "etcd-peer", Run: runEtcdPeerCert},
            {Name: "etcd-healthcheck-client", Run: runEtcdHealthcheckCert},
            {Name: "apiserver-etcd-client", Run: runAPIServerEtcdClientCert},
            {Name: "sa", Run: runServiceAccountKey},               // 生成 SA 密钥对
        },
    }
}
```

### CA 生成核心逻辑

```go
// cmd/kubeadm/app/phases/certs/ca.go
func CreateAsCA(cfg *kubeadmapi.InitConfiguration) (*x509.Certificate, crypto.Signer, error) {
    // 1. 生成 RSA 2048 位私钥（或 ECDSA P-256）
    // 2. 构造 X.509 证书模板:
    //    - Subject: CN=kubernetes, O=kubernetes
    //    - KeyUsage: DigitalSignature, KeyEncipherment, CertSign
    //    - BasicConstraints: CA=true
    //    - Validity: 10 年
    // 3. 使用私钥自签名
    // 4. 返回证书和私钥
}
```

---

## 源码参考

### 核心源码路径

| 组件 | 源码路径 | 说明 |
|------|---------|------|
| kubeadm 证书阶段 | `cmd/kubeadm/app/phases/certs/` | 证书生成入口 |
| PKI 工具库 | `cmd/kubeadm/app/util/pkiutil/` | 证书操作工具函数 |
| 通用证书库 | `staging/src/k8s.io/client-go/util/cert/` | 证书解析/生成 |
| kubelet 证书管理 | `pkg/kubelet/certificate/` | kubelet 证书轮换 |
| CSR Controller | `pkg/controller/certificates/` | CSR 自动审批 |
| ServiceAccount | `pkg/serviceaccount/` | SA Token 签发 |
| 证书审批 | `pkg/controller/certificates/approval/` | CSR 审批策略 |

### 关键函数速查

| 函数 | 位置 | 说明 |
|------|------|------|
| `CreateAsCA` | `pkiutil/cert.go` | 创建自签名 CA |
| `CreateCertAndKey` | `pkiutil/cert.go` | 创建证书和密钥对 |
| `NewSignedCert` | `pkiutil/cert.go` | 创建被 CA 签名的证书 |
| `GetAPIServerAltNames` | `pkiutil/cert.go` | 计算 API Server SAN |
| `CertOrKeyExist` | `pkiutil/cert.go` | 检查证书是否已存在 |
| `WriteCertAndKey` | `pkiutil/cert.go` | 写入证书和密钥到文件 |
| `ValidateCert` | `pkiutil/cert.go` | 验证证书有效性 |

---

## 版本说明

- 基于 Kubernetes v1.28 - v1.32 源码分析
- kubeadm 证书默认有效期：1 年（CA 10 年）
- kubelet 证书自动轮换：自 v1.19 起稳定
- `kubeadm certs renew` 命令自 v1.20 起替代 `kubeadm alpha certs renew`
- External CA 支持：kubeadm 检测到 `ca.crt` 存在但 `ca.key` 不存在时进入外部 CA 模式
- 证书自动续期监控：推荐使用 `x509-certificate-exporter` 或 Prometheus 告警

## Related

- Domain-34: CNCF Landscape 开源项目 — Cross-reference
- [[实体/release-notes-networking.md|发布说明索引 — 网络]] — Cross-reference
- 网络 MOC — Cross-reference
- Topic 应用层架构设计最佳实践 — Cross-reference
- topic-application-architecture MOC — Cross-reference
- [[概念/bp-common-best-practices.md|Kubernetes 通用最佳实践参考]] — Cross-reference
- [[元数据/KUDIG Knowledge Base Architecture.md|KUDIG Knowledge Base Architecture]] — Cross-reference
- [[AI基础设施/基础设施/03-gpu-scheduling-management.md|GPU 调度与管理]] — Cross-reference
- [[AI基础设施/基础设施/05-distributed-training-frameworks.md|分布式训练框架]] — Cross-reference
- 发布变更 MOC — Cross-reference
- [[技能/learn-decision-tree-mermaid.md|故障排查决策树 - Mermaid 可视化版]] — Cross-reference
- [[技能/skill-22-daemonset-failure.md|DaemonSet 故障诊断与修复 / DaemonSet Failure Diagnosis & Remediation]] — Cross-reference
- [[平台工程/运维/06-monitoring-alerting-system.md|监控告警体系]] — Cross-reference
- Domain 30: 企业级灾备与业务连续性 (Enterprise Disaster Recovery & Business Continuity) — Cross-reference
- [[实体/ecosystem-changelog.md|生态组件变更日志索引]] — Cross-reference
- [[生态参考/领域索引/cluster-index.md|Cluster 集群知识图谱索引]]
- [[生态参考/领域索引/pvc-index.md|PVC 知识图谱索引]]
- [[生态参考/领域索引/terway-index.md|Terway 知识图谱索引]]
- [[生态参考/领域索引/nginx-ingress-index.md|nginx-ingress-controller 知识图谱索引]]
- [[生态参考/领域索引/higress-index.md|Higress 知识图谱索引]]


<!-- risk-assessed -->
