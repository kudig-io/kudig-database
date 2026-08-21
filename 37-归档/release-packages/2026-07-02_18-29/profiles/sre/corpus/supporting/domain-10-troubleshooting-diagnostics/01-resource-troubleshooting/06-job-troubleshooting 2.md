---
title: Job 故障排查
description: '# 22 - Job 故障排查 (Job Troubleshooting)'
summary: 'kubectl get job <job-name> -n <namespace> -o jsonpath='{.spec.completionMode}''
category: troubleshooting
tags:
- job
- batch
- parallel
- ttl-seconds
- backoff
- controller-manager
- prometheus
- rag
- etcd
- apiserver
tier: core
created: '2026-05-23'
last_updated: '2026-07-02'
difficulty: intermediate
reading_level: intermediate
audience:
- SRE
- 运维工程师
- 技术支持
estimated_read_time: 5min
intent_queries:
- Job 执行失败
- 批处理任务不完成
- Pod 退出码非零
trigger_keywords:
- Job
- 故障排查
- troubleshooting
prerequisites:
- kubectl-basics
- troubleshooting-methodology
- prometheus-basics
- cni-basics
- etcd-basics
k8s_versions:
- 1.25
- 1.26
- 1.27
- 1.28
- 1.29
- 1.3
- 1.31
- 1.32
authors:
- name: KUDIG Team
  role: contributor
cross_refs:
- type: domain
  path: ../domain-01-cluster-fundamentals/
  label: '相关知识域: domain-01-cluster-fundamentals'
- type: domain
  path: ../domain-03-networking-traffic/
  label: '相关知识域: domain-03-networking-traffic'
- type: domain
  path: ../domain-06-observability/
  label: '相关知识域: domain-06-observability'
- type: fta
  path: ../domain-10-troubleshooting-diagnostics/topic-fta/list/job-cronjob-fta.md
  label: '故障树: job-cronjob'
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 22 - Job 故障排查 (Job Troubleshooting)

---

<!-- chunk: 1. Job 故障诊断总览 (Job Diagnosis Overview) -->
## 1. Job 故障诊断总览 (Job Diagnosis Overview)

### 1.1 常见问题现象分类

| 问题类型 | 症状表现 | 影响范围 | 紧急程度 |
|---------|---------|---------|---------|
| **Pod启动失败** | Job Pod无法创建 | 任务无法执行 | P0 - 紧急 |
| **执行超时** | Job运行时间过长 | 资源占用 | P1 - 高 |
| **执行失败** | 容器退出码非0 | 业务逻辑中断 | P0 - 紧急 |
| **并行控制问题** | 并发数异常 | 资源竞争 | P1 - 高 |
| **清理失败** | 完成后未清理 | 资源泄漏 | P2 - 中 |

### 1.2 Job 架构回顾

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                        Job 故障诊断架构                                      │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  ┌─────────────────────────────────────────────────────────────────────┐  │
│  │                        Job控制器                                     │  │
│  │  ┌───────────────────────────────────────────────────────────────┐  │  │
│  │  │                   kube-controller-manager                     │  │  │
│  │  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐           │  │  │
│  │  │  │   Job       │  │   Pod       │  │   并发控制   │           │  │  │
│  │  │  │ Controller  │  │ Controller  │  │   Manager   │           │  │  │
│  │  │  └─────────────┘  └─────────────┘  └─────────────┘           │  │  │
│  │  └───────────────────────────────────────────────────────────────┘  │  │
│  └─────────────────────────────────────────────────────────────────────┘  │
│                              │                                              │
│         ┌────────────────────┼────────────────────┐                       │
│         │                    │                    │                       │
│         ▼                    ▼                    ▼                       │
│  ┌─────────────┐    ┌─────────────┐    ┌─────────────┐                   │
│  │   任务创建   │   │   Pod调度    │   │   状态监控   │                   │
│  │ (Create)    │   │ (Schedule)  │   │ (Monitor)   │                   │
│  └─────────────┘    └─────────────┘    └─────────────┘                   │
│         │                    │                    │                       │
│         ▼                    ▼                    ▼                       │
│  ┌─────────────────────────────────────────────────────────────────────┐  │
│  │                        Pod执行层                                     │  │
│  │  ┌─────────────┐    ┌─────────────┐    ┌─────────────┐              │  │
│  │  │   Pod-1     │    │   Pod-2     │    │   Pod-3     │              │  │
│  │  │ (Running)   │    │ (Completed) │    │ (Failed)    │              │  │
│  │  └─────────────┘    └─────────────┘    └─────────────┘              │  │
│  └─────────────────────────────────────────────────────────────────────┘  │
│                              │                                              │
│                              ▼                                              │
│  ┌─────────────────────────────────────────────────────────────────────┐  │
│  │                      任务结果处理                                     │  │
│  │  ┌─────────────┐    ┌─────────────┐    ┌─────────────┐              │  │
│  │  │   成功处理   │   │   失败重试   │   │   清理回收   │              │  │
│  │  │ (Success)   │   │ (Retry)     │   │ (Cleanup)   │              │  │
│  │  └─────────────┘    └─────────────┘    └─────────────┘              │  │
│  └─────────────────────────────────────────────────────────────────────┘  │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

