---
title: 'Day 26: 存储卷创建 & 删除'
description: '# Day 26: 存储卷创建 & 删除'
summary: '本文深入讲解 Kubernetes 存储体系的核心机制——PV/PVC/StorageClass 的创建、绑定和生命周期管理。存储是有状态应用（数据库、消息队列）运行的基石。在 ACK 环境中，你将学习如何使用阿里云的云盘（ESSD）、NAS、OSS 等存储产品，通过静态和动态两种方式创建存储卷，并理解不同回收策略对数据安全的影响。'
category: learning
tags:
- k8s
- training
- hands-on
- mysql
- statefulset
- rag
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 'Day 26: 存储卷创建 & 删除 是什么'
- '如何 Day 26: 存储卷创建 & 删除'
trigger_keywords:
- Day
- '26:'
- 存储卷创建
- 删除
- learn
prerequisites:
- kubectl-basics
- gpu-ml-basics
- mysql-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Day 26: 存储卷创建 & 删除

```yaml
---
title: Day 26: 存储卷创建与删除
last_updated: 2026-05-18
difficulty: intermediate
intent_queries:
  - "Kubernetes PV PVC"
  - "StorageClass"
  - "存储卷创建删除"
  - "阿里云云盘"
  - "动态供给"
trigger_keywords:
  - "PV"
  - "PVC"
  - "StorageClass"
  - "云盘"
  - "NAS"
  - "OSS"
  - "存储卷"
  - "动态供给"
  - "CSI"
reading_level: intermediate
audience:
  - sre工程师
  - ops工程师
  - 运维工程师
estimated_read_time: 45min
related_domains:
  - 存储
  - 故障诊断
related_topics:
  - 生产运维/topic-learn/inner-training/week-4-network-storage/day-27-storage-mount
  - 生产运维/topic-learn/inner-training/week-4-network-storage/checkpoint
  - 存储/01-storage-architecture-overview
id: WEEK4-DAY26
topic: training
type: hands-on
tags: [week-4, day-26, storage, pv, pvc, k8s, k8s-1.28-1.33]
---
```

> **学习时间**: 4-5 小时 | **主题**: PV/PVC 创建与生命周期管理

---

## 概述

本文深入讲解 Kubernetes 存储体系的核心机制——PV/PVC/StorageClass 的创建、绑定和生命周期管理。存储是有状态应用（数据库、消息队列）运行的基石。在 ACK 环境中，你将学习如何使用阿里云的云盘（ESSD）、NAS、OSS 等存储产品，通过静态和动态两种方式创建存储卷，并理解不同回收策略对数据安全的影响。

### 学习目标

- 理解 PV / PVC / StorageClass 三者关系和绑定机制
- 掌握阿里云云盘（Disk）和 NAS 类型的 PV 创建方式
- 能通过静态和动态方式创建存储卷
- 了解存储卷的回收策略与删除注意事项
- 掌握 PVC Pending 等常见存储问题的排查方法

---

## 核心概念详解

### PV/PVC/StorageClass 关系

K8s 的存储体系采用"供给-消费"模式：

- **PV（PersistentVolume）**: 集群管理员提供的存储资源（或由 StorageClass 动态创建）。PV 代表一块已配置好的存储（如一块云盘、一个 NAS 文件系统），它的生命周期独立于 Pod
- **PVC（PersistentVolumeClaim）**: 用户对存储的"申请"。PVC 声明所需的存储大小、访问模式和 StorageClass，K8s 自动将 PVC 绑定到满足条件的 PV
- **StorageClass**: 定义存储的"类别"和动态供给方式。当 PVC 指定了 StorageClass 时，K8s 自动调用 CSI 驱动创建对应的存储资源并生成 PV

绑定流程：

```
方式一（静态供给）: 管理员创建 PV → 用户创建 PVC → K8s 自动绑定匹配的 PV
方式二（动态供给）: 用户创建 PVC（指定 StorageClass）→ CSI 驱动自动创建底层存储 → 自动生成 PV → 自动绑定
```

