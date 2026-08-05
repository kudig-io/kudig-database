---
title: 工作负载最佳实践
description: 大规模 Kubernetes 集群中工作负载的资源管理、QoS、弹性伸缩、PDB、调度约束、优雅上下线与配额治理的生产级最佳实践
summary: 覆盖 requests/limits 设定、QoS 分级、HPA/VPA、PDB、亲和与拓扑分布、探针与优雅停机、配额与准入治理
category: references
tags:
- k8s
- best-practices
- workload
- hpa
- scheduling
- production
tier: core
created: '2026-08-03'
last_updated: '2026-08-03'
difficulty: intermediate
audience:
- SRE
- 应用开发者
- 平台工程师
estimated_read_time: 25min
---

# 工作负载最佳实践

> 大规模集群中，单个工作负载的不规范会被放大为集群级事故。本文给出工作负载全生命周期（定义 → 部署 → 运行 → 扩缩 → 下线）的生产规范。

## 1. 资源管理：requests 与 limits

### 1.1 基本原则

- **每个容器必须设置 `resources.requests`**：调度器按 requests 做装箱，不设 requests 的 Pod 会挤爆节点
- `limits` 策略分语言：
  - **Java/Go 等有运行时感知的应用**：CPU limit 可不设（避免节流），内存 limit 必须设
  - **不设 CPU limit 的前提是**：节点有 `cpu-manager` 或业务能承受争抢；对有严格时延要求的服务改用 static CPU 绑定
- requests 应基于**压测数据**而非拍脑袋：以 P95 使用量为基准，上浮 20–30%

### 1.2 QoS 分级与应用

| QoS | 条件 | 适用场景 |
|---|---|---|
| Guaranteed | requests == limits（CPU + 内存都设且相等） | 核心有状态服务、时延敏感服务 |
| Burstable | requests < limits | 大多数无状态业务 |
| BestEffort | 什么都不设 | **生产禁用**（节点压力时最先被驱逐） |

> 大规模集群建议通过 LimitRange + 准入控制（OPA/Kyverno）**强制拒绝 BestEffort Pod**。

### 1.3 集群级治理

```yaml
# LimitRange：为 Namespace 提供默认值与上限
apiVersion: v1
kind: LimitRange
metadata:
  name: default-limits
spec:
  limits:
  - type: Container
    default:              # 默认 limits
      cpu: "2"
      memory: 4Gi
    defaultRequest:       # 默认 requests
      cpu: 200m
      memory: 256Mi
    max:                  # 单容器上限，防止超大 Pod 拖垮节点
      cpu: "8"
      memory: 16Gi
---
# ResourceQuota：Namespace 级总量管控
apiVersion: v1
kind: ResourceQuota
metadata:
  name: ns-quota
spec:
  hard:
    requests.cpu: "100"
    requests.memory: 200Gi
    limits.cpu: "200"
    limits.memory: 400Gi
    pods: "500"
    services.loadbalancers: "5"    # 防止滥用云 LB 产生费用
    persistentvolumeclaims: "100"
```

## 2. 弹性伸缩

### 2.1 HPA

- 指标优先级：CPU/内存（基础）→ custom metrics（QPS、队列长度）→ external metrics
- 大规模注意：**避免所有 HPA 在同一时刻共振扩容**——配置 `behavior.scaleUp/scaleDown` 稳定窗口

```yaml
behavior:
  scaleDown:
    stabilizationWindowSeconds: 300   # 缩容冷静期，防止抖动
    policies:
    - type: Percent
      value: 50
      periodSeconds: 60
  scaleUp:
    stabilizationWindowSeconds: 30
    policies:
    - type: Percent
      value: 100
      periodSeconds: 30
    - type: Pods
      value: 10
      periodSeconds: 30
    selectPolicy: Max
```

- HPA 副本上限必须与 PDB、配额、集群容量联动评估
- **不要把 HPA 和 VPA 同时作用于同一指标**（如 CPU）——会互相打架

### 2.2 VPA

- 用于 requests 自动推荐/调整；生产建议先用 `updateMode: "Off"` 只出建议，人工确认
- 与 HPA 组合：HPA 管副本数（水平），VPA 管 requests（垂直），指标分离

### 2.3 集群级弹性

- Cluster Autoscaler：节点组粒度，注意 `--max-node-provision-time` 与云厂商供给 SLA
- Karpenter：大规模集群更优，按需供给异构机型；注意配置 `disruption` 策略防止频繁重排
- **节点扩容速度是 HPA 生效的前提**：压测时必须验证"HPA 触发 → CA 供给节点 → Pod 就绪"的端到端时延

## 3. 可用性：PDB 与调度约束

### 3.1 PodDisruptionBudget

- **所有多副本生产负载必须配置 PDB**，否则节点维护/驱逐会击穿服务

```yaml
apiVersion: policy/v1
kind: PodDisruptionBudget
metadata:
  name: app-pdb
spec:
  minAvailable: 50%        # 或 maxUnavailable: 1
  selector:
    matchLabels:
      app: myapp
```

- 注意：PDB 只约束**主动驱逐**（eviction），不约束节点故障、OOM 等非自愿中断
- unhealthyPodEvictionPolicy（1.27+）：`IfHealthyBudget` 可放行不健康 Pod 的驱逐，避免僵死

### 3.2 拓扑分布

大规模集群优先用 `topologySpreadConstraints` 替代手写反亲和：

