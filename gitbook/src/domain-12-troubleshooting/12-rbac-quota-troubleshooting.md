# 12 - RBAC与ResourceQuota 故障排查 (RBAC & Quota Troubleshooting)

> **适用版本**: Kubernetes v1.25-v1.32 | **最后更新**: 2026-01

---

## 1. RBAC 权限问题排查 (RBAC Troubleshooting)

### 1.1 权限错误识别

```
# 常见RBAC错误信息
Error from server (Forbidden): pods is forbidden: User "user@example.com" cannot list resource "pods" in API group "" in the namespace "default"

Error from server (Forbidden): deployments.apps is forbidden: User "system:serviceaccount:default:my-sa" cannot create resource "deployments" in API group "apps" in the namespace "production"
```

### 1.2 RBAC 排查流程

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                         RBAC 权限排查流程                                    │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│   ┌─────────────────┐                                                       │
│   │ Forbidden 错误   │                                                       │
│   └────────┬────────┘                                                       │
│            │                                                                │
│            ▼                                                                │
│   ┌──────────────────────────────────────────────────────┐                 │
│   │ Step 1: 确认主体(User/Group/ServiceAccount)          │                 │
│   │ 从错误信息中提取主体名称                              │                 │
│   └──────────────────────────────────────────────────────┘                 │
│            │                                                                │
│            ▼                                                                │
│   ┌──────────────────────────────────────────────────────┐                 │
│   │ Step 2: 检查现有权限                                  │                 │
│   │ kubectl auth can-i --list --as=<user>               │                 │
│   └──────────────────────────────────────────────────────┘                 │
│            │                                                                │
│            ▼                                                                │
│   ┌──────────────────────────────────────────────────────┐                 │
│   │ Step 3: 查找相关RoleBinding/ClusterRoleBinding       │                 │
│   │ kubectl get rolebinding,clusterrolebinding          │                 │
│   └──────────────────────────────────────────────────────┘                 │
│            │                                                                │
│            ├─── 无绑定 ──▶ 创建RoleBinding/ClusterRoleBinding              │
│            │                                                                │
│            ├─── Role权限不足 ──▶ 更新Role/ClusterRole                      │
│            │                                                                │
│            └─── 绑定错误 ──▶ 检查subjects配置                               │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

### 1.3 常用排查命令

```bash
# === 检查特定用户的权限 ===
kubectl auth can-i list pods --as=user@example.com
kubectl auth can-i create deployments --as=user@example.com -n production
kubectl auth can-i '*' '*' --as=admin@example.com  # 检查是否有所有权限

# === 列出用户所有权限 ===
kubectl auth can-i --list --as=user@example.com
kubectl auth can-i --list --as=user@example.com -n production

# === 检查ServiceAccount权限 ===
kubectl auth can-i list pods --as=system:serviceaccount:default:my-sa
kubectl auth can-i --list --as=system:serviceaccount:default:my-sa -n default

# === 查找用户的RoleBinding ===
kubectl get rolebinding -A -o wide | grep "user@example.com"
kubectl get clusterrolebinding -o wide | grep "user@example.com"

# === 查找ServiceAccount的绑定 ===
kubectl get rolebinding -n <namespace> -o yaml | grep -A5 "serviceAccount"
kubectl get clusterrolebinding -o yaml | grep -A5 "serviceAccount"

# === 查看Role/ClusterRole详情 ===
kubectl describe role <role-name> -n <namespace>
kubectl describe clusterrole <clusterrole-name>
```

### 1.4 RBAC 问题分类

| 问题类型 | 错误特征 | 解决方案 |
|:---|:---|:---|
| **无任何绑定** | 用户没有任何权限 | 创建RoleBinding |
| **命名空间错误** | 权限在其他命名空间 | 检查binding的namespace |
| **资源类型错误** | pods vs deployments | 检查Role的resources |
| **API组错误** | apps组 vs 核心组 | 检查apiGroups |
| **动词错误** | get vs list | 检查verbs |
| **主体名称错误** | User vs ServiceAccount | 检查subjects |

### 1.5 创建权限绑定

