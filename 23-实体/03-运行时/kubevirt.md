---
title: KubeVirt [entities]
description: '## 概述'
summary: '该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[22-概念/01-核心架构/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]'
category: entities
tags:
- k8s
- cncf
- runtime
- kubevirt
- containerd
- crd
- operator
tier: peripheral
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- KubeVirt 是什么
- 如何 KubeVirt
trigger_keywords:
- KubeVirt
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# KubeVirt

> **CNCF 状态**: Incubating | **类别**: Runtime | **主要语言**: Go

## 概述

KubeVirt 是一个 CNCF 孵化项目，由 Red Hat 主导开发，旨在将传统虚拟机（VM）工作负载引入 Kubernetes 平台。它允许开发团队在 K8s 集群中以管理容器相同的方式管理虚拟机，使得那些难以容器化的遗留应用（如 Windows 工作负载、需要直接硬件访问的应用）也能享受云原生的编排能力。KubeVirt 通过自定义资源定义（CRD）将 VM 抽象为 Kubernetes 原生对象，无需维护独立的虚拟化管理平台。项目于 2017 年开源，目前已被 Red Hat OpenShift Virtualization、SUSE Rancher 等商业产品采用。

## Key Features（核心能力）

- **VM 即 K8s 资源**：通过 VirtualMachine 和 VirtualMachineInstance CRD 将虚拟机生命周期完全纳入 K8s API 管理
- **容器与 VM 混合编排**：支持在同一个 Pod 中运行 VM 和 sidecar 容器，实现边车模式注入
- **云原生存储与网络**：复用 CSI 存储驱动和 CNI 网络插件，PVC 可直接挂载为 VM 磁盘
- **Live Migration**：支持虚拟机热迁移，在节点维护期间实现零停机工作负载转移
- **GPU/设备直通**：通过 Kubernetes Device Plugin 支持 GPU、SR-IOV 等硬件直通
- **标准化 API**：兼容 libvirt 和 QEMU/KVM，提供稳定的虚拟化抽象层

## 架构与工作原理

KubeVirt 架构由多个组件构成：virt-api 负责 VM 相关 API 的认证和准入控制；virt-controller 管理 VM 对应 Pod 的生命周期；virt-handler 以 DaemonSet 形式运行在每个节点，负责与本地 libvirt 通信；virt-launcher 为每个 VMI 创建专属 Pod，在其中运行 libvirtd 实例管理 QEMU/KVM 虚拟机。CDI（Containerized Data Importer）组件负责从 HTTP、S3、Registry 等数据源导入磁盘镜像到 PVC。所有组件通过 Kubernetes 控制器模式协调状态。

## K8s 集成

KubeVirt 深度集成 Kubernetes 生态系统：VM 以 CRD 形式存在，可通过 kubectl get vmi 管理；使用 K8s 调度器进行节点分配；复用 PVC/StorageClass 提供持久化存储；通过 NetworkPolicy 和 CNI 插件实现网络安全；支持 HPA 和 VPA 进行资源自动伸缩。CDI 组件也是以 Operator 模式部署的 K8s 原生控制器。

## 生产用例

- **遗留应用迁移**：将需要完整 OS 环境的传统应用（如老旧数据库、Windows 应用）迁移到 K8s 平台
- **混合工作负载平台**：在同一集群中同时运行容器化微服务和 VM 工作负载，统一管理
- **开发测试环境**：为需要完整 VM 环境的开发团队提供自助式 K8s 原生虚拟机服务
- **安全隔离场景**：多租户环境中利用 VM 级别的强隔离，满足合规要求

## 安装与配置

```bash
# 🟢 安装 KubeVirt Operator
kubectl apply -f https://github.com/kubevirt/kubevirt/releases/download/v1.2.0/kubevirt-operator.yaml
kubectl apply -f https://github.com/kubevirt/kubevirt/releases/download/v1.2.0/kubevirt-cr.yaml

# 🟢 等待部署完成
kubectl wait --for=condition=Available kv/kubevirt -n kubevirt --timeout=300s

# 🟢 验证安装
kubectl get pods -n kubevirt
kubectl get crd | grep kubevirt.io

# 🟢 安装 virtctl CLI
curl -L https://github.com/kubevirt/kubevirt/releases/download/v1.2.0/virtctl-v1.2.0-linux-amd64 -o virtctl
chmod +x virtctl && mv virtctl /usr/local/bin/

# 🟢 安装 CDI (Containerized Data Importer)
kubectl apply -f https://github.com/kubevirt/containerized-data-importer/releases/download/v1.58.0/cdi-operator.yaml
kubectl apply -f https://github.com/kubevirt/containerized-data-importer/releases/download/v1.58.0/cdi-cr.yaml
```

### VirtualMachine CRD 示例

```yaml
apiVersion: kubevirt.io/v1
kind: VirtualMachine
metadata:
  name: ubuntu-vm
  namespace: default
spec:
  running: true
  template:
    metadata:
      labels:
        app: ubuntu-vm
    spec:
      domain:
        cpu:
          cores: 2
        memory:
          guest: 4Gi
        devices:
          disks:
          - name: rootdisk
            disk:
              bus: virtio
          - name: cloudinit
            disk:
              bus: virtio
          interfaces:
          - name: default
            masquerade: {}
        resources:
          requests:
            memory: 4Gi
      networks:
      - name: default
        pod: {}
      volumes:
      - name: rootdisk
        containerDisk:
          image: quay.io/kubevirt/ubuntu-22.04-container-disk
      - name: cloudinit
        cloudInitNoCloud:
          userData: |
            #cloud-config
            password: ubuntu
            chpasswd:
              expire: false
            ssh_authorized_keys:
            - ssh-rsa AAAA...
```

