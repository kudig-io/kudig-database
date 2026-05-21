---
title: Wiki Index
description: '- [[concepts/k8s-production-best-practices.md|Kubernetes 生产环境最佳实践]]'
category: general
tags:
- k8s
- etcd
- apiserver
- kubelet
- scheduler
- prometheus
- grafana
- istio
- cilium
- flux
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 15min
intent_queries:
- Wiki Index 是什么
- 如何 Wiki Index
trigger_keywords:
- Wiki
- Index
prerequisites:
- kubectl-basics
- service-mesh-basics
- prometheus-basics
- monitoring-basics
- ebpf-basics
- cilium-basics
- etcd-basics
- gpu-scheduling-basics
- policy-basics
---

# Wiki Index

*This index is automatically maintained. Last updated: 2026-05-21*

## Concepts

Kubernetes core concepts, architecture patterns, and operational knowledge.

- [[MOC|Global Knowledge Domain Navigation]]
- [[index|KUDIG Knowledge Base Home]]
- [[concepts/KUDIG Knowledge Base Architecture.md|KUDIG Knowledge Base Architecture]]
- [[concepts/Kubernetes Core Concepts.md|Kubernetes Core Concepts]]
- [[concepts/kubernetes-architecture-overview.md|Kubernetes Architecture Overview]]
- [[concepts/controller-pattern.md|Controller Pattern (Reconciliation Loop)]]
- [[concepts/declarative-api.md|Declarative API]]
- [[concepts/watch-mechanism.md|Watch Mechanism (List-Watch)]]
- [[concepts/eventual-consistency.md|Eventual Consistency in Kubernetes]]
- [[pod-lifecycle|Pod Lifecycle]]
- [[concepts/scheduling-algorithm.md|Scheduling Algorithm (Filter+Score)]]
- [[concepts/service-networking.md|Service Networking]]
- [[concepts/storage-model.md|Persistent Storage Model (PV/PVC/StorageClass)]]
- [[concepts/security-defense-depth.md|Defense-in-Depth Security]]
- [[concepts/observability-pillars.md|Observability Pillars (Metrics/Logs/Traces)]]
- [[concepts/autoscaling-strategies.md|Autoscaling Strategies (HPA/VPA/Karpenter)]]
- [[operator-pattern|Operator Pattern (CRD + Controller)]]
- [[concepts/high-availability-patterns.md|High Availability Patterns]]
- [[concepts/multi-tenancy-isolation.md|Multi-Tenancy Isolation]]
- [[concepts/resource-management.md|Resource Management (Requests/Limits/QoS)]]
- [[concepts/docker-architecture.md|Docker Architecture]]
- [[concepts/linux-container-foundation.md|Linux Container Foundation]]
- [[concepts/linux-security-modules.md|Linux Security Modules (SELinux/AppArmor)]]
- [[concepts/gitops-principles.md|GitOps Principles]]
- [[concepts/infrastructure-as-code.md|Infrastructure as Code]]
- [[concepts/service-mesh-architecture.md|Service Mesh Architecture]]
- [[concepts/cilium-ebpf-networking.md|Cilium eBPF Networking]]
- [[concepts/microservice-resilience-patterns.md|Microservice Resilience Patterns]]
- [[supply-chain-security|Supply Chain Security]]
- [[concepts/secrets-management.md|Secrets Management]]
- [[concepts/block-file-object-storage.md|Block vs File vs Object Storage]]
- [[concepts/overlayfs-storage.md|OverlayFS Storage]]
- [[concepts/ci-cd-pipeline-patterns.md|CI/CD Pipeline Patterns]]
- [[concepts/platform-engineering-idp.md|Platform Engineering (IDP)]]
- [[concepts/etcd Operational Reference.md|etcd Operational Reference]]
- [[concepts/linux-sysctl-tuning.md|Linux sysctl Tuning]]
- [[concepts/cloud-native-defense-in-depth.md|Cloud-Native Defense in Depth]]
- [[concepts/container-runtime-comparison.md|Container Runtime Comparison]]
- [[concepts/Symptom-SOP-RootCause Mapping.md|Symptom-SOP-RootCause Mapping]]
- [[concepts/k8s-production-best-practices.md|Kubernetes 生产环境最佳实践]]
- [[concepts/kubernetes-version-evolution.md|Kubernetes 版本演进]]
- [[concepts/core-dependency-version-matrix.md|核心依赖版本矩阵]]
- [[concepts/gitops-tool-evolution.md|GitOps 工具演进]]
- [[concepts/service-mesh-evolution.md|服务网格演进]]
- [[concepts/observability-stack-evolution.md|可观测性栈演进]]
- [[concepts/security-tool-evolution.md|安全工具演进]]
- [[concepts/storage-tool-evolution.md|存储工具演进]]
- [[concepts/node-lifecycle-management.md|节点生命周期管理]]
- [[concepts/kubernetes-pki-certificate-system.md|Kubernetes PKI 证书体系]]
- [[concepts/cni-networking-model.md|CNI 网络模型]]
- [[concepts/deployment-controller-architecture.md|Deployment 控制器架构]]
- [[concepts/cli-tools-evolution.md|CLI 工具演进]]

