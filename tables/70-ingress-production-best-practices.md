# 134 - Ingress 生产最佳实践

> **适用版本**: Kubernetes v1.25 - v1.32 | **最后更新**: 2026-01

---

## 一、生产环境检查清单

### 1.1 部署前检查清单

| 检查项 | 类别 | 状态 | 说明 |
|-------|------|------|------|
| **高可用部署** | 可用性 | ☐ | 至少 3 副本，跨可用区 |
| **资源配置** | 性能 | ☐ | 配置合理的 requests/limits |
| **HPA 配置** | 弹性 | ☐ | 配置自动扩缩容 |
| **PDB 配置** | 可用性 | ☐ | 配置 Pod 中断预算 |
| **反亲和性** | 可用性 | ☐ | 分散到不同节点/可用区 |
| **TLS 配置** | 安全 | ☐ | 启用 TLS 1.2+，强制 HTTPS |
| **证书管理** | 安全 | ☐ | 自动证书续期 (cert-manager) |
| **限流配置** | 安全 | ☐ | 配置请求限流 |
| **安全响应头** | 安全 | ☐ | 配置所有安全响应头 |
| **日志配置** | 运维 | ☐ | JSON 格式，请求 ID |
| **监控告警** | 运维 | ☐ | Prometheus 指标 + Grafana |
| **网络策略** | 安全 | ☐ | NetworkPolicy 隔离 |

### 1.2 运行时检查清单

| 检查项 | 检查命令 | 预期结果 |
|-------|---------|---------|
| 控制器运行状态 | `kubectl get pods -n ingress-nginx` | 所有 Pod Running |
| 配置重载状态 | 检查 `config_last_reload_successful` 指标 | 值为 1 |
| 证书有效期 | 检查 `ssl_expire_time_seconds` 指标 | > 30 天 |
| 错误率 | 检查 5xx 错误率 | < 0.1% |
| 延迟 | 检查 P99 延迟 | < 500ms |
| 资源使用 | `kubectl top pods -n ingress-nginx` | < 80% limits |

---

## 二、高可用部署架构

### 2.1 生产级部署架构

```
┌─────────────────────────────────────────────────────────────────────────────────┐
│                          生产级 Ingress 架构                                     │
├─────────────────────────────────────────────────────────────────────────────────┤
│                                                                                  │
│                           ┌──────────────────┐                                  │
│                           │   DNS / CDN      │                                  │
│                           │  (可选:WAF/DDoS) │                                  │
│                           └────────┬─────────┘                                  │
│                                    │                                            │
│                                    ▼                                            │
│                     ┌──────────────────────────────┐                           │
│                     │      Cloud Load Balancer     │                           │
│                     │   (跨可用区高可用)            │                           │
│                     └──────────────┬───────────────┘                           │
│                                    │                                            │
│           ┌────────────────────────┼────────────────────────┐                  │
│           │                        │                        │                  │
│           ▼                        ▼                        ▼                  │
│  ┌─────────────────┐    ┌─────────────────┐    ┌─────────────────┐           │
│  │  Ingress Pod    │    │  Ingress Pod    │    │  Ingress Pod    │           │
│  │  (Zone A)       │    │  (Zone B)       │    │  (Zone C)       │           │
│  │  ┌───────────┐  │    │  ┌───────────┐  │    │  ┌───────────┐  │           │
│  │  │  NGINX    │  │    │  │  NGINX    │  │    │  │  NGINX    │  │           │
│  │  └───────────┘  │    │  └───────────┘  │    │  └───────────┘  │           │
│  └─────────────────┘    └─────────────────┘    └─────────────────┘           │
│           │                        │                        │                  │
│           └────────────────────────┼────────────────────────┘                  │
│                                    │                                            │
│                                    ▼                                            │
│                     ┌──────────────────────────────┐                           │
│                     │      Backend Services        │                           │
│                     │   (Service Mesh / 直连)      │                           │
│                     └──────────────────────────────┘                           │
│                                                                                  │
└─────────────────────────────────────────────────────────────────────────────────┘
```

### 2.2 完整的生产部署配置

