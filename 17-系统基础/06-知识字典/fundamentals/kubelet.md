---
title: kubelet
description: kubelet 是运行在每个 Kubernetes 节点上的代理程序。它确保容器按照 PodSpec 中描述的规格运行，是节点上最重要的组件。...
summary: kubelet 是运行在每个 Kubernetes 节点上的代理程序。它确保容器按照 PodSpec 中描述的规格运行，是节点上最重要的组件。...
category: dictionary
tags:
- k8s
- glossary
- kubelet
- node
tier: core
created: '2026-06-24'
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- kubelet 是什么
- kubelet 详解
trigger_keywords:
- kubelet
- dictionary
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# kubelet

> **英文名**: kubelet

## 概述

kubelet 是运行在每个 Kubernetes 节点上的代理程序。它确保容器按照 PodSpec 中描述的规格运行，是节点上最重要的组件。

## 核心概念/原理

### 核心职责

- **Pod 管理**：根据 API Server 下发的 PodSpec 创建、更新和删除容器。
- **健康检查**：执行 Liveness、Readiness 和 Startup 探针。
- **资源监控**：上报节点资源使用情况和 Pod 指标。
- **日志收集**：管理容器日志文件。
- **Volume 管理**：挂载和卸载 Volume。
- **镜像管理**：通过 CRI 拉取容器镜像。

### 通信模式

kubelet 通过 API Server 获取 Pod 配置，同时向 API Server 报告节点状态和 Pod 状态。kubelet 还暴露 `/healthz`、`/metrics` 等端点供监控使用。

## 关键机制或特性

- kubelet 通过 CRI（Container Runtime Interface）与容器运行时通信。
- 支持 Static Pod（通过 manifest 目录或 URL 直接创建，不经过 API Server）。
- kubelet 的 `--config` 参数通过 KubeletConfiguration 进行配置。
- 支持 cgroup v1 和 cgroup v2。

## 使用场景与最佳实践

- 合理配置 `--max-pods` 限制单节点 Pod 数量。
- 设置 `--image-gc-high-threshold` 和 `--image-gc-low-threshold` 管理镜像垃圾回收。
- 配置 `--eviction-hard` 和 `--eviction-soft` 防止节点资源耗尽。
- 定期升级 kubelet 版本，保持与 API Server 的兼容性。

## 参考链接

