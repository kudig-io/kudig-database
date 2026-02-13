# CSI 存储驱动深度排查与架构优化指南

> **适用版本**: Kubernetes v1.25 - v1.32, CSI Spec v1.8+ | **最后更新**: 2026-02 | **难度**: 资深专家级

---

## 0. 读者对象与价值
| 角色 | 目标 | 核心收获 |
| :--- | :--- | :--- |
| **初学者** | 理解 CSI 的基本组成与日志查看方式 | 掌握 CSI 控制面与数据面的 Pod 分布、常用 Sidecar 职责与基础报错排查。 |
| **中级运维** | 解决存储扩容失败、快照无法创建等功能性问题 | 理解 `csi-resizer`、`csi-snapshotter` 工作流，掌握 gRPC 接口调用的基础诊断。 |
| **资深专家** | 处理底层 gRPC 通信异常、驱动版本迁移与性能瓶颈 | 深入 CSI 三大服务（Identity/Controller/Node）、掌握 `csc` 工具直接调试 Socket、解决 kubelet 与 CSI 注册冲突。 |

---

## 0. 10 分钟快速诊断

1. **CSI 组件就绪**：`kubectl get pods -n kube-system | grep -E "csi|storage"`，确认 controller/node 插件均 Running。
2. **驱动注册**：`kubectl get csinode <node> -o yaml`，确认驱动条目存在；节点上检查 `/var/lib/kubelet/plugins/` Socket。
3. **控制面日志**：`kubectl logs -n kube-system <csi-controller> -c csi-provisioner|csi-attacher`，定位 Create/Attach 失败原因。
4. **Node 侧日志**：`kubectl logs -n kube-system <csi-node-pod> -c <driver>`，查看 NodePublish/NodeStage 错误。
5. **超时/限流**：若 `DeadlineExceeded`，关注后端 API 速率限制与网络抖动。
6. **快速缓解**：
   - Socket 异常：重启 csi-node Pod，确认 hostPath 与 kubelet 根目录一致。
   - 资源过载：提升 controller 副本和资源限制，削峰 PVC 创建。
7. **证据留存**：保存 csi-controller/node 日志、CSINode 状态与失败请求时间点。

---

## 1. 核心架构与底层机制

### 1.1 CSI 三大 gRPC 服务深度解析

CSI（Container Storage Interface）驱动通过 **Unix Domain Socket (UDS)** 暴露三个标准 gRPC 服务，所有操作基于 CSI Spec v1.8+ 定义：

#### 1.1.1 Identity Service（身份与能力服务）

**职责**：提供驱动的基础信息与能力声明，是 K8s 与 CSI 驱动"握手"的第一步。

| gRPC 方法 | 作用 | 典型响应 |
|:---------|:-----|:---------|
| `GetPluginInfo` | 返回驱动名称和版本 | `name: "ebs.csi.aws.com"`, `version: "v1.15.0"` |
| `GetPluginCapabilities` | 声明支持的服务（Controller/Volume 扩展） | `CONTROLLER_SERVICE`, `VOLUME_ACCESSIBILITY_CONSTRAINTS` |
| `Probe` | 健康检查，确认驱动就绪 | `ready: true` |

**关键机制**：
- **驱动名称唯一性**：`name` 字段必须在集群全局唯一，格式通常为反向 DNS（如 `disk.csi.azure.com`）
- **能力协商**：K8s 根据 `Capabilities` 决定是否启用 Topology、快照等高级功能

```protobuf
// CSI Spec 定义
service Identity {
  rpc GetPluginInfo(GetPluginInfoRequest) returns (GetPluginInfoResponse);
  rpc GetPluginCapabilities(GetPluginCapabilitiesRequest) returns (GetPluginCapabilitiesResponse);
  rpc Probe(ProbeRequest) returns (ProbeResponse);
}
```

#### 1.1.2 Controller Service（控制面服务）

**职责**：在存储后端执行卷的生命周期管理（创建/删除/扩容/快照），运行在 **CSI Controller Pod** 中。

| gRPC 方法 | 作用 | K8s 调用时机 |
|:---------|:-----|:-------------|
| `CreateVolume` | 在后端创建卷（如云盘/NFS 目录） | PVC 创建时，由 `csi-provisioner` 触发 |
| `DeleteVolume` | 删除后端卷 | PVC 删除且 `reclaimPolicy: Delete` 时触发 |
| `ControllerPublishVolume` | 将卷关联到节点（Attach） | Pod 调度到节点后，由 `csi-attacher` 触发 |
| `ControllerUnpublishVolume` | 解除卷与节点的关联（Detach） | Pod 删除或迁移时触发 |
| `ControllerExpandVolume` | 在线扩容卷（控制面阶段） | PVC `spec.resources.requests.storage` 增大时触发 |
| `CreateSnapshot` | 创建卷快照 | `VolumeSnapshot` 对象创建时，由 `csi-snapshotter` 触发 |
| `ListVolumes` | 列出所有卷（用于自愈/审计） | 定期巡检或卷丢失时调用 |

**高级特性**：
- **Topology 感知**：`CreateVolume` 请求中携带 `accessibility_requirements`，确保卷创建在 Pod 可调度的可用区
- **多写冲突检测**：`ControllerPublishVolume` 会检查卷当前是否已附着到其他节点，防止 RWO 卷被多点挂载

```yaml
# 示例：ControllerPublishVolume 的输入参数
volume_id: "vol-1a2b3c4d"
node_id: "node-1"
volume_capability:
  access_mode:
    mode: SINGLE_NODE_WRITER  # RWO
  mount:
    fs_type: "ext4"
```

#### 1.1.3 Node Service（节点数据面服务）

**职责**：在节点上执行卷的挂载/卸载操作，运行在 **CSI Node DaemonSet** 中。

| gRPC 方法 | 作用 | 执行位置 |
|:---------|:-----|:---------|
| `NodeStageVolume` | **全局挂载**：格式化块设备并挂载到 `/var/lib/kubelet/plugins` | 每个节点的第一个 Pod 挂载时 |
| `NodeUnstageVolume` | 卸载全局挂载点 | 节点上所有使用该卷的 Pod 都删除后 |
| `NodePublishVolume` | **Pod 级挂载**：将全局挂载点绑定到 Pod 目录 | 每个 Pod 启动时 |
| `NodeUnpublishVolume` | 卸载 Pod 级挂载点 | Pod 删除时 |
| `NodeGetVolumeStats` | 获取卷的容量/inode 使用情况 | kubelet 定期调用（用于 `kubectl top pvc`） |
| `NodeExpandVolume` | 在线扩容文件系统（节点阶段） | `ControllerExpandVolume` 完成后触发 |

**关键机制**：
- **两阶段挂载设计**：
  - **Stage**：将块设备挂载到全局路径（如 `/var/lib/kubelet/plugins/kubernetes.io/csi/pv/<pv-name>/globalmount`），对节点上所有 Pod 共享
  - **Publish**：使用 Bind Mount 将 Stage 路径挂载到 Pod 目录（如 `/var/lib/kubelet/pods/<pod-uid>/volumes/...`）
  - **优势**：多个 Pod 使用同一 PV（RWX）时，只需 Stage 一次，减少格式化和设备操作

```
Node Filesystem Hierarchy:
/var/lib/kubelet/
├── plugins/
│   └── kubernetes.io~csi/
│       └── pv-abc123/
│           └── globalmount/  ← NodeStageVolume 挂载点
└── pods/
    └── <pod-uid>/
        └── volumes/
            └── kubernetes.io~csi/
                └── pv-abc123/  ← NodePublishVolume 绑定点
```

---

### 1.2 CSI Sidecar 容器链与调用流程

Kubernetes **不直接**与 CSI 驱动通信，而是通过一组官方维护的 **External Sidecar** 容器作为中间层：

#### 1.2.1 控制面 Sidecar 容器（运行在 CSI Controller Pod）

| Sidecar 名称 | 监听的 K8s 资源 | 触发的 CSI gRPC 调用 | 关键配置 |
|:------------|:----------------|:--------------------|:---------|
| **csi-provisioner** | `PersistentVolumeClaim` | `CreateVolume` / `DeleteVolume` | `--feature-gates=Topology=true` 启用拓扑感知 |
| **csi-attacher** | `VolumeAttachment` | `ControllerPublishVolume` / `ControllerUnpublishVolume` | `--timeout=60s` 设置 Attach 超时 |
| **csi-resizer** | `PVC.spec.resources.requests.storage` | `ControllerExpandVolume` | `--handle-volume-inuse-error=false` 避免扩容中的 Pod 重启 |
| **csi-snapshotter** | `VolumeSnapshot` | `CreateSnapshot` / `DeleteSnapshot` | `--extra-create-metadata=true` 添加快照元数据 |
| **livenessprobe** | - | `Probe` （定期健康检查） | `--probe-timeout=3s` |

**调用链示例（PVC 创建流程）**：
```
1. User 创建 PVC
   ↓
2. csi-provisioner 监听到 PVC（status: Pending）
   ↓
3. csi-provisioner 通过 UDS 调用驱动的 CreateVolume
   ↓
4. CSI 驱动连接存储后端（如 AWS API），创建 EBS 卷
   ↓
5. CSI 驱动返回 volume_id: "vol-abc123"
   ↓
6. csi-provisioner 创建 PV 对象，并绑定到 PVC
   ↓
7. PVC 状态变为 Bound
```

#### 1.2.2 节点侧 Sidecar 容器（运行在 CSI Node DaemonSet）

| Sidecar 名称 | 职责 | 关键配置 |
|:------------|:----|:---------|
| **node-driver-registrar** | 将 CSI 驱动的 Socket 路径注册到 kubelet 的插件管理器 | `--kubelet-registration-path=/var/lib/kubelet/plugins/<driver-name>/csi.sock` |

**注册流程**：
```
1. node-driver-registrar 启动时，连接驱动的 Socket
   ↓
2. 调用 GetPluginInfo 获取驱动名称
   ↓
3. 通过 kubelet 的注册 Socket 上报驱动信息
   Socket 路径：/var/lib/kubelet/plugins_registry/<driver-name>-reg.sock
   ↓
4. kubelet 更新 CSINode 对象，添加驱动条目
   ↓
5. kubelet 的 Volume Manager 开始信任该驱动，允许挂载卷
```

#### 1.2.3 完整架构图

