---
title: "KubeRay 分布式计算平台生产部署"
description: "KubeRay Operator 生产部署：RayCluster/RayJob/RayService CR 配置、自动伸缩、Ray Serve 推理与分布式训练"
summary: "覆盖 Ray 核心架构（Head/Worker/Raylet/Object Store/GCS）、KubeRay Operator 部署、RayCluster 自动伸缩与 GPU 资源配置、RayJob 批处理、Ray Serve 在线推理、Ray Train 分布式训练及 Object Store OOM/GCS 故障排查"
category: AI基础设施
tags:
- kuberay
- ray
- distributed-computing
- ray-cluster
- ray-serve
- ray-train
- ray-job
- autoscaling
- gpu
- object-store
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
- "KubeRay 如何在 K8s 上部署 Ray 集群"
- "RayCluster 自动伸缩怎么配置"
- "Ray Serve 和 Ray Train 生产实践"
trigger_keywords:
- kuberay
- ray
- raycluster
- rayjob
- rayservice
- ray-serve
- ray-train
- distributed-computing
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

# KubeRay 分布式计算平台生产部署

## 概述

Ray 是一个通用的分布式计算框架，为 AI/ML 工作负载提供了从数据处理、分布式训练到在线推理的全栈能力。KubeRay 是 Ray 在 Kubernetes 上的官方 Operator，通过 RayCluster、RayJob、RayService 三个 CRD 将 Ray 集群的生命周期管理融入 K8s 声明式体系。

KubeRay 的核心价值在于：将 Ray 的弹性伸缩能力与 K8s 的资源管理结合，实现 GPU 资源的按需分配；通过 Ray Serve 提供低延迟在线推理；通过 Ray Train 实现 PyTorch/TensorFlow 分布式训练的弹性扩缩；通过 RayJob 管理批处理任务的生命周期。关于分布式训练框架的整体对比，参见 [[15-AI基础设施/01-基础设施/05-distributed-training-frameworks]]；GPU 调度参见 [[22-概念/07-调度与资源/gpu-scheduling-ai-workloads]]。

## 架构与核心概念

### Ray 核心架构

```
┌─────────────────────────────────────────────────────────────────┐
│                        Ray Cluster                               │
├─────────────────────────────────────────────────────────────────┤
│  ┌─────────────────────────────────────────────────────────────┐│
│  │  Head Node                                                   ││
│  │  ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌───────────────┐  ││
│  │  │ GCS      │ │ Ray      │ │ Object   │ │ Dashboard     │  ││
│  │  │ Server   │ │ Dashboard│ │ Store    │ │ (Port 8265)   │  ││
│  │  │ (元数据) │ │ (API)    │ │ (Plasma) │ │               │  ││
│  │  └──────────┘ └──────────┘ └──────────┘ └───────────────┘  ││
│  │  ┌──────────┐ ┌──────────────────────────────────────────┐  ││
│  │  │ Raylet   │ │ Autoscaler (监控资源需求，触发扩缩容)     │  ││
│  │  └──────────┘ └──────────────────────────────────────────┘  ││
│  └─────────────────────────────────────────────────────────────┘│
│  ┌─────────────────────────────────────────────────────────────┐│
│  │  Worker Node 1              │  Worker Node 2                 ││
│  │  ┌──────────┐ ┌─────────┐  │  ┌──────────┐ ┌─────────┐    ││
│  │  │ Raylet   │ │ Object  │  │  │ Raylet   │ │ Object  │    ││
│  │  │ (调度)   │ │ Store   │  │  │ (调度)   │ │ Store   │    ││
│  │  └──────────┘ └─────────┘  │  └──────────┘ └─────────┘    ││
│  │  ┌─────────────────────┐   │  ┌─────────────────────┐      ││
│  │  │ Worker Processes    │   │  │ Worker Processes    │      ││
│  │  │ (Tasks/Actors)      │   │  │ (Tasks/Actors)      │      ││
│  │  └─────────────────────┘   │  └─────────────────────┘      ││
│  └─────────────────────────────────────────────────────────────┘│
└─────────────────────────────────────────────────────────────────┘
```