```bash
# === 为用户绑定角色 ===
kubectl create rolebinding user-admin --clusterrole=admin --user=user@example.com -n production

# === 为ServiceAccount绑定角色 ===
kubectl create rolebinding sa-view --clusterrole=view --serviceaccount=default:my-sa -n production

# === 为组绑定集群角色 ===
kubectl create clusterrolebinding dev-cluster-admin --clusterrole=cluster-admin --group=developers

# === YAML方式创建 ===
cat <<EOF | kubectl apply -f -
apiVersion: rbac.authorization.k8s.io/v1
kind: RoleBinding
metadata:
  name: read-pods
  namespace: production
subjects:
- kind: User
  name: user@example.com
  apiGroup: rbac.authorization.k8s.io
roleRef:
  kind: ClusterRole
  name: view
  apiGroup: rbac.authorization.k8s.io
EOF
```

---

## 2. ServiceAccount 问题排查 (ServiceAccount Issues)

### 2.1 Pod无法访问API

```bash
# === 检查Pod使用的ServiceAccount ===
kubectl get pod <pod-name> -n <namespace> -o jsonpath='{.spec.serviceAccountName}'

# === 检查ServiceAccount是否存在 ===
kubectl get sa <sa-name> -n <namespace>

# === 检查SA的Token Secret (K8s 1.24之前) ===
kubectl get secret -n <namespace> | grep <sa-name>

# === 检查SA的权限 ===
kubectl auth can-i --list --as=system:serviceaccount:<namespace>:<sa-name>

# === 从Pod内部测试API访问 ===
kubectl exec -it <pod-name> -n <namespace> -- \
  curl -k -H "Authorization: Bearer $(cat /var/run/secrets/kubernetes.io/serviceaccount/token)" \
  https://kubernetes.default.svc/api/v1/namespaces/default/pods
```

### 2.2 Token 问题

```bash
# === K8s 1.24+ Token投射 ===
kubectl get pod <pod-name> -n <namespace> -o yaml | grep -A20 "serviceAccountToken"

# === 检查Token是否挂载 ===
kubectl exec -it <pod-name> -n <namespace> -- ls -la /var/run/secrets/kubernetes.io/serviceaccount/

# === 检查Token内容 ===
kubectl exec -it <pod-name> -n <namespace> -- cat /var/run/secrets/kubernetes.io/serviceaccount/token

# === 手动创建Token (K8s 1.24+) ===
kubectl create token <sa-name> -n <namespace> --duration=3600s
```

---

## 3. ResourceQuota 问题排查 (ResourceQuota Troubleshooting)

### 3.1 配额错误识别

```
# 常见ResourceQuota错误
Error from server (Forbidden): pods "my-pod" is forbidden: exceeded quota: my-quota, requested: cpu=500m, used: cpu=900m, limited: cpu=1

Error from server (Forbidden): persistentvolumeclaims "my-pvc" is forbidden: exceeded quota: storage-quota, requested: requests.storage=10Gi, used: requests.storage=95Gi, limited: requests.storage=100Gi
```

### 3.2 配额排查命令

```bash
# === 查看命名空间配额 ===
kubectl get resourcequota -n <namespace>
kubectl describe resourcequota -n <namespace>

# === 查看配额使用详情 ===
kubectl describe quota <quota-name> -n <namespace>
# 输出示例:
# Name:       my-quota
# Namespace:  production
# Resource    Used   Hard
# --------    ----   ----
# cpu         900m   1
# memory      2Gi    4Gi
# pods        5      10

# === 查看当前资源使用 ===
kubectl get pods -n <namespace> -o custom-columns=NAME:.metadata.name,CPU:.spec.containers[*].resources.requests.cpu,MEM:.spec.containers[*].resources.requests.memory

# === 计算剩余配额 ===
kubectl describe quota -n <namespace> | grep -E "^(cpu|memory|pods)"
```

### 3.3 配额类型说明

| 配额类型 | 资源名称 | 说明 |
|:---|:---|:---|
| **计算资源** | requests.cpu, limits.cpu | CPU配额 |
| **计算资源** | requests.memory, limits.memory | 内存配额 |
| **对象数量** | pods, services, secrets, configmaps | 对象数限制 |
| **存储资源** | requests.storage, persistentvolumeclaims | 存储配额 |
| **扩展资源** | requests.nvidia.com/gpu | GPU等扩展资源 |

