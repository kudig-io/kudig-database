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
---

# GitOps / CI-CD 全局索引

> 全局索引：按关键字 **gitops-cicd** 聚合项目内所有相关内容。

## 架构基础

- [[domain-01-cluster-fundamentals/01-kubernetes-architecture-overview|Kubernetes 架构全景图 (Architecture Overview)]]
- [[domain-01-cluster-fundamentals/02-core-components-deep-dive|Kubernetes 核心组件深度剖析 (Core Components Deep Dive)]]
- [[domain-01-cluster-fundamentals/05-kubectl-commands-reference|kubectl 命令完整参考 (kubectl Commands Complete Reference)]]
- [[domain-01-cluster-fundamentals/07-upgrade-paths-strategy|07 - 升级路径与策略指南]]
- [[domain-01-cluster-fundamentals/08-multi-tenancy-architecture|08 - 多租户架构设计 (Multi-Tenancy Architecture)]]
- [[domain-01-cluster-fundamentals/12-cluster-deployment-patterns|12 - Kubernetes 集群部署架构模式指南]]
- [[domain-01-cluster-fundamentals/14-security-architecture|14 - Kubernetes 安全架构深度分析]]
- [[domain-01-cluster-fundamentals/15-observability-architecture|15 - Kubernetes 可观测性架构体系]]
- [[domain-01-cluster-fundamentals/17-production-operations-best-practices|17 - 生产环境运维最佳实践 (Production Operations Best Practices)]]
- [[domain-01-cluster-fundamentals/18-upgrade-migration-strategy|18 - Kubernetes 升级和迁移策略指南]]
- [[domain-01-cluster-fundamentals/99-kubectl-v1.29-v1.33-new-commands-guide|Kubectl v1.29 - v1.33 新命令与用法速查]]

## 控制平面

- [[domain-01-cluster-fundamentals/26-gitops-automation-operations|GitOps自动化运维实践 (GitOps Automation Operations Practice)]]

## 平台运维

- [[domain-07-platform-engineering/07-gitops-configuration-management|GitOps配置管理 (GitOps Configuration Management)]]

## 扩展生态

- [[domain-15-specialized-tech/06-helm-charts-management|47 - Helm Chart开发与管理]]
- [[domain-15-specialized-tech/07-helm-advanced-operations|129 - Helm 高级运维：复杂部署、CI/CD 集成与安全最佳实践]]
- [[domain-15-specialized-tech/08-cicd-pipelines|21 - CI/CD管道表]]
- [[domain-15-specialized-tech/09-gitops-workflow-argocd|48 - GitOps工作流]]

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

- [[domain-10-troubleshooting-diagnostics/topic-skills/08-deployment-rollout-failure|Deployment 滚动更新与回滚故障诊断 / Deployment Rollout & Rollback Failure Diagnosis]]
- [[domain-10-troubleshooting-diagnostics/topic-skills/16-logging-pipeline-failure|日志收集与管理故障诊断与修复 / Logging Pipeline Diagnosis & Remediation]]

## YAML 清单参考

- [[domain-18-manifests-patterns/36-ecosystem-kustomize-helm-argocd|36 - 生态工具 (Kustomize / Helm / ArgoCD) YAML 配置参考]]

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

- [[domain-11-production-operations/10-gitops-pipeline-practices|10-GitOps流水线实践]]

## 技术论文

- [[domain-19-landscape-references/05-kubernetes-gitops-complete-practice-guide|Kubernetes GitOps 完整实践指南 (GitOps Complete Practice Guide)]]

## GitOps / CI-CD

- [[domain-08-release-change-management/00-open-source-projects-index|Domain-23 GitOps & CI/CD — 开源项目索引]]
- [[domain-08-release-change-management/01-argo-cd-enterprise-gitops|Argo CD企业级GitOps实践指南]]
- [[domain-08-release-change-management/02-jenkins-enterprise-cicd|Jenkins企业级CI/CD流水线深度实践]]
- [[domain-08-release-change-management/03-gitlab-enterprise-cicd|GitLab CI/CD Enterprise Pipeline Automation Platform]]
- [[domain-08-release-change-management/04-github-actions-enterprise|GitHub Actions Enterprise CI/CD Platform 深度实践]]
- [[domain-08-release-change-management/99-argo-cd-gitops-guide|Argo CD 企业级 GitOps 实践指南]]
- [[domain-08-release-change-management/99-flux-gitops-guide|Flux GitOps 实践指南]]
- [[domain-08-release-change-management/99-tekton-cicd-guide|Tekton 云原生 CI/CD 实践指南]]
- [[domain-08-release-change-management/99-tekton-java-cicd-guide|Tekton Java CI/CD 流水线实践指南]]

