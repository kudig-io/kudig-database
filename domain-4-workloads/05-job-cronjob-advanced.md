# 05 - Job 与 CronJob 高级用法 (Job & CronJob Advanced Usage)

> **适用版本**: v1.25 - v1.32 | **最后更新**: 2026-02 | **参考**: [Kubernetes Jobs](https://kubernetes.io/docs/concepts/workloads/controllers/job/)

## Job 类型详解与使用场景

### 1. Job 类型对比矩阵

| Job 类型 | 并发策略 | 完成条件 | 适用场景 | 生产建议 |
|----------|----------|----------|----------|----------|
| **普通 Job** | Allow/Forbid/Replace | completions 次成功 | 单次批处理 | 设置 activeDeadlineSeconds |
| **并行 Job** | Allow | completions 次成功 | 分布式计算 | 合理设置 parallelism |
| **工作队列 Job** | Forbid | 任意 Pod 成功 | 消费队列任务 | 不设置 completions |
| **Indexed Job** | Forbid | 所有索引完成 | 需要序号的任务 | v1.21+ 推荐 |

### 2. 生产级 Job 模板库

#### 2.1 数据处理批处理作业

```yaml
apiVersion: batch/v1
kind: Job
metadata:
  name: data-processing-job
  namespace: analytics
  labels:
    app: data-processor
    job-type: batch-processing
  annotations:
    job-description: "每日数据ETL处理作业"
spec:
  # 并发控制
  parallelism: 5           # 同时运行5个Pod
  completions: 20          # 总共需要完成20次
  
  # 失败重试策略
  backoffLimit: 3          # 最多重试3次
  ttlSecondsAfterFinished: 86400  # 完成后保留24小时
  
  # 超时控制
  activeDeadlineSeconds: 7200  # 2小时超时
  
  template:
    metadata:
      labels:
        app: data-processor
        job-name: data-processing-job
    
    spec:
      # 重启策略
      restartPolicy: OnFailure  # 失败时重启容器，不重新创建Pod
      
      # 节点选择
      nodeSelector:
        node-type: compute-optimized
      
      # 容器配置
      containers:
      - name: processor
        image: data-processor:latest
        imagePullPolicy: Always
        
        # 环境变量传递任务参数
        env:
        - name: BATCH_SIZE
          value: "1000"
        - name: PROCESSOR_ID
          valueFrom:
            fieldRef:
              fieldPath: metadata.name  # Pod名称作为处理器ID
        - name: JOB_COMPLETION_INDEX  # Indexed Job 索引
          valueFrom:
            fieldRef:
              fieldPath: metadata.annotations['batch.kubernetes.io/job-completion-index']
        
        # 资源配置
        resources:
          requests:
            cpu: "1"
            memory: "2Gi"
          limits:
            cpu: "2"
            memory: "4Gi"
        
        # 健康检查
        livenessProbe:
          exec:
            command: ["sh", "-c", "ps aux | grep processor"]
          initialDelaySeconds: 30
          periodSeconds: 60
        
        # 存储挂载
        volumeMounts:
        - name: data-input
          mountPath: /input
        - name: data-output
          mountPath: /output
        - name: processing-logs
          mountPath: /var/log/processing
        
        # 安全上下文
        securityContext:
          runAsNonRoot: true
          runAsUser: 1000
          allowPrivilegeEscalation: false
          readOnlyRootFilesystem: false  # 需要写入输出文件
      
      # 存储卷
      volumes:
      - name: data-input
        persistentVolumeClaim:
          claimName: data-input-pvc
      - name: data-output
        persistentVolumeClaim:
          claimName: data-output-pvc
      - name: processing-logs
        emptyDir: {}
```

#### 2.2 工作队列消费者作业

```yaml
apiVersion: batch/v1
kind: Job
metadata:
  name: queue-worker
  namespace: backend
spec:
  parallelism: 10          # 启动10个工作进程
  # 注意：不设置 completions，让工作队列决定何时完成
  
  backoffLimit: 5
  ttlSecondsAfterFinished: 3600
  
  template:
    spec:
      restartPolicy: Never   # 失败时不重启，创建新Pod
      
      containers:
      - name: worker
        image: queue-worker:latest
        env:
        - name: RABBITMQ_URL
          valueFrom:
            secretKeyRef:
              name: rabbitmq-secret
              key: url
        - name: WORKER_CONCURRENCY
          value: "5"  # 每个Pod并发处理5个任务
        
        resources:
          requests:
            cpu: "200m"
            memory: "256Mi"
          limits:
            cpu: "1"
            memory: "1Gi"
        
        # 启动探针确保连接建立
        startupProbe:
          exec:
            command: ["python", "health_check.py"]
          failureThreshold: 30
          periodSeconds: 10
        
        # 预停止钩子实现优雅关闭
        lifecycle:
          preStop:
            exec:
              command: ["/bin/sh", "-c", "touch /tmp/shutdown && sleep 30"]
```

### 3. CronJob 高级配置模式

#### 3.1 企业级定时任务模板

```yaml
apiVersion: batch/v1
kind: CronJob
metadata:
  name: enterprise-daily-report
  namespace: reporting
  labels:
    app: report-generator
    schedule: daily
spec:
  # 调度配置
  schedule: "0 1 * * *"     # 每天凌晨1点执行
  timeZone: "Asia/Shanghai" # v1.27+ 支持时区
  
  # 并发控制
  concurrencyPolicy: Forbid   # 禁止并发执行
  startingDeadlineSeconds: 300 # 错过调度时间300秒内仍执行
  
  # 历史记录管理
  successfulJobsHistoryLimit: 3   # 保留3个成功记录
  failedJobsHistoryLimit: 5       # 保留5个失败记录
  
  jobTemplate:
    spec:
      # Job 配置继承
      parallelism: 1
      backoffLimit: 2
      activeDeadlineSeconds: 10800  # 3小时超时
      
      template:
        metadata:
          labels:
            app: report-generator
            cronjob: enterprise-daily-report
        
        spec:
          # 服务账户和安全
          serviceAccountName: report-sa
          automountServiceAccountToken: false
          
          restartPolicy: OnFailure
          
          containers:
          - name: reporter
            image: report-generator:latest
            imagePullPolicy: Always
            
            # 日期参数传递
            env:
            - name: REPORT_DATE
              valueFrom:
                fieldRef:
                  fieldPath: metadata.creationTimestamp
            - name: ENVIRONMENT
              value: "production"
            
            # 资源配置
            resources:
              requests:
                cpu: "500m"
                memory: "1Gi"
              limits:
                cpu: "2"
                memory: "4Gi"
            
            # 健康检查
            livenessProbe:
              exec:
                command: ["python", "check_status.py"]
              initialDelaySeconds: 60
              periodSeconds: 300  # 每5分钟检查一次
            
            # 输出挂载
            volumeMounts:
            - name: reports-output
              mountPath: /reports
            - name: temp-storage
              mountPath: /tmp
            
            # 安全配置
            securityContext:
              runAsNonRoot: true
              runAsUser: 1000
              allowPrivilegeEscalation: false
          
          volumes:
          - name: reports-output
            persistentVolumeClaim:
              claimName: reports-pvc
          - name: temp-storage
            emptyDir: {}
```

#### 3.2 多时区分布式任务

```yaml
apiVersion: batch/v1
kind: CronJob
metadata:
  name: global-user-sync
  namespace: sync
spec:
  # UTC 时间调度
  schedule: "0 2 * * *"  # UTC 凌晨2点
  timeZone: "UTC"
  
  # 允许一定并发
  concurrencyPolicy: Allow
  successfulJobsHistoryLimit: 10
  
  jobTemplate:
    spec:
      parallelism: 3  # 同时处理3个区域
      
      template:
        spec:
          # 区域选择器
          nodeSelector:
            region: us-east-1  # 或动态选择
          
          initContainers:
          - name: region-detector
            image: busybox:1.35
            command:
            - sh
            - -c
            - |
              # 根据节点标签确定处理区域
              REGION=$(curl -s http://169.254.169.254/latest/meta-data/placement/region)
              echo "Processing region: $REGION"
              echo "region=$REGION" > /shared/region.env
            volumeMounts:
            - name: shared-data
              mountPath: /shared
          
          containers:
          - name: sync-worker
            image: user-sync-worker:latest
            env:
            - name: TARGET_REGION
              valueFrom:
                configMapKeyRef:
                  name: regional-config
                  key: region
            
            # 区域特定配置
            envFrom:
            - configMapRef:
                name: regional-endpoints
            
            resources:
              requests:
                cpu: "1"
                memory: "2Gi"
              limits:
                cpu: "2"
                memory: "4Gi"
            
            volumeMounts:
            - name: shared-data
              mountPath: /config
            
          volumes:
          - name: shared-data
            emptyDir: {}
```

### 4. 高级调度策略

#### 4.1 条件触发任务

```yaml
# 使用 Kueue 进行队列调度
apiVersion: batch/v1
kind: Job
metadata:
  name: resource-intensive-job
  namespace: compute
  labels:
    kueue.x-k8s.io/queue-name: compute-queue
spec:
  parallelism: 20
  completions: 100
  
  template:
    spec:
      # 资源配额请求
      containers:
      - name: compute-worker
        image: ml-training:latest
        resources:
          requests:
            cpu: "8"
            memory: "32Gi"
            nvidia.com/gpu: "1"
          limits:
            cpu: "8"
            memory: "32Gi"
            nvidia.com/gpu: "1"
```

#### 4.2 依赖链任务

```yaml
# 父任务
apiVersion: batch/v1
kind: Job
metadata:
  name: data-extraction
  namespace: etl
spec:
  template:
    spec:
      containers:
      - name: extractor
        image: data-extractor:latest
        env:
        - name: OUTPUT_PATH
          value: "/shared/data.json"
        volumeMounts:
        - name: shared-volume
          mountPath: /shared
  
  # 完成后触发子任务
  completionMode: NonIndexed
  
---
# 子任务 (通过外部控制器触发)
apiVersion: batch/v1
kind: Job
metadata:
  name: data-transformation
  namespace: etl
  annotations:
    depends-on: "data-extraction"
spec:
  template:
    spec:
      containers:
      - name: transformer
        image: data-transformer:latest
        env:
        - name: INPUT_PATH
          value: "/shared/data.json"
        volumeMounts:
        - name: shared-volume
          mountPath: /shared
  
  # 使用 initContainer 等待父任务完成
  template:
    spec:
      initContainers:
      - name: wait-for-parent
        image: busybox:1.35
        command:
        - sh
        - -c
        - |
          until kubectl get job data-extraction -o jsonpath='{.status.succeeded}' | grep -q 1; do
            echo "Waiting for parent job to complete..."
            sleep 30
          done
```

### 5. 监控与告警配置

#### 5.1 Job 特定监控指标

```yaml
# PrometheusRule 配置
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: job-monitoring
  namespace: monitoring
spec:
  groups:
  - name: job.rules
    rules:
    # Job 执行时间过长
    - alert: JobDurationTooLong
      expr: |
        time() - kube_job_status_start_time > 7200
      for: 30m
      labels:
        severity: warning
      annotations:
        summary: "Job {{ $labels.job }} 执行时间超过2小时"
    
    # Job 失败率过高
    - alert: JobFailureRateHigh
      expr: |
        rate(kube_job_status_failed[1h]) > 0.1
      for: 15m
      labels:
        severity: critical
      annotations:
        summary: "Job {{ $labels.job }} 失败率超过10%"
    
    # CronJob 错过调度
    - alert: CronJobMissedSchedules
      expr: |
        kube_cronjob_status_last_schedule_time - 
        kube_cronjob_spec_suspend == 0
      for: 10m
      labels:
        severity: warning
      annotations:
        summary: "CronJob {{ $labels.cronjob }} 错过调度时间"
```

#### 5.2 自定义指标导出

```yaml
# Job 指标导出器
apiVersion: v1
kind: ConfigMap
metadata:
  name: job-metrics-exporter
  namespace: monitoring
data:
  exporter.sh: |
    #!/bin/bash
    # Job 自定义指标导出脚本
    
    JOB_NAME=${JOB_NAME:-$(hostname)}
    START_TIME=$(date +%s)
    
    # 模拟业务处理
    process_data() {
        echo "Processing data for job: $JOB_NAME"
        sleep $((RANDOM % 300))  # 随机处理时间
        
        # 导出自定义指标
        echo "# HELP job_processing_duration_seconds Job processing duration"
        echo "# TYPE job_processing_duration_seconds gauge"
        echo "job_processing_duration_seconds{job=\"$JOB_NAME\"} $(($(date +%s) - START_TIME))"
        
        echo "# HELP job_processed_records_total Records processed"
        echo "# TYPE job_processed_records_total counter"
        echo "job_processed_records_total{job=\"$JOB_NAME\"} $((RANDOM % 10000))"
    }
    
    process_data
```

### 6. 故障排查与恢复

#### 6.1 常用诊断命令

```bash
# 1. 查看 Job 状态详情
kubectl describe job <job-name> -n <namespace>

# 2. 查看 Job 相关的所有 Pod
kubectl get pods --selector=job-name=<job-name> -n <namespace>

# 3. 查看失败 Pod 的日志
kubectl logs -l job-name=<job-name> --field-selector=status.phase=Failed -n <namespace>

# 4. 手动重新运行失败的 Job
kubectl delete job <job-name> -n <namespace>
kubectl create -f <job-definition.yaml>

# 5. 查看 CronJob 调度历史
kubectl get cronjob <cronjob-name> -n <namespace> -o yaml

# 6. 手动触发 CronJob
kubectl create job --from=cronjob/<cronjob-name> manual-<job-name> -n <namespace>

# 7. 暂停 CronJob 调度
kubectl patch cronjob <cronjob-name> -n <namespace> -p '{"spec":{"suspend":true}}'

# 8. 恢复 CronJob 调度
kubectl patch cronjob <cronjob-name> -n <namespace> -p '{"spec":{"suspend":false}}'
```

#### 6.2 自动化运维脚本

```bash
#!/bin/bash
# Job 健康检查和自动恢复脚本

JOB_NAME=$1
NAMESPACE=${2:-default}
MAX_RETRIES=${3:-3}

echo "检查 Job $JOB_NAME 状态..."

# 获取 Job 状态
ACTIVE=$(kubectl get job $JOB_NAME -n $NAMESPACE -o jsonpath='{.status.active}')
SUCCEEDED=$(kubectl get job $JOB_NAME -n $NAMESPACE -o jsonpath='{.status.succeeded}')
FAILED=$(kubectl get job $JOB_NAME -n $NAMESPACE -o jsonpath='{.status.failed}')

echo "活跃: $ACTIVE, 成功: $SUCCEEDED, 失败: $FAILED"

# 检查是否失败
if [ "$FAILED" -gt 0 ]; then
    echo "检测到失败的 Job，准备恢复..."
    
    # 获取失败原因
    kubectl describe job $JOB_NAME -n $NAMESPACE | grep -A 10 "Events:"
    
    # 检查重试次数
    BACKOFF_LIMIT=$(kubectl get job $JOB_NAME -n $NAMESPACE -o jsonpath='{.spec.backoffLimit}')
    
    if [ "$FAILED" -le "$BACKOFF_LIMIT" ]; then
        echo "仍在重试范围内，等待自动恢复..."
        exit 0
    else
        echo "超过重试限制，执行手动恢复..."
        
        # 删除失败的 Job
        kubectl delete job $JOB_NAME -n $NAMESPACE
        
        # 重新创建 Job (假设 YAML 文件存在)
        if [ -f "${JOB_NAME}.yaml" ]; then
            kubectl create -f ${JOB_NAME}.yaml -n $NAMESPACE
            echo "Job 已重新创建"
        else
            echo "警告: 找不到 Job 定义文件 ${JOB_NAME}.yaml"
        fi
    fi
fi

# 检查执行时间
START_TIME=$(kubectl get job $JOB_NAME -n $NAMESPACE -o jsonpath='{.status.startTime}')
if [ ! -z "$START_TIME" ]; then
    START_SECONDS=$(date -d "$START_TIME" +%s)
    CURRENT_SECONDS=$(date +%s)
    DURATION=$((CURRENT_SECONDS - START_SECONDS))
    
    if [ $DURATION -gt 7200 ]; then  # 超过2小时
        echo "警告: Job 运行时间过长 (${DURATION} 秒)"
        # 可以在这里发送告警或终止任务
    fi
fi

echo "Job 状态检查完成 ✓"
```

### 7. 性能优化实践

#### 7.1 并行处理优化

```yaml
apiVersion: batch/v1
kind: Job
metadata:
  name: parallel-processor
  namespace: compute
spec:
  # 动态并行度
  parallelism: 50
  completions: 1000
  
  # 精细化控制
  completionMode: Indexed  # v1.21+ 支持
  
  template:
    spec:
      # Pod 拓扑分布
      topologySpreadConstraints:
      - maxSkew: 2
        topologyKey: topology.kubernetes.io/zone
        whenUnsatisfiable: ScheduleAnyway
        labelSelector:
          matchLabels:
            app: parallel-processor
      
      containers:
      - name: worker
        image: parallel-worker:latest
        
        # 索引感知处理
        env:
        - name: JOB_COMPLETION_INDEX
          valueFrom:
            fieldRef:
              fieldPath: metadata.annotations['batch.kubernetes.io/job-completion-index']
        - name: TOTAL_COMPLETIONS
          value: "1000"
        
        # 资源优化
        resources:
          requests:
            cpu: "200m"
            memory: "512Mi"
          limits:
            cpu: "1"
            memory: "2Gi"
        
        # 快速启动配置
        startupProbe:
          exec:
            command: ["python", "warmup.py"]
          failureThreshold: 10
          periodSeconds: 5
```

---

**批处理原则**: 明确完成条件，控制并发数量，设置合理超时，完善监控告警，实现自动恢复。

---

**文档维护**: Kusheet Project | **作者**: Allen Galler (allengaller@gmail.com)