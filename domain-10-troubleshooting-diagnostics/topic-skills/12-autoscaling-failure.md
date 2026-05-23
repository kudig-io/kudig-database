---
title: HPA/VPA/Cluster Autoscaler 弹性伸缩故障诊断 / Autoscaling Failure Diagnosis & Remediation
description: '## 1. 概述'
category: scaling
tags:
- k8s
- skills
- sop
- runbook
- apiserver
- kubelet
- prometheus
- helm
- kafka
- hpa
last_updated: '2026-04-26'
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 技术支持
estimated_read_time: 20min
intent_queries:
- HPA/VPA/Cluster Autoscaler 弹性伸缩故障诊断 / Autoscaling Failure Diagnosis & Remediation 是什么
- 如何 HPA/VPA/Cluster Autoscaler 弹性伸缩故障诊断 / Autoscaling Failure Diagnosis & Remediation
trigger_keywords:
- HPA not scaling
- VPA recommendation not applied
- cluster autoscaler failed
- node pool scale up failed
- metrics server unavailable
- custom metrics missing
- KEDA scaledobject error
- scaling delay
- autoscaler flapping
- resource fragmentation
- HPA 不扩容
- HPA 不缩容
- VPA 推荐值异常
- 节点池扩容失败
- 自动扩缩容不生效
- 指标获取失败
prerequisites:
- kubectl-basics
- troubleshooting-methodology
- helm-basics
- prometheus-basics
- kafka-basics
- gpu-scheduling-basics
skill_id: SKILL-12_AUTOSCALING_FAILURE-001
skill_name: HPA/VPA/Cluster Autoscaler 弹性伸缩故障诊断 / Autoscaling Failure Diagnosis & Remediation
version: 1.0.0
k8s_versions:
- 1.28.x
- 1.29.x
- 1.30.x
- 1.31.x
- 1.32.x
agent_execution_mode: L2-semi-auto
created: "2026-05-23"
---

<!-- condition: kubectl get hpa -A -o jsonpath='{range .items[?(@.status.currentReplicas != @.status.desiredReplicas)]} {.metadata.namespace}/{.metadata.name}{"\n"}{end}' 显示副本数不匹配 -->

# HPA/VPA/Cluster Autoscaler 弹性伸缩故障诊断 / Autoscaling Failure Diagnosis & Remediation

---

## 1. 概述

弹性伸缩是 [[Kubernetes|Kubernetes]] 实现资源效率和应用高可用的核心能力。当弹性伸缩失效时，可能导致**资源浪费**（无法缩容）、**服务降级**（无法扩容）或**成本失控**。Kubernetes 提供三个层次的弹性伸缩机制，本 [[SKILL|Skill]] 覆盖它们的完整故障诊断：

- **HPA (Horizontal Pod Autoscaler)**: 基于指标（CPU/Memory/自定义指标/外部指标）自动调整 Pod 副本数
- **VPA (Vertical Pod Autoscaler)**: 自动调整 Pod 的 CPU/Memory requests 和 limits
- **Cluster Autoscaler (CA)**: 基于 Pending Pod 自动调整集群节点数量
- **[[KEDA|KEDA]] (Kubernetes Event-Driven Autoscaling)**: 基于事件源（Kafka/RabbitMQ/Prometheus 等）的扩缩容

### 典型触发场景

1. **HPA 扩容失败**: 流量激增时 HPA 未能及时扩容，导致服务响应变慢或请求失败
2. **VPA 推荐值不生效**: VPA 给出了资源调整建议但 Pod 未按预期更新
3. **Cluster Autoscaler 节点扩容卡住**: Pod Pending 但集群未自动添加新节点
4. **伸缩抖动（Flapping）**: HPA 频繁扩缩导致服务不稳定，资源浪费
5. **成本异常**: 缩容不及时或缩容被阻止，导致资源闲置和成本增加

### 前置条件

- **RBAC 权限**: 对 HPA/VPA/Deployment/ConfigMap 的 get/list/watch/update 权限
- **Metrics Server**: 已部署且正常运行（HPA 基于 CPU/Memory 时必需）
- **工具要求**: kubectl (v1.28+), metrics-server, jq（可选）
- **云厂商 CLI**: 阿里云 CLI (aliyun) / AWS CLI (aws) / GCP CLI (gcloud)（节点池操作时需要）

> ⚠️ **重要**: HPA/VPA 不应同时配置管理同一资源的相同指标。VPA 在 Auto/Recreate 模式下会重启 Pod，生产环境需谨慎使用。

---

## 2. 症状识别

### 2.1 症状模式表

| # | 症状描述 | 检测方法 | 置信度 | 排除条件 |
|---|---------|---------|--------|---------|
| S1 | HPA TARGETS 列显示 `<unknown>/XX%` / HPA targets show unknown | `kubectl get hpa -A` 查看 TARGETS 列是否有 `<unknown>` | 0.95 | 刚创建 HPA 尚未完成首次指标采集（等待 15-30s） |
| S2 | HPA currentReplicas 长期不变，既不扩也不缩 / HPA replicas unchanged | `kubectl get hpa -A -o wide` 对比 MINPODS/MAXPODS/REPLICAS，观察数分钟无变化 | 0.85 | 当前负载恰好稳定在目标值附近；已达到 min/max 边界 |
| S3 | HPA 频繁扩缩（flapping），Replicas 在两个值之间震荡 / HPA flapping | `kubectl describe hpa <name>` 查看 Events，短时间内交替出现 ScaleUp/ScaleDown | 0.90 | 业务流量确实高频波动（如促销秒杀场景） |
| S4 | VPA recommendation 为空或所有值为 0 / VPA recommendation empty | `kubectl describe vpa <name>` 查看 Recommendation 字段为空或 CPU/Memory 为 0 | 0.85 | VPA 刚创建尚未完成首次推荐（需等待 5-10 分钟） |
| S5 | VPA UpdateMode=Auto 但 Pod 不重启，资源未更新 / VPA not applying recommendations | `kubectl get vpa <name> -o yaml` 显示 UpdateMode=Auto，但 Pod resources 与推荐值不一致 | 0.80 | VPA admission controller 正常但推荐变化幅度小于阈值 |
| S6 | Cluster Autoscaler 日志 "could not scale up" / CA scale up failed | `kubectl logs -n kube-system deploy/cluster-autoscaler --tail=100 \| grep -i "could not"` | 0.90 | 节点池已达最大容量（预期行为） |
| S7 | 节点池扩容成功但 Pod 仍 Pending / Node added but [[Pods|pods]] still pending | `kubectl get nodes` 显示新节点 Ready，但 `kubectl get pods` 仍有 Pending | 0.85 | 新节点未满足 Pod 的 nodeSelector/affinity/tolerations |
| S8 | 节点缩容被阻止，CA 日志显示 "cannot be removed" / Scale down blocked | `kubectl logs -n kube-system deploy/cluster-autoscaler --tail=200 \| grep "cannot be removed"` | 0.90 | 节点上有 PDB 保护的 Pod 且剩余节点无法承载（预期行为） |
| S9 | KEDA ScaledObject 状态为 Unknown 或 Error / KEDA ScaledObject unhealthy | `kubectl get scaledobject -A` 查看 READY 列显示 False 或 Unknown | 0.85 | KEDA operator 刚部署或重启中 |
| S10 | Metrics Server Pod CrashLoopBackOff 或 Pending / Metrics Server unhealthy | `kubectl get pods -n kube-system -l k8s-app=metrics-server` 状态异常 | 0.95 | 集群未安装 Metrics Server（需要确认是否需要 HPA） |
| S11 | 自定义指标 API 返回错误 / Custom metrics API error | `kubectl get --raw /apis/custom.metrics.k8s.io/v1beta1/` 返回 404 或错误 | 0.85 | 集群未配置 Prometheus Adapter 或 KEDA（如仅使用内置指标则正常） |

### 2.2 工单关键词映射

以下为常见工单描述示例，Agent 应将其映射到本 Skill：

**中文工单描述**:
- "HPA 配置了但是不生效，一直是 1 个副本"
- "指标显示 unknown，HPA 无法获取 CPU 使用率"
- "节点池扩容失败，Pod 一直 Pending"
- "VPA 推荐的值很奇怪，内存给了 50GB"
- "自动伸缩一会扩一会缩，太频繁了"
- "Cluster Autoscaler 日志报错，节点加不上去"
- "KEDA 从 Kafka 拿不到指标，ScaledObject 不工作"
- "成本告警，集群节点数量异常多"
- "缩容不下来，闲置节点太多"

**English ticket descriptions**:
- "HPA is not scaling up even though CPU is high"
- "VPA recommendations are not being applied to pods"
- "Cluster autoscaler keeps failing to add nodes"
- "Metrics server showing unknown for HPA targets"
- "Custom metrics API returns 404"
- "KEDA scaledobject stuck in error state"
- "Autoscaler flapping, causing service instability"
- "Node pool scale down blocked"
- "Scaling latency too high during traffic spike"

### 2.3 排除标准

以下场景**不适用**本 Skill，Agent 应路由到其他 Skill 或手动处理：

