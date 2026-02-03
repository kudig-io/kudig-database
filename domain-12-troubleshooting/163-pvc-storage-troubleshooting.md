# 163 - PVC与存储全面故障排查 (PVC & Storage Comprehensive Troubleshooting)

> **适用版本**: Kubernetes v1.25-v1.32 | **最后更新**: 2026-01

---

## 一、存储故障诊断总览 (Storage Troubleshooting Overview)

### 1.1 PVC 状态速查表

| 状态 | 含义 | 常见原因 | 严重程度 | 处理优先级 |
|-----|------|---------|---------|-----------|
| **Pending** | 等待绑定PV | 无匹配PV/SC配置错误/CSI驱动故障 | 高 | P1 |
| **Bound** | 已绑定PV | 正常状态 | - | - |
| **Lost** | 后端PV丢失 | PV被删除/存储后端不可用 | 紧急 | P0 |
| **Terminating** | 正在删除 | finalizer阻塞/被Pod使用中 | 中 | P2 |

### 1.2 PV 状态速查表

| 状态 | 含义 | 说明 | 后续操作 |
|-----|------|------|---------|
| **Available** | 可用 | 未绑定到PVC,可被声明 | - |
| **Bound** | 已绑定 | 绑定到特定PVC | - |
| **Released** | 已释放 | PVC已删除,数据保留待回收 | 手动清理或自动回收 |
| **Failed** | 失败 | 自动回收失败 | 手动处理 |

### 1.3 存储故障排查流程图

```
                    ┌─────────────────────────────────────┐
                    │         PVC 故障排查入口              │
                    └─────────────────┬───────────────────┘
                                      │
                    ┌─────────────────▼───────────────────┐
                    │     kubectl get pvc -n <ns>         │
                    │     检查 PVC 状态                    │
                    └─────────────────┬───────────────────┘
                                      │
          ┌───────────────────────────┼───────────────────────────┐
          │                           │                           │
   ┌──────▼──────┐           ┌───────▼───────┐           ┌───────▼───────┐
   │   Pending   │           │    Bound      │           │     Lost      │
   └──────┬──────┘           └───────┬───────┘           └───────┬───────┘
          │                          │                           │
   ┌──────▼──────────┐       ┌───────▼──────────┐       ┌───────▼──────────┐
   │ 检查StorageClass │      │ Pod挂载是否正常?  │       │ 检查PV是否存在    │
   │ CSI驱动状态      │       └───────┬──────────┘       │ 存储后端状态      │
   └──────┬──────────┘               │                   └───────┬──────────┘
          │                   ┌──────┴──────┐                    │
   ┌──────▼──────────┐       │            │                ┌────▼────────────┐
   │ describe pvc    │    是│            │否              │ 尝试恢复PV       │
   │ 查看Events      │      │            │                │ 数据备份恢复     │
   └─────────────────┘  ┌───▼────┐  ┌────▼─────┐          └──────────────────┘
                        │ 正常   │  │检查挂载   │
                        └────────┘  │错误信息   │
                                    └──────────┘
```

---

## 二、PVC Pending 问题排查 (PVC Pending Diagnosis)

### 2.1 常见原因与诊断

| 原因 | Events关键字 | 诊断命令 | 解决方案 |
|-----|-------------|---------|---------|
| **StorageClass不存在** | `storageclass.storage.k8s.io "xxx" not found` | `kubectl get sc` | 创建对应SC |
| **无匹配PV** | `no persistent volumes available` | `kubectl get pv` | 创建匹配的PV |
| **PV容量不足** | `no persistent volumes available` | 检查PV/PVC容量 | 创建更大容量PV |
| **访问模式不匹配** | `no persistent volumes available` | 检查accessModes | 调整访问模式 |
| **CSI驱动故障** | `waiting for a volume to be created` | `kubectl get pods -n kube-system \| grep csi` | 修复CSI驱动 |
| **配额限制** | `exceeded quota` | `kubectl get resourcequota` | 调整配额 |
| **节点亲和性** | `volume node affinity conflict` | 检查allowedTopologies | 调整拓扑约束 |
| **Provisioner故障** | `failed to provision volume` | 检查provisioner日志 | 修复provisioner |