```yaml
# Namespace
apiVersion: v1
kind: Namespace
metadata:
  name: ingress-nginx
  labels:
    name: ingress-nginx
    app.kubernetes.io/name: ingress-nginx
---
# ServiceAccount
apiVersion: v1
kind: ServiceAccount
metadata:
  name: ingress-nginx
  namespace: ingress-nginx
---
# ConfigMap - 全局配置
apiVersion: v1
kind: ConfigMap
metadata:
  name: ingress-nginx-controller
  namespace: ingress-nginx
data:
  # 性能配置
  worker-processes: "auto"
  max-worker-connections: "65535"
  keep-alive: "75"
  keep-alive-requests: "10000"
  upstream-keepalive-connections: "500"
  
  # 安全配置
  ssl-protocols: "TLSv1.2 TLSv1.3"
  ssl-ciphers: "ECDHE-ECDSA-AES128-GCM-SHA256:ECDHE-RSA-AES128-GCM-SHA256:ECDHE-ECDSA-AES256-GCM-SHA384:ECDHE-RSA-AES256-GCM-SHA384"
  ssl-prefer-server-ciphers: "true"
  ssl-session-cache: "true"
  ssl-session-cache-size: "50m"
  ssl-session-timeout: "10m"
  hsts: "true"
  hsts-max-age: "31536000"
  hsts-include-subdomains: "true"
  hsts-preload: "true"
  server-tokens: "false"
  hide-headers: "X-Powered-By,Server"
  
  # 日志配置
  log-format-escape-json: "true"
  log-format-upstream: '{"time":"$time_iso8601","request_id":"$req_id","remote_addr":"$remote_addr","x_forwarded_for":"$proxy_add_x_forwarded_for","method":"$request_method","host":"$host","uri":"$uri","status":$status,"bytes_sent":$bytes_sent,"request_time":$request_time,"upstream_response_time":"$upstream_response_time","upstream_addr":"$upstream_addr","http_user_agent":"$http_user_agent"}'
  
  # 指标配置
  enable-prometheus-metrics: "true"
  
  # 代理配置
  proxy-body-size: "100m"
  proxy-connect-timeout: "10"
  proxy-read-timeout: "60"
  proxy-send-timeout: "60"
  proxy-buffer-size: "16k"
  
  # 安全配置
  allow-snippet-annotations: "false"
---
# Deployment
apiVersion: apps/v1
kind: Deployment
metadata:
  name: ingress-nginx-controller
  namespace: ingress-nginx
  labels:
    app.kubernetes.io/name: ingress-nginx
    app.kubernetes.io/component: controller
spec:
  replicas: 3
  selector:
    matchLabels:
      app.kubernetes.io/name: ingress-nginx
      app.kubernetes.io/component: controller
  strategy:
    type: RollingUpdate
    rollingUpdate:
      maxSurge: 1
      maxUnavailable: 1
  template:
    metadata:
      labels:
        app.kubernetes.io/name: ingress-nginx
        app.kubernetes.io/component: controller
      annotations:
        prometheus.io/scrape: "true"
        prometheus.io/port: "10254"
    spec:
      serviceAccountName: ingress-nginx
      terminationGracePeriodSeconds: 300
      
      # 安全上下文
      securityContext:
        runAsNonRoot: true
        runAsUser: 101
        runAsGroup: 101
        fsGroup: 101
      
      # 反亲和性 - 分散部署
      affinity:
        podAntiAffinity:
          requiredDuringSchedulingIgnoredDuringExecution:
          - labelSelector:
              matchLabels:
                app.kubernetes.io/component: controller
            topologyKey: kubernetes.io/hostname
          preferredDuringSchedulingIgnoredDuringExecution:
          - weight: 100
            podAffinityTerm:
              labelSelector:
                matchLabels:
                  app.kubernetes.io/component: controller
              topologyKey: topology.kubernetes.io/zone
      
      # 拓扑分布约束
      topologySpreadConstraints:
      - maxSkew: 1
        topologyKey: topology.kubernetes.io/zone
        whenUnsatisfiable: ScheduleAnyway
        labelSelector:
          matchLabels:
            app.kubernetes.io/component: controller
      
      containers:
      - name: controller
        image: registry.k8s.io/ingress-nginx/controller:v1.10.0
        args:
          - /nginx-ingress-controller
          - --publish-service=$(POD_NAMESPACE)/ingress-nginx-controller
          - --election-id=ingress-controller-leader
          - --controller-class=k8s.io/ingress-nginx
          - --ingress-class=nginx
          - --configmap=$(POD_NAMESPACE)/ingress-nginx-controller
          - --validating-webhook=:8443
          - --validating-webhook-certificate=/usr/local/certificates/cert
          - --validating-webhook-key=/usr/local/certificates/key
        
        securityContext:
          allowPrivilegeEscalation: false
          readOnlyRootFilesystem: true
          capabilities:
            drop:
            - ALL
            add:
            - NET_BIND_SERVICE
        
        env:
        - name: POD_NAME
          valueFrom:
            fieldRef:
              fieldPath: metadata.name
        - name: POD_NAMESPACE
          valueFrom:
            fieldRef:
              fieldPath: metadata.namespace
        - name: LD_PRELOAD
          value: /usr/local/lib/libmimalloc.so
        
        ports:
        - name: http
          containerPort: 80
          protocol: TCP
        - name: https
          containerPort: 443
          protocol: TCP
        - name: metrics
          containerPort: 10254
          protocol: TCP
        - name: webhook
          containerPort: 8443
          protocol: TCP
        
        resources:
          requests:
            cpu: 200m
            memory: 256Mi
          limits:
            cpu: 2000m
            memory: 1Gi
        
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
        
        lifecycle:
          preStop:
            exec:
              command:
                - /wait-shutdown
        
        volumeMounts:
        - name: webhook-cert
          mountPath: /usr/local/certificates/
          readOnly: true
        - name: tmp
          mountPath: /tmp
      
      volumes:
      - name: webhook-cert
        secret:
          secretName: ingress-nginx-admission
      - name: tmp
        emptyDir: {}
---
# Service - LoadBalancer
apiVersion: v1
kind: Service
metadata:
  name: ingress-nginx-controller
  namespace: ingress-nginx
  labels:
    app.kubernetes.io/name: ingress-nginx
  annotations:
    # 阿里云 SLB 配置
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-spec: slb.s2.medium
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-address-type: internet
    # 保留源 IP
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-force-override-listeners: "true"
spec:
  type: LoadBalancer
  externalTrafficPolicy: Local
  ports:
  - name: http
    port: 80
    targetPort: http
    protocol: TCP
  - name: https
    port: 443
    targetPort: https
    protocol: TCP
  selector:
    app.kubernetes.io/name: ingress-nginx
    app.kubernetes.io/component: controller
---
# Service - Metrics
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
    targetPort: metrics
  selector:
    app.kubernetes.io/name: ingress-nginx
    app.kubernetes.io/component: controller
---
# HPA
apiVersion: autoscaling/v2
kind: HorizontalPodAutoscaler
metadata:
  name: ingress-nginx-controller
  namespace: ingress-nginx
spec:
  scaleTargetRef:
    apiVersion: apps/v1
    kind: Deployment
    name: ingress-nginx-controller
  minReplicas: 3
  maxReplicas: 10
  metrics:
  - type: Resource
    resource:
      name: cpu
      target:
        type: Utilization
        averageUtilization: 70
  - type: Resource
    resource:
      name: memory
      target:
        type: Utilization
        averageUtilization: 80
  behavior:
    scaleUp:
      stabilizationWindowSeconds: 60
      policies:
      - type: Pods
        value: 2
        periodSeconds: 60
    scaleDown:
      stabilizationWindowSeconds: 300
      policies:
      - type: Percent
        value: 10
        periodSeconds: 60
---
# PDB
apiVersion: policy/v1
kind: PodDisruptionBudget
metadata:
  name: ingress-nginx-controller
  namespace: ingress-nginx
spec:
  minAvailable: 2
  selector:
    matchLabels:
      app.kubernetes.io/name: ingress-nginx
      app.kubernetes.io/component: controller
---
# IngressClass
apiVersion: networking.k8s.io/v1
kind: IngressClass
metadata:
  name: nginx
  annotations:
    ingressclass.kubernetes.io/is-default-class: "true"
spec:
  controller: k8s.io/ingress-nginx
```