```
┌─────────────────────────────────────────────────────────────────────────┐
│                        Kubernetes Control Plane                          │
│                                                                           │
│  ┌─────────┐  ┌─────────┐  ┌──────────────┐  ┌──────────────┐          │
│  │   PVC   │  │   PV    │  │VolumeAttach- │  │VolumeSnapshot│          │
│  │ Object  │  │ Object  │  │ment Object   │  │ Object       │          │
│  └────┬────┘  └────┬────┘  └──────┬───────┘  └──────┬───────┘          │
│       │            │              │                  │                   │
│       │ Watch      │ Watch        │ Watch            │ Watch             │
│       v            v              v                  v                   │
│  ┌────────────────────────────────────────────────────────────┐         │
│  │           CSI Controller Pod (Deployment)                   │         │
│  │  ┌──────────────┐ ┌──────────────┐ ┌──────────────┐       │         │
│  │  │csi-          │ │csi-          │ │csi-          │       │         │
│  │  │provisioner   │ │attacher      │ │snapshotter   │       │         │
│  │  └──────┬───────┘ └──────┬───────┘ └──────┬───────┘       │         │
│  │         │                │                │                │         │
│  │         └────────────────┴────────────────┘                │         │
│  │                          │                                  │         │
│  │                          v (Unix Domain Socket)             │         │
│  │                  ┌──────────────────┐                      │         │
│  │                  │  CSI Driver      │                      │         │
│  │                  │  (Controller)    │                      │         │
│  │                  └──────────────────┘                      │         │
│  └────────────────────────────┬─────────────────────────────────        │
└─────────────────────────────────────────────────────────────────────────┘
                                │ gRPC: CreateVolume
                                │       ControllerPublishVolume
                                v
┌─────────────────────────────────────────────────────────────────────────┐
│                         Storage Backend                                  │
│            (AWS EBS / Azure Disk / GCP PD / Ceph / NFS)                 │
└─────────────────────────────┬───────────────────────────────────────────┘
                              │ Attach Block Device to Node
                              v
┌─────────────────────────────────────────────────────────────────────────┐
│                         Worker Node                                      │
│                                                                           │
│  ┌──────────────────────────────────────────────────────────────┐       │
│  │                    kubelet                                     │       │
│  │  ┌──────────────────────────────────────────────────┐        │       │
│  │  │          Volume Manager                           │        │       │
│  │  │  ├─> Attach/Detach Controller (本地)             │        │       │
│  │  │  ├─> DesiredStateOfWorld Populator              │        │       │
│  │  │  └─> Volume Plugin Manager                       │        │       │
│  │  └──────────────────────────────────────────────────┘        │       │
│  └─────────────────────────────┬────────────────────────────────        │
│                                │ gRPC via Socket                         │
│                                v                                         │
│  ┌──────────────────────────────────────────────────────────────┐       │
│  │           CSI Node Pod (DaemonSet)                            │       │
│  │  ┌────────────────────┐  ┌────────────────────────┐          │       │
│  │  │node-driver-        │  │CSI Driver              │          │       │
│  │  │registrar           │  │(Node Service)          │          │       │
│  │  └────────┬───────────┘  └────────┬───────────────┘          │       │
│  │           │ Register              │                           │       │
│  │           └───────────────────────┘                           │       │
│  │                                   │                           │       │
│  │  Socket: /var/lib/kubelet/plugins/csi.sock                   │       │
│  └──────────────────────────────────────────────────────────────        │
│                                   │                                      │
│                                   v (NodeStageVolume)                    │
│  ┌──────────────────────────────────────────────────────────────┐       │
│  │  /var/lib/kubelet/plugins/kubernetes.io~csi/pv-abc/          │       │
│  │         globalmount/  ← 全局挂载点                            │       │
│  └──────────────────────────────────────────────────────────────        │
│                                   │                                      │
│                                   v (NodePublishVolume - Bind Mount)     │
│  ┌──────────────────────────────────────────────────────────────┐       │
│  │  /var/lib/kubelet/pods/<pod-uid>/volumes/                    │       │
│  │         kubernetes.io~csi/pv-abc/  ← Pod 挂载点               │       │
│  └──────────────────────────────────────────────────────────────        │
│                                   │                                      │
│                                   v (Bind Mount to Container)            │
│  ┌──────────────────────────────────────────────────────────────┐       │
│  │                    Pod Container                              │       │
│  │              volumeMounts:                                    │       │
│  │              - mountPath: /data                               │       │
│  └──────────────────────────────────────────────────────────────        │
└─────────────────────────────────────────────────────────────────────────┘
```

---

### 1.3 Socket 通信机制与故障隔离

#### 1.3.1 Unix Domain Socket 路径规范

| 组件 | Socket 路径 | 权限要求 |
|:----|:-----------|:---------|
| CSI Driver (Controller) | `/csi/csi.sock`（容器内） | `0660`，Sidecar 容器可访问 |
| CSI Driver (Node) | `/var/lib/kubelet/plugins/<driver-name>/csi.sock` | `0660`，kubelet 可访问 |
| kubelet Registration Socket | `/var/lib/kubelet/plugins_registry/<driver-name>-reg.sock` | `0660`，node-registrar 可写入 |

**关键配置点**：
- **HostPath 挂载**：CSI Node DaemonSet 必须将 `/var/lib/kubelet` 挂载到容器内，且 `mountPropagation: Bidirectional`
- **SELinux/AppArmor**：部分发行版需要设置 Socket 文件的安全上下文（如 `type: svirt_sandbox_file_t`）

#### 1.3.2 故障隔离机制

| 场景 | 隔离策略 | 技术实现 |
|:----|:--------|:--------|
| CSI 驱动崩溃 | 控制面与节点侧独立 | Controller Pod 崩溃不影响已挂载的卷；Node Pod 崩溃只影响新挂载 |
| Socket 文件损坏 | 自动重建 | kubelet 检测到 Socket 不可用时，会触发 Pod 重启 |
| gRPC 调用超时 | 指数退避重试 | Sidecar 默认超时 60s，失败后重试间隔：1s → 2s → 4s → 8s |
| 多驱动共存 | 命名空间隔离 | 每个驱动使用独立的 Socket 文件和注册路径 |

**高级调试**：
```bash
# 1. 查看 Socket 权限和所有者
ls -laZ /var/lib/kubelet/plugins/*/csi.sock

# 2. 测试 Socket 连通性（需安装 csc 工具）
csc identity plugin-info \
  --endpoint unix:///var/lib/kubelet/plugins/ebs.csi.aws.com/csi.sock

# 3. 监控 gRPC 调用耗时
kubectl logs -n kube-system csi-ebs-controller -c csi-provisioner \
  | grep "CreateVolume" | tail -20
```

---

## 1.4 CSI 完整生命周期流程

### 1.4.1 从 PVC 创建到 Pod 挂载的 7 个阶段

| 阶段 | 执行组件 | K8s 对象变化 | CSI gRPC 调用 | 耗时范围 |
|:----|:--------|:------------|:-------------|:---------|
| **1. Provisioning** | csi-provisioner | PVC: Pending → Bound | `CreateVolume` | 5-30s（依赖后端 API） |
| **2. Scheduling** | kube-scheduler | Pod: Pending → Scheduled | - | <1s |
| **3. Attaching** | csi-attacher | 创建 `VolumeAttachment` | `ControllerPublishVolume` | 10-60s（块设备附着） |
| **4. Waiting** | kubelet | `VolumeAttachment.status.attached: true` | - | <5s |
| **5. Staging** | kubelet + csi-node | 创建全局挂载点 | `NodeStageVolume` | 2-10s（格式化+挂载） |
| **6. Publishing** | kubelet + csi-node | 创建 Pod 挂载点 | `NodePublishVolume` | <1s（Bind Mount） |
| **7. Running** | kubelet | Pod: Running | - | <1s |

**关键观测点**：
```bash
# 查看 PVC 处于哪个阶段
kubectl describe pvc <name> | grep -A5 Events

# 查看 VolumeAttachment 状态（Attaching 阶段）
kubectl get volumeattachment -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{.status.attached}{"\n"}{end}'

# 查看 kubelet 日志（Staging/Publishing 阶段）
journalctl -u kubelet -f | grep -i "csi\|volume"
```

### 1.4.2 从 Pod 删除到卷释放的 5 个阶段

| 阶段 | 执行组件 | K8s 对象变化 | CSI gRPC 调用 | 故障风险点 |
|:----|:--------|:------------|:-------------|:----------|
| **1. Terminating** | kubelet | Pod: Terminating | - | 容器无法停止（SIGTERM/SIGKILL 超时） |
| **2. Unpublishing** | kubelet + csi-node | 删除 Pod 挂载点 | `NodeUnpublishVolume` | 文件句柄未释放（进程占用） |
| **3. Unstaging** | kubelet + csi-node | 删除全局挂载点 | `NodeUnstageVolume` | 设备繁忙（`umount: device busy`） |
| **4. Detaching** | csi-attacher | 删除 `VolumeAttachment` | `ControllerUnpublishVolume` | 后端 API 故障（云平台限流） |
| **5. Reclaiming** | csi-provisioner | 删除 PV（若 `reclaimPolicy: Delete`） | `DeleteVolume` | 数据删除失败（后端卷已手动删除） |

**典型故障链**：
```
Pod 删除超时 (30min+)
  ↓
Unpublishing 失败：进程 PID 12345 占用 /data
  ↓
kubelet 无法卸载，VolumeAttachment 卡住
  ↓
新 Pod 调度到其他节点，触发 Multi-Attach 错误
  ↓
StatefulSet 滚动更新失败
```

### 1.4.3 完整流程时序图

```
User                API Server         Scheduler          kubelet            CSI Controller      CSI Node         Storage Backend
 │                       │                  │                │                    │                  │                 │
 │ 1. Create PVC         │                  │                │                    │                  │                 │
 ├──────────────────────>│                  │                │                    │                  │                 │
 │                       │                  │                │                    │                  │                 │
 │                       │ 2. Watch PVC     │                │                    │                  │                 │
 │                       ├─────────────────────────────────────────────────────────>│                  │                 │
 │                       │                  │                │                    │                  │                 │
 │                       │                  │                │   3. CreateVolume  │                  │                 │
 │                       │                  │                │<───────────────────────────────────────┤                 │
 │                       │                  │                │                    │                  │                 │
 │                       │                  │                │                    │ 4. Create Volume │                 │
 │                       │                  │                │                    ├─────────────────────────────────────>│
 │                       │                  │                │                    │                  │                 │
 │                       │                  │                │   5. Return vol-id │                  │                 │
 │                       │                  │                │<───────────────────────────────────────┤                 │
 │                       │                  │                │                    │                  │                 │
 │                       │ 6. Create PV     │                │                    │                  │                 │
 │                       │<─────────────────────────────────────────────────────────                 │                 │
 │                       │                  │                │                    │                  │                 │
 │ 7. Create Pod         │                  │                │                    │                  │                 │
 ├──────────────────────>│                  │                │                    │                  │                 │
 │                       │                  │                │                    │                  │                 │
 │                       │ 8. Schedule Pod  │                │                    │                  │                 │
 │                       ├─────────────────>│                │                    │                  │                 │
 │                       │                  │                │                    │                  │                 │
 │                       │ 9. Create VA     │                │                    │                  │                 │
 │                       ├────────────────────────────────────────────────────────>│                  │                 │
 │                       │                  │                │                    │                  │                 │
 │                       │                  │                │ 10. ControllerPublishVolume            │                 │
 │                       │                  │                │<───────────────────────────────────────┤                 │
 │                       │                  │                │                    │                  │                 │
 │                       │                  │                │                    │ 11. Attach Volume│                 │
 │                       │                  │                │                    ├─────────────────────────────────────>│
 │                       │                  │                │                    │                  │                 │
 │                       │                  │ 12. Start Pod  │                    │                  │                 │
 │                       │                  ├───────────────>│                    │                  │                 │
 │                       │                  │                │                    │                  │                 │
 │                       │                  │                │ 13. NodeStageVolume                   │                 │
 │                       │                  │                ├──────────────────────────────────────>│                 │
 │                       │                  │                │                    │                  │                 │
 │                       │                  │                │                    │ 14. Format & Mount│                │
 │                       │                  │                │<──────────────────────────────────────┤                 │
 │                       │                  │                │                    │                  │                 │
 │                       │                  │                │ 15. NodePublishVolume                 │                 │
 │                       │                  │                ├──────────────────────────────────────>│                 │
 │                       │                  │                │                    │                  │                 │
 │                       │                  │                │                    │ 16. Bind Mount    │                 │
 │                       │                  │                │<──────────────────────────────────────┤                 │
 │                       │                  │                │                    │                  │                 │
 │                       │                  │                │ 17. Pod Running    │                  │                 │
 │<──────────────────────────────────────────────────────────┤                    │                  │                 │
```

