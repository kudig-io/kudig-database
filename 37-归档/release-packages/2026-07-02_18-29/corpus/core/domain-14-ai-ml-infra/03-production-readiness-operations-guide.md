---
title: AI/ML 基础设施生产就绪运维指南
description: 面向生产环境的 AI/ML 基础设施检查清单、风险缓解、日常运维与故障排查指南
summary: 面向生产环境的 AI/ML 基础设施检查清单、风险缓解、日常运维与故障排查指南
category: ai-ml-infra
tags:
- production
- best-practices
- ai-ml-infra
- operations
- gpu
- mlops
- inference
- training
tier: core
created: '2026-07-01'
last_updated: '2026-07'
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 平台工程师
- MLOps 工程师
estimated_read_time: 20min
intent_queries:
- AI/ML 基础设施生产就绪运维指南是什么
- 如何按生产环境要求运维 AI/ML 基础设施
trigger_keywords:
- 生产就绪
- 运维指南
- AI/ML
- GPU
- MLOps
- 推理
- 训练
prerequisites:
- kubectl-basics
- gpu-scheduling-basics
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


# AI/ML 基础设施生产就绪运维指南

本指南面向准备将 AI/ML 工作负载接入 Kubernetes 生产环境的 SRE、平台工程师与 MLOps 团队，聚焦 GPU 调度、训练/推理服务、MLOps 平台与 AI Agent 的运行时治理。AI/ML 基础设施与传统业务负载相比，具有资源昂贵、任务周期长、网络拓扑敏感、模型资产敏感等特点，因此生产就绪评审需要覆盖硬件生命周期、调度策略、可观测性、成本控制、安全合规与灾备等多个维度。阅读前建议先了解 [[32-发布/package/2026-07-02_18-29/corpus/core/domain-14-ai-ml-infra/01-ai-infra/01-gpu-scheduling-management|GPU 调度与管理]] 与 [[32-发布/package/2026-07-02_18-29/corpus/peripheral/domain-14-ai-ml-infra/01-ai-infra/03-gpu-monitoring-dcgm|GPU 监控与可观测性]] 中的基础概念。

## 1. 生产环境检查清单

在宣告 AI/ML 平台具备生产服务能力之前，建议逐项核对以下内容：

