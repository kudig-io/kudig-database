---
title: 容量规划与生产就绪指南
description: 面向 Kubernetes 生产环境的容量规划与生产就绪门控手册，覆盖余量规则、饱和信号、上线门控、自动扩缩容策略与成本感知规划。
summary: 面向 Kubernetes 生产环境的容量规划与生产就绪门控手册，覆盖余量规则、饱和信号、上线门控、自动扩缩容策略与成本感知规划。
category: production-operations
tags:
- production
- best-practices
- playbook
- production-operations
- capacity-planning
- autoscaling
- cost-optimization
- finops
tier: core
created: '2026-07-01'
last_updated: '2026-07'
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 平台工程师
estimated_read_time: 30min
intent_queries:
- Kubernetes 容量规划如何做
- 生产环境容量余量规则
- 饱和信号与上线门控
- K8s 自动扩缩容策略
- 成本感知容量规划
trigger_keywords:
- capacity planning
- 容量规划
- headroom
- saturation signals
- launch gate
- autoscaling
- cost-aware planning
prerequisites:
- kubectl-basics
- prometheus-basics
- hpa-basics
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

# 容量规划与生产就绪指南

本指南面向 SRE、平台工程师与运维团队，提供 Kubernetes 生产环境容量规划的运行手册。核心目标是：在业务增长、突发流量与成本控制之间建立可量化的容量门控，避免因容量不足导致的服务降级，同时防止过度预留造成的资源浪费。容量规划不是一次性的计算任务，而是贯穿服务全生命周期的持续工程实践，需要与发布管理、FinOps、可靠性工程紧密协作。

## 1. 适用场景与范围

本指南适用于以下场景：

- 新服务上线前的容量评审与生产就绪门控。
- 现有服务的周期性容量审查（周/月/季度）。
- 大促、版本发布、用户增长等场景前的容量预算与扩缩容演练。
- 覆盖计算、内存、磁盘、网络与连接数等多维资源，不替代底层基础设施容量规划。

本指南不深入讲解业务容量建模方法，也不替代云厂商的底层资源规划工具，而是聚焦 Kubernetes 层面的可执行动作与检查点。

## 2. 前置条件与工具

在开始容量规划前，请确认以下前置条件已经满足：

- 已部署 Prometheus + Grafana，具备节点与工作负载资源指标。
- 已启用 metrics-server 与 HPA/VPA/Cluster Autoscaler（或 Karpenter）。
- 已按命名空间配置 ResourceQuota 与 LimitRange。
- 已建立成本标签体系（team、env、cost-center 等）。
- 可选：Kubecost / OpenCost 用于成本可视化。

## 3. 核心概念

### 3.1 余量规则（Headroom Rules）

余量是为突发流量、故障转移与滚动升级保留的资源缓冲。推荐基线：

| 资源维度 | 峰值目标 | 说明 |
|---|---|---|
| CPU 使用率 | < 70% | 保留 30% 用于突发与 HPA 响应延迟 |
| 内存使用率 | < 80% | 避免 OOM 与内核回收压力 |
| 节点池利用率 | < 75% | 确保 cluster-autoscaler 有空间快速扩容 |
| PVC 使用率 | < 75% 告警，< 85% 紧急 | 预留扩容窗口 |
| Pod 调度余量 | 常驻 10–20% 空闲节点 | 应对滚动更新与节点故障 |

余量不是浪费，而是为以下场景购买的保险：

- 业务突发流量（如营销活动、热点事件）
- 节点故障后的重新调度
- 滚动更新期间的双倍副本占用
- 集群自动扩缩容的响应延迟

不同业务场景可以适当调整余量。例如，面向终端用户的在线服务应保持更高余量，而离线批处理任务可以适当降低。

### 3.2 饱和信号（Saturation Signals）

饱和信号是容量即将耗尽的前置指标，应纳入告警：

- CPU Throttle 次数增长
- 内存增长斜率 > 预测阈值
- Pending Pod 数量持续 > 0
- 调度失败事件增加
- 磁盘 inode / 容量使用率
- 网络带宽接近节点上限
- 连接数、线程数、文件句柄接近限制

饱和信号比使用率更接近真实风险，因为使用率平均化后往往掩盖了局部热点。例如，一个节点的平均 CPU 使用率为 50%，但某些 Pod 可能已经被 throttle，这时需要关注饱和信号而非平均值。

### 3.3 上线门控（Launch Gates）

上线前必须通过的容量检查：

