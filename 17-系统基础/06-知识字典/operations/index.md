---
title: 生产运维知识词典
description: 涵盖 Kubernetes 生产运维全领域的完整术语体系，包括 GitOps、混沌工程、备份恢复、SRE、FinOps、变更管理等
summary: 生产运维领域词典，覆盖 ArgoCD、Flux、Velero、Chaos Mesh、SLO、FinOps、节点运维等核心概念
category: dictionary
tags:
- dictionary
- operations
- gitops
- sre
- chaos-engineering
- backup
tier: core
created: '2026-07-02'
last_updated: 2026-07
difficulty: intermediate
audience:
- SRE
- 平台工程师
- 运维工程师
---

# 生产运维知识词典（Operations）

> 本词典覆盖 Kubernetes 生产运维领域的核心术语、技术组件及工程实践，是 SRE 和运维工程师保障生产环境稳定性的权威参考。

## 领域概述

生产运维是保障 Kubernetes 集群稳定运行的核心学科，包括：

- **持续交付**：GitOps、渐进式发布、回滚
- **可靠性工程**：SLO、混沌工程、故障演练
- **数据保护**：备份、灾难恢复、快照
- **节点运维**：升级、维护、扩缩容
- **成本优化**：FinOps、资源右调、绿色计算
- **事件管理**：告警、Runbook、复盘

## 核心术语定义

### GitOps 与持续交付

| 术语 | 定义 | 典型实现 |
|------|------|----------|
| GitOps | Git 作为单一事实来源的运维模式 | ArgoCD/Flux |
| ArgoCD | 声明式 GitOps 持续交付工具 | CNCF 毕业 |
| Flux | GitOps 工具集，K8s 原生 | CNCF 毕业 |
| Tekton | 云原生 CI/CD 流水线框架 | CD Foundation |
| PipeCD | 统一持续交付平台 | Cybozu |
| Flagger | 渐进式交付 Operator | 金丝雀/蓝绿/A-B |
| Rollback | 回滚到之前版本 | kubectl rollout undo |
| Rolling Update | 滚动更新，逐步替换 Pod | Deployment 默认策略 |

### 可靠性工程 (SRE)

| 术语 | 定义 | 关键特性 |
|------|------|----------|
| SLI | 服务水平指标 | 量化度量（延迟/可用性） |
| SLO | 服务水平目标 | 可接受的服务质量 |
| SLA | 服务水平协议 | 合同约束，含赔偿 |
| Error Budget | 错误预算 = 1 - SLO | 允许的最大不可用 |
| SRE Maturity | SRE 成熟度模型 | 从救火到预防 |
| Incident Management | 事件管理流程 | 检测→响应→恢复→复盘 |
| Runbook | 运维操作手册 | 标准化故障处理 |
| PDB | Pod 中断预算 | 保护最小可用数 |

### 混沌工程

| 术语 | 定义 | 典型实现 |
|------|------|----------|
| Chaos Engineering | 主动注入故障验证系统韧性 | 原则驱动 |
| Chaos Mesh | CNCF 混沌工程平台 | PingCAP 开源 |
| Litmus | CNCF 混沌工程框架 | 实验即代码 |
| ChaosBlade | 阿里开源混沌工程工具 | 多平台支持 |
| Krkn | Red Hat 混沌工程工具 | 大规模集群 |
| KubeBurner | 负载生成与压力测试 | 性能基准 |

### 备份与灾难恢复

| 术语 | 定义 | 典型实现 |
|------|------|----------|
| Velero | K8s 资源备份与迁移工具 | 资源 + PV 快照 |
| K8Up | K8s 备份 Operator | Restic + S3 |
| Backup/DR | 备份与灾难恢复策略 | 3-2-1 原则 |
| etcd Backup | etcd 数据备份 | etcdctl snapshot |

### 节点运维

| 术语 | 定义 | 关键特性 |
|------|------|----------|
| Cordon | 标记节点不可调度 | 新 Pod 不调度到该节点 |
| Uncordon | 恢复节点可调度 | 与 Cordon 相反 |
| Drain | 驱逐节点上所有 Pod | 维护前操作 |
| Node Autoscaling | 节点自动扩缩容 | Cluster Autoscaler/Karpenter |
| Node Shutdown | 节点优雅关机 | 处理 Pod 驱逐 |
| Kured | 节点重启守护进程 | 内核更新后自动重启 |
| Upgrade | 集群版本升级 | 控制平面 + 节点 |

### 成本与绿色计算