### 2.2 详细诊断命令

```bash
# ========== 基础检查 ==========

# 1. 获取PVC状态和详情
kubectl get pvc -n <namespace> -o wide
kubectl describe pvc <pvc-name> -n <namespace>

# 2. 检查Events
kubectl get events -n <namespace> --field-selector involvedObject.name=<pvc-name>

# 3. 检查StorageClass
kubectl get sc
kubectl describe sc <storageclass-name>
kubectl get sc <sc-name> -o yaml

# 4. 检查PV状态
kubectl get pv
kubectl get pv -o custom-columns='NAME:.metadata.name,CAPACITY:.spec.capacity.storage,ACCESS:.spec.accessModes,STATUS:.status.phase,CLAIM:.spec.claimRef.name'

# ========== CSI驱动检查 ==========

# 5. 检查CSI驱动安装
kubectl get csidrivers

# 6. 检查CSI节点状态
kubectl get csinodes
kubectl describe csinode <node-name>

# 7. 检查CSI控制器Pod
kubectl get pods -n kube-system -l app=csi-provisioner
kubectl get pods -n kube-system | grep csi

# 8. 检查CSI控制器日志
kubectl logs -n kube-system -l app=csi-controller -c csi-provisioner --tail=100
kubectl logs -n kube-system <csi-controller-pod> --all-containers

# ========== 配额检查 ==========

# 9. 检查ResourceQuota
kubectl get resourcequota -n <namespace>
kubectl describe resourcequota -n <namespace>

# 10. 检查LimitRange
kubectl get limitrange -n <namespace>
```

### 2.3 StorageClass 配置问题

#### 动态Provisioner检查

```bash
# 检查StorageClass是否配置正确
kubectl get sc -o yaml

# 检查provisioner是否运行
kubectl get deployment -n kube-system | grep -E 'csi|provisioner|storage'

# 检查provisioner RBAC权限
kubectl get clusterrolebinding | grep -E 'csi|provisioner'
```

#### 常见StorageClass问题

| 问题 | 症状 | 检查方法 | 解决方案 |
|-----|------|---------|---------|
| provisioner拼写错误 | PVC一直Pending | `kubectl get sc -o yaml \| grep provisioner` | 修正provisioner名称 |
| parameters配置错误 | provision失败 | 检查云厂商文档 | 修正parameters |
| 默认SC未设置 | PVC无SC时Pending | `kubectl get sc \| grep default` | 设置默认SC |
| volumeBindingMode错误 | 调度前无法绑定 | 检查SC配置 | 调整为WaitForFirstConsumer |

#### 设置默认StorageClass

```bash
# 设置默认StorageClass
kubectl patch storageclass <sc-name> -p '{"metadata": {"annotations":{"storageclass.kubernetes.io/is-default-class":"true"}}}'

# 取消默认StorageClass
kubectl patch storageclass <old-default-sc> -p '{"metadata": {"annotations":{"storageclass.kubernetes.io/is-default-class":"false"}}}'
```

---

## 三、CSI驱动故障排查 (CSI Driver Troubleshooting)

### 3.1 CSI架构检查

```
┌─────────────────────────────────────────────────────────────────┐
│                     CSI 驱动架构                                 │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  ┌──────────────────┐        ┌──────────────────┐              │
│  │  CSI Controller  │        │    CSI Node      │              │
│  │  (Deployment)    │        │   (DaemonSet)    │              │
│  ├──────────────────┤        ├──────────────────┤              │
│  │ - provisioner    │        │ - node-driver-   │              │
│  │ - attacher       │        │   registrar      │              │
│  │ - snapshotter    │        │ - driver         │              │
│  │ - resizer        │        │ - liveness-probe │              │
│  └────────┬─────────┘        └────────┬─────────┘              │
│           │                           │                         │
│           │      CSI Socket           │                         │
│           │      (gRPC)               │                         │
│           ▼                           ▼                         │
│  ┌─────────────────────────────────────────────────┐           │
│  │              存储后端 (云盘/NFS/Ceph等)           │           │
│  └─────────────────────────────────────────────────┘           │
└─────────────────────────────────────────────────────────────────┘
```

