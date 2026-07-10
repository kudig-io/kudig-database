---
title: 14 - 节点存储
description: 'title: 节点存储'
summary: 'CSI Node 插件以 DaemonSet 方式运行在每个节点上，负责卷的挂载和卸载：'
category: general
tags:
- reference
- storage
- kubelet
- opa
- redis
- daemonset
- operator
- rag
tier: supporting
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 15min
intent_queries:
- 节点存储 是什么
- 如何 节点存储
- Kubernetes 07 platform engineering 最佳实践
trigger_keywords:
- 节点存储
- platform
- engineering
- code
- analysis
prerequisites:
- kubectl-basics
- platform-engineering-basics
- redis-basics
- policy-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




title: 节点存储
description: '# 14 - 节点存储'
category: functions
tags:
- k8s
- operations
- cluster-management
- kubelet
- opa
- redis
- daemonset
- operator
- rag
last_updated: 2026-05-18
difficulty: intermediate
reading_level: intermediate
audience:
- Kubernetes 运维工程师
- 存储工程师
estimated_read_time: 5min
intent_queries:
- Kubernetes node storage management
- emptyDir hostPath local PV
- CSI node plugin NodeStageVolume NodePublishVolume
- volume mount propagation
- storage topology node selector
trigger_keywords:
- storage
- emptyDir
- hostPath
- local PV
- CSI
- NodeStageVolume
- NodePublishVolume
- volume
- mount
- propagation
- storageClass
- volumeBindingMode
- WaitForFirstConsumer
- CSIDriver
- CSINode
related_domains:
- 存储
related_topics:
- 存储/06-storage-fundamental-concepts
- 存储/09-pv-pvc-troubleshooting
- 集群基础/22-container-storage-deep-dive
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

# 14 - 节点存储

> **适用版本**: Kubernetes v1.25 - v1.32 | **运维重点**: 节点存储管理、CSI Node 插件、卷挂载机制
> **源码路径**: `pkg/volume/` `pkg/kubelet/volumemanager/`

---

## 节点存储类型

```
节点存储层次:
  ┌─────────────────────────────────────────────────────────────┐
  │  临时存储 (emptyDir)                                          │
  │  - 存储在节点磁盘                                            │
  │  - Pod 删除后清除                                            │
  │  - 可选 memory (tmpfs)                                       │
  └─────────────────────────────────────────────────────────────┘
  ┌─────────────────────────────────────────────────────────────┐
  │  主机存储 (hostPath)                                         │
  │  - 挂载节点文件系统                                           │
  │  - 用于日志收集、监控等                                        │
  └─────────────────────────────────────────────────────────────┘
  ┌─────────────────────────────────────────────────────────────┐
  │ 持久存储 (PV/PVC)                                            │
  │  - 云盘/NFS/本地存储                                         │
  │  - 通过 CSI 挂载                                             │
  └─────────────────────────────────────────────────────────────┘
```

### emptyDir 详解

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: emptydir-demo
spec:
  containers:
  - name: writer
    image: busybox
    command: ["/bin/sh", "-c", "while true; do echo $(date) >> /data/log.txt; sleep 5; done"]
    volumeMounts:
    - name: shared-data
      mountPath: /data
  - name: reader
    image: busybox
    command: ["/bin/sh", "-c", "tail -f /data/log.txt"]
    volumeMounts:
    - name: shared-data
      mountPath: /data
      readOnly: true
  volumes:
  - name: shared-data
    emptyDir:
      medium: ""        # 磁盘（默认）
      sizeLimit: 500Mi  # 大小限制
```

```yaml
# 内存临时卷（tmpfs）
apiVersion: v1
kind: Pod
metadata:
  name: emptydir-memory
spec:
  containers:
  - name: cache
    image: redis:alpine
    volumeMounts:
    - name: cache-data
      mountPath: /data
  volumes:
  - name: cache-data
    emptyDir:
      medium: Memory
      sizeLimit: 2Gi
```

### hostPath 详解

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: hostpath-demo
spec:
  containers:
  - name: log-collector
    image: busybox
    command: ["tail", "-f", "/host-logs/syslog"]
    volumeMounts:
    - name: host-logs
      mountPath: /host-logs
      readOnly: true
  volumes:
  - name: host-logs
    hostPath:
      path: /var/log
      type: DirectoryOrCreate
```

| hostPath type | 说明 |
|---------------|------|
| `DirectoryOrCreate` | 目录不存在时自动创建 |
| `Directory` | 必须已存在的目录 |
| `FileOrCreate` | 文件不存在时自动创建 |
| `File` | 必须已存在的文件 |
| `Socket` | Unix socket |
| `CharDevice` / `BlockDevice` | 字符/块设备 |

---

## Local PV

