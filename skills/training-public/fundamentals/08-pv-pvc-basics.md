---
title: 第八课：存储 - PV 和 PVC [fundamentals]
description: 'title: 第八课：存储 - PV 和 PVC'
summary: 'title: 第八课：存储 - PV 和 PVC'
category: learning
tags:
- k8s
- training
- hands-on
- mysql
- hpa
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
- 第八课：存储 - PV 和 PVC 是什么
- 如何 第八课：存储 - PV 和 PVC
- Kubernetes 11 production operations 最佳实践
trigger_keywords:
- 第八课：存储
- PV
- PVC
- production
- operations
- learn
prerequisites:
- kubectl-basics
- gpu-ml-basics
- mysql-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




---
title: 第八课：存储 - PV 和 PVC
description: '# 第八课：存储 - PV 和 PVC'
category: learning
tags:
- tutorial
- k8s
- training
- lecturer
- mysql
- hpa
- rag
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 初学者
- 运维工程师
- 培训师
- 技术经理
estimated_read_time: 5min
intent_queries:
- 第八课：存储 - PV 和 PVC 是什么
- 如何 第八课：存储 - PV 和 PVC
trigger_keywords:
- 第八课：存储
- PV
- PVC
- k8s
- learning
authors:
- name: KUDIG Team
  role: contributor
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'

tier: peripheral---
# 第八课：存储 - PV 和 PVC

> **章节**: 入门引导 | **难度**: 入门 | **时长**: 20 分钟

---

## 学习目标

1. 理解 PV 和 PVC 的概念
2. 掌握存储的创建和挂载方法
3. 了解 StorageClass 的作用
4. 学会排查存储挂载问题

---

## 1. 存储的问题引入

### 1.1 问题场景

```
【场景】

你部署了一个数据库应用 MySQL，Pod 需要持久化存储数据。
问题：
• Pod 可能会漂移到不同的节点
• Pod 重启后数据不能丢失
• 需要持久化的存储空间

【解决方案】

这就引出了 PV/PVC 的概念！

PV (PersistentVolume) = 持久化卷，集群中的一块存储
PVC (PersistentVolumeClaim) = 持久化卷声明，应用申请的存储请求

就像：
• PV = 仓库里的储物柜（物理存储）
• PVC = 你申请的储物柜使用券（请求）
• Pod = 你这个租户（使用存储的应用）
```

### 1.2 存储类比

```
【图书馆类比】

PV = 图书馆的储物柜
PVC = 借储物柜的申请单
Pod = 借了储物柜的读者

• 储物柜（PV）由图书馆（集群管理员）准备
• 读者（Pod）填写申请单（PVC）来申请使用
• 管理员分配一个合适的储物柜给读者
• 读者可以自由使用，但数据会一直保存在柜子里

【K8s 类比】

PV = 持久化卷，集群级别的存储资源
PVC = 持久化卷声明，命名空间级别的请求
StorageClass = 存储类型（如 SSD、HDD、云盘）
```

---

## 2. 创建 PV 和 PVC

### 2.1 PV 的创建

```
【NFS 网络存储示例】

apiVersion: v1
kind: PersistentVolume
metadata:
  name: my-pv
spec:
  capacity:
    storage: 10Gi       # 存储容量
  accessModes:
    - ReadWriteOnce     # 单节点读写
  persistentVolumeReclaimPolicy: Retain  # 回收策略
  nfs:
    server: 192.168.1.100
    path: /data/nfs

【访问模式】

ReadWriteOnce (RWO) - 单节点读写
ReadWriteMany (RWX) - 多节点读写
ReadOnlyMany (ROX) - 多节点只读

【回收策略】

Retain = 删除 PVC 后保留数据（手动回收）
Delete = 删除 PVC 后自动删除数据
Recycle = 删除 PVC 后自动清空数据（已废弃）
```

### 2.2 PVC 的创建

```
【PVC 示例】

apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: my-pvc
spec:
  accessModes:
    - ReadWriteOnce
  resources:
    requests:
      storage: 5Gi      # 请求 5Gi 存储
  storageClassName: ""  # 空字符串表示不使用 StorageClass

【Pod 使用 PVC】

apiVersion: v1
kind: Pod
metadata:
  name: my-app
spec:
  containers:
  - name: my-container
    image: mysql:8.0
    volumeMounts:
    - name: data
      mountPath: /var/lib/mysql
  volumes:
  - name: data
    persistentVolumeClaim:
      claimName: my-pvc
```

### 2.3 StorageClass

```
【概念】

StorageClass 动态制备 PV，不需要预先创建。
用户只需要申请 PVC，系统自动生成对应的 PV。

【云存储示例 (阿里云)】

apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: alicloud-disk-ssd
provisioner: diskplugin.csi.alibabacloud.com
parameters:
  type: cloud_ssd
  encrypted: "false"

【使用 StorageClass 的 PVC】

apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: my-pvc
spec:
  accessModes:
    - ReadWriteOnce
  resources:
    requests:
      storage: 50Gi
  storageClassName: alicloud-disk-ssd

系统会自动创建一块 50Gi 的 SSD 云盘！
```

