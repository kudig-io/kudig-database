# Fluentd

> **成熟度**: Graduated | **加入时间**: 2016-11 | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官方网站** | https://www.fluentd.org |
| **GitHub** | https://github.com/fluent/fluentd |
| **文档** | https://docs.fluentd.org |
| **许可证** | Apache-2.0 |
| **主要语言** | Ruby, C |
| **CNCF 分类** | Observability |

---

## 项目概述

### 简介
Fluentd 是一个开源的统一日志层(Unified Logging Layer)，用于收集、处理、转换和转发日志数据到各种后端存储系统。它通过 JSON 统一日志格式，解耦了日志生产者和消费者。

### 发展历程
| 时间 | 里程碑 |
|:---|:---|
| 2011 | 由 Treasure Data 创建 |
| 2016-11 | 成为 CNCF 首批毕业项目之一 |
| 2019 | Fluent Bit 加入 CNCF |
| 至今 | 500+ 插件生态系统 |

### 核心定位
Fluentd 是云原生日志管理的基础组件，是 EFK (Elasticsearch-Fluentd-Kibana) 栈的核心，也是 Kubernetes 日志收集的事实标准。

---

## 架构设计

### 数据流架构

```
┌─────────────────────────────────────────────────────────────────┐
│                    Fluentd 数据流架构                            │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  日志源                                                          │
│  ┌────────┐ ┌────────┐ ┌────────┐ ┌────────┐                   │
│  │应用日志│ │系统日志│ │容器日志│ │ HTTP   │                   │
│  └───┬────┘ └───┬────┘ └───┬────┘ └───┬────┘                   │
│      │          │          │          │                         │
│      └──────────┴──────────┴──────────┘                         │
│                       │                                          │
│                       ▼                                          │
│  ┌─────────────────────────────────────────────────────────────┐│
│  │                      Fluentd                                 ││
│  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐         ││
│  │  │   Input     │  │   Filter    │  │   Output    │         ││
│  │  │  Plugins    │──►│  Plugins    │──►│  Plugins    │         ││
│  │  │             │  │             │  │             │         ││
│  │  │ • tail      │  │ • parser    │  │ • elasticsearch│       ││
│  │  │ • forward   │  │ • record    │  │ • kafka     │         ││
│  │  │ • http      │  │   transformer│  │ • s3        │         ││
│  │  │ • syslog    │  │ • grep      │  │ • stdout    │         ││
│  │  └─────────────┘  └─────────────┘  └─────────────┘         ││
│  │                          │                                   ││
│  │                          ▼                                   ││
│  │                   ┌─────────────┐                            ││
│  │                   │   Buffer    │                            ││
│  │                   │ (memory/file)│                           ││
│  │                   └─────────────┘                            ││
│  └─────────────────────────────────────────────────────────────┘│
│                       │                                          │
│      ┌────────────────┼────────────────┬───────────────┐        │
│      ▼                ▼                ▼               ▼        │
│  ┌────────┐    ┌────────┐    ┌────────┐    ┌────────┐         │
│  │Elastic │    │ Kafka  │    │  S3    │    │ ClickHouse│       │
│  │search  │    │        │    │        │    │          │         │
│  └────────┘    └────────┘    └────────┘    └────────┘         │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

### 插件类型

| 类型 | 功能 | 示例 |
|:---|:---|:---|
| **Input** | 数据输入 | tail, forward, http, syslog |
| **Parser** | 日志解析 | json, regexp, nginx, apache |
| **Filter** | 数据转换 | record_transformer, grep |
| **Output** | 数据输出 | elasticsearch, kafka, s3 |
| **Buffer** | 缓冲控制 | memory, file |
| **Formatter** | 格式化输出 | json, csv, single_value |

---

## 核心配置

### 基础配置结构

```xml
# /etc/fluentd/fluent.conf

# 系统配置
<system>
  log_level info
  workers 4
</system>