**核心组件说明**：
- **GCS（Global Control Store）**：集群元数据中心，存储 Actor 注册表、节点信息、Placement Group 状态。GCS 故障会导致整个集群不可用。
- **Raylet**：每个节点一个，负责本地资源管理和任务调度。
- **Object Store（Plasma）**：基于共享内存的零拷贝对象存储，用于 Task 间数据传递。默认占用 30% 节点内存。
- **Autoscaler**：监控集群资源需求，通过 KubeRay Operator 触发 Worker Pod 的创建/删除。

### KubeRay CRD 对比

| CRD | 用途 | 生命周期 | 适用场景 |
|-----|------|---------|---------|
| RayCluster | 管理 Ray 集群 | 长期运行 | 交互式开发、Ray Serve 推理 |
| RayJob | 提交批处理任务 | 任务完成即销毁 | 数据处理、超参搜索、批量推理 |
| RayService | 管理 Ray Serve 应用 | 长期运行 + 滚动更新 | 在线推理服务 |

## 生产部署

### KubeRay Operator 安装

🟡 中风险：安装集群级 Operator。

```bash
# 添加 Helm 仓库
helm repo add kuberay https://ray-project.github.io/kuberay-helm/
helm repo update

# 安装 KubeRay Operator（生产配置）
helm install kuberay-operator kuberay/kuberay-operator \
  --namespace kuberay-system --create-namespace \
  --set image.tag=v1.2.2 \
  --set resources.requests.cpu=500m \
  --set resources.requests.memory=512Mi \
  --set resources.limits.cpu=2 \
  --set resources.limits.memory=2Gi \
  --set featureGates.RAY_JOB_CRD_ENABLED=true \
  --version 1.2.2

# 验证安装
kubectl get pods -n kuberay-system
kubectl get crd | grep ray.io
```

### RayCluster CR 配置（自动伸缩 + GPU）

🟡 中风险：创建 Ray 集群，会分配 GPU 资源。

```yaml
apiVersion: ray.io/v1
kind: RayCluster
metadata:
  name: ray-gpu-cluster
  namespace: ai-compute
spec:
  rayVersion: "2.35.0"
  enableInTreeAutoscaling: true
  autoscalerOptions:
    upscalingMode: Default
    idleTimeoutSeconds: 120
    resources:
      limits:
        cpu: "2"
        memory: "2Gi"
  headGroupSpec:
    rayStartParams:
      dashboard-host: "0.0.0.0"
      num-cpus: "0"  # Head 节点不运行任务
    serviceType: ClusterIP
    template:
      metadata:
        labels:
          ray.io/node-type: head
      spec:
        containers:
        - name: ray-head
          image: rayproject/ray-ml:2.35.0-gpu
          ports:
          - containerPort: 6379
            name: gcs
          - containerPort: 8265
            name: dashboard
          - containerPort: 10001
            name: client
          resources:
            limits:
              cpu: "4"
              memory: "16Gi"
            requests:
              cpu: "2"
              memory: "8Gi"
          volumeMounts:
          - name: ray-logs
            mountPath: /tmp/ray
          env:
          - name: RAY_OBJECT_STORE_MEMORY
            value: "8000000000"  # 8GB Object Store
        volumes:
        - name: ray-logs
          emptyDir: {}
  workerGroupSpecs:
  - groupName: gpu-workers
    replicas: 2
    minReplicas: 1
    maxReplicas: 8
    rayStartParams:
      num-gpus: "1"
    template:
      metadata:
        labels:
          ray.io/node-type: worker
          ray.io/worker-group: gpu-workers
      spec:
        containers:
        - name: ray-worker
          image: rayproject/ray-ml:2.35.0-gpu
          resources:
            limits:
              nvidia.com/gpu: 1
              cpu: "8"
              memory: "32Gi"
            requests:
              nvidia.com/gpu: 1
              cpu: "4"
              memory: "16Gi"
          volumeMounts:
          - name: ray-logs
            mountPath: /tmp/ray
          - name: shm
            mountPath: /dev/shm
        volumes:
        - name: ray-logs
          emptyDir: {}
        - name: shm
          emptyDir:
            medium: Memory
            sizeLimit: "8Gi"
        nodeSelector:
          nvidia.com/gpu.present: "true"
        tolerations:
        - key: nvidia.com/gpu
          operator: Exists
          effect: NoSchedule
  - groupName: cpu-workers
    replicas: 2
    minReplicas: 0
    maxReplicas: 16
    rayStartParams:
      num-cpus: "4"
    template:
      spec:
        containers:
        - name: ray-worker
          image: rayproject/ray-ml:2.35.0
          resources:
            limits:
              cpu: "4"
              memory: "16Gi"
            requests:
              cpu: "2"
              memory: "8Gi"
```

