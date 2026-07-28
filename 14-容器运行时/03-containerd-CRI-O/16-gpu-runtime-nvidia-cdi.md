---
title: "GPU 运行时：NVIDIA Container Toolkit 与 CDI 规范"
description: "NVIDIA GPU 容器运行时架构、CDI 设备规范、GPU 隔离、MIG 配置及故障排查"
summary: "系统讲解 NVIDIA Container Toolkit 的运行时 hook 机制、CDI（Container Device Interface）规范、GPU 容器隔离策略、MIG（Multi-Instance GPU）容器配置及生产环境 GPU 故障排查方法"
category: 容器运行时
tags:
- gpu
- nvidia
- cdi
- mig
- cuda
- device-plugin
- container-toolkit
tier: core
created: '2026-07-19'
last_updated: 2026-07
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 平台工程师
- AI 基础设施工程师
estimated_read_time: 20min
intent_queries:
- "如何配置 NVIDIA GPU 容器运行时"
- "CDI 规范是什么"
- "MIG 容器如何配置"
trigger_keywords:
- nvidia
- gpu
- cdi
- mig
- cuda
- container-toolkit
- device-plugin
prerequisites:
- kubectl-basics
- containerd-basics
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
authors:
- name: KUDIG Team
  role: contributor
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。

# GPU 运行时：NVIDIA Container Toolkit 与 CDI

## 概述

GPU 容器化是 AI 基础设施的核心技术挑战。与 CPU 不同，GPU 设备需要专用的驱动栈、运行时 hook 和设备管理接口才能在容器中正确使用。NVIDIA Container Toolkit（原 nvidia-docker2）通过在 OCI 运行时规范中注入 prestart hook，将 GPU 设备文件、驱动库和 CUDA 运行时透明地挂载到容器内部。

CDI（Container Device Interface）是 CNCF 正在推进的设备管理标准化规范，旨在替代厂商特定的运行时 hook 机制，为所有设备厂商（NVIDIA、AMD、Intel）提供统一的容器设备接入接口。MIG（Multi-Instance GPU）则是 NVIDIA Ampere/Hopper 架构的硬件级 GPU 切分技术，允许将一块物理 GPU 切分为最多 7 个隔离的计算实例。

## 核心概念

### NVIDIA Container Toolkit 架构

```
Pod Spec (nvidia.com/gpu: 1)
    ↓
kubelet → CRI → containerd/CRI-O
    ↓
OCI Runtime (runc/crun)
    ↓ prestart hook
nvidia-container-runtime-hook
    ↓
libnvidia-container（检测 GPU、挂载设备和库）
    ↓
容器内可见：/dev/nvidia0, /usr/lib/x86_64-linux-gnu/libcuda.so, ...
```

核心组件：
- **nvidia-container-toolkit**：包含运行时 hook 和 CLI 工具
- **libnvidia-container**：底层库，负责 GPU 设备发现和挂载
- **nvidia-container-runtime**：OCI runtime wrapper，在 runc 前注入 hook
- **nvidia-ctk**：管理工具，配置 containerd/CRI-O 的 runtime handler

### CDI（Container Device Interface）规范

CDI 定义了设备厂商向容器运行时声明设备的标准格式：

```json
{
  "cdiVersion": "0.7.0",
  "kind": "nvidia.com/gpu",
  "devices": [
    {
      "name": "gpu0",
      "containerEdits": {
        "deviceNodes": [
          {"path": "/dev/nvidia0", "type": "c", "major": 195, "minor": 0}
        ],
        "mounts": [
          {"containerPath": "/usr/lib/libcuda.so", "hostPath": "/usr/lib/libcuda.so.535.129.03"}
        ],
        "env": ["NVIDIA_VISIBLE_DEVICES=GPU-xxxx-xxxx"]
      }
    }
  ]
}
```

CDI 的优势：
- 标准化：不依赖厂商特定的 runtime hook
- 声明式：设备信息以 JSON/YAML 文件存储在 `/etc/cdi/` 或 `/var/run/cdi/`
- 运行时无关：containerd、CRI-O、Podman 均支持
- 动态更新：设备热插拔时更新 CDI spec 文件即可

### MIG 架构

```
物理 GPU（如 A100 80GB）
├── MIG Instance 1: 1g.10gb（1/7 算力，10GB 显存）
├── MIG Instance 2: 2g.20gb（2/7 算力，20GB 显存）
├── MIG Instance 3: 3g.40gb（3/7 算力，40GB 显存）
└── MIG Instance 4: 1g.10gb（1/7 算力，10GB 显存）

# MIG Profile 组合受硬件限制（几何约束）
# A100 80GB 有效组合：7x1g.10gb / 3x2g.20gb+1x1g.10gb / 1x7g.80gb 等
```

