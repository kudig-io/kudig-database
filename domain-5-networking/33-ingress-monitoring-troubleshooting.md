# 133 - Ingress 监控与故障排查

> **适用版本**: Kubernetes v1.25 - v1.32 | **最后更新**: 2026-01

---

## 一、Ingress 监控指标

### 1.1 关键监控指标

| 指标类别 | 指标名称 | 说明 | 告警阈值建议 |
|---------|---------|------|-------------|
| **可用性** | 请求成功率 | 2xx/3xx 请求比例 | < 99% |
| **可用性** | 错误率 | 4xx/5xx 请求比例 | > 1% |
| **性能** | 请求延迟 (P50) | 50% 请求延迟 | > 100ms |
| **性能** | 请求延迟 (P99) | 99% 请求延迟 | > 1s |
| **容量** | QPS | 每秒请求数 | 接近容量上限 |
| **容量** | 并发连接数 | 活跃连接数 | 接近 max_connections |
| **资源** | CPU 使用率 | 控制器 CPU | > 80% |
| **资源** | 内存使用率 | 控制器内存 | > 80% |
| **健康** | 上游健康状态 | 后端服务健康 | 有不健康后端 |
| **配置** | 配置重载次数 | 配置变更频率 | 异常频繁 |

### 1.2 NGINX Ingress Prometheus 指标

| 指标名称 | 类型 | 标签 | 说明 |
|---------|------|------|------|
| `nginx_ingress_controller_requests` | Counter | ingress, namespace, status, method, path | 请求总数 |
| `nginx_ingress_controller_request_duration_seconds` | Histogram | ingress, namespace, status, method, path | 请求延迟分布 |
| `nginx_ingress_controller_request_size` | Histogram | ingress, namespace | 请求大小分布 |
| `nginx_ingress_controller_response_size` | Histogram | ingress, namespace | 响应大小分布 |
| `nginx_ingress_controller_bytes_sent` | Counter | ingress, namespace | 发送字节总数 |
| `nginx_ingress_controller_nginx_process_connections` | Gauge | state | 连接数 (active/reading/writing/waiting) |
| `nginx_ingress_controller_nginx_process_requests_total` | Counter | - | NGINX 处理请求总数 |
| `nginx_ingress_controller_config_hash` | Gauge | - | 配置哈希值 |
| `nginx_ingress_controller_config_last_reload_successful` | Gauge | - | 最近配置重载是否成功 |
| `nginx_ingress_controller_config_last_reload_successful_timestamp_seconds` | Gauge | - | 最近成功重载时间戳 |
| `nginx_ingress_controller_ssl_certificate_info` | Gauge | namespace, secret, host | 证书信息 |
| `nginx_ingress_controller_ssl_expire_time_seconds` | Gauge | namespace, secret, host | 证书过期时间 |

### 1.3 启用 Prometheus 指标

```yaml
# NGINX Ingress Controller 启用指标
apiVersion: v1
kind: ConfigMap
metadata:
  name: ingress-nginx-controller
  namespace: ingress-nginx
data:
  enable-prometheus-metrics: "true"
  
---
# Service 暴露指标端口
apiVersion: v1
kind: Service
metadata:
  name: ingress-nginx-controller-metrics
  namespace: ingress-nginx
  labels:
    app.kubernetes.io/name: ingress-nginx
spec:
  type: ClusterIP
  ports:
  - name: metrics
    port: 10254
    targetPort: 10254
  selector:
    app.kubernetes.io/name: ingress-nginx
    app.kubernetes.io/component: controller

---
# ServiceMonitor (Prometheus Operator)
apiVersion: monitoring.coreos.com/v1
kind: ServiceMonitor
metadata:
  name: ingress-nginx-controller
  namespace: monitoring
  labels:
    app: ingress-nginx
spec:
  selector:
    matchLabels:
      app.kubernetes.io/name: ingress-nginx
  namespaceSelector:
    matchNames:
    - ingress-nginx
  endpoints:
  - port: metrics
    interval: 15s
    path: /metrics
```

---

