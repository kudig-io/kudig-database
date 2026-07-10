---
title: Elastic Stack企业级可观测性平台深度实践
description: 'title: Elastic Stack企业级可观测性平台深度实践'
summary: 'title: Elastic Stack企业级可观测性平台深度实践'
category: general
tags:
- observability
- monitoring
- alerting
- prometheus
- grafana
- docker
- redis
- mysql
- kafka
- elasticsearch
tier: supporting
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 45min
intent_queries:
- elastic-stack-enterprise-observability是什么？
- elastic-stack-enterprise-observability的使用方法
- elastic-stack-enterprise-observability的最佳实践
trigger_keywords:
- Elastic
- Stack企业级可观测性平台深度实践
- observability
prerequisites:
- kubectl-basics
- observability-basics
- prometheus-basics
- monitoring-basics
- kafka-basics
- redis-basics
- mysql-basics
- logging-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




title: Elastic Stack企业级可观测性平台深度实践
description: '# Elastic Stack企业级可观测性平台深度实践'
category: enterprise-monitoring-alerting
tags:
- k8s
- monitoring
- alerting
- [[Prometheus|prometheus]]
- docker
- redis
- mysql
- kafka
- elasticsearch
- [[StatefulSet|statefulset]]
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- SRE
- 监控工程师
- 运维工程师
estimated_read_time: 5min
intent_queries:
- Elastic Stack企业级可观测性平台深度实践 是什么
- 如何 Elastic Stack企业级可观测性平台深度实践
- Kubernetes 20 enterprise monitoring alerting 最佳实践
trigger_keywords:
- Elastic
- Stack企业级可观测性平台深度实践
- enterprise
- monitoring
- alerting
cross_refs:
- type: cheatsheet
  path: ../系统基础/topic-cheat-sheet/promql.md
  label: '速查卡: promql'
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

# Elastic Stack企业级可观测性平台深度实践

> **文档定位**: 企业级Elasticsearch、Logstash、Kibana、Beats完整可观测性解决方案 | **更新时间**: 2026-02-07
> 
> 本文档深入解析Elastic Stack在企业环境中的完整可观测性平台建设，涵盖日志分析、指标监控、APM追踪、安全分析等核心功能，为构建统一的企业级数据洞察平台提供专业指导。

<!-- chunk: 📋 文档目录 -->## 📋 文档目录

