---
title: Kubernetes AI/ML 生产运维 Runbook
description: 覆盖 GPU OOM、NCCL 超时、推理延迟、模型回滚、训练检查点、MIG/DRA、多租户配额与 AI 工作负载可观测性的生产级运维手册
summary: 覆盖 GPU OOM、NCCL 超时、推理延迟、模型回滚、训练检查点、MIG/DRA、多租户配额与 AI 工作负载可观测性的生产级运维手册
category: ai-ml-infra
tags:
- production
- best-practices
- playbook
- ai
- ml
- gpu
- nvidia
- nccl
- inference
- checkpoint
- mig
- dra
- quota
- observability
tier: core
created: '2026-07-01'
last_updated: '2026-07'
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 平台工程师
estimated_read_time: 25min
intent_queries:
- Kubernetes AI/ML 生产运维 Runbook 是什么
- GPU OOM 怎么处理
- NCCL 超时怎么排查
- 推理延迟高怎么优化
- 模型回滚怎么做
- MIG DRA 怎么用
- AI 多租户配额
trigger_keywords:
- ai ml ops
- gpu oom
- nccl timeout
- inference latency
- model rollback
- checkpoint
- mig
- dra
- multi-tenant quota
- gpu observability
prerequisites:
- kubectl-basics
- gpu-scheduling-basics
- nvidia-gpu-basics
- prometheus-basics
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
- '1.33'
authors:
- name: KUDIG Team
  role: contributor
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。


# Kubernetes AI/ML 生产运维 Runbook

> **适用范围**: Kubernetes v1.28–v1.33 | **最后更新**: 2026-07 | **文档类型**: 生产运维 Runbook

本 Runbook 面向管理 AI/ML 生产平台的 SRE 与 MLOps 工程师，聚焦 GPU 工作负载的高频故障与运维场景：GPU OOM、NCCL 分布式训练超时、推理延迟飙升、模型版本回滚、训练检查点保护、MIG/DRA 资源切分、多租户 GPU 配额与可观测性。AI 工作负载具有资源密集、故障成本高、调试复杂的特点，必须建立专门的监控、配额与应急响应流程。

---

## 1. 适用场景与范围

- **GPU OOM**：训练或推理 Pod 因显存不足被 OOMKilled，或触发 CUDA out-of-memory。
- **NCCL 超时**：多机多卡训练中 NCCL 集合通信超时，常见于网络、拓扑、IB/RoCE 配置问题。
- **推理延迟**：在线推理服务 P99 延迟超过 SLO，可能由批处理大小、模型版本、GPU 抢占导致。
- **模型回滚**：新模型上线后指标下降，需要快速切回上一版本。
- **检查点保护**：长周期训练任务必须周期性保存 checkpoint，并在节点故障后恢复。
- **MIG/DRA**：NVIDIA MIG 物理切分与 Kubernetes DRA 动态资源分配的生产落地。
- **多租户配额**：按团队/项目分配 GPU、显存、CPU、内存配额，防止 noisy neighbor。

---

## 2. 前置条件与工具

### 2.1 基础设施前提

- 节点已安装 NVIDIA Driver + NVIDIA Container Toolkit + device-plugin。
- 已部署 DCGM Exporter 与 Node Feature Discovery（NFD）。
- 已配置 RuntimeClass（nvidia）与 GPU Operator。
- 训练存储使用高性能并行文件系统（Lustre/BeeGFS/FSx for Lustre）或对象存储 + PVC。

### 2.2 必备工具

| 工具 | 用途 | 推荐版本 |
|------|------|----------|
| `nvidia-smi` | GPU 状态与显存查看 | 随驱动 |
| `dcgmi` | GPU 健康诊断 | 3.x+ |
| `nccl-tests` | NCCL 性能基准 | 2.20+ |
| `kubectl` | Pod/节点/事件查看 | v1.28+ |
| Prometheus + DCGM Exporter | GPU 指标采集 | v3.x+ |
| Volcano/Yunikorn | 批调度与队列 | 1.9+ / 1.5+ |
| KServe/Triton | 推理服务管理 | 0.13+ / 2.48+ |

---

## 3. 标准操作流程

### 3.1 GPU OOM 诊断与处理

#### 现场采集

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 查看 Pod 状态与事件
kubectl describe pod <pod> -n <ns>
kubectl logs <pod> -n <ns> --previous

# 查看节点 GPU 显存
kubectl exec -it <pod> -n <ns> -- nvidia-smi

