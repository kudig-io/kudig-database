---
title: Elastic Stack企业级日志分析深度实践
description: '# Elastic Stack企业级日志分析深度实践'
summary: '本文档深入探讨Elastic Stack企业级日志分析系统的架构设计、部署实践和运维管理，基于大规模企业环境的实践经验，提供从日志采集到智能分析的完整技术指南，帮助企业构建高效、可靠的日志治理体系。'
category: enterprise-monitoring-alerting
tags:
- k8s
- monitoring
- alerting
- prometheus
- docker
- redis
- kafka
- elasticsearch
- daemonset
- job
tier: supporting
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- SRE
- 监控工程师
- 运维工程师
estimated_read_time: 5min
intent_queries:
- Elastic Stack企业级日志分析深度实践 是什么
- 如何 Elastic Stack企业级日志分析深度实践
- Kubernetes 20 enterprise monitoring alerting 最佳实践
trigger_keywords:
- Elastic
- Stack企业级日志分析深度实践
- enterprise
- monitoring
- alerting
prerequisites:
- kubectl-basics
- observability-basics
- prometheus-basics
- monitoring-basics
- kafka-basics
- redis-basics
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
authors:
- name: KUDIG Team
  role: contributor
cross_refs:
- type: cheatsheet
  path: ../系统基础/topic-cheat-sheet/promql.md
  label: '速查卡: promql'
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Elastic Stack企业级日志分析深度实践

> **作者**: 企业级日志分析架构专家 | **版本**: v1.0 | **更新时间**: 2026-02-07
> **适用场景**: 企业级日志治理与实时分析 | **复杂度**: ⭐⭐⭐⭐⭐

<!-- chunk: 🎯 摘要 -->## 🎯 摘要

本文档深入探讨Elastic Stack企业级日志分析系统的架构设计、部署实践和运维管理，基于大规模企业环境的实践经验，提供从日志采集到智能分析的完整技术指南，帮助企业构建高效、可靠的日志治理体系。

<!-- chunk: 1. Elastic Stack架构深度解析 -->## 1. Elastic Stack架构深度解析

## 1.1 核心组件架构

```mermaid
graph TB
    subgraph "数据采集层"
        A[Filebeat] --> B[Log Files]
        C[Metricbeat] --> D[System Metrics]
        E[Winlogbeat] --> F[Windows Events]
        G[Packetbeat] --> H[Network Traffic]
        I[Auditbeat] --> J[Security Events]
        K[Functionbeat] --> L[Serverless Logs]
    end
    
    subgraph "数据处理层"
        M[Logstash] --> N[Data Parsing]
        O[Elasticsearch Ingest Node] --> P[Pipeline Processing]
        Q[Kafka] --> R[Buffer Queue]
        S[Redis] --> T[Cache Layer]
    end
    
    subgraph "存储检索层"
        U[Elasticsearch Cluster] --> V[Master Nodes]
        U --> W[Data Nodes]
        U --> X[Ingest Nodes]
        V --> Y[Cluster State]
        W --> Z[Shard Distribution]
        X --> AA[Document Indexing]
    end
    
    subgraph "分析展示层"
        AB[Kibana] --> AC[Dashboards]
        AD[Elasticsearch SQL] --> AE[Ad-hoc Queries]
        AF[Machine Learning] --> AG[Anomaly Detection]
        AH[Alerting] --> AI[Notification System]
    end
    
    subgraph "治理管控层"
        AJ[Security] --> AK[RBAC/RBAC]
        AL[Index Lifecycle] --> AM[ILM Policies]
        AN[Monitoring] --> AO[Elastic Stack Monitoring]
        AP[Backup] --> AQ[Snapshot Repository]
    end
```

## 1.2 企业级架构优势

## 1.2.1 高可用性保障
- **集群高可用**: 多节点集群部署确保服务连续性
- **数据冗余**: 副本分片机制保证数据安全性
- **问题自动恢复**: 自动故障检测和节点替换机制
- **跨区域复制**: 地理位置分散的数据备份策略

## 1.2.2 性能优化能力
- **水平扩展**: 支持动态增加节点处理更大数据量
- **智能分片**: 自动优化分片分布和大小
- **缓存机制**: 多层次缓存提升查询性能
- **压缩存储**: 高效的数据压缩算法节省存储空间

<!-- chunk: 2. 企业级部署架构 -->## 2. 企业级部署架构

## 2.1 多层高可用部署

## 2.1.1 Elasticsearch集群部署

