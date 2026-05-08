# DNS 全局索引

> 全局索引：按关键字 **dns** 聚合项目内所有相关内容。

## 架构基础

- [Domain-1 架构基础 — 开源项目索引](./domain-1-architecture-fundamentals/00-open-source-projects-index.md)
- [Kubernetes 架构全景图 (Architecture Overview)](./domain-1-architecture-fundamentals/01-kubernetes-architecture-overview.md)
- [Kubernetes 核心组件深度剖析 (Core Components Deep Dive)](./domain-1-architecture-fundamentals/02-core-components-deep-dive.md)
- [kubectl 命令完整参考 (kubectl Commands Complete Reference)](./domain-1-architecture-fundamentals/05-kubectl-commands-reference.md)
- [06 - 集群配置参数完全参考](./domain-1-architecture-fundamentals/06-cluster-configuration-parameters.md)
- [07 - 升级路径与策略指南](./domain-1-architecture-fundamentals/07-upgrade-paths-strategy.md)
- [08 - 多租户架构设计 (Multi-Tenancy Architecture)](./domain-1-architecture-fundamentals/08-multi-tenancy-architecture.md)
- [12 - Kubernetes 集群部署架构模式指南](./domain-1-architecture-fundamentals/12-cluster-deployment-patterns.md)
- [13 - Kubernetes 性能调优专项指南](./domain-1-architecture-fundamentals/13-performance-tuning-guide.md)
- [14 - Kubernetes 安全架构深度分析](./domain-1-architecture-fundamentals/14-security-architecture.md)
- [15 - Kubernetes 可观测性架构体系](./domain-1-architecture-fundamentals/15-observability-architecture.md)
- [18 - Kubernetes 升级和迁移策略指南](./domain-1-architecture-fundamentals/18-upgrade-migration-strategy.md)
- [Kubernetes 核心组件 v1.29 - v1.33 新特性速查](./domain-1-architecture-fundamentals/99-kubernetes-core-components-v1.29-v1.33-update.md)
- [Kubernetes v1.29 - v1.33 完整 Feature Gate 与特性参考手册](./domain-1-architecture-fundamentals/99-kubernetes-v1.29-v1.33-complete-feature-gates-reference.md)
- [Kubernetes v1.29 - v1.33 版本特性深度指南](./domain-1-architecture-fundamentals/99-kubernetes-v1.29-v1.33-features-guide.md)

## 设计原理

- [01 - Kubernetes 设计原则与哲学 (Foundations)](./domain-2-design-principles/01-design-principles-foundations.md)
- [07 - 分布式共识与 etcd 原理 (etcd & Raft)](./domain-2-design-principles/07-distributed-consensus-etcd.md)
- [08 - 高可用架构模式 (HA Patterns)](./domain-2-design-principles/08-high-availability-patterns.md)
- [10 - CAP 定理与分布式系统基础 (CAP Theorem)](./domain-2-design-principles/10-cap-theorem-distributed-systems.md)
- [11 - 扩展性设计模式 (Extensibility)](./domain-2-design-principles/11-extensibility-design-patterns.md)
- [14 - 服务网格与微服务架构设计](./domain-2-design-principles/14-service-mesh-architecture.md)
- [15 - 混沌工程与故障注入设计](./domain-2-design-principles/15-chaos-engineering.md)
- [18 - 性能优化原理](./domain-2-design-principles/18-performance-optimization-principles.md)

## 网络知识域

- [04 - DNS 服务发现与 CoreDNS 调优](./domain-5-networking/11-dns-service-discovery-coredns.md)
- [33 - 服务发现与 DNS 配置 (Service Discovery & DNS)](./domain-5-networking/12-dns-service-discovery.md)
- [53 - CoreDNS 架构与核心原理 (Architecture & Principles)](./domain-5-networking/13-coredns-architecture-principles.md)
- [54 - CoreDNS Corefile 配置详解 (Corefile Configuration)](./domain-5-networking/14-coredns-configuration-corefile.md)
- [55 - CoreDNS 插件完整参考 (Plugins Reference)](./domain-5-networking/15-coredns-plugins-reference.md)
- [56 - CoreDNS 故障排查与性能优化 (Troubleshooting & Optimization)](./domain-5-networking/28-coredns-troubleshooting-optimization.md)

