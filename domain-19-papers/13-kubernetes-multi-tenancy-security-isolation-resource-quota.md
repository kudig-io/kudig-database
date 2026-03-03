# Kubernetes 多租户安全隔离与资源配额管理 (Multi-Tenancy Security Isolation and Resource Quota Management)

> **作者**: 多租户架构专家 | **版本**: v1.5 | **更新时间**: 2026-03-03
> **适用场景**: 企业级多租户平台 | **复杂度**: ⭐⭐⭐⭐⭐

## 🎯 摘要

本文档深入探讨了Kubernetes多租户环境下的安全隔离机制和资源配额管理策略，基于大型企业多租户平台的实践经验，提供从基础隔离到高级安全控制的完整解决方案，帮助企业构建安全、高效的多租户Kubernetes平台。

## 1. 多租户架构设计原则

### 1.1 多租户核心概念

```yaml
多租户架构原则:
  1. 数据隔离 (Data Isolation)
     - 命名空间级别的资源隔离
     - 存储卷的数据隔离
     - 网络流量的隔离
  
  2. 资源隔离 (Resource Isolation)
     - CPU/Memory资源限制
     - 存储空间配额管理
     - 网络带宽限制
  
  3. 安全隔离 (Security Isolation)
     - RBAC权限控制
     - 网络策略隔离
     - 镜像安全扫描
  
  4. 运维隔离 (Operational Isolation)
     - 日志监控独立
     - 配置管理分离
     - 故障影响范围控制
```

### 1.2 隔离级别模型

```yaml
隔离级别分类:
  Level 1 - 轻度隔离:
    - 基于命名空间的逻辑隔离
    - 基础资源配额限制
    - 标准RBAC权限控制
    - 适用: 开发测试环境
  
  Level 2 - 中度隔离:
    - 网络策略强制隔离
    - 严格的资源配额管理
    - 高级RBAC和安全策略
    - 适用: 准生产环境
  
  Level 3 - 重度隔离:
    - 运行时沙箱隔离
    - 硬件级资源隔离
    - 完整的安全策略执行
    - 适用: 生产环境
  
  Level 4 - 超重度隔离:
    - 完全独立的集群
    - 物理资源完全隔离
    - 专用网络和存储
    - 适用: 合规要求极高的场景
```

## 2. 命名空间与资源配额

### 2.1 命名空间管理策略

```yaml
# 命名空间模板配置
apiVersion: v1
kind: Namespace
metadata:
  name: tenant-a-prod
  labels:
    tenant: tenant-a
    environment: production
    billing-code: TC-001
    security-level: high
  annotations:
    description: "Tenant A production environment"
    owner: "team-tenant-a@example.com"
    budget: "10000"
    quota-profile: "high-performance"
---
# 命名空间自动配置
apiVersion: v1
kind: ConfigMap
metadata:
  name: namespace-defaults
  namespace: kube-system
data:
  namespace-template.yaml: |
    apiVersion: v1
    kind: Namespace
    metadata:
      name: {{TENANT_ID}}-{{ENVIRONMENT}}
      labels:
        tenant: {{TENANT_ID}}
        environment: {{ENVIRONMENT}}
        created-by: "multi-tenant-platform"
      annotations:
        description: "{{TENANT_NAME}} {{ENVIRONMENT}} namespace"
        creation-timestamp: "{{TIMESTAMP}}"
---
# 命名空间初始化作业
apiVersion: batch/v1
kind: Job
metadata:
  name: namespace-initializer
  namespace: kube-system
spec:
  template:
    spec:
      serviceAccountName: namespace-initializer
      containers:
      - name: initializer
        image: busybox:1.35
        command:
        - sh
        - -c
        - |
          # 创建默认资源配额
          kubectl create quota default-quota --hard=cpu=20,memory=50Gi,pods=100,persistentvolumeclaims=20 -n $NAMESPACE
          
          # 创建默认限制范围
          kubectl apply -f - <<EOF
          apiVersion: v1
          kind: LimitRange
          metadata:
            name: default-limits
            namespace: $NAMESPACE
          spec:
            limits:
            - default:
                cpu: 500m
                memory: 1Gi
              defaultRequest:
                cpu: 100m
                memory: 128Mi
              type: Container
            - default:
                storage: 10Gi
              type: PersistentVolumeClaim
          EOF
        env:
        - name: NAMESPACE
          valueFrom:
            fieldRef:
              fieldPath: metadata.namespace
      restartPolicy: Never
  backoffLimit: 4
```

