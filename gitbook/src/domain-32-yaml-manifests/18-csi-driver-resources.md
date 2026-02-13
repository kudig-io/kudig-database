# 18 - CSI 驱动资源 YAML 配置参考

> **文档版本**: 2026-02  
> **适用范围**: Kubernetes v1.25 - v1.32  
> **资源类型**: CSIDriver, CSINode, CSIStorageCapacity  
> **API 版本**: storage.k8s.io/v1  
> **用途**: CSI 驱动注册、节点拓扑、存储容量管理

---

## 一、CSI 架构概述

### 1.1 CSI (Container Storage Interface) 介绍

**CSI** 是 Kubernetes 定义的容器存储接口标准,用于解耦存储供应商实现与 Kubernetes 核心代码。

**架构组件**:
```
┌────────────────────────────────────────────────────────┐
│                   Kubernetes 控制平面                   │
│  ┌──────────────────┐  ┌───────────────────────────┐  │
│  │ kube-controller  │  │  External Components      │  │
│  │  - PV Controller │  │  - External Provisioner   │  │
│  │  - Attach/Detach │  │  - External Attacher      │  │
│  │    Controller    │  │  - External Resizer       │  │
│  └──────────────────┘  │  - External Snapshotter   │  │
│                        └───────────────────────────┘  │
│                                 ↓ gRPC                 │
│                        ┌───────────────────────────┐  │
│                        │  CSI Driver (Controller)  │  │
│                        │  - CreateVolume           │  │
│                        │  - DeleteVolume           │  │
│                        │  - ControllerPublishVolume│  │
│                        └───────────────────────────┘  │
└────────────────────────────────────────────────────────┘
                                 ↓
┌────────────────────────────────────────────────────────┐
│                   Kubernetes 节点                       │
│  ┌──────────────────┐         ┌─────────────────────┐ │
│  │    Kubelet       │ ← gRPC →│ CSI Driver (Node)   │ │
│  │  - Volume Manager│         │ - NodeStageVolume   │ │
│  └──────────────────┘         │ - NodePublishVolume │ │
│                               └─────────────────────┘ │
│                                       ↓                │
│                               ┌─────────────────────┐ │
│                               │  Host Filesystem    │ │
│                               │  /var/lib/kubelet/  │ │
│                               └─────────────────────┘ │
└────────────────────────────────────────────────────────┘
```

### 1.2 CSI 三阶段挂载流程

```
========== 阶段 1: Controller Publish (控制平面) ==========
External Attacher 调用 CSI Driver 的 ControllerPublishVolume
    ↓
将存储卷连接到目标节点 (如 AWS EBS Attach Volume 到 EC2 实例)
    ↓
返回 publish_context (如设备路径 /dev/xvdf)

========== 阶段 2: Node Stage (节点) ==========
Kubelet 调用 CSI Driver 的 NodeStageVolume
    ↓
格式化块设备 (如 mkfs.ext4 /dev/xvdf)
    ↓
挂载到全局临时目录 (staging path)
例: /var/lib/kubelet/plugins/kubernetes.io/csi/pv/pvc-abc123/globalmount

========== 阶段 3: Node Publish (节点) ==========
Kubelet 调用 CSI Driver 的 NodePublishVolume
    ↓
绑定挂载到 Pod 目录
例: /var/lib/kubelet/pods/<pod-uid>/volumes/kubernetes.io~csi/<pvc-name>/mount
    ↓
Pod 容器内可见 (volumeMounts.mountPath)

========== 卸载流程 (反向) ==========
NodeUnpublishVolume → NodeUnstageVolume → ControllerUnpublishVolume
```

**关键特性**:
- **Stage/Unstage**: 全局挂载,多个 Pod 可共享(同一节点)
- **Publish/Unpublish**: Pod 独占挂载,生命周期跟随 Pod
- **可选性**: 驱动可声明是否需要 Attach/Stage 操作

---

## 二、CSIDriver (驱动注册)

### 2.1 API 资源信息

```yaml
# API 元数据
apiVersion: storage.k8s.io/v1    # CSIDriver 在 v1.18+ 稳定
kind: CSIDriver                  # 资源类型

metadata:
  name: ebs.csi.aws.com          # 驱动名称(集群级别唯一,必须与驱动实现匹配)
  labels:
    app.kubernetes.io/name: aws-ebs-csi-driver
```

**关键特性**:
- **集群级别资源**: CSIDriver 不属于任何 namespace
- **驱动能力声明**: 告知 Kubernetes 驱动支持哪些功能
- **可选创建**: 驱动可在部署时自动创建此对象

### 2.2 核心字段结构

```yaml
spec:
  attachRequired: true           # 是否需要 Attach/Detach 操作(默认 true)
  podInfoOnMount: true           # 是否需要 Pod 信息(默认 false)
  volumeLifecycleModes:          # 支持的卷生命周期模式(可选)
  - Persistent                   # 持久卷(PV/PVC)
  - Ephemeral                    # 临时卷(Generic Ephemeral Volumes)
  fsGroupPolicy: File            # FSGroup 策略(v1.19+ Beta, v1.23+ GA)
  storageCapacity: true          # 是否报告存储容量(v1.21+ Beta, v1.24+ GA)
  requiresRepublish: false       # 是否需要定期重新发布(v1.20+ Beta, v1.26+ GA)
  seLinuxMount: false            # 是否支持 SELinux 挂载(v1.25+ Alpha)
  tokenRequests:                 # 服务账户令牌请求(v1.20+ Beta, v1.22+ GA)
  - audience: "vault.example.com"
    expirationSeconds: 3600
```

---

### 2.3 attachRequired (挂载需求)

```yaml
spec:
  attachRequired: true           # 是否需要 ControllerPublishVolume/ControllerUnpublishVolume
  # true: 块存储(需要 attach 到节点),如 AWS EBS, Azure Disk
  # false: 网络文件系统(直接挂载),如 NFS, CephFS
```