1. **GPU 驱动与 Operator 版本受控**：冻结 NVIDIA GPU Operator、驱动、Container Toolkit 版本，升级前在金丝雀节点验证，保留回滚 Values 与镜像 tag。生产环境禁止自动升级驱动，避免新版本与现有 CUDA/cuDNN 不兼容导致整节点不可用。
2. **GPU 虚拟化策略落地**：明确训练节点使用整卡或 MIG 大实例，推理/开发节点使用 MIG 小实例或 Time-Slicing，节点标签与 `RuntimeClass` 配置一致。不同 GPU 型号（A100、H100、L40S 等）应分别建立节点池，避免混部导致性能不可预测。
3. **命名空间级配额与优先级**：为每个团队配置 `ResourceQuota`（`nvidia.com/gpu`、`memory`、`ephemeral-storage`）与 `LimitRange`，并启用 `PriorityClass` 区分训练、推理、离线任务。高优先级推理任务可在资源紧张时抢占低优先级离线训练任务。
4. **工作负载 QoS 为 Guaranteed**：训练 Pod 的 CPU/内存/GPU 请求与限制必须相等，避免在节点压力时被驱逐或降频。推理服务同样建议设置为 Guaranteed，以保证延迟稳定性。
5. **DCGM 监控与告警就绪**：所有 GPU 节点部署 DCGM Exporter，配置 XID 错误、ECC 双比特错误、温度、显存、掉卡等关键告警。告警需分级：P0 为硬件错误与掉卡，P1 为显存/温度阈值，P2 为利用率与成本优化。
6. **模型权重与凭据安全**：HuggingFace Token、私有 Registry 凭据、模型下载密钥必须存入 Secret，敏感模型权重使用 KMS/Sealed Secrets 加密，禁止挂载到非必要容器。定期执行 `kubectl auth can-i` 审计，确保非业务 ServiceAccount 无法访问模型 Secret。
7. **网络隔离策略生效**：AI 训练/推理命名空间之间启用 `NetworkPolicy`，推理入口通过 Gateway/VirtualService 暴露，训练节点默认拒绝外部入站。AI Agent 的工具调用侧流量需单独规划 egress 白名单。
8. **RDMA/RoCE 网络 Fabric 验证**：分布式训练节点已验证 RDMA 连通性、PFC/ECCN 配置、`NCCL_IB_DISABLE=0` 生效，并通过 `ib_write_bw` 或 NCCL Tests 达到预期带宽。网络配置变更必须先在测试集群跑通 PyTorch 分布式示例。
9. **PDB 与中断预算**：推理服务配置 `PodDisruptionBudget`，多机训练任务配置 gang 调度策略，避免节点维护时同时中断。对长周期训练任务，建议在维护窗口前协调业务方主动保存 checkpoint。
10. **节点污点与容忍策略**：GPU 节点已打 `nvidia.com/gpu=true:NoSchedule` 等污点，非 GPU 工作负载不会抢占 GPU 资源。CPU 密集型离线任务应通过反亲和性远离 GPU 节点，避免争夺 PCIe 与网络带宽。
11. **镜像供应链可信**：GPU 基础镜像、模型 Serving 镜像存储在私有 Registry，启用镜像签名与 admission 校验，避免使用 `latest` tag。大模型镜像建议将模型权重与业务代码分层，减少每次迭代拉取时间。
12. **控制面容量评估**：评估 API Server 与 etcd 在高 Pod 轮转、大量 PyTorchJob/TrainingJob 创建场景下的 QPS 与存储压力，必要时启用 API Server 优先级与公平性（APF）。建议对训练框架 CRD 的 list/watch 操作设置合理的 rate limit。
13. **日志与指标保留策略**：训练日志、TensorBoard/MLflow 指标、Prometheus 样本保留周期与成本分摊规则已明确，避免长期高基数指标拖垮监控。对高基数标签（如 step、batch_id）应在采集端进行丢弃或聚合。
14. **灾备与checkpoint机制**：模型 Registry、Vector DB、训练 checkpoint 已配置对象存储跨区复制或 Velero 备份，关键推理模型支持多副本滚动回滚。训练任务应配置周期性 checkpoint，并在 spot 实例被回收前触发优雅保存。

## 2. 关键风险与缓解措施

| 风险 | 影响 | 缓解措施与命令 |
|------|------|----------------|
| **GPU 驱动/固件不兼容导致节点 NotReady** | 整批训练任务失败、节点不可调度 | 冻结驱动版本；升级前在节点执行 `nvidia-smi` 与 `nvidia-ctk --version` 验证；使用 GPU Operator 的 `driver.upgradePolicy.autoUpgrade=false`，升级失败时 `kubectl drain <node> --ignore-daemonsets` 后回滚 Helm Values。 |
| **训练任务 OOM 或 NCCL Timeout 导致大规模失败** | 数小时训练成果丢失、集群资源空转 | 为训练 Pod 设置 `requests=limits`；启用周期性 checkpoint 保存到对象存储；NCCL 调试环境变量 `NCCL_DEBUG=INFO`、`NCCL_IB_TIMEOUT=22`、`NCCL_P2P_LEVEL=NVL`；使用 Volcano/Kueue gang 调度避免部分 Pod 启动后 hang 住。 |
| **模型权重与凭据泄露** | 商业模型与数据资产外泄 | 使用 External Secrets Operator 或 Vault 注入 HF Token；对模型 PVC/Secret 启用 RBAC 最小权限；运行 `kubectl auth can-i list secrets --as=system:serviceaccount:default:default -n ai-training` 定期审计。 |
| **RDMA/RoCE 网络拥塞或 PFC 配置错误** | NCCL 性能暴跌、分布式训练极慢 | 在节点执行 `ibdev2netdev`、`cat /sys/class/net/ens*/settings/flow_control` 验证 PFC；Prometheus 监控 `DCGM_FI_PROF_NVLINK_*` 与节点网卡 drop 计数；网络团队按 [[domain-03-networking-traffic/README.md|网络域]] 标准统一 RoCE 配置。 |
| **Spot/抢占实例导致训练中断** | 长任务被强制终止、checkpoint 不完整 | 对 Spot GPU 节点打 `node.kubernetes.io/lifecycle: spot` 标签，训练任务使用 `torch.elastic`/`TorchElastic` 并设置 `--max-restarts`；关键推理节点使用按需实例；配置 `kubectl drain` 前通过 Node Problem Detector 触发 checkpoint 保存。 |