**关键时间点监控**：
```yaml
# Prometheus 告警规则
- alert: CSIVolumeProvisioningSlowgender
  expr: histogram_quantile(0.99, rate(csi_sidecar_operations_seconds_bucket{method_name="CreateVolume"}[5m])) > 60
  annotations:
    summary: "CSI 卷创建 P99 延迟超过 60 秒"
    
- alert: CSIAttachingTimeout
  expr: (time() - kube_volumeattachment_created) > 300
  for: 5m
  annotations:
    summary: "VolumeAttachment {{ $labels.volumeattachment }} 附着超时（>5分钟）"
```

---

## 2. 专家级故障矩阵与观测工具

### 2.1 专家级故障矩阵（按生命周期分类）

#### 2.1.1 控制面阶段故障

| 现象分类 | 深度根因分析 | 关键观测指令 | 快速缓解策略 |
|:--------|:------------|:------------|:------------|
| **CreateVolume 超时** | 存储后端配额耗尽（如云平台单账户 EBS 卷数限制）、API 速率限制（429 Too Many Requests）、驱动程序死锁 | `kubectl logs <csi-controller> -c csi-provisioner \| grep "CreateVolume"` | 扩容配额；启用削峰（`volumeBindingMode: WaitForFirstConsumer`）；重启 CSI Controller |
| **VolumeAttachment 卡住** | 节点已下线但未清理附件、云平台 Detach API 失败、内核模块缺失（如 SCSI 驱动） | `kubectl get va -o wide`；节点执行 `lsblk` 确认设备状态 | 删除残留的 VolumeAttachment；手动在云控制台解绑卷；重启节点 |
| **Snapshot 永久 Pending** | 后端快照 ID 已失效但 `VolumeSnapshotContent` 未更新、CRD 版本不兼容（v1beta1 vs v1） | `kubectl get volumesnapshotcontent -o yaml` | 删除并重建快照；升级 snapshot-controller 至 v6.0+ |
| **ControllerExpandVolume 失败** | 卷正在使用中且驱动不支持在线扩容（Capability: `ONLINE_EXPAND` 缺失）、后端文件系统限制（如 ext4 最大 16TB） | `kubectl describe pvc \| grep "Resizing"` | 检查 CSIDriver Capability；必要时卸载后扩容 |

#### 2.1.2 节点侧阶段故障

| 现象分类 | 深度根因分析 | 关键观测指令 | 快速缓解策略 |
|:--------|:------------|:------------|:------------|
| **CSI Driver Not Registered** | Socket 目录未通过 HostPath 挂载、kubelet 根目录非默认路径（如 OpenShift 使用 `/var/data/kubelet`）、SELinux 阻止访问 | `kubectl get csinode <node> -o yaml`；节点检查 `ls -laZ /var/lib/kubelet/plugins/` | 修正 DaemonSet 的 `hostPath` 挂载路径；设置 SELinux 上下文（`chcon -t svirt_sandbox_file_t`） |
| **NodeStageVolume 超时** | 块设备格式化耗时长（大容量卷首次格式化可达 10min+）、文件系统损坏需要 fsck | `kubectl logs <csi-node-pod> -c <driver>` | 增加超时时间（`--timeout=600s`）；预先格式化卷（云平台快照） |
| **NodePublishVolume 失败** | 全局挂载点不存在（Stage 失败但未报错）、Bind Mount 权限错误（容器用户 UID 不匹配）、文件系统满（inode 耗尽） | 节点执行 `findmnt \| grep csi`；`df -i` 检查 inode | 修复 Stage 阶段错误；调整容器 `securityContext`；扩容或清理文件 |
| **Device Busy 无法卸载** | 进程占用挂载点（如僵尸进程、NFS 客户端卡住）、内核 bug（老版本内核的 mount namespace 泄漏） | `lsof +D /var/lib/kubelet/pods/<uid>/volumes/` | 强制杀死占用进程（`fuser -km <path>`）；重启节点；升级内核至 5.10+ |

#### 2.1.3 跨阶段复合故障

| 现象分类 | 深度根因分析 | 关键观测指令 | 快速缓解策略 |
|:--------|:------------|:------------|:------------|
| **Multi-Attach 错误** | 旧 Pod 的 VolumeAttachment 未清理（节点 NotReady）、RWO 卷被误设为 RWX | `kubectl get va -o json \| jq '.items[] \| select(.spec.nodeName=="<old-node>") \| .metadata.name'` | 删除残留附件；确认 AccessMode 配置；启用 VolumeAttachment 自动清理（见案例 1） |
| **PVC 扩容后容量未变** | ControllerExpandVolume 完成但 NodeExpandVolume 未触发、文件系统不支持在线扩容（需重启 Pod） | `kubectl get pvc -o jsonpath='{.status.capacity.storage}'` vs `kubectl exec <pod> -- df -h` | 重启 Pod（触发 NodeExpandVolume）；检查驱动是否支持 `NodeExpandVolume` |
| **gRPC Deadline Exceeded** | 存储后端延迟（如跨区域 NFS）、驱动内存泄漏导致 GC 停顿、网络抖动 | `kubectl logs <csi-pod> \| grep "DeadlineExceeded"` | 增加超时时间；优化后端性能（如启用缓存）；重启驱动 Pod |

---

### 2.2 专家工具箱

#### 2.2.1 CSI 驱动直接测试工具

```bash
# 1. 使用 csc 工具绕过 K8s 直接测试 CSI Socket
# 安装 csc（需在节点或 CSI Pod 内执行）
go install github.com/rexray/gocsi/csc@latest

# 获取驱动信息
csc identity plugin-info \
  --endpoint unix:///var/lib/kubelet/plugins/ebs.csi.aws.com/csi.sock

# 获取驱动能力
csc identity plugin-capabilities \
  --endpoint unix:///var/lib/kubelet/plugins/ebs.csi.aws.com/csi.sock

# 获取节点信息
csc node get-info \
  --endpoint unix:///var/lib/kubelet/plugins/ebs.csi.aws.com/csi.sock
```

#### 2.2.2 gRPC 调用抓包工具

```bash
# 使用 grpcurl 访问 UDS（需安装 grpcurl）
# 查看支持的服务
grpcurl -unix -plaintext \
  /var/lib/kubelet/plugins/ebs.csi.aws.com/csi.sock \
  list

# 调用 GetPluginInfo
grpcurl -unix -plaintext \
  /var/lib/kubelet/plugins/ebs.csi.aws.com/csi.sock \
  csi.v1.Identity/GetPluginInfo

# 调用 NodeGetVolumeStats（查看卷使用情况）
grpcurl -unix -plaintext \
  -d '{"volume_id": "vol-abc123", "volume_path": "/var/lib/kubelet/pods/.../volumes/..."}' \
  /var/lib/kubelet/plugins/ebs.csi.aws.com/csi.sock \
  csi.v1.Node/NodeGetVolumeStats
```

#### 2.2.3 CSI 对象巡检脚本

```bash
#!/bin/bash
# 巡检所有 CSI 组件状态

echo "=== CSI Drivers ==="
kubectl get csidrivers

echo -e "\n=== CSI Nodes ==="
kubectl get csinodes -o custom-columns=\
NAME:.metadata.name,\
DRIVERS:.spec.drivers[*].name

echo -e "\n=== CSI Controller Pods ==="
kubectl get pods -n kube-system -l app=csi-controller -o wide

echo -e "\n=== CSI Node Pods ==="
kubectl get pods -n kube-system -l app=csi-node -o wide

echo -e "\n=== VolumeAttachments (Last 10) ==="
kubectl get volumeattachment --sort-by=.metadata.creationTimestamp \
  -o custom-columns=\
NAME:.metadata.name,\
PV:.spec.source.persistentVolumeName,\
NODE:.spec.nodeName,\
ATTACHED:.status.attached,\
AGE:.metadata.creationTimestamp | tail -10

echo -e "\n=== Stuck VolumeAttachments (Age > 5min, Not Attached) ==="
kubectl get volumeattachment -o json | jq -r '
  .items[] |
  select(.status.attached == false) |
  select((now - (.metadata.creationTimestamp | fromdateiso8601)) > 300) |
  [.metadata.name, .spec.nodeName, .spec.source.persistentVolumeName] |
  @tsv
' | column -t

echo -e "\n=== CSI Driver Logs (Recent Errors) ==="
for pod in $(kubectl get pods -n kube-system -l app=csi-controller -o name); do
  echo "--- $pod ---"
  kubectl logs -n kube-system $pod --tail=50 | grep -i "error\|failed\|timeout" | tail -5
done
```

#### 2.2.4 监控告警规则

```yaml
# Prometheus 告警规则（保存为 csi-alerts.yaml）
groups:
- name: csi-storage
  interval: 30s
  rules:
  # 1. CSI 驱动未注册
  - alert: CSIDriverNotRegistered
    expr: kube_csinode_info == 0
    for: 5m
    labels:
      severity: critical
    annotations:
      summary: "节点 {{ $labels.node }} 的 CSI 驱动未注册"
      description: "检查 node-driver-registrar 日志和 Socket 路径"

  # 2. VolumeAttachment 长时间未附着
  - alert: VolumeAttachmentStuck
    expr: (time() - kube_volumeattachment_created{attached="false"}) > 300
    for: 5m
    labels:
      severity: warning
    annotations:
      summary: "VolumeAttachment {{ $labels.volumeattachment }} 附着超时"
      description: "检查 csi-attacher 日志和存储后端状态"

  # 3. CSI 操作延迟
  - alert: CSIOperationSlow
    expr: histogram_quantile(0.99, rate(csi_sidecar_operations_seconds_bucket[5m])) > 60
    for: 10m
    labels:
      severity: warning
    annotations:
      summary: "CSI {{ $labels.method_name }} 操作 P99 延迟 > 60s"
      description: "存储后端性能下降或 API 限流"

  # 4. CSI Controller Pod 重启频繁
  - alert: CSIControllerRestartLoop
    expr: rate(kube_pod_container_status_restarts_total{pod=~"csi-.*-controller.*"}[15m]) > 0.5
    for: 15m
    labels:
      severity: critical
    annotations:
      summary: "CSI Controller {{ $labels.pod }} 重启频繁（>7次/15min）"
      description: "检查 OOM、驱动 panic 或配置错误"

  # 5. CSI Node Pod 不健康
  - alert: CSINodePodDown
    expr: kube_pod_status_phase{pod=~"csi-.*-node.*", phase!="Running"} == 1
    for: 5m
    labels:
      severity: critical
    annotations:
      summary: "CSI Node Pod {{ $labels.pod }} 在节点 {{ $labels.node }} 未运行"
      description: "新 Pod 将无法挂载卷"

  # 6. NodeStageVolume 失败率高
  - alert: CSIStagingFailureHigh
    expr: rate(csi_sidecar_operations_seconds_count{method_name="NodeStageVolume", grpc_status_code!="OK"}[5m]) > 0.1
    for: 10m
    labels:
      severity: warning
    annotations:
      summary: "NodeStageVolume 失败率 > 10%"
      description: "检查节点文件系统和块设备状态"
```

