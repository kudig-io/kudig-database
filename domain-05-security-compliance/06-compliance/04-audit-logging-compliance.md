---
title: 04 - 审计日志与合规性管理
description: '# 04 - 审计日志与合规性管理'
category: security
tags:
- k8s
- security
- rbac
- authentication
- authorization
- apiserver
- prometheus
- grafana
- elasticsearch
- job
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- 安全工程师
- SRE
- 架构师
estimated_read_time: 5min
intent_queries:
- 审计日志与合规性管理 是什么
- 如何 审计日志与合规性管理
- Kubernetes 7 security 最佳实践
trigger_keywords:
- 审计日志与合规性管理
- security
prerequisites:
- kubectl-basics
- rbac-basics
- prometheus-basics
- monitoring-basics
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
cross_refs:
- type: domain
  path: ../domain-01-cluster-fundamentals/
  label: '相关知识域: domain-01-cluster-fundamentals'
- type: domain
  path: ../domain-06-observability/
  label: '相关知识域: domain-06-observability'
- type: cheatsheet
  path: ../domain-17-system-foundation/topic-cheat-sheet/tls-pki.md
  label: '速查卡: tls-pki'
---

# 04 - 审计日志与合规性管理

> **适用版本**: v1.25 - v1.32 | **最后更新**: 2026-02 | **参考**: [kubernetes.io/docs/tasks/debug-application-cluster/audit](https://kubernetes.io/docs/tasks/debug-application-cluster/audit/)

<!-- chunk: 审计日志架构全景 -->
## 审计日志架构全景

```
┌─────────────────────────────────────────────────────────────────────────────────────┐
│                          Kubernetes 审计日志体系                                     │
├─────────────────────────────────────────────────────────────────────────────────────┤
│                                                                                      │
│  ┌────────────────────────────────────────────────────────────────────────────────┐ │
│  │                        Audit Policy Configuration                              │ │
│  │  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐       │ │
│  │  │   Metadata   │  │   Request    │  │ RequestResp  │  │     None     │       │ │
│  │  │   级别       │  │   级别       │  │   级别       │  │   级别       │       │ │
│  │  │ (轻量级)    │  │ (中等)      │  │ (详细)      │  │ (不记录)    │       │ │
│  │  └──────┬───────┘  └──────┬───────┘  └──────┬───────┘  └──────┬───────┘       │ │
│  │         │                 │                 │                 │                │ │
│  │         └─────────────────┼─────────────────┼─────────────────┘                │ │
│  │                           │                 │                                  │ │
│  │                    ┌──────▼─────────────────▼──────┐                          │ │
│  │                    │    审计策略规则引擎           │                          │ │
│  │                    │  Audit Policy Engine          │                          │ │
│  │                    └───────────────────────────────┘                          │ │
│  └────────────────────────────────────────────────────────────────────────────────┘ │
│                                    │                                                │
│  ┌─────────────────────────────────▼──────────────────────────────────────────────┐ │
│  │                        Audit Log Processing                                    │ │
│  │  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐       │ │
│  │  │   API Server │  │   Logstash   │  │   Fluentd    │  │   Filebeat   │       │ │
│  │  │   本地存储   │  │   处理       │  │   收集       │  │   采集       │       │ │
│  │  │              │  │              │  │              │  │              │       │ │
│  │  │ • JSON格式   │  │ • 过滤       │  │ • 解析       │  │ • 轻量级     │       │ │
│  │  │ • 轮转       │  │ • 转换       │  │ • 路由       │  │ • 采集       │       │ │
│  │  └──────┬───────┘  └──────┬───────┘  └──────┬───────┘  └──────┬───────┘       │ │
│  │         │                 │                 │                 │                │ │
│  │         └─────────────────┼─────────────────┼─────────────────┘                │ │
│  │                           │                 │                                  │ │
│  │                    ┌──────▼─────────────────▼──────┐                          │ │
│  │                    │       Centralized Storage     │                          │ │
│  │                    │    集中式日志存储系统          │                          │ │
│  │                    └───────────────────────────────┘                          │ │
│  └────────────────────────────────────────────────────────────────────────────────┘ │
│                                    │                                                │
│  ┌─────────────────────────────────▼──────────────────────────────────────────────┐ │
│  │                      Compliance & Analysis                                     │ │
│  │  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐       │ │
│  │  │     ELK      │  │   Grafana    │  │   Kibana     │  │   Splunk     │       │ │
│  │  │   Stack      │  │   Dashboard  │  │   分析       │  │   Enterprise │       │ │
│  │  │              │  │              │  │              │  │              │       │ │
│  │  │ • 存储       │  │ • 可视化     │  │ • 搜索       │  │ • 企业级     │       │ │
│  │  │ • 搜索       │  │ • 告警       │  │ • 分析       │  │ • 合规       │       │ │
│  │  └──────────────┘  └──────────────┘  └──────────────┘  └──────────────┘       │ │
│  └────────────────────────────────────────────────────────────────────────────────┘ │
│                                                                                      │
└─────────────────────────────────────────────────────────────────────────────────────┘
```

<!-- chunk: 审计策略配置 -->
## 审计策略配置

### 审计级别详解

| 级别 | 记录内容 | 性能影响 | 存储需求 | 适用场景 |
|-----|---------|---------|---------|---------|
| **None** | 不记录 | 无 | 无 | 健康检查等 |
| **Metadata** | 请求元数据 | 低 | 中等 | 大多数资源 |
| **Request** | 元数据+请求体 | 中 | 较高 | 敏感资源 |
| **RequestResponse** | 元数据+请求+响应 | 高 | 很高 | 关键资源 |

### 生产级审计策略

```yaml
# 01-audit-policy.yaml
apiVersion: audit.k8s.io/v1
kind: Policy
rules:
  # 1. 不记录健康检查和系统组件
  - level: None
    users: ["system:kube-proxy", "system:node-problem-detector"]
    verbs: ["get", "list", "watch"]
    resources:
    - group: ""
      resources: ["endpoints", "services", "services/status"]
      
  # 2. Secrets访问记录Request级别
  - level: Request
    resources:
    - group: ""
      resources: ["secrets", "configmaps"]
    verbs: ["get", "list", "watch", "create", "update", "patch", "delete"]
    
  # 3. RBAC变更记录Request级别
  - level: Request
    resources:
    - group: "rbac.authorization.k8s.io"
      resources: ["roles", "rolebindings", "clusterroles", "clusterrolebindings"]
    verbs: ["create", "update", "patch", "delete"]
    
  # 4. Pod执行命令记录Request级别
  - level: Request
    resources:
    - group: ""
      resources: ["pods/exec", "pods/attach", "pods/portforward"]
    verbs: ["create"]
    
  # 5. ServiceAccount Token创建
  - level: Request
    resources:
    - group: ""
      resources: ["serviceaccounts/token"]
    verbs: ["create"]
    
  # 6. 节点相关操作
  - level: Request
    resources:
    - group: ""
      resources: ["nodes", "nodes/proxy", "nodes/status"]
    verbs: ["create", "update", "patch", "delete"]
    
  # 7. 持久化存储相关
  - level: Request
    resources:
    - group: ""
      resources: ["persistentvolumes", "persistentvolumeclaims"]
    verbs: ["create", "update", "patch", "delete"]
    - group: "storage.k8s.io"
      resources: ["storageclasses", "csidrivers", "csinodes"]
      
  # 8. 网络策略变更
  - level: Request
    resources:
    - group: "networking.k8s.io"
      resources: ["networkpolicies", "ingresses"]
    verbs: ["create", "update", "patch", "delete"]
    
  # 9. 准入控制器配置
  - level: Request
    resources:
    - group: "admissionregistration.k8s.io"
      resources: ["validatingwebhookconfigurations", "mutatingwebhookconfigurations"]
    verbs: ["create", "update", "patch", "delete"]
    
  # 10. API Server配置变更
  - level: RequestResponse
    resources:
    - group: ""
      resources: ["namespaces", "resourcequotas", "limitranges"]
    verbs: ["create", "update", "patch", "delete"]
    
  # 11. 其他资源记录Metadata级别
  - level: Metadata
    omitStages:
    - "RequestReceived"
```

<!-- chunk: API Server 审计配置 -->
## API Server 审计配置

### 静态Pod配置

```yaml
# /etc/kubernetes/manifests/kube-apiserver.yaml
apiVersion: v1
kind: Pod
metadata:
  name: kube-apiserver
  namespace: kube-system
spec:
  containers:
  - name: kube-apiserver
    command:
    - kube-apiserver
    # 审计日志配置
    - --audit-log-path=/var/log/kubernetes/audit/audit.log
    - --audit-log-maxage=30
    - --audit-log-maxbackup=10
    - --audit-log-maxsize=100
    - --audit-policy-file=/etc/kubernetes/audit/policy.yaml
    - --audit-log-format=json
    - --audit-log-batch-buffer-size=10000
    - --audit-log-batch-max-size=400
    - --audit-log-batch-max-wait=30s
    - --audit-log-batch-throttle-enable=true
    - --audit-log-batch-throttle-burst=15
    - --audit-log-batch-throttle-qps=10
    # Webhook审计配置(可选)
    - --audit-webhook-config-file=/etc/kubernetes/audit/webhook-config.yaml
    - --audit-webhook-batch-buffer-size=1000
    - --audit-webhook-batch-max-size=400
    - --audit-webhook-batch-max-wait=30s
```

### 审计Webhook配置

```yaml
# 02-audit-webhook-config.yaml
apiVersion: v1
kind: Config
clusters:
- name: audit-webhook
  cluster:
    server: https://audit-collector.example.com/audit
    certificate-authority: /etc/kubernetes/audit/ca.crt
contexts:
- context:
    cluster: audit-webhook
    user: audit-webhook
  name: audit-webhook
current-context: audit-webhook
users:
- name: audit-webhook
  user:
    client-certificate: /etc/kubernetes/audit/client.crt
    client-key: /etc/kubernetes/audit/client.key
```

<!-- chunk: 日志收集与处理 -->
## 日志收集与处理

### Fluent Bit 配置

```yaml
# 03-fluent-bit-config.yaml
apiVersion: v1
kind: ConfigMap
metadata:
  name: fluent-bit-config
  namespace: logging
data:
  fluent-bit.conf: |
    [SERVICE]
        Flush         5
        Log_Level     info
        Daemon        off
        Parsers_File  parsers.conf
        HTTP_Server   On
        HTTP_Listen   0.0.0.0
        HTTP_Port     2020

    [INPUT]
        Name              tail
        Path              /var/log/kubernetes/audit/audit.log
        Parser            json
        Tag               audit.*
        Refresh_Interval  5
        Mem_Buf_Limit     50MB
        Skip_Long_Lines   On
        DB                /var/log/flb_kube.db
        DB.Sync           Normal

    [FILTER]
        Name          record_modifier
        Match         audit.*
        Record        cluster_name ${CLUSTER_NAME}
        Record        source kubernetes_audit

    [FILTER]
        Name          modify
        Match         audit.*
        Add           timestamp ${TIMESTAMP}

    [OUTPUT]
        Name          es
        Match         audit.*
        Host          elasticsearch.logging
        Port          9200
        Index         k8s-audit-%Y.%m.%d
        Type          _doc
        Logstash_Format Off
        Replace_Dots  On
        Retry_Limit   False

  parsers.conf: |
    [PARSER]
        Name        json
        Format      json
        Time_Key    requestReceivedTimestamp
        Time_Format %Y-%m-%dT%H:%M:%S.%LZ
        Time_Keep   On
        Decode_Field_As escaped_utf8 log do_next
        Decode_Field_As json log
```

### Logstash 处理管道

```ruby
# 04-logstash-pipeline.conf
input {
  beats {
    port => 5044
    codec => "json"
  }
}

filter {
  # 解析审计日志
  if [kubernetes][container][name] == "kube-apiserver" {
    json {
      source => "message"
      target => "audit_log"
    }
    
    # 提取关键字段
    mutate {
      add_field => {
        "user" => "%{[audit_log][user][username]}"
        "verb" => "%{[audit_log][verb]}"
        "resource" => "%{[audit_log][objectRef][resource]}"
        "namespace" => "%{[audit_log][objectRef][namespace]}"
        "response_code" => "%{[audit_log][responseStatus][code]}"
      }
    }
    
    # 分类处理
    if [audit_log][level] == "Request" or [audit_log][level] == "RequestResponse" {
      # 敏感操作标记
      if [audit_log][objectRef][resource] == "secrets" or 
         [audit_log][objectRef][resource] == "serviceaccounts/token" {
        mutate {
          add_tag => ["sensitive_operation"]
        }
      }
      
      # RBAC变更标记
      if [audit_log][objectRef][group] == "rbac.authorization.k8s.io" {
        mutate {
          add_tag => ["rbac_change"]
        }
      }
    }
  }
  
  # 时间戳处理
  date {
    match => [ "requestReceivedTimestamp", "ISO8601" ]
    target => "@timestamp"
  }
}

output {
  # Elasticsearch输出
  elasticsearch {
    hosts => ["elasticsearch:9200"]
    index => "k8s-audit-%{+YYYY.MM.dd}"
    document_type => "_doc"
  }
  
  # 告警输出到Alertmanager
  if "sensitive_operation" in [tags] or "rbac_change" in [tags] {
    http {
      url => "http://alertmanager:9093/api/v1/alerts"
      http_method => "post"
      format => "json"
      mapping => {
        "alerts" => [
          {
            "status" => "firing"
            "labels" => {
              "alertname" => "KubernetesAuditAlert"
              "severity" => "warning"
              "operation" => "%{[verb]}"
              "resource" => "%{[resource]}"
            }
            "annotations" => {
              "summary" => "Kubernetes审计告警: %{[verb]} %{[resource]}"
              "description" => "用户 %{[user]} 执行了 %{[verb]} 操作于 %{[resource]}"
            }
            "generatorURL" => "http://grafana:3000"
          }
        ]
      }
    }
  }
}
```

<!-- chunk: 合规性检查与报告 -->
## 合规性检查与报告

### CIS Kubernetes Benchmark 审计

```bash
#!/bin/bash
# 05-cis-audit.sh

echo "=== Kubernetes CIS合规性审计 ==="
echo "审计时间: $(date)"
echo "集群版本: $(kubectl version --short | grep Server | awk '{print $3}')"
echo ""

# 1. 审计日志检查
echo "1. 审计日志配置检查"
if kubectl get pod -n kube-system -l component=kube-apiserver -o jsonpath='{.items[*].spec.containers[*].command}' | grep -q "audit-log-path"; then
    echo "✅ 审计日志已启用"
    AUDIT_LOG_PATH=$(kubectl get pod -n kube-system -l component=kube-apiserver -o jsonpath='{.items[*].spec.containers[*].command}' | grep -o '\--audit-log-path=[^ ]*' | cut -d= -f2)
    echo "   日志路径: $AUDIT_LOG_PATH"
else
    echo "❌ 审计日志未启用"
fi

# 2. 审计策略检查
echo -e "\n2. 审计策略检查"
if kubectl get pod -n kube-system -l component=kube-apiserver -o jsonpath='{.items[*].spec.containers[*].command}' | grep -q "audit-policy-file"; then
    echo "✅ 审计策略文件已配置"
else
    echo "❌ 审计策略文件未配置"
fi

# 3. 敏感操作审计检查
echo -e "\n3. 敏感操作审计覆盖检查"
SENSITIVE_RESOURCES=("secrets" "serviceaccounts/token" "roles" "rolebindings" "clusterroles" "clusterrolebindings")

for resource in "${SENSITIVE_RESOURCES[@]}"; do
    if grep -q "\"resource\":\"$resource\"" $AUDIT_LOG_PATH/*.log 2>/dev/null; then
        echo "✅ $resource 操作已被审计"
    else
        echo "⚠️  $resource 操作审计缺失"
    fi
done

# 4. 日志保留策略检查
echo -e "\n4. 日志保留策略检查"
MAX_AGE=$(kubectl get pod -n kube-system -l component=kube-apiserver -o jsonpath='{.items[*].spec.containers[*].command}' | grep -o '\--audit-log-maxage=[0-9]*' | cut -d= -f2)
if [ "$MAX_AGE" -ge "30" ]; then
    echo "✅ 日志保留期符合要求: ${MAX_AGE}天"
else
    echo "❌ 日志保留期不足: ${MAX_AGE}天"
fi

# 5. 生成合规报告
echo -e "\n=== 合规检查摘要 ==="
TOTAL_CHECKS=5
PASSED_CHECKS=$(grep -c "✅" <<< "$(tail -n +10)")
COMPLIANCE_SCORE=$(( PASSED_CHECKS * 100 / TOTAL_CHECKS ))

echo "合规评分: ${COMPLIANCE_SCORE}% (${PASSED_CHECKS}/${TOTAL_CHECKS})"

if [ $COMPLIANCE_SCORE -ge 80 ]; then
    echo "🟢 合规状态: 良好"
elif [ $COMPLIANCE_SCORE -ge 60 ]; then
    echo "🟡 合规状态: 需要改进"
else
    echo "🔴 合规状态: 不合规"
fi
```

### SOX/PCI-DSS 合规报告模板

```yaml
# 06-compliance-report-template.yaml
apiVersion: batch/v1
kind: CronJob
metadata:
  name: compliance-report-generator
  namespace: monitoring
spec:
  schedule: "0 2 * * 1"  # 每周一凌晨2点执行
  jobTemplate:
    spec:
      template:
        spec:
          containers:
          - name: report-generator
            image: compliance-reporter:latest
            command:
            - /bin/sh
            - -c
            - |
              # 生成合规报告
              echo "生成合规报告..."
              
              # 1. 审计日志统计
              LOG_STATS=$(curl -s "http://elasticsearch:9200/k8s-audit*/_count" | jq '.count')
              
              # 2. 敏感操作统计
              SENSITIVE_OPS=$(curl -s "http://elasticsearch:9200/k8s-audit*/_count" -H "Content-Type: application/json" -d '{
                "query": {
                  "terms": {
                    "audit_log.objectRef.resource": ["secrets", "serviceaccounts/token"]
                  }
                }
              }' | jq '.count')
              
              # 3. RBAC变更统计
              RBAC_CHANGES=$(curl -s "http://elasticsearch:9200/k8s-audit*/_count" -H "Content-Type: application/json" -d '{
                "query": {
                  "term": {
                    "audit_log.objectRef.group": "rbac.authorization.k8s.io"
                  }
                }
              }' | jq '.count')
              
              # 4. 生成报告
              cat > /reports/compliance-report-$(date +%Y%m%d).md << EOF
              # Kubernetes 合规报告
              
              **报告日期**: $(date)
              **集群版本**: $(kubectl version --short | grep Server | awk '{print $3}')
              
              ## 审计统计
              - 总审计事件数: ${LOG_STATS}
              - 敏感操作数: ${SENSITIVE_OPS}
              - RBAC变更数: ${RBAC_CHANGES}
              
              ## 合规检查项
              | 检查项 | 状态 | 说明 |
              |-------|------|------|
              | 审计日志启用 | ✅ | 已配置 |
              | 敏感操作审计 | ✅ | 覆盖完整 |
              | RBAC审计 | ✅ | 变更可追溯 |
              | 日志保留 | ✅ | 符合要求 |
              
              ## 建议改进项
              1. 增加对ConfigMap的审计级别
              2. 优化审计日志存储策略
              3. 建立定期审计回顾机制
              EOF
              
              echo "报告生成完成"
            volumeMounts:
            - name: reports
              mountPath: /reports
          volumes:
          - name: reports
            emptyDir: {}
          restartPolicy: OnFailure
```

<!-- chunk: 监控与告警配置 -->
## 监控与告警配置

### Prometheus 告警规则

```yaml
# 07-audit-alerts.yaml
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: audit-alerts
  namespace: monitoring
spec:
  groups:
  - name: audit
    rules:
    # 审计日志丢失告警
    - alert: AuditLogMissing
      expr: |
        absent(apiserver_audit_event_total)
      for: 5m
      labels:
        severity: critical
      annotations:
        summary: "审计日志未生成"
        description: "API Server可能未正确配置审计日志"
        
    # 敏感操作告警
    - alert: SensitiveOperationDetected
      expr: |
        sum(rate(apiserver_audit_event_total{
          objectRef_resource=~"secrets|serviceaccounts/token",
          verb=~"create|update|delete"
        }[5m])) > 0
      for: 1m
      labels:
        severity: warning
      annotations:
        summary: "检测到敏感操作"
        description: "用户 {{ $labels.user_username }} 对 {{ $labels.objectRef_resource }} 执行了 {{ $labels.verb }} 操作"
        
    # RBAC变更告警
    - alert: RBACChangeDetected
      expr: |
        sum(rate(apiserver_audit_event_total{
          objectRef_group="rbac.authorization.k8s.io",
          verb=~"create|update|delete"
        }[5m])) > 0
      for: 1m
      labels:
        severity: info
      annotations:
        summary: "RBAC配置发生变更"
        description: "RBAC资源 {{ $labels.objectRef_resource }} 被 {{ $labels.verb }}"
        
    # 大量失败请求
    - alert: HighAuditFailures
      expr: |
        sum(rate(apiserver_audit_error_total[5m])) > 10
      for: 5m
      labels:
        severity: warning
      annotations:
        summary: "审计日志写入失败"
        description: "审计日志写入错误率过高，请检查存储空间和配置"
        
    # 审计日志延迟
    - alert: AuditLogLatencyHigh
      expr: |
        histogram_quantile(0.99, rate(apiserver_audit_event_age_seconds_bucket[5m])) > 30
      for: 10m
      labels:
        severity: warning
      annotations:
        summary: "审计日志处理延迟"
        description: "99%的审计事件处理时间超过30秒"
```

### Grafana 仪表板配置

```json
{
  "dashboard": {
    "title": "Kubernetes 审计监控",
    "panels": [
      {
        "title": "审计事件总数",
        "type": "stat",
        "targets": [
          {
            "expr": "sum(apiserver_audit_event_total)",
            "legendFormat": "总事件数"
          }
        ]
      },
      {
        "title": "按资源类型的审计事件",
        "type": "piechart",
        "targets": [
          {
            "expr": "sum by (objectRef_resource) (apiserver_audit_event_total)",
            "legendFormat": "{{objectRef_resource}}"
          }
        ]
      },
      {
        "title": "敏感操作趋势",
        "type": "graph",
        "targets": [
          {
            "expr": "sum(rate(apiserver_audit_event_total{objectRef_resource=~\"secrets|serviceaccounts/token\"}[5m]))",
            "legendFormat": "敏感操作"
          }
        ]
      },
      {
        "title": "RBAC变更历史",
        "type": "table",
        "targets": [
          {
            "expr": "apiserver_audit_event_total{objectRef_group=\"rbac.authorization.k8s.io\"}",
            "format": "table"
          }
        ]
      },
      {
        "title": "审计日志延迟分布",
        "type": "heatmap",
        "targets": [
          {
            "expr": "rate(apiserver_audit_event_age_seconds_bucket[5m])",
            "format": "heatmap",
            "legendFormat": "{{le}}"
          }
        ]
      }
    ]
  }
}
```

<!-- chunk: 审计日志分析脚本 -->
## 审计日志分析脚本

```bash
#!/bin/bash
# 08-audit-analysis.sh

AUDIT_LOG_DIR="/var/log/kubernetes/audit"
REPORT_DIR="/var/reports/audit"

mkdir -p $REPORT_DIR

# 1. 生成每日摘要报告
daily_summary() {
    DATE=$(date +%Y-%m-%d)
    LOG_FILE="$AUDIT_LOG_DIR/audit.log"
    
    echo "=== Kubernetes 审计日志摘要报告 ($DATE) ===" > $REPORT_DIR/daily-summary-$DATE.md
    
    # 统计各类操作
    echo "## 操作统计" >> $REPORT_DIR/daily-summary-$DATE.md
    echo "" >> $REPORT_DIR/daily-summary-$DATE.md
    
    for verb in get list watch create update patch delete; do
        count=$(grep "\"verb\":\"$verb\"" $LOG_FILE | wc -l)
        echo "- $verb: $count" >> $REPORT_DIR/daily-summary-$DATE.md
    done
    
    # 敏感操作统计
    echo "" >> $REPORT_DIR/daily-summary-$DATE.md
    echo "## 敏感操作" >> $REPORT_DIR/daily-summary-$DATE.md
    echo "" >> $REPORT_DIR/daily-summary-$DATE.md
    
    secrets_ops=$(grep "\"objectRef\":{\"resource\":\"secrets\"" $LOG_FILE | wc -l)
    sa_token_ops=$(grep "\"objectRef\":{\"resource\":\"serviceaccounts/token\"" $LOG_FILE | wc -l)
    rbac_ops=$(grep "\"objectRef\":{\"group\":\"rbac.authorization.k8s.io\"" $LOG_FILE | wc -l)
    
    echo "- Secret操作: $secrets_ops" >> $REPORT_DIR/daily-summary-$DATE.md
    echo "- SA Token操作: $sa_token_ops" >> $REPORT_DIR/daily-summary-$DATE.md
    echo "- RBAC变更: $rbac_ops" >> $REPORT_DIR/daily-summary-$DATE.md
    
    # 异常行为检测
    echo "" >> $REPORT_DIR/daily-summary-$DATE.md
    echo "## 异常行为检测" >> $REPORT_DIR/daily-summary-$DATE.md
    echo "" >> $REPORT_DIR/daily-summary-$DATE.md
    
    # 检测高频操作用户
    echo "高频操作用户:" >> $REPORT_DIR/daily-summary-$DATE.md
    grep "\"user\":{\"username\":" $LOG_FILE | \
        sed 's/.*"username":"\([^"]*\)".*/\1/' | \
        sort | uniq -c | sort -nr | head -10 >> $REPORT_DIR/daily-summary-$DATE.md
    
    echo "报告已生成: $REPORT_DIR/daily-summary-$DATE.md"
}

# 2. 安全事件分析
security_analysis() {
    echo "=== 安全事件分析 ==="
    
    # 检测可疑的Pod执行操作
    echo "可疑的Pod执行操作:"
    grep "\"resource\":\"pods/exec\"" $AUDIT_LOG_DIR/audit.log | \
        jq -r '.user.username + " -> " + .objectRef.namespace + "/" + .objectRef.name' | \
        sort | uniq -c | sort -nr | head -5
    
    # 检测大量Secret访问
    echo -e "\n大量Secret访问:"
    grep "\"resource\":\"secrets\"" $AUDIT_LOG_DIR/audit.log | \
        jq -r '.user.username' | sort | uniq -c | sort -nr | head -5
    
    # 检测RBAC权限提升
    echo -e "\nRBAC权限变更:"
    grep "\"group\":\"rbac.authorization.k8s.io\"" $AUDIT_LOG_DIR/audit.log | \
        jq -r '.user.username + " " + .verb + " " + .objectRef.resource' | \
        sort | uniq -c | sort -nr
}

# 执行分析
daily_summary
security_analysis
```

<!-- chunk: 合规性最佳实践 -->
## 合规性最佳实践

| 实践项 | 说明 | 实施建议 | 优先级 |
|-------|------|---------|-------|
| **审计全覆盖** | 所有敏感操作必须审计 | 配置完整的审计策略 | P0 |
| **日志保护** | 审计日志必须保护 | 权限控制，防篡改 | P0 |
| **定期审查** | 定期审查审计日志 | 建立审查流程 | P1 |
| **长期保留** | 符合法规要求保留 | 至少保留1年 | P0 |
| **实时告警** | 异常行为实时告警 | 配置监控告警 | P0 |
| **合规报告** | 定期生成合规报告 | 自动化报告生成 | P1 |

---
**审计合规原则**: 全面覆盖 + 实时监控 + 长期保留 + 定期审查
---
**表格底部标记**: Kusheet Project, 作者 Allen Galler (allengaller@gmail.com)

---

<!-- chunk: Obsidian 相关文档 -->
## Obsidian 相关文档

- [[domain-05-security-compliance/MOC.md|domain-05-security-compliance MOC]]
- [[domain-05-security-compliance/README.md|Security Domain]]
- [[domain-05-security-compliance/00-open-source-projects-index.md|Domain-7 安全 — 开源项目索引]]
- [[domain-05-security-compliance/01-authentication-authorization-system.md|Kubernetes 认证授权体系详解]]
- [[domain-05-security-compliance/02-network-security-policies.md|网络安全策略与零信任架构]]
- [[domain-05-security-compliance/03-runtime-security-defense.md|运行时安全防护与威胁检测]]
- [[domain-05-security-compliance/05-policy-validation-tools.md|05 - 策略校验与准入控制工具 (Policy Validation)]]
- [[domain-05-security-compliance/06-pod-security-standards.md|06 - Pod安全标准详解]]
- [[domain-05-security-compliance/07-rbac-matrix-configuration.md|07 - RBAC权限矩阵表]]
- [[domain-05-security-compliance/08-security-best-practices.md|08 - 安全最佳实践表]]
- [[domain-05-security-compliance/09-security-hardening-production.md|Kubernetes 安全加固]]
- [[domain-05-security-compliance/10-certificate-management.md|证书管理与 TLS 配置]]

## See Also

- [[domain-05-security-compliance/02-network-security-policies.md|02-network-security-policies]]
- [[domain-05-security-compliance/03-runtime-security-defense.md|03-runtime-security-defense]]
- [[domain-05-security-compliance/05-policy-validation-tools.md|05-policy-validation-tools]]
- [[domain-05-security-compliance/06-pod-security-standards.md|06-pod-security-standards]]

- [[domain-05-security-compliance/README.md|返回目录]]

## Related

- [[domain-19-landscape-references/topic-index/security-index|Security 安全知识图谱索引]]
