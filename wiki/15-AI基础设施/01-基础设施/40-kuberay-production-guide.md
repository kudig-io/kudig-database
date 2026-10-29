---
title: KubeRay 生产实践指南
description: Ray on Kubernetes 生产实践 — KubeRay 三大 CRD（RayCluster/RayJob/RayService）字段详解、自动扩缩容与 HPA/CA/Karpenter/Kueue/Volcano 协同、vLLM on Ray 推理、Ray Data 数据管道、head 故障影响域与 GCS 排查、LLM 训练平台弹性算力池案例
summary: KubeRay v1.7 生产实践全解 — 为什么 Ray 需要 actor/task 模型编排、三大 CRD 生产级 YAML、Autoscaler v2 扩缩容参数、批调度集成（Kueue 准入 + Volcano gang）、Ray Serve 网络与端口模型、head 挂掉/OOM/GCS 故障排查流程、监控指标与 LLM 训练平台弹性算力池实战
category: ai-infra
tags:
- ai
- ml
- llm
- kubernetes
- kuberay
- ray
- raycluster
- rayjob
- rayservice
- autoscaling
- distributed-training
- vllm
- kueue
- volcano
- gpu
tier: core
created: '2026-10-14'
last_updated: '2026-10-14'
difficulty: advanced
audience:
- AI 工程师
- MLOps 工程师
- SRE
- 平台工程师
estimated_read_time: 30min
intent_queries:
- KubeRay 怎么在生产环境部署 Ray 集群
- RayCluster RayJob RayService 三个 CRD 怎么选
- KubeRay 自动扩缩容怎么和 Karpenter Kueue 配合
- Ray head 节点挂了会怎样怎么排查
- vLLM 跑在 Ray Serve 上怎么配置
trigger_keywords:
- KubeRay
- RayCluster
- RayJob
- RayService
- Ray Autoscaler
- GCS
- Ray Serve
- vLLM on Ray
- 弹性训练
prerequisites:
- kubectl-basics
- crd-operator-basics
- gpu-scheduling-basics
versions:
  kuberay: v1.7.1
  ray: 2.59.0
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备 RBAC 权限（KubeRay Operator 需管理 ray.io API 组与 Pod/Service）；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险（只读/信息收集，无副作用）。

# KubeRay 生产实践指南

> **适用版本**: KubeRay Operator v1.7.1 / Ray 2.59.0（2026-10 核对 GitHub Releases）；Kubernetes 1.28–1.37
> **难度**: 高级
> **前置知识**: [[wiki/15-AI基础设施/01-基础设施/05-distributed-training-frameworks.md|分布式训练框架]]、[[wiki/15-AI基础设施/05-K8s-AI基础设施/06-kuberay-distributed-computing|KubeRay 分布式计算入门]]

---

## 📋 目录

