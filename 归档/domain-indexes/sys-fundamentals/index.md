---
title: Domain 17 内容索引
summary: Domain 17 内容索引
category: 系统基础
tags:
- index
- 系统基础
- navigation
tier: supporting
sources:
- auto-generated
created: 2026-05-21
updated: 2026-05-21
last_updated: 2026-05-21
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Domain 17 内容索引

> 本索引汇总了 系统基础 下的所有文档，按主题分组。

## 概述
- [[README]] — Domain 总览

## 按主题分组

### Linux 系统

- [[01-linux-system-architecture]] — Linux system architecture
- [[02-linux-process-management]] — Linux process management
- [[03-linux-filesystem-deep-dive]] — Linux filesystem deep dive
- [[04-linux-networking-configuration]] — Linux networking configuration
- [[05-linux-storage-management]] — Linux storage management
- [[06-linux-performance-tuning]] — Linux performance tuning
- [[07-linux-security-hardening]] — Linux security hardening
- [[08-linux-container-fundamentals]] — Linux container fundamentals
- [[09-linux-operations-basics]] — Linux operations basics
- [[99-linux-commands-reference]] — Linux commands reference

### 硬件技术

- [[01-cloud-hardware-architecture]] — Cloud hardware architecture
- [[02-server-architecture-principles]] — Server architecture principles
- [[03-cpu-technology-deep-dive]] — Cpu technology deep dive
- [[04-motherboard-chipset-technology]] — Motherboard chipset technology
- [[05-memory-technology-deep-dive]] — Memory technology deep dive
- [[06-storage-hdd-technology]] — Storage hdd technology
- [[07-storage-ssd-technology]] — Storage ssd technology
- [[08-network-hardware-technology]] — Network hardware technology
- [[09-hardware-vendors-ecosystem]] — Hardware vendors ecosystem
- [[10-hardware-troubleshooting-methodology]] — Hardware troubleshooting methodology
- [[11-cpu-memory-troubleshooting]] — Cpu memory troubleshooting
- [[12-storage-troubleshooting]] — Storage troubleshooting
- [[13-network-hardware-troubleshooting]] — Network hardware troubleshooting
- [[14-power-thermal-troubleshooting]] — Power thermal troubleshooting
- [[15-bios-firmware-troubleshooting]] — Bios firmware troubleshooting
- [[16-kubernetes-hardware-troubleshooting]] — Kubernetes hardware troubleshooting
- [[17-hardware-error-codes-reference]] — Hardware error codes reference
- [[18-hardware-failure-case-studies]] — Hardware failure case studies

### K8s 事件系统

- [[01-event-system-architecture]] — Event system architecture
- [[02-pod-container-lifecycle-events]] — Pod container lifecycle events
- [[03-image-pull-events]] — Image pull events
- [[04-probe-health-check-events]] — Probe health check events
- [[05-scheduling-preemption-events]] — Scheduling preemption events
- [[06-node-lifecycle-condition-events]] — Node lifecycle condition events
- [[07-deployment-replicaset-events]] — Deployment replicaset events
- [[08-statefulset-daemonset-events]] — Statefulset daemonset events
- [[09-job-cronjob-batch-events]] — Job cronjob batch events
- [[10-service-networking-events]] — Service networking events
- [[11-storage-volume-events]] — Storage volume events
- [[12-autoscaling-events]] — Autoscaling events
- [[13-security-admission-rbac-events]] — Security admission rbac events
- [[14-namespace-resource-gc-events]] — Namespace resource gc events
- [[15-ecosystem-addon-events]] — Ecosystem addon events

### 98 Merged Indexes

- [[00-open-source-projects-index-from-domain-14]] — Open source projects index from domain 14
- [[00-open-source-projects-index-from-domain-31]] — Open source projects index from domain 31
- [[00-open-source-projects-index-from-domain-33]] — Open source projects index from domain 33
- [[MOC-from-domain-14]] — MOC from domain 14
- [[MOC-from-domain-31]] — MOC from domain 31
- [[MOC-from-domain-33]] — MOC from domain 33
- [[README-from-domain-14]] — README from domain 14
- [[README-from-domain-31]] — README from domain 31
- [[README-from-domain-33]] — README from domain 33

