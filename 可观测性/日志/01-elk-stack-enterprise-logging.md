---
title: ELK Stack企业级日志管理系统深度实践
description: 'title: ELK Stack企业级日志管理系统深度实践'
summary: 'title: ELK Stack企业级日志管理系统深度实践'
category: general
tags:
- observability
- logging
- grafana
- docker
- kafka
- elasticsearch
- statefulset
- webhook
- serverless
- rag
tier: supporting
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 25min
intent_queries:
- 日志是什么？
- 如何使用日志？
- 日志的最佳实践是什么？
trigger_keywords:
- ELK
- Stack企业级日志管理系统深度实践
- observability
prerequisites:
- kubectl-basics
- observability-basics
- monitoring-basics
- kafka-basics
- logging-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




title: ELK Stack企业级日志管理系统深度实践
description: '# ELK Stack企业级日志管理系统深度实践'
category: logging-management-analytics
tags:
- k8s
- logging
- efk
- loki
- grafana
- docker
- kafka
- elasticsearch
- [[StatefulSet|statefulset]]
- webhook
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- SRE
- 运维工程师
- 数据工程师
estimated_read_time: 5min
intent_queries:
- ELK Stack企业级日志管理系统深度实践 是什么
- 如何 ELK Stack企业级日志管理系统深度实践
- [[Kubernetes|Kubernetes]] 21 logging management analytics 最佳实践
trigger_keywords:
- ELK
- Stack企业级日志管理系统深度实践
- logging
- management
- analytics
authors:
- name: KUDIG Team
  role: contributor
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
---

# ELK Stack企业级日志管理系统深度实践

> **作者**: 日志系统架构专家 | **版本**: v1.0 | **更新时间**: 2026-02-07
> **适用场景**: 企业级日志平台架构 | **复杂度**: ⭐⭐⭐⭐⭐

<!-- chunk: 🎯 摘要 -->## 🎯 摘要

本文档深入探讨了ELK Stack企业级日志管理系统的架构设计、部署实践和运维管理，基于大规模生产环境的实践经验，提供从日志收集到分析可视化的完整技术指南，帮助企业构建高效、可靠的日志管理体系。

<!-- chunk: 1. ELK架构深度解析 -->## 1. ELK架构深度解析

## 1.1 核心组件架构

```mermaid
graph TB
    subgraph "日志收集层"
        A[Filebeat] --> B[Logstash]
        C[Metricbeat] --> B
        D[Packetbeat] --> B
        E[Winlogbeat] --> B
        F[Journald] --> B
    end
    
    subgraph "日志处理层"
        B --> G[Elasticsearch Ingest Node]
        G --> H[Logstash Processing Pipeline]
        H --> I[Elasticsearch Master Node]
    end
    
    subgraph "存储检索层"
        I --> J[Elasticsearch Data Node]
        J --> K[Elasticsearch Coordinating Node]
    end
    
    subgraph "分析展示层"
        L[Kibana] --> K
        M[Grafana] --> K
        N[APM Server] --> K
    end
    
    subgraph "监控管理层"
        O[Elasticsearch Monitoring]
        P[X-Pack Security]
        Q[Elasticsearch Alerting]
    end
```

## 1.2 组件功能详解

```yaml
ELK Stack组件说明:
  Elasticsearch:
    功能: 分布式搜索引擎和分析引擎
    特性: 
      - 全文搜索和结构化搜索
      - 实时分析能力
      - 水平扩展性
      - 高可用性
    版本: 8.11.0+
  
  Logstash:
    功能: 数据处理管道
    特性:
      - 输入插件丰富
      - 过滤处理强大
      - 输出插件多样
      - 可编程性高
    版本: 8.11.0+
  
  Kibana:
    功能: 数据可视化和管理界面
    特性:
      - 丰富的图表类型
      - 仪表板定制
      - 开发者工具
      - 机器学习集成
    版本: 8.11.0+
  
  Beats:
    功能: 轻量级数据收集器
    特性:
      - 资源占用少
      - 部署简单
      - 实时性强
      - 插件化架构
    版本: 8.11.0+
```

