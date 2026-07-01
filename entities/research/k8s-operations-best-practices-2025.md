---
title: K8S Operations Best Practices 2025
summary: 'Source: https://github.com/argoproj/argo-cd/releases'
category: entities
tags:
- k8s-operations-best-practices-2025
tier: supporting
created: '2026-07-01'
---

# Kubernetes Production Operations Best Practices 2025-2026

## 1. GitOps Evolution

### ArgoCD (Latest: v3.4.2)
Source: https://github.com/argoproj/argo-cd/releases

ArgoCD has moved well beyond v3.0 - current releases are v3.2.12 (stable) and v3.4.2 (latest).

Key features in the v3.x line:
- **Progressive Syncs**: ApplicationSet supports rolling/progressive deployments across
  multiple applications, allowing staged rollouts with automatic promotion or rollback.
  Source: https://argo-cd.readthedocs.io/en/latest/operator-manual/applicationset/Progressive-Syncs/
- **ApplicationSet as first-class citizen**: Deeply integrated for managing fleets of
  applications from a single ApplicationSet template.
- **Multi-cluster management**: Native support for deploying to remote clusters with
  cluster secrets and ApplicationSet generators (cluster, git, list, matrix, merge).
- **Improved RBAC**: Fine-grained project-scoped permissions.
- **OCI Helm support**: Native OCI registry support for Helm charts.
- **Notification controller**: Built-in notifications to Slack, Teams, email, webhooks.
- **Argo CD Application Controller improvements**: Better performance at scale with
  thousands of applications.

Release cadence: Active release branches for v3.2.x (stable) and v3.4.x (latest).

Source: https://argo-cd.readthedocs.io/en/stable/
Source: https://blog.argoproj.io/


### Flux v2 (Latest: v2.8.8)
Source: https://github.com/fluxcd/flux2/releases

Flux v2 continues active development with multiple component releases:
- Source Controller: v1.5.5
- Kustomize Controller: integrated
- Helm Controller: v4.2.0
- Notification Controller: v5.19.1
- Image Automation Controller: v1.1.4
- Image Reflector Controller: v1.8.5

Key capabilities:
- **Multi-source support**: Git repositories, OCI artifacts, Helm repositories, S3 buckets
- **OCI HelmRelease**: Native OCI support for Helm charts via source-controller
- **FluxCon 2025**: Community conference highlighting new features and adoption
- **Flagger integration**: Progressive delivery with canary/A-B/blue-green deployments
- **SOPS integration**: Native secrets encryption via Mozilla SOPS
- **Image automation**: Automatic image scanning and deployment updates
- **Notification providers**: Slack, Teams, Discord, GitHub, GitLab, webhooks, Grafana

Source: https://fluxcd.io/
Source: https://fluxcd.io/flux/get-started/


### GitOps Best Practices 2025
- Use ApplicationSet (ArgoCD) or Kustomize overlays (Flux) for multi-env management
- Implement Progressive Delivery for production rollouts
- Store all manifests in Git with signed commits
- Use GitOps for infrastructure AND application deployments
- Implement policy-as-code with Kyverno or OPA Gatekeeper alongside GitOps
- Use OCI registries as artifact stores for Helm charts and container images


---

## 2. Day-2 Operations Automation

### Core Day-2 Operations
- **Certificate rotation**: Automate with cert-manager, integrate with HashiCorp Vault
- **Secret rotation**: External Secrets Operator or CSI Secrets Store for dynamic secrets
- **Node upgrades**: Rolling node replacement via Cluster API or kured for reboot management
- **Resource right-sizing**: VPA (Vertical Pod Autoscaler) recommendations, KRR for VPA recs
- **Cost optimization**: kubecost, OpenCost, or CAST AI for automated resource optimization
- **Security patching**: Automated CVE scanning with Trivy/Grype, admission policies via Kyverno

### Automation Tooling
- **Ansible + Kubernetes**: ansible-operator for Day-2 operational automation
- **Terraform/Pulumi**: Infrastructure provisioning with drift detection
- **Crossplane**: Kubernetes-native infrastructure provisioning
- **KubeOps/operators**: Custom operators for application-specific Day-2 tasks

### Monitoring & Observability Stack 2025
- **kube-prometheus-stack**: Prometheus + Grafana + Alertmanager
- **OpenTelemetry Collector**: Vendor-neutral telemetry collection
- **Grafana Loki**: Log aggregation
- **Tempo/Jaeger**: Distributed tracing
- **Adaptive metrics**: Prometheus recording rules auto-generated


