---
title: 生产就绪内容补充推送 — 2026-07-01
description: 基于 20 个 Domain 生产环境缺口分析，补充的 per-domain 生产就绪运维指南与跨域 Runbook 清单。
summary: 基于 20 个 Domain 生产环境缺口分析，补充的 per-domain 生产就绪运维指南与跨域 Runbook 清单。
category: reports
tags:
- reports
- production
- best-practices
- operations
- gap-analysis
- runbook
tier: peripheral
created: '2026-07-01'
last_updated: '2026-07'
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 平台工程师
estimated_read_time: 10min
intent_queries:
- 本次补充了哪些生产就绪内容
- 20 个 Domain 生产就绪指南在哪里
trigger_keywords:
- 生产就绪
- 内容补充
- Runbook
- 缺口分析
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。


# 生产就绪内容补充推送 — 2026-07-01

## 背景

本次推送基于 `_reports/domain-content-gap-analysis-2026-07-01.md` 对 20 个编号 Domain 的生产环境缺口审计结果，优先补齐 **高影响、跨域通用、可直接落地** 的运维内容。

---

## 新增内容总览

| 类型 | 数量 | 说明 |
|:---|:---:|:---|
| Per-Domain 生产就绪运维指南 | 20 | 每个编号 Domain 一本入口级指南 |
| 跨域生产 Runbook / 操作指南 | 8 | 覆盖证书、升级、灾备、Fleet GitOps、事件响应、FinOps、AI/ML、边缘 |
| 第二波专项 Runbook / 指南 | 10 | PRR、AWS EKS、阿里云 ACK、运行时安全、供应链安全、SLO 运营、多集群、节点与运行时、Etcd、Redis |
| 第三波专项 Runbook / 指南 | 6 | GKE、Azure AKS、PostgreSQL、MySQL、Wasm 生产部署、安全运营 |
| 缺口分析报告 | 1 | `_reports/domain-content-gap-analysis-2026-07-01.md` |
| 合计 | **45** | 全部为新增或完整增强文件，未破坏已有内容 |

---

## Per-Domain 生产就绪运维指南

每个指南均包含：生产检查清单、关键风险与缓解、日常运维操作、故障排查速查、跨域协作边界、推荐阅读。

| Domain | 文件路径 |
|:---|:---|
| 01 集群基础 | `domain-01-cluster-fundamentals/99-production-readiness-operations-guide.md` |
| 02 工作负载与应用 | `domain-02-workloads-applications/99-production-readiness-operations-guide.md` |
| 03 网络流量 | `domain-03-networking-traffic/99-production-readiness-operations-guide.md` |
| 04 存储数据 | `domain-04-storage-data/99-production-readiness-operations-guide.md` |
| 05 安全合规 | `domain-05-security-compliance/99-production-readiness-operations-guide.md` |
| 06 可观测性 | `domain-06-observability/99-production-readiness-operations-guide.md` |
| 07 平台工程 | `domain-07-platform-engineering/99-production-readiness-operations-guide.md` |
| 08 发布变更管理 | `domain-08-release-change-management/99-production-readiness-operations-guide.md` |
| 09 可靠性工程 | `domain-09-reliability-engineering/99-production-readiness-operations-guide.md` |
| 10 排障诊断 | `domain-10-troubleshooting-diagnostics/99-production-readiness-operations-guide.md` |
| 11 生产运维 | `domain-11-production-operations/99-production-readiness-operations-guide.md` |
| 12 云厂商 | `domain-12-cloud-providers/99-production-readiness-operations-guide.md` |
| 13 容器运行时 | `domain-13-container-runtime/99-production-readiness-operations-guide.md` |
| 14 AI/ML 基础设施 | `domain-14-ai-ml-infra/99-production-readiness-operations-guide.md` |
| 15 专项技术 | `domain-15-specialized-tech/99-production-readiness-operations-guide.md` |
| 16 数据库中间件 | `domain-16-database-middleware/99-production-readiness-operations-guide.md` |
| 17 系统基础 | `domain-17-system-foundation/99-production-readiness-operations-guide.md` |
| 18 清单模式 | `domain-18-manifests-patterns/99-production-readiness-operations-guide.md` |
| 19 生态参考 | `domain-19-landscape-references/99-production-readiness-operations-guide.md` |
| 20 应用模式 | `domain-20-application-patterns/99-production-readiness-operations-guide.md` |

