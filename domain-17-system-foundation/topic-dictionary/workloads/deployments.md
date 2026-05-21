---
title: Deployments
description: '## 概述'
category: dictionary
tags:
- k8s
- glossary
- terminology
- prometheus
- hpa
- pdb
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Deployments 是什么
- 如何 Deployments
trigger_keywords:
- Deployments
- dictionary
prerequisites:
- kubectl-basics
- pod-lifecycle
- cloud-provider-basics
- prometheus-basics
---

# Deployments

## 概述
Deployment 为 Pod 和 ReplicaSet 提供声明式更新能力。用户描述期望状态，Deployment 控制器以受控速率将实际状态变更为期望状态。它是 Kubernetes 中管理无状态应用最常用的工作负载资源。

## 核心概念/原理
- **Pod 模板（`.spec.template`）**：定义 Pod 的规格，必须包含与应用选择器匹配的标签；`restartPolicy` 只能为 `Always`。
- **选择器（`.spec.selector`）**：标签选择器，用于识别 Deployment 管理的 Pod。创建后不可变。
- **副本数（`.spec.replicas`）**：期望运行的 Pod 数量，默认为 1。若由 HPA 管理，应避免在清单中硬编码该字段。
- **更新策略（`.spec.strategy`）**：
  - `RollingUpdate`（默认）：逐步创建新 Pod、删除旧 Pod。可配置 `maxSurge`（最大可超出副本数）和 `maxUnavailable`（最大不可用副本数），默认均为 25%。
  - `Recreate`：先删除所有旧 Pod，再创建新 Pod。
- **进度截止时间（`.spec.progressDeadlineSeconds`）**：默认 600 秒。若在此时间内未推进完成，Deployment 状态会标记为 `ProgressDeadlineExceeded`。
- **最小就绪时间（`.spec.minReadySeconds`）**：新 Pod 就绪后需持续 healthy 的最短时间，才被视为可用。
- **修订历史限制（`.spec.revisionHistoryLimit`）**：保留的旧 ReplicaSet 数量，默认 10，用于回滚。

## 关键机制或特性
- **版本管理**：每次修改 Pod 模板都会创建一个新的 ReplicaSet 作为修订版本。旧版本保留以便回滚。
- **回滚**：支持 `kubectl rollout undo` 回滚到上一版本或指定版本。
- **暂停/恢复**：`kubectl rollout pause` 可暂停滚动更新，允许累积多个修改后一次性生效。
- **比例缩放（Proportional Scaling）**：在滚动更新过程中收到扩缩容请求时，控制器会按现有活跃 ReplicaSet 的比例分配新增/减少的副本。
- **终止副本追踪（Beta）**：`DeploymentReplicaSetTerminatingReplicas` 特性门控启用后，可通过 `.status.terminatingReplicas` 查看处于终止状态的副本数。

## 使用场景
- 无状态 Web 应用和 API 服务的部署与更新。
- 需要零停机滚动发布和快速回滚能力的场景。
- 配合 HPA 实现自动水平扩缩容。

## 最佳实践/注意事项
- 确保选择器与 Pod 模板的标签匹配，且不要与其他控制器重叠。
- 若使用 HPA，建议从 manifest 中移除 `spec.replicas`，避免 `kubectl apply` 与 HPA 发生冲突。
- 为需要长时间预热的服务设置合理的 `minReadySeconds` 和 readiness probe。
- 设置合适的 `progressDeadlineSeconds` 以便及时发现卡住的发布。
- 注意 `maxSurge` 和 `maxUnavailable` 的配置对资源消耗和可用性的影响。

## 实战 YAML 示例

以下为生产级 Deployment 配置，包含滚动更新策略、探针、PDB 和资源管理：

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: web-api
  namespace: prod
  labels:
    app: web-api
    version: v2.1.0
  annotations:
    kubernetes.io/change-cause: "升级至 v2.1.0: 修复连接池泄漏"  # 记录变更原因
spec:
  replicas: 3
  revisionHistoryLimit: 5                    # 保留 5 个历史版本用于回滚
  progressDeadlineSeconds: 300               # 5 分钟内必须完成发布
  minReadySeconds: 30                        # Pod 就绪 30 秒后才算可用
  strategy:
    type: RollingUpdate
    rollingUpdate:
      maxSurge: 1                            # 最多多 1 个 Pod
      maxUnavailable: 0                      # 不允许任何 Pod 不可用（零停机）
  selector:
    matchLabels:
      app: web-api
  template:
    metadata:
      labels:
        app: web-api
        version: v2.1.0
      annotations:
        prometheus.io/scrape: "true"
        prometheus.io/port: "8080"
    spec:
      terminationGracePeriodSeconds: 60
      serviceAccountName: web-api-sa
      securityContext:
        runAsNonRoot: true
        runAsUser: 1000
        seccompProfile:
          type: RuntimeDefault
      containers:
      - name: api
        image: myregistry.com/web-api:v2.1.0
        ports:
        - containerPort: 8080
          name: http
        resources:
          requests:
            cpu: "500m"
            memory: "512Mi"
          limits:
            cpu: "2000m"
            memory: "1Gi"
        startupProbe:
          httpGet:
            path: /healthz
            port: http
          periodSeconds: 5
          failureThreshold: 30
        livenessProbe:
          httpGet:
            path: /healthz
            port: http
          periodSeconds: 15
          timeoutSeconds: 3
        readinessProbe:
          httpGet:
            path: /ready
            port: http
          periodSeconds: 10
          timeoutSeconds: 3
        lifecycle:
          preStop:
            exec:
              command: ["/bin/sh", "-c", "sleep 10"]  # 等待 LB 摘流量
      affinity:
        podAntiAffinity:
          requiredDuringSchedulingIgnoredDuringExecution:
          - labelSelector:
              matchLabels:
                app: web-api
            topologyKey: kubernetes.io/hostname     # 每个节点最多一个副本
      topologySpreadConstraints:
      - maxSkew: 1
        topologyKey: topology.kubernetes.io/zone
        whenUnsatisfiable: DoNotSchedule
        labelSelector:
          matchLabels:
            app: web-api
