---
title: Istio
description: 'description: ''## 项目概述'''
category: general
tags:
- cncf
- ecosystem
- prometheus
- grafana
- jaeger
- istio
- envoy
- ingress
- gateway
- crd
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Istio 是什么
- 如何 Istio
- Kubernetes 19 landscape references 最佳实践
trigger_keywords:
- Istio
- landscape
- references
prerequisites:
- kubectl-basics
- cncf-ecosystem
- helm-basics
- service-mesh-basics
- prometheus-basics
- monitoring-basics
- tls-basics
- tracing-basics
---

title: Istio
description: '## 项目概述'
category: cncf-landscape
tags:
- k8s
- cncf
- cloud-native
- ecosystem
- prometheus
- grafana
- jaeger
- istio
- envoy
- ingress
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 架构师
- 技术决策者
- SRE
estimated_read_time: 5min
intent_queries:
- Istio 是什么
- 如何 Istio
- Kubernetes 34 cncf landscape 最佳实践
trigger_keywords:
- Istio
- cncf
- landscape
cross_refs:
- type: fta
  path: ../domain-10-troubleshooting-diagnostics/topic-fta/list/service-mesh-istio-fta.md
  label: '故障树: service-mesh-istio'
authors:
- name: KUDIG Team
  role: contributor
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
---
# Istio

> **成熟度**: Graduated | **加入时间**: 2022-09 | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官方网站** | https://istio.io |
| **GitHub** | https://github.com/istio/istio |
| **文档** | https://istio.io/latest/docs |
| **许可证** | Apache-2.0 |
| **主要语言** | Go |
| **CNCF 分类** | Service Mesh |

---

## 项目概述

### 简介
Istio 是一个开源的服务网格平台，为微服务提供流量管理、安全、可观测性等功能，无需修改应用代码。

### 核心定位
Istio 通过 Sidecar 代理模式，在服务间通信层面提供统一的流量控制、安全策略和遥测数据收集能力，是企业级服务网格的主流选择。

### 发展历程
- **2017-05**: Google、IBM、Lyft 联合发布 Istio
- **2018-07**: Istio 1.0 发布
- **2022-09**: 加入 CNCF 作为孵化项目
- **2023-07**: 成为 CNCF 毕业项目
- **2024**: Istio 1.21+ 持续演进

---

## 核心功能

### 主要特性
- **流量管理**: 智能路由、流量镜像、金丝雀发布
- **安全**: mTLS 加密、认证授权、证书管理
- **可观测性**: 指标、日志、分布式追踪
- **策略执行**: 访问控制、速率限制、配额管理
- **扩展性**: WebAssembly 插件支持

### 功能架构
```
┌─────────────────────────────────────────────────────────────┐
│                      Control Plane                          │
│  ┌──────────────────────────────────────────────────────── ┐│
│  │                       istiod                            ││
│  │  ┌─────────┐ ┌─────────┐ ┌─────────┐ ┌─────────────┐   ││
│  │  │  Pilot  │ │ Citadel │ │  Galley │ │   Mixer     │   ││
│  │  │(Traffic)│ │(Security)│ │(Config) │ │ (Telemetry) │   ││
│  │  └─────────┘ └─────────┘ └─────────┘ └─────────────┘   ││
│  └─────────────────────────────────────────────────────────┘│
└─────────────────────────────────────────────────────────────┘
                              │ xDS
                              ▼
┌─────────────────────────────────────────────────────────────┐
│                      Data Plane                             │
│  ┌─────────────────┐  ┌─────────────────┐                  │
│  │     Service A   │  │     Service B   │                  │
│  │  ┌───────────┐  │  │  ┌───────────┐  │                  │
│  │  │  Envoy    │◄─┼──┼─►│  Envoy    │  │                  │
│  │  │  Sidecar  │  │  │  │  Sidecar  │  │                  │
│  │  └───────────┘  │  │  └───────────┘  │                  │
│  └─────────────────┘  └─────────────────┘                  │
└─────────────────────────────────────────────────────────────┘
```

---

## 技术架构

### 整体架构
Istio 采用控制平面和数据平面分离的架构。控制平面（istiod）管理配置和证书，数据平面（Envoy Sidecar）处理实际流量。

### 核心组件
| 组件 | 功能 | 说明 |
|:---|:---|:---|
| istiod | 控制平面 | 统一的控制平面组件 |
| Envoy | 数据平面代理 | Sidecar 代理，处理服务间通信 |
| Ingress Gateway | 入口网关 | 处理集群入站流量 |
| Egress Gateway | 出口网关 | 处理集群出站流量 |

