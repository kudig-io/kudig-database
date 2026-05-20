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
---

# GitOps / CI-CD 全局索引

> 全局索引：按关键字 **gitops-cicd** 聚合项目内所有相关内容。

## 架构基础

- [Kubernetes 架构全景图 (Architecture Overview)](./domain-1-architecture-fundamentals/01-kubernetes-architecture-overview.md)
- [Kubernetes 核心组件深度剖析 (Core Components Deep Dive)](./domain-1-architecture-fundamentals/02-core-components-deep-dive.md)
- [kubectl 命令完整参考 (kubectl Commands Complete Reference)](./domain-1-architecture-fundamentals/05-kubectl-commands-reference.md)
- [07 - 升级路径与策略指南](./domain-1-architecture-fundamentals/07-upgrade-paths-strategy.md)
- [08 - 多租户架构设计 (Multi-Tenancy Architecture)](./domain-1-architecture-fundamentals/08-multi-tenancy-architecture.md)
- [12 - Kubernetes 集群部署架构模式指南](./domain-1-architecture-fundamentals/12-cluster-deployment-patterns.md)
- [14 - Kubernetes 安全架构深度分析](./domain-1-architecture-fundamentals/14-security-architecture.md)
- [15 - Kubernetes 可观测性架构体系](./domain-1-architecture-fundamentals/15-observability-architecture.md)
- [17 - 生产环境运维最佳实践 (Production Operations Best Practices)](./domain-1-architecture-fundamentals/17-production-operations-best-practices.md)
- [18 - Kubernetes 升级和迁移策略指南](./domain-1-architecture-fundamentals/18-upgrade-migration-strategy.md)
- [Kubectl v1.29 - v1.33 新命令与用法速查](./domain-1-architecture-fundamentals/99-kubectl-v1.29-v1.33-new-commands-guide.md)

## 控制平面

- [GitOps自动化运维实践 (GitOps Automation Operations Practice)](./domain-3-control-plane/26-gitops-automation-operations.md)

## 平台运维

- [GitOps配置管理 (GitOps Configuration Management)](./domain-9-platform-ops/07-gitops-configuration-management.md)

## 扩展生态

- [47 - Helm Chart开发与管理](./domain-10-extensions/06-helm-charts-management.md)
- [129 - Helm 高级运维：复杂部署、CI/CD 集成与安全最佳实践](./domain-10-extensions/07-helm-advanced-operations.md)
- [21 - CI/CD管道表](./domain-10-extensions/08-cicd-pipelines.md)
- [48 - GitOps工作流](./domain-10-extensions/09-gitops-workflow-argocd.md)

## 结构化故障排查 - 控制平面

- [API Server 故障排查指南](./topic-structural-trouble-shooting/01-control-plane/01-apiserver-troubleshooting.md)
- [Scheduler 故障排查指南](./topic-structural-trouble-shooting/01-control-plane/03-scheduler-troubleshooting.md)
- [Controller Manager 故障排查指南](./topic-structural-trouble-shooting/01-control-plane/04-controller-manager-troubleshooting.md)
- [Webhook 与准入控制故障排查指南](./topic-structural-trouble-shooting/01-control-plane/05-webhook-admission-troubleshooting.md)
- [控制平面升级迁移问题处理指南](./topic-structural-trouble-shooting/01-control-plane/10-control-plane-upgrade-troubleshooting.md)

## 结构化故障排查 - 网络

- [CNI 网络插件故障排查指南](./topic-structural-trouble-shooting/03-networking/01-cni-troubleshooting.md)
- [CoreDNS/DNS 故障排查指南](./topic-structural-trouble-shooting/03-networking/02-dns-troubleshooting.md)
- [Service 与 Ingress 故障排查指南](./topic-structural-trouble-shooting/03-networking/03-service-ingress-troubleshooting.md)
- [NetworkPolicy 深度排查与零信任安全治理指南](./topic-structural-trouble-shooting/03-networking/04-networkpolicy-troubleshooting.md)
- [Service Mesh (Istio) 深度排查与性能调优指南](./topic-structural-trouble-shooting/03-networking/05-service-mesh-istio-troubleshooting.md)
- [Terway（阿里云 CNI）网络故障排查指南](./topic-structural-trouble-shooting/03-networking/07-terway-troubleshooting.md)
- [Flannel 网络故障排查指南](./topic-structural-trouble-shooting/03-networking/08-flannel-troubleshooting.md)

## 结构化故障排查 - 存储

