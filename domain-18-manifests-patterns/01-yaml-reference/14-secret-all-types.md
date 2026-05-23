---
title: 14 - Secret 全类型 YAML 配置参考
description: '# 14 - Secret 全类型 YAML 配置参考'
category: yaml-manifests
tags:
- k8s
- yaml
- manifest
- template
- etcd
- apiserver
- kubelet
- helm
- argocd
- docker
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- SRE
- 开发工程师
- 运维工程师
estimated_read_time: 5min
intent_queries:
- Secret 全类型 YAML 配置参考 是什么
- 如何 Secret 全类型 YAML 配置参考
- Kubernetes 32 yaml manifests 最佳实践
trigger_keywords:
- Secret
- 全类型
- YAML
- 配置参考
- yaml
- manifests
prerequisites:
- kubectl-basics
- helm-basics
- gitops-basics
- etcd-basics
- mysql-basics
- tls-basics
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
created: "2026-05-23"
---

# 14 - Secret 全类型 YAML 配置参考

> **适用版本**: [[Kubernetes|Kubernetes]] v1.25 - v1.32 | **最后更新**: 2026-02  
> **相关领域**: [域3-存储与配置](../domain-3-storage/) | **前置知识**: Pod, ConfigMap  
> **关联配置**: [13-ConfigMap参考](./13-configmap-reference.md) | [RBAC配置](./07-rbac-complete.md)

---

<!-- chunk: 📋 目录 -->## 📋 目录

