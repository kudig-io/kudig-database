> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。

# Domain Content Gap Analysis — 2026-07-01

Synthesized from 20 subagent domain audit reports (`domain-01` through `domain-20`). This document is a prioritization reference for senior SREs and content owners.

---

## 1. Executive Summary

### 1.1 Overall Coverage Scores

| Domain | Coverage Score | Priority |
|---|---|---|
| domain-01-cluster-fundamentals | 3 / 5 | P1 |
| domain-02-workloads-applications | 3 / 5 | P1 |
| domain-03-networking-traffic | 3 / 5 | P1 |
| domain-04-storage-data | 3 / 5 | P1 |
| domain-05-security-compliance | 3 / 5 | P1 |
| domain-06-observability | 3 / 5 | P2 |
| domain-07-platform-engineering | 3 / 5 | P1 |
| domain-08-release-change-management | 3 / 5 | P1 |
| domain-09-reliability-engineering | 3 / 5 | P1 |
| domain-10-troubleshooting-diagnostics | 3 / 5 | P2 |
| domain-11-production-operations | 2 / 5 | P0 |
| domain-12-cloud-providers | 2 / 5 | P0 |
| domain-13-container-runtime | 3 / 5 | P2 |
| domain-14-ai-ml-infra | 2.5 / 5 | P1 |
| domain-15-specialized-tech | 2 / 5 | P1 |
| domain-16-database-middleware | 3 / 5 | P2 |
| domain-17-system-foundation | 3 / 5 | P2 |
| domain-18-manifests-patterns | 3 / 5 | P2 |
| domain-19-landscape-references | 2 / 5 | P1 |
| domain-20-application-patterns | 2 / 5 | P0 |

**Average coverage: 2.7 / 5**

### 1.2 Top 5 Cross-Cutting Gaps

1. **Certificate / PKI Lifecycle & Rotation Runbooks** — Kubernetes internal CA rotation (`kubeadm certs renew`, front-proxy, etcd, kubelet), cert-manager CA rotation, ingress/mTLS cert expiry, and break-glass rotation are missing or fragmented across domains 01, 05, 07, 08, 09, 11, 16.
2. **Multi-Cluster / Fleet Management & GitOps at Scale** — No authoritative coverage of ApplicationSet, Karmada/OCM/Cluster API, cross-cluster promotion, secret sync, global load balancing, or fleet observability. Gaps appear in domains 02, 03, 07, 08, 09, 11, 12, 14, 18, 20.
3. **Disaster Recovery / Business Continuity Runbooks** — Missing actionable runbooks for control-plane failure, etcd quorum loss, AZ/region failure, stateful workload DR, network blackout, and Velero/etcd restore validation. Affects domains 01, 03, 04, 07, 09, 11, 12, 14, 16, 20.
4. **Production Readiness Review (PRR) & Onboarding Gates** — No consolidated PRR template or service-onboarding checklist covering HA, DR, observability, SLOs, PDBs, quotas, security, and rollback. Mentioned in domains 07, 09, 11, 20.
5. **Incident Response / On-Call Runbooks** — Severity routing, escalation matrices, communication templates, war-room procedures, and postmortem frameworks are absent or too thin in domains 01, 07, 09, 10, 11, 14.

---

## 2. Per-Domain Gap Summary

