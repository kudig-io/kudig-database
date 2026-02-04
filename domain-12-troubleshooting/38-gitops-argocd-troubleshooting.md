# 38 - GitOps和ArgoCD故障排查 (GitOps and ArgoCD Troubleshooting)

> **适用版本**: Kubernetes v1.25-v1.32 | ArgoCD v2.4+ | **最后更新**: 2026-02 | **参考**: [ArgoCD Documentation](https://argo-cd.readthedocs.io/)

---

## 1. GitOps和ArgoCD故障诊断总览 (GitOps and ArgoCD Diagnosis Overview)

### 1.1 常见故障现象分类

| 故障类型 | 症状表现 | 影响范围 | 紧急程度 |
|---------|---------|---------|---------|
| **应用同步失败** | Application状态异常 | 应用部署中断 | P0 - 紧急 |
| **Git仓库连接问题** | 无法访问代码仓库 | 部署流水线中断 | P0 - 紧急 |
| **权限认证失败** | RBAC/Token无效 | GitOps操作受限 | P1 - 高 |
| **资源冲突** | Kubernetes资源冲突 | 应用状态不一致 | P1 - 高 |
| **性能瓶颈** | ArgoCD响应缓慢 | 运维效率降低 | P2 - 中 |

### 1.2 GitOps架构回顾

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                    GitOps故障诊断架构                                       │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  ┌─────────────────────────────────────────────────────────────────────┐  │
│  │                      开发层                                         │  │
│  │  ┌─────────────┐    ┌─────────────┐    ┌─────────────┐              │  │
│  │  │   Git仓库   │    │   CI/CD     │    │   开发者    │              │  │
│  │  │ (GitHub)    │    │  (Pipeline) │    │ (Developer) │              │  │
│  │  └─────────────┘    └─────────────┘    └─────────────┘              │  │
│  └─────────────────────────────────────────────────────────────────────┘  │
│                              │                                              │
│         ┌────────────────────┼────────────────────┐                       │
│         │                    │                    │                       │
│         ▼                    ▼                    ▼                       │
│  ┌─────────────┐    ┌─────────────┐    ┌─────────────┐                   │
│  │   源码管理   │   │   构建打包   │   │   代码审查   │                   │
│  │ (Source)    │   │  (Build)    │   │  (Review)   │                   │
│  │   变更      │   │   镜像      │   │   合并      │                   │
│  └─────────────┘    └─────────────┘    └─────────────┘                   │
│         │                    │                    │                       │
│         ▼                    ▼                    ▼                       │
│  ┌─────────────────────────────────────────────────────────────────────┐  │
│  │                      GitOps控制器层                                 │  │
│  │  ┌───────────────────────────────────────────────────────────────┐  │  │
│  │  │                       ArgoCD                                  │  │  │
│  │  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐           │  │  │
│  │  │  │   Repo Server│  │  App Controller│ │   API Server │           │  │  │
│  │  │  │  (Git Sync)  │  │  (Reconcile)   │ │  (Interface) │           │  │  │
│  │  │  └─────────────┘  └─────────────┘  └─────────────┘           │  │  │
│  │  └───────────────────────────────────────────────────────────────┘  │  │
│  └─────────────────────────────────────────────────────────────────────┘  │
│                              │                                              │
│         ┌────────────────────┼────────────────────┐                       │
│         │                    │                    │                       │
│         ▼                    ▼                    ▼                       │
│  ┌─────────────┐    ┌─────────────┐    ┌─────────────┐                   │
│  │   配置管理   │   │   状态监控   │   │   权限控制   │                   │
│  │ (Config)    │   │  (Monitor)  │   │  (RBAC)     │                   │
│  │   同步      │   │   对比      │   │   认证      │                   │
│  └─────────────┘    └─────────────┘    └─────────────┘                   │
│                              │                                              │
│                              ▼                                              │
│  ┌─────────────────────────────────────────────────────────────────────┐  │
│  │                      Kubernetes集群                                 │  │
│  │  ┌─────────────┐    ┌─────────────┐    ┌─────────────┐              │  │
│  │  │   API Server│    │   etcd      │    │   Worker    │              │  │
│  │  │  (Control)  │    │  (Storage)  │    │  (Nodes)    │              │  │
│  │  └─────────────┘    └─────────────┘    └─────────────┘              │  │
│  └─────────────────────────────────────────────────────────────────────┘  │
│                              │                                              │
│                              ▼                                              │
│  ┌─────────────────────────────────────────────────────────────────────┐  │
│  │                      应用运行层                                     │  │
│  │  ┌─────────────┐    ┌─────────────┐    ┌─────────────┐              │  │
│  │  │   Deployments│   │   Services  │    │   Configs   │              │  │
│  │  │  (应用)     │    │  (服务)     │    │  (配置)     │              │  │
│  │  └─────────────┘    └─────────────┘    └─────────────┘              │  │
│  └─────────────────────────────────────────────────────────────────────┘  │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

---

## 2. 应用同步和部署故障排查 (Application Sync and Deployment Issues)

### 2.1 应用状态分析

```bash
# ========== 1. 应用状态检查 ==========
# 查看所有应用状态
argocd app list

# 检查特定应用详细信息
argocd app get <application-name>

# 分析应用健康状态
argocd app get <application-name> --refresh

# 查看应用事件和历史
argocd app history <application-name>

# ========== 2. 资源状态对比 ==========
# 比较期望状态和实际状态
argocd app diff <application-name>

# 查看具体资源差异
argocd app diff <application-name> --kind Deployment --name <deployment-name>

# 导出差异到文件
argocd app diff <application-name> > diff-report.txt

# ========== 3. 同步失败诊断 ==========
# 强制刷新应用状态
argocd app get <application-name> --hard-refresh

# 检查同步状态详细信息
argocd app get <application-name> --show-operation

# 分析同步操作历史
argocd app history <application-name> --operation-id <operation-id>

# 查看同步操作日志
argocd app logs <application-name>
```

### 2.2 同步策略和配置问题

```bash
# ========== 同步配置验证 ==========
# 检查应用同步策略
argocd app get <application-name> -o yaml | grep -A 10 syncPolicy

# 验证自动同步配置
argocd app get <application-name> -o yaml | grep -A 5 automated

# 检查忽略差异配置
argocd app get <application-name> -o yaml | grep -A 10 ignoreDifferences

# ========== 手动同步操作 ==========
# 手动触发同步
argocd app sync <application-name>

# 强制同步（覆盖现有资源）
argocd app sync <application-name> --force

# 选择性同步特定资源
argocd app sync <application-name> --resource kind:name

# 并行同步多个应用
for app in $(argocd app list -o name); do
    echo "Syncing $app"
    argocd app sync $app --prune &
done
wait

# ========== 回滚操作 ==========
# 回滚到指定版本
argocd app rollback <application-name> <revision>

# 查看可回滚的历史版本
argocd app history <application-name> | grep -E "(Deployed|Healthy)"

# 预演回滚操作
argocd app diff <application-name> --revision <target-revision>
```

### 2.3 资源冲突解决

```bash
# ========== 资源所有权检查 ==========
# 查看资源被哪个应用管理
kubectl get <resource-type> <resource-name> -o jsonpath='{.metadata.annotations."argocd.argoproj.io/tracking-id"}'

# 检查资源冲突
argocd app get <application-name> --show-resources | grep -E "(OutOfSync|Missing)"

# ========== 强制获取资源所有权 ==========
# 从其他应用夺取资源所有权
argocd app patch-resource <application-name> \
    --kind <resource-kind> \
    --resource-name <resource-name> \
    --patch '{"metadata": {"annotations": {"argocd.argoproj.io/tracking-id": "<new-tracking-id>"}}}' \
    --patch-type merge

# ========== 清理孤儿资源 ==========
# 删除不再被管理的资源
argocd app sync <application-name> --prune

# 查看将被清理的资源
argocd app sync <application-name> --prune --dry-run

# 手动清理特定资源
kubectl delete <resource-type> <resource-name> -n <namespace>
```

---

## 3. Git仓库连接和认证问题排查 (Git Repository Connection and Authentication Issues)

### 3.1 Git仓库连接状态检查

```bash
# ========== 1. 仓库连接验证 ==========
# 测试Git仓库连接
argocd repo list

# 验证特定仓库连接
argocd repo get <repository-url>

# 测试仓库访问权限
argocd repo get <repository-url> --refresh

# 检查仓库状态详细信息
argocd repo get <repository-url> -o yaml

# ========== 2. SSH密钥和认证问题 ==========
# 检查SSH密钥配置
argocd cert list

# 验证SSH密钥有效性
ssh -T git@github.com

# 测试Git克隆操作
git clone <repository-url> /tmp/test-repo

# 检查known_hosts配置
cat ~/.ssh/known_hosts | grep github.com

# ========== 3. HTTPS认证问题 ==========
# 验证HTTPS凭据
argocd repo get <https-repository-url> --refresh

# 测试HTTPS访问
curl -u <username>:<token> <repository-url>

# 检查Git凭据存储
git config --global credential.helper store
cat ~/.git-credentials

# ========== 仓库连接诊断工具 ==========
# 创建仓库连接测试脚本
cat <<'EOF' > repo-connection-test.sh
#!/bin/bash

REPO_URL=$1

if [ -z "$REPO_URL" ]; then
    echo "Usage: $0 <repo-url>"
    exit 1
fi

echo "Testing repository connection: $REPO_URL"

# 1. ArgoCD仓库状态检查
echo "1. Checking ArgoCD repository status..."
argocd repo get $REPO_URL --refresh

# 2. 直接Git连接测试
echo "2. Testing direct Git connection..."
if echo $REPO_URL | grep -q "^git@"; then
    # SSH测试
    ssh -T $(echo $REPO_URL | sed 's/git@//' | sed 's/\/.*//') 2>&1 | grep -q "successfully authenticated" && \
        echo "  ✓ SSH authentication successful" || \
        echo "  ✗ SSH authentication failed"
elif echo $REPO_URL | grep -q "^https://"; then
    # HTTPS测试
    curl -s -o /dev/null -w "%{http_code}" $REPO_URL | grep -q "200\|301\|302" && \
        echo "  ✓ HTTPS connection successful" || \
        echo "  ✗ HTTPS connection failed"
fi

# 3. 克隆测试
echo "3. Testing repository clone..."
TEST_DIR="/tmp/repo-test-$(date +%s)"
git clone $REPO_URL $TEST_DIR && \
    echo "  ✓ Repository clone successful" && \
    rm -rf $TEST_DIR || \
    echo "  ✗ Repository clone failed"

# 4. 网络连通性测试
echo "4. Testing network connectivity..."
REPO_HOST=$(echo $REPO_URL | sed 's|^.*://||' | sed 's|/.*||' | sed 's|:.*||')
ping -c 3 $REPO_HOST && \
    echo "  ✓ Network connectivity OK" || \
    echo "  ✗ Network connectivity failed"

echo "Connection test completed"
EOF

chmod +x repo-connection-test.sh
```

### 3.2 凭据管理和轮换

```bash
# ========== 凭据配置 ==========
# 添加SSH私钥
argocd repo add git@github.com:org/repo.git --ssh-private-key-path ~/.ssh/id_rsa

# 添加HTTPS凭据
argocd repo add https://github.com/org/repo.git --username <username> --password <token>

# 使用凭据模板
argocd repocreds add https://github.com --username <username> --password <token>

# ========== 凭据轮换策略 ==========
# 创建凭据轮换脚本
cat <<'EOF' > credential-rotation.sh
#!/bin/bash

# 凭据轮换脚本
echo "Starting credential rotation process"

# 1. 备份当前凭据
echo "Backing up current credentials..."
argocd repo list -o yaml > repo-backup-$(date +%Y%m%d).yaml

# 2. 生成新凭据（这里示例使用GitHub API）
echo "Generating new GitHub token..."
NEW_TOKEN=$(curl -s -u "<username>:<personal-access-token>" \
    -X POST \
    -H "Accept: application/vnd.github.v3+json" \
    https://api.github.com/authorizations \
    -d '{"scopes":["repo"],"note":"argocd-$(date +%Y%m%d)"}' | \
    jq -r '.token')

# 3. 更新ArgoCD凭据
echo "Updating ArgoCD credentials..."
argocd repo add https://github.com/org/repo.git \
    --username <username> \
    --password $NEW_TOKEN \
    --upsert

# 4. 验证新凭据
echo "Verifying new credentials..."
argocd repo get https://github.com/org/repo.git --refresh

# 5. 清理旧凭据
echo "Cleaning up old credentials..."
# 这里可以添加清理逻辑

echo "Credential rotation completed"
EOF

chmod +x credential-rotation.sh

# ========== 凭据安全性检查 ==========
# 检查凭据泄露风险
argocd admin settings validate --redis-secret <secret-name>

# 验证凭据权限范围
curl -s -u "<username>:<token>" https://api.github.com/user/repos | jq '.[].permissions'

# 审计凭据使用历史
kubectl logs -n argocd -l app.kubernetes.io/name=argocd-repo-server --since=24h | grep -i "auth\|credential"
```

---

## 4. 权限控制和RBAC问题排查 (Access Control and RBAC Issues)

### 4.1 RBAC配置验证

```bash
# ========== 1. 用户权限检查 ==========
# 查看当前用户权限
argocd account get-user-info

# 测试特定操作权限
argocd app list --as <username>

# 验证RBAC策略
argocd admin settings rbac validate --policy-file rbac-policy.csv

# 查看用户角色分配
argocd account list

# ========== 2. 角色和策略管理 ==========
# 查看RBAC策略
kubectl get configmap argocd-rbac-cm -n argocd -o yaml

# 验证策略语法
argocd admin settings rbac validate --policy "p, admin, applications, *, */*, allow"

# 测试策略效果
argocd admin settings rbac can <username> <resource> <action> <object>

# ========== 3. OIDC集成问题 ==========
# 检查OIDC配置
kubectl get configmap argocd-cm -n argocd -o yaml | grep -A 20 oidc

# 验证OIDC连接
argocd account list --as <oidc-username>

# 测试令牌有效性
curl -s https://<argocd-server>/api/version -H "Authorization: Bearer <jwt-token>"

# ========== 权限诊断工具 ==========
# 创建权限诊断脚本
cat <<'EOF' > rbac-diagnostic.sh
#!/bin/bash

USERNAME=${1:-admin}

echo "=== RBAC Diagnostic for user: $USERNAME ==="

# 1. 基本用户信息
echo "1. User Information:"
argocd account get-user-info --as $USERNAME

# 2. 角色分配检查
echo "2. Role Assignments:"
ROLES=$(argocd account get $USERNAME -o jsonpath='{.capabilities.roles[*]}' 2>/dev/null)
if [ -n "$ROLES" ]; then
    echo "  Assigned roles: $ROLES"
else
    echo "  No roles assigned"
fi

# 3. 权限测试矩阵
echo "3. Permission Matrix:"

RESOURCES=("applications" "projects" "repositories" "clusters")
ACTIONS=("get" "create" "update" "delete")

for resource in "${RESOURCES[@]}"; do
    for action in "${ACTIONS[@]}"; do
        if argocd admin settings rbac can $USERNAME $resource $action "*" 2>/dev/null; then
            echo "  ✓ $USERNAME can $action $resource"
        else
            echo "  ✗ $USERNAME cannot $action $resource"
        fi
    done
done

# 4. 项目权限检查
echo "4. Project Permissions:"
for project in $(argocd proj list -o name); do
    echo "  Project: $project"
    argocd proj get $project -o yaml | grep -A 5 roles
done

echo "RBAC diagnostic completed"
EOF

chmod +x rbac-diagnostic.sh
```

### 4.2 项目和应用权限管理

```bash
# ========== 项目权限配置 ==========
# 查看项目配置
argocd proj get <project-name>

# 验证项目角色
argocd proj role list <project-name>

# 检查项目策略
argocd proj get <project-name> -o yaml | grep -A 20 roles

# ========== 应用权限边界 ==========
# 查看应用所属项目
argocd app get <application-name> -o jsonpath='{.spec.project}'

# 验证应用权限范围
argocd app get <application-name> -o yaml | grep -A 10 destination

# 检查命名空间限制
argocd proj get <project-name> -o jsonpath='{.spec.destinations[*].namespace}'

# ========== 权限继承关系 ==========
# 分析权限继承
kubectl get applications -A -o jsonpath='{
    range .items[*]
}{
        .metadata.name
}{
        "\tProject: "
}{
        .spec.project
}{
        "\n"
}{
    end
}'
```

---

## 5. 性能优化和监控配置 (Performance Optimization and Monitoring)

### 5.1 ArgoCD性能监控

```bash
# ========== 性能指标收集 ==========
# 查看ArgoCD组件资源使用
kubectl top pods -n argocd

# 监控API服务器性能
kubectl exec -n argocd svc/argocd-server -- curl -s http://localhost:8082/metrics

# 检查Redis缓存性能
kubectl exec -n argocd svc/argocd-redis -- redis-cli info stats

# ========== 性能瓶颈诊断 ==========
# 创建性能分析脚本
cat <<'EOF' > argocd-performance-analyzer.sh
#!/bin/bash

echo "=== ArgoCD Performance Analysis ==="

# 1. 组件资源使用
echo "1. Component Resource Usage:"
kubectl top pods -n argocd

# 2. 应用同步性能
echo "2. Application Sync Performance:"
for app in $(argocd app list -o name | head -10); do
    SYNC_TIME=$(argocd app history $app --limit 1 -o jsonpath='{.items[0].finishedAt}' 2>/dev/null)
    if [ -n "$SYNC_TIME" ]; then
        echo "  $app: Last sync at $SYNC_TIME"
    fi
done

# 3. Git操作延迟
echo "3. Git Operation Latency:"
kubectl logs -n argocd -l app.kubernetes.io/name=argocd-repo-server --since=1h | \
    grep -i "git.*took" | \
    awk '{print $NF}' | \
    sort -n | \
    tail -10

# 4. API响应时间
echo "4. API Response Times:"
kubectl logs -n argocd -l app.kubernetes.io/name=argocd-server --since=1h | \
    grep -i "response.*time" | \
    awk '{print $NF}' | \
    sort -n | \
    tail -10

echo "Performance analysis completed"
EOF

chmod +x argocd-performance-analyzer.sh

# ========== 性能优化配置 ==========
# 调整ArgoCD资源配置
cat <<EOF | kubectl apply -f -
apiVersion: apps/v1
kind: Deployment
metadata:
  name: argocd-application-controller
  namespace: argocd
spec:
  template:
    spec:
      containers:
      - name: application-controller
        resources:
          requests:
            cpu: "500m"
            memory: "1Gi"
          limits:
            cpu: "2"
            memory: "4Gi"
        env:
        - name: ARGOCD_CONTROLLER_REPLICAS
          value: "2"
        - name: ARGOCD_APPLICATION_CONTROLLER_REPO_SERVER_TIMEOUT_SECONDS
          value: "180"
EOF
```

### 5.2 监控告警配置

```bash
# ========== Prometheus监控集成 ==========
# 创建ServiceMonitor配置
cat <<EOF | kubectl apply -f -
apiVersion: monitoring.coreos.com/v1
kind: ServiceMonitor
metadata:
  name: argocd-metrics
  namespace: monitoring
spec:
  selector:
    matchLabels:
      app.kubernetes.io/name: argocd-metrics
  endpoints:
  - port: metrics
    path: /metrics
    interval: 30s
    metricRelabelings:
    - sourceLabels: [__name__]
      regex: 'argocd_.*'
      action: keep
---
apiVersion: v1
kind: Service
metadata:
  name: argocd-metrics
  namespace: argocd
  labels:
    app.kubernetes.io/name: argocd-metrics
spec:
  selector:
    app.kubernetes.io/name: argocd-server
  ports:
  - name: metrics
    port: 8082
    targetPort: 8082
EOF

# ========== 告警规则配置 ==========
cat <<EOF | kubectl apply -f -
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: argocd-alerts
  namespace: monitoring
spec:
  groups:
  - name: argocd.rules
    rules:
    - alert: ArgoCDDown
      expr: up{job="argocd-server"} == 0
      for: 2m
      labels:
        severity: critical
      annotations:
        summary: "ArgoCD server is down"
        
    - alert: ApplicationOutOfSync
      expr: argocd_app_info{sync_status="OutOfSync"} > 0
      for: 5m
      labels:
        severity: warning
      annotations:
        summary: "{{ \$labels.name }} application is out of sync"
        
    - alert: ApplicationDegraded
      expr: argocd_app_info{health_status="Degraded"} > 0
      for: 3m
      labels:
        severity: critical
      annotations:
        summary: "{{ \$labels.name }} application is degraded"
        
    - alert: GitRepoSyncFailure
      expr: increase(argocd_git_request_total{request_type="ls-remote", response_status="500"}[5m]) > 3
      for: 2m
      labels:
        severity: warning
      annotations:
        summary: "Git repository sync failures detected"
        
    - alert: HighSyncDuration
      expr: histogram_quantile(0.99, rate(argocd_app_sync_duration_seconds_bucket[5m])) > 300
      for: 5m
      labels:
        severity: warning
      annotations:
        summary: "High application sync duration ({{ \$value }}s)"
        
    - alert: ArgoCDHighMemoryUsage
      expr: container_memory_usage_bytes{container="application-controller", namespace="argocd"} > 3*1024*1024*1024
      for: 5m
      labels:
        severity: warning
      annotations:
        summary: "ArgoCD application controller high memory usage ({{ \$value | humanize1024 }})"
EOF

# ========== 自定义监控面板 ==========
# 创建Grafana仪表板配置
cat <<'EOF' > argocd-dashboard.json
{
  "dashboard": {
    "title": "ArgoCD Operations Overview",
    "panels": [
      {
        "title": "Application Health Status",
        "type": "stat",
        "targets": [
          {
            "expr": "count(argocd_app_info{health_status=\"Healthy\"})",
            "legendFormat": "Healthy"
          },
          {
            "expr": "count(argocd_app_info{health_status=\"Degraded\"})",
            "legendFormat": "Degraded"
          },
          {
            "expr": "count(argocd_app_info{sync_status=\"OutOfSync\"})",
            "legendFormat": "OutOfSync"
          }
        ]
      },
      {
        "title": "Sync Operations Over Time",
        "type": "graph",
        "targets": [
          {
            "expr": "rate(argocd_app_sync_total[5m])",
            "legendFormat": "{{status}}"
          }
        ]
      },
      {
        "title": "Git Operations Latency",
        "type": "heatmap",
        "targets": [
          {
            "expr": "rate(argocd_git_request_duration_seconds_bucket[5m])",
            "legendFormat": "{{le}}"
          }
        ]
      },
      {
        "title": "Application Controller Resource Usage",
        "type": "graph",
        "targets": [
          {
            "expr": "container_cpu_usage_seconds_total{container=\"application-controller\", namespace=\"argocd\"}",
            "legendFormat": "CPU Usage"
          },
          {
            "expr": "container_memory_usage_bytes{container=\"application-controller\", namespace=\"argocd\"}",
            "legendFormat": "Memory Usage"
          }
        ]
      }
    ]
  }
}
EOF
```

---

## 6. 灾难恢复和备份策略 (Disaster Recovery and Backup Strategies)

### 6.1 配置备份和恢复

```bash
# ========== 配置备份脚本 ==========
# 创建完整备份脚本
cat <<'EOF' > argocd-backup.sh
#!/bin/bash

BACKUP_DIR=${1:-"/backup/argocd"}
TIMESTAMP=$(date +%Y%m%d-%H%M%S)

echo "Starting ArgoCD backup to $BACKUP_DIR/$TIMESTAMP"

mkdir -p $BACKUP_DIR/$TIMESTAMP

# 1. 备份ArgoCD配置
echo "Backing up ArgoCD configuration..."
kubectl get configmap -n argocd -o yaml > $BACKUP_DIR/$TIMESTAMP/configmaps.yaml
kubectl get secret -n argocd -o yaml > $BACKUP_DIR/$TIMESTAMP/secrets.yaml

# 2. 备份应用定义
echo "Backing up applications..."
kubectl get applications -A -o yaml > $BACKUP_DIR/$TIMESTAMP/applications.yaml

# 3. 备份项目配置
echo "Backing up projects..."
kubectl get appprojects -A -o yaml > $BACKUP_DIR/$TIMESTAMP/projects.yaml

# 4. 备份仓库配置
echo "Backing up repositories..."
argocd repo list -o yaml > $BACKUP_DIR/$TIMESTAMP/repositories.yaml

# 5. 备份集群配置
echo "Backing up clusters..."
argocd cluster list -o yaml > $BACKUP_DIR/$TIMESTAMP/clusters.yaml

# 6. 备份RBAC配置
echo "Backing up RBAC configuration..."
kubectl get configmap argocd-rbac-cm -n argocd -o yaml > $BACKUP_DIR/$TIMESTAMP/rbac-config.yaml

# 7. 创建备份元数据
cat > $BACKUP_DIR/$TIMESTAMP/metadata.json <<METADATA
{
  "timestamp": "$TIMESTAMP",
  "argocd_version": "$(argocd version --client --short)",
  "kubernetes_version": "$(kubectl version --short)",
  "backup_components": [
    "configmaps",
    "secrets", 
    "applications",
    "projects",
    "repositories",
    "clusters",
    "rbac"
  ],
  "created_by": "$(whoami)"
}
METADATA

# 8. 压缩备份
tar -czf $BACKUP_DIR/argocd-backup-$TIMESTAMP.tar.gz -C $BACKUP_DIR $TIMESTAMP
rm -rf $BACKUP_DIR/$TIMESTAMP

echo "Backup completed: $BACKUP_DIR/argocd-backup-$TIMESTAMP.tar.gz"
EOF

chmod +x argocd-backup.sh

# ========== 恢复脚本 ==========
# 创建恢复脚本
cat <<'EOF' > argocd-restore.sh
#!/bin/bash

BACKUP_FILE=$1
RESTORE_NAMESPACE=${2:-argocd}

if [ -z "$BACKUP_FILE" ]; then
    echo "Usage: $0 <backup-file.tar.gz> [namespace]"
    exit 1
fi

echo "Starting ArgoCD restore from $BACKUP_FILE to namespace $RESTORE_NAMESPACE"

# 解压备份
TEMP_DIR="/tmp/argocd-restore-$(date +%s)"
mkdir -p $TEMP_DIR
tar -xzf $BACKUP_FILE -C $TEMP_DIR

BACKUP_CONTENTS=$(ls $TEMP_DIR)
BACKUP_TIMESTAMP=$(basename $BACKUP_CONTENTS)

echo "Restoring from backup: $BACKUP_TIMESTAMP"

# 1. 恢复命名空间
kubectl create namespace $RESTORE_NAMESPACE --dry-run=client -o yaml | kubectl apply -f -

# 2. 恢复配置映射
echo "Restoring ConfigMaps..."
kubectl apply -f $TEMP_DIR/$BACKUP_TIMESTAMP/configmaps.yaml -n $RESTORE_NAMESPACE

# 3. 恢复密钥
echo "Restoring Secrets..."
kubectl apply -f $TEMP_DIR/$BACKUP_TIMESTAMP/secrets.yaml -n $RESTORE_NAMESPACE

# 4. 等待ArgoCD启动
echo "Waiting for ArgoCD to start..."
sleep 60

# 5. 恢复应用
echo "Restoring Applications..."
kubectl apply -f $TEMP_DIR/$BACKUP_TIMESTAMP/applications.yaml

# 6. 恢复项目
echo "Restoring Projects..."
kubectl apply -f $TEMP_DIR/$BACKUP_TIMESTAMP/projects.yaml

# 7. 恢复仓库配置
echo "Restoring Repositories..."
argocd repo import $TEMP_DIR/$BACKUP_TIMESTAMP/repositories.yaml

# 8. 恢复集群配置
echo "Restoring Clusters..."
argocd cluster import $TEMP_DIR/$BACKUP_TIMESTAMP/clusters.yaml

# 9. 恢复RBAC配置
echo "Restoring RBAC configuration..."
kubectl apply -f $TEMP_DIR/$BACKUP_TIMESTAMP/rbac-config.yaml -n $RESTORE_NAMESPACE

# 清理临时目录
rm -rf $TEMP_DIR

echo "Restore completed"
EOF

chmod +x argocd-restore.sh
```

### 6.2 高可用和故障转移

```bash
# ========== 高可用部署配置 ==========
# 配置多副本控制器
cat <<EOF | kubectl apply -f -
apiVersion: apps/v1
kind: Deployment
metadata:
  name: argocd-application-controller
  namespace: argocd
spec:
  replicas: 3
  selector:
    matchLabels:
      app.kubernetes.io/name: argocd-application-controller
  template:
    metadata:
      labels:
        app.kubernetes.io/name: argocd-application-controller
    spec:
      affinity:
        podAntiAffinity:
          preferredDuringSchedulingIgnoredDuringExecution:
          - weight: 100
            podAffinityTerm:
              labelSelector:
                matchLabels:
                  app.kubernetes.io/name: argocd-application-controller
              topologyKey: kubernetes.io/hostname
      containers:
      - name: application-controller
        image: quay.io/argoproj/argocd:v2.4.0
        args:
        - /usr/local/bin/argocd-application-controller
        - --status-processors
        - "20"
        - --operation-processors
        - "10"
        - --repo-server
        - argocd-repo-server:8081
        env:
        - name: ARGOCD_CONTROLLER_SHARDING_ENABLED
          value: "true"
        - name: ARGOCD_CONTROLLER_SHARDING_ALGORITHM
          value: "legacy"
EOF

# ========== 故障转移配置 ==========
# 创建故障转移控制器
cat <<'EOF' > failover-controller.yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: argocd-failover-controller
  namespace: argocd
spec:
  replicas: 1
  selector:
    matchLabels:
      app: argocd-failover-controller
  template:
    metadata:
      labels:
        app: argocd-failover-controller
    spec:
      containers:
      - name: controller
        image: argocd-failover-controller:latest
        env:
        - name: HEALTH_CHECK_INTERVAL
          value: "30s"
        - name: FAILOVER_THRESHOLD
          value: "3"
        - name: PRIMARY_CLUSTER
          value: "main-cluster"
        - name: STANDBY_CLUSTERS
          value: "backup-cluster1,backup-cluster2"
        volumeMounts:
        - name: kubeconfig
          mountPath: /root/.kube
          readOnly: true
      volumes:
      - name: kubeconfig
        secret:
          secretName: argocd-kubeconfig
---
apiVersion: batch/v1
kind: CronJob
metadata:
  name: argocd-health-check
  namespace: argocd
spec:
  schedule: "*/5 * * * *"
  jobTemplate:
    spec:
      template:
        spec:
          containers:
          - name: health-check
            image: curlimages/curl
            command:
            - /bin/sh
            - -c
            - |
              # 检查ArgoCD健康状态
              curl -sf http://argocd-server/healthz || {
                echo "ArgoCD health check failed"
                # 发送告警
                curl -X POST http://alertmanager:9093/api/v1/alerts \
                  -H "Content-Type: application/json" \
                  -d '[{
                    "status": "firing",
                    "labels": {
                      "alertname": "ArgoCDHealthCheckFailed",
                      "severity": "critical"
                    },
                    "annotations": {
                      "summary": "ArgoCD health check failed"
                    }
                  }]'
              }
          restartPolicy: OnFailure
EOF
```

---