- [PV/PVC 存储深度排查与持久化治理指南](./topic-structural-trouble-shooting/04-storage/01-pv-pvc-troubleshooting.md)
- [CSI 存储驱动深度排查与架构优化指南](./topic-structural-trouble-shooting/04-storage/02-csi-troubleshooting.md)
- [StorageClass 配置与动态供给故障排查指南](./topic-structural-trouble-shooting/04-storage/05-storageclass-troubleshooting.md)

## 结构化故障排查 - 调度资源

- [HPA 与 VPA 自动扩缩容故障排查指南](./topic-structural-trouble-shooting/07-resources-scheduling/02-autoscaling-troubleshooting.md)
- [PodDisruptionBudget (PDB) 故障排查指南](./topic-structural-trouble-shooting/07-resources-scheduling/04-pdb-troubleshooting.md)

## 结构化故障排查 - AI/ML

- [Kubeflow 平台故障排查指南](./topic-structural-trouble-shooting/10-ai-ml-workloads/02-kubeflow-troubleshooting.md)

## 结构化故障排查 - GitOps/DevOps

- [GitOps/DevOps 故障排查指南](./topic-structural-trouble-shooting/11-gitops-devops/01-gitops-devops-troubleshooting.md)
- [Tekton CI/CD 流水线故障排查指南](./topic-structural-trouble-shooting/11-gitops-devops/02-tekton-troubleshooting.md)
- [Flux 镜像自动化故障排查指南](./topic-structural-trouble-shooting/11-gitops-devops/03-flux-image-automation-troubleshooting.md)

## 结构化故障排查 - 可观测性

- [可观测性故障排查指南](./topic-structural-trouble-shooting/12-monitoring-observability/01-monitoring-observability-troubleshooting.md)
- [OpenTelemetry Collector 故障排查指南](./topic-structural-trouble-shooting/12-monitoring-observability/02-opentelemetry-troubleshooting.md)
- [FinOps 成本优化与云费用故障排查指南](./topic-structural-trouble-shooting/12-monitoring-observability/04-finops-cost-optimization-troubleshooting.md)

## 结构化故障排查

- [kube-proxy 故障排查指南](./topic-structural-trouble-shooting/02-node-components/02-kube-proxy-troubleshooting.md)
- [镜像与镜像仓库故障排查指南](./topic-structural-trouble-shooting/02-node-components/05-image-registry-troubleshooting.md)
- [GPU 与设备插件故障排查指南](./topic-structural-trouble-shooting/02-node-components/06-gpu-device-plugin-troubleshooting.md)
- [Deployment 故障排查指南](./topic-structural-trouble-shooting/05-workloads/02-deployment-troubleshooting.md)
- [StatefulSet 故障排查指南](./topic-structural-trouble-shooting/05-workloads/03-statefulset-troubleshooting.md)
- [DaemonSet 故障排查指南](./topic-structural-trouble-shooting/05-workloads/04-daemonset-troubleshooting.md)
- [ConfigMap 与 Secret 故障排查指南](./topic-structural-trouble-shooting/05-workloads/06-configmap-secret-troubleshooting.md)
- [日志与监控故障排查指南](./topic-structural-trouble-shooting/08-cluster-operations/02-logging-monitoring-troubleshooting.md)
- [Helm 部署故障排查指南](./topic-structural-trouble-shooting/08-cluster-operations/03-helm-troubleshooting.md)
- [CRD 与 Operator 故障排查指南](./topic-structural-trouble-shooting/08-cluster-operations/05-crd-operator-troubleshooting.md)
- [Kustomize 部署故障排查指南](./topic-structural-trouble-shooting/08-cluster-operations/06-kustomize-troubleshooting.md)

## FTA 故障树

- [备份/恢复异常 FTA 树](./topic-fta/list/backup-restore-fta.md)

## 技能卡片

- [Deployment 滚动更新与回滚故障诊断 / Deployment Rollout & Rollback Failure Diagnosis](./topic-skills/08-deployment-rollout-failure.md)
- [日志收集与管理故障诊断与修复 / Logging Pipeline Diagnosis & Remediation](./topic-skills/16-logging-pipeline-failure.md)

## YAML 清单参考

- [36 - 生态工具 (Kustomize / Helm / ArgoCD) YAML 配置参考](./domain-32-yaml-manifests/36-ecosystem-kustomize-helm-argocd.md)

## 术语词典

