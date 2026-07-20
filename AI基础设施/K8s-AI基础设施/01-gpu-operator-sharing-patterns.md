---
title: "NVIDIA GPU Operator 与 GPU 共享模式生产部署"
description: "GPU Operator 完整部署与 MIG/Time-slicing/MPS/vGPU/CDI 共享模式生产实践"
summary: "深入解析 NVIDIA GPU Operator 架构与 Helm 部署，覆盖 MIG 切分、Time-slicing、MPS daemon、HAMi vGPU、CDI 新范式等 GPU 共享方案的生产配置与故障排查"
category: AI基础设施
tags:
- gpu-operator
- nvidia
- mig
- time-slicing
- mps
- vgpu
- cdi
- gpu-sharing
- helm
- device-plugin
tier: core
created: '2026-07-19'
last_updated: 2026-07
difficulty: advanced
reading_level: advanced
audience:
- AI 工程师
- MLOps 工程师
- SRE
estimated_read_time: 20min
intent_queries:
- "GPU Operator 如何部署到生产环境"
- "MIG 和 Time-slicing 如何选择"
- "GPU 共享方案对比与故障排查"
trigger_keywords:
- gpu-operator
- mig
- time-slicing
- mps
- vgpu
- cdi
- gpu-sharing
prerequisites:
- kubectl-basics
- helm-basics
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

# NVIDIA GPU Operator 与 GPU 共享模式生产部署

## 概述

在 Kubernetes 集群中管理 GPU 资源是 AI 基础设施的核心挑战。NVIDIA GPU Operator 通过自动化部署 GPU 驱动、Device Plugin、DCGM Exporter、MIG Manager 等组件，将原本需要逐节点手动配置的复杂流程简化为声明式管理。然而，GPU 资源的稀缺性要求我们必须在独占与共享之间找到平衡——MIG 提供硬件级隔离，Time-slicing 实现轻量级复用，MPS 提供进程级并发，vGPU 方案（如 HAMi）提供灵活的虚拟化切分，而 CDI 则代表了设备管理的下一代范式。

本文将系统性地覆盖 GPU Operator 的生产部署、各共享模式的配置方法、方案选型对比以及常见故障的排查路径。关于 GPU 调度的基础概念，参见 [[概念/gpu-scheduling-ai-workloads]]；GPU 监控相关内容参见 [[AI基础设施/基础设施/04-gpu-monitoring-dcgm]]。

## 架构与核心概念

### GPU Operator 组件架构

GPU Operator 本质上是一个 Kubernetes Operator，通过 NVIDIA Driver Installer（nvidia-driver-installer DaemonSet）管理驱动生命周期，通过 NVIDIA Device Plugin 向 kubelet 注册 GPU 设备，通过 DCGM Exporter 暴露 GPU 监控指标，通过 MIG Manager 管理多实例 GPU 的切分配置。

```
┌─────────────────────────────────────────────────────────┐
│                    GPU Operator (Controller)              │
├─────────────────────────────────────────────────────────┤
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐  │
│  │ Driver       │  │ Device       │  │ DCGM         │  │
│  │ Installer    │  │ Plugin       │  │ Exporter     │  │
│  │ (DaemonSet)  │  │ (DaemonSet)  │  │ (DaemonSet)  │  │
│  └──────────────┘  └──────────────┘  └──────────────┘  │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐  │
│  │ MIG Manager  │  │ GPU Feature  │  │ Validator    │  │
│  │ (DaemonSet)  │  │ Discovery    │  │ (Job)        │  │
│  └──────────────┘  └──────────────┘  └──────────────┘  │
└─────────────────────────────────────────────────────────┘
```

### GPU 共享模式概览

