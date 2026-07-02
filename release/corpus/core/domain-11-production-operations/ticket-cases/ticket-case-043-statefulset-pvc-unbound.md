---
title: StatefulSet Pod 启动失败：PVC 未绑定
description: 专有云 ACK 集群因 StorageClass 扩容失败导致 StatefulSet Pod 的 PVC 未绑定、Pod 无法启动的工单闭环样本。
summary: 专有云 ACK 集群因 StorageClass 扩容失败导致 StatefulSet Pod 的 PVC 未绑定、Pod 无法启动的工单闭环样本。
category: domain-11-production-operations/ticket-case
tags:
- ack
- zyy
- statefulset
- pvc
- storage
- disk
- p1
tier: core
created: '2026-06-26T14:00:00+08:00'
updated: '2026-06-26T16:00:00+08:00'
incident_id: INC-2026-ACK-043
priority: P1
severity: high
affected_cluster: ack-zyy-prod-04
affected_namespace: middleware
ticket_type: 存储故障
skill_ref:
- '[[domain-04-storage-data/02-pvc-expansion-guide.md|PVC 扩容指南]]'
- '[[domain-10-troubleshooting-diagnostics/topic-fta/list/csi-fta.md|CSI 异常故障树分析]]'
fta_ref:
- '[[domain-10-troubleshooting-diagnostics/topic-fta/list/csi-fta.md|FTA: CSI 存储异常]]'
last_updated: 2026-06-26 16:00:00+08:00
duplicate_of: INC-2026-ACK-048
status: duplicate
duplication_reason: 与 "INC-2026-ACK-048" 主题重复，内容角度相似，降低 RAG 权重
difficulty: intermediate
reading_level: intermediate
audience:
- AI Agent
- SRE
- 运维工程师
estimated_read_time: 8min
intent_queries:
- StatefulSet Pod 启动失败：PVC 未绑定 如何处理
trigger_keywords:
- StatefulSet
prerequisites:
- kubectl-basics
- k8s-storage
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
authors:
- name: KUDIG Team
  role: contributor
relationships:
- target: '[[concepts/statefulset.md]]'
  type: related_to
- target: '[[domain-11-production-operations/ticket-cases/ticket-case-040-node-diskpressure-eviction.md]]'
  type: related_to
- target: '[[domain-11-production-operations/ticket-cases/ticket-case-042-pod-pending-resource-taint.md]]'
  type: related_to
---



# 工单描述

客户在专有云 ACK 集群 `ack-zyy-prod-04` 的 `middleware` 命名空间部署 Redis Cluster，使用 StatefulSet 管理。滚动升级后第 2 个 Pod 持续处于 `Pending` 状态，describe 显示 PVC 未绑定。客户描述如下：

> “我们 Redis Cluster 用 StatefulSet 部署，今天升级镜像后 redis-2 一直 Pending。describe pod 看到 `persistentvolumeclaim redis-data-redis-2 not found`，但 PVC 明明是 StatefulSet 自动创建的。describe pvc 看到 `Waiting for a volume to be created`。是不是存储出问题了？”

受影响命名空间为 `middleware`，运行 Redis Cluster、ZooKeeper 等中间件有状态服务。

## 分类与优先级判定

- **工单类型**：存储故障 / StatefulSet 启动失败。
- **优先级**：P1。
- **严重级别**：high。

判定依据：
1. 有状态中间件 Redis Cluster 节点无法启动，存在数据一致性风险。
2. PVC 未绑定直接影响 Pod 启动，属于存储依赖层故障。
3. 未造成服务完全不可用，但集群处于降级状态，需尽快修复。

## 诊断步骤

按“先 PVC 状态、后 StorageClass、再 CSI 日志”的顺序排查：

```bash
# 1. 查看 StatefulSet Pod 与 PVC 状态
kubectl get pod -n middleware -l app=redis -o wide
kubectl get pvc -n middleware -l app=redis

# 2. 查看 Pending PVC 的事件与错误信息
kubectl describe pvc redis-data-redis-2 -n middleware | tail -40

# 3. 检查 StorageClass 参数与回收策略
kubectl get storageclass alicloud-disk-ssd -o yaml

# 4. 查看 CSI provisioner 日志
kubectl logs -n kube-system -l app=csi-plugin -c csi-provisioner --tail=200 | grep -iE "redis-data-redis-2|error|fail|Insufficient"

# 5. 查看 CSI plugin 节点日志
kubectl logs -n kube-system -l app=csi-plugin -c csi-plugin --tail=200 | grep -iE "redis-data-redis-2|CreateVolume"

# 6. 检查 ACK 控制台云盘容量与可用区库存
aliyun ecs DescribeDisks --RegionId cn-zhangjiakou --ZoneId cn-zhangjiakou-a --DiskCategory cloud_ssd --Status Available --output cols=DiskId,Size,ZoneId rows=Disks.Disk[]

# 7. 通过 ASO 查看对应节点本地盘状态
kubectl get nodestorage -n kube-system cn-zhangjiakou.172.16.4.21 -o yaml
```