```yaml
# elasticsearch-cluster.yaml
apiVersion: elasticsearch.k8s.elastic.co/v1
kind: Elasticsearch
metadata:
  name: enterprise-logs
  namespace: logging
spec:
  version: 8.11.3
  nodeSets:
  # 主节点集 - 专用集群管理
  - name: master-nodes
    count: 3
    config:
      node.roles: ["master"]
      cluster.routing.allocation.disk.threshold_enabled: true
      cluster.routing.allocation.disk.watermark.low: "85%"
      cluster.routing.allocation.disk.watermark.high: "90%"
      cluster.routing.allocation.disk.watermark.flood_stage: "95%"
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
          env:
          - name: ES_JAVA_OPTS
            value: "-Xms4g -Xmx4g"
        affinity:
          podAntiAffinity:
            requiredDuringSchedulingIgnoredDuringExecution:
            - labelSelector:
                matchLabels:
                  elasticsearch.k8s.elastic.co/cluster-name: enterprise-logs
                  elasticsearch.k8s.elastic.co/node-set-name: master-nodes
              topologyKey: kubernetes.io/hostname
    volumeClaimTemplates:
    - metadata:
        name: elasticsearch-data
      spec:
        accessModes:
        - ReadWriteOnce
        resources:
          requests:
            storage: 100Gi
        storageClassName: fast-ssd

  # 数据节点集 - 专用数据存储和搜索
  - name: data-nodes
    count: 6
    config:
      node.roles: ["data", "ingest"]
      indices.breaker.total.use_real_memory: true
      indices.breaker.fielddata.limit: "40%"
      indices.breaker.request.limit: "20%"
    podTemplate:
      spec:
        containers:
        - name: elasticsearch
          resources:
            requests:
              memory: 16Gi
              cpu: 4
            limits:
              memory: 32Gi
              cpu: 8
          env:
          - name: ES_JAVA_OPTS
            value: "-Xms16g -Xmx16g"
        initContainers:
        - name: sysctl
          securityContext:
            privileged: true
          command: ['sh', '-c', 'sysctl -w vm.max_map_count=262144']
    volumeClaimTemplates:
    - metadata:
        name: elasticsearch-data
      spec:
        accessModes:
        - ReadWriteOnce
        resources:
          requests:
            storage: 1Ti
        storageClassName: standard

  # 协调节点集 - 专用请求协调
  - name: coordinating-nodes
    count: 3
    config:
      node.roles: ["ingest"]
      search.remote.connect: false
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
          env:
          - name: ES_JAVA_OPTS
            value: "-Xms8g -Xmx8g"
```

## 2.1.2 Filebeat分布式采集

```yaml
# filebeat-deployment.yaml
apiVersion: apps/v1
kind: DaemonSet
metadata:
  name: filebeat
  namespace: logging
  labels:
    app: filebeat
spec:
  selector:
    matchLabels:
      app: filebeat
  template:
    metadata:
      labels:
        app: filebeat
    spec:
      serviceAccountName: filebeat
      terminationGracePeriodSeconds: 30
      hostNetwork: true
      dnsPolicy: ClusterFirstWithHostNet
      containers:
      - name: filebeat
        image: docker.elastic.co/beats/filebeat:8.11.3
        args: [
          "-c", "/etc/filebeat.yml",
          "-e",
        ]
        env:
        - name: ELASTICSEARCH_HOST
          value: "enterprise-logs-es-http.logging.svc.cluster.local"
        - name: ELASTICSEARCH_PORT
          value: "9200"
        - name: ELASTICSEARCH_USERNAME
          valueFrom:
            secretKeyRef:
              name: elasticsearch-credentials
              key: username
        - name: ELASTICSEARCH_PASSWORD
          valueFrom:
            secretKeyRef:
              name: elasticsearch-credentials
              key: password
        - name: NODE_NAME
          valueFrom:
            fieldRef:
              fieldPath: spec.nodeName
        securityContext:
          runAsUser: 0
        resources:
          limits:
            memory: 1Gi
            cpu: 1000m
          requests:
            cpu: 100m
            memory: 100Mi
        volumeMounts:
        - name: config
          mountPath: /etc/filebeat.yml
          readOnly: true
          subPath: filebeat.yml
        - name: data
          mountPath: /usr/share/filebeat/data
        - name: varlibdockercontainers
          mountPath: /var/lib/docker/containers
          readOnly: true
        - name: varlog
          mountPath: /var/log
          readOnly: true
        - name: timezone
          mountPath: /etc/localtime
          readOnly: true
      volumes:
      - name: config
        configMap:
          defaultMode: 0600
          name: filebeat-config
      - name: varlibdockercontainers
        hostPath:
          path: /var/lib/docker/containers
      - name: varlog
        hostPath:
          path: /var/log
      - name: timezone
        hostPath:
          path: /etc/localtime
      - name: data
        hostPath:
          path: /var/lib/filebeat-data
          type: DirectoryOrCreate
```

## 2.1.3 Filebeat配置优化