## CNCF 生态

- [[domain-19-landscape-references/graduated/argo/argo|Argo]]
- [[domain-19-landscape-references/graduated/cert-manager/cert-manager|cert-manager]]
- [[domain-19-landscape-references/graduated/cilium/cilium|Cilium]]
- [[domain-19-landscape-references/graduated/cri-o/cri-o|CRI-O]]
- [[domain-19-landscape-references/graduated/crossplane/crossplane|Crossplane]]
- [[domain-19-landscape-references/graduated/cubefs/cubefs|CubeFS]]
- [[domain-19-landscape-references/graduated/dapr/dapr|Dapr]]
- [[domain-19-landscape-references/graduated/dragonfly/dragonfly|Dragonfly]]
- [[domain-19-landscape-references/graduated/falco/falco|Falco]]
- [[domain-19-landscape-references/graduated/flux/flux|Flux]]
- [[domain-19-landscape-references/graduated/harbor/harbor|Harbor]]
- [[domain-19-landscape-references/graduated/helm/helm|Helm]]
- [[domain-19-landscape-references/graduated/in-toto/in-toto|in-toto]]
- [[domain-19-landscape-references/graduated/jaeger/jaeger|Jaeger]]
- [[domain-19-landscape-references/graduated/keda/keda|KEDA]]
- [[domain-19-landscape-references/graduated/knative/knative|Knative]]
- [[domain-19-landscape-references/graduated/kubeedge/kubeedge|KubeEdge]]
- [[domain-19-landscape-references/graduated/kubernetes/kubernetes|Kubernetes]]
- [[domain-19-landscape-references/graduated/linkerd/linkerd|Linkerd]]
- [[domain-19-landscape-references/graduated/prometheus/prometheus|Prometheus]]
- [[domain-19-landscape-references/graduated/spire/spire|SPIRE]]
- [[domain-19-landscape-references/incubating/artifact-hub/artifact-hub|Artifact Hub]]
- [[domain-19-landscape-references/incubating/backstage/backstage|Backstage]]
- [[domain-19-landscape-references/incubating/buildpacks/buildpacks|Buildpacks]]
- [[domain-19-landscape-references/incubating/chaos-mesh/chaos-mesh|Chaos Mesh]]
- [[domain-19-landscape-references/incubating/contour/contour|Contour]]
- [[domain-19-landscape-references/incubating/cortex/cortex|Cortex]]
- [[domain-19-landscape-references/incubating/emissary-ingress/emissary-ingress|Emissary-Ingress]]
- [[domain-19-landscape-references/incubating/fluid/fluid|Fluid]]
- [[domain-19-landscape-references/incubating/karmada/karmada|Karmada]]
- [[domain-19-landscape-references/incubating/kserve/kserve|KServe]]
- [[domain-19-landscape-references/incubating/kubeflow/kubeflow|Kubeflow]]
- [[domain-19-landscape-references/incubating/kubescape/kubescape|Kubescape]]
- [[domain-19-landscape-references/incubating/kubevela/kubevela|KubeVela]]
- [[domain-19-landscape-references/incubating/kyverno/kyverno|Kyverno]]
- [[domain-19-landscape-references/incubating/litmus/litmus|Litmus]]
- [[domain-19-landscape-references/incubating/longhorn/longhorn|Longhorn]]
- [[domain-19-landscape-references/incubating/metal3-io/metal3-io|Metal3-io]]
- [[domain-19-landscape-references/incubating/nats/nats|NATS]]
- [[domain-19-landscape-references/incubating/notary-project/notary-project|Notary Project]]
- [[domain-19-landscape-references/incubating/opencost/opencost|OpenCost]]
- [[domain-19-landscape-references/incubating/openfga/openfga|OpenFGA]]
- [[domain-19-landscape-references/incubating/openkruise/openkruise|OpenKruise]]
- [[domain-19-landscape-references/incubating/opentelemetry/opentelemetry|OpenTelemetry]]
- [[domain-19-landscape-references/incubating/openyurt/openyurt|OpenYurt]]
- [[domain-19-landscape-references/incubating/operator-framework/operator-framework|Operator Framework]]
- [[domain-19-landscape-references/incubating/strimzi/strimzi|Strimzi]]
- [[domain-19-landscape-references/incubating/volcano/volcano|Volcano]]
- [[domain-19-landscape-references/sandbox/aeraki-mesh/aeraki-mesh|Aeraki Mesh]]
- [[domain-19-landscape-references/sandbox/akri/akri|Akri]]
- [[domain-19-landscape-references/sandbox/antrea/antrea|Antrea]]
- [[domain-19-landscape-references/sandbox/armada/armada|Armada]]
- [[domain-19-landscape-references/sandbox/athenz/athenz|Athenz]]
- [[domain-19-landscape-references/sandbox/atlantis/atlantis|Atlantis]]
- [[domain-19-landscape-references/sandbox/bank-vaults/bank-vaults|Bank-Vaults]]
- [[domain-19-landscape-references/sandbox/capsule/capsule|Capsule]]
- [[domain-19-landscape-references/sandbox/carina/carina|Carina]]
- [[domain-19-landscape-references/sandbox/cartography/cartography|Cartography]]
- [[domain-19-landscape-references/sandbox/carvel/carvel|Carvel]]
- [[domain-19-landscape-references/sandbox/cdk8s/cdk8s|cdk8s]]
- [[domain-19-landscape-references/sandbox/chaosblade/chaosblade|ChaosBlade]]
- [[domain-19-landscape-references/sandbox/cloudnativepg/cloudnativepg|CloudNativePG]]
- [[domain-19-landscape-references/sandbox/clusternet/clusternet|Clusternet]]
- [[domain-19-landscape-references/sandbox/clusterpedia/clusterpedia|Clusterpedia]]
- [[domain-19-landscape-references/sandbox/cohdi/cohdi|CoHDI (Composable Hyperconverged Disaggregated Infrastructure)]]
- [[domain-19-landscape-references/sandbox/copa/copa|Copa (Copacetic)]]
- [[domain-19-landscape-references/sandbox/cozystack/cozystack|Cozystack]]
- [[domain-19-landscape-references/sandbox/dalec/dalec|Dalec (Declarative Application Linux Environment Creator)]]
- [[domain-19-landscape-references/sandbox/devspace/devspace|DevSpace]]
- [[domain-19-landscape-references/sandbox/dex/dex|Dex]]
- [[domain-19-landscape-references/sandbox/easegress/easegress|Easegress]]
- [[domain-19-landscape-references/sandbox/eraser/eraser|Eraser]]
- [[domain-19-landscape-references/sandbox/external-secrets/external-secrets|External Secrets Operator]]
- [[domain-19-landscape-references/sandbox/hami/hami|HAMi (Heterogeneous AI Computing Virtualization Middleware)]]
- [[domain-19-landscape-references/sandbox/headlamp/headlamp|Headlamp]]
- [[domain-19-landscape-references/sandbox/holmesgpt/holmesgpt|HolmesGPT]]
- [[domain-19-landscape-references/sandbox/hwameistor/hwameistor|HwameiStor]]
- [[domain-19-landscape-references/sandbox/interlink/interlink|InterLink]]
- [[domain-19-landscape-references/sandbox/k0s/k0s|K0s]]
- [[domain-19-landscape-references/sandbox/k3s/k3s|k3s]]
- [[domain-19-landscape-references/sandbox/k8gb/k8gb|K8GB (Kubernetes Global Balancer)]]
- [[domain-19-landscape-references/sandbox/k8sgpt/k8sgpt|K8sGPT]]
- [[domain-19-landscape-references/sandbox/k8up/k8up|K8up]]
- [[domain-19-landscape-references/sandbox/kagent/kagent|Kagent (Kubernetes AI Agent)]]
- [[domain-19-landscape-references/sandbox/kaito/kaito|KAITO (Kubernetes AI Toolchain Operator)]]
- [[domain-19-landscape-references/sandbox/kanister/kanister|Kanister]]
- [[domain-19-landscape-references/sandbox/kcl/kcl|KCL (KusionStack Configuration Language)]]
- [[domain-19-landscape-references/sandbox/kepler/kepler|Kepler]]
- [[domain-19-landscape-references/sandbox/kgateway/kgateway|K Gateway (formerly Gloo Gateway)]]
- [[domain-19-landscape-references/sandbox/kitops/kitops|KitOps]]
- [[domain-19-landscape-references/sandbox/kmesh/kmesh|Kmesh]]
- [[domain-19-landscape-references/sandbox/ko/ko|ko]]
- [[domain-19-landscape-references/sandbox/koordinator/koordinator|Koordinator]]
- [[domain-19-landscape-references/sandbox/kpt/kpt|kpt]]
- [[domain-19-landscape-references/sandbox/kube-ovn/kube-ovn|Kube-OVN]]
- [[domain-19-landscape-references/sandbox/kubean/kubean|Kubean]]
- [[domain-19-landscape-references/sandbox/kubearmor/kubearmor|KubeArmor]]
- [[domain-19-landscape-references/sandbox/kubeclipper/kubeclipper|KubeClipper]]
- [[domain-19-landscape-references/sandbox/kubeelasti/kubeelasti|KubeElastic]]
- [[domain-19-landscape-references/sandbox/kubefleet/kubefleet|KubeFleet]]
- [[domain-19-landscape-references/sandbox/kuberhealthy/kuberhealthy|Kuberhealthy]]
- [[domain-19-landscape-references/sandbox/kubeslice/kubeslice|KubeSlice]]
- [[domain-19-landscape-references/sandbox/kubestellar/kubestellar|KubeStellar]]
- [[domain-19-landscape-references/sandbox/kubewarden/kubewarden|Kubewarden]]
- [[domain-19-landscape-references/sandbox/kudo/kudo|KUDO (Kubernetes Universal Declarative Operator)]]
- [[domain-19-landscape-references/sandbox/kuma/kuma|Kuma]]
- [[domain-19-landscape-references/sandbox/kured/kured|Kured]]
- [[domain-19-landscape-references/sandbox/kusionstack/kusionstack|KusionStack]]
- [[domain-19-landscape-references/sandbox/logging-operator/logging-operator|Logging Operator]]
- [[domain-19-landscape-references/sandbox/meshery/meshery|Meshery]]
- [[domain-19-landscape-references/sandbox/metallb/metallb|MetalLB]]
- [[domain-19-landscape-references/sandbox/microcks/microcks|Microcks]]
- [[domain-19-landscape-references/sandbox/modelpack/modelpack|ModelPack]]
- [[domain-19-landscape-references/sandbox/network-service-mesh/network-service-mesh|Network Service Mesh (NSM)]]
- [[domain-19-landscape-references/sandbox/open-policy-containers/open-policy-containers|Open Policy Containers (OPCR)]]
- [[domain-19-landscape-references/sandbox/openchoreo/openchoreo|OpenChoreo]]
- [[domain-19-landscape-references/sandbox/openebs/openebs|OpenEBS]]
- [[domain-19-landscape-references/sandbox/openfunction/openfunction|OpenFunction]]
- [[domain-19-landscape-references/sandbox/opengemini/opengemini|openGemini]]
- [[domain-19-landscape-references/sandbox/opengitops/opengitops|OpenGitOps]]
- [[domain-19-landscape-references/sandbox/opentofu/opentofu|OpenTofu]]
- [[domain-19-landscape-references/sandbox/oras/oras|ORAS]]
- [[domain-19-landscape-references/sandbox/oscal-compass/oscal-compass|OSCAL Compass]]
- [[domain-19-landscape-references/sandbox/ovn-kubernetes/ovn-kubernetes|OVN-Kubernetes]]
- [[domain-19-landscape-references/sandbox/oxia/oxia|Oxia]]
- [[domain-19-landscape-references/sandbox/paralus/paralus|Paralus]]
- [[domain-19-landscape-references/sandbox/perses/perses|Perses]]
- [[domain-19-landscape-references/sandbox/pipecd/pipecd|PipeCD]]
- [[domain-19-landscape-references/sandbox/piraeus-datastore/piraeus-datastore|Piraeus Datastore]]
- [[domain-19-landscape-references/sandbox/pixie/pixie|Pixie]]
- [[domain-19-landscape-references/sandbox/porter/porter|Porter]]
- [[domain-19-landscape-references/sandbox/radius/radius|Radius]]
- [[domain-19-landscape-references/sandbox/ratify/ratify|Ratify]]
- [[domain-19-landscape-references/sandbox/runme-notebooks/runme-notebooks|Runme]]
- [[domain-19-landscape-references/sandbox/schemahero/schemahero|SchemaHero]]
- [[domain-19-landscape-references/sandbox/serverless-devs/serverless-devs|Serverless Devs]]
- [[domain-19-landscape-references/sandbox/shipwright/shipwright|Shipwright]]
- [[domain-19-landscape-references/sandbox/slimfaas/slimfaas|SlimFaas]]
- [[domain-19-landscape-references/sandbox/slimtoolkit/slimtoolkit|SlimToolkit]]
- [[domain-19-landscape-references/sandbox/sops/sops|SOPS]]
- [[domain-19-landscape-references/sandbox/spiderpool/spiderpool|Spiderpool]]
- [[domain-19-landscape-references/sandbox/spinkube/spinkube|SpinKube]]
- [[domain-19-landscape-references/sandbox/stacker/stacker|Stacker]]
- [[domain-19-landscape-references/sandbox/telepresence/telepresence|Telepresence]]
- [[domain-19-landscape-references/sandbox/tinkerbell/tinkerbell|Tinkerbell]]
- [[domain-19-landscape-references/sandbox/tokenetes/tokenetes|Tokenetes]]
- [[domain-19-landscape-references/sandbox/tremor/tremor|Tremor]]
- [[domain-19-landscape-references/sandbox/trickster/trickster|Trickster]]
- [[domain-19-landscape-references/sandbox/vineyard/vineyard|Vineyard (v6d)]]
- [[domain-19-landscape-references/sandbox/virtual-kubelet/virtual-kubelet|Virtual Kubelet]]
- [[domain-19-landscape-references/sandbox/vscode-kubernetes-tools/vscode-kubernetes-tools|VS Code Kubernetes Tools]]
- [[domain-19-landscape-references/sandbox/werf/werf|werf]]
- [[domain-19-landscape-references/sandbox/xregistry/xregistry|xRegistry]]

