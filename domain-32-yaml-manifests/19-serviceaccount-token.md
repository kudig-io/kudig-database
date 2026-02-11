# 19 - ServiceAccount / Token 管理 YAML 配置参考

## 概述

ServiceAccount 是 Kubernetes 中为 Pod 提供身份标识的资源对象。从 v1.22 开始，Kubernetes 引入了 Bound Service Account Token 机制，提供了更安全、时间限制的令牌管理方式。本文档覆盖 ServiceAccount、TokenRequest、TokenReview 和 CertificateSigningRequest 的完整 YAML 配置。

**适用版本**: Kubernetes v1.25 - v1.32  
**更新时间**: 2026-02

---

## 1. ServiceAccount 基础配置

### 1.1 基本 ServiceAccount

```yaml
apiVersion: v1
kind: ServiceAccount
metadata:
  name: my-service-account
  namespace: default
  labels:
    app: myapp
    tier: backend
  annotations:
    # 描述此 ServiceAccount 的用途
    description: "ServiceAccount for backend application"

# automountServiceAccountToken 控制是否自动挂载 token
# true(默认): 自动挂载到 /var/run/secrets/kubernetes.io/serviceaccount/
# false: 不自动挂载,需要显式配置 volume
automountServiceAccountToken: true

# imagePullSecrets 用于从私有镜像仓库拉取镜像
# 会被添加到使用此 ServiceAccount 的所有 Pod 中
imagePullSecrets:
  - name: harbor-registry-secret
  - name: dockerhub-secret

# secrets 字段在 v1.24+ 已弃用,不再自动创建长期 token
# 现在使用 TokenRequest API 或手动创建 Secret
# secrets:
#   - name: my-service-account-token
```

### 1.2 禁用自动挂载 Token 的 ServiceAccount

```yaml
apiVersion: v1
kind: ServiceAccount
metadata:
  name: no-token-sa
  namespace: default
  annotations:
    # 用于不需要访问 Kubernetes API 的应用
    purpose: "For applications that don't need K8s API access"

# 禁用自动挂载 token,提升安全性
# 可以在 Pod 级别覆盖此设置
automountServiceAccountToken: false
```

### 1.3 带镜像拉取密钥的 ServiceAccount

```yaml
apiVersion: v1
kind: ServiceAccount
metadata:
  name: app-with-private-images
  namespace: production
  labels:
    env: prod
    team: platform

automountServiceAccountToken: true

# 配置多个镜像仓库的拉取密钥
imagePullSecrets:
  # Harbor 私有仓库
  - name: harbor-prod-secret
  # AWS ECR
  - name: ecr-registry-secret
  # Google Container Registry
  - name: gcr-json-key
```

---

## 2. Pod 中使用 ServiceAccount

### 2.1 Pod 指定 ServiceAccount

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: pod-with-sa
  namespace: default
spec:
  # 指定使用的 ServiceAccount
  # 如果不指定,默认使用 namespace 中的 "default" ServiceAccount
  serviceAccountName: my-service-account
  
  # Pod 级别可以覆盖 ServiceAccount 的 automountServiceAccountToken 设置
  # false: 即使 SA 设置为 true,此 Pod 也不挂载 token
  automountServiceAccountToken: true
  
  containers:
  - name: app
    image: myapp:v1.0
    # Token 会自动挂载到以下路径:
    # /var/run/secrets/kubernetes.io/serviceaccount/token      - JWT token
    # /var/run/secrets/kubernetes.io/serviceaccount/ca.crt     - CA 证书
    # /var/run/secrets/kubernetes.io/serviceaccount/namespace  - namespace 名称
    volumeMounts:
    - name: kube-api-access
      mountPath: /var/run/secrets/kubernetes.io/serviceaccount
      readOnly: true
  
  volumes:
  # v1.22+ 自动创建的投影卷(Projected Volume)
  # 包含 token, ca.crt, namespace
  - name: kube-api-access
    projected:
      sources:
      - serviceAccountToken:
          # token 过期时间(秒),默认 3600(1小时)
          expirationSeconds: 3600
          # token 绑定到此 Pod,增强安全性
          path: token
      - configMap:
          name: kube-root-ca.crt
          items:
          - key: ca.crt
            path: ca.crt
      - downwardAPI:
          items:
          - path: namespace
            fieldRef:
              fieldPath: metadata.namespace
```

### 2.2 禁用 Token 挂载的 Pod

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: pod-no-token
  namespace: default
spec:
  serviceAccountName: no-token-sa
  # Pod 级别禁用 token 挂载
  # 适用于不需要访问 Kubernetes API 的应用
  automountServiceAccountToken: false
  
  containers:
  - name: nginx
    image: nginx:1.25
    ports:
    - containerPort: 80
```

### 2.3 自定义 Token 过期时间

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: pod-custom-token-expiry
  namespace: default
spec:
  serviceAccountName: my-service-account
  
  containers:
  - name: app
    image: myapp:v1.0
    volumeMounts:
    - name: custom-token
      mountPath: /var/run/secrets/tokens
      readOnly: true
  
  volumes:
  # 自定义 token 配置,使用 Projected Volume
  - name: custom-token
    projected:
      sources:
      - serviceAccountToken:
          # 受众(audience),用于验证 token 的目标
          # 默认为 kube-apiserver 的地址
          audience: "https://kubernetes.default.svc"
          # token 过期时间: 24小时
          expirationSeconds: 86400
          # token 文件路径
          path: custom-token
      - configMap:
          name: kube-root-ca.crt
          items:
          - key: ca.crt
            path: ca.crt