```yaml
# filebeat-config.yaml
apiVersion: v1
kind: ConfigMap
metadata:
  name: filebeat-config
  namespace: logging
  labels:
    app: filebeat
data:
  filebeat.yml: |-
    filebeat.inputs:
    # 应用日志采集
    - type: log
      enabled: true
      paths:
        - /var/log/containers/*_application_*.log
      processors:
        - add_kubernetes_metadata:
            host: ${NODE_NAME}
            matchers:
            - logs_path:
                logs_path: "/var/log/containers/"
        - decode_json_fields:
            fields: ["message"]
            process_array: false
            max_depth: 10
            target: ""
            overwrite_keys: true
        - drop_fields:
            fields: ["input", "agent", "ecs", "log", "stream"]
      fields:
        log_type: application
        environment: production
      
    # 系统日志采集
    - type: log
      enabled: true
      paths:
        - /var/log/*.log
        - /var/log/*/*.log
      exclude_files: ['\.gz$']
      multiline.pattern: '^\d{4}-\d{2}-\d{2}'
      multiline.negate: true
      multiline.match: after
      processors:
        - add_locale: ~
        - add_fields:
            target: ''
            fields:
              log_type: system
              environment: production
              
    # 安全日志采集
    - type: log
      enabled: true
      paths:
        - /var/log/auth.log
        - /var/log/secure
      processors:
        - add_fields:
            target: ''
            fields:
              log_type: security
              environment: production

    # 输出配置
    output.elasticsearch:
      hosts: ["${ELASTICSEARCH_HOST}:${ELASTICSEARCH_PORT}"]
      username: "${ELASTICSEARCH_USERNAME}"
      password: "${ELASTICSEARCH_PASSWORD}"
      index: "filebeat-%{[agent.version]}-%{+yyyy.MM.dd}"
      bulk_max_size: 2048
      worker: 2
      compression_level: 9
      timeout: 90
      max_retries: 3
      
    # 负载均衡
    output.elasticsearch.loadbalance: true
    
    # SSL配置
    output.elasticsearch.ssl:
      enabled: true
      certificate_authorities: ["/etc/pki/root/ca.pem"]
      certificate: "/etc/pki/client/cert.pem"
      key: "/etc/pki/client/key.pem"
      verification_mode: certificate
      
    # 队列配置
    queue.mem:
      events: 4096
      flush.min_events: 512
      flush.timeout: 5s
      
    # 日志配置
    logging.level: info
    logging.to_files: true
    logging.files:
      path: /var/log/filebeat
      name: filebeat
      keepfiles: 7
      permissions: 0644
      
    # 监控配置
    http.enabled: true
    http.host: localhost
    http.port: 5066
```

## 2.2 安全加固配置

## 2.2.1 网络安全策略

```yaml
# elastic-network-policy.yaml
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: elasticsearch-network-policy
  namespace: logging
spec:
  podSelector:
    matchLabels:
      elasticsearch.k8s.elastic.co/cluster-name: enterprise-logs
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
  # 允许Beats客户端访问
  - from:
    - namespaceSelector:
        matchLabels:
          name: application
    ports:
    - protocol: TCP
      port: 9200
  # 允许内部节点通信
  - from:
    - podSelector:
        matchLabels:
          elasticsearch.k8s.elastic.co/cluster-name: enterprise-logs
    ports:
    - protocol: TCP
      port: 9300
  egress:
  # 允许访问外部依赖
  - to:
    - ipBlock:
        cidr: 0.0.0.0/0
    ports:
    - protocol: TCP
      port: 53  # DNS
    - protocol: UDP
      port: 53  # DNS
  # 允许节点间通信
  - to:
    - podSelector:
        matchLabels:
          elasticsearch.k8s.elastic.co/cluster-name: enterprise-logs
    ports:
    - protocol: TCP
      port: 9300
```

## 2.2.2 认证授权配置

```yaml
# elastic-security-config.yaml
apiVersion: elasticsearch.k8s.elastic.co/v1
kind: Elasticsearch
metadata:
  name: enterprise-logs
  namespace: logging
spec:
  auth:
    fileRealm:
    - username: admin
      password: ${ADMIN_PASSWORD}
      roles: ["superuser"]
    - username: kibana_system
      password: ${KIBANA_PASSWORD}
      roles: ["kibana_system"]
    - username: beats_system
      password: ${BEATS_PASSWORD}
      roles: ["beats_system"]
    - username: logstash_system
      password: ${LOGSTASH_PASSWORD}
      roles: ["logstash_system"]
      
  secureSettings:
  - secretName: elasticsearch-secure-settings
  
  http:
    tls:
      certificate:
        secretName: elasticsearch-http-certs
      selfSignedCertificate:
        disabled: true
        
  transport:
    tls:
      certificate:
        secretName: elasticsearch-transport-certs
      selfSignedCertificate:
        disabled: true
```

<!-- chunk: 3. 企业级日志治理策略 -->## 3. 企业级日志治理策略

## 3.1 统一日志格式标准化

## 3.1.1 ECS(Elastic Common Schema)实施