| 共享模式 | 隔离级别 | 适用场景 | 性能开销 | 支持 GPU |
|---------|---------|---------|---------|---------|
| MIG | 硬件级（独立 SM/显存/带宽） | 多租户推理、SLA 保障 | 极低 | A100/H100/H200 |
| Time-slicing | 时间片轮转（无隔离） | 开发测试、轻量推理 | 低（上下文切换） | 所有 NVIDIA GPU |
| MPS | 进程级并发（共享 SM） | 小模型并发推理 | 极低 | Volta+ |
| vGPU (HAMi) | 软件虚拟化（显存/算力配额） | 多租户、细粒度分配 | 低-中 | 所有 NVIDIA GPU |
| CDI | 设备抽象层（取决于后端） | 异构设备统一管理 | 取决于实现 | 所有 CDI 兼容设备 |

## 生产部署

### Helm 部署 GPU Operator

🟡 中风险：会修改集群状态，安装多个 DaemonSet 和系统级组件。

```bash
# 添加 NVIDIA Helm 仓库
helm repo add nvidia https://helm.ngc.nvidia.com/nvidia
helm repo update

# 生产环境部署（指定驱动版本、启用 DCGM、配置 tolerations）
helm install gpu-operator nvidia/gpu-operator \
  --namespace gpu-operator --create-namespace \
  --set driver.version=550.90.07 \
  --set driver.repository=nvcr.io/nvidia \
  --set dcgmExporter.enabled=true \
  --set migManager.enabled=true \
  --set devicePlugin.config.name=time-slicing-config \
  --set validator.cuda.enabled=true \
  --set toolkit.enabled=true \
  --set operator.defaultRuntime=containerd \
  --set daemonsets.tolerations[0].key=nvidia.com/gpu \
  --set daemonsets.tolerations[0].operator=Exists \
  --set daemonsets.tolerations[0].effect=NoSchedule \
  --version v24.3.0
```

### 验证 GPU Operator 部署状态

🟢 低风险/只读：信息收集，无副作用。

```bash
# 检查所有 GPU Operator 组件状态
kubectl get pods -n gpu-operator -o wide

# 验证 GPU 节点标签（GPU Feature Discovery）
kubectl get nodes -l nvidia.com/gpu.present=true -o custom-columns=\
NAME:.metadata.name,\
GPU_PRODUCT:.metadata.labels.nvidia\\.com/gpu\\.product,\
GPU_COUNT:.metadata.labels.nvidia\\.com/gpu\\.count,\
GPU_MEMORY:.metadata.labels.nvidia\\.com/gpu\\.memory

# 验证 Device Plugin 注册
kubectl describe node <gpu-node> | grep -A 5 "Allocatable" | grep nvidia

# 运行 CUDA 验证 Pod
kubectl run cuda-test --rm -it --image=nvidia/cuda:12.4.0-base-ubuntu22.04 \
  --restart=Never -- nvidia-smi
```

### MIG 切分配置（A100/H100）

MIG（Multi-Instance GPU）允许将一块物理 GPU 切分为最多 7 个独立实例，每个实例拥有独立的 Streaming Multiprocessor、显存控制器和 L2 缓存。

🔴 高风险：MIG 切分会重置 GPU 上所有运行中的工作负载，必须在无业务负载时执行。

```yaml
# mig-config.yaml - MIG 实例配置文件
apiVersion: v1
kind: ConfigMap
metadata:
  name: mig-parted-config
  namespace: gpu-operator
data:
  config.yaml: |
    version: v1
    mig-configs:
      # A100 80GB: 切分为 2 个 3g.40gb + 1 个 1g.10gb
      a100-mixed:
        - devices: all
          mig-enabled: true
          mig-devices:
            "3g.40gb": 2
            "1g.10gb": 1
      # H100 80GB: 全部切分为 7 个 1g.10gb（最大并发）
      h100-all-1g:
        - devices: all
          mig-enabled: true
          mig-devices:
            "1g.10gb": 7
      # 禁用 MIG（恢复整卡模式）
      all-disabled:
        - devices: all
          mig-enabled: false
```

```bash
# 应用 MIG 配置（GPU Operator MIG Manager 会自动执行切分）
kubectl apply -f mig-config.yaml

# 🔴 高风险：切换 MIG 策略（会终止 GPU 上所有 Pod）
kubectl label nodes <gpu-node> nvidia.com/mig.config=a100-mixed --overwrite

# 验证 MIG 实例
kubectl exec -it <pod-with-gpu> -- nvidia-smi mig -lgi
kubectl exec -it <pod-with-gpu> -- nvidia-smi mig -lci
```

