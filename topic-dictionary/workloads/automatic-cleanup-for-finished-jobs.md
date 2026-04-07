# Automatic Cleanup for Finished Jobs

## 概述
TTL-after-finished 控制器为已完成的 Job 对象提供基于生存时间（TTL）的自动清理机制。它有助于减少 API Server 中已完成 Job 的累积，降低 etcd 压力。

## 核心概念/原理
- **触发时机**：计时器在 Job 状态变为 `Complete` 或 `Failed` 时开始计时。
- **级联删除**：TTL 到期后，控制器会自动删除 Job 及其依赖对象（如 Pod），并遵守对象的 finalizers 等生命周期保证。
- **配置字段**：在 Job 的 `spec.ttlSecondsAfterFinished` 字段中指定 TTL 秒数。

## 关键机制或特性
- **动态修改**：可以在 Job 创建后或完成后修改 `ttlSecondsAfterFinished` 字段，但若在原有 TTL 已过期后再延长，Kubernetes 不保证一定保留该 Job。
- **时间偏差敏感**：TTL 控制器依赖 Job 状态中的时间戳判断 TTL 是否到期，集群时钟偏差可能导致清理时间出现偏差。
- **多种设置方式**：
  - 在 Job 清单中直接声明。
  - 为已完成的 Job 手动设置。
  - 通过 mutating admission webhook 动态注入。
  - 编写自定义控制器按策略管理 TTL。

## 使用场景
- 大规模批处理平台中自动清理已成功或失败的临时 Job。
- 与 CronJob 配合，管理周期性任务产生的历史 Job（但 CronJob 本身也有 history limit）。
- 需要按完成状态设置不同保留策略的场景（可通过 webhook 实现）。

## 最佳实践/注意事项
- 建议为直接创建的 Job（ unmanaged jobs ）设置 `ttlSecondsAfterFinished`，因为默认删除策略可能导致 Pod 在 Job 删除后残留。
- 设置非零 TTL 时，注意集群时钟同步，避免意外提前或延迟清理。
- 若需要长期保留 Job 状态供审计使用，应使用外部日志/审计系统，而非依赖 Kubernetes API 对象。

## 生产 YAML 示例

### 基本 TTL 配置

```yaml
apiVersion: batch/v1
kind: Job
metadata:
  name: data-export-20260407
  namespace: batch-jobs
spec:
  ttlSecondsAfterFinished: 3600    # 完成后 1 小时自动清理
  template:
    spec:
      containers:
      - name: exporter
        image: registry.example.com/tools/data-exporter:v3.2
        command: ["python", "export.py", "--date=2026-04-07"]
        resources:
          requests:
            cpu: "500m"
            memory: "512Mi"
          limits:
            cpu: "1"
            memory: "1Gi"
      restartPolicy: Never
  backoffLimit: 3
```

### CronJob 结合 TTL 清理

```yaml
apiVersion: batch/v1
kind: CronJob
metadata:
  name: nightly-report
  namespace: analytics
spec:
  schedule: "0 2 * * *"
  jobTemplate:
    spec:
      ttlSecondsAfterFinished: 86400    # 保留 24 小时后清理
      template:
        spec:
          containers:
          - name: reporter
            image: registry.example.com/analytics/reporter:v1.5
          restartPolicy: OnFailure
      backoffLimit: 2
  successfulJobsHistoryLimit: 3
  failedJobsHistoryLimit: 5
```

### 通过 Mutating Webhook 统一注入 TTL

```yaml
# 示例 webhook 配置，为所有没有设置 TTL 的 Job 注入默认值
apiVersion: admissionregistration.k8s.io/v1
kind: MutatingWebhookConfiguration
metadata:
  name: job-ttl-injector
webhooks:
- name: ttl.jobs.example.com
  rules:
  - apiGroups: ["batch"]
    apiVersions: ["v1"]
    operations: ["CREATE"]
    resources: ["jobs"]
  clientConfig:
    service:
      name: job-ttl-webhook
      namespace: system
      path: /inject-ttl
  admissionReviewVersions: ["v1"]
  sideEffects: None
  # webhook 逻辑：if not spec.ttlSecondsAfterFinished → patch 为 7200（2 小时）
```

## TTL 行为对照表

| 配置值 | 行为 | 适用场景 |
|--------|------|----------|
| `0` | Job 完成后立即删除（含关联 Pod） | 一次性任务，无需保留历史 |
| `3600` | 完成后 1 小时删除 | 保留短暂的调试窗口 |
| `86400` | 完成后 24 小时删除 | 需要隔天检查结果 |
| 未设置 | 永不自动清理 | 需手动管理或外部清理策略 |
| 完成后修改 | 从修改时刻起生效，但不保证原 TTL 已过期的 Job 仍存在 | 紧急延长保留期 |

## 故障排查

| 症状 | 可能原因 | 排查步骤 |
|------|----------|----------|
| 已完成 Job 未被清理 | TTL 控制器未启用或 `ttlSecondsAfterFinished` 未设置 | `kubectl get job -o jsonpath='{.spec.ttlSecondsAfterFinished}'` 检查字段 |
| Job 被提前删除 | 集群节点时钟不同步导致 TTL 计算偏差 | 检查 NTP 同步状态；`kubectl get job -o jsonpath='{.status.completionTime}'` |
| Job 删除后 Pod 仍残留 | Pod 的 ownerReference 丢失或被手动修改 | `kubectl get pods -l job-name=<name>` 检查孤儿 Pod |
| CronJob 历史 Job 堆积 | `successfulJobsHistoryLimit` 设置过高且未配置 TTL | 同时配置 `historyLimit` 和 `ttlSecondsAfterFinished` |

## 生产检查清单

- [ ] 所有直接创建的 Job 已设置 `ttlSecondsAfterFinished`
- [ ] CronJob 的 `jobTemplate` 中同时配置 TTL 和 historyLimit
- [ ] 集群节点 NTP 时钟同步正常（偏差 < 1 秒）
- [ ] 审计需求已通过外部日志系统（ELK/Loki）满足，不依赖 API 对象留存
- [ ] 监控 etcd 中 Job 对象数量，设置阈值告警
- [ ] 为大批量 Job 场景考虑 Mutating Webhook 统一注入默认 TTL

## 命令快速参考

```bash
# 查看所有已完成但未清理的 Job
kubectl get jobs --field-selector=status.successful=1 -A

# 查看 Job 的 TTL 配置和完成时间
kubectl get job <name> -o jsonpath='TTL={.spec.ttlSecondsAfterFinished} Completed={.status.completionTime}'

# 手动为已完成 Job 追加 TTL（保留 2 小时）
kubectl patch job <name> -p '{"spec":{"ttlSecondsAfterFinished":7200}}'

# 立即清理：设置 TTL 为 0
kubectl patch job <name> -p '{"spec":{"ttlSecondsAfterFinished":0}}'

# 批量清理命名空间内所有已完成的 Job
kubectl delete jobs --field-selector=status.successful=1 -n <namespace>

# 检查 etcd 中 Job 对象数量（需要 etcd 访问权限）
kubectl get jobs -A --no-headers | wc -l
```

## 交叉引用

- [Jobs](jobs.md) — Job 控制器的完整生命周期管理
- [CronJob](cronjob.md) — 周期性 Job 与 historyLimit 配置
- [Disruptions](disruptions.md) — Pod 中断预算与 Job 的交互
- [工作负载管理](managing-workloads.md) — 批量管理和清理资源的实践

## 参考链接
- https://kubernetes.io/docs/concepts/workloads/controllers/ttlafterfinished/