# 输入：收集容器日志
<source>
  @type tail
  @id container_logs
  path /var/log/containers/*.log
  pos_file /var/log/fluentd-containers.pos
  tag kubernetes.*
  read_from_head true
  <parse>
    @type json
    time_key time
    time_format %Y-%m-%dT%H:%M:%S.%NZ
  </parse>
</source>

# 输入：接收其他 Fluentd 转发的日志
<source>
  @type forward
  @id forward_input
  port 24224
  bind 0.0.0.0
</source>

# 过滤：添加 Kubernetes 元数据
<filter kubernetes.**>
  @type kubernetes_metadata
  @id filter_kubernetes_metadata
</filter>

# 过滤：解析应用日志
<filter kubernetes.var.log.containers.app-**>
  @type parser
  key_name log
  reserve_data true
  <parse>
    @type json
  </parse>
</filter>

# 输出：发送到 Elasticsearch
<match kubernetes.**>
  @type elasticsearch
  @id output_elasticsearch
  host elasticsearch.logging.svc.cluster.local
  port 9200
  logstash_format true
  logstash_prefix kubernetes
  include_timestamp true
  
  <buffer>
    @type file
    path /var/log/fluentd-buffers/kubernetes.buffer
    flush_mode interval
    flush_interval 5s
    retry_type exponential_backoff
    retry_max_interval 30s
    chunk_limit_size 8MB
    total_limit_size 512MB
  </buffer>
</match>
```

### 高级路由

```xml
# 多目标路由
<match app.**>
  @type copy
  
  # 发送到 Elasticsearch (全量)
  <store>
    @type elasticsearch
    host es-cluster
    logstash_format true
  </store>
  
  # 发送到 Kafka (特定标签)
  <store>
    @type kafka2
    brokers kafka:9092
    topic_key tag
    default_topic app-logs
  </store>
  
  # 发送到 S3 (归档)
  <store>
    @type s3
    s3_bucket logs-archive
    s3_region us-west-2
    path logs/%Y/%m/%d/
    <buffer time>
      @type file
      timekey 3600
      timekey_wait 10m
    </buffer>
  </store>
</match>

# 条件路由
<match **>
  @type rewrite_tag_filter
  <rule>
    key level
    pattern /error|fatal/
    tag alerts.${tag}
  </rule>
  <rule>
    key level
    pattern /.*/
    tag logs.${tag}
  </rule>
</match>

<match alerts.**>
  @type slack
  webhook_url https://hooks.slack.com/services/xxx
</match>
```

---

## Kubernetes 部署

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
          image: fluent/fluentd-kubernetes-daemonset:v1-debian-elasticsearch
          env:
            - name: FLUENT_ELASTICSEARCH_HOST
              value: "elasticsearch.logging.svc.cluster.local"
            - name: FLUENT_ELASTICSEARCH_PORT
              value: "9200"
          resources:
            limits:
              memory: 512Mi
            requests:
              cpu: 100m
              memory: 200Mi
          volumeMounts:
            - name: varlog
              mountPath: /var/log
            - name: containers
              mountPath: /var/log/containers
              readOnly: true
            - name: config
              mountPath: /fluentd/etc/
      volumes:
        - name: varlog
          hostPath:
            path: /var/log
        - name: containers
          hostPath:
            path: /var/log/containers
        - name: config
          configMap:
            name: fluentd-config
```

---

## Fluentd vs Fluent Bit

```
┌─────────────────────────────────────────────────────────────────┐
│                  Fluentd vs Fluent Bit                           │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  特性              Fluentd              Fluent Bit              │
│  ─────────────────────────────────────────────────────────────  │
│  语言              Ruby + C             C                       │
│  内存占用          ~40MB                ~650KB                  │
│  插件数量          500+                 80+                     │
│  适用场景          聚合器               边缘收集器              │
│  处理能力          中等                 高                      │
│  灵活性            高 (Ruby 脚本)       中等                    │
│  推荐用途          中心聚合             节点收集                │
│                                                                  │
│  典型架构:                                                       │
│  ┌────────┐    ┌────────┐    ┌────────┐    ┌────────┐          │
│  │Node 1  │    │Node 2  │    │Node 3  │    │Node N  │          │
│  │Fluent  │    │Fluent  │    │Fluent  │    │Fluent  │          │
│  │Bit     │    │Bit     │    │Bit     │    │Bit     │          │
│  └───┬────┘    └───┬────┘    └───┬────┘    └───┬────┘          │
│      │             │             │             │                │
│      └─────────────┴──────┬──────┴─────────────┘                │
│                           │                                      │
│                           ▼                                      │
│                    ┌─────────────┐                               │
│                    │  Fluentd    │  (聚合、处理、路由)           │
│                    │ Aggregator  │                               │
│                    └──────┬──────┘                               │
│                           │                                      │
│                           ▼                                      │
│                    ┌─────────────┐                               │
│                    │ Elasticsearch│                              │
│                    └─────────────┘                               │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

---

## 性能优化

### 缓冲配置

```xml
<buffer>
  @type file
  path /var/log/fluentd-buffers/
  
  # 分块配置
  chunk_limit_size 8MB
  total_limit_size 2GB
  
  # 刷新策略
  flush_mode interval
  flush_interval 5s
  flush_thread_count 2
  
  # 重试策略
  retry_type exponential_backoff
  retry_forever true
  retry_max_interval 30s
  
  # 溢出处理
  overflow_action drop_oldest_chunk
</buffer>
```

### 多 Worker 配置

```xml
<system>
  workers 4
  root_dir /var/log/fluentd
</system>

<worker 0-1>
  <source>
    @type tail
    path /var/log/app1/*.log
    tag app1
  </source>
</worker>

<worker 2-3>
  <source>
    @type tail
    path /var/log/app2/*.log
    tag app2
  </source>
</worker>
```

---

## 参考资源

- [官方文档](https://docs.fluentd.org)
- [GitHub Repo](https://github.com/fluent/fluentd)
- [CNCF 项目页面](https://www.cncf.io/projects/fluentd/)
- [插件列表](https://www.fluentd.org/plugins)
- [Fluent Bit](https://fluentbit.io/)

---

**维护者**: Kudig Team | **许可证**: MIT
