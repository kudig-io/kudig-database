---
title: Wiki Log
description: '- [2026-05-21] RELEASE-NOTES-INDEX-INGEST domain-19-landscape-references/_archived-release-notes/: 创建 8 个类别索引页 —
  references/release-notes-observability.md (可观测性, 374 篇)、release-notes-security.md (安全, 218 '
category: general
tags:
- k8s
- etcd
- kubelet
- prometheus
- grafana
- jaeger
- istio
- envoy
- cilium
- calico
last_updated: 2026-05-21
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 45min
intent_queries:
- Wiki Log 是什么
- 如何 Wiki Log
trigger_keywords:
- Wiki
- Log
prerequisites:
- kubectl-basics
- helm-basics
- service-mesh-basics
- prometheus-basics
- monitoring-basics
- ebpf-basics
- cilium-basics
- cni-basics
- etcd-basics
- tls-basics
- policy-basics
- backup-basics
- logging-basics
- tracing-basics
- observability-basics
---

# Wiki Log

- [2026-05-21T23:08:00+08:00] CROSS_LINK phase=complete-orphan-elimination pages_scanned=4672 links_added=9121 pages_modified=~2400 orphans_remaining=0 (0.0%) cohesion=1.0000 TARGET_ACHIEVED. Waves: emitter-backlinks(3773), index-wikilink-conversion(1428), index-backlinks(2455), release-note-entity-links(743+68), generic-orphan-inline(372+235), final-cleanup(47), calico-backlinks(9). All 2,659 original orphans resolved.

- [2026-05-21T17:15:00Z] CROSS_LINK phase=final-push pages_scanned=4003 total_links_added_fixed=19937 pages_modified=5685 orphans_remaining=160 (4.0%) TARGET_ACHIEVED


- [2026-05-21T17:00:00Z] CROSS_LINK phase=deep-dive pages_scanned=3993 total_links_added_fixed=14612 pages_modified=4323 orphans_remaining=255 (6.4%)


- [2026-05-21T17:30:00+08:00] WIKI_SYNTHESIZE pages_scanned=578 synthesis_created=10 candidates_skipped=0 (round 2: 10 skipped candidates from round 1)


- [2026-05-21T16:45:00Z] CROSS_LINK phase=final pages_scanned=3982 links_added=7045 pages_modified=2433 orphans_remaining=954 domain_orphans=25 release_orphans=1


- [2026-05-21T17:00:00+08:00] WIKI_[[_meta/dashboard|dashboard]] name=dashboard tool=dataview views=9 filter=concepts/entities/skills/references/synthesis


- [2026-05-21T16:30:00Z] CROSS_LINK phase=domain-release-notes pages_scanned=3982 links_added=4918 pages_modified=3486 domain_orphans_remaining=586 release_orphans_remaining=355


- [2026-05-21T16:07:01+08:00] WIKI_SYNTHESIZE pages_scanned=578 synthesis_created=5 candidates_skipped=10


- [2026-05-21T16:15:00Z] CROSS_LINK phase=backlink pages_scanned=3981 links_added=4760 typed_relations_written=4745 pages_modified=969 orphans_remaining=2752 misc_affinity_updated=0 promotion_candidates=0


- [2026-05-21T16:06:00Z] CROSS_LINK pages_scanned=3979 links_added=3824 typed_relations_written=3824 pages_modified=793 orphans_remaining=2757 misc_affinity_updated=0 promotion_candidates=0


- [2026-05-21] RELEASE-NOTES-INDEX-INGEST domain-19-landscape-references/_archived-release-notes/: 创建 8 个类别索引页 — references/release-notes-observability.md (可观测性, 374 篇)、release-notes-security.md (安全, 218 篇)、release-notes-cli-tools.md (CLI 工具, 187 篇)、release-notes-cicd-gitops.md (CI/CD & GitOps, 171 篇)、release-notes-networking.md (网络, 157 篇)、release-notes-core-deps.md (核心依赖, 83 篇)、release-notes-storage.md (存储, 76 篇)、release-notes-kubernetes.md (Kubernetes, 55 篇)。33 个项目覆盖 1,321 个发布说明源文件（MOC.md + README.md 已存在）。manifest 已含全部 1,323 个 domain-19-landscape-references/_archived-release-notes/ 条目及 SHA-256 哈希。每页包含项目总览表、版本覆盖、Breaking Changes 汇总、wikilink 到对应 entities/ 页面。
- [2026-05-21] TOPIC-FUNCTIONS-INGEST domain-02-workloads-applications/topic-functions/: 创建 14 个 wiki 页面 — 9 skills (kubeadm-cluster-lifecycle, kubeadm-cluster-deletion, kubeadm-ha-cluster-setup, kubelet-certificate-rotation, node-drain-and-maintenance, kubelet-eviction-mechanism, deployment-rolling-update, deployment-canary-and-bluegreen, deployment-workload-selection) + 4 concepts (kubernetes-pki-certificate-system, cni-networking-model, node-lifecycle-management, deployment-controller-architecture) + 1 synthesis (kubeadm-cluster-operations)。82 个源文件已摄入，覆盖 5 个子专题：cluster-cert (17)、cluster-create (25)、cluster-delete (13)、deployment-create (10)、node-create (17)。覆盖领域：kubeadm init 12 阶段生命周期、PKI 三组 CA 体系、证书轮换与 TLS Bootstrap、高可用部署（stacked/external etcd）、节点注册与生命周期管理、工作负载控制器选型、Deployment 滚动更新、金丝雀/蓝绿发布、kubectl drain 与节点维护、kubelet 资源驱逐机制、集群删除与 reset 清理流程、CNI 网络模型与 DNS 解析。
- [2026-05-21] RELEASE-NOTES-INGEST domain-19-landscape-references/_archived-release-notes/: 创建 13 个 wiki 页面 — 8 concepts (kubernetes-version-evolution, core-dependency-version-matrix, gitops-tool-evolution, service-mesh-evolution, observability-stack-evolution, security-tool-evolution, storage-tool-evolution, cli-tools-evolution) + 3 entities (kubernetes-changelog, core-deps-changelog, ecosystem-changelog) + 2 references (version-upgrade-guide, release-notes-reading-guide)。1,321 个源文件已摄入。覆盖：Kubernetes v0.x-v1.36 版本演进、5 大核心依赖版本矩阵、GitOps 工具（Argo CD/Flux/Tekton）、服务网格（Istio/Envoy/Cilium/Linkerd/Calico）、可观测性栈（Prometheus/Grafana/Loki/Thanos/OpenTelemetry）、安全工具（Falco/OPA/Gatekeeper/Trivy/cert-manager）、存储工具（Rook/Longhorn/Velero）、CLI 工具（Helm/Kind/Kops/Minikube/Kustomize）。
- [2026-05-21] FULL-INGEST domain-10-troubleshooting-diagnostics/topic-fta/domain-02-workloads-applications/topic-functions/domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/topic-skills: 创建 81 个 wiki 页面。222 个源文件全部摄入。FTA故障树 list/ → skills/ (44个独立页面), 操作函数 → references/ (5个分组页面: cluster-cert/cluster-create/cluster-delete/deployment-create/node-create), 结构化排查 → skills/ (13个分组页面: control-plane/node-components/networking/storage/workloads/security-auth/resources-scheduling/cluster-operations/cloud-provider/command-output/ai-ml-workloads/gitops-devops/monitoring-observability), 诊断Skill → skills/ (19个页面含评估/skill-set/skills-run)。manifest 总计 1944 条目。
- [2026-05-21] BEST-PRACTICES-INGEST domain-11-production-operations/topic-best-practices/: 创建 12 个 wiki 页面 — 1 concept (k8s-production-best-practices) + 11 skills (cluster/network/storage/monitoring/logging/tracing/deployment/scaling/disaster-recovery/network-security/pod-security guides)。更新 1 现有页面 (secrets-management)。13 个源文件已摄入。覆盖：生产最佳实践模式、集群配置、CNI 选型、存储分层、可观测性栈（Prometheus/EFK/Jaeger）、部署策略（滚动/蓝绿/金丝雀）、自动扩缩容（HPA/VPA/CA）、灾难恢复（Velero）、网络安全（NetworkPolicy/mTLS）、Pod 安全（PSS/seccomp）、密钥管理（etcd 加密/Vault/External Secrets）。
- [2026-05-21] TOPIC-LEARN VERIFICATION: 验证 27 个 learn-* 页面已完整覆盖 137 个 domain-11-production-operations/topic-learn/ 源文件。index.md 已添加 learn-* 页面完整列表。manifest 已含 137 domain-11-production-operations/topic-learn/ 条目及 SHA-256 哈希。
- [2026-05-21] FULL-INGEST domain-01-cluster-fundamentals through domain-03-networking-traffic + topic-*: Created 91 wiki pages across concepts/ (39), entities/ (23), skills/ (15), references/ (11), synthesis/ (3). 1,093 source documents ingested. Coverage: K8s architecture, Docker, Linux containers, networking, storage, security, GitOps, IaC, service mesh, observability, FTA methodology, diagnostic skills, agent orchestration, CNCF ecosystem.
- [2026-05-21] Wiki enhancement pass — all 96 original pages enhanced with typed relationships, provenance fields, confidence recalculation.
- [2026-05-21] REMAINING-INGEST: Created 5 wiki pages — k8s-knowledge-map, k8s-difficulty-index, kudig-man-pages-index, kudig-prompts-catalog.md|kudig-prompts-catalog]], kudig-templates-catalog.md|kudig-templates-catalog]]. Total: 101 pages.
- [2026-05-21] WIKI_SYNTHESIZE 扫描页面=63 新增综合=5 跳过候选=10 — 综合页面全部中文输出：eBPF x 运行时安全、GitOps x 平台工程、纵深防御 x 供应链安全、服务网格 x 零信任安全、IaC x 多集群管理。
- [2026-05-21] FULL-INGEST domain-01-cluster-fundamentals through domain-12: Created 35 wiki pages across concepts/ (12), entities/ (11), skills/ (8), references/ (4). Coverage: architecture, design principles, control plane, workloads, networking, storage, security, observability, autoscaling, extensions, AI infra, troubleshooting methodology.
- [2026-05-21] FULL-INGEST topic-*/docs: Created 22 wiki pages (8 skills, 7 references, 3 concepts, 4 synthesis). Coverage: FTA methodology, diagnostic execution engine, symptom vector matching, agent orchestration, runbook automation, troubleshooting frameworks.
- [2026-05-21] INIT vault_path="/Users/allengaller/Documents/GitHub/kudig-io/kudig-database" categories=concepts,entities,skills,references,synthesis,journal,projects
- [2026-05-21] Wiki-setup — Obsidian Wiki vault structure initialized
- [2026-05-20] Obsidian Wiki 模式改进第二轮 — intent_queries 扩展 (715 文件), chunk markers (893 文件), wikilinks (1,041 文件)
- [2026-05-20] Obsidian Wiki 模式改进第一轮 — MOC (63 个), Frontmatter 100%, 双向链接, aliases, 决策树, 场景导航
- [2026-05-21] DIGEST period="1d" source_docs=3543 wiki_pages=101 domains=40 topics=21 cross_refs=15306 saved=true
- [2026-05-21] CROSS_LINK pages_scanned=166 links_added=60+ typed_relations_written=26 pages_modified=35 orphans_remaining=19 focus=FTA-skills,concepts,references,docs,synthesis hub_backlinks=8


