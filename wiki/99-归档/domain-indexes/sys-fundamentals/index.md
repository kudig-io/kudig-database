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
- [[03-linux-process-management]] — Linux process management
- [[04-linux-filesystem-deep-dive]] — Linux filesystem deep dive
- [[05-linux-networking-configuration]] — Linux networking configuration
- [[06-linux-storage-management]] — Linux storage management
- [[07-linux-performance-tuning]] — Linux performance tuning
- [[08-linux-security-hardening]] — Linux security hardening
- [[09-linux-container-fundamentals]] — Linux container fundamentals
- [[10-linux-operations-basics]] — Linux operations basics
- [[15-linux-commands-reference]] — Linux commands reference

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
- [[01-open-source-projects-index-from-domain-31]] — Open source projects index from domain 31
- [[02-open-source-projects-index-from-domain-33]] — Open source projects index from domain 33
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

- [[wiki/17-系统基础/06-知识字典/configuration/configmaps.md|configmaps]] — Configmaps
- [[wiki/17-系统基础/06-知识字典/configuration/liveness-readiness-and-startup-probes.md|liveness-readiness-and-startup-probes]] — Liveness readiness and startup probes
- [[wiki/17-系统基础/06-知识字典/configuration/organizing-cluster-access-using-kubeconfig-files.md|organizing-cluster-access-using-kubeconfig-files]] — Organizing cluster access using kubeconfig files
- [[wiki/17-系统基础/06-知识字典/configuration/resource-management-for-pods-and-containers.md|resource-management-for-pods-and-containers]] — Resource management for pods and containers
- [[wiki/17-系统基础/06-知识字典/configuration/resource-management-for-windows-nodes.md|resource-management-for-windows-nodes]] — Resource management for windows nodes
- [[wiki/17-系统基础/06-知识字典/configuration/secrets.md|secrets]] — Secrets

#### 基础概念

- [[wiki/17-系统基础/06-知识字典/fundamentals/about-cgroup-v2.md|about-cgroup-v2]] — About cgroup v2
- [[wiki/17-系统基础/06-知识字典/fundamentals/annotations.md|annotations]] — Annotations
- [[wiki/17-系统基础/06-知识字典/fundamentals/cloud-controller-manager.md|cloud-controller-manager]] — Cloud controller manager
- [[wiki/17-系统基础/06-知识字典/fundamentals/communication-between-nodes-and-the-control-plane.md|communication-between-nodes-and-the-control-plane]] — Communication between nodes and the control plane
- [[wiki/17-系统基础/06-知识字典/fundamentals/controllers.md|controllers]] — Controllers
- [[wiki/17-系统基础/06-知识字典/fundamentals/field-selectors.md|field-selectors]] — Field selectors
- [[wiki/17-系统基础/06-知识字典/fundamentals/finalizers.md|finalizers]] — Finalizers
- [[wiki/17-系统基础/06-知识字典/fundamentals/garbage-collection.md|garbage-collection]] — Garbage collection
- [[wiki/17-系统基础/06-知识字典/fundamentals/kubernetes-components.md|kubernetes-components]] — Kubernetes components
- [[wiki/17-系统基础/06-知识字典/fundamentals/kubernetes-concepts-reference.md|kubernetes-concepts-reference]] — Kubernetes concepts reference
- [[wiki/17-系统基础/06-知识字典/fundamentals/kubernetes-object-management.md|kubernetes-object-management]] — Kubernetes object management
- [[wiki/17-系统基础/06-知识字典/fundamentals/kubernetes-self-healing.md|kubernetes-self-healing]] — Kubernetes self healing
- [[wiki/17-系统基础/06-知识字典/fundamentals/labels-and-selectors.md|labels-and-selectors]] — Labels and selectors
- [[wiki/17-系统基础/06-知识字典/fundamentals/leases.md|leases]] — Leases
- [[wiki/17-系统基础/06-知识字典/fundamentals/mixed-version-proxy.md|mixed-version-proxy]] — Mixed version proxy
- [[wiki/17-系统基础/06-知识字典/fundamentals/namespaces.md|namespaces]] — Namespaces
- [[wiki/17-系统基础/06-知识字典/fundamentals/nodes.md|nodes]] — Nodes
- [[wiki/17-系统基础/06-知识字典/fundamentals/object-names-and-ids.md|object-names-and-ids]] — Object names and ids
- [[wiki/17-系统基础/06-知识字典/fundamentals/objects-in-kubernetes.md|objects-in-kubernetes]] — Objects in kubernetes
- [[wiki/17-系统基础/06-知识字典/fundamentals/owners-and-dependents.md|owners-and-dependents]] — Owners and dependents
- [[wiki/17-系统基础/06-知识字典/fundamentals/recommended-labels.md|recommended-labels]] — Recommended labels
- [[wiki/17-系统基础/06-知识字典/fundamentals/storage-versions.md|storage-versions]] — Storage versions
- [[wiki/17-系统基础/06-知识字典/fundamentals/the-kubectl-command-line-tool.md|the-kubectl-command-line-tool]] — The kubectl command line tool
- [[wiki/17-系统基础/06-知识字典/fundamentals/the-kubernetes-api.md|the-kubernetes-api]] — The kubernetes api

