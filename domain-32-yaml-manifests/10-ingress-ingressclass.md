# 10 - Ingress / IngressClass YAML 配置参考

> **适用版本**: Kubernetes v1.25 - v1.32 | **最后更新**: 2026-02

## 概述

**Ingress** 是 Kubernetes 中用于暴露 HTTP/HTTPS 服务的 API 对象,提供基于域名和路径的流量路由、TLS 终结、负载均衡等七层(L7)网络功能。Ingress 不是一个单独的服务,而是一组规则,由 **Ingress Controller** 实现这些规则。

**IngressClass** (v1.19+ GA) 定义了 Ingress 的实现类别,支持在同一集群中运行多个 Ingress Controller(如 Nginx、Traefik、HAProxy 等)。

### 核心概念

**Ingress 的作用**:
- **统一入口**: 单个 LoadBalancer IP 暴露多个服务
- **路径路由**: 根据 URL 路径转发到不同服务
- **域名路由**: 基于主机名(Host)的虚拟主机
- **TLS 终结**: 在边缘统一处理 HTTPS 证书
- **负载均衡**: 在多个 Pod 之间分发流量
- **高级功能**: URL 重写、速率限制、身份验证等(通过注解)

**Ingress 与 Service 的关系**:
```
外部流量 → LoadBalancer/NodePort → Ingress Controller → Ingress 规则 → Service → Pod
```

**常见 Ingress Controller**:
- **NGINX Ingress Controller** (Kubernetes 官方)
- **Ingress-NGINX** (NGINX Inc. 维护)
- **Traefik**
- **HAProxy Ingress**
- **Contour** (Envoy 代理)
- **Istio Gateway** (服务网格)
- **Kong Ingress**
- **云厂商**: AWS ALB、GCP GCLB、Azure Application Gateway

### 使用场景

1. **多服务暴露**: 单个 IP 暴露多个后端服务
2. **基于域名的路由**: `api.example.com` → API 服务, `www.example.com` → 前端服务
3. **路径路由**: `/api/*` → API 服务, `/static/*` → 静态资源服务
4. **TLS 集中管理**: 统一管理 SSL/TLS 证书
5. **金丝雀发布**: 流量分割到不同版本
6. **API 网关**: 速率限制、认证、日志等

---

## Ingress API 信息

| API Group | API Version | Kind    | 稳定性 |
|-----------|-------------|---------|--------|
| networking.k8s.io | v1    | Ingress | GA (v1.19+) |

**完整 API 路径**:
```
GET /apis/networking.k8s.io/v1/namespaces/{namespace}/ingresses/{name}
```

**缩写**: `ing`

**命名空间作用域**: 是

**历史版本**:
- `extensions/v1beta1` (已废弃,v1.22 移除)
- `networking.k8s.io/v1beta1` (已废弃,v1.22 移除)
- `networking.k8s.io/v1` (v1.19+ GA,当前标准)

---

## Ingress 完整字段规格表

### 顶层字段

| 字段路径 | 类型 | 必需 | 默认值 | 说明 | 版本要求 |
|---------|------|------|--------|------|----------|
| `apiVersion` | string | 是 | - | networking.k8s.io/v1 | v1.19+ |
| `kind` | string | 是 | - | Ingress | v1.19+ |
| `metadata.name` | string | 是 | - | Ingress 名称 | v1.19+ |
| `metadata.namespace` | string | 否 | default | 命名空间 | v1.19+ |
| `metadata.annotations` | map | 否 | - | 注解(Controller 特定配置) | v1.19+ |
| `spec.ingressClassName` | string | 推荐 | - | IngressClass 名称 | v1.19+ |
| `spec.defaultBackend` | object | 否 | - | 默认后端(所有未匹配流量) | v1.19+ |
| `spec.tls[]` | array | 否 | - | TLS 配置列表 | v1.19+ |
| `spec.rules[]` | array | 否 | - | 路由规则列表 | v1.19+ |

### spec.defaultBackend 字段

| 字段路径 | 类型 | 必需 | 默认值 | 说明 |
|---------|------|------|--------|------|
| `service.name` | string | 是 | - | 后端 Service 名称 |
| `service.port.number` | int32 | 条件 | - | Service 端口号 |
| `service.port.name` | string | 条件 | - | Service 端口名称 |
| `resource` | object | 否 | - | 自定义资源后端(高级) |

**注意**: `service.port.number` 和 `service.port.name` 二选一

### spec.tls[] 字段

| 字段路径 | 类型 | 必需 | 默认值 | 说明 |
|---------|------|------|--------|------|
| `hosts[]` | []string | 否 | - | TLS 证书覆盖的主机名列表 |
| `secretName` | string | 是 | - | 存储 TLS 证书的 Secret 名称 |

**Secret 格式要求**:
```yaml
type: kubernetes.io/tls
data:
  tls.crt: <base64 编码的证书>
  tls.key: <base64 编码的私钥>
```

### spec.rules[] 字段

| 字段路径 | 类型 | 必需 | 默认值 | 说明 |
|---------|------|------|--------|------|
| `host` | string | 否 | - | 主机名(支持通配符) |
| `http` | object | 是 | - | HTTP 规则配置 |

**host 格式**:
- 精确匹配: `api.example.com`
- 通配符: `*.example.com` (仅支持最左侧标签)
- 空值: 匹配所有主机名

### spec.rules[].http 字段

| 字段路径 | 类型 | 必需 | 默认值 | 说明 |
|---------|------|------|--------|------|
| `paths[]` | array | 是 | - | 路径匹配规则列表 |

### spec.rules[].http.paths[] 字段

| 字段路径 | 类型 | 必需 | 默认值 | 说明 |
|---------|------|------|--------|------|
| `path` | string | 否 | / | URL 路径 |
| `pathType` | string | 是 | - | 路径匹配类型 |
| `backend` | object | 是 | - | 后端服务配置 |

### pathType 详解

| 值 | 匹配规则 | 示例 | 匹配路径 | 不匹配路径 |
|----|---------|------|----------|------------|
| **Exact** | 精确匹配(区分大小写) | `/api/v1` | `/api/v1` | `/api/v1/`, `/api/v2` |
| **Prefix** | 前缀匹配(按路径元素) | `/api` | `/api`, `/api/`, `/api/v1` | `/api-docs`, `/apiv1` |
| **ImplementationSpecific** | Controller 特定实现 | `/api.*` | 取决于 Controller | 取决于 Controller |

**Prefix 路径匹配细节**:
- `/foo` 匹配 `/foo`, `/foo/`, `/foo/bar`
- `/foo` **不**匹配 `/foobar`, `/foo-bar`
- 按路径元素(以 `/` 分隔)匹配,而非简单字符串前缀

**匹配优先级**:
1. Exact 类型优先于 Prefix
2. 更长的路径优先于短路径
3. 精确主机名优先于通配符主机名

