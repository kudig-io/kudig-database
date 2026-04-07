# Volume Attributes Classes（卷属性类）

## 概述

VolumeAttributesClass（VAC）是 Kubernetes 在 v1.34 中达到 GA（默认启用）的一项功能，它允许管理员定义存储的可变“属性类”。与 StorageClass 主要关注卷的初始供给不同，VolumeAttributesClass 关注的是已创建卷的属性修改，例如调整 IOPS 或吞吐量。

## 核心概念/原理

- **可变存储属性**：VolumeAttributesClass 提供了一种机制，用于描述存储卷在生命周期内可以变更的性能或服务等级。
- **CSI 驱动依赖**：该功能仅适用于由 CSI 驱动支持的存储，并且要求 CSI 驱动实现 `ModifyVolume` API。
- **独立资源**：VolumeAttributesClass 是集群级别的资源，通过 `driverName` 关联到具体的 CSI 驱动。

## 关键机制或特性

### API 结构

```yaml
apiVersion: storage.k8s.io/v1
kind: VolumeAttributesClass
metadata:
  name: silver
driverName: pd.csi.storage.gke.io
parameters:
  provisioned-iops: "3000"
  provisioned-throughput: "50"
```

- `driverName`：指定用于供给和修改卷的 CSI 驱动名称。
- `parameters`：描述卷的存储属性参数，参数不可变（创建后不能修改）。

### 与 PVC 的关联

- 用户在 PVC 中通过 `volumeAttributesClassName` 字段指定要使用的 VolumeAttributesClass。
- 该字段在 PVC 中是**可变的**，允许用户切换不同的 VolumeAttributesClass 来触发卷属性修改。

### 修改流程示例

1. 现有 PVC 使用 `silver` 类：
   ```yaml
   spec:
     volumeAttributesClassName: silver
   ```
2. 集群管理员创建了新的 `gold` 类（更高 IOPS/吞吐量）。
3. 用户更新 PVC：
   ```yaml
   spec:
     volumeAttributesClassName: gold
   ```
4. CSI external-resizer 检测到变化后，调用 CSI 驱动的 `ModifyVolume` API 修改底层卷属性。

### Provisioner 与 Resizer

- **Provisioner**：由 `kubernetes-csi/external-provisioner` 实现，在动态供给时应用 VolumeAttributesClass 的参数。
- **Resizer**：由 `kubernetes-csi/external-resizer` 实现，在 PVC 的 `volumeAttributesClassName` 变更时调用 `ModifyVolume`。

### 参数限制

- 最多 512 个参数。
- 参数对象（键和值）的总长度不得超过 256 KiB。

## 使用场景

- **动态性能调整**：业务高峰期将卷从低性能等级（silver）切换到高性能等级（gold），提升 IOPS 和吞吐量。
- **存储成本优化**：在低负载期间降级卷属性以节省存储成本。
- **细粒度服务质量管理**：为不同应用提供差异化的存储性能等级，并在生命周期内灵活变更。

## 最佳实践/注意事项

- 确保所使用的 CSI 驱动已支持 `ModifyVolume` API，否则无法生效。
- VolumeAttributesClass 本身的 `parameters` 一旦创建不可变更；如需调整参数，需要创建新的 VolumeAttributesClass 并更新 PVC 引用。
- 参数的具体键值对完全取决于 CSI 驱动的实现，使用前请查阅对应驱动的文档。
- 如果集群中不需要动态修改卷属性，可以选择禁用该特性门。

## 生产 YAML 示例

### 分级 VolumeAttributesClass + PVC 动态切换

```yaml
# Silver 级别 — 标准性能
apiVersion: storage.k8s.io/v1
kind: VolumeAttributesClass
metadata:
  name: silver
driverName: ebs.csi.aws.com
parameters:
  iops: "3000"
  throughput: "125"
---
# Gold 级别 — 高性能
apiVersion: storage.k8s.io/v1
kind: VolumeAttributesClass
metadata:
  name: gold
driverName: ebs.csi.aws.com
parameters:
  iops: "10000"
  throughput: "500"
---
# PVC 使用 Silver 级别
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: db-data
  namespace: database
spec:
  accessModes: ["ReadWriteOnce"]
  storageClassName: gp3-encrypted
  volumeAttributesClassName: silver         # 初始使用 Silver
  resources:
    requests:
      storage: 200Gi
```

```bash
# 业务高峰期切换到 Gold 级别
kubectl patch pvc db-data -n database \
  -p '{"spec":{"volumeAttributesClassName":"gold"}}'
```

## 故障排查

| 症状 | 可能原因 | 排查步骤 |
|------|----------|----------|
| VAC 切换后性能未变 | CSI 驱动不支持 ModifyVolume | 确认 CSI 驱动版本支持 ModifyVolume API |
| PVC 状态显示 ModifyVolumeFailed | 底层存储不支持指定参数 | 检查 PVC conditions；查看 external-resizer 日志 |

## 生产检查清单

- [ ] CSI 驱动支持 ModifyVolume API
- [ ] 建立 Silver/Gold/Platinum 分级体系
- [ ] 参数值参考 CSI 驱动文档

## 命令快速参考

```bash
# 查看 VolumeAttributesClass
kubectl get volumeattributesclasses

# 切换 PVC 的 VAC
kubectl patch pvc <name> -n <ns> -p '{"spec":{"volumeAttributesClassName":"gold"}}'

# 查看 PVC 当前 VAC
kubectl get pvc <name> -o jsonpath='{.spec.volumeAttributesClassName}'
```

## 交叉引用

- [存储类](./storage-classes.md) — StorageClass 关注初始供给，VAC 关注运行时修改
- [持久卷](./persistent-volumes.md) — PVC 引用 VAC

## 参考链接

- https://kubernetes.io/docs/concepts/storage/volume-attributes-classes/