| 排除条件 | 正确路由 | 说明 |
|---------|---------|------|
| HPA 配置正确但 Pod 扩容后仍无法调度 | SKILL-POD-002 | Pod Pending 问题，可能是资源不足或调度约束 |
| Pod 已扩容但应用性能未改善 | 应用性能调优 | 非弹性伸缩问题，可能是应用瓶颈 |
| 节点 NotReady 导致 CA 行为异常 | SKILL-NODE-001 | 先解决节点问题再处理伸缩问题 |
| HPA 正常工作但业务方认为扩容阈值设置不合理 | 配置优化讨论 | 非故障，是阈值参数调优需求 |
| VPA 在 Off 模式下仅提供建议不自动应用 | 预期行为 | UpdateMode=Off 时 VPA 仅推荐不执行 |
| 多租户集群中 ResourceQuota 阻止扩容 | 资源配额管理 | 检查 namespace 配额设置 |

---

## 3. 快速分级（2 分钟内完成）

### 3.1 影响评估

按顺序执行以下命令，判断故障爆炸半径：

**Step T1**: 统计异常 HPA 数量和生产环境占比（15s）
```bash
# 获取所有 HPA 状态，统计 targets 为 unknown 的数量
kubectl get hpa -A 2>/dev/null | grep -c "unknown" && \
echo "Total HPAs:" && kubectl get hpa -A --no-headers 2>/dev/null | wc -l

# 或更精确的检查
kubectl get hpa -A -o jsonpath='{range .items[*]}{.metadata.namespace}/{.metadata.name} TARGETS={.status.currentMetrics[*].resource.current.averageUtilization}{"\n"}{end}'
```
> **判断规则**:
> - 所有 HPA targets 均为 unknown → **Metrics Server 故障**（P1），跳转 D1.3
> - 部分 HPA targets 为 unknown → **部分指标采集问题**（P2），继续 T2
> - 生产环境关键服务的 HPA 异常 → 升级为 **P1**

**Step T2**: 检查 Pending Pod 和 Cluster Autoscaler 状态（30s）
```bash
# 检查是否有 Pending Pod（可能需要 CA 扩容）
kubectl get pods -A --field-selector=status.phase=Pending --no-headers | wc -l

# 检查 CA 状态（如果部署了 CA）
kubectl get configmap -n kube-system cluster-autoscaler-status -o yaml 2>/dev/null | grep -A5 "ScaleUp\|ScaleDown"
```
> **判断规则**:
> - Pending Pod > 10 且 CA 日志显示扩容失败 → **P1**（严重影响服务可用性）
> - Pending Pod > 0 但 CA 正在处理中 → **P2**（等待 CA 扩容）
> - 无 Pending Pod 但缩容被阻止 → **P3**（成本影响）

**Step T3**: 检查 Metrics Server 健康状态（30s）
```bash
# 检查 Metrics Server 部署状态
kubectl get deploy -n kube-system metrics-server

# 测试 Metrics API 可用性
kubectl get --raw /apis/metrics.k8s.io/v1beta1/nodes 2>&1 | head -3

# 快速验证 top 命令
kubectl top nodes --use-protocol-buffers 2>&1 | head -3
```
> **判断规则**:
> - Metrics Server Pod 异常或 API 不可用 → 所有 HPA 将失效，**P1**
> - Metrics API 正常但部分节点无数据 → **P2**
> - 一切正常 → 继续其他维度排查

### 3.2 严重性分级

| 条件 | 级别 | 说明 | SLA 要求 |
|------|------|------|---------|
| Metrics Server 完全不可用 **或** 多个生产服务 HPA 失效 | **P1** | 所有 HPA 无法获取指标，扩容完全失效，流量高峰期可能导致服务崩溃 | 立即响应，30min 内修复 |
| 单个关键服务 HPA 异常 **或** CA 扩容持续失败导致大量 Pod Pending | **P1** | 影响特定但关键的服务可用性 | 立即响应，30min 内修复 |
| 部分 HPA 指标异常 **或** VPA 推荐异常 | **P2** | 影响资源效率但不直接影响服务可用性 | 30min 内响应，2h 内修复 |
| CA 缩容被阻止 **或** 非关键服务伸缩异常 | **P3** | 主要影响成本，对服务稳定性影响有限 | 4h 内处理 |

### 3.3 立即升级触发条件

以下任一条件满足时，**跳过诊断流程，立即升级至人工 SRE / 值班工程师**：

- **核心服务不可用**: HPA 失效导致核心服务 Pod 数量不足，请求大量失败
- **成本失控**: CA 异常导致节点数量在短时间内翻倍或更多
- **云厂商 API 故障**: CA 与云厂商 API 通信完全失败（需云厂商支持介入）
- **数据一致性风险**: VPA Auto 模式异常导致 StatefulSet Pod 意外重启
- **多集群级联**: 多个集群同时出现相同伸缩故障

> **升级消息模板**: 参见 Section 8.2

---

## 4. 诊断工作流

### Phase 1: HPA 快速诊断（只读，零风险）

> **目标**: 快速定位 HPA 不生效的原因，区分指标问题、配置问题或目标工作负载问题。
> **预计耗时**: 3-5 分钟

**Step D1.1**: 获取 HPA 全局状态概览
- **命令**:
  ```bash
  kubectl get hpa -A -o wide
  ```
- **超时**: 10s
- **预期输出模式**: 表格输出包含 NAMESPACE, NAME, REFERENCE, TARGETS, MINPODS, MAXPODS, REPLICAS, AGE
- **判断规则**:
  - TARGETS 列显示 `<unknown>/<target>%` → 指标获取失败，继续 D1.3 检查 Metrics Server
  - REPLICAS 等于 MINPODS 且 TARGETS 远高于目标 → 可能无法扩容（检查 MAXPODS 或调度问题）
  - REPLICAS 等于 MAXPODS → 已达扩容上限，检查是否需要调整配置
  - TARGETS 显示正常数值 → HPA 正在工作，继续 D1.2 深入检查
- **版本差异**: 无

**Step D1.2**: 获取 HPA 详细状态和 Conditions
- **命令**:
  ```bash
  kubectl describe hpa <hpa-name> -n <namespace>
  ```
- **超时**: 10s
- **预期输出模式**: 关注以下字段：
  ```
  Conditions:
    Type            Status  Reason            Message
    ----            ------  ------            -------
    AbleToScale     True    SucceededRescale  the HPA controller was able to update the target scale
    ScalingActive   True    ValidMetricFound  the HPA was able to successfully calculate a replica count
    ScalingLimited  False   DesiredWithinRange  the desired count is within the acceptable range
  ```
- **判断规则**:
  - `ScalingActive=False` + Reason=`FailedGetResourceMetric` → RC-001（Metrics Server 问题）
  - `ScalingActive=False` + Reason=`FailedGetExternalMetric` → RC-006（自定义指标 Adapter 问题）
  - `AbleToScale=False` + Reason=`FailedRescale` → RC-003（HPA 配置或目标问题）
  - `ScalingLimited=True` + Reason=`TooFewReplicas` → 已达 minReplicas 无法缩容
  - `ScalingLimited=True` + Reason=`TooManyReplicas` → 已达 maxReplicas 无法扩容
  - Events 中出现 `FailedGetResourceMetric` → 目标 Pod 未设置 resources.requests（RC-002）
- **版本差异**:
  - **[v1.28+]**: HPA v2 API 是默认版本，v1 API（HPA v1）已弃用但仍可用
  - **[v1.30+]**: HPA ContainerResource 指标支持更精确的容器级别指标

**Step D1.3**: 检查 Metrics Server 健康状态
- **命令**:
  ```bash
  # 检查 Metrics Server 部署
  kubectl get deploy -n kube-system metrics-server
  
  # 检查 Metrics Server Pod 状态和日志
  kubectl get pods -n kube-system -l k8s-app=metrics-server
  kubectl logs -n kube-system -l k8s-app=metrics-server --tail=50
  
  # 测试 Metrics API
  kubectl get --raw /apis/metrics.k8s.io/v1beta1/nodes | jq '.items[].metadata.name' 2>/dev/null || echo "Metrics API failed"
  ```
- **超时**: 15s
- **预期输出模式**: Deployment Ready, Pod Running, API 返回节点列表
- **判断规则**:
  - Metrics Server Pod 不存在或 CrashLoopBackOff → RC-001（Metrics Server 未部署或异常）
  - Pod Running 但 API 返回错误 → 检查 Metrics Server 启动参数和证书
  - Pod 日志包含 `certificate` 或 `TLS` 错误 → 证书配置问题
  - Pod 日志包含 `unable to fetch node metrics` → kubelet 10250 端口不可达
  - API 正常但 `kubectl top pods` 部分失败 → 特定节点的 kubelet 问题
- **版本差异**: 无

**Step D1.4**: 检查目标工作负载的 resources.requests 配置
- **命令**:
  ```bash
  # 获取 HPA 目标 Deployment 的 Pod 模板
  HPA_TARGET=$(kubectl get hpa <hpa-name> -n <namespace> -o jsonpath='{.spec.scaleTargetRef.name}')
  kubectl get deploy $HPA_TARGET -n <namespace> -o jsonpath='{.spec.template.spec.containers[*].resources}' | jq .
  
  # 或直接检查 Pod 的资源请求
  kubectl get pods -n <namespace> -l app=<app-label> -o jsonpath='{range .items[*]}{.metadata.name}: cpu={.spec.containers[*].resources.requests.cpu}, memory={.spec.containers[*].resources.requests.memory}{"\n"}{end}'
  ```
