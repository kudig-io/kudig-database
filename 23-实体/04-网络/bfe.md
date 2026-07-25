---
title: BFE
description: '## 概述'
summary: 'BFE 是百度开源的现代化七层负载均衡器和反向代理，处理百度内部每天数万亿级别的请求。它提供高级流量路由、安全防护、可观测性等能力，支持 HTTP/HTTPS/HTTP2/QUIC 等协议，适合作为 Kubernetes [[Ingress|Ingress]] Controller 或独立的流量网关。'
category: entities
tags:
- k8s
- cncf
- networking
- bfe
- prometheus
- grafana
- ingress
- crd
- operator
tier: peripheral
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- BFE 是什么
- 如何 BFE
trigger_keywords:
- BFE
prerequisites:
- kubectl-basics
- prometheus-basics
- monitoring-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# BFE

> **CNCF 状态**: Sandbox | **类别**: Networking | **主要语言**: Go

## 概述

BFE（Baidu Front End）是百度开源的 CNCF 沙箱项目，是一个企业级的前端代理和流量接入平台。它在百度内部承担着每秒数百万请求的流量接入任务。BFE 设计目标是提供比 Nginx/HAProxy 更好的运维体验和更丰富的流量管理功能，特别在多租户隔离、流量路由和协议转换方面有独到设计。BFE 支持 HTTP/HTTPS、HTTPS、WebSocket、gRPC、SPDY 等多种协议。

## Key Features（核心能力）

- **多租户架构**：内置租户隔离机制，支持多业务线共享同一实例
- **高级流量路由**：基于请求内容的细粒度路由（Header、Cookie、Query、Path）
- **协议转换**：支持 HTTP 到 gRPC、HTTP/2 到 HTTP/1.1 的协议转换
- **多协议支持**：HTTP/HTTPS/HTTP2/WebSocket/gRPC/TCP/UDP
- **WAF 集成**：内置 Web 应用防火墙功能
- **细粒度限流**：支持基于租户、路由、请求特征的细粒度速率限制

## 架构与工作原理

BFE 采用多租户核心设计：流量首先按租户（Tenant）分类，每个租户内通过路由规则（Route）分发到不同的子集群（Subcluster）。BFE 的路由模型支持基于请求 Header、Path、Cookie、Query 参数的多条件匹配。数据平面使用 Go 语言编写，利用 goroutine 实现高并发。BFE 支持热加载配置，无需重启即可更新路由规则。

## K8s 集成

BFE 可作为 Kubernetes 的 Ingress Controller 使用。通过 CRD 定义 BFE 特有的路由和租户配置，或通过标准 Ingress 资源集成。BFE 以 Deployment 部署，前端通过 LoadBalancer Service 或 NodePort 暴露。支持自动发现 K8s Service Endpoints 作为后端，通过健康检查自动剔除不健康节点。

## 生产用例

- **企业级流量接入**：大规模互联网应用的前端接入网关
- **多租户网关**：多业务线共享同一网关实例，逻辑隔离
- **协议适配**：前端 HTTPS 到后端 gRPC 的协议转换
- **灰度发布**：基于请求特征的细粒度流量分配

## 安装与配置

```bash
# 🟢 Helm 安装
helm repo add bfe https://bfenetworks.github.io/bfe-helm-charts/
helm install bfe bfe/bfe -n bfe-system --create-namespace

# 🟢 验证安装
kubectl get pods -n bfe-system
kubectl get svc -n bfe-system

# 🟢 测试连接
curl -v http://bfe.bfe-system.svc/health

# 🟢 查看 BFE 状态 API
curl http://bfe.bfe-system.svc:8080/monitor/proxy_state
```

### 租户和路由配置

```json
{
  "Version": "1.0",
  "DefaultNext": "error_page",
  "ProductRule": {
    "example_product": {
      "Cond": "req_host_in(\"www.example.com\")",
      "BasicRule": [
        {
          "Cond": "req_path_prefix_in(\"/api\", false)",
          "ClusterName": "api_backend"
        },
        {
          "Cond": "req_path_prefix_in(\"/static\", false)",
          "ClusterName": "static_backend"
        }
      ]
    }
  }
}
```

### 集群配置

```json
{
  "Version": "1.0",
  "Config": {
    "api_backend": {
      "Backend": [
        {
          "Name": "api-1",
          "Addr": "10.0.1.10:8080",
          "Weight": 5
        },
        {
          "Name": "api-2",
          "Addr": "10.0.1.11:8080",
          "Weight": 5
        }
      ],
      "CheckConf": {
        "Schem": "http",
        "Uri": "/health",
        "Host": "api.example.com",
        "StatusCode": 200
      },
      "GslbBasic": {
        "RetryLevel": 0,
        "HashConf": {
          "HashStrategy": 0,
          "HashFactor": 0
        }
      }
    }
  }
}
```