## 扩展生态

- [101 - 包管理与应用分发工具 (Package Management & Distribution)](./domain-10-extensions/05-package-management-tools.md)
- [103 - 容器镜像构建工具 (Container Image Build)](./domain-10-extensions/10-image-build-tools.md)

## 故障排查域

- [25 - 网络连通性故障排查 (Network Connectivity Troubleshooting)](./domain-12-troubleshooting/25-network-connectivity-troubleshooting.md)
- [26 - DNS 故障排查 (DNS Troubleshooting)](./domain-12-troubleshooting/26-dns-troubleshooting.md)

## 结构化故障排查 - 网络

- [CNI 网络插件故障排查指南](./topic-structural-trouble-shooting/03-networking/01-cni-troubleshooting.md)
- [CoreDNS/DNS 故障排查指南](./topic-structural-trouble-shooting/03-networking/02-dns-troubleshooting.md)
- [Service 与 Ingress 故障排查指南](./topic-structural-trouble-shooting/03-networking/03-service-ingress-troubleshooting.md)
- [NetworkPolicy 深度排查与零信任安全治理指南](./topic-structural-trouble-shooting/03-networking/04-networkpolicy-troubleshooting.md)
- [Service Mesh (Istio) 深度排查与性能调优指南](./topic-structural-trouble-shooting/03-networking/05-service-mesh-istio-troubleshooting.md)
- [Gateway API 深度排查与下一代流量治理指南](./topic-structural-trouble-shooting/03-networking/06-gateway-api-troubleshooting.md)
- [Terway（阿里云 CNI）网络故障排查指南](./topic-structural-trouble-shooting/03-networking/07-terway-troubleshooting.md)

## 结构化故障排查 - 调度资源

- [资源与调度故障排查指南](./topic-structural-trouble-shooting/07-resources-scheduling/01-resources-quota-troubleshooting.md)

## 结构化故障排查 - 可观测性

- [可观测性故障排查指南](./topic-structural-trouble-shooting/12-monitoring-observability/01-monitoring-observability-troubleshooting.md)
- [OpenTelemetry Collector 故障排查指南](./topic-structural-trouble-shooting/12-monitoring-observability/02-opentelemetry-troubleshooting.md)
- [eBPF 可观测性故障排查指南](./topic-structural-trouble-shooting/12-monitoring-observability/03-ebpf-observability-troubleshooting.md)
- [FinOps 成本优化与云费用故障排查指南](./topic-structural-trouble-shooting/12-monitoring-observability/04-finops-cost-optimization-troubleshooting.md)

## 结构化故障排查