---

## 三、多环境配置管理

### 3.1 环境差异化配置

| 配置项 | 开发环境 | 测试环境 | 生产环境 |
|-------|---------|---------|---------|
| **副本数** | 1 | 2 | 3+ |
| **资源 requests** | 100m/128Mi | 200m/256Mi | 500m/512Mi |
| **资源 limits** | 500m/512Mi | 1000m/1Gi | 2000m/2Gi |
| **HPA** | 禁用 | 可选 | 启用 |
| **PDB** | 禁用 | 可选 | 启用 |
| **TLS** | 自签名 | Let's Encrypt Staging | Let's Encrypt Prod |
| **限流** | 宽松 | 中等 | 严格 |
| **日志级别** | debug | info | warn |
| **监控** | 基础 | 完整 | 完整+告警 |

### 3.2 Kustomize 多环境配置

```yaml
# base/kustomization.yaml
apiVersion: kustomize.config.k8s.io/v1beta1
kind: Kustomization
resources:
- deployment.yaml
- service.yaml
- configmap.yaml
- ingress-class.yaml

---
# overlays/dev/kustomization.yaml
apiVersion: kustomize.config.k8s.io/v1beta1
kind: Kustomization
resources:
- ../../base
namespace: ingress-nginx-dev
namePrefix: dev-
patches:
- patch: |-
    - op: replace
      path: /spec/replicas
      value: 1
    - op: replace
      path: /spec/template/spec/containers/0/resources/requests/cpu
      value: "100m"
    - op: replace
      path: /spec/template/spec/containers/0/resources/requests/memory
      value: "128Mi"
  target:
    kind: Deployment
    name: ingress-nginx-controller

---
# overlays/staging/kustomization.yaml
apiVersion: kustomize.config.k8s.io/v1beta1
kind: Kustomization
resources:
- ../../base
namespace: ingress-nginx-staging
namePrefix: staging-
patches:
- patch: |-
    - op: replace
      path: /spec/replicas
      value: 2
  target:
    kind: Deployment
    name: ingress-nginx-controller

---
# overlays/prod/kustomization.yaml
apiVersion: kustomize.config.k8s.io/v1beta1
kind: Kustomization
resources:
- ../../base
- hpa.yaml
- pdb.yaml
- servicemonitor.yaml
namespace: ingress-nginx
patches:
- patch: |-
    - op: replace
      path: /spec/replicas
      value: 3
    - op: replace
      path: /spec/template/spec/containers/0/resources/requests/cpu
      value: "500m"
    - op: replace
      path: /spec/template/spec/containers/0/resources/requests/memory
      value: "512Mi"
    - op: replace
      path: /spec/template/spec/containers/0/resources/limits/cpu
      value: "2000m"
    - op: replace
      path: /spec/template/spec/containers/0/resources/limits/memory
      value: "2Gi"
  target:
    kind: Deployment
    name: ingress-nginx-controller
```