### RayJob CR（批处理任务）

🟡 中风险：提交计算任务，消耗集群资源。

```yaml
apiVersion: ray.io/v1
kind: RayJob
metadata:
  name: batch-inference-job
  namespace: ai-compute
spec:
  entrypoint: python /app/batch_inference.py --input s3://data/input/ --output s3://data/output/
  runtimeEnvYAML: |
    pip:
      - transformers==4.44.0
      - torch==2.4.0
    env_vars:
      MODEL_PATH: "/models/llama-3-8b-instruct"
  shutdownAfterJobFinishes: true
  ttlSecondsAfterFinished: 3600
  submitterPodTemplate:
    spec:
      containers:
      - name: submitter
        image: rayproject/ray:2.35.0
        resources:
          requests:
            cpu: "1"
            memory: "2Gi"
  rayClusterSpec:
    rayVersion: "2.35.0"
    headGroupSpec:
      rayStartParams:
        dashboard-host: "0.0.0.0"
      template:
        spec:
          containers:
          - name: ray-head
            image: rayproject/ray-ml:2.35.0-gpu
            resources:
              limits:
                cpu: "4"
                memory: "16Gi"
    workerGroupSpecs:
    - groupName: gpu-workers
      replicas: 4
      minReplicas: 4
      maxReplicas: 4
      template:
        spec:
          containers:
          - name: ray-worker
            image: rayproject/ray-ml:2.35.0-gpu
            resources:
              limits:
                nvidia.com/gpu: 1
                memory: "32Gi"
```

### RayService（在线推理）

🟡 中风险：创建长期运行的推理服务。

```yaml
apiVersion: ray.io/v1
kind: RayService
metadata:
  name: ray-serve-llm
  namespace: ai-serving
spec:
  serveConfigV2: |
    applications:
    - name: llm-serving
      import_path: serve_app:deployment
      runtime_env:
        pip:
          - vllm==0.6.3
      deployments:
      - name: LLMDeployment
        num_replicas: 2
        ray_actor_options:
          num_gpus: 1
        autoscaling_config:
          min_replicas: 1
          max_replicas: 4
          target_num_ongoing_requests_per_replica: 10
  rayClusterConfig:
    rayVersion: "2.35.0"
    headGroupSpec:
      rayStartParams:
        dashboard-host: "0.0.0.0"
      template:
        spec:
          containers:
          - name: ray-head
            image: rayproject/ray-ml:2.35.0-gpu
            resources:
              limits:
                cpu: "4"
                memory: "16Gi"
    workerGroupSpecs:
    - groupName: serving-workers
      replicas: 2
      minReplicas: 1
      maxReplicas: 4
      template:
        spec:
          containers:
          - name: ray-worker
            image: rayproject/ray-ml:2.35.0-gpu
            resources:
              limits:
                nvidia.com/gpu: 1
                memory: "32Gi"
            volumeMounts:
            - name: model-storage
              mountPath: /models
          volumes:
          - name: model-storage
            persistentVolumeClaim:
              claimName: model-pvc-llama3-8b
```