---

## 3. 查看和管理

### 3.1 查看存储资源

```
# 🟢 低风险：只读/信息收集，通常无副作用
【查看 PV】

kubectl get persistentvolume
kubectl get pv

输出示例：
NAME      CAPACITY   ACCESS MODES   RECLAIM POLICY   STATUS
my-pv     10Gi       RWO            Retain           Bound

【查看 PVC】

kubectl get persistentvolumeclaim
kubectl get pvc

输出示例：
NAME      STATUS   VOLUME   CAPACITY   ACCESS MODES
my-pvc    Bound    my-pv    10Gi       RWO

【查看 Pod 挂载的存储】

kubectl describe pod <pod-name> | grep -A10 "Volumes"
```
### 3.2 存储生命周期

```
【绑定流程】

1. 用户创建 PVC
2. K8s 根据请求查找匹配的 PV
3. 如果找到，绑定 PV 和 PVC
4. Pod 通过 PVC 使用 PV

【注意】

PVC 一旦绑定，PV 就被这个 PVC 独占。
其他 PVC 无法绑定到已绑定的 PV。
```

---

## 4. 常见问题

### 4.1 PVC 一直 Pending

```
# 🟢 低风险：只读/信息收集，通常无副作用
【排查步骤】

1. 检查 PVC 详情
   kubectl describe pvc <pvc-name>

2. 查看 Events 里的原因：
   • "no persistent volumes available" → 没有符合条件的 PV
   • StorageClass 不存在
   • 容量请求过大，没有足够的存储

3. 检查 StorageClass
   kubectl get storageclass

4. 如果是云存储，检查 CSI driver 是否正常运行
   kubectl get pods -n kube-system | grep csi
```
### 4.2 Pod 无法挂载 Volume

```
# 🟢 低风险：只读/信息收集，通常无副作用
【排查步骤】

1. 检查 PVC 是否已绑定
   kubectl get pvc

2. 检查 PV 状态
   kubectl get pv

3. 检查 Pod 的 volumes 配置
   kubectl describe pod <pod-name> | grep -A5 "Volumes"

4. 检查挂载路径是否正确
   volumeMounts:
   - name: data
     mountPath: /var/lib/mysql

5. 检查节点的存储驱动
   不同存储类型需要不同的 CSI 驱动
```
### 4.3 存储空间不足

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

```
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
【排查】

1. 查看节点磁盘使用
   kubectl describe node <node-name> | grep -A5 "Allocated"

2. 查看 PV 的实际使用
   kubectl exec -it <pod> -- df -h <mount-path>

3. 扩容 PVC（如果支持）
   kubectl patch pvc <pvc-name> -p '{"spec":{"resources":{"requests":{"storage":"100Gi"}}}}'
```
---

## 5. 总结

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源
> - `kubectl delete`：删除资源（可由声明式清单重建）

> **🔴 高风险操作警告**
>
> 下方命令属于不可逆或高影响操作，执行前请确认：
> - 已备份关键数据与配置
> - 处于批准的变更窗口期
> - 已获得相关责任人授权
> - 已准备回滚或恢复方案
> - 目标集群、Namespace、节点/资源名称正确无误

```
# 🔴 高风险：可能造成数据丢失或服务中断，执行前需备份、变更审批与回滚方案
【命令速查】

查看 PV：
kubectl get pv

查看 PVC：
kubectl get pvc

创建 PV：
kubectl apply -f pv.yaml

创建 PVC：
kubectl apply -f pvc.yaml

删除 PVC：
kubectl delete pvc <pvc-name>

【核心要点】

1. PV 是集群级别的存储资源
2. PVC 是命名空间级别的存储请求
3. StorageClass 支持动态制备 PV
4. Pod 通过 volume 挂载 PVC
5. 访问模式：RWO/RWX/ROX

【下节课预告】

下节课我们会学习 HPA 自动伸缩：
• 为什么需要自动伸缩
• 如何配置 HPA
• 常见问题排查

有问题吗？"
```
---

**关联文档**:
- [../08-scaling/08-hpa-basics.md](../08-scaling/08-hpa-basics.md) — HPA 自动伸缩
- [../../故障诊断/topic-skills/06-pvc-storage-failure.md](../../故障诊断/topic-skills/06-pvc-storage-failure.md) — 存储问题 [[SKILL|Skill]]
- [../../存储/](../../存储/) — K8s 存储文档

## See Also

- 06-configmap-secret
- 07-namespace-resource-quota
- 09-hpa-basics
- 10-health-check


<!-- risk-assessed -->