#### 多云与边缘

- [[wiki/17-系统基础/06-知识字典/multi-cloud/edge-computing-and-k3s.md|edge-computing-and-k3s]] — Edge computing and k3s
- [[wiki/17-系统基础/06-知识字典/multi-cloud/multi-cloud-operations.md|multi-cloud-operations]] — Multi cloud operations
- [[wiki/17-系统基础/06-知识字典/multi-cloud/spaceborne-computing.md|spaceborne-computing]] — Spaceborne computing

#### 网络

- [[wiki/17-系统基础/06-知识字典/networking/cluster-mesh.md|cluster-mesh]] — Cluster mesh
- [[wiki/17-系统基础/06-知识字典/networking/cluster-networking.md|cluster-networking]] — Cluster networking
- [[wiki/17-系统基础/06-知识字典/networking/dns-for-services-and-pods.md|dns-for-services-and-pods]] — Dns for services and pods
- [[wiki/17-系统基础/06-知识字典/networking/ebpf-and-cilium-networking.md|ebpf-and-cilium-networking]] — Ebpf and cilium networking
- [[wiki/17-系统基础/06-知识字典/networking/endpointslices.md|endpointslices]] — Endpointslices
- [[wiki/17-系统基础/06-知识字典/networking/gateway-api.md|gateway-api]] — Gateway api
- [[wiki/17-系统基础/06-知识字典/networking/ingress.md|ingress]] — Ingress
- [[wiki/17-系统基础/06-知识字典/networking/ingress-controllers.md|ingress-controllers]] — Ingress controllers
- [[wiki/17-系统基础/06-知识字典/networking/ipv4-ipv6-dual-stack.md|ipv4-ipv6-dual-stack]] — Ipv4 ipv6 dual stack
- [[wiki/17-系统基础/06-知识字典/networking/network-policies.md|network-policies]] — Network policies
- [[wiki/17-系统基础/06-知识字典/networking/networking-on-windows.md|networking-on-windows]] — Networking on windows
- [[wiki/17-系统基础/06-知识字典/networking/service.md|service]] — Service
- [[wiki/17-系统基础/06-知识字典/networking/service-clusterip-allocation.md|service-clusterip-allocation]] — Service clusterip allocation
- [[wiki/17-系统基础/06-知识字典/networking/service-internal-traffic-policy.md|service-internal-traffic-policy]] — Service internal traffic policy
- [[wiki/17-系统基础/06-知识字典/networking/service-mesh.md|service-mesh]] — Service mesh
- [[wiki/17-系统基础/06-知识字典/networking/telco-cloud-and-5g-mec.md|telco-cloud-and-5g-mec]] — Telco cloud and 5g mec
- [[wiki/17-系统基础/06-知识字典/networking/topology-aware-routing.md|topology-aware-routing]] — Topology aware routing

