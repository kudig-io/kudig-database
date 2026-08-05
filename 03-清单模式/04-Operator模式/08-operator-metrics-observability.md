---
title: Operator Metrics 可观测性
description: Prometheus 指标暴露、ServiceMonitor 配置与运维仪表板
summary: Operator Prometheus 指标设计，包括 Reconcile 计数、工作队列深度、controller-runtime 默认指标及自定义业务指标
category: manifests-patterns
tags:
- k8s
- manifests
- operator
- metrics
- prometheus
- observability
tier: core
created: '2026-07-11'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 平台工程师
- SRE
- 开发工程师
estimated_read_time: 10min
intent_queries:
- Operator Prometheus 指标
- controller-runtime metrics
- Operator 可观测性
trigger_keywords:
- metrics
- prometheus
- servicemonitor
- reconcile
- observability
prerequisites:
- operator-basics
- prometheus-basics
authors:
- name: KUDIG Team
  role: contributor
---

# Operator Metrics 可观测性

## 1. controller-runtime 默认指标

controller-runtime 自带 Prometheus 指标，路径默认为 `/metrics`：

| 指标 | 类型 | 说明 |
|------|------|------|
| `controller_runtime_reconcile_total` | Counter | Reconcile 调用次数（按 controller, result 标签） |
| `controller_runtime_reconcile_errors_total` | Counter | Reconcile 错误次数 |
| `controller_runtime_reconcile_time_seconds` | Histogram | Reconcile 耗时分布 |
| `workqueue_depth` | Gauge | 工作队列深度 |
| `workqueue_adds_total` | Counter | 入队总数 |
| `leader_election_master_status` | Gauge | Leader 状态（1=Leader, 0=Follower） |

## 2. 自定义业务指标

```go
import (
    "github.com/prometheus/client_golang/prometheus"
    "sigs.k8s.io/controller-runtime/pkg/metrics"
)

var (
    webappReplicasGauge = prometheus.NewGaugeVec(
        prometheus.GaugeOpts{
            Name: "webapp_ready_replicas",
            Help: "当前就绪的 WebApp 副本数",
        },
        []string{"namespace", "webapp", "image"},
    )

    externalResourceOpsCounter = prometheus.NewCounterVec(
        prometheus.CounterOpts{
            Name: "webapp_external_resource_ops_total",
            Help: "外部资源操作计数",
        },
        []string{"operation", "resource_type", "status"},
    )

    reconcileLatency = prometheus.NewHistogramVec(
        prometheus.HistogramOpts{
            Name:    "webapp_reconcile_latency_seconds",
            Help:    "WebApp Reconcile 延迟",
            Buckets: []float64{0.1, 0.5, 1, 2, 5, 10, 30},
        },
        []string{"namespace"},
    )
)

func init() {
    metrics.Registry.MustRegister(
        webappReplicasGauge,
        externalResourceOpsCounter,
        reconcileLatency,
    )
}
```

## 3. 在 Reconcile 中记录指标

```go
func (r *WebAppReconciler) Reconcile(ctx context.Context, req ctrl.Request) (ctrl.Result, error) {
    start := time.Now()
    defer func() {
        reconcileLatency.WithLabelValues(req.Namespace).Observe(time.Since(start).Seconds())
    }()

    var webapp platformv1.WebApp
    if err := r.Get(ctx, req.NamespacedName, &webapp); err != nil {
        return ctrl.Result{}, client.IgnoreNotFound(err)
    }

    // 业务指标
    webappReplicasGauge.WithLabelValues(
        webapp.Namespace, webapp.Name, webapp.Spec.Image,
    ).Set(float64(webapp.Status.ReadyReplicas))

    // 操作计数
    externalResourceOpsCounter.WithLabelValues(
        "create", "dns_record", "success",
    ).Inc()

    return ctrl.Result{}, nil
}
```

## 4. Metrics 服务暴露

```yaml
apiVersion: v1
kind: Service
metadata:
  name: webapp-controller-metrics
  namespace: webapp-system
  labels:
    app: webapp-controller-manager
spec:
  ports:
    - name: metrics
      port: 8080
      targetPort: 8080
      protocol: TCP
  selector:
    control-plane: controller-manager
```

## 5. ServiceMonitor 配置

```yaml
apiVersion: monitoring.coreos.com/v1
kind: ServiceMonitor
metadata:
  name: webapp-controller-monitor
  namespace: webapp-system
  labels:
    release: prometheus  # 匹配 Prometheus Operator 的 selector
spec:
  selector:
    matchLabels:
      app: webapp-controller-manager
  endpoints:
    - port: metrics
      path: /metrics
      interval: 30s
      scrapeTimeout: 10s
      honorLabels: true
```

## 6. 安全的 Metrics 暴露（kube-rbac-proxy）

生产环境推荐通过 kube-rbac-proxy 保护 Metrics 端点：

```yaml
spec:
  template:
    spec:
      containers:
        - name: kube-rbac-proxy
          image: quay.io/brancz/kube-rbac-proxy:v0.15.0
          args:
            - --secure-listen-address=0.0.0.0:8443
            - --upstream=http://127.0.0.1:8080/
            - --logtostderr=true
            - --v=0
          ports:
            - containerPort: 8443
              name: https
          resources:
            requests:
              cpu: 10m
              memory: 20Mi
            limits:
              cpu: 100m
              memory: 64Mi
        - name: manager
          args:
            - --metrics-bind-address=127.0.0.1:8080  # 仅本地访问
```

对应 ServiceMonitor 使用 https：

```yaml
endpoints:
  - port: https
    scheme: https
    tlsConfig:
      insecureSkipVerify: true
    bearerTokenFile: /var/run/secrets/kubernetes.io/serviceaccount/token
```

## 7. 推荐 Grafana 告警规则

| 告警 | PromQL | 说明 |
|------|--------|------|
| Reconcile 错误率高 | `rate(controller_runtime_reconcile_errors_total[5m]) > 0.1` | Reconcile 频繁失败 |
| 工作队列堆积 | `workqueue_depth > 100` | 控制器处理不过来 |
| 无 Leader | `leader_election_master_status == 0` 持续 5m | 所有副本都不是 Leader |
| Reconcile 延迟高 | `histogram_quantile(0.95, rate(controller_runtime_reconcile_time_seconds_bucket[5m])) > 10` | P95 延迟超 10s |

## Related

- [[03-清单模式/04-Operator模式/03-operator-reconciliation-patterns|调谐循环模式]]
- [[03-清单模式/04-Operator模式/05-operator-leader-election|Leader Election 高可用]]

## See Also

- [controller-runtime Metrics 文档](https://book.kubebuilder.io/reference/metrics)
- [Prometheus Operator ServiceMonitor](https://prometheus-operator.dev/docs/operator/api/#monitoring.coreos.com/v1.ServiceMonitor)

<!-- risk-assessed -->