---

<!-- chunk: 2. Job 基础状态检查 (Basic Status Check) -->
## 2. Job 基础状态检查 (Basic Status Check)

### 2.1 Job 资源状态验证

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# ========== 1. 基础信息检查 ==========
# 查看所有Job
kubectl get jobs --all-namespaces

# 查看特定Job详细信息
kubectl describe job <job-name> -n <namespace>

# 检查Job配置
kubectl get job <job-name> -n <namespace> -o yaml

# ========== 2. 执行状态检查 ==========
# 查看Job执行状态
kubectl get job <job-name> -n <namespace> -o jsonpath='{
    .status.succeeded,
    .status.failed,
    .status.active,
    .status.startTime,
    .status.completionTime
}'

# 检查相关Pod
kubectl get pods -n <namespace> --selector=job-name=<job-name>

# 查看Pod详细状态
kubectl describe pods -n <namespace> --selector=job-name=<job-name>
```
### 2.2 并行和完成配置检查

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# ========== 并行配置检查 ==========
# 检查并行度设置
kubectl get job <job-name> -n <namespace> -o jsonpath='{
    .spec.parallelism,
    .spec.completions
}'

# 验证实际并行执行情况
kubectl get pods -n <namespace> --selector=job-name=<job-name> --field-selector=status.phase=Running

# ========== 完成条件检查 ==========
# 检查完成条件
kubectl get job <job-name> -n <namespace> -o jsonpath='{.spec.completionMode}'

# 验证成功Pod数量
SUCCESSFUL_PODS=$(kubectl get pods -n <namespace> --selector=job-name=<job-name> --field-selector=status.phase=Succeeded --no-headers | wc -l)
echo "Successful pods: $SUCCESSFUL_PODS"
```
---

<!-- chunk: 3. 执行失败问题排查 (Execution Failure Troubleshooting) -->
## 3. 执行失败问题排查 (Execution Failure Troubleshooting)

### 3.1 Pod启动失败

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# ========== 1. 调度失败分析 ==========
# 查看调度失败事件
kubectl get events -n <namespace> --field-selector involvedObject.kind=Pod,reason=FailedScheduling

# 检查资源配额
kubectl get resourcequota -n <namespace>

# 验证节点资源
kubectl describe nodes | grep -A5 "Allocated resources"

# ========== 2. 镜像拉取失败 ==========
# 检查镜像仓库访问
kubectl get job <job-name> -n <namespace> -o jsonpath='{.spec.template.spec.containers[*].image}'

# 验证镜像拉取密钥
kubectl get secret -n <namespace> | grep pull

