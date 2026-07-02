---
title: CSI / CNI 版本兼容矩阵
description: '| **Ceph CSI (RBD)** v3.6+ | 1.22 | 1.26-1.33 | 1.33 | Rook 部署 |'
summary: '| **Ceph CSI (RBD)** v3.6+ | 1.22 | 1.26-1.33 | 1.33 | Rook 部署 |'
category: networking
tags:
- k8s
- networking
- service
- ingress
- cni
- istio
- cilium
- flannel
- calico
- rook
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 网络工程师
- 运维工程师
estimated_read_time: 10min
intent_queries:
- CSI / CNI 版本兼容矩阵 是什么
- 如何 CSI / CNI 版本兼容矩阵
- Kubernetes 5 networking 最佳实践
trigger_keywords:
- CSI
- CNI
- 版本兼容矩阵
- networking
prerequisites:
- kubectl-basics
- networking-basics
- service-mesh-basics
- ebpf-basics
- cilium-basics
- cni-basics
- redis-basics
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
authors:
- name: KUDIG Team
  role: contributor
cross_refs:
- type: domain
  path: ../domain-01-cluster-fundamentals/
  label: '相关知识域: domain-01-cluster-fundamentals'
- type: domain
  path: ../domain-03-networking-traffic/
  label: '相关知识域: domain-03-networking-traffic'
- type: domain
  path: ../domain-06-observability/
  label: '相关知识域: domain-06-observability'
- type: fta
  path: ../domain-10-troubleshooting-diagnostics/topic-fta/list/csi-fta.md
  label: '故障树: csi'
- type: cheatsheet
  path: ../domain-17-system-foundation/topic-cheat-sheet/networking.md
  label: '速查卡: networking'
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# CSI / CNI 版本兼容矩阵

> **文档类型**: 版本对照手册 | **适用版本**: K8s 1.28-1.33 | **最后更新**: 2026-05
> **使用场景**: Agent 判断"某 CSI/CNI 版本是否与当前 K8s 版本兼容"、"升级时应注意哪些兼容性问题"

---

<!-- chunk: 1. CSI (Container Storage Interface) 版本对照 -->
## 1. CSI (Container Storage Interface) 版本对照

### 1.1 CSI Spec 版本与 K8s 版本对应关系

| CSI Spec 版本 | K8s 支持状态 | K8s 版本 | 说明 |
|---------------|-------------|---------|------|
| CSI Spec v1.0.0 | **GA (Stable)** | 1.26+ | 完整 GA，所有生产可用 |
| CSI Spec v1beta1 | **DEPRECATED** | 1.26+ | 1.27 开始废弃，1.28 仍支持但不推荐 |
| CSI Spec v1.0.0 (保留) | **GA** | 所有版本 | 最终稳定版本 |

### 1.2 主流 CSI Driver 版本兼容性

| CSI Driver | 最低 K8s 版本 | 推荐 K8s 版本 | 最高测试版本 | 说明 |
|-----------|--------------|--------------|-------------|------|
| **in-tree (已移除)** | - | - | 1.26 | 树内驱动 1.26 开始被移除 |
| **AWS EBS CSI** v1.20+ | 1.20 | 1.26-1.33 | 1.33 | 生产稳定 |
| **GCE PD CSI** v1.10+ | 1.20 | 1.26-1.33 | 1.33 | GKE 原生支持 |
| **Azure Disk CSI** v1.20+ | 1.20 | 1.26-1.33 | 1.33 | AKS 原生支持 |
| **阿里云 Cloud Disk CSI** v1.0+ | 1.21 | 1.28-1.33 | 1.33 | ACK 原生支持 |
| **Ceph CSI (RBD)** v3.6+ | 1.22 | 1.26-1.33 | 1.33 | [[Rook|Rook]] 部署 |
| **Ceph CSI (CephFS)** v3.6+ | 1.22 | 1.26-1.33 | 1.33 | Rook 部署 |
| **NFS CSI** v4.0+ | 1.20 | 1.26-1.33 | 1.33 | 需要外部Provisioner |
| **[[Longhorn|Longhorn]] CSI** v1.5+ | 1.21 | 1.26-1.33 | 1.33 | CNCF 项目 |
| **TopoLVM CSI** v0.10+ | 1.21 | 1.28-1.33 | 1.33 | 本地 LVM 支持 |
| **OpenEBS CSI** v3.0+ | 1.22 | 1.26-1.33 | 1.33 | CNCF 项目 |

### 1.3 CSI 特性门控 (FeatureGate) 对照