### 速查手册

- [[MOC]] — MOC
- [[README]] — README
- [[docker]] — Docker
- [[gateway-api]] — Gateway api
- [[git]] — Git
- [[gitops]] — Gitops
- [[go]] — Go
- [[helm]] — Helm
- [[k8s]] — K8s
- [[kubectl-scene-cheatsheet]] — Kubectl scene cheatsheet
- [[linux]] — Linux
- [[networking]] — Networking
- [[promql]] — Promql
- [[sql]] — Sql
- [[tls-pki]] — Tls pki

### 术语词典

#### Topic Dictionary

- [[GAP-ANALYSIS]] — GAP ANALYSIS
- [[MOC]] — MOC
- [[README]] — README
- [[k8s-glossary]] — K8s glossary

#### 配置管理

- [[系统基础/topic-dictionary/configuration/configmaps.md|configmaps]] — Configmaps
- [[系统基础/topic-dictionary/configuration/liveness-readiness-and-startup-probes.md|liveness-readiness-and-startup-probes]] — Liveness readiness and startup probes
- [[系统基础/topic-dictionary/configuration/organizing-cluster-access-using-kubeconfig-files.md|organizing-cluster-access-using-kubeconfig-files]] — Organizing cluster access using kubeconfig files
- [[系统基础/topic-dictionary/configuration/resource-management-for-pods-and-containers.md|resource-management-for-pods-and-containers]] — Resource management for pods and containers
- [[系统基础/topic-dictionary/configuration/resource-management-for-windows-nodes.md|resource-management-for-windows-nodes]] — Resource management for windows nodes
- [[系统基础/topic-dictionary/configuration/secrets.md|secrets]] — Secrets

#### 基础概念

- [[系统基础/topic-dictionary/fundamentals/about-cgroup-v2.md|about-cgroup-v2]] — About cgroup v2
- [[系统基础/topic-dictionary/fundamentals/annotations.md|annotations]] — Annotations
- [[系统基础/topic-dictionary/fundamentals/cloud-controller-manager.md|cloud-controller-manager]] — Cloud controller manager
- [[系统基础/topic-dictionary/fundamentals/communication-between-nodes-and-the-control-plane.md|communication-between-nodes-and-the-control-plane]] — Communication between nodes and the control plane
- [[系统基础/topic-dictionary/fundamentals/controllers.md|controllers]] — Controllers
- [[系统基础/topic-dictionary/fundamentals/field-selectors.md|field-selectors]] — Field selectors
- [[系统基础/topic-dictionary/fundamentals/finalizers.md|finalizers]] — Finalizers
- [[系统基础/topic-dictionary/fundamentals/garbage-collection.md|garbage-collection]] — Garbage collection
- [[系统基础/topic-dictionary/fundamentals/kubernetes-components.md|kubernetes-components]] — Kubernetes components
- [[系统基础/topic-dictionary/fundamentals/kubernetes-concepts-reference.md|kubernetes-concepts-reference]] — Kubernetes concepts reference
- [[系统基础/topic-dictionary/fundamentals/kubernetes-object-management.md|kubernetes-object-management]] — Kubernetes object management
- [[系统基础/topic-dictionary/fundamentals/kubernetes-self-healing.md|kubernetes-self-healing]] — Kubernetes self healing
- [[系统基础/topic-dictionary/fundamentals/labels-and-selectors.md|labels-and-selectors]] — Labels and selectors
- [[系统基础/topic-dictionary/fundamentals/leases.md|leases]] — Leases
- [[系统基础/topic-dictionary/fundamentals/mixed-version-proxy.md|mixed-version-proxy]] — Mixed version proxy
- [[系统基础/topic-dictionary/fundamentals/namespaces.md|namespaces]] — Namespaces
- [[系统基础/topic-dictionary/fundamentals/nodes.md|nodes]] — Nodes
- [[系统基础/topic-dictionary/fundamentals/object-names-and-ids.md|object-names-and-ids]] — Object names and ids
- [[系统基础/topic-dictionary/fundamentals/objects-in-kubernetes.md|objects-in-kubernetes]] — Objects in kubernetes
- [[系统基础/topic-dictionary/fundamentals/owners-and-dependents.md|owners-and-dependents]] — Owners and dependents
- [[系统基础/topic-dictionary/fundamentals/recommended-labels.md|recommended-labels]] — Recommended labels
- [[系统基础/topic-dictionary/fundamentals/storage-versions.md|storage-versions]] — Storage versions
- [[系统基础/topic-dictionary/fundamentals/the-kubectl-command-line-tool.md|the-kubectl-command-line-tool]] — The kubectl command line tool
- [[系统基础/topic-dictionary/fundamentals/the-kubernetes-api.md|the-kubernetes-api]] — The kubernetes api

