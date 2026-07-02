---
title: AI/ML 工作负载问题排查指南 [docs]
description: '# AI/ML 工作负载问题排查指南'
summary: '# AI/ML 工作负载问题排查指南'
category: general
tags:
- k8s
- kubelet
- docker
- job
- gpu
- cuda
- nvidia
- kubeflow
- kserve
- rag
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- AI/ML 工作负载问题排查指南 是什么
- 如何 AI/ML 工作负载问题排查指南
- AI/ML 工作负载问题排查指南 问题排查
- AI/ML 工作负载问题排查指南 排障步骤
trigger_keywords:
- AI
- ML
- 工作负载问题排查指南
prerequisites:
- kubectl-basics
- pod-lifecycle
- gpu-scheduling-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# AI/ML 工作负载问题排查指南

> **版本**: v1.0
> **创建日期**: 2026-05-18
> **用途**: AI/ML 工作负载在 [[entities/kubernetes.md|kubernetes]] 上的问题排查指南
> **覆盖**: 分布式训练 (MPI/NCCL)、模型服务 (KServe/Triton)、数据处理 (Spark/Flink)

---

## 1. 分布式训练问题排查

### 1.1 MPIJob 启动失败

| 症状 | 诊断命令 | 根因 | 修复命令 |
|------|---------|------|---------|
| MPIJob 一直 Pending | `kubectl describe mpijob <name>` | GPU 资源不足 | `kubectl patch mpijob -p '{"spec": {"slotsPerWorker": "1"}}'` |
| Worker 无法创建 | `kubectl get pods -n <ns>` | 工作节点打污点 | 添加 toleration |
| 启动脚本失败 | `kubectl logs -f mpi-worker-0` | 镜像拉取失败 | 检查镜像配置 |
| Rendezvous 失败 | `kubectl logs mpi-worker-0 | grep -i " rendezvous"` | 网络不通 | 检查 Pod 网络策略 |

### 1.2 NCCL 通信问题

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 检查 NCCL 通信测试
kubectl exec -it mpi-worker-0 -- bash -c "NCCL_DEBUG=INFO NCCL_TOPO_DUMP=1 ./nccl-tests/build/all_reduce_perf"

# 常见 NCCL 错误
# NCCL WARN init.png: Cannot find CAMD I/O elevation
# → GPU 拓扑检测失败，运行 NCCL_IGNORE_CUDA_GRAPH=1

# NCCL WARN NET/Socket : Connection reset by peer
# → GPU 间网络问题，检查 RoCE/InfiniBand 配置

# NCCL WARN init.png: missing shield
# → 多网卡时手动指定 NCCL_IB_PCI_RELAXED_ORDERING=1
```
| 错误信息 | 根因 | 修复 |
|---------|------|------|
| NCCL_TIMEOUT | NCCL 通信超时 | 增加 NCCL_TIMEOUT 环境变量 |
| NCCL_IGNORESIG | 信号处理问题 | 设置 NCCL_IGNORE_SIGTERM=1 |
| Connection refused | 防火墙阻断 | 开放 GPU 节点间端口 |

### 1.3 分布式训练常见问题

| 场景 | 诊断步骤 | 修复方案 |
|------|---------|---------|
| 梯度同步慢 | `kubectl exec mpi-worker-0 -- nvidia-smi topo -m` | 检查 GPU NVLink/PCIe 拓扑 |
| GPU 利用率低 | `kubectl top pods` | 检查数据加载是否瓶颈 |
| OOM during training | `kubectl describe pod` 查看 OOMKilled | 减小 batch size 或启用 gradient checkpointing |
| 训练卡在某个 epoch | 检查 dmesg 是否有 GPU Xid 错误 | 重启 GPU driver 或回滚训练脚本 |

### 1.4 DeepSpeed 问题

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# DeepSpeed 日志检查
kubectl logs -f deployment/<name> | grep -i "deepspeed|ZeRO"

# 常见问题
# RuntimeError: Cannot find deepspeed ops
# → 重新安装 deepspeed: pip install deepspeed --force-reinstall

# ZeroOptimizer initialization error
# → 检查 stage 配置与可用显存是否匹配
```
---

## 2. KServe/Triton 模型服务问题

### 2.1 KServe InferenceService 无法启动

