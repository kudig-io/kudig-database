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

- [2026-10-14] VERSION_WATCH argocd=v3.5.3→v3.5.5(coordinated-security-release:4CRITICAL+2HIGH+2MODERATE, breaking=helmGlobals.configHome+4MiB-OCI-limit) others_unchanged(k8s=v1.37.1/cilium=v1.20.2/istio=1.31.1/prometheus=v3.15.0/terway=v1.17.7) entity_updated=23-实体/08-交付与制品/argocd.md(+安全通告章节+verified-upon=argocd-3.5.5)

- [2026-10-14] RELEASE_NOTES_REFRESH scope=23-实体/15-参考与索引 8个索引页 34项目最新版全部经GitHub-API核对(2026-10) highlights=helm4.3/kustomize5.8/grafana13.2/opa1.21/prometheus3.15/argo-cd3.5/tekton1.17/envoy1.40/etcd3.7/containerd2.4/cri-o1.37/calico3.33/rook1.21 kubernetes-changelog=v1.36→v1.37 entity_kubernetes=+版本状态行(类别修正Observability→Orchestration) readme_version_claim=v1.32+→v1.37 script=download-release-notes.py(changelog范围+1.37)

- [2026-10-14] CORPUS_REGEN unique_pairs=511(+6seed:GPU/HELM/WEBHOOK各2条) fta_coverage=50/50(44→50) domain_coverage=18/18(15/18→PASS, 补齐GPU+HELM+WEBHOOK) severity=WARN(结构性medium偏重) command_diversity=FAIL(9.7%,生成器复用核心诊断命令所致,存量已知缺口) with_actions=再生成(32块)

- [2026-10-14] QUALITY_CYCLE wikilinks=54→0(改名残留重映射:5个旧名→连字符名,40文件53链+1手动) cross_refs=2→0(backup-restore-fta前缀恢复+cni-architecture-fundamentals 02→03) readme_sync=修复TREE_EXEMPT双向豁免(30-站点/33-源码为gitignore目录) gtm=sync+check_OK(4446篇) frontmatter=0errors(4536文件,164warns存量) heading=0bad(4016文件) ruff=OK code_examples=OK metrics=refreshed(broken_wikilinks=0,avg_inbound=8.74)

- [2026-10-14] CONTENT_EXPANSION docs_added=11(+dup1已清理) lines_total=~4900 gap_analysis=云原生薄弱区扫描(40+主题计数) topics=镜像分发加速(eStargz/Nydus/Dragonfly)+OCI1.1-artifact+不可变OS(Talos/Bottlerocket/Flatcar)+机密计算CoCo(TDX/SEV-SNP/CCA)+GatewayAPI生态对比(EG/APISIX/Higress/istio-GAMMA)+KubeRay+GPU共享切分(MIG/time-slicing/vGPU/DRA)+Kueue-vs-Volcano+KubeVela-OAM+Knative-Eventing+湖仓onK8s(Iceberg/Paimon) domains=14容器运行时+3/15AI基础设施+3/05网络+1/08安全+1/04应用模式+2/07数据库中间件+1 versions=全部gh-api核对(KubeRay1.7.1/Kueue0.20.1/Volcano1.16.0/CoCo-op0.17.0/Kata4.2.0/oras1.3.4等) indexes=6个目录index补录 readmes=根README6域篇数与描述同步 gtm=4446→4457 gates=wikilinks0+frontmatter0err(4547)+heading0bad(4027)+readme_sync_OK metrics=refreshed note=AI板块由并行子代理产出后人工校验修复断链1条与编号重排(40/41重复清理)

- [2026-10-15] CONTENT_EXPANSION_R2 docs_added=5 lines=~2265 topics=CiliumMesh多集群网络(ClusterMesh/GlobalService/跨集群策略)+DPU智能网卡卸载(BlueField/IPU/卸载层级)+GreenOps可持续算力(SCI/Kepler/碳感知调度)+边缘设备接入(Akri/SuperEdge vs KubeEdge/OpenYurt/K3s)+Wasm云原生运行时(runwasi/SpinKube/wasmCloud/WASI-P2) domains=05-网络+2/13-生产运维+1/16-专项技术+2 versions=gh核对(Kepler0.12.0/Akri0.14.0/spin-operator0.6.1/runwasi0.6.1/wasmCloud2.10.3/cilium1.20.2) indexes=3目录补录 readmes=根README3域篇数同步 gtm=4457→4464 gates=wikilinks0+readme_sync_OK metrics=refreshed 两轮累计=16篇/~7200行/9域

- [2026-10-16] INTERVIEW_QA_APPEND q_added=Q9-Q12 topics=apiserver整体变慢排查(etcd第一嫌疑人:fsync<10ms红线/db-size/leader状态→再APF队列+慢webhook)+etcd-NOSPACE三步修复(compact→逐台defrag→alarm-disarm,quota默认2GB上限8GB)+大LIST与event洪泛定位(storage_objects+audit-responseSize+flowcontrol-rejected)+apiserver重启LIST风暴(watch断连→reflector全量relist→watch cache冷启动回源etcd) file=36-报告/k8s-高级面试快问快答-沉淀.md(Q1-Q8→Q12)

- [2026-10-16] INTERVIEW_QA_APPEND q_added=Q13-Q20 topics=集群组成组件专项(scheduler过滤打分绑定+percentage-of-nodes-to-score采样/KCM控制器盘点与scheduler分工+Lease15s-10s-2s/kubelet三大名场面PLEG relist1s+静态Pod mirror+cgroup-driver一致性/iptables-vs-IPVS规模分界+nftables+eBPF绕开/控制面HA三层apiserver无状态+KCM-SCH单活Lease+stacked-vs-external-etcd/CRI链路pause锚netns+shim独立父子/CoreDNS ndots5放大6倍+conntrack-race-5s+NodeLocal-169.254.20.10/kubeadm证书1年到期renew-all后必须重启静态Pod+CA10年不自动续) file=36-报告/k8s-高级面试快问快答-沉淀.md(Q1-Q12→Q20) sources=01-集群基础/03-控制平面/{13,15,16,20,21,38,03}-deep-dive

