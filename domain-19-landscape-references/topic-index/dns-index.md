---
title: DNS 知识图谱索引
description: '## DNS 知识图谱'
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

# DNS 知识图谱索引

> 知识图谱：按关键字 **dns** 聚合项目内所有相关内容。

## 核心文档 (直接相关)

### 网络知识域 (CoreDNS/DNS 核心)

- [[domain-03-networking-traffic/11-dns-service-discovery-coredns|04 - DNS 服务发现与 CoreDNS 调优]]
- [[domain-03-networking-traffic/12-dns-service-discovery|33 - 服务发现与 DNS 配置 (Service Discovery & DNS)]]
- [[domain-03-networking-traffic/13-coredns-architecture-principles|53 - CoreDNS 架构与核心原理 (Architecture & Principles)]]
- [[domain-03-networking-traffic/14-coredns-configuration-corefile|54 - CoreDNS Corefile 配置详解 (Corefile Configuration)]]
- [[domain-03-networking-traffic/15-coredns-plugins-reference|55 - CoreDNS 插件完整参考 (Plugins Reference)]]
- [[domain-03-networking-traffic/28-coredns-troubleshooting-optimization|56 - CoreDNS 故障排查与性能优化 (Troubleshooting & Optimization)]]

### 故障排查

- [[domain-10-troubleshooting-diagnostics/25-network-connectivity-troubleshooting|25 - 网络连通性故障排查 (Network Connectivity Troubleshooting)]]
- [[domain-10-troubleshooting-diagnostics/26-dns-troubleshooting|26 - DNS 故障排查 (DNS Troubleshooting)]]
- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/03-networking/02-dns-troubleshooting|CoreDNS/DNS 故障排查指南]]

### 术语词典

- [[domain-17-system-foundation/topic-dictionary/networking/dns-for-services-and-pods|DNS for Services and Pods]]
- [[domain-17-system-foundation/topic-dictionary/networking/service|Service]]
- [[domain-17-system-foundation/topic-dictionary/networking/endpointslices|EndpointSlices]]

## 关联文档 (K8s 集成)

### 网络相关

- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/03-networking/03-service-ingress-troubleshooting|Service 与 Ingress 故障排查指南]]
- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/03-networking/04-networkpolicy-troubleshooting|NetworkPolicy 深度排查与零信任安全治理指南]]
- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/03-networking/05-service-mesh-istio-troubleshooting|Service Mesh (Istio) 深度排查与性能调优指南]]
- [[domain-17-system-foundation/topic-dictionary/networking/cluster-mesh|多集群网络互联（Cluster Mesh）]]

### 技能卡片

- [[domain-10-troubleshooting-diagnostics/topic-skills/04-dns-resolution-failure|DNS 解析故障诊断与修复 / DNS Resolution Failure Diagnosis & Remediation]]

### FTA 故障树

- [[domain-10-troubleshooting-diagnostics/topic-fta/list/dns-fta|DNS 异常 FTA 树]]

## 扩展参考

### DNS 原理

- [[domain-03-networking-traffic/03-dns-principles-configuration|DNS 原理与配置]]

### CoreDNS 生态

- [[domain-19-landscape-references/graduated/coredns/coredns|CoreDNS]]
- [[domain-19-landscape-references/sandbox/k8gb/k8gb|K8GB (Kubernetes Global Balancer)]]

### 培训演示

- [[domain-11-production-operations/topic-presentations/kubernetes-coredns-presentation|Kubernetes CoreDNS 全栈进阶培训 (从入门到专家)]]
- [[domain-11-production-operations/topic-learn/public-training/one-month/week-2-core-tech/day-12-networking-1|Day 12: 网络栈 - CNI + Service + DNS]]
