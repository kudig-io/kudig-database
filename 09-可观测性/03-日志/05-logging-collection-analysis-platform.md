---
title: 05-日志收集分析平台
description: '# 05-日志收集分析平台'
summary: '完整的日志收集分析平台是可观测性体系的重要组成部分。本文档详细介绍基于ELK/EFK技术栈的企业级日志解决方案。'
category: production-operations
tags:
- k8s
- production
- operations
- best-practices
- prometheus
- docker
- opa
- redis
- kafka
- elasticsearch
tier: supporting
created: '2026-05-23'
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 平台工程师
estimated_read_time: 5min
intent_queries:
- 日志收集分析平台 是什么
- 如何 日志收集分析平台
- Kubernetes 18 production operations 最佳实践
trigger_keywords:
- 日志收集分析平台
- production
- operations
prerequisites:
- kubectl-basics
- observability-basics
- prometheus-basics
- kafka-basics
- redis-basics
- policy-basics
- logging-basics
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
authors:
- name: KUDIG Team
  role: contributor
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 05-日志收集分析平台

> **适用范围**: [[Kubernetes|Kubernetes]] v1.25-v1.32 | **维护状态**: 🔧 持续更新中 | **专家级别**: ⭐⭐⭐⭐⭐

<!-- chunk: 📋 概述 -->## 📋 概述

完整的日志收集分析平台是可观测性体系的重要组成部分。本文档详细介绍基于ELK/EFK技术栈的企业级日志解决方案。

<!-- chunk: 🏗️ 日志架构设计 -->## 🏗️ 日志架构设计

## 分层日志架构

## 1. 日志收集层
```yaml
# Fluent Bit DaemonSet配置
apiVersion: apps/v1
kind: DaemonSet
metadata:
  name: fluent-bit
  namespace: logging
spec:
  selector:
    matchLabels:
      app: fluent-bit
  template:
    metadata:
      labels:
        app: fluent-bit
    spec:
      serviceAccountName: fluent-bit
      containers:
      - name: fluent-bit
        image: fluent/fluent-bit:2.0
        ports:
        - containerPort: 2020
        env:
        - name: FLUENT_ELASTICSEARCH_HOST
          value: "elasticsearch.logging.svc"
        - name: FLUENT_ELASTICSEARCH_PORT
          value: "9200"
        volumeMounts:
        - name: varlog
          mountPath: /var/log
        - name: varlibdockercontainers
          mountPath: /var/lib/docker/containers
          readOnly: true
        - name: fluent-bit-config
          mountPath: /fluent-bit/etc/
        resources:
          limits:
            memory: 100Mi
          requests:
            cpu: 100m
            memory: 100Mi
      volumes:
      - name: varlog
        hostPath:
          path: /var/log
      - name: varlibdockercontainers
        hostPath:
          path: /var/lib/docker/containers
      - name: fluent-bit-config
        configMap:
          name: fluent-bit-config
---
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
        Tag               kube.*
        Path              /var/log/containers/*.log
        Parser            docker
        DB                /var/log/flb_kube.db
        Mem_Buf_Limit     5MB
        Skip_Long_Lines   On
        Refresh_Interval  10

    [FILTER]
        Name                kubernetes
        Match               kube.*
        Kube_URL            https://kubernetes.default.svc:443
        Kube_CA_File        /var/run/secrets/kubernetes.io/serviceaccount/ca.crt
        Kube_Token_File     /var/run/secrets/kubernetes.io/serviceaccount/token
        K8S-Logging.Parser  On
        K8S-Logging.Exclude Off

    [OUTPUT]
        Name            es
        Match           kube.*
        Host            ${FLUENT_ELASTICSEARCH_HOST}
        Port            ${FLUENT_ELASTICSEARCH_PORT}
        Logstash_Format On
        Replace_Dots    On
        Retry_Limit     False
```

## 2. 日志缓冲层
```yaml
# Kafka作为日志缓冲
apiVersion: apps/v1
kind: StatefulSet
metadata:
  name: kafka
  namespace: logging
spec:
  serviceName: kafka-headless
  replicas: 3
  selector:
    matchLabels:
      app: kafka
  template:
    metadata:
      labels:
        app: kafka
    spec:
      containers:
      - name: kafka
        image: confluentinc/cp-kafka:7.3.0
        env:
        - name: KAFKA_BROKER_ID
          valueFrom:
            fieldRef:
              fieldPath: metadata.name
        - name: KAFKA_ZOOKEEPER_CONNECT
          value: "zookeeper:2181"
        - name: KAFKA_ADVERTISED_LISTENERS
          value: "PLAINTEXT://kafka-$(KAFKA_BROKER_ID).kafka-headless:9092"
        - name: KAFKA_OFFSETS_TOPIC_REPLICATION_FACTOR
          value: "3"
        - name: KAFKA_TRANSACTION_STATE_LOG_REPLICATION_FACTOR
          value: "3"
        ports:
        - containerPort: 9092
        volumeMounts:
        - name: data
          mountPath: /var/lib/kafka
  volumeClaimTemplates:
  - metadata:
      name: data
    spec:
      accessModes: ["ReadWriteOnce"]
      resources:
        requests:
          storage: 100Gi
```

