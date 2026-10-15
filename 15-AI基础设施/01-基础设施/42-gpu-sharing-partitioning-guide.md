---
title: GPU 共享与切分技术全解
description: Kubernetes GPU 共享与切分技术全解 — NVIDIA MIG 硬件级切分（1g.10g 规格与 A100/H100/H200 支持矩阵、驱动与 fabric-manager 要求）、Time-Slicing 分时复用配置与无内存隔离的坑、vGPU/池化方案（license 生态与 HAMi 思路）、DRA（KEP-3061）动态资源分配进展；隔离性/计量计费/调度语义对比矩阵与选型决策表
summary: 四种 GPU 共享方案生产级对比 — MIG（硬件隔离、7 实例上限、算力带宽 QoS）、Time-Slicing（device plugin 分时、零隔离成本最低）、vGPU/HAMi 池化（显存切分与统一池化）、DRA（K8s 原生设备分配的未来）；含完整 YAML、与 Kueue/拓扑调度集成、MIG 配置失败与推理长尾排查、推理共享 vs 训练独占选型决策
category: ai-infra
tags:
- ai
- gpu
- kubernetes
- mig
- time-slicing
- vgpu
- hami
- dra
- device-plugin
- nvidia
- inference
- quota-management
- kueue
- finops
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
- Kubernetes 里一张 GPU 怎么共享给多个 Pod
- MIG 和 time-slicing 和 vGPU 怎么选
- GPU time-slicing 有哪些坑
- K8s DRA 对 GPU 调度有什么影响
- MIG 切分失败怎么排查
trigger_keywords:
- MIG
- Multi-Instance GPU
- time-slicing
- 分时复用
- vGPU
- HAMi
- GPU 池化
- DRA
- 动态资源分配
- GPU 共享
- 显存切分
prerequisites:
- kubectl-basics
- gpu-scheduling-basics
- device-plugin-basics
versions:
  nvidia_gpu_operator: v25.x
  kubernetes: '1.31-1.37'
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。GPU 切分类操作（MIG 重配置）会**重置 GPU 并终止其上所有进程**，属于最高风险变更。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险（只读/信息收集，无副作用）。

# GPU 共享与切分技术全解

> **适用版本**: NVIDIA GPU Operator v25.x / 驱动 R570+ / K8s 1.31-1.37（2026-10）
> **核心矛盾**: GPU 越来越大（80~192GB）、推理实例越来越小（7~32GB），独占整卡 = 60% 以上算力浪费

---

## 📋 目录

