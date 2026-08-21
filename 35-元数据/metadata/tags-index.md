---
title: 标签索引 (Tags Index)
description: 按技术标签聚合的文档索引，支持按主题快速检索知识库内容
summary: 按核心技术组件、网络、存储、安全、可观测性、AI/ML、方法论等维度聚合的文档索引
category: references
tags:
- tags-index
- meta
- navigation
- search
tier: supporting
created: '2026-05-23'
last_updated: '2026-07-21'
difficulty: beginner
audience:
- 所有工程师
estimated_read_time: 5min
---

# 标签索引 (Tags Index)

> 按技术标签聚合的文档索引，便于按主题快速检索。标签定义见 [[35-元数据/metadata/taxonomy.md|Tag Taxonomy]]。

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
- 19-故障诊断/06-FTA故障树/list/etcd-fta.md  # N5: 旧短路径 故障诊断/FTA故障树/ 修复

### api-server
- 集群基础/12: API Server 深度解析
- 故障诊断/01: API Server 故障排查
- 19-故障诊断/06-FTA故障树/list/apiserver-fta.md  # N5: 旧短路径 故障诊断/FTA故障树/ 修复

### scheduler
- 集群基础/20: Scheduler 深度解析
- 工作负载/30: 调度器配置
- 19-故障诊断/06-FTA故障树/list/scheduler-fta.md  # N5: 旧短路径 故障诊断/FTA故障树/ 修复

---

## 网络

### cni
- 集群基础/23: CNI 深度解析
- 网络/07: CNI 插件对比
- 19-故障诊断/06-FTA故障树/list/cni-fta.md  # N5: 旧短路径 故障诊断/FTA故障树/ 修复

### service
- 网络/11: Service 概念与类型
- 19-故障诊断/06-FTA故障树/list/service-fta.md  # N5: 旧短路径 故障诊断/FTA故障树/ 修复

### dns
- 网络/16: DNS 服务发现
- 故障诊断/26: DNS 故障排查
- 19-故障诊断/06-FTA故障树/list/dns-fta.md  # N5: 旧短路径 故障诊断/FTA故障树/ 修复

### ingress
- 网络/27: Ingress 基础
- 故障诊断/15: Ingress 故障排查
- 19-故障诊断/06-FTA故障树/list/ingress-fta.md  # N5: 旧短路径 故障诊断/FTA故障树/ 修复（原坏链接 [[26-技能/...]] 替换）

### gateway-api
- 网络/35: Gateway API 概览
- 19-故障诊断/06-FTA故障树/list/gateway-api-fta.md  # N5: 旧短路径 故障诊断/FTA故障树/ 修复

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
- 19-故障诊断/06-FTA故障树/list/rbac-fta.md  # N5: 旧短路径 故障诊断/FTA故障树/ 修复

### network-policy
- 安全/02: 网络安全策略
- 故障诊断/16: NetworkPolicy 故障排查
- 19-故障诊断/06-FTA故障树/list/networkpolicy-fta.md  # N5: 旧短路径 故障诊断/FTA故障树/ 修复

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
- 19-故障诊断/06-FTA故障树/01-23: FTA 方法论体系  # N5: 旧短路径 故障诊断/FTA故障树/ 修复
- 19-故障诊断/06-FTA故障树/list: 48 个组件故障树  # N5+M2: 旧短路径修复、统计 36→48

### febm
- 19-故障诊断/07-FEBM方法论/01-08: FEBM 取证方法论  # N5: 旧短路径 故障诊断/FEBM方法论/ 修复

### troubleshooting
- domain-12: 42+ 篇故障排查
- topic-structural-trouble-shooting: 结构化排障
- topic-skills: 18 个诊断-修复 Skill

---

## 平台与工程

### gitops
- [[11-发布变更/01-GitOps/index.md|01-GitOps]] — GitOps 工作流、ArgoCD、Flux
- [[27-标签/05-交付与运维/gitops|gitops 标签枢纽]]

### helm
- [[03-清单模式/03-Helm值模式/index.md|03-Helm值模式]] — Helm Chart 配置参考
- [[27-标签/05-交付与运维/helm|helm 标签枢纽]]

### operator
- [[07-数据库中间件/05-Operator管理/index.md|05-Operator管理]] — Operator 开发与管理
- [[27-标签/01-核心平台/operator|operator 标签枢纽]]

---

## 容器运行时

### docker
- [[14-容器运行时/01-Docker/index.md|01-Docker]] — Docker 架构与运维
- [[27-标签/01-核心平台/containerd|containerd 标签枢纽]]

### containerd
- [[14-容器运行时/03-containerd-CRI-O/index.md|03-containerd-CRI-O]] — containerd/CRI-O 运行时

---

## 可靠性

### sre
- [[12-可靠性/06-SRE实践/index.md|06-SRE实践]] — SRE 方法论与实践
- [[27-标签/05-交付与运维/sre|sre 标签枢纽]]

### chaos-engineering
- [[12-可靠性/04-混沌工程/index.md|04-混沌工程]] — 混沌工程实践

---

> 本索引将随文档 Frontmatter 的逐步添加而自动化生成。
> 标签定义权威源：[[35-元数据/metadata/taxonomy.md|Tag Taxonomy]]

## Related

- [[35-元数据/metadata/taxonomy.md|Tag Taxonomy]] — 标签定义权威源
- [[35-元数据/metadata/difficulty-index.md|难度分级索引]] — 按难度检索
- [[35-元数据/metadata/knowledge-map.md|知识图谱]] — 模块依赖关系
