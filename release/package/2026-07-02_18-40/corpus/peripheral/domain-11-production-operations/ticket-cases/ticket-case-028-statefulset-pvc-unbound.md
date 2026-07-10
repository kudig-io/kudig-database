---
title: StatefulSet Pod 启动失败：PVC 未绑定
description: 专有云 ACK 集群 Kafka StatefulSet 因 volumeClaimTemplate 指定了不存在的 StorageClass
  导致 PVC 长时间 Pending 的工单闭环样本。
summary: 专有云 ACK 集群 Kafka StatefulSet 因 volumeClaimTemplate 指定了不存在的 StorageClass 导致
  PVC 长时间 Pending 的工单闭环样本。
category: domain-11-production-operations/ticket-case
tags:
- ack
- zyy
- statefulset
- pvc
- storageclass
- kafka
- p1
- storage
tier: peripheral
created: '2026-06-26T10:00:00+08:00'
updated: '2026-06-26T13:15:00+08:00'
incident_id: INC-2026-ACK-028
priority: P1
severity: high
affected_cluster: ack-zyy-prod-05
affected_namespace: middleware
ticket_type: 存储故障
skill_ref:
- PVC Pending 诊断
- StatefulSet 存储管理
fta_ref:
- 'FTA: StatefulSet PVC 未绑定'
last_updated: 2026-06-26 13:15:00+08:00
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
- target: '[[domain-11-production-operations/工单案例/ticket-case-043-statefulset-pvc-unbound.md]]'
  type: related_to
- target: '[[concepts/statefulset.md]]'
  type: related_to
- target: '[[domain-11-production-operations/工单案例/ticket-case-040-node-diskpressure-eviction.md]]'
  type: related_to
- target: '[[domain-11-production-operations/工单案例/ticket-case-042-pod-pending-resource-taint.md]]'
  type: related_to
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 工单描述

客户反馈其部署在专有云 ACK 集群 `ack-zyy-prod-05` 的 Kafka 集群出现部分 broker 无法启动，`kafka-broker-2` 一直处于 `Pending` 状态。客户描述如下：

> “我们的 Kafka 集群是用 StatefulSet 部署的，今天扩容到 5 个 broker 后，kafka-broker-2 一直起不来。describe pod 看到 FailedMount 和 unbound immediate PersistentVolumeClaims。PVC 状态是 Pending，StorageClass 名称好像也没问题。麻烦帮忙看一下是不是存储那边出问题了。”

该 Kafka 集群为消息中间件核心组件，承载订单、支付、库存三大业务的异步消息，broker 数不足会直接影响分区高可用。

## 分类与优先级判定

- **工单类型**：存储故障 / StatefulSet 启动异常。
- **优先级**：P1。
- **严重级别**：high。

判定依据：
1. 生产环境中间件集群出现 broker 无法启动，分区副本数不足，存在数据丢失风险。
2. 问题集中在 PVC 与 StorageClass 层，属于存储供给链路异常。
3. 虽未完全中断服务，但已造成集群高可用降级，需在 30 分钟内定位并修复。

## 诊断步骤

按“先 Pod 事件、再 PVC/StorageClass、最后 CSI 供给日志”的顺序排查：

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 1. 查看 StatefulSet 与 Pod 状态
kubectl get statefulset kafka-broker -n middleware
kubectl get pod -n middleware -l app=kafka-broker -o wide

# 2. 查看 Pending Pod 的 Event 与 PVC 挂载错误
kubectl describe pod -n middleware kafka-broker-2 | tail -80
kubectl get events -n middleware --field-selector involvedObject.name=kafka-broker-2 --sort-by='.lastTimestamp'

# 3. 查看 PVC 状态与 StorageClass
kubectl get pvc -n middleware | grep kafka-broker-2
kubectl describe pvc data-kafka-broker-2 -n middleware

# 4. 检查集群内可用 StorageClass
kubectl get storageclass
kubectl get storageclass alicloud-disk-essd-pl1 -o yaml

# 5. 检查 CSI provisioner 日志
kubectl logs -n kube-system -l app=csi-plugin -c csi-plugin --tail=300 | grep -iE "kafka-broker-2|provision|storageclass"
kubectl logs -n kube-system -l app=csi-provisioner -c csi-provisioner --tail=300 | grep -iE "kafka-broker-2|NotFound|Failed"

# 6. 通过 ACK 控制台或 aliyun CLI 检查云盘配额与可用区
aliyun ecs DescribeAvailableResource \
  --RegionId cn-shanghai \
  --DestinationResource Disk \
  --ZoneId cn-shanghai-f \
  --InstanceType ecs.c7.2xlarge

# 7. 检查 ASO 或 ACK 存储相关 CR
kubectl get sc -o yaml | grep -B2 -A5 "essd-pl1"
```
## 根因分析

经排查，发现 Kafka StatefulSet 的 `volumeClaimTemplates` 中指定的 StorageClass 为 `alicloud-disk-essd-pl1`：

```yaml
volumeClaimTemplates:
  - metadata:
      name: data
    spec:
      accessModes: ["ReadWriteOnce"]
      storageClassName: alicloud-disk-essd-pl1
      resources:
        requests:
          storage: 500Gi
