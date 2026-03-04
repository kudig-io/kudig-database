# Trickster

> **成熟度**: Sandbox | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官网** | https://trickstercache.org/ |
| **GitHub** | https://github.com/trickstercache/trickster |
| **许可证** | Apache-2.0 |
| **开发语言** | Go |
| **CNCF 状态** | Sandbox |

---

## 项目概述

Trickster 是一个 HTTP 反向代理/缓存，专为时序数据库（Prometheus, InfluxDB, ClickHouse）的 Dashboard 查询加速设计。它通过增量时间序列缓存（Delta Proxy Cache）显著减少对后端数据库的查询压力，降低 Grafana 等 Dashboard 的加载时间。

### 核心特性

- **时序感知缓存**: 仅请求缓存中缺失的时间段（Delta Proxy）
- **多后端支持**: Prometheus, InfluxDB, ClickHouse, Generic HTTP
- **Dashboard 加速**: Grafana Dashboard 加载速度提升数十倍
- **负载削减**: 大幅减少后端时序数据库的查询压力
- **Collapsing 转发**: 合并相同查询的并发请求
- **配置热更新**: 支持运行时配置重载
- **TLS 终止**: 内置 TLS 反向代理

---

## 快速开始

### 安装

```bash
# Docker 运行
docker run -d --name trickster \
  -p 8480:8480 -p 8481:8481 \
  -v $(pwd)/trickster.yaml:/etc/trickster/trickster.yaml \
  trickstercache/trickster:latest

# Helm 安装
helm repo add trickster https://helm.trickstercache.org
helm install trickster trickster/trickster \
  --namespace trickster \
  --create-namespace
```

### 配置

```yaml
# trickster.yaml
backends:
  prometheus:
    provider: prometheus
    origin_url: http://prometheus:9090
    cache_name: default
    paths:
      api/v1/query_range:
        handler: proxycache
        methods: [GET, POST]
      api/v1/query:
        handler: proxy
        methods: [GET, POST]

caches:
  default:
    provider: memory
    memory:
      max_size_bytes: 536870912  # 512MB

frontend:
  listen_port: 8480

metrics:
  listen_port: 8481
```

### Grafana 集成

```yaml
# Grafana 数据源配置 - 指向 Trickster 而非直接连接 Prometheus
apiVersion: 1
datasources:
  - name: Prometheus (Cached)
    type: prometheus
    access: proxy
    url: http://trickster:8480  # Trickster 地址
    isDefault: true
```

---

## 最佳实践

1. **Dashboard 加速**: 将 Grafana 数据源指向 Trickster，而非直接连接 Prometheus
2. **内存管理**: 根据查询模式合理配置缓存大小
3. **Collapsing**: 启用请求合并减少 Dashboard 刷新时的重复查询
4. **监控**: 监控 Trickster 自身的缓存命中率和延迟指标
5. **多后端**: 为不同的数据源配置独立的 Trickster backend

---

## 参考资源

- [Trickster 官方文档](https://trickstercache.org/docs/)
- [Trickster GitHub](https://github.com/trickstercache/trickster)
- [CNCF Sandbox Projects](https://www.cncf.io/sandbox-projects/)

---

**维护者**: Kudig Team | **许可证**: MIT
