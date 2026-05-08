# Kubernetes 存储体系全栈进阶培训 (从入门到专家)

> **适用版本**: Kubernetes v1.28 - v1.32 | **文档类型**: 全栈技术实战指南
> **目标受众**: 初级运维、存储架构师、SRE
> **核心原则**: 理解持久化本质、掌握 CSI 挂载机制、确保数据容灾闭环
> **培训时长**: 约 4 小时（含实操）

---

## 第一阶段：快速入门与核心概念（45 分钟）

### 1.1 为什么需要持久化存储？

- **容器特性**: 容器文件系统是临时的（Ephemeral），Pod 重启后数据会丢失
- **核心资源**:
  - **PV (PersistentVolume)**: 实际的存储资源（如一块云盘、一个 NFS 目录）
  - **PVC (PersistentVolumeClaim)**: 用户对存储的需求申请（"我要 10G，RWO 权限"）
  - **StorageClass**: 存储的"模板"，实现自动按需创建 PV (Dynamic Provisioning)

### 1.2 存储抽象层次

```
应用层 (Containers)
    ↓ Volume (Pod 级别)
    ↓ PVC (命名空间级声明)
    ↓ PV (集群级资源)
    ↓ StorageClass (存储类型模板)
    ↓ CSI Driver (存储插件接口)
    ↓ 底层存储 (云盘/NAS/Ceph/Local)
```

### 1.3 简单示例 (使用 StorageClass)

```yaml
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: my-pvc
spec:
  accessModes:
    - ReadWriteOnce
  resources:
    requests:
      storage: 10Gi
  storageClassName: alibabacloud-disk-ssd
```

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: app-with-pvc
spec:
  containers:
  - name: app
    image: nginx
    volumeMounts:
    - name: data
      mountPath: /usr/share/nginx/html
  volumes:
  - name: data
    persistentVolumeClaim:
      claimName: my-pvc
```

### 1.4 四种访问模式速查

| 模式 | 缩写 | 说明 | 典型场景 |
|------|------|------|---------|
| ReadWriteOnce | RWO | 单节点读写 | 数据库 |
| ReadOnlyMany | ROX | 多节点只读 | 配置文件 |
| ReadWriteMany | RWX | 多节点读写 | 共享日志 |
| ReadWriteOncePod | RWOP | 单 Pod 独占 | 严格单写 |

### 1.5 实操练习：创建第一个 PVC + Pod

```bash
# 1. 查看可用 StorageClass
kubectl get sc

# 2. 创建 PVC
cat <<EOF | kubectl apply -f -
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: lab-pvc
spec:
  accessModes: [ReadWriteOnce]
  storageClassName: standard
  resources:
    requests:
      storage: 5Gi
EOF

# 3. 观察绑定
kubectl get pvc lab-pvc -w

# 4. 创建 Pod 使用 PVC
cat <<EOF | kubectl apply -f -
apiVersion: v1
kind: Pod
metadata:
  name: lab-pod
spec:
  containers:
  - name: app
    image: busybox
    command: ["sleep", "3600"]
    volumeMounts:
    - name: data
      mountPath: /data
  volumes:
  - name: data
    persistentVolumeClaim:
      claimName: lab-pvc
EOF

# 5. 验证
kubectl get pod lab-pod
kubectl exec lab-pod -- df -h /data
```

---

## 第二阶段：核心架构与深度原理（60 分钟）

### 2.1 CSI 挂载全流程

```
1. CreateVolume     → CSI Controller 创建云盘
2. ControllerPublish → Attach 云盘到 ECS 节点
3. NodeStageVolume   → 格式化 + 挂载到 Staging 路径
4. NodePublishVolume → Bind-mount 到 Pod 目标路径
5. [Pod 运行中]
6. NodeUnpublishVolume → 卸载 Pod 目录
7. NodeUnstageVolume   → 清理 Staging
8. ControllerUnpublish → Detach 云盘
9. DeleteVolume        → 删除云盘
```

### 2.2 存储拓扑感知

- **延迟绑定 (`WaitForFirstConsumer`)**: 确保云盘创建在 Pod 被调度的那个可用区 (AZ)，解决跨区挂载失败问题
- **拓扑键**: `topology.kubernetes.io/zone` / `topology.kubernetes.io/hostname`

### 2.3 VolumeBindingMode 对比

| 模式 | 绑定时机 | 适用场景 |
|------|---------|---------|
| Immediate | PVC 创建即绑定 | 单可用区集群 |
| WaitForFirstConsumer | Pod 调度后再绑定 | 多可用区集群（推荐） |

### 2.4 回收策略

| 策略 | 行为 | 适用场景 |
|------|------|---------|
| Retain | 保留数据 | 生产环境（必须） |
| Delete | 自动删除 | 开发/测试 |
| Recycle | 清空重用（已废弃） | 不推荐 |

### 2.5 实操练习：验证挂载流程

```bash
# 查看 CSI 卷挂载两阶段路径
POD_UID=$(kubectl get pod lab-pod -o jsonpath='{.metadata.uid}')
PV_NAME=$(kubectl get pvc lab-pvc -o jsonpath='{.spec.volumeName}')

