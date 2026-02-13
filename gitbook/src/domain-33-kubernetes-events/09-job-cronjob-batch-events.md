# 09 - Job 与 CronJob 批处理事件

> **适用版本**: Kubernetes v1.25 - v1.32 | **最后更新**: 2026-02 | **作者**: Allen Galler

> **本文档详细记录 Job 和 CronJob 控制器产生的所有批处理相关事件。**

---

## 📋 目录

- [事件索引表](#事件索引表)
- [Job 控制器事件](#job-控制器事件)
- [CronJob 控制器事件](#cronjob-控制器事件)
- [批处理执行生命周期](#批处理执行生命周期)
- [深度分析](#深度分析)
- [故障排查模式](#故障排查模式)
- [相关参考](#相关参考)

---

## 事件索引表

### Job Controller Events

| Event Reason | Type | 频率 | 起始版本 | 描述 |
|--------------|------|------|----------|------|
| SuccessfulCreate | Normal | 高频 | v1.0+ | 成功创建 Pod |
| SuccessfulDelete | Normal | 中频 | v1.0+ | 成功删除 Pod |
| FailedCreate | Warning | 中频 | v1.0+ | 创建 Pod 失败 |
| Completed | Normal | 高频 | v1.0+ | Job 完成 |
| BackoffLimitExceeded | Warning | 中频 | v1.0+ | 达到重试上限 |
| DeadlineExceeded | Warning | 低频 | v1.2+ | 超过活跃截止时间 |
| TooManyActivePods | Warning | 罕见 | v1.3+ | 活跃 Pod 过多 |
| TooManySucceededPods | Warning | 罕见 | v1.3+ | 成功 Pod 过多 |
| Suspended | Normal | 低频 | v1.22+ | Job 已暂停 |
| Resumed | Normal | 低频 | v1.22+ | Job 已恢复 |
| FailedJob | Warning | 低频 | v1.26+ | Indexed Job 失败 |
| SuccessCriteriaMet | Normal | 低频 | v1.28+ | 满足成功策略 |

### CronJob Controller Events

| Event Reason | Type | 频率 | 起始版本 | 描述 |
|--------------|------|------|----------|------|
| SuccessfulCreate | Normal | 高频 | v1.4+ | 成功创建 Job |
| SuccessfulDelete | Normal | 中频 | v1.4+ | 成功删除 Job |
| SawCompletedJob | Normal | 高频 | v1.4+ | 发现已完成 Job |
| UnexpectedJob | Warning | 罕见 | v1.4+ | 发现未预期的 Job |
| MissingJob | Normal | 罕见 | v1.4+ | 预期但未找到 Job |
| TooManyMissedTimes | Warning | 低频 | v1.4+ | 错过太多执行时间 |
| FailedCreate | Warning | 中频 | v1.4+ | 创建 Job 失败 |
| ForbidConcurrent | Warning | 低频 | v1.4+ | 并发策略禁止 |

---

## Job 控制器事件

### 1. SuccessfulCreate (Pod 创建成功)

**事件模板:**
```yaml
Type: Normal
Reason: SuccessfulCreate
Message: "Created pod: <pod-name>"
Source: job-controller
First Seen: 2026-02-10T10:00:00Z
Last Seen: 2026-02-10T10:00:00Z
Count: 1
```

**触发条件:**
- Job 控制器成功创建新的 Pod 副本
- Job 并行度要求创建多个 Pod
- Pod 失败后重新创建（在 backoffLimit 内）

**字段详解:**
- `<pod-name>`: 新创建的 Pod 名称，格式 `<job-name>-<random>`

**版本信息:**
- **起始版本**: v1.0
- **最后变更**: v1.24（Indexed Jobs 增强）

**生产影响:**
- ✅ 正常执行流程
- 📊 可用于追踪 Job 执行进度
- 🔍 配合 `.spec.completions` 监控完成度

**观测示例:**
```bash
# 查看 Job Pod 创建事件
kubectl describe job batch-processor | grep SuccessfulCreate

# 统计已创建 Pod 数量
kubectl get pods -l job-name=batch-processor --no-headers | wc -l
```

**关联配置:**
```yaml
apiVersion: batch/v1
kind: Job
metadata:
  name: batch-processor
spec:
  completions: 5      # 需要成功完成 5 个 Pod
  parallelism: 2      # 并行运行 2 个 Pod
  backoffLimit: 3     # 最多重试 3 次
  template:
    spec:
      containers:
      - name: worker
        image: busybox
        command: ["sh", "-c", "echo Processing && sleep 10"]
      restartPolicy: Never
```

---

### 2. SuccessfulDelete (Pod 删除成功)

**事件模板:**
```yaml
Type: Normal
Reason: SuccessfulDelete
Message: "Deleted pod: <pod-name>"
Source: job-controller
First Seen: 2026-02-10T10:05:00Z
Last Seen: 2026-02-10T10:05:00Z
Count: 1
```

**触发条件:**
- Job 完成后清理 Pod（根据 `ttlSecondsAfterFinished`）
- Job 被删除时级联删除 Pod
- 手动删除 Job 时清理

**字段详解:**
- `<pod-name>`: 被删除的 Pod 名称

**版本信息:**
- **起始版本**: v1.0
- **最后变更**: v1.21（TTL 清理增强）

**生产影响:**
- ✅ 资源自动清理
- 🗑️ 防止 Pod 堆积
- ⚠️ 如果频繁出现可能是 Job 被意外删除

**观测示例:**
```bash
# 查看 Pod 删除事件
kubectl describe job batch-processor | grep SuccessfulDelete

# 检查 TTL 配置
kubectl get job batch-processor -o jsonpath='{.spec.ttlSecondsAfterFinished}'
```

**关联配置:**
```yaml
apiVersion: batch/v1
kind: Job
metadata:
  name: batch-processor
spec:
  ttlSecondsAfterFinished: 300  # 完成后 5 分钟自动删除
  template:
    spec:
      containers:
      - name: worker
        image: busybox
      restartPolicy: Never
```

---

### 3. FailedCreate (Pod 创建失败)

**事件模板:**
```yaml
Type: Warning
Reason: FailedCreate
Message: "Error creating: pods \"<pod-name>\" is forbidden: exceeded quota: compute-resources"
Source: job-controller
First Seen: 2026-02-10T10:00:00Z
Last Seen: 2026-02-10T10:00:30Z
Count: 5
```

**触发条件:**
- ResourceQuota 限制导致无法创建 Pod
- Pod 安全策略阻止创建
- 节点资源不足（间接原因）
- 镜像拉取策略问题

**常见错误消息:**
```
# 配额超限
Error creating: pods "xxx" is forbidden: exceeded quota: compute-resources

# 安全策略阻止
Error creating: pods "xxx" is forbidden: violates PodSecurity "restricted:latest"

# 服务账号问题
Error creating: pods "xxx" is forbidden: error looking up service account default/job-sa

# 节点选择器问题
Error creating: No nodes are available that match all of the following predicates
```

**版本信息:**
- **起始版本**: v1.0
- **最后变更**: v1.25（PSS 错误信息改进）

**生产影响:**
- ⛔ **严重**: Job 无法执行
- 🚨 需要立即介入
- 📈 可能导致 CronJob 积压

**故障排查:**
```bash
# 1. 检查详细错误
kubectl describe job <job-name>

# 2. 检查 ResourceQuota
kubectl get resourcequota -A
kubectl describe resourcequota <quota-name> -n <namespace>

# 3. 检查 Pod 安全策略
kubectl get psp
kubectl auth can-i use podsecuritypolicies/<psp-name> --as=system:serviceaccount:<namespace>:<sa>

# 4. 检查服务账号
kubectl get sa <sa-name> -n <namespace>

# 5. 模拟 Pod 创建
kubectl run test-pod --image=busybox --dry-run=server
```

**修复方案:**
```yaml
# 方案 1: 调整 ResourceQuota
apiVersion: v1
kind: ResourceQuota
metadata:
  name: compute-quota
spec:
  hard:
    requests.cpu: "100"      # 增加配额
    requests.memory: "200Gi"

# 方案 2: 降低 Job 资源请求
apiVersion: batch/v1
kind: Job
spec:
  template:
    spec:
      containers:
      - name: worker
        resources:
          requests:
            cpu: "100m"      # 降低资源请求
            memory: "128Mi"

# 方案 3: 调整 Pod 安全上下文
apiVersion: batch/v1
kind: Job
spec:
  template:
    spec:
      securityContext:
        runAsNonRoot: true
        runAsUser: 1000
        seccompProfile:
          type: RuntimeDefault
```

---

### 4. Completed (Job 完成)

**事件模板:**
```yaml
Type: Normal
Reason: Completed
Message: "Job completed"
Source: job-controller
First Seen: 2026-02-10T10:10:00Z
Last Seen: 2026-02-10T10:10:00Z
Count: 1
```

**触发条件:**
- 成功完成的 Pod 数量达到 `.spec.completions`
- 所有必需的 Pod 成功执行
- 满足 `successPolicy` 条件（v1.28+）

**字段详解:**
- 无附加字段，简单完成通知

**版本信息:**
- **起始版本**: v1.0
- **最后变更**: v1.28（successPolicy 支持）

**生产影响:**
- ✅ Job 成功完成
- 📊 可用于监控和告警
- 🔄 触发后续流程（如 CronJob 下次调度）

**观测示例:**
```bash
# 查看 Job 完成状态
kubectl get job batch-processor -o jsonpath='{.status.conditions[?(@.type=="Complete")].status}'

# 查看完成时间
kubectl get job batch-processor -o jsonpath='{.status.completionTime}'

# 查看成功 Pod 数量
kubectl get job batch-processor -o jsonpath='{.status.succeeded}'
```

**状态检查:**
```yaml
# Job 完成后的状态
status:
  conditions:
  - type: Complete
    status: "True"
    lastProbeTime: 2026-02-10T10:10:00Z
    lastTransitionTime: 2026-02-10T10:10:00Z
  succeeded: 5              # 成功 Pod 数量
  completionTime: 2026-02-10T10:10:00Z
  startTime: 2026-02-10T10:00:00Z
```

---

### 5. BackoffLimitExceeded (重试上限已达)

**事件模板:**
```yaml
Type: Warning
Reason: BackoffLimitExceeded
Message: "Job has reached the specified backoff limit"
Source: job-controller
First Seen: 2026-02-10T10:15:00Z
Last Seen: 2026-02-10T10:15:00Z
Count: 1
```

**触发条件:**
- Pod 失败次数达到 `.spec.backoffLimit`（默认 6）
- 重试间隔采用指数退避算法
- Job 最终标记为失败

**字段详解:**
- 无附加字段，表示重试耗尽

**版本信息:**
- **起始版本**: v1.0
- **最后变更**: v1.26（Pod Failure Policy 支持）

**生产影响:**
- ⛔ **严重**: Job 永久失败
- 🚨 需要人工介入分析原因
- 📊 影响 CronJob 下次执行

**重试机制说明:**
```
Pod 失败次数与退避时间:
  失败 1 次: 10s 后重试
  失败 2 次: 20s 后重试
  失败 3 次: 40s 后重试
  失败 4 次: 80s 后重试
  失败 5 次: 160s 后重试
  失败 6 次: 达到 backoffLimit，Job 失败

最大退避时间: 6 分钟
```

**观测示例:**
```bash
# 查看失败原因
kubectl describe job batch-processor

# 查看失败 Pod 日志
kubectl logs -l job-name=batch-processor --tail=100

# 查看失败 Pod 状态
kubectl get pods -l job-name=batch-processor -o wide

# 查看重试次数
kubectl get job batch-processor -o jsonpath='{.status.failed}'
```

**深度分析:**
```bash
# 分析所有失败 Pod 的退出码
kubectl get pods -l job-name=batch-processor -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{.status.containerStatuses[0].state.terminated.exitCode}{"\n"}{end}'

# 查看 Pod 失败时间线
kubectl get events --field-selector involvedObject.kind=Pod --sort-by='.lastTimestamp' | grep batch-processor
```

**故障排查模式:**
```yaml
# 常见失败原因与解决方案

# 1. 退出码 1 - 应用程序错误
status:
  containerStatuses:
  - state:
      terminated:
        exitCode: 1
        reason: Error
# 解决: 检查应用日志，修复代码逻辑

# 2. 退出码 137 - 内存 OOM
status:
  containerStatuses:
  - state:
      terminated:
        exitCode: 137
        reason: OOMKilled
# 解决: 增加内存限制或优化内存使用

# 3. 退出码 143 - SIGTERM 终止
status:
  containerStatuses:
  - state:
      terminated:
        exitCode: 143
        reason: Error
# 解决: 检查 activeDeadlineSeconds 配置
```

**配置优化:**
```yaml
# 方案 1: 增加重试次数
apiVersion: batch/v1
kind: Job
metadata:
  name: batch-processor
spec:
  backoffLimit: 10          # 允许 10 次重试
  template:
    spec:
      containers:
      - name: worker
        image: myapp:v1

# 方案 2: 使用 Pod Failure Policy (v1.26+)
apiVersion: batch/v1
kind: Job
metadata:
  name: batch-processor
spec:
  backoffLimit: 6
  podFailurePolicy:
    rules:
    - action: FailJob      # 立即失败，不重试
      onExitCodes:
        containerName: worker
        operator: In
        values: [42]       # 业务逻辑错误
    - action: Ignore       # 忽略此类失败，不计入 backoffLimit
      onExitCodes:
        containerName: worker
        operator: In
        values: [2]        # 临时网络错误
    - action: Count        # 计入 backoffLimit 但继续重试
      onPodConditions:
      - type: DisruptionTarget
  template:
    spec:
      containers:
      - name: worker
        image: myapp:v1

# 方案 3: 结合 activeDeadlineSeconds
apiVersion: batch/v1
kind: Job
metadata:
  name: batch-processor
spec:
  backoffLimit: 6
  activeDeadlineSeconds: 1800  # 30 分钟总超时
  template:
    spec:
      containers:
      - name: worker
        image: myapp:v1
```

---

### 6. DeadlineExceeded (活跃截止时间超时)

**事件模板:**
```yaml
Type: Warning
Reason: DeadlineExceeded
Message: "Job was active longer than specified deadline"
Source: job-controller
First Seen: 2026-02-10T10:30:00Z
Last Seen: 2026-02-10T10:30:00Z
Count: 1
```

**触发条件:**
- Job 运行时间超过 `.spec.activeDeadlineSeconds`
- 从 Job 开始执行到超时时间到达
- 所有运行中的 Pod 将被终止

**字段详解:**
- 无附加字段，表示总超时

**版本信息:**
- **起始版本**: v1.2
- **最后变更**: v1.21（清理增强）

**生产影响:**
- ⛔ **严重**: Job 被强制终止
- 🚨 可能导致数据不一致
- ⏱️ 需要评估合理的超时时间

**观测示例:**
```bash
# 查看 Job 运行时长
kubectl get job batch-processor -o jsonpath='{.status.startTime}'
kubectl get job batch-processor -o jsonpath='{.status.completionTime}'

# 查看超时配置
kubectl get job batch-processor -o jsonpath='{.spec.activeDeadlineSeconds}'
```

**配置建议:**
```yaml
# 示例: 批量数据处理 Job
apiVersion: batch/v1
kind: Job
metadata:
  name: data-import
spec:
  activeDeadlineSeconds: 3600  # 1 小时超时
  backoffLimit: 3
  template:
    spec:
      containers:
      - name: importer
        image: data-importer:v1
        resources:
          limits:
            cpu: "2"
            memory: "4Gi"
      restartPolicy: Never

# 计算公式:
# activeDeadlineSeconds = (单次执行时间 × completions × 安全系数) + 启动开销
# 示例: (600s × 5 × 1.5) + 300s = 4800s
```

---

### 7. TooManyActivePods (活跃 Pod 过多)

**事件模板:**
```yaml
Type: Warning
Reason: TooManyActivePods
Message: "too many active pods running for the job"
Source: job-controller
First Seen: 2026-02-10T10:00:00Z
Last Seen: 2026-02-10T10:05:00Z
Count: 10
```

**触发条件:**
- 活跃 Pod 数量超过 `.spec.parallelism`
- 控制器异常导致 Pod 创建失控
- 外部直接创建了相同 label 的 Pod

**版本信息:**
- **起始版本**: v1.3
- **触发场景**: 罕见（通常是 Bug）

**生产影响:**
- ⚠️ **中等**: 可能导致资源浪费
- 🔍 需要检查控制器健康状态
- 🐛 可能是系统 Bug

**故障排查:**
```bash
# 检查活跃 Pod 数量
kubectl get pods -l job-name=batch-processor --field-selector=status.phase=Running --no-headers | wc -l

# 检查 parallelism 配置
kubectl get job batch-processor -o jsonpath='{.spec.parallelism}'

# 检查 Job Controller 日志
kubectl logs -n kube-system -l component=kube-controller-manager --tail=200 | grep job-controller
```

---

### 8. TooManySucceededPods (成功 Pod 过多)

**事件模板:**
```yaml
Type: Warning
Reason: TooManySucceededPods
Message: "too many succeeded pods running for the job"
Source: job-controller
First Seen: 2026-02-10T10:10:00Z
Last Seen: 2026-02-10T10:10:00Z
Count: 1
```

**触发条件:**
- 成功 Pod 数量超过 `.spec.completions`
- 外部手动创建了额外的 Pod 并成功完成
- 控制器状态同步问题

**版本信息:**
- **起始版本**: v1.3
- **触发场景**: 罕见（异常情况）

**生产影响:**
- ⚠️ **低**: 通常不影响功能
- 🔍 表示可能存在同步问题
- 🗑️ 可能导致资源清理延迟

---

### 9. Suspended (Job 已暂停)

**事件模板:**
```yaml
Type: Normal
Reason: Suspended
Message: "Job suspended"
Source: job-controller
First Seen: 2026-02-10T10:00:00Z
Last Seen: 2026-02-10T10:00:00Z
Count: 1
```

**触发条件:**
- `.spec.suspend` 设置为 `true`
- 所有活跃 Pod 被删除
- Job 保持暂停状态直到恢复

**字段详解:**
- 无附加字段，简单暂停通知

**版本信息:**
- **起始版本**: v1.22 (Beta)
- **GA 版本**: v1.24

**生产影响:**
- ✅ 正常操作流程
- ⏸️ 用于临时停止执行
- 💾 保留 Job 状态和配置

**使用场景:**
```bash
# 1. 暂停 Job
kubectl patch job batch-processor -p '{"spec":{"suspend":true}}'

# 2. 检查暂停状态
kubectl get job batch-processor -o jsonpath='{.spec.suspend}'

# 3. 查看活跃 Pod（应为 0）
kubectl get pods -l job-name=batch-processor --field-selector=status.phase=Running
```

**应用场景:**
```yaml
# 场景 1: 维护窗口期暂停批处理
apiVersion: batch/v1
kind: Job
metadata:
  name: data-sync
spec:
  suspend: true          # 创建时即暂停
  completions: 100
  parallelism: 10
  template:
    spec:
      containers:
      - name: syncer
        image: data-sync:v1

# 场景 2: 动态流量控制
# 高峰期暂停非关键 Job
kubectl patch job non-critical-batch -p '{"spec":{"suspend":true}}'

# 低峰期恢复
kubectl patch job non-critical-batch -p '{"spec":{"suspend":false}}'
```

---

### 10. Resumed (Job 已恢复)

**事件模板:**
```yaml
Type: Normal
Reason: Resumed
Message: "Job resumed"
Source: job-controller
First Seen: 2026-02-10T10:05:00Z
Last Seen: 2026-02-10T10:05:00Z
Count: 1
```

**触发条件:**
- `.spec.suspend` 从 `true` 变为 `false`
- Job 控制器开始重新创建 Pod
- 从上次暂停位置继续执行

**版本信息:**
- **起始版本**: v1.22 (Beta)
- **GA 版本**: v1.24

**生产影响:**
- ✅ 正常恢复流程
- ▶️ Job 继续执行
- 📊 可配合监控和自动化

**观测示例:**
```bash
# 恢复 Job
kubectl patch job batch-processor -p '{"spec":{"suspend":false}}'

# 查看恢复后 Pod 创建
kubectl get events --field-selector involvedObject.name=batch-processor --sort-by='.lastTimestamp'
```

---

### 11. FailedJob (Indexed Job 失败)

**事件模板:**
```yaml
Type: Warning
Reason: FailedJob
Message: "Job failed: index X failed"
Source: job-controller
First Seen: 2026-02-10T10:20:00Z
Last Seen: 2026-02-10T10:20:00Z
Count: 1
```

**触发条件:**
- Indexed Job 中某个索引的 Pod 失败
- 配合 `podFailurePolicy` 使用
- 索引任务不可恢复失败

**版本信息:**
- **起始版本**: v1.26
- **特性**: Indexed Jobs + Pod Failure Policy

**Indexed Jobs 说明:**
```yaml
apiVersion: batch/v1
kind: Job
metadata:
  name: indexed-processor
spec:
  completions: 10           # 10 个索引任务 (0-9)
  parallelism: 3            # 并行 3 个
  completionMode: Indexed   # 索引模式
  template:
    spec:
      containers:
      - name: worker
        image: processor:v1
        env:
        - name: JOB_COMPLETION_INDEX  # 自动注入索引 (0-9)
          valueFrom:
            fieldRef:
              fieldPath: metadata.annotations['batch.kubernetes.io/job-completion-index']
        command:
        - sh
        - -c
        - |
          echo "Processing index: $JOB_COMPLETION_INDEX"
          # 根据索引处理不同数据分片
      restartPolicy: Never
```

**应用场景:**
- 大规模数据分片处理
- 参数化批量任务
- MapReduce 风格作业

---

### 12. SuccessCriteriaMet (满足成功策略)

**事件模板:**
```yaml
Type: Normal
Reason: SuccessCriteriaMet
Message: "Pods satisfied success criteria"
Source: job-controller
First Seen: 2026-02-10T10:15:00Z
Last Seen: 2026-02-10T10:15:00Z
Count: 1
```

**触发条件:**
- 满足自定义 `successPolicy` 条件
- 达到成功阈值即可完成 Job
- 无需等待所有 Pod 成功

**版本信息:**
- **起始版本**: v1.28 (Alpha)
- **特性门控**: `JobSuccessPolicy`

**successPolicy 功能:**
```yaml
apiVersion: batch/v1
kind: Job
metadata:
  name: ml-training
spec:
  completions: 10           # 总共 10 个训练副本
  parallelism: 10
  successPolicy:
    rules:
    - succeededIndexes: "0-2"      # 索引 0、1、2 成功即可
      succeededCount: 3             # 或任意 3 个成功
  completionMode: Indexed
  template:
    spec:
      containers:
      - name: trainer
        image: ml-trainer:v1
```

**使用场景:**
- 机器学习分布式训练（部分成功即可）
- 冗余计算任务
- 采样式批处理

---

## CronJob 控制器事件

### 13. SuccessfulCreate (Job 创建成功)

**事件模板:**
```yaml
Type: Normal
Reason: SuccessfulCreate
Message: "Created job <job-name>"
Source: cronjob-controller
First Seen: 2026-02-10T10:00:00Z
Last Seen: 2026-02-10T10:00:00Z
Count: 1
```

**触发条件:**
- CronJob 按 schedule 触发新的 Job
- 调度时间到达且满足执行条件
- 并发策略允许创建新 Job

**字段详解:**
- `<job-name>`: 新创建的 Job 名称，格式 `<cronjob-name>-<timestamp>`

**版本信息:**
- **起始版本**: v1.4 (batch/v1beta1)
- **GA 版本**: v1.21 (batch/v1)

**生产影响:**
- ✅ 正常调度流程
- 📅 可追踪执行历史
- 🔍 用于审计和监控

**观测示例:**
```bash
# 查看 CronJob 最近创建的 Job
kubectl get jobs -l cronjob-name=hourly-backup --sort-by=.metadata.creationTimestamp

# 查看调度历史
kubectl describe cronjob hourly-backup | grep "Last Schedule Time"

# 查看活跃 Job
kubectl get cronjob hourly-backup -o jsonpath='{.status.active}'
```

---

### 14. SuccessfulDelete (Job 删除成功)

**事件模板:**
```yaml
Type: Normal
Reason: SuccessfulDelete
Message: "Deleted job <job-name>"
Source: cronjob-controller
First Seen: 2026-02-10T11:00:00Z
Last Seen: 2026-02-10T11:00:00Z
Count: 1
```

**触发条件:**
- 超过 `.spec.successfulJobsHistoryLimit` 保留数量
- 超过 `.spec.failedJobsHistoryLimit` 保留数量
- CronJob 自动清理历史 Job

**字段详解:**
- `<job-name>`: 被删除的历史 Job 名称

**版本信息:**
- **起始版本**: v1.4
- **默认值**: `successfulJobsHistoryLimit: 3`, `failedJobsHistoryLimit: 1`

**生产影响:**
- ✅ 自动资源清理
- 🗑️ 防止 Job 堆积
- 📊 保留适量历史用于审计

**配置建议:**
```yaml
apiVersion: batch/v1
kind: CronJob
metadata:
  name: hourly-backup
spec:
  schedule: "0 * * * *"
  successfulJobsHistoryLimit: 5   # 保留最近 5 次成功 Job
  failedJobsHistoryLimit: 3       # 保留最近 3 次失败 Job
  jobTemplate:
    spec:
      template:
        spec:
          containers:
          - name: backup
            image: backup-tool:v1
          restartPolicy: OnFailure
```

---

### 15. SawCompletedJob (发现已完成 Job)

**事件模板:**
```yaml
Type: Normal
Reason: SawCompletedJob
Message: "Saw completed job: <job-name>, status: Complete"
Source: cronjob-controller
First Seen: 2026-02-10T10:30:00Z
Last Seen: 2026-02-10T10:30:00Z
Count: 1
```

**触发条件:**
- CronJob 控制器检测到 Job 完成
- 用于更新 `.status.lastSuccessfulTime`
- 触发历史清理逻辑

**字段详解:**
- `<job-name>`: 已完成的 Job 名称
- `status`: Complete（成功）或 Failed（失败）

**版本信息:**
- **起始版本**: v1.4

**生产影响:**
- ✅ 正常状态同步
- 📊 更新执行统计
- 🔄 准备下次调度

---

### 16. UnexpectedJob (发现未预期 Job)

**事件模板:**
```yaml
Type: Warning
Reason: UnexpectedJob
Message: "Saw unexpected active job: <job-name>"
Source: cronjob-controller
First Seen: 2026-02-10T10:00:00Z
Last Seen: 2026-02-10T10:00:00Z
Count: 1
```

**触发条件:**
- 发现不在 CronJob 管理列表中的活跃 Job
- Job 的 label 匹配但 ownerReference 不匹配
- 手动创建了同名 Job

**版本信息:**
- **起始版本**: v1.4
- **触发场景**: 罕见（异常情况）

**生产影响:**
- ⚠️ **中等**: 表示状态不一致
- 🔍 需要检查是否有手动操作
- 🐛 可能是控制器同步问题

**故障排查:**
```bash
# 检查所有相关 Job
kubectl get jobs -l cronjob-name=hourly-backup

# 检查 Job 的 ownerReference
kubectl get job <job-name> -o jsonpath='{.metadata.ownerReferences}'

# 检查 CronJob 状态
kubectl get cronjob hourly-backup -o yaml
```

---

### 17. MissingJob (预期 Job 未找到)

**事件模板:**
```yaml
Type: Normal
Reason: MissingJob
Message: "Expected but did not find job: <expected-job-name>"
Source: cronjob-controller
First Seen: 2026-02-10T10:00:00Z
Last Seen: 2026-02-10T10:00:00Z
Count: 1
```

**触发条件:**
- CronJob 状态中记录了某个 Job 但实际不存在
- Job 被外部删除
- 控制器恢复后状态同步

**版本信息:**
- **起始版本**: v1.4
- **触发场景**: 罕见

**生产影响:**
- ℹ️ **低**: 控制器自动修正状态
- 🔄 通常自动恢复
- 📝 记录异常删除操作

---

### 18. TooManyMissedTimes (错过太多执行时间)

**事件模板:**
```yaml
Type: Warning
Reason: TooManyMissedTimes
Message: "Too many missed start times (> 100). Set or decrease .spec.startingDeadlineSeconds or check clock skew."
Source: cronjob-controller
First Seen: 2026-02-10T10:00:00Z
Last Seen: 2026-02-10T10:00:00Z
Count: 1
```

**触发条件:**
- CronJob 长时间未被调度（如 Controller 宕机）
- 恢复后发现错过超过 100 次调度时间
- 避免创建大量历史 Job

**字段详解:**
- 错过次数阈值: 100 次

**版本信息:**
- **起始版本**: v1.4
- **最后变更**: v1.21（错误信息改进）

**生产影响:**
- ⚠️ **高**: 表示调度中断
- 🚨 需要检查控制器健康状态
- ⏰ 可能错过重要任务

**故障排查:**
```bash
# 1. 检查 CronJob 最后调度时间
kubectl get cronjob hourly-backup -o jsonpath='{.status.lastScheduleTime}'

# 2. 检查控制器日志
kubectl logs -n kube-system -l component=kube-controller-manager | grep cronjob

# 3. 检查时钟同步
date
kubectl get nodes -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{.status.conditions[?(@.type=="Ready")].lastHeartbeatTime}{"\n"}{end}'

# 4. 检查 startingDeadlineSeconds
kubectl get cronjob hourly-backup -o jsonpath='{.spec.startingDeadlineSeconds}'
```

**配置优化:**
```yaml
apiVersion: batch/v1
kind: CronJob
metadata:
  name: critical-backup
spec:
  schedule: "*/5 * * * *"         # 每 5 分钟
  startingDeadlineSeconds: 300    # 5 分钟内必须启动
  concurrencyPolicy: Replace      # 替换旧的未完成 Job
  successfulJobsHistoryLimit: 3
  failedJobsHistoryLimit: 1
  jobTemplate:
    spec:
      activeDeadlineSeconds: 600  # 单次 Job 10 分钟超时
      template:
        spec:
          containers:
          - name: backup
            image: backup:v1
          restartPolicy: OnFailure

# startingDeadlineSeconds 计算:
# 应略大于 schedule 间隔，以容忍短暂延迟
# 示例: schedule 间隔 5 分钟，设置 300 秒（5 分钟）或 600 秒（10 分钟）
```

---

### 19. FailedCreate (Job 创建失败)

**事件模板:**
```yaml
Type: Warning
Reason: FailedCreate
Message: "Error creating job: Job.batch \"xxx\" is invalid: spec.template.spec.restartPolicy: Invalid value: \"Always\": Unsupported value"
Source: cronjob-controller
First Seen: 2026-02-10T10:00:00Z
Last Seen: 2026-02-10T10:05:00Z
Count: 5
```

**触发条件:**
- Job 模板配置错误
- ResourceQuota 限制
- RBAC 权限不足
- 验证 webhook 拒绝

**常见错误消息:**
```
# 配置错误
Error creating job: Job.batch "xxx" is invalid: spec.template.spec.restartPolicy: Invalid value: "Always"

# 配额超限
Error creating job: forbidden: exceeded quota: compute-resources

# 权限不足
Error creating job: jobs.batch is forbidden: User "system:serviceaccount:default:cronjob-controller" cannot create resource "jobs"

# Webhook 拒绝
Error creating job: admission webhook "validate.job" denied the request: invalid configuration
```

**版本信息:**
- **起始版本**: v1.4

**生产影响:**
- ⛔ **严重**: CronJob 无法执行
- 🚨 需要立即修复配置
- 📊 持续失败会积压调度

**故障排查:**
```bash
# 1. 验证 Job 模板
kubectl create job test-job --from=cronjob/hourly-backup --dry-run=server

# 2. 检查 RBAC 权限
kubectl auth can-i create jobs --as=system:serviceaccount:default:cronjob-controller

# 3. 检查 ResourceQuota
kubectl describe resourcequota -A

# 4. 检查 ValidatingWebhookConfiguration
kubectl get validatingwebhookconfiguration
```

**修复方案:**
```yaml
# 常见错误修复

# 错误 1: restartPolicy 必须是 OnFailure 或 Never
apiVersion: batch/v1
kind: CronJob
spec:
  jobTemplate:
    spec:
      template:
        spec:
          restartPolicy: OnFailure  # 不能是 Always

# 错误 2: 授予 RBAC 权限
apiVersion: rbac.authorization.k8s.io/v1
kind: Role
metadata:
  name: cronjob-executor
rules:
- apiGroups: ["batch"]
  resources: ["jobs"]
  verbs: ["create", "get", "list", "delete"]
---
apiVersion: rbac.authorization.k8s.io/v1
kind: RoleBinding
metadata:
  name: cronjob-executor-binding
roleRef:
  apiGroup: rbac.authorization.k8s.io
  kind: Role
  name: cronjob-executor
subjects:
- kind: ServiceAccount
  name: cronjob-sa
  namespace: default
```

---

### 20. ForbidConcurrent (并发策略禁止)

**事件模板:**
```yaml
Type: Warning
Reason: ForbidConcurrent
Message: "Cannot create job: too many jobs running (xxx) for the CronJob, concurrencyPolicy is Forbid"
Source: cronjob-controller
First Seen: 2026-02-10T10:00:00Z
Last Seen: 2026-02-10T10:00:00Z
Count: 1
```

**触发条件:**
- `.spec.concurrencyPolicy` 设置为 `Forbid`
- 上次 Job 尚未完成
- 新的调度时间到达但被阻止

**字段详解:**
- `xxx`: 当前运行中的 Job 数量
- `concurrencyPolicy`: 并发策略

**版本信息:**
- **起始版本**: v1.4

**生产影响:**
- ⚠️ **中等**: 跳过当前调度
- 📊 可能导致任务积压
- ⏱️ 需要评估 Job 执行时间

**并发策略详解:**
```yaml
apiVersion: batch/v1
kind: CronJob
metadata:
  name: data-sync
spec:
  schedule: "*/10 * * * *"        # 每 10 分钟
  concurrencyPolicy: Forbid       # Allow | Forbid | Replace
  jobTemplate:
    spec:
      template:
        spec:
          containers:
          - name: syncer
            image: syncer:v1

# 三种并发策略:

# 1. Allow (默认) - 允许并发运行
#    适用场景: 无状态、无资源竞争的任务
#    风险: 可能导致资源争抢

# 2. Forbid - 禁止并发，跳过新调度
#    适用场景: 有状态、资源独占的任务
#    风险: 如果 Job 执行时间过长，会持续跳过调度

# 3. Replace - 替换旧的运行中 Job
#    适用场景: 只需最新结果的任务
#    风险: 可能中断正在执行的重要操作
```

**观测示例:**
```bash
# 查看活跃 Job
kubectl get jobs -l cronjob-name=data-sync --field-selector=status.successful!=1

# 查看 Job 运行时长
kubectl get jobs -l cronjob-name=data-sync -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{.status.startTime}{"\t"}{.status.completionTime}{"\n"}{end}'

# 统计跳过次数
kubectl get events --field-selector reason=ForbidConcurrent,involvedObject.name=data-sync --sort-by='.lastTimestamp'
```

**优化方案:**
```yaml
# 方案 1: 优化 Job 执行时间
apiVersion: batch/v1
kind: CronJob
spec:
  schedule: "*/10 * * * *"
  concurrencyPolicy: Forbid
  jobTemplate:
    spec:
      activeDeadlineSeconds: 480   # 8 分钟超时（小于调度间隔）
      template:
        spec:
          containers:
          - name: syncer
            resources:
              requests:
                cpu: "1"             # 增加资源加速执行
                memory: "2Gi"

# 方案 2: 调整调度间隔
apiVersion: batch/v1
kind: CronJob
spec:
  schedule: "*/15 * * * *"          # 增加到 15 分钟
  concurrencyPolicy: Forbid

# 方案 3: 改用 Replace 策略（适用于幂等任务）
apiVersion: batch/v1
kind: CronJob
spec:
  schedule: "*/10 * * * *"
  concurrencyPolicy: Replace         # 自动终止旧 Job

# 方案 4: 改用 Allow + 分布式锁（应用层控制）
apiVersion: batch/v1
kind: CronJob
spec:
  schedule: "*/10 * * * *"
  concurrencyPolicy: Allow
  jobTemplate:
    spec:
      template:
        spec:
          containers:
          - name: syncer
            image: syncer-with-lock:v1  # 应用内实现分布式锁
```

---

## 批处理执行生命周期

### Job 执行流程

```
┌─────────────────────────────────────────────────────────────────┐
│                     Job Execution Lifecycle                      │
└─────────────────────────────────────────────────────────────────┘

创建阶段:
  kubectl apply -f job.yaml
         │
         ▼
  [Job Controller 接收]
         │
         ├─── 验证配置
         ├─── 计算所需 Pod 数量
         │    (min(parallelism, completions - succeeded))
         └─── 生成 Pod 模板
                │
                ▼
         [SuccessfulCreate Event]
         创建 Pod: job-xxx-abc123


执行阶段:
  [Pod 运行中]
         │
         ├─── 正常完成 (exitCode=0)
         │         │
         │         ▼
         │    succeeded++
         │         │
         │         ├─── succeeded < completions
         │         │         │
         │         │         └──> [创建新 Pod]
         │         │
         │         └─── succeeded == completions
         │                   │
         │                   ▼
         │            [Completed Event]
         │            Job 完成
         │
         └─── 失败 (exitCode != 0)
                   │
                   ▼
              failed++
                   │
                   ├─── failed < backoffLimit
                   │         │
                   │         ├─── 等待退避时间 (指数退避)
                   │         └──> [SuccessfulCreate Event]
                   │              重新创建 Pod
                   │
                   └─── failed >= backoffLimit
                             │
                             ▼
                      [BackoffLimitExceeded Event]
                      Job 失败


超时检查:
  [Job Controller 定期检查]
         │
         └─── (now - startTime) > activeDeadlineSeconds
                   │
                   ▼
            [DeadlineExceeded Event]
            终止所有 Pod，Job 失败


清理阶段:
  [Job 完成或失败]
         │
         └─── (now - completionTime) > ttlSecondsAfterFinished
                   │
                   ▼
            [SuccessfulDelete Event]
            删除 Job 及其 Pod


暂停/恢复:
  kubectl patch job xxx -p '{"spec":{"suspend":true}}'
         │
         ▼
  [Suspended Event]
  删除所有活跃 Pod
         │
         └─── kubectl patch job xxx -p '{"spec":{"suspend":false}}'
                   │
                   ▼
              [Resumed Event]
              重新创建 Pod
```

### CronJob 调度流程

```
┌─────────────────────────────────────────────────────────────────┐
│                   CronJob Scheduling Lifecycle                   │
└─────────────────────────────────────────────────────────────────┘

调度周期:
  [CronJob Controller 每 10 秒同步一次]
         │
         └─── 计算下一次调度时间
                   │
                   ├─── 未到调度时间
                   │         └──> 等待
                   │
                   └─── 到达调度时间
                             │
                             ▼
                      [检查 startingDeadlineSeconds]
                             │
                             ├─── 超过截止时间
                             │         └──> 跳过此次调度
                             │
                             └─── 在截止时间内
                                       │
                                       ▼
                                [检查 concurrencyPolicy]
                                       │
                                       ├─── Forbid + 有活跃 Job
                                       │         │
                                       │         ▼
                                       │    [ForbidConcurrent Event]
                                       │    跳过此次调度
                                       │
                                       ├─── Replace + 有活跃 Job
                                       │         │
                                       │         ▼
                                       │    删除旧 Job，创建新 Job
                                       │
                                       └─── Allow 或无活跃 Job
                                                 │
                                                 ▼
                                          [SuccessfulCreate Event]
                                          创建新 Job: cronjob-xxx-1234567890


Job 监控:
  [CronJob Controller 监控 Job 状态]
         │
         ├─── Job 完成
         │         │
         │         ▼
         │    [SawCompletedJob Event]
         │    更新 lastSuccessfulTime
         │         │
         │         └─── 检查历史数量
         │                   │
         │                   └─── 超过 successfulJobsHistoryLimit
         │                             │
         │                             ▼
         │                      [SuccessfulDelete Event]
         │                      删除旧 Job
         │
         └─── Job 失败
                   │
                   ▼
              [SawCompletedJob Event]
              status: Failed
                   │
                   └─── 检查历史数量
                             │
                             └─── 超过 failedJobsHistoryLimit
                                       │
                                       ▼
                                [SuccessfulDelete Event]
                                删除旧 Job


异常处理:
  [控制器宕机恢复]
         │
         └─── 计算错过的调度次数
                   │
                   ├─── 错过次数 <= 100
                   │         └──> 创建最近一次的 Job
                   │
                   └─── 错过次数 > 100
                             │
                             ▼
                      [TooManyMissedTimes Event]
                      跳过所有错过的调度


时间计算示例:
  schedule: "*/5 * * * *"  (每 5 分钟)

  当前时间: 10:03:00
  上次调度: 10:00:00
  下次调度: 10:05:00
  等待时间: 2 分钟

  startingDeadlineSeconds: 300 (5 分钟)
  调度窗口: [10:00:00, 10:05:00]

  如果 10:06:00 才执行检查:
    - 超过 startingDeadlineSeconds: No (10:06 - 10:05 = 1 分钟 < 5 分钟)
    - 继续创建 Job

  如果 10:11:00 才执行检查:
    - 超过 startingDeadlineSeconds: Yes (10:11 - 10:05 = 6 分钟 > 5 分钟)
    - 跳过此次调度，等待下次 10:10:00
```

---

## 深度分析

### 1. BackoffLimitExceeded 深度剖析

**退避算法实现:**
```go
// Kubernetes Job Controller 退避算法伪代码

func getBackoffDuration(failureCount int) time.Duration {
    // 基础退避时间: 10 秒
    baseDelay := 10 * time.Second
    
    // 指数退避: 10s * 2^(failures-1)
    // 失败 1 次: 10s * 2^0 = 10s
    // 失败 2 次: 10s * 2^1 = 20s
    // 失败 3 次: 10s * 2^2 = 40s
    // 失败 4 次: 10s * 2^3 = 80s
    // 失败 5 次: 10s * 2^4 = 160s
    delay := baseDelay * (1 << (failureCount - 1))
    
    // 最大退避时间: 6 分钟
    maxDelay := 6 * time.Minute
    if delay > maxDelay {
        return maxDelay
    }
    
    return delay
}

// 计算下次重试时间
nextRetryTime := podFailureTime.Add(getBackoffDuration(failureCount))
```

**退避时间表:**
| 失败次数 | 退避时间 | 累计等待时间 | 说明 |
|---------|---------|-------------|------|
| 1 | 10s | 10s | 2^0 × 10s |
| 2 | 20s | 30s | 2^1 × 10s |
| 3 | 40s | 70s | 2^2 × 10s |
| 4 | 80s | 150s | 2^3 × 10s |
| 5 | 160s | 310s | 2^4 × 10s |
| 6 | 320s (5m20s) | 630s (10m30s) | 2^5 × 10s |
| 7+ | 360s (6m) | - | 达到最大值 |

**失败场景分类:**

```yaml
# 场景 1: 临时网络错误（应该重试）
apiVersion: batch/v1
kind: Job
metadata:
  name: api-caller
spec:
  backoffLimit: 6            # 允许多次重试
  template:
    spec:
      containers:
      - name: caller
        image: curl:latest
        command: ["curl", "https://api.example.com"]
      restartPolicy: Never

# 场景 2: 数据验证错误（不应重试）
apiVersion: batch/v1
kind: Job
metadata:
  name: data-validator
spec:
  backoffLimit: 0            # 不重试，立即失败
  podFailurePolicy:          # v1.26+
    rules:
    - action: FailJob        # 数据错误直接失败
      onExitCodes:
        containerName: validator
        operator: In
        values: [1]          # 退出码 1 表示数据无效
  template:
    spec:
      containers:
      - name: validator
        image: validator:v1
      restartPolicy: Never

# 场景 3: 混合场景（选择性重试）
apiVersion: batch/v1
kind: Job
metadata:
  name: smart-processor
spec:
  backoffLimit: 5
  podFailurePolicy:
    rules:
    - action: FailJob              # 配置错误不重试
      onExitCodes:
        operator: In
        values: [2]
    - action: Ignore               # 节点驱逐不计入失败
      onPodConditions:
      - type: DisruptionTarget
    - action: Count                # 其他错误正常计入
      onExitCodes:
        operator: NotIn
        values: [0, 2]
  template:
    spec:
      containers:
      - name: processor
        image: processor:v1
      restartPolicy: Never
```

**监控和告警:**
```bash
# Prometheus 查询示例

# 1. Job 失败率
sum(rate(kube_job_status_failed{namespace="production"}[5m]))
  /
sum(rate(kube_job_status_succeeded{namespace="production"}[5m]) + rate(kube_job_status_failed{namespace="production"}[5m]))

# 2. BackoffLimitExceeded 事件数量
sum(increase(kube_events_total{reason="BackoffLimitExceeded"}[1h]))

# 3. Job 重试次数分布
histogram_quantile(0.95, 
  sum(rate(kube_job_status_failed[5m])) by (job_name, le)
)

# 4. 平均失败等待时间
avg(kube_job_complete_time - kube_job_start_time) 
  by (job_name)
  where kube_job_status_failed > 0
```

**告警规则:**
```yaml
# Prometheus AlertManager 规则
groups:
- name: job_alerts
  rules:
  - alert: JobBackoffLimitExceeded
    expr: |
      sum(increase(kube_events_total{reason="BackoffLimitExceeded"}[5m])) > 0
    for: 1m
    labels:
      severity: warning
    annotations:
      summary: "Job {{ $labels.name }} 达到重试上限"
      description: "Job 在过去 5 分钟内达到 backoffLimit，需要人工介入"

  - alert: JobHighFailureRate
    expr: |
      sum(rate(kube_job_status_failed[5m])) by (namespace, job_name)
        /
      sum(rate(kube_job_status_succeeded[5m] + kube_job_status_failed[5m])) by (namespace, job_name)
      > 0.5
    for: 10m
    labels:
      severity: critical
    annotations:
      summary: "Job {{ $labels.job_name }} 失败率过高"
      description: "失败率超过 50%，当前值: {{ $value }}"
```

---

### 2. Job Suspend/Resume 特性 (v1.22+)

**使用场景:**

```yaml
# 场景 1: 计划维护窗口
apiVersion: batch/v1
kind: Job
metadata:
  name: db-migration
spec:
  suspend: true              # 创建时暂停
  completions: 100
  parallelism: 10
  template:
    spec:
      containers:
      - name: migrator
        image: db-migrator:v1

# 运维流程:
# 1. 创建 Job（暂停状态）
kubectl apply -f db-migration.yaml

# 2. 维护窗口开始，恢复 Job
kubectl patch job db-migration -p '{"spec":{"suspend":false}}'

# 3. 紧急情况需要暂停
kubectl patch job db-migration -p '{"spec":{"suspend":true}}'

# 4. 问题解决后恢复
kubectl patch job db-migration -p '{"spec":{"suspend":false}}'
```

```yaml
# 场景 2: 资源动态调度
apiVersion: v1
kind: ConfigMap
metadata:
  name: job-scheduler-config
data:
  peak_hours: "09:00-18:00"    # 高峰期暂停低优先级 Job
  off_hours: "18:00-09:00"     # 低峰期恢复

---
# 自动化脚本（伪代码）
# 高峰期自动暂停非关键 Job
apiVersion: batch/v1
kind: CronJob
metadata:
  name: job-suspender
spec:
  schedule: "0 9 * * *"        # 每天 9:00
  jobTemplate:
    spec:
      template:
        spec:
          containers:
          - name: suspender
            image: kubectl:latest
            command:
            - sh
            - -c
            - |
              kubectl patch job non-critical-batch -p '{"spec":{"suspend":true}}'

---
# 低峰期自动恢复
apiVersion: batch/v1
kind: CronJob
metadata:
  name: job-resumer
spec:
  schedule: "0 18 * * *"       # 每天 18:00
  jobTemplate:
    spec:
      template:
        spec:
          containers:
          - name: resumer
            image: kubectl:latest
            command:
            - sh
            - -c
            - |
              kubectl patch job non-critical-batch -p '{"spec":{"suspend":false}}'
```

**状态变化跟踪:**
```bash
# 监控 suspend 状态变化
kubectl get events --watch | grep -E 'Suspended|Resumed'

# 查看 Job 暂停历史
kubectl describe job db-migration | grep -A 5 "Suspended\|Resumed"

# 统计暂停时长
# 使用自定义脚本或 Prometheus 查询
```

**限制和注意事项:**
- 暂停时所有活跃 Pod 将被删除（数据可能丢失）
- 恢复后从头开始执行（非断点续传）
- `.status.succeeded` 计数保留
- 适用于幂等性任务

---

### 3. Indexed Jobs (v1.24 GA)

**完整示例:**
```yaml
apiVersion: batch/v1
kind: Job
metadata:
  name: data-processor
spec:
  completions: 10              # 10 个索引任务 (0-9)
  parallelism: 3               # 并行 3 个
  completionMode: Indexed      # 索引模式
  template:
    metadata:
      labels:
        app: data-processor
    spec:
      restartPolicy: Never
      containers:
      - name: processor
        image: python:3.9
        env:
        - name: JOB_COMPLETION_INDEX
          valueFrom:
            fieldRef:
              fieldPath: metadata.annotations['batch.kubernetes.io/job-completion-index']
        - name: DATA_BUCKET
          value: "s3://my-data-bucket"
        command:
        - python
        - -c
        - |
          import os
          import sys
          
          # 获取当前索引
          index = int(os.environ['JOB_COMPLETION_INDEX'])
          total = 10
          
          print(f"Processing index: {index}/{total}")
          
          # 数据分片逻辑
          start_id = index * 1000
          end_id = (index + 1) * 1000
          
          print(f"Processing records {start_id} to {end_id}")
          
          # 模拟处理
          # process_data_shard(start_id, end_id)
          
          print(f"Index {index} completed successfully")
```

**数据分片策略:**
```python
# 示例: 大规模数据处理

# 方案 1: 均匀分片
def get_shard_range(index, total_tasks, total_records):
    shard_size = total_records // total_tasks
    start = index * shard_size
    end = start + shard_size if index < total_tasks - 1 else total_records
    return start, end

# 使用:
# Total: 1000万条记录, 100 个任务
# Index 0: 0 - 100,000
# Index 1: 100,000 - 200,000
# Index 99: 9,900,000 - 10,000,000

# 方案 2: 哈希分片
def get_shard_key(index, total_tasks):
    return lambda key: hash(key) % total_tasks == index

# 使用:
# 只处理 hash(record_id) % 100 == index 的记录

# 方案 3: 范围分片（适用于有序数据）
def get_date_range(index, total_tasks):
    start_date = datetime(2024, 1, 1) + timedelta(days=index*30)
    end_date = start_date + timedelta(days=30)
    return start_date, end_date

# 使用:
# Index 0: 2024-01-01 to 2024-01-31
# Index 1: 2024-02-01 to 2024-03-02
```

**MapReduce 风格实现:**
```yaml
# Map 阶段: Indexed Job 处理数据分片
apiVersion: batch/v1
kind: Job
metadata:
  name: map-job
spec:
  completions: 100
  parallelism: 20
  completionMode: Indexed
  template:
    spec:
      containers:
      - name: mapper
        image: map-processor:v1
        env:
        - name: INDEX
          valueFrom:
            fieldRef:
              fieldPath: metadata.annotations['batch.kubernetes.io/job-completion-index']
        - name: OUTPUT_PATH
          value: "/data/map-output"
        volumeMounts:
        - name: shared-data
          mountPath: /data
      volumes:
      - name: shared-data
        persistentVolumeClaim:
          claimName: map-reduce-pvc
      restartPolicy: Never

---
# Reduce 阶段: 单个 Job 汇总结果
apiVersion: batch/v1
kind: Job
metadata:
  name: reduce-job
spec:
  completions: 1
  template:
    spec:
      containers:
      - name: reducer
        image: reduce-processor:v1
        env:
        - name: INPUT_PATH
          value: "/data/map-output"
        - name: OUTPUT_PATH
          value: "/data/final-result"
        volumeMounts:
        - name: shared-data
          mountPath: /data
      volumes:
      - name: shared-data
        persistentVolumeClaim:
          claimName: map-reduce-pvc
      restartPolicy: Never
```

**监控 Indexed Jobs:**
```bash
# 查看各索引完成情况
kubectl get pods -l job-name=data-processor -o custom-columns=\
  NAME:.metadata.name,\
  INDEX:.metadata.annotations.batch\.kubernetes\.io/job-completion-index,\
  STATUS:.status.phase

# 输出示例:
# NAME                      INDEX   STATUS
# data-processor-0-abc123   0       Succeeded
# data-processor-1-def456   1       Running
# data-processor-2-ghi789   2       Succeeded
# data-processor-3-jkl012   3       Pending

# 统计完成度
kubectl get job data-processor -o jsonpath='{.status.succeeded}/{.spec.completions}'
# 输出: 7/10

# 查看特定索引的日志
INDEX=5
kubectl logs -l job-name=data-processor,batch.kubernetes.io/job-completion-index=$INDEX
```

---

### 4. CronJob 时区支持 (v1.25+)

**时区配置:**
```yaml
apiVersion: batch/v1
kind: CronJob
metadata:
  name: daily-report
spec:
  schedule: "0 9 * * *"              # 每天 9:00
  timeZone: "Asia/Shanghai"          # 东八区时间 (v1.25+)
  jobTemplate:
    spec:
      template:
        spec:
          containers:
          - name: reporter
            image: report-generator:v1
          restartPolicy: OnFailure

# 常用时区:
# "Asia/Shanghai"      - 中国标准时间 (UTC+8)
# "America/New_York"   - 美国东部时间 (UTC-5/-4)
# "Europe/London"      - 英国时间 (UTC+0/+1)
# "UTC"                - 协调世界时
```

**版本兼容性:**
- **v1.24 及更早**: 不支持 `timeZone`，使用控制器所在节点时区
- **v1.25+**: 支持 `timeZone` 字段（Beta）
- **v1.27**: `timeZone` 字段升级为 GA

---

## 故障排查模式

### 问题 1: Job 长时间不创建 Pod

**症状:**
```bash
kubectl get job my-job
# NAME     COMPLETIONS   DURATION   AGE
# my-job   0/5           0s         5m

kubectl get pods -l job-name=my-job
# No resources found
```

**排查步骤:**
```bash
# 1. 检查 Job 事件
kubectl describe job my-job | grep Events -A 20

# 可能看到:
# Type     Reason        Message
# Warning  FailedCreate  Error creating: pods "my-job-xxx" is forbidden: exceeded quota

# 2. 检查 ResourceQuota
kubectl describe resourcequota -n <namespace>

# 3. 检查 Job 配置
kubectl get job my-job -o yaml | grep -A 10 "resources:"

# 4. 检查节点资源
kubectl top nodes

# 5. 模拟创建
kubectl create job test --image=busybox --dry-run=server
```

**解决方案:**
- 增加 ResourceQuota 限制
- 降低 Job 资源请求
- 清理旧的 Job 释放配额
- 检查 LimitRange 配置

---

### 问题 2: CronJob 不按时执行

**症状:**
```bash
kubectl get cronjob hourly-backup
# NAME            SCHEDULE      SUSPEND   ACTIVE   LAST SCHEDULE   AGE
# hourly-backup   0 * * * *     False     0        62m             5h

# 最后调度时间是 62 分钟前（应该是最近 1 小时内）
```

**排查步骤:**
```bash
# 1. 检查 CronJob 事件
kubectl describe cronjob hourly-backup | grep Events -A 20

# 可能看到:
# Type     Reason            Message
# Warning  FailedCreate      Error creating job: ...
# Warning  ForbidConcurrent  Cannot create job: too many jobs running

# 2. 检查活跃 Job
kubectl get jobs -l cronjob-name=hourly-backup --field-selector=status.successful!=1

# 3. 检查 concurrencyPolicy
kubectl get cronjob hourly-backup -o jsonpath='{.spec.concurrencyPolicy}'

# 4. 检查 startingDeadlineSeconds
kubectl get cronjob hourly-backup -o jsonpath='{.spec.startingDeadlineSeconds}'

# 5. 检查控制器健康
kubectl get pods -n kube-system -l component=kube-controller-manager
kubectl logs -n kube-system -l component=kube-controller-manager --tail=100 | grep cronjob
```

**解决方案:**
```bash
# 方案 1: 清理卡住的 Job
kubectl delete job -l cronjob-name=hourly-backup --field-selector=status.successful!=1

# 方案 2: 调整并发策略
kubectl patch cronjob hourly-backup -p '{"spec":{"concurrencyPolicy":"Replace"}}'

# 方案 3: 增加超时时间
kubectl patch cronjob hourly-backup -p '{"spec":{"startingDeadlineSeconds":600}}'

# 方案 4: 优化 Job 执行时间
kubectl patch cronjob hourly-backup -p '{"spec":{"jobTemplate":{"spec":{"activeDeadlineSeconds":1800}}}}'
```

---

### 问题 3: Job 频繁达到 BackoffLimitExceeded

**症状:**
```bash
kubectl get job data-import
# NAME          COMPLETIONS   DURATION   AGE
# data-import   0/1           5m         5m

kubectl describe job data-import
# Type     Reason                  Message
# Warning  BackoffLimitExceeded    Job has reached the specified backoff limit
```

**深度分析:**
```bash
# 1. 查看所有失败 Pod 的退出码
kubectl get pods -l job-name=data-import \
  -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{.status.containerStatuses[0].state.terminated.exitCode}{"\t"}{.status.containerStatuses[0].state.terminated.reason}{"\n"}{end}'

# 输出示例:
# data-import-abc123   137   OOMKilled
# data-import-def456   137   OOMKilled
# data-import-ghi789   137   OOMKilled

# 2. 查看失败 Pod 日志
kubectl logs -l job-name=data-import --tail=100 --prefix=true

# 3. 分析失败模式
# 退出码 137: OOMKilled - 需要增加内存
# 退出码 1: 应用错误 - 检查代码逻辑
# 退出码 143: SIGTERM - 检查超时配置
```

**针对性解决:**
```yaml
# 场景 1: OOM 导致失败
apiVersion: batch/v1
kind: Job
metadata:
  name: data-import
spec:
  backoffLimit: 3
  template:
    spec:
      containers:
      - name: importer
        image: importer:v1
        resources:
          limits:
            memory: "4Gi"     # 增加内存
          requests:
            memory: "2Gi"

# 场景 2: 外部依赖超时
apiVersion: batch/v1
kind: Job
metadata:
  name: api-caller
spec:
  backoffLimit: 10            # 增加重试次数
  podFailurePolicy:           # v1.26+
    rules:
    - action: Ignore          # 网络错误不计入失败
      onExitCodes:
        operator: In
        values: [7, 28]       # curl 错误码
  template:
    spec:
      containers:
      - name: caller
        image: curl:latest
        command:
        - sh
        - -c
        - "curl --retry 3 --retry-delay 10 https://api.example.com"

# 场景 3: 数据错误立即失败
apiVersion: batch/v1
kind: Job
metadata:
  name: validator
spec:
  backoffLimit: 0             # 不重试
  template:
    spec:
      containers:
      - name: validator
        image: validator:v1
```

---

## 相关参考

### 内部文档

**Domain-33 Kubernetes Events:**
- [01-pod-lifecycle-events.md](./01-pod-lifecycle-events.md) - Pod 生命周期事件（FailedScheduling 等）
- [02-deployment-events.md](./02-deployment-events.md) - Deployment 事件
- [03-statefulset-events.md](./03-statefulset-events.md) - StatefulSet 事件
- [04-daemonset-events.md](./04-daemonset-events.md) - DaemonSet 事件
- [05-replicaset-events.md](./05-replicaset-events.md) - ReplicaSet 事件
- [06-hpa-events.md](./06-hpa-events.md) - HPA 自动扩缩容事件
- [07-pvc-storage-events.md](./07-pvc-storage-events.md) - PVC/PV 存储事件
- [08-service-ingress-events.md](./08-service-ingress-events.md) - Service/Ingress 网络事件

**Troubleshooting 文档:**
- [topic-structural-trouble-shooting/05-workloads/05-job-cronjob-troubleshooting.md](../topic-structural-trouble-shooting/05-workloads/05-job-cronjob-troubleshooting.md) - Job/CronJob 故障排查
- [topic-structural-trouble-shooting/01-control-plane/04-controller-manager-troubleshooting.md](../topic-structural-trouble-shooting/01-control-plane/04-controller-manager-troubleshooting.md) - Controller Manager 故障排查

**Domain 文档:**
- [domain-8-kubernetes-workloads/](../domain-8-kubernetes-workloads/) - Workload 工作负载详解
- [domain-17-batch-processing/](../domain-17-batch-processing/) - 批处理模式最佳实践

### 官方文档

**Job/CronJob:**
- [Jobs - Run to Completion](https://kubernetes.io/docs/concepts/workloads/controllers/job/)
- [CronJob](https://kubernetes.io/docs/concepts/workloads/controllers/cron-jobs/)
- [Pod Failure Policy](https://kubernetes.io/docs/concepts/workloads/controllers/job/#pod-failure-policy) (v1.26+)
- [Success Policy](https://kubernetes.io/docs/concepts/workloads/controllers/job/#success-policy) (v1.28+)

**KEPs (Kubernetes Enhancement Proposals):**
- [KEP-2232: Suspend Job](https://github.com/kubernetes/enhancements/tree/master/keps/sig-apps/2232-suspend-jobs)
- [KEP-2214: Indexed Job](https://github.com/kubernetes/enhancements/tree/master/keps/sig-apps/2214-indexed-job)
- [KEP-3329: Pod Failure Policy](https://github.com/kubernetes/enhancements/tree/master/keps/sig-apps/3329-retriable-and-non-retriable-failures)
- [KEP-3998: Job Success/Completion Policy](https://github.com/kubernetes/enhancements/tree/master/keps/sig-apps/3998-job-success-completion-policy)

### 最佳实践

**资源配置:**
```yaml
# 生产环境推荐配置
apiVersion: batch/v1
kind: Job
metadata:
  name: production-job
  labels:
    app: production-job
    version: v1.0
spec:
  # 执行配置
  completions: 1                    # 单次执行
  parallelism: 1                    # 单并发
  backoffLimit: 3                   # 最多重试 3 次
  activeDeadlineSeconds: 3600       # 1 小时总超时
  ttlSecondsAfterFinished: 86400    # 完成后 24 小时自动删除
  
  # Pod 模板
  template:
    metadata:
      labels:
        app: production-job
    spec:
      restartPolicy: OnFailure      # 失败时重启容器而非重建 Pod
      
      # 资源配置
      containers:
      - name: worker
        image: worker:v1.0
        resources:
          requests:
            cpu: "500m"
            memory: "1Gi"
          limits:
            cpu: "1000m"            # 限制 CPU 防止抢占
            memory: "2Gi"           # 限制内存防止 OOM
        
        # 健康检查
        livenessProbe:
          exec:
            command: ["pgrep", "-f", "worker"]
          initialDelaySeconds: 30
          periodSeconds: 10
        
        # 环境变量
        env:
        - name: LOG_LEVEL
          value: "INFO"
        - name: RETRY_ATTEMPTS
          value: "3"
        
        # 卷挂载
        volumeMounts:
        - name: config
          mountPath: /etc/config
        - name: data
          mountPath: /data
      
      # 卷配置
      volumes:
      - name: config
        configMap:
          name: job-config
      - name: data
        persistentVolumeClaim:
          claimName: job-data-pvc
      
      # 安全上下文
      securityContext:
        runAsNonRoot: true
        runAsUser: 1000
        fsGroup: 1000
        seccompProfile:
          type: RuntimeDefault
```

```yaml
# CronJob 生产配置
apiVersion: batch/v1
kind: CronJob
metadata:
  name: production-cronjob
  labels:
    app: production-cronjob
    version: v1.0
spec:
  # 调度配置
  schedule: "0 2 * * *"                # 每天凌晨 2 点
  timeZone: "Asia/Shanghai"            # 东八区时间
  concurrencyPolicy: Forbid            # 禁止并发执行
  startingDeadlineSeconds: 3600        # 1 小时调度窗口
  successfulJobsHistoryLimit: 7        # 保留最近 7 天成功记录
  failedJobsHistoryLimit: 3            # 保留最近 3 次失败记录
  
  # Job 模板
  jobTemplate:
    spec:
      backoffLimit: 2
      activeDeadlineSeconds: 7200      # 2 小时总超时
      ttlSecondsAfterFinished: 172800  # 完成后 48 小时删除
      
      template:
        metadata:
          labels:
            app: production-cronjob
        spec:
          restartPolicy: OnFailure
          containers:
          - name: worker
            image: worker:v1.0
            resources:
              requests:
                cpu: "200m"
                memory: "512Mi"
              limits:
                cpu: "500m"
                memory: "1Gi"
```

**监控指标:**
```yaml
# ServiceMonitor 示例（Prometheus Operator）
apiVersion: monitoring.coreos.com/v1
kind: ServiceMonitor
metadata:
  name: job-metrics
spec:
  selector:
    matchLabels:
      app: job-exporter
  endpoints:
  - port: metrics
    interval: 30s
    
# 关键指标:
# - kube_job_status_succeeded          # Job 成功数
# - kube_job_status_failed             # Job 失败数
# - kube_job_complete_duration_seconds # Job 执行时长
# - kube_cronjob_next_schedule_time    # CronJob 下次调度时间
# - kube_cronjob_status_last_schedule_time # 最后调度时间
```

---

> **KUDIG-DATABASE** | Domain-33: Kubernetes Events 全域事件大全 | 文档 09/15