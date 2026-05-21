---
title: 'Domain 98: 云原生 API 网关技术体系 (Cloud-Native API Gateway Technology Stack)'
description: 本领域专注于云原生 API 网关技术栈的深度实践，涵盖 Higress、Apache APISIX、Kong、Envoy Gateway、Traefik 等主流开源 API 网关产品。领域范围聚焦于南北向（Ingress）流量治理，包括
  Kubernetes Gateway API 标准、Wasm 插件生态、API 安全体系、可观测性集成以及生产环境最佳实践。东西向（服务网格）流量治理请参考 [D
category: cloud-native-api-gateway
tags:
- k8s
- api-gateway
- envoy
- apisix
- higress
- etcd
- prometheus
- grafana
- jaeger
- istio
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 架构师
- 运维工程师
estimated_read_time: 5min
intent_queries:
- 'Domain 98: 云原生 API 网关技术体系 (Cloud-Native API Gateway Technology Stack) 是什么'
- '如何 Domain 98: 云原生 API 网关技术体系 (Cloud-Native API Gateway Technology Stack)'
- Kubernetes 40 cloud native api gateway 最佳实践
trigger_keywords:
- Domain
- '98:'
- 云原生
- API
- 网关技术体系
- Cloud-Native
- API
- Gateway
prerequisites:
- kubectl-basics
- networking-basics
- service-mesh-basics
- prometheus-basics
- monitoring-basics
- ebpf-basics
- etcd-basics
- tls-basics
- policy-basics
- tracing-basics
- observability-basics
---

# Domain 98: 云原生 API 网关技术体系 (Cloud-Native API Gateway Technology Stack)

> **适用范围**: 云原生 API 网关、Ingress 控制器、Gateway API | **维护状态**: 持续更新中 | **专家级别**: ⭐⭐⭐⭐⭐ | **更新时间**: 2026-03-04

## 📋 领域概览

本领域专注于云原生 API 网关技术栈的深度实践，涵盖 Higress、Apache APISIX、Kong、Envoy Gateway、Traefik 等主流开源 API 网关产品。领域范围聚焦于南北向（Ingress）流量治理，包括 Kubernetes Gateway API 标准、Wasm 插件生态、API 安全体系、可观测性集成以及生产环境最佳实践。东西向（服务网格）流量治理请参考 [Domain-26: 服务网格](../domain-03-networking-traffic)。

## 📚 文档目录

### 🎯 基础理论与标准 (01-03)
- **[01-云原生API网关架构总览](./01-api-gateway-architecture-overview.md)** - API 网关 vs Ingress 控制器 vs 服务网格网关；请求生命周期；控制平面/数据平面分离；CNCF 生态定位；产品选型决策树
- **[02-Kubernetes Gateway API标准深度解析](./02-kubernetes-gateway-api-deep-dive.md)** - GatewayClass/Gateway/HTTPRoute/ReferenceGrant CRD 体系；角色模型；v1.0/v1.1 GA；一致性测试；各产品支持矩阵
- **[03-API网关选型指南与对比矩阵](./03-api-gateway-selection-guide.md)** - 12+ 维度功能矩阵；场景决策框架；TCO 分析；迁移成本评估；国内企业采用趋势

### 🌐 主流产品深度实践 (04-09)
- **[04-Higress云原生API网关企业级实践](./04-higress-enterprise-gateway.md)** - Istiod 控制平面 + Envoy 数据平面架构；xDS 配置下发原理；Mac 快速 Demo（Docker/Kind）；McpBridge 注册中心对接；Wasm 插件开发实战；AI 网关能力（LLM 代理、Token 限流、多模型 Fallback、语义缓存）；Gateway API 一致性；生产调优与故障排查；竞品横向对比
- **[05-Apache APISIX企业级API网关实践](./05-apisix-enterprise-gateway.md)** - etcd 控制平面 + OpenResty 数据平面；100+ 插件；Wasm 支持；多语言插件运行时；APISIX Ingress Controller；ADC 声明式配置
- **[06-Kong API网关企业级实践](./06-kong-enterprise-gateway.md)** - PostgreSQL/DB-less/Konnect 模式；KIC 架构；Kong Gateway Operator；deck 声明式配置；Gateway API 一致性；混合模式部署
- **[07-Envoy Gateway企业级实践](./07-envoy-gateway-enterprise.md)** - Envoy 官方项目；Gateway API 原生 API 接口；ExtensionPolicy/RateLimitPolicy CRD；EnvoyPatchPolicy 扩展性；Wasm 集成
- **[08-Traefik API网关企业级实践](./08-traefik-enterprise-gateway.md)** - Traefik v3 架构；Provider（K8s CRD/Ingress/Gateway API）；IngressRoute/Middleware CRD；TLS 自动化（ACME）；Hub API 门户
- **[09-传统Ingress控制器向云原生API网关迁移](./09-nginx-ingress-migration-guide.md)** - 功能差距分析；迁移模式（并行部署、注解映射、增量切换）；nginx-ingress 迁移到 APISIX/Higress/Kong 实战；零停机迁移清单

### 🔧 核心能力专题 (10-12)
- **[10-Wasm插件生态与开发实践](./10-wasm-plugin-ecosystem.md)** - proxy-wasm ABI 规范；产品支持矩阵；Go/Rust Wasm 插件开发；插件生命周期管理；性能开销分析；Wasm vs Lua vs 原生插件对比
- **[11-API网关安全体系：认证、鉴权与WAF](./11-api-gateway-security-practices.md)** - JWT/OIDC/mTLS/API Key 认证；OPA 集成；WAF（ModSecurity）；限流策略（令牌桶、滑动窗口、分布式限流）；Bot 检测
- **[12-API网关可观测性：指标、日志与链路追踪](./12-api-gateway-observability.md)** - 黄金信号；各产品 Prometheus 指标；结构化访问日志；OpenTelemetry/Zipkin/Jaeger 集成；Grafana 仪表盘设计；告警规则

### ⚡ 生产运维与高级主题 (13-14)
- **[13-API网关性能基准测试与调优](./13-api-gateway-performance-benchmarks.md)** - 基准测试方法论（wrk2/hey/fortio）；Higress/APISIX/Kong/Envoy Gateway/Traefik 对比；调优参数；eBPF 加速路径
- **[14-API网关生产运维最佳实践](./14-api-gateway-production-operations.md)** - HA 部署模式；滚动升级；GitOps 配置管理；证书生命周期；灾备预案；容量规划；多租户网关；AI 网关生产模式

## 🎯 学习路径建议

### 🔰 入门阶段
1. **01-架构总览** → 理解 API 网关核心概念与分类
2. **02-Gateway API** → 掌握 Kubernetes 标准网关接口
3. **03-选型指南** → 根据场景选择合适产品

### ⭐ 进阶阶段
1. **04~08-产品深度实践** → 根据团队技术栈选择对应产品深入学习
2. **10-Wasm 插件** → 掌握现代网关插件开发模式
3. **09-迁移指南** → 从传统 Ingress 迁移到云原生 API 网关

### 🔒 专家阶段
1. **11-安全体系** → 构建企业级 API 安全边界
2. **12-可观测性** → 建立网关层全链路可观测体系
3. **13-性能调优** → 基准测试与生产环境性能优化
4. **14-生产运维** → 大规模网关集群运维最佳实践

## 📊 技术深度对比

| 文档 | 技术深度 | 实践价值 | 适用场景 | 复杂度 |
|------|----------|----------|----------|--------|
| 01-架构总览 | ⭐⭐⭐⭐ | 高 | 技术选型、架构设计 | 中 |
| 02-Gateway API | ⭐⭐⭐⭐⭐ | 很高 | 标准化接入、多产品对接 | 中高 |
| 03-选型指南 | ⭐⭐⭐ | 很高 | 技术决策、方案评估 | 低 |
| 04-Higress | ⭐⭐⭐⭐⭐ | 很高 | AI 网关、阿里云生态 | 中高 |
| 05-APISIX | ⭐⭐⭐⭐⭐ | 很高 | 高性能 API 管理 | 中高 |
| 06-Kong | ⭐⭐⭐⭐⭐ | 很高 | 企业 API 平台 | 中高 |
| 07-Envoy Gateway | ⭐⭐⭐⭐⭐ | 高 | Gateway API 原生体验 | 中 |
| 08-Traefik | ⭐⭐⭐⭐ | 高 | 轻量级入口、自动化 TLS | 中 |
| 09-迁移指南 | ⭐⭐⭐⭐ | 很高 | 存量系统升级 | 中高 |
| 10-Wasm 插件 | ⭐⭐⭐⭐⭐ | 高 | 插件开发、扩展定制 | 高 |
| 11-安全体系 | ⭐⭐⭐⭐⭐ | 很高 | 零信任 API 安全 | 高 |
| 12-可观测性 | ⭐⭐⭐⭐ | 很高 | 监控运维、故障定位 | 中 |
| 13-性能基准 | ⭐⭐⭐⭐⭐ | 高 | 性能优化、容量规划 | 高 |
| 14-生产运维 | ⭐⭐⭐⭐⭐ | 很高 | 大规模生产环境 | 高 |

## 🔧 核心技术栈

```bash
# 主流 API 网关产品
Higress (CNCF Sandbox)              # 阿里云开源，AI 网关能力
Apache APISIX (Apache TLP)          # 高性能，100+ 插件
Kong (Kong Inc.)                    # 企业级 API 平台
Envoy Gateway (CNCF/Envoy)         # Gateway API 原生实现
Traefik (Traefik Labs)             # 轻量级，自动化 TLS

# 标准与规范
Kubernetes Gateway API v1.1         # 统一网关标准接口
proxy-wasm ABI                      # Wasm 插件标准接口

# 插件运行时
Wasm (TinyGo/Rust/AssemblyScript)  # 跨语言安全沙箱
Lua (OpenResty/LuaJIT)             # APISIX/Kong 原生
Go/Java Plugin Runner              # 多语言插件运行时

# 安全组件
OPA (Open Policy Agent)            # 策略引擎
ModSecurity                        # WAF 引擎
cert-manager                       # 证书自动化

# 可观测性
OpenTelemetry                      # 统一遥测框架
Prometheus + Grafana               # 指标监控
```

## 📚 相关领域链接

- **[Domain-5: 网络](../domain-03-networking-traffic)** - Kubernetes 网络架构、Ingress 基础、Gateway API 概览
- **[Domain-26: 服务网格](../domain-03-networking-traffic)** - Istio/Linkerd 等东西向流量治理
- **[Domain-8: 可观测性](../domain-06-observability)** - 监控告警体系基础
- **[Domain-25: 云原生安全](../domain-05-security-compliance)** - 安全架构基础
- **[Domain-34: CNCF Landscape](../domain-19-landscape-references)** - CNCF 开源项目全景
- **[Domain-35: eBPF 技术](../domain-35-ebpf-technology)** - eBPF 加速数据路径

---
*本文档由云原生技术专家团队维护，内容基于 2026 年云原生 API 网关生态最新实践。*

## Related

- [[README]]