| Domain | Score | Top Missing Production Topics | Top Incomplete Files | Priority |
|---|---|---|---|---|
| **domain-01-cluster-fundamentals** | 3 / 5 | 1. Control-plane incident runbook & SLO dashboard 2. PKI / certificate lifecycle & rotation 3. Node lifecycle, OS hardening & graceful maintenance 4. Pod Security Standards (PSA) enforcement guide 5. Audit logging policy & SIEM integration | `01-production-architecture-design-principles.md` (stale PSP, old CA image); `01-architecture-overview/17-production-operations-best-practices.md` (PSP); `07-performance-tuning/19-cluster-performance-tuning.md` (Docker, unsafe mount options); `03-control-plane/10-plane-backup-disaster-recovery.md` (circular dependency) | P1 |
| **domain-02-workloads-applications** | 3 / 5 | 1. Workload identity & ServiceAccount hardening 2. NetworkPolicy / L7 workload segmentation 3. KEDA / event-driven autoscaling 4. Argo Rollouts / Flagger progressive delivery 5. Stateful workload backup & DR | `README.md` (incomplete, duplicated); `00-core-workloads/12-advanced-pod-patterns.md` (too thin); `00-core-workloads/21-hpa-vpa-autoscaling.md` (VPA weak); Java top-level files duplicated in `topic-java-kubernetes/` | P1 |
| **domain-03-networking-traffic** | 3 / 5 | 1. Calico production operations guide 2. Cloud CNI ops (AWS VPC CNI, Azure CNI, GCP CNI) 3. Network disaster recovery / BC runbook 4. Network SLO/SLI & error budgeting 5. CNI / kube-proxy / CoreDNS / Ingress upgrade runbooks | Six stub `00-core-k8s-networking/4*-terway-*.md` files duplicate `topic-terway/`; `34-network-performance-tuning.md` (no SLOs/cloud tuning); `18-network-encryption-mtls.md` (no rotation/DR); `39-csi-cni-version-matrix.md` (CSI scope bleed) | P1 |
| **domain-04-storage-data** | 3 / 5 | 1. Storage multi-tenancy governance 2. Ephemeral storage management 3. CSI driver security & supply-chain hardening 4. Storage chaos engineering / failure testing 5. Capacity planning & forecasting | `01-k8s-storage/07-storage-daily-operations.md` (typo `vvc`, unsafe advice); `13-storage-security-compliance.md` (deprecated PSP); `15-storage-disaster-recovery.md` (fictional CRDs); `08-storage-performance-tuning.md` (duplicates monitoring file) | P1 |
| **domain-05-security-compliance** | 3 / 5 | 1. JIT / break-glass / PAM access lifecycle 2. Cloud-native workload identity (IRSA, GKE Workload Identity, AKS) 3. Secret & encryption key rotation runbooks 4. Security component HA/DR (Vault, cert-manager, OPA, Kyverno, Falco) 5. Node / host OS hardening | `01-identity-access/01-authentication-authorization-system.md` (duplicate frontmatter); `P3-11-security-incident-sop-compliance-checklist.md` (stale paths); `05-supply-chain/10-image-security-scanning.md` & `13-image-security-scanning.md` (duplicate); `07-incident-response/20-incident-response-process.md` (Docker-only forensics) | P1 |
| **domain-06-observability** | 3 / 5 | 1. Prometheus cardinality governance & metric reliability 2. eBPF-based observability 3. Synthetic / blackbox monitoring 4. Observability stack self-monitoring 5. GPU / AI-ML workload observability | `README.md` (stale file counts/paths); `10-monitoring-metrics-prometheus.md`, `26-troubleshooting-tools.md`, `27-performance-profiling-tools.md`, `03-loki-enterprise-log-aggregation.md`, `99-kubernetes-v1.33-observability-guide.md` (duplicate frontmatter); `98-merged-indexes/*` (stale references) | P2 |
| **domain-07-platform-engineering** | 3 / 5 | 1. Production readiness checklist / onboarding gate 2. Patch management & OS/node lifecycle automation 3. Certificate rotation operational runbook 4. Secret management at platform scale 5. Incident response & on-call runbooks | `README.md` (broken wikilink); `operate/15-production-troubleshooting.md` (too thin); `operate/25-virtual-clusters.md` (thin, wrong title); `operate/16-platform-upgrade-migration.md` (duplicated section); multiple files with duplicate frontmatter | P1 |
| **domain-08-release-change-management** | 3 / 5 | 1. Container artifact registry production guide 2. Kubernetes cluster upgrade runbook 3. Multi-cluster / fleet GitOps 4. Secrets management in GitOps 5. Release engineering & versioning | `README.md` (broken wikilink); `04-testing-quality/01-selenium-enterprise-automation.md` (malformed frontmatter); `01-gitops/99-*` guide files duplicating enterprise files; `04-production-environment-deployment.md` (upgrade/cert sections too shallow) | P1 |
| **domain-09-reliability-engineering** | 3 / 5 | 1. Kubernetes-native control-plane resilience runbook 2. PodDisruptionBudgets & graceful disruption 3. Stateful workload DR patterns 4. Cluster upgrade reliability & rollback runbook 5. Certificate lifecycle reliability at scale | `09-disaster-recovery-playbooks/01-dr-scenarios-catalog.md` (stub); `02-az-failure-playbook.md` (no cloud specifics); `08-performance-testing/*.md` (superficial); `05-chaos-engineering/*.md` (too short); `02-disaster-recovery/18-cross-region-disaster-recovery.md` (malformed frontmatter, broken Istio YAML) | P1 |
| **domain-10-troubleshooting-diagnostics** | 3 / 5 | 1. Incident management & postmortem process 2. Change correlation & deployment-induced incident runbook 3. Multi-tenant noisy-neighbor / resource contention 4. Cluster-wide emergency lockdown / isolation 5. Cloud-provider-specific symptom runbooks | `README.md` (lists only 4 of 12 subdirs); `SUMMARY.md` (project-wide, misleading); ~30+ files in `topic-structural-trouble-shooting/` with duplicate frontmatter; `topic-skills/07-pvc-storage-failure 2.md` (duplicate) | P2 |
| **domain-11-production-operations** | 2 / 5 | 1. Capacity planning & production readiness 2. Multi-cluster / fleet operations 3. Backup, DR and business continuity 4. Production security hardening operations 5. Cluster lifecycle & upgrade operations | `01-finops/13-kubernetes-cost-governance.md` (simulated data, duplicate frontmatter); `04-green-computing/15-green-computing-sustainability.md` (simulated data); `02-governance/14-resource-quota-management.md` (no governance patterns); `03-incident-response/23-incident-response-handling.md` (outdated, `componentstatuses`); `ticket-cases/` (significant duplication) | P0 |
| **domain-12-cloud-providers** | 2 / 5 | 1. Provider-specific cluster upgrade runbooks 2. DR / backup / restore per cloud 3. Provider-native observability setup 4. Security hardening & guardrails per cloud 5. IAM / workload identity deep dives | `README.md` (omits subdirs 09–15); `02-aws-eks/aws-eks-overview.md` (stale version, no Pod Identity/Karpenter); `03-google-cloud-gke/google-cloud-gke-overview.md` (stale); `04-azure-aks/azure-aks-overview.md` (stale); most provider overviews are templated/marketing-only | P0 |
| **domain-13-container-runtime** | 3 / 5 | 1. ACR / ACR Enterprise Edition production guide 2. Node image GC & disk-pressure runbook 3. Image pull troubleshooting runbook for K8s 4. Container runtime security hardening (node-level) 5. Fleet-wide runtime upgrade runbook | `README.md` (outdated map, missing dirs); `98-merged-indexes/index.md` (missing new dirs); `01-docker/01-docker-architecture-overview.md` (stale links, bloated See Also); `01-docker/07-docker-security-best-practices.md` (too thin); `01-containerd-deep-guide.md` (overlaps `03-containerd-cri-o/`) | P2 |
| **domain-14-ai-ml-infra** | 2.5 / 5 | 1. AI/ML cluster HA & control-plane resilience 2. GPU node firmware / BIOS / driver lifecycle 3. AI platform DR & backup 4. Multi-cluster / federated AI training & inference 5. Production incident response runbooks | `README.md` (lists non-existent `03-mlops/`); `02-ai-agents/` vs `02-ai-agents/` (near-complete duplication); `01-ai-infra/17-llm-inference-serving.md` (malformed Istio YAML); `01-ai-infra/11-ai-security-model-protection.md` (weak K8s controls); `01-ai-infra/99-kubeflow-ai-platform-guide.md` (generic, no HA/upgrade) | P1 |
| **domain-15-specialized-tech** | 2 / 5 | 1. Edge fleet lifecycle management 2. Edge control-plane / CloudCore HA & DR 3. Edge observability under constrained bandwidth 4. Wasm production deployment patterns 5. Admission webhook HA & failure-policy runbook | `01-edge-computing/99-kubernetes-developer-toolchain-guide.md` (misplaced); `03-edge-computing-production-deployment.md` (root, inconsistent name, risky advice); `03-extensions/11-service-mesh-overview.md` (overlaps domain-03, broken YAML); multiple `03-extensions/` files belong in other domains | P1 |
| **domain-16-database-middleware** | 3 / 5 | 1. Database security hardening on K8s 2. etcd on Kubernetes guide 3. Cache / RPC / API gateway middleware (`02-middleware/` promised but missing) 4. Database DR & multi-cluster runbook 5. Database SLO/SLI & capacity planning | `README.md` (lists non-existent `02-middleware/`); `01-databases/99-cloudnativepg-enterprise-guide.md` ("Kafka CRD" typo); `01-databases/01-mysql-enterprise-database.md` (broken `if` syntax, cleartext passwords); `04-time-series-db/01-prometheus-tsdb-deep-dive.md` (83 lines, broken link) | P2 |
| **domain-17-system-foundation** | 3 / 5 | 1. Time synchronization for K8s nodes 2. systemd & kubelet service management 3. Kernel live patching 4. OS image & node hardening baseline 5. NUMA / CPU topology awareness | `01-linux/07-linux-security-hardening.md` (references removed PSP); `01-linux/06-linux-performance-tuning.md` (conntrack warnings missing); `01-linux/08-linux-container-fundamentals.md` (Docker-heavy); `02-hardware/16-kubernetes-hardware-troubleshooting.md` (debug image gaps, force drain); `README.md` (omits cheat-sheet/dictionary) | P2 |
| **domain-18-manifests-patterns** | 3 / 5 | 1. Production GitOps / declarative delivery patterns 2. Multi-cluster / fleet management patterns 3. Cluster backup, DR & BC patterns 4. Observability instrumentation manifests 5. FinOps / resource optimization patterns | `README.md` (omits `98-merged-indexes/`); `01-yaml-reference/03-pod-specification-complete.md` (duplicate frontmatter); `01-yaml-reference/23-pod-security-standards.md` (title typo); `36-ecosystem-kustomize-helm-argocd.md` (basic ArgoCD); `27-hpa-autoscaling-v2.md` (no VPA/KEDA/Karpenter) | P2 |
| **domain-19-landscape-references** | 2 / 5 | 1. CNCF landscape curation / selection guides 2. Kubernetes / ecosystem version lifecycle & EOL matrix 3. Managed Kubernetes vendor comparison 4. Security advisory / CVE tracking integration 5. Ecosystem deprecation & migration tracker | `README.md` (claims CNCF coverage but dirs empty, broken links); `01-cncf-landscape/{graduated,incubating,sandbox}/` (empty); `topic-release-notes/README.md` (confusing archive location); `98-merged-indexes/` (stale Domain-34 imports) | P1 |
| **domain-20-application-patterns** | 2 / 5 | 1. Pod availability & lifecycle patterns 2. Resource QoS & right-sizing 3. Scheduling & topology resilience 4. Stateful application runbook patterns 5. Multi-cluster & DR patterns | `README.md` (references non-existent `01-reference-architectures/`); `topic-application-architecture/README.md` (90 vs 96 file count, typos); `98-merged-indexes/MOC-from-domain-42.md` (stale); many shallow/template-heavy vertical files | P0 |