- [2026-10-16] INTERVIEW_QA_APPEND q_added=Q21-Q25 topics=阿里云ACK网络/Terway选型(Terway-vs-Flannel四对比点+SLB直通/五模式性能密度取舍VPC路由48条上限+ENI独占+ENIIP默认+Trunking密度王+IPvlan内核4.19+建集群选定不可切/容量公式(最大ENI数-1)×每ENI辅助IP数+VPC网段不可改+Pod独立vSwitch/IP不足四步收敛vSwitch余量→ENI配额→terway-cli→日志+OpenAPI-100QPS瓶颈+Trunking预分配/NetworkPolicy按模式三实现安全组+iptables+eBPF只支持L3L4+ReservedIP固定IP+CRD五件套+两级IPAM本地池+GC60s) file=36-报告/k8s-高级面试快问快答-沉淀.md(Q1-Q20→Q25) sources=05-网络/06-Terway/{01-product,02-architecture}+18-云厂商/01-阿里云/公有云-ACK/03-Terway-CNI网络+004-ack-vpc-network note=03旧文档三种模式与专题五模式不一致以专题为准

- [2026-10-16] INTERVIEW_QA_APPEND q_added=Q26-Q36 topics=Node与Pod专项(NotReady时间线10s-40s-taint-5min+双通道心跳KEP0009写放大+Lease永不删除/五conditions×TaintNodesByCondition拦截在污点层/capacity-vs-allocatable预留扣减+enforceNodeAllocatable/drain三flag卡点+eviction-API-vs-delete+PDB只管自愿驱逐/单节点NotReady五步刀法10248-10250/三层状态phase-conditions-containerState/三探针只有readiness摘流/137不一定是OOM+pre vious日志/优雅终止宽限期含PreStop+preStop-sleep让摘流/QoS倒序驱逐+CPU限流内存杀+PDB不管资源压力/init-vs-native-sidecar三痛点+1.28alpha-1.29beta) file=36-报告/k8s-高级面试快问快答-沉淀.md(Q1-Q25→Q36) sources=22-概念/08-可靠性与运维/node-lifecycle-management+19-故障诊断/01-核心排障/06-node-notready-diagnosis+01-集群基础/03-控制平面/15-kubelet-deep-dive+17-系统基础/06-知识字典/workloads/pod-lifecycle+19-故障诊断/04-高级排障/structural-05-workloads/01-pod-troubleshooting note=与Q13-Q20去重(PLEG/静态Pod/cgroup-driver在Q15,pause在Q18,调度失败在Q13)交叉引用不重复

- [2026-10-16] INTERVIEW_QA_COMPANION file=36-报告/k8s-集群核心组件-快问快答-白话版.md(new) covers=Q13-Q20八组件白话讲解(全局比喻:apiserver前台/etcd档案室/scheduler分工HR/KCM盯KPI经理/kubelet车间主任/kube-proxy门牌员/containerd搬砖工/CoreDNS查号台)+串联题4条 purpose=简单易懂自学版与沉淀版互补 沉淀文件divider加互链

- [2026-10-16] INTERVIEW_QA_APPEND q_added=Q37-Q47 topics=Node与Pod专项第二批(SyncPod施工链sandbox→init→main→probe+卡点按阶段对号/bootstrap-token→CSR→csrapproving→注册四关/驱逐软硬阈值+grace-period+minimum-reclaim+镜像GC独立线85→80/CPU-Manager-static-policy独占cpuset只给Guaranteed整数核/CA改节点组vs-Karpenter-JIT直接bind+缩容PDB/ConfigMap注入三命env快照+volume热更1min+subPath不热更/PriorityClass抢占PostFilter选victims-vs-节点压力驱逐两套/hostNetwork-hostPID-hostIPC系统组件刚需+hostPort-vs-NodePort/PSS三档privileged-baseline-restricted+PSA-enforce-audit-warn迁移/容器重启vs-Pod重建emptyDir存亡分界+Memory-medium吃limit/CFS限流100ms周期throttled比例>10%动limit) file=36-报告/k8s-高级面试快问快答-沉淀.md(Q1-Q36→Q47) sources=22-概念/08-可靠性与运维/node-lifecycle-management+19-故障诊断/04-高级排障/structural-06-security-auth/03-pod-security-troubleshooting+20-最佳实践/08-security/pod-security note=Q43回收Q13下一问(抢占选victims),Q37-Q47跨引用Q2/Q13/Q18/Q24/Q33/Q34/Q35/Q38/Q40

- [2026-10-16] INTERVIEW_QA_APPEND q_added=Q48-Q57 topics=存储PV-PVC专项(PV-PVC双向指针claimRef+volumeName由persistentvolume-controller调和/phase状态机Available-Bound-Released-Failed+Released移除claimRef重上架/回收策略Retain-Delete-Recycle已废弃动态供给默认Delete/四种accessModes RWO节点级非Pod级Multi-Attach仅跨节点/绑定四条件容量+模式+SC+selector且PVC-spec-immutable/动态供给五步CSI-CreateVolume→pvc-uid-PV→claimRef直绑/volumeBindingMode云上必须WaitForFirstConsumer防跨AZ等待期Pending正常/Pending五层排查SC存在→绑定模式→provisioner→匹配→quota/扩容SC-allowVolumeExpansion可后开缩容一律不行/StatefulSet-volumeClaimTemplates改模板不回改存量PVC)+文末深挖静态vs动态供给三自留地与三要素两保险 file=36-报告/k8s-高级面试快问快答-沉淀.md(Q1-Q47→Q57) sources=22-概念/04-存储/pv+persistent-volume-claim note=并行会话产出的卡补记日志