### 访问模式

| 访问模式 | 缩写 | 说明 | 云盘支持 | NAS 支持 |
|---------|------|------|---------|---------|
| ReadWriteOnce | RWO | 单节点读写 | 是 | 是 |
| ReadOnlyMany | ROX | 多节点只读 | 否 | 是 |
| ReadWriteMany | RWX | 多节点读写 | 否 | 是 |
| ReadWriteOncePod | RWOP | 单 Pod 读写（K8s 1.27+） | 是 | 是 |

### 回收策略

| 策略 | 删除 PVC 后行为 | 推荐场景 |
|------|---------------|---------|
| Delete | PV + 底层存储一起删除 | 测试/临时数据 |
| Retain | PV 变为 Released，底层存储保留 | **生产环境推荐** |
| Recycle | 已废弃，不建议使用 | 不推荐 |

### ACK 支持的存储类型

| 存储类型 | CSI Driver | StorageClass | 访问模式 | 特点 | 适用场景 |
|---------|-----------|-------------|---------|------|---------|
| 云盘 ESSD | diskplugin.csi.alibabacloud.com | alicloud-disk-essd | RWO | 高 IOPS、低延迟 | 数据库、有状态应用 |
| 云盘高效 | diskplugin.csi.alibabacloud.com | alicloud-disk-efficiency | RWO | 性价比高 | 一般存储 |
| NAS | nasplugin.csi.alibabacloud.com | alicloud-nas | RWX | 共享文件访问 | 多 Pod 共享文件 |
| OSS | ossplugin.csi.alibabacloud.com | alicloud-oss | ROX | 低成本、高可靠 | 静态资源、日志归档 |

云盘的关键限制：只支持 RWO（同一时间只能被一个节点挂载）。这意味着使用云盘的 Pod 不能在多个节点上运行（适合 [[StatefulSet|StatefulSet]]），且节点问题时需要等待 Volume Detach 后才能在另一个节点上 Attach。

---

## 实战演练

### 任务 1: 查看默认 StorageClass (30min)

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看已有的 StorageClass
kubectl get sc
# 预期输出:
# NAME                       PROVISIONER                       RECLAIMPOLICY   VOLUMEBINDINGMODE   AGE
# alicloud-disk-essd         diskplugin.csi.alibabacloud.com   Delete          Immediate           30d
# alicloud-disk-efficiency   diskplugin.csi.alibabacloud.com   Delete          Immediate           30d
# alicloud-nas               nasplugin.csi.alibabacloud.com    Delete          Immediate           30d

# 查看默认 StorageClass 详情
kubectl describe sc alicloud-disk-essd
# 预期输出:
# Name:                  alicloud-disk-essd
# Provisioner:           diskplugin.csi.alibabacloud.com
# Parameters:            type=cloud_essd,regionId=cn-hangzhou
# ReclaimPolicy:         Delete
# VolumeBindingMode:     Immediate
# AllowVolumeExpansion:  true

# 查看 CSI 插件状态
kubectl get pods -n kube-system | grep csi
# 预期输出:
# csi-plugin-abcde                        4/4     Running   0          30d
# csi-plugin-fghij                        4/4     Running   0          30d
# csi-provisioner-0                       1/1     Running   0          30d

# 查看 CSI Driver
kubectl get csidrivers
# 预期输出:
# NAME                              ATTACHREQUIRED   PODINFOONMOUNT   STORAGECAPACITY   TOKENREQUESTS   FSGroup   SElinuxMount
# diskplugin.csi.alibabacloud.com    true             false            false             <unset>         <unset>   <unset>
# nasplugin.csi.alibabacloud.com     false            false            false             <unset>         <unset>   <unset>