### GPU 共享技术对比

| 技术 | 隔离级别 | 粒度 | 性能开销 | 适用场景 |
|------|---------|------|---------|---------|
| 独占（Exclusive） | 完全隔离 | 整卡 | 无 | 训练任务 |
| MIG | 硬件隔离 | 1/7 卡 | 极低（<2%） | 多租户推理 |
| MPS | 进程级共享 | 任意比例 | 低（5-10%） | 小模型推理 |
| Time-slicing | 时间片轮转 | 任意比例 | 中（上下文切换） | 开发/测试 |
| vGPU（HAMi） | 软件隔离 | 算力+显存 | 中 | 多租户通用 |

## 生产部署

### NVIDIA GPU Operator 部署

```yaml
# 🟡 中风险：部署 GPU Operator（管理驱动、toolkit、device-plugin）
# helm install gpu-operator nvidia/gpu-operator -n gpu-operator --create-namespace \
#   --set driver.version=535.129.03 \
#   --set toolkit.enabled=true \
#   --set devicePlugin.enabled=true \
#   --set dcgmExporter.enabled=true \
#   --set mig.strategy=mixed

apiVersion: nvidia.com/v1
kind: ClusterPolicy
metadata:
  name: gpu-operator-policy
spec:
  driver:
    enabled: true
    version: "535.129.03"
    repository: nvcr.io/nvidia
    image: driver
  toolkit:
    enabled: true
    version: "v1.14.3-ubi8"
  devicePlugin:
    enabled: true
    config:
      name: gpu-plugin-config
      default: default
  dcgmExporter:
    enabled: true
    version: "3.3.5-3.4.1-ubuntu22.04"
  mig:
    strategy: mixed  # mixed: 同时暴露整卡和 MIG 实例
  validator:
    enabled: true
```

### CDI 模式配置（推荐）

```bash
# 🟡 中风险：配置 CDI 模式替代传统 runtime hook
# 在 GPU 节点上启用 CDI
sudo nvidia-ctk cdi generate --output=/etc/cdi/nvidia.yaml

# 验证 CDI spec
cat /etc/cdi/nvidia.yaml
# 应包含所有 GPU 设备的声明

# 配置 containerd 使用 CDI
sudo nvidia-ctk runtime configure --runtime=containerd --cdi.enabled=true
sudo systemctl restart containerd

# 验证 CDI 设备可用
sudo nvidia-ctk cdi list
# 输出：nvidia.com/gpu=gpu0, nvidia.com/gpu=gpu1, ...
```

```yaml
# 🟢 低风险：使用 CDI 注解请求 GPU 的 Pod
apiVersion: v1
kind: Pod
metadata:
  name: gpu-workload-cdi
  annotations:
    cdi.k8s.io/gpu: "nvidia.com/gpu=gpu0"
spec:
  containers:
  - name: cuda-app
    image: nvcr.io/nvidia/cuda:12.3.0-base-ubuntu22.04
    command: ["nvidia-smi"]
    resources:
      limits:
        nvidia.com/gpu: 1
```

### MIG 容器配置

```bash
# 🔴 高风险：启用 MIG 需要重置 GPU，会中断所有 GPU 工作负载
# 1. 排空 GPU 节点
kubectl drain gpu-node-01 --ignore-daemonsets --delete-emptydir-data

# 2. 启用 MIG 模式
sudo nvidia-smi -i 0 -mig 1

# 3. 创建 MIG 实例（以 A100 80GB 为例）
# 创建 3 个 2g.20gb + 1 个 1g.10gb 实例
sudo nvidia-smi mig -i 0 -cgi 14,14,14,9 -C

# 4. 验证 MIG 实例
nvidia-smi mig -i 0 -lgi
nvidia-smi mig -i 0 -lci

# 5. 重新生成 CDI spec（包含 MIG 设备）
sudo nvidia-ctk cdi generate --output=/etc/cdi/nvidia.yaml

# 6. 恢复节点调度
kubectl uncordon gpu-node-01
```

```yaml
# 🟢 低风险：请求 MIG 实例的 Pod
apiVersion: v1
kind: Pod
metadata:
  name: mig-inference
spec:
  containers:
  - name: inference
    image: registry.example.com/inference-server:v2
    resources:
      limits:
        nvidia.com/mig-2g.20gb: 1  # 请求一个 2g.20gb MIG 实例
---
# GPU Device Plugin 配置（暴露 MIG 资源）
apiVersion: v1
kind: ConfigMap
metadata:
  name: gpu-plugin-config
  namespace: gpu-operator
data:
  config.yaml: |
    version: v1
    flags:
      migStrategy: mixed
    sharing:
      timeSlicing:
        resources:
        - name: nvidia.com/gpu
          replicas: 1
```

