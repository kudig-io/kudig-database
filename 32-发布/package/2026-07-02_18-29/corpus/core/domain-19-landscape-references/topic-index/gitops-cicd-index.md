---
title: GitOps / CI-CD 全局索引
description: '## 架构基础'
summary: '## 架构基础'
category: index
tags:
- k8s
- index
- catalog
- apiserver
- kubelet
- scheduler
- controller-manager
- prometheus
- jaeger
- istio
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 25min
intent_queries:
- GitOps / CI-CD 全局索引 是什么
- 如何 GitOps / CI-CD 全局索引
trigger_keywords:
- GitOps
- CI-CD
- 全局索引
- index
prerequisites:
- kubectl-basics
- cncf-ecosystem
- helm-basics
- service-mesh-basics
- prometheus-basics
- gitops-basics
- ebpf-basics
- cilium-basics
- gpu-scheduling-basics
- tls-basics
- policy-basics
- logging-basics
- tracing-basics
- observability-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# GitOps / CI-CD 全局索引

> 全局索引：按关键字 **gitops-cicd** 聚合项目内所有相关内容。

## 架构基础

- [[concepts/kubernetes-architecture-overview.md|Kubernetes 架构全景图 (Architecture Overview)]]
- [[32-发布/package/2026-07-02_18-29/corpus/core/domain-01-cluster-fundamentals/01-architecture-overview/01-core-components-deep-dive|02 core components deep dive]]
- [[32-发布/package/2026-07-02_18-29/corpus/core/domain-01-cluster-fundamentals/04-kubectl/01-kubectl-commands-reference|05 kubectl commands reference]]
- 07 - 升级路径与策略指南
- 08 - 多租户架构设计 (Multi-Tenancy Architecture)
- 12 - Kubernetes 集群部署架构模式指南
- 14 - Kubernetes 安全架构深度分析
- 15 - Kubernetes 可观测性架构体系
- 17 - 生产环境运维最佳实践 (Production Operations Best Practices)
- 18 - Kubernetes 升级和迁移策略指南
- Kubectl v1.29 - v1.33 新命令与用法速查

## 控制平面

- GitOps自动化运维实践 (GitOps Automation Operations Practice)

## 平台运维

- GitOps配置管理 (GitOps Configuration Management)

## 扩展生态

- 47 - Helm Chart开发与管理
- 129 - Helm 高级运维：复杂部署、CI/CD 集成与安全最佳实践
- 21 - CI/CD管道表
- 48 - GitOps工作流

## 结构化故障排查 - 控制平面

- [[domain-10-troubleshooting-diagnostics/高级排障/01-control-plane/01-apiserver-troubleshooting.md|API Server 故障排查指南]]
- [[domain-10-troubleshooting-diagnostics/高级排障/01-control-plane/03-scheduler-troubleshooting.md|Scheduler 故障排查指南]]
- [[domain-10-troubleshooting-diagnostics/高级排障/01-control-plane/04-controller-manager-troubleshooting.md|Controller Manager 故障排查指南]]
- [[domain-10-troubleshooting-diagnostics/高级排障/01-control-plane/05-webhook-admission-troubleshooting.md|Webhook 与准入控制故障排查指南]]
- [[domain-10-troubleshooting-diagnostics/高级排障/01-control-plane/10-control-plane-upgrade-troubleshooting.md|控制平面升级迁移问题处理指南]]

## 结构化故障排查 - 网络