<!-- chunk: 2. 企业级部署架构 -->## 2. 企业级部署架构

## 2.1 高可用集群部署

```yaml
# Elasticsearch集群部署
apiVersion: apps/v1
kind: StatefulSet
metadata:
  name: elasticsearch-master
  namespace: logging
spec:
  serviceName: elasticsearch-master
  replicas: 3
  selector:
    matchLabels:
      app: elasticsearch
      role: master
  template:
    metadata:
      labels:
        app: elasticsearch
        role: master
    spec:
      initContainers:
      - name: sysctl
        image: busybox:1.27.2
        command:
        - sysctl
        - -w
        - vm.max_map_count=262144
        securityContext:
          privileged: true
      containers:
      - name: elasticsearch
        image: docker.elastic.co/elasticsearch/elasticsearch:8.11.0
        env:
        - name: cluster.name
          value: "elk-cluster"
        - name: node.name
          valueFrom:
            fieldRef:
              fieldPath: metadata.name
        - name: discovery.seed_hosts
          value: "elasticsearch-master-0.elasticsearch-master,elasticsearch-master-1.elasticsearch-master,elasticsearch-master-2.elasticsearch-master"
        - name: cluster.initial_master_nodes
          value: "elasticsearch-master-0,elasticsearch-master-1,elasticsearch-master-2"
        - name: ES_JAVA_OPTS
          value: "-Xms2g -Xmx2g"
        - name: xpack.security.enabled
          value: "true"
        - name: xpack.security.transport.ssl.enabled
          value: "true"
        ports:
        - containerPort: 9200
          name: http
        - containerPort: 9300
          name: transport
        volumeMounts:
        - name: elasticsearch-data
          mountPath: /usr/share/elasticsearch/data
        resources:
          requests:
            memory: "2Gi"
            cpu: "1"
          limits:
            memory: "4Gi"
            cpu: "2"
  volumeClaimTemplates:
  - metadata:
      name: elasticsearch-data
    spec:
      accessModes: [ "ReadWriteOnce" ]
      storageClassName: "fast-ssd"
      resources:
        requests:
          storage: 100Gi
---
# Elasticsearch数据节点部署
apiVersion: apps/v1
kind: StatefulSet
metadata:
  name: elasticsearch-data
  namespace: logging
spec:
  serviceName: elasticsearch-data
  replicas: 3
  selector:
    matchLabels:
      app: elasticsearch
      role: data
  template:
    metadata:
      labels:
        app: elasticsearch
        role: data
    spec:
      initContainers:
      - name: sysctl
        image: busybox:1.27.2
        command:
        - sysctl
        - -w
        - vm.max_map_count=262144
        securityContext:
          privileged: true
      containers:
      - name: elasticsearch
        image: docker.elastic.co/elasticsearch/elasticsearch:8.11.0
        env:
        - name: cluster.name
          value: "elk-cluster"
        - name: node.name
          valueFrom:
            fieldRef:
              fieldPath: metadata.name
        - name: node.roles
          value: "data,content,transform"
        - name: discovery.seed_hosts
          value: "elasticsearch-master-0.elasticsearch-master,elasticsearch-master-1.elasticsearch-master,elasticsearch-master-2.elasticsearch-master"
        - name: ES_JAVA_OPTS
          value: "-Xms4g -Xmx4g"
        - name: xpack.security.enabled
          value: "true"
        ports:
        - containerPort: 9200
          name: http
        - containerPort: 9300
          name: transport
        volumeMounts:
        - name: elasticsearch-data
          mountPath: /usr/share/elasticsearch/data
        resources:
          requests:
            memory: "4Gi"
            cpu: "2"
          limits:
            memory: "8Gi"
            cpu: "4"
  volumeClaimTemplates:
  - metadata:
      name: elasticsearch-data
    spec:
      accessModes: [ "ReadWriteOnce" ]
      storageClassName: "fast-ssd"
      resources:
        requests:
          storage: 500Gi
```