- [一、概述：GPU 利用率痛点](#一概述gpu-利用率痛点)
- [二、四种方案技术原理](#二四种方案技术原理)
- [三、隔离性/计量计费/调度语义对比矩阵](#三隔离性计量计费调度语义对比矩阵)
- [四、生产级配置与操作指南](#四生产级配置与操作指南)
- [五、调度器集成](#五调度器集成)
- [六、故障排查与诊断流程](#六故障排查与诊断流程)
- [七、最佳实践、性能与监控](#七最佳实践性能与监控)
- [八、选型决策表与生产案例](#八选型决策表与生产案例)
- [九、版本兼容性与参考链接](#九版本兼容性与参考链接)

---

## 一、概述：GPU 利用率痛点

### 1.1 三组典型数字

- **推理实例规格失配**：7B 量化模型 FP16 推理仅需 ~12~16GB 显存，独占 80GB A100 时显存利用率 <25%，SM 利用率常年在 10~30%；
- **开发测试碎片需求**：算法同学要"1/4 张卡跑实验"，原生 `nvidia.com/gpu` 的最小分配单位是 1 整卡；
- **预算现实**：H100 单卡时薪高昂，任何 SLA 允许的共享都是直接的 FinOps 收益（详见 [[15-AI基础设施/05-K8s-AI基础设施/15-gpu-cost-attribution-multitenant.md|GPU 成本分摊与多租户]]）。

### 1.2 共享的技术前提

GPU 是"计算 + 显存 + 带宽 + 引擎（NVDEC/NVENC/拷贝引擎）"的复合资源，共享方案的本质区别在于**沿哪个维度切**：

- 沿**硬件引擎与显存通道**切 → MIG（静态、强隔离）；
- 沿**时间**切 → Time-Slicing（内核级上下文轮转，软隔离）；
- 沿**显存容量 + 计算比例**切 → vGPU/HAMi 池化（拦截 API 层）；
- 不切设备、改**分配协议** → DRA（K8s 原生精细分配，仍在演进）。

---

## 二、四种方案技术原理

### 2.1 NVIDIA MIG（Multi-Instance GPU）

**原理**：Ampere 及以后的数据中心 GPU 硬件级分区，把一张物理 GPU 切成最多 7 个实例，每个 MIG 实例拥有**独立的 SM 集合、独立 L2 缓存切片、独立显存控制器与带宽、独立计算引擎**。实例之间硬件级故障与性能隔离——邻居实例打满不影响你的 QoS，这是唯一能写进 SLA 的隔离级别。

**MIG 规格命名法**：`<计算切片数>g.<显存 GB 数>`，以 80GB A100（8 个计算切片、7 个显存切片）为例：

| 规格实例 | 计算切片 | 显存 | 单卡最多实例 | 典型用途 |
|---------|---------|------|------------|---------|
| `7g.80gb` | 7/8 | 80GB | 1 | 等同整卡（开启 MIG 管理的单实例） |
| `4g.40gb` | 4/8 | 40GB | 2 | 30B 级推理 |
| `3g.40gb` | 3/8 | 40GB | 2（与 4g.40gb 共 7 切片：4+3） | 中型推理 + 训练日志处理 |
| `2g.20gb` | 2/8 | 20GB | 3 | 7~14B 推理 |
| `1g.10gb` | 1/8 | 10GB | 7 | 小模型/嵌入/小流量实验 |
| `1g.20gb` | 1/8 | 20GB | 3（1g.20×3 + 2g.20 + …） | 显存型小负载 |

**支持矩阵（关键约束）**：

| GPU | MIG 支持 | 实例上限 | 备注 |
|-----|---------|---------|------|
| A100 40/80GB (SXM/PCIe) | ✅ | 7 | 最成熟，社区文档最全 |
| H100 80GB (SXM5) | ✅ | 7 | 需 fabric-manager ≥ 驱动版本要求 |
| H100 PCIe / H200 | ✅ | 7 | H200 每实例显存等比放大（141GB 版单实例可达 ~132GB） |
| A30 | ✅ | 4 | 最大切 1g.6gb ×4 |
| A800/H800 | ✅ | 同 A100/H100 | 国内采购常见，配置路径一致 |
| L40S/L20/消费级 | ❌ | — | 无 MIG 引擎，只能走分时/池化 |

**驱动与系统要求**：驱动 R470+（H100 需 R515+，生产建议 R570 系列）；SXM 板卡需 **NVSwitch fabric-manager 与驱动严格同版本**，版本错位是 MIG 配置失败第一根因（见第六章）；数据中心 GPU 且 Guest OS 对 MIG 硬件直通有额外要求（VM 场景需 unlocked_mig）。

**两种策略**：

- `single`：整卡单一规格（如 7×1g.10gb），运维最简单，节点标签即规格；
- `mixed`：一卡多规格混合（GPU OperatormigManager 支持 per-node config），灵活但资源碎片化，调度表达式更复杂。

### 2.2 Time-Slicing（device plugin 分时复用）

**原理**：NVIDIA device plugin 的 `replicas` 机制——把同一张物理 GPU 以扩展资源 `nvidia.com/gpu` **重复注册 N 次**，N 个 Pod 可"各拿到 1 个 GPU"；实际执行时由 GPU 内核调度器按时间片轮转上下文。调度器以为有 N 张卡，物理上只有 1 张。

```yaml
# 核心配置（GPU Operator ConfigMap，完整版见第四章）
sharing:
  timeSlicing:
    resources:
    - name: nvidia.com/gpu
      replicas: 4     # 1 张卡被登记为 4 个可分配实例
```

**本质缺陷——无内存隔离**：

1. **显存总和可超卖**：4 个 Pod 各自认为有 80GB，实际共享 80GB，任一 Pod 显存膨胀即触发邻居 OOM（`CUDA error: out of memory` 随机落在"最倒霉的那个"上）；
2. **无 QoS**：时间片轮转粒度 ~1ms 级，邻居大 kernel 会让你的 P99 尾延迟成倍抖动——**推理服务 P99 敏感场景慎用**（排查见第六章）；
3. **无故障隔离**：一个 Pod 的 CUDA 非法访问可能拖垮整卡（XID 错误复位）；
4. **计量只有分配视图**：`nvidia.com/gpu` 记的是"分到几份"，不是真实用量，计费要靠 DCGM 指标二次归因。

**定位**：开发测试环境、内部工具、低峰批处理——"零成本凑合用"的最优解；对外计费或 SLA 场景不合格。

### 2.3 vGPU / 池化方案

**商业 vGPU（NVIDIA vGPU / BizLib 等）**：在驱动层虚拟化，显存与算力按比例（如各 50%）硬限制，支持显存超额与 QoS。代价：**按卡/按实例的 license 采购与 license server 运维**、版本与宿主驱动强绑定、生态相对封闭。适用：云服务商用 GPU 主机产品线、强合规多租户。

**开源统一池化（HAMi 思路）**：

- 在节点部署核心组件（如 HAMi 的 `k8s-device-plugin` + `vGPU scheduler` 调度器扩展）：业务 Pod 申报 `nvidia.com/gpu: 1` + 自定义注解表达"显存 12Gi、算力 30%"之类的**小数卡语义**；
- 调度器扩展在 **webhook/调度扩展点**拦截请求，按"显存余量 + 算力配额"二次分配，把 Pod 与具体卡绑定；
- 业务容器内通过**替换的 libcuda（API 拦截/重定向）**实现显存限额与算力限流，无需修改应用代码；
- 集中 scheduler 维护全集群"卡-显存-算力"三维台账，支持任务级别的超卖比例与优先级抢占。

**优点**：显存维度可切（MIG 做不到 10GB 以下任意粒度）、无硬件代际要求、可统一纳管 MIG/整卡/分时。**风险**：调度器扩展是**二级调度**，与原生调度器/其他调度器插件（Kueue 准入、DRA）存在职责重叠，升级 K8s 大版本时需验证兼容；libcuda 拦截层与 CUDA 新版本存在适配周期。GPU Operator 侧的 HAMi 模式部署见 [[15-AI基础设施/05-K8s-AI基础设施/01-gpu-operator-sharing-patterns.md|GPU Operator 部署与共享模式]]。

### 2.4 Kubernetes DRA（Dynamic Resource Allocation，KEP-3061）

**动机**：device plugin 机制（2017）的天花板——无法表达"我想要 2 张同 NVLink 域的 GPU + 各自 20% 显存"这类结构化需求；资源信息只有整数计数；设备生命周期与 Pod 强耦合。DRA 引入三个 API：`ResourceSlice`（节点申报设备能力清单）、`ResourceClaim`（工作负载的结构化设备请求）、`ResourceClaimTemplate`（按 Pod 模板化申领），由 DRA 驱动与调度器共同完成"按属性选设备"。

**与 GPU 的关系**：NVIDIA 已提供 DRA driver（GPU Operator 可选启用，alpha 阶段），支持 MIG 拓扑、NVLink 域、显存等属性的结构化申领；MIG + DRA 是官方给出的"细粒度 + 原生调度"组合拳方向。

**版本进展（截至 v1.37）**：

- v1.26：DRA alpha 引入（KEP-3061）；
- v1.32/v1.33：**结构化参数**（KEP-4381）进入 Beta，调度器可直接在 API 对象上做匹配，性能问题解决；
- v1.34/v1.35：DRA 核心 Beta 默认开启，逐步覆盖设备热升级等运维面；
- v1.37：延续增强（如 `DRADerivedAttributes`、`DRADeviceCompatibilityGroups`、`DRAOptionalNodePreparation` 等能力增强项，见 [[01-集群基础/06-升级路径/05-kubernetes-v1.37-upgrade-guide.md|Kubernetes v1.37 升级指南]]），生态仍在从"框架就绪"走向"GPU 生产可用"。

**结论**：DRA 是终局方向但**尚非生产默认选项**；生产选型仍以 MIG/分时/池化为主，新集群可开 DRA 并行试点。CDI 与设备注入的演进另见 [[15-AI基础设施/05-K8s-AI基础设施/17-cdi-device-plugin-framework.md|CDI 与 Device Plugin 框架]]。

---

## 三、隔离性/计量计费/调度语义对比矩阵

| 维度 | MIG | Time-Slicing | vGPU/商业 | HAMi 类池化 | DRA |
|------|-----|-------------|-----------|------------|-----|
| 计算隔离 | ✅ 硬件级（独立 SM） | ❌ 时间片轮转 | ✅ 配额级 | 🟡 限流级（拦截层） | 取决于底层设备 |
| 显存隔离 | ✅ 硬性边界 | ❌ 完全共享，可超卖 | ✅ 硬限制 | ✅ 限额（可超卖策略） | 按属性申领 |
| 故障隔离 | ✅ 实例间隔离 | ❌ 整卡连坐 | 🟡 部分隔离 | 🟡 依赖实现 | — |
| 性能 QoS | ✅ 带宽/缓存独占 | ❌ P99 抖动大 | 🟡 配额内稳定 | 🟡 限流下抖动小 | — |
| 最小粒度 | 1g.10gb（1/8 卡） | 1/N 卡（时间） | license 档位 | 显存 GB 级任意 | 属性表达式 |
| 动态性 | ❌ 切分需重置 GPU（分钟级窗口） | ✅ 无需变更 | 🟡 | ✅ 请求时分配 | ✅ 申领时分配 |
| 计量计费 | ✅ 实例级精确（DCGM per-MIG） | ❌ 只有份数 | ✅ license 计量 | 🟡 台账精确、用量靠指标 | 🟡 claim 精确 |
| 调度语义 | 整数扩展资源（`nvidia.com/mig-1g.10gb`） | 整数扩展资源（虚增） | 整数扩展资源 | 注解 + 二级调度 | ResourceClaim（结构化） |
| 与原生调度器耦合 | 完全原生 | 完全原生 | 完全原生 | 二级调度（冲突面） | 原生（新协议） |
| 成本 | 无额外成本 | 无 | license 费 | 无（人力成本） | 无 |
| 硬件要求 | Ampere+ 数据中心卡 | 任意 NVIDIA GPU | 支持 vGPU 的卡 | 任意 | 驱动/DRA driver 成熟度 |

---

## 四、生产级配置与操作指南

### 4.1 Time-Slicing（GPU Operator）

```yaml
# gpu-operator/time-slicing-config.yaml — 完整可拷贝
apiVersion: v1
kind: ConfigMap
metadata:
  name: time-slicing-config
  namespace: gpu-operator    # 必须与 GPU Operator 同命名空间
data:
  a100-80gb: |-
    version: v1
    sharing:
      timeSlicing:
        resources:
        - name: nvidia.com/gpu
          replicas: 4         # 80GB 卡切 4 份
  h100-mig-off: |-
    version: v1
    sharing:
      timeSlicing:
        resources:
        - name: nvidia.com/gpu
          replicas: 2
          exclude: ["nvidia.com/mig-1g.10gb"]   # 不与 MIG 资源叠加
```

```bash
# 🟡 让 device plugin 使用该 ConfigMap（首次安装或原地升级参数）
helm upgrade gpu-operator nvidia/gpu-operator -n gpu-operator \
  --set devicePlugin.config.name=time-slicing-config \
  --set devicePlugin.config.default=a100-80gb

# 🟡 按节点池差异化：给节点打 config 标签
kubectl label node <node> nvidia.com/device-plugin.config=a100-80gb --overwrite

# 🟢 验证：节点容量应出现 4×GPU/卡
kubectl describe node <node> | grep -A3 'nvidia.com/gpu'
```

Pod 侧照常 `resources.limits: {nvidia.com/gpu: 1}`——对业务完全透明。

### 4.2 MIG（GPU Operator migManager）

```yaml
# gpu-operator/mig-config.yaml — mixed 策略示例
apiVersion: v1
kind: ConfigMap
metadata:
  name: mig-parted-config
  namespace: gpu-operator
data:
  config.yaml: |-
    version: v1
    mig-configs:
      all-1g.10gb:
        - devices: all
          mig-enabled: true
          mig-devices:
            1g.10gb: 7
      a100-mixed:
        - devices: [0,1]
          mig-enabled: true
          mig-devices:
            3g.40gb: 2
        - devices: [2,3]
          mig-enabled: true
          mig-devices:
            2g.20gb: 3
        - devices: [4,5,6,7]
          mig-enabled: false    # 整卡保留给训练
```

```bash
# 🟡 安装/确认 migManager 与 MIG 策略
helm upgrade gpu-operator nvidia/gpu-operator -n gpu-operator \
  --set migManager.enabled=true \
  --set migManager.config.name=mig-parted-config

# 🔴 切换节点 MIG 配置（会重置 GPU 并杀掉其上所有进程！）
kubectl label node <gpu-node> nvidia.com/mig.config=a100-mixed --overwrite

# 🟢 观察切分结果
kubectl get node <gpu-node> -o json | jq '.status.allocatable' | grep mig
# 期望出现：nvidia.com/mig-1g.10gb: 14, nvidia.com/mig-2g.20gb: 6, nvidia.com/mig-3g.40gb: 4
```

Pod 申领：

```yaml
resources:
  limits:
    nvidia.com/mig-3g.40gb: 1   # 注意：MIG 模式下必须用 mig 资源名，不能再申 nvidia.com/gpu
```

### 4.3 HAMi（开源池化，思路示例）

```bash
# 🟡 一体化安装（以 HAMi 官方 chart 为例，装前确认与现有调度链路的冲突）
helm repo add hami https://project-hami.github.io/HAMI
helm install hami hami/hami -n kube-system \
  --set devicePlugin.devMemoryDecimals=1024

# 业务侧表达"0.3 张卡 + 12GB 显存"
# metadata.annotations: nvidia.com/vgpu-manager-memory: "12288"
# resources.limits: nvidia.com/gpu: 1（由调度器改写为小数卡语义）
```

升级 K8s 或更换默认调度器前，务必在预发验证 HAMi 的 scheduler 扩展兼容性（这是池化方案最大的长期维护成本）。

### 4.4 DRA 试点

```yaml
# ResourceClaimTemplate：每 Pod 申领一块 MIG 1g.10gb（需 DRA 特性门禁 + NVIDIA DRA driver）
apiVersion: resource.k8s.io/v1beta1
kind: ResourceClaimTemplate
metadata:
  name: mig-1g-claim
spec:
  spec:
    devices:
      requests:
      - name: gpu
        exactly:
          deviceClassName: gpu.nvidia.com
          selectors:
          - cel:
              expression: device.attributes['gpu.nvidia.com'].migProfile == '1g.10gb'
```

v1.34+ 集群默认无需特性门禁即可创建对象，但 GPU 属性申领仍依赖 NVIDIA DRA driver 的成熟度，试点集群建议与生产集群隔离。

---

## 五、调度器集成

### 5.1 扩展资源与拓扑感知

- 所有方案最终都表现为**扩展资源**（`nvidia.com/gpu`、`nvidia.com/mig-*`），原生调度器按整数分配；MIG mixed 节点会出现多种 mig 资源并存，需用 nodeSelector/affinity 收敛 Pod 与规格；
- 多卡训练要**拓扑感知**：GPU Operator 的 device plugin 会暴露 NVLink/PCIe 拓扑标签，配合调度器插件（如 NVIDIA k8s-device-plugin 的优先 NVLink 模式）或 DRA 的拓扑选择器，避免 8 卡训练落成"4+4 跨 PCIe UPI"导致 AllReduce 带宽减半；高性能网络背景见 [[15-AI基础设施/05-K8s-AI基础设施/10-rdma-infiniband-gpudirect-networking.md|RDMA 高性能网络]]。

### 5.2 与 Kueue 配合（配额治理）

```yaml
# ClusterQueue 片段：把 MIG 资源纳入配额
apiVersion: kueue.x-k8s.io/v1beta1
kind: ClusterQueue
metadata:
  name: inference-cq
spec:
  resourceGroups:
  - coveredResources: ["nvidia.com/mig-1g.10gb", "nvidia.com/mig-3g.40gb"]
    flavors:
    - name: mig-flavor
      quota:
        min: {nvidia.com/mig-1g.10gb: "56"}     # 8×A100 节点 ×7
        max: {nvidia.com/mig-3g.40gb: "16"}
```

要点：**共享资源（time-slicing）不建议进配额**——虚拟份数与真实卡数脱钩，配额失真；MIG 实例才是可记账的计量单位。Kueue 语义详见 [[15-AI基础设施/05-K8s-AI基础设施/22-kueue-vs-volcano-batch-scheduling.md|Kueue vs Volcano 批调度对比]]。

---

## 六、故障排查与诊断流程

### 6.1 MIG 配置失败（最常见故障族）

```bash
# 🟢 现象采集：migManager 卡住，节点不出现 mig 资源
kubectl -n gpu-operator logs -l app.kubernetes.io/component=mig-manager --tail=50
kubectl get node <node> -o json | jq '.status.allocatable' | grep -i mig
nvidia-smi mig -lgi    # 节点上直接看当前实例布局
```

| 报错/现象 | 根因 | 处置 |
|-----------|------|------|
| `Unable to create MIG instances … ErrUnsupportedSystem` | fabric-manager 与驱动版本不匹配（SXM 卡） | 🟡 `apt show nvidia-fabricmanager-<ver>` 对齐驱动版本后重启服务 |
| `nvidia-smi` 正常但 allocatable 无 mig 资源 | device plugin 未以 `MIG_STRATEGY=single/mixed` 运行 | 🟡 核对 GPU Operator `devicePlugin.args.migStrategy` 与节点 label 一致 |
| 切分命令报 `MIG mode is disabled` | MIG 模式未启用（需重置 GPU） | 🔴 `nvidia-smi -mig 1` 后切分（杀进程窗口） |
| mixed 切分成功但 Pod Pending | Pod 申的规格该节点没有（如申 2g.20gb 却在 all-1g 节点） | 🟢 `kubectl describe pod` + 检查节点 mig 资源清单与 nodeSelector |
| 切分后部分实例 `Unrecoverable` | ECC/硬件瞬态 | 🟢 记录 XID；🔴 重建实例（`nvidia-smi mig -cgi … -C`），必要时工单换卡 |
| Pod 内 CUDA 报 `cannot use MIG device …` | 容器内 device 注入与 MIG 索引错位 | 🟡 确认镜像 CUDA ≥11.1 且未同时挂整卡与 MIG 设备 |

### 6.2 Time-Slice 争抢导致推理长尾

特征：QPS 平稳但 P99 周期性飙高、`nvidia-smi` 同卡多进程、邻居是训练/批处理任务。

```bash
# 🟢 定位：看每卡上的进程与引擎占用
kubectl exec -it <pod> -- nvidia-smi          # 同一 physical GPU 上的其他 PID？
# 🟢 用 DCGM 看分卡 SM/显存时序（Prometheus）
rate(DCGM_FI_DEV_SM_CLOCK{gpu="3"}[2m])       # 与邻居进程的 GPU index 对齐
DCGM_FI_DEV_FB_USED / DCGM_FI_DEV_FB_FREE     # 显存接近 100% → 超卖 OOM 前兆
```

处置路径：把延迟敏感与批处理负载按 `nodeSelector` 分池（分时节点 vs 独占节点）→ 仍抖动则迁移到 MIG/池化 → 对外 SLA 场景直接禁用分时。监控体系见 [[15-AI基础设施/01-基础设施/04-gpu-monitoring-dcgm.md|GPU 监控 DCGM]]。

### 6.3 共享场景通用检查单

1. Pod 是否拿到了**它以为的**那块卡：`ls /dev/nvidia*` 与 `nvidia-smi -L`（MIG 应显示 MIG 设备）；
2. 显存水位：DCGM `DCGM_FI_DEV_FB_USED` 按物理卡聚合，分时场景之和必须 <100%；
3. XID 事件：`DCGM_FI_DEV_XID_ERRORS` >0 且分时共享 → 大概率邻居连坐，立即隔离；
4. 计费对账：分配视图（kube-state-metrics）vs 用量视图（DCGM）双轨，差异即超卖量。

---

## 七、最佳实践、性能与监控

### 最佳实践

1. **分池而非混池**：独占训练池 / MIG 推理池 / 分时实验池物理隔离，label 贯穿 NodePool→Queue→Pod；
2. **MIG 规格宁少勿多**：每节点池固定 1~2 种规格（如 all-1g.10gb），mixed 只用于过渡期；
3. **MIG 变更是重置操作**：切分窗口纳入变更管理（drain → 切分 → 验证），拒绝"线上热切"；
4. **分时副本数保守**：`replicas ≤ 显存需求上限/物理显存`，为 CUDA context 与碎片留 10% 余量；
5. **计量走 DCGM 不走分配**：共享场景的 chargeback 只能用 per-MIG/per-PID 指标归因。

### 反模式

- ❌ MIG 节点上业务申 `nvidia.com/gpu: 1`（必然 Pending，MIG 资源名不同）；
- ❌ 用 time-slicing 承载对外计费推理（无 QoS、无显存隔离、计量失真三重不合格）；
- ❌ 池化调度器与 Kueue/Volcano 抢同一批 Pod 的调度权（职责边界先画清，见 [[15-AI基础设施/01-基础设施/03-gpu-scheduling-management.md|GPU 调度与管理]]）；
- ❌ 把 DRA 当生产默认（生态未就绪，试点隔离）。

### 监控指标基线

| 指标 | 用途 |
|------|------|
| `DCGM_FI_DEV_FB_USED` / `FB_FREE` | 显存超卖预警（分时池核心告警） |
| `DCGM_FI_PROF_GR_ENGINE_ACTIVE` | 每 MIG 实例算力利用率（计费归因） |
| `DCGM_FI_DEV_XID_ERRORS` | 共享连坐故障的第一信号 |
| `DCGM_FI_PROF_PIPE_TENSOR_ACTIVE` | Tensor Core 占比（判断"显存型小负载"是否值得 MIG） |
| kube-state `kube_node_status_allocatable` 扩展资源 | 分配视图 vs DCGM 用量视图对账 |

---

## 八、选型决策表与生产案例

### 8.1 场景选型决策表

| 场景 | 首选 | 次选 | 禁用 |
|------|------|------|------|
| 对外推理（SLA/P99 敏感） | MIG（3g/1g 按模型规格） | 商业 vGPU | time-slicing |
| 内部推理（容忍抖动） | time-slicing（replicas 2~4） | MIG | — |
| 分布式训练（多机多卡） | 整卡独占 | — | 一切共享方案 |
| 微调（单卡 LoRA） | 整卡或 4g.40gb | 池化显存切分 | time-slicing |
| 开发实验/Notebook | time-slicing | HAMi 小数卡 | 商业 vGPU（license 浪费） |
| 多租户对外卖 GPU | 商业 vGPU / MIG | HAMi + 配额 | time-slicing |
| 云厂商弹性算力池 | 池化（HAMi 类）+ MIG 混合 | — | 单一方案硬扛 |

一句话版：**推理共享 MIG 优先、实验分时凑合、训练永远独占、对外多租户上池化/商业方案、盯住 DRA 演进**。

### 8.2 生产案例：推理集群 MIG 化改造

某推理平台 64×A100-80GB 跑 30+ 个 7B~14B 模型，改造前每模型独占整卡、GPU 时均利用率 18%、扩卡申请排队。

改造：`all-1g.10gb` ×48 卡（小模型 336 实例）+ `3g.40gb` ×8 卡（30B 模型 16 实例）+ 整卡 ×8（保留微调/大模型）。Kueue ClusterQueue 以 mig 实例为配额单位，按业务线分 cohort。

结果：承载模型数 ×3.1，物理卡零新增；P99 反而下降（MIG 实例消除了原先整卡上的混部争抢）；计费从"按卡"改为"按 MIG 实例小时"，业务侧账单下降 40~60%，平台总收益持平。踩坑记录：切换当周 fabric-manager 版本回退导致 3 个节点切分失败（见 6.1），此后升级流程固定了"驱动+fabricmanager 同版本"的预检脚本。

---

## 九、版本兼容性与参考链接

### 9.1 版本兼容速查

| 组件 | 约束 |
|------|------|
| MIG + 驱动 | A100：R470+；H100/H200：R515+（生产建议 R570 系列） |
| fabric-manager | 与驱动**完全同版本**（SXM 卡 MIG 前置条件） |
| GPU Operator | MIG Manager 默认启用；time-slicing 需显式配置 ConfigMap |
| HAMi | 与 K8s 版本有兼容矩阵，升级 K8s 前查 release notes；与 Kueue/Volcano 并存需声明调度边界 |
| DRA | K8s v1.34+ 核心已 Beta；GPU 属性申领依赖 NVIDIA DRA driver 成熟度，试点先行 |
| CUDA | MIG 要求 ≥11.1；容器内挂载 MIG 设备需 nvidia-container-toolkit ≥1.7 |

### 9.2 参考

- NVIDIA MIG 用户指南：https://docs.nvidia.com/datacenter/tesla/mig-user-guide/
- NVIDIA GPU Operator（time-slicing/MIG 配置）：https://docs.nvidia.com/datacenter/cloud-native/gpu-operator/latest/
- KEP-3061 Dynamic Resource Allocation：https://github.com/kubernetes/enhancements/issues/3061
- HAMi 项目：https://github.com/Project-HAMi/HAMi
- 库内延伸：[[15-AI基础设施/05-K8s-AI基础设施/01-gpu-operator-sharing-patterns.md|GPU Operator 部署与共享模式]]、[[15-AI基础设施/05-K8s-AI基础设施/02-gpu-cluster-scheduling-inference-serving.md|GPU 集群调度与推理服务]]、[[15-AI基础设施/05-K8s-AI基础设施/19-volcano-kaiops-gpu-operator-2025.md|Volcano+GPU Operator 2025 实践]]、[[15-AI基础设施/01-基础设施/17-llm-inference-serving.md|LLM 推理服务]]

## Open Questions

- DRA 的 GPU 生产化时间表（NVIDIA DRA driver 从 alpha 到 GA）尚未有官方承诺，2027 年值得复查；
- HAMi 类池化与 Kueue 的"准入-分配"职责如何标准化共存，社区暂无统一模式。

<!-- risk-assessed -->
