---
title: 平台工程生产就绪运维指南
description: 面向 Kubernetes 平台工程域的生产就绪检查、风险缓解、日常运维与故障排查的综合性运维指南
summary: 平台工程域生产就绪检查清单、关键风险缓解、日常运维命令与故障排查速查
category: domain/platform-engineering
tags:
- production
- best-practices
- platform-engineering
- operations
- readiness
- sre
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
- 平台工程生产就绪运维指南是什么
- 如何按生产环境要求运维平台工程
trigger_keywords:
- 生产就绪
- 运维指南
- 平台工程
- platform engineering
- PRR
prerequisites:
- kubectl-basics
- platform-engineering-basics
- helm-basics
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

# 平台工程生产就绪运维指南

> **适用范围**: 以 Kubernetes 为底座的内部开发者平台（IDP）与平台工程团队  
> **最后更新**: 2026-07-01  
> **难度**: 高级

本指南覆盖平台工程域从生产就绪评审（PRR）到日常运维、故障排查的完整闭环，重点填补[[_reports/domain-content-gap-analysis-2026-07-01.md|域内容缺口分析]]中指出的"生产就绪检查清单、补丁与节点生命周期、证书轮换、平台级密钥管理、事故响应"等短板。

平台工程（Platform Engineering）团队的核心职责是为上层应用团队提供稳定、安全、可自愈的内部开发者平台（IDP）。生产就绪不仅仅意味着组件安装成功，更要求具备可观测性、可回滚性、可恢复性以及明确的事故响应路径。本指南将检查清单、风险缓解、运维操作与排查速查整合为一份可执行的入口文档，供 SRE 与平台工程师在 PRR、上线前验收及日常巡检中直接使用。

---

## 一、生产环境检查清单

在将平台工程相关组件（IDP 门户、GitOps 控制器、自动扩缩容、多租户治理、Secret 同步等）声明为生产就绪前，必须逐项确认以下 12 项检查点。每一项均附带验证命令与明确的通过标准，便于在 PRR 会议或上线 gate 中逐项勾选。

| 序号 | 检查项 | 验证命令 / 方法 | 通过标准 |
|:---|:---|:---|:---|
| 1 | 控制平面高可用 | `kubectl get nodes -l node-role.kubernetes.io/control-plane` | 至少 3 个控制平面节点，状态 Ready |
| 2 | etcd 备份可恢复 | `etcdctl snapshot status /backup/etcd-$(date +%F).db` | 每日备份，72 小时内完成恢复演练 |
| 3 | 平台核心组件 PDB | `kubectl get pdb -n argocd` / `-n keda` / `-n karpenter` | minAvailable ≥ 1，覆盖所有控制器 Pod |
| 4 | 多租户资源隔离 | `kubectl get resourcequota -n <team>` | 每个命名空间配置 CPU / 内存 / Pod / PVC 配额 |
| 5 | 默认拒绝网络策略 | `kubectl get networkpolicies -A` | 平台命名空间启用默认拒绝，仅放行白名单端口 |
| 6 | Secret 加密与轮换 | `kubectl get secret -n platform` 配合 KMS/ Vault 审计 | Secret 静态加密，平台级凭据 90 天轮换 |
| 7 | 证书到期监控 | `kubectl get certificates -A` / cert-manager metrics | 所有证书剩余有效期 ≥ 30 天 |
| 8 | GitOps 源完整性 | `argocd app list` + `argocd repo list` | 仓库启用 GPG / SSH 签名验证 |
| 9 | 自动扩缩容基线 | `kubectl get nodepool,scaledobject -A` | NodePool / ScaledObject 配置经过负载测试 |
| 10 | 可观测性全覆盖 | `kubectl get servicemonitor,probe -A` | 平台组件暴露 RED/USE 指标并配置告警 |
| 11 | 灾难恢复 runbook | 检查 `domain-09-reliability-engineering` 中对应 playbooks | RTO/RPO 经业务确认并季度演练 |
| 12 | 变更窗口与回滚 | `argocd app history` / Helm release 历史 | 所有平台组件保留 ≥ 10 个历史版本 |

清单中的每一项都应落实到责任人（Owner）与检查周期。建议在 Confluence、Notion 或内部 Git 仓库中维护一份可审计的 PRR 记录表，记录每次评审日期、发现项、修复人与复验结果。对于多集群平台，应在每个生产集群重复执行本清单，并通过 GitOps 将集群基线配置版本化，确保任何偏离都能被快速发现。

---

## 二、关键风险与缓解措施

平台工程域在生产环境中面临的高影响风险及对应的缓解方案如下。每项风险均给出可直接执行的命令或配置片段，便于纳入 runbook 或自动化脚本。