## 2.2 Filebeat日志收集配置

```yaml
# Filebeat配置文件
filebeat.inputs:
- type: log
  enabled: true
  paths:
    - /var/log/*.log
    - /var/log/application/*.log
    - /var/log/nginx/*.log
  fields:
    service: application
    environment: production
  fields_under_root: true
  multiline.pattern: '^\d{4}-\d{2}-\d{2}'
  multiline.negate: true
  multiline.match: after
  ignore_older: 72h
  close_inactive: 2h
  scan_frequency: 10s

- type: container
  enabled: true
  paths:
    - /var/lib/docker/containers/*/*.log
  stream: all
  processors:
    - add_docker_metadata: ~
    - add_kubernetes_metadata:
        host: ${NODE_NAME}
        matchers:
        - logs_path:
            logs_path: "/var/lib/docker/containers/"

processors:
- add_host_metadata: ~
- add_cloud_metadata: ~
- add_fields:
    target: ''
    fields:
      index_prefix: "application-logs"
      log_type: "application"

output.elasticsearch:
  hosts: ["elasticsearch-data-0.elasticsearch-data:9200"]
  username: "${ELASTIC_USERNAME}"
  password: "${ELASTIC_PASSWORD}"
  index: "%{[index_prefix]}-%{+yyyy.MM.dd}"
  bulk_max_size: 2048
  worker: 2

setup.template.enabled: false
setup.ilm.enabled: false

logging.level: info
logging.to_files: true
logging.files:
  path: /var/log/filebeat
  name: filebeat
  keepfiles: 7
  permissions: 0644
```

<!-- chunk: 3. 日志处理管道设计 -->## 3. 日志处理管道设计

## 3.1 Logstash配置管道

