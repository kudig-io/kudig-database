# 服务网格（Service Mesh）

## 概述

**服务网格（Service Mesh）** 是一种专门处理服务间通信的基础设施层，通过透明代理（Sidecar 或 eBPF）为微服务提供统一的流量管理、安全通信（mTLS）和可观测性能力。2026 年的服务网格技术已形成 **Sidecar 模式（Istio、Linkerd）** 与 **Sidecar-less 模式（Cilium Service Mesh、Istio Ambient Mesh）** 并存的格局。

## 核心概念/原理

### 1. 服务网格核心能力

服务网格为所有服务间调用提供三大支柱能力：
- **流量管理**：负载均衡、熔断、重试、超时、A/B 测试、金丝雀发布
- **安全通信**：自动 mTLS 加密、身份认证、授权策略
- **可观测性**：分布式追踪、指标采集（延迟、错误率、吞吐量）、访问日志

### 2. Sidecar 模式

每个应用 Pod 中注入一个轻量级代理容器（如 Envoy），拦截所有入站和出站流量：
- **Istio**：基于 Envoy，功能最全面，企业采用最广
- **Linkerd**：自研 Rust 代理，极致轻量，资源开销最低

```yaml
# Istio Sidecar 注入示例
apiVersion: apps/v1
kind: Deployment
metadata:
  name: my-app
spec:
  template:
    metadata:
      annotations:
        sidecar.istio.io/inject: "true"
```

### 3. Sidecar-less 模式

将代理功能从 Pod 中剥离，下沉到节点级或内核级：
- **Cilium Service Mesh**：基于 eBPF 实现 L4 负载均衡和策略，L7 使用 per-node Envoy
- **Istio Ambient Mesh**：将数据平面分为 zTunnel（L4，per-node）和 Waypoint Proxy（L7，按需）

**Sidecar-less 优势**：
- 避免每个 Pod 的 CPU/Memory 开销（通常节省 20%–40%）
- 更快的 Pod 启动时间（无需等待 Sidecar Ready）
- 更简单的应用生命周期管理

### 4. 控制平面与数据平面

- **数据平面（Data Plane）**：实际转发流量的代理层
- **控制平面（Control Plane）**：负责配置分发、证书管理、身份发现
  - Istio：`istiod` 统一管理 Pilot、Citadel、Galley
  - Linkerd：`linkerd-destination` 提供路由和身份服务

## 关键机制或特性

### 流量管理

| 能力 | 说明 | 适用场景 |
|------|------|----------|
| **负载均衡** | 支持轮询、最少连接、一致性哈希、基于权重的分发 | 通用流量分发 |
| **熔断** | 当错误率超过阈值时自动切断流量，防止级联故障 | 保护下游依赖 |
| **重试与超时** | 自动重试失败请求并设置最大等待时间 | 提升容错能力 |
| **流量镜像** | 将生产流量复制到影子环境用于测试 | 安全验证新版本 |
| **金丝雀发布** | 按百分比逐步将流量切到新版本 | 降低发布风险 |
| **A/B 测试** | 基于 Header/Cookie 将流量路由到不同版本 | 业务实验对比 |

### 安全通信

- **自动 mTLS**：服务网格自动为所有服务间通信启用双向 TLS，无需应用改造
- **SPIFFE/SPIRE**：统一的服务身份框架，为每个工作负载颁发可验证的身份证书
- **授权策略**：基于源身份、目标路径、HTTP 方法等定义细粒度访问控制

### 可观测性

- **分布式追踪**：通过 OpenTelemetry/Jaeger/Zipkin 追踪跨服务请求链路
- **SRE 黄金指标**：自动采集 RED 指标（Rate、Errors、Duration）
- **访问日志**：记录每个请求的详细元数据，用于审计和故障排查

## 使用场景

1. **微服务流量治理**：数百个微服务之间的调用需要统一的负载均衡、熔断和重试策略
2. **零信任安全架构**：所有服务间通信强制 mTLS，配合 NetworkPolicy 实现纵深防御
3. **渐进式发布**：通过金丝雀和流量镜像验证新版本，降低生产发布风险
4. **多云混合云互联**：通过服务网格的 Cluster Mesh 能力连接跨云、跨数据中心的集群
5. **遗留系统现代化**：为无法改造的老系统透明添加安全、监控和流量管理能力

## 最佳实践/注意事项

