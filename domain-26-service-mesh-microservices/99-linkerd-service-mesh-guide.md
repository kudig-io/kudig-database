# Linkerd 轻量级服务网格实践指南

> **适用版本**: Linkerd v2.18 (stable) / Linkerd v2.19 (edge)  
> **最后更新**: 2026-04-24  
> **难度**: 初级 → 中级

---

## 📋 目录

- [一、架构与设计理念](#一架构与设计理念)
- [二、安装部署](#二安装部署)
- [三、自动 mTLS](#三自动-mtls)
- [四、流量管理](#四流量管理)
- [五、可观测性](#五可观测性)
- [六、多集群连接](#六多集群连接)
- [七、Linkerd vs Istio 对比](#七linkerd-vs-istio-对比)
- [八、生产级配置](#八生产级配置)
- [九、故障排查](#九故障排查)

---

## 一、架构与设计理念

```
Linkerd 架构 (数据平面)
Pod
├── App Container
└── linkerd-proxy Sidecar (Rust 编写)
    ├── 透明 TCP 代理 (iptables/NFT 重定向)
    ├── 自动 mTLS (每 Pod 证书)
    ├── HTTP/2 多路复用
    ├── 负载均衡 (EWMA/Power of Two Choices)
    └── Prometheus 指标导出

控制平面 (轻量级)
├── destination     ← 服务发现
├── identity        ← 证书颁发 (SPIFFE/SPIRE)
├── proxy-injector  ← Sidecar 自动注入
└── tap / viz       ← 流量观察
```

### 核心设计原则

| 原则 | 说明 |
|:---|:---|
| 极简主义 | 功能聚焦，减少配置复杂度 |
| 零配置安全 | mTLS 默认启用，无需额外配置 |
| 性能优先 | Rust 代理，亚毫秒级延迟增加 |
| 渐进式采用 | 可按命名空间逐步接入 |

---

## 二、安装部署

### 2.1 CLI 安装

```bash
# 安装 linkerd CLI
curl --proto '=https' --tlsv1.2 -sSfL https://run.linkerd.io/install | sh
export PATH=$PATH:$HOME/.linkerd2/bin
linkerd version

# 检查集群兼容性
linkerd check --pre
```

### 2.2 控制平面安装

```bash
# 默认安装 (开发/测试)
linkerd install | kubectl apply -f -

# 生产级安装 (HA 模式)
linkerd install \
  --ha \
  --controller-replicas 3 \
  --set proxyInit.runAsRoot=true \
  | kubectl apply -f -

# 验证安装
linkerd check
```

### 2.3 Viz 扩展 (监控仪表板)

```bash
linkerd viz install | kubectl apply -f -
linkerd viz check

# 启动本地仪表板
linkerd viz dashboard
```

### 2.4 多集群扩展

```bash
linkerd multicluster install | kubectl apply -f -
linkerd multicluster check
```

---

## 三、自动 mTLS

### 3.1 默认行为

```
无需配置，自动启用:
1. identity 服务为每个 Pod 签发证书 (24h TTL)
2. 代理自动轮换证书
3. 所有 Pod 间通信自动加密
4. 支持外部 CA 集成 (cert-manager)
```

### 3.2 验证 mTLS

```bash
# 查看证书状态
linkerd identity deployment/myapp -n production

# 检查 mTLS 状态
linkerd viz stat deployment -n production
# 注意 SECURED 列显示是否为加密流量
```

### 3.3 与 cert-manager 集成 (外部 CA)

```yaml
apiVersion: linkerd.io/v1alpha2
kind: Issuer
metadata:
  name: linkerd-identity-issuer
  namespace: linkerd
spec:
  certManager:
    issuerRef:
      kind: ClusterIssuer
      name: myorg-ca
```

---

## 四、流量管理

### 4.1 自动负载均衡

```
Linkerd 自动使用 EWMA 算法:
- 基于延迟的负载均衡
- 自动避开慢节点
- 无需配置
```

### 4.2 重试与超时

```yaml
apiVersion: policy.linkerd.io/v1beta3
kind: HTTPRoute
metadata:
  name: myapp-route
  namespace: production
spec:
  parentRefs:
  - name: myapp
    kind: Service
    group: core
  rules:
  - matches:
    - path:
        type: PathPrefix
        value: /api
    backendRefs:
    - name: myapp
      port: 8080
    # 重试配置 (ServiceProfile 中定义)
---
apiVersion: linkerd.io/v1alpha2
kind: ServiceProfile
metadata:
  name: myapp.production.svc.cluster.local
  namespace: production
spec:
  routes:
  - name: GET /api/users
    condition:
      method: GET
      pathRegex: /api/users
    timeout: 300ms
    retryBudget:
      retryRatio: 0.2
      minRetriesPerSecond: 10
      ttl: 10s
    isRetryable: true
```

### 4.3 流量分割 (金丝雀发布)

```yaml
apiVersion: split.smi-spec.io/v1alpha4
kind: TrafficSplit
metadata:
  name: myapp-canary
  namespace: production
spec:
  service: myapp
n  backends:
  - service: myapp-stable
    weight: 90
  - service: myapp-canary
    weight: 10
```

### 4.4 故障注入

```yaml
apiVersion: policy.linkerd.io/v1alpha1
kind: FaultInjection
metadata:
  name: myapp-fault
  namespace: production
spec:
  targetRef:
    group: ""
    kind: Service
    name: myapp
  requestAbort:
    httpStatus: 503
    percentage: 1  # 1% 请求失败
```

---

## 五、可观测性

### 5.1 黄金指标

```bash
# 实时流量统计
linkerd viz stat deployment -n production

# 输出:
# NAME        MESHED   SUCCESS      RPS   LATENCY_P50   LATENCY_P95   LATENCY_P99    TCP_CONN
# myapp          3/3   100.00%   10.5rps          15ms          45ms          89ms           5
# myapp-db       2/2    99.95%    8.2rps           5ms          20ms          35ms          10

# 按路径统计
linkerd viz top deployment/myapp -n production

# 依赖拓扑
linkerd viz edges deployment -n production
```

### 5.2 Prometheus 指标

| 指标 | 说明 |
|:---|:---|
| request_total | 请求总数 (按响应码分类) |
| response_latency_ms | 响应延迟分布 |
| tcp_open_total | TCP 连接打开数 |
| tcp_close_total | TCP 连接关闭数 |
| control_heartbeat_latency_ms | 控制平面延迟 |

### 5.3 Grafana Dashboard

```bash
# 导入官方 Dashboard
# Dashboard ID: 7639 (Linkerd2 dashboards)
# 或从 linkerd-viz 获取
kubectl port-forward -n linkerd-viz svc/grafana 3000:3000
```

---

## 六、多集群连接

### 6.1 架构

```
Cluster A (east)                    Cluster B (west)
├── linkerd-gateway                 ├── linkerd-gateway
│   └── 暴露公网地址                 └── 暴露公网地址
├── Service: myapp                  ├── Service: myapp
└── ServiceExport: myapp            └── ServiceImport: myapp (来自 east)
```

### 6.2 连接配置

```bash
# Cluster A: 导出服务
linkerd multicluster link --cluster-name east | kubectl apply -f -
kubectl label svc myapp mirror.linkerd.io/exported=true -n production

# Cluster B: 访问远程服务
kubectl get svc -n production
# myapp-east  (自动创建，指向 Cluster A)
```

---

## 七、Linkerd vs Istio 对比

| 维度 | Linkerd | Istio |
|:---|:---|:---|
| **控制平面** | 轻量级 (~500MB) | 较重 (~2GB+) |
| **数据平面** | Rust (内存 ~20MB) | Envoy C++ (内存 ~100MB+) |
| **延迟开销** | < 1ms | 1-3ms |
| **资源占用** | 低 | 高 |
| **功能覆盖** | 核心服务网格功能 | 完整服务网格 + 网关 |
| **配置复杂度** | 极简 | 丰富但复杂 |
| **Ambient Mesh** | 无 (仅 Sidecar) | Ambient + Sidecar 双模式 |
| **多集群** | 支持 (简单) | 支持 (完整) |
| **WASM 扩展** | 不支持 | 支持 |
| **学习曲线** | 低 | 高 |
| **企业支持** | Buoyant | Solo.io / Tetrate |
| **CNCF 状态** | Graduated (2021) | Graduated (2023) |

### 选型决策

```
选择 Linkerd 如果:
  ✅ 追求极简配置和低开销
  ✅ 主要需求是 mTLS + 可观测性
  ✅ 资源受限环境 (边缘/IoT)
  ✅ 团队服务网格经验有限
  ✅ 渐进式采用，快速落地

选择 Istio 如果:
  ✅ 需要 Ambient Mesh (无 Sidecar)
  ✅ 复杂流量管理 (超时/重试/镜像/故障注入)
  ✅ 需要 WASM 扩展
  ✅ 大规模多集群 (100+ 集群)
  ✅ 需要 API Gateway 功能
```

---

## 八、生产级配置

### 8.1 命名空间级注入控制

```yaml
apiVersion: v1
kind: Namespace
metadata:
  name: production
  annotations:
    linkerd.io/inject: enabled
---
# 排除特定 Pod
apiVersion: apps/v1
kind: Deployment
metadata:
  name: legacy-app
spec:
  template:
    metadata:
      annotations:
        linkerd.io/inject: disabled
```

### 8.2 资源限制

```yaml
# Proxy 资源限制 (全局默认值)
linkerd install \
  --set proxy.resources.cpu.limit=500m \
  --set proxy.resources.memory.limit=128Mi \
  --set proxy.resources.cpu.request=100m \
  --set proxy.resources.memory.request=64Mi
```

### 8.3 高可用配置

```yaml
# 控制平面 HA
spec:
  replicas: 3
  affinity:
    podAntiAffinity:
      requiredDuringSchedulingIgnoredDuringExecution:
      - labelSelector:
          matchLabels:
            linkerd.io/control-plane-component: destination
        topologyKey: kubernetes.io/hostname
```

---

## 九、故障排查

### 9.1 常用诊断命令

```bash
# 检查控制平面
linkerd check

# 检查数据平面
linkerd check --proxy

# 查看 Pod 代理状态
linkerd viz stat pod -n production

# 流量实时观察
linkerd viz tap deployment/myapp -n production

# 查看代理日志
kubectl logs -n production deployment/myapp -c linkerd-proxy

# 检查证书
linkerd identity -n production deployment/myapp

# 网络诊断
linkerd diagnostics policy -n production pod/myapp-xxx
```

### 9.2 常见问题

| 问题 | 原因 | 解决 |
|:---|:---|:---|
| Pod 无法启动 | init 容器失败 | 检查 proxy-init 权限 (NET_ADMIN) |
| mTLS 未生效 | 注入未启用 | 确认 namespace annotation 或 pod annotation |
| 延迟增加 | 代理资源不足 | 增加 proxy CPU/memory limit |
| 流量不统计 | viz 扩展未安装 | linkerd viz install |
| 证书过期 | identity 服务异常 | 重启 identity 或检查 cert-manager |

---

## 参考链接

- [Linkerd 官方文档](https://linkerd.io/2/overview/)
- [Linkerd GitHub](https://github.com/linkerd/linkerd2)
- [Linkerd 性能基准](https://linkerd.io/2021/05/27/linkerd-vs-istio-benchmarks/)
- [SMI (Service Mesh Interface)](https://smi-spec.io/)
- [Buoyant Enterprise](https://buoyant.io/)
