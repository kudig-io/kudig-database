---
title: 可靠性工程 生产就绪运维指南
description: 面向 SRE 的 Kubernetes 可靠性工程生产就绪检查、风险缓解、日常运维与故障排查手册
summary: 面向 SRE 的 Kubernetes 可靠性工程生产就绪检查、风险缓解、日常运维与故障排查手册
category: reliability-engineering
tags:
- production
- best-practices
- reliability-engineering
- operations
- sre
- slo
- disaster-recovery
- chaos-engineering
tier: core
created: '2026-07-01'
last_updated: '2026-07'
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 平台工程师
estimated_read_time: 20min
intent_queries:
- 可靠性工程 生产就绪运维指南是什么
- 如何按生产环境要求运维 可靠性工程
trigger_keywords:
- 生产就绪
- 运维指南
- 可靠性工程
- 生产就绪检查
- 可靠性风险
prerequisites:
- kubectl-basics
- sre-practices
- prometheus-basics
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
- '1.33'
authors:
- name: KUDIG Team
  role: contributor
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。


# 可靠性工程 生产就绪运维指南

> **核心原则**: 生产就绪不是一次性检查点，而是贯穿容量、变更、故障与观测的持续性工程实践。可靠性是设计出来的，更是运营出来的。

本指南面向 SRE 与平台工程师，聚焦 [[kubernetes.md|Kubernetes]] 可靠性工程在生产环境的落地要求。它整合了 KUDIG 知识库在 SLO/SLI、发布门控、备份恢复、混沌工程与灾备演练方面的现有内容，并针对当前 gap 分析中识别的控制平面韧性、PodDisruptionBudget 生产实践、有状态应用 DR、集群升级回滚与证书生命周期可靠性等关键缺口，提供可操作的检查清单、命令参考与跨域协作边界。

在生产环境中，可靠性的挑战通常不是单一组件故障，而是多个子系统在压力下的级联失效。因此，本指南强调以 SLO 为中心的闭环治理：先定义可量化的可靠性目标，再通过发布门控、容量管理、备份恢复与混沌演练持续验证目标可达性，最终通过无责复盘沉淀经验。

## 1. 生产环境检查清单

以下 12 项是投产前必须逐项确认的内容。任何未通过项都应在上线前闭环，或记录明确的风险接受人与补偿措施。

| 编号 | 检查项 | 验收标准 | 常用命令 |
|------|--------|---------|----------|
| 1 | 控制平面高可用 | API Server、etcd、scheduler 多实例健康，无单点 | `kubectl get pods -n kube-system` / `etcdctl endpoint health` |
| 2 | 节点跨可用区分布 | 工作节点均匀分布在 ≥3 AZ，关键 Pod 配置拓扑分布 | `kubectl get nodes -L topology.kubernetes.io/zone` |
| 3 | 优雅中断策略 | 核心服务配置 PodDisruptionBudget，确保升级/缩容时最小可用副本 | `kubectl get pdb -A` |
| 4 | 资源配额与限制 | 所有生产命名空间配置 ResourceQuota / LimitRange，Pod 设置 Request & Limit | `kubectl describe quota -n <ns>` |
| 5 | 备份与恢复验证 | etcd、PVC、命名级备份策略已配置，近 30 天内完成恢复演练 | `velero backup get` / `etcdctl snapshot status` |
| 6 | SLO/SLI 已定义 | 关键用户旅程已设定可量化的 SLO，错误预算与 Burn Rate 告警已上线 | 见 [[32-发布/package/2026-07-02_18-53/corpus/core/domain-09-reliability-engineering/02-slo-sli/02-slo-implementation-guide|SLO 设定与实施指南]] |
| 7 | 发布门控生效 | 错误预算不足时自动阻断或降级发布，金丝雀/蓝绿策略已配置 | 见 [[32-发布/package/2026-07-02_18-53/corpus/core/domain-09-reliability-engineering/04-sre-practices/01-release-gate-slo-based|基于 SLO 的发布门控]] |
| 8 | 证书生命周期监控 | K8s CA、cert-manager、Ingress mTLS 证书有效期 ≥30 天告警 | `kubeadm certs check-expiration` / `kubectl get certificate -A` |
| 9 | 灾备演练记录 | AZ/Region 级、控制面、有状态应用三类场景已演练并有 RTO/RPO 数据 | 见 [[32-发布/package/2026-07-02_18-53/corpus/supporting/domain-09-reliability-engineering/10-disaster-recovery-playbooks/01-dr-scenarios-catalog|灾备场景目录]] |
| 10 | 混沌工程安全护栏 | 实验具备 blast-radius 控制、kill-switch、自动回滚，且避开生产高峰 | 见 [[32-发布/package/2026-07-02_18-53/corpus/supporting/domain-09-reliability-engineering/06-chaos-engineering/02-chaos-experiment-design|混沌实验设计]] |
| 11 | 可观测性闭环 | 日志、指标、链路覆盖关键路径，告警已关联 on-call 值班表与升级策略 | `kubectl get prometheusrules -A` |
| 12 | 事后复盘文化 | 近 90 天内 P1/P0 事故已完成无责复盘，改进项有 Owner 与截止日期 | 见 [[32-发布/package/2026-07-02_18-53/corpus/supporting/domain-09-reliability-engineering/07-postmortem/01-blameless-postmortem-template|无责事后复盘模板]] |

