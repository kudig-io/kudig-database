---
title: 第四轮 Broken Wikilinks 修复报告（2026-06-26）
description: 修复剩余 82 个核心概念 broken wikilinks
summary: 修复剩余 82 个核心概念 broken wikilinks
category: reports
tags:
- wiki-lint
- broken-links
tier: supporting
created: '2026-06-26'
updated: '2026-06-26'
last_updated: 2026-06-26
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 第四轮 Broken Wikilinks 修复报告

- 初始剩余: 69
- 成功修复: 44
- 转纯文本: 19
- 失败: 6
- 修复后剩余: 7

## Fixed

| Source | Original | Replacement | Confidence |
|---|---|---|---|
| `_reports/broken-links-round4-fix-2026-06-26.md` | `[[entities/kudig-prompts-catalog.md|kudig prompts catalog]]` | `[[entities/kudig-prompts-catalog.md|kudig prompts catalog]]` | exact |
| `_reports/broken-links-round4-fix-2026-06-26.md` | `chaos mesh` | `系统基础/topic-dictionary/operations/chaos-mesh.md` | exact |
| `_reports/broken-links-round4-fix-2026-06-26.md` | `artifact hub` | `系统基础/topic-dictionary/tooling/artifact-hub.md` | exact |
| `_reports/broken-links-round4-fix-2026-06-26.md` | `virtual kubelet` | `系统基础/topic-dictionary/fundamentals/virtual-kubelet.md` | exact |
| `_reports/broken-links-round4-fix-2026-06-26.md` | `operator framework` | `系统基础/topic-dictionary/platform-engineering/operator-framework.md` | exact |
| `_reports/broken-links-round4-fix-2026-06-26.md` | `connect rpc` | `系统基础/topic-dictionary/networking/connect-rpc.md` | exact |
| `_reports/broken-links-round4-fix-2026-06-26.md` | `[[entities/oscal-compass.md|oscal compass]]` | `[[entities/oscal-compass.md|oscal compass]]` | exact |
| `_reports/broken-links-round4-fix-2026-06-26.md` | `kubernetes` | `系统基础/topic-dictionary/fundamentals/kubernetes.md` | fuzzy |
| `_reports/broken-links-round4-fix-2026-06-26.md` | `prometheus` | `系统基础/topic-dictionary/observability/prometheus.md` | fuzzy |
| `_reports/broken-links-round4-fix-2026-06-26.md` | `etcd` | `系统基础/topic-dictionary/fundamentals/etcd.md` | fuzzy |
| `_reports/broken-links-round4-fix-2026-06-26.md` | `service` | `系统基础/topic-dictionary/networking/service.md` | fuzzy |
| `_reports/broken-links-round4-fix-2026-06-26.md` | `kubelet` | `系统基础/topic-dictionary/fundamentals/kubelet.md` | fuzzy |
| `_reports/broken-links-round4-fix-2026-06-26.md` | `gitops cicd index` | `生态参考/topic-index/gitops-cicd-index.md` | fuzzy |
| `_reports/broken-links-round4-fix-2026-06-26.md` | `helm` | `系统基础/topic-dictionary/tooling/helm.md` | fuzzy |
| `_reports/broken-links-round4-fix-2026-06-26.md` | `ingress` | `系统基础/topic-dictionary/networking/ingress.md` | fuzzy |
| `_reports/broken-links-round4-fix-2026-06-26.md` | `argocd` | `entities/argocd.md` | fuzzy |
| `_reports/broken-links-round4-fix-2026-06-26.md` | `README` | `生态参考/topic-release-notes/README.md` | fuzzy |
| `_reports/broken-links-round4-fix-2026-06-26.md` | `cilium` | `系统基础/topic-dictionary/networking/cilium.md` | fuzzy |
| `_reports/broken-links-round4-fix-2026-06-26.md` | `istio` | `系统基础/topic-dictionary/networking/istio.md` | fuzzy |
| `_reports/broken-links-round4-fix-2026-06-26.md` | `pods` | `系统基础/topic-dictionary/workloads/pods.md` | fuzzy |
| `_reports/broken-links-round4-fix-2026-06-26.md` | `[[CONTRIBUTING.md]]` | `[[CONTRIBUTING.md]]` | fuzzy |
| `_reports/broken-links-round4-fix-2026-06-26.md` | `hot` | `_meta/journal/hot.md` | fuzzy |
| `_reports/broken-links-round4-fix-2026-06-26.md` | `README` | `生态参考/topic-release-notes/README.md` | fuzzy |
| `_reports/broken-links-round4-fix-2026-06-26.md` | `log` | `_meta/journal/log.md` | fuzzy |
| `_reports/broken-links-round4-fix-2026-06-26.md` | `notary project` | `系统基础/topic-dictionary/security/notary-project.md` | exact |
| `_reports/broken-links-round4-fix-2026-06-26.md` | `cloud custodian` | `系统基础/topic-dictionary/operations/cloud-custodian.md` | exact |
| `_reports/broken-links-round4-fix-2026-06-26.md` | `piraeus datastore` | `系统基础/topic-dictionary/storage/piraeus-datastore.md` | exact |
| `_reports/broken-links-round4-fix-2026-06-26.md` | `[[entities/serverless-devs.md|serverless devs]]` | `[[entities/serverless-devs.md|serverless devs]]` | exact |
| `_reports/broken-links-round4-fix-2026-06-26.md` | `oauth2 proxy` | `系统基础/topic-dictionary/security/oauth2-proxy.md` | exact |
| `_reports/broken-links-round4-fix-2026-06-26.md` | `pod lifecycle` | `系统基础/topic-dictionary/workloads/pod-lifecycle.md` | exact |
| `_reports/broken-links-round4-fix-2026-06-26.md` | `podman desktop` | `系统基础/topic-dictionary/tooling/podman-desktop.md` | exact |
| `_reports/broken-links-round4-fix-2026-06-26.md` | `aeraki mesh` | `系统基础/topic-dictionary/networking/aeraki-mesh.md` | exact |
| `_reports/broken-links-round4-fix-2026-06-26.md` | `serverless workflow` | `系统基础/topic-dictionary/workloads/serverless-workflow.md` | exact |
| `_reports/broken-links-round4-fix-2026-06-26.md` | `confidential containers` | `系统基础/topic-dictionary/security/confidential-containers.md` | exact |
| `_reports/broken-links-round4-fix-2026-06-26.md` | `logging operator` | `系统基础/topic-dictionary/observability/logging-operator.md` | exact |
| `_reports/broken-links-round4-fix-2026-06-26.md` | `secrets management` | `skills/best-practices/best-practices/security/secrets-management.md` | exact |
| `_reports/broken-links-round4-fix-2026-06-26.md` | `[[entities/inspektor-gadget.md|inspektor gadget]]` | `[[entities/inspektor-gadget.md|inspektor gadget]]` | exact |
| `_reports/broken-links-round4-fix-2026-06-26.md` | `kubernetes` | `entities/kubernetes.md` | fuzzy |
| `_reports/broken-links-round4-fix-2026-06-26.md` | `k8s` | `系统基础/topic-cheat-sheet/k8s.md` | fuzzy |
| `_reports/broken-links-round4-fix-2026-06-26.md` | `[[concepts/kubernetes-architecture-overview.md|kubernetes architecture overview]]` | `[[concepts/kubernetes-architecture-overview.md|kubernetes architecture overview]]` | fuzzy |
| `_reports/broken-links-round4-fix-2026-06-26.md` | `go` | `系统基础/topic-cheat-sheet/go.md` | fuzzy |
| `_reports/broken-links-round4-fix-2026-06-26.md` | `etcd index` | `生态参考/topic-index/etcd-index.md` | fuzzy |
| `_reports/broken-links-round4-fix-2026-06-26.md` | `containerd` | `系统基础/topic-dictionary/fundamentals/containerd.md` | fuzzy |
| `_reports/broken-links-round4-fix-2026-06-26.md` | `sidecar containers` | `系统基础/topic-dictionary/workloads/sidecar-containers.md` | fuzzy |

