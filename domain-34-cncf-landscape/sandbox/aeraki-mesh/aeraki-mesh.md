# Aeraki Mesh

> **成熟度**: Sandbox | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官网** | https://www.aeraki.net/ |
| **GitHub** | https://github.com/aeraki-mesh/aeraki |
| **许可证** | Apache-2.0 |
| **开发语言** | Go |
| **CNCF 状态** | Sandbox |

---

## 项目概述

Aeraki Mesh 是 Istio 服务网格的扩展框架，专注于为非 HTTP 协议提供流量管理能力。在微服务架构中，除了 HTTP/gRPC 之外，还广泛使用 Dubbo、Thrift、Redis、Kafka 等协议。Aeraki Mesh 通过扩展 Istio 的数据面（Envoy）和控制面，使这些非 HTTP 协议也能享受服务网格的流量路由、负载均衡、熔断限流和可观测性能力。

### 核心特性

- **多协议支持**: 原生支持 Dubbo、Thrift、Redis、Kafka 等非 HTTP 协议的流量管理
- **MetaProtocol 框架**: 提供通用的七层协议扩展框架，快速适配新协议
- **流量路由**: 支持基于协议头（如 Dubbo Service、Thrift Method）的智能路由
- **负载均衡**: 为非 HTTP 协议提供多种负载均衡策略（轮询、随机、一致性哈希）
- **限流熔断**: 协议级别的限流、熔断和重试策略
- **可观测性**: 自动收集非 HTTP 协议的指标（请求量、延迟、错误率）

---

## 架构设计

```
┌──────────────────────────────────────────────────────┐
│                  Istio Control Plane                  │
│                                                       │
│  ┌──────────┐     ┌─────────────────────────────┐    │
│  │  Istiod   │◄───│     Aeraki Controller        │    │
│  │ (Pilot)   │    │  (协议识别/xDS 配置生成)     │    │
│  └─────┬─────┘    └──────────────┬──────────────┘    │
└────────┼─────────────────────────┼───────────────────┘
         │  xDS                    │  Aeraki xDS
    ┌────▼─────────────────────────▼────────────┐
    │              Envoy Sidecar                  │
    │                                             │
    │  ┌─────────────┐  ┌──────────────────────┐ │
    │  │ HTTP Filter  │  │  MetaProtocol Proxy  │ │
    │  │ (Istio 原生) │  │  ┌────────────────┐  │ │
    │  │              │  │  │ Dubbo Codec    │  │ │
    │  │              │  │  │ Thrift Codec   │  │ │
    │  │              │  │  │ Redis Codec    │  │ │
    │  │              │  │  │ Kafka Codec    │  │ │
    │  │              │  │  │ Custom Codec   │  │ │
    │  │              │  │  └────────────────┘  │ │
    │  └─────────────┘  └──────────────────────┘ │
    └────────────────────────────────────────────┘
         │                        │
    ┌────▼────┐            ┌──────▼──────┐
    │ HTTP    │            │ Dubbo/Thrift │
    │ Service │            │ Redis/Kafka  │
    └─────────┘            └─────────────┘
```

---

## 快速开始

### 安装

```bash
# 前提: 已安装 Istio
# 安装 Aeraki Mesh
git clone https://github.com/aeraki-mesh/aeraki.git
cd aeraki

# 使用 Helm 安装
helm install aeraki chart/aeraki \
  --namespace istio-system \
  --set aeraki.istiodAddr=istiod.istio-system:15010

# 安装 MetaProtocol Proxy (Envoy 扩展)
kubectl apply -f https://raw.githubusercontent.com/aeraki-mesh/aeraki/master/demo/metaprotocol-dubbo.yaml
```

### Dubbo 流量路由示例

```yaml
# dubbo-routing.yaml
apiVersion: metaprotocol.aeraki.io/v1alpha1
kind: MetaRouter
metadata:
  name: dubbo-route
  namespace: dubbo-demo
spec:
  hosts:
    - dubbo-provider.dubbo-demo.svc.cluster.local
  routes:
    - name: v1-route
      match:
        attributes:
          dubbo_service_interface:
            exact: com.example.UserService
          dubbo_service_method:
            exact: getUser
      route:
        - destination:
            host: dubbo-provider.dubbo-demo.svc.cluster.local
            subset: v1
          weight: 80
        - destination:
            host: dubbo-provider.dubbo-demo.svc.cluster.local
            subset: v2
          weight: 20
```