```yaml
# ecs-mapping-template.yaml
apiVersion: v1
kind: ConfigMap
metadata:
  name: ecs-mapping-template
  namespace: logging
data:
  ecs-template.json: |
    {
      "index_patterns": ["filebeat-*", "metricbeat-*"],
      "settings": {
        "number_of_shards": 3,
        "number_of_replicas": 1,
        "refresh_interval": "30s",
        "blocks": {
          "read_only_allow_delete": "false"
        }
      },
      "mappings": {
        "_source": {
          "enabled": true
        },
        "properties": {
          "@timestamp": {
            "type": "date"
          },
          "labels": {
            "type": "object",
            "dynamic": true
          },
          "message": {
            "type": "text",
            "norms": false
          },
          "tags": {
            "type": "keyword"
          },
          "agent": {
            "properties": {
              "ephemeral_id": {
                "type": "keyword"
              },
              "id": {
                "type": "keyword"
              },
              "name": {
                "type": "keyword"
              },
              "type": {
                "type": "keyword"
              },
              "version": {
                "type": "keyword"
              }
            }
          },
          "container": {
            "properties": {
              "id": {
                "type": "keyword"
              },
              "image": {
                "properties": {
                  "name": {
                    "type": "keyword"
                  }
                }
              },
              "name": {
                "type": "keyword"
              },
              "runtime": {
                "type": "keyword"
              }
            }
          },
          "host": {
            "properties": {
              "architecture": {
                "type": "keyword"
              },
              "hostname": {
                "type": "keyword"
              },
              "name": {
                "type": "keyword"
              },
              "os": {
                "properties": {
                  "family": {
                    "type": "keyword"
                  },
                  "platform": {
                    "type": "keyword"
                  },
                  "version": {
                    "type": "keyword"
                  }
                }
              }
            }
          },
          "kubernetes": {
            "properties": {
              "container": {
                "properties": {
                  "name": {
                    "type": "keyword"
                  }
                }
              },
              "namespace": {
                "type": "keyword"
              },
              "node": {
                "properties": {
                  "name": {
                    "type": "keyword"
                  }
                }
              },
              "pod": {
                "properties": {
                  "name": {
                    "type": "keyword"
                  },
                  "uid": {
                    "type": "keyword"
                  }
                }
              }
            }
          },
          "log": {
            "properties": {
              "file": {
                "properties": {
                  "path": {
                    "type": "keyword"
                  }
                }
              },
              "level": {
                "type": "keyword"
              },
              "offset": {
                "type": "long"
              }
            }
          }
        }
      }
    }
```

## 3.1.2 日志字段标准化处理器

```yaml
# log-processing-pipeline.yaml
apiVersion: v1
kind: ConfigMap
metadata:
  name: log-processing-pipeline
  namespace: logging
data:
  pipeline.conf: |
    input {
      beats {
        port => 5044
        ssl => true
        ssl_certificate => "/etc/pki/tls/certs/logstash.crt"
        ssl_key => "/etc/pki/tls/private/logstash.key"
      }
    }
    
    filter {
      # 标准化时间戳
      date {
        match => [ "timestamp", "ISO8601" ]
        target => "@timestamp"
        remove_field => [ "timestamp" ]
      }
      
      # 解析JSON消息体
      json {
        source => "message"
        skip_on_invalid_json => true
      }
      
      # 标准化日志级别
      mutate {
        lowercase => [ "log.level" ]
        convert => {
          "http.response.status_code" => "integer"
          "http.request.duration" => "float"
        }
      }
      
      # 添加环境标识
      mutate {
        add_field => {
          "[fields][environment]" => "%{[@metadata][beat]}"
          "[fields][service]" => "%{[kubernetes][container][name]}"
        }
      }
      
      # IP地理位置解析
      geoip {
        source => "[client][ip]"
        target => "client_geo"
        database => "/usr/share/GeoIP/GeoLite2-City.mmdb"
      }
      
      # 用户代理解析
      useragent {
        source => "[http][request][headers][user-agent]"
        target => "user_agent"
        regexes => "/etc/logstash/regexes.yaml"
      }
      
      # 数据脱敏处理
      if [message] =~ /password|secret|token/i {
        mutate {
          gsub => [
            "message", "(password|secret|token)[=:][^&\s]+", "\\1=***MASKED***"
          ]
        }
      }
    }
    
    output {
      elasticsearch {
        hosts => ["https://elasticsearch:9200"]
        index => "logs-%{[@metadata][beat]}-%{+YYYY.MM.dd}"
        user => "${ELASTICSEARCH_USERNAME}"
        password => "${ELASTICSEARCH_PASSWORD}"
        ssl_certificate_verification => true
        ilm_enabled => true
        ilm_rollover_alias => "logs-%{[@metadata][beat]}"
        ilm_pattern => "{now/d}-000001"
        ilm_policy => "logs-policy"
      }
    }
```

## 3.2 生命周期管理策略

## 3.2.1 ILM(Index Lifecycle Management)策略

