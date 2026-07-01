---
title: "Skill: Pod Pending/调度失败的诊断和修复"
category: skill
tags: ["skill", "domain-10", "visibility/public"]
sources: ["KUDIG Gap Analysis 2026-05-21"]
created: 2026-05-21
updated: 2026-05-21
last_updated: 2026-05-21
status: reviewed
---

# Skill: Pod Pending/调度失败的诊断和修复

## 问题描述
Pod 长时间处于 Pending 状态，无法被调度到任何节点运行。远程顾问模式下需通过 `kubectl describe pod` 的 Events 来定位资源、约束或污点层面的根因。

## 常见症状
- `kubectl get pods` 显示 Pod 状态为 Pending 且持续不退
- `kubectl describe pod` Events 中出现 `Insufficient cpu` 或 `Insufficient memory`
- Events 中出现 `node(s) had taint` 或 `node(s) didn't match pod affinity/anti-affinity`
- 新扩容的 Deployment 副本部分 Running、部分 Pending
- 使用 PVC 的 Pod Pending 伴随卷绑定延迟事件

## 诊断步骤

### 步骤1: 查看 Pod Events 定位调度失败原因
```bash
kubectl describe pod <pod-name> -n <namespace>
```
> 在 Events 段落查找 `FailedScheduling` 事件，记录具体原因（资源不足、污点、亲和性、卷延迟等）。
> 如果无法执行，替代方案：请用户截取 Pod 详情页面中 Events 部分的截图，或提供 Pending Pod 的名称和命名空间。

### 步骤2: 检查集群资源与节点状态
```bash
kubectl top nodes
kubectl describe node <node-name>
kubectl get nodes -o wide
```
> 查看节点的 Allocatable 与 Requested 资源对比，确认是否有节点处于 NotReady 或 Cordoned 状态。

### 步骤3: 核对约束条件与污点容忍
```bash
kubectl get pod <pod-name> -n <namespace> -o jsonpath='{.spec.nodeSelector}{.spec.affinity}{.spec.tolerations}'
kubectl get node <node-name> -o jsonpath='{.spec.taints}{.metadata.labels}'
```
> 对比 Pod 的 `nodeSelector`、`affinity`、`tolerations` 与节点的 `labels`、`taints`，确认是否匹配。

## 修复措施
- **资源不足**：扩容节点（Cluster Autoscaler）、降低 Pod 的 request、清理低优先级 Pod
- **污点不匹配**：为 Pod 添加对应的 tolerations，或移除节点上不必要的 taints
- **亲和性冲突**：调整 Pod Anti-Affinity 的 `topologyKey`，或增加可用节点/拓扑域数量
- **nodeSelector 错误**：修正节点选择器标签，确保与目标节点标签一致
- **PVC 绑定延迟**：检查存储后端状态、StorageClass 配置及卷配额，必要时手动创建 PV
- **自定义调度插件拦截**：确认是否启用了 GPU 调度、容量调度等第三方插件，查看 kube-scheduler 日志

## 预防性措施
- 在部署前使用 `kubectl apply --dry-run=server` 验证 Pod 约束是否与节点拓扑兼容
- 对关键业务配置 Cluster Autoscaler 和优先级抢占，避免资源不足导致长期 Pending
- 建立节点标签与污点管理规范，防止非预期排斥

## 相关概念

- [[concepts/kube-scheduler.md|Kube Scheduler]] — Kubernetes 调度器原理、算法与扩展机制
- [[concepts/node-lifecycle-management.md|节点生命周期管理]] — 节点注册、状态维护与驱逐机制
- [[concepts/resource-management.md|资源管理]] — Kubernetes 资源请求、限制与配额管理