```yaml
apiVersion: v1
kind: PersistentVolume
metadata:
  name: local-pv-ssd
spec:
  capacity:
    storage: 100Gi
  accessModes:
    - ReadWriteOnce
  persistentVolumeReclaimPolicy: Retain
  storageClassName: local-ssd
  local:
    path: /mnt/ssd-disk
    fsType: xfs
  nodeAffinity:
    required:
      nodeSelectorTerms:
      - matchExpressions:
        - key: kubernetes.io/hostname
          operator: In
          values:
          - worker-node-1
```

### Local PV 生产配置要点

```yaml
# StorageClass 配置（必须使用 WaitForFirstConsumer）
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: local-ssd
provisioner: kubernetes.io/no-provisioner
reclaimPolicy: Retain
volumeBindingMode: WaitForFirstConsumer
mountOptions:
  - noatime
  - nodiratime
```

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
# 自动发现 Local PV 的 sig-storage-local-static-provisioner
# 1. 准备挂载点目录
mkdir -p /mnt/disks/ssd0
mkfs.xfs /dev/nvme0n1
mount -o noatime /dev/nvme0n1 /mnt/disks/ssd0

# 2. 查看节点上的 Local PV
kubectl get pv -o wide | grep local

# 3. 检查节点存储拓扑标签
kubectl get nodes -o jsonpath='{.items[*].metadata.labels}' | jq 'keys' | grep topology
```
---

## CSI Node 插件

### CSI Node 插件职责

CSI Node 插件以 DaemonSet 方式运行在每个节点上，负责卷的挂载和卸载：

| RPC 方法 | 阶段 | 功能说明 |
|----------|------|---------|
| `NodeGetInfo` | 注册 | 报告节点存储能力（最大卷数、拓扑信息） |
| `NodeStageVolume` | Stage | 将卷挂载到 staging 路径（全局目录） |
| `NodePublishVolume` | Publish | 将 staging 路径 bind-mount 到 Pod 目录 |
| `NodeUnpublishVolume` | 卸载 | 卸载 Pod 目录的 bind-mount |
| `NodeUnstageVolume` | 清理 | 卸载 staging 路径 |
| `NodeGetVolumeStats` | 监控 | 返回卷的使用率和 inode 统计 |

### 卷挂载两阶段流程

```
创建 Pod 使用 PVC:
                                              Node Stage Path
                                             (/var/lib/kubelet/pods/<pod-id>/volumes/kubernetes.io~csi/<pv>/mount)
                                                    ↑
    CSI Controller                          CSI Node Plugin
    ─────────────                           ────────────────
    1. CreateVolume ──→ 云存储创建
    2. ControllerPublishVolume ──→ 附加到节点
                                          3. NodeStageVolume (格式化 + 挂载到 staging)
                                                    ↓
                                              Bind Mount
                                                    ↓
                                          4. NodePublishVolume (挂载到 Pod 目标路径)
                                                    ↓
                                              Pod 容器 /data
```

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看 CSI Node 插件 Pod
kubectl get pods -n kube-system -l app=csi-plugin -o wide

# 查看 CSI Node 注册信息
kubectl get csinodes -o yaml

# 查看 Node Stage/Publish 路径
ls /var/lib/kubelet/pods/
ls /var/lib/kubelet/pods/<pod-uid>/volumes/kubernetes.io~csi/

# 查看 CSI 驱动注册的拓扑键
kubectl get csidriver -o jsonpath='{.items[*].spec.attachRequired}'
kubectl get csidriver -o jsonpath='{.items[*].spec.podInfoOnMount}'

# 查看 CSI Node 插件日志
kubectl logs -n kube-system -l app=csi-plugin -c csi-plugin --tail=100
```
### CSI Driver / CSINode API

```yaml
# CSIDriver 定义驱动行为
apiVersion: storage.k8s.io/v1
kind: CSIDriver
metadata:
  name: diskplugin.csi.alibabacloud.com
spec:
  attachRequired: true              # 需要 Controller Attach
  podInfoOnMount: true              # 传递 Pod 信息给驱动
  fsGroupPolicy: File               # fsGroup 处理策略
  volumeLifecycleModes:             # 支持的生命周期模式
    - Persistent
    - Ephemeral
  storageCapacity: true             # 支持存储容量跟踪
  tokenRequests:                    # ServiceAccount Token 投射
    - audience: "csi"
      expirationSeconds: 3600
```

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看集群中已注册的 CSI 驱动
kubectl get csidriver

