---
title: 混沌实验爆炸半径控制
description: 混沌实验的爆炸半径控制策略：命名空间隔离、百分比注入、自动中止与熔断器
summary: 用命名空间/标签/百分比/自动中止四层控制，把混沌实验的副作用锁在可逆范围内
category: reliability
tags:
- slo
- sli
- reliability
- chaos-engineering
- blast-radius
- safety
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

# 混沌实验爆炸半径控制

> **核心原则**：混沌实验的价值在于"安全地发现不安全"。如果实验本身能造成不可逆损害，它就不是混沌工程，而是"自残"。爆炸半径控制的目标是让每次实验都满足一个约束——**最坏情况下，30 秒内可完全撤销**。

## 四层控制模型

```
┌──────────────────────────────────────────────┐
│ 第4层  自动中止（Auto-abort）  SLO 突破即停    │  最内层兜底
├──────────────────────────────────────────────┤
│ 第3层  百分比注入              从 1% 开始灰度   │
├──────────────────────────────────────────────┤
│ 第2层  标签/命名空间隔离       只打特定目标     │
├──────────────────────────────────────────────┤
│ 第1层  环境隔离                Staging 先行     │  最外层
└──────────────────────────────────────────────┘
```

## 第 1 层：环境隔离

```
实验路径：Local → Staging → Prod 小流量 → Prod 全量
           ↑ 每一级必须通过才能进入下一级
```

**规则**：任何新实验类型必须先在 Staging 跑通 3 次无异常，才能进 Prod。Prod 首次只允许 1% 流量。

## 第 2 层：标签与命名空间精准选择

```yaml
# Chaos Mesh：用 selector 精准锁定，避免误伤
apiVersion: chaos-mesh.org/v1alpha1
kind: PodChaos
metadata: { name: api-pod-kill }
spec:
  selector:
    namespaces: ["prod-payment"]              # 命名空间级
    labelSelectors:
      "app": "payment-api"                     # 应用级
      "chaos-enable": "true"                   # 必须显式 opt-in 标签
    annotationSelectors:
      "chaos.example.com/max-replicas": "1"    # 注解约束
  action: pod-kill
  mode: fixed                                   # 固定数量，而非 all
  value: "1"                                    # 最多杀 1 个 pod
```

**铁律**：生产工作负载必须**显式 opt-in**（`chaos-enable: true`），否则任何实验都不应选到它。这是防止"误杀未声明参与"服务的最后一道选择层。

## 第 3 层：百分比注入（渐进式）

从 `fixed` 数量升级到 `fixed-percent`，按比例放大：

| 阶段 | mode | value | 说明 |
|------|------|-------|------|
| 首次 | `fixed` | 1 | 杀 1 个 pod |
| 第二轮 | `fixed-percent` | 10 | 杀 10% |
| 第三轮 | `fixed-percent` | 30 | 杀 30% |
| 压力测试 | `fixed-percent` | 50 | 极限 |

```yaml
spec:
  mode: fixed-percent
  value: "10"   # 仅影响 10% 副本
```

**规则**：每级至少跑一轮稳定后再升级，且每级之间留 5 分钟观察窗口。

## 第 4 层：自动中止（最关键）

### Chaos Mesh 内置 duration + 自动清理

```yaml
spec:
  duration: "60s"           # 硬超时：60 秒后自动结束
  scheduler:
    cron: "@every 0"        # 不重复
```

### 外部 SLO 看门狗（推荐）

用一个独立 Controller 监控 SLO，突破即 kill 实验：

```bash
# 🟢 只读监控 → 🟡 中危（删除 Chaos 资源）
kubectl -n monitoring run chaos-watch --rm -i --restart=Never \
  --image=prometheus-checker -- \
  watch-slo \
    --query 'error_rate{job="payment-api"}' \
    --threshold 0.01 \
    --on-breach "kubectl -n prod-payment delete podchaos --all" \
    --interval 5s
```

逻辑：每 5 秒查一次错误率，超过 1% 立即删除所有 Chaos 资源。这比 `duration` 更智能——问题一出现就停，不硬等满 60 秒。

### PrometheusRule 双阈值中止告警

```yaml
groups:
- name: chaos-abort
  rules:
  - alert: ChaosBlastRadiusExceeded
    expr: |
      error_rate{job="payment-api"} > 0.05
      and on()
      chaos_experiment_active == 1
    for: 15s
    annotations:
      summary: "混沌实验触发错误率飙升，立即评估中止"
      runbook: "删除: kubectl delete podchaos -n prod-payment --all"
```

## 中止后恢复验证

```
实验中止
   │
   ├─ T+0    删除 Chaos 资源
   ├─ T+30s  验证 pod 全部重建 (kubectl get pods | grep Running)
   ├─ T+60s  验证 SLO 回到绿区
   └─ T+5m   稳定 → 记录"最大影响" → 归档
```

## 爆炸半径 Checklist（实验前必填）

- [ ] 选择器是否精准到具体 namespace + label？没有 `*` 通配？
- [ ] 目标是否有 `chaos-enable: true` 显式 opt-in？
- [ ] `duration` 是否设了硬超时？
- [ ] 是否有 SLO 看门狗会自动中止？
- [ ] 回滚命令是否预写在 runbook 并本地验证过？
- [ ] 最坏情况下 30 秒内能否完全撤销？（否 → 重新设计实验）
- [ ] 是否通知了值班 on-call？（生产实验必须）

## 常见陷阱

1. **`mode: all`**：生产环境永远不要用 `all`，一次杀光所有副本。
2. **看门狗和实验在同一 namespace**：实验把这个 namespace 打挂了，看门狗也死了。看门狗必须独立部署。
3. **没考虑级联失败**：杀一个 pod 本没事，但 HPA 来不及扩容 → 上游超时 → 雪崩。爆炸半径不止是"直接影响的 pod 数"。
4. **依赖同样被注入**：注入延迟时连监控/告警链路一起延迟了，告警发不出来。监控链路必须免疫。

## 相关

- [[可靠性/混沌工程/05-chaos-experiment-automation.md|05 chaos experiment automation]]
- [[可靠性/混沌工程/03-chaos-experiment-design.md|03 chaos experiment design]]
- [[可靠性/SRE实践/07-incident-command-field-guide.md|07 incident command field guide]]

<!-- risk-assessed -->