---

## 3. Cross-Cutting Production Themes Missing Across Multiple Domains

| Theme | Why It Matters | Domains Affected | Recommended Owner Domain |
|---|---|---|---|
| **Certificate / PKI Lifecycle** | Cert expiry and CA rotation are recurring P0/P1 causes. | 01, 05, 07, 08, 09, 11, 16 | domain-07-platform-engineering / domain-09-reliability-engineering |
| **Multi-Cluster / Fleet / GitOps at Scale** | Production platforms are multi-cluster; needs ApplicationSet, OCM/Karmada, secret sync, global LB. | 02, 03, 07, 08, 09, 11, 12, 14, 18, 20 | domain-12-cloud-providers / domain-08-release-change-management |
| **Disaster Recovery / Business Continuity** | Needs actionable runbooks for control plane, etcd, AZ/region, network, stateful workloads. | 01, 03, 04, 07, 09, 11, 12, 14, 16, 20 | domain-09-reliability-engineering |
| **Production Readiness Review (PRR)** | Standard SRE gate for production launches. | 07, 09, 11, 20 | domain-09-reliability-engineering / domain-07-platform-engineering |
| **Incident Response / On-Call Runbooks** | Severity routing, escalation, communication, postmortem. | 01, 07, 09, 10, 11, 14 | domain-11-production-operations / domain-10-troubleshooting-diagnostics |
| **FinOps / Cost Governance** | Right-sizing, spot/preemptible, chargeback/showback, autoscaling economics. | 02, 04, 07, 08, 11, 12, 14, 18, 20 | domain-11-production-operations / domain-07-platform-engineering |
| **Observability / SLO Operations** | RED/USE, SLO/SLI, alert routing, cardinality governance, self-monitoring. | 03, 06, 09, 11, 18, 20 | domain-06-observability / domain-11-production-operations |
| **Node & Runtime Security Hardening** | SELinux/AppArmor/seccomp, privileged restrictions, runtime threat detection, PSA. | 01, 04, 05, 07, 11, 13, 17 | domain-05-security-compliance / domain-13-container-runtime |
| **GPU / AI-ML Operations** | GPU scheduling, MIG/time-slicing, DCGM, RDMA, training/inference DR, spot GPU. | 02, 06, 13, 14, 17 | domain-14-ai-ml-infra |
| **Edge / WebAssembly Production** | Fleet lifecycle, network-partition tolerance, Wasm runtime hardening, observability. | 10, 15 | domain-15-specialized-tech |
| **Cloud Provider Operational Runbooks** | Provider-specific upgrade, DR, observability, security, IAM, storage, troubleshooting. | 12, 17 | domain-12-cloud-providers |
| **Cluster Upgrade / Rollback Runbooks** | High-risk recurring operation; needs pre-checks, skew checks, rollback gates. | 01, 08, 09, 11, 12 | domain-01-cluster-fundamentals / domain-09-reliability-engineering |
| **Image Registry / Supply Chain Security** | Hardened registry, signing, admission verification, promotion pipelines. | 08, 13 | domain-13-container-runtime / domain-05-security-compliance |
| **Stateful Workload DR** | Databases, Kafka, Redis, PVC snapshots, cross-region failover, restore drills. | 02, 04, 09, 16, 20 | domain-04-storage-data / domain-16-database-middleware |

---

## 4. Recommended New Files / Pages

### domain-01-cluster-fundamentals
- `03-control-plane/36-control-plane-operational-runbook.md` — SLOs, golden signals, and incident playbooks for API server, etcd, and scheduler.
- `03-control-plane/34-pki-lifecycle-and-certificate-rotation.md` — CA renewal, leaf cert rotation, and `kubeadm certs renew` runbook.
- `03-control-plane/35-node-lifecycle-and-maintenance.md` — Cordon/drain, reboot orchestration, OS/kernel hardening.
- `03-control-plane/37-pod-security-standards-production-guide.md` — PSA `restricted`/`baseline`/`privileged` labels and PSP migration.
- `03-control-plane/38-api-server-auditing-production.md` — Audit policy, retention, and SIEM forwarding.
- `03-control-plane/39-air-gapped-cluster-deployment.md` — Image mirror, RegistryConfiguration, and Helm chart mirroring.
- `03-control-plane/40-cluster-decommissioning-runbook.md` — Safe teardown, etcd/PV sanitization, and cloud resource cleanup.

### domain-02-workloads-applications
- `00-core-workloads/24-workload-identity-security.md` — IRSA, GKE Workload Identity, bound SA tokens, projected volumes.
- `00-core-workloads/25-workload-network-segmentation.md` — Zero-trust NetworkPolicy and Cilium L7 examples.
- `00-core-workloads/26-keda-event-driven-autoscaling.md` — KEDA ScaledObjects, triggers, cooldown, fallback.
- `00-core-workloads/27-progressive-delivery-argo-rollouts.md` — Argo Rollouts canary with Prometheus/Datadog and abort/rollback.
- `00-core-workloads/28-stateful-workload-backup-dr.md` — Velero/restic, PV snapshots, cross-region DR, restore drills.
- `00-core-workloads/29-workload-cost-optimization.md` — Right-sizing, spot/preemptible affinity, request/limit economics.
- `00-core-workloads/30-workload-admission-policies.md` — Kyverno/OPA Gatekeeper enforcement for PSS, quotas, image provenance.
- `00-core-workloads/31-multi-cluster-workload-fleet.md` — Karmada, OCM, Argo CD ApplicationSets, topology-aware routing.
- `00-core-workloads/32-gpu-workloads-production.md` — GPU requests/limits, MIG, time-slicing, RuntimeClass.
- `00-core-workloads/33-workload-packaging-patterns.md` — Helm values management, Kustomize overlays, library charts.

