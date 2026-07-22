---
title: HTTP/HTTPS 协议
description: HTTP/1.1、HTTP/2、HTTP/3 协议演进、TLS 握手、证书链、Ingress/Gateway 中的 HTTP 处理、常见状态码
summary: HTTP/HTTPS 完整知识，覆盖协议版本对比、TLS 1.3 握手、证书管理、K8s Ingress HTTP 处理、故障排查
category: knowledge
tags:
- networking
- http
- https
- tls
- ingress
domain: 系统基础
difficulty: intermediate
audience:
- SRE
- 平台工程师
- 开发工程师
---

# HTTP/HTTPS 协议

> HTTP 是云原生应用最核心的应用层协议。理解 HTTP 各版本特性、TLS 加密机制、以及 K8s Ingress/Gateway 中的 HTTP 处理，是构建和运维微服务的基础。

## HTTP 协议演进

### 版本对比

| 特性 | HTTP/1.0 | HTTP/1.1 | HTTP/2 | HTTP/3 |
|------|----------|----------|--------|--------|
| 发布年份 | 1996 | 1997 | 2015 | 2022 |
| 传输层 | TCP | TCP | TCP | QUIC (UDP) |
| 连接复用 | 无 | Keep-Alive | 多路复用 | 多路复用 |
| 队头阻塞 | 严重 | 应用层 | 传输层 | 无 |
| 头部压缩 | 无 | 无 | HPACK | QPACK |
| 服务器推送 | 无 | 无 | 有 | 有 |
| 连接迁移 | 无 | 无 | 无 | 支持 |
| 加密 | 可选 | 可选 | 事实上必须 | 必须 |
| K8s 场景 | 遗留 | 传统服务 | gRPC/Ingress | 前沿 |

### HTTP/1.1 关键特性

```
# 持久连接 (Keep-Alive)
Connection: keep-alive

# 管线化 (Pipelining) - 实际很少使用
GET /a HTTP/1.1
GET /b HTTP/1.1  (不等 /a 响应就发送)

# 分块传输
Transfer-Encoding: chunked

# 虚拟主机
Host: www.example.com
```

**HTTP/1.1 的问题：**
- 队头阻塞：一个请求阻塞后续所有请求
- 头部冗余：每次请求重复发送大量头部
- 并发限制：浏览器同域最多 6 个 TCP 连接

### HTTP/2 核心机制

```
┌─────────────────────────────────────┐
│          HTTP/2 连接                 │
│                                      │
│  ┌──────┐  ┌──────┐  ┌──────┐      │
│  │Stream1│  │Stream2│  │Stream3│     │  ← 多路复用
│  │GET /a │  │GET /b │  │POST /c│    │
│  └──┬───┘  └──┬───┘  └──┬───┘      │
│     │         │         │            │
│     ▼         ▼         ▼            │
│  ┌─────────────────────────────┐    │
│  │     HPACK 压缩的帧          │    │  ← 二进制分帧
│  └─────────────────────────────┘    │
│                                      │
│  ┌─────────────────────────────┐    │
│  │        TCP 连接              │    │  ← 单连接
│  └─────────────────────────────┘    │
└─────────────────────────────────────┘
```

**HTTP/2 帧类型：**

| 帧类型 | 用途 |
|--------|------|
| DATA | 传输请求/响应体 |
| HEADERS | 传输头部（HPACK 压缩） |
| SETTINGS | 连接参数协商 |
| WINDOW_UPDATE | 流控 |
| PING | 心跳/延迟测量 |
| GOAWAY | 优雅关闭 |
| RST_STREAM | 取消单个流 |

### HTTP/3 (QUIC)

```
HTTP/3 优势:
- 基于 UDP，无队头阻塞
- 0-RTT 连接建立（会话恢复）
- 连接迁移（IP 变化不断连）
- 内置加密（TLS 1.3）

适用场景:
- 移动端（网络切换频繁）
- 高延迟网络
- 实时通信
```

## TLS/HTTPS

### TLS 1.3 握手

```
完整握手 (1-RTT):
Client ──── ClientHello ──────────────────→ Server
           (supported_versions, key_share,
            cipher_suites, SNI)

Client ←── ServerHello ──────────────────── Server
           (key_share, cipher_suite)
Client ←── {EncryptedExtensions} ────────── Server
Client ←── {Certificate} ────────────────── Server
Client ←── {CertificateVerify} ──────────── Server
Client ←── {Finished} ───────────────────── Server

Client ──── {Finished} ───────────────────→ Server
           [应用数据传输开始]

0-RTT (会话恢复):
Client ──── ClientHello + EarlyData ──────→ Server
           [立即发送应用数据]
```

### TLS 1.3 vs 1.2

| 特性 | TLS 1.2 | TLS 1.3 |
|------|---------|---------|
| RTT | 2-RTT | 1-RTT (0-RTT 恢复) |
| 密钥交换 | RSA/DHE/ECDHE | 仅 (EC)DHE |
| 加密算法 | 多种(含弱算法) | AEAD only |
| 前向安全 | 可选 | 强制 |
| 握手加密 | 明文 | 加密 |

