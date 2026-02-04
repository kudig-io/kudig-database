# 18 - CronJob 故障排查 (CronJob Troubleshooting)

> **适用版本**: Kubernetes v1.25-v1.32 | **最后更新**: 2026-02 | **参考**: [Kubernetes CronJobs](https://kubernetes.io/docs/concepts/workloads/controllers/cron-jobs/)

---

## 1. CronJob 故障诊断总览 (CronJob Diagnosis Overview)

### 1.1 常见故障现象分类

| 故障类型 | 症状表现 | 影响范围 | 紧急程度 |
|---------|---------|---------|---------|
| **任务未触发** | 未按计划执行 | 定时任务失效 | P1 - 高 |
| **任务执行失败** | Job运行失败 | 业务逻辑中断 | P0 - 紧急 |
| **并发控制问题** | 多实例同时运行 | 资源竞争/数据不一致 | P1 - 高 |
| **资源清理失败** | 历史Job堆积 | 存储资源耗尽 | P2 - 中 |
| **时区配置错误** | 执行时间偏差 | 任务时机错误 | P2 - 中 |

### 1.2 CronJob 架构回顾

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                      CronJob 故障诊断架构                                    │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  ┌─────────────────────────────────────────────────────────────────────┐  │
│  │                       时间调度层                                     │  │
│  │  ┌─────────────┐    ┌─────────────┐    ┌─────────────┐              │  │
│  │  │   Cron表达式  │   │   时区配置   │   │   并发控制   │              │  │
│  │  │ (schedule)  │   │ (timezone)  │   │ (concurrency)│              │  │
│  │  └─────────────┘    └─────────────┘    └─────────────┘              │  │
│  └─────────────────────────────────────────────────────────────────────┘  │
│                              │                                              │
│                              ▼                                              │
│  ┌─────────────────────────────────────────────────────────────────────┐  │
│  │                    CronJob控制器                                     │  │
│  │  ┌───────────────────────────────────────────────────────────────┐  │  │
│  │  │                   kube-controller-manager                     │  │  │
│  │  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐           │  │  │
│  │  │  │  CronJob    │  │   Job       │  │   Sync      │           │  │  │
│  │  │  │ Controller  │  │ Controller  │  │  Loop       │           │  │  │
│  │  │  └─────────────┘  └─────────────┘  └─────────────┘           │  │  │
│  │  └───────────────────────────────────────────────────────────────┘  │  │
│  └─────────────────────────────────────────────────────────────────────┘  │
│                              │                                              │
│         ┌────────────────────┼────────────────────┐                       │
│         │                    │                    │                       │
│         ▼                    ▼                    ▼                       │
│  ┌─────────────┐    ┌─────────────┐    ┌─────────────┐                   │
│  │   Job创建    │    │  Job清理     │    │  状态更新    │                   │
│  │ (根据策略)   │    │ (TTL策略)    │    │ (成功/失败)  │                   │
│  └─────────────┘    └─────────────┘    └─────────────┘                   │
│         │                    │                    │                       │
│         ▼                    ▼                    ▼                       │
│  ┌─────────────────────────────────────────────────────────────────────┐  │
│  │                        Job执行层                                     │  │
│  │  ┌─────────────┐    ┌─────────────┐    ┌─────────────┐              │  │
│  │  │   Job-1     │    │   Job-2     │    │   Job-3     │              │  │
│  │  │ (Running)   │    │ (Completed) │    │ (Failed)    │              │  │
│  │  └─────────────┘    └─────────────┘    └─────────────┘              │  │
│  └─────────────────────────────────────────────────────────────────────┘  │
│                              │                                              │
│                              ▼                                              │
│  ┌─────────────────────────────────────────────────────────────────────┐  │
│  │                        Pod执行层                                     │  │
│  │  ┌─────────────┐    ┌─────────────┐    ┌─────────────┐              │  │
│  │  │  Pod-A      │    │  Pod-B      │    │  Pod-C      │              │  │
│  │  │ (Container) │    │ (Container) │    │ (Container) │              │  │
│  │  └─────────────┘    └─────────────┘    └─────────────┘              │  │
│  └─────────────────────────────────────────────────────────────────────┘  │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

---

## 2. CronJob 基础状态检查 (Basic Status Check)

### 2.1 CronJob 资源检查

```bash
# ========== 1. 基础信息检查 ==========
# 查看所有CronJob
kubectl get cronjobs --all-namespaces

# 查看特定CronJob详细信息
kubectl describe cronjob <cronjob-name> -n <namespace>

# 查看CronJob配置
kubectl get cronjob <cronjob-name> -n <namespace> -o yaml

# ========== 2. 调度状态检查 ==========
# 检查下次执行时间
kubectl get cronjob <cronjob-name> -n <namespace> -o jsonpath='{.status.lastScheduleTime}'

# 检查活跃Job
kubectl get cronjob <cronjob-name> -n <namespace> -o jsonpath='{.status.active}'

# 检查最后一次执行状态
kubectl get cronjob <cronjob-name> -n <namespace> -o jsonpath='{.status.lastSuccessfulTime}'

# ========== 3. 相关Job检查 ==========
# 查看由CronJob创建的Job
kubectl get jobs -n <namespace> --sort-by=.metadata.creationTimestamp

# 查看最近的Job状态
kubectl get jobs -n <namespace> -l cronjob.kubernetes.io/is-created-by=<cronjob-name>

# 查看Job详细信息
kubectl describe job <job-name> -n <namespace>
```

### 2.2 Cron表达式验证

```bash
# ========== Cron表达式语法检查 ==========
# 常见的Cron表达式格式
# ┌───────────── minute (0 - 59)
# │ ┌───────────── hour (0 - 23)
# │ │ ┌───────────── day of the month (1 - 31)
# │ │ │ ┌───────────── month (1 - 12)
# │ │ │ │ ┌───────────── day of the week (0 - 6) (Sunday to Saturday)
# │ │ │ │ │
# │ │ │ │ │
# * * * * *

# 验证Cron表达式工具
# 方法1: 使用在线工具验证
echo "验证Cron表达式: 0 2 * * *" | crontab -l 2>/dev/null || echo "表达式语法正确"

# 方法2: 创建测试CronJob
cat <<EOF | kubectl apply -f -
apiVersion: batch/v1
kind: CronJob
metadata:
  name: test-cron-validation
  namespace: default
spec:
  schedule: "0 2 * * *"  # 每天凌晨2点执行
  jobTemplate:
    spec:
      template:
        spec:
          containers:
          - name: test
            image: busybox
            command: ["echo", "Cron expression validation test"]
          restartPolicy: OnFailure
EOF

# 检查CronJob是否被接受
kubectl get cronjob test-cron-validation
if [ $? -eq 0 ]; then
    echo "Cron表达式语法正确"
    kubectl delete cronjob test-cron-validation
else
    echo "Cron表达式语法错误"
fi
```

---

## 3. 任务未触发问题排查 (Job Not Triggering Troubleshooting)

### 3.1 排查决策树

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                    CronJob 任务未触发排查流程                                 │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│   CronJob任务未按预期执行                                                    │
│          │                                                                  │
│          ▼                                                                  │
│   ┌──────────────────────────────────────────────────────┐                 │
│   │ Step 1: 检查CronJob控制器状态                          │                 │
│   │ kubectl get pods -n kube-system | grep controller    │                 │
│   └──────────────────────────────────────────────────────┘                 │
│          │                                                                  │
│          ├─── 控制器异常 ──▶ 重启控制器Pod                                  │
│          │                                                                  │
│          ▼                                                                  │
│   ┌──────────────────────────────────────────────────────┐                 │
│   │ Step 2: 验证Cron表达式和时区                          │                 │
│   │ kubectl get cronjob <name> -n <ns> -o yaml           │                 │
│   └──────────────────────────────────────────────────────┘                 │
│          │                                                                  │
│          ├─── 表达式错误 ──▶ 修正Cron表达式                                │
│          │                                                                  │
│          ▼                                                                  │
│   ┌──────────────────────────────────────────────────────┐                 │
│   │ Step 3: 检查并发策略和挂起状态                        │                 │
│   │ kubectl describe cronjob <name> -n <ns>              │                 │
│   └──────────────────────────────────────────────────────┘                 │
│          │                                                                  │
│          ├─── 被挂起 ──▶ 取消挂起状态                                     │
│          │         └─▶ kubectl patch cronjob <name> -n <ns> -p '{"spec":{"suspend":false}}' │
│          │                                                                  │
│          ├─── 并发限制 ──▶ 检查并发策略                                   │
│          │                                                                  │
│          ▼                                                                  │
│   ┌──────────────────────────────────────────────────────┐                 │
│   │ Step 4: 检查系统时间同步                              │                 │
│   │ date && kubectl exec -n kube-system <controller-pod> -- date │        │
│   └──────────────────────────────────────────────────────┘                 │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

### 3.2 详细诊断命令

```bash
# ========== 1. 控制器状态检查 ==========
# 检查kube-controller-manager状态
kubectl get pods -n kube-system | grep controller-manager

# 查看控制器日志
kubectl logs -n kube-system -l component=kube-controller-manager --tail=100

# 检查CronJob相关的日志
kubectl logs -n kube-system -l component=kube-controller-manager | grep -i cronjob

# ========== 2. 时间同步检查 ==========
# 检查集群节点时间
kubectl get nodes -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{.status.nodeInfo.osImage}{"\n"}{end}'

# 在节点上检查时间同步
for node in $(kubectl get nodes -o jsonpath='{.items[*].metadata.name}'); do
    echo "=== Node: $node ==="
    kubectl debug node/$node --image=busybox -it -- sh -c "date && ntpstat 2>/dev/null || echo 'NTP not available'"
done

# 检查控制器Pod的时间
kubectl exec -n kube-system -l component=kube-controller-manager -- date

# ========== 3. 配置验证 ==========
# 检查CronJob配置
kubectl get cronjob <cronjob-name> -n <namespace> -o jsonpath='{
    .spec.schedule,
    .spec.timeZone,
    .spec.suspend,
    .spec.concurrencyPolicy,
    .spec.startingDeadlineSeconds
}' | jq

# 验证时区配置
kubectl get cronjob <cronjob-name> -n <namespace> -o jsonpath='{.spec.timeZone}'

# 检查并发策略
kubectl get cronjob <cronjob-name> -n <namespace> -o jsonpath='{.spec.concurrencyPolicy}'
```

---

## 4. 任务执行失败问题排查 (Job Execution Failure Troubleshooting)

### 4.1 Job失败原因分析

```bash
# ========== 1. Job状态检查 ==========
# 查看失败的Job
kubectl get jobs -n <namespace> --field-selector=status.failed>0

# 查看Job详细状态
kubectl describe job <job-name> -n <namespace>

# 检查Job条件状态
kubectl get job <job-name> -n <namespace> -o jsonpath='{.status.conditions}'

# ========== 2. Pod执行状态检查 ==========
# 查看Job创建的Pod
kubectl get pods -n <namespace> --selector=job-name=<job-name>

# 查看失败Pod的日志
kubectl logs -n <namespace> -l job-name=<job-name> --all-containers=true

# 查看Pod详细状态
kubectl describe pod -n <namespace> -l job-name=<job-name>

# ========== 3. 资源限制检查 ==========
# 检查资源配额
kubectl get resourcequota -n <namespace>

# 检查LimitRange
kubectl get limitrange -n <namespace>

# 检查Pod资源请求
kubectl get job <job-name> -n <namespace> -o jsonpath='{.spec.template.spec.containers[*].resources}'
```

### 4.2 常见失败原因及解决方案

```bash
# ========== 1. 镜像拉取失败 ==========
# 检查镜像仓库访问
kubectl get job <job-name> -n <namespace> -o jsonpath='{.spec.template.spec.containers[*].image}'

# 验证镜像拉取密钥
kubectl get secret -n <namespace> | grep pull

# 检查镜像仓库网络连通性
kubectl run debug --image=busybox -n <namespace> -it --rm -- sh
# 在容器内测试: nslookup <registry-domain>

# ========== 2. 权限不足 ==========
# 检查ServiceAccount
kubectl get job <job-name> -n <namespace> -o jsonpath='{.spec.template.spec.serviceAccountName}'

# 验证RBAC权限
kubectl auth can-i get pods --as=system:serviceaccount:<namespace>:<sa-name>

# 检查PodSecurityPolicy
kubectl get psp

# ========== 3. 存储卷问题 ==========
# 检查持久卷声明
kubectl get pvc -n <namespace>

# 验证存储类
kubectl get storageclass

# 检查卷挂载配置
kubectl get job <job-name> -n <namespace> -o jsonpath='{.spec.template.spec.volumes}'
```

---

## 5. 并发控制问题排查 (Concurrency Control Troubleshooting)

### 5.1 并发策略验证

```bash
# ========== 并发策略检查 ==========
# 查看当前并发策略
kubectl get cronjob <cronjob-name> -n <namespace> -o jsonpath='{.spec.concurrencyPolicy}'

# 检查活跃Job数量
kubectl get cronjob <cronjob-name> -n <namespace> -o jsonpath='{.status.active}'

# 列出所有相关Job
kubectl get jobs -n <namespace> -l cronjob.kubernetes.io/is-created-by=<cronjob-name> --sort-by=.metadata.creationTimestamp

# ========== 不同策略的行为测试 ==========
# Allow策略测试 (允许多个Job同时运行)
cat <<EOF | kubectl apply -f -
apiVersion: batch/v1
kind: CronJob
metadata:
  name: test-concurrency-allow
  namespace: default
spec:
  schedule: "* * * * *"  # 每分钟执行
  concurrencyPolicy: Allow
  jobTemplate:
    spec:
      template:
        spec:
          containers:
          - name: slow-task
            image: busybox
            command: ["sleep", "90"]  # 执行90秒
          restartPolicy: OnFailure
EOF

# Forbid策略测试 (禁止并发)
cat <<EOF | kubectl apply -f -
apiVersion: batch/v1
kind: CronJob
metadata:
  name: test-concurrency-forbid
  namespace: default
spec:
  schedule: "* * * * *"
  concurrencyPolicy: Forbid
  jobTemplate:
    spec:
      template:
        spec:
          containers:
          - name: slow-task
            image: busybox
            command: ["sleep", "90"]
          restartPolicy: OnFailure
EOF

# Replace策略测试 (替换旧Job)
cat <<EOF | kubectl apply -f -
apiVersion: batch/v1
kind: CronJob
metadata:
  name: test-concurrency-replace
  namespace: default
spec:
  schedule: "* * * * *"
  concurrencyPolicy: Replace
  jobTemplate:
    spec:
      template:
        spec:
          containers:
          - name: slow-task
            image: busybox
            command: ["sleep", "90"]
          restartPolicy: OnFailure
EOF
```

### 5.2 并发冲突检测

```bash
# ========== 并发冲突监控 ==========
# 监控活跃Job数量
watch -n 5 "kubectl get cronjob <cronjob-name> -n <namespace> -o jsonpath='{.status.active}'"

# 查看Job创建时间线
kubectl get jobs -n <namespace> -l cronjob.kubernetes.io/is-created-by=<cronjob-name> -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{.metadata.creationTimestamp}{"\n"}{end}' | sort -k2

# 检查Job重叠执行
kubectl get jobs -n <namespace> -l cronjob.kubernetes.io/is-created-by=<cronjob-name> -o jsonpath='{
    range .items[*]
}{
        .metadata.name
}{
        "\t"
}{
        .status.startTime
}{
        "\t"
}{
        .status.completionTime
}{
        "\n"
}{
    end
}'

# ========== 资源竞争分析 ==========
# 检查共享资源锁
kubectl get configmaps -n <namespace> | grep lock

# 检查数据库连接池
kubectl exec -n <namespace> <pod-name> -- netstat -an | grep :5432

# 监控存储I/O竞争
kubectl top pods -n <namespace> -l job-name=<job-name>
```

---

## 6. 资源清理和维护 (Resource Cleanup and Maintenance)

### 6.1 历史Job清理策略

```bash
# ========== TTL策略配置 ==========
# 为现有CronJob添加TTL策略
kubectl patch cronjob <cronjob-name> -n <namespace> -p '{
    "spec": {
        "successfulJobsHistoryLimit": 3,
        "failedJobsHistoryLimit": 1
    }
}'

# 创建带TTL策略的新CronJob
cat <<EOF | kubectl apply -f -
apiVersion: batch/v1
kind: CronJob
metadata:
  name: cleanup-example
  namespace: default
spec:
  schedule: "0 2 * * *"  # 每天凌晨2点
  successfulJobsHistoryLimit: 5  # 保留5个成功的Job
  failedJobsHistoryLimit: 2      # 保留2个失败的Job
  jobTemplate:
    spec:
      ttlSecondsAfterFinished: 86400  # 24小时后自动清理完成的Job
      template:
        spec:
          containers:
          - name: cleanup-task
            image: busybox
            command: ["find", "/tmp", "-type", "f", "-mtime", "+7", "-delete"]
          restartPolicy: OnFailure
EOF

# ========== 手动清理脚本 ==========
# 清理超过指定天数的历史Job
cat <<'EOF' > cleanup-old-jobs.sh
#!/bin/bash

NAMESPACE=${1:-default}
DAYS_OLD=${2:-7}

echo "Cleaning up Jobs older than $DAYS_OLD days in namespace: $NAMESPACE"

# 计算截止时间
CUTOFF_DATE=$(date -d "$DAYS_OLD days ago" -Iseconds)

# 获取需要清理的Job
JOBS_TO_DELETE=$(kubectl get jobs -n $NAMESPACE -o json | jq -r ".items[] | select(.status.completionTime < \"$CUTOFF_DATE\") | .metadata.name")

if [ -z "$JOBS_TO_DELETE" ]; then
    echo "No old jobs found to clean up"
    exit 0
fi

echo "Found jobs to delete:"
echo "$JOBS_TO_DELETE"

# 确认删除
read -p "Proceed with deletion? (y/N): " -n 1 -r
echo
if [[ $REPLY =~ ^[Yy]$ ]]; then
    echo "$JOBS_TO_DELETE" | while read job; do
        echo "Deleting job: $job"
        kubectl delete job $job -n $NAMESPACE
    done
    echo "Cleanup completed"
else
    echo "Cleanup cancelled"
fi
EOF

chmod +x cleanup-old-jobs.sh

# ========== 自动化清理CronJob ==========
cat <<EOF | kubectl apply -f -
apiVersion: batch/v1
kind: CronJob
metadata:
  name: job-cleanup
  namespace: default
spec:
  schedule: "0 3 * * *"  # 每天凌晨3点执行清理
  successfulJobsHistoryLimit: 2
  failedJobsHistoryLimit: 1
  jobTemplate:
    spec:
      template:
        spec:
          serviceAccountName: job-cleanup-sa
          containers:
          - name: cleaner
            image: bitnami/kubectl:latest
            command:
            - /bin/sh
            - -c
            - |
              #!/bin/sh
              set -e
              
              # 清理7天前的Job
              kubectl get jobs --all-namespaces -o json | \
              jq -r '.items[] | select(.status.completionTime != null) | 
              select(.status.completionTime < "'$(date -d '7 days ago' -Iseconds)'") | 
              .metadata.namespace + "/" + .metadata.name' | \
              while read job; do
                echo "Deleting old job: $job"
                kubectl delete job -n ${job%/*} ${job##*/}
              done
              
              # 清理孤立的Pod
              kubectl get pods --all-namespaces --field-selector=status.phase!=Running -o json | \
              jq -r '.items[] | select(.metadata.ownerReferences == null) | 
              .metadata.namespace + "/" + .metadata.name' | \
              while read pod; do
                echo "Deleting orphaned pod: $pod"
                kubectl delete pod -n ${pod%/*} ${pod##*/}
              done
          restartPolicy: OnFailure
EOF
```

### 6.2 存储资源监控

```bash
# ========== 存储使用监控 ==========
# 监控Job相关的存储增长
cat <<'EOF' > monitor-job-storage.sh
#!/bin/bash

NAMESPACE=${1:-default}
CHECK_INTERVAL=${2:-300}  # 5分钟检查一次

echo "Monitoring Job storage usage in namespace: $NAMESPACE"
echo "Check interval: $CHECK_INTERVAL seconds"

while true; do
    TIMESTAMP=$(date '+%Y-%m-%d %H:%M:%S')
    
    # 统计Job资源使用
    JOB_COUNT=$(kubectl get jobs -n $NAMESPACE --no-headers | wc -l)
    POD_COUNT=$(kubectl get pods -n $NAMESPACE -l job-name --no-headers | wc -l)
    
    # 统计存储使用 (如果有PV)
    PV_USAGE=$(kubectl get pvc -n $NAMESPACE -o jsonpath='{range .items[*]}{.spec.resources.requests.storage}{"\n"}{end}' 2>/dev/null | awk '{sum+=$1} END {print sum}')
    
    echo "$TIMESTAMP - Jobs: $JOB_COUNT, Pods: $POD_COUNT, Storage: ${PV_USAGE:-0}Gi"
    
    # 告警阈值检查
    if [ $JOB_COUNT -gt 100 ]; then
        echo "$TIMESTAMP - WARNING: High job count ($JOB_COUNT)"
    fi
    
    sleep $CHECK_INTERVAL
done
EOF

chmod +x monitor-job-storage.sh

# ========== 资源配额设置 ==========
# 为Job设置资源限制
cat <<EOF | kubectl apply -f -
apiVersion: v1
kind: ResourceQuota
metadata:
  name: job-resource-limits
  namespace: default
spec:
  hard:
    count/jobs.batch: "50"           # 最多50个Job
    count/cronjobs.batch: "10"       # 最多10个CronJob
    requests.cpu: "10"               # CPU请求总量限制
    requests.memory: 20Gi            # 内存请求总量限制
    limits.cpu: "20"                 # CPU限制总量
    limits.memory: 40Gi              # 内存限制总量
EOF
```

---

## 7. 监控告警配置 (Monitoring and Alerting)

### 7.1 关键指标监控

```bash
# ========== Job状态监控 ==========
cat <<EOF | kubectl apply -f -
apiVersion: monitoring.coreos.com/v1
kind: ServiceMonitor
metadata:
  name: cronjob-monitor
  namespace: monitoring
spec:
  selector:
    matchLabels:
      k8s-app: kube-controller-manager
  endpoints:
  - port: http-metrics
    path: /metrics
    interval: 30s
    metricRelabelings:
    - sourceLabels: [__name__]
      regex: 'kube_job.*'
      action: keep
EOF

# ========== 告警规则配置 ==========
cat <<EOF | kubectl apply -f -
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: cronjob-alerts
  namespace: monitoring
spec:
  groups:
  - name: cronjob.rules
    rules:
    - alert: CronJobNotRunning
      expr: kube_cronjob_next_schedule_time - time() > 3600
      for: 5m
      labels:
        severity: warning
      annotations:
        summary: "CronJob {{ \$labels.cronjob }} has not run for over 1 hour"
        
    - alert: CronJobFailed
      expr: increase(kube_job_failed{job_name=~".*cronjob.*"}[15m]) > 0
      for: 2m
      labels:
        severity: critical
      annotations:
        summary: "CronJob {{ \$labels.job_name }} has failed"
        
    - alert: TooManyActiveJobs
      expr: kube_cronjob_active > 5
      for: 5m
      labels:
        severity: warning
      annotations:
        summary: "CronJob {{ \$labels.cronjob }} has too many active jobs ({{ \$value }})"
        
    - alert: JobQueueBacklog
      expr: count(kube_job_status_active > 0) > 20
      for: 10m
      labels:
        severity: warning
      annotations:
        summary: "Too many active jobs in queue ({{ \$value }})"
EOF
```

### 7.2 性能分析工具

```bash
# ========== CronJob性能分析 ==========
# 创建性能监控脚本
cat <<'EOF' > cronjob-performance-analyzer.sh
#!/bin/bash

NAMESPACE=${1:-default}
OUTPUT_FILE=${2:-cronjob-performance-report.txt}

echo "CronJob Performance Analysis Report" > $OUTPUT_FILE
echo "Namespace: $NAMESPACE" >> $OUTPUT_FILE
echo "Generated: $(date)" >> $OUTPUT_FILE
echo "========================================" >> $OUTPUT_FILE

# 分析每个CronJob的执行情况
kubectl get cronjobs -n $NAMESPACE -o name | cut -d/ -f2 | while read cronjob; do
    echo "Analyzing CronJob: $cronjob" >> $OUTPUT_FILE
    
    # 获取最近的执行记录
    kubectl get jobs -n $NAMESPACE -l cronjob.kubernetes.io/is-created-by=$cronjob \
        --sort-by=.metadata.creationTimestamp \
        -o jsonpath='{range .items[-5:]}{.metadata.name}{"\t"}{.status.startTime}{"\t"}{.status.completionTime}{"\t"}{.status.succeeded}{"\n"}{end}' \
        >> $OUTPUT_FILE
    
    # 计算平均执行时间
    EXEC_TIMES=$(kubectl get jobs -n $NAMESPACE -l cronjob.kubernetes.io/is-created-by=$cronjob \
        -o jsonpath='{range .items[*]}{.status.completionTime}{"\t"}{.status.startTime}{"\n"}{end}' | \
        head -5 | while read completion start; do
            if [ -n "$completion" ] && [ -n "$start" ]; then
                echo $(( $(date -d "$completion" +%s) - $(date -d "$start" +%s) ))
            fi
        done)
    
    if [ -n "$EXEC_TIMES" ]; then
        AVG_TIME=$(echo $EXEC_TIMES | awk '{sum+=$1; count++} END {print int(sum/count)}')
        echo "Average execution time: ${AVG_TIME}s" >> $OUTPUT_FILE
    fi
    
    echo "" >> $OUTPUT_FILE
done

echo "Analysis complete. Report saved to $OUTPUT_FILE"
EOF

chmod +x cronjob-performance-analyzer.sh

# ========== 执行时间预测 ==========
# 基于历史数据分析预测下一次执行时间
cat <<'EOF' > predict-next-execution.sh
#!/bin/bash

CRONJOB_NAME=$1
NAMESPACE=${2:-default}

echo "Predicting next execution time for CronJob: $CRONJOB_NAME"

# 获取Cron表达式
SCHEDULE=$(kubectl get cronjob $CRONJOB_NAME -n $NAMESPACE -o jsonpath='{.spec.schedule}')
echo "Schedule: $SCHEDULE"

# 获取上次执行时间
LAST_RUN=$(kubectl get cronjob $CRONJOB_NAME -n $NAMESPACE -o jsonpath='{.status.lastScheduleTime}')
echo "Last run: ${LAST_RUN:-Never}"

# 使用python计算下次执行时间
python3 <<PYEOF
import croniter
from datetime import datetime
import sys

schedule = "$SCHEDULE"
last_run = "$LAST_RUN"

if last_run:
    base_time = datetime.fromisoformat(last_run.replace('Z', '+00:00'))
else:
    base_time = datetime.now()

cron = croniter.croniter(schedule, base_time)
next_run = cron.get_next(datetime)
print(f"Next scheduled run: {next_run}")
print(f"Time until next run: {(next_run - datetime.now()).total_seconds()/60:.1f} minutes")
PYEOF
EOF

chmod +x predict-next-execution.sh
```

---