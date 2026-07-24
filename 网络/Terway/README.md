---
title: 'Topic: Terway 专题 — 阿里云容器网络 (CNI) 全栈知识体系'
description: '# Topic: Terway 专题 — 阿里云容器网络 (CNI) 全栈知识体系'
summary: '本专题从 **产品、架构、使用、运维、测试、性能、CRD 操作、故障树** 八个维度系统梳理阿里云 Terway CNI 的完整知识体系，面向 ACK 集群网络架构师、SRE 和云原生开发者。'
category: terway
tags:
- k8s
- terway
- networking
- alicloud
- prometheus
- cilium
- flannel
- networkpolicy
- crd
- ebpf
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 网络工程师
estimated_read_time: 5min
intent_queries:
- 'Topic: Terway 专题 — 阿里云容器网络 (CNI) 全栈知识体系 是什么'
- '如何 Topic: Terway 专题 — 阿里云容器网络 (CNI) 全栈知识体系'
trigger_keywords:
- 'Topic:'
- Terway
- 专题
- 阿里云容器网络
- CNI
- 全栈知识体系
- terway
prerequisites:
- kubectl-basics
- networking-basics
- prometheus-basics
- ebpf-basics
- cilium-basics
- gpu-scheduling-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Topic: Terway 专题 — 阿里云容器网络 (CNI) 全栈知识体系

> **文档数量**: 8 篇专题 + 本索引 | **总行数**: ~7400 行 | **最后更新**: 2026-05 | **适用版本**: 阿里云 ACK v1.25 - v1.32+ | Terway v1.5+
>
> **全项目索引**: [terway-index.md](./terway-index.md) — 汇总仓库内全部 19 个 Terway 相关资源的跨目录统一导航

---

## 概述

本专题从 **产品、架构、使用、运维、测试、性能、CRD 操作、故障树** 八个维度系统梳理阿里云 Terway CNI 的完整知识体系，面向 ACK 集群网络架构师、SRE 和云原生开发者。

**适用读者**: SRE、网络架构师、ACK 用户、运维工程师

---

## 文档索引

| # | 文档 | 行数 | 主题 | 核心内容 |
|:---:|:---|:---:|:---|:---|
| 1 | [01-product.md](./01-product.md) | 330+ | 产品概览 | 定位与价值、版本历史(v1.0-v1.5)、5 种模式总览、CNI 对比(12 项)、依赖与限制、ECS 规格速查、适用场景 |
| 2 | [02-architecture.md](./02-architecture.md) | 950+ | 架构原理 | 整体架构图、控制面/数据面、5 种模式详解(含 ASCII 架构图)、IPAM 流程、5 个 CRD 模型、四层安全体系、CNI 规范集成、BoltDB 持久化 |
| 3 | [03-usage.md](./03-usage.md) | 730+ | 使用指南 | 安装初始化、5 种模式配置(YAML)、[[NetworkPolicy|NetworkPolicy]](iptables/Cilium)、固定 IP(PodNetworking/ReservedIP)、Pod 安全组、IPv6 双栈、Annotation 速查(20+)、容量规划 |
| 4 | [04-operations.md](./04-operations.md) | 860+ | 运维手册 | 健康检查脚本、GC 机制(设计原则/触发链路/参数调优)、[[Prometheus|Prometheus]] 告警(3 规则)、升级/回滚、故障排查决策树、IP 泄漏紧急处理、巡检清单、SRE 红线(7 条) |
| 5 | [05-testing.md](./05-testing.md) | 1040+ | 测试验证 | Pod 网络验证、6 类跨节点测试、NetworkPolicy 3 场景测试、ENI 密度压测(50 Pod)、固定 IP/GC/安全组验证、iperf3 基准、MTU 测试、端到端测试套件(可执行脚本) |
| 6 | [06-performance.md](./06-performance.md) | 600+ | 性能调优 | 5 模式性能基准、Pod 容量计算(5 规格)、内核调优(网卡多队列/sysctl/NUMA)、IP 池预热、eBPF 加速与迁移、生产基线(5 项指标+告警阈值)、性能故障排查 |
| 7 | [03b-crd-operations.md](./03b-crd-operations.md) | 740+ | CRD 操作 | 5 个 CRD 全量清单、PodENI/NodeNetworking/PodNetworking/ReservedIP/IPInstance 完整 CRUD、ConfigMap 管理(jq)、综合诊断脚本、命令速查表 |
| 8 | [07-troubleshooting-fta.md](./07-troubleshooting-fta.md) | 510+ | 故障树速查 | Mermaid FTA 全景图、6 大问题类别诊断、32 条错误信息目录、AND 门组合问题、SRE on-call 参考 |

---

## 快速导航

| 目标 | 推荐起点 |
|:---|:---|
| **快速了解 Terway** | [01-product.md](./01-product.md) — 产品定位、模式对比、CNI 选型 |
| **理解架构原理** | [02-architecture.md](./02-architecture.md) — 5 种模式数据流、IPAM、CRD 模型 |
| **上手使用配置** | [03-usage.md](./03-usage.md) — 安装、模式配置 YAML、NetworkPolicy、固定 IP |
| **生产运维排障** | [04-operations.md](./04-operations.md) — GC 机制、问题决策树、告警规则 |
| **网络测试验证** | [05-testing.md](./05-testing.md) — 端到端测试套件、压测、基准测试 |
| **CRD 资源管理** | [03b-crd-operations.md](./03b-crd-operations.md) — 5 个 CRD 完整 CRUD、诊断脚本 |
| **性能调优优化** | [06-performance.md](./06-performance.md) — 内核调优、eBPF 迁移、生产基线 |
| **SRE 故障树速查** | [07-troubleshooting-fta.md](./07-troubleshooting-fta.md) — FTA 全景图、错误信息目录、诊断命令表 |