```ruby
# Logstash主配置文件
input {
  beats {
    port => 5044
    ssl => true
    ssl_certificate => "/etc/logstash/certs/logstash.crt"
    ssl_key => "/etc/logstash/certs/logstash.key"
  }
  
  kafka {
    bootstrap_servers => "kafka-0:9092,kafka-1:9092,kafka-2:9092"
    topics => ["application-logs", "system-logs", "security-logs"]
    group_id => "logstash-consumer"
    codec => "json"
  }
}

filter {
  # 通用字段处理
  mutate {
    add_field => {
      "[@metadata][received_at]" => "%{@timestamp}"
      "[@metadata][pipeline]" => "main"
    }
    rename => {
      "message" => "raw_message"
    }
  }
  
  # 时间戳标准化
  date {
    match => [ "timestamp", "ISO8601", "yyyy-MM-dd HH:mm:ss", "UNIX_MS" ]
    target => "@timestamp"
    remove_field => [ "timestamp" ]
  }
  
  # JSON消息解析
  json {
    source => "raw_message"
    skip_on_invalid_json => true
    target => "parsed_json"
  }
  
  # 应用日志处理
  if [fields][service] == "application" {
    grok {
      match => {
        "raw_message" => [
          "%{TIMESTAMP_ISO8601:timestamp} \[%{LOGLEVEL:level}\] %{JAVACLASS:class} - %{GREEDYDATA:message}",
          "%{TIMESTAMP_ISO8601:timestamp} %{LOGLEVEL:level} %{GREEDYDATA:message}"
        ]
      }
      tag_on_failure => ["_grokparsefailure_application"]
    }
    
    # 应用特定字段提取
    if [parsed_json] {
      mutate {
        add_field => {
          "user_id" => "%{[parsed_json][userId]}"
          "request_id" => "%{[parsed_json][requestId]}"
          "response_time" => "%{[parsed_json][responseTime]}"
        }
      }
    }
  }
  
  # Nginx访问日志处理
  if [fields][service] == "nginx" {
    grok {
      match => {
        "raw_message" => '%{IPORHOST:clientip} %{USER:ident} %{USER:auth} \[%{HTTPDATE:timestamp}\] "%{WORD:verb} %{DATA:request} HTTP/%{NUMBER:httpversion}" %{NUMBER:response:int} (?:%{NUMBER:bytes:int}|-) (?:"(?:%{URI:referrer}|-)"|%{QS:referrer}) %{QS:agent}'
      }
      tag_on_failure => ["_grokparsefailure_nginx"]
    }
    
    # 用户代理解析
    useragent {
      source => "agent"
      target => "user_agent"
    }
    
    # 地理位置解析
    geoip {
      source => "clientip"
      target => "geoip"
    }
  }
  
  # 系统日志处理
  if [fields][service] == "system" {
    syslog_pri { }
    
    grok {
      match => {
        "raw_message" => "<%{POSINT:priority}>%{SYSLOGTIMESTAMP:timestamp} %{SYSLOGHOST:logsource} %{PROG:program}(?:\[%{POSINT:pid}\])?: %{GREEDYDATA:message}"
      }
      tag_on_failure => ["_grokparsefailure_syslog"]
    }
  }
  
  # 字段类型转换
  mutate {
    convert => {
      "response_time" => "float"
      "bytes" => "integer"
      "response" => "integer"
    }
  }
  
  # 添加索引路由信息
  mutate {
    add_field => {
      "[@metadata][index]" => "%{[fields][service]}-%{+YYYY.MM.dd}"
      "[@metadata][routing]" => "%{[fields][environment]}"
    }
  }
}

output {
  # 主要输出到Elasticsearch
  elasticsearch {
    hosts => ["elasticsearch-data-0.elasticsearch-data:9200"]
    index => "%{[@metadata][index]}"
    routing => "%{[@metadata][routing]}"
    user => "${ELASTIC_USERNAME}"
    password => "${ELASTIC_PASSWORD}"
    ssl => true
    ssl_certificate_verification => false
    ilm_enabled => true
    ilm_rollover_alias => "%{[fields][service]}-logs"
    ilm_pattern => "{now/d}-000001"
    ilm_policy => "log-lifecycle-policy"
    template_name => "%{[fields][service]}-template"
    template => "/etc/logstash/templates/%{[fields][service]}-template.json"
    template_overwrite => true
  }
  
  # 备份输出到Kafka
  kafka {
    bootstrap_servers => "kafka-0:9092,kafka-1:9092,kafka-2:9092"
    topic_id => "%{[fields][service]}-backup"
    codec => json
  }
  
  # 监控输出
  if "_grokparsefailure" in [tags] {
    elasticsearch {
      hosts => ["elasticsearch-data-0.elasticsearch-data:9200"]
      index => "failed-parses-%{+YYYY.MM.dd}"
      user => "${ELASTIC_USERNAME}"
      password => "${ELASTIC_PASSWORD}"
    }
  }
}
```

<!-- chunk: 4. 索引生命周期管理 -->## 4. 索引生命周期管理

## 4.1 ILM策略配置