## Wiki Ingest - 2026-05-21

批量摄入 51 个源文件，生成 51 个 wiki 页面。

### 摄入详情

- [2026-05-21] 批量摄入 domain-17-system-foundation/topic-dictionary/configuration/ → references/configuration-terms.md (6 词条)
- [2026-05-21] 批量摄入 domain-17-system-foundation/topic-dictionary/fundamentals/ → references/fundamentals-terms.md (24 词条)
- [2026-05-21] 批量摄入 domain-17-system-foundation/topic-dictionary/multi-cloud/ → references/multi-cloud-terms.md (3 词条)
- [2026-05-21] 批量摄入 domain-17-system-foundation/topic-dictionary/networking/ → references/networking-terms.md (17 词条)
- [2026-05-21] 批量摄入 domain-17-system-foundation/topic-dictionary/observability/ → references/observability-terms.md (10 词条)
- [2026-05-21] 批量摄入 domain-17-system-foundation/topic-dictionary/operations/ → references/operations-terms.md (20 词条)
- [2026-05-21] 批量摄入 domain-17-system-foundation/topic-dictionary/platform-engineering/ → references/platform-engineering-terms.md (19 词条)
- [2026-05-21] 批量摄入 domain-17-system-foundation/topic-dictionary/root/ → references/root-terms.md (2 词条)
- [2026-05-21] 批量摄入 domain-17-system-foundation/topic-dictionary/scheduling/ → references/scheduling-terms.md (16 词条)
- [2026-05-21] 批量摄入 domain-17-system-foundation/topic-dictionary/security/ → references/security-terms.md (27 词条)
- [2026-05-21] 批量摄入 domain-17-system-foundation/topic-dictionary/specialized-workloads/ → references/specialized-workloads-terms.md (10 词条)
- [2026-05-21] 批量摄入 domain-17-system-foundation/topic-dictionary/storage/ → references/storage-terms.md (17 词条)
- [2026-05-21] 批量摄入 domain-17-system-foundation/topic-dictionary/tooling/ → references/tooling-terms.md (3 词条)
- [2026-05-21] 批量摄入 domain-17-system-foundation/topic-dictionary/workloads/ → references/workloads-terms.md (33 词条)
- [2026-05-21] 摄入 domain-11-production-operations/topic-learn/fundamentals/01-what-is-kubernetes.md → skills/learn-01-what-is-kubernetes.md
- [2026-05-21] 摄入 domain-11-production-operations/topic-learn/fundamentals/02-pod-basics.md → skills/learn-02-pod-basics.md
- [2026-05-21] 摄入 domain-11-production-operations/topic-learn/fundamentals/03-deployment-basics.md → skills/learn-03-deployment-basics.md
- [2026-05-21] 摄入 domain-11-production-operations/topic-learn/fundamentals/04-service-basics.md → skills/learn-04-service-basics.md
- [2026-05-21] 摄入 domain-11-production-operations/topic-learn/fundamentals/05-ingress-basics.md → skills/learn-05-ingress-basics.md
- [2026-05-21] 摄入 domain-11-production-operations/topic-learn/fundamentals/06-configmap-secret.md → skills/learn-06-configmap-secret.md
- [2026-05-21] 摄入 domain-11-production-operations/topic-learn/fundamentals/07-namespace-resource-quota.md → skills/learn-07-namespace-resource-quota.md
- [2026-05-21] 摄入 domain-11-production-operations/topic-learn/fundamentals/08-pv-pvc-basics.md → skills/learn-08-pv-pvc-basics.md
- [2026-05-21] 摄入 domain-11-production-operations/topic-learn/fundamentals/09-hpa-basics.md → skills/learn-09-hpa-basics.md
- [2026-05-21] 摄入 domain-11-production-operations/topic-learn/fundamentals/10-health-check.md → skills/learn-10-health-check.md
- [2026-05-21] 摄入 domain-11-production-operations/topic-learn/fundamentals/11-job-cronjob.md → skills/learn-11-job-cronjob.md
- [2026-05-21] 摄入 domain-11-production-operations/topic-learn/fundamentals/12-common-problems.md → skills/learn-12-common-problems.md
- [2026-05-21] 摄入 domain-11-production-operations/topic-learn/fundamentals/13-daemonset-basics.md → skills/learn-13-daemonset-basics.md
- [2026-05-21] 摄入 domain-11-production-operations/topic-learn/fundamentals/14-statefulset-basics.md → skills/learn-14-statefulset-basics.md
- [2026-05-21] 摄入 domain-11-production-operations/topic-learn/fundamentals/15-scheduling-basics.md → skills/learn-15-scheduling-basics.md
- [2026-05-21] 批量摄入 domain-11-production-operations/topic-learn/inner-training/ → skills/learn-inner-training.md (46 篇)
- [2026-05-21] 摄入 domain-11-production-operations/topic-learn/oncall-qa/oncall-quick-qa.md → skills/learn-oncall-quick-qa.md
- [2026-05-21] 批量摄入 domain-11-production-operations/topic-learn/public-training/ → skills/learn-public-training.md (65 篇)
- [2026-05-21] 摄入 domain-11-production-operations/topic-learn/quick-start/01-day-one-checklist.md → skills/learn-01-day-one-checklist.md
- [2026-05-21] 摄入 domain-11-production-operations/topic-learn/quick-start/02-first-ticket-guide.md → skills/learn-02-first-ticket-guide.md
- [2026-05-21] 摄入 domain-11-production-operations/topic-learn/quick-start/03-oncall-handoff.md → skills/learn-03-oncall-handoff.md
- [2026-05-21] 摄入 domain-11-production-operations/topic-learn/quick-start/04-debug-tools-setup.md → skills/learn-04-debug-tools-setup.md
- [2026-05-21] 摄入 domain-11-production-operations/topic-learn/quick-start/README.md → skills/learn-README.md
- [2026-05-21] 摄入 domain-11-production-operations/topic-learn/resources/analogy-dictionary.md → skills/learn-analogy-dictionary.md
- [2026-05-21] 摄入 domain-11-production-operations/topic-learn/resources/lecturer-persona.md → skills/learn-lecturer-persona.md
- [2026-05-21] 批量摄入 domain-11-production-operations/topic-learn/root/ → skills/learn-root.md (2 篇)
- [2026-05-21] 摄入 domain-11-production-operations/topic-learn/troubleshooting/decision-tree-mermaid.md → skills/learn-decision-tree-mermaid.md
- [2026-05-21] 批量摄入 domain-11-production-operations/topic-best-practices/infrastructure/ → concepts/bp-infrastructure.md (3 篇)
- [2026-05-21] 批量摄入 domain-11-production-operations/topic-best-practices/observability/ → concepts/bp-observability.md (3 篇)
- [2026-05-21] 批量摄入 domain-11-production-operations/topic-best-practices/operations/ → concepts/bp-operations.md (3 篇)
- [2026-05-21] 摄入 domain-11-production-operations/topic-best-practices/MOC.md → concepts/bp-MOC.md
- [2026-05-21] 摄入 domain-11-production-operations/topic-best-practices/README.md → concepts/bp-README.md
- [2026-05-21] 摄入 domain-11-production-operations/topic-best-practices/common-best-practices.md → concepts/bp-common-best-practices.md
- [2026-05-21] 批量摄入 domain-11-production-operations/topic-best-practices/security/ → concepts/bp-security.md (3 篇)
- [2026-05-21] 批量摄入 domain-14-ai-ml-infra/topic-ai-agent/openclaw-workspace/ → concepts/ai-agent-openclaw-workspace.md (7 篇)
- [2026-05-21] 摄入 domain-14-ai-ml-infra/topic-ai-agent/MOC.md → concepts/ai-agent-MOC.md
- [2026-05-21] 摄入 domain-14-ai-ml-infra/topic-ai-agent/README.md → concepts/ai-agent-README.md