```

---

## 3. TokenRequest API (v1.22+)

### 3.1 TokenRequest 对象

TokenRequest 是 ServiceAccount 的子资源,用于请求短期 token。

```yaml
apiVersion: authentication.k8s.io/v1
kind: TokenRequest
metadata:
  name: my-service-account
  namespace: default
spec:
  # audiences 定义 token 的预期受众
  # token 只能被这些受众验证使用
  audiences:
    - "https://kubernetes.default.svc.cluster.local"
    - "https://api.example.com"
  
  # expirationSeconds 定义 token 的有效期(秒)
  # 最小值: 600(10分钟)
  # 默认值: 3600(1小时)
  # 最大值: 由 kube-apiserver 的 --service-account-max-token-expiration 参数控制
  expirationSeconds: 7200  # 2小时
  
  # boundObjectRef 将 token 绑定到特定对象
  # token 只能在该对象存在时有效,对象删除后 token 失效
  # 这是 Bound Service Account Token 的核心特性
  boundObjectRef:
    kind: Pod
    apiVersion: v1
    name: my-pod
    uid: "a7f3d9e2-5c1b-4e8f-9a2d-3f7e8c1b4a6d"

# status 字段由 API server 填充(只读)
status:
  # 实际的 JWT token
  token: "eyJhbGciOiJSUzI1NiIsImtpZCI6Ii..."
  # token 过期时间(RFC3339 格式)
  expirationTimestamp: "2026-02-10T14:30:00Z"
```

### 3.2 使用 kubectl 请求 Token

```bash
# 创建 TokenRequest
kubectl create token my-service-account \
  --namespace default \
  --duration 2h \
  --audience "https://api.example.com"

# 绑定到特定 Pod
kubectl create token my-service-account \
  --namespace default \
  --duration 1h \
  --bound-object-kind Pod \
  --bound-object-name my-pod \
  --bound-object-uid "a7f3d9e2-5c1b-4e8f-9a2d-3f7e8c1b4a6d"
```

### 3.3 在 Pod 中使用 TokenRequest

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: pod-with-token-request
  namespace: default
spec:
  serviceAccountName: my-service-account
  
  containers:
  - name: app
    image: myapp:v1.0
    env:
    # 将 token 作为环境变量注入
    - name: VAULT_TOKEN
      valueFrom:
        secretKeyRef:
          name: vault-token
          key: token
    volumeMounts:
    # 挂载 token 到容器
    - name: vault-token
      mountPath: /var/run/secrets/vault
      readOnly: true
  
  volumes:
  - name: vault-token
    projected:
      sources:
      # 为 Vault 请求专用 token
      - serviceAccountToken:
          audience: "vault://vault.example.com"
          expirationSeconds: 3600
          path: vault-token
```

---

## 4. 手动创建长期 Token Secret (v1.24+)

v1.24+ 不再自动创建长期 token,需要手动创建。**生产环境不推荐使用长期 token**。

### 4.1 创建 Token Secret

```yaml
apiVersion: v1
kind: Secret
metadata:
  name: my-service-account-token
  namespace: default
  annotations:
    # 关键注解: 将此 Secret 标记为 ServiceAccount token
    kubernetes.io/service-account.name: "my-service-account"
    # 可选: 添加描述
    description: "Long-lived token for CI/CD (not recommended)"
type: kubernetes.io/service-account-token

# 以下字段由 Kubernetes 自动填充
# data:
#   ca.crt: <base64-encoded-ca-cert>
#   namespace: <base64-encoded-namespace>
#   token: <base64-encoded-jwt-token>
```

### 4.2 使用长期 Token Secret

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: pod-with-long-lived-token
  namespace: default
spec:
  serviceAccountName: my-service-account
  
  containers:
  - name: app
    image: myapp:v1.0
    env:
    # 将长期 token 作为环境变量
    - name: KUBE_TOKEN
      valueFrom:
        secretKeyRef:
          name: my-service-account-token
          key: token
    volumeMounts:
    # 或挂载为文件
    - name: sa-token
      mountPath: /var/run/secrets/legacy-token
      readOnly: true
  
  volumes:
  - name: sa-token
    secret:
      secretName: my-service-account-token
      items:
      - key: token
        path: token
      - key: ca.crt
        path: ca.crt
      - key: namespace
        path: namespace
```

---

## 5. TokenReview

TokenReview 用于验证 token 的有效性和获取 token 关联的身份信息。

### 5.1 TokenReview 对象

```yaml
apiVersion: authentication.k8s.io/v1
kind: TokenReview
metadata:
  # TokenReview 是无状态的,不需要 name
  creationTimestamp: null
spec:
  # 要验证的 token(JWT 格式)
  token: "eyJhbGciOiJSUzI1NiIsImtpZCI6Ii..."
  
  # audiences 指定期望的受众列表
  # 如果 token 的 aud 声明不包含这些受众之一,验证失败
  audiences:
    - "https://kubernetes.default.svc.cluster.local"