#### 可观测性

- [[wiki/17-系统基础/06-知识字典/observability/alerting-and-slo-monitoring.md|alerting-and-slo-monitoring]] — Alerting and slo monitoring
- [[wiki/17-系统基础/06-知识字典/observability/llm-observability.md|llm-observability]] — Llm observability
- [[wiki/17-系统基础/06-知识字典/observability/log-aggregation-with-loki.md|log-aggregation-with-loki]] — Log aggregation with loki
- [[wiki/17-系统基础/06-知识字典/observability/logging-architecture.md|logging-architecture]] — Logging architecture
- [[wiki/17-系统基础/06-知识字典/observability/metrics-for-kubernetes-object-states.md|metrics-for-kubernetes-object-states]] — Metrics for kubernetes object states
- [[wiki/17-系统基础/06-知识字典/observability/metrics-for-kubernetes-system-components.md|metrics-for-kubernetes-system-components]] — Metrics for kubernetes system components
- [[wiki/17-系统基础/06-知识字典/observability/observability.md|observability]] — Observability
- [[wiki/17-系统基础/06-知识字典/observability/opentelemetry-and-distributed-tracing.md|opentelemetry-and-distributed-tracing]] — Opentelemetry and distributed tracing
- [[wiki/17-系统基础/06-知识字典/observability/system-logs.md|system-logs]] — System logs
- [[wiki/17-系统基础/06-知识字典/observability/traces-for-kubernetes-system-components.md|traces-for-kubernetes-system-components]] — Traces for kubernetes system components

#### 运维实践

- [[wiki/17-系统基础/06-知识字典/operations/backup-disaster-recovery.md|backup-disaster-recovery]] — Backup disaster recovery
- [[wiki/17-系统基础/06-知识字典/operations/capacity-planning-forecasting.md|capacity-planning-forecasting]] — Capacity planning forecasting
- [[wiki/17-系统基础/06-知识字典/operations/certificates.md|certificates]] — Certificates
- [[wiki/17-系统基础/06-知识字典/operations/change-management-release.md|change-management-release]] — Change management release
- [[wiki/17-系统基础/06-知识字典/operations/chaos-engineering.md|chaos-engineering]] — Chaos engineering
- [[wiki/17-系统基础/06-知识字典/operations/enterprise-ops-practices.md|enterprise-ops-practices]] — Enterprise ops practices
- [[wiki/17-系统基础/06-知识字典/operations/failure-patterns-analysis.md|failure-patterns-analysis]] — Failure patterns analysis
- [[wiki/17-系统基础/06-知识字典/operations/finops-and-cost-optimization.md|finops-and-cost-optimization]] — Finops and cost optimization
- [[wiki/17-系统基础/06-知识字典/operations/greenops-and-carbon-aware-computing.md|greenops-and-carbon-aware-computing]] — Greenops and carbon aware computing
- [[wiki/17-系统基础/06-知识字典/operations/incident-management-runbooks.md|incident-management-runbooks]] — Incident management runbooks
- [[wiki/17-系统基础/06-知识字典/operations/installing-addons.md|installing-addons]] — Installing addons
- [[wiki/17-系统基础/06-知识字典/operations/node-autoscaling.md|node-autoscaling]] — Node autoscaling
- [[wiki/17-系统基础/06-知识字典/operations/node-shutdowns.md|node-shutdowns]] — Node shutdowns
- [[wiki/17-系统基础/06-知识字典/operations/operations-best-practices.md|operations-best-practices]] — Operations best practices
- [[wiki/17-系统基础/06-知识字典/operations/performance-tuning-expert.md|performance-tuning-expert]] — Performance tuning expert
- [[wiki/17-系统基础/06-知识字典/operations/production-troubleshooting-playbook.md|production-troubleshooting-playbook]] — Production troubleshooting playbook
- [[wiki/17-系统基础/06-知识字典/operations/sli-slo-sla-engineering.md|sli-slo-sla-engineering]] — Sli slo sla engineering
- [[wiki/17-系统基础/06-知识字典/operations/sre-maturity-model.md|sre-maturity-model]] — Sre maturity model
- [[wiki/17-系统基础/06-知识字典/operations/stateful-services-operations.md|stateful-services-operations]] — Stateful services operations
- [[wiki/17-系统基础/06-知识字典/operations/swap-memory-management.md|swap-memory-management]] — Swap memory management