- **超时**: 10s
- **预期输出模式**: CPU 和 Memory 的 requests 值
- **判断规则**:
  - `requests.cpu` 或 `requests.memory` 为空/null → RC-002（Pod 未设置 resources.requests）
  - requests 值设置过高 → 可能导致 HPA 计算的利用率偏低，不触发扩容
  - requests 值设置过低 → 可能导致 HPA 计算的利用率偏高，频繁扩缩
- **版本差异**: 无

**Step D1.5**: 检查自定义指标 API（如配置了自定义指标）
- **命令**:
  ```bash
  # 检查自定义指标 API 是否可用
  kubectl get --raw /apis/custom.metrics.k8s.io/v1beta1/ 2>&1 | head -20
  
  # 列出可用的自定义指标
  kubectl get --raw /apis/custom.metrics.k8s.io/v1beta1/ | jq '.resources[].name' 2>/dev/null
  
  # 检查 Prometheus Adapter 部署（如果使用）
  kubectl get deploy -n custom-metrics prometheus-adapter 2>/dev/null || \
  kubectl get deploy -A -l app.kubernetes.io/name=prometheus-adapter 2>/dev/null
  ```
- **超时**: 15s
- **预期输出模式**: API 返回资源列表
- **判断规则**:
  - API 返回 404 → 未部署自定义指标 Adapter（RC-006）
  - API 可用但指标列表为空 → Adapter 配置问题（RC-006）
  - API 可用但查询特定指标失败 → 指标名称或配置错误（RC-003）
- **版本差异**: 无

**Step D1.6**: 检查外部指标 API（如配置了外部指标）
- **命令**:
  ```bash
  # 检查外部指标 API 是否可用
  kubectl get --raw /apis/external.metrics.k8s.io/v1beta1/ 2>&1 | head -20
  
  # 如果使用 KEDA，检查 KEDA metrics-apiserver
  kubectl get deploy -n keda keda-metrics-apiserver 2>/dev/null
  kubectl get apiservice v1beta1.external.metrics.k8s.io -o yaml 2>/dev/null | grep -A5 "service:"
  ```
- **超时**: 10s
- **预期输出模式**: API 返回资源列表
- **判断规则**:
  - API 返回 404 → 未部署外部指标提供者（KEDA/Stackdriver Adapter 等）
  - API 返回错误 → 外部指标源连接问题（RC-009）
  - API 可用但查询失败 → 需检查具体触发器配置
- **版本差异**: 无

---

### Phase 2: VPA/Cluster Autoscaler 深度诊断（只读，零风险）

> **目标**: 诊断 VPA 推荐异常和 Cluster Autoscaler 扩缩容问题。
> **预计耗时**: 5-10 分钟

**Step D2.1**: 检查 VPA 状态和推荐值
- **命令**:
  ```bash
  # 获取所有 VPA 状态
  kubectl get vpa -A -o wide
  
  # 查看特定 VPA 的详细推荐
  kubectl describe vpa <vpa-name> -n <namespace>
  
  # 检查 VPA Recommendation 详情
  kubectl get vpa <vpa-name> -n <namespace> -o jsonpath='{.status.recommendation.containerRecommendations[*]}' | jq .
  ```
- **超时**: 10s
- **预期输出模式**: VPA 状态和推荐的 CPU/Memory 值
- **判断规则**:
  - Recommendation 为空 → VPA Recommender 可能异常（RC-004），继续 D2.2
  - Recommendation 值异常高或低 → 检查历史指标数据或 VPA 配置
  - `UpdateMode: "Off"` → 仅推荐不自动应用，属于预期行为
  - `UpdateMode: "Auto"` 但 Pod 资源未更新 → VPA Admission Controller 问题
- **版本差异**: 无

**Step D2.2**: 检查 VPA 组件日志
- **命令**:
  ```bash
  # VPA Recommender 日志
  kubectl logs -n kube-system deploy/vpa-recommender --tail=100 2>/dev/null || \
  kubectl logs -n vpa deploy/vpa-recommender --tail=100 2>/dev/null
  
  # VPA Updater 日志（负责驱逐 Pod 以应用新配置）
  kubectl logs -n kube-system deploy/vpa-updater --tail=100 2>/dev/null
  
  # VPA Admission Controller 日志
  kubectl logs -n kube-system deploy/vpa-admission-controller --tail=100 2>/dev/null
  ```
- **超时**: 15s
- **预期输出模式**: 正常的推荐计算日志
- **判断规则**:
  - Recommender 日志包含 `error fetching metrics` → Prometheus/Metrics 源连接问题
  - Recommender 日志包含 `no pods found` → VPA 目标 selector 配置错误
  - Updater 日志包含 `eviction blocked by PDB` → PDB 阻止了 Pod 驱逐（RC-005）
  - Admission Controller 日志包含错误 → Webhook 配置或证书问题
- **版本差异**: 无

**Step D2.3**: 检查 VPA 与 HPA 冲突
- **命令**:
  ```bash
  # 检查同一目标是否同时配置了 HPA 和 VPA
  VPA_TARGET=$(kubectl get vpa <vpa-name> -n <namespace> -o jsonpath='{.spec.targetRef.name}')
  echo "VPA target: $VPA_TARGET"
  kubectl get hpa -n <namespace> -o jsonpath='{range .items[*]}{.metadata.name} -> {.spec.scaleTargetRef.name}{"\n"}{end}' | grep "$VPA_TARGET"
  ```
- **超时**: 5s
- **预期输出模式**: 显示是否有冲突
- **判断规则**:
  - 同一 Deployment 同时配置了 HPA（基于 CPU/Memory）和 VPA → RC-005
  - VPA 仅配置 CPU 建议，HPA 仅配置 Memory 指标 → 可以共存
  - 使用 KEDA 替代 HPA 时，可以与 VPA 共存
- **版本差异**:
  - **[v1.30+]**: VPA 支持 containerResourcePolicy 更精确地控制每个容器

**Step D2.4**: 检查 Cluster Autoscaler 状态
- **命令**:
  ```bash
  # 检查 CA 部署状态
  kubectl get deploy -n kube-system cluster-autoscaler 2>/dev/null || \
  kubectl get deploy -A -l app.kubernetes.io/name=cluster-autoscaler 2>/dev/null
  
  # 检查 CA 状态 ConfigMap（包含详细状态信息）
  kubectl get cm -n kube-system cluster-autoscaler-status -o yaml 2>/dev/null
  
  # 检查 CA 节点池配置（ACK 示例）
  kubectl get nodepool -A 2>/dev/null || echo "NodePool CRD not found (may not be using managed nodepool)"
  ```
- **超时**: 15s
- **预期输出模式**: CA 部署状态和状态 ConfigMap 内容
- **判断规则**:
  - CA 未部署 → 集群不支持自动节点伸缩（可能是手动管理节点）
  - status ConfigMap 显示 `ScaleUpTimestamp` 很久以前 → CA 长期未扩容
  - status 显示 `Health: Unhealthy` → CA 自身运行异常
  - status 显示 `ScaleDown: NoCandidates` → 所有节点都有不可驱逐的 Pod
- **版本差异**: 无

**Step D2.5**: 检查 Cluster Autoscaler 日志
- **命令**:
  ```bash
  # 获取 CA 最近日志
  kubectl logs -n kube-system deploy/cluster-autoscaler --tail=200 | grep -iE "scale|error|fail|cannot|node"
  
  # 检查扩容失败原因
  kubectl logs -n kube-system deploy/cluster-autoscaler --tail=500 | grep -A5 "could not scale up"
  
  # 检查缩容阻止原因
  kubectl logs -n kube-system deploy/cluster-autoscaler --tail=500 | grep -A5 "cannot be removed"
  ```
- **超时**: 20s
- **预期输出模式**: 扩缩容日志和错误信息
- **判断规则**:
  - 日志包含 `failed to create node` → 云厂商 API 调用失败（RC-012）
  - 日志包含 `max node group size reached` → 节点池已达最大值（RC-004）
  - 日志包含 `Quota exceeded` → 云账号配额不足（RC-004）
  - 日志包含 `could not scale up: no node group can be scaled up` → 无可用节点池或配置问题
  - 日志包含 `Pod is blocking scale down` → 查看具体 Pod（RC-008）
- **版本差异**: 无

**Step D2.6**: 检查节点池配置（云厂商相关）
- **命令**:
  ```bash
  # ACK（阿里云）示例
  aliyun cs DescribeClusterNodePools --ClusterId <cluster-id> 2>/dev/null | jq '.nodepools[] | {name: .nodepool_info.name, min: .scaling_group.scaling_group_min_size, max: .scaling_group.scaling_group_max_size, current: .total_nodes}'
  
  # 或使用 kubectl 查看节点池 CRD
  kubectl get nodepool -A -o custom-columns=NAME:.metadata.name,MIN:.spec.scaling.minSize,MAX:.spec.scaling.maxSize,DESIRED:.status.desiredSize,READY:.status.readySize 2>/dev/null
  
  # EKS（AWS）示例
  # aws eks describe-nodegroup --cluster-name <cluster> --nodegroup-name <nodegroup> | jq '.nodegroup.scalingConfig'
  
  # GKE 示例
  # gcloud container node-pools describe <pool-name> --cluster=<cluster> --format='json(autoscaling)'
  ```