---

## 四、生产 Ingress 配置模板

### 4.1 标准 Web 应用 Ingress

```yaml
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: webapp-ingress
  namespace: production
  labels:
    app: webapp
    environment: production
  annotations:
    # TLS 配置
    nginx.ingress.kubernetes.io/ssl-redirect: "true"
    nginx.ingress.kubernetes.io/force-ssl-redirect: "true"
    
    # 代理配置
    nginx.ingress.kubernetes.io/proxy-body-size: "50m"
    nginx.ingress.kubernetes.io/proxy-connect-timeout: "10"
    nginx.ingress.kubernetes.io/proxy-read-timeout: "60"
    nginx.ingress.kubernetes.io/proxy-send-timeout: "60"
    
    # 重试配置
    nginx.ingress.kubernetes.io/proxy-next-upstream: "error timeout http_502 http_503 http_504"
    nginx.ingress.kubernetes.io/proxy-next-upstream-tries: "3"
    
    # 限流
    nginx.ingress.kubernetes.io/limit-rps: "100"
    nginx.ingress.kubernetes.io/limit-connections: "50"
    
    # 安全响应头
    nginx.ingress.kubernetes.io/configuration-snippet: |
      add_header X-Frame-Options "SAMEORIGIN" always;
      add_header X-Content-Type-Options "nosniff" always;
      add_header X-XSS-Protection "1; mode=block" always;
      add_header Referrer-Policy "strict-origin-when-cross-origin" always;
      add_header X-Request-ID $req_id always;
spec:
  ingressClassName: nginx
  tls:
  - hosts:
    - webapp.example.com
    secretName: webapp-tls
  rules:
  - host: webapp.example.com
    http:
      paths:
      - path: /
        pathType: Prefix
        backend:
          service:
            name: webapp-service
            port:
              number: 80
```

### 4.2 API 服务 Ingress