---

## 3. Production Runbook Automation with AI

### AI-Assisted Operations (2025-2026 Trends)
- **Kubernetes Copilot tools**: AI-powered kubectl assistants for incident triage
- **Automated RCA (Root Cause Analysis)**: AI models trained on cluster events/logs
- **Intelligent alerting**: ML-based anomaly detection replacing static thresholds
- **Runbook-as-Code**: Executable runbooks stored in Git (Rundeck, StackStorm)
- **ChatOps with AI**: Slack/Teams bots that can execute kubectl commands, describe pods,
  and suggest remediation steps

### Key Tools
- **k8sgpt**: AI-powered Kubernetes debugging tool that scans clusters for issues
  Source: https://k8sgpt.ai/
- **Kubecost + AI**: Cost anomaly detection and right-sizing recommendations
- **Datadog Watchdog**: AI-powered incident detection and correlation
- **Pagerduty AI**: Automated incident routing and suggested runbooks
- **Robusta.dev**: Open-source Kubernetes troubleshooting with AI-powered insights

### Runbook Automation Patterns
1. Auto-detect common issues (CrashLoopBackOff, OOMKilled, ImagePullBackOff)
2. Map issues to predefined remediation playbooks
3. Execute approved actions automatically or with human approval
4. Document all automated actions for audit trail


---

## 4. kubectl Plugins for Ops (via Krew)
Source: https://krew.sigs.k8s.io/

Krew is the kubectl plugin manager. Essential plugins for production ops:

### Debugging & Troubleshooting
- **kubectl-debug**: Debug running pods with an ephemeral container
- **kubectl-ktop**: Top-like resource monitoring
- **kubectl-resource-capacity**: Cluster resource capacity overview
- **kubectl-ktop**: Real-time pod/node resource monitoring
- **kubectl-images**: List container images in use

### Cluster Management
- **kubectl-ctx**: Switch between cluster contexts
- **kubectl-ns**: Switch between namespaces
- **kubectl-tree**: Show resource ownership tree
- **kubectl-neat**: Remove noisy metadata from kubectl output

### Production Operations
- **kubectl-cert-manager**: Manage cert-manager resources
- **kubectl-krew**: Plugin management itself
- **kubectl-rook-ceph**: Rook Ceph storage management
- **kubectl-df-pv**: Show persistent volume disk usage

### Security & Compliance
- **kubectl-who-can**: RBAC permission checking
- **kubectl-access-matrix**: Show RBAC access matrix
- **kubectl-rakkess**: Show access rights for current user

Source: https://krew.sigs.k8s.io/docs/user-guide/quickstart/


---

## 5. Fleet Management at Scale

### Architecture Patterns
- **Hub-and-spoke**: Central management cluster + remote workload clusters
- **Hierarchical**: Regional management clusters per geography
- **GitOps-based fleet**: Single Git repo or mono-repo managing all clusters

### Tools & Platforms
- **Argo CD ApplicationSet + Cluster Generator**: Manage 100s of clusters from one ArgoCD
- **Flux with Kustomize overlays**: Per-cluster overlay directories in mono-repo
- **Rancher Fleet**: SUSE's fleet management for 1000s of clusters
  Source: https://fleet.rancher.io/
- **Google Anthos / GKE Fleet**: Managed multi-cluster with Config Sync
- **Azure Arc**: Azure's multi-cluster management plane
- **AWS EKS Connector**: Onboard any Kubernetes cluster to AWS console
- **vSphere Tanzu**: VMware's multi-cluster management

### Best Practices at Scale (100+ clusters)
1. Standardize cluster configurations with ClusterClass (Cluster API)
2. Use GitOps for ALL cluster configuration - no imperative changes
3. Implement centralized policy enforcement (Kyverno/OPA Gatekeeper)
4. Deploy observability stack centrally, per-cluster agents (Prometheus remote write)
5. Automate certificate and secret rotation at fleet level
6. Use namespace-as-a-service or virtual clusters for multi-tenancy
7. Implement consistent RBAC across all clusters


---

## 6. Cluster Lifecycle Management (Cluster API v1.13.2)
Source: https://github.com/kubernetes-sigs/cluster-api/releases
Source: https://cluster-api.sigs.k8s.io/

### Current State
- Latest: v1.13.2 (supports Kubernetes v1.32.x - v1.36.x)
- Maintenance branch: v1.12.8, v1.0.32 (legacy)
- Stable and production-ready since v1.0

