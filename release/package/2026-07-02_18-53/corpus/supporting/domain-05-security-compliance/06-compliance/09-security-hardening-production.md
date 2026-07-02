---
title: Kubernetes 安全加固
description: '# Kubernetes 安全加固'
summary: 'kubectl get pods -A -o json | jq -r '.items[] | select(.spec.containers[].securityContext.privileged==true) | .metadata.namespace + "/" + .metadata.name''
category: security
tags:
- k8s
- security
- rbac
- authentication
- authorization
- etcd
- apiserver
- kubelet
- scheduler
- containerd
tier: supporting
created: '2026-05-23'
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- 安全工程师
- SRE
- 架构师
estimated_read_time: 5min
intent_queries:
- Kubernetes 安全加固 是什么
- 如何 Kubernetes 安全加固
- Kubernetes 7 security 最佳实践
trigger_keywords:
- Kubernetes
- 安全加固
- security
prerequisites:
- kubectl-basics
- rbac-basics
- etcd-basics
- policy-basics
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
authors:
- name: KUDIG Team
  role: contributor
cross_refs:
- type: domain
  path: ../domain-01-cluster-fundamentals/
  label: '相关知识域: domain-01-cluster-fundamentals'
- type: domain
  path: ../domain-06-observability/
  label: '相关知识域: domain-06-observability'
- type: cheatsheet
  path: ../domain-17-system-foundation/topic-cheat-sheet/tls-pki.md
  label: '速查卡: tls-pki'
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# [[Kubernetes|Kubernetes]] 安全加固

> Kubernetes 版本: v1.25 - v1.32 | 适用环境: 生产集群

<!-- chunk: 安全加固层级 -->
## 安全加固层级

| 层级 | 加固内容 | 优先级 |
|------|----------|--------|
| 集群 | API Server、[[etcd|etcd]]、[[kubelet|kubelet]] | 高 |
| 网络 | [[NetworkPolicy|NetworkPolicy]]、mTLS、出口控制 | 高 |
| 工作负载 | Pod 安全、镜像安全 | 高 |
| 数据 | Secret 加密、RBAC | 高 |
| 运行时 | 沙箱、安全上下文 | 中 |
| 审计 | 日志、监控、告警 | 中 |

<!-- chunk: API Server 安全加固 -->
## API Server 安全加固

```yaml
# kube-apiserver 安全配置
apiVersion: v1
kind: Pod
metadata:
  name: kube-apiserver
  namespace: kube-system
spec:
  containers:
  - name: kube-apiserver
    command:
    - kube-apiserver
    # 认证
    - --anonymous-auth=false
    - --authentication-token-webhook-cache-ttl=10s
    # 授权
    - --authorization-mode=Node,RBAC
    # 准入控制
    - --enable-admission-plugins=NodeRestriction,PodSecurity,ServiceAccount
    - --disable-admission-plugins=DefaultStorageClass
    # TLS
    - --tls-min-version=VersionTLS12
    - --tls-cipher-suites=TLS_ECDHE_RSA_WITH_AES_128_GCM_SHA256,TLS_ECDHE_RSA_WITH_AES_256_GCM_SHA384
    # API 限制
    - --max-requests-inflight=400
    - --max-mutating-requests-inflight=200
    # 审计
    - --audit-log-path=/var/log/kubernetes/audit/audit.log
    - --audit-policy-file=/etc/kubernetes/audit/policy.yaml
    - --audit-log-maxage=30
    - --audit-log-maxbackup=10
    - --audit-log-maxsize=100
    # 加密
    - --encryption-provider-config=/etc/kubernetes/encryption/config.yaml
    # Profiling
    - --profiling=false
```

<!-- chunk: etcd 安全加固 -->
## etcd 安全加固

```yaml
# etcd 安全配置
apiVersion: v1
kind: Pod
metadata:
  name: etcd
  namespace: kube-system
spec:
  containers:
  - name: etcd
    command:
    - etcd
    # TLS 客户端
    - --client-cert-auth=true
    - --cert-file=/etc/kubernetes/pki/etcd/server.crt
    - --key-file=/etc/kubernetes/pki/etcd/server.key
    - --trusted-ca-file=/etc/kubernetes/pki/etcd/ca.crt
    # TLS 对等
    - --peer-client-cert-auth=true
    - --peer-cert-file=/etc/kubernetes/pki/etcd/peer.crt
    - --peer-key-file=/etc/kubernetes/pki/etcd/peer.key
    - --peer-trusted-ca-file=/etc/kubernetes/pki/etcd/ca.crt
    # 加密
    - --cipher-suites=TLS_ECDHE_RSA_WITH_AES_128_GCM_SHA256,TLS_ECDHE_RSA_WITH_AES_256_GCM_SHA384
```

<!-- chunk: kubelet 安全加固 -->
## kubelet 安全加固

