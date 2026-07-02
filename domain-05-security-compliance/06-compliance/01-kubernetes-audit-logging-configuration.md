---
title: Kubernetes 审计日志配置
description: 'Audit Policy 配置：请求级审计日志、审计后端 (log/webhook)、日志采集与分析 (ES/Loki)、合规保留策略、审计规则最佳实践'
summary: 'Audit Policy、审计后端、日志采集分析与合规保留策略'
category: security-compliance
tags:
- audit-logging
- compliance
- audit-policy
- elasticsearch
- loki
tier: supporting
created: '2026-07-02'
last_updated: 2026-07
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 安全工程师
- 平台工程师
estimated_read_time: 15min
intent_queries:
- Kubernetes 审计日志配置是什么
- 如何配置 Audit Policy
trigger_keywords:
- Audit Policy
- 审计日志
- audit logging
- compliance
- API Server
prerequisites:
- kubectl-basics
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


# Kubernetes 审计日志配置

## 概述

Kubernetes 审计日志记录 API Server 收到的所有请求，是安全合规、故障排查和威胁检测的核心数据源。通过配置 Audit Policy 可以精确控制记录哪些请求以及记录的详细程度。

## 1. 审计日志级别

Kubernetes 定义四个审计级别：

| 级别 | 记录内容 | 适用场景 |
|------|---------|---------|
| `None` | 不记录 | 不需要审计的请求 |
| `Metadata` | 请求元数据（时间、用户、资源、操作、结果） | 一般审计 |
| `Request` | 元数据 + 请求体 | 需要记录请求详情 |
| `RequestResponse` | 元数据 + 请求体 + 响应体 | 关键操作完整记录 |

## 2. Audit Policy 配置

### 2.1 基础 Audit Policy

```yaml
# /etc/kubernetes/audit-policy.yaml
apiVersion: audit.k8s.io/v1
kind: Policy
rules:
# 不审计的请求（减少噪音）
- level: None
  users: ["system:kube-proxy"]
  verbs: ["watch"]
  resources:
  - group: ""
    resources: ["endpoints", "services", "services/status"]

- level: None
  users: ["system:unsecured"]
  namespaces: ["kube-system"]

- level: None
  verbs: ["get"]
  resources:
  - group: ""
    resources: ["healthz", "livez", "readyz"]

# Secret 请求仅记录元数据（不泄露敏感数据）
- level: Metadata
  resources:
  - group: ""
    resources: ["secrets", "configmaps"]

# 认证和授权相关请求
- level: RequestResponse
  resources:
  - group: "authentication.k8s.io"
  - group: "authorization.k8s.io"

# RBAC 变更记录完整请求和响应
- level: RequestResponse
  resources:
  - group: "rbac.authorization.k8s.io"
    resources: ["clusterroles", "clusterrolebindings", "roles", "rolebindings"]

# Namespace 变更
- level: RequestResponse
  resources:
  - group: ""
    resources: ["namespaces"]
  verbs: ["create", "update", "patch", "delete"]

# 所有写操作记录 Request 级别
- level: Request
  verbs: ["create", "update", "patch", "delete"]

# 其他请求记录 Metadata
- level: Metadata
  omitStages:
  - "RequestReceived"
```

### 2.2 高安全环境 Audit Policy

```yaml
# /etc/kubernetes/audit-policy-strict.yaml
apiVersion: audit.k8s.io/v1
kind: Policy
rules:
# Pod exec/attach/portforward 记录完整请求
- level: RequestResponse
  verbs: ["create"]
  resources:
  - group: ""
    resources: ["pods/exec", "pods/attach", "pods/portforward"]

# 所有认证请求
- level: RequestResponse
  resources:
  - group: "authentication.k8s.io"

# Token 请求
- level: RequestResponse
  verbs: ["create"]
  resources:
  - group: ""
    resources: ["serviceaccounts/token"]

# Node 变更
- level: RequestResponse
  verbs: ["create", "update", "patch", "delete"]
  resources:
  - group: ""
    resources: ["nodes"]

# 所有写操作
- level: Request
  verbs: ["create", "update", "patch", "delete"]

# 读操作记录 Metadata
- level: Metadata
  verbs: ["get", "list", "watch"]
  omitStages:
  - "RequestReceived"
```

## 3. 审计后端配置

### 3.1 Log 后端