```json
{
  "policy": {
    "phases": {
      "hot": {
        "min_age": "0ms",
        "actions": {
          "rollover": {
            "max_age": "7d",
            "max_size": "50gb",
            "max_docs": 10000000
          },
          "set_priority": {
            "priority": 100
          }
        }
      },
      "warm": {
        "min_age": "7d",
        "actions": {
          "allocate": {
            "number_of_replicas": 1
          },
          "readonly": {},
          "set_priority": {
            "priority": 50
          }
        }
      },
      "cold": {
        "min_age": "30d",
        "actions": {
          "allocate": {
            "require": {
              "box_type": "cold"
            }
          },
          "freeze": {},
          "set_priority": {
            "priority": 0
          }
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

## 4.2 索引模板配置

```json
{
  "index_patterns": ["application-logs-*"],
  "template": {
    "settings": {
      "number_of_shards": 3,
      "number_of_replicas": 1,
      "refresh_interval": "30s",
      "blocks": {
        "read_only_allow_delete": "false"
      },
      "codec": "best_compression",
      "translog": {
        "durability": "async",
        "sync_interval": "5s"
      }
    },
    "mappings": {
      "properties": {
        "@timestamp": {
          "type": "date"
        },
        "level": {
          "type": "keyword"
        },
        "message": {
          "type": "text",
          "analyzer": "standard"
        },
        "raw_message": {
          "type": "text",
          "index": false
        },
        "service": {
          "type": "keyword"
        },
        "environment": {
          "type": "keyword"
        },
        "host": {
          "properties": {
            "name": { "type": "keyword" },
            "ip": { "type": "ip" }
          }
        },
        "container": {
          "properties": {
            "id": { "type": "keyword" },
            "name": { "type": "keyword" },
            "image": { "type": "keyword" }
          }
        },
        "kubernetes": {
          "properties": {
            "pod": {
              "properties": {
                "name": { "type": "keyword" },
                "uid": { "type": "keyword" }
              }
            },
            "namespace": { "type": "keyword" },
            "node": { "type": "keyword" }
          }
        },
        "geoip": {
          "properties": {
            "location": { "type": "geo_point" },
            "country_name": { "type": "keyword" },
            "city_name": { "type": "keyword" }
          }
        }
      }
    }
  },
  "composed_of": ["logs-mappings", "logs-settings"],
  "priority": 500,
  "version": 3,
  "_meta": {
    "description": "Application logs template"
  }
}
```

<!-- chunk: 5. Kibana可视化配置 -->## 5. Kibana可视化配置

## 5.1 仪表板配置

```json
{
  "dashboard": {
    "title": "Application Logs Overview",
    "description": "综合应用日志监控面板",
    "panelsJSON": "[{\"id\":\"application-logs-metrics\",\"type\":\"visualization\",\"panelIndex\":1,\"gridData\":{\"x\":0,\"y\":0,\"w\":24,\"h\":12}}, {\"id\":\"error-rate-trend\",\"type\":\"visualization\",\"panelIndex\":2,\"gridData\":{\"x\":0,\"y\":12,\"w\":12,\"h\":12}}, {\"id\":\"top-error-sources\",\"type\":\"visualization\",\"panelIndex\":3,\"gridData\":{\"x\":12,\"y\":12,\"w\":12,\"h\":12}}]",
    "optionsJSON": "{\"darkTheme\":false,\"hidePanelTitles\":false,\"useMargins\":true}",
    "version": 1,
    "timeRestore": true,
    "timeTo": "now",
    "timeFrom": "now-24h",
    "refreshInterval": {
      "display": "30 seconds",
      "pause": false,
      "value": 30000
    }
  }
}
```

## 5.2 可视化查询配置

```json
{
  "visualization": {
    "title": "Error Rate Trend",
    "visState": "{\"title\":\"Error Rate Trend\",\"type\":\"line\",\"params\":{\"addTooltip\":true,\"addLegend\":true,\"legendPosition\":\"right\",\"scale\":\"linear\",\"mode\":\"normal\",\"times\":[],\"addTimeMarker\":false,\"defaultYExtents\":false,\"setYExtents\":false,\"yAxis\":{}},\"aggs\":[{\"id\":\"1\",\"enabled\":true,\"type\":\"cardinality\",\"schema\":\"metric\",\"params\":{\"field\":\"request_id\"}},{\"id\":\"2\",\"enabled\":true,\"type\":\"date_histogram\",\"schema\":\"segment\",\"params\":{\"field\":\"@timestamp\",\"interval\":\"auto\",\"customInterval\":\"2h\",\"min_doc_count\":1,\"extended_bounds\":{}}},{\"id\":\"3\",\"enabled\":true,\"type\":\"terms\",\"schema\":\"group\",\"params\":{\"field\":\"level\",\"size\":5,\"order\":\"desc\",\"orderBy\":\"1\"}}],\"listeners\":{}}",
    "uiStateJSON": "{}",
    "description": "",
    "version": 1,
    "kibanaSavedObjectMeta": {
      "searchSourceJSON": "{\"index\":\"application-logs-*\",\"filter\":[],\"query\":{\"query\":\"level:ERROR OR level:CRITICAL\",\"language\":\"kuery\"}}"
    }
  }
}
```

<!-- chunk: 6. 安全与权限管理 -->## 6. 安全与权限管理

## 6.1 Elasticsearch安全配置

```yaml
# Elasticsearch安全配置
xpack.security.enabled: true
xpack.security.transport.ssl.enabled: true
xpack.security.transport.ssl.verification_mode: certificate
xpack.security.transport.ssl.key: certs/elastic-certificates.key
xpack.security.transport.ssl.certificate: certs/elastic-certificates.crt
xpack.security.transport.ssl.certificate_authorities: certs/elastic-stack-ca.crt
xpack.security.http.ssl.enabled: true
xpack.security.http.ssl.truststore.path: certs/elastic-certificates.p12
xpack.security.http.ssl.keystore.path: certs/elastic-certificates.p12