# 查看 DCGM 指标
kubectl port-forward -n monitoring svc/dcgm-exporter 9400:9400
curl -s localhost:9400/metrics | grep -i memory
```
#### 常见根因

- **Batch Size 过大**：降低 batch size 或启用梯度累积。
- **模型并行策略不当**：改用 ZeRO/FSDP/Tensor Parallelism。
- **显存泄漏**：PyTorch 缓存未释放，添加 `torch.cuda.empty_cache()`。
- **多任务共享 GPU**：MIG 切分不足，或 Request/Limit 未对齐实际显存。

#### 缓解命令

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 降低 batch size（通过环境变量或 ConfigMap）
kubectl set env deployment/<training> -n <ns> BATCH_SIZE=16

# 临时增加 GPU 资源
kubectl patch deployment <training> -n <ns> -p '{"spec":{"template":{"spec":{"containers":[{"name":"train","resources":{"limits":{"nvidia.com/gpu":"2"}}}]}}}}'
```
### 3.2 NCCL 超时排查

NCCL 超时通常表现为 `NCCL_TIMEOUT` 或 `NCCL_WATCHDOG` 报错。

#### 检查清单

1. **网络连通**：
   ```bash
   kubectl exec -it <pod> -n <ns> -- bash
   ping <peer-pod-ip>
   ib_write_bw # 若使用 InfiniBand
   ```
2. **NCCL 调试日志**：
   ```bash
   export NCCL_DEBUG=INFO
   export NCCL_DEBUG_SUBSYS=ALL
   ```
3. **拓扑与 NIC 绑定**：
   ```bash
   nvidia-smi topo -m
   ```
4. **防火墙与安全组**：确保 Pod 间 29500 等 NCCL 端口互通。
5. **IB/RoCE 配置**：检查 `NCCL_IB_DISABLE`、`NCCL_SOCKET_IFNAME` 是否设置正确。

#### 常见修复

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 强制使用 TCP Socket（RoCE 不稳定时临时规避）
kubectl set env job/<distributed-training> NCCL_IB_DISABLE=1

# 指定通信网卡
kubectl set env job/<distributed-training> NCCL_SOCKET_IFNAME=eth0
```
### 3.3 推理延迟优化

#### 诊断命令

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# KServe 推理 Pod 延迟指标
curl http://<inference-service>/v2/models/<model>/metrics

# GPU 利用率与显存
kubectl exec -it <inference-pod> -n <ns> -- nvidia-smi dmon -s u
```
#### 优化手段

| 问题 | 优化手段 |
|------|----------|
| 批处理不足 | 启用 dynamic batching / Triton ensemble |
| 模型过大 | 量化（INT8/FP16）、蒸馏、模型剪枝 |
| GPU 抢占 | 为推理服务设置高 PriorityClass 与独占 GPU |
| 冷启动 | 配置最小副本数与 KServe 预测器预热 |
| 网络延迟 | 推理 Pod 靠近入口部署，使用 topology-aware routing |

### 3.4 模型回滚

KServe InferenceService 回滚：

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 查看历史 revision
kubectl get revision -n <ns> -l serving.kserve.io/inferenceservice=<model>

# 回滚到指定 revision
kubectl patch inferenceservice <model> -n <ns> -p '{"spec":{"predictor":{"canaryTrafficPercent":0,"tensorflow":{"storageUri":"s3://models/v1.2.3"}}}}' --type=merge
```
或采用 Argo Rollouts 管理模型服务：

```bash
argocd app rollback <model-service> <revision>
```

### 3.5 训练检查点保护

#### 检查点保存策略

```yaml
spec:
  containers:
  - name: train
    env:
    - name: CHECKPOINT_DIR
      value: /checkpoints
    - name: SAVE_INTERVAL
      value: "3600"
    volumeMounts:
    - name: checkpoints
      mountPath: /checkpoints
  volumes:
  - name: checkpoints
    persistentVolumeClaim:
      claimName: training-checkpoints
```

#### 故障恢复

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 查看最新检查点
kubectl exec -it <pod> -n <ns> -- ls -lt /checkpoints

# 重新提交训练任务，加载最近检查点
kubectl create job --from=cronjob/<training-cron> resume-training-$(date +%s) -n <ns>
```
### 3.6 MIG 与 DRA 配置

#### MIG 策略

```yaml
apiVersion: v1
kind: ConfigMap
metadata:
  name: nvidia-mig-config
  namespace: gpu-operator
data:
  config.yaml: |
    version: v1
    mig-configs:
      all-1g.5gb:
        - devices: all
          mig-enabled: true
          mig-devices:
            "1g.5gb": 7
```

Pod 请求 MIG：

