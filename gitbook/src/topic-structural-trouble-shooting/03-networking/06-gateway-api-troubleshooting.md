# Gateway API 深度排查与下一代流量治理指南

> **适用版本**: Kubernetes v1.25 - v1.32, Gateway API v1.0 - v1.2 | **最后更新**: 2026-02 | **难度**: 资深专家级

---

## 0. 读者对象与价值
| 角色 | 目标 | 核心收获 |
| :--- | :--- | :--- |
| **初学者** | 从 Ingress 迁移到 Gateway API，解决“无法绑定”问题 | 掌握 Gateway/Route/Service 的拓扑关系与 `status` 诊断路径。 |
| **中级运维** | 实施多租户流量隔离与 HTTPS 证书分发 | 理解 `ReferenceGrant` 的安全模型、多控制器共存逻辑与 `BackendTLSPolicy`。 |
| **资深专家** | 构建全链路加密与 Gamma 服务网格架构 | 深入控制器实现原理、GRPCRoute 优化、Session Persistence 治理与跨集群流量分发。 |

---

## 0.5 10 分钟快速诊断

1. **GatewayClass/Gateway 状态**：`kubectl get gatewayclass,gateway -A`，确认 `Accepted/Programmed` 为 True。
2. **Route 绑定**：`kubectl get httproute -A -o yaml | grep -A3 "parents"`，确认 `Accepted` 条件与 ParentRef 正确。
3. **跨 NS 引用**：`kubectl get referencegrant -A`，缺失时会出现 `ResolvedRefs=False`。
4. **后端健康**：检查 Service/Endpoints/探针，排除 503/502 来自后端不可用。
5. **TLS/证书**：确认 Listener 绑定的 Secret 存在、证书链正确；gRPC 场景核对 H2。
6. **控制器日志**：查看 Gateway 控制器日志（如 Envoy Gateway / Nginx Gateway）定位 reconcile 失败原因。
7. **快速缓解**：
   - 回滚最近 Route/Listener 变更。
   - 临时放宽 Route 绑定限制（AllowedRoutes）以恢复流量，再逐步收敛。
8. **证据留存**：保存 Gateway/Route 状态、ReferenceGrant、控制器日志与 curl/openssl 输出。

---

## 1. 核心架构与设计哲学

### 1.1 面向角色的解耦 (Role-based Model)
Gateway API 彻底解决了 Ingress 注解（Annotations）爆炸的问题，通过资源拆分实现了职责分离：
- **Infrastructure 层 (GatewayClass)**：由基础设施管理员管理，定义负载均衡器的实现（如 F5, Nginx, Envoy）。
- **Platform 层 (Gateway)**：由平台管理员管理，定义监听端口、SSL 证书与域名限制。
- **Application 层 (HTTPRoute/GRPCRoute)**：由开发人员管理，定义路径匹配、权重转发与 Header 修改。

### 1.2 为什么是 Gateway API？
- **类型安全**：原生支持 Header 修改、权重转发，无需厂商私有注解。
- **协议原生**：支持 GRPCRoute、TCPRoute、UDPRoute，涵盖 L4-L7 全场景。
- **跨命名空间引用**：通过 `ReferenceGrant` 安全地跨 NS 引用证书或后端 Service。
- **Gamma 计划**：正在统一服务网格（Mesh）与入口网关的配置规范。

---

## 2. 专家级故障矩阵与观测工具

### 2.1 专家级故障矩阵

| 现象分类 | 深度根因分析 | 关键观测指令 |
| :--- | :--- | :--- |
| **Gateway Not Programmed** | 下层 LB 资源（如云厂商 SLB）创建失败、端口已被占用、控制器配额不足。 | `kubectl describe gateway <name> -n <ns>` |
| **Route Detached (不绑定)** | Hostname 冲突、ParentRef 指向错误、Gateway 没允许该 Namespace 的 Route 接入。 | `kubectl get httproute <name> -o yaml` |
| **ResolvedRefs: False** | 引用了不存在的 Secret/Service，或缺少 `ReferenceGrant` 导致的权限受限。 | `kubectl get referencegrant -A` |
| **503 Backend Unhealthy** | 后端 Pod 探针失败、BackendLBPolicy 配置导致请求分配到不可达节点。 | `kubectl get backendlbpolicy` (v1.2+) |

### 2.2 专家工具箱

