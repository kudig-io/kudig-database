---
title: apiserver × 滚动更新
summary: apiserver × 滚动更新：apiserver与滚动更新是Kubernetes生产环境中的两个关键维度。理解它们之间的交互关系对于构建稳定、可观测的集群至关重要。
category: synthesis
tags:
- cross-domain
- cluster
- release
tier: supporting
created: '2026-05-23'
updated: '2026-05-23'
last_updated: 2026-07
relationships:
- target: '[[实体/helm.md]]'
  type: uses
- target: '[[实体/prometheus.md]]'
  type: uses
- target: '[[实体/argocd.md]]'
  type: related_to
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。

# apiserver × 滚动更新

## 概述
滚动更新（Rolling Update）是 Deployment 和 StatefulSet 的核心发布策略。整个滚动更新过程由 apiserver 上的 Deployment Controller 驱动：Controller watch 到 Deployment 的 `spec.template` 变更后，按策略创建新 ReplicaSet 并逐步扩缩 Pod 副本数。apiserver 在这条链路中承担请求入口、状态仲裁和进度追踪三重角色——更新速度和可靠性高度依赖 apiserver 的响应性能和 etcd 的写入速度。

## 技术关联机制

1. **滚动更新的 API 驱动流程**：用户修改 Deployment 的 `spec.template`（如镜像版本）→ apiserver 接收 PATCH/PUT 请求并持久化 → Deployment Controller 通过 informer watch 到变更 → 计算新旧 ReplicaSet 的期望副本数 → 通过 apiserver 创建新 ReplicaSet 和 Pod、缩旧 ReplicaSet 的 Pod → ReplicaSet Controller 通过 apiserver 创建/删除 Pod → kube-scheduler 通过 apiserver watch 到新 Pod 执行调度。

2. **maxSurge 和 maxUnavailable**：这两个参数控制滚动更新的节奏。`maxSurge` 定义可以超出期望副本数的最大 Pod 数（如 25% 意味着 10 副本的 Deployment 最多同时有 12 个 Pod）。`maxUnavailable` 定义更新过程中允许不可用的 Pod 数。Controller 通过 apiserver 实时读取当前 Pod 数和 Ready 状态来决定下一步操作。如果 apiserver 的 status 回写延迟，Controller 可能做出错误的扩缩决策。

3. **Rollout 状态追踪**：Deployment 的 `status` 子资源包含 `updatedReplicas`、`readyReplicas`、`availableReplicas` 和 `conditions`（Progressing、Available、ReplicaFailure）。`kubectl rollout status` 通过轮询 apiserver 的 `GET /apis/apps/v1/.../deployments/<name>/status` 来判断发布是否完成。`progressDeadlineSeconds`（默认 600s）定义了超时阈值，超过后 Deployment 标记为 Progressing=False。

4. **Rollback 机制**：`kubectl rollout undo` 本质是将 Deployment 的 `spec.template` 回滚到旧 ReplicaSet 对应的版本。apiserver 接收 undo 操作后，Controller 识别到 template 变更，再次触发滚动更新（这次是从新版本回到旧版本）。保留的 ReplicaSet 历史版本数由 `revisionHistoryLimit` 控制（默认 10）。

## 实践场景

- **零停机应用发布**：配置 `maxSurge: 1, maxUnavailable: 0`，始终多一个 Pod 保证容量，实现无损滚动更新
- **快速发布**：配置 `maxSurge: 50%, maxUnavailable: 50%`，允许较激进的并行更新，适合无状态低流量服务
- **金丝雀集成**：先通过 `maxSurge: 1, maxUnavailable: 0` 创建 1 个新版本 Pod 验证健康，再逐步提高比例
- **自动回滚**：结合 Prometheus 指标和 Argo Rollouts，在滚动更新期间监测错误率，异常时自动触发 `kubectl rollout undo`

## 常见问题