| 症状 | 诊断命令 | 根因 | 修复 |
|------|---------|------|------|
| InferenceService Pending | `kubectl describe isvc <name>` | Predictor 镜像拉取失败 | 检查 imagePullSecrets |
| Pod OOMKilled | `kubectl describe pod -l serving.kserve.io/inferenceservice=<name>` | 模型过大/显存不足 | 增加 memory limit |
| 预测超时 | `kubectl logs -f <pod> -c kserve-container` | 模型加载慢/推理慢 | 使用 smaller model 或优化 batching |

### 2.2 模型加载失败

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 检查模型格式
kubectl exec -it <pod> -- ls -la /mnt/models/

# 常见模型格式问题
# TorchScript: 需要 .pt 文件 + model.pt 同名
# ONNX: 需要 model.onnx
# TensorFlow: 需要 SavedModel 目录

# 验证模型可用性
kubectl exec -it <pod> -- python -c "
import torch
model = torch.jit.load('/mnt/models/model.pt')
print('Model loaded successfully')
"
```
| 错误 | 根因 | 修复 |
|------|------|------|
| Failed to load model: Invalid archive | 模型文件损坏 | 重新上传模型到 PVC/S3 |
| missing shield for model | 模型目录结构错误 | 检查 modelsource 格式 |
| CUDA out of memory | 显存不足 | 减小模型批大小或使用量化 |

### 2.3 推理延迟高

| 检查项 | 命令 | 说明 |
|--------|------|------|
| GPU 利用率 | `nvidia-smi` | 低利用率可能意味着 CPU 瓶颈 |
| 批大小 | 检查KServe config | 增加 max_batch_size |
| 请求队列 | `kubectl describe isvc` | 队列过深需扩容 |
| 模型量化 | 检查模型格式 | 使用 INT8/FP8 量化 |

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# KServe 推理延迟监控
kubectl exec -it <pod> -- curl localhost:8080/metrics | grep prediction_latency

# 优化建议
# 1. 启用 GPU Direct Storage
# 2. 使用连续批处理 (Continuous Batching)
# 3. 启用 Flash Attention
```
### 2.4 Triton 推理服务

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# Triton 日志
kubectl logs -f <triton-pod> | tail -f

# 常见 Triton 配置问题
# TritonServer: "Model not found"
# → 检查 model repository 路径

# TritonServer: "Invalid model configuration"
# → 检查 config.pbtxt 格式

# 性能分析
kubectl exec -it <triton-pod> -- tritonserver --model-repository=/models --metrics-port=8002
```
---

## 3. 数据处理问题 (Spark/Flink)

### 3.1 Spark on K8s 问题

| 症状 | 诊断命令 | 根因 | 修复 |
|------|---------|------|------|
| Driver/Pod OOM | `kubectl describe pod spark-driver` | Driver memory 不足 | 增加 spark.driver.memory |
| Executor 无法启动 | `kubectl logs spark-executor-xxx` | 镜像问题或资源不足 | 检查资源配额 |
| 任务卡住 | `kubectl exec spark-driver -- yarn app -list` | NM/RM 连接问题 | 检查 Spark 集群配置 |
| 数据倾斜 | Spark UI Metrics | 分区不均 | 重新分区或加盐 |

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# Spark on K8s 常用命令
kubectl exec -it spark-driver -- spark-submit \
  --master k8s://https://kubernetes.default.svc \
  --deploy-mode cluster \
  --conf spark.kubernetes.container.image=<image> \
  --conf spark.executor.memory=4g \
  --conf spark.kubernetes.executor.request.cores=1 \
  /opt/spark/examples/src/main/python/pi.py

# 检查 Spark UI
kubectl port-forward spark-driver-ui 4040:4040
```
### 3.2 Flink 作业问题