# status 字段由 API server 填充(只读)
status:
  # authenticated 表示 token 是否有效
  authenticated: true
  
  # user 包含 token 关联的用户信息
  user:
    # username 是用户的唯一标识
    # ServiceAccount 格式: system:serviceaccount:<namespace>:<name>
    username: "system:serviceaccount:default:my-service-account"
    
    # uid 是 ServiceAccount 的 UID
    uid: "12345678-1234-1234-1234-123456789abc"
    
    # groups 是用户所属的组
    groups:
      - "system:serviceaccounts"
      - "system:serviceaccounts:default"
      - "system:authenticated"
    
    # extra 包含额外的用户属性
    extra:
      authentication.kubernetes.io/pod-name:
        - "my-pod"
      authentication.kubernetes.io/pod-uid:
        - "a7f3d9e2-5c1b-4e8f-9a2d-3f7e8c1b4a6d"
  
  # audiences 返回 token 中实际的受众列表
  audiences:
    - "https://kubernetes.default.svc.cluster.local"
  
  # error 字段在验证失败时包含错误信息
  # error: "token is expired"
```

### 5.2 使用 kubectl 验证 Token

```bash
# 验证 token
TOKEN="eyJhbGciOiJSUzI1NiIsImtpZCI6Ii..."
kubectl create -f - <<EOF
apiVersion: authentication.k8s.io/v1
kind: TokenReview
spec:
  token: "$TOKEN"
  audiences:
    - "https://kubernetes.default.svc.cluster.local"
EOF
```

---

## 6. CertificateSigningRequest (CSR)

CSR 用于请求 X.509 证书,常用于 TLS 认证和节点证书管理。

### 6.1 基本 CSR

```yaml
apiVersion: certificates.k8s.io/v1
kind: CertificateSigningRequest
metadata:
  name: my-app-csr
spec:
  # request 是 PEM 编码的 PKCS#10 证书签名请求
  # 使用 base64 编码后的内容
  request: |
    LS0tLS1CRUdJTiBDRVJUSUZJQ0FURSBSRVFVRVNULS0tLS0KTUlJQ3Z6Q0NBYWNDQVFBd...
  
  # signerName 指定证书签名者
  # kubernetes.io/kube-apiserver-client: 用于 API 客户端认证
  # kubernetes.io/kube-apiserver-client-kubelet: 用于 kubelet 客户端证书
  # kubernetes.io/kubelet-serving: 用于 kubelet 服务端证书
  # kubernetes.io/legacy-unknown: 遗留签名者(v1.22+ 不推荐)
  signerName: "kubernetes.io/kube-apiserver-client"
  
  # expirationSeconds 指定证书有效期(秒)
  # v1.22+ 支持
  # 最小值: 600(10分钟)
  # 默认值: 由签名者决定,通常为 8760h(1年)
  expirationSeconds: 31536000  # 1年
  
  # usages 指定证书的用途(Extended Key Usage)
  # 必须与 signerName 兼容
  usages:
    - "digital signature"      # 数字签名
    - "key encipherment"        # 密钥加密
    - "client auth"             # 客户端认证
  
  # username, uid, groups 由 API server 自动填充
  # 表示请求者的身份
  username: "system:serviceaccount:default:my-service-account"
  uid: "12345678-1234-1234-1234-123456789abc"
  groups:
    - "system:serviceaccounts"
    - "system:serviceaccounts:default"
    - "system:authenticated"
  
  # extra 包含请求者的额外属性
  extra:
    authentication.kubernetes.io/pod-name:
      - "my-pod"
    authentication.kubernetes.io/pod-uid:
      - "a7f3d9e2-5c1b-4e8f-9a2d-3f7e8c1b4a6d"

# status 字段由控制器填充(只读)
status:
  # conditions 表示 CSR 的状态
  conditions:
  - type: Approved
    status: "True"
    reason: "AutoApproved"
    message: "Auto-approved by kube-controller-manager"
    lastUpdateTime: "2026-02-10T12:00:00Z"
    lastTransitionTime: "2026-02-10T12:00:00Z"
  
  # certificate 是 PEM 编码的 X.509 证书
  # 只有在 CSR 被批准后才会填充
  certificate: |
    LS0tLS1CRUdJTiBDRVJUSUZJQ0FURS0tLS0tCk1JSURKekNDQWcrZ0F3SUJBZ0lRQWx...
```

### 6.2 用于客户端认证的 CSR

```yaml
apiVersion: certificates.k8s.io/v1
kind: CertificateSigningRequest
metadata:
  name: user-client-csr
spec:
  # 客户端证书请求(需要先生成私钥和 CSR)
  # openssl req -new -key user.key -out user.csr -subj "/CN=john/O=developers"
  # cat user.csr | base64 -w 0
  request: |
    LS0tLS1CRUdJTiBDRVJUSUZJQ0FURSBSRVFVRVNULS0tLS0KTUlJQ3Z6Q0NBYWNDQVFBd...
  
  # 用于 API 客户端认证
  signerName: "kubernetes.io/kube-apiserver-client"
  
  # 证书有效期: 90天
  expirationSeconds: 7776000
  
  usages:
    - "digital signature"
    - "key encipherment"
    - "client auth"
