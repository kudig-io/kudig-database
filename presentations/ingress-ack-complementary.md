# Ingress ACK 补充技术文档

> **文档类型**: 技术补充资料 | **适用环境**: 阿里云专有云 & 公共云 | **重点产品**: ACK  

---

## 目录

1. [负载均衡器选择策略](#1-负载均衡器选择策略)
2. [安全加固实践](#2-安全加固实践)
3. [监控告警配置](#3-监控告警配置)
4. [故障排查手册](#4-故障排查手册)
5. [性能调优指南](#5-性能调优指南)
6. [多环境管理](#6-多环境管理)

---

## 1. 负载均衡器选择策略

### 1.1 产品对比矩阵

| 特性 | CLB (传统型) | NLB (网络型) | ALB (应用型) |
|------|-------------|-------------|-------------|
| **工作层级** | 四层 (TCP/UDP) | 四层 (TCP/UDP) | 七层 (HTTP/HTTPS) |
| **性能规格** | 固定规格 | 弹性按需 | 弹性按需 |
| **最大连接数** | 100万 | 1亿+ | 100万+ |
| **新建连接数** | 10万 CPS | 100万+ CPS | 10万+ CPS |
| **SSL卸载** | 支持 | 不支持 | 原生支持 |
| **HTTP路由** | 不支持 | 不支持 | 丰富路由功能 |
| **WebSocket** | 有限支持 | 支持 | 原生支持 |
| **gRPC** | 有限支持 | 支持 | 原生支持 |
| **成本模式** | 包年包月/按量 | 按量付费 | 按量付费 |
| **适用场景** | 简单TCP负载 | 高并发游戏/直播 | Web应用/API网关 |

### 1.2 选型决策流程

```
开始
  │
  ├─→ 是否需要HTTP/HTTPS高级功能？
  │      │
  │      ├─ 是 → 选择 ALB
  │      │
  │      └─ 否 ↓
  │
  ├─→ 是否需要超高并发性能？
  │      │
  │      ├─ 是 → 选择 NLB
  │      │
  │      └─ 否 ↓
  │
  ├─→ 是否有预算限制？
  │      │
  │      ├─ 是 → 选择 CLB (包年包月)
  │      │
  │      └─ 否 ↓
  │
  └─→ 默认选择 → ALB (功能最全面)
```

### 1.3 典型应用场景配置

#### 游戏服务器负载均衡 (NLB)
```yaml
# 高并发游戏服务器配置
apiVersion: v1
kind: Service
metadata:
  name: game-server-nlb
  annotations:
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-type: "nlb"
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-spec: "nlb.s1.small"
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-zone-mapping: |
      [
        {"ZoneId": "cn-hangzhou-a", "VSwitchId": "vsw-game-a"},
        {"ZoneId": "cn-hangzhou-b", "VSwitchId": "vsw-game-b"}
      ]
    # 高性能网络配置
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-health-check-connect-timeout: "1"
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-health-check-interval: "1"
spec:
  selector:
    app: game-server
  ports:
  - name: game-tcp
    port: 7777
    targetPort: 7777
    protocol: TCP
  type: LoadBalancer
```

#### Web应用负载均衡 (ALB)
```yaml
# 现代Web应用配置
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: web-app-alb
  annotations:
    kubernetes.io/ingress.class: alb
    alb.ingress.kubernetes.io/address-type: internet
    alb.ingress.kubernetes.io/vswitch-ids: "vsw-web-a,vsw-web-b"
    # HTTP/2 支持
    alb.ingress.kubernetes.io/http2-enabled: "true"
    # WebSocket 支持
    alb.ingress.kubernetes.io/websocket-enabled: "true"
    # 压缩支持
    alb.ingress.kubernetes.io/compression-enabled: "true"
spec:
  tls:
  - hosts:
    - app.example.com
    secretName: app-tls
  rules:
  - host: app.example.com
    http:
      paths:
      - path: /api
        pathType: Prefix
        backend:
          service:
            name: api-service
            port:
              number: 8080
      - path: /ws
        pathType: Prefix
        backend:
          service:
            name: websocket-service
            port:
              number: 8080
      - path: /
        pathType: Prefix
        backend:
          service:
            name: frontend-service
            port:
              number: 80
```

#### 混合负载均衡架构
```yaml
# 同时使用多种负载均衡器的混合架构
---
# 内部服务使用NLB (高性能)
apiVersion: v1
kind: Service
metadata:
  name: internal-nlb
  annotations:
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-type: "nlb"
spec:
  selector:
    app: internal-service
  ports:
  - port: 80
    targetPort: 8080
  type: LoadBalancer

---
# 外部Web服务使用ALB (功能丰富)
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: external-alb
  annotations:
    kubernetes.io/ingress.class: alb
spec:
  tls:
  - hosts:
    - web.example.com
    secretName: web-tls
  rules:
  - host: web.example.com
    http:
      paths:
      - path: /
        pathType: Prefix
        backend:
          service:
            name: web-service
            port:
              number: 80
```

---

## 2. 安全加固实践

### 2.1 网络层面安全

#### 安全组精细化配置
```yaml
# 生产环境安全组配置
apiVersion: v1
kind: Service
metadata:
  name: secure-service
  annotations:
    # 绑定专用安全组
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-security-group-id: "sg-prod-web"
    
    # 访问控制列表
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-access-control-enable: "on"
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-access-control-type: "white"
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-access-control-list: |
      192.168.0.0/16,  # 内部网络
      10.0.0.0/8,      # VPC网络
      203.0.113.0/24   # 办公网络
    
    # 删除保护
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-delete-protection: "on"
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-modification-protection: "ConsoleProtection"
spec:
  selector:
    app: secure-app
  ports:
  - port: 443
    targetPort: 8443
  type: LoadBalancer
```

#### NetworkPolicy 网络策略
```yaml
# 严格的网络隔离策略
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: ingress-isolation-policy
  namespace: production
spec:
  podSelector:
    matchLabels:
      app: web-app
  policyTypes:
  - Ingress
  - Egress
  
  # 入站规则 - 仅允许必要的流量
  ingress:
  # 允许来自负载均衡器的流量
  - from:
    - namespaceSelector: {}
      podSelector:
        matchLabels:
          component: load-balancer
    ports:
    - protocol: TCP
      port: 80
    - protocol: TCP
      port: 443
  
  # 允许来自监控系统的流量
  - from:
    - namespaceSelector:
        matchLabels:
          name: monitoring
    ports:
    - protocol: TCP
      port: 8080  # metrics端口
  
  # 允许来自内部服务的流量
  - from:
    - namespaceSelector:
        matchLabels:
          environment: production
      podSelector:
        matchLabels:
          tier: backend
    ports:
    - protocol: TCP
      port: 8080
  
  # 出站规则 - 限制对外访问
  egress:
  # 允许访问数据库
  - to:
    - namespaceSelector:
        matchLabels:
          name: database
    ports:
    - protocol: TCP
      port: 3306
    - protocol: TCP
      port: 5432
  
  # 允许访问缓存服务
  - to:
    - namespaceSelector:
        matchLabels:
          name: cache
    ports:
    - protocol: TCP
      port: 6379
    - protocol: TCP
      port: 11211
  
  # 允许DNS查询
  - to:
    - namespaceSelector:
        matchLabels:
          name: kube-system
      podSelector:
        matchLabels:
          k8s-app: kube-dns
    ports:
    - protocol: UDP
      port: 53
    - protocol: TCP
      port: 53
```

### 2.2 应用层面安全

#### WAF集成配置
```yaml
# 集成阿里云WAF的安全配置
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: waf-secured-ingress
  annotations:
    # 基础安全配置
    nginx.ingress.kubernetes.io/ssl-redirect: "true"
    nginx.ingress.kubernetes.io/force-ssl-redirect: "true"
    
    # 安全响应头
    nginx.ingress.kubernetes.io/configuration-snippet: |
      # 防点击劫持
      add_header X-Frame-Options "DENY" always;
      
      # 防MIME类型嗅探
      add_header X-Content-Type-Options "nosniff" always;
      
      # XSS防护
      add_header X-XSS-Protection "1; mode=block" always;
      
      # 强制HTTPS
      add_header Strict-Transport-Security "max-age=31536000; includeSubDomains; preload" always;
      
      # 内容安全策略
      add_header Content-Security-Policy "default-src 'self'; script-src 'self' 'unsafe-inline'; style-src 'self' 'unsafe-inline'; img-src 'self' data:;" always;
      
      # 防信息泄露
      proxy_hide_header Server;
      proxy_hide_header X-Powered-By;
      
      # 请求大小限制
      client_max_body_size 10m;
      
      # 速率限制配置
      limit_req_zone $binary_remote_addr zone=login:10m rate=10r/m;
      limit_req_zone $binary_remote_addr zone=api:10m rate=1000r/m;
      limit_req_zone $binary_remote_addr zone=general:10m rate=100r/s;
      
      # 登录接口保护
      location /api/login {
        limit_req zone=login burst=5 nodelay;
        proxy_pass http://auth-service;
      }
      
      # API接口保护
      location /api/ {
        limit_req zone=api burst=100 nodelay;
        proxy_pass http://api-service;
      }
      
      # 通用保护
      location / {
        limit_req zone=general burst=20 nodelay;
        proxy_pass http://web-service;
      }
spec:
  ingressClassName: nginx
  tls:
  - hosts:
    - app.example.com
    secretName: app-tls
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

#### OAuth2 认证集成
```yaml
# 外部认证服务集成
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: oauth2-protected-ingress
  annotations:
    # 外部认证配置
    nginx.ingress.kubernetes.io/auth-url: "http://oauth2-proxy.auth-system.svc.cluster.local/oauth2/auth"
    nginx.ingress.kubernetes.io/auth-signin: "https://auth.example.com/oauth2/start?rd=$scheme://$host$request_uri"
    nginx.ingress.kubernetes.io/auth-response-headers: "X-Auth-Request-User,X-Auth-Request-Email,X-Auth-Request-Groups"
    
    # 认证缓存
    nginx.ingress.kubernetes.io/auth-cache-key: "$cookie_oauth2_proxy"
    nginx.ingress.kubernetes.io/auth-cache-duration: "200 202 10m, 401 5m"
    
    # 强制认证
    nginx.ingress.kubernetes.io/auth-always-set-cookie: "true"
    
    # 跳过认证的路径
    nginx.ingress.kubernetes.io/auth-snippet: |
      location /api/public {
        auth_request off;
        proxy_pass http://api-service;
      }
      location /health {
        auth_request off;
        proxy_pass http://health-service;
      }
spec:
  ingressClassName: nginx
  tls:
  - hosts:
    - app.example.com
    secretName: app-tls
  rules:
  - host: app.example.com
    http:
      paths:
      - path: /
        pathType: Prefix
        backend:
          service:
            name: protected-app
            port:
              number: 80
```

### 2.3 数据传输安全

#### mTLS 双向认证
```yaml
# mTLS配置示例
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: mtls-ingress
  annotations:
    # 启用客户端证书验证
    nginx.ingress.kubernetes.io/auth-tls-secret: "default/client-ca-secret"
    nginx.ingress.kubernetes.io/auth-tls-verify-client: "on"
    nginx.ingress.kubernetes.io/auth-tls-verify-depth: "2"
    nginx.ingress.kubernetes.io/auth-tls-pass-certificate-to-upstream: "true"
    
    # 后端TLS验证
    nginx.ingress.kubernetes.io/proxy-ssl-secret: "default/backend-tls"
    nginx.ingress.kubernetes.io/proxy-ssl-verify: "on"
    nginx.ingress.kubernetes.io/proxy-ssl-verify-depth: "2"
spec:
  ingressClassName: nginx
  tls:
  - hosts:
    - secure.example.com
    secretName: server-tls
  rules:
  - host: secure.example.com
    http:
      paths:
      - path: /
        pathType: Prefix
        backend:
          service:
            name: secure-service
            port:
              number: 443
```

---

## 3. 监控告警配置

### 3.1 完整监控体系

#### Prometheus 监控配置
```yaml
# 完整的监控配置
apiVersion: v1
kind: ConfigMap
metadata:
  name: ingress-monitoring-config
  namespace: monitoring
data:
  prometheus.yml: |
    global:
      scrape_interval: 15s
      evaluation_interval: 15s
    
    rule_files:
      - "/etc/prometheus/rules/*.yml"
    
    scrape_configs:
      # Ingress Controller指标
      - job_name: 'ingress-nginx'
        kubernetes_sd_configs:
        - role: pod
        relabel_configs:
        - source_labels: [__meta_kubernetes_pod_label_app_kubernetes_io_component]
          action: keep
          regex: controller
        - source_labels: [__address__, __meta_kubernetes_pod_annotation_prometheus_io_port]
          action: replace
          regex: ([^:]+)(?::\d+)?;(\d+)
          replacement: $1:$2
          target_label: __address__
        - source_labels: [__meta_kubernetes_namespace]
          target_label: namespace
        - source_labels: [__meta_kubernetes_pod_name]
          target_label: pod

---
# ServiceMonitor配置
apiVersion: monitoring.coreos.com/v1
kind: ServiceMonitor
metadata:
  name: ingress-nginx-controller
  namespace: monitoring
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
    relabelings:
    - sourceLabels: [__meta_kubernetes_pod_node_name]
      targetLabel: node
    - sourceLabels: [__meta_kubernetes_namespace]
      targetLabel: namespace
    - sourceLabels: [__meta_kubernetes_pod_name]
      targetLabel: pod
    metricRelabelings:
    - sourceLabels: [__name__]
      regex: 'nginx_ingress_controller_(.*)'
      targetLabel: __name__
      replacement: 'ingress_${1}'
```

#### 关键监控指标仪表板
```json
{
  "dashboard": {
    "title": "Ingress Controller 全方位监控",
    "timezone": "browser",
    "panels": [
      {
        "title": "核心指标概览",
        "type": "row",
        "panels": [
          {
            "title": "当前QPS",
            "type": "stat",
            "datasource": "Prometheus",
            "targets": [
              {
                "expr": "sum(rate(nginx_ingress_controller_requests[5m]))",
                "instant": true
              }
            ],
            "thresholds": [
              {"color": "green", "value": null},
              {"color": "orange", "value": 1000},
              {"color": "red", "value": 5000}
            ]
          },
          {
            "title": "5xx错误率",
            "type": "stat",
            "datasource": "Prometheus",
            "targets": [
              {
                "expr": "sum(rate(nginx_ingress_controller_requests{status=~\"5..\"}[5m])) / sum(rate(nginx_ingress_controller_requests[5m])) * 100",
                "instant": true
              }
            ],
            "thresholds": [
              {"color": "green", "value": null},
              {"color": "orange", "value": 1},
              {"color": "red", "value": 5}
            ],
            "unit": "percent"
          },
          {
            "title": "P99延迟",
            "type": "stat",
            "datasource": "Prometheus",
            "targets": [
              {
                "expr": "histogram_quantile(0.99, sum(rate(nginx_ingress_controller_request_duration_seconds_bucket[5m])) by (le))",
                "instant": true
              }
            ],
            "thresholds": [
              {"color": "green", "value": null},
              {"color": "orange", "value": 0.5},
              {"color": "red", "value": 1}
            ],
            "unit": "s"
          }
        ]
      },
      {
        "title": "详细性能指标",
        "type": "row",
        "panels": [
          {
            "title": "请求速率趋势",
            "type": "timeseries",
            "datasource": "Prometheus",
            "targets": [
              {
                "expr": "sum by(status) (rate(nginx_ingress_controller_requests[5m]))",
                "legendFormat": "{{status}}"
              }
            ]
          },
          {
            "title": "延迟分布",
            "type": "heatmap",
            "datasource": "Prometheus",
            "targets": [
              {
                "expr": "sum(rate(nginx_ingress_controller_request_duration_seconds_bucket[5m])) by (le)",
                "format": "heatmap"
              }
            ]
          },
          {
            "title": "连接状态",
            "type": "timeseries",
            "datasource": "Prometheus",
            "targets": [
              {
                "expr": "nginx_ingress_controller_nginx_process_connections",
                "legendFormat": "{{state}}"
              }
            ]
          }
        ]
      }
    ]
  }
}
```

### 3.2 智能告警规则

```yaml
# 智能告警规则配置
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: intelligent-ingress-alerts
  namespace: monitoring
spec:
  groups:
  - name: ingress-intelligent-alerts
    rules:
    # 智能基线告警
    - alert: IngressAnomalyDetection
      expr: |
        (
          abs(
            rate(nginx_ingress_controller_requests[5m]) 
            - avg_over_time(rate(nginx_ingress_controller_requests[1h])[1d:5m])
          ) 
          / avg_over_time(rate(nginx_ingress_controller_requests[1h])[1d:5m])
        ) > 2
      for: 10m
      labels:
        severity: warning
      annotations:
        summary: "Ingress流量异常"
        description: "当前流量与历史基线偏差超过200%，可能存在异常"
    
    # 多维度错误率告警
    - alert: MultiDimensionErrorRate
      expr: |
        sum by(host, path) (
          rate(nginx_ingress_controller_requests{status=~"5.."}[5m])
        ) / sum by(host, path) (
          rate(nginx_ingress_controller_requests[5m])
        ) > 0.05
      for: 5m
      labels:
        severity: critical
      annotations:
        summary: "特定路径错误率过高"
        description: "主机{{ $labels.host }}路径{{ $labels.path }} 5xx错误率: {{ $value | humanizePercentage }}"
    
    # 性能退化预警
    - alert: PerformanceDegradation
      expr: |
        (
          histogram_quantile(0.95, sum(rate(nginx_ingress_controller_request_duration_seconds_bucket[5m])) by (le))
          >
          1.5 * avg_over_time(
            histogram_quantile(0.95, sum(rate(nginx_ingress_controller_request_duration_seconds_bucket[1h])) by (le))[1d:1h]
          )
        )
      for: 15m
      labels:
        severity: warning
      annotations:
        summary: "Ingress性能退化"
        description: "P95延迟相比历史平均水平增长超过50%"
```

---

## 4. 故障排查手册

### 4.1 系统化诊断流程

```bash
#!/bin/bash
# 完整的Ingress故障诊断脚本

DIAGNOSE_TIME=$(date '+%Y-%m-%d %H:%M:%S')
LOG_FILE="/tmp/ingress-diagnose-${DIAGNOSE_TIME}.log"

exec > >(tee -a "$LOG_FILE") 2>&1

echo "========================================="
echo "Ingress 系统诊断报告"
echo "诊断时间: $DIAGNOSE_TIME"
echo "========================================="
echo

# 1. 基础环境检查
echo "1. 基础环境检查"
echo "================"
echo "Kubernetes版本:"
kubectl version --short
echo

echo "节点状态:"
kubectl get nodes -o wide
echo

echo "Ingress Controller状态:"
kubectl get pods -n ingress-nginx -o wide
echo

# 2. 配置状态检查
echo "2. 配置状态检查"
echo "==============="
echo "Ingress资源列表:"
kubectl get ingress -A -o wide
echo

echo "IngressClass配置:"
kubectl get ingressclass -o wide
echo

echo "Service配置:"
kubectl get svc -n ingress-nginx -o wide
echo

# 3. 详细诊断
echo "3. 详细诊断"
echo "==========="
for ingress in $(kubectl get ingress -A -o jsonpath='{range .items[*]}{.metadata.namespace}/{.metadata.name}{"\n"}{end}'); do
  echo "检查 Ingress: $ingress"
  echo "----------------------"
  kubectl describe ingress -n ${ingress%/*} ${ingress#*/} | grep -E "(Events|Warning|Error|Status)"
  echo
done

# 4. 后端服务检查
echo "4. 后端服务检查"
echo "==============="
echo "Endpoints状态:"
kubectl get endpoints -n production
echo

# 5. 网络连通性测试
echo "5. 网络连通性测试"
echo "================="
echo "DNS解析测试:"
kubectl run dns-test --rm -it --image=busybox -- nslookup kubernetes.default
echo

echo "服务连通性测试:"
kubectl run curl-test --rm -it --image=curlimages/curl -- curl -I http://app-service.production.svc.cluster.local
echo

# 6. 性能指标检查
echo "6. 性能指标检查"
echo "==============="
echo "资源使用情况:"
kubectl top pods -n ingress-nginx
echo

echo "关键指标:"
kubectl exec -n ingress-nginx deploy/ingress-nginx-controller -- curl -s localhost:10254/metrics | \
  grep -E "(nginx_ingress_controller_requests|nginx_ingress_controller_request_duration|config_last_reload)" | \
  head -20
echo

# 7. 错误日志分析
echo "7. 错误日志分析"
echo "==============="
echo "最近错误日志:"
kubectl logs -n ingress-nginx -l app.kubernetes.io/component=controller --since=1h | \
  grep -i "error\|warning\|failed" | \
  tail -10
echo

echo "诊断完成，详细日志保存在: $LOG_FILE"
```

### 4.2 常见故障排查表

| 故障现象 | 可能原因 | 诊断命令 | 解决方案 |
|----------|----------|----------|----------|
| **502 Bad Gateway** | 后端服务不可达 | `kubectl get endpoints <service>` | 检查后端Pod状态和Service配置 |
| **503 Service Unavailable** | 无健康后端实例 | `kubectl get pods -l <selector>` | 扩容后端服务或检查健康检查配置 |
| **504 Gateway Timeout** | 后端响应超时 | 查看`upstream_response_time`指标 | 增加上游超时配置或优化后端性能 |
| **404 Not Found** | 路径不匹配 | `kubectl describe ingress <name>` | 检查Ingress path配置和匹配规则 |
| **证书错误** | TLS握手失败 | `openssl s_client -connect <domain>:443` | 更新证书或检查证书链完整性 |
| **重定向循环** | 配置冲突 | 检查`ssl-redirect`注解 | 调整HTTPS重定向配置 |
| **高延迟** | 性能瓶颈 | `kubectl top pods` + Prometheus指标 | 扩容Controller或优化配置 |
| **连接拒绝** | 端口未开放 | `telnet <lb-ip> <port>` | 检查Service端口配置和安全组规则 |

### 4.3 紧急恢复操作

```bash
# 紧急故障恢复操作手册

# 1. 快速重启Ingress Controller
echo "正在重启Ingress Controller..."
kubectl rollout restart deployment/ingress-nginx-controller -n ingress-nginx
kubectl rollout status deployment/ingress-nginx-controller -n ingress-nginx --timeout=300s

# 2. 回滚到上一个稳定版本
echo "正在回滚到上一个版本..."
kubectl rollout undo deployment/ingress-nginx-controller -n ingress-nginx

# 3. 临时禁用有问题的Ingress
echo "正在禁用问题Ingress..."
kubectl annotate ingress problematic-ingress -n production nginx.ingress.kubernetes.io/server-snippet='return 503;' --overwrite

# 4. 恢复默认后端
echo "正在恢复默认后端..."
kubectl patch ingress default-backend -n production -p '{"spec":{"defaultBackend":{"service":{"name":"maintenance-page","port":{"number":80}}}}}'

# 5. 紧急扩容
echo "正在进行紧急扩容..."
kubectl scale deployment ingress-nginx-controller -n ingress-nginx --replicas=5

# 6. 清理故障Pod
echo "正在清理故障Pod..."
kubectl delete pods -n ingress-nginx -l app.kubernetes.io/component=controller --field-selector=status.phase!=Running
```

---

## 5. 性能调优指南

### 5.1 基准性能测试

```bash
#!/bin/bash
# Ingress性能基准测试脚本

TEST_DURATION=${1:-300}  # 默认测试5分钟
CONCURRENT_USERS=${2:-100}  # 默认100并发用户
TARGET_URL=${3:-"https://test.example.com"}

echo "开始Ingress性能测试"
echo "测试时长: ${TEST_DURATION}秒"
echo "并发用户: ${CONCURRENT_USERS}"
echo "目标URL: ${TARGET_URL}"
echo

# 使用hey进行压力测试
hey -z "${TEST_DURATION}s" -c ${CONCURRENT_USERS} ${TARGET_URL} | tee /tmp/ingress-benchmark-$(date +%s).txt

# 分析结果
echo
echo "性能测试结果分析:"
cat /tmp/ingress-benchmark-$(date +%s).txt | grep -E "(Requests/sec|Latencies|Status codes)"

# 记录关键指标
QPS=$(cat /tmp/ingress-benchmark-$(date +%s).txt | grep "Requests/sec" | awk '{print $2}')
P95_LATENCY=$(cat /tmp/ingress-benchmark-$(date +%s).txt | grep "95%" | awk '{print $2}')
ERROR_RATE=$(cat /tmp/ingress-benchmark-$(date +%s).txt | grep "Status code distribution" -A 10 | grep "50" | awk '{print $2}' | sed 's/[()%]//g')

echo
echo "关键性能指标:"
echo "QPS: ${QPS}"
echo "P95延迟: ${P95_LATENCY}"
echo "错误率: ${ERROR_RATE}%"
```

### 5.2 性能优化参数调优

```yaml
# 性能优化的完整配置
apiVersion: v1
kind: ConfigMap
metadata:
  name: ingress-performance-tuning
  namespace: ingress-nginx
data:
  # === 工作进程优化 ===
  worker-processes: "auto"
  worker-cpu-affinity: "auto"
  worker-shutdown-timeout: "300s"
  worker-rlimit-nofile: "131072"
  worker-connections: "65535"
  
  # === 内存优化 ===
  worker-buffer-size: "64k"
  client-body-buffer-size: "256k"
  client-header-buffer-size: "8k"
  large-client-header-buffers: "8 32k"
  
  # === 连接优化 ===
  keep-alive: "300"
  keep-alive-requests: "100000"
  keep-alive-timeout: "300"
  
  # === 上游优化 ===
  upstream-keepalive-connections: "1000"
  upstream-keepalive-requests: "10000"
  upstream-keepalive-timeout: "300"
  
  # === 缓存优化 ===
  proxy-buffer-size: "64k"
  proxy-buffers-number: "32"
  proxy-busy-buffers-size: "256k"
  proxy-temp-file-write-size: "512k"
  
  # === 超时优化 ===
  proxy-connect-timeout: "5"
  proxy-read-timeout: "300"
  proxy-send-timeout: "300"
  proxy-next-upstream-timeout: "60"
  
  # === 压缩优化 ===
  use-gzip: "true"
  gzip-level: "6"
  gzip-min-length: "256"
  gzip-types: |
    text/plain text/css application/json application/javascript 
    text/xml application/xml application/xml+rss text/javascript 
    application/x-javascript application/x-font-ttf font/opentype
  gzip-vary: "on"
  gzip-proxied: "any"
  
  # === HTTP/2优化 ===
  use-http2: "true"
  http2-max-concurrent-streams: "1000"
  http2-max-field-size: "32k"
  http2-max-header-size: "64k"
  http2-body-preread-size: "64k"
  
  # === 日志优化 ===
  access-log-path: "/dev/stdout"
  error-log-path: "/dev/stderr"
  log-format-escape-json: "true"
  log-format-upstream: |
    {
      "timestamp": "$time_iso8601",
      "request_id": "$req_id",
      "remote_addr": "$remote_addr",
      "xff": "$proxy_add_x_forwarded_for",
      "method": "$request_method",
      "host": "$host",
      "uri": "$uri",
      "query": "$query_string",
      "status": $status,
      "body_bytes": $bytes_sent,
      "request_time": $request_time,
      "upstream_addr": "$upstream_addr",
      "upstream_response_time": "$upstream_response_time",
      "upstream_status": "$upstream_status",
      "user_agent": "$http_user_agent"
    }
  
  # === 安全优化 ===
  server-tokens: "false"
  hide-headers: "X-Powered-By,Server"
  
  # === 监控优化 ===
  enable-prometheus-metrics: "true"
  metrics-per-host: "true"
```

### 5.3 容量规划指南

```yaml
# 不同规模的容量规划建议
capacity_planning:
  small_scale:  # 小规模 (QPS < 1000)
    replicas: 2
    resources:
      requests:
        cpu: "200m"
        memory: "256Mi"
      limits:
        cpu: "1000m"
        memory: "1Gi"
    config_tuning:
      worker_processes: "2"
      worker_connections: "16384"
      keep_alive_requests: "1000"
    
  medium_scale:  # 中规模 (QPS 1000-10000)
    replicas: 3
    resources:
      requests:
        cpu: "500m"
        memory: "512Mi"
      limits:
        cpu: "2000m"
        memory: "2Gi"
    config_tuning:
      worker_processes: "auto"
      worker_connections: "32768"
      keep_alive_requests: "10000"
      upstream_keepalive_connections: "500"
    
  large_scale:  # 大规模 (QPS 10000-100000)
    replicas: 5
    resources:
      requests:
        cpu: "1000m"
        memory: "1Gi"
      limits:
        cpu: "4000m"
        memory: "4Gi"
    config_tuning:
      worker_processes: "auto"
      worker_connections: "65535"
      keep_alive_requests: "100000"
      upstream_keepalive_connections: "1000"
      proxy_buffers_number: "32"
    
  xlarge_scale:  # 超大规模 (QPS > 100000)
    replicas: 10
    resources:
      requests:
        cpu: "2000m"
        memory: "2Gi"
      limits:
        cpu: "8000m"
        memory: "8Gi"
    config_tuning:
      worker_processes: "auto"
      worker_connections: "65535"
      keep_alive_requests: "1000000"
      upstream_keepalive_connections: "2000"
      proxy_buffers_number: "64"
      use_gzip: "false"  # 高并发时考虑关闭压缩
```

---

## 6. 多环境管理

### 6.1 环境差异化配置

```yaml
# 使用Kustomize管理多环境配置

# base/ingress.yaml
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: app-ingress
  annotations:
    nginx.ingress.kubernetes.io/ssl-redirect: "$(SSL_REDIRECT)"
    nginx.ingress.kubernetes.io/proxy-body-size: "$(PROXY_BODY_SIZE)"
spec:
  ingressClassName: nginx
  rules:
  - host: "$(DOMAIN)"
    http:
      paths:
      - path: /
        pathType: Prefix
        backend:
          service:
            name: app-service
            port:
              number: 80

---
# overlays/dev/config.properties
SSL_REDIRECT=false
PROXY_BODY_SIZE=50m
DOMAIN=app.dev.example.com
REPLICAS=1
RESOURCES={"requests": {"cpu": "100m", "memory": "128Mi"}}

---
# overlays/staging/config.properties
SSL_REDIRECT=true
PROXY_BODY_SIZE=100m
DOMAIN=app.staging.example.com
REPLICAS=2
RESOURCES={"requests": {"cpu": "200m", "memory": "256Mi"}}

---
# overlays/prod/config.properties
SSL_REDIRECT=true
PROXY_BODY_SIZE=200m
DOMAIN=app.example.com
REPLICAS=3
RESOURCES={"requests": {"cpu": "500m", "memory": "512Mi"}}
```

### 6.2 蓝绿部署策略

```yaml
# 蓝绿部署配置示例
---
# 蓝色环境
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: blue-ingress
  annotations:
    nginx.ingress.kubernetes.io/canary: "false"
spec:
  ingressClassName: nginx
  tls:
  - hosts:
    - app.example.com
    secretName: app-tls
  rules:
  - host: app.example.com
    http:
      paths:
      - path: /
        pathType: Prefix
        backend:
          service:
            name: app-blue
            port:
              number: 80

---
# 绿色环境 (金丝雀5%流量)
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: green-canary-ingress
  annotations:
    nginx.ingress.kubernetes.io/canary: "true"
    nginx.ingress.kubernetes.io/canary-weight: "5"
spec:
  ingressClassName: nginx
  tls:
  - hosts:
    - app.example.com
    secretName: app-tls
  rules:
  - host: app.example.com
    http:
      paths:
      - path: /
        pathType: Prefix
        backend:
          service:
            name: app-green
            port:
              number: 80

---
# 流量切换脚本
#!/bin/bash
# blue-green-switch.sh

DESIRED_WEIGHT=${1:-100}  # 默认切换到100%绿色

echo "开始流量切换到绿色环境，权重: ${DESIRED_WEIGHT}%"

# 逐步增加绿色环境权重
for weight in 10 25 50 75 ${DESIRED_WEIGHT}; do
  echo "设置绿色环境权重为 ${weight}%"
  kubectl patch ingress green-canary-ingress -p "{\"metadata\":{\"annotations\":{\"nginx.ingress.kubernetes.io/canary-weight\":\"${weight}\"}}}"
  sleep 30  # 等待30秒观察效果
done

echo "流量切换完成"
```

### 6.3 灰度发布自动化

```yaml
# 灰度发布流水线配置
---
# Argo Rollouts 配置
apiVersion: argoproj.io/v1alpha1
kind: Rollout
metadata:
  name: app-rollout
spec:
  replicas: 5
  strategy:
    canary:
      steps:
      - setWeight: 10
        pause: {duration: 60s}
      - setWeight: 25
        pause: {duration: 120s}
      - setWeight: 50
        pause: {duration: 180s}
      - setWeight: 75
        pause: {duration: 120s}
      - setWeight: 100
      analysis:
        templates:
        - templateName: success-rate
        startingStep: 2  # 从第3步开始分析
  revisionHistoryLimit: 2
  selector:
    matchLabels:
      app: app-rollout
  template:
    metadata:
      labels:
        app: app-rollout
    spec:
      containers:
      - name: app
        image: app:v2.0.0
        ports:
        - containerPort: 8080
        resources:
          requests:
            cpu: 100m
            memory: 128Mi
          limits:
            cpu: 200m
            memory: 256Mi

---
# 分析模板
apiVersion: argoproj.io/v1alpha1
kind: AnalysisTemplate
metadata:
  name: success-rate
spec:
  args:
  - name: service-name
  metrics:
  - name: success-rate
    interval: 5m
    count: 3
    # 成功率必须大于95%
    successCondition: result[0] >= 0.95
    provider:
      prometheus:
        address: http://prometheus-operated:9090
        query: |
          sum(rate(nginx_ingress_controller_requests{status!~"5..", service="{{args.service-name}}"}[5m]))
          /
          sum(rate(nginx_ingress_controller_requests{service="{{args.service-name}}"}[5m]))
```

---

**文档结束**

*本文档提供了Kubernetes Ingress在阿里云环境下的全方位技术支撑，包括负载均衡选择、安全加固、监控告警、故障排查和性能优化等关键内容。建议结合实际业务需求和环境特点进行配置调整。*