### 3.4 解决配额问题

```bash
# === 方案1: 调整配额 ===
kubectl patch resourcequota <quota-name> -n <namespace> --patch '{"spec":{"hard":{"cpu":"2","memory":"8Gi"}}}'

# === 方案2: 清理未使用资源 ===
kubectl get pods -n <namespace> --field-selector=status.phase=Succeeded -o name | xargs kubectl delete -n <namespace>
kubectl get pods -n <namespace> --field-selector=status.phase=Failed -o name | xargs kubectl delete -n <namespace>

# === 方案3: 减少资源请求 ===
# 编辑Deployment/Pod的resources.requests

# === 方案4: 申请更多配额 ===
# 联系管理员调整ResourceQuota
```

---

## 4. LimitRange 问题排查 (LimitRange Troubleshooting)

### 4.1 LimitRange 错误

```
# 常见LimitRange错误
Error from server (Forbidden): pods "my-pod" is forbidden: minimum cpu usage per Container is 100m, but request is 50m

Error from server (Forbidden): pods "my-pod" is forbidden: maximum memory usage per Container is 1Gi, but limit is 2Gi
```

### 4.2 排查命令

```bash
# === 查看LimitRange ===
kubectl get limitrange -n <namespace>
kubectl describe limitrange -n <namespace>

# === 示例输出 ===
# Type        Resource  Min    Max    Default Request  Default Limit
# ----        --------  ---    ---    ---------------  -------------
# Container   cpu       100m   2000m  200m             500m
# Container   memory    128Mi  2Gi    256Mi            512Mi
# Pod         cpu       -      4000m  -                -
# Pod         memory    -      8Gi    -                -
```

### 4.3 LimitRange 问题解决

| 错误类型 | 原因 | 解决方案 |
|:---|:---|:---|
| **低于最小值** | requests < min | 增加requests |
| **超过最大值** | limits > max | 降低limits |
| **超过默认值** | 未设置limits时超过default | 显式设置limits |
| **Pod级别超限** | 所有容器总和超过Pod限制 | 调整容器配置 |

---

## 5. 审计与调试 (Audit & Debug)

### 5.1 查看API Server审计日志

```bash
# === 检查审计日志 (如果配置) ===
cat /var/log/kubernetes/audit/audit.log | grep "403"

# === 过滤特定用户的请求 ===
cat /var/log/kubernetes/audit/audit.log | jq 'select(.user.username=="user@example.com")'

# === 查看被拒绝的请求 ===
cat /var/log/kubernetes/audit/audit.log | jq 'select(.responseStatus.code==403)'
```

### 5.2 使用kubectl who-can

```bash
# === 需要安装kubectl-who-can插件 ===
# 查看谁可以执行特定操作
kubectl who-can create pods -n production
kubectl who-can delete deployments -n production
kubectl who-can '*' '*' --all-namespaces  # 查看集群管理员
```

### 5.3 RBAC 调试技巧

```bash
# === 模拟用户操作 ===
kubectl get pods --as=user@example.com
kubectl create deployment test --image=nginx --as=user@example.com --dry-run=server

# === 模拟ServiceAccount操作 ===
kubectl get pods --as=system:serviceaccount:default:my-sa

# === 模拟组操作 ===
kubectl get pods --as=user@example.com --as-group=developers

# === 详细输出权限检查 ===
kubectl auth can-i create pods --as=user@example.com -v=8
```

---

## 6. 常见RBAC配置模板 (Common RBAC Templates)

### 6.1 只读权限

```yaml
apiVersion: rbac.authorization.k8s.io/v1
kind: Role
metadata:
  name: pod-reader
  namespace: production
rules:
- apiGroups: [""]
  resources: ["pods", "pods/log"]
  verbs: ["get", "list", "watch"]
- apiGroups: [""]
  resources: ["services", "endpoints"]
  verbs: ["get", "list", "watch"]
---
apiVersion: rbac.authorization.k8s.io/v1
kind: RoleBinding
metadata:
  name: read-pods
  namespace: production
subjects:
- kind: User
  name: viewer@example.com
  apiGroup: rbac.authorization.k8s.io
roleRef:
  kind: Role
  name: pod-reader
  apiGroup: rbac.authorization.k8s.io
```