#### 平台工程

- [[wiki/17-系统基础/06-知识字典/platform-engineering/admission-webhook-good-practices.md|admission-webhook-good-practices]] — Admission webhook good practices
- [[wiki/17-系统基础/06-知识字典/platform-engineering/api-priority-and-fairness.md|api-priority-and-fairness]] — Api priority and fairness
- [[wiki/17-系统基础/06-知识字典/platform-engineering/cluster-api-and-fleet-management.md|cluster-api-and-fleet-management]] — Cluster api and fleet management
- [[wiki/17-系统基础/06-知识字典/platform-engineering/compatibility-version-for-control-plane.md|compatibility-version-for-control-plane]] — Compatibility version for control plane
- [[wiki/17-系统基础/06-知识字典/platform-engineering/compute-storage-and-networking-extensions.md|compute-storage-and-networking-extensions]] — Compute storage and networking extensions
- [[wiki/17-系统基础/06-知识字典/platform-engineering/coordinated-leader-election.md|coordinated-leader-election]] — Coordinated leader election
- [[wiki/17-系统基础/06-知识字典/platform-engineering/custom-resources.md|custom-resources]] — Custom resources
- [[wiki/17-系统基础/06-知识字典/platform-engineering/developer-portal-and-platform-metrics.md|developer-portal-and-platform-metrics]] — Developer portal and platform metrics
- [[wiki/17-系统基础/06-知识字典/platform-engineering/device-plugins.md|device-plugins]] — Device plugins
- [[wiki/17-系统基础/06-知识字典/platform-engineering/dynamic-resource-allocation-good-practices.md|dynamic-resource-allocation-good-practices]] — Dynamic resource allocation good practices
- [[wiki/17-系统基础/06-知识字典/platform-engineering/extending-the-kubernetes-api.md|extending-the-kubernetes-api]] — Extending the kubernetes api
- [[wiki/17-系统基础/06-知识字典/platform-engineering/gitops-and-continuous-delivery.md|gitops-and-continuous-delivery]] — Gitops and continuous delivery
- [[wiki/17-系统基础/06-知识字典/platform-engineering/infrastructure-as-code-for-kubernetes.md|infrastructure-as-code-for-kubernetes]] — Infrastructure as code for kubernetes
- [[wiki/17-系统基础/06-知识字典/platform-engineering/kubernetes-api-aggregation-layer.md|kubernetes-api-aggregation-layer]] — Kubernetes api aggregation layer
- [[wiki/17-系统基础/06-知识字典/platform-engineering/kubevirt-virtual-machines.md|kubevirt-virtual-machines]] — Kubevirt virtual machines
- [[wiki/17-系统基础/06-知识字典/platform-engineering/network-plugins.md|network-plugins]] — Network plugins
- [[wiki/17-系统基础/06-知识字典/platform-engineering/operator-pattern.md|operator-pattern]] — Operator pattern
- [[wiki/17-系统基础/06-知识字典/platform-engineering/proxies-in-kubernetes.md|proxies-in-kubernetes]] — Proxies in kubernetes
- [[wiki/17-系统基础/06-知识字典/platform-engineering/webassembly-wasm-workloads.md|webassembly-wasm-workloads]] — Webassembly wasm workloads

#### 调度

