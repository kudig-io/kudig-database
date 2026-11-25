---
title: Kubernetes Cloud Providers 2025 2026
summary: 待补充摘要
category: entities
tags:
- kubernetes-cloud-providers-2025-2026
tier: supporting
created: '2026-07-01'
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。


# Kubernetes Cloud Provider Specifics 2025-2026

## 1. AWS EKS (Elastic Kubernetes Service)

### 1.1 EKS Auto Mode (GA: re:Invent 2024)
- Fully automates compute, storage, and networking for EKS clusters with a single toggle
- Built-in Karpenter-based node provisioning (no separate Karpenter install needed)
- Automatically manages node lifecycle: provisioning, scaling, updating, draining
- Includes managed storage (EBS CSI) and networking (VPC CNI) add-ons
- Automatic Kubernetes version upgrades with managed surge windows
- Built-in monitoring via CloudWatch integration
- Cost optimization: automatically selects right-sized instances, supports Spot capacity
- Node pools auto-configured based on workload requirements (requests/limits)
- Source: https://aws.amazon.com/eks/auto-mode/
- Docs: https://docs.aws.amazon.com/eks/latest/userguide/automode.html

### 1.2 EKS Pod Identity (GA: 2024)
- Replaces IRSA (IAM Roles for Service Accounts) as preferred auth method
- Pods get AWS credentials via native Kubernetes ServiceAccount tokens
- No need for OIDC provider configuration per cluster
- Supports cross-account access via IAM role chaining
- Simplified: one Pod Identity Association per namespace/service-account pair
- Works with EKS Auto Mode out of the box
- Source: https://docs.aws.amazon.com/eks/latest/userguide/pod-identities.html

### 1.3 EKS Hybrid Nodes (Preview 2024, GA 2025)
- Register on-premises or edge servers as EKS worker nodes
- Nodes run in customer datacenter but are managed by EKS control plane in AWS
- Uses AWS Systems Manager for node registration and management
- Supports VMware, bare metal, and other on-prem infrastructure
- Workload scheduling across cloud and on-prem nodes via standard K8s scheduling
- Enables gradual cloud migration without re-architecting
- Source: https://aws.amazon.com/eks/hybrid-nodes/
- Docs: https://docs.aws.amazon.com/eks/latest/userguide/hybrid-nodes.html

### 1.4 Karpenter on EKS
- Open-source node autoscaler originally by AWS, now CNCF project
- v1.0 GA (2024): stable API with NodePool and EC2NodeClass CRDs
- v1.12 (latest 2025): NodeOverlays for custom node configurations, improved disruption budgets
- Key features: just-in-time node provisioning, consolidation, drift detection
- Spot interruption handling with native EC2 integration
- EKS Auto Mode embeds Karpenter functionality (no separate install)
- For non-Auto-Mode clusters, standalone Karpenter still recommended over Cluster Autoscaler
- Source: https://karpenter.sh/docs/
- EKS-specific: https://docs.aws.amazon.com/eks/latest/userguide/karpenter.html

### 1.5 AWS CNI & CSI
- **VPC CNI** (default): assigns real VPC IPs to pods, supports prefix delegation, security groups per pod
- **EBS CSI Driver**: managed add-on, supports EBS gp3/io2/multi-attach
- **EFS CSI Driver**: shared storage for ReadWriteMany workloads
- **Mountpoint for S3**: high-throughput S3 access as filesystem
- **FSx for Lustre CSI**: high-performance computing workloads
- Spot strategy: use Karpenter NodePools with `karpenter.sh/capacity-type: spot` label; supports capacity-optimized allocation across instance types

---

## 2. GCP GKE (Google Kubernetes Engine)

### 2.1 GKE Autopilot (GA since 2021, major updates 2024-2025)
- Google fully manages nodes — users only specify pod resource requests
- Pay per pod resource usage (CPU/memory/storage), not per node
- Automatic node provisioning, upgrades, security patching
- Hardened security baseline: CIS benchmarks enforced, no SSH to nodes
- Supports GPU workloads (NVIDIA L4, A100, H100) in Autopilot mode (2024+)
- GKE Autopilot for GPU: auto-provisions GPU nodes with pre-installed drivers
- Burstable pods supported for variable workloads
- Multi-zonal and regional Autopilot clusters
- Source: https://cloud.google.com/kubernetes-engine/docs/concepts/autopilot-overview
- Docs: https://cloud.google.com/kubernetes-engine/docs/how-to/autopilot-create-cluster