#### 多云与边缘

- [[系统基础/topic-dictionary/multi-cloud/edge-computing-and-k3s.md|edge-computing-and-k3s]] — Edge computing and k3s
- [[系统基础/topic-dictionary/multi-cloud/multi-cloud-operations.md|multi-cloud-operations]] — Multi cloud operations
- [[系统基础/topic-dictionary/multi-cloud/spaceborne-computing.md|spaceborne-computing]] — Spaceborne computing

#### 网络

- [[系统基础/topic-dictionary/networking/cluster-mesh.md|cluster-mesh]] — Cluster mesh
- [[系统基础/topic-dictionary/networking/cluster-networking.md|cluster-networking]] — Cluster networking
- [[系统基础/topic-dictionary/networking/dns-for-services-and-pods.md|dns-for-services-and-pods]] — Dns for services and pods
- [[系统基础/topic-dictionary/networking/ebpf-and-cilium-networking.md|ebpf-and-cilium-networking]] — Ebpf and cilium networking
- [[系统基础/topic-dictionary/networking/endpointslices.md|endpointslices]] — Endpointslices
- [[系统基础/topic-dictionary/networking/gateway-api.md|gateway-api]] — Gateway api
- [[系统基础/topic-dictionary/networking/ingress.md|ingress]] — Ingress
- [[系统基础/topic-dictionary/networking/ingress-controllers.md|ingress-controllers]] — Ingress controllers
- [[系统基础/topic-dictionary/networking/ipv4-ipv6-dual-stack.md|ipv4-ipv6-dual-stack]] — Ipv4 ipv6 dual stack
- [[系统基础/topic-dictionary/networking/network-policies.md|network-policies]] — Network policies
- [[系统基础/topic-dictionary/networking/networking-on-windows.md|networking-on-windows]] — Networking on windows
- [[系统基础/topic-dictionary/networking/service.md|service]] — Service
- [[系统基础/topic-dictionary/networking/service-clusterip-allocation.md|service-clusterip-allocation]] — Service clusterip allocation
- [[系统基础/topic-dictionary/networking/service-internal-traffic-policy.md|service-internal-traffic-policy]] — Service internal traffic policy
- [[系统基础/topic-dictionary/networking/service-mesh.md|service-mesh]] — Service mesh
- [[系统基础/topic-dictionary/networking/telco-cloud-and-5g-mec.md|telco-cloud-and-5g-mec]] — Telco cloud and 5g mec
- [[系统基础/topic-dictionary/networking/topology-aware-routing.md|topology-aware-routing]] — Topology aware routing

#### 可观测性

- [[系统基础/topic-dictionary/observability/alerting-and-slo-monitoring.md|alerting-and-slo-monitoring]] — Alerting and slo monitoring
- [[系统基础/topic-dictionary/observability/llm-observability.md|llm-observability]] — Llm observability
- [[系统基础/topic-dictionary/observability/log-aggregation-with-loki.md|log-aggregation-with-loki]] — Log aggregation with loki
- [[系统基础/topic-dictionary/observability/logging-architecture.md|logging-architecture]] — Logging architecture
- [[系统基础/topic-dictionary/observability/metrics-for-kubernetes-object-states.md|metrics-for-kubernetes-object-states]] — Metrics for kubernetes object states
- [[系统基础/topic-dictionary/observability/metrics-for-kubernetes-system-components.md|metrics-for-kubernetes-system-components]] — Metrics for kubernetes system components
- [[系统基础/topic-dictionary/observability/observability.md|observability]] — Observability
- [[系统基础/topic-dictionary/observability/opentelemetry-and-distributed-tracing.md|opentelemetry-and-distributed-tracing]] — Opentelemetry and distributed tracing
- [[系统基础/topic-dictionary/observability/system-logs.md|system-logs]] — System logs
- [[系统基础/topic-dictionary/observability/traces-for-kubernetes-system-components.md|traces-for-kubernetes-system-components]] — Traces for kubernetes system components

