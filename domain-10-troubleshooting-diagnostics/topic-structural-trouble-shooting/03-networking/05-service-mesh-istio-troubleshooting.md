---
title: Service Mesh (Istio) 深度排查与性能调优指南 [topic-structural-trouble-shooting]
description: 'title: Service Mesh (Istio) 深度排查与性能调优指南'
summary: 'title: Service Mesh (Istio) 深度排查与性能调优指南'
category: structural-troubleshooting
tags:
- troubleshooting
- guide
- etcd
- prometheus
- grafana
- jaeger
- istio
- envoy
- helm
- docker
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 技术支持
estimated_read_time: 1h
intent_queries:
- Service Mesh (Istio) 深度排查与性能调优指南 是什么
- 如何 Service Mesh (Istio) 深度排查与性能调优指南
- Kubernetes 10 troubleshooting diagnostics 最佳实践
- Service Mesh (Istio) 深度排查与性能调优指南 故障排查
- Service Mesh (Istio) 深度排查与性能调优指南 排障步骤
trigger_keywords:
- Service
- Mesh
- Istio
- 深度排查与性能调优指南
- troubleshooting
- diagnostics
- structural
- trouble
prerequisites:
- kubectl-basics
- troubleshooting-methodology
- helm-basics
- service-mesh-basics
- prometheus-basics
- monitoring-basics
- ebpf-basics
- etcd-basics
- policy-basics
- backup-basics
- tracing-basics
---



title: [[Service|Service]]Service Mesh）|Service Mesh]] ([[Istio|Istio]]) 深度排查与性能调优指南
description: '# Service Mesh (Istio) 深度排查与性能调优指南'
category: structural-troubleshooting
tags:
- k8s
- troubleshooting
- decision-tree
- prometheus
- jaeger
- istio
- envoy
- helm
- hpa
- pdb
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 技术支持
estimated_read_time: 10min
intent_queries:
- Service Mesh (Istio) 深度排查与性能调优指南 是什么
- 如何 Service Mesh (Istio) 深度排查与性能调优指南
- Service Mesh (Istio) 深度排查与性能调优指南 故障排查
- Service Mesh (Istio) 深度排查与性能调优指南 排障步骤
trigger_keywords:
- Service
- Mesh
- Istio
- 深度排查与性能调优指南
- structural
- trouble
- shooting
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

# Service Mesh (Istio) 深度排查与性能调优指南

> **适用版本**: Kubernetes v1.25 - v1.32, Istio v1.18 - v1.24 | **最后更新**: 2026-02 | **难度**: 资深专家级

---

## 0. 读者对象与价值
| 角色 | 目标 | 核心收获 |
| :--- | :--- | :--- |
| **初学者** | 解决 Sidecar 注入、404/503 报错等基础问题 | 掌握 Istio 流量模型（VS/DR/GW）与 `istioctl` 基础诊断。 |
| **中级运维** | 优化证书管理、实施精细化流量控制 | 理解 mTLS 原理、掌握 xDS 配置同步状态分析、Envoy 日志解读。 |
| **资深专家** | 解决大规模集群瓶颈与 Ambient Mesh 落地 | 深入 xDS 底层报文（LDS/RDS/CDS/EDS）、Ambient Mesh 架构问题、硬件加速（TLS Offload）与性能调优。 |

---

## 0.5 10 分钟快速诊断

1. **控制面与代理同步**：`istioctl proxy-status`，确认所有代理 `SYNCED`；`kubectl get pods -n istio-system` 检查 istiod。
2. **Sidecar 注入**：`kubectl get pod <pod> -o jsonpath='{.spec.containers[*].name}'`，确认 `istio-proxy` 存在；必要时检查 `istio-injection=enabled` 标签。
3. **访问路径验证**：在源 Pod 内 `curl` 目标服务，结合 Envoy 访问日志解析 `response_flags`。
4. **xDS 配置核对**：`istioctl proxy-config route/cluster/endpoint <pod>`，确认 VirtualService/DR 是否已下发。
5. **mTLS 模式**：`kubectl get peerauthentication -A`，确认是否 `STRICT` 导致非 mTLS 客户端失败。
6. **Gateway 健康**：`kubectl get pods -l app=istio-ingressgateway -n istio-system`，检查 502/503 日志。
7. **快速缓解**：
   - 灰度回退：对关键命名空间先设为 `PERMISSIVE`。
   - 资源加固：提高 istiod 与 gateway 资源并扩副本。
8. **证据留存**：保存 `istioctl analyze` 输出、proxy-status、关键 Envoy 日志。

---

## 问题现象与影响分析

### 常见问题现象

- **Sidecar 注入失败**：Pod 中缺少 `istio-proxy` 或注入异常。
- **503/504/502**：上游 Endpoint 不可用、路由未下发、Gateway 资源不足。
- **mTLS 握手失败**：证书过期、信任链不一致、模式从 `PERMISSIVE` 切到 `STRICT` 导致旧客户端失败。
- **配置不同步**：`proxy-status` 出现 `STALE` 或 xDS 推送延迟。

### 影响面分析

- **业务请求失败**：流量被错误路由或被策略拒绝，导致 4xx/5xx。
- **性能下降**：Envoy CPU/内存激增，影响延迟与吞吐。
- **可观测性失真**：日志/指标采集异常，误导排障方向。

## 排查方法与步骤

1. **控制面健康检查**：`kubectl get pods -n istio-system`，查看 `istiod` 与 Gateway 状态。
2. **代理同步状态**：`istioctl proxy-status`，确认配置是否 `SYNCED`。
3. **路由与端点核对**：`istioctl proxy-config route/endpoint <pod>`，确认 VS/DR 是否下发。
4. **mTLS 模式核对**：`kubectl get peerauthentication -A`，排查模式切换导致的失败。
5. **日志与响应标记**：解析 Envoy 日志中的 `response_flags`，定位上游失败类型。
6. **修复验证**：回归关键路径，验证流量恢复与告警下降。

## 解决方案与风险控制

### 常见修复策略

- **注入失败**：修复命名空间标签或 Webhook 健康，必要时临时手动注入验证。
- **503/502**：检查 Service/Endpoints 与 Gateway 资源，必要时扩容 gateway/istiod。
- **mTLS 失败**：先临时回退到 `PERMISSIVE`，确认依赖方全部支持 mTLS 后再收敛。

### 风险控制与回滚

- **变更前**：保存 `istioctl analyze` 输出与 proxy 配置快照。
- **回滚策略**：回退最近的 VirtualService/DestinationRule 或恢复旧版本 Istiod。
- **验证**：使用 `istioctl proxy-status` 与关键业务探活确认恢复。

---

## 1. 核心架构与底层机制

### 1.1 xDS 协议：Istio 的灵魂
Istio 的控制面 `istiod` 与数据面 `Envoy` 之间通过 xDS（Discovery Service）协议通信：
- **LDS (Listener DS)**：监听器发现，定义 Envoy 监听哪些端口（如 15001/15006）。
- **RDS (Route DS)**：路由发现，VirtualService 定义的路由规则。
- **CDS (Cluster DS)**：集群发现，上游 Service 定义（包括 DestinationRule 的 LoadBalancer 策略）。
- **EDS (Endpoint DS)**：端点发现，具体的 Pod IP 列表。
- **SDS (Secret DS)**：秘钥发现，mTLS 所需的证书与私钥分发。

### 1.2 数据面演进：Sidecar vs Ambient Mesh
- **Sidecar 模式**：每个 Pod 注入 Envoy。优点：功能最全，物理隔离；缺点：资源占用大，应用生命周期绑定。
- **Ambient Mesh (无 sidecar)**：
  - **ztunnel**：节点级安全代理，处理 L4（mTLS, AuthZ）。
  - **Waypoint Proxy**：Namespace 级 L7 代理（可选），处理重试、金丝雀、WAF 等。
  - **优点**：零注入成本，显著降低 CPU/内存占用。

---

## 2. 专家级问题矩阵与观测工具

### 2.1 专家级问题矩阵

| 现象分类 | 深度根因分析 | 关键观测指令 |
| :--- | :--- | :--- |
| **503 UH (No Upstream)** | CDS/EDS 推送延迟、Service selector 匹配为空、Outlier Detection 熔断触发。 | `istioctl pc endpoint <pod> --address <target-ip>` |
| **403 RBAC 拒绝** | AuthorizationPolicy 冲突、mTLS 证书过期/信任链不一致。 | `kubectl exec -c istio-proxy -- curl localhost:15000/certs` |
| **Config Stale (状态滞后)** | istiod 负载过高、gRPC 阻塞、大规模集群 EDS 推送风暴。 | `istioctl proxy-status` |
| **502/504 (Gateway)** | Gateway Pod 资源不足、后端应用 KeepAlive 超时小于 Envoy 超时。 | `kubectl logs -l app=istio-ingressgateway` |

### 2.2 专家工具箱

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

```bash
# 1. 一键收集诊断包 (专家必备)
istioctl bug-report --namespace istio-system --duration 5m

# 2. 追踪特定的 xDS 推送延迟 (查看 istiod 内部性能)
istioctl proxy-status --server istiod-xxx.istio-system

# 3. 实时对比 API 对象与 Envoy 运行时配置 (发现推送阴影)
istioctl proxy-config cluster <pod-name> -o json > clusters.json

# 4. 进入 Ambient Mesh ztunnel 诊断模式
kubectl exec -n istio-system <ztunnel-pod> -- ztunnel-config dump
```

---

## 3. 深度排查路径

### 3.1 第一阶段：控制面健康与同步状态
确认配置是否“到家”。

```bash
# 检查 istiod 是否有大面积推送错误
kubectl logs -n istio-system -l app=istiod | grep -E "push error|cache failure"

# 分析当前 Namespace 的配置风险
istioctl analyze -n my-ns --suppress "IST0102" # 抑制已知次要警告
```

### 3.2 第二阶段：Envoy 状态码深度解析 (Response Flags)
从 Envoy 访问日志中解读流量真相：
- **UH**: Upstream unhealthy (上游没 Ready Pod)。
- **CC**: Circuit breaker (触发熔断)。
- **UF**: Upstream connection failure (mTLS 握手失败或连接重置)。
- **NR**: No route configured (VirtualService 没配对)。

---

## 4. 深度解决方案与生产最佳实践

### 4.1 解决 mTLS 迁移中的“断网”风险
**策略**：使用 `PERMISSIVE` 模式作为缓冲区。
```yaml
apiVersion: security.istio.io/v1beta1
kind: PeerAuthentication
spec:
  mtls:
    mode: PERMISSIVE  # 允许 mTLS 和普通流量并存，待监控稳定后再切 STRICT
```

### 4.2 应对大规模集群的 xDS 性能优化
- **使用 Sidecar 资源对象**：强制限制 Sidecar 监听的 Service 范围，减少内存消耗。
  ```yaml
  spec:
    egress:
    - hosts: ["./*", "istio-system/*"] # 只接收本命名空间和系统级的配置
  ```
