---
title: 多集群环境下的安全架构
description: '# 多集群环境下的安全架构'
summary: '# 多集群环境下的安全架构'
category: synthesis
tags:
- multi-cluster
- security
- zero-trust
- network-policy
- mTLS
- istio
- cilium
- opa
- falco
- rbac
tier: supporting
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 多集群环境下的安全架构 是什么
- 如何 多集群环境下的安全架构
trigger_keywords:
- 多集群环境下的安全架构
prerequisites:
- kubectl-basics
- service-mesh-basics
- cilium-basics
- policy-basics
relationships:
- target: '[[实体/cilium.md]]'
  type: uses
- target: '[[实体/external-secrets.md]]'
  type: uses
- target: '[[实体/falco.md]]'
  type: related_to
status: reviewed
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 多集群环境下的安全架构

## 概述

多集群环境下的安全架构需要解决单集群安全不曾面临的挑战：跨集群身份一致性、网络互通中的流量加密、策略的集中管理与联邦执行、以及分散集群的统一审计。零信任架构（Zero Trust）是多集群安全的设计哲学——不因集群间有网络连通就默认信任，每个跨集群请求都需要身份验证和授权。

## 安全挑战

### 五大核心挑战

```
多集群安全挑战:
├── 身份一致性
│   → 集群 A 中的 service-a 如何向集群 B 证明身份
│   → 传统 ServiceAccount 仅集群内有效
│   → 解决: SPIFFE/SPIRE 提供跨集群 Workload Identity
│
├── 网络隔离
│   → 集群间流量在公网/VPN/专线上传输
│   → 默认明文传输存在窃听风险
│   → 解决: 跨集群 mTLS（Istio Multi-Cluster）
│
├── 策略一致性
│   → 各集群独立配置 NetworkPolicy/RBAC
│   → 安全基线漂移、配置不一致
│   → 解决: OPA Gatekeeper 联邦策略 + GitOps 统一管理
│
├── Secrets 管理
│   → 每个集群独立存储 Secrets
│   → 密钥轮换需要逐集群操作
│   → 解决: External Secrets Operator 统一管理
│
└── 合规审计
    → 审计日志分散在各集群
    → 跨集群安全事件关联困难
    → 解决: 集中式 SIEM + 统一日志管道
```

## 架构方案

### 零信任多集群架构

```
┌─────────────────────────────────────────────┐
│         零信任控制平面                        │
│  ┌─────────────────────────────────────────┐ │
│  │  SPIRE Server (统一 CA)                  │ │
│  │  - 签发 SPIFFE 身份证书                  │ │
│  │  - 跨集群信任域 (trust domain) 管理       │ │
│  └─────────────────────────────────────────┘ │
│  ┌─────────────────────────────────────────┐ │
│  │  OPA Gatekeeper (联邦策略)               │ │
│  │  - 安全基线策略统一分发                   │ │
│  │  - 策略合规性持续验证                     │ │
│  └─────────────────────────────────────────┘ │
└─────────────────────────────────────────────┘
                    │ mTLS
        ┌───────────┴───────────┐
        ▼                       ▼
┌───────────────┐       ┌───────────────┐
│  Cluster East │←mTLS─→│  Cluster West │
│               │       │               │
│ ┌───────────┐ │       │ ┌───────────┐ │
│ │Istio +    │ │       │ │Istio +    │ │
│ │SPIRE Agent│ │       │ │SPIRE Agent│ │
│ └───────────┘ │       │ └───────────┘ │
│ ┌───────────┐ │       │ ┌───────────┐ │
│ │ Falco +   │ │       │ │ Falco +   │ │
│ │ SIEM Forward│       │ │ SIEM Forward│
│ └───────────┘ │       │ └───────────┘ │
└───────────────┘       └───────────────┘
```

### Istio 多集群 mTLS