---

## 交叉引用 — 库内关联文档

### Domain 知识库

| 文档 | 说明 |
|:---|:---|
| [网络/05-terway-advanced-guide.md](../网络/05-terway-advanced-guide.md) | Terway 高级指南（模式对比、ENIIP 详解、容量规划） |
| [网络/37-terway-resources-crud-operations.md](../网络/37-terway-resources-crud-operations.md) | Terway 实例 CRUD 操作指南（CRD 资源管理，1521 行） |
| [网络/38-terway-gc-mechanism.md](../网络/38-terway-gc-mechanism.md) | Terway GC 垃圾回收机制详解（942 行） |
| [网络/02-cni-architecture-fundamentals.md](../网络/02-cni-architecture-fundamentals.md) | CNI 架构基础与核心原理 |
| [网络/03-cni-plugins-comparison.md](../网络/03-cni-plugins-comparison.md) | CNI 插件对比与选型指南 |
| [网络/04-flannel-complete-guide.md](../网络/04-flannel-complete-guide.md) | Flannel 完整指南 |
| [网络/16-networkpolicy-deep-practice.md](../网络/16-networkpolicy-deep-practice.md) | NetworkPolicy 深度实践 |
| [网络/34-network-performance-tuning.md](../网络/34-network-performance-tuning.md) | 网络性能调优 |
| [云厂商/04-alicloud-ack/242-ack-vpc-network.md](../云厂商/04-alicloud-ack/242-ack-vpc-network.md) | ACK VPC 网络规划与 Terway 集成 |

### Topic 专题

| 文档 | 说明 |
|:---|:---|
| [生产运维/topic-presentations/kubernetes-terway-presentation.md](../生产运维/topic-presentations/kubernetes-terway-presentation.md) | Terway 全栈进阶培训（从入门到专家） |
| [生产运维/topic-learn/inner-training/week-4-network-storage/day-24-terway-cni.md](../生产运维/topic-learn/inner-training/week-4-network-storage/day-24-terway-cni.md) | Day 24: Terway CNI 入门学习（含实践任务） |
| [故障诊断/高级排障/structural-03-networking/07-terway-troubleshooting.md](../故障诊断/高级排障/03-networking/07-terway-troubleshooting.md) | Terway 结构化故障排查（634 行） |
| [故障诊断/topic-fta/list/terway-fta.md](../故障诊断/FTA故障树/list/terway-fta.md) | Terway 异常 FTA 故障树（含 JSON 工作流，879 行） |
| [系统基础/topic-cheat-sheet/](../系统基础/速查卡/) | 命令速查卡 (kubectl、网络诊断等) |

---

## 阅读建议

```
新手路径:     01-product → 02-architecture → 03-usage → 05-testing
SRE 路径:     07-troubleshooting-fta → 04-operations → 故障诊断/topic-fta/list/terway-fta → 06-performance
CRD 管理路径: 03-usage → 03b-crd-operations → 网络/37-terway-resources-crud-operations
架构师:       全部 8 篇 → 网络/05-terway-advanced-guide → 38-terway-gc-mechanism
培训讲师:     生产运维/topic-presentations/kubernetes-terway-presentation → 本专题精选
```

---

**Kusheet Project** | 作者: Allen Galler (allengaller@gmail.com)

## Related

- Domain-34: CNCF Landscape 开源项目 — Cross-reference
- [[实体/release-notes-networking.md|发布说明索引 — 网络]] — Cross-reference
- 网络 MOC — Cross-reference
- Topic 应用层架构设计最佳实践 — Cross-reference
- topic-application-architecture MOC — Cross-reference
- [[概念/bp-common-best-practices.md|Kubernetes 通用最佳实践参考]] — Cross-reference
- [[元数据/KUDIG Knowledge Base Architecture.md|KUDIG Knowledge Base Architecture]] — Cross-reference
- [[AI基础设施/基础设施/03-gpu-scheduling-management.md|GPU 调度与管理]] — Cross-reference
- [[AI基础设施/基础设施/05-distributed-training-frameworks.md|分布式训练框架]] — Cross-reference
- 发布变更 MOC — Cross-reference
- [[技能/learn-decision-tree-mermaid.md|故障排查决策树 - Mermaid 可视化版]] — Cross-reference
- [[技能/skill-22-daemonset-failure.md|DaemonSet 故障诊断与修复 / DaemonSet Failure Diagnosis & Remediation]] — Cross-reference
- [[平台工程/运维/06-monitoring-alerting-system.md|监控告警体系]] — Cross-reference
- Domain 30: 企业级灾备与业务连续性 (Enterprise Disaster Recovery & Business Continuity) — Cross-reference
- [[实体/ecosystem-changelog.md|生态组件变更日志索引]] — Cross-reference
- [[生态参考/领域索引/cluster-index.md|Cluster 集群知识图谱索引]]
- [[生态参考/领域索引/pvc-index.md|PVC 知识图谱索引]]
- [[生态参考/领域索引/terway-index.md|Terway 知识图谱索引]]
- [[生态参考/领域索引/nginx-ingress-index.md|nginx-ingress-controller 知识图谱索引]]
- [[生态参考/领域索引/higress-index.md|Higress 知识图谱索引]]


<!-- risk-assessed -->