## Wiki Ingest Batch — 2026-05-21

**统计**: 创建 252 | 更新 8 | 跳过 0 | 错误 0

```
[CREATE ] domain-19-landscape-references/graduated/tikv/tikv.md -> entities/tikv.md
[CREATE ] domain-19-landscape-references/graduated/dragonfly/dragonfly.md -> entities/dragonfly.md
[CREATE ] domain-19-landscape-references/graduated/cloudevents/cloudevents.md -> entities/cloudevents.md
[CREATE ] domain-19-landscape-references/graduated/fluentd/fluentd.md -> entities/fluentd.md
[CREATE ] domain-19-landscape-references/graduated/harbor/harbor.md -> entities/harbor.md
[CREATE ] domain-19-landscape-references/graduated/rook/rook.md -> entities/rook.md
[CREATE ] domain-19-landscape-references/graduated/istio/02-istio-advanced-traffic-management.md -> entities/02-istio-advanced-traffic-management.md
[UPDATE ] domain-19-landscape-references/graduated/istio/istio.md -> entities/istio.md
[CREATE ] domain-19-landscape-references/graduated/istio/03-istio-security-hardening.md -> entities/03-istio-security-hardening.md
[UPDATE ] domain-19-landscape-references/graduated/flux/flux.md -> entities/flux.md
[CREATE ] domain-19-landscape-references/graduated/jaeger/jaeger.md -> entities/jaeger.md
[CREATE ] domain-19-landscape-references/graduated/in-toto/in-toto.md -> entities/in-toto.md
[CREATE ] domain-19-landscape-references/graduated/vitess/vitess.md -> entities/vitess.md
[CREATE ] domain-19-landscape-references/graduated/tuf/tuf.md -> entities/tuf.md
[CREATE ] domain-19-landscape-references/graduated/cert-manager/cert-manager.md -> entities/cert-manager.md
[CREATE ] domain-19-landscape-references/graduated/knative/knative.md -> entities/knative.md
[CREATE ] domain-19-landscape-references/graduated/kubeedge/kubeedge.md -> entities/kubeedge.md
[CREATE ] domain-19-landscape-references/graduated/containerd/07-containerd-disaster-recovery.md -> entities/07-containerd-disaster-recovery.md
[CREATE ] domain-19-landscape-references/graduated/containerd/04-containerd-upgrade-migration.md -> entities/04-containerd-upgrade-migration.md
[UPDATE ] domain-19-landscape-references/graduated/containerd/containerd.md -> entities/containerd.md
[CREATE ] domain-19-landscape-references/graduated/containerd/05-containerd-windows-support.md -> entities/05-containerd-windows-support.md
[CREATE ] domain-19-landscape-references/graduated/containerd/02-containerd-v2-features.md -> entities/02-containerd-v2-features.md
[CREATE ] domain-19-landscape-references/graduated/containerd/08-containerd-multi-tenant.md -> entities/08-containerd-multi-tenant.md
[CREATE ] domain-19-landscape-references/graduated/containerd/03-containerd-security-hardening.md -> entities/03-containerd-security-hardening.md
[CREATE ] domain-19-landscape-references/graduated/containerd/06-containerd-observability.md -> entities/06-containerd-observability.md
[CREATE ] domain-19-landscape-references/graduated/cri-o/cri-o.md -> entities/cri-o.md
[CREATE ] domain-19-landscape-references/graduated/envoy/envoy.md -> entities/envoy.md
[CREATE ] domain-19-landscape-references/graduated/spire/spire.md -> entities/spire.md
[CREATE ] domain-19-landscape-references/graduated/keda/keda.md -> entities/keda.md
[UPDATE ] domain-19-landscape-references/graduated/crossplane/crossplane.md -> entities/crossplane.md
[CREATE ] domain-19-landscape-references/graduated/cubefs/cubefs.md -> entities/cubefs.md
[CREATE ] domain-19-landscape-references/graduated/linkerd/linkerd.md -> entities/linkerd.md
[CREATE ] domain-19-landscape-references/graduated/opa/opa.md -> entities/opa.md
[UPDATE ] domain-19-landscape-references/graduated/etcd/etcd.md -> entities/etcd.md
[CREATE ] domain-19-landscape-references/graduated/dapr/dapr.md -> entities/dapr.md
[UPDATE ] domain-19-landscape-references/graduated/falco/falco.md -> entities/falco.md
[CREATE ] domain-19-landscape-references/graduated/spiffe/spiffe.md -> entities/spiffe.md
[CREATE ] domain-19-landscape-references/graduated/prometheus/prometheus.md -> entities/prometheus.md
[CREATE ] domain-19-landscape-references/graduated/prometheus/02-prometheus-promql-advanced.md -> entities/02-prometheus-promql-advanced.md
[CREATE ] domain-19-landscape-references/graduated/prometheus/03-prometheus-ha-deployment.md -> entities/03-prometheus-ha-deployment.md
[CREATE ] domain-19-landscape-references/graduated/kubernetes/kubernetes.md -> entities/kubernetes.md
[CREATE ] domain-19-landscape-references/graduated/coredns/coredns.md -> entities/coredns.md
[CREATE ] domain-19-landscape-references/graduated/argo/argo.md -> entities/argo.md
[UPDATE ] domain-19-landscape-references/graduated/cilium/cilium.md -> entities/cilium.md
[CREATE ] domain-19-landscape-references/graduated/helm/helm.md -> entities/helm.md
[CREATE ] domain-19-landscape-references/incubating/lima/lima.md -> entities/lima.md
[CREATE ] domain-19-landscape-references/incubating/wasmcloud/wasmcloud.md -> entities/wasmcloud.md
[CREATE ] domain-19-landscape-references/incubating/cortex/cortex.md -> entities/cortex.md
[CREATE ] domain-19-landscape-references/incubating/metal3-io/metal3-io.md -> entities/metal3-io.md
[CREATE ] domain-19-landscape-references/incubating/flatcar/flatcar.md -> entities/flatcar.md
[CREATE ] domain-19-landscape-references/incubating/emissary-ingress/emissary-ingress.md -> entities/emissary-ingress.md
[CREATE ] domain-19-landscape-references/incubating/grpc/grpc.md -> entities/grpc.md
[CREATE ] domain-19-landscape-references/incubating/opentelemetry/opentelemetry.md -> entities/opentelemetry.md
[CREATE ] domain-19-landscape-references/incubating/openyurt/openyurt.md -> entities/openyurt.md
[CREATE ] domain-19-landscape-references/incubating/contour/contour.md -> entities/contour.md
[CREATE ] domain-19-landscape-references/incubating/notary-project/notary-project.md -> entities/notary-project.md
[CREATE ] domain-19-landscape-references/incubating/kserve/kserve.md -> entities/kserve.md
[CREATE ] domain-19-landscape-references/incubating/fluid/fluid.md -> entities/fluid.md
[CREATE ] domain-19-landscape-references/incubating/longhorn/longhorn.md -> entities/longhorn.md
[CREATE ] domain-19-landscape-references/incubating/openfga/openfga.md -> entities/openfga.md
[CREATE ] domain-19-landscape-references/incubating/buildpacks/buildpacks.md -> entities/buildpacks.md
[CREATE ] domain-19-landscape-references/incubating/karmada/karmada.md -> entities/karmada.md
[CREATE ] domain-19-landscape-references/incubating/nats/nats.md -> entities/nats.md
[CREATE ] domain-19-landscape-references/incubating/cni/cni.md -> entities/cni.md
[CREATE ] domain-19-landscape-references/incubating/kubescape/kubescape.md -> entities/kubescape.md
[CREATE ] domain-19-landscape-references/incubating/kubevela/kubevela.md -> entities/kubevela.md
[CREATE ] domain-19-landscape-references/incubating/kubevirt/kubevirt.md -> entities/kubevirt.md
[CREATE ] domain-19-landscape-references/incubating/thanos/thanos.md -> entities/thanos.md
[CREATE ] domain-19-landscape-references/incubating/cloud-custodian/cloud-custodian.md -> entities/cloud-custodian.md
[CREATE ] domain-19-landscape-references/incubating/chaos-mesh/chaos-mesh.md -> entities/chaos-mesh.md
[CREATE ] domain-19-landscape-references/incubating/litmus/litmus.md -> entities/litmus.md
[CREATE ] domain-19-landscape-references/incubating/operator-framework/operator-framework.md -> entities/operator-framework.md
[CREATE ] domain-19-landscape-references/incubating/opencost/opencost.md -> entities/opencost.md
[CREATE ] domain-19-landscape-references/incubating/openkruise/openkruise.md -> entities/openkruise.md
[CREATE ] domain-19-landscape-references/incubating/openfeature/openfeature.md -> entities/openfeature.md
[CREATE ] domain-19-landscape-references/incubating/keycloak/keycloak.md -> entities/keycloak.md
[CREATE ] domain-19-landscape-references/incubating/backstage/backstage.md -> entities/backstage.md
[CREATE ] domain-19-landscape-references/incubating/kubeflow/kubeflow.md -> entities/kubeflow.md
[UPDATE ] domain-19-landscape-references/incubating/kyverno/kyverno.md -> entities/kyverno.md
[CREATE ] domain-19-landscape-references/incubating/volcano/volcano.md -> entities/volcano.md
[CREATE ] domain-19-landscape-references/incubating/artifact-hub/artifact-hub.md -> entities/artifact-hub.md
[CREATE ] domain-19-landscape-references/incubating/strimzi/strimzi.md -> entities/strimzi.md
[CREATE ] domain-19-landscape-references/sandbox/opengitops/opengitops.md -> entities/opengitops.md
[CREATE ] domain-19-landscape-references/sandbox/kubeclipper/kubeclipper.md -> entities/kubeclipper.md
[CREATE ] domain-19-landscape-references/sandbox/devfile/devfile.md -> entities/devfile.md
[CREATE ] domain-19-landscape-references/sandbox/tremor/tremor.md -> entities/tremor.md
[CREATE ] domain-19-landscape-references/sandbox/kubestellar/kubestellar.md -> entities/kubestellar.md
[CREATE ] domain-19-landscape-references/sandbox/confidential-containers/confidential-containers.md -> entities/confidential-containers.md
[CREATE ] domain-19-landscape-references/sandbox/tinkerbell/tinkerbell.md -> entities/tinkerbell.md
[CREATE ] domain-19-landscape-references/sandbox/network-service-mesh/network-service-mesh.md -> entities/network-service-mesh.md
[CREATE ] domain-19-landscape-references/sandbox/trickster/trickster.md -> entities/trickster.md
[CREATE ] domain-19-landscape-references/sandbox/logging-operator/logging-operator.md -> entities/logging-operator.md
[CREATE ] domain-19-landscape-references/sandbox/pixie/pixie.md -> entities/pixie.md
[CREATE ] domain-19-landscape-references/sandbox/serverless-devs/serverless-devs.md -> entities/serverless-devs.md
[CREATE ] domain-19-landscape-references/sandbox/krkn/krkn.md -> entities/krkn.md
[CREATE ] domain-19-landscape-references/sandbox/oras/oras.md -> entities/oras.md
[CREATE ] domain-19-landscape-references/sandbox/antrea/antrea.md -> entities/antrea.md
[CREATE ] domain-19-landscape-references/sandbox/podman-container-tools/podman-container-tools.md -> entities/podman-container-tools.md
[CREATE ] domain-19-landscape-references/sandbox/holmesgpt/holmesgpt.md -> entities/holmesgpt.md
[CREATE ] domain-19-landscape-references/sandbox/kubefleet/kubefleet.md -> entities/kubefleet.md
[CREATE ] domain-19-landscape-references/sandbox/bfe/bfe.md -> entities/bfe.md
[CREATE ] domain-19-landscape-references/sandbox/spinkube/spinkube.md -> entities/spinkube.md
[CREATE ] domain-19-landscape-references/sandbox/inspektor-gadget/inspektor-gadget.md -> entities/inspektor-gadget.md
[CREATE ] domain-19-landscape-references/sandbox/kaito/kaito.md -> entities/kaito.md
[CREATE ] domain-19-landscape-references/sandbox/werf/werf.md -> entities/werf.md
[CREATE ] domain-19-landscape-references/sandbox/virtual-kubelet/virtual-kubelet.md -> entities/virtual-kubelet.md
[CREATE ] domain-19-landscape-references/sandbox/podman-desktop/podman-desktop.md -> entities/podman-desktop.md
[CREATE ] domain-19-landscape-references/sandbox/eraser/eraser.md -> entities/eraser.md
[CREATE ] domain-19-landscape-references/sandbox/connect-rpc/connect-rpc.md -> entities/connect-rpc.md
[CREATE ] domain-19-landscape-references/sandbox/youki/youki.md -> entities/youki.md
[CREATE ] domain-19-landscape-references/sandbox/kepler/kepler.md -> entities/kepler.md
[CREATE ] domain-19-landscape-references/sandbox/pipecd/pipecd.md -> entities/pipecd.md
[CREATE ] domain-19-landscape-references/sandbox/openebs/openebs.md -> entities/openebs.md
[CREATE ] domain-19-landscape-references/sandbox/clusterpedia/clusterpedia.md -> entities/clusterpedia.md
[CREATE ] domain-19-landscape-references/sandbox/loxilb/loxilb.md -> entities/loxilb.md
[CREATE ] domain-19-landscape-references/sandbox/oauth2-proxy/oauth2-proxy.md -> entities/oauth2-proxy.md
[CREATE ] domain-19-landscape-references/sandbox/copa/copa.md -> entities/copa.md
[CREATE ] domain-19-landscape-references/sandbox/vineyard/vineyard.md -> entities/vineyard.md
[CREATE ] domain-19-landscape-references/sandbox/aeraki-mesh/aeraki-mesh.md -> entities/aeraki-mesh.md
[CREATE ] domain-19-landscape-references/sandbox/chaosblade/chaosblade.md -> entities/chaosblade.md
[CREATE ] domain-19-landscape-references/sandbox/kubewarden/kubewarden.md -> entities/kubewarden.md
[CREATE ] domain-19-landscape-references/sandbox/capsule/capsule.md -> entities/capsule.md
[CREATE ] domain-19-landscape-references/sandbox/drasi/drasi.md -> entities/drasi.md
[CREATE ] domain-19-landscape-references/sandbox/microcks/microcks.md -> entities/microcks.md
[CREATE ] domain-19-landscape-references/sandbox/stacker/stacker.md -> entities/stacker.md
[CREATE ] domain-19-landscape-references/sandbox/kube-rs/kube-rs.md -> entities/kube-rs.md
[CREATE ] domain-19-landscape-references/sandbox/schemahero/schemahero.md -> entities/schemahero.md
[CREATE ] domain-19-landscape-references/sandbox/perses/perses.md -> entities/perses.md
[CREATE ] domain-19-landscape-references/sandbox/kubearmor/kubearmor.md -> entities/kubearmor.md
[CREATE ] domain-19-landscape-references/sandbox/ovn-kubernetes/ovn-kubernetes.md -> entities/ovn-kubernetes.md
[CREATE ] domain-19-landscape-references/sandbox/modelpack/modelpack.md -> entities/modelpack.md
[CREATE ] domain-19-landscape-references/sandbox/devspace/devspace.md -> entities/devspace.md
[CREATE ] domain-19-landscape-references/sandbox/sermant/sermant.md -> entities/sermant.md
[CREATE ] domain-19-landscape-references/sandbox/kmesh/kmesh.md -> entities/kmesh.md
[CREATE ] domain-19-landscape-references/sandbox/ratify/ratify.md -> entities/ratify.md
[CREATE ] domain-19-landscape-references/sandbox/carina/carina.md -> entities/carina.md
[CREATE ] domain-19-landscape-references/sandbox/distribution/distribution.md -> entities/distribution.md
[CREATE ] domain-19-landscape-references/sandbox/kgateway/kgateway.md -> entities/kgateway.md
[CREATE ] domain-19-landscape-references/sandbox/carvel/carvel.md -> entities/carvel.md
[CREATE ] domain-19-landscape-references/sandbox/kubean/kubean.md -> entities/kubean.md
[CREATE ] domain-19-landscape-references/sandbox/vscode-kubernetes-tools/vscode-kubernetes-tools.md -> entities/vscode-kubernetes-tools.md
[CREATE ] domain-19-landscape-references/sandbox/bpfman/bpfman.md -> entities/bpfman.md
[CREATE ] domain-19-landscape-references/sandbox/spin/spin.md -> entities/spin.md
[CREATE ] domain-19-landscape-references/sandbox/kured/kured.md -> entities/kured.md
[CREATE ] domain-19-landscape-references/sandbox/kusionstack/kusionstack.md -> entities/kusionstack.md
[CREATE ] domain-19-landscape-references/sandbox/kubeelasti/kubeelasti.md -> entities/kubeelasti.md
[CREATE ] domain-19-landscape-references/sandbox/containerssh/containerssh.md -> entities/containerssh.md
[CREATE ] domain-19-landscape-references/sandbox/akri/akri.md -> entities/akri.md
[CREATE ] domain-19-landscape-references/sandbox/paralus/paralus.md -> entities/paralus.md
[CREATE ] domain-19-landscape-references/sandbox/kuadrant/kuadrant.md -> entities/kuadrant.md
[CREATE ] domain-19-landscape-references/sandbox/radius/radius.md -> entities/radius.md
[CREATE ] domain-19-landscape-references/sandbox/slimtoolkit/slimtoolkit.md -> entities/slimtoolkit.md
[CREATE ] domain-19-landscape-references/sandbox/k8sgpt/k8sgpt.md -> entities/k8sgpt.md
[CREATE ] domain-19-landscape-references/sandbox/kanister/kanister.md -> entities/kanister.md
[CREATE ] domain-19-landscape-references/sandbox/hami/hami.md -> entities/hami.md
[CREATE ] domain-19-landscape-references/sandbox/k8gb/k8gb.md -> entities/k8gb.md
[CREATE ] domain-19-landscape-references/sandbox/atlantis/atlantis.md -> entities/atlantis.md
[CREATE ] domain-19-landscape-references/sandbox/score/score.md -> entities/score.md
[CREATE ] domain-19-landscape-references/sandbox/bank-vaults/bank-vaults.md -> entities/bank-vaults.md
[CREATE ] domain-19-landscape-references/sandbox/zot/zot.md -> entities/zot.md
[CREATE ] domain-19-landscape-references/sandbox/kuberhealthy/kuberhealthy.md -> entities/kuberhealthy.md
[CREATE ] domain-19-landscape-references/sandbox/container2wasm/container2wasm.md -> entities/container2wasm.md
[CREATE ] domain-19-landscape-references/sandbox/open-policy-containers/open-policy-containers.md -> entities/open-policy-containers.md
[CREATE ] domain-19-landscape-references/sandbox/kairos/kairos.md -> entities/kairos.md
[CREATE ] domain-19-landscape-references/sandbox/k0s/k0s.md -> entities/k0s.md
[CREATE ] domain-19-landscape-references/sandbox/kpt/kpt.md -> entities/kpt.md
[CREATE ] domain-19-landscape-references/sandbox/dalec/dalec.md -> entities/dalec.md
[CREATE ] domain-19-landscape-references/sandbox/konveyor/konveyor.md -> entities/konveyor.md
[CREATE ] domain-19-landscape-references/sandbox/metallb/metallb.md -> entities/metallb.md
[CREATE ] domain-19-landscape-references/sandbox/spiderpool/spiderpool.md -> entities/spiderpool.md
[CREATE ] domain-19-landscape-references/sandbox/composefs/composefs.md -> entities/composefs.md
[CREATE ] domain-19-landscape-references/sandbox/piraeus-datastore/piraeus-datastore.md -> entities/piraeus-datastore.md
[CREATE ] domain-19-landscape-references/sandbox/kube-burner/kube-burner.md -> entities/kube-burner.md
[CREATE ] domain-19-landscape-references/sandbox/telepresence/telepresence.md -> entities/telepresence.md
[CREATE ] domain-19-landscape-references/sandbox/k3s/k3s.md -> entities/k3s.md
[CREATE ] domain-19-landscape-references/sandbox/kuasar/kuasar.md -> entities/kuasar.md
[CREATE ] domain-19-landscape-references/sandbox/interlink/interlink.md -> entities/interlink.md
[CREATE ] domain-19-landscape-references/sandbox/bootc/bootc.md -> entities/bootc.md
[CREATE ] domain-19-landscape-references/sandbox/openfunction/openfunction.md -> entities/openfunction.md
[CREATE ] domain-19-landscape-references/sandbox/dex/dex.md -> entities/dex.md
[CREATE ] domain-19-landscape-references/sandbox/cohdi/cohdi.md -> entities/cohdi.md
[CREATE ] domain-19-landscape-references/sandbox/kubeslice/kubeslice.md -> entities/kubeslice.md
[CREATE ] domain-19-landscape-references/sandbox/k8up/k8up.md -> entities/k8up.md
[CREATE ] domain-19-landscape-references/sandbox/xregistry/xregistry.md -> entities/xregistry.md
[CREATE ] domain-19-landscape-references/sandbox/runme-notebooks/runme-notebooks.md -> entities/runme-notebooks.md
[CREATE ] domain-19-landscape-references/sandbox/ko/ko.md -> entities/ko.md
[CREATE ] domain-19-landscape-references/sandbox/porter/porter.md -> entities/porter.md
[CREATE ] domain-19-landscape-references/sandbox/hyperlight/hyperlight.md -> entities/hyperlight.md
[CREATE ] domain-19-landscape-references/sandbox/tokenetes/tokenetes.md -> entities/tokenetes.md
[CREATE ] domain-19-landscape-references/sandbox/kuma/kuma.md -> entities/kuma.md
[CREATE ] domain-19-landscape-references/sandbox/hwameistor/hwameistor.md -> entities/hwameistor.md
[CREATE ] domain-19-landscape-references/sandbox/open-cluster-management/open-cluster-management.md -> entities/open-cluster-management.md
[CREATE ] domain-19-landscape-references/sandbox/meshery/meshery.md -> entities/meshery.md
[CREATE ] domain-19-landscape-references/sandbox/kagent/kagent.md -> entities/kagent.md
[CREATE ] domain-19-landscape-references/sandbox/cozystack/cozystack.md -> entities/cozystack.md
[CREATE ] domain-19-landscape-references/sandbox/kube-ovn/kube-ovn.md -> entities/kube-ovn.md
[CREATE ] domain-19-landscape-references/sandbox/clusternet/clusternet.md -> entities/clusternet.md
[CREATE ] domain-19-landscape-references/sandbox/sops/sops.md -> entities/sops.md
[CREATE ] domain-19-landscape-references/sandbox/koordinator/koordinator.md -> entities/koordinator.md
[CREATE ] domain-19-landscape-references/sandbox/inclavare-containers/inclavare-containers.md -> entities/inclavare-containers.md
[CREATE ] domain-19-landscape-references/sandbox/cdk8s/cdk8s.md -> entities/cdk8s.md
[CREATE ] domain-19-landscape-references/sandbox/external-secrets/external-secrets.md -> entities/external-secrets.md
[CREATE ] domain-19-landscape-references/sandbox/slimfaas/slimfaas.md -> entities/slimfaas.md
[CREATE ] domain-19-landscape-references/sandbox/kitops/kitops.md -> entities/kitops.md
[CREATE ] domain-19-landscape-references/sandbox/hexa/hexa.md -> entities/hexa.md
[CREATE ] domain-19-landscape-references/sandbox/wasmedge/wasmedge.md -> entities/wasmedge.md
[CREATE ] domain-19-landscape-references/sandbox/kudo/kudo.md -> entities/kudo.md
[CREATE ] domain-19-landscape-references/sandbox/openchoreo/openchoreo.md -> entities/openchoreo.md
[CREATE ] domain-19-landscape-references/sandbox/keylime/keylime.md -> entities/keylime.md
[CREATE ] domain-19-landscape-references/sandbox/cadence/cadence.md -> entities/cadence.md
[CREATE ] domain-19-landscape-references/sandbox/opengemini/opengemini.md -> entities/opengemini.md
[CREATE ] domain-19-landscape-references/sandbox/kcp/kcp.md -> entities/kcp.md
[CREATE ] domain-19-landscape-references/sandbox/cloudnativepg/cloudnativepg.md -> entities/cloudnativepg.md
[CREATE ] domain-19-landscape-references/sandbox/cedar/cedar.md -> entities/cedar.md
[CREATE ] domain-19-landscape-references/sandbox/kcl/kcl.md -> entities/kcl.md
[CREATE ] domain-19-landscape-references/sandbox/serverless-workflow/serverless-workflow.md -> entities/serverless-workflow.md
[CREATE ] domain-19-landscape-references/sandbox/armada/armada.md -> entities/armada.md
[CREATE ] domain-19-landscape-references/sandbox/urunc/urunc.md -> entities/urunc.md
[CREATE ] domain-19-landscape-references/sandbox/opentofu/opentofu.md -> entities/opentofu.md
[CREATE ] domain-19-landscape-references/sandbox/oxia/oxia.md -> entities/oxia.md
[CREATE ] domain-19-landscape-references/sandbox/cartography/cartography.md -> entities/cartography.md
[CREATE ] domain-19-landscape-references/sandbox/athenz/athenz.md -> entities/athenz.md
[CREATE ] domain-19-landscape-references/sandbox/oscal-compass/oscal-compass.md -> entities/oscal-compass.md
[CREATE ] domain-19-landscape-references/sandbox/shipwright/shipwright.md -> entities/shipwright.md
[CREATE ] domain-19-landscape-references/sandbox/easegress/easegress.md -> entities/easegress.md
[CREATE ] domain-19-landscape-references/sandbox/headlamp/headlamp.md -> entities/headlamp.md
[CREATE ] domain-19-landscape-references/sandbox/submariner/submariner.md -> entities/submariner.md
[CREATE ] domain-19-landscape-references/sandbox/parsec/parsec.md -> entities/parsec.md
[CREATE ] domain-19-landscape-references/sandbox/kube-vip/kube-vip.md -> entities/kube-vip.md
[CREATE ] domain-12-cloud-providers/02-google-cloud-gke/google-cloud-gke-overview.md -> references/google-cloud-gke-overview.md
[CREATE ] domain-12-cloud-providers/06-huawei-cce/huawei-cce-overview.md -> references/huawei-cce-overview.md
[CREATE ] domain-12-cloud-providers/03-azure-aks/azure-aks-overview.md -> references/azure-aks-overview.md
[CREATE ] domain-12-cloud-providers/09-oracle-oke/oracle-oke-overview.md -> references/oracle-oke-overview.md
[CREATE ] domain-12-cloud-providers/07-ucloud-uk8s/ucloud-uk8s-overview.md -> references/ucloud-uk8s-overview.md
[CREATE ] domain-12-cloud-providers/13-alicloud-apsara-ack/251-apsara-stack-sls-logging.md -> references/251-apsara-stack-sls-logging.md
[CREATE ] domain-12-cloud-providers/13-alicloud-apsara-ack/alicloud-apsara-ack-overview.md -> references/alicloud-apsara-ack-overview.md
[CREATE ] domain-12-cloud-providers/13-alicloud-apsara-ack/250-apsara-stack-ess-scaling.md -> references/250-apsara-stack-ess-scaling.md
[CREATE ] domain-12-cloud-providers/13-alicloud-apsara-ack/252-apsara-stack-pop-operations.md -> references/252-apsara-stack-pop-operations.md
[CREATE ] domain-12-cloud-providers/01-aws-eks/aws-eks-overview.md -> references/aws-eks-overview.md
[CREATE ] domain-12-cloud-providers/12-ecloud-cke/ecloud-cke-overview.md -> references/ecloud-cke-overview.md
[CREATE ] domain-12-cloud-providers/10-volcengine-vek/volcengine-vek-overview.md -> references/volcengine-vek-overview.md
[CREATE ] domain-12-cloud-providers/05-tencent-tke/tencent-tke-overview.md -> references/tencent-tke-overview.md
[CREATE ] domain-12-cloud-providers/04-alicloud-ack/243-ack-ram-authorization.md -> references/243-ack-ram-authorization.md
[CREATE ] domain-12-cloud-providers/04-alicloud-ack/244-ack-ros-iac.md -> references/244-ack-ros-iac.md
[CREATE ] domain-12-cloud-providers/04-alicloud-ack/service-ack-practical-guide.md -> references/service-ack-practical-guide.md
[CREATE ] domain-12-cloud-providers/04-alicloud-ack/240-ack-ecs-compute.md -> references/240-ack-ecs-compute.md
[CREATE ] domain-12-cloud-providers/04-alicloud-ack/242-ack-vpc-network.md -> references/242-ack-vpc-network.md
[CREATE ] domain-12-cloud-providers/04-alicloud-ack/alicloud-ack-overview.md -> references/alicloud-ack-overview.md
[CREATE ] domain-12-cloud-providers/04-alicloud-ack/241-ack-slb-nlb-alb.md -> references/241-ack-slb-nlb-alb.md
[CREATE ] domain-12-cloud-providers/04-alicloud-ack/245-ack-ebs-storage.md -> references/245-ack-ebs-storage.md
[CREATE ] domain-12-cloud-providers/11-ctyun-tke/ctyun-tke-overview.md -> references/ctyun-tke-overview.md
[CREATE ] domain-12-cloud-providers/08-ibm-iks/ibm-iks-overview.md -> references/ibm-iks-overview.md
[CREATE ] domain-03-networking-traffic/41-terway-architecture-deep-dive.md -> entities/41-terway-architecture-deep-dive.md
[CREATE ] domain-03-networking-traffic/43-terway-crd-operations.md -> entities/43-terway-crd-operations.md
[CREATE ] domain-03-networking-traffic/44-terway-operations-manual.md -> entities/44-terway-operations-manual.md
[CREATE ] domain-03-networking-traffic/40-terway-product-overview.md -> entities/40-terway-product-overview.md
[CREATE ] domain-03-networking-traffic/42-terway-usage-guide.md -> entities/42-terway-usage-guide.md
[CREATE ] domain-03-networking-traffic/46-terway-performance-tuning.md -> entities/46-terway-performance-tuning.md
[CREATE ] domain-03-networking-traffic/45-terway-testing-validation.md -> entities/45-terway-testing-validation.md
[CREATE ] domain-03-networking-traffic/47-terway-troubleshooting-fta.md -> entities/47-terway-troubleshooting-fta.md
```


