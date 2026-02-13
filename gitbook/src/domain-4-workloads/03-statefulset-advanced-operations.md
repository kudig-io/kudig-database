# 03 - StatefulSet 高级运维指南 (StatefulSet Advanced Operations)

> **适用版本**: v1.25 - v1.32 | **最后更新**: 2026-02 | **参考**: [Kubernetes StatefulSets](https://kubernetes.io/docs/concepts/workloads/controllers/statefulset/)

## StatefulSet 核心特性解析

### 1. StatefulSet 与 Deployment 关键差异

| 特性 | Deployment | StatefulSet |
|------|------------|-------------|
| **身份标识** | 随机Pod名称 | 固定有序名称 (app-0, app-1...) |
| **网络标识** | 临时IP | 稳定DNS记录 |
| **存储** | 临时存储 | 持久化存储绑定 |
| **启动顺序** | 并行启动 | 有序启动 (0→1→2...) |
| **更新策略** | 并行更新 | 有序更新 |
| **适用场景** | 无状态应用 | 有状态应用 |

### 2. 生产级 StatefulSet 模板

#### 2.1 分布式数据库部署

```yaml
apiVersion: apps/v1
kind: StatefulSet
metadata:
  name: postgres-cluster
  namespace: database
  labels:
    app: postgres
    cluster: postgres-cluster
spec:
  serviceName: postgres-headless  # 必须的 Headless Service
  replicas: 3
  podManagementPolicy: Parallel   # 并行创建提升速度
  revisionHistoryLimit: 10
  
  # 更新策略
  updateStrategy:
    type: RollingUpdate
    rollingUpdate:
      partition: 0    # 0表示全部更新，>0实现金丝雀
  
  # 持久卷保留策略
  persistentVolumeClaimRetentionPolicy:
    whenDeleted: Retain    # 删除时保留数据
    whenScaled: Retain     # 缩容时保留数据
  
  selector:
    matchLabels:
      app: postgres
      cluster: postgres-cluster
  
  template:
    metadata:
      labels:
        app: postgres
        cluster: postgres-cluster
        version: v14.6
      annotations:
        prometheus.io/scrape: "true"
        prometheus.io/port: "9187"
    
    spec:
      # 服务账户
      serviceAccountName: postgres-sa
      
      # 节点选择和容忍
      nodeSelector:
        node-role: database
      tolerations:
      - key: dedicated
        operator: Equal
        value: database
        effect: NoSchedule
      
      # 反亲和性确保分散部署
      affinity:
        podAntiAffinity:
          requiredDuringSchedulingIgnoredDuringExecution:
          - labelSelector:
              matchLabels:
                app: postgres
            topologyKey: kubernetes.io/hostname
          preferredDuringSchedulingIgnoredDuringExecution:
          - weight: 100
            podAffinityTerm:
              labelSelector:
                matchLabels:
                  app: postgres
              topologyKey: topology.kubernetes.io/zone
      
      # 初始化容器
      initContainers:
      - name: init-permissions
        image: busybox:1.35
        command: ['sh', '-c', 'chown -R 70:70 /var/lib/postgresql/data']
        volumeMounts:
        - name: postgres-storage
          mountPath: /var/lib/postgresql/data
        securityContext:
          runAsUser: 0  # root 权限修改属主
      
      # 主容器配置
      containers:
      - name: postgres
        image: postgres:14.6-alpine
        ports:
        - containerPort: 5432
          name: postgres
        env:
        - name: POSTGRES_DB
          value: "myapp"
        - name: POSTGRES_USER
          valueFrom:
            secretKeyRef:
              name: postgres-secret
              key: username
        - name: POSTGRES_PASSWORD
          valueFrom:
            secretKeyRef:
              name: postgres-secret
              key: password
        - name: PGDATA
          value: "/var/lib/postgresql/data/pgdata"
        - name: POSTGRES_INITDB_ARGS
          value: "--auth-host=scram-sha-256"
        
        # 资源配置
        resources:
          requests:
            cpu: "1"
            memory: "2Gi"
            ephemeral-storage: "10Gi"
          limits:
            cpu: "4"
            memory: "8Gi"
            ephemeral-storage: "20Gi"
        
        # 健康检查
        livenessProbe:
          exec:
            command: ["pg_isready", "-U", "postgres"]
          initialDelaySeconds: 60
          periodSeconds: 10
          timeoutSeconds: 5
          failureThreshold: 3
        
        readinessProbe:
          exec:
            command: ["pg_isready", "-U", "postgres"]
          initialDelaySeconds: 10
          periodSeconds: 5
          timeoutSeconds: 3
          failureThreshold: 3
        
        # 安全上下文
        securityContext:
          runAsUser: 70
          runAsGroup: 70
          fsGroup: 70
          allowPrivilegeEscalation: false
          readOnlyRootFilesystem: false  # 数据库存储需要写权限
        
        # 存储挂载
        volumeMounts:
        - name: postgres-storage
          mountPath: /var/lib/postgresql/data
        - name: postgres-config
          mountPath: /etc/postgresql/postgresql.conf
          subPath: postgresql.conf
        - name: postgres-config
          mountPath: /etc/postgresql/pg_hba.conf
          subPath: pg_hba.conf
  
  # 持久卷声明模板
  volumeClaimTemplates:
  - metadata:
      name: postgres-storage
      labels:
        app: postgres
        type: data
    spec:
      accessModes: ["ReadWriteOnce"]
      storageClassName: fast-ssd
      resources:
        requests:
          storage: 500Gi
      # 支持卷扩展 (v1.11+)
      volumeMode: Filesystem
  
  # 配置卷
  - name: postgres-config
    configMap:
      name: postgres-config
```

#### 2.2 Redis 集群部署

```yaml
apiVersion: apps/v1
kind: StatefulSet
metadata:
  name: redis-cluster
  namespace: cache
spec:
  serviceName: redis-headless
  replicas: 6
  podManagementPolicy: OrderedReady  # 严格顺序
  
  updateStrategy:
    type: RollingUpdate
    rollingUpdate:
      partition: 0
  
  selector:
    matchLabels:
      app: redis-cluster
  
  template:
    metadata:
      labels:
        app: redis-cluster
        role: master  # 或 replica，通过 initContainer 设置
    
    spec:
      # Redis 集群初始化
      initContainers:
      - name: redis-cluster-init
        image: redis:7.0-alpine
        command:
        - sh
        - -c
        - |
          # 获取 Pod 序号
          INDEX=$(echo $HOSTNAME | cut -d'-' -f2)
          
          # 生成集群配置
          if [ $INDEX -lt 3 ]; then
            echo "master" > /tmp/role
          else
            echo "replica" > /tmp/role
          fi
          
          # 生成节点配置
          echo "port 6379
          cluster-enabled yes
          cluster-config-file nodes.conf
          cluster-node-timeout 5000
          appendonly yes" > /tmp/redis.conf
        volumeMounts:
        - name: redis-config
          mountPath: /tmp
        
        # 主容器
      containers:
      - name: redis
        image: redis:7.0-alpine
        command:
        - redis-server
        - /etc/redis/redis.conf
        ports:
        - containerPort: 6379
          name: redis
        - containerPort: 16379
          name: cluster
        
        resources:
          requests:
            cpu: "200m"
            memory: "512Mi"
          limits:
            cpu: "1"
            memory: "2Gi"
        
        # Redis 集群健康检查
        livenessProbe:
          tcpSocket:
            port: 6379
          initialDelaySeconds: 30
          periodSeconds: 10
        
        readinessProbe:
          exec:
            command: ["redis-cli", "ping"]
          initialDelaySeconds: 5
          periodSeconds: 5
        
        volumeMounts:
        - name: redis-storage
          mountPath: /data
        - name: redis-config
          mountPath: /etc/redis/redis.conf
          subPath: redis.conf
  
  # 存储卷
  volumeClaimTemplates:
  - metadata:
      name: redis-storage
    spec:
      accessModes: ["ReadWriteOnce"]
      storageClassName: fast-ssd
      resources:
        requests:
          storage: 100Gi
```

### 3. 高级运维操作

#### 3.1 有序滚动更新控制

```yaml
# 金丝雀更新策略
apiVersion: apps/v1
kind: StatefulSet
metadata:
  name: mysql-cluster
spec:
  updateStrategy:
    type: RollingUpdate
    rollingUpdate:
      partition: 2  # 只更新序号 >= 2 的 Pod
  
  # 更新流程：
  # 1. 设置 partition: 2 (只更新 mysql-2, mysql-3, mysql-4...)
  # 2. 验证 mysql-2 运行正常
  # 3. 设置 partition: 1 (更新 mysql-1, mysql-2...)
  # 4. 逐步减少 partition 值直到 0

---
# 暂停更新
apiVersion: apps/v1
kind: StatefulSet
metadata:
  name: mysql-cluster
spec:
  updateStrategy:
    type: OnDelete  # 手动删除 Pod 触发更新
```

#### 3.2 数据备份与恢复

```yaml
# 备份 Job 模板
apiVersion: batch/v1
kind: Job
metadata:
  name: postgres-backup
  namespace: database
spec:
  template:
    spec:
      containers:
      - name: backup
        image: postgres:14.6-alpine
        command:
        - sh
        - -c
        - |
          # 创建备份
          pg_dump -h postgres-0.postgres-headless -U postgres myapp \
          > /backup/backup-$(date +%Y%m%d-%H%M%S).sql
          
          # 清理旧备份 (保留7天)
          find /backup -name "*.sql" -mtime +7 -delete
        env:
        - name: PGPASSWORD
          valueFrom:
            secretKeyRef:
              name: postgres-secret
              key: password
        volumeMounts:
        - name: backup-storage
          mountPath: /backup
      
      volumes:
      - name: backup-storage
        persistentVolumeClaim:
          claimName: backup-pvc
      
      restartPolicy: OnFailure

---
# 定时备份 CronJob
apiVersion: batch/v1
kind: CronJob
metadata:
  name: postgres-backup-cron
  namespace: database
spec:
  schedule: "0 2 * * *"  # 每天凌晨2点
  jobTemplate:
    spec:
      template:
        spec:
          containers:
          - name: backup
            # ... 同上备份配置
```

#### 3.3 跨区域部署配置

```yaml
apiVersion: apps/v1
kind: StatefulSet
metadata:
  name: distributed-db
  namespace: production
spec:
  replicas: 9  # 3个区域 × 3个副本
  
  template:
    spec:
      # 区域感知调度
      affinity:
        nodeAffinity:
          requiredDuringSchedulingIgnoredDuringExecution:
            nodeSelectorTerms:
            - matchExpressions:
              - key: topology.kubernetes.io/region
                operator: In
                values: ["us-east-1", "us-west-1", "eu-central-1"]
        
        # 区域内反亲和性
        podAntiAffinity:
          requiredDuringSchedulingIgnoredDuringExecution:
          - labelSelector:
              matchLabels:
                app: distributed-db
            topologyKey: topology.kubernetes.io/zone
          
          # 跨区域容忍
          preferredDuringSchedulingIgnoredDuringExecution:
          - weight: 100
            podAffinityTerm:
              labelSelector:
                matchLabels:
                  app: distributed-db
              topologyKey: topology.kubernetes.io/region
      
      # 区域特定配置
      initContainers:
      - name: region-config
        image: busybox:1.35
        command:
        - sh
        - -c
        - |
          REGION=$(curl -s http://169.254.169.254/latest/meta-data/placement/region)
          echo "region=$REGION" > /shared/config.env
        volumeMounts:
        - name: shared-config
          mountPath: /shared
      
      containers:
      - name: app
        # ... 应用配置
        env:
        - name: REGION
          valueFrom:
            configMapKeyRef:
              name: regional-config
              key: region
        
        volumeMounts:
        - name: shared-config
          mountPath: /etc/regional
```

### 4. 监控与告警配置

#### 4.1 StatefulSet 特定监控

```yaml
# Prometheus 监控规则
groups:
- name: statefulset_monitoring
  rules:
  # Pod 序号连续性检查
  - alert: StatefulSetPodOrdinalGap
    expr: |
      count(count by (ordinal) (
        kube_statefulset_status_replicas{job="kube-state-metrics"}
      )) != 
      max(kube_statefulset_status_replicas{job="kube-state-metrics"})
    for: 10m
    labels:
      severity: critical
    annotations:
      summary: "StatefulSet {{ $labels.statefulset }} 存在 Pod 序号间隙"
  
  # 存储使用率告警
  - alert: StatefulSetStorageHigh
    expr: |
      kubelet_volume_stats_used_bytes / 
      kubelet_volume_stats_capacity_bytes * 100 > 85
    for: 15m
    labels:
      severity: warning
    annotations:
      summary: "StatefulSet Pod {{ $labels.pod }} 存储使用率超过 85%"
  
  # 更新卡住检测
  - alert: StatefulSetUpdateStuck
    expr: |
      kube_statefulset_status_replicas_updated != 
      kube_statefulset_replicas
    for: 20m
    labels:
      severity: critical
    annotations:
      summary: "StatefulSet {{ $labels.statefulset }} 更新卡住"
```

#### 4.2 数据库特定监控

```yaml
# PostgreSQL 监控配置
apiVersion: v1
kind: ConfigMap
metadata:
  name: postgres-exporter-config
  namespace: database
data:
  queries.yaml: |
    pg_replication_lag:
      query: "SELECT EXTRACT(EPOCH FROM (now() - pg_last_xact_replay_timestamp())) AS lag"
      metrics:
        - lag:
            usage: "GAUGE"
            description: "Replication lag in seconds"
    
    pg_connections:
      query: "SELECT count(*) as connections FROM pg_stat_activity"
      metrics:
        - connections:
            usage: "GAUGE"
            description: "Active connections"
```

### 5. 故障排查与恢复

#### 5.1 常见问题诊断命令

```bash
# 1. 查看 StatefulSet 状态
kubectl describe statefulset <statefulset-name> -n <namespace>

# 2. 检查 Pod 序号连续性
kubectl get pods -l app=<app-name> -n <namespace> -o jsonpath='{.items[*].metadata.name}' | tr ' ' '\n' | sort

# 3. 检查 PVC 状态
kubectl get pvc -l app=<app-name> -n <namespace>
kubectl describe pvc <pvc-name> -n <namespace>

# 4. 检查存储卷挂载
kubectl exec -it <pod-name> -n <namespace> -- df -h /var/lib/data

# 5. 强制删除卡住的 Pod
kubectl delete pod <pod-name> -n <namespace> --force --grace-period=0

# 6. 手动触发更新 (OnDelete 策略)
kubectl delete pod <statefulset-name>-<ordinal> -n <namespace>

# 7. 扩容/缩容
kubectl scale statefulset <statefulset-name> -n <namespace> --replicas=<number>
```

#### 5.2 数据恢复流程

```bash
#!/bin/bash
# StatefulSet 数据恢复脚本

STATEFULSET_NAME=$1
NAMESPACE=${2:-default}
BACKUP_FILE=$3

echo "开始恢复 StatefulSet $STATEFULSET_NAME 数据..."

# 1. 停止应用写入
kubectl scale statefulset $STATEFULSET_NAME -n $NAMESPACE --replicas=0

# 2. 等待所有 Pod 停止
while [[ $(kubectl get pods -l app=$STATEFULSET_NAME -n $NAMESPACE --no-headers | wc -l) -gt 0 ]]; do
    echo "等待 Pod 停止..."
    sleep 10
done

# 3. 挂载恢复 Job
cat <<EOF | kubectl apply -f -
apiVersion: batch/v1
kind: Job
metadata:
  name: ${STATEFULSET_NAME}-restore
  namespace: $NAMESPACE
spec:
  template:
    spec:
      containers:
      - name: restore
        image: postgres:14.6-alpine
        command:
        - sh
        - -c
        - |
          # 解压备份文件
          gunzip -c /backup/$BACKUP_FILE > /tmp/backup.sql
          
          # 恢复到每个 Pod
          for i in \$(seq 0 \$((${STATEFULSET_NAME}-1))); do
            echo "恢复到 ${STATEFULSET_NAME}-\$i..."
            psql -h ${STATEFULSET_NAME}-\$i.${STATEFULSET_NAME}-headless -U postgres -f /tmp/backup.sql
          done
        env:
        - name: PGPASSWORD
          valueFrom:
            secretKeyRef:
              name: postgres-secret
              key: password
        volumeMounts:
        - name: backup-storage
          mountPath: /backup
      volumes:
      - name: backup-storage
        persistentVolumeClaim:
          claimName: backup-pvc
      restartPolicy: Never
EOF

# 4. 等待恢复完成
kubectl wait --for=condition=complete job/${STATEFULSET_NAME}-restore -n $NAMESPACE --timeout=300s

# 5. 重启应用
kubectl scale statefulset $STATEFULSET_NAME -n $NAMESPACE --replicas=3

echo "数据恢复完成！"
```

### 6. 性能优化配置

#### 6.1 存储优化

```yaml
apiVersion: apps/v1
kind: StatefulSet
metadata:
  name: optimized-storage-app
spec:
  template:
    spec:
      containers:
      - name: app
        # SSD 优化挂载选项
        volumeMounts:
        - name: data-storage
          mountPath: /data
          mountPropagation: None
          subPath: data
        
        # I/O 优化环境变量
        env:
        - name: DISK_IO_SCHEDULER
          value: "deadline"  # 或 "noop" for SSD
        - name: FS_MOUNT_OPTIONS
          value: "noatime,nodiratime"

  volumeClaimTemplates:
  - metadata:
      name: data-storage
    spec:
      accessModes: ["ReadWriteOnce"]
      storageClassName: ssd-fast  # 使用高性能 StorageClass
      resources:
        requests:
          storage: 1Ti
      
      # 卷扩展支持
      volumeMode: Filesystem
```

#### 6.2 网络优化

```yaml
apiVersion: apps/v1
kind: StatefulSet
metadata:
  name: network-optimized-app
spec:
  template:
    metadata:
      annotations:
        # 启用 DPDK 或其他网络优化
        kubernetes.io/network-bandwidth: "10G"
    
    spec:
      # 主机网络模式 (谨慎使用)
      hostNetwork: true
      dnsPolicy: ClusterFirstWithHostNet
      
      containers:
      - name: app
        ports:
        - containerPort: 8080
          hostPort: 8080  # 直接映射到主机端口
```

---

**运维原则**: 数据安全第一，有序操作为本，监控告警到位，备份恢复可靠。

---

**文档维护**: Kusheet Project | **作者**: Allen Galler (allengaller@gmail.com)
