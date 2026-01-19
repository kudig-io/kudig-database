# 35 - 工作负载控制器详解 (Workload Controllers)

> **适用版本**: v1.25 - v1.32 | **最后更新**: 2026-01 | **参考**: [Kubernetes Workloads](https://kubernetes.io/docs/concepts/workloads/)

## 控制器核心特征矩阵 (Controller Matrix)

| 控制器 (Controller) | 核心用途 (Primary Use) | 标识 (Identity) | 扩缩容 (Scaling) | 更新策略 (Update) | 有序性 (Ordered) |
|-------------------|-------------------|---------------|----------------|-----------------|----------------|
| **Deployment** | 无状态微服务 | 随机 (Random) | ✅ HPA/VPA | RollingUpdate | ❌ |
| **StatefulSet** | 有状态数据库/中间件 | 固定 (Index) | ✅ HPA | RollingUpdate | ✅ |
| **DaemonSet** | 节点级插件/采集器 | 节点绑定 | ❌ 自动跟随 | RollingUpdate | ❌ |
| **Job** | 批处理/离线计算 | 随机 | ❌ 并发控制 | - | ❌ |
| **CronJob** | 定时任务/自动运维 | 随机 | ❌ 并发控制 | - | ❌ |

## 1. Deployment: 生产级最佳实践 (Production Patterns)

### 1.1 标准生产模板

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: web-api
  namespace: production
spec:
  replicas: 6
  revisionHistoryLimit: 10
  strategy:
    type: RollingUpdate
    rollingUpdate:
      maxSurge: 1          # 保证 100% 可用
      maxUnavailable: 0
  selector:
    matchLabels:
      app: web-api
  template:
    metadata:
      labels:
        app: web-api
    spec:
      topologySpreadConstraints:
      - maxSkew: 1
        topologyKey: topology.kubernetes.io/zone
        whenUnsatisfiable: ScheduleAnyway
        labelSelector:
          matchLabels:
            app: web-api
      containers:
      - name: web-api
        image: registry.example.com/web-api:1.0.0
        resources:
          requests:
            cpu: "500m"
            memory: "1Gi"
          limits:
            cpu: "1"
            memory: "2Gi"
        readinessProbe:
          httpGet:
            path: /ready
            port: 8080
          initialDelaySeconds: 5
          periodSeconds: 5
        livenessProbe:
          httpGet:
            path: /health
            port: 8080
          initialDelaySeconds: 30
          periodSeconds: 10
```

### 1.2 关键优化点

- **更新安全**:
  - 使用 `maxSurge: 1` 和 `maxUnavailable: 0` 实现 100% 可用性更新。
  - 配合 **PodDisruptionBudget (PDB)** 防止节点维护时副本数过低。
- **调度增强**:
  - 使用 `topologySpreadConstraints` 保证跨 AZ/节点均匀分布。
  - 使用 `podAntiAffinity` 避免同一 Deployment 的副本堆叠在少数节点。
- **回滚与灰度**:
  - `kubectl rollout pause/resume deployment/web-api` 控制灰度节奏。
  - `kubectl rollout undo deployment/web-api --to-revision=2` 快速回滚。

---

## 2. StatefulSet: 有状态运维 (Stateful Ops)

### 2.1 有状态集群模板

```yaml
apiVersion: apps/v1
kind: StatefulSet
metadata:
  name: mysql
  namespace: database
spec:
  serviceName: mysql-headless
  replicas: 3
  podManagementPolicy: Parallel  # 加快扩容速度
  updateStrategy:
    type: RollingUpdate
    rollingUpdate:
      partition: 1              # 仅更新序号 >=1 的 Pod, 实现金丝雀
  persistentVolumeClaimRetentionPolicy:
    whenDeleted: Retain        # 删除 StatefulSet 时保留 PVC
    whenScaled: Retain
  selector:
    matchLabels:
      app: mysql
  template:
    metadata:
      labels:
        app: mysql
    spec:
      containers:
      - name: mysql
        image: mysql:8.0
        ports:
        - containerPort: 3306
        volumeMounts:
        - name: data
          mountPath: /var/lib/mysql
  volumeClaimTemplates:
  - metadata:
      name: data
    spec:
      accessModes: ["ReadWriteOnce"]
      resources:
        requests:
          storage: 200Gi
      storageClassName: fast-ssd-pl2
```

### 2.2 设计要点

- **网络标识**: 必须配合 **Headless Service** 维持 DNS 持久化 (`pod-n.svc.ns.svc.cluster.local`)。
- **存储保留**: 使用 `persistentVolumeClaimRetentionPolicy` 控制删除/缩容时 PVC 是否保留。
- **有序性**: 默认按照序号一个一个滚动, 关键业务可以利用 `partition` 做分批金丝雀更新。

---

## 3. DaemonSet: 节点全覆盖 (Node-wide Coverage)

### 3.1 系统 DaemonSet 模板

```yaml
apiVersion: apps/v1
kind: DaemonSet
metadata:
  name: node-exporter
  namespace: monitoring
spec:
  selector:
    matchLabels:
      app: node-exporter
  updateStrategy:
    type: RollingUpdate
    rollingUpdate:
      maxUnavailable: 10%   # 控制同时升级的节点比例
  template:
    metadata:
      labels:
        app: node-exporter
    spec:
      tolerations:
      - operator: "Exists"  # 覆盖所有污点节点, 包含 master/control-plane
      containers:
      - name: node-exporter
        image: prom/node-exporter:v1.7.0
        resources:
          requests:
            cpu: "50m"
            memory: "64Mi"
          limits:
            cpu: "200m"
            memory: "256Mi"
```

### 3.2 调优要点

- `maxUnavailable` 与集群规模成比例, 推荐 5%~20%。
- 必须包含 Master/Control-Plane 容忍度, 确保监控/日志插件覆盖全集群。
- 对 IO/CPU 敏感业务, 应通过 `nodeSelector`/`affinity` 限制 DaemonSet 仅运行在指定节点池。

---

## 4. CronJob: 精准调度 (Precise Scheduling)

### 4.1 生产级 CronJob 模板

```yaml
apiVersion: batch/v1
kind: CronJob
metadata:
  name: db-backup
  namespace: ops
spec:
  schedule: "0 2 * * *"        # 每天凌晨 2 点
  timeZone: "Asia/Shanghai"    # v1.27+ GA
  concurrencyPolicy: Forbid      # 防止任务堆积
  successfulJobsHistoryLimit: 3
  failedJobsHistoryLimit: 5
  jobTemplate:
    spec:
      backoffLimit: 2
      activeDeadlineSeconds: 3600
      template:
        spec:
          restartPolicy: Never
          containers:
          - name: backup
            image: registry.example.com/backup:1.0.0
            env:
            - name: DB_HOST
              value: mysql-0.mysql.database.svc.cluster.local
```

### 4.2 常见坑位

- 未设置 `timeZone` 导致跨时区集群执行时间与预期不符。
- 未设置 `concurrencyPolicy: Forbid` 导致长时间任务堆积, 撑爆数据库连接池。
- 未配置 `activeDeadlineSeconds`, 导致 Job 永远不退出, 形成“僵尸任务”。

---

## 5. 故障排查路线 (Troubleshooting)

| 场景 | 典型症状 | 排查命令 | 处理建议 |
|------|----------|----------|----------|
| **Deployment 不滚动** | `kubectl rollout status` 卡住 | `kubectl describe deploy` / `kubectl get events` | 检查探针/资源是否导致新 Pod 不 Ready |
| **StatefulSet 卡在某个序号** | 某 Pod 一直 Pending/CrashLoop | `kubectl describe pod` | 多为存储/PV 绑定问题, 结合 PV/PVC 表排查 |
| **DaemonSet 部分节点无 Pod** | 某些节点未运行 Daemon | `kubectl describe daemonset` / `kubectl describe node` | 检查污点/选择器是否排除了这些节点 |
| **CronJob 不触发** | 预期时间没生成 Job | `kubectl get cronjob -A` / `kubectl describe cronjob` | 检查 `schedule/timeZone` 与控制器日志 |

---
---

**表格底部标记**: Kusheet Project, 作者 Allen Galler (allengaller@gmail.com)