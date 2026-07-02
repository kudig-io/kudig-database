---
title: '[2026-02-18] [P1] HPA 配置错误导致无限扩缩容 Thrashing'
summary: '[2026-02-18] [P1] HPA 配置错误导致无限扩缩容 Thrashing：11:05，监控显示 order-api Deployment
  的 Pod 数量在 5 分钟内从 3 个飙升到 15 个，又迅速降至 3 个，如此反复。Grafana 扩缩容事件图呈现"锯齿"状。'
category: case-study
tags:
- production
- incident
- workloads
- autoscaling
- hpa
tier: supporting
created: '2026-05-23'
updated: '2026-05-23'
date: '2026-02-18'
severity: P1
mttr: 32min
status: resolved
last_updated: 2026-05-23
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# [2026-02-18] HPA Thrashing 导致订单服务 Pod 在 5min 内扩缩 12 次

## 工单信息
- **工单编号**: INC-2026-0218-004
- **发现时间**: 2026-02-18 11:05 UTC
- **恢复时间**: 2026-02-18 11:37 UTC
- **影响范围**: `prod-order` namespace，订单服务 Deployment
- **业务影响**: 订单创建接口 P99 延迟从 120ms 飙升至 3.2s，部分请求 504 Gateway Timeout

## 问题现象
11:05，监控显示 `order-api` Deployment 的 Pod 数量在 5 分钟内从 3 个飙升到 15 个，又迅速降至 3 个，如此反复。Grafana 扩缩容事件图呈现"锯齿"状。

用户反馈：
- 下单页面偶发卡顿，高峰期响应慢
- 部分订单提交后没有响应

## 诊断过程

**11:07** — 查看 HPA 状态：
``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl get hpa order-api-hpa -n prod-order -o yaml
# ...
# spec:
#   scaleTargetRef:
#     apiVersion: apps/v1
#     kind: Deployment
#     name: order-api
#   minReplicas: 3
#   maxReplicas: 15
#   metrics:
#   - type: Resource
#     resource:
#       name: cpu
#       target:
#         type: Utilization
#         averageUtilization: 50
#   behavior:
#     scaleDown:
#       stabilizationWindowSeconds: 0
#       policies:
#       - type: Percent
#         value: 100
#         periodSeconds: 15
#     scaleUp:
#       stabilizationWindowSeconds: 0
#       policies:
#       - type: Percent
#         value: 100
#         periodSeconds: 15
```
**11:10** — 查看 HPA 事件：
``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl describe hpa order-api-hpa -n prod-order
# ...
# Normal  SuccessfulRescale  2m    horizontal-pod-autoscaler  New size: 15; reason: cpu resource utilization (percentage of request) above target
# Normal  SuccessfulRescale  2m15s horizontal-pod-autoscaler  New size: 3;  reason: All metrics below target
# Normal  SuccessfulRescale  2m30s horizontal-pod-autoscaler  New size: 15; reason: cpu resource utilization (percentage of request) above target
# ...（共 12 次 rescale 事件）
```
**11:12** — 分析 CPU 利用率波动原因：
``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl top pods -n prod-order -l app=order-api
# NAME                          CPU(cores)   MEMORY(bytes)
# order-api-7d9f4b8c5a-abc12   450m         256Mi
# order-api-7d9f4b8c5a-def34   420m         248Mi
# ...

# 检查 requests：
kubectl get deployment order-api -n prod-order -o jsonpath='{.spec.template.spec.containers[0].resources.requests.cpu}'
# 500m

# 实际 CPU 使用率 = 450m / 500m = 90% > 50% 阈值 → 扩容
# 扩容后新 Pod 启动，流量被稀释，单 Pod CPU 下降 → 缩容
# 缩容后流量集中到剩余 Pod，CPU 再次飙升 → 扩容
```
**11:15** — 确认 `stabilizationWindowSeconds: 0` 是问题根源。新 Pod 冷启动后 JVM 预热、连接池初始化需要时间，但 HPA 在 15 秒内就判定可以缩容，导致反复震荡。

## 根因
HPA `behavior.scaleDown.stabilizationWindowSeconds` 和 `scaleUp.stabilizationWindowSeconds` 均被设置为 `0`，且 `periodSeconds` 只有 `15s`。这意味着：
1. Pod 扩容后，流量被分散，CPU 利用率在 15 秒内就下降到 50% 以下
2. HPA 立即触发缩容
3. Pod 减少后，CPU 又超过阈值，再次扩容
4. 形成扩缩容震荡（Thrashing），JVM 反复预热，连接池反复创建，加剧延迟

## 修复动作

**11:20** — 修正 HPA 配置，添加冷却窗口和更保守的策略：

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
cat <<'EOF' | kubectl apply -f -
apiVersion: autoscaling/v2
kind: HorizontalPodAutoscaler
metadata:
  name: order-api-hpa
  namespace: prod-order
spec:
  scaleTargetRef:
    apiVersion: apps/v1
    kind: Deployment
    name: order-api
  minReplicas: 3
  maxReplicas: 15
  metrics:
  - type: Resource
    resource:
      name: cpu
      target:
        type: Utilization
        averageUtilization: 50
  behavior:
    scaleDown:
      stabilizationWindowSeconds: 300
      policies:
      - type: Percent
        value: 10
        periodSeconds: 60
    scaleUp:
      stabilizationWindowSeconds: 0
      policies:
      - type: Percent
        value: 100
        periodSeconds: 60
      - type: Pods
        value: 4
        periodSeconds: 60
      selectPolicy: Max
EOF
```
**11:25** — 验证 HPA 已稳定：
``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl get hpa order-api-hpa -n prod-order
# NAME            REFERENCE              TARGETS   MINPODS   MAXPODS   REPLICAS   AGE
# order-api-hpa   Deployment/order-api   45%/50%   3         15        8          20m

# 观察 5 分钟，replicas 稳定在 8，无 rescale 事件
kubectl get events -n prod-order --field-selector reason=SuccessfulRescale --sort-by='.lastTimestamp'
# （无新事件）
```
**11:30** — 压测验证：
```bash
# 使用 k6 进行 5min 压测
k6 run --vus 100 --duration 5m order-api-smoke-test.js
# http_req_duration..............: avg=145ms  p(95)=280ms  p(99)=420ms
```

## 验证
- 11:33 — HPA replicas 稳定在 8，CPU 利用率维持在 45%-48%
- 11:35 — 订单接口 P99 延迟恢复至 180ms
- 11:37 — 全部业务指标正常，无 504 错误

## 复盘
- **直接原因**: HPA `stabilizationWindowSeconds=0` + `periodSeconds=15s` → 扩缩容震荡 → JVM 反复预热 → 高延迟
- **根本原因**: 开发团队在压测后误将 HPA 的 scaleDown 窗口设为 0（"为了快速缩容省钱"），未经过 SRE 评审
- **改进措施**:
  1. 所有 HPA 配置纳入 GitOps，变更需 SRE 审批
  2. HPA `scaleDown.stabilizationWindowSeconds` 默认值 ≥300s，`scaleDown` 速率限制 ≤10%/min
  3. 在监控大盘添加 `hpa_rescale_rate` 指标，> 2 次/5min 触发告警
  4. 编写 HPA 配置检查脚本：`kubectl get hpa --all-namespaces -o yaml | yq '.items[].spec.behavior.scaleDown.stabilizationWindowSeconds'`
- **相关 Skill**: [[k8s-scaling-guide]]
- **相关 FTA**: [[hpa-fta]]


<!-- risk-assessed -->
