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
| `_reports/wiki-lint-audit-2026-06-26.md` | `[[实体/kudig-prompts-catalog.md|kudig prompts catalog]]` | `[[实体/kudig-prompts-catalog.md|kudig prompts catalog]]` | exact |
| `_reports/wiki-lint-audit-2026-06-26.md` | `chaos mesh` | `系统基础/topic-dictionary/operations/chaos-mesh.md` | exact |
| `_reports/wiki-lint-audit-2026-06-26.md` | `artifact hub` | `系统基础/topic-dictionary/tooling/artifact-hub.md` | exact |
| `_reports/wiki-lint-audit-2026-06-26.md` | `virtual kubelet` | `系统基础/topic-dictionary/fundamentals/virtual-kubelet.md` | exact |
| `_reports/wiki-lint-audit-2026-06-26.md` | `operator framework` | `系统基础/topic-dictionary/platform-engineering/operator-framework.md` | exact |
| `_reports/wiki-lint-audit-2026-06-26.md` | `connect rpc` | `系统基础/topic-dictionary/networking/connect-rpc.md` | exact |
| `_reports/wiki-lint-audit-2026-06-26.md` | `[[实体/oscal-compass.md|oscal compass]]` | `[[实体/oscal-compass.md|oscal compass]]` | exact |
| `_reports/broken-links-fix-2026-06-26.md` | `[[实体/kudig-prompts-catalog.md|kudig prompts catalog]]` | `[[实体/kudig-prompts-catalog.md|kudig prompts catalog]]` | exact |
| `_reports/broken-links-fix-2026-06-26.md` | `chaos mesh` | `系统基础/topic-dictionary/operations/chaos-mesh.md` | exact |
| `_reports/broken-links-fix-2026-06-26.md` | `artifact hub` | `系统基础/topic-dictionary/tooling/artifact-hub.md` | exact |
| `_reports/broken-links-fix-2026-06-26.md` | `virtual kubelet` | `系统基础/topic-dictionary/fundamentals/virtual-kubelet.md` | exact |
| `_reports/broken-links-fix-2026-06-26.md` | `operator framework` | `系统基础/topic-dictionary/platform-engineering/operator-framework.md` | exact |
| `_reports/broken-links-fix-2026-06-26.md` | `connect rpc` | `系统基础/topic-dictionary/networking/connect-rpc.md` | exact |
| `_reports/broken-links-fix-2026-06-26.md` | `[[实体/oscal-compass.md|oscal compass]]` | `[[实体/oscal-compass.md|oscal compass]]` | exact |
| `_meta/_insights.md` | `kubernetes` | `系统基础/topic-dictionary/fundamentals/kubernetes.md` | fuzzy |
| `_meta/_insights.md` | `prometheus` | `系统基础/topic-dictionary/observability/prometheus.md` | fuzzy |
| `_meta/_insights.md` | `etcd` | `系统基础/topic-dictionary/fundamentals/etcd.md` | fuzzy |
| `_meta/_insights.md` | `service` | `系统基础/topic-dictionary/networking/service.md` | fuzzy |
| `_meta/_insights.md` | `kubelet` | `系统基础/topic-dictionary/fundamentals/kubelet.md` | fuzzy |
| `_meta/_insights.md` | `gitops cicd index` | `生态参考/topic-index/gitops-cicd-index.md` | fuzzy |
| `_meta/_insights.md` | `helm` | `系统基础/topic-dictionary/tooling/helm.md` | fuzzy |
| `_meta/_insights.md` | `ingress` | `系统基础/topic-dictionary/networking/ingress.md` | fuzzy |
| `_meta/_insights.md` | `argocd` | `entities/argocd.md` | fuzzy |
| `_meta/_insights.md` | `README` | `生态参考/topic-release-notes/README.md` | fuzzy |
| `_meta/_insights.md` | `cilium` | `系统基础/topic-dictionary/networking/cilium.md` | fuzzy |
| `_meta/_insights.md` | `istio` | `系统基础/topic-dictionary/networking/istio.md` | fuzzy |
| `_meta/_insights.md` | `pods` | `系统基础/topic-dictionary/workloads/pods.md` | fuzzy |
| `_meta/_insights.md` | `[[CONTRIBUTING.md]]` | `[[CONTRIBUTING.md]]` | fuzzy |
| `_meta/_insights.md` | `hot` | `_meta/journal/hot.md` | fuzzy |
| `_meta/_insights.md` | `README` | `生态参考/topic-release-notes/README.md` | fuzzy |
| `_meta/_insights.md` | `log` | `_meta/journal/log.md` | fuzzy |
| `entities/hyperlight.md` | `operator framework` | `系统基础/topic-dictionary/platform-engineering/operator-framework.md` | exact |
| `entities/kubeslice.md` | `operator framework` | `系统基础/topic-dictionary/platform-engineering/operator-framework.md` | exact |
| `entities/clusternet.md` | `operator framework` | `系统基础/topic-dictionary/platform-engineering/operator-framework.md` | exact |
| `entities/opengemini.md` | `notary project` | `系统基础/topic-dictionary/security/notary-project.md` | exact |
| `entities/kured.md` | `notary project` | `系统基础/topic-dictionary/security/notary-project.md` | exact |
| `entities/contour.md` | `cloud custodian` | `系统基础/topic-dictionary/operations/cloud-custodian.md` | exact |
| `entities/contour.md` | `notary project` | `系统基础/topic-dictionary/security/notary-project.md` | exact |
| `entities/coredns.md` | `notary project` | `系统基础/topic-dictionary/security/notary-project.md` | exact |
| `entities/cncf-storage.md` | `piraeus datastore` | `系统基础/topic-dictionary/storage/piraeus-datastore.md` | exact |
| `entities/cncf-security.md` | `notary project` | `系统基础/topic-dictionary/security/notary-project.md` | exact |
| `entities/flatcar.md` | `[[实体/serverless-devs.md|serverless devs]]` | `[[实体/serverless-devs.md|serverless devs]]` | exact |
| `entities/serverless-devs.md` | `oauth2 proxy` | `系统基础/topic-dictionary/security/oauth2-proxy.md` | exact |
| `entities/opa.md` | `oauth2 proxy` | `系统基础/topic-dictionary/security/oauth2-proxy.md` | exact |
| `entities/composefs.md` | `oauth2 proxy` | `系统基础/topic-dictionary/security/oauth2-proxy.md` | exact |
| `entities/opencost.md` | `piraeus datastore` | `系统基础/topic-dictionary/storage/piraeus-datastore.md` | exact |
| `entities/parsec.md` | `piraeus datastore` | `系统基础/topic-dictionary/storage/piraeus-datastore.md` | exact |
| `entities/k8up.md` | `piraeus datastore` | `系统基础/topic-dictionary/storage/piraeus-datastore.md` | exact |
| `entities/kubelet.md` | `pod lifecycle` | `系统基础/topic-dictionary/workloads/pod-lifecycle.md` | exact |
| `entities/rook.md` | `virtual kubelet` | `系统基础/topic-dictionary/fundamentals/virtual-kubelet.md` | exact |
| `entities/02-containerd-v2-features.md` | `virtual kubelet` | `系统基础/topic-dictionary/fundamentals/virtual-kubelet.md` | exact |
| `entities/kudo.md` | `podman desktop` | `系统基础/topic-dictionary/tooling/podman-desktop.md` | exact |
| `entities/virtual-kubelet.md` | `podman desktop` | `系统基础/topic-dictionary/tooling/podman-desktop.md` | exact |
| `entities/45-terway-testing-validation.md` | `aeraki mesh` | `系统基础/topic-dictionary/networking/aeraki-mesh.md` | exact |
| `entities/submariner.md` | `aeraki mesh` | `系统基础/topic-dictionary/networking/aeraki-mesh.md` | exact |
| `entities/dragonfly.md` | `serverless workflow` | `系统基础/topic-dictionary/workloads/serverless-workflow.md` | exact |
| `entities/hwameistor.md` | `serverless workflow` | `系统基础/topic-dictionary/workloads/serverless-workflow.md` | exact |
| `entities/strimzi.md` | `serverless workflow` | `系统基础/topic-dictionary/workloads/serverless-workflow.md` | exact |
| `entities/cloudnativepg.md` | `serverless workflow` | `系统基础/topic-dictionary/workloads/serverless-workflow.md` | exact |
| `entities/serverless-workflow.md` | `confidential containers` | `系统基础/topic-dictionary/security/confidential-containers.md` | exact |
| `entities/bootc.md` | `confidential containers` | `系统基础/topic-dictionary/security/confidential-containers.md` | exact |
| `entities/confidential-containers.md` | `confidential containers` | `系统基础/topic-dictionary/security/confidential-containers.md` | exact |
| `entities/cncf-edge-ai.md` | `[[实体/serverless-devs.md|serverless devs]]` | `[[实体/serverless-devs.md|serverless devs]]` | exact |
| `entities/cncf-edge-ai.md` | `serverless workflow` | `系统基础/topic-dictionary/workloads/serverless-workflow.md` | exact |
| `entities/akri.md` | `podman desktop` | `系统基础/topic-dictionary/tooling/podman-desktop.md` | exact |
| `entities/spire.md` | `podman desktop` | `系统基础/topic-dictionary/tooling/podman-desktop.md` | exact |
| `entities/carina.md` | `podman desktop` | `系统基础/topic-dictionary/tooling/podman-desktop.md` | exact |
| `entities/openyurt.md` | `podman desktop` | `系统基础/topic-dictionary/tooling/podman-desktop.md` | exact |
| `entities/hami.md` | `artifact hub` | `系统基础/topic-dictionary/tooling/artifact-hub.md` | exact |
| `entities/k8gb.md` | `chaos mesh` | `系统基础/topic-dictionary/operations/chaos-mesh.md` | exact |
| `entities/tikv.md` | `chaos mesh` | `系统基础/topic-dictionary/operations/chaos-mesh.md` | exact |
| `entities/cncf-observability.md` | `logging operator` | `系统基础/topic-dictionary/observability/logging-operator.md` | exact |
| `安全/01-identity-access/99-vault-k8s-secrets-guide.md` | `secrets management` | `skills/best-practices/best-practices/security/secrets-management.md` | exact |
| `可靠性/07-sre-practices/04-toil-reduction-automation.md` | `virtual kubelet` | `系统基础/topic-dictionary/fundamentals/virtual-kubelet.md` | exact |
| `可靠性/05-chaos-engineering/02-chaos-mesh-deployment.md` | `chaos mesh` | `系统基础/topic-dictionary/operations/chaos-mesh.md` | exact |
| `可靠性/05-chaos-engineering/01-chaos-engineering-overview.md` | `chaos mesh` | `系统基础/topic-dictionary/operations/chaos-mesh.md` | exact |
| `可靠性/05-chaos-engineering/04-litmus-practices.md` | `chaos mesh` | `系统基础/topic-dictionary/operations/chaos-mesh.md` | exact |
| `可靠性/02-disaster-recovery/08-chaos-engineering-platforms.md` | `chaos mesh` | `系统基础/topic-dictionary/operations/chaos-mesh.md` | exact |
| `集群基础/02-design-principles/15-chaos-engineering.md` | `chaos mesh` | `系统基础/topic-dictionary/operations/chaos-mesh.md` | exact |
| `skills/training-public/inner-training/week-3-node-workload/day-19-pod-basics.md` | `pod lifecycle` | `系统基础/topic-dictionary/workloads/pod-lifecycle.md` | exact |
| `skills/training-public/inner-training/week-3-node-workload/README.md` | `pod lifecycle` | `系统基础/topic-dictionary/workloads/pod-lifecycle.md` | exact |
| `网络/00-core-k8s-networking/40-terway-product-overview.md` | `connect rpc` | `系统基础/topic-dictionary/networking/connect-rpc.md` | exact |
| `网络/00-core-k8s-networking/45-terway-testing-validation.md` | `aeraki mesh` | `系统基础/topic-dictionary/networking/aeraki-mesh.md` | exact |
| `系统基础/topic-dictionary/operations/chaos-engineering.md` | `chaos mesh` | `系统基础/topic-dictionary/operations/chaos-mesh.md` | exact |
| `故障诊断/topic-fta/appendix-b-tools-and-resources.md` | `chaos mesh` | `系统基础/topic-dictionary/operations/chaos-mesh.md` | exact |
| `故障诊断/00-core-troubleshooting/00-open-source-projects-index-from-domain-12.md` | `[[实体/inspektor-gadget.md|inspektor gadget]]` | `[[实体/inspektor-gadget.md|inspektor gadget]]` | exact |
| `生态参考/01-cncf-landscape/03-cncf-selection-guide.md` | `chaos mesh` | `系统基础/topic-dictionary/operations/chaos-mesh.md` | exact |
| `生态参考/02-papers/15-kubernetes-chaos-engineering-fault-injection-testing.md` | `chaos mesh` | `系统基础/topic-dictionary/operations/chaos-mesh.md` | exact |
| `生态参考/_archived-release-notes/core-deps/cri-o/RELEASE-NOTES-0.1.md` | `pod lifecycle` | `系统基础/topic-dictionary/workloads/pod-lifecycle.md` | exact |