建议将上表集成到发布平台（如 Backstage / Argo CD ApplicationSet）的 PRR Gate 中，未通过项自动阻塞 GitOps 同步，避免人工检查遗漏。PRR 评审结果应作为重大变更的准入条件，并在 `_meta/journal/` 中记录历史。

## 2. 关键风险与缓解措施

以下 5 项风险在生产环境中反复出现，应作为可靠性工程的优先级治理对象。

### 2.1 控制平面单点或性能瓶颈

控制平面是所有工作负载的调度与管理基座，一旦 API Server 过载、etcd 磁盘延迟飙升或证书过期，整个集群将陷入不可管理状态。生产事件统计表明，证书过期和 etcd 磁盘延迟是控制面故障的两大主因。

- **风险**: API Server 过载、etcd 磁盘延迟、证书过期导致全集群不可管理。
- **缓解**:
  - API Server 前置负载均衡，启用 Priority and Fairness（APF）限制大 List 请求；
  - etcd 使用独立 SSD，WAL fsync P99 控制在 10ms 以内，每日快照并验证完整性；
  - K8s 内部 CA、kubelet、certs-manager 与 Ingress 证书设置 30/15/7 天三级告警，并定期演练续期流程。
- **命令**:
  ```bash
  etcdctl snapshot save /backup/etcd-$(date +%F).db
  etcdctl snapshot status /backup/etcd-$(date +%F).db
  kubeadm certs check-expiration
  kubectl get --raw /apis/flowcontrol.apiserver.k8s.io/v1/flowschemas
  ```

### 2.2 有状态应用灾难恢复能力不足

有状态应用（数据库、消息队列、缓存、搜索引擎）的恢复复杂度远高于无状态服务。只备份不恢复演练，等同于没有备份。必须验证跨可用区甚至跨区域的备份与恢复路径。

- **风险**: 数据库/PVC 无跨区备份，StatefulSet 跨 AZ 调度失衡，故障转移未经验证。
- **缓解**:
  - 使用 Velero + CSI 快照，将关键数据备份同步到异地对象存储；
  - 配置 `topologySpreadConstraints` 与 `podAntiAffinity`，避免副本集中在一个 AZ；
  - 每季度执行主从切换、PVC 恢复、消费者组重平衡演练，并归档 RTO/RPO 数据。
- **命令**:
  ```bash
  velero backup create prod-db --include-namespaces db --snapshot-volumes
  velero backup get | grep -v Completed
  kubectl get pods -o wide -n db
  ```

### 2.3 无计划中断导致服务级联失效

节点维护、集群自动缩容、滚动更新参数不当，都可能在短时间内同时终止多个关键 Pod，触发级联超时或雪崩。PodDisruptionBudget 是防御此类风险的第一道防线。

- **风险**: 节点维护时批量驱逐关键 Pod、滚动更新过快、依赖超时未降级。
- **缓解**:
  - 为核心服务配置 PodDisruptionBudget，设定 `minAvailable` 或 `maxUnavailable`；
  - 滚动更新设置 `maxSurge=25% maxUnavailable=0`，配合 HPA 预热；
  - 配置超时、重试、熔断与兜底（如 Istio / resilience4j）。
- **命令**:
  ```bash
  kubectl create pdb web-pdb --selector app=web --min-available 2
  kubectl set image deploy/web web=web:v2 && kubectl rollout status deploy/web
  kubectl rollout undo deployment/web
  ```

### 2.4 容量不足与资源争抢

生产环境常见的容量风险包括突发流量导致 OOM、CPU Throttle、Pending Pod 堆积，以及多租户间的资源争抢。容量问题往往先于故障出现，因此需要建立常态化的容量审查机制。

