---
title: StatefulSet × PV
summary: StatefulSet × PV：StatefulSet与PV是Kubernetes生产环境中的两个关键维度。理解它们之间的交互关系对于构建稳定、可观测的集群至关重要。
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

# StatefulSet × PV

## 概述
StatefulSet 是 Kubernetes 中使用 PV 的最核心工作负载——通过 `volumeClaimTemplates` 为每个副本自动创建独立 PVC 并绑定独立 PV。这是 StatefulSet 与 Deployment 在存储管理上的根本差异：StatefulSet 的每个 Pod 拥有独立的持久化存储，Pod 重建后重新挂载同一个 PV，数据不丢失。PV 与 Pod 序号的稳定绑定（pod-0 ↔ pvc-0 ↔ pv-0）是有状态应用可靠运行的基石。

## 技术关联机制

1. **volumeClaimTemplates 机制**：StatefulSet 的 `spec.volumeClaimTemplates` 定义 PVC 模板。StatefulSet Controller 在创建每个 Pod 时，按序号从模板创建独立 PVC（命名为 `<template-name>-<sts-name>-<ordinal>`）。每个 PVC 独立走 PV 动态供给或静态绑定流程。Pod-0 挂载 pvc-0，Pod-1 挂载 pvc-1，严格一一对应。

2. **PV 与 Pod 的稳定绑定**：当 Pod-0 被删除重建时（如节点故障后），新 Pod-0 仍然挂载原来的 pvc-0（因为 PVC 独立于 Pod 生命周期）。PVC 绑定的 PV 不变，数据完整保留。这个稳定的 Pod-PVC-PV 三元组绑定是有状态应用数据持久性的核心保证。

3. **扩缩容的 PV 行为**：
   - **扩容**（N → N+1）：Controller 为新副本创建 pvc-N，CSI Driver 动态供给 pv-N。新 Pod 挂载全新空存储。
   - **缩容**（N → N-1）：Controller 删除 Pod-N 但保留 pvc-N 和 pv-N（默认行为）。重新扩容到 N 时，Pod-N 重新挂载原来的 pvc-N，数据恢复。

4. **滚动更新与 PV 数据迁移**：StatefulSet 滚动更新时 Pod 被逐个重建（逆序），但 PVC/PV 不变——新版本 Pod 挂载旧版本使用的同一个 PV。这意味着 PV 中的数据需要向前/向后兼容新旧版本应用代码。如果数据库 schema 变更不兼容，可能导致新版本 Pod 无法启动。

## 实践场景

- **数据库集群**：MySQL/PostgreSQL StatefulSet 每个副本独立 PV，Pod 重建后数据不丢失
- **消息队列持久化**：Kafka StatefulSet 的每个 Broker 挂载独立 PV 存储消息日志
- **缩容后数据保留**：StatefulSet 从 5 副本缩到 3 副本，pvc-3/pvc-4 保留数据，扩容回 5 时自动恢复
- **存储扩容**：通过修改 PVC 的 storage requests 触发 CSI Driver 在线扩容 PV 容量

## 常见问题

### 问题1：StatefulSet Pod 重建后挂载了错误的 PV
**症状**：Pod-0 重建后挂载了 Pod-1 的 PV，数据不一致
**根因**：PV-PVC 绑定关系在 etcd 恢复后错乱；或手动修改了 PVC 的 volumeName
**修复**：检查每个 PVC 的 volumeName 和 PV 的 claimRef 确保对应关系正确；必要时手动修复

### 问题2：缩容后 PVC 未删除，扩容时重新挂载旧数据
**症状**：缩容后再扩容，新 Pod 读取到了旧副本的数据
**根因**：StatefulSet 默认保留缩容副本的 PVC（`persistentVolumeClaimRetentionPolicy` 默认为 Retain）
**修复**：这是预期行为（数据保护）；如果需要全新数据，配置 `persistentVolumeClaimRetentionPolicy.whenScaled: Delete`

### 问题3：PV 容量不足导致 Pod 启动失败
**症状**：StatefulSet 新 Pod 因 PV 空间不足无法写入数据
**根因**：volumeClaimTemplates 中请求的容量太小，或存储后端配额不足
**修复**：在线扩容 PVC（如果 StorageClass 支持 allowVolumeExpansion）；调整 volumeClaimTemplates 的 storage 请求

## 关键命令

```bash
# 🟢 查看 StatefulSet 的 volumeClaimTemplates
kubectl get sts <name> -n <ns> -o jsonpath='{.spec.volumeClaimTemplates}'

# 🟢 查看 StatefulSet 自动创建的 PVC（每个副本一个）
kubectl get pvc -n <ns> | grep <sts-name>

# 🟢 查看每个 Pod 挂载的 PV
kubectl get pods -l app=<name> -n <ns> -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{.spec.volumes[*].persistentVolumeClaim.claimName}{"\n"}{end}'

# 🟢 查看 PV 的绑定状态和回收策略
kubectl get pv -o custom-columns=NAME:.metadata.name,CLAIM:.spec.claimRef.name,RECLAIM:.spec.persistentVolumeReclaimPolicy,SIZE:.spec.capacity.storage

# 🟡 在线扩容 StatefulSet PVC
for i in 0 1 2; do
  kubectl patch pvc <template-name>-<sts-name>-$i -n <ns> -p '{"spec":{"resources":{"requests":{"storage":"200Gi"}}}}'
done

# 🟢 查看 PVC 保留策略
kubectl get sts <name> -n <ns> -o jsonpath='{.spec.persistentVolumeClaimRetentionPolicy}'
```

## 权衡取舍

| 维度 | StatefulSet 倾向 | PV 倾向 | 权衡点 |
|------|-----------------|---------|--------|
| 存储独立性 | 每副本独立 PV | 需要更多 PV 资源 | 数据隔离 vs 资源成本 |
| 缩容策略 | 保留 PVC 保护数据 | Delete PVC 释放存储 | 数据安全 vs 资源效率 |
| 存储类型 | RWO 独占挂载 | RWX 共享但冲突风险 | 数据安全 vs 共享能力 |
| 扩容 | 扩容创建新空 PV | 缩容保留旧 PV | 自动化 vs 数据可控 |

## 最佳实践
1. 为 StatefulSet 的 StorageClass 配置 `reclaimPolicy: Retain`，防止 PVC/Pod 删除导致数据丢失
2. 配置 `persistentVolumeClaimRetentionPolicy.whenScaled: Retain`（默认），确保缩容后 PVC 数据保留可恢复
3. 使用 VolumeSnapshot 定期备份关键 PV 数据，确保灾难恢复能力
4. 监控 PV 使用率（`kubelet_volume_stats_used_bytes`），设置容量预警避免存储耗尽

## 工具推荐
- kubectl：基础诊断
- [[23-实体/08-交付与制品/helm.md|Helm]]/Kustomize：配置管理
- [[23-实体/07-可观测性/prometheus.md|Prometheus]]/Grafana：联合监控
- [[23-实体/08-交付与制品/argocd.md|ArgoCD]]：GitOps同步

## 相关概念
- [[statefulset|StatefulSet]]
- PV
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