## 运维操作

### Ray Dashboard 访问

🟢 低风险/只读。

```bash
# 端口转发访问 Ray Dashboard
kubectl port-forward svc/ray-gpu-cluster-head-svc -n ai-compute 8265:8265

# 查看 Ray 集群状态
kubectl get raycluster -n ai-compute
kubectl get raycluster ray-gpu-cluster -n ai-compute -o yaml | grep -A 10 "status:"

# 查看 RayJob 状态
kubectl get rayjob -n ai-compute
kubectl logs -n ai-compute -l ray.io/job-name=batch-inference-job --tail=100

# 查看 RayService 状态
kubectl get rayservice -n ai-serving
kubectl describe rayservice ray-serve-llm -n ai-serving
```

### 分布式训练（Ray Train + PyTorch）

```python
# train_script.py - Ray Train 分布式训练示例
import ray
from ray.train import ScalingConfig
from ray.train.torch import TorchTrainer
from ray.train import RunConfig, CheckpointConfig

def train_func(config):
    import torch
    from ray.train.torch import prepare_model, prepare_data_loader
    # ... 模型定义和数据加载 ...
    model = prepare_model(model)
    # ... 训练循环 ...

trainer = TorchTrainer(
    train_func,
    scaling_config=ScalingConfig(
        num_workers=4,
        use_gpu=True,
        resources_per_worker={"GPU": 1, "CPU": 4},
    ),
    run_config=RunConfig(
        storage_path="s3://checkpoints/",
        checkpoint_config=CheckpointConfig(
            num_to_keep=3,
            checkpoint_frequency=100,
        ),
    ),
)
result = trainer.fit()
```

### 监控（Prometheus 集成）

🟢 低风险/只读。

```bash
# Ray 内置 Prometheus 指标（Head Node 8080 端口）
kubectl port-forward svc/ray-gpu-cluster-head-svc -n ai-compute 8080:8080
curl -s localhost:8080/metrics | grep ray_

# 关键指标：
# ray_cluster_active_nodes - 活跃节点数
# ray_cluster_pending_nodes - 等待启动的节点数
# ray_object_store_memory - Object Store 内存使用
# ray_tasks - 各状态任务数
# ray_actors - 各状态 Actor 数
# ray_gpus_available - 可用 GPU 数
# ray_gpus_in_use - 使用中 GPU 数

# 配置 Prometheus ServiceMonitor
# apiVersion: monitoring.coreos.com/v1
# kind: ServiceMonitor
# metadata:
#   name: ray-cluster-monitor
# spec:
#   selector:
#     matchLabels:
#       ray.io/cluster: ray-gpu-cluster
#   endpoints:
#   - port: metrics
#     interval: 15s
```

## 故障排查

### Pod 调度失败

```bash
# 🟢 Step 1: 检查 Pending Pod 事件
kubectl get pods -n ai-compute -l ray.io/cluster=ray-gpu-cluster
kubectl describe pod <pending-pod> -n ai-compute | grep -A 10 "Events"

# 常见原因：
# 1. "Insufficient nvidia.com/gpu" → GPU 资源不足，检查节点可用 GPU
# 2. "node(s) didn't match node selector" → nodeSelector 无匹配节点
# 3. "pod has unbound PVCs" → PVC 未绑定

# 🟢 Step 2: 检查集群 GPU 资源总量
kubectl describe nodes -l nvidia.com/gpu.present=true | grep -A 5 "Allocated resources"

# 🟡 Step 3: 调整 Worker 副本数
kubectl patch raycluster ray-gpu-cluster -n ai-compute --type='merge' \
  -p '{"spec":{"workerGroupSpecs":[{"groupName":"gpu-workers","replicas":2}]}}'
```

### Object Store OOM