### 工作原理
1. istiod 监听 Kubernetes 资源和 Istio CRD
2. 生成 Envoy 配置并通过 xDS 下发
3. Sidecar 注入器自动为 Pod 注入 Envoy
4. Envoy 代理拦截所有进出 Pod 的流量
5. 根据配置执行流量管理、安全和遥测

---

## 使用场景

### 典型应用
- **微服务治理**: 流量管理、熔断、重试
- **零信任安全**: mTLS 服务间加密
- **金丝雀发布**: 基于权重的流量分割
- **可观测性**: 统一的指标、日志、追踪

### 适用条件
- 微服务架构的 Kubernetes 集群
- 需要统一的服务间安全策略
- 需要高级流量管理能力
- 需要丰富的可观测性数据

### 不适用场景
- 简单应用或单体架构
- 资源受限的环境
- 不需要 L7 功能的场景

---

## 快速开始

### 安装部署
```bash
# 下载 istioctl
curl -L https://istio.io/downloadIstio | sh -

# 安装 Istio
istioctl install --set profile=demo -y

# 启用命名空间自动注入
kubectl label namespace default istio-injection=enabled
```

### 基础配置
```yaml
# VirtualService 示例
apiVersion: networking.istio.io/v1beta1
kind: VirtualService
metadata:
  name: reviews
spec:
  hosts:
  - reviews
  http:
  - match:
    - headers:
        end-user:
          exact: jason
    route:
    - destination:
        host: reviews
        subset: v2
  - route:
    - destination:
        host: reviews
        subset: v1

# DestinationRule 示例
apiVersion: networking.istio.io/v1beta1
kind: DestinationRule
metadata:
  name: reviews
spec:
  host: reviews
  trafficPolicy:
    connectionPool:
      tcp:
        maxConnections: 100
  subsets:
  - name: v1
    labels:
      version: v1
  - name: v2
    labels:
      version: v2
```

### 验证测试
```bash
# 部署示例应用
kubectl apply -f samples/bookinfo/platform/kube/bookinfo.yaml

# 验证服务
kubectl get pods
istioctl analyze

# 访问应用
kubectl apply -f samples/bookinfo/networking/bookinfo-gateway.yaml
```

---

## 最佳实践

### 生产环境建议
- 使用生产配置 profile
- 配置资源限制
- 启用 mTLS STRICT 模式
- 监控 istiod 和 Envoy 资源使用

### 性能优化
- 合理配置 Sidecar 资源
- 使用 Sidecar CRD 限制配置范围
- 启用 Envoy 访问日志采样
- 优化 xDS 更新频率

### 安全加固
- 启用 PeerAuthentication STRICT 模式
- 配置 AuthorizationPolicy 细粒度访问控制
- 定期轮换证书
- 限制 istiod 权限

---

## 生态集成

### 相关 CNCF 项目
- **Envoy**: 数据平面代理
- **Prometheus**: 指标收集
- **Jaeger**: 分布式追踪
- **Kiali**: 服务网格可视化

### 常见集成方案
- Istio + Prometheus + Grafana 监控
- Istio + Jaeger 追踪
- Istio + Kiali 可视化
- Istio + Cert-manager 证书管理

---

## 社区与支持

### 社区资源
- Slack: https://slack.istio.io
- 论坛: https://discuss.istio.io
- Twitter: @IstioMesh

### 贡献指南
访问 https://github.com/istio/istio/blob/master/CONTRIBUTING.md 了解参与方式

---

## 参考资源

