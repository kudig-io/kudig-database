---
title: StatefulSet × PVC
summary: StatefulSet × PVC：StatefulSet与PVC是Kubernetes生产环境中的两个关键维度。理解它们之间的交互关系对于构建稳定、可观测的集群至关重要。
category: synthesis
tags:
- cross-domain
- workloads
- storage
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

# StatefulSet × PVC

## 概述
StatefulSet 与 PVC 的关系是 Kubernetes 有状态应用管理的核心。通过 `volumeClaimTemplates`，StatefulSet 为每个 Pod 副本自动创建和管理独立 PVC。每个 PVC 是对应 Pod 的私有存储，Pod 重建后重新绑定同一个 PVC。这种一一对应的稳定关系使得 StatefulSet 可以安全地管理数据库、消息队列等需要持久化数据的应用。理解 PVC 在 StatefulSet 生命周期中的行为对于排查存储问题和防止数据丢失至关重要。

## 技术关联机制

1. **volumeClaimTemplates 的自动创建**：StatefulSet Controller 在创建 Pod-N 时，从 `spec.volumeClaimTemplates` 模板渲染出 PVC（命名为 `<template-name>-<sts-name>-N`），通过 apiserver 创建 PVC 对象。CSI external-provisioner watch 到新 PVC 后动态供给 PV，PV Controller 完成 PVC-PV 绑定。整个流程对用户透明——只需在 StatefulSet YAML 中定义模板。

2. **PVC 的生命周期管理策略**：Kubernetes 1.27+ 支持 `spec.persistentVolumeClaimRetentionPolicy`，定义 PVC 在 StatefulSet 缩容和删除时的行为：
   - `whenScaled: Retain`（默认）：缩容时保留 PVC，再扩容时 Pod 重新挂载旧 PVC 恢复数据
   - `whenScaled: Delete`：缩容时自动删除 PVC（及 PV，取决于 reclaimPolicy），释放存储资源
   - `whenDeleted: Retain`（默认）：StatefulSet 删除时保留所有 PVC
   - `whenDeleted: Delete`：StatefulSet 删除时清理所有 PVC

3. **PVC 与 Pod 的稳定绑定**：PVC 名称包含 StatefulSet 名称和 Pod 序号（如 `data-mysql-0`），形成确定性命名。当 Pod-N 被删除重建时，Controller 检测到 PVC `data-mysql-N` 已存在且 Bound，直接让新 Pod 挂载该 PVC——不需要重新创建 PVC 或等待动态供给。这个机制确保了数据跨 Pod 重建的持久性。

4. **PVC 扩容**：StorageClass 支持 `allowVolumeExpansion: true` 时，修改 PVC 的 `spec.resources.requests.storage` 触发 CSI Driver 在线扩容底层 PV。StatefulSet 的所有 PVC 可以批量扩容（通过脚本循环 patch 每个 PVC）。扩容过程对 Pod 透明，不需要重启。

## 实践场景

- **数据库集群**：MySQL StatefulSet 通过 volumeClaimTemplates 为每个副本创建独立 PVC，Pod 重建后数据完整
- **消息队列**：Kafka StatefulSet 每个 Broker 的 PVC 存储消息日志，Pod 重建后恢复未消费消息
- **存储扩容**：数据增长后批量扩容 StatefulSet 所有 PVC 容量，在线完成无需停机
- **缩容保护**：StatefulSet 从 5 缩到 3 副本时保留 PVC，紧急扩容回 5 时数据自动恢复

## 常见问题

### 问题1：PVC 创建失败导致 Pod 卡在 Pending
**症状**：StatefulSet 扩容时新 Pod 持续 Pending，PVC 状态为 Pending
**根因**：StorageClass 的 provisioner 不可用；存储后端资源不足（如 EBS 配额超限）；StorageClass 不存在
**修复**：`kubectl describe pvc <name>` 查看 Events；检查 CSI provisioner Pod 状态；确认 StorageClass 配置正确

### 问题2：Pod 重建后挂载了空 PVC
**症状**：Pod 重建后数据丢失，挂载的 PVC 是空的
**根因**：PVC 被误删后重新创建；或 PV 的 reclaimPolicy 为 Delete 且 PVC 被删除后 PV 被回收
**修复**：检查 PVC 创建时间确认是否被重建；为 StorageClass 设置 `reclaimPolicy: Retain`；使用 VolumeSnapshot 定期备份