- **超时**: 20s
- **预期输出模式**: 节点池最小/最大/当前节点数
- **判断规则**:
  - 当前节点数 = 最大值 → 无法继续扩容（RC-004）
  - 当前节点数 = 最小值 且有 Pending Pod → 扩容可能被阻止（检查 CA 日志）
  - min = max → 自动伸缩被禁用
  - 多个节点池配置不同 AZ → 检查 Pod 的 zone 亲和性
- **版本差异**: 云厂商 CLI 版本差异，K8s 版本无关

**Step D2.7**: 分析不可调度 Pod
- **命令**:
  ```bash
  # 获取 Pending Pod 列表和原因
  kubectl get pods -A --field-selector=status.phase=Pending -o wide
  
  # 查看 Pending Pod 的调度失败原因
  kubectl describe pod <pending-pod> -n <namespace> | grep -A10 "Events:"
  
  # 检查 Pod 的资源需求
  kubectl get pod <pending-pod> -n <namespace> -o jsonpath='{.spec.containers[*].resources}' | jq .
  ```
- **超时**: 15s
- **预期输出模式**: Pending Pod 列表和调度失败事件
- **判断规则**:
  - Events 包含 `Insufficient cpu/memory` → 需要 CA 扩容新节点
  - Events 包含 `node(s) didn't match node selector` → Pod 有特定 nodeSelector，新节点可能不满足
  - Events 包含 `node(s) had taint` → Pod 需要特定 tolerations
  - Events 包含 `Insufficient nvidia.com/gpu` → GPU 资源不足，检查 GPU 节点池
- **版本差异**: 无

**Step D2.8**: 分析缩容阻止原因
- **命令**:
  ```bash
  # 检查节点上的 Pod 是否阻止缩容
  NODE_NAME="<node-to-check>"
  kubectl get pods --field-selector spec.nodeName=$NODE_NAME -A -o jsonpath='{range .items[*]}{.metadata.namespace}/{.metadata.name} controller={.metadata.ownerReferences[0].kind}{"\n"}{end}'
  
  # 检查节点是否有缩容阻止 annotation
  kubectl get node $NODE_NAME -o jsonpath='{.metadata.annotations}' | grep -i scale
  
  # 检查节点上是否有 local storage Pod
  kubectl get pods --field-selector spec.nodeName=$NODE_NAME -A -o jsonpath='{range .items[*]}{.metadata.name}: {.spec.volumes[*].emptyDir}{"\n"}{end}' | grep -v "^:"
  
  # 检查 PDB 状态
  kubectl get pdb -A -o wide
  ```
- **超时**: 15s
- **预期输出模式**: 节点上 Pod 列表和 PDB 状态
- **判断规则**:
  - 节点上有非 Deployment/ReplicaSet/StatefulSet 管理的 Pod → 阻止缩容
  - 节点有 `cluster-autoscaler.kubernetes.io/scale-down-disabled: true` annotation → 手动禁用缩容
  - 存在 local storage (emptyDir 非 memory 类型) → 默认阻止缩容（RC-008）
  - PDB 的 `ALLOWED DISRUPTIONS` 为 0 → PDB 阻止驱逐
- **版本差异**: 无

---

### Phase 3: KEDA 与高级诊断（只读，零风险）

> **目标**: 诊断 KEDA 事件驱动伸缩和高级伸缩策略问题。
> **预计耗时**: 3-5 分钟

**Step D3.1**: 检查 KEDA Operator 状态
- **命令**:
  ```bash
  # 检查 KEDA 组件部署状态
  kubectl get deploy -n keda
  
  # 检查 KEDA Pod 状态
  kubectl get pods -n keda
  
  # 检查 KEDA Operator 日志
  kubectl logs -n keda deploy/keda-operator --tail=100
  ```
- **超时**: 15s
- **预期输出模式**: KEDA 组件运行正常
- **判断规则**:
  - KEDA namespace 不存在 → KEDA 未安装
  - keda-operator Pod 异常 → KEDA 基础设施问题（RC-009）
  - keda-metrics-apiserver Pod 异常 → 外部指标 API 不可用
  - 日志包含 `error` 或 `failed to` → 需要进一步分析
- **版本差异**: 无

**Step D3.2**: 检查 ScaledObject 状态
- **命令**:
  ```bash
  # 获取所有 ScaledObject 状态
  kubectl get scaledobject -A -o wide
  
  # 查看特定 ScaledObject 详情
  kubectl describe scaledobject <name> -n <namespace>
  
  # 检查 ScaledObject 创建的 HPA
  kubectl get hpa -n <namespace> -l scaledobject.keda.sh/name=<scaledobject-name>
  ```
- **超时**: 10s
- **预期输出模式**: ScaledObject READY=True 且关联的 HPA 正常
- **判断规则**:
  - READY=False → ScaledObject 配置或触发器问题（RC-009）
  - ACTIVE=False → 当前指标值低于激活阈值，属于预期行为
  - 关联的 HPA targets 显示 unknown → 触发器无法获取指标
  - Events 包含错误 → 分析具体错误类型
- **版本差异**: 无

**Step D3.3**: 检查 TriggerAuthentication 配置
- **命令**:
  ```bash
  # 获取 TriggerAuthentication
  kubectl get triggerauthentication -n <namespace>
  kubectl get clustertriggerauthentication
  
  # 查看详情（注意：不要输出 secret 内容）
  kubectl describe triggerauthentication <name> -n <namespace>
  
  # 检查引用的 Secret 是否存在
  kubectl get secret -n <namespace> | grep -E "keda|kafka|rabbitmq|prometheus"
  ```
- **超时**: 10s
- **预期输出模式**: 认证配置存在且引用的 Secret 可用
- **判断规则**:
  - TriggerAuthentication 不存在但 ScaledObject 引用了它 → 配置错误
  - 引用的 Secret 不存在 → 认证将失败（RC-009）
  - Secret 存在但内容可能过期（如 token）→ 需要检查有效性
- **版本差异**: 无

**Step D3.4**: 检查外部指标源连通性
- **命令**:
  ```bash
  # 检查 Prometheus 连通性（如果使用 Prometheus 触发器）
  kubectl run -it --rm debug --image=curlimages/curl --restart=Never -- curl -s "http://prometheus-server.monitoring.svc:9090/api/v1/query?query=up" 2>/dev/null | head -5
  
  # 检查 Kafka 连通性（如果使用 Kafka 触发器）
  # 需要在集群内部 Pod 中测试，或检查 KEDA operator 日志中的连接错误
  
  # 检查 ScaledObject 的 Prometheus 查询
  kubectl get scaledobject <name> -n <namespace> -o jsonpath='{.spec.triggers[*].metadata}' | jq .
  ```
- **超时**: 30s
- **预期输出模式**: 外部服务响应正常
- **判断规则**:
  - 连接超时或拒绝 → 网络连通性问题或服务不可用
  - 认证失败 → TriggerAuthentication 配置问题
  - Prometheus 查询语法错误 → ScaledObject 配置错误（RC-009）
  - Kafka 消费组 lag 无法获取 → Kafka 连接或权限问题
- **版本差异**: 无

**Step D3.5**: 检查 HPA behavior 策略配置
- **命令**:
  ```bash
  # 检查 HPA 的 behavior 配置（伸缩策略）
  kubectl get hpa <hpa-name> -n <namespace> -o yaml | grep -A30 "behavior:"
  
  # 或使用 jsonpath
  kubectl get hpa <hpa-name> -n <namespace> -o jsonpath='{.spec.behavior}' | jq .
  ```
- **超时**: 5s
- **预期输出模式**: scaleUp 和 scaleDown 策略配置
- **判断规则**:
  - `scaleUp.stabilizationWindowSeconds` 很大（如 300s）→ 扩容会有明显延迟（RC-007）
  - `scaleDown.stabilizationWindowSeconds` 很大 → 缩容会很慢
  - `scaleUp.policies` 限制过严（如每分钟只能加 1 个 Pod）→ 扩容速度不足
  - behavior 未配置 → 使用默认值（scaleDown 默认 300s stabilization）
- **版本差异**:
  - **[v1.28+]**: HPA v2 API 的 behavior 字段完全支持
  - **[v1.30+]**: 支持更精细的 HPAScaleToZero 特性（alpha）

---

## 5. 根因分类

