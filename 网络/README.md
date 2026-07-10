---
title: Networking & Traffic
description: 整合原 网络/15/26/35/40 的网络知识，涵盖 K8s 网络、CNI、Service
  Mesh、API Gateway 和 eBPF。
summary: 整合原 网络/15/26/35/40 的网络知识，涵盖 K8s 网络、CNI、Service
  Mesh、API Gateway 和 eBPF。
category: domain
tags:
- networking
- cni
- service-mesh
- istio
- cilium
- api-gateway
- ebpf
- envoy
- ingress
- gateway
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Networking & Traffic 是什么
- 如何 Networking & Traffic
- Kubernetes 03 networking traffic 最佳实践
trigger_keywords:
- Networking
- Traffic
- networking
- traffic
prerequisites:
- kubectl-basics
- networking-basics
- service-mesh-basics
- ebpf-basics
- cilium-basics
- gpu-scheduling-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Networking & Traffic

整合原 网络/15/26/35/40 的网络知识，涵盖 K8s 网络、CNI、[[Service|Service]]Service Mesh）|Service Mesh]]、API Gateway 和 eBPF。

## 目录结构

| 子目录 | 内容 |
|---|---|
| 00-core-k8s-networking/ | K8s 网络架构、Service、Ingress、NetworkPolicy |
| 01-fundamentals/ | TCP/IP、DNS、负载均衡、SDN |
| 02-service-mesh/ | Istio、Linkerd、Envoy |
| 03-api-gateway/ | Gateway API、Higress |
| 04-ebpf/ | eBPF 基础、Cilium |
| topic-terway/ | 阿里云 Terway CNI 深度指南 |
| 99-attachments/ | 网络架构附件 (.xmind, .pptx) |

## 与其他 Domain 的关系

- [[集群基础/README.md|集群基础]] — 集群架构
- [[故障诊断/README.md|故障诊断]] — 网络排障

## Related

- Domain-34: CNCF Landscape 开源项目 — Cross-reference
- [[entities/release-notes-networking.md|发布说明索引 — 网络]] — Cross-reference
- 网络 MOC — Cross-reference
- Topic 应用层架构设计最佳实践 — Cross-reference
- topic-application-architecture MOC — Cross-reference
- [[concepts/bp-common-best-practices.md|Kubernetes 通用最佳实践参考]] — Cross-reference
- [[concepts/KUDIG Knowledge Base Architecture.md|KUDIG Knowledge Base Architecture]] — Cross-reference
- [[AI基础设施/基础设施/03-gpu-scheduling-management.md|GPU 调度与管理]] — Cross-reference
- [[AI基础设施/基础设施/05-distributed-training-frameworks.md|分布式训练框架]] — Cross-reference
- 发布变更 MOC — Cross-reference
- [[skills/learn-decision-tree-mermaid.md|故障排查决策树 - Mermaid 可视化版]] — Cross-reference
- [[skills/skill-22-daemonset-failure.md|DaemonSet 故障诊断与修复 / DaemonSet Failure Diagnosis & Remediation]] — Cross-reference
- [[平台工程/运维/06-monitoring-alerting-system.md|监控告警体系]] — Cross-reference
- Domain 30: 企业级灾备与业务连续性 (Enterprise Disaster Recovery & Business Continuity) — Cross-reference
- [[entities/ecosystem-changelog.md|生态组件变更日志索引]] — Cross-reference
- [[生态参考/领域索引/cluster-index.md|Cluster 集群知识图谱索引]]
- [[生态参考/领域索引/pvc-index.md|PVC 知识图谱索引]]
- [[生态参考/领域索引/terway-index.md|Terway 知识图谱索引]]
- [[生态参考/领域索引/nginx-ingress-index.md|nginx-ingress-controller 知识图谱索引]]
- [[生态参考/领域索引/higress-index.md|Higress 知识图谱索引]]


<!-- risk-assessed -->