### domain-03-networking-traffic
- `00-core-k8s-networking/03b-calico-production-guide.md` — BGP peering, IP pool design, Felix tuning, upgrade runbook.
- `00-core-k8s-networking/03c-cloud-cni-operations.md` — AWS/Azure/GCP CNI IP exhaustion, prefix delegation, SG for pods.
- `00-core-k8s-networking/50-network-disaster-recovery.md` — CNI control-plane outage, CoreDNS failure, Ingress blackout recovery.
- `00-core-k8s-networking/48-network-slo-sli.md` — Network golden signals, latency/drop-rate SLOs, conntrack saturation.
- `00-core-k8s-networking/49-network-component-upgrade-runbook.md` — CNI, kube-proxy, CoreDNS, Ingress controller upgrades.
- `00-core-k8s-networking/03d-ipv6-dual-stack-production.md` — Dual-stack design for Cilium, Calico, cloud CNIs.
- `00-core-k8s-networking/15-network-security-hardening.md` — CNI seccomp/capabilities, control-plane segmentation.
- `02-service-mesh/99-istio-control-plane-dr.md` — Istio/Linkerd HA failover, root cert rotation, mesh outage recovery.

### domain-04-storage-data
- `01-k8s-storage/xx-storage-multitenancy-governance.md` — StorageClass allowlist, quotas, chargeback labels.
- `01-k8s-storage/xx-ephemeral-storage-management.md` — emptyDir limits, node-disk GC, DiskPressure avoidance.
- `01-k8s-storage/xx-csi-security-hardening.md` — Image provenance, RBAC, seccomp, hostPath exposure.
- `01-k8s-storage/xx-storage-chaos-engineering.md` — PVC failure, CSI node plugin failure, zone outage simulation.
- `01-k8s-storage/xx-storage-capacity-planning.md` — Trending, headroom policies, class-level dashboards.
- `01-k8s-storage/xx-multicloud-csi-reference.md` — EBS gp3/io2, GCE PD, Azure Managed Disks StorageClasses.
- `03-distributed-storage/xx-openebs-topolvm-production.md` — Local PV operations for Kafka/Redis/ES.
- `03-distributed-storage/xx-minio-object-storage-production.md` — S3-compatible object storage on Kubernetes.

### domain-05-security-compliance
- `01-identity-access/12-production-iam-lifecycle.md` — JIT access, break-glass, PAM, access reviews.
- `01-identity-access/13-cloud-workload-identity.md` — IRSA, GKE/AKE Workload Identity, ACK RAM Roles.
- `06-compliance/18-certificate-key-rotation-runbook.md` — Automated rotation with zero-downtime and rollback.
- `06-compliance/19-security-infrastructure-ha-dr.md` — HA/DR for cert-manager, Vault, OPA, Kyverno, Falco.
- `02-network-security/22-service-mesh-security.md` — Istio mTLS, authorization policies, SPIFFE/SPIRE.
- `02-network-security/23-egress-dns-security.md` — Egress gateways, DNS filtering, external allowlists.
- `04-policy-governance/15-validating-admission-policy-production.md` — VAP migration from OPA/Kyverno.
- `06-compliance/20-node-host-os-hardening.md` — SELinux/AppArmor/seccomp, CIS, container runtime hardening.
- `06-compliance/21-security-observability-siem.md` — Audit/Falco/policy events to SIEM/SOAR.
- `03-runtime-security/18-container-sandbox-selection.md` — Kata/Firecracker/gVisor trade-offs.
- `05-supply-chain/14-registry-security-image-promotion.md` — Harbor hardening, immutable tags, promotion pipelines.

### domain-06-observability
- `02-metrics/xx-prometheus-cardinality-governance.md` — Label limits, relabeling, WAL corruption handling.
- `01-overview/xx-ebpf-observability.md` — eBPF network tracing and kernel diagnostics.
- `02-metrics/xx-synthetic-blackbox-monitoring.md` — External DNS/HTTPS/TCP/TLS cert probes.
- `05-alerting/xx-observability-stack-self-monitoring.md` — Prometheus/Alertmanager/Grafana/Loki outage runbooks.
- `02-metrics/xx-gpu-monitoring.md` — DCGM, NVIDIA GPU metrics, MIG monitoring.
- `02-metrics/xx-observability-multitenancy.md` — Grafana orgs, tenant-aware Alertmanager, quotas.

### domain-07-platform-engineering
- `governance/01-production-readiness-framework.md` — PRR template and onboarding gate.
- `operate/03-patch-management-node-lifecycle.md` — CVE patching and kernel updates without disruption.
- `operate/04-certificate-rotation-runbook.md` — Cert expiry response and rotation.
- `governance/12-secret-management-platform-scale.md` — Vault/ESO/Sealed Secrets patterns.
- `governance/13-network-policy-governance.md` — Default-deny, egress lockdown, policy testing.
- `operate/20-platform-incident-response-runbook.md` — Severity routing, escalation, war-room procedures.
- `operate/21-control-plane-ha-etcd-recovery.md` — Quorum loss recovery with safety checks.
- `operate/22-gitops-at-scale.md` — App-of-apps, monorepo/polyrepo, ArgoCD HA/backup.
- `governance/14-cost-allocation-showback.md` — Per-team cost attribution.

### domain-08-release-change-management
- `01-gitops/09-container-artifact-registry-production.md` — Hardened, HA, geo-replicated registry.
- `topic-deployment/05-cluster-upgrade-runbook.md` — Version skew, kubeadm order, rollback.
- `01-gitops/11-multicluster-fleet-gitops.md` — ApplicationSet, cluster generators, promotion.
- `01-gitops/12-secrets-management-gitops.md` — ESO/Sealed Secrets/SOPS with rotation.
- `03-change-management/04-release-engineering-versioning.md` — SemVer, changelogs, artifact promotion.
- `03-change-management/05-database-schema-migrations-k8s.md` — Job-based migrations, rollback.
- `03-change-management/06-feature-flags-progressive-delivery.md` — LaunchDarkly/Unleash/OpenFeature.
- `01-gitops/13-cicd-observability-dora.md` — Deployment frequency, lead time, change-failure rate.
- `03-change-management/07-certificate-rotation-runbook.md` — cert-manager, ingress mTLS rotation.
- `04-testing-quality/04-ephemeral-preview-environments.md` — Per-PR namespaces with TTL/cost controls.
- `03-change-management/08-release-validation-chaos-engineering.md` — Litmus/Chaos Mesh in release gates.
- `04-testing-quality/07-k8s-native-conformance-e2e-testing.md` — Sonobuoy, KIND, KUTTL, kube-burner.

