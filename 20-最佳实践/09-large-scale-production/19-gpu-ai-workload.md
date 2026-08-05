---
title: GPU/AI 负载与批量调度最佳实践
description: 大规模 Kubernetes 集群 GPU/AI 负载的生产实践：DRA 动态资源分配、Kueue 批量队列、GPU 共享与拓扑感知、镜像预热、训练容错与 GPU 利用率治理
summary: 覆盖 DRA（1.34 GA）与 device plugin 选型、Kueue 队列与公平共享、GPU 共享（MIG/时分）、拓扑感知调度、大镜像与数据加速、训练任务容错
category: references
tags:
- k8s
- gpu
- ai
- kueue
- dra
- scheduling
- production
tier: core
created: '2026-08-03'
last_updated: '2026-08-03'
difficulty: expert
audience:
- 平台工程师
- AI 基础设施工程师
- SRE
estimated_read_time: 25min
---

# GPU/AI 负载与批量调度最佳实践

> GPU 是大规模集群中最贵、最稀缺、利用率最容易浪费的资源。核心命题：**让正确的任务在正确的时间拿到正确的卡，且队列公平可治理**。

## 1. GPU 资源分配：DRA vs Device Plugin

| 维度 | Device Plugin（传统） | DRA（Dynamic Resource Allocation） |
|---|---|---|
| 分配方式 | 按数量申请（`nvidia.com/gpu: 2`），卡无差别 | 按属性申请（型号/显存/互联拓扑/MIG 规格），类 PVC 的 Claim 模型 |
| GPU 共享 | 需厂商插件旁路（时分/MIG 静态配置） | 原生表达共享与细粒度分配（实际切分仍由 MIG/驱动完成） |
| 拓扑感知 | 调度器盲（PCIe/NVLink/NUMA 不可见） | 属性化表达互联拓扑，支撑拓扑感知调度 |
| 成熟度 | 生产验证多年 | **核心 API 于 K8s 1.34（2025-09）GA，1.35 默认锁定开启**；但驱动与托管服务生态成熟度仍在收敛中 |

**落地建议（2026 年视角）：**

- 新建 GPU 集群（K8s ≥ 1.34）：优先评估 DRA，平台团队封装好 `DeviceClass`，业务无需写 CEL 表达式
- 存量生产集群：device plugin 体系不动，在隔离分区试点 DRA，待所用硬件的 DRA 驱动成熟后再迁移——**不要把新营收集群押在尚未收敛的路径上**
- 版本事实以目标集群版本官方文档为准复核

## 2. 批量调度与队列治理：Kueue

原生调度器决定"Pod 放哪"，但不管"任务什么时候允许开始"——多团队共享 GPU 池时会出现：大队列头阻塞、低优实验挤占生产训练、无配额公平。

**Kueue（CNCF，kubernetes-sigs）** 是 K8s 原生作业排队层，不替换调度器，通过 `spec.suspend` 门控任务准入：

| 概念 | 作用 |
|---|---|
| ClusterQueue | 集群级容量池与配额（按 ResourceFlavor 区分机型/卡型） |
| LocalQueue | 命名空间级提交入口，绑定 ClusterQueue |
| Cohort | ClusterQueue 分组，支持组内配额借用 |
| 策略 | StrictFIFO/BestEffortFIFO 排队、公平共享（Fair Sharing）、优先级抢占 |

生产要点：

- **GPU 池必须走 Kueue 队列准入，禁止业务裸提 Job/PyTorchJob**——否则排队、配额、抢占全部失效
- 集成面：BatchJob、Kubeflow 训练任务、RayJob/RayCluster、JobSet、普通 Pod 均有内置支持
- Gang 语义：分布式训练用 all-or-nothing（整组就绪才放行），避免部分 Pod 占卡空转
- 拓扑感知调度（Topology-Aware Scheduling）：多机训练按网络拓扑就近排布，降低通信开销
- MultiKueue：主集群容量不足时把作业分发到其他集群执行——多云/多集群 GPU 池的标准姿势
- 监控：内置 Prometheus 指标（排队时长、准入率、配额水位）必须接入告警

