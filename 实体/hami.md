---
title: HAMI
description: '## 概述'
summary: 'HAMi（原 vGPU_4k8s）是一个异构计算设备虚拟化中间件，为 Kubernetes 提供 GPU、NPU 等加速器的共享和虚拟化能力。它允许多个 Pod 共享同一块物理 GPU，并提供显存和算力的精细化隔离，有效提升 GPU 利用率。HAMi 支持 NVIDIA GPU、AMD GPU、华为 Ascend NPU、寒武纪 MLU 等多种异构设备。'
category: entities
tags:
- k8s
- cncf
- observability
- hami
- scheduler
- prometheus
- grafana
- crd
- operator
- gpu
tier: supporting
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- HAMI 是什么
- 如何 HAMI
trigger_keywords:
- HAMI
prerequisites:
- kubectl-basics
- prometheus-basics
- monitoring-basics
- gpu-scheduling-basics
- logging-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# HAMI

> **CNCF 状态**: Sandbox | **类别**: Observability | **主要语言**: Go, C

## 概述

HAMi（原 vGPU_4k8s）是一个异构计算设备虚拟化中间件，为 Kubernetes 提供 GPU、NPU 等加速器的共享和虚拟化能力。它允许多个 Pod 共享同一块物理 GPU，并提供显存和算力的精细化隔离，有效提升 GPU 利用率。HAMi 支持 NVIDIA GPU、AMD GPU、华为 Ascend NPU、寒武纪 MLU 等多种异构设备，在 AI/ML 工作负载场景中显著降低 GPU 硬件成本。

## 核心特性

- **GPU 共享**: 多个 Pod 共享同一块物理 GPU，支持显存隔离和算力隔离
- **多设备支持**: NVIDIA、AMD、华为 Ascend NPU、寒武纪 MLU
- **硬隔离**: 基于 GPU 硬件虚拟化技术（vGPU/MIG）实现显存和算力隔离
- **调度扩展**: 扩展 Kubernetes 调度器，支持 GPU 细粒度资源调度
- **监控集成**: 导出 GPU 使用率、显存占用等 Prometheus 指标
- **设备分片**: 支持 deviceSplitCount 控制每张 GPU 的最大共享数

## 架构

HAMi 通过 device plugin 和调度器扩展实现 GPU 虚拟化。核心组件包括：HAMi Scheduler（扩展 Kubernetes 调度器，感知 GPU 细粒度资源）、HAMi Device Plugin（替代原生 NVIDIA device plugin，上报虚拟化后的 GPU 资源）、HAMi Webhook（拦截 Pod 创建请求，注入 GPU 虚拟化配置）。设备层通过劫持 CUDA 调用或利用硬件虚拟化能力（如 NVIDIA MIG、vGPU）实现资源隔离。调度器维护全局 GPU 资源视图，根据 Pod 的 GPU 请求进行细粒度分配。

## Kubernetes 集成

HAMi 通过 Kubernetes Device Plugin Framework 上报虚拟化后的 GPU 资源（如 `nvidia.com/gpu.mem`、`nvidia.com/gpu.cores`），扩展调度器（Scheduler Extender 或 KubeSchedulerPlugin）实现细粒度 GPU 调度。Mutating Webhook 自动为 Pod 注入 GPU 虚拟化运行时配置。用户通过在 Pod spec 中添加 GPU 资源请求（如 `nvidia.com/gpu: 1` + `nvidia.com/gpu.mem: 2000`）来申请共享 GPU 资源。

## 生产使用场景

1. **推理服务共享 GPU**: 多个推理 Pod 共享一张 GPU，提升利用率降低成本
2. **开发/测试环境**: 多个开发者共享 GPU 资源进行模型调试
3. **多租户 GPU 隔离**: 不同团队的 GPU 任务实现资源隔离
4. **混合调度**: 推理任务使用 GPU 共享，训练任务使用 GPU 独占

## 安装与配置

```bash
# Helm 安装
helm repo add hami https://project-hami.github.io/HAMi
helm install hami hami/hami \
  --set devicePlugin.deviceMemoryScaling=2 \
  --set scheduler.kubeScheduler.imageTag=v0.28.9 \
  -n kube-system
# 验证部署
kubectl get pods -n kube-system | grep hami
kubectl get nodes -o json | jq '.items[].status.allocatable | with_entries(select(.key | contains("nvidia")))'
```

```yaml
# GPU 共享 Pod 示例
apiVersion: v1
kind: Pod
metadata:
  name: gpu-shared-app
spec:
  containers:
  - name: inference
    image: nvcr.io/nvidia/tritonserver:24.01-py3
    resources:
      limits:
        nvidia.com/gpu: 1          # 申请 1 个虚拟 GPU
        nvidia.com/gpumem: 4000    # 限制显存 4GB
        nvidia.com/gpucores: 25    # 限制算力 25%
---
# 多卡分配示例
apiVersion: v1
kind: Pod
spec:
  containers:
  - name: training
    resources:
      limits:
        nvidia.com/gpu: 2          # 申请 2 个虚拟 GPU
        nvidia.com/gpumem: 8000    # 每卡 8GB
```

