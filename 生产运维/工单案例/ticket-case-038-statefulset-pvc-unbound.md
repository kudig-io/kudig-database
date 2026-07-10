---
title: StatefulSet Pod 启动失败：PVC 未绑定
description: 专有云 ACK 集群 MySQL 主从 StatefulSet 新副本启动失败，根因为 StorageClass 未正确配置 allowVolumeExpansion
  且后端 NAS 挂载点已满，含诊断、修复与验证。
summary: 专有云 ACK 集群 MySQL 主从 StatefulSet 新副本启动失败，根因为 StorageClass 未正确配置 allowVolumeExpansion
  且后端 NAS 挂载点已满，含诊断、修复与验证。
category: 生产运维/ticket-case
tags:
- ack
- zyy
- statefulset
- pvc
- storageclass
- nas
- mysql
- p1
tier: peripheral
created: '2026-06-26T13:00:00+08:00'
updated: '2026-06-26T15:30:00+08:00'
incident_id: TC-2026-038
priority: P1
severity: high
affected_cluster: ack-zyy-prod-05
affected_namespace: middleware
ticket_type: 存储故障
skill_ref:
- PVC 故障诊断
- StatefulSet 存储
fta_ref:
- 'FTA: StatefulSet PVC 未绑定'
last_updated: 2026-06-26 15:30:00+08:00
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
- ack
- zyy
- statefulset
- pvc
- storageclass
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
- target: '[[生产运维/ticket-cases/ticket-case-043-statefulset-pvc-unbound.md]]'
  type: related_to
- target: '[[concepts/statefulset.md]]'
  type: related_to
- target: '[[生产运维/ticket-cases/ticket-case-040-node-diskpressure-eviction.md]]'
  type: related_to
- target: '[[生产运维/ticket-cases/ticket-case-042-pod-pending-resource-taint.md]]'
  type: related_to
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 工单描述

客户在 ACK 专有云集群 `ack-zyy-prod-05` 的 `middleware` 命名空间扩容 MySQL 从库时，新 Pod `mysql-slave-2` 一直处于 `ContainerCreating` 状态。客户描述如下：

> “我们的 MySQL 主从是 StatefulSet 部署的，今天想从 2 个副本扩到 3 个，结果 mysql-slave-2 一直 ContainerCreating。describe pod 看到 `Unable to attach or mount volumes: unmounted volumes=[data]`，还有 `persistentvolumeclaim mysql-data-mysql-slave-2 not found` 类似的提示。PVC 列表里这个 PVC 是 Pending。存储用的是 ACK 托管 NAS，麻烦看一下。”

该 MySQL 集群为订单核心依赖组件，从库扩容失败可能影响读流量分担与故障切换能力。

## 分类与优先级判定

- **工单类型**：存储故障 / StatefulSet 启动失败 / PVC 未绑定。
- **优先级**：P1。
- **严重级别**：high。

判定依据：
1. 生产环境数据中间件扩容受阻，虽未完全不可用，但影响高可用能力。
2. 报错指向 PVC 无法绑定，涉及 ACK 托管 NAS 与 StorageClass 配置。
3. 需要在 30 分钟内定位根因并恢复扩容。

## 诊断步骤

按“先 Pod 事件、后 PVC/PV、再存储后端”的顺序排查：

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 1. 查看 StatefulSet Pod 与 PVC 状态
kubectl get pod -n middleware -l app=mysql
kubectl get pvc -n middleware
kubectl get pv | grep mysql

# 2. 查看 Pod 事件与 PVC 事件
kubectl describe pod -n middleware mysql-slave-2 | grep -A 40 Events
kubectl describe pvc -n middleware mysql-data-mysql-slave-2 | grep -A 30 Events

# 3. 检查 StorageClass 配置
kubectl get storageclass
kubectl get storageclass alicloud-nas-subpath -o yaml

# 4. 检查 CSI 插件 Pod 状态
kubectl get pod -n kube-system | grep -E "csi|nas"
kubectl logs -n kube-system -l app=csi-plugin-nas --tail=200 | grep -i "mysql-slave-2|error|fail" | tail -30

# 5. 检查 NAS 挂载点与文件系统容量
aliyun nas DescribeFileSystems --RegionId cn-zhangjiakou --PageSize 50 --output cols=FileSystemId,Capacity,Status rows=FileSystems.FileSystem[]
aliyun nas DescribeMountTargets --FileSystemId fs-zyy-mysql-xxx --RegionId cn-zhangjiakou --output cols=MountTargetDomain,Status rows=MountTargets.MountTarget[]

# 6. 查看 ACK 控制台或 ASO 侧 NAS 存储池状态
ack-cli storage nas status --cluster ack-zyy-prod-05 --filesystem fs-zyy-mysql-xxx

