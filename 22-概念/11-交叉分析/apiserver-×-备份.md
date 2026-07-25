---
title: apiserver × 备份
summary: apiserver × 备份：apiserver与备份是Kubernetes生产环境中的两个关键维度。理解它们之间的交互关系对于构建稳定、可观测的集群至关重要。
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

# apiserver × 备份

## 概述
Kubernetes 集群备份的核心是备份 etcd 中存储的所有资源对象，而 etcd 数据可以通过 apiserver 间接导出（Velero 方式）或直接通过 etcdctl 物理快照。apiserver 在备份链路中扮演双重角色：它是 Velero 等备份工具读取集群资源的数据源（通过 API list 所有资源），同时也是恢复时重建资源的入口（通过 API apply 恢复资源）。理解这条 API 驱动的备份/恢复链路对于制定可靠的灾难恢复策略至关重要。

## 技术关联机制

1. **两种备份范式**：
   - **etcd 物理快照**：直接在 etcd 层面执行 `etcdctl snapshot save`，备份 etcd 的完整数据文件。这种方式绕过 apiserver，速度最快，恢复时是全量覆盖式恢复（无法选择部分资源）。
   - **apiserver API 导出（Velero）**：Velero 通过 apiserver list 所有命名空间下的所有资源（Deployment/Service/ConfigMap/Secret/PVC...），将其序列化为 JSON 存储到对象存储。恢复时通过 apiserver apply 逐个恢复。这种方式可以选择性恢复，但依赖 apiserver 可用。

2. **Velero 的备份流程**：Velero Backup Controller 通过 apiserver 创建 Backup CR → Backup Controller 通过 apiserver list 目标 Namespace 的所有资源 → 对每个资源调用 apiserver GET 获取完整定义 → 同时通过 CSI Driver 创建 PV 快照 → 将资源 JSON 和快照元数据上传到 S3 → 更新 Backup CR 的 status。

3. **apiserver 的 RBAC 对备份的影响**：Velero 的 SA 需要集群级 get/list/watch 权限才能读取所有资源，以及 create/update 权限才能恢复资源。RBAC 配置不足会导致部分资源未被备份或恢复失败。

4. **Resource Hooks 与一致性**：Velero 支持在备份前后通过 apiserver 执行 pre/post hooks（如 `kubectl exec` 执行数据库 flush），保证应用层一致性。但这依赖 apiserver 和 Pod 的网络可达性。

## 实践场景

- **定期集群备份**：配置 Velero Schedule 每日备份所有 Namespace 资源 + PV 快照到 S3，保留 30 天
- **升级前快照**：集群大版本升级前执行 etcd snapshot + Velero backup 双保险
- **Namespace 迁移**：从 dev 集群备份特定 Namespace，在 staging 集群恢复，实现环境间资源迁移
- **资源级恢复**：误删除 ConfigMap/Secret 后，从最近的 Velero Backup 中选择性恢复单个资源

## 常见问题

### 问题1：Velero 备份部分资源失败
**症状**：Backup CR 的 status 显示 Phase: PartiallyFailed，部分资源未被备份
**根因**：Velero SA 的 RBAC 权限不足；或某些 CRD 对应的 CR 未安装时 list 报错
**修复**：检查 Velero SA 的 ClusterRole 权限是否覆盖所有资源类型；查看 Backup 的 Errors 字段

### 问题2：恢复时资源冲突
**症状**：Velero restore 报错 `resource already exists`
**根因**：目标集群中已存在同名资源且配置不同，Velero 默认不覆盖已存在资源
**修复**：使用 `--existing-resource-policy=update` 允许覆盖；或先手动清理冲突资源再恢复

### 问题3：etcd 快照恢复后 apiserver 行为异常
**症状**：从 etcd 快照恢复后集群组件报错或资源状态不一致
**根因**：etcd 快照恢复是全量覆盖，如果快照和当前集群的 cert/token 版本不一致，可能导致认证失败
**修复**：恢复 etcd 快照后需要重启所有控制面组件（apiserver/controller-manager/scheduler）；必要时重新分发证书

## 关键命令

```bash
# 🟢 查看 Velero 备份列表
kubectl get backups -n velero

# 🟢 查看备份详情（含错误信息）
kubectl describe backup <name> -n velero

# 🟡 创建按需备份
velero backup create <backup-name> --include-namespaces <ns>

# 🟢 创建 etcd 快照（直接在控制面节点执行）
ETCDCTL_API=3 etcdctl --endpoints=https://127.0.0.1:2379 \
  --cacert=<ca> --cert=<cert> --key=<key> \
  snapshot save /backup/etcd-snapshot-$(date +%Y%m%d).db

# 🟡 从 Velero 恢复
velero restore create --from-backup <backup-name> --include-namespaces <ns>

# 🟢 验证 etcd 快照完整性
ETCDCTL_API=3 etcdctl snapshot status /backup/etcd-snapshot.db
```

## 权衡取舍

| 维度 | apiserver 倾向 | 备份 倾向 | 权衡点 |
|------|---------------|---------|--------|
| 备份方式 | API 导出可选择但慢 | etcd 快照全量但快 | 灵活性 vs 速度 |
| 备份频率 | 低频减少 API 负载 | 高频缩短 RPO | 集群负载 vs 数据安全 |
| 恢复粒度 | 按资源/Namespace 精细恢复 | 全量恢复简单粗暴 | 精细度 vs 操作复杂度 |
| 存储成本 | 仅备份关键资源节省空间 | 全量备份保证完整性 | 成本 vs 完备性 |

## 最佳实践
1. 生产环境同时配置 etcd 定期快照（物理备份）和 Velero Schedule（API 级备份），互为补充
2. 定期（至少季度）执行恢复演练，验证备份的可用性和恢复流程的正确性
3. 为 Velero SA 配置集群级最小权限 RBAC，覆盖所有需要备份的资源类型
4. 将备份存储在异于集群基础设施的对象存储中（如不同 AWS 账号的 S3），防止区域性故障

## 工具推荐
- kubectl：基础诊断
- [[23-实体/08-交付与制品/helm.md|Helm]]/Kustomize：配置管理
- [[23-实体/07-可观测性/prometheus.md|Prometheus]]/Grafana：联合监控
- [[23-实体/08-交付与制品/argocd.md|ArgoCD]]：GitOps同步

## 相关概念
- apiserver
- 备份
## Related

- [[23-实体/02-K8s核心组件/kubernetes.md|Kubernetes (CNCF Graduated)]]
- [[23-实体/08-交付与制品/argo.md|Argo Workflows]]
- [[19-故障诊断/08-技能体系/skill-set/k8s-pvc-storage/DIALOGUE.md|DIALOGUE]]
- [[22-概念/11-交叉分析/etcd-×-PVC.md|etcd-×-PVC]]
- [[22-概念/11-交叉分析/apiserver-×-Pod诊断.md|apiserver-×-Pod诊断]]
- [[22-概念/11-交叉分析/etcd-×-灾难恢复.md|etcd-×-灾难恢复]]


<!-- risk-assessed -->