| 症状 | 诊断命令 | 根因 | 修复 |
|------|---------|------|------|
| JobManager Pod OOM | `kubectl describe pod flink-jobmanager` | 内存配置不足 | 增加 memory 配置 |
| TaskManager 失败 | `kubectl logs flink-taskmanager-xxx` | 计算错误或资源不足 | 检查 TaskManager 日志 |
| 检查点失败 | Flink Web UI | 状态后端问题 | 检查点配置 |
| 背压严重 | Flink Web UI Backpressure | 数据倾斜或算子瓶颈 | 调整并行度 |

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# Flink 常用诊断
kubectl exec -it flink-jobmanager -- flink list   # 列出运行中的作业
kubectl exec -it flink-jobmanager -- flink cancel <job_id>  # 取消作业
kubectl exec -it flink-jobmanager -- flink savepoint  # 创建检查点
```
---

## 4. GPU 调度与分配问题

### 4.1 GPU 调度失败

| 症状 | 诊断命令 | 根因 | 修复 |
|------|---------|------|------|
| Pod 无法调度到 GPU 节点 | `kubectl describe pod` | 无 GPU 节点或打污点 | `kubectl label nodes <node> nvidia.com/gpu=true` |
| 多个 GPU 请求分配失败 | `kubectl describe node` | GPU 碎片化 | 清理占用 GPU 的 Pod |
| MIG 模式冲突 | `nvidia-smi mig -lgip` | MIG 配置不一致 | 重启 kubelet 或重置 GPU |

### 4.2 DCGM 监控数据缺失

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# DCGM exporter 检查
kubectl get pods -n monitoring -l app=dcgm-exporter
kubectl logs dcgm-exporter-xxx -n monitoring

# DCGM 指标缺失可能原因
# 1. NodeFeature 不存在 → 检查 NFD (Node Feature Discovery) 配置
# 2. DCGM Exporter 无法连接 GPU → 检查 CUDA 驱动版本
# 3. 防火墙阻断 → 开放 DCGM 端口 9400

# 验证 DCGM 可用性
docker run --rm --gpus all nvidia/cuda:12.0-base nvidia-smi dmon -s u
```
---

## 5. Kubeflow Pipeline 问题

### 5.1 Pipeline 运行失败

| 症状 | 诊断命令 | 根因 |
|------|---------|------|
| Run 一直 Pending | `kubectl describe run <name> -n kubeflow` | 控制器未就绪 |
| Step 失败 | `kubectl get pods -n kubeflow | grep <step>` | 镜像拉取/资源不足 |
| PVC 挂载失败 | `kubectl describe pvc -n kubeflow` | StorageClass 问题 |
| Tekton PipelineRun 失败 | `kubectl get pr -n kubeflow` | Pipeline 定义错误 |

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# Kubeflow Pipeline 日志
kubectl logs -f <pod-name> -n kubeflow -c main

# 检查 Pipeline 定义
kubectl get pipelinerun <name> -n kubeflow -o yaml

# 常见问题
# 认证失败: 检查 pipeline-sa 的 ServiceAccount 权限
# 镜像拉取失败: 检查 imagePullSecrets
# 资源不足: 调整 step resource limits
```
---

## 6. 快速检查清单

### AI/ML 工作负载 on-call 速查

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 1. 检查 GPU 节点状态
kubectl get nodes -o wide | grep nvidia

# 2. 检查 GPU 利用率
kubectl exec -it <gpu-pod> -- nvidia-smi

# 3. 检查 AI Pod 状态
kubectl get pods -A | grep -E "mpi|spark|flink|kserve|kubeflow" | grep -v Running

# 4. 检查 DCGM 指标
curl localhost:9400/metrics | grep gpu

# 5. 检查分布式训练 NCCL
kubectl exec -it mpi-worker-0 -- nccl-tests/build/all_reduce_perf -b 1G -e 1G -f 2

# 6. 检查 KServe InferenceService
kubectl get isvc -A
kubectl describe isvc <name>

# 7. 检查模型加载日志
kubectl logs -f <inference-pod> -c kserve-container | grep -i "model loaded|error"
```
---

## 7. 升级条件

| 条件 | 操作 |
|------|------|
| 多节点 GPU 通信问题 | 升级网络团队 |
| 模型服务无法启动且无日志 | 升级 K8s 团队 |
| 训练数据丢失/损坏 | 升级数据团队 |
| Kubeflow 控制平面问题 | 升级 SRE |

---

**关联文档**:
- [domain-14-ai-ml-infra/](../domain-14-ai-ml-infra/) — AI 基础设施完整文档
- [domain-10-troubleshooting-diagnostics/](../domain-10-troubleshooting-diagnostics/) — K8s 通用问题排查
- [domain-10-troubleshooting-diagnostics/topic-skills/](../domain-10-troubleshooting-diagnostics/topic-skills/) — 通用运维 Skill

<!-- risk-assessed -->
