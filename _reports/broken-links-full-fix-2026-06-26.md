---
title: 全库 Broken Wikilinks 修复报告（2026-06-26）
description: 扫描全库并自动修复 broken wikilinks
summary: 扫描全库并自动修复 broken wikilinks
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




# 全库 Broken Wikilinks 修复报告

- 总 broken links: 121
- 成功修复: 89
- 转纯文本: 24
- 失败/跳过: 8

## Fixed Links

| Source | Original | Replacement | Confidence |
|---|---|---|---|
| `_reports/wiki-lint-audit-2026-06-26.md` | `[[entities/kudig-prompts-catalog.md|kudig prompts catalog]]` | `[[entities/kudig-prompts-catalog.md|kudig prompts catalog]]` | exact |
| `_reports/wiki-lint-audit-2026-06-26.md` | `chaos mesh` | `domain-17-system-foundation/topic-dictionary/operations/chaos-mesh.md` | exact |
| `_reports/wiki-lint-audit-2026-06-26.md` | `artifact hub` | `domain-17-system-foundation/topic-dictionary/tooling/artifact-hub.md` | exact |
| `_reports/wiki-lint-audit-2026-06-26.md` | `virtual kubelet` | `domain-17-system-foundation/topic-dictionary/fundamentals/virtual-kubelet.md` | exact |
| `_reports/wiki-lint-audit-2026-06-26.md` | `operator framework` | `domain-17-system-foundation/topic-dictionary/platform-engineering/operator-framework.md` | exact |
| `_reports/wiki-lint-audit-2026-06-26.md` | `connect rpc` | `domain-17-system-foundation/topic-dictionary/networking/connect-rpc.md` | exact |
| `_reports/wiki-lint-audit-2026-06-26.md` | `[[entities/oscal-compass.md|oscal compass]]` | `[[entities/oscal-compass.md|oscal compass]]` | exact |
| `_reports/broken-links-fix-2026-06-26.md` | `[[entities/kudig-prompts-catalog.md|kudig prompts catalog]]` | `[[entities/kudig-prompts-catalog.md|kudig prompts catalog]]` | exact |
| `_reports/broken-links-fix-2026-06-26.md` | `chaos mesh` | `domain-17-system-foundation/topic-dictionary/operations/chaos-mesh.md` | exact |
| `_reports/broken-links-fix-2026-06-26.md` | `artifact hub` | `domain-17-system-foundation/topic-dictionary/tooling/artifact-hub.md` | exact |
| `_reports/broken-links-fix-2026-06-26.md` | `virtual kubelet` | `domain-17-system-foundation/topic-dictionary/fundamentals/virtual-kubelet.md` | exact |
| `_reports/broken-links-fix-2026-06-26.md` | `operator framework` | `domain-17-system-foundation/topic-dictionary/platform-engineering/operator-framework.md` | exact |
| `_reports/broken-links-fix-2026-06-26.md` | `connect rpc` | `domain-17-system-foundation/topic-dictionary/networking/connect-rpc.md` | exact |
| `_reports/broken-links-fix-2026-06-26.md` | `[[entities/oscal-compass.md|oscal compass]]` | `[[entities/oscal-compass.md|oscal compass]]` | exact |
| `_meta/_insights.md` | `kubernetes` | `domain-17-system-foundation/topic-dictionary/fundamentals/kubernetes.md` | fuzzy |
| `_meta/_insights.md` | `prometheus` | `domain-17-system-foundation/topic-dictionary/observability/prometheus.md` | fuzzy |
| `_meta/_insights.md` | `etcd` | `domain-17-system-foundation/topic-dictionary/fundamentals/etcd.md` | fuzzy |
| `_meta/_insights.md` | `service` | `domain-17-system-foundation/topic-dictionary/networking/service.md` | fuzzy |
| `_meta/_insights.md` | `kubelet` | `domain-17-system-foundation/topic-dictionary/fundamentals/kubelet.md` | fuzzy |
| `_meta/_insights.md` | `gitops cicd index` | `domain-19-landscape-references/topic-index/gitops-cicd-index.md` | fuzzy |
| `_meta/_insights.md` | `helm` | `domain-17-system-foundation/topic-dictionary/tooling/helm.md` | fuzzy |
| `_meta/_insights.md` | `ingress` | `domain-17-system-foundation/topic-dictionary/networking/ingress.md` | fuzzy |
| `_meta/_insights.md` | `argocd` | `entities/argocd.md` | fuzzy |
| `_meta/_insights.md` | `README` | `domain-19-landscape-references/topic-release-notes/README.md` | fuzzy |
| `_meta/_insights.md` | `cilium` | `domain-17-system-foundation/topic-dictionary/networking/cilium.md` | fuzzy |
| `_meta/_insights.md` | `istio` | `domain-17-system-foundation/topic-dictionary/networking/istio.md` | fuzzy |
| `_meta/_insights.md` | `pods` | `domain-17-system-foundation/topic-dictionary/workloads/pods.md` | fuzzy |
| `_meta/_insights.md` | `[[CONTRIBUTING.md]]` | `[[CONTRIBUTING.md]]` | fuzzy |
| `_meta/_insights.md` | `hot` | `_meta/journal/hot.md` | fuzzy |
| `_meta/_insights.md` | `README` | `domain-19-landscape-references/topic-release-notes/README.md` | fuzzy |
| `_meta/_insights.md` | `log` | `_meta/journal/log.md` | fuzzy |
| `entities/hyperlight.md` | `operator framework` | `domain-17-system-foundation/topic-dictionary/platform-engineering/operator-framework.md` | exact |
| `entities/kubeslice.md` | `operator framework` | `domain-17-system-foundation/topic-dictionary/platform-engineering/operator-framework.md` | exact |
| `entities/clusternet.md` | `operator framework` | `domain-17-system-foundation/topic-dictionary/platform-engineering/operator-framework.md` | exact |
| `entities/opengemini.md` | `notary project` | `domain-17-system-foundation/topic-dictionary/security/notary-project.md` | exact |
| `entities/kured.md` | `notary project` | `domain-17-system-foundation/topic-dictionary/security/notary-project.md` | exact |
| `entities/contour.md` | `cloud custodian` | `domain-17-system-foundation/topic-dictionary/operations/cloud-custodian.md` | exact |
| `entities/contour.md` | `notary project` | `domain-17-system-foundation/topic-dictionary/security/notary-project.md` | exact |
| `entities/coredns.md` | `notary project` | `domain-17-system-foundation/topic-dictionary/security/notary-project.md` | exact |
| `entities/cncf-storage.md` | `piraeus datastore` | `domain-17-system-foundation/topic-dictionary/storage/piraeus-datastore.md` | exact |
| `entities/cncf-security.md` | `notary project` | `domain-17-system-foundation/topic-dictionary/security/notary-project.md` | exact |
| `entities/flatcar.md` | `[[entities/serverless-devs.md|serverless devs]]` | `[[entities/serverless-devs.md|serverless devs]]` | exact |
| `entities/serverless-devs.md` | `oauth2 proxy` | `domain-17-system-foundation/topic-dictionary/security/oauth2-proxy.md` | exact |
| `entities/opa.md` | `oauth2 proxy` | `domain-17-system-foundation/topic-dictionary/security/oauth2-proxy.md` | exact |
| `entities/composefs.md` | `oauth2 proxy` | `domain-17-system-foundation/topic-dictionary/security/oauth2-proxy.md` | exact |
| `entities/opencost.md` | `piraeus datastore` | `domain-17-system-foundation/topic-dictionary/storage/piraeus-datastore.md` | exact |
| `entities/parsec.md` | `piraeus datastore` | `domain-17-system-foundation/topic-dictionary/storage/piraeus-datastore.md` | exact |
| `entities/k8up.md` | `piraeus datastore` | `domain-17-system-foundation/topic-dictionary/storage/piraeus-datastore.md` | exact |
| `entities/kubelet.md` | `pod lifecycle` | `domain-17-system-foundation/topic-dictionary/workloads/pod-lifecycle.md` | exact |
| `entities/rook.md` | `virtual kubelet` | `domain-17-system-foundation/topic-dictionary/fundamentals/virtual-kubelet.md` | exact |
| `entities/02-containerd-v2-features.md` | `virtual kubelet` | `domain-17-system-foundation/topic-dictionary/fundamentals/virtual-kubelet.md` | exact |
| `entities/kudo.md` | `podman desktop` | `domain-17-system-foundation/topic-dictionary/tooling/podman-desktop.md` | exact |
| `entities/virtual-kubelet.md` | `podman desktop` | `domain-17-system-foundation/topic-dictionary/tooling/podman-desktop.md` | exact |
| `entities/45-terway-testing-validation.md` | `aeraki mesh` | `domain-17-system-foundation/topic-dictionary/networking/aeraki-mesh.md` | exact |
| `entities/submariner.md` | `aeraki mesh` | `domain-17-system-foundation/topic-dictionary/networking/aeraki-mesh.md` | exact |
| `entities/dragonfly.md` | `serverless workflow` | `domain-17-system-foundation/topic-dictionary/workloads/serverless-workflow.md` | exact |
| `entities/hwameistor.md` | `serverless workflow` | `domain-17-system-foundation/topic-dictionary/workloads/serverless-workflow.md` | exact |
| `entities/strimzi.md` | `serverless workflow` | `domain-17-system-foundation/topic-dictionary/workloads/serverless-workflow.md` | exact |
| `entities/cloudnativepg.md` | `serverless workflow` | `domain-17-system-foundation/topic-dictionary/workloads/serverless-workflow.md` | exact |
| `entities/serverless-workflow.md` | `confidential containers` | `domain-17-system-foundation/topic-dictionary/security/confidential-containers.md` | exact |
| `entities/bootc.md` | `confidential containers` | `domain-17-system-foundation/topic-dictionary/security/confidential-containers.md` | exact |
| `entities/confidential-containers.md` | `confidential containers` | `domain-17-system-foundation/topic-dictionary/security/confidential-containers.md` | exact |
| `entities/cncf-edge-ai.md` | `[[entities/serverless-devs.md|serverless devs]]` | `[[entities/serverless-devs.md|serverless devs]]` | exact |
| `entities/cncf-edge-ai.md` | `serverless workflow` | `domain-17-system-foundation/topic-dictionary/workloads/serverless-workflow.md` | exact |
| `entities/akri.md` | `podman desktop` | `domain-17-system-foundation/topic-dictionary/tooling/podman-desktop.md` | exact |
| `entities/spire.md` | `podman desktop` | `domain-17-system-foundation/topic-dictionary/tooling/podman-desktop.md` | exact |
| `entities/carina.md` | `podman desktop` | `domain-17-system-foundation/topic-dictionary/tooling/podman-desktop.md` | exact |
| `entities/openyurt.md` | `podman desktop` | `domain-17-system-foundation/topic-dictionary/tooling/podman-desktop.md` | exact |
| `entities/hami.md` | `artifact hub` | `domain-17-system-foundation/topic-dictionary/tooling/artifact-hub.md` | exact |
| `entities/k8gb.md` | `chaos mesh` | `domain-17-system-foundation/topic-dictionary/operations/chaos-mesh.md` | exact |
| `entities/tikv.md` | `chaos mesh` | `domain-17-system-foundation/topic-dictionary/operations/chaos-mesh.md` | exact |
| `entities/cncf-observability.md` | `logging operator` | `domain-17-system-foundation/topic-dictionary/observability/logging-operator.md` | exact |
| `domain-05-security-compliance/01-identity-access/99-vault-k8s-secrets-guide.md` | `secrets management` | `skills/best-practices/best-practices/security/secrets-management.md` | exact |
| `domain-09-reliability-engineering/07-sre-practices/04-toil-reduction-automation.md` | `virtual kubelet` | `domain-17-system-foundation/topic-dictionary/fundamentals/virtual-kubelet.md` | exact |
| `domain-09-reliability-engineering/05-chaos-engineering/02-chaos-mesh-deployment.md` | `chaos mesh` | `domain-17-system-foundation/topic-dictionary/operations/chaos-mesh.md` | exact |
| `domain-09-reliability-engineering/05-chaos-engineering/01-chaos-engineering-overview.md` | `chaos mesh` | `domain-17-system-foundation/topic-dictionary/operations/chaos-mesh.md` | exact |
| `domain-09-reliability-engineering/05-chaos-engineering/04-litmus-practices.md` | `chaos mesh` | `domain-17-system-foundation/topic-dictionary/operations/chaos-mesh.md` | exact |
| `domain-09-reliability-engineering/02-disaster-recovery/08-chaos-engineering-platforms.md` | `chaos mesh` | `domain-17-system-foundation/topic-dictionary/operations/chaos-mesh.md` | exact |
| `domain-01-cluster-fundamentals/02-design-principles/15-chaos-engineering.md` | `chaos mesh` | `domain-17-system-foundation/topic-dictionary/operations/chaos-mesh.md` | exact |
| `skills/training-public/inner-training/week-3-node-workload/day-19-pod-basics.md` | `pod lifecycle` | `domain-17-system-foundation/topic-dictionary/workloads/pod-lifecycle.md` | exact |
| `skills/training-public/inner-training/week-3-node-workload/README.md` | `pod lifecycle` | `domain-17-system-foundation/topic-dictionary/workloads/pod-lifecycle.md` | exact |
| `domain-03-networking-traffic/00-core-k8s-networking/40-terway-product-overview.md` | `connect rpc` | `domain-17-system-foundation/topic-dictionary/networking/connect-rpc.md` | exact |
| `domain-03-networking-traffic/00-core-k8s-networking/45-terway-testing-validation.md` | `aeraki mesh` | `domain-17-system-foundation/topic-dictionary/networking/aeraki-mesh.md` | exact |
| `domain-17-system-foundation/topic-dictionary/operations/chaos-engineering.md` | `chaos mesh` | `domain-17-system-foundation/topic-dictionary/operations/chaos-mesh.md` | exact |
| `domain-10-troubleshooting-diagnostics/topic-fta/appendix-b-tools-and-resources.md` | `chaos mesh` | `domain-17-system-foundation/topic-dictionary/operations/chaos-mesh.md` | exact |
| `domain-10-troubleshooting-diagnostics/00-core-troubleshooting/00-open-source-projects-index-from-domain-12.md` | `[[entities/inspektor-gadget.md|inspektor gadget]]` | `[[entities/inspektor-gadget.md|inspektor gadget]]` | exact |
| `domain-19-landscape-references/01-cncf-landscape/03-cncf-selection-guide.md` | `chaos mesh` | `domain-17-system-foundation/topic-dictionary/operations/chaos-mesh.md` | exact |
| `domain-19-landscape-references/02-papers/15-kubernetes-chaos-engineering-fault-injection-testing.md` | `chaos mesh` | `domain-17-system-foundation/topic-dictionary/operations/chaos-mesh.md` | exact |
| `domain-19-landscape-references/_archived-release-notes/core-deps/cri-o/RELEASE-NOTES-0.1.md` | `pod lifecycle` | `domain-17-system-foundation/topic-dictionary/workloads/pod-lifecycle.md` | exact |