# 用户角色配置
xpack.security.authc.realms:
  native.native1:
    order: 0
  ldap.ldap1:
    order: 1
    url: "ldaps://ldap.example.com:636"
    bind_dn: "cn=admin,dc=example,dc=com"
    user_search:
      base_dn: "dc=example,dc=com"
      filter: "(cn={0})"
    group_search:
      base_dn: "dc=example,dc=com"
    files:
      role_mapping: "/usr/share/elasticsearch/config/roles_mapping.yml"
```

## 6.2 角色权限配置

```yaml
# Elasticsearch角色定义
roles:
  log_admin:
    cluster: 
      - all
    indices:
      - names: '*'
        privileges: 
          - all
    
  log_viewer:
    cluster:
      - monitor
    indices:
      - names: 'application-logs-*'
        privileges:
          - read
          - view_index_metadata
      - names: 'system-logs-*'
        privileges:
          - read
          - view_index_metadata
    
  developer:
    cluster:
      - monitor
    indices:
      - names: 'application-logs-*'
        privileges:
          - read
          - view_index_metadata
        field_security:
          grant: ['message', 'level', 'timestamp', 'service']
    
  auditor:
    cluster:
      - monitor
    indices:
      - names: '*'
        privileges:
          - read
          - view_index_metadata
        query: '{"term": {"environment": "production"}}'
```

<!-- chunk: 7. 性能优化与调优 -->## 7. 性能优化与调优

## 7.1 Elasticsearch性能调优

```yaml
# Elasticsearch性能优化配置
performance_tuning:
  jvm:
    heap_size: "31g"  # 总内存的50%，不超过32GB
    gc_settings:
      - "-XX:+UseG1GC"
      - "-XX:MaxGCPauseMillis=200"
      - "-XX:G1HeapRegionSize=32m"
  
  indexing:
    refresh_interval: "30s"
    translog:
      durability: "async"
      sync_interval: "5s"
    merge:
      policy:
        max_merge_at_once: 10
        segments_per_tier: 10
  
  search:
    request_cache: true
    query_cache: true
    field_data_cache: true
    indices:
      queries:
        cache:
          size: "20%"
  
  networking:
    tcp:
      no_delay: true
      keep_alive: true
    http:
      compression: true
      max_content_length: "200mb"
  
  thread_pools:
    search:
      size: 20
      queue_size: 1000
    write:
      size: 10
      queue_size: 1000
    get:
      size: 10
      queue_size: 1000
```

## 7.2 Logstash性能优化

```ruby
# Logstash性能优化配置
pipeline:
  batch_size: 125
  batch_delay: 50
  workers: 4
  
input {
  beats {
    port => 5044
    codec => "json"
    # 启用压缩
    client_inactivity_timeout => 3600
  }
}

filter {
  # 并行处理
  if [type] == "application" {
    # 应用特定处理
  } else if [type] == "nginx" {
    # Nginx特定处理
  }
  
  # 避免不必要的字段处理
  mutate {
    remove_field => ["@version", "tags", "_id"]
  }
}