- **风险**: 突发流量触发 OOM / CPU Throttle，多租户争抢，存储容量耗尽。
- **缓解**:
  - HPA + VPA + Cluster Autoscaler / Karpenter 组合扩缩容；
  - 命名空间级 ResourceQuota / LimitRange，防止单一租户耗尽资源；
  - PVC 使用率 80% 告警，StorageClass 启用 `allowVolumeExpansion: true`。
- **命令**:
  ```bash
  kubectl top nodes
  kubectl top pods -A --sort-by=cpu | head -n 10
  kubectl get pods -A --field-selector status.phase=Pending
  kubectl describe resourcequota -n <ns>
  ```

### 2.5 变更引入不可回滚

集群升级、应用发布和配置变更是生产故障的主要来源。缺乏回滚能力会显著延长故障恢复时间。所有生产变更都应遵循“可灰度、可监控、可回滚”的原则。

- **风险**: 集群升级后无法回退、应用发布无历史版本、配置变更导致异常。
- **缓解**:
  - 升级前执行 etcd 快照与 `kubeadm upgrade plan`，确认版本偏差在允许范围；
  - Helm / Argo Rollouts 保留最近 10 个 revision，Argo CD 启用历史回滚；
  - GitOps 流程中强制 diff 评审，重大 ConfigMap/Secret 变更先灰度。
- **命令**:
  ```bash
  kubectl version
  kubeadm upgrade plan
  helm history <release>
  argocd app rollback <app> <revision>
  kubectl diff -f config/
  ```

## 3. 日常运维操作

以下三类操作应分别按日、周、月执行，形成可靠性运营的固定节奏。所有异常发现都应记录到值班日志，并在周会中复盘。

### 3.1 每日巡检

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 集群健康概览
kubectl get nodes -o wide
kubectl get pods -A -o wide | grep -v Running | grep -v Completed

# 证书有效期
kubeadm certs check-expiration
kubectl get certificates -A -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{.status.notAfter}{"\n"}{end}'

# 备份任务状态
velero backup get | grep -v Completed
kubectl get cronjob -n velero
```
### 3.2 每周容量审查

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 资源使用率 Top 10
kubectl top nodes
kubectl top pods -A --sort-by=cpu | head -n 10

# 调度压力与 Pending Pod
kubectl get pods -A --field-selector status.phase=Pending
kubectl describe nodes | grep -A 5 "Allocated resources"
```
### 3.3 每月灾备演练

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# etcd 快照验证
etcdctl snapshot status /backup/etcd-$(date +%F-%H).db

# Velero 恢复演练（隔离命名空间）
velero restore create --from-backup prod-daily \
  --include-namespaces drill-ns \
  --namespace-mappings prod-ns:drill-ns