#### 运维实践

- [[系统基础/topic-dictionary/operations/backup-disaster-recovery.md|backup-disaster-recovery]] — Backup disaster recovery
- [[系统基础/topic-dictionary/operations/capacity-planning-forecasting.md|capacity-planning-forecasting]] — Capacity planning forecasting
- [[系统基础/topic-dictionary/operations/certificates.md|certificates]] — Certificates
- [[系统基础/topic-dictionary/operations/change-management-release.md|change-management-release]] — Change management release
- [[系统基础/topic-dictionary/operations/chaos-engineering.md|chaos-engineering]] — Chaos engineering
- [[系统基础/topic-dictionary/operations/enterprise-ops-practices.md|enterprise-ops-practices]] — Enterprise ops practices
- [[系统基础/topic-dictionary/operations/failure-patterns-analysis.md|failure-patterns-analysis]] — Failure patterns analysis
- [[系统基础/topic-dictionary/operations/finops-and-cost-optimization.md|finops-and-cost-optimization]] — Finops and cost optimization
- [[系统基础/topic-dictionary/operations/greenops-and-carbon-aware-computing.md|greenops-and-carbon-aware-computing]] — Greenops and carbon aware computing
- [[系统基础/topic-dictionary/operations/incident-management-runbooks.md|incident-management-runbooks]] — Incident management runbooks
- [[系统基础/topic-dictionary/operations/installing-addons.md|installing-addons]] — Installing addons
- [[系统基础/topic-dictionary/operations/node-autoscaling.md|node-autoscaling]] — Node autoscaling
- [[系统基础/topic-dictionary/operations/node-shutdowns.md|node-shutdowns]] — Node shutdowns
- [[系统基础/topic-dictionary/operations/operations-best-practices.md|operations-best-practices]] — Operations best practices
- [[系统基础/topic-dictionary/operations/performance-tuning-expert.md|performance-tuning-expert]] — Performance tuning expert
- [[系统基础/topic-dictionary/operations/production-troubleshooting-playbook.md|production-troubleshooting-playbook]] — Production troubleshooting playbook
- [[系统基础/topic-dictionary/operations/sli-slo-sla-engineering.md|sli-slo-sla-engineering]] — Sli slo sla engineering
- [[系统基础/topic-dictionary/operations/sre-maturity-model.md|sre-maturity-model]] — Sre maturity model
- [[系统基础/topic-dictionary/operations/stateful-services-operations.md|stateful-services-operations]] — Stateful services operations
- [[系统基础/topic-dictionary/operations/swap-memory-management.md|swap-memory-management]] — Swap memory management

#### 平台工程