output {
  elasticsearch {
    hosts => ["elasticsearch-host:9200"]
    # 批量提交优化
    flush_size => 5000
    idle_flush_time => 5
    # 连接池优化
    pool_max => 20
    pool_max_per_route => 10
  }
}
```

<!-- chunk: 8. 监控与告警 -->## 8. 监控与告警

## 8.1 系统监控配置

```yaml
# Elasticsearch监控配置
monitoring:
  collection:
    enabled: true
    exporters:
      local:
        type: local
      http:
        type: http
        host: ["monitoring-elasticsearch:9200"]
        auth:
          username: monitoring_user
          password: monitoring_password

# Logstash监控配置
monitoring.enabled: true
monitoring.elasticsearch.hosts: ["elasticsearch:9200"]
monitoring.elasticsearch.username: "logstash_monitoring"
monitoring.elasticsearch.password: "password"
```

## 8.2 告警规则配置

```yaml
# Elastic Stack告警规则
alerts:
  - name: "High Error Rate"
    type: "metric"
    condition: "avg(error_rate) > 0.05"
    timeframe: "5m"
    actions:
      - type: "email"
        recipients: ["ops-team@example.com"]
      - type: "slack"
        channel: "#alerts"
  
  - name: "Elasticsearch Cluster Health"
    type: "cluster_health"
    condition: "cluster_status != 'green'"
    timeframe: "1m"
    actions:
      - type: "pagerduty"
        routing_key: "your-pagerduty-key"
  
  - name: "Log Ingestion Lag"
    type: "ingestion_lag"
    condition: "lag_seconds > 300"
    timeframe: "10m"
    actions:
      - type: "webhook"
        url: "https://internal-api.example.com/alerts"
```

<!-- chunk: 9. 故障排查与维护 -->## 9. 故障排查与维护

## 9.1 常见问题诊断

```bash
# ELK Stack故障排查命令

# 1. 检查Elasticsearch集群状态
curl -u elastic:password -X GET "localhost:9200/_cluster/health?pretty"

# 2. 查看节点统计信息
curl -u elastic:password -X GET "localhost:9200/_nodes/stats?pretty"

# 3. 检查索引状态
curl -u elastic:password -X GET "localhost:9200/_cat/indices?v"

# 4. 查看未分配分片
curl -u elastic:password -X GET "localhost:9200/_cat/shards?v&h=index,shard,prirep,state,unassigned.reason"

# 5. 检查Logstash处理状态
curl -X GET "localhost:9600/_node/stats/pipeline?pretty"

# 6. Filebeat状态检查
filebeat test config
filebeat test output

# 7. 性能分析
curl -u elastic:password -X GET "localhost:9200/_cluster/allocation/explain?pretty"
```

## 9.2 维护脚本

```python
#!/usr/bin/env python3
# elk_maintenance.py

import requests
import json
import logging
from datetime import datetime, timedelta

class ELKMaintenance:
    def __init__(self, es_host, username, password):
        self.es_host = es_host
        self.auth = (username, password)
        self.session = requests.Session()
        self.session.auth = self.auth
        self.logger = logging.getLogger(__name__)
    
    def check_cluster_health(self):
        """检查集群健康状态"""
        try:
            response = self.session.get(f"{self.es_host}/_cluster/health")
            health = response.json()
            
            self.logger.info(f"Cluster Status: {health['status']}")
            self.logger.info(f"Active Shards: {health['active_shards']}/{health['active_shards'] + health['unassigned_shards']}")
            
            if health['status'] != 'green':
                self.logger.warning(f"Cluster health is {health['status']}")
                return False
            return True
        except Exception as e:
            self.logger.error(f"Failed to check cluster health: {e}")
            return False
    
    def clean_old_indices(self, days_to_keep=30):
        """清理旧索引"""
        try:
            # 获取所有索引
            response = self.session.get(f"{self.es_host}/_cat/indices?format=json")
            indices = response.json()
            
            cutoff_date = datetime.now() - timedelta(days=days_to_keep)
            
            for index in indices:
                index_name = index['index']
                # 解析日期
                if '-' in index_name:
                    try:
                        date_part = index_name.split('-')[-1]
                        index_date = datetime.strptime(date_part, '%Y.%m.%d')
                        
                        if index_date < cutoff_date:
                            self.logger.info(f"Deleting old index: {index_name}")
                            delete_response = self.session.delete(f"{self.es_host}/{index_name}")
                            if delete_response.status_code == 200:
                                self.logger.info(f"Successfully deleted {index_name}")
                    except ValueError:
                        continue
                        
        except Exception as e:
            self.logger.error(f"Failed to clean old indices: {e}")
    
    def optimize_indices(self):
        """优化索引性能"""
        try:
            # 强制合并小段
            response = self.session.post(f"{self.es_host}/_forcemerge?max_num_segments=1")
            if response.status_code == 200:
                self.logger.info("Index optimization completed")
        except Exception as e:
            self.logger.error(f"Failed to optimize indices: {e}")

