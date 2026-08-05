---
title: StatefulSet Pod 启动失败：PVC 未绑定与配置错误
description: 专有云 ACK 集群 MySQL 主从 StatefulSet 发布时 Pod 持续 ContainerCreating，根因涉及 PVC
  未绑定、StorageClass 参数错误与 volumeClaimTemplates 配置冲突的工单闭环样本。
summary: 专有云 ACK 集群 MySQL 主从 StatefulSet 发布时 Pod 持续 ContainerCreating，根因涉及 PVC 未绑定、StorageClass
  参数错误与 volumeClaimTemplates 配置冲突的工单闭环样本。
category: 生产运维/ticket-case
tags:
- ack
- zyy
- statefulset
- pvc
- storage
- mysql
- containercreating
- p1
tier: peripheral
created: '2026-06-26T14:00:00+08:00'
updated: '2026-06-26T17:15:00+08:00'
incident_id: INC-2026-ACK-048
priority: P1
severity: high
affected_cluster: ack-zyy-prod-06
affected_namespace: db-mysql
ticket_type: 存储故障 / 有状态应用故障
skill_ref:
- '[[06-存储/01-K8s存储/10-pv-pvc-troubleshooting.md|PV/PVC 排障]]'
- '[[06-存储/01-K8s存储/05-storageclass-dynamic-provisioning.md|StorageClass
  动态供给]]'
- '[[06-存储/04-有状态应用存储/01-stateful-app-storage-patterns.md|有状态应用存储模式]]'
fta_ref:
- 'FTA: StatefulSet PVC 启动失败'
last_updated: 2026-06-26 17:15:00+08:00
difficulty: intermediate
reading_level: intermediate
audience:
- AI Agent
- SRE
- 运维工程师
estimated_read_time: 8min
intent_queries:
- StatefulSet Pod 启动失败：PVC 未绑定与配置错误 如何处理
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
- target: '[[13-生产运维/05-工单案例/ticket-case-043-statefulset-pvc-unbound.md]]'
  type: related_to
- target: '[[22-概念/02-工作负载/statefulset.md]]'
  type: related_to
- target: '[[13-生产运维/05-工单案例/ticket-case-040-node-diskpressure-eviction.md]]'
  type: related_to
- target: '[[13-生产运维/05-工单案例/ticket-case-042-pod-pending-resource-taint.md]]'
  type: related_to
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 工单描述

客户在专有云 ACK 集群 `ack-zyy-prod-06` 部署 MySQL 主从集群时，发现 `db-mysql` 命名空间下的 StatefulSet `mysql-prod` 所有 Pod 均处于 `ContainerCreating` 状态，无法进入 Running。客户描述如下：

> “我们在 ACK 上按照官方文档部署了一个 3 节点的 MySQL 主从集群，用的是 StatefulSet + volumeClaimTemplates。创建之后 Pod 一直 ContainerCreating，describe pod 看到 failed to provision volume with StorageClass 的错误，还有 PVC 处于 Pending。我们用的是专有云自带的 alicloud-disk 插件，ESSD 云盘。麻烦帮忙看一下是 StorageClass 配错了还是 PVC 没绑定上。”

受影响命名空间为 `db-mysql`，业务为订单核心数据库，Pod 无法启动将导致数据库主从架构无法建立，影响后续业务上线。

## 分类与优先级判定

- **工单类型**：存储故障 / 有状态应用故障。
- **优先级**：P1。
- **严重级别**：high。

判定依据：
1. 生产环境数据库类有状态应用无法启动，属于关键基础设施故障。
2. 故障集中在 PVC 动态供给链路，涉及 StorageClass、CSI、底层云盘等多个环节，需要系统排查。
3. 当前业务尚未完全上线，但数据库无法就绪将阻塞后续发布，符合 P1 “生产环境 + 服务降级/阻塞” 标准。

## 诊断步骤

按“先看 PVC/PV 状态，再查 StorageClass 与 CSI，最后看 Pod 事件”的顺序排查：

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 1. 查看 StatefulSet 与 Pod 状态
kubectl get statefulset mysql-prod -n db-mysql
kubectl get pod -n db-mysql -l app=mysql-prod -o wide