**工作原理**:
```yaml
# attachRequired: true (块存储)
StorageClass (ebs.csi.aws.com)
        ↓
PVC 创建 → PV 创建 (EBS 卷 vol-0abc123)
        ↓
Pod 调度到 node-01
        ↓
Attach/Detach Controller 检测到需要 attach
        ↓
创建 VolumeAttachment 对象
        ↓
External Attacher 调用 ControllerPublishVolume(vol-0abc123, node-01)
        ↓
EBS 卷 attach 到 node-01 EC2 实例
        ↓
Kubelet 继续 Stage/Publish 流程

---

# attachRequired: false (网络文件系统)
StorageClass (nfs.csi.k8s.io)
        ↓
PVC 创建 → PV 创建 (NFS 服务器共享)
        ↓
Pod 调度到 node-01
        ↓
Kubelet 直接调用 NodeStageVolume (跳过 Attach)
        ↓
挂载 NFS 共享
```

**典型场景**:

| attachRequired | 存储类型 | 示例驱动 |
|----------------|----------|----------|
| **true** | 块存储 | ebs.csi.aws.com, disk.csi.azure.com, pd.csi.storage.gke.io |
| **false** | 网络文件系统 | nfs.csi.k8s.io, cephfs.csi.ceph.com |

### 2.4 podInfoOnMount (Pod 信息注入)

```yaml
spec:
  podInfoOnMount: true           # 是否在 NodePublishVolume 时传递 Pod 信息
```

**传递的 Pod 信息**:
```go
// NodePublishVolumeRequest 的 volume_context 字段会包含:
{
  "csi.storage.k8s.io/pod.name": "my-pod",
  "csi.storage.k8s.io/pod.namespace": "default",
  "csi.storage.k8s.io/pod.uid": "a1b2c3d4-...",
  "csi.storage.k8s.io/serviceAccount.name": "default",
  "csi.storage.k8s.io/ephemeral": "false"
}
```

**使用场景**:
1. **多租户隔离**: 驱动根据 Pod 身份设置访问权限
2. **审计日志**: 记录哪个 Pod 访问了哪个卷
3. **配额管理**: 基于 namespace 限制存储使用
4. **动态配置**: 根据 Pod 注解动态调整挂载参数

**示例: Secret Store CSI Driver**
```yaml
apiVersion: storage.k8s.io/v1
kind: CSIDriver
metadata:
  name: secrets-store.csi.k8s.io
spec:
  podInfoOnMount: true           # 需要 Pod 信息访问 Secret
  volumeLifecycleModes:
  - Ephemeral

# Pod 使用
apiVersion: v1
kind: Pod
metadata:
  name: app
spec:
  serviceAccountName: app-sa     # 驱动使用此 SA 访问 Secret 后端
  containers:
  - name: app
    image: nginx
    volumeMounts:
    - name: secrets
      mountPath: /mnt/secrets
  volumes:
  - name: secrets
    csi:
      driver: secrets-store.csi.k8s.io
      volumeAttributes:
        secretProviderClass: "aws-secrets"  # 驱动根据 Pod SA 获取权限
```

### 2.5 volumeLifecycleModes (卷生命周期模式)

```yaml
spec:
  volumeLifecycleModes:          # 支持的卷类型(可选,默认 [Persistent])
  - Persistent                   # 持久卷(PV/PVC)
  - Ephemeral                    # 临时卷(v1.16+ Beta, v1.25+ GA)
```

**模式对比**:

| 模式 | 资源对象 | 生命周期 | 使用场景 |
|------|----------|----------|----------|
| **Persistent** | PV, PVC | 独立于 Pod | 数据库、持久化存储 |
| **Ephemeral** | 无(内联 CSI) | 跟随 Pod | Secret 注入、临时缓存 |

**Ephemeral Volume 示例**:
```yaml
apiVersion: v1
kind: Pod
metadata:
  name: app-with-ephemeral
spec:
  containers:
  - name: app
    image: nginx
    volumeMounts:
    - name: scratch
      mountPath: /scratch
  volumes:
  - name: scratch
    csi:
      driver: ebs.csi.aws.com    # 驱动必须声明支持 Ephemeral
      fsType: ext4
      volumeAttributes:
        type: gp3
        size: "10Gi"             # 临时 10GB 卷
# Pod 删除时,卷自动删除
```

### 2.6 fsGroupPolicy (文件系统组策略)

```yaml
spec:
  fsGroupPolicy: File            # FSGroup 应用策略(v1.23+ GA)
  # 可选值: None | File | ReadWriteOnceWithFSType
```

**策略说明**:

| 策略 | 行为 | 适用场景 |
|------|------|----------|
| **File** | Kubelet 修改卷内所有文件的 GID 为 Pod 的 fsGroup | 通用(默认推荐) |
| **ReadWriteOnceWithFSType** | 仅对 RWO + 特定 fsType 应用 | 性能优化 |
| **None** | 不修改文件所有权 | 驱动自行处理权限 |

**工作原理**:
```yaml
# Pod 定义
apiVersion: v1
kind: Pod
metadata:
  name: secure-app
spec:
  securityContext:
    fsGroup: 3000                # 指定文件系统组 ID
  containers:
  - name: app
    image: nginx
    volumeMounts:
    - name: data
      mountPath: /data
  volumes:
  - name: data
    persistentVolumeClaim:
      claimName: my-pvc

# fsGroupPolicy: File 时的效果:
# Kubelet 在 NodePublishVolume 后执行:
# chown -R :3000 /var/lib/kubelet/pods/.../volumes/kubernetes.io~csi/my-pvc/mount
# 容器内: ls -l /data
# drwxr-sr-x 2 root 3000 4096 Feb 10 02:00 .
```