```bash
# 1. 追踪 Gateway 的完整状态机
kubectl get gateway -o custom-columns=NAME:.metadata.name,ACCEPTED:.status.conditions[?(@.type=="Accepted")].status,PROGRAMMED:.status.conditions[?(@.type=="Programmed")].status

# 2. 验证路由优先级 (发现阴影路由)
# 优先级顺序：精确 Hostname > 通配符 Hostname > 长路径匹配 > 字母序
kubectl get httproute -A --sort-by=.metadata.name

# 3. 检查控制器内部状态 (以 Envoy Gateway 为例)
egctl dashboard  # 开启 Envoy Gateway 专用调试控制台

# 4. 验证 BackendTLSPolicy (全链路加密)
kubectl get backendtlspolicy -A
```

---

## 3. 深度排查路径

### 3.1 第一阶段：资源链路完整性验证
确认 GatewayClass -> Gateway -> Route -> Service 这条链是否断裂。

```bash
# 检查 GatewayClass 是否被控制器 Accepted
kubectl get gatewayclass -o jsonpath='{.items[*].status.conditions[?(@.type=="Accepted")]}'

# 检查 Route 是否成功绑定到特定的 Parent
kubectl get httproute <name> -o jsonpath='{.status.parents[*].conditions[?(@.type=="Accepted")]}'
```

### 3.2 第二阶段：跨 Namespace 授权检查 (The ReferenceGrant)
**现象**：Route 状态正常，但后端指向一直失败。
**专家提示**：检查目标 Namespace 是否存在 `ReferenceGrant` 且配置正确。

```yaml
# 必须定义在【资源所在的 Namespace】中
kind: ReferenceGrant
spec:
  from:
  - group: gateway.networking.k8s.io
    kind: HTTPRoute
    namespace: route-ns  # 发起源
  to:
  - group: ""
    kind: Service        # 目标资源
```

---

## 4. 深度解决方案与生产最佳实践

### 4.1 解决“多域名证书管理”混乱
**方案**：利用 Gateway 的多 Listener 特性，每个域名一个 Listener，绑定各自的 Secret，并利用 `hostname` 字段进行隔离。

### 4.2 零停机迁移：Ingress 到 Gateway API
1. **共存期**：保持 Ingress Controller 运行，平行部署 Gateway API 控制器。
2. **流量切换**：利用 DNS 权重逐步将流量从 Ingress 的 LB IP 切换到 Gateway 的 LB IP。
3. **验证**：利用 `HTTPRoute` 的 `filters` 功能进行请求镜像（Mirroring），在生产环境旁路验证新规则。

### 4.3 构建全链路加密 (mTLS to Backend)
使用 `BackendTLSPolicy` 配置网关到后端 Pod 的 TLS 验证，实现真正的零信任：
```yaml
kind: BackendTLSPolicy
spec:
  targetRefs:
  - kind: Service
    name: my-backend
  tls:
    caKind: ConfigMap
    caName: root-ca
    hostname: backend.example.com
```

---

## 5. 生产环境典型案例解析

### 5.1 案例一：配置了 GRPCRoute 但返回 404
- **根因分析**：Listener 协议配置为 `HTTP` 而非 `HTTPS`（部分实现要求 gRPC 必须使用 TLS），或者没有启用 HTTP/2 支持。
- **对策**：检查 Listener 协议，并在 `GatewayClass` 或 `Gateway` 中通过 `parametersRef` 启用 H2。

### 5.2 案例二：开启 Session Persistence 后负载严重倾斜
- **根因分析**：Cookie 植入方式导致大量客户端请求被绑定到单个后端 Pod，且 Pod 滚动更新时粘滞性未释放。
- **对策**：配置 `sessionPersistence` 的 `type: Cookie` 并合理设置 `idleTimeout`。

---

## 附录：Gateway API 专家巡检表
- [ ] **版本对齐**：CRD 版本是否与控制器支持的版本（v1, v1beta1）匹配？
- [ ] **准入状态**：GatewayClass 与 Gateway 的 `Programmed` 状态是否为 True？
- [ ] **路由重叠**：不同 Route 之间是否存在相同的 Hostname + Path 导致的非预期覆盖？
- [ ] **安全授权**：所有跨 NS 引用是否都有对应的 `ReferenceGrant`？
- [ ] **监控集成**：是否采集了 `gateway_api_controller_reconcile_total` 等控制器指标？