# 查看镜像拉取事件
kubectl get events -n <namespace> --field-selector involvedObject.kind=Pod,reason=ErrImagePull
```
### 3.2 容器执行失败

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# ========== 1. 退出码分析 ==========
# 查看Pod退出码
kubectl get pods -n <namespace> --selector=job-name=<job-name> -o jsonpath='{
    range .items[*]
}{
        .metadata.name
}{
        "\t"
}{
        .status.containerStatuses[*].state.terminated.exitCode
}{
        "\t"
}{
        .status.containerStatuses[*].state.terminated.reason
}{
        "\n"
}{
    end
}'

# 查看容器日志
kubectl logs -n <namespace> --selector=job-name=<job-name> --all-containers=true

# ========== 2. 资源限制问题 ==========
# 检查OOMKilled状态
kubectl get pods -n <namespace> --selector=job-name=<job-name> -o jsonpath='{
    range .items[*]
}{
        .metadata.name
}{
        "\t"
}{
        .status.containerStatuses[*].state.terminated.reason
}{
        "\n"
}{
    end
}' | grep OOMKilled

# 验证资源请求
kubectl get job <job-name> -n <namespace> -o jsonpath='{.spec.template.spec.containers[*].resources}'
```
---

<!-- chunk: 4. 超时和性能问题 (Timeout and Performance Issues) -->
## 4. 超时和性能问题 (Timeout and Performance Issues)

### 4.1 执行时间监控

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# ========== 1. 执行时间分析 ==========
# 计算Job执行时间
kubectl get job <job-name> -n <namespace> -o jsonpath='{
    .status.startTime,
    .status.completionTime
}' | awk '{
    start=strftime("%s", systime())
    end=strftime("%s", systime())
    print "Duration: " (end-start) " seconds"
}'

# 监控长时间运行的Job
kubectl get jobs -n <namespace> -o jsonpath='{
    range .items[*]
}{
        .metadata.name
}{
        "\t"
}{
        .status.startTime
}{
        "\n"
}{
    end
}' | while read job start_time; do
    if [ -n "$start_time" ]; then
        START_SECONDS=$(date -d "$start_time" +%s)
        CURRENT_SECONDS=$(date +%s)
        DURATION=$((CURRENT_SECONDS - START_SECONDS))
        if [ $DURATION -gt 3600 ]; then  # 超过1小时
            echo "Long running job: $job (${DURATION}s)"
        fi
    fi
done

# ========== 2. 超时配置检查 ==========
# 检查活跃Deadline
kubectl get job <job-name> -n <namespace> -o jsonpath='{.spec.activeDeadlineSeconds}'

# 验证Pod TTL设置
kubectl get job <job-name> -n <namespace> -o jsonpath='{.spec.ttlSecondsAfterFinished}'
```
### 4.2 性能优化建议

```bash
# ========== 资源优化配置 ==========
cat <<EOF > optimized-job.yaml
apiVersion: batch/v1
kind: Job
metadata:
  name: optimized-job
  namespace: default
spec:
  parallelism: 3          # 并行度
  completions: 10         # 需要完成的Pod数
  activeDeadlineSeconds: 3600  # 1小时超时
  ttlSecondsAfterFinished: 300 # 完成后5分钟清理
  backoffLimit: 4         # 最多重试4次
  template:
    spec:
      containers:
      - name: worker
        image: my-worker:latest
        resources:
          requests:
            cpu: "100m"
            memory: "128Mi"
          limits:
            cpu: "500m"
            memory: "512Mi"
        env:
        - name: WORKER_ID
          valueFrom:
            fieldRef:
              fieldPath: metadata.name
      restartPolicy: OnFailure
EOF

# ========== 批量任务优化 ==========
# 创建带有索引的任务
cat <<EOF > indexed-job.yaml
apiVersion: batch/v1
kind: Job
metadata:
  name: indexed-job
  namespace: default
spec:
  completionMode: Indexed  # Kubernetes 1.24+
  completions: 5
  parallelism: 5
  template:
    spec:
      containers:
      - name: worker
        image: my-worker:latest
        env:
        - name: JOB_COMPLETION_INDEX
          valueFrom:
            fieldRef:
              fieldPath: metadata.annotations['batch.kubernetes.io/job-completion-index']