### 3.2 CSI组件健康检查

```bash
# ========== CSI Controller 检查 ==========

# 检查CSI Controller Deployment
kubectl get deploy -n kube-system | grep csi
kubectl describe deploy -n kube-system <csi-controller-deploy>

# 检查CSI Controller Pod状态
kubectl get pods -n kube-system -l app=csi-controller -o wide
kubectl describe pod -n kube-system <csi-controller-pod>

# 检查各sidecar容器状态
kubectl get pod -n kube-system <csi-controller-pod> -o jsonpath='{range .status.containerStatuses[*]}{.name}{"\t"}{.ready}{"\n"}{end}'

# ========== CSI Node 检查 ==========

# 检查CSI Node DaemonSet
kubectl get ds -n kube-system | grep csi
kubectl describe ds -n kube-system <csi-node-ds>

# 检查各节点CSI Node Pod
kubectl get pods -n kube-system -l app=csi-node -o wide

# 检查特定节点CSI状态
kubectl describe csinode <node-name>

# ========== CSI日志检查 ==========

# Controller Provisioner日志
kubectl logs -n kube-system -l app=csi-controller -c csi-provisioner --tail=200

# Controller Attacher日志
kubectl logs -n kube-system -l app=csi-controller -c csi-attacher --tail=200

# Node Driver日志
kubectl logs -n kube-system <csi-node-pod> -c csi-driver --tail=200
```

### 3.3 常见CSI故障

| 故障 | 症状 | 原因 | 解决方案 |
|-----|------|------|---------|
| **Controller CrashLoop** | PVC无法provision | 配置错误/权限不足 | 检查日志修复配置 |
| **Node未就绪** | Pod挂载失败 | CSI Node Pod故障 | 重启CSI Node |
| **Socket通信失败** | provision/attach超时 | CSI socket文件丢失 | 重启CSI Pod |
| **Cloud API限流** | 批量创建失败 | 超出API调用限制 | 降低创建速率 |
| **权限不足** | provision失败 | IAM/RBAC配置错误 | 检查权限配置 |
| **驱动版本不兼容** | 各种错误 | K8s版本与CSI不兼容 | 升级CSI驱动 |

### 3.4 VolumeAttachment诊断

```bash
# 检查VolumeAttachment状态
kubectl get volumeattachments
kubectl get volumeattachments -o custom-columns='NAME:.metadata.name,ATTACHER:.spec.attacher,PV:.spec.source.persistentVolumeName,NODE:.spec.nodeName,ATTACHED:.status.attached'

# 详细检查特定VolumeAttachment
kubectl describe volumeattachment <va-name>

# 查找卡住的VolumeAttachment
kubectl get volumeattachments -o json | jq '.items[] | select(.status.attached==false) | .metadata.name'
```

---

## 四、挂载故障排查 (Mount Troubleshooting)

### 4.1 Pod挂载失败诊断

```bash
# ========== Pod Events检查 ==========

# 查看Pod Events
kubectl describe pod <pod-name> -n <namespace> | grep -A 20 "Events"

# 常见挂载错误Events
# - FailedMount: Unable to attach or mount volumes
# - FailedAttachVolume: Multi-Attach error for volume
# - WaitingForVolumeToCreate: waiting for a volume to be created

# ========== 节点级挂载检查 ==========

# 找到Pod所在节点
kubectl get pod <pod-name> -n <namespace> -o wide

# SSH到节点检查挂载
ssh <node-ip>

# 检查挂载点
mount | grep <pv-name>
findmnt | grep kubelet

# 检查kubelet volume目录
ls -la /var/lib/kubelet/pods/<pod-uid>/volumes/

# 检查kubelet日志
journalctl -u kubelet | grep -i "mount\|volume\|attach" | tail -100
```

### 4.2 常见挂载错误与解决

