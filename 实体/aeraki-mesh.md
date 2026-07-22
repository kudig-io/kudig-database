---
title: Aeraki Mesh [entities]
description: '## 概述'
summary: 'Aeraki Mesh 是 Istio 服务网格的扩展框架，专注于为非 HTTP 协议提供流量管理能力。在微服务架构中，除了 HTTP/gRPC 之外，还广泛使用 Dubbo、Thrift、Redis、Kafka 等协议。Aeraki Mesh 通过扩展 Istio 的数据面（Envoy）和控制面，'
category: entities
tags:
- k8s
- cncf
- networking
- aeraki-mesh
- prometheus
- grafana
- istio
- envoy
- redis
- kafka
tier: peripheral
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Aeraki Mesh 是什么
- 如何 Aeraki Mesh
trigger_keywords:
- Aeraki
- Mesh
prerequisites:
- kubectl-basics
- service-mesh-basics
- prometheus-basics
- monitoring-basics
- kafka-basics
- redis-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Aeraki Mesh

> **CNCF 状态**: Sandbox | **类别**: Networking | **主要语言**: Go

## 概述

Aeraki Mesh 是由美团开源的 Istio 服务网格扩展框架，2021 年加入 CNCF Sandbox。它专注于为非 HTTP 协议提供流量管理能力。在微服务架构中，除了 HTTP/gRPC 之外，还广泛使用 Dubbo、Thrift、Redis、Kafka 等协议。Aeraki Mesh 通过扩展 Istio 的数据面（Envoy）和控制面，使这些非 HTTP 协议也能享受服务网格的流量路由、负载均衡、熔断限流和可观测性能力。

## 核心特性

- **多协议管理**: Dubbo、Thrift、Redis、Kafka、RocketMQ、Zookeeper 等协议
- **MetaProtocol**: 通用协议扩展框架，支持自定义协议
- **MetaRouter CRD**: 类似 VirtualService 的协议级路由规则
- **Istio 集成**: 与 Istio 控制平面无缝集成，共享 mTLS 和可观测性
- **Redis 读写分离**: 自动解析 Redis 协议实现读写路由
- **Dubbo 灰度**: 基于 Dubbo 服务名的版本路由和流量比例控制

## 架构

Aeraki Mesh 在 Istio 架构上增加了两个组件。Aeraki（控制面扩展）作为 Istio 的翻译器，监听 MetaRouter CRD 和 Istio Service Entry，将非 HTTP 协议的治理规则翻译为 Envoy 过滤器链配置，通过 xDS 下发。数据面上，Aeraki 为 Envoy 注入 MetaProtocol Proxy 或专用协议 Filter（如 Dubbo Proxy、Redis Proxy），在 L7 解析协议元数据（方法名、服务名、参数）进行路由决策。Aeraki 也支持 RDS（Route Discovery Service）动态下发路由规则。

## Kubernetes 集成

Aeraki Mesh 作为 Istio 的扩展部署。它监听 Kubernetes API 中的 MetaRouter CRD 和 Service Entry，通过 Istio 的 Sidecar 注入机制安装 Envoy 扩展 Filter。`Service` 端口命名（如 `tcp-dubbo`、`tcp-redis`）触发 Aeraki 应用对应协议的 Filter。MetaRouter CRD 与 VirtualService 并行工作，VirtualService 管 HTTP，MetaRouter 管非 HTTP。与 Istio 的 mTLS、AuthorizationPolicy 等安全机制完全兼容。

## 生产使用场景

1. **Dubbo 微服务网格**: 将 Java Dubbo 服务纳入网格管理，实现灰度发布和流量控制
2. **Redis 读写分离**: 自动将读请求路由到 Replica，写请求路由到 Master
3. **Kafka 流量管理**: 对 Kafka 消息流量进行限流和监控
4. **Thrift 服务治理**: 为 PHP/Thrift 服务提供熔断和超时能力

## 安装与配置

```bash
# 前置: Istio 已安装
istioctl install --set profile=default -y
# 安装 Aeraki Mesh
kubectl apply -f https://raw.githubusercontent.com/aeraki-mesh/aeraki/main/deploy/aeraki.yaml
# 验证部署
kubectl get pods -n istio-system | grep aeraki
kubectl get crd | grep metaprotocol
```

```yaml
# Dubbo 路由规则 (MetaRouter)
apiVersion: metaprotocol.aeraki.io/v1beta1
kind: MetaRouter
metadata:
  name: dubbo-router
  namespace: default
spec:
  hosts:
    - "org.apache.dubbo.demo.DemoService..*..*"
  routes:
  - name: v1-route
    match:
      metadata:
        method:
          exact: "sayHello"
    route:
      cluster: outbound|20880||dubbo-demo-provider.default.svc.cluster.local
      subset: v1
  - name: v2-canary
    match:
      metadata:
        method:
          exact: "sayHello"
      app: "canary"
    route:
      cluster: outbound|20880||dubbo-demo-provider.default.svc.cluster.local
      subset: v2
---
# Redis 读写分离
apiVersion: metaprotocol.aeraki.io/v1beta1
kind: MetaRouter
metadata:
  name: redis-route
spec:
  hosts:
    - "redis.default.svc.cluster.local"
  routes:
  - name: read-replica
    match:
      metadata:
        command:
          exact: "GET"
    route:
      cluster: outbound|6379||redis-replica.default.svc.cluster.local
  - name: write-master
    route:
      cluster: outbound|6379||redis-master.default.svc.cluster.local
```