# 查看各节点的 CSI 驱动信息
kubectl get csinodes -o wide
```
---

## 卷挂载传播

Mount propagation 控制 Pod 中挂载的卷是否对其他 Pod 或宿主机可见：

| propagation 值 | 行为 | 适用场景 |
|---------------|------|---------|
| `None`（默认） | 不传播 | 通用场景 |
| `HostToContainer` | 宿主机 → 容器单向传播 | 监控、日志采集 |
| `Bidirectional` | 双向传播 | CSI 驱动、存储插件 |

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: mount-propagation-demo
spec:
  containers:
  - name: app
    image: busybox
    command: ["sleep", "3600"]
    volumeMounts:
    - name: data
      mountPath: /data
      mountPropagation: HostToContainer
  volumes:
  - name: data
    hostPath:
      path: /mnt/data
```

> **注意**: `Bidirectional` 仅在特权容器中使用，通常用于 CSI Node Driver Registrar。

---

## 存储拓扑

```bash
# 延迟卷绑定确保调度到正确节点
volumeBindingMode: WaitForFirstConsumer

# 拓扑键:
topology.kubernetes.io/hostname    # 节点级
topology.kubernetes.io/zone        # 可用区级
topology.kubernetes.io/region       # 地域级
```

```yaml
# StorageClass 拓扑约束示例
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: zonal-ssd
provisioner: diskplugin.csi.alibabacloud.com
volumeBindingMode: WaitForFirstConsumer
allowedTopologies:
- matchLabelExpressions:
  - key: topology.kubernetes.io/zone
    values:
    - cn-hangzhou-a
    - cn-hangzhou-b
```

### 节点卷数量限制

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看节点卷限制
kubectl get csinodes -o jsonpath='{.items[*].spec.drivers[*].allocatable.count}'

# 云厂商默认限制:
# 阿里云 ACK: 单节点最大 64 块云盘
# AWS EKS: 单节点最大 39 卷（取决于实例类型）
# GCP GKE: 单节点最大 128 Persistent Disk
# Azure AKS: 单节点最大 64 托管磁盘
```
---

## 节点存储排障

### 常见问题速查

| 问题 | 原因 | 解决 |
|------|------|------|
| PVC Pending | 无可用 PV | 检查 StorageClass 和拓扑约束 |
| 卷挂载失败 | CSI 插件异常 | 检查 CSI driver Pod 状态和日志 |
| Pod 无法调度 | 拓扑限制 | 检查节点标签和 allowedTopologies |
| Multi-Attach | RWO 卷跨节点挂载 | 确保 Pod 调度到卷所在节点 |
| Unmount 挂起 | kubelet 卷管理器异常 | 检查 kubelet 日志并重启 |
| 设备繁忙 | 进程占用挂载点 | `fuser -m /mnt/data` 查找占用进程 |
| Stale NFS | 网络中断 | `umount -f /mnt/nfs` 强制卸载 |

### 节点存储诊断脚本

```bash
#!/bin/bash
# node-storage-diagnostic.sh - 节点存储诊断工具

echo "## 节点卷挂载状态"
mount | grep -E "(kubelet|csi|nfs|rbd)" | head -20

echo ""
echo "## 磁盘使用率"
df -h | grep -E "(Filesystem|/dev/)" | sort -k5 -rn

echo ""
echo "## Inode 使用率"
df -i | grep -E "(Filesystem|/dev/)" | sort -k5 -rn | head -10

echo ""
echo "## 挂载点异常检测"
cat /proc/mounts | awk '{print $2}' | while read mp; do
  if ! timeout 3 ls "$mp" >/dev/null 2>&1; then
    echo "⚠️ 挂载点无响应: $mp"
  fi
done

echo ""
echo "## I/O 统计 (top 5)"
iostat -xz 1 3 | tail -20

echo ""
echo "## CSI 卷目录统计"
echo "  Pod volumes: $(ls /var/lib/kubelet/pods/ 2>/dev/null | wc -l)"
echo "  Plugin dirs: $(ls /var/lib/kubelet/plugins/ 2>/dev/null | wc -l)"
echo "  CSI mounters: $(ls /var/lib/kubelet/plugins/kubernetes.io/csi/ 2>/dev/null | wc -l)"
```

---

## 相关文档

- [22-storage-volumes](./22-storage-volumes.md) - 集群存储卷管理
- [../../存储/06-storage-fundamental-concepts.md](../../存储/06-storage-fundamental-concepts.md) - 存储基础概念
- [../../存储/09-pv-pvc-troubleshooting.md](../../存储/09-pv-pvc-troubleshooting.md) - PV/PVC 故障排查
- [../../集群基础/22-container-storage-deep-dive.md](../../集群基础/22-container-storage-deep-dive.md) - CSI 架构深度解析

---

## Related

- [[reference|#reference Hub]] — tag hub

- [[log|log]]
- [[系统基础/速查卡/go.md|go]]
- [[系统基础/速查卡/k8s.md|k8s]]
- 22-container-storage-deep-dive
- [[生态参考/领域索引/pvc-index.md|PVC 知识图谱索引]]


<!-- risk-assessed -->