- [kubelet - Official Documentation](https://kubernetes.io/docs/reference/command-line-tools-reference/kubelet/)

## Related

[[17-系统基础/06-知识字典/fundamentals/kubernetes-components.md|Kubernetes 组件]]


<!-- risk-assessed -->
- [[01-集群基础/01-架构总览/14-troubleshooting-guide|16 - Kubernetes 故障排查专家级指南]]
- [[01-集群基础/01-架构总览/16-kubernetes-core-components-v1.29-v1.33-update|Kubernetes 核心组件 v1.29 - v1.33 新特性速查]]
- [[01-集群基础/02-设计原则/01-design-principles-foundations|01 - Kubernetes 设计原则与哲学 (Foundations)]]
- [[01-集群基础/03-控制平面/02-plane-components-interaction|控制平面组件交互详解 (Control Plane Components Interaction Deep Dive)]]
- [[02-工作负载/01-核心工作负载/18-node-management-operations|27 - 节点与节点池管理 (Node & NodePool Management)]]
- [[03-清单模式/01-YAML参考/32-lease-event-node|32 - Lease / Event / Node YAML 配置参考]]
- [[03-清单模式/01-YAML参考/34-component-configuration|34. Kubernetes 组件配置（Component Configuration）]]
- [[05-网络/01-K8s网络核心/13-dns-service-discovery|33 - 服务发现与 DNS 配置 (Service Discovery & DNS)]]
- [[05-网络/01-K8s网络核心/40-terway-gc-mechanism|38 - Terway GC (垃圾回收) 机制详解 (Terway Garbage Collection Mechanism)]]
- [[10-平台工程/02-运维/19-kubernetes-v1.33-platform-ops-guide|Kubernetes v1.29-v1.33 平台运维新特性指南]]
- [[10-平台工程/02-运维/09-backup-recovery-strategy|Kubernetes 备份与恢复概述 (Backup & Recovery Overview)]]
- [[10-平台工程/03-治理/02-performance-benchmarking-tuning|性能基准测试与调优 (Performance Benchmarking & Tuning)]]
- [[15-AI基础设施/02-AI-Agents/15-agent-corpus-gap-analysis|Agent 语料库差距分析：kudig-database 作为 K8s 运维 Agent 语料还缺什么？ [02-ai-agents]]]
- [[15-AI基础设施/02-AI-Agents/49-openclaw-memory-mechanism|OpenClaw MEMORY.md 机制深度解析 (AI基础设施)]]
- [[15-AI基础设施/02-AI-Agents/30-agent-harness-engineering|Agent Harness 工程：从模型包装到生产级 Agent 系统设计 (AI基础设施)]]
- [[15-AI基础设施/02-AI-Agents/11-cost-latency-optimization|成本与延迟优化策略 (AI基础设施)]]
- [[15-AI基础设施/02-AI-Agents/45-openclaw-user-mechanism|OpenClaw USER.md 机制深度解析 (AI基础设施)]]
- [[17-系统基础/02-硬件/18-hardware-failure-case-studies|硬件问题实战案例库]]
- [[17-系统基础/04-K8s事件/11-storage-volume-events|11 - 存储与卷事件]]
- [[17-系统基础/04-K8s事件/03-image-pull-events|03 - 镜像拉取事件]]
- [[17-系统基础/04-K8s事件/02-pod-container-lifecycle-events|02 - Pod 与容器生命周期事件]]
- [[17-系统基础/04-K8s事件/13-security-admission-rbac-events|13 - 安全、准入控制与 RBAC 事件]]
- [[17-系统基础/04-K8s事件/05-scheduling-preemption-events|05 - 调度与抢占事件]]
- [[17-系统基础/04-K8s事件/01-event-system-architecture|01 - Kubernetes 事件系统架构与 API 参考]]
- [[17-系统基础/04-K8s事件/10-service-networking-events|10 - Service 与网络事件]]
- [[17-系统基础/06-知识字典/configuration/resource-management-for-pods-and-containers|Resource Management for Pods and Containers]]
- [[17-系统基础/06-知识字典/configuration/liveness-readiness-and-startup-probes|Liveness, Readiness, and Startup Probes]]
- [[17-系统基础/06-知识字典/configuration/resource-management-for-windows-nodes|Resource Management for Windows nodes]]
- [[17-系统基础/06-知识字典/fundamentals/namespaces|命名空间]]
- [[17-系统基础/06-知识字典/fundamentals/about-cgroup-v2|About cgroup v2（关于 cgroup v2）]]
- [[17-系统基础/06-知识字典/fundamentals/nodes|Nodes（节点）]]
- [[17-系统基础/06-知识字典/fundamentals/communication-between-nodes-and-the-control-plane|Communication between Nodes and the Control Plane（节点与控制平面之间的通信）]]
- [[17-系统基础/06-知识字典/fundamentals/garbage-collection|Garbage Collection（垃圾回收）]]
- [[17-系统基础/06-知识字典/fundamentals/kubernetes-self-healing|Kubernetes Self-Healing（Kubernetes 自愈能力）]]
- [[17-系统基础/06-知识字典/networking/dns-for-services-and-pods|DNS for Services and Pods]]
- [[17-系统基础/06-知识字典/networking/telco-cloud-and-5g-mec|电信云与 5G 多接入边缘计算（MEC）]]
- [[17-系统基础/06-知识字典/networking/ipv4-ipv6-dual-stack|IPv4/IPv6 dual-stack]]
- [[17-系统基础/06-知识字典/observability/metrics-for-kubernetes-system-components|Kubernetes 系统组件指标]]
- [[17-系统基础/06-知识字典/operations/node-shutdowns|节点关闭（Node Shutdowns）]]
- [[17-系统基础/06-知识字典/operations/swap-memory-management|Swap 内存管理]]
- [[17-系统基础/06-知识字典/operations/failure-patterns-analysis|02 - Kubernetes 故障模式与根因分析字典]]
- [[17-系统基础/06-知识字典/operations/node-autoscaling|节点自动扩缩容（Node Autoscaling）]]
- [[17-系统基础/06-知识字典/operations/certificates|Certificates（PKI 证书与要求）]]
- [[17-系统基础/06-知识字典/platform-engineering/api-priority-and-fairness|API 优先级与公平性（API Priority and Fairness）]]
- [[17-系统基础/06-知识字典/platform-engineering/dynamic-resource-allocation-good-practices|动态资源分配（DRA）集群管理员最佳实践]]
- [[17-系统基础/06-知识字典/scheduling/node-declared-features|Node Declared Features]]
- [[17-系统基础/06-知识字典/scheduling/assigning-pods-to-nodes|Assigning Pods to Nodes]]
- [[17-系统基础/06-知识字典/scheduling/pod-priority-and-preemption|Pod Priority and Preemption]]
- [[17-系统基础/06-知识字典/scheduling/api-initiated-eviction|API-initiated Eviction]]
- [[17-系统基础/06-知识字典/security/service-accounts|服务账号]]
- [[17-系统基础/06-知识字典/security/hardening-guide---authentication-mechanisms|加固指南 - 认证机制]]
- [[17-系统基础/06-知识字典/security/process-id-limits-and-reservations|Process ID Limits And Reservations（进程 ID 限制与预留）]]
- [[17-系统基础/06-知识字典/security/kubernetes-api-server-bypass-risks|Kubernetes API Server 绕过风险]]
- [[17-系统基础/06-知识字典/security/node-resource-managers|Node Resource Managers（节点资源管理器）]]
- [[17-系统基础/06-知识字典/storage/ephemeral-volumes|Ephemeral Volumes（临时卷）]]
- [[17-系统基础/06-知识字典/storage/projected-volumes|Projected Volumes（投射卷）]]
- [[17-系统基础/06-知识字典/storage/local-ephemeral-storage|Local ephemeral storage（本地临时存储）]]
- [[17-系统基础/06-知识字典/storage/volumes|Volumes（卷）]]
- [[17-系统基础/06-知识字典/storage/node-specific-volume-limits|Node-specific Volume Limits（节点特定卷限制）]]
- [[17-系统基础/06-知识字典/storage/volume-health-monitoring|Volume Health Monitoring（卷健康监控）]]
- [[17-系统基础/06-知识字典/tooling/cli-commands|查看所有 Pod 及其详细信息]]
- [[17-系统基础/06-知识字典/workloads/images|容器镜像（Images）]]
- [[17-系统基础/06-知识字典/workloads/pod-quality-of-service-classes|Pod Quality of Service Classes]]
- [[17-系统基础/06-知识字典/workloads/pod-hostname|Pod Hostname]]
- [[17-系统基础/06-知识字典/workloads/container-lifecycle-hooks|容器生命周期钩子（Container Lifecycle Hooks）]]
- [[19-故障诊断/04-高级排障/structural-symptom-mapping-layer|症状快速映射层 (Symptom-SOP-RootCause Mapping) [topic-structural-trouble-shooting]]]
- [[19-故障诊断/06-FTA故障树/22-industry-standardization|第二十二章：行业标准化建议 (故障诊断)]]
- [[19-故障诊断/08-技能体系/skill-set/k8s-node-notready/reference/diagnostic-workflow|诊断工作流 / Diagnostic Workflow]]
- [[19-故障诊断/08-技能体系/skill-set/k8s-node-notready/reference/root-cause-catalog|根因分类 / Root Cause Catalog]]
- [[22-概念/12-研究/kubernetes-version-evolution|Kubernetes 版本演进]]
- [[22-概念/15-运行时与系统/linux-sysctl-tuning|Linux Sysctl Tuning for Kubernetes]]
- [[26-技能/01-集群运维/cluster-upgrade/reference/skill-reference-version-matrix|Version Matrix]]
- [[26-技能/01-集群运维/cluster-upgrade/最佳实践/k8s-cluster-configuration-guide|Kubernetes 集群配置最佳实践]]
- [[26-技能/01-集群运维/kubeadm/kubeadm-cluster-lifecycle|kubeadm 集群创建生命周期]]
- [[26-技能/01-集群运维/kubeadm/kubeadm-cluster-deletion|kubeadm 集群删除操作]]
- [[26-技能/03-节点/node/运维操作/kubelet-eviction-mechanism|kubelet 资源驱逐机制]]
- [[26-技能/03-节点/nodepool/nodepool-fta|NodePool 异常故障树分析 (skills)]]
- [[26-技能/04-工作负载/pod/培训/learn-01-day-one-checklist|Day 1: 新人首日检查清单]]
- [[26-技能/04-工作负载/pod/培训/learn-README|新人上手快速路径（Quick Start）]]
- [[26-技能/04-工作负载/pod/培训/inner-training/week-1-ack-acr-lifecycle/day-7-cluster-certificate|Day 7: K8S 集群证书]]
- [[26-技能/04-工作负载/pod/培训/public-one-month/week-1-foundation/day-6-k8s-cluster|Day 6: K8s 架构深化 + 集群配置]]
- [[26-技能/04-工作负载/pod/培训/测验/assessment-k8s-fundamentals-quiz|K8S Fundamentals Quiz]]
- [[26-技能/04-工作负载/pod/安全/structural-03-pod-security-troubleshooting|Pod 安全与 SecurityContext 故障排查指南 [topic-structural-trouble-shooting]]]
- [[26-技能/04-工作负载/pod/方法论/Kubernetes Diagnostic Skills Overview|Kubernetes Diagnostic Skills Overview]]
- [[26-技能/04-工作负载/pod/方法论/skill-reference-diagnostic-workflow|Diagnostic Workflow]]
- [[26-技能/04-工作负载/pod/方法论/skill-reference-root-cause-catalog|Root Cause Catalog]]
- [[26-技能/04-工作负载/pod/概念原理/字典-pod-lifecycle|Pod Lifecycle (concepts)]]
- [[26-技能/04-工作负载/pod/清单规范/01-pod-specification-complete|03 - Pod 完整规格说明书]]
- [[26-技能/04-工作负载/pod/清单规范/04-poddisruptionbudget-reference|28 - PodDisruptionBudget YAML 配置参考]]
- [[26-技能/04-工作负载/pod/生命周期与事件/01-pod-container-lifecycle-events|02 - Pod 与容器生命周期事件]]
- [[26-技能/04-工作负载/pod/诊断排障/技能体系-02-pod-crashloop-oomkilled|Pod CrashLoopBackOff & OOMKilled 诊断与修复]]
- [[26-技能/04-工作负载/pod/诊断排障/structural-01-pod-troubleshooting|Pod 故障排查与运行机制深度指南 [topic-structural-trouble-shooting]]]
- [[26-技能/04-工作负载/pod/调度/assigning-pods-to-nodes|Assigning Pods to Nodes]]
- [[26-技能/04-工作负载/pod/调度/pod-priority-and-preemption|Pod Priority and Preemption]]
- [[26-技能/04-工作负载/pod/调度/字典-pod-overhead|Pod Overhead]]
- [[26-技能/04-工作负载/pod/资源与自动扩缩/resource-management-for-pods-and-containers|Resource Management for Pods and Containers]]
- [[26-技能/04-工作负载/pod/资源与自动扩缩/pod-quality-of-service-classes|Pod Quality of Service Classes]]
- [[26-技能/04-工作负载/pod/配置与字典/dns-for-services-and-pods|DNS for Services and Pods]]
- [[26-技能/04-工作负载/pod/配置与字典/pod-hostname|Pod Hostname]]
- [[26-技能/04-工作负载/statefulset/statefulset-fta|StatefulSet 异常故障树分析 (skills)]]
- [[26-技能/06-存储/csi-storage/诊断排障/ts-storage|存储故障排查]]
- [[26-技能/07-安全/rbac/培训/day-11-risk-assessment/01-risk-assessment-hands-on|Day 11: K8s 安全风险识别与防护实操]]