### 证书链验证

```
根 CA (Root CA) - 自签名，预装在操作系统/浏览器
    │ 签发
    ▼
中间 CA (Intermediate CA)
    │ 签发
    ▼
服务器证书 (Leaf Certificate)
    │
    ├── CN/SAN: 域名匹配
    ├── 有效期: notBefore < now < notAfter
    ├── 密钥用途: digitalSignature, keyEncipherment
    └── 扩展密钥用途: serverAuth
```

### K8s TLS 证书管理

```yaml
# TLS Secret
apiVersion: v1
kind: Secret
metadata:
  name: tls-secret
  namespace: default
type: kubernetes.io/tls
data:
  tls.crt: <base64-encoded-cert-chain>
  tls.key: <base64-encoded-private-key>
---
# Ingress TLS 配置
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: secure-ingress
spec:
  tls:
  - hosts:
    - app.example.com
    secretName: tls-secret
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

### cert-manager 自动证书

```yaml
apiVersion: cert-manager.io/v1
kind: Certificate
metadata:
  name: app-cert
spec:
  secretName: app-tls
  dnsNames:
  - app.example.com
  - "*.app.example.com"
  issuerRef:
    name: letsencrypt-prod
    kind: ClusterIssuer
  renewBefore: 720h  # 到期前30天续期
```

## HTTP 状态码（K8s 场景）

### 4xx 客户端错误

| 状态码 | 含义 | K8s 常见原因 | 排查方向 |
|--------|------|-------------|----------|
| 400 | Bad Request | API 参数错误 | 检查请求体 |
| 401 | Unauthorized | Token 过期/无效 | 检查 ServiceAccount |
| 403 | Forbidden | RBAC 权限不足 | `kubectl auth can-i` |
| 404 | Not Found | 路由/Service 不存在 | 检查 Ingress 规则 |
| 408 | Request Timeout | 客户端超时 | 检查网络 |
| 413 | Payload Too Large | 请求体超限 | 调整 proxy-body-size |
| 429 | Too Many Requests | 限流 | 检查 RateLimit 配置 |

### 5xx 服务端错误

| 状态码 | 含义 | K8s 常见原因 | 排查方向 |
|--------|------|-------------|----------|
| 500 | Internal Error | 应用异常 | 查看 Pod 日志 |
| 502 | Bad Gateway | 后端不可达 | 检查 Endpoints |
| 503 | Service Unavailable | 无健康后端 | 检查 Readiness |
| 504 | Gateway Timeout | 后端响应超时 | 调整超时配置 |

### 502 vs 503 vs 504 区分

```
502 Bad Gateway:
  Ingress Controller → 后端 Pod: 连接被拒绝 (Pod 崩溃/端口错误)

503 Service Unavailable:
  Ingress Controller → 无可用后端 (Endpoints 为空/所有 Pod NotReady)

504 Gateway Timeout:
  Ingress Controller → 后端 Pod: 连接成功但响应超时
```

## K8s 中的 HTTP 处理

### Ingress 请求处理流程

```
客户端 HTTPS 请求
    │
    ▼
LoadBalancer (云 LB / MetalLB)
    │
    ▼
Ingress Controller Pod (Nginx/Envoy)
    │
    ├── TLS 终止 (使用 Secret 中的证书)
    ├── 路由匹配 (Host + Path)
    ├── 中间件处理 (限流/认证/重写)
    │
    ▼
后端 Service (ClusterIP)
    │
    ▼ (kube-proxy DNAT)
后端 Pod (TargetPort)
```

### Nginx Ingress 关键配置

```yaml
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: app-ingress
  annotations:
    # 超时配置
    nginx.ingress.kubernetes.io/proxy-connect-timeout: "5"
    nginx.ingress.kubernetes.io/proxy-read-timeout: "60"
    nginx.ingress.kubernetes.io/proxy-send-timeout: "60"
    # 请求体大小
    nginx.ingress.kubernetes.io/proxy-body-size: "50m"
    # 限流
    nginx.ingress.kubernetes.io/limit-rps: "100"
    # 重试
    nginx.ingress.kubernetes.io/proxy-next-upstream: "error timeout http_502 http_503"
    nginx.ingress.kubernetes.io/proxy-next-upstream-tries: "3"
    # WebSocket
    nginx.ingress.kubernetes.io/proxy-http-version: "1.1"
    nginx.ingress.kubernetes.io/configuration-snippet: |
      proxy_set_header Upgrade $http_upgrade;
      proxy_set_header Connection "upgrade";
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
            name: app-service
            port:
              number: 80
```

### Gateway API HTTP 处理

```yaml
apiVersion: gateway.networking.k8s.io/v1
kind: HTTPRoute
metadata:
  name: app-route
