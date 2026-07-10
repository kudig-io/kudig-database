---
title: 存储与卷管理 (topic-code-analysis)
description: 'title: 存储与卷管理'
summary: 'title: 存储与卷管理'
category: general
tags:
- reference
- storage
- kubelet
- scheduler
- ceph
- statefulset
- operator
- rag
tier: supporting
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 存储与卷管理 是什么
- 如何 存储与卷管理
- Kubernetes 07 platform engineering 最佳实践
trigger_keywords:
- 存储与卷管理
- platform
- engineering
- code
- analysis
prerequisites:
- kubectl-basics
- platform-engineering-basics
- logging-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




title: 存储与卷管理
description: '# 存储与卷管理'
category: functions
tags:
- k8s
- operations
- cluster-management
- kubelet
- scheduler
- ceph
- operator
- rag
last_updated: '2026-05-18'
difficulty: intermediate
reading_level: intermediate
audience:
- DevOps工程师
- Kubernetes管理员
- 应用开发者
estimated_read_time: 5min
intent_queries:
- Kubernetes storage volumes PV PVC StorageClass CSI
- Kubernetes emptyDir hostPath persistentVolumeClaim
- Kubernetes CSI driver provisioner cloud storage
- Kubernetes local PV volume topology scheduling
- Kubernetes volume limits attach detach
trigger_keywords:
- storage
- volume
- PV
- PVC
- StorageClass
- CSI
- emptyDir
- hostPath
- PersistentVolume
- PersistentVolumeClaim
- dynamic provision
- volumeBindingMode
related_domains:
- domain-7-storage
- 故障诊断
related_topics:
- StatefulSet
- volume
- CSI
- storage provisioner
- cloud storage
authors:
- name: KUDIG Team
  role: contributor
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
---

# 存储与卷管理

## 源码路径

`pkg/volume/` (in-tree volume)
`pkg/controller/volume/`
`cmd/kubeadm/app/phases/addons/` (StorageClass)

---

## 存储类型总览

```
# 🟢 低风险：只读/信息收集，通常无副作用
┌─────────────────────────────────────────────────────────────┐
│                    Storage in Kubernetes                      │
├─────────────────────────────────────────────────────────────┤
│  临时存储                                                    │
│  ├── emptyDir        临时目录，Pod 删除后清除                   │
│  └── hostPath        节点本地目录                             │
├─────────────────────────────────────────────────────────────┤
│  持久存储                                                    │
│  ├── PersistentVolume (PV)     集群级存储资源                 │
│  ├── PersistentVolumeClaim (PVC) Pod 的存储请求               │
│  └── StorageClass      存储类 (动态制备)                      │
├─────────────────────────────────────────────────────────────┤
│  云厂商存储                                                  │
│  ├── AWS EBS                                                  │
│  ├── Azure Disk                                              │
│  ├── GCE PD                                                  │
│  ├── AliCloud ESSD/Cloud Disk                                │
│  └── CSI Driver           Container Storage Interface       │
└─────────────────────────────────────────────────────────────┘
```
---

## emptyDir

Pod 临时存储，Pod 删除后数据丢失:

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: test
spec:
  containers:
  - name: test
    image: nginx
    volumeMounts:
    - name: cache
      mountPath: /tmp
  volumes:
  - name: cache
    emptyDir:
      sizeLimit: 100Mi
      medium: Memory  # 写入内存而非磁盘
```

---

## hostPath

节点本地持久存储:

```yaml
volumes:
- name: data
  hostPath:
    path: /data
    type: DirectoryOrCreate  # 或 Directory/File/...
```

**用途**:
- 日志收集 (fluentd)
-监控系统 (node-exporter)

**注意**: 只调度到特定节点，需 NodeSelector/NodeAffinity。

---

## PersistentVolume (PV) 与 PVC

```yaml
# PV 定义
apiVersion: v1
kind: PersistentVolume
metadata:
  name: my-pv
spec:
  capacity:
    storage: 10Gi
  accessModes:
    - ReadWriteOnce      # 单节点读写
    - ReadOnlyMany       # 多节点只读
    - ReadWriteMany      # 多节点读写 (依赖存储类型)
  persistentVolumeReclaimPolicy: Retain  # Retain/Delete/Recycle
  storageClassName: standard
  hostPath:
    path: /data/pv

---

# PVC 申请
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: my-pvc
spec:
  accessModes:
    - ReadWriteOnce
  resources:
    requests:
      storage: 5Gi
  storageClassName: standard
