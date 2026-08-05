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

## 网络隔离策略

### 网络分区实验

```yaml
# 使用 NetworkChaos 实现网络分区
apiVersion: chaos-mesh.org/v1alpha1
kind: NetworkChaos
metadata:
  name: network-partition
  namespace: production
spec:
  action: partition
  mode: fixed-percent
  value: "10"  # 仅影响 10% Pod
  selector:
    namespaces: ["production"]
    labelSelectors:
      app: api-service
      chaos-enable: "true"
  direction: both
  target:
    selector:
      namespaces: ["production"]
      labelSelectors:
        app: database
    mode: all
  duration: "60s"
```

### 带宽限制实验

```yaml
apiVersion: chaos-mesh.org/v1alpha1
kind: NetworkChaos
metadata:
  name: bandwidth-limit
  namespace: production
spec:
  action: bandwidth
  mode: fixed
  value: "1"
  selector:
    namespaces: ["production"]
    labelSelectors:
      app: api-service
      chaos-enable: "true"
  bandwidth:
    rate: "10mbps"  # 限制带宽
    limit: 1000
    buffer: 10000
  duration: "120s"
```

## 时间窗口控制

### 实验时间窗口

```yaml
# 使用 Schedule 控制实验时间窗口
apiVersion: chaos-mesh.org/v1alpha1
kind: Schedule
metadata:
  name: business-hours-chaos
  namespace: production
spec:
  schedule: "0 10 * * 1-5"  # 仅工作日 10:00
  historyLimit: 5
  concurrencyPolicy: Forbid
  podChaos:
    selector:
      namespaces: ["production"]
      labelSelectors:
        app: api-service
        chaos-enable: "true"
    action: pod-kill
    mode: fixed
    value: "1"
    duration: "30s"
```

### 黑名单时间窗口

```yaml
# 使用 Webhook 拒绝特定时间的实验
apiVersion: admissionregistration.k8s.io/v1
kind: ValidatingWebhookConfiguration
metadata:
  name: chaos-time-window
webhooks:
  - name: chaos-time-window.chaos-mesh.org
    clientConfig:
      service:
        name: chaos-mesh-controller-manager
        namespace: chaos-mesh
        path: /validate-chaos-experiment
    rules:
      - apiGroups: ["chaos-mesh.org"]
        apiVersions: ["v1alpha1"]
        operations: ["CREATE"]
        resources: ["podchaos", "networkchaos", "stresschaos"]
    admissionReviewVersions: ["v1"]
    sideEffects: None
```

## 依赖保护

### 关键依赖豁免

```yaml
# 保护关键依赖不被注入
apiVersion: chaos-mesh.org/v1alpha1
kind: PodChaos
metadata:
  name: api-pod-kill
  namespace: production
spec:
  selector:
    namespaces: ["production"]
    labelSelectors:
      app: api-service
      chaos-enable: "true"
    # 排除关键依赖
    expressionSelectors:
      - key: app
        operator: NotIn
        values:
          - database
          - cache
          - message-queue
  action: pod-kill
  mode: fixed
  value: "1"
  duration: "30s"
```

### 监控链路保护

```yaml
# 监控组件永远不参与混沌实验
apiVersion: v1
kind: Namespace
metadata:
  name: monitoring
  labels:
    chaos-mesh.org/exclude: "true"  # 排除标签
---
# 在 Chaos Mesh 配置中排除
apiVersion: v1
kind: ConfigMap
metadata:
  name: chaos-mesh-config
  namespace: chaos-mesh
data:
  excludedNamespaces: |
    - monitoring
    - logging
    - chaos-mesh
    - kube-system
```

## 实验审计

### 审计日志配置

```yaml
# 实验审计日志
apiVersion: v1
kind: ConfigMap
metadata:
  name: chaos-audit-config
  namespace: chaos-mesh
data:
  audit.yaml: |
    audit:
      enabled: true
      logLevel: info
      destinations:
        - type: file
          path: /var/log/chaos/audit.log
        - type: webhook
          url: http://audit-collector:8080/chaos-events
      events:
        - experiment_start
        - experiment_end
        - experiment_abort
        - target_selected
```

### 审计查询

```bash
# 🟢 低风险：查询实验历史
kubectl get events -n production --field-selector reason=ChaosExperimentStart

# 🟢 低风险：查看实验记录
kubectl get podchaos -n production -o custom-columns=\
NAME:.metadata.name,\
START:.status.startTime,\
END:.status.endTime,\
TARGETS:.status.experiment.pods

# 🟢 低风险：查看特定时间的实验
kubectl get events -n production \
  --field-selector reason=ChaosExperimentStart,involvedObject.name=api-pod-kill
```

## 紧急恢复程序

### 一键停止所有实验

```bash
#!/bin/bash
# 🔴 高风险：紧急停止所有混沌实验
set -euo pipefail

echo "=== 紧急停止所有混沌实验 ==="

# 1. 删除所有 Chaos 资源
kubectl delete podchaos --all -A --wait=false
kubectl delete networkchaos --all -A --wait=false
kubectl delete stresschaos --all -A --wait=false
kubectl delete iochaos --all -A --wait=false
kubectl delete kernelchaos --all -A --wait=false
kubectl delete httpchaos --all -A --wait=false
kubectl delete dnschaos --all -A --wait=false

# 2. 等待清理完成
sleep 10

# 3. 验证清理
REMAINING=$(kubectl get podchaos,networkchaos,stresschaos -A --no-headers 2>/dev/null | wc -l)
if [ "$REMAINING" -gt 0 ]; then
  echo "⚠️ 仍有 $REMAINING 个实验未清理"
else
  echo "✓ 所有实验已停止"
fi

# 4. 检查 Pod 状态
echo "检查 Pod 状态..."
kubectl get pods -A --field-selector=status.phase!=Running,status.phase!=Succeeded

echo "=== 紧急停止完成 ==="
```

### 恢复验证检查清单

| 序号 | 检查项 | 命令 | 通过标准 |
|-----|--------|------|----------|
| 1 | 所有实验已停止 | `kubectl get podchaos -A` | 无资源 |
| 2 | Pod 全部 Running | `kubectl get pods -A` | 无 Pending/Failed |
| 3 | 错误率正常 | 检查 Prometheus | < 1% |
| 4 | 延迟正常 | 检查 P99 | < 500ms |
| 5 | 服务可访问 | `curl http://api/health` | HTTP 200 |

## 爆炸半径评估矩阵

| 实验类型 | 直接影响 | 级联风险 | 可逆性 | 风险等级 |
|---------|---------|---------|--------|----------|
| Pod Kill (1个) | 低 | 低 | 高 (自动重建) | 🟢 低 |
| Pod Kill (30%) | 中 | 中 | 高 | 🟡 中 |
| 网络延迟 | 中 | 中 | 高 (删除即恢复) | 🟡 中 |
| 网络分区 | 高 | 高 | 中 | 🔴 高 |
| CPU 压力 | 中 | 中 | 高 | 🟡 中 |
| 磁盘 IO | 高 | 高 | 中 | 🔴 高 |
| 节点宕机 | 高 | 高 | 中 | 🔴 高 |
| AZ 故障 | 极高 | 极高 | 低 | 🔴 极高 |

## 相关

- [[12-可靠性/04-混沌工程/05-chaos-experiment-automation.md|05 chaos experiment automation]]
- [[12-可靠性/04-混沌工程/03-chaos-experiment-design.md|03 chaos experiment design]]
- [[12-可靠性/06-SRE实践/10-incident-command-field-guide.md|07 incident command field guide]]

<!-- risk-assessed -->
