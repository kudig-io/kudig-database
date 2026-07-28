---
title: GPU 异常故障树分析 (skills)
description: '<!-- condition: kubectl get nodes -o jsonpath=''{.items[*].status.capacity.nvidia.com/gpu}''
  返回 0 或 Pod 日志显示 CUDA_ERROR -->'
summary: '<!-- condition: kubectl get nodes -o jsonpath=''{.items[*].status.capacity.nvidia.com/gpu}''
  返回 0 或 Pod 日志显示 CUDA_ERROR -->'
category: skills
tags:
- k8s
- fta
- troubleshooting
- kubelet
- containerd
- daemonset
- gpu
- cuda
- nvidia
tier: supporting
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- GPU 异常故障树分析 是什么
- 如何 GPU 异常故障树分析
trigger_keywords:
- GPU
- 异常故障树分析
prerequisites:
- kubectl-basics
- gpu-scheduling-basics
fta_id: FTA-GPU-001
component: Gpu
severity: medium
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# GPU 异常故障树分析

<!-- condition: kubectl get nodes -o jsonpath='{.items[*].status.capacity.nvidia.com/gpu}' 返回 0 或 Pod 日志显示 CUDA_ERROR -->

# GPU 异常 FTA 树

## 适用范围与说明
- **目标**：覆盖 GPU 设备不可用、调度失败、驱动不兼容、运行时异常与资源碎片化的关键成因与路径。
- **范围**：Device Plugin、驱动/CUDA/cuDNN 兼容性、容器运行时（nvidia-container-runtime）、调度与拓扑、配额与资源管理、节点与硬件问题。
- **符号**：
  - **OR 门**：任一子事件成立即可触发父事件
  - **AND 门**：所有子事件同时成立才触发父事件

---

## Mermaid FTA 树

```mermaid
flowchart TD
  TE["顶事件: GPU 异常<br/>GPU 不可用 / 调度失败 / 训练中断"]
  OR0{{OR}}
  TE --> OR0

  %% ======== 一级分类 ========
  OR0 --> CAT_DEV["A. Device Plugin 异常"]
  OR0 --> CAT_DRV["B. 驱动/CUDA 兼容性异常"]
  OR0 --> CAT_SCHED["C. 调度与拓扑异常"]
  OR0 --> CAT_RT["D. 容器运行时/GPU 运行时异常"]
  OR0 --> CAT_RES["E. 资源配额与碎片化"]
  OR0 --> CAT_HW["F. 节点/硬件问题"]

  %% ======== A. Device Plugin ========
  A_OR{{OR}}
  CAT_DEV --> A_OR
  A_OR --> A1["A1. Device Plugin Pod 崩溃<br/>DaemonSet 异常"]
  A_OR --> A2["A2. GPU 设备未注册<br/>capacity 为 0"]
  A_OR --> A3["A3. Device Plugin Socket 断连<br/>kubelet 通信失败"]
  A_OR --> A4_AND["A4. GPU 设备不可见<br/>(AND 门)"]

  A4_AND_GATE{{"AND"}}
  A4_AND --> A4_AND_GATE
  A4_AND_GATE --> A4C1["NVIDIA 驱动未加载"]
  A4_AND_GATE --> A4C2["Device Plugin 已启动但检测不到设备"]

  %% ======== B. 驱动/CUDA ========
  B_OR{{OR}}
  CAT_DRV --> B_OR
  B_OR --> B1["B1. NVIDIA 驱动版本不匹配<br/>内核模块加载失败"]
  B_OR --> B2["B2. CUDA 版本不兼容<br/>应用 CUDA > 驱动支持"]
  B_OR --> B3["B3. cuDNN/NCCL 版本冲突<br/>库链接失败"]
  B_OR --> B4["B4. 驱动升级后 GPU 异常<br/>模块热加载失败"]

  %% ======== C. 调度与拓扑 ========
  C_OR{{OR}}
  CAT_SCHED --> C_OR
  C_OR --> C1["C1. 节点标签/污点不匹配<br/>nodeSelector/toleration 缺失"]
  C_OR --> C2["C2. GPU 资源碎片化<br/>单节点剩余 GPU 不满足请求"]
  C_OR --> C3["C3. 拓扑亲和性冲突<br/>跨 NUMA/NVLink 调度"]
  C_OR --> C4_AND["C4. 调度完全阻塞<br/>(AND 门)"]

  C4_AND_GATE{{"AND"}}
  C4_AND --> C4_AND_GATE
  C4_AND_GATE --> C4C1["所有 GPU 节点资源已占满"]
  C4_AND_GATE --> C4C2["Cluster Autoscaler 无法扩容 GPU 节点"]

  %% ======== D. 容器运行时 ========
  D_OR{{OR}}
  CAT_RT --> D_OR
  D_OR --> D1["D1. nvidia-container-runtime 未配置<br/>runtimeClass 缺失"]
  D_OR --> D2["D2. GPU 设备挂载失败<br/>/dev/nvidia* 不可访问"]
  D_OR --> D3["D3. NVIDIA Container Toolkit 版本不兼容"]
  D_OR --> D4["D4. containerd 配置缺失<br/>nvidia runtime handler 未注册"]

  %% ======== E. 资源配额 ========
  E_OR{{OR}}
  CAT_RES --> E_OR
  E_OR --> E1["E1. ResourceQuota 限制<br/>nvidia.com/gpu 达上限"]
  E_OR --> E2["E2. GPU 请求/限制不一致<br/>requests ≠ limits"]
  E_OR --> E3["E3. GPU 共享/虚拟化异常<br/>vGPU/MIG 配置错误"]
  E_OR --> E4_AND["E4. GPU 利用率低但分配满<br/>(AND 门)"]

  E4_AND_GATE{{"AND"}}
  E4_AND --> E4_AND_GATE
  E4_AND_GATE --> E4C1["GPU 已全部分配（allocatable = 0）"]
  E4_AND_GATE --> E4C2["实际 GPU 利用率极低"]

  %% ======== F. 节点/硬件 ========
  F_OR{{OR}}
  CAT_HW --> F_OR
  F_OR --> F1["F1. GPU 硬件问题<br/>ECC 错误 / Xid 错误"]
  F_OR --> F2["F2. GPU 温度过高<br/>降频/节流"]
  F_OR --> F3["F3. PCIe 链路异常<br/>带宽降级"]
  F_OR --> F4["F4. GPU 挂死<br/>需硬件重置"]
```

