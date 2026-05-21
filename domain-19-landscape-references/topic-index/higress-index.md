---
title: Higress 知识图谱索引
description: Higress 云原生 API 网关知识图谱索引，聚合 Higress 架构、路由配置、服务发现、Wasm 插件、AI 网关等所有相关内容
category: index
tags:
- k8s
- index
- catalog
- higress
- istio
- envoy
- ingress
- gateway
- api-gateway
- wasm
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Higress 知识图谱索引 是什么
- Higress API 网关相关内容
trigger_keywords:
- Higress
- API 网关
- Ingress
- Envoy
- Wasm
- Nacos
prerequisites:
- kubectl-basics
- cncf-ecosystem
- service-mesh-basics
---

# Higress 知识图谱索引

> 知识图谱索引：按关键字 **Higress** 聚合项目内所有相关内容。

## 核心文档 (直接相关)

### Higress 核心文档
- [[domain-03-networking-traffic/04-higress-enterprise-gateway|Higress 企业级网关实践]]
- [[domain-03-networking-traffic/README|云原生 API 网关全景图]]

### 故障排查
- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/03-networking/09-higress-troubleshooting|Higress 网关故障排查指南]]
- [[domain-10-troubleshooting-diagnostics/topic-fta/list/higress-fta|Higress FTA 故障树]]

### 域名文档
- [[domain-03-networking-traffic/|API 网关选型对比]]

## 关联文档 (K8s集成)

### Ingress 相关
- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/03-networking/03-service-ingress-troubleshooting|Ingress 故障排查]]
- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/03-networking/03-service-ingress-troubleshooting|Service 与 Ingress 故障排查指南]]
- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/03-networking/02-dns-troubleshooting|DNS 故障排查]]
- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/03-networking/|Service 端点异常]]

### 服务网格
- [[domain-03-networking-traffic/|服务网格与微服务架构]]
- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/03-networking/05-service-mesh-istio-troubleshooting|Istio 故障排查]]
- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/03-networking/05-service-mesh-istio-troubleshooting|Service Mesh 深度排查与性能调优]]

### Envoy 与网关
- [[domain-19-landscape-references/graduated/envoy/envoy|Envoy 深度解析]]
- [[domain-19-landscape-references/incubating/gateway-api/gateway-api|Gateway API]]
- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/03-networking/06-gateway-api-troubleshooting|Gateway API 深度排查与下一代流量治理]]

### 迁移指南
- [[domain-03-networking-traffic/09-nginx-ingress-migration-guide|nginx-ingress 迁移指南]]
- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/03-networking/09-nginx-ingress-troubleshooting#与-higress-迁移问题对照|与 Higress 迁移问题对照]]

## 扩展参考

### 云厂商集成
- [[domain-12-cloud-providers/04-alicloud-ack/|阿里云 ACK]]
- [[domain-12-cloud-providers/04-alicloud-ack/|阿里云 ASM (阿里云服务网格)]]
- [[domain-12-cloud-providers/04-alicloud-ack/|阿里云 MSE (微服务引擎)]]

### CNCF 生态
- [[domain-19-landscape-references/incubating/istio/istio|Istio]]
- [[domain-19-landscape-references/graduated/kubeedge/kubeedge|KubeEdge]]
- [[domain-19-landscape-references/graduated/envoy/envoy|Envoy]]

### 生产运维
- [[domain-11-production-operations/04-enterprise-monitoring-system|企业级监控体系]]
- [[domain-11-production-operations/07-zero-trust-security-architecture|零信任安全架构]]
- [[domain-11-production-operations/20-network-performance-optimization|网络性能优化]]

### 术语词典
- [[domain-17-system-foundation/topic-dictionary/networking/ingress-controllers|Ingress Controllers]]
- [[domain-17-system-foundation/topic-dictionary/networking/|服务网格术语]]
- [[domain-17-system-foundation/topic-dictionary/|API 相关术语]]

### 学习路径
- [[domain-11-production-operations/topic-learn/|服务网格学习路径]]
- [[domain-11-production-operations/topic-learn/|Ingress 学习路径]]

### YAML 配置
- [[domain-18-manifests-patterns/|Ingress YAML 配置参考]]
- [[domain-18-manifests-patterns/|IngressClass YAML]]