### 2.2 GKE Enterprise (formerly Anthos)
- Multi-cluster management platform spanning GCP, on-prem, other clouds
- Fleet management: register and manage clusters across environments from single pane
- Config Sync: GitOps-based policy and configuration management
- Policy Controller: OPA/Gatekeeper-based admission control
- Multi-cluster Services: cross-cluster service discovery and load balancing
- Binary Authorization: enforce trusted container images
- Managed Service Mesh (based on Istio/Envoy) with fleet-wide observability
- Pricing: per-vCPU per-hour for fleet features
- Source: https://cloud.google.com/kubernetes-engine/enterprise
- Docs: https://cloud.google.com/kubernetes-engine/docs/concepts/enterprise

### 2.3 Multi-cluster Gateway
- Gateway API-based multi-cluster ingress/load balancing
- Cross-cluster traffic routing with single external IP
- Supports weighted routing, header-based routing across clusters
- Integrates with Google Cloud Load Balancing (global/regional)
- Health-check based failover between clusters
- Mesh-based east-west traffic via Service Mesh
- Source: https://cloud.google.com/kubernetes-engine/docs/concepts/multi-cluster-gateway
- Docs: https://cloud.google.com/kubernetes-engine/docs/how-to/multi-cluster-gateway-setup

### 2.4 GCP CNI & CSI
- **GKE Dataplane V2** (default CNI): eBPF-based, built on Cilium, provides network policy enforcement
- **Persistent Disk CSI**: default CSI driver, supports dynamic provisioning of PD-SSD/PD-Standard
- **Filestore CSI**: NFS-based ReadWriteMany storage
- **GCS FUSE CSI**: mount Cloud Storage buckets as filesystems
- Spot strategy: use Spot VMs with GKE `cloud.google.com/gke-spot: "true"` node taint; automatic VM termination handling; good for batch/fault-tolerant workloads

---

## 3. Azure AKS (Azure Kubernetes Service)

### 3.1 AKS Automatic (Preview 2024, GA 2025)
- Fully managed AKS experience similar to GKE Autopilot
- Azure manages node pools, scaling, upgrades, and security
- Automatic cluster configuration with best-practice defaults
- Includes: Azure CNI overlay, Defender for Containers, Azure Monitor
- Node Auto-provisioning via Karpenter-based logic (Node Autoprovision)
- Automatic GPU node provisioning for AI/ML workloads
- Source: https://learn.microsoft.com/en-us/azure/aks/automatic-cluster

### 3.2 KAITO (Kubernetes AI Toolchain Operator)
- Open-source project by Microsoft for AI model serving on AKS
- Automatically provisions GPU nodes and deploys AI models (LLMs, vision models)
- Supports models: Llama, Falcon, Mistral, Phi, and custom models
- Integrated with Hugging Face model registry
- Handles GPU driver installation, model download, and inference server setup
- Custom Resource Definitions: Workspace, TuningJob
- Cost optimization: auto-scales GPU nodes, supports Spot VMs
- Source: https://github.com/Azure/KAITO
- Docs: https://learn.microsoft.com/en-us/azure/aks/kaito

### 3.3 Node Autoprovision (NAP)
- Azure's implementation of Karpenter for AKS
- Automatically creates and manages node pools based on pending pod requirements
- Supports multiple VM SKU types, zones, and Spot instances
- Consolidation: merges underutilized nodes to reduce cost
- Custom NodePool definitions with constraints (labels, taints, limits)
- Integrated with Azure pricing and capacity APIs
- Source: https://learn.microsoft.com/en-us/azure/aks/node-autoprovision

### 3.4 Azure CNI & CSI
- **Azure CNI** (default): assigns VNet IPs to pods, supports overlay mode for IP conservation
- **Azure CNI powered by Cilium**: eBPF-based data plane (2024+)
- **Azure Disk CSI**: default CSI, supports Premium/Standard/Ultra disks, zone-redundant storage
- **Azure Files CSI**: SMB/NFS ReadWriteMany storage
- **Azure Blob CSI**: blob storage mounting via NFSv3 or BlobFuse2
- Spot strategy: use Azure Spot VMs with `kubernetes.azure.com/scalesetpriority: spot` label; supports eviction policy (Delete/Deallocate), max price configuration

---

## 4. Alibaba Cloud ACK (Container Service for Kubernetes)

### 4.1 ACK Serverless (ASK - Alibaba Serverless Kubernetes)
- Fully serverless Kubernetes — no node management
- Pods run on ECI (Elastic Container Instances), billed per pod-second
- Auto-scales from zero to thousands of pods
- Ideal for bursty, event-driven, and batch workloads
- Supports GPU instances for AI/ML workloads
- Integrated with Alibaba Cloud VPC, SLB, NAS, OSS
- Source: https://www.alibabacloud.com/product/kubernetes

