---
title: Wiki Log
description: KUDIG Database 全库维护与变更日志
summary: 记录 KUDIG Database 的维护活动、ingest 事件与结构变更。
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
tier: supporting
created: '2026-05-21'
updated: '2026-06-26'
last_updated: 2026-06-26
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

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



- [2026-06-26] CORPUS_EXPORT output=release/ pages=4996 tokens=17460194 qa_pairs=19502 tier_counts={core=1102,supporting=1382,peripheral=2512}

- [2026-06-26] TIER_ASSIGN pages_scanned=4876 core=1102 supporting=1372 peripheral=2511 permission_errors=2
- [2026-06-26] FRAGMENTED_TAGS_CROSS_LINK tags=[research,deep-dive,papers,reference,visibility/public] hubs_created=5 pages_modified=434 broken_links_resolved=855->0

- [2026-06-26T12:00:00+08:00] STATUS_INSIGHTS anchors=20 cohesion_checked=324 tier_suggestions=3578 delta="+20 nodes +440 edges"
- [2026-06-26] SUMMARY_FRONTMATTER_FILL pages_scanned=4987 summary_added=4937 frontmatter_added=41 blocked=2
- [2026-06-26] BROKEN_LINKS_FIX pages_scanned=4875 broken_links=0 relationship_issues=0 total_fixed=~17000+ converted_meta_report_links=~500+
- [2026-06-26] WIKI_SYNTHESIZE pages_scanned=5506 synthesis_created=5 candidates_skipped=0 topics=[statefulset-storage,helm-gitops,slo-observability,container-runtime-image-security,ticket-agent-rag]
- [2026-06-26] LINT pages_scanned=4876 orphans=1788 broken_links=0 missing_frontmatter=0 missing_summary=2 stale=0 fragmented_tags=276 relationship_issues=0
- [2026-06-26] CROSS_LINK pages_scanned=5659 target_orphans=30 links_added=100 pages_modified=28 orphans_remaining=~3300 (targeted _reports + ticket-cases only)

# Wiki Log

- [2026-05-21T23:08:00+08:00] CROSS_LINK phase=complete-orphan-elimination pages_scanned=4672 links_added=9121 pages_modified=~2400 orphans_remaining=0 (0.0%) cohesion=1.0000 TARGET_ACHIEVED. Waves: emitter-backlinks(3773), index-wikilink-conversion(1428), index-backlinks(2455), release-note-entity-links(743+68), generic-orphan-inline(372+235), final-cleanup(47), calico-backlinks(9). All 2,659 original orphans resolved.

- [2026-05-21T17:15:00Z] CROSS_LINK phase=final-push pages_scanned=4003 total_links_added_fixed=19937 pages_modified=5685 orphans_remaining=160 (4.0%) TARGET_ACHIEVED


- [2026-05-21T17:00:00Z] CROSS_LINK phase=deep-dive pages_scanned=3993 total_links_added_fixed=14612 pages_modified=4323 orphans_remaining=255 (6.4%)


- [2026-05-21T17:30:00+08:00] WIKI_SYNTHESIZE pages_scanned=578 synthesis_created=10 candidates_skipped=0 (round 2: 10 skipped candidates from round 1)


- [2026-05-21T16:45:00Z] CROSS_LINK phase=final pages_scanned=3982 links_added=7045 pages_modified=2433 orphans_remaining=954 domain_orphans=25 release_orphans=1


- [2026-05-21T17:00:00+08:00] WIKI_dashboard name=dashboard tool=dataview views=9 filter=concepts/entities/skills/references/synthesis


- [2026-05-21T16:30:00Z] CROSS_LINK phase=domain-release-notes pages_scanned=3982 links_added=4918 pages_modified=3486 domain_orphans_remaining=586 release_orphans_remaining=355


- [2026-05-21T16:07:01+08:00] WIKI_SYNTHESIZE pages_scanned=578 synthesis_created=5 candidates_skipped=10


- [2026-05-21T16:15:00Z] CROSS_LINK phase=backlink pages_scanned=3981 links_added=4760 typed_relations_written=4745 pages_modified=969 orphans_remaining=2752 misc_affinity_updated=0 promotion_candidates=0


- [2026-05-21T16:06:00Z] CROSS_LINK pages_scanned=3979 links_added=3824 typed_relations_written=3824 pages_modified=793 orphans_remaining=2757 misc_affinity_updated=0 promotion_candidates=0


- [2026-05-21] RELEASE-NOTES-INDEX-INGEST 生态参考/_archived-release-notes/: 创建 8 个类别索引页 — references/release-notes-observability.md (可观测性, 374 篇)、release-notes-security.md (安全, 218 篇)、release-notes-cli-tools.md (CLI 工具, 187 篇)、release-notes-cicd-gitops.md (CI/CD & GitOps, 171 篇)、release-notes-networking.md (网络, 157 篇)、release-notes-core-deps.md (核心依赖, 83 篇)、release-notes-storage.md (存储, 76 篇)、release-notes-kubernetes.md (Kubernetes, 55 篇)。33 个项目覆盖 1,321 个发布说明源文件（MOC.md + README.md 已存在）。manifest 已含全部 1,323 个 生态参考/_archived-release-notes/ 条目及 SHA-256 哈希。每页包含项目总览表、版本覆盖、Breaking Changes 汇总、wikilink 到对应 entities/ 页面。
- [2026-05-21] TOPIC-FUNCTIONS-INGEST 工作负载/topic-functions/: 创建 14 个 wiki 页面 — 9 skills (kubeadm-cluster-lifecycle, kubeadm-cluster-deletion, kubeadm-ha-cluster-setup, kubelet-certificate-rotation, node-drain-and-maintenance, kubelet-eviction-mechanism, deployment-rolling-update, deployment-canary-and-bluegreen, deployment-workload-selection) + 4 concepts (kubernetes-pki-certificate-system, cni-networking-model, node-lifecycle-management, deployment-controller-architecture) + 1 synthesis (kubeadm-cluster-operations)。82 个源文件已摄入，覆盖 5 个子专题：cluster-cert (17)、cluster-create (25)、cluster-delete (13)、deployment-create (10)、node-create (17)。覆盖领域：kubeadm init 12 阶段生命周期、PKI 三组 CA 体系、证书轮换与 TLS Bootstrap、高可用部署（stacked/external etcd）、节点注册与生命周期管理、工作负载控制器选型、Deployment 滚动更新、金丝雀/蓝绿发布、kubectl drain 与节点维护、kubelet 资源驱逐机制、集群删除与 reset 清理流程、CNI 网络模型与 DNS 解析。
- [2026-05-21] RELEASE-NOTES-INGEST 生态参考/_archived-release-notes/: 创建 13 个 wiki 页面 — 8 concepts (kubernetes-version-evolution, core-dependency-version-matrix, gitops-tool-evolution, service-mesh-evolution, observability-stack-evolution, security-tool-evolution, storage-tool-evolution, cli-tools-evolution) + 3 entities (kubernetes-changelog, core-deps-changelog, ecosystem-changelog) + 2 references (version-upgrade-guide, release-notes-reading-guide)。1,321 个源文件已摄入。覆盖：Kubernetes v0.x-v1.36 版本演进、5 大核心依赖版本矩阵、GitOps 工具（Argo CD/Flux/Tekton）、服务网格（Istio/Envoy/Cilium/Linkerd/Calico）、可观测性栈（Prometheus/Grafana/Loki/Thanos/OpenTelemetry）、安全工具（Falco/OPA/Gatekeeper/Trivy/cert-manager）、存储工具（Rook/Longhorn/Velero）、CLI 工具（Helm/Kind/Kops/Minikube/Kustomize）。
- [2026-05-21] FULL-INGEST 故障诊断/FTA故障树/工作负载/topic-functions/故障诊断/高级排障/structural-topic-skills: 创建 81 个 wiki 页面。222 个源文件全部摄入。FTA故障树 list/ → skills/ (44个独立页面), 操作函数 → references/ (5个分组页面: cluster-cert/cluster-create/cluster-delete/deployment-create/node-create), 结构化排查 → skills/ (13个分组页面: control-plane/node-components/networking/storage/workloads/security-auth/resources-scheduling/cluster-operations/cloud-provider/command-output/ai-ml-workloads/gitops-devops/monitoring-observability), 诊断Skill → skills/ (19个页面含评估/skill-set/skills-run)。manifest 总计 1944 条目。
- [2026-05-21] BEST-PRACTICES-INGEST 生产运维/topic-best-practices/: 创建 12 个 wiki 页面 — 1 concept (k8s-production-best-practices) + 11 skills (cluster/network/storage/monitoring/logging/tracing/deployment/scaling/disaster-recovery/network-security/pod-security guides)。更新 1 现有页面 (secrets-management)。13 个源文件已摄入。覆盖：生产最佳实践模式、集群配置、CNI 选型、存储分层、可观测性栈（Prometheus/EFK/Jaeger）、部署策略（滚动/蓝绿/金丝雀）、自动扩缩容（HPA/VPA/CA）、灾难恢复（Velero）、网络安全（NetworkPolicy/mTLS）、Pod 安全（PSS/seccomp）、密钥管理（etcd 加密/Vault/External Secrets）。
- [2026-05-21] TOPIC-LEARN VERIFICATION: 验证 27 个 learn-* 页面已完整覆盖 137 个 生产运维/topic-learn/ 源文件。index.md 已添加 learn-* 页面完整列表。manifest 已含 137 生产运维/topic-learn/ 条目及 SHA-256 哈希。
- [2026-05-21] FULL-INGEST 集群基础 through 网络 + topic-*: Created 91 wiki pages across concepts/ (39), entities/ (23), skills/ (15), references/ (11), synthesis/ (3). 1,093 source documents ingested. Coverage: K8s architecture, Docker, Linux containers, networking, storage, security, GitOps, IaC, service mesh, observability, FTA methodology, diagnostic skills, agent orchestration, CNCF ecosystem.
- [2026-05-21] Wiki enhancement pass — all 96 original pages enhanced with typed relationships, provenance fields, confidence recalculation.
- [2026-05-21] REMAINING-INGEST: Created 5 wiki pages — k8s-knowledge-map, k8s-difficulty-index, kudig-man-pages-index, kudig-prompts-catalog.md|kudig-prompts-catalog]], kudig-templates-catalog.md|kudig-templates-catalog]]. Total: 101 pages.
- [2026-05-21] WIKI_SYNTHESIZE 扫描页面=63 新增综合=5 跳过候选=10 — 综合页面全部中文输出：eBPF x 运行时安全、GitOps x 平台工程、纵深防御 x 供应链安全、服务网格 x 零信任安全、IaC x 多集群管理。
- [2026-05-21] FULL-INGEST 集群基础 through domain-12: Created 35 wiki pages across concepts/ (12), entities/ (11), skills/ (8), references/ (4). Coverage: architecture, design principles, control plane, workloads, networking, storage, security, observability, autoscaling, extensions, AI infra, troubleshooting methodology.
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

