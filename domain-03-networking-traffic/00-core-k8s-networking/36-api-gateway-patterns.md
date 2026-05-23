---
title: 38 - Ingress和API Gateway对比表
description: '| **Traefik** | Traefik | 动态配置，中间件 | v1.25+ | 中-高 | 低 | - |'
category: networking
tags:
- k8s
- networking
- service
- ingress
- cni
- istio
- envoy
- hpa
- daemonset
- gateway
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 网络工程师
- 运维工程师
estimated_read_time: 5min
intent_queries:
- Ingress和API Gateway对比表 是什么
- 如何 Ingress和API Gateway对比表
- Kubernetes 5 networking 最佳实践
trigger_keywords:
- Ingress和API
- Gateway对比表
- networking
prerequisites:
- kubectl-basics
- networking-basics
- service-mesh-basics
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
  path: ../domain-03-networking-traffic/
  label: '相关知识域: domain-03-networking-traffic'
- type: domain
  path: ../domain-06-observability/
  label: '相关知识域: domain-06-observability'
- type: cheatsheet
  path: ../domain-17-system-foundation/topic-cheat-sheet/networking.md
  label: '速查卡: networking'
created: "2026-05-23"
---

# 38 - Ingress和API Gateway对比表

> **适用版本**: v1.25 - v1.32 | **最后更新**: 2026-02 | **参考**: [[entities/kubernetes|kubernetes]].io/docs/concepts/services-networking/ingress-controllers](https://kubernetes.io/docs/concepts/services-networking/ingress-controllers/)

<!-- chunk: Ingress控制器对比 -->
## Ingress控制器对比

| 控制器 | 代理类型 | 关键特性 | K8S版本 | 性能(QPS) | 学习曲线 | ACK支持 |
|-------|---------|---------|--------|-----------|---------|---------|
| **Nginx [[Ingress|Ingress]]** | Nginx | 成熟稳定，功能全面 | v1.25+ | 高 | 中 | 支持 |
| **[[Envoy|Envoy]] Gateway** | Envoy | **官方 Gateway API 实现** | v1.25+ | 很高 | 中 | - |
| **Traefik** | Traefik | 动态配置，中间件 | v1.25+ | 中-高 | 低 | - |
| **Kong** | Nginx/Kong | API网关功能 | v1.25+ | 高 | 中-高 | - |
| **HAProxy** | HAProxy | 高性能，企业级 | v1.25+ | 很高 | 中 | - |
| **Contour** | Envoy | 现代架构，HTTPProxy | v1.25+ | 高 | 中 | - |
| **Ambassador/Emissary** | Envoy | API网关，开发者友好 | v1.25+ | 高 | 中 | - |
| **ALB Ingress** | 阿里云ALB | 云原生，免运维 | v1.25+ | 很高 | 低 | 原生 |
| **Istio Gateway** | Envoy | 服务网格集成 | v1.25+ | 高 | 高 | ASM |

<!-- chunk: 功能对比矩阵 -->
## 功能对比矩阵

| 功能 | Nginx | Traefik | Kong | Contour | ALB | Gateway API |
|-----|-------|---------|------|---------|-----|-------------|
| **TLS终止** | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ |
| **路径重写** | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ |
| **流量分割** | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ |
| **限流** | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ |
| **WAF** | 插件 | 插件 | ✓ | - | ✓ | - |
| **JWT验证** | 插件 | 中间件 | ✓ | - | - | - |
| **gRPC** | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ |
| **WebSocket** | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ |
| **TCP/UDP** | ✓ | ✓ | ✓ | ✓ | ✓ | v1.31+ |
| **金丝雀** | 注解 | ✓ | ✓ | ✓ | ✓ | ✓ |
| **跨NS路由** | 有限 | ✓ | ✓ | ✓ | ✓ | ✓ |

<!-- chunk: Nginx Ingress配置 -->
## Nginx Ingress配置

```yaml
# Nginx Ingress示例
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: app-ingress
  annotations:
    kubernetes.io/ingress.class: nginx
    nginx.ingress.kubernetes.io/rewrite-target: /$2
    nginx.ingress.kubernetes.io/ssl-redirect: "true"
    nginx.ingress.kubernetes.io/proxy-body-size: "50m"
    nginx.ingress.kubernetes.io/proxy-read-timeout: "3600"
    nginx.ingress.kubernetes.io/proxy-send-timeout: "3600"
    # 限流
    nginx.ingress.kubernetes.io/limit-rps: "100"
    nginx.ingress.kubernetes.io/limit-connections: "10"
    # 金丝雀
    nginx.ingress.kubernetes.io/canary: "true"
    nginx.ingress.kubernetes.io/canary-weight: "10"
spec:
  ingressClassName: nginx
  tls:
  - hosts:
    - example.com
    secretName: tls-secret
  rules:
  - host: example.com
    http:
      paths:
      - path: /api(/|$)(.*)
        pathType: Prefix
        backend:
          service:
            name: api-service
            port:
              number: 80
      - path: /
        pathType: Prefix
        backend:
          service:
            name: frontend-service
            port:
              number: 80
```

<!-- chunk: Gateway API配置 -->
## Gateway API配置

```yaml
# Gateway API (v1.31+稳定)
apiVersion: gateway.networking.k8s.io/v1
kind: Gateway
metadata:
  name: main-gateway
spec:
  gatewayClassName: nginx  # 或其他实现
  listeners:
  - name: http
    port: 80
    protocol: HTTP
    allowedRoutes:
      namespaces:
        from: All
  - name: https
    port: 443
    protocol: HTTPS
    tls:
      mode: Terminate
      certificateRefs:
      - name: tls-secret
    allowedRoutes:
      namespaces:
        from: All
---
apiVersion: gateway.networking.k8s.io/v1
kind: HTTPRoute
metadata:
  name: app-route
spec:
  parentRefs:
  - name: main-gateway
  hostnames:
  - "example.com"
  rules:
  - matches:
    - path:
        type: PathPrefix
        value: /api
    backendRefs:
    - name: api-service
      port: 80
      weight: 90
    - name: api-service-canary
      port: 80
      weight: 10
  - matches:
    - path:
        type: PathPrefix
        value: /
    backendRefs:
    - name: frontend-service
      port: 80
```

<!-- chunk: ALB Ingress配置(ACK) -->
## ALB Ingress配置(ACK)

```yaml
# ALB Ingress
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: alb-ingress
  annotations:
    kubernetes.io/ingress.class: alb
    alb.ingress.kubernetes.io/address-type: internet
    alb.ingress.kubernetes.io/vswitch-ids: vsw-xxx
    alb.ingress.kubernetes.io/ssl-redirect: "true"
    alb.ingress.kubernetes.io/healthcheck-path: /health
    alb.ingress.kubernetes.io/healthcheck-interval-seconds: "5"
    # 金丝雀
    alb.ingress.kubernetes.io/canary: "true"
    alb.ingress.kubernetes.io/canary-by-header: "X-Canary"
    # 限流
    alb.ingress.kubernetes.io/traffic-limit-qps: "1000"
spec:
  tls:
  - hosts:
    - example.com
    secretName: tls-secret
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

<!-- chunk: 性能基准 -->
## 性能基准

| 控制器 | 并发连接 | QPS(HTTP) | 延迟(P99) | 内存使用 | CPU使用 |
|-------|---------|-----------|----------|---------|--------|
| **Nginx Ingress** | 10,000+ | 50,000+ | <10ms | 中 | 中 |
| **Traefik** | 10,000+ | 30,000+ | <15ms | 低 | 低 |
| **HAProxy** | 50,000+ | 100,000+ | <5ms | 低 | 低 |
| **Envoy(Contour)** | 10,000+ | 50,000+ | <10ms | 中 | 中 |
| **ALB** | 100,000+ | 1,000,000+ | <5ms | - | - |

<!-- chunk: 高可用部署 -->
## 高可用部署

| 控制器 | 部署方式 | HA配置 | 会话亲和 |
|-------|---------|-------|---------|
| **Nginx Ingress** | Deployment+HPA | 多副本+LB | 支持 |
| **Traefik** | Deployment+HPA | 多副本+LB | 支持 |
| **Contour** | Deployment+DaemonSet | 多副本 | 支持 |
| **ALB** | 托管 | 自动 | 支持 |

```yaml
# Nginx Ingress HA配置
apiVersion: apps/v1
kind: Deployment
metadata:
  name: nginx-ingress-controller
spec:
  replicas: 3
  selector:
    matchLabels:
      app: nginx-ingress
  template:
    spec:
      affinity:
        podAntiAffinity:
          requiredDuringSchedulingIgnoredDuringExecution:
          - labelSelector:
              matchLabels:
                app: nginx-ingress
            topologyKey: kubernetes.io/hostname
---
apiVersion: autoscaling/v2
kind: HorizontalPodAutoscaler
metadata:
  name: nginx-ingress-hpa
spec:
  scaleTargetRef:
    apiVersion: apps/v1
    kind: Deployment
    name: nginx-ingress-controller
  minReplicas: 3
  maxReplicas: 10
  metrics:
  - type: Resource
    resource:
      name: cpu
      target:
        type: Utilization
        averageUtilization: 70
```

<!-- chunk: 常见问题排查 -->
## 常见问题排查

| 问题 | 症状 | 诊断 | 解决方案 |
|-----|------|------|---------|
| **502 Bad Gateway** | 后端不可达 | 检查Service/Endpoints | 修复后端服务 |
| **503 Service Unavailable** | 无可用后端 | 检查Pod状态 | 扩容/修复Pod |
| **504 Gateway Timeout** | 后端超时 | 检查后端响应 | 调整超时配置 |
| **证书错误** | HTTPS失败 | 检查Secret/证书 | 更新证书 |
| **路由不生效** | 请求404 | 检查path配置 | 修正路径匹配 |

```bash
# Ingress排查命令
kubectl get ingress -A
kubectl describe ingress <name>
kubectl get pods -n ingress-nginx  # 检查控制器
kubectl logs -n ingress-nginx <pod>  # 查看日志
kubectl exec -n ingress-nginx <pod> -- cat /etc/nginx/nginx.conf  # 检查配置
```

---

**Ingress原则**: 选择适合场景，配置HA，启用监控

---

**表格底部标记**: Kusheet Project, 作者 Allen Galler (allengaller@gmail.com)

---

<!-- chunk: Obsidian 相关文档 -->
## Obsidian 相关文档

- domain-03-networking-traffic MOC
- [[domain-03-networking-traffic/README|Domain 5: Networking 网络]]
- Kubernetes 网络基础 Network in a Nutshell
- Domain-5 网络 — 开源项目索引
- FAQ 文档
- 网络核心组件
- CNI 架构与核心原理
- 76 - CNI插件深度对比
- 142 - Flannel 完整指南 (Flannel Complete Guide)
- Flannel WireGuard 加密后端配置
- Flannel IPv6 Dual Stack 支持
- Flannel Windows 节点支持

## See Also

- 34-network-performance-tuning
- 35-gateway-api-overview
- 37-terway-resources-crud-operations
- 38-terway-gc-mechanism
