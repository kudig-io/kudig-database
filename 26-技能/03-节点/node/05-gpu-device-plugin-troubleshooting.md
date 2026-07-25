---
title: GPU 与设备插件故障排查
description: 针对 GPU 节点设备插件（NVIDIA/AMD/RDMA）的故障排查技能，覆盖 Device Plugin 注册机制、GPU 调度失败、驱动兼容、MIG 分区、时间片共享及 DRA 演进
summary: GPU/设备插件是 AI 算力节点的核心组件，本技能覆盖设备注册、分配、驱动、MIG、RDMA 全链路故障排查
category: skill
tags:
- k8s
- node
- troubleshooting
- gpu
- nvidia
- device-plugin
- cuda
- mig
- rdma
- dra
- ai-infra
sources:
- 故障诊断/高级排障/structural-02-node-components/06-gpu-device-plugin-troubleshooting.md
- 故障诊断/核心排障/06-node-notready-diagnosis.md
created: '2026-07-23'
updated: '2026-07-23'
lifecycle: active
tier: core
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- AI 平台工程师
- 技术支持
estimated_read_time: 20min
intent_queries:
- GPU Pod 调度失败怎么排查
- nvidia.com/gpu 资源不可见
- 设备插件 CrashLoopBackOff
- CUDA 版本不兼容怎么办
- MIG 实例配置失败
- GPU 时间片共享性能下降
- RDMA 设备不可用
trigger_keywords:
- GPU
- nvidia
- device-plugin
- CUDA
- MIG
- RDMA
- DRA
- 设备插件
k8s_versions:
- '1.25'
- '1.26'
- '1.28'
- '1.30'
- '1.31'
- '1.32'
---

# GPU 与设备插件故障排查

> **适用版本**: Kubernetes v1.25 - v1.32, NVIDIA Driver 470+, Device Plugin v0.13+
>
> **版本说明**:
> - v1.26+ DRA (Dynamic Resource Allocation) Alpha
> - v1.31+ DRA 进入 Beta
> - NVIDIA MIG 需要 Driver 450+ 和 Device Plugin v0.12+
> - 时间片共享需要 Device Plugin v0.13+

---

## 1. 概述

### 核心原理：异构资源接入

Kubernetes 不原生感知 GPU。设备插件（Device Plugin）通过以下流程接入：

1. **探测与注册**：设备插件扫描宿主机 `/dev` 下的设备文件，通过 Unix Domain Socket 向 kubelet 注册资源名称（如 `nvidia.com/gpu`）
2. **容量上报**：kubelet 将资源作为"可分配容量"更新到 Node `Status.Allocatable`
3. **分配决策**：调度器根据 Pod `limits` 过滤；Pod 启动前 kubelet 调用插件 `Allocate` 方法获取设备环境变量和挂载路径

### 典型触发场景

| 场景 | 症状 | 紧急程度 |
|------|------|----------|
| GPU 资源不可见 | Node Capacity 无 `nvidia.com/gpu` | 高 |
| Pod 调度失败 | Pending + `Insufficient nvidia.com/gpu` | 高 |
| 设备插件崩溃 | DaemonSet CrashLoopBackOff | 高 |
| 驱动不兼容 | `CUDA driver version insufficient` | 中 |
| Allocate 失败 | ContainerCreating 卡住 | 高 |
| MIG 配置异常 | MIG 实例不可用 | 中 |
| GPU 硬件故障 | XID Errors / `nvidia-smi` 报错 | 严重 |

---

## 2. 症状识别

### 2.1 症状模式表

