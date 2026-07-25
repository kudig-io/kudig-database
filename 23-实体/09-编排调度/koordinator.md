---
title: Koordinator (entities)
description: '## 概述'
summary: 'Koordinator 是一个基于 QoS 的 Kubernetes 混合调度系统，专为提高集群资源利用率而设计。它通过精细化的资源管理和混部（co-location）技术，在保证延迟敏感型（LS）工作负载 SLO 的同时，充分利用空闲资源运行尽力而为型（BE）任务，实现 60%+ 的集群利用率。'
category: entities
tags:
- k8s
- cncf
- orchestration
- koordinator
- scheduler
- crd
- operator
- gpu
tier: peripheral
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Koordinator 是什么
- 如何 Koordinator
trigger_keywords:
- Koordinator
prerequisites:
- kubectl-basics
- gpu-scheduling-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# Koordinator

> **CNCF 状态**: Sandbox | **类别**: Orchestration | **主要语言**: Go

## 概述

Koordinator 是由阿里巴巴（阿里云）开源的 Kubernetes 混合调度系统，2022 年进入 CNCF Sandbox。它解决的核心问题是**集群资源利用率低下**——传统 K8s 集群中，在线服务（Deployment）平均 CPU 利用率通常仅 10-20%，大量资源处于"已申请但未使用"状态。Koordinator 通过**混部（Co-location）**技术，将这些空闲资源分配给可容忍资源抢占的批处理任务（Best-Effort，如大数据、AI 训练），在保证在线服务 SLO 的前提下将集群利用率提升到 60%+。

Koordinator 扩展了 Kubernetes 调度器，引入了 **QoS（Quality of Service）分级模型**：LSE（Latency Sensitive Exclusive）、LSR（Latency Sensitive Reserved）、LS（Latency Sensitive）、BE（Best Effort）。在线服务标记为 LS/LSR，批处理标记为 BE，调度器优先保障 LS 资源，BE 任务使用超卖（overcommit）资源。

## Key Features

- **QoS 分级调度**：LSE/LSR/LS/BE 四级 QoS，精细控制工作负载优先级
- **CPU/内存混部**：在线服务空闲的 CPU 和内存资源超卖给 BE 任务
- **GPU 共享调度**：多 Pod 共享同一 GPU（时间分片或显存分区），提升 GPU 利用率
- **CPU Burst**：允许在线服务突发使用超过 limit 的 CPU，减少延迟抖动
- **弹性 Quota**：跨团队/命名空间的弹性资源配额，允许借用空闲配额
- **Koordlet**：节点级守护进程，采集真实资源使用画像，驱动重调度和抢占

## Architecture

Koordinator 由 **Koordinator Scheduler**（扩展的 K8s 调度器，支持 QoS 感知调度）、**Koordlet**（节点级 DaemonSet，采集资源指标、执行 CPU/内存隔离策略）、**Descheduler**（重调度器，检测资源热点和 QoS 违规并迁移 Pod）和 **CRD 控制器**（管理 PodMigrationJob、Device 等）组成。Koordlet 通过 cgroup 和 EDF（Earliest Deadline First）算法实现 CPU 调度隔离，确保 LS 任务优先于 BE 任务获得 CPU 时间。

## K8s 集成

Koordinator 作为标准 Kubernetes 调度器插件运行。通过 Pod 的 label/annotation 标记 QoS 级别（`koordinator.sh/qosClass: BE`），调度器自动应用对应的调度策略。也提供自定义 CRD：`PodMigrationJob`（安全迁移 Pod）、`Device`（GPU 设备管理）、`ElasticQuota`（弹性配额）。与标准 K8s PriorityClass 和 ResourceQuota 机制兼容。

## 生产部署要点

- **渐进混部**：从低资源利用率的集群开始，逐步提高 BE 工作负载比例
- **QoS 分级**：严格按业务重要性配置 QoS 级别，确保核心服务 SLO
- **CPU Burst**：为突发流量的在线服务启用 CPU Burst，减少延迟抖动
- **资源画像**：利用 Koordlet 收集的实际资源使用数据优化资源 request
- **GPU 共享**：推理服务使用 GPU 共享调度，提升 GPU 利用率
- **弹性 Quota**：跨团队使用弹性 Quota 允许资源借用，提高整体效率

