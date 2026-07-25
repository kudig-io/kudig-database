---
title: "GPU Operator × Device Plugin × CDI 生态交叉"
summary: "NVIDIA GPU Operator、Device Plugin 和 CDI 规范如何协同构建 GPU 容器化生态，以及三者的职责边界与演进方向"
category: synthesis
tags:
- gpu
- gpu-operator
- device-plugin
- cdi
- nvidia
- container-runtime
tier: supporting
sources:
- 容器运行时/containerd-CRI-O/16-gpu-runtime-nvidia-cdi.md
- 综合/gpu-scheduling-cost.md
- 平台工程/治理/18-gpu-cluster-governance-ai-platform.md
created: '2026-07-19'
updated: '2026-07-19'
last_updated: '2026-07-19'
---

# GPU Operator × Device Plugin × CDI 生态交叉

## The Connection

GPU 容器化是 AI 基础设施的技术基石，而 NVIDIA GPU Operator、Device Plugin 和 CDI（Container Device Interface）构成了这一基石的三层架构。理解三者的职责边界和协作关系，是正确部署和运维 GPU 集群的前提。

**GPU Operator** 是顶层编排器：它通过 Kubernetes Operator 模式自动管理 GPU 节点上的所有 NVIDIA 软件栈——驱动安装、Container Toolkit 配置、Device Plugin 部署、DCGM Exporter 监控、MIG 配置。没有 GPU Operator，运维人员需要手动在每个节点上安装驱动、配置 containerd runtime hook、部署 device plugin，这在大规模集群中是不可接受的。

**Device Plugin** 是资源注册器：它实现 Kubernetes Device Plugin API（gRPC），向 kubelet 注册 `nvidia.com/gpu` 扩展资源，报告可用 GPU 数量，并在 Pod 调度到节点时执行设备分配（设置 `NVIDIA_VISIBLE_DEVICES` 环境变量、挂载 `/dev/nvidia*` 设备文件）。Device Plugin 是 kubelet 与 GPU 硬件之间的桥梁。

**CDI（Container Device Interface）** 是未来标准：它定义了设备厂商向容器运行时声明设备的标准化格式（JSON/YAML spec 文件），替代 NVIDIA 专有的 runtime hook 机制。CDI 的目标是让 containerd/CRI-O 原生理解"这个容器需要 GPU"，而不需要厂商特定的 prestart hook。

三者的关系是：GPU Operator 管理 Device Plugin 的生命周期，Device Plugin 当前通过 runtime hook 实现设备注入，而 CDI 正在逐步替代 runtime hook 成为设备注入的标准方式。GPU Operator 从 v1.14 开始支持 CDI 模式，标志着这一演进方向。^[inferred]

## Where They Co-occur

- **GPU 节点初始化**：GPU Operator 的 nvidia-driver-daemonset 安装驱动 → nvidia-container-toolkit-daemonset 配置 runtime → nvidia-device-plugin-daemonset 注册资源 → dcgm-exporter 启动监控。四个组件按依赖顺序启动，任何一个失败都会导致 GPU 不可用。

- **Pod 调度到 GPU 节点**：kubelet 通过 Device Plugin API 查询可用 GPU → 调度器分配 `nvidia.com/gpu: 1` → kubelet 调用 Device Plugin 的 `Allocate()` → Device Plugin 通过 CDI spec 或 runtime hook 注入设备 → 容器内可见 GPU。

- **MIG 切分场景**：GPU Operator 的 mig-manager 组件执行 MIG 切分 → Device Plugin 以 `mixed` 或 `single` 策略暴露 MIG 实例为独立资源（`nvidia.com/mig-2g.20gb`）→ CDI spec 为每个 MIG 实例生成独立设备声明 → 调度器按 MIG 资源分配。

- **驱动升级**：GPU Operator 滚动更新 driver-daemonset → 逐节点排空 → 卸载旧驱动 → 安装新驱动 → 重新生成 CDI spec → 重启 containerd → Device Plugin 重新注册 → 节点恢复调度。

- **多厂商设备**：CDI 的标准化意味着 AMD（ROCm）、Intel（Gaudi）可以使用相同的 CDI 接口声明设备，而不需要各自实现 Device Plugin + runtime hook。这是 CDI 相对 NVIDIA 专有方案的核心优势。

