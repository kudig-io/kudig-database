---
title: etcd
description: etcd 是一个高可用的分布式键值存储系统，是 Kubernetes 集群的核心数据存储。集群的所有状态信息（包括 Pod、Service、ConfigMap、...
summary: etcd 是一个高可用的分布式键值存储系统，是 Kubernetes 集群的核心数据存储。集群的所有状态信息（包括 Pod、Service、ConfigMap、...
category: dictionary
tags:
- k8s
- glossary
- etcd
- control-plane
- storage
tier: core
created: '2026-06-24'
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- etcd 是什么
- etcd 详解
trigger_keywords:
- etcd
- dictionary
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# etcd

> **英文名**: etcd

## 概述

etcd 是一个高可用的分布式键值存储系统，是 Kubernetes 集群的核心数据存储。集群的所有状态信息（包括 Pod、Service、ConfigMap、Secret 等所有资源对象）都持久化在 etcd 中。

## 核心概念/原理

### 核心特性

- **强一致性**：基于 Raft 共识算法，保证所有读取返回最新数据。
- **Watch 机制**：支持对 key 或 key 前缀的变更监听，是 Kubernetes 事件驱动架构的基础。
- **事务支持**：支持多 key 的原子操作。
- **MVCC 存储**：使用多版本并发控制，保留 key 的历史版本。

### 在 Kubernetes 中的角色

API Server 是唯一直接与 etcd 通信的组件。所有 Kubernetes 对象通过 API Server 读写 etcd。etcd 中的数据变更触发控制器和 Informer 的响应。

## 关键机制或特性

- etcd 集群推荐至少 3 个成员以实现容错（可容忍 1 个节点故障）。
- 5 个成员的集群可容忍 2 个节点故障，适合大规模生产环境。
- 需要定期执行 compaction（压缩历史版本）和 defragmentation（回收空间）。
- 备份策略：定期执行 `etcdctl snapshot save` 并存储在异地。

## 使用场景与最佳实践

- **性能**：使用 SSD 存储，避免网络延迟；大规模集群考虑独立 etcd 集群。
- **安全**：启用 TLS 加密所有 etcd 通信（peer 和 client）。
- **备份**：实施自动化备份策略，定期验证备份可恢复性。
- **监控**：关注 WAL fsync 延迟、backend commit 延迟等关键指标。
- **版本**：Kubernetes 对 etcd 版本有严格要求，参见兼容性矩阵。

## 参考链接

