---
dialogue_id: "DIALOGUE-HPA-001"
skill_id: "SKILL-HPA-001"
role: "remote-consultant"
language: "zh"
severity: "medium"
status: "reviewed"
created: 2026-05-21
updated: 2026-05-21
last_updated: 2026-05-21
title: "HPA 不扩容，流量高峰期服务响应慢 — 远程顾问对话脚本"
category: dialogue
tags: ["dialogue", "remote-consultant", "troubleshooting", "visibility/public"]
---

# HPA 不扩容，流量高峰期服务响应慢 — 远程顾问对话脚本

> 对应概念：[[concepts/horizontal-pod-autoscaler.md|HPA]]
> 顾问身份：部署在客户专有云之外的远程 SRE 专家，**无法直接连接集群**。

---

## 对话入口

**工程师**：HPA 在流量高峰期没有自动扩容，导致服务响应变慢。

**顾问回应**：收到。请先确认：涉及的应用名称、命名空间，以及当前的 Pod 数量和 HPA 目标副本数是多少？

---

### 步骤 1: 确认 HPA 资源状态

**顾问**：请先查看 HPA 的当前状态：

```bash
kubectl get hpa -n <namespace>
```

> **如果无法执行**：请通过集群管理控制台查看 HPA 列表，或提供 HPA 名称以便后续命令使用。

```bash
kubectl get hpa <hpa-name> -n <namespace> -o jsonpath='{.status.conditions}'
```

> **如果无法执行**：请查看 HPA 的 Events 和 Conditions 信息，重点关注 `ScalingActive` 和 `AbleToScale` 状态。

**预期用户回复**：HPA 的 TARGETS 列显示为 `unknown`，或 `CURRENT` 值远低于目标值，或 Conditions 显示 `ScalingDisabled`。

**下一步判断**：
- 若 TARGETS 为 unknown → 进入步骤 2 检查 metrics-server
- 若 CURRENT 远低于 target 但仍未扩容 → 进入步骤 3 查看 HPA 事件
- 若 HPA 不存在 → 进入步骤 6 修复方案（创建 HPA）

---

### 步骤 2: 检查 metrics-server

**顾问**：请确认 metrics-server 是否正常运行：

```bash
kubectl top nodes
```

> **如果无法执行**：请通过控制台查看节点资源监控图表，或检查 metrics-server Pod 是否存在于 kube-system 命名空间。

```bash
kubectl get pods -n kube-system | grep metrics-server
```

> **如果无法执行**：请确认集群是否启用了监控插件，或查看节点监控数据是否可获取。

```bash
kubectl top pods -n <namespace>
```

> **如果无法执行**：请查看应用 Pod 的资源使用监控数据，确认 CPU/内存指标是否正常上报。

**预期用户回复**：metrics-server 未运行，或 `kubectl top` 报错 `metrics not available yet`，或 metrics-server Pod 处于 CrashLoopBackOff。

**下一步判断**：
- 若 metrics-server 未运行 → 进入步骤 6 修复方案（安装 metrics-server）
- 若 metrics-server 正常但 top 无数据 → 检查 Pod 的 metrics 端点
- 若 metrics-server 正常 → 进入步骤 3 查看 HPA 事件

---

### 步骤 3: 查看 HPA 事件

**顾问**：请查看 HPA 的详细事件信息：

```bash
kubectl describe hpa <hpa-name> -n <namespace>
```

> **如果无法执行**：请通过控制台查看 HPA 详情页的 Events 标签，或提供 describe 输出的截图。

```bash
kubectl get events -n <namespace> --field-selector reason=FailedGetResourceMetric
```

> **如果无法执行**：请搜索 Events 中是否有 `FailedGetResourceMetric` 或 `FailedComputeMetricsReplicas` 相关错误。

**预期用户回复**：Events 中出现 `FailedGetResourceMetric`、`ScalingActive=False` 或 `the HPA was unable to compute the replica count` 等错误。

**下一步判断**：
- 若 FailedGetResourceMetric → 检查 metrics-server 或自定义指标配置
- 若 ScalingLimited → 进入步骤 4 检查 target
- 若 正常但无扩容事件 → 进入步骤 5 检查 behavior

---

### 步骤 4: 检查 target 配置

**顾问**：请查看 HPA 的 target 和指标配置：

```bash
kubectl get hpa <hpa-name> -n <namespace> -o yaml
```

> **如果无法执行**：请通过控制台查看 HPA 的 YAML 配置，或提供 HPA 配置文件的内容。