**性能考虑**:
```yaml
# ⚠️ File 策略对大量文件会很慢(递归 chown)
# 10 万个文件可能需要数分钟

# ✅ 优化: 使用 ReadWriteOnceWithFSType
spec:
  fsGroupPolicy: ReadWriteOnceWithFSType
  # 仅在特定条件下应用 fsGroup

# ✅ 或使用 None,驱动层实现高效权限管理
spec:
  fsGroupPolicy: None
```

### 2.7 storageCapacity (存储容量)

```yaml
spec:
  storageCapacity: true          # 是否创建 CSIStorageCapacity 对象(v1.24+ GA)
```

**功能**:
- 驱动报告可用存储容量(按拓扑域)
- 调度器根据容量信息调度 Pod (WaitForFirstConsumer 模式)
- 避免因容量不足导致的调度失败

**工作流程**:
```
CSI Driver 扫描节点本地存储
        ↓
创建/更新 CSIStorageCapacity 对象
例: node-01 上 local-storage 还有 500GB 可用
        ↓
调度器调度 Pod 时:
  - PVC 请求 100GB
  - 检查各节点的 CSIStorageCapacity
  - 选择有足够容量的节点
        ↓
避免调度到容量不足的节点
```

**示例**:
```yaml
apiVersion: storage.k8s.io/v1
kind: CSIDriver
metadata:
  name: local.csi.k8s.io
spec:
  storageCapacity: true          # Local PV 场景必须启用

# CSI Driver 会自动创建:
---
apiVersion: storage.k8s.io/v1
kind: CSIStorageCapacity
metadata:
  name: local.csi.k8s.io-node-01
  namespace: kube-system
storageClassName: local-storage
nodeTopology:
  matchLabels:
    kubernetes.io/hostname: node-01
capacity: 500Gi                  # node-01 上还有 500GB 可用
```

### 2.8 requiresRepublish (定期重新发布)

```yaml
spec:
  requiresRepublish: true        # 是否需要定期调用 NodePublishVolume(v1.26+ GA)
```

**使用场景**:
1. **凭证轮换**: Secret Store CSI 需要定期刷新 Secret
2. **租约续期**: 分布式锁文件系统需要续期
3. **健康检查**: 定期验证挂载状态

**默认行为**:
- Kubelet 每 `node_publish_interval` (默认 5 分钟) 调用一次 NodePublishVolume
- 驱动可利用此机会刷新挂载状态

### 2.9 tokenRequests (令牌请求)

```yaml
spec:
  tokenRequests:                 # 请求 ServiceAccount 令牌(v1.22+ GA)
  - audience: "vault.example.com"  # 令牌受众
    expirationSeconds: 3600      # 过期时间(秒)
```

**使用场景**:
- CSI 驱动需要以 Pod 身份访问外部服务(如 Vault, AWS IAM)
- 实现细粒度的身份验证

**工作原理**:
```
Pod 创建,指定 serviceAccountName: app-sa
        ↓
Kubelet 调用 TokenRequest API 为 app-sa 生成令牌
  audience: vault.example.com
  expirationSeconds: 3600
        ↓
Kubelet 将令牌挂载到 CSI Driver 容器
        ↓
CSI Driver 在 NodePublishVolume 时使用令牌访问 Vault
        ↓
Vault 验证令牌受众和签名,返回 Secret
        ↓
CSI Driver 将 Secret 挂载到 Pod 卷
```

**示例: Secrets Store CSI Driver**
```yaml
apiVersion: storage.k8s.io/v1
kind: CSIDriver
metadata:
  name: secrets-store.csi.k8s.io
spec:
  podInfoOnMount: true
  tokenRequests:
  - audience: "sts.amazonaws.com"  # AWS IRSA
    expirationSeconds: 3600

# Pod 使用
apiVersion: v1
kind: Pod
metadata:
  name: app
spec:
  serviceAccountName: app-sa     # 必须配置 IRSA 注解
  containers:
  - name: app
    image: nginx
    volumeMounts:
    - name: secrets
      mountPath: /mnt/secrets
      readOnly: true
  volumes:
  - name: secrets
    csi:
      driver: secrets-store.csi.k8s.io
      volumeAttributes:
        secretProviderClass: "aws-secrets"
```

---

### 2.10 配置模板

#### 模板 1: 块存储驱动 (AWS EBS)
```yaml
apiVersion: storage.k8s.io/v1
kind: CSIDriver
metadata:
  name: ebs.csi.aws.com
  labels:
    app.kubernetes.io/name: aws-ebs-csi-driver
spec:
  attachRequired: true           # 块存储需要 attach
  podInfoOnMount: false          # 不需要 Pod 信息
  volumeLifecycleModes:
  - Persistent
  - Ephemeral                    # v1.25+ 支持临时卷
  fsGroupPolicy: File            # 标准文件权限管理
  storageCapacity: false         # 云盘容量无限
```

#### 模板 2: 网络文件系统驱动 (NFS)
```yaml
apiVersion: storage.k8s.io/v1
kind: CSIDriver
metadata:
  name: nfs.csi.k8s.io
spec:
  attachRequired: false          # NFS 不需要 attach
  podInfoOnMount: false
  volumeLifecycleModes:
  - Persistent
  fsGroupPolicy: File
  storageCapacity: false
```

#### 模板 3: Local PV 驱动
```yaml
apiVersion: storage.k8s.io/v1
kind: CSIDriver
metadata:
  name: local.csi.k8s.io
spec:
  attachRequired: false          # 本地盘不需要 attach
  podInfoOnMount: false
  volumeLifecycleModes:
  - Persistent
  fsGroupPolicy: File
  storageCapacity: true          # 必须报告节点容量
```

