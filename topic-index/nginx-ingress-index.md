---
title: nginx-ingress-controller 知识图谱索引
description: nginx-ingress-controller 知识图谱索引，聚合 nginx-ingress 架构、Ingress 配置、TLS 终止、故障排查等所有相关内容
category: index
tags:
- k8s
- index
- catalog
- nginx
- nginx-ingress
- ingress
- gateway
- tls
- rag
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- nginx-ingress 知识图谱索引 是什么
- nginx-ingress-controller 相关内容
trigger_keywords:
- nginx-ingress
- nginx
- ingress
- TLS
- 502
- 503
---

# nginx-ingress-controller 知识图谱索引

> 知识图谱索引：按关键字 **nginx-ingress** 聚合项目内所有相关内容。

## 核心文档 (直接相关)

### nginx-ingress 核心文档
- [nginx-ingress 完全指南](./domain-5-networking/21-nginx-ingress-complete-guide.md)
- [kubernetes Ingress 基础概念与核心原理](./domain-5-networking/19-ingress-fundamentals.md)
- [Ingress Controller 深入剖析](./domain-5-networking/20-ingress-controller-deep-dive.md)
- [Ingress TLS 与证书管理](./domain-5-networking/22-ingress-tls-certificate.md)

### 故障排查
- [nginx-ingress 网关故障排查](./topic-structural-trouble-shooting/03-networking/09-nginx-ingress-troubleshooting.md)
- [nginx-ingress FTA 故障树](./topic-fta/list/nginx-ingress-fta.md)
- [Ingress/Gateway 路由故障诊断与修复](./topic-skills/13-ingress-gateway-failure.md)
- [Ingress 故障排查](./topic-structural-trouble-shooting/03-networking/03-service-ingress-troubleshooting.md)
- [Service 与 Ingress 故障排查指南](./topic-structural-trouble-shooting/03-networking/03-service-ingress-troubleshooting.md)
- [15 - Ingress 故障排查](./domain-12-troubleshooting/15-ingress-troubleshooting.md)

### 迁移指南
- [nginx-ingress 迁移指南](./domain-40-cloud-native-api-gateway/09-nginx-ingress-migration-guide.md)
- [与 Higress 迁移问题对照](./topic-structural-trouble-shooting/03-networking/09-nginx-ingress-troubleshooting.md#与-higress-迁移问题对照)

## 关联文档 (K8s集成)

### 网络与路由
- [CNI 容器网络接口深度解析](./domain-3-control-plane/23-container-network-deep-dive.md)
- [DNS 故障排查](./topic-structural-trouble-shooting/03-networking/02-dns-troubleshooting.md)
- [NetworkPolicy 零信任安全治理](./topic-structural-trouble-shooting/03-networking/04-networkpolicy-troubleshooting.md)
- [Service Mesh 故障排查](./topic-structural-trouble-shooting/03-networking/05-service-mesh-istio-troubleshooting.md)
- [Ingress 监控与故障排查](./domain-5-networking/25-ingress-monitoring-troubleshooting.md)

### 云厂商集成
- [ACK Ingress 配置](./domain-17-cloud-provider/04-alicloud-ack/)
- [AWS EKS Ingress](./domain-17-cloud-provider/01-aws-eks/)
- [GKE Ingress](./domain-17-cloud-provider/02-google-cloud-gke/)
- [Azure AKS Ingress](./domain-17-cloud-provider/03-azure-aks/)

### YAML 配置
- [Ingress YAML 配置参考](./domain-32-yaml-manifests/10-ingress-ingressclass.md)
- [IngressClass YAML](./domain-32-yaml-manifests/10-ingress-ingressclass.md)

## 扩展参考

### API 网关
- [Higress 企业级网关实践](./domain-40-cloud-native-api-gateway/04-higress-enterprise-gateway.md)
- [API 网关全景图](./domain-40-cloud-native-api-gateway/README.md)
- [Gateway API](./domain-34-cncf-landscape/incubating/gateway-api/gateway-api.md)
- [Envoy](./domain-34-cncf-landscape/graduated/envoy/envoy.md)

### Ingress 控制器对比
- [Ingress Controllers](./topic-dictionary/networking/ingress-controllers.md)
- [Contour](./domain-34-cncf-landscape/incubating/contour/contour.md)
- [Emissary-Ingress](./domain-34-cncf-landscape/incubating/emissary-ingress/emissary-ingress.md)

### 生产运维
- [网络性能优化](./domain-18-production-operations/20-network-performance-optimization.md)
- [企业级监控体系](./domain-18-production-operations/04-enterprise-monitoring-system.md)
- [零信任安全架构](./domain-18-production-operations/07-zero-trust-security-architecture.md)

### 术语词典
- [TLS/SSL 相关术语](./topic-dictionary/security/)
- [负载均衡相关术语](./topic-dictionary/networking/)

### 学习培训
- [Ingress 学习路径](./topic-learn/)
- [网络与存储周](./topic-learn/inner-training/week-4-network-storage/)
- [Day 23: Ingress](./topic-learn/inner-training/week-4-network-storage/day-23-ingress.md)