| 术语 | 定义 | 典型实现 |
|------|------|----------|
| FinOps | 云财务运营，成本优化 | 可视化 + 优化 |
| GreenOps | 绿色计算，碳足迹优化 | Kepler |
| Cloud Custodian | 云资源治理策略引擎 | 规则驱动 |
| Capacity Planning | 容量规划与预测 | 资源趋势分析 |

### 运维工具与平台

| 术语 | 定义 | 典型实现 |
|------|------|----------|
| K8sGPT | AI 驱动的 K8s 诊断 | 自然语言诊断 |
| HolmesGPT | AI 故障排查助手 | Robusta |
| KuberHealthy | 健康检查合成监控 | 主动探测 |
| Meshery | 服务网格管理平台 | 多网格支持 |
| Kubean | 集群生命周期管理 | 道客开源 |
| Tinkerbell | 裸金属服务器配置 | Equinix 开源 |
| Konveyor | 应用迁移平台 | Red Hat |
| cert-manager | 证书自动管理 | Let's Encrypt |
| Swap Memory | 交换内存管理 | K8s 1.30+ 支持 |

## 技术组件索引

### GitOps 与交付类

- [[17-系统基础/06-知识字典/operations/gitops.md|GitOps（原则与实践）]]
- [[17-系统基础/06-知识字典/operations/argo.md|ArgoCD]]
- [[17-系统基础/06-知识字典/operations/flux.md|Flux]]
- [[17-系统基础/06-知识字典/operations/tekton.md|Tekton（CI/CD）]]
- [[17-系统基础/06-知识字典/operations/pipecd.md|PipeCD]]
- [[17-系统基础/06-知识字典/operations/flagger.md|Flagger（渐进式交付）]]
- [[17-系统基础/06-知识字典/operations/rolling-update.md|滚动更新]]
- [[17-系统基础/06-知识字典/operations/rollback.md|回滚]]
- [[17-系统基础/06-知识字典/operations/change-management-release.md|变更管理]]

### SRE 与可靠性类

- [[17-系统基础/06-知识字典/operations/sli-slo-sla-engineering.md|SLI/SLO/SLA 工程]]
- [[17-系统基础/06-知识字典/operations/sre-maturity-model.md|SRE 成熟度模型]]
- [[17-系统基础/06-知识字典/operations/incident-management-runbooks.md|事件管理与 Runbook]]
- [[17-系统基础/06-知识字典/operations/production-troubleshooting-playbook.md|生产排障手册]]
- [[17-系统基础/06-知识字典/operations/failure-patterns-analysis.md|故障模式分析]]
- [[17-系统基础/06-知识字典/operations/pdb.md|Pod Disruption Budget]]
- [[17-系统基础/06-知识字典/operations/operations-best-practices.md|运维最佳实践]]
- [[17-系统基础/06-知识字典/operations/enterprise-ops-practices.md|企业运维实践]]

### 混沌工程类

- [[17-系统基础/06-知识字典/operations/chaos-engineering.md|混沌工程（原则）]]
- [[17-系统基础/06-知识字典/operations/chaos-mesh.md|Chaos Mesh]]
- [[17-系统基础/06-知识字典/operations/litmus.md|Litmus]]
- [[17-系统基础/06-知识字典/operations/chaosblade.md|ChaosBlade]]
- [[17-系统基础/06-知识字典/operations/krkn.md|Krkn]]
- [[17-系统基础/06-知识字典/operations/kube-burner.md|KubeBurner（压力测试）]]

### 备份与恢复类

- [[17-系统基础/06-知识字典/operations/backup-disaster-recovery.md|备份与灾难恢复]]
- [[17-系统基础/06-知识字典/operations/velero.md|Velero]]
- [[17-系统基础/06-知识字典/operations/k8up.md|K8Up]]
- [[17-系统基础/06-知识字典/operations/stateful-services-operations.md|有状态服务运维]]

### 节点运维类

- [[17-系统基础/06-知识字典/operations/cordon.md|Cordon]]
- [[17-系统基础/06-知识字典/operations/uncordon.md|Uncordon]]
- [[17-系统基础/06-知识字典/operations/drain.md|Drain]]
- [[17-系统基础/06-知识字典/operations/node-autoscaling.md|节点自动扩缩]]
- [[17-系统基础/06-知识字典/operations/node-shutdowns.md|节点关机]]
- [[17-系统基础/06-知识字典/operations/kured.md|Kured（重启守护）]]
- [[17-系统基础/06-知识字典/operations/upgrade.md|集群升级]]
- [[17-系统基础/06-知识字典/operations/scale.md|扩缩容]]
- [[17-系统基础/06-知识字典/operations/swap-memory-management.md|Swap 内存管理]]