```yaml
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: api-ingress
  namespace: production
  labels:
    app: api
    environment: production
  annotations:
    # TLS
    nginx.ingress.kubernetes.io/ssl-redirect: "true"
    nginx.ingress.kubernetes.io/backend-protocol: "HTTP"
    
    # 代理配置 - API 可能需要更长超时
    nginx.ingress.kubernetes.io/proxy-body-size: "10m"
    nginx.ingress.kubernetes.io/proxy-connect-timeout: "10"
    nginx.ingress.kubernetes.io/proxy-read-timeout: "120"
    nginx.ingress.kubernetes.io/proxy-send-timeout: "60"
    
    # 严格限流
    nginx.ingress.kubernetes.io/limit-rps: "50"
    nginx.ingress.kubernetes.io/limit-connections: "20"
    
    # CORS
    nginx.ingress.kubernetes.io/enable-cors: "true"
    nginx.ingress.kubernetes.io/cors-allow-origin: "https://webapp.example.com"
    nginx.ingress.kubernetes.io/cors-allow-methods: "GET, POST, PUT, DELETE, OPTIONS"
    nginx.ingress.kubernetes.io/cors-allow-headers: "DNT,Keep-Alive,User-Agent,X-Requested-With,If-Modified-Since,Cache-Control,Content-Type,Authorization"
    nginx.ingress.kubernetes.io/cors-allow-credentials: "true"
    
    # 负载均衡
    nginx.ingress.kubernetes.io/load-balance: "ewma"
spec:
  ingressClassName: nginx
  tls:
  - hosts:
    - api.example.com
    secretName: api-tls
  rules:
  - host: api.example.com
    http:
      paths:
      - path: /v1
        pathType: Prefix
        backend:
          service:
            name: api-v1-service
            port:
              number: 8080
      - path: /v2
        pathType: Prefix
        backend:
          service:
            name: api-v2-service
            port:
              number: 8080
```

### 4.3 WebSocket 服务 Ingress

```yaml
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: websocket-ingress
  namespace: production
  annotations:
    nginx.ingress.kubernetes.io/ssl-redirect: "true"
    
    # WebSocket 配置
    nginx.ingress.kubernetes.io/proxy-read-timeout: "3600"
    nginx.ingress.kubernetes.io/proxy-send-timeout: "3600"
    
    # 启用 WebSocket 支持
    nginx.ingress.kubernetes.io/configuration-snippet: |
      proxy_set_header Upgrade $http_upgrade;
      proxy_set_header Connection "upgrade";
spec:
  ingressClassName: nginx
  tls:
  - hosts:
    - ws.example.com
    secretName: ws-tls
  rules:
  - host: ws.example.com
    http:
      paths:
      - path: /
        pathType: Prefix
        backend:
          service:
            name: websocket-service
            port:
              number: 8080
```

### 4.4 gRPC 服务 Ingress

```yaml
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: grpc-ingress
  namespace: production
  annotations:
    nginx.ingress.kubernetes.io/ssl-redirect: "true"
    
    # gRPC 后端协议
    nginx.ingress.kubernetes.io/backend-protocol: "GRPC"
    
    # gRPC 超时配置
    nginx.ingress.kubernetes.io/proxy-read-timeout: "3600"
    nginx.ingress.kubernetes.io/proxy-send-timeout: "3600"
spec:
  ingressClassName: nginx
  tls:
  - hosts:
    - grpc.example.com
    secretName: grpc-tls
  rules:
  - host: grpc.example.com
    http:
      paths:
      - path: /
        pathType: Prefix
        backend:
          service:
            name: grpc-service
            port:
              number: 50051
```

### 4.5 静态资源 Ingress (带缓存)

```yaml
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: static-ingress
  namespace: production
  annotations:
    nginx.ingress.kubernetes.io/ssl-redirect: "true"
    
    # 缓存配置
    nginx.ingress.kubernetes.io/configuration-snippet: |
      # 静态资源缓存
      location ~* \.(jpg|jpeg|png|gif|ico|css|js|woff|woff2)$ {
        expires 30d;
        add_header Cache-Control "public, no-transform";
        add_header X-Cache-Status $upstream_cache_status;
      }
    
    # 宽松限流
    nginx.ingress.kubernetes.io/limit-rps: "500"
spec:
  ingressClassName: nginx
  tls:
  - hosts:
    - static.example.com
    secretName: static-tls
  rules:
  - host: static.example.com
    http:
      paths:
      - path: /
        pathType: Prefix
        backend:
          service:
            name: static-service
            port:
              number: 80
```

---

## 五、运维操作手册

### 5.1 日常运维操作

