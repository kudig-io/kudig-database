---
title: Kubernetes (CNCF Graduated)
description: '## 概述'
summary: 'description: ''## 项目概述'''
category: entities
tags:
- k8s
- cncf
- observability
- kubernetes
- etcd
- scheduler
- prometheus
- grafana
- istio
- cilium
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 15min
intent_queries:
- Kubernetes (CNCF Graduated) 是什么
- 如何 Kubernetes (CNCF Graduated)
trigger_keywords:
- Kubernetes
- CNCF
- Graduated
prerequisites:
- kubectl-basics
- service-mesh-basics
- prometheus-basics
- monitoring-basics
- gitops-basics
- ebpf-basics
- cilium-basics
- etcd-basics
- kafka-basics
- redis-basics
- gpu-scheduling-basics
- observability-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# [[Kubernetes|Kubernetes]] (CNCF Graduated)

> **CNCF 状态**: Graduated | **类别**: Observability | **主要语言**: Go

## 概述

description: '## 项目概述'

## 核心能力

- **容器编排**: 自动化容器的部署、扩展和运维
- **服务发现**: 内置 DNS 和负载均衡
- **自动恢复**: 自动重启失败容器、替换节点
- **滚动更新**: 零停机部署和回滚
- **配置管理**: ConfigMap 和 Secret 管理
- **存储编排**: 自动挂载存储系统

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[22-概念/01-核心架构/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- 使用高可用控制平面（3+ 节点）
- 配置资源限制（requests/limits）
- 启用 RBAC 和 Pod Security Standards
- 定期备份 etcd 数据
- 使用节点亲和性和反亲和性
- 合理配置 API Server 限流参数

## 架构定位

在 CNCF 生态中，kubernetes 属于 **Observability** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[etcd]]
- [[23-实体/07-可观测性/prometheus-grafana.md|prometheus-grafana]]
- [[istio]]
- [[containerd]]
- [[23-实体/08-交付与制品/argocd.md|argocd]]

## Related

- [[23-实体/15-参考与索引/cncf-orchestration.md|cncf-orchestration]] — CNCF 编排与应用管理项目全景
- [[prometheus]] — Prometheus
- [[interlink]] — InterLink
- [[23-实体/15-参考与索引/kubernetes-changelog.md|kubernetes-changelog]] — Kubernetes 变更日志索引
- [[etcd]] — etcd

