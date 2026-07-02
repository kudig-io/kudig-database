---
title: 'Day 26: 存储卷创建与删除实操'
description: '# Day 26: 存储卷创建与删除实操'
summary: 'PersistentVolumeClaim (PVC) - Pod 申请存储的请求'
category: learning
tags:
- k8s
- training
- hands-on
- kubelet
- statefulset
- rag
- cilium
- flannel
- calico
- ingress
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 'Day 26: 存储卷创建与删除实操 是什么'
- '如何 Day 26: 存储卷创建与删除实操'
trigger_keywords:
- Day
- '26:'
- 存储卷创建与删除实操
- learn
prerequisites:
- kubectl-basics
- gpu-ml-basics
- ebpf-basics
- cilium-basics
- cni-basics
---



# Day 26: 存储卷创建与删除实操

> **日期**: Week 4 Day 5 | **主题**: PV/PVC 创建与生命周期管理 | **版本**: K8s 1.28-1.33

---

## 1. 存储核心概念

### 1.1 存储对象关系

```
StorageClass (存储类)
    ↓ 定义存储类型和供给方式
PersistentVolume (PV) - 集群级存储资源
    ↓ 绑定关系
PersistentVolumeClaim (PVC) - Pod 申请存储的请求
    ↓ 挂载
Pod 使用 PVC
```

### 1.2 存储类型

| 类型 | 说明 | 示例 | 访问模式 |
|------|------|------|---------|
| Block | 块存储 | 云盘、LVM | RWO |
| File | 文件存储 | NFS、CIFS | RWO/ROX/RWX |
| Object | 对象存储 | S3、OSS | 需要特殊挂载 |

### 1.3 访问模式

| 模式 | 缩写 | 说明 | 适用存储 |
|------|------|------|---------|
| ReadWriteOnce | RWO | 单节点读写 | 云盘、块存储 |
| ReadOnlyMany | ROX | 多节点只读 | NFS |
| ReadWriteMany | RWX | 多节点读写 | NFS、NAS |
| ReadWriteOncePod | RWOP | 单 Pod 独占读写 | K8s 1.27+ |

### 1.4 回收策略

| 策略 | 说明 | 使用场景 |
|------|------|---------|
| Retain | 保留 PV 和数据 | 生产数据库 |
| Delete | 自动删除 PV 和底层存储 | 临时数据、缓存 |
| Recycle | 已废弃，用动态供给替代 | 不推荐 |

---

## 2. 创建 PV/PVC

### 2.1 Static Provisioning（静态）

```yaml
# 创建 PV
apiVersion: v1
kind: PersistentVolume
metadata:
  name: pv-hostpath
  labels:
    type: local
spec:
  capacity:
    storage: 10Gi
  volumeMode: Filesystem
  accessModes:
    - ReadWriteOnce
  persistentVolumeReclaimPolicy: Retain
  storageClassName: slow
  hostPath:
    path: /data/pv-hostpath
    type: DirectoryOrCreate
---
# 创建 PVC
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: pvc-hostpath
spec:
  volumeName: pv-hostpath
  storageClassName: slow
  accessModes:
    - ReadWriteOnce
  resources:
    requests:
      storage: 5Gi
```

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

```bash
# 应用配置
kubectl apply -f pv-static.yaml

# 查看绑定状态
kubectl get pv,pvc

# 示例输出:
# NAME                           CAPACITY   ACCESS MODES   RECLAIM POLICY   STATUS   CLAIM                   STORAGECLASS
# persistentvolume/pv-hostpath   10Gi       RWO            Retain           Bound    default/pvc-hostpath    slow
#
# NAME                                STATUS   VOLUME        CAPACITY   ACCESS MODES   STORAGECLASS
# persistentvolumeclaim/pvc-hostpath   Bound    pv-hostpath   10Gi       RWO            slow

# PV 绑定过程说明:
# 1. PVC 提交后，K8s PersistentVolume Controller 查找匹配的 PV
# 2. 匹配条件: storageClassName 相同、accessModes 兼容、capacity >= request
# 3. 找到后建立双向绑定关系
# 4. 如果使用 volumeName 则直接绑定指定 PV
```

