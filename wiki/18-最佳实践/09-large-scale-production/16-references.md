---
title: 参考语料来源清单
description: 大规模 Kubernetes 生产最佳实践专题引用的权威语料来源清单，含官方文档、云厂商指南与社区实践，标注检索日期与权威等级
summary: 本专题引用来源索引：Kubernetes 官方文档、AWS EKS 最佳实践、阿里云 ACK 官方建议、CIS/OWASP 安全标准与社区实践
category: references
tags:
- k8s
- references
- sources
tier: supporting
created: '2026-08-03'
last_updated: '2026-08-03'
difficulty: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
---

# 参考语料来源清单

> 本专题内容以公开权威语料为依据综合整理。检索日期：2026-08-03。权威等级沿用检索结果评级（S > A > B > C）。

## Kubernetes 官方文档（权威等级 A）

| 来源 | 用于本专题 |
|---|---|
| [Considerations for large clusters（v1.36 文档）](https://kubernetes.io/docs/setup/best-practices/cluster-large/) | 官方上限（5,000 节点/150k Pod/300k 容器/单节点 110 Pod）、云配额预申请、分批扩容、Addon 资源调整、关键组件 PriorityClass（`system-cluster-critical`/`system-node-critical`）→ [[01-overview]]、[[02-cluster-configuration]]、[[11-autoscaling-capacity]] |
| [Scalability updates in Kubernetes 1.6（官方博客）](https://kubernetes.io/blog/2017/03/scalability-updates-in-kubernetes-1-6/) | 官方可伸缩性 SLO：API P99 < 1s、Pod 启动 P99 < 5s（镜像已预热）；>5,000 节点建议联邦 → [[01-overview]]、[[15-slo-chaos-engineering]] |
| [Good practices for Dynamic Resource Allocation（v1.36 文档）](https://kubernetes.io/docs/concepts/cluster-administration/dra/) | 高规模环境 scheduler/controller-manager QPS 调优思路（DRA 场景，可迁移参考）→ [[02-cluster-configuration]] |

## 云厂商官方指南（权威等级 A）

| 来源 | 用于本专题 |
|---|---|
| [阿里云 ACK 大规模集群使用建议](https://help.aliyun.com/zh/ack/ack-managed-and-ack-dedicated/user-guide/suggestions-on-how-to-work-with-large-ack-pro-clusters) | 大规模定义（>500 节点或 >10,000 Pod）、中心化控制器反模式（避免每节点控制器全量 Watch/List）→ [[01-overview]]、[[02-cluster-configuration]] |
| [阿里云 ACK 集群高可用架构推荐配置](https://help.aliyun.com/zh/ack/ack-managed-and-ack-dedicated/user-guide/best-practices-for-cluster/) | Worker 大规格实例建议、大规模集群指引 → [[01-overview]]、[[11-autoscaling-capacity]] |
| [阿里云 ACK 集群成本管理最佳实践](https://help.aliyun.com/zh/ack/ack-managed-and-ack-dedicated/user-guide/cluster-cost-optimization-recommendations) | 生产不推荐 ≤2C4G 节点、按负载画像选规格（内存型 1:8、GPU 1:8–1:12）、FinOps 体系 → [[11-autoscaling-capacity]]、[[14-cost-finops]] |
| [阿里云 ACK One（分布式云容器平台）](https://www.aliyun.com/product/aliware/adcp) | 舰队多集群管理、GitOps 分发、备份容灾一体化 → [[10-multi-cluster]] |
| [AWS EKS Best Practices Guide（awslabs/aws-eks-best-practices）](https://github.com/aws/aws-eks-best-practices) | Day-2 运营、安全、可靠性、性能效率、成本优化体系 → 全专题 |

## 安全标准与加固（权威等级 A/B/NA）

| 来源 | 用于本专题 |
|---|---|
| CIS Kubernetes Benchmark（经 [AKS CIS 对照文档](https://docs.azure.cn/en-us/aks/cis-kubernetes) 与 [microk8s CIS 加固讨论](https://discuss.kubernetes.io/t/cis-hardening-and-assesment/24491) 交叉确认） | 准入插件基线（`EventRateLimit`/`AlwaysPullImages`/`NodeRestriction`）、`system:masters` 规避、bind/impersonate/escalate 限制、PSS 关键项 → [[12-security-hardening-baseline]] |
| [OWASP Kubernetes Top 10（2025 版，经 appsecsanta 指南引述）](https://appsecsanta.com/application-security/kubernetes-security-guide) | K01–K10 风险映射、K08 集群到云横向移动（新增）、K01/K02/K05 高频审计发现 → [[12-security-hardening-baseline]] |
| [A Practical Guide to Kubernetes Security（sealos.io，2025）](https://sealos.io/blog/a-practical-guide-to-kubernetes-security-hardening-your-cluster-in-2025/) | 威胁模型、90 天加固路线、供应链（SBOM/cosign/digest pin）、运行时检测（Falco/Tetragon）、云身份（IRSA/Workload Identity）→ [[12-security-hardening-baseline]] |

## 弹性伸缩与运维实践（社区，权威等级 B/NA，观点性内容已标注）

| 来源 | 用于本专题 |
|---|---|
| [EKS Best Practices for Platform Engineers（itmagic.pro，2026）](https://itmagic.pro/blog/eks-best-practices) | Karpenter 30–60s vs CA 3–5min、升级顺序与预检、EKS 控制面不可回滚、90 天运营计划、IRSA/KMS/PSS 八大控制 → [[11-autoscaling-capacity]]、[[13-upgrade-certificate-runbook]]、[[12-security-hardening-baseline]] |
| [EKS Karpenter Deep Dive（stormforge.io）](https://www.stormforge.io/kubernetes-autoscaling/eks-karpenter/) | Karpenter 已捐赠 CNCF、NodePool/NodeClass 架构 → [[11-autoscaling-capacity]] |
| [CA → Karpenter 迁移指南（allcloud.io）](https://allcloud.io/blog/emea-il-aws-eks-migration-from-cluster-autoscaler-to-karpenter-guide/) | 成本优化案例（约 58%，已在正文标注"案例参考，勿直接套用"）→ [[14-cost-finops]] |
| [Docker & Kubernetes Hardening Guide（thehgtech.com）](https://thehgtech.com/guides/container-security.html) | PSS 落地路径（warn → enforce）、RBAC 审计命令 → [[12-security-hardening-baseline]] |

## GPU/AI 与服务网格（2026-08-03 增补）

| 来源 | 用于本专题 |
|---|---|
| [Red Hat Developer：DRA GA in OpenShift 4.21（权威 A）](https://developers.redhat.com/articles/2026/03/25/dynamic-resource-allocation-goes-ga-red-hat-openshift-421-smarter-gpu) | DRA 于 K8s 1.34 GA、device plugin 五大局限（无共享/无拓扑/无参数化） → [[19-gpu-ai-workload]] |
| [kubernetes-sigs/kueue（GitHub，权威 S）](https://github.com/kubernetes-sigs/kueue) | Kueue 功能集：排队策略、Fair Sharing、Cohort、MultiKueue、Topology-Aware Scheduling、all-or-nothing → [[19-gpu-ai-workload]] |
| [AI/ML on Kubernetes 2026 Stack Guide（kubernetesguru.com）](https://kubernetesguru.com/ai-ml-on-kubernetes-2026-stack-guide/) | Kueue 利用率经验值（25–35% → 60–85%，社区口径已标注）、NVIDIA GPU Operator、KAI Scheduler → [[19-gpu-ai-workload]] |
| [Istio 官方文档：Sidecar or ambient?](https://istio.io/latest/docs/overview/dataplane-modes/) | ambient 单集群生产就绪（1.22+）、ztunnel/waypoint 架构、sidecar vs ambient 对比表（延迟、安全模型） → [[20-service-mesh-l7]] |
| [Istio 官方性能基准](https://istio.io/latest/docs/ops/deployment/performance-and-scalability/) | ambient L4 P99 延迟增量约 0.16–0.20ms vs sidecar 0.63–0.88ms → [[20-service-mesh-l7]] |
| [Tetrate：Istio Ambient Mode vs Ambient Mesh](https://tetrate.io/learn/istio-ambient-mode-vs-ambient-mesh) | NIST SP 800-233 代理模型指南引用 → [[20-service-mesh-l7]] |

## 使用注意

- 官方文档（kubernetes.io / help.aliyun.com / AWS）内容可视为事实基线；社区博客（B/NA 级）用于实践观点参考，正文已按需标注"经验值/案例参考"
- 版本相关参数（如 skew 政策、API 废弃）以目标集群版本对应的官方文档为准复核
- 本清单随专题更新维护，新增引用请追加登记