### domain-09-reliability-engineering
- `09-disaster-recovery-playbooks/04-control-plane-failure-playbook.md` — API server/etcd/scheduler recovery.
- `07-sre-practices/05-pod-disruption-budgets-production.md` — Graceful disruption patterns.
- `02-disaster-recovery/10-stateful-app-dr-patterns.md` — Kafka/ES/Redis/PostgreSQL/MySQL DR.
- `09-disaster-recovery-playbooks/05-cluster-upgrade-rollback-playbook.md` — Pre-checks, health gates, rollback.
- `07-sre-practices/06-certificate-reliability-runbook.md` — K8s CA, cert-manager failure modes.
- `07-sre-practices/07-production-readiness-review-template.md` — PRR checklist.
- `02-disaster-recovery/21-multi-cluster-fleet-reliability.md` — Fleet policy, global LB, federation.
- `09-disaster-recovery-playbooks/06-network-partition-dns-failure-playbook.md` — CoreDNS/CNI/mesh failures.
- `09-disaster-recovery-playbooks/07-security-incident-recovery-playbook.md` — Compromise isolation, credential rotation.
- `02-disaster-recovery/22-gitops-reliability-recovery.md` — ArgoCD failure and sync-wave recovery.
- `07-sre-practices/08-resilience-patterns.md` — Circuit breakers, retries, bulkheads.

### domain-10-troubleshooting-diagnostics
- `03-advanced-troubleshooting/45-incident-management-postmortem.md` — Severity, IC role, templates.
- `03-advanced-troubleshooting/46-change-correlation-runbook.md` — Map symptoms to recent changes.
- `topic-structural-trouble-shooting/07-resources-scheduling/05-multitenancy-resource-contention.md` — Noisy-neighbor diagnosis.
- `topic-structural-trouble-shooting/06-security-auth/05-emergency-lockdown.md` — Cluster/namespace isolation, evidence preservation.
- `03-advanced-troubleshooting/47-version-specific-known-issues.md` — Per-minor-release bug/regression matrix.
- `topic-structural-trouble-shooting/09-cloud-provider/eks-gke-aks-ack-runbooks.md` — Provider-specific failures.
- `topic-structural-trouble-shooting/02-node-components/07-windows-node-troubleshooting.md` — Windows node/containerd issues.
- `topic-structural-trouble-shooting/08-cluster-operations/07-edge-kubeedge-troubleshooting.md` — Edge network/device-plugin failures.
- `02-infrastructure-troubleshooting/35-airgapped-environment-troubleshooting.md` — Offline image/Helm/license issues.

### domain-11-production-operations
- `05-capacity-planning-readiness.md` — Headroom rules and pre-launch gates.
- `06-multi-cluster-operations.md` — Federation, fleet policy, multi-cluster observability.
- `07-disaster-recovery-backup.md` — RPO/RTO, etcd backup verification, restore runbooks.
- `08-security-operations-runbook.md` — PSP→PSS migration, secret rotation, CIS remediation.
- `09-cluster-upgrade-runbook.md` — Pre-checks, drain sequence, deprecation scan, rollback.
- `10-observability-operations.md` — SLO review cadence, alert tuning, dashboard-as-code.
- `11-gitops-operations.md` — ArgoCD/Flux incident response, drift detection.
- `12-automated-remediation.md` — PDB-aware restart, node problem detector, descheduler.
- `13-node-and-runtime-ops.md` — containerd/image pulls, kubelet PLEG, OS patch cadence.

### domain-12-cloud-providers
- `{aws,azure,gke,alicloud,tencent,huawei}-upgrade-runbook.md` — Provider-specific upgrade procedures.
- `{aws,azure,gke,alicloud,tencent,huawei}-disaster-recovery.md` — Provider DR/backup runbooks.
- `{aws,azure,gke,alicloud,tencent,huawei}-observability.md` — CloudWatch/Azure Monitor/SLS/CLS setup.
- `{aws,azure,gke,alicloud,tencent,huawei}-security-hardening.md` — Private endpoints, KMS, admission control.
- `{aws,azure,gke,alicloud,tencent,huawei}-iam-workload-identity.md` — IRSA, Workload Identity, RRSA.
- `{aws,azure,gke,alicloud,tencent,huawei}-network-troubleshooting.md` — CNI, LB, NAT, DNS issues.
- `{aws,azure,gke,alicloud,tencent,huawei}-storage.md` — CSI, snapshots, cross-AZ constraints.
- `{aws,azure,gke,alicloud,tencent,huawei}-cost-management.md` — Showback, savings plans, right-sizing.
- `{aws,azure,gke,alicloud,tencent,huawei}-troubleshooting-runbook.md` — API throttling, quotas, node NotReady.
- `08-multi-cloud/11-fleet-gitops-operations.md` — Fleet policy, secret sync, ApplicationSet.

### domain-13-container-runtime
- `02-image-management/07-alibaba-acr-enterprise.md` — ACR EE operations for ACK.
- `03-containerd-cri-o/04-node-image-gc-disk-management.md` — Image GC, DiskPressure runbook.
- `03-containerd-cri-o/05-image-pull-troubleshooting-runbook.md` — ImagePullBackOff/SandboxCreate diagnosis.
- `03-containerd-cri-o/06-runtime-security-hardening.md` — seccomp/AppArmor, privileged restrictions, Falco/Tetragon.
- `03-containerd-cri-o/07-runtime-fleet-upgrade-runbook.md` — Canary pools, version matrix, rollback.
- `03-containerd-cri-o/08-nerdctl-operations-guide.md` — nerdctl debug/build on K8s nodes.
- `04-image-build/04-podman-buildah-skopeo-guide.md` — Red Hat rootless build tooling.
- `03-containerd-cri-o/09-gpu-container-runtime.md` — NVIDIA Container Toolkit, CDI, MIG, RuntimeClass.
- `02-image-management/08-image-signing-admission-policy.md` — cosign/notation + Kyverno/OPA enforcement.

### domain-14-ai-ml-infra
- `01-ai-infra/00-ai-cluster-ha-control-plane.md` — Apiserver/etcd sizing for high-churn GPU workloads.
- `01-ai-infra/38-gpu-node-maintenance-firmware.md` — GPU VBIOS, NVLink, IB/RoCE firmware lifecycle.
- `01-ai-infra/39-ai-platform-disaster-recovery.md` — Kubeflow/MLflow DB, model registry, vector DB DR.
- `01-ai-infra/40-multicluster-ai-federation.md` — Cross-region training/inference, data locality.
- `01-ai-infra/90-runbooks/` — GPU OOM, NCCL timeout, inference latency, model rollback runbooks.
- `01-ai-infra/41-spot-gpu-elastic-training.md` — Spot GPUs, TorchElastic, checkpointing.
- `01-ai-infra/42-ai-secrets-management.md` — HF tokens, model-weight encryption, registry access.
- `01-ai-infra/43-rdma-network-fabric-ops.md` — IB/RoCE CNI, PFC/ECCN, NCCL debug.
- `01-ai-infra/44-gpu-chargeback-multi-tenant.md` — Per-team quotas, Kubecost chargeback.

