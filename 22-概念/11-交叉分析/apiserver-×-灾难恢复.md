---
title: apiserver × 灾难恢复
summary: apiserver × 灾难恢复：apiserver与灾难恢复是Kubernetes生产环境中的两个关键维度。理解它们之间的交互关系对于构建稳定、可观测的集群至关重要。
category: synthesis
tags:
- cross-domain
- cluster
- reliability
tier: supporting
created: '2026-05-23'
updated: '2026-05-23'
last_updated: 2026-07
relationships:
- target: '[[23-实体/08-交付与制品/helm.md]]'
  type: uses
- target: '[[23-实体/07-可观测性/prometheus.md]]'
  type: uses
- target: '[[23-实体/08-交付与制品/argocd.md]]'
  type: related_to
status: reviewed
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。

# apiserver × 灾难恢复

## 概述
apiserver 是 Kubernetes 灾难恢复的核心枢纽——当集群发生灾难性故障（etcd 数据损坏、控制面节点全部丢失、区域级故障）时，恢复过程围绕 apiserver 重建展开。灾难恢复有两层含义：apiserver 自身从故障中恢复（如从 etcd 快照重建），以及通过 apiserver 恢复业务资源（如 Velero restore）。理解 apiserver 在灾难恢复链路中的角色对于制定有效的 RTO/RPO 策略至关重要。

## 技术关联机制

1. **apiserver 灾难场景分类**：
   - **etcd 数据损坏/丢失**：apiserver 依赖 etcd 存储所有集群状态。etcd 数据丢失意味着所有 Deployment/Service/Secret 等资源定义消失，apiserver 虽然运行但返回空数据。
   - **apiserver 进程崩溃**：apiserver Pod/进程异常退出，所有 kubectl 命令失败，Controller Manager 无法 watch 资源。但已运行的 Pod 和 kube-proxy 规则不受影响（数据面仍工作）。
   - **控制面节点全部丢失**：apiserver、etcd、scheduler、controller-manager 全部不可用。数据面（Pod 运行和流量转发）在短期内仍可工作，但无法管理。

2. **etcd 快照恢复流程**：在新的/修复的控制面节点上，停止 apiserver → 使用 `etcdctl snapshot restore` 从快照恢复 etcd 数据 → 启动 etcd → 启动 apiserver。apiserver 启动后从 etcd 读取所有资源定义，Controller Manager 开始 reconcile，恢复集群到快照时间点的状态。这个过程是灾难恢复中最关键、最高风险的操作。

3. **Velero 恢复流程**：Velero Restore Controller 通过 apiserver 逐个 apply 备份中的资源。这要求目标集群的 apiserver 已正常运行。Velero 恢复是 Namespace 级或资源级的精细化恢复，适用于"误删资源"场景，而非"etcd 完全丢失"场景。

4. **apiserver 恢复后的集群状态同步**：apiserver 恢复后，各 Controller 通过 informer 重新 list/watch 资源，对比期望状态和实际状态触发 reconcile。这个过程可能产生大量 API 请求（relist 风暴），需要关注 apiserver 性能。

## 实践场景

- **etcd 误操作恢复**：运维人员误执行了 `kubectl delete namespace` 删除了关键 Namespace，通过 Velero 从最近的备份中恢复该 Namespace 的所有资源
- **控制面区域故障**：生产集群所在 AZ 发生故障，通过跨区域 etcd 快照在新 AZ 重建控制面，恢复全部集群状态
- **证书过期导致 apiserver 不可用**：集群 CA/ apiserver 证书过期后所有 API 调用失败，需要通过节点 SSH 手动更新证书并重启 apiserver
- **rancher/managed Kubernetes 恢复**：云厂商托管集群（如 EKS/GKE）的控制面由云厂商管理，业务侧通过 Velero 做 Namespace 级备份恢复

## 常见问题

### 问题1：etcd 快照恢复后 apiserver 无法正常启动
**症状**：etcd 恢复后启动 apiserver 报错或行为异常
**根因**：快照版本与当前 apiserver 版本不兼容；或 etcd 恢复的数据中包含旧证书/token
**修复**：确保 etcd 快照与 apiserver 版本匹配；恢复后重启所有控制面组件；必要时重新分发证书