- [ConfigMaps](./topic-dictionary/configuration/configmaps.md)
- [注解](./topic-dictionary/fundamentals/annotations.md)
- [知识地图](./topic-dictionary/fundamentals/kubernetes-concepts-reference.md)
- [Kubernetes 对象管理](./topic-dictionary/fundamentals/kubernetes-object-management.md)
- [标签和选择器](./topic-dictionary/fundamentals/labels-and-selectors.md)
- [推荐标签](./topic-dictionary/fundamentals/recommended-labels.md)
- [kubectl 命令行工具](./topic-dictionary/fundamentals/the-kubectl-command-line-tool.md)
- [边缘计算与轻量级 Kubernetes](./topic-dictionary/multi-cloud/edge-computing-and-k3s.md)
- [10 - 多云混合云运维手册](./topic-dictionary/multi-cloud/multi-cloud-operations.md)
- [太空计算（Spaceborne Computing）](./topic-dictionary/multi-cloud/spaceborne-computing.md)
- [eBPF 与 Cilium 网络](./topic-dictionary/networking/ebpf-and-cilium-networking.md)
- [Gateway API](./topic-dictionary/networking/gateway-api.md)
- [Ingress Controllers](./topic-dictionary/networking/ingress-controllers.md)
- [电信云与 5G 多接入边缘计算（MEC）](./topic-dictionary/networking/telco-cloud-and-5g-mec.md)
- [日志聚合与 Loki](./topic-dictionary/observability/log-aggregation-with-loki.md)
- [OpenTelemetry 与分布式链路追踪](./topic-dictionary/observability/opentelemetry-and-distributed-tracing.md)
- [备份与灾难恢复（Backup & Disaster Recovery）](./topic-dictionary/operations/backup-disaster-recovery.md)
- [14 - 变更管理与发布策略](./topic-dictionary/operations/change-management-release.md)
- [混沌工程（Chaos Engineering）](./topic-dictionary/operations/chaos-engineering.md)
- [企业级运维最佳实践](./topic-dictionary/operations/enterprise-ops-practices.md)
- [02 - Kubernetes 故障模式与根因分析字典](./topic-dictionary/operations/failure-patterns-analysis.md)
- [12 - 生产事故管理与应急手册](./topic-dictionary/operations/incident-management-runbooks.md)
- [安装插件（Installing Addons）](./topic-dictionary/operations/installing-addons.md)
- [01 - Kubernetes 生产环境运维最佳实践字典](./topic-dictionary/operations/operations-best-practices.md)
- [03 - Kubernetes 性能调优专家指南](./topic-dictionary/operations/performance-tuning-expert.md)
- [16 - 生产环境故障排查剧本](./topic-dictionary/operations/production-troubleshooting-playbook.md)
- [15 - SLI/SLO/SLA工程实践](./topic-dictionary/operations/sli-slo-sla-engineering.md)
- [04 - SRE运维成熟度模型](./topic-dictionary/operations/sre-maturity-model.md)
- [有状态服务运维](./topic-dictionary/operations/stateful-services-operations.md)
- [API 优先级与公平性（API Priority and Fairness）](./topic-dictionary/platform-engineering/api-priority-and-fairness.md)
- [Cluster API 与集群舰队管理](./topic-dictionary/platform-engineering/cluster-api-and-fleet-management.md)
- [开发者门户与平台工程度量](./topic-dictionary/platform-engineering/developer-portal-and-platform-metrics.md)
- [GitOps 与持续交付](./topic-dictionary/platform-engineering/gitops-and-continuous-delivery.md)
- [Kubernetes 基础设施即代码（IaC）](./topic-dictionary/platform-engineering/infrastructure-as-code-for-kubernetes.md)
- [Operator 模式](./topic-dictionary/platform-engineering/operator-pattern.md)
- [Kubernetes Scheduler](./topic-dictionary/scheduling/kubernetes-scheduler.md)
- [09 - 云原生安全专家指南](./topic-dictionary/security/cloud-native-security-practices.md)
- [多租户](./topic-dictionary/security/multi-tenancy.md)
- [策略即代码（Policy as Code）](./topic-dictionary/security/policy-as-code.md)
- [密钥管理深度指南](./topic-dictionary/security/secrets-management-deep-dive.md)
- [SPIFFE / SPIRE 与工作负载身份](./topic-dictionary/security/spiffe-spire-identity.md)
- [软件供应链安全](./topic-dictionary/security/supply-chain-security.md)
- [08 - AI/ML基础设施专业词典](./topic-dictionary/specialized-workloads/ai-infra-specialist.md)
- [高性能计算与生物信息学（HPC & Bioinformatics）](./topic-dictionary/specialized-workloads/hpc-and-bioinformatics.md)
- [KServe 模型服务平台](./topic-dictionary/specialized-workloads/kserve-model-serving.md)
- [大语言模型（LLM）推理优化](./topic-dictionary/specialized-workloads/llm-inference-optimization.md)
- [MLOps 流水线与模型仓库](./topic-dictionary/specialized-workloads/mlops-pipelines-and-model-registry.md)
- [向量数据库与 RAG 基础设施](./topic-dictionary/specialized-workloads/vector-databases-and-rag-infrastructure.md)
- [高性能存储网络（RDMA / NVMe-oF）](./topic-dictionary/storage/high-performance-storage-networks.md)
- [对象存储与数据流水线](./topic-dictionary/storage/object-storage-and-data-pipelines.md)
- [知识地图](./topic-dictionary/tooling/cli-commands.md)
- [容器镜像优化](./topic-dictionary/tooling/container-image-optimization.md)
- [Kusheet 工具与开源项目 URL 汇总](./topic-dictionary/tooling/tool-ecosystem.md)
- [容器运行时接口（Container Runtime Interface, CRI）](./topic-dictionary/workloads/container-runtime-interface-cri.md)
- [DaemonSet](./topic-dictionary/workloads/daemonset.md)
- [Deployments](./topic-dictionary/workloads/deployments.md)
- [Managing Workloads](./topic-dictionary/workloads/managing-workloads.md)
- [ReplicationController](./topic-dictionary/workloads/replicationcontroller.md)
- [Spot 与可抢占工作负载](./topic-dictionary/workloads/spot-and-preemptible-workloads.md)
- [StatefulSets](./topic-dictionary/workloads/statefulsets.md)