- [[wiki/17-系统基础/06-知识字典/scheduling/api-initiated-eviction.md|api-initiated-eviction]] — Api initiated eviction
- [[wiki/17-系统基础/06-知识字典/scheduling/assigning-pods-to-nodes.md|assigning-pods-to-nodes]] — Assigning pods to nodes
- [[wiki/17-系统基础/06-知识字典/scheduling/dynamic-resource-allocation.md|dynamic-resource-allocation]] — Dynamic resource allocation
- [[wiki/17-系统基础/06-知识字典/scheduling/gang-scheduling.md|gang-scheduling]] — Gang scheduling
- [[wiki/17-系统基础/06-知识字典/scheduling/karpenter-autoscaling.md|karpenter-autoscaling]] — Karpenter autoscaling
- [[wiki/17-系统基础/06-知识字典/scheduling/kubernetes-scheduler.md|kubernetes-scheduler]] — Kubernetes scheduler
- [[wiki/17-系统基础/06-知识字典/scheduling/node-declared-features.md|node-declared-features]] — Node declared features
- [[wiki/17-系统基础/06-知识字典/scheduling/node-pressure-eviction.md|node-pressure-eviction]] — Node pressure eviction
- [[wiki/17-系统基础/06-知识字典/scheduling/pod-overhead.md|pod-overhead]] — Pod overhead
- [[wiki/17-系统基础/06-知识字典/scheduling/pod-priority-and-preemption.md|pod-priority-and-preemption]] — Pod priority and preemption
- [[wiki/17-系统基础/06-知识字典/scheduling/pod-scheduling-readiness.md|pod-scheduling-readiness]] — Pod scheduling readiness
- [[wiki/17-系统基础/06-知识字典/scheduling/pod-topology-spread-constraints.md|pod-topology-spread-constraints]] — Pod topology spread constraints
- [[wiki/17-系统基础/06-知识字典/scheduling/resource-bin-packing.md|resource-bin-packing]] — Resource bin packing
- [[wiki/17-系统基础/06-知识字典/scheduling/scheduler-performance-tuning.md|scheduler-performance-tuning]] — Scheduler performance tuning
- [[wiki/17-系统基础/06-知识字典/scheduling/scheduling-framework.md|scheduling-framework]] — Scheduling framework
- [[wiki/17-系统基础/06-知识字典/scheduling/taints-and-tolerations.md|taints-and-tolerations]] — Taints and tolerations

#### 安全