| 根因 ID | 描述 | 概率 | 诊断证据 | 风险等级 |
|--------|------|------|---------|---------|
| RC-001 | **Metrics Server 不可用/异常** — Metrics Server 未部署、Pod 异常或 API 不可达，导致所有基于 CPU/Memory 的 HPA 无法获取指标 | ~20% | D1.3 Metrics Server Pod 异常；D1.1 所有 HPA targets 显示 unknown | 🟡 |
| RC-002 | **Pod 未设置 resources.requests** — 目标 Pod 的容器未配置 CPU/Memory requests，HPA 无法计算利用率百分比 | ~15% | D1.4 resources.requests 为空；D1.2 Events 包含 FailedGetResourceMetric | 🟢 |
| RC-003 | **HPA 目标指标配置错误** — HPA 配置的指标名称错误、类型不匹配或 selector 不正确 | ~12% | D1.2 ScalingActive=False；D1.5 自定义指标 API 查询失败 | 🟢 |
| RC-004 | **Cluster Autoscaler 节点池配额耗尽** — 节点池已达最大节点数或云账号资源配额不足 | ~10% | D2.5 日志包含 "max node group size reached" 或 "Quota exceeded"；D2.6 当前=最大 | 🟡 |
| RC-005 | **VPA 与 HPA 冲突** — 同一 Deployment 同时配置了 VPA（Auto 模式）和 HPA（基于 CPU/Memory），导致行为冲突 | ~8% | D2.3 检测到冲突配置 | 🟡 |
| RC-006 | **自定义指标 Adapter 异常** — Prometheus Adapter 或其他自定义指标提供者未部署或配置错误 | ~7% | D1.5 custom.metrics.k8s.io API 返回 404 或错误 | 🟡 |
| RC-007 | **stabilizationWindowSeconds 配置导致伸缩延迟** — HPA behavior 中的稳定窗口配置过大，导致扩缩容响应慢 | ~6% | D3.5 stabilizationWindowSeconds 值很大（>180s） | 🟢 |
| RC-008 | **CA 缩容被 PDB/annotation/local storage 阻止** — 节点上存在阻止驱逐的因素，CA 无法缩容 | ~5% | D2.8 节点有阻止 annotation 或 PDB disruptions 为 0；D2.5 日志包含 "cannot be removed" | 🟢 |
| RC-009 | **KEDA 触发器认证失败或外部指标源不可达** — ScaledObject 引用的 TriggerAuthentication 配置错误或外部服务不可用 | ~5% | D3.2 ScaledObject READY=False；D3.3 Secret 不存在；D3.4 连接失败 | 🟡 |
| RC-010 | **资源碎片化导致无法调度** — 集群有足够总资源但碎片化分布，单个节点无法满足 Pod 需求 | ~4% | D2.7 Pending Pod 原因显示 Insufficient；CA 日志显示有资源但无法调度 | 🟡 |
| RC-011 | **HPA 指标采样间隔不匹配** — HPA 的 --horizontal-pod-autoscaler-sync-period 与指标更新频率不匹配 | ~4% | HPA 反应迟钝；Metrics 数据新但 HPA 未及时响应 | 🟢 |
| RC-012 | **CA 与云厂商 API 通信失败** — Cluster Autoscaler 无法调用云厂商 API 创建/删除节点（认证失败、网络问题、API 限流） | ~4% | D2.5 日志包含 "failed to create node" 或 "API error"；云厂商控制台显示 API 错误 | 🔴 |

---

## 6. 修复操作

### 6.1 🟢 低风险（Agent 可建议自动执行）

#### REM-001: 添加 Pod resources.requests
- **适用根因**: RC-002
- **前置检查**:
  ```bash
  # 确认目标 Deployment 的 Pod 确实缺少 resources.requests
  kubectl get deploy <deployment-name> -n <namespace> -o jsonpath='{.spec.template.spec.containers[*].resources}' | jq .
  # 预期: requests 字段为空或缺失
  ```
- **执行命令**:
  ```bash
  # 使用 kubectl patch 添加 resources.requests
  kubectl patch deploy <deployment-name> -n <namespace> --type=json -p='[
    {
      "op": "add",
      "path": "/spec/template/spec/containers/0/resources",
      "value": {
        "requests": {
          "cpu": "100m",
          "memory": "128Mi"
        },
        "limits": {
          "cpu": "500m",
          "memory": "512Mi"
        }
      }
    }
  ]'
  
  # 注意: 上述值为示例，应根据实际应用负载调整
  ```
- **后置验证**:
  ```bash
  # 等待 Pod 重建
  kubectl rollout status deploy/<deployment-name> -n <namespace> --timeout=120s
  
  # 验证 HPA 可以获取指标
  sleep 30  # 等待指标采集
  kubectl get hpa -n <namespace>
  # 预期: TARGETS 列显示实际百分比而非 unknown
  ```
- **回滚命令**:
  ```bash
  kubectl rollout undo deploy/<deployment-name> -n <namespace>
  ```

#### REM-002: 修正 HPA 指标配置
- **适用根因**: RC-003
- **前置检查**:
  ```bash
  # 检查当前 HPA 配置
  kubectl get hpa <hpa-name> -n <namespace> -o yaml > /tmp/hpa-backup.yaml
  cat /tmp/hpa-backup.yaml
  ```
- **执行命令**:
  ```bash
  # 示例: 修正指标类型从 Value 到 AverageValue（根据实际情况调整）
  kubectl patch hpa <hpa-name> -n <namespace> --type=json -p='[
    {
      "op": "replace",
      "path": "/spec/metrics/0/resource/target/type",
      "value": "Utilization"
    },
    {
      "op": "replace",
      "path": "/spec/metrics/0/resource/target/averageUtilization",
      "value": 70
    }
  ]'
  
  # 或者使用 kubectl apply 应用修正后的配置
  kubectl apply -f /tmp/hpa-corrected.yaml
  ```
- **后置验证**:
  ```bash
  kubectl describe hpa <hpa-name> -n <namespace>
  # 预期: Conditions 中 ScalingActive=True, AbleToScale=True
  ```
- **回滚命令**:
  ```bash
  kubectl apply -f /tmp/hpa-backup.yaml
  ```

#### REM-003: 调整 stabilizationWindowSeconds
- **适用根因**: RC-007
- **前置检查**:
  ```bash
  # 查看当前 behavior 配置
  kubectl get hpa <hpa-name> -n <namespace> -o jsonpath='{.spec.behavior}' | jq .
  ```
- **执行命令**:
  ```bash
  # 调整 scaleUp 的 stabilizationWindowSeconds 以加快扩容响应
  kubectl patch hpa <hpa-name> -n <namespace> --type=merge -p='{
    "spec": {
      "behavior": {
        "scaleUp": {
          "stabilizationWindowSeconds": 0,
          "policies": [
            {
              "type": "Percent",
              "value": 100,
              "periodSeconds": 15
            },
            {
              "type": "Pods",
              "value": 4,
              "periodSeconds": 15
            }
          ],
          "selectPolicy": "Max"
        },
        "scaleDown": {
          "stabilizationWindowSeconds": 300,
          "policies": [
            {
              "type": "Percent",
              "value": 10,
              "periodSeconds": 60
            }
          ]
        }
      }
    }
  }'
  ```
- **后置验证**:
  ```bash
  kubectl get hpa <hpa-name> -n <namespace> -o jsonpath='{.spec.behavior}' | jq .
  # 预期: stabilizationWindowSeconds 已更新
  
  # 触发负载测试验证扩容速度
  kubectl run load-test --rm -i --image=busybox --restart=Never -- /bin/sh -c "while true; do wget -q -O- http://<service-name>.<namespace>.svc; done" &
  sleep 60
  kubectl get hpa <hpa-name> -n <namespace> -w
  ```
- **回滚命令**:
  ```bash
  # 恢复默认 behavior
  kubectl patch hpa <hpa-name> -n <namespace> --type=json -p='[{"op": "remove", "path": "/spec/behavior"}]'
  ```

#### REM-004: 修复 HPA behavior 策略
- **适用根因**: RC-007, RC-003
- **前置检查**:
  ```bash
  kubectl get hpa <hpa-name> -n <namespace> -o yaml > /tmp/hpa-behavior-backup.yaml
  ```
- **执行命令**:
  ```bash
  # 配置合理的扩缩容策略
  kubectl apply -f - <<EOF
  apiVersion: autoscaling/v2
  kind: HorizontalPodAutoscaler
  metadata:
    name: <hpa-name>
    namespace: <namespace>
  spec:
    scaleTargetRef:
      apiVersion: apps/v1
      kind: Deployment
      name: <deployment-name>
    minReplicas: 2
    maxReplicas: 10
    metrics:
    - type: Resource
      resource:
        name: cpu
        target:
          type: Utilization
          averageUtilization: 70
    behavior:
      scaleUp:
        stabilizationWindowSeconds: 0
        policies:
        - type: Percent
          value: 100
          periodSeconds: 15
        - type: Pods
          value: 4
          periodSeconds: 15
        selectPolicy: Max
      scaleDown:
        stabilizationWindowSeconds: 300
        policies:
        - type: Percent
          value: 10
          periodSeconds: 60
        selectPolicy: Min
  EOF
  ```
- **后置验证**:
  ```bash
  kubectl describe hpa <hpa-name> -n <namespace>
  # 预期: behavior 已更新
  ```
- **回滚命令**:
  ```bash
  kubectl apply -f /tmp/hpa-behavior-backup.yaml
  ```

---

### 6.2 🟡 中风险（Agent 建议，人工审批后执行）

#### REM-005: 部署/修复 Metrics Server
- **适用根因**: RC-001
- **影响说明**: Metrics Server 是集群基础组件，其部署或修改会影响所有依赖指标的功能（HPA、kubectl top）。但 Metrics Server 是无状态的，重启/重建不会丢失数据。
- **审批提示**: "建议部署/修复 Metrics Server。该操作会在 kube-system 命名空间创建/更新 Deployment，不会影响现有工作负载，但所有 HPA 可能在服务恢复期间（约 30s）无法获取指标。是否批准？"
- **前置检查**:
  ```bash
  # 检查当前 Metrics Server 状态
  kubectl get deploy -n kube-system metrics-server
  kubectl get pods -n kube-system -l k8s-app=metrics-server
  kubectl logs -n kube-system -l k8s-app=metrics-server --tail=20
  ```