| # | 症状描述（错误消息/事件原文） | 检测方法 | 置信度 | 排除条件 | 路由 |
|---|---------------------------|---------|:---:|---------|------|
| S1 | Node Capacity 中无 `nvidia.com/gpu`（GPU 资源不可见） | `kubectl describe node <node> \| grep nvidia.com/gpu` | 0.90 | 节点本身 NotReady → 先转 [[26-技能/03-节点/node/01-node-notready-diagnosis.md\|NotReady 诊断]] | → GPU-RC-01/02/03 |
| S2 | GPU Pod Pending + Events `0/N nodes are available: Insufficient nvidia.com/gpu` | `kubectl describe pod` Events 段 | 0.95 | 资源确实耗尽属容量规划而非故障 → GPU-RC-05 | → GPU-RC-01/05 |
| S3 | 设备插件 Pod `CrashLoopBackOff` / 日志 `plugin registration failed` | `kubectl logs -n kube-system <device-plugin-pod>` | 0.95 | kubelet 自身异常导致注册失败 → 先查 kubelet | → GPU-RC-01/09 |
| S4 | 容器内报 `CUDA driver version is insufficient for CUDA runtime version` | 应用容器日志 `kubectl logs` | 0.95 | 应用层 CUDA 代码 Bug 不属本技能（见 2.2） | → GPU-RC-06 |
| S5 | Pod 启动失败 + kubelet 日志 `failed to allocate device` / `UnexpectedAdmissionError` | `journalctl -u kubelet \| grep -i device` | 0.90 | 节点重启后 checkpoint 未恢复属临时现象，等待插件重注册 | → GPU-RC-08/09 |
| S6 | 插件日志 `device marked as unhealthy` / dmesg 出现 `NVRM: Xid` 错误码 | 插件日志 + `dmesg \| grep -i xid` | 0.90 | 可恢复 Xid（如 13/31 应用级）无需换卡 | → GPU-RC-04 |
| S7 | `nvidia-smi` 报 `MIG mode enabled but no MIG instances` / MIG 设备列表为空 | 节点执行 `nvidia-smi -L` | 0.95 | 未启用 MIG 的集群不适用 | → GPU-RC-07 |
| S8 | `nvidia-smi` 命令失败：`NVIDIA-SMI has failed because it couldn't communicate with the NVIDIA driver` | 节点 SSH 执行 `nvidia-smi` | 0.95 | 内核升级后驱动未重编译属已知变更场景 | → GPU-RC-02 |
| S9 | 分布式训练报 `ibstat` 异常 / NCCL `no IB device found` | `ibstat` + 训练框架日志 | 0.85 | 网络策略导致的通信失败 → 转网络排障（见 2.2） | → GPU-RC-10 |

### 2.2 排除标准

以下情况不属于本技能范围：
- 应用层 CUDA 代码 Bug（非基础设施问题）
- 网络策略导致的分布式训练通信失败（→ 网络排障）
- PVC/存储问题导致的数据加载失败（→ 存储排障）

---

## 3. 快速分级（2 分钟内完成）

```bash
# 🟢 低风险：只读
# 1. 检查 GPU 资源可见性
kubectl get nodes -o custom-columns=NAME:.metadata.name,GPU:.status.allocatable."nvidia\.com/gpu"

# 2. 检查设备插件 DaemonSet 状态
kubectl get ds -n kube-system -l name=nvidia-device-plugin-daemonset -o wide

# 3. 检查 Pending GPU Pods
kubectl get pods -A --field-selector=status.phase=Pending -o json | jq -r '
  .items[] | select(.spec.containers[].resources.limits."nvidia.com/gpu" != null) |
  [.metadata.namespace, .metadata.name] | @tsv'
```

### 严重性分级

| 级别 | 条件 | 响应时间 |
|------|------|----------|
| P0-严重 | 全部 GPU 节点不可用 / XID 硬件错误 | 立即 |
| P1-高 | 单节点 GPU 不可见 / 设备插件崩溃 | 15 分钟 |
| P2-中 | 部分 GPU Pod 调度失败 / 驱动兼容问题 | 1 小时 |
| P3-低 | 时间片性能下降 / MIG 配置优化 | 4 小时 |

---

## 4. 诊断工作流

### Phase 1: 快速检查（只读，零风险）

```bash
# 🟢 低风险：只读
# 1.1 设备插件状态
kubectl get ds -n kube-system | grep -E "nvidia|gpu|device"
kubectl get pods -n kube-system -l app=nvidia-device-plugin-daemonset -o wide

# 1.2 设备插件日志
kubectl logs -n kube-system -l app=nvidia-device-plugin-daemonset --tail=100

# 1.3 Node GPU 资源
kubectl describe node <gpu-node> | grep -A10 "Capacity\|Allocatable\|Allocated"

# 1.4 GPU Pod 分布
kubectl get pods -A -o json | jq -r '
  .items[] | select(.spec.containers[].resources.limits."nvidia.com/gpu" != null) |
  [.metadata.namespace, .metadata.name, .spec.nodeName, .status.phase] | @tsv' | column -t

# 1.5 kubelet 设备相关日志
journalctl -u kubelet | grep -i "device\|plugin\|gpu\|nvidia" | tail -50
```