```

```bash
# Pod 使用 PVC
spec:
  containers:
  - name: app
    volumeMounts:
    - name: storage
      mountPath: /data
  volumes:
  - name: storage
    persistentVolumeClaim:
      claimName: my-pvc
```

---

## StorageClass

动态制备存储卷，无需手动创建 PV:

```yaml
# AWS EBS StorageClass
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: standard
provisioner: ebs.csi.aws.com
parameters:
  type: gp3
  fsType: ext4
  reclaimPolicy: Retain
allowVolumeExpansion: true
volumeBindingMode: WaitForFirstConsumer  # 延迟绑定
```

---

## CSI (Container Storage Interface)

CSI 是 Kubernetes 与存储厂商对接的标准接口:

```
                    ┌─────────────────────────┐
                    │   CSI Driver (厂商提供)   │
                    │  - Controller Plugin     │
                    │  - Node Plugin          │
                    └───────────┬─────────────┘
                                │
                    ┌───────────▼─────────────┐
                    │      Kubernetes         │
                    │  - CSI Driver Registry  │
                    │  - External Attacher    │
                    │  - External Provisioner │
                    │  - Node Driver Registrar│
                    └─────────────────────────┘
```

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 部署 CSI Driver (以 AWS EBS 为例)
kubectl apply -f https://raw.githubusercontent.com/kubernetes-sigs/aws-ebs-csi-driver/master/deploy/kubernetes/base/
```
---

## kubeadm 与存储

kubeadm init 不配置默认 StorageClass，但会配置一些 in-tree 插件:

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看当前 StorageClass
kubectl get storageclass

# 输出 (取决于云厂商):
# NAME         PROVISIONER           RECLAIMPOLICY   VOLUMEBINDINGMODE
# standard     ebs.csi.aws.com       Retain          WaitForFirstConsumer
# fast         ebs.csi.aws.com       Delete          Immediate
```
---

## local PV (本地存储)

```yaml
# 本地 PV (静态制备)
apiVersion: v1
kind: PersistentVolume
metadata:
  name: local-pv
spec:
  capacity:
    storage: 100Gi
  accessModes:
  - ReadWriteOnce
  persistentVolumeReclaimPolicy: Retain
  storageClassName: local-storage
  local:
    path: /mnt/disk
  nodeAffinity:
    required:
      nodeSelectorTerms:
      - matchExpressions:
        - key: kubernetes.io/hostname
          operator: In
          values:
          - node-1
```

**注意**: 需要配合 Local Persistent Volume Scheduler 插件进行调度优化。

---

## 存储拓扑与调度

延迟绑定确保 Pod 调度到可用存储的节点:

```yaml
# WaitForFirstConsumer (推荐)
volumeBindingMode: WaitForFirstConsumer

# Immediate (立即绑定，可能导致 Pod 无法调度)
volumeBindingMode: Immediate
```

```
Pod 创建 → PVC → StorageClass → 等待调度决策 → 绑定到最优节点 → Pod 调度到该节点 → 挂载
```

---

## volumeLimits

节点存储容量限制:

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# kubelet 配置 (--volume-stats-aggregation-period)
# 查看节点存储能力
kubectl get node <node> -o jsonpath='{.status.capacity}'

# 输出:
# {
#   "cpu": "4",
#   "ephemeral-storage": "100Gi",
#   "memory": "8Gi",
#   "pods": "110"
# }
```
---

## 常见问题

| 问题 | 原因 | 解决 |
|------|------|------|
| PVC Pending | 无可用 PV/StorageClass | 创建 PV 或确认 StorageClass 存在 |
| Pod 无法挂载 | 存储未准备就绪 | 检查 attach/detach controller |
| 存储类型不支持 ReadWriteMany | 存储本身不支持 | 使用支持 NFS/CephFS 的存储 |
| 磁盘空间不足 | 节点磁盘满 | 清理磁盘或扩容 |
| CSI driver not found | Driver 未部署 | `kubectl get csidriver` 检查 |

## Related

- [[reference|#reference Hub]] — tag hub

- [[log|log]]
- [[系统基础/topic-cheat-sheet/go.md|go]]
- [[系统基础/topic-cheat-sheet/k8s.md|k8s]]
- [[entities/kubernetes.md|kubernetes]]
- [[entities/fluentd.md|Fluentd]]
- [[生态参考/topic-index/pvc-index.md|PVC 知识图谱索引]]


<!-- risk-assessed -->