| FeatureGate | K8s 版本 | CSI Spec | 说明 |
|-------------|---------|----------|------|
| `CSIInlineVolume` | GA | v1.0+ | Pod spec 内联 volumes（ephemeral） |
| `CSIVolumeGP3` | GA | v1.0+ | AWS EBS gp3 volume 类型 |
| `CSIStorageCapacity` | GA | v1.0+ | CSI 存储容量发现 |
| `GenericEphemeralVolume` | GA | v1.0+ | 通用临时卷（不只是 CSI） |
| `CSIMigration` | GA | v1.0+ | 树内驱动迁移到 CSI |
| `CSIMigrationAWS` | GA | v1.0+ | AWS 树内迁移 |
| `CSIMigrationGCE` | GA | v1.0+ | GCE 树内迁移 |
| `CSIMigrationAzureDisk` | GA | v1.0+ | Azure 树内迁移 |
| `CSIMigrationvSphere` | Beta | v1.0+ | vSphere 树内迁移（1.30+） |
| `CSIMigrationPortworx` | Alpha | v1.0+ | Portworx 迁移 |
| `CSIStartVolumeExpandController` | GA | v1.0+ | 卷扩展控制器 |

### 1.4 CSI 卷扩展注意事项

```yaml
# StorageClass 必须开启 allowVolumeExpansion: true
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: standard
provisioner: pd.csi.storage.gke.io
allowVolumeExpansion: true
parameters:
  type: pd-standard
volumeBindingMode: WaitForFirstConsumer
allowVolumeExpansion: "true"  # 字符串 "true"
```

### 1.5 树内驱动 → CSI 迁移状态（K8s 1.28+）

| 存储类型 | 迁移完成版本 | 说明 |
|---------|------------|------|
| AWS EBS | **GA (1.29+)** | in-tree 已移除 1.28 |
| GCE PD | **GA (1.29+)** | in-tree 已移除 1.28 |
| Azure Disk | **GA (1.30+)** | in-tree 1.28 开始废弃 |
| Azure File | **Beta (1.30+)** | 仍在迁移中 |
| vSphere | **Beta (1.30+)** | 需要 vSphere 7.0u3+ |
| Ceph RBD | Alpha | 进行中 |
| Ceph FS | Alpha | 进行中 |

---

<!-- chunk: 2. CNI (Container Network Interface) 版本对照 -->
## 2. CNI (Container Network Interface) 版本对照

### 2.1 CNI Spec 版本与 K8s 版本对应关系

| CNI Spec 版本 | K8s 支持状态 | 说明 |
|---------------|-------------|------|
| CNI Spec v0.4.0 | GA | 经典 CNI 版本 |
| CNI Spec v1.0.0 | GA (K8s 1.25+) | 最新的稳定 CNI 标准 |
| CNI Spec v1.1.0 | GA (K8s 1.27+) | 支持设备插件和 IPAM 扩展 |

### 2.2 主流 CNI 插件版本兼容性

| CNI 插件 | 最低 K8s 版本 | 推荐版本 | 最高测试版本 | 特殊要求 |
|---------|--------------|---------|-------------|---------|
| **Flannel** v0.22+ | 1.26 | 0.22.x | 1.33 | backend: vxlan/host-gw/udp |
| **Calico** v3.24+ | 1.28 | 3.24.x-3.28.x | 1.33 | BGP/IXP/WireGuard 支持 |
| **Calico with eBPF** | 1.28 | 3.27.x+ | 1.33 | 需要内核 5.10+ |
| **Cilium** v1.14+ | 1.28 | 1.14.x-1.16.x | 1.33 | eBPF 全功能需要内核 5.10+ |
| **Weave Net** v2.8+ | 1.26 | 2.8.x | 1.33 | 简单部署场景 |
| **Multus** v3.9+ | 1.26 | 3.9.x-4.0.x | 1.33 | 需要配合其他 CNI |
| **Kube-router** v1.5+ | 1.26 | 1.5.x | 1.33 | 全功能（LB/FW/BGP） |

### 2.3 Calico 版本详细对照

| Calico 版本 | 最低 K8s | 推荐 K8s | eBPF Mode | BGP Route Reflector | 说明 |
|-------------|---------|---------|-----------|-------------------|------|
| v3.24.x | 1.22 | 1.26-1.29 | Beta | GA | 最稳定推荐 |
| v3.25.x | 1.22 | 1.26-1.30 | Beta | GA | |
| v3.26.x | 1.24 | 1.27-1.31 | Beta | GA | |
| v3.27.x | 1.24 | 1.28-1.32 | GA | GA | WireGuard 加密 GA |
| v3.28.x | 1.26 | 1.28-1.33 | GA | GA | 最新的生产推荐 |

**重要**：Calico v3.24+ 移除了对 Kubernetes 1.24 以下版本的支持。

### 2.4 Cilium 版本详细对照

