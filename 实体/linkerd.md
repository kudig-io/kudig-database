---
title: Linkerd (entities)
description: '## 概述'
summary: '该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[概念/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]'
category: entities
tags:
- k8s
- cncf
- observability
- linkerd
- prometheus
- grafana
- istio
- gateway
- crd
- operator
tier: supporting
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Linkerd 是什么
- 如何 Linkerd
trigger_keywords:
- Linkerd
prerequisites:
- kubectl-basics
- service-mesh-basics
- prometheus-basics
- monitoring-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Linkerd

> **CNCF 状态**: Graduated | **类别**: Observability | **主要语言**: Rust, Go

## 概述

Linkerd 是 CNCF 毕业项目（Graduated），由 Buoyant 创建，是第一个服务网格项目（2016 年开源）。Linkerd 为 Kubernetes 提供零配置的透明服务网格能力，包括 mTLS 加密、流量路由、熔断、重试和可观测性。与 Istio 相比，Linkerd 以极致轻量和简单易用著称——数据平面使用 Rust 编写的微代理（Linkerd2-proxy），资源开销极低。

## 核心特性

- **超轻量数据平面**: Rust 编写的 Linkerd2-proxy，极低资源开销
- **零配置 mTLS**: 自动为所有 Pod 间通信启用 mTLS，无需配置证书
- **金丝雀发布**: 通过 TrafficSplit CRD 实现按权重的流量分发
- **内置可观测性**: 自动生成 Service Mesh 级别的 latency/request/error 指标
- **简单运维**: 无需 Envoy 配置，控制面自动下发代理规则
- **多集群**: Linkerd Multicluster 支持跨集群服务通信

## 架构

Linkerd 采用控制平面-数据平面分离架构。控制平面包括：Destination（服务发现和路由配置下发）、Identity（mTLS 证书签发）、Proxy Injector（Mutating Webhook 自动注入代理）。数据平面是 Linkerd2-proxy（Rust 微代理），以 Sidecar 形式注入到每个 Pod。iptables 将所有入站和出站流量重定向到代理。代理处理 mTLS 加密、协议感知路由（HTTP/gRPC/TCP）和指标采集。控制平面通过 gRPC 向代理下发配置。

## Kubernetes 集成

Linkerd 通过 CRD（ServiceProfile、TrafficSplit、HTTPRoute、AuthorizationPolicy）和 Mutating Webhook 集成。`linkerd inject` 或命名空间注解 `linkerd.io/inject=enabled` 触发自动 Sidecar 注入。ServiceProfile CRD 定义每个 Service 的延迟预算和重试策略。TrafficSplit CRD（配合 SMI 标准）实现金丝雀发布。支持 Gateway API 的 HTTPRoute 进行 L7 路由。控制平面组件以 Deployment 运行，数据平面以 Init Container + Sidecar 注入。

## 生产使用场景

1. **零信任安全**: 自动 mTLS 加密所有服务间通信
2. **金丝雀发布**: 通过 TrafficSplit 逐步将流量导入新版本
3. **服务可观测性**: 获得自动的 per-service latency、成功率和请求量指标
4. **多集群通信**: 使用 Multicluster 在多个集群间透明路由请求

## 安装

```bash
# 安装 Linkerd CLI
brew install linkerd
# 验证集群兼容性
linkerd check --pre
# 安装控制平面
linkerd install --crds | kubectl apply -f -
linkerd install | kubectl apply -f -
linkerd check
# 启用命名空间的网格注入
kubectl annotate namespace default linkerd.io/inject=enabled
kubectl rollout restart deployment -n default
# 金丝雀发布
kubectl apply -f - <<EOF
apiVersion: split.smi-spec.io/v1alpha2
kind: TrafficSplit
spec:
  service: web
  backends:
  - service: web-stable
    weight: 900m
  - service: web-canary
    weight: 100m
EOF
```

## 替代方案

| 项目 | 优势 | 劣势 |
|------|------|------|
| **Linkerd** | CNCF 毕业、极轻量、Rust 代理 | L7 功能不如 Istio 丰富 |
| Istio | 功能最全面 | 资源开销大、配置复杂 |
| Cilium Service Mesh | eBPF 无 Sidecar | 功能仍在发展中 |
| Consul Connect | 多平台支持 | 非 K8s 原生 |

## 架构定位

在 CNCF 生态中，Linkerd 属于 **Service Mesh** 类别，是最早的服务网格项目和 CNCF 毕业项目。它以简单和轻量著称，是 Istio 的主要替代方案。

## 参考链接

- [[实体/prometheus-grafana.md|prometheus-grafana]]
- [[istio]]
- [[实体/crd-custom-resources.md|crd-custom-resources]]
- [[概念/controller-pattern.md|controller-pattern]]
- [[概念/service-mesh-architecture.md|service-mesh-architecture]]

## Related

- [[kgateway]] — kgateway
- [[urunc]] — urunc
- [[connect-rpc]] — Connect RPC
- [[antrea]] — Antrea
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- 99-linkerd-service-mesh-guide
- 02-linkerd-enterprise-service-mesh
- linkerd
- [[归档/release-notes/networking/linkerd/RELEASE-NOTES-18.9.md|RELEASE-NOTES-18.9]]
- [[归档/release-notes/networking/linkerd/RELEASE-NOTES-18.8.md|RELEASE-NOTES-18.8]]
- [[归档/release-notes/networking/linkerd/RELEASE-NOTES-18.7.md|RELEASE-NOTES-18.7]]
- RELEASE-NOTES-0.2
- RELEASE-NOTES-0.3
- RELEASE-NOTES-0.4
- RELEASE-NOTES-0.1
- RELEASE-NOTES-0.5
- [[实体/networking-terms.md|K8s 网络术语参考]] — Cross-reference
- [[实体/release-notes-networking.md|发布说明索引 — 网络]] — Cross-reference
- [[实体/kudig-ecosystem-guide.md|KUDIG 开源生态指南与深度研究指南]] — Cross-reference
- [[概念/service-mesh-evolution.md|服务网格演进]] — Cross-reference
- [[概念/bp-security.md|最佳实践：Security]] — Cross-reference
- [[技能/ts-cloud-provider.md|云服务商集成排查]] — Cross-reference
- [[实体/cncf-networking.md|CNCF 网络与服务网格项目全景]] — Cross-reference
- [[实体/cncf-cicd.md|CNCF CI/CD 与发布管理项目全景]] — Cross-reference
- [[生态参考/领域索引/service-mesh-index.md|Service Mesh 服务网格知识图谱索引]]
- [[生态参考/领域索引/network-index.md|Network 网络知识图谱索引]]
- [[生态参考/领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
