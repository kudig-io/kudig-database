---
title: SPIFFE / SPIRE 与工作负载身份
description: '## 概述'
category: dictionary
tags:
- k8s
- glossary
- terminology
- istio
- envoy
- cilium
- opa
- mysql
- postgresql
- daemonset
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- SPIFFE / SPIRE 与工作负载身份 是什么
- 如何 SPIFFE / SPIRE 与工作负载身份
trigger_keywords:
- SPIFFE
- SPIRE
- 与工作负载身份
- dictionary
prerequisites:
- kubectl-basics
- cloud-provider-basics
- service-mesh-basics
- cilium-basics
- mysql-basics
- policy-basics
created: "2026-05-23"
created: 2026-05
---

# [[SPIFFE|SPIFFE]] / [[SPIRE|SPIRE]] 与工作负载身份

## 概述

在零信任（Zero Trust）安全架构中，"**永远不要信任，永远要验证**"是核心原则。传统的基于 IP 地址或网络边界的身份验证在 [[Kubernetes|Kubernetes]] 动态环境中已不再可靠。**SPIFFE（Secure Production Identity Framework For Everyone）** 和 **SPIRE（SPIFFE Runtime Environment）** 是 CNCF 孵化的开源项目，为跨云、跨集群的工作负载提供了统一、自动化、可密码学验证的身份标准。2026 年，SPIFFE/SPIRE 正在成为服务网格、mTLS、API 网关和云原生工作负载身份管理的事实标准。

## 核心概念/原理

### 1. 工作负载身份的挑战

在 Kubernetes 中，工作负载的标识传统上依赖于：
- **IP 地址**：Pod 重启后 IP 变化，不可靠
- **[[Service|Service]] Account Token**：仅适用于 K8s 内部，跨集群/跨云时难以统一
- **X.509 证书**：手动分发和管理证书在大规模场景下几乎不可行

SPIFFE 解决了上述问题，通过为每个工作负载分配一个**全球唯一的身份标识（SPIFFE ID）**和**自动签发的短期证书**。

### 2. SPIFFE ID

SPIFFE ID 采用 URI 格式，具有以下结构：
```
spiffe://trust-domain/ns/namespace/sa/service-account
```

例如：
```
spiffe://production.example.com/ns/payments/sa/api-gateway
```

这个 ID 是工作负载的"身份证"，具有以下特性：
- **全局唯一**：通过 trust domain 区分不同组织、集群或云环境
- **平台无关**：无论工作负载运行在 Kubernetes、VM、AWS Lambda 还是裸机上，身份格式统一
- **可验证**：通过 X.509-SVID 或 JWT-SVID 进行密码学验证

### 3. SVID（SPIFFE Verifiable Identity Document）

SVID 是 SPIFFE 身份的可验证凭证，有两种形式：
- **X.509-SVID**：基于 X.509 的短期证书，广泛用于 mTLS 双向认证
- **JWT-SVID**：JSON Web Token，适用于 HTTP/API 场景的身份传递

SVID 的典型有效期为 **1 小时到 24 小时**，到期前自动轮换，显著降低了凭证泄露的风险。

### 4. SPIRE 架构

SPIRE 是 SPIFFE 标准的具体实现，核心组件包括：
- **SPIRE Server**：集群级证书颁发机构（CA），负责签发和验证 SVID
- **SPIRE Agent**：运行在每个节点上的 DaemonSet，通过本地 Unix Domain Socket 向 Pod 提供 SVID
- **Node Attestation**：验证节点身份（如通过 AWS IAM、Azure MSI、Kubernetes Service Account）
- **Workload Attestation**：验证 Pod/容器身份，将 SPIFFE ID 与具体的 K8s Namespace + ServiceAccount 绑定

```
SPIRE Server (CA)
    ↓ 签发 SVID
SPIRE Agent (per-node DaemonSet)
    ↓ 通过 UDS 注入到 Pod
Workload Pod (持有 X.509-SVID / JWT-SVID)
    ↓ mTLS / JWT 验证
目标服务 (验证 SPIFFE ID)
```

## 关键机制或特性

### 身份验证 vs 授权

SPIFFE/SPIRE 解决的是**身份验证（Authentication）**问题：
- **证明"你是谁"**：通过 SVID 验证工作负载的身份
- **不解决"你能做什么"**：授权策略仍需通过服务网格（Istio/Linkerd/Cilium）、OPA/Kyverno 或应用层的 RBAC 实现