## Converted

| Source | Original | Reason |
|---|---|---|
| `_reports/broken-links-round4-fix-2026-06-26.md` | `-t 0` | no match |
| `_reports/broken-links-round4-fix-2026-06-26.md` | `"${CURRENT_CTX}" != kind-*` | no match |
| `_reports/broken-links-round4-fix-2026-06-26.md` | `"year",
    "make",
    "model",
    "length"` | no match |
| `_reports/broken-links-round4-fix-2026-06-26.md` | `'current_density', ...` | no match |
| `_reports/broken-links-round4-fix-2026-06-26.md` | `deployment` | no match |
| `_reports/broken-links-round4-fix-2026-06-26.md` | `[[STRUCTURE.md]]` | no match |
| `_reports/broken-links-round4-fix-2026-06-26.md` | `_meta/corpus-config/profiles/rag-ticket-agent-profile` | _meta link |
| `_reports/broken-links-round4-fix-2026-06-26.md` | `'current_density', 'temperature',
                             'pressure', 'electrolyte_conc', 'input_power'` | no match |
| `_reports/broken-links-round4-fix-2026-06-26.md` | `current_density, temperature,
                              pressure, electrolyte_conc, input_power` | no match |
| `_reports/broken-links-round4-fix-2026-06-26.md` | `'type', 'checkbox', 'radio'` | no match |
| `_reports/broken-links-round4-fix-2026-06-26.md` | `'className', /^hljs-/` | no match |
| `_reports/broken-links-round4-fix-2026-06-26.md` | `'className', 'number', 'operator', 'token'` | no match |
| `_reports/broken-links-round4-fix-2026-06-26.md` | `系统基础/topic-dictionary/workloads/[[sidecar-containers` | no match |
| `_reports/broken-links-round4-fix-2026-06-26.md` | `系统基础/topic-dictionary/scheduling/[[gang-scheduling` | no match |
| `_reports/broken-links-round4-fix-2026-06-26.md` | `系统基础/topic-dictionary/scheduling/[[dynamic-resource-allocation` | no match |
| `_reports/broken-links-round4-fix-2026-06-26.md` | `系统基础/topic-dictionary/scheduling/[[pod-overhead` | no match |
| `_reports/broken-links-round4-fix-2026-06-26.md` | `[[系统基础/topic-dictionary/workloads/ephemeral-containers.md|ephemeral containers]]` | no match |
| `_reports/broken-links-round4-fix-2026-06-26.md` | `平台工程/topic-code-analysis/MOC`` | no match |
| `_reports/broken-links-round4-fix-2026-06-26.md` | `basename` → `path/basename`
- AI基础设施/ — 修复 ~150 个文件中的 150+ 嵌套链接
- 应用模式/ — 修复 ~90 个文件中的 450+ 嵌套链接
- 故障诊断/topic-fta/ — 修复 ~30 个文件中的 140+ 嵌套链接
- 故障诊断/topic-febm/ — 修复 ~10 个文件中的 45+ 嵌套链接
- 生产运维/topic-best-practices/migration/ — 修复 ~10 个文件中的 40+ 嵌套链接
- 其他 domain 文件 — 修复 ~50 个文件中的 115+ 嵌套链接` | no match |