### 4.2 ACK Pro (Managed Kubernetes)
- Enterprise-grade managed Kubernetes with SLA guarantees
- Multi-cluster management via ACK One (fleet management)
- Integrated with Alibaba Cloud Security Center and Log Service
- Supports ARM64 nodes (Yitian 710 processors)
- KNative and service mesh integration
- Cost optimization: preemptible instances (Spot equivalent), auto-scaling with Karpenter-compatible scaling
- Hybrid cloud support via ACK@Edge for edge and on-prem deployments
- Source: https://www.alibabacloud.com/product/container-service-for-kubernetes

### 4.3 Alibaba Cloud CNI & CSI
- **Terway CNI**: Alibaba's custom CNI using eBPF, supports ENI-based pod networking
- **CSI Plugin**: supports Alibaba Cloud Disk, NAS (NFS), OSS (Object Storage)
- Preemptible strategy: use preemptible instances with 1-90% discount; automatic reclamation with 5-min warning; use instance hibernation for stateful workloads

---

## 5. Multi-Cloud Abstraction Layers

### 5.1 Cluster API (CAPI)
- Kubernetes SIG project for declarative cluster lifecycle management
- Manages clusters across providers via Kubernetes-style CRDs
- Providers: AWS, GCP, Azure, vSphere, OpenStack, Equinix Metal, and 30+ others
- Machine, MachineSet, MachineDeployment resources for node lifecycle
- ClusterClass: reusable templates for standardized cluster provisioning
- Bootstrap providers: kubeadm, EKS, AKS, GKE, RKE2, Talos
- Control plane providers for managed and self-managed clusters
- Source: https://cluster-api.sigs.k8s.io/
- GitHub: https://github.com/kubernetes-sigs/cluster-api

### 5.2 Rancher (SUSE)
- Multi-cluster Kubernetes management platform
- Manages clusters across any infrastructure: cloud, on-prem, edge
- Fleet: GitOps-based continuous delivery across clusters
- NeuVector: integrated runtime security and zero-trust networking
- Harvester: hyperconverged infrastructure built on K8s for VM workloads
- Supports importing and managing EKS, AKS, GKE clusters
- RKE2 (Rancher Kubernetes Engine 2): hardened, FIPS-compliant K8s distribution
- K3s: lightweight K8s for edge, IoT, CI/CD
- Source: https://www.rancher.com/
- Docs: https://ranchermanager.docs.rancher.com/

### 5.3 VMware Tanzu (Broadcom)
- Tanzu Platform (formerly Tanzu Application Platform): app-centric K8s platform
- Tanzu Mission Control: multi-cluster fleet management (SaaS)
- Tanzu Kubernetes Grid (TKG): consistent K8s runtime across clouds and on-prem
- Tanzu for Kubernetes Operations: integrated toolchain for operations
- vSphere with Tanzu: native K8s on vSphere (VM Service, Supervisor Cluster)
- Supports AWS, Azure, and on-prem vSphere deployments
- Note: Broadcom acquisition (2023) has shifted strategy; focus on VMware Cloud Foundation integration
- Source: https://tanzu.vmware.com/

### 5.4 Other Notable Abstractions
- **Crossplane** (CNCF): Kubernetes-native control planes for any cloud resource via CRDs/XRDs
- **vCluster**: virtual clusters within a host cluster for multi-tenancy
- **Loft Labs**: platform for self-service K8s namespaces and vClusters
- **Gardener** (SAP): open-source K8s-as-a-Service across any cloud
- **k0s** (Mirantis): zero-friction K8s distribution for any infrastructure

---

## 6. Cloud-Specific CSI/CNI Integration Comparison

| Feature              | AWS EKS              | GCP GKE              | Azure AKS             | Alibaba ACK           |
|----------------------|----------------------|----------------------|-----------------------|-----------------------|
| **Default CNI**      | VPC CNI              | Dataplane V2 (Cilium)| Azure CNI             | Terway (eBPF)         |
| **eBPF CNI**         | VPC CNI + Cilium opt | Built-in (Dataplane V2)| Azure CNI + Cilium  | Terway native         |
| **Network Policy**   | Calico / VPC CNI     | Built-in eBPF        | Azure NP / Cilium     | Terway / Calico       |
| **Block CSI**        | EBS CSI              | PD CSI               | Azure Disk CSI        | Alibaba Disk CSI      |
| **File CSI**         | EFS CSI              | Filestore CSI        | Azure Files CSI       | Alibaba NAS CSI       |
| **Object CSI**       | S3 Mountpoint        | GCS FUSE CSI         | Blob CSI (BlobFuse2)  | OSS CSI               |
| **HPC Storage**      | FSx for Lustre       | GCS + Parallelstore  | ANF / Blob CSI        | CPFS (Parallel FS)    |
| **Pod Networking**   | VNet IPs or prefix   | Alias IPs            | VNet IPs or overlay   | ENI or vSwitch        |
| **SG per Pod**       | Yes (VPC CNI)        | No (use NP)          | No (use NP)           | No (use NP)           |
| **Dual Stack**       | Yes (IPv4/IPv6)      | Yes                  | Yes                   | Yes                   |