### 问题2：Velero 恢复后部分资源状态不正确
**症状**：恢复后 Pod 无法启动、Service Endpoints 为空
**根因**：备份中不包含某些隐式资源（如 ServiceAccount token Secret）；或 CRD 在恢复时未先恢复导致 CR apply 失败
**修复**：确保 Velero 备份包含所有必要资源类型；恢复时使用 `--include-cluster-resources=true` 包含集群级资源

### 问题3：灾难恢复后 Pod 分布异常
**症状**：apiserver 恢复后大量 Pod 被重新调度，节点负载不均
**根因**：恢复过程中 kubelet 重新注册 Node，scheduler 触发大规模重调度
**修复**：恢复后逐步检查 Pod 分布；必要时使用 `kubectl cordon` 防止过度调度到少数节点；等待集群自动均衡

## 关键命令

```bash
# 🟢 检查 apiserver 健康状态
kubectl get --raw='/readyz?verbose'
kubectl get componentstatuses

# 🟢 检查 etcd 健康状态（在控制面节点执行）
ETCDCTL_API=3 etcdctl --endpoints=https://127.0.0.1:2379 \
  --cacert=<ca> --cert=<cert> --key=<key> endpoint health

# 🔴 从 etcd 快照恢复（高风险，需先停止 apiserver）
ETCDCTL_API=3 etcdctl snapshot restore /backup/etcd-snapshot.db \
  --data-dir=/var/lib/etcd-new

# 🟡 通过 Velero 恢复命名空间
velero restore create --from-backup <backup-name> --include-namespaces <ns>

# 🟢 验证恢复后集群状态
kubectl get nodes
kubectl get pods -A | grep -v Running
kubectl get cs

# 🟢 检查 apiserver 审计日志确认恢复操作
kubectl get events -A --sort-by=.lastTimestamp | tail -20
```

## 权衡取舍

| 维度 | apiserver 倾向 | 灾难恢复 倾向 | 权衡点 |
|------|---------------|-------------|--------|
| 备份方式 | API 备份精细但慢 | etcd 快照全量但快 | 灵活性 vs 恢复速度 |
| RTO 目标 | 长 RTO 降低日常负载 | 短 RTO 要求快速恢复 | 成本 vs 业务连续性 |
| 恢复粒度 | 资源级精准恢复 | 集群级整体恢复 | 精细度 vs 操作简便 |
| 备份存储 | 本地存储快速访问 | 异地存储保障区域容灾 | 访问速度 vs 容灾能力 |

## 最佳实践
1. 定期（如每日）执行 etcd 快照并将快照复制到异地存储（不同区域/不同云账号的 S3）
2. 同时配置 Velero Schedule 做 Namespace 级备份，支持精细化恢复
3. 每季度执行灾难恢复演练，验证 RTO/RPO 目标可达，并更新恢复 runbook
4. 为 apiserver 证书设置 60 天过期告警，避免证书过期导致的 "静默灾难"

## 工具推荐
- kubectl：基础诊断
- [[23-实体/08-交付与制品/helm.md|Helm]]/Kustomize：配置管理
- [[23-实体/07-可观测性/prometheus.md|Prometheus]]/Grafana：联合监控
- [[23-实体/08-交付与制品/argocd.md|ArgoCD]]：GitOps同步

## 相关概念
- apiserver
- 灾难恢复
## Related

- [[23-实体/02-K8s核心组件/kubernetes.md|Kubernetes (CNCF Graduated)]]
- [[23-实体/08-交付与制品/argo.md|Argo Workflows]]
- [[19-故障诊断/08-技能体系/skill-set/k8s-pvc-storage/DIALOGUE.md|DIALOGUE]]
- [[22-概念/11-交叉分析/etcd-×-PVC.md|etcd-×-PVC]]
- [[22-概念/11-交叉分析/apiserver-×-Pod诊断.md|apiserver-×-Pod诊断]]
- [[22-概念/11-交叉分析/etcd-×-灾难恢复.md|etcd-×-灾难恢复]]


<!-- risk-assessed -->