### Redis 流量管理

```yaml
# redis-routing.yaml
apiVersion: metaprotocol.aeraki.io/v1alpha1
kind: MetaRouter
metadata:
  name: redis-route
  namespace: redis-demo
spec:
  hosts:
    - redis.redis-demo.svc.cluster.local
  routes:
    - name: read-route
      match:
        attributes:
          redis_cmd:
            prefix: GET
      route:
        - destination:
            host: redis-replica.redis-demo.svc.cluster.local
    - name: write-route
      route:
        - destination:
            host: redis-primary.redis-demo.svc.cluster.local
```

---

## 高级功能

### MetaProtocol 自定义协议扩展

```go
// 实现自定义协议 Codec
type MyProtocolCodec struct{}

func (c *MyProtocolCodec) Decode(buf []byte) (*MetaData, error) {
    // 解析自定义协议头
    metadata := &MetaData{
        Headers: map[string]string{
            "service": parseService(buf),
            "method":  parseMethod(buf),
        },
    }
    return metadata, nil
}
```

### 限流配置

```yaml
apiVersion: metaprotocol.aeraki.io/v1alpha1
kind: MetaRouter
metadata:
  name: dubbo-rate-limit
spec:
  hosts:
    - dubbo-provider.dubbo-demo.svc.cluster.local
  globalRateLimit:
    rateLimitService: rate-limit.istio-system.svc.cluster.local
    descriptors:
      - property: dubbo_service_interface
        descriptorKey: service
    requestTimeout: 100ms
  localRateLimit:
    tokenBucket:
      fillInterval: 60s
      maxTokens: 1000
      tokensPerFill: 1000
```

### 可观测性指标

```yaml
# Aeraki 自动为 Dubbo/Thrift 协议暴露以下指标:
# - aeraki_meta_protocol_request_total (请求总数)
# - aeraki_meta_protocol_request_duration_milliseconds (请求延迟)
# - aeraki_meta_protocol_request_bytes_total (请求字节数)
# - aeraki_meta_protocol_response_bytes_total (响应字节数)

# Prometheus 采集规则
- job_name: aeraki-metrics
  kubernetes_sd_configs:
    - role: pod
  relabel_configs:
    - source_labels: [__meta_kubernetes_pod_annotation_prometheus_io_scrape]
      action: keep
      regex: true
```

---

## 与其他方案对比

| 特性 | Aeraki Mesh | Istio 原生 | Linkerd | MOSN |
|:---|:---|:---|:---|:---|
| HTTP 流量管理 | 支持 (Istio) | 完整 | 完整 | 支持 |
| Dubbo 支持 | 原生七层 | 仅四层 | 不支持 | 支持 |
| Thrift 支持 | 原生七层 | 仅四层 | 不支持 | 支持 |
| Redis 流量管理 | 支持读写分离 | 不支持 | 不支持 | 有限 |
| 协议扩展框架 | MetaProtocol | 无 | 无 | 有限 |
| Istio 兼容性 | 完全兼容 | 原生 | 独立 | 可替换 |

---

## 最佳实践

1. **协议识别**: 确保 Service 端口命名遵循 Istio 协议识别规范 (如 `tcp-dubbo`)
2. **版本灰度**: 使用 MetaRouter 进行 Dubbo 版本灰度发布，结合权重控制流量比例
3. **Redis 读写分离**: 利用 Redis 协议解析能力实现自动读写分离
4. **指标采集**: 启用 Aeraki 协议指标，配合 Prometheus + Grafana 监控非 HTTP 服务
5. **渐进扩展**: 先用 MetaProtocol 管理核心协议，再逐步扩展到更多自定义协议

---

## 参考资源

- [Aeraki Mesh 官方文档](https://www.aeraki.net/docs/)
- [Aeraki Mesh GitHub](https://github.com/aeraki-mesh/aeraki)
- [MetaProtocol Proxy](https://github.com/aeraki-mesh/meta-protocol-proxy)
- [Istio 官方文档](https://istio.io/latest/docs/)
- [CNCF Sandbox Projects](https://www.cncf.io/sandbox-projects/)

---

**维护者**: Kudig Team | **许可证**: MIT