- **开启 EDS 分量推送**：避免一个 Pod 变动导致全集群推送。

### 4.3 解决 Ingress Gateway 证书更新延迟
- **方案**：尽量使用 `SDS`（credentialName 引用 Secret），避免通过 hostpath 挂载证书。Secret 更新后，Envoy 会自动通过 SDS 接口热加载，无需重启。

---

## 5. 生产环境典型案例解析

### 5.1 案例一：启用 Sidecar 后应用启动报错“Connection Refused”
- **根因分析**：应用启动早于 Sidecar (Envoy)，导致应用尝试外连时网络规则尚未生效。
- **对策**：开启 Sidecar 启动顺序保护 (Istio 1.22+ 利用 K8s Native Sidecar)。
  ```yaml
  # Helm 或 Global MeshConfig
  values.global.proxy.holdApplicationUntilProxyReceivesConfig=true
  ```

### 5.2 案例二：跨机房调用时偶尔出现 503，但应用正常
- **根因分析**：跨机房网络抖动触发了 Envoy 的默认重试逻辑。
- **对策**：在 VirtualService 中显式配置幂等接口的重试策略，并增加超时缓冲。

---

## 附录：Istio 专家巡检表
- [ ] **配置健康度**：`istioctl analyze` 是否无严重错误？
- [ ] **同步状态**：所有代理是否处于 `SYNCED` 状态？
- [ ] **证书安全**：mTLS 模式是否已升级至 `STRICT` (核心业务)？
- [ ] **资源冗余**：Gateway Pod 是否配置了 HPA 和 PDB？
- [ ] **观测闭环**：是否集成了 Kiali、Jaeger 和 Prometheus 且指标正常？
- [ ] **优雅停机**：应用是否处理了 SIGTERM 信号并配合 Sidecar 的 `drainTime`？


---

## 1.3 Istio 控制平面深度解析

### 1.3.1 Istiod 统一架构

**组件整合历史**

```
Istio 1.5 之前 (多组件):
┌────────────────────────────────────────────────────────┐
│ Pilot     - 流量管理 (xDS 服务器)                      │
│ Citadel   - 证书管理 (CA + SDS)                       │
│ Galley    - 配置验证与分发                             │
│ Mixer     - 遥测与策略 (已废弃)                        │
└────────────────────────────────────────────────────────┘

Istio 1.5+ (统一):
┌────────────────────────────────────────────────────────┐
│                       Istiod                           │
│  ┌──────────────────────────────────────────────┐      │
│  │  Config Ingestion (Galley 功能)             │      │
│  │  - 监听 K8s API Server                      │      │
│  │  - 监听 VirtualService/DestinationRule      │      │
│  │  - 配置校验与转换                            │      │
│  └───────────────────┬──────────────────────────┘      │
│                      │                                 │
│  ┌──────────────────▼──────────────────────────┐      │
│  │  xDS Server (Pilot 功能)                    │      │
│  │  - LDS/RDS/CDS/EDS 推送                     │      │
│  │  - 维护 Envoy 连接池                        │      │
│  │  - 增量推送优化                              │      │
│  └───────────────────┬──────────────────────────┘      │
│                      │                                 │
│  ┌──────────────────▼──────────────────────────┐      │
│  │  CA (Citadel 功能)                          │      │
│  │  - 签发工作负载证书                          │      │
│  │  - SPIFFE Identity 管理                     │      │
│  │  - 证书轮换 (默认 24h)                       │      │
│  └──────────────────────────────────────────────┘      │
│                                                        │
└────────────────────────────────────────────────────────┘
         │                    │                  │
         ▼                    ▼                  ▼
   ┌─────────┐          ┌─────────┐       ┌─────────┐
   │ Envoy 1 │          │ Envoy 2 │       │ Envoy N │
   │ (Sidecar)│          │(Gateway)│       │(Sidecar)│
   └─────────┘          └─────────┘       └─────────┘
```

**Istiod 核心流程**

```go
// 简化的 Istiod xDS 推送流程 (伪代码)

func (p *PilotServer) HandleXDSConnection(stream grpc.ServerStream) {
    // 1. Envoy 连接并发送 DiscoveryRequest
    request := stream.Recv()
    
    // 2. 识别 Envoy 身份 (Pod Name/Namespace/IP)
    proxyID := extractProxyID(request)
    
    // 3. 查询该 Proxy 需要的配置
    // - 基于 Sidecar 资源限制作用域
    // - 基于 Namespace 过滤 Service
    relevantServices := filterServices(proxyID)
    
    // 4. 生成 xDS 配置
    switch request.TypeUrl {
    case "type.googleapis.com/envoy.config.listener.v3.Listener":
        // 生成 Listener (端口 15001/15006/应用端口)
        listeners := generateListeners(proxyID, relevantServices)
        response := &DiscoveryResponse{
            TypeUrl: request.TypeUrl,
            Resources: listeners,
            Nonce: generateNonce(),
            VersionInfo: getCurrentVersion(),
        }
        
    case "type.googleapis.com/envoy.config.route.v3.RouteConfiguration":
        // 生成 Route (VirtualService 规则)
        routes := generateRoutes(proxyID, relevantServices)
        
    case "type.googleapis.com/envoy.config.cluster.v3.Cluster":
        // 生成 Cluster (DestinationRule 配置)
        clusters := generateClusters(proxyID, relevantServices)
        
    case "type.googleapis.com/envoy.config.endpoint.v3.ClusterLoadAssignment":
        // 生成 Endpoint (Pod IP 列表)
        endpoints := generateEndpoints(proxyID, relevantServices)
    }
    
    // 5. 推送配置到 Envoy
    stream.Send(response)
    
    // 6. 等待 Envoy ACK
    ack := stream.Recv()
    if ack.ErrorDetail != nil {
        logError("Envoy rejected config", ack.ErrorDetail)
    }
}

// 触发增量推送的事件:
// - Service/Pod 创建/删除/更新
// - VirtualService/DestinationRule 变更
// - Certificate 轮换
// - Sidecar 资源变更
func (p *PilotServer) OnConfigChange(event ConfigEvent) {
    // 1. 计算影响范围 (哪些 Envoy 需要更新)
    affectedProxies := calculateAffectedProxies(event)
    
    // 2. 增量推送 (只推送变更部分)
    for _, proxy := range affectedProxies {
        deltaConfig := computeDelta(proxy, event)
        pushToProxy(proxy, deltaConfig)
    }
}
```

**xDS 配置示例**

```json
// LDS (Listener Discovery Service) - Envoy 监听哪些端口
{
  "name": "0.0.0.0_15006",
  "address": {
    "socketAddress": {
      "address": "0.0.0.0",
      "portValue": 15006  // Inbound 流量入口
    }
  },
  "filterChains": [
    {
      "filters": [
        {
          "name": "envoy.filters.network.http_connection_manager",
          "typedConfig": {
            "@type": "type.googleapis.com/envoy.extensions.filters.network.http_connection_manager.v3.HttpConnectionManager",
            "routeConfig": {
              "name": "inbound|8080||myservice.default.svc.cluster.local"
            }
          }
        }
      ]
    }
  ]
}

// RDS (Route Discovery Service) - VirtualService 路由规则
{
  "name": "8080",
  "virtualHosts": [
    {
      "name": "myservice.default.svc.cluster.local:8080",
      "domains": ["myservice.default.svc.cluster.local", "myservice", "10.96.1.5"],
      "routes": [
        {
          "match": {
            "prefix": "/api/v1",
            "headers": [
              {
                "name": "x-canary",
                "exactMatch": "true"
              }
            ]
          },
          "route": {
            "weightedClusters": {
              "clusters": [
                {"name": "outbound|8080|v2|myservice.default.svc.cluster.local", "weight": 10},
                {"name": "outbound|8080|v1|myservice.default.svc.cluster.local", "weight": 90}
              ]
            },
            "timeout": "15s",
            "retryPolicy": {
              "retryOn": "5xx",
              "numRetries": 3
            }
          }
        }
      ]
    }
  ]
}

// CDS (Cluster Discovery Service) - DestinationRule 配置
{
  "name": "outbound|8080|v1|myservice.default.svc.cluster.local",
  "type": "EDS",  // 通过 EDS 获取 Endpoint
  "edsClusterConfig": {
    "serviceName": "outbound|8080|v1|myservice.default.svc.cluster.local"
  },
  "connectTimeout": "10s",
  "lbPolicy": "LEAST_REQUEST",  // 负载均衡策略
  "circuitBreakers": {
    "thresholds": [
      {
        "maxConnections": 1024,
        "maxPendingRequests": 1024,
        "maxRequests": 1024,
        "maxRetries": 3
      }
    ]
  },
  "outlierDetection": {  // 异常检测 (熔断)
    "consecutiveErrors": 5,
    "interval": "10s",
    "baseEjectionTime": "30s",
    "maxEjectionPercent": 50
  },
  "transportSocket": {  // mTLS 配置
    "name": "envoy.transport_sockets.tls",
    "typedConfig": {
      "@type": "type.googleapis.com/envoy.extensions.transport_sockets.tls.v3.UpstreamTlsContext",
      "sni": "outbound_.8080_.v1_.myservice.default.svc.cluster.local"
    }
  }
}

// EDS (Endpoint Discovery Service) - Pod IP 列表
{
  "clusterName": "outbound|8080|v1|myservice.default.svc.cluster.local",
  "endpoints": [
    {
      "lbEndpoints": [
        {
          "endpoint": {
            "address": {
              "socketAddress": {
                "address": "10.244.1.5",
                "portValue": 8080
              }
            }
          },
          "healthStatus": "HEALTHY",
          "loadBalancingWeight": 1
        },
        {
          "endpoint": {
            "address": {
              "socketAddress": {
                "address": "10.244.2.8",
                "portValue": 8080
              }
            }
          },
          "healthStatus": "HEALTHY"
        }
      ]
    }
  ]
}
```

### 1.3.2 mTLS 证书体系深度解析

**SPIFFE Identity 架构**

```
┌─────────────────────────────────────────────────────────┐
│              Istio mTLS 证书链                           │
├─────────────────────────────────────────────────────────┤
│                                                         │
│  ┌────────────────────────────────────────┐             │
│  │     Root CA (自签名或外部 CA)           │             │
│  │  - 证书有效期: 10 年                    │             │
│  │  - 私钥存储: K8s Secret (istio-ca-secret)│            │
│  │  - SPIFFE Trust Domain: cluster.local  │             │
│  └───────────────────┬────────────────────┘             │
│                      │ 签发                             │
│                      ▼                                  │
│  ┌────────────────────────────────────────┐             │
│  │     Intermediate CA (Istiod 内置)       │             │
│  │  - 证书有效期: 1 年                     │             │
│  │  - 自动轮换                              │             │
│  └───────────────────┬────────────────────┘             │
│                      │ 签发                             │
│              ┌───────┴───────┐                          │
│              │                │                          │
│              ▼                ▼                          │
│  ┌──────────────────┐  ┌──────────────────┐             │
│  │ Workload Cert 1  │  │ Workload Cert 2  │             │
│  │  Pod: frontend   │  │  Pod: backend    │             │
│  │  Identity:       │  │  Identity:       │             │
│  │  spiffe://       │  │  spiffe://       │             │
│  │  cluster.local/  │  │  cluster.local/  │             │
│  │  ns/default/     │  │  ns/prod/        │             │
│  │  sa/frontend     │  │  sa/backend      │             │
│  │                  │  │                  │             │
│  │  有效期: 24h     │  │  有效期: 24h     │             │
│  │  (自动轮换)      │  │  (自动轮换)      │             │
│  └──────────────────┘  └──────────────────┘             │
│                                                         │
└─────────────────────────────────────────────────────────┘
```

