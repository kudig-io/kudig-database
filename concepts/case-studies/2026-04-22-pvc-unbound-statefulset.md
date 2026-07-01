---
title: "[2026-04-22] [P1] PVC 未绑定导致 StatefulSet 无法启动"
category: case-study
tags: [production, incident, storage, pvc, statefulset, csi]
date: "2026-04-22"
severity: P1
mttr: "40min"
status: resolved
created: "2026-05-23"
updated: "2026-05-23"
last_updated: 2026-05-23
---

# [2026-04-22] StorageClass 删除后 PVC 无法绑定，MySQL StatefulSet 启动失败

## 工单信息
- **工单编号**: INC-2026-0422-009
- **发现时间**: 2026-04-22 07:30 UTC
- **恢复时间**: 2026-04-22 08:10 UTC
- **影响范围**: `prod-data` namespace，`mysql-primary` StatefulSet
- **业务影响**: MySQL 主库无法启动，依赖主库的写服务全部不可用 40 分钟

## 问题现象
07:30，`mysql-primary-0` Pod 状态为 `Pending`，已持续 25 分钟：
```bash
kubectl get pods -n prod-data -l app=mysql-primary
# NAME              READY   STATUS    RESTARTS   AGE
# mysql-primary-0   0/1     Pending   0          25m
```

数据库告警：`mysql_primary_connection_count` 为 0，从库复制延迟持续增长。

## 诊断过程

**07:32** — 查看 Pod 事件：
```bash
kubectl describe pod mysql-primary-0 -n prod-data
# Events:
#   Warning  FailedScheduling  25m  ...  
#     0/20 nodes are available: 
#     20 pod has unbound immediate PersistentVolumeClaims.
```

**07:34** — 检查 PVC：
```bash
kubectl get pvc -n prod-data
# NAME                         STATUS    VOLUME   CAPACITY   STORAGECLASS
# data-mysql-primary-0         Pending                                    gp3
```

**07:35** — 查看 PVC 详情：
```bash
kubectl describe pvc data-mysql-primary-0 -n prod-data
# Events:
#   Warning  ProvisioningFailed  25m  ...  
#     failed to provision volume with StorageClass "gp3": 
#     storageclass.storage.k8s.io "gp3" not found
```

**07:37** — 检查 StorageClass：
```bash
kubectl get storageclass
# NAME            PROVISIONER             RECLAIMPOLICY   VOLUMEBINDINGMODE
# gp2             kubernetes.io/aws-ebs   Delete          Immediate
# fast-ssd        ebs.csi.aws.com         Delete          WaitForFirstConsumer
# （gp3 StorageClass 已不存在）
```

**07:39** — 查看变更历史：
```bash
# 检查 ArgoCD 同步记录
# 04-21 23:00 的 "cleanup unused resources" 任务删除了 gp3 StorageClass
# 原因：运维团队认为 gp3 已被 gp2 替代，且没有 PVC 使用 gp3
```

**07:41** — 进一步排查：
```bash
# 原来 StatefulSet 的 volumeClaimTemplates 仍引用 gp3
kubectl get statefulset mysql-primary -n prod-data -o yaml | grep -A10 volumeClaimTemplates
# volumeClaimTemplates:
# - metadata:
#     name: data
#   spec:
#     accessModes: ["ReadWriteOnce"]
#     storageClassName: gp3
#     resources:
#       requests:
#         storage: 100Gi
```

## 根因
1. `mysql-primary` StatefulSet 的 `volumeClaimTemplates` 使用 `storageClassName: gp3`
2. 04-21 23:00，运维团队清理未使用资源时删除了 `gp3` StorageClass
3. 04-22 07:05，`mysql-primary-0` 因磁盘压力触发 eviction，被调度到新节点
4. 新节点上 PVC `data-mysql-primary-0` 需要重新绑定，但 `gp3` StorageClass 已不存在，PVC 处于 `Pending`
5. StatefulSet 控制器无法启动 Pod，MySQL 主库宕机

## 修复动作

**07:45** — 恢复 gp3 StorageClass：

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

```bash
cat <<'EOF' | kubectl apply -f -
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: gp3
provisioner: ebs.csi.aws.com
parameters:
  type: gp3
  encrypted: "true"
reclaimPolicy: Delete
allowVolumeExpansion: true
volumeBindingMode: WaitForFirstConsumer
EOF
```

**07:50** — 观察 PVC 绑定：
```bash
kubectl get pvc data-mysql-primary-0 -n prod-data -w
# NAME                         STATUS    VOLUME                                     CAPACITY
# data-mysql-primary-0         Bound     pvc-abc123-def456-ghi789                  100Gi
```

**07:52** — Pod 启动：
```bash
kubectl get pod mysql-primary-0 -n prod-data
# NAME              READY   STATUS    RESTARTS   AGE
# mysql-primary-0   1/1     Running   0          2m
```

**07:55** — 验证 MySQL 健康：

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

```bash
kubectl exec -n prod-data mysql-primary-0 -- mysql -u root -p$PASSWORD -e "SHOW STATUS LIKE 'Uptime';"
# +---------------+-------+
# | Variable_name | Value |
# +---------------+-------+
# | Uptime        | 120   |
# +---------------+-------+
```

**08:00** — 检查从库复制延迟：

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

```bash
kubectl exec -n prod-data mysql-replica-0 -- mysql -u root -p$PASSWORD -e "SHOW SLAVE STATUS\G" | grep Seconds_Behind_Master
# Seconds_Behind_Master: 1800
```

**08:05** — 延迟恢复正常：

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

```bash
kubectl exec -n prod-data mysql-replica-0 -- mysql -u root -p$PASSWORD -e "SHOW SLAVE STATUS\G" | grep Seconds_Behind_Master
# Seconds_Behind_Master: 0
```

## 验证
- 08:08 — 写服务全部恢复，订单写入正常
- 08:10 — MySQL 主从复制延迟归零，数据一致性验证通过

## 复盘
- **直接原因**: gp3 StorageClass 被误删 → PVC 无法绑定 → StatefulSet Pod Pending → MySQL 宕机
- **根本原因**: 资源清理脚本未检查 StatefulSet 的 volumeClaimTemplates 引用
- **改进措施**:
  1. 删除 StorageClass 前执行依赖检查：`kubectl get pvc,pv --all-namespaces -o json | jq '.items[].spec.storageClassName'`
  2. 为 StorageClass 添加 `storageclass.kubernetes.io/is-default-class: "true"` 注解，优先使用默认 SC
  3. StatefulSet 的 volumeClaimTemplates 使用 `storageClassName: ""`（使用默认 SC），不硬编码 SC 名称
  4. 删除任何集群级资源前必须经过 SRE 变更评审
- **相关 Skill**: [[manage-persistent-storage]]
- **相关 FTA**: [[csi-fta]]