## Entities

K8s resource types, components, and tools.

- [[docs/API-DOC-MAP.md|API Resource Mapping]]
- [[docs/COMMAND-DOC-MAP.md|Command Mapping]]
- [[docs/ERROR-FTA-MAP.md|Error Code FTA Mapping]]
- [[etcd|etcd]]
- [[entities/kube-apiserver.md|kube-apiserver]]
- [[entities/kube-scheduler.md|kube-scheduler]]
- [[entities/kubelet.md|kubelet]]
- [[deployment|Deployment]]
- [[entities/statefulset.md|StatefulSet]]
- [[entities/cni-plugins.md|CNI Plugins]]
- [[entities/networkpolicy.md|NetworkPolicy]]
- [[entities/crd-custom-resources.md|CRD (Custom Resource Definition)]]
- [[entities/container-runtime.md|Container Runtime]]
- [[entities/csi-drivers.md|CSI Drivers]]
- [[docker|Docker]]
- [[containerd|containerd]]
- [[istio|Istio]]
- [[cilium|Cilium]]
- [[entities/prometheus-grafana.md|Prometheus + Grafana]]
- [[falco|Falco]]
- [[entities/tetragon.md|Tetragon]]
- [[entities/trivy.md|Trivy]]
- [[entities/vault.md|HashiCorp Vault]]
- [[kyverno|Kyverno]]
- [[flux|Flux]]
- [[crossplane|Crossplane]]
- [[entities/kubernetes-changelog.md|Kubernetes 变更日志索引]]
- [[entities/core-deps-changelog.md|核心依赖变更日志索引]]
- [[entities/ecosystem-changelog.md|生态组件变更日志索引]]

## Skills

Operational skills and troubleshooting handbooks.