| 错误信息 | 原因 | 解决方案 |
|---------|------|---------|
| `MountVolume.SetUp failed: mount failed: exit status 32` | 目标设备不存在/已挂载 | 检查设备状态,清理stale mount |
| `Unable to attach or mount volumes: timeout` | CSI attacher超时 | 检查CSI驱动,增加超时时间 |
| `Multi-Attach error for volume` | RWO卷被多节点挂载 | 等待旧Pod终止或强制detach |
| `volume is already exclusively attached to one node` | 独占卷冲突 | 确保调度到同一节点或使用RWX |
| `wrong fs type, bad option, bad superblock` | 文件系统错误 | 检查fsType配置,修复文件系统 |
| `read-only file system` | 只读挂载 | 检查挂载选项和PV accessModes |

### 4.3 Multi-Attach问题处理

```bash
# 场景: RWO卷被多个节点挂载导致挂载失败

# 1. 查找哪些Pod在使用这个PVC
kubectl get pods -A -o json | jq -r '.items[] | select(.spec.volumes[].persistentVolumeClaim.claimName=="<pvc-name>") | "\(.metadata.namespace)/\(.metadata.name)"'

# 2. 检查VolumeAttachment
kubectl get volumeattachments | grep <pv-name>

# 3. 查看旧的attach记录
kubectl describe volumeattachment <va-name>

# 4. 如果旧Pod已不存在但attachment未清理,强制删除
# 警告: 确保旧Pod已经不再运行!
kubectl delete volumeattachment <va-name>

# 5. 或者修改PV的nodeAffinity让新Pod调度到同一节点
```

### 4.4 文件系统问题诊断

```bash
# 在节点上执行

# 检查块设备
lsblk
fdisk -l

# 检查文件系统
blkid <device>
file -s <device>

# 文件系统修复(谨慎操作)
# 1. 先umount
umount <mount-point>

# 2. 执行fsck
fsck -y <device>

# 3. 重新挂载
mount <device> <mount-point>
```

---

## 五、存储容量与扩容问题 (Storage Capacity & Expansion)

### 5.1 PVC扩容排查

```bash
# 检查StorageClass是否支持扩容
kubectl get sc <sc-name> -o jsonpath='{.allowVolumeExpansion}'

# 扩容PVC
kubectl patch pvc <pvc-name> -n <namespace> -p '{"spec":{"resources":{"requests":{"storage":"100Gi"}}}}'

# 检查扩容状态
kubectl get pvc <pvc-name> -n <namespace> -o jsonpath='{.status.conditions}'

# 查看扩容Events
kubectl describe pvc <pvc-name> -n <namespace> | grep -A 5 "Conditions"
```

### 5.2 扩容问题诊断

| 扩容状态 | 含义 | 可能原因 | 解决方案 |
|---------|------|---------|---------|
| `FileSystemResizePending` | 等待Pod重启以扩展文件系统 | 需要重启Pod | 删除Pod触发重新挂载 |
| `Resizing` | 正在扩容中 | 正常状态 | 等待完成 |
| 扩容失败 | 云API错误 | 配额/API限制 | 检查云平台配额 |
| 容量未变化 | CSI不支持在线扩容 | 驱动限制 | 离线扩容 |

### 5.3 离线扩容流程

```bash
# 1. 停止使用该PVC的Pod
kubectl scale deployment <deploy> --replicas=0 -n <namespace>

# 2. 执行扩容
kubectl patch pvc <pvc-name> -n <namespace> -p '{"spec":{"resources":{"requests":{"storage":"100Gi"}}}}'

# 3. 等待扩容完成
kubectl get pvc <pvc-name> -n <namespace> -w

# 4. 恢复Pod
kubectl scale deployment <deploy> --replicas=<original-count> -n <namespace>
```

---

## 六、存储性能问题 (Storage Performance Issues)

### 6.1 IO性能诊断