## Converted to Plain Text

| Source | Original |
|---|---|
| `_reports/obsidian-wiki-skills-evaluation-2026-05-24.md` | `-t 0` |
| `_reports/obsidian-wiki-skills-evaluation-2026-05-24.md` | `'current_density', ...` |
| `_meta/_insights.md` | `kubernetes` |
| `_meta/_insights.md` | `k8s` |
| `_meta/_insights.md` | `[[concepts/kubernetes-architecture-overview.md|kubernetes architecture overview]]` |
| `_meta/_insights.md` | `go` |
| `_meta/_insights.md` | `etcd index` |
| `_meta/_insights.md` | `containerd` |
| `_meta/_insights.md` | `Deployment\` |
| `_meta/_insights.md` | `KUDIG-DATABASE 目录结构规范\` |
| `synthesis/ticket-agent-rag.md` | `_meta/corpus-config/profiles/rag-ticket-agent-profile` |
| `domain-20-application-patterns/topic-application-architecture/85-hydrogen-energy.md` | `'current_density', 'temperature',
                             'pressure', 'electrolyte_conc', 'input_power'` |
| `domain-20-application-patterns/topic-application-architecture/85-hydrogen-energy.md` | `current_density, temperature,
                              pressure, electrolyte_conc, input_power` |
| `web/node_modules/hast-util-sanitize/readme.md` | `'type', 'checkbox', 'radio'` |
| `web/node_modules/hast-util-sanitize/readme.md` | `'className', /^hljs-/` |
| `web/node_modules/hast-util-sanitize/readme.md` | `'className', 'number', 'operator', 'token'` |
| `domain-19-landscape-references/topic-index/pod-index.md` | `domain-17-system-foundation/topic-dictionary/workloads/[[sidecar-containers` |
| `domain-19-landscape-references/topic-index/etcd-index.md` | `domain-17-system-foundation/topic-dictionary/scheduling/[[gang-scheduling` |
| `domain-19-landscape-references/topic-index/ai-gpu-index.md` | `domain-17-system-foundation/topic-dictionary/scheduling/[[dynamic-resource-allocation` |
| `domain-19-landscape-references/topic-index/ai-gpu-index.md` | `domain-17-system-foundation/topic-dictionary/scheduling/[[gang-scheduling` |
| `domain-19-landscape-references/topic-index/scheduler-index.md` | `domain-17-system-foundation/topic-dictionary/scheduling/[[dynamic-resource-allocation` |
| `domain-19-landscape-references/topic-index/scheduler-index.md` | `domain-17-system-foundation/topic-dictionary/scheduling/[[gang-scheduling` |
| `domain-19-landscape-references/topic-index/scheduler-index.md` | `domain-17-system-foundation/topic-dictionary/scheduling/[[pod-overhead` |
| `domain-19-landscape-references/_archived-release-notes/kubernetes/CHANGELOG-1.25.md` | `[ephemeral-containers` |

## Failed/Skipped

| Source | Original | Reason |
|---|---|---|
| `_reports/WIKI-LINT-REPORT-2026-05-21.md` | `domain-07-platform-engineering/topic-code-analysis/MOC`` | pattern not found |
| `_reports/WIKI-LINT-REPORT-2026-05-21.md` | `basename` → `path/basename`
- domain-14-ai-ml-infra/ — 修复 ~150 个文件中的 150+ 嵌套链接
- domain-20-application-patterns/ — 修复 ~90 个文件中的 450+ 嵌套链接
- domain-10-troubleshooting-diagnostics/topic-fta/ — 修复 ~30 个文件中的 140+ 嵌套链接
- domain-10-troubleshooting-diagnostics/topic-febm/ — 修复 ~10 个文件中的 45+ 嵌套链接
- domain-11-production-operations/topic-best-practices/migration/ — 修复 ~10 个文件中的 40+ 嵌套链接
- 其他 domain 文件 — 修复 ~50 个文件中的 115+ 嵌套链接` | pattern not found |
| `_reports/obsidian-wiki-skills-evaluation-2026-05-24.md` | `[[]]` | pattern not found |
| `_reports/wiki-lint-2026-05-24.md` | `-t 0` | pattern not found |
| `.comate/specs/topic-skills-code-review/doc.md` | `-t 0` | pattern not found |
| `.comate/specs/topic-skills-code-review/tasks.md` | `-t 0` | pattern not found |
| `.comate/specs/topic-skills-enhancement/summary.md` | `"${CURRENT_CTX}" != kind-*` | pattern not found |
| `web/node_modules/d3-dsv/README.md` | `"year",
    "make",
    "model",
    "length"` | pattern not found |

<!-- risk-assessed -->