- [[系统基础/topic-dictionary/platform-engineering/admission-webhook-good-practices.md|admission-webhook-good-practices]] — Admission webhook good practices
- [[系统基础/topic-dictionary/platform-engineering/api-priority-and-fairness.md|api-priority-and-fairness]] — Api priority and fairness
- [[系统基础/topic-dictionary/platform-engineering/cluster-api-and-fleet-management.md|cluster-api-and-fleet-management]] — Cluster api and fleet management
- [[系统基础/topic-dictionary/platform-engineering/compatibility-version-for-control-plane.md|compatibility-version-for-control-plane]] — Compatibility version for control plane
- [[系统基础/topic-dictionary/platform-engineering/compute-storage-and-networking-extensions.md|compute-storage-and-networking-extensions]] — Compute storage and networking extensions
- [[系统基础/topic-dictionary/platform-engineering/coordinated-leader-election.md|coordinated-leader-election]] — Coordinated leader election
- [[系统基础/topic-dictionary/platform-engineering/custom-resources.md|custom-resources]] — Custom resources
- [[系统基础/topic-dictionary/platform-engineering/developer-portal-and-platform-metrics.md|developer-portal-and-platform-metrics]] — Developer portal and platform metrics
- [[系统基础/topic-dictionary/platform-engineering/device-plugins.md|device-plugins]] — Device plugins
- [[系统基础/topic-dictionary/platform-engineering/dynamic-resource-allocation-good-practices.md|dynamic-resource-allocation-good-practices]] — Dynamic resource allocation good practices
- [[系统基础/topic-dictionary/platform-engineering/extending-the-kubernetes-api.md|extending-the-kubernetes-api]] — Extending the kubernetes api
- [[系统基础/topic-dictionary/platform-engineering/gitops-and-continuous-delivery.md|gitops-and-continuous-delivery]] — Gitops and continuous delivery
- [[系统基础/topic-dictionary/platform-engineering/infrastructure-as-code-for-kubernetes.md|infrastructure-as-code-for-kubernetes]] — Infrastructure as code for kubernetes
- [[系统基础/topic-dictionary/platform-engineering/kubernetes-api-aggregation-layer.md|kubernetes-api-aggregation-layer]] — Kubernetes api aggregation layer
- [[系统基础/topic-dictionary/platform-engineering/kubevirt-virtual-machines.md|kubevirt-virtual-machines]] — Kubevirt virtual machines
- [[系统基础/topic-dictionary/platform-engineering/network-plugins.md|network-plugins]] — Network plugins
- [[系统基础/topic-dictionary/platform-engineering/operator-pattern.md|operator-pattern]] — Operator pattern
- [[系统基础/topic-dictionary/platform-engineering/proxies-in-kubernetes.md|proxies-in-kubernetes]] — Proxies in kubernetes
- [[系统基础/topic-dictionary/platform-engineering/webassembly-wasm-workloads.md|webassembly-wasm-workloads]] — Webassembly wasm workloads

#### 调度

- [[系统基础/topic-dictionary/scheduling/api-initiated-eviction.md|api-initiated-eviction]] — Api initiated eviction
- [[系统基础/topic-dictionary/scheduling/assigning-pods-to-nodes.md|assigning-pods-to-nodes]] — Assigning pods to nodes
- [[系统基础/topic-dictionary/scheduling/dynamic-resource-allocation.md|dynamic-resource-allocation]] — Dynamic resource allocation
- [[系统基础/topic-dictionary/scheduling/gang-scheduling.md|gang-scheduling]] — Gang scheduling
- [[系统基础/topic-dictionary/scheduling/karpenter-autoscaling.md|karpenter-autoscaling]] — Karpenter autoscaling
- [[系统基础/topic-dictionary/scheduling/kubernetes-scheduler.md|kubernetes-scheduler]] — Kubernetes scheduler
- [[系统基础/topic-dictionary/scheduling/node-declared-features.md|node-declared-features]] — Node declared features
- [[系统基础/topic-dictionary/scheduling/node-pressure-eviction.md|node-pressure-eviction]] — Node pressure eviction
- [[系统基础/topic-dictionary/scheduling/pod-overhead.md|pod-overhead]] — Pod overhead
- [[系统基础/topic-dictionary/scheduling/pod-priority-and-preemption.md|pod-priority-and-preemption]] — Pod priority and preemption
- [[系统基础/topic-dictionary/scheduling/pod-scheduling-readiness.md|pod-scheduling-readiness]] — Pod scheduling readiness
- [[系统基础/topic-dictionary/scheduling/pod-topology-spread-constraints.md|pod-topology-spread-constraints]] — Pod topology spread constraints
- [[系统基础/topic-dictionary/scheduling/resource-bin-packing.md|resource-bin-packing]] — Resource bin packing
- [[系统基础/topic-dictionary/scheduling/scheduler-performance-tuning.md|scheduler-performance-tuning]] — Scheduler performance tuning
- [[系统基础/topic-dictionary/scheduling/scheduling-framework.md|scheduling-framework]] — Scheduling framework
- [[系统基础/topic-dictionary/scheduling/taints-and-tolerations.md|taints-and-tolerations]] — Taints and tolerations

