---
title: Ingress
summary: Ingress 是 Kubernetes 中管理外部访问集群内服务的 API 对象，提供 HTTP/HTTPS 路由。
category: concepts
tags:
- core-concept
- k8s
- networking
- visibility/public
tier: core
sources:
- KUDIG Stub Generation 2026-05-24
created: 2026-05-24
updated: 2026-07-11
last_updated: 2026-07
status: stable
---



# Ingress

## 概述

Ingress 是 Kubernetes 内置的七层（HTTP/HTTPS）路由 API 对象。它将集群外部的 HTTP/HTTPS 流量按主机名（host）和路径（path）路由到集群内的 Service，并集中提供 TLS 终止、虚拟主机、基于名称的转发等能力。Ingress 资源本身只是规则声明，真正的路由由 **Ingress Controller**（如 NGINX Ingress、Traefik、HAProxy、Envoy Gateway）实现。相比每个服务都开 LoadBalancer，Ingress 能以单一入口聚合大量服务，显著节省公网 IP 与云负载均衡器成本。

## 架构与工作原理

```
   Internet (HTTPS 443)
        │
        ▼
┌──────────────────────────────────┐
│ LoadBalancer (公网) → NodePort    │
└─────────────┬────────────────────┘
              │
              ▼
┌──────────────────────────────────┐
│ Ingress Controller (DaemonSet/Deployment) │
│ 监听 Ingress 资源 → 生成反向代理配置        │
└─────────────┬────────────────────┘
              │ 按 host/path 转发
   ┌──────────┼───────────┐
   ▼          ▼           ▼
 Service A  Service B  Service C
```

**工作流**：
1. 用户创建 `Ingress` 资源，声明 host、path 与后端 Service 的映射。
2. Ingress Controller 通过 Watch 机制感知变化，将其翻译为自身代理（如 nginx.conf / Envoy xDS）配置并热加载。
3. 外部流量经云负载均衡器（或 NodePort）进入 Controller，Controller 按 Ingress 规则将请求代理到对应 Service。
4. Controller 直接连后端 Pod（通过 Endpoints 获取），减少一跳 kube-proxy DNAT。

**IngressClass**：自 v1.19 起，每个 Ingress 必须通过 `ingressClassName` 关联一个 IngressClass，用以区分集群中多个 Controller（如 nginx 内网、nginx 外网）。

## 关键组件与特性

| 概念 | 说明 |
|------|------|
| Ingress 资源 | networking.k8s.io/v1，声明路由规则 |
| IngressClass | 标识由哪个 Controller 实现 |
| Ingress Controller | 实现 Ingress 规则的实际代理（NGINX/Traefik/Envoy 等） |
| TLS Secret | 存放证书 + 私钥，由 spec.tls 引用，提供 HTTPS |
| 默认后端 | spec.defaultBackend，无匹配规则时返回（常用 404） |
| 注解（annotations） | 厂商扩展点：超时、限流、rewrite、body 大小等 |

## 配置示例

```yaml
---
# 1. IngressClass（集群级，只需一次）
apiVersion: networking.k8s.io/v1
kind: IngressClass
metadata:
  name: nginx
spec:
  controller: k8s.io/ingress-nginx
---
# 2. TLS 证书 Secret（cert-manager 自动生成时可省略）
apiVersion: v1
kind: Secret
metadata:
  name: webapp-tls
  namespace: production
type: kubernetes.io/tls
data:
  tls.crt: <base64>
  tls.key: <base64>
---
# 3. Ingress 规则
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: webapp-ingress
  namespace: production
  annotations:
    nginx.ingress.kubernetes.io/ssl-redirect: "true"
    nginx.ingress.kubernetes.io/proxy-body-size: "10m"
    cert-manager.io/cluster-issuer: letsencrypt-prod
spec:
  ingressClassName: nginx
  tls:
  - hosts: [app.example.com, api.example.com]
    secretName: webapp-tls
  rules:
  - host: app.example.com
    http:
      paths:
      - path: /
        pathType: Prefix
        backend:
          service:
            name: webapp
            port:
              number: 80
  - host: api.example.com
    http:
      paths:
      - path: /v1
        pathType: Prefix
        backend:
          service:
            name: api-v1
            port:
              name: http
      - path: /v2
        pathType: Prefix
        backend:
          service:
            name: api-v2
            port:
              number: 80
```

## 常用操作与命令

```bash
# 查看 Ingress 及其解析的后端
kubectl get ingress -n production
kubectl describe ingress webapp-ingress

# 查看 Controller 日志（NGINX Ingress）
kubectl logs -n ingress-nginx -l app.kubernetes.io/name=ingress-nginx -f

# 实时观察访问日志
kubectl logs -n ingress-nginx -l app.kubernetes.io/name=ingress-nginx -f | grep app.example.com

# 测试路由（指定 Host）
curl -H "Host: api.example.com" https://<LB-IP>/v1 -k

# cert-manager 证书状态
kubectl get certificate -n production
kubectl describe certificate webapp-tls -n production
```

## 最佳实践

1. **统一入口 + 多域名**：用通配符证书或 cert-manager 自动签发，单 Controller 承载多域名。
2. **pathType 用 ImplementationSpecific 或 Prefix**：避免 `Exact` 适用面太窄。
3. **HTTPS 强制 + HSTS**：`ssl-redirect: "true"` 并在响应头加 Strict-Transport-Security。
4. **限流与 WAF**：通过注解接入 rate-limit、modsecurity，保护后端。
5. **Controller 高可用**：Deployment 设 replicas≥2 + Pod 反亲和跨节点，外层 LB 健康检查。
6. **金丝雀发布**：用 NGINX Ingress 的 canary 注解（按 header/权重分流），实现无侵入灰度。

## 常见陷阱

- **404 NotFound**：host 或 path 与请求不匹配，或 IngressClass 未指定导致 Controller 忽略。
- **503 Service Unavailable**：后端 Service 无 Endpoints（selector 错或 Pod 未就绪）。
- **证书不更新**：cert-manager 未配置或 DNS-01 challenge 失败，检查 CertificateRequest 状态。
- **annotation 不生效**：用了与 Controller 不匹配的注解前缀（如把 nginx 注解用在 traefik 上）。
- **后端获取真实客户端 IP**：默认 X-Forwarded-For 链路，需后端信任 Controller；externalTrafficPolicy 影响 LB→Controller 这一段。
- **单点故障**：Controller 只跑一个副本，挂了整站不可达，务必多副本 + 反亲和。

## 相关概念

- [[概念/kubernetes.md|Kubernetes]]
- [[概念/service.md|Service]] — Ingress 的后端
- [[概念/networkpolicy.md|NetworkPolicy]]
- [[概念/kubernetes-architecture-overview.md|Kubernetes 架构概览]]
- [[README]]

## Related

- [[visibility-public|#visibility/public Hub]] — tag hub