**证书轮换流程**

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

```bash
# 证书轮换过程 (无需重启 Pod)

# 时间线:
# T=0:    Pod 启动, Envoy 请求证书
# T=1s:   Istiod 签发证书 (有效期 24h)
# T=12h:  Envoy 开始准备新证书 (提前 50%)
# T=18h:  Envoy 请求新证书
# T=18h:  Istiod 签发新证书, Envoy 热加载
# T=24h:  旧证书过期 (但已被替换)

# 1. 查看当前证书信息
kubectl exec -c istio-proxy <pod> -- \
  curl -s localhost:15000/certs | jq '.[0]'

# 输出示例:
{
  "ca_cert": "-----BEGIN CERTIFICATE-----\n...",  # Intermediate CA
  "cert_chain": "-----BEGIN CERTIFICATE-----\n...", # Workload Cert
  "valid_from": "2026-02-10T10:00:00Z",
  "expiration_time": "2026-02-11T10:00:00Z"  # 24h 有效期
}

# 2. 查看证书 Subject Alternative Name (SAN)
kubectl exec -c istio-proxy <pod> -- \
  openssl x509 -in /etc/certs/cert-chain.pem -text -noout | grep "Subject Alternative Name" -A1

# 输出:
# Subject Alternative Name:
#   URI:spiffe://cluster.local/ns/default/sa/myapp

# 3. 验证证书链
kubectl exec -c istio-proxy <pod> -- \
  openssl verify -CAfile /etc/certs/root-cert.pem /etc/certs/cert-chain.pem

# 输出: /etc/certs/cert-chain.pem: OK
```

**mTLS 握手过程**

```
Client Pod (frontend)           Server Pod (backend)
     │                                │
     │  1. TCP SYN                    │
     ├────────────────────────────────>│
     │                                │
     │  2. TCP SYN-ACK                │
     │<────────────────────────────────┤
     │                                │
     │  3. TCP ACK                    │
     ├────────────────────────────────>│
     │                                │
     │  4. TLS ClientHello            │
     │  - 支持的 cipher suites        │
     │  - SNI: backend.default.svc    │
     ├────────────────────────────────>│
     │                                │
     │  5. TLS ServerHello            │
     │  - 选择的 cipher: ECDHE-RSA    │
     │  - 服务端证书 (backend)        │
     │  - 请求客户端证书 (mTLS!)      │
     │<────────────────────────────────┤
     │                                │
     │  6. TLS Certificate            │
     │  - 客户端证书 (frontend)       │
     │  - 证书验证:                   │
     │    * SAN: spiffe://...frontend │
     │    * CA: Istio Root CA         │
     ├────────────────────────────────>│
     │                                │
     │  7. Finished                   │
     │  - 完成握手                    │
     ├────────────────────────────────>│
     │                                │
     │  8. Application Data           │
     │  - HTTP 请求 (加密)            │
     ├────────────────────────────────>│
     │                                │
```

**PeerAuthentication 策略详解**

```yaml
# 模式 1: STRICT - 强制 mTLS (推荐生产环境)
apiVersion: security.istio.io/v1beta1
kind: PeerAuthentication
metadata:
  name: default
  namespace: prod
spec:
  mtls:
    mode: STRICT  # 拒绝所有非 mTLS 流量
  # 结果: 
  # - Istio 内部流量: ✅ (自动 mTLS)
  # - 外部客户端: ❌ (除非也用 mTLS)

---
# 模式 2: PERMISSIVE - 允许混合 (迁移期)
apiVersion: security.istio.io/v1beta1
kind: PeerAuthentication
metadata:
  name: default
  namespace: staging
spec:
  mtls:
    mode: PERMISSIVE  # 同时接受 mTLS 和明文
  # 结果:
  # - Istio 内部流量: ✅ (优先 mTLS)
  # - 外部客户端: ✅ (降级到明文)
  # - 风险: 可能存在安全降级攻击

---
# 模式 3: DISABLE - 禁用 mTLS (仅测试)
apiVersion: security.istio.io/v1beta1
kind: PeerAuthentication
metadata:
  name: legacy-app
  namespace: dev
spec:
  selector:
    matchLabels:
      app: legacy-db  # 只针对特定应用
  mtls:
    mode: DISABLE
  # 结果:
  # - 该应用: 纯明文通信
  # - 其他应用: 仍然 mTLS

---
# 模式 4: 端口级策略 (混合场景)
apiVersion: security.istio.io/v1beta1
kind: PeerAuthentication
metadata:
  name: mixed-ports
  namespace: hybrid
spec:
  mtls:
    mode: STRICT
  portLevelMtls:
    8080:
      mode: STRICT    # HTTP API 强制 mTLS
    9090:
      mode: DISABLE   # Prometheus metrics 允许明文
```

### 1.3.3 Envoy Sidecar 生命周期管理

**Sidecar 注入机制**

```yaml
# Sidecar 注入由 Mutating Webhook 实现
apiVersion: admissionregistration.k8s.io/v1
kind: MutatingWebhookConfiguration
metadata:
  name: istio-sidecar-injector
webhooks:
- name: sidecar-injector.istio.io
  clientConfig:
    service:
      name: istiod
      namespace: istio-system
      path: /inject
  rules:
  - operations: ["CREATE"]
    apiGroups: [""]
    apiVersions: ["v1"]
    resources: ["pods"]
  namespaceSelector:
    matchLabels:
      istio-injection: enabled  # 命名空间需要此标签

# 注入内容 (简化版):
# 1. initContainers:
#    - istio-init: 配置 iptables 规则 (劫持流量)
# 2. containers:
#    - istio-proxy: Envoy 容器
# 3. volumes:
#    - istio-envoy: Envoy 配置
#    - istio-certs: mTLS 证书
#    - istio-token: ServiceAccount Token
```

**流量劫持原理 (iptables)**

```bash
# istio-init 容器配置的 iptables 规则

# 1. Outbound 流量劫持 (应用发出的流量)
iptables -t nat -A OUTPUT \
  -p tcp \
  ! -d 127.0.0.1/32 \
  -j ISTIO_OUTPUT

iptables -t nat -A ISTIO_OUTPUT \
  -m owner --uid-owner 1337 \  # 1337 = istio-proxy UID
  -j RETURN  # Envoy 自身流量不劫持

iptables -t nat -A ISTIO_OUTPUT \
  -j ISTIO_REDIRECT

iptables -t nat -A ISTIO_REDIRECT \
  -p tcp \
  -j REDIRECT --to-port 15001  # 重定向到 Envoy Outbound Listener

# 2. Inbound 流量劫持 (进入 Pod 的流量)
iptables -t nat -A PREROUTING \
  -p tcp \
  -j ISTIO_INBOUND

iptables -t nat -A ISTIO_INBOUND \
  -p tcp --dport 15020 \
  -j RETURN  # Health check 端口不劫持

iptables -t nat -A ISTIO_INBOUND \
  -p tcp --dport 15021 \
  -j RETURN  # Status 端口不劫持

iptables -t nat -A ISTIO_INBOUND \
  -p tcp \
  -j ISTIO_IN_REDIRECT

iptables -t nat -A ISTIO_IN_REDIRECT \
  -p tcp \
  -j REDIRECT --to-port 15006  # 重定向到 Envoy Inbound Listener

# 流量路径示例:
# 应用发送请求: app:8080 → curl http://backend:8080/api
# ↓
# iptables OUTPUT 链劫持 → 重定向到 127.0.0.1:15001 (Envoy Outbound)
# ↓
# Envoy 查询 xDS → 获取 backend 的 Cluster/Endpoint
# ↓
# Envoy 建立 mTLS 连接到 backend Pod IP:15006
# ↓
# backend 的 iptables PREROUTING 链劫持 → 127.0.0.1:15006 (Envoy Inbound)
# ↓
# Envoy 验证 mTLS 证书 → 转发到应用 127.0.0.1:8080
```

**Sidecar 启动顺序问题**

```yaml
# 问题: 应用容器先于 Envoy 启动, 导致初始请求失败

# 解决方案 1: HoldApplicationUntilProxyStarts (Istio 1.7+)
apiVersion: install.istio.io/v1alpha1
kind: IstioOperator
spec:
  values:
    global:
      proxy:
        holdApplicationUntilProxyStarts: true
  # 原理: 在应用容器的 postStart hook 中等待 Envoy Ready

---
# 解决方案 2: Native Sidecar (K8s 1.28+, Istio 1.22+)
apiVersion: v1
kind: Pod
spec:
  initContainers:
  - name: istio-proxy
    image: istio/proxyv2:1.22.0
    restartPolicy: Always  # 标记为 Native Sidecar
  containers:
  - name: app
    image: myapp:v1
  # K8s 保证 istio-proxy 先启动并 Ready

---
# 解决方案 3: 应用重试 (应用侧改造)
# 应用启动时对外部依赖进行重试
import time
import requests

def connect_database():
    for i in range(10):
        try:
            conn = psycopg2.connect(host='db.prod', port=5432)
            return conn
        except:
            print(f"Retry {i}/10")
            time.sleep(2)
    raise Exception("Cannot connect to database")

```

---

## 2.3 专家级问题矩阵 (按组件分类)

### 2.3.1 控制平面问题 (Istiod)

| 问题现象 | 根因分析 | 排查路径 | 典型场景 |
|----------|----------|----------|----------|
| Proxy Status: STALE | xDS 推送阻塞或超时 | `kubectl logs -n istio-system istiod-xxx | grep "push error"` | 大规模集群 (5000+ Pod) |
| Config 推送延迟 > 30s | Istiod CPU/内存不足 | `kubectl top pod -n istio-system` | EDS 推送风暴 |
| VirtualService 不生效 | 配置语法错误 | `istioctl analyze -n <ns>` | 正则表达式错误 |
| Certificate 签发失败 | CA Secret 丢失 | `kubectl get secret istio-ca-secret -n istio-system` | 误删除 Secret |

### 2.3.2 数据平面问题 (Envoy Sidecar)

