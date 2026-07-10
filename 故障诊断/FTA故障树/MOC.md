---
title: topic-fta MOC
description: topic-fta 专题导航页，覆盖 79 篇文档
summary: topic-fta 专题导航页，覆盖 79 篇文档
category: moc
tags:
- k8s
- moc
- fta
- etcd
- apiserver
- controller-manager
- cilium
- flannel
- calico
- daemonset
tier: core
created: '2026-05-23'
last_updated: '2026-05-21'
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 技术支持
estimated_read_time: 15min
intent_queries:
- topic-fta MOC 是什么
- 如何 topic-fta MOC
- Kubernetes 10 troubleshooting diagnostics 最佳实践
- topic-fta MOC 故障排查
- topic-fta MOC 排障步骤
- topic-fta MOC 根因分析
trigger_keywords:
- topic-fta
- MOC
- troubleshooting
- diagnostics
- fta
prerequisites:
- kubectl-basics
- troubleshooting-methodology
- ebpf-basics
- cilium-basics
- cni-basics
- etcd-basics
- gpu-scheduling-basics
fta_id: FTA-MOC-001
component: Moc
severity: high
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# topic-fta MOC

> **MOC 版本**: 1.0
> **专题**: topic-fta
> **文档数量**: 79 篇
> **最后更新**: 2026-05-21
> **用途**: 本专题的导航入口，汇总所有相关文档

---

## 专题概述

FTA 故障树 — 故障树分析文档集合

### 专题定位

| 维度 | 说明 |
|---|---|
| **专题** | topic-fta |
| **文档数量** | 79 篇（展示前 50 篇） |
| **难度分布** | 入门 0 / 进阶 1 / 高级 39 / 专家 0 |

---

## 文档清单

