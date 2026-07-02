---
title: AI/ML 工作负载排查
description: '# AI/ML 工作负载排查'
summary: '1. **GPU 可见性**：`kubectl get nodes -o jsonpath='{.items[*].status.allocatable.nvidia\.com/gpu}'`，确认资源暴露。'
category: skills
tags:
- k8s
- troubleshooting
- structural
- ai-ml-workloads
- controller-manager
- istio
- minio
- mysql
- daemonset
- job
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- AI/ML 工作负载排查 是什么
- 如何 AI/ML 工作负载排查
trigger_keywords:
- AI
- ML
- 工作负载排查
prerequisites:
- kubectl-basics
- pod-lifecycle
- service-mesh-basics
- mysql-basics
- gpu-scheduling-basics
- tls-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# AI/ML 工作负载排查

### 01 Ai Ml Workloads Troubleshootingompt 模板|Troubleshooting]]

#### 0. 10 分钟快速诊断

1. **GPU 可见性**：`kubectl get nodes -o jsonpath='{.items[*].status.allocatable.nvidia\.com/gpu}'`，确认资源暴露。
2. **设备插件**：检查 Device Plugin DaemonSet 状态与日志。
3. **训练作业**：查看分布式训练 Pod 事件，关注 NCCL/网络报错。
4. **数据与存储**：确认数据集 PVC 挂载、I/O 吞吐与热点。
5. **资源请求**：核对 GPU/CPU/内存 requests/limits，避免碎片化。
6. **快速缓解**：
   - 降低 batch size 或启用混合精度。
   - 调整亲和性/拓扑，让训练 Pod 同机房/同交换机。
7. **证据留存**：保存训练日志、GPU 指标、Pod 事件与拓扑信息。

#### AI/ML 特有问题现象

| 问题类型 | 典型现象 | 影响程度 | 紧急级别 |
|---------|---------|---------|---------|
| GPU 资源调度失败 | `0/5 nodes are available: 5 Insufficient nvidia.com/gpu` | ⭐⭐⭐ 高 | P0 |
| 分布式训练通信失败 | `NCCL error: unhandled cuda error` | ⭐⭐⭐ 高 | P0 |
| 模型服务推理超时 | `model inference timeout after 30s` | ⭐⭐ 中 | P1 |
| 数据集加载性能问题 | `dataset loading took 30+ minutes` | ⭐⭐ 中 | P1 |
| GPU 内存不足崩溃 | `CUDA out of memory` | ⭐⭐⭐ 高 | P0 |
| 模型版本管理混乱 | `serving model version mismatch` | ⭐⭐ 中 | P1 |
| 训练任务资源浪费 | `GPU utilization < 20%` | ⭐⭐ 中 | P1 |
| 成本控制失效 | `unexpected GPU billing spike` | ⭐⭐⭐ 高 | P0 |

#### AI/ML 工作负载状态检查

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# GPU 资源状态检查
echo "=== GPU 资源状态检查 ==="
kubectl get nodes -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{.status.capacity.nvidia\.com/gpu}{"\n"}{end}'

# NVIDIA Device Plugin 状态
echo "=== NVIDIA Device Plugin 状态 ==="
kubectl get pods -n kube-system -l app=nvidia-device-plugin-daemonset

# GPU 利用率监控
echo "=== GPU 利用率检查 ==="
kubectl get nodes -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{.status.allocatable.nvidia\.com/gpu}{" allocated\n"}{end}'

# 分布式训练作业状态
echo "=== 分布式训练作业状态 ==="
kubectl get jobs -l app=distributed-training --all-namespaces
kubectl get pods -l app=distributed-training --all-namespaces -o wide

