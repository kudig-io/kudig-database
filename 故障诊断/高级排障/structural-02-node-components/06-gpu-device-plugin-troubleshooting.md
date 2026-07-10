---
title: GPU 与设备插件故障排查指南 [topic-structural-trouble-shooting]
description: 'title: GPU 与设备插件故障排查指南'
summary: 'title: GPU 与设备插件故障排查指南'
category: structural-troubleshooting
tags:
- troubleshooting
- guide
- kubelet
- containerd
- docker
- daemonset
- operator
- gpu
- cuda
- nvidia
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 技术支持
estimated_read_time: 25min
intent_queries:
- GPU 与设备插件故障排查指南 是什么
- 如何 GPU 与设备插件故障排查指南
- Kubernetes 10 troubleshooting diagnostics 最佳实践
- GPU 与设备插件故障排查指南 故障排查
- GPU 与设备插件故障排查指南 排障步骤
trigger_keywords:
- GPU
- 与设备插件故障排查指南
- troubleshooting
- diagnostics
- structural
- trouble
- shooting
prerequisites:
- kubectl-basics
- troubleshooting-methodology
- gpu-scheduling-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




title: GPU 与设备插件故障排查指南
description: '# GPU 与设备插件故障排查指南'
category: structural-troubleshooting
tags:
- k8s
- troubleshooting
- decision-tree
- [[kubelet|kubelet]]
- [[containerd|containerd]]
- docker
- daemonset
- operator
- gpu
- cuda
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 技术支持
estimated_read_time: 5min
intent_queries:
- GPU 与设备插件故障排查指南 是什么
- 如何 GPU 与设备插件故障排查指南
- GPU 与设备插件故障排查指南 故障排查
- GPU 与设备插件故障排查指南 排障步骤
trigger_keywords:
- GPU
- 与设备插件故障排查指南
- structural
- trouble
- shooting
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

# GPU 与设备插件故障排查指南

> **适用版本**: Kubernetes v1.25 - v1.32, NVIDIA Driver 470+, Device Plugin v0.13+ | **最后更新**: 2026-01 | **难度**: 高级
>
> **版本说明**:
> - v1.26+ DRA (Dynamic Resource Allocation) Alpha
> - v1.31+ DRA 进入 Beta
> - NVIDIA MIG 需要 Driver 450+ 和 Device Plugin v0.12+
> - 时间片共享需要 Device Plugin v0.13+

## 🎯 本文档价值

| 读者对象 | 价值体现 |
| :--- | :--- |
| **初学者** | 搞清楚 GPU 资源在 Kubernetes 中是如何被发现和分配的，学会使用 `nvidia-smi` 验证基础驱动环境，掌握 GPU Pod 的标准配置模板。 |
| **资深专家** | 深入理解 Device Plugin 的 gRPC 注册与 ListAndWatch 机制、MIG（多实例 GPU）的分区逻辑、时间片共享（Time-Slicing）的调度权衡，以及 DRA（动态资源分配）架构对 AI 推理集群的演进方向。 |

---

## 0. 10 分钟快速诊断

1. **组件与资源可见性**：`kubectl -n kube-system get ds -l name=nvidia-device-plugin-daemonset -o wide`（或对应厂商插件）；`kubectl get node <name> -o jsonpath='{.status.allocatable.nvidia\.com/gpu}'`。
2. **驱动健康**：节点执行 `nvidia-smi`，若报错检查驱动/XID；`dmesg | grep -i nvidia | tail`。
3. **Pod 事件**：对 Pending/失败的 GPU Pod `kubectl describe pod`，查看调度原因（资源不足/拓扑/亲和性）或启动错误（挂载/环境变量缺失）。
4. **插件日志与注册**：`kubectl logs -n kube-system ds/nvidia-device-plugin-daemonset -c nvidia-device-plugin-ctr --tail=50`，确认 `ListAndWatch`/`Allocate` 是否报错；查看 `/var/lib/kubelet/device-plugins/kubelet_internal_checkpoint`。
5. **MIG/时间片/NUMA**：检查是否开启 MIG，规格是否匹配；时间片共享需插件版本 ≥0.13；跨 NUMA 部署可需 `TopologyManager` 设置。
6. **快速缓解**：
   - 单节点异常：`cordon` 节点，重载驱动或重启插件 DaemonSet；若 XID 持续，重启机器或下架 GPU。
   - 资源碎片：执行排空重调度，或调整请求规格/关闭 MIG 分片以释放连续资源。
   - 配置错误：回滚自定义插件镜像/参数，恢复官方默认 DaemonSet。