- **执行命令**:
  ```bash
  # 如果 Metrics Server 未安装，使用官方 manifest 安装
  kubectl apply -f https://github.com/kubernetes-sigs/metrics-server/releases/latest/download/components.yaml
  
  # 如果是证书问题（自签名证书集群），需要添加 --kubelet-insecure-tls 参数
  kubectl patch deploy metrics-server -n kube-system --type=json -p='[
    {
      "op": "add",
      "path": "/spec/template/spec/containers/0/args/-",
      "value": "--kubelet-insecure-tls"
    }
  ]'
  
  # 如果 Pod 已存在但异常，尝试重启
  kubectl rollout restart deploy/metrics-server -n kube-system
  ```
- **后置验证**:
  ```bash
  # 等待 Metrics Server 就绪
  kubectl rollout status deploy/metrics-server -n kube-system --timeout=120s
  
  # 验证 API 可用
  kubectl get --raw /apis/metrics.k8s.io/v1beta1/nodes
  kubectl top nodes
  
  # 验证 HPA 可以获取指标
  sleep 30
  kubectl get hpa -A
  # 预期: TARGETS 列不再显示 unknown
  ```
- **回滚命令**:
  ```bash
  # 如果新部署出问题，删除 Metrics Server
  kubectl delete -f https://github.com/kubernetes-sigs/metrics-server/releases/latest/download/components.yaml
  
  # 或回滚 patch
  kubectl rollout undo deploy/metrics-server -n kube-system
  ```

#### REM-006: 修复自定义指标 Adapter
- **适用根因**: RC-006
- **影响说明**: 自定义指标 Adapter（如 Prometheus Adapter）的修复可能需要更新配置或重启服务，期间依赖自定义指标的 HPA 将无法获取指标。
- **审批提示**: "建议修复自定义指标 Adapter。修复期间依赖自定义指标的 HPA 可能暂时无法工作。是否批准？"
- **前置检查**:
  ```bash
  # 检查 Prometheus Adapter 部署
  kubectl get deploy -n custom-metrics prometheus-adapter 2>/dev/null || \
  kubectl get deploy -A -l app.kubernetes.io/name=prometheus-adapter
  
  # 检查配置
  kubectl get cm -n custom-metrics adapter-config -o yaml 2>/dev/null
  ```
- **执行命令**:
  ```bash
  # 检查并修复 Prometheus Adapter 配置
  # 示例: 确保配置中包含需要的指标规则
  
  # 如果 Adapter Pod 异常，重启
  kubectl rollout restart deploy/prometheus-adapter -n custom-metrics
  
  # 或使用 Helm 更新配置
  # helm upgrade prometheus-adapter prometheus-community/prometheus-adapter -n custom-metrics -f values.yaml
  ```
- **后置验证**:
  ```bash
  # 验证 API 可用
  kubectl get --raw /apis/custom.metrics.k8s.io/v1beta1/
  
  # 验证特定指标可查询
  kubectl get --raw "/apis/custom.metrics.k8s.io/v1beta1/namespaces/<namespace>/pods/*/http_requests_total"
  ```
- **回滚命令**:
  ```bash
  kubectl rollout undo deploy/prometheus-adapter -n custom-metrics
  ```

#### REM-007: 解除 CA 缩容阻止
- **适用根因**: RC-008
- **影响说明**: 移除缩容阻止条件可能导致节点被缩容，节点上的 Pod 将被驱逐到其他节点。需确保其他节点有足够资源。
- **审批提示**: "建议移除节点 `<node-name>` 的缩容阻止 annotation，或调整 PDB 配置。移除后该节点可能被 CA 缩容，节点上的 Pod 将被迁移。是否批准？"
- **前置检查**:
  ```bash
  # 检查阻止原因
  kubectl get node <node-name> -o jsonpath='{.metadata.annotations}' | grep -i scale
  kubectl get pdb -A -o wide
  
  # 检查其他节点是否有足够资源
  kubectl top nodes
  ```
- **执行命令**:
  ```bash
  # 移除 scale-down-disabled annotation
  kubectl annotate node <node-name> cluster-autoscaler.kubernetes.io/scale-down-disabled-
  
  # 如果是 PDB 阻止，考虑临时调整 PDB（谨慎操作）
  # kubectl patch pdb <pdb-name> -n <namespace> --type=merge -p='{"spec":{"minAvailable":1}}'
  
  # 如果是 local storage 阻止且数据可丢弃
  # 需要配置 CA 参数 --skip-nodes-with-local-storage=false（需重启 CA）
  ```
- **后置验证**:
  ```bash
  # 检查 annotation 已移除
  kubectl get node <node-name> -o jsonpath='{.metadata.annotations}' | grep -i scale
  
  # 观察 CA 日志，确认节点可以被缩容
  kubectl logs -n kube-system deploy/cluster-autoscaler --tail=100 | grep <node-name>
  ```
- **回滚命令**:
  ```bash
  # 重新添加 annotation 阻止缩容
  kubectl annotate node <node-name> cluster-autoscaler.kubernetes.io/scale-down-disabled=true
  ```

#### REM-008: 修复 KEDA ScaledObject 配置
- **适用根因**: RC-009
- **影响说明**: 修改 ScaledObject 配置可能导致关联的 HPA 重建，期间伸缩可能暂时不工作。
- **审批提示**: "建议修复 KEDA ScaledObject `<scaledobject-name>` 配置。修复期间伸缩功能可能暂时中断。是否批准？"
- **前置检查**:
  ```bash
  kubectl get scaledobject <name> -n <namespace> -o yaml > /tmp/scaledobject-backup.yaml
  kubectl describe scaledobject <name> -n <namespace>
  ```
- **执行命令**:
  ```bash
  # 示例: 修复 Prometheus 触发器配置
  kubectl apply -f - <<EOF
  apiVersion: keda.sh/v1alpha1
  kind: ScaledObject
  metadata:
    name: <scaledobject-name>
    namespace: <namespace>
  spec:
    scaleTargetRef:
      name: <deployment-name>
    minReplicaCount: 1
    maxReplicaCount: 10
    cooldownPeriod: 30
    triggers:
    - type: prometheus
      metadata:
        serverAddress: http://prometheus-server.monitoring.svc:9090
        metricName: http_requests_total
        threshold: "100"
        query: sum(rate(http_requests_total{service="<service>"}[2m]))
  EOF
  
  # 如果需要更新 TriggerAuthentication
  # kubectl apply -f triggerauthentication.yaml
  ```
- **后置验证**:
  ```bash
  kubectl get scaledobject <name> -n <namespace>
  # 预期: READY=True
  
  kubectl get hpa -n <namespace> -l scaledobject.keda.sh/name=<scaledobject-name>
  # 预期: HPA 已创建且 targets 正常
  ```
- **回滚命令**:
  ```bash
  kubectl apply -f /tmp/scaledobject-backup.yaml
  ```

---

### 6.3 🔴 高风险（Agent 仅提供指导，人工执行）

#### REM-009: CA 节点池配额扩容（云厂商操作）
- **适用根因**: RC-004, RC-012
- **影响说明**: 需要在云厂商控制台或通过 CLI 调整节点池配额或账号资源限制。这是基础设施层面的变更，可能影响成本和其他资源的可用配额。
- **操作步骤**:
  1. **确认配额限制类型**:
     ```bash
     # 检查 CA 日志确认具体限制
     kubectl logs -n kube-system deploy/cluster-autoscaler --tail=200 | grep -iE "quota|limit|max"
     ```
  2. **调整节点池最大值**（以 ACK 为例）:
     ```bash
     # 使用阿里云 CLI
     aliyun cs ModifyClusterNodePool \
       --ClusterId <cluster-id> \
       --NodepoolId <nodepool-id> \
       --ScalingGroup '{"MaxSize": 50}'  # 根据需要调整
     
     # 或通过阿里云控制台操作
     ```
  3. **申请账号配额提升**（如果是账号级配额）:
     ```bash
     # 阿里云: 通过配额中心申请
     # AWS: 通过 Service Quotas 申请
     # GCP: 通过配额页面申请
     ```
  4. **验证配额已调整**:
     ```bash
     kubectl logs -n kube-system deploy/cluster-autoscaler --tail=100 | grep -i scale
     # 预期: 不再有 quota 相关错误
     ```
- **安全检查**:
  - 评估扩容带来的成本影响
  - 确认新的配额限制合理
  - 设置成本告警
- **回滚方案**:
  ```bash
  # 降低节点池最大值
  aliyun cs ModifyClusterNodePool \
    --ClusterId <cluster-id> \
    --NodepoolId <nodepool-id> \
    --ScalingGroup '{"MaxSize": <original-max>}'
  ```

