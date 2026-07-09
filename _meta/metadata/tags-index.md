---
title: 标签索引 (Tags Index) [metadata]
description: '- domain-1: 架构概览、核心组件、性能调优'
summary: '- domain-1: 架构概览、核心组件、性能调优'
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
tier: peripheral
created: '2026-05-23'
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

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 标签索引 (Tags Index)

> 按技术标签聚合的文档索引，便于按主题快速检索

---

## 核心组件

### kubernetes
- domain-1: 架构概览、核心组件、性能调优
- domain-2: 设计原理、控制器模式
- domain-3: 控制平面全组件
- 系统基础/topic-cheat-sheet/k8s.md

### etcd
- 集群基础/07: 分布式共识 etcd
- 集群基础/11: etcd 深度解析
- 故障诊断/02: etcd 故障排查
- 故障诊断/topic-fta/list/etcd-fta.md

### api-server
- 集群基础/12: API Server 深度解析
- 故障诊断/01: API Server 故障排查
- 故障诊断/topic-fta/list/apiserver-fta.md

### scheduler
- 集群基础/20: Scheduler 深度解析
- 工作负载/30: 调度器配置
- 故障诊断/topic-fta/list/scheduler-fta.md

---

## 网络

### cni
- 集群基础/23: CNI 深度解析
- 网络/07: CNI 插件对比
- 故障诊断/topic-fta/list/cni-fta.md

### service
- 网络/11: Service 概念与类型
- 故障诊断/topic-fta/list/service-fta.md

### dns
- 网络/16: DNS 服务发现
- 故障诊断/26: DNS 故障排查
- 故障诊断/topic-fta/list/dns-fta.md

### ingress
- 网络/27: Ingress 基础
- 故障诊断/15: Ingress 故障排查
- 故障诊断/topic-fta/[[skills/ingress-fta.md|ingress-fta]].md

### gateway-api
- 网络/35: Gateway API 概览
- 故障诊断/topic-fta/list/gateway-api-fta.md

---

## 存储

### pv-pvc
- 存储/02: PV 架构基础
- 故障诊断/14: PVC 故障排查

### csi
- 集群基础/22: CSI 深度解析
- 存储/05: CSI 驱动集成
- 故障诊断/04: CSI 故障排查

---

## 安全

### rbac
- 安全/01: 认证授权系统
- 故障诊断/12: RBAC 故障排查
- 故障诊断/topic-fta/list/rbac-fta.md

### network-policy
- 安全/02: 网络安全策略
- 故障诊断/16: NetworkPolicy 故障排查
- 故障诊断/topic-fta/list/networkpolicy-fta.md

### runtime-security
- 安全/03: 运行时安全防御

---

## 可观测性

### prometheus
- 可观测性/02: 监控指标系统
- domain-20: 企业监控告警
- 系统基础/topic-cheat-sheet/promql.md

### tracing
- 可观测性/04: 分布式链路追踪

### logging
- domain-21: 日志管理分析

---

## AI/ML

### gpu
- AI基础设施/03: GPU 调度管理
- AI基础设施/04: GPU 监控 DCGM

### llm
- AI基础设施/15-25: LLM 全生命周期
- 02-ai-agents: AI Agent 工程

### distributed-training
- AI基础设施/05: 分布式训练框架

---

## 方法论

### fta
- 故障诊断/topic-fta/01-23: FTA 方法论体系
- 故障诊断/topic-fta/list: 36 个组件故障树

### febm
- 故障诊断/topic-febm/01-08: FEBM 取证方法论

### troubleshooting
- domain-12: 42+ 篇故障排查
- topic-structural-trouble-shooting: 结构化排障
- topic-skills: 18 个诊断-修复 Skill

---

> 本索引将随文档 Frontmatter 的逐步添加而自动化生成


<!-- risk-assessed -->