#### 2.2.5 自动化修复脚本

```bash
#!/bin/bash
# 自动清理卡住的 VolumeAttachment（谨慎使用！）

# 查找附着在 NotReady 节点的 VolumeAttachment
NOT_READY_NODES=$(kubectl get nodes --no-headers | awk '$2 != "Ready" {print $1}')

for node in $NOT_READY_NODES; do
  echo "Processing NotReady node: $node"
  
  # 获取该节点的 VolumeAttachment
  kubectl get volumeattachment -o json | jq -r \
    --arg node "$node" \
    '.items[] | select(.spec.nodeName == $node) | .metadata.name' \
  | while read va; do
    echo "  Deleting VolumeAttachment: $va"
    kubectl delete volumeattachment "$va" --force --grace-period=0
  done
done

echo "Cleanup completed. Verify with: kubectl get volumeattachment"
```

---

## 3. 深度排查路径

### 3.1 第一阶段：控制面 Sidecar 协同验证

**目标**：确认 API Server 的变更（PVC/VA/Snapshot）是否被 Sidecar 正确捕获并转换为 CSI 调用。

```bash
# 1. 检查 Provisioner 是否捕获了创建请求
kubectl logs -n kube-system <csi-controller> -c csi-provisioner \
  | grep "CreateVolume" | tail -20

# 预期输出示例：
# I1210 10:23:15 provision.go:123] CreateVolume called for PVC "default/data-mysql-0"
# I1210 10:23:18 provision.go:145] CreateVolume succeeded: volume_id="vol-abc123"

# 2. 检查 Attacher 是否成功调用后端 API 完成 Attach
kubectl logs -n kube-system <csi-controller> -c csi-attacher \
  | grep "ControllerPublishVolume" | tail -20

# 预期输出示例：
# I1210 10:24:01 attacher.go:89] ControllerPublishVolume: volume="vol-abc123" node="node-1"
# I1210 10:24:05 attacher.go:102] ControllerPublishVolume succeeded

# 3. 检查 Resizer 是否触发了扩容
kubectl logs -n kube-system <csi-controller> -c csi-resizer \
  | grep "ControllerExpandVolume" | tail -20

# 4. 查看 Leader Election 状态（多副本场景）
kubectl get lease -n kube-system | grep csi
# 预期：只有一个 Holder 持有锁
```

**常见异常信号**：
- `"failed to create volume: Unauthorized"` → CSI 驱动的云平台凭证过期
- `"quota exceeded"` → 存储后端配额不足
- `"topology constraint not satisfied"` → 可用区约束无法满足

---

### 3.2 第二阶段：Node 侧注册与挂载验证

**目标**：确认 kubelet 是否能通过 Socket 与驱动"对话"，以及挂载操作是否成功。

```bash
# 1. 验证驱动是否成功注册到 kubelet
kubectl get csinode <node-name> -o yaml | grep -A5 drivers:

# 预期输出：
# spec:
#   drivers:
#   - name: ebs.csi.aws.com
#     nodeID: i-0a1b2c3d4e5f6g7h8
#     topologyKeys:
#     - topology.ebs.csi.aws.com/zone

# 2. 查看 node-registrar 日志
kubectl logs -n kube-system <csi-node-pod> -c node-driver-registrar

# 预期输出：
# I1210 10:20:01 main.go:71] Registration Server started at: /registration/ebs.csi.aws.com-reg.sock
# I1210 10:20:02 main.go:80] Registration process completed

# 3. 在节点上检查 Socket 文件
ls -la /var/lib/kubelet/plugins/*/csi.sock
# 预期：-rw-rw---- 1 root root 0 Dec 10 10:20 /var/lib/kubelet/plugins/ebs.csi.aws.com/csi.sock

# 4. 测试 Socket 可用性
csc identity plugin-info \
  --endpoint unix:///var/lib/kubelet/plugins/ebs.csi.aws.com/csi.sock

# 5. 查看 kubelet 的卷操作日志
journalctl -u kubelet -f | grep -i "csi\|operationexecutor"

# 预期：看到 MountVolume.MountDevice、MountVolume.SetUp 等操作
```

**常见异常信号**：
- `"driver name ebs.csi.aws.com not found in the list of registered CSI drivers"` → 注册失败
- `"socket file /var/lib/kubelet/plugins/.../csi.sock does not exist"` → DaemonSet hostPath 配置错误
- `"permission denied"` → SELinux/AppArmor 阻止访问

---

### 3.3 第三阶段：存储后端与数据平面深度诊断

**目标**：排查底层块设备、文件系统和云平台 API 层面的问题。

```bash
# 1. 查看节点上的块设备列表
lsblk -o NAME,SIZE,TYPE,MOUNTPOINT,FSTYPE
# 预期：看到 CSI 卷对应的块设备（如 /dev/nvme1n1）

# 2. 检查设备是否已挂载
findmnt -t ext4,xfs,nfs | grep csi

# 3. 查看设备挂载详情
mount | grep "/var/lib/kubelet/plugins/kubernetes.io~csi"

# 4. 检查文件系统健康状态（需 root 权限）
sudo fsck -n /dev/nvme1n1p1  # -n 表示只读模式

# 5. 查看设备 IO 统计
iostat -x 1 5 | grep nvme1n1
# 关注 %util（使用率）、await（平均等待时间）

# 6. 抓取存储后端 API 调用（以 AWS EBS 为例）
# 在 CSI Controller Pod 内启用 debug 日志
kubectl set env deployment/ebs-csi-controller \
  -n kube-system \
  AWS_SDK_LOAD_CONFIG=1 \
  AWS_LOG_LEVEL=debug

# 查看 AWS API 调用详情
kubectl logs -n kube-system <csi-controller> -c ebs-plugin | grep "DEBUG"

# 7. 验证云平台卷状态（以 AWS 为例）
aws ec2 describe-volumes --volume-ids vol-abc123 \
  --query 'Volumes[0].[State,Attachments[0].State]' \
  --output text
# 预期：available 或 in-use, attached
```

**云平台常见故障**：
- **AWS EBS**：
  - `VolumeInUse` 错误：卷已附着到其他实例，需手动 Detach
  - `RequestLimitExceeded`：API 调用频率超限，需启用指数退避
- **Azure Disk**：
  - `OperationNotAllowed`：磁盘类型不支持在线扩容（如 Standard HDD）
  - `AttachDiskWhileBeingDetached`：并发操作冲突，需等待 Detach 完成
- **GCP Persistent Disk**：
  - `RESOURCE_EXHAUSTED`：区域 SSD 配额不足
  - `INVALID_ARGUMENT`：磁盘大小不满足最小增量（如 PD-SSD 最小 10GB）

---

### 3.4 第四阶段：完整链路跟踪与时间线重建

**目标**：重建从 PVC 创建到 Pod 运行的完整时间线，定位耗时瓶颈。

```bash
#!/bin/bash
# 脚本：csi-timeline.sh
# 用法：./csi-timeline.sh <pvc-name> <namespace>

PVC_NAME=$1
NAMESPACE=$2

echo "=== PVC Timeline ==="
kubectl get events -n $NAMESPACE --sort-by='.lastTimestamp' \
  --field-selector involvedObject.name=$PVC_NAME \
  -o custom-columns=TIME:.lastTimestamp,TYPE:.type,REASON:.reason,MESSAGE:.message

echo -e "\n=== PV Timeline ==="
PV_NAME=$(kubectl get pvc -n $NAMESPACE $PVC_NAME -o jsonpath='{.spec.volumeName}')
kubectl get events --sort-by='.lastTimestamp' \
  --field-selector involvedObject.name=$PV_NAME \
  -o custom-columns=TIME:.lastTimestamp,TYPE:.type,REASON:.reason,MESSAGE:.message

echo -e "\n=== VolumeAttachment Timeline ==="
VA_NAME=$(kubectl get volumeattachment -o json | jq -r \
  --arg pv "$PV_NAME" \
  '.items[] | select(.spec.source.persistentVolumeName == $pv) | .metadata.name')
if [ -n "$VA_NAME" ]; then
  kubectl get events --sort-by='.lastTimestamp' \
    --field-selector involvedObject.name=$VA_NAME \
    -o custom-columns=TIME:.lastTimestamp,TYPE:.type,REASON:.reason,MESSAGE:.message
fi

echo -e "\n=== Pod Timeline ==="
POD_NAME=$(kubectl get pods -n $NAMESPACE -o json | jq -r \
  --arg pvc "$PVC_NAME" \
  '.items[] | select(.spec.volumes[]?.persistentVolumeClaim.claimName == $pvc) | .metadata.name' \
  | head -1)
if [ -n "$POD_NAME" ]; then
  kubectl get events -n $NAMESPACE --sort-by='.lastTimestamp' \
    --field-selector involvedObject.name=$POD_NAME \
    -o custom-columns=TIME:.lastTimestamp,TYPE:.type,REASON:.reason,MESSAGE:.message
fi
```

**示例输出解读**：
```
=== PVC Timeline ===
TIME                        TYPE     REASON                MESSAGE
2026-02-10T10:23:15+08:00  Normal   Provisioning          External provisioner is provisioning volume
2026-02-10T10:23:18+08:00  Normal   ProvisioningSucceeded Successfully provisioned volume vol-abc123

=== VolumeAttachment Timeline ===
TIME                        TYPE     REASON                MESSAGE
2026-02-10T10:24:01+08:00  Normal   SuccessfulAttach      Attach volume "vol-abc123" to node "node-1" succeeded

=== Pod Timeline ===
TIME                        TYPE     REASON                MESSAGE
2026-02-10T10:24:05+08:00  Normal   Scheduled             Successfully assigned default/mysql-0 to node-1
2026-02-10T10:24:07+08:00  Normal   SuccessfulAttachVolume AttachVolume.Attach succeeded
2026-02-10T10:24:12+08:00  Normal   SuccessfulMountVolume  MountVolume.SetUp succeeded
2026-02-10T10:24:15+08:00  Normal   Started               Started container mysql

# 时间线分析：
# - Provisioning: 3s (正常)
# - Attaching: 3s (正常)
# - Mounting: 5s (正常)
# - Total: 15s (符合预期)
```

---

## 4. 深度解决方案与生产最佳实践

### 4.1 解决"Socket 路径不一致"导致的注册失败

**场景**：某些发行版（如 OpenShift、Rancher）的 kubelet 目录非默认的 `/var/lib/kubelet`。