7. **证据留存**：保存插件日志、`nvidia-smi` 输出、Pod 事件、Node allocatable/已分配快照、XID 代码及 dmesg 片段。

---

## 1. 核心原理解析：异构资源接入

### 1.1 设备插件 (Device Plugin) 注册机制

Kubernetes 不原生感知 GPU。接入过程如下：
1. **探测与注册**：设备插件（如 NVIDIA Device Plugin）扫描宿主机 `/dev` 下的特殊文件，并通过 Unix Domain Socket 向 kubelet 注册自己管理的资源名称（如 `nvidia.com/gpu`）。
2. **容量上报**：kubelet 将这些资源作为“可分配容量”更新到 Node 对象的 `Status`。
3. **分配决策**：调度器根据 Pod 的 `limits` 请求进行过滤。在 Pod 真正启动前，kubelet 调用插件的 `Allocate` 方法，获取该 Pod 专属的设备环境变量（如 `NVIDIA_VISIBLE_DEVICES`）和挂载路径。

### 1.2 生产环境典型“AI 算力坑”

1. **GPU 碎片化与抢占失败**：
   - **现象**：Node 上显示有 1 个空闲 GPU，但 Pod 依然 Pending。
   - **深层原因**：该 GPU 可能被分配给了某个正在创建或 Terminating 中的 Pod，或者因为 MIG 模式下，物理 GPU 的剩余空间不足以切分出请求的实例规格。
2. **XID Errors（驱动/硬件问题）**：
   - **现象**：`nvidia-smi` 报错 `Unable to determine the device handle`。
   - **对策**：查看内核 `dmesg`。XID 错误代码（如 XID 31 为内存错误）直接决定了是需要重启驱动还是更换物理硬件。

# 专家级观测工具链（Expert's Toolbox）

```bash
# 专家级：验证 kubelet 与插件的 Socket 通信
# 查看 kubelet 内部设备管理器状态
cat /var/lib/kubelet/device-plugins/kubelet_internal_checkpoint

# 专家级：监控 GPU 核心指标（需部署 DCGM Exporter）
curl localhost:9400/metrics | grep DCGM_FI_DEV_GPU_UTIL

# 专家级：深度检查 NVIDIA 运行时的配置文件
# 确认路径映射和库文件加载逻辑
cat /etc/nvidia-container-runtime/config.toml
```

---

## 目录