### Time-slicing ConfigMap 配置

Time-slicing 通过 NVIDIA 驱动的时间片机制让多个 Pod 共享同一块 GPU，无需硬件支持，适用于所有 NVIDIA GPU。

🟡 中风险：修改 Device Plugin 配置，需要重启相关 DaemonSet。

```yaml
# time-slicing-config.yaml
apiVersion: v1
kind: ConfigMap
metadata:
  name: time-slicing-config
  namespace: gpu-operator
data:
  any: |
    version: v1
    sharing:
      timeSlicing:
        renameByDefault: false
        failRequestsGreaterThanOne: false
        resources:
          - name: nvidia.com/gpu
            replicas: 4  # 每块 GPU 虚拟为 4 个可分配单元
  a100: |
    version: v1
    sharing:
      timeSlicing:
        resources:
          - name: nvidia.com/gpu
            replicas: 2
```

```bash
# 应用 Time-slicing 配置
kubectl apply -f time-slicing-config.yaml

# 重启 Device Plugin 使配置生效
kubectl rollout restart daemonset nvidia-device-plugin-daemonset -n gpu-operator

# 验证：节点可分配 GPU 数量应变为 replicas × 物理 GPU 数
kubectl describe node <gpu-node> | grep "nvidia.com/gpu"
```

### MPS Daemon 部署

MPS（Multi-Process Service）允许多个 CUDA 进程真正并发执行在同一 GPU 上，避免 Time-slicing 的上下文切换开销。

🟡 中风险：修改 GPU 运行时配置。

```yaml
# mps-config.yaml
apiVersion: v1
kind: ConfigMap
metadata:
  name: mps-config
  namespace: gpu-operator
data:
  any: |
    version: v1
    sharing:
      mps:
        renameByDefault: false
        resources:
          - name: nvidia.com/gpu
            replicas: 8  # MPS 支持更多并发进程
```

```bash
# 应用 MPS 配置
kubectl apply -f mps-config.yaml
kubectl label nodes <gpu-node> nvidia.com/mps.config=mps-config --overwrite

# 验证 MPS daemon 运行状态
kubectl exec -it <pod> -- echo get_server_list | nvidia-cuda-mps-control
```

### CDI（Container Device Interface）新范式

CDI 是 CNCF 维护的设备管理标准，旨在替代 Device Plugin 的局限性，提供更灵活的设备注入机制。GPU Operator v24.x 已支持 CDI 模式。

```yaml
# 启用 CDI 模式的 GPU Operator 配置
# helm values 片段
cdi:
  enabled: true
  default: true  # 将 CDI 设为默认设备注入方式
```

```bash
# 验证 CDI 设备注册
kubectl exec -it <pod> -- ls /etc/cdi/
kubectl exec -it <pod> -- cat /etc/cdi/nvidia.yaml
```

## 运维操作

### GPU Operator 升级

🔴 高风险：驱动升级会触发节点上所有 GPU Pod 重启。

```bash
# 升级前：排空 GPU 节点上的工作负载
kubectl drain <gpu-node> --ignore-daemonsets --delete-emptydir-data --timeout=300s

# 执行 Helm 升级
helm upgrade gpu-operator nvidia/gpu-operator \
  --namespace gpu-operator \
  --set driver.version=555.42.02 \
  --reuse-values \
  --version v24.6.0

# 监控驱动安装进度
kubectl logs -f daemonset/nvidia-driver-daemonset -n gpu-operator -c nvidia-driver-installer

# 恢复节点调度
kubectl uncordon <gpu-node>
```

### GPU 健康检查与自动修复

🟢 低风险/只读。