- [架构概述](#架构概述)
- [核心组件深度解析](#核心组件深度解析)
- [企业级部署架构](#企业级部署架构)
- [日志分析与处理](#日志分析与处理)
- [指标监控系统](#指标监控系统)
- [APM应用性能监控](#apm应用性能监控)
- [安全信息与事件管理](#安全信息与事件管理)
- [可视化与告警](#可视化与告警)
- [性能优化策略](#性能优化策略)
- [最佳实践总结](#最佳实践总结)

---

<!-- chunk: 架构概述 -->## 架构概述

## Elastic Stack平台架构

```yaml
# Elastic Stack企业级可观测性平台整体架构
elastic_stack_platform:
  数据采集层:
    filebeat: 文件日志采集器
    metricbeat: 系统指标采集器
    packetbeat: 网络数据包分析器
    winlogbeat: Windows事件日志采集器
    auditbeat: 审计数据采集器
    heartbeat: 可用性监控器
    apm_server: APM数据接收器
    
  数据处理层:
    logstash: 数据处理和转换管道
    elasticsearch_ingest_nodes: 摄取节点处理
    apm_server_processing: APM数据处理
    
  存储分析层:
    elasticsearch_cluster: 分布式搜索引擎集群
    ilm_policy: 索引生命周期管理
    snapshot_repository: 快照备份存储
    
  展示管理层:
    kibana: 数据可视化和分析平台
    apm_ui: APM专用界面
    siem_app: 安全信息事件管理
    monitoring_ui: 集群监控界面
```

## 核心价值主张

**统一数据平台**
- 单一平台处理Logs、Metrics、APM三大数据类型
- 统一的查询语言和API接口
- 跨领域数据关联分析能力
- 降低多工具链集成复杂度

**实时分析能力**
- 亚秒级数据摄取和查询响应
- 实时流式数据处理
- 机器学习驱动的异常检测
- 交互式数据分析体验

**企业级特性**
- 多租户架构和细粒度权限控制
- 数据加密和合规性保障
- 高可用部署和灾备能力
- 水平扩展和弹性伸缩

---

<!-- chunk: 核心组件深度解析 -->## 核心组件深度解析

## Elasticsearch架构详解

## 集群架构设计

```yaml
# Elasticsearch企业级集群架构
elasticsearch_cluster:
  master_nodes:
    - node_name: es-master-01
      roles: [master]
      heap_size: 4g
      storage: 50gb
      
    - node_name: es-master-02
      roles: [master]
      heap_size: 4g
      storage: 50gb
      
    - node_name: es-master-03
      roles: [master]
      heap_size: 4g
      storage: 50gb
      
  data_nodes:
    - node_name: es-data-hot-01
      roles: [data, ingest]
      heap_size: 31g
      storage: 2tb_ssd
      node_attributes:
        data: hot
        
    - node_name: es-data-warm-01
      roles: [data]
      heap_size: 31g
      storage: 4tb_hdd
      node_attributes:
        data: warm
        
  coordinating_nodes:
    - node_name: es-coord-01
      roles: [ingest]
      heap_size: 8g
      storage: 100gb
      
  machine_learning_nodes:
    - node_name: es-ml-01
      roles: [ml, transform]
      heap_size: 8g
      storage: 500gb
```

## 索引生命周期管理(ILM)

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
            "number_of_replicas": 1,
            "include": {
              "data": "warm"
            }
          },
          "forcemerge": {
            "max_num_segments": 1
          },
          "set_priority": {
            "priority": 50
          }
        }
      },
      "cold": {
        "min_age": "30d",
        "actions": {
          "allocate": {
            "number_of_replicas": 1,
            "include": {
              "data": "cold"
            }
          },
          "freeze": {},
          "set_priority": {
            "priority": 0
          }
        }
      },
      "delete": {
        "min_age": "365d",
        "actions": {
          "delete": {}
        }
      }
    }
  }
}
```

## Beats数据采集器详解

## Filebeat配置优化

```yaml
# Filebeat企业级配置
filebeat.inputs:
  - type: log
    enabled: true
    paths:
      - /var/log/application/*.log
      - /var/log/nginx/access.log
      - /var/log/system/*.log
    fields:
      service: web-application
      environment: production
      data_center: dc1
      
    multiline.pattern: '^\d{4}-\d{2}-\d{2}'
    multiline.negate: true
    multiline.match: after
    
    ignore_older: 72h
    close_inactive: 2h
    clean_inactive: 25h
    
    harvester_buffer_size: 16384
    max_bytes: 10485760

  - type: container
    enabled: true
    paths:
      - '/var/lib/docker/containers/*/*.log'
    stream: all
    cri.parse_flags: true
    ids:
      - "*"

processors:
  - add_host_metadata: ~
  - add_cloud_metadata: ~
  - add_docker_metadata: ~
  - add_kubernetes_metadata:
      in_cluster: true
      
  - decode_json_fields:
      fields: ["message"]
      process_array: false
      max_depth: 10
      target: "json"
      overwrite_keys: true
      
  - drop_fields:
      fields: ["agent", "ecs", "log", "input"]
      ignore_missing: true

output.elasticsearch:
  hosts: ["https://es-coord-01:9200", "https://es-coord-02:9200"]
  username: "${ELASTIC_USERNAME}"
  password: "${ELASTIC_PASSWORD}"
  ssl.certificate_authorities: ["/etc/filebeat/certs/ca.crt"]
  ssl.certificate: "/etc/filebeat/certs/filebeat.crt"
  ssl.key: "/etc/filebeat/certs/filebeat.key"
  
  bulk_max_size: 2048
  flush_interval: 1s
  compression_level: 3
  
  index: "filebeat-%{[agent.version]}-%{+yyyy.MM.dd}"

setup.template.enabled: true
setup.template.name: "filebeat"
setup.template.pattern: "filebeat-*"
setup.ilm.enabled: true
setup.ilm.rollover_alias: "filebeat"
setup.ilm.pattern: "{now/d}-000001"
```

## Metricbeat系统监控

```yaml
# Metricbeat系统监控配置
metricbeat.modules:
  - module: system
    metricsets:
      - cpu
      - load
      - memory
      - network
      - process
      - process_summary
      - uptime
      - socket
    enabled: true
    period: 10s
    processes: ['.*']
    
  - module: docker
    metricsets:
      - container
      - cpu
      - diskio
      - healthcheck
      - image
      - info
      - memory
      - network
    enabled: true
    period: 30s
    hosts: ["unix:///var/run/docker.sock"]
    
  - module: kubernetes
    metricsets:
      - container
      - node
      - pod
      - system
      - volume
    enabled: true
    period: 30s
    hosts: ["${NODE_NAME}:10255"]
    bearer_token_file: /var/run/secrets/kubernetes.io/serviceaccount/token
    ssl.verification_mode: none

processors:
  - add_cloud_metadata: ~
  - add_docker_metadata: ~
  - add_kubernetes_metadata:
      in_cluster: true
      
  - script:
      lang: javascript
      id: calculate_derived_metrics
      source: >
        function process(event) {
          // 计算CPU使用率百分比
          var cpu_total = event.Get("system.cpu.total.norm.pct");
          if (cpu_total !== null) {
            event.Put("system.cpu.usage_percent", Math.round(cpu_total * 100));
          }
          
          // 计算内存使用率
          var memory_used = event.Get("system.memory.actual.used.bytes");
          var memory_total = event.Get("system.memory.total");
          if (memory_used !== null && memory_total !== null && memory_total > 0) {
            var memory_pct = (memory_used / memory_total) * 100;
            event.Put("system.memory.usage_percent", Math.round(memory_pct));
          }
        }

output.elasticsearch:
  hosts: ["https://es-coord-01:9200"]
  username: "${ELASTIC_USERNAME}"
  password: "${ELASTIC_PASSWORD}"
  indices:
    - index: "metricbeat-system-%{+yyyy.MM.dd}"
      when.contains:
        kubernetes.namespace: "system"
        
    - index: "metricbeat-apps-%{+yyyy.MM.dd}"
      when.not.contains:
        kubernetes.namespace: "system"
```

---

<!-- chunk: 企业级部署架构 -->## 企业级部署架构

## 高可用集群部署

## Kubernetes部署架构

```yaml
# Elastic Stack Kubernetes部署配置
apiVersion: v1
kind: Namespace
metadata:
  name: elastic-stack

---
# Elasticsearch StatefulSet
apiVersion: apps/v1
kind: StatefulSet
metadata:
  name: elasticsearch
  namespace: elastic-stack
spec:
  serviceName: elasticsearch-headless
  replicas: 6
  selector:
    matchLabels:
      app: elasticsearch
  template:
    metadata:
      labels:
        app: elasticsearch
    spec:
      initContainers:
        - name: sysctl
          image: busybox:1.27.2
          command: ["sysctl", "-w", "vm.max_map_count=262144"]
          securityContext:
            privileged: true
        - name: chown
          image: docker.elastic.co/elasticsearch/elasticsearch:8.11.0
          command: ["chown", "-R", "1000:1000", "/usr/share/elasticsearch/data"]
          volumeMounts:
            - name: elasticsearch-data
              mountPath: /usr/share/elasticsearch/data
              
      containers:
        - name: elasticsearch
          image: docker.elastic.co/elasticsearch/elasticsearch:8.11.0
          env:
            - name: cluster.name
              value: "enterprise-elastic"
            - name: node.name
              valueFrom:
                fieldRef:
                  fieldPath: metadata.name
            - name: discovery.seed_hosts
              value: "elasticsearch-0.elasticsearch-headless,elasticsearch-1.elasticsearch-headless,elasticsearch-2.elasticsearch-headless"
            - name: cluster.initial_master_nodes
              value: "elasticsearch-0,elasticsearch-1,elasticsearch-2"
            - name: ES_JAVA_OPTS
              value: "-Xms31g -Xmx31g"
            - name: xpack.security.enabled
              value: "true"
            - name: xpack.security.transport.ssl.enabled
              value: "true"
              
          ports:
            - containerPort: 9200
              name: http
            - containerPort: 9300
              name: transport
              
          readinessProbe:
            exec:
              command:
                - bash
                - -c
                - |
                  curl -s --cacert /usr/share/elasticsearch/config/certs/ca.crt \
                  -u ${ELASTIC_USERNAME}:${ELASTIC_PASSWORD} \
                  https://127.0.0.1:9200/_cluster/health?local=true | grep -q '"status":"green"|"status":"yellow"'
            initialDelaySeconds: 60
            periodSeconds: 10
            
          livenessProbe:
            exec:
              command:
                - bash
                - -c
                - |
                  curl -s --cacert /usr/share/elasticsearch/config/certs/ca.crt \
                  -u ${ELASTIC_USERNAME}:${ELASTIC_PASSWORD} \
                  https://127.0.0.1:9200/_cluster/health?local=true | grep -q '"status":"red"' && exit 1 || exit 0
            initialDelaySeconds: 120
            periodSeconds: 30
            
          resources:
            requests:
              memory: "32Gi"
              cpu: "8"
            limits:
              memory: "64Gi"
              cpu: "16"
              
          volumeMounts:
            - name: elasticsearch-data
              mountPath: /usr/share/elasticsearch/data
            - name: elasticsearch-config
              mountPath: /usr/share/elasticsearch/config/elasticsearch.yml
              subPath: elasticsearch.yml
            - name: certs
              mountPath: /usr/share/elasticsearch/config/certs
              readOnly: true
              
      volumes:
        - name: elasticsearch-config
          configMap:
            name: elasticsearch-config
        - name: certs
          secret:
            secretName: elasticsearch-certs
            
  volumeClaimTemplates:
    - metadata:
        name: elasticsearch-data
      spec:
        accessModes: ["ReadWriteOnce"]
        storageClassName: fast-ssd
        resources:
          requests:
            storage: 2Ti
```

## 网络安全配置

```yaml
# 网络策略配置
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: elastic-stack-policy
  namespace: elastic-stack
spec:
  podSelector:
    matchLabels:
      app: elasticsearch
  policyTypes:
    - Ingress
    - Egress
    
  ingress:
    # 允许Kibana访问
    - from:
        - podSelector:
            matchLabels:
              app: kibana
      ports:
        - protocol: TCP
          port: 9200
          
    # 允许Beats访问
    - from:
        - namespaceSelector:
            matchLabels:
              name: monitoring
      ports:
        - protocol: TCP
          port: 9200
          
    # 允许内部节点通信
    - from:
        - podSelector:
            matchLabels:
              app: elasticsearch
      ports:
        - protocol: TCP
          port: 9300
          
  egress:
    # 允许DNS查询
    - to:
        - namespaceSelector:
            matchLabels:
              name: kube-system
      ports:
        - protocol: UDP
          port: 53
        - protocol: TCP
          port: 53
          
    # 允许外部存储访问（如S3）
    - to:
        - ipBlock:
            cidr: 0.0.0.0/0
      ports:
        - protocol: TCP
          port: 443
```

## 安全加固配置

## RBAC权限管理

```yaml
# Elasticsearch角色和用户配置
roles:
  admin_role:
    cluster: 
      - all
    indices:
      - names: ["*"]
        privileges: ["all"]
    applications:
      - application: "kibana-.kibana"
        privileges: ["all"]
        
  monitoring_role:
    cluster:
      - monitor
      - manage_index_templates
    indices:
      - names: [".monitoring*", "metricbeat-*", "filebeat-*"]
        privileges: ["read", "view_index_metadata"]
        
  log_reader_role:
    cluster: []
    indices:
      - names: ["filebeat-*", "logstash-*"]
        privileges: ["read", "view_index_metadata"]
        
  apm_writer_role:
    cluster: []
    indices:
      - names: ["apm-*"]
        privileges: ["write", "create_index", "manage"]
        
users:
  elastic_admin:
    password: "${ADMIN_PASSWORD}"
    roles: ["admin_role", "kibana_admin"]
    
  monitoring_user:
    password: "${MONITORING_PASSWORD}"
    roles: ["monitoring_role"]
    
  log_collector:
    password: "${LOG_COLLECTOR_PASSWORD}"
    roles: ["log_reader_role"]
    
  apm_server:
    password: "${APM_SERVER_PASSWORD}"
    roles: ["apm_writer_role"]
```

## TLS证书管理

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
#!/bin/bash
# Elasticsearch TLS证书生成脚本

# 创建CA
openssl genrsa -out ca.key 4096
openssl req -new -x509 -key ca.key -out ca.crt -days 3650 -subj "/CN=Elasticsearch CA"

# 为每个节点生成证书
NODES=("es-master-01" "es-master-02" "es-master-03" "es-data-01" "es-data-02" "es-coord-01")

for node in "${NODES[@]}"; do
    # 生成节点私钥
    openssl genrsa -out ${node}.key 2048
    
    # 生成证书签名请求
    openssl req -new -key ${node}.key -out ${node}.csr -subj "/CN=${node}"
    
    # 使用CA签署证书
    openssl x509 -req -in ${node}.csr -CA ca.crt -CAkey ca.key -CAcreateserial -out ${node}.crt -days 365
    
    # 创建PKCS#12格式证书（用于Java客户端）
    openssl pkcs12 -export -in ${node}.crt -inkey ${node}.key -out ${node}.p12 -name ${node} -CAfile ca.crt -caname root -password pass:${node}_password
done

# 生成HTTP层证书
openssl genrsa -out http.key 2048
openssl req -new -key http.key -out http.csr -subj "/CN=elasticsearch-http"
openssl x509 -req -in http.csr -CA ca.crt -CAkey ca.key -CAcreateserial -out http.crt -days 365

# 创建Kubernetes Secret
kubectl create secret generic elasticsearch-certs \
    --from-file=ca.crt \
    --from-file=es-master-01.crt \
    --from-file=es-master-01.key \
    --from-file=es-master-02.crt \
    --from-file=es-master-02.key \
    --from-file=es-master-03.crt \
    --from-file=es-master-03.key \
    --from-file=http.crt \
    --from-file=http.key \
    -n elastic-stack
```
---

<!-- chunk: 日志分析与处理 -->## 日志分析与处理

## Logstash管道配置

## 复杂日志处理管道

```ruby
# Logstash企业级配置
input {
  beats {
    port => 5044
    ssl => true
    ssl_certificate => "/etc/logstash/certs/logstash.crt"
    ssl_key => "/etc/logstash/certs/logstash.key"
  }
  
  kafka {
    bootstrap_servers => "kafka-01:9092,kafka-02:9092,kafka-03:9092"
    topics => ["application-logs", "system-logs", "security-logs"]
    group_id => "logstash-consumer"
    codec => json
  }
}

filter {
  # 通用字段标准化
  mutate {
    add_field => {
      "[@metadata][ingest_timestamp]" => "%{@timestamp}"
      "[fields][collector]" => "logstash"
    }
    
    rename => {
      "message" => "[log][original]"
      "host" => "[host][name]"
    }
  }
  
  # 时间戳处理
  date {
    match => [ "[log][timestamp]", "ISO8601", "yyyy-MM-dd HH:mm:ss", "UNIX_MS" ]
    target => "@timestamp"
    timezone => "Asia/Shanghai"
  }
  
  # JSON日志解析
  json {
    source => "[log][original]"
    skip_on_invalid_json => true
    target => "[json]"
  }
  
  # 用户代理解析
  useragent {
    source => "[http][request][headers][user-agent]"
    target => "[user_agent]"
    regexes => "/etc/logstash/regexes.yaml"
  }
  
  # 地理位置解析
  geoip {
    source => "[client][ip]"
    target => "[geoip]"
    database => "/etc/logstash/GeoLite2-City.mmdb"
  }
  
  # 应用程序特定处理
  if [fields][service] == "nginx" {
    grok {
      match => {
        "[log][original]" => "%{IPORHOST:[nginx][access][remote_ip]} - %{DATA:[nginx][access][user_name]} \[%{HTTPDATE:[nginx][access][time]}\] \"%{WORD:[nginx][access][method]} %{DATA:[nginx][access][url]} HTTP/%{NUMBER:[nginx][access][http_version]}\" %{NUMBER:[nginx][access][response_code]} %{NUMBER:[nginx][access][body_sent][bytes]} \"%{DATA:[nginx][access][referrer]}\" \"%{DATA:[nginx][access][agent]}\""
      }
    }
    
    mutate {
      convert => {
        "[nginx][access][response_code]" => "integer"
        "[nginx][access][body_sent][bytes]" => "integer"
      }
    }
  }
  
  # 异常检测和丰富
  ruby {
    code => "
      # 计算响应时间等级
      if event.get('[nginx][access][body_sent][bytes]') && event.get('[nginx][access][body_sent][bytes]') > 1048576
        event.set('[nginx][access][size_category]', 'large')
      elsif event.get('[nginx][access][body_sent][bytes]') && event.get('[nginx][access][body_sent][bytes]') > 102400
        event.set('[nginx][access][size_category]', 'medium')
      else
        event.set('[nginx][access][size_category]', 'small')
      end
      
      # 标记异常访问
      if event.get('[nginx][access][response_code]') && event.get('[nginx][access][response_code]').to_i >= 500
        event.set('[error][type]', 'server_error')
      elsif event.get('[nginx][access][response_code]') && event.get('[nginx][access][response_code]').to_i >= 400
        event.set('[error][type]', 'client_error')
      end
    "
  }
  
  # 数据脱敏
  mutate {
    replace => {
      "[user][password]" => "[MASKED]"
      "[credit_card][number]" => "[MASKED]"
    }
  }
}

output {
  # 主要输出到Elasticsearch
  elasticsearch {
    hosts => ["https://es-coord-01:9200", "https://es-coord-02:9200"]
    user => "${ELASTIC_USERNAME}"
    password => "${ELASTIC_PASSWORD}"
    ssl_certificate_verification => true
    cacert => "/etc/logstash/certs/ca.crt"
    
    index => "%{[@metadata][beat]}-%{[@metadata][version]}-%{+YYYY.MM.dd}"
    template_name => "logstash"
    template => "/etc/logstash/templates/logstash-template.json"
    template_overwrite => true
    
    # 批量处理优化
    document_id => "%{[@metadata][fingerprint]}"
    action => "index"
    retry_max_interval => 60
    retry_max_times => 3
  }
  
  # 备份输出到对象存储
  s3 {
    access_key_id => "${AWS_ACCESS_KEY_ID}"
    secret_access_key => "${AWS_SECRET_ACCESS_KEY}"
    region => "cn-north-1"
    bucket => "log-backup-enterprise"
    time_file => 10
    size_file => 10485760
    codec => "json_lines"
    prefix => "logs/%{+YYYY}/%{+MM}/%{+dd}/"
  }
  
  # 实时告警输出
  if [error][type] == "server_error" or [nginx][access][response_code] >= 500 {
    kafka {
      bootstrap_servers => "kafka-alerts:9092"
      topic_id => "critical-alerts"
      codec => json
    }
  }
}
```

## 索引模板配置

```json
{
  "index_patterns": ["filebeat-*", "logstash-*"],
  "template": {
    "settings": {
      "number_of_shards": 3,
      "number_of_replicas": 1,
      "refresh_interval": "30s",
      "blocks": {
        "read_only_allow_delete": "false"
      },
      "analysis": {
        "analyzer": {
          "log_analyzer": {
            "type": "custom",
            "tokenizer": "whitespace",
            "filter": ["lowercase", "stop"]
          }
        }
      }
    },
    "mappings": {
      "dynamic_templates": [
        {
          "strings_as_keywords": {
            "match_mapping_type": "string",
            "mapping": {
              "type": "text",
              "analyzer": "log_analyzer",
              "fields": {
                "keyword": {
                  "type": "keyword",
                  "ignore_above": 256
                }
              }
            }
          }
        }
      ],
      "properties": {
        "@timestamp": {
          "type": "date"
        },
        "message": {
          "type": "text",
          "analyzer": "log_analyzer"
        },
        "host.name": {
          "type": "keyword"
        },
        "service.name": {
          "type": "keyword"
        },
        "log.level": {
          "type": "keyword"
        },
        "http.response.status_code": {
          "type": "short"
        },
        "geoip.location": {
          "type": "geo_point"
        }
      }
    }
  },
  "composed_of": ["logs-mappings", "logs-settings"],
  "priority": 500,
  "version": 3,
  "_meta": {
    "description": "Default template for log data"
  }
}
```

---

<!-- chunk: 指标监控系统 -->## 指标监控系统

## Metricbeat高级配置

## 自定义指标收集

```yaml
# Metricbeat自定义模块配置
metricbeat.modules:
  # 自定义JVM监控
  - module: jolokia
    metricsets: ["jmx"]
    enabled: true
    period: 30s
    hosts: ["localhost:8778"]
    namespace: "jvm"
    jmx.mappings:
      - mbean: "java.lang:type=Memory"
        attributes:
          - attr: HeapMemoryUsage
            field: memory.heap
          - attr: NonHeapMemoryUsage
            field: memory.non_heap
            
      - mbean: "java.lang:type=Threading"
        attributes:
          - attr: ThreadCount
            field: threads.count
          - attr: PeakThreadCount
            field: threads.peak
            
      - mbean: "java.lang:type=OperatingSystem"
        attributes:
          - attr: SystemLoadAverage
            field: system.load.average
          - attr: ProcessCpuLoad
            field: process.cpu.load

  # 数据库监控
  - module: mysql
    metricsets: ["status", "performance"]
    enabled: true
    period: 30s
    hosts: ["tcp(127.0.0.1:3306)/"]
    username: "${MYSQL_MONITOR_USER}"
    password: "${MYSQL_MONITOR_PASSWORD}"
    
    # 自定义SQL查询监控
    sql_queries:
      - name: "slow_queries"
        query: "SHOW GLOBAL STATUS LIKE 'Slow_queries'"
        fields:
          - name: "slow_queries_count"
            column: "Value"
            type: "long"
            
      - name: "connection_stats"
        query: "SHOW STATUS LIKE 'Threads_connected'"
        fields:
          - name: "current_connections"
            column: "Value"
            type: "long"

  # Redis监控
  - module: redis
    metricsets: ["info", "keyspace"]
    enabled: true
    period: 30s
    hosts: ["localhost:6379"]
    password: "${REDIS_PASSWORD}"
    
    # 自定义键空间分析
    keyspace_analysis:
      enabled: true
      sample_keys: 1000
      expiration_analysis: true
```

## 指标预处理和丰富

```javascript
// Metricbeat JavaScript处理器示例
processors:
  - script:
      lang: javascript
      id: calculate_derived_metrics
      source: >
        function process(event) {
          // 计算CPU使用率变化率
          var prev_cpu = event.Get("prev.system.cpu.total.norm.pct");
          var current_cpu = event.Get("system.cpu.total.norm.pct");
          
          if (prev_cpu !== null && current_cpu !== null) {
            var cpu_delta = Math.abs(current_cpu - prev_cpu);
            event.Put("system.cpu.delta", cpu_delta);
            
            // 标记CPU尖刺
            if (cpu_delta > 0.3) {
              event.Put("system.cpu.spike", true);
            }
          }
          
          // 计算内存压力指数
          var memory_used_pct = event.Get("system.memory.actual.used.pct");
          var swap_used_pct = event.Get("system.memory.swap.used.pct");
          
          if (memory_used_pct !== null && swap_used_pct !== null) {
            var memory_pressure = (memory_used_pct * 0.7) + (swap_used_pct * 0.3);
            event.Put("system.memory.pressure_index", memory_pressure);
          }
          
          // 网络异常检测
          var network_in_drops = event.Get("system.network.in.dropped");
          var network_out_drops = event.Get("system.network.out.dropped");
          
          if ((network_in_drops !== null && network_in_drops > 100) || 
              (network_out_drops !== null && network_out_drops > 100)) {
            event.Put("network.anomaly", true);
          }
        }
```

---

<!-- chunk: APM应用性能监控 -->## APM应用性能监控

## APM Server配置

## 高级APM配置

```yaml
# APM Server企业级配置
apm-server:
  host: "0.0.0.0:8200"
  max_connections: 1000
  idle_timeout: 45s
  read_timeout: 30s
  write_timeout: 30s
  shutdown_timeout: 5s
  
  ssl:
    enabled: true
    certificate: "/etc/apm-server/certs/apm-server.crt"
    key: "/etc/apm-server/certs/apm-server.key"
    certificate_authorities: ["/etc/apm-server/certs/ca.crt"]
    client_authentication: "optional"

  rum:
    enabled: true
    allow_origins: ["*"]
    allow_headers: ["Content-Type", "Authorization"]
    rate_limit:
      event_limit: 300
      ip_limit: 1000
      
  kibana:
    enabled: true
    host: "kibana:5601"
    username: "${KIBANA_USERNAME}"
    password: "${KIBANA_PASSWORD}"
    
  elasticsearch:
    hosts: ["https://es-coord-01:9200", "https://es-coord-02:9200"]
    username: "${ELASTIC_USERNAME}"
    password: "${ELASTIC_PASSWORD}"
    ssl.certificate_authorities: ["/etc/apm-server/certs/ca.crt"]
    
    bulk_max_size: 2048
    flush_interval: 1s
    compression_level: 3
    
  # 采样配置
  sampling:
    tail:
      enabled: true
      interval: 1m
      policies:
        - service:
            name: "critical-service"
          sample_rate: 1.0
          
        - service:
            name: "standard-service"
          sample_rate: 0.1
          
        - trace:
            outcome: "failure"
          sample_rate: 1.0

  # 数据丰富
  data_streams:
    enabled: true
    namespace: "default"
    
  # 外部监控集成
  monitoring:
    enabled: true
    elasticsearch:
      hosts: ["https://es-coord-01:9200"]
      username: "${MONITORING_USERNAME}"
      password: "${MONITORING_PASSWORD}"
```

## 应用程序APM集成

## Java应用APM配置

```java
// Spring Boot应用APM配置示例
@Configuration
public class ApmConfiguration {
    
    @Bean
    public ElasticApmAgent elasticApmAgent() {
        // 配置APM代理
        System.setProperty("elastic.apm.service_name", "user-service");
        System.setProperty("elastic.apm.server_urls", "https://apm-server:8200");
        System.setProperty("elastic.apm.secret_token", "${APM_SECRET_TOKEN}");
        System.setProperty("elastic.apm.application_packages", "com.company.userservice");
        System.setProperty("elastic.apm.environment", "production");
        System.setProperty("elastic.apm.log_level", "INFO");
        
        // 高级配置
        System.setProperty("elastic.apm.span_frames_min_duration", "5ms");
        System.setProperty("elastic.apm.transaction_max_spans", "500");
        System.setProperty("elastic.apm.central_config", "true");
        System.setProperty("elastic.apm.metrics_interval", "30s");
        
        return new ElasticApmAgent();
    }
    
    @Bean
    public WebMvcConfigurer webMvcConfigurer() {
        return new WebMvcConfigurer() {
            @Override
            public void addInterceptors(InterceptorRegistry registry) {
                registry.addInterceptor(new ApmTransactionInterceptor());
            }
        };
    }
}

// 自定义Span注解
@Retention(RetentionPolicy.RUNTIME)
@Target(ElementType.METHOD)
public @interface TracedOperation {
    String value() default "";
    String type() default "business";
}

// AOP切面处理
@Aspect
@Component
public class ApmTracingAspect {
    
    @Around("@annotation(tracedOperation)")
    public Object traceMethod(ProceedingJoinPoint joinPoint, TracedOperation tracedOperation) throws Throwable {
        Span span = ElasticApm.currentSpan()
            .startSpan(tracedOperation.type(), "method", tracedOperation.value());
            
        try {
            span.setName(joinPoint.getSignature().getName());
            span.activate();
            
            // 添加方法参数作为标签
            Object[] args = joinPoint.getArgs();
            for (int i = 0; i < args.length; i++) {
                span.addLabel("param_" + i, String.valueOf(args[i]));
            }
            
            Object result = joinPoint.proceed();
            span.addLabel("result_type", result != null ? result.getClass().getSimpleName() : "null");
            
            return result;
        } catch (Exception e) {
            span.captureException(e);
            throw e;
        } finally {
            span.end();
        }
    }
}
```

## 数据库查询监控

```java
// 数据库查询性能监控
@Component
public class DatabasePerformanceMonitor {
    
    @EventListener
    public void handleSlowQuery(SlowQueryEvent event) {
        Transaction transaction = ElasticApm.currentTransaction();
        
        if (transaction != null) {
            Span span = transaction.startSpan("db", "mysql", "query");
            span.setName("Slow Query Detection");
            span.addLabel("sql", event.getSql());
            span.addLabel("execution_time_ms", event.getExecutionTime());
            span.addLabel("rows_affected", event.getRowsAffected());
            
            // 设置性能阈值
            if (event.getExecutionTime() > 1000) {
                span.setOutcome(Outcome.FAILURE);
                transaction.setOutcome(Outcome.FAILURE);
                span.addLabel("performance_issue", "slow_query");
            }
            
            span.end();
        }
    }
    
    // JPA查询拦截器
    @Component
    public static class JpaQueryInterceptor implements StatementInspector {
        
        @Override
        public String inspect(String sql) {
            // 记录查询性能
            long startTime = System.currentTimeMillis();
            
            return sql; // 返回原始SQL，不影响执行
        }
    }
}
```

---

<!-- chunk: 安全信息与事件管理 -->## 安全信息与事件管理

## SIEM配置

## 威胁检测规则

```yaml
# Elastic SIEM威胁检测规则
apiVersion: detection.k8s.elastic.co/v1alpha1
kind: DetectionRule
metadata:
  name: suspicious-login-patterns
  namespace: security
spec:
  name: "可疑登录模式检测"
  description: "检测异常的登录行为和潜在的安全威胁"
  enabled: true
  risk_score: 73
  severity: high
  type: detection
  language: kuery
  
  query: |
    event.action:"user_login" and 
    user.name:* and 
    (
      # 异常时间登录
      (event.created:[now-1h TO now] and 
       (event.created.hour:< 6 or event.created.hour:> 22)) or
       
      # 多地理位置登录
      (geoip.country_iso_code:* and 
       geoip.country_iso_code != geoip.previous_country_iso_code) or
       
      # 失败登录尝试过多
      (event.outcome:"failure" and 
       event.action_count:> 5)
    )
    
  threat:
    - framework: MITRE ATT&CK
      tactic:
        id: TA0006
        name: Credential Access
        reference: https://attack.mitre.org/tactics/TA0006/
      technique:
        - id: T1110
          name: Brute Force
          reference: https://attack.mitre.org/techniques/T1110/
          
  schedule:
    interval: 5m
    lookback: 1h
    
  actions:
    - action_type: "index"
      destination_index: "security-alerts"
      
    - action_type: "webhook"
      url: "https://security-orchestrator/webhook/incident"
      payload:
        incident_type: "suspicious_login"
        priority: "high"
        assign_to: "security-team"
```

## 安全日志分析

```python
# 安全日志分析Python脚本
from elasticsearch import Elasticsearch
import pandas as pd
from datetime import datetime, timedelta
import numpy as np

class SecurityAnalyzer:
    def __init__(self, es_client):
        self.es = es_client
        self.index_pattern = "logs-security-*"
        
    def detect_bruteforce_attacks(self, time_window_hours=24):
        """检测暴力破解攻击"""
        query = {
            "bool": {
                "must": [
                    {"term": {"event.category": "authentication"}},
                    {"term": {"event.outcome": "failure"}},
                    {"range": {
                        "@timestamp": {
                            "gte": f"now-{time_window_hours}h/h",
                            "lt": "now/h"
                        }
                    }}
                ]
            }
        }
        
        # 聚合分析
        aggs = {
            "by_source_ip": {
                "terms": {
                    "field": "source.ip",
                    "size": 1000
                },
                "aggs": {
                    "failure_count": {
                        "cardinality": {
                            "field": "user.name.keyword"
                        }
                    },
                    "unique_users": {
                        "cardinality": {
                            "field": "user.name.keyword"
                        }
                    },
                    "timeline": {
                        "date_histogram": {
                            "field": "@timestamp",
                            "calendar_interval": "1h"
                        }
                    }
                }
            }
        }
        
        response = self.es.search(
            index=self.index_pattern,
            body={
                "query": query,
                "aggs": aggs,
                "size": 0
            }
        )
        
        # 分析结果
        threats = []
        for bucket in response['aggregations']['by_source_ip']['buckets']:
            failure_count = bucket['failure_count']['value']
            unique_users = bucket['unique_users']['value']
            
            # 判断是否为暴力破解
            if failure_count > 10 and unique_users > 5:
                threats.append({
                    'source_ip': bucket['key'],
                    'failure_attempts': failure_count,
                    'affected_users': unique_users,
                    'risk_score': min(100, failure_count * 2 + unique_users * 5),
                    'timestamp': datetime.now().isoformat()
                })
                
        return threats
        
    def analyze_lateral_movement(self):
        """分析横向移动行为"""
        query = {
            "bool": {
                "must": [
                    {"terms": {"event.action": ["user_login", "session_start"]}},
                    {"exists": {"field": "host.name"}},
                    {"range": {
                        "@timestamp": {
                            "gte": "now-7d/d",
                            "lt": "now/d"
                        }
                    }}
                ]
            }
        }
        
        aggs = {
            "user_sessions": {
                "terms": {
                    "field": "user.name.keyword",
                    "size": 1000
                },
                "aggs": {
                    "hosts": {
                        "cardinality": {
                            "field": "host.name.keyword"
                        }
                    },
                    "distinct_hosts": {
                        "terms": {
                            "field": "host.name.keyword",
                            "size": 100
                        }
                    }
                }
            }
        }
        
        response = self.es.search(
            index=self.index_pattern,
            body={
                "query": query,
                "aggs": aggs,
                "size": 0
            }
        )
        
        # 检测异常的主机访问模式
        suspicious_users = []
        for bucket in response['aggregations']['user_sessions']['buckets']:
            host_count = bucket['hosts']['value']
            
            if host_count > 10:  # 单用户访问超过10台主机
                suspicious_users.append({
                    'user': bucket['key'],
                    'hosts_accessed': host_count,
                    'host_list': [h['key'] for h in bucket['distinct_hosts']['buckets']],
                    'anomaly_score': host_count
                })
                
        return suspicious_users

# 使用示例
if __name__ == "__main__":
    es = Elasticsearch(['https://es-coord-01:9200'], 
                      http_auth=('username', 'password'),
                      verify_certs=True)
    
    analyzer = SecurityAnalyzer(es)
    
    # 检测暴力破解
    bruteforce_threats = analyzer.detect_bruteforce_attacks()
    print(f"发现 {len(bruteforce_threats)} 个暴力破解威胁")
    
    # 分析横向移动
    lateral_movements = analyzer.analyze_lateral_movement()
    print(f"发现 {len(lateral_movements)} 个可疑横向移动行为")
```

---

<!-- chunk: 可视化与告警 -->## 可视化与告警

## Kibana仪表板配置

## 高级可视化配置

```json
{
  "dashboard": {
    "id": "enterprise-observability-dashboard",
    "title": "企业级可观测性总览",
    "description": "综合展示基础设施、应用性能和安全状态",
    "panels": [
      {
        "id": "system-health-panel",
        "type": "visualization",
        "gridData": {
          "x": 0,
          "y": 0,
          "w": 24,
          "h": 12
        },
        "embeddableConfig": {
          "visState": {
            "title": "系统健康状态",
            "type": "timelion",
            "params": {
              "expression": ".es(index=metricbeat-*, timefield='@timestamp', metric='avg:system.cpu.user.pct').label('CPU使用率'), .es(index=metricbeat-*, timefield='@timestamp', metric='avg:system.memory.actual.used.pct').label('内存使用率'), .es(index=metricbeat-*, timefield='@timestamp', metric='avg:system.disk.used.pct').label('磁盘使用率')"
            }
          }
        }
      },
      {
        "id": "application-performance-panel",
        "type": "visualization",
        "gridData": {
          "x": 0,
          "y": 12,
          "w": 24,
          "h": 12
        },
        "embeddableConfig": {
          "visState": {
            "title": "应用性能监控",
            "type": "lens",
            "references": [
              {
                "id": "apm-transaction-duration",
                "name": "indexpattern-datasource-layer-0",
                "type": "index-pattern"
              }
            ],
            "state": {
              "visualization": {
                "layers": [
                  {
                    "layerId": "layer_0",
                    "layerType": "data",
                    "state": {
                      "columns": [
                        {
                          "columnId": "x-axis-column",
                          "sourceField": "@timestamp"
                        },
                        {
                          "columnId": "y-axis-column",
                          "sourceField": "transaction.duration.us"
                        }
                      ]
                    }
                  }
                ]
              }
            }
          }
        }
      }
    ],
    "options": {
      "useMargins": true,
      "hidePanelTitles": false
    },
    "timeRestore": true,
    "timeTo": "now",
    "timeFrom": "now-24h",
    "refreshInterval": {
      "pause": false,
      "value": 30000
    }
  }
}
```

## 告警规则配置

```yaml
# Watcher告警配置
PUT _watcher/watch/system-resource-alert
{
  "trigger": {
    "schedule": {
      "interval": "5m"
    }
  },
  "input": {
    "search": {
      "request": {
        "search_type": "query_then_fetch",
        "indices": ["metricbeat-*"],
        "body": {
          "size": 0,
          "query": {
            "bool": {
              "filter": [
                {
                  "range": {
                    "@timestamp": {
                      "gte": "now-5m"
                    }
                  }
                }
              ]
            }
          },
          "aggs": {
            "hosts": {
              "terms": {
                "field": "host.name.keyword",
                "size": 100
              },
              "aggs": {
                "avg_cpu": {
                  "avg": {
                    "field": "system.cpu.user.pct"
                  }
                },
                "avg_memory": {
                  "avg": {
                    "field": "system.memory.actual.used.pct"
                  }
                }
              }
            }
          }
        }
      }
    }
  },
  "condition": {
    "script": {
      "source": """
        def alerts = [];
        for (bucket in ctx.payload.aggregations.hosts.buckets) {
          if (bucket.avg_cpu.value > 0.85 || bucket.avg_memory.value > 0.90) {
            alerts.add([
              'host': bucket.key,
              'cpu_usage': bucket.avg_cpu.value,
              'memory_usage': bucket.avg_memory.value
            ]);
          }
        }
        ctx.alerts = alerts;
        return alerts.size() > 0;
      """
    }
  },
  "actions": {
    "send_email": {
      "email": {
        "to": ["ops-team@company.com"],
        "subject": "系统资源告警 - {{ctx.alerts.size()}}台主机",
        "body": """
          告警详情:
          
          {% for alert in ctx.alerts %}
          主机: {{alert.host}}
          CPU使用率: {{alert.cpu_usage}}%
          内存使用率: {{alert.memory_usage}}%
          
          {% endfor %}
          
          请及时处理资源瓶颈问题。
        """
      }
    },
    "create_incident": {
      "webhook": {
        "scheme": "https",
        "host": "incident-management.company.com",
        "port": 443,
        "method": "post",
        "path": "/api/incidents",
        "body": "{{#toJson}}ctx{{/toJson}}"
      }
    }
  }
}
```

---

<!-- chunk: 性能优化策略 -->## 性能优化策略

## Elasticsearch性能调优

## 索引优化配置

```yaml
# 索引性能优化配置
index_settings:
  # 分片策略
  number_of_shards: 6
  number_of_replicas: 1
  
  # 刷新间隔优化
  refresh_interval: 30s
  
  # 合并策略
  merge.policy:
    max_merge_at_once: 10
    segments_per_tier: 10
    max_merged_segment: 5gb
    
  # 缓存配置
  requests.cache.enable: true
  fielddata.cache.size: 40%
  
  # 查询缓存
  queries.cache.enabled: true
  
  # Translog配置
  translog:
    durability: async
    sync_interval: 30s
    retention:
      size: 512mb
      age: 12h

# 特定索引模板优化
PUT _index_template/logs-optimized
{
  "index_patterns": ["logs-*"],
  "template": {
    "settings": {
      "number_of_shards": 3,
      "number_of_replicas": 1,
      "refresh_interval": "60s",
      "codec": "best_compression",
      "blocks": {
        "read_only_allow_delete": "false"
      }
    },
    "mappings": {
      "dynamic_templates": [
        {
          "strings_as_keywords": {
            "match_mapping_type": "string",
            "mapping": {
              "type": "keyword",
              "ignore_above": 1024
            }
          }
        }
      ],
      "properties": {
        "@timestamp": {
          "type": "date"
        },
        "message": {
          "type": "text",
          "index": false
        },
        "host.name": {
          "type": "keyword"
        }
      }
    }
  }
}
```

## 查询性能优化

```json
{
  "profile": true,
  "query": {
    "bool": {
      "must": [
        {
          "range": {
            "@timestamp": {
              "gte": "now-1h",
              "lte": "now"
            }
          }
        },
        {
          "term": {
            "service.name.keyword": "user-service"
          }
        }
      ],
      "filter": [
        {
          "exists": {
            "field": "error.message"
          }
        }
      ]
    }
  },
  "aggs": {
    "errors_by_type": {
      "terms": {
        "field": "error.type.keyword",
        "size": 10,
        "min_doc_count": 1
      },
      "aggs": {
        "top_errors": {
          "top_hits": {
            "size": 3,
            "_source": {
              "includes": ["@timestamp", "error.message", "trace.id"]
            },
            "sort": [
              {
                "@timestamp": {
                  "order": "desc"
                }
              }
            ]
          }
        }
      }
    }
  },
  "highlight": {
    "fields": {
      "error.message": {}
    }
  }
}
```

## 集群健康监控

```bash
#!/bin/bash
# Elasticsearch集群健康检查脚本

CLUSTER_URL="https://es-coord-01:9200"
AUTH_HEADER="Authorization: Basic $(echo -n 'username:password' | base64)"

# 检查集群健康状态
cluster_health=$(curl -s -k -H "$AUTH_HEADER" "$CLUSTER_URL/_cluster/health")
status=$(echo $cluster_health | jq -r '.status')

echo "集群状态: $status"

# 检查节点状态
nodes_stats=$(curl -s -k -H "$AUTH_HEADER" "$CLUSTER_URL/_nodes/stats")
node_count=$(echo $nodes_stats | jq '.nodes | length')

echo "节点数量: $node_count"

# 检查索引状态
indices_stats=$(curl -s -k -H "$AUTH_HEADER" "$CLUSTER_URL/_cat/indices?v&health=red")
if [ -n "$indices_stats" ]; then
    echo "红色索引:"
    echo "$indices_stats"
fi

# 检查磁盘使用情况
disk_usage=$(curl -s -k -H "$AUTH_HEADER" "$CLUSTER_URL/_cat/allocation?v")
echo "磁盘分配情况:"
echo "$disk_usage"

# 检查JVM堆内存使用
heap_usage=$(curl -s -k -H "$AUTH_HEADER" "$CLUSTER_URL/_nodes/stats/jvm" | jq '.nodes[].jvm.mem')
echo "JVM内存使用:"
echo "$heap_usage"
```

---

<!-- chunk: 最佳实践总结 -->## 最佳实践总结

## 部署架构最佳实践

```yaml
# 生产环境推荐配置
production_recommendations:
  cluster_sizing:
    master_nodes: 3
    data_nodes: 6+
    coordinating_nodes: 2+
    ml_nodes: 2
    
  hardware_requirements:
    master_nodes:
      cpu: 4 cores
      memory: 16GB
      storage: 100GB SSD
      
    data_hot_nodes:
      cpu: 16 cores
      memory: 128GB
      storage: 2TB NVMe
      
    data_warm_nodes:
      cpu: 8 cores
      memory: 64GB
      storage: 4TB HDD
      
  network_configuration:
    bandwidth: 10Gbps minimum
    latency: < 2ms between nodes
    mtu: 9000 (jumbo frames)
    
  backup_strategy:
    snapshot_frequency: every_6_hours
    retention_policy: 30_days_local_90_days_remote
    verification_schedule: daily
```

## 监控和维护

## 日常运维检查清单

- [ ] 集群健康状态检查
- [ ] 节点资源使用率监控
- [ ] 索引分片分布均衡
- [ ] 磁盘空间使用情况
- [ ] JVM垃圾回收性能
- [ ] 查询性能基准测试
- [ ] 备份完整性验证
- [ ] 安全配置审查

## 性能基准测试

```bash
#!/bin/bash
# Elasticsearch性能基准测试脚本

ES_HOST="https://es-coord-01:9200"
INDEX_NAME="benchmark-test-$(date +%Y%m%d)"
ITERATIONS=10000

# 创建测试索引
curl -X PUT "$ES_HOST/$INDEX_NAME" -H "Content-Type: application/json" -d '
{
  "settings": {
    "number_of_shards": 3,
    "number_of_replicas": 1,
    "refresh_interval": "-1"
  },
  "mappings": {
    "properties": {
      "timestamp": {"type": "date"},
      "message": {"type": "text"},
      "value": {"type": "double"}
    }
  }
}'

# 批量索引测试
echo "开始批量索引测试..."
start_time=$(date +%s)

for i in $(seq 1 $ITERATIONS); do
    bulk_data='{"index":{"_index":"'$INDEX_NAME'"}}\n{"timestamp":"'$(( $(date +%s) * 1000 ))'","message":"Test message '$i'","value":'$i'}\n'
    curl -s -X POST "$ES_HOST/_bulk" -H "Content-Type: application/x-ndjson" -d "$bulk_data" > /dev/null
done

# 刷新索引
curl -X POST "$ES_HOST/$INDEX_NAME/_refresh"

end_time=$(date +%s)
duration=$((end_time - start_time))
rate=$((ITERATIONS / duration))

echo "索引完成: $ITERATIONS 文档用时 $duration 秒，速率: $rate docs/sec"

# 查询性能测试
echo "开始查询性能测试..."

query_times=()
for i in {1..100}; do
    start=$(date +%s%3N)
    curl -s -X GET "$ES_HOST/$INDEX_NAME/_search" -H "Content-Type: application/json" -d '
    {
      "query": {
        "range": {
          "value": {
            "gte": 1000,
            "lte": 5000
          }
        }
      },
      "size": 100
    }' > /dev/null
    
    end=$(date +%s%3N)
    query_times+=($((end - start)))
done

# 计算平均查询时间
sum=0
for time in "${query_times[@]}"; do
    sum=$((sum + time))
done
avg_time=$((sum / ${#query_times[@]}))

echo "平均查询时间: ${avg_time}ms"

# 清理测试数据
curl -X DELETE "$ES_HOST/$INDEX_NAME"
```

通过以上全面的Elastic Stack企业级可观测性平台实践，可以构建强大的日志分析、指标监控、APM追踪和安全分析一体化解决方案。

---

<!-- chunk: Obsidian 相关文档 -->## Obsidian 相关文档

- domain-20-enterprise-monitoring-alerting MOC
- [[可观测性/README.md|Domain 06: 企业级监控与告警 (Enterprise Monitoring & Alerting)]]
- [[可观测性/00-open-source-projects-index.md|Domain-20 企业监控与告警 — 开源项目索引]]
- Prometheus企业级监控系统深度实践
- Grafana Enterprise Observability Platform 深度实践
- OpenTelemetry分布式追踪与可观测性深度实践
- Thanos Enterprise Metrics Federation and Long-term Storage
- Datadog企业级APM深度实践
- Datadog 企业级监控平台深度实践
- Elastic Stack企业级日志分析深度实践
- Zabbix Enterprise Monitoring Platform 深度实践
- New Relic Enterprise APM Platform 深度实践

## See Also

- 05-datadog-enterprise-monitoring
- 06-elastic-stack-enterprise-logging
- 07-zabbix-enterprise-monitoring
- 08-new-relic-enterprise-apm

- [[可观测性/README.md|返回目录]]

## Related

- [[生态参考/领域索引/observability-index.md|Observability 可观测性知识图谱索引]]


<!-- risk-assessed -->