```

### 6.3 用于 Kubelet 服务端证书的 CSR

```yaml
apiVersion: certificates.k8s.io/v1
kind: CertificateSigningRequest
metadata:
  name: kubelet-serving-csr
spec:
  request: |
    LS0tLS1CRUdJTiBDRVJUSUZJQ0FURSBSRVFVRVNULS0tLS0KTUlJQ3Z6Q0NBYWNDQVFBd...
  
  # 用于 kubelet 服务端证书
  # Subject Alternative Names (SANs) 应包含节点的 hostname 和 IP
  signerName: "kubernetes.io/kubelet-serving"
  
  expirationSeconds: 31536000  # 1年
  
  usages:
    - "digital signature"
    - "key encipherment"
    - "server auth"             # 服务端认证
```

### 6.4 批准和拒绝 CSR

```bash
# 批准 CSR
kubectl certificate approve my-app-csr

# 拒绝 CSR
kubectl certificate deny user-client-csr

# 查看 CSR
kubectl get csr

# 获取签名后的证书
kubectl get csr my-app-csr -o jsonpath='{.status.certificate}' | base64 -d > client.crt
```

---

## 7. 内部原理: Bound Service Account Token (v1.22+)

### 7.1 工作机制

Bound Service Account Token 是 v1.22 引入的安全增强特性:

1. **时间绑定**: Token 有明确的过期时间,默认 1 小时
2. **对象绑定**: Token 绑定到特定 Pod,Pod 删除后 token 失效
3. **受众绑定**: Token 包含 audience 声明,只能用于指定的服务
4. **自动轮转**: Kubelet 会在 token 过期前自动刷新

### 7.2 Token 结构 (JWT)

```json
{
  "aud": [
    "https://kubernetes.default.svc.cluster.local"
  ],
  "exp": 1707572400,
  "iat": 1707568800,
  "iss": "https://kubernetes.default.svc.cluster.local",
  "kubernetes.io": {
    "namespace": "default",
    "pod": {
      "name": "my-pod",
      "uid": "a7f3d9e2-5c1b-4e8f-9a2d-3f7e8c1b4a6d"
    },
    "serviceaccount": {
      "name": "my-service-account",
      "uid": "12345678-1234-1234-1234-123456789abc"
    }
  },
  "nbf": 1707568800,
  "sub": "system:serviceaccount:default:my-service-account"
}
```

### 7.3 与旧版 Token 的区别

| 特性 | 旧版 Token (v1.23-) | Bound Token (v1.22+) |
|------|---------------------|----------------------|
| 存储方式 | Secret 对象 | Projected Volume |
| 有效期 | 永久 | 可配置(默认 1小时) |
| 绑定 Pod | 否 | 是 |
| 自动轮转 | 否 | 是 |
| 受众验证 | 否 | 是 |
| 安全性 | 低 | 高 |

### 7.4 Kubelet 自动轮转

Kubelet 负责管理 Pod 中的 token:

```yaml
# Kubelet 配置 (/var/lib/kubelet/config.yaml)
apiVersion: kubelet.config.k8s.io/v1beta1
kind: KubeletConfiguration
authentication:
  webhook:
    enabled: true
    cacheTTL: 2m
  anonymous:
    enabled: false
  x509:
    clientCAFile: /etc/kubernetes/pki/ca.crt
```

Token 轮转逻辑:
- Token 过期时间的 80% 后开始尝试刷新
- 例如: 1小时 token 会在 48 分钟后开始刷新
- 刷新失败会重试,直到 token 过期

---

## 8. 生产案例

### 8.1 案例 1: 微服务应用的 ServiceAccount 隔离

**场景**: 多租户 SaaS 平台,每个租户的微服务需要独立的身份和权限。

```yaml
---
# 租户 A 的前端服务 ServiceAccount
apiVersion: v1
kind: ServiceAccount
metadata:
  name: tenant-a-frontend
  namespace: tenant-a
  labels:
    tenant: tenant-a
    tier: frontend
  annotations:
    description: "ServiceAccount for tenant A frontend services"
automountServiceAccountToken: true
imagePullSecrets:
  - name: harbor-registry

---
# 租户 A 的后端服务 ServiceAccount
apiVersion: v1
kind: ServiceAccount
metadata:
  name: tenant-a-backend
  namespace: tenant-a
  labels:
    tenant: tenant-a
    tier: backend
  annotations:
    description: "ServiceAccount for tenant A backend services"
automountServiceAccountToken: true
imagePullSecrets:
  - name: harbor-registry

---
# 前端 Deployment
apiVersion: apps/v1
kind: Deployment
metadata:
  name: frontend
  namespace: tenant-a
spec:
  replicas: 3
  selector:
    matchLabels:
      app: frontend
      tenant: tenant-a
  template:
    metadata:
      labels:
        app: frontend
        tenant: tenant-a
    spec:
      serviceAccountName: tenant-a-frontend
      # 前端不需要访问 K8s API,禁用 token
      automountServiceAccountToken: false
      containers:
      - name: frontend
        image: harbor.example.com/tenant-a/frontend:v1.0
        ports:
        - containerPort: 8080

