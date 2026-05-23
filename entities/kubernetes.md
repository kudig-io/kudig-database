---
title: Kubernetes (CNCF Graduated)
description: '## 概述'
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
created: "2026-05-23"
---

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

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[concepts/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]

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
- [[entities/prometheus-grafana.md|prometheus-grafana]]
- [[istio]]
- [[containerd]]
- [[entities/argocd.md|argocd]]

## Related

- [[entities/cncf-orchestration.md|cncf-orchestration]] — CNCF 编排与应用管理项目全景
- [[prometheus]] — Prometheus
- [[interlink]] — InterLink
- [[entities/kubernetes-changelog.md|kubernetes-changelog]] — Kubernetes 变更日志索引
- [[etcd]] — etcd

- [[domain-02-workloads-applications/06-java-cicd-tekton-argocd.md|06-java-cicd-tekton-argocd]]
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
- [[domain-01-cluster-fundamentals/99-kubernetes-production-architecture-blueprint.md|99-kubernetes-production-architecture-blueprint]]
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
- [[domain-17-system-foundation/README.md|Domain-33: Kubernetes Events 全域事件大全]]
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
- [[domain-02-workloads-applications/05-quarkus-native-kubernetes.md|05-quarkus-native-kubernetes]]
- [[domain-02-workloads-applications/02-spring-boot-kubernetes-production.md|02-spring-boot-kubernetes-production]]
- [[domain-02-workloads-applications/07-java-observability-kubernetes.md|07-java-observability-kubernetes]]
- [[domain-02-workloads-applications/03-jvm-gc-container-tuning.md|03-jvm-gc-container-tuning]]
- [[domain-02-workloads-applications/04-java-operator-sdk-development.md|04-java-operator-sdk-development]]
- [[domain-02-workloads-applications/README.md|Java on Kubernetes 综合实践指南]]
- domain-java-kubernetes MOC
- 16-kubernetes-hardware-troubleshooting
- [[domain-10-troubleshooting-diagnostics/topic-fta/kubernetes-fta-full-analysis-v2.md|kubernetes-fta-full-analysis-v2]]
- [[domain-10-troubleshooting-diagnostics/topic-fta/kubernetes-fta-full-analysis.md|kubernetes-fta-full-analysis]]
- vscode-kubernetes-tools
- [[journal/digest-2026-05-21-full|Wiki 全量知识库摘要 — 2026-05-21]] — Cross-reference
- [[_reports/WIKI-LINT-REPORT-2026-05-21|Wiki Lint Report — 2026-05-21]] — Cross-reference
- [[references/KUDIG Cheat Sheet Index|KUDIG Cheat Sheet Index]] — Cross-reference
- [[references/specialized-workloads-terms|K8s 专用工作负载术语参考]] — Cross-reference
- [[references/linux-sysctl-reference|Linux Sysctl Reference for Kubernetes]] — Cross-reference
- [[references/networking-terms|K8s 网络术语参考]] — Cross-reference
- [[references/k8s-workloads-domain-guide|Kubernetes Workloads Domain Guide]] — Cross-reference
- [[references/k8s-design-principles-deep-dive|设计原理：声明式 API、控制器模式与 etcd 共识]] — Cross-reference
- [[references/kubernetes-port-reference|Kubernetes Port Reference]] — Cross-reference
- [[references/workloads-terms|K8s 工作负载术语参考]] — Cross-reference
- [[references/k8s-glossary-index|K8s 术语表索引]] — Cross-reference
- [[references/fundamentals-terms|K8s 基础概念术语参考]] — Cross-reference
- [[references/release-notes-kubernetes|发布说明索引 — Kubernetes]] — Cross-reference
- [[references/k8s-architecture-fundamentals|K8s 架构基础与核心组件原理]] — Cross-reference
- [[references/root-terms|K8s Root术语参考]] — Cross-reference
- [[references/scheduling-terms|K8s 调度术语参考]] — Cross-reference
- [[references/kudig-contribution-guide|贡献指南、项目概览与版本发布说明]] — Cross-reference
- [[references/release-notes-reading-guide|发布说明阅读指南]] — Cross-reference
- [[references/k8s-advanced-ecosystem|硬件知识体系、CNCF 全景生态与 eBPF 平台工程]] — Cross-reference
- [[references/storage-terms|K8s 存储术语参考]] — Cross-reference
- [[references/observability-terms|K8s 可观测性术语参考]] — Cross-reference
- [[references/kubectl Scenario Quick Reference|kubectl Scenario Quick Reference]] — Cross-reference
- [[references/kubectl-quick-reference|Kubectl Quick Reference]] — Cross-reference
- [[references/k8s-deployment-create|Kubernetes Deployment 创建操作指南]] — Cross-reference
- [[references/k8s-knowledge-map|Kubernetes Knowledge Map]] — Cross-reference
- [[references/k8s-cluster-delete|Kubernetes 集群删除操作指南]] — Cross-reference
- [[references/release-notes-cli-tools|发布说明索引 — CLI 工具]] — Cross-reference
- [[references/KUDIG Frontmatter Spec|KUDIG Frontmatter Specification]] — Cross-reference
- [[references/k8s-cluster-create|Kubernetes 集群创建操作指南]] — Cross-reference
- [[references/configuration-terms|K8s 配置管理术语参考]] — Cross-reference
- [[references/release-notes-core-deps|发布说明索引 — 核心依赖]] — Cross-reference
- [[references/k8s-ai-infra-domain-guide|AI Infrastructure on Kubernetes Domain Guide]] — Cross-reference
- [[references/tooling-terms|K8s 工具链术语参考]] — Cross-reference
- [[references/k8s-cluster-cert|Kubernetes 集群证书管理操作指南]] — Cross-reference
- [[references/k8s-node-create|Kubernetes 节点管理操作指南]] — Cross-reference
- [[references/platform-engineering-terms|K8s 平台工程术语参考]] — Cross-reference
- [[references/multi-cloud-terms|K8s 多云架构术语参考]] — Cross-reference
- [[references/kudig-man-pages-index|KUDIG Man Pages Index]] — Cross-reference
- [[references/version-upgrade-guide|版本升级指南]] — Cross-reference
- [[references/k8s-difficulty-index|Kubernetes Difficulty Index]] — Cross-reference
- [[references/k8s-networking-domain-guide|Kubernetes Networking Domain Guide]] — Cross-reference
- [[references/operations-terms|K8s 运维运营术语参考]] — Cross-reference
- [[references/kubernetes-api-versions-reference|Kubernetes API Versions Reference]] — Cross-reference
- [[synthesis/kubeadm-cluster-operations|kubeadm 集群运维全景]] — Cross-reference
- [[synthesis/etcd x 高可用模式|etcd × 高可用模式]] — Cross-reference
- [[synthesis/K8s 问题分布与 MTTR 基准|K8s 问题分布与 MTTR 基准]] — Cross-reference
- [[synthesis/Structural Troubleshooting Framework|Structural Troubleshooting Framework]] — Cross-reference
- [[synthesis/声明式 API × 控制器模式|声明式 API × 控制器模式]] — Cross-reference
- [[synthesis/eBPF x 运行时安全|eBPF x 运行时安全]] — Cross-reference
- [[concepts/deployment-controller-architecture|Deployment 控制器架构]] — Cross-reference
- [[concepts/bp-common-best-practices|Kubernetes 通用最佳实践参考]] — Cross-reference
- [[concepts/kubernetes-pki-certificate-system|Kubernetes PKI 证书体系]] — Cross-reference
- [[concepts/bp-infrastructure|最佳实践：Infrastructure]] — Cross-reference
- [[concepts/bp-observability|最佳实践：Observability]] — Cross-reference
- [[concepts/bp-operations|最佳实践：Operations]] — Cross-reference
- [[concepts/declarative-api|Declarative API]] — Cross-reference
- [[concepts/core-dependency-version-matrix|核心依赖版本矩阵]] — Cross-reference
- [[concepts/kubernetes-version-evolution|Kubernetes 版本演进]] — Cross-reference
- [[concepts/multi-tenancy-isolation|Multi-Tenancy Isolation]] — Cross-reference
- [[concepts/cli-tools-evolution|CLI 工具演进]] — Cross-reference
- [[concepts/etcd Operational Reference|etcd Operational Reference]] — Cross-reference
- [[concepts/ai-agent-openclaw-workspace|OpenClaw 工作空间配置]] — Cross-reference
- [[concepts/ai-agent-README|AI Agent 工程专题]] — Cross-reference
- [[concepts/cni-networking-model|CNI 网络模型与插件对比]] — Cross-reference
- [[concepts/linux-sysctl-tuning|Linux Sysctl Tuning for Kubernetes]] — Cross-reference
- [[concepts/storage-tool-evolution|存储工具演进]] — Cross-reference
- [[concepts/KUDIG Knowledge Base Architecture|KUDIG Knowledge Base Architecture]] — Cross-reference
- [[concepts/bp-README|Kubernetes 最佳实践指南]] — Cross-reference
- [[concepts/eventual-consistency|Eventual Consistency in Kubernetes]] — Cross-reference
- [[concepts/k8s-production-best-practices|Kubernetes 生产环境最佳实践]] — Cross-reference
- [[concepts/node-lifecycle-management|节点生命周期管理]] — Cross-reference
- [[concepts/production-operations-best-practices|Production Operations Best Practices]] — Cross-reference
- [[concepts/bp-security|最佳实践：Security]] — Cross-reference
- [[concepts/observability-stack-evolution|可观测性栈演进]] — Cross-reference
- [[concepts/security-tool-evolution|安全工具演进]] — Cross-reference
- [[concepts/watch-mechanism|Watch Mechanism (List-Watch)]] — Cross-reference
- [[concepts/gitops-tool-evolution|GitOps 工具演进]] — Cross-reference
- [[concepts/linux-security-modules|Linux Security Modules for Containers]] — Cross-reference
- [[skills/learn-05-ingress-basics|第五课：Ingress - 外部 HTTP/HTTPS 访问]] — Cross-reference
- [[skills/learn-01-day-one-checklist|Day 1: 新人首日检查清单]] — Cross-reference
- [[skills/k8s-storage-configuration-guide|Kubernetes 存储配置最佳实践]] — Cross-reference
- [[skills/k8s-scaling-guide|Kubernetes 扩缩容最佳实践]] — Cross-reference
- [[skills/k8s-disaster-recovery-guide|Kubernetes 灾难恢复最佳实践]] — Cross-reference
- [[skills/ts-ai-ml-workloads|AI/ML 工作负载排查]] — Cross-reference
- [[skills/dns-fta|DNS 异常故障树分析]] — Cross-reference
- [[skills/learn-decision-tree-mermaid|故障排查决策树 - Mermaid 可视化版]] — Cross-reference
- [[skills/node-fta|Node 异常故障树分析]] — Cross-reference
- [[skills/kubelet-certificate-rotation|kubelet 证书轮换机制]] — Cross-reference
- [[skills/learn-README|新人上手快速路径（Quick Start）]] — Cross-reference
- [[skills/assessment-k8s-fundamentals-quiz-answers|K8S Fundamentals Quiz Answers]] — Cross-reference
- [[skills/k8s-network-security-guide|Kubernetes 网络安全最佳实践]] — Cross-reference
- [[skills/ts-node-components|节点组件故障排查]] — Cross-reference
- [[skills/learn-13-daemonset-basics|第13课：DaemonSet 与节点守护]] — Cross-reference
- [[skills/kubeadm-cluster-lifecycle|kubeadm 集群创建生命周期]] — Cross-reference
- [[skills/k8s-logging-management-guide|Kubernetes 日志管理最佳实践]] — Cross-reference
- [[skills/skill-20-networkpolicy-connectivity|NetworkPolicy 连通性故障诊断 / NetworkPolicy Connectivity Troubleshooting]] — Cross-reference
- [[skills/assessment-troubleshooting-lab-exam|Troubleshooting Lab Exam]] — Cross-reference
- [[skills/k8s-monitoring-guide|Kubernetes 监控最佳实践]] — Cross-reference
- [[skills/deployment-canary-and-bluegreen|金丝雀与蓝绿发布]] — Cross-reference
- [[skills/skill-k8s-node-notready-USAGE-GUIDE|Usage Guide]] — Cross-reference
- [[skills/learn-01-what-is-kubernetes|第一课：Kubernetes 入门]] — Cross-reference
- [[skills/ts-security-auth|安全认证故障排查]] — Cross-reference
- [[skills/skill-reference-version-matrix|Version Matrix]] — Cross-reference
- [[skills/develop-crd-operator|Develop CRD Operator]] — Cross-reference
- [[skills/skill-23-job-cronjob-failure|Job/CronJob 故障诊断与修复 / Job & CronJob Failure Diagnosis & Remediation]] — Cross-reference
- [[skills/node-drain-and-maintenance|节点驱逐与维护]] — Cross-reference
- [[skills/k8s-distributed-tracing-guide|Kubernetes 分布式追踪最佳实践]] — Cross-reference
- [[skills/skill-21-statefulset-failure|StatefulSet 故障诊断与修复 / StatefulSet Failure Diagnosis & Remediation]] — Cross-reference
- [[skills/Kubernetes Diagnostic Skills Overview|Kubernetes Diagnostic Skills Overview]] — Cross-reference
- [[skills/kubeadm-cluster-deletion|kubeadm 集群删除操作]] — Cross-reference
- [[skills/kubeadm-ha-cluster-setup|kubeadm 高可用集群搭建]] — Cross-reference
- [[skills/k8s-deployment-strategies-guide|Kubernetes 部署策略最佳实践]] — Cross-reference
- [[skills/k8s-cluster-configuration-guide|Kubernetes 集群配置最佳实践]] — Cross-reference
- [[skills/skill-reference-diagnostic-workflow|Diagnostic Workflow]] — Cross-reference
- [[skills/skill-22-daemonset-failure|DaemonSet 故障诊断与修复 / DaemonSet Failure Diagnosis & Remediation]] — Cross-reference
- [[skills/ts-control-plane|控制平面故障排查]] — Cross-reference
- [[skills/skill-reference-remediation-playbook|Remediation Playbook]] — Cross-reference
- [[skills/learn-lecturer-persona|K8S 讲师角色设定与场景规范]] — Cross-reference
- [[skills/learn-inner-training|Kubernetes 培训：Inner Training]] — Cross-reference
- [[skills/learn-15-scheduling-basics|第15课：调度与亲和性]] — Cross-reference
- [[skills/assessment-daily-check-quiz|Daily Check Quiz]] — Cross-reference
- [[skills/skill-reference-root-cause-catalog|Root Cause Catalog]] — Cross-reference
- [[skills/learn-root|Kubernetes 培训：Root]] — Cross-reference
- [[skills/skills-run-README|Skills Demo — 本地运行工单诊断技能]] — Cross-reference
- [[skills/deployment-workload-selection|工作负载控制器选型]] — Cross-reference
- [[skills/k8s-network-configuration-guide|Kubernetes 网络配置最佳实践]] — Cross-reference
- [[skills/monitor-kubernetes-metrics|Monitor Kubernetes Metrics]] — Cross-reference
- [[skills/learn-02-pod-basics|第二课：Pod - K8s 的最小调度单元]] — Cross-reference
- [[skills/learn-04-service-basics|第四课：Service - 让应用可以被访问]] — Cross-reference
- [[skills/learn-public-training|Kubernetes 培训：Public Training]] — Cross-reference
- [[skills/ts-gitops-devops|GitOps/DevOps 排查]] — Cross-reference
- [[skills/learn-04-debug-tools-setup|Day 4: 调试工具全家桶安装]] — Cross-reference
- [[skills/learn-02-first-ticket-guide|Day 2: 第一个工单处理指南]] — Cross-reference
- [[skills/learn-oncall-quick-qa|工单数字人快速问答 - On-Call 速查]] — Cross-reference
- [[skills/skill-MOC|topic-skills MOC]] — Cross-reference
- [[skills/skill-README|topic-skills — 工单智能体 Kubernetes 诊断 Skill 库]] — Cross-reference
- [[skills/learn-12-common-problems|第十课：常见问题排查]] — Cross-reference
- [[skills/skill-19-node-resource-pressure|节点资源压力诊断与修复 / Node Resource Pressure Diagnosis & Remediation]] — Cross-reference
- [[skills/ts-storage|存储故障排查]] — Cross-reference
- [[skills/ts-cluster-operations|集群运维故障排查]] — Cross-reference
- [[skills/skill-assets-escalation-template|Escalation Template]] — Cross-reference
- [[entities/cncf-cicd|CNCF CI/CD 与发布管理项目全景]] — Cross-reference
- [[entities/kube-apiserver|kube-apiserver]] — Cross-reference
- [[entities/inspektor-gadget|Inspektor Gadget]] — Cross-reference
- [[entities/metal3-io|Metal3]] — Cross-reference
- [[entities/core-deps-changelog|核心依赖变更日志索引]] — Cross-reference
- [[entities/cncf-edge-ai|CNCF 边缘计算与 AI/ML 项目全景]] — Cross-reference
- [[entities/container-runtime|Container Runtime]] — Cross-reference
- [[entities/clusterpedia|Clusterpedia]] — Cross-reference
- [[entities/cncf-observability|CNCF 可观测性项目全景]] — Cross-reference
- [[domain-19-landscape-references/topic-index/etcd-index|etcd 知识图谱索引]]
- [[domain-19-landscape-references/topic-index/openkruise-index|OpenKruise 全局索引]]
- [[domain-19-landscape-references/topic-index/helm-index|Helm 全局索引]]
- [[domain-19-landscape-references/topic-index/gitops-cicd-index|GitOps / CI-CD 全局索引]]
- [[domain-19-landscape-references/topic-index/csi-index|CSI (Container Storage Interface) 知识图谱索引]]
- CHANGELOG-1.2
- CHANGELOG-1.3