- [一、概述：为什么 Ray 需要自己的编排层](#一概述为什么-ray-需要自己的编排层)
- [二、架构设计与三大 CRD 字段详解](#二架构设计与三大-crd-字段详解)
- [三、生产级配置与操作指南](#三生产级配置与操作指南)
- [四、自动扩缩容与批调度集成](#四自动扩缩容与批调度集成)
- [五、故障排查与诊断流程](#五故障排查与诊断流程)
- [六、最佳实践与反模式](#六最佳实践与反模式)
- [七、性能调优与监控指标](#七性能调优与监控指标)
- [八、版本兼容性、升级与生产案例](#八版本兼容性升级与生产案例)
- [九、参考链接与延伸阅读](#九参考链接与延伸阅读)

---

## 一、概述：为什么 Ray 需要自己的编排层

### 1.1 Ray 的计算模型：task/actor vs Pod

Ray 是面向 AI 的通用分布式计算框架，它的调度单元不是容器，而是**语言级的 task 与 actor**：

| 维度 | Ray（task/actor 模型） | Kubernetes（Pod 模型） |
|------|----------------------|----------------------|
| 调度粒度 | 毫秒级的函数调用 / 长生命周期有状态对象 | 秒级的容器生命周期 |
| 资源计量 | 逻辑资源（`num_cpus`、`num_gpus`、自定义资源），进程内记账 | 物理资源（CPU/内存/扩展资源），cgroup 记账 |
| 放置策略 | 同节点亲和（locality）、跨节点亲和（placement group）、拓扑感知 | NodeAffinity/拓扑分布约束，表达能力不同 |
| 弹性 | 应用内秒级扩缩 worker，actor 可迁移 | Pod 级扩缩，分钟级 |
| 状态 | actor 状态由 Ray 管理，支持 `max_restarts` 重启恢复 | Pod 状态由 kubelet 管理 |

一个 Ray 集群内可能同时跑着：数据预处理（Ray Data）、超参搜索（Ray Tune）、分布式训练（Ray Train）、在线推理（Ray Serve）。这些 workload 共享同一个集群的 GPU 池，由 **Ray 内置 Autoscaler 按应用资源请求**决定要不要向 Kubernetes 申请更多 Pod。这就是核心矛盾：**Kubernetes 只能看到 Pod，看不到 task/actor 的排队与饥饿**；把 Ray 集群当成一个普通 Deployment 去 HPA，等于让外行指挥内行。

### 1.2 裸 Kubernetes 跑 Ray 的痛点

不用 KubeRay、直接手写 StatefulSet/Deployment 起 Ray 集群时，生产环境会撞上这些问题：

1. **生命周期管理割裂**：head 挂了谁负责重建？worker 组怎么独立扩缩？job 跑完集群谁来回收？需要自己写一堆脚本。
2. **服务发现硬编码**：worker 需要 head 地址，head 重建后 IP 变化，需要自己维护 head Service 与启动参数注入。
3. **Autoscaler 与 K8s 控制器打架**：Ray Autoscaler 会直接调 K8s API 创建/删除 worker Pod（in-tree autoscaling），与手工管理的 Deployment 互相覆盖。
4. **job 语义缺失**：训练任务应该是"提交-运行-退出-回收"的一等公民，原生 Job 又无法承载"先起集群再跑脚本"的两段式。
5. **在线服务零支持**：Ray Serve 的部署、健康检查、流量切换、多版本 rollout 都需要 K8s 侧配合。

### 1.3 KubeRay 是什么

KubeRay 是 Ray 官方维护的 Kubernetes Operator（ray.io API 组），用 CRD 把 Ray 集群的生命周期、任务提交、在线服务全部声明式化。它不做计算调度——**task/actor 仍由 Ray 内部调度，KubeRay 只负责把"Ray 需要多少台机器"翻译成 Pod**。两者职责边界：

```
用户 YAML (RayCluster/RayJob/RayService)
      │
      ▼
KubeRay Operator ──创建/修复──▶ head Pod + worker Pod + head Service
      ▲                             │
      │ 监听资源使用/待调度任务        ▼
      └──── Ray Autoscaler（跑在 head Pod 内）──── Ray 内部 task/actor 调度
```

---

## 二、架构设计与三大 CRD 字段详解

### 2.1 Operator 析构

KubeRay Operator（单 Deployment，默认 1 副本，生产可开 leader election 多副本）监听三类 CRD 并持续 reconcile：

- **RayCluster Reconciler**：保证实际 Pod 数量收敛到期望（head 唯一、每组 worker 在 min/max 之间）、注入 head 地址、维护 head Service、转发 Ray Autoscaler 的副本决策。
- **RayJob Reconciler**：先起 RayCluster（内嵌 spec 或 `clusterSelector` 引用已有集群），再拉起 submitter Pod 通过 Dashboard REST API（8265 端口）提交作业，镜像 job 状态到 CRD status，结束后按策略回收。
- **RayService Reconciler**：管理 RayCluster + Serve 应用（`serveConfigV2`），做健康探测、双集群灰度（更新时先起新集群再切流量）、维护 serve Service。

### 2.2 RayCluster 关键字段

```yaml
apiVersion: ray.io/v1
kind: RayCluster
metadata:
  name: llm-train          # head Service 将命名为 llm-train-head-svc
spec:
  rayVersion: '2.59.0'     # 仅作展示/校验提示，不强制
  enableInTreeAutoscaling: true   # 允许 Ray Autoscaler 操作 K8s 资源（RBAC 需放行）
  autoscalerOptions:
    upscalingMode: Default        # Default=平稳扩容 / Aggressive=立即扩到 max
    idleTimeoutSeconds: 60        # 空闲 worker 多久后回收
  headGroupSpec:
    serviceType: ClusterIP
    rayStartParams:
      dashboard-host: '0.0.0.0'
      num-cpus: '0'               # head 不跑业务 task，避免被抢占关键组件资源
    template:                     # 标准 PodTemplate，写资源/卷/容忍/亲和
      spec: {...}
  workerGroupSpecs:
  - groupName: gpu-group
    replicas: 4          # 初始副本
    minReplicas: 2       # Autoscaler 扩缩下界（downscaling 目标）
    maxReplicas: 32      # Autoscaler 扩缩上界（upscaling 目标）
    numOfHosts: 1        # 每个 Pod 承载几个主机（多网卡推理拓扑用，训练保持 1）
    rayStartParams:
      num-gpus: '8'      # 声明给 Ray 的逻辑 GPU 数，必须与节点真实卡数一致
    template: {...}
```

字段要点：

| 字段 | 生产含义 |
|------|---------|
| `enableInTreeAutoscaling` | 开启后 head 内的 Autoscaler 进程可直接创建/删除 worker Pod；关掉则纯静态集群 |
| `minReplicas`/`maxReplicas` | 弹性边界；Kueue 接管时通常把 `replicas` 交给 Kueue 准入后填充 |
| `numOfHosts` | vLLM on Ray 的 PD 分离（prefill/decode）跨机 tensor-parallel 拓扑会用到 |
| `template.spec.schedulerName` | 可设为 `volcano`，配合队列标签走 gang 调度 |
| `ray.io/ft: "true"` 注解 | 开启 Ray 容错模式（GCS FT），要求 Ray ≥ 2.0 且作业幂等 |

### 2.3 RayJob 关键字段

```yaml
apiVersion: ray.io/v1
kind: RayJob
metadata:
  name: sft-qwen3
spec:
  submissionMode: K8sJobMode      # 由 submitter Pod 提交（默认）；InteractiveMode 手动
  shutdownAfterJobFinishes: true  # 作业终态后删除内嵌集群（省钱的关键开关）
  ttlSecondsAfterFinished: 86400  # CRD 保留 1 天供审计
  entrypoint: python train_sft.py --epochs 3
  runtimeEnvYAML: |               # 依赖注入，避免打巨型镜像
    pip:
      - torch==2.9.0
      - transformers
  submitterPodTemplate: {...}     # 可复用训练镜像与镜像拉取密钥
  rayClusterSpec: {...}           # 与 RayCluster.spec 相同结构
  # clusterSelector:              # 二选一：复用常驻集群则用标签选择器
  #   ray.io/cluster: shared-pool
```

RayJob 的 status 会镜像 Ray 作业状态（`PENDING/RUNNING/STOPPED/SUCCEEDED/FAILED`），并产出 `jobId`、`dashboardURL`，是平台侧对接工单系统的关键字段。

### 2.4 RayService 关键字段

```yaml
apiVersion: ray.io/v1
kind: RayService
metadata:
  name: vllm-serve
spec:
  serviceUnhealthySecondThreshold: 300   # serve 健康失败多久判定不健康（影响滚动）
  deploymentUnhealthySecondThreshold: 300
  serveService:                   # 对外推理流量入口 Service（KubeRay 维护）
    spec:
      type: ClusterIP
      selector: {}                # 留空，由 KubeRay 注入新集群标签实现流量切换
  rayClusterSpec: {...}
  serveConfigV2: |                # Serve 应用声明（yaml 字符串）
    applications:
      - name: llm
        import_path: llm.deployment:entrypoint
        route_prefix: /
        runtime_env:
          env_vars: {MODEL_ID: Qwen/Qwen3-32B}
```

更新 RayService 时，KubeRay 默认策略是**原地更新**（Serve 多版本 rollout）；只有当健康探测超时才回退到"双集群切换"——先起新 RayCluster，Serve 就绪后把 `serveService` 的 selector 指向新集群，再删旧集群。生产建议显式设置两个 `UnhealthySecondThreshold`，否则默认值太长会拖慢发布。

### 2.5 三大 CRD 选型速查

| 场景 | 选择 | 理由 |
|------|------|------|
| 常驻共享算力池、多团队复用 | RayCluster | 集群即资源池，成本可控 |
| 训练/批处理任务（跑完即走） | RayJob | 生命周期绑定作业，自动回收 |
| 在线推理服务 | RayService | 健康、灰度、Service 全托管 |
| 平台层统一排队配额 | RayJob/RayCluster + Kueue | 见第四章 |

---

## 三、生产级配置与操作指南

### 3.1 安装 Operator

```bash
# 🟢 低风险：仅添加仓库
helm repo add kuberay https://ray-project.github.io/kuberay/
helm repo update

# 🟡 中风险：安装到独立 namespace（建议锁版本）
helm install kuberay-operator kuberay/kuberay-operator \
  --version 1.7.1 \
  -n ray-system --create-namespace \
  --set leaderElection.enabled=true \
  --set resources.limits.cpu=500m \
  --set resources.limits.memory=512Mi

# 🟢 验证
kubectl -n ray-system get pods -l app.kubernetes.io/name=kuberay-operator
```

### 3.2 RayCluster 完整生产 YAML（GPU 训练池）

```yaml
apiVersion: ray.io/v1
kind: RayCluster
metadata:
  name: llm-train-pool
  labels:
    ray.io/cluster-role: training
spec:
  rayVersion: '2.59.0'
  enableInTreeAutoscaling: true
  autoscalerOptions:
    upscalingMode: Default
    idleTimeoutSeconds: 120
  headGroupSpec:
    serviceType: ClusterIP
    template:
      spec:
        containers:
        - name: ray-head
          image: rayproject/ray:2.59.0-py311-cu128
          resources:
            limits: {cpu: '4', memory: 16Gi}
            requests: {cpu: '4', memory: 16Gi}
          ports:
          - containerPort: 6379   # GCS
          - containerPort: 8265   # Dashboard
          - containerPort: 10001  # Ray Client
  workerGroupSpecs:
  - groupName: h100-group
    replicas: 4
    minReplicas: 0
    maxReplicas: 16
    minSuccessSeconds: 120       # 新 worker 就绪多久才算稳定（防抖）
    rayStartParams: {num-gpus: '8'}
    template:
      metadata:
        labels: {pool: h100-training}
      spec:
        schedulerName: volcano   # 可选：接 Volcano gang（见 4.4）
        nodeSelector: {nvidia.com/gpu.product: NVIDIA-H100-80GB-HBM3}
        tolerations:
        - key: nvidia.com/gpu
          operator: Exists
          effect: NoSchedule
        containers:
        - name: ray-worker
          image: rayproject/ray:2.59.0-py311-cu128
          resources:
            limits: {cpu: '96', memory: 800Gi, nvidia.com/gpu: '8'}
            requests: {cpu: '96', memory: 800Gi, nvidia.com/gpu: '8'}
          volumeMounts:
          - mountPath: /dev/shm
            name: dshm           # NCCL/PyTorch 共享内存必需
        volumes:
        - name: dshm
          emptyDir: {medium: Memory, sizeLimit: '32Gi'}
```

### 3.3 RayJob 完整 YAML（提交即回收）

```yaml
apiVersion: ray.io/v1
kind: RayJob
metadata:
  name: lora-finetune-qwen
spec:
  shutdownAfterJobFinishes: true
  ttlSecondsAfterFinished: 21600
  entrypoint: >-
    python -m train.lora_sft
    --model Qwen/Qwen3-14B --lora-rank 64
    --data s3://ml-corpus/sft/ --epochs 2
  runtimeEnvYAML: |
    pip:
      - torch==2.9.0
      - peft
      - datasets
    env_vars:
      HF_ENDPOINT: https://hf-mirror.example.com
      RAY_TRAIN_ENABLE_...: ""
  submitterPodTemplate:
    spec:
      containers: [{name: submitter, image: 'rayproject/ray:2.59.0-py311-cu128'}]
  rayClusterSpec:
    rayVersion: '2.59.0'
    headGroupSpec: {...}     # 同 3.2 精简版
    workerGroupSpecs: [...]
```

### 3.4 RayService（vLLM on Ray 推理）

```yaml
apiVersion: ray.io/v1
kind: RayService
metadata:
  name: vllm-qwen3
spec:
  serviceUnhealthySecondThreshold: 300
  deploymentUnhealthySecondThreshold: 300
  serveService:
    spec: {type: ClusterIP}
  rayClusterSpec:
    rayVersion: '2.59.0'
    headGroupSpec: {...}
    workerGroupSpecs:
    - groupName: infer-group
      replicas: 2
      minReplicas: 1
      maxReplicas: 8
      rayStartParams: {num-gpus: '8'}
      template: {...}
  serveConfigV2: |
    applications:
      - name: vllm
        import_path: ray.llm.entrypoints.vllm:build_openai_apis
        route_prefix: /
        runtime_env:
          env_vars:
            MODEL_ID: Qwen/Qwen3-32B-Instruct
            TENSOR_PARALLEL_DEGREE: '4'
```

更多推理引擎侧调优（KV cache、量化、PD 分离）见 [[wiki/15-AI基础设施/05-K8s-AI基础设施/03-vllm-inference-serving-production.md|vLLM 生产部署]] 与 [[wiki/15-AI基础设施/05-K8s-AI基础设施/16-sglang-lmdeploy-inference.md|SGLang 与 LMDeploy]]。

### 3.5 网络与服务发现

KubeRay 自动创建的 head Service 名为 `<cluster-name>-head-svc`，端口约定：

| 端口 | 用途 | 暴露建议 |
|------|------|---------|
| 6379 | GCS（Ray 内部状态服务，worker 连它入集群） | 仅集群内 |
| 8265 | Dashboard / Job 提交 REST API | 平台内网 + Ingress |
| 10001 | Ray Client（`ray.init("ray://...")`） | 按需 |
| 8000 | Ray Serve HTTP 入口（RayService 场景经 serve-svc） | 经 serveService/网关对外 |

worker 入网机制：KubeRay 向 worker Pod 注入 head Service 名称环境变量，`ray start` 通过 `ray://<head-svc>:6379` 解析——**head 重建换 IP 不影响 worker 重连**，这是必须用 head Service 而非 head Pod IP 的原因。跨 namespace 访问 Dashboard 时注意 NetworkPolicy 放行 8265。高性能训练网络（NCCL/RDMA）另见 [[wiki/15-AI基础设施/05-K8s-AI基础设施/10-rdma-infiniband-gpudirect-networking|RDMA 高性能网络]]。

---

## 四、自动扩缩容与批调度集成

### 4.1 Ray Autoscaler：upscaling/downscaling

Ray Autoscaler（Ray ≥ 2.44 镜像默认 v2 版本，KubeRay 亦可用注解 `ray.io/autoscaler-type` 显式指定）运行在 head Pod 内，决策回路：

- **upscaling**：应用待调度 task/actor 的资源请求 + 当前利用率（`targetUtilizationFraction`，默认约 80%）→ 计算缺口 → 以 `upscalingSpeed`（每轮最多新增当前节点数的比例）分批把 worker 组拉向 `maxReplicas`。`Aggressive` 模式则一步到位。
- **downscaling**：节点空闲超过 `idleTimeoutSeconds`（KubeRay 侧字段）后按利用率阈值逐台回收，绝不低于 `minReplicas`。
- 所有动作最终体现为 KubeRay 更新 RayCluster 的 `replicas` 字段——**Autoscaler 不直接创建 Pod，收敛由 Operator 保证**，这避免了早期版本两者打架的问题。

反直觉但重要：downscaling 杀的是"空闲"节点，但如果 actor 被调度到某节点且 `max_restarts=0`，节点回收即 actor 永久死亡。弹性训练必须使用 Ray Train 检查点 + 容错语义。

### 4.2 与 HPA / Cluster Autoscaler / Karpenter 协同

| 层 | 工具 | 职责 | 是否冲突 |
|----|------|------|---------|
| 应用层 | Ray Autoscaler | 决定 Ray 集群要几个 worker Pod | 唯一决策者 |
| Pod 弹性层 | HPA | **不要**对 Ray Pod 使用 | 冲突：HPA 缩容会绕过 Ray 语义直接杀节点 |
| 节点层 | Cluster Autoscaler / Karpenter | 响应 Pending Pod 扩节点、缩空闲节点 | 互补 |

标准分工：Ray Autoscaler → 创建 Pending 的 worker Pod → Karpenter 按 NodePool/NodeClaim 供货 → 空闲节点由 Karpenter 整机回收。给训练 worker Pod 打上 `do-not-disrupt`（Karpenter disruption 注解）避免训练中被腾挪。推理场景的请求驱动伸缩不要用 HPA 压 RayService，而是调大 worker 组 `maxReplicas` 让 Autoscaler 按 Serve 队列深度扩容；KEDA 等事件驱动方案见 [[wiki/15-AI基础设施/05-K8s-AI基础设施/14-model-serving-autoscaling-keda.md|推理服务自动伸缩]]。

### 4.3 与 Kueue 集成（配额与排队）

Kueue（v0.20.1，RayJob/RayCluster 集成均已稳定）把 Ray 集群纳入配额治理：

```yaml
# RayJob 上只需两个动作：
metadata:
  labels:
    kueue.x-k8s.io/queue-name: llm-training-lq   # 指向 LocalQueue
spec:
  suspend: true    # Kueue 准入前不启动；准入后 Kueue 自动置 false
```

- Kueue 按 RayJob 内嵌 `rayClusterSpec` 的 head+worker **峰值资源**（取 maxReplicas 口径，可配置）计算配额占用，准入后才放行创建集群。
- 常驻 RayCluster 模式下，Kueue 通过"准入检查 + 副本门控"约束 `maxReplicas` 扩容上限不越过 ClusterQueue 配额。
- 抢占/借用语义与普通 Job 相同，详见 [[wiki/15-AI基础设施/05-K8s-AI基础设施/22-kueue-vs-volcano-batch-scheduling.md|Kueue vs Volcano 批调度对比]]。

### 4.4 与 Volcano gang 调度集成

KubeRay 本身不实现 gang，但对"要么全起、要么不起"的硬性训练拓扑，可将 head/worker 全部交给 Volcano：

```yaml
# 所有 group 的 template.spec 统一：
spec:
  schedulerName: volcano
  # Volcano 通过 PodGroup 聚合同名 group 的 Pod
```

配合 Volcano Queue（`volcano.sh/queue-name` 标签）与 PodGroup `minMember` 实现"凑不齐 N 张卡就整队等待"，避免分布式训练占一半卡死等另一半。集成细节见 [[wiki/15-AI基础设施/05-K8s-AI基础设施/07-training-operators-volcano-mpi.md|Volcano 训练调度]]。

---

## 五、故障排查与诊断流程

### 5.1 head 挂掉的影响域

head Pod 承载 GCS（全局状态服务）+ Dashboard + Autoscaler + Raylet，是**单点**。head 重启的后果分层：

| 组件 | 影响 | 恢复 |
|------|------|------|
| GCS 元数据 | 未开 FT：所有 actor/task 状态丢失、作业失败；开 `ray.io/ft`：task 可重放恢复（依赖幂等与 max_restarts） | KubeRay 重建 head Pod，worker 重新注册 |
| Autoscaler | 短暂停止弹性，不影响已运行 Pod | head 就绪后自愈 |
| Serve 流量 | head 重启期间 Serve 副本（在 worker 上）可能仍在，但路由/控制面不可用 | 控制面恢复后自动重组 |
| RayJob | 提交通道 8265 中断；作业进程在 worker 上的可继续跑 | submitter 重试提交幂等作业 |

排查顺序：

```bash
# 🟢 head 为何重启
kubectl -n ray get pod <head> -o jsonpath='{.status.containerStatuses[0].lastState}' | jq
kubectl -n ray describe pod <head> | sed -n '/Events/,$p'

# 🟢 GCS 是否健康（head 内）
kubectl -n ray exec <head> -- curl -s localhost:8265/api/version
kubectl -n ray logs <head> -- raylet | grep -i gcs   # 🟢
```

### 5.2 worker OOM 与驱逐

- Ray 对象存储（`/dev/shm` + 内存）超限触发 `ObjectStoreFullError`；系统 OOM 则被 kubelet 驱逐（`Evicted`）。
- GPU 显存 OOM 表现为进程退出码 1 + `CUDA out of memory`，Pod 不会重启——Ray 层面该节点资源记账残留，任务悬挂。
- 处理：给 worker 配 `emptyDir medium:Memory` 足量 shm、`RAY_object_spilling_config` 落盘溢写、`max_restarts` + 检查点。

```bash
# 🟢 定位哪个 worker OOM
kubectl -n ray get pods -o wide | grep -E 'OOM|Evicted'
kubectl -n ray logs <worker> -- previous | grep -i -m1 'out of memory'   # 🟢
# 🟡 手动删除记账残留的僵死 worker（KubeRay 会按期望副本补齐）
kubectl -n ray delete pod <bad-worker>
```

### 5.3 GCS 状态与容错要点

- Ray 2.x 的外部存储（Redis GCS）方案已被社区移除，GCS 状态随 head Pod 生命周期——**head Pod 的内存就是集群状态的持久层**，因此生产上 head 要 requests=limits（Guaranteed QoS）、不打可抢占容忍、不开 Karpenter disruption。
- `ray.io/ft: "true"` 开启 GCS FT 后：task 断点可恢复、worker 断连不致命，但 actor 恢复仍依赖应用声明 `max_restarts` 且构造幂等。

### 5.4 常见事件速查

| 事件/现象 | 含义 | 动作 |
|-----------|------|------|
| `HeadPodRunningAndReady` | head 已就绪 | — |
| `UpdatedResource` | Autoscaler/Operator 调整了 worker 副本 | 观察 5.1 决策日志 |
| worker Pending 且事件 `Insufficient nvidia.com/gpu` | 节点层缺货 | 查 Karpenter 供货/配额 |
| RayJob 卡 `PENDING` | Kueue 未准入或 Dashboard 未就绪 | `kubectl get rayjob -o yaml` 看 `jobDeploymentStatus` |
| RayService 反复重建集群 | 健康探测超时 | 查 Serve 应用日志，调大 UnhealthySecondThreshold |

```bash
# 🟢 一站式状态采集
kubectl get raycluster,rayjob,rayservice -A
kubectl -n ray describe raycluster llm-train-pool
kubectl -n ray exec deploy/... -- ray status   # 集群内资源视角（在 head 执行）
```

---

## 六、最佳实践与反模式

### ✅ 最佳实践

1. **head 最小化**：head 不跑计算（`num-cpus: '0'`），资源只够控制面，避免与业务互杀。
2. **训练用 RayJob + `shutdownAfterJobFinishes`**：空闲 GPU 是平台最大的浪费源。
3. **共享池 + Kueue 配额**：常驻 RayCluster 按团队 LocalQueue 切分；临时需求走 RayJob 独立集群。
4. **镜像分层**：基础 CUDA 镜像 + `runtimeEnvYAML` 装 pip 依赖，避免 30GB 巨镜像。
5. ** shm 必配**：所有 GPU worker 挂 Memory 型 emptyDir 到 `/dev/shm`。
6. **拓扑一致**：tensor-parallel 组的 worker 用节点亲和钉在同机型/同交换机，跨 NVLink 域会显著拉慢 NCCL。

### ❌ 反模式

1. **对 Ray worker 用 HPA**——绕过 Ray 资源记账，秒级制造 actor 孤儿。
2. **head 挂海量 PVC/低优先级**——head 是状态单点，被抢占等于全集群重启。
3. **`maxReplicas: 1000` 而没有配额层**——Autoscaler 会诚实地把 Karpenter 打爆。
4. **用 `kubectl scale` 手改 worker 副本**——下一轮 reconcile/Autoscaler 决策会覆盖你的值。
5. **在 head 上跑 Ray Data 重管道**——OOM 的头号来源。

---

## 七、性能调优与监控指标

### 7.1 关键指标（Prometheus 抓取 Dashboard `/metrics`）

| 指标 | 含义 | 告警建议 |
|------|------|---------|
| `ray_cluster_active_nodes` / `ray_cluster_pending_nodes` | 节点供需 | pending 持续 > 5min → 节点层瓶颈 |
| `ray_scheduler_tasks`（queued） | 待调度任务堆积 | 持续增长且节点已到 max → 调上限 |
| `ray_memory_used_bytes` / `ray_memory_monitor` | 对象存储压力 | > 90% → 开 spilling |
| `ray_node_failure` / actor 死亡计数 | 稳定性 | 突增 → 查 5.1/5.2 |
| Serve：`ray_serve_num_requests`、P99 延迟 | 推理质量 | 见 [[wiki/15-AI基础设施/05-K8s-AI基础设施/13-ai-observability-arize-phoenix|AI 可观测性]] |

### 7.2 调优抓手

- **扩容延迟**：`upscalingMode: Aggressive` + Karpenter 节点启动探针预热；训练场景冷启动占尾延迟大头。
- **缩容抖动**：调大 `idleTimeoutSeconds` 与 `minSuccessSeconds`，防止"刚回收又扩回"。
- **GPU 利用率**：结合 DCGM 判断 Ray 逻辑记账与真实利用率差（详见 [[wiki/15-AI基础设施/01-基础设施/04-gpu-monitoring-dcgm.md|GPU 监控 DCGM]]），必要时用 MIG 共享（见 [[wiki/15-AI基础设施/01-基础设施/42-gpu-sharing-partitioning-guide.md|GPU 共享与切分指南]]）。

---

## 八、版本兼容性、升级与生产案例

### 8.1 版本矩阵与升级注意

| 组件 | 版本（2026-10 GitHub latest） | 要点 |
|------|------------------------------|------|
| KubeRay Operator | v1.7.1 | v1.x 内原地升级 CRD：`kubectl apply -f kuberay/release/crd-upgrade.yaml`；v0.x→v1.x 需迁移废弃字段 |
| Ray | 2.59.0 | 镜像 tag 与 `rayVersion` 保持一致；Ray ≥ 2.44 默认 v2 Autoscaler |
| Kubernetes | 1.28–1.37 | 无强耦合；DRA 特性对 Ray 无直接影响，但可改善底层 GPU 供给 |

升级顺序：先升 Operator（向后兼容旧 CR）→ 灰度新 Ray 镜像的 worker 组 → 最后升 head（head 版本必须 ≥ worker）。**跨大版本 Ray 升级不要原地做**，RayService 走双集群切换，RayJob 天然按作业粒度滚动。

### 8.2 生产案例：LLM 训练平台弹性算力池

某 200+ H100 训练平台的拓扑：

- **常驻层**：2 个 RayCluster（训练池/数据处理池），Karpenter NodePool 按 `gpu.product` 供货，节点池启用容量预留保底。
- **配额层**：Kueue ClusterQueue 按团队 cohort 借用；RayJob（临时训练）与常驻池（数据预处理 Ray Data）共享 cohort，夜间预处理任务借走训练配额跑大吞吐 ETL，白天训练优先级抢占回收。
- **弹性语义**：训练 worker 组 `minReplicas=0`，工作流引擎按 DAG 阶段（tokenize→train→eval）提交 RayJob，`shutdownAfterJobFinishes` 保证每小时级回收。
- **收益**：GPU 周均利用率从 41% 提升到 73%；Karpenter 整机缩容 + 任务级回收把空闲 GPU 费用压掉约 1/3。成本分摊口径见 [[wiki/15-AI基础设施/05-K8s-AI基础设施/15-gpu-cost-attribution-multitenant.md|GPU 成本分摊与多租户]]。

---

## 九、参考链接与延伸阅读

- KubeRay 官方文档：https://docs.ray.io/en/latest/cluster/kubernetes/index.html
- KubeRay GitHub（版本核对源）：https://github.com/ray-project/kuberay
- Ray GitHub Releases：https://github.com/ray-project/ray/releases
- Kueue RayJob/RayCluster 支持矩阵：https://kueue.sigs.k8s.io/docs/tasks/run_rayjobs/
- Ray Serve LLM（vLLM on Ray）：https://docs.ray.io/en/latest/serve/llm/overview.html

### Related

- [[wiki/15-AI基础设施/05-K8s-AI基础设施/06-kuberay-distributed-computing|KubeRay 分布式计算入门]]
- [[wiki/15-AI基础设施/05-K8s-AI基础设施/08-batch-scheduling-kueue-yunikorn.md|Kueue 与 YuniKorn 批量调度]]
- [[wiki/15-AI基础设施/05-K8s-AI基础设施/22-kueue-vs-volcano-batch-scheduling.md|Kueue vs Volcano 批调度深度对比]]
- [[wiki/15-AI基础设施/01-基础设施/05-distributed-training-frameworks.md|分布式训练框架]]
- [[wiki/15-AI基础设施/05-K8s-AI基础设施/11-finetuning-peft-lora-deepspeed|微调基础设施]]