## 生产场景

1. **在线+离线混部**：电商交易（LS）与数据分析（BE）混部，利用交易服务空闲时段跑分析
2. **GPU 共享推理**：多个低 QPS AI 推理服务共享 GPU，降低 GPU 成本 50%+
3. **CI/CD 批处理混部**：构建任务（BE）利用集群空闲资源，不额外采购机器
4. **弹性配额管理**：多团队共享集群，弹性 Quota 自动借用/归还资源

## 安装与配置

```bash
# Helm 安装 Koordinator
helm repo add koordinator-sh https://koordinator-sh.github.io/charts/
helm repo update
helm install koordinator koordinator-sh/koordinator -n koordinator-system --create-namespace \
  --set scheduler.replicas=2 \
  --set koordlet.enabled=true \
  --set descheduler.enabled=true

# 等待组件就绪
kubectl wait --for=condition=available deployment/koord-scheduler -n koordinator-system --timeout=120s
kubectl get pods -n koordinator-system
```

```yaml
# BE 工作负载示例（批处理任务使用超卖资源）
apiVersion: v1
kind: Pod
metadata:
  name: spark-worker
  labels:
    koordinator.sh/qos-class: BE
spec:
  schedulerName: koordinator-scheduler
  containers:
  - name: worker
    image: spark:latest
    resources:
      limits:
        kubernetes.io/batch-cpu: "4"
        kubernetes.io/batch-memory: "8Gi"
---
# GPU 共享调度示例（多 Pod 共享 GPU）
apiVersion: v1
kind: Pod
metadata:
  name: inference-svc
  labels:
    koordinator.sh/qos-class: LS
spec:
  schedulerName: koordinator-scheduler
  containers:
  - name: model-server
    image: triton-inference:latest
    resources:
      limits:
        kubernetes.io/gpu-core: "50"   # 50% GPU 算力
        kubernetes.io/gpu-memory: "4Gi"  # 4Gi 显存
---
# 弹性 Quota 配置
apiVersion: scheduling.sigs.k8s.io/v1alpha1
kind: ElasticQuota
metadata:
  name: team-ml
  namespace: ml-team
spec:
  max:
    cpu: "100"
    memory: 200Gi
  min:
    cpu: "20"
    memory: 40Gi
```

## 运维操作

```bash
# 🟢 查看节点资源超卖情况
kubectl get nodes -o custom-columns=NAME:.metadata.name,BATCH-CPU:.status.allocatable.kubernetes\.io/batch-cpu,BATCH-MEM:.status.allocatable.kubernetes\.io/batch-memory

# 🟢 查看 GPU 设备分配
kubectl get device -A
kubectl get pods -o custom-columns=NAME:.metadata.name,GPU:.spec.containers[0].resources.limits.kubernetes\.io/gpu-core

# 🟡 调整节点超卖比例
kubectl annotate node node-1 koordinator.sh/colocation-profile='{"cpuReclaimThresholdPercent":60,"memoryReclaimThresholdPercent":70}' --overwrite

# 🟡 触发 Pod 迁移（解决资源热点）
kubectl apply -f - <<EOF
apiVersion: scheduling.koordinator.sh/v1alpha1
kind: PodMigrationJob
metadata:
  name: migrate-hot-pod
spec:
  podRef:
    namespace: production
    name: hot-pod-xxx
  mode: EvictThenMigrate
EOF

# 🟢 查看 Koordlet 采集的资源画像
kubectl get nodemetrics node-1 -o yaml | grep -A5 cpuUsage

# 🔴 禁用混部（紧急场景，停止所有 BE 任务）
kubectl annotate node node-1 koordinator.sh/colocation-enabled=false --overwrite
```

## 故障排查

