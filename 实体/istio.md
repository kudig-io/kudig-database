---
title: Istio (entities)
description: '- [[技能/k8s-network-security-guide.md|k8s-network-security-guide]]
  — Kubernetes 网络安全最佳实践'
summary: '- [[技能/k8s-network-security-guide.md|k8s-network-security-guide]] —
  Kubernetes 网络安全最佳实践'
category: entities
tags:
- k8s
- service-mesh
- istio
- envoy
- mtls
- traffic-management
- ingress
- gateway
- helm
- operator
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Istio 是什么
- 如何 Istio
trigger_keywords:
- Istio
prerequisites:
- kubectl-basics
- helm-basics
- service-mesh-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Istio

Istio is the most widely adopted [[Service|service]]Service Mesh）|service mesh]], graduated from CNCF in 2023. It provides transparent traffic management, security, and observability for microservices.

## Key Facts

- **Status**: CNCF graduated (2023)
- **Data Plane**: Envoy proxy (C++)
- **Control Plane**: istiod (merged Pilot+Citadel+Galley)
- **Memory**: ~2GB control plane, ~100MB/sidecar
- **Modes**: Sidecar (traditional), Ambient (sidecarless, GA v1.29)

## Core Resources

| Resource | Purpose |
|----------|---------|
| VirtualService | Traffic routing, weight splitting, retries, timeouts |
| DestinationRule | Connection pools, outlier detection, traffic policies |
| Gateway | [[Ingress|Ingress]]/egress traffic entry point |
| PeerAuthentication | mTLS mode (STRICT/PERMISSIVE/DISABLE) |
| AuthorizationPolicy | L7 access control (allow/deny rules) |
| RequestAuthentication | JWT validation for external services |

## Ambient Mesh (v1.29 GA)

Istio Ambient replaces sidecars with:
- **ztunnel**: Node-level L4 proxy (Rust, ~50MB/node) for mTLS and L4 policies
- **Waypoint Proxy**: Per-service L7 proxy for advanced traffic management

Benefits: lower resource overhead, simpler operations, no sidecar injection issues.

## 安装与配置

```bash
# 🟢 使用 istioctl 安装 (推荐)
istioctl install --set profile=default -y

# 🟢 验证安装
istioctl verify-install
kubectl get pods -n istio-system

# 🟢 启用 Sidecar 注入
kubectl label namespace default istio-injection=enabled

# 🟢 Ambient 模式安装
istioctl install --set profile=ambient -y
kubectl label namespace default istio.io/dataplane-mode=ambient

# 🟡 卸载 Istio
istioctl uninstall --purge -y
kubectl delete namespace istio-system
```

### IstioOperator 配置示例

```yaml
apiVersion: install.istio.io/v1alpha1
kind: IstioOperator
metadata:
  name: production-istio
  namespace: istio-system
spec:
  profile: default
  meshConfig:
    accessLogFile: /dev/stdout
    enableTracing: true
    defaultConfig:
      proxyMetadata:
        ISTIO_META_DNS_CAPTURE: "true"
      holdApplicationUntilProxyStarts: true
    outboundTrafficPolicy:
      mode: REGISTRY_ONLY
  components:
    pilot:
      k8s:
        resources:
          requests:
            cpu: 500m
            memory: 2Gi
        hpaSpec:
          maxReplicas: 5
          minReplicas: 2
    ingressGateways:
    - name: istio-ingressgateway
      enabled: true
      k8s:
        hpaSpec:
          maxReplicas: 5
          minReplicas: 2
  values:
    global:
      proxy:
        resources:
          requests:
            cpu: 100m
            memory: 128Mi
          limits:
            cpu: 500m
            memory: 512Mi
```

## 流量管理

### VirtualService 金丝雀发布

