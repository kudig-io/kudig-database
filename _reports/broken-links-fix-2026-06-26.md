---
title: Broken Wikilinks 修复报告（2026-06-26）
description: 根据 wiki-lint 审计自动修复 broken wikilinks
summary: 根据 wiki-lint 审计自动修复 broken wikilinks
category: reports
tags:
- wiki-lint
- broken-links
- maintenance
tier: supporting
created: '2026-06-26'
updated: '2026-06-26'
last_updated: 2026-06-26
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Broken Wikilinks 修复报告

- 总 broken links: 50
- 成功修复: 50
- 转纯文本: 0
- 失败/跳过: 0

## Fixed Links

| Source | Original | Replacement | Confidence |
|---|---|---|---|
| `log.md` | `_meta/dashboard.md` | `_meta/dashboard.md` | exact |
| `concepts/kubernetes-version-evolution.md` | `[[系统基础/topic-dictionary/workloads/ephemeral-containers.md|ephemeral containers]]` | `系统基础/topic-dictionary/workloads/ephemeral-containers.md` | exact |
| `concepts/pod-lifecycle.md` | `[[系统基础/topic-dictionary/workloads/init-containers.md|init containers]]` | `系统基础/topic-dictionary/workloads/init-containers.md` | exact |
| `concepts/pod-lifecycle.md` | `[[系统基础/topic-dictionary/workloads/sidecar-containers.md|sidecar containers]]` | `系统基础/topic-dictionary/workloads/sidecar-containers.md` | exact |
| `concepts/pod-lifecycle.md` | `_meta/journal/digest-2026-05-21-full.md` | `_meta/journal/digest-2026-05-21-full.md` | exact |
| `concepts/pod-lifecycle.md` | `_reports/WIKI-LINT-REPORT-2026-05-21.md` | `_reports/WIKI-LINT-REPORT-2026-05-21.md` | exact |
| `concepts/linux-sysctl-tuning.md` | `[[concepts/container-runtime.md|container runtime]]` | `container runtime` | exact |
| `concepts/kubernetes-architecture-overview.md` | `[[concepts/container-runtime.md|container runtime]]` | `container runtime` | exact |
| `concepts/block-file-object-storage.md` | `[[entities/csi-drivers.md|csi drivers]]` | `entities/csi-drivers.md` | exact |
| `concepts/overlayfs-storage.md` | `[[concepts/container-runtime.md|container runtime]]` | `container runtime` | exact |
| `concepts/container-runtime-comparison.md` | `[[concepts/container-runtime.md|container runtime]]` | `container runtime` | exact |
| `concepts/operator-pattern.md` | `_meta/journal/digest-2026-05-21.md` | `_meta/journal/digest-2026-05-21.md` | exact |
| `concepts/docker-architecture.md` | `[[concepts/container-runtime.md|container runtime]]` | `container runtime` | exact |
| `concepts/linux-container-foundation.md` | `[[系统基础/topic-dictionary/workloads/pod-hostname.md|pod hostname]]` | `系统基础/topic-dictionary/workloads/pod-hostname.md` | exact |
| `concepts/microservice-resilience-patterns.md` | `[[concepts/service-mesh-architecture.md|service mesh architecture]]` | `[[concepts/service-mesh-architecture.md|service mesh architecture]]` | exact |
| `concepts/Kubernetes Core Concepts.md` | `[[concepts/declarative-api.md|declarative api]]` | `[[concepts/declarative-api.md|declarative api]]` | exact |
| `concepts/storage-model.md` | `[[entities/csi-drivers.md|csi drivers]]` | `entities/csi-drivers.md` | exact |
| `concepts/watch-mechanism.md` | `[[concepts/kubernetes-architecture-overview.md|kubernetes architecture overview]]` | `[[concepts/kubernetes-architecture-overview.md|kubernetes architecture overview]]` | exact |
| `skills/manage-persistent-storage.md` | `[[skills/troubleshoot-pod-issues.md|troubleshoot pod issues]]` | `[[skills/troubleshoot-pod-issues.md|troubleshoot pod issues]]` | exact |
| `skills/skill-k8s-node-notready-USAGE-GUIDE.md` | `[[skills/troubleshoot-pod-issues.md|troubleshoot pod issues]]` | `[[skills/troubleshoot-pod-issues.md|troubleshoot pod issues]]` | exact |
| `skills/troubleshoot-pod-issues.md` | `[[系统基础/topic-dictionary/workloads/ephemeral-containers.md|ephemeral containers]]` | `系统基础/topic-dictionary/workloads/ephemeral-containers.md` | exact |
| `skills/troubleshoot-pod-issues.md` | `[[skills/monitor-kubernetes-metrics.md|monitor kubernetes metrics]]` | `[[skills/monitor-kubernetes-metrics.md|monitor kubernetes metrics]]` | exact |
| `skills/troubleshoot-pod-issues.md` | `[[skills/configure-health-probes.md|configure health probes]]` | `[[skills/configure-health-probes.md|configure health probes]]` | exact |
| `skills/learn-01-what-is-kubernetes.md` | `[[concepts/kubernetes-architecture-overview.md|kubernetes architecture overview]]` | `[[concepts/kubernetes-architecture-overview.md|kubernetes architecture overview]]` | exact |
| `skills/skill-reference-version-matrix.md` | `[[skills/troubleshoot-node-issues.md|troubleshoot node issues]]` | `[[skills/troubleshoot-node-issues.md|troubleshoot node issues]]` | exact |
| `skills/troubleshoot-node-issues.md` | `[[concepts/container-runtime.md|container runtime]]` | `container runtime` | exact |
| `skills/configure-health-probes.md` | `[[skills/troubleshoot-pod-issues.md|troubleshoot pod issues]]` | `[[skills/troubleshoot-pod-issues.md|troubleshoot pod issues]]` | exact |
| `skills/skill-reference-diagnostic-workflow.md` | `[[concepts/container-runtime.md|container runtime]]` | `container runtime` | exact |
| `skills/skill-assets-escalation-template.md` | `[[entities/kudig-prompts-catalog.md|kudig prompts catalog]]` | `[[entities/kudig-prompts-catalog.md|kudig prompts catalog]]` | exact |
| `平台工程/12-automated-operations-toolchain.md` | `_reports/release-notes/22-production-checklist.md` | `_reports/release-notes/22-production-checklist.md` | exact |
| `可靠性/README.md` | `chaos mesh` | `系统基础/topic-dictionary/operations/chaos-mesh.md` | exact |
| `entities/kanister.md` | `_meta/journal/digest-2026-05-21-full.md` | `_meta/journal/digest-2026-05-21-full.md` | exact |
| `entities/armada.md` | `_meta/journal/digest-2026-05-21-full.md` | `_meta/journal/digest-2026-05-21-full.md` | exact |
| `entities/cncf-cicd.md` | `artifact hub` | `系统基础/topic-dictionary/tooling/artifact-hub.md` | exact |
| `entities/argo.md` | `_meta/journal/digest-2026-05-21-full.md` | `_meta/journal/digest-2026-05-21-full.md` | exact |
| `entities/kubernetes.md` | `_meta/journal/digest-2026-05-21-full.md` | `_meta/journal/digest-2026-05-21-full.md` | exact |
| `entities/kubernetes.md` | `_reports/WIKI-LINT-REPORT-2026-05-21.md` | `_reports/WIKI-LINT-REPORT-2026-05-21.md` | exact |
| `entities/kubernetes-changelog.md` | `[[系统基础/topic-dictionary/workloads/ephemeral-containers.md|ephemeral containers]]` | `系统基础/topic-dictionary/workloads/ephemeral-containers.md` | exact |
| `entities/interlink.md` | `virtual kubelet` | `系统基础/topic-dictionary/fundamentals/virtual-kubelet.md` | exact |
| `entities/prometheus.md` | `_reports/WIKI-LINT-REPORT-2026-05-21.md` | `_reports/WIKI-LINT-REPORT-2026-05-21.md` | exact |
| `entities/cncf-orchestration.md` | `operator framework` | `系统基础/topic-dictionary/platform-engineering/operator-framework.md` | exact |
| `entities/envoy.md` | `_meta/journal/digest-2026-05-21-full.md` | `_meta/journal/digest-2026-05-21-full.md` | exact |
| `entities/40-terway-product-overview.md` | `connect rpc` | `系统基础/topic-dictionary/networking/connect-rpc.md` | exact |
| `entities/cncf-infrastructure.md` | `chaos mesh` | `系统基础/topic-dictionary/operations/chaos-mesh.md` | exact |
| `entities/etcd.md` | `_meta/journal/digest-2026-05-21.md` | `_meta/journal/digest-2026-05-21.md` | exact |
| `entities/cni.md` | `_meta/journal/digest-2026-05-21-full.md` | `_meta/journal/digest-2026-05-21-full.md` | exact |
| `entities/slimtoolkit.md` | `connect rpc` | `系统基础/topic-dictionary/networking/connect-rpc.md` | exact |
| `entities/slimtoolkit.md` | `[[entities/oscal-compass.md|oscal compass]]` | `[[entities/oscal-compass.md|oscal compass]]` | exact |
| `entities/oscal-compass.md` | `connect rpc` | `系统基础/topic-dictionary/networking/connect-rpc.md` | exact |
| `entities/linkerd.md` | `connect rpc` | `系统基础/topic-dictionary/networking/connect-rpc.md` | exact |

## Converted to Plain Text

| Source | Original |
|---|---|

## Failed/Skipped

| Source | Original | Reason |
|---|---|---|

<!-- risk-assessed -->
