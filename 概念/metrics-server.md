---
title: Metrics Server
summary: Metrics Server 是 Kubernetes 集群中用于收集和提供资源使用指标的核心组件。
category: concepts
tags:
- core-concept
- k8s
- observability
- autoscaling
- visibility/public
tier: supporting
sources:
- KUDIG Stub Generation 2026-05-24
created: 2026-05-24
updated: 2026-07-11
last_updated: 2026-07
status: stable
---



# Metrics Server

## 概述

Metrics Server 是 Kubernetes 集群级别的**资源使用指标聚合器**，是 `kubectl top`、HPA（Horizontal Pod Autoscaler）、VPA（Vertical Pod Autoscaler）和集群自动伸缩器工作的前置依赖。它通过 kubelet 的 Summary API 采集每个节点和 Pod 的 CPU/内存使用数据，聚合后通过 Metrics API（`metrics.k8s.io`）对外提供。它是轻量级、集群内置的"够用就好"方案，**不是** Prometheus 的替代——Metrics Server 只保存最近实时值、不存储历史，也不支持自定义指标。

## 架构与工作原理

```
┌─────────────── 每个工作节点 ───────────────┐
│  cAdvisor（内置在 kubelet）                  │
│     │ /metrics/resource /stats/summary       │
│     ▼                                         │
│  kubelet Summary API（HTTP）                 │
└───────────────────┬───────────────────────────┘
                    │ Metrics Server 主动抓取（每 60s）
                    ▼
┌─────────────── Metrics Server ──────────────┐
│  Deployment（通常 2 副本高可用）              │
│  聚合 Pod/Node 指标，缓存在内存               │
│  通过 APIService 注册 metrics.k8s.io          │
└───────────────────┬───────────────────────────┘
                    │ Metrics API（聚合层）
        ┌───────────┼───────────────┐
        ▼           ▼               ▼
   kubectl top   HPA/VPA        Cluster Autoscaler
```

**工作流**：
1. Metrics Server 作为 Deployment 运行在 `kube-system`，通过 APIService 把自身注册为 `metrics.k8s.io/v1beta1` 的 API 提供者。
2. 它每约 60 秒调用每个节点 kubelet 的 Summary API（`/stats/summary`），采集 CPU/内存使用。
3. kubelet 内嵌的 **cAdvisor**（基于 Google cAdvisor）从 cgroup 读取容器资源数据。
4. Metrics Server 聚合并缓存最近一次采样到内存，通过 Metrics API 返回。
5. HPA 周期性查询 Metrics API，对比阈值决策扩缩；`kubectl top` 同样调用该 API。

**前置条件**：
- kubelet 启用 `--authentication-kubelet-https`、`--authorization-mode`，让 Metrics Server 能鉴权访问。
- API Server 开启聚合层（Aggregation Layer），通常默认开启。
- 节点 cgroup driver 与 kubelet 一致，否则 CPU 统计不准。

## 关键组件与特性

| 组件 | 作用 |
|------|------|
| Metrics Server Pod | 聚合器，运行在 kube-system |
| APIService metrics.k8s.io | 注册聚合 API |
| kubelet Summary API | 节点本地指标源 |
| cAdvisor | kubelet 内嵌，读 cgroup |
| HPA | 主要消费者，按 CPU/内存伸缩 |
| VPA | 按历史用量推荐 resources |

**局限**：
- 只存最近一次采样，无历史；要历史用 Prometheus。
- 只支持 CPU/内存（核心资源），自定义指标需 Prometheus Adapter / KEDA。
- 60s 采样间隔，HPA 默认 30s 轮询，扩缩有分钟级延迟。
- 不适合作为告警数据源。

## 配置示例

Metrics Server 通常以 Helm 或清单方式部署，关键启动参数：

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: metrics-server
  namespace: kube-system
