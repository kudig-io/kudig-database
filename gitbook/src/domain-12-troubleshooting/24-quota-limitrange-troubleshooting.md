# 24 - Quota/LimitRange 故障排查 (Quota/LimitRange Troubleshooting)

> **适用版本**: Kubernetes v1.25-v1.32 | **最后更新**: 2026-02 | **参考**: [Kubernetes Resource Quotas](https://kubernetes.io/docs/concepts/policy/resource-quotas/), [Kubernetes Limit Ranges](https://kubernetes.io/docs/concepts/policy/limit-range/)

---

## 1. 资源配额故障诊断总览 (Resource Quota Diagnosis Overview)

### 1.1 常见故障现象分类

| 故障类型 | 症状表现 | 影响范围 | 紧急程度 |
|---------|---------|---------|---------|
| **配额超限** | 资源创建被拒绝 | 应用部署失败 | P0 - 紧急 |
| **配额配置错误** | 限制不合理 | 资源浪费/不足 | P1 - 高 |
| **LimitRange冲突** | 容器启动失败 | Pod调度异常 | P0 - 紧急 |
| **配额计算错误** | 统计不准确 | 资源管理混乱 | P2 - 中 |
| **默认值缺失** | 容器资源配置不当 | 性能问题 | P1 - 高 |

### 1.2 资源配额架构回顾

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                   Quota/LimitRange 故障诊断架构                               │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  ┌─────────────────────────────────────────────────────────────────────┐  │
│  │                      Namespace资源层                                  │  │
│  │  ┌─────────────┐    ┌─────────────┐    ┌─────────────┐              │  │
│  │  │   Pod资源   │    │   存储资源   │   │   网络资源   │              │  │
│  │  │ (Compute)   │    │ (Storage)   │   │ (Network)   │              │  │
│  │  └─────────────┘    └─────────────┘    └─────────────┘              │  │
│  └─────────────────────────────────────────────────────────────────────┘  │
│                              │                                              │
│         ┌────────────────────┼────────────────────┐                       │
│         │                    │                    │                       │
│         ▼                    ▼                    ▼                       │
│  ┌─────────────┐    ┌─────────────┐    ┌─────────────┐                   │
│  │ Resource    │    │ LimitRange  │    │   默认值    │                   │
│  │ Quota       │    │   策略      │    │  (Defaults) │                   │
│  │   限制      │    │   限制      │    │   应用      │                   │
│  └─────────────┘    └─────────────┘    └─────────────┘                   │
│         │                    │                    │                       │
│         ▼                    ▼                    ▼                       │
│  ┌─────────────────────────────────────────────────────────────────────┐  │
│  │                   配额控制器                                       │  │
│  │  ┌───────────────────────────────────────────────────────────────┐  │  │
│  │  │                   kube-apiserver                              │  │  │
│  │  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐           │  │  │
│  │  │  │   Quota     │  │ LimitRange  │  │ Admission   │           │  │  │
│  │  │  │ Controller  │  │ Controller  │  │ Controller  │           │  │  │
│  │  │  └─────────────┘  └─────────────┘  └─────────────┘           │  │  │
│  │  └───────────────────────────────────────────────────────────────┘  │  │
│  └─────────────────────────────────────────────────────────────────────┘  │
│                              │                                              │
│         ┌────────────────────┼────────────────────┐                       │
│         │                    │                    │                       │
│         ▼                    ▼                    ▼                       │
│  ┌─────────────┐    ┌─────────────┐    ┌─────────────┐                   │
│  │   资源申请   │   │   资源分配   │   │   资源回收   │                   │
│  │ (Requests)  │   │ (Allocated) │   │ (Reclaimed) │                   │
│  │   验证      │   │   跟踪      │   │   管理      │                   │
│  └─────────────┘    └─────────────┘    └─────────────┘                   │
│                              │                                              │
│                              ▼                                              │
│  ┌─────────────────────────────────────────────────────────────────────┐  │
│  │                      监控告警层                                     │  │
│  │  ┌─────────────┐    ┌─────────────┐    ┌─────────────┐              │  │
│  │  │   使用率     │   │   告警       │   │   报告       │              │  │
│  │  │ (Usage)     │   │ (Alerts)    │   │ (Reports)   │              │  │
│  │  └─────────────┘    └─────────────┘    └─────────────┘              │  │
│  └─────────────────────────────────────────────────────────────────────┘  │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

---

## 2. ResourceQuota 基础状态检查 (ResourceQuota Basic Status Check)

### 2.1 配额资源配置验证

```bash
# ========== 1. 配额资源概览 ==========
# 查看所有ResourceQuota
kubectl get resourcequota --all-namespaces

# 查看特定Namespace的配额
kubectl get resourcequota -n <namespace-name>

# 查看配额详细信息
kubectl describe resourcequota -n <namespace-name>

# 检查配额配置
kubectl get resourcequota <quota-name> -n <namespace-name> -o yaml

# ========== 2. 配额使用情况检查 ==========
# 查看配额使用统计
kubectl get resourcequota -n <namespace-name> -o jsonpath='{
    .items[*].status.used
}'

# 检查硬限制
kubectl get resourcequota -n <namespace-name> -o jsonpath='{
    .items[*].status.hard
}'

# 计算使用率
kubectl get resourcequota -n <namespace-name> -o jsonpath='{
    .items[*].metadata.name
}{
        "\t"
}{
        .status.used.pods
}{
        "/"
}{
        .status.hard.pods
}{
        "\t"
}{
        .status.used."requests.cpu"
}{
        "/"
}{
        .status.hard."requests.cpu"
}{
        "\n"
}'

# ========== 3. 配额类型检查 ==========
# 检查计算资源配额
kubectl get resourcequota -n <namespace-name> -o jsonpath='{
    .items[0].status.hard."requests.cpu"
}{
        "\t"
}{
        .items[0].status.hard."requests.memory"
}{
        "\t"
}{
        .items[0].status.hard."limits.cpu"
}{
        "\t"
}{
        .items[0].status.hard."limits.memory"
}'

# 检查对象计数配额
kubectl get resourcequota -n <namespace-name> -o jsonpath='{
    .items[0].status.hard.pods
}{
        "\t"
}{
        .items[0].status.hard.services
}{
        "\t"
}{
        .items[0].status.hard.persistentvolumeclaims
}'

# 检查存储配额
kubectl get resourcequota -n <namespace-name> -o jsonpath='{
    .items[0].status.hard."requests.storage"
}{
        "\t"
}{
        .items[0].status.hard."persistentvolumeclaims"
}'
```

### 2.2 配额超限问题诊断

```bash
# ========== 1. 超限事件检查 ==========
# 查看配额超限事件
kubectl get events -n <namespace-name> --field-selector reason=FailedCreate

# 检查具体的错误信息
kubectl get events -n <namespace-name> --field-selector reason=FailedCreate -o jsonpath='{
    range .items[*]
}{
        .message
}{
        "\n"
}{
    end
}' | grep -i quota

# 分析超限原因
kubectl describe resourcequota <quota-name> -n <namespace-name> | grep -A10 "Status:"

# ========== 2. 资源请求分析 ==========
# 分析Pod资源请求
kubectl get pods -n <namespace-name> -o jsonpath='{
    range .items[*]
}{
        .metadata.name
}{
        "\t"
}{
        .spec.containers[*].resources.requests.cpu
}{
        "\t"
}{
        .spec.containers[*].resources.requests.memory
}{
        "\n"
}{
    end
}'

# 统计总资源使用
kubectl get pods -n <namespace-name> -o jsonpath='{
    range .items[*]
}{
        .spec.containers[*].resources.requests.cpu
}{
        " "
}{
    end
}' | tr ' ' '\n' | grep -v '^$' | awk '{sum+=$1} END {print "Total CPU requests:", sum}'

# ========== 3. 配额调整建议 ==========
# 计算合理的配额值
CURRENT_PODS=$(kubectl get pods -n <namespace-name> --no-headers | wc -l)
CURRENT_CPU_REQUESTS=$(kubectl get pods -n <namespace-name> -o jsonpath='{range .items[*]}{.spec.containers[*].resources.requests.cpu}{"\n"}{end}' | awk '{sum+=$1} END {print sum}')
CURRENT_MEM_REQUESTS=$(kubectl get pods -n <namespace-name> -o jsonpath='{range .items[*]}{.spec.containers[*].resources.requests.memory}{"\n"}{end}' | sed 's/Gi//g' | awk '{sum+=$1} END {print sum}')

echo "Current usage analysis:"
echo "Pods: $CURRENT_PODS"
echo "CPU requests: ${CURRENT_CPU_REQUESTS} cores"
echo "Memory requests: ${CURRENT_MEM_REQUESTS}Gi"

# 建议配额值 (增加20%缓冲)
RECOMMENDED_PODS=$((CURRENT_PODS * 120 / 100))
RECOMMENDED_CPU=$(echo "$CURRENT_CPU_REQUESTS * 1.2" | bc)
RECOMMENDED_MEM=$(echo "$CURRENT_MEM_REQUESTS * 1.2" | bc)

echo "Recommended quota values:"
echo "Pods: $RECOMMENDED_PODS"
echo "CPU requests: ${RECOMMENDED_CPU} cores"
echo "Memory requests: ${RECOMMENDED_MEM}Gi"
```

---

## 3. LimitRange 配置问题排查 (LimitRange Configuration Issues)

### 3.1 LimitRange 状态检查

```bash
# ========== 1. LimitRange配置检查 ==========
# 查看LimitRange资源
kubectl get limitrange --all-namespaces

# 查看特定Namespace的LimitRange
kubectl get limitrange -n <namespace-name>

# 检查LimitRange详细配置
kubectl describe limitrange -n <namespace-name>

# 查看LimitRange YAML配置
kubectl get limitrange <limitrange-name> -n <namespace-name> -o yaml

# ========== 2. 限制范围验证 ==========
# 检查容器限制配置
kubectl get limitrange -n <namespace-name> -o jsonpath='{
    .items[0].spec.limits[?(@.type=="Container")].default
}'

# 检查Pod限制配置
kubectl get limitrange -n <namespace-name> -o jsonpath='{
    .items[0].spec.limits[?(@.type=="Pod")].default
}'

# 检查最小/最大限制
kubectl get limitrange -n <namespace-name> -o jsonpath='{
    .items[0].spec.limits[?(@.type=="Container")].min
}{
        "\t"
}{
        .items[0].spec.limits[?(@.type=="Container")].max
}'

# ========== 3. 默认值应用检查 ==========
# 测试默认值应用
cat <<EOF | kubectl apply -f -
apiVersion: v1
kind: Pod
metadata:
  name: test-defaults
  namespace: <namespace-name>
spec:
  containers:
  - name: test-container
    image: nginx
    # 不指定resources，测试默认值应用
EOF

# 验证默认值是否正确应用
kubectl get pod test-defaults -n <namespace-name> -o jsonpath='{
    .spec.containers[0].resources
}'
```

### 3.2 限制冲突问题诊断

```bash
# ========== 1. 限制范围冲突检测 ==========
# 检查最小值大于最大值的情况
kubectl get limitrange -n <namespace-name> -o jsonpath='{
    range .items[0].spec.limits[?(@.type=="Container")]
}{
        "\nMin CPU: "
}{
        .min.cpu
}{
        ", Max CPU: "
}{
        .max.cpu
}{
        "\nMin Memory: "
}{
        .min.memory
}{
        ", Max Memory: "
}{
        .max.memory
}{
        "\n"
}{
    end
}'

# 检查默认值超出范围
kubectl get limitrange -n <namespace-name> -o jsonpath='{
    range .items[0].spec.limits[?(@.type=="Container")]
}{
        "\nDefault CPU: "
}{
        .default.cpu
}{
        ", Min CPU: "
}{
        .min.cpu
}{
        ", Max CPU: "
}{
        .max.cpu
}{
        "\n"
}{
    end
}' | while read line; do
    echo "$line" | awk '{
        if ($4 > $6 && $6 != "") print "ERROR: Default CPU exceeds max limit";
        if ($4 < $8 && $8 != "") print "ERROR: Default CPU below min limit";
    }'
done

# ========== 2. Pod创建失败分析 ==========
# 查看LimitRange相关的创建失败事件
kubectl get events -n <namespace-name> --field-selector reason=FailedCreate | grep -i limit

# 分析具体的限制错误
kubectl get events -n <namespace-name> -o jsonpath='{
    range .items[?(@.reason=="FailedCreate")]
}{
        .message
}{
        "\n"
}{
    end
}' | grep -E "(exceed|minimum|required)"

# ========== 3. 限制策略优化 ==========
# 创建合理的LimitRange配置
cat <<EOF > optimized-limitrange.yaml
apiVersion: v1
kind: LimitRange
metadata:
  name: optimized-limits
  namespace: <namespace-name>
spec:
  limits:
  # Container级别限制
  - type: Container
    default:
      cpu: "500m"        # 默认500毫核
      memory: "512Mi"    # 默认512MB内存
    defaultRequest:
      cpu: "100m"        # 默认请求100毫核
      memory: "256Mi"    # 默认请求256MB内存
    min:
      cpu: "10m"         # 最小10毫核
      memory: "32Mi"     # 最小32MB内存
    max:
      cpu: "2"           # 最大2核
      memory: "4Gi"      # 最大4GB内存
    maxLimitRequestRatio:
      cpu: 5             # 限制/请求比例不超过5倍
      memory: 2          # 内存限制/请求比例不超过2倍
      
  # Pod级别限制
  - type: Pod
    max:
      cpu: "4"           # Pod总CPU不超过4核
      memory: "8Gi"      # Pod总内存不超过8GB
      
  # PVC级别限制
  - type: PersistentVolumeClaim
    min:
      storage: "1Gi"     # 最小1GB存储
    max:
      storage: "100Gi"   # 最大100GB存储
EOF
```

---

## 4. 配额超限问题处理 (Quota Exceeded Issue Handling)

### 4.1 立即解决方案

```bash
# ========== 1. 临时配额调整 ==========
# 增加Pod配额
kubectl patch resourcequota <quota-name> -n <namespace-name> -p '{
    "spec": {
        "hard": {
            "pods": "50"
        }
    }
}'

# 增加CPU配额
kubectl patch resourcequota <quota-name> -n <namespace-name> -p '{
    "spec": {
        "hard": {
            "requests.cpu": "20",
            "limits.cpu": "40"
        }
    }
}'

# 增加内存配额
kubectl patch resourcequota <quota-name> -n <namespace-name> -p '{
    "spec": {
        "hard": {
            "requests.memory": "40Gi",
            "limits.memory": "80Gi"
        }
    }
}'

# ========== 2. 资源清理释放 ==========
# 查找可删除的资源
kubectl get pods -n <namespace-name> --sort-by=.metadata.creationTimestamp

# 删除Completed状态的Job Pods
kubectl delete pods -n <namespace-name> --field-selector=status.phase==Succeeded

# 清理Evicted状态的Pods
kubectl delete pods -n <namespace-name> --field-selector=status.phase==Failed

# 删除未使用的PVC
kubectl get pvc -n <namespace-name> -o jsonpath='{
    range .items[?(@.status.phase=="Bound")]
}{
        .metadata.name
}{
        "\t"
}{
        .metadata.annotations."pv.kubernetes.io/bind-completed"
}{
        "\n"
}{
    end
}' | grep -v true

# ========== 3. 优先级调整 ==========
# 为关键应用设置优先级
kubectl patch deployment <critical-app> -n <namespace-name> -p '{
    "spec": {
        "template": {
            "spec": {
                "priorityClassName": "high-priority"
            }
        }
    }
}'
```

### 4.2 根本原因分析

```bash
# ========== 资源使用趋势分析 ==========
# 收集历史资源使用数据
cat <<'EOF' > resource-usage-analyzer.sh
#!/bin/bash

NAMESPACE=$1
DAYS=${2:-7}

echo "Analyzing resource usage trends for namespace: $NAMESPACE"
echo "Analysis period: Last $DAYS days"

# 创建临时目录存储数据
TEMP_DIR="/tmp/quota-analysis-$(date +%s)"
mkdir -p $TEMP_DIR

# 收集每日资源使用情况
for i in $(seq 0 $DAYS); do
    DATE=$(date -d "$i days ago" +%Y-%m-%d)
    echo "Collecting data for $DATE"
    
    # 获取当日的配额使用情况
    kubectl get resourcequota -n $NAMESPACE -o jsonpath='{
        .items[0].status.used.pods
    }{
        "\t"
    }{
        .items[0].status.used."requests.cpu"
    }{
        "\t"
    }{
        .items[0].status.used."requests.memory"
    }{
        "\t"
    }{
        .items[0].status.used."persistentvolumeclaims"
    }' > $TEMP_DIR/usage-$DATE.txt 2>/dev/null || echo "0	0	0	0" > $TEMP_DIR/usage-$DATE.txt
done

# 生成使用趋势报告
echo "=== Resource Usage Trend Report ===" > $TEMP_DIR/report.txt
echo "Date	Pods	CPU(MilliCores)	Memory(Gi)	PVCs" >> $TEMP_DIR/report.txt

for i in $(seq 0 $DAYS); do
    DATE=$(date -d "$i days ago" +%Y-%m-%d)
    if [ -f $TEMP_DIR/usage-$DATE.txt ]; then
        USAGE=$(cat $TEMP_DIR/usage-$DATE.txt)
        echo "$DATE	$USAGE" >> $TEMP_DIR/report.txt
    fi
done

cat $TEMP_DIR/report.txt

# 清理临时文件
rm -rf $TEMP_DIR
EOF

chmod +x resource-usage-analyzer.sh

# ========== 应用资源需求分析 ==========
# 分析各个应用的资源使用模式
kubectl get deployments -n <namespace-name> -o jsonpath='{
    range .items[*]
}{
        .metadata.name
}{
        "\t"
}{
        .spec.replicas
}{
        "\t"
}{
        .spec.template.spec.containers[0].resources.requests.cpu
}{
        "\t"
}{
        .spec.template.spec.containers[0].resources.requests.memory
}{
        "\n"
}{
    end
}' | sort -k3 -n

# 识别资源密集型应用
kubectl top pods -n <namespace-name> --sort-by=cpu | head -10
kubectl top pods -n <namespace-name> --sort-by=memory | head -10
```

---

## 5. 监控和告警配置 (Monitoring and Alerting)

### 5.1 配额使用监控

```bash
# ========== 监控指标配置 ==========
cat <<EOF | kubectl apply -f -
apiVersion: monitoring.coreos.com/v1
kind: ServiceMonitor
metadata:
  name: quota-monitor
  namespace: monitoring
spec:
  selector:
    matchLabels:
      k8s-app: kube-state-metrics
  endpoints:
  - port: http-metrics
    path: /metrics
    interval: 30s
    metricRelabelings:
    - sourceLabels: [__name__]
      regex: 'kube_resourcequota.*'
      action: keep
EOF

# ========== 告警规则配置 ==========
cat <<EOF | kubectl apply -f -
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: quota-alerts
  namespace: monitoring
spec:
  groups:
  - name: quota.rules
    rules:
    - alert: QuotaUsageHigh
      expr: kube_resourcequota_used_hard_ratio > 0.8
      for: 5m
      labels:
        severity: warning
      annotations:
        summary: "Namespace {{ \$labels.namespace }} quota usage is high ({{ \$value | humanizePercentage }})"
        
    - alert: QuotaAlmostExceeded
      expr: kube_resourcequota_used_hard_ratio > 0.95
      for: 2m
      labels:
        severity: critical
      annotations:
        summary: "Namespace {{ \$labels.namespace }} quota almost exceeded ({{ \$value | humanizePercentage }})"
        
    - alert: QuotaExceeded
      expr: kube_resourcequota_used_hard_ratio >= 1
      for: 1m
      labels:
        severity: critical
      annotations:
        summary: "Namespace {{ \$labels.namespace }} quota exceeded ({{ \$value | humanizePercentage }})"
        
    - alert: MissingResourceQuota
      expr: count(kube_resourcequota) by (namespace) == 0
      for: 30m
      labels:
        severity: warning
      annotations:
        summary: "Namespace {{ \$labels.namespace }} has no resource quota configured"
EOF
```

### 5.2 配额使用仪表板

```bash
# ========== Grafana仪表板配置 ==========
cat <<'EOF' > quota-dashboard.json
{
  "dashboard": {
    "title": "Resource Quota Dashboard",
    "panels": [
      {
        "title": "Quota Usage by Namespace",
        "type": "bargauge",
        "targets": [
          {
            "expr": "kube_resourcequota_used_hard_ratio * 100",
            "legendFormat": "{{namespace}} - {{resource}}"
          }
        ]
      },
      {
        "title": "Quota Usage Trend",
        "type": "graph",
        "targets": [
          {
            "expr": "kube_resourcequota_used_hard_ratio",
            "legendFormat": "{{namespace}}/{{resource}}"
          }
        ]
      },
      {
        "title": "Top Resource Consumers",
        "type": "table",
        "targets": [
          {
            "expr": "topk(10, kube_resourcequota_used)",
            "legendFormat": "{{namespace}}/{{resource}}"
          }
        ]
      },
      {
        "title": "Quota Violations",
        "type": "stat",
        "targets": [
          {
            "expr": "count(kube_resourcequota_used_hard_ratio >= 1)",
            "legendFormat": "Violations"
          }
        ]
      }
    ]
  }
}
EOF

# ========== 配额健康检查脚本 ==========
cat <<'EOF' > quota-health-check.sh
#!/bin/bash

NAMESPACE=${1:-all}
THRESHOLD=${2:-80}  # 告警阈值百分比

if [ "$NAMESPACE" = "all" ]; then
    NAMESPACES=$(kubectl get namespaces -o jsonpath='{.items[*].metadata.name}')
else
    NAMESPACES=$NAMESPACE
fi

echo "=== Resource Quota Health Check ==="
echo "Threshold: ${THRESHOLD}%"
echo ""

WARNING_COUNT=0
CRITICAL_COUNT=0

for ns in $NAMESPACES; do
    echo "Checking namespace: $ns"
    
    # 获取配额使用情况
    USAGE_DATA=$(kubectl get resourcequota -n $ns -o jsonpath='{
        range .items[*]
    }{
            .metadata.name
    }{
            "\t"
    }{
            .status.used.pods
    }{
            "/"
    }{
            .status.hard.pods
    }{
            "\t"
    }{
            .status.used."requests.cpu"
    }{
            "/"
    }{
            .status.hard."requests.cpu"
    }{
            "\t"
    }{
            .status.used."requests.memory"
    }{
            "/"
    }{
            .status.hard."requests.memory"
    }{
            "\n"
    }{
        end
    }' 2>/dev/null)
    
    if [ -z "$USAGE_DATA" ]; then
        echo "  No resource quota configured"
        continue
    fi
    
    echo "$USAGE_DATA" | while IFS=$'\t' read -r quota pods cpu mem; do
        echo "  Quota: $quota"
        
        # 分析Pod使用率
        if [[ $pods =~ ^([0-9]+)/([0-9]+)$ ]]; then
            USED=${BASH_REMATCH[1]}
            TOTAL=${BASH_REMATCH[2]}
            if [ $TOTAL -gt 0 ]; then
                PERCENT=$((USED * 100 / TOTAL))
                if [ $PERCENT -ge $THRESHOLD ]; then
                    if [ $PERCENT -ge 95 ]; then
                        echo "    ⚠️  Pods: ${PERCENT}% (${USED}/${TOTAL}) - CRITICAL"
                        CRITICAL_COUNT=$((CRITICAL_COUNT + 1))
                    else
                        echo "    ⚠️  Pods: ${PERCENT}% (${USED}/${TOTAL}) - WARNING"
                        WARNING_COUNT=$((WARNING_COUNT + 1))
                    fi
                else
                    echo "    ✓ Pods: ${PERCENT}% (${USED}/${TOTAL})"
                fi
            fi
        fi
        
        # 分析CPU使用率
        if [[ $cpu =~ ^([0-9.]+[m]?)/([0-9.]+[m]?)$ ]]; then
            USED_CPU=${BASH_REMATCH[1]}
            TOTAL_CPU=${BASH_REMATCH[2]}
            # 转换为毫核进行比较
            USED_MILLI=$(echo $USED_CPU | sed 's/m$//' | awk '{if($1~/m$/) print $1; else print $1*1000}')
            TOTAL_MILLI=$(echo $TOTAL_CPU | sed 's/m$//' | awk '{if($1~/m$/) print $1; else print $1*1000}')
            if [ $TOTAL_MILLI -gt 0 ]; then
                PERCENT=$((USED_MILLI * 100 / TOTAL_MILLI))
                if [ $PERCENT -ge $THRESHOLD ]; then
                    if [ $PERCENT -ge 95 ]; then
                        echo "    ⚠️  CPU: ${PERCENT}% (${USED_CPU}/${TOTAL_CPU}) - CRITICAL"
                        CRITICAL_COUNT=$((CRITICAL_COUNT + 1))
                    else
                        echo "    ⚠️  CPU: ${PERCENT}% (${USED_CPU}/${TOTAL_CPU}) - WARNING"
                        WARNING_COUNT=$((WARNING_COUNT + 1))
                    fi
                else
                    echo "    ✓ CPU: ${PERCENT}% (${USED_CPU}/${TOTAL_CPU})"
                fi
            fi
        fi
    done
    echo ""
done

echo "=== Summary ==="
echo "Warnings: $WARNING_COUNT"
echo "Critical: $CRITICAL_COUNT"

if [ $CRITICAL_COUNT -gt 0 ]; then
    echo "Overall status: ❌ CRITICAL"
    exit 2
elif [ $WARNING_COUNT -gt 0 ]; then
    echo "Overall status: ⚠️  WARNING"
    exit 1
else
    echo "Overall status: ✓ OK"
    exit 0
fi
EOF

chmod +x quota-health-check.sh
```

---

## 6. 最佳实践和优化建议 (Best Practices and Optimization)

### 6.1 配额配置最佳实践

```bash
# ========== 推荐的配额配置模板 ==========
cat <<EOF > quota-best-practices.yaml
# 开发环境配额
apiVersion: v1
kind: ResourceQuota
metadata:
  name: dev-quota
  namespace: development
spec:
  hard:
    # 计算资源
    requests.cpu: "4"
    requests.memory: 8Gi
    limits.cpu: "8"
    limits.memory: 16Gi
    
    # 对象数量
    pods: "20"
    services: "10"
    replicationcontrollers: "5"
    secrets: "20"
    configmaps: "20"
    
    # 存储资源
    persistentvolumeclaims: "10"
    requests.storage: 50Gi
    
    # 网络资源
    services.loadbalancers: "2"
    services.nodeports: "5"

---
# 生产环境配额
apiVersion: v1
kind: ResourceQuota
metadata:
  name: prod-quota
  namespace: production
spec:
  hard:
    # 计算资源
    requests.cpu: "20"
    requests.memory: 40Gi
    limits.cpu: "40"
    limits.memory: 80Gi
    
    # 对象数量
    pods: "100"
    services: "30"
    replicationcontrollers: "20"
    secrets: "50"
    configmaps: "50"
    
    # 存储资源
    persistentvolumeclaims: "50"
    requests.storage: 500Gi
    
    # 网络资源
    services.loadbalancers: "10"
    services.nodeports: "20"
EOF

# ========== LimitRange最佳实践 ==========
cat <<EOF > limitrange-best-practices.yaml
apiVersion: v1
kind: LimitRange
metadata:
  name: best-practice-limits
  namespace: <namespace-name>
spec:
  limits:
  # 容器级别限制
  - type: Container
    # 合理的默认值
    default:
      cpu: "200m"
      memory: "256Mi"
    defaultRequest:
      cpu: "50m"
      memory: "128Mi"
    
    # 最小限制 (防止资源浪费)
    min:
      cpu: "10m"
      memory: "16Mi"
    
    # 最大限制 (防止资源独占)
    max:
      cpu: "2"
      memory: "4Gi"
    
    # 限制/请求比例控制
    maxLimitRequestRatio:
      cpu: 10    # CPU限制不超过请求的10倍
      memory: 4  # 内存限制不超过请求的4倍
      
  # Pod级别限制
  - type: Pod
    max:
      cpu: "4"     # 单个Pod总CPU不超过4核
      memory: "8Gi" # 单个Pod总内存不超过8GB
      
  # PVC级别限制
  - type: PersistentVolumeClaim
    min:
      storage: "1Gi"    # 最小存储1GB
    max:
      storage: "100Gi"  # 最大存储100GB
EOF
```

### 6.2 自动化管理工具

```bash
# ========== 配额自动调整脚本 ==========
cat <<'EOF' > auto-quota-adjuster.sh
#!/bin/bash

# 自动配额调整工具
# 根据历史使用情况自动调整配额

NAMESPACE=$1
ADJUSTMENT_FACTOR=${2:-1.2}  # 调整因子(120%)

if [ -z "$NAMESPACE" ]; then
    echo "Usage: $0 <namespace> [adjustment-factor]"
    exit 1
fi

echo "Analyzing current resource usage for namespace: $NAMESPACE"

# 获取当前配额
CURRENT_QUOTA=$(kubectl get resourcequota -n $NAMESPACE -o jsonpath='{.items[0].metadata.name}' 2>/dev/null)
if [ -z "$CURRENT_QUOTA" ]; then
    echo "No resource quota found in namespace $NAMESPACE"
    exit 1
fi

# 计算当前使用量
CURRENT_PODS=$(kubectl get pods -n $NAMESPACE --no-headers | wc -l)
CURRENT_CPU=$(kubectl get pods -n $NAMESPACE -o jsonpath='{range .items[*]}{.spec.containers[*].resources.requests.cpu}{"\n"}{end}' | grep -v '^$' | awk '{sum+=$1} END {print sum+0}')
CURRENT_MEM=$(kubectl get pods -n $NAMESPACE -o jsonpath='{range .items[*]}{.spec.containers[*].resources.requests.memory}{"\n"}{end}' | grep -v '^$' | sed 's/Gi//g' | awk '{sum+=$1} END {print sum+0}')

echo "Current usage:"
echo "  Pods: $CURRENT_PODS"
echo "  CPU: ${CURRENT_CPU} cores"
echo "  Memory: ${CURRENT_MEM}Gi"

# 计算建议配额值
RECOMMENDED_PODS=$(echo "$CURRENT_PODS * $ADJUSTMENT_FACTOR" | bc | cut -d. -f1)
RECOMMENDED_CPU=$(echo "$CURRENT_CPU * $ADJUSTMENT_FACTOR" | bc)
RECOMMENDED_MEM=$(echo "$CURRENT_MEM * $ADJUSTMENT_FACTOR" | bc)

echo "Recommended quota values:"
echo "  Pods: $RECOMMENDED_PODS"
echo "  CPU: ${RECOMMENDED_CPU} cores"
echo "  Memory: ${RECOMMENDED_MEM}Gi"

# 确认调整
read -p "Apply these quota adjustments? (y/N): " -n 1 -r
echo
if [[ $REPLY =~ ^[Yy]$ ]]; then
    echo "Adjusting resource quota..."
    
    kubectl patch resourcequota $CURRENT_QUOTA -n $NAMESPACE -p "{
        \"spec\": {
            \"hard\": {
                \"pods\": \"$RECOMMENDED_PODS\",
                \"requests.cpu\": \"${RECOMMENDED_CPU}\",
                \"requests.memory\": \"${RECOMMENDED_MEM}Gi\",
                \"limits.cpu\": \"$(echo "$RECOMMENDED_CPU * 2" | bc)\",
                \"limits.memory\": \"$(echo "$RECOMMENDED_MEM * 2" | bc)Gi\"
            }
        }
    }"
    
    echo "Quota adjustment completed"
else
    echo "Quota adjustment cancelled"
fi
EOF

chmod +x auto-quota-adjuster.sh

# ========== 配额使用报告生成器 ==========
cat <<'EOF' > quota-report-generator.sh
#!/bin/bash

OUTPUT_FORMAT=${1:-html}
OUTPUT_FILE=${2:-quota-report.$OUTPUT_FORMAT}

echo "Generating quota report in $OUTPUT_FORMAT format..."

case $OUTPUT_FORMAT in
    html)
        cat > $OUTPUT_FILE <<HTML
<!DOCTYPE html>
<html>
<head>
    <title>Resource Quota Report</title>
    <style>
        body { font-family: Arial, sans-serif; margin: 20px; }
        table { border-collapse: collapse; width: 100%; }
        th, td { border: 1px solid #ddd; padding: 8px; text-align: left; }
        th { background-color: #f2f2f2; }
        .high { background-color: #ffeb3b; }
        .critical { background-color: #f44336; color: white; }
        .normal { background-color: #4caf50; color: white; }
    </style>
</head>
<body>
    <h1>Resource Quota Report - $(date)</h1>
    
    <table>
        <tr>
            <th>Namespace</th>
            <th>Resource</th>
            <th>Used</th>
            <th>Hard Limit</th>
            <th>Usage %</th>
            <th>Status</th>
        </tr>
HTML

        # 生成数据行
        kubectl get resourcequota --all-namespaces -o jsonpath='{
            range .items[*]
        }{
                .metadata.namespace
        }{
                "\t"
        }{
                .metadata.name
        }{
                "\t"
        }{
                range $key, $value := .status.used
        }{
                    $key
        }{
                    ":"
        }{
                    $value
        }{
                    ","
        }{
                end
        }{
                "\t"
        }{
                range $key, $value := .status.hard
        }{
                    $key
        }{
                    ":"
        }{
                    $value
        }{
                    ","
        }{
                end
        }{
                "\n"
        }{
            end
        }' | while IFS=$'\t' read namespace quota used hard; do
            # 解析used和hard数据
            echo "$used" | tr ',' '\n' | while IFS=':' read resource used_value; do
                if [ -n "$resource" ] && [ -n "$used_value" ]; then
                    # 在这里解析hard限制并计算百分比
                    hard_value=$(echo "$hard" | grep "$resource:" | cut -d: -f2 | tr -d ',')
                    if [ -n "$hard_value" ] && [ "$hard_value" != "0" ]; then
                        percentage=$((used_value * 100 / hard_value))
                        
                        # 确定状态类
                        if [ $percentage -ge 90 ]; then
                            status_class="critical"
                            status_text="CRITICAL"
                        elif [ $percentage -ge 80 ]; then
                            status_class="high"
                            status_text="HIGH"
                        else
                            status_class="normal"
                            status_text="OK"
                        fi
                        
                        cat >> $OUTPUT_FILE <<ROW
        <tr>
            <td>$namespace</td>
            <td>$resource</td>
            <td>$used_value</td>
            <td>$hard_value</td>
            <td>$percentage%</td>
            <td class="$status_class">$status_text</td>
        </tr>
ROW
                    fi
                fi
            done
        done

        cat >> $OUTPUT_FILE <<HTML
    </table>
</body>
</html>
HTML
        ;;
        
    csv)
        echo "Namespace,Resource,Used,Hard_Limit,Usage_Percent,Status" > $OUTPUT_FILE
        
        kubectl get resourcequota --all-namespaces -o jsonpath='{
            range .items[*]
        }{
                .metadata.namespace
        }{
                "\t"
        }{
                range $key, $value := .status.used
        }{
                    $key
        }{
                    ":"
        }{
                    $value
        }{
                    ","
        }{
                end
        }{
                "\t"
        }{
                range $key, $value := .status.hard
        }{
                    $key
        }{
                    ":"
        }{
                    $value
        }{
                    ","
        }{
                end
        }{
                "\n"
        }{
            end
        }' | while IFS=$'\t' read namespace used hard; do
            echo "$used" | tr ',' '\n' | while IFS=':' read resource used_value; do
                if [ -n "$resource" ] && [ -n "$used_value" ]; then
                    hard_value=$(echo "$hard" | grep "$resource:" | cut -d: -f2 | tr -d ',')
                    if [ -n "$hard_value" ] && [ "$hard_value" != "0" ]; then
                        percentage=$((used_value * 100 / hard_value))
                        
                        if [ $percentage -ge 90 ]; then
                            status="CRITICAL"
                        elif [ $percentage -ge 80 ]; then
                            status="HIGH"
                        else
                            status="OK"
                        fi
                        
                        echo "$namespace,$resource,$used_value,$hard_value,$percentage%,$status" >> $OUTPUT_FILE
                    fi
                fi
            done
        done
        ;;
esac

echo "Report generated: $OUTPUT_FILE"
EOF

chmod +x quota-report-generator.sh
```

---