| 症状 | 可能原因 | 排查命令 | 修复方案 |
|------|----------|----------|----------|
| BE Pod 无法调度 | 节点无可超卖资源 | `kubectl describe pod <be-pod>` | 降低 LS 工作负载或添加节点 |
| GPU 共享失败 | Device CRD 未创建或 GPU 驱动异常 | `kubectl get device` | 检查 koordlet 日志和 GPU 驱动 |
| LS 服务延迟抨动 | BE 任务抢占 CPU 资源 | `kubectl top pod -n production` | 调高 cpuReclaimThreshold |
| 弹性 Quota 不生效 | Quota CRD 未正确配置 | `kubectl get elasticquota -A` | 检查 min/max 配置和命名空间关联 |
| Koordlet CrashLoop | 节点 cgroup 版本不兼容 | `kubectl logs -n koordinator-system -l app=koordlet` | 确认 cgroup v2 并更新 koordlet |

```
排查流程：
├── BE Pod 调度失败
│   ├── kubectl describe pod 查看调度事件
│   ├── 检查节点 batch-cpu/batch-memory 可分配量
│   ├── 确认 koordinator-scheduler 正常运行
│   └── 检查节点是否禁用了混部
├── LS 服务 SLO 违规
│   ├── 检查 Koordlet 的 CPU 压制日志
│   ├── 确认 QoS 级别标记正确
│   ├── 检查 CPU Burst 是否启用
│   └── 调整 cpuReclaimThresholdPercent
└── GPU 共享异常
    ├── kubectl get device 确认 GPU 设备已注册
    ├── 检查 GPU 驱动和 CUDA 版本兼容性
    └── 查看 koord-scheduler 日志中的 GPU 分配记录
```

## 生产案例

### 案例 1：电商大促混部提升利用率

- **场景**：电商集群 500 节点，平时 CPU 利用率仅 15%，大促时需要 3 倍资源，非大促时大量资源浪费
- **排查**：固定资源分配导致非大促时 85% 资源空闲，大促前需提前 2 周扩容机器
- **方案**：在线服务标记为 LS，数据分析/CI 构建标记为 BE，利用 Koordinator 混部填充空闲资源
- **效果**：集群利用率从 15% 提升至 65%，年度服务器采购减少 40%，大促前无需提前扩容

### 案例 2：GPU 共享推理降本

- **场景**：AI 推理服务 20+ 个模型，每个模型独占 1 张 A100 GPU，平均 GPU 利用率仅 25%
- **排查**：低 QPS 模型独占 GPU 造成巨大浪费，A100 单价 10万+，成本压力巨大
- **方案**：使用 Koordinator GPU 共享调度，按 gpu-core 和 gpu-memory 分配，多模型共享同一 GPU
- **效果**：GPU 利用率从 25% 提升至 75%，GPU 数量从 20 张减至 8 张，年度 GPU 成本节省 120 万

## 对比

| 特性 | Koordinator | Volcano | YuniKorn | 默认 Scheduler | 适用场景 |
|------|------------|---------|---------|---------------|----------|
| 混部（Co-location） | ✅ 核心能力 | ⚠️ | ⚠️ | ❌ | 提升利用率 |
| GPU 共享 | ✅ | ⚠️ | ❌ | ❌ | AI 推理降本 |
| QoS 分级 | ✅ 4 级 | ❌ | ✅ 3 级 | ❌ | 多优先级工作负载 |
| 批处理队列 | ⚠️ | ✅ | ✅ | ❌ | 大数据/AI 训练 |
| 生产成熟度 | 高（阿里） | 高（华为） | 中 | 高 | 企业级稳定性 |

## 参考链接

- [[deployment]]
- [[23-实体/02-K8s核心组件/crd-custom-resources.md|crd-custom-resources]]
- [[22-概念/01-核心架构/controller-pattern.md|controller-pattern]]
- [[pod-lifecycle]]
- [[23-实体/02-K8s核心组件/kube-scheduler.md|kube-scheduler]]

## Related

- [[eraser]] — Eraser
- [[kubewarden]] — Kubewarden
- [[devfile]] — Devfile
- [[cohdi]] — Cohdi
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- koordinator
- index/gitops-cicd-index|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
