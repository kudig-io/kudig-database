# Kubernetes Ingress 全栈进阶培训 (从入门到专家)

> **适用版本**: Kubernetes v1.28 - v1.32 | **文档类型**: 全栈技术实战指南
> **目标受众**: 初级运维、流量治理专家、SRE
> **核心原则**: 掌握七层网关基础、实现精细化流量管控、构建高性能接入体系

---

## 🔰 第一阶段：快速入门与核心概念

### 1.1 什么是 Ingress？
*   **场景**: Service (NodePort) 在暴露多个服务时需要管理大量端口且缺乏域名路由能力。
*   **功能**: Ingress 是集群的 **HTTP/HTTPS 统一入口**。它提供基于 **域名 (Host)** 和 **路径 (Path)** 的负载均衡。
*   **注意**: Ingress 资源本身是“规则”，需要 **Ingress Controller** (如 Nginx) 来实现这些规则。

### 1.2 基础配置示例
```yaml
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: minimal-ingress
spec:
  rules:
  - host: myapp.example.com
    http:
      paths:
      - path: /api
        pathType: Prefix
        backend:
          service:
            name: api-service
            port:
              number: 80
```

---

## 📘 第二阶段：核心架构与深度原理 (Deep Dive)

### 2.1 Ingress 控制器工作原理
*   **动态更新**: 监听 API Server -> 触发配置重载。
*   **优化**: Nginx Ingress 通过 Lua 动态修改 Shared Memory 中的 Upstream，实现无 Reload 的 Pod 更新。

### 2.2 流量路径全追踪
*   **路径**: 外部 LB -> Ingress Controller Pod (HostNetwork/NodePort) -> Backend Service -> Pod。

---

## ⚡ 第三阶段：生产部署与高可用架构

### 2.1 企业级高可用方案
*   **独占节点**: 为 Ingress Controller 分配专用节点，隔离 TLS 握手带来的 CPU 压力。
*   **HPA**: 结合自定义指标（如并发请求数）进行动态扩缩。

### 2.2 证书管理 (TLS)
*   **Cert-Manager**: 联动实现证书自动续签。
*   **Secret**: 将证书存储在 Kubernetes Secret 中并在 Ingress 中引用。

---

## 📈 第四阶段：性能调优与可观测性 (Performance)

### 3.1 极致调优
*   **长连接**: 开启 `upstream-keepalive`。
*   **缓冲区**: 调大 `proxy_buffer_size` 处理超大 Header。
*   **内核**: 优化 `net.core.somaxconn` 提升挂起连接处理能力。

---

## 🛠️ 第五阶段：复杂排障与 SRE 运维 (SRE Ops)

### 4.1 监控指标
| 指标 | 含义 | 专家关注点 |
| :--- | :--- | :--- |
| `nginx_ingress_controller_response_duration_seconds` | 响应时延 (P99) | 识别长尾请求 |
| `nginx_ingress_controller_requests{status=~"5.."}` | 5xx 错误率 | 核心业务报警指标 |

### 🏆 SRE 运维红线
*   *红线 1: 生产环境严禁在无 HPA 的情况下运行 Ingress 控制器。*
*   *红线 2: 任何 Ingress 配置变更必须经过语法校验。*
*   *红线 3: 必须监控正则匹配逻辑，防止 CPU 正则回溯攻击。*