### 2.2 资源配额管理

```yaml
# 高级资源配额配置
apiVersion: v1
kind: ResourceQuota
metadata:
  name: tenant-a-quota
  namespace: tenant-a-prod
spec:
  hard:
    # 计算资源配额
    requests.cpu: "20"
    requests.memory: "50Gi"
    limits.cpu: "40"
    limits.memory: "100Gi"
    
    # 存储资源配额
    requests.storage: "100Gi"
    persistentvolumeclaims: "50"
    
    # 对象数量配额
    pods: "100"
    services: "20"
    secrets: "50"
    configmaps: "30"
    replicationcontrollers: "10"
    resourcequotas: "1"
    services.loadbalancers: "5"
    services.nodeports: "10"
    
    # 自定义资源配额
    "count/deployments.apps": "20"
    "count/statefulsets.apps": "5"
    "count/daemonsets.apps": "3"
    "count/jobs.batch": "15"
    "count/cronjobs.batch": "10"
---
# 配额使用监控配置
apiVersion: v1
kind: ConfigMap
metadata:
  name: quota-monitoring
  namespace: monitoring
data:
  quota-exporter-config.yaml: |
    # 资源配额监控配置
    scrape_configs:
    - job_name: 'resource-quota'
      kubernetes_sd_configs:
      - role: endpoints
      relabel_configs:
      - source_labels: [__meta_kubernetes_namespace]
        target_label: namespace
      - source_labels: [__meta_kubernetes_service_name]
        target_label: service
      metric_relabel_configs:
      - source_labels: [__name__]
        regex: 'kube_resourcequota'
        target_label: __name__
        replacement: 'tenant_resource_quota_usage'
```

## 3. 网络隔离策略

### 3.1 多租户网络策略

```yaml
# 租户间网络隔离策略
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: tenant-isolation
  namespace: tenant-a-prod
spec:
  podSelector: {}
  policyTypes:
  - Ingress
  - Egress
  ingress:
  - from:
    - namespaceSelector:
        matchLabels:
          tenant: tenant-a
      podSelector:
        matchLabels:
          tenant: tenant-a
    ports:
    - protocol: TCP
      port: 80
    - protocol: TCP
      port: 443
    - protocol: TCP
      port: 8080
  egress:
  - to:
    - namespaceSelector:
        matchLabels:
          tenant: tenant-a
      podSelector:
        matchLabels:
          tenant: tenant-a
    ports:
    - protocol: TCP
      port: 80
    - protocol: TCP
      port: 443
    - protocol: TCP
      port: 5432
    - protocol: TCP
      port: 6379
  - to:
    - namespaceSelector:
        matchLabels:
          name: kube-system
    ports:
    - protocol: UDP
      port: 53  # DNS
    - protocol: TCP
      port: 53
---
# 跨租户服务访问策略
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: cross-tenant-access
  namespace: shared-services
spec:
  podSelector:
    matchLabels:
      app: shared-database
  policyTypes:
  - Ingress
  ingress:
  - from:
    - namespaceSelector:
        matchLabels:
          tenant: tenant-a
      podSelector:
        matchLabels:
          allowed-to-access: shared-db
    - namespaceSelector:
        matchLabels:
          tenant: tenant-b
      podSelector:
        matchLabels:
          allowed-to-access: shared-db
    ports:
    - protocol: TCP
      port: 5432
---
# 租户对外访问策略
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: tenant-egress
  namespace: tenant-a-prod
spec:
  podSelector: {}
  policyTypes:
  - Egress
  egress:
  - to:
    - ipBlock:
        cidr: 0.0.0.0/0
      except:
      - 10.0.0.0/8
      - 172.16.0.0/12
      - 192.168.0.0/16
    ports:
    - protocol: TCP
      port: 443
    - protocol: TCP
      port: 80
    - protocol: TCP
      port: 53
      protocol: UDP
```