- [[02-工作负载/02-Java-on-K8s/06-java-cicd-tekton-argocd.md|06-java-cicd-tekton-argocd]]
- ovn-kubernetes
- 99-spring-cloud-kubernetes-service-mesh-guide
- 08-kubernetes-network-policies-security-micro-segmentation
- 19-kubernetes-gateway-api-modern-traffic-management
- 18-kubernetes-ebpf-cilium-deep-practice
- 13-kubernetes-multi-tenancy-security-isolation-resource-quota
- 16-kubernetes-edge-computing-kubeedge-practice
- 09-kubernetes-service-mesh-istio-integration
- 20-kubernetes-supply-chain-security-sbom-slsa-sigstore
- 12-kubernetes-scheduler-deep-optimization-custom-scheduling
- 23-kubernetes-opentelemetry-native-observability
- 02-kubernetes-large-scale-performance-optimization
- 03-kubernetes-zero-trust-security-architecture
- 11-kubernetes-api-server-deep-optimization-extension
- 05-kubernetes-gitops-complete-practice-guide
- 15-kubernetes-chaos-engineering-fault-injection-testing
- 22-kubernetes-webassembly-wasm-workloads
- 10-kubernetes-automation-sre-practices
- 17-kubernetes-aiml-gpu-scheduling-llm-inference
- 01-kubernetes-production-readiness-assessment
- 26-kubernetes-vcluster-virtual-cluster-multi-tenancy
- 06-kubernetes-cost-governance-finops-practice
- 21-kubernetes-platform-engineering-internal-developer-platform
- 14-kubernetes-event-driven-architecture-asynchronous-processing
- 24-kubernetes-policy-as-code-governance-automation
- 07-kubernetes-csi-storage-deep-practice
- 04-kubernetes-multi-cloud-hybrid-deployment
- 99-kubernetes-v1.33-observability-guide
- 99-java-observability-kubernetes-guide
- 99-kubernetes-v1.33-platform-ops-guide
- 99-kubernetes-deployment-patterns-architecture
- 99-kubernetes-multi-tenant-architecture
- [[01-集群基础/00-总览/99-kubernetes-production-architecture-blueprint.md|99-kubernetes-production-architecture-blueprint]]
- 13-kubernetes-cost-governance
- 99-kubernetes-v1.33-workloads-guide
- 99-spring-boot-kubernetes-guide
- 99-kubernetes-v1.33-design-principles-evolution
- 99-kubernetes-developer-toolchain-guide
- 09-job-cronjob-batch-events
- 11-storage-volume-events
- 12-autoscaling-events
- 03-image-pull-events
- 15-ecosystem-addon-events
- 08-statefulset-daemonset-events
- [[17-系统基础/README.md|Domain-33: Kubernetes Events 全域事件大全]]
- 02-pod-container-lifecycle-events
- 00-open-source-projects-index
- 04-probe-health-check-events
- 13-security-admission-rbac-events
- 05-scheduling-preemption-events
- 01-event-system-architecture
- 07-deployment-replicaset-events
- 06-node-lifecycle-condition-events
- domain-33-kubernetes-events MOC
- 14-namespace-resource-gc-events
- 10-service-networking-events
- 07-kubernetes-backup-restore-deep-dive
- 07-redis-kubernetes-operator
- 08-kafka-kubernetes-strimzi
- 04-database-middleware-kubernetes
- 11-kubernetes-source-code-architecture
- 99-kubernetes-core-components-v1.29-v1.33-update
- 99-kubernetes-v1.25-v1.33-feature-comparison-table
- 99-kubernetes-version-lifecycle-support-policy
- 99-kubernetes-v1.33-deprecation-migration-guide
- 99-kubernetes-core-features-mermaid-diagrams
- 99-kubernetes-v1.33-practical-cookbook
- 99-kubernetes-v1.29-v1.33-features-guide
- 99-kubernetes-api-version-matrix
- 99-kubernetes-v1.33-ecosystem-compatibility-matrix
- 99-kubernetes-v1.33-quick-reference-card
- 99-kubernetes-v1.33-upgrade-guide
- 99-kubernetes-v1.33-production-best-practices
- Kubernetes 架构全景图
- 99-kubernetes-v1.29-v1.33-complete-feature-gates-reference
- 13-kubernetes-operations-fundamentals
- 11-kubernetes-security-hardening
- 99-java-security-kubernetes-guide
- 05-ibm-cloud-kubernetes-service-enterprise
- 02-kubernetes-gateway-api-deep-dive
- [[02-工作负载/02-Java-on-K8s/05-quarkus-native-kubernetes.md|05-quarkus-native-kubernetes]]
- [[02-工作负载/02-Java-on-K8s/02-spring-boot-kubernetes-production.md|02-spring-boot-kubernetes-production]]
- [[02-工作负载/02-Java-on-K8s/07-java-observability-kubernetes.md|07-java-observability-kubernetes]]
- [[02-工作负载/02-Java-on-K8s/03-jvm-gc-container-tuning.md|03-jvm-gc-container-tuning]]
- [[02-工作负载/02-Java-on-K8s/04-java-operator-sdk-development.md|04-java-operator-sdk-development]]
- [[02-工作负载/README.md|Java on Kubernetes 综合实践指南]]
- domain-java-kubernetes MOC
- 16-kubernetes-hardware-troubleshooting
- [[19-故障诊断/06-FTA故障树/kubernetes-fta-full-analysis-v2.md|kubernetes-fta-full-analysis-v2]]
- [[19-故障诊断/06-FTA故障树/kubernetes-fta-full-analysis.md|kubernetes-fta-full-analysis]]
- vscode-kubernetes-tools
- Wiki 全量知识库摘要 — 2026-05-21 — Cross-reference
- Wiki Lint Report — 2026-05-21 — Cross-reference
- [[23-实体/15-参考与索引/KUDIG Cheat Sheet Index.md|KUDIG Cheat Sheet Index]] — Cross-reference
- [[23-实体/15-参考与索引/specialized-workloads-terms.md|K8s 专用工作负载术语参考]] — Cross-reference
- [[23-实体/15-参考与索引/linux-sysctl-reference.md|Linux Sysctl Reference for Kubernetes]] — Cross-reference
- [[23-实体/15-参考与索引/networking-terms.md|K8s 网络术语参考]] — Cross-reference
- [[23-实体/15-参考与索引/k8s-workloads-domain-guide.md|Kubernetes Workloads Domain Guide]] — Cross-reference
- [[23-实体/15-参考与索引/k8s-design-principles-deep-dive.md|设计原理：声明式 API、控制器模式与 etcd 共识]] — Cross-reference
- [[23-实体/15-参考与索引/kubernetes-port-reference.md|Kubernetes Port Reference]] — Cross-reference
- [[23-实体/15-参考与索引/workloads-terms.md|K8s 工作负载术语参考]] — Cross-reference
- [[23-实体/15-参考与索引/k8s-glossary-index.md|K8s 术语表索引]] — Cross-reference
- [[23-实体/15-参考与索引/fundamentals-terms.md|K8s 基础概念术语参考]] — Cross-reference
- [[23-实体/15-参考与索引/release-notes-kubernetes.md|发布说明索引 — Kubernetes]] — Cross-reference
- [[23-实体/15-参考与索引/k8s-architecture-fundamentals.md|K8s 架构基础与核心组件原理]] — Cross-reference
- [[23-实体/15-参考与索引/root-terms.md|K8s Root术语参考]] — Cross-reference
- [[23-实体/15-参考与索引/scheduling-terms.md|K8s 调度术语参考]] — Cross-reference
- [[23-实体/15-参考与索引/kudig-contribution-guide.md|贡献指南、项目概览与版本发布说明]] — Cross-reference
- [[21-生态参考/98-merged-indexes/index.md|发布说明阅读指南]] — Cross-reference
- [[23-实体/15-参考与索引/k8s-advanced-ecosystem.md|硬件知识体系、CNCF 全景生态与 eBPF 平台工程]] — Cross-reference
- [[23-实体/15-参考与索引/storage-terms.md|K8s 存储术语参考]] — Cross-reference
- [[23-实体/15-参考与索引/observability-terms.md|K8s 可观测性术语参考]] — Cross-reference
- [[23-实体/15-参考与索引/kubectl Scenario Quick Reference.md|kubectl Scenario Quick Reference]] — Cross-reference
- [[23-实体/15-参考与索引/kubectl-quick-reference.md|Kubectl Quick Reference]] — Cross-reference
- [[23-实体/15-参考与索引/k8s-deployment-create.md|Kubernetes Deployment 创建操作指南]] — Cross-reference
- [[23-实体/15-参考与索引/k8s-knowledge-map.md|Kubernetes Knowledge Map]] — Cross-reference
- [[23-实体/15-参考与索引/k8s-cluster-delete.md|Kubernetes 集群删除操作指南]] — Cross-reference
- [[23-实体/15-参考与索引/release-notes-cli-tools.md|发布说明索引 — CLI 工具]] — Cross-reference
- [[23-实体/15-参考与索引/KUDIG Frontmatter Spec.md|KUDIG Frontmatter Specification]] — Cross-reference
- [[23-实体/15-参考与索引/k8s-cluster-create.md|Kubernetes 集群创建操作指南]] — Cross-reference
- [[23-实体/15-参考与索引/configuration-terms.md|K8s 配置管理术语参考]] — Cross-reference
- [[23-实体/15-参考与索引/release-notes-core-deps.md|发布说明索引 — 核心依赖]] — Cross-reference
- [[23-实体/15-参考与索引/k8s-ai-infra-domain-guide.md|AI Infrastructure on Kubernetes Domain Guide]] — Cross-reference
- [[23-实体/15-参考与索引/tooling-terms.md|K8s 工具链术语参考]] — Cross-reference
- [[23-实体/15-参考与索引/k8s-cluster-cert.md|Kubernetes 集群证书管理操作指南]] — Cross-reference
- [[23-实体/15-参考与索引/k8s-node-create.md|Kubernetes 节点管理操作指南]] — Cross-reference
- [[23-实体/15-参考与索引/platform-engineering-terms.md|K8s 平台工程术语参考]] — Cross-reference
- [[23-实体/15-参考与索引/multi-cloud-terms.md|K8s 多云架构术语参考]] — Cross-reference
- [[23-实体/15-参考与索引/kudig-man-pages-index.md|KUDIG Man Pages Index]] — Cross-reference
- [[23-实体/15-参考与索引/version-upgrade-guide.md|版本升级指南]] — Cross-reference
- [[23-实体/15-参考与索引/k8s-difficulty-index.md|Kubernetes Difficulty Index]] — Cross-reference
- [[23-实体/15-参考与索引/k8s-networking-domain-guide.md|Kubernetes Networking Domain Guide]] — Cross-reference
- [[23-实体/15-参考与索引/operations-terms.md|K8s 运维运营术语参考]] — Cross-reference
- [[23-实体/15-参考与索引/kubernetes-api-versions-reference.md|Kubernetes API Versions Reference]] — Cross-reference
- [[22-概念/08-可靠性与运维/kubeadm-cluster-operations.md|kubeadm 集群运维全景]] — Cross-reference
- [[22-概念/11-交叉分析/etcd × 高可用模式.md|etcd × 高可用模式]] — Cross-reference
- [[22-概念/08-可靠性与运维/k8s-mttr-benchmark.md|K8s 问题分布与 MTTR 基准]] — Cross-reference
- [[22-概念/08-可靠性与运维/Structural Troubleshooting Framework.md|Structural Troubleshooting Framework]] — Cross-reference
- [[22-概念/11-交叉分析/声明式 API × 控制器模式.md|声明式 API × 控制器模式]] — Cross-reference
- [[22-概念/11-交叉分析/eBPF × 运行时安全.md|eBPF x 运行时安全]] — Cross-reference
- [[22-概念/02-工作负载/deployment-controller-architecture.md|Deployment 控制器架构]] — Cross-reference
- [[22-概念/10-最佳实践/bp-common-best-practices.md|Kubernetes 通用最佳实践参考]] — Cross-reference
- [[22-概念/05-安全/kubernetes-pki-certificate-system.md|Kubernetes PKI 证书体系]] — Cross-reference
- [[22-概念/10-最佳实践/bp-infrastructure.md|最佳实践：Infrastructure]] — Cross-reference
- [[22-概念/10-最佳实践/bp-observability.md|最佳实践：Observability]] — Cross-reference
- [[22-概念/10-最佳实践/bp-operations.md|最佳实践：Operations]] — Cross-reference
- [[22-概念/01-核心架构/declarative-api.md|Declarative API]] — Cross-reference
- [[22-概念/01-核心架构/core-dependency-version-matrix.md|核心依赖版本矩阵]] — Cross-reference
- [[22-概念/12-研究/kubernetes-version-evolution.md|Kubernetes 版本演进]] — Cross-reference
- [[22-概念/05-安全/multi-tenancy-isolation.md|Multi-Tenancy Isolation]] — Cross-reference
- [[22-概念/12-研究/cli-tools-evolution.md|CLI 工具演进]] — Cross-reference
- [[22-概念/01-核心架构/etcd Operational Reference.md|etcd Operational Reference]] — Cross-reference
- [[22-概念/12-研究/ai-agent-openclaw-workspace.md|OpenClaw 工作空间配置]] — Cross-reference
- [[22-概念/12-研究/ai-agent-README.md|AI Agent 工程专题]] — Cross-reference
- [[22-概念/03-网络/cni-networking-model.md|CNI 网络模型与插件对比]] — Cross-reference
- [[22-概念/15-运行时与系统/linux-sysctl-tuning.md|Linux Sysctl Tuning for Kubernetes]] — Cross-reference
- [[22-概念/12-研究/storage-tool-evolution.md|存储工具演进]] — Cross-reference
- [[35-元数据/metadata/KUDIG Knowledge Base Architecture.md|KUDIG Knowledge Base Architecture]] — Cross-reference
- [[22-概念/10-最佳实践/bp-README.md|Kubernetes 最佳实践指南]] — Cross-reference
- [[22-概念/01-核心架构/eventual-consistency.md|Eventual Consistency in Kubernetes]] — Cross-reference
- [[22-概念/10-最佳实践/k8s-production-best-practices.md|Kubernetes 生产环境最佳实践]] — Cross-reference
- [[22-概念/08-可靠性与运维/node-lifecycle-management.md|节点生命周期管理]] — Cross-reference
- [[22-概念/10-最佳实践/production-operations-best-practices.md|Production Operations Best Practices]] — Cross-reference
- [[22-概念/10-最佳实践/bp-security.md|最佳实践：Security]] — Cross-reference
- [[22-概念/12-研究/observability-stack-evolution.md|可观测性栈演进]] — Cross-reference
- [[22-概念/12-研究/security-tool-evolution.md|安全工具演进]] — Cross-reference
- [[22-概念/01-核心架构/watch-mechanism.md|Watch Mechanism (List-Watch)]] — Cross-reference
- [[22-概念/12-研究/gitops-tool-evolution.md|GitOps 工具演进]] — Cross-reference
- [[22-概念/05-安全/linux-security-modules.md|Linux Security Modules for Containers]] — Cross-reference
- [[26-技能/05-网络/ingress/培训/learn-05-ingress-basics.md|第五课：Ingress - 外部 HTTP/HTTPS 访问]] — Cross-reference
- [[26-技能/04-工作负载/pod/培训/learn-01-day-one-checklist.md|Day 1: 新人首日检查清单]] — Cross-reference
- [[26-技能/06-存储/csi-storage/最佳实践/k8s-storage-configuration-guide.md|Kubernetes 存储配置最佳实践]] — Cross-reference
- [[26-技能/04-工作负载/hpa-vpa/最佳实践/k8s-scaling-guide.md|Kubernetes 扩缩容最佳实践]] — Cross-reference
- [[26-技能/02-控制面/etcd/最佳实践/k8s-disaster-recovery-guide.md|Kubernetes 灾难恢复最佳实践]] — Cross-reference
- [[26-技能/03-节点/gpu/诊断排障/ts-ai-ml-workloads.md|AI/ML 工作负载排查]] — Cross-reference
- [[26-技能/05-网络/dns/dns-fta.md|DNS 异常故障树分析]] — Cross-reference
- [[26-技能/04-工作负载/pod/培训/learn-decision-tree-mermaid.md|故障排查决策树 - Mermaid 可视化版]] — Cross-reference
- [[26-技能/03-节点/node-fta.md|Node 异常故障树分析]] — Cross-reference
- [[26-技能/07-安全/certificate/kubelet-certificate-rotation.md|kubelet 证书轮换机制]] — Cross-reference
- [[26-技能/04-工作负载/pod/培训/learn-README.md|新人上手快速路径（Quick Start）]] — Cross-reference
- [[26-技能/04-工作负载/pod/培训/测验/assessment-k8s-fundamentals-quiz-answers.md|K8S Fundamentals Quiz Answers]] — Cross-reference
- [[26-技能/05-网络/networkpolicy/最佳实践/k8s-network-security-guide.md|Kubernetes 网络安全最佳实践]] — Cross-reference
- [[26-技能/03-节点/node/诊断排障/ts-node-components.md|节点组件故障排查]] — Cross-reference
- [[26-技能/04-工作负载/daemonset/培训/learn-13-daemonset-basics.md|第13课：DaemonSet 与节点守护]] — Cross-reference
- [[26-技能/01-集群运维/kubeadm/kubeadm-cluster-lifecycle.md|kubeadm 集群创建生命周期]] — Cross-reference
- [[26-技能/08-可观测性/monitoring/最佳实践/k8s-logging-management-guide.md|Kubernetes 日志管理最佳实践]] — Cross-reference
- [[26-技能/05-网络/networkpolicy/skill-20-networkpolicy-connectivity.md|NetworkPolicy 连通性故障诊断 / NetworkPolicy Connectivity Troubleshooting]] — Cross-reference
- [[26-技能/04-工作负载/pod/培训/测验/assessment-troubleshooting-lab-exam.md|Troubleshooting Lab Exam]] — Cross-reference
- [[26-技能/08-可观测性/monitoring/最佳实践/k8s-monitoring-guide.md|Kubernetes 监控最佳实践]] — Cross-reference
- [[26-技能/04-工作负载/deployment/deployment-canary-and-bluegreen.md|金丝雀与蓝绿发布]] — Cross-reference
- [[26-技能/03-节点/node/skill-notready/skill-k8s-node-notready-USAGE-GUIDE.md|Usage Guide]] — Cross-reference
- [[26-技能/04-工作负载/pod/培训/learn-01-what-is-kubernetes.md|第一课：Kubernetes 入门]] — Cross-reference
- [[26-技能/07-安全/rbac/诊断排障/ts-security-auth.md|安全认证故障排查]] — Cross-reference
- [[26-技能/01-集群运维/cluster-upgrade/reference/skill-reference-version-matrix.md|Version Matrix]] — Cross-reference
- [[26-技能/02-控制面/crd-operator/运维操作/develop-crd-operator.md|Develop CRD Operator]] — Cross-reference
- [[26-技能/04-工作负载/job-cronjob/skill-23-job-cronjob-failure.md|Job/CronJob 故障诊断与修复 / Job & CronJob Failure Diagnosis & Remediation]] — Cross-reference
- [[26-技能/03-节点/node/运维操作/node-drain-and-maintenance.md|节点驱逐与维护]] — Cross-reference
- [[26-技能/08-可观测性/monitoring/最佳实践/k8s-distributed-tracing-guide.md|Kubernetes 分布式追踪最佳实践]] — Cross-reference
- [[26-技能/04-工作负载/statefulset/skill-21-statefulset-failure.md|StatefulSet 故障诊断与修复 / StatefulSet Failure Diagnosis & Remediation]] — Cross-reference
- [[26-技能/04-工作负载/pod/方法论/Kubernetes Diagnostic Skills Overview.md|Kubernetes Diagnostic Skills Overview]] — Cross-reference
- [[26-技能/01-集群运维/kubeadm/kubeadm-cluster-deletion.md|kubeadm 集群删除操作]] — Cross-reference
- [[26-技能/01-集群运维/kubeadm/kubeadm-ha-cluster-setup.md|kubeadm 高可用集群搭建]] — Cross-reference
- [[26-技能/04-工作负载/deployment/最佳实践/k8s-deployment-strategies-guide.md|Kubernetes 部署策略最佳实践]] — Cross-reference
- [[26-技能/01-集群运维/cluster-upgrade/最佳实践/k8s-cluster-configuration-guide.md|Kubernetes 集群配置最佳实践]] — Cross-reference
- [[26-技能/04-工作负载/pod/方法论/skill-reference-diagnostic-workflow.md|Diagnostic Workflow]] — Cross-reference
- [[26-技能/04-工作负载/daemonset/skill-22-daemonset-failure.md|DaemonSet 故障诊断与修复 / DaemonSet Failure Diagnosis & Remediation]] — Cross-reference
- [[26-技能/02-控制面/apiserver/诊断排障/ts-control-plane.md|控制平面故障排查]] — Cross-reference
- [[26-技能/04-工作负载/pod/方法论/skill-reference-remediation-playbook.md|Remediation Playbook]] — Cross-reference
- [[26-技能/04-工作负载/pod/培训/learn-lecturer-persona.md|K8S 讲师角色设定与场景规范]] — Cross-reference
- [[26-技能/04-工作负载/pod/培训/learn-inner-training.md|Kubernetes 培训：Inner Training]] — Cross-reference
- [[26-技能/02-控制面/scheduler/培训/learn-15-scheduling-basics.md|第15课：调度与亲和性]] — Cross-reference
- [[26-技能/04-工作负载/pod/培训/测验/assessment-daily-check-quiz.md|Daily Check Quiz]] — Cross-reference
- [[26-技能/04-工作负载/pod/方法论/skill-reference-root-cause-catalog.md|Root Cause Catalog]] — Cross-reference
- [[26-技能/04-工作负载/pod/培训/learn-root.md|Kubernetes 培训：Root]] — Cross-reference
- [[26-技能/04-工作负载/pod/方法论/skills-run-README.md|Skills Demo — 本地运行工单诊断技能]] — Cross-reference
- [[26-技能/04-工作负载/deployment/deployment-workload-selection.md|工作负载控制器选型]] — Cross-reference
- [[26-技能/05-网络/cni/最佳实践/k8s-network-configuration-guide.md|Kubernetes 网络配置最佳实践]] — Cross-reference
- [[26-技能/08-可观测性/monitoring/monitor-kubernetes-metrics.md|Monitor Kubernetes Metrics]] — Cross-reference
- [[26-技能/04-工作负载/pod/培训/learn-02-pod-basics.md|第二课：Pod - K8s 的最小调度单元]] — Cross-reference
- [[26-技能/05-网络/service/培训/learn-04-service-basics.md|第四课：Service - 让应用可以被访问]] — Cross-reference
- [[26-技能/04-工作负载/pod/培训/learn-public-training.md|Kubernetes 培训：Public Training]] — Cross-reference
- [[26-技能/01-集群运维/gitops-argocd/诊断排障/ts-gitops-devops.md|GitOps/DevOps 排查]] — Cross-reference
- [[26-技能/04-工作负载/pod/培训/learn-04-debug-tools-setup.md|Day 4: 调试工具全家桶安装]] — Cross-reference
- [[26-技能/04-工作负载/pod/培训/learn-02-first-ticket-guide.md|Day 2: 第一个工单处理指南]] — Cross-reference
- [[26-技能/04-工作负载/pod/培训/learn-oncall-quick-qa.md|工单数字人快速问答 - On-Call 速查]] — Cross-reference
- [[26-技能/04-工作负载/pod/方法论/skill-MOC.md|topic-skills MOC]] — Cross-reference
- [[26-技能/04-工作负载/pod/方法论/skill-README.md|topic-skills — 工单智能体 Kubernetes 诊断 Skill 库]] — Cross-reference
- [[26-技能/04-工作负载/pod/培训/learn-12-common-problems.md|第十课：常见问题排查]] — Cross-reference
- [[26-技能/03-节点/skill-19-node-resource-pressure.md|节点资源压力诊断与修复 / Node Resource Pressure Diagnosis & Remediation]] — Cross-reference
- [[26-技能/06-存储/csi-storage/诊断排障/ts-storage.md|存储故障排查]] — Cross-reference
- [[26-技能/01-集群运维/cluster-upgrade/诊断排障/ts-cluster-operations.md|集群运维故障排查]] — Cross-reference
- [[26-技能/03-节点/node/skill-notready/skill-assets-escalation-template.md|Escalation Template]] — Cross-reference
- [[23-实体/15-参考与索引/cncf-cicd.md|CNCF CI/CD 与发布管理项目全景]] — Cross-reference
- [[23-实体/02-K8s核心组件/kube-apiserver.md|kube-apiserver]] — Cross-reference
- [[23-实体/07-可观测性/inspektor-gadget.md|Inspektor Gadget]] — Cross-reference
- [[23-实体/09-编排调度/metal3-io.md|Metal3]] — Cross-reference
- [[23-实体/15-参考与索引/core-deps-changelog.md|核心依赖变更日志索引]] — Cross-reference
- [[23-实体/15-参考与索引/cncf-edge-ai.md|CNCF 边缘计算与 AI/ML 项目全景]] — Cross-reference
- [[23-实体/02-K8s核心组件/container-runtime.md|Container Runtime]] — Cross-reference
- [[23-实体/09-编排调度/clusterpedia.md|Clusterpedia]] — Cross-reference
- [[23-实体/15-参考与索引/cncf-observability.md|CNCF 可观测性项目全景]] — Cross-reference
- [[21-生态参考/03-领域索引/etcd-index.md|etcd 知识图谱索引]]
- [[21-生态参考/03-领域索引/openkruise-index.md|OpenKruise 全局索引]]
- [[21-生态参考/03-领域索引/helm-index.md|Helm 全局索引]]
- [[21-生态参考/03-领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]
- [[21-生态参考/03-领域索引/csi-index.md|CSI (Container Storage Interface) 知识图谱索引]]
- CHANGELOG-1.2
- CHANGELOG-1.3


<!-- risk-assessed -->