### GPU 节点健康检查 DaemonSet

```yaml
# 🟢 低风险：GPU 健康检查
apiVersion: apps/v1
kind: DaemonSet
metadata:
  name: gpu-health-check
  namespace: gpu-operator
spec:
  selector:
    matchLabels:
      app: gpu-health-check
  template:
    metadata:
      labels:
        app: gpu-health-check
    spec:
      nodeSelector:
        nvidia.com/gpu.present: "true"
      containers:
      - name: dcgm-exporter
        image: nvcr.io/nvidia/k8s/dcgm-exporter:3.3.5-3.4.1-ubuntu22.04
        ports:
        - name: metrics
          containerPort: 9400
        securityContext:
          runAsNonRoot: false
          runAsUser: 0
        volumeMounts:
        - name: dev
          mountPath: /dev
        resources:
          requests:
            cpu: 50m
            memory: 64Mi
          limits:
            cpu: 200m
            memory: 128Mi
      volumes:
      - name: dev
        hostPath:
          path: /dev
```

## 运维操作

### GPU 状态检查

```bash
# 🟢 低风险：GPU 状态查看
# 节点级 GPU 信息
nvidia-smi
nvidia-smi -q -d UTILIZATION,MEMORY,TEMPERATURE,POWER

# 查看 GPU 进程
nvidia-smi pmon -c 1

# 检查 GPU 拓扑（NVLink/PCIe）
nvidia-smi topo -m

# K8s 层面检查 GPU 资源
kubectl describe node gpu-node-01 | grep -A5 "Allocated resources"
kubectl get nodes -l nvidia.com/gpu.present=true -o custom-columns=NAME:.metadata.name,GPU:.status.allocatable.nvidia\\.com/gpu

# 检查 GPU Operator 组件状态
kubectl get pods -n gpu-operator
kubectl get clusterpolicy
```

### GPU 驱动升级

```bash
# 🔴 高风险：驱动升级需要重启节点，中断所有 GPU 工作负载
# 使用 GPU Operator 升级驱动
kubectl patch clusterpolicy gpu-operator-policy \
  --type merge -p '{"spec":{"driver":{"version":"550.54.14"}}}'

# 观察升级进度
kubectl get pods -n gpu-operator -w
# 等待 nvidia-driver-daemonset 滚动更新完成

# 验证新驱动
kubectl exec -n gpu-operator -it ds/nvidia-driver-daemonset -- nvidia-smi

# 手动升级（非 Operator 管理）
sudo systemctl stop nvidia-persistenced
sudo apt-get install -y nvidia-driver-550
sudo reboot
```

### GPU 故障恢复

```bash
# 🔴 高风险：GPU 重置会中断该 GPU 上的所有工作负载
# 检测 GPU 错误
nvidia-smi -q -d ECC,RETIRED_PAGES

# 重置单个 GPU（不影响其他 GPU）
sudo nvidia-smi -i 0 -r

# 如果 GPU 进入不可恢复状态
sudo nvidia-smi -i 0 --gpu-reset

# 标记节点不可调度（GPU 硬件故障）
kubectl cordon gpu-node-01
kubectl taint nodes gpu-node-01 nvidia.com/gpu=hardware-failure:NoSchedule

# 驱逐 GPU Pod 到健康节点
kubectl drain gpu-node-01 --ignore-daemonsets --delete-emptydir-data
```

## 故障排查

### 常见 GPU 容器问题

```bash
# 🟢 低风险：GPU 容器问题诊断
# 问题 1：Pod 报 "UnexpectedAdmissionError: requested resource nvidia.com/gpu not registered"
# 原因：device-plugin 未运行或未就绪
kubectl get pods -n gpu-operator -l app=nvidia-device-plugin-daemonset
kubectl logs -n gpu-operator -l app=nvidia-device-plugin-daemonset --tail=50

# 问题 2：容器内 nvidia-smi 报 "Failed to initialize NVML"
# 原因：驱动库未正确挂载
kubectl exec -it gpu-pod -- ls /usr/lib/x86_64-linux-gnu/libcuda*
kubectl exec -it gpu-pod -- cat /proc/driver/nvidia/version

# 问题 3：CUDA OOM
# 检查显存使用
kubectl exec -it gpu-pod -- nvidia-smi --query-gpu=memory.used,memory.total --format=csv
# 检查是否有显存泄漏
kubectl exec -it gpu-pod -- nvidia-smi pmon -c 5

# 问题 4：GPU Xid 错误
dmesg | grep -i "NVRM: Xid"
# Xid 79: GPU fallen off bus（硬件故障）
# Xid 48: Double Bit ECC Error（显存错误）
# Xid 31: GPU memory page fault（驱动/应用 bug）

# 问题 5：MIG 实例创建失败
sudo nvidia-smi mig -i 0 -lgi
# 检查是否有残留 MIG 实例
sudo nvidia-smi mig -i 0 -dgi -f
```