---
# 后端 Deployment(需要访问 K8s API)
apiVersion: apps/v1
kind: Deployment
metadata:
  name: backend
  namespace: tenant-a
spec:
  replicas: 3
  selector:
    matchLabels:
      app: backend
      tenant: tenant-a
  template:
    metadata:
      labels:
        app: backend
        tenant: tenant-a
    spec:
      serviceAccountName: tenant-a-backend
      # 后端需要访问 K8s API,启用 token
      automountServiceAccountToken: true
      containers:
      - name: backend
        image: harbor.example.com/tenant-a/backend:v1.0
        ports:
        - containerPort: 9090
        volumeMounts:
        - name: kube-api-access
          mountPath: /var/run/secrets/kubernetes.io/serviceaccount
          readOnly: true
      volumes:
      - name: kube-api-access
        projected:
          sources:
          - serviceAccountToken:
              # 短期 token,1小时过期
              expirationSeconds: 3600
              path: token
          - configMap:
              name: kube-root-ca.crt
              items:
              - key: ca.crt
                path: ca.crt
          - downwardAPI:
              items:
              - path: namespace
                fieldRef:
                  fieldPath: metadata.namespace
```

**安全要点**:
- 前端服务不需要 K8s API 访问,禁用 `automountServiceAccountToken`
- 后端服务使用短期 token (1小时),自动轮转
- 每个租户使用独立的 namespace 和 ServiceAccount
- 使用 RBAC 限制每个 ServiceAccount 的权限(见 RBAC 章节)

---

### 8.2 案例 2: CI/CD Pipeline 的 Token 管理

**场景**: GitLab CI/CD 需要部署应用到 Kubernetes,使用短期 token 而非长期凭证。

```yaml
---
# CI/CD ServiceAccount
apiVersion: v1
kind: ServiceAccount
metadata:
  name: gitlab-ci
  namespace: ci-cd
  labels:
    app: gitlab-ci
    purpose: deployment
  annotations:
    description: "ServiceAccount for GitLab CI/CD deployment"
automountServiceAccountToken: false
imagePullSecrets:
  - name: gitlab-registry

---
# CI/CD Job 使用的 Pod
apiVersion: v1
kind: Pod
metadata:
  name: deploy-job
  namespace: ci-cd
  labels:
    app: gitlab-ci
    job: deploy
spec:
  serviceAccountName: gitlab-ci
  restartPolicy: Never
  
  # InitContainer: 获取短期 token
  initContainers:
  - name: get-token
    image: bitnami/kubectl:1.29
    command:
    - sh
    - -c
    - |
      # 创建 2小时有效期的 token
      kubectl create token gitlab-ci \
        --namespace ci-cd \
        --duration 2h \
        --audience "https://kubernetes.default.svc" \
        > /tmp/token/kube-token
      
      echo "Token created successfully"
    volumeMounts:
    - name: token
      mountPath: /tmp/token
  
  containers:
  - name: deploy
    image: bitnami/kubectl:1.29
    command:
    - sh
    - -c
    - |
      # 使用 token 进行部署
      export KUBE_TOKEN=$(cat /tmp/token/kube-token)
      
      kubectl apply -f /manifests/deployment.yaml \
        --token="$KUBE_TOKEN" \
        --namespace=production
      
      kubectl rollout status deployment/myapp \
        --token="$KUBE_TOKEN" \
        --namespace=production
    volumeMounts:
    - name: token
      mountPath: /tmp/token
      readOnly: true
    - name: manifests
      mountPath: /manifests
      readOnly: true
  
  volumes:
  - name: token
    emptyDir:
      medium: Memory  # 使用内存存储 token,更安全
  - name: manifests
    configMap:
      name: deployment-manifests
```

**GitLab CI 配置** (.gitlab-ci.yml):

```yaml
deploy:
  stage: deploy
  image: bitnami/kubectl:1.29
  script:
    # 使用 kubectl create token 获取短期 token
    - export KUBE_TOKEN=$(kubectl create token gitlab-ci --namespace ci-cd --duration 2h)
    
    # 使用 token 进行部署
    - kubectl apply -f k8s/deployment.yaml --token="$KUBE_TOKEN" --namespace=production
    
    # 等待 rollout 完成
    - kubectl rollout status deployment/myapp --token="$KUBE_TOKEN" --namespace=production
  only:
    - main