- [[MOC|Skills Navigation]]
- [[MOC|Cheat Sheet Navigation]]
- [[skills/configure-health-probes.md|Configure Health Probes]]
- [[skills/troubleshoot-pod-issues.md|Troubleshoot Pod Issues]]
- [[skills/troubleshoot-node-issues.md|Troubleshoot Node Issues]]
- [[skills/backup-restore-etcd.md|Backup and Restore etcd]]
- [[skills/monitor-kubernetes-metrics.md|Monitor Kubernetes Metrics]]
- [[skills/manage-persistent-storage.md|Manage Persistent Storage]]
- [[skills/audit-rbac-configurations.md|Audit RBAC Configurations]]
- [[skills/develop-crd-operator.md|Develop CRD Operator]]
- [[skills/FTA Methodology and Core Principles.md|FTA Methodology and Core Principles]]
- [[skills/FTA Diagnostic Execution Engine.md|FTA Diagnostic Execution Engine]]
- [[skills/FTA-Driven Runbook Automation.md|FTA-Driven Runbook Automation]]
- [[skills/Kubernetes FTA Top Events Index.md|Kubernetes FTA Top Events Index]]
- [[skills/Kubernetes Diagnostic Skills Overview.md|Kubernetes Diagnostic Skills Overview]]
- [[skills/Symptom Vector Matching Engine.md|Symptom Vector Matching Engine]]
- [[skills/Agent Orchestration Patterns.md|Agent Orchestration Patterns]]
- [[skills/kubeadm-cluster-lifecycle.md|kubeadm 集群创建生命周期]]
- [[skills/kubeadm-cluster-deletion.md|kubeadm 集群删除操作]]
- [[skills/kubeadm-ha-cluster-setup.md|kubeadm 高可用集群搭建]]
- [[skills/kubelet-certificate-rotation.md|kubelet 证书轮换机制]]
- [[skills/node-drain-and-maintenance.md|节点驱逐与维护]]
- [[skills/kubelet-eviction-mechanism.md|kubelet 资源驱逐机制]]
- [[skills/deployment-rolling-update.md|Deployment 滚动更新策略]]
- [[skills/deployment-canary-and-bluegreen.md|金丝雀与蓝绿发布]]
- [[skills/deployment-workload-selection.md|工作负载控制器选型]]

### K8S 最佳实践 (best-practices-*)

Kubernetes 生产环境最佳实践操作指南。

- [[skills/k8s-cluster-configuration-guide.md|Kubernetes 集群配置最佳实践]]
- [[skills/k8s-network-configuration-guide.md|Kubernetes 网络配置最佳实践]]
- [[skills/k8s-storage-configuration-guide.md|Kubernetes 存储配置最佳实践]]
- [[skills/k8s-monitoring-guide.md|Kubernetes 监控最佳实践]]
- [[skills/k8s-logging-management-guide.md|Kubernetes 日志管理最佳实践]]
- [[skills/k8s-distributed-tracing-guide.md|Kubernetes 分布式追踪最佳实践]]
- [[skills/k8s-deployment-strategies-guide.md|Kubernetes 部署策略最佳实践]]
- [[skills/k8s-scaling-guide.md|Kubernetes 扩缩容最佳实践]]
- [[skills/k8s-disaster-recovery-guide.md|Kubernetes 灾难恢复最佳实践]]
- [[skills/k8s-network-security-guide.md|Kubernetes 网络安全最佳实践]]
- [[skills/k8s-pod-security-guide.md|Kubernetes Pod 安全最佳实践]]

### K8S 培训体系 (learn-*)

Kubernetes 新人培训、On-Call 速查、工单答疑体系。

- [[skills/learn-root.md|K8S 培训体系导航]]
- [[skills/learn-README.md|新人上手快速路径（4 天计划）]]
- [[skills/learn-01-what-is-kubernetes.md|第 1 课: 什么是 Kubernetes]]
- [[skills/learn-02-pod-basics.md|第 2 课: Pod 基础]]
- [[skills/learn-03-deployment-basics.md|第 3 课: Deployment 基础]]
- [[skills/learn-04-service-basics.md|第 4 课: Service 基础]]
- [[skills/learn-05-ingress-basics.md|第 5 课: Ingress 基础]]
- [[skills/learn-06-configmap-secret.md|第 6 课: ConfigMap 与 Secret]]
- [[skills/learn-07-namespace-resource-quota.md|第 7 课: Namespace 与资源配额]]
- [[skills/learn-08-pv-pvc-basics.md|第 8 课: PV/PVC 持久存储]]
- [[skills/learn-09-hpa-basics.md|第 9 课: HPA 自动伸缩]]
- [[skills/learn-10-health-check.md|第 10 课: 健康检查探针]]
- [[skills/learn-11-job-cronjob.md|第 11 课: Job/CronJob 任务调度]]
- [[skills/learn-12-common-problems.md|第 12 课: 常见问题排查]]
- [[skills/learn-13-daemonset-basics.md|第 13 课: DaemonSet 基础]]
- [[skills/learn-14-statefulset-basics.md|第 14 课: StatefulSet 基础]]
- [[skills/learn-15-scheduling-basics.md|第 15 课: 调度与亲和性]]
- [[skills/learn-01-day-one-checklist.md|新人 Day 1 检查清单]]
- [[skills/learn-02-first-ticket-guide.md|新人首个工单处理指南]]
- [[skills/learn-03-oncall-handoff.md|On-Call 交接 SOP]]
- [[skills/learn-04-debug-tools-setup.md|调试工具安装与配置]]
- [[skills/learn-lecturer-persona.md|K8S 讲师角色设定（数字人教练）]]
- [[skills/learn-analogy-dictionary.md|K8S 概念类比词典]]
- [[skills/learn-oncall-quick-qa.md|工单数字人快速问答（20 场景）]]
- [[skills/learn-decision-tree-mermaid.md|故障排查决策树（10 场景）]]
- [[skills/learn-public-training.md|K8S 运维实战培训（28 天体系）]]
- [[skills/learn-inner-training.md|ACK/ACR 内部培训（28 天体系）]]

