---
title: SPIFFE (entities)
description: '## 概述'
summary: '该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[概念/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]'
category: entities
tags:
- k8s
- cncf
- security
- spiffe
- istio
- crd
- operator
- kubeflow
tier: peripheral
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- SPIFFE 是什么
- 如何 SPIFFE
trigger_keywords:
- SPIFFE
prerequisites:
- kubectl-basics
- service-mesh-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# SPIFFE

> **CNCF 状态**: Graduated | **类别**: Security | **主要语言**: 规范文档

## 概述

SPIFFE（Secure Production Identity Framework for Everyone）是一个 CNCF 毕业项目，由 Scytale（现 HPE）发起，为云原生环境提供统一的 workload 身份框架。它定义了一套标准来为不同环境中的工作负载（容器、VM、裸机）颁发和验证加密身份。SPIFFE 解决了跨集群、跨云、跨平台工作负载间 mTLS 认证的难题，无需依赖 IP 地址或网络边界。SPIFFE 由两部分组成：SPIFFE 规范（定义身份格式）和 SPIRE（参考实现）。

## Key Features（核心能力）

- **SPIFFE ID**：标准化的工作负载身份标识 URI（如 spiffe://example.org/ns/default/sa/myapp）
- **SVID（SPIFFE Verifiable Identity Document）**：承载身份的凭证，支持 X.509 和 JWT 两种格式
- **Workload API**：为工作负载提供身份获取、信任bundle 更新的 gRPC API
- **联邦信任**：支持跨域信任建立，实现不同信任域间的工作负载互信
- **多节点架构**：支持 Agent-Server 架构，Server 集群提供 HA
- **可扩展的插件体系**：支持多种密钥存储、节点认证、工作负载注册插件

## 架构与工作原理

SPIFFE 的参考实现 SPIRE 采用 Server-Agent 架构：SPIRE Server 负责签发 SVID 和管理信任域，通过可插拔的 KeyManager 存储签名密钥；SPIRE Agent 以 DaemonSet 彐式运行在每个节点，通过 Workload API 为本地进程提供 SVID 和信任 Bundle。Agent 通过节点认证（Node Attestation）证明节点身份，然后为已注册的工作负载签发 SVID。工作负载通过 Unix Domain Socket 访问 Workload API 获取凭证。

## K8s 集成

在 Kubernetes 中，SPIRE Agent 以 DaemonSet 部署到每个节点，通过 K8s ServiceAccount Token 进行节点认证。工作负载通过 Pod 身份（ServiceAccount + Namespace）自动获取对应的 SPIFFE 身份。SPIFFE 可与 Envoy Proxy 集成实现自动 mTLS，也可与 Istio Service Mesh 集成替代自签证书。

## 生产用例

- **Zero Trust 网络**：工作负载间 mTLS 双向认证，无需信任网络边界
- **跨集群服务通信**：不同 K8s 集群中的服务通过 SPIFFE 联邦信任安全通信
- **多云互连**：跨 AWS、GCP、Azure 的服务建立统一身份和信任关系
- **合规要求**：满足零信任架构（ZTA）和金融级安全合规要求

## 安装与配置

```bash
# 🟢 Helm 安装 SPIRE
helm repo add spiffe https://spiffe.github.io/helm-charts/
helm install spire spiffe/spire -n spire-system --create-namespace \
  --set server.replicaCount=3 \
  --set server.dataStorage.size=10Gi

# 🟢 验证安装
kubectl get pods -n spire-system
kubectl get crd | grep spire

# 🟢 检查 SPIRE Server 状态
kubectl exec -n spire-system spire-server-0 -- spire-server healthcheck

# 🟢 查看已注册的 Workload
kubectl exec -n spire-system spire-server-0 -- spire-server entry show

# 🟢 查看 Agent 状态
kubectl exec -n spire-system spire-server-0 -- spire-server agent show
```

### SPIRE Server 配置

```yaml
# spire-server ConfigMap 核心配置
apiVersion: v1
kind: ConfigMap
metadata:
  name: spire-server
  namespace: spire-system
data:
  server.conf: |
    server {
      bind_address = "0.0.0.0"
      bind_port = "8081"
      trust_domain = "example.org"
      data_dir = "/run/spire/data"
      log_level = "INFO"
      ca_ttl = "24h"
      default_x509_svid_ttl = "1h"
      default_jwt_svid_ttl = "5m"
    }
    
    plugins {
      DataStore "sql" {
        plugin_data {
          database_type = "postgres"
          connection_string = "host=spire-db dbname=spire user=spire password=${DB_PASS}"
        }
      }
      KeyManager "disk" {
        plugin_data {
          keys_path = "/run/spire/data/keys.json"
        }
      }
      NodeAttestor "k8s_psat" {
        plugin_data {
          clusters = {
            "production" = {
              service_account_allow_list = ["spire-system:spire-agent"]
            }
          }
        }
      }
    }
```

### Workload 注册

```bash
# 🟡 注册 Workload (基于 ServiceAccount)
kubectl exec -n spire-system spire-server-0 -- \
  spire-server entry create \
  -spiffeID spiffe://example.org/ns/default/sa/myapp \
  -parentID spiffe://example.org/agent/k8s \
  -selector k8s:ns:default \
  -selector k8s:sa:myapp \
  -x509SVIDTTL 1h

# 🟢 查看注册条目
kubectl exec -n spire-system spire-server-0 -- spire-server entry show

# 🟡 删除注册条目
kubectl exec -n spire-system spire-server-0 -- spire-server entry delete -entryID <entry-id>
```