- [API Server 故障排查指南](./topic-structural-trouble-shooting/01-control-plane/01-apiserver-troubleshooting.md)
- [Scheduler 故障排查指南](./topic-structural-trouble-shooting/01-control-plane/03-scheduler-troubleshooting.md)
- [Webhook 与准入控制故障排查指南](./topic-structural-trouble-shooting/01-control-plane/05-webhook-admission-troubleshooting.md)
- [控制平面升级迁移问题处理指南](./topic-structural-trouble-shooting/01-control-plane/10-control-plane-upgrade-troubleshooting.md)
- [kube-proxy 故障排查指南](./topic-structural-trouble-shooting/02-node-components/02-kube-proxy-troubleshooting.md)
- [容器运行时故障排查指南](./topic-structural-trouble-shooting/02-node-components/03-container-runtime-troubleshooting.md)
- [节点故障专项排查指南](./topic-structural-trouble-shooting/02-node-components/04-node-troubleshooting.md)
- [镜像与镜像仓库故障排查指南](./topic-structural-trouble-shooting/02-node-components/05-image-registry-troubleshooting.md)
- [PV/PVC 存储深度排查与持久化治理指南](./topic-structural-trouble-shooting/04-storage/01-pv-pvc-troubleshooting.md)
- [CSI 存储驱动深度排查与架构优化指南](./topic-structural-trouble-shooting/04-storage/02-csi-troubleshooting.md)
- [StorageClass 配置与动态供给故障排查指南](./topic-structural-trouble-shooting/04-storage/05-storageclass-troubleshooting.md)
- [Pod 故障排查与运行机制深度指南](./topic-structural-trouble-shooting/05-workloads/01-pod-troubleshooting.md)
- [StatefulSet 故障排查指南](./topic-structural-trouble-shooting/05-workloads/03-statefulset-troubleshooting.md)
- [DaemonSet 故障排查指南](./topic-structural-trouble-shooting/05-workloads/04-daemonset-troubleshooting.md)
- [Job 与 CronJob 故障排查指南](./topic-structural-trouble-shooting/05-workloads/05-job-cronjob-troubleshooting.md)
- [集群高可用与灾备故障排查指南](./topic-structural-trouble-shooting/08-cluster-operations/04-ha-disaster-recovery-troubleshooting.md)
- [云厂商集成故障排查指南](./topic-structural-trouble-shooting/09-cloud-provider/01-cloud-provider-integration-troubleshooting.md)
- [多云/混合云网络故障排查指南](./topic-structural-trouble-shooting/09-cloud-provider/02-multi-cloud-networking-troubleshooting.md)
- [云资源配额与 API 限流故障排查指南](./topic-structural-trouble-shooting/09-cloud-provider/03-cloud-resource-quota-troubleshooting.md)
- [AI/ML 工作负载故障排查指南](./topic-structural-trouble-shooting/10-ai-ml-workloads/01-ai-ml-workloads-troubleshooting.md)
- [Kubeflow 平台故障排查指南](./topic-structural-trouble-shooting/10-ai-ml-workloads/02-kubeflow-troubleshooting.md)
- [MPI Operator 与分布式训练故障排查指南](./topic-structural-trouble-shooting/10-ai-ml-workloads/03-mpi-operator-troubleshooting.md)
- [Flux 镜像自动化故障排查指南](./topic-structural-trouble-shooting/11-gitops-devops/03-flux-image-automation-troubleshooting.md)

## FTA 故障树

- [DNS 异常 FTA 树](./topic-fta/list/dns-fta.md)

## 技能卡片

- [DNS 解析故障诊断与修复 / DNS Resolution Failure Diagnosis & Remediation](./topic-skills/04-dns-resolution-failure.md)

## 术语词典