# 查看已有的 PV
kubectl get pv
# 预期输出: 列出所有 PV（如果有）
```
### 任务 2: 动态创建云盘 PVC (40min)

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 创建 PVC（动态供给，自动创建云盘）
cat <<EOF | kubectl apply -f -
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: disk-pvc-demo
spec:
  accessModes:
  - ReadWriteOnce
  storageClassName: alicloud-disk-essd
  resources:
    requests:
      storage: 20Gi
EOF
# 预期输出: persistentvolumeclaim/disk-pvc-demo created

# 查看 PVC 状态（等待首次挂载时才创建云盘）
kubectl get pvc disk-pvc-demo
# 预期输出:
# NAME             STATUS    VOLUME   CAPACITY   ACCESS MODES   STORAGECLASS          AGE
# disk-pvc-demo    Pending                                       alicloud-disk-essd    10s
# 注意: 使用 WaitForFirstConsumer 模式时，PVC 在 Pod 使用前不会绑定

# 创建 Pod 触发云盘创建和 PVC 绑定
cat <<EOF | kubectl apply -f -
apiVersion: v1
kind: Pod
metadata:
  name: disk-pod-demo
spec:
  containers:
  - name: app
    image: registry.cn-hangzhou.aliyuncs.com/acs-sample/nginx:1.24
    volumeMounts:
    - name: disk-vol
      mountPath: /data
  volumes:
  - name: disk-vol
    persistentVolumeClaim:
      claimName: disk-pvc-demo
EOF
# 预期输出: pod/disk-pod-demo created

# 等待 Pod Running
kubectl get pod disk-pod-demo -w
# 预期输出（动态变化）:
# NAME             READY   STATUS              RESTARTS   AGE
# disk-pod-demo    0/1     ContainerCreating   0          10s
# disk-pod-demo    1/1     Running             0          30s

# 查看 PVC 和 PV 绑定状态
kubectl get pvc disk-pvc-demo
# 预期输出:
# NAME             STATUS   VOLUME               CAPACITY   ACCESS MODES   STORAGECLASS          AGE
# disk-pvc-demo    Bound    d-xxxxx              20Gi       RWO            alicloud-disk-essd    1m

kubectl get pv
# 预期输出:
# NAME       CAPACITY   ACCESS MODES   RECLAIM POLICY   STATUS   CLAIM                    STORAGECLASS          AGE
# d-xxxxx    20Gi       RWO            Delete           Bound    default/disk-pvc-demo    alicloud-disk-essd    1m

# 验证挂载
kubectl exec disk-pod-demo -- df -h /data
# 预期输出:
# Filesystem                Size      Used Available Use% Mounted on
# /dev/vdc                 20.0G     24.0K     20.0G   0% /data

kubectl exec disk-pod-demo -- sh -c 'echo "test data written at $(date)" > /data/test.txt && cat /data/test.txt'
# 预期输出: test data written at Mon Jan 15 10:30:00 UTC 2024
```
### 任务 3: 静态创建 NAS PV (40min)

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 创建 NAS 类型 PV（静态方式，需要已有 NAS 文件系统）
cat <<EOF | kubectl apply -f -
apiVersion: v1
kind: PersistentVolume
metadata:
  name: nas-pv-demo
  labels:
    type: nas
spec:
  capacity:
    storage: 50Gi
  accessModes:
  - ReadWriteMany
  persistentVolumeReclaimPolicy: Retain
  csi:
    driver: nasplugin.csi.alibabacloud.com
    volumeHandle: nas-pv-demo
    volumeAttributes:
      server: "<nas-mount-target>.cn-hangzhou.nas.aliyuncs.com"
      path: "/training-demo"
---
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: nas-pvc-demo
spec:
  accessModes:
  - ReadWriteMany
  resources:
    requests:
      storage: 50Gi
  selector:
    matchLabels:
      type: nas
EOF
# 预期输出:
# persistentvolume/nas-pv-demo created
# persistentvolumeclaim/nas-pvc-demo created

# 查看绑定状态
kubectl get pv nas-pv-demo
# 预期输出:
# NAME          CAPACITY   ACCESS MODES   RECLAIM POLICY   STATUS   CLAIM                  AGE
# nas-pv-demo   50Gi       RWX            Retain           Bound    default/nas-pvc-demo   10s