if __name__ == "__main__":
    maintenance = ELKMaintenance(
        "http://localhost:9200",
        "elastic",
        "password"
    )
    
    maintenance.check_cluster_health()
    maintenance.clean_old_indices(30)
    maintenance.optimize_indices()
```

<!-- chunk: 10. 最佳实践与未来发展 -->## 10. 最佳实践与未来发展

## 10.1 日志管理最佳实践

```markdown
<!-- chunk: 📝 日志管理最佳实践 -->## 📝 日志管理最佳实践

## 1. 日志格式标准化
- 使用JSON格式记录结构化日志
- 统一时间戳格式(ISO8601)
- 包含必要的上下文信息
- 避免敏感信息泄露

## 2. 索引策略优化
- 按服务和时间分割索引
- 合理设置分片和副本数
- 实施生命周期管理
- 定期清理过期数据

## 3. 性能优化要点
- 适当调整JVM堆大小
- 优化批量处理参数
- 启用适当的缓存机制
- 监控和调优资源使用

## 4. 安全合规要求
- 启用传输层加密
- 实施细粒度访问控制
- 定期审计日志访问
- 符合数据保护法规
```

## 10.2 技术发展趋势

```yaml
日志技术发展趋势:
  1. 云原生日志:
     - Serverless日志收集
     - 多云统一日志平台
     - 边缘计算日志处理
     - 无服务器架构集成
  
  2. 智能化分析:
     - AI驱动的异常检测
     - 自然语言处理日志
     - 自动根因分析
     - 预测性维护
  
  3. 实时处理增强:
     - 流式处理能力提升
     - 复杂事件处理
     - 实时告警响应
     - 交互式查询优化
```

---
*本文档基于企业级日志管理系统实践经验编写，持续更新最新技术和最佳实践。*

---

<!-- chunk: Obsidian 相关文档 -->## Obsidian 相关文档

- domain-21-logging-management-analytics MOC
- [[可观测性/README.md|Domain 06: 日志管理与分析 (Logging Management & Analytics)]]
- [[可观测性/00-open-source-projects-index.md|Domain-21 日志管理与分析 — 开源项目索引]]
- Fluentd企业级日志收集与处理深度实践
- Loki Enterprise Log Aggregation and Analytics Platform
- 企业级日志治理与合规审计深度实践
- Graylog 企业级日志管理平台深度实践
- Splunk企业级日志分析与安全智能平台深度实践
- 企业级实时日志分析与业务洞察深度实践
- Splunk Enterprise Log Analytics Platform 深度实践
- Loggly Cloud Log Management Platform 深度实践

## See Also

- 05-splunk-enterprise-log-analytics
- 06-loggly-cloud-log-management
- 02-fluentd-enterprise-log-processing
- 03-loki-enterprise-log-aggregation

- [[可观测性/README.md|返回目录]]

## Related

- [[生态参考/topic-index/observability-index.md|Observability 可观测性知识图谱索引]]


<!-- risk-assessed -->