### 2.2 Dynamic Provisioning（动态）

```yaml
# 创建 StorageClass
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: standard
provisioner: [[entities/kubernetes.md|kubernetes]].io/gce-pd  # 或 aws-ebs / kubernetes.io/azure-disk
parameters:
  type: pd-standard  # SSD: pd-ssd
  fstype: ext4
  replication-type: regional-pd
reclaimPolicy: Delete
allowVolumeExpansion: true
volumeBindingMode: WaitForFirstConsumer  # 延迟绑定
---
# 动态 PVC（无需手动创建 PV）
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: dynamic-pvc
spec:
  storageClassName: standard
  accessModes:
    - ReadWriteOnce
  resources:
    requests:
      storage: 20Gi
```

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

```bash
# 应用配置
kubectl apply -f storageclass.yaml
kubectl apply -f dynamic-pvc.yaml

# 查看动态创建过程
kubectl get pvc dynamic-pvc -w

# 示例输出:
# NAME           STATUS    VOLUME   CAPACITY   ACCESS MODES   STORAGECLASS
# dynamic-pvc    Pending                                       standard
# dynamic-pvc    Pending   pvc-xxx                             standard
# dynamic-pvc    Bound     pvc-xxx   20Gi       RWO            standard

# 查看 StorageClass 列表
kubectl get storageclass

# 示例输出:
# NAME                 PROVISIONER             RECLAIMPOLICY   VOLUMEBINDINGMODE      ALLOWVOLUMEEXPANSION
# standard (default)   kubernetes.io/gce-pd    Delete          WaitForFirstConsumer   true
# slow                 kubernetes.io/no-provisioner   Retain   Immediate              false
```

### 2.3 使用 PVC

```yaml
# Pod 使用 PVC
apiVersion: v1
kind: Pod
metadata:
  name: app-with-pvc
spec:
  containers:
    - name: app
      image: nginx:1.25-alpine
      volumeMounts:
        - name: app-storage
          mountPath: /data
      command: ['sh', '-c', 'echo "Data stored" > /data/test.txt && sleep 3600']
  volumes:
    - name: app-storage
      persistentVolumeClaim:
        claimName: dynamic-pvc
```

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源
> - `kubectl delete`：删除资源（可由声明式清单重建）
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

```bash
kubectl apply -f pod-pvc.yaml

# 验证数据持久化
kubectl exec app-with-pvc -- cat /data/test.txt
# 输出: Data stored

# 删除 Pod 后重建，数据仍然存在
kubectl delete pod app-with-pvc
kubectl apply -f pod-pvc.yaml
kubectl exec app-with-pvc -- cat /data/test.txt
# 输出: Data stored (数据持久化成功)
```

---

## 3. 常见存储配置

### 3.1 NFS 存储

```yaml
# NFS PV（支持 ReadWriteMany）
apiVersion: v1
kind: PersistentVolume
metadata:
  name: pv-nfs
spec:
  capacity:
    storage: 100Gi
  accessModes:
    - ReadWriteMany
  nfs:
    server: nfs-server.example.com
    path: /exports/data
  storageClassName: nfs
  persistentVolumeReclaimPolicy: Retain
---
# NFS PVC
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: pvc-nfs
spec:
  storageClassName: nfs
  accessModes:
    - ReadWriteMany
  resources:
    requests:
      storage: 50Gi
---
# 多 Pod 共享 NFS 存储
apiVersion: apps/v1
kind: Deployment
metadata:
  name: shared-storage-app
spec:
  replicas: 3
  selector:
    matchLabels:
      app: shared
  template:
    metadata:
      labels:
        app: shared
    spec:
      containers:
      - name: app
        image: nginx:1.25-alpine
        volumeMounts:
        - name: shared-data
          mountPath: /shared
        command: ['sh', '-c', 'echo "$(hostname) writing at $(date)" >> /shared/log.txt && sleep 3600']
      volumes:
      - name: shared-data
        persistentVolumeClaim:
          claimName: pvc-nfs
```

### 3.2 云盘存储（AWS EBS）

