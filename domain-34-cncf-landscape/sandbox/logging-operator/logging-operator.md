# Logging Operator

> **成熟度**: Sandbox | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官网** | https://kube-logging.dev/ |
| **GitHub** | https://github.com/kube-logging/logging-operator |
| **许可证** | Apache-2.0 |
| **开发语言** | Go |
| **CNCF 状态** | Sandbox |

---

## 项目概述

Logging Operator 是一个 Kubernetes Operator，用于自动化部署和配置 Kubernetes 集群的日志收集管道。它基于 Fluentd 和 Fluent Bit 构建，通过 CRD 声明式地管理日志的收集、过滤、转换和路由，支持将日志发送到 Elasticsearch、Loki、S3、Kafka 等多种后端。

### 核心特性

- **声明式配置**: 通过 CRD 定义日志管道，完全声明式管理
- **Fluent Bit + Fluentd**: Fluent Bit 做轻量级收集，Fluentd 做聚合和路由
- **多租户支持**: 按 Namespace 隔离日志管道，支持多租户场景
- **丰富的输出**: Elasticsearch, Loki, S3, Kafka, CloudWatch, Splunk 等
- **日志过滤**: 正则匹配、JSON 解析、字段修改、敏感信息脱敏
- **TLS 加密**: 组件间通信支持 mTLS 加密
- **SyslogNG 支持**: 除 Fluentd 外还支持 SyslogNG 作为聚合器

---

## 架构设计

```
┌────────────────────────────────────────────────────┐
│                 Kubernetes Cluster                   │
│                                                      │
│  ┌──────────┐ ┌──────────┐ ┌──────────┐            │
│  │  App Pod  │ │  App Pod  │ │  App Pod  │  ...      │
│  │  stdout/  │ │  stdout/  │ │  stdout/  │           │
│  │  stderr   │ │  stderr   │ │  stderr   │           │
│  └─────┬─────┘ └─────┬─────┘ └─────┬─────┘          │
│        │              │              │                │
│  ┌─────┴──────────────┴──────────────┴──────────┐   │
│  │          Fluent Bit (DaemonSet)               │   │
│  │  - 节点级日志收集                              │   │
│  │  - 容器日志解析                                │   │
│  │  - Kubernetes metadata 注入                   │   │
│  └──────────────────────┬───────────────────────┘   │
│                         │ (Forward Protocol)         │
│  ┌──────────────────────┴───────────────────────┐   │
│  │         Fluentd (StatefulSet)                 │   │
│  │  - 日志聚合和缓冲                              │   │
│  │  - 过滤和转换                                  │   │
│  │  - 路由到多个输出                              │   │
│  └─────┬──────────┬──────────┬─────────────────┘   │
│        │          │          │                       │
└────────┼──────────┼──────────┼───────────────────────┘
         ▼          ▼          ▼
   ┌──────────┐ ┌───────┐ ┌────────┐
   │Elastic-  │ │ Loki  │ │  S3    │
   │search    │ │       │ │        │
   └──────────┘ └───────┘ └────────┘
```

### CRD 模型

| CRD | 说明 |
|:---|:---|
| **Logging** | 全局配置，定义 Fluent Bit 和 Fluentd 的部署参数 |
| **Flow** | Namespace 级别的日志路由规则（过滤 + 输出） |
| **ClusterFlow** | 集群级别的日志路由规则 |
| **Output** | Namespace 级别的日志输出目标定义 |
| **ClusterOutput** | 集群级别的日志输出目标定义 |
| **SyslogNGConfig** | SyslogNG 模式的全局配置 |

---

## 快速开始

### 安装 Logging Operator

```bash
# 使用 Helm 安装
helm repo add kube-logging https://kube-logging.github.io/helm-charts
helm repo update

helm install logging-operator kube-logging/logging-operator \
  --namespace logging \
  --create-namespace
```

### 创建 Logging 资源

```yaml
apiVersion: logging.banzaicloud.io/v1beta1
kind: Logging
metadata:
  name: default-logging
spec:
  controlNamespace: logging
  fluentbit:
    metrics:
      serviceMonitor: true
    tolerations:
      - operator: Exists
    filterKubernetes:
      kube_tag_prefix: "kubernetes.var.log.containers."
      merge_log: "true"
      merge_log_key: "log"
  fluentd:
    metrics:
      serviceMonitor: true
    scaling:
      replicas: 2
    bufferStorageVolume:
      pvc:
        spec:
          accessModes:
            - ReadWriteOnce
          resources:
            requests:
              storage: 10Gi
```

---

## 配置详解

### 输出到 Elasticsearch

```yaml
apiVersion: logging.banzaicloud.io/v1beta1
kind: ClusterOutput
metadata:
  name: es-output
  namespace: logging
spec:
  elasticsearch:
    host: elasticsearch.elastic-system.svc.cluster.local
    port: 9200
    scheme: https
    ssl_verify: true
    ssl_version: TLSv1_2
    user: elastic
    password:
      valueFrom:
        secretKeyRef:
          name: es-credentials
          key: password
    logstash_format: true
    logstash_prefix: "k8s-logs"
    buffer:
      type: file
      path: /buffers/es
      chunk_limit_size: 8MB
      total_limit_size: 2GB
      flush_interval: 5s
      retry_max_interval: 30
      retry_forever: true
```

### 输出到 Loki

