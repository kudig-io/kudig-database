---
title: KUDIG Domain 目录映射表
category: references
tags:
- structure
- taxonomy
- mapping
- domain
- llm-wiki
tier: supporting
created: '2026-07-09'
last_updated: '2026-07-09'
---

# KUDIG Domain 目录映射表

> 本表是当前 20 个中文源文档域与规范化英文 `domain-XX-<slug>` 之间的唯一映射，供批量迁移、语料配置和 wikilink 重写使用。

| 编号 | 中文目录 | 推荐英文目录名 | Taxonomy Tag | 当前第二层子目录 | 推荐第二层结构 |
|---:|---|---|---|---|---|
| 01 | `集群基础/` | `domain-01-cluster-fundamentals` | `domain/cluster-fundamentals` | `01-architecture-overview/`, `02-design-principles/`, `03-control-plane/`, `04-api-versions/`, `05-kubectl/`, `06-upgrade-paths/`, `07-performance-tuning/`, `98-merged-indexes/` | `00-overview/`, `01-architecture-overview/`, `02-design-principles/`, `03-control-plane/`, `04-api-versions/`, `05-kubectl/`, `06-upgrade-paths/`, `07-performance-tuning/`, `98-merged-indexes/`, `99-production-readiness-operations-guide.md` |
| 02 | `工作负载/` | `domain-02-workloads-applications` | `domain/workloads-applications` | `00-core-workloads/`, `98-merged-indexes/`, `topic-functions/`, `topic-java-kubernetes/` | `00-overview/`, `01-core-workloads/`, `02-deployment-patterns/`, `98-merged-indexes/`, `topic-functions/`, `topic-java-kubernetes/`, `99-production-readiness-operations-guide.md` |
| 03 | `网络/` | `domain-03-networking-traffic` | `domain/networking-traffic` | `00-core-k8s-networking/`, `01-fundamentals/`, `02-service-mesh/`, `03-api-gateway/`, `04-ebpf/`, `98-merged-indexes/`, `99-attachments/`, `topic-terway/` | `00-overview/`, `01-core-k8s-networking/`, `02-fundamentals/`, `03-service-mesh/`, `04-api-gateway/`, `05-ebpf/`, `98-merged-indexes/`, `topic-terway/`, `99-production-readiness-operations-guide.md` |
| 04 | `存储/` | `domain-04-storage-data` | `domain/storage-data` | `01-k8s-storage/`, `02-storage-fundamentals/`, `03-distributed-storage/`, `04-stateful-app-storage/`, `98-merged-indexes/` | `00-overview/`, `01-k8s-storage/`, `02-storage-fundamentals/`, `03-distributed-storage/`, `04-stateful-app-storage/`, `98-merged-indexes/`, `99-production-readiness-operations-guide.md` |
| 05 | `安全/` | `domain-05-security-compliance` | `domain/security-compliance` | `01-identity-access/`, `02-network-security/`, `03-runtime-security/`, `04-policy-governance/`, `05-supply-chain/`, `06-compliance/`, `07-incident-response/`, `98-merged-indexes/` | `00-overview/`, `01-identity-access/`, `02-network-security/`, `03-runtime-security/`, `04-policy-governance/`, `05-supply-chain/`, `06-compliance/`, `07-incident-response/`, `98-merged-indexes/`, `99-production-readiness-operations-guide.md` |
| 06 | `可观测性/` | `domain-06-observability` | `domain/observability` | `01-overview/`, `02-metrics/`, `03-logging/`, `04-tracing/`, `05-alerting/`, `06-slo-sli/`, `07-tools/`, `98-merged-indexes/` | `00-overview/`, `01-metrics/`, `02-logging/`, `03-tracing/`, `04-alerting/`, `05-slo-sli/`, `06-tools/`, `98-merged-indexes/`, `99-production-readiness-operations-guide.md` |
| 07 | `平台工程/` | `domain-07-platform-engineering` | `domain/platform-engineering` | `98-merged-indexes/`, `build/`, `developer-experience/`, `governance/`, `operate/`, `topic-code-analysis/` | `00-overview/`, `01-build/`, `02-developer-experience/`, `03-governance/`, `04-operate/`, `98-merged-indexes/`, `topic-code-analysis/`, `99-production-readiness-operations-guide.md` |
| 08 | `发布变更/` | `domain-08-release-change-management` | `domain/release-change-management` | `01-gitops/`, `02-iac/`, `03-change-management/`, `04-testing-quality/`, `98-merged-indexes/`, `topic-deployment/`, `topic-migration/` | `00-overview/`, `01-gitops/`, `02-iac/`, `03-change-management/`, `04-testing-quality/`, `98-merged-indexes/`, `topic-deployment/`, `topic-migration/`, `99-production-readiness-operations-guide.md` |
| 09 | `可靠性/` | `domain-09-reliability-engineering` | `domain/reliability-engineering` | `01-backup-recovery/`, `02-disaster-recovery/`, `03-capacity-planning/`, `04-slo-sli/`, `05-chaos-engineering/`, `06-postmortem/`, `07-sre-practices/`, `08-performance-testing/`, `09-disaster-recovery-playbooks/`, `98-merged-indexes/` | `00-overview/`, `01-backup-recovery/`, `02-disaster-recovery/`, `03-capacity-planning/`, `04-slo-sli/`, `05-chaos-engineering/`, `06-postmortem/`, `07-sre-practices/`, `08-performance-testing/`, `09-disaster-recovery-playbooks/`, `98-merged-indexes/`, `99-production-readiness-operations-guide.md` |
| 10 | `故障诊断/` | `domain-10-troubleshooting-diagnostics` | `domain/troubleshooting-diagnostics` | `00-core-troubleshooting/`, `01-resource-troubleshooting/`, `02-infrastructure-troubleshooting/`, `03-advanced-troubleshooting/`, `04-jvm-tuning/`, `98-merged-indexes/`, `tools/`, `topic-febm/`, `topic-fta/`, `topic-multi-fault-scenarios/`, `topic-qa-corpus/`, `topic-skills/`, `topic-structural-trouble-shooting/` | `00-overview/`, `01-core-troubleshooting/`, `02-resource-troubleshooting/`, `03-infrastructure-troubleshooting/`, `04-advanced-troubleshooting/`, `05-jvm-tuning/`, `98-merged-indexes/`, `tools/`, `topic-febm/`, `topic-fta/`, `topic-multi-fault-scenarios/`, `topic-qa-corpus/`, `topic-skills/`, `topic-structural-trouble-shooting/`, `99-production-readiness-operations-guide.md` |
| 11 | `生产运维/` | `domain-11-production-operations` | `domain/production-operations` | `01-finops/`, `02-governance/`, `03-incident-response/`, `04-green-computing/`, `98-merged-indexes/`, `reply-templates/`, `ticket-cases/` | `00-overview/`, `01-finops/`, `02-governance/`, `03-incident-response/`, `04-green-computing/`, `98-merged-indexes/`, `reply-templates/`, `ticket-cases/`, `99-production-readiness-operations-guide.md` |
| 12 | `云厂商/` | `domain-12-cloud-providers` | `domain/cloud-providers` | `01-alibaba-cloud/`, `02-aws-eks/`, `03-google-cloud-gke/`, `04-azure-aks/`, `05-alicloud-ack/`, `06-tencent-tke/`, `07-huawei-cce/`, `08-multi-cloud/`, `09-ucloud-uk8s/`, `10-ibm-iks/`, `11-oracle-oke/`, `12-volcengine-vek/`, `13-ctyun-tke/`, `14-ecloud-cke/`, `15-alicloud-apsara-ack/`, `98-merged-indexes/` | `00-overview/`, `01-alibaba-cloud/`, `02-aws-eks/`, `03-google-cloud-gke/`, `04-azure-aks/`, `05-alicloud-ack/`, `06-tencent-tke/`, `07-huawei-cce/`, `08-multi-cloud/`, `09-ucloud-uk8s/`, `10-ibm-iks/`, `11-oracle-oke/`, `12-volcengine-vek/`, `13-ctyun-tke/`, `14-ecloud-cke/`, `15-alicloud-apsara-ack/`, `98-merged-indexes/`, `99-production-readiness-operations-guide.md` |
| 13 | `容器运行时/` | `domain-13-container-runtime` | `domain/container-runtime` | `01-docker/`, `02-image-management/`, `03-containerd-cri-o/`, `04-image-build/`, `05-runtime-migration/`, `98-merged-indexes/` | `00-overview/`, `01-docker/`, `02-image-management/`, `03-containerd-cri-o/`, `04-image-build/`, `05-runtime-migration/`, `98-merged-indexes/`, `99-production-readiness-operations-guide.md` |
| 14 | `AI基础设施/` | `domain-14-ai-ml-infra` | `domain/ai-ml-infra` | `01-ai-infra/`, `02-ai-agents/`, `03-agent-runtime/`, `98-merged-indexes/`, `topic-ai-coding/` | `00-overview/`, `01-ai-infra/`, `02-ai-agents/`, `03-agent-runtime/`, `98-merged-indexes/`, `topic-ai-coding/`, `99-production-readiness-operations-guide.md` |
| 15 | `专项技术/` | `domain-15-specialized-tech` | `domain/specialized-tech` | `01-edge-computing/`, `02-webassembly/`, `03-extensions/`, `04-serverless/`, `05-ebpf-programming/`, `98-merged-indexes/` | `00-overview/`, `01-edge-computing/`, `02-webassembly/`, `03-extensions/`, `04-serverless/`, `05-ebpf-programming/`, `98-merged-indexes/`, `99-production-readiness-operations-guide.md` |
| 16 | `数据库中间件/` | `domain-16-database-middleware` | `domain/database-middleware` | `01-databases/`, `02-cache/`, `03-message-queues/`, `04-time-series-db/`, `05-operator-management/`, `06-data-streaming/`, `98-merged-indexes/` | `00-overview/`, `01-databases/`, `02-cache/`, `03-message-queues/`, `04-time-series-db/`, `05-operator-management/`, `06-data-streaming/`, `98-merged-indexes/`, `99-production-readiness-operations-guide.md` |
| 17 | `系统基础/` | `domain-17-system-foundation` | `domain/system-foundation` | `01-linux/`, `02-hardware/`, `03-kubernetes-events/`, `98-merged-indexes/`, `topic-cheat-sheet/`, `topic-dictionary/` | `00-overview/`, `01-linux/`, `02-hardware/`, `03-kubernetes-events/`, `98-merged-indexes/`, `topic-cheat-sheet/`, `topic-dictionary/`, `99-production-readiness-operations-guide.md` |
| 18 | `清单模式/` | `domain-18-manifests-patterns` | `domain/manifests-patterns` | `01-yaml-reference/`, `02-kustomize-patterns/`, `03-helm-values-patterns/`, `98-merged-indexes/` | `00-overview/`, `01-yaml-reference/`, `02-kustomize-patterns/`, `03-helm-values-patterns/`, `98-merged-indexes/`, `99-production-readiness-operations-guide.md` |
| 19 | `生态参考/` | `domain-19-landscape-references` | `domain/landscape-references` | `_archived-release-notes/`, `01-cncf-landscape/`, `02-papers/`, `98-merged-indexes/`, `topic-index/`, `topic-release-notes/` | `00-overview/`, `01-cncf-landscape/`, `02-papers/`, `98-merged-indexes/`, `topic-index/`, `topic-release-notes/`, `archived-release-notes/`, `99-production-readiness-operations-guide.md` |
| 20 | `应用模式/` | `domain-20-application-patterns` | `domain/application-patterns` | `98-merged-indexes/`, `sub-patterns/`, `topic-application-architecture/`, `topic-production-patterns/` | `00-overview/`, `01-application-architecture/`, `02-production-patterns/`, `03-sub-patterns/`, `98-merged-indexes/`, `99-production-readiness-operations-guide.md` |

---

## 第二层命名约定

1. **编号章节**：`NN-<english-slug>/`，`NN` 为两位数字，从 `00`（overview）开始顺序递增。
2. **自动生成索引**：`98-merged-indexes/`，保留现有位置。
3. **域级入口文件**：`99-production-readiness-operations-guide.md`。
4. **跨域专题**：仅在子主题确实跨多个 domain 时使用 `topic-<slug>/`；否则应放入对应编号章节。
5. **归档目录**：避免使用 `_` 前缀在非元数据域中；归档内容使用 `archived-<name>/` 或不放在域内。

---

## 使用方式

- **批量重命名脚本**：读取本表第一列和第三列，生成 `git mv` 命令序列。
- **语料配置**：以第三列 `domain-XX-<slug>/` 作为 `include.path` 的标准前缀。
- **wikilink 重写**：在迁移脚本中，将第二列替换为第三列，并同步更新 `index.md`、`README.md`、`STRUCTURE.md`。