```yaml
# ilm-policies.yaml
apiVersion: v1
kind: ConfigMap
metadata:
  name: ilm-policies
  namespace: logging
data:
  application-logs-policy.json: |
    {
      "policy": {
        "phases": {
          "hot": {
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
                "include": {},
                "exclude": {
                  "box_type": "hot"
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
                "number_of_replicas": 0,
                "require": {
                  "box_type": "cold"
                },
                "include": {},
                "exclude": {}
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

  system-logs-policy.json: |
    {
      "policy": {
        "phases": {
          "hot": {
            "actions": {
              "rollover": {
                "max_age": "3d",
                "max_size": "20gb"
              },
              "set_priority": {
                "priority": 150
              }
            }
          },
          "warm": {
            "min_age": "3d",
            "actions": {
              "allocate": {
                "number_of_replicas": 1
              },
              "forcemerge": {
                "max_num_segments": 1
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

  security-logs-policy.json: |
    {
      "policy": {
        "phases": {
          "hot": {
            "actions": {
              "rollover": {
                "max_age": "1d",
                "max_size": "10gb"
              }
            }
          },
          "warm": {
            "min_age": "1d",
            "actions": {
              "readonly": {}
            }
          },
          "cold": {
            "min_age": "7d",
            "actions": {
              "freeze": {}
            }
          },
          "delete": {
            "min_age": "180d",
            "actions": {
              "delete": {}
            }
          }
        }
      }
    }
```

<!-- chunk: 4. 智能分析与告警 -->## 4. 智能分析与告警

## 4.1 机器学习异常检测

## 4.1.1 异常检测作业配置

```yaml
# ml-anomaly-detection.yaml
apiVersion: v1
kind: ConfigMap
metadata:
  name: ml-anomaly-jobs
  namespace: logging
data:
  http-response-codes.json: |
    {
      "job_id": "http_response_codes_analysis",
      "job_type": "anomaly_detector",
      "description": "HTTP响应码异常检测",
      "analysis_config": {
        "bucket_span": "15m",
        "detectors": [
          {
            "detector_description": "异常HTTP错误率",
            "function": "high_count",
            "by_field_name": "http.response.status_code",
            "partition_field_name": "kubernetes.namespace"
          }
        ],
        "influencers": [
          "kubernetes.namespace",
          "kubernetes.pod.name",
          "host.name"
        ]
      },
      "data_description": {
        "time_field": "@timestamp",
        "time_format": "epoch_ms"
      },
      "model_plot_config": {
        "enabled": true,
        "annotations_enabled": true
      },
      "analysis_limits": {
        "model_memory_limit": "1GB",
        "categorization_examples_limit": 4
      },
      "datafeed_config": {
        "datafeed_id": "datafeed-http_response_codes_analysis",
        "indices": ["filebeat-*"],
        "scroll_size": 1000,
        "delayed_data_check_config": {
          "enabled": true
        },
        "query": {
          "bool": {
            "filter": [
              {
                "exists": {
                  "field": "http.response.status_code"
                }
              },
              {
                "range": {
                  "http.response.status_code": {
                    "gte": 400
                  }
                }
              }
            ]
          }
        }
      }
    }

  log-volume-anomaly.json: |
    {
      "job_id": "log_volume_anomaly_detection",
      "job_type": "anomaly_detector",
      "description": "日志量异常检测",
      "analysis_config": {
        "bucket_span": "1h",
        "detectors": [
          {
            "detector_description": "异常日志量增长",
            "function": "high_count",
            "partition_field_name": "kubernetes.container.name"
          }
        ],
        "influencers": [
          "kubernetes.container.name",
          "kubernetes.namespace"
        ]
      },
      "data_description": {
        "time_field": "@timestamp"
      },
      "model_plot_config": {
        "enabled": true
      },
      "analysis_limits": {
        "model_memory_limit": "512MB"
      },
      "datafeed_config": {
        "datafeed_id": "datafeed-log_volume_anomaly_detection",
        "indices": ["filebeat-*"],
        "query_delay": "60s",
        "frequency": "150s",
        "query": {
          "match_all": {}
        }
      }
    }
```

## 4.1.2 实时告警配置

```yaml
# alerting-rules.yaml
apiVersion: v1
kind: ConfigMap
metadata:
  name: elasticsearch-alerts
  namespace: logging
data:
  alert-rules.json: |
    {
      "name": "企业级日志告警规则",
      "schedule": "0 */5 * * * ?",
      "inputs": [
        {
          "search": {
            "request": {
              "search_type": "query_then_fetch",
              "indices": ["filebeat-*"],
              "rest_total_hits_as_int": true,
              "body": {
                "size": 0,
                "query": {
                  "bool": {
                    "must": [
                      {
                        "range": {
                          "@timestamp": {
                            "gte": "now-5m",
                            "lt": "now"
                          }
                        }
                      }
                    ],
                    "should": [
                      {
                        "match": {
                          "log.level": "ERROR"
                        }
                      },
                      {
                        "match": {
                          "log.level": "FATAL"
                        }
                      }
                    ],
                    "minimum_should_match": 1
                  }
                },
                "aggs": {
                  "error_counts": {
                    "terms": {
                      "field": "kubernetes.container.name.keyword",
                      "size": 10
                    }
                  }
                }
              }
            }
          }
        }
      ],
      "triggers": [
        {
          "name": "高频错误日志告警",
          "severity": "high",
          "condition": {
            "script": {
              "source": "ctx.results[0].hits.total.value > params.threshold",
              "lang": "painless",
              "params": {
                "threshold": 100
              }
            }
          },
          "actions": [
            {
              "name": "发送告警通知",
              "destination_id": "slack_notifications",
              "message_template": {
                "source": "发现高频错误日志:\n- 时间范围: {{ctx.periodStart}} - {{ctx.periodEnd}}\n- 错误总数: {{ctx.results[0].hits.total.value}}\n- 详情请查看Kibana仪表板"
              },
              "throttle_enabled": true,
              "throttle": {
                "value": 10,
                "unit": "MINUTES"
              }
            }
          ]
        }
      ]
    }
```

