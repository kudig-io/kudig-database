---
title: DNS 知识图谱索引
description: '## DNS 知识图谱'
summary: '## DNS 知识图谱'
category: index
tags:
- k8s
- index
- catalog
- dns
- coredns
- service-discovery
- istio
- ingress
- networkpolicy
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 15min
intent_queries:
- DNS 知识图谱 是什么
- DNS CoreDNS 相关文档
trigger_keywords:
- DNS
- 知识图谱
- index
- CoreDNS
prerequisites:
- kubectl-basics
- cncf-ecosystem
- service-mesh-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# DNS 知识图谱索引

> 知识图谱：按关键字 **dns** 聚合项目内所有相关内容。

## 核心文档 (直接相关)

### 网络知识域 ([[entities/coredns.md|CoreDNS]]/DNS 核心)

- 04 - DNS 服务发现与 CoreDNS 调优
- 33 - 服务发现与 DNS 配置 ([[网络/00-core-k8s-networking/11-dns-service-discovery-coredns.md|11 dns service discovery coredns]]
- 53 - CoreDNS 架构与核心原理 (Architecture & Principles)
- 54 - CoreDNS Corefile 配置详解 (Corefile Configuration)
- 55 - CoreDNS 插件完整参考 (Plugins Reference)
- 56 - CoreDNS 故障排查与性能优化 (KUDIG 故障排查 Prompt 模板 & Optimization)

### 故障排查

- [[故障诊断/02-infrastructure-troubleshooting/25-network-connectivity-troubleshooting.md|25 - 网络连通性故障排查 (Network Connectivity Troubleshooting)]]
- [[故障诊断/02-infrastructure-troubleshooting/26-dns-troubleshooting.md|26 - DNS 故障排查 (DNS Troubleshooting)]]
- [[故障诊断/topic-structural-trouble-shooting/03-networking/02-dns-troubleshooting.md|CoreDNS/DNS 故障排查指南]]

### 术语词典

- [[系统基础/topic-dictionary/networking/dns-for-services-and-pods.md|DNS for Services and Pods]]
- [[系统基础/topic-dictionary/networking/service.md|Service]]
- [[系统基础/topic-dictionary/networking/endpointslices.md|EndpointSlices]]

## 关联文档 (K8s 集成)

### 网络相关

- [[故障诊断/topic-structural-trouble-shooting/03-networking/03-service-ingress-troubleshooting.md|Service 与 Ingress 故障排查指南]]
- [[故障诊断/topic-structural-trouble-shooting/03-networking/04-networkpolicy-troubleshooting.md|NetworkPolicy 深度排查与零信任安全治理指南]]
- [[故障诊断/topic-structural-trouble-shooting/03-networking/05-service-mesh-istio-troubleshooting.md|Service Mesh (Istio) 深度排查与性能调优指南]]
- [[系统基础/topic-dictionary/networking/cluster-mesh.md|多集群网络互联（Cluster Mesh）]]

### 技能卡片

- [[故障诊断/topic-skills/04-dns-resolution-failure.md|DNS 解析故障诊断与修复 / DNS Resolution Failure Diagnosis & Remediation]]

### FTA 故障树

- [[故障诊断/topic-fta/list/dns-fta.md|DNS 异常 FTA 树]]

## 扩展参考

### DNS 原理

- DNS 原理与配置

### CoreDNS 生态

- CoreDNS
- K8GB (Kubernetes Global Balancer)

### 培训演示

- Kubernetes CoreDNS 全栈进阶培训 (从入门到专家)
- Day 12: 网络栈 - CNI + Service + DNS


<!-- risk-assessed -->