### 3.2 高级网络隔离

```yaml
# 基于Cilium的高级网络策略
apiVersion: cilium.io/v2
kind: CiliumNetworkPolicy
metadata:
  name: advanced-tenant-isolation
  namespace: tenant-a-prod
spec:
  endpointSelector:
    matchLabels:
      tenant: tenant-a
  ingress:
  - fromEndpoints:
    - matchLabels:
        tenant: tenant-a
        role: frontend
    toPorts:
    - ports:
      - port: "8080"
        protocol: TCP
      rules:
        http:
        - method: "GET"
          path: "/api/v1/.*"
        - method: "POST"
          path: "/api/v1/users"
          headers:
          - "X-API-Key: .*"
  egress:
  - toEndpoints:
    - matchLabels:
        tenant: tenant-a
        role: backend
    toPorts:
    - ports:
      - port: "8080"
        protocol: TCP
  - toCIDR:
    - "10.100.0.0/16"
    toPorts:
    - ports:
      - port: "5432"
        protocol: TCP
---
# 网络策略审计配置
apiVersion: audit.k8s.io/v1
kind: Policy
metadata:
  name: network-policy-audit
rules:
- level: RequestResponse
  resources:
  - group: "networking.k8s.io"
    resources: ["networkpolicies"]
  verbs: ["create", "update", "delete", "patch"]
  omitStages:
  - RequestReceived
```

## 4. RBAC权限管理

### 4.1 租户角色权限设计

```yaml
# 租户管理员角色
apiVersion: rbac.authorization.k8s.io/v1
kind: ClusterRole
metadata:
  name: tenant-admin
rules:
- apiGroups: [""]
  resources: ["pods", "services", "configmaps", "secrets", "persistentvolumeclaims", "serviceaccounts"]
  verbs: ["get", "list", "watch", "create", "update", "patch", "delete"]
- apiGroups: ["apps"]
  resources: ["deployments", "statefulsets", "daemonsets", "replicasets"]
  verbs: ["get", "list", "watch", "create", "update", "patch", "delete"]
- apiGroups: ["batch"]
  resources: ["jobs", "cronjobs"]
  verbs: ["get", "list", "watch", "create", "update", "patch", "delete"]
- apiGroups: ["networking.k8s.io"]
  resources: ["ingresses", "networkpolicies"]
  verbs: ["get", "list", "watch", "create", "update", "patch", "delete"]
- apiGroups: [""]
  resources: ["limitranges", "resourcequotas"]
  verbs: ["get", "list", "watch"]
---
# 租户开发人员角色
apiVersion: rbac.authorization.k8s.io/v1
kind: Role
metadata:
  name: tenant-developer
  namespace: tenant-a-prod
rules:
- apiGroups: [""]
  resources: ["pods", "configmaps", "secrets", "persistentvolumeclaims"]
  verbs: ["get", "list", "watch", "create", "update", "patch"]
- apiGroups: [""]
  resources: ["pods/exec", "pods/portforward"]
  verbs: ["create"]
- apiGroups: ["apps"]
  resources: ["deployments", "replicasets"]
  verbs: ["get", "list", "watch", "create", "update", "patch"]
- apiGroups: ["batch"]
  resources: ["jobs"]
  verbs: ["get", "list", "watch", "create", "update", "patch"]
---
# 租户只读用户角色
apiVersion: rbac.authorization.k8s.io/v1
kind: Role
metadata:
  name: tenant-viewer
  namespace: tenant-a-prod
rules:
- apiGroups: [""]
  resources: ["pods", "services", "configmaps", "secrets", "persistentvolumeclaims"]
  verbs: ["get", "list", "watch"]
- apiGroups: ["apps"]
  resources: ["deployments", "statefulsets", "daemonsets", "replicasets"]
  verbs: ["get", "list", "watch"]
- apiGroups: ["batch"]
  resources: ["jobs", "cronjobs"]
  verbs: ["get", "list", "watch"]
```