```yaml
topologySpreadConstraints:
- maxSkew: 1
  topologyKey: topology.kubernetes.io/zone   # 跨可用区
  whenUnsatisfiable: ScheduleAnyway          # 硬约束用 DoNotSchedule
  labelSelector:
    matchLabels:
      app: myapp
- maxSkew: 1
  topologyKey: kubernetes.io/hostname        # 跨节点
  whenUnsatisfiable: DoNotSchedule
  labelSelector:
    matchLabels:
      app: myapp
```

### 3.3 亲和性使用原则

- `podAntiAffinity` 同副本打散：副本数 ≤ 节点数时用 required；副本多时用 preferred
- 有状态服务（数据库中间件）用 required 级节点反亲和，宁可 Pending 不混部
- 避免深层 nodeAffinity 链导致调度器计算放大——大集群下亲和规则过多会显著拖慢调度

## 4. 健康检查与优雅上下线

### 4.1 探针规范

| 探针 | 用途 | 关键参数 |
|---|---|---|
| startupProbe | 慢启动应用兜底（JVM 启动 1–3 分钟） | `failureThreshold × periodSeconds` 覆盖最长启动时间 |
| readinessProbe | 摘流/接流 | 短间隔（5–10s），**不要**在探针里检查下游依赖 |
| livenessProbe | 自愈重启 | 长间隔、宽松阈值，防止雪崩式连环重启 |

> 反模式：readinessProbe 里检查数据库连通性——数据库抖动会导致**全副本摘流**，放大故障。

### 4.2 优雅下线

```yaml
spec:
  terminationGracePeriodSeconds: 60
  containers:
  - name: app
    lifecycle:
      preStop:
        exec:
          command: ["/bin/sh", "-c", "sleep 15"]  # 等待 endpoints 摘除传播
```

- 下线时序：收到 SIGTERM → 停止接新请求 → 处理完存量请求 → 退出
- `preStop` sleep 是为了弥补 kube-proxy/endpoint 传播延迟（尤其 IPVS/大集群下传播慢）
- 应用必须正确处理 SIGTERM；Java 注意 `-XX:+ExitOnOutOfMemoryError` 与 shutdown hook

## 5. 镜像与发布规范

- **禁止使用 `latest` 标签**；镜像 tag 用不可变语义（git commit / 语义化版本）
- 镜像大小治理：多阶段构建、distroless/alpine 基础镜像，目标 < 500 MB
- 生产命名空间 `imagePullPolicy` 与准入策略：禁止 `:latest`、必须来自受信仓库
- 发布策略：
  - 无状态：RollingUpdate，`maxSurge: 25%`, `maxUnavailable: 0`（宁可慢不能断）
  - 有状态：OnDelete / Partition 滚动，人工控制节奏
  - 高风险变更：金丝雀（Argo Rollouts / Flagger）+ 自动分析回滚

## 6. 安全基线（工作负载侧）

```yaml
securityContext:
  runAsNonRoot: true
  runAsUser: 1000
  allowPrivilegeEscalation: false
  readOnlyRootFilesystem: true        # 可写路径用 emptyDir 挂载
  capabilities:
    drop: ["ALL"]
  seccompProfile:
    type: RuntimeDefault
```

- 命名空间启用 **Pod Security Standards**（`restricted` 级）；存量集群先 `audit/warn` 再 `enforce`
- `automountServiceAccountToken: false` 为默认，需要访问 APIServer 的组件单独开启 + 最小 RBAC
- 详见 [[08-security-defense-checklist]]

## 7. 特殊负载类型

### 7.1 DaemonSet

- 节点级组件（日志采集、监控 Agent、CNI）必须设置资源 requests，防止挤占业务
- 大规模集群下 DaemonSet 滚动更新要分批（`maxUnavailable` 小步）——全集群 Agent 同时重启会形成风暴

### 7.2 StatefulSet

- `podManagementPolicy: OrderedReady`（默认）保证顺序；允许并行的场景用 `Parallel`
- 存储卷用 volumeClaimTemplates，PVC 回收策略与备份策略配套（见 [[05-storage]]）

### 7.3 批处理/AI 负载

- 大规模批任务用 **Kueue / Volcano** 做队列与配额管理，禁止裸提海量 Job 直接压垮调度器
- GPU 负载：独占卡调度、拓扑感知（NVLink/NUMA）、镜像预热到节点

## 8. 大规模特有注意事项

| 问题 | 表现 | 对策 |
|---|---|---|
| 全量重启风暴 | 集群级事件后数万 Pod 同时拉镜像 | 镜像预热、P2P 分发、分批重启 |
| HPA 共振 | 多服务同一指标同时扩容击穿集群容量 | 稳定窗口 + 集群容量水位预留（≥ 20% headroom） |
| endpoints 爆炸 | 单 Service 后端上千 Pod，kube-proxy 规则同步慢 | 拆分 Service、用 externalTrafficPolicy: Local 减规则量 |
| CronJob 集中触发 | 整点/零点海量 Job 同时启动 | 使用 `startingDeadlineSeconds` + 分散调度时间 |

## 9. 常见反模式

| 反模式 | 后果 |
|---|---|
| 不设 requests | 节点超卖，高负载时整机抖动、连锁驱逐 |
| readiness 探针查下游 | 依赖抖动导致全副本摘流，自我雪崩 |
| 无 PDB | 节点维护直接击穿服务 |
| maxUnavailable > 0 的发布 | 发布过程容量下降，叠加流量高峰引发故障 |
| 单 Deployment 上千副本 | 滚动更新/事件风暴，拆分为多实例部署 |

## Related

- [[01-overview|大规模集群总览与规模基线]]
- [[05-storage|存储最佳实践]]
- [[07-pre-production-checklist|生产上线前检查项]]
- [[20-最佳实践/07-scenarios/app-deployment|应用部署场景]]