- [ConfigMaps](./topic-dictionary/configuration/configmaps.md)
- [注解](./topic-dictionary/fundamentals/annotations.md)
- [Kubernetes 组件](./topic-dictionary/fundamentals/kubernetes-components.md)
- [知识地图](./topic-dictionary/fundamentals/kubernetes-concepts-reference.md)
- [标签和选择器](./topic-dictionary/fundamentals/labels-and-selectors.md)
- [命名空间](./topic-dictionary/fundamentals/namespaces.md)
- [对象名称和 ID](./topic-dictionary/fundamentals/object-names-and-ids.md)
- [边缘计算与轻量级 Kubernetes](./topic-dictionary/multi-cloud/edge-computing-and-k3s.md)
- [10 - 多云混合云运维手册](./topic-dictionary/multi-cloud/multi-cloud-operations.md)
- [多集群网络互联（Cluster Mesh）](./topic-dictionary/networking/cluster-mesh.md)
- [DNS for Services and Pods](./topic-dictionary/networking/dns-for-services-and-pods.md)
- [eBPF 与 Cilium 网络](./topic-dictionary/networking/ebpf-and-cilium-networking.md)
- [EndpointSlices](./topic-dictionary/networking/endpointslices.md)
- [Ingress](./topic-dictionary/networking/ingress.md)
- [Network Policies](./topic-dictionary/networking/network-policies.md)
- [Networking on Windows](./topic-dictionary/networking/networking-on-windows.md)
- [Service ClusterIP allocation](./topic-dictionary/networking/service-clusterip-allocation.md)
- [Service](./topic-dictionary/networking/service.md)
- [Topology Aware Routing](./topic-dictionary/networking/topology-aware-routing.md)
- [Certificates（PKI 证书与要求）](./topic-dictionary/operations/certificates.md)
- [14 - 变更管理与发布策略](./topic-dictionary/operations/change-management-release.md)
- [混沌工程（Chaos Engineering）](./topic-dictionary/operations/chaos-engineering.md)
- [企业级运维最佳实践](./topic-dictionary/operations/enterprise-ops-practices.md)
- [02 - Kubernetes 故障模式与根因分析字典](./topic-dictionary/operations/failure-patterns-analysis.md)
- [FinOps 与成本优化](./topic-dictionary/operations/finops-and-cost-optimization.md)
- [GreenOps 与碳感知计算](./topic-dictionary/operations/greenops-and-carbon-aware-computing.md)
- [12 - 生产事故管理与应急手册](./topic-dictionary/operations/incident-management-runbooks.md)
- [安装插件（Installing Addons）](./topic-dictionary/operations/installing-addons.md)
- [01 - Kubernetes 生产环境运维最佳实践字典](./topic-dictionary/operations/operations-best-practices.md)
- [03 - Kubernetes 性能调优专家指南](./topic-dictionary/operations/performance-tuning-expert.md)
- [16 - 生产环境故障排查剧本](./topic-dictionary/operations/production-troubleshooting-playbook.md)
- [15 - SLI/SLO/SLA工程实践](./topic-dictionary/operations/sli-slo-sla-engineering.md)
- [04 - SRE运维成熟度模型](./topic-dictionary/operations/sre-maturity-model.md)
- [有状态服务运维](./topic-dictionary/operations/stateful-services-operations.md)
- [Assigning Pods to Nodes](./topic-dictionary/scheduling/assigning-pods-to-nodes.md)
- [Pod Priority and Preemption](./topic-dictionary/scheduling/pod-priority-and-preemption.md)
- [Pod Topology Spread Constraints](./topic-dictionary/scheduling/pod-topology-spread-constraints.md)
- [Scheduler Performance Tuning](./topic-dictionary/scheduling/scheduler-performance-tuning.md)
- [09 - 云原生安全专家指南](./topic-dictionary/security/cloud-native-security-practices.md)
- [多租户](./topic-dictionary/security/multi-tenancy.md)
- [08 - AI/ML基础设施专业词典](./topic-dictionary/specialized-workloads/ai-infra-specialist.md)
- [对象存储与数据流水线](./topic-dictionary/storage/object-storage-and-data-pipelines.md)
- [知识地图](./topic-dictionary/tooling/cli-commands.md)
- [容器镜像优化](./topic-dictionary/tooling/container-image-optimization.md)
- [Kusheet 工具与开源项目 URL 汇总](./topic-dictionary/tooling/tool-ecosystem.md)
- [Advanced Pod Configuration](./topic-dictionary/workloads/advanced-pod-configuration.md)
- [Autoscaling Workloads](./topic-dictionary/workloads/autoscaling-workloads.md)
- [容器环境（Container Environment）](./topic-dictionary/workloads/container-environment.md)
- [CronJob](./topic-dictionary/workloads/cronjob.md)
- [DaemonSet](./topic-dictionary/workloads/daemonset.md)
- [Deployments](./topic-dictionary/workloads/deployments.md)
- [Ephemeral Containers](./topic-dictionary/workloads/ephemeral-containers.md)
- [Init Containers](./topic-dictionary/workloads/init-containers.md)
- [Pod Hostname](./topic-dictionary/workloads/pod-hostname.md)
- [Pods](./topic-dictionary/workloads/pods.md)
- [运行时类（RuntimeClass）](./topic-dictionary/workloads/runtime-class.md)
- [Spot 与可抢占工作负载](./topic-dictionary/workloads/spot-and-preemptible-workloads.md)
- [StatefulSets](./topic-dictionary/workloads/statefulsets.md)

## Docker

- [Docker 容器生命周期管理](./domain-13-docker/03-docker-container-lifecycle.md)
- [Docker 网络深度解析](./domain-13-docker/04-docker-networking-deep-dive.md)
- [Docker Compose 编排](./domain-13-docker/06-docker-compose-orchestration.md)
- [Docker 故障排查指南](./domain-13-docker/08-docker-troubleshooting-guide.md)
- [Java 应用容器化最佳实践指南](./domain-13-docker/12-java-containerization-guide.md)