```yaml
# Istio Multi-Primary 多集群配置
# 两个集群通过共享根 CA 建立信任
apiVersion: v1
kind: Secret
metadata:
  name: cacerts
  namespace: istio-system
type: Opaque
data:
  root-cert.pem: <base64-encoded-root-ca>
  ca-cert.pem: <base64-encoded-intermediate-ca>
  ca-key.pem: <base64-encoded-ca-key>
---
# 跨集群服务发现
apiVersion: networking.istio.io/v1beta1
kind: ServiceEntry
metadata:
  name: cross-cluster-service
spec:
  hosts:
  - "order-service.ns-east.svc.cluster.global"
  location: MESH_INTERNAL
  ports:
  - number: 80
    name: http
    protocol: HTTP
  resolution: DNS
  endpoints:
  - address: cluster-west-inggateway.external-ip
    ports:
      http: 15443
```

## 工具链

| 层面 | 工具 | 功能 |
|------|------|------|
| 服务网格 | Istio, Linkerd, [[实体/cilium.md|Cilium]] Mesh | 跨集群 mTLS、服务发现、流量管理 |
| 身份 | SPIFFE/SPIRE | 统一 Workload Identity，跨集群身份验证 |
| 策略 | OPA/Gatekeeper (联邦策略) | 安全基线统一执行、合规性验证 |
| Secrets | [[实体/external-secrets.md|External Secrets]] Operator | 跨集群密钥同步和轮换 |
| 审计 | [[实体/falco.md|Falco]] + SIEM | 运行时安全事件集中收集 |
| 网络 | Cilium Cluster Mesh | 跨集群 Pod 间通信 |

### External Secrets 跨集群密钥管理

```yaml
# External Secrets: 从统一密钥仓库同步到各集群
apiVersion: external-secrets.io/v1beta1
kind: ExternalSecret
metadata:
  name: database-credentials
spec:
  refreshInterval: 1h
  secretStoreRef:
    name: vault-backend            # 各集群连接同一 Vault
    kind: ClusterSecretStore
  target:
    name: db-secret
    creationPolicy: Owner
  data:
  - secretKey: username
    remoteRef:
      key: production/db/credentials
      property: username
  - secretKey: password
    remoteRef:
      key: production/db/credentials
      property: password
```

## 最佳实践

- **统一 CA 和信任域**：所有集群使用同一个根 CA（通过 SPIRE 或 Istio），确保 mTLS 证书在跨集群通信中可被验证
- **安全策略 GitOps 化**：NetworkPolicy、RBAC、AuthorizationPolicy 等安全配置通过 Git 统一管理，通过 ArgoCD 分发到所有集群——确保策略一致性
- **集中式审计日志**：所有集群的安全审计日志（Falco、K8s Audit Log、Istio Access Log）转发到集中 SIEM（Elasticsearch/Splunk），支持跨集群事件关联
- **最小化集群间暴露面**：集群间通信通过专用 IngressGateway，仅暴露必要端口——不要将整个 Pod CIDR 暴露给其他集群
- **定期安全基线审计**：使用 kube-bench、kube-hunter 定期扫描所有集群的安全基线一致性，发现漂移立即修复

## 常见陷阱

- **集群间信任域不一致**：如果各集群使用不同 CA，跨集群 mTLS 会失败——必须在部署前规划统一的信任架构
- **NetworkPolicy 跨集群不生效**：NetworkPolicy 是集群内资源，不会自动跨集群——跨集群网络隔离需要 Service Mesh 的 AuthorizationPolicy 或 Cilium Cluster Mesh 策略
- **密钥分散管理导致泄露**：每个集群独立管理 Secrets，轮换时遗漏某些集群——使用 External Secrets 统一管理可避免

## 相关 Domain

- 安全/01-security-baseline/01-zero-trust-architecture
- 网络/03-service-mesh/01-istio-multi-cluster

## 相关页面

- [[概念/service-mesh-security-governance.md|服务网格安全治理]] — 单集群内的安全策略
- [[概念/security-observability-correlation.md|安全与可观测性关联]] — 安全事件分析
- [[概念/multi-tenancy-isolation.md|多租户隔离]] — 集群内隔离

## Related

- [[系统基础/知识字典/configuration/secrets.md|Secrets]]
- [[实体/istio.md|Istio (entities)]]


<!-- risk-assessed -->