## 根因分析

PVC `redis-data-redis-2` 的 StorageClass `alicloud-disk-ssd` 设置了 `allowVolumeExpansion: true`，客户此前通过修改 StatefulSet volumeClaimTemplates 将每个 Redis 实例的存储从 50Gi 扩容到 100Gi。前两个 PVC 扩容成功，但 `redis-data-redis-2` 扩容时触发底层云盘操作失败：

```
ControllerExpandVolume failed: rpc error: code = ResourceExhausted desc = no enough disk capacity in zone cn-zhangjiakou-a
```

由于 PVC 当前处于 `Resizing` 状态且未绑定到 PV，StatefulSet 控制器无法为 `redis-2` Pod 完成卷挂载，Pod 持续 Pending。根本原因是目标可用区 SSD 云盘库存不足，同时 StorageClass 未配置多可用区拓扑感知，导致扩容只能在单一可用区尝试。类似的场景在 ESSD 自动扩容、性能级别变更时也可能出现，尤其是在业务高峰期间云盘库存波动较大，扩容操作更容易失败。

## 修复命令

**第一步：临时回滚 PVC 扩容请求，先恢复 Pod 启动**

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl delete`：删除资源（可由声明式清单重建）
> - `kubectl rollout undo/restart`：触发滚动变更，影响副本

```bash
# 备份当前 PVC
kubectl get pvc redis-data-redis-2 -n middleware -o yaml > /tmp/redis-data-redis-2-backup.yaml

# 由于 StatefulSet volumeClaimTemplates 不能直接修改已创建 PVC 的 size，
# 先删除处于 Resizing 的 PVC（数据不会丢失，PV 保留）
kubectl delete pvc redis-data-redis-2 -n middleware

# 等待 StatefulSet 重新创建 PVC 并绑定（此时使用原 50Gi 大小）
kubectl rollout restart statefulset/redis -n middleware
```

**第二步：修改 StorageClass 启用拓扑感知与多可用区调度**

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

```bash
kubectl patch storageclass alicloud-disk-ssd -p '{"allowedTopologies":[{"matchLabelExpressions":[{"key":"topology.kubernetes.io/zone","values":["cn-zhangjiakou-a","cn-zhangjiakou-b","cn-zhangjiakou-c"]}]}]}'
```

**第三步：切换到有库存的可用区扩容**

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl label/annotate`：改元数据可能影响选择器/控制器

```bash
# 确认 cn-zhangjiakou-b 有 SSD 库存后，为该节点池增加可用区标签
kubectl label node cn-zhangjiakou.172.16.4.31 topology.kubernetes.io/zone=cn-zhangjiakou-b --overwrite

# 再次触发 StatefulSet 滚动更新扩容
kubectl set image statefulset/redis redis=redis:7.2-alpine -n middleware
kubectl rollout status statefulset/redis -n middleware --timeout=600s
```

**第四步：检查新 PVC 已绑定并扩容成功**

```bash
kubectl get pvc redis-data-redis-2 -n middleware -o jsonpath='{.status.phase} {.spec.resources.requests.storage} {.status.capacity.storage}'
```

若扩容后 Redis 节点数据异常，可立即从备份的 PVC YAML 与数据快照恢复，并暂停 StatefulSet 的滚动更新，优先保障集群主从拓扑稳定。

## 验证命令

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

```bash
# 1. StatefulSet 所有 Pod Running
kubectl get pod -n middleware -l app=redis -o wide

# 2. PVC 全部 Bound 且容量正确
kubectl get pvc -n middleware -l app=redis -o custom-columns=NAME:.metadata.name,STATUS:.status.phase,CAPACITY:.status.capacity.storage

# 3. PV 与云盘映射正常
kubectl get pv -o json | jq '.items[] | select(.spec.claimRef.namespace=="middleware") | {name: .metadata.name, claim: .spec.claimRef.name, diskID: .spec.csi.volumeHandle}'

# 4. Redis Cluster 节点互通正常
kubectl exec -n middleware redis-0 -- redis-cli -h redis-1.redis ping
kubectl exec -n middleware redis-0 -- redis-cli cluster info | grep cluster_state

# 5. ACK 控制台查看云盘状态
aliyun ecs DescribeDisks --RegionId cn-zhangjiakou --DiskIds '["d-8vbdummyredis2"]' --output cols=DiskId,Size,Status,ZoneId rows=Disks.Disk[]
```