kubectl get pvc nas-pvc-demo
# 预期输出:
# NAME            STATUS   VOLUME         CAPACITY   ACCESS MODES   AGE
# nas-pvc-demo    Bound    nas-pv-demo    50Gi       RWX            30s

# 测试多 Pod 共享 NAS（创建两个 Pod 挂载同一个 PVC）
cat <<EOF | kubectl apply -f -
apiVersion: apps/v1
kind: Deployment
metadata:
  name: nas-test
spec:
  replicas: 2
  selector:
    matchLabels:
      app: nas-test
  template:
    metadata:
      labels:
        app: nas-test
    spec:
      containers:
      - name: app
        image: registry.cn-hangzhou.aliyuncs.com/acs-sample/nginx:1.24
        volumeMounts:
        - name: nas-vol
          mountPath: /data
        command: ['sh', '-c', 'echo "Hello from $(hostname)" >> /data/shared.log && sleep 3600']
      volumes:
      - name: nas-vol
        persistentVolumeClaim:
          claimName: nas-pvc-demo
EOF

# 验证共享数据
kubectl exec nas-test-abc12 -- cat /data/shared.log
# 预期输出:
# Hello from nas-test-abc12
# Hello from nas-test-def34
```
### 任务 4: 存储卷删除与回收策略 (30min)

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
# 查看当前 PV 的回收策略
kubectl get pv -o custom-columns='NAME:.metadata.name,RECLAIM:.spec.persistentVolumeReclaimPolicy,STATUS:.status.phase,CLAIM:.spec.claimRef.name'
# 预期输出:
# NAME          RECLAIM   STATUS   CLAIM
# d-xxxxx       Delete    Bound    disk-pvc-demo
# nas-pv-demo   Retain    Bound    nas-pvc-demo

# 删除 Pod（PVC 不受影响）
kubectl delete pod disk-pod-demo
kubectl delete deployment nas-test

# 验证 PVC 和 PV 仍然存在
kubectl get pvc
kubectl get pv

# 删除 PVC（触发回收策略）
kubectl delete pvc disk-pvc-demo
# Delete 策略: PV 和底层云盘一起删除

kubectl delete pvc nas-pvc-demo
# Retain 策略: PV 变为 Released 状态，底层 NAS 保留

# 查看 PV 状态变化
kubectl get pv
# 预期输出:
# nas-pv-demo   50Gi   RWX   Retain   Released   default/nas-pvc-demo

# 清理 Released 状态的 PV
kubectl delete pv nas-pv-demo

# 注意: 如果要重新使用 Retain 策略保留的存储，需要:
# 1. 手动清除 PV 的 claimRef
# 2. 或者创建新的 PV 指向同一底层存储
```
---

## 配置示例

### StatefulSet + 动态 PVC 模板

```yaml
apiVersion: apps/v1
kind: StatefulSet
metadata:
  name: database
spec:
  serviceName: database
  replicas: 3
  selector:
    matchLabels:
      app: database
  template:
    metadata:
      labels:
        app: database
    spec:
      containers:
      - name: mysql
        image: mysql:8.0
        env:
        - name: MYSQL_ROOT_PASSWORD
          valueFrom:
            secretKeyRef:
              name: mysql-secret
              key: password
        ports:
        - containerPort: 3306
        volumeMounts:
        - name: data
          mountPath: /var/lib/mysql
        resources:
          requests:
            cpu: 500m
            memory: 512Mi
          limits:
            cpu: 1000m
            memory: 1Gi
  volumeClaimTemplates:
  - metadata:
      name: data
    spec:
      accessModes: ["ReadWriteOnce"]
      storageClassName: alicloud-disk-essd
      resources:
        requests:
          storage: 50Gi
```