## Wiki 重新生成 - 2026-05-21

重新生成 14 个 wiki 页面（修复内容提取）。

### 详情

- [2026-05-21] 重新生成 references/configuration-terms.md (6 词条)
- [2026-05-21] 重新生成 references/fundamentals-terms.md (24 词条)
- [2026-05-21] 重新生成 references/multi-cloud-terms.md (3 词条)
- [2026-05-21] 重新生成 references/networking-terms.md (17 词条)
- [2026-05-21] 重新生成 references/observability-terms.md (10 词条)
- [2026-05-21] 重新生成 references/operations-terms.md (20 词条)
- [2026-05-21] 重新生成 references/platform-engineering-terms.md (19 词条)
- [2026-05-21] 重新生成 references/root-terms.md (4 词条)
- [2026-05-21] 重新生成 references/scheduling-terms.md (16 词条)
- [2026-05-21] 重新生成 references/security-terms.md (27 词条)
- [2026-05-21] 重新生成 references/specialized-workloads-terms.md (10 词条)
- [2026-05-21] 重新生成 references/storage-terms.md (17 词条)
- [2026-05-21] 重新生成 references/tooling-terms.md (3 词条)
- [2026-05-21] 重新生成 references/workloads-terms.md (33 词条)

## CNCF 聚合页面 + 域文档摄入 - 2026-05-21