### domain-15-specialized-tech
- `01-edge-computing/11-edge-fleet-lifecycle-management.md` — At-scale onboarding/offboarding.
- `01-edge-computing/12-edge-ha-dr.md` — CloudCore HA, regional failover.
- `01-edge-computing/13-edge-observability-production.md` — Bandwidth-constrained metrics/logs.
- `02-webassembly/11-wasm-production-deployment.md` — Wasm/Container hybrid scheduling.
- `03-extensions/17-webhook-production-runbook.md` — Webhook failure-policy matrix and recovery.
- `03-extensions/18-policy-governance-extensions.md` — Kyverno/OPA rollout patterns.

### domain-16-database-middleware
- `01-databases/00-database-security-hardening.md` — NetworkPolicy, encryption, least-privilege RBAC.
- `01-databases/09-etcd-on-kubernetes.md` — etcd quorum, disk latency, backup.
- `02-middleware/` (new subdir) — Cache/RPC/gateway middleware.
- `01-databases/10-database-dr-multi-cluster-runbook.md` — Cross-region failover, PITR, RTO/RPO.
- `01-databases/11-database-slo-capacity-planning.md` — Availability/latency SLOs, saturation signals.
- `01-databases/12-database-troubleshooting-runbook.md` — Split-brain, replication lag, PVC full.
- `01-databases/13-database-migration-guide.md` — VM→container, cross-cluster cutover.
- `01-databases/14-cloud-managed-databases-on-kubernetes.md` — AWS/GCP/Azure managed DB integration.
- `04-time-series-db/04-thanos-cortex-mimir.md` — Long-term retention, remote-write tuning.

### domain-17-system-foundation
- `01-linux/10-time-synchronization-chrony-ntp.md` — Node clock sync for etcd/API server.
- `01-linux/11-systemd-node-service-management.md` — kubelet/containerd systemd dependencies.
- `01-linux/12-numa-cpu-topology-for-k8s.md` — `numactl`, CPU manager static policy.
- `01-linux/13-k8s-node-os-image-hardening-baseline.md` — CIS, immutable infra, disk partitioning.
- `02-hardware/19-gpu-hardware-for-kubernetes.md` — NVLink/InfiniBand, MIG, DRA prerequisites.
- `02-hardware/20-bmc-redfish-automation.md` — Automated BMC provisioning/power cycling.
- `03-kubernetes-events/16-event-driven-runbooks-and-correlation.md` — Events to runbooks, severity classification.
- `03-kubernetes-events/17-control-plane-events.md` — Apiserver/scheduler/controller-manager failure events.
- `03-kubernetes-events/18-multi-cluster-edge-events.md` — Fleet/edge event routing.

### domain-18-manifests-patterns
- `02-gitops-delivery-patterns/` — AppProject, ApplicationSet, sync waves, sealed/external secrets.
- `02-multi-cluster-federation-patterns.md` — Karmada/OCM/Cluster API, Cluster Mesh.
- `03-disaster-recovery-patterns/` — Velero, etcd snapshot CronJob, cross-region restore.
- `04-observability-manifests-patterns/` — ServiceMonitor, PrometheusRule, OTel Collector, Fluent Bit.
- `05-finops-resource-optimization-patterns.md` — VPA, spot tolerations, bin-packing policies.
- `06-autoscaling-patterns/` — Karpenter NodePool, KEDA ScaledObject, VPA.
- `07-security-policy-patterns/` — Kyverno/OPA, cert-manager, cosign admission, default-deny NetworkPolicy.
- `08-multi-tenancy-patterns.md` — Namespace bundles, HNC, vCluster/Capsule.
- `09-service-mesh-manifests-patterns.md` — Istio VirtualService/DestinationRule/PeerAuthentication.
- `10-troubleshooting-runbook-manifests/` — Debug containers, netshoot, RBAC impersonation.
- `11-ai-ml-workload-manifests.md` — GPU operator, MIG, MPIJob/PyTorchJob, vLLM.

### domain-19-landscape-references
- `01-cncf-landscape/README.md` — Maturity-based selection framework.
- `01-cncf-landscape/graduated/`, `incubating/`, `sandbox/` — Project guides with adopt/trial/avoid recommendations.
- `kubernetes-version-lifecycle.md` — EOL calendar, supported skew matrix.
- `security-advisories-and-upgrade-matrix.md` — CVE response runbook and upgrade recommendations.
- `ecosystem-deprecation-migration-tracker.md` — Deprecation timeline and migration paths.
- `managed-kubernetes-comparison.md` — EKS/AKS/GKE/ACK/TKE feature/SLA comparison.
- `performance-benchmarks-index.md` — CNI/CSI/mesh/ingress benchmark decision matrix.

### domain-20-application-patterns
- `topic-production-patterns/pod-availability-lifecycle.md` — PDB, probes, graceful shutdown.
- `topic-production-patterns/resource-qos-rightsizing.md` — requests/limits, QoS, VPA.
- `topic-production-patterns/scheduling-topology-patterns.md` — topology spread, affinity, spot nodes.
- `topic-production-patterns/stateful-app-patterns.md` — StatefulSet, PVC snapshot, backup/restore.
- `topic-production-patterns/batch-cronjob-patterns.md` — Idempotency, deadlines, dead-letter.
- `topic-production-patterns/progressive-delivery-patterns.md` — Canary, feature flags, Argo Rollouts.
- `topic-production-patterns/application-security-hardening.md` — PSS, SecurityContext, mTLS, image signing.
- `topic-production-patterns/multi-cluster-dr-patterns.md` — Active-active, federation, RTO/RPO.
- `topic-production-patterns/app-observability-slo-patterns.md` — RED/USE, SLO/SLI, alert routing.
- `topic-production-patterns/cost-optimization-finops.md` — Right-sizing, spot, chargeback.
- `topic-production-patterns/application-runbooks.md` — CrashLoopBackOff, OOMKilled, ingress 5xx.
- `topic-production-patterns/release-change-management-patterns.md` — GitOps promotion, rollback.

---

## 5. Recommended Enhancements to Existing Files

### Structural / metadata fixes (all domains)
- Deduplicate double YAML frontmatter blocks in many files (noted in domains 01, 05, 06, 09, 10, 12, 13, 14, 17, 18).
- Update `README.md` files to match actual subdirectories and file counts (domains 02, 04, 10, 11, 12, 13, 16, 17, 18, 19, 20).
- Remove or consolidate stale `98-merged-indexes/` imports and broken Obsidian links (domains 03, 04, 10, 12, 16, 19, 20).
- Refresh `last_updated` and `k8s_versions` metadata to current supported range (1.28–1.33).

