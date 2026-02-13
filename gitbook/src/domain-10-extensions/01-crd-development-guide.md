# 01 - CRD自定义资源定义开发指南

> **适用版本**: v1.25 - v1.32 | **最后更新**: 2026-02 | **参考**: [kubernetes.io/docs/concepts/extend-kubernetes/api-extension/custom-resources/](https://kubernetes.io/docs/concepts/extend-kubernetes/api-extension/custom-resources/)

## CRD核心概念与架构

### CRD vs API Extension对比

| 特性 | CRD (CustomResourceDefinition) | API Aggregation |
|-----|-------------------------------|----------------|
| **复杂度** | 简单，声明式 | 复杂，需要编程 |
| **存储** | etcd内置 | 自定义存储 |
| **验证** | OpenAPI v3 Schema | 自定义验证逻辑 |
| **转换** | 版本转换支持 | 完全自定义 |
| **适用场景** | 简单资源扩展 | 复杂业务逻辑 |

### CRD版本演化历程

```
v1.7  ──▶  v1.16  ──▶  v1.22  ──▶  v1.25+
 │          │          │          │
CRD v1beta1  CRD v1    结构化    结构化+默认值
(已废弃)    (稳定)    融合       融合+验证
```

## CRD开发完整流程

### 1. CRD定义规范

```yaml
# crd-example.yaml
apiVersion: apiextensions.k8s.io/v1
kind: CustomResourceDefinition
metadata:
  # 名称格式: plural.group.domain
  name: mysqlclusters.database.example.com
spec:
  # 组名 - 通常使用反向域名
  group: database.example.com
  
  # 版本列表
  versions:
  - name: v1beta1
    # 是否作为存储版本
    storage: false
    # 是否提供服务
    served: true
    # OpenAPI v3 schema验证
    schema:
      openAPIV3Schema:
        type: object
        properties:
          spec:
            type: object
            properties:
              replicas:
                type: integer
                minimum: 1
                maximum: 10
                default: 1
              version:
                type: string
                enum:
                - "5.7"
                - "8.0"
                default: "8.0"
              storage:
                type: object
                properties:
                  size:
                    type: string
                    pattern: "^[0-9]+Gi$"
                  class:
                    type: string
                required: ["size"]
            required: ["replicas", "storage"]
          status:
            type: object
            properties:
              phase:
                type: string
                enum:
                - Pending
                - Creating
                - Running
                - Failed
              replicas:
                type: integer
              conditions:
                type: array
                items:
                  type: object
                  properties:
                    type:
                      type: string
                    status:
                      type: string
                      enum: ["True", "False", "Unknown"]
                    reason:
                      type: string
                    message:
                      type: string
                    lastTransitionTime:
                      type: string
                      format: date-time
    
    # 子资源支持
    subresources:
      # 支持kubectl scale
      scale:
        specReplicasPath: .spec.replicas
        statusReplicasPath: .status.replicas
        labelSelectorPath: .status.labelSelector
      # 支持kubectl status
      status: {}
    
    # 打印列定义 (kubectl get显示)
    additionalPrinterColumns:
    - name: Replicas
      type: integer
      description: Number of replicas
      jsonPath: .spec.replicas
    - name: Status
      type: string
      description: Cluster status
      jsonPath: .status.phase
    - name: Age
      type: date
      jsonPath: .metadata.creationTimestamp
    
    # 版本转换策略
    conversion:
      strategy: None  # 或Webhook
  
  # 作用域: Namespaced或Cluster
  scope: Namespaced
  
  # 名称定义
  names:
    # 复数形式
    plural: mysqlclusters
    # 单数形式
    singular: mysqlcluster
    # Kind名称
    kind: MySQLCluster
    # 简短名称 (kubectl get mc)
    shortNames:
    - mc
    - mysql
    # 列表Kind
    listKind: MySQLClusterList
```

### 2. CR实例示例

```yaml
# mysql-cluster-example.yaml
apiVersion: database.example.com/v1beta1
kind: MySQLCluster
metadata:
  name: my-cluster
  namespace: default
spec:
  replicas: 3
  version: "8.0"
  storage:
    size: "100Gi"
    class: "fast-ssd"
  # 可选配置
  backup:
    enabled: true
    schedule: "0 2 * * *"
    retention: "7d"
status:
  phase: Pending
  replicas: 0
  conditions:
  - type: Available
    status: "False"
    reason: "Creating"
    message: "MySQL cluster is being created"
    lastTransitionTime: "2024-01-01T10:00:00Z"
```

## 高级CRD特性

### 1. 默认值与枚举

```yaml
# 高级schema特性
schema:
  openAPIV3Schema:
    type: object
    properties:
      spec:
        type: object
        properties:
          # 默认值
          logLevel:
            type: string
            default: "INFO"
            enum: ["DEBUG", "INFO", "WARN", "ERROR"]
          
          # 数组验证
          whitelist:
            type: array
            items:
              type: string
              pattern: "^\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}$"
            maxItems: 100
          
          # 对象验证
          resources:
            type: object
            properties:
              limits:
                type: object
                properties:
                  cpu:
                    type: string
                    pattern: "^[0-9]+(m|)$"
                  memory:
                    type: string
                    pattern: "^[0-9]+(Mi|Gi)$"
                required: ["cpu", "memory"]
            required: ["limits"]
          
          # 条件验证 (oneOf/anyOf/allOf)
          config:
            oneOf:
            - required: ["file"]
            - required: ["inline"]
```

### 2. 版本转换配置

```yaml
# 多版本CRD
versions:
- name: v1alpha1
  storage: false
  served: true
- name: v1beta1
  storage: true
  served: true
  # 版本转换配置
  conversion:
    strategy: Webhook
    webhook:
      clientConfig:
        service:
          namespace: system
          name: webhook-service
          path: /convert
      conversionReviewVersions: ["v1", "v1beta1"]
```

### 7. 高级验证与默认值

```yaml
# 高级OpenAPI v3 Schema配置
schema:
  openAPIV3Schema:
    type: object
    properties:
      spec:
        type: object
        # 服务端默认值（Kubernetes 1.25+）
        default:
          replicas: 1
          version: "8.0"
          storage:
            size: "10Gi"
            class: "standard"
        properties:
          replicas:
            type: integer
            minimum: 1
            maximum: 100
            # 客户端默认值
            default: 1
            # 自定义验证规则
            x-kubernetes-validations:
            - rule: "self >= 1 and self <= 100"
              message: "副本数必须在1-100之间"
              
          version:
            type: string
            # 枚举值验证
            enum: ["5.7", "8.0", "8.1"]
            default: "8.0"
            
          storage:
            type: object
            required: ["size"]
            properties:
              size:
                type: string
                # 正则表达式验证
                pattern: "^[0-9]+(Gi|Mi|Ti)$"
                default: "10Gi"
              class:
                type: string
                default: "standard"
                
      # 状态字段配置
      status:
        type: object
        # 保留未知字段（用于控制器写入额外信息）
        x-kubernetes-preserve-unknown-fields: true
        properties:
          phase:
            type: string
            enum: ["Pending", "Creating", "Running", "Failed", "Terminating"]
          replicas:
            type: integer
          conditions:
            type: array
            items:
              type: object
              properties:
                type:
                  type: string
                status:
                  type: string
                  enum: ["True", "False", "Unknown"]
                reason:
                  type: string
                message:
                  type: string
                lastTransitionTime:
                  type: string
                  format: date-time
```

### 8. 版本转换与迁移策略

```yaml
# Webhook版本转换配置
conversion:
  strategy: Webhook
  webhook:
    conversionReviewVersions: ["v1", "v1beta1"]
    clientConfig:
      service:
        namespace: mysql-operator-system
        name: mysql-conversion-webhook
        path: /convert
        port: 443
      caBundle: LS0tLS1CRUdJTiBDRVJUSUZJQ0FURS0tLS0tCk1JSUM3VENDQ...

---
# 转换Webhook实现示例 (Go)
apiVersion: admissionregistration.k8s.io/v1
kind: ConversionReview
metadata:
  name: mysql-conversion-webhook
spec:
  conversions:
  - source:
      apiVersion: database.example.com/v1beta1
      kind: MySQLCluster
      # v1beta1对象数据
    target:
      apiVersion: database.example.com/v1
      kind: MySQLCluster
      # 转换后的v1对象数据
```

### 9. 生产环境部署策略

```bash
#!/bin/bash
# production-crd-deployment.sh

set -euo pipefail

CRD_NAME="mysqlclusters.database.example.com"
OPERATOR_NAMESPACE="mysql-operator-system"
BACKUP_DIR="/tmp/crd-backups/$(date +%Y%m%d_%H%M%S)"

# 1. 预部署检查
pre_deployment_check() {
    echo "🔍 执行预部署检查..."
    
    # 检查Kubernetes版本兼容性
    kubectl version --short | grep -q "v1.25" || {
        echo "❌ Kubernetes版本不兼容，需要v1.25+"
        exit 1
    }
    
    # 检查CRD是否已存在
    if kubectl get crd ${CRD_NAME} >/dev/null 2>&1; then
        echo "⚠️  CRD已存在，准备升级..."
        UPGRADE_MODE=true
        create_backup
    fi
    
    # 验证CRD配置文件
    kubectl apply --dry-run=client -f config/crd/bases/${CRD_NAME}.yaml -o yaml > /dev/null
    echo "✅ CRD配置验证通过"
}

# 2. 创建备份
create_backup() {
    echo "💾 创建现有CRD备份..."
    mkdir -p ${BACKUP_DIR}
    
    # 备份CRD定义
    kubectl get crd ${CRD_NAME} -o yaml > ${BACKUP_DIR}/crd-definition.yaml
    
    # 备份现有实例
    kubectl get mysqlcluster --all-namespaces -o yaml > ${BACKUP_DIR}/crd-instances.yaml 2>/dev/null || true
    
    echo "✅ 备份完成: ${BACKUP_DIR}"
}

# 3. 安全部署
safe_deploy() {
    echo "🚀 执行安全部署..."
    
    # 分阶段部署
    echo "1/3: 部署CRD定义..."
    kubectl apply -f config/crd/bases/${CRD_NAME}.yaml
    
    # 等待CRD注册完成
    echo "2/3: 等待CRD就绪..."
    until kubectl get crd ${CRD_NAME} >/dev/null 2>&1; do
        echo "等待CRD注册..."
        sleep 2
    done
    
    # 验证部署结果
    echo "3/3: 验证部署结果..."
    kubectl get crd ${CRD_NAME} -o wide
    
    echo "✅ CRD部署成功"
}

# 4. 滚动升级（如果需要）
rolling_upgrade() {
    if [ "${UPGRADE_MODE:-false}" = "true" ]; then
        echo "🔄 执行滚动升级..."
        
        # 逐步更新现有实例
        kubectl get mysqlcluster --all-namespaces -o jsonpath='{range .items[*]}{.metadata.namespace}/{.metadata.name}{"\n"}{end}' | \
        while read ns_name; do
            ns=$(echo $ns_name | cut -d'/' -f1)
            name=$(echo $ns_name | cut -d'/' -f2)
            echo "升级实例: ${ns}/${name}"
            
            # 添加版本标记
            kubectl patch mysqlcluster ${name} -n ${ns} \
                -p '{"metadata":{"annotations":{"crd-version":"v1"}}}' \
                --type=merge
            
            # 验证升级状态
            sleep 5
            kubectl get mysqlcluster ${name} -n ${ns} -o jsonpath='{.status.phase}'
        done
    fi
}

# 5. 健康检查
health_check() {
    echo "🏥 执行健康检查..."
    
    # 检查CRD状态
    kubectl get crd ${CRD_NAME} -o jsonpath='{.status.conditions[?(@.type=="Established")].status}'
    
    # 检查API资源注册
    kubectl api-resources | grep -q mysqlcluster && echo "✅ API资源注册正常"
    
    # 测试CRD功能
    echo "🧪 功能测试..."
    cat <<EOF | kubectl apply -f -
apiVersion: database.example.com/v1
kind: MySQLCluster
metadata:
  name: test-deployment
  namespace: ${OPERATOR_NAMESPACE}
spec:
  replicas: 1
  storage:
    size: "5Gi"
EOF
    
    sleep 10
    kubectl get mysqlcluster test-deployment -n ${OPERATOR_NAMESPACE} && echo "✅ 功能测试通过"
    kubectl delete mysqlcluster test-deployment -n ${OPERATOR_NAMESPACE}
}

# 6. 回滚机制
rollback_if_needed() {
    if [ $? -ne 0 ] && [ -d "${BACKUP_DIR}" ]; then
        echo "❌ 部署失败，执行回滚..."
        kubectl apply -f ${BACKUP_DIR}/crd-definition.yaml
        echo "✅ 回滚完成"
    fi
}

# 主执行流程
main() {
    echo "🚀 开始CRD生产环境部署"
    echo "CRD名称: ${CRD_NAME}"
    echo "操作命名空间: ${OPERATOR_NAMESPACE}"
    echo "备份目录: ${BACKUP_DIR}"
    echo "----------------------------------------"
    
    pre_deployment_check
    safe_deploy
    rolling_upgrade
    health_check
    
    echo "🎉 CRD生产环境部署完成！"
    echo "备份位置: ${BACKUP_DIR}"
}

# 错误处理
trap rollback_if_needed ERR

main "$@"
```

## CRD部署与管理

### 1. 部署脚本

```bash
#!/bin/bash
# deploy-crd.sh

set -e

CRD_FILE="config/crd/bases/database.example.com_mysqlclusters.yaml"
NAMESPACE="mysql-operator-system"

echo "🔍 验证CRD文件..."
kubectl apply --dry-run=client -f ${CRD_FILE} -o yaml > /dev/null
echo "✅ CRD文件语法正确"

echo "🚀 部署CRD..."
kubectl apply -f ${CRD_FILE}

echo "⏳ 等待CRD就绪..."
until kubectl get crd mysqlclusters.database.example.com > /dev/null 2>&1; do
  echo "等待CRD注册..."
  sleep 2
done

echo "📋 验证CRD状态..."
kubectl get crd mysqlclusters.database.example.com -o wide

echo "🧪 测试CRD..."
cat <<EOF | kubectl apply -f -
apiVersion: database.example.com/v1beta1
kind: MySQLCluster
metadata:
  name: test-cluster
spec:
  replicas: 1
  storage:
    size: "10Gi"
EOF

echo "🧹 清理测试资源..."
kubectl delete mysqlcluster test-cluster

echo "🎉 CRD部署完成!"
```

### 2. CRD验证工具

```bash
# 使用kubeval验证
kubeval --strict --ignore-missing-schemas ${CRD_FILE}

# 使用conftest验证策略
conftest test -p policy/crd.rego ${CRD_FILE}

# 使用kubebuilder验证
kubebuilder alpha crd gen --input-dir=config/crd/bases/

# 验证CRD是否存在
kubectl get crd | grep mysqlcluster
```

## CRD生产环境最佳实践

### 1. 命名规范与版本管理

```
# 推荐命名模式
plural.group.domain.com

# 示例
mysqlclusters.database.example.com  ✅
mysql.database.example.com          ❌ (不够明确)
databases.mysql.example.com         ✅

# 版本演化策略
v1alpha1 → v1beta1 → v1 → v2alpha1...
```

### 2. 性能优化配置

```yaml
# CRD性能优化配置
apiVersion: apiextensions.k8s.io/v1
kind: CustomResourceDefinition
metadata:
  name: mysqlclusters.database.example.com
  annotations:
    # 启用结构化模式
    api-approved.kubernetes.io: "https://github.com/kubernetes/enhancements/pull/1602"
spec:
  # 启用服务端默认值
  versions:
  - name: v1
    schema:
      openAPIV3Schema:
        type: object
        properties:
          spec:
            type: object
            default:  # 服务端默认值
              replicas: 1
              version: "8.0"
            properties:
              replicas:
                type: integer
                minimum: 1
                maximum: 100
                default: 1
                # 启用字段验证
                x-kubernetes-validations:
                - rule: "self >= 1 and self <= 100"
                  message: "副本数必须在1-100之间"
                  
    # 启用修剪未知字段
    additionalPrinterColumns:
    - name: Ready
      type: string
      jsonPath: .status.conditions[?(@.type=="Ready")].status
    - name: Age
      type: date
      jsonPath: .metadata.creationTimestamp
```

### 3. 安全加固措施

```yaml
# CRD安全配置
metadata:
  annotations:
    # RBAC自动更新
    rbac.authorization.k8s.io/autoupdate: "true"
    # 资源配额
    quota.openshift.io/core-resource: "true"
    # 审计日志级别
    audit.kubernetes.io/log-level: "RequestResponse"
    
# 状态子资源保护
subresources:
  status:
    # 只允许控制器更新status
    x-kubernetes-status-subresource: true

# 字段保护配置
schema:
  openAPIV3Schema:
    type: object
    properties:
      spec:
        type: object
        # 敏感字段加密存储
        x-kubernetes-embedded-resource: true
        properties:
          credentials:
            type: object
            # 敏感字段不显示在kubectl get中
            x-kubernetes-preserve-unknown-fields: false
```

### 4. 监控与可观测性

```yaml
# CRD监控集成
apiVersion: monitoring.coreos.com/v1
kind: ServiceMonitor
metadata:
  name: crd-controller-metrics
  namespace: monitoring
spec:
  selector:
    matchLabels:
      app: crd-controller
  endpoints:
  - port: metrics
    interval: 30s
    path: /metrics
    metricRelabelings:
    # 控制器工作队列指标
    - sourceLabels: [__name__]
      regex: 'workqueue_(.+)'
      targetLabel: __name__
      replacement: 'crd_controller_$1'
    # 自定义业务指标
    - sourceLabels: [__name__]
      regex: 'mysqlcluster_(.+)'
      targetLabel: __name__
      replacement: 'mysql_$1'

---
# 告警规则配置
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: crd-alerts
  namespace: monitoring
spec:
  groups:
  - name: crd.rules
    rules:
    - alert: CRDControllerDown
      expr: absent(up{job="crd-controller"} == 1)
      for: 5m
      labels:
        severity: critical
      annotations:
        summary: "CRD控制器不可用"
        
    - alert: CRDReconcileErrors
      expr: rate(controller_runtime_reconcile_errors_total[5m]) > 0.1
      for: 10m
      labels:
        severity: warning
      annotations:
        summary: "CRD协调错误率过高"
```

### 5. 部署与升级策略

```bash
#!/bin/bash
# crd-deployment-script.sh

# CRD部署前检查
validate_crd_deployment() {
  echo "🔍 验证CRD部署环境..."
  
  # 检查Kubernetes版本兼容性
  kubectl version --short | grep -q "v1.25" || {
    echo "❌ Kubernetes版本不兼容，需要v1.25+"
    exit 1
  }
  
  # 检查CRD是否存在冲突
  if kubectl get crd mysqlclusters.database.example.com >/dev/null 2>&1; then
    echo "⚠️  CRD已存在，执行升级流程"
    UPGRADE_MODE=true
  fi
}

# 安全部署CRD
safe_deploy_crd() {
  local crd_file=$1
  
  echo "🚀 安全部署CRD..."
  
  # 创建备份
  if [ "$UPGRADE_MODE" = true ]; then
    echo "💾 创建现有CRD备份..."
    kubectl get crd mysqlclusters.database.example.com -o yaml > backup-crd-$(date +%Y%m%d-%H%M%S).yaml
  fi
  
  # 预验证
  echo "📋 预验证CRD配置..."
  kubectl apply --dry-run=client -f ${crd_file} -o yaml > /dev/null || {
    echo "❌ CRD配置验证失败"
    exit 1
  }
  
  # 分阶段部署
  echo "🎯 执行分阶段部署..."
  kubectl apply -f ${crd_file}
  
  # 等待CRD就绪
  echo "⏳ 等待CRD注册完成..."
  until kubectl get crd mysqlclusters.database.example.com >/dev/null 2>&1; do
    echo "等待CRD注册..."
    sleep 2
  done
  
  # 验证部署结果
  echo "✅ 验证部署结果..."
  kubectl get crd mysqlclusters.database.example.com -o wide
}

# 滚动升级策略
rolling_upgrade_crd() {
  local new_version=$1
  
  echo "🔄 执行滚动升级到版本: ${new_version}"
  
  # 1. 部署新版本CRD（不破坏现有实例）
  kubectl apply -f crd-v${new_version}.yaml
  
  # 2. 逐步迁移现有实例
  kubectl get mysqlcluster --all-namespaces -o jsonpath='{range .items[*]}{.metadata.namespace}/{.metadata.name}{"\n"}{end}' | \
  while read ns_name; do
    ns=$(echo $ns_name | cut -d'/' -f1)
    name=$(echo $ns_name | cut -d'/' -f2)
    echo "升级实例: ${ns}/${name}"
    kubectl patch mysqlcluster ${name} -n ${ns} -p '{"metadata":{"annotations":{"crd-version":"v'${new_version}'"}}}' --type=merge
  done
  
  # 3. 验证升级结果
  echo "✅ 验证升级结果..."
  kubectl get mysqlcluster --all-namespaces -o wide
}

# 回滚机制
rollback_crd() {
  local backup_file=$1
  
  echo "🔙 执行CRD回滚..."
  
  if [ -f "${backup_file}" ]; then
    kubectl apply -f ${backup_file}
    echo "✅ 回滚完成"
  else
    echo "❌ 备份文件不存在: ${backup_file}"
    exit 1
  fi
}

# 主执行流程
main() {
  validate_crd_deployment
  safe_deploy_crd "config/crd/bases/database.example.com_mysqlclusters.yaml"
  
  if [ "$UPGRADE_MODE" = true ]; then
    rolling_upgrade_crd "2"
  fi
}

main "$@"
```

### 6. 故障排除与调试

```bash
#!/bin/bash
# crd-debugging-toolkit.sh

# CRD诊断工具
diagnose_crd_issues() {
  echo "=== CRD诊断报告 ==="
  
  # 1. CRD基本状态检查
  echo "1. CRD基本信息检查:"
  kubectl get crd mysqlclusters.database.example.com -o wide
  
  # 2. CRD条件检查
  echo "2. CRD条件状态:"
  kubectl get crd mysqlclusters.database.example.com -o jsonpath='{.status.conditions[*].type}' | tr ' ' '\n'
  
  # 3. API资源检查
  echo "3. API资源注册状态:"
  kubectl api-resources | grep mysqlcluster
  
  # 4. OpenAPI schema验证
  echo "4. OpenAPI schema验证:"
  kubectl get --raw "/openapi/v2" | jq '.definitions."com.example.database.v1.MySQLCluster"'
  
  # 5. RBAC权限检查
  echo "5. RBAC权限检查:"
  kubectl auth can-i create mysqlclusters.database.example.com --as=system:serviceaccount:default:test-sa
}

# 实例故障诊断
debug_crd_instances() {
  echo "=== CRD实例诊断 ==="
  
  # 获取所有实例
  kubectl get mysqlcluster --all-namespaces -o wide
  
  # 检查实例状态详细信息
  kubectl get mysqlcluster --all-namespaces -o jsonpath='{range .items[*]}{.metadata.namespace}/{.metadata.name}: {.status.phase}{"\n"}{end}'
  
  # 检查控制器日志
  echo "检查控制器日志:"
  kubectl logs -l app=crd-controller -n crd-system --tail=100
  
  # 检查事件
  echo "检查相关事件:"
  kubectl get events --field-selector involvedObject.kind=MySQLCluster --sort-by='.lastTimestamp'
}

# 性能分析
analyze_crd_performance() {
  echo "=== CRD性能分析 ==="
  
  # 工作队列状态
  echo "工作队列深度:"
  kubectl get --raw "/metrics" | grep "workqueue_depth" | grep "mysqlcluster"
  
  # 协调延迟
  echo "协调操作延迟:"
  kubectl get --raw "/metrics" | grep "controller_runtime_reconcile_time"
  
  # API调用统计
  echo "API调用统计:"
  kubectl get --raw "/metrics" | grep "rest_client_requests_total"
}

# 执行完整诊断
perform_complete_diagnostics() {
  diagnose_crd_issues
  debug_crd_instances
  analyze_crd_performance
  
  echo "=== 诊断完成 ==="
  echo "建议检查点:"
  echo "1. CRD定义是否符合OpenAPI v3规范"
  echo "2. 控制器是否有足够的RBAC权限"
  echo "3. etcd存储是否正常"
  echo "4. 网络连接是否稳定"
}

perform_complete_diagnostics
```

### 4. 监控与可观测性

```yaml
# CRD监控集成
apiVersion: monitoring.coreos.com/v1
kind: ServiceMonitor
metadata:
  name: crd-controller-metrics
  namespace: monitoring
spec:
  selector:
    matchLabels:
      app: crd-controller
  endpoints:
  - port: metrics
    interval: 30s
    path: /metrics
    metricRelabelings:
    # 控制器工作队列指标
    - sourceLabels: [__name__]
      regex: 'workqueue_(.+)'
      targetLabel: __name__
      replacement: 'crd_controller_$1'
    # 自定义业务指标
    - sourceLabels: [__name__]
      regex: 'mysqlcluster_(.+)'
      targetLabel: __name__
      replacement: 'mysql_$1'

---
# 告警规则配置
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: crd-alerts
  namespace: monitoring
spec:
  groups:
  - name: crd.rules
    rules:
    - alert: CRDControllerDown
      expr: absent(up{job="crd-controller"} == 1)
      for: 5m
      labels:
        severity: critical
      annotations:
        summary: "CRD控制器不可用"
        
    - alert: CRDReconcileErrors
      expr: rate(controller_runtime_reconcile_errors_total[5m]) > 0.1
      for: 10m
      labels:
        severity: warning
      annotations:
        summary: "CRD协调错误率过高"
```

### 5. 部署与升级策略

```bash
#!/bin/bash
# crd-deployment-script.sh

# CRD部署前检查
validate_crd_deployment() {
  echo "🔍 验证CRD部署环境..."
  
  # 检查Kubernetes版本兼容性
  kubectl version --short | grep -q "v1.25" || {
    echo "❌ Kubernetes版本不兼容，需要v1.25+"
    exit 1
  }
  
  # 检查CRD是否存在冲突
  if kubectl get crd mysqlclusters.database.example.com >/dev/null 2>&1; then
    echo "⚠️  CRD已存在，执行升级流程"
    UPGRADE_MODE=true
  fi
}

# 安全部署CRD
safe_deploy_crd() {
  local crd_file=$1
  
  echo "🚀 安全部署CRD..."
  
  # 创建备份
  if [ "$UPGRADE_MODE" = true ]; then
    echo "💾 创建现有CRD备份..."
    kubectl get crd mysqlclusters.database.example.com -o yaml > backup-crd-$(date +%Y%m%d-%H%M%S).yaml
  fi
  
  # 预验证
  echo "📋 预验证CRD配置..."
  kubectl apply --dry-run=client -f ${crd_file} -o yaml > /dev/null || {
    echo "❌ CRD配置验证失败"
    exit 1
  }
  
  # 分阶段部署
  echo "🎯 执行分阶段部署..."
  kubectl apply -f ${crd_file}
  
  # 等待CRD就绪
  echo "⏳ 等待CRD注册完成..."
  until kubectl get crd mysqlclusters.database.example.com >/dev/null 2>&1; do
    echo "等待CRD注册..."
    sleep 2
  done
  
  # 验证部署结果
  echo "✅ 验证部署结果..."
  kubectl get crd mysqlclusters.database.example.com -o wide
}

# 滚动升级策略
rolling_upgrade_crd() {
  local new_version=$1
  
  echo "🔄 执行滚动升级到版本: ${new_version}"
  
  # 1. 部署新版本CRD（不破坏现有实例）
  kubectl apply -f crd-v${new_version}.yaml
  
  # 2. 逐步迁移现有实例
  kubectl get mysqlcluster --all-namespaces -o jsonpath='{range .items[*]}{.metadata.namespace}/{.metadata.name}{"\n"}{end}' | \
  while read ns_name; do
    ns=$(echo $ns_name | cut -d'/' -f1)
    name=$(echo $ns_name | cut -d'/' -f2)
    echo "升级实例: ${ns}/${name}"
    kubectl patch mysqlcluster ${name} -n ${ns} -p '{"metadata":{"annotations":{"crd-version":"v'${new_version}'"}}}' --type=merge
  done
  
  # 3. 验证升级结果
  echo "✅ 验证升级结果..."
  kubectl get mysqlcluster --all-namespaces -o wide
}

# 回滚机制
rollback_crd() {
  local backup_file=$1
  
  echo "🔙 执行CRD回滚..."
  
  if [ -f "${backup_file}" ]; then
    kubectl apply -f ${backup_file}
    echo "✅ 回滚完成"
  else
    echo "❌ 备份文件不存在: ${backup_file}"
    exit 1
  fi
}

# 主执行流程
main() {
  validate_crd_deployment
  safe_deploy_crd "config/crd/bases/database.example.com_mysqlclusters.yaml"
  
  if [ "$UPGRADE_MODE" = true ]; then
    rolling_upgrade_crd "2"
  fi
}

main "$@"
```

### 6. 故障排除与调试

```bash
#!/bin/bash
# crd-debugging-toolkit.sh

# CRD诊断工具
diagnose_crd_issues() {
  echo "=== CRD诊断报告 ==="
  
  # 1. CRD基本状态检查
  echo "1. CRD基本信息检查:"
  kubectl get crd mysqlclusters.database.example.com -o wide
  
  # 2. CRD条件检查
  echo "2. CRD条件状态:"
  kubectl get crd mysqlclusters.database.example.com -o jsonpath='{.status.conditions[*].type}' | tr ' ' '\n'
  
  # 3. API资源检查
  echo "3. API资源注册状态:"
  kubectl api-resources | grep mysqlcluster
  
  # 4. OpenAPI schema验证
  echo "4. OpenAPI schema验证:"
  kubectl get --raw "/openapi/v2" | jq '.definitions."com.example.database.v1.MySQLCluster"'
  
  # 5. RBAC权限检查
  echo "5. RBAC权限检查:"
  kubectl auth can-i create mysqlclusters.database.example.com --as=system:serviceaccount:default:test-sa
}

# 实例故障诊断
debug_crd_instances() {
  echo "=== CRD实例诊断 ==="
  
  # 获取所有实例
  kubectl get mysqlcluster --all-namespaces -o wide
  
  # 检查实例状态详细信息
  kubectl get mysqlcluster --all-namespaces -o jsonpath='{range .items[*]}{.metadata.namespace}/{.metadata.name}: {.status.phase}{"\n"}{end}'
  
  # 检查控制器日志
  echo "检查控制器日志:"
  kubectl logs -l app=crd-controller -n crd-system --tail=100
  
  # 检查事件
  echo "检查相关事件:"
  kubectl get events --field-selector involvedObject.kind=MySQLCluster --sort-by='.lastTimestamp'
}

# 性能分析
analyze_crd_performance() {
  echo "=== CRD性能分析 ==="
  
  # 工作队列状态
  echo "工作队列深度:"
  kubectl get --raw "/metrics" | grep "workqueue_depth" | grep "mysqlcluster"
  
  # 协调延迟
  echo "协调操作延迟:"
  kubectl get --raw "/metrics" | grep "controller_runtime_reconcile_time"
  
  # API调用统计
  echo "API调用统计:"
  kubectl get --raw "/metrics" | grep "rest_client_requests_total"
}

# 执行完整诊断
perform_complete_diagnostics() {
  diagnose_crd_issues
  debug_crd_instances
  analyze_crd_performance
  
  echo "=== 诊断完成 ==="
  echo "建议检查点:"
  echo "1. CRD定义是否符合OpenAPI v3规范"
  echo "2. 控制器是否有足够的RBAC权限"
  echo "3. etcd存储是否正常"
  echo "4. 网络连接是否稳定"
}

perform_complete_diagnostics
```

### 2. 版本管理策略

```yaml
# 版本演进建议
versions:
# v1alpha1 - 实验性功能
- name: v1alpha1
  served: false  # 不对外提供
  storage: false
  
# v1beta1 - Beta功能
- name: v1beta1
  served: true
  storage: false
  
# v1 - 稳定版本
- name: v1
  served: true
  storage: true  # 主存储版本
```

### 3. 安全考虑

```yaml
# 安全相关的CRD配置
metadata:
  annotations:
    # RBAC最小权限
    rbac.authorization.k8s.io/autoupdate: "true"
    
    # 资源配额
    quota.openshift.io/core-resource: "true"
    
    # 审计日志
    audit.kubernetes.io/log-level: "Metadata"

# 状态保护
subresources:
  status:
    # 只允许控制器更新status
    x-kubernetes-status-subresource: true
```

## CRD故障排除

### 常见问题诊断

```bash
# 1. CRD验证失败
kubectl describe crd mysqlclusters.database.example.com

# 2. 实例创建失败
kubectl get events --field-selector involvedObject.kind=MySQLCluster

# 3. Schema验证错误
kubectl api-resources | grep mysqlcluster

# 4. 版本转换问题
kubectl get mysqlcluster -o yaml | kubectl convert -f - --output-version=v1beta1

# 5. 权限问题
kubectl auth can-i create mysqlclusters.database.example.com
```

### 调试命令集合

```bash
# 查看CRD详细信息
kubectl get crd mysqlclusters.database.example.com -o yaml

# 查看CRD支持的版本
kubectl get crd mysqlclusters.database.example.com -o jsonpath='{.spec.versions[*].name}'

# 查看打印列配置
kubectl get crd mysqlclusters.database.example.com -o jsonpath='{.spec.versions[*].additionalPrinterColumns}'

# 测试CR实例
kubectl create -f test-instance.yaml --dry-run=server -o yaml

# 验证OpenAPI schema
kubectl get --raw "/openapi/v2" | jq '.definitions | keys[] | select(contains("mysqlcluster"))'
```

## CRD监控与运维

### 1. 监控指标

```yaml
# Prometheus监控配置
apiVersion: monitoring.coreos.com/v1
kind: ServiceMonitor
metadata:
  name: crd-metrics
  namespace: monitoring
spec:
  selector:
    matchLabels:
      app: crd-controller
  endpoints:
  - port: metrics
    interval: 30s
    path: /metrics
    metricRelabelings:
    - sourceLabels: [__name__]
      regex: 'workqueue_(.+)'
      targetLabel: __name__
      replacement: 'crd_controller_$1'
```

### 2. 健康检查

```bash
#!/bin/bash
# crd-health-check.sh

NAMESPACE="mysql-operator-system"
CRD_NAME="mysqlclusters.database.example.com"

echo "=== CRD健康检查 ==="

# 1. CRD存在性检查
if ! kubectl get crd ${CRD_NAME} >/dev/null 2>&1; then
  echo "❌ CRD ${CRD_NAME} 不存在"
  exit 1
fi
echo "✅ CRD存在"

# 2. CRD版本检查
VERSIONS=$(kubectl get crd ${CRD_NAME} -o jsonpath='{.spec.versions[*].name}')
echo "📋 支持版本: ${VERSIONS}"

# 3. 存储版本检查
STORAGE_VERSION=$(kubectl get crd ${CRD_NAME} -o jsonpath='{.spec.versions[?(@.storage==true)].name}')
echo "💾 存储版本: ${STORAGE_VERSION}"

# 4. 实例数量检查
INSTANCE_COUNT=$(kubectl get ${CRD_NAME} --all-namespaces --no-headers | wc -l)
echo "📊 实例总数: ${INSTANCE_COUNT}"

# 5. 控制器状态检查
CONTROLLER_POD=$(kubectl get pods -n ${NAMESPACE} -l control-plane=controller-manager -o name)
if [ -n "${CONTROLLER_POD}" ]; then
  kubectl get ${CONTROLLER_POD} -n ${NAMESPACE} -o wide
else
  echo "⚠️ 未找到控制器Pod"
fi

echo "✅ CRD健康检查完成"
```

---
**CRD开发原则**: 结构化定义、版本兼容、安全验证、可观测性

---
**表格底部标记**: Kusheet Project, 作者 Allen Galler (allengaller@gmail.com)