- [[domain-10-troubleshooting-diagnostics/高级排障/03-networking/01-cni-troubleshooting.md|CNI 网络插件故障排查指南]]
- [[domain-10-troubleshooting-diagnostics/高级排障/03-networking/02-dns-troubleshooting.md|CoreDNS/DNS 故障排查指南]]
- [[domain-10-troubleshooting-diagnostics/高级排障/03-networking/03-service-ingress-troubleshooting.md|Service 与 Ingress 故障排查指南]]
- [[domain-10-troubleshooting-diagnostics/高级排障/03-networking/04-networkpolicy-troubleshooting.md|NetworkPolicy 深度排查与零信任安全治理指南]]
- [[domain-10-troubleshooting-diagnostics/高级排障/03-networking/05-service-mesh-istio-troubleshooting.md|Service Mesh (Istio) 深度排查与性能调优指南]]
- [[domain-10-troubleshooting-diagnostics/高级排障/03-networking/07-terway-troubleshooting.md|Terway（阿里云 CNI）网络故障排查指南]]
- [[domain-10-troubleshooting-diagnostics/高级排障/03-networking/08-flannel-troubleshooting.md|Flannel 网络故障排查指南]]

## 结构化故障排查 - 存储

- [[domain-10-troubleshooting-diagnostics/高级排障/04-storage/01-pv-pvc-troubleshooting.md|PV/PVC 存储深度排查与持久化治理指南]]
- [[domain-10-troubleshooting-diagnostics/高级排障/04-storage/02-csi-troubleshooting.md|CSI 存储驱动深度排查与架构优化指南]]
- [[domain-10-troubleshooting-diagnostics/高级排障/04-storage/05-storageclass-troubleshooting.md|StorageClass 配置与动态供给故障排查指南]]

## 结构化故障排查 - 调度资源

- [[domain-10-troubleshooting-diagnostics/高级排障/07-resources-scheduling/02-autoscaling-troubleshooting.md|HPA 与 VPA 自动扩缩容故障排查指南]]
- [[domain-10-troubleshooting-diagnostics/高级排障/07-resources-scheduling/04-pdb-troubleshooting.md|PodDisruptionBudget (PDB) 故障排查指南]]

## 结构化故障排查 - AI/ML

- [[32-发布/package/2026-07-02_18-29/corpus/core/domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/12-ai-ml-workloads/02-kubeflow-troubleshooting|Kubeflow 平台故障排查指南]]

## 结构化故障排查 - GitOps/DevOps

- [[32-发布/package/2026-07-02_18-29/corpus/core/domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/14-gitops-devops/01-gitops-devops-troubleshooting|GitOps/DevOps 故障排查指南]]
- [[32-发布/package/2026-07-02_18-29/corpus/core/domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/14-gitops-devops/02-tekton-troubleshooting|Tekton CI/CD 流水线故障排查指南]]
- [[32-发布/package/2026-07-02_18-29/corpus/core/domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/14-gitops-devops/03-flux-image-automation-troubleshooting|Flux 镜像自动化故障排查指南]]

## 结构化故障排查 - 可观测性

- [[32-发布/package/2026-07-02_18-29/corpus/core/domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/15-monitoring-observability/01-monitoring-observability-troubleshooting|可观测性故障排查指南]]
- [[32-发布/package/2026-07-02_18-29/corpus/core/domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/15-monitoring-observability/02-opentelemetry-troubleshooting|OpenTelemetry Collector 故障排查指南]]
- [[32-发布/package/2026-07-02_18-29/corpus/core/domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/15-monitoring-observability/04-finops-cost-optimization-troubleshooting|FinOps 成本优化与云费用故障排查指南]]

## 结构化故障排查