## 3. 日志存储层
```yaml
# Elasticsearch集群配置
apiVersion: elasticsearch.k8s.elastic.co/v1
kind: Elasticsearch
metadata:
  name: elasticsearch
  namespace: logging
spec:
  version: 8.5.3
  nodeSets:
  - name: default
    count: 3
    config:
      node.store.allow_mmap: false
    volumeClaimTemplates:
    - metadata:
        name: elasticsearch-data
      spec:
        accessModes:
        - ReadWriteOnce
        resources:
          requests:
            storage: 100Gi
    podTemplate:
      spec:
        containers:
        - name: elasticsearch
          env:
          - name: ES_JAVA_OPTS
            value: -Xms2g -Xmx2g
          resources:
            requests:
              memory: 2Gi
              cpu: 1
            limits:
              memory: 4Gi
              cpu: 2
```

<!-- chunk: 🔍 日志分析平台 -->## 🔍 日志分析平台

## Kibana配置

## 1. Kibana部署配置
```yaml
# Kibana配置
apiVersion: kibana.k8s.elastic.co/v1
kind: Kibana
metadata:
  name: kibana
  namespace: logging
spec:
  version: 8.5.3
  count: 1
  elasticsearchRef:
    name: elasticsearch
  config:
    server.publicBaseUrl: "https://kibana.example.com"
    xpack.security.encryptionKey: "something_at_least_32_characters"
    xpack.security.session.idleTimeout: "1h"
    xpack.security.session.lifespan: "30d"
  http:
    tls:
      selfSignedCertificate:
        disabled: true
```

## 2. 日志索引模板
```json
{
  "index_patterns": ["kubernetes-*"],
  "template": {
    "settings": {
      "number_of_shards": 3,
      "number_of_replicas": 1,
      "refresh_interval": "30s",
      "blocks": {
        "read_only_allow_delete": "false"
      }
    },
    "mappings": {
      "properties": {
        "@timestamp": { "type": "date" },
        "log.level": { "type": "keyword" },
        "message": { "type": "text" },
        "kubernetes": {
          "properties": {
            "pod": { "type": "keyword" },
            "namespace": { "type": "keyword" },
            "container": { "type": "keyword" },
            "node": { "type": "keyword" }
          }
        },
        "host": {
          "properties": {
            "name": { "type": "keyword" }
          }
        }
      }
    }
  }
}
```

## 日志解析优化

## 1. 结构化日志处理
```yaml
# 日志解析配置
parsers.conf: |
  [PARSER]
      Name   json
      Format json
      Time_Key time
      Time_Format %d/%b/%Y:%H:%M:%S %z

  [PARSER]
      Name   docker
      Format json
      Time_Key time
      Time_Format %Y-%m-%dT%H:%M:%S.%L
      Time_Keep   On

  [PARSER]
      Name   syslog
      Format regex
      Regex ^\<(?<pri>[0-9]+)\>(?<time>[^ ]* {1,2}[^ ]* [^ ]*) (?<host>[^ ]*) (?<ident>[a-zA-Z0-9_\/\.\-]*)(?:\[(?<pid>[0-9]+)\])?(?:[^\:]*\:)? *(?<message>.*)$
      Time_Key time
      Time_Format %b %d %H:%M:%S
```

## 2. 多格式日志适配
```yaml
# 多格式日志路由
apiVersion: v1
kind: ConfigMap
metadata:
  name: fluent-bit-multi-format
  namespace: logging
data:
  fluent-bit.conf: |
    [SERVICE]
        Flush         1
        Log_Level     info
        
    [INPUT]
        Name              tail
        Path              /var/log/containers/app-*.log
        Parser            docker
        Tag               app.*

    [INPUT]
        Name              tail
        Path              /var/log/containers/nginx-*.log
        Parser            nginx
        Tag               nginx.*

    [FILTER]
        Name          rewrite_tag
        Match         app.*
        Rule          $kubernetes['container_name'] ^(app-.+)$ app.$1 false

    [OUTPUT]
        Name          es
        Match         app.*
        Index         kubernetes-app-%Y.%m.%d
```

<!-- chunk: 📊 日志分析实践 -->## 📊 日志分析实践

## 关键指标监控