### 6.2 开发者权限

```yaml
apiVersion: rbac.authorization.k8s.io/v1
kind: Role
metadata:
  name: developer
  namespace: development
rules:
- apiGroups: ["", "apps", "batch"]
  resources: ["*"]
  verbs: ["*"]
- apiGroups: ["networking.k8s.io"]
  resources: ["ingresses"]
  verbs: ["get", "list", "watch", "create", "update", "patch"]
```

### 6.3 CI/CD ServiceAccount

```yaml
apiVersion: v1
kind: ServiceAccount
metadata:
  name: cicd-deployer
  namespace: production
---
apiVersion: rbac.authorization.k8s.io/v1
kind: Role
metadata:
  name: deployer
  namespace: production
rules:
- apiGroups: ["apps"]
  resources: ["deployments"]
  verbs: ["get", "list", "watch", "create", "update", "patch"]
- apiGroups: [""]
  resources: ["configmaps", "secrets"]
  verbs: ["get", "list", "watch", "create", "update", "patch"]
---
apiVersion: rbac.authorization.k8s.io/v1
kind: RoleBinding
metadata:
  name: cicd-deployer
  namespace: production
subjects:
- kind: ServiceAccount
  name: cicd-deployer
  namespace: production
roleRef:
  kind: Role
  name: deployer
  apiGroup: rbac.authorization.k8s.io
```

---

## 7. 诊断命令速查 (Quick Reference)

```bash
# === RBAC检查 ===
kubectl auth can-i <verb> <resource> --as=<user>
kubectl auth can-i --list --as=<user>
kubectl get rolebinding,clusterrolebinding -A | grep <user-or-sa>

# === 查看Role/ClusterRole ===
kubectl get role,clusterrole
kubectl describe role <name> -n <namespace>
kubectl describe clusterrole <name>

# === 查看绑定 ===
kubectl get rolebinding -n <namespace>
kubectl get clusterrolebinding
kubectl describe rolebinding <name> -n <namespace>

# === ResourceQuota检查 ===
kubectl get resourcequota -n <namespace>
kubectl describe resourcequota <name> -n <namespace>

# === LimitRange检查 ===
kubectl get limitrange -n <namespace>
kubectl describe limitrange <name> -n <namespace>

# === ServiceAccount检查 ===
kubectl get sa -n <namespace>
kubectl get secret -n <namespace> | grep <sa-name>
```

---

## 深度解决方案与最佳实践体系

### 4.1 RBAC权限治理最佳实践

#### 4.1.1 权限最小化原则实施

**精细化权限控制策略：**

```yaml
# 生产环境RBAC最佳实践模板
apiVersion: rbac.authorization.k8s.io/v1
kind: Role
metadata:
  namespace: production
  name: app-operator-role
rules:
# 应用管理权限
- apiGroups: [""]
  resources: ["pods", "services", "configmaps"]
  verbs: ["get", "list", "watch", "create", "update", "patch", "delete"]
# 状态查看权限
- apiGroups: [""]
  resources: ["pods/log", "events"]
  verbs: ["get", "list", "watch"]
# 限制危险操作
- apiGroups: [""]
  resources: ["secrets"]
  verbs: ["get", "list"]  # 只读，禁止修改
```

**权限审计与清理脚本：**
```bash
#!/bin/bash
# rbac_audit.sh

echo "=== RBAC权限审计报告 ==="

# 过度授权检测
echo -e "\n--- 过度授权检查 ---"
kubectl get clusterroles -o jsonpath='{range .items[*]}{.metadata.name}{"\n"}{range .rules[*]}{.verbs}{" "}{.resources}{"\n"}{end}{end}' | \
  grep -E "(star|\*)"

# 未使用RoleBinding检测
echo -e "\n--- 未使用的RoleBinding ---"
kubectl get rolebindings --all-namespaces -o jsonpath='{range .items[*]}{.metadata.namespace}/{.metadata.name}{"\n"}{end}' | \
  while read binding; do
    ns=$(echo $binding | cut -d'/' -f1)
    name=$(echo $binding | cut -d'/' -f2)
    if ! kubectl get pods -n $ns --no-headers 2>/dev/null | grep -q "."; then
      echo "未使用: $binding"
    fi
  done
```