# 7. 检查 StatefulSet volumeClaimTemplates
kubectl get statefulset mysql-slave -n middleware -o yaml | grep -A 30 volumeClaimTemplates
```
## 根因分析

`mysql-data-mysql-slave-2` PVC 长期处于 Pending，根因为 StorageClass `alicloud-nas-subpath` 关联的 NAS 文件系统 `fs-zyy-mysql-xxx` 已达容量上限，无法创建新的 subpath 目录。具体链路如下：

1. **容量耗尽：** NAS 文件系统总容量 500GiB，当前已用 498GiB，接近 100%，CSI 在 provision 新 PV 时返回 `NoSpace` 类错误。
2. **StorageClass 配置问题：** `alicloud-nas-subpath` 未开启 `allowVolumeExpansion: true`，且未配置容量告警，导致历史 PVC 无法在线扩容，新 PVC 也无法创建。
3. **PVC 未绑定：** CSI provisioner 因后端存储无法分配空间，未成功创建 PV，PVC 状态保持 Pending，Pod 因无法挂载卷而 ContainerCreating。

根因置信度：**高**（CSI 日志中出现 `Failed to create subpath: no space left on device`）。

### 风险与影响评估

- **业务影响：** MySQL 从库扩容失败导致读流量分担能力不足，若主库再出现故障，将缺乏足够的从库承接故障切换，存在高可用风险。
- **扩散风险：** 同一 NAS 文件系统可能被多个 StatefulSet 共享，容量耗尽将影响所有依赖该文件系统的 PVC 创建与扩容。
- **数据风险：** 历史 MySQL 数据未丢失，但若在容量不足期间发生主库写入突增，可能因 NAS 配额耗尽导致写入异常。
- **恢复关键：** 必须同时解决后端 NAS 容量问题与 StorageClass 可扩展性问题，否则新 PVC 即使临时创建成功，后续也无法扩容。

## 修复命令

**第一步：临时扩容 NAS 文件系统容量**

```bash
# 在阿里云控制台或 CLI 扩容 NAS 文件系统
aliyun nas UpgradeFileSystem \
  --FileSystemId fs-zyy-mysql-xxx \
  --Capacity 1000 \
  --RegionId cn-zhangjiakou

# 等待扩容完成
aliyun nas DescribeFileSystems \
  --FileSystemId fs-zyy-mysql-xxx \
  --RegionId cn-zhangjiakou \
  --output cols=FileSystemId,Capacity,Status rows=FileSystems.FileSystem[]
```

**第二步：启用 StorageClass 的 allowVolumeExpansion**

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
kubectl patch storageclass alicloud-nas-subpath --patch='{"allowVolumeExpansion": true}'
```
**第三步：删除 Pending PVC，让 StatefulSet 控制器重新创建**

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl delete`：删除资源（可由声明式清单重建）

> **🔴 高风险操作警告**
>
> 下方命令属于不可逆或高影响操作，执行前请确认：
> - 已备份关键数据与配置
> - 处于批准的变更窗口期
> - 已获得相关责任人授权
> - 已准备回滚或恢复方案
> - 目标集群、Namespace、节点/资源名称正确无误

``` bash
# 🔴 高风险：可能造成数据丢失或服务中断，执行前需备份、变更审批与回滚方案
kubectl delete pvc mysql-data-mysql-slave-2 -n middleware
# StatefulSet 控制器会自动重新创建 PVC
sleep 30
kubectl get pvc -n middleware mysql-data-mysql-slave-2
```
**第四步：确认新 Pod 启动成功**

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl get pod -n middleware mysql-slave-2 -o wide
kubectl describe pod -n middleware mysql-slave-2 | grep -A 20 Events
```
**第五步：对历史 MySQL 数据卷进行扩容（可选，建议低峰期执行）**

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
kubectl patch pvc mysql-data-mysql-slave-0 -n middleware -p '{"spec":{"resources":{"requests":{"storage":"200Gi"}}}}'
kubectl patch pvc mysql-data-mysql-slave-1 -n middleware -p '{"spec":{"resources":{"requests":{"storage":"200Gi"}}}}'
```
## 验证命令

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 1. PVC 已 Bound
kubectl get pvc -n middleware mysql-data-mysql-slave-2 -o jsonpath='{.status.phase}'

# 2. PV 已创建并关联到 NAS subpath
kubectl get pv $(kubectl get pvc -n middleware mysql-data-mysql-slave-2 -o jsonpath='{.spec.volumeName}') -o yaml | grep -E "volumeHandle|path"

# 3. Pod 状态 Running
kubectl get pod -n middleware mysql-slave-2 -o jsonpath='{.status.phase}'

# 4. 进入 Pod 验证 MySQL 从库同步状态
kubectl exec -n middleware mysql-slave-2 -- mysql -uroot -p$MYSQL_ROOT_PASSWORD -e "SHOW SLAVE STATUS\G" | grep -E "Slave_IO_Running|Slave_SQL_Running|Seconds_Behind_Master"

# 5. NAS 文件系统容量使用率下降
aliyun nas DescribeFileSystems --FileSystemId fs-zyy-mysql-xxx --RegionId cn-zhangjiakou --output cols=FileSystemId,Capacity,MeteredSize rows=FileSystems.FileSystem[]

# 6. CSI 日志无新的 provision 失败
kubectl logs -n kube-system -l app=csi-plugin-nas --tail=100 | grep -i "mysql-slave-2|error" || echo "无新错误"
```
## 回复客户话术