### 成本与治理类

- [[17-系统基础/06-知识字典/operations/finops-and-cost-optimization.md|FinOps 与成本优化]]
- [[17-系统基础/06-知识字典/operations/greenops-and-carbon-aware-computing.md|GreenOps 与绿色计算]]
- [[17-系统基础/06-知识字典/operations/cloud-custodian.md|Cloud Custodian]]
- [[17-系统基础/06-知识字典/operations/capacity-planning-forecasting.md|容量规划]]

### 工具与平台类

- [[17-系统基础/06-知识字典/operations/k8sgpt.md|K8sGPT（AI 诊断）]]
- [[17-系统基础/06-知识字典/operations/holmesgpt.md|HolmesGPT]]
- [[17-系统基础/06-知识字典/operations/kuberhealthy.md|KuberHealthy]]
- [[17-系统基础/06-知识字典/operations/meshery.md|Meshery]]
- [[17-系统基础/06-知识字典/operations/kubean.md|Kubean]]
- [[17-系统基础/06-知识字典/operations/tinkerbell.md|Tinkerbell]]
- [[17-系统基础/06-知识字典/operations/konveyor.md|Konveyor]]
- [[17-系统基础/06-知识字典/operations/cert-manager.md|cert-manager]]
- [[17-系统基础/06-知识字典/operations/certificates.md|证书管理]]
- [[17-系统基础/06-知识字典/operations/installing-addons.md|Addon 安装]]
- [[17-系统基础/06-知识字典/operations/performance-tuning-expert.md|性能调优]]

## 运维流程框架

### 事件响应流程

```
事件响应生命周期:

1. 检测 (Detection)
   └─ 告警触发 / 用户报告 / 主动发现

2. 分类 (Triage)
   └─ 确定严重级别 (P1-P4)
   └─ 指定事件负责人 (IC)

3. 响应 (Response)
   └─ 执行 Runbook / 回滚 / 扩容 / 降级

4. 恢复 (Recovery)
   └─ 验证服务恢复 / 监控观察期

5. 复盘 (Post-Mortem)
   └─ 时间线 / 根因 / 改进措施 / 无责文化
```

### 变更管理流程

```
变更管理:

1. 变更申请
   └─ 变更内容 / 影响范围 / 回滚方案

2. 风险评估
   └─ 低风险: 自动审批
   └─ 高风险: CAB 审批

3. 变更窗口
   └─ 业务低峰期执行
   └─ 冻结期禁止变更

4. 执行与验证
   └─ 灰度发布 / 金丝雀 / 蓝绿
   └─ 健康检查 / 监控观察

5. 回滚 (如需)
   └─ 自动回滚触发条件
   └─ 手动回滚流程
```

## 生产最佳实践

### 备份策略

1. **3-2-1 原则**：3 份副本、2 种介质、1 份异地
2. **etcd 备份**：每 30min 自动备份，保留 7 天
3. **PV 快照**：数据库每小时快照，保留 24h
4. **定期演练**：每季度验证备份可恢复性

### 混沌工程

1. **从生产环境外开始**：先在测试环境验证
2. **最小爆炸半径**：从小范围故障开始
3. **自动化实验**：定期自动执行混沌实验
4. **度量韧性**：记录 MTTR、错误率变化

### 节点维护

1. **Cordon → Drain → 维护 → Uncordon**
2. **PDB 保护**：确保 Drain 时不违反 PDB
3. **批量操作**：每次维护不超过 10% 节点
4. **内核更新**：使用 Kured 自动滚动重启

## 故障排查要点

| 故障现象 | 可能原因 | 排查方向 |
|----------|----------|----------|
| ArgoCD 同步失败 | 清单错误/权限不足 | 检查 ArgoCD UI、RBAC |
| Velero 备份失败 | 存储后端不可达/快照失败 | 检查 Velero 日志、存储连接 |
| 节点 NotReady | kubelet 异常/网络不通 | 检查节点状态、kubelet 日志 |
| Drain 卡住 | PDB 阻止/本地存储 | 检查 PDB、使用 --delete-emptydir-data |
| 升级失败 | 版本不兼容/插件冲突 | 检查升级日志、API 兼容性 |

## 学习路径