## 4.2 自定义分析仪表板

## 4.2.1 核心业务指标看板

```json
{
  "dashboard": {
    "title": "企业级日志分析仪表板",
    "description": "综合展示应用健康状况和系统性能",
    "panels": [
      {
        "id": "error_rate_panel",
        "type": "visualization",
        "title": "错误率趋势",
        "visState": {
          "title": "错误率趋势",
          "type": "line",
          "params": {
            "addTooltip": true,
            "addLegend": true,
            "legendPosition": "right",
            "times": [],
            "addTimeMarker": false,
            "dimensions": {
              "x": {
                "accessor": 0,
                "format": {
                  "id": "date",
                  "params": {
                    "pattern": "YYYY-MM-DD HH:mm"
                  }
                },
                "params": {
                  "date": true,
                  "interval": "auto",
                  "time_zone": "browser"
                },
                "aggType": "date_histogram"
              },
              "y": [
                {
                  "accessor": 2,
                  "format": {
                    "id": "percent"
                  },
                  "params": {},
                  "aggType": "avg"
                }
              ]
            }
          },
          "aggs": [
            {
              "id": "1",
              "enabled": true,
              "type": "count",
              "schema": "metric",
              "params": {}
            },
            {
              "id": "2",
              "enabled": true,
              "type": "date_histogram",
              "schema": "segment",
              "params": {
                "field": "@timestamp",
                "interval": "auto",
                "customInterval": "2h",
                "min_doc_count": 1,
                "extended_bounds": {}
              }
            },
            {
              "id": "3",
              "enabled": true,
              "type": "filters",
              "schema": "group",
              "params": {
                "filters": [
                  {
                    "input": {
                      "query": {
                        "match": {
                          "log.level": "ERROR"
                        }
                      }
                    },
                    "label": "错误日志"
                  },
                  {
                    "input": {
                      "query": {
                        "match_all": {}
                      }
                    },
                    "label": "总日志"
                  }
                ]
              }
            }
          ]
        }
      }
    ]
  }
}
```

<!-- chunk: 5. 企业级最佳实践 -->## 5. 企业级最佳实践

## 5.1 性能优化策略

## 5.1.1 索引优化配置

```yaml
# index-optimization.yaml
apiVersion: v1
kind: ConfigMap
metadata:
  name: index-optimization-settings
  namespace: logging
data:
  optimization-settings.json: |
    {
      "index": {
        "number_of_shards": 3,
        "number_of_replicas": 1,
        "refresh_interval": "30s",
        "translog.durability": "async",
        "translog.sync_interval": "30s",
        "blocks": {
          "read_only_allow_delete": "false"
        },
        "codec": "best_compression",
        "routing": {
          "allocation": {
            "enable": "all",
            "total_shards_per_node": 5
          }
        },
        "unassigned": {
          "node_left": {
            "delayed_timeout": "5m"
          }
        }
      },
      "index.lifecycle": {
        "name": "logs-policy",
        "rollover_alias": "logs-current"
      }
    }
```

## 5.1.2 查询性能优化

```yaml
# query-optimization.yaml
apiVersion: v1
kind: ConfigMap
metadata:
  name: query-optimization-settings
  namespace: logging
data:
  query-settings.json: |
    {
      "indices.queries.cache.size": "10%",
      "indices.fielddata.cache.size": "20%",
      "indices.requests.cache.enable": true,
      "search.default_search_timeout": "30s",
      "thread_pool.search.size": 20,
      "thread_pool.search.queue_size": 1000,
      "thread_pool.index.size": 10,
      "thread_pool.index.queue_size": 200
    }
```

## 5.2 成本控制策略

## 5.2.1 存储成本优化