- [2026-10-16] INTERVIEW_QA_APPEND q_added=Q58-Q67 topics=Deployment专项(三层结构Deployment管版本RS管副本数Pod是耗材/maxSurge向上取整maxUnavailable向下取整默认25%+推荐maxUnavailable=0+Recreate有停机窗/progressDeadlineSeconds默认600s只报警不回滚不清理且必须大于minReadySeconds/rollout-undo触发全新滚动产生新revision+revisionHistoryLimit默认10+GitOps下undo会被sync覆盖/minReadySeconds挡站不稳探针挡起不来deadline呼救闭环/pod-template-hash分版本selector-immutable+同namespace重叠selector被拒/RS只数数量不纠内容+缩容择劣顺序未调度→Pending→未Ready→PodDeletionCost高的留→删最新/灰度三板斧pause半路停车+双Deployment按副本比+Argo-Rollouts按weight+Analysis/rollout-restart=restartedAt注解触发空发布受滚动保护+GitOps漂移处理/选型判断线副本能否互换+StatefulSet三机制稳定标识有序性独立盘+partition原生金丝雀) file=36-报告/k8s-高级面试快问快答-沉淀.md(Q1-Q57→Q67) sources=20-最佳实践/06-workloads/deployment+22-概念/05-工作负载/deployment+16-专项技术 note=跨引用Q4/Q14/Q32/Q33/Q34/Q42/Q46/Q57,尾部钩子指向HPA/VPA专题;重叠selector拒载与ProgressDeadlineExceeded语义经web核实

- [2026-10-16] INTERVIEW_QA_APPEND q_added=Q68-Q71 topics=专有云ACK节点池扩容(完整链路Pending→CA→POP内网VIP→ESS期望实例数→伏羲资源池交付ECS→镜像含kubelet-CSI-Terway→join注册→调度,节点池↔伸缩组一一映射+ASOP租户ECS配额≥max前置/专有云vs公有云五差异私有物理池+POP内网无公网+交付分钟级+InsufficientCapacity常态+天目替代CloudMonitor/扩容失败四层切法资源不足查配额伏羲水位+心跳超时查aliyun-service+镜像→伸缩组不一致查ProviderID映射→报警不触发查天目Agent,判断线伸缩活动发起过没=触发层vs资源层/运维纪律六条配额前置+镜像标准化+PDB+缩容宜慢冷却300s-OldestInstance-实例保护+天目三联大盘伸缩活动/节点数/PendingPod+定期演练Pending→弹节点→调度计时) file=36-报告/k8s-高级面试快问快答-沉淀.md(Q1-Q67→Q71) sources=18-云厂商/01-阿里云/专有云-ACK/002-apsara-stack-ess-scaling+004-apsara-stack-pop-operations+005-apsara-tianji-aso-operations+alicloud-apsara-ack-overview+01-专有云架构概述+公有云-ACK/07-ack-node-pool-management note=跨引用Q29/Q38/Q41,尾部钩子指向升级补丁管理(006)

- [2026-10-20] INTERVIEW_QA_APPEND q_added=Q73 topics=Calico-vs-Terway选型(五维:出身绑定VPC-ENI-OpenAPI vs 云厂商无关/数据面Terway-ENIIP无封装90-95% vs Calico-BGP-IPIP-VXLAN云上BGP不透传跨节点黑洞默认Overlay损耗10-30%/本质区别IP在VPC平面vs自管Overlay平面/策略L3L4+Calico扩展GlobalNetworkPolicy+DNS+WireGuard加密/云集成SLB直通Pod级安全组Trunking vs 全自搭)+性能排序+选型判断线纯ACK选Terway多云自建选Calico file=36-报告/k8s-高级面试快问快答-沉淀.md(Q1-Q72→Q73,并行会话已推进至Q72按尾号续编) sources=05-网络/06-Terway/01-product+05-网络/01-K8s网络核心/04-cni-plugins-comparison

- [2026-10-20] INTERVIEW_QA_APPEND q_added=Q74-Q77 topics=kubepods总闸OOM专题(源于真实工单:单节点十几个Burstable pod凌晨3点OOM60+次,根因=无limit pod仍被杀)(Q74四层天花板容器limit→pod级cgroup全有limit才设→kubepods聚合默认必设=allocatable即总闸→节点物理内存+enforceNodeAllocatable=pods默认开+内核先reclaim后cgroup-OOM+v1/v2查询路径/Q75挑人规则oom_score_adj公式BestEffort1000-Burstable按request占比-Guaranteed-997+与kubelet驱逐的区分Evicted-vs-OOMKilled系统预留空着MemAvailable高于eviction-hard所以内核先动手+v2-memory.oom.group整pod团灭kill计数虚高+request是保命符limit是误杀器/Q76案例五步dmesg看constraint→确认总闸→memory.stat拆anon-file→时间轴找定时任务诱因→被杀名单验证分数规律+60次=杀释放重启重建再杀循环/Q77治理三板斧业务pod关笼子limit3-4x-request校准+系统组件CSI-CNI穿盔甲limit留空-request调大-PriorityClass+诱因拆弹错峰限流污点隔离+监控kubepods水位85%告警) file=36-报告/k8s-高级面试快问快答-沉淀.md(Q1-Q73→Q77,并行会话再抢Q73后按尾号续编) sources=对话内真实工单复盘+01-集群基础/03-控制平面/15-kubelet-deep-dive+26-技能/03-节点/node/运维操作/kubelet-eviction-mechanism note=跨引用Q2/Q35/Q40/Q47,四层账本=Q2三层账本的扩展版