### 4.2 权限绑定策略

```yaml
# 租户权限绑定
apiVersion: rbac.authorization.k8s.io/v1
kind: RoleBinding
metadata:
  name: tenant-a-admin
  namespace: tenant-a-prod
subjects:
- kind: User
  name: alice@example.com
  apiGroup: rbac.authorization.k8s.io
roleRef:
  kind: Role
  name: tenant-admin
  apiGroup: rbac.authorization.k8s.io
---
apiVersion: rbac.authorization.k8s.io/v1
kind: RoleBinding
metadata:
  name: tenant-a-developers
  namespace: tenant-a-prod
subjects:
- kind: Group
  name: tenant-a-developers
  apiGroup: rbac.authorization.k8s.io
roleRef:
  kind: Role
  name: tenant-developer
  apiGroup: rbac.authorization.k8s.io
---
apiVersion: rbac.authorization.k8s.io/v1
kind: ClusterRoleBinding
metadata:
  name: tenant-platform-admin
subjects:
- kind: User
  name: platform-admin@example.com
  apiGroup: rbac.authorization.k8s.io
roleRef:
  kind: ClusterRole
  name: cluster-admin
  apiGroup: rbac.authorization.k8s.io
---
# 租户服务账户权限
apiVersion: v1
kind: ServiceAccount
metadata:
  name: tenant-app-sa
  namespace: tenant-a-prod
  annotations:
    iam.amazonaws.com/role: arn:aws:iam::123456789012:role/tenant-app-role
---
apiVersion: rbac.authorization.k8s.io/v1
kind: RoleBinding
metadata:
  name: tenant-app-binding
  namespace: tenant-a-prod
subjects:
- kind: ServiceAccount
  name: tenant-app-sa
  namespace: tenant-a-prod
roleRef:
  kind: Role
  name: tenant-app-role
  apiGroup: rbac.authorization.k8s.io
```

## 5. 安全策略与准入控制

### 5.1 Pod安全策略

```yaml
# 多租户Pod安全策略
apiVersion: v1
kind: ConfigMap
metadata:
  name: tenant-pod-security-policy
  namespace: kube-system
data:
  policy.yaml: |
    # 租户Pod安全基线
    allowedUsers:
      - min: 1000
        max: 65535
    
    forbiddenSysctls:
      - "kernel.*"
      - "net.*"
      - "vm.*"
    
    allowedCapabilities:
      - CHOWN
      - SETUID
      - SETGID
      - DAC_OVERRIDE
      - FOWNER
      - SYS_CHROOT
    
    forbiddenCapabilities:
      - ALL
    
    readOnlyRootFilesystem: true
    privileged: false
    allowPrivilegeEscalation: false
---
# 准入控制器配置
apiVersion: admissionregistration.k8s.io/v1
kind: ValidatingWebhookConfiguration
metadata:
  name: tenant-security-validation
webhooks:
- name: tenant-security.example.com
  rules:
  - apiGroups: [""]
    apiVersions: ["v1"]
    operations: ["CREATE", "UPDATE"]
    resources: ["pods", "deployments", "statefulsets"]
    scope: "Namespaced"
  clientConfig:
    service:
      name: tenant-security-webhook
      namespace: security-system
      path: "/validate"
    caBundle: <CA_BUNDLE_BASE64>
  admissionReviewVersions: ["v1"]
  sideEffects: None
  timeoutSeconds: 10
  failurePolicy: Fail
  matchPolicy: Equivalent
  namespaceSelector:
    matchExpressions:
    - key: tenant
      operator: Exists
---
apiVersion: admissionregistration.k8s.io/v1
kind: MutatingWebhookConfiguration
metadata:
  name: tenant-security-mutation
webhooks:
- name: tenant-security-mutator.example.com
  rules:
  - apiGroups: [""]
    apiVersions: ["v1"]
    operations: ["CREATE"]
    resources: ["pods"]
    scope: "Namespaced"
  clientConfig:
    service:
      name: tenant-security-webhook
      namespace: security-system
      path: "/mutate"
    caBundle: <CA_BUNDLE_BASE64>
  admissionReviewVersions: ["v1"]
  sideEffects: None
  timeoutSeconds: 10
  failurePolicy: Fail
  reinvocationPolicy:IfNeeded
```

