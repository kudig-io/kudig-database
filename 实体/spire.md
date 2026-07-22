---
title: SPIRE (entities)
description: '## 概述'
summary: '该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[概念/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]'
category: entities
tags:
- k8s
- cncf
- security
- spire
- kubelet
- istio
- crd
- operator
tier: peripheral
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- SPIRE 是什么
- 如何 SPIRE
trigger_keywords:
- SPIRE
prerequisites:
- kubectl-basics
- service-mesh-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# SPIRE

> **CNCF 状态**: Graduated | **类别**: Security | **主要语言**: Go

## 概述

SPIRE（SPIFFE Runtime Environment）是 CNCF 毕业项目 SPIFFE 的参考实现，由 HPE 主导开发。它是一个生产级的工作负载身份管理平台，实现了 SPIFFE 规范定义的 Workload API 和身份验证机制。SPIRE 为云原生环境中的工作负载提供自动化的加密身份颁发、轮转和验证，无需人工干预。它支持 Kubernetes、Docker、VM、裸机等多种部署环境，是零信任架构（Zero Trust Architecture）的关键基础设施。

## Key Features（核心能力）

- **身份自动颁发**：工作负载启动时自动获取短期 SVID（X.509 或 JWT）
- **Agent-Server 架构**：Server 集群管理信任域，Agent 以 DaemonSet 运行在每个节点
- **节点认证**：支持多种节点认证方式（K8s PSAT、AWS IID、GCP、Azure）
- **可插拔架构**：通过插件扩展密钥存储、节点认证、工作负载注册器
- **SVID 自动轮转**：短期凭证到期前自动轮转，无需应用感知
- **联邦信任**：支持跨信任域的联邦身份验证

## 架构与工作原理

SPIRE 采用 Server-Agent 架构。SPIRE Server 负责签发 SVID、管理信任域和签名密钥，支持集群部署实现 HA。Server 通过 KeyManager 插件管理私钥（支持内存、Disk、AWS KMS、HashiCorp Vault 等）。SPIRE Agent 运行在每个节点，通过 Node Attestor 证明节点身份，通过 Workload Registrar 注册工作负载身份映射。Agent 暴露 Workload API（Unix Domain Socket），工作负载通过它获取 SVID。

## K8s 集成

在 Kubernetes 中，SPIRE Server 通过 StatefulSet 部署实现 HA，使用 PVC 存储数据库。SPIRE Agent 以 DaemonSet 形式部署，通过 K8s ServiceAccount Token 进行节点认证。工作负载通过 Pod 的 ServiceAccount 和 Namespace 自动映射到 SPIFFE 身份。可集成 Envoy 的 SPIFFE 校验过滤器实现透明 mTLS。

## 生产用例

- **零信任服务网格**：无需 Istio 即可实现服务间 mTLS 双向认证
- **跨集群信任**：在不同 K8s 集群间建立统一的工作负载身份信任
- **合规审计**：提供细粒度的工作负载身份和访问审计
- **混合云安全**：统一管理 K8s、VM、Serverless 工作负载的身份

## 安装与配置

```bash
# 🟢 Helm 安装 SPIRE
helm repo add spiffe https://spiffe.github.io/helm-charts/
helm install spire-crds spiffe/spire-crds -n spire-server --create-namespace
helm install spire spiffe/spire -n spire-server \
  --set server.replicaCount=3 \
  --set server.dataStorage.size=10Gi

# 🟢 验证安装
kubectl get pods -n spire-server
kubectl get crd | grep spire

# 🟢 检查 Server 健康
kubectl exec -n spire-server spire-server-0 -- spire-server healthcheck

# 🟢 查看 Agent 状态
kubectl exec -n spire-server spire-server-0 -- spire-server agent show

# 🟢 查看注册条目
kubectl exec -n spire-server spire-server-0 -- spire-server entry show
```

### Workload 注册示例

```bash
# 🟡 注册基于 ServiceAccount 的 Workload
kubectl exec -n spire-server spire-server-0 -- \
  spire-server entry create \
  -spiffeID spiffe://example.org/ns/default/sa/myapp \
  -parentID spiffe://example.org/agent/k8s \
  -selector k8s:ns:default \
  -selector k8s:sa:myapp \
  -x509SVIDTTL 1h \
  -jwtSVIDTTL 5m

# 🟡 注册 DNS 名称
kubectl exec -n spire-server spire-server-0 -- \
  spire-server entry create \
  -spiffeID spiffe://example.org/ns/istio-system/sa/istio-ingressgateway \
  -parentID spiffe://example.org/agent/k8s \
  -selector k8s:ns:istio-system \
  -selector k8s:sa:istio-ingressgateway \
  -dns ingress.example.com
```

### Pod 挂载 Workload API Socket

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: myapp
  namespace: default
spec:
  containers:
  - name: app
    image: myapp:latest
    volumeMounts:
    - name: spire-agent-socket
      mountPath: /run/spire/agent-sockets
      readOnly: true
  volumes:
  - name: spire-agent-socket
    hostPath:
      path: /run/spire/agent-sockets
      type: Directory