- **评估 Sidecar vs Sidecar-less**：资源敏感型工作负载优先考虑 Cilium Service Mesh 或 Istio Ambient；功能复杂场景优先 Istio Sidecar
- **控制平面高可用**：istiod 或 linkerd-control-plane 必须多副本部署，避免单点故障导致全集群流量中断
- **渐进式启用 mTLS**：生产环境建议从 `PERMISSIVE` 模式开始，验证无误后再切换到 `STRICT` 模式
- **监控 Sidecar 资源消耗**：Istio Envoy 默认请求 100m CPU / 128Mi 内存，大规模部署时这笔开销不可忽视
- **避免 L7 策略滥用**：过多的 HTTP 路由规则会降低代理性能，应定期审查和精简 VirtualService/HTTPRoute
- **证书轮换监控**：服务网格依赖 CA 自动签发证书，必须监控证书过期和轮换失败告警
- **与 Gateway API 对齐**：2026 年推荐使用 Kubernetes Gateway API 替代 Istio 专用的 VirtualService/Gateway 资源

## 方案选型决策

| 维度 | Istio Sidecar | Istio Ambient | Cilium Mesh | Linkerd |
|------|--------------|---------------|-------------|---------|
| 数据面 | Per-Pod Envoy | zTunnel + Waypoint | eBPF + Per-node Envoy | Per-Pod Rust proxy |
| 资源开销 | 高（每 Pod +100m/128Mi） | 中（按需 L7） | 低 | 最低 |
| L7 功能 | 最完整 | 完整（Waypoint） | 中等 | 中等 |
| 启动延迟 | 有（等 Sidecar Ready） | 无 | 无 | 有（较小） |
| mTLS | 自动 | 自动（zTunnel） | WireGuard/SPIFFE | 自动 |
| 成熟度 | 最高 | GA（v1.24+） | 生产就绪 | 高 |
| Gateway API | 原生支持 | 原生支持 | 原生支持 | 支持 |

## 故障排查

| 症状 | 可能原因 | 排查步骤 |
|------|----------|----------|
| Sidecar 注入后 Pod 启动变慢 | Envoy 初始化和 xDS 配置拉取耗时 | 检查 istiod 负载；调整 Envoy concurrency |
| 服务间 503 错误 | 目标 Pod 未注入 Sidecar 或 mTLS 模式不匹配 | `istioctl analyze`；检查 PeerAuthentication 模式 |
| istiod 单点故障导致路由异常 | 控制平面未多副本部署 | `kubectl get deploy istiod -n istio-system` 确认副本数 ≥ 2 |
| 证书过期导致通信中断 | CA 根证书过期或轮换失败 | `istioctl proxy-config secret <pod>` 检查证书有效期 |
| L7 策略导致性能下降 | VirtualService/HTTPRoute 规则过多 | 精简路由规则；评估是否仅需 L4 策略 |

## 生产检查清单

- [ ] 控制平面（istiod/linkerd-control-plane）至少 2 副本
- [ ] mTLS 从 `PERMISSIVE` 模式开始，验证后切 `STRICT`
- [ ] 监控 Sidecar 资源消耗和注入成功率
- [ ] 证书轮换机制正常工作，设置过期告警
- [ ] L7 路由规则定期审查和精简
- [ ] 使用 Gateway API 替代 Istio 专用的 VirtualService/Gateway
- [ ] 灰度发布使用 HTTPRoute weight 进行流量分割

## 命令快速参考

```bash
# Istio 诊断
istioctl analyze -n production
istioctl proxy-status
istioctl proxy-config routes <pod>

# 查看 Sidecar 注入状态
kubectl get pods -n production -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{.spec.containers[*].name}{"\n"}{end}'

# Linkerd 诊断
linkerd check
linkerd viz stat deploy -n production
linkerd viz top deploy/<name> -n production

# 查看 mTLS 状态
istioctl authn tls-check <pod>

# 流量镜像验证
kubectl get virtualservice -n production -o yaml
```

## 交叉引用

- [eBPF 与 Cilium](ebpf-and-cilium-networking.md) — Cilium Sidecar-less Service Mesh
- [Gateway API](gateway-api.md) — 推荐的服务网格流量管理 API
- [Network Policies](network-policies.md) — L3/L4 层面的策略补充
- [Cluster Mesh](cluster-mesh.md) — 多集群服务网格互联

## 参考链接

- [Istio Documentation](https://istio.io/latest/docs/)
- [Linkerd Documentation](https://linkerd.io/2/overview/)
- [Cilium Service Mesh](https://docs.cilium.io/en/stable/network/servicemesh/)
- [Istio Ambient Mesh](https://istio.io/latest/docs/ambient/)
- [Ajeet Singh Raina - Top 5 Trends Shaping Kubernetes in 2026](https://www.ajeetraina.com/top-5-trends-shaping-kubernetes-in-2026/)