- [[domain-10-troubleshooting-diagnostics/高级排障/02-node-components/02-kube-proxy-troubleshooting.md|kube-proxy 故障排查指南]]
- [[domain-10-troubleshooting-diagnostics/高级排障/02-node-components/05-image-registry-troubleshooting.md|镜像与镜像仓库故障排查指南]]
- [[domain-10-troubleshooting-diagnostics/高级排障/02-node-components/06-gpu-device-plugin-troubleshooting.md|GPU 与设备插件故障排查指南]]
- [[domain-10-troubleshooting-diagnostics/高级排障/05-workloads/02-deployment-troubleshooting.md|Deployment 故障排查指南]]
- [[domain-10-troubleshooting-diagnostics/高级排障/05-workloads/03-statefulset-troubleshooting.md|StatefulSet 故障排查指南]]
- [[domain-10-troubleshooting-diagnostics/高级排障/05-workloads/04-daemonset-troubleshooting.md|DaemonSet 故障排查指南]]
- [[domain-10-troubleshooting-diagnostics/高级排障/05-workloads/06-configmap-secret-troubleshooting.md|ConfigMap 与 Secret 故障排查指南]]
- [[domain-10-troubleshooting-diagnostics/高级排障/08-cluster-operations/02-logging-monitoring-troubleshooting.md|日志与监控故障排查指南]]
- [[domain-10-troubleshooting-diagnostics/高级排障/08-cluster-operations/03-helm-troubleshooting.md|Helm 部署故障排查指南]]
- [[domain-10-troubleshooting-diagnostics/高级排障/08-cluster-operations/05-crd-operator-troubleshooting.md|CRD 与 Operator 故障排查指南]]
- [[domain-10-troubleshooting-diagnostics/高级排障/08-cluster-operations/06-kustomize-troubleshooting.md|Kustomize 部署故障排查指南]]

## FTA 故障树

- [[domain-10-troubleshooting-diagnostics/FTA故障树/list/backup-restore-fta.md|备份/恢复异常 FTA 树]]

## 技能卡片

- [[09-deployment-rollout-failure|Deployment 滚动更新与回滚故障诊断 / Deployment Rollout & Rollback Failure Diagnosis]]
- [[32-发布/package/2026-07-02_18-29/corpus/core/domain-10-troubleshooting-diagnostics/topic-skills/14-logging-pipeline-failure|日志收集与管理故障诊断与修复 / Logging Pipeline Diagnosis & Remediation]]

## YAML 清单参考

- 36 - 生态工具 (Kustomize / Helm / ArgoCD) YAML 配置参考

## 术语词典

