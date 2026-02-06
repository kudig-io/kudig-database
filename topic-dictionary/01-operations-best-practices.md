# 01 - Kubernetes 生产环境运维最佳实践字典

> **适用版本**: Kubernetes v1.25-v1.32 | **最后更新**: 2026-02 | **作者**: Allen Galler | **质量等级**: ⭐⭐⭐⭐⭐ 专家级

> **生产环境实战经验总结**: 基于万级节点集群运维经验，涵盖从故障处理到性能优化的全方位最佳实践

---

## 目录

- [1. 生产环境配置标准](#1-生产环境配置标准)
- [2. 高可用架构模式](#2-高可用架构模式)
- [3. 安全加固指南](#3-安全加固指南)
- [4. 监控告警最佳实践](#4-监控告警最佳实践)
- [5. 灾备恢复方案](#5-灾备恢复方案)
- [6. 自动化运维策略](#6-自动化运维策略)
- [7. 成本优化实践](#7-成本优化实践)
- [8. 多集群管理规范](#8-多集群管理规范)

---

## 1. 生产环境配置标准

### 1.1 集群配置基线

| 配置项 | 推荐值 | 说明 | 风险等级 |
|-------|--------|------|---------|
| **API Server并发限制** | `--max-requests-inflight=400` | 控制并发请求数量 | 中 |
| | `--max-mutating-requests-inflight=200` | 写操作并发限制 | 中 |
| **etcd存储配额** | `--quota-backend-bytes=8GB` | 存储空间限制 | 高 |
| **事件保留时间** | `--event-ttl=1h` | 减少etcd存储压力 | 低 |
| **节点最大Pod数** | `--max-pods=110` | 标准环境配置 | 中 |
| | `--max-pods=500` | AWS云环境配置 | 高 |
| **镜像垃圾回收** | `--image-gc-high-threshold=85` | 高水位触发GC | 中 |
| | `--image-gc-low-threshold=80` | 低水位停止GC | 中 |

### 1.2 资源配置标准模板

```yaml
# ========== 生产环境Deployment标准配置 ==========
apiVersion: apps/v1
kind: Deployment
metadata:
  name: production-app-standard
  namespace: production
  labels:
    app: production-app
    tier: backend
    version: v1.0
spec:
  replicas: 3
  strategy:
    type: RollingUpdate
    rollingUpdate:
      maxUnavailable: 1
      maxSurge: 1
  selector:
    matchLabels:
      app: production-app
  template:
    metadata:
      labels:
        app: production-app
        version: v1.0
      annotations:
        # 注入构建信息
        build.timestamp: "2026-02-05T10:30:00Z"
        build.commit: "a1b2c3d4"
    spec:
      # 优先级设置
      priorityClassName: high-priority
      
      # 节点选择策略
      affinity:
        podAntiAffinity:
          preferredDuringSchedulingIgnoredDuringExecution:
          - weight: 100
            podAffinityTerm:
              labelSelector:
                matchExpressions:
                - key: app
                  operator: In
                  values:
                  - production-app
              topologyKey: kubernetes.io/hostname
      
      # 容忍污点
      tolerations:
      - key: dedicated
        operator: Equal
        value: production
        effect: NoSchedule
        
      containers:
      - name: app
        image: registry.example.com/app:v1.0
        imagePullPolicy: Always
        
        # 核心资源配置
        resources:
          requests:
            cpu: "250m"
            memory: "512Mi"
          limits:
            cpu: "1000m"
            memory: "1Gi"
            
        # 健康检查配置
        livenessProbe:
          httpGet:
            path: /healthz
            port: 8080
          initialDelaySeconds: 60
          periodSeconds: 10
          timeoutSeconds: 5
          failureThreshold: 3
          
        readinessProbe:
          httpGet:
            path: /ready
            port: 8080
          initialDelaySeconds: 30
          periodSeconds: 5
          timeoutSeconds: 3
          failureThreshold: 3
          
        # 启动探针（K8s 1.18+）
        startupProbe:
          httpGet:
            path: /startup
            port: 8080
          initialDelaySeconds: 10
          periodSeconds: 5
          timeoutSeconds: 3
          failureThreshold: 30
          
        # 环境变量配置
        env:
        - name: LOG_LEVEL
          value: "INFO"
        - name: JAVA_OPTS
          value: "-Xmx768m -Xms512m -XX:+UseG1GC"
        - name: GOMEMLIMIT
          value: "800MiB"
          
        # 安全上下文
        securityContext:
          runAsNonRoot: true
          runAsUser: 1000
          readOnlyRootFilesystem: true
          allowPrivilegeEscalation: false
          
        # 挂载卷
        volumeMounts:
        - name: tmp-volume
          mountPath: /tmp
        - name: logs-volume
          mountPath: /var/log/app
          
      volumes:
      - name: tmp-volume
        emptyDir: {}
      - name: logs-volume
        persistentVolumeClaim:
          claimName: app-logs-pvc
```

### 1.3 网络策略标准

```yaml
# ========== 默认拒绝网络策略 ==========
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: default-deny-all
  namespace: production
spec:
  podSelector: {}
  policyTypes:
  - Ingress
  - Egress

---
# ========== 允许DNS查询策略 ==========
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: allow-dns-access
  namespace: production
spec:
  podSelector: {}
  policyTypes:
  - Egress
  egress:
  - to:
    - namespaceSelector:
        matchLabels:
          name: kube-system
    ports:
    - protocol: UDP
      port: 53
    - protocol: TCP
      port: 53

---
# ========== 应用间通信策略 ==========
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: app-communication-policy
  namespace: production
spec:
  podSelector:
    matchLabels:
      app: frontend
  policyTypes:
  - Ingress
  ingress:
  - from:
    - podSelector:
        matchLabels:
          app: backend
    ports:
    - protocol: TCP
      port: 8080
```

---

## 2. 高可用架构模式

### 2.1 控制平面高可用

```yaml
# ========== 生产环境控制平面配置 ==========
apiVersion: kubeadm.k8s.io/v1beta3
kind: ClusterConfiguration
metadata:
  name: production-cluster
networking:
  serviceSubnet: "10.96.0.0/12"
  podSubnet: "10.244.0.0/16"
  dnsDomain: "cluster.local"
etcd:
  local:
    extraArgs:
      listen-client-urls: "https://0.0.0.0:2379"
      advertise-client-urls: "https://ETCD_IP:2379"
      initial-cluster-token: "etcd-cluster-1"
      initial-cluster-state: "new"
      auto-compaction-mode: "periodic"
      auto-compaction-retention: "1"
    serverCertSANs:
    - "etcd01.example.com"
    - "etcd02.example.com"
    - "etcd03.example.com"
apiServer:
  certSANs:
  - "k8s-api.example.com"
  - "10.0.0.100"  # Load Balancer VIP
  extraArgs:
    authorization-mode: "Node,RBAC"
    enable-bootstrap-token-auth: "true"
    encryption-provider-config: "/etc/kubernetes/encryption-config.yaml"
controllerManager:
  extraArgs:
    cluster-signing-cert-file: "/etc/kubernetes/pki/ca.crt"
    cluster-signing-key-file: "/etc/kubernetes/pki/ca.key"
scheduler:
  extraArgs:
    bind-address: "0.0.0.0"
```

### 2.2 应用层面高可用

```yaml
# ========== 多区域部署策略 ==========
apiVersion: apps/v1
kind: Deployment
metadata:
  name: multi-region-app
  namespace: production
spec:
  replicas: 6
  selector:
    matchLabels:
      app: multi-region-app
  template:
    metadata:
      labels:
        app: multi-region-app
    spec:
      affinity:
        # 跨可用区分布
        podAntiAffinity:
          requiredDuringSchedulingIgnoredDuringExecution:
          - labelSelector:
              matchExpressions:
              - key: app
                operator: In
                values:
                - multi-region-app
            topologyKey: topology.kubernetes.io/zone
            
        # 节点亲和性
        nodeAffinity:
          preferredDuringSchedulingIgnoredDuringExecution:
          - weight: 100
            preference:
              matchExpressions:
              - key: topology.kubernetes.io/region
                operator: In
                values:
                - us-west-1
                - us-east-1
                
      # 拓扑分布约束
      topologySpreadConstraints:
      - maxSkew: 1
        topologyKey: topology.kubernetes.io/zone
        whenUnsatisfiable: DoNotSchedule
        labelSelector:
          matchLabels:
            app: multi-region-app
```

---

## 3. 安全加固指南

### 3.1 Pod安全标准

```yaml
# ========== 生产环境Pod安全配置 ==========
apiVersion: v1
kind: Pod
metadata:
  name: secure-pod
  namespace: production
spec:
  # 服务账户
  serviceAccountName: app-service-account
  
  # 安全上下文
  securityContext:
    runAsNonRoot: true
    runAsUser: 10001
    fsGroup: 2000
    supplementalGroups: [3000]
    
  containers:
  - name: app
    image: registry.example.com/secure-app:v1.0
    securityContext:
      # 容器安全设置
      allowPrivilegeEscalation: false
      readOnlyRootFilesystem: true
      runAsNonRoot: true
      runAsUser: 10001
      capabilities:
        drop:
        - ALL
        add:
        - NET_BIND_SERVICE  # 如需绑定低端口
        
    # 只读挂载重要目录
    volumeMounts:
    - name: tmpfs
      mountPath: /tmp
    - name: app-config
      mountPath: /config
      readOnly: true
      
  volumes:
  - name: tmpfs
    emptyDir:
      medium: Memory
  - name: app-config
    configMap:
      name: app-config
```

### 3.2 网络安全策略

```yaml
# ========== 生产网络安全策略 ==========
apiVersion: security.k8s.io/v1
kind: PodSecurityPolicy
metadata:
  name: production-psp
spec:
  privileged: false
  allowPrivilegeEscalation: false
  requiredDropCapabilities:
  - ALL
  volumes:
  - configMap
  - emptyDir
  - projected
  - secret
  - downwardAPI
  - persistentVolumeClaim
  hostNetwork: false
  hostIPC: false
  hostPID: false
  runAsUser:
    rule: MustRunAsNonRoot
  seLinux:
    rule: RunAsAny
  supplementalGroups:
    rule: MustRunAs
    ranges:
    - min: 1
      max: 65535
  fsGroup:
    rule: MustRunAs
    ranges:
    - min: 1
      max: 65535
  readOnlyRootFilesystem: true

---
# ========== RBAC最小权限原则 ==========
apiVersion: rbac.authorization.k8s.io/v1
kind: Role
metadata:
  namespace: production
  name: app-developer-role
rules:
- apiGroups: [""]
  resources: ["pods", "pods/log"]
  verbs: ["get", "list", "watch"]
- apiGroups: ["apps"]
  resources: ["deployments"]
  verbs: ["get", "list", "watch"]
- apiGroups: [""]
  resources: ["configmaps", "secrets"]
  verbs: ["get"]

---
apiVersion: rbac.authorization.k8s.io/v1
kind: RoleBinding
metadata:
  name: app-developer-binding
  namespace: production
subjects:
- kind: User
  name: developer@example.com
  apiGroup: rbac.authorization.k8s.io
roleRef:
  kind: Role
  name: app-developer-role
  apiGroup: rbac.authorization.k8s.io
```

---

## 4. 监控告警最佳实践

### 4.1 核心监控指标

```yaml
# ========== Prometheus核心告警规则 ==========
groups:
- name: kubernetes.system.rules
  rules:
  # API Server监控
  - alert: APIServerDown
    expr: up{job="apiserver"} == 0
    for: 2m
    labels:
      severity: critical
    annotations:
      summary: "API Server实例 {{ $labels.instance }} 不可用"
      description: "API Server已经宕机超过2分钟，请立即处理"

  - alert: APIServerLatencyHigh
    expr: histogram_quantile(0.99, rate(apiserver_request_duration_seconds_bucket[5m])) > 1
    for: 5m
    labels:
      severity: warning
    annotations:
      summary: "API Server响应延迟过高"
      description: "99th百分位响应时间超过1秒"

  # etcd监控
  - alert: EtcdNoLeader
    expr: etcd_server_has_leader == 0
    for: 1m
    labels:
      severity: critical
    annotations:
      summary: "etcd集群无领导者"
      description: "etcd集群已失去领导者超过1分钟"

  - alert: EtcdHighFsyncDuration
    expr: histogram_quantile(0.99, etcd_disk_backend_commit_duration_seconds_bucket) > 0.5
    for: 5m
    labels:
      severity: warning
    annotations:
      summary: "etcd磁盘同步延迟高"
      description: "99th百分位fsync延迟超过500ms"

  # 节点监控
  - alert: NodeNotReady
    expr: kube_node_status_condition{condition="Ready",status="true"} == 0
    for: 5m
    labels:
      severity: critical
    annotations:
      summary: "节点 {{ $labels.node }} 不可用"
      description: "节点已处于NotReady状态超过5分钟"

  - alert: NodeMemoryPressure
    expr: kube_node_status_condition{condition="MemoryPressure",status="true"} == 1
    for: 10m
    labels:
      severity: warning
    annotations:
      summary: "节点 {{ $labels.node }} 内存压力大"
      description: "节点内存使用率达到警告阈值"

  # Pod监控
  - alert: PodCrashLooping
    expr: rate(kube_pod_container_status_restarts_total[15m]) > 0.2
    for: 15m
    labels:
      severity: warning
    annotations:
      summary: "Pod {{ $labels.namespace }}/{{ $labels.pod }} 频繁重启"
      description: "Pod重启频率超过每分钟0.2次"

  - alert: PodNotReady
    expr: kube_pod_status_ready{condition="true"} == 0
    for: 15m
    labels:
      severity: warning
    annotations:
      summary: "Pod {{ $labels.namespace }}/{{ $labels.pod }} 未就绪"
      description: "Pod长时间未进入Ready状态"
```

### 4.2 应用监控配置

```yaml
# ========== ServiceMonitor配置 ==========
apiVersion: monitoring.coreos.com/v1
kind: ServiceMonitor
metadata:
  name: app-monitor
  namespace: monitoring
  labels:
    team: sre
spec:
  selector:
    matchLabels:
      app: production-app
  namespaceSelector:
    matchNames:
    - production
  endpoints:
  - port: http-metrics
    interval: 30s
    path: /metrics
    relabelings:
    - sourceLabels: [__meta_kubernetes_pod_name]
      targetLabel: pod
    - sourceLabels: [__meta_kubernetes_namespace]
      targetLabel: namespace
    - sourceLabels: [__meta_kubernetes_service_name]
      targetLabel: service
    
  # 自定义指标采集
  - port: http-app
    interval: 60s
    path: /actuator/prometheus
    params:
      include: ["jvm.memory.used", "http.server.requests"]
```

---

## 5. 灾备恢复方案

### 5.1 etcd备份策略

```bash
#!/bin/bash
# ========== etcd备份脚本 ==========
set -euo pipefail

BACKUP_DIR="/backup/etcd"
DATE=$(date +%Y%m%d_%H%M%S)
ETCDCTL_API=3

# 创建备份目录
mkdir -p ${BACKUP_DIR}/${DATE}

# 执行快照备份
etcdctl --endpoints=https://127.0.0.1:2379 \
  --cert=/etc/kubernetes/pki/etcd/server.crt \
  --key=/etc/kubernetes/pki/etcd/server.key \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  snapshot save ${BACKUP_DIR}/${DATE}/etcd-snapshot.db

# 验证备份完整性
etcdctl --write-out=table snapshot status ${BACKUP_DIR}/${DATE}/etcd-snapshot.db

# 压缩备份文件
tar -czf ${BACKUP_DIR}/${DATE}.tar.gz -C ${BACKUP_DIR} ${DATE}

# 清理旧备份（保留最近7天）
find ${BACKUP_DIR} -name "*.tar.gz" -mtime +7 -delete
find ${BACKUP_DIR} -mindepth 1 -maxdepth 1 -type d -empty -delete

echo "etcd backup completed: ${BACKUP_DIR}/${DATE}.tar.gz"
```

### 5.2 应用数据备份

```yaml
# ========== Velero备份配置 ==========
apiVersion: velero.io/v1
kind: Schedule
metadata:
  name: daily-backup
  namespace: velero
spec:
  schedule: "0 2 * * *"  # 每天凌晨2点
  template:
    includedNamespaces:
    - production
    - staging
    excludedNamespaces:
    - kube-system
    - monitoring
    includedResources:
    - deployments
    - services
    - configmaps
    - secrets
    - persistentvolumeclaims
    labelSelector:
      matchLabels:
        backup: enabled
    snapshotVolumes: true
    ttl: 168h  # 保留7天

---
# ========== 灾难恢复演练配置 ==========
apiVersion: velero.io/v1
kind: Restore
metadata:
  name: dr-test-restore
  namespace: velero
spec:
  backupName: daily-backup-20260205020000
  includedNamespaces:
  - production-dr-test
  restorePVs: true
  preserveNodePorts: true
```

---

## 6. 自动化运维策略

### 6.1 GitOps流水线

```yaml
# ========== ArgoCD应用配置 ==========
apiVersion: argoproj.io/v1alpha1
kind: Application
metadata:
  name: production-app
  namespace: argocd
spec:
  project: production
  source:
    repoURL: https://github.com/company/production-app.git
    targetRevision: HEAD
    path: k8s/overlays/production
    helm:
      valueFiles:
      - values-production.yaml
  destination:
    server: https://kubernetes.default.svc
    namespace: production
  syncPolicy:
    automated:
      prune: true
      selfHeal: true
    retry:
      limit: 5
      backoff:
        duration: 5s
        factor: 2
        maxDuration: 3m
    syncOptions:
    - CreateNamespace=true
    - PruneLast=true

---
# ========== 多环境配置管理 ==========
apiVersion: v1
kind: ConfigMap
metadata:
  name: app-environment-config
  namespace: production
data:
  # 生产环境特定配置
  DATABASE_URL: "postgresql://prod-db:5432/app"
  LOG_LEVEL: "WARN"
  CACHE_TTL: "300"
  ENABLE_DEBUG: "false"
  MAX_CONNECTIONS: "100"
  
  # 安全配置
  TLS_MIN_VERSION: "TLS1.2"
  HSTS_MAX_AGE: "31536000"
  CORS_ALLOWED_ORIGINS: "https://app.example.com"
```

### 6.2 自动扩缩容配置

```yaml
# ========== HPA高级配置 ==========
apiVersion: autoscaling/v2
kind: HorizontalPodAutoscaler
metadata:
  name: app-hpa
  namespace: production
spec:
  scaleTargetRef:
    apiVersion: apps/v1
    kind: Deployment
    name: app-deployment
  minReplicas: 3
  maxReplicas: 20
  metrics:
  - type: Resource
    resource:
      name: cpu
      target:
        type: Utilization
        averageUtilization: 70
  - type: Resource
    resource:
      name: memory
      target:
        type: Utilization
        averageUtilization: 80
  - type: Pods
    pods:
      metric:
        name: http_requests_per_second
      target:
        type: AverageValue
        averageValue: "100"
  behavior:
    scaleDown:
      stabilizationWindowSeconds: 300
      policies:
      - type: Percent
        value: 10
        periodSeconds: 60
    scaleUp:
      stabilizationWindowSeconds: 60
      policies:
      - type: Percent
        value: 50
        periodSeconds: 60
      selectPolicy: Max

---
# ========== VPA配置 ==========
apiVersion: autoscaling.k8s.io/v1
kind: VerticalPodAutoscaler
metadata:
  name: app-vpa
  namespace: production
spec:
  targetRef:
    apiVersion: apps/v1
    kind: Deployment
    name: app-deployment
  updatePolicy:
    updateMode: "Auto"
  resourcePolicy:
    containerPolicies:
    - containerName: app
      minAllowed:
        cpu: 100m
        memory: 256Mi
      maxAllowed:
        cpu: 2000m
        memory: 4Gi
      controlledResources: ["cpu", "memory"]
```

---

## 7. 成本优化实践

### 7.1 资源优化策略

```yaml
# ========== 成本优化资源配置 ==========
apiVersion: apps/v1
kind: Deployment
metadata:
  name: cost-optimized-app
  namespace: production
spec:
  replicas: 3
  template:
    spec:
      # Spot实例容忍
      tolerations:
      - key: spot-instance
        operator: Equal
        value: "true"
        effect: NoSchedule
        
      # 节点亲和性 - 优先使用成本较低的实例
      affinity:
        nodeAffinity:
          preferredDuringSchedulingIgnoredDuringExecution:
          - weight: 100
            preference:
              matchExpressions:
              - key: node.kubernetes.io/instance-type
                operator: In
                values:
                - t3.medium
                - t3.large
          - weight: 50
            preference:
              matchExpressions:
              - key: cloud.google.com/gke-preemptible
                operator: In
                values:
                - "true"
                
      containers:
      - name: app
        image: app:v1.0
        resources:
          requests:
            # 基于实际使用量精确配置
            cpu: "150m"
            memory: "384Mi"
          limits:
            # 合理的上限，避免浪费
            cpu: "500m"
            memory: "768Mi"
            
        # 应用层优化
        env:
        - name: JAVA_OPTS
          value: "-Xmx640m -Xms384m -XX:MaxRAMPercentage=80.0"
        - name: GOMEMLIMIT
          value: "680MiB"
```

### 7.2 成本监控告警

```yaml
# ========== 成本监控告警规则 ==========
groups:
- name: cost.monitoring.rules
  rules:
  - alert: HighResourceUtilizationCost
    expr: avg(rate(container_cpu_usage_seconds_total[1h])) by (namespace) * 100 > 80
    for: 1h
    labels:
      severity: warning
    annotations:
      summary: "命名空间 {{ $labels.namespace }} CPU使用率过高"
      description: "平均CPU使用率超过80%，可能存在资源配置过度"

  - alert: MemoryOverProvisioned
    expr: (kube_pod_container_resource_limits_memory_bytes - container_memory_working_set_bytes) / kube_pod_container_resource_limits_memory_bytes * 100 > 50
    for: 6h
    labels:
      severity: info
    annotations:
      summary: "内存过度配置"
      description: "Pod内存预留量超过实际使用量50%以上"

  - alert: UnusedPersistentVolumes
    expr: kube_persistentvolume_status_phase{phase="Available"} == 1
    for: 24h
    labels:
      severity: warning
    annotations:
      summary: "存在未使用的持久卷"
      description: "检测到闲置的PV，建议清理以降低成本"
```

---

## 8. 多集群管理规范

### 8.1 集群联邦配置

```yaml
# ========== Cluster API配置 ==========
apiVersion: cluster.x-k8s.io/v1beta1
kind: Cluster
metadata:
  name: production-cluster-us-west
  namespace: capi-system
spec:
  clusterNetwork:
    services:
      cidrBlocks: ["10.128.0.0/12"]
    pods:
      cidrBlocks: ["10.0.0.0/8"]
    serviceDomain: "cluster.local"
  infrastructureRef:
    apiVersion: infrastructure.cluster.x-k8s.io/v1beta1
    kind: AWSCluster
    name: production-cluster-us-west
  controlPlaneRef:
    apiVersion: controlplane.cluster.x-k8s.io/v1beta1
    kind: KubeadmControlPlane
    name: production-cluster-us-west-control-plane

---
# ========== 多集群服务发现 ==========
apiVersion: multicluster.x-k8s.io/v1alpha1
kind: ServiceExport
metadata:
  name: global-service
  namespace: production
spec: {}

---
apiVersion: multicluster.x-k8s.io/v1alpha1
kind: ServiceImport
metadata:
  name: global-service
  namespace: production
spec:
  type: ClusterSetIP
  ports:
  - name: http
    protocol: TCP
    port: 80
```

### 8.2 统一监控配置

```yaml
# ========== Thanos多集群监控 ==========
apiVersion: v1
kind: Service
metadata:
  name: thanos-sidecar
  namespace: monitoring
  labels:
    app: thanos-sidecar
spec:
  ports:
  - name: grpc
    port: 10901
    targetPort: 10901
  - name: http
    port: 10902
    targetPort: 10902
  clusterIP: None

---
apiVersion: apps/v1
kind: StatefulSet
metadata:
  name: thanos-query
  namespace: monitoring
spec:
  serviceName: thanos-query
  replicas: 2
  selector:
    matchLabels:
      app: thanos-query
  template:
    metadata:
      labels:
        app: thanos-query
    spec:
      containers:
      - name: thanos-query
        image: quay.io/thanos/thanos:v0.32.0
        args:
        - query
        - --grpc-address=0.0.0.0:10901
        - --http-address=0.0.0.0:10902
        - --store=dnssrv+_grpc._tcp.thanos-sidecar.monitoring.svc.cluster.local
        - --query.replica-label=replica
        ports:
        - name: grpc
          containerPort: 10901
        - name: http
          containerPort: 10902
```

---

## 9. 生产环境故障应急响应

### 9.1 故障分级响应机制

| 故障等级 | 响应时间 | 通知范围 | 处理流程 | 记录要求 |
|---------|---------|---------|---------|---------|
| **P0 - 核心服务中断** | 5分钟内响应 | 全体技术团队+管理层 | 立即组建应急小组，启动应急预案 | 详细故障时间线记录 |
| **P1 - 重要功能异常** | 30分钟内响应 | 相关技术团队 | 指定负责人处理，定期同步进展 | 故障分析报告必填 |
| **P2 - 一般性问题** | 2小时内响应 | 对应模块负责人 | 按正常流程处理，纳入周报 | 问题跟踪记录 |
| **P3 - 优化建议类** | 下一工作日处理 | 相关人员 | 纳入改进计划 | 需求池管理 |

### 9.2 应急响应标准操作程序(SOP)

```bash
#!/bin/bash
# ========== 生产环境应急响应脚本 ==========
set -euo pipefail

INCIDENT_ID=$(date +%Y%m%d_%H%M%S)_${RANDOM}
INCIDENT_DIR="/var/incidents/${INCIDENT_ID}"
mkdir -p ${INCIDENT_DIR}

log_incident() {
    local severity=$1
    local component=$2
    local description=$3
    
    echo "$(date '+%Y-%m-%d %H:%M:%S') [${severity}] ${component}: ${description}" | \
        tee -a ${INCIDENT_DIR}/incident.log
    
    # 发送告警通知
    case ${severity} in
        "P0")
            # 紧急通知所有相关人员
            send_emergency_alert "${description}"
            ;;
        "P1")
            # 通知相关技术团队
            send_team_alert "${component}" "${description}"
            ;;
    esac
}

# 故障诊断函数
diagnose_cluster_health() {
    echo "=== 集群健康状态诊断 ===" > ${INCIDENT_DIR}/diagnosis.txt
    
    # 检查控制平面状态
    kubectl get componentstatuses >> ${INCIDENT_DIR}/diagnosis.txt 2>&1
    
    # 检查节点状态
    kubectl get nodes -o wide >> ${INCIDENT_DIR}/diagnosis.txt 2>&1
    
    # 检查关键系统Pod状态
    kubectl get pods -n kube-system >> ${INCIDENT_DIR}/diagnosis.txt 2>&1
    
    # 检查事件日志
    kubectl get events --sort-by='.lastTimestamp' -A | tail -20 >> ${INCIDENT_DIR}/diagnosis.txt
}

# 自动化恢复尝试
attempt_auto_recovery() {
    local component=$1
    
    case ${component} in
        "coredns")
            echo "尝试重启CoreDNS..."
            kubectl rollout restart deployment coredns -n kube-system
            ;;
        "kube-proxy")
            echo "尝试重启kube-proxy DaemonSet..."
            kubectl delete pods -n kube-system -l k8s-app=kube-proxy
            ;;
        *)
            echo "组件${component}暂无自动恢复策略"
            return 1
            ;;
    esac
}

# 使用示例
# log_incident "P0" "API Server" "API Server响应超时，影响集群管理"
# diagnose_cluster_health
# attempt_auto_recovery "coredns"
```

### 9.3 故障复盘与改进

```yaml
# ========== 故障复盘模板 ==========
apiVersion: incident.review/v1
kind: PostMortemReport
metadata:
  name: incident-${INCIDENT_ID}
spec:
  incidentDetails:
    startTime: "2026-02-05T14:30:00Z"
    endTime: "2026-02-05T15:45:00Z"
    duration: "1h15m"
    severity: "P0"
    affectedServices:
    - name: user-api-service
      impact: "50%请求失败"
    - name: order-processing
      impact: "完全不可用"
  
  timeline:
  - time: "14:30"
    event: "监控系统告警：API Server响应时间超过阈值"
    actor: "Prometheus Alertmanager"
  - time: "14:32"
    event: "值班工程师确认问题并通知SRE团队"
    actor: "on-call engineer"
  - time: "14:35"
    event: "启动应急响应流程，创建故障工单"
    actor: "incident commander"
  - time: "14:40"
    event: "初步诊断发现etcd集群出现网络分区"
    actor: "SRE team"
  - time: "15:10"
    event: "执行etcd集群恢复操作"
    actor: "database specialist"
  - time: "15:30"
    event: "服务恢复正常，开始验证"
    actor: "QA team"
  - time: "15:45"
    event: "确认服务稳定，关闭故障工单"
    actor: "incident commander"
  
  rootCauseAnalysis:
    primaryCause: "etcd集群网络分区导致脑裂"
    contributingFactors:
    - 网络设备固件bug
    - 缺乏网络健康检查机制
    - 故障转移测试不充分
    
  correctiveActions:
  - immediate:
    - 修复网络设备固件
    - 增加etcd健康检查频率
    - 完善故障转移测试流程
  - longTerm:
    - 部署网络监控系统
    - 建立多地域etcd集群
    - 完善灾难恢复预案
  
  lessonsLearned:
  - 网络基础设施的可靠性直接影响集群稳定性
  - 需要建立更完善的监控告警体系
  - 定期进行故障演练的重要性
```

---

## 10. 生产环境安全最佳实践

### 10.1 零信任安全实施框架

```yaml
# ========== 生产环境零信任安全配置 ==========
apiVersion: security.k8s.io/v1
kind: PodSecurityPolicy
metadata:
  name: production-restricted
spec:
  privileged: false
  allowPrivilegeEscalation: false
  requiredDropCapabilities:
  - ALL
  volumes:
  - configMap
  - emptyDir
  - projected
  - secret
  - downwardAPI
  - persistentVolumeClaim
  hostNetwork: false
  hostIPC: false
  hostPID: false
  runAsUser:
    rule: MustRunAsNonRoot
  seLinux:
    rule: RunAsAny
  supplementalGroups:
    rule: MustRunAs
    ranges:
    - min: 1
      max: 65535
  fsGroup:
    rule: MustRunAs
    ranges:
    - min: 1
      max: 65535
  readOnlyRootFilesystem: true

---
# ========== 网络策略实施 ==========
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: default-deny-all
  namespace: production
spec:
  podSelector: {}
  policyTypes:
  - Ingress
  - Egress
  ingress:
  - from:
    - namespaceSelector:
        matchLabels:
          name: monitoring
      podSelector:
        matchLabels:
          app: prometheus
    ports:
    - protocol: TCP
      port: 9090
  egress:
  - to:
    - namespaceSelector:
        matchLabels:
          name: kube-system
    ports:
    - protocol: UDP
      port: 53
    - protocol: TCP
      port: 53

---
# ========== RBAC最小权限配置 ==========
apiVersion: rbac.authorization.k8s.io/v1
kind: Role
metadata:
  namespace: production
  name: app-developer-role
rules:
- apiGroups: [""]
  resources: ["pods", "pods/log", "services", "configmaps"]
  verbs: ["get", "list", "watch"]
- apiGroups: ["apps"]
  resources: ["deployments", "statefulsets"]
  verbs: ["get", "list", "watch"]
- apiGroups: ["batch"]
  resources: ["jobs", "cronjobs"]
  verbs: ["get", "list", "watch"]

---
apiVersion: rbac.authorization.k8s.io/v1
kind: RoleBinding
metadata:
  name: app-developer-binding
  namespace: production
subjects:
- kind: User
  name: developer@example.com
  apiGroup: rbac.authorization.k8s.io
roleRef:
  kind: Role
  name: app-developer-role
  apiGroup: rbac.authorization.k8s.io
```

### 10.2 安全监控与告警策略

| 安全维度 | 监控指标 | 告警阈值 | 响应动作 | 处理时效 |
|---------|---------|---------|---------|---------|
| **身份认证** | 异常登录尝试、令牌泄露 | >5次失败登录/小时 | 立即锁定账户 | 5分钟 |
| **权限变更** | RBAC规则修改、ServiceAccount变更 | 任何未授权变更 | 安全审计、回滚变更 | 30分钟 |
| **网络访问** | 异常端口访问、外部连接 | 连接到黑名单IP | 阻断流量、安全调查 | 15分钟 |
| **镜像安全** | 漏洞扫描结果、基线不符合 | Critical/High漏洞 | 阻断部署、紧急修复 | 1小时 |
| **运行时安全** | 异常系统调用、文件修改 | 违反安全策略 | 隔离容器、告警通知 | 10分钟 |

### 10.3 合规性自动化检查

```bash
#!/bin/bash
# ========== Kubernetes安全合规检查脚本 ==========
set -euo pipefail

COMPLIANCE_REPORT="/var/reports/compliance-$(date +%Y%m%d).txt"
echo "Kubernetes安全合规检查报告 - $(date)" > ${COMPLIANCE_REPORT}

# CIS基准检查
check_cis_benchmark() {
    echo "=== CIS Kubernetes Benchmark 检查 ===" >> ${COMPLIANCE_REPORT}
    
    # 检查API Server配置
    if kubectl get pod -n kube-system -l component=kube-apiserver -o jsonpath='{.items[*].spec.containers[*].command}' | \
       grep -q "anonymous-auth=false"; then
        echo "✅ API Server匿名认证已禁用" >> ${COMPLIANCE_REPORT}
    else
        echo "❌ API Server匿名认证未禁用" >> ${COMPLIANCE_REPORT}
    fi
    
    # 检查etcd加密
    if kubectl get pod -n kube-system -l component=etcd -o jsonpath='{.items[*].spec.containers[*].command}' | \
       grep -q "auto-tls=true"; then
        echo "✅ etcd自动TLS已启用" >> ${COMPLIANCE_REPORT}
    else
        echo "❌ etcd自动TLS未启用" >> ${COMPLIANCE_REPORT}
    fi
    
    # 检查Pod安全策略
    psp_count=$(kubectl get psp --no-headers | wc -l)
    if [ ${psp_count} -gt 0 ]; then
        echo "✅ 已配置${psp_count}个Pod安全策略" >> ${COMPLIANCE_REPORT}
    else
        echo "❌ 未配置Pod安全策略" >> ${COMPLIANCE_REPORT}
    fi
}

# GDPR合规检查
check_gdpr_compliance() {
    echo -e "\n=== GDPR合规检查 ===" >> ${COMPLIANCE_REPORT}
    
    # 检查数据加密
    secrets_encrypted=$(kubectl get secrets -A --no-headers | wc -l)
    echo "🔒 加密Secret数量: ${secrets_encrypted}" >> ${COMPLIANCE_REPORT}
    
    # 检查日志保留策略
    log_retention_days=$(kubectl get cm -n kube-system kube-proxy -o jsonpath='{.data.config\.yaml}' | \
                        grep -o "log-flush-frequency=[0-9]*" | cut -d'=' -f2 || echo "未配置")
    echo "📝 日志刷新频率: ${log_retention_days}s" >> ${COMPLIANCE_REPORT}
}

check_cis_benchmark
check_gdpr_compliance

echo -e "\n合规检查完成，详情请查看: ${COMPLIANCE_REPORT}"
```

## 11. 成本优化与资源管理

### 11.1 资源配额与限制管理

```yaml
# ========== 生产环境资源配额配置 ==========
apiVersion: v1
kind: ResourceQuota
metadata:
  name: production-quota
  namespace: production
spec:
  hard:
    # 计算资源配额
    requests.cpu: "100"
    requests.memory: "200Gi"
    limits.cpu: "200"
    limits.memory: "400Gi"
    
    # 存储资源配额
    requests.storage: "10Ti"
    persistentvolumeclaims: "1000"
    
    # 对象数量配额
    pods: "10000"
    services: "500"
    secrets: "1000"
    configmaps: "1000"

---
# ========== LimitRange配置 ==========
apiVersion: v1
kind: LimitRange
metadata:
  name: production-limits
  namespace: production
spec:
  limits:
  - type: Container
    default:
      cpu: "1"
      memory: "1Gi"
    defaultRequest:
      cpu: "100m"
      memory: "128Mi"
    max:
      cpu: "8"
      memory: "16Gi"
    min:
      cpu: "10m"
      memory: "16Mi"
  - type: Pod
    max:
      cpu: "16"
      memory: "32Gi"
```

### 11.2 成本监控与优化策略

| 优化维度 | 监控指标 | 优化策略 | 预期收益 | 实施复杂度 |
|---------|---------|---------|---------|-----------|
| **节点资源** | CPU/内存利用率、节点空闲率 | 水平扩缩容、节点池优化 | 20-40%成本节约 | ⭐⭐ |
| **存储成本** | PVC使用率、快照保留 | 生命周期管理、冷热数据分离 | 30-50%存储节约 | ⭐⭐⭐ |
| **网络费用** | 流量使用、跨区域传输 | CDN优化、就近部署 | 25-35%网络节约 | ⭐⭐ |
| **Spot实例** | 按需/竞价实例比例 | 智能调度策略 | 50-80%计算节约 | ⭐⭐⭐⭐ |
| **镜像缓存** | 镜像拉取次数、缓存命中率 | 镜像预热、本地缓存 | 15-25%拉取节约 | ⭐⭐ |

### 11.3 成本优化自动化脚本

```bash
#!/bin/bash
# ========== Kubernetes成本优化分析脚本 ==========
set -euo pipefail

COST_ANALYSIS_DIR="/var/cost-analysis/$(date +%Y%m%d)"
mkdir -p ${COST_ANALYSIS_DIR}

analyze_cluster_costs() {
    echo "=== 集群成本分析报告 ===" > ${COST_ANALYSIS_DIR}/cost-report.txt
    
    # 节点成本分析
    echo "节点成本分布:" >> ${COST_ANALYSIS_DIR}/cost-report.txt
    kubectl get nodes -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{.status.capacity.cpu}{"\t"}{.status.capacity.memory}{"\n"}{end}' | \
    while read node cpu mem; do
        # 基于实例类型的估算成本（示例价格）
        case ${node} in
            *m5.large*) hourly_cost=0.096 ;;
            *m5.xlarge*) hourly_cost=0.192 ;;
            *m5.2xlarge*) hourly_cost=0.384 ;;
            *) hourly_cost=0.200 ;;  # 默认价格
        esac
        monthly_cost=$(echo "${hourly_cost} * 730" | bc -l)
        echo "${node}: $${monthly_cost}/月 (${cpu}vCPU, ${mem})" >> ${COST_ANALYSIS_DIR}/cost-report.txt
    done
    
    # Pod资源使用分析
    echo -e "\nPod资源使用效率:" >> ${COST_ANALYSIS_DIR}/cost-report.txt
    kubectl top pods -A --no-headers | \
    awk '{
        cpu_req=$3+0; mem_req=$4+0;
        cpu_util=$5+0; mem_util=$6+0;
        cpu_efficiency = (cpu_util/cpu_req)*100;
        mem_efficiency = (mem_util/mem_req)*100;
        if(cpu_efficiency < 30 || mem_efficiency < 30) {
            print $1"/"$2": CPU效率="cpu_efficiency"% Memory效率="mem_efficiency"%"
        }
    }' >> ${COST_ANALYSIS_DIR}/cost-report.txt
    
    # 存储成本分析
    echo -e "\n存储成本分析:" >> ${COST_ANALYSIS_DIR}/cost-report.txt
    kubectl get pvc -A -o jsonpath='{range .items[*]}{.metadata.namespace}{"\t"}{.metadata.name}{"\t"}{.spec.resources.requests.storage}{"\n"}{end}' | \
    while read ns pvc size; do
        # 基于存储类型的估算成本
        storage_cost=$(echo "${size%Gi} * 0.10" | bc -l)  # $0.10/GiB/月
        echo "${ns}/${pvc}: ${size} ($${storage_cost}/月)" >> ${COST_ANALYSIS_DIR}/cost-report.txt
    done
}

generate_optimization_recommendations() {
    echo -e "\n=== 优化建议 ===" >> ${COST_ANALYSIS_DIR}/cost-report.txt
    
    # 低效Pod推荐
    echo "建议优化的Pod:" >> ${COST_ANALYSIS_DIR}/cost-report.txt
    kubectl top pods -A --no-headers | \
    awk '$5 < 30 || $6 < 30 {print $1"/"$2" - 资源使用率低"}' >> ${COST_ANALYSIS_DIR}/cost-report.txt
    
    # 节点优化建议
    echo -e "\n节点优化建议:" >> ${COST_ANALYSIS_DIR}/cost-report.txt
    kubectl get nodes -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{.status.allocatable.cpu}{"\n"}{end}' | \
    while read node allocatable; do
        pod_count=$(kubectl get pods --field-selector spec.nodeName=${node} --no-headers | wc -l)
        pods_per_core=$(echo "${pod_count}/${allocatable}" | bc -l)
        if (( $(echo "${pods_per_core} < 2" | bc -l) )); then
            echo "${node}: CPU利用率低，考虑缩小实例规格" >> ${COST_ANALYSIS_DIR}/cost-report.txt
        fi
    done
}

analyze_cluster_costs
generate_optimization_recommendations

echo "成本分析报告已生成: ${COST_ANALYSIS_DIR}/cost-report.txt"
```

## 12. 变更管理与发布策略

### 12.1 GitOps流水线最佳实践

```yaml
# ========== ArgoCD应用配置模板 ==========
apiVersion: argoproj.io/v1alpha1
kind: Application
metadata:
  name: production-app-template
  namespace: argocd
spec:
  project: production
  source:
    repoURL: https://github.com/company/production-app.git
    targetRevision: HEAD
    path: k8s/overlays/production
    helm:
      valueFiles:
      - values-production.yaml
      parameters:
      - name: image.tag
        value: ${ARGOCD_APP_REVISION}
  destination:
    server: https://kubernetes.default.svc
    namespace: production
  syncPolicy:
    automated:
      prune: true
      selfHeal: true
      allowEmpty: false
    retry:
      limit: 5
      backoff:
        duration: 5s
        factor: 2
        maxDuration: 3m
    syncOptions:
    - CreateNamespace=true
    - PruneLast=true
    - RespectIgnoreDifferences=true
    - ApplyOutOfSyncOnly=true

---
# ========== 多环境配置管理 ==========
apiVersion: v1
kind: ConfigMap
metadata:
  name: app-environment-config
  namespace: production
data:
  # 生产环境特定配置
  DATABASE_URL: "postgresql://prod-db.cluster.local:5432/app"
  REDIS_URL: "redis://prod-redis.cluster.local:6379"
  LOG_LEVEL: "WARN"
  CACHE_TTL: "300"
  ENABLE_DEBUG: "false"
  MAX_CONNECTIONS: "100"
  
  # 安全配置
  TLS_MIN_VERSION: "TLS1.3"
  HSTS_MAX_AGE: "31536000"
  CORS_ALLOWED_ORIGINS: "https://app.example.com"
  SECURITY_HEADERS: |
    Strict-Transport-Security: max-age=31536000; includeSubDomains
    X-Content-Type-Options: nosniff
    X-Frame-Options: DENY
    Content-Security-Policy: default-src 'self'
```

### 12.2 渐进式发布策略

| 发布策略 | 实施方式 | 风险控制 | 监控指标 | 回滚机制 |
|---------|---------|---------|---------|---------|
| **蓝绿部署** | 维护两套完整环境 | 零停机时间 | 健康检查、性能指标 | 一键切换回旧环境 |
| **金丝雀发布** | 逐步增加新版本流量 | 限制影响范围 | 错误率、延迟指标 | 自动回滚到稳定版本 |
| **滚动更新** | 逐个替换Pod实例 | 原地升级 | 就绪探针、存活探针 | 失败时暂停并回滚 |
| **功能开关** | 代码层面控制功能 | 精确控制范围 | 业务指标、用户反馈 | 动态开启/关闭功能 |

### 12.3 变更审批与审计流程

```yaml
# ========== 变更管理流程配置 ==========
apiVersion: changemanagement.example.com/v1
kind: ChangeRequest
metadata:
  name: cr-20260205-001
spec:
  changeType: "Production Deployment"
  priority: "High"
  affectedSystems:
  - name: "user-service"
    environment: "production"
    criticality: "Business Critical"
  
  approvalWorkflow:
    reviewers:
    - role: "SRE Team Lead"
      required: true
    - role: "Security Officer"
      required: true
    - role: "Product Owner"
      required: false
    
    approvalConditions:
    - type: "Automated Tests"
      status: "Passed"
      required: true
    - type: "Security Scan"
      status: "Clean"
      required: true
    - type: "Performance Test"
      status: "Within Threshold"
      required: true
  
  rollbackPlan:
    triggerConditions:
    - metric: "error_rate"
      threshold: "5%"
      duration: "5m"
    - metric: "response_time"
      threshold: "2s"
      duration: "10m"
    - metric: "business_impact"
      threshold: "significant_degradation"
      duration: "immediate"
    
    rollbackActions:
    - action: "argo_rollout_undo"
      target: "user-service"
      timeout: "300s"
    - action: "notification_slack"
      target: "#production-alerts"
      message: "Automatic rollback triggered for user-service"
```

---

**表格底部标记**: Kusheet Project | 作者: Allen Galler (allengaller@gmail.com) | 最后更新: 2026-02 | 版本: v1.25-v1.32 | 质量等级: ⭐⭐⭐⭐⭐ 专家级