1. 服务已定义 requests/limits，且 requests ≤ 实际需求的 80%。
2. HPA 已配置并验证触发逻辑。
3. 当前命名空间资源配额未超过 70%。
4. 依赖服务容量已评估，不存在下游瓶颈。
5. 已制定回滚或限流预案。

上线门控应在 GitOps 或发布平台中固化，未通过时自动阻塞发布。这样可以避免容量未就绪的服务直接进入生产环境。

## 4. 标准操作流程

### 4.1 容量现状评估

```bash
# 节点整体利用率
kubectl top nodes

# 工作负载资源使用 Top 20
kubectl top pods -A --sort-by=cpu | head -n 20
kubectl top pods -A --sort-by=memory | head -n 20

# Pending 与 Evicted Pod
kubectl get pods -A --field-selector=status.phase=Pending
kubectl get pods -A --field-selector=status.phase=Failed | grep Evicted

# 命名空间配额使用
kubectl describe resourcequota -n <namespace>

# 节点可分配资源余量
kubectl describe node <node> | grep -A 8 "Allocated resources"
```

### 4.2 容量趋势预测

基于 Prometheus 查询：

```promql
# CPU 使用率 7 天增长趋势
predict_linear(
  avg(rate(container_cpu_usage_seconds_total{namespace="prod"}[5m]))[7d:1h], 7*24*3600
)

# 内存使用趋势
predict_linear(
  avg(container_memory_working_set_bytes{namespace="prod"})[7d:1h], 7*24*3600
)

# 节点池饱和度
count(kube_node_status_condition{condition="Ready",status="true"} == 1) /
count(kube_node_info)
```

容量预测应结合业务计划（如新用户、新功能、营销活动）进行校正，不能完全依赖历史趋势外推。例如，如果下月有大促，历史趋势可能无法反映流量突增。

### 4.3 上线前容量门控

```bash
# 检查目标命名空间配额
kubectl get resourcequota -n <namespace> -o yaml

# 检查 HPA 配置
kubectl get hpa -n <namespace> -o yaml

# 模拟滚动更新期间资源占用
kubectl get deployment <app> -n <namespace> -o yaml | grep -A 5 strategy

# 确认 cluster-autoscaler 可扩容（以 AWS/EKS 为例）
kubectl logs -n kube-system deployment/cluster-autoscaler --tail=100 | grep -i "scale-up"
```

上线前应通过压测验证：

- 预期峰值下 CPU/内存使用率是否 < 70%/80%
- HPA 是否在目标阈值附近平滑触发
- 扩容延迟是否满足业务容忍度
- 回滚是否可在 5 分钟内完成

### 4.4 自动扩缩容策略

**HPA（横向 Pod 扩缩容）**：

```yaml
apiVersion: autoscaling/v2
kind: HorizontalPodAutoscaler
metadata:
  name: app-hpa
  namespace: prod
spec:
  scaleTargetRef:
    apiVersion: apps/v1
    kind: Deployment
    name: app
  minReplicas: 3
  maxReplicas: 50
  metrics:
    - type: Resource
      resource:
        name: cpu
        target:
          type: Utilization
          averageUtilization: 60
    - type: Resource
      resource:
        name: memory
        target:
          type: Utilization
          averageUtilization: 70
  behavior:
    scaleUp:
      stabilizationWindowSeconds: 60
      policies:
        - type: Percent
          value: 100
          periodSeconds: 60
    scaleDown:
      stabilizationWindowSeconds: 300
      policies:
        - type: Percent
          value: 10
          periodSeconds: 60
```

HPA 设计要点：

- target 不应设置过高，否则扩容时已经过载；也不应过低，否则频繁震荡。
- 建议同时配置 CPU 与内存指标，避免单一指标失真。
- scaleDown 的 stabilization window 应足够长，避免流量抖动导致反复扩缩。

**Cluster Autoscaler**：

```bash
helm upgrade cluster-autoscaler autoscaler/cluster-autoscaler \
  --set autoDiscovery.clusterName=<CLUSTER_NAME> \
  --set extraArgs.balance-similar-node-groups=true \
  --set extraArgs.skip-nodes-with-system-pods=false
```

**Karpenter**（按需节点池）：

```yaml
apiVersion: karpenter.sh/v1beta1
kind: NodePool
metadata:
  name: default
spec:
  template:
    spec:
      requirements:
        - key: karpenter.k8s.aws/instance-category
          operator: In
          values: ["m", "c"]
        - key: karpenter.sh/capacity-type
          operator: In
          values: ["spot", "on-demand"]
  limits:
    cpu: 1000
    memory: 4000Gi
```