```

## 运维操作

### 常用命令

```bash
# 🟢 查看 Server 日志
kubectl logs -n spire-server -l app=spire-server --tail=50

# 🟢 查看 Agent 日志
kubectl logs -n spire-system -l app=spire-agent --tail=50

# 🟢 检查健康状态
kubectl exec -n spire-server spire-server-0 -- spire-server healthcheck

# 🟢 查看 Agent 列表
kubectl exec -n spire-server spire-server-0 -- spire-server agent show

# 🟡 驱逐异常 Agent
kubectl exec -n spire-server spire-server-0 -- \
  spire-server agent evict -spiffeID spiffe://example.org/agent/k8s/<node-id>

# 🟢 查看 Bundle
kubectl exec -n spire-server spire-server-0 -- spire-server bundle show

# 🟡 生成 Agent 加入 Token
kubectl exec -n spire-server spire-server-0 -- \
  spire-server token generate -spiffeID spiffe://example.org/agent/k8s

# 🟡 创建联邦信任
kubectl exec -n spire-server spire-server-0 -- \
  spire-server bundle set -id spiffe://partner.org -path /tmp/partner-bundle.pem

# 🟢 查看注册条目详情
kubectl exec -n spire-server spire-server-0 -- spire-server entry show -spiffeID spiffe://example.org/ns/default/sa/myapp
```

## 故障排查

| 症状 | 可能原因 | 排查命令 | 修复方案 |
|------|----------|----------|----------|
| Workload 无法获取 SVID | 未注册/Selector 不匹配 | `spire-server entry show` | 创建匹配的注册条目 |
| Agent 未连接 | 节点认证失败 | `spire-server agent show` | 检查 PSAT 配置和 RBAC |
| SVID 过期 | TTL 过短/轮转失败 | `spire-agent api fetch x509` | 调整 TTL 配置 |
| Server 不可用 | 数据库连接失败 | `kubectl logs spire-server-0` | 检查 PostgreSQL 连接 |
| Socket 不可访问 | 挂载路径错误 | `ls /run/spire/agent-sockets` | 检查 hostPath 配置 |
| 联邦信任失败 | Bundle 不匹配 | `spire-server bundle show -id <domain>` | 更新联邦 Bundle |

### 排查流程

```
1. kubectl get pods -n spire-server → 确认组件状态
2. spire-server healthcheck → Server 健康检查
3. spire-server agent show → Agent 连接状态
4. spire-server entry show → 注册条目检查
5. spire-agent api fetch x509 → 测试 SVID 获取
6. kubectl logs spire-server-0 → 查看服务日志
```

## 生产案例

### 案例1: 多集群 Zero Trust 网络
- **场景**: 3个 K8s 集群 + VM 工作负载需要统一身份
- **方案**: 每集群部署 SPIRE，通过联邦信任建立跨集群互信
- **效果**: 消除 IP 白名单，实现真正的 Zero Trust

### 案例2: Envoy mTLS 无 Sidecar
- **场景**: 不想部署完整 Service Mesh，但需要 mTLS
- **方案**: SPIRE + Envoy SDS，直接从 Workload API 获取证书
- **效果**: 轻量级 mTLS，无需 Istio 全套组件

## 对比替代方案

| 维度 | SPIRE | Vault PKI | Istio mTLS | cert-manager |
|------|-------|-----------|-----------|-------------|
| 专注领域 | 工作负载身份 | 密钥管理 | 服务网格 | 证书管理 |
| 自动轮转 | 支持 | 支持 | 支持 | 支持 |
| 跨平台 | K8s/VM/裸机 | 通用 | 仅 Mesh | 仅 K8s |
| 联邦信任 | 原生 | 无 | 有限 | 无 |
| 复杂度 | 中 | 中 | 高 | 低 |

## 检查清单

- [ ] SPIRE Server 副本数 >= 3 (HA)
- [ ] 使用外部数据库 (PostgreSQL) 而非内存
- [ ] Agent DaemonSet 在所有节点运行
- [ ] Workload 注册条目已配置
- [ ] SVID TTL 合理 (X.509: 1h, JWT: 5m)
- [ ] 联邦信任已配置 (跨集群场景)
- [ ] 监控 Server 和 Agent 健康状态
- [ ] 定期审计注册条目和 Agent 列表

## Related

- [[openchoreo]] — OpenChoreo
- [[podman-desktop]] — Podman Desktop
- [[openyurt]] — OpenYurt
- [[carina]] — Carina
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- spire
- [[实体/cncf-security.md|CNCF 安全与合规项目全景]] — Cross-reference
- [[生态参考/领域索引/cert-index.md|Certificate / TLS 证书知识图谱索引]]
- [[生态参考/领域索引/security-index.md|Security 安全知识图谱索引]]
- [[生态参考/领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