spec:
  replicas: 2
  template:
    spec:
      containers:
      - name: metrics-server
        image: registry.k8s.io/metrics-server/metrics-server:v0.7.2
        args:
        - --cert-dir=/tmp
        - --secure-port=10250
        - --kubelet-preferred-address-types=InternalIP,InternalDNS,Hostname
        - --kubelet-use-node-status-port
        - --metric-resolution=60s          # 采样间隔
        # 内网/自签证书集群常见开启
        - --kubelet-insecure-tls           # 仅测试环境，生产应修复证书
        resources:
          requests: {cpu: 100m, memory: 200Mi}
          limits:   {cpu: 500m, memory: 1Gi}
```

HPA 依赖示例：

```yaml
apiVersion: autoscaling/v2
kind: HorizontalPodAutoscaler
metadata: {name: webapp, namespace: production}
spec:
  scaleTargetRef: {apiVersion: apps/v1, kind: Deployment, name: webapp}
  minReplicas: 3
  maxReplicas: 20
  metrics:
  - type: Resource
    resource:
      name: cpu
      target: {type: Utilization, averageUtilization: 70}
```

## 常用操作与命令

```bash
# 验证 Metrics Server 是否就绪
kubectl get apiservice v1beta1.metrics.k8s.io -o yaml
kubectl top node
kubectl top pod -n production --sort-by=cpu

# 排查：API 不可用
kubectl get --raw /apis/metrics.k8s.io/v1beta1/nodes
kubectl get --raw /apis/metrics.k8s.io/v1beta1/namespaces/production/pods

# 查看 metrics-server 日志
kubectl logs -n kube-system -l k8s-app=metrics-server

# HPA 是否拿到指标
kubectl describe hpa webapp -n production    # 看 Targets 列是否有 <unknown>

# 自定义指标需部署 Prometheus Adapter
helm repo add prometheus-community https://prometheus-community.github.io/helm-charts
helm install prometheus-adapter prometheus-community/prometheus-adapter
```

## 最佳实践

1. **生产环境必装**：HPA/VPA/kubectl top 都依赖它，不装等于没有弹性伸缩能力。
2. **多副本高可用**：metrics-server 副本数 ≥2，避免单点导致 HPA 失效。
3. **修复 kubelet 证书**：避免用 `--kubelet-insecure-tls`，生产应签发或复用 kubelet 证书。
4. **采样间隔 60s**：过短增加负载，过长 HPA 反应慢；保持默认 60s 通常最优。
5. **HPA 配合自定义指标**：CPU/内存不够时接 Prometheus Adapter 或 KEDA（按 QPS/队列长度伸缩）。
6. **历史/告警用 Prometheus**：Metrics Server 不替代监控栈，二者定位不同。
7. **资源预留**：大集群（500+ 节点）metrics-server 需要更多 CPU/内存，按官方 sizing 调整。

## 常见陷阱

- **kubectl top 报 error: metrics not available**：Metrics Server 未就绪、APIService 异常、或节点 Summary API 拒绝访问。
- **HPA unable to read metrics**：同样因 Metrics API 缺失，HPA 的 Targets 显示 `<unknown>`。
- **kubelet 证书不被信任**：自建集群常见，临时用 `--kubelet-insecure-tls` 但生产必须修复。
- **cgroup driver 不一致**：CPU 统计偏差大，HPA 误判。
- **大集群 OOM**：metrics-server 内存不够聚合所有节点数据，按集群规模调大 limit。
- **聚合层未开启**：APIService 处于 False，Metrics API 路由不到 metrics-server。
- **Pod 未设 requests**：HPA 按 utilization 算时除 0，需每个容器都有 requests。

## 源码实现分析

### Metrics Server 数据采集流程

```go
// sigs.k8s.io/metrics-server/pkg/scraper/client/summary/client.go
// Metrics Server 从每个节点的 kubelet Summary API 采集指标
func (c *client) GetSummary(ctx context.Context, node string) (*stats.Summary, error) {
    // 1. 调用 kubelet /stats/summary 端点
    url := fmt.Sprintf("https://%s:10250/stats/summary", node)
    resp, err := c.httpClient.Get(url)
    // 返回：节点 CPU/内存 + 每个 Pod 的 CPU/内存
}