| 操作 | 命令 | 说明 |
|-----|------|------|
| 查看状态 | `kubectl get pods -n ingress-nginx` | 检查控制器运行状态 |
| 查看日志 | `kubectl logs -n ingress-nginx -l app.kubernetes.io/component=controller -f` | 实时日志 |
| 查看配置 | `kubectl exec -n ingress-nginx <pod> -- cat /etc/nginx/nginx.conf` | 检查生成的配置 |
| 测试配置 | `kubectl exec -n ingress-nginx <pod> -- nginx -t` | 验证配置语法 |
| 重载配置 | `kubectl rollout restart deployment -n ingress-nginx ingress-nginx-controller` | 重启控制器 |
| 扩缩容 | `kubectl scale deployment -n ingress-nginx ingress-nginx-controller --replicas=5` | 手动扩容 |
| 查看指标 | `kubectl exec -n ingress-nginx <pod> -- curl localhost:10254/metrics` | 检查 Prometheus 指标 |

### 5.2 紧急回滚操作

```bash
# 1. 回滚 Ingress 配置
kubectl rollout undo deployment/app-deployment -n production

# 2. 如果是 Ingress 资源问题，恢复之前的配置
kubectl apply -f ingress-backup.yaml

# 3. 如果是控制器问题，回滚控制器
kubectl rollout undo deployment/ingress-nginx-controller -n ingress-nginx

# 4. 紧急切换流量 (金丝雀回滚)
kubectl patch ingress app-canary -n production -p '{"metadata":{"annotations":{"nginx.ingress.kubernetes.io/canary-weight":"0"}}}'

# 5. 临时禁用 Ingress
kubectl annotate ingress app-ingress -n production nginx.ingress.kubernetes.io/server-snippet='return 503;'
```

### 5.3 证书更新操作

```bash
# 查看证书过期时间
kubectl get secret app-tls -n production -o jsonpath='{.data.tls\.crt}' | base64 -d | openssl x509 -dates -noout

# 使用 cert-manager 手动触发续期
kubectl cert-manager renew app-cert -n production

# 手动更新证书
kubectl create secret tls app-tls \
  --cert=new-tls.crt \
  --key=new-tls.key \
  -n production \
  --dry-run=client -o yaml | kubectl apply -f -
```

---

## 六、故障演练

### 6.1 故障演练场景

| 场景 | 模拟方法 | 预期结果 | 恢复方法 |
|-----|---------|---------|---------|
| **控制器故障** | 删除一个 Pod | 自动重建，流量不中断 | 自动恢复 |
| **后端故障** | 停止后端服务 | 返回 502，其他后端正常 | 重启后端 |
| **配置错误** | 应用错误配置 | 验证 Webhook 拒绝 | 修正配置 |
| **证书过期** | 使用过期证书 | HTTPS 错误 | 更新证书 |
| **资源耗尽** | 压力测试 | HPA 扩容 | 自动恢复 |
| **网络分区** | 隔离可用区 | 其他可用区继续服务 | 恢复网络 |

### 6.2 混沌工程实验

```yaml
# 使用 Chaos Mesh 进行故障注入
apiVersion: chaos-mesh.org/v1alpha1
kind: PodChaos
metadata:
  name: ingress-pod-kill
  namespace: chaos-testing
spec:
  action: pod-kill
  mode: one
  selector:
    namespaces:
      - ingress-nginx
    labelSelectors:
      app.kubernetes.io/component: controller
  scheduler:
    cron: "@every 2h"
---
# 网络延迟注入
apiVersion: chaos-mesh.org/v1alpha1
kind: NetworkChaos
metadata:
  name: ingress-network-delay
  namespace: chaos-testing
spec:
  action: delay
  mode: all
  selector:
    namespaces:
      - ingress-nginx
    labelSelectors:
      app.kubernetes.io/component: controller
  delay:
    latency: "100ms"
    jitter: "50ms"
    correlation: "50"
  duration: "5m"
```

---

## 七、成本优化

### 7.1 资源优化建议

| 优化项 | 方法 | 效果 |
|-------|------|------|
| **合理副本数** | 根据流量调整，启用 HPA | 避免资源浪费 |
| **资源限制** | 设置合理的 requests/limits | 提高资源利用率 |
| **连接复用** | 启用 upstream keepalive | 减少连接开销 |
| **压缩** | 启用 gzip/brotli | 减少带宽成本 |
| **缓存** | 合理配置缓存策略 | 减少后端负载 |
| **日志** | 采样日志或仅记录错误 | 减少存储成本 |

