---
title: GitOps / CI-CD 全局索引
description: '## 架构基础'
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
created: "2026-05-23"
---

# GitOps / CI-CD 全局索引

> 全局索引：按关键字 **gitops-cicd** 聚合项目内所有相关内容。

## 架构基础

- [[concepts/kubernetes-architecture-overview|Kubernetes 架构全景图 (Architecture Overview)]]]]
- [[domain-01-cluster-fundamentals/01-architecture-overview/02-core-components-deep-dive]]
- [[domain-01-cluster-fundamentals/05-kubectl/05-kubectl-commands-reference]]
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

- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/01-control-plane/01-apiserver-troubleshooting|API Server 故障排查指南]]
- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/01-control-plane/03-scheduler-troubleshooting|Scheduler 故障排查指南]]
- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/01-control-plane/04-controller-manager-troubleshooting|Controller Manager 故障排查指南]]
- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/01-control-plane/05-webhook-admission-troubleshooting|Webhook 与准入控制故障排查指南]]
- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/01-control-plane/10-control-plane-upgrade-troubleshooting|控制平面升级迁移问题处理指南]]

## 结构化故障排查 - 网络

- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/03-networking/01-cni-troubleshooting|CNI 网络插件故障排查指南]]
- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/03-networking/02-dns-troubleshooting|CoreDNS/DNS 故障排查指南]]
- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/03-networking/03-service-ingress-troubleshooting|Service 与 Ingress 故障排查指南]]
- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/03-networking/04-networkpolicy-troubleshooting|NetworkPolicy 深度排查与零信任安全治理指南]]
- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/03-networking/05-service-mesh-istio-troubleshooting|Service Mesh (Istio) 深度排查与性能调优指南]]
- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/03-networking/07-terway-troubleshooting|Terway（阿里云 CNI）网络故障排查指南]]
- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/03-networking/08-flannel-troubleshooting|Flannel 网络故障排查指南]]

## 结构化故障排查 - 存储

- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/04-storage/01-pv-pvc-troubleshooting|PV/PVC 存储深度排查与持久化治理指南]]
- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/04-storage/02-csi-troubleshooting|CSI 存储驱动深度排查与架构优化指南]]
- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/04-storage/05-storageclass-troubleshooting|StorageClass 配置与动态供给故障排查指南]]

## 结构化故障排查 - 调度资源

- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/07-resources-scheduling/02-autoscaling-troubleshooting|HPA 与 VPA 自动扩缩容故障排查指南]]
- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/07-resources-scheduling/04-pdb-troubleshooting|PodDisruptionBudget (PDB) 故障排查指南]]

## 结构化故障排查 - AI/ML

- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/10-ai-ml-workloads/02-kubeflow-troubleshooting|Kubeflow 平台故障排查指南]]

## 结构化故障排查 - GitOps/DevOps

- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/11-gitops-devops/01-gitops-devops-troubleshooting|GitOps/DevOps 故障排查指南]]
- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/11-gitops-devops/02-tekton-troubleshooting|Tekton CI/CD 流水线故障排查指南]]
- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/11-gitops-devops/03-flux-image-automation-troubleshooting|Flux 镜像自动化故障排查指南]]

## 结构化故障排查 - 可观测性

- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/12-monitoring-observability/01-monitoring-observability-troubleshooting|可观测性故障排查指南]]
- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/12-monitoring-observability/02-opentelemetry-troubleshooting|OpenTelemetry Collector 故障排查指南]]
- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/12-monitoring-observability/04-finops-cost-optimization-troubleshooting|FinOps 成本优化与云费用故障排查指南]]

## 结构化故障排查

- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/02-node-components/02-kube-proxy-troubleshooting|kube-proxy 故障排查指南]]
- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/02-node-components/05-image-registry-troubleshooting|镜像与镜像仓库故障排查指南]]
- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/02-node-components/06-gpu-device-plugin-troubleshooting|GPU 与设备插件故障排查指南]]
- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/05-workloads/02-deployment-troubleshooting|Deployment 故障排查指南]]
- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/05-workloads/03-statefulset-troubleshooting|StatefulSet 故障排查指南]]
- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/05-workloads/04-daemonset-troubleshooting|DaemonSet 故障排查指南]]
- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/05-workloads/06-configmap-secret-troubleshooting|ConfigMap 与 Secret 故障排查指南]]
- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/08-cluster-operations/02-logging-monitoring-troubleshooting|日志与监控故障排查指南]]
- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/08-cluster-operations/03-helm-troubleshooting|Helm 部署故障排查指南]]
- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/08-cluster-operations/05-crd-operator-troubleshooting|CRD 与 Operator 故障排查指南]]
- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/08-cluster-operations/06-kustomize-troubleshooting|Kustomize 部署故障排查指南]]

## FTA 故障树

- [[domain-10-troubleshooting-diagnostics/topic-fta/list/backup-restore-fta|备份/恢复异常 FTA 树]]

## 技能卡片

