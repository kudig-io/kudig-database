---
title: Service Mesh 服务网格知识图谱索引
description: '## 知识图谱'
summary: '## 知识图谱'
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
tier: core
created: '2026-05-23'
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

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Service Mesh 服务网格知识图谱索引

> 知识图谱：按主题 **Service Mesh** 聚合相关文档，按关联度分层级组织。

---

## 一、核心文档 (直接相关)

> 这些文档以 Service Mesh 为主题或直接面向服务网格运维场景。

### 架构与设计

- 服务网格与微服务架构设计
- 服务网格集成表
- 服务网格进阶配置

### 主流方案

- Istio 企业级服务网格架构与实践
- Linkerd 企业级服务网格深度实践
- Consul Connect 企业级服务网格管理
- Envoy Proxy 企业级服务网格数据平面深度实践
- [[domain-16-database-middleware/01-databases/03-distributed-database-enterprise.md|03 distributed database enterprise]]
- Traefik Mesh (Maesh) Enterprise Service Mesh 深度实践

### 入门指南

- Istio 企业级服务网格入门指南
- Linkerd 轻量级服务网格实践指南
- [[domain-02-workloads-applications/00-core-workloads/99-spring-boot-kubernetes-guide.md|99 spring boot kubernetes guide]]

### 故障排查

- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/03-networking/05-service-mesh-istio-troubleshooting.md|Service Mesh (Istio) 深度排查与性能调优指南]]

### CNCF 生态

- Istio
- Linkerd
- Envoy
- Cilium

---

## 二、关联文档 (K8s 集成)

> 这些文档涉及 Service Mesh 但以其他 K8s 组件为主题。

### 网络与安全

- 网络加密与mTLS
- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/03-networking/06-gateway-api-troubleshooting.md|Gateway API 深度排查与下一代流量治理指南]]
- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/03-networking/04-networkpolicy-troubleshooting.md|NetworkPolicy 深度排查与零信任安全治理指南]]

### 可观测性

- [[domain-17-system-foundation/topic-dictionary/observability/opentelemetry-and-distributed-tracing.md|OpenTelemetry 与分布式链路追踪]]
- 服务网格与微服务架构设计

### 技术论文

- Kubernetes 服务网格深度实践与Istio集成

---

## 三、扩展参考

> 以下为 K8s 全域参考，Service Mesh 问题可参考网络、安全等章节。

### 网络相关

- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/03-networking/01-cni-troubleshooting.md|CNI 网络插件故障排查指南]]
- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/03-networking/03-service-ingress-troubleshooting.md|Service 与 Ingress 故障排查指南]]
- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/03-networking/02-dns-troubleshooting.md|CoreDNS/DNS 故障排查指南]]

### 安全相关

- [[domain-17-system-foundation/topic-dictionary/security/spiffe-spire-identity.md|SPIFFE / SPIRE 与工作负载身份]]

### 术语词典

- [[domain-17-system-foundation/topic-dictionary/networking/service-mesh.md|服务网格（Service Mesh）]]
- [[domain-17-system-foundation/topic-dictionary/networking/gateway-api.md|Gateway API]]
- [[domain-17-system-foundation/topic-dictionary/networking/network-policies.md|网络策略（Network Policies）]]


<!-- risk-assessed -->