```

**安全优势**:
- 不使用长期 token,避免凭证泄露风险
- Token 有明确的过期时间(2小时)
- Token 存储在内存中,不写入磁盘
- 每次 CI/CD 任务使用新的 token

---

### 8.3 案例 3: 多集群应用的 Token 绑定

**场景**: 应用需要访问多个 Kubernetes 集群,使用不同受众的 token。

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: multi-cluster-app
  namespace: default
spec:
  serviceAccountName: multi-cluster-sa
  
  containers:
  - name: app
    image: myapp:v1.0
    env:
    # 主集群 API 地址
    - name: MAIN_CLUSTER_API
      value: "https://main-cluster.example.com:6443"
    # DR 集群 API 地址
    - name: DR_CLUSTER_API
      value: "https://dr-cluster.example.com:6443"
    
    volumeMounts:
    # 主集群 token
    - name: main-cluster-token
      mountPath: /var/run/secrets/main-cluster
      readOnly: true
    # DR 集群 token
    - name: dr-cluster-token
      mountPath: /var/run/secrets/dr-cluster
      readOnly: true
    # 本地集群 token
    - name: local-cluster-token
      mountPath: /var/run/secrets/local-cluster
      readOnly: true
  
  volumes:
  # 主集群 token (audience 为主集群 API)
  - name: main-cluster-token
    projected:
      sources:
      - serviceAccountToken:
          audience: "https://main-cluster.example.com:6443"
          expirationSeconds: 7200  # 2小时
          path: token
      - configMap:
          name: main-cluster-ca
          items:
          - key: ca.crt
            path: ca.crt
  
  # DR 集群 token (audience 为 DR 集群 API)
  - name: dr-cluster-token
    projected:
      sources:
      - serviceAccountToken:
          audience: "https://dr-cluster.example.com:6443"
          expirationSeconds: 7200
          path: token
      - configMap:
          name: dr-cluster-ca
          items:
          - key: ca.crt
            path: ca.crt
  
  # 本地集群 token
  - name: local-cluster-token
    projected:
      sources:
      - serviceAccountToken:
          audience: "https://kubernetes.default.svc"
          expirationSeconds: 3600
          path: token
      - configMap:
          name: kube-root-ca.crt
          items:
          - key: ca.crt
            path: ca.crt

---
# 主集群 CA 证书 ConfigMap
apiVersion: v1
kind: ConfigMap
metadata:
  name: main-cluster-ca
  namespace: default
data:
  ca.crt: |
    -----BEGIN CERTIFICATE-----
    MIIDCzCCAfOgAwIBAgIQAbC...
    -----END CERTIFICATE-----

---
# DR 集群 CA 证书 ConfigMap
apiVersion: v1
kind: ConfigMap
metadata:
  name: dr-cluster-ca
  namespace: default
data:
  ca.crt: |
    -----BEGIN CERTIFICATE-----
    MIIDCzCCAfOgAwIBAgIQXyZ...
    -----END CERTIFICATE-----
```

**应用代码示例** (Go):

```go
package main

import (
    "io/ioutil"
    "k8s.io/client-go/kubernetes"
    "k8s.io/client-go/rest"
)

func main() {
    // 主集群客户端
    mainClient := createClient(
        "/var/run/secrets/main-cluster/token",
        "/var/run/secrets/main-cluster/ca.crt",
        "https://main-cluster.example.com:6443",
    )
    
    // DR 集群客户端
    drClient := createClient(
        "/var/run/secrets/dr-cluster/token",
        "/var/run/secrets/dr-cluster/ca.crt",
        "https://dr-cluster.example.com:6443",
    )
    
    // 本地集群客户端
    localClient := createClient(
        "/var/run/secrets/local-cluster/token",
        "/var/run/secrets/local-cluster/ca.crt",
        "https://kubernetes.default.svc",
    )
}

func createClient(tokenPath, caPath, host string) *kubernetes.Clientset {
    token, _ := ioutil.ReadFile(tokenPath)
    
    config := &rest.Config{
        Host:        host,
        BearerToken: string(token),
        TLSClientConfig: rest.TLSClientConfig{
            CAFile: caPath,
        },
    }
    
    clientset, _ := kubernetes.NewForConfig(config)
    return clientset
}
```

---

### 8.4 案例 4: Vault 集成的 Token 管理

**场景**: 应用使用 HashiCorp Vault 管理敏感配置,需要专用的 Vault token。

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: app-with-vault
  namespace: default
spec:
  serviceAccountName: vault-app-sa
  
  # InitContainer: 使用 K8s token 交换 Vault token
  initContainers:
  - name: vault-authenticator
    image: vault:1.15
    command:
    - sh
    - -c
    - |
      # 读取 Kubernetes ServiceAccount token
      export KUBE_TOKEN=$(cat /var/run/secrets/vault/k8s-token)
      
      # 使用 K8s token 在 Vault 中进行身份验证
      export VAULT_TOKEN=$(vault write -field=token \
        auth/kubernetes/login \
        role=myapp \
        jwt="$KUBE_TOKEN")
      
      # 将 Vault token 写入共享卷
      echo "$VAULT_TOKEN" > /vault-token/token
      
      echo "Vault authentication successful"
    env:
    - name: VAULT_ADDR
      value: "https://vault.example.com:8200"
    - name: VAULT_SKIP_VERIFY
      value: "false"
    - name: VAULT_CACERT
      value: "/etc/vault/ca.crt"
    volumeMounts:
    - name: vault-k8s-token
      mountPath: /var/run/secrets/vault
      readOnly: true
    - name: vault-token
      mountPath: /vault-token
    - name: vault-ca
      mountPath: /etc/vault
      readOnly: true
  
  containers:
  - name: app
    image: myapp:v1.0
    env:
    - name: VAULT_ADDR
      value: "https://vault.example.com:8200"
    - name: VAULT_TOKEN_PATH
      value: "/vault-token/token"
    volumeMounts:
    # 挂载 Vault token
    - name: vault-token
      mountPath: /vault-token
      readOnly: true
    - name: vault-ca
      mountPath: /etc/vault
      readOnly: true
  
  volumes:
  # Kubernetes ServiceAccount token (用于 Vault 认证)
  - name: vault-k8s-token
    projected:
      sources:
      - serviceAccountToken:
          # audience 设置为 Vault 地址
          audience: "vault://vault.example.com"
          # 短期 token: 1小时
          expirationSeconds: 3600
          path: k8s-token
  
  # Vault token 存储 (内存)
  - name: vault-token
    emptyDir:
      medium: Memory
  
  # Vault CA 证书
  - name: vault-ca
    configMap:
      name: vault-ca-cert