## 云服务商

- [AWS EKS (Elastic Kubernetes Service) 概述](./domain-17-cloud-provider/01-aws-eks/aws-eks-overview.md)
- [Google Cloud GKE (Google Kubernetes Engine) 概述](./domain-17-cloud-provider/02-google-cloud-gke/google-cloud-gke-overview.md)
- [Azure AKS (Azure Kubernetes Service) 概述](./domain-17-cloud-provider/03-azure-aks/azure-aks-overview.md)
- [ACK 关联产品 - 负载均衡 (SLB/NLB/ALB)](./domain-17-cloud-provider/04-alicloud-ack/241-ack-slb-nlb-alb.md)
- [ACK 关联产品 - ROS 资源编排 (IaC)](./domain-17-cloud-provider/04-alicloud-ack/244-ack-ros-iac.md)
- [腾讯云 TKE (Tencent Kubernetes Engine) 概述](./domain-17-cloud-provider/05-tencent-tke/tencent-tke-overview.md)
- [UCloud UK8S (UCloud Kubernetes Service) 高性价比企业级实战指南](./domain-17-cloud-provider/07-ucloud-uk8s/ucloud-uk8s-overview.md)
- [火山引擎 VEK (Volcengine Kubernetes) 字节级深度实战指南](./domain-17-cloud-provider/10-volcengine-vek/volcengine-vek-overview.md)

## 生产运维

- [10-GitOps流水线实践](./domain-18-production-operations/10-gitops-pipeline-practices.md)

## 技术论文

- [Kubernetes GitOps 完整实践指南 (GitOps Complete Practice Guide)](./domain-19-papers/05-kubernetes-gitops-complete-practice-guide.md)

## GitOps / CI-CD

- [Domain-23 GitOps & CI/CD — 开源项目索引](./domain-23-gitops-ci-cd/00-open-source-projects-index.md)
- [Argo CD企业级GitOps实践指南](./domain-23-gitops-ci-cd/01-argo-cd-enterprise-gitops.md)
- [Jenkins企业级CI/CD流水线深度实践](./domain-23-gitops-ci-cd/02-jenkins-enterprise-cicd.md)
- [GitLab CI/CD Enterprise Pipeline Automation Platform](./domain-23-gitops-ci-cd/03-gitlab-enterprise-cicd.md)
- [GitHub Actions Enterprise CI/CD Platform 深度实践](./domain-23-gitops-ci-cd/04-github-actions-enterprise.md)
- [Argo CD 企业级 GitOps 实践指南](./domain-23-gitops-ci-cd/99-argo-cd-gitops-guide.md)
- [Flux GitOps 实践指南](./domain-23-gitops-ci-cd/99-flux-gitops-guide.md)
- [Tekton 云原生 CI/CD 实践指南](./domain-23-gitops-ci-cd/99-tekton-cicd-guide.md)
- [Tekton Java CI/CD 流水线实践指南](./domain-23-gitops-ci-cd/99-tekton-java-cicd-guide.md)