- [etcd - Official Documentation](https://etcd.io/docs/)

## Related

[[17-系统基础/06-知识字典/fundamentals/storage-versions.md|存储版本]]


<!-- risk-assessed -->
- [[01-集群基础/01-架构总览/04-source-code-structure|04 - Kubernetes 源码结构深度解析]]
- [[01-集群基础/01-架构总览/14-troubleshooting-guide|16 - Kubernetes 故障排查专家级指南]]
- [[01-集群基础/01-架构总览/16-kubernetes-core-components-v1.29-v1.33-update|Kubernetes 核心组件 v1.29 - v1.33 新特性速查]]
- [[01-集群基础/02-设计原则/03-declarative-api-pattern|02 - 声明式 API 与面向终态设计 (Declarative API)]]
- [[01-集群基础/02-设计原则/05-watch-list-mechanism|04 - List-Watch 机制深度解析 (List-Watch)]]
- [[01-集群基础/02-设计原则/15-service-mesh-architecture|14 - 服务网格与微服务架构设计]]
- [[01-集群基础/03-控制平面/11-etcd-deep-dive|etcd 深度解析]]
- [[01-集群基础/03-控制平面/02-plane-components-interaction|控制平面组件交互详解 (Control Plane Components Interaction Deep Dive)]]
- [[01-集群基础/03-控制平面/12-apiserver-deep-dive|kube-apiserver 深度解析]]
- [[02-工作负载/00-总览/02-kubernetes-multi-tenant-architecture|Kubernetes 多租户与资源隔离生产架构]]
- [[03-清单模式/01-YAML参考/32-lease-event-node|32 - Lease / Event / Node YAML 配置参考]]
- [[03-清单模式/01-YAML参考/09-endpoints-endpointslice|09 - Endpoints / EndpointSlice YAML 配置参考]]
- [[04-应用模式/02-行业架构/69-6g-core-network|6G 核心网架构设计 — 阿里云视角]]
- [[04-应用模式/02-行业架构/23-xinchuang-it-innovation|信创替代架构设计 — 阿里云视角]]
- [[05-网络/01-K8s网络核心/16-coredns-plugins-reference|55 - CoreDNS 插件完整参考 (Plugins Reference)]]
- [[07-数据库中间件/01-数据库/02-postgresql-enterprise-database|PostgreSQL 企业级数据库高可用架构]]
- [[08-安全/06-合规审计/10-compliance-audit-practices|Kubernetes 合规与审计]]
- [[10-平台工程/02-运维/19-kubernetes-v1.33-platform-ops-guide|Kubernetes v1.29-v1.33 平台运维新特性指南]]
- [[10-平台工程/02-运维/09-backup-recovery-strategy|Kubernetes 备份与恢复概述 (Backup & Recovery Overview)]]
- [[10-平台工程/03-治理/02-performance-benchmarking-tuning|性能基准测试与调优 (Performance Benchmarking & Tuning)]]
- [[11-发布变更/07-迁移方案/06-stateful-services-migration|06 - 有状态服务迁移 [migration]]]
- [[15-AI基础设施/02-AI-Agents/19-agentscope-memory-context|AgentScope 记忆管理与上下文工程 (AI基础设施)]]
- [[15-AI基础设施/02-AI-Agents/15-agent-corpus-gap-analysis|Agent 语料库差距分析：kudig-database 作为 K8s 运维 Agent 语料还缺什么？ [02-ai-agents]]]
- [[15-AI基础设施/02-AI-Agents/49-openclaw-memory-mechanism|OpenClaw MEMORY.md 机制深度解析 (AI基础设施)]]
- [[15-AI基础设施/02-AI-Agents/47-openclaw-tools-mechanism|OpenClaw TOOLS.md 机制深度解析 (AI基础设施)]]
- [[15-AI基础设施/02-AI-Agents/11-cost-latency-optimization|成本与延迟优化策略 (AI基础设施)]]
- [[15-AI基础设施/02-AI-Agents/48-openclaw-skill-mechanism|OpenClaw SKILL.md 机制深度解析 (AI基础设施)]]
- [[15-AI基础设施/02-AI-Agents/44-openclaw-soul-mechanism|OpenClaw SOUL.md 机制深度解析 (AI基础设施)]]
- [[15-AI基础设施/02-AI-Agents/openclaw-workspace/IDENTITY|KuDig Doctor — 身份标识 (02-ai-agents)]]
- [[15-AI基础设施/02-AI-Agents/openclaw-workspace/MEMORY|记忆系统 (02-ai-agents)]]
- [[15-AI基础设施/02-AI-Agents/openclaw-workspace/USER|用户画像 — ACK 运维工程师 (02-ai-agents)]]
- [[15-AI基础设施/02-AI-Agents/openclaw-workspace/SOUL|KuDig Doctor — 角色人格与绝对红线 (02-ai-agents)]]
- [[17-系统基础/02-硬件/18-hardware-failure-case-studies|硬件问题实战案例库]]
- [[17-系统基础/04-K8s事件/13-security-admission-rbac-events|13 - 安全、准入控制与 RBAC 事件]]
- [[17-系统基础/04-K8s事件/01-event-system-architecture|01 - Kubernetes 事件系统架构与 API 参考]]
- [[19-故障诊断/01-核心排障/02-control-plane-etcd-troubleshooting|etcd 故障排查]]
- [[19-故障诊断/03-基础设施排障/07-backup-restore-troubleshooting|31 - 备份恢复故障排查 (Backup and Restore Troubleshooting)]]
- [[19-故障诊断/04-高级排障/09-symptom-sop-mapping|症状 → SOP 映射手册]]
- [[19-故障诊断/04-高级排障/structural-symptom-mapping-layer|症状快速映射层 (Symptom-SOP-RootCause Mapping) [topic-structural-trouble-shooting]]]
- [[19-故障诊断/06-FTA故障树/22-industry-standardization|第二十二章：行业标准化建议 (故障诊断)]]
- [[19-故障诊断/10-QA语料/command-output-diagnosis|命令输出解读语料 — Agent 诊断推理核心数据 [故障诊断]]]
- [[20-最佳实践/04-migration/06-stateful-services-migration|06 - 有状态服务迁移 [migration]]]
- [[22-概念/01-核心架构/kubernetes-core-concepts|Kubernetes Core Concepts]]
- [[22-概念/01-核心架构/etcd-operational-reference|etcd Operational Reference]]
- [[22-概念/01-核心架构/declarative-api|Declarative API]]
- [[22-概念/01-核心架构/core-dependency-version-matrix|核心依赖版本矩阵]]
- [[22-概念/01-核心架构/watch-mechanism|Watch Mechanism (List-Watch)]]
- [[22-概念/03-网络/tcp-udp-protocol-stack|TCP/UDP Protocol Stack]]
- [[22-概念/08-可靠性与运维/kubeadm-cluster-operations|kubeadm 集群运维全景]]
- [[22-概念/08-可靠性与运维/K8s-故障分布与-MTTR-基准|K8s 问题分布与 MTTR 基准]]
- [[22-概念/08-可靠性与运维/Structural-Troubleshooting-Framework|Structural Troubleshooting Framework]]
- [[22-概念/11-交叉分析/etcd-×-PVC|etcd × PVC]]
- [[22-概念/11-交叉分析/etcd-×-灾难恢复|etcd × 灾难恢复]]
- [[22-概念/11-交叉分析/etcd-×-Prometheus|etcd × Prometheus]]
- [[22-概念/11-交叉分析/etcd-×-StatefulSet|etcd × StatefulSet]]
- [[22-概念/11-交叉分析/etcd-×-IaC|etcd × IaC]]
- [[22-概念/11-交叉分析/服务网格 × 零信任安全|服务网格 x 零信任安全]]
- [[22-概念/11-交叉分析/etcd-×-蓝绿发布|etcd × 蓝绿发布]]
- [[22-概念/11-交叉分析/etcd-×-Pod诊断|etcd × Pod诊断]]
- [[22-概念/11-交叉分析/etcd-×-GitOps|etcd × GitOps]]
- [[22-概念/11-交叉分析/etcd-×-节点诊断|etcd × 节点诊断]]
- [[22-概念/11-交叉分析/etcd × 高可用模式|etcd × 高可用模式]]
- [[22-概念/11-交叉分析/etcd-×-滚动更新|etcd × 滚动更新]]
- [[22-概念/11-交叉分析/etcd-×-Deployment|etcd × Deployment]]
- [[22-概念/11-交叉分析/etcd-×-RBAC|etcd × RBAC]]
- [[22-概念/11-交叉分析/etcd-×-Service|etcd × Service]]
- [[22-概念/11-交叉分析/etcd-×-Grafana|etcd × Grafana]]
- [[22-概念/11-交叉分析/etcd-×-NetworkPolicy|etcd × NetworkPolicy]]
- [[22-概念/11-交叉分析/etcd-×-Ingress|etcd × Ingress]]
- [[22-概念/11-交叉分析/etcd-×-备份|etcd × 备份]]
- [[22-概念/12-研究/kubernetes-version-evolution|Kubernetes 版本演进]]
- [[22-概念/12-研究/ai-agent-openclaw-workspace|OpenClaw 工作空间配置]]
- [[23-实体/02-K8s核心组件/kube-controller-manager|kube-controller-manager]]
- [[23-实体/15-参考与索引/specialized-workloads-terms|K8s 专用工作负载术语参考]]
- [[23-实体/15-参考与索引/k8s-design-principles-deep-dive|设计原理：声明式 API、控制器模式与 etcd 共识]]
- [[23-实体/15-参考与索引/workloads-terms|K8s 工作负载术语参考]]
- [[23-实体/15-参考与索引/k8s-structured-troubleshooting|结构化排障方法论：配置优先、全组件排障指南]]
- [[23-实体/15-参考与索引/fundamentals-terms|K8s 基础概念术语参考]]
- [[23-实体/15-参考与索引/k8s-architecture-fundamentals|K8s 架构基础与核心组件原理]]
- [[23-实体/15-参考与索引/k8s-advanced-ecosystem|硬件知识体系、CNCF 全景生态与 eBPF 平台工程]]
- [[23-实体/15-参考与索引/k8s-control-plane-deep-dive|控制平面深度剖析：API Server、Scheduler、KCM 与 CRI/CSI/CNI]]
- [[23-实体/15-参考与索引/k8s-deployment-create|Kubernetes Deployment 创建操作指南]]
- [[23-实体/15-参考与索引/k8s-cluster-delete|Kubernetes 集群删除操作指南]]
- [[23-实体/15-参考与索引/k8s-cluster-create|Kubernetes 集群创建操作指南]]
- [[23-实体/15-参考与索引/tooling-terms|K8s 工具链术语参考]]
- [[23-实体/15-参考与索引/k8s-cluster-cert|Kubernetes 集群证书管理操作指南]]
- [[23-实体/15-参考与索引/k8s-node-create|Kubernetes 节点管理操作指南]]
- [[23-实体/15-参考与索引/KUDIG Scenario Taxonomy|KUDIG Scenario Taxonomy]]
- [[23-实体/15-参考与索引/multi-cloud-terms|K8s 多云架构术语参考]]
- [[23-实体/15-参考与索引/version-upgrade-guide|版本升级指南]]
- [[23-实体/15-参考与索引/operations-terms|K8s 运维运营术语参考]]
- [[26-技能/01-集群运维/cluster/01-apiserver-controlplane|控制平面不可用（kube-apiserver）诊断与修复]]
- [[26-技能/01-集群运维/cluster/02-etcd-troubleshooting|etcd 集群故障诊断与恢复]]
- [[26-技能/01-集群运维/cluster-upgrade/最佳实践/k8s-cluster-configuration-guide|Kubernetes 集群配置最佳实践]]
- [[26-技能/01-集群运维/gitops-argocd/诊断排障/ts-gitops-devops|GitOps/DevOps 排查]]
- [[26-技能/01-集群运维/kubeadm/kubeadm-cluster-lifecycle|kubeadm 集群创建生命周期]]
- [[26-技能/01-集群运维/kubeadm/kubeadm-cluster-deletion|kubeadm 集群删除操作]]
- [[26-技能/01-集群运维/kubeadm/kubeadm-ha-cluster-setup|kubeadm 高可用集群搭建]]
- [[26-技能/01-集群运维/migration/06-stateful-services-migration|06 - 有状态服务迁移 [migration]]]
- [[26-技能/02-控制面/apiserver/诊断排障/ts-control-plane|控制平面故障排查]]
- [[26-技能/03-节点/node/skill-notready/skill-assets-escalation-template|Escalation Template]]
- [[26-技能/03-节点/node/运维操作/node-drain-and-maintenance|节点驱逐与维护]]
- [[26-技能/04-工作负载/pod/培训/learn-01-day-one-checklist|Day 1: 新人首日检查清单]]
- [[26-技能/04-工作负载/pod/培训/learn-02-first-ticket-guide|Day 2: 第一个工单处理指南]]
- [[26-技能/04-工作负载/pod/培训/learn-oncall-quick-qa|工单数字人快速问答 - On-Call 速查]]
- [[26-技能/04-工作负载/pod/培训/inner-training/week-1-ack-acr-lifecycle/day-7-cluster-certificate|Day 7: K8S 集群证书]]
- [[26-技能/04-工作负载/pod/培训/public-one-month/public-one-month-training|Kubernetes 生产运维实战训练营]]
- [[26-技能/04-工作负载/pod/培训/public-one-month/week-1-foundation/day-6-k8s-cluster|Day 6: K8s 架构深化 + 集群配置]]
- [[26-技能/04-工作负载/pod/方法论/Kubernetes FTA Top Events Index|Kubernetes FTA Top Events Index]]
- [[26-技能/04-工作负载/pod/方法论/FTA Methodology and Core Principles|FTA Methodology and Core Principles]]
- [[26-技能/04-工作负载/pod/概念原理/etcd-×-Pod诊断|etcd × Pod诊断]]
- [[26-技能/04-工作负载/pod/清单规范/01-pod-specification-complete|03 - Pod 完整规格说明书]]
- [[26-技能/04-工作负载/statefulset/statefulset-fta|StatefulSet 异常故障树分析 (skills)]]
- [[26-技能/06-存储/csi-storage/诊断排障/ts-storage|存储故障排查]]
- [[26-技能/07-安全/rbac/培训/day-11-risk-assessment/01-risk-assessment-hands-on|Day 11: K8s 安全风险识别与防护实操]]
- [[26-技能/07-安全/rbac/诊断排障/ts-security-auth|安全认证故障排查]]