典型的零信任流程：
1. SPIRE 为 Pod A 签发 SVID：`spiffe://prod/ns/frontend/sa/web-app`
2. Pod A 使用 X.509-SVID 发起 mTLS 连接到 Pod B
3. Pod B 验证 Pod A 的 SVID，确认其 SPIFFE ID
4. Pod B 的授权策略检查 `frontend` namespace 的服务是否允许访问 `/api/data`

### 跨集群与跨云身份联邦

通过 **Trust Domain Federation**，不同集群或不同云厂商的 SPIRE Server 可以建立互信：
- 集群 A 的 SPIRE Server 信任集群 B 的根 CA
- 工作负载可以携带集群 A 的 SVID 访问集群 B 的服务
- 这是构建全球化零信任网络的基础

### 与 Service Mesh 集成

主流服务网格已原生支持 SPIFFE/SPIRE：
- **Istio**：Citadel 签发的身份基于 SPIFFE 标准，可与 SPIRE 集成
- **Cilium Service Mesh**：支持 SPIFFE 身份作为 NetworkPolicy 的匹配条件
- **[[envoy|Envoy]]**：通过 SDS（Secret Discovery Service）从 SPIRE Agent 动态获取 SVID

### 动态凭证轮换

SPIRE 的短期 SVID 自动轮换机制大幅提升了安全性：
- 证书有效期短（如 1 小时），即使被窃取，攻击窗口也极小
- 轮换过程对应用透明，无需重启 Pod
- 撤销异常工作负载的 SVID 可立即生效

## 使用场景

1. **跨集群微服务 mTLS**：分布在 AWS 和阿里云的 Kubernetes 集群中的微服务，通过 SPIFFE ID 实现统一的零信任双向认证
2. **API 网关身份验证**：API Gateway 验证客户端提供的 JWT-SVID，仅允许来自 `spiffe://prod/ns/frontend/*` 的请求访问内部 API
3. **多租户数据库访问控制**：数据库代理根据连接的 SPIFFE ID 自动路由到对应的租户 schema，无需在应用中管理数据库密码
4. **CI/CD Pipeline 安全**：构建和部署工具通过 SPIFFE ID 向 Kubernetes API Server 认证，替代长期有效的 ServiceAccount Token
5. **边缘设备身份管理**：工厂边缘的 K3s 节点和云端主集群通过 SPIRE Federation 建立互信，边缘 AI 推理服务安全地访问云端模型仓库

## 最佳实践/注意事项

- **Trust Domain 命名要稳定**：一旦确定尽量不要更改，因为 SPIFFE ID 会广泛嵌入到授权策略中
- **SPIRE Server 必须高可用**：作为整个集群的 CA，建议部署 3 个以上副本并使用外部数据库（如 PostgreSQL、MySQL）存储注册信息
- **Node Attestation 要可靠**：选择强身份验证方式（如 AWS IAM Role、TPM），防止恶意节点加入集群并获取合法 SVID
- **监控 SVID 签发和轮换**：异常的签发频率或失败的 attestation 可能是攻击迹象
- **授权策略与 SPIFFE ID 对齐**：在 NetworkPolicy 或服务网格策略中使用 SPIFFE ID 而非 IP 地址，确保策略在 Pod 漂移后仍然有效
- **定期轮换 CA 根证书**：虽然 SVID 是短期的，但 SPIRE Server 的根 CA 也应定期轮换并制定灾难恢复计划
- **最小化 SPIRE Agent 权限**：Agent 需要访问宿主机上的 Pod 元数据，但应通过受限的 RBAC 和 seccomp 配置运行
- **与 Secret 管理解耦**：SPIFFE/SPIRE 替代的是长期密码/证书，而不是所有 Secret。应用配置、API Key 等仍应使用 Vault/ESO 管理
- **渐进式采用**：从新的微服务开始引入 SPIFFE mTLS，逐步替换传统的基于 IP 或静态证书的认证方式

## 参考链接

- [SPIFFE Specification](https://spiffe.io/docs/latest/spiffe-about/overview/)
- [SPIRE Documentation](https://spiffe.io/docs/latest/spire-about/spire-concepts/)
- [Istio SPIFFE Integration](https://istio.io/latest/docs/concepts/security/#istio-identity)
- [Cilium SPIFFE Support](https://docs.cilium.io/en/stable/security/spiffe/)
- [CNCF - Zero Trust in Cloud Native Environments](https://www.cncf.io/blog/2022/05/10/zero-trust-in-cloud-native-environments/)

## Related

- [[domain-19-landscape-references/topic-index/service-mesh-index.md|Service Mesh 服务网格知识图谱索引]]
- [[domain-19-landscape-references/topic-index/security-index.md|Security 安全知识图谱索引]]
- [[domain-19-landscape-references/topic-index/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]
