---
title: PVC 扩容指南
summary: PVC 扩容指南：随着业务增长，存储空间不足是生产环境常见问题。Kubernetes 支持 PVC（Persistent Volume Claim）的动态扩容，但需满足特定前提条件，且不同存储类型的扩容特性差异显著。
category: 存储
tags:
- domain-04
- 存储
- PVC
- 扩容
- 云盘
- NAS
- CSI
- visibility/public
tier: supporting
sources:
- KUDIG Gap Analysis 2026-05-21
created: 2026-05-21
updated: 2026-05-21
last_updated: 2026-05-21
status: reviewed
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# PVC 扩容指南

## 概述

随着业务增长，存储空间不足是生产环境常见问题。Kubernetes 支持 PVC（Persistent Volume Claim）的动态扩容，但需满足特定前提条件，且不同存储类型的扩容特性差异显著。

## 扩容前提条件

### StorageClass 配置

```yaml
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: expandable-ssd
provisioner: diskplugin.csi.alibabacloud.com
parameters:
  type: cloud_ssd
allowVolumeExpansion: true   # 必须设置为 true
```

`allowVolumeExpansion: true` 是 PVC 扩容的必要条件。如为 `false`，则无法在线扩容，只能重建 PVC。

### 支持的存储类型

| 存储类型 | 是否支持扩容 | 扩容方式 | 备注 |
|---|---|---|---|
| 云盘（ESSD/SSD） | 是 | 在线扩容 | 需 CSI 驱动支持 |
| NAS | 是 | 在线扩容 | 修改配额即可 |
| 本地盘 | 否 | 不支持 | 需重建 Pod + 数据迁移 |
| NFS | 依赖实现 | 通常在线 | 取决于 NFS Server 配置 |

## 在线扩容 vs 离线扩容

### 在线扩容（推荐）

Pod 无需重启，存储层直接扩展容量：

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 1. 修改 PVC 的 spec.resources.requests.storage
kubectl patch pvc my-pvc -p '{"spec":{"resources":{"requests":{"storage":"100Gi"}}}}'

# 2. 检查扩容状态
kubectl get pvc my-pvc -w

# 3. 进入 Pod 扩展文件系统（ext4/xfs）
kubectl exec -it my-pod -- resize2fs /dev/sda
```
**条件**：
- StorageClass 支持在线扩容
- CSI 驱动实现了 `ExpandVolume` 接口
- 文件系统支持在线扩展（ext4、XFS 均支持）

### 离线扩容

需要停止 Pod，解除挂载后扩容：

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `kubectl scale --replicas=0`：缩容到 0，立即停服
> - `kubectl edit/patch`：修改运行中的资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 1. 缩容 StatefulSet 至 0
kubectl scale sts my-app --replicas=0

# 2. 修改 PVC 容量
kubectl patch pvc data-my-app-0 -p '{"spec":{"resources":{"requests":{"storage":"100Gi"}}}}'

# 3. 扩容文件系统（节点上执行）
resize2fs /dev/vdb

# 4. 恢复 Pod
kubectl scale sts my-app --replicas=1
```
适用场景：CSI 驱动不支持在线扩容，或存储类型限制。

## 阿里云 ACK 扩容

### 云盘在线扩容流程

阿里云 ACK 使用 `diskplugin.csi.alibabacloud.com` 作为 CSI provisioner：

1. 确认 StorageClass 已启用扩容：
   ```bash
   kubectl get sc alicloud-disk-ssd -o jsonpath='{.allowVolumeExpansion}'
   ```

2. 修改 PVC 容量（云盘只能扩大不能缩小）

3. CSI 插件自动调用阿里云 OpenAPI 扩容云盘

4. 进入 Pod 执行文件系统扩展

### NAS 扩容

NAS 扩容本质是修改文件系统配额：

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# NAS PVC 扩容后无需 resize 文件系统
kubectl patch pvc nas-pvc -p '{"spec":{"resources":{"requests":{"storage":"500Gi"}}}}'
```
NAS 扩容通常即时生效，但受限于 NAS 实例的总容量规格。

## 扩容失败排查

| 失败现象 | 根因 | 解决方案 |
|---|---|---|
| `Resizing` 状态卡住 | CSI 驱动未实现扩容 | 升级 CSI 插件 |
| 云盘扩容成功但文件系统未扩展 | 未执行 resize2fs/xfs_growfs | 手动扩展文件系统 |
| 报错 "only dynamically provisioned pvc can be resized" | PV 非动态供应 | 重建 PVC + 迁移数据 |
| 配额超限 | 云账号磁盘配额不足 | 申请提升配额或释放闲置磁盘 |
| 文件系统类型不支持 | 如使用 VFAT、NTFS | 重建为 ext4/XFS |

## 替代方案：新建 PVC + 数据迁移

当扩容不可行时（如本地盘、不支持扩容的存储），采用替代方案：

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 1. 创建新 PVC（更大容量）
kubectl apply -f new-larger-pvc.yaml

# 2. 启动临时 Pod，同时挂载新旧 PVC
# 3. 执行数据复制（rsync 或 cp）
# 4. 更新 StatefulSet/Deployment 使用新 PVC
# 5. 验证数据完整性后删除旧 PVC
```
> 数据迁移期间建议暂停写入或切换至只读模式，避免数据不一致。