```yaml
# kube-apiserver 配置
apiVersion: kubeadm.k8s.io/v1beta3
kind: ClusterConfiguration
apiServer:
  extraArgs:
    audit-policy-file: /etc/kubernetes/audit-policy.yaml
    audit-log-path: /var/log/kubernetes/audit/audit.log
    audit-log-maxage: 30
    audit-log-maxbackup: 10
    audit-log-maxsize: 100
    audit-log-format: json
  extraVolumes:
  - name: audit-policy
    hostPath: /etc/kubernetes/audit-policy.yaml
    mountPath: /etc/kubernetes/audit-policy.yaml
    readOnly: true
    pathType: File
  - name: audit-log
    hostPath: /var/log/kubernetes/audit
    mountPath: /var/log/kubernetes/audit
    readOnly: false
    pathType: DirectoryOrCreate
```

### 3.2 Webhook 后端

```yaml
# /etc/kubernetes/audit-webhook-config.yaml
apiVersion: v1
kind: Config
clusters:
- name: audit-backend
  cluster:
    server: https://audit-backend.example.com:9443
    certificate-authority: /etc/kubernetes/pki/audit-ca.crt
users:
- name: kube-apiserver
  user:
    client-certificate: /etc/kubernetes/pki/audit-client.crt
    client-key: /etc/kubernetes/pki/audit-client.key
current-context: audit-webhook
contexts:
- context:
    cluster: audit-backend
    user: kube-apiserver
  name: audit-webhook
```

```yaml
# kube-apiserver Webhook 配置
apiServer:
  extraArgs:
    audit-webhook-config-file: /etc/kubernetes/audit-webhook-config.yaml
    audit-webhook-batch-max-size: "100"
    audit-webhook-batch-max-wait: "5s"
    audit-webhook-initial-backoff: "10s"
    audit-webhook-mode: batch
```

### 3.3 同时使用 Log 和 Webhook

```yaml
# kube-apiserver 同时启用两种后端
apiServer:
  extraArgs:
    # Log 后端
    audit-log-path: /var/log/kubernetes/audit/audit.log
    audit-policy-file: /etc/kubernetes/audit-policy.yaml
    audit-log-maxage: "30"
    audit-log-maxsize: "100"
    # Webhook 后端（发送到集中式审计系统）
    audit-webhook-config-file: /etc/kubernetes/audit-webhook-config.yaml
    audit-webhook-mode: batch
```

## 4. 日志采集与分析

### 4.1 Elasticsearch 集成

```yaml
# Fluentd DaemonSet 配置（采集审计日志）
apiVersion: apps/v1
kind: DaemonSet
metadata:
  name: fluentd-audit
  namespace: logging
spec:
  selector:
    matchLabels:
      app: fluentd-audit
  template:
    metadata:
      labels:
        app: fluentd-audit
    spec:
      tolerations:
      - key: node-role.kubernetes.io/master
        effect: NoSchedule
      - key: node-role.kubernetes.io/control-plane
        effect: NoSchedule
      containers:
      - name: fluentd
        image: fluent/fluentd-kubernetes-daemonset:v1.16-debian-elasticsearch8-1
        env:
        - name: FLUENT_ELASTICSEARCH_HOST
          value: "elasticsearch.logging.svc"
        - name: FLUENT_ELASTICSEARCH_PORT
          value: "9200"
        volumeMounts:
        - name: audit-log
          mountPath: /var/log/kubernetes/audit
          readOnly: true
        - name: config
          mountPath: /fluentd/etc/conf.d
      volumes:
      - name: audit-log
        hostPath:
          path: /var/log/kubernetes/audit
          type: Directory
      - name: config
        configMap:
          name: fluentd-audit-config
---
apiVersion: v1
kind: ConfigMap
metadata:
  name: fluentd-audit-config
  namespace: logging
data:
  audit.conf: |
    <source>
      @type tail
      path /var/log/kubernetes/audit/audit.log
      pos_file /var/log/fluentd-audit.pos
      tag k8s.audit
      <parse>
        @type json
        time_key requestReceivedTimestamp
        time_format %Y-%m-%dT%H:%M:%S.%NZ
      </parse>
    </source>

    <filter k8s.audit>
      @type record_transformer
      <record>
        index_name "k8s-audit-${Time.at(time).strftime('%Y.%m')}"
      </record>
    </filter>

    <match k8s.audit>
      @type elasticsearch
      host elasticsearch.logging.svc
      port 9200
      index_name ${index_name}
      type_name _doc
      logstash_format false
      <buffer>
        flush_interval 10s
        chunk_limit_size 5M
        retry_max_interval 30s
      </buffer>
    </match>
```

### 4.2 Loki 集成