### 5.2 策略引擎配置

```yaml
# Open Policy Agent (OPA) 策略
apiVersion: v1
kind: ConfigMap
metadata:
  name: tenant-security-policy
  namespace: opa-system
data:
  tenant.rego: |
    package kubernetes.admission
    
    # 检查租户命名空间标签
    deny[msg] {
        input.request.kind.kind == "Namespace"
        not input.request.object.metadata.labels.tenant
        msg := "命名空间必须包含tenant标签"
    }
    
    # 限制特权容器
    deny[msg] {
        input.request.kind.kind == "Pod"
        input.request.operation == "CREATE"
        container := input.request.object.spec.containers[_]
        container.securityContext.privileged == true
        msg := "不允许创建特权容器"
    }
    
    # 检查资源限制
    deny[msg] {
        input.request.kind.kind == "Pod"
        input.request.operation == "CREATE"
        container := input.request.object.spec.containers[_]
        not container.resources.limits.cpu
        not container.resources.limits.memory
        msg := "容器必须指定资源限制"
    }
    
    # 租户间访问控制
    deny[msg] {
        input.request.kind.kind == "Pod"
        input.request.operation == "CREATE"
        tenant := input.review.namespace.labels.tenant
        target_tenant := input.request.object.spec.containers[_].env[_].value
        target_tenant != tenant
        msg := sprintf("不允许访问其他租户资源: %s", [target_tenant])
    }
---
# Kyverno策略配置
apiVersion: kyverno.io/v1
kind: ClusterPolicy
metadata:
  name: tenant-security-rules
spec:
  validationFailureAction: Enforce
  background: false
  rules:
  - name: require-tenant-label
    match:
      resources:
        kinds:
        - Namespace
    validate:
      message: "命名空间必须包含tenant标签"
      pattern:
        metadata:
          labels:
            tenant: "?*"
  - name: disallow-privileged
    match:
      resources:
        kinds:
        - Pod
    validate:
      message: "不允许创建特权容器"
      pattern:
        spec:
          =(securityContext):
            =(privileged): "false"
          containers:
          - =(securityContext):
              =(privileged): "false"
  - name: require-resource-limits
    match:
      resources:
        kinds:
        - Pod
    validate:
      message: "容器必须指定资源限制"
      pattern:
        spec:
          containers:
          - resources:
              limits:
                cpu: "?*"
                memory: "?*"
```

## 6. vCluster虚拟集群与ValidatingAdmissionPolicy — 2026更新

### 6.1 vCluster虚拟集群多租户

vCluster提供虚拟Kubernetes集群能力，是多租户架构的重要补充，在隔离性和成本之间提供了优秀的折中方案。