- [[domain-10-troubleshooting-diagnostics/topic-skills/08-deployment-rollout-failure|[[Deployment 滚动更新与回滚故障诊断 / Deployment Rollout & Rollback Failure Diagnosis|Deployment 滚动更新与回滚故障诊断 / Deployment Rollout & Rollback Failure Diagnosis]]]]
- [[domain-10-troubleshooting-diagnostics/topic-skills/16-logging-pipeline-failure|日志收集与管理故障诊断与修复 / Logging Pipeline Diagnosis & Remediation]]

## YAML 清单参考

- 36 - 生态工具 (Kustomize / Helm / ArgoCD) YAML 配置参考

## 术语词典

- [[domain-17-system-foundation/topic-dictionary/configuration/configmaps|ConfigMaps]]
- [[domain-17-system-foundation/topic-dictionary/fundamentals/annotations|注解]]
- [[domain-17-system-foundation/topic-dictionary/fundamentals/kubernetes-concepts-reference|知识地图]]
- [[domain-17-system-foundation/topic-dictionary/fundamentals/kubernetes-object-management|Kubernetes 对象管理]]
- [[domain-17-system-foundation/topic-dictionary/fundamentals/labels-and-selectors|标签和选择器]]
- [[domain-17-system-foundation/topic-dictionary/fundamentals/recommended-labels|推荐标签]]
- [[domain-17-system-foundation/topic-dictionary/fundamentals/the-kubectl-command-line-tool|kubectl 命令行工具]]
- [[domain-17-system-foundation/topic-dictionary/multi-cloud/edge-computing-and-k3s|边缘计算与轻量级 Kubernetes]]
- [[domain-17-system-foundation/topic-dictionary/multi-cloud/multi-cloud-operations|10 - 多云混合云运维手册]]
- [[domain-17-system-foundation/topic-dictionary/multi-cloud/spaceborne-computing|太空计算（Spaceborne Computing）]]
- [[domain-17-system-foundation/topic-dictionary/networking/ebpf-and-cilium-networking|eBPF 与 Cilium 网络]]
- [[domain-17-system-foundation/topic-dictionary/networking/gateway-api|Gateway API]]
- [[domain-17-system-foundation/topic-dictionary/networking/ingress-controllers|Ingress Controllers]]
- [[domain-17-system-foundation/topic-dictionary/networking/telco-cloud-and-5g-mec|电信云与 5G 多接入边缘计算（MEC）]]
- [[domain-17-system-foundation/topic-dictionary/observability/log-aggregation-with-loki|日志聚合与 Loki]]
- [[domain-17-system-foundation/topic-dictionary/observability/opentelemetry-and-distributed-tracing|OpenTelemetry 与分布式链路追踪]]
- [[domain-17-system-foundation/topic-dictionary/operations/backup-disaster-recovery|备份与灾难恢复（Backup & Disaster Recovery）]]
- [[domain-17-system-foundation/topic-dictionary/operations/change-management-release|14 - 变更管理与发布策略]]
- [[domain-17-system-foundation/topic-dictionary/operations/chaos-engineering|混沌工程（Chaos Engineering）]]
- [[domain-17-system-foundation/topic-dictionary/operations/enterprise-ops-practices|企业级运维最佳实践]]
- [[domain-17-system-foundation/topic-dictionary/operations/failure-patterns-analysis|02 - Kubernetes 故障模式与根因分析字典]]
- [[domain-17-system-foundation/topic-dictionary/operations/incident-management-runbooks|12 - 生产事故管理与应急手册]]
- [[domain-17-system-foundation/topic-dictionary/operations/installing-addons|安装插件（Installing Addons）]]
- [[domain-17-system-foundation/topic-dictionary/operations/operations-best-practices|01 - Kubernetes 生产环境运维最佳实践字典]]
- [[domain-17-system-foundation/topic-dictionary/operations/performance-tuning-expert|03 - Kubernetes 性能调优专家指南]]
- [[domain-17-system-foundation/topic-dictionary/operations/production-troubleshooting-playbook|16 - 生产环境故障排查剧本]]
- [[domain-17-system-foundation/topic-dictionary/operations/sli-slo-sla-engineering|15 - SLI/SLO/SLA工程实践]]
- [[domain-17-system-foundation/topic-dictionary/operations/sre-maturity-model|04 - SRE运维成熟度模型]]
- [[domain-17-system-foundation/topic-dictionary/operations/stateful-services-operations|有状态服务运维]]
- [[domain-17-system-foundation/topic-dictionary/platform-engineering/api-priority-and-fairness|API 优先级与公平性（API Priority and Fairness）]]
- [[domain-17-system-foundation/topic-dictionary/platform-engineering/cluster-api-and-fleet-management|Cluster API 与集群舰队管理]]
- [[domain-17-system-foundation/topic-dictionary/platform-engineering/developer-portal-and-platform-metrics|开发者门户与平台工程度量]]
- [[domain-17-system-foundation/topic-dictionary/platform-engineering/gitops-and-continuous-delivery|GitOps 与持续交付]]
- [[domain-17-system-foundation/topic-dictionary/platform-engineering/infrastructure-as-code-for-kubernetes|Kubernetes 基础设施即代码（IaC）]]
- [[domain-17-system-foundation/topic-dictionary/platform-engineering/operator-pattern|Operator 模式]]
- [[domain-17-system-foundation/topic-dictionary/scheduling/kubernetes-scheduler|Kubernetes Scheduler]]
- [[domain-17-system-foundation/topic-dictionary/security/cloud-native-security-practices|09 - 云原生安全专家指南]]
- [[domain-17-system-foundation/topic-dictionary/security/multi-tenancy|多租户]]
- [[domain-17-system-foundation/topic-dictionary/security/policy-as-code|策略即代码（Policy as Code）]]
- [[domain-17-system-foundation/topic-dictionary/security/secrets-management-deep-dive|密钥管理深度指南]]
- [[domain-17-system-foundation/topic-dictionary/security/spiffe-spire-identity|SPIFFE / SPIRE 与工作负载身份]]
- [[domain-17-system-foundation/topic-dictionary/security/supply-chain-security|软件供应链安全]]
- [[domain-17-system-foundation/topic-dictionary/specialized-workloads/ai-infra-specialist|08 - AI/ML基础设施专业词典]]
- [[domain-17-system-foundation/topic-dictionary/specialized-workloads/hpc-and-bioinformatics|高性能计算与生物信息学（HPC & Bioinformatics）]]
- [[domain-17-system-foundation/topic-dictionary/specialized-workloads/kserve-model-serving|KServe 模型服务平台]]
- [[domain-17-system-foundation/topic-dictionary/specialized-workloads/llm-inference-optimization|大语言模型（LLM）推理优化]]
- [[domain-17-system-foundation/topic-dictionary/specialized-workloads/mlops-pipelines-and-model-registry|MLOps 流水线与模型仓库]]
- [[domain-17-system-foundation/topic-dictionary/specialized-workloads/vector-databases-and-rag-infrastructure|向量数据库与 RAG 基础设施]]
- [[domain-17-system-foundation/topic-dictionary/storage/high-performance-storage-networks|高性能存储网络（RDMA / NVMe-oF）]]
- [[domain-17-system-foundation/topic-dictionary/storage/object-storage-and-data-pipelines|对象存储与数据流水线]]
- [[domain-17-system-foundation/topic-dictionary/tooling/cli-commands|知识地图]]
- [[domain-17-system-foundation/topic-dictionary/tooling/container-image-optimization|容器镜像优化]]
- [[domain-17-system-foundation/topic-dictionary/tooling/tool-ecosystem|Kusheet 工具与开源项目 URL 汇总]]
- [[domain-17-system-foundation/topic-dictionary/workloads/container-runtime-interface-cri|容器运行时接口（Container Runtime Interface, CRI）]]
- [[domain-17-system-foundation/topic-dictionary/workloads/daemonset|DaemonSet]]
- [[domain-17-system-foundation/topic-dictionary/workloads/deployments|Deployments]]
- [[domain-17-system-foundation/topic-dictionary/workloads/managing-workloads|Managing Workloads]]
- [[domain-17-system-foundation/topic-dictionary/workloads/replicationcontroller|ReplicationController]]
- [[domain-17-system-foundation/topic-dictionary/workloads/spot-and-preemptible-workloads|Spot 与可抢占工作负载]]
- [[domain-17-system-foundation/topic-dictionary/workloads/statefulsets|StatefulSets]]