# Node Stage 路径
ls /var/lib/kubelet/pods/$POD_UID/volumes/kubernetes.io~csi/$PV_NAME/

# 查看 VolumeAttachment
kubectl get volumeattachment | grep $PV_NAME

# 查看 CSI Node 插件日志
kubectl logs -n kube-system -l app=csi-plugin --tail=50
```

---

## 第三阶段：生产部署与极致优化（60 分钟）

### 3.1 StorageClass 性能分级

```yaml
# 生产级（PL3）
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: fast-ssd-pl3
provisioner: diskplugin.csi.alibabacloud.com
parameters:
  type: cloud_essd
  performanceLevel: PL3
reclaimPolicy: Retain
volumeBindingMode: WaitForFirstConsumer
allowVolumeExpansion: true
mountOptions: [noatime, discard]

---
# 标准级（PL1）
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: standard-ssd-pl1
provisioner: diskplugin.csi.alibabacloud.com
parameters:
  type: cloud_essd
  performanceLevel: PL1
reclaimPolicy: Retain
volumeBindingMode: WaitForFirstConsumer
allowVolumeExpansion: true
```

### 3.2 卷扩容实战

```bash
# 1. 确认 StorageClass 允许扩容
kubectl get sc -o yaml | grep allowVolumeExpansion

# 2. 在线扩容 PVC
kubectl patch pvc lab-pvc -p '{"spec":{"resources":{"requests":{"storage":"20Gi"}}}}'

# 3. 观察扩容过程
kubectl describe pvc lab-pvc | grep -A5 Conditions

# 4. 验证扩容结果
kubectl exec lab-pod -- df -h /data
```

### 3.3 VolumeSnapshot 快照

```yaml
apiVersion: snapshot.storage.k8s.io/v1
kind: VolumeSnapshotClass
metadata:
  name: csi-snapclass
driver: diskplugin.csi.alibabacloud.com
deletionPolicy: Delete

---
apiVersion: snapshot.storage.k8s.io/v1
kind: VolumeSnapshot
metadata:
  name: lab-snapshot
spec:
  volumeSnapshotClassName: csi-snapclass
  source:
    persistentVolumeClaimName: lab-pvc
```

```bash
# 查看快照状态
kubectl get volumesnapshot lab-snapshot -o wide

# 从快照恢复
cat <<EOF | kubectl apply -f -
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: lab-pvc-restored
spec:
  accessModes: [ReadWriteOnce]
  storageClassName: standard-ssd-pl1
  dataSource:
    name: lab-snapshot
    kind: VolumeSnapshot
    apiGroup: snapshot.storage.k8s.io
  resources:
    requests:
      storage: 20Gi
EOF
```

### 3.4 监控告警配置

```yaml
# Prometheus 存储告警规则
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: storage-alerts
spec:
  groups:
  - name: storage
    rules:
    - alert: PVCUsageHigh
      expr: kubelet_volume_stats_used_bytes / kubelet_volume_stats_capacity_bytes > 0.85
      for: 5m
      labels:
        severity: warning
      annotations:
        summary: "PVC {{ $labels.persistentvolumeclaim }} 使用率超过 85%"

    - alert: PVCUsageCritical
      expr: kubelet_volume_stats_used_bytes / kubelet_volume_stats_capacity_bytes > 0.95
      for: 1m
      labels:
        severity: critical
      annotations:
        summary: "PVC {{ $labels.persistentvolumeclaim }} 使用率超过 95%，即将写满"

    - alert: PVCPendingLong
      expr: kube_persistentvolumeclaim_status_phase{phase="Pending"} == 1
      for: 10m
      labels:
        severity: warning
      annotations:
        summary: "PVC {{ $labels.persistentvolumeclaim }} Pending 超过 10 分钟"