### 问题3：PVC 无法删除（Terminating 状态）
**症状**：缩容后 PVC 应被删除但卡在 Terminating
**根因**：PV 的 finalizer 阻止删除；或 CSI Driver 无法完成 PV 清理
**修复**：检查 PVC 的 finalizer；确认 CSI Driver 正常；必要时手动移除 finalizer（⚠️ 高风险）

## 关键命令

```bash
# 🟢 查看 StatefulSet 自动创建的 PVC
kubectl get pvc -n <ns> | grep <sts-name>

# 🟢 查看 PVC 与 Pod 的绑定关系
kubectl get pods -l app=<name> -n <ns> -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{.spec.volumes[*].persistentVolumeClaim.claimName}{"\n"}{end}'

# 🟢 查看 PVC 详细信息和 Events
kubectl describe pvc <template-name>-<sts-name>-0 -n <ns>

# 🟢 检查 PVC 保留策略
kubectl get sts <name> -n <ns> -o jsonpath='{.spec.persistentVolumeClaimRetentionPolicy}'

# 🟡 批量扩容 StatefulSet 所有 PVC
for i in $(seq 0 $(($(kubectl get sts <name> -n <ns> -o jsonpath='{.spec.replicas')-1))); do
  kubectl patch pvc <template-name>-<sts-name>-$i -n <ns> -p '{"spec":{"resources":{"requests":{"storage":"200Gi"}}}}'
done

# 🟢 监控 PVC 使用率
kubectl exec <sts-name>-0 -n <ns> -- df -h <mount-path>
```

## 权衡取舍

| 维度 | StatefulSet 倾向 | PVC 倾向 | 权衡点 |
|------|-----------------|---------|--------|
| PVC 数量 | 每副本独立 PVC | 大量 PVC 管理 etcd 存储 | 数据隔离 vs 管理开销 |
| 缩容策略 | 保留 PVC 保护数据 | 删除 PVC 释放资源 | 数据安全 vs 资源效率 |
| 创建时机 | Pod 创建时自动创建 | 可能延迟导致 Pod Pending | 自动化 vs 可靠性 |
| 扩容 | 在线扩容无停机 | 依赖 CSI Driver 能力 | 便利性 vs 兼容性 |

## 最佳实践
1. 为 StatefulSet 的 StorageClass 配置 `reclaimPolicy: Retain`，防止 PVC 删除导致 PV 数据丢失
2. 配置 `persistentVolumeClaimRetentionPolicy.whenScaled: Retain`（默认），确保缩容后数据可恢复
3. 使用 VolumeSnapshot 定期备份关键 PVC 数据，验证恢复流程
4. 监控每个 PVC 的使用率（`kubelet_volume_stats_used_bytes`），设置 80% 容量预警

## 工具推荐
- kubectl：基础诊断
- [[23-实体/08-交付与制品/helm.md|Helm]]/Kustomize：配置管理
- [[23-实体/07-可观测性/prometheus.md|Prometheus]]/Grafana：联合监控
- [[23-实体/08-交付与制品/argocd.md|ArgoCD]]：GitOps同步

## 相关概念
- [[StatefulSet]]
- PVC
## Related

- [[23-实体/02-K8s核心组件/kubernetes.md|Kubernetes (CNCF Graduated)]]
- [[23-实体/08-交付与制品/argo.md|Argo Workflows]]
- [[19-故障诊断/08-技能体系/skill-set/k8s-pvc-storage/DIALOGUE.md|DIALOGUE]]
- [[19-故障诊断/08-技能体系/skill-set/k8s-pvc-storage/DIALOGUE.md|DIALOGUE]]
- [[22-概念/11-交叉分析/etcd-×-PVC.md|etcd-×-PVC]]
- [[22-概念/11-交叉分析/apiserver-×-Pod诊断.md|apiserver-×-Pod诊断]]
- [[22-概念/11-交叉分析/Deployment-×-PVC.md|Deployment-×-PVC]]
- [[22-概念/11-交叉分析/apiserver-×-PVC.md|apiserver-×-PVC]]


<!-- risk-assessed -->