- **DRA（Dynamic Resource Allocation）演进**：K8s 1.30+ 的 DRA API 将进一步替代 Device Plugin 的静态资源注册模式，允许更灵活的设备分配（如"给我一块显存 > 40GB 的 GPU"）。CDI 是 DRA 的设备描述层，Device Plugin 是过渡方案。^[inferred]

## Cross-cutting Insight

GPU 容器化生态的演进方向是**标准化和解耦**。早期（nvidia-docker 时代），GPU 容器化是 NVIDIA 专有的 hack：修改 Docker daemon 配置、注入 prestart hook、硬编码设备路径。当前（GPU Operator + Device Plugin），虽然仍是 NVIDIA 主导，但已通过 Kubernetes 标准 API（Device Plugin、DaemonSet）实现了可管理性。未来（CDI + DRA），设备管理将完全标准化：任何设备厂商通过 CDI spec 声明设备，K8s 通过 DRA 按需分配，容器运行时原生理解设备需求——GPU 将不再是"特殊资源"，而是与网卡、FPGA 一样的标准设备。

但从工程实践角度，这一演进是渐进的：生产环境仍然以 GPU Operator + Device Plugin 为主（成熟、稳定），CDI 作为可选增强（减少 hook 依赖），DRA 仍在 alpha/beta 阶段。平台团队的策略应该是：当前用 GPU Operator 管理全栈，启用 CDI 模式减少技术债务，关注 DRA 进展为未来迁移做准备。^[inferred]

## Tensions and Trade-offs

| 张力 | 选择 A | 选择 B | 建议 |
|------|--------|--------|------|
| 管理方式 | GPU Operator（全托管） | 手动安装（精细控制） | 生产用 Operator，特殊场景手动 |
| 设备注入 | Runtime Hook（成熟） | CDI（标准化） | 新部署启用 CDI，存量保持 Hook |
| 资源注册 | Device Plugin（静态） | DRA（动态） | 当前用 DP，关注 DRA GA |
| MIG 策略 | single（纯 MIG） | mixed（整卡+MIG） | 推理用 mixed，训练用 single |
| 驱动管理 | Operator 管理（自动） | 节点镜像预装（稳定） | 大规模用 Operator，安全敏感用预装 |

## Practical Patterns

```yaml
# 🟢 低风险：验证 GPU 生态组件状态
# 检查 GPU Operator 全栈
kubectl get clusterpolicy -o wide
kubectl get pods -n gpu-operator -o custom-columns=\
NAME:.metadata.name,STATUS:.status.phase,NODE:.spec.nodeName

# 检查 Device Plugin 注册
kubectl get nodes -o custom-columns=\
NAME:.metadata.name,GPU:.status.allocatable.nvidia\\.com/gpu,MIG:.status.allocatable.nvidia\\.com/mig-2g\\.20gb

# 检查 CDI spec
# 在 GPU 节点上：
ls /etc/cdi/ /var/run/cdi/
cat /etc/cdi/nvidia.yaml | head -20

# 验证端到端
kubectl run gpu-test --image=nvcr.io/nvidia/cuda:12.3.0-base-ubuntu22.04 \
  --restart=Never --limits='nvidia.com/gpu=1' -- nvidia-smi
kubectl logs gpu-test
kubectl delete pod gpu-test
```

## Related

- [[14-容器运行时/03-containerd-CRI-O/16-gpu-runtime-nvidia-cdi|GPU 运行时：NVIDIA Container Toolkit 与 CDI]]
- [[24-综合/01-AI与机器学习/gpu-scheduling-cost|GPU Scheduling × Cost Optimization]]
- [[10-平台工程/03-治理/18-gpu-cluster-governance-ai-platform|GPU 集群治理]]
- [[24-综合/01-AI与机器学习/training-inference-data-lifecycle|训练 × 推理 × 数据生命周期]]
- [[14-容器运行时/03-containerd-CRI-O/12-container-shim-v2|containerd shim v2 架构]]
- [[15-AI基础设施/05-K8s-AI基础设施|K8s AI 基础设施]]