```yaml
多租户方案演进:
  | 方案 | 隔离级别 | 成本 | 复杂度 | API兼容 | 适用场景 |
  |------|---------|------|--------|---------|---------|
  | Namespace | 弱(共享API Server) | 低 | 低 | 受限 | 信任度高的团队 |
  | vCluster | 中(虚拟API Server) | 中 | 中 | 完整K8s API | SaaS/开发测试 |
  | 物理集群 | 强(完全隔离) | 高 | 高 | 完整 | 强合规要求 |

vCluster核心价值:
  - 每个租户获得独立K8s API Server和控制平面
  - 租户可自主管理CRD、RBAC、命名空间
  - 底层共享Host集群资源(节点、网络、存储)
  - 创建时间 < 30秒(vs 物理集群5-15分钟)
  - 资源开销: ~256MB内存/vCluster
  
  详见: "[26-vCluster与虚拟集群多租户](./26-kubernetes-vcluster-virtual-cluster-multi-tenancy.md)"
```

### 6.2 ValidatingAdmissionPolicy租户策略

```yaml
# 使用K8s原生VAP实现租户策略强制执行
apiVersion: admissionregistration.k8s.io/v1
kind: ValidatingAdmissionPolicy
metadata:
  name: tenant-resource-limits
spec:
  failurePolicy: Fail
  matchConstraints:
    resourceRules:
      - apiGroups: [""]
        apiVersions: ["v1"]
        operations: ["CREATE", "UPDATE"]
        resources: ["pods"]
  validations:
    - expression: >
        object.spec.containers.all(c,
          has(c.resources) && has(c.resources.limits) &&
          quantity(c.resources.limits.memory).isLessThan(quantity("8Gi")) &&
          quantity(c.resources.limits.cpu).isLessThan(quantity("4"))
        )
      message: "租户Pod资源不能超过单Pod限制(CPU<4, Memory<8Gi)"
---
apiVersion: admissionregistration.k8s.io/v1
kind: ValidatingAdmissionPolicyBinding
metadata:
  name: tenant-resource-limits-binding
spec:
  policyName: tenant-resource-limits
  validationActions: [Deny]
  matchResources:
    namespaceSelector:
      matchLabels:
        tenant-tier: "standard"

多租户策略分层:
  平台级(ANP + VAP):
    - 全局网络隔离(AdminNetworkPolicy)
    - 全局资源限制(ValidatingAdmissionPolicy)
    - 镜像源白名单
    
  租户级(Kyverno/OPA):
    - 租户自定义策略
    - 内部资源配额管理
    - 标签规范强制
    
  详见: "[24-策略即代码与治理自动化](./24-kubernetes-policy-as-code-governance-automation.md)"
```

## 7. 监控与审计

### 6.1 多租户监控架构

```yaml
# 租户监控配置
apiVersion: monitoring.coreos.com/v1
kind: ServiceMonitor
metadata:
  name: tenant-monitoring
  namespace: monitoring
spec:
  selector:
    matchLabels:
      tenant: tenant-a
  namespaceSelector:
    matchNames:
    - tenant-a-prod
    - tenant-a-staging
  endpoints:
  - port: metrics
    path: /metrics
    interval: 30s
    metricRelabelings:
    - sourceLabels: [__name__]
      regex: '(.*)'
      targetLabel: __name__
      replacement: 'tenant_${1}'
    - sourceLabels: [namespace]
      targetLabel: tenant_namespace
---
# 租户资源使用监控
apiVersion: v1
kind: ConfigMap
metadata:
  name: tenant-metrics-config
  namespace: monitoring
data:
  recording-rules.yaml: |
    # 租户资源使用记录规则
    groups:
    - name: tenant-resource-usage
      rules:
      - record: tenant:cpu_usage:sum
        expr: |
          sum by(tenant, namespace) (
            rate(container_cpu_usage_seconds_total[5m])
          )
      
      - record: tenant:memory_usage:sum
        expr: |
          sum by(tenant, namespace) (
            container_memory_working_set_bytes
          )
      
      - record: tenant:storage_usage:sum
        expr: |
          sum by(tenant, namespace) (
            kubelet_volume_stats_used_bytes
          )
      
      - record: tenant:pods_count:sum
        expr: |
          count by(tenant, namespace) (
            kube_pod_info
          )
---
# 租户告警规则
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: tenant-alert-rules
  namespace: monitoring
spec:
  groups:
  - name: tenant.rules
    rules:
    # 资源配额告警
    - alert: TenantQuotaExceeded
      expr: |
        kube_resourcequota{resource="requests.cpu", type="used"}
        /
        kube_resourcequota{resource="requests.cpu", type="hard"}
        > 0.9
      for: 5m
      labels:
        severity: warning
        tenant: "{{ $labels.namespace }}"
      annotations:
        summary: "租户 {{ $labels.namespace }} CPU配额使用超过90%"
        description: "CPU配额使用率达到 {{ $value | humanizePercentage }}"
    
    # 网络隔离告警
    - alert: TenantNetworkViolation
      expr: |
        increase(tenant_network_policy_violations_total[10m]) > 0
      for: 2m
      labels:
        severity: critical
      annotations:
        summary: "检测到租户网络策略违规"
        description: "租户间网络访问违反安全策略"
```