---

## 跨域生产 Runbook / 操作指南

| 主题 | 文件路径 | 覆盖重点 |
|:---|:---|:---|
| 证书 / PKI 生命周期 | `domain-01-cluster-fundamentals/03-control-plane/34-certificate-pki-lifecycle-runbook.md` | kubeadm 证书轮换、CA 轮换、cert-manager、Ingress/mTLS、过期告警 |
| 集群升级 | `domain-01-cluster-fundamentals/03-control-plane/35-cluster-upgrade-runbook.md` | 升级前检查、控制面/工作节点滚动升级、回滚决策矩阵 |
| 灾难恢复与业务连续性 | `domain-09-reliability-engineering/09-disaster-recovery-playbooks/03-disaster-recovery-bc-runbook.md` | RTO/RPO、etcd 恢复、Velero 集群恢复、AZ/Region 切换、演练 |
| Fleet GitOps | `domain-08-release-change-management/01-gitops/08-fleet-gitops-operations-guide.md` | ApplicationSet、Karmada/OCM/Cluster API、跨集群 Secret、漂移检测 |
| 事件响应模板 | `domain-11-production-operations/03-incident-response/24-incident-response-runbook-template.md` | 严重级别、角色分工、War Room、证据保全、Postmortem |
| FinOps 成本治理 | `domain-11-production-operations/01-finops/14-finops-cost-governance-runbook.md` | 标签体系、Kubecost/OpenCost、Showback/Chargeback、Spot、异常检测 |
| AI/ML 运维 | `domain-14-ai-ml-infra/01-ai-infra/45-ai-ml-ops-runbook.md` | GPU OOM、NCCL 超时、推理延迟、模型回滚、Checkpoint、MIG/DRA |
| 边缘生产运维 | `domain-15-specialized-tech/01-edge-computing/14-edge-production-runbook.md` | CloudCore/EdgeCore HA、节点纳管、离线自治、边缘 DR |

---

## 第二波专项 Runbook / 指南

| 主题 | 文件路径 | 覆盖重点 |
|:---|:---|:---|
| 生产就绪评审（PRR）模板 | `domain-07-platform-engineering/99-production-readiness-review-template.md` | 检查清单、风险矩阵、会签、上线门控、回滚标准 |
| AWS EKS 生产 Runbook | `domain-12-cloud-providers/02-aws-eks/99-aws-eks-production-runbook.md` | 集群创建、IRSA、VPC CNI、升级、DR、可观测性、成本、排障 |
| 阿里云 ACK 生产 Runbook | `domain-12-cloud-providers/05-alicloud-ack/99-alicloud-ack-production-runbook.md` | 集群生命周期、Terway、RRSA、自动伸缩、升级、灾备、SLS、成本 |
| 容器运行时安全加固 | `domain-13-container-runtime/03-containerd-cri-o/06-runtime-security-hardening.md` | seccomp、AppArmor/SELinux、特权限制、User Namespaces、Falco/Tetragon |
| 供应链安全 Runbook | `domain-05-security-compliance/05-supply-chain/14-supply-chain-security-runbook.md` | SBOM、镜像签名、Kyverno/OPA 准入、仓库安全、CI/CD 加固 |
| SLO 运营指南 | `domain-06-observability/99-slo-operations-guide.md` | SLI/SLO/SLA、错误预算、燃速告警、告警评审、Dashboard-as-Code |
| 多集群运维 | `domain-11-production-operations/06-multi-cluster-operations.md` | 集群注册、舰队策略、Secret 同步、全局负载均衡、跨集群可观测性 |
| 节点与运行时运维 | `domain-11-production-operations/13-node-and-runtime-ops.md` | containerd、kubelet PLEG、NPD、descheduler、OS 补丁、镜像 GC |
| Etcd on Kubernetes | `domain-16-database-middleware/01-databases/09-etcd-on-kubernetes.md` | Quorum、磁盘延迟、备份恢复、成员替换、TLS 轮换、可观测性 |
| Redis on Kubernetes | `domain-16-database-middleware/01-databases/15-redis-kubernetes-production-guide.md` | Sentinel/Cluster、持久化、备份、NetworkPolicy、资源 QoS、故障转移 |

