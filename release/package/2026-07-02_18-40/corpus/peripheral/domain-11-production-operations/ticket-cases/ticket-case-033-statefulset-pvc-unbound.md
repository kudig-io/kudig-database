---
title: StatefulSet Pod 启动失败：PVC 未绑定
description: 专有云 ACK 有状态 MySQL 集群因 StorageClass 可用区拓扑限制导致 PVC 无法动态供给，Pod 长期 Pending
  的工单闭环样本。
summary: 专有云 ACK 有状态 MySQL 集群因 StorageClass 可用区拓扑限制导致 PVC 无法动态供给，Pod 长期 Pending 的工单闭环样本。
category: production-operations
tags:
- ack
- zyy
- statefulset
- pvc
- mysql
- csi
- storageclass
- p1
tier: peripheral
created: '2026-06-26T14:00:00+08:00'
updated: '2026-06-26T16:30:00+08:00'
incident_id: TC-2026-033
priority: P1
severity: high
affected_cluster: ack-zyy-prod-01
affected_namespace: middleware
ticket_type: 有状态应用启动失败
skill_ref:
- '[[domain-04-storage-data/01-k8s-storage/09-pv-pvc-troubleshooting.md|PV/PVC 排障]]'
- '[[domain-02-workloads-applications/00-core-workloads/03-statefulset-advanced-operations.md|StatefulSet
  进阶运维]]'
fta_ref:
- '[[domain-10-troubleshooting-diagnostics/topic-fta/list/statefulset-fta.md|FTA:
  StatefulSet 启动失败]]'
- '[[domain-10-troubleshooting-diagnostics/topic-fta/list/csi-fta.md|FTA: CSI 存储异常]]'
last_updated: 2026-06-26 16:30:00+08:00
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
- mysql
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
- target: '[[domain-11-production-operations/ticket-cases/ticket-case-043-statefulset-pvc-unbound.md]]'
  type: related_to
- target: '[[concepts/statefulset.md]]'
  type: related_to
- target: '[[domain-11-production-operations/ticket-cases/ticket-case-040-node-diskpressure-eviction.md]]'
  type: related_to
- target: '[[domain-11-production-operations/ticket-cases/ticket-case-042-pod-pending-resource-taint.md]]'
  type: related_to
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 工单描述

客户反馈中间件团队部署的 MySQL 主从集群在 `middleware` 命名空间无法启动，`mysql-0` Pod 一直处于 Pending。客户描述如下：

> “我们用 StatefulSet 部署的 MySQL，pvc 一直是 Pending，describe pvc 看到 `Failed to provision volume with StorageClass "alicloud-disk-ssd": No available disk in zone cn-beijing-c`。Pod 调度到了 c 区，但 c 区好像没有 ESSD 了。之前 a、b 区都正常，今天缩容后新增节点被调度到了 c 区就出问题了。麻烦尽快看一下，数据库起不来影响多个业务读写。”

该集群为专有云 `ack-zyy-prod-01`，跨可用区 a/b/c 部署，使用阿里云 CSI 插件 `alicloud-disk-csi` 进行 ESSD 动态供给。

## 分类与优先级判定

- **工单类型**：有状态应用启动失败 / 存储供给失败。
- **优先级**：P1。
- **严重级别**：high。

判定依据：
1. 生产环境 MySQL 有状态服务无法启动，影响下游多个业务读写。
2. PVC Pending 直接阻塞 StatefulSet 滚动上线，根因指向存储可用区拓扑。
3. 需要在 15 分钟内给出是将 Pod 迁移到其他可用区还是扩容 c 区存储的方案。

## 诊断步骤

按“先看 PVC/PV 状态、再看 CSI 日志、最后看可用区拓扑”的顺序排查：

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 1. 确认 StatefulSet 与 Pod 状态
kubectl get sts mysql -n middleware
kubectl get pod mysql-0 -n middleware -o wide

# 2. 查看 PVC 状态与事件
kubectl get pvc -n middleware
kubectl describe pvc data-mysql-0 -n middleware | tail -50

# 3. 检查 StorageClass 与拓扑限制
kubectl get storageclass alicloud-disk-ssd -o yaml

# 4. 采集 CSI 插件日志
kubectl logs -n kube-system -l app=csi-plugin -c csi-plugin --tail=200 | grep -i 'provision|error|zone'
kubectl logs -n kube-system -l app=csi-provisioner -c csi-provisioner --tail=200 | grep mysql

# 5. 查询节点可用区标签
kubectl get nodes -L topology.kubernetes.io/zone -l nodepool-id=np-zyy-middleware