### 问题1：滚动更新后新 Pod 持续 CrashLoopBackOff
**症状**：新版本 Pod 启动失败，但 maxUnavailable=0 保证旧 Pod 仍在运行
**根因**：新镜像配置错误/环境变量缺失/依赖服务不可达
**修复**：`kubectl logs <new-pod> --previous` 排查崩溃原因；执行 `kubectl rollout undo deployment/<name>`

### 问题2：rollout status 超过 progressDeadlineSeconds
**症状**：`kubectl rollout status` 报 `error: deployment "xxx" exceeded its progress deadline`
**根因**：新 Pod 的 readinessProbe 持续失败；或镜像拉取超时
**修复**：检查 Pod 的 Events 和 probe 配置；执行 `kubectl rollout undo` 回滚到稳定版本

### 问题3：大规模滚动更新触发 apiserver 限流
**症状**：500+ 副本的 Deployment 滚动更新时 Pod 创建缓慢，apiserver 报 429 Too Many Requests
**根因**：大量 Pod 创建请求触发了 APF（API Priority and Fairness）限流
**修复**：调整 Deployment 的 `maxSurge` 为较小值（如 10%）控制并发；配置 APF FlowSchema 为部署流量提升配额

## 关键命令

```bash
# 🟢 查看滚动更新状态
kubectl rollout status deployment/<name> -n <ns>

# 🟢 查看更新历史
kubectl rollout history deployment/<name> -n <ns>

# 🟢 查看 ReplicaSet 状态
kubectl get rs -n <ns> -l app=<name>

# 🟡 触发滚动更新
kubectl set image deployment/<name> <container>=<new-image> -n <ns>

# 🟡 回滚到上一个版本
kubectl rollout undo deployment/<name> -n <ns>

# 🟡 回滚到指定版本
kubectl rollout undo deployment/<name> --to-revision=<n> -n <ns>

# 🟢 查看 Deployment 详细 conditions
kubectl get deployment <name> -n <ns> -o jsonpath='{.status.conditions}'
```

## 权衡取舍

| 维度 | apiserver 倾向 | 滚动更新 倾向 | 权衡点 |
|------|---------------|-------------|--------|
| 更新速度 | 低并发减少 API 压力 | 高并发快速完成发布 | 集群稳定 vs 发布效率 |
| maxSurge | 低 surge 节省资源 | 高 surge 加速更新 | 资源成本 vs 发布速度 |
| maxUnavailable | 0 保证容量不受影响 | 高值加速旧 Pod 清理 | 可用性 vs 更新速度 |
| 历史版本 | 少保留节省 etcd 空间 | 多保留便于回滚 | 存储成本 vs 回滚灵活性 |

## 最佳实践
1. 生产环境配置 `maxUnavailable: 0` 确保滚动更新期间容量不减少
2. 设置合理的 `progressDeadlineSeconds`（如 300-600s），避免发布卡住无人发现
3. 配置 readinessProbe 确保 Pod 真正就绪后才接入流量
4. 保留足够的 `revisionHistoryLimit`（如 10）以支持多次回滚，但不要过大消耗 etcd 空间

## 工具推荐
- kubectl：基础诊断
- [[实体/helm.md|Helm]]/Kustomize：配置管理
- [[实体/prometheus.md|Prometheus]]/Grafana：联合监控
- [[实体/argocd.md|ArgoCD]]：GitOps同步

## 相关概念
- apiserver
- 滚动更新
## Related

- [[实体/kubernetes.md|Kubernetes (CNCF Graduated)]]
- [[实体/argo.md|Argo Workflows]]
- [[故障诊断/技能体系/skill-set/k8s-pvc-storage/DIALOGUE.md|DIALOGUE]]
- [[概念/etcd-×-PVC.md|etcd-×-PVC]]
- [[概念/apiserver-×-Pod诊断.md|apiserver-×-Pod诊断]]
- [[概念/etcd-×-灾难恢复.md|etcd-×-灾难恢复]]


<!-- risk-assessed -->