### spec.rules[].http.paths[].backend 字段

| 字段路径 | 类型 | 必需 | 默认值 | 说明 |
|---------|------|------|--------|------|
| `service.name` | string | 是 | - | 后端 Service 名称 |
| `service.port.number` | int32 | 条件 | - | Service 端口号 |
| `service.port.name` | string | 条件 | - | Service 端口名称 |
| `resource` | object | 否 | - | 自定义资源后端 |

---

## IngressClass API 信息

| API Group | API Version | Kind         | 稳定性 |
|-----------|-------------|--------------|--------|
| networking.k8s.io | v1    | IngressClass | GA (v1.19+) |

**完整 API 路径**:
```
GET /apis/networking.k8s.io/v1/ingressclasses/{name}
```

**命名空间作用域**: 否(集群级别资源)

---

## IngressClass 完整字段规格表

### 顶层字段

| 字段路径 | 类型 | 必需 | 默认值 | 说明 | 版本要求 |
|---------|------|------|--------|------|----------|
| `apiVersion` | string | 是 | - | networking.k8s.io/v1 | v1.19+ |
| `kind` | string | 是 | - | IngressClass | v1.19+ |
| `metadata.name` | string | 是 | - | IngressClass 名称 | v1.19+ |
| `metadata.annotations` | map | 否 | - | 注解(可设置默认 IngressClass) | v1.19+ |
| `spec.controller` | string | 是 | - | Controller 标识符 | v1.19+ |
| `spec.parameters` | object | 否 | - | Controller 特定参数 | v1.19+ |

### 默认 IngressClass 配置

**注解**:
```yaml
metadata:
  annotations:
    ingressclass.kubernetes.io/is-default-class: "true"
```

**行为**:
- Ingress 未指定 `ingressClassName` 时使用默认 IngressClass
- 仅允许一个 IngressClass 设置为默认(多个会导致冲突)

### spec.controller 字段

**格式**: 唯一的域名格式字符串,标识 Ingress Controller

**示例**:
- NGINX Ingress: `k8s.io/ingress-nginx`
- Traefik: `traefik.io/ingress-controller`
- HAProxy: `haproxy.org/ingress-controller`
- AWS ALB: `ingress.k8s.aws/alb`
- Istio: `istio.io/ingress-controller`

### spec.parameters 字段

| 字段路径 | 类型 | 必需 | 说明 |
|---------|------|------|------|
| `apiGroup` | string | 否 | 参数资源的 API Group |
| `kind` | string | 是 | 参数资源类型 |
| `name` | string | 是 | 参数资源名称 |
| `namespace` | string | 否 | 参数资源命名空间(命名空间级别资源) |
| `scope` | string | 否 | 作用域(Cluster/Namespace) |

**示例**(AWS ALB Ingress Controller):
```yaml
spec:
  parameters:
    apiGroup: elbv2.k8s.aws
    kind: IngressClassParams
    name: my-alb-params
```

---

## 常用注解参考

注解(Annotations)是 Ingress 扩展功能的主要方式,不同 Ingress Controller 支持不同注解。以下列出常见 Controller 的注解。

### NGINX Ingress Controller (k8s.io/ingress-nginx)

#### 基础配置

| 注解键 | 值类型 | 说明 | 示例 |
|-------|--------|------|------|
| `nginx.ingress.kubernetes.io/rewrite-target` | string | URL 路径重写 | `/$1` |
| `nginx.ingress.kubernetes.io/ssl-redirect` | bool | 强制 HTTPS 重定向 | `"true"` |
| `nginx.ingress.kubernetes.io/force-ssl-redirect` | bool | 强制 HTTPS(即使无 TLS 配置) | `"true"` |
| `nginx.ingress.kubernetes.io/backend-protocol` | string | 后端协议(HTTP/HTTPS/GRPC/GRPCS) | `"HTTPS"` |
| `nginx.ingress.kubernetes.io/proxy-body-size` | string | 请求体大小限制 | `"100m"` |
| `nginx.ingress.kubernetes.io/proxy-connect-timeout` | string | 连接超时(秒) | `"60"` |
| `nginx.ingress.kubernetes.io/proxy-send-timeout` | string | 发送超时(秒) | `"60"` |
| `nginx.ingress.kubernetes.io/proxy-read-timeout` | string | 读取超时(秒) | `"60"` |

#### 高级功能

| 注解键 | 值类型 | 说明 | 示例 |
|-------|--------|------|------|
| `nginx.ingress.kubernetes.io/rate-limit` | string | 速率限制(rps) | `"10"` |
| `nginx.ingress.kubernetes.io/whitelist-source-range` | string | IP 白名单(CIDR) | `"10.0.0.0/8,172.16.0.0/12"` |
| `nginx.ingress.kubernetes.io/auth-type` | string | 认证类型(basic/digest) | `"basic"` |
| `nginx.ingress.kubernetes.io/auth-secret` | string | 认证 Secret 名称 | `"basic-auth"` |
| `nginx.ingress.kubernetes.io/auth-realm` | string | 认证域 | `"Authentication Required"` |
| `nginx.ingress.kubernetes.io/enable-cors` | bool | 启用 CORS | `"true"` |
| `nginx.ingress.kubernetes.io/cors-allow-origin` | string | CORS 允许源 | `"*"` |
| `nginx.ingress.kubernetes.io/cors-allow-methods` | string | CORS 允许方法 | `"GET, POST, PUT"` |
| `nginx.ingress.kubernetes.io/cors-allow-headers` | string | CORS 允许头 | `"DNT,X-CustomHeader"` |

#### 金丝雀发布(Canary)

| 注解键 | 值类型 | 说明 | 示例 |
|-------|--------|------|------|
| `nginx.ingress.kubernetes.io/canary` | bool | 启用金丝雀 | `"true"` |
| `nginx.ingress.kubernetes.io/canary-weight` | string | 流量权重(0-100) | `"10"` |
| `nginx.ingress.kubernetes.io/canary-by-header` | string | 基于请求头 | `"canary"` |
| `nginx.ingress.kubernetes.io/canary-by-header-value` | string | 请求头值匹配 | `"always"` |
| `nginx.ingress.kubernetes.io/canary-by-cookie` | string | 基于 Cookie | `"canary"` |

#### SSL/TLS 配置

| 注解键 | 值类型 | 说明 | 示例 |
|-------|--------|------|------|
| `nginx.ingress.kubernetes.io/ssl-protocols` | string | SSL 协议版本 | `"TLSv1.2 TLSv1.3"` |
| `nginx.ingress.kubernetes.io/ssl-ciphers` | string | SSL 加密套件 | `"HIGH:!aNULL:!MD5"` |
| `nginx.ingress.kubernetes.io/ssl-passthrough` | bool | SSL 透传(后端处理 TLS) | `"true"` |
| `nginx.ingress.kubernetes.io/ssl-prefer-server-ciphers` | bool | 优先服务器加密套件 | `"true"` |

