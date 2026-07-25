---
title: 节点生命周期管理
description: '# 节点生命周期管理'
summary: 'Kubernetes 节点从加入到移除的完整生命周期涉及 [[kubelet|kubelet]]、Node Lifecycle Controller、Cluster Autoscaler 等多个组件的协作。无论控制面节点还是工作节点，都必须经过注册、认证、状态上报等关键流程。'
category: concepts
tags:
- k8s
- node
- kubelet
- lifecycle
- registration
- conditions
- node-controller
- containerd
- cri-o
- rbac
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 节点生命周期管理 是什么
- 如何 节点生命周期管理
trigger_keywords:
- 节点生命周期管理
prerequisites:
- kubectl-basics
- gpu-scheduling-basics
status: reviewed
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 节点生命周期管理

## 概述

Kubernetes 节点从加入到移除的完整生命周期涉及 [[kubelet|kubelet]]、Node Lifecycle Controller、Cluster Autoscaler 等多个组件的协作。无论控制面节点还是工作节点，都必须经过注册、认证、状态上报等关键流程。

## 节点生命周期五个阶段

> ⚠️ **🔴 灾难性操作** — 含不可逆命令，执行前必须满足变更窗口+双人复核+事前备份+回滚方案
> - `kubeadm reset`：清理节点所有 K8s 配置/证书/CNI，节点脱离集群

```
阶段 1: 节点准备
├── 物理/虚拟机创建
├── 安装容器运行时（containerd/cri-o）
├── 安装 kubelet 二进制 + systemd 服务
└── 网络配置（CNI 插件安装）

阶段 2: 节点注册
├── kubeadm join / bootstrap token 认证
├── CSR 签发 → kubelet 获取正式证书
├── Node 对象创建 → API Server 注册
└── kubelet 状态上报 → Ready

阶段 3: 正常运行
├── Pod 调度与容器管理
├── 资源监控与上报（cAdvisor）
├── 证书自动轮换
└── 健康检查与状态同步

阶段 4: 节点运维
├── drain → 维护操作 → uncordon
├── 版本升级（kubelet/kubeadm/OS）
├── 弹性伸缩（Cluster Autoscaler）
└── 问题排查与恢复

阶段 5: 节点移除
├── drain 驱逐所有 Pod
├── delete node 从集群移除
├── kubeadm reset 清理节点  # ⚠️ 清理节点所有 K8s 配置
└── 云厂商释放实例
```

## Node 对象结构

```yaml
apiVersion: v1
kind: Node
metadata:
  name: node-1
  labels:
    kubernetes.io/arch: amd64
    kubernetes.io/os: linux
    kubernetes.io/hostname: node-1
    node-role.kubernetes.io/worker: ""
spec:
  podCIDR: 10.244.1.0/24
  taints:
  - key: node.kubernetes.io/not-ready
    effect: NoSchedule
status:
  conditions:
  - type: Ready
    status: "True"
    reason: KubeletReady
  addresses:
  - type: InternalIP
    address: 192.168.1.10
  capacity:
    cpu: "4"
    memory: 8Gi
    pods: "110"
  allocatable:
    cpu: "3800m"
    memory: 7Gi
    pods: "110"
```

## 节点 Conditions

| Condition | 说明 | 影响 |
|-----------|------|------|
| `Ready` | 节点健康状态 | False → NoExecute 驱逐 |
| `MemoryPressure` | 内存压力 | 驱逐 BestEffort Pod |
| `DiskPressure` | 磁盘压力 | 驱逐日志和未使用镜像 |
| `PIDPressure` | PID 不足 | 驱逐 Pod |
| `NetworkUnavailable` | 网络未配置 | 阻止调度 |

## kubelet 核心组件

```
Kubelet:
  ├── nodeName / hostname
  ├── kubeClient（API 客户端）
  ├── podManager（Pod 管理）
  ├── containerManager（容器管理）
  ├── evictionManager（驱逐管理器）
  ├── certificateManager（证书管理器）
  ├── probeManager（探针管理器）
  ├── pleg（Pod 生命周期事件生成器）
  ├── statusManager（状态管理器）
  ├── volumeManager（卷管理器）
  └── networkPlugin（网络插件）
```