### GPU Operator 问题排查

```bash
# 🟢 低风险：GPU Operator 诊断
# 检查 ClusterPolicy 状态
kubectl describe clusterpolicy gpu-operator-policy

# 检查各组件日志
kubectl logs -n gpu-operator -l app=nvidia-driver-daemonset --tail=100
kubectl logs -n gpu-operator -l app=nvidia-device-plugin-daemonset --tail=100
kubectl logs -n gpu-operator -l app=nvidia-container-toolkit-daemonset --tail=100

# 检查 validator 结果
kubectl logs -n gpu-operator -l app=nvidia-operator-validator --tail=50

# 检查节点上的 containerd 配置
sudo cat /etc/containerd/config.toml | grep -A10 nvidia
```

### 性能问题排查

```bash
# 🟢 低风险：GPU 性能诊断
# 检查 GPU 利用率和功耗
nvidia-smi dmon -s u -c 10
# u: utilization, p: power, t: temperature

# 检查 PCIe 带宽
nvidia-smi -q -d PERFORMANCE

# 检查 NVLink 状态（多 GPU 训练）
nvidia-smi nvlink -s

# 检查 GPU 时钟频率（是否降频）
nvidia-smi -q -d CLOCK
# 如果 clocks.sm 远低于 max，可能是功耗/温度限制

# DCGM 诊断（全面 GPU 健康检查）
dcgmi diag -r 3  # level 3: 全面诊断（耗时较长）
```

## 最佳实践

### 生产环境配置

1. **使用 GPU Operator 管理驱动**：避免手动安装驱动导致版本不一致，Operator 确保驱动与 CUDA 版本匹配
2. **启用 CDI 模式**：CDI 是未来标准，比传统 runtime hook 更可靠、更易调试
3. **MIG 策略选择**：
   - 推理集群：MIG mixed 模式，按模型大小分配不同 MIG profile
   - 训练集群：禁用 MIG，使用整卡 + NVLink 拓扑感知调度
4. **DCGM Exporter 必装**：GPU 监控的基础，暴露利用率、显存、温度、ECC 错误等指标
5. **节点 Taint**：GPU 节点添加 `nvidia.com/gpu=present:NoSchedule` taint，防止非 GPU Pod 调度到昂贵节点

### 安全与隔离

1. **GPU 设备隔离**：每个容器只能看到分配给它的 GPU（通过 `NVIDIA_VISIBLE_DEVICES` 环境变量）
2. **MIG 硬件隔离**：MIG 实例间显存和计算完全隔离，适合多租户场景
3. **限制容器权限**：GPU 容器不需要 privileged 模式，仅需 `/dev/nvidia*` 设备访问
4. **驱动版本锁定**：生产环境锁定驱动版本，避免自动升级导致兼容性问题

### 与 AI 工作负载集成

- 训练任务配合 [[22-概念/07-调度与资源/gang-scheduling|Gang Scheduling]] 确保多 GPU Pod 同时调度
- 推理服务配合 [[24-综合/01-AI与机器学习/gpu-scheduling-cost|GPU 调度与成本优化]] 实现 MIG 共享
- GPU 监控集成到 [[23-实体/07-可观测性/prometheus|Prometheus]] + Grafana 看板
- 参考 [[15-AI基础设施/05-K8s-AI基础设施/index|K8s AI 基础设施]] 了解完整 AI 平台架构

## Related

- [[24-综合/01-AI与机器学习/gpu-operator-device-plugin-ecosystem|GPU Operator × Device Plugin × CDI 生态]]
- [[24-综合/01-AI与机器学习/gpu-scheduling-cost|GPU 调度与成本优化]]
- [[14-容器运行时/03-containerd-CRI-O/12-container-shim-v2|containerd shim v2 架构]]
- [[10-平台工程/03-治理/18-gpu-cluster-governance-ai-platform|GPU 集群治理]]
- [[15-AI基础设施/05-K8s-AI基础设施/index|K8s AI 基础设施]]
- [[17-系统基础/01-Linux/06-linux-performance-tuning|Linux 性能调优]]
