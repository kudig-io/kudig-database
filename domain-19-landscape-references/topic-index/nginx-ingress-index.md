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
created: "2026-05-23"
---

# nginx-ingress-controller 知识图谱索引

> 知识图谱索引：按关键字 **nginx-[[Ingress|ingress]]** 聚合项目内所有相关内容。

## 核心文档 (直接相关)

### nginx-ingress 核心文档
- nginx-ingress 完全指南
- [[assets/presentations/kubernetes-ingress-presentation]]
- Ingress Controller 深入剖析
- Ingress TLS 与证书管理

### 故障排查
- troubleshooting-diagnostics/topic-structural-trouble-shooting/03-networking/09-nginx-ingress-troubleshooting|nginx-ingress 网关故障排查]]
- [[domain-10-troubleshooting-diagnostics/topic-fta/list/nginx-ingress-fta|nginx-[[[[Ingress 异常故障树分析|Ingress 异常故障树分析]]|ingress FTA]] 故障树]]
- [[domain-10-troubleshooting-diagnostics/topic-skills/13-ingress-gateway-failure|Ingress/Gateway 路由故障诊断与修复]]
- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/03-networking/03-service-ingress-troubleshooting|Ingress 故障排查]]
- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/03-networking/03-service-ingress-troubleshooting|[[Service 与 Ingress 故障排查指南|Service 与 Ingress 故障排查指南]]]]
- [[domain-10-troubleshooting-diagnostics/15-ingress-troubleshooting|15 - Ingress 故障排查]]

### 迁移指南
- nginx-ingress 迁移指南
- 与 Higress 迁移问题对照

## 关联文档 (K8s集成)

### 网络与路由
- CNI 容器网络接口深度解析
- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/03-networking/02-dns-troubleshooting|DNS 故障排查]]
- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/03-networking/04-networkpolicy-troubleshooting|NetworkPolicy 零信任安全治理]]
- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/03-networking/05-service-mesh-istio-troubleshooting|Service Mesh 故障排查]]
- Ingress 监控与故障排查

### 云厂商集成
- ACK Ingress 配置
- AWS EKS Ingress
- GKE Ingress
- Azure AKS Ingress

### YAML 配置
- Ingress YAML 配置参考
- IngressClass YAML

## 扩展参考

### API 网关
- Higress 企业级网关实践
- [[domain-03-networking-traffic/README|API 网关全景图]]
- Gateway API
- Envoy

### Ingress 控制器对比
- [[domain-17-system-foundation/topic-dictionary/networking/ingress-controllers|[[Ingress Controllers|Ingress Controllers]]]]
- Contour
- Emissary-Ingress

### 生产运维
- 网络性能优化
- 企业级监控体系
- 零信任安全架构

### 术语词典
- TLS/SSL 相关术语
- 负载均衡相关术语

### 学习培训
- Ingress 学习路径
- 网络与存储周
- [[skills/training-public/inner-training/week-4-network-storage/day-23-ingress]]