## CNCF 生态

- [Argo](./domain-34-cncf-landscape/graduated/argo/argo.md)
- [cert-manager](./domain-34-cncf-landscape/graduated/cert-manager/cert-manager.md)
- [Cilium](./domain-34-cncf-landscape/graduated/cilium/cilium.md)
- [CRI-O](./domain-34-cncf-landscape/graduated/cri-o/cri-o.md)
- [Crossplane](./domain-34-cncf-landscape/graduated/crossplane/crossplane.md)
- [CubeFS](./domain-34-cncf-landscape/graduated/cubefs/cubefs.md)
- [Dapr](./domain-34-cncf-landscape/graduated/dapr/dapr.md)
- [Dragonfly](./domain-34-cncf-landscape/graduated/dragonfly/dragonfly.md)
- [Falco](./domain-34-cncf-landscape/graduated/falco/falco.md)
- [Flux](./domain-34-cncf-landscape/graduated/flux/flux.md)
- [Harbor](./domain-34-cncf-landscape/graduated/harbor/harbor.md)
- [Helm](./domain-34-cncf-landscape/graduated/helm/helm.md)
- [in-toto](./domain-34-cncf-landscape/graduated/in-toto/in-toto.md)
- [Jaeger](./domain-34-cncf-landscape/graduated/jaeger/jaeger.md)
- [KEDA](./domain-34-cncf-landscape/graduated/keda/keda.md)
- [Knative](./domain-34-cncf-landscape/graduated/knative/knative.md)
- [KubeEdge](./domain-34-cncf-landscape/graduated/kubeedge/kubeedge.md)
- [Kubernetes](./domain-34-cncf-landscape/graduated/kubernetes/kubernetes.md)
- [Linkerd](./domain-34-cncf-landscape/graduated/linkerd/linkerd.md)
- [Prometheus](./domain-34-cncf-landscape/graduated/prometheus/prometheus.md)
- [SPIRE](./domain-34-cncf-landscape/graduated/spire/spire.md)
- [Artifact Hub](./domain-34-cncf-landscape/incubating/artifact-hub/artifact-hub.md)
- [Backstage](./domain-34-cncf-landscape/incubating/backstage/backstage.md)
- [Buildpacks](./domain-34-cncf-landscape/incubating/buildpacks/buildpacks.md)
- [Chaos Mesh](./domain-34-cncf-landscape/incubating/chaos-mesh/chaos-mesh.md)
- [Contour](./domain-34-cncf-landscape/incubating/contour/contour.md)
- [Cortex](./domain-34-cncf-landscape/incubating/cortex/cortex.md)
- [Emissary-Ingress](./domain-34-cncf-landscape/incubating/emissary-ingress/emissary-ingress.md)
- [Fluid](./domain-34-cncf-landscape/incubating/fluid/fluid.md)
- [Karmada](./domain-34-cncf-landscape/incubating/karmada/karmada.md)
- [KServe](./domain-34-cncf-landscape/incubating/kserve/kserve.md)
- [Kubeflow](./domain-34-cncf-landscape/incubating/kubeflow/kubeflow.md)
- [Kubescape](./domain-34-cncf-landscape/incubating/kubescape/kubescape.md)
- [KubeVela](./domain-34-cncf-landscape/incubating/kubevela/kubevela.md)
- [Kyverno](./domain-34-cncf-landscape/incubating/kyverno/kyverno.md)
- [Litmus](./domain-34-cncf-landscape/incubating/litmus/litmus.md)
- [Longhorn](./domain-34-cncf-landscape/incubating/longhorn/longhorn.md)
- [Metal3-io](./domain-34-cncf-landscape/incubating/metal3-io/metal3-io.md)
- [NATS](./domain-34-cncf-landscape/incubating/nats/nats.md)
- [Notary Project](./domain-34-cncf-landscape/incubating/notary-project/notary-project.md)
- [OpenCost](./domain-34-cncf-landscape/incubating/opencost/opencost.md)
- [OpenFGA](./domain-34-cncf-landscape/incubating/openfga/openfga.md)
- [OpenKruise](./domain-34-cncf-landscape/incubating/openkruise/openkruise.md)
- [OpenTelemetry](./domain-34-cncf-landscape/incubating/opentelemetry/opentelemetry.md)
- [OpenYurt](./domain-34-cncf-landscape/incubating/openyurt/openyurt.md)
- [Operator Framework](./domain-34-cncf-landscape/incubating/operator-framework/operator-framework.md)
- [Strimzi](./domain-34-cncf-landscape/incubating/strimzi/strimzi.md)
- [Volcano](./domain-34-cncf-landscape/incubating/volcano/volcano.md)
- [Aeraki Mesh](./domain-34-cncf-landscape/sandbox/aeraki-mesh/aeraki-mesh.md)
- [Akri](./domain-34-cncf-landscape/sandbox/akri/akri.md)
- [Antrea](./domain-34-cncf-landscape/sandbox/antrea/antrea.md)
- [Armada](./domain-34-cncf-landscape/sandbox/armada/armada.md)
- [Athenz](./domain-34-cncf-landscape/sandbox/athenz/athenz.md)
- [Atlantis](./domain-34-cncf-landscape/sandbox/atlantis/atlantis.md)
- [Bank-Vaults](./domain-34-cncf-landscape/sandbox/bank-vaults/bank-vaults.md)
- [Capsule](./domain-34-cncf-landscape/sandbox/capsule/capsule.md)
- [Carina](./domain-34-cncf-landscape/sandbox/carina/carina.md)
- [Cartography](./domain-34-cncf-landscape/sandbox/cartography/cartography.md)
- [Carvel](./domain-34-cncf-landscape/sandbox/carvel/carvel.md)
- [cdk8s](./domain-34-cncf-landscape/sandbox/cdk8s/cdk8s.md)
- [ChaosBlade](./domain-34-cncf-landscape/sandbox/chaosblade/chaosblade.md)
- [CloudNativePG](./domain-34-cncf-landscape/sandbox/cloudnativepg/cloudnativepg.md)
- [Clusternet](./domain-34-cncf-landscape/sandbox/clusternet/clusternet.md)
- [Clusterpedia](./domain-34-cncf-landscape/sandbox/clusterpedia/clusterpedia.md)
- [CoHDI (Composable Hyperconverged Disaggregated Infrastructure)](./domain-34-cncf-landscape/sandbox/cohdi/cohdi.md)
- [Copa (Copacetic)](./domain-34-cncf-landscape/sandbox/copa/copa.md)
- [Cozystack](./domain-34-cncf-landscape/sandbox/cozystack/cozystack.md)
- [Dalec (Declarative Application Linux Environment Creator)](./domain-34-cncf-landscape/sandbox/dalec/dalec.md)
- [DevSpace](./domain-34-cncf-landscape/sandbox/devspace/devspace.md)
- [Dex](./domain-34-cncf-landscape/sandbox/dex/dex.md)
- [Easegress](./domain-34-cncf-landscape/sandbox/easegress/easegress.md)
- [Eraser](./domain-34-cncf-landscape/sandbox/eraser/eraser.md)
- [External Secrets Operator](./domain-34-cncf-landscape/sandbox/external-secrets/external-secrets.md)
- [HAMi (Heterogeneous AI Computing Virtualization Middleware)](./domain-34-cncf-landscape/sandbox/hami/hami.md)
- [Headlamp](./domain-34-cncf-landscape/sandbox/headlamp/headlamp.md)
- [HolmesGPT](./domain-34-cncf-landscape/sandbox/holmesgpt/holmesgpt.md)
- [HwameiStor](./domain-34-cncf-landscape/sandbox/hwameistor/hwameistor.md)
- [InterLink](./domain-34-cncf-landscape/sandbox/interlink/interlink.md)
- [K0s](./domain-34-cncf-landscape/sandbox/k0s/k0s.md)
- [k3s](./domain-34-cncf-landscape/sandbox/k3s/k3s.md)
- [K8GB (Kubernetes Global Balancer)](./domain-34-cncf-landscape/sandbox/k8gb/k8gb.md)
- [K8sGPT](./domain-34-cncf-landscape/sandbox/k8sgpt/k8sgpt.md)
- [K8up](./domain-34-cncf-landscape/sandbox/k8up/k8up.md)
- [Kagent (Kubernetes AI Agent)](./domain-34-cncf-landscape/sandbox/kagent/kagent.md)
- [KAITO (Kubernetes AI Toolchain Operator)](./domain-34-cncf-landscape/sandbox/kaito/kaito.md)
- [Kanister](./domain-34-cncf-landscape/sandbox/kanister/kanister.md)
- [KCL (KusionStack Configuration Language)](./domain-34-cncf-landscape/sandbox/kcl/kcl.md)
- [Kepler](./domain-34-cncf-landscape/sandbox/kepler/kepler.md)
- [K Gateway (formerly Gloo Gateway)](./domain-34-cncf-landscape/sandbox/kgateway/kgateway.md)
- [KitOps](./domain-34-cncf-landscape/sandbox/kitops/kitops.md)
- [Kmesh](./domain-34-cncf-landscape/sandbox/kmesh/kmesh.md)
- [ko](./domain-34-cncf-landscape/sandbox/ko/ko.md)
- [Koordinator](./domain-34-cncf-landscape/sandbox/koordinator/koordinator.md)
- [kpt](./domain-34-cncf-landscape/sandbox/kpt/kpt.md)
- [Kube-OVN](./domain-34-cncf-landscape/sandbox/kube-ovn/kube-ovn.md)
- [Kubean](./domain-34-cncf-landscape/sandbox/kubean/kubean.md)
- [KubeArmor](./domain-34-cncf-landscape/sandbox/kubearmor/kubearmor.md)
- [KubeClipper](./domain-34-cncf-landscape/sandbox/kubeclipper/kubeclipper.md)
- [KubeElastic](./domain-34-cncf-landscape/sandbox/kubeelasti/kubeelasti.md)
- [KubeFleet](./domain-34-cncf-landscape/sandbox/kubefleet/kubefleet.md)
- [Kuberhealthy](./domain-34-cncf-landscape/sandbox/kuberhealthy/kuberhealthy.md)
- [KubeSlice](./domain-34-cncf-landscape/sandbox/kubeslice/kubeslice.md)
- [KubeStellar](./domain-34-cncf-landscape/sandbox/kubestellar/kubestellar.md)
- [Kubewarden](./domain-34-cncf-landscape/sandbox/kubewarden/kubewarden.md)
- [KUDO (Kubernetes Universal Declarative Operator)](./domain-34-cncf-landscape/sandbox/kudo/kudo.md)
- [Kuma](./domain-34-cncf-landscape/sandbox/kuma/kuma.md)
- [Kured](./domain-34-cncf-landscape/sandbox/kured/kured.md)
- [KusionStack](./domain-34-cncf-landscape/sandbox/kusionstack/kusionstack.md)
- [Logging Operator](./domain-34-cncf-landscape/sandbox/logging-operator/logging-operator.md)
- [Meshery](./domain-34-cncf-landscape/sandbox/meshery/meshery.md)
- [MetalLB](./domain-34-cncf-landscape/sandbox/metallb/metallb.md)
- [Microcks](./domain-34-cncf-landscape/sandbox/microcks/microcks.md)
- [ModelPack](./domain-34-cncf-landscape/sandbox/modelpack/modelpack.md)
- [Network Service Mesh (NSM)](./domain-34-cncf-landscape/sandbox/network-service-mesh/network-service-mesh.md)
- [Open Policy Containers (OPCR)](./domain-34-cncf-landscape/sandbox/open-policy-containers/open-policy-containers.md)
- [OpenChoreo](./domain-34-cncf-landscape/sandbox/openchoreo/openchoreo.md)
- [OpenEBS](./domain-34-cncf-landscape/sandbox/openebs/openebs.md)
- [OpenFunction](./domain-34-cncf-landscape/sandbox/openfunction/openfunction.md)
- [openGemini](./domain-34-cncf-landscape/sandbox/opengemini/opengemini.md)
- [OpenGitOps](./domain-34-cncf-landscape/sandbox/opengitops/opengitops.md)
- [OpenTofu](./domain-34-cncf-landscape/sandbox/opentofu/opentofu.md)
- [ORAS](./domain-34-cncf-landscape/sandbox/oras/oras.md)
- [OSCAL Compass](./domain-34-cncf-landscape/sandbox/oscal-compass/oscal-compass.md)
- [OVN-Kubernetes](./domain-34-cncf-landscape/sandbox/ovn-kubernetes/ovn-kubernetes.md)
- [Oxia](./domain-34-cncf-landscape/sandbox/oxia/oxia.md)
- [Paralus](./domain-34-cncf-landscape/sandbox/paralus/paralus.md)
- [Perses](./domain-34-cncf-landscape/sandbox/perses/perses.md)
- [PipeCD](./domain-34-cncf-landscape/sandbox/pipecd/pipecd.md)
- [Piraeus Datastore](./domain-34-cncf-landscape/sandbox/piraeus-datastore/piraeus-datastore.md)
- [Pixie](./domain-34-cncf-landscape/sandbox/pixie/pixie.md)
- [Porter](./domain-34-cncf-landscape/sandbox/porter/porter.md)
- [Radius](./domain-34-cncf-landscape/sandbox/radius/radius.md)
- [Ratify](./domain-34-cncf-landscape/sandbox/ratify/ratify.md)
- [Runme](./domain-34-cncf-landscape/sandbox/runme-notebooks/runme-notebooks.md)
- [SchemaHero](./domain-34-cncf-landscape/sandbox/schemahero/schemahero.md)
- [Serverless Devs](./domain-34-cncf-landscape/sandbox/serverless-devs/serverless-devs.md)
- [Shipwright](./domain-34-cncf-landscape/sandbox/shipwright/shipwright.md)
- [SlimFaas](./domain-34-cncf-landscape/sandbox/slimfaas/slimfaas.md)
- [SlimToolkit](./domain-34-cncf-landscape/sandbox/slimtoolkit/slimtoolkit.md)
- [SOPS](./domain-34-cncf-landscape/sandbox/sops/sops.md)
- [Spiderpool](./domain-34-cncf-landscape/sandbox/spiderpool/spiderpool.md)
- [SpinKube](./domain-34-cncf-landscape/sandbox/spinkube/spinkube.md)
- [Stacker](./domain-34-cncf-landscape/sandbox/stacker/stacker.md)
- [Telepresence](./domain-34-cncf-landscape/sandbox/telepresence/telepresence.md)
- [Tinkerbell](./domain-34-cncf-landscape/sandbox/tinkerbell/tinkerbell.md)
- [Tokenetes](./domain-34-cncf-landscape/sandbox/tokenetes/tokenetes.md)
- [Tremor](./domain-34-cncf-landscape/sandbox/tremor/tremor.md)
- [Trickster](./domain-34-cncf-landscape/sandbox/trickster/trickster.md)
- [Vineyard (v6d)](./domain-34-cncf-landscape/sandbox/vineyard/vineyard.md)
- [Virtual Kubelet](./domain-34-cncf-landscape/sandbox/virtual-kubelet/virtual-kubelet.md)
- [VS Code Kubernetes Tools](./domain-34-cncf-landscape/sandbox/vscode-kubernetes-tools/vscode-kubernetes-tools.md)
- [werf](./domain-34-cncf-landscape/sandbox/werf/werf.md)
- [xRegistry](./domain-34-cncf-landscape/sandbox/xregistry/xregistry.md)