**根因**：CSI Node DaemonSet 的 `hostPath` 挂载路径与节点实际路径不匹配，导致 Socket 文件无法被 kubelet 发现。

**对策**：

1. **查找节点的 kubelet 根目录**：
```bash
# 方法 1：查看 kubelet 启动参数
ps aux | grep kubelet | grep -o -- '--root-dir=[^ ]*'
# 输出示例：--root-dir=/var/data/kubelet

# 方法 2：查看现有 CSI 插件的挂载点
kubectl get csinode <node> -o yaml | grep allocatableVolumes -A5
```

2. **修改 CSI Node DaemonSet**：
```yaml
apiVersion: apps/v1
kind: DaemonSet
metadata:
  name: csi-node
spec:
  template:
    spec:
      containers:
      - name: node-driver-registrar
        args:
        - --kubelet-registration-path=/var/data/kubelet/plugins/ebs.csi.aws.com/csi.sock  # 修改这里
      - name: ebs-plugin
        volumeMounts:
        - name: kubelet-dir
          mountPath: /var/lib/kubelet
          mountPropagation: Bidirectional
      volumes:
      - name: kubelet-dir
        hostPath:
          path: /var/data/kubelet  # 修改这里
          type: Directory
```

3. **验证修复**：
```bash
# 检查 Socket 文件是否存在
kubectl exec -n kube-system <csi-node-pod> -c node-driver-registrar -- \
  ls -la /var/lib/kubelet/plugins/ebs.csi.aws.com/csi.sock

# 检查 CSINode 对象是否更新
kubectl get csinode <node> -o yaml | grep -A3 drivers:
```

---

### 4.2 处理"CSI 驱动升级引发的挂载点残留"

**场景**：升级 CSI 驱动版本（如从 v1.10 → v1.15）后，部分节点的卷无法卸载，Pod 卡在 Terminating。

**根因**：
- 新旧版本的驱动名称不一致（如 `ebs.csi.aws.com` → `ebs.csi.aws.com.v2`）
- 挂载点被旧版本驱动占用，新版本驱动无法识别

**对策**：

1. **升级前检查驱动名称**：
```bash
# 查看当前驱动名称
kubectl get csidriver -o custom-columns=NAME:.metadata.name

# 查看新版本驱动的名称（从镜像或文档确认）
kubectl run tmp --rm -it --image=<new-csi-image> -- \
  /csi-driver --version
```

2. **优雅升级流程**（适用于有状态应用）：
```bash
# Step 1: Cordon 节点，停止新 Pod 调度
kubectl cordon node-1

# Step 2: Drain 节点（触发 Pod 迁移）
kubectl drain node-1 --ignore-daemonsets --delete-emptydir-data --timeout=10m

# Step 3: 升级 CSI 驱动（滚动更新 DaemonSet）
kubectl set image daemonset/csi-node \
  -n kube-system \
  ebs-plugin=<new-image>

# Step 4: 等待驱动就绪
kubectl rollout status daemonset/csi-node -n kube-system

# Step 5: Uncordon 节点
kubectl uncordon node-1
```

3. **强制清理残留挂载点**（风险操作！仅限紧急情况）：
```bash
# 在节点上执行（需 root 权限）
# 1. 查找残留挂载点
mount | grep "/var/lib/kubelet/pods" | grep csi

# 2. 强制卸载（示例）
umount -f -l /var/lib/kubelet/pods/<pod-uid>/volumes/kubernetes.io~csi/pv-abc123

# 3. 清理 VolumeAttachment
kubectl delete volumeattachment csi-<hash> --force --grace-period=0

# 4. 清理 Pod
kubectl delete pod <pod-name> -n <namespace> --force --grace-period=0
```

---

### 4.3 开启"存储容量感知调度"（Storage Capacity Tracking）

**场景**：使用本地存储（Local PV）或容量受限的存储后端时，避免 Pod 调度到空间不足的节点。

**原理**：CSI 驱动定期上报节点的可用存储容量，kube-scheduler 在调度时过滤掉容量不足的节点。

**配置步骤**：

1. **启用 Feature Gate**（K8s 1.24+ 默认启用）：
```yaml
# kube-scheduler 配置
apiVersion: kubescheduler.config.k8s.io/v1
kind: KubeSchedulerConfiguration
featureGates:
  CSIStorageCapacity: true
```

2. **CSI 驱动侧配置**：
```yaml
# CSIDriver 对象
apiVersion: storage.k8s.io/v1
kind: CSIDriver
metadata:
  name: local.csi.example.com
spec:
  storageCapacity: true  # 启用容量跟踪
  volumeLifecycleModes:
  - Persistent
```

3. **CSI Controller 启用容量上报**：
```yaml
# 在 CSI Controller Deployment 中添加 Sidecar
apiVersion: apps/v1
kind: Deployment
metadata:
  name: csi-controller
spec:
  template:
    spec:
      containers:
      - name: csi-provisioner
        args:
        - --enable-capacity=true  # 启用容量计算
        - --capacity-poll-interval=5m  # 轮询间隔
```

4. **验证容量信息**：
```bash
# 查看 CSIStorageCapacity 对象
kubectl get csistoragecapacities -o wide

# 示例输出：
# NAME                      STORAGECLASS   CAPACITY   NODE     AGE
# local-storage-node-1      local-path     100Gi      node-1   5m
# local-storage-node-2      local-path     50Gi       node-2   5m
```

---

### 4.4 配置"延迟绑定"（Late Binding）削峰填谷

**场景**：大批量创建 PVC 时（如批量部署 StatefulSet），避免集中触发存储后端 API，造成速率限制或配额耗尽。

**配置**：
```yaml
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: ebs-delayed
provisioner: ebs.csi.aws.com
volumeBindingMode: WaitForFirstConsumer  # 关键配置
parameters:
  type: gp3
  fsType: ext4
```

**效果对比**：
- **Immediate**（默认）：PVC 创建后立即 Provision，可能导致 API 突发
- **WaitForFirstConsumer**：Pod 调度到节点后才 Provision，自然削峰

---

## 5. 生产环境典型案例解析

### 5.1 案例一：Socket 路径不一致导致 30% 节点批量挂载失败

#### 5.1.1 故障现场

**时间线**：
- **T+0min**：运维团队在 Rancher 集群（使用自定义 kubelet 路径）上部署 AWS EBS CSI 驱动 v1.15
- **T+5min**：用户创建 100 个 StatefulSet Pod，30% 的 Pod 卡在 `ContainerCreating`
- **T+10min**：查看事件发现错误：`MountVolume.SetUp failed: driver name ebs.csi.aws.com not found in the list of registered CSI drivers`
- **T+15min**：查询发现故障节点均分布在特定节点池（节点池 B，使用 `/opt/rancher/kubelet` 作为根目录）

**初始假设**：
- ❌ CSI Controller Pod 故障 → 已验证 Controller 正常运行
- ❌ 网络分区 → 已验证节点网络正常
- ❌ kubelet 版本不兼容 → 已验证版本一致（v1.28.5）

#### 5.1.2 深度排查过程

**Step 1：检查 CSINode 对象**
```bash
# 正常节点（节点池 A）
kubectl get csinode node-a-1 -o yaml
# spec:
#   drivers:
#   - name: ebs.csi.aws.com
#     nodeID: i-0a1b2c3d
#     ✅ 驱动已注册

# 故障节点（节点池 B）
kubectl get csinode node-b-1 -o yaml
# spec:
#   drivers: []  # ❌ 驱动列表为空
```

**Step 2：检查 node-driver-registrar 日志**
```bash
kubectl logs -n kube-system csi-ebs-node-xxx -c node-driver-registrar

# 错误日志：
# E1210 10:25:03 main.go:71] Failed to register driver: 
#   open /var/lib/kubelet/plugins_registry/ebs.csi.aws.com-reg.sock: no such file or directory
```

**Step 3：检查节点的 kubelet 根目录**
```bash
# 在故障节点上执行
ps aux | grep kubelet | grep -o -- '--root-dir=[^ ]*'
# --root-dir=/opt/rancher/kubelet  # ❌ 非默认路径

# 检查 Socket 实际位置
ls /opt/rancher/kubelet/plugins_registry/
# （空目录，Socket 未创建）

# 检查 CSI Pod 的挂载点
kubectl exec -n kube-system csi-ebs-node-xxx -c ebs-plugin -- \
  ls /var/lib/kubelet/plugins_registry/
# （Pod 内路径正确，但主机路径错误）
```

**Step 4：定位根因**
- CSI Node DaemonSet 使用硬编码的 `/var/lib/kubelet`
- 节点池 B 的 kubelet 使用 `/opt/rancher/kubelet`
- Pod 内的路径映射错误，导致 Socket 文件无法被 kubelet 发现

#### 5.1.3 解决方案

**立即缓解**（15 分钟）：
```bash
# 1. 为节点池 B 创建专用的 CSI Node DaemonSet
kubectl apply -f - <<EOF
apiVersion: apps/v1
kind: DaemonSet
metadata:
  name: csi-ebs-node-rancher
  namespace: kube-system
spec:
  selector:
    matchLabels:
      app: ebs-csi-node-rancher
  template:
    metadata:
      labels:
        app: ebs-csi-node-rancher
    spec:
      nodeSelector:
        node-pool: pool-b  # 仅调度到节点池 B
      containers:
      - name: node-driver-registrar
        image: registry.k8s.io/sig-storage/csi-node-driver-registrar:v2.9.0
        args:
        - --kubelet-registration-path=/opt/rancher/kubelet/plugins/ebs.csi.aws.com/csi.sock  # 修改路径
        volumeMounts:
        - name: registration-dir
          mountPath: /registration
      - name: ebs-plugin
        image: public.ecr.aws/ebs-csi-driver/aws-ebs-csi-driver:v1.15.0
        securityContext:
          privileged: true
        volumeMounts:
        - name: kubelet-dir
          mountPath: /var/lib/kubelet
          mountPropagation: Bidirectional
        - name: plugin-dir
          mountPath: /csi
      volumes:
      - name: kubelet-dir
        hostPath:
          path: /opt/rancher/kubelet  # 修改路径
          type: Directory
      - name: plugin-dir
        hostPath:
          path: /opt/rancher/kubelet/plugins/ebs.csi.aws.com  # 修改路径
          type: DirectoryOrCreate
      - name: registration-dir
        hostPath:
          path: /opt/rancher/kubelet/plugins_registry  # 修改路径
          type: Directory
EOF

# 2. 删除原有的 CSI Node Pod（触发重建）
kubectl delete pod -n kube-system -l app=ebs-csi-node --field-selector spec.nodeName=node-b-1
```

