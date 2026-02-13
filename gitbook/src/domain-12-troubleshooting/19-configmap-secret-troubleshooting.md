# 19 - ConfigMap/Secret 故障排查 (ConfigMap/Secret Troubleshooting)

> **适用版本**: Kubernetes v1.25-v1.32 | **最后更新**: 2026-02 | **参考**: [Kubernetes ConfigMaps](https://kubernetes.io/docs/concepts/configuration/configmap/), [Kubernetes Secrets](https://kubernetes.io/docs/concepts/configuration/secret/)

---

## 1. 配置管理故障诊断总览 (Configuration Management Diagnosis Overview)

### 1.1 常见故障现象分类

| 故障类型 | 症状表现 | 影响范围 | 紧急程度 |
|---------|---------|---------|---------|
| **配置未注入** | 环境变量为空/挂载失败 | 应用配置缺失 | P0 - 紧急 |
| **配置热更新失效** | 修改后应用不感知 | 配置变更延迟 | P1 - 高 |
| **敏感信息泄露** | Secret明文存储 | 安全风险 | P0 - 紧急 |
| **大小限制超限** | 创建失败 | 部署中断 | P1 - 高 |
| **权限不足** | 无法读取配置 | 应用启动失败 | P0 - 紧急 |

### 1.2 配置管理架构回顾

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                   ConfigMap/Secret 故障诊断架构                               │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  ┌─────────────────────────────────────────────────────────────────────┐  │
│  │                    配置数据源层                                      │  │
│  │  ┌─────────────┐    ┌─────────────┐    ┌─────────────┐              │  │
│  │  │   外部系统   │   │   GitOps    │   │   手动创建   │              │  │
│  │  │ (Vault等)   │   │ (ArgoCD)    │   │ (kubectl)   │              │  │
│  │  └─────────────┘    └─────────────┘    └─────────────┘              │  │
│  └─────────────────────────────────────────────────────────────────────┘  │
│                              │                                              │
│                              ▼                                              │
│  ┌─────────────────────────────────────────────────────────────────────┐  │
│  │                  Kubernetes API Server                              │  │
│  │  ┌───────────────────────────────────────────────────────────────┐  │  │
│  │  │                     etcd存储                                    │  │  │
│  │  │  ┌─────────────┐    ┌─────────────┐    ┌─────────────┐       │  │  │
│  │  │  │ ConfigMap   │    │   Secret    │    │  存储加密    │       │  │  │
│  │  │  │   数据      │    │   数据      │    │  (Encryption)│       │  │  │
│  │  │  └─────────────┘    └─────────────┘    └─────────────┘       │  │  │
│  │  └───────────────────────────────────────────────────────────────┘  │  │
│  └─────────────────────────────────────────────────────────────────────┘  │
│                              │                                              │
│         ┌────────────────────┼────────────────────┐                       │
│         │                    │                    │                       │
│         ▼                    ▼                    ▼                       │
│  ┌─────────────┐    ┌─────────────┐    ┌─────────────┐                   │
│  │   注入方式   │    │   挂载方式   │    │   引用方式   │                   │
│  │ (env/value) │    │ (volume)    │    │ (ref)       │                   │
│  └─────────────┘    └─────────────┘    └─────────────┘                   │
│         │                    │                    │                       │
│         ▼                    ▼                    ▼                       │
│  ┌─────────────────────────────────────────────────────────────────────┐  │
│  │                        Pod应用层                                     │  │
│  │  ┌─────────────┐    ┌─────────────┐    ┌─────────────┐              │  │
│  │  │   容器进程   │   │   文件系统   │   │   环境变量   │              │  │
│  │  │  (Process)  │   │   (FS)      │   │   (Env)     │              │  │
│  │  └─────────────┘    └─────────────┘    └─────────────┘              │  │
│  └─────────────────────────────────────────────────────────────────────┘  │
│                              │                                              │
│                              ▼                                              │
│  ┌─────────────────────────────────────────────────────────────────────┐  │
│  │                      应用运行时层                                    │  │
│  │  ┌─────────────┐    ┌─────────────┐    ┌─────────────┐              │  │
│  │  │   配置读取   │   │   热更新监听  │   │   错误处理   │              │  │
│  │  │ (Read Conf) │   │ (Watch FS)  │   │ (Error Handle)│              │  │
│  │  └─────────────┘    └─────────────┘    └─────────────┘              │  │
│  └─────────────────────────────────────────────────────────────────────┘  │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

---

## 2. 基础配置检查 (Basic Configuration Check)

### 2.1 配置资源状态验证

```bash
# ========== 1. 资源存在性检查 ==========
# 检查ConfigMap是否存在
kubectl get configmap <configmap-name> -n <namespace>

# 检查Secret是否存在
kubectl get secret <secret-name> -n <namespace>

# 列出所有配置资源
kubectl get configmap,secret -n <namespace>

# ========== 2. 配置内容检查 ==========
# 查看ConfigMap详细内容
kubectl get configmap <configmap-name> -n <namespace> -o yaml

# 查看Secret内容 (base64解码)
kubectl get secret <secret-name> -n <namespace> -o jsonpath='{.data}' | jq 'with_entries(.value |= @base64d)'

# 检查特定键值
kubectl get configmap <configmap-name> -n <namespace> -o jsonpath='{.data.<key-name>}'

# ========== 3. 引用关系检查 ==========
# 查找引用特定ConfigMap的Pod
kubectl get pods -n <namespace> -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{.spec.volumes[*].configMap.name}{"\n"}{end}' | grep <configmap-name>

# 查找引用特定Secret的Pod
kubectl get pods -n <namespace> -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{.spec.volumes[*].secret.secretName}{"\n"}{end}' | grep <secret-name>
```

### 2.2 权限和访问控制检查

```bash
# ========== RBAC权限检查 ==========
# 检查ServiceAccount权限
kubectl auth can-i get configmaps --as=system:serviceaccount:<namespace>:<serviceaccount-name>
kubectl auth can-i get secrets --as=system:serviceaccount:<namespace>:<serviceaccount-name>

# 检查RoleBinding配置
kubectl get rolebindings -n <namespace> -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{.subjects[*].name}{"\n"}{end}'

# 验证Pod Security Context
kubectl get pod <pod-name> -n <namespace> -o jsonpath='{.spec.securityContext}'
```

---

## 3. 配置注入问题排查 (Configuration Injection Troubleshooting)

### 3.1 环境变量注入问题

```bash
# ========== 1. Pod配置检查 ==========
# 查看Pod的环境变量配置
kubectl get pod <pod-name> -n <namespace> -o jsonpath='{.spec.containers[*].env}' | jq

# 检查环境变量来源
kubectl get pod <pod-name> -n <namespace> -o jsonpath='{.spec.containers[*].env[*].valueFrom}'

# ========== 2. 运行时环境变量验证 ==========
# 进入Pod检查实际环境变量
kubectl exec -n <namespace> <pod-name> -- env | grep <CONFIG_PREFIX>

# 检查特定环境变量值
kubectl exec -n <namespace> <pod-name> -- printenv <ENV_VAR_NAME>

# ========== 3. 常见注入问题诊断 ==========
# 检查ConfigMap键名匹配
kubectl get configmap <configmap-name> -n <namespace> -o jsonpath='{.data}' | jq 'keys'

# 验证Secret键名
kubectl get secret <secret-name> -n <namespace> -o jsonpath='{.data}' | jq 'keys'

# 检查大小写敏感性
kubectl get pod <pod-name> -n <namespace> -o jsonpath='{.spec.containers[*].env[*].name}'
```

### 3.2 文件挂载问题

```bash
# ========== 1. 卷挂载检查 ==========
# 查看Pod卷配置
kubectl get pod <pod-name> -n <namespace> -o jsonpath='{.spec.volumes}' | jq

# 检查卷挂载点
kubectl get pod <pod-name> -n <namespace> -o jsonpath='{.spec.containers[*].volumeMounts}' | jq

# ========== 2. 文件系统验证 ==========
# 检查挂载点是否存在
kubectl exec -n <namespace> <pod-name> -- ls -la <mount-path>

# 查看挂载文件内容
kubectl exec -n <namespace> <pod-name> -- cat <mount-path>/<file-name>

# 检查文件权限
kubectl exec -n <namespace> <pod-name> -- stat <mount-path>/<file-name>

# ========== 3. subPath问题排查 ==========
# 检查subPath配置
kubectl get pod <pod-name> -n <namespace> -o jsonpath='{.spec.containers[*].volumeMounts[?(@.subPath)]}' | jq

# 验证subPath文件存在性
kubectl exec -n <namespace> <pod-name> -- ls -la <mount-path>/<subPath>
```

---

## 4. 热更新机制问题 (Hot Reload Issues)

### 4.1 ConfigMap热更新验证

```bash
# ========== 1. 更新传播检查 ==========
# 修改ConfigMap
kubectl patch configmap <configmap-name> -n <namespace> -p '{"data":{"test-key":"new-value"}}'

# 监控Pod是否收到更新
kubectl exec -n <namespace> <pod-name> -- watch -n 1 'cat <config-file-path>'

# 检查kubelet更新事件
kubectl get events -n <namespace> --field-selector involvedObject.name=<pod-name>

# ========== 2. 应用层面热更新检查 ==========
# 检查应用是否监听文件变化
kubectl exec -n <namespace> <pod-name> -- ps aux | grep inotify

# 验证应用重载机制
kubectl logs -n <namespace> <pod-name> -f | grep -i reload

# ========== 3. 更新延迟分析 ==========
# 测量更新传播时间
START_TIME=$(date +%s)
kubectl patch configmap <configmap-name> -n <namespace> -p '{"data":{"timestamp":"'$(date)'"}}'
while true; do
    VALUE=$(kubectl exec -n <namespace> <pod-name> -- cat <config-file-path> | grep timestamp | cut -d'"' -f4)
    if [ -n "$VALUE" ]; then
        END_TIME=$(date +%s)
        echo "Update propagation time: $((END_TIME - START_TIME)) seconds"
        break
    fi
    sleep 1
done
```

### 4.2 不支持热更新的场景

```bash
# ========== 1. 环境变量不支持热更新 ==========
# 验证环境变量在Pod生命周期内不变
ORIGINAL_VALUE=$(kubectl exec -n <namespace> <pod-name> -- printenv <ENV_VAR>)
kubectl patch configmap <configmap-name> -n <namespace> -p '{"data":{"<key>":"updated-value"}}'
UPDATED_VALUE=$(kubectl exec -n <namespace> <pod-name> -- printenv <ENV_VAR>)
echo "Original: $ORIGINAL_VALUE"
echo "Updated: $UPDATED_VALUE"
echo "Values equal: $([ "$ORIGINAL_VALUE" = "$UPDATED_VALUE" ] && echo "Yes" || echo "No")"

# ========== 2. projected卷更新机制 ==========
# 检查projected卷配置
kubectl get pod <pod-name> -n <namespace> -o jsonpath='{.spec.volumes[?(@.projected)]}' | jq

# projected卷支持热更新，但有延迟
```

---

## 5. 安全性问题排查 (Security Issues Troubleshooting)

### 5.1 Secret泄露检测

```bash
# ========== 1. 明文存储检查 ==========
# 搜索可能包含敏感信息的ConfigMap
kubectl get configmaps --all-namespaces -o jsonpath='{range .items[*]}{.metadata.namespace}{"/"}{.metadata.name}{"\n"}{end}' | while read cm; do
    NS=$(echo $cm | cut -d/ -f1)
    NAME=$(echo $cm | cut -d/ -f2)
    DATA=$(kubectl get configmap $NAME -n $NS -o jsonpath='{.data}' 2>/dev/null)
    if echo "$DATA" | grep -E "(password|token|key|secret)" >/dev/null; then
        echo "WARNING: Potential secret in ConfigMap $cm"
    fi
done

# ========== 2. 权限过度宽松检查 ==========
# 检查Secret访问权限
kubectl get roles,rolebindings -A -o jsonpath='{range .items[*]}{.metadata.namespace}{"/"}{.metadata.name}{"\t"}{.rules[*].resources}{"\n"}{end}' | grep -E "(secrets|configmaps)"

# 检查默认ServiceAccount权限
kubectl get rolebindings -n <namespace> -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{.subjects[*].name}{"\n"}{end}' | grep default

# ========== 3. 加密存储验证 ==========
# 检查etcd加密配置
kubectl get encryptionconfig -n kube-system

# 验证Secret存储加密
ETCD_POD=$(kubectl get pods -n kube-system -l component=etcd -o jsonpath='{.items[0].metadata.name}')
kubectl exec -n kube-system $ETCD_POD -- sh -c "ETCDCTL_API=3 etcdctl get /registry/secrets/<namespace>/<secret-name> | strings | grep -E '(password|token)'"
```

### 5.2 安全最佳实践检查

```bash
# ========== 推荐的安全配置模板 ==========
cat <<EOF > secure-config-template.yaml
apiVersion: v1
kind: Secret
metadata:
  name: app-secret
  namespace: production
  annotations:
    # 启用不可变配置 (Kubernetes 1.21+)
    kubernetes.io/change-cause: "Initial creation"
type: Opaque
immutable: true  # 防止意外修改
data:
  # 敏感数据使用base64编码
  database-password: $(echo -n "secure-password" | base64)
  api-token: $(echo -n "jwt-token-here" | base64)
---
apiVersion: v1
kind: ConfigMap
metadata:
  name: app-config
  namespace: production
  annotations:
    kubernetes.io/change-cause: "Configuration version 1.0"
data:
  # 非敏感配置
  app.properties: |
    server.port=8080
    logging.level=INFO
EOF

# ========== 权限最小化配置 ==========
cat <<EOF | kubectl apply -f -
apiVersion: rbac.authorization.k8s.io/v1
kind: Role
metadata:
  name: config-reader
  namespace: production
rules:
- apiGroups: [""]
  resources: ["configmaps"]
  verbs: ["get", "list"]
---
apiVersion: rbac.authorization.k8s.io/v1
kind: RoleBinding
metadata:
  name: app-config-binding
  namespace: production
subjects:
- kind: ServiceAccount
  name: app-service-account
  namespace: production
roleRef:
  kind: Role
  name: config-reader
  apiGroup: rbac.authorization.k8s.io
EOF
```

---

## 6. 性能和容量问题 (Performance and Capacity Issues)

### 6.1 大小限制和性能优化

```bash
# ========== 1. 大小限制检查 ==========
# ConfigMap/Secret大小限制: 1MB
# 检查现有配置大小
kubectl get configmap <configmap-name> -n <namespace> -o jsonpath='{.data}' | wc -c

# 检查etcd存储压力
ETCD_POD=$(kubectl get pods -n kube-system -l component=etcd -o jsonpath='{.items[0].metadata.name}')
kubectl exec -n kube-system $ETCD_POD -- sh -c "ETCDCTL_API=3 etcdctl endpoint status --cluster --write-out=table"

# ========== 2. 分割大配置策略 ==========
# 将大ConfigMap分割为多个小ConfigMap
cat <<EOF > split-config-example.yaml
apiVersion: v1
kind: ConfigMap
metadata:
  name: app-config-part1
  namespace: production
data:
  # 第一部分配置
  database.properties: |
    db.host=postgresql.example.com
    db.port=5432
    
---
apiVersion: v1
kind: ConfigMap
metadata:
  name: app-config-part2
  namespace: production
data:
  # 第二部分配置
  cache.properties: |
    cache.enabled=true
    cache.size=1000
EOF

# ========== 3. 外部配置存储集成 ==========
# 使用HashiCorp Vault集成示例
cat <<EOF > vault-integration.yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: app-with-vault
  namespace: production
spec:
  template:
    spec:
      serviceAccountName: app-service-account
      containers:
      - name: app
        image: myapp:latest
        env:
        - name: VAULT_ADDR
          value: "http://vault:8200"
        - name: VAULT_TOKEN
          valueFrom:
            secretKeyRef:
              name: vault-token
              key: token
        volumeMounts:
        - name: vault-agent-config
          mountPath: /vault/config
      volumes:
      - name: vault-agent-config
        configMap:
          name: vault-agent-config
EOF
```

### 6.2 监控和告警配置

```bash
# ========== 配置资源监控 ==========
cat <<EOF | kubectl apply -f -
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: configmap-secret-alerts
  namespace: monitoring
spec:
  groups:
  - name: config.rules
    rules:
    - alert: LargeConfigMapDetected
      expr: kube_configmap_metadata_resource_version > 1000000
      for: 5m
      labels:
        severity: warning
      annotations:
        summary: "Large ConfigMap detected in namespace {{ \$labels.namespace }}"
        
    - alert: ConfigMapNotFound
      expr: absent(kube_configmap_info{configmap="<configmap-name>"})
      for: 2m
      labels:
        severity: critical
      annotations:
        summary: "Required ConfigMap missing"
        
    - alert: SecretAccessDenied
      expr: rate(apiserver_request_total{resource="secrets",code=~"4.."}[5m]) > 0
      for: 1m
      labels:
        severity: critical
      annotations:
        summary: "Secret access denied (namespace {{ \$labels.namespace }})"
EOF

# ========== 配置变更审计 ==========
# 启用配置变更审计日志
cat <<EOF > audit-policy-config.yaml
apiVersion: audit.k8s.io/v1
kind: Policy
rules:
- level: RequestResponse
  resources:
  - group: ""
    resources: ["configmaps", "secrets"]
  verbs: ["create", "update", "patch", "delete"]
EOF
```

---

## 7. CI/CD集成和自动化 (CI/CD Integration and Automation)

### 7.1 配置验证流水线

```bash
# ========== 配置Linting脚本 ==========
cat <<'EOF' > config-linter.sh
#!/bin/bash

# ConfigMap/Secret配置验证脚本
NAMESPACE=${1:-default}

echo "Validating configurations in namespace: $NAMESPACE"

# 检查敏感信息泄露
check_sensitive_data() {
    echo "Checking for sensitive data in ConfigMaps..."
    kubectl get configmaps -n $NAMESPACE -o jsonpath='{range .items[*]}{.metadata.name}{"\n"}{end}' | while read cm; do
        DATA=$(kubectl get configmap $cm -n $NAMESPACE -o jsonpath='{.data}' 2>/dev/null)
        if echo "$DATA" | grep -E "(password|token|key|secret)" >/dev/null; then
            echo "WARNING: Potential sensitive data in ConfigMap: $cm"
        fi
    done
}

# 检查配置大小
check_config_size() {
    echo "Checking configuration sizes..."
    kubectl get configmaps -n $NAMESPACE -o jsonpath='{range .items[*]}{.metadata.name}{"\n"}{end}' | while read cm; do
        SIZE=$(kubectl get configmap $cm -n $NAMESPACE -o jsonpath='{.data}' | wc -c)
        if [ $SIZE -gt 500000 ]; then  # 500KB警告阈值
            echo "WARNING: Large ConfigMap $cm: $((SIZE/1024)) KB"
        fi
    done
}

# 检查引用完整性
check_references() {
    echo "Checking configuration references..."
    kubectl get pods -n $NAMESPACE -o jsonpath='{range .items[*]}{.metadata.name}{"\n"}{end}' | while read pod; do
        # 检查ConfigMap引用
        kubectl get pod $pod -n $NAMESPACE -o jsonpath='{.spec.volumes[*].configMap.name}' 2>/dev/null | tr ' ' '\n' | while read cm_ref; do
            if [ -n "$cm_ref" ] && ! kubectl get configmap $cm_ref -n $NAMESPACE >/dev/null 2>&1; then
                echo "ERROR: Pod $pod references non-existent ConfigMap: $cm_ref"
            fi
        done
        
        # 检查Secret引用
        kubectl get pod $pod -n $NAMESPACE -o jsonpath='{.spec.volumes[*].secret.secretName}' 2>/dev/null | tr ' ' '\n' | while read secret_ref; do
            if [ -n "$secret_ref" ] && ! kubectl get secret $secret_ref -n $NAMESPACE >/dev/null 2>&1; then
                echo "ERROR: Pod $pod references non-existent Secret: $secret_ref"
            fi
        done
    done
}

# 执行检查
check_sensitive_data
check_config_size
check_references

echo "Configuration validation completed"
EOF

chmod +x config-linter.sh

# ========== 自动化轮换脚本 ==========
cat <<'EOF' > secret-rotation.sh
#!/bin/bash

# Secret自动轮换脚本
SECRET_NAME=$1
NAMESPACE=${2:-default}
NEW_VALUE=$3

if [ -z "$SECRET_NAME" ] || [ -z "$NEW_VALUE" ]; then
    echo "Usage: $0 <secret-name> <namespace> <new-value>"
    exit 1
fi

echo "Rotating secret: $SECRET_NAME in namespace: $NAMESPACE"

# 备份当前Secret
kubectl get secret $SECRET_NAME -n $NAMESPACE -o yaml > backup-${SECRET_NAME}-$(date +%Y%m%d-%H%M%S).yaml

# 更新Secret
kubectl patch secret $SECRET_NAME -n $NAMESPACE -p "{\"data\":{\"value\":\"$(echo -n "$NEW_VALUE" | base64)\"}}"

# 验证更新
UPDATED_VALUE=$(kubectl get secret $SECRET_NAME -n $NAMESPACE -o jsonpath='{.data.value}' | base64 -d)
if [ "$UPDATED_VALUE" = "$NEW_VALUE" ]; then
    echo "Secret rotation successful"
    
    # 触发相关Pod重启以获取新值
    echo "Restarting pods that use this secret..."
    kubectl get pods -n $NAMESPACE -o jsonpath='{range .items[*]}{.metadata.name}{"\n"}{end}' | while read pod; do
        if kubectl get pod $pod -n $NAMESPACE -o jsonpath='{.spec.volumes[*].secret.secretName}' | grep -q $SECRET_NAME; then
            echo "Restarting pod: $pod"
            kubectl delete pod $pod -n $NAMESPACE
        fi
    done
else
    echo "ERROR: Secret rotation failed"
    exit 1
fi
EOF

chmod +x secret-rotation.sh
```

### 7.2 GitOps配置管理

```bash
# ========== ArgoCD配置同步检查 ==========
# 检查应用同步状态
kubectl get applications -n argocd -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{.status.sync.status}{"\n"}{end}'

# 验证配置漂移
argocd app diff <app-name> --local <path-to-configs>

# ========== Helm配置模板最佳实践 ==========
cat <<EOF > config-helm-template.yaml
{{- if .Values.config }}
apiVersion: v1
kind: ConfigMap
metadata:
  name: {{ include "app.fullname" . }}-config
  namespace: {{ .Release.Namespace }}
  labels:
    {{- include "app.labels" . | nindent 4 }}
  annotations:
    checksum/config: {{ include (print $.Template.BasePath "/configmap.yaml") . | sha256sum }}
data:
  {{- range $key, $value := .Values.config }}
  {{ $key }}: |
    {{- $value | nindent 4 }}
  {{- end }}
{{- end }}

---
{{- if .Values.secrets }}
apiVersion: v1
kind: Secret
metadata:
  name: {{ include "app.fullname" . }}-secret
  namespace: {{ .Release.Namespace }}
  labels:
    {{- include "app.labels" . | nindent 4 }}
type: Opaque
data:
  {{- range $key, $value := .Values.secrets }}
  {{ $key }}: {{ $value | b64enc | quote }}
  {{- end }}
{{- end }}
EOF
```

---