# 6. 通过阿里云 ECS 查询可用区磁盘库存
aliyun ecs DescribeAvailableResource \
  --RegionId cn-beijing \
  --DestinationResource DataDisk \
  --ZoneId cn-beijing-c \
  --InstanceType ecs.c7.2xlarge

# 7. 查看已挂载磁盘
aliyun ecs DescribeDisks \
  --RegionId cn-beijing \
  --ZoneId cn-beijing-c \
  --output cols=DiskId,Status,Size rows=Disks.Disk[]
```
## 根因分析

综合 PVC 事件、CSI 日志与可用区资源查询，判定根因为 **StatefulSet 的 Pod 被调度至 c 区，但 c 区当前无 ESSD 云盘库存，导致 alicloud-disk-csi 无法动态创建 PV**，置信度 **高**。

1. **调度拓扑与存储拓扑不一致**：节点池 `np-zyy-middleware` 近期缩容后，新扩容节点因库存原因仅落到 c 区，而 `alicloud-disk-ssd` StorageClass 未显式限制可用区，但底层 ESSD 在 c 区已售罄。
2. **StatefulSet volumeClaimTemplates 不可变更**：StatefulSet 创建后 PVC 模板无法直接修改，因此不能简单切换 StorageClass，需要删除 StatefulSet（保留 Pod/孤儿 PVC）后重建，或迁移 Pod 到 a/b 区。
3. **业务影响**：MySQL 主节点 `mysql-0` 无法启动，导致只读从库可能晋升失败，影响写入口。

## 修复命令

**第一步：将 c 区节点标记为不可调度，避免新 Pod 再落入无盘区**

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `kubectl cordon`：标记节点不可调度

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
kubectl cordon cn-beijing.172.16.4.30
```
**第二步：删除 StatefulSet（孤儿模式保留 Pod，避免数据丢失）**

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl delete`：删除资源（可由声明式清单重建）

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl delete sts mysql -n middleware --cascade=orphan
```
**第三步：删除无法绑定的 Pending PVC**

> 注：此 PVC 尚未绑定，无数据，可直接删除；若已存在数据请先备份。

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
kubectl delete pvc data-mysql-0 -n middleware
```
**第四步：为 Pod 增加可用区亲和性，强制调度到 a/b 区**

修改 StatefulSet YAML 后重新 apply：

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
cat <<'EOF' | kubectl apply -f -
apiVersion: apps/v1
kind: StatefulSet
metadata:
  name: mysql
  namespace: middleware
spec:
  serviceName: mysql
  replicas: 3
  selector:
    matchLabels:
      app: mysql
  template:
    metadata:
      labels:
        app: mysql
    spec:
      affinity:
        nodeAffinity:
          requiredDuringSchedulingIgnoredDuringExecution:
            nodeSelectorTerms:
              - matchExpressions:
                  - key: topology.kubernetes.io/zone
                    operator: In
                    values:
                      - cn-beijing-a
                      - cn-beijing-b
      containers:
        - name: mysql
          image: mysql:8.0.36
          env:
            - name: MYSQL_ROOT_PASSWORD
              valueFrom:
                secretKeyRef:
                  name: mysql-secret
                  key: root-password
          volumeMounts:
            - name: data
              mountPath: /var/lib/mysql
  volumeClaimTemplates:
    - metadata:
        name: data
      spec:
        accessModes: ["ReadWriteOnce"]
        storageClassName: alicloud-disk-ssd
        resources:
          requests:
            storage: 100Gi
EOF
```
**第五步：验证新 PVC 自动创建并绑定**

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl get pvc -n middleware -w
kubectl get pod mysql-0 -n middleware -o wide
```
**备选长期方案：为 c 区扩容 ESSD 库存**

若业务要求三可用区部署，可在 c 区提交 ESSD 扩容工单，库存恢复后去除 nodeAffinity 限制。

## 验证命令

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 1. PVC 全部 Bound
kubectl get pvc -n middleware -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{.status.phase}{"\n"}{end}'

# 2. MySQL Pod 全部 Running
kubectl get pod -n middleware -l app=mysql -o wide

# 3. MySQL 主从同步状态
kubectl exec -n middleware mysql-0 -- mysql -uroot -p$(kubectl get secret mysql-secret -n middleware -o jsonpath='{.data.root-password}' | base64 -d) -e "SHOW SLAVE STATUS\G" | grep -E 'Slave_IO_Running|Slave_SQL_Running'

# 4. 业务侧连接测试
kubectl run mysql-client --rm -it --restart=Never -n middleware --image=mysql:8.0.36 -- \
  mysql -hmysql.middleware.svc.cluster.local -uroot -p$(kubectl get secret mysql-secret -n middleware -o jsonpath='{.data.root-password}' | base64 -d) -e "SELECT 1;"

# 5. CSI 无新 provision 错误
kubectl logs -n kube-system -l app=csi-plugin -c csi-plugin --tail=100 | grep -i error || echo "无新错误"
```
## 回复客户话术