```bash
# Service 端口命名触发协议识别
kubectl label svc dubbo-demo-provider app=dubbo-demo
kubectl annotate svc dubbo-demo-provider aeraki.io/protocol=dubbo
# 或端口命名: tcp-dubbo, tcp-redis, tcp-thrift, tcp-kafka
```

## 运维操作

```bash
# 🟢 查看 Aeraki 控制面状态
kubectl get pods -n istio-system -l app=aeraki
kubectl logs -n istio-system -l app=aeraki --tail=50

# 🟢 查看 MetaRouter 规则
kubectl get metarouters -A
kubectl describe metarouter <name>

# 🟢 检查 Envoy Filter 是否生成
kubectl get envoyfilters -A | grep aeraki
istioctl proxy-config filter <pod> | grep meta_protocol

# 🟡 更新路由规则
kubectl apply -f metarouter.yaml
# 验证规则下发
istioctl proxy-config route <pod> | grep <service>

# 🟡 重启 Aeraki 控制面
kubectl rollout restart deployment/aeraki -n istio-system

# 🔴 卸载 Aeraki（影响所有非 HTTP 协议治理）
kubectl delete -f https://raw.githubusercontent.com/aeraki-mesh/aeraki/main/deploy/aeraki.yaml
kubectl delete crd metarouters.metaprotocol.aeraki.io
```

## 故障排查

| 症状 | 可能原因 | 排查命令 | 修复方案 |
|------|----------|----------|----------|
| MetaRouter 不生效 | 端口命名/注解不正确 | `kubectl get svc <name> -o yaml` | 确保端口名为 tcp-dubbo 或添加 aeraki.io/protocol 注解 |
| Envoy Filter 未生成 | Aeraki 未监听到 CRD | `kubectl logs -n istio-system -l app=aeraki` | 检查 Aeraki Pod 状态和 RBAC |
| Dubbo 路由失败 | 服务名不匹配 | `istioctl proxy-config cluster <pod>` | 核对 MetaRouter hosts 与实际服务名 |
| Redis 读写分离失效 | 命令解析失败 | `kubectl logs <envoy-sidecar>` | 确认 Redis 协议版本兼容 |
| 与 Istio 升级冲突 | Envoy Filter 版本不兼容 | `istioctl analyze` | 升级 Aeraki 至匹配 Istio 版本 |

```
排查流程：
├─ 路由不生效
│  ├─ 检查 Service 端口命名/注解是否触发协议识别
│  ├─ 检查 MetaRouter hosts 是否匹配
│  └─ istioctl proxy-config 确认 Filter 已下发
├─ 协议解析失败
│  ├─ 检查 Envoy 日志中的协议错误
│  ├─ 确认协议版本与 Aeraki 支持范围
│  └─ 检查 mTLS 是否影响协议解析
└─ 控制面异常
   ├─ Aeraki Pod 日志检查
   └─ 确认与 Istio 版本兼容性
```

## 生产案例

### 案例 1：大型 Java 微服务 Dubbo 网格化

- **场景**: 200+ Dubbo 微服务需要纳入服务网格，实现灰度发布和流量控制
- **排查**: 传统 Spring Cloud 方案需要修改代码引入 SDK，改造成本巨大
- **方案**: 部署 Aeraki Mesh，通过 MetaRouter 实现 Dubbo 方法级路由和流量比例控制
- **效果**: 零代码改造实现 Dubbo 服务网格化，灰度发布时间从小时级降至分钟级

### 案例 2：Redis 读写分离自动化

- **场景**: 应用直连 Redis Master，读压力大导致写延迟升高
- **排查**: 应用代码中读写未分离，修改代码涉及多个团队
- **方案**: Aeraki Redis Proxy 自动解析命令，GET 路由到 Replica，SET 路由到 Master
- **效果**: 零代码修改实现读写分离，Master 负载降低 60%

## 替代方案对比

| 维度 | Aeraki Mesh | Istio 原生 | Spring Cloud | Envoy Filter 手动 |
|------|-------------|------------|--------------|------------------|
| 协议支持 | Dubbo/Thrift/Redis/Kafka | HTTP/gRPC | Java 生态 | 任意 |
| 侵入性 | 零侵入(Sidecar) | 零侵入 | 需 SDK | 零侵入 |
| 路由粒度 | 方法级 | 路径级 | 方法级 | 自定义 |
| 维护成本 | 低(CRD) | 低 | 中 | 极高 |
| 适用场景 | 非 HTTP 协议治理 | HTTP 服务 | Java 微服务 | 特殊需求 |

## 架构定位

在 CNCF 生态中，Aeraki Mesh 属于 **Networking / Service Mesh** 类别，是 Istio 在非 HTTP 协议治理方面的重要补充。它解决了传统服务网格仅覆盖 HTTP 的局限性。

## 参考链接

- [[实体/prometheus-grafana.md|prometheus-grafana]]
- [[istio]]
- [[概念/controller-pattern.md|controller-pattern]]
- [[pod-lifecycle]]

## Related

- [[grpc]] — gRPC
- [[istio]] — Istio
- [[envoy]] — Envoy
- [[prometheus]] — Prometheus
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- aeraki-mesh
- [[实体/cncf-networking.md|CNCF 网络与服务网格项目全景]] — Cross-reference
- [[生态参考/领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