```bash
# 在使用存储的Pod中执行

# 磁盘IO测试 - 顺序写
dd if=/dev/zero of=/mnt/data/testfile bs=1G count=1 oflag=direct

# 磁盘IO测试 - 顺序读
dd if=/mnt/data/testfile of=/dev/null bs=1G count=1 iflag=direct

# 使用fio进行详细测试
fio --name=randwrite --ioengine=libaio --direct=1 --bs=4k --iodepth=64 --size=4G --rw=randwrite --filename=/mnt/data/testfile

# IOPS测试
fio --name=iops --ioengine=libaio --direct=1 --bs=4k --iodepth=32 --size=1G --rw=randrw --filename=/mnt/data/testfile
```

### 6.2 性能问题排查

| 性能问题 | 症状 | 诊断方法 | 解决方案 |
|---------|------|---------|---------|
| **IOPS不足** | 随机IO慢 | fio测试 | 升级存储类型(SSD) |
| **吞吐量不足** | 大文件读写慢 | dd测试 | 升级存储带宽 |
| **延迟高** | IO操作卡顿 | iostat监控 | 检查网络/存储后端 |
| **存储满** | 写入失败 | df -h | 扩容或清理数据 |
| **IO队列满** | IO阻塞 | cat /sys/block/xxx/queue/nr_requests | 调整队列深度 |

### 6.3 容量监控

```bash
# 检查PVC使用量(需要kubelet启用volume stats)
kubectl get --raw /api/v1/nodes/<node>/proxy/stats/summary | jq '.pods[].volume[] | select(.pvcRef.name=="<pvc-name>")'

# 使用Prometheus查询PVC使用率
# kubelet_volume_stats_used_bytes / kubelet_volume_stats_capacity_bytes

# Pod内检查
kubectl exec -it <pod> -- df -h /mnt/data
```

---

## 七、快照与备份故障 (Snapshot & Backup Issues)

### 7.1 VolumeSnapshot诊断

```bash
# 检查VolumeSnapshotClass
kubectl get volumesnapshotclass
kubectl describe volumesnapshotclass <vsc-name>

# 检查VolumeSnapshot状态
kubectl get volumesnapshot -n <namespace>
kubectl describe volumesnapshot <snapshot-name> -n <namespace>

# 检查VolumeSnapshotContent
kubectl get volumesnapshotcontent
kubectl describe volumesnapshotcontent <vsc-name>
```

### 7.2 快照常见问题

| 问题 | 原因 | 解决方案 |
|-----|------|---------|
| 快照Pending | VolumeSnapshotClass不存在 | 创建正确的VolumeSnapshotClass |
| 快照创建失败 | CSI不支持快照 | 检查CSI驱动是否支持snapshot |
| 从快照恢复失败 | 快照内容已删除 | 检查VolumeSnapshotContent |
| 快照删除卡住 | 存在依赖的PVC | 先删除依赖的PVC |

### 7.3 从快照恢复PVC

```yaml
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: restored-pvc
  namespace: default
spec:
  accessModes:
  - ReadWriteOnce
  resources:
    requests:
      storage: 50Gi
  storageClassName: standard
  dataSource:
    name: my-snapshot
    kind: VolumeSnapshot
    apiGroup: snapshot.storage.k8s.io
```

---

## 八、云厂商存储问题 (Cloud Provider Storage Issues)

### 8.1 阿里云ACK存储

```bash
# ========== 阿里云磁盘CSI检查 ==========

# 检查云盘CSI驱动
kubectl get pods -n kube-system -l app=csi-plugin
kubectl get pods -n kube-system -l app=csi-provisioner

# 检查云盘CSI日志
kubectl logs -n kube-system -l app=csi-provisioner -c csi-provisioner --tail=100

# 使用aliyun CLI检查磁盘
aliyun ecs DescribeDisks --DiskIds '["d-xxx"]'

# 检查磁盘挂载关系
aliyun ecs DescribeDisks --InstanceId <instance-id>

# ========== 阿里云NAS存储检查 ==========

# 检查NAS CSI驱动
kubectl get pods -n kube-system -l app=csi-nas-controller

# 检查NAS挂载点
aliyun nas DescribeMountTargets --FileSystemId <fs-id>
```

### 8.2 AWS EKS存储