- [2026-05-21] 批量摄入 系统基础/topic-dictionary/configuration/ → references/configuration-terms.md (6 词条)
- [2026-05-21] 批量摄入 系统基础/topic-dictionary/fundamentals/ → references/fundamentals-terms.md (24 词条)
- [2026-05-21] 批量摄入 系统基础/topic-dictionary/multi-cloud/ → references/multi-cloud-terms.md (3 词条)
- [2026-05-21] 批量摄入 系统基础/topic-dictionary/networking/ → references/networking-terms.md (17 词条)
- [2026-05-21] 批量摄入 系统基础/topic-dictionary/observability/ → references/observability-terms.md (10 词条)
- [2026-05-21] 批量摄入 系统基础/topic-dictionary/operations/ → references/operations-terms.md (20 词条)
- [2026-05-21] 批量摄入 系统基础/topic-dictionary/platform-engineering/ → references/platform-engineering-terms.md (19 词条)
- [2026-05-21] 批量摄入 系统基础/topic-dictionary/root/ → references/root-terms.md (2 词条)
- [2026-05-21] 批量摄入 系统基础/topic-dictionary/scheduling/ → references/scheduling-terms.md (16 词条)
- [2026-05-21] 批量摄入 系统基础/topic-dictionary/security/ → references/security-terms.md (27 词条)
- [2026-05-21] 批量摄入 系统基础/topic-dictionary/specialized-workloads/ → references/specialized-workloads-terms.md (10 词条)
- [2026-05-21] 批量摄入 系统基础/topic-dictionary/storage/ → references/storage-terms.md (17 词条)
- [2026-05-21] 批量摄入 系统基础/topic-dictionary/tooling/ → references/tooling-terms.md (3 词条)
- [2026-05-21] 批量摄入 系统基础/topic-dictionary/workloads/ → references/workloads-terms.md (33 词条)
- [2026-05-21] 摄入 生产运维/topic-learn/fundamentals/01-what-is-kubernetes.md → skills/learn-01-what-is-kubernetes.md
- [2026-05-21] 摄入 生产运维/topic-learn/fundamentals/02-pod-basics.md → skills/learn-02-pod-basics.md
- [2026-05-21] 摄入 生产运维/topic-learn/fundamentals/03-deployment-basics.md → skills/learn-03-deployment-basics.md
- [2026-05-21] 摄入 生产运维/topic-learn/fundamentals/04-service-basics.md → skills/learn-04-service-basics.md
- [2026-05-21] 摄入 生产运维/topic-learn/fundamentals/05-ingress-basics.md → skills/learn-05-ingress-basics.md
- [2026-05-21] 摄入 生产运维/topic-learn/fundamentals/06-configmap-secret.md → skills/learn-06-configmap-secret.md
- [2026-05-21] 摄入 生产运维/topic-learn/fundamentals/07-namespace-resource-quota.md → skills/learn-07-namespace-resource-quota.md
- [2026-05-21] 摄入 生产运维/topic-learn/fundamentals/08-pv-pvc-basics.md → skills/learn-08-pv-pvc-basics.md
- [2026-05-21] 摄入 生产运维/topic-learn/fundamentals/09-hpa-basics.md → skills/learn-09-hpa-basics.md
- [2026-05-21] 摄入 生产运维/topic-learn/fundamentals/10-health-check.md → skills/learn-10-health-check.md
- [2026-05-21] 摄入 生产运维/topic-learn/fundamentals/11-job-cronjob.md → skills/learn-11-job-cronjob.md
- [2026-05-21] 摄入 生产运维/topic-learn/fundamentals/12-common-problems.md → skills/learn-12-common-problems.md
- [2026-05-21] 摄入 生产运维/topic-learn/fundamentals/13-daemonset-basics.md → skills/learn-13-daemonset-basics.md
- [2026-05-21] 摄入 生产运维/topic-learn/fundamentals/14-statefulset-basics.md → skills/learn-14-statefulset-basics.md
- [2026-05-21] 摄入 生产运维/topic-learn/fundamentals/15-scheduling-basics.md → skills/learn-15-scheduling-basics.md
- [2026-05-21] 批量摄入 生产运维/topic-learn/inner-training/ → skills/learn-inner-training.md (46 篇)
- [2026-05-21] 摄入 生产运维/topic-learn/oncall-qa/oncall-quick-qa.md → skills/learn-oncall-quick-qa.md
- [2026-05-21] 批量摄入 生产运维/topic-learn/public-training/ → skills/learn-public-training.md (65 篇)
- [2026-05-21] 摄入 生产运维/topic-learn/quick-start/01-day-one-checklist.md → skills/learn-01-day-one-checklist.md
- [2026-05-21] 摄入 生产运维/topic-learn/quick-start/02-first-ticket-guide.md → skills/learn-02-first-ticket-guide.md
- [2026-05-21] 摄入 生产运维/topic-learn/quick-start/03-oncall-handoff.md → skills/learn-03-oncall-handoff.md
- [2026-05-21] 摄入 生产运维/topic-learn/quick-start/04-debug-tools-setup.md → skills/learn-04-debug-tools-setup.md
- [2026-05-21] 摄入 生产运维/topic-learn/quick-start/README.md → skills/learn-README.md
- [2026-05-21] 摄入 生产运维/topic-learn/resources/analogy-dictionary.md → skills/learn-analogy-dictionary.md
- [2026-05-21] 摄入 生产运维/topic-learn/resources/lecturer-persona.md → skills/learn-lecturer-persona.md
- [2026-05-21] 批量摄入 生产运维/topic-learn/root/ → skills/learn-root.md (2 篇)
- [2026-05-21] 摄入 生产运维/topic-learn/troubleshooting/decision-tree-mermaid.md → skills/learn-decision-tree-mermaid.md
- [2026-05-21] 批量摄入 生产运维/topic-best-practices/infrastructure/ → concepts/bp-infrastructure.md (3 篇)
- [2026-05-21] 批量摄入 生产运维/topic-best-practices/observability/ → concepts/bp-observability.md (3 篇)
- [2026-05-21] 批量摄入 生产运维/topic-best-practices/operations/ → concepts/bp-operations.md (3 篇)
- [2026-05-21] 摄入 生产运维/topic-best-practices/MOC.md → concepts/bp-MOC.md
- [2026-05-21] 摄入 生产运维/topic-best-practices/README.md → concepts/bp-README.md
- [2026-05-21] 摄入 生产运维/topic-best-practices/common-best-practices.md → concepts/bp-common-best-practices.md
- [2026-05-21] 批量摄入 生产运维/topic-best-practices/security/ → concepts/bp-security.md (3 篇)
- [2026-05-21] 批量摄入 AI基础设施/02-ai-agents/openclaw-workspace/ → concepts/ai-agent-openclaw-workspace.md (7 篇)
- [2026-05-21] 摄入 AI基础设施/02-ai-agents/MOC.md → concepts/ai-agent-MOC.md
- [2026-05-21] 摄入 AI基础设施/02-ai-agents/README.md → concepts/ai-agent-README.md