# 2. 查看 PVC 状态
kubectl get pvc -n db-mysql
kubectl describe pvc data-mysql-prod-0 -n db-mysql

# 3. 查看 StorageClass 详情
kubectl get storageclass alicloud-disk-ssd -o yaml
kubectl get storageclass alicloud-disk-essd -o yaml

# 4. 查看 CSI 插件 Pod 状态
kubectl get pod -n kube-system -l app=csi-plugin
kubectl get pod -n kube-system -l app=csi-provisioner
kubectl logs -n kube-system -l app=csi-provisioner -c csi-provisioner --tail=200 | grep -i "mysql-prod|fail|error" | tail -30

# 5. 查看 Pod 创建事件
kubectl describe pod mysql-prod-0 -n db-mysql | grep -A 30 Events
kubectl get events -n db-mysql --field-selector reason=FailedMount --sort-by='.lastTimestamp'

# 6. 检查 ESSD 云盘在阿里云侧的创建状态
aliyun ecs DescribeDisks \
  --RegionId cn-zhangjiakou \
  --ZoneId cn-zhangjiakou-a \
  --Tag.1.Key k8s.namespace \
  --Tag.1.Value db-mysql \
  --output cols=DiskId,Status,Size,Category rows=Disks.Disk[]

# 7. 检查节点上 CSI 插件日志
kubectl logs -n kube-system $(kubectl get pod -n kube-system -l app=csi-plugin -o jsonpath='{.items[0].metadata.name}') -c csi-plugin --tail=100 | grep -i "mysql|fail|error" | tail -20

# 8. 通过 ASO 检查存储相关 CRD 状态
kubectl get disk.csi.alibabacloud.com -n kube-system | head -10
```
## 根因分析

通过 PVC 描述与 CSI 日志确认，存在以下三个问题：

**问题一：StorageClass 中 `zoneId` 参数与节点可用区不匹配**

PVC 事件显示：

```
Warning  ProvisioningFailed  ...  alicloud/disk  failed to provision volume with StorageClass "alicloud-disk-essd": create disk error: InvalidZone.NotSupported: The specified zone is not supported.
```

StatefulSet 的 `volumeClaimTemplates` 指定了 `storageClassName: alicloud-disk-essd`，但该 StorageClass 的 `parameters.zoneId` 固定为 `cn-zhangjiakou-a`，而当前节点池 `np-db-mysql` 实际部署在 `cn-zhangjiakou-b` 与 `cn-zhangjiakou-c`。ESSD 云盘只能在同一可用区内挂载，跨区创建导致失败。

**问题二：volumeClaimTemplates 中 `storageClassName` 指向不存在的 StorageClass**

部分 PVC 事件显示：

```
Warning  ProvisioningFailed  ...  persistentvolume-controller  storageclass "alicloud-disk-ssd" not found
```

客户最初参考旧文档使用了 `alicloud-disk-ssd`，但该 StorageClass 在当前集群中已被管理员删除并统一替换为 `alicloud-disk-essd`。

**问题三：ESSD 云盘类型与 `performanceLevel` 参数不兼容**

CSI 日志中发现：

```
failed to create disk: InvalidPerformanceLevel.Malformed: The specified performance level is not valid for disk category ESSD.
```

StorageClass 中 `parameters.performanceLevel=PL1`，但当前可用区仅支持 `PL0` 级别的 ESSD Entry 盘，导致云盘创建失败。

## 修复命令

**第一步：删除错误的 StatefulSet 与 PVC（注意：本例为首次部署，无数据）**

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
kubectl delete statefulset mysql-prod -n db-mysql
kubectl delete pvc -n db-mysql -l app=mysql-prod
```
> 注意：若 PVC 中已有业务数据，不可直接删除，应先备份或手动创建 PV/PVC。

**第二步：创建正确的 StorageClass，使用拓扑感知自动选择可用区**

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
kubectl apply -f - <<EOF
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: alicloud-disk-essd-topology
provisioner: diskplugin.csi.alibabacloud.com
parameters:
  regionId: cn-zhangjiakou
  type: cloud_essd
  performanceLevel: PL0
  encrypted: "false"