| 问题现象 | 根因分析 | 排查路径 | 典型场景 |
|----------|----------|----------|----------|
| 503 UH (Upstream Unhealthy) | Endpoint 未就绪 | `istioctl pc endpoint <pod> --address <ip>` | Pod 健康检查失败 |
| 503 UC (Upstream Connection Failure) | mTLS 握手失败 | `kubectl logs <pod> -c istio-proxy | grep "TLS error"` | 证书过期/时钟偏移 |
| 503 UF (Upstream Failure) | 上游应用返回错误 | 查看应用日志 | 应用 Bug |
| 404 NR (No Route) | VirtualService 未匹配 | `istioctl pc route <pod>` | 路径拼写错误 |
| 429 RL (Rate Limited) | 触发限流 | 检查 EnvoyFilter 限流配置 | QPS 超限 |
| 503 UO (Upstream Overflow) | 连接池耗尽 | `istioctl pc cluster <pod> | grep circuit_breakers` | 并发过高 |

### 2.3.3 Gateway 问题 (Ingress/Egress)

| 问题现象 | 根因分析 | 排查路径 | 典型场景 |
|----------|----------|----------|----------|
| 502 Bad Gateway | 后端 Pod 不存在 | `kubectl get endpoints <svc>` | Service Selector 错误 |
| 504 Gateway Timeout | 后端响应超时 | 检查 VirtualService timeout 配置 | 数据库慢查询 |
| TLS 握手失败 | 证书配置错误 | `kubectl logs -l app=istio-ingressgateway` | SAN 不匹配 |
| Gateway 无响应 | Pod OOM/CrashLoop | `kubectl get pods -n istio-system` | 资源限制过低 |

### 2.3.4 性能问题

| 问题现象 | 根因分析 | 排查路径 | 典型场景 |
|----------|----------|----------|----------|
| Envoy CPU 100% | 路由规则过多/正则复杂 | `kubectl top pod --containers` | VirtualService 使用复杂正则 |
| 内存持续增长 | xDS 配置过大 | `kubectl exec -c istio-proxy -- curl localhost:15000/memory` | 未使用 Sidecar 资源限制作用域 |
| 请求延迟增加 | Envoy 过载 | `kubectl exec -c istio-proxy -- curl localhost:15000/stats | grep overload` | QPS 超过 Envoy 容量 |
| 证书轮换风暴 | 大量 Pod 同时续签 | 监控 Istiod CA 负载 | 证书有效期过短 |

---

## 3.3 深度排查脚本集

### 3.3.1 Istio 健康检查脚本

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

```bash
#!/bin/bash
# 文件: istio-health-check.sh
# 用途: 全面检查 Istio 集群健康状态

set -e

NAMESPACE=${1:-istio-system}

echo "=== Istio Health Check ==="
echo "Namespace: $NAMESPACE"
echo

# 1. 检查控制平面状态
echo "--- Control Plane Status ---"
kubectl get pods -n $NAMESPACE -l app=istiod -o wide

ISTIOD_POD=$(kubectl get pod -n $NAMESPACE -l app=istiod -o jsonpath='{.items[0].metadata.name}')
echo "Istiod Pod: $ISTIOD_POD"

# 检查 Istiod Ready
READY=$(kubectl get pod -n $NAMESPACE $ISTIOD_POD -o jsonpath='{.status.containerStatuses[0].ready}')
if [ "$READY" != "true" ]; then
  echo "❌ Istiod is NOT Ready"
  kubectl logs -n $NAMESPACE $ISTIOD_POD --tail=50
  exit 1
else
  echo "✅ Istiod is Ready"
fi

# 2. 检查 Proxy 同步状态
echo -e "\n--- Proxy Sync Status ---"
istioctl proxy-status | grep -v SYNCED || echo "⚠️  Some proxies are not SYNCED"

# 3. 检查配置健康
echo -e "\n--- Configuration Analysis ---"
ISSUES=$(istioctl analyze -A 2>&1 | grep -c "Error" || echo "0")
if [ "$ISSUES" -gt 0 ]; then
  echo "❌ Found $ISSUES configuration errors:"
  istioctl analyze -A
else
  echo "✅ No configuration errors"
fi

# 4. 检查 mTLS 状态
echo -e "\n--- mTLS Status ---"
kubectl get peerauthentication -A -o custom-columns=\
NAMESPACE:.metadata.namespace,\
NAME:.metadata.name,\
MODE:.spec.mtls.mode

# 5. 检查 Gateway 健康
echo -e "\n--- Gateway Status ---"
kubectl get pods -n $NAMESPACE -l app=istio-ingressgateway -o wide

GATEWAY_POD=$(kubectl get pod -n $NAMESPACE -l app=istio-ingressgateway -o jsonpath='{.items[0].metadata.name}' 2>/dev/null)
if [ -n "$GATEWAY_POD" ]; then
  echo "Gateway Pod: $GATEWAY_POD"
  kubectl logs -n $NAMESPACE $GATEWAY_POD --tail=20 | grep -E "error|failed" || echo "No recent errors"
else
  echo "ℹ️  No Ingress Gateway found"
fi

# 6. 检查证书状态
echo -e "\n--- Certificate Status ---"
# 随机选择一个带 Sidecar 的 Pod
SAMPLE_POD=$(kubectl get pods -A -l security.istio.io/tlsMode=istio -o jsonpath='{.items[0].metadata.name}' -n $(kubectl get pods -A -l security.istio.io/tlsMode=istio -o jsonpath='{.items[0].metadata.namespace}') 2>/dev/null)
SAMPLE_NS=$(kubectl get pods -A -l security.istio.io/tlsMode=istio -o jsonpath='{.items[0].metadata.namespace}' 2>/dev/null)

if [ -n "$SAMPLE_POD" ]; then
  echo "Sample Pod: $SAMPLE_NS/$SAMPLE_POD"
  CERT_EXPIRY=$(kubectl exec -n $SAMPLE_NS $SAMPLE_POD -c istio-proxy -- \
    curl -s localhost:15000/certs | jq -r '.[0].expiration_time' 2>/dev/null)
  
  if [ -n "$CERT_EXPIRY" ]; then
    echo "Certificate expires: $CERT_EXPIRY"
    # 计算剩余时间
    EXPIRY_TS=$(date -d "$CERT_EXPIRY" +%s 2>/dev/null || echo "0")
    NOW_TS=$(date +%s)
    REMAINING_HOURS=$(( ($EXPIRY_TS - $NOW_TS) / 3600 ))
    
    if [ $REMAINING_HOURS -lt 1 ]; then
      echo "⚠️  Certificate expires in less than 1 hour!"
    else
      echo "✅ Certificate valid for $REMAINING_HOURS hours"
    fi
  fi
else
  echo "ℹ️  No Sidecar pods found for certificate check"
fi

# 7. 检查资源使用
echo -e "\n--- Resource Usage ---"
kubectl top pods -n $NAMESPACE

# 8. 性能指标
echo -e "\n--- Performance Metrics ---"
if [ -n "$ISTIOD_POD" ]; then
  echo "Istiod xDS connections:"
  kubectl exec -n $NAMESPACE $ISTIOD_POD -- \
    curl -s localhost:15014/metrics | grep "pilot_xds_pushes_total" | head -5
fi

echo -e "\n=== Health Check Complete ==="
```

### 3.3.2 Envoy 配置调试脚本

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

```bash
#!/bin/bash
# 文件: envoy-config-debug.sh
# 用途: 深度分析 Envoy 配置

POD_NAME=${1}
POD_NS=${2:-default}

echo "=== Envoy Configuration Debug ==="
echo "Pod: $POD_NS/$POD_NAME"
echo

# 1. 检查 Proxy 同步状态
echo "--- Proxy Status ---"
istioctl proxy-status | grep "$POD_NAME"

# 2. 获取 Listener 配置
echo -e "\n--- Listeners ---"
istioctl pc listener $POD_NAME -n $POD_NS -o json > /tmp/listeners.json
echo "Listeners saved to /tmp/listeners.json"
cat /tmp/listeners.json | jq -r '.[] | "\(.name): \(.address.socketAddress.portValue)"'

# 3. 获取 Route 配置
echo -e "\n--- Routes ---"
istioctl pc route $POD_NAME -n $POD_NS -o json > /tmp/routes.json
echo "Routes saved to /tmp/routes.json"
cat /tmp/routes.json | jq -r '.[].virtualHosts[].domains[]' | sort -u

# 4. 获取 Cluster 配置
echo -e "\n--- Clusters ---"
istioctl pc cluster $POD_NAME -n $POD_NS -o json > /tmp/clusters.json
echo "Clusters saved to /tmp/clusters.json"
cat /tmp/clusters.json | jq -r '.[] | "\(.name): \(.type)"' | head -20

# 5. 获取 Endpoint 配置
echo -e "\n--- Endpoints (sample) ---"
istioctl pc endpoint $POD_NAME -n $POD_NS -o json > /tmp/endpoints.json
echo "Endpoints saved to /tmp/endpoints.json"
cat /tmp/endpoints.json | jq -r '.[0:5][] | "\(.clusterName): \(.endpoint.address.socketAddress.address):\(.endpoint.address.socketAddress.portValue)"'

# 6. 检查熔断配置
echo -e "\n--- Circuit Breaker Status ---"
kubectl exec -n $POD_NS $POD_NAME -c istio-proxy -- \
  curl -s localhost:15000/clusters | grep "circuit_breakers" | head -10

# 7. 检查 Envoy 统计
echo -e "\n--- Envoy Stats (Top Metrics) ---"
kubectl exec -n $POD_NS $POD_NAME -c istio-proxy -- \
  curl -s localhost:15000/stats | grep -E "upstream_rq_|downstream_rq_" | head -20

# 8. 检查最近的配置推送
echo -e "\n--- Recent Config Updates ---"
kubectl exec -n $POD_NS $POD_NAME -c istio-proxy -- \
  curl -s localhost:15000/config_dump | \
  jq -r '.configs[] | select(.["@type"] | contains("Listener")) | .last_updated' | head -1

echo -e "\n=== Debug Complete ==="
echo "Full config dumps saved to /tmp/*.json"
```

### 3.3.3 流量追踪脚本

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