- [[domain-17-system-foundation/知识字典/configuration/configmaps.md|ConfigMaps]]
- [[domain-17-system-foundation/知识字典/fundamentals/annotations.md|注解]]
- [[domain-17-system-foundation/知识字典/fundamentals/kubernetes-concepts-reference.md|知识地图]]
- [[domain-17-system-foundation/知识字典/fundamentals/kubernetes-object-management.md|Kubernetes 对象管理]]
- [[domain-17-system-foundation/知识字典/fundamentals/labels-and-selectors.md|标签和选择器]]
- [[domain-17-system-foundation/知识字典/fundamentals/recommended-labels.md|推荐标签]]
- [[domain-17-system-foundation/知识字典/fundamentals/the-kubectl-command-line-tool.md|kubectl 命令行工具]]
- [[domain-17-system-foundation/知识字典/multi-cloud/edge-computing-and-k3s.md|边缘计算与轻量级 Kubernetes]]
- [[domain-17-system-foundation/知识字典/multi-cloud/multi-cloud-operations.md|10 - 多云混合云运维手册]]
- [[domain-17-system-foundation/知识字典/multi-cloud/spaceborne-computing.md|太空计算（Spaceborne Computing）]]
- [[domain-17-system-foundation/知识字典/networking/ebpf-and-cilium-networking.md|eBPF 与 Cilium 网络]]
- [[domain-17-system-foundation/知识字典/networking/gateway-api.md|Gateway API]]
- [[domain-17-system-foundation/知识字典/networking/ingress-controllers.md|Ingress Controllers]]
- [[domain-17-system-foundation/知识字典/networking/telco-cloud-and-5g-mec.md|电信云与 5G 多接入边缘计算（MEC）]]
- [[domain-17-system-foundation/知识字典/observability/log-aggregation-with-loki.md|日志聚合与 Loki]]
- [[domain-17-system-foundation/知识字典/observability/opentelemetry-and-distributed-tracing.md|OpenTelemetry 与分布式链路追踪]]
- [[domain-17-system-foundation/知识字典/operations/backup-disaster-recovery.md|备份与灾难恢复（Backup & Disaster Recovery）]]
- [[domain-17-system-foundation/知识字典/operations/change-management-release.md|14 - 变更管理与发布策略]]
- [[domain-17-system-foundation/知识字典/operations/chaos-engineering.md|混沌工程（Chaos Engineering）]]
- [[domain-17-system-foundation/知识字典/operations/enterprise-ops-practices.md|企业级运维最佳实践]]
- [[domain-17-system-foundation/知识字典/operations/failure-patterns-analysis.md|02 - Kubernetes 故障模式与根因分析字典]]
- [[domain-17-system-foundation/知识字典/operations/incident-management-runbooks.md|12 - 生产事故管理与应急手册]]
- [[domain-17-system-foundation/知识字典/operations/installing-addons.md|安装插件（Installing Addons）]]
- [[domain-17-system-foundation/知识字典/operations/operations-best-practices.md|01 - Kubernetes 生产环境运维最佳实践字典]]
- [[domain-17-system-foundation/知识字典/operations/performance-tuning-expert.md|03 - Kubernetes 性能调优专家指南]]
- [[domain-17-system-foundation/知识字典/operations/production-troubleshooting-playbook.md|16 - 生产环境故障排查剧本]]
- [[domain-17-system-foundation/知识字典/operations/sli-slo-sla-engineering.md|15 - SLI/SLO/SLA工程实践]]
- [[domain-17-system-foundation/知识字典/operations/sre-maturity-model.md|04 - SRE运维成熟度模型]]
- [[domain-17-system-foundation/知识字典/operations/stateful-services-operations.md|有状态服务运维]]
- [[domain-17-system-foundation/知识字典/platform-engineering/api-priority-and-fairness.md|API 优先级与公平性（API Priority and Fairness）]]
- [[domain-17-system-foundation/知识字典/platform-engineering/cluster-api-and-fleet-management.md|Cluster API 与集群舰队管理]]
- [[domain-17-system-foundation/知识字典/platform-engineering/developer-portal-and-platform-metrics.md|开发者门户与平台工程度量]]
- [[domain-17-system-foundation/知识字典/platform-engineering/gitops-and-continuous-delivery.md|GitOps 与持续交付]]
- [[domain-17-system-foundation/知识字典/platform-engineering/infrastructure-as-code-for-kubernetes.md|Kubernetes 基础设施即代码（IaC）]]
- [[domain-17-system-foundation/知识字典/platform-engineering/operator-pattern.md|Operator 模式]]
- [[domain-17-system-foundation/知识字典/scheduling/kubernetes-scheduler.md|Kubernetes Scheduler]]
- [[domain-17-system-foundation/知识字典/security/cloud-native-security-practices.md|09 - 云原生安全专家指南]]
- [[domain-17-system-foundation/知识字典/security/multi-tenancy.md|多租户]]
- [[domain-17-system-foundation/知识字典/security/policy-as-code.md|策略即代码（Policy as Code）]]
- [[domain-17-system-foundation/知识字典/security/secrets-management-deep-dive.md|密钥管理深度指南]]
- [[domain-17-system-foundation/知识字典/security/spiffe-spire-identity.md|SPIFFE / SPIRE 与工作负载身份]]
- [[domain-17-system-foundation/知识字典/security/supply-chain-security.md|软件供应链安全]]
- [[domain-17-system-foundation/知识字典/specialized-workloads/ai-infra-specialist.md|08 - AI/ML基础设施专业词典]]
- [[domain-17-system-foundation/知识字典/specialized-workloads/hpc-and-bioinformatics.md|高性能计算与生物信息学（HPC & Bioinformatics）]]
- [[domain-17-system-foundation/知识字典/specialized-workloads/kserve-model-serving.md|KServe 模型服务平台]]
- [[domain-17-system-foundation/知识字典/specialized-workloads/llm-inference-optimization.md|大语言模型（LLM）推理优化]]
- [[domain-17-system-foundation/知识字典/specialized-workloads/mlops-pipelines-and-model-registry.md|MLOps 流水线与模型仓库]]
- [[domain-17-system-foundation/知识字典/specialized-workloads/vector-databases-and-rag-infrastructure.md|向量数据库与 RAG 基础设施]]
- [[domain-17-system-foundation/知识字典/storage/high-performance-storage-networks.md|高性能存储网络（RDMA / NVMe-oF）]]
- [[domain-17-system-foundation/知识字典/storage/object-storage-and-data-pipelines.md|对象存储与数据流水线]]
- [[domain-17-system-foundation/知识字典/tooling/cli-commands.md|知识地图]]
- [[domain-17-system-foundation/知识字典/tooling/container-image-optimization.md|容器镜像优化]]
- [[domain-17-system-foundation/知识字典/tooling/tool-ecosystem.md|Kusheet 工具与开源项目 URL 汇总]]
- [[domain-17-system-foundation/知识字典/workloads/container-runtime-interface-cri.md|容器运行时接口（Container Runtime Interface, CRI）]]
- [[domain-17-system-foundation/知识字典/workloads/daemonset.md|DaemonSet]]
- [[domain-17-system-foundation/知识字典/workloads/deployments.md|Deployments]]
- [[domain-17-system-foundation/知识字典/workloads/managing-workloads.md|Managing Workloads]]
- [[domain-17-system-foundation/知识字典/workloads/replicationcontroller.md|ReplicationController]]
- [[domain-17-system-foundation/知识字典/workloads/spot-and-preemptible-workloads.md|Spot 与可抢占工作负载]]
- [[domain-17-system-foundation/知识字典/workloads/statefulsets.md|StatefulSets]]

