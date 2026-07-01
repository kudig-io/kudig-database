---
title: KubeVirt：在 Kubernetes 上运行虚拟机
description: '## 概述'
category: dictionary
tags:
- k8s
- glossary
- terminology
- cilium
- harbor
- ceph
- mysql
- postgresql
- daemonset
- job
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- KubeVirt：在 Kubernetes 上运行虚拟机 是什么
- 如何 KubeVirt：在 Kubernetes 上运行虚拟机
trigger_keywords:
- KubeVirt：在
- Kubernetes
- 上运行虚拟机
- dictionary
prerequisites:
- kubectl-basics
- cloud-provider-basics
- cilium-basics
- mysql-basics
- gpu-scheduling-basics
- backup-basics
created: "2026-05-23"
created: 2026-05
---

# [[KubeVirt|KubeVirt]]：在 [[Kubernetes|Kubernetes]] 上运行虚拟机

## 概述

**KubeVirt** 是 CNCF 孵化项目，允许在 Kubernetes 集群中像管理 Pod 一样管理虚拟机（VM）。随着企业从 VMware 等传统虚拟化平台向云原生迁移，以及 AI/ML、数据库等有状态工作负载在 Kubernetes 上的成熟，KubeVirt 在 2025–2026 年迅速成为混合负载平台的核心技术。它实现了容器与虚拟机在同一控制平面下的统一编排。

## 核心概念/原理

### 1. KubeVirt 架构

KubeVirt 通过 Kubernetes Operator 在集群中部署以下核心组件：
- **virt-api**：提供 KubeVirt 自定义资源的 API 服务
- **virt-controller**：负责 VM 生命周期管理（创建、调度、扩缩容）
- **virt-handler**：运行在每个节点上的 [[DaemonSet|DaemonSet]]，管理本节点 VM 的启动、停止和监控
- **virt-launcher**：每个 VM 对应一个 Pod，Pod 中运行 libvirt/QEMU 进程

### 2. 核心 CRD

| CRD | 作用 | 类比 |
|-----|------|------|
| **VirtualMachine（VM）** | 定义虚拟机的配置模板 | Deployment |
| **VirtualMachineInstance（VMI）** | VM 的运行时实例 | Pod |
| **VirtualMachinePool** | 管理一组相同的 VM | [[ReplicaSet|ReplicaSet]] |
| **DataVolume** | 管理 VM 磁盘镜像的创建和导入 | PVC + Job |

### 3. VM 与容器的统一编排

KubeVirt VM 实际上是以特殊 Pod 形式运行在 Kubernetes 节点上的：
- 该 Pod 运行 `virt-launcher` 容器
- `virt-launcher` 内部通过 QEMU/KVM 启动真正的虚拟机
- VM 可以使用与容器相同的 CNI 网络、StorageClass 存储和 Namespace 隔离

```yaml
apiVersion: kubevirt.io/v1
kind: VirtualMachine
metadata:
  name: windows-vm
spec:
  running: true
  template:
    spec:
      domain:
        cpu:
          cores: 4
        resources:
          requests:
            memory: 8Gi
        devices:
          disks:
          - name: system-disk
            disk:
              bus: virtio
      volumes:
      - name: system-disk
        persistentVolumeClaim:
          claimName: windows-pvc
```

## 关键机制或特性

### 实时迁移（Live Migration）

KubeVirt 支持在不影响业务的情况下将运行中的 VM 从一个节点迁移到另一个节点：
- 用于节点维护、负载均衡或故障转移
- 需要共享存储（如 NFS、Ceph RBD）
- 网络身份（MAC/IP）在迁移过程中保持不变

### 网络与存储集成

- **网络**：VM 使用与容器相同的 CNI 插件，可直接获得 Cluster IP、支持 NetworkPolicy
- **存储**：通过 DataVolume 从 HTTP/S3/Registry 导入镜像，或直接挂载 PVC
- **GPU 透传**：支持将物理 GPU 直接分配给 VM（PCI Passthrough），适用于 Windows AI 工作负载

### 与容器共存

同一 Namespace 中可同时运行容器 Pod 和 KubeVirt VM：
- 容器负责现代化微服务
- VM 负责遗留应用、Windows 工作负载或需要完整 OS 内核隔离的场景
- 通过 Service/Ingress 统一暴露服务

## 使用场景

1. **VMware 替代与迁移**：企业将传统的 vSphere VM 迁移到 Kubernetes 平台，统一基础设施管理
2. **遗留应用容器化过渡期**：无法直接容器化的老系统先以 VM 形式运行在 [[entities/kubernetes.md|k8s]] 上，逐步重构
3. **Windows 工作负载**：Windows 容器功能有限，通过 KubeVirt 运行完整 Windows VM 运行 .NET Framework 应用
4. **数据库与有状态服务**：MySQL、PostgreSQL、Oracle 等需要强磁盘 I/O 一致性的数据库先在 VM 中运行
5. **AI 训练 VM**：为数据科学家提供带 GPU 透传的 Ubuntu/Windows VM，既保留 K8s 资源调度能力，又提供完整 OS 环境

## 最佳实践/注意事项

- **网络身份稳定性**：VM 迁移时 IP/MAC 必须保持不变，因此需要使用支持二层网络的 CNI（如 OVN、Cilium）
- **存储必须共享**：Live Migration 要求 VM 磁盘存储在所有节点可访问，推荐使用 Ceph RBD 或 NFS
- **节点必须支持 KVM**：运行 KubeVirt 的节点 BIOS 中必须启用虚拟化支持（Intel VT-x / AMD-V）
- **资源预留**：为 QEMU 进程和 virt-launcher 预留足够的 CPU 和内存开销
- **VM 镜像管理**：建立内部的 VM 镜像仓库（如 Harbor），避免每次从互联网下载大镜像
- **备份策略**：使用 Velero 或专门的 VM 备份工具（如 Kasten）定期备份 VM 磁盘和配置
- **监控扩展**：除了 Kubernetes 原生监控，还需在 VM 内部署 Guest Agent 以获取 OS 级指标
- **多租户隔离**：通过 Namespace + ResourceQuota 隔离不同团队的 VM 资源，防止" noisy neighbor "

