---
title: 'Day 23: Ingress 实操'
description: '**日期**: Week 4 Day 2 | **主题**: Ingress 路由规则与控制器配置 | **版本**: K8s 1.28-1.33'
category: learning
tags:
- k8s
- training
- hands-on
- envoy
- helm
- ingress
- gateway
- rag
- cilium
- flannel
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 'Day 23: Ingress 实操 是什么'
- '如何 Day 23: Ingress 实操'
trigger_keywords:
- Day
- '23:'
- Ingress
- 实操
- learn
prerequisites:
- kubectl-basics
- gpu-ml-basics
- helm-basics
- ebpf-basics
- cilium-basics
- cni-basics
- tls-basics
created: "2026-05-23"
---

# [[skills/training-public/inner-training/week-4-network-storage/day-23-ingress|Day 23: Ingress]]ss|Ingress]] 实操

> **日期**: Week 4 Day 2 | **主题**: Ingress 路由规则与控制器配置 | **版本**: K8s 1.28-1.33

---

## 1. Ingress 核心概念

### 1.1 Ingress 架构

```
客户端 → Ingress Controller (Nginx/Traefik/Envoy) → Service → Pod
              ↓
         Ingress Resource (路由规则)
```

### 1.2 Ingress 控制器类型

| 控制器 | 特点 | 适用场景 |
|--------|------|---------|
| NGINX Ingress Controller | 功能丰富，性能高 | 生产环境 |
| Traefik | 支持 Let's Encrypt 自动证书 | 内部服务 |
| Ambassador | 基于 [[Envoy|Envoy]]，支持 canary | API Gateway |
| GKE Ingress | GCP 原生集成 | GCP 环境 |

---

## 2. 安装 Ingress Controller

### 2.1 NGINX Ingress Controller

```bash
# 方式 1: Helm 安装
helm repo add ingress-nginx https://kubernetes.github.io/ingress-nginx
helm repo update
helm install nginx-ingress ingress-nginx/ingress-nginx \
  --namespace ingress-nginx \
  --create-namespace \
  --set controller.replicaCount=2 \
  --set controller.service.type=LoadBalancer

# 方式 2: 手动部署
kubectl apply -f https://raw.githubusercontent.com/kubernetes/ingress-nginx/main/deploy/static/provider/cloud/deploy.yaml
```

### 2.2 验证安装

```bash
# 检查 Ingress Controller Pod
kubectl get pods -n ingress-nginx -l app.kubernetes.io/name=ingress-nginx

# 查看 Ingress Class
kubectl get ingressclass

# 获取 LoadBalancer IP
kubectl get svc -n ingress-nginx
```

---

## 3. Ingress 路由配置

### 3.1 基础路由

```yaml
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: web-ingress
  annotations:
    nginx.ingress.kubernetes.io/rewrite-target: /
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
                name: web-svc
                port:
                  number: 80
```

### 3.2 路径重写

```yaml
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: api-ingress
  annotations:
    nginx.ingress.kubernetes.io/rewrite-target: /api/$2
spec:
  ingressClassName: nginx
  rules:
    - host: api.example.com
      http:
        paths:
          - path: /web(/|$)(.*)
            pathType: ImplementationSpecific
            backend:
              service:
                name: web-svc
                port:
                  number: 80
          - path: /auth(/|$)(.*)
            pathType: ImplementationSpecific
            backend:
              service:
                name: auth-svc
                port:
                  number: 8080
```

### 3.3 基于域名的多服务路由

```yaml
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: multi-host-ingress
spec:
  ingressClassName: nginx
  rules:
    - host: web.example.com
      http:
        paths:
          - path: /
            pathType: Prefix
            backend:
              service:
                name: web-svc
                port:
                  number: 80
    - host: api.example.com
      http:
        paths:
          - path: /
            pathType: Prefix
            backend:
              service:
                name: api-svc
                port:
                  number: 8080
```

---

## 4. TLS 配置

### 4.1 HTTPS 路由

```yaml
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: https-ingress
spec:
  ingressClassName: nginx
  tls:
    - hosts:
        - api.example.com
      secretName: api-tls-secret  # 包含证书和私钥
  rules:
    - host: api.example.com
      http:
        paths:
          - path: /
            pathType: Prefix
            backend:
              service:
                name: api-svc
                port:
                  number: 8080
```

### 4.2 自动 Let's Encrypt 证书

```yaml
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: tls-ingress
  annotations:
    [[cert-manager|cert-manager]].io/cluster-issuer: letsencrypt-prod
    nginx.ingress.kubernetes.io/ssl-redirect: "true"
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
          - path: /
            pathType: Prefix
            backend:
              service:
                name: api-svc
                port:
                  number: 8080
```

### 4.3 创建 TLS Secret

```bash
# 从证书文件创建
kubectl create secret tls api-tls-secret \
  --cert=api.crt \
  --key=api.key

# 从 PEM 文件创建
kubectl create secret tls api-tls-secret \
  --cert=/path/to/cert.pem \
  --key=/path/to/key.pem

# 查看 secret
kubectl get secret api-tls-secret -o yaml
```