```yaml
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: ebs-sc
provisioner: ebs.csi.aws.com
parameters:
  type: gp3
  iops: "3000"
  throughput: "125"
volumeBindingMode: WaitForFirstConsumer
allowVolumeExpansion: true
reclaimPolicy: Delete
---
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: ebs-pvc
spec:
  storageClassName: ebs-sc
  accessModes:
    - ReadWriteOnce
  resources:
    requests:
      storage: 20Gi
```

### 3.3 阿里云 ESSD 存储

```yaml
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: alicloud-disk-essd
provisioner: diskplugin.csi.alibabacloud.com
parameters:
  type: cloud_essd
  performanceLevel: PL1
  encrypted: "true"
reclaimPolicy: Retain
allowVolumeExpansion: true
volumeBindingMode: WaitForFirstConsumer
---
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: essd-pvc
spec:
  storageClassName: alicloud-disk-essd
  accessModes:
    - ReadWriteOnce
  resources:
    requests:
      storage: 100Gi
```

---

## 4. 删除 PV/PVC

### 4.1 安全删除流程

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `kubectl scale --replicas=0`：缩容到 0，立即停服
> - `kubectl delete`：删除资源（可由声明式清单重建）
> - `kubectl edit/patch`：修改运行中的资源

```bash
# 1. 确认没有 Pod 使用 PVC
kubectl get pods -A -o json | jq -r '.items[] |
  select(.spec.volumes[]?.persistentVolumeClaim?.claimName == "pvc-name") |
  "\(.metadata.namespace)/\(.metadata.name)"'

# 2. 删除使用 PVC 的 Pod（先 scale down Deployment）
kubectl scale deployment <name> -n <namespace> --replicas=0

# 3. 删除 PVC
kubectl delete pvc pvc-name -n <namespace>

# 4. 检查 PV 状态
kubectl get pv

# 示例输出 (Retain 策略):
# NAME       CAPACITY   ACCESS MODES   RECLAIM POLICY   STATUS     CLAIM
# pv-xxx     20Gi       RWO            Retain           Released   default/pvc-name

# 5. 如果 PV 卡在 Terminating，手动清理 finalizers
kubectl patch pv <pv-name> -p '{"metadata":{"finalizers":null}}' --type=merge

# 6. 如果是 Retain 策略，需要手动清理
# 删除 PV（数据仍保留在底层存储中）
kubectl delete pv <pv-name>
# 底层存储需要手动到云控制台删除
```

### 4.2 PV reclaimPolicy 处理

```bash
# 查看 PV reclaimPolicy
kubectl get pv -o custom-columns='NAME:.metadata.name,POLICY:.spec.persistentVolumeReclaimPolicy,STATUS:.status.phase,CLAIM:.spec.claimRef.name'

# 示例输出:
# NAME       POLICY    STATUS    CLAIM
# pv-xxx     Retain    Released  default/pvc-name
# pv-yyy     Delete    Bound     default/another-pvc
# pv-zzz     Retain    Bound     default/db-pvc

# Retain 策略处理流程:
# 1. PVC 删除 → PV 变为 Released 状态
# 2. PV 中数据保留，不能被新 PVC 绑定
# 3. 需要手动: 清理底层数据 → 删除 PV → 重建 PV（如果需要）

# Delete 策略处理流程:
# 1. PVC 删除 → 自动删除 PV 和底层存储
# 2. 数据不可恢复！
# 3. 适合临时数据、缓存等
```

---

## 5. 存储故障排查

### 5.1 PVC Pending