## 1. 错误日志监控
```yaml
# 错误日志告警规则
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: log-error-alerts
  namespace: monitoring
spec:
  groups:
  - name: log.rules
    rules:
    - alert: HighErrorRate
      expr: rate(log_messages_total{level="error"}[5m]) > 10
      for: 5m
      labels:
        severity: warning
      annotations:
        summary: "High error log rate detected"
        description: "{{ $labels.app }} is generating {{ printf \"%.2f\" $value }} errors per second"
```

## 2. 应用性能日志
```json
{
  "dashboard": {
    "title": "Application Logs Analysis",
    "panels": [
      {
        "title": "Error Rate by Service",
        "type": "graph",
        "targets": [
          {
            "query": "SELECT count(*) as error_count FROM \"kubernetes-*\" WHERE log.level = 'error' GROUP BY kubernetes.container"
          }
        ]
      },
      {
        "title": "Response Time Distribution",
        "type": "heatmap",
        "targets": [
          {
            "query": "SELECT percentile(response_time, 50) as p50, percentile(response_time, 95) as p95, percentile(response_time, 99) as p99 FROM \"kubernetes-*\" GROUP BY time(5m)"
          }
        ]
      }
    ]
  }
}
```

## 日志搜索优化

## 1. Elasticsearch索引生命周期管理
```yaml
# ILM策略配置
apiVersion: elasticsearch.k8s.elastic.co/v1
kind: Elasticsearch
metadata:
  name: elasticsearch
spec:
  auth:
    fileRealm:
    - username: ilm_admin
      password: changeme
  http:
    tls:
      certificate:
        secretName: elasticsearch-cert
---
PUT _ilm/policy/log_retention_policy
{
  "policy": {
    "phases": {
      "hot": {
        "actions": {
          "rollover": {
            "max_age": "7d",
            "max_size": "50gb"
          }
        }
      },
      "warm": {
        "min_age": "7d",
        "actions": {
          "forcemerge": {
            "max_num_segments": 1
          }
        }
      },
      "cold": {
        "min_age": "30d",
        "actions": {
          "freeze": {}
        }
      },
      "delete": {
        "min_age": "90d",
        "actions": {
          "delete": {}
        }
      }
    }
  }
}
```

## 2. 日志采样策略
```yaml
# 智能日志采样配置
apiVersion: v1
kind: ConfigMap
metadata:
  name: fluent-bit-sampling
  namespace: logging
data:
  fluent-bit.conf: |
    [SERVICE]
        Flush         1
        Log_Level     info
        
    [INPUT]
        Name              tail
        Path              /var/log/containers/debug-*.log
        Parser            docker
        Tag               debug.*
        
    [FILTER]
        Name          throttle
        Match         debug.*
        Rate          100
        Window        300
        Interval      1s
        
    [FILTER]
        Name          grep
        Match         *
        Exclude       log.level debug
        Exclude       kubernetes.container debug-container
```

<!-- chunk: 🔧 平台运维管理 -->## 🔧 平台运维管理

## 安全访问控制

## 1. 认证授权配置
```yaml
# Kibana安全配置
apiVersion: kibana.k8s.elastic.co/v1
kind: Kibana
metadata:
  name: kibana
spec:
  secureSettings:
  - secretName: kibana-secure-settings
---
apiVersion: v1
kind: Secret
metadata:
  name: kibana-secure-settings
type: Opaque
data:
  xpack.security.authc.providers: |
    basic.basic1:
      order: 0
    saml.saml1:
      order: 1
      realm: saml1
  xpack.security.encryptionKey: |
    base64_encoded_encryption_key
```

## 2. 网络策略配置
```yaml
# 日志组件网络隔离
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: logging-isolation
  namespace: logging
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
    ports:
    - protocol: TCP
      port: 9200
  - from:
    - podSelector:
        matchLabels:
          app: kibana
    ports:
    - protocol: TCP
      port: 9200
  egress:
  - to:
    - namespaceSelector:
        matchLabels:
          name: kube-system
    ports:
    - protocol: TCP
      port: 53
    - protocol: UDP
      port: 53
```

## 性能调优

## 1. 资源配额管理
```yaml
# 日志组件资源限制
apiVersion: v1
kind: ResourceQuota
metadata:
  name: logging-quota
  namespace: logging
spec:
  hard:
    requests.cpu: "4"
    requests.memory: 8Gi
    limits.cpu: "8"
    limits.memory: 16Gi
    persistentvolumeclaims: "10"
    requests.storage: 1Ti
---
apiVersion: v1
kind: LimitRange
metadata:
  name: logging-limits
  namespace: logging
spec:
  limits:
  - default:
      cpu: 1
      memory: 2Gi
    defaultRequest:
      cpu: 500m
      memory: 1Gi
    type: Container
```