spec:
  parentRefs:
  - name: main-gateway
  hostnames:
  - app.example.com
  rules:
  - matches:
    - path:
        type: PathPrefix
        value: /api
    filters:
    - type: RequestHeaderModifier
      requestHeaderModifier:
        add:
        - name: X-Request-ID
          value: "%REQ_ID%"
    backendRefs:
    - name: api-service
      port: 8080
      weight: 90
    - name: api-service-canary
      port: 8080
      weight: 10  # 10% 流量到金丝雀
```

## HTTP 故障排查

### curl 诊断命令

```bash
# 🟢 完整请求信息
curl -vvv http://app.example.com/api

# 🟢 只看响应头
curl -I http://app.example.com/

# 🟢 计时分析
curl -o /dev/null -s -w '\
    DNS: %{time_namelookup}s\n\
    Connect: %{time_connect}s\n\
    TLS: %{time_appconnect}s\n\
    TTFB: %{time_starttransfer}s\n\
    Total: %{time_total}s\n\
    HTTP Code: %{http_code}\n' \
    https://app.example.com/

# 🟢 指定 IP 测试（跳过 DNS）
curl --resolve app.example.com:443:10.0.1.100 https://app.example.com/

# 🟢 忽略证书验证（仅测试）
curl -k https://self-signed.example.com/

# 🟢 查看证书信息
curl -vI https://app.example.com/ 2>&1 | openssl x509 -noout -dates -subject

# 🟢 HTTP/2 测试
curl --http2 -I https://app.example.com/
```

### 常见 HTTP 问题排查

| 症状 | 可能原因 | 排查命令 |
|------|----------|----------|
| 连接超时 | 防火墙/NetworkPolicy | `nc -zv <pod-ip> <port>` |
| 连接拒绝 | Pod 未监听/崩溃 | `kubectl logs <pod>` |
| 502 | 后端不可达 | `kubectl get endpoints <svc>` |
| 503 | 无健康后端 | `kubectl get pods -l app=x` |
| 504 | 后端响应慢 | `kubectl logs`, 调整超时 |
| TLS 错误 | 证书过期/域名不匹配 | `openssl s_client -connect` |
| 重定向循环 | Ingress 配置冲突 | 检查 annotations |

### TLS 证书排查

```bash
# 🟢 检查远程证书
openssl s_client -connect app.example.com:443 -servername app.example.com

# 🟢 查看证书有效期
echo | openssl s_client -connect app.example.com:443 2>/dev/null | openssl x509 -noout -dates

# 🟢 验证证书链
openssl verify -CAfile ca-bundle.crt server.crt

# 🟢 检查 K8s Secret 中的证书
kubectl get secret tls-secret -o jsonpath='{.data.tls\.crt}' | base64 -d | openssl x509 -noout -text

# 🟢 检查 cert-manager 证书状态
kubectl get certificates -A
kubectl describe certificate app-cert
```

## 生产案例

### 案例1：Ingress 502 错误

**症状：** 滚动更新期间出现大量 502

**根因：** Pod 终止时 Endpoints 更新有延迟，Ingress 仍转发到已终止的 Pod

**解决：**
```yaml
# 添加 preStop 等待
lifecycle:
  preStop:
    exec:
      command: ["/bin/sh", "-c", "sleep 10"]
```

### 案例2：TLS 证书过期导致服务中断

**症状：** 用户报 `NET::ERR_CERT_DATE_INVALID`

**根因：** 手动管理的证书过期，未配置自动续期

**解决：** 部署 cert-manager + Let's Encrypt 自动续期

### 案例3：HTTP/2 与 gRPC 不兼容

**症状：** gRPC 调用返回 `UNAVAILABLE`

**根因：** Ingress 未启用 HTTP/2 后端通信

**解决：**
```yaml
nginx.ingress.kubernetes.io/backend-protocol: "GRPC"
nginx.ingress.kubernetes.io/use-regex: "true"
```

## 版本兼容矩阵

| 组件 | HTTP/2 | HTTP/3 | TLS 1.3 |
|------|--------|--------|---------|
| Nginx Ingress | ✓ | 实验性 | ✓ |
| Envoy/Istio | ✓ | ✓ (1.20+) | ✓ |
| Gateway API | ✓ | 规划中 | ✓ |
| Traefik | ✓ | ✓ (2.10+) | ✓ |
| HAProxy | ✓ | ✓ (2.6+) | ✓ |

## 检查清单

- [ ] 理解 HTTP/1.1、HTTP/2、HTTP/3 区别
- [ ] 掌握 TLS 1.3 握手流程
- [ ] 能排查 HTTP 状态码问题
- [ ] 理解 Ingress TLS 终止配置
- [ ] 掌握 curl 诊断技巧
- [ ] 能管理 K8s TLS 证书
- [ ] 理解 HTTP/2 对 gRPC 的重要性
- [ ] 掌握超时和重试配置

## 参考链接

- [[系统基础/网络基础/index.md|网络基础总索引]]
- [[系统基础/速查卡/tls-pki.md|TLS/PKI 速查卡]]
- [[系统基础/速查卡/gateway-api.md|Gateway API 速查卡]]
- [[系统基础/知识字典/networking/index.md|网络知识字典]]