reclaimPolicy: Retain
allowVolumeExpansion: true
volumeBindingMode: WaitForFirstConsumer
EOF
```
`volumeBindingMode: WaitForFirstConsumer` 可让 PVC 延迟到 Pod 调度完成后再绑定，从而自动选择 Pod 所在可用区的云盘。

**第三步：更新 StatefulSet 的 volumeClaimTemplates**

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
kubectl apply -f - <<EOF
apiVersion: apps/v1
kind: StatefulSet
metadata:
  name: mysql-prod
  namespace: db-mysql
spec:
  serviceName: mysql-prod
  replicas: 3
  selector:
    matchLabels:
      app: mysql-prod
  template:
    metadata:
      labels:
        app: mysql-prod
    spec:
      containers:
      - name: mysql
        image: registry-vpc.cn-zhangjiakou.aliyuncs.com/acs/mysql:8.0.36
        ports:
        - containerPort: 3306
        volumeMounts:
        - name: data
          mountPath: /var/lib/mysql
        env:
        - name: MYSQL_ROOT_PASSWORD
          valueFrom:
            secretKeyRef:
              name: mysql-secret
              key: root-password
  volumeClaimTemplates:
  - metadata:
      name: data
    spec:
      accessModes: ["ReadWriteOnce"]
      storageClassName: alicloud-disk-essd-topology
      resources:
        requests:
          storage: 100Gi
EOF
```
**第四步：确认 StatefulSet 滚动创建成功**

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl rollout status statefulset mysql-prod -n db-mysql --timeout=600s
```
## 验证命令

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 1. 确认 StatefulSet 与 Pod 状态
kubectl get statefulset mysql-prod -n db-mysql
kubectl get pod -n db-mysql -l app=mysql-prod -o wide

# 2. 确认 PVC 已 Bound 并与 Pod 一一对应
kubectl get pvc -n db-mysql
kubectl get pv | grep mysql-prod

# 3. 确认 PV 对应的云盘已在阿里云侧创建
aliyun ecs DescribeDisks \
  --RegionId cn-zhangjiakou \
  --Tag.1.Key k8s.namespace \
  --Tag.1.Value db-mysql \
  --output cols=DiskId,Status,Size,ZoneId,Category rows=Disks.Disk[]

# 4. 进入 Pod 验证数据目录可写
kubectl exec -n db-mysql -it mysql-prod-0 -- df -h /var/lib/mysql
kubectl exec -n db-mysql -it mysql-prod-0 -- touch /var/lib/mysql/test-write && echo "write ok"

# 5. 验证 MySQL 服务可访问
kubectl exec -n db-mysql -it mysql-prod-0 -- mysql -uroot -p$(kubectl get secret mysql-secret -n db-mysql -o jsonpath='{.data.root-password}' | base64 -d) -e "SELECT 1;"

# 6. 检查 CSI 日志无新的报错
kubectl logs -n kube-system -l app=csi-provisioner -c csi-provisioner --tail=50 | grep -i "mysql-prod" | tail -10
```
## 回复客户话术

> 您好，经排查，本次 `mysql-prod` StatefulSet Pod 持续 `ContainerCreating` 的根因是 **PVC 动态供给链路配置错误**，具体包括：
>
> 1. **StorageClass 中可用区参数固定为 `cn-zhangjiakou-a`**，而实际节点部署在 `cn-zhangjiakou-b/c`，导致 ESSD 云盘无法创建；
> 2. **旧版 StorageClass `alicloud-disk-ssd` 已不存在**，但 StatefulSet 仍引用该名称；
> 3. **ESSD 性能级别 `PL1` 在当前可用区不支持**，需降级为 `PL0`。
>
> 我们已完成以下处置：
> - 删除首次部署时的错误 StatefulSet 与空 PVC；
> - 新建拓扑感知 StorageClass `alicloud-disk-essd-topology`，使用 `WaitForFirstConsumer` 自动匹配 Pod 所在可用区；
> - 更新 StatefulSet 引用新的 StorageClass 并重新创建。
>
> 当前 3 个 MySQL Pod 均已 Running，PVC 全部 Bound，数据目录可读写，MySQL 服务可正常登录。建议后续：
> - 为数据库类有状态应用统一使用 `volumeBindingMode: WaitForFirstConsumer` 的 StorageClass，参考 [[06-存储/01-K8s存储/05-storageclass-dynamic-provisioning.md|StorageClass 动态供给]]；
> - 在云侧提前确认目标可用区支持的 ESSD 性能级别；
> - 将本案例纳入数据库上线 Checklist，参考 [[06-存储/04-有状态应用存储/01-stateful-app-storage-patterns.md|有状态应用存储模式]]。
>
> 如有疑问，请随时联系。

