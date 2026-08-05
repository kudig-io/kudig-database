---
title: 综合目录索引
description: 跨域综合文章索引 — 将多个知识域的技术交叉融合，提供全景式深度分析
summary: 跨域综合文档目录，涵盖控制平面、网络、存储、安全、可观测性、AI基础设施、平台工程、可靠性等多域交叉分析
category: index
tags:
- index
- synthesis
- cross-domain
tier: core
created: '2026-07-02'
last_updated: '2026-07-21'
---

# 综合 (Synthesis)

> 跨域综合文章 — 将多个知识域的技术交叉融合，提供全景式深度分析。每篇文章探讨两个或多个技术/概念的交叉点、协同与张力。

## 控制平面与核心架构

- [[24-综合/07-平台与数据/kubernetes-etcd.md|Kubernetes × etcd]] — etcd 作为集群唯一状态存储的架构分析
- [[24-综合/03-网络与服务网格/kubernetes-service.md|Kubernetes × Service]] — Service 抽象与网络模型交叉
- [[24-综合/03-网络与服务网格/service-ingress.md|Service × Ingress]] — 服务暴露与流量入口协同

## 可观测性与监控

- [[24-综合/05-可观测性/kubernetes-prometheus.md|Kubernetes × Prometheus]] — 监控体系与 K8s 集成
- [[24-综合/05-可观测性/opentelemetry-prometheus.md|OpenTelemetry × Prometheus]] — 遥测标准与监控融合
- [[24-综合/05-可观测性/ebpf-observability.md|eBPF × Observability]] — 内核级可观测性
- [[24-综合/05-可观测性/slo-observability.md|SLO × Observability]] — SLO 驱动的可观测性实践
- [[24-综合/01-AI与机器学习/observability-ai-llm-monitoring.md|Observability × AI/LLM Monitoring]] — AI 工作负载监控

## 网络与安全

- [[24-综合/03-网络与服务网格/cilium-service-mesh.md|Cilium × Service Mesh]] — eBPF 驱动的服务网格
- [[24-综合/03-网络与服务网格/networkpolicy-service-mesh.md|NetworkPolicy × Service Mesh]] — 网络安全策略与网格协同
- [[24-综合/03-网络与服务网格/service-mesh-mtls-zero-trust.md|Service Mesh × mTLS × Zero Trust]] — 零信任网络架构
- [[24-综合/03-网络与服务网格/zero-trust-networkpolicy-segmentation.md|Zero Trust × NetworkPolicy × Segmentation]] — 网络分段与零信任
- [[24-综合/04-安全与合规/rbac-multitenancy.md|RBAC × Multi-tenancy]] — 多租户访问控制
- [[24-综合/04-安全与合规/cert-manager-tls.md|cert-manager × TLS]] — 自动化证书管理
- [[24-综合/04-安全与合规/sigstore-cosign-supply-chain.md|Sigstore × Cosign × Supply Chain]] — 软件供应链安全
- [[24-综合/04-安全与合规/container-runtime-image-security.md|Container Runtime × Image Security]] — 运行时与镜像安全
- [[24-综合/04-安全与合规/container-registry-image-scanning.md|Container Registry × Image Scanning]] — 镜像仓库与扫描
- [[24-综合/04-安全与合规/compliance-k8s-soc2-hipaa.md|Compliance × K8s × SOC2/HIPAA]] — 合规与审计

## 发布与平台工程

- [[24-综合/02-交付与GitOps/argocd-gitops.md|ArgoCD × GitOps]] — GitOps 持续部署实践
- [[24-综合/02-交付与GitOps/helm-gitops.md|Helm × GitOps]] — Helm 与 GitOps 工作流融合
- [[24-综合/02-交付与GitOps/argo-rollouts-progressive-delivery.md|Argo Rollouts × Progressive Delivery]] — 渐进式交付
- [[24-综合/02-交付与GitOps/crossplane-iac.md|Crossplane × IaC]] — 基础设施即代码
- [[24-综合/07-平台与数据/platform-engineering-devex.md|Platform Engineering × DevEx]] — 平台工程与开发者体验
- [[24-综合/02-交付与GitOps/multi-cluster-gitops-federation.md|Multi-cluster × GitOps × Federation]] — 多集群联邦管理
- [[24-综合/06-可靠性与成本/multitenancy-resource-isolation-governance.md|Multi-tenancy × Resource Isolation × Governance]] — 多租户治理
- [[24-综合/04-安全与合规/opa-kyverno-policy-as-code.md|OPA × Kyverno × Policy as Code]] — 策略即代码