```bash
#!/bin/bash
# storage-cost-optimization.sh

echo "=== Elasticsearch存储成本优化分析 ==="

# 1. 分析索引大小
echo "1. 当前索引大小分析:"
curl -s -u ${ES_USER}:${ES_PASS} \
  "${ES_HOST}:9200/_cat/indices?v&s=store.size:desc&bytes=gb" | head -20

# 2. 识别大索引
echo "2. 识别大于10GB的索引:"
curl -s -u ${ES_USER}:${ES_PASS} \
  "${ES_HOST}:9200/_cat/indices?h=index,store.size&s=store.size:desc" | \
  awk '$2+0 > 10 {print $1, $2"GB"}'

# 3. 分析字段占用
echo "3. 字段存储占用分析:"
for index in $(curl -s -u ${ES_USER}:${ES_PASS} "${ES_HOST}:9200/_cat/indices?h=index" | grep filebeat | head -5); do
  echo "索引: $index"
  curl -s -u ${ES_USER}:${ES_PASS} "${ES_HOST}:9200/$index/_field_caps" | \
    jq -r '.fields | to_entries[] | "\(.key): \(.value.text?.metadata?.size // 0) bytes"' | \
    sort -k2 -nr | head -10
  echo "---"
done

# 4. 建议的优化措施
echo "4. 存储优化建议:"
echo "   - 启用更好的压缩算法"
echo "   - 删除不必要的字段"
echo "   - 调整分片数量"
echo "   - 实施更积极的ILM策略"
echo "   - 考虑冷热数据分离"
```

## 5.2.2 资源使用监控

```yaml
# resource-monitoring.yaml
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: elasticsearch-resource-alerts
  namespace: monitoring
spec:
  groups:
  - name: elasticsearch.resource.monitoring
    rules:
    # JVM堆内存使用率
    - alert: ElasticsearchHeapUsageHigh
      expr: |
        elasticsearch_jvm_memory_used_bytes{area="heap"} / 
        elasticsearch_jvm_memory_max_bytes{area="heap"} > 0.85
      for: 5m
      labels:
        severity: warning
        team: sre
      annotations:
        summary: "Elasticsearch堆内存使用率过高"
        description: "{{ $labels.node }}节点堆内存使用率: {{ $value }}%"
    
    # 磁盘使用率
    - alert: ElasticsearchDiskUsageHigh
      expr: |
        elasticsearch_filesystem_data_available_bytes / 
        elasticsearch_filesystem_data_size_bytes < 0.15
      for: 10m
      labels:
        severity: critical
        team: sre
      annotations:
        summary: "Elasticsearch磁盘空间不足"
        description: "{{ $labels.node }}节点磁盘剩余空间: {{ $value }}%"
    
    # CPU使用率
    - alert: ElasticsearchCPUUsageHigh
      expr: |
        rate(elasticsearch_process_cpu_percent[5m]) > 80
      for: 5m
      labels:
        severity: warning
        team: sre
      annotations:
        summary: "Elasticsearch CPU使用率过高"
        description: "{{ $labels.node }}节点CPU使用率: {{ $value }}%"
```

<!-- chunk: 6. 故障排查与维护 -->## 6. 故障排查与维护

## 6.1 常见问题诊断脚本

```bash
#!/bin/bash
# elasticsearch-troubleshooting.sh

echo "=== Elasticsearch故障诊断工具 ==="

CLUSTER_URL="https://elasticsearch:9200"
AUTH="-u admin:${ES_PASSWORD}"

# 1. 集群健康状态
echo "1. 集群健康状态:"
curl -s ${AUTH} "${CLUSTER_URL}/_cluster/health?pretty"

# 2. 节点状态
echo "2. 节点状态:"
curl -s ${AUTH} "${CLUSTER_URL}/_nodes/stats?pretty" | jq '.nodes[].name'

# 3. 未分配分片
echo "3. 未分配分片检查:"
curl -s ${AUTH} "${CLUSTER_URL}/_cat/shards?h=index,shard,prirep,state,unassigned.reason" | \
  grep UNASSIGNED

# 4. 索引统计
echo "4. 索引统计信息:"
curl -s ${AUTH} "${CLUSTER_URL}/_stats" | jq '{
  "总文档数": .indices._all.primaries.docs.count,
  "总存储大小": .indices._all.primaries.store.size_in_bytes,
  "索引数量": .indices | keys | length
}'

# 5. 慢查询日志
echo "5. 慢查询分析:"
curl -s ${AUTH} "${CLUSTER_URL}/_search?pretty" -H 'Content-Type: application/json' -d'
{
  "query": {
    "range": {
      "@timestamp": {
        "gte": "now-1h"
      }
    }
  },
  "aggs": {
    "slow_queries": {
      "terms": {
        "field": "kubernetes.container.name.keyword",
        "size": 10,
        "order": {
          "_count": "desc"
        }
      }
    }
  },
  "size": 0
}'

# 6. GC活动监控
echo "6. 垃圾回收活动:"
curl -s ${AUTH} "${CLUSTER_URL}/_nodes/stats/jvm?pretty" | \
  jq '.nodes[].jvm.gc.collectors.old.collection_count'
```

## 6.2 维护操作自动化