#### 4.1.2 动态权限管理系统

**基于标签的权限自动分配：**
```yaml
# 动态RoleBinding模板
apiVersion: rbac.authorization.k8s.io/v1
kind: RoleBinding
metadata:
  name: team-access-binding
  namespace: {{namespace}}
subjects:
- kind: Group
  name: {{team-group}}
  apiGroup: rbac.authorization.k8s.io
roleRef:
  kind: Role
  name: namespace-reader
  apiGroup: rbac.authorization.k8s.io
```

**自动化权限管理脚本：**
```bash
#!/bin/bash
# auto_rbac_manager.sh

TEAM=$1
NAMESPACE=$2
ACTION=${3:-apply}

case $ACTION in
  "apply")
    # 创建团队命名空间和权限
    kubectl create namespace $NAMESPACE
    kubectl apply -f rbac/team-$TEAM-role.yaml -n $NAMESPACE
    kubectl apply -f rbac/team-$TEAM-rolebinding.yaml -n $NAMESPACE
    ;;
  "revoke")
    # 撤销团队权限
    kubectl delete rolebinding team-$TEAM-rolebinding -n $NAMESPACE
    ;;
  "audit")
    # 权限审计
    kubectl auth can-i list pods --as=system:serviceaccount:$NAMESPACE:$TEAM-sa
    ;;
esac
```

### 4.2 ResourceQuota优化策略

#### 4.2.1 分层配额管理体系

**多级配额配置：**
```yaml
# 集群级硬限制
apiVersion: v1
kind: ResourceQuota
metadata:
  name: cluster-hard-limits
  namespace: default
spec:
  hard:
    requests.cpu: "100"
    requests.memory: "200Gi"
    limits.cpu: "200"
    limits.memory: "400Gi"
    persistentvolumeclaims: "1000"
    services.loadbalancers: "50"

---
# 团队级软限制
apiVersion: v1
kind: ResourceQuota
metadata:
  name: team-soft-limits
  namespace: team-dev
spec:
  hard:
    requests.cpu: "20"
    requests.memory: "40Gi"
    limits.cpu: "40"
    limits.memory: "80Gi"
  scopes:
  - NotTerminating
```

**配额使用监控仪表板：**
```bash
#!/bin/bash
# quota_monitor.sh

echo "=== ResourceQuota使用情况 ==="

kubectl get resourcequota --all-namespaces -o custom-columns=NAMESPACE:.metadata.namespace,NAME:.metadata.name,USED_CPU:.status.used.requests\.cpu,LIMIT_CPU:.status.hard.requests\.cpu,USED_MEM:.status.used.requests\.memory,LIMIT_MEM:.status.hard.requests\.memory --no-headers | \
  awk '{
    cpu_util = ($3/$4)*100
    mem_util = ($5/$6)*100
    printf "%s/%s - CPU: %.1f%%, Memory: %.1f%%\n", $1, $2, cpu_util, mem_util
  }' | sort -k5 -nr
```

#### 4.2.2 动态配额调整机制

**基于使用率的自动配额调整：**
```bash
#!/bin/bash
# dynamic_quota_adjustment.sh

NAMESPACE=$1
THRESHOLD_HIGH=80
THRESHOLD_LOW=30

# 获取当前使用率
CPU_USAGE=$(kubectl get resourcequota -n $NAMESPACE -o jsonpath='{.items[0].status.used.requests\.cpu}')
CPU_LIMIT=$(kubectl get resourcequota -n $NAMESPACE -o jsonpath='{.items[0].status.hard.requests\.cpu}')

USAGE_PERCENT=$((CPU_USAGE * 100 / CPU_LIMIT))

if [ $USAGE_PERCENT -gt $THRESHOLD_HIGH ]; then
  # 扩容配额
  NEW_LIMIT=$((CPU_LIMIT * 120 / 100))
  kubectl patch resourcequota default -n $NAMESPACE -p "{\"spec\":{\"hard\":{\"requests.cpu\":\"${NEW_LIMIT}\"}}}"
  echo "已扩容CPU配额至 ${NEW_LIMIT}"
elif [ $USAGE_PERCENT -lt $THRESHOLD_LOW ]; then
  # 缩容配额
  NEW_LIMIT=$((CPU_LIMIT * 80 / 100))
  kubectl patch resourcequota default -n $NAMESPACE -p "{\"spec\":{\"hard\":{\"requests.cpu\":\"${NEW_LIMIT}\"}}}"
  echo "已缩容CPU配额至 ${NEW_LIMIT}"
fi
```