### Key Features
- **ClusterClass**: Declarative cluster templates - define once, deploy many
  - Topology-based cluster management
  - Runtime extensions for customization
  - In-place upgrades via ClusterClass
- **MachinePools**: Manage groups of machines as a pool (vs MachineSet per-machine)
- **MachineSetPreflightChecks**: Pre-flight validation before provisioning machines
- **Bootstrap providers**: kubeadm, MicroK8s, EKS, Talos, RKE2
- **Infrastructure providers**: AWS, Azure, GCP, vSphere, OpenStack, Equinix Metal,
  Hetzner, DigitalOcean, and 30+ providers
- **Control Plane providers**: kubeadm, Kamaji, Nested, Talos
- **Management cluster**: KCP (Kubernetes Control Plane) for management clusters

### Lifecycle Operations
- **Provisioning**: Declarative cluster creation from YAML/ClusterClass
- **Upgrades**: Rolling upgrades of control plane and worker nodes
- **Scaling**: Horizontal scaling via MachineDeployment/MachinePool
- **Remediation**: Automatic node replacement on health check failures
- **Deletion**: Graceful cluster teardown with resource cleanup

### Best Practices
1. Use ClusterClass for standardized cluster definitions
2. Implement MachineHealthCheck for automatic remediation
3. Pin Kubernetes versions and use rolling upgrade strategy
4. Use infrastructure-specific node classes for optimized instance types
5. Implement proper etcd backup before upgrades
6. Test cluster upgrades in staging with ClusterClass overrides


---

## 7. Multi-Tenancy Best Practices

### Approaches
1. **Namespace-based**: Single cluster, namespaces per tenant with RBAC + NetworkPolicy
2. **Virtual clusters (vcluster)**: Each tenant gets a virtual Kubernetes cluster
   Source: https://www.vcluster.com/
3. **Node-based**: Dedicated node pools per tenant
4. **Cluster-per-tenant**: Full isolation, highest cost

### Tooling
- **vcluster (Loft Labs)**: Virtual clusters within a host cluster
  - Full cluster API compatibility for tenants
  - Resource quotas and limits enforced
  - Syncer manages resource translation
- **Capsule**: Multi-tenant operator for namespace-based tenancy
  Source: https://capsule.clastix.io/
  - Tenant CRD defines boundaries
  - Automatic NetworkPolicy enforcement
  - Resource quota inheritance
- **Hierarchical Namespaces (HNC)**: Namespace hierarchy with policy inheritance
  Source: https://github.com/kubernetes-sigs/hierarchical-namespaces
- **Kyverno**: Policy-based multi-tenancy enforcement
- **OPA Gatekeeper**: Rego-based policy enforcement

### Best Practices
1. Define tenant boundaries clearly (namespace, virtual cluster, or cluster)
2. Enforce resource quotas per tenant (CPU, memory, storage, pod count)
3. Use NetworkPolicies to isolate tenant traffic
4. Implement RBAC per tenant with service accounts
5. Use admission controllers to enforce policies
6. Implement cost allocation per tenant (kubecost tenant labels)
7. Limit cluster-wide permissions for tenant users
8. Use Pod Security Standards (restricted/baseline) per tenant

### Security Isolation
- Pod Security Standards (PSS): enforce restricted profile for untrusted tenants
- RuntimeClass: use gVisor/Kata for additional workload isolation
- Seccomp/AppArmor: mandatory profiles for sensitive workloads
- Network segmentation: Calico/Cilium network policies per tenant


---

## Summary of Key Versions (as of May 2025)

| Tool               | Version     | Source                                                    |
|--------------------|-------------|-----------------------------------------------------------|
| ArgoCD             | v3.4.2      | https://github.com/argoproj/argo-cd/releases              |
| Flux v2            | v2.8.8      | https://github.com/fluxcd/flux2/releases                  |
| Cluster API        | v1.13.2     | https://github.com/kubernetes-sigs/cluster-api/releases   |
| Krew (kubectl)     | maintained  | https://krew.sigs.k8s.io/                                 |
| vcluster           | maintained  | https://www.vcluster.com/                                 |
| Kyverno            | maintained  | https://kyverno.io/                                       |
| k8sgpt             | maintained  | https://k8sgpt.ai/                                        |

---

Research compiled: May 2025
Sources: GitHub releases pages, official documentation sites, project blogs
