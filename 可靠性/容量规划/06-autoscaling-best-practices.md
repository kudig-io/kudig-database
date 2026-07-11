---
title: 自动扩缩容最佳实践
description: HPA / VPA / Cluster Autoscaler / Karpenter 协同工作的最佳实践与避坑指南
summary: 四类 Autoscaler 职责划分 + 协同配置 + 冷启动优化 + 常见冲突排查
category: reliability
tags:
- slo
- sli
- reliability
- autoscaling
- hpa
- vpa
- karpenter
- capacity
tier: core
created: '2026-07-11'
last_updated: 2026-07
difficulty: advanced
audience:
- SRE
- 架构师
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。

# 自动扩缩容最佳实践

> **核心原则**：扩缩容不是"一个 HPA 解决一切"。HPA 管副本数、VPA 管单副本资源、Cluster Autoscaler/Karpenter 管节点——**四者职责不同，必须分层协同**。让任何一个越界都会导致震荡、成本失控或扩容失败。

## 四层 Autoscaler 职责矩阵

```
┌─────────────────────────────────────────────┐
│ Karpenter / Cluster Autoscaler              │  节点层：Pod Pending → 加节点
├─────────────────────────────────────────────┤
│ HPA   (Horizontal)                          │  副本层：负载高 → 加副本
├─────────────────────────────────────────────┤
│ VPA   (Vertical)                            │  容器层：OOM/CPU → 调大 requests
├─────────────────────────────────────────────┤
│ Application (内置限流/降级)                   │  应用层：自保
└─────────────────────────────────────────────┘
```

| 扩缩容器 | 响应时间 | 适用场景 | 风险 |
|---------|---------|---------|------|
| HPA | 秒–分钟 | 流量型扩容 | 节点不够会卡 Pending |
| VPA | 分钟–小时 | 资源配比优化 | 重启 Pod（Live 模式） |
| Cluster Autoscaler | 分钟 | 节点补足 | 冷启动慢（2–5 分钟） |
| Karpenter | 秒级 | 节点补足（快） | 需 Spot 管理 |

## 1. HPA：基于自定义指标 + 多策略

```yaml
apiVersion: autoscaling/v2
kind: HorizontalPodAutoscaler
metadata: { name: api }
spec:
  scaleTargetRef: { apiVersion: apps/v1, kind: Deployment, name: api }
  minReplicas: 3
  maxReplicas: 50
  metrics:
  - type: Resource
    resource: { name: cpu, target: { type: Utilization, averageUtilization: 70 } }
  - type: Pods                      # 自定义指标（RPS）
    pods:
      metric: { name: http_requests_per_second }
      target: { type: AverageValue, averageValue: "500" }
  behavior:                         # ★ 防震荡的关键
    scaleUp:
      stabilizationWindowSeconds: 0     # 扩容立即响应
      policies:
      - { type: Percent, value: 100, periodSeconds: 30 }   # 30s 翻倍
      selectPolicy: Max
    scaleDown:
      stabilizationWindowSeconds: 300   # ★ 缩容慢，至少稳定 5 分钟才缩
      policies:
      - { type: Percent, value: 10, periodSeconds: 60 }    # 每分钟最多缩 10%
      selectPolicy: Min
```

**要点**：
- `scaleUp` 快、`scaleDown` 慢——缩容保守是防震荡的金科玉律。
- `maxReplicas` 必须有上限，否则一个指标错误能把你扩到破产。

## 2. VPA：仅推荐模式起步

🟡 **中危**：VPA `Auto` 模式会重启 Pod，生产首次启用必须用 `Off`（仅推荐）观察 2 周。

```yaml
apiVersion: autoscaling.k8s.io/v1
kind: VerticalPodAutoscaler
metadata: { name: api }
spec:
  targetRef: { apiVersion: apps/v1, kind: Deployment, name: api }
  updatePolicy: { updateMode: "Off" }   # ★ 起步用 Off，只出建议不改 Pod
  resourcePolicy:
    containerPolicies:
    - containerName: '*'
      controlledResources: ["cpu", "memory"]
      maxAllowed: { cpu: 4, memory: 8Gi }   # ★ 设上限防失控
```

⚠️ **HPA 与 VPA 冲突铁律**：不要在同一 Deployment 上同时用 HPA（基于 CPU 利用率）和 VPA（Auto 模式）——两者会互相打架。要么 HPA 用自定义指标 + VPA Off，要么 VPA 不动 CPU。

## 3. Karpenter：秒级节点供给

```yaml
# NodePool：定义"要什么样的节点"
apiVersion: karpenter.sh/v1
kind: NodePool
metadata: { name: default }
spec:
  template:
    spec:
      requirements:
      - key: kubernetes.io/arch
        operator: In
        values: ["amd64"]
      - key: karpenter.sh/capacity-type
        operator: In
        values: ["on-demand", "spot"]   # ★ Spot 省钱但需应用层容忍
      - key: node.kubernetes.io/instance-type
        operator: In
        values: ["m6i.large","m6i.xlarge","c6i.large"]
      expireAfter: 720h                  # ★ 节点 30 天强制轮换，防漂移
  limits:
    cpu: 1000
    memory: 4000Gi
  disruption:
    consolidationPolicy: WhenEmptyOrUnderutilized
    consolidateAfter: 30s
```

Karpenter vs Cluster Autoscaler：
- **Karpenter** 更快（秒级）、更省（直接选最便宜机型）、更智能（主动整合）。
- **Cluster Autoscaler** 更成熟、与各大云厂 ASG 深度集成、运维心智负担低。
- 新集群建议 Karpenter；老集群迁移前充分验证。

## 4. 冷启动优化

节点扩容要 2–5 分钟（申机器→启动→注册→调度），这期间 SLO 会破。对策：

1. **预留 buffer**：HPA `minReplicas` 设成能扛 1.5 倍日常峰值的副本数。
2. **OverProvisioning**：用低优先级的"占位 Pod"提前把节点拉起来，流量来时被高优先级 Pod 抢占：

```yaml
apiVersion: scheduling.k8s.io/v1
kind: PriorityClass
metadata: { name: overprovisioning }
value: -1
```

3. **预热连接池**：Pod 启动后 readiness probe 之前先建好 DB/缓存连接。

## 排查 Checklist

- [ ] HPA 显示 `AbleToScale=False`？→ 检查 maxReplicas / 节点资源
- [ ] Pod 卡 Pending？→ `kubectl describe pod` 看 scheduler 事件，查 CA/Karpenter 日志
- [ ] 扩缩容震荡？→ 检查 `stabilizationWindowSeconds` 与指标抖动
- [ ] VPA 不工作？→ 确认 `updateMode` 不是 Off，且没与 HPA 争 CPU

## 相关

- [[可靠性/容量规划/02-hpa-vpa-cluster-autoscaler-karpenter.md|02 hpa vpa cluster autoscaler karpenter]]
- [[可靠性/容量规划/07-resource-right-sizing-guide.md|07 resource right sizing guide]]
- [[可靠性/容量规划/01-capacity-planning-framework.md|01 capacity planning framework]]

<!-- risk-assessed -->