```bash
# 查看 GPU 分配状态
kubectl get pods -A -o json | jq '[.items[] | select(.spec.containers[].resources.limits["nvidia.com/gpu"] != null)]'
# 查看节点 GPU 资源
kubectl describe node <gpu-node> | grep -A5 "nvidia.com"
```

## 运维操作

```bash
# 🟢 查看 GPU 资源使用情况
kubectl get nodes -o custom-columns=NAME:.metadata.name,GPU:.status.allocatable.'nvidia\.com/gpu'
kubectl logs -n kube-system -l app=hami-device-plugin --tail=50

# 🟢 检查 GPU 隔离是否生效
kubectl exec -it <pod> -- nvidia-smi
kubectl exec -it <pod> -- cat /usr/local/vgpu/containers.json

# 🟡 重启 Device Plugin
kubectl rollout restart daemonset/hami-device-plugin -n kube-system

# 🟡 调整显存超卖比例
helm upgrade hami hami/hami --set devicePlugin.deviceMemoryScaling=1.5 -n kube-system

# 🔴 卸载 HAMi（影响所有 GPU Pod）
helm uninstall hami -n kube-system
kubectl delete mutatingwebhookconfigurations hami-webhook
```

## 故障排查

| 症状 | 可能原因 | 排查命令 | 修复方案 |
|------|----------|----------|----------|
| Pod Pending (Insufficient nvidia.com/gpu) | GPU 资源不足/调度器未扩展 | `kubectl describe pod <name>` | 检查 HAMi scheduler 是否运行 |
| CUDA OOM | 显存超卖过高/隔离失效 | `kubectl exec <pod> -- nvidia-smi` | 降低 deviceMemoryScaling |
| Device Plugin CrashLoop | NVIDIA 驱动版本不兼容 | `kubectl logs -n kube-system ds/hami-device-plugin` | 升级驱动或调整 HAMi 版本 |
| Webhook 拦截失败 | 证书过期/网络不通 | `kubectl logs -n kube-system -l app=hami-webhook` | 重新生成证书或检查网络策略 |
| 算力隔离不生效 | GPU 架构不支持 | 检查 GPU 型号和驱动版本 | 使用支持算力隔离的 GPU 型号 |

```
排查流程：
├─ Pod 无法调度
│  ├─ 检查 HAMi scheduler 是否 Running
│  ├─ 检查节点 allocatable 中 nvidia.com/gpu 是否 > 0
│  └─ 检查 deviceSplitCount 是否达到上限
├─ GPU 资源隔离异常
│  ├─ nvidia-smi 查看实际显存占用
│  ├─ 检查 containers.json 配置是否注入
│  └─ 确认 CUDA 版本与 HAMi 兼容
└─ 组件异常
   ├─ Device Plugin → 检查驱动和内核模块
   └─ Webhook → 检查 TLS 证书有效期
```

## 生产案例

### 案例 1：推理服务 GPU 共享降本

- **场景**: 10 个推理服务各占用 1 张 A100，实际 GPU 利用率仅 15%
- **排查**: 通过 HAMi 监控发现各服务显存占用仅 2-4GB（A100 80GB）
- **方案**: 配置 deviceSplitCount=8，每个 Pod 限制 8GB 显存 + 20% 算力，8 个服务共享 1 张 A100
- **效果**: GPU 数量从 10 张减少到 2 张，年节省硬件成本 80%

### 案例 2：多租户 GPU 资源隔离

- **场景**: AI 平台多团队共享 GPU 集群，需要防止单任务抢占全部资源
- **排查**: 某团队训练任务占满显存导致其他团队推理服务 OOM
- **方案**: 通过 HAMi gpumem 限制 + Namespace ResourceQuota 实现双层隔离
- **效果**: 各团队 GPU 资源独立可控，OOM 事件归零

## 替代方案对比

| 维度 | HAMi | NVIDIA GPU Operator | Volcano GPU | Run:AI |
|------|------|--------------------|--------------|--------|
| 显存隔离 | ✅ 硬隔离 | MIG 仅 A100+ | ❌ | ✅ |
| 算力隔离 | ✅ | MIG | ❌ | ✅ |
| 多设备 | NVIDIA/AMD/NPU/MLU | 仅 NVIDIA | 仅 NVIDIA | 仅 NVIDIA |
| 调度集成 | 扩展调度器 | Device Plugin | 批调度 | 独立调度 |
| 适用场景 | 多设备共享 | 官方标准 | 批处理 | 企业级 |

## 架构定位

在 CNCF 生态中，HAMi 属于 **Scheduling / AI Infrastructure** 类别，解决了 Kubernetes GPU 资源粒度过粗的问题。它与 Volcano、Kueue 等批处理调度器互补。

## 参考链接

- [[实体/prometheus-grafana.md|prometheus-grafana]]
- [[deployment]]
- [[概念/container-runtime-comparison.md|container-runtime-comparison]]
- [[pod-lifecycle]]
- [[实体/kube-scheduler.md|kube-scheduler]]

## Related

- [[fluentd]] — Fluentd
- [[cubefs]] — CubeFS
- [[artifact-hub]] — Artifact Hub
- [[pipecd]] — PipeCD
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- hami
- [[生态参考/领域索引/ai-gpu-index.md|AI / GPU 基础设施知识图谱索引]]
- [[生态参考/领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