## 培训学习

- [项目 P1: 从零搭建 K8s 集群](./topic-learn/public-training/one-month/projects/p1-k8s-cluster-setup.md)
- [项目 P2: 生产级应用全栈编排](./topic-learn/public-training/one-month/projects/p2-production-app-orchestration.md)
- [项目 P4: GitOps 流水线](./topic-learn/public-training/one-month/projects/p4-gitops-pipeline.md)
- [项目 P5: 毕业综合实践项目](./topic-learn/public-training/one-month/projects/p5-graduation-project.md)
- [🔥 Kubernetes 生产运维实战训练营 🔥](./topic-learn/public-training/one-month/public-one-month-training.md)
- [K8s 命令速查表](./topic-learn/public-training/one-month/resources/commands-cheatsheet.md)
- [知识图谱模板](./topic-learn/public-training/one-month/resources/knowledge-map.md)
- [文档阅读顺序索引](./topic-learn/public-training/one-month/resources/reading-sequence.md)
- [Week 1 Checkpoint: 自测检验](./topic-learn/public-training/one-month/week-1-foundation/checkpoint.md)
- [Day 5: Kubernetes 架构全貌](./topic-learn/public-training/one-month/week-1-foundation/day-5-k8s-architecture.md)
- [Day 6: K8s 架构深化 + 集群配置](./topic-learn/public-training/one-month/week-1-foundation/day-6-k8s-cluster.md)
- [Day 7: 周复习 + 综合实践](./topic-learn/public-training/one-month/week-1-foundation/day-7-review-practice.md)
- [Week 2 Checkpoint: 自测检验](./topic-learn/public-training/one-month/week-2-core-tech/checkpoint.md)
- [Day 10: 工作负载 - Deployment + StatefulSet + DaemonSet](./topic-learn/public-training/one-month/week-2-core-tech/day-10-workloads-1.md)
- [Day 13: 网络栈 - Ingress + NetworkPolicy](./topic-learn/public-training/one-month/week-2-core-tech/day-13-networking-2.md)
- [Day 17: 可观测性 - 监控 + Prometheus](./topic-learn/public-training/one-month/week-3-operations/day-17-observability-1.md)
- [Day 18: 可观测性 - 日志 + 分布式追踪](./topic-learn/public-training/one-month/week-3-operations/day-18-observability-2.md)

## 迁移专题

- [09 - 迁移工具链参考](./topic-migration/09-migration-toolchain.md)