## References

Reference documents, specifications, and domain guides.

- [[docs/TAG-DICTIONARY.md|Tag Dictionary]]
- [[docs/SYNONYM-DICTIONARY.md|Synonym Dictionary]]
- [[docs/FRONTMATTER-SPEC.md|Frontmatter Specification]]
- [[docs/SCENARIO-TAXONOMY.md|Scenario Taxonomy]]
- [[references/k8s-architecture-domain-guide.md|Kubernetes Architecture Domain Guide]]
- [[references/k8s-workloads-domain-guide.md|Kubernetes Workloads Domain Guide]]
- [[references/k8s-networking-domain-guide.md|Kubernetes Networking Domain Guide]]
- [[references/k8s-ai-infra-domain-guide.md|AI Infrastructure on Kubernetes Domain Guide]]
- [[references/kubectl Scenario Quick Reference.md|kubectl Scenario Quick Reference]]
- [[references/KUDIG Tag Dictionary.md|KUDIG Tag Dictionary]]
- [[references/KUDIG Scenario Taxonomy.md|KUDIG Scenario Taxonomy]]
- [[references/KUDIG Frontmatter Spec.md|KUDIG Frontmatter Spec]]
- [[references/kudig-man-pages-index.md|KUDIG Man Pages Index]]
- [[references/KUDIG Cheat Sheet Index.md|KUDIG Cheat Sheet Index]]
- KUDIG Templates and Agent Prompts
- [[references/k8s-knowledge-map.md|Kubernetes Knowledge Map]]
- [[references/k8s-difficulty-index.md|Kubernetes Difficulty Index]]
- [[kudig-prompts-catalog|KUDIG Prompts Catalog]]
- [[references/version-upgrade-guide.md|版本升级指南]]
- [[references/release-notes-reading-guide.md|发布说明阅读指南]]

## Synthesis

Comprehensive analysis and cross-cutting assessment.

- [[_reports/OBSIDIAN-WIKI-AGENT-CORPUS-IMPROVEMENT-PLAN.md|Obsidian Wiki Improvement Plan]]
- [[_reports/FULL-FIX-PROGRESS-2026-05-19.md|Full Fix Progress Overview]]
- [[synthesis/Kubernetes Fault [[entities/distribution.md|distribution]] and MTTR.md|Kubernetes Fault Distribution and MTTR]]
- [[synthesis/Production Troubleshooting Playbook.md|Production Troubleshooting Playbook]]
- [[synthesis/Structural Troubleshooting Framework.md|Structural Troubleshooting Framework]]
- [[synthesis/eBPF x 运行时安全.md|eBPF x 运行时安全]]
- [[synthesis/GitOps x 平台工程.md|GitOps x 平台工程]]
- [[synthesis/纵深防御 x 供应链安全.md|纵深防御 x 供应链安全]]
- [[synthesis/服务网格 x 零信任安全.md|服务网格 x 零信任安全]]
- [[synthesis/IaC x 多集群管理.md|IaC x 多集群管理]]
- [[synthesis/kubeadm-cluster-operations.md|kubeadm 集群运维全景]]
- [[synthesis/Operator 模式 × 可观测性|Operator 模式 × 可观测性]]
- [[synthesis/Deployment × Secret 管理|Deployment × Secret 管理]]
- [[synthesis/Pod 生命周期 × 存储模型|Pod 生命周期 × 存储模型]]
- [[synthesis/CNI 插件 × NetworkPolicy|CNI 插件 × NetworkPolicy]]
- [[synthesis/etcd × 可观测性|etcd × 可观测性]]