**长期优化**（自动检测 kubelet 路径）：
```yaml
# 使用 Init Container 动态获取 kubelet 路径
apiVersion: apps/v1
kind: DaemonSet
metadata:
  name: csi-ebs-node
spec:
  template:
    spec:
      initContainers:
      - name: detect-kubelet-dir
        image: busybox:1.36
        command:
        - sh
        - -c
        - |
          # 从节点进程获取 kubelet 根目录
          KUBELET_DIR=$(grep -o '\--root-dir=[^ ]*' /host/proc/*/cmdline 2>/dev/null | head -1 | cut -d= -f2)
          if [ -z "$KUBELET_DIR" ]; then
            KUBELET_DIR="/var/lib/kubelet"  # 默认值
          fi
          echo $KUBELET_DIR > /shared/kubelet-dir
        volumeMounts:
        - name: proc
          mountPath: /host/proc
          readOnly: true
        - name: shared-config
          mountPath: /shared
      containers:
      - name: node-driver-registrar
        args:
        - --kubelet-registration-path=$(cat /shared/kubelet-dir)/plugins/ebs.csi.aws.com/csi.sock
        volumeMounts:
        - name: shared-config
          mountPath: /shared
      volumes:
      - name: proc
        hostPath:
          path: /proc
      - name: shared-config
        emptyDir: {}
```

#### 5.1.4 防护措施

**1. 部署前检查清单**：
```bash
#!/bin/bash
# 脚本：check-kubelet-paths.sh

echo "=== Checking kubelet root directories ==="
for node in $(kubectl get nodes -o name | cut -d/ -f2); do
  echo "Node: $node"
  kubectl debug node/$node -it --image=busybox:1.36 -- \
    chroot /host sh -c "ps aux | grep kubelet | grep -o -- '--root-dir=[^ ]*' || echo 'Using default: /var/lib/kubelet'"
done
```

**2. 监控告警**：
```yaml
# Prometheus 告警
- alert: CSIDriverNotRegisteredOnNode
  expr: kube_csinode_info{driver="ebs.csi.aws.com"} == 0
  for: 5m
  annotations:
    summary: "节点 {{ $labels.node }} 的 CSI 驱动未注册"
    runbook: "检查 kubelet 路径配置和 node-driver-registrar 日志"
```

**3. 自动修复 CronJob**：
```yaml
apiVersion: batch/v1
kind: CronJob
metadata:
  name: csi-registration-checker
  namespace: kube-system
spec:
  schedule: "*/10 * * * *"
  jobTemplate:
    spec:
      template:
        spec:
          serviceAccountName: csi-admin
          containers:
          - name: checker
            image: bitnami/kubectl:1.28
            command:
            - bash
            - -c
            - |
              # 查找驱动未注册的节点
              UNREGISTERED_NODES=$(kubectl get csinode -o json | jq -r '
                .items[] |
                select(.spec.drivers | map(select(.name == "ebs.csi.aws.com")) | length == 0) |
                .metadata.name
              ')
              
              if [ -n "$UNREGISTERED_NODES" ]; then
                for node in $UNREGISTERED_NODES; do
                  echo "Restarting CSI Node Pod on $node"
                  kubectl delete pod -n kube-system \
                    -l app=ebs-csi-node \
                    --field-selector spec.nodeName=$node
                done
              fi
          restartPolicy: OnFailure
```

**成果**：
- ✅ 故障节点从 30% 降至 0%
- ✅ 新节点池自动适配不同 kubelet 路径
- ✅ 平均修复时间从 35 分钟降至 5 分钟

---

### 5.2 案例二：CSI 驱动升级导致挂载点残留与 Pod 卡死

#### 5.2.1 故障现场

**背景**：
- 集群运行 AWS EBS CSI 驱动 v1.10.0
- 计划升级至 v1.15.0 以支持 GP3 卷的性能优化
- 集群规模：200 节点，3000+ PVC

**时间线**：
- **T+0min**：通过 Helm 执行滚动升级：`helm upgrade aws-ebs-csi-driver --set image.tag=v1.15.0`
- **T+10min**：升级完成，新版本 Pod 全部 Running
- **T+30min**：用户报告部分 StatefulSet 无法滚动更新，Pod 卡在 `Terminating` 状态超过 30 分钟
- **T+60min**：尝试强制删除 Pod（`kubectl delete --force --grace-period=0`）无效，Pod 仍然存在

**影响范围**：
- 35 个 StatefulSet（主要是数据库和消息队列）
- 约 150 个 Pod 卡死
- 业务影响：无法发布新版本，部分服务降级

#### 5.2.2 深度排查过程

**Step 1：检查 Pod 事件**
```bash
kubectl describe pod mysql-0 -n production

# Events:
# Normal   Killing  30m  kubelet  Stopping container mysql
# Warning  FailedUnmount  28m (x10 over 30m)  kubelet
#   UnmountVolume.TearDown failed: rpc error: code = Internal 
#   desc = Volume /var/lib/kubelet/pods/.../volumes/kubernetes.io~csi/pv-abc123/mount 
#   is still mounted on node node-5
```

**Step 2：检查节点挂载点**
```bash
# 在节点 node-5 上执行
mount | grep pv-abc123

# 输出：
# /dev/nvme1n1 on /var/lib/kubelet/plugins/kubernetes.io~csi/pv/pv-abc123/globalmount type ext4 (rw,relatime)
# /var/lib/kubelet/plugins/kubernetes.io~csi/pv/pv-abc123/globalmount on /var/lib/kubelet/pods/.../volumes/kubernetes.io~csi/pv-abc123 type none (rw,bind)
```

**Step 3：尝试手动卸载**
```bash
umount /var/lib/kubelet/pods/<uid>/volumes/kubernetes.io~csi/pv-abc123
# umount: /var/lib/kubelet/pods/.../volumes/kubernetes.io~csi/pv-abc123: device is busy.

# 查找占用进程
lsof +D /var/lib/kubelet/pods/<uid>/volumes/kubernetes.io~csi/pv-abc123
# COMMAND   PID  USER   FD   TYPE DEVICE SIZE/OFF NODE NAME
# (无输出，但仍然卸载失败)
```

**Step 4：检查 CSI Node Pod 日志**
```bash
kubectl logs -n kube-system csi-ebs-node-xxx -c ebs-plugin

# 错误日志：
# E1210 11:05:23 nodeserver.go:234] NodeUnpublishVolume failed: 
#   failed to unmount target "/var/lib/kubelet/pods/.../volumes/kubernetes.io~csi/pv-abc123": 
#   exit status 32: umount: /var/lib/kubelet/pods/.../volumes/kubernetes.io~csi/pv-abc123: not mounted

# ❌ 关键发现：新版本驱动认为挂载点"不存在"，但实际仍然挂载
```

**Step 5：对比新旧版本差异**
```bash
# 查看旧版本的挂载点格式
kubectl exec -n kube-system <old-csi-pod> -- mount | grep pv-abc123
# /dev/nvme1n1 on /var/lib/kubelet/plugins/kubernetes.io~csi/pv-abc123/globalmount

# 查看新版本的挂载点格式
kubectl exec -n kube-system <new-csi-pod> -- mount | grep pv-abc123
# /dev/nvme1n1 on /var/lib/kubelet/plugins/kubernetes.io~csi/pv/pv-abc123/globalmount
#                                                        ^^^  # ❌ 路径多了 "/pv/" 前缀
```

**根因分析**：
- v1.10 → v1.15 升级过程中，CSI 驱动修改了内部挂载路径格式
- 旧版本创建的挂载点路径：`.../pv-abc123/globalmount`
- 新版本期望的路径：`.../pv/pv-abc123/globalmount`
- 新版本驱动无法识别旧路径，导致卸载失败

#### 5.2.3 解决方案

**紧急止损**（手动清理，2 小时完成）：
```bash
#!/bin/bash
# 脚本：cleanup-legacy-mounts.sh

# 1. 获取所有卡住的 Pod 及其节点
kubectl get pods -A --field-selector status.phase=Terminating -o json | \
jq -r '.items[] | [.metadata.name, .metadata.namespace, .spec.nodeName] | @tsv' | \
while read pod ns node; do
  echo "Processing Pod: $pod in $ns on $node"
  
  # 2. 在节点上强制卸载
  kubectl debug node/$node -it --image=alpine:3.18 -- sh -c "
    chroot /host bash -c '
      # 查找该 Pod 的所有挂载点
      POD_UID=\$(kubectl get pod $pod -n $ns -o jsonpath=\"{.metadata.uid}\")
      MOUNT_POINTS=\$(mount | grep \$POD_UID | awk \"{print \\\$3}\")
      
      for mp in \$MOUNT_POINTS; do
        echo \"Unmounting: \$mp\"
        umount -f -l \$mp || true  # 强制 lazy umount
      done
      
      # 清理全局挂载点（旧版本格式）
      for gm in /var/lib/kubelet/plugins/kubernetes.io~csi/pv-*/globalmount; do
        if mountpoint -q \$gm; then
          echo \"Unmounting globalmount: \$gm\"
          umount -f -l \$gm || true
        fi
      done
    '
  "
  
  # 3. 删除 VolumeAttachment
  VA_NAME=\$(kubectl get volumeattachment -o json | jq -r \
    --arg pod \"$pod\" \
    '.items[] | select(.metadata.annotations[\"kubernetes.io/pod.name\"] == \$pod) | .metadata.name')
  if [ -n \"\$VA_NAME\" ]; then
    echo \"Deleting VolumeAttachment: \$VA_NAME\"
    kubectl delete volumeattachment \$VA_NAME --force --grace-period=0
  fi
  
  # 4. 强制删除 Pod
  kubectl delete pod $pod -n $ns --force --grace-period=0
done
```

**彻底修复**（回滚 + 重新升级，6 小时完成）：
```bash
# Step 1: 回滚到 v1.10.0
helm rollback aws-ebs-csi-driver

# Step 2: 等待所有 Pod 正常（验证旧版本驱动能正确卸载）
kubectl wait --for=condition=Ready pod -l app=ebs-csi-node -n kube-system --timeout=10m

# Step 3: 滚动重启所有使用 EBS 卷的 StatefulSet（触发卸载+重新挂载）
for sts in $(kubectl get sts -A -o json | jq -r '.items[] | "\(.metadata.namespace)/\(.metadata.name)"'); do
  echo "Restarting StatefulSet: $sts"
  ns=$(echo $sts | cut -d/ -f1)
  name=$(echo $sts | cut -d/ -f2)
  kubectl rollout restart statefulset/$name -n $ns
  kubectl rollout status statefulset/$name -n $ns --timeout=30m
done

# Step 4: 再次升级到 v1.15.0（此时所有挂载点都是干净的）
helm upgrade aws-ebs-csi-driver \
  --set image.tag=v1.15.0 \
  --set controller.podAnnotations."cluster-autoscaler\.kubernetes\.io/safe-to-evict"="false"
```

#### 5.2.4 防护措施

**1. 升级前完整测试**：
```bash
# 在测试集群验证升级路径
# 1. 部署相同版本的 CSI 驱动
# 2. 创建测试 StatefulSet
# 3. 模拟升级流程
# 4. 验证 Pod 能正常删除和重建
```

**2. 分批升级策略**：
```yaml
# DaemonSet 升级策略
apiVersion: apps/v1
kind: DaemonSet
metadata:
  name: csi-ebs-node
spec:
  updateStrategy:
    type: RollingUpdate
    rollingUpdate:
      maxUnavailable: 5  # 限制同时升级的节点数
  template:
    spec:
      containers:
      - name: ebs-plugin
        image: public.ecr.aws/ebs-csi-driver/aws-ebs-csi-driver:v1.15.0
```