## 二、Grafana Dashboard

### 2.1 推荐 Dashboard

| Dashboard ID | 名称 | 说明 |
|-------------|------|------|
| **9614** | NGINX Ingress Controller | 官方推荐 Dashboard |
| **14314** | Kubernetes Nginx Ingress Prometheus | 详细请求分析 |
| **11875** | Ingress Nginx (Grafana.com) | 社区维护 |

### 2.2 关键面板配置

```yaml
# 请求成功率
- title: "请求成功率"
  query: |
    sum(rate(nginx_ingress_controller_requests{status=~"2..|3.."}[5m])) 
    / sum(rate(nginx_ingress_controller_requests[5m])) * 100

# 错误率分布
- title: "错误率 (按状态码)"
  query: |
    sum by (status) (rate(nginx_ingress_controller_requests{status=~"4..|5.."}[5m]))

# 请求延迟 P99
- title: "请求延迟 P99"
  query: |
    histogram_quantile(0.99, sum(rate(nginx_ingress_controller_request_duration_seconds_bucket[5m])) by (le, ingress))

# QPS
- title: "QPS (按 Ingress)"
  query: |
    sum by (ingress) (rate(nginx_ingress_controller_requests[5m]))

# 连接数
- title: "连接数"
  query: |
    nginx_ingress_controller_nginx_process_connections

# 证书过期时间
- title: "证书过期剩余时间"
  query: |
    (nginx_ingress_controller_ssl_expire_time_seconds - time()) / 86400
```

---

## 三、告警规则

### 3.1 可用性告警

```yaml
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: ingress-availability-alerts
  namespace: monitoring
spec:
  groups:
  - name: ingress-availability
    rules:
    # Ingress 不可用
    - alert: IngressControllerDown
      expr: |
        absent(nginx_ingress_controller_nginx_process_connections)
      for: 5m
      labels:
        severity: critical
      annotations:
        summary: "Ingress Controller 不可用"
        description: "Ingress Controller 无法获取指标，可能已停止运行"
    
    # 错误率过高
    - alert: IngressHighErrorRate
      expr: |
        sum(rate(nginx_ingress_controller_requests{status=~"5.."}[5m])) 
        / sum(rate(nginx_ingress_controller_requests[5m])) > 0.01
      for: 5m
      labels:
        severity: warning
      annotations:
        summary: "Ingress 5xx 错误率过高"
        description: "Ingress 5xx 错误率: {{ $value | humanizePercentage }}"
    
    # 严重错误率
    - alert: IngressCriticalErrorRate
      expr: |
        sum(rate(nginx_ingress_controller_requests{status=~"5.."}[5m])) 
        / sum(rate(nginx_ingress_controller_requests[5m])) > 0.05
      for: 2m
      labels:
        severity: critical
      annotations:
        summary: "Ingress 5xx 错误率严重"
        description: "Ingress 5xx 错误率超过 5%: {{ $value | humanizePercentage }}"
    
    # 配置重载失败
    - alert: IngressConfigReloadFailed
      expr: |
        nginx_ingress_controller_config_last_reload_successful == 0
      for: 5m
      labels:
        severity: warning
      annotations:
        summary: "Ingress 配置重载失败"
        description: "Ingress Controller 配置重载失败，可能存在配置错误"
```

### 3.2 性能告警