```yaml
apiVersion: logging.banzaicloud.io/v1beta1
kind: ClusterOutput
metadata:
  name: loki-output
  namespace: logging
spec:
  loki:
    url: http://loki-gateway.loki.svc.cluster.local
    labels:
      app: ""
      namespace: ""
      node_name: ""
    extra_labels:
      cluster: production
    buffer:
      type: file
      path: /buffers/loki
      chunk_limit_size: 1MB
      total_limit_size: 1GB
      flush_interval: 3s
```

### 输出到 S3

```yaml
apiVersion: logging.banzaicloud.io/v1beta1
kind: ClusterOutput
metadata:
  name: s3-archive
  namespace: logging
spec:
  s3:
    s3_bucket: my-logs-bucket
    s3_region: us-east-1
    path: "logs/${tag}/%Y/%m/%d/"
    aws_key_id:
      valueFrom:
        secretKeyRef:
          name: aws-credentials
          key: access-key
    aws_sec_key:
      valueFrom:
        secretKeyRef:
          name: aws-credentials
          key: secret-key
    store_as: gzip
    buffer:
      type: file
      path: /buffers/s3
      timekey: 3600
      timekey_wait: 60
      chunk_limit_size: 256MB
```

### Flow - 日志路由和过滤

```yaml
apiVersion: logging.banzaicloud.io/v1beta1
kind: Flow
metadata:
  name: app-logs
  namespace: production
spec:
  match:
    - select:
        labels:
          app: my-application
        container_names:
          - app
          - sidecar
    - exclude:
        labels:
          log-exclude: "true"
  filters:
    - parser:
        key_name: log
        parse:
          type: json
          time_key: timestamp
          time_format: "%Y-%m-%dT%H:%M:%S.%NZ"
    - record_modifier:
        records:
          - cluster_name: "production-east"
            environment: "production"
    - grep:
        exclude:
          - key: level
            pattern: /debug/
    - dedot:
        de_dot_separator: "_"
        de_dot_nested: true
  localOutputRefs:
    - es-output
  globalOutputRefs:
    - s3-archive
---
# ClusterFlow - 全局审计日志
apiVersion: logging.banzaicloud.io/v1beta1
kind: ClusterFlow
metadata:
  name: audit-logs
  namespace: logging
spec:
  match:
    - select:
        namespaces:
          - kube-system
        labels:
          component: kube-apiserver
  filters:
    - tag_normaliser: {}
    - record_modifier:
        records:
          - log_type: "audit"
  globalOutputRefs:
    - s3-archive
```

---

## 高级功能

### 多租户日志隔离

```yaml
# 团队 A 的日志管道
apiVersion: logging.banzaicloud.io/v1beta1
kind: Output
metadata:
  name: team-a-output
  namespace: team-a
spec:
  elasticsearch:
    host: es.example.com
    port: 9200
    index_name: team-a-logs
---
apiVersion: logging.banzaicloud.io/v1beta1
kind: Flow
metadata:
  name: team-a-flow
  namespace: team-a
spec:
  match:
    - select: {}  # team-a namespace 下的所有日志
  localOutputRefs:
    - team-a-output
```

### 敏感信息脱敏

```yaml
filters:
  - record_transformer:
      enable_ruby: true
      records:
        - message: ${record["message"].gsub(/\b\d{4}[-\s]?\d{4}[-\s]?\d{4}[-\s]?\d{4}\b/, '****-****-****-****')}
        - message: ${record["message"].gsub(/password[=:]\S+/, 'password=***')}
```

### SyslogNG 模式

```yaml
apiVersion: logging.banzaicloud.io/v1beta1
kind: Logging
metadata:
  name: syslogng-logging
spec:
  controlNamespace: logging
  syslogNG:
    metrics:
      serviceMonitor: true
  fluentbit: {}
---
apiVersion: logging.banzaicloud.io/v1beta1
kind: SyslogNGClusterOutput
metadata:
  name: syslog-output
  namespace: logging
spec:
  elasticsearch:
    url: "https://es.example.com:9200"
    index: "syslog-${YEAR}.${MONTH}.${DAY}"
```

---

## 监控

### Prometheus 指标

| 指标 | 说明 |
|:---|:---|
| `fluentbit_input_records_total` | Fluent Bit 输入记录总数 |
| `fluentbit_output_retries_total` | 输出重试次数 |
| `fluentd_output_status_buffer_total_bytes` | Fluentd 缓冲区使用量 |
| `fluentd_output_status_emit_records` | Fluentd 输出记录数 |

---

## 最佳实践

1. **缓冲配置**: 生产环境使用 PVC 持久化缓冲区，防止数据丢失
2. **资源限制**: 为 Fluent Bit 和 Fluentd 设置合理的 CPU/内存限制
3. **日志分级**: 使用 Flow 过滤掉 debug 级别日志减少存储开销
4. **多输出**: 热数据发往 Elasticsearch/Loki，冷数据归档到 S3
5. **多租户**: 利用 Flow/Output 的 Namespace 隔离实现多租户日志管理
6. **监控缓冲**: 关注缓冲区使用率，避免因输出目标不可用导致缓冲溢出

---

## 参考资源

- [Logging Operator 官方文档](https://kube-logging.dev/docs/)
- [Logging Operator GitHub](https://github.com/kube-logging/logging-operator)
- [Fluentd 文档](https://docs.fluentd.org/)
- [CNCF Sandbox Projects](https://www.cncf.io/sandbox-projects/)

---

**维护者**: Kudig Team | **许可证**: MIT