## Converted to Plain Text

| Source | Original |
|---|---|
| `_reports/obsidian-wiki-skills-evaluation-2026-05-24.md` | `-t 0` |
| `_reports/obsidian-wiki-skills-evaluation-2026-05-24.md` | `'current_density', ...` |
| `_meta/_insights.md` | `kubernetes` |
| `_meta/_insights.md` | `k8s` |
| `_meta/_insights.md` | `[[概念/kubernetes-architecture-overview.md|kubernetes architecture overview]]` |
| `_meta/_insights.md` | `go` |
| `_meta/_insights.md` | `etcd index` |
| `_meta/_insights.md` | `containerd` |
| `_meta/_insights.md` | `Deployment\` |
| `_meta/_insights.md` | `KUDIG-DATABASE 目录结构规范\` |
| `synthesis/ticket-agent-rag.md` | `_meta/corpus-config/profiles/rag-ticket-agent-profile` |
| `应用模式/topic-application-architecture/85-hydrogen-energy.md` | `'current_density', 'temperature',
                             'pressure', 'electrolyte_conc', 'input_power'` |
| `应用模式/topic-application-architecture/85-hydrogen-energy.md` | `current_density, temperature,
                              pressure, electrolyte_conc, input_power` |
| `web/node_modules/hast-util-sanitize/readme.md` | `'type', 'checkbox', 'radio'` |
| `web/node_modules/hast-util-sanitize/readme.md` | `'className', /^hljs-/` |
| `web/node_modules/hast-util-sanitize/readme.md` | `'className', 'number', 'operator', 'token'` |
| `生态参考/topic-index/pod-index.md` | `系统基础/topic-dictionary/workloads/[[sidecar-containers` |
| `生态参考/topic-index/etcd-index.md` | `系统基础/topic-dictionary/scheduling/[[gang-scheduling` |
| `生态参考/topic-index/ai-gpu-index.md` | `系统基础/topic-dictionary/scheduling/[[dynamic-resource-allocation` |
| `生态参考/topic-index/ai-gpu-index.md` | `系统基础/topic-dictionary/scheduling/[[gang-scheduling` |
| `生态参考/topic-index/scheduler-index.md` | `系统基础/topic-dictionary/scheduling/[[dynamic-resource-allocation` |
| `生态参考/topic-index/scheduler-index.md` | `系统基础/topic-dictionary/scheduling/[[gang-scheduling` |
| `生态参考/topic-index/scheduler-index.md` | `系统基础/topic-dictionary/scheduling/[[pod-overhead` |
| `生态参考/_archived-release-notes/kubernetes/CHANGELOG-1.25.md` | `[ephemeral-containers` |

## Failed/Skipped

| Source | Original | Reason |
|---|---|---|
| `_reports/WIKI-LINT-REPORT-2026-05-21.md` | `平台工程/topic-code-analysis/MOC`` | pattern not found |
| `_reports/WIKI-LINT-REPORT-2026-05-21.md` | `basename` → `path/basename`
- AI基础设施/ — 修复 ~150 个文件中的 150+ 嵌套链接
- 应用模式/ — 修复 ~90 个文件中的 450+ 嵌套链接
- 故障诊断/topic-fta/ — 修复 ~30 个文件中的 140+ 嵌套链接
- 故障诊断/topic-febm/ — 修复 ~10 个文件中的 45+ 嵌套链接
- 生产运维/topic-best-practices/migration/ — 修复 ~10 个文件中的 40+ 嵌套链接
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