```yaml
apiVersion: networking.istio.io/v1beta1
kind: VirtualService
metadata:
  name: reviews-route
spec:
  hosts:
  - reviews.prod.svc.cluster.local
  http:
  - match:
    - headers:
        x-canary:
          exact: "true"
    route:
    - destination:
        host: reviews.prod.svc.cluster.local
        subset: v2
  - route:
    - destination:
        host: reviews.prod.svc.cluster.local
        subset: v1
      weight: 90
    - destination:
        host: reviews.prod.svc.cluster.local
        subset: v2
      weight: 10
    retries:
      attempts: 3
      perTryTimeout: 2s
      retryOn: 5xx,reset,connect-failure
    timeout: 10s
```

### DestinationRule 连接池与熔断

```yaml
apiVersion: networking.istio.io/v1beta1
kind: DestinationRule
metadata:
  name: reviews-dest
spec:
  host: reviews.prod.svc.cluster.local
  trafficPolicy:
    connectionPool:
      tcp:
        maxConnections: 100
        connectTimeout: 30ms
      http:
        h2UpgradePolicy: DEFAULT
        http1MaxPendingRequests: 100
        http2MaxRequests: 1000
        maxRequestsPerConnection: 10
    outlierDetection:
      consecutive5xxErrors: 5
      interval: 10s
      baseEjectionTime: 30s
      maxEjectionPercent: 50
  subsets:
  - name: v1
    labels:
      version: v1
  - name: v2
    labels:
      version: v2
```

## 运维操作

### 常用命令

```bash
# 🟢 查看 Sidecar 注入状态
kubectl get pods -n default -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{range .spec.containers[*]}{.name}{" "}{end}{"\n"}{end}'

# 🟢 查看 Envoy 配置 (proxy-config)
istioctl proxy-config listeners <pod-name> -n <namespace>
istioctl proxy-config clusters <pod-name> -n <namespace>
istioctl proxy-config routes <pod-name> -n <namespace>
istioctl proxy-config endpoints <pod-name> -n <namespace>
istioctl proxy-config secrets <pod-name> -n <namespace>

# 🟢 查看 Sidecar 日志
kubectl logs <pod-name> -c istio-proxy -n <namespace>

# 🟢 分析配置问题
istioctl analyze -n <namespace>
istioctl analyze --all-namespaces

# 🟢 查看 xDS 同步状态
istioctl proxy-status

# 🟡 手动触发 Sidecar 重启
kubectl rollout restart deployment/<name> -n <namespace>

# 🟢 查看 Istio 版本
istioctl version --remote

# 🟢 查看 mTLS 状态
istioctl authn tls-check

# 🟢 流量镜像配置验证
kubectl get virtualservice -o yaml | grep -A5 mirror
```

### 升级策略

```bash
# 🟡 原地升级 (canary upgrade 推荐)
istioctl install --set profile=default --set revision=1-29-0
kubectl label namespace default istio.io/rev=1-29-0 --overwrite
# 逐步迁移 namespace 后删除旧版本
istioctl uninstall --revision 1-28-0
```

## 故障排查

| 症状 | 可能原因 | 排查命令 | 修复方案 |
|------|----------|----------|----------|
| Sidecar 未注入 | Namespace 未标记/标签冲突 | `kubectl get ns <ns> --show-labels` | 添加 istio-injection=enabled |
| 503 UC (no healthy upstream) | 上游服务不可达 | `istioctl proxy-config endpoints <pod>` | 检查后端 Pod 就绪状态 |
| 503 UH (no healthy upstream) | 熔断触发 | 查看 DestinationRule outlierDetection | 调整熔断阈值 |
| mTLS 握手失败 | PeerAuthentication 不匹配 | `istioctl authn tls-check` | 统一 mTLS 模式 |
| 路由不生效 | VirtualService 匹配规则冲突 | `istioctl analyze` | 检查 host/match 优先级 |
| Sidecar OOM | 大集群 endpoints 过多 | `kubectl top pod -c istio-proxy` | 使用 Sidecar 资源限制导出 |
| DNS 解析失败 | ISTIO_META_DNS_CAPTURE 未启用 | `istioctl proxy-config listeners <pod> --port 15053` | 启用 DNS 代理 |
| 延迟增加 | Sidecar 额外跳转 | `istioctl proxy-config routes` | 评估 Ambient 模式 |

