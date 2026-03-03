# Envoy

> **成熟度**: Graduated | **加入时间**: 2017-09 | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官方网站** | https://www.envoyproxy.io |
| **GitHub** | https://github.com/envoyproxy/envoy |
| **文档** | https://www.envoyproxy.io/docs |
| **许可证** | Apache-2.0 |
| **主要语言** | C++ |
| **CNCF 分类** | Service Mesh |

---

## 项目概述

### 简介
Envoy 是一个高性能的开源边缘和服务代理，专为云原生应用设计，由 Lyft 开发并开源。

### 核心定位
Envoy 作为现代化的 L7 代理和通信总线，解决了微服务架构中的服务发现、负载均衡、流量管理、可观测性等核心网络问题，是众多服务网格解决方案的数据平面基础。

### 发展历程
- **2016-09**: Lyft 开源 Envoy
- **2017-09**: 加入 CNCF 作为孵化项目
- **2018-11**: 成为 CNCF 毕业项目
- **2024**: Envoy v1.29+ 持续演进

---

## 核心功能

### 主要特性
- **L3/L4 代理**: TCP/UDP 代理，支持 TLS 终止
- **L7 代理**: HTTP/2、gRPC、WebSocket 支持
- **服务发现**: 支持多种服务发现机制
- **负载均衡**: 多种负载均衡算法
- **健康检查**: 主动和被动健康检查
- **可观测性**: 丰富的统计、日志、追踪支持
- **动态配置**: xDS API 动态配置更新

### 功能架构
```
┌─────────────────────────────────────────────────────────────┐
│                        Envoy Proxy                          │
│  ┌──────────────────────────────────────────────────────── ┐│
│  │                    Listener                             ││
│  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────────┐ ││
│  │  │   Filter    │  │   Filter    │  │   Filter        │ ││
│  │  │   Chain     │  │   Chain     │  │   Chain         │ ││
│  │  └─────────────┘  └─────────────┘  └─────────────────┘ ││
│  └─────────────────────────────────────────────────────────┘│
│  ┌──────────────────────────────────────────────────────── ┐│
│  │                    Cluster Manager                      ││
│  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────────┐ ││
│  │  │  Cluster A  │  │  Cluster B  │  │  Cluster C      │ ││
│  │  │ (Endpoints) │  │ (Endpoints) │  │  (Endpoints)    │ ││
│  │  └─────────────┘  └─────────────┘  └─────────────────┘ ││
│  └─────────────────────────────────────────────────────────┘│
└─────────────────────────────────────────────────────────────┘
```

---

## 技术架构

### 整体架构
Envoy 采用单进程多线程架构，通过非阻塞事件循环处理连接，支持热重启实现零停机配置更新。

### 核心组件
| 组件 | 功能 | 说明 |
|:---|:---|:---|
| Listener | 监听器 | 监听下游连接，配置过滤器链 |
| Filter | 过滤器 | 处理请求/响应的可插拔模块 |
| Cluster | 集群 | 上游服务端点的逻辑分组 |
| Endpoint | 端点 | 上游服务的网络地址 |
| xDS | 动态配置 | 通过 API 动态更新配置 |

### 工作原理
1. Listener 监听下游客户端连接
2. 请求经过 Filter Chain 处理（认证、限流、路由等）
3. Router Filter 根据路由规则选择目标 Cluster
4. 负载均衡器从 Cluster 中选择 Endpoint
5. 将请求转发到上游服务并返回响应

---

## 使用场景

### 典型应用
- **API 网关**: 作为边缘代理处理入站流量
- **服务网格 Sidecar**: 作为服务间通信的数据平面
- **负载均衡器**: 替代传统硬件/软件负载均衡
- **前端代理**: 为后端服务提供统一入口

### 适用条件
- 需要高性能 L7 代理
- 微服务架构的服务间通信
- 需要高级流量管理能力
- 需要丰富的可观测性数据

### 不适用场景
- 简单的四层负载均衡（可用更轻量方案）
- 资源极度受限的环境
- 不需要高级 L7 功能的场景

---

## 快速开始

### 安装部署
```bash
# Docker 运行
docker run -d --name envoy -p 10000:10000 envoyproxy/envoy:v1.29-latest

# 二进制安装
wget https://github.com/envoyproxy/envoy/releases/download/v1.29.0/envoy-x86_64
chmod +x envoy-x86_64
./envoy-x86_64 -c envoy.yaml
```

### 基础配置
```yaml
# envoy.yaml
static_resources:
  listeners:
  - name: listener_0
    address:
      socket_address:
        address: 0.0.0.0
        port_value: 10000
    filter_chains:
    - filters:
      - name: envoy.filters.network.http_connection_manager
        typed_config:
          "@type": type.googleapis.com/envoy.extensions.filters.network.http_connection_manager.v3.HttpConnectionManager
          stat_prefix: ingress_http
          route_config:
            name: local_route
            virtual_hosts:
            - name: local_service
              domains: ["*"]
              routes:
              - match:
                  prefix: "/"
                route:
                  cluster: service_backend
          http_filters:
          - name: envoy.filters.http.router
            typed_config:
              "@type": type.googleapis.com/envoy.extensions.filters.http.router.v3.Router

  clusters:
  - name: service_backend
    connect_timeout: 30s
    type: LOGICAL_DNS
    lb_policy: ROUND_ROBIN
    load_assignment:
      cluster_name: service_backend
      endpoints:
      - lb_endpoints:
        - endpoint:
            address:
              socket_address:
                address: backend.example.com
                port_value: 80
```

### 验证测试
```bash
# 测试代理
curl http://localhost:10000

# 查看统计信息
curl http://localhost:9901/stats

# 查看配置
curl http://localhost:9901/config_dump
```

---

## 最佳实践

### 生产环境建议
- 启用 TLS 终止和 mTLS
- 配置合理的超时和重试策略
- 使用动态配置（xDS）而非静态配置
- 启用访问日志和追踪

### 性能优化
- 合理配置连接池大小
- 启用 HTTP/2 和连接复用
- 使用合适的负载均衡策略
- 配置断路器防止级联故障

### 安全加固
- 启用 mTLS 服务间加密
- 配置 RBAC 访问控制
- 使用 JWT 认证
- 限制管理端口访问

---

## 生态集成

### 相关 CNCF 项目
- **Istio**: 使用 Envoy 作为数据平面
- **Contour**: Envoy 的 Kubernetes Ingress 控制器
- **Emissary-Ingress**: 基于 Envoy 的 API 网关
- **OpenTelemetry**: 可观测性数据导出

### 常见集成方案
- Istio + Envoy 服务网格
- Contour + Envoy Kubernetes Ingress
- Envoy + Jaeger 分布式追踪
- Envoy + Prometheus 指标监控

---

## 社区与支持

### 社区资源
- Slack: https://envoyproxy.slack.com
- 邮件列表: envoy-users@googlegroups.com
- Twitter: @EnvoyProxy

### 贡献指南
访问 https://www.envoyproxy.io/docs/envoy/latest/start/contribution 了解参与方式

---

## 参考资源

- [官方文档](https://www.envoyproxy.io/docs)
- [GitHub Repo](https://github.com/envoyproxy/envoy)
- [CNCF 项目页面](https://www.cncf.io/projects/envoy/)
- [Envoy 博客](https://blog.envoyproxy.io/)

---

**维护者**: Kudig Team | **许可证**: MIT