```
基础: kubectl 运维 → 滚动更新 → 备份恢复
进阶: GitOps (ArgoCD) → SLO 体系 → 混沌工程
高级: 多集群运维 → FinOps → 自动化运维平台
专家: AIOps → 自愈系统 → 全局流量调度
```

## 参考链接

- https://argo-cd.readthedocs.io/
- https://fluxcd.io/
- https://velero.io/
- https://chaos-mesh.org/
- https://litmuschaos.io/
- https://sre.google/sre-book/table-of-contents/

## Related

- [[17-系统基础/06-知识字典/platform-engineering/gitops-and-continuous-delivery.md|GitOps 与持续交付]]
- [[17-系统基础/06-知识字典/observability/alerting-and-slo-monitoring.md|告警与 SLO]]
- [[17-系统基础/06-知识字典/scheduling/cluster-autoscaler.md|Cluster Autoscaler]]
- [[17-系统基础/06-知识字典/reliability/chaos-engineering.md|混沌工程]]

## 常用运维命令速查

```bash
# === 节点维护 ===
# 标记节点不可调度
kubectl cordon my-node
# 驱逐 Pod
kubectl drain my-node --ignore-daemonsets --delete-emptydir-data --grace-period=60
# 恢复调度
kubectl uncordon my-node
# 查看节点状态
kubectl get nodes -o wide

# === 滚动更新 ===
# 查看更新状态
kubectl rollout status deployment/my-app
# 查看历史版本
kubectl rollout history deployment/my-app
# 回滚
kubectl rollout undo deployment/my-app
# 回滚到指定版本
kubectl rollout undo deployment/my-app --to-revision=3

# === 备份恢复 (Velero) ===
# 创建备份
velero backup create my-backup --include-namespaces=production
# 查看备份
velero backup get
# 恢复
velero restore create --from-backup my-backup
# 定时备份
velero schedule create daily-backup --schedule="0 2 * * *" --include-namespaces=production

# === ArgoCD ===
# 查看应用状态
argocd app list
argocd app get my-app
# 同步
argocd app sync my-app
# 回滚
argocd app rollback my-app

# === 混沌工程 (Chaos Mesh) ===
# 查看实验
kubectl get chaos -A
# 创建 Pod Kill 实验
kubectl apply -f pod-kill.yaml
# 删除实验
kubectl delete chaos pod-kill-experiment

# === etcd 备份 ===
# 创建快照
ETCDCTL_API=3 etcdctl snapshot save /backup/etcd-$(date +%Y%m%d).db \
  --endpoints=https://127.0.0.1:2379 \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/server.crt \
  --key=/etc/kubernetes/pki/etcd/server.key
# 查看快照状态
ETCDCTL_API=3 etcdctl snapshot status /backup/etcd.db --write-out=table

# === 集群升级 ===
# 升级控制平面
kubeadm upgrade plan
kubeadm upgrade apply v1.31.0
# 升级节点
kubeadm upgrade node
```

## 生产案例研究

### 案例：混沌工程发现单点故障

**背景：** 某公司核心服务声称“高可用”，但从未验证。

**混沌实验：**
1. Pod Kill: 随机删除 Pod → 发现服务中断 30s（HPA 未及时扩容）
2. Node Drain: 模拟节点维护 → 发现 StatefulSet 重新调度需 5min
3. 网络分区: 模拟 AZ 故障 → 发现数据库主从切换失败

**改进措施：**
1. HPA 设置 minReplicas=3，避免缩容到单点
2. StatefulSet 使用 WaitForFirstConsumer，加快重调度
3. 数据库使用 Operator 自动故障转移
4. 定期混沌实验纳入 CI/CD

## 常见问题 FAQ

**Q1: Velero 和 etcd 备份有什么区别？**

A: 
- Velero: 备份 K8s 资源（Deployment/Service/ConfigMap）+ PV 快照，用于迁移/恢复
- etcd 备份: 备份整个集群状态（所有资源），用于灾难恢复
建议：两者都做。etcd 备份用于集群级灾难，Velero 用于应用级备份/迁移。

**Q2: 混沌工程会不会影响生产？**

A: 会，所以要控制爆炸半径：
1. 从测试环境开始
2. 生产环境小范围实验（单个 Pod、非核心服务）
3. 设置实验超时，自动回滚
4. 业务低峰期执行
5. 有完整的监控和快速恢复能力

**Q3: 集群升级的最佳实践？**