EOF
```

---

<!-- chunk: 5. 重试和失败处理 (Retry and Failure Handling) -->
## 5. 重试和失败处理 (Retry and Failure Handling)

### 5.1 重试策略配置

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# ========== 重试限制检查 ==========
# 查看当前重试次数
kubectl get job <job-name> -n <namespace> -o jsonpath='{.status.failed}'

# 检查重试限制
kubectl get job <job-name> -n <namespace> -o jsonpath='{.spec.backoffLimit}'

# ========== 自定义重试逻辑 ==========
# 带有条件重试的Job
cat <<EOF > conditional-retry-job.yaml
apiVersion: batch/v1
kind: Job
metadata:
  name: conditional-retry-job
  namespace: default
spec:
  backoffLimit: 3
  template:
    spec:
      containers:
      - name: worker
        image: my-worker:latest
        command: ["/bin/sh", "-c"]
        args:
        - |
          # 业务逻辑
          if [ \$((RANDOM % 3)) -eq 0 ]; then
            echo "Simulating failure"
            exit 1
          else
            echo "Task completed successfully"
            exit 0
          fi
      restartPolicy: OnFailure
EOF

# ========== 失败处理策略 ==========
# 带有清理逻辑的Job
cat <<EOF > cleanup-job.yaml
apiVersion: batch/v1
kind: Job
metadata:
  name: cleanup-job
  namespace: default
spec:
  template:
    spec:
      containers:
      - name: main
        image: my-worker:latest
        command: ["./process-data.sh"]
      - name: cleanup
        image: busybox
        command: ["./cleanup.sh"]
        env:
        - name: CLEANUP_ON_FAILURE
          value: "true"
      restartPolicy: Never
  ttlSecondsAfterFinished: 600
EOF
```
### 5.2 失败分析工具

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源
> - `kubectl delete`：删除资源（可由声明式清单重建）

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# ========== Job失败分析脚本 ==========
cat <<'EOF' > job-failure-analyzer.sh
#!/bin/bash

JOB_NAME=$1
NAMESPACE=${2:-default}

echo "Analyzing failures for Job: $JOB_NAME in namespace: $NAMESPACE"

# 收集失败Pod信息
FAILED_PODS=$(kubectl get pods -n $NAMESPACE --selector=job-name=$JOB_NAME --field-selector=status.phase=Failed -o jsonpath='{.items[*].metadata.name}')

if [ -z "$FAILED_PODS" ]; then
    echo "No failed pods found"
    exit 0
fi

echo "Found failed pods: $FAILED_PODS"

# 分析每个失败Pod
for pod in $FAILED_PODS; do
    echo "=== Analyzing Pod: $pod ==="
    
    # 查看Pod状态
    kubectl describe pod $pod -n $NAMESPACE | grep -A20 "Status:"
    
    # 查看容器状态
    kubectl describe pod $pod -n $NAMESPACE | grep -A10 "Container ID:"
    
    # 查看日志
    echo "--- Last 50 log lines ---"
    kubectl logs $pod -n $NAMESPACE --tail=50 2>/dev/null || echo "No logs available"
    
    # 查看事件
    echo "--- Related events ---"
    kubectl get events -n $NAMESPACE --field-selector involvedObject.name=$pod
done

# 统计失败原因
echo "=== Failure Statistics ==="
kubectl get pods -n $NAMESPACE --selector=job-name=$JOB_NAME -o jsonpath='{
    range .items[*]
}{
        .status.containerStatuses[*].state.terminated.reason
}{
        "\n"
}{
    end
}' | sort | uniq -c
EOF

chmod +x job-failure-analyzer.sh

# ========== 自动重试机制 ==========
cat <<'EOF' > auto-retry-job.sh
#!/bin/bash

JOB_NAME=$1
NAMESPACE=${2:-default}
MAX_RETRIES=${3:-3}

retry_count=0

while [ $retry_count -lt $MAX_RETRIES ]; do
    echo "Attempt $((retry_count + 1)) for job: $JOB_NAME"
    
    # 删除现有Job
    kubectl delete job $JOB_NAME -n $NAMESPACE 2>/dev/null
    
    # 创建新的Job
    kubectl create job $JOB_NAME --image=my-worker:latest -n $NAMESPACE -- ./my-task.sh
    
    # 等待Job完成
    while true; do
        STATUS=$(kubectl get job $JOB_NAME -n $NAMESPACE -o jsonpath='{.status.conditions[?(@.type=="Complete")].status}')
        FAILED=$(kubectl get job $JOB_NAME -n $NAMESPACE -o jsonpath='{.status.failed}')
        
        if [ "$STATUS" = "True" ]; then
            echo "Job completed successfully"
            exit 0
        elif [ -n "$FAILED" ] && [ "$FAILED" -gt 0 ]; then
            echo "Job failed, retrying..."
            retry_count=$((retry_count + 1))
            break
        fi
        
        sleep 10
    done
