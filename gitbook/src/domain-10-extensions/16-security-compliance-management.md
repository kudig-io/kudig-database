# 16 - 安全合规管理 (Security & Compliance Management)

> **适用版本**: v1.25 - v1.32 | **最后更新**: 2026-02 | **参考**: [kubernetes.io/docs/concepts/security](https://kubernetes.io/docs/concepts/security/)

## 零信任安全架构

### 安全架构原则

```
┌─────────────────────────────────────────────────────────────────────────┐
│                           零信任安全架构                                 │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                          │
│  ┌─────────────────┐    ┌─────────────────┐    ┌─────────────────┐     │
│  │   身份认证层     │    │   访问控制层     │    │   数据保护层     │    │
│  │                 │    │                 │    │                 │    │
│  │  OIDC/SAML      │    │  RBAC/PSP       │    │  加密存储        │    │
│  │  多因子认证      │◄──►│  网络策略        │◄──►│  密钥管理        │    │
│  │  服务账户        │    │  安全组          │    │  审计日志        │    │
│  └─────────────────┘    └─────────────────┘    └─────────────────┘     │
│           │                       │                       │              │
│           ▼                       ▼                       ▼              │
│  ┌─────────────────┐    ┌─────────────────┐    ┌─────────────────┐     │
│  │   持续监控层     │    │   威胁检测层     │    │   应急响应层     │    │
│  │                 │    │                 │    │                 │    │
│  │  实时监控        │    │  异常行为检测     │    │  自动化响应      │    │
│  │  告警通知        │    │  入侵检测        │    │  隔离修复        │    │
│  │  合规报告        │    │  漏洞扫描        │    │  取证分析        │    │
│  └─────────────────┘    └─────────────────┘    └─────────────────┘     │
│                                                                          │
└─────────────────────────────────────────────────────────────────────────┘
```

## 身份认证与授权

### OIDC集成配置

```yaml
# Dex OAuth2服务器部署
apiVersion: apps/v1
kind: Deployment
metadata:
  name: dex
  namespace: auth-system
spec:
  replicas: 2
  selector:
    matchLabels:
      app: dex
  template:
    metadata:
      labels:
        app: dex
    spec:
      containers:
      - name: dex
        image: dexidp/dex:v2.37.0
        ports:
        - containerPort: 5556
        args:
        - dex
        - serve
        - /etc/dex/config.yaml
        volumeMounts:
        - name: config
          mountPath: /etc/dex
        - name: tls
          mountPath: /etc/dex/tls
        readinessProbe:
          httpGet:
            path: /.well-known/openid-configuration
            port: 5556
            scheme: HTTPS
        resources:
          requests:
            cpu: 50m
            memory: 64Mi
          limits:
            cpu: 100m
            memory: 128Mi
      volumes:
      - name: config
        configMap:
          name: dex-config
      - name: tls
        secret:
          secretName: dex-tls

---
# Dex配置文件
apiVersion: v1
kind: ConfigMap
metadata:
  name: dex-config
  namespace: auth-system
data:
  config.yaml: |
    issuer: https://dex.example.com
    storage:
      type: kubernetes
      config:
        inCluster: true
    web:
      https: 0.0.0.0:5556
      tlsCert: /etc/dex/tls/tls.crt
      tlsKey: /etc/dex/tls/tls.key
    expiry:
      signingKeys: "6h"
      idTokens: "24h"
      refreshTokens:
        disableRotation: false
        validIfNotUsedFor: "2160h" # 90 days
    logger:
      level: "info"
      format: "json"
    
    staticClients:
    - id: kubernetes
      redirectURIs:
      - 'http://localhost:8000'
      name: 'Kubernetes'
      secret: ZXhhbXBsZS1hcHAtc2VjcmV0
    
    connectors:
    - type: ldap
      name: LDAP
      id: ldap
      config:
        host: ldap.example.com:636
        insecureNoSSL: false
        startTLS: true
        rootCA: /etc/dex/ldap-ca.pem
        bindDN: cn=admin,dc=example,dc=com
        bindPW: password
        usernamePrompt: Email Address
        userSearch:
          baseDN: ou=People,dc=example,dc=com
          filter: "(objectClass=person)"
          username: mail
          idAttr: uid
          emailAttr: mail
          nameAttr: cn
        groupSearch:
          baseDN: ou=Groups,dc=example,dc=com
          filter: "(objectClass=groupOfNames)"
          userMatchers:
          - userAttr: DN
            groupAttr: member
          nameAttr: cn
    
    - type: github
      id: github
      name: GitHub
      config:
        clientID: abc123
        clientSecret: abc123
        redirectURI: https://dex.example.com/callback
        orgs:
        - name: your-org
```

### Kubernetes RBAC配置

```yaml
# 精细化RBAC权限管理
apiVersion: rbac.authorization.k8s.io/v1
kind: ClusterRole
metadata:
  name: platform-admin
rules:
# 核心资源管理
- apiGroups: [""]
  resources: ["nodes", "namespaces", "persistentvolumes"]
  verbs: ["get", "list", "watch", "create", "update", "patch", "delete"]
# 工作负载管理
- apiGroups: ["apps"]
  resources: ["deployments", "statefulsets", "daemonsets", "replicasets"]
  verbs: ["get", "list", "watch", "create", "update", "patch", "delete"]
# 网络策略管理
- apiGroups: ["networking.k8s.io"]
  resources: ["networkpolicies", "ingresses"]
  verbs: ["get", "list", "watch", "create", "update", "patch", "delete"]
# 安全策略管理
- apiGroups: ["policy"]
  resources: ["podsecuritypolicies", "poddisruptionbudgets"]
  verbs: ["get", "list", "watch", "create", "update", "patch", "delete"]
# 存储管理
- apiGroups: [""]
  resources: ["persistentvolumeclaims", "storageclasses"]
  verbs: ["get", "list", "watch", "create", "update", "patch", "delete"]
# 监控和日志访问
- apiGroups: [""]
  resources: ["events", "pods/log"]
  verbs: ["get", "list", "watch"]

---
# 命名空间级管理员角色
apiVersion: rbac.authorization.k8s.io/v1
kind: Role
metadata:
  name: namespace-admin
  namespace: production
rules:
- apiGroups: [""]
  resources: ["pods", "services", "configmaps", "secrets", "persistentvolumeclaims"]
  verbs: ["get", "list", "watch", "create", "update", "patch", "delete"]
- apiGroups: ["apps"]
  resources: ["deployments", "statefulsets", "daemonsets"]
  verbs: ["get", "list", "watch", "create", "update", "patch", "delete"]
- apiGroups: ["batch"]
  resources: ["jobs", "cronjobs"]
  verbs: ["get", "list", "watch", "create", "update", "patch", "delete"]
- apiGroups: ["networking.k8s.io"]
  resources: ["networkpolicies", "ingresses"]
  verbs: ["get", "list", "watch", "create", "update", "patch", "delete"]

---
# 角色绑定配置
apiVersion: rbac.authorization.k8s.io/v1
kind: RoleBinding
metadata:
  name: team-a-admins
  namespace: production
roleRef:
  apiGroup: rbac.authorization.k8s.io
  kind: Role
  name: namespace-admin
subjects:
- kind: Group
  name: team-a-admins
  apiGroup: rbac.authorization.k8s.io
- kind: ServiceAccount
  name: ci-cd-bot
  namespace: cicd

---
# 集群角色绑定
apiVersion: rbac.authorization.k8s.io/v1
kind: ClusterRoleBinding
metadata:
  name: platform-admins
roleRef:
  apiGroup: rbac.authorization.k8s.io
  kind: ClusterRole
  name: platform-admin
subjects:
- kind: Group
  name: platform-admins
  apiGroup: rbac.authorization.k8s.io
```

## 网络安全策略

### NetworkPolicy配置

```yaml
# 默认拒绝所有流量
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: default-deny-all
  namespace: production
spec:
  podSelector: {}
  policyTypes:
  - Ingress
  - Egress

---
# 允许内部服务通信
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: allow-internal-traffic
  namespace: production
spec:
  podSelector:
    matchLabels:
      app: web-app
  policyTypes:
  - Ingress
  ingress:
  - from:
    - podSelector:
        matchLabels:
          app: api-gateway
    - namespaceSelector:
        matchLabels:
          name: monitoring
    ports:
    - protocol: TCP
      port: 8080
  egress:
  - to:
    - podSelector:
        matchLabels:
          app: database
    ports:
    - protocol: TCP
      port: 3306

---
# 允许外部访问API网关
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: allow-external-api
  namespace: production
spec:
  podSelector:
    matchLabels:
      app: api-gateway
  policyTypes:
  - Ingress
  ingress:
  - from:
    - ipBlock:
        cidr: 0.0.0.0/0
    ports:
    - protocol: TCP
      port: 443
    - protocol: TCP
      port: 80
```

### Cilium网络安全策略

```yaml
# Cilium网络安全策略
apiVersion: cilium.io/v2
kind: CiliumNetworkPolicy
metadata:
  name: enhanced-security-policy
  namespace: production
spec:
  endpointSelector:
    matchLabels:
      app: secure-app
  ingress:
  - fromEndpoints:
    - matchLabels:
        app: trusted-client
    toPorts:
    - ports:
      - port: "8080"
        protocol: TCP
      rules:
        http:
        - method: "GET"
          path: "/api/*"
        - method: "POST"
          path: "/api/users"
  egress:
  - toEntities:
    - world
    toPorts:
    - ports:
      - port: "443"
        protocol: TCP
  - toServices:
    - k8sService:
        serviceName: database
        namespace: database
    toPorts:
    - ports:
      - port: "5432"
        protocol: TCP
```

## Pod安全策略

### Pod Security Standards

```yaml
# Pod安全标准配置
apiVersion: v1
kind: Namespace
metadata:
  name: production-secure
  labels:
    pod-security.kubernetes.io/enforce: restricted
    pod-security.kubernetes.io/enforce-version: latest
    pod-security.kubernetes.io/audit: restricted
    pod-security.kubernetes.io/warn: baseline

---
# Pod安全上下文配置
apiVersion: v1
kind: Pod
metadata:
  name: secure-application
  namespace: production-secure
spec:
  securityContext:
    runAsNonRoot: true
    runAsUser: 1000
    runAsGroup: 3000
    fsGroup: 2000
    seccompProfile:
      type: RuntimeDefault
  containers:
  - name: app
    image: myapp:latest
    securityContext:
      allowPrivilegeEscalation: false
      readOnlyRootFilesystem: true
      capabilities:
        drop:
        - ALL
        add:
        - NET_BIND_SERVICE
      runAsNonRoot: true
      runAsUser: 1000
    volumeMounts:
    - name: tmp-volume
      mountPath: /tmp
    - name: logs-volume
      mountPath: /var/log
  volumes:
  - name: tmp-volume
    emptyDir: {}
  - name: logs-volume
    emptyDir: {}
```

### Kyverno策略引擎

```yaml
# Kyverno安全策略
apiVersion: kyverno.io/v1
kind: ClusterPolicy
metadata:
  name: require-pod-security-standards
spec:
  validationFailureAction: enforce
  background: true
  rules:
  - name: require-run-as-non-root
    match:
      resources:
        kinds:
        - Pod
    validate:
      message: "容器必须以非root用户运行"
      pattern:
        spec:
          containers:
          - securityContext:
              runAsNonRoot: true
              
  - name: require-drop-all-capabilities
    match:
      resources:
        kinds:
        - Pod
    validate:
      message: "容器必须放弃所有不必要的能力"
      pattern:
        spec:
          containers:
          - securityContext:
              capabilities:
                drop: ["ALL"]
                
  - name: require-read-only-root-filesystem
    match:
      resources:
        kinds:
        - Pod
    validate:
      message: "容器必须使用只读根文件系统"
      pattern:
        spec:
          containers:
          - securityContext:
              readOnlyRootFilesystem: true

---
# 镜像漏洞扫描策略
apiVersion: kyverno.io/v1
kind: ClusterPolicy
metadata:
  name: scan-images-for-vulnerabilities
spec:
  validationFailureAction: audit
  background: true
  rules:
  - name: check-image-vulnerabilities
    match:
      resources:
        kinds:
        - Pod
    context:
    - name: vulnerabilityReport
      apiCall:
        urlPath: "/api/v1/namespaces/{{request.namespace}}/vulnerabilityreports/aqua-image-vuln-{{request.object.metadata.name}}"
    validate:
      message: "镜像包含严重安全漏洞"
      deny:
        conditions:
        - key: "{{ vulnerabilityReport.report.summary.criticalCount }}"
          operator: GreaterThan
          value: "0"
```

## 密钥与配置管理

### Sealed Secrets加密配置

```yaml
# SealedSecret加密配置
apiVersion: bitnami.com/v1alpha1
kind: SealedSecret
metadata:
  name: database-credentials
  namespace: production
spec:
  encryptedData:
    username: AgBy3i4OJSWK+PiTySYZZA9rO43cGDEq.....
    password: B1iGhsDQCtbEQVAefE+bqhvbR1QEXiN6......
  template:
    metadata:
      name: database-credentials
      namespace: production
    type: Opaque

---
# 外部密钥管理系统集成
apiVersion: external-secrets.io/v1beta1
kind: ExternalSecret
metadata:
  name: aws-credentials
  namespace: production
spec:
  secretStoreRef:
    name: aws-parameter-store
    kind: ClusterSecretStore
  target:
    name: aws-credentials
  data:
  - secretKey: access-key-id
    remoteRef:
      key: /prod/aws/access-key-id
  - secretKey: secret-access-key
    remoteRef:
      key: /prod/aws/secret-access-key
```

### HashiCorp Vault集成

```yaml
# Vault Agent Injector配置
apiVersion: apps/v1
kind: Deployment
metadata:
  name: vault-enabled-app
  namespace: production
spec:
  replicas: 3
  selector:
    matchLabels:
      app: vault-app
  template:
    metadata:
      annotations:
        vault.hashicorp.com/agent-inject: "true"
        vault.hashicorp.com/role: "app-role"
        vault.hashicorp.com/agent-inject-secret-database-config: "database/creds/app-role"
        vault.hashicorp.com/agent-inject-template-database-config: |
          {
            "username": "{{ .Data.username }}",
            "password": "{{ .Data.password }}"
          }
      labels:
        app: vault-app
    spec:
      serviceAccountName: vault-app
      containers:
      - name: app
        image: myapp:latest
        env:
        - name: DB_CREDENTIALS_FILE
          value: "/vault/secrets/database-config"
        volumeMounts:
        - name: vault-secrets
          mountPath: /vault/secrets
      volumes:
      - name: vault-secrets
        emptyDir: {}
```

## 审计与合规

### Kubernetes审计配置

```yaml
# 审计策略配置
apiVersion: audit.k8s.io/v1
kind: Policy
rules:
# 记录认证相关事件
- level: Metadata
  resources:
  - group: authentication.k8s.io
    resources: ["tokenreviews", "subjectaccessreviews"]
  verbs: ["create"]

# 记录修改核心资源的请求
- level: RequestResponse
  resources:
  - group: ""
    resources: ["pods", "services", "configmaps", "secrets", "persistentvolumes"]
  - group: "apps"
    resources: ["deployments", "statefulsets", "daemonsets"]
  verbs: ["create", "update", "delete", "patch"]

# 记录RBAC变更
- level: RequestResponse
  resources:
  - group: "rbac.authorization.k8s.io"
    resources: ["roles", "rolebindings", "clusterroles", "clusterrolebindings"]
  verbs: ["create", "update", "delete", "patch"]

# 记录网络策略变更
- level: RequestResponse
  resources:
  - group: "networking.k8s.io"
    resources: ["networkpolicies"]
  verbs: ["create", "update", "delete", "patch"]

# 忽略健康检查和只读操作
- level: None
  users: ["system:kube-proxy", "system:node"]
  verbs: ["watch", "get", "list"]
  nonResourceURLs:
  - "/healthz*"
  - "/version"

---
# APIServer审计配置
apiVersion: v1
kind: Pod
metadata:
  name: kube-apiserver
  namespace: kube-system
spec:
  containers:
  - name: kube-apiserver
    image: k8s.gcr.io/kube-apiserver:v1.28.0
    command:
    - kube-apiserver
    - --audit-policy-file=/etc/kubernetes/audit/policy.yaml
    - --audit-log-path=/var/log/kubernetes/audit.log
    - --audit-log-maxage=30
    - --audit-log-maxbackup=10
    - --audit-log-maxsize=100
    - --audit-log-format=json
    volumeMounts:
    - name: audit-policy
      mountPath: /etc/kubernetes/audit
    - name: audit-log
      mountPath: /var/log/kubernetes
  volumes:
  - name: audit-policy
    configMap:
      name: audit-policy
  - name: audit-log
    hostPath:
      path: /var/log/kubernetes
      type: DirectoryOrCreate
```

### 合规检查工具

```yaml
# kube-bench安全基准检查
apiVersion: batch/v1
kind: Job
metadata:
  name: kube-bench-master
  namespace: security-audit
spec:
  template:
    spec:
      hostPID: true
      containers:
      - name: kube-bench
        image: aquasec/kube-bench:0.6.12
        command: ["kube-bench", "master"]
        volumeMounts:
        - name: var-lib-etcd
          mountPath: /var/lib/etcd
          readOnly: true
        - name: etc-kubernetes
          mountPath: /etc/kubernetes
          readOnly: true
        - name: etc-systemd
          mountPath: /etc/systemd
          readOnly: true
      restartPolicy: Never
      volumes:
      - name: var-lib-etcd
        hostPath:
          path: "/var/lib/etcd"
      - name: etc-kubernetes
        hostPath:
          path: "/etc/kubernetes"
      - name: etc-systemd
        hostPath:
          path: "/etc/systemd"

---
# CIS基准检查配置
apiVersion: v1
kind: ConfigMap
metadata:
  name: cis-benchmark-config
  namespace: security-audit
data:
  config.yaml: |
    checks:
    - id: 1.1.1
      description: "Ensure that the API server pod specification file permissions are set to 644 or more restrictive"
      remediation: "Run the below command on the master node: chmod 644 /etc/kubernetes/manifests/kube-apiserver.yaml"
      scored: true
      
    - id: 1.1.2
      description: "Ensure that the API server pod specification file ownership is set to root:root"
      remediation: "Run the below command on the master node: chown root:root /etc/kubernetes/manifests/kube-apiserver.yaml"
      scored: true
      
    - id: 1.2.1
      description: "Ensure that the --anonymous-auth argument is set to false"
      remediation: "Edit the API server pod specification file /etc/kubernetes/manifests/kube-apiserver.yaml and set the below parameter: --anonymous-auth=false"
      scored: true
```

## 威胁检测与响应

### Falco实时威胁检测

```yaml
# Falco部署配置
apiVersion: apps/v1
kind: DaemonSet
metadata:
  name: falco
  namespace: security-monitoring
spec:
  selector:
    matchLabels:
      app: falco
  template:
    metadata:
      labels:
        app: falco
    spec:
      containers:
      - name: falco
        image: falcosecurity/falco:0.36.2
        securityContext:
          privileged: true
        env:
        - name: FALCO_BPF_PROBE
          value: ""
        - name: DROP_PRIVILEGES
          value: "false"
        volumeMounts:
        - mountPath: /host/var/run/docker.sock
          name: docker-socket
        - mountPath: /host/dev
          name: dev-fs
        - mountPath: /host/proc
          name: proc-fs
        - mountPath: /host/boot
          name: boot-fs
        - mountPath: /host/lib/modules
          name: lib-modules
        - mountPath: /host/usr
          name: usr-fs
        - mountPath: /host/etc
          name: etc-fs
        - mountPath: /etc/falco
          name: config-volume
      volumes:
      - name: docker-socket
        hostPath:
          path: /var/run/docker.sock
      - name: dev-fs
        hostPath:
          path: /dev
      - name: proc-fs
        hostPath:
          path: /proc
      - name: boot-fs
        hostPath:
          path: /boot
      - name: lib-modules
        hostPath:
          path: /lib/modules
      - name: usr-fs
        hostPath:
          path: /usr
      - name: etc-fs
        hostPath:
          path: /etc
      - name: config-volume
        configMap:
          name: falco-config

---
# Falco规则配置
apiVersion: v1
kind: ConfigMap
metadata:
  name: falco-config
  namespace: security-monitoring
data:
  falco.yaml: |
    log_level: info
    priority: debug
    buffer_dim: 8388608
    outputs:
      rate: 10
      max_burst: 50
    json_output: true
    json_include_output_property: true
    stdout_output:
      enabled: true
    syslog_output:
      enabled: true
    program_output:
      enabled: false
    http_output:
      enabled: true
      url: http://falco-exporter.security-monitoring.svc:9095
      
  falco_rules.yaml: |
    - rule: Terminal shell in container
      desc: A shell was used as the entrypoint/exec point into a container with an attached terminal.
      condition: >
        spawned_process and container
        and shell_procs and proc.tty != 0
        and container_entrypoint
      output: >
        A shell was spawned in a container with an attached terminal (user=%user.name %container.info
        shell=%proc.name parent=%proc.pname cmdline=%proc.cmdline terminal=%proc.tty container_id=%container.id image=%container.image.repository:%container.image.tag)
      priority: NOTICE
      tags: [container, shell, mitre_execution]

    - rule: Contact K8S API Server From Container
      desc: Detect attempts to contact the K8s API Server from a container
      condition: outbound and k8s_api_server and container
      output: Detected outbound connection to K8s API Server (command=%proc.cmdline pid=%proc.pid connection=%fd.name user=%user.name container_id=%container.id image=%container.image.repository)
      priority: WARNING
      tags: [network, k8s, mitre_discovery]
```

### 安全事件响应

```yaml
# 自动化安全响应配置
apiVersion: apps/v1
kind: Deployment
metadata:
  name: security-response-automation
  namespace: security-operations
spec:
  replicas: 1
  selector:
    matchLabels:
      app: security-response
  template:
    metadata:
      labels:
        app: security-response
    spec:
      containers:
      - name: responder
        image: security/responder:latest
        env:
        - name: ALERT_WEBHOOK_URL
          value: "http://alertmanager.monitoring.svc:9093/api/v2/alerts"
        - name: INCIDENT_CHANNEL
          value: "#security-incidents"
        - name: QUARANTINE_NAMESPACE
          value: "quarantine"
        volumeMounts:
        - name: rules-config
          mountPath: /etc/responder/rules
      volumes:
      - name: rules-config
        configMap:
          name: response-rules

---
# 响应规则配置
apiVersion: v1
kind: ConfigMap
metadata:
  name: response-rules
  namespace: security-operations
data:
  rules.yaml: |
    response_rules:
    - name: high_privilege_container
      trigger:
        alert_name: "HighPrivilegeContainer"
        severity: "critical"
      actions:
      - type: "container_kill"
        target: "pod"
        conditions:
          privileged: true
      - type: "namespace_quarantine"
        target: "namespace"
        grace_period: "5m"
      - type: "slack_notification"
        channel: "#security-alerts"
        message_template: "高权限容器被终止: {{ .PodName }} in {{ .Namespace }}"
        
    - name: unauthorized_access
      trigger:
        alert_name: "UnauthorizedAccess"
        severity: "warning"
      actions:
      - type: "network_isolate"
        target: "pod"
        duration: "30m"
      - type: "log_collection"
        target: "pod"
        collectors: ["logs", "processes", "network_connections"]
      - type: "email_notification"
        recipients: ["security-team@example.com"]
        subject: "未授权访问检测"
```

## 合规性检查清单

### 生产环境安全基线

```bash
#!/bin/bash
# security-baseline-check.sh

echo "=== Kubernetes安全基线检查 ==="

# 1. API Server安全配置检查
echo "1. 检查API Server安全配置:"
kubectl get pod -n kube-system -l component=kube-apiserver -o jsonpath='{.items[0].spec.containers[0].command}' | \
  grep -E "(--anonymous-auth=false|--authorization-mode=RBAC|--profiling=false)"

# 2. etcd安全检查
echo "2. 检查etcd安全配置:"
kubectl get pod -n kube-system -l component=etcd -o jsonpath='{.items[0].spec.containers[0].command}' | \
  grep -E "(--client-cert-auth=true|--auto-tls=false)"

# 3. 网络策略检查
echo "3. 检查网络策略:"
kubectl get networkpolicy --all-namespaces | wc -l

# 4. Pod安全策略检查
echo "4. 检查Pod安全策略:"
kubectl get podsecuritypolicy | wc -l

# 5. RBAC配置检查
echo "5. 检查RBAC配置:"
kubectl get clusterrolebinding | grep -E "(cluster-admin|system:masters)"

# 6. 密钥管理检查
echo "6. 检查密钥管理:"
kubectl get secrets --all-namespaces | grep -c "Opaque"

# 7. 审计日志检查
echo "7. 检查审计日志配置:"
kubectl get pod -n kube-system -l component=kube-apiserver -o jsonpath='{.items[0].spec.containers[0].command}' | \
  grep -E "(--audit-log-path|--audit-policy-file)"

echo "=== 安全检查完成 ==="
```

### GDPR合规检查

```yaml
# 数据保护合规配置
apiVersion: v1
kind: ConfigMap
metadata:
  name: gdpr-compliance-config
  namespace: compliance
data:
  data-protection-policy.yaml: |
    personal_data_handling:
      data_minimization: true
      purpose_limitation: true
      storage_limitation: "2592000"  # 30天
      data_portability: true
      right_to_erasure: true
      
    consent_management:
      explicit_consent_required: true
      consent_logging: true
      consent_withdrawal: true
      
    data_breach_notification:
      notification_threshold: 1000  # 影响1000个用户以上
      notification_timeline: "72h"  # 72小时内通知
      authorities_to_notify:
        - "supervisory-authority@example.com"
      
    privacy_by_design:
      data_encryption_at_rest: true
      data_encryption_in_transit: true
      pseudonymization: true
      access_controls: true
```

---

**表格底部标记**: Kusheet Project, 作者 Allen Galler (allengaller@gmail.com)