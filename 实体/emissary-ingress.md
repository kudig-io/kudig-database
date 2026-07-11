---
title: Emissary-Ingress (entities)
description: '## 概述'
summary: 'Emissary-Ingress（原 Ambassador API Gateway）是 Kubernetes 原生的 API 网关，基于 Envoy Proxy 构建。'
category: entities
tags:
- k8s
- cncf
- networking
- emissary-ingress
- prometheus
- grafana
- envoy
- containerd
- ingress
- gateway
tier: core
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Emissary-Ingress 是什么
- 如何 Emissary-Ingress
trigger_keywords:
- Emissary-Ingress
prerequisites:
- kubectl-basics
- prometheus-basics
- monitoring-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# Emissary-[[Ingress|Ingress]]

> **CNCF 状态**: Incubating | **类别**: Networking | **主要语言**: Python, Go

## 概述

Emissary-Ingress（原 Ambassador API Gateway）是 Kubernetes 原生的 API 网关，由 Ambassador Labs 开发，2021 年加入 CNCF 孵化。它基于 Envoy Proxy 构建，提供丰富的流量管理、认证授权和可观测性能力，是微服务架构的入口层解决方案。与传统的 Ingress Controller（如 nginx-ingress）不同，Emissary 的设计理念是"API Gateway as a Kubernetes-native service"——通过 CRD（Mapping、Host、Listener）声明式配置路由，无需 annotations 注解。Emissary 支持 Kubernetes Ingress API 和 Gateway API，可以作为标准 Ingress Controller 或功能更强大的 API Gateway 使用，提供金丝雀发布、A/B 测试、认证集成、速率限制等高级功能。

## 核心能力

- **Kubernetes 原生**: 通过 CRD（Mapping/Host/Listener）声明式配置，完全 GitOps 兼容
- **基于 Envoy**: 利用 Envoy 的高性能、可扩展性和丰富的过滤器
- **自助服务路由**: 开发者通过 Mapping CRD 自主配置路由规则
- **金丝雀发布**: 支持基于权重的流量切分和 A/B 测试
- **认证集成**: OAuth2、JWT、API Key、外部认证服务
- **速率限制**: 细粒度的流量控制，支持全局和局部限流

## 架构

Emissary-Ingress 采用 Envoy 代理 + 控制器模式：

- **Emissary Controller**: 监听 CRD（Mapping、Host、Listener、AuthService 等），生成 Envoy 配置
- **Envoy Proxy**: 实际数据面代理，处理入站流量并路由到后端服务
- **Mapping CRD**: 核心路由定义（前缀 → 后端服务映射、权重、超时、重试）
- **Host CRD**: 定义虚拟主机和 TLS 配置
- **Listener CRD**: 定义监听端口和协议
- **AuthService**: 外部认证服务集成（JWT、OAuth2）

数据流：`客户端 → Envoy → Mapping 匹配 → AuthService (可选) → RateLimit → 后端 Service`

## K8s 集成

Emissary-Ingress 以 Helm Chart 或 Operator 方式部署。Emissary Controller 作为 Deployment 运行，监听集群中的 Mapping/Host/Listener CRD。Controller 将 CRD 配置翻译为 Envoy 的 xDS 配置，通过 gRPC stream 推送给 Envoy 代理。Envoy 作为 Deployment 运行（多副本高可用），通过 Kubernetes Service 暴露（LoadBalancer 类型）。Emissary 支持 [[概念/kubernetes-architecture-overview.md|Kubernetes 架构]] 中的标准 Ingress API 和 Gateway API，可以作为集群的唯一入口。

## 生产场景

1. **微服务 API 网关**: 统一所有微服务的入口，提供认证、限流和路由
2. **金丝雀发布**: 通过 Mapping 权重实现新版本灰度发布
3. **gRPC API 代理**: 为 gRPC 微服务提供负载均衡和超时控制
4. **多租户 API**: 通过 Host CRD 为不同租户提供独立的虚拟主机和 TLS

## 安装

```bash
# Helm 安装 Emissary-Ingress
helm repo add datawire https://app.getambassador.io
helm install emissary-ingress datawire/emissary-ingress -n emissary-system --create-namespace \
  --set service.type=LoadBalancer

# 等待就绪
kubectl wait --for=condition=available deployment/emissary-ingress -n emissary-system

# 创建路由映射
kubectl apply -f - <<EOF
apiVersion: getambassador.io/v3alpha1
kind: Mapping
metadata:
  name: backend
spec:
  hostname: "*"
  prefix: /backend/
  service: backend-service.default:8080
  rate_limits:
  - resources:
    - "backend"
---
apiVersion: getambassador.io/v3alpha1
kind: Host
metadata:
  name: default-host
spec:
  hostname: "*"
  mappingSelector:
    matchLabels:
      ambassador: default
  tls:
    context: default-tls
EOF

# 金丝雀发布（10% 流量到新版本）
kubectl apply -f - <<EOF
apiVersion: getambassador.io/v3alpha1
kind: Mapping
metadata:
  name: backend-canary
spec:
  hostname: "*"
  prefix: /backend/
  service: backend-service-v2.default:8080
  weight: 10
EOF
```

## 对比

| 特性 | Emissary | nginx-ingress | Contour | Kong |
|------|----------|--------------|---------|------|
| 底层引擎 | Envoy | nginx | Envoy | nginx/OpenResty |
| API Gateway | ✅ | ⚠️ | ⚠️ | ✅ |
| CRD 配置 | ✅ Mapping | ⚠️ annotation | ✅ | ✅ |
| CNCF 状态 | Incubating | 非 CNCF | Graduated | 非 CNCF |

## 架构定位

在 CNCF 生态中，Emissary 属于 **Networking** 类别，为云原生应用提供 Kubernetes 原生 API 网关能力。

## 参考链接

- [[实体/prometheus-grafana.md|prometheus-grafana]]
- [[实体/crd-custom-resources.md|crd-custom-resources]]
- [[概念/secrets-management.md|secrets-management]]
- [[pod-lifecycle]]
- [[概念/security-defense-depth.md|security-defense-depth]]

## Related

- [[04-containerd-upgrade-migration]] — containerd 升级迁移
- [[spin]] — Spin
- [[backstage]] — Backstage
- [[envoy]] — Envoy
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- emissary-ingress
- [[实体/cncf-networking.md|CNCF 网络与服务网格项目全景]] — Cross-reference
- [[生态参考/领域索引/nginx-ingress-index.md|nginx-ingress-controller 知识图谱索引]]
- [[生态参考/领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