```yaml
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: ingress-performance-alerts
  namespace: monitoring
spec:
  groups:
  - name: ingress-performance
    rules:
    # 高延迟告警
    - alert: IngressHighLatency
      expr: |
        histogram_quantile(0.99, sum(rate(nginx_ingress_controller_request_duration_seconds_bucket[5m])) by (le)) > 1
      for: 5m
      labels:
        severity: warning
      annotations:
        summary: "Ingress P99 延迟过高"
        description: "Ingress P99 延迟: {{ $value | humanizeDuration }}"
    
    # 连接数过高
    - alert: IngressHighConnections
      expr: |
        nginx_ingress_controller_nginx_process_connections{state="active"} > 10000
      for: 5m
      labels:
        severity: warning
      annotations:
        summary: "Ingress 活跃连接数过高"
        description: "活跃连接数: {{ $value }}"
    
    # CPU 使用率过高
    - alert: IngressHighCPU
      expr: |
        sum(rate(container_cpu_usage_seconds_total{namespace="ingress-nginx",container="controller"}[5m])) 
        / sum(kube_pod_container_resource_limits{namespace="ingress-nginx",container="controller",resource="cpu"}) > 0.8
      for: 10m
      labels:
        severity: warning
      annotations:
        summary: "Ingress Controller CPU 使用率过高"
        description: "CPU 使用率: {{ $value | humanizePercentage }}"
    
    # 内存使用率过高
    - alert: IngressHighMemory
      expr: |
        sum(container_memory_working_set_bytes{namespace="ingress-nginx",container="controller"}) 
        / sum(kube_pod_container_resource_limits{namespace="ingress-nginx",container="controller",resource="memory"}) > 0.8
      for: 10m
      labels:
        severity: warning
      annotations:
        summary: "Ingress Controller 内存使用率过高"
        description: "内存使用率: {{ $value | humanizePercentage }}"
```

### 3.3 证书告警

```yaml
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: ingress-certificate-alerts
  namespace: monitoring
spec:
  groups:
  - name: ingress-certificates
    rules:
    # 证书即将过期 (30 天)
    - alert: IngressCertificateExpiringSoon
      expr: |
        (nginx_ingress_controller_ssl_expire_time_seconds - time()) < 2592000
      for: 1h
      labels:
        severity: warning
      annotations:
        summary: "Ingress 证书即将过期"
        description: "证书 {{ $labels.host }} 将在 {{ $value | humanizeDuration }} 后过期"
    
    # 证书即将过期 (7 天)
    - alert: IngressCertificateExpiringCritical
      expr: |
        (nginx_ingress_controller_ssl_expire_time_seconds - time()) < 604800
      for: 1h
      labels:
        severity: critical
      annotations:
        summary: "Ingress 证书即将过期 (紧急)"
        description: "证书 {{ $labels.host }} 将在 {{ $value | humanizeDuration }} 后过期，请立即更新！"
    
    # 证书已过期
    - alert: IngressCertificateExpired
      expr: |
        nginx_ingress_controller_ssl_expire_time_seconds < time()
      for: 0m
      labels:
        severity: critical
      annotations:
        summary: "Ingress 证书已过期"
        description: "证书 {{ $labels.host }} 已过期！"
```

---

## 四、日志分析

### 4.1 访问日志格式

```yaml
# ConfigMap 配置 JSON 格式日志
apiVersion: v1
kind: ConfigMap
metadata:
  name: ingress-nginx-controller
  namespace: ingress-nginx
data:
  log-format-escape-json: "true"
  log-format-upstream: |
    {
      "time": "$time_iso8601",
      "remote_addr": "$remote_addr",
      "x_forwarded_for": "$proxy_add_x_forwarded_for",
      "request_id": "$req_id",
      "remote_user": "$remote_user",
      "bytes_sent": $bytes_sent,
      "request_time": $request_time,
      "status": $status,
      "vhost": "$host",
      "request_proto": "$server_protocol",
      "path": "$uri",
      "request_query": "$args",
      "request_length": $request_length,
      "duration": $request_time,
      "method": "$request_method",
      "http_referrer": "$http_referer",
      "http_user_agent": "$http_user_agent",
      "upstream_addr": "$upstream_addr",
      "upstream_response_time": "$upstream_response_time",
      "upstream_response_length": "$upstream_response_length",
      "upstream_status": "$upstream_status"
    }
```

### 4.2 日志收集配置 (Fluent Bit)