```yaml
# kubelet 配置
apiVersion: kubelet.config.k8s.io/v1beta1
kind: KubeletConfiguration
# 认证
authentication:
  anonymous:
    enabled: false
  webhook:
    enabled: true
    cacheTTL: 2m0s
  x509:
    clientCAFile: /etc/kubernetes/pki/ca.crt
# 授权
authorization:
  mode: Webhook
  webhook:
    cacheAuthorizedTTL: 5m0s
    cacheUnauthorizedTTL: 30s
# TLS
tlsCipherSuites:
- TLS_ECDHE_RSA_WITH_AES_128_GCM_SHA256
- TLS_ECDHE_RSA_WITH_AES_256_GCM_SHA384
tlsMinVersion: VersionTLS12
# 安全选项
readOnlyPort: 0
protectKernelDefaults: true
makeIPTablesUtilChains: true
eventRecordQPS: 5
rotateCertificates: true
serverTLSBootstrap: true
```

<!-- chunk: Secret 加密配置 -->
## Secret 加密配置

```yaml
# /etc/kubernetes/encryption/config.yaml
apiVersion: apiserver.config.k8s.io/v1
kind: EncryptionConfiguration
resources:
- resources:
  - secrets
  - configmaps
  providers:
  # KMS 加密 (推荐)
  - kms:
      apiVersion: v2
      name: alicloud-kms
      endpoint: unix:///var/run/kmsplugin/socket.sock
      timeout: 3s
  # 或使用 AES-CBC
  - aescbc:
      keys:
      - name: key1
        secret: <BASE64_ENCODED_32_BYTE_KEY>
  - identity: {}
```

<!-- chunk: Pod 安全加固 -->
## Pod 安全加固

```yaml
# 安全加固 Pod 示例
apiVersion: v1
kind: Pod
metadata:
  name: secure-pod
spec:
  securityContext:
    runAsNonRoot: true
    runAsUser: 1000
    runAsGroup: 1000
    fsGroup: 1000
    seccompProfile:
      type: RuntimeDefault
  containers:
  - name: app
    image: myapp:v1
    securityContext:
      allowPrivilegeEscalation: false
      readOnlyRootFilesystem: true
      capabilities:
        drop:
        - ALL
      privileged: false
    resources:
      limits:
        cpu: 1
        memory: 512Mi
      requests:
        cpu: 100m
        memory: 128Mi
    volumeMounts:
    - name: tmp
      mountPath: /tmp
    - name: cache
      mountPath: /var/cache
  volumes:
  - name: tmp
    emptyDir: {}
  - name: cache
    emptyDir: {}
  automountServiceAccountToken: false
```

<!-- chunk: NetworkPolicy 加固 -->
## NetworkPolicy 加固

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
# 仅允许必要流量
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: allow-app-traffic
  namespace: production
spec:
  podSelector:
    matchLabels:
      app: web
  policyTypes:
  - Ingress
  - Egress
  ingress:
  - from:
    - namespaceSelector:
        matchLabels:
          name: ingress-nginx
      podSelector:
        matchLabels:
          app: nginx-ingress
    ports:
    - port: 8080
      protocol: TCP
  egress:
  - to:
    - namespaceSelector:
        matchLabels:
          name: production
      podSelector:
        matchLabels:
          app: database
    ports:
    - port: 5432
  - to:  # DNS
    - namespaceSelector: {}
      podSelector:
        matchLabels:
          k8s-app: kube-dns
    ports:
    - port: 53
      protocol: UDP
```

<!-- chunk: RBAC 最小权限 -->
## RBAC 最小权限

```yaml
# 只读角色
apiVersion: rbac.authorization.k8s.io/v1
kind: Role
metadata:
  name: app-reader
  namespace: production
rules:
- apiGroups: [""]
  resources: ["pods", "services", "configmaps"]
  verbs: ["get", "list", "watch"]
- apiGroups: ["apps"]
  resources: ["deployments", "replicasets"]
  verbs: ["get", "list", "watch"]
---
# 应用部署角色 (最小权限)
apiVersion: rbac.authorization.k8s.io/v1
kind: Role
metadata:
  name: app-deployer
  namespace: production
rules:
- apiGroups: ["apps"]
  resources: ["deployments"]
  verbs: ["get", "list", "watch", "update", "patch"]
  resourceNames: ["my-app"]  # 限制具体资源
- apiGroups: [""]
  resources: ["configmaps", "secrets"]
  verbs: ["get", "list"]
  resourceNames: ["my-app-config", "my-app-secret"]
```

<!-- chunk: 镜像安全策略 -->
## 镜像安全策略

```yaml
# Kyverno 镜像策略
apiVersion: kyverno.io/v1
kind: ClusterPolicy
metadata:
  name: require-signed-images
spec:
  validationFailureAction: Enforce
  background: false
  rules:
  - name: verify-signature
    match:
      any:
      - resources:
          kinds:
          - Pod
    verifyImages:
    - imageReferences:
      - "registry.example.com/*"
      attestors:
      - count: 1
        entries:
        - keys:
            publicKeys: |
              -----BEGIN PUBLIC KEY-----
              ...
              -----END PUBLIC KEY-----
---
# 禁止使用 latest 标签
apiVersion: kyverno.io/v1
kind: ClusterPolicy
metadata:
  name: disallow-latest-tag
