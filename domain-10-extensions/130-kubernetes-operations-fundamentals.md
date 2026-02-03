# 130 - Kubernetes 运维基础技能：日志管理、备份恢复、安全加固与性能调优

## 日志管理与分析

### 集中式日志架构

```bash
# 日志收集架构
┌─────────────────┐    ┌─────────────────┐    ┌─────────────────┐
│   Kubernetes    │───▶│  Fluentd/Filebeat│───▶│   Elasticsearch │
│   Pods/Nodes    │    │    Logstash     │    │                 │
└─────────────────┘    └─────────────────┘    └─────────────────┘
                                │                       │
                                ▼                       ▼
                        ┌─────────────────┐    ┌─────────────────┐
                        │   Kafka/RabbitMQ│    │   Kibana        │
                        │   (缓冲队列)    │    │   (可视化)      │
                        └─────────────────┘    └─────────────────┘
```

### 日志收集方案对比

| 方案 | 优势 | 劣势 | 适用场景 |
|------|------|------|----------|
| **EFK Stack** | 集成度高，易部署 | 资源消耗大 | 中小规模集群 |
| **ELK Stack** | 功能丰富，社区支持好 | 复杂度高 | 企业级应用 |
| **Loki+Promtail** | 轻量级，成本低 | 查询功能受限 | 云原生环境 |
| **Fluentd+Elasticsearch** | 高性能，灵活配置 | 学习曲线陡峭 | 大规模集群 |

### 日志收集最佳实践

```yaml
# fluent-bit-configmap.yaml
apiVersion: v1
kind: ConfigMap
metadata:
  name: fluent-bit-config
  namespace: logging
data:
  fluent-bit.conf: |
    [SERVICE]
        Flush         1
        Log_Level     info
        Daemon        off
        Parsers_File  parsers.conf
        HTTP_Server   On
        HTTP_Listen   0.0.0.0
        HTTP_Port     2020

    [INPUT]
        Name              tail
        Path              /var/log/containers/*.log
        Parser            docker
        Tag               kube.*
        Refresh_Interval  5
        Mem_Buf_Limit     5MB
        Skip_Long_Lines   On
        DB                /var/log/flb_kube.db
        DB.Sync           Normal

    [FILTER]
        Name                kubernetes
        Match               kube.*
        Kube_URL            https://kubernetes.default.svc:443
        Kube_CA_File        /var/run/secrets/kubernetes.io/serviceaccount/ca.crt
        Kube_Token_File     /var/run/secrets/kubernetes.io/serviceaccount/token
        Kube_Tag_Prefix     kube.var.log.containers.
        Merge_Log           On
        Merge_Log_Key       log_processed
        K8S-Logging.Parser  On
        K8S-Logging.Exclude Off

    [OUTPUT]
        Name            es
        Match           *
        Host            ${FLUENT_ELASTICSEARCH_HOST}
        Port            ${FLUENT_ELASTICSEARCH_PORT}
        Index           kubernetes_cluster
        Type            flb_type
        Logstash_Format On
        Replace_Dots    On
        Retry_Limit     False
```

### 日志查询与分析

```bash
# Kibana 查询示例
# 错误日志查询
kubernetes.container_name:myapp AND level:error

# 性能相关查询
kubernetes.container_name:myapp AND response_time:[500 TO *]

# 异常堆栈查询
exception OR stacktrace OR panic

# 时间范围查询
@timestamp:[now-1h TO now]
```

## 备份与恢复策略

### Velero 备份配置

```yaml
# velero-backup-config.yaml
---
apiVersion: v1
kind: Namespace
metadata:
  name: velero
---
apiVersion: v1
kind: Secret
metadata:
  namespace: velero
  name: cloud-credentials
stringData:
  cloud: |
    [default]
    aws_access_key_id=YOUR_ACCESS_KEY
    aws_secret_access_key=YOUR_SECRET_KEY
---
apiVersion: velero.io/v1
kind: BackupStorageLocation
metadata:
  name: default
  namespace: velero
spec:
  provider: aws
  objectStorage:
    bucket: velero-backups-bucket
    prefix: cluster1
  config:
    region: us-west-2
    s3ForcePathStyle: "true"
    s3Url: http://minio.velero.svc:9000
---
apiVersion: velero.io/v1
kind: VolumeSnapshotLocation
metadata:
  name: default
  namespace: velero
spec:
  provider: aws
  config:
    region: us-west-2
```