```bash
#!/bin/bash
# 文件: istio-traffic-trace.sh
# 用途: 追踪请求在 Service Mesh 中的完整路径

SOURCE_POD=${1}
SOURCE_NS=${2:-default}
TARGET_SVC=${3}
TARGET_NS=${4:-default}

echo "=== Istio Traffic Trace ==="
echo "Source: $SOURCE_NS/$SOURCE_POD"
echo "Target: $TARGET_NS/$TARGET_SVC"
echo

# 1. 检查源 Pod Sidecar 状态
echo "--- Source Pod Status ---"
kubectl get pod -n $SOURCE_NS $SOURCE_POD -o jsonpath='{.metadata.annotations.sidecar\.istio\.io/status}' | jq .

# 2. 获取目标 Service Cluster IP
TARGET_IP=$(kubectl get svc -n $TARGET_NS $TARGET_SVC -o jsonpath='{.spec.clusterIP}')
echo -e "\nTarget Service IP: $TARGET_IP"

# 3. 检查 Envoy Outbound Route
echo -e "\n--- Outbound Route Configuration ---"
istioctl pc route $SOURCE_POD -n $SOURCE_NS | grep -A5 "$TARGET_SVC"

# 4. 检查 Envoy Cluster 配置
echo -e "\n--- Cluster Configuration ---"
istioctl pc cluster $SOURCE_POD -n $SOURCE_NS | grep "$TARGET_SVC"

# 5. 检查 Endpoint 列表
echo -e "\n--- Available Endpoints ---"
istioctl pc endpoint $SOURCE_POD -n $SOURCE_NS | grep "$TARGET_SVC"

# 6. 执行实际请求并查看日志
echo -e "\n--- Executing Test Request ---"
kubectl exec -n $SOURCE_NS $SOURCE_POD -c istio-proxy -- \
  curl -v -s -o /dev/null http://$TARGET_SVC.$TARGET_NS:80/ 2>&1 | grep -E "HTTP/|< |>" | head -20

# 7. 查看 Envoy 访问日志
echo -e "\n--- Envoy Access Log (last 5 requests) ---"
kubectl logs -n $SOURCE_NS $SOURCE_POD -c istio-proxy --tail=5 | \
  jq -r 'select(.authority == "'$TARGET_SVC.$TARGET_NS'") | "\(.method) \(.path) → \(.response_code) (\(.response_flags))"'

# 8. 检查目标 Pod 的 Inbound 配置
echo -e "\n--- Target Pod Inbound Configuration ---"
TARGET_POD=$(kubectl get endpoints -n $TARGET_NS $TARGET_SVC -o jsonpath='{.subsets[0].addresses[0].targetRef.name}' 2>/dev/null)

if [ -n "$TARGET_POD" ]; then
  echo "Target Pod: $TARGET_POD"
  istioctl pc listener $TARGET_POD -n $TARGET_NS | grep "0.0.0.0_15006"
else
  echo "⚠️  No target pod found"
fi

echo -e "\n=== Trace Complete ==="
```

---

## 4.5 大规模集群性能优化

### 4.5.1 xDS 推送优化

**问题**: 5000+ Pod 集群中, Istiod CPU 100%, Config 推送延迟 > 60s

**优化方案**:

```yaml
# 1. 启用增量 xDS (Delta xDS)
apiVersion: install.istio.io/v1alpha1
kind: IstioOperator
spec:
  meshConfig:
    defaultConfig:
      proxyMetadata:
        ISTIO_DELTA_XDS: "true"
  # 结果: 只推送变更部分, 减少 90% 数据量

---
# 2. 使用 Sidecar 资源限制作用域
apiVersion: networking.istio.io/v1beta1
kind: Sidecar
metadata:
  name: default
  namespace: prod
spec:
  egress:
  - hosts:
    - "./*"  # 只接收本命名空间的 Service
    - "istio-system/*"  # 系统服务
    - "shared/*"  # 共享服务命名空间
  # 结果: 每个 Proxy 的配置量减少 80%

---
# 3. 调整 Istiod 资源
apiVersion: install.istio.io/v1alpha1
kind: IstioOperator
spec:
  components:
    pilot:
      k8s:
        resources:
          requests:
            cpu: 4000m
            memory: 8Gi
          limits:
            cpu: 8000m
            memory: 16Gi
        hpaSpec:
          minReplicas: 3
          maxReplicas: 10
          metrics:
          - type: Resource
            resource:
              name: cpu
              targetAverageUtilization: 70
```

**效果对比**:

| 指标 | 优化前 | 优化后 | 改善 |
|------|--------|--------|------|
| Istiod CPU | 95% | 45% | **2.1x** |
| Config 推送延迟 | 60s | 3s | **20x** |
| 每个 Proxy 配置大小 | 50MB | 5MB | **10x** |
| Proxy 内存占用 | 512MB | 128MB | **4x** |

### 4.5.2 Envoy 性能调优

```yaml
# 调整 Envoy 并发和资源限制
apiVersion: install.istio.io/v1alpha1
kind: IstioOperator
spec:
  meshConfig:
    defaultConfig:
      # 1. 并发工作线程 (默认 2)
      concurrency: 4  # 适用于 CPU > 2 核的节点
      
      # 2. 连接池配置
      connectionPool:
        tcp:
          maxConnections: 10000
        http:
          http1MaxPendingRequests: 10000
          http2MaxRequests: 10000
          maxRequestsPerConnection: 0  # 禁用 HTTP/1.1 keep-alive 限制
      
      # 3. 熔断配置
      outlierDetection:
        consecutiveErrors: 5
        interval: 10s
        baseEjectionTime: 30s
        maxEjectionPercent: 50
        minHealthPercent: 50
      
      # 4. 资源限制
      resources:
        requests:
          cpu: 500m
          memory: 512Mi
        limits:
          cpu: 2000m
          memory: 1Gi
```

### 4.5.3 mTLS 性能优化

```yaml
# 启用 TLS Offload (需要硬件支持)
apiVersion: install.istio.io/v1alpha1
kind: IstioOperator
spec:
  meshConfig:
    defaultConfig:
      proxyMetadata:
        # 使用 BoringSSL (比 OpenSSL 快 20%)
        TLS_PROVIDER: "boringssl"
        
        # 启用 TLS Session Resume
        TLS_SESSION_CACHE_SIZE: "10000"
  values:
    global:
      proxy:
        # 使用 AES-NI 硬件加速
        env:
          TLS_CIPHER_SUITES: "ECDHE-ECDSA-AES128-GCM-SHA256:ECDHE-RSA-AES128-GCM-SHA256"

# 证书轮换优化
---
apiVersion: install.istio.io/v1alpha1
kind: IstioOperator
spec:
  meshConfig:
    # 延长证书有效期 (减少轮换频率)
    defaultProviders:
      metrics:
      - name: workload-cert-ttl
        default: 72h  # 从 24h 延长到 72h
```

---

## 5.4 案例四: Istio 升级导致 Sidecar 批量重启

**问题背景**

- **集群**: GKE 1.29, 800 节点, 15000+ Pod
- **升级**: Istio 1.18 → 1.20
- **方式**: 使用 `istioctl upgrade` 一键升级

**问题过程**

```
时间线:
10:00 - 执行 istioctl upgrade
10:05 - Istiod 新版本部署完成
10:10 - 开始推送新版本 xDS 配置
10:15 - 发现大量 Envoy 重启 (OOMKilled)
10:20 - 集群范围服务中断 (50% Pod 不可用)
10:25 - 紧急回滚 Istio 1.18
10:40 - 服务逐步恢复
11:30 - 问题完全解决
```

**根因分析**

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

```bash
# 1. 检查 Envoy 重启原因
kubectl get pods -A -l security.istio.io/tlsMode=istio \
  -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{.status.containerStatuses[?(@.name=="istio-proxy")].lastState.terminated.reason}{"\n"}{end}' | \
  grep -v "^$"

# 输出: 大量 "OOMKilled"

# 2. 检查 Envoy 内存使用
kubectl top pod --containers -A | grep istio-proxy | sort -k4 -rn | head -20
# 输出: 内存使用从 200MB 飙升到 1.5GB

# 3. 查看 xDS 配置大小
kubectl exec -n prod <pod> -c istio-proxy -- \
  curl -s localhost:15000/config_dump | wc -c
# 输出: 52MB (Istio 1.18: 8MB)

# 4. 分析配置差异
istioctl pc cluster <pod> --fqdn '*' | wc -l
# Istio 1.18: 1500 clusters
# Istio 1.20: 8000 clusters (5.3x 增长!)

# 5. 发现根因: 新版本默认行为变更
# Istio 1.20 默认启用了"全局服务发现"
# 每个 Sidecar 接收所有命名空间的 Service 配置
```

**修复方案**

```yaml
# 方案: 使用 Sidecar 资源限制作用域

# 1. 创建全局 Sidecar 配置
apiVersion: networking.istio.io/v1beta1
kind: Sidecar
metadata:
  name: default
  namespace: istio-config  # 特殊命名空间
spec:
  egress:
  - hosts:
    - "./*"  # 本命名空间
    - "istio-system/*"  # 系统服务
    - "kube-system/*"  # 系统服务
  # 不包括其他命名空间, 减少配置量

# 2. 为需要跨命名空间通信的服务添加例外
---
apiVersion: networking.istio.io/v1beta1
kind: Sidecar
metadata:
  name: frontend-sidecar
  namespace: prod
spec:
  workloadSelector:
    labels:
      app: frontend
  egress:
  - hosts:
    - "prod/*"  # 本命名空间
    - "shared/*"  # 共享服务
    - "payment/*"  # 支付服务命名空间
    - "istio-system/*"

# 3. 调整 Envoy 资源限制
---
apiVersion: install.istio.io/v1alpha1
kind: IstioOperator
spec:
  values:
    global:
      proxy:
        resources:
          requests:
            cpu: 500m
            memory: 512Mi
          limits:
            cpu: 2000m
            memory: 2Gi  # 从 1Gi 提升到 2Gi

# 4. 渐进式升级策略
# 不使用 istioctl upgrade, 改用金丝雀部署

# 4.1 部署新版本 Istiod (不删除旧版本)
kubectl label namespace prod istio-injection-
kubectl label namespace prod istio.io/rev=1-20

# 4.2 应用 Sidecar 配置
kubectl apply -f sidecar-global.yaml

# 4.3 选择 5% Pod 重启 (测试新版本)
kubectl rollout restart deployment/canary-app -n prod

# 4.4 监控 24 小时
# - 内存使用: <1GB ✅
# - 重启次数: 0 ✅
# - 服务可用性: 99.99% ✅

# 4.5 逐步扩展到全部 Pod
for ns in $(kubectl get ns -l istio-injection=enabled -o name | cut -d/ -f2); do
  kubectl label namespace $ns istio-injection-
  kubectl label namespace $ns istio.io/rev=1-20
  kubectl rollout restart deployment -n $ns
  sleep 300  # 每个命名空间间隔 5 分钟
done
```

**防护措施**

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

```bash
# 1. 升级前配置审计
istioctl experimental precheck

# 2. 升级前性能基准测试
# 在测试集群验证新版本配置大小
kubectl exec -n test <pod> -c istio-proxy -- \
  curl -s localhost:15000/memory | jq '.["total_allocated_bytes"]'

# 3. 使用金丝雀升级
apiVersion: install.istio.io/v1alpha1
kind: IstioOperator
metadata:
  name: control-plane-1-20
spec:
  revision: 1-20  # 使用 revision 而非覆盖
  ...

# 4. 配置资源监控告警
# Prometheus AlertRule
- alert: EnvoyHighMemory
  expr: container_memory_usage_bytes{container="istio-proxy"} > 1073741824
  for: 5m
  annotations:
    summary: "Envoy memory usage > 1GB"

# 5. 定期清理无用配置
istioctl pc cluster <pod> --fqdn '*' | grep -E "BlackHoleCluster|PassthroughCluster" | wc -l
```

**业务影响**