将 domain-34-cncf-landscape、domain-17-cloud-provider、domain-03-networking-traffic 的未摄入文档完成 wiki-ingest。

### 策略

CNCF 229 个项目文档不逐个生成聚合页面，而是按功能类别聚合为 8 个聚合页面：
可观测性、安全、存储、网络、运行时、CI/CD、编排、边缘/AI/基础设施。

### 新增聚合页面

| 页面 | 涵盖项目数 | 路径 |
|------|-----------|------|
| CNCF 可观测性全景 | 16 | entities/cncf-observability.md |
| CNCF 安全与合规全景 | 31 | entities/cncf-security.md |
| CNCF 存储与数据库全景 | 19 | entities/cncf-storage.md |
| CNCF 网络与服务网格全景 | 28 | entities/cncf-networking.md |
| CNCF 容器运行时与工具链全景 | 26 | entities/cncf-runtime.md |
| CNCF CI/CD 与发布管理全景 | 15 | entities/cncf-cicd.md |
| CNCF 编排与应用管理全景 | 27 | entities/cncf-orchestration.md |
| CNCF 边缘计算与 AI/ML 全景 | 20 | entities/cncf-edge-ai.md |
| CNCF 基础设施与混沌工程全景 | 26 | entities/cncf-infrastructure.md |