```bash
# 🟢 检查 Object Store 使用情况
# 通过 Ray Dashboard → Memory 页面查看
# 或通过 Ray API：
# ray status 查看 object store 使用

# 症状：Worker 被 OOMKilled，日志中出现
# "ray.exceptions.ObjectStoreFullError" 或
# "Plasma store out of memory"

# 修复方案：
# 1. 增大 Object Store 内存
#    env: RAY_OBJECT_STORE_MEMORY = "16000000000"  # 16GB
# 2. 减少并发任务数，避免大量中间对象堆积
# 3. 使用 ray.put() 显式管理大对象生命周期
# 4. 设置 RAY_object_spilling_threshold 启用对象溢写到磁盘
```

### GCS 故障

```bash
# 🟢 检查 GCS 状态
kubectl logs <head-pod> -n ai-compute -c ray-head | grep -i "gcs\|error"

# GCS 故障表现：
# - 所有 Worker 断开连接
# - 新任务无法提交
# - Actor 无法创建

# 🔴 GCS 恢复（需要重启 Head Pod，会丢失所有运行中的任务）
kubectl delete pod <head-pod> -n ai-compute
# KubeRay Operator 会自动重建 Head Pod
# 注意：如果未配置 GCS 外部 Redis 持久化，集群状态将丢失

# 生产建议：启用 GCS Fault Tolerance（外部 Redis）
# headGroupSpec:
#   rayStartParams:
#     redis-password: "xxx"
#   env:
#   - name: RAY_REDIS_ADDRESS
#     value: "redis://redis-svc:6379"
```

### Ray Serve 推理延迟异常

```bash
# 🟢 检查 Ray Serve 状态
ray serve status  # 在 Head Pod 内执行

# 🟢 检查副本数和请求队列
curl -s http://ray-head-svc:52365/api/serve/applications/ | jq .

# 常见原因：
# 1. 副本数不足 → 调整 autoscaling_config
# 2. GPU 利用率 100% → 模型推理本身是瓶颈
# 3. Object Store 压力 → 大请求/响应占用过多共享内存
```

## 最佳实践

1. **Head 节点隔离**：Head 节点设置 `num-cpus: 0` 避免运行用户任务，确保 GCS 和 Dashboard 的稳定性。Head Pod 资源请求要充足（至少 4 CPU / 16GB 内存）。

2. **GCS 高可用**：生产环境必须启用 GCS Fault Tolerance，将元数据持久化到外部 Redis。否则 Head Pod 重启将导致整个集群状态丢失。

3. **Object Store 容量规划**：默认 Object Store 占节点内存的 30%。数据密集型任务（如大规模 Shuffle）需要增大 Object Store 或启用对象溢写（Spilling）。

4. **Worker Group 分层**：按 GPU 型号和用途划分 Worker Group（如 A100 训练组、T4 推理组、CPU 数据处理组），通过 Placement Group 确保任务调度到正确的硬件。

5. **RayJob TTL 管理**：设置 `shutdownAfterJobFinishes: true` 和合理的 `ttlSecondsAfterFinished`，避免已完成任务的集群持续占用资源。

6. **共享内存配置**：Ray 的 Object Store 依赖 `/dev/shm`，K8s 默认 64MB 远远不够。必须挂载 `emptyDir.medium: Memory` 并设置足够大小。

7. **与 Gang Scheduling 配合**：分布式训练需要所有 Worker 同时就绪，建议配合 [[22-概念/07-调度与资源/gang-scheduling]] 避免资源碎片化。Volcano 或 Kueue 可作为调度器。

8. **日志持久化**：Ray 日志默认写入 `/tmp/ray/session_*/logs/`，Pod 重启后丢失。生产环境应挂载持久卷或配置日志采集到集中式日志系统。

## Related

- [[15-AI基础设施/01-基础设施/05-distributed-training-frameworks]]
- [[22-概念/07-调度与资源/gpu-scheduling-ai-workloads]]
- [[22-概念/07-调度与资源/gang-scheduling]]
- [[15-AI基础设施/01-基础设施/04-gpu-monitoring-dcgm]]
- [[19-故障诊断/]]