---

### Traefik Ingress Controller

| 注解键 | 值类型 | 说明 | 示例 |
|-------|--------|------|------|
| `traefik.ingress.kubernetes.io/router.entrypoints` | string | 入口点 | `"web,websecure"` |
| `traefik.ingress.kubernetes.io/router.middlewares` | string | 中间件列表 | `"default-stripprefix@kubernetescrd"` |
| `traefik.ingress.kubernetes.io/router.tls` | bool | 启用 TLS | `"true"` |
| `traefik.ingress.kubernetes.io/router.tls.certresolver` | string | 证书解析器(如 Let's Encrypt) | `"letsencrypt"` |
| `traefik.ingress.kubernetes.io/rate-limit` | string | 速率限制配置 | `"average: 100, burst: 200"` |

---

### HAProxy Ingress Controller

| 注解键 | 值类型 | 说明 | 示例 |
|-------|--------|------|------|
| `haproxy.org/path-rewrite` | string | 路径重写 | `"/api/(.*) /\\1"` |
| `haproxy.org/rate-limit` | string | 速率限制 | `"100"` |
| `haproxy.org/whitelist` | string | IP 白名单 | `"192.168.1.0/24"` |
| `haproxy.org/timeout-client` | string | 客户端超时 | `"50s"` |
| `haproxy.org/timeout-server` | string | 服务端超时 | `"50s"` |
| `haproxy.org/ssl-redirect` | bool | HTTPS 重定向 | `"true"` |

---

### AWS ALB Ingress Controller

| 注解键 | 值类型 | 说明 | 示例 |
|-------|--------|------|------|
| `alb.ingress.kubernetes.io/scheme` | string | LB 类型(internet-facing/internal) | `"internet-facing"` |
| `alb.ingress.kubernetes.io/target-type` | string | 目标类型(ip/instance) | `"ip"` |
| `alb.ingress.kubernetes.io/certificate-arn` | string | ACM 证书 ARN | `"arn:aws:acm:..."` |
| `alb.ingress.kubernetes.io/listen-ports` | string | 监听端口 | `'[{"HTTP": 80}, {"HTTPS": 443}]'` |
| `alb.ingress.kubernetes.io/ssl-policy` | string | SSL 策略 | `"ELBSecurityPolicy-TLS-1-2-2017-01"` |
| `alb.ingress.kubernetes.io/healthcheck-path` | string | 健康检查路径 | `"/health"` |
| `alb.ingress.kubernetes.io/healthcheck-interval-seconds` | string | 健康检查间隔 | `"15"` |
| `alb.ingress.kubernetes.io/success-codes` | string | 健康检查成功码 | `"200-299"` |
| `alb.ingress.kubernetes.io/load-balancer-attributes` | string | LB 属性 | `"idle_timeout.timeout_seconds=60"` |

---

### GCP GCE Ingress Controller

| 注解键 | 值类型 | 说明 | 示例 |
|-------|--------|------|------|
| `kubernetes.io/ingress.class` | string | Ingress 类别(旧版) | `"gce"` |
| `kubernetes.io/ingress.global-static-ip-name` | string | 静态 IP 名称 | `"my-static-ip"` |
| `ingress.gcp.kubernetes.io/pre-shared-cert` | string | 预共享证书 | `"my-cert"` |
| `cloud.google.com/neg` | string | NEG 配置 | `'{"ingress": true}'` |
| `cloud.google.com/backend-config` | string | 后端配置 | `"my-backend-config"` |

---

## TLS 配置

### 基础 TLS 配置

```yaml
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: tls-ingress
  namespace: production
spec:
  ingressClassName: nginx
  tls:
  - hosts:
    - example.com
    - www.example.com
    secretName: example-tls  # TLS Secret 名称
  rules:
  - host: example.com
    http:
      paths:
      - path: /
        pathType: Prefix
        backend:
          service:
            name: web
            port:
              number: 80
```

### 创建 TLS Secret

**方法 1: 从证书文件创建**
```bash
kubectl create secret tls example-tls \
  --cert=path/to/tls.crt \
  --key=path/to/tls.key \
  -n production
```

**方法 2: YAML 定义**
```yaml
apiVersion: v1
kind: Secret
metadata:
  name: example-tls
  namespace: production
type: kubernetes.io/tls
data:
  tls.crt: <base64 编码的证书内容>
  tls.key: <base64 编码的私钥内容>
```

**生成 base64 编码**:
```bash
cat tls.crt | base64 -w 0
cat tls.key | base64 -w 0
```

### 多域名 TLS 配置

```yaml
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: multi-tls-ingress
spec:
  ingressClassName: nginx
  tls:
  - hosts:
    - api.example.com
    secretName: api-tls
  - hosts:
    - www.example.com
    - example.com
    secretName: web-tls
  rules:
  - host: api.example.com
    http:
      paths:
      - path: /
        pathType: Prefix
        backend:
          service:
            name: api
            port:
              number: 8080
  - host: www.example.com
    http:
      paths:
      - path: /
        pathType: Prefix
        backend:
          service:
            name: web
            port:
              number: 80
```

### 自动证书管理(cert-manager)

**安装 cert-manager**:
```bash
kubectl apply -f https://github.com/cert-manager/cert-manager/releases/download/v1.13.0/cert-manager.yaml
```

**配置 Let's Encrypt Issuer**:
```yaml
apiVersion: cert-manager.io/v1
kind: ClusterIssuer
metadata:
  name: letsencrypt-prod
spec:
  acme:
    server: https://acme-v02.api.letsencrypt.org/directory
    email: admin@example.com
    privateKeySecretRef:
      name: letsencrypt-prod
    solvers:
    - http01:
        ingress:
          class: nginx
```

**Ingress 配置(自动获取证书)**:
```yaml
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: auto-tls-ingress
  annotations:
    cert-manager.io/cluster-issuer: "letsencrypt-prod"  # 关键注解
spec:
  ingressClassName: nginx
  tls:
  - hosts:
    - example.com
    secretName: example-tls  # cert-manager 自动创建此 Secret
  rules:
  - host: example.com
    http:
      paths:
      - path: /
        pathType: Prefix
        backend:
          service:
            name: web
            port:
              number: 80
```

**验证证书状态**:
```bash
# 查看 Certificate 资源
kubectl get certificate -n namespace

# 查看证书详情
kubectl describe certificate example-tls -n namespace

# 查看 Secret 是否创建
kubectl get secret example-tls -n namespace
```

---

## 最小配置示例

### 最简 Ingress(单服务)

```yaml
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: minimal-ingress
spec:
  ingressClassName: nginx
  rules:
  - http:
      paths:
      - path: /
        pathType: Prefix
        backend:
          service:
            name: web
            port:
              number: 80
```

### 最简 IngressClass

```yaml
apiVersion: networking.k8s.io/v1
kind: IngressClass
metadata:
  name: nginx
spec:
  controller: k8s.io/ingress-nginx
```

---

## 生产级配置示例

### 示例 1: 多域名多路径路由

```yaml
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: multi-domain-ingress
  namespace: production
  labels:
    app: web
    env: prod
  annotations:
    # NGINX 配置
    nginx.ingress.kubernetes.io/ssl-redirect: "true"
    nginx.ingress.kubernetes.io/force-ssl-redirect: "true"
    nginx.ingress.kubernetes.io/proxy-body-size: "50m"
    nginx.ingress.kubernetes.io/proxy-connect-timeout: "60"
    nginx.ingress.kubernetes.io/proxy-send-timeout: "120"
    nginx.ingress.kubernetes.io/proxy-read-timeout: "120"
    
    # 速率限制
    nginx.ingress.kubernetes.io/limit-rps: "100"
    
    # 自动证书管理
    cert-manager.io/cluster-issuer: "letsencrypt-prod"
spec:
  ingressClassName: nginx
  tls:
  - hosts:
    - www.example.com
    - api.example.com
    secretName: example-tls
  rules:
  # 前端服务(主域名)
  - host: www.example.com
    http:
      paths:
      - path: /
        pathType: Prefix
        backend:
          service:
            name: frontend
            port:
              number: 80
  
  # API 服务(API 子域名)
  - host: api.example.com
    http:
      paths:
      # API v1
      - path: /v1
        pathType: Prefix
        backend:
          service:
            name: api-v1
            port:
              number: 8080
      # API v2
      - path: /v2
        pathType: Prefix
        backend:
          service:
            name: api-v2
            port:
              number: 8080
      # 健康检查端点
      - path: /health
        pathType: Exact
        backend:
          service:
            name: api-v2
            port:
              name: health
```

### 示例 2: 路径重写与 URL 重定向

```yaml
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: rewrite-ingress
  namespace: app
  annotations:
    # 路径重写: /api/users → /users
    nginx.ingress.kubernetes.io/rewrite-target: /$2
    
    # URL 重定向: /old-path → /new-path
    nginx.ingress.kubernetes.io/permanent-redirect: "https://example.com/new-path"
    
    # 使用正则捕获组
    nginx.ingress.kubernetes.io/use-regex: "true"
spec:
  ingressClassName: nginx
  rules:
  - host: api.example.com
    http:
      paths:
      # 重写规则: /api/* → /*
      - path: /api(/|$)(.*)
        pathType: ImplementationSpecific
        backend:
          service:
            name: backend-api
            port:
              number: 8080
```

**路径重写详解**:
- 请求: `https://api.example.com/api/users/123`
- 匹配: `/api(/|$)(.*)`
  - 捕获组 1: `/`
  - 捕获组 2: `users/123`
- 重写目标: `/$2` → `/users/123`
- 后端收到: `/users/123`

### 示例 3: 金丝雀发布(Canary Deployment)

```yaml
# 稳定版本 Ingress
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: app-stable
  namespace: production
spec:
  ingressClassName: nginx
  rules:
  - host: app.example.com
    http:
      paths:
      - path: /
        pathType: Prefix
        backend:
          service:
            name: app-v1
            port:
              number: 80

---
# 金丝雀版本 Ingress
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: app-canary
  namespace: production
  annotations:
    nginx.ingress.kubernetes.io/canary: "true"
    nginx.ingress.kubernetes.io/canary-weight: "10"  # 10% 流量到金丝雀版本
spec:
  ingressClassName: nginx
  rules:
  - host: app.example.com
    http:
      paths:
      - path: /
        pathType: Prefix
        backend:
          service:
            name: app-v2
            port:
              number: 80
```

**金丝雀策略**:

**1. 基于权重(Weight)**:
```yaml
annotations:
  nginx.ingress.kubernetes.io/canary: "true"
  nginx.ingress.kubernetes.io/canary-weight: "20"  # 20% 流量
```

**2. 基于请求头(Header)**:
```yaml
annotations:
  nginx.ingress.kubernetes.io/canary: "true"
  nginx.ingress.kubernetes.io/canary-by-header: "X-Canary"
  nginx.ingress.kubernetes.io/canary-by-header-value: "always"
# 请求头 X-Canary: always 时路由到金丝雀版本
```

**3. 基于 Cookie**:
```yaml
annotations:
  nginx.ingress.kubernetes.io/canary: "true"
  nginx.ingress.kubernetes.io/canary-by-cookie: "canary"
# Cookie canary=always 时路由到金丝雀版本
```

**4. 组合策略(优先级: Header > Cookie > Weight)**:
```yaml
annotations:
  nginx.ingress.kubernetes.io/canary: "true"
  nginx.ingress.kubernetes.io/canary-by-header: "X-Canary"
  nginx.ingress.kubernetes.io/canary-by-header-value: "beta"
  nginx.ingress.kubernetes.io/canary-weight: "10"
# 1. 如果有 X-Canary: beta → 金丝雀
# 2. 否则 10% 概率 → 金丝雀
# 3. 其余 → 稳定版本
```

### 示例 4: 认证与授权

#### Basic Auth

```yaml
# 创建 htpasswd 文件
# htpasswd -c auth username
# 创建 Secret
# kubectl create secret generic basic-auth --from-file=auth -n namespace

apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: auth-ingress
  namespace: admin
  annotations:
    nginx.ingress.kubernetes.io/auth-type: "basic"
    nginx.ingress.kubernetes.io/auth-secret: "basic-auth"
    nginx.ingress.kubernetes.io/auth-realm: "Authentication Required - Admin Area"
spec:
  ingressClassName: nginx
  rules:
  - host: admin.example.com
    http:
      paths:
      - path: /
        pathType: Prefix
        backend:
          service:
            name: admin-dashboard
            port:
              number: 80
```

**创建 Basic Auth Secret**:
```bash
# 生成密码文件
htpasswd -c auth admin
# 输入密码: ***

# 创建 Secret
kubectl create secret generic basic-auth \
  --from-file=auth \
  -n admin

# 验证
kubectl get secret basic-auth -n admin -o yaml
```

#### OAuth2 认证(外部认证服务)

```yaml
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: oauth-ingress
  namespace: app
  annotations:
    nginx.ingress.kubernetes.io/auth-url: "https://oauth2-proxy.example.com/oauth2/auth"
    nginx.ingress.kubernetes.io/auth-signin: "https://oauth2-proxy.example.com/oauth2/start?rd=$escaped_request_uri"
    nginx.ingress.kubernetes.io/auth-response-headers: "X-Auth-Request-User,X-Auth-Request-Email"
spec:
  ingressClassName: nginx
  rules:
  - host: app.example.com
    http:
      paths:
      - path: /
        pathType: Prefix
        backend:
          service:
            name: protected-app
            port:
              number: 80
```

### 示例 5: IP 白名单与速率限制

```yaml
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: secured-ingress
  namespace: api
  annotations:
    # IP 白名单(CIDR)
    nginx.ingress.kubernetes.io/whitelist-source-range: "10.0.0.0/8,172.16.0.0/12,192.168.1.0/24"
    
    # 速率限制
    nginx.ingress.kubernetes.io/limit-rps: "50"          # 每秒请求数
    nginx.ingress.kubernetes.io/limit-rpm: "1000"        # 每分钟请求数
    nginx.ingress.kubernetes.io/limit-connections: "10"  # 并发连接数
    
    # 速率限制超出后的响应
    nginx.ingress.kubernetes.io/limit-rate-after: "100"  # 前 100KB 不限速
    nginx.ingress.kubernetes.io/limit-rate: "1024"       # 之后限速 1MB/s
spec:
  ingressClassName: nginx
  rules:
  - host: api.example.com
    http:
      paths:
      - path: /
        pathType: Prefix
        backend:
          service:
            name: api
            port:
              number: 8080
```

### 示例 6: CORS 配置

```yaml
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: cors-ingress
  namespace: api
  annotations:
    # 启用 CORS
    nginx.ingress.kubernetes.io/enable-cors: "true"
    
    # CORS 配置
    nginx.ingress.kubernetes.io/cors-allow-origin: "https://example.com,https://www.example.com"
    nginx.ingress.kubernetes.io/cors-allow-methods: "GET, POST, PUT, DELETE, OPTIONS"
    nginx.ingress.kubernetes.io/cors-allow-headers: "DNT,User-Agent,X-Requested-With,If-Modified-Since,Cache-Control,Content-Type,Range,Authorization"
    nginx.ingress.kubernetes.io/cors-expose-headers: "Content-Length,Content-Range"
    nginx.ingress.kubernetes.io/cors-allow-credentials: "true"
    nginx.ingress.kubernetes.io/cors-max-age: "86400"  # 预检请求缓存时间(秒)
spec:
  ingressClassName: nginx
  rules:
  - host: api.example.com
    http:
      paths:
      - path: /
        pathType: Prefix
        backend:
          service:
            name: api
            port:
              number: 8080
```

### 示例 7: gRPC 服务

```yaml
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: grpc-ingress
  namespace: grpc
  annotations:
    nginx.ingress.kubernetes.io/backend-protocol: "GRPC"     # 后端协议为 gRPC
    nginx.ingress.kubernetes.io/ssl-redirect: "true"
    nginx.ingress.kubernetes.io/grpc-backend: "true"
spec:
  ingressClassName: nginx
  tls:
  - hosts:
    - grpc.example.com
    secretName: grpc-tls
  rules:
  - host: grpc.example.com
    http:
      paths:
      - path: /
        pathType: Prefix
        backend:
          service:
            name: grpc-service
            port:
              number: 50051
```

**gRPC 客户端调用**:
```go
conn, err := grpc.Dial("grpc.example.com:443",
    grpc.WithTransportCredentials(credentials.NewTLS(&tls.Config{})))
```

### 示例 8: WebSocket 支持

```yaml
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: websocket-ingress
  namespace: realtime
  annotations:
    # WebSocket 配置
    nginx.ingress.kubernetes.io/proxy-read-timeout: "3600"    # 1 小时超时
    nginx.ingress.kubernetes.io/proxy-send-timeout: "3600"
    nginx.ingress.kubernetes.io/proxy-connect-timeout: "60"
    
    # WebSocket 特定头部
    nginx.ingress.kubernetes.io/configuration-snippet: |
      proxy_set_header Upgrade $http_upgrade;
      proxy_set_header Connection "upgrade";
spec:
  ingressClassName: nginx
  rules:
  - host: ws.example.com
    http:
      paths:
      - path: /
        pathType: Prefix
        backend:
          service:
            name: websocket-service
            port:
              number: 8080
```

### 示例 9: 默认后端(404 页面)

```yaml
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: default-backend-ingress
  namespace: frontend
spec:
  ingressClassName: nginx
  defaultBackend:  # 未匹配任何规则时的后端
    service:
      name: custom-404
      port:
        number: 80
  rules:
  - host: example.com
    http:
      paths:
      - path: /app
        pathType: Prefix
        backend:
          service:
            name: app-service
            port:
              number: 80
# 访问 example.com/unknown → 路由到 custom-404 服务
```

### 示例 10: 多 IngressClass 配置

```yaml
# IngressClass: NGINX
apiVersion: networking.k8s.io/v1
kind: IngressClass
metadata:
  name: nginx
  annotations:
    ingressclass.kubernetes.io/is-default-class: "true"  # 默认 IngressClass
spec:
  controller: k8s.io/ingress-nginx

---
# IngressClass: Traefik
apiVersion: networking.k8s.io/v1
kind: IngressClass
metadata:
  name: traefik
spec:
  controller: traefik.io/ingress-controller

---
# IngressClass: AWS ALB
apiVersion: networking.k8s.io/v1
kind: IngressClass
metadata:
  name: alb
spec:
  controller: ingress.k8s.aws/alb
  parameters:
    apiGroup: elbv2.k8s.aws
    kind: IngressClassParams
    name: alb-params

---
# Ingress 使用 NGINX
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: nginx-ingress
spec:
  ingressClassName: nginx  # 明确指定
  rules:
  - host: nginx.example.com
    http:
      paths:
      - path: /
        pathType: Prefix
        backend:
          service:
            name: app
            port:
              number: 80

---
# Ingress 使用 Traefik
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: traefik-ingress
spec:
  ingressClassName: traefik  # 明确指定
  rules:
  - host: traefik.example.com
    http:
      paths:
      - path: /
        pathType: Prefix
        backend:
          service:
            name: app
            port:
              number: 80
```

---

## 内部原理

### Ingress Controller 工作流程

```
1. 监听 API Server
   ↓
   Ingress、Service、Endpoints 变化
   ↓
2. 解析 Ingress 规则
   ↓
   提取 host、path、backend、annotations
   ↓
3. 生成代理配置
   ↓
   NGINX: nginx.conf
   Traefik: 动态配置
   HAProxy: haproxy.cfg
   ↓
4. 重载/热更新配置
   ↓
   nginx -s reload
   Traefik 动态更新(无重载)
   ↓
5. 处理流量
   ↓
   外部请求 → Ingress Controller → Service → Pod
```

### NGINX Ingress 配置生成示例

**Ingress YAML**:
```yaml
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: example
spec:
  rules:
  - host: example.com
    http:
      paths:
      - path: /api
        pathType: Prefix
        backend:
          service:
            name: api
            port:
              number: 8080
```

**生成的 nginx.conf 片段**:
```nginx
server {
    listen 80;
    server_name example.com;
    
    location /api {
        proxy_pass http://upstream-api-8080;
        proxy_set_header Host $host;
        proxy_set_header X-Real-IP $remote_addr;
        proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;
        proxy_set_header X-Forwarded-Proto $scheme;
    }
}

upstream upstream-api-8080 {
    server 10.244.1.5:8080;  # Pod IP 1
    server 10.244.2.8:8080;  # Pod IP 2
}
```

### TLS 终结流程

```
1. 客户端发起 HTTPS 请求
   ↓
   https://example.com/api
   ↓
2. Ingress Controller 接收
   ↓
   监听 443 端口
   ↓
3. TLS 握手
   ↓
   使用 Secret 中的证书和私钥
   ↓
4. 解密请求
   ↓
   提取 HTTP 请求数据
   ↓
5. 路由到后端
   ↓
   HTTP 请求(明文) → Service → Pod
   ↓
6. 返回响应
   ↓
   Pod → Service → Ingress Controller
   ↓
7. 加密响应
   ↓
   TLS 加密 → 客户端
```

### IngressClass 选择机制

**优先级**:
1. **明确指定**: `spec.ingressClassName: nginx`
2. **默认 IngressClass**: 带注解 `ingressclass.kubernetes.io/is-default-class: "true"`
3. **旧版注解**(已废弃): `metadata.annotations: kubernetes.io/ingress.class: nginx`
4. **无 IngressClass**: Ingress Controller 根据自身配置决定是否处理

---

## 版本兼容性

| 功能特性 | 引入版本 | 稳定版本 | 说明 |
|---------|---------|---------|------|
| Ingress (extensions/v1beta1) | v1.1 | v1.19 (废弃) | 早期 API,已移除 |
| Ingress (networking.k8s.io/v1beta1) | v1.14 | v1.22 (移除) | 过渡版本 |
| **Ingress (networking.k8s.io/v1)** | v1.19 (GA) | **当前标准** | 推荐使用 |
| IngressClass | v1.18 (Beta) | v1.19 (GA) | 多 Controller 支持 |
| pathType 字段 | v1.18 | v1.19 | Exact/Prefix/ImplementationSpecific |
| defaultBackend.service.port.name | v1.19 | v1.19 | 支持端口名称引用 |
| IngressClass 默认类别 | v1.19 | v1.19 | is-default-class 注解 |

**v1 API 与 v1beta1 主要区别**:
1. **pathType 必需**: v1 要求明确指定路径类型
2. **backend 结构变化**: `serviceName`/`servicePort` → `service.name`/`service.port`
3. **ingressClassName**: 替代 `kubernetes.io/ingress.class` 注解(推荐)

**迁移示例**(v1beta1 → v1):
```yaml
# v1beta1 (已废弃)
apiVersion: networking.k8s.io/v1beta1
kind: Ingress
metadata:
  annotations:
    kubernetes.io/ingress.class: nginx
spec:
  rules:
  - http:
      paths:
      - path: /
        backend:
          serviceName: web
          servicePort: 80

# v1 (当前标准)
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: example
spec:
  ingressClassName: nginx
  rules:
  - http:
      paths:
      - path: /
        pathType: Prefix
        backend:
          service:
            name: web
            port:
              number: 80
```

---

## 最佳实践

### 1. 使用 IngressClass 而非注解

**推荐**:
```yaml
spec:
  ingressClassName: nginx
```

**不推荐**(已废弃):
```yaml
metadata:
  annotations:
    kubernetes.io/ingress.class: nginx
```

### 2. 明确指定 pathType

**推荐**:
```yaml
paths:
- path: /api
  pathType: Prefix  # 明确指定
  backend: ...
```

**不推荐**:
```yaml
# 遗漏 pathType 会导致验证失败
```

### 3. TLS 证书管理

**使用 cert-manager 自动化**:
- 自动获取和续期证书
- 避免证书过期导致的中断
- 支持 Let's Encrypt、自签名等

**配置示例**:
```yaml
annotations:
  cert-manager.io/cluster-issuer: "letsencrypt-prod"
```

### 4. 路径匹配优先级规划

**设计规则**:
- 更具体的路径放在前面
- Exact 优先于 Prefix
- 长路径优先于短路径

**示例**:
```yaml
paths:
- path: /api/v2/users
  pathType: Exact
  backend: ...
- path: /api/v2
  pathType: Prefix
  backend: ...
- path: /api
  pathType: Prefix
  backend: ...
```

### 5. 监控与日志

**启用访问日志**:
```yaml
annotations:
  nginx.ingress.kubernetes.io/enable-access-log: "true"
```

**集成 Prometheus 监控**:
```yaml
# NGINX Ingress Controller 默认暴露指标
# 访问: http://<controller-pod>:10254/metrics

# 关键指标:
# - nginx_ingress_controller_requests (请求总数)
# - nginx_ingress_controller_request_duration_seconds (延迟)
# - nginx_ingress_controller_response_size (响应大小)
```

### 6. 安全加固

**强制 HTTPS**:
```yaml
annotations:
  nginx.ingress.kubernetes.io/ssl-redirect: "true"
```

**IP 白名单**:
```yaml
annotations:
  nginx.ingress.kubernetes.io/whitelist-source-range: "10.0.0.0/8"
```

**速率限制**:
```yaml
annotations:
  nginx.ingress.kubernetes.io/limit-rps: "100"
```

### 7. 高可用部署

**Ingress Controller 多副本**:
```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: ingress-nginx-controller
spec:
  replicas: 3  # 至少 3 个副本
  selector:
    matchLabels:
      app: ingress-nginx
  template:
    spec:
      affinity:
        podAntiAffinity:  # 避免多个副本在同一节点
          requiredDuringSchedulingIgnoredDuringExecution:
          - labelSelector:
              matchExpressions:
              - key: app
                operator: In
                values:
                - ingress-nginx
            topologyKey: "kubernetes.io/hostname"
```

### 8. 资源限制

```yaml
spec:
  containers:
  - name: controller
    resources:
      limits:
        cpu: "1000m"
        memory: "512Mi"
      requests:
        cpu: "100m"
        memory: "128Mi"
```

### 9. 配置验证

**部署前验证**:
```bash
# 验证 YAML 语法
kubectl apply --dry-run=client -f ingress.yaml

# 验证配置是否生效
kubectl get ingress -n namespace
kubectl describe ingress ingress-name -n namespace

# 检查 Ingress Controller 日志
kubectl logs -n ingress-nginx deployment/ingress-nginx-controller
```

### 10. 渐进式部署

**金丝雀发布流程**:
1. 部署新版本服务(v2)
2. 创建金丝雀 Ingress(10% 流量)
3. 监控错误率和延迟
4. 逐步增加权重(10% → 50% → 100%)
5. 完全切换后删除旧版本

---

## FAQ

### Q1: Ingress 创建后无法访问?

**排查步骤**:
```bash
# 1. 检查 Ingress 是否创建
kubectl get ingress -n namespace

# 2. 查看 Ingress 状态
kubectl describe ingress ingress-name -n namespace

# 3. 检查 Ingress Controller 是否运行
kubectl get pods -n ingress-nginx

# 4. 查看 Controller 日志
kubectl logs -n ingress-nginx deployment/ingress-nginx-controller

# 5. 检查 Service 是否存在
kubectl get svc backend-service -n namespace

# 6. 检查 Endpoints 是否有 IP
kubectl get endpoints backend-service -n namespace

# 7. 测试 Service 内部连通性
kubectl run test --image=curlimages/curl --rm -it -- curl http://backend-service.namespace.svc.cluster.local

# 8. 检查 DNS 解析(如果使用域名)
nslookup example.com

# 9. 检查 Ingress Controller Service
kubectl get svc -n ingress-nginx ingress-nginx-controller
```

**常见问题**:
- Ingress Controller 未部署
- IngressClass 不匹配
- 后端 Service 不存在或无 Endpoints
- TLS 证书问题
- 防火墙/安全组规则

### Q2: 多个 Ingress 匹配同一域名?

**行为**: 合并规则(同一 IngressClass)

```yaml
# Ingress 1
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: ingress-1
spec:
  ingressClassName: nginx
  rules:
  - host: example.com
    http:
      paths:
      - path: /api
        pathType: Prefix
        backend:
          service:
            name: api
            port:
              number: 80

---
# Ingress 2
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: ingress-2
spec:
  ingressClassName: nginx
  rules:
  - host: example.com
    http:
      paths:
      - path: /web
        pathType: Prefix
        backend:
          service:
            name: web
            port:
              number: 80
```

**结果**: 两个路径规则都生效
- `/api` → api 服务
- `/web` → web 服务

### Q3: 如何实现同一域名的多个 TLS 证书?

**场景**: 不同子路径使用不同证书

**答案**: **不支持**。TLS 握手发生在 HTTP 层之前,此时无法知道请求路径,因此一个域名只能有一个证书。

**解决方案**:
1. 使用通配符证书(`*.example.com`)
2. 使用 SAN(Subject Alternative Name)多域名证书
3. 为不同路径使用不同子域名

### Q4: Ingress 如何实现灰度发布?

**方案 1: 金丝雀 Ingress(NGINX)**
```yaml
# 见前文示例 3
```

**方案 2: 服务网格(Istio)**
```yaml
apiVersion: networking.istio.io/v1beta1
kind: VirtualService
metadata:
  name: app
spec:
  hosts:
  - app.example.com
  http:
  - match:
    - headers:
        user-type:
          exact: beta
    route:
    - destination:
        host: app-v2
  - route:
    - destination:
        host: app-v1
      weight: 90
    - destination:
        host: app-v2
      weight: 10
```

### Q5: 如何查看 Ingress 分配的 IP 地址?

```bash
# 查看 Ingress 状态
kubectl get ingress -n namespace

# 输出示例:
# NAME      CLASS   HOSTS            ADDRESS         PORTS     AGE
# example   nginx   example.com      203.0.113.50    80, 443   5m

# 详细信息
kubectl describe ingress example -n namespace | grep Address
```

**获取 Ingress Controller Service 的 IP**:
```bash
kubectl get svc -n ingress-nginx ingress-nginx-controller

# LoadBalancer 类型会显示 EXTERNAL-IP
# NAME                       TYPE           CLUSTER-IP     EXTERNAL-IP    PORT(S)
# ingress-nginx-controller   LoadBalancer   10.96.100.50   203.0.113.50   80:30080/TCP,443:30443/TCP
```

### Q6: Ingress 支持 TCP/UDP 服务吗?

**答案**: 标准 Ingress 仅支持 HTTP/HTTPS(七层)。

**TCP/UDP 暴露方案**:
1. **使用 Service type=LoadBalancer**
2. **NGINX Ingress Controller 的 TCP/UDP ConfigMap**

**NGINX TCP 暴露示例**:
```yaml
# ConfigMap
apiVersion: v1
kind: ConfigMap
metadata:
  name: tcp-services
  namespace: ingress-nginx
data:
  3306: "database/mysql:3306"  # 外部端口:服务命名空间/服务名:端口
  6379: "cache/redis:6379"

---
# Ingress Controller Deployment 添加参数
args:
- --tcp-services-configmap=$(POD_NAMESPACE)/tcp-services
```

### Q7: 如何强制删除卡住的 Ingress?

```bash
# 正常删除
kubectl delete ingress ingress-name -n namespace

# 强制删除(移除 finalizers)
kubectl patch ingress ingress-name -n namespace \
  -p '{"metadata":{"finalizers":[]}}' --type=merge

# 或直接编辑
kubectl edit ingress ingress-name -n namespace
# 删除 metadata.finalizers 字段
```

### Q8: Ingress 与 Gateway API 的区别?

| 特性 | Ingress | Gateway API |
|------|---------|-------------|
| **成熟度** | GA(稳定) | Beta(v1.28+) |
| **表达能力** | 有限(HTTP/HTTPS) | 丰富(HTTP/TCP/UDP/gRPC) |
| **角色分离** | 无 | 明确(基础设施/开发者) |
| **跨命名空间** | 不支持 | 支持 |
| **可扩展性** | 注解(非标准) | 标准 API |
| **未来方向** | 维护模式 | 新一代标准 |

**Gateway API 示例**:
```yaml
apiVersion: gateway.networking.k8s.io/v1beta1
kind: Gateway
metadata:
  name: my-gateway
spec:
  gatewayClassName: nginx
  listeners:
  - name: http
    protocol: HTTP
    port: 80

---
apiVersion: gateway.networking.k8s.io/v1beta1
kind: HTTPRoute
metadata:
  name: my-route
spec:
  parentRefs:
  - name: my-gateway
  hostnames:
  - example.com
  rules:
  - backendRefs:
    - name: web
      port: 80
```

**建议**:
- 现有项目: 继续使用 Ingress(稳定可靠)
- 新项目: 考虑 Gateway API(未来趋势)

---

## 生产案例

### 案例 1: 微服务 API 网关

**场景**: 统一入口暴露多个微服务 API

```yaml
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: api-gateway
  namespace: production
  annotations:
    # 自动证书
    cert-manager.io/cluster-issuer: "letsencrypt-prod"
    
    # 速率限制
    nginx.ingress.kubernetes.io/limit-rps: "200"
    nginx.ingress.kubernetes.io/limit-connections: "50"
    
    # CORS
    nginx.ingress.kubernetes.io/enable-cors: "true"
    nginx.ingress.kubernetes.io/cors-allow-origin: "https://app.example.com"
    nginx.ingress.kubernetes.io/cors-allow-methods: "GET, POST, PUT, DELETE, OPTIONS"
    nginx.ingress.kubernetes.io/cors-allow-credentials: "true"
    
    # 超时配置
    nginx.ingress.kubernetes.io/proxy-connect-timeout: "60"
    nginx.ingress.kubernetes.io/proxy-send-timeout: "120"
    nginx.ingress.kubernetes.io/proxy-read-timeout: "120"
    
    # 请求体大小
    nginx.ingress.kubernetes.io/proxy-body-size: "10m"
spec:
  ingressClassName: nginx
  tls:
  - hosts:
    - api.example.com
    secretName: api-tls
  rules:
  - host: api.example.com
    http:
      paths:
      # 用户服务
      - path: /api/v1/users
        pathType: Prefix
        backend:
          service:
            name: user-service
            port:
              number: 8080
      
      # 订单服务
      - path: /api/v1/orders
        pathType: Prefix
        backend:
          service:
            name: order-service
            port:
              number: 8080
      
      # 支付服务
      - path: /api/v1/payments
        pathType: Prefix
        backend:
          service:
            name: payment-service
            port:
              number: 8080
      
      # 通知服务
      - path: /api/v1/notifications
        pathType: Prefix
        backend:
          service:
            name: notification-service
            port:
              number: 8080
      
      # 健康检查(公开)
      - path: /health
        pathType: Exact
        backend:
          service:
            name: health-check
            port:
              number: 80
```

---

### 案例 2: 蓝绿部署与流量切换

**场景**: 零停机时间的版本切换

```yaml
# 蓝色环境(当前生产)
apiVersion: v1
kind: Service
metadata:
  name: app-blue
  namespace: production
spec:
  selector:
    app: myapp
    version: blue
  ports:
  - port: 80
    targetPort: 8080

---
# 绿色环境(新版本)
apiVersion: v1
kind: Service
metadata:
  name: app-green
  namespace: production
spec:
  selector:
    app: myapp
    version: green
  ports:
  - port: 80
    targetPort: 8080

---
# Ingress(初始指向蓝色)
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: app-ingress
  namespace: production
spec:
  ingressClassName: nginx
  rules:
  - host: app.example.com
    http:
      paths:
      - path: /
        pathType: Prefix
        backend:
          service:
            name: app-blue  # 初始指向蓝色
            port:
              number: 80
```

**切换流程**:
```bash
# 1. 部署绿色环境
kubectl apply -f deployment-green.yaml

# 2. 验证绿色环境健康
kubectl port-forward svc/app-green 9090:80 -n production
curl http://localhost:9090/health

# 3. 切换 Ingress 到绿色(编辑 YAML 或 patch)
kubectl patch ingress app-ingress -n production \
  --type='json' \
  -p='[{"op": "replace", "path": "/spec/rules/0/http/paths/0/backend/service/name", "value":"app-green"}]'

# 4. 验证生产流量
curl https://app.example.com/

# 5. 观察一段时间后,删除蓝色环境
kubectl delete deployment app-blue -n production
```

---

### 案例 3: 多租户 SaaS 平台

**场景**: 为每个租户提供独立子域名

```yaml
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: saas-platform
  namespace: saas
  annotations:
    cert-manager.io/cluster-issuer: "letsencrypt-prod"
    nginx.ingress.kubernetes.io/ssl-redirect: "true"
spec:
  ingressClassName: nginx
  tls:
  - hosts:
    - "*.example.com"  # 通配符证书
    secretName: wildcard-tls
  rules:
  # 主应用(www)
  - host: www.example.com
    http:
      paths:
      - path: /
        pathType: Prefix
        backend:
          service:
            name: frontend
            port:
              number: 80
  
  # 租户 A
  - host: tenant-a.example.com
    http:
      paths:
      - path: /
        pathType: Prefix
        backend:
          service:
            name: tenant-service
            port:
              number: 8080
  
  # 租户 B
  - host: tenant-b.example.com
    http:
      paths:
      - path: /
        pathType: Prefix
        backend:
          service:
            name: tenant-service
            port:
              number: 8080
  
  # 动态租户(由应用根据子域名路由)
  - host: "*.example.com"
    http:
      paths:
      - path: /
        pathType: Prefix
        backend:
          service:
            name: tenant-service
            port:
              number: 8080
```

**租户服务逻辑**(伪代码):
```go
func handler(w http.ResponseWriter, r *http.Request) {
    host := r.Host
    parts := strings.Split(host, ".")
    tenantID := parts[0]  // 提取子域名作为租户 ID
    
    // 根据租户 ID 路由到不同数据库/配置
    handleTenantRequest(tenantID, w, r)
}
```

---

## 相关资源

### 官方文档
- [Ingress 概念](https://kubernetes.io/docs/concepts/services-networking/ingress/)
- [Ingress API 参考](https://kubernetes.io/docs/reference/kubernetes-api/service-resources/ingress-v1/)
- [IngressClass API 参考](https://kubernetes.io/docs/reference/kubernetes-api/service-resources/ingress-class-v1/)
- [Ingress Controllers](https://kubernetes.io/docs/concepts/services-networking/ingress-controllers/)

### Ingress Controller 实现
- [NGINX Ingress Controller](https://kubernetes.github.io/ingress-nginx/)
- [Ingress-NGINX](https://github.com/nginxinc/kubernetes-ingress) (NGINX Inc.)
- [Traefik](https://doc.traefik.io/traefik/providers/kubernetes-ingress/)
- [HAProxy Ingress](https://haproxy-ingress.github.io/)
- [Contour](https://projectcontour.io/) (Envoy-based)
- [Istio Gateway](https://istio.io/latest/docs/tasks/traffic-management/ingress/)
- [Kong Ingress](https://docs.konghq.com/kubernetes-ingress-controller/)
- [AWS ALB Ingress Controller](https://kubernetes-sigs.github.io/aws-load-balancer-controller/)

### 证书管理
- [cert-manager](https://cert-manager.io/)
- [Let's Encrypt](https://letsencrypt.org/)

### 下一代标准
- [Gateway API](https://gateway-api.sigs.k8s.io/)

### 监控与可观测性
- [Prometheus NGINX Ingress Exporter](https://github.com/nginxinc/nginx-prometheus-exporter)
- [Grafana Dashboards for NGINX Ingress](https://grafana.com/grafana/dashboards/)

### 工具
- [kubectl-ingress-nginx](https://kubernetes.github.io/ingress-nginx/kubectl-plugin/) - NGINX Ingress 调试插件
- [ingress-conformance-bats](https://github.com/kubernetes-sigs/ingress-controller-conformance) - Ingress 一致性测试

---

**最后更新**: 2026-02  
**维护者**: Kubernetes 运维团队  
**反馈**: 如有问题请提交 Issue