### 排查流程

```
1. istioctl proxy-status → 确认 xDS 同步 (SYNCED)
2. istioctl analyze → 检查配置错误
3. istioctl proxy-config all <pod> → 验证下发配置
4. kubectl logs <pod> -c istio-proxy → 查看 Envoy 日志
5. istioctl proxy-config secrets <pod> → 验证证书状态
6. curl -v http://service:port → 确认连通性
```

## 生产案例

### 案例1: 大集群 Sidecar 内存溢出
- **场景**: 5000+ Service 集群，Sidecar 内存超过 1GB
- **根因**: 默认配置下 Envoy 缓存所有 Service 的 endpoints
- **解决**: 使用 Sidecar 资源限制导出范围
```yaml
apiVersion: networking.istio.io/v1beta1
kind: Sidecar
metadata:
  name: default
  namespace: my-app
spec:
  egress:
  - hosts:
    - "./*"  # 本命名空间
    - "istio-system/*"
    - "dependency-ns/*"  # 仅依赖的命名空间
```

### 案例2: 升级导致流量中断
- **场景**: 从 1.27 升级到 1.28 时部分服务 503
- **根因**: 新旧版本 Envoy filter 不兼容
- **解决**: 使用 Canary Upgrade，逐 namespace 迁移 revision

### 案例3: mTLS STRICT 模式导致 Job 失败
- **场景**: CronJob 连接外部数据库超时
- **根因**: STRICT mTLS 阻止了非 mesh 流量
- **解决**: 为特定 workload 设置 PERMISSIVE 或使用 ServiceEntry

## Istio vs Linkerd vs Consul Connect

| 维度 | Istio | Linkerd | Consul Connect |
|------|-------|---------|----------------|
| 数据面 | Envoy (C++) | linkerd2-proxy (Rust) | Envoy |
| 控制面 | istiod | linkerd-control-plane | Consul Server |
| 资源开销 | 高 (~100MB/sidecar) | 低 (~30MB/sidecar) | 中 |
| 功能丰富度 | 最完整 | 简洁 | 中 |
| 学习曲线 | 陡峭 | 平缓 | 中等 |
| Ambient 模式 | 支持 (GA) | 不支持 | 不支持 |
| 多集群 | 原生支持 | 支持 | 原生支持 |
| CNCF 状态 | Graduated | Graduated | 非 CNCF |

## 版本兼容矩阵

| Istio 版本 | K8s 版本 | Envoy 版本 | 关键特性 |
|-----------|----------|-----------|----------|
| 1.29 | 1.28-1.32 | 1.32 | Ambient GA |
| 1.28 | 1.27-1.31 | 1.31 | Waypoint 增强 |
| 1.27 | 1.26-1.30 | 1.30 | Telemetry API GA |
| 1.26 | 1.25-1.29 | 1.29 | ztunnel 稳定 |

## 检查清单

- [ ] istiod 副本数 >= 2，配置 PDB
- [ ] Sidecar 资源 requests/limits 已设置
- [ ] PeerAuthentication 策略已明确 (STRICT/PERMISSIVE)
- [ ] AuthorizationPolicy 已配置默认拒绝
- [ ] VirtualService 超时和重试已设置
- [ ] DestinationRule 熔断阈值已配置
- [ ] 使用 Sidecar 资源限制导出范围 (大集群)
- [ ] 启用访问日志用于审计
- [ ] 升级使用 Canary Upgrade 策略
- [ ] 监控 istiod 和 Sidecar 资源使用

## Related