## 弹性伸缩与成本

- [[24-综合/06-可靠性与成本/keda-hpa.md|KEDA × HPA]] — 事件驱动自动伸缩
- [[24-综合/06-可靠性与成本/autoscaling-cost-optimization.md|Autoscaling × Cost Optimization]] — 伸缩与成本平衡
- [[24-综合/01-AI与机器学习/ai-workload-cost-optimization-finops.md|AI Workload × Cost × FinOps]] — AI 工作负载成本治理
- [[24-综合/01-AI与机器学习/gpu-scheduling-cost.md|GPU Scheduling × Cost]] — GPU 调度与成本优化

## 存储与数据

- [[24-综合/07-平台与数据/statefulset-cloud-native-storage.md|StatefulSet × Cloud Native Storage]] — 有状态工作负载存储
- [[24-综合/01-AI与机器学习/storage-ai-workload-data-pipeline.md|Storage × AI Workload × Data Pipeline]] — AI 数据管道存储
- [[24-综合/07-平台与数据/kafka-database-cdc-streaming.md|Kafka × Database × CDC × Streaming]] — 数据流与变更捕获

## 可靠性与灾备

- [[24-综合/06-可靠性与成本/velero-disaster-recovery.md|Velero × Disaster Recovery]] — 集群备份与灾难恢复
- [[24-综合/06-可靠性与成本/backup-multicloud-dr-strategy.md|Backup × Multi-cloud × DR Strategy]] — 多云灾备策略
- [[24-综合/06-可靠性与成本/chaos-engineering-sre-resilience.md|Chaos Engineering × SRE × Resilience]] — 混沌工程与韧性

## AI 与机器学习

- [[24-综合/01-AI与机器学习/gpu-operator-device-plugin-ecosystem.md|GPU Operator × Device Plugin × Ecosystem]] — GPU 生态全景
- [[24-综合/01-AI与机器学习/feature-store-rag-ml-platform.md|Feature Store × RAG × ML Platform]] — ML 平台与 RAG
- [[24-综合/01-AI与机器学习/training-inference-data-lifecycle.md|Training × Inference × Data Lifecycle]] — AI 数据生命周期
- [[24-综合/01-AI与机器学习/ticket-agent-rag.md|Ticket Agent × RAG]] — 工单 Agent 与 RAG 集成

## 系统与性能

- [[24-综合/07-平台与数据/linux-kernel-container-performance.md|Linux Kernel × Container Performance]] — 内核与容器性能

---

## 综合文档写作规范

每篇综合文档应包含：
1. **The Connection** — 两个/多个技术的核心关联点
2. **Where They Co-occur** — 生产环境中的共同出现场景
3. **Cross-cutting Insight** — 跨域洞察与深层耦合分析
4. **Tensions and Trade-offs** — 张力与权衡矩阵
5. **Open Questions** — 未解决的开放问题
6. **Related** — 交叉引用链接

## Related

- [[27-标签/01-核心平台/k8s|k8s 标签枢纽]] — Kubernetes 核心知识
- [[27-标签/04-可观测性/observability|observability 标签枢纽]] — 可观测性知识
- [[27-标签/03-安全与合规/security|security 标签枢纽]] — 安全知识
- [[27-标签/05-交付与运维/production|production 标签枢纽]] — 生产实践

## 子目录

- [[24-综合/01-AI与机器学习/index.md|01-AI与机器学习]]
- [[24-综合/02-交付与GitOps/index.md|02-交付与GitOps]]
- [[24-综合/03-网络与服务网格/index.md|03-网络与服务网格]]
- [[24-综合/04-安全与合规/index.md|04-安全与合规]]
- [[24-综合/05-可观测性/index.md|05-可观测性]]
- [[24-综合/06-可靠性与成本/index.md|06-可靠性与成本]]
- [[24-综合/07-平台与数据/index.md|07-平台与数据]]