```yaml
# Promtail 配置采集审计日志
apiVersion: v1
kind: ConfigMap
metadata:
  name: promtail-audit-config
  namespace: logging
data:
  promtail.yaml: |
    server:
      http_listen_port: 9080

    positions:
      filename: /tmp/positions.yaml

    clients:
    - url: http://loki-gateway.logging.svc/loki/api/v1/push

    scrape_configs:
    - job_name: k8s-audit
      static_configs:
      - targets:
        - localhost
        labels:
          job: k8s-audit
          __path__: /var/log/kubernetes/audit/*.log

      pipeline_stages:
      - json:
          expressions:
            user: user.username
            verb: verb
            resource: objectRef.resource
            namespace: objectRef.namespace
            name: objectRef.name
            stage: stage
            code: responseStatus.code

      - labels:
          user:
          verb:
          resource:
          namespace:
          stage:
```

## 5. 审计日志查询示例

### 5.1 Elasticsearch 查询

```json
// 查询所有 Secret 访问
GET k8s-audit-*/_search
{
  "query": {
    "bool": {
      "must": [
        { "term": { "objectRef.resource": "secrets" } },
        { "terms": { "verb": ["get", "list", "watch"] } }
      ],
      "filter": [
        { "range": { "requestReceivedTimestamp": { "gte": "now-24h" } } }
      ]
    }
  },
  "sort": [
    { "requestReceivedTimestamp": { "order": "desc" } }
  ]
}

// 查询认证失败事件
GET k8s-audit-*/_search
{
  "query": {
    "bool": {
      "must": [
        { "term": { "objectRef.resource": "tokenreviews" } },
        { "range": { "responseStatus.code": { "gte": 400 } } }
      ]
    }
  }
}
```

### 5.2 LogQL 查询（Loki）

```logql
# 查询所有 Secret 访问
{k8s-audit} | json | resource="secrets" | verb=~"get|list"

# 查询特定用户的操作
{k8s-audit} | json | user="admin@example.com"

# 查询失败的请求
{k8s-audit} | json | code >= 400

# 查询 Pod exec 操作（高风险操作）
{k8s-audit} | json | resource="pods/exec" | verb="create"

# 查询 RBAC 变更
{k8s-audit} | json | resource=~"clusterroles|clusterrolebindings|roles|rolebindings"
```

## 6. 合规保留策略

### 6.1 数据保留配置

```yaml
# Elasticsearch ILM 策略
PUT _ilm/policy/k8s-audit-retention
{
  "policy": {
    "phases": {
      "hot": {
        "min_age": "0ms",
        "actions": {
          "rollover": {
            "max_size": "50gb",
            "max_age": "7d"
          },
          "set_priority": {
            "priority": 100
          }
        }
      },
      "warm": {
        "min_age": "30d",
        "actions": {
          "shrink": {
            "number_of_shards": 1
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
        "min_age": "90d",
        "actions": {
          "searchable_snapshot": {
            "snapshot_repository": "audit-archive"
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

### 6.2 Loki 保留配置

```yaml
# Loki 审计日志保留配置
auth_enabled: false

schema_config:
  configs:
  - from: 2024-01-01
    store: tsdb
    object_store: s3
    schema: v13
    index:
      prefix: k8s-audit
      period: 24h

storage_config:
  aws:
    s3: s3://audit-logs/loki
    s3forcepathstyle: true

compactor:
  working_directory: /loki/compactor
  retention_enabled: true
  retention_delete_delay: 2h
  retention_delete_worker_count: 150

limits_config:
  retention_period: 8760h  # 1 年
```

## 7. 最佳实践

```
审计日志检查清单：

□ 定义清晰的 Audit Policy（避免日志爆炸）
□ 敏感资源（Secret、ConfigMap）仅记录 Metadata
□ 关键操作（RBAC 变更、Namespace 操作）记录 RequestResponse
□ 配置日志轮转（大小和天数限制）
□ 使用集中式日志系统（ES/Loki）
□ 设置合规保留策略（通常 1 年）
□ 配置审计日志监控告警
□ 定期审查审计日志完整性
□ 保护审计日志不被篡改
□ 测试审计日志恢复流程
```

## Related

- [[domain-06-observability/03-logging/01-efk-stack|EFK 日志栈]]
- [[domain-05-security-compliance/06-compliance/02-encryption-at-rest-transit|加密方案]]

## See Also

- [Kubernetes 审计日志文档](https://kubernetes.io/docs/concepts/security/auditing/)
- [Audit Policy 参考](https://kubernetes.io/docs/reference/config-api/apiserver-audit.v1/)


<!-- risk-assessed -->