// sigs.k8s.io/metrics-server/pkg/server/server.go
// Metrics Server 暴露 Metrics API (metrics.k8s.io/v1beta1)
func (s *server) GetPodMetrics(ctx context.Context, ns string) (*metrics.PodMetricsList, error) {
    // 2. 从内存缓存中查询最新指标
    pods := s.storage.GetPods(ns)
    for _, pod := range pods {
        // 3. 返回每个容器的 CPU/内存使用量
        metrics = append(metrics, metrics.PodMetrics{
            Containers: pod.Containers,  // cpu: nanocores, memory: bytes
        })
    }
    return &metrics.PodMetricsList{Items: metrics}
}
```

### Metrics Server 架构

```
┌───────────────────────────────────────────────────────────┐
│          Metrics Server 架构                          │
├───────────────────────────────────────────────────────────┤
│                                                           │
│  kubectl top / HPA / VPA                                 │
│    │  metrics.k8s.io/v1beta1 API                         │
│    ▼                                                      │
│  kube-apiserver (聚合层)                                │
│    │  APIService: v1beta1.metrics.k8s.io                 │
│    ▼                                                      │
│  Metrics Server Pod(s)                                   │
│    │  每 60s 采集一次                                    │
│    ▼                                                      │
│  kubelet /stats/summary (每个节点)                      │
│    │  cAdvisor 采集容器指标                            │
│    ▼                                                      │
│  cgroup (CPU/内存实际使用量)                            │
│                                                           │
│  关键特性:                                               │
│  • 只保留最新数据点，无历史                          │
│  • 不是监控系统，不能替代 Prometheus               │
│  • 通过 APIService 聚合层注册到 apiserver           │
│  • HPA 每 15s 查询一次 Metrics API                    │
└───────────────────────────────────────────────────────────┘
```

### 生产部署示例（🟡 部署到集群）

```yaml
# metrics-server 高可用部署
apiVersion: apps/v1
kind: Deployment
metadata:
  name: metrics-server
  namespace: kube-system
spec:
  replicas: 2  # 高可用
  selector:
    matchLabels:
      k8s-app: metrics-server
  template:
    spec:
      containers:
      - name: metrics-server
        image: registry.k8s.io/metrics-server/metrics-server:v0.7.1
        args:
        - --metric-resolution=60s
        - --kubelet-preferred-address-types=InternalIP
        # 生产环境不用 --kubelet-insecure-tls
        resources:
          requests:
            cpu: 100m
            memory: 200Mi
          limits:
            memory: 512Mi  # 大集群需调大
```

## 面试要点

1. **Metrics Server 与 Prometheus 的区别？**
   - Metrics Server：只保留最新数据点，供 HPA/kubectl top
   - Prometheus：历史数据存储、告警、可视化
   - 两者互补，不是替代关系

2. **HPA 如何获取指标？**
   - HPA Controller 每 15s 查询 Metrics API
   - Metrics API 由 Metrics Server 通过 APIService 聚合层提供
   - 自定义指标需 Prometheus Adapter 或 KEDA

3. **Metrics Server 的数据采集流程？**
   - 每 60s 从每个节点的 kubelet /stats/summary 采集
   - kubelet 通过 cAdvisor 读取 cgroup 数据
   - Metrics Server 聚合后通过 Metrics API 暴露

4. **生产环境 Metrics Server 注意事项？**
   - 多副本高可用（≥2）
   - 不用 --kubelet-insecure-tls，修复 kubelet 证书
   - 大集群调大资源限制
   - Pod 必须设 requests，否则 HPA 无法计算 utilization

## 参见

- [[kubernetes]] — k8s 领域核心页面
- [[概念/pods.md|Pod]] — 指标采集对象
- [[概念/autoscaling-strategies.md|自动伸缩策略]]
- [[概念/bp-observability.md|可观测性最佳实践]]
- [[README]]

## Related

- [[visibility-public|#visibility/public Hub]] — tag hub