### 6.2 审计日志管理

```yaml
# 审计策略配置
apiVersion: audit.k8s.io/v1
kind: Policy
metadata:
  name: multi-tenant-audit-policy
rules:
# 记录所有租户相关的操作
- level: RequestResponse
  resources:
  - group: ""
    resources: ["namespaces"]
  verbs: ["create", "update", "patch", "delete"]

# 记录资源配额操作
- level: RequestResponse
  resources:
  - group: ""
    resources: ["resourcequotas", "limitranges"]
  verbs: ["create", "update", "patch", "delete"]

# 记录网络策略操作
- level: RequestResponse
  resources:
  - group: "networking.k8s.io"
    resources: ["networkpolicies"]
  verbs: ["create", "update", "patch", "delete"]

# 记录RBAC操作
- level: RequestResponse
  resources:
  - group: "rbac.authorization.k8s.io"
    resources: ["roles", "rolebindings", "clusterroles", "clusterrolebindings"]
  verbs: ["create", "update", "patch", "delete"]

# 记录敏感资源访问
- level: RequestResponse
  resources:
  - group: ""
    resources: ["secrets", "configmaps"]
  verbs: ["get", "list", "watch", "create", "update", "patch", "delete"]
  userGroups: ["system:authenticated"]

# 记录Pod操作
- level: Metadata
  resources:
  - group: ""
    resources: ["pods"]
  verbs: ["create", "update", "patch", "delete"]

# 忽略系统组件操作
- level: None
  users: ["system:kube-proxy", "system:node", "system:serviceaccount:kube-system:*"]
  verbs: ["get", "list", "watch"]
---
# 日志转发配置
apiVersion: v1
kind: ConfigMap
metadata:
  name: audit-log-forwarder
  namespace: logging
data:
  fluent-bit.conf: |
    [SERVICE]
        Flush         1
        Log_Level     info
        Daemon        off
        Parsers_File  parsers.conf
    
    [INPUT]
        Name              tail
        Path              /var/log/kube-apiserver-audit.log
        Parser            json
        Tag               audit.*
        Refresh_Interval  10
    
    [FILTER]
        Name   grep
        Match  audit.*
        Regex  $$.metadata.labels.tenant ^tenant-.*
    
    [OUTPUT]
        Name  es
        Match audit.*
        Host  elasticsearch.logging.svc.cluster.local
        Port  9200
        Index tenant-audit-logs
        Type  _doc

---
*本文档基于大型企业多租户平台的实践经验编写，提供从基础隔离到高级安全控制的完整解决方案。*

*2026-03 更新: 新增vCluster虚拟集群多租户方案、ValidatingAdmissionPolicy原生租户策略；关联文档 [24-策略即代码](./24-kubernetes-policy-as-code-governance-automation.md)、[26-vCluster多租户](./26-kubernetes-vcluster-virtual-cluster-multi-tenancy.md)；版本升级至v1.5。*