- [2026-10-20] INTERVIEW_QA_NEW_FILE q_added=Q1-Q24 topics=安全专项基础+进阶双批(基础Q1-Q12:三道门认证401→鉴权403→准入只管写mutating先于validating+system-masters/RBAC四对象RoleBinding可引ClusterRole+escalate防提权+KCM自动还原内置角色/SA-bound-token-1.24分界1hTTL+80%或24h轮换+绑Pod+automount关/Secret静态加密KMS-v2-1.29GA+identity放末尾+开后必须重写存量+etcdctl验k8s:enc前缀/逃逸五路privileged挂盘+containerd.sock+CAP_SYS_ADMIN-release_agent+hostPID+runc-5736/securityContext六件套默认14cap含NET_RAW+readOnlyRootFilesystem防持久化/NetworkPolicy-denial白名单无deny规则+Flannel静默不生效+egress必放DNS53+peer同项AND拆项OR/供应链三闸Trivy+cosign-Kyverno+digest固定+base补丁不自动进已有镜像/准入webhook failurePolicy默认Ignore但安全必须Fail+自拦截死锁/audit默认不开送出集群+secrets只Metadata+events1h≠证据/kubelet端口10250exec=RCE级匿名+AlwaysAllow=沦陷+10255关+护栏在安全组/多租户namespace软隔离只防误不防恶+vCluster+验收深挖四层预防检测基线追溯kube-bench共享责任)(进阶Q13-Q24:PKI三套CA cluster+etcd独立+front-proxy+SAN清单+metrics-server-x509八成front-proxy/Node-authorizer+NodeRestriction kubelet权限限定本节点+Node必须在RBAC前+证书被偷≠集群沦陷/身份治理OIDC组映射RBAC+客户端证书无吊销=泄漏等过期或换CA+exec凭证插件+CI走ACK-RRSA无AK+impersonation透传/VAP-CEL-1.30GA validationActions-Deny-Warn-Audit并存无failurePolicy两难+webhook仍守mutate外呼+MutatingAdmissionPolicy-1.32alpha/沙箱gVisor用户态内核vs-Kata每Pod轻量VM+RuntimeClass+场景不可信代码+沙箱不免PSS/GitOps密管三档SealedSecrets+SOPS+ESO主力Git无密文+明文进过Git=已泄露+ESO配RRSA/零信任netpol三盲区明文+IP信任+无L7→mTLS+SPIFFE-ID派生SA+PERMISSIVE→STRICT+纵深非替代/AlwaysPullImages防缓存投毒+与digest固定组合拳+代价registry硬依赖→mirror/distroless落盘面CVE面意外面三笔账+debug走ephemeral+alpine≠安全/挖矿应急六步围而不杀→取证→定路径→斩凭证→清剿含CronJob-DaemonSet-webhook驻留位→复盘/runc内核CVE应急四步定性含可利用依据→jsonpath版本盘点→临时缓解先行→滚动升级控制面也要升/收口卡接手集群10分钟摸底十动作暴露面→权限面→数据面→可追溯→版本面) file=36-报告/k8s-安全面试快问快答-沉淀.md(新建,Q1-Q24本地编号) sources=kubernetes官方机制+交叉引用高级沉淀Q20/Q25/Q30/Q42/Q44/Q45+基础加固Q1 note=主沉淀文件正被并行会话连续追加故独立建文件避免编号竞态,跨文件引用用"高级沉淀Qxx"前缀;本条同时补欠该文件创建时的journal记录;文件头日期已修正为2026-10-20

- [2026-10-20] INTERVIEW_QA_APPEND q_added=Q78 topics=ACKOne选型OCM(前提纠正"整体选OCM"是伪命题只发生在纳管层+Fleet四层各选其长:纳管OCM-ManagedCluster注册表+klusterlet出站反连+ManifestWork下发+addon/调度Karmada托管变体policy.one.alibabacloud.com/交付ArgoCD/数据面MSE-ALB网关+多集群Service;五条选型逻辑:同源发起2021-11-RedHat+蚂蚁+阿里云联合开源OCM=ACM上游+职责匹配薄底座注册证书通道addon无差异化价值+解耦实证分发引擎KubeVela-Application→Karmada语义切换纳管层未动+ACM上游生产背书+标准OCM语义开放不锁定;备选反事实:Karmada全栈=CNCF口径互补定位/自研通道=Anthos-GKEHub+Azure-Arc路线丢开放性/Clusternet竞对主导/KubeFed停滞/Rancher-ACM商业竞品无副本调度/KubeAdmiral阿里系第三路线未入主线;CNCF成熟度反差=OCM-2021-11-09-Sandbox至今vs-Karmada-已Graduated,答法徽章≠成熟度纳管层看ACM生产背书+发起方兜底调度层要社区活力;云厂商对照自研是主流Anthos-Arc-EKSConnector开源同源才是阿里差异;边界金句Placement只选集群不切副本;红线不说ACK-One基于Karmada全称命题+OCM≠ACM+开源Karmada-YAML不能原样搬Fleet) file=36-报告/k8s-高级面试快问快答-沉淀.md(Q1-Q77→Q78) sources=18-云厂商/01-阿里云/分布式云-ACKOne/01-ack-one-multi-cluster-architecture+03-ack-one-ocm-selection-rationale+cncf.io/projects/open-cluster-management+cncf.io/blog/2022/09/26 note=同批新建KB文档03-ack-one-ocm-selection-rationale.md(分层选型/OCM架构边界/五条逻辑/备选反事实/成熟度反差/云厂商对照/决策树/面试口径)并更新ACKOne-index.md;编号竞态:并行会话已推进Q73-Calico/Q74-Q77-OOM,本卡锚定Q77尾行续编Q78;下一问钩子兑现Q72注册集群云资源所有权悬案