## 运维操作

### 常用命令

```bash
# 🟢 查看 VM 列表
kubectl get vm -A
kubectl get vmi -A  # 运行中的实例

# 🟢 查看 VM 详情
kubectl describe vm ubuntu-vm
kubectl describe vmi ubuntu-vm

# 🟡 启动/停止 VM
virtctl start ubuntu-vm
virtctl stop ubuntu-vm

# 🟢 控制台访问
virtctl console ubuntu-vm

# 🟢 VNC 访问
virtctl vnc ubuntu-vm

# 🟡 热迁移
virtctl migrate ubuntu-vm

# 🟢 查看迁移状态
kubectl get vmim -A  # VirtualMachineInstanceMigration

# 🟡 强制停止 (graceful 失败时)
virtctl stop ubuntu-vm --force --grace-period=0

# 🟢 查看 virt-handler 日志
kubectl logs -n kubevirt -l kubevirt.io=virt-handler --tail=50

# 🟢 查看 virt-controller 日志
kubectl logs -n kubevirt -l kubevirt.io=virt-controller --tail=50

# 🟡 创建快照
virtctl vm-snapshot create ubuntu-vm --name snapshot-1

# 🟡 恢复快照
virtctl vm-snapshot restore ubuntu-vm --name snapshot-1
```

## 故障排查

| 症状 | 可能原因 | 排查命令 | 修复方案 |
|------|----------|----------|----------|
| VM 无法启动 | 节点不支持 KVM/资源不足 | `kubectl describe vmi <name>` | 检查 /dev/kvm 存在，确认节点资源 |
| VMI Pending | PVC 未绑定/调度失败 | `kubectl describe vmi <name>` | 检查 StorageClass 和节点亲和 |
| 热迁移失败 | 共享存储未配置/网络问题 | `kubectl get vmim -o yaml` | 确认使用共享存储 (RWX PVC) |
| 控制台无响应 | virt-launcher Pod 异常 | `kubectl logs virt-launcher-<vm>-<id>` | 检查 QEMU 进程状态 |
| 网络不通 | CNI 配置问题 | `kubectl exec -it <pod> -- ip addr` | 检查 masquerade/bridge 配置 |
| 磁盘导入失败 | CDI 配置错误/源不可达 | `kubectl describe datavolume <name>` | 检查 CDI 日志和源 URL |

### 排查流程

```
1. kubectl get vm/vmi → 确认状态 (Running/Pending/Failed)
2. kubectl describe vmi <name> → 查看 Events 和 Conditions
3. kubectl logs virt-launcher-<vm>-<id> → 查看 QEMU 日志
4. kubectl logs -l kubevirt.io=virt-handler → 查看节点级日志
5. kubectl get events --sort-by=.lastTimestamp → 查看集群事件
```

## 生产案例

### 案例1: Windows 工作负载迁移
- **场景**: 企业有 50+ Windows Server 应用无法容器化
- **方案**: 使用 KubeVirt + CDI 导入 Windows 镜像，通过 virtio 驱动优化性能
- **效果**: 统一管理容器和 VM，运维成本降低 40%

### 案例2: GPU 直通 AI 推理
- **场景**: AI 推理服务需要 GPU 直通和完整 OS 环境
- **方案**: KubeVirt + NVIDIA Device Plugin，GPU 直通给 VM
- **效果**: VM 内 GPU 性能接近裸机，统一 K8s 调度

## 对比替代方案

| 维度 | KubeVirt | OpenStack | Kata Containers | oVirt |
|------|----------|-----------|-----------------|-------|
| 管理平台 | K8s 原生 | 独立控制面 | K8s 原生 | 独立控制面 |
| VM 体验 | 完整 VM | 完整 VM | 轻量 VM | 完整 VM |
| 容器共存 | 同一集群 | 需集成 | 同一 Pod | 不支持 |
| 热迁移 | 支持 | 支持 | 不支持 | 支持 |
| GPU 直通 | 支持 | 支持 | 有限 | 支持 |
| 学习曲线 | 低 (K8s用户) | 高 | 低 | 中 |

## 检查清单

- [ ] 节点支持硬件虚拟化 (VT-x/AMD-V) 且 /dev/kvm 存在
- [ ] virt-handler DaemonSet 在所有目标节点运行
- [ ] 共享存储 (RWX) 已配置用于热迁移
- [ ] CDI 已安装用于磁盘导入
- [ ] VM 资源 requests/limits 已设置
- [ ] 网络策略已配置 (masquerade/bridge)
- [ ] 监控 virt-controller 和 virt-handler 健康状态
- [ ] 定期创建 VM 快照用于备份

## Related

- [[carvel]] — Carvel
- [[holmesgpt]] — HolmesGPT
- [[ko]] — ko
- [[openfunction]] — OpenFunction
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- kubevirt
- index/etcd-index|etcd 知识图谱索引]]


<!-- risk-assessed -->