## 云服务商

- [[domain-12-cloud-providers/01-aws-eks/aws-eks-overview|AWS EKS (Elastic Kubernetes Service) 概述]]
- [[domain-12-cloud-providers/02-google-cloud-gke/google-cloud-gke-overview|Google Cloud GKE (Google Kubernetes Engine) 概述]]
- [[domain-12-cloud-providers/03-azure-aks/azure-aks-overview|Azure AKS (Azure Kubernetes Service) 概述]]
- [[domain-12-cloud-providers/04-alicloud-ack/241-ack-slb-nlb-alb|ACK 关联产品 - 负载均衡 (SLB/NLB/ALB)]]
- [[domain-12-cloud-providers/04-alicloud-ack/244-ack-ros-iac|ACK 关联产品 - ROS 资源编排 (IaC)]]
- [[domain-12-cloud-providers/05-tencent-tke/tencent-tke-overview|腾讯云 TKE (Tencent Kubernetes Engine) 概述]]
- [[domain-12-cloud-providers/07-ucloud-uk8s/ucloud-uk8s-overview|UCloud UK8S (UCloud Kubernetes Service) 高性价比企业级实战指南]]
- [[domain-12-cloud-providers/10-volcengine-vek/volcengine-vek-overview|火山引擎 VEK (Volcengine Kubernetes) 字节级深度实战指南]]

## 生产运维

- 10-GitOps流水线实践

## 技术论文

- [[domain-19-landscape-references/02-papers/05-kubernetes-gitops-complete-practice-guide]]

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

- [[domain-08-release-change-management/topic-migration/09-migration-toolchain|09 - 迁移工具链参考]]