- [[synthesis/控制器模式 × Deployment|控制器模式 × Deployment]]
- [[synthesis/CRD × 可观测性|CRD × 可观测性]]
- [[synthesis/Pod 生命周期 × Secret 管理|Pod 生命周期 × Secret 管理]]
- [[synthesis/Operator 模式 × Pod 生命周期|Operator 模式 × Pod 生命周期]]
- [[synthesis/控制器模式 × 可观测性|控制器模式 × 可观测性]]
- [[synthesis/Secret 管理 × 存储模型|Secret 管理 × 存储模型]]
- [[synthesis/Cilium eBPF × 可观测性|Cilium eBPF × 可观测性]]
- [[synthesis/可观测性支柱 × Prometheus-Grafana|可观测性支柱 × Prometheus-Grafana]]
- [[synthesis/CI-CD 流水线 × Secret 管理|CI-CD 流水线 × Secret 管理]]
- [[synthesis/etcd × Operator 模式|etcd × Operator 模式]]
## Journal

Update logs and activity records.

- [[log.md|Wiki Log]]
- [[CHANGELOG.md|Changelog]]

## Projects

Project-organized knowledge.

- Production Scenario Navigation
- [[MOC|Learning Paths]]
- [[kudig-templates-catalog|KUDIG Templates Catalog]]

## Related

- [[concepts/multi-tenancy-isolation.md|multi-tenancy-isolation]] — Multi-Tenancy Isolation
- [[concepts/security-defense-depth.md|security-defense-depth]] — Defense-in-Depth Security
- [[concepts/platform-engineering-idp.md|platform-engineering-idp]] — Platform Engineering and Internal Developer Platforms
- [[concepts/kubernetes-pki-certificate-system.md|kubernetes-pki-certificate-system]] — Kubernetes PKI 证书体系
- [[concepts/service-mesh-architecture.md|service-mesh-architecture]] — Service Mesh Architecture

## Domains

生产环境维度整合后的 20 个核心 Domain。

### Tier 1 — 核心技术域

