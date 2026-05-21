---
title: SPIFFE
description: 'description: ''## 项目概述'''
category: general
tags:
- cncf
- ecosystem
- istio
- envoy
- postgresql
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 15min
intent_queries:
- SPIFFE 是什么
- 如何 SPIFFE
- Kubernetes 19 landscape references 最佳实践
trigger_keywords:
- SPIFFE
- landscape
- references
prerequisites:
- kubectl-basics
- cncf-ecosystem
- service-mesh-basics
- tls-basics
---

title: SPIFFE
description: '## 项目概述'
category: cncf-landscape
tags:
- k8s
- cncf
- cloud-native
- ecosystem
- istio
- envoy
- postgresql
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 架构师
- 技术决策者
- SRE
estimated_read_time: 5min
intent_queries:
- SPIFFE 是什么
- 如何 SPIFFE
- Kubernetes 34 cncf landscape 最佳实践
trigger_keywords:
- SPIFFE
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
# SPIFFE

> **成熟度**: Graduated | **加入时间**: 2018-03 | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官方网站** | https://spiffe.io |
| **GitHub** | https://github.com/spiffe/spiffe |
| **文档** | https://spiffe.io/docs |
| **许可证** | Apache-2.0 |
| **主要语言** | 规范文档 |
| **CNCF 分类** | Security |

---

## 项目概述

### 简介
SPIFFE (Secure Production Identity Framework for Everyone) 是一套开放标准规范，定义了在异构环境中为软件服务建立身份认证的框架。它解决了分布式系统中服务身份验证的核心问题。

### 发展历程
| 时间 | 里程碑 |
|:---|:---|
| 2017 | 由 Scytale 公司发起 |
| 2018-03 | 加入 CNCF Sandbox |
| 2020-06 | 晋升为 CNCF Incubating |
| 2022-09 | 晋升为 CNCF Graduated |

### 核心定位
SPIFFE 是零信任架构（Zero Trust）的基石，通过标准化的服务身份格式和验证机制，实现跨平台、跨环境的安全服务通信。

---

## 核心概念

### SPIFFE ID
SPIFFE ID 是服务的唯一身份标识符，采用 URI 格式：

```
spiffe://trust-domain/path

# 示例
spiffe://example.org/frontend/web-server
spiffe://prod.acme.com/payments/api
spiffe://k8s-cluster/ns/default/sa/nginx
```

| 组成部分 | 说明 |
|:---|:---|
| **scheme** | 固定为 `spiffe://` |
| **trust-domain** | 身份颁发机构的域名 |
| **path** | 服务的具体路径标识 |

### SVID (SPIFFE Verifiable Identity Document)
SVID 是 SPIFFE ID 的可验证载体，包含身份证明和加密材料：

```
┌─────────────────────────────────────────┐
│              SVID 结构                   │
├─────────────────────────────────────────┤
│  ┌─────────────────────────────────┐    │
│  │        X.509-SVID               │    │
│  │  • Subject CN/SAN 包含 SPIFFE ID│    │
│  │  • 支持 mTLS 双向认证           │    │
│  │  • 短期证书自动轮换             │    │
│  └─────────────────────────────────┘    │
│                                         │
│  ┌─────────────────────────────────┐    │
│  │        JWT-SVID                 │    │
│  │  • JWT 格式的身份令牌           │    │
│  │  • sub claim 为 SPIFFE ID       │    │
│  │  • 支持无连接认证场景           │    │
│  └─────────────────────────────────┘    │
└─────────────────────────────────────────┘
```

### Trust Bundle
Trust Bundle 是一组用于验证 SVID 的根证书集合：

```yaml
# Trust Bundle 示例
trust_domain: example.org
jwt_authorities:
  - public_key: "-----BEGIN PUBLIC KEY-----..."
    key_id: "abc123"
x509_authorities:
  - asn1: "-----BEGIN CERTIFICATE-----..."
```

---

## 架构设计

### 零信任模型

```
┌─────────────────────────────────────────────────────────────────┐
│                     SPIFFE 零信任架构                            │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│    ┌───────────────────┐         ┌───────────────────┐          │
│    │   Service A       │  mTLS   │   Service B       │          │
│    │  ┌─────────────┐  │◄───────►│  ┌─────────────┐  │          │
│    │  │ SPIFFE ID:  │  │         │  │ SPIFFE ID:  │  │          │
│    │  │ /payments   │  │         │  │ /frontend   │  │          │
│    │  └─────────────┘  │         │  └─────────────┘  │          │
│    │        ▲          │         │        ▲          │          │
│    └────────│──────────┘         └────────│──────────┘          │
│             │                             │                      │
│             │ SVID                        │ SVID                 │
│             │                             │                      │
│    ┌────────┴─────────────────────────────┴────────┐            │
│    │           SPIFFE Identity Provider             │            │
│    │     (SPIRE / 其他 SPIFFE 兼容实现)             │            │
│    └────────────────────────────────────────────────┘            │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

### 联邦架构 (Federation)

```
┌─────────────────────┐         ┌─────────────────────┐
│  Trust Domain A     │         │  Trust Domain B     │
│  example.org        │◄───────►│  partner.com        │
│                     │ Bundle  │                     │
│  ┌───────────────┐  │ Exchange│  ┌───────────────┐  │
│  │ Service A     │  │         │  │ Service B     │  │
│  │ spiffe://     │  │         │  │ spiffe://     │  │
│  │ example.org/  │──┼─────────┼──│ partner.com/  │  │
│  │ api           │  │  mTLS   │  │ backend       │  │
│  └───────────────┘  │         │  └───────────────┘  │
└─────────────────────┘         └─────────────────────┘
```

---

## Workload API

### API 概述
Workload API 是工作负载获取身份的标准接口：

```go
// Workload API 主要接口
type WorkloadAPI interface {
    // 获取 X.509 SVID
    FetchX509SVID() (*X509SVID, error)
    
    // 获取 JWT SVID
    FetchJWTSVID(audience []string) (*JWTSVID, error)
    
    // 获取 Trust Bundle
    FetchX509Bundles() (*X509BundleSet, error)
    
    // 订阅 SVID 更新
    WatchX509Context(watcher X509ContextWatcher) error
}
```

### 使用示例

```go
package main