```

但集群实际只部署了以下 StorageClass：

- `alicloud-disk-essd`（默认，PL0）
- `alicloud-disk-essd-performance`（PL1/PL2 混合，由管理员自定义命名）
- `alicloud-disk-topology`（WaitForFirstConsumer 模式）

`alicloud-disk-essd-pl1` 这个 StorageClass **不存在**。因此 CSI provisioner 在收到 PVC 创建请求后，无法找到对应的 StorageClass，也无法调用阿里云 OpenAPI 创建云盘，PVC 一直 Pending。Pod 因 `unbound immediate PersistentVolumeClaims` 无法完成调度与挂载。

`kubectl describe pvc data-kafka-broker-2 -n middleware` 输出关键信息：

```
Status:        Pending
Events:
  Type     Reason              Age   From                         Message
  ----     ------              ----  ----                         -------
  Warning  ProvisioningFailed  12m   persistentvolume-controller  storageclass.storage.k8s.io "alicloud-disk-essd-pl1" not found
```

根本原因是 **StatefulSet 配置中引用了不存在的 StorageClass，导致动态供给失败**。

## 修复命令

**第一步：确认可用的 StorageClass 及其性能等级**

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl get storageclass -o custom-columns='NAME:.metadata.name,PROVISIONER:.provisioner,RECLAIMPOLICY:.reclaimPolicy,VOLUMEBINDINGMODE:.volumeBindingMode,DEFAULT:.metadata.annotations.storageclass\.kubernetes\.io/is-default-class'
```
确认 `alicloud-disk-essd-performance` 对应 ESSD PL1，满足 Kafka 性能要求。

**第二步：临时为已 Pending 的 PVC 修改 StorageClass（仅对未绑定 PVC 有效）**

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
kubectl patch pvc data-kafka-broker-2 -n middleware --type='merge' -p '{
  "spec": {
    "storageClassName": "alicloud-disk-essd-performance"
  }
}'
```
> 注意：PVC 一旦绑定，无法修改 StorageClass。本例中 PVC 尚未绑定，因此可以 patch。

**第三步：更新 StatefulSet volumeClaimTemplates，避免后续新 broker 复现**

由于 StatefulSet 的 `volumeClaimTemplates` 字段不可直接 patch，需要采用 **孤儿删除 + 重新创建** 的方式保留现有 Pod：

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源
> - `kubectl delete`：删除资源（可由声明式清单重建）

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 1. 导出当前 StatefulSet YAML
kubectl get statefulset kafka-broker -n middleware -o yaml > /tmp/kafka-broker-ss.yaml

# 2. 在 YAML 中将 storageClassName 从 alicloud-disk-essd-pl1 改为 alicloud-disk-essd-performance
sed -i 's/alicloud-disk-essd-pl1/alicloud-disk-essd-performance/g' /tmp/kafka-broker-ss.yaml

# 3. 删除 StatefulSet 但不删除 Pod（孤儿模式）
kubectl delete statefulset kafka-broker -n middleware --cascade=orphan

# 4. 重新应用修改后的 YAML
kubectl apply -f /tmp/kafka-broker-ss.yaml
```
**第四步：手动触发 kafka-broker-2 重建，使其使用已修正的 PVC**

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl delete`：删除资源（可由声明式清单重建）

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 删除 Pending 的 Pod，StatefulSet 控制器会按新模板重建
kubectl delete pod kafka-broker-2 -n middleware

# 观察 StatefulSet 滚动状态
kubectl rollout status statefulset kafka-broker -n middleware --timeout=300s
```
**第五步：如需要保留原 StorageClass 名称，可在集群中创建缺失的 StorageClass（长期方案）**

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
cat <<EOF | kubectl apply -f -
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: alicloud-disk-essd-pl1
provisioner: diskplugin.csi.alibabacloud.com
parameters:
  regionId: cn-shanghai
  zoneId: cn-shanghai-f
  type: cloud_essd
  performanceLevel: PL1
reclaimPolicy: Delete
allowVolumeExpansion: true
volumeBindingMode: Immediate
EOF
```
> 该操作需根据实际 region、zone 与磁盘类型调整参数，建议在非生产环境验证后再执行。

## 验证命令

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 1. PVC 已 Bound
kubectl get pvc -n middleware | grep kafka-broker-2

# 2. Pod 已 Running 并挂载云盘
kubectl get pod -n middleware kafka-broker-2 -o wide
kubectl describe pod -n middleware kafka-broker-2 | grep -A 10 "Volumes|Mounts"

# 3. Kafka broker 已加入集群
kubectl logs -n middleware kafka-broker-2 --tail=100 | grep -iE "started|broker"

# 4. Kafka 分区副本恢复
kubectl exec -n middleware kafka-broker-0 -- kafka-topics.sh --bootstrap-server localhost:9092 --describe --topic order-events

# 5. 验证新创建的 PVC 使用正确的 StorageClass
kubectl get pvc data-kafka-broker-3 -n middleware -o jsonpath='{.spec.storageClassName}'

# 6. ACK 控制台查看云盘已创建并挂载
aliyun ecs DescribeDisks \
  --RegionId cn-shanghai \
  --ZoneId cn-shanghai-f \
  --TagKey "ack.aliyun.com" \
  --output cols=DiskId,Status,Size,Type rows=Disks.Disk[]
```
## 回复客户话术

