# 23 - Namespace 故障排查 (Namespace Troubleshooting)

> **适用版本**: Kubernetes v1.25-v1.32 | **最后更新**: 2026-02 | **参考**: [Kubernetes Namespaces](https://kubernetes.io/docs/concepts/overview/working-with-objects/namespaces/)

---

## 1. Namespace 故障诊断总览 (Namespace Diagnosis Overview)

### 1.1 常见故障现象分类

| 故障类型 | 症状表现 | 影响范围 | 紧急程度 |
|---------|---------|---------|---------|
| **Namespace创建失败** | 无法创建新命名空间 | 资源隔离失效 | P1 - 高 |
| **资源配额超限** | 超出配额限制 | 应用部署失败 | P0 - 紧急 |
| **权限访问拒绝** | RBAC权限不足 | 操作受限 | P1 - 高 |
| **网络策略冲突** | 策略规则相互干扰 | 网络访问异常 | P1 - 高 |
| **资源清理失败** | 删除时残留资源 | 管理混乱 | P2 - 中 |
| **标签选择器失效** | 标签匹配异常 | 服务发现失败 | P1 - 高 |

### 1.2 Namespace 架构回顾

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                     Namespace 故障诊断架构                                   │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  ┌─────────────────────────────────────────────────────────────────────┐  │
│  │                       Namespace管理层                                │  │
│  │  ┌─────────────┐    ┌─────────────┐    ┌─────────────┐              │  │
│  │  │  default    │    │  production │    │ development │              │  │
│  │  │   (系统)    │    │   (生产)    │    │  (开发)     │              │  │
│  │  └─────────────┘    └─────────────┘    └─────────────┘              │  │
│  └─────────────────────────────────────────────────────────────────────┘  │
│                              │                                              │
│         ┌────────────────────┼────────────────────┐                       │
│         │                    │                    │                       │
│         ▼                    ▼                    ▼                       │
│  ┌─────────────┐    ┌─────────────┐    ┌─────────────┐                   │
│  │   资源配额   │   │   网络策略   │   │   RBAC权限   │                   │
│  │ (Resource   │   │ (Network    │   │ (Role/      │                   │
│  │ Quota)      │   │ Policy)     │   │ Binding)    │                   │
│  └─────────────┘    └─────────────┘    └─────────────┘                   │
│         │                    │                    │                       │
│         ▼                    ▼                    ▼                       │
│  ┌─────────────────────────────────────────────────────────────────────┐  │
│  │                      Namespace控制器                                 │  │
│  │  ┌───────────────────────────────────────────────────────────────┐  │  │
│  │  │                   kube-apiserver                              │  │  │
│  │  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐           │  │  │
│  │  │  │ Namespace   │  │   Quota     │  │   Admission │           │  │  │
│  │  │  │ Controller  │  │ Controller  │  │  Controller │           │  │  │
│  │  │  └─────────────┘  └─────────────┘  └─────────────┘           │  │  │
│  │  └───────────────────────────────────────────────────────────────┘  │  │
│  └─────────────────────────────────────────────────────────────────────┘  │
│                              │                                              │
│         ┌────────────────────┼────────────────────┐                       │
│         │                    │                    │                       │
│         ▼                    ▼                    ▼                       │
│  ┌─────────────┐    ┌─────────────┐    ┌─────────────┐                   │
│  │   对象管理   │   │   生命周期   │   │   监控告警   │                   │
│  │ (Objects)   │   │ (Lifecycle) │   │ (Monitoring)│                   │
│  │   创建/删除  │   │   管理      │   │   告警      │                   │
│  └─────────────┘    └─────────────┘    └─────────────┘                   │
│                              │                                              │
│                              ▼                                              │
│  ┌─────────────────────────────────────────────────────────────────────┐  │
│  │                      下层资源层                                     │  │
│  │  ┌─────────────┐    ┌─────────────┐    ┌─────────────┐              │  │
│  │  │   Pod资源   │    │   Service   │    │   Storage   │              │  │
│  │  │ (计算资源)   │    │  (网络资源)  │    │  (存储资源)  │              │  │
│  │  └─────────────┘    └─────────────┘    └─────────────┘              │  │
│  └─────────────────────────────────────────────────────────────────────┘  │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

---

## 2. Namespace 基础状态检查 (Basic Status Check)

### 2.1 Namespace 资源状态验证

```bash
# ========== 1. 基础信息检查 ==========
# 查看所有Namespace
kubectl get namespaces

# 查看特定Namespace详细信息
kubectl describe namespace <namespace-name>

# 检查Namespace配置
kubectl get namespace <namespace-name> -o yaml

# ========== 2. Namespace状态检查 ==========
# 查看Namespace状态
kubectl get namespace <namespace-name> -o jsonpath='{.status.phase}'

# 检查终止状态的Namespace
kubectl get namespaces --field-selector status.phase=Terminating

# 查看Namespace事件
kubectl get events --field-selector involvedObject.kind=Namespace,involvedObject.name=<namespace-name>

# ========== 3. 标签和注解检查 ==========
# 查看Namespace标签
kubectl get namespace <namespace-name> --show-labels

# 检查安全标签
kubectl get namespace <namespace-name> -o jsonpath='{.metadata.labels.pod-security\.kubernetes\.io/enforce}'

# 查看Namespace注解
kubectl get namespace <namespace-name> -o jsonpath='{.metadata.annotations}'
```

### 2.2 资源配额状态检查

```bash
# ========== ResourceQuota检查 ==========
# 查看Namespace中的资源配额
kubectl get resourcequota -n <namespace-name>

# 查看ResourceQuota详细信息
kubectl describe resourcequota -n <namespace-name>

# 检查配额使用情况
kubectl get resourcequota -n <namespace-name> -o jsonpath='{
    .items[*].status.used
}'

# ========== LimitRange检查 ==========
# 查看LimitRange配置
kubectl get limitrange -n <namespace-name>

# 检查容器资源限制
kubectl describe limitrange -n <namespace-name>

# 验证默认资源请求
kubectl get limitrange -n <namespace-name> -o jsonpath='{.items[*].spec.limits[*].default}'
```

---

## 3. 资源配额超限问题排查 (Resource Quota Exceeded Troubleshooting)

### 3.1 配额使用情况分析

```bash
# ========== 1. 配额消耗检查 ==========
# 查看所有资源配额使用情况
kubectl get resourcequota -A -o jsonpath='{
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
        .status.used.pods
}{
        "/"
}{
        .status.hard.pods
}{
        "\n"
}{
    end
}'

# 检查CPU和内存配额使用
kubectl get resourcequota -n <namespace-name> -o jsonpath='{
    .items[*].status.used."requests.cpu"
}{
        "\t"
}{
        .items[*].status.hard."requests.cpu"
}{
        "\t"
}{
        .items[*].status.used."requests.memory"
}{
        "\t"
}{
        .items[*].status.hard."requests.memory"
}'

# ========== 2. 配额超限诊断 ==========
# 查看配额超限事件
kubectl get events -n <namespace-name> --field-selector reason=FailedCreate

# 检查具体的配额限制
kubectl describe resourcequota <quota-name> -n <namespace-name>

# 分析资源请求与限制
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
        .spec.containers[*].resources.limits.cpu
}{
        "\n"
}{
    end
}'
```

### 3.2 配额调整和优化

```bash
# ========== 配额调整策略 ==========
# 增加Pod配额
kubectl patch resourcequota <quota-name> -n <namespace-name> -p '{
    "spec": {
        "hard": {
            "pods": "50"
        }
    }
}'

# 调整CPU配额
kubectl patch resourcequota <quota-name> -n <namespace-name> -p '{
    "spec": {
        "hard": {
            "requests.cpu": "20",
            "limits.cpu": "40"
        }
    }
}'

# ========== 配额优化建议 ==========
# 创建合理的配额模板
cat <<EOF > namespace-quota-template.yaml
apiVersion: v1
kind: ResourceQuota
metadata:
  name: compute-resources
  namespace: <namespace-name>
spec:
  hard:
    requests.cpu: "10"
    requests.memory: 20Gi
    limits.cpu: "20"
    limits.memory: 40Gi
    requests.storage: 100Gi
    persistentvolumeclaims: "20"
    pods: "30"
    services: "10"
    secrets: "20"
    configmaps: "20"
---
apiVersion: v1
kind: LimitRange
metadata:
  name: container-limits
  namespace: <namespace-name>
spec:
  limits:
  - default:
      cpu: 500m
      memory: 1Gi
    defaultRequest:
      cpu: 100m
      memory: 256Mi
    type: Container
EOF

# ========== 资源使用分析 ==========
# 分析Namespace资源使用趋势
cat <<'EOF' > quota-usage-analyzer.sh
#!/bin/bash

NAMESPACE=$1
DAYS=${2:-7}

echo "Analyzing resource quota usage for namespace: $NAMESPACE"
echo "Analysis period: Last $DAYS days"

# 收集历史配额使用数据
for i in $(seq 0 $DAYS); do
    DATE=$(date -d "$i days ago" +%Y-%m-%d)
    echo "$DATE:"
    
    # 获取当天的配额使用情况
    kubectl get resourcequota -n $NAMESPACE -o jsonpath='{
        .items[*].status.used.pods
    }{
        "\t"
    }{
        .items[*].status.used."requests.cpu"
    }{
        "\t"
    }{
        .items[*].status.used."requests.memory"
    }' 2>/dev/null || echo "No data"
done
EOF

chmod +x quota-usage-analyzer.sh
```

---

## 4. RBAC权限问题排查 (RBAC Permission Issues)

### 4.1 权限配置检查

```bash
# ========== 1. 角色绑定检查 ==========
# 查看Namespace中的RoleBindings
kubectl get rolebindings -n <namespace-name>

# 查看ClusterRoleBindings影响当前Namespace
kubectl get clusterrolebindings -o jsonpath='{
    range .items[*]
}{
        .metadata.name
}{
        "\t"
}{
        .subjects[*].kind
}{
        "/"
}{
        .subjects[*].name
}{
        "\n"
}{
    end
}' | grep -E "(ServiceAccount|User|Group).*<namespace-name>"

# 检查特定用户的权限
kubectl auth can-i get pods --as=<user-name> -n <namespace-name>

# ========== 2. ServiceAccount权限检查 ==========
# 查看Namespace中的ServiceAccounts
kubectl get serviceaccounts -n <namespace-name>

# 检查ServiceAccount绑定的角色
kubectl get rolebindings -n <namespace-name> -o jsonpath='{
    range .items[*]
}{
        .metadata.name
}{
        "\t"
}{
        .roleRef.name
}{
        "\t"
}{
        .subjects[?(@.kind=="ServiceAccount")].name
}{
        "\n"
}{
    end
}'

# 验证ServiceAccount令牌
kubectl get secret -n <namespace-name> -o jsonpath='{
    range .items[?(@.type=="kubernetes.io/service-account-token")]
}{
        .metadata.name
}{
        "\n"
}{
    end
}'
```

### 4.2 权限不足问题解决

```bash
# ========== 权限问题诊断 ==========
# 模拟权限测试
USERS_TO_TEST=("developer" "operator" "admin")
for user in "${USERS_TO_TEST[@]}"; do
    echo "Testing permissions for user: $user"
    
    # 测试基本读取权限
    if kubectl auth can-i get pods --as=$user -n <namespace-name>; then
        echo "  ✓ Can get pods"
    else
        echo "  ✗ Cannot get pods"
    fi
    
    # 测试创建权限
    if kubectl auth can-i create deployments --as=$user -n <namespace-name>; then
        echo "  ✓ Can create deployments"
    else
        echo "  ✗ Cannot create deployments"
    fi
done

# ========== 权限修复配置 ==========
# 创建开发人员角色
cat <<EOF | kubectl apply -f -
apiVersion: rbac.authorization.k8s.io/v1
kind: Role
metadata:
  name: developer-role
  namespace: <namespace-name>
rules:
- apiGroups: [""]
  resources: ["pods", "services", "configmaps", "secrets"]
  verbs: ["get", "list", "watch", "create", "update", "patch", "delete"]
- apiGroups: ["apps"]
  resources: ["deployments", "statefulsets", "daemonsets"]
  verbs: ["get", "list", "watch", "create", "update", "patch", "delete"]
- apiGroups: ["batch"]
  resources: ["jobs", "cronjobs"]
  verbs: ["get", "list", "watch", "create", "update", "patch", "delete"]
---
apiVersion: rbac.authorization.k8s.io/v1
kind: RoleBinding
metadata:
  name: developer-binding
  namespace: <namespace-name>
subjects:
- kind: User
  name: developer@example.com
  apiGroup: rbac.authorization.k8s.io
roleRef:
  kind: Role
  name: developer-role
  apiGroup: rbac.authorization.k8s.io
EOF

# ========== 权限审计脚本 ==========
cat <<'EOF' > permission-audit.sh
#!/bin/bash

NAMESPACE=$1

echo "Performing permission audit for namespace: $NAMESPACE"

# 收集所有ServiceAccounts
SERVICE_ACCOUNTS=$(kubectl get serviceaccounts -n $NAMESPACE -o jsonpath='{.items[*].metadata.name}')

echo "=== ServiceAccount Permissions ==="
for sa in $SERVICE_ACCOUNTS; do
    echo "ServiceAccount: $sa"
    
    # 检查绑定的角色
    ROLES=$(kubectl get rolebindings -n $NAMESPACE -o jsonpath='{
        range .items[?(@.subjects[0].name=="'$sa'")]
    }{
            .roleRef.name
    }{
            " "
    }{
        end
    }')
    
    if [ -n "$ROLES" ]; then
        echo "  Bound roles: $ROLES"
    else
        echo "  No role bindings found"
    fi
done

# 检查未使用的ServiceAccounts
echo "=== Unused ServiceAccounts ==="
kubectl get pods -n $NAMESPACE -o jsonpath='{
    range .items[*]
}{
        .spec.serviceAccountName
}{
        "\n"
}{
    end
}' | sort | uniq > /tmp/used-sas.txt

for sa in $SERVICE_ACCOUNTS; do
    if ! grep -q "^$sa$" /tmp/used-sas.txt; then
        echo "Unused ServiceAccount: $sa"
    fi
done

rm /tmp/used-sas.txt
EOF

chmod +x permission-audit.sh
```

---

## 5. 网络策略冲突排查 (Network Policy Conflict Troubleshooting)

### 5.1 网络策略状态检查

```bash
# ========== 1. 策略配置检查 ==========
# 查看Namespace中的NetworkPolicies
kubectl get networkpolicies -n <namespace-name>

# 检查策略详细配置
kubectl describe networkpolicies -n <namespace-name>

# 分析策略选择器
kubectl get networkpolicy -n <namespace-name> -o jsonpath='{
    range .items[*]
}{
        .metadata.name
}{
        "\t"
}{
        .spec.podSelector.matchLabels
}{
        "\n"
}{
    end
}'

# ========== 2. 策略冲突检测 ==========
# 检查重叠的选择器
kubectl get networkpolicy -n <namespace-name> -o jsonpath='{
    range .items[*]
}{
        .metadata.name
}{
        "\t"
}{
        .spec.podSelector
}{
        "\n"
}{
    end
}' | sort -k2 | uniq -d

# 分析策略规则冲突
kubectl get networkpolicy -n <namespace-name> -o jsonpath='{
    range .items[*]
}{
        .metadata.name
}{
        "\t"
}{
        .spec.policyTypes
}{
        "\n"
}{
    end
}'
```

### 5.2 网络连通性测试

```bash
# ========== 连通性验证工具 ==========
# 创建网络测试Pod
cat <<EOF | kubectl apply -f -
apiVersion: v1
kind: Pod
metadata:
  name: network-test-pod
  namespace: <namespace-name>
  labels:
    app: network-test
spec:
  containers:
  - name: tester
    image: nicolaka/netshoot
    command: ["sleep", "3600"]
EOF

# 测试Pod间通信
kubectl exec -n <namespace-name> network-test-pod -- sh -c "
echo 'Testing connectivity to other pods...'
for pod in \$(kubectl get pods -n <namespace-name> -o jsonpath='{.items[*].status.podIP}'); do
    echo \"Testing \$pod\"
    nc -zv \$pod 80 2>&1 | grep -E '(succeeded|open)'
done
"

# ========== 策略效果验证 ==========
# 临时禁用网络策略测试
kubectl delete networkpolicy --all -n <namespace-name>

# 验证网络恢复
kubectl exec -n <namespace-name> network-test-pod -- ping -c 3 <target-pod-ip>

# 重新应用策略
kubectl apply -f <original-policy-file.yaml>
```

---

## 6. Namespace清理和管理 (Namespace Cleanup and Management)

### 6.1 终止状态Namespace处理

```bash
# ========== 1. 终止状态诊断 ==========
# 查看卡住的Namespace
kubectl get namespaces --field-selector status.phase=Terminating

# 检查终止原因
kubectl describe namespace <terminating-namespace>

# 查看相关资源
kubectl api-resources --namespaced=true --verbs=delete | cut -f1 -d" " | xargs -n1 kubectl get -n <terminating-namespace>

# ========== 2. 强制清理方法 ==========
# 方法1: 删除finalizers
kubectl get namespace <terminating-namespace> -o jsonpath='{.metadata.finalizers}' | jq

# 移除finalizers
kubectl patch namespace <terminating-namespace> -p '{"metadata":{"finalizers":[]}}' --type=merge

# 方法2: API直接删除
kubectl proxy &
PID=$!
sleep 2
curl -k -H "Content-Type: application/json" -X PUT --data-binary @- http://127.0.0.1:8001/api/v1/namespaces/<terminating-namespace>/finalize <<EOF
{
  "kind": "Namespace",
  "apiVersion": "v1",
  "metadata": {
    "name": "<terminating-namespace>"
  }
}
EOF
kill $PID

# ========== 3. 预防措施 ==========
# 设置合理的终结器
cat <<EOF > namespace-finalizer-config.yaml
apiVersion: v1
kind: Namespace
metadata:
  name: <namespace-name>
  finalizers:
  - kubernetes
spec:
  # 配置适当的资源限制防止清理问题
EOF
```

### 6.2 Namespace生命周期管理

```bash
# ========== 自动化清理脚本 ==========
cat <<'EOF' > namespace-lifecycle-manager.sh
#!/bin/bash

ACTION=$1
NAMESPACE=$2

case $ACTION in
    create)
        echo "Creating namespace: $NAMESPACE"
        kubectl create namespace $NAMESPACE
        
        # 应用默认配置
        kubectl annotate namespace $NAMESPACE created-by=automation
        kubectl label namespace $NAMESPACE environment=development
        
        # 应用默认配额
        cat <<QUOTA | kubectl apply -f -
apiVersion: v1
kind: ResourceQuota
metadata:
  name: default-quota
  namespace: $NAMESPACE
spec:
  hard:
    pods: "10"
    requests.cpu: "2"
    requests.memory: 4Gi
    limits.cpu: "4"
    limits.memory: 8Gi
QUOTA
        
        echo "Namespace $NAMESPACE created successfully"
        ;;
        
    delete)
        echo "Preparing to delete namespace: $NAMESPACE"
        
        # 确认操作
        read -p "Are you sure you want to delete namespace $NAMESPACE? (yes/no): " confirm
        if [ "$confirm" != "yes" ]; then
            echo "Operation cancelled"
            exit 1
        fi
        
        # 备份重要资源
        echo "Backing up resources..."
        kubectl get all -n $NAMESPACE -o yaml > backup-${NAMESPACE}-$(date +%Y%m%d-%H%M%S).yaml
        
        # 删除namespace
        kubectl delete namespace $NAMESPACE
        echo "Namespace $NAMESPACE deletion initiated"
        ;;
        
    audit)
        echo "Auditing namespace: $NAMESPACE"
        
        # 检查资源使用
        echo "Resource usage:"
        kubectl describe resourcequota -n $NAMESPACE
        
        # 检查权限配置
        echo "Permission analysis:"
        kubectl get rolebindings,clusterrolebindings -n $NAMESPACE
        
        # 检查网络策略
        echo "Network policies:"
        kubectl get networkpolicies -n $NAMESPACE
        
        # 检查未使用资源
        echo "Unused resources:"
        kubectl get pods,svc,deployments -n $NAMESPACE --no-headers | wc -l
        ;;
        
    *)
        echo "Usage: $0 {create|delete|audit} <namespace-name>"
        exit 1
        ;;
esac
EOF

chmod +x namespace-lifecycle-manager.sh

# ========== Namespace健康检查 ==========
cat <<'EOF' > namespace-health-check.sh
#!/bin/bash

NAMESPACE=$1

if [ -z "$NAMESPACE" ]; then
    echo "Usage: $0 <namespace-name>"
    exit 1
fi

echo "Performing health check for namespace: $NAMESPACE"

# 检查基本状态
STATUS=$(kubectl get namespace $NAMESPACE -o jsonpath='{.status.phase}')
echo "Namespace status: $STATUS"

if [ "$STATUS" != "Active" ]; then
    echo "ERROR: Namespace is not active"
    exit 1
fi

# 检查配额使用
echo "Resource quota usage:"
kubectl describe resourcequota -n $NAMESPACE | grep -E "(Used|Hard)"

# 检查Pod状态
RUNNING_PODS=$(kubectl get pods -n $NAMESPACE --no-headers | grep Running | wc -l)
TOTAL_PODS=$(kubectl get pods -n $NAMESPACE --no-headers | wc -l)
echo "Pods: $RUNNING_PODS/$TOTAL_PODS running"

# 检查服务状态
SERVICES=$(kubectl get svc -n $NAMESPACE --no-headers | wc -l)
echo "Services: $SERVICES"

# 检查网络策略
POLICIES=$(kubectl get networkpolicies -n $NAMESPACE --no-headers | wc -l)
echo "Network policies: $POLICIES"

# 健康评估
if [ $RUNNING_PODS -eq $TOTAL_PODS ] && [ $TOTAL_PODS -gt 0 ]; then
    echo "✓ Namespace is healthy"
    exit 0
else
    echo "✗ Namespace has issues"
    exit 1
fi
EOF

chmod +x namespace-health-check.sh
```

---

## 7. 监控和告警配置 (Monitoring and Alerting)

### 7.1 Namespace监控指标

```bash
# ========== 监控配置 ==========
cat <<EOF | kubectl apply -f -
apiVersion: monitoring.coreos.com/v1
kind: ServiceMonitor
metadata:
  name: namespace-monitor
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
      regex: 'kube_namespace.*'
      action: keep
EOF

# ========== 告警规则配置 ==========
cat <<EOF | kubectl apply -f -
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: namespace-alerts
  namespace: monitoring
spec:
  groups:
  - name: namespace.rules
    rules:
    - alert: NamespaceTerminating
      expr: kube_namespace_status_phase{phase="Terminating"} == 1
      for: 5m
      labels:
        severity: warning
      annotations:
        summary: "Namespace {{ \$labels.namespace }} is terminating"
        
    - alert: NamespaceQuotaExceeded
      expr: kube_resourcequota_used_hard_ratio > 0.9
      for: 2m
      labels:
        severity: warning
      annotations:
        summary: "Namespace {{ \$labels.namespace }} quota usage exceeds 90%"
        
    - alert: NamespaceHasNoQuota
      expr: count(kube_resourcequota) by (namespace) == 0
      for: 10m
      labels:
        severity: warning
      annotations:
        summary: "Namespace {{ \$labels.namespace }} has no resource quota configured"
        
    - alert: OrphanedNamespace
      expr: kube_namespace_created > 2592000  # 30天
      for: 1h
      labels:
        severity: info
      annotations:
        summary: "Namespace {{ \$labels.namespace }} has been unused for 30 days"
EOF
```

### 7.2 Namespace管理仪表板

```bash
# ========== Grafana仪表板配置 ==========
cat <<'EOF' > namespace-dashboard.json
{
  "dashboard": {
    "title": "Namespace Overview",
    "panels": [
      {
        "title": "Namespace Status",
        "type": "stat",
        "targets": [
          {
            "expr": "count(kube_namespace_status_phase{phase=\"Active\"})",
            "legendFormat": "Active"
          },
          {
            "expr": "count(kube_namespace_status_phase{phase=\"Terminating\"})",
            "legendFormat": "Terminating"
          }
        ]
      },
      {
        "title": "Resource Quota Usage",
        "type": "graph",
        "targets": [
          {
            "expr": "kube_resourcequota_used_hard_ratio",
            "legendFormat": "{{namespace}} - {{resource}}"
          }
        ]
      },
      {
        "title": "Namespace Age Distribution",
        "type": "histogram",
        "targets": [
          {
            "expr": "time() - kube_namespace_created",
            "legendFormat": "{{namespace}}"
          }
        ]
      }
    ]
  }
}
EOF

# ========== Namespace报告生成 ==========
cat <<'EOF' > namespace-report-generator.sh
#!/bin/bash

REPORT_DIR=${1:-/tmp/namespace-reports}
mkdir -p $REPORT_DIR

TIMESTAMP=$(date +%Y%m%d-%H%M%S)
REPORT_FILE="$REPORT_DIR/namespace-report-$TIMESTAMP.html"

cat > $REPORT_FILE <<HTML
<!DOCTYPE html>
<html>
<head>
    <title>Kubernetes Namespace Report</title>
    <style>
        body { font-family: Arial, sans-serif; margin: 20px; }
        table { border-collapse: collapse; width: 100%; }
        th, td { border: 1px solid #ddd; padding: 8px; text-align: left; }
        th { background-color: #f2f2f2; }
        .critical { background-color: #ffcccc; }
        .warning { background-color: #ffffcc; }
        .ok { background-color: #ccffcc; }
    </style>
</head>
<body>
    <h1>Kubernetes Namespace Report - $(date)</h1>
    
    <h2>Namespace Summary</h2>
    <table>
        <tr>
            <th>Namespace</th>
            <th>Status</th>
            <th>Age</th>
            <th>Pods</th>
            <th>Services</th>
            <th>Quota Usage</th>
        </tr>
HTML

# 生成每个namespace的数据
kubectl get namespaces -o jsonpath='{
    range .items[*]
}{
        .metadata.name
}{
        "\t"
}{
        .status.phase
}{
        "\t"
}{
        .metadata.creationTimestamp
}{
        "\n"
}{
    end
}' | while read name status created; do
    AGE_DAYS=$(( ($(date +%s) - $(date -d "$created" +%s)) / 86400 ))
    POD_COUNT=$(kubectl get pods -n $name --no-headers 2>/dev/null | wc -l)
    SVC_COUNT=$(kubectl get svc -n $name --no-headers 2>/dev/null | wc -l)
    
    # 获取配额使用率
    QUOTA_USAGE=$(kubectl get resourcequota -n $name -o jsonpath='{.items[*].status.used.pods}' 2>/dev/null || echo "N/A")
    
    # 确定状态类
    if [ "$status" != "Active" ]; then
        CLASS="critical"
    elif [ $AGE_DAYS -gt 365 ]; then
        CLASS="warning"
    else
        CLASS="ok"
    fi
    
    cat >> $REPORT_FILE <<ROW
        <tr class="$CLASS">
            <td>$name</td>
            <td>$status</td>
            <td>$AGE_DAYS days</td>
            <td>$POD_COUNT</td>
            <td>$SVC_COUNT</td>
            <td>$QUOTA_USAGE</td>
        </tr>
ROW
done

cat >> $REPORT_FILE <<HTML
    </table>
    
    <h2>Recommendations</h2>
    <ul>
        <li>Review namespaces in Terminating state</li>
        <li>Clean up namespaces older than 1 year with no resources</li>
        <li>Configure resource quotas for namespaces without quotas</li>
        <li>Review and optimize network policies</li>
    </ul>
</body>
</html>
HTML

echo "Report generated: $REPORT_FILE"
EOF

chmod +x namespace-report-generator.sh
```

---