#### REM-010: VPA + HPA 联合策略重构
- **适用根因**: RC-005
- **影响说明**: 重构 VPA 和 HPA 的配置需要仔细规划，可能需要临时禁用一方。错误配置可能导致 Pod 频繁重启或伸缩异常。
- **操作步骤**:
  1. **评估当前冲突情况**:
     ```bash
     # 确认冲突的资源和指标
     kubectl get vpa <vpa-name> -n <namespace> -o yaml
     kubectl get hpa <hpa-name> -n <namespace> -o yaml
     ```
  2. **选择解决方案**:
     - **方案 A**: VPA 仅控制 Memory，HPA 控制 CPU
       ```yaml
       # VPA 配置
       spec:
         resourcePolicy:
           containerPolicies:
           - containerName: "*"
             controlledResources: ["memory"]  # 仅控制 memory
       ```
     - **方案 B**: 使用 KEDA + VPA（KEDA 不直接与 VPA 冲突）
     - **方案 C**: 禁用 VPA Auto 模式，仅使用推荐值
       ```bash
       kubectl patch vpa <vpa-name> -n <namespace> --type=merge -p='{"spec":{"updatePolicy":{"updateMode":"Off"}}}'
       ```
  3. **应用新配置**:
     ```bash
     # 先删除冲突的配置，再应用新配置
     kubectl delete hpa <hpa-name> -n <namespace>  # 如果要改为 KEDA
     kubectl apply -f new-scaling-config.yaml
     ```
  4. **验证新策略**:
     ```bash
     kubectl get hpa,vpa -n <namespace>
     # 观察一段时间确认无冲突
     ```
- **安全检查**:
  - 在非生产环境先测试新配置
  - 保留原始配置备份
  - 逐步切换，观察行为
- **回滚方案**:
  ```bash
  kubectl apply -f /tmp/original-hpa.yaml
  kubectl apply -f /tmp/original-vpa.yaml
  ```

---

### 6.4 ⚫ 严重（需高级 SRE 审批）

#### REM-011: Cluster Autoscaler 替换/重建
- **适用根因**: RC-012, RC-004
- **审批要求**: 需要高级 SRE + 集群管理员审批
- **数据备份**: 备份 CA 配置和状态 ConfigMap
- **操作步骤**:
  1. **备份当前配置**:
     ```bash
     kubectl get deploy -n kube-system cluster-autoscaler -o yaml > /tmp/ca-deploy-backup.yaml
     kubectl get cm -n kube-system cluster-autoscaler-status -o yaml > /tmp/ca-status-backup.yaml
     ```
  2. **检查云厂商认证配置**:
     ```bash
     # 确认 ServiceAccount、IAM Role 或 Secret 配置正确
     kubectl get sa -n kube-system cluster-autoscaler -o yaml
     kubectl get secret -n kube-system | grep autoscaler
     ```
  3. **更新或重建 CA**:
     ```bash
     # 使用最新版本的 CA
     # 参考云厂商文档获取最新 manifest
     
     # ACK: 通过控制台启用/更新节点自动伸缩
     # EKS: 更新 EKS add-on 或重新部署
     # GKE: GKE 内置 CA，通过控制台管理
     
     # 手动部署的 CA 重建
     kubectl delete deploy -n kube-system cluster-autoscaler
     kubectl apply -f new-cluster-autoscaler.yaml
     ```
  4. **验证 CA 运行正常**:
     ```bash
     kubectl get deploy -n kube-system cluster-autoscaler
     kubectl logs -n kube-system deploy/cluster-autoscaler --tail=100
     # 预期: 无认证错误，能正常检测和处理 Pending Pod
     ```
- **回滚方案**:
  ```bash
  kubectl apply -f /tmp/ca-deploy-backup.yaml
  ```

---

## 7. 验证确认

### 7.1 即时验证（修复后 1-2 分钟内）

```bash
# V1: 确认 HPA 可以获取指标
kubectl get hpa -A
# 预期: TARGETS 列显示实际百分比，不再是 <unknown>

# V2: 确认 HPA Conditions 正常
kubectl get hpa <hpa-name> -n <namespace> -o jsonpath='{range .status.conditions[*]}{.type}={.status}{"\n"}{end}'
# 预期: ScalingActive=True, AbleToScale=True

# V3: 确认 Metrics Server API 可用
kubectl get --raw /apis/metrics.k8s.io/v1beta1/nodes | jq '.items | length'
# 预期: 返回节点数量

# V4: 确认 VPA 推荐值正常（如适用）
kubectl get vpa -A -o custom-columns=NAME:.metadata.name,NAMESPACE:.metadata.namespace,CPU:.status.recommendation.containerRecommendations[0].target.cpu,MEMORY:.status.recommendation.containerRecommendations[0].target.memory
# 预期: CPU 和 MEMORY 列显示合理的推荐值

# V5: 确认 CA 状态正常（如适用）
kubectl get cm -n kube-system cluster-autoscaler-status -o yaml | grep -A5 "Health:"
# 预期: Health: Healthy

# V6: 确认 KEDA ScaledObject 正常（如适用）
kubectl get scaledobject -A
# 预期: READY=True
```

### 7.2 短期监控（5-30 分钟）

| 监控项 | 命令/指标 | 预期趋势 | 异常阈值 |
|-------|----------|---------|---------|
| HPA 副本数变化 | `kube_horizontalpodautoscaler_status_current_replicas` | 根据负载平滑变化 | 30 分钟内无变化但负载明显变化 |
| HPA 目标指标 | `kube_horizontalpodautoscaler_status_target_metric` | 与实际指标一致 | 持续显示 0 或 unknown |
| Pending Pod 数量 | `kubectl get pods -A --field-selector=status.phase=Pending \| wc -l` | 快速降为 0 | 10 分钟后仍有大量 Pending |
| CA 扩容事件 | CA 日志中 "Successfully added node" | 有 Pending Pod 时触发扩容 | 扩容请求失败 |
| VPA 推荐更新 | `kubectl get vpa -o jsonpath='{.items[*].status.recommendation}'` | 推荐值定期更新 | 超过 24 小时无更新 |
| Metrics Server 响应时间 | `kubectl top nodes --v=6` (观察延迟) | < 1s | > 5s |

### 7.3 解决确认标准

以下条件**全部满足**时，可确认故障已解决：

- [ ] 所有 HPA 的 TARGETS 列显示正常的百分比值
- [ ] HPA Conditions 中 ScalingActive=True, AbleToScale=True
- [ ] 无 Pending Pod（或 Pending Pod 正在被处理中）
- [ ] Metrics Server API 响应正常 (`kubectl top nodes` 成功)
- [ ] 自定义指标 API 可用（如使用）
- [ ] CA 日志无异常错误
- [ ] VPA 推荐值合理（如使用）
- [ ] KEDA ScaledObject READY=True（如使用）
- [ ] 触发负载测试后 HPA 能正确扩缩容

### 7.4 回归检测（24 小时内关注）

| 关注项 | 检查方法 | 频率 | 异常行动 |
|-------|---------|------|---------|
| HPA targets 状态 | `kubectl get hpa -A \| grep unknown` | 每小时 | 重新进入诊断流程 |
| Metrics Server 健康 | `kubectl get --raw /apis/metrics.k8s.io/v1beta1/nodes` | 每小时 | 检查 Pod 状态和日志 |
| HPA 扩缩容行为 | Prometheus 指标 `kube_horizontalpodautoscaler_status_current_replicas` 趋势 | 持续 | 与预期负载模式对比 |
| CA 扩容成功率 | CA 日志 `Successfully added` vs `failed to` | 每 4 小时 | 检查云厂商 API 状态 |
| 资源利用率 | `kubectl top nodes` 和 `kubectl top pods` | 每 4 小时 | 确认资源使用与伸缩行为匹配 |
| 成本变化 | 云厂商账单/成本管理 | 每日 | 异常成本增长需排查 |

---

## 8. 升级协议

### 8.1 自动升级条件

| 条件 | 说明 | 触发时机 |
|------|------|---------|
| **诊断超时** | 诊断工作流执行超过 **15 分钟**未能确认根因 | Phase 2 结束后仍无明确根因 |
| **修复失败** | 同一修复操作执行 **2 次**仍未通过后置验证 | REM-xxx 执行后 V1-V6 验证失败 |
| **严重性升级** | 初始分级为 P2 但影响面扩大（如更多 HPA 失效或 Pending Pod 持续增加） | 诊断过程中异常 HPA 数量增加或 Pending Pod > 50 |
| **未知根因** | 完成 Phase 1-3 所有诊断步骤但无法匹配任何已知根因 | 所有诊断步骤均无明确异常发现 |
| **云厂商问题** | CA 与云厂商 API 通信失败且非认证问题 | D2.5 显示 API 错误但 credentials 验证正确 |
| **成本风险** | 检测到异常的节点扩容行为可能导致成本失控 | 短时间内节点数量异常增加 |

### 8.2 升级消息模板

```
【{severity}】弹性伸缩故障诊断 - {cluster_name}
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
- 故障概述: {component}（HPA/VPA/CA/KEDA）异常，{symptom_summary}
- 影响范围: 
  - 受影响 HPA/VPA: {affected_count}/{total_count}
  - Pending Pod 数量: {pending_pod_count}
  - 关键服务受影响: {affected_services}
- 已完成诊断:
  - Phase 1 HPA 检查: {phase1_summary}
  - Phase 2 VPA/CA 检查: {phase2_summary}
  - Phase 3 KEDA 检查: {phase3_summary}
- 初步发现:
  - 可能根因: {suspected_root_cause} ({root_cause_id})
  - 关键证据: {key_evidence}
- 已尝试修复:
  - {attempted_remediation} → 结果: {remediation_result}
- 需要: {action_needed}
- 成本影响评估: {cost_impact}
- 工单编号: {ticket_id}
- Skill 版本: SKILL-SCALE-001 v1.0
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
```

