---
title: Istio 安全加固 (entities)
description: '# Istio 安全加固'
summary: '该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[22-概念/01-核心架构/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]'
category: entities
tags:
- k8s
- cncf
- service-mesh
- 03-istio-security-hardening
- prometheus
- grafana
- istio
- crd
- operator
tier: core
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Istio 安全加固 是什么
- 如何 Istio 安全加固
trigger_keywords:
- Istio
- 安全加固
prerequisites:
- kubectl-basics
- service-mesh-basics
- prometheus-basics
- monitoring-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Istio 安全加固

> **CNCF 状态**: Graduated | **类别**: [[Service|Service]]Service Mesh）|Service Mesh]] | **主要语言**: Go

## 概述

Istio 安全加固指南涵盖 Istio 服务网格的安全配置最佳实践，包括 mTLS（双向 TLS）、认证授权（AuthorizationPolicy）、证书管理、安全策略和合规配置。Istio 是 CNCF 毕业项目，由 Google、IBM 和 Lyft 联合创建。安全是 Istio 的核心能力之一——通过 Sidecar 代理自动实现零信任网络（Zero Trust Network），为微服务通信提供加密、认证和细粒度授权。

## 核心特性

- **自动 mTLS**: Pod 间通信自动启用双向 TLS，无需修改应用代码
- **PeerAuthentication**: 按 Namespace/Workload 粒度配置 mTLS 模式（DISABLE/PERMISSIVE/STRICT）
- **AuthorizationPolicy**: 基于身份（SPIFFE）、命名空间、IP 的细粒度访问控制
- **RequestAuthentication**: JWT Token 验证和认证
- **证书管理**: 集成 cert-manager 或内置 CA 进行证书签发和轮换
- **审计日志**: 通过 Telemetry API 配置访问日志和安全审计

## 架构

Istio 安全架构基于 SPIFFE（Secure Production Identity Framework for Everyone）。istiod 作为 CA（Certificate Authority）为每个 Pod 签发 SVID（SPIFFE Verifiable Identity Document），格式为 `spiffe://trust-domain/ns/namespace/sa/serviceaccount`。Envoy Sidecar 使用 SVID 建立 mTLS 连接。AuthorizationPolicy 使用 SPIFFE ID 作为身份标识进行授权决策。RequestAuthentication 通过 Envoy 的 JWT Auth Filter 验证外部 JWT Token。证书轮换由 istiod 自动管理（默认 24 小时）。

## Kubernetes 集成

Istio 安全通过 CRD 配置。PeerAuthentication CRD 定义 mTLS 模式（Namespace 级或 Workload 级）。AuthorizationPolicy CRD 定义允许/拒绝规则（基于 Source、Operation、Condition）。RequestAuthentication CRD 定义 JWT 认证规则。安全策略通过 xDS API 下发到 Envoy Sidecar。与 Kubernetes RBAC 互补——K8s RBAC 管 API 访问，Istio AuthorizationPolicy 管运行时流量。

## 生产使用场景

1. **零信任安全**: 所有服务间通信强制 mTLS STRICT 模式
2. **细粒度授权**: 仅允许特定服务调用敏感 API
3. **JWT 认证**: 在网格入口验证外部 JWT Token
4. **合规审计**: 启用访问日志满足等保/SOC2 审计要求

## 安装

```bash
# 安装 Istio（启用 mTLS）
istioctl install --set profile=production
# 强制 mTLS
kubectl apply -f - <<EOF
apiVersion: security.istio.io/v1
kind: PeerAuthentication
metadata: { name: default, namespace: production }
spec:
  mtls: { mode: STRICT }
# 授权策略
---
apiVersion: security.istio.io/v1
kind: AuthorizationPolicy
metadata: { name: deny-all, namespace: production }
spec: {}
# 允许特定服务访问
---
apiVersion: security.istio.io/v1
kind: AuthorizationPolicy
metadata: { name: allow-frontend }
spec:
  selector: { matchLabels: { app: backend } }
  action: ALLOW
  rules:
  - from:
    - source: { principals: ["cluster.local/ns/production/sa/frontend"] }
EOF
```

## 替代方案

| 方案 | 优势 | 劣势 |
|------|------|------|
| **Istio Security** | 功能最全面、自动 mTLS | Sidecar 资源开销 |
| Linkerd mTLS | 极轻量 | 授权策略不如 Istio 丰富 |
| Cilium NetworkPolicy | eBPF 原生、无 Sidecar | 仅网络层控制 |
| OPA Gatekeeper | 策略灵活 | 非运行时执行 |

## 架构定位

在 CNCF 生态中，Istio 安全加固属于 **Service Mesh / Security** 类别，是零信任网络架构在 Kubernetes 上的标准实现。它与 SPIFFE/SPIRE、cert-manager 等项目协同工作。

## 参考链接

- [[23-实体/07-可观测性/prometheus-grafana.md|prometheus-grafana]]
- [[istio]]
- [[23-实体/vault.md|[[HashiCorp Vault|vault]]]]
- [[22-概念/05-安全/secrets-management.md|secrets-management]]
- [[pod-lifecycle]]

## Related

- [[dalec]] — Dalec
- [[vineyard]] — Vineyard
- [[distribution]] — Distribution
- [[istio]] — Istio
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- RELEASE-NOTES-1.9
- RELEASE-NOTES-1.28
- RELEASE-NOTES-0.8
- RELEASE-NOTES-1.18
- RELEASE-NOTES-1.19
- RELEASE-NOTES-1.8
- RELEASE-NOTES-1.29
- RELEASE-NOTES-1.16
- RELEASE-NOTES-1.22
- RELEASE-NOTES-1.3
- RELEASE-NOTES-0.2
- RELEASE-NOTES-1.26
- RELEASE-NOTES-1.7
- RELEASE-NOTES-1.12
- RELEASE-NOTES-0.6
- RELEASE-NOTES-1.27
- RELEASE-NOTES-1.6
- RELEASE-NOTES-1.13
- RELEASE-NOTES-0.7
- RELEASE-NOTES-1.17
- RELEASE-NOTES-1.23
- RELEASE-NOTES-1.2
- RELEASE-NOTES-0.3
- RELEASE-NOTES-1.5
- RELEASE-NOTES-1.24
- RELEASE-NOTES-1.10
- RELEASE-NOTES-0.4
- RELEASE-NOTES-1.14
- RELEASE-NOTES-1.1
- RELEASE-NOTES-1.20
- RELEASE-NOTES-1.15
- RELEASE-NOTES-1.0
- RELEASE-NOTES-1.21
- RELEASE-NOTES-0.1
- RELEASE-NOTES-1.4
- RELEASE-NOTES-1.25
- RELEASE-NOTES-1.11
- RELEASE-NOTES-0.5
- 03-istio-security-hardening
- [[23-实体/15-参考与索引/cncf-networking.md|CNCF 网络与服务网格项目全景]] — Cross-reference


<!-- risk-assessed -->