1. [异构资源接入逻辑](#1-核心原理解析异构资源接入)
2. [专家观测工具链](#专家级观测工具链experts-toolbox)
3. [问题现象与分配逻辑解析](#12-常见问题现象)
4. [基础排查步骤（初学者）](#22-排查命令集)
5. [深度治理方案](#第三部分解决方案与风险控制)

---

## 问题现象与影响分析

### 1.1 Kubernetes 设备插件架构

```
┌─────────────────────────────────────────────────────────────────────┐
│                         Kubernetes Node                             │
├─────────────────────────────────────────────────────────────────────┤
│                                                                     │
│   ┌─────────────────────────────────────────────────────────────┐   │
│   │                        kubelet                               │   │
│   │  ┌──────────────────────────────────────────────────────┐   │   │
│   │  │            Device Plugin Manager                      │   │   │
│   │  │  - 监听 /var/lib/kubelet/device-plugins/             │   │   │
│   │  │  - 管理设备插件注册                                   │   │   │
│   │  │  - 处理设备分配请求                                   │   │   │
│   │  └──────────────────────────────────────────────────────┘   │   │
│   └─────────────────────────────────────────────────────────────┘   │
│                              │                                       │
│              gRPC (Unix Socket)                                      │
│                              │                                       │
│   ┌──────────────────────────┴──────────────────────────────────┐   │
│   │                   Device Plugins                             │   │
│   │                                                              │   │
│   │  ┌────────────────┐  ┌────────────────┐  ┌────────────────┐ │   │
│   │  │  NVIDIA GPU    │  │  AMD GPU       │  │  RDMA/InfiniBand│ │   │
│   │  │  Plugin        │  │  Plugin        │  │  Plugin        │ │   │
│   │  │                │  │                │  │                │ │   │
│   │  │ nvidia.com/gpu │  │ amd.com/gpu    │  │ rdma/hca       │ │   │
│   │  └───────┬────────┘  └───────┬────────┘  └───────┬────────┘ │   │
│   │          │                   │                   │          │   │
│   └──────────┼───────────────────┼───────────────────┼──────────┘   │
│              │                   │                   │              │
│   ┌──────────┴───────────────────┴───────────────────┴──────────┐   │
│   │                     Hardware Layer                          │   │
│   │  ┌────────────┐    ┌────────────┐    ┌────────────┐        │   │
│   │  │  NVIDIA    │    │  AMD       │    │  Mellanox  │        │   │
│   │  │  GPU Cards │    │  GPU Cards │    │  NICs      │        │   │
│   │  └────────────┘    └────────────┘    └────────────┘        │   │
│   └─────────────────────────────────────────────────────────────┘   │
│                                                                     │
└─────────────────────────────────────────────────────────────────────┘

设备插件注册流程:
┌────────────┐    1. 启动并连接    ┌────────────┐
│   Device   │ ─────────────────> │   kubelet  │
│   Plugin   │                    │  (Manager) │
└────────────┘                    └────────────┘
      │                                  │
      │    2. Register(ResourceName)     │
      │ ─────────────────────────────>   │
      │                                  │
      │    3. ListAndWatch()             │
      │ <───────────────────────────     │
      │                                  │
      │    4. 返回设备列表               │
      │ ─────────────────────────────>   │
      │                                  │
      │    5. Allocate() on Pod request  │
      │ <───────────────────────────     │
      │                                  │
      │    6. 返回挂载/环境变量配置      │
      │ ─────────────────────────────>   │
      └──────────────────────────────────┘
```

### 1.2 常见问题现象

| 问题类型 | 现象描述 | 错误信息 | 查看方式 |
|----------|----------|----------|----------|
| GPU 不可见 | Node 上看不到 GPU 资源 | Capacity 中无 nvidia.com/gpu | `kubectl describe node` |
| Pod 调度失败 | GPU Pod 一直 Pending | Insufficient nvidia.com/gpu | `kubectl describe pod` |
| 设备插件崩溃 | 插件 Pod CrashLoopBackOff | plugin registration failed | `kubectl logs` |
| 驱动问题 | 容器内无法使用 GPU | CUDA driver version insufficient | 应用日志 |
| 设备分配失败 | Pod 启动失败 | failed to allocate device | kubelet 日志 |
| 设备健康检查 | GPU 标记为 unhealthy | device marked as unhealthy | 插件日志 |
| 共享 GPU | 资源碎片化 | 无法精细分配 GPU 资源 | Node 资源状态 |
| MIG 问题 | MIG 设备不可用 | MIG mode enabled but no instances | nvidia-smi |

### 1.3 影响分析

| 问题类型 | 直接影响 | 间接影响 | 影响范围 |
|----------|----------|----------|----------|
| GPU 不可见 | ML 工作负载无法调度 | 模型训练/推理停滞 | 所有 GPU 工作负载 |
| 设备插件崩溃 | 新 Pod 无法获取 GPU | 已运行 Pod 不受影响 | 新调度的 Pod |
| 驱动不兼容 | CUDA 程序运行失败 | 应用崩溃 | 特定 CUDA 版本应用 |
| 设备分配失败 | Pod 启动失败 | 工作负载不可用 | 请求该设备的 Pod |

## 排查方法与步骤

### 2.1 排查决策树

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

```
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
GPU/设备 Pod 问题
        │
        ▼
┌───────────────────┐
│  Pod 状态是什么？  │
└───────────────────┘
        │
        ├── Pending ──────────────────────────────────────┐
        │                                                  │
        │   ┌─────────────────────────────────────────┐   │
        │   │ 检查调度事件                            │   │
        │   │ kubectl describe pod <pod>              │   │
        │   └─────────────────────────────────────────┘   │
        │                  │                               │
        │                  ▼                               │
        │   ┌─────────────────────────────────────────┐   │
        │   │ Insufficient nvidia.com/gpu?            │   │
        │   └─────────────────────────────────────────┘   │
        │          │                │                      │
        │         是               否                      │
        │          │                │                      │
        │          ▼                ▼                      │
        │   ┌────────────┐   ┌────────────────┐           │
        │   │ 检查 Node  │   │ 检查其他资源   │           │
        │   │ GPU 容量   │   │ 或 affinity    │           │
        │   └────────────┘   └────────────────┘           │
        │          │                                       │
        │          ▼                                       │
        │   ┌─────────────────────────────────────────┐   │
        │   │ Node 有 GPU Capacity?                   │   │
        │   └─────────────────────────────────────────┘   │
        │          │                │                      │
        │         否               是                      │
        │          │                │                      │
        │          ▼                ▼                      │
        │   ┌────────────┐   ┌────────────────┐           │
        │   │ 设备插件   │   │ 检查已分配     │           │
        │   │ 问题       │   │ vs 可用数量    │           │
        │   └────────────┘   └────────────────┘           │
        │                                                  │
        ├── ContainerCreating ────────────────────────────┤
        │                                                  │
        │   ┌─────────────────────────────────────────┐   │
        │   │ 检查 kubelet 日志                       │   │
        │   │ journalctl -u kubelet | grep -i gpu     │   │
        │   └─────────────────────────────────────────┘   │
        │                  │                               │
        │                  ▼                               │
        │   ┌─────────────────────────────────────────┐   │
        │   │ device allocation 错误?                 │   │
        │   └─────────────────────────────────────────┘   │
        │          │                │                      │
        │         是               否                      │
        │          │                │                      │
        │          ▼                ▼                      │
        │   ┌────────────┐   ┌────────────────┐           │
        │   │ 设备插件   │   │ 检查其他容器   │           │
        │   │ Allocate   │   │ 启动问题       │           │
        │   │ 失败       │   │                │           │
        │   └────────────┘   └────────────────┘           │
        │                                                  │
        └── Running 但 GPU 不工作 ────────────────────────┤
                                                           │
            ┌─────────────────────────────────────────┐   │
            │ 容器内检查 nvidia-smi                   │   │
            │ kubectl exec <pod> -- nvidia-smi        │   │
            └─────────────────────────────────────────┘   │
                           │                               │
                           ▼                               │
            ┌─────────────────────────────────────────┐   │
            │ nvidia-smi 能否正常运行?                │   │
            └─────────────────────────────────────────┘   │
                   │                │                      │
                  否               是                      │
                   │                │                      │
                   ▼                ▼                      │
            ┌────────────┐   ┌────────────────┐           │
            │ 驱动/设备  │   │ 应用层 CUDA    │           │
            │ 挂载问题   │   │ 版本兼容问题   │           │
            └────────────┘   └────────────────┘           │
                                                           │
                                                           ▼
                                                    ┌────────────┐
                                                    │ 问题定位   │
                                                    │ 完成       │
                                                    └────────────┘
```
### 2.2 排查命令集

#### 设备插件状态检查

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 检查设备插件 DaemonSet 状态
kubectl get ds -n kube-system | grep -E "nvidia|gpu|device"

# 检查设备插件 Pod 状态
kubectl get pods -n kube-system -l app=nvidia-device-plugin-daemonset
kubectl get pods -n gpu-operator-resources

# 查看设备插件日志
kubectl logs -n kube-system -l app=nvidia-device-plugin-daemonset --tail=100

# 检查 NVIDIA GPU Operator 组件 (如果使用)
kubectl get pods -n gpu-operator -o wide
```
#### Node GPU 资源检查

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看节点 GPU 资源
kubectl get nodes -o json | jq '.items[] | {name: .metadata.name, capacity: .status.capacity, allocatable: .status.allocatable}' | grep -A5 -B1 gpu

# 详细查看单个节点
kubectl describe node <node-name> | grep -A10 "Capacity|Allocatable|Allocated"

# 查看 GPU 资源分配情况
kubectl get pods -A -o json | jq '.items[] | select(.spec.containers[].resources.limits."nvidia.com/gpu" != null) | {namespace: .metadata.namespace, name: .metadata.name, node: .spec.nodeName, gpu: .spec.containers[].resources.limits."nvidia.com/gpu"}'
```
#### 主机层 GPU 检查

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# SSH 到 GPU 节点后执行

# NVIDIA GPU 状态
nvidia-smi

# 详细 GPU 信息
nvidia-smi -q

# GPU 进程
nvidia-smi pmon -c 1

# 驱动版本
cat /proc/driver/nvidia/version

# 检查 NVIDIA 设备文件
ls -la /dev/nvidia*

# 检查 NVIDIA 内核模块
lsmod | grep nvidia

# 检查设备插件 socket
ls -la /var/lib/kubelet/device-plugins/

# 检查容器运行时 GPU 配置
# containerd
cat /etc/containerd/config.toml | grep -A10 nvidia

# Docker
cat /etc/docker/daemon.json | jq '.runtimes'
```
#### kubelet 设备相关日志

```bash
# kubelet 设备插件相关日志
journalctl -u kubelet | grep -i "device|plugin|gpu|nvidia" | tail -50

# 设备分配日志
journalctl -u kubelet | grep -i "allocate" | tail -20

# ListAndWatch 相关
journalctl -u kubelet | grep -i "ListAndWatch" | tail -20
```

### 2.3 排查注意事项

| 注意事项 | 说明 | 风险等级 |
|----------|------|----------|
| 不要随意重启设备插件 | 会影响正在运行的 GPU 工作负载的监控 | 中 |
| 驱动升级需要排空节点 | 升级驱动需要先迁移 GPU 工作负载 | 高 |
| MIG 配置变更需重启 | 更改 MIG 模式需要重启 GPU | 高 |
| 时间片配置谨慎调整 | 影响所有共享 GPU 的 Pod 性能 | 中 |
| 检查 CUDA 版本兼容性 | 驱动版本决定支持的最高 CUDA 版本 | 中 |

## 解决方案与风险控制

### 3.1 设备插件未注册/不可用

**问题现象**：Node 上看不到 GPU 资源，`kubectl describe node` 中 Capacity 无 `nvidia.com/gpu`。

**解决步骤**：

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 步骤 1: 检查设备插件 DaemonSet 是否存在且运行正常
kubectl get ds -n kube-system nvidia-device-plugin-daemonset
kubectl get pods -n kube-system -l app=nvidia-device-plugin-daemonset -o wide

# 如果没有安装，部署 NVIDIA Device Plugin
kubectl apply -f https://raw.githubusercontent.com/NVIDIA/k8s-device-plugin/v0.14.0/nvidia-device-plugin.yml

# 步骤 2: 检查插件 Pod 日志
kubectl logs -n kube-system -l app=nvidia-device-plugin-daemonset

# 步骤 3: 在节点上检查基础设施
# SSH 到节点
nvidia-smi  # 确认驱动工作正常

# 检查设备插件 socket 目录
ls -la /var/lib/kubelet/device-plugins/

# 检查 nvidia 运行时是否配置
# 对于 containerd
cat /etc/containerd/config.toml | grep -A20 "\[plugins.*containerd.*runtimes.*nvidia\]"

# 步骤 4: 如果运行时未配置，配置 nvidia-container-runtime
# /etc/containerd/config.toml 添加:
```
**containerd 配置示例**：

```toml
# /etc/containerd/config.toml

version = 2

[plugins."io.containerd.grpc.v1.cri".containerd]
  default_runtime_name = "nvidia"

[plugins."io.containerd.grpc.v1.cri".containerd.runtimes.nvidia]
  runtime_type = "io.containerd.runc.v2"

[plugins."io.containerd.grpc.v1.cri".containerd.runtimes.nvidia.options]
  BinaryName = "/usr/bin/nvidia-container-runtime"
```

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `systemctl stop/restart`：停止/重启系统服务，影响节点上所有容器
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
# 重启 containerd (需要排空节点上的工作负载)
systemctl restart containerd

# 步骤 5: 重启设备插件
kubectl delete pods -n kube-system -l app=nvidia-device-plugin-daemonset

# 步骤 6: 验证 GPU 资源出现
kubectl describe node <gpu-node> | grep -i nvidia
```
### 3.2 GPU Pod 调度失败 (Insufficient)

**问题现象**：GPU Pod 一直 Pending，事件显示 `Insufficient nvidia.com/gpu`。

**解决步骤**：

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 步骤 1: 检查集群 GPU 资源总量
kubectl get nodes -o custom-columns=NAME:.metadata.name,GPU:.status.allocatable."nvidia\.com/gpu"

# 步骤 2: 检查 GPU 资源使用情况
kubectl get pods -A -o json | jq -r '
  .items[] | 
  select(.spec.containers[].resources.limits."nvidia.com/gpu" != null) |
  [.metadata.namespace, .metadata.name, .spec.nodeName, 
   (.spec.containers[] | .resources.limits."nvidia.com/gpu" // "0")] | 
  @tsv' | column -t

# 步骤 3: 计算可用 GPU
# 总容量 - 已分配 = 可用

# 步骤 4: 如果资源不足，考虑以下选项:
# a. 等待其他 GPU 工作负载完成
# b. 添加更多 GPU 节点
# c. 使用 GPU 共享方案 (MIG, 时间片)
# d. 优化请求的 GPU 数量
```
**检查 Pod 是否请求过多 GPU**：

```yaml
# 检查 Pod 配置
apiVersion: v1
kind: Pod
metadata:
  name: gpu-pod
spec:
  containers:
  - name: cuda-container
    image: nvidia/cuda:12.0-runtime
    resources:
      limits:
        nvidia.com/gpu: 1  # 确认是否真的需要这么多
```

### 3.3 设备插件 Allocate 失败

**问题现象**：Pod 在 ContainerCreating 状态卡住，kubelet 日志显示 allocate 失败。

**解决步骤**：

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl delete`：删除资源（可由声明式清单重建）

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 步骤 1: 查看 kubelet 日志定位具体错误
journalctl -u kubelet | grep -i "allocate|device" | tail -50

# 步骤 2: 检查设备插件健康状态
kubectl logs -n kube-system -l app=nvidia-device-plugin-daemonset | grep -i "health|error|fail"

# 步骤 3: 在节点上检查 GPU 设备状态
nvidia-smi -q | grep -A5 "GPU Current Temp|Power Draw|ECC"

# 步骤 4: 检查设备文件权限
ls -la /dev/nvidia*

# 步骤 5: 如果设备不健康，可能需要:
# a. 重置 GPU
nvidia-smi --gpu-reset -i 0  # 危险操作，会影响使用该 GPU 的所有进程

# b. 检查硬件问题
nvidia-smi -q | grep -i "retired|error"

# 步骤 6: 重启设备插件刷新设备列表
kubectl delete pods -n kube-system -l app=nvidia-device-plugin-daemonset
```
### 3.4 容器内 GPU 不可用

**问题现象**：Pod 运行中，但容器内 `nvidia-smi` 失败或 CUDA 程序报错。

**解决步骤**：

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 步骤 1: 进入容器检查
kubectl exec -it <pod-name> -- bash

# 容器内执行
nvidia-smi
# 如果失败，检查设备是否挂载
ls -la /dev/nvidia*

# 检查环境变量
env | grep -i nvidia
env | grep -i cuda

# 步骤 2: 检查 Pod 配置是否正确请求了 GPU
kubectl get pod <pod-name> -o yaml | grep -A10 resources

# 步骤 3: 检查容器运行时是否正确配置
# 在节点上
crictl inspect <container-id> | grep -i nvidia

# 步骤 4: 如果环境变量缺失，检查设备插件配置
# 设备插件应该返回正确的环境变量
```
**正确的 GPU Pod 配置示例**：

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: gpu-test
spec:
  restartPolicy: OnFailure
  containers:
  - name: cuda-test
    image: nvidia/cuda:12.0-base-ubuntu22.04
    command: ["nvidia-smi"]
    resources:
      limits:
        nvidia.com/gpu: 1  # 必须在 limits 中指定
```

### 3.5 CUDA 版本不兼容

**问题现象**：应用报错 `CUDA driver version is insufficient for CUDA runtime version`。

**解决步骤**：

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 步骤 1: 检查节点驱动版本支持的 CUDA 版本
nvidia-smi  # 右上角显示支持的最高 CUDA 版本

# 步骤 2: 检查应用使用的 CUDA 版本
kubectl exec <pod-name> -- cat /usr/local/cuda/version.txt
# 或
kubectl exec <pod-name> -- nvcc --version

# 步骤 3: 确认兼容性
# NVIDIA 驱动版本与 CUDA 版本对应关系:
# https://docs.nvidia.com/cuda/cuda-toolkit-release-notes/index.html

# 步骤 4: 解决方案
# a. 升级节点驱动 (需要排空节点)
# b. 使用较低版本的 CUDA 镜像
```
**版本兼容性参考**：

| CUDA Version | Minimum Driver Version |
|--------------|------------------------|
| CUDA 12.x    | >= 525.60.13           |
| CUDA 11.8    | >= 520.61.05           |
| CUDA 11.7    | >= 515.43.04           |
| CUDA 11.6    | >= 510.39.01           |

### 3.6 MIG (Multi-Instance GPU) 问题

**问题现象**：MIG 模式启用但设备不可用，或 MIG 实例不符合预期。

**解决步骤**：

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `kubectl drain`：驱逐节点所有 Pod，业务流量受影响
> - `kubectl delete`：删除资源（可由声明式清单重建）

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 步骤 1: 检查 GPU 是否支持 MIG (A100, A30, H100 等)
nvidia-smi -q | grep "MIG Mode"

# 步骤 2: 查看当前 MIG 配置
nvidia-smi mig -lgi  # 列出 GPU Instances
nvidia-smi mig -lci  # 列出 Compute Instances

# 步骤 3: 如果需要重新配置 MIG
# 首先排空节点上的 GPU 工作负载
kubectl drain <node> --ignore-daemonsets

# 启用 MIG 模式 (需要重启)
nvidia-smi -mig 1 -i 0

# 重启节点或重置 GPU
# 重启后创建 MIG 实例
nvidia-smi mig -cgi 9,9,9,9,9,9,9 -i 0  # 创建 7 个 1g.5gb 实例
nvidia-smi mig -cci -i 0  # 创建计算实例

# 步骤 4: 验证 MIG 设备
nvidia-smi -L

# 步骤 5: 重启设备插件以发现新的 MIG 设备
kubectl delete pods -n kube-system -l app=nvidia-device-plugin-daemonset

# 步骤 6: 恢复节点
kubectl uncordon <node>
```
**MIG 设备请求示例**：

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: mig-pod
spec:
  containers:
  - name: mig-container
    image: nvidia/cuda:12.0-runtime
    resources:
      limits:
        # MIG 设备资源名称格式
        nvidia.com/mig-1g.5gb: 1  # 请求 1 个 1g.5gb MIG 实例
```

### 3.7 GPU 时间片共享问题

**问题现象**：使用时间片共享时性能下降或调度异常。

**配置时间片共享**：

```yaml
# NVIDIA Device Plugin ConfigMap
apiVersion: v1
kind: ConfigMap
metadata:
  name: nvidia-device-plugin-config
  namespace: kube-system
data:
  config.yaml: |
    version: v1
    sharing:
      timeSlicing:
        renameByDefault: false
        failRequestsGreaterThanOne: false
        resources:
        - name: nvidia.com/gpu
          replicas: 4  # 每个 GPU 虚拟成 4 个
```

**解决步骤**：

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源
> - `kubectl rollout undo/restart`：触发滚动变更，影响副本

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 步骤 1: 应用时间片配置
kubectl apply -f nvidia-device-plugin-config.yaml

# 步骤 2: 重启设备插件使配置生效
kubectl rollout restart ds/nvidia-device-plugin-daemonset -n kube-system

# 步骤 3: 验证虚拟 GPU 数量
kubectl describe node <gpu-node> | grep nvidia.com/gpu
# 应该看到 Capacity 变成原来的 4 倍

# 步骤 4: 监控时间片使用情况
# 时间片共享会导致 GPU 利用率显示异常，需要关注实际性能
```
### 3.8 RDMA/InfiniBand 设备问题

**问题现象**：高性能网络设备不可用，分布式训练性能差。

**解决步骤**：

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 步骤 1: 检查 RDMA 设备插件
kubectl get ds -n kube-system | grep rdma

# 步骤 2: 在节点上检查 RDMA 设备
ibstat
ibv_devices
rdma link

# 步骤 3: 检查节点资源
kubectl describe node <node> | grep -i rdma

# 步骤 4: 部署 RDMA 设备插件 (如果未部署)
# 以 k8s-rdma-shared-dev-plugin 为例
kubectl apply -f https://raw.githubusercontent.com/Mellanox/k8s-rdma-shared-dev-plugin/master/images/k8s-rdma-shared-dev-plugin-ds.yaml
```
**RDMA Pod 配置示例**：

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: rdma-pod
spec:
  containers:
  - name: rdma-container
    image: your-rdma-app
    resources:
      limits:
        rdma/hca_shared_devices_a: 1  # RDMA 设备资源
        nvidia.com/gpu: 1             # GPU 设备
    securityContext:
      capabilities:
        add: ["IPC_LOCK"]  # RDMA 需要的能力
```

### 3.9 安全生产风险提示

| 操作 | 风险等级 | 潜在风险 | 建议措施 |
|------|----------|----------|----------|
| 升级 GPU 驱动 | 高 | 所有 GPU 工作负载中断 | 排空节点，灰度升级 |
| 重置 GPU (`nvidia-smi --gpu-reset`) | 高 | 杀死所有使用该 GPU 的进程 | 确保无运行工作负载 |
| 修改 MIG 配置 | 高 | 需要重启，影响所有 GPU Pod | 排空节点后操作 |
| 重启设备插件 | 中 | 新 Pod 短时无法调度 | 选择低峰期 |
| 修改时间片配置 | 中 | 影响 GPU 资源计算和调度 | 充分测试后上线 |
| 修改容器运行时配置 | 中 | 需重启 containerd | 排空节点后操作 |

### 附录：快速诊断命令

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# ===== 一键诊断脚本 =====

echo "=== GPU Node 状态 ==="
kubectl get nodes -o custom-columns=NAME:.metadata.name,GPU:.status.allocatable."nvidia\.com/gpu"

echo -e "\n=== 设备插件状态 ==="
kubectl get pods -n kube-system -l app=nvidia-device-plugin-daemonset -o wide

echo -e "\n=== GPU Pod 分布 ==="
kubectl get pods -A -o json | jq -r '
  .items[] | 
  select(.spec.containers[].resources.limits."nvidia.com/gpu" != null) |
  [.metadata.namespace, .metadata.name, .spec.nodeName, .status.phase] | 
  @tsv' | column -t

echo -e "\n=== Pending GPU Pods ==="
kubectl get pods -A --field-selector=status.phase=Pending -o json | jq -r '
  .items[] | 
  select(.spec.containers[].resources.limits."nvidia.com/gpu" != null) |
  [.metadata.namespace, .metadata.name] | 
  @tsv'

echo -e "\n=== 设备插件日志 (最近 10 条) ==="
kubectl logs -n kube-system -l app=nvidia-device-plugin-daemonset --tail=10 2>/dev/null || echo "无法获取日志"

# ===== 节点级检查 (需要 SSH 到节点) =====
# nvidia-smi
# ls -la /var/lib/kubelet/device-plugins/
# journalctl -u kubelet | grep -i gpu | tail -20
```
### 附录：常用设备插件部署

```yaml
# NVIDIA Device Plugin (标准部署)
---
apiVersion: apps/v1
kind: DaemonSet
metadata:
  name: nvidia-device-plugin-daemonset
  namespace: kube-system
spec:
  selector:
    matchLabels:
      name: nvidia-device-plugin-ds
  updateStrategy:
    type: RollingUpdate
  template:
    metadata:
      labels:
        name: nvidia-device-plugin-ds
    spec:
      tolerations:
      - key: nvidia.com/gpu
        operator: Exists
        effect: NoSchedule
      priorityClassName: system-node-critical
      containers:
      - image: nvcr.io/nvidia/k8s-device-plugin:v0.14.0
        name: nvidia-device-plugin-ctr
        env:
        - name: FAIL_ON_INIT_ERROR
          value: "false"
        securityContext:
          allowPrivilegeEscalation: false
          capabilities:
            drop: ["ALL"]
        volumeMounts:
        - name: device-plugin
          mountPath: /var/lib/kubelet/device-plugins
      volumes:
      - name: device-plugin
        hostPath:
          path: /var/lib/kubelet/device-plugins
      nodeSelector:
        # 只在有 GPU 的节点上运行
        nvidia.com/gpu.present: "true"
```

## Related

- 08-docker-troubleshooting-guide
- 16-troubleshooting-guide
- [[系统基础/速查卡/go.md|go]]
- [[系统基础/速查卡/k8s.md|k8s]]
- [[系统基础/速查卡/docker.md|docker]]
- [[生态参考/领域索引/pod-index.md|Pod 知识图谱索引]]
- [[生态参考/领域索引/node-index.md|Node 知识图谱索引]]
- [[生态参考/领域索引/ai-gpu-index.md|AI / GPU 基础设施知识图谱索引]]
- [[生态参考/领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]
- [[生态参考/领域索引/scheduler-index.md|Scheduler 调度与弹性伸缩知识图谱索引]]

## See Also

- [[故障诊断/高级排障/02-node-components/04-node-troubleshooting.md|04-node-troubleshooting]]
- [[故障诊断/高级排障/02-node-components/05-image-registry-troubleshooting.md|05-image-registry-troubleshooting]]
- [[故障诊断/高级排障/02-node-components/01-kubelet-troubleshooting.md|01-kubelet-troubleshooting]]
- [[故障诊断/高级排障/02-node-components/02-kube-proxy-troubleshooting.md|02-kube-proxy-troubleshooting]]

```

<!-- risk-assessed -->