## 复盘与沉淀

本次故障是 StatefulSet + 动态云盘供给在多可用区场景下的典型配置错误。核心教训：

1. **避免在 StorageClass 中硬编码可用区**：专有云 ACK 集群通常跨多个可用区部署，硬编码 `zoneId` 会导致 Pod 调度与云盘创建可用区不一致。应使用 `volumeBindingMode: WaitForFirstConsumer`，让 PVC 随 Pod 调度延迟绑定。对于 MySQL、Redis、Kafka 等有状态应用，还应结合 `podAntiAffinity` 将副本分散到不同可用区，进一步提升可用性。
2. **StorageClass 名称变更需同步更新所有工作负载**：集群管理员删除旧 StorageClass 后，所有依赖该名称的 YAML 都会失效。建议在变更前通过 `kubectl get pvc -A` 扫描引用关系，或保留旧 StorageClass 作为别名一段时间。同时应在 GitOps 仓库中全局搜索旧 StorageClass 名称，确保无遗漏。
3. **ESSD 性能级别需与可用区匹配**：不同可用区对 `PL0/PL1/PL2/PL3` 的支持可能不同，创建 StorageClass 前应先通过阿里云控制台或 API 确认。对于性能敏感的数据库，建议先在测试环境创建测试 PVC 验证云盘创建速度与 IOPS 是否满足预期。
4. **数据备份优先**：虽然本例为首次部署且无数据，但生产环境中处理 StatefulSet 与 PVC 问题时，务必先评估数据风险。删除 PVC 前需确认 PV 的 `reclaimPolicy` 为 `Retain`，并已做快照或物理备份。

建议将本案例加入 StatefulSet PVC FTA，并在日常巡检中增加：
- PVC Pending 超过 5 分钟告警；
- StorageClass 引用不存在告警；
- 云盘创建失败次数告警；
- StatefulSet Pod 处于 `ContainerCreating` 超过 10 分钟告警。

后续 SOP 更新要点：
1. 数据库类 StatefulSet 上线前必须验证 StorageClass 的 `volumeBindingMode` 与 `parameters`；
2. 多可用区集群禁用带固定 `zoneId` 的 StorageClass；
3. 为每个命名空间维护一份受信任的 StorageClass 白名单。

## 是否需要升级及交接信息

- **是否升级**：已定位并修复，暂不需要升级；若后续发现 CSI 插件自身 Bug 导致云盘创建不稳定，需升级至 **ACK 存储团队**。
- **交接信息**：
  - 故障单号：`INC-2026-ACK-048`
  - 根因：StorageClass 可用区与性能级别配置错误、旧 StorageClass 不存在
  - 影响集群：`ack-zyy-prod-06`
  - 影响命名空间：`db-mysql`
  - 临时修复：删除错误 StatefulSet/PVC、创建拓扑感知 StorageClass、重新部署
  - 长期方案：统一数据库 StorageClass 规范、建立上线前配置检查清单
  - 待跟进：确认 MySQL 主从同步配置完成，更新数据库上线 SOP

## Related

- StatefulSet Pod 启动失败：PVC 未绑定
- StatefulSet
- 节点磁盘压力 DiskPressure 导致 Pod 被驱逐
- Pod Pending：资源不足与 Taint 不匹配
- StatefulSet
- 节点磁盘压力 DiskPressure 导致 Pod 被驱逐
- Pod Pending：资源不足与 Taint 不匹配


<!-- risk-assessed -->