### Manifest 更新

- 新增 6 个域文档（MOC.md + README.md × 3 个域）
- Manifest 总条目：3273
- 更新 references/k8s-cloud-provider-comparison.md 补充 14 个源文件链接

### 文档统计

- domain-34-cncf-landscape: 236 篇全部已摄入
- domain-17-cloud-provider: 26 篇全部已摄入
- domain-5-networking: 57 篇全部已摄入
- [2026-05-21] LINT issues_found=5 orphans=96 broken_links=~100 stale=0 lifecycle_issues=8 missing_summary=2095 missing_fm=38 relationship_issues=0

- [2026-05-21] CORPUS-CONFIG-UPDATE: 重构全量语料配置，引入'提炼知识 + 源文档'双层结构。更新 rag-full-profile.yaml（添加 concepts/ entities/ skills/ references/ synthesis/ 提炼知识层 + domain-*/topic-*/docs/ 源文档层 + domain-19-landscape-references/_archived-release-notes/ 按需层）、rag-sre-profile.yaml（添加 skills/ entities/ concepts/ synthesis/ 提炼知识）、rag-learning-profile.yaml（添加 concepts/ skills/learn-* 提炼知识）、notebooklm-profile.yaml（添加 concepts/ skills/ synthesis/ 高质量提炼页面）、rag-chunking-strategy.md（分层分块策略 + 元数据增强规范）、README.md（双层结构文档 + 快速开始指南）。全部 YAML 语法验证通过。