---
apiVersion: policy/v1
kind: PodDisruptionBudget
metadata:
  name: web-api-pdb
  namespace: prod
spec:
  minAvailable: 2                             # 任何时候至少保持 2 个可用副本
  selector:
    matchLabels:
      app: web-api
```

## 故障排查

### 滚动更新卡住不动
- **症状**: `kubectl rollout status` 显示 `Waiting for deployment "xxx" rollout to finish`，长时间无进展。
- **常见原因**: 新 Pod 的 Readiness Probe 持续失败、镜像拉取失败、资源不足无法调度新 Pod。
- **诊断命令**:
  ```bash
  # 查看 Deployment 状态条件
  kubectl get deployment web-api -n prod -o jsonpath='{.status.conditions[*].message}'
  # 查看新 ReplicaSet 下的 Pod 状态
  kubectl get rs -n prod -l app=web-api --sort-by=.metadata.creationTimestamp
  kubectl get pods -n prod -l app=web-api | grep -v Running
  # 查看 Pod 事件
  kubectl describe pod <new-pod-name> -n prod | tail -20
  ```
- **解决方案**: 修复应用问题后等待自动恢复，或 `kubectl rollout undo` 回滚到上一版本。

### 发布后出现 5xx 错误
- **症状**: 滚动更新完成后短暂出现 HTTP 5xx 错误。
- **常见原因**: `preStop` 钩子未配置，旧 Pod 被终止时仍在接收流量；`minReadySeconds` 设置过小。
- **诊断命令**:
  ```bash
  # 检查 Deployment 是否配置了 preStop 和 minReadySeconds
  kubectl get deployment web-api -n prod -o jsonpath='{.spec.template.spec.containers[0].lifecycle}'
  kubectl get deployment web-api -n prod -o jsonpath='{.spec.minReadySeconds}'
  ```
- **解决方案**: 配置 `preStop` 钩子（如 `sleep 10`），设置合理的 `minReadySeconds`。

### ReplicaSet 过多导致 API 压力
- **症状**: `kubectl get rs` 显示大量旧 ReplicaSet。
- **诊断命令**:
  ```bash
  kubectl get rs -n prod -l app=web-api --no-headers | wc -l
  ```
- **解决方案**: 设置合理的 `revisionHistoryLimit`（生产建议 3-5），清理无用的旧 ReplicaSet。

## 生产检查清单

- [ ] `strategy.rollingUpdate.maxUnavailable: 0` 确保零停机部署
- [ ] `minReadySeconds` >= 30 秒，防止新 Pod 未完全预热即承担流量
- [ ] `progressDeadlineSeconds` 设置合理（建议 300-600 秒）
- [ ] `revisionHistoryLimit` 设置为 3-5，避免 ReplicaSet 堆积
- [ ] `PodDisruptionBudget` 已创建，保障节点维护和集群升级时的可用性
- [ ] `kubernetes.io/change-cause` 注解记录每次变更原因
- [ ] `podAntiAffinity` 确保副本分散到不同节点
- [ ] `topologySpreadConstraints` 确保跨可用区分布
- [ ] 所有探针已正确配置（startupProbe / livenessProbe / readinessProbe）
- [ ] `preStop` 生命周期钩子确保优雅终止
- [ ] HPA 场景下不硬编码 `spec.replicas`

## 命令快速参考

```bash
# 查看 Deployment 状态
kubectl rollout status deployment/web-api -n prod

# 查看发布历史
kubectl rollout history deployment/web-api -n prod

# 回滚到上一版本
kubectl rollout undo deployment/web-api -n prod

# 回滚到指定版本
kubectl rollout undo deployment/web-api -n prod --to-revision=3

# 暂停滚动更新（累积修改后再生效）
kubectl rollout pause deployment/web-api -n prod

# 恢复滚动更新
kubectl rollout resume deployment/web-api -n prod

# 手动扩缩容
kubectl scale deployment/web-api -n prod --replicas=5

# 查看关联的 ReplicaSet
kubectl get rs -n prod -l app=web-api --sort-by=.metadata.creationTimestamp
```

## 交叉引用

- [Deployment 生产模式详解](../../domain-02-workloads-applications/02-deployment-production-patterns.md)
- [工作负载概览与架构](../../domain-02-workloads-applications/01-workload-overview-architecture.md)
- [工作负载监控与告警](../../domain-02-workloads-applications/06-workload-monitoring-alerting.md)
- [工作负载故障排查手册](../../domain-02-workloads-applications/07-workload-troubleshooting-handbook.md)
- [Deployment 故障树分析 (FTA)](../../domain-10-troubleshooting-diagnostics/topic-fta/list/deployment-fta.md)
- [HPA 水平自动扩缩](./horizontal-pod-autoscaling.md)

## 参考链接
- https://kubernetes.io/docs/concepts/workloads/controllers/deployment/

## Related

- [[domain-19-landscape-references/topic-index/gitops-cicd-index|GitOps / CI-CD 全局索引]]
