---
title: 'Skill: Pod Pending/调度失败的诊断和修复'
summary: 'Skill: Pod Pending/调度失败的诊断和修复：Pod 长时间处于 Pending 状态，无法被调度到任何节点运行。远程顾问模式下需通过
  kubectl describe pod 的 Events 来定位资源、约束或污点层面的根因。'
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
``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl describe pod <pod-name> -n <namespace>
```
> 在 Events 段落查找 `FailedScheduling` 事件，记录具体原因（资源不足、污点、亲和性、卷延迟等）。
> 如果无法执行，替代方案：请用户截取 Pod 详情页面中 Events 部分的截图，或提供 Pending Pod 的名称和命名空间。

### 步骤2: 检查集群资源与节点状态
``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl top nodes
kubectl describe node <node-name>
kubectl get nodes -o wide
```
> 查看节点的 Allocatable 与 Requested 资源对比，确认是否有节点处于 NotReady 或 Cordoned 状态。

### 步骤3: 核对约束条件与污点容忍
``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
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

## 生产案例

### 案例 1：Pod Anti-Affinity 导致新 Pod 无法调度

**背景**：某微服务配置了 `requiredDuringScheduling` Pod Anti-Affinity（topologyKey: kubernetes.io/hostname），3 副本集群中 3 个节点已被占满，第 4 个副本永远 Pending。

**根因**：硬性反亲和性要求每个节点最多 1 个该服务 Pod，节点数 = 副本数时无法扩容。

**修复**：
``` bash
# 🟡 中风险：将硬性反亲和改为软性
kubectl patch deploy web -n prod --type json -p '[{"op":"replace","path":"/spec/template/spec/affinity/podAntiAffinity/requiredDuringSchedulingIgnoredDuringExecution","value":[]}]'
# 或添加新节点
kubectl scale nodepool default --replicas=5  # 云厂商 CLI
```

### 案例 2：节点 Taint 未正确配置导致批量 Pending

**背景**：节点池升级后自动添加了 `node.kubernetes.io/unschedulable:NoSchedule` taint，新 Pod 全部 Pending。

**根因**：升级流程中节点被 cordon 但未 uncordon，或升级失败后 taint 未清除。

**修复**：
``` bash
# 🟡 中风险：移除节点 taint
kubectl taint nodes <node> node.kubernetes.io/unschedulable:NoSchedule-
kubectl uncordon <node>
```

## 升级决策点

- **P0（立即处理）**：核心业务 Pod Pending 超过 10min，服务副本不足影响可用性
- **P1（30分钟内）**：非核心 Pod Pending，当前副本数仍可支撑业务
- **P2（下一工作日）**：批处理/测试 Pod Pending，不影响在线服务

## 面试要点

1. **Q: kube-scheduler 的调度流程是什么？**
   A: 分为 Filter（过滤）和 Score（评分）两阶段。Filter 排除不满足约束的节点（资源不足、taint、亲和性、PVC 拓扑等），Score 对剩余节点打分（LeastAllocated、BalancedAllocation、ImageLocality 等插件），选择最高分节点。最后 Bind 将 Pod 绑定到节点。

2. **Q: Pod Pending 的常见原因和排查思路？**
   A: ① `kubectl describe pod` 查看 Events 中的 FailedScheduling 消息；② 资源不足（Insufficient cpu/memory）→ 扩容或降低 request；③ taint/toleration 不匹配 → 添加 toleration；④ PVC 未绑定 → 检查 StorageClass/PV；⑤ 亲和性冲突 → 调整约束或增加节点。

3. **Q: 如何扩展 kube-scheduler？**
   A: 通过 Scheduling Framework 插件机制：在 Filter/Score/Reserve/Bind 等扩展点插入自定义逻辑。配置通过 KubeSchedulerConfiguration 的 profiles 字段定义。常见扩展：GPU 调度（nvidia device plugin）、容量调度（Capacity Scheduler）、Gang Scheduling（Coscheduling）。

## 相关概念

- [[22-概念/07-调度与资源/kube-scheduler.md|Kube Scheduler]] — Kubernetes 调度器原理、算法与扩展机制
- [[22-概念/08-可靠性与运维/node-lifecycle-management.md|节点生命周期管理]] — 节点注册、状态维护与驱逐机制
- [[22-概念/07-调度与资源/resource-management.md|资源管理]] — Kubernetes 资源请求、限制与配额管理

## Related

- [[visibility-public|#visibility/public Hub]] — tag hub


<!-- risk-assessed -->