## 3. 日常运维操作

### 3.1 GPU 集群状态巡检

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看 GPU 节点与可调度状态
kubectl get nodes -L nvidia.com/gpu.product,nvidia.com/gpu.count,nvidia.com/mig.config

# 查看 GPU 资源分配情况
kubectl describe node <gpu-node> | grep -A 12 "Allocated resources"

# 查看 GPU Operator 组件状态
kubectl get pods -n gpu-operator
helm list -n gpu-operator

# 登录节点查看 GPU 硬件状态
kubectl debug node/<gpu-node> -it --image=nvcr.io/nvidia/cuda:12.2.0-base-ubuntu22.04 -- nvidia-smi -L
```
### 3.2 训练/推理工作负载巡检

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 按命名空间统计 GPU 使用
kubectl top pods -n ai-training --containers | grep nvidia.com/gpu

# 查看 Pending GPU Pod 的调度事件
kubectl get pods -n ai-training --field-selector=status.phase=Pending -o wide
kubectl describe pod <pending-pod> -n ai-training | grep -E "Events|nvidia.com/gpu|Insufficient"

# 查看训练 Job 运行状态
kubectl get pytorchjob -n ai-training
kubectl get trainingjob -n ai-training
```
### 3.3 监控查询常用 PromQL

```promql
# 集群 GPU 利用率
avg(DCGM_FI_DEV_GPU_UTIL)

# 显存使用率高于 90% 的 GPU
(DCGM_FI_DEV_FB_USED / DCGM_FI_DEV_FB_TOTAL) * 100 > 90

# 按命名空间统计显存占用
sum(DCGM_FI_DEV_FB_USED) by (namespace)

# XID 错误增长
increase(DCGM_FI_DEV_XID_ERRORS[5m]) > 0
```

### 3.4 配额与成本巡检

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看各命名空间 GPU 配额与使用
kubectl get resourcequota -n ai-training
kubectl describe resourcequota team-a-quota -n ai-training

# 按标签统计当前运行中的 GPU Pod
kubectl get pods -A -o custom-columns='NS:.metadata.namespace,NAME:.metadata.name,GPU:.spec.containers[*].resources.limits.nvidia\.com/gpu,NODE:.spec.nodeName' | grep -v '<none>'
```
### 3.5 推理服务健康检查

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看推理 Deployment/Service 状态
kubectl get deployments -n ai-inference
kubectl rollout status deployment/<model-serving> -n ai-inference

# 测试推理端点（示例为 vLLM/Triton 服务）
curl -X POST http://<model-serving>.ai-inference.svc.cluster.local:8000/v1/completions \
  -H "Content-Type: application/json" \
  -d '{"model": "llm-model", "prompt": "hello", "max_tokens": 10}'
```
### 3.6 AI Agent 运行时检查

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看 Agent Pod 与工具调用日志
kubectl get pods -n ai-agents
kubectl logs -n ai-agents -l app=agent-core --tail=200 | grep -i "tool\|error\|latency"

# 验证 Agent ServiceAccount 权限范围
kubectl auth can-i --list --as=system:serviceaccount:ai-agents:agent-sa -n ai-agents
```
### 3.7 GPU 节点维护

> **🔴 高风险操作警告**
>
> 下方命令属于不可逆或高影响操作，执行前请确认：
> - 已备份关键数据与配置
> - 处于批准的变更窗口期
> - 已获得相关责任人授权
> - 已准备回滚或恢复方案
> - 目标集群、Namespace、节点/资源名称正确无误

``` bash
# 🔴 高风险：可能造成数据丢失或服务中断，执行前需备份、变更审批与回滚方案
# 标记节点不可调度并排空（保留 DaemonSet）
kubectl cordon <gpu-node>
kubectl drain <gpu-node> --ignore-daemonsets --delete-emptydir-data --force --pod-selector='app notin (nvidia-device-plugin,dcgm-exporter)'