```yaml
apiVersion: v1
kind: ConfigMap
metadata:
  name: fluent-bit-config
  namespace: logging
data:
  fluent-bit.conf: |
    [SERVICE]
        Flush         1
        Log_Level     info
        Daemon        off
        Parsers_File  parsers.conf

    [INPUT]
        Name              tail
        Path              /var/log/containers/ingress-nginx-controller*.log
        Parser            docker
        Tag               ingress.*
        Refresh_Interval  5
        Mem_Buf_Limit     5MB

    [FILTER]
        Name          parser
        Match         ingress.*
        Key_Name      log
        Parser        json
        Reserve_Data  True

    [OUTPUT]
        Name            es
        Match           ingress.*
        Host            elasticsearch.logging.svc.cluster.local
        Port            9200
        Index           ingress-logs
        Type            _doc

  parsers.conf: |
    [PARSER]
        Name        docker
        Format      json
        Time_Key    time
        Time_Format %Y-%m-%dT%H:%M:%S.%L

    [PARSER]
        Name        json
        Format      json
```

### 4.3 日志查询示例 (Elasticsearch/Kibana)

```json
// 查询 5xx 错误
{
  "query": {
    "bool": {
      "must": [
        { "range": { "status": { "gte": 500, "lt": 600 } } },
        { "range": { "@timestamp": { "gte": "now-1h" } } }
      ]
    }
  },
  "sort": [{ "@timestamp": "desc" }]
}

// 查询高延迟请求
{
  "query": {
    "bool": {
      "must": [
        { "range": { "request_time": { "gte": 1 } } },
        { "range": { "@timestamp": { "gte": "now-1h" } } }
      ]
    }
  }
}

// 按 URI 聚合错误
{
  "size": 0,
  "query": {
    "range": { "status": { "gte": 400 } }
  },
  "aggs": {
    "by_uri": {
      "terms": { "field": "path.keyword", "size": 20 }
    }
  }
}
```

---

## 五、故障排查指南

### 5.1 常见故障诊断流程

```
┌─────────────────────────────────────────────────────────────────────────────────┐
│                           Ingress 故障诊断流程                                   │
├─────────────────────────────────────────────────────────────────────────────────┤
│                                                                                  │
│  ┌──────────────────────────────────────────────────────────────────────────┐  │
│  │  Step 1: 确认症状                                                         │  │
│  │  - 完全无法访问？                                                         │  │
│  │  - 部分请求失败？                                                         │  │
│  │  - 响应缓慢？                                                            │  │
│  │  - 证书错误？                                                            │  │
│  └─────────────────────────────────┬────────────────────────────────────────┘  │
│                                    │                                           │
│                                    ▼                                           │
│  ┌──────────────────────────────────────────────────────────────────────────┐  │
│  │  Step 2: 检查 Ingress Controller                                         │  │
│  │  kubectl get pods -n ingress-nginx                                       │  │
│  │  kubectl describe pod -n ingress-nginx <pod-name>                        │  │
│  │  kubectl logs -n ingress-nginx <pod-name>                                │  │
│  └─────────────────────────────────┬────────────────────────────────────────┘  │
│                                    │                                           │
│                                    ▼                                           │
│  ┌──────────────────────────────────────────────────────────────────────────┐  │
│  │  Step 3: 检查 Ingress 资源                                                │  │
│  │  kubectl get ingress -A                                                  │  │
│  │  kubectl describe ingress <name>                                         │  │
│  │  kubectl get events --field-selector involvedObject.name=<ingress>       │  │
│  └─────────────────────────────────┬────────────────────────────────────────┘  │
│                                    │                                           │
│                                    ▼                                           │
│  ┌──────────────────────────────────────────────────────────────────────────┐  │
│  │  Step 4: 检查后端服务                                                     │  │
│  │  kubectl get svc,endpoints <service-name>                                │  │
│  │  kubectl get pods -l <service-selector>                                  │  │
│  │  kubectl exec -it <pod> -- curl localhost:<port>/health                 │  │
│  └─────────────────────────────────┬────────────────────────────────────────┘  │
│                                    │                                           │
│                                    ▼                                           │
│  ┌──────────────────────────────────────────────────────────────────────────┐  │
│  │  Step 5: 检查网络连通性                                                   │  │
│  │  kubectl exec -n ingress-nginx <pod> -- curl <service>.<ns>.svc:port    │  │
│  │  kubectl run tmp --rm -it --image=nicolaka/netshoot -- /bin/bash        │  │
│  └──────────────────────────────────────────────────────────────────────────┘  │
│                                                                                  │
└─────────────────────────────────────────────────────────────────────────────────┘
```