验证阶段需要同时关注 Redis Cluster 的节点状态与数据一致性。若扩容过程中节点长时间不可用，可能导致集群进入 fail 状态，需要手动执行 `cluster meet` 与重新分配槽位。建议在验证前先执行 `redis-cli cluster info` 与 `cluster nodes` 确认无迁移任务正在进行。

## 回复客户话术

> 您好，经排查，本次 Redis Cluster `redis-2` Pod Pending 的根因是 **PVC 扩容失败导致未绑定**。StatefulSet 的 volumeClaimTemplates 将存储从 50Gi 扩容到 100Gi 后，目标可用区 `cn-zhangjiakou-a` 的 SSD 云盘库存不足，PVC 一直处于 `Resizing` 状态，无法挂载到 Pod。我们已完成以下处置：
>
> 1. 删除异常 PVC 并重启 StatefulSet，先以原 50Gi 恢复 Pod 启动；
> 2. 为 StorageClass 配置多可用区拓扑感知；
> 3. 切换到库存充足的可用区完成扩容与滚动更新。
>
> 当前 Redis Cluster 全部节点 Running，cluster_state 为 ok。建议后续：
> - 扩容前通过 ACK 控制台或 OpenAPI 查询目标可用区云盘库存；
> - 有状态服务优先使用多可用区 StorageClass；
> - 配置 PVC 绑定失败告警。
>
> 如有新异常请随时联系。

## 复盘与沉淀

本次故障说明有状态服务的存储扩容不仅受 Kubernetes 层面控制，更受底层云资源库存与可用区分布限制。StorageClass 默认未配置 `allowedTopologies` 时，扩容操作可能集中在创建 PV 时的原始可用区，一旦该可用区库存耗尽，PVC 将长时间无法完成扩容。

在专有云 ACK 中，建议为不同可用区分别创建 StorageClass，或在单一 StorageClass 中声明多个可用区拓扑。同时，扩容前应当使用 `aliyun ecs DescribeAvailableResource` 查询目标区域与可用区的云盘库存，避免盲目修改 volumeClaimTemplates。对于 Redis、ZooKeeper 等需要快速恢复的有状态服务，建议提前在不同可用区预创建备用 PV 或启用动态扩容的并行重试机制，避免单可用区库存不足导致整个集群节点无法启动。

另外，StatefulSet 的 `volumeClaimTemplates` 一旦修改，已有 PVC 不会自动更新大小，需要手动删除 PVC 触发重建。这一行为在生产环境中风险较高，建议在变更窗口期执行，并确保数据已持久化到 PV。删除 PVC 时只要保留 PV，数据不会丢失，但仍需备份关键配置与索引文件。

后续 SOP 更新要点：
1. 存储扩容变更前必须查询目标可用区库存；
2. 有状态服务统一使用多可用区拓扑 StorageClass；
3. 监控 `kube_persistentvolumeclaim_status_phase{phase!="Bound"}` 持续 5 分钟触发 P1 告警；
4. StatefulSet 扩容必须安排在变更窗口，变更前进行 PVC 与数据备份；
5. 将本案例写入 StatefulSet PVC 故障回复模板。

## 是否需要升级及交接信息

- **是否升级**：已定位并修复，暂不需要升级；若多可用区均出现云盘库存不足，需升级至 **专有云基础设施团队** 协调库存。
- **交接信息**：
  - 故障单号：`INC-2026-ACK-043`
  - 根因：StorageClass 未配置多可用区拓扑，目标可用区 SSD 云盘库存不足
  - 影响命名空间：`middleware`，服务：Redis Cluster
  - 临时修复：删除异常 PVC 并回滚到 50Gi 启动
  - 长期方案：多可用区 StorageClass + 扩容前库存查询
  - 待跟进：确认剩余 Redis 实例是否按计划扩容到 100Gi，更新存储变更 SOP

## Related

- StatefulSet
- 节点磁盘压力 DiskPressure 导致 Pod 被驱逐
- Pod Pending：资源不足与 Taint 不匹配