### 2.1 GitOps 控制器单点故障

- **风险**: Argo CD / Flux 控制器崩溃导致应用停止同步，无法回滚。
- **缓解**:
  ```bash
  # 检查控制器高可用副本
  kubectl get deploy argocd-server argocd-repo-server argocd-application-controller -n argocd -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{.spec.replicas}{"\n"}{end}'
  # 建议：server ≥ 2, repo-server ≥ 2, application-controller 启用分片 (--replicas > 1)
  ```
- **配置要点**: 为所有控制器设置 PodDisruptionBudget、反亲和性、持久化 Redis。同时建议将 Argo CD 的 `application-controller` 配置为分片模式，以提升大规模集群下的同步吞吐；Redis 采用 Sentinel 或 Redis Cluster 模式部署，避免单点。平台团队应定期执行 `argocd admin settings validate` 校验配置一致性。

### 2.2 证书过期引发服务中断

- **风险**: cert-manager、Ingress、平台 webhook 证书过期导致 API 不可用。
- **缓解**:
  ```bash
  # 查看 cert-manager 管理的证书状态
  kubectl get certificate -A
  kubectl get certificaterequest,order,challenge -n cert-manager
  # 紧急续期
  kubectl cert-manager renew --namespace=<ns> <certificate-name>
  ```
- **配置要点**: 配置 `Certificate` 的 `renewBefore: 720h`，Prometheus 告警 `certmanager_certificate_expiration_timestamp_seconds < 30*24*3600`。对于 Kubernetes 内部 CA（front-proxy、etcd、kubelet）以及 Ingress mTLS 证书，应建立独立的跟踪表，明确到期时间、负责人与轮换窗口。建议将证书到期告警分级：≤ 30 天 warning，≤ 7 天 critical，≤ 1 天 page。

### 2.3 节点补丁与生命周期失控

- **风险**: OS 内核漏洞长期未修，或节点替换导致有状态服务中断。
- **缓解**:
  ```bash
  # 使用 Karpenter 时设置节点过期策略
  kubectl patch nodepool default --type merge -p '{"spec":{"disruption":{"expireAfter":"720h"}}}'
  # 手动维护节点
  kubectl drain <node> --ignore-daemonsets --delete-emptydir-data --pod-selector='app!=critical-db'
  ```
- **配置要点**: 配合 EKS Node Update / GKE Node Auto-Repair / ACK 节点池镜像升级策略。制定节点维护窗口，优先采用不可变基础设施（cattle node）方式替换节点，而非原地升级。对有状态工作负载，应使用 PodDisruptionBudget、拓扑分布约束和持久卷快照确保节点替换期间的数据可用性。所有节点替换操作必须通过变更管理流程审批，并在维护前通知受影响业务方。

### 2.4 平台级 Secret 泄露

- **风险**: Git 仓库误提交 Secret、第三方 scaler 凭据明文存储。
- **缓解**:
  ```bash
  # 验证 Sealed Secrets / External Secrets 状态
  kubectl get sealedsecrets -A
  kubectl get externalsecrets -n platform
  # 禁止直接使用 opaque Secret 存储云凭据
  kubectl get secrets -A --field-selector=type=Opaque -o custom-columns='NS:.metadata.namespace,NAME:.metadata.name' | grep -E 'aws|azure|gcp'
  ```
- **配置要点**: 使用 External Secrets Operator 对接 Vault / AWS Secrets Manager / Azure Key Vault，启用 Secret 自动轮换。禁止在 Git 仓库中存储任何原始 Secret，CI/CD 流水线应集成 `git-secrets`、`truffleHog` 或 `gitleaks` 进行预提交扫描。对于 KEDA TriggerAuthentication、Argo CD 仓库凭据等关键 Secret，应启用最小权限原则，并定期审计 `kubectl get secret` 的访问日志。

### 2.5 多租户资源争抢与噪声邻居

- **风险**: 某团队 Pod 耗尽节点资源，影响平台核心服务。
- **缓解**:
  ```bash
  # 检查 LimitRange / ResourceQuota
  kubectl describe limitrange -n <team>
  kubectl describe resourcequota -n <team>
  # 启用 PriorityClass 与准入策略
  kubectl get priorityclass
  ```
- **配置要点**: 平台核心组件使用 `system-cluster-critical` PriorityClass；租户命名空间默认限制 CPU / 内存 requests 与 limits。建议为每个租户设置 ResourceQuota、LimitRange 和 NetworkPolicy 模板，通过 Kyverno、OPA Gatekeeper 或 ValidatingAdmissionPolicy 强制注入。对共享节点池，启用 cgroup v2 与 CPU / 内存 requests 严格匹配调度，防止资源超售导致的尾部延迟。

---

