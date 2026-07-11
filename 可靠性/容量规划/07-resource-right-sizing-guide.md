---
title: 资源配额右调优指南
description: 用 VPA + Goldilocks 实现资源 requests/limits 的数据驱动式右调优工作流
summary: VPA Off 模式采集 → Goldilocks 出建议 → GitOps 灰度落地 → 持续校准的右调优闭环
category: reliability
tags:
- slo
- sli
- reliability
- vpa
- goldilocks
- resource
- cost
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

# 资源配额右调优指南

> **核心原则**：资源右调优不是"凭感觉拍个数字"，而是**让真实负载数据说话**。`requests` 决定调度与成本，`limits` 决定稳定性——拍脑袋设的值要么浪费 30% 成本，要么在峰值 OOM。数据驱动的右调优能同时收回浪费、避免事故。

## 右调优闭环

```
部署(初始值) ──▶ VPA Off 采集(2w) ──▶ Goldilocks 建议 ──▶ GitOps 灰度落地
                         ▲                                          │
                         └────────── 持续校准(月度) ◀────────────────┘
```

## 为什么用 VPA + Goldilocks 组合

- **VPA**：内核级采集 CPU/内存真实用量，给出推荐值。但单独用只输出原始建议，无门槛分级，直接用容易激进。
- **Goldilocks**：在 VPA 之上加一层，按 `ensure` 策略输出不同激进程度的建议（本本分分 / 平衡 / 激进），适合不同稳定性需求的负载。

```
            VPA(原始建议)
                 │
        Goldilocks 分层
    ┌────────────┼────────────┐
    ▼            ▼            ▼
 保守(slack)   平衡(default)  紧凑(packing)
 低风险         一般服务       省成本/可容忍波动
```

## 第 1 步：启用 VPA 采集（仅推荐模式）

🟡 **中危**：起步必须 `updateMode: Off`，只采集不改 Pod，避免生产 Pod 被重启。

```yaml
apiVersion: autoscaling.k8s.io/v1
kind: VerticalPodAutoscaler
metadata: { name: api-vpa, namespace: prod }
spec:
  targetRef: { apiVersion: apps/v1, kind: Deployment, name: api }
  updatePolicy: { updateMode: "Off" }   # ★ 只采集，不自动改
  resourcePolicy:
    containerPolicies:
    - containerName: '*'
      controlledResources: ["cpu","memory"]
```

采集至少 2 周（覆盖一个完整业务周期，含峰值）。`kubectl describe vpa api-vpa` 看 recommendation。

## 第 2 步：部署 Goldilocks

```bash
# 🟡 中危：会安装集群级组件
helm repo add fairwinds-stable https://charts.fairwinds.com/stable
helm install goldilocks fairwinds-stable/goldilocks -n goldilocks --create-namespace

# 🟢 只读：给 namespace 打标签启用
kubectl label namespace prod goldilocks.fairwinds.com/enabled=true
```

```yaml
# 给 Deployment 加注解选择策略
apiVersion: apps/v1
kind: Deployment
metadata:
  name: api
  annotations:
    goldilocks.fairwinds.com/v1beta1-ensure: "balanced"   # conservative|balanced|projection
```

## 第 3 步：读取建议

```bash
# 🟢 只读
kubectl get vpa -n prod
# NAME       RECOMMENDED   TARGET
# api-vpa   True          Deployment/api

kubectl get -n goldilocks recommendation -o yaml
```

输出示例：
```yaml
recommendation:
  cpu:
    recommendation: 250m      # 当前 500m，可省一半
  memory:
    recommendation: 384Mi     # 当前 1Gi，可省 60%
```

## 第 4 步：GitOps 灰度落地

⚠️ **永远不要直接应用 Goldilocks 的自动 patch**——通过 GitOps 走 PR，人工评审后再灰度。

```yaml
# Git 仓库里的 Deployment，基于建议调小
spec:
  template:
    spec:
      containers:
      - name: api
        resources:
          requests: { cpu: 250m, memory: 384Mi }   # ← 调整
          limits:   { cpu: 1,    memory: 768Mi }    # limit = 2-3x request
        # limits 不调太死：CPU limit 会 throttle 导致延迟飙升
```

**灰度策略**：先在 Staging 跑 3 天 → Prod 先改 1 个副本（金丝雀）→ 观察 P99 与 OOM 率 → 全量。

## limits 与 requests 的取舍

| 项目 | requests | limits |
|------|----------|--------|
| 作用 | 调度 + 成本 | 稳定性 + 防抢占 |
| CPU | 必须精确 | 建议 unset 或 2-3x；过紧会 CFS throttle |
| 内存 | 必须精确 | 建议 = request 或略高；超限 OOMKill |

⚠️ **CPU throttle 陷阱**：设了 CPU limit 且应用突发用 CPU 时，会被 cgroup 限流导致延迟尖刺。对延迟敏感服务，**不设 CPU limit** 或用 `cpu.cfs_quota_us` 谨慎。

## 验证与持续校准

```bash
# 🟢 只读：对比调优前后成本
kubectl cost namespace --show-cpu --show-memory -n prod

# 调优后 1 个月复查：实际用量是否仍贴合建议？
kubectl describe vpa api-vpa | grep -A5 Recommendation
```

每月跑一次"右调优日"，处理：
1. 新上线服务（无历史数据）的初始值
2. 流量模式变化的服务（促销季前后）
3. Goldilocks 建议漂移 > 30% 的服务

## 常见陷阱

1. **VPA Auto 模式 + HPA on CPU**：两者争抢 CPU 目标，Pod 反复重启。组合用时 HPA 必须用自定义指标。
2. **采集窗口太短**：只采 2 天就调，会漏掉周末/促销峰值，调完峰值就 OOM。
3. **limits = requests**：看似"精确"，实际是放弃了 burst 缓冲，瞬时峰值就 OOM。
4. **调一次就不管**：负载会漂移，右调优是持续工程，不是一次性项目。

## 相关

- [[可靠性/容量规划/06-autoscaling-best-practices.md|06 autoscaling best practices]]
- [[可靠性/容量规划/03-resource-quota-limitrange.md|03 resource quota limitrange]]
- [[可靠性/容量规划/24-capacity-planning-forecasting.md|24 capacity planning forecasting]]

<!-- risk-assessed -->
