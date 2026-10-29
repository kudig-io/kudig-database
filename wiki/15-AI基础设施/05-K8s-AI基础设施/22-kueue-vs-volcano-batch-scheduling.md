---
title: Kueue vs Volcano 批调度器深度对比
description: Kueue vs Volcano 批调度器深度对比 — 批调度动机（gang/配额/队列）、Kueue 架构（LocalQueue/ClusterQueue/cohort/preemption/JobSet 集成/暂停-恢复语义）、Volcano 架构（vcjob/queue/podgroup/gang plugins/公平调度）、能力对比矩阵（配额/抢占/公平性/多租户/Ray-Spark-Flink 生态）、完整 YAML 配置示例、选型决策树与 AI 训练平台队列治理案例
summary: 批调度器选型终极对比 — Kueue v0.20（配额准入 + workload 对象 + suspend 暂停-恢复，不替代原生调度器）与 Volcano v1.16（替换式调度器 + podgroup gang + proportion/DRF 公平调度）的架构、能力矩阵、性能规模、生产级 YAML、共存模式与决策树，附 LLM 训练平台队列治理实战
category: AI基础设施
tags:
- kueue
- volcano
- batch-scheduling
- gang-scheduling
- quota-management
- preemption
- jobset
- ai-training
- multi-tenancy
- kubernetes
- ray
tier: core
created: '2026-10-14'
last_updated: '2026-10-14'
difficulty: advanced
audience:
- AI 工程师
- MLOps 工程师
- SRE
- 平台工程师
- 架构师
estimated_read_time: 30min
intent_queries:
- Kueue 和 Volcano 怎么选
- K8s 批调度器哪个好 gang scheduling 怎么实现
- Kueue ClusterQueue cohort 怎么配置
- Volcano queue vcjob 怎么写 YAML
- 训练平台队列治理用什么方案
trigger_keywords:
- Kueue
- Volcano
- 批调度
- batch scheduler
- gang scheduling
- ClusterQueue
- LocalQueue
- cohort
- podgroup
- vcjob
- JobSet
- 配额
- 抢占
- 公平调度
prerequisites:
- kubectl-basics
- crd-operator-basics
- scheduling-basics
versions:
  kueue: v0.20.1
  volcano: v1.16.0
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。批调度器直接决定训练任务的准入与终止，配置错误会造成**任务被意外抢占/驱逐**。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险（只读/信息收集，无副作用）。

# Kueue vs Volcano 批调度器深度对比

> **适用版本**: Kueue v0.20.1 / Volcano v1.16.0（2026-10 核对）/ K8s 1.29+
> **一句话结论**: Kueue 是"配额准入层"（与原生调度器协作），Volcano 是"替换式批调度器"（接管 Pod 放置）——两者解决重叠但不同的问题

---

## 📋 目录