## 三、日常运维操作

日常运维应遵循"先观察、再确认、后变更"的原则，所有变更尽量通过 GitOps 发起，并保留审计痕迹。以下命令按巡检频率分为每日、每周、每月三类。

### 3.1 每日巡检

```bash
# 1. 核心命名空间 Pod 状态
kubectl get pods -n argocd -n keda -n karpenter -n cert-manager -n vault --show-labels

# 2. 平台组件资源使用
kubectl top pods -n argocd
kubectl top nodes -l node-role.kubernetes.io/platform=true

# 3. 事件检查（排除 Normal）
kubectl get events -A --field-selector type!=Normal --sort-by=.lastTimestamp | tail -50

# 4. 节点状态与污点
kubectl get nodes -o custom-columns='NAME:.metadata.name,STATUS:.status.conditions[-1].type,TAINTS:.spec.taints[*].key'
```

每日巡检的重点是发现异常事件、Pod 重启、资源使用突增等早期信号。建议将上述命令封装为 `platform-daily-check` 脚本，并通过 CronJob 在指定 SRE 工具节点上运行，输出推送至 Slack 或企业微信。

### 3.2 每周巡检

```bash
# 检查平台组件版本与升级可用性
helm list -n argocd
helm list -n keda
helm list -n karpenter

# 检查证书剩余有效期
kubectl get certificate -A -o jsonpath='{range .items[*]}{.metadata.namespace}{"/"}{.metadata.name}{"\t"}{.status.notAfter}{"\n"}{end}' | sort -k2

# 检查未使用的 ConfigMap / Secret 垃圾
kubectl get configmaps,secrets -A --field-selector=type=Opaque | wc -l
```

每周巡检关注版本漂移、证书健康与资源垃圾。建议将可升级组件整理为周报，由平台团队评审后纳入下周变更计划。

### 3.3 每月巡检

```bash
# 检查平台组件 RBAC 与权限收敛
kubectl auth can-i --list -n argocd | grep -E 'create|delete|update'

# 检查节点操作系统与内核版本
kubectl get nodes -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{.status.nodeInfo.osImage}{"\t"}{.status.nodeInfo.kernelVersion}{"\n"}{end}'

# 检查 NodePool / ScaledObject 资源上限
kubectl get nodepool -A -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{.spec.limits}{"\n"}{end}'
```

每月巡检重点关注权限收敛、节点生命周期与资源上限。建议将巡检结果与 FinOps 团队共享，识别长期低利用率节点池或过度配置的 ScaledObject。

### 3.4 GitOps 应用同步与回滚

```bash
# 查看应用健康状态
argocd app list
argocd app get <app-name>

# 手动同步并等待
argocd app sync <app-name> --prune --timeout 300

# 回滚到上一个版本
argocd app rollback <app-name> 0
```

在变更窗口内执行 GitOps 同步时，建议先使用 `argocd app diff <app-name>` 确认实际变更范围，避免误删资源。对于关键平台组件，回滚操作应经双人复核，并在变更管理工具中记录回滚原因。

### 3.5 自动扩缩容例行检查

```bash
# Karpenter
kubectl get nodeclaims -A
kubectl get nodepool -A -o wide

# KEDA
kubectl get scaledobject -A
kubectl get hpa -A -l app.kubernetes.io/managed-by=keda-operator
```

扩缩容例行检查应结合业务负载曲线进行。重点关注 NodePool 是否接近 limits、ScaledObject 是否触发 maxReplicas、以及 HPA 指标是否存在异常波动。

### 3.6 平台级证书与 Secret 轮换

```bash
# 触发 cert-manager 证书轮换
kubectl cert-manager renew --namespace=platform --all

# 滚动重启依赖该 Secret 的工作负载
kubectl rollout restart deploy/<workload> -n <ns>
```

Secret 轮换应尽量在业务低峰期执行，并预先在 staging 环境验证依赖工作负载的滚动重启行为。对于无法热加载的组件，应提前通知业务方并准备维护窗口。

---

## 四、故障排查速查

以下速查表覆盖平台工程域最常见的故障场景。每一行均按照"现象 → 可能根因 → 确认命令 → 处置"的结构组织，便于值班工程师在事故发生时快速定位与止血。