## Remaining

| Source | Original |
|---|---|
| `_reports/broken-links-round3-fix-2026-06-26.md` | `sidecar containers` |
| `_reports/WIKI-LINT-REPORT-2026-05-21.md` | `平台工程/topic-code-analysis/MOC`` |
| `_reports/WIKI-LINT-REPORT-2026-05-21.md` | `basename` → `path/basename`
- AI基础设施/ — 修复 ~150 个文件中的 150+ 嵌套链接
- 应用模式/ — 修复 ~90 个文件中的 450+ 嵌套链接
- 故障诊断/topic-fta/ — 修复 ~30 个文件中的 140+ 嵌套链接
- 故障诊断/topic-febm/ — 修复 ~10 个文件中的 45+ 嵌套链接
- 生产运维/topic-best-practices/migration/ — 修复 ~10 个文件中的 40+ 嵌套链接
- 其他 domain 文件 — 修复 ~50 个文件中的 115+ 嵌套链接` |
| `_reports/broken-links-full-fix-2026-06-26.md` | `sidecar containers` |
| `_reports/broken-links-round4-fix-2026-06-26.md` | `sidecar containers` |
| `_reports/obsidian-wiki-skills-evaluation-2026-05-24.md` | `[[]]` |
| `_reports/wiki-lint-2026-05-24.md` | `-t 0` |

<!-- risk-assessed -->
