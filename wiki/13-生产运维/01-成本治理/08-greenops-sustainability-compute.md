---
title: GreenOps 云原生可持续算力运营
description: 从 EU CSRD 合规动机到 Kepler 能耗归因测量，再到碳感知调度与成熟度治理的 GreenOps 全链路深度实践
summary: GreenOps 深度实践 — 监管动机（CSRD/数据中心能耗申报/碳配额/电价曲线）、PUE/CUE/SCI 指标体系与碳排放范围口径差异、Kepler eBPF+RAPL 每 Pod 能耗测量并与 OpenCost/Kubecost 成本数据 join、分层降碳手段、碳感知调度实操与 L1-L4 成熟度模型
category: production-operations
tags:
- k8s
- greenops
- finops
- sustainability
- kepler
- ebpf
- carbon-aware
- keda
- karmada
- prometheus
- cost-optimization
tier: supporting
created: '2026-10-15'
last_updated: '2026-10-15'
difficulty: advanced
audience:
- SRE
- 平台工程师
- FinOps 工程师
- 架构师
estimated_read_time: 18min
intent_queries:
- GreenOps 是什么，与 FinOps 有什么区别
- 如何在 Kubernetes 上测量每个 Pod 的能耗与碳排放
- Kepler 如何用 eBPF 和 RAPL 做 Pod 级能耗归因
- 如何实现碳感知调度，按电网碳强度时移负载
- SCI 软件碳强度指标怎么计算
- 云厂商碳仪表盘的碳排放范围口径有什么差异
trigger_keywords:
- GreenOps
- 可持续计算
- 碳排放
- Kepler
- 能耗归因
- PUE
- SCI
- 碳感知调度
- 碳强度
- CSRD
prerequisites:
- kubectl-basics
- prometheus-basics
- helm-basics
- cost-governance-basics
k8s_versions:
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
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。

# GreenOps 云原生可持续算力运营

> **适用版本**: Kepler v0.12.0（sustainable-computing-io/kepler，2026-10 核对）
> **定位**: 成本治理域的"碳维度"延伸 — 与 [[wiki/13-生产运维/04-绿色计算/03-greenops-sustainable-computing-guide.md|GreenOps 可持续计算指南]] 互补，本文聚焦 K8s 侧的**测量、归因与调度实操**

---

## 📋 目录

