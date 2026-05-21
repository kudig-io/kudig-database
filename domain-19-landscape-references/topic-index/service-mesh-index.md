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
- cilium
- coredns
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
prerequisites:
- kubectl-basics
- cncf-ecosystem
- service-mesh-basics
- cilium-basics
- observability-basics
---

# Service Mesh 服务网格知识图谱索引

> 知识图谱：按主题 **Service Mesh** 聚合相关文档，按关联度分层级组织。

---

## 一、核心文档 (直接相关)

> 这些文档以 Service Mesh 为主题或直接面向服务网格运维场景。

### 架构与设计

- [[domain-01-cluster-fundamentals/14-service-mesh-architecture|服务网格与微服务架构设计]]
- [[domain-15-specialized-tech/11-service-mesh-overview|服务网格集成表]]
- [[domain-15-specialized-tech/12-service-mesh-advanced|服务网格进阶配置]]

### 主流方案

- [[domain-03-networking-traffic/01-istio-enterprise-service-mesh|Istio 企业级服务网格架构与实践]]
- [[domain-03-networking-traffic/02-linkerd-enterprise-service-mesh|Linkerd 企业级服务网格深度实践]]
- [[domain-03-networking-traffic/03-consul-connect-enterprise|Consul Connect 企业级服务网格管理]]
- [[domain-03-networking-traffic/04-envoy-proxy-enterprise|Envoy Proxy 企业级服务网格数据平面深度实践]]
- [[domain-03-networking-traffic/05-dapr-enterprise-distributed-runtime|Dapr (Distributed Application Runtime) Enterprise 深度实践]]
- [[domain-03-networking-traffic/06-traefik-mesh-enterprise|Traefik Mesh (Maesh) Enterprise Service Mesh 深度实践]]

### 入门指南

- [[domain-03-networking-traffic/99-istio-service-mesh-guide|Istio 企业级服务网格入门指南]]
- [[domain-03-networking-traffic/99-linkerd-service-mesh-guide|Linkerd 轻量级服务网格实践指南]]
- [[domain-03-networking-traffic/99-spring-cloud-kubernetes-service-mesh-guide|Spring Cloud Kubernetes 与服务网格集成指南]]

### 故障排查

- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/03-networking/05-service-mesh-istio-troubleshooting|Service Mesh (Istio) 深度排查与性能调优指南]]

### CNCF 生态

- [[domain-19-landscape-references/graduated/istio/istio|Istio]]
- [[domain-19-landscape-references/graduated/linkerd/linkerd|Linkerd]]
- [[domain-19-landscape-references/graduated/envoy/envoy|Envoy]]
- [[domain-19-landscape-references/graduated/cilium/cilium|Cilium]]

---

## 二、关联文档 (K8s 集成)

> 这些文档涉及 Service Mesh 但以其他 K8s 组件为主题。

### 网络与安全

- [[domain-03-networking-traffic/18-network-encryption-mtls|网络加密与mTLS]]
- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/03-networking/06-gateway-api-troubleshooting|Gateway API 深度排查与下一代流量治理指南]]
- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/03-networking/04-networkpolicy-troubleshooting|NetworkPolicy 深度排查与零信任安全治理指南]]

### 可观测性

- [[domain-17-system-foundation/topic-dictionary/observability/opentelemetry-and-distributed-tracing|OpenTelemetry 与分布式链路追踪]]
- [[domain-01-cluster-fundamentals/14-service-mesh-architecture|服务网格与微服务架构设计]]

### 技术论文

- [[domain-19-landscape-references/09-kubernetes-service-mesh-istio-integration|Kubernetes 服务网格深度实践与Istio集成]]

---

## 三、扩展参考

> 以下为 K8s 全域参考，Service Mesh 故障可参考网络、安全等章节。

### 网络相关

- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/03-networking/01-cni-troubleshooting|CNI 网络插件故障排查指南]]
- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/03-networking/03-service-ingress-troubleshooting|Service 与 Ingress 故障排查指南]]
- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/03-networking/02-dns-troubleshooting|CoreDNS/DNS 故障排查指南]]

### 安全相关

- [[domain-17-system-foundation/topic-dictionary/security/spiffe-spire-identity|SPIFFE / SPIRE 与工作负载身份]]

### 术语词典

- [[domain-17-system-foundation/topic-dictionary/networking/service-mesh|服务网格（Service Mesh）]]
- [[domain-17-system-foundation/topic-dictionary/networking/gateway-api|Gateway API]]
- [[domain-17-system-foundation/topic-dictionary/networking/network-policies|网络策略（Network Policies）]]