### 备份策略配置

```yaml
# backup-schedules.yaml
---
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
    excludedNamespaces: []
    includedResources:
    - "*"
    excludedResources:
    - events
    - events.events.k8s.io
    - backups.velero.io
    - restores.velero.io
    - resticrepositories.velero.io
    ttl: "240h"  # 10天
---
apiVersion: velero.io/v1
kind: Schedule
metadata:
  name: weekly-backup
  namespace: velero
spec:
  schedule: "0 1 * * 0"  # 每周日凌晨1点
  template:
    includedNamespaces:
    - "*"
    excludedNamespaces:
    - velero
    - kube-system
    - kube-public
    - kube-node-lease
    snapshotVolumes: true
    ttl: "1680h"  # 70天
```

### 恢复操作示例

```bash
# 列出备份
velero get backups

# 恢复特定备份
velero restore create --from-backup prod-backup-20231201

# 恢复特定命名空间
velero restore create --from-backup prod-backup-20231201 --include-namespaces=production

# 恢复特定资源类型
velero restore create --from-backup prod-backup-20231201 --include-resources=pods,services

# 恢复到不同命名空间
velero restore create --from-backup prod-backup-20231201 --namespace-mappings=production:staging

# 检查恢复状态
velero restore get
velero restore describe <restore-name>
```

### etcd 备份与恢复

```bash
# etcd 备份脚本
#!/bin/bash
ETCDCTL_API=3 etcdctl \
  --endpoints=https://127.0.0.1:2379 \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/server.crt \
  --key=/etc/kubernetes/pki/etcd/server.key \
  snapshot save /var/lib/etcd-backup/etcd-snapshot-$(date +%Y%m%d-%H%M%S).db

# etcd 恢复步骤
# 1. 停止所有控制平面组件
systemctl stop kube-apiserver
systemctl stop etcd

# 2. 恢复 etcd 数据
ETCDCTL_API=3 etcdctl snapshot restore /path/to/etcd-snapshot.db \
  --data-dir=/var/lib/etcd-restored \
  --initial-cluster="master1=https://master1-ip:2380,master2=https://master2-ip:2380,master3=https://master3-ip:2380" \
  --initial-advertise-peer-urls=https://master-ip:2380

# 3. 更新 etcd 配置指向新数据目录
# 4. 重启 etcd 服务
systemctl start etcd
```

## 安全加固措施

### RBAC 最佳实践

```yaml
# rbac-security-best-practices.yaml
---
# 最小权限原则示例
apiVersion: v1
kind: ServiceAccount
metadata:
  name: restricted-app-sa
  namespace: production
---
apiVersion: rbac.authorization.k8s.io/v1
kind: Role
metadata:
  namespace: production
  name: pod-reader
rules:
- apiGroups: [""]
  resources: ["pods"]
  verbs: ["get", "watch", "list"]
---
apiVersion: rbac.authorization.k8s.io/v1
kind: RoleBinding
metadata:
  name: read-pods
  namespace: production
subjects:
- kind: ServiceAccount
  name: restricted-app-sa
  namespace: production
roleRef:
  kind: Role
  name: pod-reader
  apiGroup: rbac.authorization.k8s.io
---
# 禁止特权用户
apiVersion: rbac.authorization.k8s.io/v1
kind: ClusterRole
metadata:
  name: privileged-restriction
rules:
- apiGroups: [""]
  resources: ["pods/securitycontextconstraints"]
  verbs: ["use"]
  resourceNames: ["restricted"]
```

### Pod 安全标准 (PSS)

