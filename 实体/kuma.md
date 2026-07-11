---
title: Kuma (entities)
description: '## 概述'
summary: 'Kuma 是一个通用服务网格控制平面，设计简单易用且功能强大。'
category: entities
tags:
- k8s
- cncf
- service-mesh
- kuma
- prometheus
- grafana
- jaeger
- envoy
- gateway
- crd
tier: peripheral
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Kuma 是什么
- 如何 Kuma
trigger_keywords:
- Kuma
prerequisites:
- kubectl-basics
- prometheus-basics
- monitoring-basics
- tracing-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# Kuma

> **CNCF 状态**: Sandbox | **类别**: Service Mesh | **主要语言**: Go

## 概述

Kuma 是一个通用服务网格控制平面，由 Kong 公司开源，2020 年加入 CNCF 沙箱。它设计简单易用且功能强大，基于 Envoy 代理构建，支持 Kubernetes 和虚拟机/裸金属环境。Kuma 的核心差异化在于多网格（Multi-Mesh）能力——单一控制平面可以管理多个独立的服务网格部署（Mesh），每个 Mesh 拥有独立的策略和配置。这使得 Kuma 特别适合需要为不同团队/产品线提供独立服务网格的企业场景。Kuma 提供开箱即用的策略，包括 mTLS、流量管理、可观测性和安全，支持通过 CRD 声明式配置。它还内置了 Kong Gateway 的集成能力。

## 核心能力

- **多平台支持**: 同时支持 Kubernetes（Sidecar 模式）和虚拟机/裸金属（透明代理模式）
- **多网格管理**: 单一控制平面管理多个独立 Mesh，每个 Mesh 有独立策略
- **多区域部署**: Multi-Zone 架构，单一全局控制面管理跨集群/区域的服务网格
- **零信任安全**: 自动 mTLS、MeshTrafficPermission 细粒度访问控制
- **可观测性**: 集成 Prometheus、Jaeger、Datadog 的指标、追踪和日志
- **流量管理**: 负载均衡、熔断、重试、超时、金丝雀发布

## 架构

Kuma 采用通用控制平面 + Envoy 数据面设计：

- **kuma-cp (Control Plane)**: 核心控制平面，管理 Mesh、策略和 Envoy 配置
- **kuma-dp (Data Plane Proxy)**: Envoy 代理包装器，在 Sidecar 或独立模式运行
- **Mesh CRD**: 定义 Mesh 实例，配置 mTLS、tracing、metrics 等全局策略
- **Policy CRDs**: MeshTrafficPermission、MeshRetry、MeshTimeout、MeshLoadBalancingStrategy 等
- **Zone Control Plane**: Multi-Zone 模式下的区域代理，连接全局控制面
- **Kuma Ingress**: 跨 Zone 流量的入口网关

策略执行流程：`Mesh 策略 → kuma-cp → xDS → kuma-dp (Envoy) → 流量拦截`

## K8s 集成

Kuma 通过 Helm Chart 以 Kubernetes 原生方式部署。kuma-cp 作为 Deployment 运行在控制面命名空间。通过 Mutating Webhook 自动向 Pod 注入 kuma-dp（Envoy）Sidecar。Mesh CRD 和各种策略 CRD 通过标准 Kubernetes API 管理，支持 GitOps。Multi-Zone 模式下，每个集群运行一个 kuma-cp（Zone CP），连接到全局 kuma-cp。Kuma 自动配置 Envoy 的 mTLS、流量拦截和可观测性。与 [[概念/kubernetes-architecture-overview.md|Kubernetes 架构]] 中的标准 Deployment/Service 和网络策略机制集成。

## 生产场景

1. **微服务安全**: 为所有微服务自动启用 mTLS 和细粒度访问控制
2. **混合部署**: Kubernetes Pod 和虚拟机统一纳入服务网格管理
3. **多区域容灾**: Multi-Zone 架构提供跨区域的服务发现和流量管理
4. **渐进式迁移**: 从部分服务开始纳入网格，逐步扩展到全集群

## 安装

```bash
# Helm 安装 Kuma
helm repo add kuma https://kumahq.github.io/charts
helm install kuma kuma/kuma -n kuma-system --create-namespace

# 等待控制面就绪
kubectl wait --for=condition=available deployment/kuma-control-plane -n kuma-system

# 为命名空间启用 Sidecar 注入
kubectl annotate namespace default kuma.io/sidecar-injection=enabled

# 创建 Mesh 策略（启用 mTLS）
kubectl apply -f - <<EOF
apiVersion: kuma.io/v1alpha1
kind: Mesh
metadata:
  name: default
spec:
  mtls:
    enabledBackend: ca-1
    backends:
    - name: ca-1
      type: builtin
  tracing:
    backends:
    - name: jaeger
      type: zipkin
      conf:
        url: http://jaeger-collector.observability.svc:9411/api/v2/spans
---
apiVersion: kuma.io/v1alpha1
kind: MeshTrafficPermission
metadata:
  name: allow-all
  namespace: kuma-system
spec:
  targetRef:
    kind: Mesh
  from:
  - targetRef:
      kind: Mesh
    default:
      action: Allow
EOF

# 访问 Kuma GUI
kubectl port-forward svc/kuma-control-plane -n kuma-system 5681:5681
```

## 对比

| 特性 | Kuma | Istio | Linkerd | Consul Connect |
|------|------|-------|---------|----------------|
| 多网格 | ✅ | ❌ | ❌ | ⚠️ |
| VM 支持 | ✅ | ⚠️ | ❌ | ✅ |
| 多区域 | ✅ | ⚠️ | ❌ | ✅ |
| 底层引擎 | Envoy | Envoy | Linkerd2-proxy | Envoy/HAProxy |
| CNCF 状态 | Sandbox | Graduated | Graduated | 非 CNCF |

## 架构定位

在 CNCF 生态中，Kuma 属于 **Service Mesh** 类别，为云原生应用提供通用服务网格控制平面能力。

## 参考链接

- [[实体/prometheus-grafana.md|prometheus-grafana]]
- [[deployment]]
- [[概念/service-mesh-architecture.md|service-mesh-architecture]]
- [[概念/gitops-principles.md|gitops-principles]]
- [[概念/observability-pillars.md|observability-pillars]]

## Related

- [[kubefleet]] — KubeFleet
- [[envoy]] — Envoy
- [[prometheus]] — Prometheus
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[jaeger]] — Jaeger

- kuma
- [[实体/cncf-networking.md|CNCF 网络与服务网格项目全景]] — Cross-reference
- [[生态参考/领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
