---
title: Networking & Traffic
description: 整合原 domain-03-networking-traffic/15/26/35/40 的网络知识，涵盖 K8s 网络、CNI、Service Mesh、API Gateway 和 eBPF。
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
created: "2026-05-23"
---

# Networking & Traffic

整合原 domain-03-networking-traffic/15/26/35/40 的网络知识，涵盖 K8s 网络、CNI、[[Service|Service]]Service Mesh）|Service Mesh]]、API Gateway 和 eBPF。

## 目录结构

| 子目录 | 内容 |
|---|---|
| 00-core-k8s-networking/ | K8s 网络架构、Service、Ingress、NetworkPolicy |
| 01-fundamentals/ | TCP/IP、DNS、负载均衡、SDN |
| 02-service-mesh/ | Istio、Linkerd、Envoy |
| 03-api-gateway/ | Gateway API、Higress |
| 04-ebpf/ | eBPF 基础、Cilium |
| 99-attachments/ | 网络架构附件 (.xmind, .pptx) |

## 与其他 Domain 的关系

- [[domain-01-cluster-fundamentals/README|domain-01-cluster-fundamentals]] — 集群架构
- [[domain-10-troubleshooting-diagnostics/README|domain-10-troubleshooting-diagnostics]] — 网络排障

## Related

- Domain-34: CNCF Landscape 开源项目 — Cross-reference
- [[references/release-notes-networking|发布说明索引 — 网络]] — Cross-reference
- domain-03-networking-traffic MOC — Cross-reference
- Topic 应用层架构设计最佳实践 — Cross-reference
- topic-application-architecture MOC — Cross-reference
- [[concepts/bp-common-best-practices|Kubernetes 通用最佳实践参考]] — Cross-reference
- [[concepts/KUDIG Knowledge Base Architecture|KUDIG Knowledge Base Architecture]] — Cross-reference
- [[domain-14-ai-ml-infra/01-ai-infra/03-gpu-scheduling-management|GPU 调度与管理]] — Cross-reference
- [[domain-14-ai-ml-infra/01-ai-infra/05-distributed-training-frameworks|分布式训练框架]] — Cross-reference
- domain-08-release-change-management MOC — Cross-reference
- [[skills/learn-decision-tree-mermaid|故障排查决策树 - Mermaid 可视化版]] — Cross-reference
- [[skills/skill-22-daemonset-failure|DaemonSet 故障诊断与修复 / DaemonSet Failure Diagnosis & Remediation]] — Cross-reference
- [[domain-07-platform-engineering/operate/06-monitoring-alerting-system|监控告警体系]] — Cross-reference
- Domain 30: 企业级灾备与业务连续性 (Enterprise Disaster Recovery & Business Continuity) — Cross-reference
- [[entities/ecosystem-changelog|生态组件变更日志索引]] — Cross-reference
- [[domain-19-landscape-references/topic-index/cluster-index|Cluster 集群知识图谱索引]]
- [[domain-19-landscape-references/topic-index/pvc-index|PVC 知识图谱索引]]
- [[domain-19-landscape-references/topic-index/terway-index|Terway 知识图谱索引]]
- [[domain-19-landscape-references/topic-index/nginx-ingress-index|nginx-ingress-controller 知识图谱索引]]
- [[domain-19-landscape-references/topic-index/higress-index|Higress 知识图谱索引]]