### 8.3 交接信息包

升级时，Agent 需准备以下完整信息供人工接手：

1. **完整诊断路径**: 按时间顺序列出已执行的每个诊断步骤及输出摘要
2. **已排除的根因**: 列出已通过诊断排除的根因及排除依据
3. **可能的根因假设**: 基于已有证据提出的根因假设及置信度
4. **关键资源快照**:
   ```bash
   # HPA 状态
   kubectl get hpa -A -o wide > hpa-status.txt
   kubectl describe hpa -A > hpa-describe.txt
   
   # VPA 状态（如使用）
   kubectl get vpa -A -o yaml > vpa-status.yaml
   
   # CA 状态和日志
   kubectl get cm -n kube-system cluster-autoscaler-status -o yaml > ca-status.yaml
   kubectl logs -n kube-system deploy/cluster-autoscaler --tail=500 > ca-logs.txt
   
   # Metrics Server 状态
   kubectl get deploy -n kube-system metrics-server -o yaml > metrics-server-deploy.yaml
   kubectl logs -n kube-system -l k8s-app=metrics-server --tail=200 > metrics-server-logs.txt
   
   # Pending Pods
   kubectl get pods -A --field-selector=status.phase=Pending -o wide > pending-pods.txt
   ```
5. **事件时间线**: 最近 30 分钟内的关键事件按时间排列

---

## 9. K8s 版本兼容矩阵

### 9.1 功能差异表

| 功能/行为 | v1.28 | v1.29 | v1.30 | v1.31 | v1.32 |
|----------|-------|-------|-------|-------|-------|
| HPA v2 API | GA（默认） | GA | GA | GA | GA |
| HPA v1 API | 已弃用 | 已弃用 | 已弃用 | 移除计划中 | 可能移除 |
| HPA behavior 字段 | GA | GA | GA | GA | GA |
| HPA ContainerResource 指标 | beta | beta | GA | GA | GA |
| HPAScaleToZero | alpha | alpha | alpha | beta | beta |
| VPA | addon | addon | addon | addon | addon |
| Cluster Autoscaler | 1.28.x | 1.29.x | 1.30.x | 1.31.x | 1.32.x |
| KEDA | 2.12+ | 2.13+ | 2.14+ | 2.15+ | 2.16+ |
| Metrics Server | 0.6.x+ | 0.7.x | 0.7.x | 0.7.x | 0.7.x |

### 9.2 诊断命令差异

| 命令/参数 | v1.28 | v1.29 | v1.30 | v1.31 | v1.32 |
|----------|-------|-------|-------|-------|-------|
| `kubectl get hpa -o wide` | 支持 | 支持 | 支持 | 支持 | 支持 |
| `kubectl top pods/nodes` | 支持 | 支持 | 支持 | 支持 | 支持 |
| `kubectl get --raw /apis/metrics.k8s.io/v1beta1/` | 支持 | 支持 | 支持 | 支持 | 支持 |
| `kubectl get --raw /apis/custom.metrics.k8s.io/v1beta1/` | 支持 | 支持 | 支持 | 支持 | 支持 |
| `kubectl get --raw /apis/external.metrics.k8s.io/v1beta1/` | 支持 | 支持 | 支持 | 支持 | 支持 |
| HPA behavior 配置 | 完全支持 | 完全支持 | 完全支持 | 完全支持 | 完全支持 |

### 9.3 关键 API 版本

| 资源 | v1.28 | v1.29 | v1.30 | v1.31 | v1.32 |
|------|-------|-------|-------|-------|-------|
| HorizontalPodAutoscaler | autoscaling/v2 (推荐) | autoscaling/v2 | autoscaling/v2 | autoscaling/v2 | autoscaling/v2 |
| VerticalPodAutoscaler | autoscaling.k8s.io/v1 | v1 | v1 | v1 | v1 |
| PodDisruptionBudget | policy/v1 | v1 | v1 | v1 | v1 |
| ScaledObject (KEDA) | keda.sh/v1alpha1 | v1alpha1 | v1alpha1 | v1alpha1 | v1alpha1 |

### 9.4 云厂商 Cluster Autoscaler 配置差异

| 配置项 | ACK（阿里云） | EKS（AWS） | GKE |
|-------|--------------|-----------|-----|
| CA 部署方式 | 控制台/CLI 启用 | EKS Add-on 或手动部署 | 内置（自动管理） |
| 节点池 CRD | NodePool | - | - |
| 认证方式 | RAM Role (STS) | IAM Role for SA | Workload Identity |
| 扩容延迟（典型） | 1-3 分钟 | 2-5 分钟 | 1-2 分钟 |
| 配额限制位置 | 弹性伸缩组/账号配额 | EC2 Service Quotas | Compute Engine Quotas |
| 日志位置 | kube-system/cluster-autoscaler Pod | kube-system Pod 或 CloudWatch | Cloud Logging |

---

## 10. 知识进化

### 10.1 常见误诊模式

| 误诊场景 | 表面现象 | 实际根因 | 避免方法 |
|---------|---------|---------|---------|
| **将 stabilization window 延迟误判为 HPA 故障** | HPA 配置正确，负载增加但副本数不变 | behavior.scaleUp.stabilizationWindowSeconds 配置导致预期的延迟 | D3.5 检查 behavior 配置，了解 stabilization window 是防抖动特性而非故障 |
| **将资源碎片化误判为配额不足** | CA 日志显示无法扩容，集群总资源足够 | 单个节点无法满足 Pod 资源需求（如 Pod 请求 8 核，节点只剩 4 核） | D2.7 检查 Pending Pod 的具体资源需求，D2.6 检查节点池配置的实例规格 |
| **将 VPA Off 模式误判为 VPA 故障** | VPA 有推荐值但 Pod 资源未更新 | UpdateMode=Off 设计如此，仅推荐不自动应用 | D2.1 检查 VPA updateMode 配置，Off 模式是预期行为 |
| **将 HPA minReplicas 限制误判为缩容故障** | 负载很低但副本数维持在某个值不再下降 | 已达到 minReplicas 限制 | D1.1 检查 MINPODS 和 REPLICAS 的关系 |
| **将 Metrics Server 启动延迟误判为故障** | 刚部署的 HPA 显示 unknown | Metrics Server 需要 30-60s 采集初始数据 | 给新部署的组件足够的启动时间（60s）再判断是否故障 |
| **将 KEDA cooldownPeriod 误判为触发器故障** | ScaledObject READY=True 但不缩容 | cooldownPeriod 内不允许缩容 | D3.2 检查 cooldownPeriod 配置，理解这是防抖动特性 |

### 10.2 深度知识引用

需要深入了解根因机制时，参考以下资源：

| 主题 | 引用路径 | 适用场景 |
|------|---------|---------|
| HPA/VPA 原理与调优 | `domain-10-troubleshooting-diagnostics/17-hpa-vpa-troubleshooting.md` | 理解 HPA 控制循环和 VPA 推荐算法 |
| Cluster Autoscaler 深度排查 | `domain-10-troubleshooting-diagnostics/28-cluster-autoscaler-troubleshooting.md` | CA 扩缩容决策机制和云厂商集成 |
| Kubernetes 调度原理 | `domain-02-workloads-applications/` | 理解 Pod 调度、资源请求和亲和性 |
| 云厂商集成 | `domain-12-cloud-providers/` | ACK/EKS/GKE 特定配置和限制 |
| 成本优化 | `domain-14-ai-ml-infra/26-cost-optimization-overview.md` | 伸缩策略的成本影响分析 |
| KEDA 事件驱动伸缩 | KEDA 官方文档 | 各类触发器配置和故障排查 |

### 10.3 Skill 改进记录

| 日期 | 版本 | 变更 | 原因 |
|------|------|------|------|
| 2026-04 | v1.0 | 初始版本发布。覆盖 HPA/VPA/CA/KEDA 四种伸缩机制，12 个根因，11 个修复操作 | 弹性伸缩是 Kubernetes 核心能力，故障影响服务可用性和成本 |

### 10.4 待补充的知识空白

以下领域在当前版本中覆盖有限，后续版本将增强：

1. **GPU 工作负载伸缩**: 基于 GPU 利用率的 HPA 配置和 GPU 节点池 CA
2. **多集群伸缩**: Cluster Federation 场景下的跨集群伸缩
3. **Serverless 伸缩**: [[domain-19-landscape-references/01-cncf-landscape/graduated/knative/knative|Knative]] / OpenFaaS 等 Serverless 框架的伸缩机制
4. **预测性伸缩**: 基于历史数据和 ML 的预测性伸缩方案
5. **混合云伸缩**: 跨云厂商的节点池伸缩协调
6. **FinOps 集成**: 伸缩决策与成本优化的深度集成

## Related

- [[domain-19-landscape-references/topic-index/scheduler-index|Scheduler 调度与弹性伸缩知识图谱索引]]