| Cilium 版本 | 最低 K8s | 推荐 K8s | Hubble | Cluster Mesh | 说明 |
|-------------|---------|---------|--------|-------------|------|
| v1.14.x | 1.22 | 1.26-1.29 | GA | GA | 生产推荐 |
| v1.15.x | 1.24 | 1.27-1.30 | GA | GA | |
| v1.16.x | 1.26 | 1.28-1.33 | GA | GA | 最新的生产推荐 |

**内核要求**：
- Cilium 基本功能：内核 4.9+
- Cilium eBPF 数据路径：内核 5.10+
- Cilium 完整功能（Hubble/BGP/Cluster Mesh）：内核 5.10+

### 2.5 Flannel 版本与 K8s 版本

| Flannel 版本 | 后端 | K8s 版本 | 说明 |
|-------------|------|---------|------|
| v0.21.x | VXLAN | 1.26-1.29 | 稳定 |
| v0.22.x | VXLAN/host-gw | 1.26-1.32 | 稳定，推荐 |
| v0.23.x | VXLAN/host-gw | 1.26-1.33 | 较新，使用 WireGuard |

### 2.6 CNI 特性门控对照

| FeatureGate | K8s 版本 | CNI 影响 | 说明 |
|-------------|---------|---------|------|
| `IPv6DualStack` | GA | 所有 CNI | 双栈网络支持 |
| `NetworkPolicyEndpoints` | GA | Calico/Cilium | 需 CNI 支持 |
| `TopologyAwareHints` | GA | 部分 CNI | Service 拓扑感知 |
| `MaxPodsPerNode` | - | - | 依赖 CNI IPAM 配置 |

---

<!-- chunk: 3. 存储驱动与 K8s 版本兼容性 -->
## 3. 存储驱动与 K8s 版本兼容性

### 3.1 存储类参数兼容性

| 存储类型 | StorageClass 参数 | K8s 版本 | 说明 |
|---------|------------------|---------|------|
| AWS EBS gp3 | `type: gp3` | 1.26+ | gp3 是 gp2 的升级版 |
| AWS EBS io2 | `type: io2` | 1.28+ | io2 比 io1 更高性能 |
| GCE PD SSD | `type: pd-ssd` | 1.26+ | |
| Azure UltraSSD | `diskType: UltraSSD_LRS` | 1.28+ | 需要启用 Azure 超前功能 |
| 阿里云 ESSD | `type: cloud_essd` | 1.28+ | PL1/PL2/PL3 等级 |
| 阿里云 ESSD Entry | `type: cloud_essd_entry` | 1.30+ | 低成本版 |

### 3.2 快照 (VolumeSnapshot) 版本

| 组件 | K8s 版本 | CSI 版本 | 说明 |
|------|---------|----------|------|
| VolumeSnapshotClass | GA | v1.0+ | 稳定 |
| VolumeSnapshotContent | GA | v1.0+ | 稳定 |
| VolumeSnapshot | GA | v1.0+ | 稳定 |
| CSI Snapshotter | v5.0+ | v1.0+ | 需要 CSI driver 支持 |

---

<!-- chunk: 4. 网络策略与 K8s 版本 -->
## 4. 网络策略与 K8s 版本

### 4.1 网络策略资源版本

| 资源 | API 版本 | K8s 版本 | 说明 |
|------|---------|---------|------|
| NetworkPolicy | networking.k8s.io/v1 | 1.25+ | v1 版本唯一支持 |
| CiliumNetworkPolicy | cilium.io/v2 | - | Cilium 特有 |
| CalicoNetworkPolicy | projectcalico.org/v3 | - | Calico 特有 |
| NetworkPolicyEntry | Alpha | 1.31+ | 增强版字段（未来版本） |

### 4.2 Pod 级别的网络安全

| 功能 | K8s 版本 | CNI 支持 | 说明 |
|------|---------|---------|------|
| K8s NetworkPolicy | 1.25+ GA | Calico/Cilium/Weave | 标准 |
| Kubernetes Ingress | GA | 所有 CNI | |
| Service Mesh (mTLS) | - | Istio/Linkerd/Cilium | 需额外配置 |

---

<!-- chunk: 5. 多 CIDR 与 IPAM -->
## 5. 多 CIDR 与 IPAM

### 5.1 CNI IPAM 配置兼容性

| CNI | 主要 IPAM 模式 | K8s 1.28 | K8s 1.33 | 说明 |
|-----|--------------|---------|---------|------|
| Calico | Calico IPAM (BGP) | ✅ | ✅ | 分布式 IPAM |
| Calico | host-local | ✅ | ✅ | 简单本地池 |
| Cilium | Cilium IPAM (eBPF) | ✅ | ✅ | 基于 eBPF |
| Cilium | host-local | ✅ | ✅ | 作为回退 |
| Flannel | host-local | ✅ | ✅ | 简单模式 |
| Cilium ENI (AWS) | ENI IPAM | ✅ | ✅ | 云厂商模式 |

