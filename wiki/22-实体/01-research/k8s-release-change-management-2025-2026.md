---
title: K8S Release Change Management 2025 2026
summary: 1. Developer merges code -> CI builds image 2. CI updates Git repo (image
  tag in Helm values or Kustomize overlay) 3. Argo CD/Flux detects change, syncs to
  cluster 4. Argo Rollouts/Flagger manages ...
category: entities
tags:
- k8s-release-change-management-2025-2026
tier: supporting
created: '2026-07-01'
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。


# Kubernetes Release & Change Management 2025-2026
## Research Findings — Compiled 2026-05-24

---

## 1. PROGRESSIVE DELIVERY (Argo Rollouts, Flagger)

### Argo Rollouts
- **Latest Version**: v1.9.0 (released 2026-03-19)
  - v1.9.0-rc1 (2025-11-07), v1.9.0-rc2 (2025-11-18)
  - Previous stable: v1.8.4
- **Key v1.9.0 features**:
  - `ensure progress when canary > desired replicas (#4619)` — fixes edge cases where canary stalls
  - `set weight before hash for new canary (#4564)` — improved traffic routing ordering
  - `add canaryStepString route msg (#4490)` — better observability for canary steps
  - Switch to `*bool` for `clusterScope` (#4551) — improved CRD configuration
- **Core Capabilities** (unchanged, mature):
  - Drop-in replacement for Kubernetes Deployment using `Rollout` CRD
  - Native support for canary, blue-green, canary analysis, experimentation
  - `AnalysisTemplates` for metric-driven automated promotion/rollback
  - Manual gates (pause/resume) for human-in-the-loop approvals
  - Traffic management integrations: Istio, Linkerd, Nginx Ingress, AWS ALB, Ambassador, Traefik, SMI
  - `Experiment` resources for A/B testing
- **OpenFeature Integration** (emerging pattern 2025):
  - Feature-flagged progressive delivery: combine Argo Rollouts canary with OpenFeature/flagd for feature toggles
  - Roll out features gradually with flag control independent of deployment
  - Sources: towardsaws.com, Medium (Atmosly)
- **Red Hat OpenShift GitOps 1.12** includes Argo Rollouts for progressive deployment delivery
  - Supports metric analysis for automated rollout/rollback
  - Source: docs.redhat.com

### Flagger
- Flagger continues as a progressive delivery tool by Flux/Weaveworks
- Supports canary, A/B, blue-green deployments
- Integrates with Istio, Linkerd, Contour, NGINX, Gloo, Traefik, AWS App Mesh
- Automated traffic shifting based on Prometheus metrics analysis
- Source: docs.flagger.app

### Sources
- https://github.com/argoproj/argo-rollouts/releases
- https://argo-rollouts.readthedocs.io
- https://docs.redhat.com/en/documentation/red_hat_openshift_gitops/1.12/html/argo_rollouts/
- https://docs.flagger.app
- https://medium.com/atmosly/argo-rollouts-a-complete-guide-to-progressive-delivery-in-kubernetes-a2d739d7c41e
- https://towardsaws.com/feature-flagged-progressive-delivery-argo-rollouts-openfeature-bd93c8ddd75f
- https://dstw.github.io/2025/06/01/progressive-delivery-pipeline/
- https://k8s.info/docs/advanced/progressive-delivery

---

## 2. CANARY / BLUE-GREEN / FEATURE FLAGS

### Canary Deployment Best Practices (2025)
- **Use Argo Rollouts or Flagger** as dedicated progressive delivery controllers
- **Traffic management**: Use service mesh (Istio/Linkerd) or ingress controller for fine-grained traffic splitting (e.g., 5% -> 25% -> 50% -> 100%)
- **Automated analysis**: Define `AnalysisTemplate` with Prometheus/Datadog/CloudWatch metrics for automated promotion/rollback based on error rates, latency P99, etc.
- **Manual gates**: Add pause steps between canary increments for human review
- **Pod disruption budgets**: Ensure PDBs protect during canary rollouts
- **Resource isolation**: Use separate namespaces or workloads for canary vs stable

### Blue-Green Deployments
- Argo Rollouts natively supports blue-green with:
  - Pre-promotion analysis and post-promotion analysis
  - Automatic rollback if analysis fails
  - Scale-down delay for the "blue" (old) version
- Blue-green provides instant cutover but requires 2x resources during deployment

### Feature Flags (2025 Trends)
- **OpenFeature**: CNCF standard for feature flagging; vendor-neutral API
- **flagd**: Reference OpenFeature implementation, Kubernetes-native
- **Flipt**: Open-source feature flag platform with OpenFeature provider
- **Pattern**: Feature flags decouple deployment from feature release
  - Deploy code behind flag -> enable for canary -> expand rollout -> GA
  - Enables trunk-based development with controlled exposure

### Sources
- https://argo-rollouts.readthedocs.io/en/stable/features/blue-green/
- https://argo-rollouts.readthedocs.io/en/stable/features/canary/
- https://openfeature.dev
- https://github.com/open-feature/flagd
- https://github.com/flipt-io/flipt

---

## 3. KUBERNETES RELEASE CADENCE (v1.33 - v1.36)

### v1.33 — "Octarine" (April 23, 2025)
- **64 enhancements**: 18 Stable, 20 Beta, 24 Alpha, 2 Deprecated/Withdrawn
- **Key stable features**:
  - In-Place Pod Resize (Beta) — resize CPU/memory without restart
  - Job's SuccessPolicy GA
  - Job's Backoff Limit Per Index GA
  - Updates to Container Lifecycle
  - Image Pull Policy improvements
- Source: https://kubernetes.io/blog/2025/04/23/kubernetes-v1-33-release/

### v1.34 — "O' WaW (Of Wind & Will)" (August 2025)
- **Key GA features**:
  - DRA (Dynamic Resource Allocation) graduated to GA
  - Recovery From Volume Expansion Failure (GA)
  - VolumeAttributesClass for Volume Modification (GA)
  - Pod Replacement Policy for Jobs (GA)
  - Decoupled Taint Manager (Stable)
  - Autoconfiguration for Node Cgroup Driver (GA)
- **Key Beta features**:
  - Pod Level Resources (Beta)
  - PSI Metrics for Kubernetes (Beta)
  - Service Account Token Integration for Image Pulls (Beta)
  - Mutable CSI Node Allocatable (Beta)
- **Notable Alpha**:
  - Snapshottable API server cache
  - User preferences (kuberc) in kubectl
  - Finer-Grained Control Over Container Restarts
  - CPU Manager Static Policy Option for Uncore Cache Alignment
- Source: https://kubernetes.io/blog/ (multiple v1.34 articles)

### v1.35 — "Timbernetes (The World Tree Release)" (November/December 2025)
- **Key GA features**:
  - In-Place Pod Resize graduated to Stable
  - Job Managed By (GA)
  - Fine-grained Supplemental Groups Control (GA)
  - Kubelet Configuration Drop-in Directory (GA)
- **Key Alpha/Beta features**:
  - Mutable PersistentVolume Node Affinity (Alpha)
  - Extended Toleration Operators for Numeric Comparisons (Alpha)
  - Workload Aware Scheduling
  - Watch Based Route Reconciliation in Cloud Controller Manager
  - Enhanced Debugging with Versioned z-pages APIs
  - New level of efficiency with in-place Pod restart
- Source: https://kubernetes.io/blog/ (multiple v1.35 articles)

### v1.36 — "Haru (ハル)" (Spring 2026)
- **Key GA features**:
  - User Namespaces in Kubernetes (GA)
  - PSI Metrics (GA) — graduated from Beta in v1.34
  - Volume Group Snapshots (GA)
  - Fine-Grained Kubelet API Authorization (GA)
  - Declarative Validation (GA)
  - SELinux Volume Label Changes (GA)
- **Key Beta/Alpha features**:
  - In-Place Vertical Scaling for Pod-Level Resources (Beta)
  - Mixed Version Proxy (Beta)
  - Server-Sided Sharded List and Watch
  - Pod-Level Resource Managers (Alpha)
  - Mutable Pod Resources for Suspended Jobs (Beta)
  - Tiered Memory Protection with Memory QoS
  - Staleness Mitigation and Observability for Controllers
  - Advancing Workload-Aware Scheduling
  - Admission Policies That Can't Be Deleted
- **Notable**: Deprecation and removal of Service ExternalIPs
- Source: https://kubernetes.io/blog/ (multiple v1.36 articles)

### Gateway API (cross-cutting)
- v1.4 (2025) and v1.5 (2026) releases
- Moving features to Stable
- Ingress2Gateway 1.0 announced for migration path

### Sources
- https://kubernetes.io/blog/2025/04/23/kubernetes-v1-33-release/
- https://kubernetes.io/blog/ (v1.34, v1.35, v1.36 blog posts)
- https://kubernetes.io/releases/

---

## 4. GITOPS DEPLOYMENT STRATEGIES

### Core GitOps Principles (2025)
- **Declarative**: Desired state stored in Git
- **Versioned**: Git as single source of truth
- **Automated**: Agents pull and reconcile
- **Observable**: Drift detection and alerting

### Argo CD (dominant GitOps tool)
- Native support for Helm, Kustomize, plain YAML, Jsonnet
- ApplicationSets for multi-cluster/multi-env deployments
- Progressive delivery integration via Argo Rollouts
- Notifications and sync windows for change management
- Source: https://argo-cd.readthedocs.io

### Flux CD
- CNCF graduated project
- Tight integration with Flagger for progressive delivery
- Kustomize-first approach with Helm controller
- Image automation controllers for automated image updates
- Source: https://fluxcd.io

### GitOps + Progressive Delivery Pattern
1. Developer merges code -> CI builds image
2. CI updates Git repo (image tag in Helm values or Kustomize overlay)
3. Argo CD/Flux detects change, syncs to cluster
4. Argo Rollouts/Flagger manages progressive rollout
5. Analysis runs against metrics
6. Auto-promote or auto-rollback based on results

---

## 5. HELM CHARTS EVOLUTION

### Helm (2025-2026)
- **Helm 3** remains the standard; Helm 4 in development
- **OCI Registry support** is mature — charts stored as OCI artifacts
  - `helm push`/`helm pull` with `oci://` prefix
  - Supported by Harbor, GitHub Container Registry, AWS ECR, Azure ACR, GCP Artifact Registry
- **Chart signing** with Sigstore/Cosign for supply chain security
- **Helm Unittest** and **Chart Testing (ct)** for CI validation
- **Best practices**:
  - Use values.schema.json for input validation
  - Separate environment-specific values from chart defaults
  - Use Helmfile or Helmsman for multi-chart orchestration
  - Version pin dependencies in Chart.yaml

### Kustomize
- Built into `kubectl` (`kubectl apply -k`)
- Best for organizations that prefer overlays over templating
- Strengths: patch-based customization, no templating language
- Used heavily with Flux CD (Kustomize controller)
- Common pattern: base + per-environment overlays (dev/staging/prod)

### Helm vs Kustomize (2025 consensus)
- **Helm**: Better for distributing reusable packages, ecosystem sharing
- **Kustomize**: Better for internal team overlays, simpler mental model
- **Many teams use both**: Helm for third-party charts, Kustomize for custom apps

---

## 6. DEPLOYMENT VERIFICATION & AUTOMATED ROLLBACK

### Automated Rollback Mechanisms
- **Argo Rollouts AnalysisTemplates**:
  - Define Prometheus/Datadog/CloudWatch queries
  - Set success conditions (e.g., `result[0] < 0.01` for error rate < 1%)
  - Automatic rollback if analysis fails
  - Configurable intervals, counts, and failure limits
- **Flagger Metric Templates**:
  - Similar approach with Prometheus queries
  - Canary analysis with configurable thresholds
  - Automatic rollback on metric failure
- **Kubernetes native**:
  - `Deployment` supports `spec.rollbackTo` (deprecated) — prefer Rollouts
  - Liveness/readiness probes as basic verification
  - PodDisruptionBudgets for availability during rollouts

### Deployment Verification Best Practices (2025)
1. **Pre-deployment checks**: Validate manifests (kubeval/kubeconform), security scans (Trivy/Snyk)
2. **Smoke tests**: Run health checks immediately after deployment
3. **Metric-based analysis**: Monitor error rates, latency, saturation for N minutes
4. **Log analysis**: Check for increased error/warning log volume
5. **Synthetic testing**: Run integration tests against new version in canary
6. **Manual approval gates**: Optional human checkpoint before full promotion
7. **Post-deployment monitoring**: Extended observation window after promotion

---

## 7. CHANGE MANAGEMENT COMPLIANCE

### Compliance Framework for K8s (2025)
- **Git as audit trail**: Every change tracked in Git with author, timestamp, review status
- **Pull request workflows**: Require PR reviews before merging to deployment branches
- **RBAC + Admission Controllers**: Enforce who can deploy what, where
- **OPA/Gatekeeper / Kyverno**: Policy-as-code for compliance rules
  - Require specific labels, resource limits, security contexts
  - Restrict image sources to approved registries
  - Enforce namespace policies
- **Sync windows** (Argo CD): Restrict deployment times to maintenance windows
- **Signed commits and images**: Supply chain integrity verification
- **Drift detection**: Alert when cluster state diverges from Git

### Change Management Process
1. **Request**: Developer creates PR with desired changes
2. **Review**: Peer review + automated policy checks (OPA/Kyverno)
3. **Approve**: Required approvals from designated reviewers
4. **Merge**: Merge to main/trunk branch
5. **Deploy**: GitOps agent detects change, initiates rollout
6. **Verify**: Progressive delivery with automated analysis
7. **Promote/Rollback**: Based on verification results
8. **Audit**: Full Git history provides compliance audit trail

### Tools for Compliance
- **OPA/Gatekeeper**: Policy admission control
- **Kyverno**: Kubernetes-native policy engine
- **Sigstore/Cosign**: Image signing and verification
- **SLSA framework**: Supply chain security levels
- **Audit logging**: Kubernetes audit logs + SIEM integration

---

## SUMMARY TABLE

| Topic | Key Tools | Status 2025-2026 |
|-------|-----------|-------------------|
| Progressive Delivery | Argo Rollouts v1.9.0, Flagger | Mature, production-ready |
| Canary Deployments | Argo Rollouts, Flagger, Istio | Best practice with analysis |
| Blue-Green | Argo Rollouts | Native support, stable |
| Feature Flags | OpenFeature, flagd, Flipt | CNCF standard emerging |
| GitOps | Argo CD, Flux CD | Both CNCF graduated |
| Helm Charts | Helm 3, OCI registries | OCI mature, Helm 4 coming |
| Kustomize | Built into kubectl | Stable, Flux integration |
| K8s v1.33 | "Octarine" | April 2025, 64 enhancements |
| K8s v1.34 | "O' WaW" | Aug 2025, DRA GA |
| K8s v1.35 | "Timbernetes" | Nov 2025, In-Place Resize GA |
| K8s v1.36 | "Haru" | Spring 2026, User NS GA |
| Compliance | OPA, Kyverno, Sigstore | Policy-as-code standard |


<!-- risk-assessed -->