> 您好，工单 TC-2026-038 已处理完成。
>
> **现象确认：** `middleware/mysql-slave` StatefulSet 扩容至 3 副本时，`mysql-slave-2` 长期处于 `ContainerCreating`，对应 PVC `mysql-data-mysql-slave-2` 为 Pending。
>
> **根因：** 后端 NAS 文件系统 `fs-zyy-mysql-xxx` 容量已用满（500GiB 中已用 498GiB），CSI 在 provision 新卷时因 `NoSpace` 失败，导致 PVC 无法绑定；同时 StorageClass 未开启 `allowVolumeExpansion`，历史卷也无法在线扩容。
>
> **已执行修复：**
> 1. 将 NAS 文件系统从 500GiB 扩容至 1000GiB；
> 2. 为 StorageClass `alicloud-nas-subpath` 启用 `allowVolumeExpansion: true`；
> 3. 删除 Pending PVC，由 StatefulSet 控制器重新创建并成功 Bound；
> 4. `mysql-slave-2` 已正常启动并加入 MySQL 主从复制。
>
> **当前状态：** PVC 已 Bound，Pod 已 Running，从库同步状态正常（Slave_IO_Running / Slave_SQL_Running 均为 Yes）。
>
> **后续建议：**
> - 为 NAS 文件系统配置容量使用率告警，建议阈值 75% 预警、85% P2 告警；
> - 定期评估 MySQL 数据增长趋势，提前扩容存储；
> - 参考 PVC 故障诊断 建立存储容量基线；
> - 对核心中间件 PVC 开启数据备份与快照策略，避免容量异常时影响数据安全。
>
> 如有异常请随时联系。

## 复盘与沉淀

本次故障揭示了 StatefulSet 与托管 NAS 存储在容量管理上的联动风险。StatefulSet 的 `volumeClaimTemplates` 会在扩容时自动为每个新副本创建 PVC，但 PVC 能否成功绑定，完全取决于后端存储是否有足够容量。在 ACK 专有云使用 alicloud-nas-subpath 等共享文件系统时，多个 PVC 可能共享同一个 NAS 文件系统，单个文件系统容量耗尽会影响所有依赖它的 StatefulSet。

排查时应建立“Pod → PVC → PV → StorageClass → 后端存储”的完整链路。很多同学习惯于在 Kubernetes 侧反复检查 PVC/PV，却忽略了 NAS 文件系统本身的容量状态。`kubectl describe pvc` 中的事件通常只会显示 `waiting for a volume to be created`，不会直接说明后端 NAS 已满，因此需要主动查看 CSI provisioner 日志与 NAS 控制台。

本次修复中删除 Pending PVC 让 StatefulSet 重新创建是一种安全做法，因为 StatefulSet 会严格按照 `volumeClaimTemplates` 重建同名 PVC，不会丢失已有数据。但若 PVC 已 Bound 后再删除，则必须确认是否保留 PV 回收策略，否则可能触发数据删除。

建议后续建立以下机制：
1. **存储容量看板：** 按 NAS 文件系统维度展示总容量、已用容量、PVC 数量与增长率；
2. **自动扩容：** 对核心中间件 NAS 文件系统配置自动扩容策略，达到阈值后自动扩大容量；
3. **容量规划：** 在业务上线前评估数据增长曲线，避免将多个高写入应用共享到同一文件系统；
4. **备份策略：** 对 MySQL 等关键数据卷配置定期快照，参考 StatefulSet 存储 最佳实践。

## 是否需要升级及交接信息

- **是否升级**：否（已闭环）。若 NAS 文件系统扩容后仍频繁出现容量告警，需升级至 **存储基础设施团队** 评估是否需拆分文件系统或迁移至 ESSD。
- **交接信息**：
  - 故障单号：`TC-2026-038`
  - 根因：NAS 文件系统容量耗尽，StorageClass 未开启 allowVolumeExpansion
  - 影响集群：`ack-zyy-prod-05`
  - 影响命名空间：`middleware`
  - 临时修复：扩容 NAS 文件系统并重新创建 PVC
  - 长期方案：建立 NAS 容量监控、告警与自动扩容策略
  - 待跟进：确认 MySQL 从库复制延迟稳定，评估历史 PVC 扩容窗口

## Related

- StatefulSet Pod 启动失败：PVC 未绑定
- StatefulSet
- 节点磁盘压力 DiskPressure 导致 Pod 被驱逐
- Pod Pending：资源不足与 Taint 不匹配


<!-- risk-assessed -->