#### 安全

- [[系统基础/topic-dictionary/security/application-security-checklist.md|application-security-checklist]] — Application security checklist
- [[系统基础/topic-dictionary/security/cloud-native-security.md|cloud-native-security]] — Cloud native security
- [[系统基础/topic-dictionary/security/cloud-native-security-practices.md|cloud-native-security-practices]] — Cloud native security practices
- [[系统基础/topic-dictionary/security/controlling-access-to-the-kubernetes-api.md|controlling-access-to-the-kubernetes-api]] — Controlling access to the kubernetes api
- [[系统基础/topic-dictionary/security/good-practices-for-kubernetes-secrets.md|good-practices-for-kubernetes-secrets]] — Good practices for kubernetes secrets
- [[系统基础/topic-dictionary/security/hardening-guide---authentication-mechanisms.md|hardening-guide---authentication-mechanisms]] — Hardening guide   authentication mechanisms
- [[系统基础/topic-dictionary/security/hardening-guide---scheduler-configuration.md|hardening-guide---scheduler-configuration]] — Hardening guide   scheduler configuration
- [[系统基础/topic-dictionary/security/kubernetes-api-server-bypass-risks.md|kubernetes-api-server-bypass-risks]] — Kubernetes api server bypass risks
- [[系统基础/topic-dictionary/security/limit-ranges.md|limit-ranges]] — Limit ranges
- [[系统基础/topic-dictionary/security/linux-kernel-security-constraints-for-pods-and-containers.md|linux-kernel-security-constraints-for-pods-and-containers]] — Linux kernel security constraints for pods and containers
- [[系统基础/topic-dictionary/security/multi-tenancy.md|multi-tenancy]] — Multi tenancy
- [[系统基础/topic-dictionary/security/node-resource-managers.md|node-resource-managers]] — Node resource managers
- [[系统基础/topic-dictionary/security/pod-security-admission.md|pod-security-admission]] — Pod security admission
- [[系统基础/topic-dictionary/security/pod-security-policies.md|pod-security-policies]] — Pod security policies
- [[系统基础/topic-dictionary/security/pod-security-standards.md|pod-security-standards]] — Pod security standards
- [[系统基础/topic-dictionary/security/policy-as-code.md|policy-as-code]] — Policy as code
- [[系统基础/topic-dictionary/security/process-id-limits-and-reservations.md|process-id-limits-and-reservations]] — Process id limits and reservations
- [[系统基础/topic-dictionary/security/resource-quotas.md|resource-quotas]] — Resource quotas
- [[系统基础/topic-dictionary/security/role-based-access-control-good-practices.md|role-based-access-control-good-practices]] — Role based access control good practices
- [[系统基础/topic-dictionary/security/runtime-security.md|runtime-security]] — Runtime security
- [[系统基础/topic-dictionary/security/secrets-management-deep-dive.md|secrets-management-deep-dive]] — Secrets management deep dive
- [[系统基础/topic-dictionary/security/security-checklist.md|security-checklist]] — Security checklist
- [[系统基础/topic-dictionary/security/security-for-linux-nodes.md|security-for-linux-nodes]] — Security for linux nodes
- [[系统基础/topic-dictionary/security/security-for-windows-nodes.md|security-for-windows-nodes]] — Security for windows nodes
- [[系统基础/topic-dictionary/security/service-accounts.md|service-accounts]] — Service accounts
- [[系统基础/topic-dictionary/security/spiffe-spire-identity.md|spiffe-spire-identity]] — Spiffe spire identity
- [[系统基础/topic-dictionary/security/supply-chain-security.md|supply-chain-security]] — Supply chain security

#### 专项工作负载