- [一、概述与合规动机](#一概述与合规动机)
- [二、核心指标体系与碳口径](#二核心指标体系与碳口径)
- [三、测量架构：Kepler 深度解析](#三测量架构kepler-深度解析)
- [四、降碳手段分层](#四降碳手段分层)
- [五、碳感知调度实操](#五碳感知调度实操)
- [六、GreenOps 成熟度模型与监控](#六greenops-成熟度模型与监控)
- [七、故障排查与诊断流程](#七故障排查与诊断流程)
- [八、最佳实践与反模式](#八最佳实践与反模式)
- [九、版本兼容性与升级注意](#九版本兼容性与升级注意)

---

## 一、概述与合规动机

### 1.1 什么是 GreenOps

GreenOps 是把"能耗、碳排放"作为与成本同量级的一等运营指标，纳入云原生平台的度量、优化与治理闭环。FinOps 回答"钱花在哪、怎么省"，GreenOps 回答"碳排在哪、怎么降"——两者共享同一套分摊（Chargeback）与右移（Right-Sizing）基础设施，只是计价单位从 `$/Pod` 扩展为 `gCO2eq/Pod`。

### 1.2 为什么现在是入场时间

| 驱动力 | 内容 | 对平台团队的影响 |
|--------|------|------------------|
| EU CSRD | 欧盟《企业可持续发展报告指令》，配套 ESRS E1 气候标准，要求披露范围 1/2/3 排放与能源结构 | 用云企业必须把 IT 排放算到可审计粒度，云上工作负载归因落到团队/Pod |
| EU EED（重修版） | 能效指令要求成员国向欧洲数据中心数据库申报 >500kW 设施的能耗与 PUE | 自建/托管机房被迫公开 PUE，选址与采购决策数据化 |
| 碳配额/内部碳定价 | 越来越多集团对 BU 收内部碳价，碳排放直接变成预算科目 | Pod 级碳账单与成本账单并列，进入 [[wiki/13-生产运维/01-成本治理/01-cost-allocation-chargeback.md|成本分摊]] 体系 |
| 电价曲线 | 现货电价随风光出力波动（欧洲 EPEX 日内价差可达 3-10 倍），电价低谷与碳强度低谷高度相关但非同一曲线 | 计算负载"时移（temporal shifting）"同时省钱与降碳，调度系统需要外部数据输入 |
| 硬件能效拐点 | GPU/AI 负载让单机柜功率密度突破 30-100kW，能耗成本占比首超折旧 | 能耗优化 ROI 超过大部分传统性能优化 |

> **关键认知**：法规要的是"可审计的核算"，工程能做的是"可归因的测量 + 可调度的优化"。两者之间的桥梁就是本文主线——Kepler 测量 → Prometheus 归因 → 调度器执行。

---

## 二、核心指标体系与碳口径

### 2.1 数据中心效率指标

| 指标 | 全称 | 公式 | 归属层级 |
|------|------|------|----------|
| PUE | Power Usage Effectiveness | 总设施能耗 / IT 设备能耗 | 机房（数据中心运营方） |
| WUE | Water Usage Effectiveness | 年耗水量（L）/ IT 设备能耗（kWh） | 机房 |
| CUE | Carbon Usage Effectiveness | 总碳排放（kgCO2eq）/ IT 设备能耗（kWh） | 机房，等价于"运维碳强度" |

PUE 是分摊系数：`Pod 实际碳排 = Pod IT 能耗 × PUE × 电网碳强度`。租户无法改 PUE，只能在**选址（Region）**上选择 PUE 与电网碳强度更优的机房——这是采购/IaC 决策，不是运行时决策。

### 2.2 SCI：软件碳强度（ISO/IEC 21031）

SCI（Software Carbon Intensity）是首个软件层碳排国际标准，定义：

```text
SCI = ((E × I) + M) / R

E = 软件运行能耗（kWh）
I = 电网碳强度（gCO2eq/kWh，位置基准 location-based）
M = 硬件隐含碳（embodied carbon，制造/运输/报废分摊，gCO2eq）
R = 功能单位（每千次请求 / 每用户 / 每小时…）
```

工程要点：**E×I 是运行时项**，K8s 侧可测可优化（本文三至五章）；**M 是隐含碳项**，利用率越高分摊越低——"压榨现有硬件"本身就是降碳，这解释了为什么缩容合并节点比换新低功耗机型更优先；**R 必须先定义**再谈优化，否则 SCI 数字不可比（换功能单位可以"洗绿"指标）。

### 2.3 碳排放范围 1/2/3 与云厂商口径差异

| 范围 | 定义 | K8s 用户相关性 |
|------|------|----------------|
| Scope 1 | 自有设施直接燃烧排放 | 自建机房柴油发电机等，通常极小 |
| Scope 2 | 外购电力（云机房用电） | **主要项**，云上负载的运行时排放 |
| Scope 3 | 价值链其他排放（上游硬件制造、云厂商自身运营等） | 云厂商报表里口径最不一致的部分 |

三大云厂商碳仪表盘的关键差异（以各厂商最新文档为准）：

| 维度 | AWS CCFT | Azure Emissions Impact Dashboard | GCP Carbon Footprint |
|------|----------|----------------------------------|----------------------|
| 覆盖范围 | Scope 2（位置基准）+ 部分 Scope 3 | Scope 2 + Scope 3（含硬件） | Scope 2 + Scope 3 |
| 计价基准 | 以位置基准为主 | 双口径并存 | 位置基准为主，支持无碳能源抵扣调整 |
| 时效 | 滞后约一个季度 | 月级 | 月级（与账单对齐较好） |
| 粒度 | 账户/服务/Region | 订阅/Region | 项目/Region |

> **口径陷阱**：同一套负载在三家报表里数字可能差 2 倍以上。自建核算管道时应固定"位置基准 + 明确 Scope 3 边界"，保留云厂商原始数据做对账，而不是直接搬仪表盘数字进 ESG 报告。

---

## 三、测量架构：Kepler 深度解析

Kepler（Kubernetes-based Efficient Power Level Exporter，CNCF Sandbox，v0.12.0）是 K8s 侧能耗归因的事实标准。eBPF 基础参见 [[wiki/05-网络/05-eBPF/01-ebpf-architecture-fundamentals.md|eBPF 架构基础]] 与 [[wiki/09-可观测性/06-工具/08-ebpf-observability.md|eBPF 可观测性工具]]。

### 3.1 架构：eBPF 计数 + RAPL 能耗 + 比例分摊

```text
┌────────────────────────── Node ──────────────────────────┐
│ eBPF: cgroup 级 CPU 指令数/周期数 + 进程→cgroup 归属       │
│ RAPL（Intel MSR / powercap sysfs）:                       │
│   ├─ Package（CPU 插槽）能耗 counter                      │
│   ├─ DRAM 能耗 counter                                   │
│   └─ GPU 旁路走 DCGM/NVML exporter                       │
│ Kepler Exporter（DaemonSet）:                              │
│   ├─ counter 差分 → 节点总能耗（真值锚点）                  │
│   ├─ Pod 指令/周期占比 × 总能耗 → Pod 能耗归因              │
│   ├─ 无 RAPL 节点 → Model Server ML 估算                  │
│   └─ 暴露 Prometheus 指标（Pod/Node 粒度）                  │
└───────────────────────────────────────────────────────────┘
```

三个设计要点：

1. **RAPL 是"锚"不是"全量"**：只覆盖 Package+DRAM，风扇/主板/电源损耗用固定系数补齐（`other_host_components`）；GPU 能耗依赖 DCGM exporter 的 `DCGM_FI_DEV_POWER_USAGE`。
2. **归因是比例分摊**：Pod 能耗 = 节点实测能耗 ×（Pod 计数器 / 全节点计数器），空闲能耗也按占用比例摊到每个 Pod——这让"缩容到零"在账面上真实有效。
3. **云 VM 无 MSR 时降级到模型估算**：Kepler Model Server 用回归模型推算节点功率，精度低于裸金属 RAPL（误差通常 10%-20% 量级），对比数据时要区分 `power_source` 标签（`rapl` / `redfish` / `model_estimation`）。

### 3.2 部署与核心指标

```bash
# 🟢 低风险：添加 helm repo 并查看可配置项
helm repo add kepler https://sustainable-computing-io.github.io/kepler
helm show values kepler/kepler | less

# 🟡 中风险：部署 Kepler（DaemonSet + 可选 Model Server），先在非生产验证
helm install kepler kepler/kepler --version 0.12.0 \
  --namespace kepler --create-namespace \
  --set model-server.enabled=true

# 🟢 低风险：验证指标暴露（实际指标名以输出为准）
kubectl -n kepler exec ds/kepler-exporter -- \
  wget -qO- localhost:8888/metrics | grep -E '^kepler_(pod|node)_' | head
```

核心指标（v0.12 实际暴露名以 `/metrics` 为准，历史版本有改名）：

| 指标 | 含义 |
|------|------|
| `kepler_node_package_energy_millijoule` | 节点 CPU 插槽累计能耗（mJ） |
| `kepler_node_dram_energy_millijoule` | 节点内存累计能耗（mJ） |
| `kepler_pod_package_energy_millijoule` | Pod 归因的 CPU 能耗（mJ，counter） |
| `kepler_pod_dram_energy_millijoule` | Pod 归因的内存能耗（mJ） |
| `kepler_pod_gpu_energy_millijoule`（GPU 节点） | Pod 归因的 GPU 能耗 |

### 3.3 与成本数据 join 出 gCO2eq/Pod

思路：Kepler 提供"每 Pod 焦耳"，OpenCost/Kubecost 提供"每节点成本"（基础见 [[wiki/13-生产运维/01-成本治理/04-kubecost-finops-automation.md|Kubecost 成本自动化]]），电网碳强度由外部 API 注入为 Prometheus 指标，三者在 recording rule 里合成：

```yaml
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: greenops-carbon-rules
  namespace: kepler
spec:
  groups:
  - name: greenops.carbon
    rules:
    # 每 Pod 功率（瓦）：counter 差分求速率，mJ/s == mW
    - record: greenops:pod_watts
      expr: |
        sum by (namespace, pod, node) (
          rate(kepler_pod_package_energy_millijoule[5m])
        + rate(kepler_pod_dram_energy_millijoule[5m])
        ) / 1000
    # 每 Pod 每小时碳排（gCO2eq/h）= kW × 碳强度(g/kWh) × 1h
    - record: greenops:pod_carbon_grams_per_hour
      expr: |
        greenops:pod_watts / 1000
          * on (node) group_left grid_carbon_intensity_g_per_kwh
    # 每 1000 美元成本对应碳排（碳效率榜，识别"又贵又脏"的负载）
    - record: greenops:pod_carbon_per_cost
      expr: |
        greenops:pod_carbon_grams_per_hour
          / on (node) group_left node_total_hourly_cost * 1000
```

> `node_total_hourly_cost` 来自 OpenCost（Kubecost 同源）。join 键是节点标签——确保两侧节点命名一致。Grafana 出两张核心看板：`Top gCO2eq/h by namespace`（碳榜）与 `gCO2eq per $1000`（碳效率榜）。

---

## 四、降碳手段分层

| 层级 | 手段 | 决策主体 | 典型收益 | 参考 |
|------|------|----------|----------|------|
| IaC/采购 | Region 选址（PUE+电网碳强度）、低碳实例代次、CFE 采购 | 平台/采购 | 最大（一次性 20-50%） | 2.1/2.3 节 |
| 节点级 | 节点自动缩容、C-states、整机功耗上限（RAPL PL1） | 平台 | 5-15% | [[wiki/10-平台工程/02-运维/17-karpenter-node-autoscaling-guide.md|Karpenter 节点弹性]] |
| 调度级 | 碳感知调度（时移/迁移）、Spot/低利用率时段跑批 | 平台+业务 | 5-30%（可时移负载） | 第五章 |
| 应用级 | requests 右移、缩容到零、镜像瘦身、代码能效 | 业务团队 | 5-20% | 下文 4.3 |

### 4.1 IaC/采购层：选址是最大杠杆

- 电网碳强度可差一个数量级（Nordic 电网 <50 vs 煤电主导 Region >500 gCO2eq/kWh），换 Region 的减排幅度远超任何运行时优化。落地为在 Terraform/Cluster API 的 Region 白名单里编码"碳强度上限 + PUE 上限"，新集群只能落在合规 Region，多集群分布参见 [[wiki/10-平台工程/02-运维/10-multi-cluster-management.md|多集群管理]]。
- 约束优先级：数据驻留与延迟合规 > 碳最优，选址降碳必须在合规边界内做。

### 4.2 节点级：缩容与功耗状态

- **节点自动缩容**是第一手段：空节点不是"零碳"，idle 服务器仍吃 30-60% 峰值功率，Karpenter/cluster-autoscaler 的 scale-down 策略直接等价于减排（故障案例见 [[wiki/13-生产运维/05-工单案例/ticket-case-020-cluster-autoscaler-scale-failure.md|缩容失败工单]]）。
- **C-states（CPU 空闲功耗状态）**：空闲核心进深睡眠省电，但深 C-state 唤醒延迟拉高 P99——延迟敏感节点可 `cpupower idle-set -D 1` 限制深度；虚拟化下由宿主决定，租户通常不可控。
- **RAPL 功耗上限（PL1）**：批处理节点限功率换能效（降频后每焦耳算力通常更高），代价是作业变慢，需与业务窗口权衡。

### 4.3 应用级：请求右移与镜像瘦身

- **requests 右移**：过大的 requests 阻断 bin-packing、迫使集群保留更多节点，用 VPA/Goldilocks 修正，见 [[wiki/13-生产运维/01-成本治理/02-idle-resource-right-sizing.md|闲置资源 Right-Sizing]]——碳账与成本账在这里完全同向。
- **缩容到零**：KEDA/Knative 把空闲期 Pod 归零；注意冷启动与控制面常驻能耗的隐性碳排转移。
- **镜像瘦身与代码能效**：1GB 镜像 × 500 节点 × 日均 3 次拉取的传输能耗不可忽略（多阶段构建 + distroless + 镜像预热）；GPU 推理热点路径优化同吞吐降功率，参考 [[wiki/15-AI基础设施/05-K8s-AI基础设施/14-model-serving-autoscaling-keda.md|推理服务弹性]]。

---

## 五、碳感知调度实操

目标：让**可延迟负载**（CI、批处理、模型训练、索引重建）在电网碳强度/电价低谷时段运行，高峰期缩容——"负载数据的削峰填谷"。

### 5.1 数据面：碳强度/电价采集器

```yaml
apiVersion: batch/v1
kind: CronJob
metadata:
  name: grid-carbon-fetcher
  namespace: kepler
spec:
  schedule: "*/15 * * * *"          # 碳强度数据通常 15-60min 粒度
  successfulJobsHistoryLimit: 1
  failedJobsHistoryLimit: 1
  jobTemplate:
    spec:
      template:
        spec:
          serviceAccountName: carbon-fetcher
          restartPolicy: OnFailure
          containers:
          - name: fetcher
            image: curlimages/curl:8.10.1
            envFrom:
            - secretRef:
                name: carbon-api-token
            env:
            - name: ZONE             # ElectricityMaps zone code，如 DE
              value: "DE"
            command:
            - /bin/sh
            - -ce
            - |
              INTENSITY=$(curl -sf -H "Authorization: Bearer $APITOKEN" \
                "https://api.electricitymap.org/v3/carbon-intensity/latest?zone=$ZONE" \
                | sed -n 's/.*"carbonIntensity":\([0-9.]*\).*/\1/p')
              [ -n "$INTENSITY" ] || { echo "fetch failed"; exit 1; }
              cat <<EOF | curl -s --data-binary @- http://pushgateway.kepler:9091/metrics/job/grid-carbon/zone/$ZONE
              # TYPE grid_carbon_intensity_g_per_kwh gauge
              grid_carbon_intensity_g_per_kwh{zone="$ZONE"} $INTENSITY
              # TYPE grid_price_eur_per_kwh gauge
              grid_price_eur_per_kwh{zone="$ZONE"} ${PRICE:-0}
              EOF
```

> 电价用同结构接入（EPEX/ENTSO-E 或国内电网代理购电价目）。电价与碳强度相关性高但非同一曲线：电价含输配费与阻塞，碳强度只反映发电结构——两条曲线都采集，调度策略分别配权重。

### 5.2 执行面 A：KEDA 按碳强度弹性（单集群时移）

延续 [[wiki/10-平台工程/02-运维/18-keda-event-driven-autoscaling-guide.md|KEDA 事件驱动弹性]] 的模式，把"碳强度"当作一个普通的伸缩信号：

```yaml
apiVersion: keda.sh/v1alpha1
kind: ScaledObject
metadata:
  name: batch-worker-carbon-aware
  namespace: batch-jobs
spec:
  scaleTargetRef:
    name: batch-worker            # Deployment，镜像是可断点续跑的批处理 worker
  minReplicaCount: 0              # 碳强度高时缩到零（业务允许时移时）
  maxReplicaCount: 50
  cooldownPeriod: 600
  triggers:
  - type: prometheus
    metadata:
      serverAddress: http://prometheus.monitoring:9090
      query: |
        (grid_carbon_intensity_g_per_kwh < bool 250)
          * clamp_max(pending_job_count, 50)
      threshold: "1"              # 碳强度 <250 且有积压才扩容
```

要点：worker 必须**可中断/可续跑**（任务幂等、进度落盘），否则时移变成丢任务。

### 5.3 执行面 B：Karmada 多集群按碳强度选址（空间迁移）

Karmada 调度器支持自定义插件与 ClusterAffinity。简单落地：给每个成员集群打"碳强度等级"标签，由外部控制器依据各 Region 实时碳强度更新标签，PropagationPolicy 按标签放置：

```yaml
apiVersion: policy.karmada.io/v1alpha1
kind: PropagationPolicy
metadata:
  name: nightly-model-training
  namespace: ml-training
spec:
  resourceSelectors:
  - apiVersions: [batch/v1]
    kind: Job
  placement:
    clusterAffinity:
      labelSelector:
        matchExpressions:
        - key: carbon.karmada.io/intensity-class
          operator: In
          values: ["low", "medium"]     # 只放行低碳集群
    replicaScheduling:
      replicaSchedulingType: Duplicated   # 数据并行训练可复制到多集群
# 外部脚本维持集群标签为实时等级（🟡 修改集群标签，注意标签属主）：
#   kubectl label cluster member-eu-north carbon.karmada.io/intensity-class=low --overwrite
```

生态补充：`kube-green` 适合"夜间整体缩容非关键 Deployment"的简单场景；本节 5.2 的 Kepler+KEDA 组合是 CNCF 社区当前事实上的参考实现。

---

## 六、GreenOps 成熟度模型与监控

### 6.1 L1-L4 成熟度

| 级别 | 名称 | 能力特征 | 出口判据（进入下一级） |
|------|------|----------|------------------------|
| L1 | 度量 | Kepler/OpenCost 部署，能回答"集群/节点能耗多少" | 指标覆盖 >95% 节点，数据连续 30 天无缺口 |
| L2 | 报告 | gCO2eq/Pod/团队级账单与月报，与云厂商仪表盘对账 | 碳账进入团队周会/季度评审，误差 <20% 且可解释 |
| L3 | 优化 | 右移/缩容/碳感知调度上线，减排项有前后对照 | 至少一个可归因的减排闭环（A/B 对照数据） |
| L4 | 治理 | 碳预算（内部碳价）进入配额与准入控制，ESG 审计留痕 | 超碳预算触发准入拦截，报告通过内/外部审计 |

> L2 的"对账"是分水岭：自测数（位置基准、比例分摊）与云仪表盘（口径各异）必然有差异，能解释差异来源才算真正掌握口径。

### 6.2 监控与告警指标

| 指标 | PromQL 要点 | 告警建议 |
|------|-------------|----------|
| 集群碳强度趋势 | `sum(greenops:pod_carbon_grams_per_hour)` | 周环比突增 >30% 告警 |
| 节点能效（J/req） | 能耗 rate ÷ 业务 QPS | 劣化即新版本代码能效回归信号 |
| Kepler 数据缺口 | `up{job="kepler"} == 0` 或 counter 无增长 | P2 告警（账单断账风险） |
| 碳感知策略命中 | KEDA 缩容次数 × 时移任务量 | 周报统计减排量 |

---

## 七、故障排查与诊断流程

```text
现象: Prometheus 里看不到 Pod 能耗
  ├─ Kepler Pod 未 Running？ → kubectl -n kepler get ds（🟢）
  ├─ power_source=model_estimation 且 Model Server 未部署？ → 补启 model-server（🟡）
  └─ cgroup v1/旧内核节点 eBPF map 键不匹配 → 逐节点看 exporter 日志
现象: Pod 能耗占比 >100% 或恒为 0（失真）
  ├─ RAPL counter 回绕/多 NUMA 只读一路 → 对比 powertop/msr 实测（🟢）
  ├─ 虚拟化环境 MSR 未透传 → 换模型估算或裸金属池
  └─ 驻留进程未归因 → 升级 Kepler 修复版本
```

| 现象 | 排查命令（🟢） | 常见根因 |
|------|----------------|----------|
| 某节点无 `kepler_node_*` | `kubectl -n kepler logs ds/kepler-exporter -c kepler-exporter --field-selector spec.nodeName=X` | 内核版本过低，BPF 加载失败 |
| `kepler_pod_gpu_*` 缺失 | `kubectl -n gpu-stack get ds -o wide` | DCGM exporter 未部署/未接入 |
| 碳强度指标断流 | `curl pushgateway:9091/metrics`（🟢） | API 配额耗尽、CronJob RBAC 失败 |
| join 后 Pod 匹配不上成本 | 检查两侧 node 标签拼写 | Node name 与 instance 标签映射缺失 |
| KEDA 时移把在线服务缩没了 | `kubectl -n batch-jobs get hpa`（🟢） | 选错 scaleTarget / 未区分在线与离线负载 |

---

## 八、最佳实践与反模式

### 8.1 最佳实践

1. **先口径后优化**：任何减排动作前固定核算口径（位置基准、PUE 系数、Scope 边界），否则无法证明减排。
2. **复用 FinOps 骨架**：碳分摊、预算、异常检测直接复用成本治理管道，两套账本一个系统，参见 [[wiki/13-生产运维/01-成本治理/06-finops-cost-governance-runbook.md|FinOps 治理 Runbook]]。
3. **时移与在线负载分池**：给可时移 workload 打标（`sched.k8s.io/deferrable=true`），碳感知策略只作用于该池。
4. **减排量用 A/B 对照证明**：同负载"策略开/关"各跑一周，报告差值，避免把自然波动记成功劳。
5. **碳强度进入 Region 选型 IaC 模块**，与延迟/合规约束一起做约束求解，而不是事后迁移。

### 8.2 反模式

| 反模式 | 后果 | 纠正 |
|--------|------|------|
| 为省碳牺牲可靠性 SLO（缩容在线服务、砍多副本） | 一次事故的碳足迹（应急响应、重建、重算）远超省下的电 | 可靠性约束是硬边界，GreenOps 只在边界内寻优 |
| 只算 Scope 2 报喜 | 数字好看但审计被驳回，隐含碳被忽视 | Scope 2+3 同步核算，披露口径写入文档 |
| 用市场基准"买证"抵扣后宣称零碳 | 工程团队失去优化动力 | 运营指标用位置基准，采购抵扣单独记账 |
| 频繁跨 Region 追碳强度 | 迁移流量/数据复制能耗与稳定性成本超过收益 | 空间迁移以天/周为粒度，时移以小时为粒度 |
| 只买低碳电不提利用率 | 硬件隐含碳 M 项居高不下（缺 SCI 分母思维） | 利用率优先：合并、右移、缩容到零 |
| 把 PUE 当租户 KPI | 团队无法影响 PUE，指标失真 | PUE 归平台/采购，租户考核单位功能碳排 |

---

## 九、版本兼容性与升级注意

### 9.1 版本现状（2026-10 核对）

- Kepler：`v0.12.0`（`gh api repos/sustainable-computing-io/kepler/releases/latest`）。Helm chart 与 CRD 同步演进，升级前先 `helm diff`。
- 指标命名在 v0.5→v0.7 期间有破坏性调整（carbon 指标口径变化），跨版本升级后必须回归 dashboard/recording rule；本文 PromQL 以 v0.12 输出为准，落地前用 `/metrics` 实际输出核对。
- Kepler Model Server 独立演进，升级时注意模型文件与 exporter 版本配套。

### 9.2 升级检查单

1. Kepler DaemonSet 滚动后 `power_source` 标签分布是否变化（rapl→estimation 会造成能耗台阶）。
2. KEDA/Karmada 升级后 CRD 语义变化（`replicaScheduling` 字段历史上有行为调整），灰度一个策略再放量。
3. 碳强度 API 供应商改版（如 ElectricityMaps v3 路径）会静默失败——对 fetcher 加 `absent()` 告警。

### 9.3 参考链接与延伸阅读

- Kepler：https://github.com/sustainable-computing-io/kepler
- SCI 标准：ISO/IEC 21031 Software Carbon Intensity spec
- Green Software Foundation：https://greensoftware.foundation
- ElectricityMaps：https://www.electricitymap.org ；WattTime：https://watttime.org
- KEDA：https://keda.sh ；Karmada：https://karmada.io

## Related

- [[wiki/13-生产运维/04-绿色计算/03-greenops-sustainable-computing-guide.md|GreenOps 可持续计算指南]]
- [[wiki/13-生产运维/04-绿色计算/01-green-computing-sustainability.md|绿色计算与可持续性]]
- [[wiki/13-生产运维/01-成本治理/02-idle-resource-right-sizing.md|闲置资源 Right-Sizing]]
- [[wiki/10-平台工程/02-运维/17-karpenter-node-autoscaling-guide.md|Karpenter 节点弹性]]
- [[wiki/10-平台工程/02-运维/18-keda-event-driven-autoscaling-guide.md|KEDA 事件驱动弹性]]
- [[wiki/10-平台工程/02-运维/10-multi-cluster-management.md|多集群管理]]