- [2026-05-21] STRUCTURE-REFACTOR: 目录结构规范化。创建 STRUCTURE.md 文档化四层目录结构；metadata/ 移入 _meta/metadata/；reports/ 改名为 _reports/；更新所有引用路径；全部 4 个 corpus-config profile 添加工程工具目录排除。

## Related

- [[concepts/bp-observability|bp-observability]] — 最佳实践：Observability
- [[concepts/bp-infrastructure|bp-infrastructure]] — 最佳实践：Infrastructure
- [[concepts/kubernetes-pki-certificate-system|kubernetes-pki-certificate-system]] — Kubernetes PKI 证书体系
- [[concepts/bp-common-best-practices|bp-common-best-practices]] — Kubernetes 通用最佳实践参考
- [[concepts/deployment-controller-architecture|deployment-controller-architecture]] — Deployment 控制器架构
- [2026-05-21] CROSS_LINK pages_scanned=588 links_added=2883 typed_relations_written=0 pages_modified=582 orphans_remaining=38 misc_affinity_updated=0 promotion_candidates=0
- [2026-05-21] LINT_CONSOLIDATE links_fixed=0 orphans_rescued=496 lifecycle_updates=8 tier_demotions=0 tag_fixes=0 contradiction_callouts=0 report=synthesis/consolidation-2026-05-21.md
- [2026-05-21] CROSS_LINK_FIX broken_links_fixed=152 nested_fixed=39 md_suffix_fixed=3041 path_normalized=5398 files_modified=83 final_orphans≈0 final_broken_links=0