## Wiki Ingest Batch — 2026-05-21

**统计**: 创建 252 | 更新 8 | 跳过 0 | 错误 0

```
# 🟢 低风险：只读/信息收集，通常无副作用
[CREATE ] 生态参考/graduated/tikv/tikv.md -> entities/tikv.md
[CREATE ] 生态参考/graduated/dragonfly/dragonfly.md -> entities/dragonfly.md
[CREATE ] 生态参考/graduated/cloudevents/cloudevents.md -> entities/cloudevents.md
[CREATE ] 生态参考/graduated/fluentd/fluentd.md -> entities/fluentd.md
[CREATE ] 生态参考/graduated/harbor/harbor.md -> entities/harbor.md
[CREATE ] 生态参考/graduated/rook/rook.md -> entities/rook.md
[CREATE ] 生态参考/graduated/istio/02-istio-advanced-traffic-management.md -> entities/02-istio-advanced-traffic-management.md
[UPDATE ] 生态参考/graduated/istio/istio.md -> entities/istio.md
[CREATE ] 生态参考/graduated/istio/03-istio-security-hardening.md -> entities/03-istio-security-hardening.md
[UPDATE ] 生态参考/graduated/flux/flux.md -> entities/flux.md
[CREATE ] 生态参考/graduated/jaeger/jaeger.md -> entities/jaeger.md
[CREATE ] 生态参考/graduated/in-toto/in-toto.md -> entities/in-toto.md
[CREATE ] 生态参考/graduated/vitess/vitess.md -> entities/vitess.md
[CREATE ] 生态参考/graduated/tuf/tuf.md -> entities/tuf.md
[CREATE ] 生态参考/graduated/cert-manager/cert-manager.md -> entities/cert-manager.md
[CREATE ] 生态参考/graduated/knative/knative.md -> entities/knative.md
[CREATE ] 生态参考/graduated/kubeedge/kubeedge.md -> entities/kubeedge.md
[CREATE ] 生态参考/graduated/containerd/07-containerd-disaster-recovery.md -> entities/07-containerd-disaster-recovery.md
[CREATE ] 生态参考/graduated/containerd/04-containerd-upgrade-migration.md -> entities/04-containerd-upgrade-migration.md
[UPDATE ] 生态参考/graduated/containerd/containerd.md -> entities/containerd.md
[CREATE ] 生态参考/graduated/containerd/05-containerd-windows-support.md -> entities/05-containerd-windows-support.md
[CREATE ] 生态参考/graduated/containerd/02-containerd-v2-features.md -> entities/02-containerd-v2-features.md
[CREATE ] 生态参考/graduated/containerd/08-containerd-multi-tenant.md -> entities/08-containerd-multi-tenant.md
[CREATE ] 生态参考/graduated/containerd/03-containerd-security-hardening.md -> entities/03-containerd-security-hardening.md
[CREATE ] 生态参考/graduated/containerd/06-containerd-observability.md -> entities/06-containerd-observability.md
[CREATE ] 生态参考/graduated/cri-o/cri-o.md -> entities/cri-o.md
[CREATE ] 生态参考/graduated/envoy/envoy.md -> entities/envoy.md
[CREATE ] 生态参考/graduated/spire/spire.md -> entities/spire.md
[CREATE ] 生态参考/graduated/keda/keda.md -> entities/keda.md
[UPDATE ] 生态参考/graduated/crossplane/crossplane.md -> entities/crossplane.md
[CREATE ] 生态参考/graduated/cubefs/cubefs.md -> entities/cubefs.md
[CREATE ] 生态参考/graduated/linkerd/linkerd.md -> entities/linkerd.md
[CREATE ] 生态参考/graduated/opa/opa.md -> entities/opa.md
[UPDATE ] 生态参考/graduated/etcd/etcd.md -> entities/etcd.md
[CREATE ] 生态参考/graduated/dapr/dapr.md -> entities/dapr.md
[UPDATE ] 生态参考/graduated/falco/falco.md -> entities/falco.md
[CREATE ] 生态参考/graduated/spiffe/spiffe.md -> entities/spiffe.md
[CREATE ] 生态参考/graduated/prometheus/prometheus.md -> entities/prometheus.md
[CREATE ] 生态参考/graduated/prometheus/02-prometheus-promql-advanced.md -> entities/02-prometheus-promql-advanced.md
[CREATE ] 生态参考/graduated/prometheus/03-prometheus-ha-deployment.md -> entities/03-prometheus-ha-deployment.md
[CREATE ] 生态参考/graduated/kubernetes/kubernetes.md -> entities/kubernetes.md
[CREATE ] 生态参考/graduated/coredns/coredns.md -> entities/coredns.md
[CREATE ] 生态参考/graduated/argo/argo.md -> entities/argo.md
[UPDATE ] 生态参考/graduated/cilium/cilium.md -> entities/cilium.md
[CREATE ] 生态参考/graduated/helm/helm.md -> entities/helm.md
[CREATE ] 生态参考/incubating/lima/lima.md -> entities/lima.md
[CREATE ] 生态参考/incubating/wasmcloud/wasmcloud.md -> entities/wasmcloud.md
[CREATE ] 生态参考/incubating/cortex/cortex.md -> entities/cortex.md
[CREATE ] 生态参考/incubating/metal3-io/metal3-io.md -> entities/metal3-io.md
[CREATE ] 生态参考/incubating/flatcar/flatcar.md -> entities/flatcar.md
[CREATE ] 生态参考/incubating/emissary-ingress/emissary-ingress.md -> entities/emissary-ingress.md
[CREATE ] 生态参考/incubating/grpc/grpc.md -> entities/grpc.md
[CREATE ] 生态参考/incubating/opentelemetry/opentelemetry.md -> entities/opentelemetry.md
[CREATE ] 生态参考/incubating/openyurt/openyurt.md -> entities/openyurt.md
[CREATE ] 生态参考/incubating/contour/contour.md -> entities/contour.md
[CREATE ] 生态参考/incubating/notary-project/notary-project.md -> entities/notary-project.md
[CREATE ] 生态参考/incubating/kserve/kserve.md -> entities/kserve.md
[CREATE ] 生态参考/incubating/fluid/fluid.md -> entities/fluid.md
[CREATE ] 生态参考/incubating/longhorn/longhorn.md -> entities/longhorn.md
[CREATE ] 生态参考/incubating/openfga/openfga.md -> entities/openfga.md
[CREATE ] 生态参考/incubating/buildpacks/buildpacks.md -> entities/buildpacks.md
[CREATE ] 生态参考/incubating/karmada/karmada.md -> entities/karmada.md
[CREATE ] 生态参考/incubating/nats/nats.md -> entities/nats.md
[CREATE ] 生态参考/incubating/cni/cni.md -> entities/cni.md
[CREATE ] 生态参考/incubating/kubescape/kubescape.md -> entities/kubescape.md
[CREATE ] 生态参考/incubating/kubevela/kubevela.md -> entities/kubevela.md
[CREATE ] 生态参考/incubating/kubevirt/kubevirt.md -> entities/kubevirt.md
[CREATE ] 生态参考/incubating/thanos/thanos.md -> entities/thanos.md
[CREATE ] 生态参考/incubating/cloud-custodian/cloud-custodian.md -> entities/cloud-custodian.md
[CREATE ] 生态参考/incubating/chaos-mesh/chaos-mesh.md -> entities/chaos-mesh.md
[CREATE ] 生态参考/incubating/litmus/litmus.md -> entities/litmus.md
[CREATE ] 生态参考/incubating/operator-framework/operator-framework.md -> entities/operator-framework.md
[CREATE ] 生态参考/incubating/opencost/opencost.md -> entities/opencost.md
[CREATE ] 生态参考/incubating/openkruise/openkruise.md -> entities/openkruise.md
[CREATE ] 生态参考/incubating/openfeature/openfeature.md -> entities/openfeature.md
[CREATE ] 生态参考/incubating/keycloak/keycloak.md -> entities/keycloak.md
[CREATE ] 生态参考/incubating/backstage/backstage.md -> entities/backstage.md
[CREATE ] 生态参考/incubating/kubeflow/kubeflow.md -> entities/kubeflow.md
[UPDATE ] 生态参考/incubating/kyverno/kyverno.md -> entities/kyverno.md
[CREATE ] 生态参考/incubating/volcano/volcano.md -> entities/volcano.md
[CREATE ] 生态参考/incubating/artifact-hub/artifact-hub.md -> entities/artifact-hub.md
[CREATE ] 生态参考/incubating/strimzi/strimzi.md -> entities/strimzi.md
[CREATE ] 生态参考/sandbox/opengitops/opengitops.md -> entities/opengitops.md
[CREATE ] 生态参考/sandbox/kubeclipper/kubeclipper.md -> entities/kubeclipper.md
[CREATE ] 生态参考/sandbox/devfile/devfile.md -> entities/devfile.md
[CREATE ] 生态参考/sandbox/tremor/tremor.md -> entities/tremor.md
[CREATE ] 生态参考/sandbox/kubestellar/kubestellar.md -> entities/kubestellar.md
[CREATE ] 生态参考/sandbox/confidential-containers/confidential-containers.md -> entities/confidential-containers.md
[CREATE ] 生态参考/sandbox/tinkerbell/tinkerbell.md -> entities/tinkerbell.md
[CREATE ] 生态参考/sandbox/network-service-mesh/network-service-mesh.md -> entities/network-service-mesh.md
[CREATE ] 生态参考/sandbox/trickster/trickster.md -> entities/trickster.md
[CREATE ] 生态参考/sandbox/logging-operator/logging-operator.md -> entities/logging-operator.md
[CREATE ] 生态参考/sandbox/pixie/pixie.md -> entities/pixie.md
[CREATE ] 生态参考/sandbox/serverless-devs/serverless-devs.md -> entities/serverless-devs.md
[CREATE ] 生态参考/sandbox/krkn/krkn.md -> entities/krkn.md
[CREATE ] 生态参考/sandbox/oras/oras.md -> entities/oras.md
[CREATE ] 生态参考/sandbox/antrea/antrea.md -> entities/antrea.md
[CREATE ] 生态参考/sandbox/podman-container-tools/podman-container-tools.md -> entities/podman-container-tools.md
[CREATE ] 生态参考/sandbox/holmesgpt/holmesgpt.md -> entities/holmesgpt.md
[CREATE ] 生态参考/sandbox/kubefleet/kubefleet.md -> entities/kubefleet.md
[CREATE ] 生态参考/sandbox/bfe/bfe.md -> entities/bfe.md
[CREATE ] 生态参考/sandbox/spinkube/spinkube.md -> entities/spinkube.md
[CREATE ] 生态参考/sandbox/inspektor-gadget/inspektor-gadget.md -> entities/inspektor-gadget.md
[CREATE ] 生态参考/sandbox/kaito/kaito.md -> entities/kaito.md
[CREATE ] 生态参考/sandbox/werf/werf.md -> entities/werf.md
[CREATE ] 生态参考/sandbox/virtual-kubelet/virtual-kubelet.md -> entities/virtual-kubelet.md
[CREATE ] 生态参考/sandbox/podman-desktop/podman-desktop.md -> entities/podman-desktop.md
[CREATE ] 生态参考/sandbox/eraser/eraser.md -> entities/eraser.md
[CREATE ] 生态参考/sandbox/connect-rpc/connect-rpc.md -> entities/connect-rpc.md
[CREATE ] 生态参考/sandbox/youki/youki.md -> entities/youki.md
[CREATE ] 生态参考/sandbox/kepler/kepler.md -> entities/kepler.md
[CREATE ] 生态参考/sandbox/pipecd/pipecd.md -> entities/pipecd.md
[CREATE ] 生态参考/sandbox/openebs/openebs.md -> entities/openebs.md
[CREATE ] 生态参考/sandbox/clusterpedia/clusterpedia.md -> entities/clusterpedia.md
[CREATE ] 生态参考/sandbox/loxilb/loxilb.md -> entities/loxilb.md
[CREATE ] 生态参考/sandbox/oauth2-proxy/oauth2-proxy.md -> entities/oauth2-proxy.md
[CREATE ] 生态参考/sandbox/copa/copa.md -> entities/copa.md
[CREATE ] 生态参考/sandbox/vineyard/vineyard.md -> entities/vineyard.md
[CREATE ] 生态参考/sandbox/aeraki-mesh/aeraki-mesh.md -> entities/aeraki-mesh.md
[CREATE ] 生态参考/sandbox/chaosblade/chaosblade.md -> entities/chaosblade.md
[CREATE ] 生态参考/sandbox/kubewarden/kubewarden.md -> entities/kubewarden.md
[CREATE ] 生态参考/sandbox/capsule/capsule.md -> entities/capsule.md
[CREATE ] 生态参考/sandbox/drasi/drasi.md -> entities/drasi.md
[CREATE ] 生态参考/sandbox/microcks/microcks.md -> entities/microcks.md
[CREATE ] 生态参考/sandbox/stacker/stacker.md -> entities/stacker.md
[CREATE ] 生态参考/sandbox/kube-rs/kube-rs.md -> entities/kube-rs.md
[CREATE ] 生态参考/sandbox/schemahero/schemahero.md -> entities/schemahero.md
[CREATE ] 生态参考/sandbox/perses/perses.md -> entities/perses.md
[CREATE ] 生态参考/sandbox/kubearmor/kubearmor.md -> entities/kubearmor.md
[CREATE ] 生态参考/sandbox/ovn-kubernetes/ovn-kubernetes.md -> entities/ovn-kubernetes.md
[CREATE ] 生态参考/sandbox/modelpack/modelpack.md -> entities/modelpack.md
[CREATE ] 生态参考/sandbox/devspace/devspace.md -> entities/devspace.md
[CREATE ] 生态参考/sandbox/sermant/sermant.md -> entities/sermant.md
[CREATE ] 生态参考/sandbox/kmesh/kmesh.md -> entities/kmesh.md
[CREATE ] 生态参考/sandbox/ratify/ratify.md -> entities/ratify.md
[CREATE ] 生态参考/sandbox/carina/carina.md -> entities/carina.md
[CREATE ] 生态参考/sandbox/distribution/distribution.md -> entities/distribution.md
[CREATE ] 生态参考/sandbox/kgateway/kgateway.md -> entities/kgateway.md
[CREATE ] 生态参考/sandbox/carvel/carvel.md -> entities/carvel.md
[CREATE ] 生态参考/sandbox/kubean/kubean.md -> entities/kubean.md
[CREATE ] 生态参考/sandbox/vscode-kubernetes-tools/vscode-kubernetes-tools.md -> entities/vscode-kubernetes-tools.md
[CREATE ] 生态参考/sandbox/bpfman/bpfman.md -> entities/bpfman.md
[CREATE ] 生态参考/sandbox/spin/spin.md -> entities/spin.md
[CREATE ] 生态参考/sandbox/kured/kured.md -> entities/kured.md
[CREATE ] 生态参考/sandbox/kusionstack/kusionstack.md -> entities/kusionstack.md
[CREATE ] 生态参考/sandbox/kubeelasti/kubeelasti.md -> entities/kubeelasti.md
[CREATE ] 生态参考/sandbox/containerssh/containerssh.md -> entities/containerssh.md
[CREATE ] 生态参考/sandbox/akri/akri.md -> entities/akri.md
[CREATE ] 生态参考/sandbox/paralus/paralus.md -> entities/paralus.md
[CREATE ] 生态参考/sandbox/kuadrant/kuadrant.md -> entities/kuadrant.md
[CREATE ] 生态参考/sandbox/radius/radius.md -> entities/radius.md
[CREATE ] 生态参考/sandbox/slimtoolkit/slimtoolkit.md -> entities/slimtoolkit.md
[CREATE ] 生态参考/sandbox/k8sgpt/k8sgpt.md -> entities/k8sgpt.md
[CREATE ] 生态参考/sandbox/kanister/kanister.md -> entities/kanister.md
[CREATE ] 生态参考/sandbox/hami/hami.md -> entities/hami.md
[CREATE ] 生态参考/sandbox/k8gb/k8gb.md -> entities/k8gb.md
[CREATE ] 生态参考/sandbox/atlantis/atlantis.md -> entities/atlantis.md
[CREATE ] 生态参考/sandbox/score/score.md -> entities/score.md
[CREATE ] 生态参考/sandbox/bank-vaults/bank-vaults.md -> entities/bank-vaults.md
[CREATE ] 生态参考/sandbox/zot/zot.md -> entities/zot.md
[CREATE ] 生态参考/sandbox/kuberhealthy/kuberhealthy.md -> entities/kuberhealthy.md
[CREATE ] 生态参考/sandbox/container2wasm/container2wasm.md -> entities/container2wasm.md
[CREATE ] 生态参考/sandbox/open-policy-containers/open-policy-containers.md -> entities/open-policy-containers.md
[CREATE ] 生态参考/sandbox/kairos/kairos.md -> entities/kairos.md
[CREATE ] 生态参考/sandbox/k0s/k0s.md -> entities/k0s.md
[CREATE ] 生态参考/sandbox/kpt/kpt.md -> entities/kpt.md
[CREATE ] 生态参考/sandbox/dalec/dalec.md -> entities/dalec.md
[CREATE ] 生态参考/sandbox/konveyor/konveyor.md -> entities/konveyor.md
[CREATE ] 生态参考/sandbox/metallb/metallb.md -> entities/metallb.md
[CREATE ] 生态参考/sandbox/spiderpool/spiderpool.md -> entities/spiderpool.md
[CREATE ] 生态参考/sandbox/composefs/composefs.md -> entities/composefs.md
[CREATE ] 生态参考/sandbox/piraeus-datastore/piraeus-datastore.md -> entities/piraeus-datastore.md
[CREATE ] 生态参考/sandbox/kube-burner/kube-burner.md -> entities/kube-burner.md
[CREATE ] 生态参考/sandbox/telepresence/telepresence.md -> entities/telepresence.md
[CREATE ] 生态参考/sandbox/k3s/k3s.md -> entities/k3s.md
[CREATE ] 生态参考/sandbox/kuasar/kuasar.md -> entities/kuasar.md
[CREATE ] 生态参考/sandbox/interlink/interlink.md -> entities/interlink.md
[CREATE ] 生态参考/sandbox/bootc/bootc.md -> entities/bootc.md
[CREATE ] 生态参考/sandbox/openfunction/openfunction.md -> entities/openfunction.md
[CREATE ] 生态参考/sandbox/dex/dex.md -> entities/dex.md
[CREATE ] 生态参考/sandbox/cohdi/cohdi.md -> entities/cohdi.md
[CREATE ] 生态参考/sandbox/kubeslice/kubeslice.md -> entities/kubeslice.md
[CREATE ] 生态参考/sandbox/k8up/k8up.md -> entities/k8up.md
[CREATE ] 生态参考/sandbox/xregistry/xregistry.md -> entities/xregistry.md
[CREATE ] 生态参考/sandbox/runme-notebooks/runme-notebooks.md -> entities/runme-notebooks.md
[CREATE ] 生态参考/sandbox/ko/ko.md -> entities/ko.md
[CREATE ] 生态参考/sandbox/porter/porter.md -> entities/porter.md
[CREATE ] 生态参考/sandbox/hyperlight/hyperlight.md -> entities/hyperlight.md
[CREATE ] 生态参考/sandbox/tokenetes/tokenetes.md -> entities/tokenetes.md
[CREATE ] 生态参考/sandbox/kuma/kuma.md -> entities/kuma.md
[CREATE ] 生态参考/sandbox/hwameistor/hwameistor.md -> entities/hwameistor.md
[CREATE ] 生态参考/sandbox/open-cluster-management/open-cluster-management.md -> entities/open-cluster-management.md
[CREATE ] 生态参考/sandbox/meshery/meshery.md -> entities/meshery.md
[CREATE ] 生态参考/sandbox/kagent/kagent.md -> entities/kagent.md
[CREATE ] 生态参考/sandbox/cozystack/cozystack.md -> entities/cozystack.md
[CREATE ] 生态参考/sandbox/kube-ovn/kube-ovn.md -> entities/kube-ovn.md
[CREATE ] 生态参考/sandbox/clusternet/clusternet.md -> entities/clusternet.md
[CREATE ] 生态参考/sandbox/sops/sops.md -> entities/sops.md
[CREATE ] 生态参考/sandbox/koordinator/koordinator.md -> entities/koordinator.md
[CREATE ] 生态参考/sandbox/inclavare-containers/inclavare-containers.md -> entities/inclavare-containers.md
[CREATE ] 生态参考/sandbox/cdk8s/cdk8s.md -> entities/cdk8s.md
[CREATE ] 生态参考/sandbox/external-secrets/external-secrets.md -> entities/external-secrets.md
[CREATE ] 生态参考/sandbox/slimfaas/slimfaas.md -> entities/slimfaas.md
[CREATE ] 生态参考/sandbox/kitops/kitops.md -> entities/kitops.md
[CREATE ] 生态参考/sandbox/hexa/hexa.md -> entities/hexa.md
[CREATE ] 生态参考/sandbox/wasmedge/wasmedge.md -> entities/wasmedge.md
[CREATE ] 生态参考/sandbox/kudo/kudo.md -> entities/kudo.md
[CREATE ] 生态参考/sandbox/openchoreo/openchoreo.md -> entities/openchoreo.md
[CREATE ] 生态参考/sandbox/keylime/keylime.md -> entities/keylime.md
[CREATE ] 生态参考/sandbox/cadence/cadence.md -> entities/cadence.md
[CREATE ] 生态参考/sandbox/opengemini/opengemini.md -> entities/opengemini.md
[CREATE ] 生态参考/sandbox/kcp/kcp.md -> entities/kcp.md
[CREATE ] 生态参考/sandbox/cloudnativepg/cloudnativepg.md -> entities/cloudnativepg.md
[CREATE ] 生态参考/sandbox/cedar/cedar.md -> entities/cedar.md
[CREATE ] 生态参考/sandbox/kcl/kcl.md -> entities/kcl.md
[CREATE ] 生态参考/sandbox/serverless-workflow/serverless-workflow.md -> entities/serverless-workflow.md
[CREATE ] 生态参考/sandbox/armada/armada.md -> entities/armada.md
[CREATE ] 生态参考/sandbox/urunc/urunc.md -> entities/urunc.md
[CREATE ] 生态参考/sandbox/opentofu/opentofu.md -> entities/opentofu.md
[CREATE ] 生态参考/sandbox/oxia/oxia.md -> entities/oxia.md
[CREATE ] 生态参考/sandbox/cartography/cartography.md -> entities/cartography.md
[CREATE ] 生态参考/sandbox/athenz/athenz.md -> entities/athenz.md
[CREATE ] 生态参考/sandbox/oscal-compass/oscal-compass.md -> entities/oscal-compass.md
[CREATE ] 生态参考/sandbox/shipwright/shipwright.md -> entities/shipwright.md
[CREATE ] 生态参考/sandbox/easegress/easegress.md -> entities/easegress.md
[CREATE ] 生态参考/sandbox/headlamp/headlamp.md -> entities/headlamp.md
[CREATE ] 生态参考/sandbox/submariner/submariner.md -> entities/submariner.md
[CREATE ] 生态参考/sandbox/parsec/parsec.md -> entities/parsec.md
[CREATE ] 生态参考/sandbox/kube-vip/kube-vip.md -> entities/kube-vip.md
[CREATE ] 云厂商/02-google-cloud-gke/google-cloud-gke-overview.md -> references/google-cloud-gke-overview.md
[CREATE ] 云厂商/06-huawei-cce/huawei-cce-overview.md -> references/huawei-cce-overview.md
[CREATE ] 云厂商/03-azure-aks/azure-aks-overview.md -> references/azure-aks-overview.md
[CREATE ] 云厂商/09-oracle-oke/oracle-oke-overview.md -> references/oracle-oke-overview.md
[CREATE ] 云厂商/07-ucloud-uk8s/ucloud-uk8s-overview.md -> references/ucloud-uk8s-overview.md
[CREATE ] 云厂商/13-alicloud-apsara-ack/251-apsara-stack-sls-logging.md -> references/251-apsara-stack-sls-logging.md
[CREATE ] 云厂商/13-alicloud-apsara-ack/alicloud-apsara-ack-overview.md -> references/alicloud-apsara-ack-overview.md
[CREATE ] 云厂商/13-alicloud-apsara-ack/250-apsara-stack-ess-scaling.md -> references/250-apsara-stack-ess-scaling.md
[CREATE ] 云厂商/13-alicloud-apsara-ack/252-apsara-stack-pop-operations.md -> references/252-apsara-stack-pop-operations.md
[CREATE ] 云厂商/01-aws-eks/aws-eks-overview.md -> references/aws-eks-overview.md
[CREATE ] 云厂商/12-ecloud-cke/ecloud-cke-overview.md -> references/ecloud-cke-overview.md
[CREATE ] 云厂商/10-volcengine-vek/volcengine-vek-overview.md -> references/volcengine-vek-overview.md
[CREATE ] 云厂商/05-tencent-tke/tencent-tke-overview.md -> references/tencent-tke-overview.md
[CREATE ] 云厂商/04-alicloud-ack/243-ack-ram-authorization.md -> references/243-ack-ram-authorization.md
[CREATE ] 云厂商/04-alicloud-ack/244-ack-ros-iac.md -> references/244-ack-ros-iac.md
[CREATE ] 云厂商/04-alicloud-ack/service-ack-practical-guide.md -> references/service-ack-practical-guide.md
[CREATE ] 云厂商/04-alicloud-ack/240-ack-ecs-compute.md -> references/240-ack-ecs-compute.md
[CREATE ] 云厂商/04-alicloud-ack/242-ack-vpc-network.md -> references/242-ack-vpc-network.md
[CREATE ] 云厂商/04-alicloud-ack/alicloud-ack-overview.md -> references/alicloud-ack-overview.md
[CREATE ] 云厂商/04-alicloud-ack/241-ack-slb-nlb-alb.md -> references/241-ack-slb-nlb-alb.md
[CREATE ] 云厂商/04-alicloud-ack/245-ack-ebs-storage.md -> references/245-ack-ebs-storage.md
[CREATE ] 云厂商/11-ctyun-tke/ctyun-tke-overview.md -> references/ctyun-tke-overview.md
[CREATE ] 云厂商/08-ibm-iks/ibm-iks-overview.md -> references/ibm-iks-overview.md
[CREATE ] 网络/41-terway-architecture-deep-dive.md -> entities/41-terway-architecture-deep-dive.md
[CREATE ] 网络/43-terway-crd-operations.md -> entities/43-terway-crd-operations.md
[CREATE ] 网络/44-terway-operations-manual.md -> entities/44-terway-operations-manual.md
[CREATE ] 网络/40-terway-product-overview.md -> entities/40-terway-product-overview.md
[CREATE ] 网络/42-terway-usage-guide.md -> entities/42-terway-usage-guide.md
[CREATE ] 网络/46-terway-performance-tuning.md -> entities/46-terway-performance-tuning.md
[CREATE ] 网络/45-terway-testing-validation.md -> entities/45-terway-testing-validation.md
[CREATE ] 网络/47-terway-troubleshooting-fta.md -> entities/47-terway-troubleshooting-fta.md
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

将 domain-34-cncf-landscape、domain-17-cloud-provider、网络 的未摄入文档完成 wiki-ingest。

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

- [2026-05-21] CORPUS-CONFIG-UPDATE: 重构全量语料配置，引入'提炼知识 + 源文档'双层结构。更新 rag-full-profile.yaml（添加 concepts/ entities/ skills/ references/ synthesis/ 提炼知识层 + domain-*/topic-*/docs/ 源文档层 + 生态参考/_archived-release-notes/ 按需层）、rag-sre-profile.yaml（添加 skills/ entities/ concepts/ synthesis/ 提炼知识）、rag-learning-profile.yaml（添加 concepts/ skills/learn-* 提炼知识）、notebooklm-profile.yaml（添加 concepts/ skills/ synthesis/ 高质量提炼页面）、rag-chunking-strategy.md（分层分块策略 + 元数据增强规范）、README.md（双层结构文档 + 快速开始指南）。全部 YAML 语法验证通过。

- [2026-05-21] STRUCTURE-REFACTOR: 目录结构规范化。创建 STRUCTURE.md 文档化四层目录结构；metadata/ 移入 _meta/metadata/；reports/ 改名为 _reports/；更新所有引用路径；全部 4 个 corpus-config profile 添加工程工具目录排除。

## Related

- [[22-概念/10-最佳实践/bp-observability.md|bp-observability]] — 最佳实践：Observability
- [[22-概念/10-最佳实践/bp-infrastructure.md|bp-infrastructure]] — 最佳实践：Infrastructure
- [[22-概念/05-安全/kubernetes-pki-certificate-system.md|kubernetes-pki-certificate-system]] — Kubernetes PKI 证书体系
- [[22-概念/10-最佳实践/bp-common-best-practices.md|bp-common-best-practices]] — Kubernetes 通用最佳实践参考
- [[22-概念/02-工作负载/deployment-controller-architecture.md|deployment-controller-architecture]] — Deployment 控制器架构
- [2026-05-21] CROSS_LINK pages_scanned=588 links_added=2883 typed_relations_written=0 pages_modified=582 orphans_remaining=38 misc_affinity_updated=0 promotion_candidates=0
- [2026-05-21] LINT_CONSOLIDATE links_fixed=0 orphans_rescued=496 lifecycle_updates=8 tier_demotions=0 tag_fixes=0 contradiction_callouts=0 report=synthesis/consolidation-2026-05-21.md
- [2026-05-21] CROSS_LINK_FIX broken_links_fixed=152 nested_fixed=39 md_suffix_fixed=3041 path_normalized=5398 files_modified=83 final_orphans≈0 final_broken_links=0

- [2026-05-21T09:32:13Z] LINT issues_found=6436 orphans=2659 broken_links=262 stale=0 contradictions=0 prov_issues=0 missing_summary=3031 fragmented_clusters=31 visibility_issues=407 promotion_candidates=0 synthesis_gaps=36 relationship_issues=0

- [2026-05-21T17:45:00Z] LINT_FIX_EXECUTED — nested_wikilinks_fixed=940, cheat_sheet_template_fixed=15, contributing_fixed=1, moc_nested_fixed=1, topic_readme_refs_fixed=5, escaped_wikilinks_fixed=48, reports_chinese_fixed=3, domain_x_placeholder_fixed=1, moc_path_mismatch_fixed=106 (domain-41:50, domain-43:6, domain-42:50), domain42_moc_entries_added=47, meta_reports_frontmatter_fixed=31, shell_code_false_positives=0 (skipped, all in code blocks)

### [2026-05-21T18:30:00Z] DOMAIN-REFACTOR: 生产环境维度 Domain 整合

**触发原因**: 43 个 Domain 存在维度混杂（技术组件+工具品牌+内容载体+生命周期+部署场景五维混用），内容重叠率 25-30%，生产环境故障排查需跨 5-7 个目录检索。

**执行操作**:
- 合并可观测性三域 (8+20+21) → `可观测性`
- 合并安全三域 (7+25+39) → `安全`
- 拆分 production-operations (18) → 8 个目标域
- 合并 platform-ops + platform-engineering (9+36) → `平台工程`
- 合并网络相关四域 (5+15+26+35+40) → `网络`
- 合并存储两域 (6+16) → `存储`
- 合并架构基础三域 (1+2+3) → `集群基础`
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
  - 可靠性: 从 18 文件扩充到 35 文件 (+17)。新增 SLO/SLI 体系 (4 文件: SLI 定义、SLO 实施、错误预算、Burn Rate 告警)、混沌工程 (4 文件: 概述、Chaos Mesh、实验设计、Litmus)、事后复盘 (2 文件: 无责模板、文化指南)、SRE 实践 (4 文件: 可用性计算、发布门控、事故指挥、Toil 削减)、性能测试 (2 文件: 负载测试、混沌集成)、灾备手册 (2 文件: 场景目录、AZ 故障恢复)。
  - 数据库中间件: 从 13 文件扩充到 22 文件 (+9)。新增消息队列 (3 文件: NATS、Pulsar、选型对比)、时序数据库 (2 文件: Prometheus TSDB、InfluxDB vs TimescaleDB)、Operator 管理 (2 文件: 设计模式、MySQL/PG/Redis 对比)、数据流处理 (2 文件: CDC、流处理框架)。
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

## [2026-05-24] update | wiki-lint 全面修复执行
- P0: taxonomy 补全 34 个高频标签
- P0: orphan 入站链接修复（概念 stub 30 + 报告 14 + 归档 78 + 元数据 3 + cross-linker 18）= 143 links
- P1: 碎片化标签 cross-linker（6 个标签, 18 files）
- P1: hub 页面 summary 补全（18 个 top hub 页面）
- P2: 创建 5 个高价值 synthesis 页面
- P2: 模糊匹配链接规范化（4 files, 6 links）
- 总计: 74 files changed, 143+ new wikilinks, 5 new pages

## [2026-05-24] dedup | wiki-dedup 审计与合并
- mode=merge pages_scanned=4762 pairs_found=795 merged=325 kept_separate=470 needs_review=0
- 删除 domain-20 旧目录 01-reference-architectures（96 文件，与 topic-application-architecture 完全相同）
- 删除 CNCF landscape 实体重复（229 文件，与 entities/ 近似相同）
- 修复 327 个指向已删除文件的 wikilinks
- 添加 12 个 training 讲师/公开版交叉引用
- 0 内容损失（所有删除的文件都有更完整的对应版本）

- [2026-05-24] WIKI_RESEARCH topic="Kubernetes 存储深度研究" rounds=3 sources_fetched=8 pages_created=5
  - 创建 concepts/csi-drivers.md（从 stub 更新为完整内容，14.9KB）
  - 创建 concepts/cloud-native-storage-systems.md（19.8KB，Longhorn/Rook-Ceph/OpenEBS/JuiceFS 对比）
  - 创建 concepts/storage-performance-optimization.md（8.8KB，基准测试/NVMe 调优/QoS）
  - 创建 concepts/storage-data-protection.md（12.1KB，Velero/不可变备份/DR 策略）
  - 创建 synthesis/Research: Kubernetes Storage 2025-2026.md（4.9KB，主合成页）

- [2026-05-24] WIKI_RESEARCH topic="Kubernetes 可靠性工程深度研究" rounds=3 sources_fetched=10 pages_created=7
  - 创建 concepts/slo-error-budget-framework.md（22KB，SLO/Error Budget 框架）
  - 创建 concepts/chaos-engineering-platforms.md（14.5KB，混沌工程平台对比）
  - 创建 concepts/incident-management-patterns.md（15.8KB，事件管理与复盘模式）
  - 创建 concepts/capacity-planning-cost-optimization.md（7.3KB，容量规划与成本优化）
  - 创建 concepts/multi-cluster-dr-automation.md（10.6KB，多集群灾备与自动化）
  - 创建 synthesis/Research: Kubernetes Reliability Engineering 2025-2026.md（4.8KB，合成页）

- [2026-05-24] WIKI_RESEARCH topic="Kubernetes 生产运营深度研究" rounds=3 sources_fetched=8 pages_created=3
  - 创建 concepts/gitops-production-operations.md（15.6KB，GitOps/ArgoCD/Flux/CAPI/Fleet）
  - 创建 concepts/finops-greenops-practices.md（19KB，FinOps/GreenOps/GPU/Spot）
  - 创建 synthesis/Research: Kubernetes Production Operations 2025-2026.md（3.7KB，合成页）

- [2026-05-24] WIKI_RESEARCH topic="Kubernetes AI/ML 基础设施" rounds=3 sources_fetched=10 pages_created=2
  - 创建 concepts/k8s-ai-ml-infrastructure.md（8.2KB，GPU/DRA/LLM/Kubeflow/Ray）
  - 创建 synthesis/Research: Kubernetes AI-ML Infrastructure 2025-2026.md（3.9KB）

- [2026-05-24] WIKI_RESEARCH topic="Kubernetes 安全合规" rounds=3 sources_fetched=8 pages_created=2
  - 创建 concepts/k8s-security-compliance.md（9.7KB，供应链/策略引擎/运行时/密钥/网络/PSS/CIS）
  - 创建 synthesis/Research: Kubernetes Security Compliance 2025-2026.md（4.4KB）

- [2026-05-24] WIKI_RESEARCH topic="Kubernetes 网络演进" rounds=3 sources_fetched=12 pages_created=2
  - 创建 concepts/k8s-networking-evolution.md（6KB，CNI/Service Mesh/Gateway API/eBPF/DNS）
  - 创建 synthesis/Research: Kubernetes Networking 2025-2026.md（4.8KB）

- [2026-05-24] WIKI_RESEARCH topic="Kubernetes 可观测性" rounds=3 sources_fetched=8 pages_created=2
  - 创建 concepts/k8s-observability-stack.md（6.3KB，OTel/Prometheus 3.0/Grafana LGTM/eBPF）
  - 创建 synthesis/Research: Kubernetes Observability 2025-2026.md（5.4KB）

- [2026-05-24] WIKI_RESEARCH topic="Kubernetes 平台工程" rounds=3 sources_fetched=8 pages_created=2
  - 创建 concepts/platform-engineering-idp.md（5.6KB，IDP/Backstage/Crossplane/Humanitec/Kratix）
  - 创建 synthesis/Research: Kubernetes Platform Engineering 2025-2026.md（5.6KB）

- [2026-05-24] WIKI_RESEARCH topic="Kubernetes 发布变更管理" rounds=3 sources_fetched=10 pages_created=2
  - 创建 concepts/progressive-delivery-strategies.md（6.6KB，Argo Rollouts/Canary/Blue-Green/K8S 版本）
  - 创建 synthesis/Research: Kubernetes Release Change Management 2025-2026.md（6.2KB）

- [2026-05-24] WIKI_RESEARCH topic="Kubernetes 容器运行时" rounds=3 sources_fetched=10 pages_created=2
  - 创建 concepts/container-runtime-evolution.md（2.8KB，containerd 2.x/WASM/CoCo/懒加载）
  - 创建 synthesis/Research: Kubernetes Container Runtime 2025-2026.md（5.2KB）

- [2026-05-24] WIKI_RESEARCH topic="Kubernetes 专项技术" rounds=3 sources_fetched=8 pages_created=2
  - 创建 concepts/specialized-k8s-technologies.md（2.7KB，eBPF/WASM/边缘/Serverless）
  - 创建 synthesis/Research: Kubernetes Specialized Technologies 2025-2026.md（6KB）

- [2026-05-24] WIKI_RESEARCH topic="Kubernetes 云厂商集成" rounds=3 sources_fetched=8 pages_created=2
  - 创建 concepts/cloud-provider-k8s-integration.md（9KB，EKS/GKE/AKS/ACK/多云抽象）
  - 创建 synthesis/Research: Kubernetes Cloud Providers 2025-2026.md（4KB）

- [2026-05-24] WIKI_RESEARCH topic="Kubernetes 应用模式" rounds=3 sources_fetched=10 pages_created=2
  - 创建 concepts/application-patterns-k8s.md（10.2KB，Ambient Mesh/Kueue/vCluster/Dapr）
  - 创建 synthesis/Research: Kubernetes Application Patterns 2025-2026.md（4.3KB）

- [2026-05-24] WIKI_RESEARCH topic="Kubernetes 系统基础" rounds=3 sources_fetched=11 pages_created=2
  - 创建 concepts/system-foundation-hardware-kernel.md（7KB，DPU/GPU/ARM64/cgroup v2/eBPF/PSI）
  - 创建 synthesis/Research: Kubernetes System Foundation 2025-2026.md（4.6KB）

- [2026-05-24] LINT 修复 broken wikilinks
  - 概念页：63 个 broken links 修复（10 个文件）
  - 合成页：30 个 broken links 修复（6 个文件）
  - 全部新概念页 orphan 检查通过（0 orphan）
  - 提交：3615d3b8

- [2026-05-24] CROSS_LINK 增强新页面交叉链接
  - 19 概念页添加 Related 段落（每页 2-3 个跨概念链接）
  - 11 合成页添加跨域关联段落（每页 4 个跨域链接）
  - 总计新增 ~100 个 wikilinks

- [2026-07-02] LINT issues_found=935 orphans=406 broken_links=935 stale=37 lifecycle_issues=4 visibility_issues=2 missing_summary=9 broken_links_fixed=441 stale_refreshed=37
  - 441 directory-as-wikilink 修复（135 个 index.md 文件）
  - 37 stale core pages last_updated 刷新至 2026-07-02
  - topic-index Chinese title 修复（进行中）
  - 报告: _reports/wiki-lint-2026-07-02.md


<!-- risk-assessed -->

- [2026-07-02] MAINTENANCE_CYCLE broken_links=935→0 orphans=406→305 stale_core=37_released lifecycle_fixed=5 pii_sources=2 cross_links_added=20 release_pages=3329 release_qa=15094 release_tokens=11553184

- [2026-07-02] STATUS corpus=3329(core=804,sup=1509,per=1016) vault=5553 orphans=305 broken=0 release_tokens=11553184 qa_pairs=15094 report=_reports/wiki-status-2026-07-02.md

- [2026-07-02] QUALITY_CYCLE sre_export=472pages/1.96M_tokens cross_linker_r2=14files_edited synthesis_added=4(kubernetes-prometheus,kubernetes-etcd,kubernetes-service,service-ingress) release_verified=PASS qa_index_created broken_links=0(vault)+1(_archives,skipped) report=_reports/full-quality-cycle-2026-07-02.md

- [2026-07-02] RELEASE_RESTRUCTURE layout=release/{scripts,package/<DATE_TIME>/} packages=2(18-29+18-40) script_default=auto-timestamp safety_guard=added(vault/release/scripts/package protected) AGENT-USAGE.md=written

- [2026-07-02] CORPUS_V2 profile_update=rag-full-profile.yaml(+synthesis/+topic-dictionary) export=3903pages/12.3M_tokens delta=+574pages/+724K_tokens synthesis=10pages topic_dictionary=564pages tier_counts=core:1122/sup:1693/per:1088 index.md=regenerated AGENT-USAGE.md=updated package=release/package/2026-07-02_18-53/

- [2026-08-25] MAINTENANCE_CYCLE broken_wikilinks=109→0(remap=108,manual=1,files_edited=26) readme_sync=18→0 frontmatter_OK(4491) heading_OK(3972) ruff_OK path_sync=scripts→31-脚本 domain_mapping=updated(09/10/22) changelog=updated tools=ruff0.16.4(uv)

- [2026-08-28] RESEARCH topic=kubernetes-v1.37-garhwal released=2026-08-26 enhancements=67(stable=16,beta=23,alpha=27) docs_added=2(upgrade-guide@01-集群基础/06-升级路径/05-kubernetes-v1.37-upgrade-guide.md + release-research@25-研究/04-可靠性与运维/kubernetes-v1.37-release-research.md) indexes_updated=2(升级路径+研究MOC) deprecations=ipvs(KEP-5495,v1.40禁用/v1.43移除)+kube-dns+static-pod-api-refs(逃生门移除)+cgroup-v1(KEP-5573)+selinux-mount-ga(KEP-1710) gates=frontmatter_OK+wikilinks=0+readme_sync_OK