| # | 文档 | 难度 | 标签 | 估计阅读时间 |
|---|---|---|---|---|
| 1 | [[故障诊断/FTA故障树/01-fta-origin-and-evolution.md|[[第一章：FTA 起源与发展史|第一章：FTA 起源与发展史]]]] |  | fta, troubleshooting |  |
| 2 | [[故障诊断/FTA故障树/02-fta-mathematical-foundations.md|[[第二章：FTA 数学基础与理论模型|第二章：FTA 数学基础与理论模型]]]] |  | fta, troubleshooting |  |
| 3 | [[故障诊断/FTA故障树/03-fta-symbol-system-and-standards.md|[[第三章：FTA 符号体系与标准规范|第三章：FTA 符号体系与标准规范]]]] |  | fta, troubleshooting |  |
| 4 | [[故障诊断/FTA故障树/04-fta-core-principles.md|第四章：FTA 方法论核心原则]] |  | fta, troubleshooting |  |
| 5 | [[故障诊断/FTA故障树/05-fta-construction-process.md|第五章：FTA 构建完整流程]] |  | fta, troubleshooting |  |
| 6 | [[故障诊断/FTA故障树/06-fta-verification-and-quality.md|第六章：FTA 验证与质量保证]] |  | fta, troubleshooting |  |
| 7 | [[故障诊断/FTA故障树/07-fta-maintenance-and-evolution.md|第七章：FTA 维护与演进策略]] |  | fta, troubleshooting |  |
| 8 | [[故障诊断/FTA故障树/08-ai-agent-ops-revolution.md|第八章：AI Agent 时代的运维范式革命]] |  | fta, troubleshooting, daily-ops |  |
| 9 | [[故障诊断/FTA故障树/09-fta-as-agent-knowledge-skeleton.md|第九章：FTA 作为 AI Agent 的知识骨架]] |  | fta, troubleshooting |  |
| 10 | [[故障诊断/FTA故障树/10-agent-orchestration-patterns.md|第十章：Agent 编排模式与 FTA 逻辑门映射]] |  | fta, troubleshooting |  |
| 11 | [[故障诊断/FTA故障树/11-fta-driven-runbook-automation.md|第十一章：FTA 驱动的 Runbook 自动化]] |  | fta, troubleshooting |  |
| 12 | [[故障诊断/FTA故障树/12-fta-aiops-integration.md|第十二章：FTA 与 AIOps 平台集成架构]] |  | fta, troubleshooting, daily-ops |  |
| 13 | [[故障诊断/FTA故障树/13-intelligent-ticket-processing.md|第十三章：智能工单处理的 AI Agent 架构]] |  | fta, troubleshooting |  |
| 14 | [[故障诊断/FTA故障树/14-fta-system-engineering.md|第十四章：构建 FTA 系统的工程化方法]] |  | fta, troubleshooting |  |
| 15 | [[故障诊断/FTA故障树/15-fta-quality-assessment.md|第十五章：FTA 质量评估与优化]] |  | fta, troubleshooting |  |
| 16 | [[故障诊断/FTA故障树/16-team-capability-building.md|第十六章：团队能力建设]] |  | fta, troubleshooting |  |
| 17 | [[故障诊断/FTA故障树/17-industry-benchmarks.md|第十七章：行业标杆案例分析]] |  | fta, troubleshooting, performance |  |
| 18 | [[故障诊断/FTA故障树/18-typical-scenarios.md|第十八章：典型场景完整方案]] |  | fta, troubleshooting |  |
| 19 | [[故障诊断/FTA故障树/19-pitfalls-and-best-practices.md|第十九章：避坑指南与常见误区]] |  | fta, troubleshooting, best-practice |  |
| 20 | [[故障诊断/FTA故障树/20-fta-llm-opportunities.md|第二十章：FTA + 大语言模型的新机遇]] |  | fta, troubleshooting |  |
| 21 | [[故障诊断/FTA故障树/21-self-evolving-ops-system.md|第二十一章：自进化的智能运维系统]] |  | fta, troubleshooting, daily-ops |  |
| 22 | [[故障诊断/FTA故障树/22-industry-standardization.md|第二十二章：行业标准化建议]] |  | fta, troubleshooting |  |
| 23 | [[故障诊断/FTA故障树/23-fta-production-quick-start.md|第23章：FTA 生产环境快速启动与 SRE 集成指南]] |  | fta, troubleshooting, production |  |
| 24 | [[故障诊断/FTA故障树/ack-fta-generator-v2.md|ACK-FTA 生成器增强版提示词]] |  | fta, troubleshooting |  |
| 25 | [[故障诊断/FTA故障树/appendix-a-glossary.md|附录 A：FTA 术语表]] |  | fta, troubleshooting |  |
| 26 | [[故障诊断/FTA故障树/appendix-b-tools-and-resources.md|附录 B：工具与资源清单]] |  | fta, troubleshooting |  |
| 27 | [[故障诊断/FTA故障树/appendix-c-references.md|附录 C：参考文献]] |  | fta, troubleshooting, reference |  |
| 28 | [[故障诊断/FTA故障树/appendix-d-templates.md|附录 D：FTA 模板参考 (历史参考)]] |  | fta, troubleshooting |  |
| 29 | [[故障诊断/FTA故障树/fta-diagnosis-improvement.md|FTA 排查逻辑改进建议]] |  | fta, troubleshooting |  |
| 30 | [[故障诊断/FTA故障树/fta-execution-engine.md|FTA 诊断执行引擎]] |  | fta, troubleshooting |  |
| 31 | [[故障诊断/FTA故障树/fta-index.md|FTA 故障树完整索引]] |  | fta, troubleshooting |  |
| 32 | [[故障诊断/FTA故障树/fta-methodology-and-agentic-practices.md|FTA 故障树分析方法论与 AI Agent 智能运维实践]] |  | fta, troubleshooting |  |
| 33 | [[故障诊断/FTA故障树/kubernetes-fta-full-analysis-v2.md|Kubernetes 全量故障树分析(FTA)排查手册 - 增强版]] |  | fta, troubleshooting |  |
| 34 | [[故障诊断/FTA故障树/kubernetes-fta-full-analysis.md|Kubernetes 全量故障树分析(FTA)排查手册]] |  | fta, troubleshooting |  |
| 35 | apiserver-fta.md | 高级 | fta, troubleshooting, apiserver | 5min |
| 36 | backup-restore-fta.md | 高级 | fta, troubleshooting, backup | 5min |
| 37 | [[故障诊断/FTA故障树/list/calico-fta.md|calico FTA 树：Calico CNI 故障诊断]] |  | fta, troubleshooting |  |
| 38 | [[故障诊断/FTA故障树/list/certificate-fta.md|证书异常故障树分析]] | 高级 | fta, troubleshooting, certificate | 5min |
| 39 | [[故障诊断/FTA故障树/list/cilium-fta.md|cilium FTA 树：eBPF/Cilium CNI 故障诊断]] |  | fta, troubleshooting |  |
| 40 | [[故障诊断/FTA故障树/list/cloud-provider-fta.md|云平台集成异常故障树分析]] | 高级 | fta, troubleshooting, cloud-provider | 5min |
| 41 | [[故障诊断/FTA故障树/list/cluster-autoscaler-fta.md|Cluster Autoscaler 异常故障树分析]] | 高级 | fta, troubleshooting, cluster-autoscaler | 5min |
| 42 | [[故障诊断/FTA故障树/list/cluster-upgrade-fta.md|集群升级异常故障树分析]] | 高级 | fta, troubleshooting, cluster-upgrade | 5min |
| 43 | [[故障诊断/FTA故障树/list/controller-manager-fta.md|Controller Manager 异常故障树分析]] | 高级 | fta, troubleshooting, controller-manager | 5min |
| 44 | [[故障诊断/FTA故障树/list/crd-operator-fta.md|CRD/Operator 异常故障树分析]] | 高级 | fta, troubleshooting, crd | 5min |
| 45 | csi-fta.md | 高级 | fta, troubleshooting, csi | 5min |
| 46 | [[故障诊断/FTA故障树/list/daemonset-fta.md|DaemonSet 异常故障树分析]] | 高级 | fta, troubleshooting, daemonset | 5min |
| 47 | [[故障诊断/FTA故障树/list/deployment-fta.md|Deployment 异常故障树分析]] | 高级 | fta, troubleshooting, deployment | 5min |
| 48 | [[故障诊断/FTA故障树/list/dns-fta.md|DNS 异常故障树分析]] | 高级 | fta, troubleshooting, dns | 5min |
| 49 | [[故障诊断/FTA故障树/list/etcd-fta.md|etcd 异常故障树分析]] | 高级 | fta, troubleshooting, etcd | 5min |
| 50 | flannel-fta.md | 高级 | fta, troubleshooting, flannel | 5min |
| ... | 共 79 篇文档 | | | |

