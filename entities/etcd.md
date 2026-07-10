---
title: etcd (entities)
description: '- etcd 深度解析'
summary: etcd is the backing datastore for Kubernetes. All cluster state ([[Pods|Pods]],
  Services, [[ConfigMaps|ConfigMaps]], [[Secrets|Secrets]], etc.) is persisted to
  etcd. It uses Raft consensus for faul...
category: entities
tags:
- k8s
- etcd
- raft
- mvcc
- database
- control-plane
- apiserver
- operator
- rag
- scheduler
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- etcd 是什么
- 如何 etcd
trigger_keywords:
- etcd
prerequisites:
- kubectl-basics
- ebpf-basics
- etcd-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# etcd

## Overview

etcd is the backing datastore for Kubernetes. All cluster state ([[Pods|Pods]], Services, [[ConfigMaps|ConfigMaps]], [[Secrets|Secrets]], etc.) is persisted to etcd. It uses Raft consensus for fault-tolerant replication and MVCC (Multi-Version Concurrency Control) for watchable history.

## Key Properties

| Property | Value |
|----------|-------|
| **Consensus** | Raft (Leader/Follower/Candidate) |
| **Storage** | B+ Tree with MVCC revision chains |
| **Watch** | Real-time event streaming by revision |
| **Ports** | 2379 (client gRPC), 2380 (peer replication) |
| **Data Path** | `/registry/{resource-type}/{namespace}/{name}` |
| **Quota** | Default 2GB (`--quota-backend-bytes=8GB` for production) |

## Raft Consensus

- Odd number of nodes (3, 5, or 7)
- Tolerates f failures with 2f+1 nodes
- Leader handles all writes; followers replicate log
- Configurable heartbeat (100ms) and election timeout (1000ms)

## MVCC and Watch

Every write increments a global revision number. Watch streams track from a specific revision and receive events for changes. etcd compaction removes old revisions periodically (default every 5 minutes) to reclaim space.

## Production Requirements

- **Storage**: SSD or NVMe for low fsync latency (<10ms p99)
- **Backup**: Hourly snapshots with `etcdctl snapshot save`
- **Defragmentation**: Regular `etcdctl defrag` to reclaim space after compaction
- **Monitoring**: Watch disk commit duration, db size, leader changes, proposal failures

## Related
- [[concepts/etcd × Operator 模式.md|etcd × Operator 模式]] — 综合
- [[concepts/etcd × 可观测性.md|etcd × 可观测性]] — 综合

- [[grpc]] — gRPC
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[concepts/kubernetes-architecture-overview.md|kubernetes-architecture-overview]] — Kubernetes Architecture Overview
- [[concepts/high-availability-patterns.md|high-availability-patterns]] — High Availability Patterns
- [[skills/backup-restore-etcd.md|backup-restore-etcd]] — Backup and Restore etcd
- [[concepts/kubernetes-architecture-overview.md|Kubernetes Architecture Overview]]
- [[concepts/watch-mechanism.md|Watch Mechanism]]
- [[concepts/high-availability-patterns.md|High Availability Patterns]]
- [[skills/backup-restore-etcd.md|Backup and Restore etcd]]
- [[entities/kube-apiserver.md|kube-apiserver]]
- [[concepts/etcd Operational Reference.md|etcd Operational Reference]]