#### 模板 4: Secret Store 驱动
```yaml
apiVersion: storage.k8s.io/v1
kind: CSIDriver
metadata:
  name: secrets-store.csi.k8s.io
spec:
  attachRequired: false
  podInfoOnMount: true           # 需要 Pod 身份
  volumeLifecycleModes:
  - Ephemeral                    # 仅支持临时卷
  fsGroupPolicy: None            # Secret 文件不需要 fsGroup
  requiresRepublish: true        # 定期刷新 Secret
  tokenRequests:                 # 请求 SA 令牌
  - audience: "vault.example.com"
    expirationSeconds: 3600
```

---

## 三、CSINode (节点拓扑)

### 3.1 API 资源信息

```yaml
# API 元数据
apiVersion: storage.k8s.io/v1    # CSINode 在 v1.17+ 稳定
kind: CSINode                    # 资源类型

metadata:
  name: node-01                  # 必须与 Node 对象名称一致
  labels:
    kubernetes.io/hostname: node-01
```

**关键特性**:
- **集群级别资源**: CSINode 不属于任何 namespace
- **自动创建**: Kubelet 自动创建/更新此对象
- **拓扑信息**: 报告节点上安装的 CSI 驱动及其拓扑域

### 3.2 核心字段结构

```yaml
spec:
  drivers:                       # 节点上安装的 CSI 驱动列表
  - name: ebs.csi.aws.com        # 驱动名称
    nodeID: i-0abc123def456       # 节点标识符(驱动特定)
    topologyKeys:                # 拓扑键列表
    - topology.ebs.csi.aws.com/zone
    allocatable:                 # 可分配资源(v1.19+ Beta, v1.28+ GA)
      count: 39                  # 可附加的卷数量
```

### 3.3 Drivers (驱动列表)

```yaml
spec:
  drivers:
  - name: ebs.csi.aws.com        # 驱动名称(必填)
    nodeID: i-0abc123def456       # 节点 ID(必填,驱动特定)
    topologyKeys:                # 拓扑键(可选)
    - topology.ebs.csi.aws.com/zone
    - topology.kubernetes.io/region
    allocatable:                 # 可分配资源(可选)
      count: 39                  # AWS 实例可附加的 EBS 卷数
```

**nodeID 说明**:

| 驱动 | nodeID 含义 | 示例 |
|------|-------------|------|
| ebs.csi.aws.com | EC2 实例 ID | i-0abc123def456 |
| disk.csi.azure.com | Azure VM 名称 | vm-node-01 |
| pd.csi.storage.gke.io | GCE 实例名称 | gke-cluster-node-01 |
| local.csi.k8s.io | 节点主机名 | node-01 |

**topologyKeys 说明**:
- 定义节点的拓扑维度
- 调度器根据 PV 的 nodeAffinity 与节点的 topologyKeys 匹配
- 确保 Pod 调度到可访问 PV 的节点

**allocatable 说明**:
- 报告节点可附加的最大卷数(如 AWS m5.large 最多 39 个 EBS 卷)
- 调度器避免超出限制

### 3.4 实际示例

#### 示例 1: AWS EBS CSI 节点
```yaml
apiVersion: storage.k8s.io/v1
kind: CSINode
metadata:
  name: ip-10-0-1-100.us-west-2.compute.internal
  labels:
    kubernetes.io/hostname: ip-10-0-1-100.us-west-2.compute.internal
spec:
  drivers:
  - name: ebs.csi.aws.com
    nodeID: i-0a1b2c3d4e5f6g7h8   # EC2 实例 ID
    topologyKeys:
    - topology.ebs.csi.aws.com/zone
    allocatable:
      count: 39                    # m5.2xlarge 实例限制
```

**查看命令**:
```bash
# 查看所有 CSINode
kubectl get csinode
# NAME                                            DRIVERS   AGE
# ip-10-0-1-100.us-west-2.compute.internal        1         10d

# 查看详情
kubectl get csinode ip-10-0-1-100.us-west-2.compute.internal -o yaml
```

#### 示例 2: 多驱动节点
```yaml
apiVersion: storage.k8s.io/v1
kind: CSINode
metadata:
  name: node-01
spec:
  drivers:
  - name: ebs.csi.aws.com        # 云盘驱动
    nodeID: i-0abc123
    topologyKeys:
    - topology.ebs.csi.aws.com/zone
    allocatable:
      count: 39
  - name: local.csi.k8s.io       # 本地盘驱动
    nodeID: node-01
    topologyKeys:
    - kubernetes.io/hostname
    # local 驱动无 allocatable (容量通过 CSIStorageCapacity 报告)
```

---

## 四、CSIStorageCapacity (存储容量)

### 4.1 API 资源信息

```yaml
# API 元数据
apiVersion: storage.k8s.io/v1    # CSIStorageCapacity 在 v1.24+ 稳定
kind: CSIStorageCapacity         # 资源类型

metadata:
  name: local.csi.k8s.io-node-01-local-storage  # 自动生成名称
  namespace: kube-system         # 通常在 kube-system
  labels:
    app.kubernetes.io/name: local-csi-driver
```

**关键特性**:
- **命名空间资源**: CSIStorageCapacity 属于特定 namespace (通常 kube-system)
- **自动创建**: CSI Driver 的 External Provisioner 自动创建/更新
- **调度优化**: 调度器根据容量信息避免调度到容量不足的节点

### 4.2 核心字段结构

```yaml
storageClassName: local-storage  # 关联的 StorageClass(必填)
nodeTopology:                    # 节点拓扑选择器(可选)
  matchLabels:
    kubernetes.io/hostname: node-01
capacity: 500Gi                  # 可用容量(可选,与 maximumVolumeSize 二选一)
maximumVolumeSize: 1Ti           # 单个卷的最大大小(可选)
```

### 4.3 使用场景