---

## 生产级观测与证据

| 类别 | 关键信号 |
|------|---------|
| **事件** | `FailedScheduling (Insufficient nvidia.com/gpu)`、Device Plugin Pod 重启事件、`nvidia-smi` Xid 错误事件 |
| **关键指标** | `kube_node_status_

## 生产案例

### 案例 1: NVIDIA Device Plugin 崩溃导致 GPU Pod 无法调度

| 时间 | 事件 |
|------|------|
| 15:00 | 新提交的训练任务 Pod Pending，提示 "0/10 nodes: Insufficient nvidia.com/gpu" |
| 15:05 | `kubectl get nodes -o json | jq '.items[].status.allocatable["nvidia.com/gpu"]'` 显示所有节点为 0 |
| 15:10 | `kubectl get pods -n kube-system -l name=nvidia-device-plugin-ds` 显示 CrashLoopBackOff |
| 15:15 | 日志显示 "Failed to initialize NVML: could not load NVML library" |
| 15:20 | 🔴 重启 nvidia-device-plugin DaemonSet，GPU 资源重新注册 |

**根因**: 节点驱动升级后未重启 device-plugin，NVML 库路径变更。

### 案例 2: GPU 显存 OOM 导致训练任务反复重启

**现象**: Pod 运行 10min 后 OOMKilled，但 `kubectl describe pod` 显示 memory limit 未超。

**诊断**: `nvidia-smi` 显示 GPU 显存 100%，容器内 `CUDA_VISIBLE_DEVICES` 配置正确但 batch size 过大

**修复**: 🟢 调整训练参数 batch_size，或设置 `nvidia.com/gpu-memory` 资源限制

## 升级决策点

| 级别 | 条件 | 动作 |
|------|------|------|
| P0 | 全集群 GPU 资源不可用 | 检查 device-plugin + 驱动状态 |
| P1 | 单节点 GPU 异常 | 检查该节点 nvidia-smi + 驱动 |
| P2 | GPU 利用率低 | 优化调度策略和资源共享 |

## 面试要点

1. **Q: Kubernetes GPU 调度的工作原理？**
   A: NVIDIA Device Plugin 以 DaemonSet 运行，通过 NVML 发现 GPU 并注册为扩展资源(nvidia.com/gpu)，kubelet 通过 gRPC 与 plugin 通信分配设备，容器通过 /dev/nvidia* 设备文件访问 GPU。

2. **Q: GPU 共享的几种方案？**
   A: ① 时间片共享(Time-slicing): 多 Pod 共享同一 GPU，无隔离 ② MIG(Multi-Instance GPU): A100 硬件分区，强隔离 ③ vGPU: 虚拟化层切分 ④ MPS: CUDA 多进程服务。

3. **Q: 如何监控 GPU 资源使用？**
   A: DCGM Exporter 暴露 Prometheus 指标(gpu_temp/gpu_utilization/gpu_memory_used)，配合 Grafana 仪表盘，设置显存使用率 >90% 告警。

## 相关链接

- [[26-技能/04-工作负载/pod/方法论/FTA Methodology and Core Principles.md|FTA 方法论]]
- [[26-技能/04-工作负载/pod/方法论/FTA Diagnostic Execution Engine.md|[[19-故障诊断/06-FTA故障树/fta-execution-engine|FTA 诊断执行引擎]]]]
- [[26-技能/03-节点/node/诊断排障/ts-node-components.md|节点组件排查]]

## Related

- [[26-技能/04-工作负载/pod/方法论/Symptom Vector Matching Engine.md|[[Symptom Vector Matching Engine|Symptom Vector Matching Engine]]]] — Symptom Vector Matching Engine
- [[26-技能/04-工作负载/pod/方法论/skill-reference-root-cause-catalog.md|skill-reference-root-cause-catalog]] — Root Cause Catalog
- [[23-实体/02-K8s核心组件/container-runtime.md|container-runtime]] — Container Runtime
- [[23-实体/02-K8s核心组件/kubelet.md|kubelet]] — kubelet
- [[containerd]] — containerd

- [[19-故障诊断/06-FTA故障树/list/gpu-fta.md|GPU 异常故障树分析]]
- [[26-技能/04-工作负载/pod/培训/测验/assessment-daily-check-quiz.md|Daily Check Quiz]] — Cross-reference


<!-- risk-assessed -->