## 故障排查

| 症状 | 可能原因 | 排查命令/方法 |
|------|---------|-------------|
| VM 启动失败（virt-launcher CrashLoopBackOff） | 节点不支持 KVM 或 BIOS 未启用虚拟化 | `kubectl get vmi <name> -o yaml` 查看 conditions；`cat /dev/kvm` 检查 KVM |
| VM 无法分配 IP | CNI 插件不支持 VM 或 Multus 配置错误 | `kubectl get vmi <name> -o jsonpath='{.status.interfaces}'` |
| Live Migration 失败 | 存储非共享或节点资源不足 | `kubectl get vmim <name> -o yaml`；确保 PVC 使用 RWX 存储 |
| VM 磁盘 I/O 极慢 | 存储后端性能不足或未使用 virtio 驱动 | 检查 disk bus 配置（应为 `virtio`）；检查 StorageClass IOPS 限制 |
| DataVolume Import 卡住 | 源镜像不可达或 CDI 控制器异常 | `kubectl get dv <name>`；`kubectl -n cdi logs -l app=cdi-deployment` |
| GPU 透传不生效 | IOMMU 未启用或设备未绑定 vfio-pci | 节点检查 `dmesg \| grep -i iommu`；`ls /dev/vfio/` |
| VM 内存被杀 | 宿主机 memory overcommit 导致 OOM | 确保 VM requests 与 limits 一致；检查节点 memory pressure |
| Guest Agent 无数据 | VM 内未安装 qemu-guest-agent | 进入 VM 检查 `systemctl status qemu-guest-agent` |

## 生产检查清单

- [ ] 所有 KubeVirt 节点 BIOS 已启用虚拟化（Intel VT-x / AMD-V）
- [ ] `/dev/kvm` 设备在所有 KubeVirt 节点可用
- [ ] VM 使用 virtio 磁盘和网络驱动以获得最佳性能
- [ ] Live Migration 场景使用 RWX 共享存储（Ceph RBD、NFS）
- [ ] CNI 支持二层网络保持 VM 迁移时 IP/MAC 不变（OVN、Cilium）
- [ ] VM 的 CPU 和内存 requests 与 limits 一致（避免宿主机 overcommit）
- [ ] QEMU 进程和 virt-launcher 的资源开销已预留
- [ ] VM 内部署了 qemu-guest-agent 以获取 OS 级指标
- [ ] VM 磁盘镜像使用内部镜像仓库（如 Harbor），避免外部下载
- [ ] Velero / Kasten 备份策略覆盖 VM 磁盘和配置
- [ ] 多租户通过 Namespace + ResourceQuota 隔离 VM 资源
- [ ] GPU 透传场景已启用 IOMMU 和 vfio-pci 驱动绑定

## 命令快速参考

```bash
# 查看所有 VirtualMachine
kubectl get vm -A

# 查看运行中的 VirtualMachineInstance
kubectl get vmi -A

# 查看 VM 详情
kubectl describe vm <name>

# 启动 VM
virtctl start <vm-name>

# 停止 VM
virtctl stop <vm-name>

# 重启 VM
virtctl restart <vm-name>

# 通过 console 连接 VM
virtctl console <vm-name>

# 通过 VNC 连接 VM
virtctl vnc <vm-name>

# 触发 Live Migration
virtctl migrate <vm-name>

# 查看迁移状态
kubectl get vmim -A

# 查看 DataVolume 状态（磁盘导入进度）
kubectl get dv -A

# 检查节点 KVM 支持
kubectl get nodes -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{.status.allocatable.devices\.kubevirt\.io/kvm}{"\n"}{end}'

# 查看 KubeVirt 组件状态
kubectl get pods -n kubevirt

# 查看 CDI（Containerized Data Importer）组件状态
kubectl get pods -n cdi
```

## 交叉引用

- [device-plugins.md](./device-plugins.md) — GPU 透传和 Device Plugin 机制
- [compute-storage-and-networking-extensions.md](./compute-storage-and-networking-extensions.md) — 计算/存储/网络扩展
- [custom-resources.md](./custom-resources.md) — KubeVirt CRD 体系
- [operator-pattern.md](./operator-pattern.md) — KubeVirt Operator 部署模式
- [../storage/persistent-volumes.md](../storage/persistent-volumes.md) — VM 磁盘的 PV/PVC 管理
- [../networking/service.md](../networking/service.md) — VM 的 Service 暴露

## 参考链接

- [KubeVirt Official Documentation](https://kubevirt.io/user-guide/)
- [KubeVirt GitHub Repository](https://github.com/kubevirt/kubevirt)
- [VMblog - 2026 Kubernetes and KubeVirt Predictions](https://vmblog.com/prediction/2026-kubernetes-and-cilium-networking-predictions/)
- [Loginline - Kubernetes Trends 2026](https://www.loginline.com/en/blog/2026-kubernetes-trends)

## Related

- [[domain-17-system-foundation/topic-dictionary/platform-engineering/admission-webhook-good-practices.md|Admission Webhook 最佳实践]]
- [[domain-17-system-foundation/topic-dictionary/platform-engineering/api-group.md|API 组]]
- [[domain-17-system-foundation/topic-dictionary/platform-engineering/api-priority-and-fairness.md|API 优先级与公平性（API Priority and Fairness）]]