- [2026-10-20] INTERVIEW_QA_NEW_FILE q_added=Q1-Q10 topics=容器技术本体专项(①容器vsVM共享内核namespace+cgroup进程隔离vs-hypervisor独立内核+沙箱Kata-gVisor中间态/②namespace八种pid-net-mnt-uts-ipc-user-cgroup-time+user是rootless基石+cgroup-v1每子系统一棵树vs-v2统一单树叶子挂进程PSI+K8s1.25GA/③OverlayFS-lowerdir只读-upperdir可写-copy-up整文件+whiteout字符设备0:0遮蔽→容器rm不省磁盘+overlay2内核4.0原生约128层/④docker-run链路CLI→docker.sock→dockerd→containerd→shim-runc-v2→runc建ns-cgroup-rootfs-exec后立即退出+shim常驻stdio-exitcode-生命周期解耦dockerd重启容器不死+1.24删dockershim/⑤exec=setns(/proc/<pid>/ns/*先user后其他+pid-ns需fork生效)+进程父是shim与PID1平级+nsenter救命/⑥PID1未注册handler信号被内核丢弃→stop等满10s+收养孤儿须wait否则僵尸→exec直启或tini--init+shareProcessNamespace后pause收尸/⑦CFS-quota每100ms窗口用满throttle→多线程集体挂起P99毛刺+96核GOMAXPROCS烧0.2核quota推演+automaxprocs/Go1.25容器感知+cpu.stat-nr_throttled第一证据/⑧内存三层账本进程RSS-容器memory.current-节点MemAvailable+page-cache计入limit纯读写大文件也OOM+memory.events-oom_kill告警+137-vs-Evicted两层/⑨procfs整机全局视图free-nproc-JVM堆=物理1/4撞4G-limit必死→UseContainerSupport-8u191默认开+MaxRAMPercentage+lxcfs-FUSE回写+Go-automaxprocs/⑩瘦身五板斧多阶段-distroless-合并RUN同层清缓存-.dockerignore-依赖清单前置保缓存+registry层删不掉改digest+GC+BuildKit缓存挂载) file=36-报告/容器技术专家面试快问快答-沉淀.md(新建,独立编号Q1-Q10,避开主文件Q78编号竞态,跨文件引用用K8s·Qxx前缀) sources=14-容器运行时/00-总览/03-container-internals-deep-dive(同批新建KB底料总纲:隔离/存储/链路/资源/镜像五章+延伸追问题库)+交叉引用主沉淀Q2/Q15/Q18/Q33/Q34/Q40+沙箱选型/CRI内幕/OCI对比/镜像层优化 note=同批两文件互链;延伸候选rootless/multi-arch/veth/seccomp/存储驱动/CRI-O/eBPF已写进文件尾待下批沉淀;main file同日被并行会话推进到Q78故独立建文件

- [2026-10-20] INTERVIEW_QA_APPEND+KB_NOTE q_added=Q79-Q86 topics=PV/PVC进阶拓展(基础Q48-Q57的深化,同批新建KB笔记22-概念/04-存储/03-pv-pvc-lifecycle-deep-dive.md供给·绑定·生命周期全链路深挖:静态vs动态供给(local-PV三自留地+动态三要素两保险)+绑定细节(双向指针claimRef-volumeName+PVC-spec-immutable+storageClassName空串匹配语义表+pvc-protection/pv-protection-finalizer+Released三种复活方式)/CSI挂载三道门Attach(AD-Controller+VolumeAttachment+ControllerPublishVolume)→Stage(kubelet-NodeStageVolume-globalmount)→Publish(bind-mount)+FailedAttachVolume-vs-FailedMount对号/卷拓扑CSINode注册topologyKeys+WaitForFirstConsumer底层+CSIStorageCapacity-1.24GA/在线扩容链ControllerExpandVolume→NodeExpandVolume+FileSystemResizePending离线分支+缩容一律不行/VolumeSnapshot三件套+dataSource恢复+crash-consistent口径+快照≠备份/删除回收全链路pvc-protection等Pod停用→reclaimPolicy分叉Retain-Released黄金窗口-Delete走CSI-DeleteVolume+卡Terminating三层/RWOP版本线1.22-alpha-1.27-beta默认-1.29-GA(web-verified-KEP-2486,修正Q51原写1.27-stable的错误)/Generic-Ephemeral-Volume随Pod生死PVC+vs-emptyDir-vs-CSI-inline-ephemeral辨析/fsGroup递归chgrp+fsGroupChangePolicy-OnRootMismatch+NFS-root-squash服务端问题/监控四类容量bytes+inodes双口径-对象状态-供给链路-云盘限速水位) file=36-报告/k8s-高级面试快问快答-沉淀.md(Q1-Q78→Q86,并行会话已推进Q59-Q78-Deployment+专有云+OOM+ACKOne,追加前重查尾号锚定Q78尾行) sources=22-概念/04-存储/pv+persistent-volume-claim+01-volume-attachment-controller+02-csi-storage-capacity-tracking+kubernetes.io/blog/2023/12/18/read-write-once-pod-access-mode-ga note=KB笔记03-pv-pvc-lifecycle-deep-dive.md已登记进04-存储index.md并与pv/pvc/storageclass/01-VolumeAttachment/02-CSIStorageCapacity互链;RWOP版本修正已回写Q51

- [2026-10-20] INTERVIEW_QA_APPEND q_added=Q87 topics=finalizer与并发写陷阱题(方向纠偏patch基准=服务端最新对象不碰finalizers动不了别人的锁+无RV整体PUT才是主凶基准=手里过期副本/删除两阶段链路LB-Service:CCM打load-balancer-cleanup→deletionTimestamp→清云SLB/EIP→摘锁→真删+存储层判定=任何写操作让finalizers变空即触发硬删=事故机关/四种patch对finalizers行为矩阵:strategic=并集patchStrategy-merge加得进删不掉+merge-RFC7386=整体替换null清空即解卡命令原理+json=精确op配test防索引竞态+SSA=所有权认领后漏带静默移除/嫌疑人排序:自研operator两反面教材(凭空构造对象Update+对账把陌生finalizer当漂移清掉)>kubectl-replace-apply-force>多集群搬运RV天然对不上>平台层label回写+官方KCM/CCM走informer+RV+RetryOnConflict是守护者/正确姿势:finalizer增删用AddRemoveFinalizer+Update+RetryOnConflict+改字段最小patch不带finalizers+SSA不认领+摘锁前逐项确认外部资源已清) file=36-报告/k8s-高级面试快问快答-沉淀.md(Q1-Q86→Q87,并行会话抢注Q79-Q86存储专项后按尾号续编) sources=对话讨论+22-概念/01-核心架构/04-finalizer-deletion-lifecycle(本批新建) note=姊妹卡Q80-pvc-protection互链;KB新建04-finalizer-deletion-lifecycle.md并更新01-核心架构index.md+README篇数(概念306→307/提炼层1381→1382);Edit工具两次被并行写入打断,改用尾部校验+heredoc追加原子模式成功

- [2026-10-20] INTERVIEW_QA_APPEND+KB_NOTE q_added=Q88-Q93 topics=集群组件·扩展与治理层(apiserver六道门链路总纲:authn→authz→mutating→schema→validating→etcd+401/403/422分界/CRD vs 聚合apiserver选型+conversion-webhook+structural-schema/准入webhook四种搞挂姿势:failurePolicy-Fail+timeout10s+范围失控+sideEffects/CCM四控制器cloud-node·node-lifecycle·route·service+无CCM三症状/metrics-server四跳链路cAdvisor→Summary→60s聚合→metrics.k8s.io+与Prometheus分工/kubelet TLS bootstrap五步token→CSR→approver→signer→rotate) kb_note=01-集群基础/03-控制平面/42-cluster-extension-and-governance-deep-dive.md(new,含六道门链路图+全景串联图) index=03-控制平面index补42+漏链24/41 warning=沉淀文件多会话竞写:本会话观察到Q88弹性金字塔卡(HPA/VPA/CA/KEDA)出现后被另一会话整文件重写覆盖丢失,属主会话需补回;本会话基于真实尾部Q87顺延Q88-Q93,uniq校验无重复,占位符+sed动态编号+>>原子追加避开工具状态竞态

- [2026-10-20] INTERVIEW_QA_APPEND q_added=Q94-Q103 topics=HPA-VPA弹性伸缩专项(扩缩公式ceil+Utilization分母requests非limit+无requests不参与计算/依赖链cAdvisor→metrics-server→HPA-controller+Conditions四层排查/防抖三参数sync15s+tolerance10%+downscale稳定窗口300s扩0s不对称+behavior逐向policies/手动scale与HPA冲突听HPA+GitOps对spec.replicas做ignoreDifferences/CFS限流互骗回收Q47限流压平usage致HPA钝感+自定义指标规避/VPA三件套Recommender算账Updater动手Admission-webhook只拦CREATE故存量必须驱逐+updateMode五档Off-Initial-Recreate-InPlaceOrRecreate-依赖K8s1.33原地resize+Auto=Recreate别名/OOMKilled计入推荐加成/DaemonSet不适用addon-resizer替代+PDB硬前置+minAllowed-maxAllowed夹范围uncappedTarget对比/HPA×VPA冲突机理Utilization分母被VPA改requests联动震荡+两共存方案分维度自定义指标与分资源CPU归HPA内存归VPA/KEDA补零ScaledObject生成托管HPA+60触发器+冷启动成本/四层金字塔HPA管几个-VPA管多大-CA管多少台-KEDA管要不要+Pending信号咬合Q41) file=36-报告/k8s-高级面试快问快答-沉淀.md(Q1-Q93→Q103) sources=22-概念/07-调度与资源/{horizontal-pod-autoscaler,autoscaling-strategies,hpa-vpa-autoscaling-deep-dive}+17-系统基础/06-知识字典/workloads/vertical-pod-autoscaling note=同批新增KB深度笔记hpa-vpa-autoscaling-deep-dive.md(四层金字塔/防抖参数表/限流互骗回路/updateMode全谱/共存机理/决策树/落地清单)并入index互链;Q68-Q93为并行会话占用(专有云节点池/SOFA-CCM/Calico/OOM系列/ACKOne-OCM/存储CSI-finalizer/控制面链路-API扩展-TLSBootstrap),本会话经三次竞态重编号Q68→Q79→Q88→Q94,采用python3降序替换+守卫检查一条命令原子追加(BSD-sed多字节bad-flag曾致整组表达式未生效,错号块已精确切除重做)

- [2026-10-20] INTERVIEW_QA_APPEND q_added=Q104 topics=K8s运维Agent判断层设计(判断模型Jev/System-One-Model:TypeSafe-2026-09首个"输出判断不生成文字"模型+三原语Choice标签置信概率/Score有序量尺期望分可落档间/Noul是非概率+systemOne一次多问并行隔离免context-rot毫秒级+两种挂法主模型当工具调vs-Harness工具前后自动问主模型无感+K8s六判断点事件分诊Choice/告警分级Score-P3-P0/危险命令门控Noul+Choice范围/变更验收Noul+Score回滚/工具路由/值班打断+置信门控三段高自动中降级dry-run低升级给人附概率+原子问题拆解节点健康四Noul代码加权+铁律Jev回答问题不执行决定阈值副作用在代码+能否自训:Jev本体闭箱不私有化不微调全客户共享权重定制只靠state/questions故K8s版Jev=写模板¥0训练1万次/天≈¥3-6,自训只能训同范式替代difflogic可微分逻辑门网络"与非门模型"特征向量输入类别钉死CPU微秒Mac可训算力¥0-20真实成本标注,专有云/离线/数据不出域唯一选项+判断线五条件文本输入动作可枚举秒级高频允许置信分流) file=36-报告/k8s-高级面试快问快答-沉淀.md(Q1-Q103→Q104,并行会话同时段推进Q79-Q103存储CSI/API机制/弹性,本会话守卫检测两次撞号后动态取尾号+1落Q104) sources=15-AI基础设施/02-AI-Agents/53-jev-judgment-model-system-one-k8s-ops+github.com/cholf5/random/issues/52(对照docs.typesafe.ai与SDK-v0.6.0)+sohu深度拆解(四层栈/不可定制/定价$0.042/M)+difflogic(Petersen-NeurIPS2022/2024,arXiv-2607.09399) note=同批新建KB文档53-jev-judgment-model-system-one-k8s-ops.md(输出空间辨析/三原语API契约+K8s风格curl与SDK示例/两挂法kubectl门控mermaid时序/四原因四层栈/六判断点模板+置信门控三段+state裁剪/三路线成本模型与决策树/冷静清单红线)并更新02-AI-Agents/index.md;用户原话"与非门判断型model"=Jev,官方未确认内部是否逻辑门网络,文中标注为社区关联说法;定价两源矛盾(科普文称无公开定价页/拆解文给$0.042)均如实标注以console账单为准;下一问钩子指向告警降噪/值班打断量化

- [2026-10-20] INTERVIEW_QA_APPEND+KB_NOTE q_added=Q105 topics=TAM银行大项目服务方法论(职业软技能卡,主文件首张:三层框架/原则五铁律=稳定压倒一切变更三问可灰度可回退有预案+NoSurprise坏消息带选项第一时间报+承诺即负债口头承诺当天纪要留痕+合规红线高于商务+长期主义/方法论六模块=干系人地图分层沟通向上价值向下细节对同级厂商讲责任界面+关键人轮岗靠机制传承+全生命周期含割接三件套演练≥2回退条件前置写死战时值守表+故障黄金24h止血定性RCA对上模板六要素改进闭环+两周信任修复窗口+例行运营周报月报QBR高管拜访敌人是存在感归零+指挥家式资源编排只做判断协调兜底+价值翻译99.99%=53min给对接人汇报素材+期望管理说no三件套+赋能转移/哲学五条=敬畏生产监管+一致性信任+敢于说不+翻译官传教士+赋能非替代/银行四特殊=监管刚性等保密评监管报送+稳定KPI一票否决+组织复杂轮岗常态+多厂商责任界面/权力来源=信任+信息两种货币/场景playbook=30-60-90接手+黄金24h+QBR三段式+续约提前6月+八反模式) file=36-报告/k8s-高级面试快问快答-沉淀.md(Q1-Q104→Q105,python原子追加:动态取尾号+1+碰撞守卫+验重无重号) sources=对话讨论+13-生产运维/09-TAM服务方法论/01-tam-banking-large-project-methodology(本批新建,目录index+01主笔记:角色定位与SA交付PM-SLA边界/银行四特殊/五铁律/六模块/场景playbook/反模式八条/面试口径映射,已挂13-生产运维index子目录+文档,互链03-事件响应war-room-on-call-escalation+07-运维手册sla-slo+05-工单案例) note=本轮观察:读盘曾短暂见Q87尾态,随后Q88-Q104(集群组件/HPA-VPA/运维Agent-Jev判断层)完整复原——并行会话整文件覆写竞争仍在,追加一律python原子模式;技术侧故障对照Q74-Q77
- [2026-10-22] INTERVIEW_QA_APPEND+KB_NOTE q_added=Q115 topics=双CCM抢写定位与收敛(信号六类:ingress-IP抖/LB后端高频增删/zone标签与uninitialized-taint反复/删除卡Terminating-finalizer拉锯/事件风暴/最高优先级=节点莫名被删=第二CCM-cloud-node-lifecycle查不到实例判不存在删Node正当逻辑成误删机关;定位四步:loadBalancerClass+注解归属→ActionTrail两RAM身份谁改监听后端→双控制器日志时间线对齐→RBAC盘点nodes-delete/services-patch;收敛三步:定唯一写者→loadBalancerClass划域(KEP-1959-1.24GA不可变只能重建切换前双跑)+非主方RBAC收到只读+自有前缀注解写→admission固化无class一律归ACK-CCM;ACK存量样板alibaba-cloud-loadbalancer-id+force-override-listeners默认false) file=36-报告/k8s-高级面试快问快答-沉淀.md(Q1-Q114→Q115,python动态尾号+1原子追加:首轮守卫撞Q106-再查已到Q109-落卡时并行会话已推进到Q114,三次竞态全靠实时max+1躲过) sources=对话讨论+kubernetes.io/docs/concepts/architecture/cloud-controller+KEP-1959+help.aliyun.com/ack cloud-controller-manager note=同批新建KB深度笔记01-集群基础/03-控制平面/43-dual-cloud-controller-coexistence-deep-dive.md(单实例语义/四类冲突面含节点误删机关/分域共治四机制/mermaid分域图/SOFA侧实质形态Endpoint同步器+BMCP节点接入器/排查收敛手册/先例边界/红线)并在14-cloud-controller-manager-deep-dive.md的Related加反向链接;Q72(SOFA-CCM架构卡,本会话前轮)核实完好唯一;03-控制平面index.md未手改(自动生成);下一问钩子=LB所有权迁移(存量SLB的CCM托管⇄自管切换)
- [2026-10-22] INTERVIEW_QA_APPEND_R2 q_added=Q126-Q127 topics=apiserver按段定界(etcd/webhook/flowcontrol三把尺子+audit-stage时间戳归因+pprof)+APF座位模型(600座400+200/FlowSchema归级/借座/429语义/exempt风险) dropped=2(rv三语义/磁盘证据链等与并行批Q117-Q119撞题撤并,KB笔记保留全量内容) kb_note=22-概念/01-核心架构/05-apiserver-latency-triage-deep-dive.md(新建:etcd第一嫌疑人三件证据+按段定界+五故障模式+PromQL速查+一页速记) backlinks=3(etcd-operational-reference+watch-mechanism+19-故障诊断/01-核心排障/01 apiserver排查SeeAlso) patch=Q10弹药补--etcd-compaction-interval口径(对齐Q118) race=追加时Q106被并行会话抢占两次(105→125),守卫拒绝后按实时尾号重编号,撞题卡撤并
- [2026-10-22] INTERVIEW_QA_APPEND+KB_NOTE q_added=Q116-Q125 topics=etcd面试深挖十连(quorum交集定理/奇偶3v5跨Region+PreVote-3.5默认开/写路径五步WAL-fsync+bbolt批量两段延迟+fio验盘口径/compact执行者=apiserver-5m-compactor+compact≠defrag+碎片判断total-vs-in_use/watch-cache+too-old-410+RV三语义+ConsistentListFromCache-1.31β默认开/Lease两层=K8s对象kube-node-lease心跳renewTime宽限期40s→1.32起50s+选主15-10-2 vs etcd原生TTL=Events-event-ttl-1h/线性读ReadIndex四步vs串行/恢复实务=备份全自动恢复半手动runbook六步+revision回退+bump-revision-mark-compacted/单节点换成员learner+promote/stacked-vs-external+跨AZ拓扑) file=36-报告/k8s-高级面试快问快答-沉淀.md(Q1-Q105→Q125) sources=22-概念/01-核心架构/etcd-quorum-lease-restore-deep-dive(本批新建)+01-集群基础/03-控制平面/11-etcd-deep-dive+23-实体/02-K8s核心组件/etcd+22-概念/11-交叉分析/etcd-×-节点诊断+10-平台工程/02-运维/15-lease-leader-election note=web核实三点:node-monitor-grace-period默认40s→1.32起50s(k8s.io-KCM参考);ConsistentListFromCache=1.30α/1.31β默认开(k8s.io-blog-2024-08/KEP-2340);snapshot-restore --bump-revision/--mark-compacted=3.5.x起etcdctl·3.6起etcdutl(CHANGELOG-3.5/3.6);本批避开已有Q1(3挂2)/Q10(NOSPACE)角度;并行会话同时段插入Q106-Q115(ACK生命周期+多集群),本批python守卫读实时尾号Q115→批内重编号Q116-Q125落盘+笔记内引用同步+10;反向wiki-link回填6处(etcd实体/deep-dive/operational-reference/watch-mechanism/lease-leader-election/etcd-×-节点诊断);验重无重号

- [2026-10-20] INTERVIEW_QA_APPEND+KB_NOTE q_added=Q106-Q108 topics=ACK集群新建+升级生命周期(新建七阶段:RAM服务关联角色/形态五选托管Pro为主+专有版停演进/版本选默认较新minor/网络规划CNI+三网段一次定死/组件Ingress-CSI-SLS/节点池可仅建管控面/就绪链路ESS→cloud-init→TLSBootstrap注册→addons;四不可改=CNI Terway↔Flannel+所属VPC+ServiceCIDR+TerwayIPvlan,可追加Pod vSwitch;升级六步:定目标只升允许版本/precheck废弃API(PSP 1.25经典事故)+组件与版本解耦可单独先升+PDB+Failclosed webhook/备份etcd+备份中心/容量缓冲=并行节点×承载/控制面先升存量Pod无感+节点池分批cordon→drain→替换系统盘or原地升级→重入/验证+IaC回写消漂移;单向门机理=etcd storage version只进不退,快照恢复=灾难恢复非回滚,前向修复;排障三分:排不出去(PDB ALLOWED=0自锁单副本minAvailable=1/pods-eviction Failclosed webhook/emptydir/裸Pod)vs落不下来(容量/Terway IP/亲和污点)vs新节点注册失败,先暂停任务再处置,禁drain --force一把梭;节点池滚动vs Deployment滚动对比=滚动单位/PDB对readiness/回滚无) file=36-报告/k8s-高级面试快问快答-沉淀.md(Q1-Q105→Q108,python原子追加:动态尾号+1+同题守卫+重号复核;上轮对话曾按67尾态预期Q68/Q69,本轮实时重查发现Q68-Q70已被并行会话占用,实际落Q106-Q108——再次验证追加前必须实时重查尾号) sources=18-云厂商/01-阿里云/公有云-ACK/{01-alicloud-ack-production-runbook,02-ACK集群运维,07-ack-node-pool-management,03-Terway-CNI网络} note=同批新建KB深度笔记08-ack-cluster-lifecycle-create-upgrade.md(不可改清单/七阶段mermaid/网络规划表/precheck九项表/两阶段执行/排障表/滚动对比表/单向门机理/命令速查);反链已挂07节点池(新增相关文档节)+03-Terway相关文档列表+01-runbook本域资料;index自动生成未手改

- [2026-10-22] INTERVIEW_QA_APPEND+KB_NOTE q_added=Q109-Q114 topics=多集群架构专项(总纲Q109三模式=治理/调度/通信三抽象层+判别法"同步什么/方向/流量过不过"+N-S多集群网关第四块拼图; 故障域Q110三问法"谁物化资源/谁推配置/谁有到期物"+OCM-pull出站反连自洽+Karmada挂=容灾能力离线(失联阈值/驱逐定时器全停)+istiod工作负载证书24h定时炸弹+恢复后对账冲回应急手改; 服务发现Q111 MCS-API=ServiceExport显式导出/ServiceImport控制器派生/clusterset.local DNS/ClusterSet+namespace-sameness/2026仍v1alpha1未内置/实现矩阵Submariner-Lighthouse最早+Cilium≥1.17β与GlobalServices注解并存+Istio不走MCS自有registry合并+GKE-MCS+ACKOne多集群Service; E-W路线Q112四条=ClusterMesh直连(Pod-CIDR不重叠+cluster.name/id唯一1-255)/Submariner隧道+Globalnet-NAT兜底重叠/Istio东西向网关(1.8废弃.global改DNS-proxying+同名合并;multi-primary vs primary-remote×同网/跨网)/云产品网关(ACKOne不做Pod直连不要求全局CIDR规划); 取舍Q113拆集群四触发=爆炸半径/合规域/升级独立/信任边界而非5000节点上限(硬顶数字5000节点15万Pod单节点110+etcd≤8GB)+反模式四件(无模板/随意开/网段不登记/独立账号)+成熟度阶梯别跳级; 容灾Q114三段链路探测/重调度/流量切换+RTO=探测+决策+冷启动+流量收敛而RPO在数据层+Karmada两级failover(cluster级taint+failover-eviction-timeout默认约5m/应用级purgeMode v1.17口径=Directly|Graciously|Never旧版Immediately+失联集群Directly清理不保证执行→新旧双跑窗口,有状态靠fencing/租约)+同城双活RPO≈0 vs 两地三中心RPO>0) file=36-报告/k8s-高级面试快问快答-沉淀.md(Q105→Q109-Q114,并行会话同时段抢Q106-Q108=ACK集群生命周期专项,python动态尾号+1+守卫一次通过;前置web核实三处版本敏感事实:MCS-API仍v1alpha1无beta/Cilium1.17才引入MCS-APIβ/Karmada purgeMode改名Immediately→Directly且Graciously未被改,Q114追加后python精确修补) sources=28-资产/architecture-diagrams/architecture-multi-cluster+18-云厂商/01-阿里云/分布式云-ACKOne/{01,02,03}+20-最佳实践/09-large-scale-production/10-multi-cluster+github.com/kubernetes/enhancements KEP-1645+docs.cilium.io clustermesh/mcsapi+karmada.io application-failover note=同批新建KB深挖笔记22-概念/01-核心架构/05-multi-cluster-architecture-patterns-deep-dive.md(判别表七维含"控制面挂了丢什么"列/故障域挂表三问法/MCS实现矩阵/E-W四路线对比表/拆集群判断与成熟度阶梯/容灾三段与RTO公式/mermaid选型决策树/追问预判映射Q109-Q114);反链四挂=51-网络联邦Related+ACKOne/01 Related+10-multi-cluster Related+multi-cluster-dr-automation相关概念;架构资产页小修=.global标注1.8废弃+挂深挖链接+last_updated→2026-10-22;下一批钩子=多集群GitOps防漂移(ApplicationSet生成器/Hub单点)
