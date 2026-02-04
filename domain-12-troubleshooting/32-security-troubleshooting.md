# 32 - 安全相关故障排查 (Security Troubleshooting)

> **适用版本**: Kubernetes v1.25-v1.32 | **最后更新**: 2026-02 | **参考**: [Kubernetes Security](https://kubernetes.io/docs/concepts/security/)

---

## 1. 安全故障诊断总览 (Security Diagnosis Overview)

### 1.1 常见安全故障现象分类

| 故障类型 | 症状表现 | 影响范围 | 紧急程度 |
|---------|---------|---------|---------|
| **认证失败** | 401 Unauthorized | 用户访问拒绝 | P0 - 紧急 |
| **授权不足** | 403 Forbidden | 权限受限 | P1 - 高 |
| **TLS证书问题** | SSL证书错误/过期 | 加密通信中断 | P0 - 紧急 |
| **镜像安全漏洞** | 漏洞扫描告警 | 安全风险 | P1 - 高 |
| **网络策略失效** | 未预期的网络访问 | 安全边界突破 | P0 - 紧急 |
| **特权容器运行** | 容器逃逸风险 | 主机安全威胁 | P0 - 紧急 |

### 1.2 安全架构回顾

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                      安全故障诊断架构                                        │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  ┌─────────────────────────────────────────────────────────────────────┐  │
│  │                       身份认证层                                     │  │
│  │  ┌─────────────┐    ┌─────────────┐    ┌─────────────┐              │  │
│  │  │   Service   │    │   OIDC      │    │   webhook   │              │  │
│  │  │  Account    │    │  认证       │    │   认证      │              │  │
│  │  └─────────────┘    └─────────────┘    └─────────────┘              │  │
│  └─────────────────────────────────────────────────────────────────────┘  │
│                              │                                              │
│                              ▼                                              │
│  ┌─────────────────────────────────────────────────────────────────────┐  │
│  │                       授权控制层                                     │  │
│  │  ┌─────────────┐    ┌─────────────┐    ┌─────────────┐              │  │
│  │  │    RBAC     │    │   ABAC      │    │   Node      │              │  │
│  │  │   授权      │    │   授权      │    │   授权      │              │  │
│  │  └─────────────┘    └─────────────┘    └─────────────┘              │  │
│  └─────────────────────────────────────────────────────────────────────┘  │
│                              │                                              │
│         ┌────────────────────┼────────────────────┐                       │
│         │                    │                    │                       │
│         ▼                    ▼                    ▼                       │
│  ┌─────────────┐    ┌─────────────┐    ┌─────────────┐                   │
│  │   网络安全   │   │   镜像安全   │   │   运行时安全  │                   │
│  │ (Network    │   │ (Image      │   │ (Runtime    │                   │
│  │ Policy)     │   │ Security)   │   │ Security)   │                   │
│  └─────────────┘    └─────────────┘    └─────────────┘                   │
│         │                    │                    │                       │
│         ▼                    ▼                    ▼                       │
│  ┌─────────────────────────────────────────────────────────────────────┐  │
│  │                      安全执行层                                     │  │
│  │  ┌─────────────┐    ┌─────────────┐    ┌─────────────┐              │  │
│  │  │   Pod安全   │    │   容器安全   │   │   节点安全   │              │  │
│  │  │   上下文    │    │   镜像      │   │   配置      │              │  │
│  │  └─────────────┘    └─────────────┘    └─────────────┘              │  │
│  └─────────────────────────────────────────────────────────────────────┘  │
│                              │                                              │
│                              ▼                                              │
│  ┌─────────────────────────────────────────────────────────────────────┐  │
│  │                      安全监控层                                     │  │
│  │  ┌─────────────┐    ┌─────────────┐    ┌─────────────┐              │  │
│  │  │   审计日志   │   │   威胁检测   │   │   漏洞扫描   │              │  │
│  │  │ (Audit Log) │   │ (Detection) │   │ (Scanning)  │              │  │
│  │  └─────────────┘    └─────────────┘    └─────────────┘              │  │
│  └─────────────────────────────────────────────────────────────────────┘  │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

---

## 2. 认证和授权问题排查 (Authentication and Authorization Troubleshooting)

### 2.1 认证失败问题

```bash
# ========== 1. 认证配置检查 ==========
# 检查API Server认证配置
kubectl get pods -n kube-system -l component=kube-apiserver -o jsonpath='{.items[0].spec.containers[0].command}' | jq

# 查看认证方式
kubectl get pods -n kube-system -l component=kube-apiserver -o jsonpath='{.items[0].spec.containers[0].command}' | grep -E "(--oidc|--token|--client-ca)"

# ========== 2. ServiceAccount问题 ==========
# 检查ServiceAccount存在性
kubectl get serviceaccount -n <namespace> <sa-name>

# 验证Token有效性
TOKEN_SECRET=$(kubectl get serviceaccount <sa-name> -n <namespace> -o jsonpath='{.secrets[0].name}')
kubectl get secret $TOKEN_SECRET -n <namespace> -o jsonpath='{.data.token}' | base64 -d

# 测试Token认证
APISERVER=$(kubectl config view --minify -o jsonpath='{.clusters[0].cluster.server}')
TOKEN=$(kubectl get secret $TOKEN_SECRET -n <namespace> -o jsonpath='{.data.token}' | base64 -d)
curl -k -H "Authorization: Bearer $TOKEN" $APISERVER/api/v1/namespaces

# ========== 3. 证书认证问题 ==========
# 检查客户端证书
kubectl config view --raw -o jsonpath='{.users[0].user.client-certificate-data}' | base64 -d | openssl x509 -text -noout

# 验证证书有效期
kubectl config view --raw -o jsonpath='{.users[0].user.client-certificate-data}' | base64 -d | openssl x509 -noout -dates

# 检查CA证书
kubectl config view --raw -o jsonpath='{.clusters[0].cluster.certificate-authority-data}' | base64 -d | openssl x509 -text -noout
```

### 2.2 RBAC授权问题

```bash
# ========== 1. 权限验证 ==========
# 检查用户权限
kubectl auth can-i get pods --as=<user-name>
kubectl auth can-i create deployments --as=<user-name> --as-group=<group-name>

# 检查ServiceAccount权限
kubectl auth can-i get pods --as=system:serviceaccount:<namespace>:<sa-name>

# ========== 2. RBAC资源配置检查 ==========
# 查看Role和RoleBinding
kubectl get roles,rolebindings -n <namespace>

# 查看ClusterRole和ClusterRoleBinding
kubectl get clusterroles,clusterrolebindings | grep <role-name>

# 检查具体权限规则
kubectl get role <role-name> -n <namespace> -o yaml

# ========== 3. 权限调试 ==========
# 使用who-can工具
kubectl who-can get pods -n <namespace>

# 检查默认绑定
kubectl get clusterrolebinding | grep system:authenticated
kubectl get clusterrolebinding | grep system:unauthenticated

# 验证权限继承
kubectl get rolebinding -n <namespace> -o jsonpath='{
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
        .subjects[*].kind
}{
        "/"
}{
        .subjects[*].name
}{
        "\n"
}{
    end
}'
```

---

## 3. 网络安全问题排查 (Network Security Troubleshooting)

### 3.1 NetworkPolicy失效问题

```bash
# ========== 1. 策略配置验证 ==========
# 检查NetworkPolicy支持
kubectl api-versions | grep networking.k8s.io

# 查看所有网络策略
kubectl get networkpolicy --all-namespaces

# 检查CNI插件支持
kubectl get pods -n kube-system | grep -E "(calico|cilium|weave)"

# ========== 2. 策略效果测试 ==========
# 创建测试策略
cat <<EOF | kubectl apply -f -
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: test-deny-all
  namespace: <namespace>
spec:
  podSelector: {}
  policyTypes:
  - Ingress
  - Egress
EOF

# 测试策略效果
kubectl run test-client --image=busybox -n <namespace> -it --rm -- sh
# 在容器内尝试访问其他Pod

# ========== 3. 策略规则分析 ==========
# 检查策略匹配
kubectl get networkpolicy <policy-name> -n <namespace> -o jsonpath='{.spec.podSelector}'

# 验证标签选择器
kubectl get pods -n <namespace> --show-labels | grep <selector-label>

# 检查策略方向
kubectl get networkpolicy <policy-name> -n <namespace> -o jsonpath='{.spec.policyTypes}'
```

### 3.2 TLS证书问题

```bash
# ========== 1. 证书状态检查 ==========
# 检查API Server证书
openssl s_client -connect <api-server-endpoint>:6443 -servername <api-server-host> 2>/dev/null | openssl x509 -noout -dates

# 检查Ingress TLS证书
kubectl get secret <tls-secret-name> -n <namespace> -o jsonpath='{.data.tls\.crt}' | base64 -d | openssl x509 -text -noout

# 验证证书链
echo | openssl s_client -connect <hostname>:443 2>/dev/null | openssl x509 -noout -text

# ========== 2. 证书续期问题 ==========
# 检查证书过期时间
kubectl get csr -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{.status.certificate}{"\n"}{end}' | while read name cert; do
    if [ -n "$cert" ]; then
        echo "$cert" | base64 -d | openssl x509 -noout -dates
    fi
done

# 使用kubeadm续期证书
kubeadm certs check-expiration
kubeadm certs renew all

# ========== 3. 证书信任问题 ==========
# 检查CA证书配置
kubectl config view --raw -o jsonpath='{.clusters[0].cluster.certificate-authority-data}' | base64 -d > ca.crt
openssl verify -CAfile ca.crt <server-cert.pem>

# 测试证书信任链
curl -v --cacert ca.crt https://<api-server>:6443
```

---

## 4. 镜像和运行时安全 (Image and Runtime Security)

### 4.1 镜像安全扫描

```bash
# ========== 1. 镜像漏洞扫描 ==========
# 使用Trivy扫描镜像
trivy image <image-name>:<tag>

# 扫描本地镜像
docker images | grep <image-pattern> | while read repo tag id rest; do
    echo "Scanning $repo:$tag"
    trivy image $repo:$tag
done

# ========== 2. 镜像签名验证 ==========
# 检查镜像签名
cosign verify --key <public-key> <image-name>:<tag>

# 验证镜像完整性
docker inspect <image-name>:<tag> | jq '.[0].Id'

# 检查镜像来源
kubectl get pods -n <namespace> -o jsonpath='{
    range .items[*]
}{
        .spec.containers[*].image
}{
        "\n"
}{
    end
}' | sort | uniq

# ========== 3. 镜像仓库安全 ==========
# 检查私有仓库配置
kubectl get secret -n <namespace> | grep docker

# 验证镜像拉取密钥
kubectl get secret <pull-secret-name> -n <namespace> -o jsonpath='{.data.\.dockerconfigjson}' | base64 -d | jq

# 测试镜像仓库访问
kubectl run test-pull --image=<private-registry>/<image>:<tag> -n <namespace> --restart=Never
```

### 4.2 容器安全配置

```bash
# ========== 1. 安全上下文检查 ==========
# 检查Pod安全上下文
kubectl get pod <pod-name> -n <namespace> -o jsonpath='{.spec.securityContext}'

# 验证容器安全配置
kubectl get pod <pod-name> -n <namespace> -o jsonpath='{.spec.containers[*].securityContext}'

# 检查特权模式
kubectl get pod <pod-name> -n <namespace> -o jsonpath='{.spec.containers[*].securityContext.privileged}'

# ========== 2. PSP/PSS策略检查 ==========
# 检查PodSecurityPolicy (如果启用)
kubectl get psp

# 检查PodSecurity Admission
kubectl get namespaces -o jsonpath='{
    range .items[*]
}{
        .metadata.name
}{
        "\t"
}{
        .metadata.labels.pod-security\.kubernetes\.io/enforce
}{
        "\n"
}{
    end
}'

# 验证安全级别
kubectl get namespace <namespace> -o jsonpath='{.metadata.labels.pod-security\.kubernetes\.io/enforce}'

# ========== 3. 容器运行时安全 ==========
# 检查运行时配置
kubectl get nodes -o jsonpath='{
    range .items[*]
}{
        .metadata.name
}{
        "\t"
}{
        .status.nodeInfo.containerRuntimeVersion
}{
        "\n"
}{
    end
}'

# 验证Seccomp配置
kubectl get pod <pod-name> -n <namespace> -o jsonpath='{.spec.securityContext.seccompProfile}'

# 检查AppArmor配置
kubectl get pod <pod-name> -n <namespace> -o jsonpath='{.metadata.annotations.container\.apparmor\.security\.beta\.kubernetes\.io/*}'
```

---

## 5. 安全监控和审计 (Security Monitoring and Auditing)

### 5.1 审计日志配置

```bash
# ========== 1. 审计策略检查 ==========
# 查看审计策略配置
kubectl get pods -n kube-system -l component=kube-apiserver -o jsonpath='{.items[0].spec.containers[0].command}' | grep audit

# 检查审计日志配置
cat /etc/kubernetes/audit-policy.yaml

# 验证审计后端配置
kubectl get pods -n kube-system -l component=kube-apiserver -o jsonpath='{.items[0].spec.containers[0].command}' | grep -E "(--audit-log|--audit-webhook)"

# ========== 2. 审计日志分析 ==========
# 查看审计日志
tail -f /var/log/kubernetes/audit.log

# 分析认证失败事件
grep '"responseStatus":{"code":401' /var/log/kubernetes/audit.log | jq '.'

# 分析授权失败事件
grep '"responseStatus":{"code":403' /var/log/kubernetes/audit.log | jq '.'

# 统计访问模式
jq '.verb, .user.username' /var/log/kubernetes/audit.log | sort | uniq -c

# ========== 3. 安全事件监控 ==========
# 监控异常Pod创建
kubectl get events --field-selector involvedObject.kind=Pod,reason=FailedCreate

# 监控权限提升尝试
kubectl get events --field-selector involvedObject.kind=Pod,reason=FailedAuthorization

# 检查可疑容器行为
kubectl get pods --all-namespaces -o jsonpath='{
    range .items[*]
}{
        .metadata.namespace
}{
        "/"
}{
        .metadata.name
}{
        "\t"
}{
        .spec.containers[*].securityContext.privileged
}{
        "\n"
}{
    end
}' | grep true
```

### 5.2 威胁检测配置

```bash
# ========== Falco配置检查 ==========
# 检查Falco安装状态
kubectl get pods -n falco

# 查看Falco规则
kubectl get configmap falco-rules -n falco -o yaml

# 检查Falco告警
kubectl logs -n falco -l app=falco --since=1h | grep -i alert

# ========== 安全扫描工具 ==========
# 运行kube-bench安全检查
kube-bench run --targets=node,master

# 执行kube-hunter扫描
kube-hunter --pod

# 运行Sonobuoy合规检查
sonobuoy run --mode=certified-conformance

# ========== 漏洞扫描集成 ==========
# 配置安全扫描CronJob
cat <<EOF | kubectl apply -f -
apiVersion: batch/v1
kind: CronJob
metadata:
  name: security-scan
  namespace: security
spec:
  schedule: "0 2 * * *"  # 每天凌晨2点执行
  jobTemplate:
    spec:
      template:
        spec:
          containers:
          - name: scanner
            image: aquasec/trivy:latest
            command:
            - /bin/sh
            - -c
            - |
              # 扫描所有命名空间的镜像
              kubectl get pods --all-namespaces -o jsonpath='{range .items[*]}{.spec.containers[*].image}{"\n"}{end}' | \
              sort | uniq | while read image; do
                echo "Scanning $image"
                trivy image --severity HIGH,CRITICAL $image
              done
          restartPolicy: OnFailure
EOF
```

---

## 6. 应急响应和修复 (Incident Response and Remediation)

### 6.1 安全事件应急流程

```bash
# ========== 1. 立即响应措施 ==========
# 隔离受感染Pod
kubectl delete pod <compromised-pod> -n <namespace>

# 限制ServiceAccount权限
kubectl patch serviceaccount <sa-name> -n <namespace> -p '{"secrets":[]}'

# 禁用可疑用户访问
kubectl delete clusterrolebinding <suspicious-user-binding>

# ========== 2. 取证和分析 ==========
# 收集系统信息
kubectl cluster-info dump --namespaces=<affected-namespace> --output-directory=/tmp/incident-$(date +%Y%m%d-%H%M%S)

# 导出相关日志
kubectl logs <affected-pod> -n <namespace> --previous > /tmp/pod-logs.txt

# 检查资源变更历史
kubectl get events --field-selector involvedObject.name=<affected-resource> --sort-by='.lastTimestamp'

# ========== 3. 修复和加固 ==========
# 更新受损镜像
kubectl set image deployment/<deployment-name> <container-name>=<new-image>:<tag> -n <namespace>

# 加强网络策略
cat <<EOF | kubectl apply -f -
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: restrictive-policy
  namespace: <namespace>
spec:
  podSelector: {}
  policyTypes:
  - Ingress
  - Egress
  ingress:
  - from:
    - namespaceSelector:
        matchLabels:
          name: <allowed-namespace>
  egress:
  - to:
    - namespaceSelector:
        matchLabels:
          name: <allowed-namespace>
EOF

# 旋转密钥和证书
kubeadm certs renew all
kubectl delete secret <compromised-secret> -n <namespace>
kubectl create secret generic <new-secret> --from-literal=key=value -n <namespace>
```

### 6.2 安全加固最佳实践

```bash
# ========== 推荐安全配置模板 ==========
cat <<EOF > secure-pod-template.yaml
apiVersion: v1
kind: Pod
metadata:
  name: secure-pod
  namespace: production
spec:
  securityContext:
    runAsNonRoot: true
    runAsUser: 1000
    fsGroup: 2000
    seccompProfile:
      type: RuntimeDefault
  containers:
  - name: app
    image: my-secure-app:latest
    securityContext:
      allowPrivilegeEscalation: false
      readOnlyRootFilesystem: true
      capabilities:
        drop:
        - ALL
        add:
        - NET_BIND_SERVICE
    resources:
      requests:
        memory: "64Mi"
        cpu: "250m"
      limits:
        memory: "128Mi"
        cpu: "500m"
    livenessProbe:
      httpGet:
        path: /health
        port: 8080
      initialDelaySeconds: 30
      periodSeconds: 10
    readinessProbe:
      httpGet:
        path: /ready
        port: 8080
      initialDelaySeconds: 5
      periodSeconds: 5
  automountServiceAccountToken: false
EOF

# ========== 命名空间安全策略 ==========
cat <<EOF | kubectl apply -f -
apiVersion: v1
kind: Namespace
metadata:
  name: production
  labels:
    pod-security.kubernetes.io/enforce: restricted
    pod-security.kubernetes.io/enforce-version: latest
    pod-security.kubernetes.io/audit: restricted
    pod-security.kubernetes.io/warn: restricted
EOF

# ========== RBAC最小权限配置 ==========
cat <<EOF | kubectl apply -f -
apiVersion: rbac.authorization.k8s.io/v1
kind: Role
metadata:
  namespace: production
  name: deployment-manager
rules:
- apiGroups: ["apps"]
  resources: ["deployments"]
  verbs: ["get", "list", "watch", "update", "patch"]
- apiGroups: [""]
  resources: ["pods"]
  verbs: ["get", "list", "watch"]
---
apiVersion: rbac.authorization.k8s.io/v1
kind: RoleBinding
metadata:
  name: deployment-manager-binding
  namespace: production
subjects:
- kind: User
  name: developer@example.com
  apiGroup: rbac.authorization.k8s.io
roleRef:
  kind: Role
  name: deployment-manager
  apiGroup: rbac.authorization.k8s.io
EOF
```

---