**场景 1: Local PV 容量感知**
```yaml
# Local PV 驱动报告每个节点的可用容量
apiVersion: storage.k8s.io/v1
kind: CSIStorageCapacity
metadata:
  name: local.csi.k8s.io-node-01
  namespace: kube-system
storageClassName: local-storage
nodeTopology:
  matchLabels:
    kubernetes.io/hostname: node-01
capacity: 500Gi                  # node-01 还有 500GB 可用

---
apiVersion: storage.k8s.io/v1
kind: CSIStorageCapacity
metadata:
  name: local.csi.k8s.io-node-02
  namespace: kube-system
storageClassName: local-storage
nodeTopology:
  matchLabels:
    kubernetes.io/hostname: node-02
capacity: 200Gi                  # node-02 还有 200GB 可用

---
# 调度器行为:
# PVC 请求 300Gi
# - node-01: 500Gi 可用 → ✅ 可调度
# - node-02: 200Gi 可用 → ❌ 容量不足
# 结果: Pod 调度到 node-01
```

**场景 2: 拓扑域容量报告**
```yaml
# 云环境按可用区报告容量
apiVersion: storage.k8s.io/v1
kind: CSIStorageCapacity
metadata:
  name: ebs.csi.aws.com-us-west-2a
  namespace: kube-system
storageClassName: ebs-gp3
nodeTopology:
  matchLabelExpressions:
  - key: topology.kubernetes.io/zone
    operator: In
    values:
    - us-west-2a
maximumVolumeSize: 16Ti          # AWS EBS gp3 最大 16TiB
# 注意: 云盘通常不报告 capacity (容量"无限")
```

### 4.4 配置示例

#### 示例 1: Local PV 容量
```yaml
apiVersion: storage.k8s.io/v1
kind: CSIStorageCapacity
metadata:
  name: local.csi.k8s.io-node-03-nvme
  namespace: kube-system
  labels:
    app.kubernetes.io/name: local-csi-driver
storageClassName: local-nvme
nodeTopology:
  matchLabels:
    kubernetes.io/hostname: node-03
    disk-type: nvme
capacity: 1Ti                    # node-03 的 NVMe 盘还有 1TB
```

#### 示例 2: 多可用区容量
```yaml
# 可用区 A
apiVersion: storage.k8s.io/v1
kind: CSIStorageCapacity
metadata:
  name: ceph-rbd-zone-a
  namespace: rook-ceph
storageClassName: ceph-rbd
nodeTopology:
  matchLabelExpressions:
  - key: topology.rook.io/zone
    operator: In
    values:
    - zone-a
capacity: 10Ti                   # zone-a 的 Ceph 池还有 10TB

---
# 可用区 B
apiVersion: storage.k8s.io/v1
kind: CSIStorageCapacity
metadata:
  name: ceph-rbd-zone-b
  namespace: rook-ceph
storageClassName: ceph-rbd
nodeTopology:
  matchLabelExpressions:
  - key: topology.rook.io/zone
    operator: In
    values:
    - zone-b
capacity: 5Ti                    # zone-b 的 Ceph 池还有 5TB
```

---

## 五、生产案例

### 5.1 案例 1: 部署 AWS EBS CSI Driver

**完整部署配置**:

```yaml
# 步骤 1: 创建 IAM 策略和角色 (AWS 控制台/CLI)
# 参考: https://github.com/kubernetes-sigs/aws-ebs-csi-driver/blob/master/docs/example-iam-policy.json

---
# 步骤 2: 创建 Kubernetes ServiceAccount (IRSA)
apiVersion: v1
kind: ServiceAccount
metadata:
  name: ebs-csi-controller-sa
  namespace: kube-system
  annotations:
    eks.amazonaws.com/role-arn: arn:aws:iam::123456789012:role/AmazonEKS_EBS_CSI_DriverRole

---
# 步骤 3: 部署 CSI Driver (Helm 推荐)
# helm repo add aws-ebs-csi-driver https://kubernetes-sigs.github.io/aws-ebs-csi-driver
# helm install aws-ebs-csi-driver aws-ebs-csi-driver/aws-ebs-csi-driver \
#   --namespace kube-system \
#   --set controller.serviceAccount.create=false \
#   --set controller.serviceAccount.name=ebs-csi-controller-sa

# 或手动部署 (简化示例):
---
apiVersion: storage.k8s.io/v1
kind: CSIDriver
metadata:
  name: ebs.csi.aws.com
spec:
  attachRequired: true
  podInfoOnMount: false
  volumeLifecycleModes:
  - Persistent
  - Ephemeral
  fsGroupPolicy: File

---
# Controller Deployment
apiVersion: apps/v1
kind: Deployment
metadata:
  name: ebs-csi-controller
  namespace: kube-system
spec:
  replicas: 2
  selector:
    matchLabels:
      app: ebs-csi-controller
  template:
    metadata:
      labels:
        app: ebs-csi-controller
    spec:
      serviceAccountName: ebs-csi-controller-sa
      priorityClassName: system-cluster-critical
      containers:
      - name: ebs-plugin
        image: public.ecr.aws/ebs-csi-driver/aws-ebs-csi-driver:v1.27.0
        args:
        - controller
        - --endpoint=$(CSI_ENDPOINT)
        - --logtostderr
        - --v=2
        env:
        - name: CSI_ENDPOINT
          value: unix:///var/lib/csi/sockets/pluginproxy/csi.sock
        - name: AWS_REGION
          value: us-west-2
        volumeMounts:
        - name: socket-dir
          mountPath: /var/lib/csi/sockets/pluginproxy/
      - name: csi-provisioner
        image: registry.k8s.io/sig-storage/csi-provisioner:v3.6.0
        args:
        - --csi-address=$(ADDRESS)
        - --v=2
        - --feature-gates=Topology=true
        - --enable-capacity
        - --capacity-ownerref-level=2
        env:
        - name: ADDRESS
          value: /var/lib/csi/sockets/pluginproxy/csi.sock
        volumeMounts:
        - name: socket-dir
          mountPath: /var/lib/csi/sockets/pluginproxy/
      - name: csi-attacher
        image: registry.k8s.io/sig-storage/csi-attacher:v4.4.0
        args:
        - --csi-address=$(ADDRESS)
        - --v=2
        env:
        - name: ADDRESS
          value: /var/lib/csi/sockets/pluginproxy/csi.sock
        volumeMounts:
        - name: socket-dir
          mountPath: /var/lib/csi/sockets/pluginproxy/
      - name: csi-resizer
        image: registry.k8s.io/sig-storage/csi-resizer:v1.9.0
        args:
        - --csi-address=$(ADDRESS)
        - --v=2
        env:
        - name: ADDRESS
          value: /var/lib/csi/sockets/pluginproxy/csi.sock
        volumeMounts:
        - name: socket-dir
          mountPath: /var/lib/csi/sockets/pluginproxy/
      - name: csi-snapshotter
        image: registry.k8s.io/sig-storage/csi-snapshotter:v6.3.0
        args:
        - --csi-address=$(ADDRESS)
        - --leader-election=true
        env:
        - name: ADDRESS
          value: /var/lib/csi/sockets/pluginproxy/csi.sock
        volumeMounts:
        - name: socket-dir
          mountPath: /var/lib/csi/sockets/pluginproxy/
      - name: liveness-probe
        image: registry.k8s.io/sig-storage/livenessprobe:v2.11.0
        args:
        - --csi-address=/csi/csi.sock
        volumeMounts:
        - name: socket-dir
          mountPath: /csi
      volumes:
      - name: socket-dir
        emptyDir: {}

---
# Node DaemonSet
apiVersion: apps/v1
kind: DaemonSet
metadata:
  name: ebs-csi-node
  namespace: kube-system
spec:
  selector:
    matchLabels:
      app: ebs-csi-node
  template:
    metadata:
      labels:
        app: ebs-csi-node
    spec:
      hostNetwork: true
      priorityClassName: system-node-critical
      tolerations:
      - operator: Exists
      containers:
      - name: ebs-plugin
        image: public.ecr.aws/ebs-csi-driver/aws-ebs-csi-driver:v1.27.0
        args:
        - node
        - --endpoint=$(CSI_ENDPOINT)
        - --logtostderr
        - --v=2
        env:
        - name: CSI_ENDPOINT
          value: unix:/csi/csi.sock
        securityContext:
          privileged: true
        volumeMounts:
        - name: kubelet-dir
          mountPath: /var/lib/kubelet
          mountPropagation: Bidirectional
        - name: plugin-dir
          mountPath: /csi
        - name: device-dir
          mountPath: /dev
      - name: node-driver-registrar
        image: registry.k8s.io/sig-storage/csi-node-driver-registrar:v2.9.0
        args:
        - --csi-address=$(ADDRESS)
        - --kubelet-registration-path=$(DRIVER_REG_SOCK_PATH)
        - --v=2
        env:
        - name: ADDRESS
          value: /csi/csi.sock
        - name: DRIVER_REG_SOCK_PATH
          value: /var/lib/kubelet/plugins/ebs.csi.aws.com/csi.sock
        volumeMounts:
        - name: plugin-dir
          mountPath: /csi
        - name: registration-dir
          mountPath: /registration
      - name: liveness-probe
        image: registry.k8s.io/sig-storage/livenessprobe:v2.11.0
        args:
        - --csi-address=/csi/csi.sock
        volumeMounts:
        - name: plugin-dir
          mountPath: /csi
      volumes:
      - name: kubelet-dir
        hostPath:
          path: /var/lib/kubelet
          type: Directory
      - name: plugin-dir
        hostPath:
          path: /var/lib/kubelet/plugins/ebs.csi.aws.com/
          type: DirectoryOrCreate
      - name: registration-dir
        hostPath:
          path: /var/lib/kubelet/plugins_registry/
          type: Directory
      - name: device-dir
        hostPath:
          path: /dev
          type: Directory

---
# 步骤 4: 创建 StorageClass
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: ebs-gp3
  annotations:
    storageclass.kubernetes.io/is-default-class: "true"
provisioner: ebs.csi.aws.com
volumeBindingMode: WaitForFirstConsumer
allowVolumeExpansion: true
reclaimPolicy: Delete
parameters:
  type: gp3
  iops: "3000"
  throughput: "125"
  encrypted: "true"

---
# 步骤 5: 测试 PVC
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: test-pvc
spec:
  accessModes: [ReadWriteOnce]
  storageClassName: ebs-gp3
  resources:
    requests:
      storage: 10Gi

---
# 步骤 6: 测试 Pod
apiVersion: v1
kind: Pod
metadata:
  name: test-pod
spec:
  containers:
  - name: app
    image: nginx
    volumeMounts:
    - name: data
      mountPath: /data
  volumes:
  - name: data
    persistentVolumeClaim:
      claimName: test-pvc
```

