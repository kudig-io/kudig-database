# 127 - Ingress 基础概念与核心原理

> **适用版本**: Kubernetes v1.25 - v1.32 | **最后更新**: 2026-01 | **参考**: [kubernetes.io/docs/concepts/services-networking/ingress](https://kubernetes.io/docs/concepts/services-networking/ingress/)

---

## 一、Ingress 概念总览

### 1.1 什么是 Ingress

| 概念 | 英文术语 | 定义 | 作用 |
|-----|---------|------|------|
| **Ingress** | Ingress | Kubernetes API 对象，管理集群外部访问集群内服务的规则 | 定义 HTTP/HTTPS 路由规则 |
| **Ingress Controller** | Ingress Controller | 负责实现 Ingress 规则的控制器组件 | 实际处理流量转发 |
| **IngressClass** | IngressClass | 定义 Ingress 控制器类型的 API 对象 | 指定使用哪个控制器 |
| **Backend** | Backend | Ingress 路由到的后端服务 | 接收转发的流量 |
| **Default Backend** | Default Backend | 未匹配任何规则时的默认后端 | 处理未知请求 |

### 1.2 Ingress vs 其他网络方案对比

| 方案 | 协议支持 | 负载均衡层 | 外部访问方式 | 适用场景 | 复杂度 |
|-----|---------|-----------|------------|---------|-------|
| **ClusterIP** | TCP/UDP | L4 | 仅集群内部 | 内部服务通信 | 低 |
| **NodePort** | TCP/UDP | L4 | Node IP:端口 | 开发测试、简单场景 | 低 |
| **LoadBalancer** | TCP/UDP | L4 | 云LB外部IP | 单服务对外暴露 | 中 |
| **Ingress** | HTTP/HTTPS | L7 | 统一入口 | 多服务HTTP路由 | 中 |
| **Gateway API** | HTTP/HTTPS/TCP/UDP/gRPC | L4/L7 | 统一入口 | 复杂多协议场景 | 高 |
| **Service Mesh** | 全协议 | L4/L7 | Sidecar代理 | 微服务治理 | 高 |

### 1.3 Ingress 核心特性

| 特性 | 英文 | 说明 | 支持版本 |
|-----|------|------|---------|
| **基于主机名路由** | Host-based Routing | 根据 HTTP Host 头转发请求 | v1.0+ |
| **基于路径路由** | Path-based Routing | 根据 URL 路径转发请求 | v1.0+ |
| **TLS 终止** | TLS Termination | 在入口处解密 HTTPS 流量 | v1.0+ |
| **名称虚拟主机** | Name-based Virtual Hosting | 同一 IP 托管多个域名 | v1.0+ |
| **负载均衡** | Load Balancing | 在后端 Pod 间分配流量 | v1.0+ |
| **路径类型** | Path Types | Exact/Prefix/ImplementationSpecific | v1.18+ |
| **IngressClass** | IngressClass | 多控制器支持 | v1.18+ (GA v1.19) |

---

## 二、Ingress 资源模型

### 2.1 Ingress API 结构

```
Ingress (networking.k8s.io/v1)
├── metadata
│   ├── name                    # Ingress 名称
│   ├── namespace               # 命名空间
│   └── annotations             # 控制器特定配置
├── spec
│   ├── ingressClassName        # 指定 IngressClass
│   ├── defaultBackend          # 默认后端
│   │   └── service
│   │       ├── name            # Service 名称
│   │       └── port            # 端口配置
│   ├── tls[]                   # TLS 配置数组
│   │   ├── hosts[]             # 证书适用的主机名
│   │   └── secretName          # 证书 Secret 名称
│   └── rules[]                 # 路由规则数组
│       ├── host                # 主机名
│       └── http
│           └── paths[]         # 路径规则
│               ├── path        # URL 路径
│               ├── pathType    # 路径匹配类型
│               └── backend     # 后端服务
└── status
    └── loadBalancer            # 负载均衡器状态
        └── ingress[]
            ├── ip              # 分配的 IP
            └── hostname        # 分配的主机名
```

### 2.2 Ingress 规范字段详解

| 字段 | 类型 | 必填 | 说明 | 示例 |
|-----|------|-----|------|------|
| `metadata.name` | string | 是 | Ingress 资源名称 | `my-ingress` |
| `metadata.namespace` | string | 否 | 命名空间，默认 default | `production` |
| `metadata.annotations` | map | 否 | 控制器配置注解 | 见注解章节 |
| `spec.ingressClassName` | string | 推荐 | IngressClass 名称 | `nginx` |
| `spec.defaultBackend` | object | 否 | 默认后端配置 | 见下文 |
| `spec.tls` | array | 否 | TLS 配置列表 | 见 TLS 章节 |
| `spec.rules` | array | 否 | 路由规则列表 | 见下文 |

### 2.3 路径类型详解 (Path Types)

| 路径类型 | 英文 | 匹配规则 | 示例路径 `/foo` 匹配 | 不匹配 |
|---------|------|---------|-------------------|-------|
| **Exact** | 精确匹配 | 完全匹配 URL 路径 | `/foo` | `/foo/`, `/foo/bar` |
| **Prefix** | 前缀匹配 | 基于 `/` 分隔的路径前缀 | `/foo`, `/foo/`, `/foo/bar` | `/foobar` |
| **ImplementationSpecific** | 实现特定 | 由 IngressClass 决定 | 取决于控制器实现 | 取决于控制器实现 |

**Prefix 路径匹配细节:**

| 请求路径 | Ingress 路径 | 是否匹配 | 原因 |
|---------|-------------|---------|------|
| `/aaa/bb` | `/aaa/bb` | 是 | 精确前缀 |
| `/aaa/bbb` | `/aaa/bb` | 否 | 无 `/` 分隔 |
| `/aaa/bbb/` | `/aaa/bb` | 否 | 无 `/` 分隔 |
| `/aaa/bbb` | `/aaa/bbb/` | 是 | 尾部 `/` 忽略 |
| `/aaa/bbb/` | `/aaa/bbb` | 是 | 尾部 `/` 忽略 |
| `/aaa/bbb/ccc` | `/aaa/bbb` | 是 | 前缀匹配 |

---

## 三、IngressClass 详解

### 3.1 IngressClass 概念

| 概念 | 说明 |
|-----|------|
| **目的** | 支持集群中运行多个 Ingress 控制器 |
| **作用** | 指定 Ingress 由哪个控制器处理 |
| **默认 IngressClass** | 可设置为集群默认，未指定时自动使用 |
| **参数配置** | 支持通过 parameters 传递控制器特定配置 |

### 3.2 IngressClass 配置

```yaml
apiVersion: networking.k8s.io/v1
kind: IngressClass
metadata:
  name: nginx
  annotations:
    # 设置为默认 IngressClass
    ingressclass.kubernetes.io/is-default-class: "true"
spec:
  # 控制器标识符
  controller: k8s.io/ingress-nginx
  # 可选: 控制器参数
  parameters:
    apiGroup: k8s.nginx.org
    kind: IngressNginxConfiguration
    name: nginx-config
    namespace: ingress-nginx
    # scope: Namespace 或 Cluster
    scope: Namespace
```

### 3.3 常见 IngressClass Controller 标识

| 控制器 | Controller 标识符 | IngressClass 名称建议 |
|-------|-----------------|---------------------|
| NGINX Ingress (社区版) | `k8s.io/ingress-nginx` | `nginx` |
| NGINX Ingress (F5版) | `nginx.org/ingress-controller` | `nginx-plus` |
| Traefik | `traefik.io/ingress-controller` | `traefik` |
| HAProxy | `haproxy.org/ingress-controller` | `haproxy` |
| Contour | `projectcontour.io/ingress-controller` | `contour` |
| Kong | `ingress-controllers.konghq.com/kong` | `kong` |
| ALB (阿里云) | `ingress.k8s.alibabacloud/alb` | `alb` |
| AWS ALB | `ingress.k8s.aws/alb` | `alb` |
| GCE | `ingress.gce.io/ingress` | `gce` |

---

## 四、Ingress 流量转发原理

### 4.1 Ingress 工作流程

```
                                    ┌─────────────────────────────────────────────┐
                                    │              Kubernetes Cluster              │
┌──────────┐    ┌───────────────┐   │  ┌───────────────────────────────────────┐  │
│  Client  │───>│  External LB  │───│─>│        Ingress Controller             │  │
│ (浏览器) │    │  (可选,云环境) │   │  │  (NGINX/Traefik/HAProxy/Envoy等)      │  │
└──────────┘    └───────────────┘   │  └─────────────────┬─────────────────────┘  │
                                    │                    │                         │
                                    │         ┌─────────┴─────────┐               │
                                    │         │  读取 Ingress 规则  │               │
                                    │         └─────────┬─────────┘               │
                                    │                   ↓                         │
                                    │  ┌────────────────────────────────────────┐ │
                                    │  │         Ingress Resource               │ │
                                    │  │  ┌─────────────────────────────────┐   │ │
                                    │  │  │ rules:                          │   │ │
                                    │  │  │   - host: api.example.com       │   │ │
                                    │  │  │     http:                       │   │ │
                                    │  │  │       paths:                    │   │ │
                                    │  │  │         - path: /v1             │   │ │
                                    │  │  │           backend: svc-v1       │   │ │
                                    │  │  │         - path: /v2             │   │ │
                                    │  │  │           backend: svc-v2       │   │ │
                                    │  │  └─────────────────────────────────┘   │ │
                                    │  └────────────────────────────────────────┘ │
                                    │                   ↓                         │
                                    │  ┌────────────────────────────────────────┐ │
                                    │  │           Service (ClusterIP)          │ │
                                    │  │         svc-v1        svc-v2           │ │
                                    │  └──────────┬───────────────┬─────────────┘ │
                                    │             ↓               ↓               │
                                    │  ┌─────────────────┐ ┌─────────────────┐   │
                                    │  │   Pod (v1)      │ │   Pod (v2)      │   │
                                    │  │  ┌───────────┐  │ │  ┌───────────┐  │   │
                                    │  │  │ Container │  │ │  │ Container │  │   │
                                    │  │  └───────────┘  │ │  └───────────┘  │   │
                                    │  └─────────────────┘ └─────────────────┘   │
                                    └─────────────────────────────────────────────┘
```

### 4.2 Ingress Controller 工作原理

| 阶段 | 步骤 | 说明 |
|-----|------|------|
| **1. 监听** | Watch API Server | 监听 Ingress、Service、Endpoints、Secret 等资源变化 |
| **2. 解析** | Parse Rules | 解析 Ingress 规则，生成路由配置 |
| **3. 配置** | Generate Config | 生成代理服务器配置 (如 nginx.conf) |
| **4. 重载** | Reload/Hot Update | 重新加载配置或热更新路由 |
| **5. 转发** | Forward Traffic | 根据规则将流量转发到后端 Service |
| **6. 上报** | Update Status | 更新 Ingress 状态 (分配的 IP/Hostname) |

### 4.3 流量转发路径对比

| 转发模式 | 路径 | 特点 | 控制器支持 |
|---------|-----|------|-----------|
| **Service 转发** | Ingress → Service → Pod | 经过 kube-proxy，有额外跳转 | 默认模式 |
| **Endpoint 直连** | Ingress → Pod (Endpoints) | 绕过 Service，性能更优 | NGINX, Traefik |
| **EndpointSlice** | Ingress → Pod (EndpointSlices) | 大规模集群优化 | 现代控制器 |

---

## 五、基础 Ingress 配置示例

### 5.1 单服务 Ingress

```yaml
# 最简单的 Ingress，将所有流量转发到一个服务
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: minimal-ingress
  namespace: default
spec:
  ingressClassName: nginx
  defaultBackend:
    service:
      name: web-service
      port:
        number: 80
```

### 5.2 基于主机名的路由

```yaml
# 不同域名转发到不同服务
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: host-based-routing
  namespace: default
spec:
  ingressClassName: nginx
  rules:
  # 主站
  - host: www.example.com
    http:
      paths:
      - path: /
        pathType: Prefix
        backend:
          service:
            name: www-service
            port:
              number: 80
  # API 站点
  - host: api.example.com
    http:
      paths:
      - path: /
        pathType: Prefix
        backend:
          service:
            name: api-service
            port:
              number: 8080
  # 管理后台
  - host: admin.example.com
    http:
      paths:
      - path: /
        pathType: Prefix
        backend:
          service:
            name: admin-service
            port:
              number: 3000
```

### 5.3 基于路径的路由

```yaml
# 同一域名不同路径转发到不同服务
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: path-based-routing
  namespace: default
spec:
  ingressClassName: nginx
  rules:
  - host: app.example.com
    http:
      paths:
      # API 服务 - 精确匹配 /api
      - path: /api
        pathType: Prefix
        backend:
          service:
            name: api-service
            port:
              number: 8080
      # 用户服务
      - path: /users
        pathType: Prefix
        backend:
          service:
            name: user-service
            port:
              number: 8081
      # 订单服务
      - path: /orders
        pathType: Prefix
        backend:
          service:
            name: order-service
            port:
              number: 8082
      # 默认 - 前端服务
      - path: /
        pathType: Prefix
        backend:
          service:
            name: frontend-service
            port:
              number: 80
```

### 5.4 混合路由 (主机名 + 路径)

```yaml
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: mixed-routing
  namespace: production
spec:
  ingressClassName: nginx
  tls:
  - hosts:
    - "*.example.com"
    secretName: wildcard-tls-secret
  rules:
  # 生产环境 API
  - host: api.example.com
    http:
      paths:
      - path: /v1
        pathType: Prefix
        backend:
          service:
            name: api-v1
            port:
              number: 8080
      - path: /v2
        pathType: Prefix
        backend:
          service:
            name: api-v2
            port:
              number: 8080
  # 移动端专用 API
  - host: mobile-api.example.com
    http:
      paths:
      - path: /
        pathType: Prefix
        backend:
          service:
            name: mobile-api
            port:
              number: 8080
  # Web 应用
  - host: www.example.com
    http:
      paths:
      - path: /static
        pathType: Prefix
        backend:
          service:
            name: static-service
            port:
              number: 80
      - path: /
        pathType: Prefix
        backend:
          service:
            name: web-app
            port:
              number: 3000
```

### 5.5 使用服务名称端口

```yaml
# 使用 Service 的命名端口而非端口号
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: named-port-ingress
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
            name: web-service
            port:
              # 使用 Service 中定义的端口名称
              name: http
---
apiVersion: v1
kind: Service
metadata:
  name: web-service
spec:
  selector:
    app: web
  ports:
  - name: http        # 端口名称
    port: 80
    targetPort: 8080
  - name: https
    port: 443
    targetPort: 8443
```

---

## 六、Ingress 版本演进

### 6.1 API 版本演进

| 版本 | API Version | 状态 | K8s 版本 |
|-----|-------------|------|---------|
| **Alpha** | `extensions/v1beta1` | 已弃用 | v1.1 - v1.13 |
| **Beta** | `networking.k8s.io/v1beta1` | 已弃用 | v1.14 - v1.21 |
| **GA** | `networking.k8s.io/v1` | 稳定 | v1.19+ |

### 6.2 各版本关键变更

| K8s 版本 | Ingress 相关变更 |
|---------|-----------------|
| **v1.18** | IngressClass 引入 (Beta)，pathType 字段引入 |
| **v1.19** | Ingress 和 IngressClass GA |
| **v1.20** | `kubernetes.io/ingress.class` 注解弃用警告 |
| **v1.22** | `extensions/v1beta1` 和 `networking.k8s.io/v1beta1` 移除 |
| **v1.25** | 默认 IngressClass 行为改进 |
| **v1.28** | Ingress 路径匹配增强，更严格的验证 |
| **v1.30** | Gateway API v1.0 GA，与 Ingress 互补 |
| **v1.31** | Gateway API TCPRoute/UDPRoute Beta |

### 6.3 迁移到 networking.k8s.io/v1

```yaml
# 旧版本 (extensions/v1beta1) - 已不支持
apiVersion: extensions/v1beta1
kind: Ingress
metadata:
  name: old-ingress
  annotations:
    kubernetes.io/ingress.class: nginx  # 旧方式
spec:
  rules:
  - host: example.com
    http:
      paths:
      - path: /
        backend:
          serviceName: web-service   # 旧字段名
          servicePort: 80            # 旧字段名
---
# 新版本 (networking.k8s.io/v1) - 当前标准
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: new-ingress
spec:
  ingressClassName: nginx            # 新方式
  rules:
  - host: example.com
    http:
      paths:
      - path: /
        pathType: Prefix             # 必填字段
        backend:
          service:                   # 结构化配置
            name: web-service
            port:
              number: 80
```

---

## 七、Ingress 与相关资源关系

### 7.1 资源依赖关系

| Ingress 配置项 | 依赖资源 | 关系说明 |
|---------------|---------|---------|
| `spec.ingressClassName` | IngressClass | 指定使用的控制器类 |
| `spec.rules[].backend.service` | Service | 路由目标服务 |
| `spec.tls[].secretName` | Secret (TLS) | TLS 证书存储 |
| `spec.defaultBackend` | Service | 默认后端服务 |
| (隐式) | Endpoints/EndpointSlices | 实际 Pod 地址 |

### 7.2 完整资源示例

```yaml
# 1. IngressClass
apiVersion: networking.k8s.io/v1
kind: IngressClass
metadata:
  name: nginx
  annotations:
    ingressclass.kubernetes.io/is-default-class: "true"
spec:
  controller: k8s.io/ingress-nginx
---
# 2. TLS Secret
apiVersion: v1
kind: Secret
metadata:
  name: app-tls
  namespace: default
type: kubernetes.io/tls
data:
  tls.crt: <base64-encoded-cert>
  tls.key: <base64-encoded-key>
---
# 3. Service
apiVersion: v1
kind: Service
metadata:
  name: web-service
  namespace: default
spec:
  selector:
    app: web
  ports:
  - name: http
    port: 80
    targetPort: 8080
---
# 4. Deployment
apiVersion: apps/v1
kind: Deployment
metadata:
  name: web-deployment
  namespace: default
spec:
  replicas: 3
  selector:
    matchLabels:
      app: web
  template:
    metadata:
      labels:
        app: web
    spec:
      containers:
      - name: web
        image: nginx:1.21
        ports:
        - containerPort: 8080
---
# 5. Ingress
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: web-ingress
  namespace: default
  annotations:
    nginx.ingress.kubernetes.io/ssl-redirect: "true"
spec:
  ingressClassName: nginx
  tls:
  - hosts:
    - web.example.com
    secretName: app-tls
  rules:
  - host: web.example.com
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

---

## 八、常用 kubectl 命令

### 8.1 Ingress 管理命令

| 命令 | 说明 |
|-----|------|
| `kubectl get ingress` | 列出所有 Ingress |
| `kubectl get ingress -A` | 列出所有命名空间的 Ingress |
| `kubectl get ingress <name> -o yaml` | 查看 Ingress YAML |
| `kubectl describe ingress <name>` | 查看 Ingress 详情 |
| `kubectl create ingress <name> --rule="host/path=svc:port"` | 创建 Ingress |
| `kubectl edit ingress <name>` | 编辑 Ingress |
| `kubectl delete ingress <name>` | 删除 Ingress |

### 8.2 实操命令示例

```bash
# 列出所有 Ingress 及其关联的后端
kubectl get ingress -A -o wide

# 查看 Ingress 详细信息（含 Events）
kubectl describe ingress my-ingress -n production

# 使用命令行创建简单 Ingress
kubectl create ingress demo-ingress \
  --class=nginx \
  --rule="demo.example.com/*=demo-service:80"

# 创建带 TLS 的 Ingress
kubectl create ingress secure-ingress \
  --class=nginx \
  --rule="secure.example.com/*=web-service:80,tls=tls-secret"

# 查看 IngressClass
kubectl get ingressclass

# 查看默认 IngressClass
kubectl get ingressclass -o jsonpath='{.items[?(@.metadata.annotations.ingressclass\.kubernetes\.io/is-default-class=="true")].metadata.name}'

# 检查 Ingress 控制器 Pod 状态
kubectl get pods -n ingress-nginx

# 查看 Ingress 控制器日志
kubectl logs -n ingress-nginx -l app.kubernetes.io/component=controller --tail=100

# 测试 Ingress 是否生效
curl -H "Host: demo.example.com" http://<INGRESS-IP>/
```

---

## 九、Ingress 设计原则与最佳实践

### 9.1 设计原则

| 原则 | 说明 | 建议 |
|-----|------|------|
| **单一职责** | 每个 Ingress 负责一个域名或一组相关服务 | 避免过度复杂的单个 Ingress |
| **命名规范** | 使用有意义的命名 | `<app>-<env>-ingress` |
| **版本控制** | Ingress 配置纳入 GitOps | 使用版本控制管理 |
| **渐进发布** | 使用金丝雀/蓝绿部署 | 降低发布风险 |
| **监控告警** | 监控 Ingress 可用性和性能 | 配置 Prometheus 指标 |

### 9.2 反模式

| 反模式 | 问题 | 正确做法 |
|-------|-----|---------|
| **单点故障** | 控制器无高可用 | 部署多副本 + PodAntiAffinity |
| **资源无限制** | 控制器资源耗尽 | 配置 requests/limits |
| **无 TLS** | 明文传输敏感数据 | 启用 TLS，强制 HTTPS 重定向 |
| **过长超时** | 连接资源耗尽 | 合理配置超时参数 |
| **无速率限制** | 易受 DDoS 攻击 | 配置限流策略 |

---

**Ingress 核心原则**: 选择合适的控制器 → 合理规划路由结构 → 启用 TLS 加密 → 配置高可用 → 实施监控告警

---

**表格底部标记**: Kusheet Project, 作者 Allen Galler (allengaller@gmail.com)