- **影响时间**: 30 分钟
- **影响范围**: 50% Pod OOMKilled 重启
- **服务可用性**: 降至 70% (P99 延迟 10s+)
- **损失**: 约 500 笔交易失败

---

## 附录: Istio 专家巡检清单 (扩展版)

### 每日自动化巡检

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

```bash
#!/bin/bash
# 文件: istio-daily-check.sh

echo "=== Istio Daily Health Check ==="
date
echo

# 1. 控制平面健康
echo "--- Control Plane ---"
kubectl get pods -n istio-system -o wide
kubectl top pods -n istio-system

# 2. Proxy 同步状态
echo -e "\n--- Proxy Sync Status ---"
STALE_COUNT=$(istioctl proxy-status | grep -c "STALE" || echo "0")
echo "STALE proxies: $STALE_COUNT"
if [ "$STALE_COUNT" -gt 10 ]; then
  echo "⚠️  Too many STALE proxies"
fi

# 3. 配置错误
echo -e "\n--- Configuration Errors ---"
istioctl analyze -A --suppress "IST0102,IST0103"

# 4. Certificate 有效期
echo -e "\n--- Certificate Expiry ---"
# 采样检查
for ns in $(kubectl get ns -l istio-injection=enabled -o jsonpath='{.items[*].metadata.name}' | tr ' ' '\n' | head -5); do
  POD=$(kubectl get pod -n $ns -l security.istio.io/tlsMode=istio -o jsonpath='{.items[0].metadata.name}' 2>/dev/null)
  if [ -n "$POD" ]; then
    EXPIRY=$(kubectl exec -n $ns $POD -c istio-proxy -- curl -s localhost:15000/certs 2>/dev/null | jq -r '.[0].expiration_time')
    echo "$ns/$POD: $EXPIRY"
  fi
done

# 5. Gateway 错误率
echo -e "\n--- Gateway Error Rate ---"
kubectl logs -n istio-system -l app=istio-ingressgateway --tail=1000 | \
  grep -oP '"response_code":\K\d+' | \
  awk '{count[$1]++} END {for (code in count) print code ": " count[code]}' | \
  sort -t: -k2 -rn

echo -e "\n=== Check Complete ==="
```

### 每周手动巡检

- [ ] **配置审计**: 导出所有 VirtualService/DestinationRule, 检查过期规则
- [ ] **性能基准**: 对比 xDS 推送延迟、Envoy CPU/内存趋势
- [ ] **证书管理**: 确认 Root CA 有效期 (10年), Workload Cert 轮换正常
- [ ] **Gateway 容量**: 验证 Ingress Gateway HPA 配置, 压测 QPS 上限
- [ ] **Sidecar 作用域**: 检查是否有 Pod 接收过多配置 (>20MB)
- [ ] **mTLS 覆盖率**: 确认关键服务已启用 STRICT 模式
- [ ] **升级计划**: 检查 Istio 新版本, 规划升级窗口

---

## 6. Terway + ASM (阿里云 ACK) 交互问题场景

> **适用集群**: 阿里云 ACK + Terway 网络模式 + ASM (Alibaba Service Mesh)
> **难度**: 高级
> **最后更新**: 2026-05

### 6.1 Terway ENI 模式与 Istio Sidecar 流量劫持异常

#### 问题现象

| 现象 | 报错信息 | 影响 |
|------|----------|------|
| Pod 内流量全部走 istio-proxy 但延迟极高 | Envoy 内部 `Connection reset` | 业务请求超时 |
| Terway ENI 模式下 sidecar 无法劫持流量 | `iptables: No chain/target` | 服务网格功能失效 |
| Pod IP 无法被 Envoy 健康检查 | `health check failed` | Endpoints 被剔除 |

#### 根因分析

Terway ENI 模式使用阿里云弹性网卡（ENI）直接挂载到 Pod，网络流量不经过节点主网卡。当 Istio 的 iptables 规则尝试劫持流量时：
- ENI 模式的 veth pair 命名方式与标准 CNI 不同
- `istio-init` 容器初始化脚本无法找到正确的网卡接口
- 流量被直接路由到 ENI，绕过了 Envoy sidecar

#### 排查步骤

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

```bash
# Step 1: 确认 Pod 使用 Terway ENI 模式
kubectl get pod {pod-name} -n {namespace} -o jsonpath='{.metadata.annotations.k8s\.aliyun\.com/eni-mode}'

# Step 2: 检查 Pod 的网络接口
kubectl exec -it {pod-name} -n {namespace} -c istio-proxy -- ip addr

# Step 3: 对比 istio-proxy 和主容器的网络命名
kubectl exec -it {pod-name} -n {namespace} -- sh -c 'ip link show'

# Step 4: 检查 iptables 规则是否正确
kubectl exec -it {pod-name} -n {namespace} -c istio-proxy -- iptables -L -t nat | grep ISTIO
```

#### 解决方案

**方案 A: 启用 Istio eBPF 模式（推荐）**

Istio 1.18+ 支持 eBPF 模式，可以绕过 iptables 直接劫持流量：

```yaml
# 在 IstioOperator 中启用 eBPF
apiVersion: install.istio.io/v1alpha1
kind: IstioOperator
metadata:
  name: istio-ebpf
spec:
  profile: default
  components:
    ingressGateways:
      - name: istio-ingressgateway
        enabled: true
  values:
    global:
      meshConfig:
        enablePrometheusMerge: true
    pilot:
      env:
        # 启用 eBPF 模式
        PILOT_ENABLE_EBPF: "true"
```

**方案 B: 降级到 Veth 模式**

如果集群支持，可以在 Pod annotation 中指定使用 Veth 模式：

```yaml
apiVersion: v1
kind: Pod
metadata:
  annotations:
    k8s.aliyun.com/eni-mode: "false"  # 禁用 ENI 模式，使用 Veth
spec:
```

### 6.2 Terway VPC 路由与 Istio Ingress Gateway 冲突

#### 问题现象

| 现象 | 报错信息 | 影响 |
|------|----------|------|
| Istio Ingress Gateway 无法接收外部流量 | `connection refused` | 外部请求全部 502 |
| VPC 路由表显示正常但流量不达 | Pod ENI 直接绑定 EIP | 无法通过 Gateway 路由 |
| Gateway Pod 显示 Running 但无法访问 | CLB 健康检查超时 | CLB 后端无响应 |

#### 根因分析

Terway ENI 模式下的 Pod 可以直接绑定 EIP（阿里云弹性公网 IP），流量绕过 Istio Ingress Gateway。当 Ingress Gateway 的 CLB 配置了 Pod 后端时：
- CLB 直接指向 Pod ENI IP
- 流量不经过 Istio sidecar，无法被服务网格策略拦截
- mTLS 双向认证失败

#### 排查步骤

```bash
# Step 1: 检查 Pod 是否有 EIP 直接绑定
kubectl get pod {pod-name} -n {namespace} -o jsonpath='{.metadata.annotations.k8s\.aliyun\.com/eip}'

# Step 2: 检查 Istio Ingress Gateway 的 CLB 配置
aliyun slb describeloadbalancer --region {region} --loadbalancer-id {lb-id}

# Step 3: 检查 VPC 路由表
aliyun vpc describeRouteEntries --VpcId {vpc-id} --RouteTableId {rt-id}

# Step 4: 查看 Ingress Gateway 日志
kubectl logs -n istio-system -l app=istio-ingressgateway --tail=50 | grep -i "eip|eni"
```

#### 解决方案

**方案 A: 移除 Pod EIP 绑定，统一通过 Ingress Gateway 入口**

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl label/annotate`：改元数据可能影响选择器/控制器

```bash
# 移除 Pod 的 EIP 直接绑定
kubectl annotate pod {pod-name} -n {namespace} k8s.aliyun.com/eip-

# 确认 Ingress Gateway CLB 后端已更新
aliyun slb describebackendservers --region {region} --loadbalancer-id {lb-id}
```

**方案 B: 配置 Service 对齐到 Gateway**

确保应用 Service 类型为 ClusterIP/NodePort，由 Istio Ingress Gateway 统一处理入口流量：

```yaml
apiVersion: v1
kind: Service
metadata:
  name: {app-svc}
  annotations:
    # 明确指定通过 Ingress Gateway 接入
    istio.io/ingress: "true"
spec:
  type: ClusterIP
  ports:
    - port: 8080
      targetPort: 8080
```

### 6.3 Terway IPVLAN 模式与 Envoy XDP Offload 兼容性

#### 问题现象

| 现象 | 报错信息 | 影响 |
|------|----------|------|
| Terway IPVLAN 模式下 Envoy XDP 报 `No such device` | XDP offload 失败 | 性能无法优化 |
| 高吞吐场景下 CPU 使用率异常高 | iptables 软中断瓶颈 | 网络延迟增加 |
| Pod 网络延迟抖动但指标正常 | 内核 IPVlan L2 模式冲突 | 服务间通信不稳定 |

#### 根因分析

Terway IPVLAN 模式使用内核 IPVLAN 驱动，在 L2 或 L3 模式下工作。Istio 的 XDP offload（通过 eBPF）需要直接访问网络设备，但：
- IPVLAN 创建的是虚拟接口，不是标准 eth0
- eBPF 程序无法 attach 到 IPVLAN 接口
- 需要内核版本 >= 5.10 且支持 netlink

#### 排查步骤

```bash
# Step 1: 检查 Pod 使用 IPVLAN 模式
kubectl get pod {pod-name} -n {namespace} -o jsonpath='{.metadata.annotations.k8s\.aliyun\.com/network-mode}'
# 期望输出: ipvlan

# Step 2: 检查内核版本
uname -r
# 要求 >= 5.10

# Step 3: 检查 IPVLAN 接口状态
ip link show | grep ipvlan

# Step 4: 检查 eBPF/XDP 可用性
bpftool net show
cat /proc/sys/net/core/bpf_jit_enable
```

#### 解决方案

**方案 A: 降级到标准 Veth 模式**

如果 XDP offload 是关键性能优化点，可以降级 Pod 网络模式：

```yaml
apiVersion: v1
kind: Pod
metadata:
  annotations:
    k8s.aliyun.com/network-mode: "veth"
spec:
```

**方案 B: 禁用 XDP offload，使用 iptables**

在 Istio 配置中禁用 XDP：

```yaml
apiVersion: install.istio.io/v1alpha1
kind: IstioOperator
metadata:
  name: istio-no-xdp
spec:
  profile: default
  values:
    pilot:
      env:
        PILOT_ENABLE_XDP_OFFLOAD: "false"
```

### 6.4 Terway + ASM 问题快速检测命令

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

```bash
#!/bin/bash
# Terway + ASM 交互问题快速检测

echo "=== Terway + ASM 交互故障检测 ==="

# 检测 1: Pod ENI 模式检查
echo -e "\n--- Pod ENI Mode ---"
kubectl get pods -n istio-system -l app=istio-ingressgateway -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{.metadata.annotations.k8s\.aliyun\.com/eni-mode}{"\n"}{end}'