### 7.2 多租户 Ingress 共享

```yaml
# 使用 IngressClass 参数实现多租户隔离
apiVersion: networking.k8s.io/v1
kind: IngressClass
metadata:
  name: nginx-shared
spec:
  controller: k8s.io/ingress-nginx
  parameters:
    apiGroup: k8s.nginx.org
    kind: IngressNginxConfiguration
    name: shared-config
---
# 不同租户使用相同控制器，通过命名空间隔离
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: tenant-a-ingress
  namespace: tenant-a
spec:
  ingressClassName: nginx-shared
  rules:
  - host: tenant-a.example.com
    http:
      paths:
      - path: /
        pathType: Prefix
        backend:
          service:
            name: tenant-a-service
            port:
              number: 80
---
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: tenant-b-ingress
  namespace: tenant-b
spec:
  ingressClassName: nginx-shared
  rules:
  - host: tenant-b.example.com
    http:
      paths:
      - path: /
        pathType: Prefix
        backend:
          service:
            name: tenant-b-service
            port:
              number: 80
```

---

## 八、版本升级指南

### 8.1 升级前检查

| 检查项 | 说明 |
|-------|------|
| 阅读 Release Notes | 了解新版本变更和已知问题 |
| 检查 API 兼容性 | 确认注解和配置兼容性 |
| 备份当前配置 | 导出所有 Ingress 资源 |
| 测试新版本 | 在测试环境验证 |
| 准备回滚方案 | 确保可以快速回滚 |

### 8.2 升级步骤

```bash
# 1. 备份当前配置
kubectl get ingress -A -o yaml > ingress-backup.yaml
kubectl get configmap -n ingress-nginx -o yaml > configmap-backup.yaml

# 2. 更新 Helm Chart (推荐方式)
helm repo update
helm upgrade ingress-nginx ingress-nginx/ingress-nginx \
  --namespace ingress-nginx \
  --set controller.image.tag=v1.10.0 \
  --set controller.updateStrategy.type=RollingUpdate \
  --set controller.updateStrategy.rollingUpdate.maxUnavailable=1

# 3. 验证升级
kubectl rollout status deployment/ingress-nginx-controller -n ingress-nginx

# 4. 检查新版本运行状态
kubectl get pods -n ingress-nginx
kubectl logs -n ingress-nginx -l app.kubernetes.io/component=controller --tail=50

# 5. 验证流量正常
curl -I https://app.example.com
```

### 8.3 回滚步骤

```bash
# 使用 Helm 回滚
helm rollback ingress-nginx 1 -n ingress-nginx

# 或使用 kubectl 回滚
kubectl rollout undo deployment/ingress-nginx-controller -n ingress-nginx

# 恢复配置
kubectl apply -f configmap-backup.yaml
kubectl apply -f ingress-backup.yaml
```

---

## 九、常见问题 FAQ

### 9.1 配置相关

| 问题 | 原因 | 解决方案 |
|-----|------|---------|
| Ingress 配置不生效 | 配置重载失败 | 检查控制器日志 |
| 多个 Ingress 冲突 | 相同 host/path | 使用不同路径或合并 |
| 注解不起作用 | 注解名称错误或不支持 | 查阅官方文档 |
| TLS 不工作 | Secret 格式错误 | 检查证书格式 |

### 9.2 性能相关

| 问题 | 原因 | 解决方案 |
|-----|------|---------|
| 高延迟 | 资源不足/后端慢 | 扩容/优化后端 |
| 502 错误 | 后端不可达 | 检查 Service/Endpoints |
| 连接数过高 | 未启用 keepalive | 配置 upstream-keepalive |
| 内存增长 | 配置过大/泄漏 | 调整配置/升级版本 |

### 9.3 安全相关

| 问题 | 原因 | 解决方案 |
|-----|------|---------|
| 证书错误 | 证书过期/不匹配 | 更新证书 |
| 被 DDoS | 无限流保护 | 配置限流/启用 WAF |
| 信息泄露 | 响应头暴露信息 | 配置 server-tokens=false |
| 配置注入 | 启用了 snippet | 禁用 allow-snippet-annotations |

---

**生产运维原则**: 高可用设计 → 安全优先 → 可观测性 → 持续优化

---

**表格底部标记**: Kusheet Project, 作者 Allen Galler (allengaller@gmail.com)