```bash
# ========== AWS EBS CSI检查 ==========

# 检查EBS CSI驱动
kubectl get pods -n kube-system -l app=ebs-csi-controller
kubectl get pods -n kube-system -l app=ebs-csi-node

# 检查CSI日志
kubectl logs -n kube-system -l app=ebs-csi-controller -c csi-provisioner --tail=100

# 使用AWS CLI检查EBS卷
aws ec2 describe-volumes --volume-ids vol-xxx

# 检查卷attachments
aws ec2 describe-volumes --filters "Name=attachment.instance-id,Values=<instance-id>"

# ========== AWS EFS CSI检查 ==========

# 检查EFS CSI驱动
kubectl get pods -n kube-system -l app=efs-csi-controller

# 检查EFS文件系统
aws efs describe-file-systems --file-system-id fs-xxx
aws efs describe-mount-targets --file-system-id fs-xxx
```

### 8.3 GCP GKE存储

```bash
# ========== GCE PD CSI检查 ==========

# 检查GCE PD CSI驱动
kubectl get pods -n kube-system -l k8s-app=gcp-compute-persistent-disk-csi-driver

# 检查CSI日志
kubectl logs -n kube-system -l k8s-app=gcp-compute-persistent-disk-csi-driver -c csi-provisioner --tail=100

# 使用gcloud检查磁盘
gcloud compute disks describe <disk-name> --zone <zone>
gcloud compute instances describe <instance-name> --zone <zone> --format='get(disks)'
```

---

## 九、PV回收与数据保护 (PV Reclaim & Data Protection)

### 9.1 回收策略检查

| 回收策略 | 行为 | 适用场景 |
|---------|------|---------|
| **Retain** | PVC删除后PV保留,数据不删除 | 生产环境重要数据 |
| **Delete** | PVC删除后PV和底层存储一起删除 | 临时数据 |
| **Recycle** | 已废弃,不推荐使用 | - |

```bash
# 检查PV回收策略
kubectl get pv <pv-name> -o jsonpath='{.spec.persistentVolumeReclaimPolicy}'

# 修改回收策略
kubectl patch pv <pv-name> -p '{"spec":{"persistentVolumeReclaimPolicy":"Retain"}}'
```

### 9.2 Released状态PV处理

```bash
# 查找Released状态的PV
kubectl get pv | grep Released

# 方案1: 重新绑定到新PVC(手动清除claimRef)
kubectl patch pv <pv-name> -p '{"spec":{"claimRef": null}}'

# 方案2: 删除PV(数据会保留在存储后端)
kubectl delete pv <pv-name>

# 方案3: 手动清理数据后重新创建PV
```

### 9.3 Finalizer阻塞删除

```bash
# 检查PVC/PV的finalizers
kubectl get pvc <pvc-name> -o jsonpath='{.metadata.finalizers}'
kubectl get pv <pv-name> -o jsonpath='{.metadata.finalizers}'

# 移除finalizer(谨慎操作)
kubectl patch pvc <pvc-name> -p '{"metadata":{"finalizers":null}}'
kubectl patch pv <pv-name> -p '{"metadata":{"finalizers":null}}'
```

---

## 十、存储监控与告警 (Storage Monitoring & Alerting)

### 10.1 核心监控指标

| 指标 | PromQL | 说明 | 告警阈值 |
|-----|--------|------|---------|
| PVC使用率 | `kubelet_volume_stats_used_bytes/kubelet_volume_stats_capacity_bytes` | 容量使用百分比 | > 85% |
| PVC Pending数量 | `kube_persistentvolumeclaim_status_phase{phase="Pending"}` | 等待绑定的PVC | > 0 持续5分钟 |
| PVC inode使用率 | `kubelet_volume_stats_inodes_used/kubelet_volume_stats_inodes` | inode使用百分比 | > 80% |
| 存储延迟 | `rate(kubelet_volume_stats_write_latency_seconds_sum[5m])` | 写入延迟 | P99 > 100ms |

### 10.2 Prometheus告警规则