---

## 统计信息

| 指标 | 数值 |
|---|---|
| 文档总数 | 79 |

---

*本文档由 scripts/generate-mocs.py 自动生成，最后更新 2026-05-21。*

## Related

- Wiki Lint Report — 2026-05-21 — Cross-reference
- [[entities/release-notes-storage.md|发布说明索引 — 存储]] — Cross-reference
- [[entities/release-notes-observability.md|发布说明索引 — 可观测性]] — Cross-reference
- [[entities/release-notes-networking.md|发布说明索引 — 网络]] — Cross-reference
- [[entities/release-notes-kubernetes.md|发布说明索引 — Kubernetes]] — Cross-reference
- [[entities/release-notes-security.md|发布说明索引 — 安全]] — Cross-reference
- [[entities/k8s-knowledge-map.md|Kubernetes Knowledge Map]] — Cross-reference
- [[entities/release-notes-cicd-gitops.md|发布说明索引 — CI/CD 与 GitOps]] — Cross-reference
- [[entities/release-notes-cli-tools.md|发布说明索引 — CLI 工具]] — Cross-reference
- [[entities/release-notes-core-deps.md|发布说明索引 — 核心依赖]] — Cross-reference
- [[entities/k8s-difficulty-index.md|Kubernetes Difficulty Index]] — Cross-reference
- 网络 MOC — Cross-reference
- [[网络/K8s网络核心/02-cni-architecture-fundamentals.md|CNI 架构与核心原理]] — Cross-reference
- [[可观测性/总览/01-observability-architecture-overview.md|Kubernetes 可观测性架构体系]] — Cross-reference
- [[AI基础设施/基础设施/03-gpu-scheduling-management.md|GPU 调度与管理]] — Cross-reference
- [[AI基础设施/基础设施/05-distributed-training-frameworks.md|分布式训练框架]] — Cross-reference
- 发布变更 MOC — Cross-reference
- [[集群基础/kubectl/05-kubectl-commands-reference.md|kubectl 命令完整参考]] — Cross-reference
- [[集群基础/架构总览/02-core-components-deep-dive.md|Kubernetes 核心组件深度剖析]] — Cross-reference
- [[存储/K8s存储/02-pv-architecture-fundamentals.md|PV/PVC 核心概念与企业级实践]] — Cross-reference
- [[存储/K8s存储/01-storage-architecture-overview.md|存储架构概览与核心组件]] — Cross-reference
- [[生态参考/领域索引/MOC.md|topic-index MOC]] — Cross-reference


<!-- risk-assessed -->
