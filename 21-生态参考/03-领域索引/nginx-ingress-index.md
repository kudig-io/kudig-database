---
title: nginx-ingress-controller 知识图谱索引
description: nginx-ingress-controller 知识图谱索引，聚合 nginx-ingress 架构、Ingress 配置、TLS 终止、故障排查等所有相关内容
summary: nginx-ingress-controller 知识图谱索引，聚合 nginx-ingress 架构、Ingress 配置、TLS 终止、故障排查等所有相关内容
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
tier: core
created: '2026-05-23'
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

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# nginx-ingress-controller 知识图谱索引

> 知识图谱索引：按关键字 **nginx-ingress** 聚合项目内所有相关内容。

## 核心文档 (直接相关)

### nginx-ingress 核心文档
- nginx-ingress 完全指南
- [[28-资产/presentations/kubernetes-ingress-presentation.md|kubernetes ingress presentation]]
- Ingress Controller 深入剖析
- Ingress TLS 与证书管理

### 故障排查
- nginx-ingress 网关故障排查
- [[19-故障诊断/06-FTA故障树/list/nginx-ingress-fta.md|nginx-ingress FTA 故障树]]
- [[19-故障诊断/08-技能体系/13-ingress-gateway-failure.md|Ingress/Gateway 路由故障诊断与修复]]
- [[19-故障诊断/04-高级排障/03-networking/03-service-ingress-troubleshooting.md|Ingress 故障排查]]
- [[19-故障诊断/04-高级排障/03-networking/03-service-ingress-troubleshooting.md|Service 与 Ingress 故障排查指南]]
- [[19-故障诊断/02-资源排障/15-ingress-troubleshooting.md|15 - Ingress 故障排查]]

### 迁移指南
- nginx-ingress 迁移指南
- 与 Higress 迁移问题对照

## 关联文档 (K8s集成)

### 网络与路由
- CNI 容器网络接口深度解析
- [[19-故障诊断/04-高级排障/03-networking/02-dns-troubleshooting.md|DNS 故障排查]]
- [[19-故障诊断/04-高级排障/03-networking/04-networkpolicy-troubleshooting.md|NetworkPolicy 零信任安全治理]]
- [[19-故障诊断/04-高级排障/03-networking/05-service-mesh-istio-troubleshooting.md|Service Mesh 故障排查]]
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
- [[05-网络/README.md|API 网关全景图]]
- Gateway API
- Envoy

### Ingress 控制器对比
- [[17-系统基础/06-知识字典/networking/ingress-controllers.md|Ingress Controllers]]
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
- [[26-技能/04-工作负载/pod/培训/inner-training/week-4-network-storage/day-23-ingress.md|day 23 ingress]]


<!-- risk-assessed -->