```yaml
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: storage-alerts
  namespace: monitoring
spec:
  groups:
  - name: storage
    rules:
    # PVC Pending告警
    - alert: PVCPending
      expr: kube_persistentvolumeclaim_status_phase{phase="Pending"} == 1
      for: 5m
      labels:
        severity: warning
      annotations:
        summary: "PVC {{ $labels.namespace }}/{{ $labels.persistentvolumeclaim }} 处于Pending状态"
        description: "PVC超过5分钟未绑定,请检查StorageClass和CSI驱动"
    
    # PVC使用率告警
    - alert: PVCNearFull
      expr: |
        kubelet_volume_stats_used_bytes / kubelet_volume_stats_capacity_bytes > 0.85
      for: 5m
      labels:
        severity: warning
      annotations:
        summary: "PVC使用率超过85%"
        description: "{{ $labels.namespace }}/{{ $labels.persistentvolumeclaim }} 使用率: {{ $value | humanizePercentage }}"
    
    # PVC即将满告警
    - alert: PVCCritical
      expr: |
        kubelet_volume_stats_used_bytes / kubelet_volume_stats_capacity_bytes > 0.95
      for: 2m
      labels:
        severity: critical
      annotations:
        summary: "PVC使用率超过95%,即将满"
        description: "{{ $labels.namespace }}/{{ $labels.persistentvolumeclaim }} 使用率: {{ $value | humanizePercentage }}"
    
    # inode使用率告警
    - alert: PVCInodeNearFull
      expr: |
        kubelet_volume_stats_inodes_used / kubelet_volume_stats_inodes > 0.8
      for: 5m
      labels:
        severity: warning
      annotations:
        summary: "PVC inode使用率超过80%"
        description: "{{ $labels.namespace }}/{{ $labels.persistentvolumeclaim }} inode使用率高"
    
    # PV Lost告警
    - alert: PVLost
      expr: kube_persistentvolume_status_phase{phase="Lost"} == 1
      for: 1m
      labels:
        severity: critical
      annotations:
        summary: "PV {{ $labels.persistentvolume }} 状态为Lost"
        description: "PV丢失,请立即检查存储后端状态"
    
    # CSI驱动不可用
    - alert: CSIDriverUnavailable
      expr: |
        kube_pod_status_phase{namespace="kube-system", pod=~".*csi.*", phase!="Running"} == 1
      for: 3m
      labels:
        severity: critical
      annotations:
        summary: "CSI驱动Pod异常"
        description: "{{ $labels.pod }} 状态异常,可能影响存储功能"
```

---

## 十一、快速诊断脚本 (Quick Diagnosis Script)

### 11.1 一键PVC诊断脚本

```bash
#!/bin/bash
# PVC故障诊断脚本

PVC_NAME=${1:?"Usage: $0 <pvc-name> [namespace]"}
NAMESPACE=${2:-default}

echo "=========================================="
echo "PVC诊断: ${NAMESPACE}/${PVC_NAME}"
echo "=========================================="

# 1. PVC基础信息
echo -e "\n[1] PVC状态:"
kubectl get pvc ${PVC_NAME} -n ${NAMESPACE} -o wide

# 2. PVC详情
echo -e "\n[2] PVC详情:"
kubectl describe pvc ${PVC_NAME} -n ${NAMESPACE}

# 3. 检查绑定的PV
PV_NAME=$(kubectl get pvc ${PVC_NAME} -n ${NAMESPACE} -o jsonpath='{.spec.volumeName}')
if [ -n "$PV_NAME" ]; then
    echo -e "\n[3] 绑定的PV: ${PV_NAME}"
    kubectl get pv ${PV_NAME} -o wide
    kubectl describe pv ${PV_NAME}
fi

# 4. 检查StorageClass
SC_NAME=$(kubectl get pvc ${PVC_NAME} -n ${NAMESPACE} -o jsonpath='{.spec.storageClassName}')
if [ -n "$SC_NAME" ]; then
    echo -e "\n[4] StorageClass: ${SC_NAME}"
    kubectl get sc ${SC_NAME} -o yaml
fi

# 5. 检查CSI驱动
echo -e "\n[5] CSI驱动状态:"
kubectl get pods -n kube-system | grep -E 'csi|provisioner'

# 6. 检查Events
echo -e "\n[6] 相关Events:"
kubectl get events -n ${NAMESPACE} --field-selector involvedObject.name=${PVC_NAME} --sort-by='.lastTimestamp'

# 7. 检查使用此PVC的Pod
echo -e "\n[7] 使用此PVC的Pod:"
kubectl get pods -n ${NAMESPACE} -o json | jq -r ".items[] | select(.spec.volumes[].persistentVolumeClaim.claimName==\"${PVC_NAME}\") | .metadata.name"

echo -e "\n=========================================="
echo "诊断完成"
echo "=========================================="
```