# 模型服务状态
echo "=== 模型服务状态 ==="
kubectl get services -l app=model-serving --all-namespaces
kubectl get deployments -l app=model-serving --all-namespaces
```
---

### 02 Kubeflow Troubleshooting

#### 0. 10 分钟快速诊断

1. **核心组件存活**：`kubectl get pods -n kubeflow`，确认所有 Pod 状态为 Running，特别关注 `kubeflow-pipelines`、`katib-controller`、`kserve-controller-manager`。
2. **认证授权**：访问 Kubeflow Central Dashboard，确认身份认证服务（Dex/OIDC/Auth0）正常工作，无登录循环。
3. **Pipeline 状态**：`kubectl get workflows -A`，查看是否有失败的 Argo Workflow。
4. **KServe 推理服务**：`kubectl get inferenceservices -A`，确认模型服务状态为 `Ready`。
5. **Notebook 服务器**：`kubectl get notebooks -A`，检查 Jupyter Notebook Pod 状态。
6. **快速缓解**：
   - Pipeline 卡住：删除失败的 Workflow 并重新提交。
   - Notebook 无法启动：检查 PVC 和镜像拉取状态。
   - KServe 模型加载失败：检查模型存储 URI 和 Secret 权限。
7. **证据留存**：保存 Kubeflow 组件日志、Workflow YAML、InferenceService 状态、Notebook 事件。

---

#### 2. 排查方法与步骤



#### 2.1 诊断原理说明

Kubeflow 是一个复杂的 ML 平台，由多个独立组件组成：

```
┌─────────────────────────────────────────────────────────────────┐
│                    Kubeflow Central Dashboard                    │
│  (认证: Dex/OIDC + 授权: Istio AuthorizationPolicy)              │
├─────────────────────────────────────────────────────────────────┤
│  Kubeflow Pipelines         │  Katib Hyperparameter Tuning     │
│  - ml-pipeline (API)        │  - katib-controller              │
│  - ml-pipeline-ui           │  - katib-db-manager              │
│  - argo-workflow-controller │  - suggestion algorithms         │
│  - persistence-agent        │  - metrics-collector             │
├─────────────────────────────────────────────────────────────────┤
│  KServe Model Serving       │  Notebook Servers                │
│  - kserve-controller        │  - notebook-controller           │
│  - inference services       │  - jupyter-web-app               │
│  - storage-initializer      │  - pvcaccess-management          │
│  - transformers/explainers  │                                  │
├─────────────────────────────────────────────────────────────────┤
│  共享基础设施                                                   │
│  - MinIO/S3 (Artifact Store)  │  - MySQL (Pipeline DB)         │
│  - Istio Ingress Gateway      │  - Cert-manager (TLS)          │
└─────────────────────────────────────────────────────────────────┘
```

**关键依赖链**：
- Pipeline 依赖：Argo Workflows → MinIO/S3 → MySQL
- Katib 依赖：Katib Controller → Suggestion Services → Metric
...(截断)

---

### 03 Mpi Operator Troubleshooting

#### 0. 10 分钟快速诊断

1. **MPIJob 状态**：`kubectl get mpijobs -A`，确认 `Launcher` 和 `Worker` Pod 状态。
2. **Launcher 日志**：`kubectl logs -l mpi-job-name=<job-name> -l mpi-role=launcher --tail=100`，查看 `mpirun` 输出。
3. **Worker  readiness**：`kubectl get pods -l mpi-job-name=<job-name> -l mpi-role=worker`，确认所有 Worker 为 Running。
4. **SSH-less 连通性**：进入 Launcher Pod，执行 `mpirun --hostfile /etc/mpi/hostfile hostname`，验证 Worker 可达。
5. **GPU/网络可见性**：检查 Worker Pod 内 `nvidia-smi` 和 `ibstat`（如使用 InfiniBand）。
6. **快速缓解**：
   - Worker 启动缓慢：检查镜像拉取和节点资源。
   - 通信失败：确认 NetworkPolicy 允许 Worker 之间通信。
   - 进程数不匹配：检查 `slots` 配置与节点 GPU/CPU 数是否一致。
7. **证据留存**：保存 Launcher/Worker 日志、hostfile、MPIJob YAML、节点资源状态。

---

#### 2. 排查方法与步骤



#### 2.1 诊断原理说明

MPI Operator 基于 Kubernetes CRD 实现 MPI 作业调度，其核心架构：

```
用户提交 MPIJob
        │
        ▼
┌─────────────────────┐
│   MPI Operator       │ ──► 监听 MPIJob，创建 Launcher + Worker Pods
│   (mpi-operator)     │
└──────────┬──────────┘
           │
    ┌──────┴──────┐
    ▼             ▼
┌─────────┐   ┌─────────┐
│ Launcher │   │ Worker  │
│ Pod      │   │ Pods    │
│          │   │         │
│ mpirun   │──►│ 训练进程 │
│ ssh client│   │ sshd    │
└─────────┘   └─────────┘
```

**关键机制**：
- **Hostfile 生成**：MPI Operator 自动为 Launcher 生成 `/etc/mpi/hostfile`，包含所有 Worker 的 DNS 名称和 slots 数
- **SSH-less 通信**：通过 `mpiexec` + `ssh` 或 `pmi` 实现 Launcher 到 Worker 的进程启动
- **进程映射**：`slots` 决定每个 Worker 上启动的进程数，通常等于 GPU 数或 CPU 核心数

## 相关链接

- [[entities/k8s-knowledge-map.md|K8s 知识图谱]]

## Related

- [[kserve]] — KServe
- [[dex]] — Dex
- [[cert-manager]] — cert-manager
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[argo]] — Argo Workflows


<!-- risk-assessed -->