## 培训学习

- [[domain-11-production-operations/topic-learn/public-training/one-month/projects/p1-k8s-cluster-setup|项目 P1: 从零搭建 K8s 集群]]
- [[domain-11-production-operations/topic-learn/public-training/one-month/projects/p2-production-app-orchestration|项目 P2: 生产级应用全栈编排]]
- [[domain-11-production-operations/topic-learn/public-training/one-month/projects/p4-gitops-pipeline|项目 P4: GitOps 流水线]]
- [[domain-11-production-operations/topic-learn/public-training/one-month/projects/p5-graduation-project|项目 P5: 毕业综合实践项目]]
- [[domain-11-production-operations/topic-learn/public-training/one-month/public-one-month-training|🔥 Kubernetes 生产运维实战训练营 🔥]]
- [[domain-11-production-operations/topic-learn/public-training/one-month/resources/commands-cheatsheet|K8s 命令速查表]]
- [[domain-11-production-operations/topic-learn/public-training/one-month/resources/knowledge-map|知识图谱模板]]
- [[domain-11-production-operations/topic-learn/public-training/one-month/resources/reading-sequence|文档阅读顺序索引]]
- [[domain-11-production-operations/topic-learn/public-training/one-month/week-1-foundation/checkpoint|Week 1 Checkpoint: 自测检验]]
- [[domain-11-production-operations/topic-learn/public-training/one-month/week-1-foundation/day-5-k8s-architecture|Day 5: Kubernetes 架构全貌]]
- [[domain-11-production-operations/topic-learn/public-training/one-month/week-1-foundation/day-6-k8s-cluster|Day 6: K8s 架构深化 + 集群配置]]
- [[domain-11-production-operations/topic-learn/public-training/one-month/week-1-foundation/day-7-review-practice|Day 7: 周复习 + 综合实践]]
- [[domain-11-production-operations/topic-learn/public-training/one-month/week-2-core-tech/checkpoint|Week 2 Checkpoint: 自测检验]]
- [[domain-11-production-operations/topic-learn/public-training/one-month/week-2-core-tech/day-10-workloads-1|Day 10: 工作负载 - Deployment + StatefulSet + DaemonSet]]
- [[domain-11-production-operations/topic-learn/public-training/one-month/week-2-core-tech/day-13-networking-2|Day 13: 网络栈 - Ingress + NetworkPolicy]]
- [[domain-11-production-operations/topic-learn/public-training/one-month/week-3-operations/day-17-observability-1|Day 17: 可观测性 - 监控 + Prometheus]]
- [[domain-11-production-operations/topic-learn/public-training/one-month/week-3-operations/day-18-observability-2|Day 18: 可观测性 - 日志 + 分布式追踪]]

## 迁移专题

- [[domain-08-release-change-management/topic-migration/09-migration-toolchain|09 - 迁移工具链参考]]