```yaml
# pod-security-standards.yaml
apiVersion: v1
kind: Namespace
metadata:
  name: secure-apps
  labels:
    pod-security.kubernetes.io/enforce: restricted
    pod-security.kubernetes.io/audit: restricted
    pod-security.kubernetes.io/warn: restricted
---
# 自定义限制策略
apiVersion: v1
kind: LimitRange
metadata:
  name: security-limitrange
  namespace: secure-apps
spec:
  limits:
  - type: Container
    default:
      allowPrivilegeEscalation: false
      runAsNonRoot: true
      privileged: false
      readOnlyRootFilesystem: false  # 根据应用需求调整
```

### 网络策略安全

```yaml
# network-security-policies.yaml
---
# 默认拒绝所有流量
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
# 允许同一命名空间内部通信
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: allow-same-namespace
  namespace: production
spec:
  podSelector: {}
  ingress:
  - from:
    - podSelector: {}
  policyTypes:
  - Ingress
---
# 允许特定标签间的通信
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: allow-app-to-db
  namespace: production
spec:
  podSelector:
    matchLabels:
      app: database
  policyTypes:
  - Ingress
  ingress:
  - from:
    - podSelector:
        matchLabels:
          app: frontend
    ports:
    - protocol: TCP
      port: 5432
```

### 安全扫描与合规

```bash
# 使用 Trivy 扫描镜像
trivy image nginx:latest
trivy image --security-checks vuln,config --exit-code 1 myapp:latest

# 扫描 Kubernetes 配置
trivy config --severity HIGH,CRITICAL ./k8s-manifests/

# 使用 Kubesec 扫描
curl -sSX POST --data-binary @deployment.yaml http://kubesec.io:8080/scan

# 使用 Conftest 验证配置
conftest test -p policies/ deployment.yaml
```

## 性能调优指南

### 资源请求与限制

```yaml
# resource-optimization.yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: optimized-app
spec:
  replicas: 3
  selector:
    matchLabels:
      app: optimized-app
  template:
    metadata:
      labels:
        app: optimized-app
    spec:
      containers:
      - name: app
        image: myapp:latest
        # 基于实际使用情况设置
        resources:
          requests:
            cpu: "500m"
            memory: "512Mi"
          limits:
            cpu: "1000m"  # 防止资源滥用
            memory: "1Gi"   # 防止内存泄漏导致OOM
        # 健康检查优化
        livenessProbe:
          httpGet:
            path: /healthz
            port: 8080
          initialDelaySeconds: 30
          periodSeconds: 10
          timeoutSeconds: 3
          failureThreshold: 3
        readinessProbe:
          httpGet:
            path: /ready
            port: 8080
          initialDelaySeconds: 5
          periodSeconds: 5
          timeoutSeconds: 2
          failureThreshold: 3
        # 启动探针（适用于启动较慢的应用）
        startupProbe:
          httpGet:
            path: /startup
            port: 8080
          initialDelaySeconds: 10
          periodSeconds: 10
          failureThreshold: 30  # 5分钟内必须成功
```

### Horizontal Pod Autoscaler (HPA)

```yaml
# hpa-configuration.yaml
apiVersion: autoscaling/v2
kind: HorizontalPodAutoscaler
metadata:
  name: app-hpa
spec:
  scaleTargetRef:
    apiVersion: apps/v1
    kind: Deployment
    name: optimized-app
  minReplicas: 2
  maxReplicas: 10
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
  # 自定义指标
  - type: Pods
    pods:
      metric:
        name: packets-per-second
      target:
        type: AverageValue
        averageValue: "1k"
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
```

### 集群级性能优化

```bash
# API Server 优化参数
--max-requests-inflight=400
--max-mutating-requests-inflight=200
--request-timeout=300s
--enable-priority-and-fairness=true

# Kubelet 优化参数
--kube-reserved=cpu=200m,memory=512Mi
--system-reserved=cpu=200m,memory=512Mi
--eviction-hard=memory.available<100Mi,nodefs.available<10%
--feature-gates=TopologyManager=true

# 调度器优化
--profiling=false
--bind-timeout-seconds=600
--percentage-of-nodes-to-score=50
```

### 监控与性能分析

