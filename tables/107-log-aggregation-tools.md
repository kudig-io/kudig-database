# 日志聚合工具

> **适用版本**: v1.25 - v1.32 | **最后更新**: 2026-01 | **参考**: [Loki](https://grafana.com/oss/loki/) | [Fluentd](https://www.fluentd.org/)

## 工具对比

| 工具 | 架构 | 索引模式 | 存储成本 | 查询性能 | K8s集成 | 学习曲线 | 生产推荐 |
|------|------|---------|---------|---------|---------|---------|---------|
| **Loki** | 分布式 | 标签索引 | ⭐⭐⭐⭐⭐ | ⭐⭐⭐⭐ | ⭐⭐⭐⭐⭐ | 低 | 强烈推荐 |
| **Fluentd** | 采集代理 | N/A | N/A | N/A | ⭐⭐⭐⭐⭐ | 中 | 采集端推荐 |
| **Vector** | 采集代理 | N/A | N/A | N/A | ⭐⭐⭐⭐ | 中 | 高性能场景 |
| **Filebeat** | 采集代理 | N/A | N/A | N/A | ⭐⭐⭐⭐ | 低 | ELK栈 |
| **Elasticsearch** | 分布式 | 全文索引 | ⭐⭐ | ⭐⭐⭐⭐⭐ | ⭐⭐⭐⭐ | 高 | 复杂查询 |
| **OpenSearch** | 分布式 | 全文索引 | ⭐⭐ | ⭐⭐⭐⭐⭐ | ⭐⭐⭐⭐ | 高 | ES替代 |

---

## Loki 生产部署

### 架构模式

```
┌────────────────────────────────────────┐
│         Kubernetes Cluster             │
│  ┌────────┐  ┌────────┐  ┌────────┐   │
│  │  Pod   │  │  Pod   │  │  Pod   │   │
│  │ stdout │  │ /logs  │  │ syslog │   │
│  └───┬────┘  └────┬───┘  └────┬───┘   │
│      │            │            │       │
│      └────────────┴────────────┘       │
│                   │                    │
│          ┌────────▼────────┐           │
│          │   Promtail      │           │
│          │  (DaemonSet)    │           │
│          └────────┬────────┘           │
└───────────────────┼────────────────────┘
                    │ push
                    v
┌─────────────────────────────────────────┐
│          Loki Cluster                   │
│  ┌──────────┐  ┌──────────┐  ┌───────┐ │
│  │Distributor│  │ Ingester │  │ Querier│ │
│  └─────┬────┘  └─────┬────┘  └───┬───┘ │
│        │             │            │     │
│        └─────────────┴────────────┘     │
│                      │                  │
└──────────────────────┼──────────────────┘
                       │
                       v
            ┌──────────────────┐
            │ Object Storage   │
            │  (S3/OSS/GCS)    │
            └──────────────────┘
```

### Helm 安装 Loki

```bash
# 单体模式(测试/小规模)
helm install loki grafana/loki \
  -n monitoring --create-namespace \
  --set loki.auth_enabled=false \
  --set loki.storage.type=filesystem

# 分布式模式(生产)
helm install loki grafana/loki-distributed \
  -n monitoring \
  --set loki.storage.type=s3 \
  --set loki.storage.s3.endpoint=oss-cn-hangzhou.aliyuncs.com \
  --set loki.storage.s3.region=cn-hangzhou \
  --set loki.storage.bucketNames.chunks=loki-chunks \
  --set loki.storage.bucketNames.ruler=loki-ruler \
  --set loki.storage.bucketNames.admin=loki-admin \
  --set ingester.replicas=3 \
  --set querier.replicas=3 \
  --set distributor.replicas=3 \
  --set queryFrontend.replicas=2
```

### Loki 配置详解

```yaml
apiVersion: v1
kind: ConfigMap
metadata:
  name: loki-config
  namespace: monitoring
data:
  loki.yaml: |
    auth_enabled: false
    
    server:
      http_listen_port: 3100
      grpc_listen_port: 9096
    
    # 分布式模式
    memberlist:
      join_members:
        - loki-memberlist:7946
    
    # 存储配置
    common:
      path_prefix: /loki
      storage:
        s3:
          endpoint: oss-cn-hangzhou.aliyuncs.com
          bucketnames: loki-chunks
          access_key_id: ${S3_ACCESS_KEY}
          secret_access_key: ${S3_SECRET_KEY}
          s3forcepathstyle: true
      ring:
        kvstore:
          store: memberlist
    
    # Schema配置
    schema_config:
      configs:
        - from: 2024-01-01
          store: tsdb  # v2.9+推荐
          object_store: s3
          schema: v12
          index:
            prefix: loki_index_
            period: 24h
    
    # 数据保留
    limits_config:
      retention_period: 30d  # 保留30天
      ingestion_rate_mb: 10  # 每租户10MB/s
      ingestion_burst_size_mb: 20
      max_query_length: 721h  # 30天查询范围
      max_query_parallelism: 32
      max_streams_per_user: 10000
      max_line_size: 256KB
    
    # 压缩配置
    compactor:
      working_directory: /loki/compactor
      shared_store: s3
      compaction_interval: 10m
      retention_enabled: true
      retention_delete_delay: 2h
      retention_delete_worker_count: 150
    
    # 查询优化
    query_range:
      align_queries_with_step: true
      cache_results: true
      results_cache:
        cache:
          embedded_cache:
            enabled: true
            max_size_mb: 500
```

---

## Promtail 日志采集

### DaemonSet 部署

```yaml
apiVersion: apps/v1
kind: DaemonSet
metadata:
  name: promtail
  namespace: monitoring
spec:
  selector:
    matchLabels:
      app: promtail
  template:
    metadata:
      labels:
        app: promtail
    spec:
      serviceAccountName: promtail
      containers:
        - name: promtail
          image: grafana/promtail:2.9.3
          args:
            - -config.file=/etc/promtail/promtail.yaml
          volumeMounts:
            - name: config
              mountPath: /etc/promtail
            - name: varlog
              mountPath: /var/log
              readOnly: true
            - name: varlibdockercontainers
              mountPath: /var/lib/docker/containers
              readOnly: true
            - name: positions
              mountPath: /tmp/positions
          env:
            - name: HOSTNAME
              valueFrom:
                fieldRef:
                  fieldPath: spec.nodeName
          resources:
            requests:
              cpu: 100m
              memory: 128Mi
            limits:
              cpu: 200m
              memory: 256Mi
      volumes:
        - name: config
          configMap:
            name: promtail-config
        - name: varlog
          hostPath:
            path: /var/log
        - name: varlibdockercontainers
          hostPath:
            path: /var/lib/docker/containers
        - name: positions
          hostPath:
            path: /tmp/promtail-positions
---
apiVersion: v1
kind: ServiceAccount
metadata:
  name: promtail
  namespace: monitoring
---
apiVersion: rbac.authorization.k8s.io/v1
kind: ClusterRole
metadata:
  name: promtail
rules:
  - apiGroups: [""]
    resources: ["nodes", "pods"]
    verbs: ["get", "list", "watch"]
---
apiVersion: rbac.authorization.k8s.io/v1
kind: ClusterRoleBinding
metadata:
  name: promtail
roleRef:
  apiGroup: rbac.authorization.k8s.io
  kind: ClusterRole
  name: promtail
subjects:
  - kind: ServiceAccount
    name: promtail
    namespace: monitoring
```

### Promtail 配置详解

```yaml
apiVersion: v1
kind: ConfigMap
metadata:
  name: promtail-config
  namespace: monitoring
data:
  promtail.yaml: |
    server:
      http_listen_port: 3101
      grpc_listen_port: 0
    
    positions:
      filename: /tmp/positions/positions.yaml
    
    clients:
      - url: http://loki-gateway/loki/api/v1/push
        batchwait: 1s
        batchsize: 1048576  # 1MB
    
    scrape_configs:
      # Kubernetes容器日志
      - job_name: kubernetes-pods
        kubernetes_sd_configs:
          - role: pod
        
        # 标签提取
        relabel_configs:
          # 仅采集有日志注解的Pod
          - source_labels: [__meta_kubernetes_pod_annotation_prometheus_io_scrape]
            action: keep
            regex: true
          
          # 提取命名空间
          - source_labels: [__meta_kubernetes_namespace]
            target_label: namespace
          
          # 提取Pod名称
          - source_labels: [__meta_kubernetes_pod_name]
            target_label: pod
          
          # 提取容器名称
          - source_labels: [__meta_kubernetes_pod_container_name]
            target_label: container
          
          # 提取应用标签
          - source_labels: [__meta_kubernetes_pod_label_app]
            target_label: app
          
          # 提取环境标签
          - source_labels: [__meta_kubernetes_pod_label_environment]
            target_label: environment
          
          # 日志路径
          - source_labels: [__meta_kubernetes_pod_uid, __meta_kubernetes_pod_container_name]
            target_label: __path__
            replacement: /var/log/pods/*$1*/*$2/*.log
        
        # 日志处理管道
        pipeline_stages:
          # 1. CRI日志格式解析
          - cri: {}
          
          # 2. JSON日志解析
          - json:
              expressions:
                level: level
                message: msg
                timestamp: time
                trace_id: trace_id
                span_id: span_id
          
          # 3. 时间戳解析
          - timestamp:
              source: timestamp
              format: RFC3339Nano
          
          # 4. 提取标签
          - labels:
              level:
              trace_id:
          
          # 5. 多行日志聚合(Java堆栈)
          - multiline:
              firstline: '^\d{4}-\d{2}-\d{2}'
              max_wait_time: 3s
          
          # 6. 正则提取
          - regex:
              expression: '^(?P<level>\w+)\s+\[(?P<thread>[\w-]+)\]'
          
          # 7. 过滤低价值日志
          - match:
              selector: '{app="myapp"} |= "debug"'
              action: drop
          
          # 8. 脱敏处理
          - replace:
              expression: '(password|token)=[^\s]+'
              replace: '$1=***REDACTED***'
      
      # 系统日志
      - job_name: system-logs
        static_configs:
          - targets: [localhost]
            labels:
              job: system-logs
              __path__: /var/log/*.log
```

---

## Fluentd 企业实践

### DaemonSet 部署

```yaml
apiVersion: apps/v1
kind: DaemonSet
metadata:
  name: fluentd
  namespace: logging
spec:
  selector:
    matchLabels:
      app: fluentd
  template:
    metadata:
      labels:
        app: fluentd
    spec:
      serviceAccountName: fluentd
      tolerations:
        - key: node-role.kubernetes.io/master
          effect: NoSchedule
      containers:
        - name: fluentd
          image: fluent/fluentd-kubernetes-daemonset:v1.16-debian-elasticsearch7-1
          env:
            - name: FLUENT_ELASTICSEARCH_HOST
              value: "elasticsearch.logging.svc.cluster.local"
            - name: FLUENT_ELASTICSEARCH_PORT
              value: "9200"
            - name: FLUENT_ELASTICSEARCH_SCHEME
              value: "http"
            - name: FLUENT_UID
              value: "0"  # 需要读取日志文件
          volumeMounts:
            - name: varlog
              mountPath: /var/log
            - name: varlibdockercontainers
              mountPath: /var/lib/docker/containers
              readOnly: true
            - name: fluentd-config
              mountPath: /fluentd/etc
          resources:
            requests:
              cpu: 200m
              memory: 256Mi
            limits:
              cpu: 500m
              memory: 512Mi
      volumes:
        - name: varlog
          hostPath:
            path: /var/log
        - name: varlibdockercontainers
          hostPath:
            path: /var/lib/docker/containers
        - name: fluentd-config
          configMap:
            name: fluentd-config
```

### Fluentd 配置

```xml
<source>
  @type tail
  @id in_tail_container_logs
  path /var/log/containers/*.log
  pos_file /var/log/fluentd-containers.log.pos
  tag kubernetes.*
  read_from_head true
  <parse>
    @type json
    time_format %Y-%m-%dT%H:%M:%S.%NZ
  </parse>
</source>

# Kubernetes元数据过滤
<filter kubernetes.**>
  @type kubernetes_metadata
  @id filter_kube_metadata
  kubernetes_url "#{ENV['FLUENT_FILTER_KUBERNETES_URL'] || 'https://' + ENV.fetch('KUBERNETES_SERVICE_HOST') + ':' + ENV.fetch('KUBERNETES_SERVICE_PORT') + '/api'}"
  verify_ssl "#{ENV['KUBERNETES_VERIFY_SSL'] || true}"
  ca_file "#{ENV['KUBERNETES_CA_FILE']}"
  skip_labels "#{ENV['FLUENT_KUBERNETES_METADATA_SKIP_LABELS'] || 'false'}"
  skip_container_metadata "#{ENV['FLUENT_KUBERNETES_METADATA_SKIP_CONTAINER_METADATA'] || 'false'}"
  skip_master_url "#{ENV['FLUENT_KUBERNETES_METADATA_SKIP_MASTER_URL'] || 'false'}"
  skip_namespace_metadata "#{ENV['FLUENT_KUBERNETES_METADATA_SKIP_NAMESPACE_METADATA'] || 'false'}"
</filter>

# 日志解析
<filter kubernetes.**>
  @type parser
  key_name log
  reserve_data true
  remove_key_name_field true
  <parse>
    @type multi_format
    # JSON格式
    <pattern>
      format json
      time_key time
      time_format %Y-%m-%dT%H:%M:%S.%NZ
    </pattern>
    # Nginx格式
    <pattern>
      format /^(?<remote>[^ ]*) - (?<user>[^ ]*) \[(?<time>[^\]]*)\] "(?<method>\S+)(?: +(?<path>[^\"]*?)(?: +\S*)?)?" (?<code>[^ ]*) (?<size>[^ ]*)/
      time_format %d/%b/%Y:%H:%M:%S %z
    </pattern>
  </parse>
</filter>

# 数据脱敏
<filter kubernetes.**>
  @type record_transformer
  enable_ruby true
  <record>
    log ${record["log"].gsub(/password=\S+/, "password=***")}
  </record>
</filter>

# 输出到Elasticsearch
<match kubernetes.**>
  @type elasticsearch
  @id out_es
  host "#{ENV['FLUENT_ELASTICSEARCH_HOST']}"
  port "#{ENV['FLUENT_ELASTICSEARCH_PORT']}"
  scheme "#{ENV['FLUENT_ELASTICSEARCH_SCHEME'] || 'http'}"
  ssl_verify "#{ENV['FLUENT_ELASTICSEARCH_SSL_VERIFY'] || 'true'}"
  
  logstash_format true
  logstash_prefix kubernetes
  logstash_dateformat %Y.%m.%d
  
  include_tag_key true
  type_name _doc
  
  <buffer>
    @type file
    path /var/log/fluentd-buffers/kubernetes.system.buffer
    flush_mode interval
    retry_type exponential_backoff
    flush_thread_count 2
    flush_interval 5s
    retry_forever false
    retry_max_interval 30
    chunk_limit_size 2M
    queue_limit_length 8
    overflow_action block
  </buffer>
</match>

# 输出到Loki
<match kubernetes.**>
  @type loki
  url "http://loki-gateway/loki/api/v1/push"
  tenant ""
  
  <label>
    namespace
    pod
    container
    app
  </label>
  
  <buffer>
    flush_interval 10s
    flush_at_shutdown true
  </buffer>
</match>
```

---

## Vector 高性能采集

### 配置示例

```toml
# vector.toml
[sources.kubernetes_logs]
type = "kubernetes_logs"

[transforms.parse_json]
type = "remap"
inputs = ["kubernetes_logs"]
source = '''
  . = parse_json!(.message)
  .level = downcase!(.level)
'''

[transforms.filter_errors]
type = "filter"
inputs = ["parse_json"]
condition = '.level == "error" || .level == "warn"'

[sinks.loki]
type = "loki"
inputs = ["filter_errors"]
endpoint = "http://loki-gateway"
encoding.codec = "json"
labels.namespace = "{{ kubernetes.namespace_name }}"
labels.pod = "{{ kubernetes.pod_name }}"
labels.container = "{{ kubernetes.container_name }}"

[sinks.prometheus]
type = "prometheus_exporter"
inputs = ["parse_json"]
address = "0.0.0.0:9090"
```

---

## LogQL 查询实战

### 基础查询

```logql
# 1. 按标签过滤
{namespace="production", app="myapp"}

# 2. 正则匹配
{app="myapp"} |~ "error|exception"

# 3. 排除日志
{app="myapp"} != "healthcheck"

# 4. JSON字段过滤
{app="myapp"} | json | level="ERROR"

# 5. 行过滤+JSON解析
{app="myapp"} |= "database" | json | query_time > 1000
```

### 聚合查询

```logql
# 每分钟错误日志数量
rate({app="myapp"} |= "error" [1m])

# 按级别统计
sum by (level) (rate({app="myapp"} | json [5m]))

# P95响应时间
quantile_over_time(0.95,
  {app="myapp"} | json | unwrap response_time [5m]
)

# 错误率
sum(rate({app="myapp", level="error"} [5m]))
/
sum(rate({app="myapp"} [5m]))
```

---

## 性能优化建议

| 优化项 | 方法 | 效果 |
|-------|------|------|
| **采集过滤** | Promtail pipeline_stages过滤 | 减少50%无用日志 |
| **标签优化** | 限制高基数标签 | 降低索引开销 |
| **批量推送** | 增大batchsize | 减少网络开销 |
| **压缩传输** | 启用gzip | 节省70%带宽 |
| **查询优化** | 使用时间范围限制 | 提升查询速度 |

---

## 常见问题

**Q: Loki vs Elasticsearch如何选择?**  
A: Loki成本低(不索引内容)、K8s原生，适合日志查询；ES全文检索强，适合复杂查询。

**Q: 如何处理多行日志?**  
A: Promtail使用multiline stage，Fluentd使用multiline parser。

**Q: 日志丢失如何排查?**  
A: 检查采集器资源限制、网络连接、Buffer配置、目标存储容量。