## Linux 基础

- [01 - Linux 系统架构与内核深度解析：生产环境运维专家指南](./domain-14-linux/01-linux-system-architecture.md)
- [04 - Linux 网络配置与性能优化：生产环境网络运维专家指南](./domain-14-linux/04-linux-networking-configuration.md)
- [09 - Linux 运维基础与应急响应：生产环境运维专家实践指南](./domain-14-linux/09-linux-operations-basics.md)
- [Linux 命令大全参考](./domain-14-linux/99-linux-commands-reference.md)

## 网络基础

- [Domain-15 网络基础 — 开源项目索引](./domain-15-network-fundamentals/00-open-source-projects-index.md)
- [网络协议栈详解](./domain-15-network-fundamentals/01-network-protocols-stack.md)
- [TCP/UDP 协议深度解析](./domain-15-network-fundamentals/02-tcp-udp-deep-dive.md)
- [DNS 原理与配置](./domain-15-network-fundamentals/03-dns-principles-configuration.md)
- [负载均衡技术](./domain-15-network-fundamentals/04-load-balancing-technologies.md)
- [网络安全基础](./domain-15-network-fundamentals/05-network-security-fundamentals.md)
- [SDN 与网络虚拟化](./domain-15-network-fundamentals/06-sdn-network-virtualization.md)
- [Cilium eBPF 网络与安全实践指南](./domain-15-network-fundamentals/99-cilium-ebpf-network-guide.md)

## 存储基础

- [05 - 企业级存储管理与运维实践](./domain-16-storage-fundamentals/05-storage-management-operations.md)

## 云服务商

- [Domain-17 云厂商 — 开源项目索引](./domain-17-cloud-provider/00-open-source-projects-index.md)
- [AWS EKS (Elastic Kubernetes Service) 概述](./domain-17-cloud-provider/01-aws-eks/aws-eks-overview.md)
- [Google Cloud GKE (Google Kubernetes Engine) 概述](./domain-17-cloud-provider/02-google-cloud-gke/google-cloud-gke-overview.md)
- [Azure AKS (Azure Kubernetes Service) 概述](./domain-17-cloud-provider/03-azure-aks/azure-aks-overview.md)
- [阿里云 ACK (Alibaba Cloud Container Service for Kubernetes) 概述](./domain-17-cloud-provider/04-alicloud-ack/alicloud-ack-overview.md)
- [Kubernetes Service ACK 实战指南](./domain-17-cloud-provider/04-alicloud-ack/service-ack-practical-guide.md)
- [腾讯云 TKE (Tencent Kubernetes Engine) 概述](./domain-17-cloud-provider/05-tencent-tke/tencent-tke-overview.md)
- [火山引擎 VEK (Volcengine Kubernetes) 字节级深度实战指南](./domain-17-cloud-provider/10-volcengine-vek/volcengine-vek-overview.md)
- [天翼云 TKE (Tianyi Cloud Kubernetes Engine) 概述](./domain-17-cloud-provider/11-ctyun-tke/ctyun-tke-overview.md)
- [移动云 CKE (China Mobile Cloud Kubernetes Engine) 企业级深度实战指南](./domain-17-cloud-provider/12-ecloud-cke/ecloud-cke-overview.md)
- [阿里云专有版 ACK (Apsara Stack ACK) 金融级深度解析](./domain-17-cloud-provider/13-alicloud-apsara-ack/alicloud-apsara-ack-overview.md)

## 生产运维