### 5.2 Pod CIDR 规划建议

| 集群规模 | 建议 Pod CIDR | 节点 CIDR | 说明 |
|---------|--------------|-----------|------|
| < 50 节点 | /16 (65534 IPs) | /24 每节点 | 充足空间 |
| 50-200 节点 | /16 或 /12 | /24 每节点 | 中等规模 |
| > 200 节点 | /12 或 /8 | /24 每节点 | 大规模集群 |
| 多租户 | 按租户划分子网 | /24 或 /25 | 网络隔离 |

---

<!-- chunk: 6. 升级注意事项 -->
## 6. 升级注意事项

### 6.1 CNI 升级前检查

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 1. 查看当前 CNI 版本
kubectl get pods -n kube-system -l k8s-app=<cni-name> -o jsonpath='{.items[0].spec.containers[0].image}'

# 2. 检查 CNI 配置
cat /etc/cni/net.d/
ls -la /etc/cni/net.d/

# 3. 查看当前 CNI 状态
calicoctl node status  # Calico
cilium status          # Cilium

# 4. 检查是否有自定义 CRD
kubectl get crd | grep -i calico
kubectl get crd | grep -i cilium

# 5. 备份 CNI 配置
cp -r /etc/cni/net.d /backup/cni-net.d-$(date +%Y%m%d)
```
### 6.2 CSI 升级前检查

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 1. 查看当前 CSI driver 版本
kubectl get pods -n kube-system -l app.kubernetes.io/name=<csi-driver> -o jsonpath='{.items[0].spec.containers[0].image}'

# 2. 检查 CSI driver 状态
kubectl get csidriver
kubectl get StorageClass

# 3. 检查 VolumeSnapshot 是否有进行中的操作
kubectl get volumesnapshot -A
kubectl get volumeattachments -A

# 4. 确认所有 PVC 已 Bound
kubectl get pvc -A | grep -v Bound

# 5. 备份 StorageClass 配置
kubectl get StorageClass -o yaml > /backup/storageclass-$(date +%Y%m%d).yaml
```
---

<!-- chunk: 附录：兼容性速查表 -->
## 附录：兼容性速查表

| K8s 版本 | 最低 Calico | 最低 Cilium | 最低 CSI Driver |
|---------|------------|------------|----------------|
| 1.28 | v3.24+ | v1.14+ | CSI Spec v1.0+ |
| 1.29 | v3.25+ | v1.14+ | CSI Spec v1.0+ |
| 1.30 | v3.26+ | v1.15+ | CSI Spec v1.0+ |
| 1.31 | v3.27+ | v1.15+ | CSI Spec v1.0+ |
| 1.32 | v3.28+ | v1.16+ | CSI Spec v1.0+ |
| 1.33 | v3.28+ | v1.16+ | CSI Spec v1.0+ |

---

```yaml
---
id: CSI-CNI-MATRIX-001
domain: storage-networking
type: version-matrix
tags: [csi, cni, version-matrix, compatibility, k8s-1.28-1.33, agent-corpus]
intent_queries:
  - "Calico v3.28 支持哪些 K8s 版本"
  - "Cilium v1.16 对 K8s 1.33 兼容吗"
  - "CSI Spec v1 vs v1beta1 在 K8s 1.30 的区别"
  - "树内存储驱动在哪个版本移除"
difficulty: advanced
target_roles: [sre, ops-engineer]
k8s_versions: ["1.28", "1.29", "1.30", "1.31", "1.32", "1.33"]
related:
  - domain-03-networking-traffic/03-cni-plugins-comparison.md
  - domain-04-storage-data/05-csi-drivers-integration.md
  - domain-01-cluster-fundamentals/21-container-runtime-deep-dive.md
---
```

---

<!-- chunk: Obsidian 相关文档 -->
## Obsidian 相关文档

- domain-03-networking-traffic MOC
- [[domain-03-networking-traffic/README.md|Domain 03: Networking 网络]]
- Kubernetes 网络基础 Network in a Nutshell
- Domain-5 网络 — 开源项目索引
- FAQ 文档
- 网络核心组件
- CNI 架构与核心原理
- 76 - CNI插件深度对比
- 142 - Flannel 完整指南 (Flannel Complete Guide)
- Flannel WireGuard 加密后端配置
- Flannel IPv6 Dual Stack 支持
- Flannel Windows 节点支持

## See Also

- 37-terway-resources-crud-operations
- 38-terway-gc-mechanism
- 40-terway-product-overview
- 41-terway-architecture-deep-dive


<!-- risk-assessed -->