**验证步骤**:
```bash
# 1. 检查 CSI Driver 注册
kubectl get csidriver
# NAME              ATTACHREQUIRED   PODINFOONMOUNT   MODES        AGE
# ebs.csi.aws.com   true             false            Persistent   5m

# 2. 检查 Controller Pods
kubectl get pods -n kube-system -l app=ebs-csi-controller
# NAME                                  READY   STATUS    RESTARTS   AGE
# ebs-csi-controller-7d8c9f5b6b-abc12   5/5     Running   0          5m
# ebs-csi-controller-7d8c9f5b6b-def34   5/5     Running   0          5m

# 3. 检查 Node DaemonSet
kubectl get ds -n kube-system ebs-csi-node
# NAME            DESIRED   CURRENT   READY   UP-TO-DATE   AVAILABLE   NODE SELECTOR   AGE
# ebs-csi-node    3         3         3       3            3           <none>          5m

# 4. 检查 CSINode 对象
kubectl get csinode
# NAME                                            DRIVERS   AGE
# ip-10-0-1-100.us-west-2.compute.internal        1         5m

kubectl get csinode <node-name> -o yaml | grep -A 10 drivers

# 5. 测试 PVC 创建
kubectl apply -f test-pvc.yaml
kubectl get pvc test-pvc
# NAME       STATUS   VOLUME      CAPACITY   ACCESS MODES   STORAGECLASS   AGE
# test-pvc   Bound    pvc-abc123  10Gi       RWO            ebs-gp3        30s

# 6. 测试 Pod 挂载
kubectl apply -f test-pod.yaml
kubectl wait --for=condition=Ready pod/test-pod --timeout=120s
kubectl exec test-pod -- df -h /data
# Filesystem      Size  Used Avail Use% Mounted on
# /dev/nvme1n1    9.8G   24M  9.7G   1% /data
```

### 5.2 案例 2: 监控 CSI Driver

**Prometheus 监控配置**:

```yaml
# ServiceMonitor (Prometheus Operator)
apiVersion: monitoring.coreos.com/v1
kind: ServiceMonitor
metadata:
  name: ebs-csi-controller
  namespace: kube-system
spec:
  selector:
    matchLabels:
      app: ebs-csi-controller
  endpoints:
  - port: metrics
    path: /metrics
    interval: 30s

---
# 关键指标
# csi_operations_seconds_bucket - CSI 操作延迟直方图
# csi_operations_seconds_count - CSI 操作总数
# storage_operation_duration_seconds - 存储操作耗时

# Grafana 仪表板查询示例:
# 1. PV 供给成功率
# sum(rate(storage_operation_duration_seconds_count{operation_name="provision",status="success"}[5m])) 
# / 
# sum(rate(storage_operation_duration_seconds_count{operation_name="provision"}[5m]))

# 2. Attach 操作 P99 延迟
# histogram_quantile(0.99, 
#   sum(rate(csi_operations_seconds_bucket{method_name="ControllerPublishVolume"}[5m])) by (le)
# )

# 3. 失败的存储操作
# sum by (operation_name) (
#   rate(storage_operation_duration_seconds_count{status!="success"}[5m])
# )
```

**告警规则**:
```yaml
groups:
- name: csi-driver-alerts
  rules:
  # CSI 操作失败率 > 5%
  - alert: CSIOperationHighFailureRate
    expr: |
      sum(rate(storage_operation_duration_seconds_count{status!="success"}[5m])) by (operation_name)
      /
      sum(rate(storage_operation_duration_seconds_count[5m])) by (operation_name)
      > 0.05
    for: 10m
    labels:
      severity: warning
    annotations:
      summary: "CSI 操作 {{ $labels.operation_name }} 失败率过高"
  
  # Attach 超时 (> 2 分钟)
  - alert: CSIAttachTimeout
    expr: |
      histogram_quantile(0.99,
        sum(rate(csi_operations_seconds_bucket{method_name="ControllerPublishVolume"}[5m])) by (le)
      ) > 120
    labels:
      severity: critical
    annotations:
      summary: "CSI Attach 操作 P99 延迟超过 2 分钟"
  
  # Controller Pod 不健康
  - alert: CSIControllerDown
    expr: |
      kube_deployment_status_replicas_available{deployment="ebs-csi-controller"} == 0
    for: 5m
    labels:
      severity: critical
    annotations:
      summary: "CSI Controller 所有副本不可用"
```

---

## 六、故障排查

### 6.1 PV 无法 Attach

**症状**:
```bash
kubectl describe pod my-pod
# Events:
#   Warning  FailedAttachVolume  1m  attachdetach-controller  
#            AttachVolume.Attach failed for volume "pvc-abc123": rpc error: code = Internal
```

**排查步骤**:
```bash
# 1. 检查 VolumeAttachment 对象
kubectl get volumeattachment
# NAME                                                        ATTACHER          PV          NODE      ATTACHED   AGE
# csi-abc123                                                  ebs.csi.aws.com   pvc-abc123  node-01   false      5m

kubectl describe volumeattachment csi-abc123
# Events:
#   Warning  FailedAttach  1m  external-attacher  rpc error: code = ResourceExhausted desc = Node has reached max volume attach limit

# 2. 检查节点卷附加数
kubectl get csinode node-01 -o jsonpath='{.spec.drivers[?(@.name=="ebs.csi.aws.com")].allocatable.count}'
# 39 (最大限制)

kubectl get volumeattachment -o json | jq -r '.items[] | select(.spec.nodeName=="node-01") | .metadata.name' | wc -l
# 39 (已达上限)

# 3. 检查 External Attacher 日志
kubectl logs -n kube-system -l app=ebs-csi-controller -c csi-attacher

# 解决方案:
# - 删除未使用的 VolumeAttachment
# - 将 Pod 调度到其他节点
# - 使用 NVMe 实例类型(更高卷限制)
```

### 6.2 挂载失败

**症状**:
```bash
kubectl describe pod my-pod
# Events:
#   Warning  FailedMount  1m  kubelet  
#            MountVolume.SetUp failed for volume "pvc-abc123": rpc error: code = Internal desc = Could not mount
```