- [[wiki/17-系统基础/06-知识字典/security/application-security-checklist.md|application-security-checklist]] — Application security checklist
- [[wiki/17-系统基础/06-知识字典/security/cloud-native-security.md|cloud-native-security]] — Cloud native security
- [[wiki/17-系统基础/06-知识字典/security/cloud-native-security-practices.md|cloud-native-security-practices]] — Cloud native security practices
- [[wiki/17-系统基础/06-知识字典/security/controlling-access-to-the-kubernetes-api.md|controlling-access-to-the-kubernetes-api]] — Controlling access to the kubernetes api
- [[wiki/17-系统基础/06-知识字典/security/good-practices-for-kubernetes-secrets.md|good-practices-for-kubernetes-secrets]] — Good practices for kubernetes secrets
- [[wiki/17-系统基础/06-知识字典/security/hardening-guide---authentication-mechanisms.md|hardening-guide---authentication-mechanisms]] — Hardening guide   authentication mechanisms
- [[wiki/17-系统基础/06-知识字典/security/hardening-guide---scheduler-configuration.md|hardening-guide---scheduler-configuration]] — Hardening guide   scheduler configuration
- [[wiki/17-系统基础/06-知识字典/security/kubernetes-api-server-bypass-risks.md|kubernetes-api-server-bypass-risks]] — Kubernetes api server bypass risks
- [[wiki/17-系统基础/06-知识字典/security/limit-ranges.md|limit-ranges]] — Limit ranges
- [[wiki/17-系统基础/06-知识字典/security/linux-kernel-security-constraints-for-pods-and-containers.md|linux-kernel-security-constraints-for-pods-and-containers]] — Linux kernel security constraints for pods and containers
- [[wiki/17-系统基础/06-知识字典/security/multi-tenancy.md|multi-tenancy]] — Multi tenancy
- [[wiki/17-系统基础/06-知识字典/security/node-resource-managers.md|node-resource-managers]] — Node resource managers
- [[wiki/17-系统基础/06-知识字典/security/pod-security-admission.md|pod-security-admission]] — Pod security admission
- [[wiki/17-系统基础/06-知识字典/security/pod-security-policies.md|pod-security-policies]] — Pod security policies
- [[wiki/17-系统基础/06-知识字典/security/pod-security-standards.md|pod-security-standards]] — Pod security standards
- [[wiki/17-系统基础/06-知识字典/security/policy-as-code.md|policy-as-code]] — Policy as code
- [[wiki/17-系统基础/06-知识字典/security/process-id-limits-and-reservations.md|process-id-limits-and-reservations]] — Process id limits and reservations
- [[wiki/17-系统基础/06-知识字典/security/resource-quotas.md|resource-quotas]] — Resource quotas
- [[wiki/17-系统基础/06-知识字典/security/role-based-access-control-good-practices.md|role-based-access-control-good-practices]] — Role based access control good practices
- [[wiki/17-系统基础/06-知识字典/security/runtime-security.md|runtime-security]] — Runtime security
- [[wiki/17-系统基础/06-知识字典/security/secrets-management-deep-dive.md|secrets-management-deep-dive]] — Secrets management deep dive
- [[wiki/17-系统基础/06-知识字典/security/security-checklist.md|security-checklist]] — Security checklist
- [[wiki/17-系统基础/06-知识字典/security/security-for-linux-nodes.md|security-for-linux-nodes]] — Security for linux nodes
- [[wiki/17-系统基础/06-知识字典/security/security-for-windows-nodes.md|security-for-windows-nodes]] — Security for windows nodes
- [[wiki/17-系统基础/06-知识字典/security/service-accounts.md|service-accounts]] — Service accounts
- [[wiki/17-系统基础/06-知识字典/security/spiffe-spire-identity.md|spiffe-spire-identity]] — Spiffe spire identity
- [[wiki/17-系统基础/06-知识字典/security/supply-chain-security.md|supply-chain-security]] — Supply chain security

#### 专项工作负载

- [[wiki/17-系统基础/06-知识字典/specialized-workloads/ai-infra-specialist.md|ai-infra-specialist]] — Ai infra specialist
- [[wiki/17-系统基础/06-知识字典/specialized-workloads/gpu-resource-management-and-partitioning.md|gpu-resource-management-and-partitioning]] — Gpu resource management and partitioning
- [[wiki/17-系统基础/06-知识字典/specialized-workloads/guide-for-running-windows-containers-in-kubernetes.md|guide-for-running-windows-containers-in-kubernetes]] — Guide for running windows containers in kubernetes
- [[wiki/17-系统基础/06-知识字典/specialized-workloads/hpc-and-bioinformatics.md|hpc-and-bioinformatics]] — Hpc and bioinformatics
- [[wiki/17-系统基础/06-知识字典/specialized-workloads/kserve-model-serving.md|kserve-model-serving]] — Kserve model serving
- [[wiki/17-系统基础/06-知识字典/specialized-workloads/kueue-job-queue-management.md|kueue-job-queue-management]] — Kueue job queue management
- [[wiki/17-系统基础/06-知识字典/specialized-workloads/llm-inference-optimization.md|llm-inference-optimization]] — Llm inference optimization
- [[wiki/17-系统基础/06-知识字典/specialized-workloads/mlops-pipelines-and-model-registry.md|mlops-pipelines-and-model-registry]] — Mlops pipelines and model registry
- [[wiki/17-系统基础/06-知识字典/specialized-workloads/vector-databases-and-rag-infrastructure.md|vector-databases-and-rag-infrastructure]] — Vector databases and rag infrastructure
- [[wiki/17-系统基础/06-知识字典/specialized-workloads/windows-containers-in-kubernetes.md|windows-containers-in-kubernetes]] — Windows containers in kubernetes