A: 
1. 阅读 Release Notes，检查 API 废弃
2. 测试环境先升级验证
3. 备份 etcd
4. 升级控制平面（一次一个小版本）
5. 滚动升级节点（Cordon → Drain → Upgrade → Uncordon）
6. 升级 Addon（CoreDNS/kube-proxy 等）
7. 验证集群健康

**Q4: SLO 怎么定？**

A: 
1. 识别关键用户旅程（登录、下单、支付）
2. 选择 SLI（成功率、延迟 P99）
3. 基于历史数据设定目标（不要一开始就 99.99%）
4. 从 99.5% 开始，逐步提高
5. 错误预算用于平衡发布速度与稳定性

**Q5: FinOps 如何快速见效？**

A: 快速收益：
1. 识别空闲资源（CPU <10% 的 Pod）→ 缩容
2. 使用 Spot/竞价实例（无状态服务）→ 节省 60-80%
3. 右调 requests（基于实际使用）→ 节省 20-40%
4. 自动缩容到零（Knative/KEDA）→ 非工作时间节省
5. 存储分层（热/温/冷）→ 节省 50%+

## 缩略语表

| 缩写 | 全称 | 说明 |
|------|------|------|
| SRE | Site Reliability Engineering | 站点可靠性工程 |
| SLI | Service Level Indicator | 服务水平指标 |
| SLO | Service Level Objective | 服务水平目标 |
| SLA | Service Level Agreement | 服务水平协议 |
| PDB | Pod Disruption Budget | Pod 中断预算 |
| MTTR | Mean Time To Recovery | 平均恢复时间 |
| MTTD | Mean Time To Detection | 平均发现时间 |
| IC | Incident Commander | 事件指挥官 |
| CAB | Change Advisory Board | 变更顾问委员会 |
| RPO | Recovery Point Objective | 恢复点目标 |
| RTO | Recovery Time Objective | 恢复时间目标 |

## 版本兼容性矩阵

| 组件 | K8s 1.28 | K8s 1.29 | K8s 1.30 | K8s 1.31 |
|------|-----------|-----------|-----------|----------|
| ArgoCD | v2.9+ | v2.10+ | v2.11+ | v2.12+ |
| Flux | v2.1+ | v2.2+ | v2.3+ | v2.4+ |
| Velero | v1.12+ | v1.13+ | v1.14+ | v1.15+ |
| Chaos Mesh | v2.6+ | v2.7+ | v2.8+ | v2.9+ |
| cert-manager | v1.13+ | v1.14+ | v1.15+ | v1.16+ |
| Tekton | v0.53+ | v0.56+ | v0.59+ | v0.62+ |

## 运维成熟度检查清单

| 级别 | 检查项 | 状态 |
|------|--------|------|
| L1 基础 | 所有变更走 GitOps 流程 | ☐ |
| L1 基础 | etcd 自动备份已配置 | ☐ |
| L1 基础 | 节点维护有标准流程 | ☐ |
| L2 进阶 | SLO 已定义并可视化 | ☐ |
| L2 进阶 | 告警有对应 Runbook | ☐ |
| L2 进阶 | 定期备份恢复演练 | ☐ |
| L3 高级 | 混沌工程定期执行 | ☐ |
| L3 高级 | 自动化故障转移 | ☐ |
| L3 高级 | 成本可视化与优化 | ☐ |
| L4 专家 | AIOps 异常检测 | ☐ |
| L4 专家 | 自愈系统 | ☐ |
| L4 专家 | 全局流量调度 | ☐ |

## 紧急响应快速参考

| 场景 | 紧急操作 |
|------|----------|
| 服务不可用 | `kubectl rollout undo deployment/my-app` |
| 节点故障 | `kubectl cordon node && kubectl drain node` |
| 资源耗尽 | `kubectl scale deployment/my-app --replicas=0` |
| 配置错误 | `kubectl rollout undo` 或 ArgoCD 回滚 |
| 证书过期 | `kubectl cert-manager renew --all` |
| etcd 异常 | 从快照恢复: `etcdctl snapshot restore` |

## 运维工具链推荐

| 场景 | 推荐工具 | 备选 |
|------|----------|------|
| GitOps | ArgoCD | Flux |
| CI/CD | Tekton + ArgoCD | Jenkins X |
| 备份 | Velero | K8Up |
| 混沌工程 | Chaos Mesh | Litmus |
| 证书 | cert-manager | Vault PKI |
| 监控 | Prometheus + Grafana | Datadog |
| 日志 | Loki + Fluent Bit | ELK |
| 追踪 | Tempo + OTel | Jaeger |
| AI 诊断 | K8sGPT | HolmesGPT |