### 4.5 成本感知规划

- 按 team/env/cost-center 打标签，定期出账。
- 对非核心负载使用 Spot/抢占式实例，关键负载使用 On-Demand 或 Reserved。
- 使用 VPA 推荐值优化 requests，避免过度申请。
- 对长期低利用率工作负载进行缩容或下线。

```bash
# 查看节点成本标签
kubectl get nodes -L team,env,cost-center

# 查看按命名空间统计的 Pod 资源请求
kubectl get pods -A -o custom-columns=NS:.metadata.namespace,NAME:.metadata.name,REQ_CPU:.spec.containers[*].resources.requests.cpu,REQ_MEM:.spec.containers[*].resources.requests.memory
```

成本感知规划要求 SRE 与 FinOps 团队每月联合审查资源利用率与账单，识别浪费并制定优化计划。

## 5. 关键检查点与验证命令

| 检查项 | 验证命令/配置 |
|---|---|
| 节点利用率 | `kubectl top nodes` |
| Pod 资源申请 | `kubectl get pods -A -o jsonpath='{..resources.requests}'` |
| HPA 状态 | `kubectl get hpa -A` |
| 节点池伸缩事件 | `kubectl logs -n kube-system deployment/cluster-autoscaler` |
| 配额使用 | `kubectl describe resourcequota -n <ns>` |
| 容量预测 | Prometheus `predict_linear` 查询 |
| 成本标签 | `kubectl get nodes -L team,env,cost-center` |
| 滚动更新参数 | `kubectl get deploy <app> -o yaml \| grep strategy -A 5` |

## 6. 常见故障与 Remediation

| 现象 | 可能根因 | 处置 |
|---|---|---|
| 大量 Pod Pending | 节点池资源不足 / requests 过大 / 污点不匹配 | 扩容节点池；优化 requests；检查污点与亲和性 |
| HPA 未触发但 CPU 已满 | metrics-server 异常 / target 设置过高 / Pod 未设 request | 修复 metrics-server；降低 target；补充 requests |
| 扩容速度跟不上流量 | CA 扫描周期长 / 节点启动慢 / 镜像拉取慢 | 调整 CA scan-interval；使用预置镜像；启用镜像缓存 |
| 成本突增 | Spot 实例被回收 / 资源泄漏 / HPA maxReplicas 过大 | 优化 Spot 使用策略；回收闲置资源；收紧 maxReplicas |
| 滚动更新期间服务降级 | maxUnavailable 过大 / PDB 缺失 | 配置 `maxSurge=25%, maxUnavailable=0`；补充 PDB |
| 存储容量耗尽 | PVC 未设置告警 / 数据增长超预期 | 设置 75%/85% 告警；启用 StorageClass 扩容 |
| 节点资源争抢 | 多租户未设置 ResourceQuota / LimitRange | 补充命名空间级配额与默认 limits |

## 7. 风险与注意事项

- **不要仅依赖平均值**：容量规划应以 P95/P99 峰值为准，平均值会掩盖突发流量。
- **HPA 不是万能**：HPA 有分钟级延迟，对秒级突发流量需配合缓冲区、限流或缓存。
- **Spot 实例风险**：关键控制面、数据库、有状态服务不应运行在 Spot 实例上。
- **跨 AZ 平衡**：节点池应跨可用区分布，避免单 AZ 故障导致容量断崖。
- **变更窗口**：扩容/缩容操作应避开业务高峰，并在低峰期演练。
- **容量规划≠性能优化**：容量规划解决“够不够”，性能优化解决“快不快”，两者需结合。

## 8. 相关 Runbook / 推荐阅读

- [[生产运维/99-production-readiness-operations-guide.md|生产运维域生产就绪运维指南]]
- [[可靠性/容量规划/01-capacity-planning-framework.md|容量规划框架]]
- [[可靠性/容量规划/02-hpa-vpa-cluster-autoscaler-karpenter.md|HPA/VPA/Cluster Autoscaler/Karpenter]]
- [[生产运维/成本治理/13-kubernetes-cost-governance.md|Kubernetes 成本治理]]
- [[生产运维/集群治理/14-resource-quota-management.md|资源配额管理]]
- [[可观测性/SLO-SLI/02-slo-implementation-guide.md|SLO 设定与实施指南]]

---

*容量规划是持续过程，建议每周审视关键指标、每月评估趋势、每季度进行容量预算复盘，并将结论同步到 FinOps 与发布管理团队。*
