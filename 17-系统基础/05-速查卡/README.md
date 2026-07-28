---
title: 速查卡 (Cheat Sheet)
description: 覆盖 Kubernetes 生产运维全场景的快速参考卡片集
summary: 覆盖 Kubernetes 生产运维全场景的快速参考卡片集
category: cheatsheet
tags:
- cheatsheet
- quick-reference
- prometheus
- helm
- containerd
- docker
- mysql
- postgresql
- rbac
- rag
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 速查卡 (Cheat Sheet) 是什么
- 如何 速查卡 (Cheat Sheet)
trigger_keywords:
- 速查卡
- Cheat
- Sheet
- cheat
- sheet
prerequisites:
- kubectl-basics
- cloud-provider-basics
- helm-basics
- prometheus-basics
- mysql-basics
- gpu-scheduling-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 速查卡 (Cheat Sheet)

> 覆盖 [[kubernetes|Kubernetes]] 生产运维全场景的快速参考卡片集

## 概述

本目录包含 **9 张** 精心编写的技术速查卡，面向生产环境运维工程师和开发者，提供命令、语法、配置的快速参考。每张速查卡均经过生产验证，包含真实场景示例。

## 速查卡索引

| # | 速查卡 | 内容覆盖 | 适用版本 | 大小 |
|:---:|:---|:---|:---|:---:|
| 1 | [Kubernetes 速查卡](./k8s.md) | kubectl 命令、集群管理、Pod 操作、网络、存储、RBAC、排障 | v1.25-v1.32 | 37KB |
| 2 | Linux 速查卡](./linux.md) | 系统管理、进程、网络、存储、安全、Shell 脚本 | RHEL 7-9, Ubuntu 20-24 | 44KB |
| 3 | Go 语言速查卡](./go.md) | 语法、并发、网络、数据库、测试、性能优化 | Go 1.20-1.22 | 49KB |
| 4 | Docker/Containerd 速查卡](./docker.md) | 容器生命周期、镜像管理、网络、存储、Compose、ctr | Docker 20.10+, [[containerd|containerd]] 1.6+ | 11KB |
| 5 | [PromQL 速查卡](./promql.md) | 指标查询、聚合函数、Kubernetes 监控、告警规则 | Prometheus 2.40+ | 11KB |
| 6 | [网络诊断速查卡](./networking.md) | DNS 诊断、TCP 调试、HTTP 测试、抓包分析、K8s 网络 | TCP/IP | 14KB |
| 7 | [Git 速查表](./git.md) | 日常操作、分支管理、撤销操作、故障排查 | Git 2.30+ | 12KB |
| 8 | [SQL 速查表](./sql.md) | 查询语法、表操作、索引优化、数据库管理 | MySQL 8.0, PostgreSQL 14 | 20KB |
| 9 | [TLS/PKI 速查卡](./tls-pki.md) | 证书格式、OpenSSL 命令、证书链、K8s 证书管理、监控脚本 | x509, TLS 1.2/1.3 | 11KB |

## 使用场景

### 日常运维速查
``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 快速查找 kubectl 命令
open 系统基础/topic-cheat-sheet/k8s.md

# Linux 性能排查命令
open 系统基础/topic-cheat-sheet/linux.md
```
### 导入 AI 知识库
- **NotebookLM**: 导入整个目录作为速查参考源
- **IMA / 豆包**: 适合日常术语和命令查询
- **RAG 应用**: 作为快速检索层，配合 domain-* 深度内容

### 打印/离线使用
每张速查卡设计为可独立使用的完整参考文档，适合打印或导出 PDF。

## 与其他模块的关系

| 速查卡 | 深度知识来源 | 故障排查 |
|:---|:---|:---|
| k8s.md | 集群基础 ~ 故障诊断 | domain-12, topic-fta |
| linux.md | 系统基础 | 故障诊断/35 |
| docker.md | 容器运行时 | 故障诊断/08 |
| promql.md | domain-8, 可观测性 | 故障诊断/30 |
| networking.md | domain-5, 网络 | 故障诊断/25-26 |
| tls-pki.md | 安全 | 故障诊断/13 |
| git.md | 发布变更 | - |
| sql.md | 数据库中间件 | - |
| go.md | 集群基础 (源码阅读) | - |

## 贡献指南

新增速查卡请遵循以下规范：
- 文件名使用小写连字符格式（如 `helm.md`）
- 每个条目包含：命令/语法 + 简要说明 + 示例
- 标注适用版本范围
- 优先收录生产环境高频操作

## Related

- Domain-34: CNCF Landscape 开源项目 — Cross-reference
- [[23-实体/15-参考与索引/release-notes-networking.md|发布说明索引 — 网络]] — Cross-reference
- 网络 MOC — Cross-reference
- Topic 应用层架构设计最佳实践 — Cross-reference
- topic-application-architecture MOC — Cross-reference
- [[22-概念/10-最佳实践/bp-common-best-practices.md|Kubernetes 通用最佳实践参考]] — Cross-reference
- [[35-元数据/metadata/KUDIG Knowledge Base Architecture.md|KUDIG Knowledge Base Architecture]] — Cross-reference
- [[15-AI基础设施/01-基础设施/03-gpu-scheduling-management.md|GPU 调度与管理]] — Cross-reference
- [[15-AI基础设施/01-基础设施/05-distributed-training-frameworks.md|分布式训练框架]] — Cross-reference
- 发布变更 MOC — Cross-reference
- [[26-技能/04-工作负载/pod/培训/learn-decision-tree-mermaid.md|故障排查决策树 - Mermaid 可视化版]] — Cross-reference
- [[26-技能/04-工作负载/daemonset/skill-22-daemonset-failure.md|DaemonSet 故障诊断与修复 / DaemonSet Failure Diagnosis & Remediation]] — Cross-reference
- [[10-平台工程/02-运维/06-monitoring-alerting-system.md|监控告警体系]] — Cross-reference
- Domain 30: 企业级灾备与业务连续性 (Enterprise Disaster Recovery & Business Continuity) — Cross-reference
- [[23-实体/15-参考与索引/ecosystem-changelog.md|生态组件变更日志索引]] — Cross-reference
- [[21-生态参考/03-领域索引/cluster-index.md|Cluster 集群知识图谱索引]]
- [[21-生态参考/03-领域索引/pvc-index.md|PVC 知识图谱索引]]
- [[21-生态参考/03-领域索引/terway-index.md|Terway 知识图谱索引]]
- [[21-生态参考/03-领域索引/nginx-ingress-index.md|nginx-ingress-controller 知识图谱索引]]
- [[21-生态参考/03-领域索引/higress-index.md|Higress 知识图谱索引]]


<!-- risk-assessed -->