# 混沌实验（仅具备 kill-switch 时执行）
kubectl annotate deployment/web chaos-mesh.org/experiment-active="true" --overwrite
```
## 4. 故障排查速查

下表汇总了可靠性工程域最常见的生产症状、根因定位命令与修复动作。值班工程师可据此快速 narrowing down 问题范围。

| 现象 | 可能根因 | 确认命令 | 修复 / 缓解 |
|------|---------|---------|------------|
| 大量 Pod 同时 Terminating | 节点维护未配置 PDB 或集群自动缩容 | `kubectl get pdb -A` / `kubectl get events --sort-by=.lastTimestamp` | 补充 PDB；维护时 drain 加 `--ignore-daemonsets --delete-emptydir-data` |
| API Server 延迟高 / 502 | etcd 磁盘 IO 延迟、ListAll 大请求、API Server 负载不均 | `etcdctl endpoint health` / `kubectl get --raw /metrics` | 启用 APF；扩容 etcd 磁盘；为 large-list client 分页 |
| 证书过期告警 | kubeadm/kubelet/cert-manager 证书未自动续期 | `kubeadm certs check-expiration` / `kubectl get cert -A` | `kubeadm certs renew all`；检查 cert-manager Issuer/ClusterIssuer |
| 有状态 Pod 无法启动，PVC 挂载失败 | CSI 驱动异常、AZ 快照与节点 AZ 不匹配、StorageClass 未启用扩容 | `kubectl describe pvc <name>` / `kubectl logs -n kube-system csi-*` | 修复 CSI；调整 Pod 调度到快照源 AZ；扩容 PVC |
| HPA 未触发但 CPU 已满 | metrics-server 异常、HPA target 设置不合理、Pod 未设置 Request | `kubectl get hpa -A` / `kubectl top pods` | 修复 metrics-server；校准 target；为容器补充 resources.requests |
| Velero 备份失败 | Restic/Velero Pod OOM、对象存储权限不足、VolumeSnapshotClass 不支持 | `kubectl logs -n velero deploy/velero` | 增大 Velero 资源；检查 IAM Role / OIDC；确认 CSI snapshot class |
| 错误预算快速消耗 | 新版本发布异常、下游依赖故障、证书/网络抖动 | Burn Rate > 14.4x 告警 | 触发发布门控自动回滚；启动事故响应流程 |

## 5. 与其他域的协作边界

可靠性工程不是孤立职能，必须与相邻域建立清晰的职责界面。否则容易出现监控指标定义了却无人维护、发布策略有了却无门控、备份有了却从未恢复等“形式化可靠”的问题。

- **[[domain-06-observability/README.md|可观测性域]]**：SRE 负责定义 SLO/SLI 与错误预算，可观测性域负责指标采集、存储、告警路由与 Dashboard。双方共同维护 Burn Rate 告警与发布门控大盘。
- **[[domain-08-release-change-management/README.md|发布变更管理域]]**：发布策略（金丝雀、蓝绿、回滚）由变更域落地，SRE 负责将错误预算与发布门控集成到 CI/CD 与 GitOps 流程中。
- **[[domain-11-production-operations/README.md|生产运维域]]**：生产运维负责值班、事件响应与沟通，SRE 提供可靠性数据（SLO 状态、容量基线、历史故障模式）支撑决策。
- **[[domain-05-security-compliance/README.md|安全合规域]]**：证书生命周期、Secret 轮换、Pod 安全策略由安全域主导，SRE 负责监控有效期与故障场景下的快速恢复。
- **[[domain-01-cluster-fundamentals/README.md|集群基础域]]**：控制平面、etcd、节点生命周期由集群域负责，SRE 负责基于这些基座构建上层可靠性能力（PDB、拓扑分布、升级回滚）。
- **[[domain-04-storage-data/README.md|存储数据域]]**：有状态应用 DR 的底层能力（快照、备份、跨区复制）由存储域提供，SRE 负责编排应用级恢复流程与 RTO/RPO 验证。

## 6. 推荐阅读

### 本域核心参考

- [[32-发布/package/2026-07-02_18-53/corpus/core/domain-09-reliability-engineering/02-slo-sli/02-slo-implementation-guide|SLO 设定与实施指南]]
- [[32-发布/package/2026-07-02_18-53/corpus/core/domain-09-reliability-engineering/04-sre-practices/01-release-gate-slo-based|基于 SLO 的发布门控]]
- [[domain-09-reliability-engineering/备份恢复/01-etcd-backup-restore.md|etcd 备份与恢复]]
- [[32-发布/package/2026-07-02_18-53/corpus/supporting/domain-09-reliability-engineering/02-disaster-recovery/02-velero-backup-recovery-guide|Velero 备份恢复指南]]
- [[32-发布/package/2026-07-02_18-53/corpus/supporting/domain-09-reliability-engineering/06-chaos-engineering/02-chaos-experiment-design|混沌实验设计]]
- [[32-发布/package/2026-07-02_18-53/corpus/supporting/domain-09-reliability-engineering/10-disaster-recovery-playbooks/01-dr-scenarios-catalog|灾备场景目录]]
- [[32-发布/package/2026-07-02_18-53/corpus/peripheral/domain-09-reliability-engineering/05-disaster-recovery-playbooks/01-az-failure-playbook|AZ 故障 Playbook]]

### Gap 分析推荐新建

- 控制面故障 Playbook（待补充）
- PodDisruptionBudget 生产实践（待补充）
- 有状态应用 DR 模式（待补充）
- 集群升级与回滚 Playbook（待补充）
- 证书生命周期可靠性 Runbook（待补充）

### 跨域参考

- [[domain-06-observability/README.md|可观测性域]]
- [[domain-08-release-change-management/README.md|发布变更管理域]]
- [[domain-11-production-operations/README.md|生产运维域]]

---

*本指南应与每季度的生产就绪评审（PRR）结合使用，评审结果与改进项建议归档到 `_meta/journal/` 以便追踪。持续运营可靠性的关键在于：把每一次演练、每一场事故、每一个告警都转化为可复用的工程能力。*


<!-- risk-assessed -->