```bash
kubectl get hpa <hpa-name> -n <namespace> -o jsonpath='{.spec.metrics}'
```

> **如果无法执行**：请手动查看 YAML 中 `spec.metrics` 部分，确认 target 类型（Utilization/AverageValue/Value）和数值。

**预期用户回复**：target 的 utilization 值设置过高（如 80%），或使用了不存在的自定义指标，或 `scaleTargetRef` 指向了错误的 Deployment。

**下一步判断**：
- 若 target 设置不合理 → 进入步骤 6 修复方案（调整 target）
- 若 scaleTargetRef 错误 → 进入步骤 6 修复方案（修正引用）
- 若配置正常 → 进入步骤 5 检查 behavior

---

### 步骤 5: 检查 behavior 配置

**顾问**：请检查 HPA 的扩缩容行为配置：

```bash
kubectl get hpa <hpa-name> -n <namespace> -o jsonpath='{.spec.behavior}'
```

> **如果无法执行**：请查看 HPA YAML 中 `spec.behavior` 部分，确认 scaleUp 和 scaleDown 的策略设置。

```bash
kubectl get hpa <hpa-name> -n <namespace> -o yaml | grep -A 20 scaleUp
```

> **如果无法执行**：请手动检查 YAML 中 `scaleUp.stabilizationWindowSeconds` 和 `policies` 配置，是否存在过长的稳定窗口或过于保守的扩容策略。

**预期用户回复**：scaleUp 配置了较长的 stabilizationWindowSeconds（如 300s），或 policies 限制了扩容速率（如每分钟只允许增加 1 个 Pod）。

**下一步判断**：
- 若 behavior 过于保守 → 进入步骤 6 修复方案（修改 behavior）
- 若 behavior 正常 → 提示检查 Deployment 的 maxSurge/maxUnavailable 限制

---

### 步骤 6: 提供修复方案

**顾问**：根据以上排查，请按对应根因执行修复：

#### 方案 A：安装 metrics-server

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

```bash
kubectl apply -f https://github.com/kubernetes-sigs/metrics-server/releases/latest/download/components.yaml
```

> **如果无法执行**：请从官方仓库下载 YAML 文件后本地 apply，或确认集群云厂商是否提供一键安装 metrics-server 的选项。

#### 方案 B：调整 target 配置

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

```bash
kubectl patch hpa <hpa-name> -n <namespace> --type='merge' -p='{"spec":{"metrics":[{"type":"Resource","resource":{"name":"cpu","target":{"type":"Utilization","averageUtilization":50}}}]}}'
```

> **如果无法执行**：请使用 `kubectl edit hpa <hpa-name> -n <namespace>` 手动降低 `averageUtilization` 的值（如从 80% 改为 50%）。

#### 方案 C：修改 behavior 配置

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

```bash
cat <<EOF | kubectl apply -f -
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
  maxReplicas: 20
  metrics:
  - type: Resource
    resource:
      name: cpu
      target:
        type: Utilization
        averageUtilization: 50
  behavior:
    scaleUp:
      stabilizationWindowSeconds: 30
      policies:
      - type: Percent
        value: 100
        periodSeconds: 15
    scaleDown:
      stabilizationWindowSeconds: 300
      policies:
      - type: Percent
        value: 10
        periodSeconds: 60
EOF
```

> **如果无法执行**：请将上述 YAML 保存为文件后 apply，或使用 `kubectl edit` 修改现有 HPA 的 behavior 字段。

#### 方案 D：修正 scaleTargetRef

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

```bash
kubectl patch hpa <hpa-name> -n <namespace> --type='merge' -p='{"spec":{"scaleTargetRef":{"apiVersion":"apps/v1","kind":"Deployment","name":"<correct-deployment>"}}}'
```

> **如果无法执行**：请使用 `kubectl edit hpa <hpa-name> -n <namespace>` 手动修改 `spec.scaleTargetRef.name` 为正确的 Deployment 名称。

**验证修复**：

```bash
kubectl get hpa <hpa-name> -n <namespace> -w
```

> **如果无法执行**：请间歇性执行 `kubectl get hpa <hpa-name> -n <namespace>` 观察 CURRENT 和 REPLICAS 是否随负载上升而增加。

---

## 相关概念

- [[concepts/horizontal-pod-autoscaler.md|HPA]]
- [[concepts/metrics-server.md|Metrics Server]]
- [[concepts/horizontal-pod-autoscaler.md|自动扩缩容策略]]