### Phase 2: 深度检查（只读，需 SSH）

```bash
# 🟢 低风险：只读
# 2.1 GPU 驱动状态
nvidia-smi
nvidia-smi -q | grep -A5 "GPU Current Temp\|Power Draw\|ECC"
cat /proc/driver/nvidia/version

# 2.2 设备文件与内核模块
ls -la /dev/nvidia*
lsmod | grep nvidia

# 2.3 设备插件 Socket
ls -la /var/lib/kubelet/device-plugins/
cat /var/lib/kubelet/device-plugins/kubelet_internal_checkpoint

# 2.4 容器运行时 GPU 配置
cat /etc/containerd/config.toml | grep -A10 nvidia

# 2.5 XID 错误检查
dmesg | grep -i nvidia | tail -20

# 2.6 GPU 进程监控
nvidia-smi pmon -c 1
```

### Phase 3: 主动探测（中风险）

```bash
# 🟡 中风险
# 3.1 容器内 GPU 验证
kubectl exec <gpu-pod> -- nvidia-smi
kubectl exec <gpu-pod> -- ls -la /dev/nvidia*
kubectl exec <gpu-pod> -- env | grep -i nvidia

# 3.2 运行时检查
crictl inspect <container-id> | grep -i nvidia

# 3.3 DCGM 监控（如已部署）
curl localhost:9400/metrics | grep DCGM_FI_DEV_GPU_UTIL
```

---

## 5. 根因分类

| 编号 | 根因 | 概率 | 关键证据 |
|------|------|------|----------|
| GPU-RC-01 | 设备插件未部署/崩溃 | 高 | DaemonSet 不健康 |
| GPU-RC-02 | NVIDIA 驱动未安装/损坏 | 高 | `nvidia-smi` 失败 |
| GPU-RC-03 | containerd nvidia 运行时未配置 | 中 | config.toml 无 nvidia 段 |
| GPU-RC-04 | GPU 硬件故障 (XID Error) | 中 | dmesg XID 错误码 |
| GPU-RC-05 | GPU 资源已耗尽 | 高 | Allocatable 全部分配 |
| GPU-RC-06 | CUDA/Driver 版本不兼容 | 中 | 应用报版本错误 |
| GPU-RC-07 | MIG 配置错误 | 低 | MIG 实例列表异常 |
| GPU-RC-08 | 设备文件权限问题 | 低 | `/dev/nvidia*` 权限异常 |
| GPU-RC-09 | kubelet Device Plugin Manager 异常 | 低 | checkpoint 文件损坏 |
| GPU-RC-10 | RDMA/InfiniBand 设备不可用 | 低 | `ibstat` 报错 |

---

## 6. 修复操作

### 6.1 🟢 低风险

```bash
# 重启设备插件 Pod（不影响已运行 GPU 工作负载）
kubectl delete pods -n kube-system -l app=nvidia-device-plugin-daemonset

# 验证 GPU 资源恢复
kubectl describe node <gpu-node> | grep -i nvidia
```

### 6.2 🟡 中风险（人工审批）

```bash
# 部署 NVIDIA Device Plugin（如未安装）
kubectl apply -f https://raw.githubusercontent.com/NVIDIA/k8s-device-plugin/v0.14.0/nvidia-device-plugin.yml

# 配置 containerd nvidia 运行时
# /etc/containerd/config.toml 添加:
# [plugins."io.containerd.grpc.v1.cri".containerd.runtimes.nvidia]
#   runtime_type = "io.containerd.runc.v2"
# [plugins."io.containerd.grpc.v1.cri".containerd.runtimes.nvidia.options]
#   BinaryName = "/usr/bin/nvidia-container-runtime"

# 重启 containerd（需先 cordon 节点）
kubectl cordon <node>
systemctl restart containerd
kubectl uncordon <node>
```

### 6.3 🔴 高风险

```bash
# GPU 重置（会杀死所有使用该 GPU 的进程）
nvidia-smi --gpu-reset -i 0

# MIG 重新配置（需排空节点 + 重启 GPU）
kubectl drain <node> --ignore-daemonsets --delete-emptydir-data
nvidia-smi -mig 1 -i 0
# 重启节点后:
nvidia-smi mig -cgi 9,9,9,9,9,9,9 -i 0
nvidia-smi mig -cci -i 0
kubectl uncordon <node>

# GPU 驱动升级（需排空节点）
kubectl drain <node> --ignore-daemonsets
# 执行驱动安装/升级脚本
# 验证: nvidia-smi
kubectl uncordon <node>
```