### domain-01-cluster-fundamentals
- `01-production-architecture-design-principles.md` — Replace PodSecurityPolicy examples with PSA labels; update Cluster Autoscaler image/registry.
- `01-architecture-overview/17-production-operations-best-practices.md` — Rewrite Pod Security section for PSA; add audit-policy snippet.
- `07-performance-tuning/19-cluster-performance-tuning.md` — Remove Docker section; warn against `insecure_skip_verify`; replace unsafe ext4 options.
- `03-control-plane/10-plane-backup-disaster-recovery.md` — Fix circular restore dependency; correct shell conditionals; add quorum-restore procedure.
- `03-control-plane/33-kubelet-eviction-thresholds.md` — Deduplicate `imagefs.available` signal table.

### domain-02-workloads-applications
- `00-core-workloads/12-advanced-pod-patterns.md` — Expand restartable init containers, sidecar injection, ephemeral containers.
- `00-core-workloads/18-node-management-operations.md` — Deepen cordon/drain/uncordon and PDB interplay.
- `00-core-workloads/21-hpa-vpa-autoscaling.md` — Add VPA + HPA conflict guidance and recommender modes.
- `00-core-workloads/07-workload-troubleshooting-handbook.md` — Add concrete runbook checklists with command snippets.
- Consolidate duplicated Java files at root and `topic-java-kubernetes/`.

### domain-03-networking-traffic
- `34-network-performance-tuning.md` — Add SLOs, benchmarking methodology, cloud NIC/queue tuning.
- `18-network-encryption-mtls.md` — Add cert-rotation runbook and key-escrow section.
- `32-multi-cluster-networking.md` — Add multi-cluster DR, security, and failover patterns.
- `39-csi-cni-version-matrix.md` — Move CSI sections to domain-04.

### domain-04-storage-data
- `01-k8s-storage/13-storage-security-compliance.md` — Replace deprecated PSP with PSA/Kyverno/OPA examples.
- `01-k8s-storage/15-storage-disaster-recovery.md` — Remove fictional CRDs; rewrite with Velero/VolSync/cloud replication.
- `01-k8s-storage/08-storage-performance-tuning.md` & `12-storage-monitoring-alerting.md` — Deduplicate.
- `01-k8s-storage/10-storage-backup-disaster-recovery.md` & `15-storage-disaster-recovery.md` — Differentiate or merge.
- Move `02-pvc-expansion-guide.md` into `01-k8s-storage/`.

### domain-05-security-compliance
- `01-identity-access/01-authentication-authorization-system.md` — Remove duplicate frontmatter block.
- `P3-11-security-incident-sop-compliance-checklist.md` — Fix stale cross-references.
- `05-supply-chain/10-image-security-scanning.md` & `13-image-security-scanning.md` — Deduplicate.
- `07-incident-response/20-incident-response-process.md` — Add `crictl`/`ctr` equivalents to Docker forensics commands.
- `02-network-security/21-multicluster-security.md` — Replace custom controller with OCM/Rancher Fleet/Argo CD references.
- `03-runtime-security/99-falco-runtime-security-guide.md` — Fix non-standard metrics and legacy Grafana panels.
- `06-compliance/09-security-hardening-production.md` — Add upgrade, CA rotation, and HA considerations.

### domain-06-observability
- `README.md` — Correct file counts and `related_docs` paths.
- `10-monitoring-metrics-prometheus.md`, `26-troubleshooting-tools.md`, `27-performance-profiling-tools.md`, `03-loki-enterprise-log-aggregation.md`, `99-kubernetes-v1.33-observability-guide.md` — Remove duplicate frontmatter.
- `98-merged-indexes/FINAL-QUALITY-ASSESSMENT.md` & `UPDATED-QUALITY-REPORT.md` — Update stale filename references.
- `04-tracing/03-opentelemetry-distributed-tracing.md` — Expand Collector production patterns.

### domain-07-platform-engineering
- `README.md` — Fix broken wikilink.
- `operate/15-production-troubleshooting.md` — Add severity routing, escalation, runbook templates.
- `operate/25-virtual-clusters.md` — Correct title and expand operational depth.
- `operate/16-platform-upgrade-migration.md` — Remove duplicated overview section; refresh metadata.
- `operate/99-kubernetes-v1.33-platform-ops-guide.md` — Reference from README and align `k8s_versions`.

### domain-08-release-change-management
- `README.md` — Fix broken wikilink.
- `topic-deployment/README.md` — Update timestamp and verify decision tree.
- `04-testing-quality/01-selenium-enterprise-automation.md` — Consolidate duplicate YAML frontmatter.
- `01-gitops/99-*` guide files — Clarify scope vs numbered enterprise files or merge.
- `04-production-environment-deployment.md` — Extract upgrade and cert-rotation into dedicated runbooks.
- `03-change-management/22-change-management-process.md` — Audit duplication with newer files.

### domain-09-reliability-engineering
- `09-disaster-recovery-playbooks/01-dr-scenarios-catalog.md` — Add scenario → owner → runbook → last-drilled mapping.
- `09-disaster-recovery-playbooks/02-az-failure-playbook.md` — Add per-provider commands, topology spread, StatefulSet PVC/AZ handling.
- `08-performance-testing/*.md` — Add soak/stress/spike methodology and SLO correlation.
- `05-chaos-engineering/*.md` — Add safety rails, blast-radius controls, kill-switch mechanics.
- `02-disaster-recovery/18-cross-region-disaster-recovery.md` — Fix frontmatter and Istio YAML; add RTO/RPO tradeoffs.
- `03-slo-sli-guide.md` (root) — Convert to pointer or remove to avoid drift with `04-slo-sli/`.

### domain-10-troubleshooting-diagnostics
- `README.md` — Accurately list all subdirectories.
- `SUMMARY.md` — Replace with domain-specific summary or rename.
- `topic-structural-trouble-shooting/**/*.md` — Clean duplicate frontmatter.
- `topic-skills/07-pvc-storage-failure 2.md` — Merge or remove duplicate.
- `tools/README.md` — Add modern tools (`kubectl debug`, `inspektor-gadget`, `kor`, `krelay`).

### domain-11-production-operations
- `01-finops/13-kubernetes-cost-governance.md` — Remove simulated data; add ACK pricing integration.
- `04-green-computing/15-green-computing-sustainability.md` — Add security warnings for privileged DaemonSet; clarify non-existent scheduler plugins.
- `02-governance/14-resource-quota-management.md` — Add hierarchical namespaces, team onboarding SOP, chargeback workflow.
- `01-production-sre-daily-ops.md` — Link checks to runbooks; add severity thresholds.
- `03-incident-response/23-incident-response-handling.md` — Remove `componentstatuses`; update metadata; clarify ownership vs `04-incident-response-template.md`.
- `ticket-cases/` — Deduplicate and add canonical taxonomy/index.

### domain-12-cloud-providers
- `README.md` — Update directory table to include subdirs 09–15; fix legacy names.
- Provider overviews (AWS, GKE, Azure, Tencent, Huawei, UCloud, IBM, Oracle, Volcengine, Ctyun, ECloud) — Remove duplicate frontmatter; refresh versions; add upgrade, DR, observability, security, IAM, storage, cost, and troubleshooting sections.
- `08-multi-cloud/00-multi-cloud-hybrid-deployment-strategy.md` — Add executable failover, secret sync, image replication, global LB runbooks.