### Workload API 使用

```bash
# 在 Pod 中通过 Unix Socket 获取 SVID
# 挂载 spire-agent socket
# volumes:
# - name: spire-agent-socket
#   hostPath:
#     path: /run/spire/agent-sockets
#     type: Directory

# 获取 X.509 SVID
spire-agent api fetch x509

# 获取 JWT SVID
spire-agent api fetch jwt -audience my-service

# 获取信任 Bundle
spire-agent api fetch x509 --write /tmp/svid
```

## 运维操作

### 常用命令

```bash
# 🟢 查看 SPIRE Server 日志
kubectl logs -n spire-system -l app=spire-server --tail=50

# 🟢 查看 SPIRE Agent 日志
kubectl logs -n spire-system -l app=spire-agent --tail=50

# 🟢 检查 Server 健康
kubectl exec -n spire-system spire-server-0 -- spire-server healthcheck

# 🟢 查看 Agent 列表
kubectl exec -n spire-system spire-server-0 -- spire-server agent show

# 🟡 驱逐异常 Agent
kubectl exec -n spire-system spire-server-0 -- spire-server agent evict -spiffeID spiffe://example.org/agent/k8s/<node>

# 🟢 查看 Bundle
kubectl exec -n spire-system spire-server-0 -- spire-server bundle show

# 🟡 创建联邦信任
kubectl exec -n spire-system spire-server-0 -- \
  spire-server bundle set -id spiffe://partner.org -path /tmp/partner-bundle.pem

# 🟢 查看 Token (用于 Agent 注册)
kubectl exec -n spire-system spire-server-0 -- spire-server token generate -spiffeID spiffe://example.org/agent/k8s
```

## 故障排查

| 症状 | 可能原因 | 排查命令 | 修复方案 |
|------|----------|----------|----------|
| Workload 无法获取 SVID | 未注册/Selector 不匹配 | `spire-server entry show` | 创建匹配的注册条目 |
| Agent 未连接 | 节点认证失败 | `spire-server agent show` | 检查 PSAT 配置和 RBAC |
| SVID 过期 | TTL 过短/轮转失败 | `spire-agent api fetch x509` | 调整 TTL 配置 |
| 联邦信任失败 | Bundle 不匹配 | `spire-server bundle show -id <domain>` | 更新联邦 Bundle |
| Server 不可用 | 数据库连接失败 | `kubectl logs spire-server-0` | 检查 PostgreSQL 连接 |
| Socket 不可访问 | 挂载路径错误 | `ls /run/spire/agent-sockets` | 检查 hostPath 配置 |

### 排查流程

```
1. kubectl get pods -n spire-system → 确认组件状态
2. spire-server healthcheck → Server 健康检查
3. spire-server agent show → Agent 连接状态
4. spire-server entry show → 注册条目检查
5. spire-agent api fetch x509 → 测试 SVID 获取
6. kubectl logs spire-server-0 → 查看服务日志
```

## 生产案例

### 案例1: 多集群 Zero Trust
- **场景**: 3个 K8s 集群间服务需要 mTLS 互信
- **方案**: 每个集群部署 SPIRE，通过联邦信任建立跨集群互信
- **效果**: 无需共享 CA，各集群独立管理身份，实现 Zero Trust

### 案例2: 混合云工作负载身份
- **场景**: K8s + VM + 裸机服务需要统一身份
- **方案**: SPIRE 统一管理所有工作负载身份，Envoy 使用 SVID 进行 mTLS
- **效果**: 统一身份框架，消除 IP 白名单依赖

## 对比替代方案

| 维度 | SPIFFE/SPIRE | Istio mTLS | Vault PKI | 传统 CA |
|------|-------------|-----------|-----------|--------|
| 身份标准 | SPIFFE ID | 自定义 | 自定义 | X.509 |
| 自动轮转 | 支持 | 支持 | 支持 | 手动 |
| 跨平台 | K8s/VM/裸机 | 仅 Mesh | 通用 | 通用 |
| 联邦信任 | 原生支持 | 有限 | 无 | 复杂 |
| 工作负载 API | 原生 | Sidecar | API | 无 |
| 复杂度 | 中 | 高 | 中 | 低 |

## 检查清单

- [ ] SPIRE Server 副本数 >= 3 (HA)
- [ ] 使用外部数据库 (PostgreSQL) 而非内存存储
- [ ] Agent 以 DaemonSet 部署在所有节点
- [ ] Workload 注册条目已配置
- [ ] SVID TTL 合理 (建议 1h)
- [ ] 联邦信任已配置 (跨集群场景)
- [ ] 监控 SPIRE Server 和 Agent 健康状态
- [ ] 定期审计注册条目和 Agent 列表

## Related

- [[tikv]] — TiKV
- [[k8gb]] — K8GB
- [[lima]] — Lima
- [[kubeflow]] — Kubeflow
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- spiffe
- [[实体/cncf-security.md|[[CNCF 安全与合规项目全景|CNCF 安全与合规项目全景]]]] — Cross-reference
- index/etcd-index|etcd 知识图谱索引]]
- [[生态参考/领域索引/cert-index.md|Certificate / TLS 证书知识图谱索引]]
- [[生态参考/领域索引/security-index.md|Security 安全知识图谱索引]]


<!-- risk-assessed -->