### 5.2 常见问题与解决方案

| 问题 | 症状 | 可能原因 | 诊断命令 | 解决方案 |
|-----|------|---------|---------|---------|
| **502 Bad Gateway** | 请求返回 502 | 后端服务不可达 | `kubectl get endpoints` | 检查后端 Pod 状态 |
| **503 Service Unavailable** | 请求返回 503 | 无可用后端 | `kubectl get pods -l app=xxx` | 扩容或修复后端 |
| **504 Gateway Timeout** | 请求超时 | 后端响应过慢 | 查看 upstream_response_time | 增加超时或优化后端 |
| **404 Not Found** | 路径不存在 | Ingress 规则不匹配 | `kubectl describe ingress` | 检查 path 配置 |
| **证书错误** | HTTPS 握手失败 | 证书过期/不匹配 | `openssl s_client` | 更新证书 |
| **重定向循环** | ERR_TOO_MANY_REDIRECTS | 配置冲突 | 检查 ssl-redirect | 调整重定向配置 |
| **配置不生效** | 新规则无效 | 配置重载失败 | 检查 controller 日志 | 修复配置语法 |
| **高延迟** | 响应缓慢 | 资源不足/后端慢 | 检查 Prometheus 指标 | 扩容/优化 |
| **连接拒绝** | Connection refused | 控制器未运行 | `kubectl get pods` | 重启控制器 |

### 5.3 诊断命令速查

```bash
# ========== Ingress Controller 检查 ==========
# 查看控制器 Pod 状态
kubectl get pods -n ingress-nginx -o wide

# 查看控制器日志
kubectl logs -n ingress-nginx -l app.kubernetes.io/component=controller --tail=100 -f

# 查看控制器详情
kubectl describe pod -n ingress-nginx -l app.kubernetes.io/component=controller

# 检查 NGINX 配置
kubectl exec -n ingress-nginx <controller-pod> -- cat /etc/nginx/nginx.conf

# 检查特定 server 配置
kubectl exec -n ingress-nginx <controller-pod> -- cat /etc/nginx/nginx.conf | grep -A 50 "server_name example.com"

# 测试 NGINX 配置语法
kubectl exec -n ingress-nginx <controller-pod> -- nginx -t

# 查看 NGINX 进程状态
kubectl exec -n ingress-nginx <controller-pod> -- ps aux

# ========== Ingress 资源检查 ==========
# 列出所有 Ingress
kubectl get ingress -A -o wide

# 查看 Ingress 详情
kubectl describe ingress <ingress-name> -n <namespace>

# 查看 Ingress YAML
kubectl get ingress <ingress-name> -n <namespace> -o yaml

# 查看 IngressClass
kubectl get ingressclass

# 查看 Ingress 事件
kubectl get events --field-selector involvedObject.kind=Ingress

# ========== 后端服务检查 ==========
# 检查 Service 和 Endpoints
kubectl get svc,endpoints <service-name> -n <namespace>

# 检查 Pod 状态
kubectl get pods -l <selector> -n <namespace> -o wide

# 测试后端连通性
kubectl exec -n ingress-nginx <controller-pod> -- curl -v http://<service>.<namespace>.svc.cluster.local:<port>/

# 从临时 Pod 测试
kubectl run tmp-debug --rm -it --image=curlimages/curl -- curl -v http://<service>.<namespace>.svc.cluster.local:<port>/

# ========== TLS/证书检查 ==========
# 查看证书 Secret
kubectl get secret <tls-secret> -n <namespace> -o yaml

# 验证证书内容
kubectl get secret <tls-secret> -n <namespace> -o jsonpath='{.data.tls\.crt}' | base64 -d | openssl x509 -text -noout

# 检查证书有效期
kubectl get secret <tls-secret> -n <namespace> -o jsonpath='{.data.tls\.crt}' | base64 -d | openssl x509 -dates -noout

# 测试 HTTPS 连接
curl -vI https://<domain> 2>&1 | grep -E "SSL|subject|issuer|expire"

# 检查 SNI
openssl s_client -connect <domain>:443 -servername <domain> 2>/dev/null | openssl x509 -noout -text

# ========== 网络排查 ==========
# DNS 解析测试
kubectl run tmp-dns --rm -it --image=busybox -- nslookup <service>.<namespace>.svc.cluster.local

# 端口连通性测试
kubectl run tmp-nc --rm -it --image=nicolaka/netshoot -- nc -zv <service> <port>

# 完整网络诊断
kubectl run tmp-netshoot --rm -it --image=nicolaka/netshoot -- /bin/bash
# 在容器内执行:
# curl -v http://service.namespace.svc.cluster.local:port/
# traceroute service.namespace.svc.cluster.local
# dig service.namespace.svc.cluster.local

# ========== 性能排查 ==========
# 查看控制器资源使用
kubectl top pod -n ingress-nginx

# 查看 NGINX 连接状态
kubectl exec -n ingress-nginx <controller-pod> -- curl -s localhost:10246/nginx_status

# 查看 Prometheus 指标
kubectl exec -n ingress-nginx <controller-pod> -- curl -s localhost:10254/metrics | grep nginx_ingress

# ========== 配置验证 ==========
# 验证 Ingress 配置 (dry-run)
kubectl apply -f ingress.yaml --dry-run=server

# 使用 ingress-nginx 验证器
kubectl exec -n ingress-nginx <controller-pod> -- /nginx-ingress-controller --validate-config
```