1. [API 概述与版本](#api-概述与版本)
2. [Secret 类型完整列表](#secret-类型完整列表)
3. [Opaque 通用 Secret](#opaque-通用-secret)
4. [kubernetes.io/service-account-token](#kubernetesioservice-account-token)
5. [kubernetes.io/dockerconfigjson](#kubernetesiodockerconfigjson)
6. [kubernetes.io/basic-auth](#kubernetesiobasic-auth)
7. [kubernetes.io/ssh-auth](#kubernetesiossh-auth)
8. [kubernetes.io/tls](#kubernetesiotls)
9. [bootstrap.kubernetes.io/token](#bootstrapkubernetesiotoken)
10. [内部实现原理](#内部实现原理)
11. [生产实战案例](#生产实战案例)
12. [版本兼容性与最佳实践](#版本兼容性与最佳实践)

---

<!-- chunk: API 概述与版本 -->## API 概述与版本

#<!-- chunk: 基本信息 -->## 基本信息

| 属性 | 值 |
|------|-----|
| **API Group** | `` (core) |
| **API Version** | `v1` |
| **Kind** | `Secret` |
| **命名空间作用域** | ✅ 是 |
| **缩写** | 无 |

#<!-- chunk: 核心特性 -->## 核心特性

```yaml
# Secret 与 ConfigMap 的差异
特性对比:
1. 数据编码:     Secret (Base64)  vs ConfigMap (明文)
2. etcd 加密:    Secret (可启用)  vs ConfigMap (不支持)
3. 挂载方式:     Secret (tmpfs)   vs ConfigMap (tmpfs)
4. API 权限:     Secret (RBAC严格) vs ConfigMap (常规)
5. 审计日志:     Secret (敏感字段脱敏) vs ConfigMap (完整记录)
```

#<!-- chunk: 安全模型 -->## 安全模型

| 维度 | 说明 | 配置项 |
|------|------|--------|
| **传输加密** | API Server 到 etcd 使用 TLS | 默认启用 |
| **静态加密** | etcd 中数据加密 | 需配置 EncryptionConfiguration |
| **内存挂载** | tmpfs, 永不写入磁盘 | [[kubelet|kubelet]] 自动处理 |
| **权限控制** | RBAC 细粒度控制 | 最小权限原则 |

---

<!-- chunk: Secret 类型完整列表 -->## Secret 类型完整列表

#<!-- chunk: 内置类型表 -->## 内置类型表

| Type | 用途 | 必需字段 | 版本 |
|------|------|----------|------|
| `Opaque` | 通用键值对(默认) | 无 | v1 |
| `kubernetes.io/service-account-token` | ServiceAccount 令牌 (遗留) | `kubernetes.io/service-account.name` | v1 (Deprecated) |
| `kubernetes.io/dockercfg` | `.dockercfg` 文件 (遗留) | `.dockercfg` | v1 (Deprecated) |
| `kubernetes.io/dockerconfigjson` | `.docker/config.json` | `.dockerconfigjson` | v1 |
| `kubernetes.io/basic-auth` | HTTP Basic 认证 | `username`, `password` | v1 |
| `kubernetes.io/ssh-auth` | SSH 私钥 | `ssh-privatekey` | v1 |
| `kubernetes.io/tls` | TLS 证书和私钥 | `tls.crt`, `tls.key` | v1 |
| `bootstrap.kubernetes.io/token` | Bootstrap Token | `token-id`, `token-secret` | v1 |

#<!-- chunk: 字段规格表 -->## 字段规格表

| 字段路径 | 类型 | 必填 | 版本 | 说明 |
|----------|------|------|------|------|
| `apiVersion` | string | ✅ | v1 | 固定为 `v1` |
| `kind` | string | ✅ | v1 | 固定为 `Secret` |
| `metadata.name` | string | ✅ | v1 | Secret 名称 |
| `metadata.namespace` | string | ❌ | v1 | 命名空间(默认 default) |
| `type` | string | ❌ | v1 | Secret 类型(默认 Opaque) |
| `data` | map[string][]byte | ❌ | v1 | Base64 编码的键值对 |
| `stringData` | map[string]string | ❌ | v1 | 明文字符串(自动转 Base64) |
| `immutable` | bool | ❌ | v1.21+ | 不可变标记 |

---

<!-- chunk: Opaque 通用 Secret -->## Opaque 通用 Secret

#<!-- chunk: 基础示例 -->## 基础示例

```yaml
apiVersion: v1
kind: Secret
metadata:
  name: app-secrets
  namespace: default
# 默认类型: Opaque (通用键值对)
type: Opaque
data:
  # Base64 编码的值
  # echo -n "password123" | base64 => cGFzc3dvcmQxMjM=
  database_password: cGFzc3dvcmQxMjM=
  api_key: YWJjZGVmZ2hpamtsbW5vcHFyc3R1dnd4eXo=
```

#<!-- chunk: 使用 stringData (推荐) -->## 使用 stringData (推荐)

```yaml
apiVersion: v1
kind: Secret
metadata:
  name: app-secrets-plain
  namespace: default
type: Opaque
# stringData: 明文输入, Kubernetes 自动转换为 Base64
stringData:
  database_password: "password123"
  api_key: "abcdefghijklmnopqrstuvwxyz"
  connection_string: "Server=mysql.default.svc;Database=mydb;Uid=root;Pwd=secret;"

# 注意: stringData 只在创建/更新时使用, kubectl get 时会转换为 data
```

#<!-- chunk: 环境变量注入 -->## 环境变量注入

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: app-pod
spec:
  containers:
  - name: app
    image: myapp:latest
    env:
    # 单个 Secret 键作为环境变量
    - name: DATABASE_PASSWORD
      valueFrom:
        secretKeyRef:
          name: app-secrets
          key: database_password
    
    - name: API_KEY
      valueFrom:
        secretKeyRef:
          name: app-secrets
          key: api_key
          optional: false  # 默认: Secret 不存在时 Pod 启动失败
    
    # 可选 Secret (不存在时不报错)
    - name: OPTIONAL_KEY
      valueFrom:
        secretKeyRef:
          name: optional-secret
          key: some_key
          optional: true
```

#<!-- chunk: envFrom 批量注入 -->## envFrom 批量注入

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: envfrom-pod
spec:
  containers:
  - name: app
    image: myapp:latest
    # 将 Secret 所有键作为环境变量
    envFrom:
    - secretRef:
        name: app-secrets
    
    # 结果: 容器中自动创建环境变量
    # database_password=password123
    # api_key=abcdefghijklmnopqrstuvwxyz
```

#<!-- chunk: Volume 挂载 -->## Volume 挂载

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: volume-secret-pod
spec:
  containers:
  - name: app
    image: myapp:latest
    volumeMounts:
    # 挂载 Secret 为文件
    - name: secret-volume
      mountPath: /etc/secrets
      readOnly: true
    
    # 结果: 在容器中生成文件
    # /etc/secrets/database_password (内容: password123)
    # /etc/secrets/api_key (内容: abcdefghijklmnopqrstuvwxyz)
  
  volumes:
  - name: secret-volume
    secret:
      secretName: app-secrets
      # 可选: 设置文件权限
      defaultMode: 0400  # r-------- (仅 owner 可读)
```

#<!-- chunk: kubectl 创建 Opaque Secret -->## kubectl 创建 Opaque Secret

```bash
# 从字面量创建
kubectl create secret generic db-secret \
  --from-literal=username=admin \
  --from-literal=password=secret123

# 从文件创建
echo -n "password123" > password.txt
kubectl create secret generic file-secret \
  --from-file=password=password.txt

# 从 .env 文件创建
cat > .env <<EOF
DB_HOST=mysql.default.svc
DB_PORT=3306
DB_PASSWORD=secret
EOF
kubectl create secret generic env-secret \
  --from-env-file=.env

# 从目录创建 (目录下所有文件)
mkdir secrets
echo "secret1" > secrets/key1
echo "secret2" > secrets/key2
kubectl create secret generic dir-secret \
  --from-file=secrets/
```

---

<!-- chunk: kubernetes.io/service-account-token -->## kubernetes.io/service-account-token

#<!-- chunk: 说明 -->## 说明

| 属性 | 值 |
|------|-----|
| **用途** | ServiceAccount 的 API 令牌 (遗留方式) |
| **状态** | Deprecated (自 v1.22, 推荐使用 TokenRequest API) |
| **自动创建** | v1.24+ 默认禁用, 需显式创建 |

#<!-- chunk: 遗留方式 (v1.23 之前) -->## 遗留方式 (v1.23 之前)

```yaml
# Kubernetes v1.23 之前自动创建
apiVersion: v1
kind: ServiceAccount
metadata:
  name: my-sa
  namespace: default
---
# 自动生成的 Secret (已废弃)
apiVersion: v1
kind: Secret
metadata:
  name: my-sa-token-xxxxx
  namespace: default
  annotations:
    kubernetes.io/service-account.name: my-sa
type: kubernetes.io/service-account-token
data:
  ca.crt: <base64-ca-cert>
  namespace: ZGVmYXVsdA==  # base64: default
  token: <base64-jwt-token>
```

#<!-- chunk: 显式创建 (v1.24+) -->## 显式创建 (v1.24+)

```yaml
# v1.24+ 需要显式创建 (非推荐方式)
apiVersion: v1
kind: Secret
metadata:
  name: my-sa-token
  namespace: default
  annotations:
    kubernetes.io/service-account.name: my-sa
type: kubernetes.io/service-account-token
# Kubernetes 自动填充 data 字段
```

#<!-- chunk: 推荐方式: TokenRequest API (v1.22+) -->## 推荐方式: TokenRequest API (v1.22+)

```yaml
# 方式1: Pod 自动挂载 (默认行为)
apiVersion: v1
kind: Pod
metadata:
  name: app-pod
spec:
  serviceAccountName: my-sa
  containers:
  - name: app
    image: myapp:latest
    # 自动挂载 Token 到:
    # /var/run/secrets/kubernetes.io/serviceaccount/token
    # (短期 Token, 自动轮换)

---
# 方式2: 显式投射 Token (推荐)
apiVersion: v1
kind: Pod
metadata:
  name: projected-token-pod
spec:
  serviceAccountName: my-sa
  containers:
  - name: app
    image: myapp:latest
    volumeMounts:
    - name: sa-token
      mountPath: /var/run/secrets/tokens
      readOnly: true
  
  volumes:
  - name: sa-token
    projected:
      sources:
      - serviceAccountToken:
          path: my-sa-token
          expirationSeconds: 3600  # 1小时过期
          audience: "https://kubernetes.default.svc"
```

---

<!-- chunk: kubernetes.io/dockerconfigjson -->## kubernetes.io/dockerconfigjson

#<!-- chunk: 基础示例 -->## 基础示例

```yaml
apiVersion: v1
kind: Secret
metadata:
  name: docker-registry-secret
  namespace: default
type: kubernetes.io/dockerconfigjson
data:
  # Base64 编码的 Docker config.json
  .dockerconfigjson: eyJhdXRocyI6eyJyZWdpc3RyeS5leGFtcGxlLmNvbSI6eyJ1c2VybmFtZSI6ImFkbWluIiwicGFzc3dvcmQiOiJwYXNzd29yZCIsImF1dGgiOiJZV1J0YVc0NmNHRnpjM2R2Y21RPSJ9fX0=
```

#<!-- chunk: JSON 格式解析 -->## JSON 格式解析

```json
// .dockerconfigjson 解码后的内容:
{
  "auths": {
    "registry.example.com": {
      "username": "admin",
      "password": "password",
      "email": "admin@example.com",  // 可选
      "auth": "YWRtaW46cGFzc3dvcmQ="  // base64(username:password)
    },
    "docker.io": {
      "username": "myuser",
      "password": "mypassword",
      "auth": "bXl1c2VyOm15cGFzc3dvcmQ="
    }
  }
}
```

#<!-- chunk: kubectl 创建 -->## kubectl 创建

```bash
# 方式1: 命令行参数
kubectl create secret docker-registry docker-secret \
  --docker-server=registry.example.com \
  --docker-username=admin \
  --docker-password=password \
  --docker-email=admin@example.com

# 方式2: 从现有 Docker config
kubectl create secret generic docker-config-secret \
  --from-file=.dockerconfigjson=$HOME/.docker/config.json \
  --type=kubernetes.io/dockerconfigjson

# 方式3: 多仓库配置
cat > config.json <<EOF
{
  "auths": {
    "registry.example.com": {
      "auth": "$(echo -n 'admin:password' | base64)"
    },
    "docker.io": {
      "auth": "$(echo -n 'user:pass' | base64)"
    }
  }
}
EOF
kubectl create secret generic multi-registry-secret \
  --from-file=.dockerconfigjson=config.json \
  --type=kubernetes.io/dockerconfigjson
```

#<!-- chunk: Pod 使用镜像拉取凭证 -->## Pod 使用镜像拉取凭证

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: private-image-pod
spec:
  # 方式1: 单个 Secret
  imagePullSecrets:
  - name: docker-secret
  
  containers:
  - name: app
    image: registry.example.com/myapp:latest

---
# 方式2: ServiceAccount 默认凭证
apiVersion: v1
kind: ServiceAccount
metadata:
  name: my-sa
  namespace: default
imagePullSecrets:
- name: docker-secret
- name: another-registry-secret
---
apiVersion: v1
kind: Pod
metadata:
  name: sa-image-pull-pod
spec:
  serviceAccountName: my-sa
  containers:
  - name: app
    image: registry.example.com/myapp:latest
```

---

<!-- chunk: kubernetes.io/basic-auth -->## kubernetes.io/basic-auth

#<!-- chunk: 基础示例 -->## 基础示例

```yaml
apiVersion: v1
kind: Secret
metadata:
  name: basic-auth-secret
  namespace: default
type: kubernetes.io/basic-auth
stringData:
  username: admin
  password: password123
  # 可选: 额外的非标准字段
  # extra-field: value
```

#<!-- chunk: 必需字段 -->## 必需字段

| 字段 | 必需 | 说明 |
|------|------|------|
| `username` | ✅ | 用户名 |
| `password` | ✅ | 密码 |

#<!-- chunk: 使用示例: Ingress Basic Auth -->## 使用示例: Ingress Basic Auth

```yaml
# Nginx Ingress Controller Basic Auth
apiVersion: v1
kind: Secret
metadata:
  name: ingress-basic-auth
  namespace: web
type: kubernetes.io/basic-auth
stringData:
  # 用户名: admin
  username: admin
  # 密码: secret (使用 htpasswd 生成)
  # htpasswd -nb admin secret
  password: secret

---
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: protected-app
  namespace: web
  annotations:
    # Nginx Ingress 启用 Basic Auth
    nginx.ingress.kubernetes.io/auth-type: basic
    nginx.ingress.kubernetes.io/auth-secret: ingress-basic-auth
    nginx.ingress.kubernetes.io/auth-realm: "Authentication Required"
spec:
  rules:
  - host: app.example.com
    http:
      paths:
      - path: /
        pathType: Prefix
        backend:
          service:
            name: app-service
            port:
              number: 80
```

#<!-- chunk: 使用示例: HTTP 客户端认证 -->## 使用示例: HTTP 客户端认证

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: http-client-pod
spec:
  containers:
  - name: client
    image: curlimages/curl:latest
    command:
    - sh
    - -c
    - |
      # 读取 Basic Auth 凭证
      USERNAME=$(cat /etc/secrets/username)
      PASSWORD=$(cat /etc/secrets/password)
      
      # 使用 Basic Auth 访问 API
      curl -u $USERNAME:$PASSWORD https://api.example.com/data
    volumeMounts:
    - name: basic-auth
      mountPath: /etc/secrets
      readOnly: true
  
  volumes:
  - name: basic-auth
    secret:
      secretName: basic-auth-secret
```

---

<!-- chunk: kubernetes.io/ssh-auth -->## kubernetes.io/ssh-auth

#<!-- chunk: 基础示例 -->## 基础示例

```yaml
apiVersion: v1
kind: Secret
metadata:
  name: ssh-key-secret
  namespace: default
type: kubernetes.io/ssh-auth
stringData:
  # SSH 私钥 (PEM 格式)
  ssh-privatekey: |
    -----BEGIN OPENSSH PRIVATE KEY-----
    b3BlbnNzaC1rZXktdjEAAAAABG5vbmUAAAAEbm9uZQAAAAAAAAABAAABFwAAAAdzc2gtcn
    NhAAAAAwEAAQAAAQEA1234567890abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOP
    QRSTUVWXYZ...
    -----END OPENSSH PRIVATE KEY-----
```

#<!-- chunk: 必需字段 -->## 必需字段

| 字段 | 必需 | 说明 |
|------|------|------|
| `ssh-privatekey` | ✅ | SSH 私钥 (PEM 格式) |

#<!-- chunk: kubectl 创建 -->## kubectl 创建

```bash
# 从 SSH 私钥文件创建
kubectl create secret generic ssh-secret \
  --from-file=ssh-privatekey=$HOME/.ssh/id_rsa \
  --type=kubernetes.io/ssh-auth
```

#<!-- chunk: 使用示例: Git Clone -->## 使用示例: Git Clone

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: git-clone-pod
spec:
  initContainers:
  # InitContainer: 克隆 Git 仓库
  - name: git-clone
    image: alpine/git:latest
    command:
    - sh
    - -c
    - |
      # 设置 SSH 密钥权限
      mkdir -p /root/.ssh
      cp /etc/git-secret/ssh-privatekey /root/.ssh/id_rsa
      chmod 600 /root/.ssh/id_rsa
      
      # 添加 Git 服务器到 known_hosts
      ssh-keyscan github.com >> /root/.ssh/known_hosts
      
      # 克隆私有仓库
      git clone git@github.com:mycompany/private-repo.git /workspace
    volumeMounts:
    - name: ssh-key
      mountPath: /etc/git-secret
      readOnly: true
    - name: workspace
      mountPath: /workspace
  
  containers:
  - name: app
    image: myapp:latest
    volumeMounts:
    - name: workspace
      mountPath: /app
  
  volumes:
  - name: ssh-key
    secret:
      secretName: ssh-key-secret
      defaultMode: 0400
  - name: workspace
    emptyDir: {}
```

#<!-- chunk: 使用示例: SSH 客户端连接 -->## 使用示例: SSH 客户端连接

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: ssh-client-pod
spec:
  containers:
  - name: ssh-client
    image: alpine:latest
    command:
    - sh
    - -c
    - |
      apk add --no-cache openssh-client
      
      # 配置 SSH 密钥
      mkdir -p /root/.ssh
      cp /etc/ssh-secret/ssh-privatekey /root/.ssh/id_rsa
      chmod 600 /root/.ssh/id_rsa
      
      # SSH 连接到远程服务器
      ssh -o StrictHostKeyChecking=no user@remote-server.example.com "ls -la"
    volumeMounts:
    - name: ssh-key
      mountPath: /etc/ssh-secret
      readOnly: true
  
  volumes:
  - name: ssh-key
    secret:
      secretName: ssh-key-secret
      defaultMode: 0400
```

---

<!-- chunk: kubernetes.io/tls -->## kubernetes.io/tls

#<!-- chunk: 基础示例 -->## 基础示例

```yaml
apiVersion: v1
kind: Secret
metadata:
  name: tls-secret
  namespace: default
type: kubernetes.io/tls
data:
  # Base64 编码的证书和私钥
  tls.crt: LS0tLS1CRUdJTiBDRVJUSUZJQ0FURS0tLS0t...
  tls.key: LS0tLS1CRUdJTiBSU0EgUFJJVkFURSBLRVktLS0tLQ==
```

#<!-- chunk: 必需字段 -->## 必需字段

| 字段 | 必需 | 说明 |
|------|------|------|
| `tls.crt` | ✅ | TLS 证书 (PEM 格式) |
| `tls.key` | ✅ | TLS 私钥 (PEM 格式) |

#<!-- chunk: kubectl 创建 -->## kubectl 创建

```bash
# 从证书文件创建
kubectl create secret tls tls-secret \
  --cert=tls.crt \
  --key=tls.key

# 使用 stringData (YAML)
kubectl apply -f - <<EOF
apiVersion: v1
kind: Secret
metadata:
  name: tls-secret
type: kubernetes.io/tls
stringData:
  tls.crt: |
    -----BEGIN CERTIFICATE-----
    MIIDXTCCAkWgAwIBAgIJAK...
    -----END CERTIFICATE-----
  tls.key: |
    -----BEGIN RSA PRIVATE KEY-----
    MIIEpAIBAAKCAQEA1234...
    -----END RSA PRIVATE KEY-----
EOF
```

#<!-- chunk: 使用示例: Ingress TLS -->## 使用示例: Ingress TLS

```yaml
apiVersion: v1
kind: Secret
metadata:
  name: example-tls
  namespace: web
type: kubernetes.io/tls
stringData:
  tls.crt: |
    -----BEGIN CERTIFICATE-----
    # 证书内容 (包括中间证书链)
    -----END CERTIFICATE-----
  tls.key: |
    -----BEGIN RSA PRIVATE KEY-----
    # 私钥内容
    -----END RSA PRIVATE KEY-----

---
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: tls-ingress
  namespace: web
spec:
  # TLS 配置
  tls:
  - hosts:
    - app.example.com
    - www.example.com
    secretName: example-tls
  
  rules:
  - host: app.example.com
    http:
      paths:
      - path: /
        pathType: Prefix
        backend:
          service:
            name: app-service
            port:
              number: 80
```

#<!-- chunk: 使用示例: 应用 mTLS -->## 使用示例: 应用 mTLS

```yaml
# 服务端证书
apiVersion: v1
kind: Secret
metadata:
  name: server-tls
  namespace: default
type: kubernetes.io/tls
stringData:
  tls.crt: |
    -----BEGIN CERTIFICATE-----
    # 服务端证书
    -----END CERTIFICATE-----
  tls.key: |
    -----BEGIN RSA PRIVATE KEY-----
    # 服务端私钥
    -----END RSA PRIVATE KEY-----

---
# 客户端 CA 证书 (验证客户端)
apiVersion: v1
kind: Secret
metadata:
  name: client-ca
  namespace: default
type: Opaque
stringData:
  ca.crt: |
    -----BEGIN CERTIFICATE-----
    # 客户端 CA 证书
    -----END CERTIFICATE-----

---
apiVersion: v1
kind: Pod
metadata:
  name: mtls-server
spec:
  containers:
  - name: nginx
    image: nginx:1.25
    ports:
    - containerPort: 443
    volumeMounts:
    # 服务端证书
    - name: server-tls
      mountPath: /etc/nginx/ssl
      readOnly: true
    # 客户端 CA (验证客户端证书)
    - name: client-ca
      mountPath: /etc/nginx/client-ca
      readOnly: true
    # Nginx 配置
    - name: nginx-config
      mountPath: /etc/nginx/nginx.conf
      subPath: nginx.conf
  
  volumes:
  - name: server-tls
    secret:
      secretName: server-tls
  - name: client-ca
    secret:
      secretName: client-ca
  - name: nginx-config
    configMap:
      name: nginx-mtls-config
---
# Nginx mTLS 配置
apiVersion: v1
kind: ConfigMap
metadata:
  name: nginx-mtls-config
data:
  nginx.conf: |
    events {}
    http {
      server {
        listen 443 ssl;
        
        # 服务端证书
        ssl_certificate /etc/nginx/ssl/tls.crt;
        ssl_certificate_key /etc/nginx/ssl/tls.key;
        
        # 客户端证书验证
        ssl_client_certificate /etc/nginx/client-ca/ca.crt;
        ssl_verify_client on;
        
        location / {
          return 200 "mTLS Success\n";
        }
      }
    }
```

#<!-- chunk: cert-manager 自动管理 -->## cert-manager 自动管理

```yaml
# 使用 cert-manager 自动生成和续订证书
apiVersion: cert-manager.io/v1
kind: Certificate
metadata:
  name: example-com-tls
  namespace: web
spec:
  # 自动创建的 Secret 名称
  secretName: example-com-tls
  
  # 证书有效期
  duration: 2160h  # 90 天
  renewBefore: 360h  # 提前 15 天续订
  
  # DNS 名称
  dnsNames:
  - example.com
  - www.example.com
  - "*.example.com"
  
  # 证书颁发者
  issuerRef:
    name: letsencrypt-prod
    kind: ClusterIssuer
    group: cert-manager.io

# cert-manager 会自动创建 kubernetes.io/tls 类型的 Secret
```

---

<!-- chunk: bootstrap.kubernetes.io/token -->## bootstrap.kubernetes.io/token

#<!-- chunk: 基础示例 -->## 基础示例

```yaml
apiVersion: v1
kind: Secret
metadata:
  name: bootstrap-token-abc123
  namespace: kube-system
type: bootstrap.kubernetes.io/token
stringData:
  # 必需: Token ID (6个字符)
  token-id: abc123
  
  # 必需: Token Secret (16个字符)
  token-secret: 0123456789abcdef
  
  # 可选: 描述
  description: "Bootstrap token for new nodes"
  
  # 可选: 过期时间 (RFC3339 格式)
  expiration: "2026-12-31T23:59:59Z"
  
  # 可选: 用途 (逗号分隔)
  usage-bootstrap-authentication: "true"
  usage-bootstrap-signing: "true"
  
  # 可选: 允许的认证组
  auth-extra-groups: "system:bootstrappers:worker"
```

#<!-- chunk: 必需字段 -->## 必需字段

| 字段 | 必需 | 格式 | 说明 |
|------|------|------|------|
| `token-id` | ✅ | `[a-z0-9]{6}` | Token 标识符 (6个小写字母或数字) |
| `token-secret` | ✅ | `[a-z0-9]{16}` | Token 密钥 (16个小写字母或数字) |

#<!-- chunk: 创建 Bootstrap Token -->## 创建 Bootstrap Token

```bash
# 使用 kubeadm 创建 (推荐)
kubeadm token create \
  --description "Node join token" \
  --ttl 24h \
  --usages "signing,authentication"

# 输出示例:
# abc123.0123456789abcdef

# 手动创建 Secret
kubectl apply -f - <<EOF
apiVersion: v1
kind: Secret
metadata:
  name: bootstrap-token-abc123
  namespace: kube-system
type: bootstrap.kubernetes.io/token
stringData:
  token-id: abc123
  token-secret: 0123456789abcdef
  description: "Manually created bootstrap token"
  expiration: "$(date -u -d '+24 hours' --rfc-3339=seconds | sed 's/ /T/')"
  usage-bootstrap-authentication: "true"
  usage-bootstrap-signing: "true"
  auth-extra-groups: "system:bootstrappers:default-node-token"
EOF
```

#<!-- chunk: 使用场景: 节点加入集群 -->## 使用场景: 节点加入集群

```bash
# 新节点使用 Bootstrap Token 加入集群
kubeadm join <control-plane-endpoint>:6443 \
  --token abc123.0123456789abcdef \
  --discovery-token-ca-cert-hash sha256:<hash> \
  --node-name worker-node-1
```

---

<!-- chunk: 内部实现原理 -->## 内部实现原理

#<!-- chunk: etcd 静态加密配置 -->## etcd 静态加密配置

```yaml
# EncryptionConfiguration 启用 Secret 加密
apiVersion: apiserver.config.k8s.io/v1
kind: EncryptionConfiguration
resources:
  # 加密 Secret 资源
  - resources:
    - secrets
    providers:
    # 提供者优先级从高到低
    # 1. AES-CBC 加密
    - aescbc:
        keys:
        - name: key1
          secret: <base64-encoded-32-byte-key>
    
    # 2. 身份提供者 (不加密, 用于解密旧数据)
    - identity: {}

# API Server 启动参数
# --encryption-provider-config=/etc/kubernetes/encryption-config.yaml

# 验证加密状态
# etcdctl get /registry/secrets/default/my-secret
# 加密成功: k8s:enc:aescbc:v1:key1:...
# 未加密:   k8s:...
```

#<!-- chunk: 加密算法对比 -->## 加密算法对比

| 提供者 | 算法 | 性能 | 安全性 | 推荐场景 |
|--------|------|------|--------|----------|
| `identity` | 无加密 | 最快 | 低 | 开发环境 |
| `aescbc` | AES-CBC | 中等 | 中 | 通用生产环境 |
| `aesgcm` | AES-GCM | 快 | 高 | 高性能需求 |
| `secretbox` | XSalsa20+Poly1305 | 快 | 高 | 现代加密需求 |
| `kms` | 外部 KMS (如 AWS KMS) | 慢 | 最高 | 合规要求 |

#<!-- chunk: tmpfs 内存挂载 -->## tmpfs 内存挂载

```yaml
# Secret 永远不会写入节点磁盘, 仅存在于内存 tmpfs 中

# 查看 Secret 挂载
apiVersion: v1
kind: Pod
metadata:
  name: secret-mount-test
spec:
  containers:
  - name: test
    image: busybox
    command: ["sh", "-c", "sleep 3600"]
    volumeMounts:
    - name: secret-vol
      mountPath: /etc/secrets
  volumes:
  - name: secret-vol
    secret:
      secretName: my-secret

# 进入容器查看挂载类型
# kubectl exec secret-mount-test -- mount | grep secrets
# tmpfs on /etc/secrets type tmpfs (ro,relatime)
```

#<!-- chunk: Base64 编码原理 -->## Base64 编码原理

```yaml
# Base64 不是加密, 仅是编码

# 编码示例:
# echo -n "password123" | base64
# cGFzc3dvcmQxMjM=

# 解码示例:
# echo "cGFzc3dvcmQxMjM=" | base64 -d
# password123

# 注意事项:
# 1. Base64 可轻易解码, 不提供安全性
# 2. 真正的安全依赖 RBAC + etcd 加密 + TLS
# 3. 避免在日志/事件中暴露 Secret 内容
```

#<!-- chunk: Secret 自动更新机制 -->## Secret 自动更新机制

```yaml
# Secret 更新传播到 Pod (与 ConfigMap 相同)

# 1. Volume 挂载: 自动更新 (1-2分钟)
apiVersion: v1
kind: Pod
metadata:
  name: auto-update-pod
spec:
  containers:
  - name: app
    image: myapp:latest
    volumeMounts:
    - name: secret-vol
      mountPath: /etc/secrets
  volumes:
  - name: secret-vol
    secret:
      secretName: app-secret

# 2. 环境变量: 永不更新 (需重启 Pod)
apiVersion: v1
kind: Pod
metadata:
  name: static-env-pod
spec:
  containers:
  - name: app
    image: myapp:latest
    env:
    - name: PASSWORD
      valueFrom:
        secretKeyRef:
          name: app-secret
          key: password

# 3. subPath 挂载: 永不更新 (需重启 Pod)
```

---

<!-- chunk: 生产实战案例 -->## 生产实战案例

#<!-- chunk: 案例1: 数据库密码管理 -->## 案例1: 数据库密码管理

```yaml
# 场景: 安全管理数据库凭证

# 1. 数据库 Secret
apiVersion: v1
kind: Secret
metadata:
  name: mysql-credentials
  namespace: production
type: Opaque
immutable: true  # 生产环境强制不可变
stringData:
  root-password: "SuperSecret123!"
  app-username: "myapp_user"
  app-password: "AppPassword456!"
  connection-string: "Server=mysql-primary.production.svc;Database=myapp;Uid=myapp_user;Pwd=AppPassword456!"

---
# 2. MySQL StatefulSet
apiVersion: apps/v1
kind: StatefulSet
metadata:
  name: mysql
  namespace: production
spec:
  serviceName: mysql
  replicas: 1
  selector:
    matchLabels:
      app: mysql
  template:
    metadata:
      labels:
        app: mysql
    spec:
      containers:
      - name: mysql
        image: mysql:8.0
        env:
        # Root 密码
        - name: MYSQL_ROOT_PASSWORD
          valueFrom:
            secretKeyRef:
              name: mysql-credentials
              key: root-password
        # 应用数据库和用户
        - name: MYSQL_DATABASE
          value: "myapp"
        - name: MYSQL_USER
          valueFrom:
            secretKeyRef:
              name: mysql-credentials
              key: app-username
        - name: MYSQL_PASSWORD
          valueFrom:
            secretKeyRef:
              name: mysql-credentials
              key: app-password
        ports:
        - containerPort: 3306
        volumeMounts:
        - name: data
          mountPath: /var/lib/mysql
  
  volumeClaimTemplates:
  - metadata:
      name: data
    spec:
      accessModes: ["ReadWriteOnce"]
      resources:
        requests:
          storage: 10Gi

---
# 3. 应用 Deployment
apiVersion: apps/v1
kind: Deployment
metadata:
  name: myapp
  namespace: production
spec:
  replicas: 3
  selector:
    matchLabels:
      app: myapp
  template:
    metadata:
      labels:
        app: myapp
    spec:
      containers:
      - name: app
        image: myapp:latest
        env:
        # 注入数据库连接字符串
        - name: DATABASE_CONNECTION_STRING
          valueFrom:
            secretKeyRef:
              name: mysql-credentials
              key: connection-string
```

#<!-- chunk: 案例2: TLS 证书自动化 (cert-manager) -->## 案例2: TLS 证书自动化 (cert-manager)

```yaml
# 场景: 使用 cert-manager 自动管理 Let's Encrypt 证书

# 1. ClusterIssuer (集群级证书颁发者)
apiVersion: cert-manager.io/v1
kind: ClusterIssuer
metadata:
  name: letsencrypt-prod
spec:
  acme:
    # Let's Encrypt 生产环境
    server: https://acme-v02.api.letsencrypt.org/directory
    email: admin@example.com
    privateKeySecretRef:
      name: letsencrypt-prod-account-key
    
    # HTTP-01 挑战验证
    solvers:
    - http01:
        ingress:
          class: nginx

---
# 2. Certificate 资源 (自动创建 Secret)
apiVersion: cert-manager.io/v1
kind: Certificate
metadata:
  name: example-com-tls
  namespace: web
spec:
  # 自动创建的 Secret 名称
  secretName: example-com-tls
  
  # 证书配置
  duration: 2160h  # 90 天
  renewBefore: 360h  # 提前 15 天续订
  
  # 域名列表
  dnsNames:
  - example.com
  - www.example.com
  
  # 证书颁发者
  issuerRef:
    name: letsencrypt-prod
    kind: ClusterIssuer

---
# 3. Ingress 使用自动生成的证书
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: example-ingress
  namespace: web
  annotations:
    # cert-manager 自动管理证书
    cert-manager.io/cluster-issuer: "letsencrypt-prod"
spec:
  tls:
  - hosts:
    - example.com
    - www.example.com
    secretName: example-com-tls  # cert-manager 自动创建此 Secret
  
  rules:
  - host: example.com
    http:
      paths:
      - path: /
        pathType: Prefix
        backend:
          service:
            name: web-service
            port:
              number: 80
```

#<!-- chunk: 案例3: 镜像仓库凭证管理 -->## 案例3: 镜像仓库凭证管理

```yaml
# 场景: 多命名空间共享私有镜像仓库凭证

# 1. 创建镜像拉取 Secret
apiVersion: v1
kind: Secret
metadata:
  name: harbor-registry
  namespace: default
type: kubernetes.io/dockerconfigjson
stringData:
  .dockerconfigjson: |
    {
      "auths": {
        "harbor.company.com": {
          "username": "robot-account",
          "password": "robot-token-abc123",
          "auth": "$(echo -n 'robot-account:robot-token-abc123' | base64)"
        }
      }
    }

---
# 2. 复制到多个命名空间 (使用脚本或 GitOps)
# for ns in team-a team-b team-c; do
#   kubectl get secret harbor-registry -n default -o yaml | \
#   sed "s/namespace: default/namespace: $ns/" | \
#   kubectl apply -f -
# done

---
# 3. ServiceAccount 默认镜像拉取凭证
apiVersion: v1
kind: ServiceAccount
metadata:
  name: default
  namespace: team-a
imagePullSecrets:
- name: harbor-registry

---
# 4. Pod 自动使用凭证
apiVersion: v1
kind: Pod
metadata:
  name: private-image-pod
  namespace: team-a
spec:
  # 使用 default ServiceAccount, 自动获取 imagePullSecrets
  serviceAccountName: default
  containers:
  - name: app
    image: harbor.company.com/myproject/myapp:latest
```

#<!-- chunk: 案例4: External Secrets Operator -->## 案例4: External Secrets Operator

```yaml
# 场景: 从外部密钥管理系统 (如 AWS Secrets Manager) 同步 Secret

# 1. 安装 External Secrets Operator
# helm repo add external-secrets https://charts.external-secrets.io
# helm install external-secrets external-secrets/external-secrets -n external-secrets-system

---
# 2. SecretStore (连接到 AWS Secrets Manager)
apiVersion: external-secrets.io/v1beta1
kind: SecretStore
metadata:
  name: aws-secrets-manager
  namespace: production
spec:
  provider:
    aws:
      service: SecretsManager
      region: us-west-2
      auth:
        secretRef:
          accessKeyIDSecretRef:
            name: aws-credentials
            key: access-key-id
          secretAccessKeySecretRef:
            name: aws-credentials
            key: secret-access-key

---
# 3. ExternalSecret (同步外部密钥到 Kubernetes Secret)
apiVersion: external-secrets.io/v1beta1
kind: ExternalSecret
metadata:
  name: database-credentials
  namespace: production
spec:
  # 刷新间隔
  refreshInterval: 1h
  
  # 关联 SecretStore
  secretStoreRef:
    name: aws-secrets-manager
    kind: SecretStore
  
  # 目标 Kubernetes Secret
  target:
    name: mysql-credentials
    creationPolicy: Owner
  
  # 数据映射
  data:
  - secretKey: username
    remoteRef:
      key: prod/mysql/username
  - secretKey: password
    remoteRef:
      key: prod/mysql/password

# External Secrets Operator 会自动创建并同步 Secret:
# apiVersion: v1
# kind: Secret
# metadata:
#   name: mysql-credentials
#   namespace: production
# type: Opaque
# data:
#   username: <synced-value>
#   password: <synced-value>
```

---

<!-- chunk: 版本兼容性与最佳实践 -->## 版本兼容性与最佳实践

#<!-- chunk: 版本演进 -->## 版本演进

| Kubernetes 版本 | Secret 变更 |
|-----------------|-------------|
| v1.21+ | `immutable` 字段 (GA) |
| v1.22+ | ServiceAccount Token 不再自动创建 Secret |
| v1.24+ | ServiceAccount Token 默认使用 TokenRequest API |
| v1.25+ | 移除 `.dockercfg` 支持警告 |
| v1.32+ | 无重大变更 |

#<!-- chunk: 最佳实践 -->## 最佳实践

##<!-- chunk: 1. 启用 etcd 加密 -->## 1. 启用 etcd 加密

```yaml
# 生产环境必须启用 Secret 加密
apiVersion: apiserver.config.k8s.io/v1
kind: EncryptionConfiguration
resources:
  - resources:
    - secrets
    providers:
    - aesgcm:
        keys:
        - name: key1
          secret: <32-byte-base64-key>
    - identity: {}

# 生成随机密钥:
# head -c 32 /dev/urandom | base64
```

##<!-- chunk: 2. 使用 External Secrets -->## 2. 使用 External Secrets

```yaml
# 推荐: 不在 Kubernetes 中存储敏感信息
# 使用外部密钥管理系统:
# - AWS Secrets Manager
# - HashiCorp Vault
# - Azure Key Vault
# - Google Secret Manager

# 通过 External Secrets Operator 同步
```

##<!-- chunk: 3. RBAC 最小权限 -->## 3. RBAC 最小权限

```yaml
# 限制 Secret 访问权限
apiVersion: rbac.authorization.k8s.io/v1
kind: Role
metadata:
  name: secret-reader
  namespace: production
rules:
# 仅允许读取特定 Secret
- apiGroups: [""]
  resources: ["secrets"]
  resourceNames: ["app-secrets"]
  verbs: ["get"]

---
# 禁止列出所有 Secret
apiVersion: rbac.authorization.k8s.io/v1
kind: Role
metadata:
  name: no-secret-list
  namespace: production
rules:
- apiGroups: [""]
  resources: ["secrets"]
  verbs: ["get"]  # 不包含 "list"
```

##<!-- chunk: 4. 不可变 Secret -->## 4. 不可变 Secret

```yaml
# 生产环境使用不可变 Secret
apiVersion: v1
kind: Secret
metadata:
  name: prod-secret-v1
  namespace: production
immutable: true
type: Opaque
stringData:
  api_key: "prod-key-12345"

# 优势:
# 1. 防止意外修改
# 2. 降低 kubelet 监听负载
# 3. 版本化管理 (prod-secret-v1, v2, v3...)
```

##<!-- chunk: 5. 避免日志泄露 -->## 5. 避免日志泄露

```yaml
# 反模式: Secret 泄露到日志
apiVersion: v1
kind: Pod
metadata:
  name: bad-practice
spec:
  containers:
  - name: app
    image: myapp:latest
    env:
    - name: API_KEY
      valueFrom:
        secretKeyRef:
          name: api-secret
          key: api_key
    command:
    # 危险! 密钥会输出到日志
    - sh
    - -c
    - echo "API Key: $API_KEY" && ./app

# 正确实践: 应用内部读取, 不输出到 stdout
```

#<!-- chunk: FAQ -->## FAQ

##<!-- chunk: Q1: Secret 与 ConfigMap 如何选择? -->## Q1: Secret 与 ConfigMap 如何选择?

**A:** 选择标准:
- **Secret**: 密码、API密钥、TLS证书等敏感信息
- **ConfigMap**: 配置文件、环境变量、非敏感参数

##<!-- chunk: Q2: Base64 编码是否安全? -->## Q2: Base64 编码是否安全?

**A:** **不安全**, Base64 可轻易解码:
- 真正的安全依赖: RBAC + etcd加密 + TLS传输
- Base64 仅用于处理二进制数据

##<!-- chunk: Q3: 如何轮换 Secret? -->## Q3: 如何轮换 Secret?

**A:** 三种策略:
```yaml
# 策略1: 不可变 Secret + 版本化 (推荐)
# - 创建 secret-v2
# - 更新 Deployment 引用
# - 滚动更新 Pod
# - 删除 secret-v1

# 策略2: 原地更新 (Volume 挂载)
# - 更新 Secret
# - 等待 kubelet 同步 (1-2分钟)
# - 应用检测文件变更并重载

# 策略3: 原地更新 + 强制重启
# - 更新 Secret
# - kubectl rollout restart deployment/myapp
```

##<!-- chunk: Q4: 如何在 CI/CD 中管理 Secret? -->## Q4: 如何在 CI/CD 中管理 Secret?

**A:** 推荐方案:
1. **Sealed Secrets**: 加密后提交 Git
2. **External Secrets**: 从外部同步
3. **GitOps + Vault**: ArgoCD + HashiCorp Vault
4. **SOPS**: 加密 YAML 文件

##<!-- chunk: Q5: Secret 可以跨命名空间引用吗? -->## Q5: Secret 可以跨命名空间引用吗?

**A:** 原生不支持, 解决方案:
```yaml
# 方案1: 复制 Secret 到目标命名空间
# 方案2: 使用 Reflector (https://github.com/emberstack/kubernetes-reflector)
apiVersion: v1
kind: Secret
metadata:
  name: source-secret
  namespace: default
  annotations:
    reflector.v1.k8s.emberstack.com/reflection-allowed: "true"
    reflector.v1.k8s.emberstack.com/reflection-auto-enabled: "true"
    reflector.v1.k8s.emberstack.com/reflection-allowed-namespaces: "team-a,team-b"
```

---

<!-- chunk: 相关资源 -->## 相关资源

#<!-- chunk: 官方文档 -->## 官方文档
- Secret 概念: https://kubernetes.io/docs/concepts/configuration/secret/
- 加密静态数据: https://kubernetes.io/docs/tasks/administer-cluster/encrypt-data/

#<!-- chunk: 工具推荐 -->## 工具推荐
- **External Secrets Operator**: https://external-secrets.io/
- **Sealed Secrets**: https://github.com/bitnami-labs/sealed-secrets
- **cert-manager**: https://cert-manager.io/
- **Reflector**: https://github.com/emberstack/kubernetes-reflector
- **SOPS**: https://github.com/mozilla/sops

#<!-- chunk: 本知识库相关文档 -->## 本知识库相关文档
- [13 - ConfigMap 参考](./13-configmap-reference.md)
- [07 - RBAC 完整配置](./07-rbac-complete.md)
- [存储卷类型参考](./06-volume-types.md)

---

**最后更新**: 2026-02 | **维护者**: Kudig.io 社区 | **反馈**: [GitHub Issues](https://github.com/kudig-io/kudig-database)

---

<!-- chunk: Obsidian 相关文档 -->## Obsidian 相关文档

- domain-32-yaml-manifests MOC
- [[domain-18-manifests-patterns/README.md|Domain-32: Kubernetes YAML 配置完整参考手册]]
- Domain-32 YAML 清单 — 开源项目索引
- 01 - YAML 语法基础与 Kubernetes 资源通用规范
- 02 - Namespace / ResourceQuota / LimitRange YAML 配置参考
- 03 - Pod 完整规格说明书
- 04 - Deployment / ReplicaSet YAML 配置参考
- 05 - StatefulSet YAML 配置参考
- 06 - DaemonSet YAML 配置参考
- 07 - Job / CronJob YAML 配置参考
- 08 - Service 全类型 YAML 配置参考
- 09 - Endpoints / EndpointSlice YAML 配置参考

## See Also

- 12-gateway-api-advanced-routes
- 13-configmap-reference
- 15-persistentvolume-reference
- 16-persistentvolumeclaim-reference

## Related

- [[domain-19-landscape-references/topic-index/security-index|Security 安全知识图谱索引]]