- [[系统基础/topic-dictionary/specialized-workloads/ai-infra-specialist.md|ai-infra-specialist]] — Ai infra specialist
- [[系统基础/topic-dictionary/specialized-workloads/gpu-resource-management-and-partitioning.md|gpu-resource-management-and-partitioning]] — Gpu resource management and partitioning
- [[系统基础/topic-dictionary/specialized-workloads/guide-for-running-windows-containers-in-kubernetes.md|guide-for-running-windows-containers-in-kubernetes]] — Guide for running windows containers in kubernetes
- [[系统基础/topic-dictionary/specialized-workloads/hpc-and-bioinformatics.md|hpc-and-bioinformatics]] — Hpc and bioinformatics
- [[系统基础/topic-dictionary/specialized-workloads/kserve-model-serving.md|kserve-model-serving]] — Kserve model serving
- [[系统基础/topic-dictionary/specialized-workloads/kueue-job-queue-management.md|kueue-job-queue-management]] — Kueue job queue management
- [[系统基础/topic-dictionary/specialized-workloads/llm-inference-optimization.md|llm-inference-optimization]] — Llm inference optimization
- [[系统基础/topic-dictionary/specialized-workloads/mlops-pipelines-and-model-registry.md|mlops-pipelines-and-model-registry]] — Mlops pipelines and model registry
- [[系统基础/topic-dictionary/specialized-workloads/vector-databases-and-rag-infrastructure.md|vector-databases-and-rag-infrastructure]] — Vector databases and rag infrastructure
- [[系统基础/topic-dictionary/specialized-workloads/windows-containers-in-kubernetes.md|windows-containers-in-kubernetes]] — Windows containers in kubernetes

#### 存储

- [[系统基础/topic-dictionary/storage/csi-volume-cloning.md|csi-volume-cloning]] — Csi volume cloning
- [[系统基础/topic-dictionary/storage/dynamic-volume-provisioning.md|dynamic-volume-provisioning]] — Dynamic volume provisioning
- [[系统基础/topic-dictionary/storage/ephemeral-volumes.md|ephemeral-volumes]] — Ephemeral volumes
- [[系统基础/topic-dictionary/storage/high-performance-storage-networks.md|high-performance-storage-networks]] — High performance storage networks
- [[系统基础/topic-dictionary/storage/local-ephemeral-storage.md|local-ephemeral-storage]] — Local ephemeral storage
- [[系统基础/topic-dictionary/storage/node-specific-volume-limits.md|node-specific-volume-limits]] — Node specific volume limits
- [[系统基础/topic-dictionary/storage/object-storage-and-data-pipelines.md|object-storage-and-data-pipelines]] — Object storage and data pipelines
- [[系统基础/topic-dictionary/storage/persistent-volumes.md|persistent-volumes]] — Persistent volumes
- [[系统基础/topic-dictionary/storage/projected-volumes.md|projected-volumes]] — Projected volumes
- [[系统基础/topic-dictionary/storage/storage-capacity.md|storage-capacity]] — Storage capacity
- [[系统基础/topic-dictionary/storage/storage-classes.md|storage-classes]] — Storage classes
- [[系统基础/topic-dictionary/storage/volume-attributes-classes.md|volume-attributes-classes]] — Volume attributes classes
- [[系统基础/topic-dictionary/storage/volume-health-monitoring.md|volume-health-monitoring]] — Volume health monitoring
- [[系统基础/topic-dictionary/storage/volume-snapshot-classes.md|volume-snapshot-classes]] — Volume snapshot classes
- [[系统基础/topic-dictionary/storage/volume-snapshots.md|volume-snapshots]] — Volume snapshots
- [[系统基础/topic-dictionary/storage/volumes.md|volumes]] — Volumes
- [[系统基础/topic-dictionary/storage/windows-storage.md|windows-storage]] — Windows storage

#### 工具链

- [[系统基础/topic-dictionary/tooling/cli-commands.md|cli-commands]] — Cli commands
- [[系统基础/topic-dictionary/tooling/container-image-optimization.md|container-image-optimization]] — Container image optimization
- [[系统基础/topic-dictionary/tooling/tool-ecosystem.md|tool-ecosystem]] — Tool ecosystem

#### 工作负载