- etcd 深度解析
- 19-etcd-operations
- 07-distributed-consensus-etcd
- [[故障诊断/核心排障/02-control-plane-etcd-troubleshooting.md|02-control-plane-etcd-troubleshooting]]
- [[故障诊断/高级排障/10-etcd-maintenance.md|10-etcd-maintenance]]
- [[故障诊断/FTA故障树/list/etcd-fta.md|etcd 异常故障树分析]]
- [[故障诊断/高级排障/01-control-plane/02-etcd-troubleshooting.md|02-etcd-troubleshooting]]
- RELEASE-NOTES-0.2
- [[_archives/release-notes/core-deps/etcd/RELEASE-NOTES-3.5.md|RELEASE-NOTES-3.5]]
- [[_archives/release-notes/core-deps/etcd/RELEASE-NOTES-2.0.md|RELEASE-NOTES-2.0]]
- [[_archives/release-notes/core-deps/etcd/RELEASE-NOTES-3.1.md|RELEASE-NOTES-3.1]]
- [[_archives/release-notes/core-deps/etcd/RELEASE-NOTES-2.1.md|RELEASE-NOTES-2.1]]
- [[_archives/release-notes/core-deps/etcd/RELEASE-NOTES-3.0.md|RELEASE-NOTES-3.0]]
- RELEASE-NOTES-0.3
- [[_archives/release-notes/core-deps/etcd/RELEASE-NOTES-3.4.md|RELEASE-NOTES-3.4]]
- [[_archives/release-notes/core-deps/etcd/RELEASE-NOTES-2.2.md|RELEASE-NOTES-2.2]]
- [[_archives/release-notes/core-deps/etcd/RELEASE-NOTES-3.3.md|RELEASE-NOTES-3.3]]
- RELEASE-NOTES-0.4
- RELEASE-NOTES-0.1
- [[_archives/release-notes/core-deps/etcd/RELEASE-NOTES-3.6.md|RELEASE-NOTES-3.6]]
- [[_archives/release-notes/core-deps/etcd/RELEASE-NOTES-2.3.md|RELEASE-NOTES-2.3]]
- [[_archives/release-notes/core-deps/etcd/RELEASE-NOTES-3.2.md|RELEASE-NOTES-3.2]]
- Wiki Digest — Daily (2026-05-21) — Cross-reference
- [[entities/KUDIG Cheat Sheet Index.md|KUDIG Cheat Sheet Index]] — Cross-reference
- [[entities/specialized-workloads-terms.md|K8s 专用工作负载术语参考]] — Cross-reference
- [[entities/k8s-design-principles-deep-dive.md|设计原理：声明式 API、控制器模式与 etcd 共识]] — Cross-reference
- [[entities/workloads-terms.md|K8s 工作负载术语参考]] — Cross-reference
- [[entities/k8s-structured-troubleshooting.md|结构化排障方法论：配置优先、全组件排障指南]] — Cross-reference
- [[entities/fundamentals-terms.md|K8s 基础概念术语参考]] — Cross-reference
- [[entities/k8s-architecture-fundamentals.md|K8s 架构基础与核心组件原理]] — Cross-reference
- [[生态参考/98-merged-indexes/index.md|发布说明阅读指南]] — Cross-reference
- [[entities/k8s-advanced-ecosystem.md|硬件知识体系、CNCF 全景生态与 eBPF 平台工程]] — Cross-reference
- [[entities/k8s-control-plane-deep-dive.md|控制平面深度剖析：API Server、Scheduler、KCM 与 CRI/CSI/CNI]] — Cross-reference
- [[entities/kubectl-quick-reference.md|Kubectl Quick Reference]] — Cross-reference
- [[entities/k8s-deployment-create.md|Kubernetes Deployment 创建操作指南]] — Cross-reference
- [[entities/k8s-production-operations.md|生产运维：GitOps、FinOps、灾备恢复与变更管理]] — Cross-reference
- [[entities/k8s-cluster-delete.md|Kubernetes 集群删除操作指南]] — Cross-reference
- [[entities/k8s-cluster-create.md|Kubernetes 集群创建操作指南]] — Cross-reference
- [[entities/release-notes-core-deps.md|发布说明索引 — 核心依赖]] — Cross-reference
- [[entities/tooling-terms.md|K8s 工具链术语参考]] — Cross-reference
- [[entities/k8s-cluster-cert.md|Kubernetes 集群证书管理操作指南]] — Cross-reference
- [[entities/k8s-node-create.md|Kubernetes 节点管理操作指南]] — Cross-reference
- [[entities/KUDIG Scenario Taxonomy.md|KUDIG Scenario Taxonomy]] — Cross-reference
- [[entities/multi-cloud-terms.md|K8s 多云架构术语参考]] — Cross-reference
- [[entities/kudig-man-pages-index.md|KUDIG Man Pages Index]] — Cross-reference
- [[entities/version-upgrade-guide.md|版本升级指南]] — Cross-reference
- [[entities/operations-terms.md|K8s 运维运营术语参考]] — Cross-reference
- [[concepts/kubeadm-cluster-operations.md|kubeadm 集群运维全景]] — Cross-reference
- [[concepts/etcd x 高可用模式.md|etcd × 高可用模式]] — Cross-reference
- [[concepts/Production Troubleshooting Playbook.md|Production Troubleshooting Playbook]] — Cross-reference
- [[concepts/k8s-mttr-benchmark.md|K8s 问题分布与 MTTR 基准]] — Cross-reference
- [[concepts/Structural Troubleshooting Framework.md|Structural Troubleshooting Framework]] — Cross-reference
- [[concepts/声明式 API × 控制器模式.md|声明式 API × 控制器模式]] — Cross-reference
- [[concepts/deployment-controller-architecture.md|Deployment 控制器架构]] — Cross-reference
- [[concepts/kubernetes-pki-certificate-system.md|Kubernetes PKI 证书体系]] — Cross-reference
- [[concepts/bp-infrastructure.md|最佳实践：Infrastructure]] — Cross-reference
- [[concepts/declarative-api.md|Declarative API]] — Cross-reference
- [[concepts/core-dependency-version-matrix.md|核心依赖版本矩阵]] — Cross-reference
- [[concepts/kubernetes-version-evolution.md|Kubernetes 版本演进]] — Cross-reference
- [[concepts/ai-agent-openclaw-workspace.md|OpenClaw 工作空间配置]] — Cross-reference
- [[concepts/infrastructure-as-code.md|Infrastructure as Code]] — Cross-reference
- [[concepts/KUDIG Knowledge Base Architecture.md|KUDIG Knowledge Base Architecture]] — Cross-reference
- [[concepts/eventual-consistency.md|Eventual Consistency in Kubernetes]] — Cross-reference
- [[concepts/k8s-production-best-practices.md|Kubernetes 生产环境最佳实践]] — Cross-reference
- [[concepts/Kubernetes Core Concepts.md|Kubernetes Core Concepts]] — Cross-reference
- [[concepts/tcp-udp-protocol-stack.md|TCP/UDP Protocol Stack]] — Cross-reference
- [[skills/learn-01-day-one-checklist.md|Day 1: 新人首日检查清单]] — Cross-reference
- [[skills/k8s-disaster-recovery-guide.md|Kubernetes 灾难恢复最佳实践]] — Cross-reference
- [[skills/ts-node-components.md|节点组件故障排查]] — Cross-reference
- [[skills/kubeadm-cluster-lifecycle.md|kubeadm 集群创建生命周期]] — Cross-reference
- [[skills/ts-security-auth.md|安全认证故障排查]] — Cross-reference
- [[skills/develop-crd-operator.md|Develop CRD Operator]] — Cross-reference
- [[skills/node-drain-and-maintenance.md|节点驱逐与维护]] — Cross-reference
- [[skills/statefulset-fta.md|StatefulSet 异常故障树分析]] — Cross-reference
- [[skills/kubeadm-cluster-deletion.md|kubeadm 集群删除操作]] — Cross-reference
- [[skills/kubeadm-ha-cluster-setup.md|kubeadm 高可用集群搭建]] — Cross-reference
- [[skills/k8s-cluster-configuration-guide.md|Kubernetes 集群配置最佳实践]] — Cross-reference
- [[skills/ts-control-plane.md|控制平面故障排查]] — Cross-reference
- [[skills/monitor-kubernetes-metrics.md|Monitor Kubernetes Metrics]] — Cross-reference
- [[skills/ts-gitops-devops.md|GitOps/DevOps 排查]] — Cross-reference
- [[skills/learn-02-first-ticket-guide.md|Day 2: 第一个工单处理指南]] — Cross-reference
- [[skills/learn-oncall-quick-qa.md|工单数字人快速问答 - On-Call 速查]] — Cross-reference
- [[skills/skill-MOC.md|topic-skills MOC]] — Cross-reference
- [[skills/FTA-Driven Runbook Automation.md|FTA-Driven Runbook Automation]] — Cross-reference
- [[skills/ts-storage.md|存储故障排查]] — Cross-reference
- [[skills/ts-cluster-operations.md|集群运维故障排查]] — Cross-reference
- [[skills/skill-assets-escalation-template.md|Escalation Template]] — Cross-reference
- [[集群基础/控制平面/11-etcd-deep-dive.md|etcd 深度解析]] — Cross-reference
- [[集群基础/控制平面/12-apiserver-deep-dive.md|kube-apiserver 深度解析]] — Cross-reference
- Domain-3: Kubernetes控制平面 — Cross-reference
- [[entities/core-deps-changelog.md|核心依赖变更日志索引]] — Cross-reference
- [[entities/cncf-edge-ai.md|CNCF 边缘计算与 AI/ML 项目全景]] — Cross-reference
- [[生态参考/领域索引/backup-dr-index.md|Backup & DR 备份与灾备知识图谱索引]]
- [[生态参考/领域索引/etcd-index.md|etcd 知识图谱索引]]


<!-- risk-assessed -->