## 远程顾问指导要点

远程顾问无法直接操作存储，需通过以下方式指导客户：

1. **前置检查**：扩容前确认 `StorageClass.allowVolumeExpansion` 和 CSI 驱动版本
2. **扩容监控**：指导客户执行 `kubectl get pvc -w` 观察扩容过程，卡住时收集 CSI 插件日志
3. **失败判断**：根据报错信息快速判定是存储层问题还是文件系统层问题
4. **替代方案评估**：当在线扩容不可行时，评估数据迁移的可行性（数据量大小、停机时间容忍度）

> 存储扩容涉及数据安全，任何操作前务必确认备份已完成。远程顾问应要求客户提供扩容前后的 `df -h` 和 `kubectl get pvc` 输出作为验证依据。

## 自动化容量监控与告警

### PrometheusRule — PVC 容量告警

```yaml
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: pvc-capacity-alerts
  namespace: monitoring
spec:
  groups:
    - name: pvc-capacity
      rules:
        - alert: PVCSpaceRunningLow
          expr: |
            kubelet_volume_stats_available_bytes / kubelet_volume_stats_capacity_bytes < 0.15
          for: 10m
          labels:
            severity: warning
          annotations:
            summary: "PVC {{ $labels.namespace }}/{{ $labels.persistentvolumeclaim }} 剩余空间 < 15%"
            runbook_url: "https://wiki.internal/runbooks/pvc-expansion"

        - alert: PVCSpaceCritical
          expr: |
            kubelet_volume_stats_available_bytes / kubelet_volume_stats_capacity_bytes < 0.05
          for: 5m
          labels:
            severity: critical
          annotations:
            summary: "❗ PVC {{ $labels.namespace }}/{{ $labels.persistentvolumeclaim }} 即将写满 (< 5%)"

        - alert: PVCExpansionInProgress
          expr: |
            kube_persistentvolumeclaim_status_condition{condition="FileSystemResizePending"} == 1
          for: 30m
          labels:
            severity: warning
          annotations:
            summary: "PVC {{ $labels.persistentvolumeclaim }} 文件系统扩容等待 >30min"

        - alert: PVCInodeExhaustion
          expr: |
            kubelet_volume_stats_inodes_used / kubelet_volume_stats_inodes > 0.9
          for: 15m
          labels:
            severity: warning
          annotations:
            summary: "PVC {{ $labels.persistentvolumeclaim }} inode 使用率 > 90%"
```

### 容量趋势预测

```promql
# 预测 PVC 何时写满（线性回归）
predict_linear(
  kubelet_volume_stats_available_bytes{namespace="production"}[7d],
  7 * 24 * 3600  # 预测 7 天后
) < 0

# 按命名空间统计存储使用
sum by (namespace) (kubelet_volume_stats_used_bytes) / 1024^3  # GiB

# Top 10 容量使用 PVC
topk(10,
  kubelet_volume_stats_used_bytes / kubelet_volume_stats_capacity_bytes
)
```

## StatefulSet 批量扩容

### 批量扩容脚本

```bash
#!/bin/bash
# 🟡 中风险：StatefulSet PVC 批量扩容
STS_NAME=$1
NEW_SIZE=$2
NS=${3:-default}

if [ -z "$STS_NAME" ] || [ -z "$NEW_SIZE" ]; then
  echo "用法: $0 <statefulset-name> <new-size> [namespace]"
  echo "示例: $0 postgres 200Gi production"
  exit 1
fi

echo "=== StatefulSet PVC 批量扩容 ==="
echo "目标: $STS_NAME → $NEW_SIZE"

# 1. 检查 StorageClass 是否支持扩容
SC=$(kubectl get pvc -n $NS -l app=$STS_NAME -o jsonpath='{.items[0].spec.storageClassName}')
EXPANDABLE=$(kubectl get sc $SC -o jsonpath='{.allowVolumeExpansion}')
if [ "$EXPANDABLE" != "true" ]; then
  echo "❌ StorageClass $SC 不支持扩容 (allowVolumeExpansion=$EXPANDABLE)"
  exit 1
fi
echo "✅ StorageClass $SC 支持扩容"

# 2. 获取所有 PVC
PVCS=$(kubectl get pvc -n $NS -l app=$STS_NAME -o name)
COUNT=$(echo "$PVCS" | wc -l | tr -d ' ')
echo "找到 $COUNT 个 PVC"

# 3. 逐个扩容
for pvc in $PVCS; do
  NAME=${pvc#persistentvolumeclaim/}
  CURRENT=$(kubectl get pvc $NAME -n $NS -o jsonpath='{.spec.resources.requests.storage}')
  echo "  扩容 $NAME: $CURRENT → $NEW_SIZE"
  kubectl patch pvc $NAME -n $NS \
    -p "{\"spec\":{\"resources\":{\"requests\":{\"storage\":\"$NEW_SIZE\"}}}}"
done

# 4. 等待扩容完成
echo ""
echo "等待扩容完成..."
sleep 10
kubectl get pvc -n $NS -l app=$STS_NAME -o custom-columns=
  NAME:.metadata.name,SIZE:.spec.resources.requests.storage,STATUS:.status.phase

echo "=== 完成 ==="
```

