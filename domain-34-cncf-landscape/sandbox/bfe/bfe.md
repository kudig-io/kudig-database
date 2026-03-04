# BFE (Baidu Front End)

> **成熟度**: Sandbox | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官网** | https://www.bfe-networks.net/ |
| **GitHub** | https://github.com/bfenetworks/bfe |
| **许可证** | Apache-2.0 |
| **开发语言** | Go |
| **CNCF 状态** | Sandbox |

---

## 项目概述

BFE 是百度开源的现代化七层负载均衡器和反向代理，处理百度内部每天数万亿级别的请求。它提供高级流量路由、安全防护、可观测性等能力，支持 HTTP/HTTPS/HTTP2/QUIC 等协议，适合作为 Kubernetes Ingress Controller 或独立的流量网关。

### 核心特性

- **高级路由**: 基于 Host, Path, Header, Cookie, Query 等多维度路由
- **安全防护**: 内置 WAF 规则、IP 黑白名单、频率限制
- **协议支持**: HTTP/1.x, HTTP/2, TLS, QUIC/HTTP3, WebSocket
- **流量管理**: 金丝雀发布、A/B 测试、流量镜像
- **可扩展**: 模块化架构，支持自定义插件开发
- **Kubernetes Ingress**: 作为 Ingress Controller 使用

---

## 快速开始

### 安装

```bash
# Docker 运行
docker run -d --name bfe -p 8080:8080 -p 8443:8443 -p 8421:8421 \
  bfenetworks/bfe:latest

# 从源码编译
go install github.com/bfenetworks/bfe@latest
```

### 路由配置

```json
{
  "Version": "1.0",
  "ProductRules": {
    "example_product": [
      {
        "Cond": "req_host_in(\"www.example.com\")",
        "ClusterName": "cluster_example"
      }
    ]
  }
}
```

### Kubernetes Ingress

```yaml
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: my-app
  annotations:
    kubernetes.io/ingress.class: bfe
    bfe.ingress.kubernetes.io/balance.weight: '{"backend-v1": 80, "backend-v2": 20}'
spec:
  rules:
    - host: app.example.com
      http:
        paths:
          - path: /api
            pathType: Prefix
            backend:
              service:
                name: api-service
                port:
                  number: 8080
```

---

## 最佳实践

1. **路由规则**: 使用 BFE 的条件表达式实现复杂路由逻辑
2. **安全防护**: 启用内置 WAF 模块保护后端服务
3. **QUIC/HTTP3**: 对延迟敏感的前端服务启用 QUIC 支持
4. **流量管控**: 使用权重路由实现金丝雀发布和灰度策略
5. **监控**: 利用 8421 端口的监控接口集成 Prometheus

---

## 参考资源

- [BFE 官方文档](https://www.bfe-networks.net/en_us/ABOUT.md)
- [BFE GitHub](https://github.com/bfenetworks/bfe)
- [CNCF Sandbox Projects](https://www.cncf.io/sandbox-projects/)

---

**维护者**: Kudig Team | **许可证**: MIT