### 5.4 HTTP 状态码故障排查

| 状态码 | 描述 | 常见原因 | 排查步骤 |
|-------|------|---------|---------|
| **400** | Bad Request | 请求格式错误、Header 过大 | 检查请求、增大 buffer |
| **401** | Unauthorized | 认证失败 | 检查 auth 配置和凭证 |
| **403** | Forbidden | IP 黑名单、ACL 限制 | 检查白名单和 WAF |
| **404** | Not Found | 路径不匹配 | 检查 Ingress path 配置 |
| **413** | Payload Too Large | 请求体过大 | 增大 proxy-body-size |
| **429** | Too Many Requests | 限流触发 | 检查限流配置 |
| **499** | Client Closed Request | 客户端提前关闭 | 检查客户端超时 |
| **500** | Internal Server Error | 后端错误 | 检查后端应用日志 |
| **502** | Bad Gateway | 后端不可达 | 检查后端 Pod/Service |
| **503** | Service Unavailable | 无可用后端 | 检查 Endpoints |
| **504** | Gateway Timeout | 后端超时 | 增加超时或优化后端 |

---

## 六、分布式追踪

### 6.1 启用 OpenTracing

```yaml
# ConfigMap 配置
apiVersion: v1
kind: ConfigMap
metadata:
  name: ingress-nginx-controller
  namespace: ingress-nginx
data:
  # 启用 OpenTracing
  enable-opentracing: "true"
  # Jaeger 配置
  jaeger-collector-host: jaeger-collector.tracing.svc.cluster.local
  jaeger-service-name: nginx-ingress
  jaeger-sampler-type: const
  jaeger-sampler-param: "1"
```

### 6.2 请求追踪配置

```yaml
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: traced-ingress
  annotations:
    # 启用追踪
    nginx.ingress.kubernetes.io/enable-opentracing: "true"
    # 信任入站 span
    nginx.ingress.kubernetes.io/opentracing-trust-incoming-span: "true"
spec:
  ingressClassName: nginx
  rules:
  - host: app.example.com
    http:
      paths:
      - path: /
        pathType: Prefix
        backend:
          service:
            name: app-service
            port:
              number: 80
```

### 6.3 传递请求 ID