```

**Vault 配置**:

```hcl
# Kubernetes 认证后端配置
path "auth/kubernetes/config" {
  capabilities = ["read"]
}

# 定义 myapp 角色
path "auth/kubernetes/role/myapp" {
  bound_service_account_names = ["vault-app-sa"]
  bound_service_account_namespaces = ["default"]
  policies = ["myapp-policy"]
  ttl = 1h
  max_ttl = 4h
}

# myapp 策略
path "secret/data/myapp/*" {
  capabilities = ["read"]
}
```

---

### 8.5 案例 5: 临时 Debug Pod 的 Token 管理

**场景**: SRE 需要创建临时 debug pod 访问集群,使用短期 token。

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: debug-pod
  namespace: kube-system
  labels:
    app: debug
    sre: "true"
spec:
  serviceAccountName: sre-debug-sa
  
  # 临时 Pod,2小时后自动删除
  activeDeadlineSeconds: 7200
  
  containers:
  - name: debug
    image: nicolaka/netshoot:latest
    command: ["sleep", "7200"]
    env:
    - name: KUBECONFIG
      value: "/tmp/kubeconfig"
    volumeMounts:
    - name: debug-token
      mountPath: /var/run/secrets/kubernetes.io/serviceaccount
      readOnly: true
  
  volumes:
  - name: debug-token
    projected:
      sources:
      - serviceAccountToken:
          # 短期 token: 2小时
          expirationSeconds: 7200
          # 绑定到此 Pod
          path: token
      - configMap:
          name: kube-root-ca.crt
          items:
          - key: ca.crt
            path: ca.crt
      - downwardAPI:
          items:
          - path: namespace
            fieldRef:
              fieldPath: metadata.namespace

---
# 创建临时 kubeconfig 的 Job
apiVersion: batch/v1
kind: Job
metadata:
  name: create-debug-kubeconfig
  namespace: kube-system
spec:
  template:
    spec:
      serviceAccountName: sre-debug-sa
      restartPolicy: OnFailure
      containers:
      - name: create-config
        image: bitnami/kubectl:1.29
        command:
        - sh
        - -c
        - |
          # 获取 2小时短期 token
          TOKEN=$(kubectl create token sre-debug-sa \
            --namespace kube-system \
            --duration 2h)
          
          # 创建 kubeconfig
          kubectl config set-cluster kubernetes \
            --server=https://kubernetes.default.svc \
            --certificate-authority=/var/run/secrets/kubernetes.io/serviceaccount/ca.crt \
            --embed-certs=true \
            --kubeconfig=/tmp/kubeconfig
          
          kubectl config set-credentials sre-debug \
            --token="$TOKEN" \
            --kubeconfig=/tmp/kubeconfig
          
          kubectl config set-context debug \
            --cluster=kubernetes \
            --user=sre-debug \
            --namespace=kube-system \
            --kubeconfig=/tmp/kubeconfig
          
          kubectl config use-context debug \
            --kubeconfig=/tmp/kubeconfig
          
          # 输出 kubeconfig (SRE 可以复制使用)
          echo "=== KUBECONFIG (Valid for 2 hours) ==="
          cat /tmp/kubeconfig
          echo "=== END ==="
```

**使用方式**:

```bash
# 1. 创建 debug pod
kubectl apply -f debug-pod.yaml

# 2. 获取 kubeconfig
kubectl logs -n kube-system job/create-debug-kubeconfig

# 3. 复制 kubeconfig 到本地
cat > /tmp/debug-kubeconfig <<'EOF'
# ... 粘贴输出的 kubeconfig ...
EOF

# 4. 使用临时 kubeconfig
export KUBECONFIG=/tmp/debug-kubeconfig
kubectl get pods --all-namespaces

# 5. 2小时后 token 自动失效,kubeconfig 无法使用
```

---

## 9. 最佳实践

### 9.1 安全建议

1. **最小权限原则**:
   - 为每个应用创建专用 ServiceAccount
   - 使用 RBAC 限制 ServiceAccount 的权限
   - 不使用 `default` ServiceAccount

2. **禁用不必要的 Token 挂载**:
   ```yaml
   automountServiceAccountToken: false
   ```

3. **使用短期 Token**:
   - 优先使用 Bound Service Account Token (v1.22+)
   - 设置合理的 `expirationSeconds` (建议 1-4 小时)
   - 避免创建长期 token Secret

4. **Token 绑定**:
   - 使用 `boundObjectRef` 将 token 绑定到 Pod
   - 设置 `audience` 限制 token 的使用范围