| 现象 | 可能根因 | 确认命令 | 处置 |
|:---|:---|:---|:---|
| Argo CD 应用长期处于 `Unknown` | 应用控制器分片不均或 Redis 失联 | `kubectl logs -n argocd deploy/argocd-application-controller` | 重启控制器分片 leader；检查 Redis HA |
| `ScaledObject` 状态 `False` | TriggerAuthentication 缺失或凭据过期 | `kubectl describe scaledobject <name> -n <ns>` | 更新 Secret / TriggerAuthentication，重新创建 ScaledObject |
| Karpenter 无法创建 NodeClaim | IAM / 子网 / 安全组标签不匹配 | `kubectl logs -n karpenter deploy/karpenter-controller` | 检查 EC2NodeClass selector 与云资源标签一致性 |
| 平台 Pod 被频繁驱逐 | 节点资源压力或污点漂移 | `kubectl describe node <node>` | 扩容节点池；调整 Pod requests / QoS |
| cert-manager 订单 stuck | ACME 验证失败或 DNS 配置错误 | `kubectl describe challenge -n <ns>` | 修复 DNS 记录；清理 challenge 让 cert-manager 重建 |
| 租户应用无法拉取镜像 | 镜像仓库凭据缺失或过期 | `kubectl get secret regcred -n <ns>` | 更新 imagePullSecret 或启用 ECR credential helper |
| 平台 API 响应慢或超时 | API server 负载高或 etcd 延迟高 | `kubectl top node -l node-role.kubernetes.io/control-plane` / `etcdctl endpoint health` | 扩容控制平面节点；检查 etcd 磁盘 I/O 与碎片 |
| GitOps 同步导致资源被误删 | 启用 prune 且仓库路径配置错误 | `argocd app diff <app-name>` | 立即禁用 auto-sync 与 prune，回滚应用定义 |
| KEDA 缩容过慢导致资源浪费 | stabilizationWindowSeconds 过长 | `kubectl get hpa <name> -o yaml` | 调整缩容稳定窗口与策略 |
| 多租户命名空间突破配额 | ResourceQuota 配置遗漏或过大 | `kubectl describe resourcequota -n <team>` | 收紧配额，启用准入策略自动注入默认配额 |

排查时应优先使用 `kubectl get events -A --sort-by=.lastTimestamp` 获取集群级事件上下文，再结合具体组件日志深入定位。对于涉及业务影响的事故，应在止血后立即创建事故工单，记录时间线、根因假设与已采取措施。

---

## 五、与其他域的协作边界

平台工程域处于承上启下的位置，需与以下域明确职责边界与协作接口。清晰的边界能够避免"都觉得归对方管"的灰色地带，提升事故响应效率。

- **与 [[domain-08-release-change-management/README.md|发布变更管理]] 协作**：平台团队负责 GitOps 控制器、镜像仓库、Secret 同步等基础设施的高可用；变更管理团队负责应用发布策略、灰度与回滚流程。
- **与 [[domain-05-security-compliance/README.md|安全合规]] 协作**：平台团队落地网络策略、Secret 管理、准入策略与 Pod Security Standards；安全合规团队制定策略基线、审计与事件响应标准。
- **与 [[domain-06-observability/README.md|可观测性]] 协作**：平台团队暴露平台组件指标、日志与 trace；可观测性团队负责 SLO/SLI 体系、告警路由与长期存储。
- **与 [[domain-09-reliability-engineering/README.md|可靠性工程]] 协作**：平台团队维护节点生命周期、自动扩缩容与证书轮换；可靠性团队负责 RTO/RPO 设计、灾备演练与混沌工程。
- **与 [[domain-11-production-operations/README.md|生产运维]] 协作**：平台团队提供 IDP 自服务能力与运维工具链；生产运维团队负责值班、工单、FinOps 与事故沟通。

除上述协作关系外，平台工程域还应与 [[domain-12-cloud-providers/README.md|云服务商]] 域保持同步，及时了解托管 Kubernetes 的新特性、版本生命周期与已知问题；与 [[domain-13-container-runtime/README.md|容器运行时]] 域协同推进镜像安全扫描、运行时升级与节点镜像标准化。

---

## 六、推荐阅读

### 同域关键文档

- [[domain-07-platform-engineering/99-karpenter-node-autoscaling-guide.md|Karpenter 节点自动扩展实践指南]]
- [[domain-07-platform-engineering/99-keda-event-driven-autoscaling-guide.md|KEDA 事件驱动自动缩放实践指南]]
- [[domain-07-platform-engineering/12-automated-operations-toolchain.md|自动化运维工具链]]
- [[domain-07-platform-engineering/operate/06-monitoring-alerting-system.md|监控告警体系]]

### 相关域核心文档

- [[domain-08-release-change-management/README.md|发布与变更管理]]
- [[domain-05-security-compliance/README.md|安全合规]]
- [[domain-09-reliability-engineering/README.md|可靠性工程]]
- [[domain-06-observability/README.md|可观测性]]

---

*本指南作为平台工程域生产就绪的入口文档，建议与域内自动扩缩容、GitOps、监控告警等专题文档配合使用，并每季度根据实际演练结果更新检查项。*