done

echo "Max retries reached, job failed permanently"
exit 1
EOF

chmod +x auto-retry-job.sh
```
---

<!-- chunk: 6. 监控和告警配置 (Monitoring and Alerting) -->
## 6. 监控和告警配置 (Monitoring and Alerting)

### 6.1 Job状态监控

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# ========== Job监控指标 ==========
cat <<EOF | kubectl apply -f -
apiVersion: monitoring.coreos.com/v1
kind: ServiceMonitor
metadata:
  name: job-monitor
  namespace: monitoring
spec:
  selector:
    matchLabels:
      k8s-app: kube-state-metrics
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
  name: job-alerts
  namespace: monitoring
spec:
  groups:
  - name: job.rules
    rules:
    - alert: JobFailed
      expr: kube_job_status_failed > 0
      for: 2m
      labels:
        severity: critical
      annotations:
        summary: "Job {{ \$labels.exported_job }} has failed pods (namespace {{ \$labels.namespace }})"
        
    - alert: JobLongRunning
      expr: time() - kube_job_status_start_time > 3600
      for: 5m
      labels:
        severity: warning
      annotations:
        summary: "Job {{ \$labels.exported_job }} has been running for over 1 hour (namespace {{ \$labels.namespace }})"
        
    - alert: JobStuck
      expr: kube_job_complete == 0 and kube_job_status_active == 0 and kube_job_status_failed == 0
      for: 10m
      labels:
        severity: warning
      annotations:
        summary: "Job {{ \$labels.exported_job }} appears to be stuck (namespace {{ \$labels.namespace }})"
        
    - alert: HighJobFailureRate
      expr: increase(kube_job_status_failed[15m]) > 5
      for: 5m
      labels:
        severity: critical
      annotations:
        summary: "High job failure rate detected in namespace {{ \$labels.namespace }}"
EOF
```
### 6.2 性能分析工具

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# ========== Job性能分析脚本 ==========
cat <<'EOF' > job-performance-analyzer.sh
#!/bin/bash

NAMESPACE=${1:-default}
DURATION=${2:-3600}  # 分析过去1小时的数据

echo "Analyzing Job performance in namespace: $NAMESPACE"
echo "Time window: Last $DURATION seconds"

CUTOFF_TIME=$(date -d "@$(($(date +%s) - DURATION))" -Iseconds)

# 收集Job统计数据
kubectl get jobs -n $NAMESPACE -o jsonpath='{
    range .items[?(@.status.startTime >= "'$CUTOFF_TIME'")]
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
        "\t"
}{
        .status.succeeded
}{
        "\t"
}{
        .status.failed
}{
        "\n"
}{
    end
}' > /tmp/job-stats.txt

# 分析结果
echo "=== Job Performance Summary ==="
echo "Total jobs analyzed: $(wc -l < /tmp/job-stats.txt)"
echo ""

# 计算平均执行时间
awk -F'\t' '{
    if ($3 != "") {
        start = $2
        end = $3
        duration = (systime() - mktime(gensub(/[-:TZ]/, " ", "g", end))) - (systime() - mktime(gensub(/[-:TZ]/, " ", "g", start)))
        total_duration += duration
        count++
    }
} END {
    if (count > 0) {
        printf "Average execution time: %.2f seconds\n", total_duration/count
    }
}' /tmp/job-stats.txt

# 成功率统计
awk -F'\t' '{
    total++
    if ($4 > 0) success++
    if ($5 > 0) failed++
} END {
    printf "Success rate: %.2f%% (%d/%d)\n", (success/total)*100, success, total
    printf "Failure rate: %.2f%% (%d/%d)\n", (failed/total)*100, failed, total
}' /tmp/job-stats.txt

# 清理临时文件
rm /tmp/job-stats.txt
EOF