### 4.3 LimitRange标准化配置

#### 4.3.1 应用类型差异化限制

**Web应用限制配置：**
```yaml
apiVersion: v1
kind: LimitRange
metadata:
  name: webapp-limits
  namespace: webapps
spec:
  limits:
  - type: Container
    default:
      cpu: "500m"
      memory: "512Mi"
    defaultRequest:
      cpu: "100m"
      memory: "256Mi"
    max:
      cpu: "2"
      memory: "4Gi"
    min:
      cpu: "50m"
      memory: "64Mi"
```

**批处理应用限制配置：**
```yaml
apiVersion: v1
kind: LimitRange
metadata:
  name: batch-limits
  namespace: batch-jobs
spec:
  limits:
  - type: Container
    default:
      cpu: "1"
      memory: "2Gi"
    defaultRequest:
      cpu: "500m"
      memory: "1Gi"
    max:
      cpu: "4"
      memory: "16Gi"
```

#### 4.3.2 限制范围验证工具

**LimitRange合规性检查：**
```bash
#!/bin/bash
# limitrange_validator.sh

echo "=== LimitRange合规性检查 ==="

kubectl get limitrange --all-namespaces -o jsonpath='{range .items[*]}{.metadata.namespace}{" "}{.metadata.name}{"\n"}{range .spec.limits[*]}{.type}{" "}{.defaultRequest.cpu}{" "}{.default.memory}{"\n"}{end}{end}' | \
  while read line; do
    if echo "$line" | grep -q "Container"; then
      cpu_req=$(echo "$line" | awk '{print $2}')
      mem_req=$(echo "$line" | awk '{print $3}')
      
      # 检查是否符合最小要求
      if [[ "$cpu_req" < "100m" ]] || [[ "$mem_req" < "128Mi" ]]; then
        echo "⚠️  不合规配置: $line"
      fi
    fi
  done
```

### 4.4 综合治理平台

#### 4.4.1 统一权限管理中心

**权限申请审批流程：**
```yaml
# 权限申请CRD
apiVersion: rbac.k8s.io/v1
kind: PermissionRequest
metadata:
  name: app-team-access-request
spec:
  requester: "app-team"
  namespace: "app-production"
  permissions:
  - resource: "pods"
    verbs: ["get", "list", "watch"]
  - resource: "services"
    verbs: ["get", "list"]
  duration: "30d"
  justification: "应用监控和故障排查需要"
```

#### 4.4.2 自动化合规检查

**定期合规扫描脚本：**
```bash
#!/bin/bash
# compliance_scanner.sh

DATE=$(date +%Y%m%d)
REPORT_FILE="/var/reports/rbac_compliance_$DATE.txt"

{
  echo "=== RBAC & Quota 合规扫描报告 - $(date) ==="
  echo ""
  
  # 权限过度授予检查
  echo "1. 权限过度授予检查:"
  ./scripts/rbac_audit.sh
  
  echo ""
  # 配额使用异常检查
  echo "2. 配额使用异常检查:"
  ./scripts/quota_monitor.sh
  
  echo ""
  # LimitRange合规性检查
  echo "3. LimitRange合规性检查:"
  ./scripts/limitrange_validator.sh
  
} > $REPORT_FILE

# 发送告警邮件
if grep -q "⚠️" $REPORT_FILE; then
  mail -s "RBAC合规警告" ops-team@company.com < $REPORT_FILE
fi
```

---