#### 存储

- [[wiki/17-系统基础/06-知识字典/storage/csi-volume-cloning.md|csi-volume-cloning]] — Csi volume cloning
- [[wiki/17-系统基础/06-知识字典/storage/dynamic-volume-provisioning.md|dynamic-volume-provisioning]] — Dynamic volume provisioning
- [[wiki/17-系统基础/06-知识字典/storage/ephemeral-volumes.md|ephemeral-volumes]] — Ephemeral volumes
- [[wiki/17-系统基础/06-知识字典/storage/high-performance-storage-networks.md|high-performance-storage-networks]] — High performance storage networks
- [[wiki/17-系统基础/06-知识字典/storage/local-ephemeral-storage.md|local-ephemeral-storage]] — Local ephemeral storage
- [[wiki/17-系统基础/06-知识字典/storage/node-specific-volume-limits.md|node-specific-volume-limits]] — Node specific volume limits
- [[wiki/17-系统基础/06-知识字典/storage/object-storage-and-data-pipelines.md|object-storage-and-data-pipelines]] — Object storage and data pipelines
- [[wiki/17-系统基础/06-知识字典/storage/persistent-volumes.md|persistent-volumes]] — Persistent volumes
- [[wiki/17-系统基础/06-知识字典/storage/projected-volumes.md|projected-volumes]] — Projected volumes
- [[wiki/17-系统基础/06-知识字典/storage/storage-capacity.md|storage-capacity]] — Storage capacity
- [[wiki/17-系统基础/06-知识字典/storage/storage-classes.md|storage-classes]] — Storage classes
- [[wiki/17-系统基础/06-知识字典/storage/volume-attributes-classes.md|volume-attributes-classes]] — Volume attributes classes
- [[wiki/17-系统基础/06-知识字典/storage/volume-health-monitoring.md|volume-health-monitoring]] — Volume health monitoring
- [[wiki/17-系统基础/06-知识字典/storage/volume-snapshot-classes.md|volume-snapshot-classes]] — Volume snapshot classes
- [[wiki/17-系统基础/06-知识字典/storage/volume-snapshots.md|volume-snapshots]] — Volume snapshots
- [[wiki/17-系统基础/06-知识字典/storage/volumes.md|volumes]] — Volumes
- [[wiki/17-系统基础/06-知识字典/storage/windows-storage.md|windows-storage]] — Windows storage

#### 工具链

- [[wiki/17-系统基础/06-知识字典/tooling/cli-commands.md|cli-commands]] — Cli commands
- [[wiki/17-系统基础/06-知识字典/tooling/container-image-optimization.md|container-image-optimization]] — Container image optimization
- [[wiki/17-系统基础/06-知识字典/tooling/tool-ecosystem.md|tool-ecosystem]] — Tool ecosystem

#### 工作负载