chmod +x job-performance-analyzer.sh

# ========== Job资源使用监控 ==========
cat <<'EOF' > job-resource-monitor.sh
#!/bin/bash

JOB_NAME=$1
NAMESPACE=${2:-default}
INTERVAL=${3:-30}

echo "Monitoring resource usage for Job: $JOB_NAME"

while true; do
    TIMESTAMP=$(date '+%Y-%m-%d %H:%M:%S')
    
    # 获取相关Pod
    PODS=$(kubectl get pods -n $NAMESPACE --selector=job-name=$JOB_NAME -o jsonpath='{.items[*].metadata.name}')
    
    if [ -z "$PODS" ]; then
        echo "$TIMESTAMP - No pods found for job $JOB_NAME"
        sleep $INTERVAL
        continue
    fi
    
    # 监控每个Pod的资源使用
    for pod in $PODS; do
        CPU=$(kubectl top pod $pod -n $NAMESPACE --no-headers 2>/dev/null | awk '{print $2}')
        MEM=$(kubectl top pod $pod -n $NAMESPACE --no-headers 2>/dev/null | awk '{print $3}')
        echo "$TIMESTAMP - $pod: CPU=$CPU, Memory=$MEM"
    done
    
    sleep $INTERVAL
done
EOF

chmod +x job-resource-monitor.sh
```
---

---

<!-- chunk: Obsidian 相关文档 -->
## Obsidian 相关文档

- domain-10-troubleshooting-diagnostics KUDIG Database — Global MOC
- [[domain-10-troubleshooting-diagnostics/README.md|Domain-12 故障排查 (Troubleshooting)]]
- Domain-12 故障排查 — 开源项目索引
- [[domain-10-troubleshooting-diagnostics/核心排障/01-control-plane-apiserver-troubleshooting.md|[[API Server 故障排查|API Server 故障排查]]]]
- [[domain-10-troubleshooting-diagnostics/核心排障/02-control-plane-etcd-troubleshooting.md|[[etcd 故障排查|etcd 故障排查]]]]
- [[domain-10-troubleshooting-diagnostics/核心排障/03-networking-cni-troubleshooting.md|CNI 网络插件故障排查]]
- [[domain-10-troubleshooting-diagnostics/核心排障/04-storage-csi-troubleshooting.md|CSI 存储驱动故障排查]]
- [[domain-10-troubleshooting-diagnostics/核心排障/05-pod-pending-diagnosis.md|Pod Pending 状态深度诊断]]
- [[domain-10-troubleshooting-diagnostics/核心排障/06-node-notready-diagnosis.md|Node NotReady 状态深度诊断]]
- [[domain-10-troubleshooting-diagnostics/核心排障/07-oom-memory-diagnosis.md|OOM 和内存问题诊断]]
- [[domain-10-troubleshooting-diagnostics/核心排障/08-pod-comprehensive-troubleshooting.md|Pod 全面故障排查]]
- [[domain-10-troubleshooting-diagnostics/资源排障/09-node-comprehensive-troubleshooting.md|Node 全面故障排查]]
- [[domain-10-troubleshooting-diagnostics/FTA故障树/list/apiserver-fta.md|API Server 异常故障树分析]]
- [[domain-10-troubleshooting-diagnostics/FTA故障树/list/backup-restore-fta.md|备份/恢复异常故障树分析]]
- [[domain-10-troubleshooting-diagnostics/FTA故障树/list/calico-fta.md|calico FTA 树：Calico CNI 故障诊断]]

## See Also

- [[domain-10-troubleshooting-diagnostics/资源排障/20-daemonset-troubleshooting.md|20-daemonset-troubleshooting]]
- [[domain-10-troubleshooting-diagnostics/资源排障/21-statefulset-troubleshooting.md|21-statefulset-troubleshooting]]
- [[domain-10-troubleshooting-diagnostics/资源排障/23-namespace-troubleshooting.md|23-namespace-troubleshooting]]
- [[domain-10-troubleshooting-diagnostics/资源排障/24-quota-limitrange-troubleshooting.md|24-quota-limitrange-troubleshooting]]


<!-- risk-assessed -->