# 检测 2: Istio sidecar 流量劫持检查
echo -e "\n--- Sidecar iptables Rules ---"
for pod in $(kubectl get pods -n default -o jsonpath='{.items[*].metadata.name}'); do
  if kubectl exec -it $pod -c istio-proxy -- iptables -L -t nat 2>/dev/null | grep -q ISTIO; then
    echo "$pod: iptables ISTIO rules present"
  else
    echo "$pod: WARNING - No ISTIO iptables rules found"
  fi
done

# 检测 3: EIP 直接绑定检查
echo -e "\n--- Pod EIP Binding ---"
kubectl get pods -n default -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{.metadata.annotations.k8s\.aliyun\.com/eip}{"\n"}{end}' | grep -v "<no value>"

# 检测 4: IPVLAN 模式检查
echo -e "\n--- Pod IPVLAN Mode ---"
kubectl get pods -n default -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{.metadata.annotations.k8s\.aliyun\.com/network-mode}{"\n"}{end}' | grep ipvlan

# 检测 5: Envoy XDP 可用性检查
echo -e "\n--- Envoy XDP Status ---"
kubectl exec -it istiod-0 -n istio-system -- pilot-agent status 2>/dev/null | grep -i xdp || echo "XDP status unknown"

echo -e "\n=== 检测完成 ==="
```

---

## 7. 多集群服务网格

### 7.1 跨集群流量管理

```yaml
multicluster_mesh:
  # 集群联邦配置
  federation:
    primary_cluster: "prod-cluster-1"
    secondary_clusters:
      - name: "prod-cluster-2"
        failover_priority: 1
      - name: "prod-cluster-3"
        failover_priority: 2

    # 跨集群服务发现
    service_discovery:
      method: "Primary-Cluster DNS"
      replication_delay: 5s
      failover_timeout: 30s

  # 跨集群流量策略
  traffic_policy:
    locality_lb:
      enabled: true
      failover:
        - from: "zone-1"
          to: "zone-2"
    outlier_detection:
      consecutive_5xx: 5
      interval: 10s
      base_ejection_time: 30s
```

### 7.2 跨集群故障转移

```yaml
cross_cluster_failover:
  # 自动故障转移配置
  auto_failover:
    enabled: true
    health_check_interval: 10s
    failure_threshold: 3
    recovery_threshold: 2

  # 故障转移流程
  failover_steps:
    - name: "检测主集群不可用"
      condition: "3 次健康检查失败"
      action: "触发故障转移"

    - name: "更新 DNS 路由"
      action: |
        # 通过阿里云 DNS API 更新解析
        aliyun alidns UpdateDomainRecord \
          --RecordId {record_id} \
          --Value new_cluster_ip

    - name: "通知备用集群"
      action: |
        kubectl label namespace {ns} \
          istio-injection=enabled \
          cluster=failover-target

    - name: "验证流量切换"
      action: |
        kubectl exec -it {test-pod} -- \
          curl -s {service}.{namespace}.svc.cluster.local

  # 回滚流程
  rollback:
    enabled: true
    manual_approval_required: true
    steps:
      - "确认主集群恢复"
      - "等待流量稳定"
      - "切换回主集群"
```

---

## 8. 生产可观测性

### 8.1 服务网格指标体系

```yaml
mesh_observability:
  # 关键指标
  metrics:
    # 流量指标
    - name: "istio_requests_total"
      type: "counter"
      labels: ["destination_service", "response_code"]
      slo: "成功率 >= 99.9%"

    - name: "istio_request_duration_seconds"
      type: "histogram"
      labels: ["destination_service"]
      slo: "P99 < 500ms"

    # 副作用car代理指标
    - name: "istio_proxy_cpu_seconds_total"
      type: "counter"
      labels: ["pod_name"]
      slo: "CPU < 1 core"

    - name: "istio_proxy_memory_bytes"
      type: "gauge"
      labels: ["pod_name"]
      slo: "Memory < 512MB"

    # 控制面指标
    - name: "istiod_xds_push_duration_seconds"
      type: "histogram"
      labels: ["component"]
      slo: "P99 < 1s"

    - name: "istiod_conflict_config_total"
      type: "counter"
      labels: ["type"]
      alert_threshold: "> 0"

  # SLO 告警规则
  alert_rules:
    - name: "Request Success Rate Low"
      severity: P1
      condition: "rate(istio_requests_total{response_code=~'5..'}) / rate(istio_requests_total) > 0.001"
      channels: ["pagerduty", "slack-mesh-alerts"]

    - name: "Proxy Memory High"
      severity: P2
      condition: "istio_proxy_memory_bytes > 536870912"  # 512MB
      channels: ["slack-mesh-alerts"]

    - name: "XDS Push Slow"
      severity: P2
      condition: "histogram_quantile(0.99, istiod_xds_push_duration_seconds) > 1"
      channels: ["slack-mesh-alerts"]
```

### 8.2 分布式追踪集成

```yaml
distributed_tracing:
  # Jaeger 配置
  jaeger:
    enabled: true
    sampling:
      type: "probabilistic"
      rate: 0.1  # 10% 采样
      min_rate: 100  # 最低每秒 100 个采样

  # B3 传播头配置
  propagation:
    headers:
      - "x-b3-traceid"
      - "x-b3-spanid"
      - "x-b3-parentspanid"
      - "x-b3-sampled"

  # 追踪上下文注入
  context_propagation:
    injection_points:
      - "application"
      - "istio-proxy"
      - "gateway"
    baggage_headers:
      - "x-ot-span-context"
      - "x-request-id"
```

### 8.3 服务拓扑图

```yaml
service_topology:
  # 自动生成拓扑图
  kiali_integration:
    enabled: true
    refresh_interval: 30s
    graph_depth: 3

  # 服务依赖分析
  dependency_analysis:
    enabled: true
    interval: 5m
    output: "service-dependencies.json"

  # 健康度评分
  health_score:
    calculation: "weighted_average"
    weights:
      request_success_rate: 0.4
      latency_p99: 0.3
      proxy_resource_usage: 0.2
      config_sync_status: 0.1
    thresholds:
      healthy: "> 0.9"
      degraded: "0.7-0.9"
      unhealthy: "< 0.7"
```

---

## 9. 安全加固

### 9.1 mTLS 与证书管理

```yaml
security_hardening:
  # mTLS 配置
  mtls:
    mode: "STRICT"  # 生产环境必须 STRICT
    auto_rotation: true
    rotation_interval: 24h
    grace_period: 1h

  # 证书颁发者配置
  certificate_authority:
    type: "Istiod"  # 使用 Istiod 内置 CA
    root_cert_rotation: 365d
    workload_cert_ttl: 24h

  # SPIFFE 身份配置
  spiffe:
    trust_domain: "cluster.local"
    workload_selector:
      method: "namespace"  # 按命名空间选择
      namespaces: ["prod", "core"]

  # AuthorizationPolicy 示例
  authorization_policy:
    # 默认拒绝
    default: "deny-all"
    # 允许特定流量
    rules:
      - from:
          - source:
              principals: ["cluster.local/ns/prod/sa/gateway"]
        to:
          - operation:
              ports: ["8080", "8443"]
      - from:
          - source:
              principals: ["cluster.local/ns/prod/sa/*"]
        to:
          - operation:
              ports: ["8080"]
```

### 9.2 网络策略

```yaml
network_policies:
  # 控制面命名空间保护
  control_plane:
    ingress:
      - from:
          - namespaceSelector:
              matchLabels:
                name: "kube-system"
        ports:
          - protocol: TCP
            port: 15012  # istiod 端口
          - protocol: TCP
            port: 15014  # istiod metrics
    egress:
      - to:
          - podSelector:
              matchLabels:
                app: "istiod"

  # 数据面命名空间隔离
  data_plane:
    strict_isolation:
      enabled: true
      require_mtls: true
      namespace_isolation:
        - name: "prod"
          allowed_egress:
            - "*.svc.cluster.local"
            - "metrics-server.kube-system.svc.cluster.local"
          allowed_ingress: []  # 默认无允许

  # 应用命名空间策略
  application_ns:
    name: "prod"
    ingress_from_gateway:
      - from:
          - namespaceSelector:
              matchLabels:
                name: "istio-system"
            port: 8080
    egress_to_external:
      enabled: false  # 默认禁止访问外部
```

### 9.3 审计日志

```yaml
audit_logging:
  # 记录的操作
  operations:
    - "AuthorizationPolicy created/updated/deleted"
    - "PeerAuthentication created/updated/deleted"
    - "RequestAuthentication created/updated/deleted"
    - "Sidecar created/updated/deleted"
    - "DestinationRule created/updated/deleted"

  # 日志格式
  log_format:
    timestamp: ISO8601
    operation: string
    resource_type: string
    resource_name: string
    namespace: string
    actor: string  # user or system
    result: "success|failure"
    change_detail: object

  # 告警规则
  security_alerts:
    - name: "AuthorizationPolicy Allow-All"
      severity: P1
      condition: 'rule[*].from[0].source.namespaces[0] == "*"'
      action: "通知安全团队"

    - name: "PeerAuthentication PERMISSIVE"
      severity: P1
      condition: 'mtls.mode == "PERMISSIVE"'
      action: "要求切换 STRICT"
```

---

## 10. 成本优化

### 10.1 Envoy 资源优化

```yaml
envoy_optimization:
  # 资源限制
  resource_limits:
    cpu_limit: "500m"
    memory_limit: "512Mi"
    cpu_request: "100m"
    memory_request: "128Mi"

  # 性能调优
  performance_tuning:
    # 连接池配置
    http2_settings:
      max_concurrent_streams: 100
      initial_window_size: 65536
      connection_window_size: 1048576

    # 缓冲区配置
    upstream_buffer: 8MB
    downstream_buffer: 8MB

    # 追踪采样优化
    tracing:
      sampling_rate: 0.1  # 降低采样率
      min_sampling_rate: 100
```

### 10.2 控制面扩展性

```yaml
control_plane_scaling:
  # istiod 扩展配置
  istiod:
    replicas: 2  # 生产至少 2 个
    hpa:
      min_replicas: 2
      max_replicas: 5
      target_cpu: 70

  # xDS 推送优化
  xds_optimization:
    debounce_duration: 100ms
    node_cache_duration: 5m
    fetch_debounce: 250ms
    enable_eds_caching: true

  # Gateway 扩展配置
  ingress_gateway:
    replicas: 3
    hpa:
      min_replicas: 2
      max_replicas: 10
      target_cpu: 80
    resources:
      cpu_limit: "2"
      memory_limit: "2Gi"