import (
    "context"
    "github.com/spiffe/go-spiffe/v2/workloadapi"
)

func main() {
    ctx := context.Background()
    
    // 创建 Workload API 客户端
    client, _ := workloadapi.New(ctx)
    defer client.Close()
    
    // 获取 X.509 SVID
    x509SVID, _ := client.FetchX509SVID(ctx)
    
    // 使用 SVID 进行 mTLS
    tlsConfig := tlsconfig.MTLSClientConfig(
        x509SVID,
        x509SVID.Bundles,
        tlsconfig.AuthorizeAny(),
    )
}
```

---

## 使用场景

### 1. 服务网格身份
```yaml
# Istio + SPIFFE 集成
apiVersion: security.istio.io/v1beta1
kind: PeerAuthentication
metadata:
  name: default
spec:
  mtls:
    mode: STRICT
# 所有服务自动获得 SPIFFE ID
# spiffe://cluster.local/ns/default/sa/myservice
```

### 2. 多云服务通信
```
Cloud A (AWS)                    Cloud B (GCP)
┌────────────────┐              ┌────────────────┐
│ spiffe://      │              │ spiffe://      │
│ aws.acme.com/  │◄────────────►│ gcp.acme.com/  │
│ orders         │   联邦 mTLS  │ inventory      │
└────────────────┘              └────────────────┘
```

### 3. 数据库访问控制
```sql
-- PostgreSQL 使用 SPIFFE 身份认证
-- pg_hba.conf
hostssl all all 0.0.0.0/0 cert clientcert=verify-full

-- 客户端证书 CN 为 SPIFFE ID
-- spiffe://example.org/app/backend
```

---

## 生态集成

| 项目 | 集成方式 |
|:---|:---|
| **SPIRE** | SPIFFE 的参考实现 |
| **Istio** | 服务网格原生支持 |
| **Envoy** | SDS 获取 SVID |
| **Consul** | Connect 功能集成 |
| **HashiCorp Vault** | PKI 引擎集成 |
| **cert-manager** | CSI Driver 支持 |

---

## 最佳实践

### Trust Domain 设计
```
# 推荐：按组织/环境划分
spiffe://prod.example.org/...
spiffe://staging.example.org/...
spiffe://dev.example.org/...

# 避免：单一 Trust Domain
spiffe://example.org/prod/...  # 不推荐
```

### 路径命名规范
```
# Kubernetes 环境
spiffe://cluster.local/ns/{namespace}/sa/{serviceaccount}

# VM 环境
spiffe://example.org/datacenter/{dc}/host/{hostname}

# 多租户环境
spiffe://example.org/tenant/{tenant-id}/service/{name}
```

---

## 参考资源

- [SPIFFE 规范文档](https://github.com/spiffe/spiffe/tree/main/standards)
- [官方文档](https://spiffe.io/docs)
- [GitHub Repo](https://github.com/spiffe/spiffe)
- [CNCF 项目页面](https://www.cncf.io/projects/spiffe/)
- [零信任架构指南](https://spiffe.io/docs/latest/spiffe-about/overview/)

---

**维护者**: Kudig Team | **许可证**: MIT

## Related

- [[domain-17-system-foundation/topic-cheat-sheet/go.md|go]]
- [[domain-17-system-foundation/topic-cheat-sheet/sql.md|sql]]
- [[domain-17-system-foundation/topic-cheat-sheet/k8s.md|k8s]]
- [[domain-17-system-foundation/topic-cheat-sheet/git.md|git]]
- [[entities/kubernetes.md|kubernetes]]
- [[entities/cncf-security|CNCF 安全与合规项目全景]] — Cross-reference
- [[domain-19-landscape-references/topic-index/etcd-index|etcd 知识图谱索引]]
- [[domain-19-landscape-references/topic-index/cert-index|Certificate / TLS 证书知识图谱索引]]
- [[domain-19-landscape-references/topic-index/security-index|Security 安全知识图谱索引]]
