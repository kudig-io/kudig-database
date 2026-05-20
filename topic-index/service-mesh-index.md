---
title: Service Mesh 服务网格知识图谱索引
description: '## 知识图谱'
category: index
tags:
- k8s
- index
- catalog
- service-mesh
- istio
- linkerd
- envoy
- microservice
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 20min
intent_queries:
- Service Mesh 服务网格知识图谱 是什么
- 如何 Service Mesh 服务网格知识图谱
trigger_keywords:
- Service
- Mesh
- 服务网格
- 知识图谱
- istio
- linkerd
- envoy
---

# Service Mesh 服务网格知识图谱索引

> 知识图谱：按主题 **Service Mesh** 聚合相关文档，按关联度分层级组织。

---

## 一、核心文档 (直接相关)

> 这些文档以 Service Mesh 为主题或直接面向服务网格运维场景。

### 架构与设计

- [服务网格与微服务架构设计](./domain-2-design-principles/14-service-mesh-architecture.md)
- [服务网格集成表](./domain-10-extensions/11-service-mesh-overview.md)
- [服务网格进阶配置](./domain-10-extensions/12-service-mesh-advanced.md)

### 主流方案

- [Istio 企业级服务网格架构与实践](./domain-26-service-mesh-microservices/01-istio-enterprise-service-mesh.md)
- [Linkerd 企业级服务网格深度实践](./domain-26-service-mesh-microservices/02-linkerd-enterprise-service-mesh.md)
- [Consul Connect 企业级服务网格管理](./domain-26-service-mesh-microservices/03-consul-connect-enterprise.md)
- [Envoy Proxy 企业级服务网格数据平面深度实践](./domain-26-service-mesh-microservices/04-envoy-proxy-enterprise.md)
- [Dapr (Distributed Application Runtime) Enterprise 深度实践](./domain-26-service-mesh-microservices/05-dapr-enterprise-distributed-runtime.md)
- [Traefik Mesh (Maesh) Enterprise Service Mesh 深度实践](./domain-26-service-mesh-microservices/06-traefik-mesh-enterprise.md)

### 入门指南

- [Istio 企业级服务网格入门指南](./domain-26-service-mesh-microservices/99-istio-service-mesh-guide.md)
- [Linkerd 轻量级服务网格实践指南](./domain-26-service-mesh-microservices/99-linkerd-service-mesh-guide.md)
- [Spring Cloud Kubernetes 与服务网格集成指南](./domain-26-service-mesh-microservices/99-spring-cloud-kubernetes-service-mesh-guide.md)

### 故障排查

- [Service Mesh (Istio) 深度排查与性能调优指南](./topic-structural-trouble-shooting/03-networking/05-service-mesh-istio-troubleshooting.md)

### CNCF 生态

- [Istio](./domain-34-cncf-landscape/graduated/istio/istio.md)
- [Linkerd](./domain-34-cncf-landscape/graduated/linkerd/linkerd.md)
- [Envoy](./domain-34-cncf-landscape/graduated/envoy/envoy.md)
- [Cilium](./domain-34-cncf-landscape/graduated/cilium/cilium.md)

---

## 二、关联文档 (K8s 集成)

> 这些文档涉及 Service Mesh 但以其他 K8s 组件为主题。

### 网络与安全

- [网络加密与mTLS](./domain-5-networking/18-network-encryption-mtls.md)
- [Gateway API 深度排查与下一代流量治理指南](./topic-structural-trouble-shooting/03-networking/06-gateway-api-troubleshooting.md)
- [NetworkPolicy 深度排查与零信任安全治理指南](./topic-structural-trouble-shooting/03-networking/04-networkpolicy-troubleshooting.md)

### 可观测性

- [OpenTelemetry 与分布式链路追踪](./topic-dictionary/observability/opentelemetry-and-distributed-tracing.md)
- [服务网格与微服务架构设计](./domain-2-design-principles/14-service-mesh-architecture.md)

### 技术论文

- [Kubernetes 服务网格深度实践与Istio集成](./domain-19-papers/09-kubernetes-service-mesh-istio-integration.md)

---

## 三、扩展参考

> 以下为 K8s 全域参考，Service Mesh 故障可参考网络、安全等章节。

### 网络相关

- [CNI 网络插件故障排查指南](./topic-structural-trouble-shooting/03-networking/01-cni-troubleshooting.md)
- [Service 与 Ingress 故障排查指南](./topic-structural-trouble-shooting/03-networking/03-service-ingress-troubleshooting.md)
- [CoreDNS/DNS 故障排查指南](./topic-structural-trouble-shooting/03-networking/02-dns-troubleshooting.md)

### 安全相关

- [SPIFFE / SPIRE 与工作负载身份](./topic-dictionary/security/spiffe-spire-identity.md)

### 术语词典

- [服务网格（Service Mesh）](./topic-dictionary/networking/service-mesh.md)
- [Gateway API](./topic-dictionary/networking/gateway-api.md)
- [网络策略（Network Policies）](./topic-dictionary/networking/network-policies.md)
