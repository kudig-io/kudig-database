# 30 - 监控告警故障排查 (Monitoring and Alerting Troubleshooting)

> **适用版本**: Kubernetes v1.25-v1.32 | **最后更新**: 2026-02 | **参考**: [Prometheus Monitoring](https://prometheus.io/docs/), [Kubernetes Monitoring](https://kubernetes.io/docs/tasks/debug/debug-cluster/resource-metrics-pipeline/)

---

## 1. 监控告警故障诊断总览 (Monitoring and Alerting Diagnosis Overview)

### 1.1 常见故障现象分类

| 故障类型 | 症状表现 | 影响范围 | 紧急程度 |
|---------|---------|---------|---------|
| **监控数据缺失** | 指标采集失败 | 可见性丧失 | P0 - 紧急 |
| **告警不触发** | 严重问题未告警 | 故障发现延迟 | P0 - 紧急 |
| **告警风暴** | 大量误报 | 运维疲劳 | P1 - 高 |
| **监控组件故障** | Prometheus/Grafana宕机 | 监控系统瘫痪 | P0 - 紧急 |
| **指标延迟** | 数据更新滞后 | 决策依据不准 | P1 - 高 |

### 1.2 监控告警架构回顾

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                 监控告警故障诊断架构                                          │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  ┌─────────────────────────────────────────────────────────────────────┐  │
│  │                       数据采集层                                     │  │
│  │  ┌─────────────┐    ┌─────────────┐    ┌─────────────┐              │  │
│  │  │   Node      │    │   Pod       │    │   Service   │              │  │
│  │  │  Exporter   │    │  Exporter   │    │  Monitor    │              │  │
│  │  └─────────────┘    └─────────────┘    └─────────────┘              │  │
│  └─────────────────────────────────────────────────────────────────────┘  │
│                              │                                              │
│         ┌────────────────────┼────────────────────┐                       │
│         │                    │                    │                       │
│         ▼                    ▼                    ▼                       │
│  ┌─────────────┐    ┌─────────────┐    ┌─────────────┐                   │
│  │   指标聚合   │   │   日志收集   │   │   追踪数据   │                   │
│  │ (Prometheus)│   │ (Loki/EFK)  │   │ (Jaeger)    │                   │
│  │   存储      │   │   存储      │   │   存储      │                   │
│  └─────────────┘    └─────────────┘    └─────────────┘                   │
│         │                    │                    │                       │
│         ▼                    ▼                    ▼                       │
│  ┌─────────────────────────────────────────────────────────────────────┐  │
│  │                      告警处理层                                     │  │
│  │  ┌───────────────────────────────────────────────────────────────┐  │  │
│  │  │                     Alertmanager                              │  │  │
│  │  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐           │  │  │
│  │  │  │   告警分组   │  │   抑制机制   │  │   通知发送   │           │  │  │
│  │  │  │ (Grouping)  │  │ (Inhibition)│  │  (Dispatch) │           │  │  │
│  │  │  └─────────────┘  └─────────────┘  └─────────────┘           │  │  │
│  │  └───────────────────────────────────────────────────────────────┘  │  │
│  └─────────────────────────────────────────────────────────────────────┘  │
│                              │                                              │
│         ┌────────────────────┼────────────────────┐                       │
│         │                    │                    │                       │
│         ▼                    ▼                    ▼                       │
│  ┌─────────────┐    ┌─────────────┐    ┌─────────────┐                   │
│  │   可视化     │   │   告警通知   │   │   自动修复   │                   │
│  │ (Grafana)   │   │ (Webhook)   │   │ (Runbook)   │                   │
│  │   仪表板     │   │   集成      │   │   执行      │                   │
│  └─────────────┘    └─────────────┘    └─────────────┘                   │
│                              │                                              │
│                              ▼                                              │
│  ┌─────────────────────────────────────────────────────────────────────┐  │
│  │                      运维响应层                                     │  │
│  │  ┌─────────────┐    ┌─────────────┐    ┌─────────────┐              │  │
│  │  │   SRE团队   │    │   开发团队   │   │   管理层     │              │  │
│  │  │ (PagerDuty) │    │ (Slack)     │   │ (Email)     │              │  │
│  │  └─────────────┘    └─────────────┘    └─────────────┘              │  │
│  └─────────────────────────────────────────────────────────────────────┘  │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

---

## 2. 监控组件状态检查 (Monitoring Components Status Check)

### 2.1 Prometheus基础状态验证

```bash
# ========== 1. Prometheus Pod状态 ==========
# 检查Prometheus组件状态
kubectl get pods -n monitoring -l app=prometheus

# 查看Prometheus详细信息
kubectl describe pods -n monitoring -l app=prometheus

# 检查Prometheus服务
kubectl get service -n monitoring prometheus-k8s

# 验证Prometheus配置
kubectl get configmap -n monitoring prometheus-config -o yaml

# ========== 2. 指标采集状态 ==========
# 检查目标抓取状态
kubectl port-forward -n monitoring svc/prometheus-k8s 9090:9090 &
sleep 5
curl -s http://localhost:9090/api/v1/targets | jq '.data.activeTargets[] | select(.health!="up")'

# 查看抓取统计
curl -s http://localhost:9090/api/v1/targets | jq '{
    total: (.data.activeTargets | length),
    up: (.data.activeTargets | map(select(.health=="up")) | length),
    down: (.data.activeTargets | map(select(.health=="down")) | length)
}'

# 检查特定job状态
curl -s http://localhost:9090/api/v1/targets | jq '.data.activeTargets[] | select(.labels.job=="kubernetes-nodes")'

# ========== 3. 存储和性能检查 ==========
# 检查Prometheus存储使用
kubectl exec -n monitoring prometheus-k8s-0 -- df -h /prometheus

# 查看TSDB状态
curl -s http://localhost:9090/api/v1/status/tsdb | jq

# 检查查询性能
curl -s http://localhost:9090/api/v1/query?query=up | jq '.status'

# 清理端口转发
kill %1 2>/dev/null
```

### 2.2 Grafana可视化检查

```bash
# ========== Grafana状态验证 ==========
# 检查Grafana Pod状态
kubectl get pods -n monitoring -l app=grafana

# 验证Grafana服务
kubectl get service -n monitoring grafana

# 检查Grafana数据源配置
kubectl get configmap -n monitoring grafana-datasources -o yaml

# 测试Grafana连接
GRAFANA_POD=$(kubectl get pods -n monitoring -l app=grafana -o jsonpath='{.items[0].metadata.name}')
kubectl port-forward -n monitoring $GRAFANA_POD 3000:3000 &
sleep 5

# 测试数据源连接
curl -s -u admin:admin http://localhost:3000/api/datasources | jq '.[] | {name: .name, type: .type, status: .status}'

# 检查仪表板加载
curl -s -u admin:admin http://localhost:3000/api/search | jq '.[] | {title: .title, uid: .uid}'

# 清理端口转发
kill %1 2>/dev/null

# ========== 仪表板健康检查 ==========
# 检查核心仪表板是否存在
CORE_DASHBOARDS=("Kubernetes Cluster", "Nodes", "Pods", "Deployments")
for dashboard in "${CORE_DASHBOARDS[@]}"; do
    echo "Checking dashboard: $dashboard"
    kubectl get configmap -n monitoring | grep -i "$dashboard" && echo "  ✓ Found" || echo "  ✗ Missing"
done
```

---

## 3. 指标采集问题排查 (Metrics Collection Issues)

### 3.1 指标缺失问题

```bash
# ========== 1. Exporter状态检查 ==========
# 检查Node Exporter状态
kubectl get daemonset -n monitoring node-exporter
kubectl get pods -n monitoring -l app=node-exporter

# 验证Node Exporter指标端点
NODE_IP=$(kubectl get nodes -o jsonpath='{.items[0].status.addresses[?(@.type=="InternalIP")].address}')
NODE_EXPORTER_PORT=$(kubectl get service -n monitoring node-exporter -o jsonpath='{.spec.ports[0].port}')

curl -s http://$NODE_IP:$NODE_EXPORTER_PORT/metrics | head -20

# 检查Kube-State-Metrics
kubectl get deployment -n monitoring kube-state-metrics
kubectl get pods -n monitoring -l app.kubernetes.io/name=kube-state-metrics

# 验证集群状态指标
curl -s http://localhost:8080/metrics --resolve kube-state-metrics.monitoring:8080:$(kubectl get ep -n monitoring kube-state-metrics -o jsonpath='{.subsets[0].addresses[0].ip}') | head -10

# ========== 2. ServiceMonitor配置检查 ==========
# 查看ServiceMonitor配置
kubectl get servicemonitor -n monitoring

# 检查特定ServiceMonitor
kubectl describe servicemonitor -n monitoring prometheus-k8s

# 验证目标发现配置
kubectl get configmap -n monitoring prometheus-k8s -o yaml | grep -A20 "kubernetes_sd_configs"

# ========== 3. 指标路径和端口验证 ==========
# 检查Pod指标端点
kubectl get pods -n <application-namespace> -o jsonpath='{
    range .items[*]
}{
        .metadata.name
}{
        "\t"
}{
        .spec.containers[*].ports[*].containerPort
}{
        "\t"
}{
        .spec.containers[*].ports[*].name
}{
        "\n"
}{
    end
}'

# 测试应用指标端点
APP_POD=$(kubectl get pods -n <application-namespace> -o jsonpath='{.items[0].metadata.name}')
kubectl port-forward -n <application-namespace> $APP_POD <port>:<port> &
sleep 3
curl -s http://localhost:<port>/metrics | head -10
kill %1
```

### 3.2 采集配置问题

```bash
# ========== 采集规则验证 ==========
# 检查Prometheus配置
kubectl get configmap -n monitoring prometheus-k8s -o yaml | grep -A50 "scrape_configs"

# 验证采集间隔配置
kubectl get configmap -n monitoring prometheus-k8s -o yaml | grep -E "(scrape_interval|evaluation_interval)"

# 检查采集超时设置
kubectl get configmap -n monitoring prometheus-k8s -o yaml | grep "scrape_timeout"

# ========== TLS和认证配置 ==========
# 检查证书配置
kubectl get secrets -n monitoring | grep -E "(tls|cert)"

# 验证客户端证书
kubectl get secret -n monitoring prometheus-client-certs -o yaml

# 测试HTTPS指标端点
curl -s --cert /path/to/client.crt --key /path/to/client.key https://secure-endpoint:port/metrics

# ========== 网络策略影响 ==========
# 检查网络策略限制
kubectl get networkpolicy -n monitoring

# 验证Pod间网络连通性
kubectl run network-test --image=busybox -n monitoring -it --rm -- sh -c "
wget -q -O - http://prometheus-k8s:9090/-/healthy || echo 'Cannot reach Prometheus'
"
```

---

## 4. 告警系统故障排查 (Alerting System Issues)

### 4.1 Alertmanager状态检查

```bash
# ========== 1. Alertmanager基础状态 ==========
# 检查Alertmanager Pod状态
kubectl get pods -n monitoring -l alertmanager=main

# 查看Alertmanager配置
kubectl get configmap -n monitoring alertmanager-main -o yaml

# 验证Alertmanager集群状态
kubectl port-forward -n monitoring svc/alertmanager-main 9093:9093 &
sleep 3

# 检查集群成员状态
curl -s http://localhost:9093/api/v2/status | jq '.clusterStatus.peers'

# 查看当前活跃告警
curl -s http://localhost:9093/api/v2/alerts | jq 'length'

# 清理端口转发
kill %1 2>/dev/null

# ========== 2. 告警规则验证 ==========
# 检查PrometheusRule配置
kubectl get prometheusrule -n monitoring

# 查看具体告警规则
kubectl get prometheusrule -n monitoring k8s.rules -o yaml

# 验证规则语法
kubectl port-forward -n monitoring svc/prometheus-k8s 9090:9090 &
sleep 3

# 测试规则评估
curl -s "http://localhost:9090/api/v1/rules" | jq '.data.groups[] | select(.name=="k8s.rules")'

# 检查规则执行状态
curl -s "http://localhost:9090/api/v1/alerts" | jq

# 清理端口转发
kill %1 2>/dev/null
```

### 4.2 告警不触发问题

```bash
# ========== 告警条件验证 ==========
# 测试告警表达式
ALERT_EXPRESSION='up == 0'
curl -s "http://localhost:9090/api/v1/query?query=$ALERT_EXPRESSION" | jq

# 检查指标数据存在性
curl -s "http://localhost:9090/api/v1/series?match[]=up" | jq

# 验证时间序列数据
curl -s "http://localhost:9090/api/v1/query_range?query=up&start=$(date -d '1 hour ago' +%s)&end=$(date +%s)&step=60" | jq

# ========== 告警抑制和分组检查 ==========
# 检查抑制规则
kubectl get configmap -n monitoring alertmanager-main -o yaml | grep -A20 "inhibit_rules"

# 验证分组配置
kubectl get configmap -n monitoring alertmanager-main -o yaml | grep -A10 "route"

# 测试告警分组
echo 'Checking alert grouping configuration...'
kubectl get configmap -n monitoring alertmanager-main -o yaml | grep -E "(group_by|group_wait|group_interval)"

# ========== 告警静默和过滤 ==========
# 检查活动静默规则
curl -s http://localhost:9093/api/v2/silences | jq '.[] | select(.status.state=="active")'

# 验证接收器配置
kubectl get configmap -n monitoring alertmanager-main -o yaml | grep -A20 "receivers"

# 测试通知通道
curl -s http://localhost:9093/api/v2/receivers | jq
```

---

## 5. 告警风暴和误报问题 (Alert Storm and False Positive Issues)

### 5.1 告警频率分析

```bash
# ========== 告警统计分析 ==========
# 收集告警历史数据
kubectl port-forward -n monitoring svc/alertmanager-main 9093:9093 &
sleep 3

# 获取过去24小时的告警
curl -s "http://localhost:9093/api/v2/alerts?active=false&silenced=false&inhibited=false" | \
jq -r '.[] | "\(.startsAt) \(.labels.alertname) \(.labels.severity)"' | \
sort > /tmp/alerts-24h.log

# 分析告警频率
echo "=== Alert Frequency Analysis ==="
cat /tmp/alerts-24h.log | awk '{print $2}' | sort | uniq -c | sort -nr

# 分析告警严重程度分布
echo "=== Severity Distribution ==="
cat /tmp/alerts-24h.log | awk '{print $3}' | sort | uniq -c

# 识别高频告警
echo "=== Top 10 Frequent Alerts ==="
cat /tmp/alerts-24h.log | awk '{print $2}' | sort | uniq -c | sort -nr | head -10

# 清理端口转发和临时文件
kill %1 2>/dev/null
rm /tmp/alerts-24h.log

# ========== 告警关联性分析 ==========
# 分析同时发生的告警
cat <<'EOF' > alert-correlation-analyzer.sh
#!/bin/bash

echo "=== Alert Correlation Analysis ==="

# 收集时间窗口内的告警
TIME_WINDOW="10m"  # 10分钟时间窗口
kubectl port-forward -n monitoring svc/alertmanager-main 9093:9093 &
sleep 3

# 获取最近的告警
curl -s "http://localhost:9093/api/v2/alerts" | jq -r '.[] | "\(.startsAt) \(.labels.alertname)"' > /tmp/current-alerts.log

# 分析告警时间聚类
echo "Alerts in clusters:"
awk '{
    timestamp = $1
    alertname = $2
    # 简单的时间分组逻辑
    split(timestamp, parts, "T")
    date_part = parts[1]
    time_part = parts[2]
    split(time_part, time_parts, ":")
    hour = time_parts[1]
    minute = int(time_parts[2] / 10) * 10  # 按10分钟分组
    time_key = date_part " " hour ":" minute
    clusters[time_key][alertname]++
}
END {
    for (time_key in clusters) {
        printf "%s:\n", time_key
        for (alert in clusters[time_key]) {
            printf "  %s: %d\n", alert, clusters[time_key][alert]
        }
        print ""
    }
}' /tmp/current-alerts.log

kill %1 2>/dev/null
rm /tmp/current-alerts.log
EOF

chmod +x alert-correlation-analyzer.sh
```

### 5.2 告警规则优化

```bash
# ========== 告警规则调优 ==========
# 创建优化的告警规则
cat <<EOF > optimized-alert-rules.yaml
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: optimized-alerts
  namespace: monitoring
spec:
  groups:
  - name: optimized.rules
    rules:
    # 优化CPU使用率告警
    - alert: HighCPUUsageOptimized
      expr: rate(container_cpu_usage_seconds_total[5m]) > 0.8
      for: 10m  # 增加持续时间减少抖动
      labels:
        severity: warning
      annotations:
        summary: "High CPU usage detected ({{ \$value }}%)"
        
    # 添加告警抑制规则
    - alert: NodeDown
      expr: up{job="kubernetes-nodes"} == 0
      for: 2m
      labels:
        severity: critical
      annotations:
        summary: "Node {{ \$labels.instance }} is down"
        
    # 避免重复告警
    - alert: MultiplePodsCrashing
      expr: rate(kube_pod_container_status_restarts_total[10m]) > 5
      for: 15m  # 更长的持续时间
      labels:
        severity: critical
      annotations:
        summary: "Multiple pods crashing in namespace {{ \$labels.namespace }}"
        
    # 添加智能抑制
    - alert: ClusterMemoryPressure
      expr: node_memory_MemAvailable_bytes / node_memory_MemTotal_bytes < 0.1
      for: 5m
      labels:
        severity: warning
      annotations:
        summary: "Cluster memory pressure ({{ \$value }}% available)"
EOF

# ========== 告警去重配置 ==========
# 配置Alertmanager去重策略
cat <<EOF > alertmanager-deduplication.yaml
apiVersion: v1
kind: ConfigMap
metadata:
  name: alertmanager-main
  namespace: monitoring
data:
  alertmanager.yaml: |
    global:
      resolve_timeout: 5m
    
    route:
      group_by: ['alertname', 'job']
      group_wait: 30s
      group_interval: 5m
      repeat_interval: 3h  # 减少重复通知
      
      # 告警抑制规则
      inhibit_rules:
      - source_match:
          alertname: 'NodeDown'
        target_match:
          alertname: 'ServiceDown'
        equal: ['instance']
        
      - source_match:
          alertname: 'ClusterMemoryPressure'
        target_match:
          alertname: 'PodEvicted'
        equal: ['namespace']
        
      routes:
      - match:
          severity: 'critical'
        group_interval: 2m
        repeat_interval: 1h
        
      - match:
          severity: 'warning'
        group_interval: 10m
        repeat_interval: 6h
EOF
```

---

## 6. 监控性能和容量规划 (Monitoring Performance and Capacity Planning)

### 6.1 监控系统性能优化

```bash
# ========== 性能指标收集 ==========
# 监控Prometheus性能
cat <<'EOF' > prometheus-performance-monitor.sh
#!/bin/bash

echo "=== Prometheus Performance Monitor ==="

kubectl port-forward -n monitoring svc/prometheus-k8s 9090:9090 &
sleep 3

# 收集关键性能指标
METRICS=(
    "prometheus_tsdb_head_series"
    "prometheus_tsdb_head_chunks"
    "prometheus_tsdb_head_gc_duration_seconds"
    "prometheus_target_interval_length_seconds"
    "prometheus_engine_queries"
    "prometheus_engine_queries_concurrent_max"
)

for metric in "${METRICS[@]}"; do
    echo "Metric: $metric"
    curl -s "http://localhost:9090/api/v1/query?query=$metric" | jq '.data.result[0].value[1]'
    echo ""
done

# 检查存储使用情况
echo "Storage usage:"
kubectl exec -n monitoring prometheus-k8s-0 -- du -sh /prometheus

# 检查内存使用
echo "Memory usage:"
kubectl top pods -n monitoring -l app=prometheus

kill %1 2>/dev/null
EOF

chmod +x prometheus-performance-monitor.sh

# ========== 容量规划工具 ==========
# 创建容量规划脚本
cat <<'EOF' > monitoring-capacity-planner.sh
#!/bin/bash

echo "=== Monitoring Capacity Planning ==="

# 计算当前指标数量
kubectl port-forward -n monitoring svc/prometheus-k8s 9090:9090 &
sleep 3

SERIES_COUNT=$(curl -s "http://localhost:9090/api/v1/query?query=prometheus_tsdb_head_series" | jq -r '.data.result[0].value[1]')
echo "Current time series: $SERIES_COUNT"

# 计算增长率
echo "Calculating growth rate..."
# 这里可以添加历史数据分析逻辑

# 预测未来需求
DAYS_TO_PLAN=${1:-30}
GROWTH_RATE=${2:-0.1}  # 10%月增长率

PROJECTED_SERIES=$(echo "$SERIES_COUNT * (1 + $GROWTH_RATE) ^ ($DAYS_TO_PLAN / 30)" | bc)
echo "Projected series in $DAYS_TO_PLAN days: $PROJECTED_SERIES"

# 计算存储需求
STORAGE_PER_SERIES_PER_DAY_KB=2  # 每个时间序列每天约2KB
RETENTION_DAYS=15

REQUIRED_STORAGE_KB=$(echo "$PROJECTED_SERIES * $STORAGE_PER_SERIES_PER_DAY_KB * $RETENTION_DAYS" | bc)
REQUIRED_STORAGE_GB=$(echo "scale=2; $REQUIRED_STORAGE_KB / 1024 / 1024" | bc)

echo "Required storage: ${REQUIRED_STORAGE_GB}GB"

# 检查当前存储配置
CURRENT_STORAGE=$(kubectl get pvc -n monitoring prometheus-prometheus-k8s-db-prometheus-k8s-0 -o jsonpath='{.spec.resources.requests.storage}')
echo "Current storage allocation: $CURRENT_STORAGE"

kill %1 2>/dev/null
EOF

chmod +x monitoring-capacity-planner.sh
```

### 6.2 监控系统高可用配置

```bash
# ========== 高可用Prometheus配置 ==========
# 配置Prometheus联邦
cat <<EOF > prometheus-federation.yaml
apiVersion: monitoring.coreos.com/v1
kind: Prometheus
metadata:
  name: prometheus-federated
  namespace: monitoring
spec:
  replicas: 2
  retention: 15d
  externalLabels:
    cluster: primary
  remoteWrite:
  - url: http://prometheus-remote:9090/api/v1/write
    writeRelabelConfigs:
    - sourceLabels: [__name__]
      regex: 'up|prometheus_.*'
      action: keep
  additionalScrapeConfigs:
    name: additional-scrape-configs
    key: prometheus-additional.yaml
EOF

# ========== 备份和恢复策略 ==========
# 创建监控数据备份Job
cat <<EOF > monitoring-backup-job.yaml
apiVersion: batch/v1
kind: CronJob
metadata:
  name: prometheus-backup
  namespace: monitoring
spec:
  schedule: "0 2 * * *"  # 每天凌晨2点执行
  jobTemplate:
    spec:
      template:
        spec:
          containers:
          - name: backup
            image: prometheus/prometheus:v2.40.0
            command:
            - /bin/sh
            - -c
            - |
              # 创建快照
              curl -XPOST http://prometheus-k8s:9090/api/v1/admin/tsdb/snapshot
              
              # 压缩备份数据
              SNAPSHOT_DIR=\$(ls -t /prometheus/snapshots/ | head -1)
              tar -czf /backup/prometheus-snapshot-\$(date +%Y%m%d-%H%M%S).tar.gz -C /prometheus/snapshots/ \$SNAPSHOT_DIR
              
              # 上传到对象存储
              aws s3 cp /backup/prometheus-snapshot-\$(date +%Y%m%d-%H%M%S).tar.gz s3://monitoring-backups/
            volumeMounts:
            - name: prometheus-storage
              mountPath: /prometheus
            - name: backup-storage
              mountPath: /backup
          volumes:
          - name: prometheus-storage
            persistentVolumeClaim:
              claimName: prometheus-prometheus-k8s-db-prometheus-k8s-0
          - name: backup-storage
            emptyDir: {}
          restartPolicy: OnFailure
EOF

# ========== 灾难恢复计划 ==========
# 创建恢复脚本
cat <<'EOF' > monitoring-recovery-plan.sh
#!/bin/bash

BACKUP_FILE=$1
RESTORE_TIME=${2:-$(date +%Y-%m-%dT%H:%M:%S)}

if [ -z "$BACKUP_FILE" ]; then
    echo "Usage: $0 <backup-file> [restore-time]"
    exit 1
fi

echo "=== Monitoring System Recovery Plan ==="
echo "Backup file: $BACKUP_FILE"
echo "Restore time: $RESTORE_TIME"

# 1. 停止监控组件
echo "1. Stopping monitoring components..."
kubectl scale deployment -n monitoring prometheus-k8s --replicas=0
kubectl scale deployment -n monitoring alertmanager-main --replicas=0

# 2. 恢复数据
echo "2. Restoring data from backup..."
# 解压备份文件到适当位置
# tar -xzf $BACKUP_FILE -C /prometheus/data/

# 3. 启动组件
echo "3. Starting monitoring components..."
kubectl scale deployment -n monitoring prometheus-k8s --replicas=2
kubectl scale deployment -n monitoring alertmanager-main --replicas=3

# 4. 验证恢复
echo "4. Verifying recovery..."
sleep 60
kubectl port-forward -n monitoring svc/prometheus-k8s 9090:9090 &
sleep 5
curl -s http://localhost:9090/api/v1/query?query=up | jq '.status'
kill %1

echo "Recovery process completed"
EOF

chmod +x monitoring-recovery-plan.sh
```

---

## 7. 监控告警最佳实践 (Monitoring and Alerting Best Practices)

### 7.1 告警设计原则

```bash
# ========== 黄金信号监控 ==========
# 创建基于USE方法的告警规则
cat <<EOF > golden-signals-alerts.yaml
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: golden-signals
  namespace: monitoring
spec:
  groups:
  - name: golden-signals.rules
    rules:
    # 延迟 (Latency)
    - alert: HighRequestLatency
      expr: histogram_quantile(0.99, rate(http_request_duration_seconds_bucket[5m])) > 1
      for: 5m
      labels:
        severity: warning
      annotations:
        summary: "High request latency (99th percentile > 1s)"
        
    # 流量 (Traffic)
    - alert: UnusualTrafficPattern
      expr: abs(rate(http_requests_total[5m]) - rate(http_requests_total[1h] offset 1d)) / rate(http_requests_total[1h] offset 1d) > 0.5
      for: 10m
      labels:
        severity: warning
      annotations:
        summary: "Unusual traffic pattern detected"
        
    # 错误 (Errors)
    - alert: HighErrorRate
      expr: rate(http_requests_total{status=~"5.."}[5m]) / rate(http_requests_total[5m]) > 0.01
      for: 2m
      labels:
        severity: critical
      annotations:
        summary: "High error rate ({{ \$value }}%)"
        
    # 饱和度 (Saturation)
    - alert: HighCPUSaturation
      expr: rate(node_cpu_seconds_total{mode!="idle"}[5m]) > 0.9
      for: 5m
      labels:
        severity: warning
      annotations:
        summary: "High CPU saturation ({{ \$value }}%)"
        
    - alert: HighMemorySaturation
      expr: (node_memory_MemTotal_bytes - node_memory_MemAvailable_bytes) / node_memory_MemTotal_bytes > 0.9
      for: 5m
      labels:
        severity: warning
      annotations:
        summary: "High memory saturation ({{ \$value }}%)"
EOF
```

### 7.2 监控仪表板模板

```bash
# ========== 核心仪表板配置 ==========
# 创建集群概览仪表板
cat <<'EOF' > cluster-overview-dashboard.json
{
  "dashboard": {
    "title": "Kubernetes Cluster Overview",
    "panels": [
      {
        "title": "Cluster Health Status",
        "type": "stat",
        "targets": [
          {
            "expr": "sum(up{job=\"kubernetes-nodes\"})",
            "legendFormat": "Nodes Up"
          },
          {
            "expr": "count(kube_pod_status_ready{condition=\"true\"})",
            "legendFormat": "Pods Ready"
          }
        ]
      },
      {
        "title": "Resource Utilization",
        "type": "graph",
        "targets": [
          {
            "expr": "100 - (avg(rate(node_cpu_seconds_total{mode=\"idle\"}[5m])) * 100)",
            "legendFormat": "CPU Usage %"
          },
          {
            "expr": "(sum(node_memory_MemTotal_bytes) - sum(node_memory_MemAvailable_bytes)) / sum(node_memory_MemTotal_bytes) * 100",
            "legendFormat": "Memory Usage %"
          }
        ]
      },
      {
        "title": "Alert Summary",
        "type": "table",
        "targets": [
          {
            "expr": "count(ALERTS{alertstate=\"firing\"}) by (alertname, severity)",
            "legendFormat": "{{alertname}} ({{severity}})"
          }
        ]
      }
    ]
  }
}
EOF

# ========== 应用性能仪表板 ==========
cat <<'EOF' > application-performance-dashboard.json
{
  "dashboard": {
    "title": "Application Performance",
    "panels": [
      {
        "title": "Request Rate",
        "type": "graph",
        "targets": [
          {
            "expr": "rate(http_requests_total[5m])",
            "legendFormat": "{{job}} - {{method}} {{path}}"
          }
        ]
      },
      {
        "title": "Error Rate",
        "type": "graph",
        "targets": [
          {
            "expr": "rate(http_requests_total{status=~\"5..\"}[5m])",
            "legendFormat": "{{job}} - 5xx Errors"
          }
        ]
      },
      {
        "title": "Latency (95th Percentile)",
        "type": "graph",
        "targets": [
          {
            "expr": "histogram_quantile(0.95, rate(http_request_duration_seconds_bucket[5m]))",
            "legendFormat": "{{job}} - {{method}} {{path}}"
          }
        ]
      },
      {
        "title": "Resource Usage by Pod",
        "type": "table",
        "targets": [
          {
            "expr": "rate(container_cpu_usage_seconds_total[5m])",
            "legendFormat": "{{pod}} CPU"
          }
        ]
      }
    ]
  }
}
EOF
```

---