```bash
# 1. 检查 PVC 状态和事件
kubectl describe pvc <pvc-name>

# 示例输出:
# Events:
#   Type     Reason              Age   From                         Message
#   Warning  ProvisioningFailed  10s   persistentvolume-controller  storageclass.storage.k8s.io "missing-sc" not found

# 2. 常见原因和解决方案
# ┌─────────────────────────┬──────────────────────────┐
# │ 原因                    │ 解决方案                  │
# ├─────────────────────────┼──────────────────────────┤
# │ StorageClass 不存在      │ 创建或修改 StorageClass   │
# │ 存储配额不足             │ 申请更多云存储配额        │
# │ 云厂商存储卷配额用尽     │ 联系云厂商提升配额        │
# │ 没有匹配的 PV（静态）    │ 创建符合条件的 PV        │
# │ WaitForFirstConsumer     │ 先创建使用 PVC 的 Pod    │
# └─────────────────────────┴──────────────────────────┘

# 3. 检查 StorageClass
kubectl get storageclass
kubectl describe storageclass <sc-name>

# 4. 检查集群存储配额
kubectl get resourcequota -A
kubectl describe resourcequota -n <namespace>
```

### 5.2 挂载失败

```bash
# 1. 检查 Pod 事件
kubectl describe pod <pod-name> | grep -A10 "Events:"

# 示例输出:
# Events:
#   Type     Reason       Age   From               Message
#   Warning  FailedMount  30s   kubelet            MountVolume.SetUp failed for volume "pvc-xxx" : rpc error: code = DeadlineExceeded

# 2. 检查 PVC 绑定状态
kubectl get pvc <pvc-name>
kubectl get pv <pv-name>

# 3. 常见错误和解决方案
# ┌────────────────────────────────┬──────────────────────────────────┐
# │ 错误信息                        │ 原因和解决方案                    │
# ├────────────────────────────────┼──────────────────────────────────┤
# │ AttachVolume.Attach failed     │ 云盘已挂载到其他实例，需先卸载    │
# │ MountVolume.Mount failed       │ 文件系统损坏，尝试 fsck 修复      │
# │ too many volumes               │ 节点挂载卷数超限，迁移 Pod       │
# │ volume already mounted         │ 卷重复挂载，检查 Pod 配置        │
# │ node(s) didn't match PV        │ 可用区不匹配，使用延迟绑定       │
# └────────────────────────────────┴──────────────────────────────────┘

# 4. CSI 驱动问题
kubectl get pods -n kube-system | grep csi
kubectl logs -n kube-system csi-driver-xxx --tail=50
kubectl get csinode
```

### 5.3 云盘常见问题

```bash
# AWS EBS 常见问题
# 1. Volume 已 attached 到其他实例
aws ec2 describe-volumes --volume-ids vol-xxx | jq '.Volumes[0].Attachments'
# 解决: 强制卸载 aws ec2 detach-volume --volume-id vol-xxx --force

# 2. AZ 不匹配
# EBS 必须和 Pod 在同一可用区
# 解决: 使用 WaitForFirstConsumer 延迟绑定
kubectl get nodes -o custom-columns='NAME:.metadata.name,ZONE:.metadata.labels.topology\.kubernetes\.io/zone'

# 3. 权限问题
# 检查 Node IAM Role 是否包含 EBS 操作权限
aws iam get-role-policy --role-name <node-role> --policy-name <policy-name>
```

---

## 6. 存储扩容

### 6.1 PVC 在线扩容

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

```bash
# StorageClass 需支持 allowVolumeExpansion: true
kubectl get storageclass <sc-name> -o jsonpath='{.allowVolumeExpansion}'

# 扩容 PVC
kubectl patch pvc <pvc-name> -p '{"spec":{"resources":{"requests":{"storage":"50Gi"}}}}'

# 验证扩容
kubectl get pvc <pvc-name>

# 示例输出:
# NAME        STATUS   VOLUME   CAPACITY   ACCESS MODES   STORAGECLASS
# data-pvc    Bound    pv-xxx   50Gi       RWO            standard

# 在 Pod 内验证文件系统已扩容
kubectl exec <pod-name> -- df -h /data

# 示例输出:
# Filesystem      Size  Used Avail Use% Mounted on
# /dev/xvda1       50G   10G   40G  20% /data
```

### 6.2 扩容限制

```yaml
# StorageClass 限制
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: ebs-sc
allowVolumeExpansion: true  # 必须为 true 才能扩容
parameters:
  type: gp3
# 扩容限制:
# - 只能扩大不能缩小
# - 不能改变 volumeBindingMode
# - 不能从 gp2 切换到 gp3（需要新建 PVC）
# - 某些存储类型需要 Pod 重启才能生效
# - 云盘扩容有最小增量限制（如 EBS 最小 1Gi）
```