## 扩容验证检查单

```bash
#!/bin/bash
# 🟢 只读：PVC 扩容后验证
PVC=$1
NS=${2:-default}

echo "=== PVC 扩容验证: $NS/$PVC ==="

# 1. PVC 状态
echo "[1/5] PVC 状态:"
kubectl get pvc $PVC -n $NS -o custom-columns=
  NAME:.metadata.name,CAPACITY:.status.capacity.storage,PHASE:.status.phase

# 2. 条件检查
echo "[2/5] 扩容条件:"
kubectl get pvc $PVC -n $NS -o jsonpath='{range .status.conditions[*]}{.type}: {.status} ({.reason}){"\n"}{end}'

# 3. PV 实际容量
echo "[3/5] PV 实际容量:"
PV=$(kubectl get pvc $PVC -n $NS -o jsonpath='{.spec.volumeName}')
kubectl get pv $PV -o jsonpath='{.spec.capacity.storage}'
echo ""

# 4. 文件系统容量（需要 Pod 运行中）
echo "[4/5] 文件系统容量:"
POD=$(kubectl get pods -n $NS -o jsonpath='{.items[0].metadata.name}' -l app=$PVC 2>/dev/null)
if [ -n "$POD" ]; then
  kubectl exec $POD -n $NS -- df -h 2>/dev/null | grep -v tmpfs | head -5
else
  echo "  无运行中的 Pod，跳过"
fi

# 5. 事件检查
echo "[5/5] 最近事件:"
kubectl get events -n $NS --field-selector involvedObject.name=$PVC --sort-by='.lastTimestamp' | tail -5

echo "=== 验证完成 ==="
```

## 容量规划与自动扩容

### 容量规划公式

```
所需容量 = 当前使用量 / 目标利用率 × 增长系数

示例:
- 当前使用: 80Gi
- 目标利用率: 70%
- 6 个月增长系数: 1.5
- 所需容量 = 80 / 0.7 × 1.5 ≈ 172Gi → 申请 200Gi
```

### KEDA 自动扩容 PVC（概念）

```yaml
# 基于容量使用率触发扩容的 CronJob
apiVersion: batch/v1
kind: CronJob
metadata:
  name: pvc-auto-expander
  namespace: platform
spec:
  schedule: "0 */6 * * *"  # 每 6 小时检查
  jobTemplate:
    spec:
      template:
        spec:
          serviceAccountName: pvc-expander
          containers:
            - name: expander
              image: bitnami/kubectl:latest
              env:
                - name: THRESHOLD
                  value: "85"  # 使用率阈值 %
                - name: EXPANSION_FACTOR
                  value: "1.5"  # 扩容倍数
                - name: MAX_SIZE_GI
                  value: "2000"  # 最大容量
              command:
                - /bin/sh
                - -c
                - |
                  echo "检查 PVC 容量使用率..."
                  kubectl get pvc -A -o json | jq -r '
                    .items[] |
                    select(.status.capacity.storage != null) |
                    "\(.metadata.namespace) \(.metadata.name) \(.status.capacity.storage)"
                  ' | while read NS PVC CAP; do
                    USED=$(kubectl exec -n $NS $(kubectl get pods -n $NS -o name | head -1) -- \
                      df -B1 /data 2>/dev/null | tail -1 | awk '{print $3}')
                    if [ -n "$USED" ]; then
                      PCT=$((USED * 100 / ${CAP%Gi} / 1073741824))
                      if [ $PCT -gt $THRESHOLD ]; then
                        NEW_SIZE=$(echo "${CAP%Gi} * $EXPANSION_FACTOR / 1" | bc)
                        echo "扩容 $NS/$PVC: ${CAP} → ${NEW_SIZE}Gi (使用率 ${PCT}%)"
                        kubectl patch pvc $PVC -n $NS \
                          -p "{\"spec\":{\"resources\":{\"requests\":{\"storage\":\"${NEW_SIZE}Gi\"}}}}"
                      fi
                    fi
                  done
          restartPolicy: OnFailure
```

## 相关链接

- [[storage-tool-evolution]] — 存储工具的演进
- [[persistent-volume-claim]] — PVC 原理与配置
- [[07-数据库中间件/00-总览/01-database-on-kubernetes-guide.md|database-on-kubernetes-guide]] — K8s 上的数据库运行指南
- [[19-故障诊断/04-高级排障/05-workloads/03-statefulset-troubleshooting.md|statefulset-troubleshooting]] — StatefulSet 问题排查

## Related

- [[visibility-public|#visibility/public Hub]] — tag hub


<!-- risk-assessed -->