## 云服务商

- [[domain-12-cloud-providers/AWS-EKS/aws-eks-overview.md|AWS EKS (Elastic Kubernetes Service) 概述]]
- [[domain-12-cloud-providers/Google-GKE/google-cloud-gke-overview.md|Google Cloud GKE (Google Kubernetes Engine) 概述]]
- [[domain-12-cloud-providers/Azure-AKS/azure-aks-overview.md|Azure AKS (Azure Kubernetes Service) 概述]]
- [[32-发布/package/2026-07-02_18-29/corpus/core/domain-12-cloud-providers/05-alicloud-ack/003-ack-slb-nlb-alb|ACK 关联产品 - 负载均衡 (SLB/NLB/ALB)]]
- [[32-发布/package/2026-07-02_18-29/corpus/core/domain-12-cloud-providers/05-alicloud-ack/006-ack-ros-iac|ACK 关联产品 - ROS 资源编排 (IaC)]]
- [[domain-12-cloud-providers/腾讯云TKE/tencent-tke-overview.md|腾讯云 TKE (Tencent Kubernetes Engine) 概述]]
- [[domain-12-cloud-providers/其他云/UCloud-UK8S/ucloud-uk8s-overview.md|UCloud UK8S (UCloud Kubernetes Service) 高性价比企业级实战指南]]
- [[domain-12-cloud-providers/其他云/火山引擎-VEK/volcengine-vek-overview.md|火山引擎 VEK (Volcengine Kubernetes) 字节级深度实战指南]]

## 生产运维

- 10-GitOps流水线实践

## 技术论文

- [[32-发布/package/2026-07-02_18-29/corpus/core/domain-19-landscape-references/01-papers/01-kubernetes-gitops-complete-practice-guide|05 kubernetes gitops complete practice guide]]

## GitOps / CI-CD

- Domain-23 GitOps & CI/CD — 开源项目索引
- Argo CD企业级GitOps实践指南
- Jenkins企业级CI/CD流水线深度实践
- GitLab CI/CD Enterprise Pipeline Automation Platform
- GitHub Actions Enterprise CI/CD Platform 深度实践
- Argo CD 企业级 GitOps 实践指南
- Flux GitOps 实践指南
- Tekton 云原生 CI/CD 实践指南
- Tekton Java CI/CD 流水线实践指南

## CNCF 生态