---

## 7. 实战练习

**练习 1**: 创建 NFS PV 和 PVC，验证 Pod 能持久化存储数据

```bash
# 步骤:
# 1. 创建 NFS PV
# 2. 创建绑定 PVC
# 3. 创建 Pod 写入数据
# 4. 删除 Pod，重建后验证数据仍在
```

**练习 2**: 模拟 PVC Pending，排查 StorageClass 问题

```bash
# 步骤:
# 1. 创建使用不存在 StorageClass 的 PVC
# 2. kubectl describe pvc 查看错误
# 3. 创建正确的 StorageClass
# 4. 验证 PVC 自动绑定
```

**练习 3**: 验证 PVC 在线扩容功能

```bash
# 步骤:
# 1. 创建 allowVolumeExpansion: true 的 StorageClass
# 2. 创建 PVC 和使用它的 Pod
# 3. 写入大量数据
# 4. 扩容 PVC
# 5. 在 Pod 内验证文件系统大小变化
```

**练习 4**: 配置云盘存储（AWS/GCP/阿里云），验证数据持久化

```bash
# 步骤:
# 1. 创建云厂商 StorageClass
# 2. 创建动态 PVC
# 3. 创建 StatefulSet 使用 PVC
# 4. 写入数据后删除 Pod
# 5. 验证新 Pod 挂载相同数据
```

---

```yaml
---
id: LEARN-WEEK4-DAY26
title: Day 26 - 存储卷创建与删除实操
topic: network-storage
type: hands-on-guide
tags: [pv, pvc, storageclass, nfs, cloud-storage, csi, hands-on, k8s-1.28-1.33]
last_updated: 2026-05-18
difficulty: intermediate
intent_queries:
  - "PV/PVC 怎么创建"
  - "StorageClass 怎么配置"
  - "PVC Pending 怎么排查"
  - "存储扩容怎么操作"
  - "NFS 存储怎么配置"
trigger_keywords:
  - PersistentVolume
  - PersistentVolumeClaim
  - StorageClass
  - Dynamic Provisioning
  - CSI
  - NFS
  - 云盘存储
  - EBS
  - 阿里云云盘
  - 存储扩容
  - Reclaim Policy
  - volumeBindingMode
reading_level: intermediate
audience:
  - sre
  - ops-engineer
estimated_read_time: 50min
related_domains:
  - domain-10-troubleshooting-diagnostics
  - domain-04-storage-data
related_topics:
  - storage
  - pv
  - pvc
  - storageclass
  - csi
related:
  - domain-11-production-operations/topic-learn/public-training/week-4-network-storage/day-27-pvc-mount/01-pvc-mount-hands-on.md
  - domain-10-troubleshooting-diagnostics/10-pv-pvc-troubleshooting.md
---
```


---

## 自测题 (Self-Check)

**1. ClusterIP 如何实现?**

<details><summary>答案</summary>

kube-proxy 通过 iptables/IPVS 将 ClusterIP DNAT 到后端 PodIP:TargetPort。

</details>

**2. [[Ingress|Ingress]] vs Gateway API?**

<details><summary>答案</summary>

Ingress 仅 HTTP, 需注解扩展; Gateway API 支持 HTTP/gRPC/TCP, 原生流量分割, 角色分离。

</details>

**3. StatefulSet 稳定网络标识原理?**

<details><summary>答案</summary>

Pod 名 <sts>-<ordinal> + Headless Service → DNS <pod>.<svc>.<ns>.svc.cluster.local。

</details>

**4. 如何选 CNI?**

<details><summary>答案</summary>

Calico (通用 BGP/VXLAN) / Cilium (eBPF 高性能) / Flannel (简单无 Policy)。生产推荐 Cilium 或 Calico。

</details>

**5. PVC 三种访问模式?**

<details><summary>答案</summary>

ReadWriteOnce (单节点 RW) / ReadOnlyMany (多节点 RO) / ReadWriteMany (多节点 RW)。

</details>