**替代方案对比**：Volcano（自带调度器，gang 调度成熟，华为系生态）、KAI Scheduler（NVIDIA 开源的 Run:ai 引擎，2025-04 开源，CNCF Sandbox，gang + GPU 共享 + 层级配额）。Kueue 优势是与原生调度器共存、侵入最小；已有调度器定制需求的团队可评估 Volcano/KAI。

## 3. GPU 共享与利用率治理

行业经验值：未治理的 GPU 集群利用率常见仅 25–35%，队列化 + 共享化后可提升到 60–85%（社区报告口径，具体因负载画像而异）。

| 手段 | 适用 | 注意 |
|---|---|---|
| MIG（A100/H100 硬件切分） | 推理/小任务混布 | 切分规格静态，重配需排空节点 |
| 时分复用（time-slicing） | 开发/测试环境 | 无显存隔离，生产慎用 |
| 队列化 + 抢占 | 所有场景 | 低优任务让路高优，回收闲置 |
| 弹性训练（容错 checkpoint） | 长训练任务 | 配合抢占策略，被抢后自动续训 |
| 推理侧共享 | vLLM/KServe 单卡多模型 | 按显存画像装箱 |

治理闭环：**DCGM 指标采集 → 按团队/任务维度利用率报表 → 低利用任务画像分析 → 配额与共享策略调整**（月度，与 [[14-cost-finops]] 节奏合并）。

## 4. 大镜像与数据加速

GPU 任务镜像常 10–30 GB（CUDA + 框架 + 模型），冷启动是弹性扩缩的最大障碍：

1. **镜像预热**：DaemonSet/预热控制器提前把热点镜像拉到 GPU 节点池
2. **懒加载**：Nydus/stargz 按需加载，冷启动从分钟级降到秒级
3. **P2P 分发**：Dragonfly 集群内分发，避免打爆镜像仓库（数千节点同时拉同一镜像的必备）
4. **模型与镜像解耦**：模型走对象存储/共享文件系统挂载或模型加载器 init container，镜像保持精简
5. **数据加速**：训练数据走 JuiceFS/Fluid + 本地缓存层，避免每 epoch 重复拉远端

## 5. 训练任务容错

- **checkpoint 制度化**：长训练必须周期性 checkpoint 到持久存储，频率按"可接受丢失时长"定
- 断点续训：任务重启自动从最近 checkpoint 恢复（框架级：PyTorch elastic / Ray Train）
- 节点故障自愈：GPU 节点异常（XID 错误、ECC 错误、NVLink 故障）→ 检测组件（node-problem-detector + DCGM）打 taint → 任务自动迁移续训
- 监控 GPU 健康指标：XID 错误率、显存 ECC、温度、功耗、NVLink 带宽，纳入告警

## 6. GPU 节点池管理

- GPU 节点独立节点池 + taint（`nvidia.com/gpu=present:NoSchedule`），仅 GPU 负载可调度
- NVIDIA GPU Operator 统一管理驱动/container runtime/device plugin/DCGM——禁止手工逐台装驱动
- 驱动与 CUDA 版本矩阵纳入变更管理，升级走 [[13-upgrade-certificate-runbook]] 的分批轮换
- GPU 节点不参与通用业务混布；CPU 密集型预处理任务调度到 CPU 池（数据管道与计算分离）

## 7. 常见反模式

| 反模式 | 后果 |
|---|---|
| 训练任务裸提裸跑不过队列 | 队头阻塞、配额失控、无法公平共享 |
| 无 gang 语义的分布式训练 | 部分 Pod 占卡等待，集群级死锁浪费 |
| 无 checkpoint 的长训练 | 一次节点故障丢掉数天算力 |
| 模型打进镜像 | 镜像 30GB+，扩缩容完全失效 |
| 开发任务与生产训练无优先级区分 | 生产任务被实验挤占，事故定性 |
| GPU 节点混布通用业务 | 高价值节点被低价值负载占住 CPU/内存 |

## Related

- [[11-autoscaling-capacity|弹性伸缩与容量规划深化]]
- [[03-workload|工作负载最佳实践（批处理负载）]]
- [[14-cost-finops|成本治理 FinOps（GPU 成本）]]
- [[15-AI基础设施/README|AI 基础设施域]]
