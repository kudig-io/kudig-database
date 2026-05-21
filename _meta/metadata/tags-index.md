---
title: 标签索引 (Tags Index)
description: 'description: ''- domain-1: 架构概览、核心组件、性能调优'''
category: general
tags:
- meta
- etcd
- apiserver
- scheduler
- prometheus
- ingress
- gateway
- rbac
- networkpolicy
- gpu
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 标签索引 (Tags Index) 是什么
- 如何 标签索引 (Tags Index)
trigger_keywords:
- 标签索引
- Tags
- Index
prerequisites:
- kubectl-basics
- prometheus-basics
- etcd-basics
- gpu-scheduling-basics
---

title: 标签索引 (Tags Index)
description: '- domain-1: 架构概览、核心组件、性能调优'
category: general
tags:
- k8s
- etcd
- apiserver
- scheduler
- prometheus
- ingress
- gateway
- rbac
- networkpolicy
- gpu
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 标签索引 (Tags Index) 是什么
- 如何 标签索引 (Tags Index)
trigger_keywords:
- 标签索引
- Tags
- Index
authors:
- name: KUDIG Team
  role: contributor
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
sources: []
created: '2026-05-21'
updated: '2026-05-21'
---
# 标签索引 (Tags Index)

> 按技术标签聚合的文档索引，便于按主题快速检索

---

## 核心组件

### kubernetes
- domain-1: 架构概览、核心组件、性能调优
- domain-2: 设计原理、控制器模式
- domain-3: 控制平面全组件
- domain-17-system-foundation/topic-cheat-sheet/k8s.md

### etcd
- domain-01-cluster-fundamentals/07: 分布式共识 etcd
- domain-01-cluster-fundamentals/11: etcd 深度解析
- domain-10-troubleshooting-diagnostics/02: etcd 故障排查
- domain-10-troubleshooting-diagnostics/topic-fta/list/etcd-fta.md

### api-server
- domain-01-cluster-fundamentals/12: API Server 深度解析
- domain-10-troubleshooting-diagnostics/01: API Server 故障排查
- domain-10-troubleshooting-diagnostics/topic-fta/list/apiserver-fta.md

### scheduler
- domain-01-cluster-fundamentals/20: Scheduler 深度解析
- domain-02-workloads-applications/30: 调度器配置
- domain-10-troubleshooting-diagnostics/topic-fta/list/scheduler-fta.md

---

## 网络

### cni
- domain-01-cluster-fundamentals/23: CNI 深度解析
- domain-03-networking-traffic/07: CNI 插件对比
- domain-10-troubleshooting-diagnostics/topic-fta/list/cni-fta.md

### service
- domain-03-networking-traffic/11: Service 概念与类型
- domain-10-troubleshooting-diagnostics/topic-fta/list/service-fta.md

### dns
- domain-03-networking-traffic/16: DNS 服务发现
- domain-10-troubleshooting-diagnostics/26: DNS 故障排查
- domain-10-troubleshooting-diagnostics/topic-fta/list/dns-fta.md

### ingress
- domain-03-networking-traffic/27: Ingress 基础
- domain-10-troubleshooting-diagnostics/15: Ingress 故障排查
- domain-10-troubleshooting-diagnostics/topic-fta/list/ingress-fta.md

### gateway-api
- domain-03-networking-traffic/35: Gateway API 概览
- domain-10-troubleshooting-diagnostics/topic-fta/list/gateway-api-fta.md

---

## 存储

### pv-pvc
- domain-04-storage-data/02: PV 架构基础
- domain-10-troubleshooting-diagnostics/14: PVC 故障排查

### csi
- domain-01-cluster-fundamentals/22: CSI 深度解析
- domain-04-storage-data/05: CSI 驱动集成
- domain-10-troubleshooting-diagnostics/04: CSI 故障排查

---

## 安全

### rbac
- domain-05-security-compliance/01: 认证授权系统
- domain-10-troubleshooting-diagnostics/12: RBAC 故障排查
- domain-10-troubleshooting-diagnostics/topic-fta/list/rbac-fta.md

### network-policy
- domain-05-security-compliance/02: 网络安全策略
- domain-10-troubleshooting-diagnostics/16: NetworkPolicy 故障排查
- domain-10-troubleshooting-diagnostics/topic-fta/list/networkpolicy-fta.md

### runtime-security
- domain-05-security-compliance/03: 运行时安全防御

---

## 可观测性

### prometheus
- domain-06-observability/02: 监控指标系统
- domain-20: 企业监控告警
- domain-17-system-foundation/topic-cheat-sheet/promql.md

### tracing
- domain-06-observability/04: 分布式链路追踪

### logging
- domain-21: 日志管理分析

---

## AI/ML

### gpu
- domain-14-ai-ml-infra/03: GPU 调度管理
- domain-14-ai-ml-infra/04: GPU 监控 DCGM

### llm
- domain-14-ai-ml-infra/15-25: LLM 全生命周期
- topic-ai-agent: AI Agent 工程

### distributed-training
- domain-14-ai-ml-infra/05: 分布式训练框架

---

## 方法论

### fta
- domain-10-troubleshooting-diagnostics/topic-fta/01-23: FTA 方法论体系
- domain-10-troubleshooting-diagnostics/topic-fta/list: 36 个组件故障树

### febm
- domain-10-troubleshooting-diagnostics/topic-febm/01-08: FEBM 取证方法论

### troubleshooting
- domain-12: 42+ 篇故障排查
- topic-structural-trouble-shooting: 结构化排障
- topic-skills: 18 个诊断-修复 Skill

---

> 本索引将随文档 Frontmatter 的逐步添加而自动化生成

---

## Obsidian 相关文档

- [[metadata/difficulty-index.md|难度分级索引 (Difficulty Index)]]
- [[metadata/README.md|元数据索引 (Metadata)]]
- [[metadata/knowledge-map.md|知识图谱 (Knowledge Map)]]
