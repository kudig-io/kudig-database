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
- istio
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
prerequisites:
- kubectl-basics
- cncf-ecosystem
- service-mesh-basics
---

# nginx-ingress-controller 知识图谱索引

> 知识图谱索引：按关键字 **nginx-ingress** 聚合项目内所有相关内容。

## 核心文档 (直接相关)

### nginx-ingress 核心文档
- [[domain-03-networking-traffic/21-nginx-ingress-complete-guide|nginx-ingress 完全指南]]
- [[domain-03-networking-traffic/19-ingress-fundamentals|kubernetes Ingress 基础概念与核心原理]]
- [[domain-03-networking-traffic/20-ingress-controller-deep-dive|Ingress Controller 深入剖析]]
- [[domain-03-networking-traffic/22-ingress-tls-certificate|Ingress TLS 与证书管理]]

### 故障排查
- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/03-networking/09-nginx-ingress-troubleshooting|nginx-ingress 网关故障排查]]
- [[domain-10-troubleshooting-diagnostics/topic-fta/list/nginx-ingress-fta|nginx-ingress FTA 故障树]]
- [[domain-10-troubleshooting-diagnostics/topic-skills/13-ingress-gateway-failure|Ingress/Gateway 路由故障诊断与修复]]
- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/03-networking/03-service-ingress-troubleshooting|Ingress 故障排查]]
- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/03-networking/03-service-ingress-troubleshooting|Service 与 Ingress 故障排查指南]]
- [[domain-10-troubleshooting-diagnostics/15-ingress-troubleshooting|15 - Ingress 故障排查]]

### 迁移指南
- [[domain-03-networking-traffic/09-nginx-ingress-migration-guide|nginx-ingress 迁移指南]]
- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/03-networking/09-nginx-ingress-troubleshooting#与-higress-迁移问题对照|与 Higress 迁移问题对照]]

## 关联文档 (K8s集成)

### 网络与路由
- [[domain-01-cluster-fundamentals/23-container-network-deep-dive|CNI 容器网络接口深度解析]]
- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/03-networking/02-dns-troubleshooting|DNS 故障排查]]
- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/03-networking/04-networkpolicy-troubleshooting|NetworkPolicy 零信任安全治理]]
- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/03-networking/05-service-mesh-istio-troubleshooting|Service Mesh 故障排查]]
- [[domain-03-networking-traffic/25-ingress-monitoring-troubleshooting|Ingress 监控与故障排查]]

### 云厂商集成
- [[domain-12-cloud-providers/04-alicloud-ack/|ACK Ingress 配置]]
- [[domain-12-cloud-providers/01-aws-eks/|AWS EKS Ingress]]
- [[domain-12-cloud-providers/02-google-cloud-gke/|GKE Ingress]]
- [[domain-12-cloud-providers/03-azure-aks/|Azure AKS Ingress]]

### YAML 配置
- [[domain-18-manifests-patterns/10-ingress-ingressclass|Ingress YAML 配置参考]]
- [[domain-18-manifests-patterns/10-ingress-ingressclass|IngressClass YAML]]

## 扩展参考

### API 网关
- [[domain-03-networking-traffic/04-higress-enterprise-gateway|Higress 企业级网关实践]]
- [[domain-03-networking-traffic/README|API 网关全景图]]
- [[domain-19-landscape-references/incubating/gateway-api/gateway-api|Gateway API]]
- [[domain-19-landscape-references/graduated/envoy/envoy|Envoy]]

### Ingress 控制器对比
- [[domain-17-system-foundation/topic-dictionary/networking/ingress-controllers|Ingress Controllers]]
- [[domain-19-landscape-references/incubating/contour/contour|Contour]]
- [[domain-19-landscape-references/incubating/emissary-ingress/emissary-ingress|Emissary-Ingress]]

### 生产运维
- [[domain-11-production-operations/20-network-performance-optimization|网络性能优化]]
- [[domain-11-production-operations/04-enterprise-monitoring-system|企业级监控体系]]
- [[domain-11-production-operations/07-zero-trust-security-architecture|零信任安全架构]]

### 术语词典
- [[domain-17-system-foundation/topic-dictionary/security/|TLS/SSL 相关术语]]
- [[domain-17-system-foundation/topic-dictionary/networking/|负载均衡相关术语]]

### 学习培训
- [[domain-11-production-operations/topic-learn/|Ingress 学习路径]]
- [[domain-11-production-operations/topic-learn/inner-training/week-4-network-storage/|网络与存储周]]
- [[domain-11-production-operations/topic-learn/inner-training/week-4-network-storage/day-23-ingress|Day 23: Ingress]]