spec:
  validationFailureAction: Enforce
  rules:
  - name: require-image-tag
    match:
      any:
      - resources:
          kinds:
          - Pod
    validate:
      message: "镜像必须指定非 latest 标签"
      pattern:
        spec:
          containers:
          - image: "!*:latest"
```

<!-- chunk: 安全审计检查 -->
## 安全审计检查

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
#!/bin/bash
# 安全审计脚本

echo "=== Kubernetes 安全审计 ==="

# 1. 检查匿名访问
echo "1. 检查匿名访问..."
kubectl auth can-i --list --as=system:anonymous

# 2. 检查特权容器
echo "2. 检查特权容器..."
kubectl get pods -A -o json | jq -r '.items[] | select(.spec.containers[].securityContext.privileged==true) | .metadata.namespace + "/" + .metadata.name'

# 3. 检查 hostNetwork/hostPID/hostIPC
echo "3. 检查 host 命名空间使用..."
kubectl get pods -A -o json | jq -r '.items[] | select(.spec.hostNetwork==true or .spec.hostPID==true or .spec.hostIPC==true) | .metadata.namespace + "/" + .metadata.name'

# 4. 检查以 root 运行的容器
echo "4. 检查 root 运行..."
kubectl get pods -A -o json | jq -r '.items[] | select(.spec.securityContext.runAsNonRoot!=true) | .metadata.namespace + "/" + .metadata.name'

# 5. 检查未设置资源限制
echo "5. 检查资源限制..."
kubectl get pods -A -o json | jq -r '.items[] | select(.spec.containers[].resources.limits==null) | .metadata.namespace + "/" + .metadata.name'

# 6. 检查 cluster-admin 绑定
echo "6. 检查 cluster-admin 绑定..."
kubectl get clusterrolebindings -o json | jq -r '.items[] | select(.roleRef.name=="cluster-admin") | .metadata.name + ": " + (.subjects[]?.name // "unknown")'

# 7. 检查 Secret 挂载
echo "7. 检查自动挂载 ServiceAccount Token..."
kubectl get pods -A -o json | jq -r '.items[] | select(.spec.automountServiceAccountToken!=false) | .metadata.namespace + "/" + .metadata.name' | head -20
```
<!-- chunk: 安全监控告警 -->
## 安全监控告警

```yaml
groups:
- name: security
  rules:
  - alert: PrivilegedContainerDetected
    expr: |
      sum(kube_pod_container_info{container!=""}) by (namespace, pod, container)
      * on(namespace, pod) group_left
      (kube_pod_status_phase{phase="Running"} == 1)
    for: 1m
    labels:
      severity: critical
    annotations:
      summary: "检测到特权容器运行"
      
  - alert: ClusterAdminBindingCreated
    expr: |
      increase(apiserver_audit_event_total{verb="create",objectRef_resource="clusterrolebindings"}[5m]) > 0
    for: 0m
    labels:
      severity: warning
    annotations:
      summary: "新建 ClusterRoleBinding"
      
  - alert: SecretAccessAnomaly
    expr: |
      sum(rate(apiserver_audit_event_total{verb="get",objectRef_resource="secrets"}[5m])) by (user_username) > 100
    for: 5m
    labels:
      severity: warning
    annotations:
      summary: "用户 {{ $labels.user_username }} Secret 访问异常"
```

<!-- chunk: CIS Benchmark 检查 -->
## CIS Benchmark 检查

| 检查项 | 级别 | 说明 |
|--------|------|------|
| 1.1 Master Node | L1/L2 | API Server、Controller Manager、Scheduler |
| 1.2 etcd | L1/L2 | 加密、认证、权限 |
| 2.1 Worker Node | L1/L2 | kubelet、kube-proxy |
| 3.1 Network | L1/L2 | NetworkPolicy、CNI |
| 4.1 Policies | L1/L2 | RBAC、PSP/PSA |
| 5.1 Workloads | L1/L2 | Pod 安全配置 |

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 使用 kube-bench 检查
kubectl apply -f https://raw.githubusercontent.com/aquasecurity/kube-bench/main/job.yaml
kubectl logs job/kube-bench
```
---

**表格底部标记**: Kusheet Project, 作者 Allen Galler (allengaller@gmail.com)

---

<!-- chunk: Obsidian 相关文档 -->
## Obsidian 相关文档

- domain-05-security-compliance MOC
- [[domain-05-security-compliance/README.md|Security Domain]]
- [[domain-05-security-compliance/00-open-source-projects-index.md|Domain-7 安全 — 开源项目索引]]
- Kubernetes 认证授权体系详解
- 网络安全策略与零信任架构
- 运行时安全防护与威胁检测
- 04 - 审计日志与合规性管理
- 05 - 策略校验与准入控制工具 (Policy Validation)
- 06 - Pod安全标准详解
- 07 - RBAC权限矩阵表
- 08 - 安全最佳实践表
- 证书管理与 TLS 配置

## See Also

- 07-rbac-matrix-configuration
- 08-security-best-practices
- 10-certificate-management
- 11-secret-management-tools

- [[domain-05-security-compliance/README.md|返回目录]]

## Related

- [[domain-19-landscape-references/topic-index/security-index.md|Security 安全知识图谱索引]]


<!-- risk-assessed -->
