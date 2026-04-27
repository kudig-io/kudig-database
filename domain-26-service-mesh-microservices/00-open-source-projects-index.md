# Domain-26 服务网格与微服务 — 开源项目索引

> **最后更新**: 2026-04-24  
> **适用版本**: Istio v1.29 / Linkerd v2.18 / Cilium v1.17

---

## 📋 目录

- [一、核心项目总览](#一核心项目总览)
- [二、Istio (CNCF Graduated)](#二istio-cncf-graduated)
- [三、Linkerd (CNCF Graduated)](#三linkerd-cncf-graduated)
- [四、Cilium Service Mesh](#四cilium-service-mesh)
- [五、Envoy 与网关生态](#五envoy-与网关生态)
- [六、Dapr 分布式运行时](#六dapr-分布式运行时)
- [七、其他服务网格](#七其他服务网格)
- [八、版本兼容矩阵](#八版本兼容矩阵)
- [九、服务网格选型指南](#九服务网格选型指南)

---

## 一、核心项目总览

| 项目 | 作用 | CNCF 状态 | 最新版本 | Stars | License |
|:---|:---|:---|:---|:---|:---|
| **Istio** | 服务网格 | Graduated | v1.29.0 | 36k+ | Apache-2.0 |
| **Linkerd** | 轻量级服务网格 | Graduated | v2.18.0 | 10k+ | Apache-2.0 |
| **Cilium** | eBPF 网络+服务网格 | Graduated | v1.17.0 | 21k+ | Apache-2.0 |
| **Envoy** | L7 代理与网关 | Graduated | v1.33.0 | 25k+ | Apache-2.0 |
| **Dapr** | 分布式应用运行时 | Graduated | v1.15.0 | 25k+ | Apache-2.0 |
| **Kuma** | Envoy 服务网格 | Kong | v2.10.0 | 3k+ | Apache-2.0 |
| **Consul Connect** | HashiCorp 服务网格 | HashiCorp | v1.20.0 | 28k+ | BSL/Apache-2.0 |
| **Gateway API** | K8s 新一代流量管理 | K8s SIG | v1.2.0 | - | Apache-2.0 |
| **Emissary-Ingress** | API 网关 | Incubating | v3.10.0 | 4.5k+ | Apache-2.0 |
| **Contour** | Envoy Ingress | Incubating | v1.30.0 | 3.5k+ | Apache-2.0 |

---

## 二、Istio (CNCF Graduated)

### 2.1 核心架构

```yaml
# 数据平面
- Envoy Proxy (Sidecar 或 Ambient 模式)

# 控制平面
- istiod: Pilot + Citadel + Galley 合并
  - 服务发现与配置下发
  - 证书管理与轮换
  - 验证与转换
```

### 2.2 关键特性

| 特性 | 说明 |
|:---|:---|
| 流量管理 | 虚拟服务、目标规则、流量分割、超时重试 |
| 安全 | mTLS (自动/强制)、授权策略、JWT 验证 |
| 可观测性 | 自动指标、分布式追踪、访问日志 |
| 多集群 | 单网络/多网络多集群、外部控制平面 |
| VM 扩展 | 将非 K8s 工作负载纳入网格 |
| Ambient Mesh | 无 Sidecar 模式 (ztunnel + waypoints) |

### 2.3 Ambient Mesh (无 Sidecar 模式)

- **ztunnel**: 每个节点的 L4 代理，处理 mTLS 和路由
- **waypoint proxy**: 按需 L7 代理，处理策略和可观测性
- **优势**: 更低资源占用、更快启动、更简单运维
- **状态**: Istio v1.29 中 Ambient 模式功能完善，生产可用

### 2.4 版本支持

| 版本 | 发布日期 | 支持终止 | K8s 兼容 |
|:---|:---|:---|:---|
| v1.29 | 2026.02 | ~2026.08 | 1.31-1.35 |
| v1.28 | 2025.11 | ~2026.05 | 1.30-1.34 |
| v1.27 | 2025.08 | 2026.04 | 1.29-1.33 |
| v1.26 | 2025.05 | 2025.12 | 1.29-1.33 |

**GitHub**: https://github.com/istio/istio
**文档**: https://istio.io/latest/docs/

---

## 三、Linkerd (CNCF Graduated)

### 3.1 核心哲学

```yaml
# 设计原则
- 极简主义 (最小配置、最快安装)
- 性能优先 (Rust 编写数据平面)
- 安全默认 (自动 mTLS)
- 可观测性内置 (无需额外配置)
```

### 3.2 架构组件

| 组件 | 作用 |
|:---|:---|
| proxy (Rust) | 超轻量级 sidecar |
| destination controller | 服务发现 |
| identity controller | 证书签发 (基于 trust anchor) |

### 3.3 与 Istio 对比

| 维度 | Istio | Linkerd |
|:---|:---|:---|
| 资源占用 | 较高 (Envoy) | 极低 (Rust proxy) |
| 功能丰富度 | 极高 | 核心功能覆盖 |
| 学习曲线 | 陡峭 | 平缓 |
| 多集群 | 成熟 | 基础支持 |
| VM 扩展 | 成熟 | 有限 |
| Ambient/Sidecar-less | ✅ Ambient | ❌ Sidecar only |
| 社区规模 | 极大 | 大 |
| 适用场景 | 大型企业复杂场景 | 中小型团队、性能敏感 |

**GitHub**: https://github.com/linkerd/linkerd2
**文档**: https://linkerd.io/2/overview/

---

## 四、Cilium Service Mesh

### 4.1 eBPF 原生服务网格

```yaml
# 核心特性
- 内核级 eBPF 实现 (无需 Sidecar)
- 兼容 Istio API (VirtualService, DestinationRule)
- 基于 Envoy 的 L7 代理 (仅需要时启用)
- 与 Cilium 网络策略统一
- 高性能 (无 iptables 开销)
```

### 4.2 三种服务模式

| 模式 | 描述 | 性能 |
|:---|:---|:---|
| LoadBalancer + Network Policy | L4 负载均衡 + 安全策略 | 最优 |
| Envoy Extension (per-node) | 节点级 L7 代理 | 优秀 |
| Sidecar (per-pod) | 传统 Sidecar 模式 | 标准 |

**GitHub**: https://github.com/cilium/cilium
**文档**: https://docs.cilium.io/en/stable/service-mesh/

---

## 五、Envoy 与网关生态

### 5.1 Envoy (CNCF Graduated)

- 云原生边缘和服务代理标准
- L3/L4/L7 代理
- 动态配置 (xDS API)
- 高级负载均衡、熔断、重试
- WASM 扩展

**GitHub**: https://github.com/envoyproxy/envoy

### 5.2 Gateway API (K8s SIG)

- **v1.2 GA**: 正式取代 Ingress 的长期标准
- 资源模型: GatewayClass → Gateway → HTTPRoute/TCPRoute/GRPCRoute
- 多租户共享网关、跨命名空间路由
- 实现: Istio, Cilium, Envoy Gateway, NGINX, Kong, Traefik 等

### 5.3 网关实现对比

| 项目 | 基于 | 特点 | CNCF |
|:---|:---|:---|:---|
| Emissary-Ingress | Envoy | 声明式 API 网关 | Incubating |
| Contour | Envoy | Heptio/VMware 背景 | Incubating |
| Envoy Gateway | Envoy | 官方 Envoy K8s 集成 | 非 CNCF |
| Kong Gateway | NGINX/OpenResty | API 管理丰富 | 非 CNCF |
| Traefik | Go 原生 | 云原生友好 | 非 CNCF |

---

## 六、Dapr 分布式运行时

### 6.1 定位差异

> Dapr 不是传统服务网格，而是**分布式应用运行时**，关注应用级构建块而非网络层。

```yaml
# 构建块 (Building Blocks)
- Service-to-service invocation (mTLS + 重试)
- State management (多种后端)
- Pub/sub messaging (多种 broker)
- Bindings (外部系统触发)
- Actors (虚拟 Actor 模型)
- Observability (自动追踪与指标)
- Configuration (动态配置)
- Secrets (密钥管理)
- Distributed lock (分布式锁)
```

### 6.2 与服务网格关系

| 维度 | Dapr | 服务网格 (Istio/Linkerd) |
|:---|:---|:---|
| 抽象层 | 应用层 (HTTP/gRPC SDK) | 网络层 (透明代理) |
| 服务发现 | 通过 Dapr sidecar | 通过数据平面 |
| 可观测性 | 应用级指标与追踪 | 网络级指标与追踪 |
| 状态管理 | 内置多种后端 | 不涉及 |
| 消息传递 | 内置 pub/sub | 不涉及 |
| 可以共存 | ✅ 推荐与 Istio 一起使用 | ✅ |

**GitHub**: https://github.com/dapr/dapr
**文档**: https://docs.dapr.io/

---

## 七、其他服务网格

### 7.1 Kuma (Kong)

- Envoy 数据平面
- 多区域部署 (Universal 和 K8s 模式)
- 与 Kong Gateway 集成
- CNCF 未托管，但 Kong 活跃维护

### 7.2 Consul Connect

- HashiCorp 生态集成
- 与 Consul 服务发现天然结合
- 支持 K8s 和 VM
- License: BSL (商业源码许可)

---

## 八、版本兼容矩阵

| 组件 | K8s v1.31 | v1.32 | v1.33 | 备注 |
|:---|:---|:---|:---|:---|
| Istio v1.29 | ✅ | ✅ | ✅ | Ambient GA |
| Linkerd v2.18 | ✅ | ✅ | ✅ | 稳定版 |
| Cilium v1.17 | ✅ | ✅ | ✅ | Service Mesh + Network |
| Envoy v1.33 | ✅ | ✅ | ✅ | 独立代理 |
| Dapr v1.15 | ✅ | ✅ | ✅ | Graduated |
| Gateway API v1.2 | ✅ | ✅ | ✅ | K8s 原生 |
| Kuma v2.10 | ✅ | ✅ | ✅ | Kong 生态 |

---

## 九、服务网格选型指南

```
┌─────────────────────────────────────────────────────────────┐
│                  服务网格选型决策树                            │
└─────────────────────────────────────────────────────────────┘

1. 完全不想用 Sidecar?
   └─ Yes ──► Cilium Service Mesh (eBPF) 或 Istio Ambient
   └─ No  ──► 继续...

2. 极致性能与低资源?
   └─ Yes ──► Linkerd (Rust proxy) 或 Cilium (eBPF)
   └─ No  ──► 继续...

3. 复杂多集群 / VM 混合?
   └─ Yes ──► Istio (最成熟的多集群方案)
   └─ No  ──► 继续...

4. 需要 L7 高级流量管理 (故障注入/超时/重试/镜像)?
   └─ Yes ──► Istio 或 Cilium + Envoy Extension
   └─ No  ──► Linkerd (基础功能足够)

5. 已有 Envoy/Gateway 投资?
   └─ Yes ──► Istio (原生 Envoy) 或 Cilium
   └─ No  ──► 任意选择

6. 需要应用级构建块 (状态/消息/配置)?
   └─ Yes ──► Dapr (+ 可选服务网格)
   └─ No  ──► 纯服务网格即可

7. 团队规模 < 10 人，快速上手?
   └─ Yes ──► Linkerd (2 分钟安装，零配置)
   └─ No  ──► Istio / Cilium

8. API 网关需求?
   └─ Yes ──► Gateway API + Envoy Gateway / Emissary / Kong
   └─ No  ──► 纯服务网格入口
```

---

## 参考链接

- [Istio 官方文档](https://istio.io/latest/docs/)
- [Linkerd 官方文档](https://linkerd.io/2/overview/)
- [Cilium Service Mesh](https://docs.cilium.io/en/stable/service-mesh/)
- [Dapr 官方文档](https://docs.dapr.io/)
- [Gateway API 官方](https://gateway-api.sigs.k8s.io/)
- [Envoy 官方文档](https://www.envoyproxy.io/docs/)
- [CNCF 服务网格白皮书](https://github.com/cncf/tag-network/blob/main/service-mesh-whitepaper.md)