**3. 自动兼容性检查**：
```bash
#!/bin/bash
# 升级前检查脚本：check-mount-compatibility.sh

echo "=== Checking for legacy mount points ==="
for node in $(kubectl get nodes -o name | cut -d/ -f2); do
  echo "Node: $node"
  kubectl debug node/$node -it --image=alpine:3.18 -- sh -c "
    chroot /host bash -c '
      # 查找旧版本格式的挂载点
      if mount | grep -q \"kubernetes.io~csi/pv-.*[^/pv]/globalmount\"; then
        echo \"❌ Found legacy mount points on $node\"
        exit 1
      else
        echo \"✅ No legacy mount points\"
      fi
    '
  "
done
```

**成果**：
- ✅ 所有 150 个卡死 Pod 恢复
- ✅ 建立 CSI 驱动升级 Runbook（包含回滚计划）
- ✅ 后续升级（v1.15 → v1.20）零故障

---

### 5.3 案例三：VolumeSnapshot 快照创建超时与后端不一致

#### 5.3.1 故障现场

**背景**：
- 使用 AWS EBS CSI 驱动的 Snapshot 功能进行定期备份
- CronJob 每天凌晨 2:00 创建所有生产数据库的快照

**时间线**：
- **T+0min**（02:00）：CronJob 触发，创建 50 个 `VolumeSnapshot` 对象
- **T+10min**：运维团队收到告警：45 个快照卡在 `Pending` 状态
- **T+30min**：手动删除并重建快照，仍然失败
- **T+60min**：查看 AWS 控制台，发现快照已在后端创建成功，但 K8s 对象显示 `ReadyToUse: false`

**影响**：
- 备份窗口超时（SLA 要求 30 分钟内完成）
- 部分快照在 K8s 中不可用，无法用于恢复测试

#### 5.2.2 深度排查过程

**Step 1：检查 VolumeSnapshot 状态**
```bash
kubectl get volumesnapshot -n production

# NAME                  READYTOUSE   SOURCEPVC   RESTORESIZE   SNAPSHOTCLASS   AGE
# mysql-snapshot-0210   False        data-mysql-0  100Gi       ebs-sc          35m

kubectl describe volumesnapshot mysql-snapshot-0210 -n production

# Status:
#   Bound Volume Snapshot Content Name:  snapcontent-abc123
#   Creation Time:  <unset>  # ❌ 创建时间未设置
#   Ready To Use:  false
# Events:
#   Warning  CreateSnapshotFailed  10m (x20 over 35m)  snapshot-controller
#     Failed to check and update snapshot content: rpc error: code = DeadlineExceeded 
#     desc = context deadline exceeded
```

**Step 2：检查 VolumeSnapshotContent**
```bash
kubectl get volumesnapshotcontent snapcontent-abc123 -o yaml

# spec:
#   deletionPolicy: Delete
#   driver: ebs.csi.aws.com
#   source:
#     volumeHandle: vol-0a1b2c3d4e5f6g7h8
#   volumeSnapshotRef:
#     name: mysql-snapshot-0210
#     namespace: production
# status:
#   snapshotHandle: ""  # ❌ 后端快照 ID 为空
#   readyToUse: false
#   creationTime: null
```

**Step 3：检查 CSI Snapshotter 日志**
```bash
kubectl logs -n kube-system <csi-controller> -c csi-snapshotter

# 错误日志：
# E1210 02:10:15 snapshot_controller.go:234] CreateSnapshot for content snapcontent-abc123 failed: 
#   rpc error: code = DeadlineExceeded desc = context deadline exceeded
# E1210 02:10:25 snapshot_controller.go:245] Retrying CreateSnapshot (attempt 5/5)
```

**Step 4：检查 CSI Driver 日志**
```bash
kubectl logs -n kube-system <csi-controller> -c ebs-plugin | grep CreateSnapshot

# 日志：
# I1210 02:05:15 controllerserver.go:345] CreateSnapshot: volume_id="vol-0a1b2c3d4e5f6g7h8"
# I1210 02:05:16 ec2_interface.go:123] AWS API call: CreateSnapshot
# I1210 02:06:45 ec2_interface.go:125] AWS API response: snapshot_id="snap-0x9y8z7w6v5u4t3"  # ✅ API 调用成功
# E1210 02:10:15 controllerserver.go:367] CreateSnapshot timeout: context deadline exceeded  # ❌ 但 gRPC 超时
```

**Step 5：验证 AWS 后端状态**
```bash
# 查询 AWS EBS 快照
aws ec2 describe-snapshots \
  --snapshot-ids snap-0x9y8z7w6v5u4t3 \
  --query 'Snapshots[0].[SnapshotId,State,Progress]' \
  --output text

# 输出：
# snap-0x9y8z7w6v5u4t3  completed  100%  # ✅ 快照已创建成功
```

**根因分析**：
- AWS `CreateSnapshot` API 是**异步**操作，返回快照 ID 后需等待快照状态变为 `completed`
- CSI 驱动的 `CreateSnapshot` 方法默认超时时间为 **5 分钟**
- 大容量卷（100GB+）的快照创建需要 **10-30 分钟**
- 超时后 gRPC 调用失败，但 AWS 快照已创建，导致 K8s 对象与后端不一致

#### 5.3.3 解决方案

**立即缓解**（手动同步后端状态，30 分钟）：
```bash
#!/bin/bash
# 脚本：sync-snapshot-status.sh

# 1. 获取所有卡住的 VolumeSnapshot
kubectl get volumesnapshot -A -o json | \
jq -r '.items[] | select(.status.readyToUse == false) | [.metadata.name, .metadata.namespace, .status.boundVolumeSnapshotContentName] | @tsv' | \
while read snap ns content; do
  echo "Processing VolumeSnapshot: $snap in $ns"
  
  # 2. 获取源 PV 的 volume_id
  PVC=$(kubectl get volumesnapshot $snap -n $ns -o jsonpath='{.spec.source.persistentVolumeClaimName}')
  VOLUME_ID=$(kubectl get pvc $PVC -n $ns -o jsonpath='{.spec.volumeName}' | \
    xargs kubectl get pv -o jsonpath='{.spec.csi.volumeHandle}')
  
  # 3. 查询 AWS 后端是否有对应的快照
  SNAPSHOT_ID=$(aws ec2 describe-snapshots \
    --filters "Name=volume-id,Values=$VOLUME_ID" "Name=start-time,Values=$(date -u -d '2 hours ago' +%Y-%m-%dT%H:%M:%S)" \
    --query 'Snapshots | sort_by(@, &StartTime) | [-1].SnapshotId' \
    --output text)
  
  if [ "$SNAPSHOT_ID" != "None" ]; then
    echo "Found backend snapshot: $SNAPSHOT_ID"
    
    # 4. 手动更新 VolumeSnapshotContent
    kubectl patch volumesnapshotcontent $content --type=json -p="[
      {\"op\": \"replace\", \"path\": \"/status/snapshotHandle\", \"value\": \"$SNAPSHOT_ID\"},
      {\"op\": \"replace\", \"path\": \"/status/readyToUse\", \"value\": true},
      {\"op\": \"replace\", \"path\": \"/status/creationTime\", \"value\": $(date -u +%s)000000000}
    ]"
    
    echo "✅ Synced snapshot $snap"
  else
    echo "❌ No backend snapshot found for $snap, needs manual investigation"
  fi
done
```

**长期优化**（增加超时时间，配置异步等待）：
```yaml
# 方案 1：增加 CSI Snapshotter 的超时时间
apiVersion: apps/v1
kind: Deployment
metadata:
  name: csi-controller
  namespace: kube-system
spec:
  template:
    spec:
      containers:
      - name: csi-snapshotter
        args:
        - --timeout=600s  # 从 300s 增加到 600s (10 分钟)
        - --retry-interval-start=10s
        - --retry-interval-max=5m
```

```yaml
# 方案 2：配置 CSI 驱动使用异步模式
apiVersion: v1
kind: ConfigMap
metadata:
  name: ebs-csi-driver-config
  namespace: kube-system
data:
  snapshot-config.yaml: |
    snapshotting:
      asyncMode: true  # 启用异步模式
      pollingInterval: 30s  # 每 30 秒检查一次快照状态
      maxWaitTime: 1800s  # 最多等待 30 分钟
```

#### 5.3.4 防护措施

**1. 监控快照创建延迟**：
```yaml
# Prometheus 告警
- alert: VolumeSnapshotCreationSlow
  expr: (time() - kube_volumesnapshot_created{ready_to_use="false"}) > 600
  for: 5m
  annotations:
    summary: "VolumeSnapshot {{ $labels.volumesnapshot }} 创建超过 10 分钟"
    description: "检查 CSI Driver 日志和 AWS 快照状态"

- alert: VolumeSnapshotBackendMismatch
  expr: |
    kube_volumesnapshotcontent_info{snapshot_handle=""} == 1
    and
    (time() - kube_volumesnapshotcontent_created) > 300
  for: 5m
  annotations:
    summary: "VolumeSnapshotContent {{ $labels.volumesnapshotcontent }} 后端 ID 为空"
    description: "可能存在 K8s 与云平台状态不一致"
```

**2. 自动同步 CronJob**：
```yaml
apiVersion: batch/v1
kind: CronJob
metadata:
  name: snapshot-sync
  namespace: kube-system
spec:
  schedule: "*/15 * * * *"  # 每 15 分钟检查一次
  jobTemplate:
    spec:
      template:
        spec:
          serviceAccountName: snapshot-admin
          containers:
          - name: syncer
            image: amazon/aws-cli:2.13.0
            command:
            - bash
            - -c
            - |
              # 同步逻辑（同上面的脚本）
              kubectl get volumesnapshot -A -o json | jq -r ...
          restartPolicy: OnFailure
```

**3. 快照分批创建**：
```yaml
apiVersion: batch/v1
kind: CronJob
metadata:
  name: backup-batch-1
spec:
  schedule: "0 2 * * *"  # 02:00 - 第一批
  jobTemplate:
    spec:
      template:
        spec:
          containers:
          - name: snapshot-creator
            image: bitnami/kubectl:1.28
            command:
            - bash
            - -c
            - |
              # 只创建 10 个快照
              for pvc in $(kubectl get pvc -n production -l backup-batch=1 -o name | head -10); do
                kubectl create -f - <<EOF
              apiVersion: snapshot.storage.k8s.io/v1
              kind: VolumeSnapshot
              metadata:
                name: $(basename $pvc)-$(date +%Y%m%d)
                namespace: production
              spec:
                source:
                  persistentVolumeClaimName: $(basename $pvc)
                volumeSnapshotClassName: ebs-sc
              EOF
                sleep 60  # 每个快照间隔 1 分钟
              done
---
apiVersion: batch/v1
kind: CronJob
metadata:
  name: backup-batch-2
spec:
  schedule: "30 2 * * *"  # 02:30 - 第二批
  # 配置同上
```

**成果**：
- ✅ 快照创建成功率从 10% 提升至 98%
- ✅ K8s 对象与 AWS 后端状态一致性达到 100%
- ✅ 备份窗口从 60 分钟缩短至 35 分钟

---

## 附录：CSI 专家巡检表

### A. 控制面健康检查

- [ ] **CSI Controller 副本数**：是否满足高可用要求（建议 ≥2）？
  ```bash
  kubectl get deployment -n kube-system -l app=csi-controller -o wide
  ```

