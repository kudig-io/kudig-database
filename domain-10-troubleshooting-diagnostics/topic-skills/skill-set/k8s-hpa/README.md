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

## 相关概念

- [[concepts/horizontal-pod-autoscaler.md|Horizontal Pod Autoscaler]] — HPA 指标采集、扩缩容算法与配置
- [[concepts/autoscaling-strategies.md|自动扩缩容策略]] — HPA、VPA、Cluster Autoscaler 选型与协同
- [[concepts/resource-management.md|资源管理]] — Kubernetes 资源请求、限制与配额管理

## Related

- [[visibility-public|#visibility/public Hub]] — tag hub


<!-- risk-assessed -->