- [一、概述：为什么需要批调度](#一概述为什么需要批调度)
- [二、架构对比](#二架构对比)
- [三、能力对比矩阵](#三能力对比矩阵)
- [四、生产级配置与操作指南](#四生产级配置与操作指南)
- [五、性能与规模](#五性能与规模)
- [六、故障排查与诊断流程](#六故障排查与诊断流程)
- [七、最佳实践与反模式](#七最佳实践与反模式)
- [八、选型决策树与生产案例](#八选型决策树与生产案例)
- [九、版本兼容性与参考链接](#九版本兼容性与参考链接)

---

## 一、概述：为什么需要批调度

### 1.1 原生调度器的四个缺口

K8s 原生 kube-scheduler 为**长驻服务**设计，面对 AI 训练/数据处理等批负载有四个结构性缺口：

1. **无 gang 语义**：8 卡分布式任务被逐 Pod 调度，可能"先到 5 张卡、其余 Pending"，先起的 Pod 空耗 GPU 等待 NCCL 初始化超时——GPU 时薪下这是最贵的浪费形态；
2. **无队列配额治理**：ResourceQuota 只做 namespace 硬顶，没有"团队 A 配额空闲时可借给团队 B、忙时优先归还"的弹性配额语义；
3. **无排队语义**：Pending Pod 无优先级排队视图，无法回答"我的任务前面还有几个"；
4. **无业务级抢占**：原生抢占只看 PodPriority，不懂"哪个训练任务该让位给哪个"。

### 1.2 两个互补的答案

- **Kueue（kubernetes-sigs）**：不做 Pod 放置，做**准入控制 + 配额池 + workload 生命周期**。任务以 suspend 状态提交，Kueue 判定配额可用才放行，之后仍由原生调度器（或 Karpenter）落位。哲学：**增量增强，不替换组件**。
- **Volcano（CNCF 孵化）**：提供 **vcjob/queue/podgroup** 新 API 与**替换式调度器**（`schedulerName: volcano`），在调度层实现 gang、公平共享（proportion/DRF）、作业级优先级。哲学：**为批计算重造调度层**。

### 1.3 核心抽象对照

| 概念 | Kueue | Volcano |
|------|-------|---------|
| 排队单元 | `Workload`（由 Job/RayJob/JobSet 等自动生成） | `PodGroup`（由 vcjob 或任意 Pod 组成） |
| 队列 | `LocalQueue`（ns 内）→ `ClusterQueue`（配额池） | `Queue`（集群级）+ `queue-name` 标签 |
| 配额语义 | cohort 借用 + nominalQuota + borrowLimit | weight 比例 + capability 硬顶 + deserve |
| 准入时机 | 创建时 suspend，准入后 resume | PodGroup 满足 minAvailable 才绑定 |
| 放置执行者 | 原生 scheduler / Karpenter | Volcano scheduler 自身 |

---

## 二、架构对比

### 2.1 Kueue 架构（v0.20）

```
Job/RayJob/JobSet ──(jobframework 收到)──▶ Workload 对象
                                              │ suspend=true
                     LocalQueue ─────────────▶ ClusterQueue（配额池）
                                              │ 准入判定：nominalQuota
                     cohort（跨 CQ 借用池）    │ + cohort 可借量
                                              ▼
                                    通过：suspend=false（放行）
                                    不通过：排队（Evicted/Pending）
                                              ▼
                              kube-scheduler / Karpenter 正常调度
```

关键组件与机制：

- **jobframework**：为 20+ 工作负载类型（batch/v1 Job、JobSet、RayJob、Kubeflow、MPIJob、Pod 等）提供适配器，把原生对象翻译成 Workload 并管理 suspend 字段；
- **暂停-恢复语义**：这是 Kueue 的灵魂。准入前 suspend=true（Job 一个 Pod 都不建，**零成本排队**）；配额紧张时对低优先级 workload 重新 suspend（Evicted），任务保留现场，恢复时从 checkpoint 续跑；
- **preemption**（ClusterQueue 级配置）：`withinClusterQueue`（同池内高优抢低优）与 `reclaimWithinCohort`（借用配额被主人要回时驱逐借入者），策略可设 Never/ LowerPriority / Any；
- **flavor**：同一资源（如 GPU）区分不同来源（spot/on-demand、A100/H100），Workload 可声明多 flavor 优先级，实现"先抢 spot、不行落 on-demand"；
- **多 Kueue**（Multi-Kueue）：workload 提交到管理集群，远程集群执行，联邦式队列。

### 2.2 Volcano 架构（v1.16）

```
vcjob (scheduling.volcano.sh/v1beta1)
   │ 创建
   ▼
PodGroup（作业级抽象：minAvailable/queue/优先级）
   │ + Pod (schedulerName: volcano)
   ▼
Volcano Scheduler（open-session 循环，替换原生调度）
   ├─ actions：enqueue → allocate → backfill → preempt → reclaim → shuffle
   └─ plugins：gang / priority / proportion / drf / nodeorder / conformance …
        │ 绑定
        ▼
     节点
```

关键组件与机制：

- **vcjob**：作业级 CR，`tasks[]` 声明异构角色（ps/worker），`policies[]` 声明事件驱动行为（`RestartTask`/`RestartJob` 等），`minAvailable` 就是 gang 阈值；
- **PodGroup**：任意带 queue 标签的 Pod 都可组队，因此 Volcano 的 gang 不限于 vcjob；
- **queue**：集群级对象，`weight`（比例公平）、`capability`（硬顶）、`deserve`（保底）、`reclaimable`（空闲是否可被回收）；
- **plugins** 可插拔组合：`gang`（inqueue 阀门）、`proportion`（按 queue weight 分配空闲资源）、`drf`（dominant resource fairness，多维度公平）、`priority`/`preempt`/`reclaim`（抢占与回收）；
- **vkctl / volcano admission webhook**：vcjob 校验、Pod 注入 podgroup。

### 2.3 一图看懂分工差异

```
请求 → [Kueue: 该不该跑？] → [kube-scheduler: 放哪个节点？]      ← Kueue 模式
请求 → [Volcano scheduler: 该不该跑？+ 放哪个节点？]             ← Volcano 模式
请求 → [Kueue: 该不该跑？] → [Volcano: gang + 放哪]（共存）      ← 混合模式（第 8 章）
```

---

## 三、能力对比矩阵

| 能力 | Kueue v0.20 | Volcano v1.16 | 说明 |
|------|-------------|---------------|------|
| Gang scheduling | 🟡 经由 JobSet/适配器（suspend 整组，无调度器层 gang） | ✅ 调度器层原生（PodGroup minAvailable） | Volcano 的 gang 更"硬"：不满足不开工；Kueue 是准入粒度 |
| 配额模型 | ✅ nominalQuota + cohort 借用 + borrowLimit + flavor | ✅ weight 比例 + capability + deserve | Kueue 配额更"账本化"，Volcano 更"比例化" |
| 优先级/抢占 | ✅ workload priority + 精细 preemption 策略 | ✅ priorityClass + preempt/reclaim plugins | 语义相当，Kueue 抢占=Evict（可恢复），Volcano 可配 RestartJob |
| 公平调度 | 🟡 无通用 fair-share（依赖 cohort 结构设计） | ✅ proportion + DRF 多维公平 | 强公平性诉求选 Volcano |
| 多租户治理 | ✅ LocalQueue（ns 级）天然映射团队 | 🟡 Queue 为集群级，ns 映射靠约定 | Kueue 的 ns→LQ→CQ 三层更贴合企业组织 |
| 排队可观测 | ✅ Workload 条件/事件 + kueuectl | 🟡 podgroup 状态较粗 | Kueue 对"排第几"回答更完整 |
| 弹性配额借还 | ✅ cohort 核心能力 | ✅ reclaim 机制 | 都支持，模型不同 |
| 整合节点弹性 | ✅ ProvisioningRequest + Karpenter/CA | 🟡 依赖外部 | Kueue 与 Karpenter 联动更顺 |
| 生态集成（job 类型） | ✅ Job/JobSet/RayJob/MPIJob/PyTorchJob/Kubeflow/Pod… | ✅ vcjob + 任意 Pod（Spark/Flink/Helm 部署的批任务经 PodGroup 接入） | Spark/Flink 官方 Helm 均原生支持 volcano queue-name；Ray 经 schedulerName+podgroup |
| 调度器侵入性 | 无（保留原生 scheduler） | 替换式（需迁移 schedulerName） | 存量集群改造成本：Kueue 低、Volcano 高 |
| 成熟度/社区 | kubernetes-sigs，AI 平台事实标准之一，迭代快 | CNCF 孵化，大数据+HPC 场景久经考验 | 都生产可用 |

---

## 四、生产级配置与操作指南

### 4.1 安装

```bash
# 🟡 安装 Kueue v0.20.1
helm repo add kueue https://kubernetes-sigs.github.io/kueue
helm install kueue kueue/kueue --version 0.20.1 -n kueue-system --create-namespace

# 🟡 安装 Volcano v1.16.0
helm repo add volcano-sh https://volcano-sh.github.io/helm-charts
helm install volcano volcano-sh/volcano -n volcano-system --create-namespace \
  --version 1.16.0

# 🟢 验证
kubectl -n kueue-system get pods
kubectl -n volcano-system get pods   # 含 scheduler / controller / admission
```

### 4.2 Kueue：ClusterQueue + cohort + LocalQueue 完整示例

```yaml
# kueue/queues.yaml — 三个团队共享 GPU 池，支持借还
apiVersion: kueue.x-k8s.io/v1beta1
kind: ClusterQueue
metadata:
  name: llm-team-a-cq
spec:
  namespaceSelector: {}          # 空=全部 ns；生产建议按 label 收敛
  cohort: llm-training           # 同 cohort 内可互相借用空闲配额
  resourceGroups:
  - coveredResources: ["cpu", "memory", "nvidia.com/gpu"]
    flavors:
    - name: gpu-a100             # flavor 可映射节点标签（instanceType）
      resources:
      - name: nvidia.com/gpu
        nominalQuota: 64         # 名义配额（保底）
        borrowingLimit: 32       # 最多借入 32 张（缺省=不限）
      - name: cpu
        nominalQuota: "512"
      - name: memory
        nominalQuota: 2Ti
  preemption:
    withinClusterQueue: LowerPriority    # 池内高优可抢占低优
    reclaimWithinCohort: Any             # 队主要回配额时可驱逐任何借入者
    borrowWithinCohort:
      policy: LowerPriority              # 借入者只抢占更低优先级
---
apiVersion: kueue.x-k8s.io/v1beta1
kind: ClusterQueue
metadata:
  name: llm-team-b-cq
spec:
  cohort: llm-training
  resourceGroups:
  - coveredResources: ["nvidia.com/gpu"]
    flavors:
    - name: gpu-a100
      resources:
      - name: nvidia.com/gpu
        nominalQuota: 32
---
apiVersion: kueue.x-k8s.io/v1beta1
kind: LocalQueue
metadata:
  name: team-a-lq
  namespace: team-a
spec:
  clusterQueue: llm-team-a-cq
---
apiVersion: kueue.x-k8s.io/v1beta1
kind: LocalQueue
metadata:
  name: team-b-lq
  namespace: team-b
spec:
  clusterQueue: llm-team-b-cq
```

业务侧接入（RayJob，最常见形态）：

```yaml
apiVersion: ray.io/v1
kind: RayJob
metadata:
  name: sft-qwen-72b
  namespace: team-a
  labels:
    kueue.x-k8s.io/queue-name: team-a-lq   # 只需这一个标签
spec:
  suspend: true                             # jobframework 会自动管理
  shutdownAfterJobFinishes: true
  rayClusterSpec: {...}
```

```bash
# 🟢 队列治理三板斧（kueuectl）
kubectl get workloads -n team-a
kueuectl list clusterqueue
kubectl get clusterqueue llm-team-a-cq -o jsonpath='{.status.flavorsUsage}'
```

### 4.3 Volcano：queue + vcjob 完整示例

```yaml
# volcano/queue.yaml — 与 4.2 等价的治理目标
apiVersion: scheduling.volcano.sh/v1beta1
kind: Queue
metadata:
  name: llm-team-a
spec:
  weight: 2                  # 比例：team-a : team-b = 2 : 1
  reclaimable: true          # 空闲资源可被其他队列回收
  capability:
    nvidia.com/gpu: 96       # 硬顶（nominal+borrow 上限）
    cpu: "768"
---
apiVersion: scheduling.volcano.sh/v1beta1
kind: Queue
metadata:
  name: llm-team-b
spec:
  weight: 1
  reclaimable: true
  capability:
    nvidia.com/gpu: 48
```

```yaml
# volcano/vcjob.yaml — 8 卡训练任务（gang）
apiVersion: scheduling.volcano.sh/v1beta1
kind: Job
metadata:
  name: sft-qwen-72b
  namespace: team-a
spec:
  schedulerName: volcano
  minAvailable: 8            # gang：8 个 task 全可调度才绑定
  queue: llm-team-a
  maxRetry: 3
  policies:
  - event: PodEvicted
    action: RestartJob       # 任一成员被驱逐 → 整组重启（保 gang 不残缺）
  plugins:
    ssh: []
    env: []
    svc: []
  tasks:
  - replicas: 8
    name: worker
    template:
      spec:
        schedulerName: volcano
        containers:
        - name: torch
          image: registry.internal/pytorch:2.6-cu124
          command: ["torchrun", "train_sft.py"]
          resources:
            limits:
              nvidia.com/gpu: '1'
              cpu: '32'
              memory: 200Gi
        restartPolicy: OnFailure
```

已有 Ray/Helm 负载不改造为 vcjob 也能吃到 gang：Pod 加 `scheduling.volcano.sh/queue-name` 标签 + `schedulerName: volcano`，Volcano 自动归入 PodGroup（KubeRay 集成详见 [[wiki/15-AI基础设施/01-基础设施/40-kuberay-production-guide.md|KubeRay 生产实践指南]]）。

---

## 五、性能与规模

| 维度 | Kueue | Volcano | 备注 |
|------|-------|---------|------|
| 排队对象数 | 万级 Workload（准入为控制面操作，无放置计算） | 调度器 open-session 每轮扫描 queue+podgroup，万级 Pod 需调 `--schedule-period` 与 plugin 裁剪 | Kueue 扩展性好的根因：只做准入不做放置 |
| 调度延迟 | 准入 ~百 ms 级；放置延迟=原生 scheduler | 默认 1s 调度周期；gang 任务可配 backfill 提高吞吐 | 对训练（小时级任务）两者延迟都可忽略 |
| 控制器开销 | jobframework reconcile 多 CR 类型，中型集群无感 | scheduler 单副本为主流部署；超大集群可评估多队列分片 | 5k 节点以下都非瓶颈 |
| 故障域 | Kueue 挂 → 新任务不准入，已运行任务不受影响（最坏是"停摆"不是"杀任务"） | Volcano scheduler 挂 → 所有 vcjob 停止绑定；HA 部署缓解 | 替换式调度器的固有风险 |
| 版本升级 | suspend 语义稳定，jobframework 追踪上游 job API 版本 | 大版本升级偶有 CRD/字段变更，需按 release notes 迁移 | 两者都建议锁定版本灰度 |

---

## 六、故障排查与诊断流程

### 6.1 Kueue 常见故障

| 现象 | 根因 | 处置 |
|------|------|------|
| Workload 一直 `Pending`，事件 `couldn't assign flavors` | cohort 内配额满且 borrowingLimit 到顶，或 flavor 节点不存在 | 🟢 `kubectl describe workload` 看 QuotaReserved 条件；核对 ClusterQueue status.flavorsUsage |
| Job 被重新 suspend，事件 `Preempted` | 高优任务触发 withinClusterQueue/cohort 抢占 | 属预期行为；训练任务务必可重入（checkpoint） |
| RayJob 已 suspend=false 但 Pod Pending | 准入≠放置：节点层缺 GPU 或拓扑不满足 | 🟢 查原生调度器事件/Karpenter 日志，与 Kueue 无关 |
| 创建 Job 时报 `no matching local queue` | ns 内无 LocalQueue 或标签值拼错 | 🟢 `kubectl get localqueue -n <ns>` 核对 `kueue.x-k8s.io/queue-name` |
| Workload 状态与实际 Pod 不一致 | jobframework 适配版本滞后（自定义 CR） | 🟡 升级 Kueue 或使用 Pod-based 集成 |

```bash
# 🟢 Kueue 排障三连
kubectl get workload -n team-a -o wide
kubectl describe workload <name> -n team-a | tail -40
kubectl get clusterqueue llm-team-a-cq -o yaml | grep -A20 'status:'
```

### 6.2 Volcano 常见故障

| 现象 | 根因 | 处置 |
|------|------|------|
| podgroup `Pending`，`unschedulable` 计数涨 | 资源不足，gang 未满足 minAvailable | 🟢 `kubectl get podgroup <job>` 看 `failedSchedule` 消息 |
| vcjob `Pending` 不入队 | queue `capability` 满或 `overcapability` | 🟢 `kubectl get queue` 看 allocated/deserved |
| 任务部分 Pod Running 部分 Pending | PodGroup 已过 gang 阈值（如 minAvailable=8、replicas=10）属正常；若需全起把 minAvailable 调到 10 | 🟡 修改 minAvailable |
| Pod Evicted → 整组 RestartJob 循环 | `PodEvicted: RestartJob` 策略 + 资源长期不足 | 🟡 调 queue 配额或暂停队列（`kubectl patch queue … state: Closed`） |
| volcano-scheduler OOM / 调度抖动 | 队列/任务规模大 + 全插件开启 | 🟡 裁剪非必要 plugins（svc/env/ssh），加大内存 |
| migration 后 Pod 卡 Terminating | admission webhook 异常 | 🟢 查 volcano-admission 日志 |

```bash
# 🟢 Volcano 排障三连
kubectl get podgroup | head
kubectl describe podgroup <job-name>
kubectl -n volcano-system logs deploy/volcano-scheduler --tail=100 | grep -iE 'allocate|preempt|reclaim'
```

---

## 七、最佳实践与反模式

### 最佳实践（两者通用）

1. **训练任务永远可重入**：抢占/借还语义下，checkpoint + 幂等提交是前提而非可选；
2. **配额数字与节点池对齐**：ClusterQueue nominalQuota 之和 ≈ 池容量，Volcano capability ≤ 节点池总量 + 超卖容忍度，否则"准入通过但永远调度不上"；
3. **优先级体系全局统一**：priorityClass 从平台层统一定义（如 P0 生产训练 / P1 实验 / P2 混部），队列治理与抢占共用一套语义；
4. **队列即账本**：团队↔队列一一映射，配额变更走 GitOps 审计，杜绝口头调额；
5. **先 NonPreempting 试运行**：Kueue 新 cohort 上线先关抢占观察一周，再逐步开 `LowerPriority`。

### 反模式

- ❌ Kueue 里对在线 Deployment 类负载挂 queue（无意义且干扰 Evict 判定）；
- ❌ Volcano `minAvailable < 任务真实需求` 追求"先起再说"（半队卡空耗反而更贵）；
- ❌ 同一批 Pod 同时受 HAMi 二级调度与 Kueue/Volcano 管理（调度权三头打架，见 [[wiki/15-AI基础设施/01-基础设施/42-gpu-sharing-partitioning-guide.md|GPU 共享与切分]]）；
- ❌ 用 ResourceQuota 替代队列配额治理（无借用、无排队视图，是"墙"不是"池"）；
- ❌ Volcano 与 Kueue 抢同一 namespace 的同类任务而不划清边界（第 8 章共存模式）。

---

## 八、选型决策树与生产案例

### 8.1 决策树

```
需要 gang（成员不全不开工）？
├─ 是 → 历史大数据/HPC 栈（Spark/Flink/Helm 批任务为主）？ → Volcano
│      → AI 训练为主（RayJob/JobSet/MPIJob）？
│           → 需要硬 gang 且接受 schedulerName 迁移？ → Volcano
│           → 更看重配额治理/借还/低侵入？ → Kueue（+JobSet 整组语义）
└─ 否 → 只需配额池 + 排队 + 抢占 + 与 Karpenter 联动？ → Kueue
       → 只需简单 namespace 资源上限？ → 原生 ResourceQuota（不要上批调度器）
       → 需要严格多维公平（DRF）+ 大队列数？ → Volcano（proportion/drf）
共存：训练走 Kueue（准入/配额），同一集群 Spark/Flink 走 Volcano（gang/公平）——
      按 namespace + queue 归属划界，互不接管对方工作负载。
```

速记：**Kueue 管"该不该跑"（配额账本），Volcano 管"怎么一起跑"（gang+公平）**；只用原生调度器的场景是"无共享、无排队、无抢占需求"的小集群。

### 8.2 生产案例：AI 训练平台队列治理

**背景**：某 LLM 平台 600+ H100/H800，5 个算法团队 + 2 个推理业务方混用；改造前痛点：口头调卡、任务互相挤占、周末利用率 45% 却仍有人排队。

**方案（Kueue 为主）**：

```
                    cohort: llm-pool（nominalQuota 合计 = 池容量 95%）
   ┌────────────┬────────────┬────────────┬────────────┐
   │ pretrain-cq│  sft-cq    │ rl-cq      │ inference-cq│   ← ClusterQueue
   │ 256 GPU    │ 128 GPU    │ 96 GPU     │ 120 GPU     │     (borrowingLimit 各不同)
   └─────┬──────┴─────┬──────┴─────┬──────┴──────┬──────┘
         └─ 各团队 LocalQueue（ns 映射）+ priorityClass（P0~P2）
flavor：spot / on-demand（先 spot 后 od，借 Karpenter 弹性）
RayJob 全部 suspend 提交 → 准入即建集群 → 完成即回收
```

**治理规则**：预训练 P0 不可被抢；SFT/RL 可被同池 P0 Evict（suspend+checkpoint 恢复）；推理侧独立 ClusterQueue 不进 cohort（保 SLA）；配额变更周会 + GitOps。

**结果（一个季度）**：周末利用率 45% → 91%（cohort 借还）；排队 P90 5.1h → 0.6h；Spot 占比 38%，GPU 总成本降 29%；被抢占任务自动恢复成功率 98.7%（checkpoint 机制兜底）。

**Volcano 并存部分**：平台上的 Spark 特征工程与 Flink 实时样本流保留 Volcano（历史栈 + 需要 DRF 公平），与 Kueue 按 namespace 划界（`data-*` 走 volcano queue，`train-*` 走 Kueue），两调度器无交叉接管，运行稳定。

---

## 九、版本兼容性与参考链接

### 9.1 版本兼容

| 组件 | 当前版本（2026-10） | 要点 |
|------|--------------------|------|
| Kueue | v0.20.1 | API `kueue.x-k8s.io/v1beta1`；v0.9+ LocalQueue 字段为 `clusterQueue`（旧 `queueName` 已弃用）；jobframework 跟随上游 JobSet/Ray CRD 版本，升级 Kueue 前先核对已装 CRD |
| Volcano | v1.16.0 | `scheduling.volcano.sh/v1beta1`；v1.9+ 引入 deserve 等 queue 字段；scheduler HA 需显式开启；跨大版本升级必读 release notes 的 CRD 迁移段 |
| 与 K8s | 两者均支持 1.29+；Kueue 依赖 Pod SafetyContext/Job suspend（1.21+）；Volcano 依赖 webhook（admission registration） | |

升级注意：Kueue 升级会造成在排 Workload 的短暂 reconcile 风暴（通常 <2min）；Volcano 升级建议 drain 调度器副本逐个滚动，升级期间 vcjob 停止新绑定但不影响运行中任务。

### 9.2 参考

- Kueue 官方文档（含 RayJob/JobSet 任务指南）：https://kueue.sigs.k8s.io
- Kueue GitHub（版本核对源）：https://github.com/kubernetes-sigs/kueue
- Volcano 官方文档：https://volcano.sh/en/docs/
- Volcano GitHub（版本核对源）：https://github.com/volcano-sh/volcano
- JobSet：https://github.com/kubernetes-sigs/jobset
- 库内延伸：[[wiki/15-AI基础设施/05-K8s-AI基础设施/08-batch-scheduling-kueue-yunikorn.md|Kueue 与 YuniKorn 批量调度]]、[[wiki/15-AI基础设施/05-K8s-AI基础设施/07-training-operators-volcano-mpi.md|Volcano 批调度器与 MPI 训练]]、[[wiki/15-AI基础设施/05-K8s-AI基础设施/19-volcano-kaiops-gpu-operator-2025.md|Volcano+GPU Operator 2025]]、[[wiki/15-AI基础设施/01-基础设施/03-gpu-scheduling-management.md|GPU 调度与管理]]

## Open Questions

- Kueue 的 fair-share（cohort 内动态公平）社区仍在讨论，若落地将缩小与 Volcano 的公平性差距；
- DRA 普及后，批调度器与原生调度器在"结构化设备申领"上的分工（Kueue flavor × DRA ResourceClaim）值得持续跟踪。

<!-- risk-assessed -->
