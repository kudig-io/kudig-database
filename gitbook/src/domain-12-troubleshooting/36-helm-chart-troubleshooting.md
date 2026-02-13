# 36 - Helm Chart 故障排查 (Helm Chart Troubleshooting)

> **适用版本**: Kubernetes v1.25-v1.32 | Helm v3.8+ | **最后更新**: 2026-02 | **参考**: [Helm Documentation](https://helm.sh/docs/)

---

## 1. Helm故障诊断总览 (Helm Diagnosis Overview)

### 1.1 常见故障现象分类

| 故障类型 | 症状表现 | 影响范围 | 紧急程度 |
|---------|---------|---------|---------|
| **Chart渲染失败** | template执行错误 | 应用部署中断 | P0 - 紧急 |
| **依赖管理问题** | 依赖下载失败 | 复杂应用无法部署 | P1 - 高 |
| **版本兼容性** | API版本不匹配 | Chart无法安装 | P1 - 高 |
| **配置参数错误** | values.yaml配置不当 | 应用功能异常 | P1 - 高 |
| **Release状态异常** | Release卡住/失败 | 应用状态不一致 | P0 - 紧急 |

### 1.2 Helm架构回顾

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                      Helm故障诊断架构                                        │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  ┌─────────────────────────────────────────────────────────────────────┐  │
│  │                       Helm客户端层                                    │  │
│  │  ┌─────────────┐    ┌─────────────┐    ┌─────────────┐              │  │
│  │  │   CLI工具   │    │   SDK接口   │    │   IDE插件   │              │  │
│  │  │  (helm)     │    │  (Go SDK)   │    │ (VSCode)    │              │  │
│  │  └─────────────┘    └─────────────┘    └─────────────┘              │  │
│  └─────────────────────────────────────────────────────────────────────┘  │
│                              │                                              │
│         ┌────────────────────┼────────────────────┐                       │
│         │                    │                    │                       │
│         ▼                    ▼                    ▼                       │
│  ┌─────────────┐    ┌─────────────┐    ┌─────────────┐                   │
│  │   Chart仓库  │   │   模板引擎   │   │   配置管理   │                   │
│  │ (Repository)│   │ (Template)  │   │ (Values)    │                   │
│  │   管理      │   │   渲染      │   │   注入      │                   │
│  └─────────────┘    └─────────────┘    └─────────────┘                   │
│         │                    │                    │                       │
│         ▼                    ▼                    ▼                       │
│  ┌─────────────────────────────────────────────────────────────────────┐  │
│  │                      Release管理层                                  │  │
│  │  ┌───────────────────────────────────────────────────────────────┐  │  │
│  │  │                     Tiller/KubeAPI                            │  │  │
│  │  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐           │  │  │
│  │  │  │   安装      │  │   升级      │  │   回滚      │           │  │  │
│  │  │  │ (Install)   │  │ (Upgrade)   │  │ (Rollback)  │           │  │  │
│  │  │  └─────────────┘  └─────────────┘  └─────────────┘           │  │  │
│  │  └───────────────────────────────────────────────────────────────┘  │  │
│  └─────────────────────────────────────────────────────────────────────┘  │
│                              │                                              │
│         ┌────────────────────┼────────────────────┐                       │
│         │                    │                    │                       │
│         ▼                    ▼                    ▼                       │
│  ┌─────────────┐    ┌─────────────┐    ┌─────────────┐                   │
│  │   Kubernetes│   │   存储后端   │   │   状态跟踪   │                   │
│  │   API交互   │   │ (Secret)    │   │  (History)  │                   │
│  │   资源创建   │   │   状态      │   │   版本      │                   │
│  └─────────────┘    └─────────────┘    └─────────────┘                   │
│                              │                                              │
│                              ▼                                              │
│  ┌─────────────────────────────────────────────────────────────────────┐  │
│  │                      应用运行层                                     │  │
│  │  ┌─────────────┐    ┌─────────────┐    ┌─────────────┐              │  │
│  │  │   Pod资源   │    │   Service   │    │   Config    │              │  │
│  │  │  (运行时)   │    │  (网络)     │    │  (配置)     │              │  │
│  │  └─────────────┘    └─────────────┘    └─────────────┘              │  │
│  └─────────────────────────────────────────────────────────────────────┘  │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

---

## 2. Chart渲染和模板问题排查 (Chart Rendering and Template Issues)

### 2.1 模板语法错误诊断

```bash
# ========== 1. 模板渲染测试 ==========
# 语法检查
helm lint <chart-path>

# 模板渲染预览（不安装）
helm template <release-name> <chart-path> --debug

# 验证values文件
helm template <release-name> <chart-path> -f values.yaml --debug

# ========== 2. 模板函数调试 ==========
# 启用详细调试输出
helm template <release-name> <chart-path> --debug --dry-run

# 检查模板函数使用
grep -r "{{" <chart-path>/templates/ | head -10

# 验证内置函数使用
helm template <release-name> <chart-path> --set debug=true

# ========== 3. 常见模板错误分析 ==========
# 变量作用域问题
helm template test-release ./mychart --set 'key1=value1,key2=value2' --debug

# 条件判断错误
helm template test-release ./mychart --set condition=true --debug

# 循环遍历问题
helm template test-release ./mychart --set 'items={item1,item2,item3}' --debug
```

### 2.2 Values配置验证

```bash
# ========== 配置结构验证 ==========
# 检查values.yaml结构
helm show values <chart-name>

# 验证自定义values
helm template <release-name> <chart-path> -f custom-values.yaml --debug

# 比较默认值和自定义值
helm show values <chart-name> > default-values.yaml
diff default-values.yaml custom-values.yaml

# ========== 参数验证工具 ==========
# 创建values验证脚本
cat <<'EOF' > values-validator.sh
#!/bin/bash

CHART_PATH=$1
VALUES_FILE=$2

if [ -z "$CHART_PATH" ]; then
    echo "Usage: $0 <chart-path> [values-file]"
    exit 1
fi

echo "Validating chart: $CHART_PATH"

# 1. 基础语法检查
echo "1. Running helm lint..."
helm lint $CHART_PATH

# 2. 模板渲染测试
echo "2. Testing template rendering..."
if [ -n "$VALUES_FILE" ] && [ -f "$VALUES_FILE" ]; then
    helm template test-release $CHART_PATH -f $VALUES_FILE --debug
else
    helm template test-release $CHART_PATH --debug
fi

# 3. 结构验证
echo "3. Validating values structure..."
if [ -f "$CHART_PATH/values.yaml" ]; then
    yamllint $CHART_PATH/values.yaml
fi

# 4. 依赖检查
echo "4. Checking dependencies..."
if [ -f "$CHART_PATH/requirements.yaml" ]; then
    helm dependency list $CHART_PATH
fi

echo "Validation completed"
EOF

chmod +x values-validator.sh

# ========== Schema验证 ==========
# 创建values.schema.json验证
cat <<'EOF' > schema-validator.sh
#!/bin/bash

CHART_PATH=$1

if [ -z "$CHART_PATH" ]; then
    echo "Usage: $0 <chart-path>"
    exit 1
fi

# 检查是否存在schema文件
if [ -f "$CHART_PATH/values.schema.json" ]; then
    echo "Found values.schema.json, validating..."
    
    # 使用ajv进行JSON Schema验证
    npm install -g ajv-cli
    
    # 验证values.yaml符合schema
    ajv validate -s $CHART_PATH/values.schema.json -d $CHART_PATH/values.yaml
    
    # 验证自定义values
    for values_file in $CHART_PATH/ci/*.yaml; do
        if [ -f "$values_file" ]; then
            echo "Validating $values_file..."
            ajv validate -s $CHART_PATH/values.schema.json -d $values_file
        fi
    done
else
    echo "No values.schema.json found"
fi
EOF

chmod +x schema-validator.sh
```

---

## 3. 依赖管理问题排查 (Dependency Management Issues)

### 3.1 依赖下载和解析

```bash
# ========== 1. 依赖状态检查 ==========
# 查看依赖列表
helm dependency list <chart-path>

# 更新依赖
helm dependency update <chart-path>

# 构建依赖
helm dependency build <chart-path>

# 验证依赖完整性
helm dependency list <chart-path> | grep -E "(missing|unpacked)"

# ========== 2. 依赖仓库配置 ==========
# 查看仓库配置
helm repo list

# 更新仓库索引
helm repo update

# 添加私有仓库
helm repo add private-repo https://charts.example.com --username <user> --password <pass>

# 验证仓库连通性
helm search repo <repo-name>

# ========== 3. 依赖版本冲突解决 ==========
# 分析依赖树
helm dependency list <chart-path> --debug

# 检查版本兼容性
helm show chart <dependency-chart> | grep version

# 强制更新特定依赖
helm dependency update <chart-path> --skip-refresh

# ========== 依赖缓存管理 ==========
# 清理依赖缓存
rm -rf ~/.cache/helm/repository/*.yaml
rm -rf ~/.helm/cache/archive/*

# 重新下载依赖
helm dependency update <chart-path>

# 验证缓存完整性
helm dependency list <chart-path> | while read line; do
    chart_name=$(echo $line | awk '{print $1}')
    chart_version=$(echo $line | awk '{print $2}')
    if [ "$chart_version" != "0.0.0" ]; then
        echo "Checking $chart_name version $chart_version"
        helm show chart $chart_name --version $chart_version
    fi
done
```

### 3.2 私有仓库和认证问题

```bash
# ========== 私有仓库配置 ==========
# 配置OCI仓库
helm registry login registry.example.com \
    --username <username> \
    --password <password>

# 推送Chart到OCI仓库
helm package <chart-path>
helm push <chart-package>.tgz oci://registry.example.com/charts

# 从OCI仓库拉取
helm pull oci://registry.example.com/charts/<chart-name> --version <version>

# ========== 认证问题诊断 ==========
# 检查认证配置
cat ~/.config/helm/registry/config.json

# 验证Token有效性
helm registry logout registry.example.com
helm registry login registry.example.com --username <user> --password <pass>

# 测试仓库访问
curl -u <user>:<pass> https://registry.example.com/v2/_catalog

# ========== 企业级认证集成 ==========
# 配置基于证书的认证
cat <<EOF > registry-config.yaml
apiVersion: v1
kind: Config
auths:
  registry.example.com:
    auth: <base64-encoded-credentials>
    email: user@example.com
EOF

# 配置代理认证
export HTTP_PROXY=http://proxy.example.com:8080
export HTTPS_PROXY=https://proxy.example.com:8080
export NO_PROXY=localhost,127.0.0.1,.example.com
```

---

## 4. Release状态异常排查 (Release Status Issues)

### 4.1 Release状态检查

```bash
# ========== 1. Release状态分析 ==========
# 查看所有Release状态
helm list --all-namespaces

# 检查特定Release详细信息
helm status <release-name> -n <namespace>

# 查看Release历史
helm history <release-name> -n <namespace>

# 分析Release事件
kubectl get events -n <namespace> --field-selector involvedObject.name=<release-name>

# ========== 2. 状态异常诊断 ==========
# 检查卡住的Release
helm list --failed --pending

# 分析失败原因
helm status <release-name> -n <namespace> --debug

# 查看Kubernetes资源状态
kubectl get all -n <namespace> -l app.kubernetes.io/instance=<release-name>

# 验证资源创建状态
kubectl describe deployment -n <namespace> -l app.kubernetes.io/instance=<release-name>

# ========== 3. Release恢复操作 ==========
# 强制更新Release状态
helm upgrade <release-name> <chart-path> -n <namespace> --force

# 回滚到上一个版本
helm rollback <release-name> -n <namespace>

# 删除并重新安装
helm uninstall <release-name> -n <namespace>
helm install <release-name> <chart-path> -n <namespace>
```

### 4.2 资源冲突和清理

```bash
# ========== 资源冲突检测 ==========
# 检查名称冲突
kubectl get all -A | grep <release-name>

# 分析标签选择器冲突
kubectl get deployments -A -o jsonpath='{
    range .items[*]
}{
        .metadata.name
}{
        "\t"
}{
        .metadata.labels."app\.kubernetes\.io/instance"
}{
        "\n"
}{
    end
}' | grep <release-name>

# 验证命名空间冲突
kubectl get namespaces | grep <target-namespace>

# ========== 清理残留资源 ==========
# 创建清理脚本
cat <<'EOF' > release-cleaner.sh
#!/bin/bash

RELEASE_NAME=$1
NAMESPACE=${2:-default}

if [ -z "$RELEASE_NAME" ]; then
    echo "Usage: $0 <release-name> [namespace]"
    exit 1
fi

echo "Cleaning release: $RELEASE_NAME in namespace: $NAMESPACE"

# 1. 删除Helm Release
helm uninstall $RELEASE_NAME -n $NAMESPACE --no-hooks

# 2. 清理残留资源
RESOURCES=("deployments" "services" "configmaps" "secrets" "pods" "replicasets" "daemonsets" "statefulsets" "jobs" "cronjobs")

for resource in "${RESOURCES[@]}"; do
    echo "Cleaning $resource..."
    kubectl delete $resource -n $NAMESPACE -l app.kubernetes.io/instance=$RELEASE_NAME --ignore-not-found
done

# 3. 清理PVC（谨慎操作）
echo "PVCs found for this release:"
kubectl get pvc -n $NAMESPACE -l app.kubernetes.io/instance=$RELEASE_NAME
read -p "Delete PVCs? (y/N): " -n 1 -r
echo
if [[ $REPLY =~ ^[Yy]$ ]]; then
    kubectl delete pvc -n $NAMESPACE -l app.kubernetes.io/instance=$RELEASE_NAME
fi

# 4. 验证清理结果
echo "Remaining resources:"
kubectl get all -n $NAMESPACE -l app.kubernetes.io/instance=$RELEASE_NAME

echo "Cleanup completed for release: $RELEASE_NAME"
EOF

chmod +x release-cleaner.sh

# ========== 手动状态修复 ==========
# 修复Release状态
cat <<'EOF' > release-state-fixer.sh
#!/bin/bash

RELEASE_NAME=$1
NAMESPACE=${2:-default}

if [ -z "$RELEASE_NAME" ]; then
    echo "Usage: $0 <release-name> [namespace]"
    exit 1
fi

echo "Fixing release state: $RELEASE_NAME"

# 1. 检查当前状态
CURRENT_STATUS=$(helm status $RELEASE_NAME -n $NAMESPACE -o jsonpath='{.info.status}')
echo "Current status: $CURRENT_STATUS"

# 2. 如果是失败状态，尝试修复
if [ "$CURRENT_STATUS" = "failed" ]; then
    echo "Release is in failed state, attempting repair..."
    
    # 检查具体错误
    helm status $RELEASE_NAME -n $NAMESPACE
    
    # 尝试升级修复
    helm upgrade $RELEASE_NAME <chart-path> -n $NAMESPACE --reuse-values
    
    # 或者回滚到最后一个成功版本
    LAST_SUCCESS_VERSION=$(helm history $RELEASE_NAME -n $NAMESPACE | grep deployed | tail -1 | awk '{print $1}')
    if [ -n "$LAST_SUCCESS_VERSION" ]; then
        echo "Rolling back to version $LAST_SUCCESS_VERSION"
        helm rollback $RELEASE_NAME $LAST_SUCCESS_VERSION -n $NAMESPACE
    fi
fi

# 3. 验证修复结果
helm status $RELEASE_NAME -n $NAMESPACE
EOF

chmod +x release-state-fixer.sh
```

---

## 5. 版本兼容性问题排查 (Version Compatibility Issues)

### 5.1 Kubernetes API版本兼容性

```bash
# ========== API版本检查 ==========
# 检查集群版本
kubectl version --short

# 验证Chart支持的Kubernetes版本
helm show chart <chart-name> | grep -E "(kubeVersion|apiVersions)"

# 检查API资源可用性
kubectl api-resources | grep -E "(apps|batch|networking)"

# 验证CRD存在性
kubectl get crds | grep <required-crd>

# ========== 版本兼容性测试 ==========
# 创建兼容性测试脚本
cat <<'EOF' > compatibility-tester.sh
#!/bin/bash

CHART_PATH=$1
KUBE_VERSION=${2:-$(kubectl version --short | grep Server | awk '{print $3}')}

if [ -z "$CHART_PATH" ]; then
    echo "Usage: $0 <chart-path> [kube-version]"
    exit 1
fi

echo "Testing compatibility for Kubernetes version: $KUBE_VERSION"

# 1. 检查Chart要求的Kubernetes版本
REQUIRED_VERSION=$(helm show chart $CHART_PATH | grep kubeVersion | cut -d: -f2 | tr -d ' ')
if [ -n "$REQUIRED_VERSION" ]; then
    echo "Chart requires Kubernetes: $REQUIRED_VERSION"
    
    # 简单的版本比较（实际应该使用语义化版本比较）
    if [[ "$KUBE_VERSION" < "$REQUIRED_VERSION" ]]; then
        echo "❌ Kubernetes version $KUBE_VERSION is lower than required $REQUIRED_VERSION"
        exit 1
    else
        echo "✓ Kubernetes version is compatible"
    fi
fi

# 2. 检查API版本支持
API_VERSIONS=$(helm show chart $CHART_PATH | grep apiVersions | cut -d: -f2 | tr -d ' []"' | tr ',' '\n')

for api_version in $API_VERSIONS; do
    echo "Checking API version: $api_version"
    
    # 提取GVK
    GROUP=$(echo $api_version | cut -d/ -f1)
    VERSION=$(echo $api_version | cut -d/ -f2 | cut -d. -f1)
    KIND=$(echo $api_version | cut -d. -f2)
    
    # 检查API资源是否存在
    if kubectl api-resources | grep -q "$KIND.*$GROUP.*$VERSION"; then
        echo "  ✓ Supported"
    else
        echo "  ❌ Not supported"
    fi
done

# 3. 模板渲染测试
echo "Testing template rendering..."
helm template test-release $CHART_PATH --kube-version $KUBE_VERSION --debug

echo "Compatibility test completed"
EOF

chmod +x compatibility-tester.sh

# ========== API废弃警告检查 ==========
# 检查废弃API使用
helm template <release-name> <chart-path> --debug |& grep -i "deprecated\|deprecation"

# 使用kubeval验证
kubeval --kubernetes-version $(kubectl version --short | grep Server | awk '{print $3}' | tr -d 'v') <chart-path>/templates/*.yaml
```

### 5.2 Helm版本兼容性

```bash
# ========== Helm客户端版本检查 ==========
# 检查Helm版本
helm version

# 验证Chart兼容的Helm版本
helm show chart <chart-name> | grep -E "(appVersion|kubeVersion|tillerVersion)"

# 检查插件兼容性
helm plugin list

# ========== 版本迁移检查 ==========
# Helm 2到3迁移状态检查
helm list -a  # Helm 2
helm list -A  # Helm 3

# 检查Release存储后端
kubectl get secrets -n <namespace> | grep sh.helm.release

# 验证迁移完整性
helm 2to3 convert <release-name> --dry-run

# ========== 向后兼容性测试 ==========
# 测试不同Helm版本的兼容性
for version in "3.8.0" "3.9.0" "3.10.0"; do
    echo "Testing with Helm $version"
    # 这里可以添加具体的测试逻辑
done
```

---

## 6. 监控和调试工具 (Monitoring and Debugging Tools)

### 6.1 调试工具配置

```bash
# ========== 调试模式启用 ==========
# 启用详细调试输出
helm install <release-name> <chart-path> --debug --dry-run

# 设置日志级别
export HELM_DEBUG=true
helm install <release-name> <chart-path>

# 启用Tiller调试（Helm 2）
helm init --upgrade --tiller-debug

# ========== 调试信息收集 ==========
# 创建调试信息收集脚本
cat <<'EOF' > helm-debug-collector.sh
#!/bin/bash

RELEASE_NAME=$1
NAMESPACE=${2:-default}

if [ -z "$RELEASE_NAME" ]; then
    echo "Usage: $0 <release-name> [namespace]"
    exit 1
fi

DEBUG_DIR="/tmp/helm-debug-$(date +%Y%m%d-%H%M%S)"
mkdir -p $DEBUG_DIR

echo "Collecting debug information for release: $RELEASE_NAME"
echo "Debug files will be saved to: $DEBUG_DIR"

# 1. Release基本信息
echo "=== Release Info ===" > $DEBUG_DIR/release-info.txt
helm status $RELEASE_NAME -n $NAMESPACE >> $DEBUG_DIR/release-info.txt

# 2. Release历史
echo "=== Release History ===" > $DEBUG_DIR/history.txt
helm history $RELEASE_NAME -n $NAMESPACE >> $DEBUG_DIR/history.txt

# 3. 模板渲染输出
echo "=== Template Output ===" > $DEBUG_DIR/template-output.yaml
helm template $RELEASE_NAME <chart-path> -n $NAMESPACE --debug > $DEBUG_DIR/template-output.yaml 2>&1

# 4. 实际创建的资源
echo "=== Created Resources ===" > $DEBUG_DIR/created-resources.txt
kubectl get all -n $NAMESPACE -l app.kubernetes.io/instance=$RELEASE_NAME -o yaml > $DEBUG_DIR/resources.yaml
kubectl describe all -n $NAMESPACE -l app.kubernetes.io/instance=$RELEASE_NAME > $DEBUG_DIR/created-resources.txt

# 5. Pod日志
echo "=== Pod Logs ===" > $DEBUG_DIR/pod-logs.txt
for pod in $(kubectl get pods -n $NAMESPACE -l app.kubernetes.io/instance=$RELEASE_NAME -o jsonpath='{.items[*].metadata.name}'); do
    echo "=== Logs for $pod ===" >> $DEBUG_DIR/pod-logs.txt
    kubectl logs $pod -n $NAMESPACE >> $DEBUG_DIR/pod-logs.txt 2>&1
    echo "" >> $DEBUG_DIR/pod-logs.txt
done

# 6. 事件信息
echo "=== Events ===" > $DEBUG_DIR/events.txt
kubectl get events -n $NAMESPACE --field-selector involvedObject.name=$RELEASE_NAME >> $DEBUG_DIR/events.txt

# 7. 系统信息
echo "=== System Info ===" > $DEBUG_DIR/system-info.txt
echo "Helm version:" >> $DEBUG_DIR/system-info.txt
helm version >> $DEBUG_DIR/system-info.txt
echo "" >> $DEBUG_DIR/system-info.txt
echo "Kubernetes version:" >> $DEBUG_DIR/system-info.txt
kubectl version >> $DEBUG_DIR/system-info.txt

# 创建压缩包
tar -czf $DEBUG_DIR.tar.gz -C /tmp $(basename $DEBUG_DIR)
echo "Debug package created: $DEBUG_DIR.tar.gz"

rm -rf $DEBUG_DIR
EOF

chmod +x helm-debug-collector.sh
```

### 6.2 监控告警配置

```bash
# ========== Helm监控集成 ==========
# 创建Helm监控ServiceMonitor
cat <<EOF | kubectl apply -f -
apiVersion: monitoring.coreos.com/v1
kind: ServiceMonitor
metadata:
  name: helm-monitor
  namespace: monitoring
spec:
  selector:
    matchLabels:
      app: helm-exporter
  endpoints:
  - port: metrics
    path: /metrics
    interval: 30s
---
apiVersion: apps/v1
kind: Deployment
metadata:
  name: helm-exporter
  namespace: monitoring
spec:
  replicas: 1
  selector:
    matchLabels:
      app: helm-exporter
  template:
    metadata:
      labels:
        app: helm-exporter
    spec:
      containers:
      - name: exporter
        image: kubevious/helm-exporter:latest
        ports:
        - containerPort: 9571
        env:
        - name: HELM_EXPORTER_NAMESPACE
          value: "default"
        - name: HELM_EXPORTER_TILLER_NAMESPACE
          value: "kube-system"
EOF

# ========== 告警规则配置 ==========
cat <<EOF | kubectl apply -f -
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: helm-alerts
  namespace: monitoring
spec:
  groups:
  - name: helm.rules
    rules:
    - alert: HelmReleaseFailed
      expr: helm_release_status{status="failed"} == 1
      for: 5m
      labels:
        severity: critical
      annotations:
        summary: "Helm release {{ \$labels.name }} in namespace {{ \$labels.namespace }} has failed"
        
    - alert: HelmReleasePending
      expr: helm_release_status{status="pending-*"} == 1
      for: 10m
      labels:
        severity: warning
      annotations:
        summary: "Helm release {{ \$labels.name }} in namespace {{ \$labels.namespace }} is stuck in pending state"
        
    - alert: HelmChartVersionMismatch
      expr: count(count by (chart, version) (helm_release_info)) > 1
      for: 5m
      labels:
        severity: warning
      annotations:
        summary: "Multiple versions of chart {{ \$labels.chart }} detected across releases"
        
    - alert: HelmRepositoryUnreachable
      expr: helm_repo_status{status="unreachable"} == 1
      for: 2m
      labels:
        severity: critical
      annotations:
        summary: "Helm repository {{ \$labels.name }} is unreachable"
EOF

# ========== 自定义监控面板 ==========
# 创建Grafana仪表板配置
cat <<'EOF' > helm-dashboard.json
{
  "dashboard": {
    "title": "Helm Releases Overview",
    "panels": [
      {
        "title": "Release Status Summary",
        "type": "stat",
        "targets": [
          {
            "expr": "count(helm_release_status{status=\"deployed\"})",
            "legendFormat": "Deployed"
          },
          {
            "expr": "count(helm_release_status{status=\"failed\"})",
            "legendFormat": "Failed"
          },
          {
            "expr": "count(helm_release_status{status=\"pending-*\"})",
            "legendFormat": "Pending"
          }
        ]
      },
      {
        "title": "Release Status Over Time",
        "type": "graph",
        "targets": [
          {
            "expr": "helm_release_status",
            "legendFormat": "{{namespace}}/{{name}} - {{status}}"
          }
        ]
      },
      {
        "title": "Chart Version Distribution",
        "type": "piechart",
        "targets": [
          {
            "expr": "count by (chart, version) (helm_release_info)",
            "legendFormat": "{{chart}} v{{version}}"
          }
        ]
      }
    ]
  }
}
EOF
```

---

## 7. 最佳实践和预防措施 (Best Practices and Prevention)

### 7.1 Chart开发最佳实践

```bash
# ========== Chart结构验证 ==========
# 创建Chart验证脚本
cat <<'EOF' > chart-validator.sh
#!/bin/bash

CHART_PATH=$1

if [ -z "$CHART_PATH" ]; then
    echo "Usage: $0 <chart-path>"
    exit 1
fi

echo "Validating chart: $CHART_PATH"

# 1. 基本结构检查
REQUIRED_FILES=("Chart.yaml" "values.yaml" "templates/")
for file in "${REQUIRED_FILES[@]}"; do
    if [ ! -e "$CHART_PATH/$file" ]; then
        echo "❌ Missing required file: $file"
        exit 1
    else
        echo "✓ Found required file: $file"
    fi
done

# 2. Chart.yaml验证
echo "Validating Chart.yaml..."
helm lint $CHART_PATH | grep -E "(Error|Warning)"

# 3. 模板语法检查
echo "Checking template syntax..."
find $CHART_PATH/templates -name "*.yaml" -exec yamllint {} \;

# 4. 最佳实践检查
echo "Checking best practices..."

# 检查是否有NOTES.txt
if [ ! -f "$CHART_PATH/templates/NOTES.txt" ]; then
    echo "⚠️  Missing NOTES.txt template"
fi

# 检查是否有_helpers.tpl
if [ ! -f "$CHART_PATH/templates/_helpers.tpl" ]; then
    echo "⚠️  Missing _helpers.tpl template"
fi

# 检查资源标签
grep -r "app.kubernetes.io/" $CHART_PATH/templates/ > /dev/null || echo "⚠️  Missing standard labels"

# 检查命名空间使用
grep -r "{{ .Release.Namespace }}" $CHART_PATH/templates/ > /dev/null || echo "⚠️  Hardcoded namespace detected"

echo "Validation completed"
EOF

chmod +x chart-validator.sh

# ========== CI/CD集成配置 ==========
# 创建Helm CI配置
cat <<'EOF' > .gitlab-ci.yml
stages:
  - lint
  - test
  - release

variables:
  HELM_VERSION: "3.10.0"

lint-chart:
  stage: lint
  image: 
    name: alpine/helm:$HELM_VERSION
    entrypoint: [""]
  script:
    - helm lint .
    - helm template test-release . --debug
  only:
    - merge_requests
    - master

test-chart:
  stage: test
  image: 
    name: alpine/helm:$HELM_VERSION
    entrypoint: [""]
  services:
    - name: rancher/k3s:v1.25.0-k3s1
      alias: k3s
  script:
    - helm install test-release . --wait --timeout 300s
    - helm test test-release
    - helm uninstall test-release
  only:
    - merge_requests
    - master

release-chart:
  stage: release
  image: 
    name: alpine/helm:$HELM_VERSION
    entrypoint: [""]
  script:
    - helm package .
    - helm push *.tgz oci://$CI_REGISTRY_PROJECT/helm-charts
  only:
    - tags
EOF
```

### 7.2 生产环境部署策略

```bash
# ========== 蓝绿部署配置 ==========
# 创建蓝绿部署values
cat <<EOF > blue-green-values.yaml
# Blue-Green Deployment Configuration
blueGreen:
  enabled: true
  # 蓝色环境配置
  blue:
    replicaCount: 3
    image:
      tag: v1.0.0
    service:
      name: myapp-blue
  # 绿色环境配置  
  green:
    replicaCount: 3
    image:
      tag: v1.1.0
    service:
      name: myapp-green

# 流量切换配置
traffic:
  blueWeight: 100
  greenWeight: 0
EOF

# ========== 金丝雀发布策略 ==========
# 创建金丝雀部署配置
cat <<EOF > canary-values.yaml
# Canary Deployment Configuration
canary:
  enabled: true
  # 金丝雀版本配置
  image:
    tag: v1.1.0-canary
  replicaCount: 1
  # 原版本配置
  stable:
    replicaCount: 3
    image:
      tag: v1.0.0

# 流量分割配置
traffic:
  canaryWeight: 10
  stableWeight: 90
EOF

# ========== 回滚自动化 ==========
# 创建自动回滚脚本
cat <<'EOF' > auto-rollback.sh
#!/bin/bash

RELEASE_NAME=$1
NAMESPACE=${2:-default}
MAX_FAILURES=${3:-3}

if [ -z "$RELEASE_NAME" ]; then
    echo "Usage: $0 <release-name> [namespace] [max-failures]"
    exit 1
fi

echo "Setting up automatic rollback for release: $RELEASE_NAME"

FAILURE_COUNT=0

while true; do
    # 检查Release状态
    STATUS=$(helm status $RELEASE_NAME -n $NAMESPACE -o jsonpath='{.info.status}')
    
    if [ "$STATUS" = "failed" ]; then
        FAILURE_COUNT=$((FAILURE_COUNT + 1))
        echo "$(date): Release failed (attempt $FAILURE_COUNT/$MAX_FAILURES)"
        
        if [ $FAILURE_COUNT -ge $MAX_FAILURES ]; then
            echo "$(date): Maximum failures reached, initiating rollback"
            
            # 回滚到上一个版本
            helm rollback $RELEASE_NAME -n $NAMESPACE
            
            # 验证回滚结果
            NEW_STATUS=$(helm status $RELEASE_NAME -n $NAMESPACE -o jsonpath='{.info.status}')
            if [ "$NEW_STATUS" = "deployed" ]; then
                echo "$(date): Rollback successful"
            else
                echo "$(date): Rollback failed"
            fi
            
            break
        fi
    elif [ "$STATUS" = "deployed" ]; then
        FAILURE_COUNT=0
        echo "$(date): Release is healthy"
    fi
    
    sleep 60
done
EOF

chmod +x auto-rollback.sh
```

---