> 您好，工单 TC-2026-033 已处理完成。
>
> **现象确认：** MySQL StatefulSet `mysql` 在 `middleware` 命名空间无法启动，`mysql-0` 对应的 PVC 长期处于 Pending。
>
> **根因：** `mysql-0` 被调度到 c 区节点，但 c 区 ESSD 云盘当前无库存，CSI 插件 `alicloud-disk-csi` 无法动态创建 PV，导致 PVC 无法绑定，进而阻塞 Pod 启动。
>
> **已执行修复：**
> 1. 将 c 区问题节点临时 cordon，避免新 Pod 继续落入无盘区；
> 2. 以孤儿模式删除 StatefulSet（保留数据 Pod）；
> 3. 删除未绑定的 Pending PVC；
> 4. 在 StatefulSet 中增加可用区 nodeAffinity，限定 Pod 仅调度到 a/b 区；
> 5. 重新创建 StatefulSet，新 PVC 成功绑定，MySQL 主节点启动。
>
> **当前状态：** `mysql-0/1/2` 全部 Running，主从同步状态正常，业务连接测试通过。
>
> **后续建议：**
> - 参考 [[domain-04-storage-data/01-k8s-storage/09-pv-pvc-troubleshooting.md|PV/PVC 排障]] 建立 StorageClass 与可用区库存的联动监控；
> - 为 `alicloud-disk-ssd` StorageClass 配置 `allowedTopologies` 或 StatefulSet 中显式 topology 约束，避免调度到无盘区；
> - 若需三可用区部署，请提交 c 区 ESSD 库存扩容工单，库存恢复后再解除 nodeAffinity 限制；
> - 定期演练 MySQL StatefulSet 备份恢复，确保数据可恢复。
>
> 如有异常请随时联系。

## 复盘与沉淀

有状态服务在云盘可用区库存、节点拓扑与调度策略之间耦合紧密。StatefulSet 的 volumeClaimTemplates 一旦创建不可变更，因此最好在首次部署时就明确可用区约束，例如通过 StorageClass `allowedTopologies` 或在 Pod 模板中设置 `nodeAffinity`。对于跨三可用区部署的数据库，应持续监控各可用区 ESSD 库存，并在库存不足时及时 cordon 对应节点或切换拓扑。

删除 StatefulSet 时使用 `--cascade=orphan` 可以保留 Pod，避免误删已绑定数据盘。但在重新创建前，必须确认旧 PVC 是否有数据：若 PVC 已 Bound 且含数据，应通过快照或备份迁移，而不是直接删除。建议将 MySQL 等关键有状态应用的备份、快照与恢复流程纳入常规演练。

针对专有云场景，还应与 IaaS 侧建立可用区库存同步机制：当某个可用区 ESSD 库存低于阈值时，自动在对应节点添加 `disk-full` 污点或 cordon 节点，防止新 Pod 落入无盘区。运维脚本可定期调用 `aliyun ecs DescribeAvailableResource` 并将结果写入 ConfigMap，供调度器或外部控制器读取。

在每次对有状态应用做拓扑变更前，建议先在 ACK 控制台创建云盘快照，并验证快照可恢复性。变更过程中使用 `kubectl get pvc -w` 实时观察绑定状态，一旦发现 Pending 超过 5 分钟立即回滚拓扑约束并检查 CSI 日志。

## 是否需要升级及交接信息

- **是否升级**：已闭环，无需升级。若 c 区 ESSD 库存长期不足且业务要求三可用区，需升级至 **存储基础设施团队** 协调扩容。
- **是否需要变更审批**：是（修改 StatefulSet 拓扑约束涉及有状态服务部署变更，已登记变更台账）。
- **交接信息**：
  - 故障单号：`TC-2026-033`
  - 根因：`c 区 ESSD 库存不足导致 PVC 无法动态供给`
  - 影响命名空间：`middleware`
  - 修复动作：cordon c 区节点 + StatefulSet 增加可用区 nodeAffinity + 重建 PVC
  - 待跟进：监控 c 区 ESSD 库存恢复情况，评估是否解除 a/b 区限制

## Related

- StatefulSet Pod 启动失败：PVC 未绑定
- StatefulSet
- 节点磁盘压力 DiskPressure 导致 Pod 被驱逐
- Pod Pending：资源不足与 Taint 不匹配


<!-- risk-assessed -->