- [官方文档](https://istio.io/latest/docs)
- [GitHub Repo](https://github.com/istio/istio)
- [CNCF 项目页面](https://www.cncf.io/projects/istio/)
- [Istio 博客](https://istio.io/latest/blog/)

---

**维护者**: Kudig Team | **许可证**: MIT

## Related

- [[domain-19-landscape-references/04-cncf-fta-index.md|04-cncf-fta-index]]
- [[log.md|log]]
- [[CONTRIBUTING.md|CONTRIBUTING]]
- [[domain-17-system-foundation/topic-cheat-sheet/go.md|go]]
- [[domain-17-system-foundation/topic-cheat-sheet/k8s.md|k8s]]
- [[references/release-notes-networking|发布说明索引 — 网络]] — Cross-reference
- [[references/k8s-platform-extensions|平台运维与扩展生态：Helm、CI/CD、Operator 开发与服务网格]] — Cross-reference
- [[references/multi-cloud-terms|K8s 多云架构术语参考]] — Cross-reference
- [[concepts/service-mesh-architecture|Service Mesh Architecture]] — Cross-reference
- [[concepts/service-mesh-evolution|服务网格演进]] — Cross-reference
- [[concepts/microservice-resilience-patterns|Microservice Resilience Patterns]] — Cross-reference
- [[concepts/bp-security|最佳实践：Security]] — Cross-reference
- [[skills/learn-05-ingress-basics|第五课：Ingress - 外部 HTTP/HTTPS 访问]] — Cross-reference
- [[skills/service-mesh-istio-fta|Service Mesh(Istio) 异常故障树分析]] — Cross-reference
- [[skills/k8s-network-security-guide|Kubernetes 网络安全最佳实践]] — Cross-reference
- [[skills/ts-cloud-provider|云服务商集成排查]] — Cross-reference
- [[skills/deployment-canary-and-bluegreen|金丝雀与蓝绿发布]] — Cross-reference
- [[entities/cncf-networking|CNCF 网络与服务网格项目全景]] — Cross-reference
- [[entities/cncf-security|CNCF 安全与合规项目全景]] — Cross-reference
- [[entities/ecosystem-changelog|生态组件变更日志索引]] — Cross-reference
- [[domain-19-landscape-references/topic-index/service-mesh-index|Service Mesh 服务网格知识图谱索引]]
- [[domain-19-landscape-references/topic-index/network-index|Network 网络知识图谱索引]]
- [[domain-19-landscape-references/topic-index/higress-index|Higress 知识图谱索引]]
- [[domain-19-landscape-references/topic-release-notes/networking/istio/RELEASE-NOTES-1.9|istio v1.9 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/networking/istio/RELEASE-NOTES-1.28|istio v1.28 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/networking/istio/RELEASE-NOTES-0.8|istio v0.8 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/networking/istio/RELEASE-NOTES-1.18|istio v1.18 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/networking/istio/RELEASE-NOTES-1.19|istio v1.19 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/networking/istio/RELEASE-NOTES-1.8|istio v1.8 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/networking/istio/RELEASE-NOTES-1.29|istio v1.29 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/networking/istio/RELEASE-NOTES-1.16|istio v1.16 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/networking/istio/RELEASE-NOTES-1.22|istio v1.22 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/networking/istio/RELEASE-NOTES-1.3|istio v1.3 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/networking/istio/RELEASE-NOTES-0.2|istio v0.2 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/networking/istio/RELEASE-NOTES-1.26|istio v1.26 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/networking/istio/RELEASE-NOTES-1.7|istio v1.7 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/networking/istio/RELEASE-NOTES-1.12|istio v1.12 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/networking/istio/RELEASE-NOTES-0.6|istio v0.6 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/networking/istio/RELEASE-NOTES-1.27|istio v1.27 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/networking/istio/RELEASE-NOTES-1.6|istio v1.6 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/networking/istio/RELEASE-NOTES-1.13|istio v1.13 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/networking/istio/RELEASE-NOTES-0.7|istio v0.7 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/networking/istio/RELEASE-NOTES-1.17|istio v1.17 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/networking/istio/RELEASE-NOTES-1.23|istio v1.23 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/networking/istio/RELEASE-NOTES-0.3|istio v0.3 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/networking/istio/RELEASE-NOTES-1.5|istio v1.5 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/networking/istio/RELEASE-NOTES-1.24|istio v1.24 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/networking/istio/RELEASE-NOTES-1.10|istio v1.10 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/networking/istio/RELEASE-NOTES-0.4|istio v0.4 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/networking/istio/RELEASE-NOTES-1.14|istio v1.14 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/networking/istio/RELEASE-NOTES-1.20|istio v1.20 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/networking/istio/RELEASE-NOTES-1.15|istio v1.15 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/networking/istio/RELEASE-NOTES-1.0|istio v1.0 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/networking/istio/RELEASE-NOTES-1.21|istio v1.21 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/networking/istio/RELEASE-NOTES-0.1|istio v0.1 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/networking/istio/RELEASE-NOTES-1.4|istio v1.4 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/networking/istio/RELEASE-NOTES-1.25|istio v1.25 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/networking/istio/RELEASE-NOTES-1.11|istio v1.11 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/networking/istio/RELEASE-NOTES-0.5|istio v0.5 Release Notes]]

## See Also

- [[domain-19-landscape-references/graduated/istio/02-istio-advanced-traffic-management.md|02-istio-advanced-traffic-management]]
- [[domain-19-landscape-references/graduated/istio/03-istio-security-hardening.md|03-istio-security-hardening]]
- [[domain-19-landscape-references/graduated/istio/02-istio-advanced-traffic-management.md|02-istio-advanced-traffic-management]]
- [[domain-19-landscape-references/graduated/istio/03-istio-security-hardening.md|03-istio-security-hardening]]