```bash
# 检查 GPU 健康状态（通过 DCGM）
kubectl exec -it <dcgm-exporter-pod> -n gpu-operator -- \
  dcgmi health -c -g 0

# 查看 GPU XID 错误（硬件故障指示）
kubectl logs <dcgm-exporter-pod> -n gpu-operator | grep "XID"

# 检查 GPU 拓扑（NVLink 连接状态）
kubectl exec -it <pod-with-gpu> -- nvidia-smi topo -m
```

## 故障排查

### GPU 不可见

```bash
# 🟢 Step 1: 检查 Device Plugin 状态
kubectl get pods -n gpu-operator -l app=nvidia-device-plugin-daemonset
kubectl logs -n gpu-operator -l app=nvidia-device-plugin-daemonset --tail=50

# 🟢 Step 2: 检查节点资源
kubectl describe node <node> | grep -A 10 "Capacity"

# 🟢 Step 3: 检查驱动是否正常加载
kubectl exec -it <pod> -- nvidia-smi
# 如果报 "Failed to initialize NVML"，检查 /dev/nvidia* 设备文件

# 🟢 Step 4: 检查 GPU Feature Discovery 标签
kubectl get node <node> -o jsonpath='{.metadata.labels}' | jq .
```

### MIG 切分失败

```bash
# 🟢 检查 MIG Manager 日志
kubectl logs -n gpu-operator -l app=nvidia-mig-manager-daemonset --tail=100

# 常见原因：
# 1. GPU 上仍有运行中的进程 → 先排空节点
# 2. MIG 配置不合法（实例组合超出 GPU 容量）→ 参考 nvidia-smi mig -lgip
# 3. 驱动版本不支持 → 确认驱动 >= 470

# 🟡 强制重置 MIG 状态
kubectl exec -it <pod> -- nvidia-smi mig -dci
kubectl exec -it <pod> -- nvidia-smi mig -dgi
```

### 驱动版本冲突

```bash
# 🟢 检查当前驱动版本
kubectl exec -it <pod> -- nvidia-smi --query-gpu=driver_version --format=csv

# 🟢 检查 GPU Operator 期望版本
kubectl get clusterpolicy -o yaml | grep driverVersion

# 🔴 如果存在 pre-installed 驱动与 Operator 管理驱动冲突
# 需要在节点上卸载手动安装的驱动后重新部署 Operator
```

## 最佳实践

1. **生产环境驱动管理**：始终通过 GPU Operator 管理驱动版本，避免手动安装导致版本漂移。使用 `driver.version` 固定版本号，禁止使用 `latest`。

2. **MIG 策略规划**：推理服务优先使用 MIG 获得硬件隔离保障；训练任务使用整卡模式获得最大带宽。通过 Node Label 区分 MIG 节点和整卡节点，配合 NodeSelector 调度。

3. **Time-slicing 适用边界**：仅用于开发测试或轻量推理场景。生产推理服务应使用 MIG 或独占 GPU，避免 Time-slicing 带来的性能不可预测性。

4. **监控先行**：部署 DCGM Exporter 并配置 Prometheus 告警规则，监控 GPU 利用率、显存使用、温度、XID 错误。参见 [[AI基础设施/基础设施/04-gpu-monitoring-dcgm]]。

5. **HAMi vGPU 方案**：对于需要细粒度显存配额但不具备 A100/H100 硬件 MIG 能力的集群，可考虑 HAMi（原 k8s-vGPU-scheduler）方案，它通过拦截 CUDA API 实现显存和算力的软隔离。

6. **CDI 迁移规划**：新集群建议直接启用 CDI 模式；存量集群可在 GPU Operator 升级时逐步迁移，CDI 与 Device Plugin 可共存。

7. **容量规划**：GPU 资源规划需结合业务增长预期，参见 [[可靠性/容量规划/]] 中的容量模型方法论。

## Related

- [[概念/gpu-scheduling-ai-workloads]]
- [[概念/dynamic-resource-allocation]]
- [[AI基础设施/基础设施/03-gpu-scheduling-management]]
- [[AI基础设施/基础设施/04-gpu-monitoring-dcgm]]
- [[故障诊断/]]