```

---

## 第四阶段：故障诊断与数据容灾（45 分钟）

### 4.1 典型故障排查

| 故障 | 症状 | 排查路径 |
|------|------|---------|
| **PVC Pending** | Pod 一直 ContainerCreating | `kubectl describe pvc` → 检查 StorageClass 和 Events |
| **Multi-Attach** | Pod 启动失败 | `kubectl describe pod` → 确认 RWO 卷所在节点 |
| **Mount Failed** | Pod CrashLoopBackOff | 检查 CSI Node 插件日志和节点磁盘状态 |
| **ProvisioningFailed** | PVC 绑定失败 | 检查云商权限、配额和 CSI Controller |

### 4.2 诊断脚本速用

```bash
# PVC 全链路诊断
kubectl describe pvc <pvc-name> -n <ns>
kubectl describe pv <pv-name>
kubectl get volumeattachment | grep <pv-name>
kubectl logs -n kube-system -l app=csi-plugin --tail=100
kubectl get events -n <ns> --sort-by=.lastTimestamp | grep storage
```

### 4.3 备份与恢复 (DR)

- **Velero**: 业界标准的 K8s 备份工具，支持集群级别备份/恢复
- **VolumeSnapshot**: 利用云平台快照实现数据秒级备份
- **应用级备份**: mysqldump / pg_dump 等数据库原生工具

```bash
# Velero 快速备份示例
velero backup create daily-backup \
  --include-namespaces production \
  --snapshot-volumes \
  --ttl 168h

# 查看备份状态
velero backup describe daily-backup
```

### 4.4 实操练习：模拟故障排查

```bash
# 场景：PVC 一直 Pending
# 1. 获取 PVC 状态
kubectl get pvc -A | grep Pending

# 2. 查看详细事件
kubectl describe pvc <pending-pvc> -n <ns>

# 3. 常见原因检查清单:
#   □ StorageClass 是否存在
#   □ CSI Driver 是否运行
#   □ 云商配额是否充足
#   □ 拓扑约束是否匹配
#   □ ResourceQuota 是否限制
```

---

## 第五阶段：安全加固与总结（30 分钟）

### 5.1 存储安全最佳实践

| 实践 | 说明 |
|------|------|
| **加密存储** | StorageClass 启用加密 + KMS 密钥管理 |
| **RBAC 控制** | 最小权限原则，限制 PVC 创建权限 |
| **网络隔离** | iSCSI/NFS 存储网络与业务网络分离 |
| **备份验证** | 定期恢复演练，验证备份有效性 |
| **审计日志** | 记录所有存储资源操作 |

### 5.2 SRE 运维红线

- **红线 1**: 生产环境核心数据必须使用 `Retain` 回收策略
- **红线 2**: 严禁直接在 Pod 中使用 HostPath 存储敏感数据
- **红线 3**: 必须配置延迟绑定策略以支持多可用区集群
- **红线 4**: 关键业务 PVC 必须配置容量告警（85% 警告，95% 严重）
- **红线 5**: 存储变更必须经过备份验证和灰度发布

### 5.3 每日/每周运维检查清单

```markdown
每日检查:
- [ ] PVC Pending 数量 < 5
- [ ] PV Released 数量 = 0
- [ ] CSI Driver Pod 运行正常
- [ ] 存储使用率 < 85%

每周检查:
- [ ] 备份执行状态和成功率
- [ ] 存储性能趋势分析
- [ ] StorageClass 配置一致性审计
```

### 5.4 进阶学习路径

| 阶段 | 学习内容 | 参考文档 |
|------|---------|---------|
| 入门 | PV/PVC/StorageClass 基础 | domain-6/06-storage-fundamental-concepts |
| 进阶 | CSI 架构与驱动集成 | domain-6/05-csi-drivers-integration |
| 高级 | 存储性能调优 | domain-6/08-storage-performance-tuning |
| 专家 | 故障诊断与灾备 | domain-6/09-pv-pvc-troubleshooting |
| 生产 | 备份容灾体系 | domain-6/10-storage-backup-disaster-recovery |

---

**表格底部标记**: Kusheet Project | 作者: Allen Galler (allengaller@gmail.com)