**排查步骤**:
```bash
# 1. 检查 Node Plugin 日志
kubectl logs -n kube-system -l app=ebs-csi-node -c ebs-plugin --tail=100 | grep pvc-abc123

# 2. 登录节点检查设备
# 找到 Pod 所在节点
kubectl get pod my-pod -o jsonpath='{.spec.nodeName}'
# node-01

# SSH 到节点
ssh user@node-01

# 检查块设备
lsblk
# NAME        MAJ:MIN RM SIZE RO TYPE MOUNTPOINT
# nvme1n1     259:0    0 100G  0 disk  (EBS 卷已 attach)

# 检查是否格式化
sudo file -s /dev/nvme1n1
# /dev/nvme1n1: data  (未格式化)

# 检查挂载点
mount | grep pvc-abc123

# 3. 检查 CSI Socket
ls -la /var/lib/kubelet/plugins/ebs.csi.aws.com/csi.sock
# srwxr-xr-x 1 root root 0 Feb 10 02:00 csi.sock

# 4. 手动测试 CSI 驱动
# 使用 csc 工具 (https://github.com/rexray/gocsi/tree/master/csc)
csc node stage \
  --endpoint unix:///var/lib/kubelet/plugins/ebs.csi.aws.com/csi.sock \
  --cap 1,mount,ext4 \
  --vol-context key=value \
  pvc-abc123 \
  /var/lib/kubelet/plugins/kubernetes.io/csi/pv/pvc-abc123/globalmount
```

### 6.3 容量不足调度失败

**症状**:
```bash
kubectl describe pod my-pod
# Events:
#   Warning  FailedScheduling  1m  default-scheduler  
#            0/3 nodes are available: 3 node(s) did not have enough local storage.
```

**排查步骤**:
```bash
# 1. 检查 CSIStorageCapacity
kubectl get csistoragecapacity -A
# NAMESPACE      NAME                              STORAGECLASS    CAPACITY
# kube-system    local-node-01                     local-storage   200Gi
# kube-system    local-node-02                     local-storage   500Gi
# kube-system    local-node-03                     local-storage   100Gi

# 2. 检查 PVC 请求
kubectl get pvc my-pvc -o jsonpath='{.spec.resources.requests.storage}'
# 300Gi (大于所有节点容量)

# 3. 更新 CSIStorageCapacity (如果驱动未自动更新)
# 驱动应自动更新,如未更新检查驱动日志:
kubectl logs -n kube-system -l app=local-csi-controller -c csi-provisioner

# 解决方案:
# - 减少 PVC 请求容量
# - 清理节点上未使用的数据
# - 添加更多节点或更大容量的磁盘
```

---

## 七、最佳实践总结

### 7.1 CSI Driver 部署

```yaml
# ✅ 使用官方 Helm Chart
# helm install <driver> <repo>/<chart> -n kube-system

# ✅ 高可用 Controller (至少 2 副本)
replicas: 2
affinity:
  podAntiAffinity:
    requiredDuringSchedulingIgnoredDuringExecution:
    - labelSelector:
        matchLabels:
          app: ebs-csi-controller
      topologyKey: kubernetes.io/hostname

# ✅ Node Plugin 容忍所有污点
tolerations:
- operator: Exists

# ✅ 配置资源限制
resources:
  requests:
    memory: "64Mi"
    cpu: "50m"
  limits:
    memory: "128Mi"
    cpu: "200m"

# ✅ 启用 Liveness Probe
livenessProbe:
  httpGet:
    path: /healthz
    port: 9808
  initialDelaySeconds: 10
  periodSeconds: 10
```

### 7.2 监控与告警

```yaml
# ✅ 关键指标
# - CSI 操作成功率
# - 操作延迟 (P50/P95/P99)
# - 失败事件
# - 卷附加数 (接近限制时告警)

# ✅ 日志收集
# 收集 Controller 和 Node Plugin 日志到中心化日志系统

# ✅ 定期测试
# - 创建/删除 PVC
# - 扩容 PVC
# - 快照创建/恢复
# - 灾难恢复演练
```

### 7.3 安全加固

```yaml
# ✅ 使用 IAM/RBAC 最小权限
# 仅授予 CSI Driver 必要的权限

# ✅ 启用加密
parameters:
  encrypted: "true"
  kmsKeyId: "arn:aws:kms:..."

# ✅ Secret 管理
# 使用 Kubernetes Secret 或外部 Secret Store

# ✅ 网络隔离
# CSI Controller 仅允许访问云 API
# Node Plugin 仅允许本地通信
```

---

## 八、参考资源

### 8.1 官方文档

- [CSI Specification](https://github.com/container-storage-interface/spec)
- [Kubernetes CSI Documentation](https://kubernetes-csi.github.io/docs/)
- [CSI Driver List](https://kubernetes-csi.github.io/docs/drivers.html)

### 8.2 主流 CSI Driver

- [AWS EBS CSI Driver](https://github.com/kubernetes-sigs/aws-ebs-csi-driver)
- [Azure Disk CSI Driver](https://github.com/kubernetes-sigs/azuredisk-csi-driver)
- [GCE PD CSI Driver](https://github.com/kubernetes-sigs/gcp-compute-persistent-disk-csi-driver)
- [Ceph CSI Driver](https://github.com/ceph/ceph-csi)
- [NFS CSI Driver](https://github.com/kubernetes-csi/csi-driver-nfs)

### 8.3 版本差异

| 功能 | v1.25 | v1.26 | v1.27 | v1.28 | v1.29 | v1.30 | v1.31 | v1.32 |
|------|-------|-------|-------|-------|-------|-------|-------|-------|
| CSIStorageCapacity GA | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| CSIDriver allocatable GA | Beta | Beta | Beta | GA | GA | GA | GA | GA |
| SELinux 挂载支持 | Alpha | Alpha | Alpha | Beta | Beta | GA | GA | GA |
| CSI Migration 完成 | Beta | GA | GA | GA | GA | GA | GA | GA |

---

**文档维护**: 2026-02  
**下一次审阅**: 2026-08 (Kubernetes v1.33 发布后)