- [[wiki/17-系统基础/06-知识字典/workloads/advanced-pod-configuration.md|advanced-pod-configuration]] — Advanced pod configuration
- [[wiki/17-系统基础/06-知识字典/workloads/automatic-cleanup-for-finished-jobs.md|automatic-cleanup-for-finished-jobs]] — Automatic cleanup for finished jobs
- [[wiki/17-系统基础/06-知识字典/workloads/autoscaling-workloads.md|autoscaling-workloads]] — Autoscaling workloads
- [[wiki/17-系统基础/06-知识字典/workloads/container-environment.md|container-environment]] — Container environment
- [[wiki/17-系统基础/06-知识字典/workloads/container-lifecycle-hooks.md|container-lifecycle-hooks]] — Container lifecycle hooks
- [[wiki/17-系统基础/06-知识字典/workloads/container-runtime-interface-cri.md|container-runtime-interface-cri]] — Container runtime interface cri
- [[wiki/17-系统基础/06-知识字典/workloads/cronjob.md|cronjob]] — Cronjob
- [[wiki/17-系统基础/06-知识字典/workloads/daemonset.md|daemonset]] — Daemonset
- [[wiki/17-系统基础/06-知识字典/workloads/deployments.md|deployments]] — Deployments
- [[wiki/17-系统基础/06-知识字典/workloads/disruptions.md|disruptions]] — Disruptions
- [[wiki/17-系统基础/06-知识字典/workloads/downward-api.md|downward-api]] — Downward api
- [[wiki/17-系统基础/06-知识字典/workloads/ephemeral-containers.md|ephemeral-containers]] — Ephemeral containers
- [[wiki/17-系统基础/06-知识字典/workloads/horizontal-pod-autoscaling.md|horizontal-pod-autoscaling]] — Horizontal pod autoscaling
- [[wiki/17-系统基础/06-知识字典/workloads/images.md|images]] — Images
- [[wiki/17-系统基础/06-知识字典/workloads/init-containers.md|init-containers]] — Init containers
- [[wiki/17-系统基础/06-知识字典/workloads/jobs.md|jobs]] — Jobs
- [[wiki/17-系统基础/06-知识字典/workloads/managing-workloads.md|managing-workloads]] — Managing workloads
- [[wiki/17-系统基础/06-知识字典/workloads/pod-group-policies.md|pod-group-policies]] — Pod group policies
- [[wiki/17-系统基础/06-知识字典/workloads/pod-hostname.md|pod-hostname]] — Pod hostname
- [[wiki/17-系统基础/06-知识字典/workloads/pod-lifecycle.md|pod-lifecycle]] — Pod lifecycle
- [[wiki/17-系统基础/06-知识字典/workloads/pod-quality-of-service-classes.md|pod-quality-of-service-classes]] — Pod quality of service classes
- [[wiki/17-系统基础/06-知识字典/workloads/pods.md|pods]] — Pods
- [[wiki/17-系统基础/06-知识字典/workloads/replicaset.md|replicaset]] — Replicaset
- [[wiki/17-系统基础/06-知识字典/workloads/replicationcontroller.md|replicationcontroller]] — Replicationcontroller
- [[wiki/17-系统基础/06-知识字典/workloads/runtime-class.md|runtime-class]] — Runtime class
- [[wiki/17-系统基础/06-知识字典/workloads/sidecar-containers.md|sidecar-containers]] — Sidecar containers
- [[wiki/17-系统基础/06-知识字典/workloads/spot-and-preemptible-workloads.md|spot-and-preemptible-workloads]] — Spot and preemptible workloads
- [[wiki/17-系统基础/06-知识字典/workloads/statefulsets.md|statefulsets]] — Statefulsets
- [[wiki/17-系统基础/06-知识字典/workloads/user-namespaces.md|user-namespaces]] — User namespaces
- [[wiki/17-系统基础/06-知识字典/workloads/vertical-pod-autoscaling.md|vertical-pod-autoscaling]] — Vertical pod autoscaling
- [[wiki/17-系统基础/06-知识字典/workloads/workload-api.md|workload-api]] — Workload api
- [[wiki/17-系统基础/06-知识字典/workloads/workload-management.md|workload-management]] — Workload management
- [[wiki/17-系统基础/06-知识字典/workloads/workload-reference.md|workload-reference]] — Workload reference

## 相关 Domain
- [[maintaining/03-发布/package/2026-07-02_18-29/corpus/supporting/skills/training-lecturer/11-workloads/index|Domain 01 集群基础 索引]]
- [[maintaining/03-发布/package/2026-07-02_18-29/corpus/supporting/skills/training-lecturer/11-workloads/index|Domain 13 容器运行时 索引]]


<!-- risk-assessed -->