## 2. 存储优化配置
```yaml
# Elasticsearch存储优化
apiVersion: elasticsearch.k8s.elastic.co/v1
kind: Elasticsearch
metadata:
  name: elasticsearch
spec:
  nodeSets:
  - name: hot-nodes
    count: 3
    config:
      node.roles: ["data_hot", "ingest"]
      index.routing.allocation.require.data: "hot"
    podTemplate:
      spec:
        containers:
        - name: elasticsearch
          resources:
            requests:
              memory: 4Gi
              cpu: 2
            limits:
              memory: 8Gi
              cpu: 4
  - name: warm-nodes
    count: 2
    config:
      node.roles: ["data_warm"]
      index.routing.allocation.require.data: "warm"
    podTemplate:
      spec:
        containers:
        - name: elasticsearch
          resources:
            requests:
              memory: 8Gi
              cpu: 2
            limits:
              memory: 16Gi
              cpu: 4
```

<!-- chunk: 📈 监控与告警 -->## 📈 监控与告警

## 日志平台健康监控

## 1. 组件健康检查
```yaml
# 日志组件健康监控
apiVersion: monitoring.coreos.com/v1
kind: ServiceMonitor
metadata:
  name: logging-health
  namespace: monitoring
spec:
  selector:
    matchLabels:
      app: elasticsearch
  endpoints:
  - port: http
    path: /_cluster/health
    interval: 30s
    scrapeTimeout: 10s
---
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: logging-platform-alerts
  namespace: monitoring
spec:
  groups:
  - name: logging.rules
    rules:
    - alert: ElasticsearchClusterRed
      expr: elasticsearch_cluster_health_status{color="red"} == 1
      for: 2m
      labels:
        severity: critical
      annotations:
        summary: "Elasticsearch cluster is in RED state"
        
    - alert: FluentBitBufferFull
      expr: fluentbit_buffer_overrun_total > 0
      for: 1m
      labels:
        severity: warning
      annotations:
        summary: "Fluent Bit buffer is overrun"
```

## 2. 性能指标监控
```json
{
  "dashboard": {
    "title": "Logging Platform Performance",
    "panels": [
      {
        "title": "Log Processing Rate",
        "type": "graph",
        "targets": [
          {
            "expr": "rate(fluentbit_input_bytes_total[5m])",
            "legendFormat": "Bytes/sec"
          }
        ]
      },
      {
        "title": "Elasticsearch Indexing Rate",
        "type": "graph",
        "targets": [
          {
            "expr": "rate(elasticsearch_indices_indexing_index_total[5m])",
            "legendFormat": "Documents/sec"
          }
        ]
      }
    ]
  }
}
```

<!-- chunk: 🔧 实施检查清单 -->## 🔧 实施检查清单

## 平台部署
- [ ] 设计日志收集架构和数据流向
- [ ] 部署日志收集代理(Fluent Bit/Fluentd)
- [ ] 配置日志缓冲层(Kafka/Redis)
- [ ] 部署日志存储(Elasticsearch集群)
- [ ] 配置日志分析界面(Kibana)
- [ ] 实现日志解析和结构化处理

## 安全与性能
- [ ] 配置访问认证和授权机制
- [ ] 实施网络安全隔离策略
- [ ] 优化存储和查询性能
- [ ] 配置索引生命周期管理
- [ ] 实施日志采样和过滤策略
- [ ] 建立监控告警体系

## 运营维护
- [ ] 制定日志保留和清理策略
- [ ] 建立日志平台运维手册
- [ ] 定期进行性能调优
- [ ] 维护日志分析模板和仪表板
- [ ] 建立故障排查和恢复流程
- [ ] 持续改进日志收集覆盖率

---

*本文档为企业级日志收集分析平台提供完整的技术实施方案和运维指导*

---

<!-- chunk: Obsidian 相关文档 -->## Obsidian 相关文档

- 生产运维 KUDIG Database — Global MOC
- [[13-生产运维/README.md|Domain 11: 生产环境运维最佳实践 ([[Production Operations|Production Operations]]ns Best Practices|Production Operations Best Practices]]佳实践字典|Operations Best Practices]])]]
- Domain-18 生产运维 — 开源项目索引
- [[01-集群基础/02-设计原则/01-production-architecture-design-principles.md|01-生产架构设计原则]]
- 02-多云混合部署策略
- 03-边缘计算生产部署
- 04-企业级监控体系
- 06-APM应用性能监控
- 07-零信任安全架构
- 08-CIS基准合规检查
- 09-软件物料清单
- 10-GitOps流水线实践

## See Also

- 03-edge-computing-production-deployment
- 04-enterprise-monitoring-system
- 06-apm-application-performance-monitoring
- 07-zero-trust-security-architecture

- [[09-可观测性/README.md|返回目录]]

<!-- risk-assessed -->