---

## 5. 流量控制

### 5.1 基于 Header 的灰度发布

```yaml
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: canary-ingress
  annotations:
    nginx.ingress.kubernetes.io/canary: "true"
    nginx.ingress.kubernetes.io/canary-weight: "30"  # 30% 流量到新版本
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
                name: api-svc-canary
                port:
                  number: 8080
```

### 5.2 基于 Header 的路由

```yaml
annotations:
  nginx.ingress.kubernetes.io/canary: "true"
  nginx.ingress.kubernetes.io/canary-by-header: "X-Canary"
  nginx.ingress.kubernetes.io/canary-by-header-value: "always"  # 带此 header 全部走 canary
```

### 5.3 速率限制

```yaml
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: rate-limited-ingress
  annotations:
    nginx.ingress.kubernetes.io/limit-rps: "10"  # 每秒 10 请求
    nginx.ingress.kubernetes.io/limit-connections: "100"  # 最大 100 并发
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
                name: api-svc
                port:
                  number: 8080
```

---

## 6. Ingress 故障排查

### 6.1 常见问题

```bash
# 1. Ingress 返回 404
# 检查 Ingress 规则是否匹配
kubectl describe ingress web-ingress

# 检查 Ingress Controller 是否正确配置
kubectl logs -n ingress-nginx -l app.kubernetes.io/name=ingress-nginx --tail=20

# 2. TLS 证书问题
kubectl describe ingress web-ingress | grep -A5 "TLS"

# 检查 secret 是否存在
kubectl get secret api-tls-secret

# 3. 路径匹配问题
# 检查 pathType 是否正确（Prefix / Exact / ImplementationSpecific）
```

### 6.2 调试命令

```bash
# 查看 Ingress 状态
kubectl get ingress -A

# 查看 Ingress 描述
kubectl describe ingress web-ingress

# 测试 DNS 解析
nslookup api.example.com

# 测试 HTTPS 证书
openssl s_client -connect api.example.com:443 -servername api.example.com

# 查看 Controller 日志
kubectl logs -n ingress-nginx -l app.kubernetes.io/name=ingress-nginx -f
```

---

## 7. 实战练习

**练习 1**: 部署 NGINX Ingress Controller，配置 HTTPS 路由到后端 Service

**练习 2**: 配置基于域名的多服务路由（web.example.com 和 api.example.com）

**练习 3**: 配置 Let's Encrypt 自动证书

**练习 4**: 配置基于 weight 的 Canary 部署，10% 流量到新版本

---

```yaml
---
id: LEARN-WEEK4-DAY23
title: Day 23 - Ingress 实操
topic: network-storage
type: hands-on-guide
tags: [ingress, nginx, tls, canary, routing, hands-on, k8s-1.28-1.33]
last_updated: 2026-05-18
difficulty: advanced
intent_queries:
  - "Ingress Controller 怎么安装"
  - "Ingress 路由规则怎么配"
  - "TLS 证书怎么配置"
  - "Canary 灰度怎么实现"
  - "Nginx Ingress Controller 配置教程"
trigger_keywords:
  - Ingress
  - Ingress Controller
  - nginx.ingress.kubernetes.io
  - TLS 证书
  - Let's Encrypt
  - Canary 部署
  - 灰度发布
  - 流量控制
  - 速率限制
  - Header 路由
reading_level: intermediate
audience:
  - sre
  - ops-engineer
  - developer
estimated_read_time: 45min
related_domains:
  - domain-10-troubleshooting-diagnostics
  - domain-03-networking-traffic
related_topics:
  - service
  - networking
  - ingress
  - tls
related:
  - domain-11-production-operations/topic-learn/public-training/week-4-network-storage/day-22-service-basics/01-service-basics-hands-on.md
  - domain-10-troubleshooting-diagnostics/09-ingress-troubleshooting.md
---
```

---

## 自测题 (Self-Check)

**1. ClusterIP 如何实现?**

<details><summary>答案</summary>

kube-proxy 通过 iptables/IPVS 将 ClusterIP DNAT 到后端 PodIP:TargetPort。

</details>

**2. Ingress vs Gateway API?**

<details><summary>答案</summary>

Ingress 仅 HTTP, 需注解扩展; Gateway API 支持 HTTP/gRPC/TCP, 原生流量分割, 角色分离。

</details>

**3. StatefulSet 稳定网络标识原理?**

<details><summary>答案</summary>

Pod 名 <sts>-<ordinal> + Headless Service → DNS <pod>.<svc>.<ns>.svc.cluster.local。

</details>

**4. 如何选 CNI?**

<details><summary>答案</summary>

Calico (通用 BGP/VXLAN) / Cilium (eBPF 高性能) / Flannel (简单无 Policy)。生产推荐 Cilium 或 Calico。

</details>

**5. PVC 三种访问模式?**

<details><summary>答案</summary>

ReadWriteOnce (单节点 RW) / ReadOnlyMany (多节点 RO) / ReadWriteMany (多节点 RW)。

</details>