- [[domain-01-cluster-fundamentals/README|Cluster Fundamentals]] — 架构基础、设计原则、控制平面 ( #k8s #architecture #fundamentals)
- [[domain-02-workloads-applications/README|Workloads & Applications]] — 工作负载、Java on K8s、应用模式 ( #workloads #applications)
- [[domain-03-networking-traffic/README|Networking & Traffic]] — K8s 网络、Service Mesh、API Gateway、eBPF ( #networking #service-mesh #cilium)
- [[domain-04-storage-data/README|Storage & Data]] — PV/PVC/CSI、存储基础、分布式存储 ( #storage #csi)
- [[domain-05-security-compliance/README|Security & Compliance]] — 认证授权、运行时安全、策略治理、供应链安全 ( #security #compliance #supply-chain)
- [[domain-06-observability/README|Observability]] — 指标、日志、链路追踪、告警、SLO/SLI ( #observability #monitoring #logging #tracing)

### Tier 2 — 平台与工程域

- [[domain-07-platform-engineering/README|Platform Engineering]] — 平台构建(IDP/Backstage)、平台运维、治理、DevEx ( #platform-engineering #idp)
- [[domain-08-release-change-management/README|Release & Change Management]] — GitOps、IaC、变更管理、测试质量 ( #gitops #iac #cicd)
- [[domain-09-reliability-engineering/README|Reliability Engineering]] — SLO/SLI 体系、混沌工程、灾备演练、事后复盘、性能测试 ( #sre #slo #chaos-engineering #disaster-recovery #postmortem)

### Tier 3 — 运维场景域

- [[domain-10-troubleshooting-diagnostics/README|Troubleshooting & Diagnostics]] — 全链路排障、FTA、结构化诊断 ( #troubleshooting #diagnostics #fta)
- [[domain-11-production-operations/README|Production Operations]] — FinOps、治理、事件响应、绿色计算 ( #finops #incident-response)

### Tier 4 — 部署与生态域

- [[domain-12-cloud-providers/README|Cloud Providers]] — 多云厂商、混合云部署 ( #cloud #aws #gcp #azure)
- [[domain-13-container-runtime/README|Container Runtime]] — Docker、镜像管理、供应链安全 ( #docker #container #image)
- [[domain-14-ai-ml-infra/README|AI/ML Infrastructure]] — AI 基础设施、AI Agent ( #ai #ml #gpu)
- [[domain-15-specialized-tech/README|Specialized Technologies]] — 边缘计算、WebAssembly、Extensions ( #edge #wasm)
- [[domain-16-database-middleware/README|Database & Middleware]] — 数据库、消息队列、时序数据库、Operator 管理、数据流处理 ( #database #middleware #message-queue #streaming)

### Tier 5 — 基础与参考域

- [[domain-90-system-foundation/README|System Foundation]] — Linux、硬件、K8s Events ( #linux #hardware)
- [[domain-91-manifests-patterns/README|Manifests & Patterns]] — YAML 参考、资源清单 ( #yaml #manifests)
- [[domain-92-landscape-references/README|Landscape & References]] — CNCF 全景、论文索引 ( #cncf #papers)
- [[domain-93-application-patterns/README|Application Patterns]] — 业务架构参考 ( #application-architecture)

## Cross-Domain Synthesis

跨域综合分析，连接多个知识域的交叉主题。

- [[synthesis/slo-monitoring-integration|SLO 与监控系统的深度集成]] — SLO/SLI 与 Prometheus/Grafana 的端到端集成
- [[synthesis/gitops-release-gate|GitOps 与发布门控的协同]] — 声明式交付与 SLO 发布门控的完整流水线
- [[synthesis/chaos-drill-integration|混沌工程与灾备演练的结合]] — 从日常混沌验证到季度 GameDay 的渐进式体系
- [[synthesis/multi-cluster-security|多集群环境下的安全架构]] — 零信任网络、联邦策略、跨集群 mTLS
- [[synthesis/observability-finops|可观测性与 FinOps 的融合]] — 资源利用率驱动的成本优化
- [[synthesis/ai-ml-observability|AI/ML 工作负载的可观测性]] — GPU 监控、训练追踪、推理服务 SLO
- [[synthesis/platform-engineering-sre|平台工程与 SRE 的协作模式]] — IDP 构建与可靠性保障的协同
- [[synthesis/edge-cloud-continuum|边缘-云连续体的运维架构]] — KubeEdge 云边协同与边缘自治
- [[synthesis/backstage-platform-catalog|Backstage 与平台目录的整合]] — 统一服务目录与自服务入口
- [[synthesis/data-protection-k8s|Kubernetes 数据保护策略]] — Velero、CSI 快照、勒索软件防护
- [[synthesis/security-observability-correlation|安全事件与可观测性的关联分析]] — Falco + Prometheus 联合检测
- [[synthesis/cost-optimization-multi-cluster|多集群成本优化策略]] — Spot 实例、跨集群调度、闲置清理
- [[synthesis/service-mesh-security-governance|服务网格与安全治理的融合]] — Istio L7 授权与 OPA 集成
- [[synthesis/ai-agent-ops-patterns|AI Agent 运维模式]] — 推理服务部署、模型版本管理、成本优化
- [[synthesis/cross-cloud-migration-playbook|跨云迁移手册]] — EKS→GKE/AKS 的迁移策略与工具链

*Domain 结构更新于 2026-05-21。原 43 个 Domain 已整合为 20 个。详见 `_reports/domain-migration-EXECUTED-2026-05-21.md`。*