```yaml
# performance-monitoring.yaml
---
apiVersion: monitoring.coreos.com/v1
kind: ServiceMonitor
metadata:
  name: app-metrics
  namespace: monitoring
spec:
  selector:
    matchLabels:
      app: optimized-app
  endpoints:
  - port: metrics
    interval: 30s
    path: /metrics
---
apiVersion: v1
kind: ConfigMap
metadata:
  name: prometheus-additional-config
  namespace: monitoring
data:
  additional.yaml: |
    - job_name: 'kubernetes-apps'
      scrape_interval: 30s
      kubernetes_sd_configs:
      - role: pod
      relabel_configs:
      - source_labels: [__meta_kubernetes_pod_annotation_prometheus_io_scrape]
        action: keep
        regex: true
      - source_labels: [__meta_kubernetes_pod_annotation_prometheus_io_path]
        action: replace
        target_label: __metrics_path__
        regex: (.+)
      - source_labels: [__address__, __meta_kubernetes_pod_annotation_prometheus_io_port]
        action: replace
        regex: ([^:]+)(?::\d+)?;(\d+)
        replacement: $1:$2
        target_label: __address__
```

### 性能调优检查清单

| 项目 | 检查项 | 最佳实践 |
|------|--------|----------|
| **资源分配** | Requests/Limits 设置 | 根据实际使用情况设置，避免过度分配 |
| **健康检查** | 探针配置 | 合理设置超时和重试参数 |
| **副本管理** | Pod 分布 | 使用 PodAntiAffinity 避免单点故障 |
| **存储性能** | 存储类选择 | 根据I/O需求选择合适的存储类 |
| **网络优化** | CNI 插件 | 选择高性能的 CNI 插件如 Cilium |
| **节点管理** | 节点资源 | 合理规划节点资源，预留系统资源 |

## 故障排除与诊断

### 常见故障诊断

```bash
# Pod 故障诊断
kubectl describe pod <pod-name> -n <namespace>
kubectl logs <pod-name> -n <namespace> --previous  # 查看崩溃前日志
kubectl exec -it <pod-name> -n <namespace> -- /bin/sh  # 进入容器调试

# 节点故障诊断
kubectl describe node <node-name>
kubectl get events --field-selector involvedObject.name=<node-name>
kubectl top nodes
kubectl top pods --all-namespaces

# 服务连接问题
kubectl get svc,ep -n <namespace>  # 检查服务和端点
kubectl run debug --image=nicolaka/netshoot -it --rm  # 网络调试工具
```

### 性能问题排查

```bash
# 资源争用检查
kubectl top nodes --sort-by=cpu
kubectl top pods --all-namespaces --sort-by=memory

# 调度问题检查
kubectl get events --sort-by='.lastTimestamp'
kubectl describe nodes | grep -A 5 -B 5 "Taints\|Conditions"

# 网络性能测试
kubectl run netperf --image=busybox --rm -it -- wget -qO- --timeout=3 <service-url>
```

### 生产环境运维脚本

```bash
#!/bin/bash
# k8s-health-check.sh - Kubernetes 集群健康检查脚本

check_cluster_status() {
    echo "=== 集群状态检查 ==="
    kubectl cluster-info
    kubectl get componentstatuses
    kubectl get nodes
}

check_system_pods() {
    echo "=== 系统 Pod 检查 ==="
    kubectl get pods -n kube-system
    kubectl get pods -n kube-system | grep -v Running | grep -v Completed
}

check_resource_usage() {
    echo "=== 资源使用情况 ==="
    kubectl top nodes
    kubectl top pods --all-namespaces | head -20
}

check_events() {
    echo "=== 最近事件 ==="
    kubectl get events --sort-by='.lastTimestamp' --field-selector type!=Normal | tail -20
}

# 执行检查
check_cluster_status
check_system_pods
check_resource_usage
check_events
```

## 总结

Kubernetes 运维是一个综合性的工作，需要掌握日志管理、备份恢复、安全加固和性能调优等多个方面的技能。运维人员应该根据实际生产环境的需求，制定相应的策略和流程，并建立完善的监控和告警机制，确保集群的稳定性和安全性。

---

**表格底部标记**: Kusheet Project, 作者 Allen Galler (allengaller@gmail.com)