- [[系统基础/topic-dictionary/workloads/advanced-pod-configuration.md|advanced-pod-configuration]] — Advanced pod configuration
- [[系统基础/topic-dictionary/workloads/automatic-cleanup-for-finished-jobs.md|automatic-cleanup-for-finished-jobs]] — Automatic cleanup for finished jobs
- [[系统基础/topic-dictionary/workloads/autoscaling-workloads.md|autoscaling-workloads]] — Autoscaling workloads
- [[系统基础/topic-dictionary/workloads/container-environment.md|container-environment]] — Container environment
- [[系统基础/topic-dictionary/workloads/container-lifecycle-hooks.md|container-lifecycle-hooks]] — Container lifecycle hooks
- [[系统基础/topic-dictionary/workloads/container-runtime-interface-cri.md|container-runtime-interface-cri]] — Container runtime interface cri
- [[系统基础/topic-dictionary/workloads/cronjob.md|cronjob]] — Cronjob
- [[系统基础/topic-dictionary/workloads/daemonset.md|daemonset]] — Daemonset
- [[系统基础/topic-dictionary/workloads/deployments.md|deployments]] — Deployments
- [[系统基础/topic-dictionary/workloads/disruptions.md|disruptions]] — Disruptions
- [[系统基础/topic-dictionary/workloads/downward-api.md|downward-api]] — Downward api
- [[系统基础/topic-dictionary/workloads/ephemeral-containers.md|ephemeral-containers]] — Ephemeral containers
- [[系统基础/topic-dictionary/workloads/horizontal-pod-autoscaling.md|horizontal-pod-autoscaling]] — Horizontal pod autoscaling
- [[系统基础/topic-dictionary/workloads/images.md|images]] — Images
- [[系统基础/topic-dictionary/workloads/init-containers.md|init-containers]] — Init containers
- [[系统基础/topic-dictionary/workloads/jobs.md|jobs]] — Jobs
- [[系统基础/topic-dictionary/workloads/managing-workloads.md|managing-workloads]] — Managing workloads
- [[系统基础/topic-dictionary/workloads/pod-group-policies.md|pod-group-policies]] — Pod group policies
- [[系统基础/topic-dictionary/workloads/pod-hostname.md|pod-hostname]] — Pod hostname
- [[系统基础/topic-dictionary/workloads/pod-lifecycle.md|pod-lifecycle]] — Pod lifecycle
- [[系统基础/topic-dictionary/workloads/pod-quality-of-service-classes.md|pod-quality-of-service-classes]] — Pod quality of service classes
- [[系统基础/topic-dictionary/workloads/pods.md|pods]] — Pods
- [[系统基础/topic-dictionary/workloads/replicaset.md|replicaset]] — Replicaset
- [[系统基础/topic-dictionary/workloads/replicationcontroller.md|replicationcontroller]] — Replicationcontroller
- [[系统基础/topic-dictionary/workloads/runtime-class.md|runtime-class]] — Runtime class
- [[系统基础/topic-dictionary/workloads/sidecar-containers.md|sidecar-containers]] — Sidecar containers
- [[系统基础/topic-dictionary/workloads/spot-and-preemptible-workloads.md|spot-and-preemptible-workloads]] — Spot and preemptible workloads
- [[系统基础/topic-dictionary/workloads/statefulsets.md|statefulsets]] — Statefulsets
- [[系统基础/topic-dictionary/workloads/user-namespaces.md|user-namespaces]] — User namespaces
- [[系统基础/topic-dictionary/workloads/vertical-pod-autoscaling.md|vertical-pod-autoscaling]] — Vertical pod autoscaling
- [[系统基础/topic-dictionary/workloads/workload-api.md|workload-api]] — Workload api
- [[系统基础/topic-dictionary/workloads/workload-management.md|workload-management]] — Workload management
- [[系统基础/topic-dictionary/workloads/workload-reference.md|workload-reference]] — Workload reference

## 相关 Domain
- [[集群基础/98-merged-indexes/index.md|Domain 01 集群基础 索引]]
- [[容器运行时/98-merged-indexes/index.md|Domain 13 容器运行时 索引]]


<!-- risk-assessed -->