- [[技能/k8s-network-security-guide.md|k8s-network-security-guide]] — Kubernetes 网络安全最佳实践
- [[03-istio-security-hardening]] — Istio 安全加固
- [[envoy]] — Envoy
- [[概念/microservice-resilience-patterns.md|microservice-resilience-patterns]] — Microservice Resilience Patterns
- [[概念/service-mesh-architecture.md|service-mesh-architecture]] — Service Mesh Architecture
- [[概念/service-mesh-architecture.md|Service Mesh Architecture]]
- [[概念/microservice-resilience-patterns.md|Microservice Resilience Patterns]]
- [[envoy|Envoy Proxy]]
- [[linkerd|Linkerd]]

- 09-kubernetes-service-mesh-istio-integration
- 02-istio-advanced-traffic-management
- RELEASE-NOTES-1.9
- RELEASE-NOTES-1.28
- RELEASE-NOTES-0.8
- RELEASE-NOTES-1.18
- RELEASE-NOTES-1.19
- RELEASE-NOTES-1.8
- RELEASE-NOTES-1.29
- RELEASE-NOTES-1.16
- RELEASE-NOTES-1.22
- RELEASE-NOTES-1.3
- RELEASE-NOTES-0.2
- RELEASE-NOTES-1.26
- RELEASE-NOTES-1.7
- RELEASE-NOTES-1.12
- RELEASE-NOTES-0.6
- RELEASE-NOTES-1.27
- RELEASE-NOTES-1.6
- RELEASE-NOTES-1.13
- RELEASE-NOTES-0.7
- RELEASE-NOTES-1.17
- RELEASE-NOTES-1.23
- RELEASE-NOTES-1.2
- RELEASE-NOTES-0.3
- RELEASE-NOTES-1.5
- RELEASE-NOTES-1.24
- RELEASE-NOTES-1.10
- RELEASE-NOTES-0.4
- RELEASE-NOTES-1.14
- RELEASE-NOTES-1.1
- RELEASE-NOTES-1.20
- RELEASE-NOTES-1.15
- RELEASE-NOTES-1.0
- RELEASE-NOTES-1.21
- RELEASE-NOTES-0.1
- RELEASE-NOTES-1.4
- RELEASE-NOTES-1.25
- RELEASE-NOTES-1.11
- RELEASE-NOTES-0.5
- 99-istio-service-mesh-guide
- 01-istio-enterprise-service-mesh
- [[故障诊断/FTA故障树/list/service-mesh-istio-fta.md|Service Mesh(Istio) 异常故障树分析]]
- [[故障诊断/高级排障/03-networking/05-service-mesh-istio-troubleshooting.md|05-service-mesh-istio-troubleshooting]]
- istio
- [[实体/release-notes-networking.md|发布说明索引 — 网络]] — Cross-reference
- [[实体/k8s-platform-extensions.md|平台运维与扩展生态：Helm、CI/CD、Operator 开发与服务网格]] — Cross-reference
- [[实体/multi-cloud-terms.md|K8s 多云架构术语参考]] — Cross-reference
- [[概念/service-mesh-evolution.md|服务网格演进]] — Cross-reference
- [[概念/bp-security.md|最佳实践：Security]] — Cross-reference
- [[技能/learn-05-ingress-basics.md|第五课：Ingress - 外部 HTTP/HTTPS 访问]] — Cross-reference
- [[技能/service-mesh-istio-fta.md|Service Mesh(Istio) 异常故障树分析]] — Cross-reference
- [[技能/ts-cloud-provider.md|云服务商集成排查]] — Cross-reference
- [[技能/deployment-canary-and-bluegreen.md|金丝雀与蓝绿发布]] — Cross-reference
- [[实体/cncf-networking.md|CNCF 网络与服务网格项目全景]] — Cross-reference
- [[实体/cncf-security.md|CNCF 安全与合规项目全景]] — Cross-reference
- [[实体/ecosystem-changelog.md|生态组件变更日志索引]] — Cross-reference
- [[生态参考/领域索引/service-mesh-index.md|Service Mesh 服务网格知识图谱索引]]
- [[生态参考/领域索引/network-index.md|Network 网络知识图谱索引]]
- [[生态参考/领域索引/higress-index.md|Higress 知识图谱索引]]


<!-- risk-assessed -->