### 11.2 存储健康检查脚本

```bash
#!/bin/bash
# 存储系统健康检查

echo "=========================================="
echo "Kubernetes 存储健康检查"
echo "=========================================="

# 1. 检查所有PVC状态
echo -e "\n[1] PVC状态概览:"
echo "Pending PVC:"
kubectl get pvc -A --field-selector status.phase=Pending 2>/dev/null || echo "  无"
echo "Lost PVC:"
kubectl get pvc -A -o json | jq -r '.items[] | select(.status.phase=="Lost") | "\(.metadata.namespace)/\(.metadata.name)"' 2>/dev/null || echo "  无"

# 2. 检查PV状态
echo -e "\n[2] PV状态概览:"
kubectl get pv -o custom-columns='NAME:.metadata.name,CAPACITY:.spec.capacity.storage,STATUS:.status.phase,CLAIM:.spec.claimRef.name' | head -20

# 3. 检查StorageClass
echo -e "\n[3] StorageClass列表:"
kubectl get sc

# 4. 检查CSI驱动
echo -e "\n[4] CSI驱动状态:"
kubectl get csidrivers
echo ""
kubectl get pods -n kube-system -l app=csi-controller -o wide 2>/dev/null
kubectl get pods -n kube-system -l app=csi-node -o wide 2>/dev/null

# 5. 检查VolumeAttachments
echo -e "\n[5] VolumeAttachment状态:"
kubectl get volumeattachments 2>/dev/null | head -10

# 6. 资源配额检查
echo -e "\n[6] 存储相关ResourceQuota:"
kubectl get resourcequota -A -o custom-columns='NAMESPACE:.metadata.namespace,NAME:.metadata.name,STORAGE-USED:.status.used.requests\.storage,STORAGE-HARD:.status.hard.requests\.storage' 2>/dev/null

echo -e "\n=========================================="
echo "健康检查完成"
echo "=========================================="
```

---

## 十二、常见问题速查 (FAQ Quick Reference)

### 12.1 问题与解决方案速查表

| 问题 | 快速诊断 | 解决方案 |
|-----|---------|---------|
| PVC一直Pending | `kubectl describe pvc` 查Events | 检查SC存在性,CSI驱动状态 |
| Pod挂载超时 | `kubectl describe pod` 查Events | 检查CSI Node,网络连通性 |
| Multi-Attach错误 | 检查VolumeAttachment | 等待旧Pod终止或强制detach |
| 扩容后容量未变 | 检查PVC conditions | 重启Pod触发文件系统扩展 |
| PV Released状态 | `kubectl get pv` | 清除claimRef或删除PV |
| CSI驱动CrashLoop | `kubectl logs` CSI Pod | 检查配置和权限 |
| 存储IO慢 | fio测试 | 升级存储类型或检查网络 |
| 存储满 | `kubectl exec df -h` | 扩容或清理数据 |

---

**存储故障排查原则**:

1. **PVC Pending**: 先查SC → 再查CSI驱动 → 最后查配额
2. **挂载失败**: 查Pod Events → 查节点CSI Pod → 查kubelet日志
3. **性能问题**: 确认存储类型 → 检查网络 → 考虑升级
4. **数据安全**: 优先使用Retain策略 → 定期备份快照

---

**表格底部标记**: Kusheet Project, 作者 Allen Galler (allengaller@gmail.com)