---

## 7. MIG 与时间片共享

### 7.1 MIG (Multi-Instance GPU) 配置

```bash
# 检查 MIG 支持（A100, A30, H100）
nvidia-smi -q | grep "MIG Mode"

# 查看当前 MIG 配置
nvidia-smi mig -lgi  # 列出 GPU Instances
nvidia-smi mig -lci  # 列出 Compute Instances
```

MIG Pod 请求示例：
```yaml
resources:
  limits:
    nvidia.com/mig-1g.5gb: 1  # 请求 1 个 1g.5gb MIG 实例
```

### 7.2 GPU 时间片共享

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

### 7.3 RDMA/InfiniBand 设备

```bash
# 检查 RDMA 设备
ibstat
ibv_devices
rdma link

# 检查节点 RDMA 资源
kubectl describe node <node> | grep -i rdma
```

RDMA Pod 配置要点：
```yaml
resources:
  limits:
    rdma/hca_shared_devices_a: 1
    nvidia.com/gpu: 1
securityContext:
  capabilities:
    add: ["IPC_LOCK"]  # RDMA 需要
```

---

## 8. CUDA 版本兼容性

| CUDA Version | Minimum Driver Version |
|--------------|------------------------|
| CUDA 12.x    | >= 525.60.13           |
| CUDA 11.8    | >= 520.61.05           |
| CUDA 11.7    | >= 515.43.04           |
| CUDA 11.6    | >= 510.39.01           |

```bash
# 检查节点驱动支持的最高 CUDA 版本
nvidia-smi  # 右上角显示

# 检查应用 CUDA 版本
kubectl exec <pod> -- nvcc --version
```

---

## 9. 生产环境典型"AI 算力坑"

### 案例 1: GPU 碎片化与抢占失败

- **现象**：Node 显示有 1 个空闲 GPU，但 Pod 依然 Pending
- **根因**：该 GPU 被分配给正在创建/Terminating 的 Pod；或 MIG 模式下剩余空间不足
- **解决**：等待 Terminating Pod 完成；或调整 MIG 分片规格

### 案例 2: XID Errors 导致设备不健康

- **现象**：`nvidia-smi` 报错 `Unable to determine the device handle`
- **根因**：XID 错误码（如 XID 31 = 内存错误）指示硬件/驱动问题
- **解决**：XID 31/79 → 更换硬件；XID 48/63 → 重置驱动或重启节点

### 案例 3: 设备插件注册失败循环

- **现象**：Device Plugin Pod CrashLoopBackOff
- **根因**：kubelet Device Plugin Manager 的 checkpoint 文件损坏
- **解决**：删除 `/var/lib/kubelet/device-plugins/kubelet_internal_checkpoint` 后重启 kubelet

---

## 版本兼容性注意事项

| 版本 | 变更 | 诊断影响 |
|------|------|----------|
| v1.26 | DRA (Dynamic Resource Allocation) Alpha | 新增 ResourceClaim 机制，传统 Device Plugin 仍可用 |
| v1.28 | DRA 改进（Structured Parameters） | ResourceClass 配置方式变化 |
| v1.31 | DRA 进入 Beta | 逐步替代 Device Plugin 的长期方向 |
| v1.32 | DRA 多设备支持 | 支持跨设备类型的复合分配 |

> [存疑：DRA 在 v1.31 Beta 后是否默认启用 Feature Gate `DynamicResourceAllocation`，需确认各发行版默认配置]

---

## 相关链接

- [[26-技能/03-节点/node/03-node-component-troubleshooting.md|节点组件故障排查]]
- [[26-技能/03-节点/node/01-node-notready-diagnosis.md|Node NotReady 诊断]]
- [[26-技能/03-节点/node/04-node-sop-runbook.md|Node SOP 与 Runbook]]
- [[26-技能/03-节点/node/reference/node-version-differences.md|版本差异对比]]
- [[19-故障诊断/04-高级排障/structural-02-node-components/06-gpu-device-plugin-troubleshooting.md|GPU 排障原始文件]]