```yaml
apiVersion: v1
kind: ConfigMap
metadata:
  name: ingress-nginx-controller
  namespace: ingress-nginx
data:
  # 生成请求 ID
  generate-request-id: "true"
  
---
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: request-id-ingress
  annotations:
    nginx.ingress.kubernetes.io/configuration-snippet: |
      # 传递请求 ID 到后端
      proxy_set_header X-Request-ID $req_id;
      # 在响应中返回请求 ID
      add_header X-Request-ID $req_id always;
spec:
  ingressClassName: nginx
  rules:
  - host: app.example.com
    http:
      paths:
      - path: /
        pathType: Prefix
        backend:
          service:
            name: app-service
            port:
              number: 80
```

---

## 七、健康检查与自愈

### 7.1 控制器健康检查

```yaml
# 控制器健康检查端点
# /healthz - 存活检查
# /readyz - 就绪检查

apiVersion: apps/v1
kind: Deployment
metadata:
  name: ingress-nginx-controller
spec:
  template:
    spec:
      containers:
      - name: controller
        livenessProbe:
          httpGet:
            path: /healthz
            port: 10254
            scheme: HTTP
          initialDelaySeconds: 10
          periodSeconds: 10
          timeoutSeconds: 1
          successThreshold: 1
          failureThreshold: 5
        readinessProbe:
          httpGet:
            path: /healthz
            port: 10254
            scheme: HTTP
          initialDelaySeconds: 10
          periodSeconds: 10
          timeoutSeconds: 1
          successThreshold: 1
          failureThreshold: 3
```

### 7.2 后端健康检查

```yaml
# 通过注解配置后端健康检查
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: healthcheck-ingress
  annotations:
    # 健康检查路径
    nginx.ingress.kubernetes.io/healthcheck-path: "/health"
    # 自定义 upstream 配置
    nginx.ingress.kubernetes.io/upstream-hash-by: "$request_uri"
spec:
  ingressClassName: nginx
  rules:
  - host: app.example.com
    http:
      paths:
      - path: /
        pathType: Prefix
        backend:
          service:
            name: app-service
            port:
              number: 80
```

---

## 八、性能调优

### 8.1 性能调优参数

| 参数 | 默认值 | 调优建议 | 说明 |
|-----|-------|---------|------|
| `worker-processes` | auto | CPU 核数 | 工作进程数 |
| `worker-connections` | 16384 | 65535 | 单进程最大连接 |
| `keep-alive` | 75 | 75 | Keep-alive 超时 |
| `keep-alive-requests` | 1000 | 10000 | 单连接最大请求 |
| `upstream-keepalive-connections` | 320 | 320-1000 | 上游连接池 |
| `proxy-buffer-size` | 4k | 16k | 代理缓冲区 |
| `proxy-buffers-number` | 4 | 8 | 缓冲区数量 |

### 8.2 高性能配置示例

```yaml
apiVersion: v1
kind: ConfigMap
metadata:
  name: ingress-nginx-controller
  namespace: ingress-nginx
data:
  # 工作进程
  worker-processes: "auto"
  worker-cpu-affinity: "auto"
  worker-shutdown-timeout: "300s"
  
  # 连接配置
  max-worker-connections: "65535"
  max-worker-open-files: "65535"
  
  # Keep-alive
  keep-alive: "75"
  keep-alive-requests: "10000"
  upstream-keepalive-connections: "500"
  upstream-keepalive-requests: "10000"
  upstream-keepalive-timeout: "60"
  
  # 缓冲区
  proxy-buffer-size: "16k"
  proxy-buffers-number: "8"
  proxy-body-size: "100m"
  
  # 压缩
  use-gzip: "true"
  gzip-level: "5"
  gzip-min-length: "1000"
  gzip-types: "application/json application/javascript text/css text/plain"
  
  # HTTP/2
  use-http2: "true"
  
  # 日志
  access-log-path: "/var/log/nginx/access.log"
  error-log-level: "warn"
```

---

**监控与排查原则**: 完善指标 → 主动告警 → 快速定位 → 根因分析

---

**表格底部标记**: Kusheet Project, 作者 Allen Galler (allengaller@gmail.com)