# 维护完成后恢复
kubectl uncordon <gpu-node>
```
## 4. 故障排查速查

| 现象 | 可能原因 | 确认命令 | 修复/缓解 |
|------|----------|----------|-----------|
| Pod 长期处于 `Pending` | GPU 资源不足、污点不匹配、MIG 配置错误 | `kubectl describe pod <pod>`、`kubectl get nodes -L nvidia.com/gpu.product` | 扩容 GPU 节点池、修正 `nodeSelector/tolerations`、重新应用 MIG 配置 |
| 训练容器 OOMKilled | 显存超限、模型/批次过大、内存泄漏 | `kubectl describe pod`、`nvidia-smi dmon` | 减小 batch size、启用梯度累积、使用 `PYTORCH_CUDA_ALLOC_CONF`、增加 GPU 显存 |
| NCCL Timeout / 分布式 hang | RDMA 不通、PFC 配置不一致、Pod 未同时启动 | `kubectl logs <worker-0>`、节点 `ib_write_bw`、检查 `NCCL_DEBUG=INFO` | 修复网络 Fabric、使用 Volcano gang 调度、调整 `NCCL_IB_TIMEOUT` |
| GPU XID 错误或 ECC 双比特错误 | 硬件故障、驱动异常、过热 | `dmesg \| grep -i "nvrm\|xid"`、`nvidia-smi -q -d ECC` | 立即 cordon 节点、隔离故障 GPU、联系硬件更换 |
| GPU 利用率低（<30%） | 数据加载瓶颈、CPU 预处理不足、任务未充分使用 Tensor Core | PromQL `DCGM_FI_DEV_GPU_UTIL`、`DCGM_FI_PROF_PIPE_TENSOR_ACTIVE` | 增加 DataLoader workers、启用 BF16/FP16、调整模型并行度 |
| 推理延迟突增 | 批处理过大、GPU 降频、节点资源争用、模型加载失败 | Grafana 查看 p99 延迟与 `DCGM_FI_DEV_CLOCK_THROTTLE_REASONS` | 调整 max batch size、增加推理副本、启用 HPA/KEDA、检查模型挂载 |
| 镜像拉取失败（大模型镜像） | 镜像过大超时、私有 Registry 凭据缺失、网络限速 | `kubectl describe pod`、`kubectl get events` | 使用镜像缓存 DaemonSet、配置 `imagePullSecrets`、拆分模型权重与业务镜像 |
| AI Agent 调用工具失败 | 工具权限过大/过小、RBAC 未授权、网络隔离 | `kubectl auth can-i`、Agent Pod 日志 | 收紧/补充 RBAC、配置 NetworkPolicy、启用工具审计日志 |
| Kubeflow/MLflow 无法访问对象存储 | Secret 失效、网络策略阻断、IAM 权限不足 | `kubectl get secret -n mlflow`、`kubectl logs -n mlflow` | 轮换对象存储凭据、检查 egress NetworkPolicy、验证云厂商 IAM |
| Vector DB 查询延迟升高 | 索引未优化、内存不足、副本数不足 | 查看向量库 Pod 资源使用与日志 | 重建索引、扩容内存、增加副本或分片 |
| Spot 实例导致训练频繁重启 | 实例被回收、checkpoint 保存不及时 | `kubectl get events --field-selector reason=Preempting` | 使用按需实例承载长任务、缩短 checkpoint 间隔、配置 TorchElastic 重启 |
| GPU Operator Pod 反复 Crash | 驱动签名、内核版本不匹配、CUDA 路径冲突 | `kubectl logs -n gpu-operator -l app=gpu-operator` | 核对内核与驱动兼容性矩阵、回滚 Operator 版本 |
| 推理模型版本回滚失败 | 新版本模型文件损坏、配置不兼容 | `kubectl describe pod`、`kubectl logs <serving-pod>` | 切换 Serving 到旧版本镜像与模型 tag、校验模型文件哈希 |

## 5. 与其他域的协作边界

AI/ML 基础设施的生产就绪不能孤立完成，需要与以下域紧密协作，并在变更评审阶段明确责任边界：

- **[[domain-02-workloads-applications/README.md|domain-02-workloads-applications]]**：负责训练/推理 Pod 的 QoS、HPA/VPA、PriorityClass、PDB、StatefulSet/Deployment 模式，以及 Java/Python 应用的容器化最佳实践。AI 平台团队应提供 GPU 资源声明模板，由应用团队按模板提交工作负载。
- **[[domain-03-networking-traffic/README.md|domain-03-networking-traffic]]**：负责 RDMA/RoCE 网络 Fabric、GPU 节点间大带宽低延迟通信、推理入口的 Gateway/Service Mesh、AI 命名空间之间的 NetworkPolicy 隔离。分布式训练出现 NCCL 问题时，应由网络团队主导 Fabric 侧排查。
- **[[domain-05-security-compliance/README.md|domain-05-security-compliance]]**：负责模型权重与 HF Token 的 Secret 生命周期、Pod Security Standards、运行时威胁检测（Falco/Tetragon）、镜像签名与供应链安全。AI 团队不得自行在代码中硬编码 API Token 或私钥。
- **[[domain-06-observability/README.md|domain-06-observability]]**：负责 DCGM/Prometheus/Grafana 采集体系、训练任务自定义指标、SLO/SLI 定义、日志聚合与告警路由。AI 平台应暴露模型服务 RED 指标，由可观测性团队统一接入告警平台。
- **[[domain-07-platform-engineering/README.md|domain-07-platform-engineering]]**：负责多租户 GPU 配额、Kubecost 成本分摊、平台级升级与变更窗口、生产就绪评审（PRR）模板。AI 服务上线前必须完成平台工程团队组织的 PRR。
- **[[domain-09-reliability-engineering/README.md|domain-09-reliability-engineering]]**：负责训练 checkpoint 与模型 Registry 的灾备、PodDisruptionBudget、混沌工程（如 GPU 节点故障注入）、事后复盘流程。建议每季度执行一次 GPU 节点故障演练。
- **[[domain-11-production-operations/README.md|domain-11-production-operations]]**：负责 GPU FinOps、Spot/按需实例混合策略、事件响应与值班 Runbook。AI 平台应提供按团队/项目的 GPU 使用量报表，支撑成本分摊。
- **[[domain-13-container-runtime/README.md|domain-13-container-runtime]]**：负责 NVIDIA Container Toolkit/CDI、GPU 镜像分层与缓存、镜像仓库高可用与签名验证。AI 镜像应遵循容器运行时团队定义的基础镜像与扫描策略。

## 6. 推荐阅读

以下文档可与本指南配合使用，覆盖从 GPU 调度、训练推理到 AI Agent 部署与安全治理的完整链路。

### 同域核心资料

- [[domain-14-ai-ml-infra/README.md|AI/ML Infrastructure 总览]]
- [[32-发布/package/2026-07-02_18-29/corpus/core/domain-14-ai-ml-infra/01-ai-infra/01-gpu-scheduling-management|GPU 调度与管理]]
- [[32-发布/package/2026-07-02_18-29/corpus/peripheral/domain-14-ai-ml-infra/01-ai-infra/03-gpu-monitoring-dcgm|GPU 监控与可观测性]]
- [[32-发布/package/2026-07-02_18-29/corpus/core/domain-14-ai-ml-infra/01-ai-infra/02-distributed-training-frameworks|分布式训练框架]]
- [[32-发布/package/2026-07-02_18-29/corpus/peripheral/domain-14-ai-ml-infra/01-ai-infra/15-llm-inference-serving|LLM 推理服务]]
- [[32-发布/package/2026-07-02_18-29/corpus/peripheral/domain-14-ai-ml-infra/01-ai-infra/09-ai-security-model-protection|AI 安全模型保护]]
- [[32-发布/package/2026-07-02_18-29/corpus/peripheral/domain-14-ai-ml-infra/01-ai-infra/12-troubleshooting-performance|性能故障排查]]
- [[domain-14-ai-ml-infra/AI-Agents/09-production-deployment-guide.md|AI Agent 生产部署指南]]
- [[domain-14-ai-ml-infra/AI-Agents/10-security-guardrails.md|AI Agent 安全护栏]]

### 跨域协作资料

- [[domain-02-workloads-applications/README.md|domain-02-workloads-applications]]
- [[domain-03-networking-traffic/README.md|domain-03-networking-traffic]]
- [[domain-05-security-compliance/README.md|domain-05-security-compliance]]
- [[domain-06-observability/README.md|domain-06-observability]]
- [[domain-07-platform-engineering/README.md|domain-07-platform-engineering]]
- [[domain-09-reliability-engineering/README.md|domain-09-reliability-engineering]]

---

*本指南依据 2026-07-01 域内容缺口分析中的 `domain-14-ai-ml-infra` 建议项编写，重点补齐生产就绪检查清单、风险缓解、日常运维与故障排查速查。*


<!-- risk-assessed -->