```python
# maintenance-automation.py
import requests
import json
import time
from datetime import datetime, timedelta

class ElasticsearchMaintenance:
    def __init__(self, es_url, username, password):
        self.es_url = es_url
        self.auth = (username, password)
        self.session = requests.Session()
        self.session.auth = self.auth
        self.session.verify = False
        
    def get_cluster_health(self):
        """获取集群健康状态"""
        response = self.session.get(f"{self.es_url}/_cluster/health")
        return response.json()
    
    def optimize_indices(self):
        """优化索引性能"""
        # 获取所有索引
        indices_response = self.session.get(f"{self.es_url}/_cat/indices?format=json")
        indices = [idx['index'] for idx in indices_response.json() 
                  if not idx['index'].startswith('.')]
        
        for index in indices:
            print(f"优化索引: {index}")
            
            # 强制合并段
            merge_response = self.session.post(
                f"{self.es_url}/{index}/_forcemerge?max_num_segments=1"
            )
            print(f"强制合并结果: {merge_response.status_code}")
            
            # 刷新索引
            refresh_response = self.session.post(f"{self.es_url}/{index}/_refresh")
            print(f"刷新索引结果: {refresh_response.status_code}")
            
            time.sleep(1)
    
    def cleanup_old_indices(self, days_to_keep=30):
        """清理旧索引"""
        cutoff_date = datetime.now() - timedelta(days=days_to_keep)
        cutoff_str = cutoff_date.strftime("%Y.%m.%d")
        
        # 获取所有索引
        indices_response = self.session.get(f"{self.es_url}/_cat/indices?format=json")
        indices = [idx['index'] for idx in indices_response.json()]
        
        # 识别可删除的索引
        indices_to_delete = []
        for index in indices:
            if index.startswith('filebeat-') or index.startswith('metricbeat-'):
                # 提取日期部分
                try:
                    date_part = index.split('-')[-1]
                    index_date = datetime.strptime(date_part, "%Y.%m.%d")
                    if index_date < cutoff_date:
                        indices_to_delete.append(index)
                except ValueError:
                    continue
        
        # 删除旧索引
        for index in indices_to_delete:
            print(f"删除旧索引: {index}")
            delete_response = self.session.delete(f"{self.es_url}/{index}")
            print(f"删除结果: {delete_response.status_code}")
    
    def rebalance_cluster(self):
        """重新平衡集群"""
        # 启用分片分配
        allocation_response = self.session.put(
            f"{self.es_url}/_cluster/settings",
            json={
                "persistent": {
                    "cluster.routing.allocation.enable": "all"
                }
            }
        )
        print(f"启用分片分配: {allocation_response.status_code}")
        
        # 等待重平衡完成
        print("等待集群重平衡...")
        while True:
            health = self.get_cluster_health()
            if health['status'] == 'green' and health['relocating_shards'] == 0:
                print("集群重平衡完成")
                break
            time.sleep(30)
    
    def run_daily_maintenance(self):
        """执行日常维护任务"""
        print(f"开始执行日常维护 - {datetime.now()}")
        
        # 1. 检查集群健康
        health = self.get_cluster_health()
        print(f"集群状态: {health['status']}")
        
        if health['status'] != 'green':
            print("警告: 集群不是绿色状态，跳过维护操作")
            return
            
        # 2. 优化索引
        self.optimize_indices()
        
        # 3. 清理旧索引
        self.cleanup_old_indices(days_to_keep=45)
        
        # 4. 重新平衡集群
        self.rebalance_cluster()
        
        print(f"日常维护完成 - {datetime.now()}")

# 使用示例
maintenance = ElasticsearchMaintenance(
    "https://elasticsearch:9200",
    "admin",
    "your_password"
)
maintenance.run_daily_maintenance()
```

通过以上企业级Elastic Stack深度实践，企业可以构建完整的日志治理体系，实现从日志采集、存储、分析到告警的全流程自动化管理，显著提升运维效率和系统可靠性。

---

<!-- chunk: Obsidian 相关文档 -->## Obsidian 相关文档

- observability/MOC.md|domain-20-enterprise-monitoring-alerting MOC]]
- [[可观测性/README.md|[[Domain 20: 企业级监控与告警 (Enterprise Monitoring & Alerting)|Domain 20: 企业级监控与告警 (Enterprise Monitoring & Alerting)]]]]
- [[可观测性/00-open-source-projects-index.md|Domain-20 企业监控与告警 — 开源项目索引]]
- [[entities/prometheus.md|prometheus]]
- Grafana Enterprise Observability Platform 深度实践
- OpenTelemetry分布式追踪与可观测性深度实践
- Thanos Enterprise Metrics Federation and Long-term Storage
- Datadog企业级APM深度实践
- Datadog 企业级监控平台深度实践
- Elastic Stack企业级可观测性平台深度实践
- Zabbix Enterprise Monitoring Platform 深度实践
- New Relic Enterprise APM Platform 深度实践

## See Also

- 05-datadog-enterprise-apm
- 05-datadog-enterprise-monitoring
- 06-elastic-stack-enterprise-observability
- 07-zabbix-enterprise-monitoring

- [[可观测性/README.md|返回目录]]

<!-- risk-assessed -->