### domain-13-container-runtime
- `README.md` & `98-merged-indexes/index.md` — Add `03-containerd-cri-o/`, `04-image-build/`, and top-level `01-containerd-deep-guide.md`.
- `01-docker/01-docker-architecture-overview.md` — Remove stale links and unrelated business-vertical See Also list.
- `01-docker/07-docker-security-best-practices.md` — Add K8s runtime security, seccomp, AppArmor/SELinux, Falco.
- `01-docker/08-docker-troubleshooting-guide.md` — Add containerd/CRI-O shim and CNI/CSI interactions.
- `01-docker/09-docker-performance-monitoring.md` — Add containerd/CRI-O and kubelet runtime metrics.
- `01-containerd-deep-guide.md` — Merge with `03-containerd-cri-o/01-containerd-production-operations.md` or convert to landing page.

### domain-14-ai-ml-infra
- `README.md` — Remove `03-mlops/` reference or create the directory.
- `02-ai-agents/` vs `02-ai-agents/` — Consolidate or clearly differentiate.
- `01-ai-infra/17-llm-inference-serving.md` — Fix malformed Istio VirtualService YAML.
- `01-ai-infra/11-ai-security-model-protection.md` — Add K8s-native controls (PSS, NetworkPolicy, OPA).
- `01-ai-infra/14-troubleshooting-performance.md` — Add severity/escalation workflow and rollback steps.
- `01-ai-infra/99-kubeflow-ai-platform-guide.md` — Add HA, external auth, and upgrade notes.

### domain-15-specialized-tech
- `01-edge-computing/99-kubernetes-developer-toolchain-guide.md` — Move to domain-17 or correct category.
- `03-edge-computing-production-deployment.md` — Rename consistently; review privileged advice.
- `03-extensions/11-service-mesh-overview.md` & `12-service-mesh-advanced.md` — Move to domain-03 or cross-reference.
- `03-extensions/09-gitops-workflow-argocd.md` — Move to domain-08.
- `03-extensions/15-monitoring-alerting-system.md` — Move to domain-06 or domain-11.
- `03-extensions/16-security-compliance-management.md` — Move to domain-05.
- `03-extensions/13-kubernetes-operations-fundamentals.md` — Move to domain-11.
- `03-extensions/14-multi-cluster-management.md` — Move to domain-12.

### domain-16-database-middleware
- `README.md` — Either populate `02-middleware/` or remove the entry.
- `98-merged-indexes/index.md` — Add missing file links.
- `01-databases/99-cloudnativepg-enterprise-guide.md` — Fix "Kafka CRD" typo.
- `01-databases/01-mysql-enterprise-database.md` — Fix `if` syntax; replace cleartext passwords with Secret references.
- `04-time-series-db/01-prometheus-tsdb-deep-dive.md` — Expand retention/remote-write/cardinality; fix broken link.
- `05-operator-management/01-database-operator-patterns.md` & `02-operator-comparison-mysql-postgres-redis.md` — Add failure modes and maturity comparison.

### domain-17-system-foundation
- `01-linux/07-linux-security-hardening.md` — Remove PSP references; add Kyverno/OPA-Gatekeeper as replacements.
- `01-linux/06-linux-performance-tuning.md` — Add conntrack interaction warnings; avoid deprecated `tcp_tw_recycle`.
- `01-linux/08-linux-container-fundamentals.md` — Add containerd/CRI-O equivalents; expand User Namespaces.
- `01-linux/04-linux-networking-configuration.md` — Add nftables/iptables-nft section.
- `02-hardware/16-kubernetes-hardware-troubleshooting.md` — Provide privileged debug DaemonSet; fix `node_edac` alert expressions; add PDB/StatefulSet warnings for drain.
- `03-kubernetes-events/01-event-system-architecture.md` — Add `events.k8s.io/v1` migration guidance; harden exporter example.
- `README.md` — Enumerate `topic-cheat-sheet/` and `topic-dictionary/`.

### domain-18-manifests-patterns
- `README.md` — Add `98-merged-indexes/` and clarify "patterns" scope.
- `01-yaml-reference/03-pod-specification-complete.md` — Remove duplicate frontmatter.
- `01-yaml-reference/23-pod-security-standards.md` — Fix title typo.
- `01-yaml-reference/36-ecosystem-kustomize-helm-argocd.md` — Add AppProject, ApplicationSet, sync waves, secrets.
- `01-yaml-reference/27-hpa-autoscaling-v2.md` — Add VPA, KEDA, Karpenter manifest patterns.
- `01-yaml-reference/22-networkpolicy-reference.md` — Add default-deny production template and CNI gotchas.
- `01-yaml-reference/33-kubeadm-cluster-bootstrap.md` — Add encryption config, audit policy, OIDC hardening.

### domain-19-landscape-references
- `README.md` — Accurately describe empty CNCF dirs and paper archive; fix broken links.
- `topic-release-notes/README.md` — Clarify archive location or move files to `topic-release-notes/`.
- `98-merged-indexes/` — Complete consolidation of Domain-34 imports.
- `01-cncf-landscape/` — Populate with graduated/incubating/sandbox guides.

### domain-20-application-patterns
- `README.md` — Point to `topic-application-architecture/` instead of non-existent `01-reference-architectures/`.
- `topic-application-architecture/README.md` — Correct file count, typos, `related_domains`, difficulty tag, and last-update date.
- `98-merged-indexes/MOC-from-domain-42.md` & `README-from-domain-42.md` — Regenerate or remove.
- Top 10 verticals (e-commerce, fintech, SaaS multi-tenant, microservice governance, DevOps, AI/ML inference, IoT, gaming, IM/RTC, data mid-platform) — Add HA, DR, observability/SLO, security, release/rollback, and runbook sections.
- Shallow/template-heavy verticals — Apply minimum production-readiness template or retire/merge.

---

## 6. Suggested Content Roadmap

| Quarter | Focus | Primary Domains |
|---|---|---|
| **Q3 2026** | Operational runbooks: incident response, cert rotation, cluster upgrade, DR/BC, control-plane resilience | 01, 07, 09, 11 |
| **Q3 2026** | Multi-cluster / fleet / GitOps at scale | 08, 12, 18 |
| **Q4 2026** | Production readiness review, FinOps, observability/SLO operations | 07, 09, 11, 18, 20 |
| **Q4 2026** | Cloud-provider operational parity (AWS/GCP/Azure/Alibaba/Tencent/Huawei) | 12 |
| **Q1 2027** | AI/ML ops runbooks and GPU/AI observability | 14, 02, 06 |
| **Q1 2027** | Edge/Wasm production, container runtime security, supply-chain security | 13, 15, 05 |
| **Ongoing** | Structural hygiene: deduplicate frontmatter, fix READMEs, refresh metadata, remove stale merged indexes | All |

---

*Generated from subagent audit reports on 2026-07-01. This is a prioritization reference, not a modification of existing wiki files.*


<!-- risk-assessed -->