```yaml
resources:
  limits:
    nvidia.com/mig-1g.5gb: 1
```

#### DRA（Dynamic Resource Allocation）

适用于 K8s v1.32+，需要启用 `DynamicResourceAllocation` feature gate 并部署 DRA driver。

### 3.7 多租户 GPU 配额

```yaml
apiVersion: v1
kind: ResourceQuota
metadata:
  name: team-ai-quota
  namespace: team-ai
spec:
  hard:
    requests.nvidia.com/gpu: 8
    limits.nvidia.com/gpu: 8
    requests.memory: 512Gi
    requests.cpu: 64
```

配合 LimitRange 与 Volcano Queue 实现优先级与抢占策略。

---

## 4. 关键检查点与验证命令

| 检查项 | 命令 | 合格标准 |
|--------|------|----------|
| GPU 节点状态 | `kubectl get nodes -L nvidia.com/gpu.count` | 节点 Ready，GPU 数量正确 |
| Pod GPU 分配 | `kubectl describe pod <pod> -n <ns>` | 已分配 nvidia.com/gpu |
| GPU 利用率 | `curl localhost:9400/metrics \| grep DCGM_FI_DEV_GPU_UTIL` | 符合预期 |
| NCCL 测试 | `all_reduce_perf -b 8M -e 1G -f 2 -g 8` | 带宽接近理论值 |
| 推理延迟 | KServe/Triton metrics | P99 ≤ SLO |
| 检查点完整性 | `ls -lt /checkpoints` | 最近 1 小时内存在 checkpoint |
| 配额使用 | `kubectl describe quota -n <ns>` | 未超限 |

---

## 5. 回滚/应急方案

- **GPU 节点故障**：将该节点设为不可调度并驱逐工作负载。
  ```bash
  kubectl cordon <node>
  kubectl drain <node> --ignore-daemonsets --force --delete-emptydir-data
  ```
- **训练任务 OOM 反复失败**：减小 batch size，启用 CPU offloading，或改用更大显存 GPU 型号。
- **推理服务降级**：立即切回上一模型版本，并通过 PDB + HPA 保证最小副本。
- **NCCL 通信完全中断**：临时改为单机多卡或减小分布式规模，排查网络后再恢复。
- **检查点损坏**：回退到上一个有效 checkpoint，损失部分训练进度。

---

## 6. 风险与注意事项

1. **GPU 驱动与 CUDA 版本匹配**：驱动、CUDA、PyTorch、NCCL 版本不一致会导致隐性性能下降或崩溃。
2. **MIG 与 DRA 共存风险**：同一节点不要混用传统 device-plugin 与 DRA，避免资源计算冲突。
3. **训练任务长周期运行**：超过 24 小时的任务必须配置 checkpoint，节点维护前提前通知并优雅终止。
4. **推理服务冷启动成本**：大模型加载时间长，需配置最小副本与 readiness probe 超时。
5. **多租户隔离**：GPU 显存隔离依赖 MIG/DRA，进程级隔离仍需结合 seccomp/AppArmor。

---

## 7. 相关 Runbook / 推荐阅读

- [[32-发布/package/2026-07-02_18-40/corpus/core/domain-14-ai-ml-infra/03-production-readiness-operations-guide|AI/ML 基础设施 生产就绪运维指南]]
- [[32-发布/package/2026-07-02_18-40/corpus/core/domain-14-ai-ml-infra/03-production-readiness-operations-guide|生产运维 生产就绪运维指南]]
- [[32-发布/package/2026-07-02_18-40/corpus/core/domain-14-ai-ml-infra/01-ai-infra/01-gpu-scheduling-management|GPU 调度与管理]]
- [[32-发布/package/2026-07-02_18-40/corpus/peripheral/domain-14-ai-ml-infra/01-ai-infra/03-gpu-monitoring-dcgm|GPU 监控与 DCGM]]
- [[32-发布/package/2026-07-02_18-40/corpus/core/domain-14-ai-ml-infra/01-ai-infra/02-distributed-training-frameworks|分布式训练框架]]
- [[32-发布/package/2026-07-02_18-40/corpus/peripheral/domain-14-ai-ml-infra/01-ai-infra/08-model-deployment-management|模型部署管理]]
- [[32-发布/package/2026-07-02_18-40/corpus/peripheral/domain-14-ai-ml-infra/01-ai-infra/12-troubleshooting-performance|AI 性能故障排查]]
- [[32-发布/package/2026-07-02_18-40/corpus/peripheral/domain-14-ai-ml-infra/01-ai-infra/15-llm-inference-serving|LLM 推理服务]]


<!-- risk-assessed -->
