---
title: etcd
description: '- [[domain-01-cluster-fundamentals/11-etcd-deep-dive.md|etcd 深度解析]]'
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

# etcd

## Overview

etcd is the backing datastore for Kubernetes. All cluster state (Pods, Services, ConfigMaps, Secrets, etc.) is persisted to etcd. It uses Raft consensus for fault-tolerant replication and MVCC (Multi-Version Concurrency Control) for watchable history.

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
- [[synthesis/etcd × Operator 模式.md|etcd × Operator 模式]] — 综合
- [[synthesis/etcd × 可观测性|etcd × 可观测性]] — 综合

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

- [[domain-01-cluster-fundamentals/11-etcd-deep-dive.md|etcd 深度解析]]
- [[domain-01-cluster-fundamentals/19-etcd-operations.md|19-etcd-operations]]
- [[domain-01-cluster-fundamentals/07-distributed-consensus-etcd.md|07-distributed-consensus-etcd]]
- [[domain-10-troubleshooting-diagnostics/02-control-plane-etcd-troubleshooting.md|02-control-plane-etcd-troubleshooting]]
- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/10-etcd-maintenance.md|10-etcd-maintenance]]
- [[domain-10-troubleshooting-diagnostics/topic-fta/list/etcd-fta.md|etcd 异常故障树分析]]
- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/01-control-plane/02-etcd-troubleshooting.md|02-etcd-troubleshooting]]
- [[domain-19-landscape-references/topic-release-notes/core-deps/etcd/RELEASE-NOTES-0.2.md|RELEASE-NOTES-0.2]]
- [[domain-19-landscape-references/topic-release-notes/core-deps/etcd/RELEASE-NOTES-3.5.md|RELEASE-NOTES-3.5]]
- [[domain-19-landscape-references/topic-release-notes/core-deps/etcd/RELEASE-NOTES-2.0.md|RELEASE-NOTES-2.0]]
- [[domain-19-landscape-references/topic-release-notes/core-deps/etcd/RELEASE-NOTES-3.1.md|RELEASE-NOTES-3.1]]
- [[domain-19-landscape-references/topic-release-notes/core-deps/etcd/RELEASE-NOTES-2.1.md|RELEASE-NOTES-2.1]]
- [[domain-19-landscape-references/topic-release-notes/core-deps/etcd/RELEASE-NOTES-3.0.md|RELEASE-NOTES-3.0]]
- [[domain-19-landscape-references/topic-release-notes/core-deps/etcd/RELEASE-NOTES-0.3.md|RELEASE-NOTES-0.3]]
- [[domain-19-landscape-references/topic-release-notes/core-deps/etcd/RELEASE-NOTES-3.4.md|RELEASE-NOTES-3.4]]
- [[domain-19-landscape-references/topic-release-notes/core-deps/etcd/RELEASE-NOTES-2.2.md|RELEASE-NOTES-2.2]]
- [[domain-19-landscape-references/topic-release-notes/core-deps/etcd/RELEASE-NOTES-3.3.md|RELEASE-NOTES-3.3]]
- [[domain-19-landscape-references/topic-release-notes/core-deps/etcd/RELEASE-NOTES-0.4.md|RELEASE-NOTES-0.4]]
- [[domain-19-landscape-references/topic-release-notes/core-deps/etcd/RELEASE-NOTES-0.1.md|RELEASE-NOTES-0.1]]
- [[domain-19-landscape-references/topic-release-notes/core-deps/etcd/RELEASE-NOTES-3.6.md|RELEASE-NOTES-3.6]]
- [[domain-19-landscape-references/topic-release-notes/core-deps/etcd/RELEASE-NOTES-2.3.md|RELEASE-NOTES-2.3]]
- [[domain-19-landscape-references/topic-release-notes/core-deps/etcd/RELEASE-NOTES-3.2.md|RELEASE-NOTES-3.2]]
- [[journal/digest-2026-05-21|Wiki Digest — Daily (2026-05-21)]] — Cross-reference
- [[references/KUDIG Cheat Sheet Index|KUDIG Cheat Sheet Index]] — Cross-reference
- [[references/specialized-workloads-terms|K8s 专用工作负载术语参考]] — Cross-reference
- [[references/k8s-design-principles-deep-dive|设计原理：声明式 API、控制器模式与 etcd 共识]] — Cross-reference
- [[references/workloads-terms|K8s 工作负载术语参考]] — Cross-reference
- [[references/k8s-structured-troubleshooting|结构化排障方法论：配置优先、全组件排障指南]] — Cross-reference
- [[references/fundamentals-terms|K8s 基础概念术语参考]] — Cross-reference
- [[references/k8s-architecture-fundamentals|K8s 架构基础与核心组件原理]] — Cross-reference
- [[references/release-notes-reading-guide|发布说明阅读指南]] — Cross-reference
- [[references/k8s-advanced-ecosystem|硬件知识体系、CNCF 全景生态与 eBPF 平台工程]] — Cross-reference
- [[references/k8s-control-plane-deep-dive|控制平面深度剖析：API Server、Scheduler、KCM 与 CRI/CSI/CNI]] — Cross-reference
- [[references/kubectl-quick-reference|Kubectl Quick Reference]] — Cross-reference
- [[references/k8s-deployment-create|Kubernetes Deployment 创建操作指南]] — Cross-reference
- [[references/k8s-production-operations|生产运维：GitOps、FinOps、灾备恢复与变更管理]] — Cross-reference
- [[references/k8s-cluster-delete|Kubernetes 集群删除操作指南]] — Cross-reference
- [[references/k8s-cluster-create|Kubernetes 集群创建操作指南]] — Cross-reference
- [[references/release-notes-core-deps|发布说明索引 — 核心依赖]] — Cross-reference
- [[references/tooling-terms|K8s 工具链术语参考]] — Cross-reference
- [[references/k8s-cluster-cert|Kubernetes 集群证书管理操作指南]] — Cross-reference
- [[references/k8s-node-create|Kubernetes 节点管理操作指南]] — Cross-reference
- [[references/KUDIG Scenario Taxonomy|KUDIG Scenario Taxonomy]] — Cross-reference
- [[references/multi-cloud-terms|K8s 多云架构术语参考]] — Cross-reference
- [[references/kudig-man-pages-index|KUDIG Man Pages Index]] — Cross-reference
- [[references/version-upgrade-guide|版本升级指南]] — Cross-reference
- [[references/operations-terms|K8s 运维运营术语参考]] — Cross-reference
- [[synthesis/kubeadm-cluster-operations|kubeadm 集群运维全景]] — Cross-reference
- [[synthesis/etcd x 高可用模式|etcd × 高可用模式]] — Cross-reference
- [[synthesis/Production Troubleshooting Playbook|Production Troubleshooting Playbook]] — Cross-reference
- [[synthesis/K8s 故障分布与 MTTR 基准|K8s 故障分布与 MTTR 基准]] — Cross-reference
- [[synthesis/Structural Troubleshooting Framework|Structural Troubleshooting Framework]] — Cross-reference
- [[synthesis/声明式 API × 控制器模式|声明式 API × 控制器模式]] — Cross-reference
- [[concepts/deployment-controller-architecture|Deployment 控制器架构]] — Cross-reference
- [[concepts/kubernetes-pki-certificate-system|Kubernetes PKI 证书体系]] — Cross-reference
- [[concepts/bp-infrastructure|最佳实践：Infrastructure]] — Cross-reference
- [[concepts/declarative-api|Declarative API]] — Cross-reference
- [[concepts/core-dependency-version-matrix|核心依赖版本矩阵]] — Cross-reference
- [[concepts/kubernetes-version-evolution|Kubernetes 版本演进]] — Cross-reference
- [[concepts/ai-agent-openclaw-workspace|OpenClaw 工作空间配置]] — Cross-reference
- [[concepts/infrastructure-as-code|Infrastructure as Code]] — Cross-reference
- [[concepts/KUDIG Knowledge Base Architecture|KUDIG Knowledge Base Architecture]] — Cross-reference
- [[concepts/eventual-consistency|Eventual Consistency in Kubernetes]] — Cross-reference
- [[concepts/k8s-production-best-practices|Kubernetes 生产环境最佳实践]] — Cross-reference
- [[concepts/Kubernetes Core Concepts|Kubernetes Core Concepts]] — Cross-reference
- [[concepts/tcp-udp-protocol-stack|TCP/UDP Protocol Stack]] — Cross-reference
- [[skills/learn-01-day-one-checklist|Day 1: 新人首日检查清单]] — Cross-reference
- [[skills/k8s-disaster-recovery-guide|Kubernetes 灾难恢复最佳实践]] — Cross-reference
- [[skills/ts-node-components|节点组件故障排查]] — Cross-reference
- [[skills/kubeadm-cluster-lifecycle|kubeadm 集群创建生命周期]] — Cross-reference
- [[skills/ts-security-auth|安全认证故障排查]] — Cross-reference
- [[skills/develop-crd-operator|Develop CRD Operator]] — Cross-reference
- [[skills/node-drain-and-maintenance|节点驱逐与维护]] — Cross-reference
- [[skills/statefulset-fta|StatefulSet 异常故障树分析]] — Cross-reference
- [[skills/kubeadm-cluster-deletion|kubeadm 集群删除操作]] — Cross-reference
- [[skills/kubeadm-ha-cluster-setup|kubeadm 高可用集群搭建]] — Cross-reference
- [[skills/k8s-cluster-configuration-guide|Kubernetes 集群配置最佳实践]] — Cross-reference
- [[skills/ts-control-plane|控制平面故障排查]] — Cross-reference
- [[skills/monitor-kubernetes-metrics|Monitor Kubernetes Metrics]] — Cross-reference
- [[skills/ts-gitops-devops|GitOps/DevOps 排查]] — Cross-reference
- [[skills/learn-02-first-ticket-guide|Day 2: 第一个工单处理指南]] — Cross-reference
- [[skills/learn-oncall-quick-qa|工单数字人快速问答 - On-Call 速查]] — Cross-reference
- [[skills/skill-MOC|topic-skills MOC]] — Cross-reference
- [[skills/FTA-Driven Runbook Automation|FTA-Driven Runbook Automation]] — Cross-reference
- [[skills/ts-storage|存储故障排查]] — Cross-reference
- [[skills/ts-cluster-operations|集群运维故障排查]] — Cross-reference
- [[skills/skill-assets-escalation-template|Escalation Template]] — Cross-reference
- [[domain-01-cluster-fundamentals/03-control-plane/11-etcd-deep-dive|etcd 深度解析]] — Cross-reference
- [[domain-01-cluster-fundamentals/03-control-plane/12-apiserver-deep-dive|kube-apiserver 深度解析]] — Cross-reference
- [[domain-01-cluster-fundamentals/98-merged-indexes/README-from-domain-01-cluster-fundamentals|Domain-3: Kubernetes控制平面]] — Cross-reference
- [[entities/core-deps-changelog|核心依赖变更日志索引]] — Cross-reference
- [[entities/cncf-edge-ai|CNCF 边缘计算与 AI/ML 项目全景]] — Cross-reference
- [[domain-19-landscape-references/topic-index/backup-dr-index|Backup & DR 备份与灾备知识图谱索引]]
- [[domain-19-landscape-references/topic-index/etcd-index|etcd 知识图谱索引]]