- [Domain-18 生产运维 — 开源项目索引](./domain-18-production-operations/00-open-source-projects-index.md)
- [01-生产架构设计原则](./domain-18-production-operations/01-production-architecture-design-principles.md)
- [02-多云混合部署策略](./domain-18-production-operations/02-multi-cloud-hybrid-deployment-strategy.md)
- [04-企业级监控体系](./domain-18-production-operations/04-enterprise-monitoring-system.md)
- [06-APM应用性能监控](./domain-18-production-operations/06-apm-application-performance-monitoring.md)
- [09-软件物料清单](./domain-18-production-operations/09-software-bill-of-materials.md)
- [11-基础设施即代码](./domain-18-production-operations/11-infrastructure-as-code.md)
- [18-跨区域容灾部署](./domain-18-production-operations/18-cross-region-disaster-recovery.md)
- [20-网络性能优化](./domain-18-production-operations/20-network-performance-optimization.md)
- [GreenOps 可持续计算与碳足迹优化指南](./domain-18-production-operations/99-greenops-sustainable-computing-guide.md)
- [Karpenter 节点自动扩展实践指南](./domain-18-production-operations/99-karpenter-node-autoscaling-guide.md)
- [KEDA 事件驱动自动缩放实践指南](./domain-18-production-operations/99-keda-event-driven-autoscaling-guide.md)
- [Kubernetes 生产环境完整架构蓝图](./domain-18-production-operations/99-kubernetes-production-architecture-blueprint.md)

## 技术论文

- [Kubernetes 大规模集群性能优化深度实践 (Large-Scale Cluster Performance Optimization)](./domain-19-papers/02-kubernetes-large-scale-performance-optimization.md)
- [Kubernetes 安全零信任架构实施指南 (Zero Trust Security Architecture Implementation)](./domain-19-papers/03-kubernetes-zero-trust-security-architecture.md)
- [Kubernetes 多云混合部署架构与实践 (Multi-Cloud Hybrid Deployment Architecture)](./domain-19-papers/04-kubernetes-multi-cloud-hybrid-deployment.md)
- [Kubernetes 网络策略与安全微隔离实践 (Network Policies and Security Micro-Segmentation Practice)](./domain-19-papers/08-kubernetes-network-policies-security-micro-segmentation.md)
- [Kubernetes 服务网格深度实践与Istio集成 (Service Mesh Deep Practice and Istio Integration)](./domain-19-papers/09-kubernetes-service-mesh-istio-integration.md)
- [Kubernetes 调度器深度优化与自定义调度 (Scheduler Deep Optimization and Custom Scheduling)](./domain-19-papers/12-kubernetes-scheduler-deep-optimization-custom-scheduling.md)
- [Kubernetes 多租户安全隔离与资源配额管理 (Multi-Tenancy Security Isolation and Resource Quota Management)](./domain-19-papers/13-kubernetes-multi-tenancy-security-isolation-resource-quota.md)
- [Kubernetes AI/ML GPU调度与LLM推理服务 (AI/ML GPU Scheduling and LLM Inference Serving)](./domain-19-papers/17-kubernetes-aiml-gpu-scheduling-llm-inference.md)
- [Kubernetes eBPF与Cilium深度实践 (eBPF and Cilium Deep Practice)](./domain-19-papers/18-kubernetes-ebpf-cilium-deep-practice.md)
- [Kubernetes Gateway API 与现代流量管理实践](./domain-19-papers/19-kubernetes-gateway-api-modern-traffic-management.md)
- [Kubernetes 供应链安全实践 (Supply Chain Security: SBOM, SLSA, and Sigstore)](./domain-19-papers/20-kubernetes-supply-chain-security-sbom-slsa-sigstore.md)
- [Kubernetes 平台工程与内部开发者平台 (Platform Engineering and Internal Developer Platform)](./domain-19-papers/21-kubernetes-platform-engineering-internal-developer-platform.md)
- [Kubernetes WebAssembly (Wasm) 工作负载实践 (WebAssembly Workloads on Kubernetes)](./domain-19-papers/22-kubernetes-webassembly-wasm-workloads.md)
- [Kubernetes OpenTelemetry 原生可观测性 (OpenTelemetry Native Observability)](./domain-19-papers/23-kubernetes-opentelemetry-native-observability.md)
- [Kubernetes 策略即代码与治理自动化 (Policy-as-Code and Governance Automation)](./domain-19-papers/24-kubernetes-policy-as-code-governance-automation.md)
- [GKE Autopilot 与 Google Cloud AI 基础设施 (GKE Autopilot and Google Cloud AI Infrastructure)](./domain-19-papers/25-gke-autopilot-google-cloud-ai-infrastructure.md)
- [Kubernetes vCluster 与虚拟集群多租户 (vCluster and Virtual Cluster Multi-Tenancy)](./domain-19-papers/26-kubernetes-vcluster-virtual-cluster-multi-tenancy.md)