### 云盘扩容

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 前提: StorageClass 的 allowVolumeExpansion 必须为 true
kubectl get sc alicloud-disk-essd -o jsonpath='{.allowVolumeExpansion}'
# true

# 修改 PVC 的 storage 字段（从 20Gi 扩容到 50Gi）
kubectl patch pvc disk-pvc-demo -p '{"spec":{"resources":{"requests":{"storage":"50Gi"}}}}'

# 查看扩容状态
kubectl get pvc disk-pvc-demo
# CAPACITY 会从 20Gi 变为 50Gi

# 注意: 扩容不需要重建 Pod，CSI 驱动会自动在线扩容
# 云盘只支持扩容不支持缩容

```
---

## 常见问题

### Q1: PVC 一直处于 Pending 怎么排查？

使用 `kubectl describe pvc <name>` 查看 Events。常见原因：1）没有满足条件的 PV（大小不够、访问模式不匹配）；2）StorageClass 不存在或配置错误；3）动态供给失败（CSI 驱动异常、云服务配额不足、账户余额不足）。

### Q2: 为什么阿里云云盘只支持 ReadWriteOnce？

云盘是块存储设备，块设备在同一时间只能被一个节点挂载（Attach）。这是存储硬件的限制，不是 K8s 的限制。如果需要多节点同时读写，应该使用 NAS 文件存储（支持 RWX）。

### Q3: 云盘 Detach 超时导致 Pod 无法调度怎么办？

当节点问题时，云盘可能仍处于 Attached 状态，无法在新节点上挂载。解决方法：1）手动强制 Detach（通过阿里云控制台或 CLI）；2）使用 ACK 托管节点池的自动修复功能；3）配置 `volumeAttachment` 超时参数。

### Q4: Retain 策略下 Released 的 PV 如何复用？

Retain 策略下 PVC 删除后 PV 变为 Released 状态，底层存储保留。要复用：1）清除 PV 的 `spec.claimRef` 字段（`kubectl patch pv <name> -p '{"spec":{"claimRef":null}}'`）；2）PV 状态变为 Available；3）新 PVC 可以绑定到这个 PV。

### Q5: 如何选择云盘的性能级别？

阿里云 ESSD 提供四个性能级别：PL0（最高 10000 IOPS）、PL1（最高 50000 IOPS）、PL2（最高 100000 IOPS）、PL3（最高 1000000 IOPS）。数据库等高 IO 场景建议 PL1 或 PL2，普通应用使用 PL0 即可。性能级别越高，费用越高。

### Q6: StatefulSet 的 volumeClaimTemplates 有什么特殊之处？

volumeClaimTemplates 为 StatefulSet 的每个 Pod 自动创建独立的 PVC。PVC 命名格式为 `<pvc-name>-<statefulset-name>-<ordinal>`（如 `data-database-0`）。当 Pod 被重新调度时，会重新绑定同一个 PVC（因为 Pod 的 ordinal 不变），从而保证数据持久性。

---

## 要点总结

| 存储类型 | StorageClass | 访问模式 | 适用场景 | 特点 |
|---------|-------------|---------|---------|------|
| 云盘 ESSD | alicloud-disk-essd | RWO | 数据库、有状态应用 | 高 IOPS、低延迟 |
| 云盘高效 | alicloud-disk-efficiency | RWO | 一般存储需求 | 性价比高 |
| NAS | alicloud-nas | RWX | 共享存储、多 Pod 读写 | 多节点共享 |
| OSS | alicloud-oss | ROX | 静态资源、日志归档 | 低成本 |

---

## 延伸阅读

- [存储架构总览](../../存储/01-storage-architecture-overview.md)
- [PV 架构基础](../../存储/02-pv-architecture-fundamentals.md)
- [StorageClass 动态供给](../../存储/04-storageclass-dynamic-provisioning.md)
- [ACK 存储管理](../../云厂商/04-alicloud-ack/245-ack-ebs-storage.md)

## Related

- index/pvc-index|PVC 知识图谱索引]]

```

<!-- risk-assessed -->