5. **避免 Token 泄露**:
   - 不要将 token 记录到日志
   - 不要通过环境变量传递 token (优先使用 volume)
   - 使用 `emptyDir.medium: Memory` 存储 token

### 9.2 运维建议

1. **监控 Token 使用**:
   ```bash
   # 查看 ServiceAccount
   kubectl get serviceaccounts --all-namespaces
   
   # 查看 Pod 使用的 ServiceAccount
   kubectl get pods -o custom-columns=\
   NAME:.metadata.name,\
   SA:.spec.serviceAccountName,\
   AUTOMOUNT:.spec.automountServiceAccountToken
   
   # 查看 CSR
   kubectl get csr
   ```

2. **审计 Token 访问**:
   - 启用 Audit Logging
   - 监控异常的 API 访问
   - 定期审查 ServiceAccount 权限

3. **定期清理**:
   ```bash
   # 清理未使用的 ServiceAccount
   kubectl get sa --all-namespaces | grep -v "default\|system:"
   
   # 清理旧的 CSR
   kubectl delete csr $(kubectl get csr -o jsonpath='{.items[?(@.status.conditions[0].type=="Approved")].metadata.name}')
   ```

### 9.3 迁移到 Bound Token

如果集群从 v1.21- 升级到 v1.22+,需要迁移到 Bound Token:

1. **检查现有 Token Secret**:
   ```bash
   kubectl get secrets --all-namespaces -o json | \
     jq -r '.items[] | select(.type=="kubernetes.io/service-account-token") | "\(.metadata.namespace)/\(.metadata.name)"'
   ```

2. **更新应用配置**:
   - 移除对长期 token Secret 的引用
   - 使用 Projected Volume 自动挂载 token
   - 或使用 `kubectl create token` 获取短期 token

3. **测试兼容性**:
   - 确保应用能正确读取 `/var/run/secrets/kubernetes.io/serviceaccount/token`
   - 验证 token 自动轮转功能
   - 测试 token 过期后的行为

---

## 10. 常见问题排查

### 10.1 Token 无效或过期

**症状**:
```
error: You must be logged in to the server (Unauthorized)
```

**排查步骤**:

```bash
# 1. 检查 token 是否存在
kubectl exec -it <pod> -- ls -la /var/run/secrets/kubernetes.io/serviceaccount/

# 2. 查看 token 内容
kubectl exec -it <pod> -- cat /var/run/secrets/kubernetes.io/serviceaccount/token

# 3. 验证 token (在 Pod 外部)
TOKEN=$(kubectl exec <pod> -- cat /var/run/secrets/kubernetes.io/serviceaccount/token)
kubectl create -f - <<EOF
apiVersion: authentication.k8s.io/v1
kind: TokenReview
spec:
  token: "$TOKEN"
EOF

# 4. 检查 Pod 的 ServiceAccount 配置
kubectl get pod <pod> -o jsonpath='{.spec.serviceAccountName}'

# 5. 检查 ServiceAccount 是否存在
kubectl get sa <serviceaccount> -o yaml
```

### 10.2 Token 未自动挂载

**症状**:
```
/var/run/secrets/kubernetes.io/serviceaccount/token: No such file or directory
```

**排查步骤**:

```bash
# 1. 检查 ServiceAccount 的 automountServiceAccountToken
kubectl get sa <serviceaccount> -o jsonpath='{.automountServiceAccountToken}'

# 2. 检查 Pod 的 automountServiceAccountToken
kubectl get pod <pod> -o jsonpath='{.spec.automountServiceAccountToken}'

# 3. 检查 Pod 的 volumes
kubectl get pod <pod> -o jsonpath='{.spec.volumes}' | jq .

# 4. 检查 kubelet 日志
# 在节点上执行
journalctl -u kubelet | grep -i "serviceaccount"
```

### 10.3 CSR 未被批准

**症状**:
```
certificatesigningrequests.certificates.k8s.io "my-csr" is forbidden
```

**排查步骤**:

```bash
# 1. 查看 CSR 状态
kubectl get csr <csr-name> -o yaml

# 2. 查看 CSR controller 日志
kubectl logs -n kube-system -l component=kube-controller-manager | grep csr

# 3. 手动批准 CSR
kubectl certificate approve <csr-name>

# 4. 检查 CSR 签名者权限
kubectl auth can-i approve certificatesigningrequests --as=system:serviceaccount:<namespace>:<sa-name>
```

---

## 11. 参考资料

- [Kubernetes ServiceAccount 官方文档](https://kubernetes.io/docs/concepts/security/service-accounts/)
- [Bound Service Account Token 提案](https://github.com/kubernetes/enhancements/tree/master/keps/sig-auth/1205-bound-service-account-tokens)
- [TokenRequest API](https://kubernetes.io/docs/reference/kubernetes-api/authentication-resources/token-request-v1/)
- [Certificate Signing Requests](https://kubernetes.io/docs/reference/access-authn-authz/certificate-signing-requests/)
- [Kubernetes RBAC 官方文档](https://kubernetes.io/docs/reference/access-authn-authz/rbac/)

---

**文档版本**: v1.0  
**最后更新**: 2026-02  
**维护者**: Kubernetes 中文社区  
**适用版本**: Kubernetes v1.25 - v1.32