```

### 10.3 成本监控与告警

```yaml
cost_monitoring:
  # 成本指标
  metrics:
    - name: "mesh_proxy_cpu_cost"
      calculation: "sum(envoy_cpu_seconds) * cpu_cost_per_core_hour"
      unit: "USD/hour"

    - name: "mesh_proxy_memory_cost"
      calculation: "sum(envoy_memory_bytes) * memory_cost_per_gb_hour"
      unit: "USD/hour"

    - name: "mesh_total_daily_cost"
      calculation: "sum(mesh_proxy_*_cost) * 24"
      unit: "USD/day"

  # 成本告警
  cost_alerts:
    - name: "Daily Cost Spike"
      severity: P2
      condition: "mesh_total_daily_cost > daily_average * 1.5"
      channels: ["slack-cost-alerts"]
```

---

## 11. 升级策略与回滚

### 11.1 升级流程

```yaml
upgrade_procedure:
  # 升级前检查
  pre_upgrade_check:
    - "确认备份已成功"
    - "检查配置兼容性"
    - "验证支持矩阵"
    - "准备回滚方案"

  # 金丝雀升级
  canary_upgrade:
    strategy: "Istio revision + Traffic Splitting"
    steps:
      - name: "安装新版本 Istiod"
        command: "istioctl install --set revision={new_version}"

      - name: "迁移单个命名空间"
        command: |
          kubectl label namespace {namespace} \
            istio.io/rev={new_version}

      - name: "验证流量正常"
        command: |
          # 检查错误率
          kubectl exec -it {test-pod} -- \
            curl -s {service}:{port}/health

      - name: "切换流量 10%"
        command: |
          kubectl apply -f - <<EOF
          apiVersion: networking.istio.io/v1alpha3
          kind: VirtualService
          metadata:
            name: {service}-canary
          spec:
            hosts:
            - {service}
            http:
            - route:
              - destination:
                  host: {service}
                  subset: stable
                weight: 90
              - destination:
                  host: {service}
                  subset: canary
                weight: 10
          EOF

      - name: "逐步增加流量"
        weights: ["10%", "30%", "50%", "100%"]
        interval: 30m
        verification: "检查错误率和延迟"

  # 回滚方案
  rollback:
    command: |
      # 回滚到旧版本
      kubectl label namespace {namespace} \
        istio.io/rev={old_version}

      # 删除新版本
      istioctl uninstall --set revision={new_version}
```

### 11.2 升级检查清单

```yaml
upgrade_checklist:
  pre_upgrade:
    - [ ] "阅读升级注意事项 ( Release Notes)"
    - [ ] "确认 etcd 快照已成功"
    - [ ] "确认 Velero 备份已完成"
    - [ ] "检查应用兼容性 (Envoy API 版本)"
    - [ ] "准备回滚验证测试"
    - [ ] "通知相关团队升级窗口"

  during_upgrade:
    - [ ] "记录升级开始时间"
    - [ ] "按命名空间逐步迁移"
    - [ ] "监控错误率变化"
    - [ ] "监控延迟变化"
    - [ ] "记录发现的问题"

  post_upgrade:
    - [ ] "确认所有命名空间已迁移"
    - [ ] "删除旧版本 Istio"
    - [ ] "运行完整回归测试"
    - [ ] "更新监控仪表板"
    - [ ] "更新文档"
    - [ ] "通知团队升级完成"
```

---

## 12. 性能基准测试

### 12.1 基准测试套件

```yaml
benchmark_suite:
  # 测试场景
  test_scenarios:
    - name: "P99 延迟基准"
      test_type: "latency"
      command: |
        fortio load -H "Host: {service}" \
          http://{gateway}:80/{path} \
          -c 50 -n 10000 -qps 1000
      thresholds:
        p50: < 10ms
        p95: < 50ms
        p99: < 100ms

    - name: "吞吐量基准"
      test_type: "throughput"
      command: |
        fortio load -H "Host: {service}" \
          http://{gateway}:80/{path} \
          -c 100 -n 100000 -qps 0
      thresholds:
        max_rps: "> 10000"
        error_rate: "< 0.01%"

    - name: "长连接基准"
      test_type: "connection"
      command: |
        fortio load -H "Host: {service}" \
          http://{gateway}:80/{path} \
          -c 10 -n 0 -duration 5m -keepalive
      thresholds:
        connection_stability: "99.99%"
        latency_p99: "< 200ms"

  # 定期执行
  schedule:
    baseline: "weekly"
    regression: "pre-release"
    capacity_planning: "monthly"

  # 结果存储
  results:
    storage: "prometheus + Grafana"
    retention: 90d
    trend_analysis: true
```

---

## 13. 日常运维脚本

### 13.1 自动化巡检脚本

```bash
#!/bin/bash
# Istio 服务网格每日巡检

set -e

NAMESPACE="istio-system"
ALERT_THRESHOLD=0.9

echo "=== Istio 服务网格每日巡检 ==="
echo "时间: $(date -u +%Y-%m-%dT%H:%M:%SZ)"

# 1. 控制面健康检查
echo -e "\n--- 1. 控制面健康 ---"
kubectl get pods -n $NAMESPACE -l app=istiod
ISTIOD_READY=$(kubectl get pods -n $NAMESPACE -l app=istiod -o jsonpath='{.items[*].status.conditions[?(@.type=="Ready")].status}')
if [ "$ISTIOD_READY" != "TrueTrue" ]; then
  echo "ALERT: Istiod Pod 不健康"
fi

# 2. Gateway 健康检查
echo -e "\n--- 2. Gateway 健康 ---"
kubectl get pods -n $NAMESPACE -l app=istio-ingressgateway
GATEWAY_READY=$(kubectl get pods -n $NAMESPACE -l app=istio-ingressgateway -o jsonpath='{.items[*].status.conditions[?(@.type=="Ready")].status}')
if [ "$GATEWAY_READY" != "True" ]; then
  echo "ALERT: Gateway Pod 不健康"
fi

# 3. mTLS 模式检查
echo -e "\n--- 3. mTLS 配置 ---"
PERMISSIVE_NS=$(kubectl get peerauthentication -A -o jsonpath='{.items[?(@.spec.mtls.mode=="PERMISSIVE")].metadata.namespace}' || echo "")
if [ -n "$PERMISSIVE_NS" ]; then
  echo "WARNING: 以下命名空间使用 PERMISSIVE 模式: $PERMISSIVE_NS"
fi

# 4. xDS 同步状态
echo -e "\n--- 4. xDS 同步状态 ---"
STALE_PROXIES=$(istioctl proxy-status 2>/dev/null | grep -c "STALE" || echo "0")
if [ "$STALE_PROXIES" -gt 0 ]; then
  echo "ALERT: $STALE_PROXIES 个代理配置 STALE"
fi

# 5. 资源使用检查
echo -e "\n--- 5. 资源使用 ---"
kubectl top pods -n $NAMESPACE --no-headers 2>/dev/null || echo "Metrics Server 不可用"

# 6. 证书过期检查
echo -e "\n--- 6. 证书过期检查 ---"
kubectl get secret -n $NAMESPACE -l istio.io/canonical-app=istiod \
  -o jsonpath='{.items[*]}' | jq -r '.[] | select(.type=="kubernetes.io/tls") | .metadata.name' | while read secret; do
  EXPIRE_DATE=$(kubectl get secret $secret -n $NAMESPACE -o jsonpath='{.data.cert\.pem}' | base64 -d | openssl x509 -enddate -noout | cut -d= -f2)
  echo "$secret: 到期 $EXPIRE_DATE"
done

# 7. 配置冲突检查
echo -e "\n--- 7. 配置冲突检查 ---"
CONFLICTS=$(istioctl analyze -n $NAMESPACE 2>/dev/null | grep -c "Conflict" || echo "0")
echo "配置冲突数: $CONFLICTS"

echo -e "\n=== 巡检完成 ==="
```

---

## 14. 生产问题案例库

### 14.1 高频问题速查

| 问题 | 快速诊断 | 解决方案 |
|------|----------|----------|
| 503 UH | Endpoints 为空 | 检查 Pod 就绪状态 |
| 503 UF | mTLS 握手失败 | 检查证书有效期 |
| 403 RBAC | AuthorizationPolicy 拒绝 | 检查策略配置 |
| 404 NR | 路由未配置 | 检查 VirtualService |
| 500 DC | 上游连接重置 | 检查 Pod 健康状态 |
| 超时 | 重试配置不当 | 增加超时时间 |

### 14.2 疑难问题深度分析

```yaml
# 问题: xDS 配置延迟导致偶发性 503
symptom: "偶发性 503，持续 < 1s"
root_cause: |
  大规模集群中，istiod xDS 推送存在抖动。
  当 Endpoint 数量 > 10000 时，推送延迟 P99 > 5s。
diagnosis:
  - "istioctl proxy-status 检查 STALE 状态"
  - "观察 istiod 日志: kubectl logs -n istio-system istiod-* | grep 'push'
  - "检查 istiod 资源使用"
solution: |
  1. 启用 EDS 分量推送:
     meshConfig:
       enableEdsCaching: true
       defaultInvocationInterval: 10s
  2. 增加 istiod 资源:
     resources:
       requests:
         cpu: "500m"
         memory: "1Gi"
  3. 考虑启用 Ambient Mesh 模式
verification: |
  持续监控 24h 无偶发性 503
```

---

> **版本**: v2.0
> **维护团队**: SRE Team / Platform Team
> **更新日期**: 2026-05-19
> **新增章节**:
> - [x] 多集群服务网格 (跨集群流量管理、故障转移)
> - [x] 生产可观测性 (SLO/SLA、指标体系、分布式追踪)
> - [x] 安全加固 (mTLS、NetworkPolicy、审计日志)
> - [x] 成本优化 (Envoy 资源、控制面扩展)
> - [x] 升级策略与回滚 (金丝雀升级、检查清单)
> - [x] 性能基准测试 (Fortio 基准测试套件)
> - [x] 日常运维脚本 (自动化巡检脚本)
> - [x] 生产问题案例库 (高频问题、疑难问题)

## Related

- 08-docker-troubleshooting-guide
- 16-troubleshooting-guide
- [[domain-17-system-foundation/topic-cheat-sheet/go.md|go]]
- [[domain-17-system-foundation/topic-cheat-sheet/helm.md|helm]]
- [[domain-17-system-foundation/topic-cheat-sheet/k8s.md|k8s]]
- [[domain-19-landscape-references/topic-index/service-mesh-index.md|Service Mesh 服务网格知识图谱索引]]
- [[domain-19-landscape-references/topic-index/dns-index.md|DNS 知识图谱索引]]
- [[domain-19-landscape-references/topic-index/nginx-ingress-index.md|nginx-ingress-controller 知识图谱索引]]
- [[domain-19-landscape-references/topic-index/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]
- [[domain-19-landscape-references/topic-index/higress-index.md|Higress 知识图谱索引]]

## See Also

- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/03-networking/03-service-ingress-troubleshooting.md|03-service-ingress-troubleshooting]]
- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/03-networking/04-networkpolicy-troubleshooting.md|04-networkpolicy-troubleshooting]]
- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/03-networking/06-gateway-api-troubleshooting.md|06-gateway-api-troubleshooting]]
- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/03-networking/07-terway-troubleshooting.md|07-terway-troubleshooting]]

```