- Argo
- cert-manager
- Cilium
- CRI-O
- Crossplane
- CubeFS
- Dapr
- Dragonfly
- Falco
- Flux
- Harbor
- Helm
- in-toto
- Jaeger
- KEDA
- Knative
- KubeEdge
- Kubernetes
- Linkerd
- Prometheus
- SPIRE
- Artifact Hub
- Backstage
- Buildpacks
- Chaos Mesh
- Contour
- Cortex
- Emissary-Ingress
- Fluid
- Karmada
- KServe
- Kubeflow
- Kubescape
- KubeVela
- Kyverno
- Litmus
- Longhorn
- Metal3-io
- NATS
- Notary Project
- OpenCost
- OpenFGA
- OpenKruise
- OpenTelemetry
- OpenYurt
- Operator Framework
- Strimzi
- Volcano
- Aeraki Mesh
- Akri
- Antrea
- Armada
- Athenz
- Atlantis
- Bank-Vaults
- Capsule
- Carina
- Cartography
- Carvel
- cdk8s
- ChaosBlade
- CloudNativePG
- Clusternet
- Clusterpedia
- CoHDI (Composable Hyperconverged Disaggregated Infrastructure)
- Copa (Copacetic)
- Cozystack
- Dalec (Declarative Application Linux Environment Creator)
- DevSpace
- Dex
- Easegress
- Eraser
- External Secrets Operator
- HAMi (Heterogeneous AI Computing Virtualization Middleware)
- Headlamp
- HolmesGPT
- HwameiStor
- InterLink
- K0s
- k3s
- K8GB (Kubernetes Global Balancer)
- K8sGPT
- K8up
- Kagent (Kubernetes AI Agent)
- KAITO (Kubernetes AI Toolchain Operator)
- Kanister
- KCL (KusionStack Configuration Language)
- Kepler
- K Gateway (formerly Gloo Gateway)
- KitOps
- Kmesh
- ko
- Koordinator
- kpt
- Kube-OVN
- Kubean
- KubeArmor
- KubeClipper
- KubeElastic
- KubeFleet
- Kuberhealthy
- KubeSlice
- KubeStellar
- Kubewarden
- KUDO (Kubernetes Universal Declarative Operator)
- Kuma
- Kured
- KusionStack
- Logging Operator
- Meshery
- MetalLB
- Microcks
- ModelPack
- Network Service Mesh (NSM)
- Open Policy Containers (OPCR)
- OpenChoreo
- OpenEBS
- OpenFunction
- openGemini
- OpenGitOps
- OpenTofu
- ORAS
- OSCAL Compass
- OVN-Kubernetes
- Oxia
- Paralus
- Perses
- PipeCD
- Piraeus Datastore
- Pixie
- Porter
- Radius
- Ratify
- Runme
- SchemaHero
- Serverless Devs
- Shipwright
- SlimFaas
- SlimToolkit
- SOPS
- Spiderpool
- SpinKube
- Stacker
- Telepresence
- Tinkerbell
- Tokenetes
- Tremor
- Trickster
- Vineyard (v6d)
- Virtual Kubelet
- VS Code Kubernetes Tools
- werf
- xRegistry

## 培训学习

- 项目 P1: 从零搭建 K8s 集群
- 项目 P2: 生产级应用全栈编排
- 项目 P4: GitOps 流水线
- 项目 P5: 毕业综合实践项目
- 🔥 Kubernetes 生产运维实战训练营 🔥
- K8s 命令速查表
- 知识图谱模板
- 文档阅读顺序索引
- Week 1 Checkpoint: 自测检验
- Day 5: Kubernetes 架构全貌
- Day 6: K8s 架构深化 + 集群配置
- Day 7: 周复习 + 综合实践
- Week 2 Checkpoint: 自测检验
- Day 10: 工作负载 - Deployment + StatefulSet + DaemonSet
- Day 13: 网络栈 - Ingress + NetworkPolicy
- Day 17: 可观测性 - 监控 + Prometheus
- Day 18: 可观测性 - 日志 + 分布式追踪

## 迁移专题

- [[domain-08-release-change-management/迁移方案/09-migration-toolchain.md|09 - 迁移工具链参考]]


<!-- risk-assessed -->