## CNCF 生态

- [cert-manager](./domain-34-cncf-landscape/graduated/cert-manager/cert-manager.md)
- [CoreDNS](./domain-34-cncf-landscape/graduated/coredns/coredns.md)
- [Dapr](./domain-34-cncf-landscape/graduated/dapr/dapr.md)
- [etcd](./domain-34-cncf-landscape/graduated/etcd/etcd.md)
- [KEDA](./domain-34-cncf-landscape/graduated/keda/keda.md)
- [KubeEdge](./domain-34-cncf-landscape/graduated/kubeedge/kubeedge.md)
- [Kubernetes](./domain-34-cncf-landscape/graduated/kubernetes/kubernetes.md)
- [SPIRE](./domain-34-cncf-landscape/graduated/spire/spire.md)
- [Vitess](./domain-34-cncf-landscape/graduated/vitess/vitess.md)
- [Artifact Hub](./domain-34-cncf-landscape/incubating/artifact-hub/artifact-hub.md)
- [gRPC](./domain-34-cncf-landscape/incubating/grpc/grpc.md)
- [OpenCost](./domain-34-cncf-landscape/incubating/opencost/opencost.md)
- [Strimzi](./domain-34-cncf-landscape/incubating/strimzi/strimzi.md)
- [Carvel](./domain-34-cncf-landscape/sandbox/carvel/carvel.md)
- [ChaosBlade](./domain-34-cncf-landscape/sandbox/chaosblade/chaosblade.md)
- [Eraser](./domain-34-cncf-landscape/sandbox/eraser/eraser.md)
- [Inclavare Containers](./domain-34-cncf-landscape/sandbox/inclavare-containers/inclavare-containers.md)
- [Inspektor Gadget](./domain-34-cncf-landscape/sandbox/inspektor-gadget/inspektor-gadget.md)
- [k3s](./domain-34-cncf-landscape/sandbox/k3s/k3s.md)
- [K8GB (Kubernetes Global Balancer)](./domain-34-cncf-landscape/sandbox/k8gb/k8gb.md)
- [Kairos](./domain-34-cncf-landscape/sandbox/kairos/kairos.md)
- [ko](./domain-34-cncf-landscape/sandbox/ko/ko.md)
- [Kuadrant](./domain-34-cncf-landscape/sandbox/kuadrant/kuadrant.md)
- [Kube-burner](./domain-34-cncf-landscape/sandbox/kube-burner/kube-burner.md)
- [Kuberhealthy](./domain-34-cncf-landscape/sandbox/kuberhealthy/kuberhealthy.md)
- [KubeSlice](./domain-34-cncf-landscape/sandbox/kubeslice/kubeslice.md)
- [Kuma](./domain-34-cncf-landscape/sandbox/kuma/kuma.md)
- [Kured](./domain-34-cncf-landscape/sandbox/kured/kured.md)
- [Network Service Mesh (NSM)](./domain-34-cncf-landscape/sandbox/network-service-mesh/network-service-mesh.md)
- [OAuth2 Proxy](./domain-34-cncf-landscape/sandbox/oauth2-proxy/oauth2-proxy.md)
- [OpenFunction](./domain-34-cncf-landscape/sandbox/openfunction/openfunction.md)
- [Parsec (Platform AbstRaction for SECurity)](./domain-34-cncf-landscape/sandbox/parsec/parsec.md)
- [Piraeus Datastore](./domain-34-cncf-landscape/sandbox/piraeus-datastore/piraeus-datastore.md)
- [Pixie](./domain-34-cncf-landscape/sandbox/pixie/pixie.md)
- [Submariner](./domain-34-cncf-landscape/sandbox/submariner/submariner.md)
- [Telepresence](./domain-34-cncf-landscape/sandbox/telepresence/telepresence.md)
- [WasmEdge](./domain-34-cncf-landscape/sandbox/wasmedge/wasmedge.md)

