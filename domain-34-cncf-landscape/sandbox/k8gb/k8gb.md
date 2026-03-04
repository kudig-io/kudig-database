# K8GB (Kubernetes Global Balancer)

> **成熟度**: Sandbox | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官网** | https://k8gb.io/ |
| **GitHub** | https://github.com/k8gb-io/k8gb |
| **许可证** | Apache-2.0 |
| **开发语言** | Go |
| **CNCF 状态** | Sandbox |

---

## 项目概述

K8GB 是一个 Kubernetes 原生的全局负载均衡解决方案，基于 DNS 实现跨集群的流量调度。它使用 Kubernetes CRD (GslbStrategy) 定义全局负载均衡策略，通过 CoreDNS 和 ExternalDNS 实现多集群间的 DNS 基础的流量管理，支持轮询、地理位置和故障转移策略。

### 核心特性

- **Kubernetes 原生**: 基于 CRD 的声明式全局负载均衡配置
- **DNS 流量管理**: 基于 DNS 的跨集群流量调度
- **多种策略**: 轮询、加权轮询、地理位置感知、故障转移
- **健康检查**: 自动检测集群/服务健康状态并切换流量
- **无单点**: 去中心化设计，每个集群独立运行 K8GB 实例
- **标准集成**: 与 Ingress Controller 和 ExternalDNS 无缝集成

---

## 架构设计

```
         User DNS Query
              │
              ▼
    ┌─────────────────┐
    │   Delegated DNS  │
    │   (NS Records)   │
    └────────┬────────┘
             │
    ┌────────┴────────┐
    │                 │
    ▼                 ▼
┌─────────┐    ┌─────────┐
│Cluster A│    │Cluster B│
│         │    │         │
│ K8GB    │◄──►│ K8GB    │  (CoreDNS Cross-sync)
│ CoreDNS │    │ CoreDNS │
│ ExtDNS  │    │ ExtDNS  │
│         │    │         │
│ Ingress │    │ Ingress │
│ App     │    │ App     │
└─────────┘    └─────────┘
```

---

## 快速开始

### 安装

```bash
helm repo add k8gb https://k8gb.io/
helm install k8gb k8gb/k8gb \
  --namespace k8gb \
  --create-namespace \
  --set k8gb.dnsZone="example.com" \
  --set k8gb.edgeDNSZone="gslb.example.com" \
  --set k8gb.clusterGeoTag="us-east-1"
```

### 定义全局负载均衡

```yaml
apiVersion: k8gb.absa.oss/v1beta1
kind: Gslb
metadata:
  name: my-app-gslb
  namespace: default
spec:
  ingress:
    ingressClassName: nginx
    rules:
      - host: app.gslb.example.com
        http:
          paths:
            - path: /
              pathType: Prefix
              backend:
                service:
                  name: my-app
                  port:
                    number: 80
  strategy:
    type: roundRobin  # 或 failover, geoip
    splitBrainThresholdSeconds: 300
    dnsTtlSeconds: 30
```

### 故障转移策略

```yaml
spec:
  strategy:
    type: failover
    primaryGeoTag: "us-east-1"  # 主集群
```

### 地理位置策略

```yaml
spec:
  strategy:
    type: geoip
    dnsTtlSeconds: 30
```

---

## 最佳实践

1. **DNS 委托**: 正确配置 DNS 委托，将 gslb 子域委托给 K8GB 管理的 CoreDNS
2. **TTL 配置**: 设置合理的 DNS TTL，平衡故障切换速度和 DNS 缓存效率
3. **健康检查**: 确保后端服务有适当的健康检查端点
4. **脑裂保护**: 配置 splitBrainThresholdSeconds 防止网络分区导致的脑裂
5. **地理标签**: 为每个集群配置准确的地理标签（geoTag）

---

## 参考资源

- [K8GB 官方文档](https://k8gb.io/docs/)
- [K8GB GitHub](https://github.com/k8gb-io/k8gb)
- [CNCF Sandbox Projects](https://www.cncf.io/sandbox-projects/)

---

**维护者**: Kudig Team | **许可证**: MIT
