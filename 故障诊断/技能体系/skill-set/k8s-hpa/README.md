---
title: 'Skill: HPA 不扩容的诊断和修复'
summary: 'Skill: HPA 不扩容的诊断和修复：HPA（Horizontal Pod Autoscaler）在负载上升时未按预期扩容，导致服务响应延迟增加或请求失败。远程顾问模式下需从指标源、阈值配置、资源限制三个层面逐层排查。'
category: skill
tags:
- skill
- domain-10
- visibility/public
tier: supporting
sources:
- KUDIG Gap Analysis 2026-05-21
created: 2026-05-21
updated: 2026-05-21
last_updated: 2026-05-21
status: reviewed
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Skill: HPA 不扩容的诊断和修复

## 问题描述
HPA（Horizontal Pod Autoscaler）在负载上升时未按预期扩容，导致服务响应延迟增加或请求失败。远程顾问模式下需从指标源、阈值配置、资源限制三个层面逐层排查。

## 常见症状
- CPU/内存使用率已远超目标值，但副本数未增加
- `kubectl describe hpa` 显示 `ScalingActive` 为 False
- `kubectl top pod` 返回错误或无数据
- HPA 事件中出现 `failed to get metrics` 或 `insufficient replicas`
- 流量突增后 Pod 数量长时间保持不变

## 诊断步骤

### 步骤1: 确认 Metrics Server 正常运行
``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl get pods -n kube-system -l k8s-app=metrics-server
kubectl top nodes
kubectl top pods -n <namespace>
```
> 如果无法执行，替代方案：询问用户是否能通过监控面板（如 Prometheus Grafana）查看 Pod CPU/内存使用率，确认指标数据是否可达。

### 步骤2: 检查 HPA 配置与当前状态
``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl describe hpa <hpa-name> -n <namespace>
```
> 关注 `Metrics` 段落中的 current 与 target 值，`Conditions` 中的 `ScalingLimited` 和 `ScalingActive` 状态，以及 Events 中的时间线。

### 步骤3: 验证资源 request 与配额限制
``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl get deployment <deployment-name> -n <namespace> -o jsonpath='{.spec.template.spec.containers[*].resources.requests}'
kubectl get resourcequota -n <namespace>
kubectl get nodes -o jsonpath='{.items[*].status.allocatable}'
```
> 若 Pod 未设置 `resources.requests`，HPA 无法计算利用率。同时检查 `maxReplicas` 是否已达上限，或 ResourceQuota/节点资源是否耗尽。

## 修复措施
- **Metrics Server 异常**：重启或重新部署 Metrics Server，检查其 Service 和 API 聚合层证书
- **未设置 request**：为容器添加 `resources.requests.cpu` 和 `resources.requests.memory`
- **阈值设置不当**：调整 `targetCPUUtilizationPercentage` 或 `target.averageValue` 与实际负载匹配
- **maxReplicas 受限**：提高 HPA 的 `maxReplicas`，或解除 ResourceQuota/节点资源瓶颈
- **冷却期等待**：HPA 扩容后有稳定窗口，若刚完成一次扩容，等待 `stabilizationWindowSeconds` 后再观察

## 预防性措施
- 为所有承载 HPA 的容器显式设置 resources.requests，避免利用率无法计算
- 配置 behavior.scaleDown.stabilizationWindowSeconds 防止流量抖动导致频繁缩容
- 监控 `kube_hpa_status_condition{condition="ScalingActive"}` 提前发现 HPA 失效

## 生产案例

### 案例 1：Metrics Server 证书过期导致 HPA 完全失效

**背景**：某金融平台集群 Metrics Server 的 TLS 证书过期，HPA 无法获取指标，大促期间 Pod 未扩容导致服务崩溃。

**时间线**：
| 时间 | 事件 | 操作 |
|------|------|------|
| 10:00 | 大促流量开始，CPU 升至 85% | 🟢 `kubectl top pods -n prod` |
| 10:05 | HPA 事件: failed to get metrics | 🟢 `kubectl describe hpa web-hpa -n prod` |
| 10:10 | 服务响应超时，错误率飙升 | 🟢 `kubectl get pods -n prod \| grep -c Running` |
| 10:12 | 确认 Metrics Server 证书过期 | 🟢 `kubectl logs -n kube-system -l k8s-app=metrics-server --tail=20` |
| 10:15 | 手动扩容 + 修复证书 | 🟡 `kubectl scale deploy web -n prod --replicas=20` |

**根因**：Metrics Server 的 serving 证书由集群 CA 签发，有效期 1 年，过期后 apiserver 无法通过 APIService 聚合层获取指标。

### 案例 2：未设置 resources.requests 导致 HPA 计算异常

**根因**：Deployment 未设置 `resources.requests.cpu`，HPA 无法计算 CPU 利用率百分比，`currentMetrics` 显示 `<unknown>`。

**修复**：
``` bash
# 🟡 中风险：添加 resources.requests
kubectl patch deployment web -n prod --type json -p '[{"op":"add","path":"/spec/template/spec/containers/0/resources/requests","value":{"cpu":"250m","memory":"256Mi"}}]'
```

## 升级决策点

- **P0（立即处理）**：HPA 失效 + 流量突增，服务已过载，需立即手动扩容
- **P1（30分钟内）**：HPA 异常但当前负载未达上限，有时间排查修复
- **P2（下一工作日）**：HPA 配置不当但当前副本数足够，不影响业务

## 面试要点

1. **Q: HPA 的扩缩容算法是什么？**
   A: `desiredReplicas = ceil[currentReplicas × (currentMetricValue / desiredMetricValue)]`。例如当前 3 副本，CPU 80%，目标 50%，则期望 = ceil[3 × (80/50)] = 5。扩容立即执行，缩容受 stabilizationWindowSeconds（默认 300s）保护。

2. **Q: HPA 与 VPA 能否同时使用？为什么？**
   A: 不能同时对同一指标使用。HPA 调整副本数，VPA 调整单 Pod 资源，同时使用会产生冲突（VPA 增加资源→CPU%下降→HPA 缩容）。可以组合使用：HPA 基于 CPU/内存，VPA 基于自定义指标或仅用于 recommendation 模式。

3. **Q: 如何监控 HPA 健康状态？**
   A: ① `kube_hpa_status_condition{condition="ScalingActive"}==0` 告警；② `kube_hpa_status_current_replicas == kube_hpa_spec_max_replicas` 持续 5min 告警（触顶）；③ 对比 `kube_hpa_status_desired_replicas` 与 `current_replicas` 差异；④ 监控 Metrics Server 可用性和延迟。

## 相关概念

- [[概念/horizontal-pod-autoscaler.md|Horizontal Pod Autoscaler]] — HPA 指标采集、扩缩容算法与配置
- [[概念/autoscaling-strategies.md|自动扩缩容策略]] — HPA、VPA、Cluster Autoscaler 选型与协同
- [[概念/resource-management.md|资源管理]] — Kubernetes 资源请求、限制与配额管理

## Related

- [[visibility-public|#visibility/public Hub]] — tag hub


<!-- risk-assessed -->