## 节点注册流程

```
kubelet 启动
  ↓
Bootstrap Token 认证
  ↓
提交 CSR（Certificate Signing Request）
  ↓
csrapproving 控制器自动审批
  ↓
获取客户端证书
  ↓
registerNode（创建 Node 对象）
  ↓
设置 labels/taints
  ↓
syncNodeStatus 循环上报状态
  ↓
Node Ready

```

## Node Lifecycle Controller

控制面中的 Node Lifecycle Controller 负责：

1. **监控节点健康**：检查 `nodeMonitorGracePeriod` 内是否收到心跳
2. **标记失联节点**：超过 `podEvictionTimeout` 后标记 Node Unknown
3. **驱逐 Pod**：对失联节点上的 Pod 执行 RateLimit 驱逐
4. **管理 PodCIDR**：为新节点分配 Pod CIDR
5. **处理污点**：设置/清除节点污点

## 弹性伸缩

| 组件 | 功能 |
|------|------|
| Cluster Autoscaler | 根据 Pod 调度需求自动增减节点 |
| Karpenter | 下一代节点自动化工具，支持 JIT 节点创建 |
| 云厂商 Autoscaling Group | AWS ASG / GCP MIG / Azure VMSS |

## 特殊节点类型

| 类型 | 特点 |
|------|------|
| Windows 节点 | 支持 Windows Server 容器，标签 `kubernetes.io/os=windows` |
| ARM 节点 | ARM64/Graviton 架构，多架构镜像 `linux/amd64,linux/arm64` |
| GPU 节点 | 需要安装 NVIDIA Device Plugin |
| 云厂商节点 | 通过 `provider-id` 关联云实例 |

## 常用管理命令

| 命令 | 说明 |
|------|------|
| `kubectl get nodes` | 列出节点 |
| `kubectl describe node <name>` | 节点详情 |
| `kubectl cordon <node>` | 标记不可调度 |
| `kubectl uncordon <node>` | 恢复调度 |
| `kubectl drain <node>` | 驱逐所有 Pod |
| `kubectl top nodes` | 节点资源使用 |
| `kubectl label node <node> key=value` | 添加标签 |
| `kubectl taint node <node> key=value:NoSchedule` | 添加污点 |

## 常见错误

| 错误 | 现象 | 原因 | 解决方案 |
|------|------|------|---------|
| 节点 NotReady | `Ready=False` | kubelet 未运行或无法连接 API Server | `systemctl status kubelet` |
| CSR 未审批 | 节点注册卡住 | 自动审批 RBAC 缺失 | 检查 ClusterRoleBinding |
| 证书过期 | 节点 NotReady | kubelet 客户端证书过期 | 检查 `rotateCertificates: true` |
| 多网卡 IP 错误 | 节点使用错误 IP | kubelet 自动选择 | 指定 `--node-ip` |
| cgroup driver 不匹配 | kubelet 启动失败 | containerd 与 kubelet 不一致 | 统一使用 `systemd` |

## 相关概念

- [[26-技能/03-节点/node/运维操作/node-drain-and-maintenance.md|[[节点驱逐与维护|节点驱逐与维护]]]]
- [[26-技能/07-安全/certificate/kubelet-certificate-rotation.md|[[kubelet 证书轮换机制|kubelet 证书轮换机制]]]]
- [[26-技能/03-节点/node/运维操作/kubelet-eviction-mechanism.md|[[kubelet 资源驱逐机制|kubelet 资源驱逐机制]]]]
- [[23-实体/02-K8s核心组件/kubelet.md|kubelet]]
- [[pod-lifecycle|Pod 生命周期]]

## Related

- [[containerd]] — containerd
- [[cri-o]] — CRI-O
- [[cni]] — CNI (Container Network Interface)
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[23-实体/02-K8s核心组件/kubelet.md|kubelet]] — kubelet

```

<!-- risk-assessed -->