- [ ] **Leader Election 状态**：是否有且仅有一个 Leader 持有锁？
  ```bash
  kubectl get lease -n kube-system | grep csi
  ```

- [ ] **Sidecar 容器健康**：是否所有 Sidecar（provisioner/attacher/resizer/snapshotter）都在运行？
  ```bash
  kubectl get pods -n kube-system -l app=csi-controller \
    -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{range .status.containerStatuses[*]}{.name}={.ready}{" "}{end}{"\n"}{end}'
  ```

- [ ] **RBAC 权限完整**：ClusterRole 是否具备操作 `VolumeAttachment`、`PV`、`PVC`、`Lease` 的权限？
  ```bash
  kubectl describe clusterrole <csi-controller-role> | grep -A10 "Resources:"
  ```

- [ ] **云平台凭证有效**：CSI 驱动的 IAM/ServiceAccount 权限是否未过期？
  ```bash
  # AWS 示例
  kubectl exec -n kube-system <csi-controller> -c ebs-plugin -- \
    aws sts get-caller-identity
  ```

---

### B. 节点侧健康检查

- [ ] **CSI Node DaemonSet 覆盖率**：是否所有节点都运行了 CSI Node Pod？
  ```bash
  # 预期：节点数 = Pod 数
  echo "Nodes: $(kubectl get nodes --no-headers | wc -l)"
  echo "CSI Pods: $(kubectl get pods -n kube-system -l app=csi-node --no-headers | wc -l)"
  ```

- [ ] **驱动注册状态**：所有节点的 `CSINode` 对象是否包含预期的驱动条目？
  ```bash
  kubectl get csinode -o custom-columns=\
  NAME:.metadata.name,\
  DRIVERS:.spec.drivers[*].name | grep -v "<driver-name>"
  # 输出为空则全部注册成功
  ```

- [ ] **Socket 文件权限**：Socket 目录是否具备正确的 SELinux/AppArmor 上下文？
  ```bash
  # 在节点上执行
  ls -laZ /var/lib/kubelet/plugins/*/csi.sock
  # 预期权限：0660，上下文包含 svirt_sandbox_file_t（SELinux）
  ```

- [ ] **kubelet 根目录一致性**：DaemonSet 的 `hostPath` 是否与节点的 kubelet 根目录匹配？
  ```bash
  ps aux | grep kubelet | grep -o -- '--root-dir=[^ ]*'
  ```

- [ ] **Mount 传播设置**：kubelet-dir 挂载是否配置了 `mountPropagation: Bidirectional`？
  ```bash
  kubectl get daemonset -n kube-system csi-node -o yaml | grep -A5 mountPropagation
  ```

---

### C. 存储对象健康检查

- [ ] **StorageClass 配置**：是否正确设置了 `volumeBindingMode` 和 `allowVolumeExpansion`？
  ```bash
  kubectl get storageclass -o custom-columns=\
  NAME:.metadata.name,\
  PROVISIONER:.provisioner,\
  BINDING:.volumeBindingMode,\
  EXPANSION:.allowVolumeExpansion
  ```

- [ ] **PV 回收策略**：是否符合业务需求（`Delete` vs `Retain`）？
  ```bash
  kubectl get pv -o custom-columns=\
  NAME:.metadata.name,\
  POLICY:.spec.persistentVolumeReclaimPolicy,\
  STATUS:.status.phase | grep -v "Retain\|Delete"
  ```

- [ ] **VolumeAttachment 堆积**：是否有长时间未附着的附件（可能是节点下线导致）？
  ```bash
  kubectl get volumeattachment -o json | jq -r '
    .items[] |
    select(.status.attached == false) |
    select((now - (.metadata.creationTimestamp | fromdateiso8601)) > 600) |
    [.metadata.name, .spec.nodeName, (now - (.metadata.creationTimestamp | fromdateiso8601))] |
    @tsv
  ' | awk '{printf "%-50s %-20s %d seconds\n", $1, $2, $3}'
  ```

- [ ] **CSIDriver Capability 声明**：是否正确声明了 Attach、扩容、快照等能力？
  ```bash
  kubectl get csidriver <driver-name> -o yaml | grep -A10 spec:
  ```

---

### D. 性能与容量检查

- [ ] **操作延迟监控**：CreateVolume/Attach/Mount 的 P99 延迟是否在合理范围（<60s）？
  ```promql
  histogram_quantile(0.99, rate(csi_sidecar_operations_seconds_bucket[5m]))
  ```

- [ ] **错误率监控**：CSI 操作的失败率是否低于 5%？
  ```promql
  rate(csi_sidecar_operations_seconds_count{grpc_status_code!="OK"}[5m])
  /
  rate(csi_sidecar_operations_seconds_count[5m])
  ```

- [ ] **存储后端配额**：是否有剩余配额（如云平台的卷数量限制）？
  ```bash
  # AWS 示例
  aws service-quotas get-service-quota \
    --service-code ec2 \
    --quota-code L-D18FCD1D \
    --query 'Quota.[Value,UsageMetric.MetricDimensions]'
  ```

- [ ] **PVC 容量使用率**：是否有 PVC 使用率超过 80%（需自动扩容）？
  ```bash
  kubectl get pvc -A -o json | jq -r '
    .items[] |
    select(.status.capacity.storage != null) |
    [.metadata.namespace, .metadata.name, .status.capacity.storage] |
    @tsv
  ' | while read ns name cap; do
    # 需结合 Prometheus 指标计算使用率
    echo "$ns/$name: $cap"
  done
  ```

---

### E. 安全与合规检查

- [ ] **敏感信息保护**：云平台凭证是否使用 Secret 存储（而非明文环境变量）？
  ```bash
  kubectl get deployment -n kube-system csi-controller -o yaml | grep -A5 "env:" | grep -i "secret"
  ```

- [ ] **最小权限原则**：ServiceAccount 是否仅授予必要的 RBAC 权限？
  ```bash
  kubectl describe clusterrolebinding <csi-binding> | grep "Role:" -A10
  ```

- [ ] **加密卷支持**：StorageClass 是否启用了静态加密（如 AWS EBS 加密）？
  ```yaml
  # 检查 StorageClass 参数
  kubectl get storageclass <name> -o yaml | grep encrypted
  ```

- [ ] **审计日志记录**：是否记录了所有 CSI 操作的审计日志？
  ```bash
  # 检查 API Server 审计策略
  kubectl -n kube-system get configmap audit-policy -o yaml | grep "storage.k8s.io"
  ```

---

### F. 故障演练清单

- [ ] **模拟 CSI Controller 故障**：删除 Controller Pod，验证 Leader Election 切换时间（<30s）
- [ ] **模拟 CSI Node 故障**：停止节点上的 CSI Node Pod，验证新 Pod 是否无法挂载卷
- [ ] **模拟存储后端故障**：人为触发云平台 API 限流，验证指数退避重试机制
- [ ] **模拟节点下线**：Drain 节点并删除，验证 VolumeAttachment 是否自动清理
- [ ] **模拟并发创建压力**：批量创建 1000 个 PVC，验证 Controller 是否能稳定处理
- [ ] **模拟快照故障**：创建快照时中断网络，验证是否能正确回滚或重试

---

### G. 版本兼容性检查

| 组件 | 推荐版本 | 兼容性说明 |
|:----|:--------|:----------|
| Kubernetes | 1.25 - 1.32 | CSI v1.8+ 需要 K8s 1.25+ |
| CSI Sidecar (provisioner) | v3.5+ | 支持 Topology 和容量跟踪 |
| CSI Sidecar (attacher) | v4.3+ | 支持多附着检测 |
| CSI Sidecar (resizer) | v1.8+ | 支持在线扩容 |
| CSI Sidecar (snapshotter) | v6.0+ | 支持 Snapshot v1 API |
| node-driver-registrar | v2.9+ | 支持 Kubelet 插件监控 |

**版本检查脚本**：
```bash
#!/bin/bash
echo "=== CSI Sidecar Versions ==="
kubectl get pods -n kube-system -l app=csi-controller -o json | \
jq -r '.items[0].spec.containers[] | select(.name | startswith("csi-")) | [.name, .image] | @tsv' | \
while read name image; do
  version=$(echo $image | grep -oP '(?<=:)v[0-9]+\.[0-9]+\.[0-9]+')
  echo "$name: $version"
done
```

---

### H. 常用诊断命令速查

```bash
# 1. 快速查看 CSI 系统状态
kubectl get csidriver,csinode,csistoragecapacity,volumeattachment -A

# 2. 查看特定 PVC 的完整生命周期
kubectl get events --sort-by='.lastTimestamp' --field-selector involvedObject.name=<pvc-name>

# 3. 查找占用卷的所有 Pod
PV_NAME=<pv-name>
kubectl get pods -A -o json | jq -r \
  --arg pv "$PV_NAME" \
  '.items[] | select(.spec.volumes[]?.persistentVolumeClaim.claimName != null) | 
  select((.metadata.namespace + "/" + .spec.volumes[].persistentVolumeClaim.claimName) as $pvc | 
  (["kubectl get pvc \($pvc) -o jsonpath={.spec.volumeName}"] | @sh | 
  gsub("\\$\\("; "(") | gsub("\\)"; ")")) | . as $cmd | 
  $cmd == $pv) | 
  [.metadata.namespace, .metadata.name, .spec.nodeName] | @tsv'

# 4. 批量清理 Failed 状态的 PVC
kubectl get pvc -A -o json | jq -r \
  '.items[] | select(.status.phase == "Failed") | 
  ["kubectl delete pvc", .metadata.name, "-n", .metadata.namespace] | @sh'

# 5. 导出 CSI 驱动配置快照
kubectl get csidriver,storageclass,volumesnapshotclass -o yaml > csi-config-backup.yaml

# 6. 监控 CSI 操作实时日志
kubectl logs -n kube-system -l app=csi-controller --all-containers=true -f | \
  grep -E "CreateVolume|DeleteVolume|ControllerPublish|ControllerUnpublish"
```

---

### I. 参考资源

- **CSI Spec**: https://github.com/container-storage-interface/spec
- **Kubernetes CSI 文档**: https://kubernetes-csi.github.io/docs/
- **CSI Sidecar 仓库**: https://github.com/kubernetes-csi
- **常见 CSI 驱动**:
  - AWS EBS: https://github.com/kubernetes-sigs/aws-ebs-csi-driver
  - Azure Disk: https://github.com/kubernetes-sigs/azuredisk-csi-driver
  - GCP PD: https://github.com/kubernetes-sigs/gcp-compute-persistent-disk-csi-driver
  - Ceph RBD: https://github.com/ceph/ceph-csi
- **故障排查工具**:
  - csc: https://github.com/rexray/gocsi/tree/master/csc
  - grpcurl: https://github.com/fullstorydev/grpcurl

---

**文档维护**：
- 最后更新：2026-02
- 维护者：Kubernetes 存储专家组
- 反馈渠道：GitHub Issues / 内部 Wiki

---

**关键词**：CSI, Container Storage Interface, gRPC, VolumeAttachment, Socket, Sidecar, Provisioner, Attacher, Snapshot, Kubernetes 存储, 故障排查