---

## 7. Spot/Preemptible Strategies Per Cloud

### AWS Spot Instances
- Up to 90% discount vs On-Demand
- 2-minute termination notice
- Strategy: Karpenter NodePools with spot capacity type, spread across instance families
- Use capacity-optimized allocation to minimize interruption rate
- Combine with On-Demand baseline (mixed instances policy)
- Spot Blocks (deprecated): use Capacity Reservations instead
- Source: https://aws.amazon.com/ec2/spot/

### GCP Spot VMs (formerly Preemptible)
- Up to 91% discount, 24-hour max lifetime (can be extended)
- 30-second termination notice
- Strategy: `cloud.google.com/gke-spot: "true"` taint/toleration
- Combine with standard nodes for baseline capacity
- GKE supports automatic rescheduling on preemption
- Source: https://cloud.google.com/kubernetes-engine/docs/how-to/spot-vms

### Azure Spot VMs
- Up to 90% discount, variable pricing
- Eviction notice varies (typically 30 seconds)
- Strategy: `kubernetes.azure.com/scalesetpriority: spot` label
- Configure eviction policy: Delete (remove pod) or Deallocate (pause VM)
- Max price option (set to -1 for pay-as-you-go price as ceiling)
- Use Azure Advisor for optimal instance selection
- Source: https://learn.microsoft.com/en-us/azure/aks/spot-node-pool

### Alibaba Preemptible Instances
- Up to 90% discount, 5-minute warning before reclamation
- Strategy: preemptible instance types in node pool configuration
- Supports "instance hibernation" to preserve state on preemption
- Use with ACK auto-scaling for cost-efficient batch processing
- Source: https://www.alibabacloud.com/help/en/ecs/user-guide/overview-of-preemptible-instances

### Best Practices (All Clouds)
- Never run single-replica stateful workloads on Spot
- Use PodDisruptionBudgets to maintain availability
- Spread across instance types/families/AZs to reduce correlated interruptions
- Implement graceful shutdown handlers (SIGTERM -> drain -> SIGKILL)
- Use priority classes: Spot for batch/dev, On-Demand for critical services

---

## Source URLs Summary

| Topic | URL |
|-------|-----|
| EKS Auto Mode | https://aws.amazon.com/eks/auto-mode/ |
| EKS Pod Identity | https://docs.aws.amazon.com/eks/latest/userguide/pod-identities.html |
| EKS Hybrid Nodes | https://aws.amazon.com/eks/hybrid-nodes/ |
| Karpenter | https://karpenter.sh/docs/ |
| GKE Autopilot | https://cloud.google.com/kubernetes-engine/docs/concepts/autopilot-overview |
| GKE Enterprise | https://cloud.google.com/kubernetes-engine/enterprise |
| Multi-cluster Gateway | https://cloud.google.com/kubernetes-engine/docs/concepts/multi-cluster-gateway |
| AKS Automatic | https://learn.microsoft.com/en-us/azure/aks/automatic-cluster |
| KAITO | https://github.com/Azure/KAITO |
| AKS Node Autoprovision | https://learn.microsoft.com/en-us/azure/aks/node-autoprovision |
| Alibaba ACK | https://www.alibabacloud.com/product/container-service-for-kubernetes |
| Cluster API | https://cluster-api.sigs.k8s.io/ |
| Rancher | https://www.rancher.com/ |
| VMware Tanzu | https://tanzu.vmware.com/ |
| AWS Spot | https://aws.amazon.com/ec2/spot/ |
| GKE Spot VMs | https://cloud.google.com/kubernetes-engine/docs/how-to/spot-vms |
| Azure Spot AKS | https://learn.microsoft.com/en-us/azure/aks/spot-node-pool |

---

*Research compiled: May 2025. Information based on publicly available documentation and announcements through Q2 2025.*


<!-- risk-assessed -->