## 运维操作

### 常用命令

```bash
# 🟢 查看 BFE Pod
kubectl get pods -n bfe-system -o wide

# 🟢 查看 BFE 日志
kubectl logs -n bfe-system -l app=bfe --tail=50

# 🟢 查看代理状态
curl http://bfe.bfe-system.svc:8080/monitor/proxy_state

# 🟢 查看连接状态
curl http://bfe.bfe-system.svc:8080/monitor/connection_state

# 🟢 查看后端集群状态
curl http://bfe.bfe-system.svc:8080/monitor/cluster_state

# 🟢 查看路由统计
curl http://bfe.bfe-system.svc:8080/monitor/route_state

# 🟡 热加载配置 (无需重启)
curl -X POST http://bfe.bfe-system.svc:8080/reload?name=route_rule

# 🟢 查看 Prometheus 指标
curl http://bfe.bfe-system.svc:8080/metrics

# 🟡 滚动重启
kubectl rollout restart deployment/bfe -n bfe-system
```

### K8s Ingress 集成

```yaml
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: bfe-ingress
  annotations:
    kubernetes.io/ingress.class: bfe
    bfe.io/product: "example_product"
spec:
  rules:
  - host: www.example.com
    http:
      paths:
      - path: /api
        pathType: Prefix
        backend:
          service:
            name: api-service
            port:
              number: 8080
      - path: /
        pathType: Prefix
        backend:
          service:
            name: web-service
            port:
              number: 80
```

## 故障排查

| 症状 | 可能原因 | 排查命令 | 修复方案 |
|------|----------|----------|----------|
| 502 Bad Gateway | 后端不可达 | `curl :8080/monitor/cluster_state` | 检查后端 Pod 和健康检查 |
| 路由不生效 | 路由规则配置错误 | `curl :8080/monitor/route_state` | 检查 Cond 表达式语法 |
| 高延迟 | 后端响应慢/连接池耗尽 | 查看 Prometheus 指标 | 优化后端/调整连接池 |
| TLS 握手失败 | 证书配置错误 | 检查 BFE 日志 | 更新证书配置 |
| 配置加载失败 | JSON 格式错误 | `kubectl logs -l app=bfe` | 修正配置文件格式 |

### 排查流程

```
1. kubectl get pods -n bfe-system → 确认 BFE Pod 状态
2. curl :8080/monitor/proxy_state → 查看代理状态
3. curl :8080/monitor/cluster_state → 查看后端状态
4. kubectl logs -l app=bfe → 查看错误日志
5. 检查路由规则和后端健康检查配置
```

## 生产案例

### 案例1: 百度内部流量接入
- **场景**: 百度内部每秒数百万请求的前端接入
- **方案**: BFE 多租户架构，按业务线隔离流量
- **效果**: 单实例处理 100万+ QPS，多业务线共享无干扰

### 案例2: 多租户 API 网关
- **场景**: SaaS 平台需要为每个租户提供独立流量策略
- **方案**: BFE 租户隔离 + 细粒度限流
- **效果**: 租户间流量完全隔离，单租户异常不影响其他

## 对比替代方案

| 维度 | BFE | Nginx | Envoy | HAProxy |
|------|-----|-------|-------|--------|
| 多租户 | 原生支持 | 不支持 | 有限 | 不支持 |
| 语言 | Go | C | C++ | C |
| 热加载 | 支持 | 支持 | xDS | 支持 |
| 协议支持 | 丰富 | 丰富 | 最丰富 | 丰富 |
| K8s Ingress | 支持 | 支持 | 支持 | 支持 |
| 社区活跃度 | 中 | 极高 | 极高 | 高 |
| 扩展性 | Go 插件 | Lua/C | Wasm/Lua | Lua |

## 检查清单

- [ ] BFE 副本数 >= 2，配置 PDB
- [ ] 健康检查配置正确
- [ ] 路由规则已测试验证
- [ ] TLS 证书已配置且自动续期
- [ ] Prometheus 指标已接入监控
- [ ] 配置了合理的限流策略
- [ ] 日志级别适当 (生产用 WARN+)
- [ ] 定期备份配置文件

## Related

- [[meshery]] — Meshery
- [[knative]] — Knative
- [[konveyor]] — Konveyor
- [[prometheus]] — Prometheus
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- bfe
- [[23-实体/15-参考与索引/cncf-networking.md|CNCF 网络与服务网格项目全景]] — Cross-reference


<!-- risk-assessed -->