> 您好，经排查，Kafka broker `kafka-broker-2` 无法启动的根因是 **StatefulSet 中引用的 StorageClass `alicloud-disk-essd-pl1` 在集群中不存在**，导致 PVC 无法动态供给，Pod 因 `unbound immediate PersistentVolumeClaims` 一直 Pending。
>
> 我们已完成以下处置：
> 1. 将 Pending PVC 的 StorageClass 修正为集群已存在的 `alicloud-disk-essd-performance`（对应 ESSD PL1，性能满足 Kafka 需求）；
> 2. 以孤儿模式删除并重建 StatefulSet，更新 `volumeClaimTemplates` 中的 StorageClass，避免后续新 broker 复现；
> 3. 重新调度 `kafka-broker-2`，当前 Pod 已 Running 并加入 Kafka 集群。
>
> 当前 Kafka 分区副本已恢复完整。建议后续：
> - 在创建 StatefulSet 前，使用 `kubectl get storageclass` 确认 StorageClass 名称与性能等级；
> - 参考 StatefulSet 存储管理 建立中间件存储基线；
> - 配置 PVC Pending 告警，提前发现动态供给失败。
>
> 如有分区同步或性能异常，请随时联系。

## 是否需要升级及交接信息

- **是否升级**：已定位并修复，暂不需要升级；若后续发现 StorageClass 缺失是 ACK 组件未按预期创建，需升级至 **ACK 存储团队**。
- **交接信息**：
  - 故障单号：`INC-2026-ACK-028`
  - 根因：`StatefulSet volumeClaimTemplates 引用了不存在的 StorageClass`
  - 影响集群：`ack-zyy-prod-05`
  - 影响命名空间：`middleware`
  - 影响组件：Kafka StatefulSet `kafka-broker`
  - 临时修复：修正 PVC StorageClass + 重建 StatefulSet
  - 长期方案：在集群中补齐标准 StorageClass 命名，或统一使用现有 `alicloud-disk-essd-performance`
  - 待跟进：确认 Kafka 集群所有分区 ISR 完整，监控 24 小时磁盘延迟

## 复盘与沉淀

本次故障表面上是一个“PVC Pending”的存储问题，实质上反映了 **基础设施命名规范与上层应用配置之间的错位**。很多团队在 YAML 中直接写死 `alicloud-disk-essd-pl1`，但不同 ACK 集群或不同管理员创建的 StorageClass 名称可能不同（如 `alicloud-disk-essd-performance`、`alicloud-disk-essd-pl1`、`alicloud-disk-essd`）。一旦集群迁移或模板复用，就会出现此类问题。

复盘要点：
1. **StorageClass 命名标准化**：建议在集群交付文档中明确定义可用 StorageClass 列表、性能等级与使用场景，禁止业务 YAML 中随意写死名称。
2. **YAML 模板化**：对 Kafka、MySQL、Redis 等有状态中间件，使用 Helm Chart 或 Kustomize 管理 StorageClass 参数，通过 values 文件注入，而不是硬编码。
3. **PVC Pending 快速诊断**：遇到 `unbound immediate PersistentVolumeClaims`，应第一时间 `kubectl describe pvc` 查看 Event，`storageclass not found` 是最直接的原因。
4. **StatefulSet 更新策略**：`volumeClaimTemplates` 是 StatefulSet 的不可变字段，不能直接 patch。需要采用 `--cascade=orphan` 删除后重建，确保现有 Pod 与 PVC 不被误删。
5. **存储拓扑与可用区一致性检查**：在多可用区 ACK 集群中，即使 StorageClass 存在，也可能因为节点所在可用区与 PVC 拓扑约束不一致导致绑定失败。建议在创建 StatefulSet 前确认 `volumeBindingMode`：若为 `WaitForFirstConsumer`，应确保各可用区都有足够云盘配额；若为 `Immediate`，则需保证 PVC 所在可用区有节点可调度。对于 Kafka 这类对延迟敏感的中间件，最好将 broker 均匀分布到多个可用区，并配置 `podAntiAffinity`，避免单可用区故障影响整个集群。
6. **变更前在测试环境验证**：建议在非生产 ACK 集群或沙箱环境中预先验证 StatefulSet 与 PVC 的绑定行为，避免直接在生产集群中试错导致数据风险。

后续 SOP 更新要点：
- 将 StorageClass 命名规范写入 StorageClass 命名规范；
- 在 CI 中增加校验：提交 YAML 中的 `storageClassName` 必须存在于目标集群；
- 将本案例写入 PVC Pending 回复模板，提升一线响应效率。

## Related

- StatefulSet Pod 启动失败：PVC 未绑定
- StatefulSet
- 节点磁盘压力 DiskPressure 导致 Pod 被驱逐
- Pod Pending：资源不足与 Taint 不匹配


<!-- risk-assessed -->