- [2026-05-21T09:32:13Z] LINT issues_found=6436 orphans=2659 broken_links=262 stale=0 contradictions=0 prov_issues=0 missing_summary=3031 fragmented_clusters=31 visibility_issues=407 promotion_candidates=0 synthesis_gaps=36 relationship_issues=0

- [2026-05-21T17:45:00Z] LINT_FIX_EXECUTED — nested_wikilinks_fixed=940, cheat_sheet_template_fixed=15, contributing_fixed=1, moc_nested_fixed=1, topic_readme_refs_fixed=5, escaped_wikilinks_fixed=48, reports_chinese_fixed=3, domain_x_placeholder_fixed=1, moc_path_mismatch_fixed=106 (domain-41:50, domain-43:6, domain-42:50), domain42_moc_entries_added=47, meta_reports_frontmatter_fixed=31, shell_code_false_positives=0 (skipped, all in code blocks)

### [2026-05-21T18:30:00Z] DOMAIN-REFACTOR: 生产环境维度 Domain 整合

**触发原因**: 43 个 Domain 存在维度混杂（技术组件+工具品牌+内容载体+生命周期+部署场景五维混用），内容重叠率 25-30%，生产环境故障排查需跨 5-7 个目录检索。

**执行操作**:
- 合并可观测性三域 (8+20+21) → `domain-06-observability`
- 合并安全三域 (7+25+39) → `domain-05-security-compliance`
- 拆分 production-operations (18) → 8 个目标域
- 合并 platform-ops + platform-engineering (9+36) → `domain-07-platform-engineering`
- 合并网络相关四域 (5+15+26+35+40) → `domain-03-networking-traffic`
- 合并存储两域 (6+16) → `domain-04-storage-data`
- 合并架构基础三域 (1+2+3) → `domain-01-cluster-fundamentals`
- 迁移 15+ 个零散域至对应目标域

**迁移原则**: 只移动不删除，内容只增不减。
**实际执行**: 
- 迁移文件总数: 1,431
- 旧域清空: 43 个目录全部删除（原 README-MIGRATED.md 已移除，遵循 llm-wiki "Compile, don't retrieve" 原则）
- 新域创建: 20 个
- 内容丢失: 0

**结构变化**:
```
43 Domain (维度混杂) → 20 Domain (按运维职能分层)
Tier 1: 核心技术域 (6) / Tier 2: 平台工程域 (3) / Tier 3: 运维场景域 (2)
Tier 4: 部署生态域 (5) / Tier 5: 基础参考域 (4)
```

**后续待办**:
- `.manifest.json` 中的旧 domain 路径因文件移动而失效，建议下次全量 ingest 时重建
- `index.md` 已更新 Domain 索引
- `cross-linker` 需运行以修复内部 wikilink 路径
- `_meta/taxonomy.md` 需更新 domain 分类体系
- 新域 README 和 MOC 已创建

- [2026-05-21] KNOWLEDGE-EXPANSION: 对 3 个知识域进行系统性扩充，新增 28 个高质量参考文档。
  - domain-09-reliability-engineering: 从 18 文件扩充到 35 文件 (+17)。新增 SLO/SLI 体系 (4 文件: SLI 定义、SLO 实施、错误预算、Burn Rate 告警)、混沌工程 (4 文件: 概述、Chaos Mesh、实验设计、Litmus)、事后复盘 (2 文件: 无责模板、文化指南)、SRE 实践 (4 文件: 可用性计算、发布门控、事故指挥、Toil 削减)、性能测试 (2 文件: 负载测试、混沌集成)、灾备手册 (2 文件: 场景目录、AZ 故障恢复)。
  - domain-16-database-middleware: 从 13 文件扩充到 22 文件 (+9)。新增消息队列 (3 文件: NATS、Pulsar、选型对比)、时序数据库 (2 文件: Prometheus TSDB、InfluxDB vs TimescaleDB)、Operator 管理 (2 文件: 设计模式、MySQL/PG/Redis 对比)、数据流处理 (2 文件: CDC、流处理框架)。
  - synthesis/: 从 29 文件扩充到 44 文件 (+15)。新增跨域分析: SLO×监控、GitOps×发布门控、混沌×灾备演练、多集群×安全、可观测性×FinOps、AI/ML×可观测性、平台工程×SRE、边缘-云连续体、Backstage×平台目录、K8s 数据保护、安全×可观测性关联、多集群成本优化、服务网格×安全治理、AI Agent 运维、跨云迁移手册。
- [2026-05-21] WIKILINK-FIX: 验证并确认所有旧 `domain-NN-` 路径引用已在工作树中清零。2,800+ 文件的 wikilink 路径已在新域迁移时同步更新。暂存区文件状态正确，无旧路径残留。
- [2026-05-21] INDEX-UPDATE: 更新 `index.md` — domain-09 和 domain-16 描述已扩展，新增 "Cross-Domain Synthesis" 章节列出 15 个新增跨域分析文件。

**相关报告**:
- `_reports/domain-production-assessment.md` — 评估报告
- `_reports/domain-migration-EXECUTED-2026-05-21.md` — 迁移执行报告
- `_reports/knowledge-completeness-assessment-2026-05-21.md` — 知识完整性评估

- [2026-05-23 16:58:09] ENHANCE 远程顾问模式四项增强完成
  - QA: 813条, 因果推理70.4%, 27 skill全部>=25
  - 阿里云: 7篇文档(3014行), 17/17对话脚本ACK分支
  - 合成分析: 236个跨域文档
  - Frontmatter修复: 对话脚本+合成文件+synthesis+case-studies
  - LINT: broken links修复, summary补全
2026-05-23 17:08:14 — 全部任务已完成
- [2026-05-23 17:36:45] LINT_CONSOLIDATE links_fixed=3 orphans_rescued=3 lifecycle_updates=0 tier_demotions=0 tag_fixes=0 contradiction_callouts=0 report=synthesis/consolidation-2026-05-23.md
- [2026-05-23 17:46:51] LINT_CONSOLIDATE_FULL broken_links_fixed=25771 files_modified=~4300 report=synthesis/consolidation-2026-05-23.md
- [2026-05-23 17:54:08] CROSS_LINK pages_scanned=124 links_added=587 typed_relations_written=587 pages_modified=123

- [2026-05-23 18:00:32] DIGEST period="7d" new_pages=198 updated_pages=0 themes=46 connections=1589 saved=true
- [2026-05-23 18:15:19] FINAL frontmatter_fixed=4488 broken_links_final=0 lifecycle_promoted=31 tags_normalized=16 cross_links=587 embedding_updated=true
- [2026-05-23 18:37:33] TAG_COHESION cluster=0.183 workloads=0.117 cross-domain=0.152 storage=0.643 networking=0.464 security=0.464 synthesis=0.257 links_added=298
- [2026-05-23 19:52:14] TAG_TAXONOMY tags_normalized=16 pages_modified=17 new_tags_added=15

## [2026-05-24] lint | wiki-lint 全面健康审计
- 扫描页面: 4,999（排除 node_modules、.comate）
- broken_links=222 (0.64%, Obsidian 可解析)
- missing_frontmatter=46
- missing_summary=4,984 (soft warning)
- stale=0
- contradictions=0
- orphans=1,624 (1,235 release notes + 144 培训 + 245 其他)
- fragmented_clusters=24
- unknown_tags=232
- large_pages=2,322
- pii_without_visibility=555
- synthesis_gaps=22
- lifecycle_pages=1
- 报告: reports/wiki-lint-2026-05-24.md