---

## 第三波专项 Runbook / 指南

| 主题 | 文件路径 | 覆盖重点 |
|:---|:---|:---|
| GKE 生产 Runbook | `domain-12-cloud-providers/03-google-cloud-gke/99-gke-production-runbook.md` | Autopilot/Standard、Workload Identity、VPC-native、节点池、升级、DR、成本、排障 |
| Azure AKS 生产 Runbook | `domain-12-cloud-providers/04-azure-aks/99-azure-aks-production-runbook.md` | 托管身份、Azure CNI、节点池、升级、DR、Azure Monitor、成本、排障 |
| PostgreSQL on Kubernetes | `domain-16-database-middleware/01-databases/16-postgresql-kubernetes-production-guide.md` | HA 拓扑、CloudNativePG/Patroni、备份/PITR、连接池、监控、故障转移 |
| MySQL on Kubernetes | `domain-16-database-middleware/01-databases/17-mysql-kubernetes-production-guide.md` | Group Replication/Operator、ProxySQL、备份、NetworkPolicy、QoS、故障转移 |
| Wasm 生产部署 | `domain-15-specialized-tech/02-webassembly/11-wasm-production-deployment.md` | containerd-wasm-shim/SpinKube、RuntimeClass、网络/存储、可观测性、供应链安全 |
| 安全运营 Runbook | `domain-11-production-operations/08-security-operations-runbook.md` | PSP→PSS 迁移、Secret 轮换、CIS 修复、漏洞响应、审计日志、事件隔离 |

---

## 质量校验

- 所有新增文件均使用项目标准 frontmatter。
- 所有文件内 Obsidian wikilink 已校验可解析（对尚未存在的推荐文件已转换为纯文本“待补充”引用）。
- 未覆盖、删除或修改任何已有文件。

---

## 后续仍需补充的内容

缺口分析中仍有专题需要深耕，主要包括：

1. **按云厂商深度补齐**：腾讯云/华为云/UCloud/IBM/Oracle 等各自的升级、DR、IAM、网络、成本、排障 Runbook（AWS、阿里云 ACK、GKE、Azure AKS 已完成）。
2. **垂直行业应用模式**：电商、金融科技、SaaS 多租户、IoT、游戏、IM/RTC、数据中台等生产架构模板。
3. **中间件专项**：Kafka/Pulsar/NATS、MongoDB 的 Day-2 运维手册（Redis、PostgreSQL、MySQL、Etcd on K8s 已完成）。
4. **安全合规深化**：VAP、节点 OS 加固、SIEM 集成、Registry 安全与镜像晋级（供应链安全、运行时安全、安全运营已完成）。
5. **可观测性深化**：OpenTelemetry Collector 生产模式、告警治理（SLO 运营已完成）。

完整清单与优先级参见 `_reports/domain-content-gap-analysis-2026-07-01.md` 第 4–6 节。

---

*Generated on 2026-07-01. 本次推送聚焦“生产就绪入口 + 跨域核心 Runbook”，后续可继续按缺口分析 roadmap 逐专题深耕。*


<!-- risk-assessed -->