## 培训学习

- [P1: ACK 集群生命周期管理](./topic-learn/inner-training/projects/p1-ack-cluster-lifecycle.md)
- [P3: 节点与工作负载管理实践](./topic-learn/inner-training/projects/p3-node-workload-management.md)
- [P4: 网络与存储综合实践](./topic-learn/inner-training/projects/p4-network-storage-practice.md)
- [P5: 毕业综合项目](./topic-learn/inner-training/projects/p5-graduation-project.md)
- [ACK/ACR/K8S 命令速查表](./topic-learn/inner-training/resources/commands-cheatsheet.md)
- [ACK/ACR/K8S 内部培训知识图谱](./topic-learn/inner-training/resources/knowledge-map.md)
- [Week 1 Checkpoint: 自测检验](./topic-learn/inner-training/week-1-ack-acr-lifecycle/checkpoint.md)
- [Day 1: ACK/ACR 管控 SR](./topic-learn/inner-training/week-1-ack-acr-lifecycle/day-1-ack-acr-sr.md)
- [Day 2: ACK SDK & API](./topic-learn/inner-training/week-1-ack-acr-lifecycle/day-2-ack-sdk-api.md)
- [Day 4: K8S 新建集群](./topic-learn/inner-training/week-1-ack-acr-lifecycle/day-4-cluster-creation.md)
- [Week 3 自测: 节点与工作负载管理](./topic-learn/inner-training/week-3-node-workload/checkpoint.md)
- [Day 21: K8S 组件运维](./topic-learn/inner-training/week-3-node-workload/day-21-component-ops.md)
- [Day 22: Service 基础](./topic-learn/inner-training/week-4-network-storage/day-22-service-basics.md)
- [Day 25: Flannel 网络](./topic-learn/inner-training/week-4-network-storage/day-25-flannel-cni.md)
- [🔥 Kubernetes 生产运维实战训练营 🔥](./topic-learn/public-training/one-month/public-one-month-training.md)
- [知识图谱模板](./topic-learn/public-training/one-month/resources/knowledge-map.md)
- [文档阅读顺序索引](./topic-learn/public-training/one-month/resources/reading-sequence.md)
- [Day 4: Linux 网络 + 性能调优](./topic-learn/public-training/one-month/week-1-foundation/day-4-linux-network.md)
- [Day 7: 周复习 + 综合实践](./topic-learn/public-training/one-month/week-1-foundation/day-7-review-practice.md)
- [Week 2 Checkpoint: 自测检验](./topic-learn/public-training/one-month/week-2-core-tech/checkpoint.md)
- [Day 10: 工作负载 - Deployment + StatefulSet + DaemonSet](./topic-learn/public-training/one-month/week-2-core-tech/day-10-workloads-1.md)
- [Day 11: 工作负载 - Pod 生命周期 + 资源管理 + HPA](./topic-learn/public-training/one-month/week-2-core-tech/day-11-workloads-2.md)
- [Day 12: 网络栈 - CNI + Service + DNS](./topic-learn/public-training/one-month/week-2-core-tech/day-12-networking-1.md)
- [Day 13: 网络栈 - Ingress + NetworkPolicy](./topic-learn/public-training/one-month/week-2-core-tech/day-13-networking-2.md)
- [Day 14: 存储体系 + 综合实践](./topic-learn/public-training/one-month/week-2-core-tech/day-14-storage-practice.md)
- [Day 18: 可观测性 - 日志 + 分布式追踪](./topic-learn/public-training/one-month/week-3-operations/day-18-observability-2.md)
- [Week 4 Checkpoint: 终极自测](./topic-learn/public-training/one-month/week-4-enterprise/checkpoint.md)
- [Day 26: FTA/FEBM 专题深化](./topic-learn/public-training/one-month/week-4-enterprise/day-26-fta-febm-deep.md)

## 演示文稿

- [Kubernetes CoreDNS 全栈进阶培训 (从入门到专家)](./topic-presentations/kubernetes-coredns-presentation.md)
