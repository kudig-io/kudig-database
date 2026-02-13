# 35 - 高级 Pod 模式与调度策略 YAML 配置参考

> **适用版本**: Kubernetes v1.25 - v1.32 | **最后更新**: 2026-02

## 目录

- [1. Init Container 模式](#1-init-container-模式)
  - [1.1 数据预加载](#11-数据预加载)
  - [1.2 配置生成](#12-配置生成)
  - [1.3 等待依赖服务](#13-等待依赖服务)
- [2. Sidecar Container 模式](#2-sidecar-container-模式)
  - [2.1 原生 Sidecar (v1.29+)](#21-原生-sidecar-v129)
  - [2.2 日志收集 Sidecar](#22-日志收集-sidecar)
  - [2.3 代理注入模式](#23-代理注入模式)
- [3. 多容器协作模式](#3-多容器协作模式)
  - [3.1 Adapter 模式](#31-adapter-模式)
  - [3.2 Ambassador 模式](#32-ambassador-模式)
  - [3.3 Proxy 模式](#33-proxy-模式)
- [4. Pod Affinity/Anti-Affinity](#4-pod-affinityanti-affinity)
  - [4.1 Required 亲和性](#41-required-亲和性)
  - [4.2 Preferred 亲和性](#42-preferred-亲和性)
  - [4.3 反亲和性模式](#43-反亲和性模式)
- [5. Topology Spread Constraints](#5-topology-spread-constraints)
  - [5.1 基础拓扑分布](#51-基础拓扑分布)
  - [5.2 高级特性 (v1.26+)](#52-高级特性-v126)
- [6. Taints and Tolerations](#6-taints-and-tolerations)
- [7. 探针最佳实践](#7-探针最佳实践)
- [8. 优雅终止与生命周期钩子](#8-优雅终止与生命周期钩子)
- [9. Projected Volumes 组合投射](#9-projected-volumes-组合投射)
- [10. Downward API 完整字段参考](#10-downward-api-完整字段参考)
- [11. Resource QoS 详解](#11-resource-qos-详解)
- [12. 生产案例](#12-生产案例)

---

## 1. Init Container 模式

Init Container 在主容器启动前按顺序执行，常用于准备工作、依赖检查和配置初始化。

### 1.1 数据预加载

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: data-loader-pod
  namespace: production
spec:
  # Init Container 按顺序执行，必须全部成功后才启动主容器
  initContainers:
  - name: download-data
    image: curlimages/curl:8.5.0
    command:
    - sh
    - -c
    - |
      # 下载静态资源到共享卷
      echo "Downloading data assets..."
      curl -fsSL https://cdn.example.com/data.tar.gz -o /data/data.tar.gz
      tar -xzf /data/data.tar.gz -C /data/
      echo "Data download completed at $(date)"
    volumeMounts:
    - name: data-volume
      mountPath: /data
    resources:
      requests:
        cpu: 100m
        memory: 128Mi
      limits:
        cpu: 500m
        memory: 256Mi
  
  - name: validate-data
    image: busybox:1.36
    command:
    - sh
    - -c
    - |
      # 验证数据完整性
      echo "Validating data integrity..."
      if [ ! -f /data/config.json ]; then
        echo "ERROR: config.json not found"
        exit 1
      fi
      echo "Data validation passed"
    volumeMounts:
    - name: data-volume
      mountPath: /data
      readOnly: true
  
  containers:
  - name: app
    image: myapp:v1.2.0
    volumeMounts:
    - name: data-volume
      mountPath: /app/data
      readOnly: true
    resources:
      requests:
        cpu: 500m
        memory: 512Mi
      limits:
        cpu: 1000m
        memory: 1Gi
  
  volumes:
  - name: data-volume
    emptyDir:
      sizeLimit: 2Gi  # 限制 emptyDir 大小 (v1.25+)
```

### 1.2 配置生成

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: config-generator-pod
  namespace: production
spec:
  initContainers:
  - name: generate-config
    image: python:3.11-alpine
    command:
    - python3
    - -c
    - |
      import json
      import os
      
      # 从环境变量生成配置文件
      config = {
          "database": {
              "host": os.getenv("DB_HOST"),
              "port": int(os.getenv("DB_PORT", "5432")),
              "name": os.getenv("DB_NAME")
          },
          "cache": {
              "enabled": os.getenv("CACHE_ENABLED", "true").lower() == "true",
              "ttl": int(os.getenv("CACHE_TTL", "3600"))
          }
      }
      
      with open("/config/app-config.json", "w") as f:
          json.dump(config, f, indent=2)
      
      print("Configuration generated successfully")
    env:
    - name: DB_HOST
      valueFrom:
        configMapKeyRef:
          name: app-config
          key: database.host
    - name: DB_PORT
      valueFrom:
        configMapKeyRef:
          name: app-config
          key: database.port
    - name: DB_NAME
      valueFrom:
        secretKeyRef:
          name: db-credentials
          key: database-name
    - name: CACHE_ENABLED
      value: "true"
    - name: CACHE_TTL
      value: "7200"
    volumeMounts:
    - name: config-volume
      mountPath: /config
  
  containers:
  - name: app
    image: myapp:v1.2.0
    volumeMounts:
    - name: config-volume
      mountPath: /etc/app/config
      readOnly: true
    command:
    - /app/server
    - --config=/etc/app/config/app-config.json
  
  volumes:
  - name: config-volume
    emptyDir: {}
```

### 1.3 等待依赖服务

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: dependent-app-pod
  namespace: production
spec:
  initContainers:
  # 等待数据库就绪
  - name: wait-for-database
    image: busybox:1.36
    command:
    - sh
    - -c
    - |
      echo "Waiting for PostgreSQL at ${DB_HOST}:${DB_PORT}..."
      until nc -z ${DB_HOST} ${DB_PORT}; do
        echo "PostgreSQL is unavailable - sleeping"
        sleep 2
      done
      echo "PostgreSQL is up - continuing"
    env:
    - name: DB_HOST
      value: "postgres.database.svc.cluster.local"
    - name: DB_PORT
      value: "5432"
  
  # 等待 Redis 就绪
  - name: wait-for-redis
    image: redis:7.2-alpine
    command:
    - sh
    - -c
    - |
      echo "Waiting for Redis at ${REDIS_HOST}:${REDIS_PORT}..."
      until redis-cli -h ${REDIS_HOST} -p ${REDIS_PORT} ping | grep -q PONG; do
        echo "Redis is unavailable - sleeping"
        sleep 2
      done
      echo "Redis is up - continuing"
    env:
    - name: REDIS_HOST
      value: "redis.cache.svc.cluster.local"
    - name: REDIS_PORT
      value: "6379"
  
  # 运行数据库迁移
  - name: run-migrations
    image: myapp:v1.2.0
    command:
    - /app/migrate
    - up
    env:
    - name: DATABASE_URL
      valueFrom:
        secretKeyRef:
          name: db-credentials
          key: connection-string
    resources:
      requests:
        cpu: 200m
        memory: 256Mi
      limits:
        cpu: 500m
        memory: 512Mi
  
  containers:
  - name: app
    image: myapp:v1.2.0
    ports:
    - containerPort: 8080
      name: http
    env:
    - name: DATABASE_URL
      valueFrom:
        secretKeyRef:
          name: db-credentials
          key: connection-string
    - name: REDIS_URL
      value: "redis://redis.cache.svc.cluster.local:6379"
```

---

## 2. Sidecar Container 模式

Sidecar 容器与主容器共享生命周期，提供辅助功能如日志收集、监控、代理等。

### 2.1 原生 Sidecar (v1.29+)

Kubernetes v1.29 引入原生 Sidecar 容器支持，通过 `restartPolicy: Always` 标识。

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: native-sidecar-pod
  namespace: production
  labels:
    app: myapp
    version: v1.0.0
spec:
  # Init Container 作为 Sidecar (v1.29+)
  initContainers:
  - name: sidecar-proxy
    image: envoyproxy/envoy:v1.29.0
    # restartPolicy: Always 表示这是一个 Sidecar 容器
    # 它会在主容器启动前启动，并与主容器同时运行
    restartPolicy: Always  # v1.29+ 特性
    ports:
    - containerPort: 15001
      name: proxy-admin
      protocol: TCP
    - containerPort: 15000
      name: proxy-inbound
      protocol: TCP
    volumeMounts:
    - name: envoy-config
      mountPath: /etc/envoy
      readOnly: true
    resources:
      requests:
        cpu: 100m
        memory: 128Mi
      limits:
        cpu: 500m
        memory: 512Mi
    # Sidecar 健康检查
    livenessProbe:
      httpGet:
        path: /ready
        port: 15001
      initialDelaySeconds: 5
      periodSeconds: 10
  
  containers:
  - name: app
    image: myapp:v1.2.0
    ports:
    - containerPort: 8080
      name: http
    env:
    - name: HTTP_PROXY
      value: "http://localhost:15000"
    - name: NO_PROXY
      value: "localhost,127.0.0.1"
    resources:
      requests:
        cpu: 500m
        memory: 512Mi
      limits:
        cpu: 1000m
        memory: 1Gi
  
  volumes:
  - name: envoy-config
    configMap:
      name: envoy-config
```

### 2.2 日志收集 Sidecar

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: log-collector-pod
  namespace: production
spec:
  containers:
  # 主应用容器
  - name: app
    image: myapp:v1.2.0
    ports:
    - containerPort: 8080
    volumeMounts:
    - name: app-logs
      mountPath: /var/log/app
    resources:
      requests:
        cpu: 500m
        memory: 512Mi
      limits:
        cpu: 1000m
        memory: 1Gi
  
  # Fluent Bit 日志收集 Sidecar
  - name: fluent-bit
    image: fluent/fluent-bit:2.2
    volumeMounts:
    - name: app-logs
      mountPath: /var/log/app
      readOnly: true
    - name: fluent-bit-config
      mountPath: /fluent-bit/etc
      readOnly: true
    env:
    - name: FLUENT_ELASTICSEARCH_HOST
      value: "elasticsearch.logging.svc.cluster.local"
    - name: FLUENT_ELASTICSEARCH_PORT
      value: "9200"
    resources:
      requests:
        cpu: 100m
        memory: 128Mi
      limits:
        cpu: 200m
        memory: 256Mi
  
  # Promtail 日志收集 Sidecar (替代方案)
  - name: promtail
    image: grafana/promtail:2.9.3
    args:
    - -config.file=/etc/promtail/promtail.yaml
    volumeMounts:
    - name: app-logs
      mountPath: /var/log/app
      readOnly: true
    - name: promtail-config
      mountPath: /etc/promtail
      readOnly: true
    env:
    - name: LOKI_URL
      value: "http://loki.logging.svc.cluster.local:3100/loki/api/v1/push"
    resources:
      requests:
        cpu: 50m
        memory: 64Mi
      limits:
        cpu: 100m
        memory: 128Mi
  
  volumes:
  - name: app-logs
    emptyDir: {}
  - name: fluent-bit-config
    configMap:
      name: fluent-bit-config
  - name: promtail-config
    configMap:
      name: promtail-config
```

### 2.3 代理注入模式

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: service-mesh-pod
  namespace: production
  labels:
    app: myapp
    version: v2.0.0
  annotations:
    # Istio 自动注入标识 (实际场景中由 Webhook 自动添加)
    sidecar.istio.io/inject: "true"
    sidecar.istio.io/proxyCPU: "100m"
    sidecar.istio.io/proxyMemory: "128Mi"
spec:
  containers:
  # 主应用容器
  - name: app
    image: myapp:v2.0.0
    ports:
    - containerPort: 8080
      name: http
      protocol: TCP
    resources:
      requests:
        cpu: 500m
        memory: 512Mi
      limits:
        cpu: 1000m
        memory: 1Gi
  
  # Istio Envoy Sidecar (由 Istio Webhook 自动注入)
  - name: istio-proxy
    image: istio/proxyv2:1.20.2
    args:
    - proxy
    - sidecar
    - --domain=$(POD_NAMESPACE).svc.cluster.local
    - --proxyLogLevel=warning
    - --proxyComponentLogLevel=misc:error
    ports:
    - containerPort: 15090
      name: http-envoy-prom
      protocol: TCP
    - containerPort: 15021
      name: status-port
      protocol: TCP
    - containerPort: 15001
      name: tcp-inbound
      protocol: TCP
    env:
    - name: POD_NAME
      valueFrom:
        fieldRef:
          fieldPath: metadata.name
    - name: POD_NAMESPACE
      valueFrom:
        fieldRef:
          fieldPath: metadata.namespace
    - name: INSTANCE_IP
      valueFrom:
        fieldRef:
          fieldPath: status.podIP
    - name: SERVICE_ACCOUNT
      valueFrom:
        fieldRef:
          fieldPath: spec.serviceAccountName
    volumeMounts:
    - name: workload-socket
      mountPath: /var/run/secrets/workload-spiffe-uds
    - name: credential-socket
      mountPath: /var/run/secrets/credential-uds
    - name: istio-envoy
      mountPath: /etc/istio/proxy
    - name: istio-data
      mountPath: /var/lib/istio/data
    - name: istio-token
      mountPath: /var/run/secrets/tokens
      readOnly: true
    resources:
      requests:
        cpu: 100m
        memory: 128Mi
      limits:
        cpu: 2000m
        memory: 1Gi
    securityContext:
      allowPrivilegeEscalation: false
      capabilities:
        drop:
        - ALL
      privileged: false
      readOnlyRootFilesystem: true
      runAsGroup: 1337
      runAsNonRoot: true
      runAsUser: 1337
  
  volumes:
  - name: workload-socket
    emptyDir: {}
  - name: credential-socket
    emptyDir: {}
  - name: istio-envoy
    emptyDir: {}
  - name: istio-data
    emptyDir: {}
  - name: istio-token
    projected:
      sources:
      - serviceAccountToken:
          path: istio-token
          expirationSeconds: 43200
          audience: istio-ca
```

---

## 3. 多容器协作模式

### 3.1 Adapter 模式

Adapter 容器将主容器的输出转换为标准格式，便于监控系统采集。

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: adapter-pattern-pod
  namespace: production
spec:
  containers:
  # 主应用容器 (输出自定义格式日志)
  - name: legacy-app
    image: legacy-app:v1.0.0
    ports:
    - containerPort: 8080
    volumeMounts:
    - name: logs
      mountPath: /var/log/app
    resources:
      requests:
        cpu: 500m
        memory: 512Mi
      limits:
        cpu: 1000m
        memory: 1Gi
  
  # Adapter 容器 (转换日志格式为 JSON)
  - name: log-adapter
    image: busybox:1.36
    command:
    - sh
    - -c
    - |
      # 将自定义格式转换为 JSON 格式
      tail -f /var/log/app/app.log | while read line; do
        # 解析日志行: [2024-01-01 12:00:00] INFO message
        timestamp=$(echo "$line" | cut -d']' -f1 | cut -d'[' -f2)
        level=$(echo "$line" | cut -d' ' -f3)
        message=$(echo "$line" | cut -d' ' -f4-)
        
        # 输出为 JSON 格式
        echo "{\"timestamp\":\"$timestamp\",\"level\":\"$level\",\"message\":\"$message\"}"
      done > /var/log/app/app.json
    volumeMounts:
    - name: logs
      mountPath: /var/log/app
    resources:
      requests:
        cpu: 50m
        memory: 64Mi
      limits:
        cpu: 100m
        memory: 128Mi
  
  # Prometheus Exporter Adapter
  - name: metrics-adapter
    image: prom/statsd-exporter:v0.26.0
    ports:
    - containerPort: 9102
      name: metrics
    args:
    - --statsd.listen-udp=:9125
    - --web.listen-address=:9102
    volumeMounts:
    - name: statsd-config
      mountPath: /etc/statsd-exporter
      readOnly: true
    resources:
      requests:
        cpu: 50m
        memory: 64Mi
      limits:
        cpu: 100m
        memory: 128Mi
  
  volumes:
  - name: logs
    emptyDir: {}
  - name: statsd-config
    configMap:
      name: statsd-exporter-config
```

### 3.2 Ambassador 模式

Ambassador 容器作为代理处理外部连接，简化主容器的网络逻辑。

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: ambassador-pattern-pod
  namespace: production
spec:
  containers:
  # 主应用容器 (只关注业务逻辑)
  - name: app
    image: myapp:v1.2.0
    ports:
    - containerPort: 8080
    env:
    # 应用只需连接 localhost，由 Ambassador 处理实际路由
    - name: DATABASE_HOST
      value: "localhost"
    - name: DATABASE_PORT
      value: "5432"
    - name: CACHE_HOST
      value: "localhost"
    - name: CACHE_PORT
      value: "6379"
    resources:
      requests:
        cpu: 500m
        memory: 512Mi
      limits:
        cpu: 1000m
        memory: 1Gi
  
  # Ambassador 容器 (处理数据库连接)
  - name: db-ambassador
    image: haproxy:2.9-alpine
    ports:
    - containerPort: 5432
      name: postgres
    volumeMounts:
    - name: haproxy-config
      mountPath: /usr/local/etc/haproxy
      readOnly: true
    resources:
      requests:
        cpu: 100m
        memory: 128Mi
      limits:
        cpu: 200m
        memory: 256Mi
    # 健康检查
    livenessProbe:
      tcpSocket:
        port: 5432
      initialDelaySeconds: 10
      periodSeconds: 10
  
  # Ambassador 容器 (处理 Redis 连接)
  - name: cache-ambassador
    image: haproxy:2.9-alpine
    ports:
    - containerPort: 6379
      name: redis
    volumeMounts:
    - name: redis-haproxy-config
      mountPath: /usr/local/etc/haproxy
      readOnly: true
    resources:
      requests:
        cpu: 50m
        memory: 64Mi
      limits:
        cpu: 100m
        memory: 128Mi
  
  volumes:
  - name: haproxy-config
    configMap:
      name: db-haproxy-config
  - name: redis-haproxy-config
    configMap:
      name: redis-haproxy-config
---
# HAProxy 配置示例 (ConfigMap)
apiVersion: v1
kind: ConfigMap
metadata:
  name: db-haproxy-config
  namespace: production
data:
  haproxy.cfg: |
    global
        maxconn 256
    
    defaults
        mode tcp
        timeout connect 5000ms
        timeout client 50000ms
        timeout server 50000ms
    
    frontend postgres_frontend
        bind *:5432
        default_backend postgres_backend
    
    backend postgres_backend
        balance roundrobin
        # 主库
        server primary postgres-primary.database.svc.cluster.local:5432 check
        # 只读副本
        server replica1 postgres-replica-1.database.svc.cluster.local:5432 check backup
        server replica2 postgres-replica-2.database.svc.cluster.local:5432 check backup
```

### 3.3 Proxy 模式

Proxy 容器拦截和管理主容器的所有网络流量，提供负载均衡、重试、熔断等功能。

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: proxy-pattern-pod
  namespace: production
spec:
  # Init Container 配置 iptables 规则，劫持流量到 Proxy
  initContainers:
  - name: init-networking
    image: istio/proxyv2:1.20.2
    command:
    - istio-iptables
    - -p
    - "15001"  # Proxy 监听端口
    - -u
    - "1337"   # Proxy 容器用户 ID
    - -g
    - "1337"   # Proxy 容器组 ID
    - -m
    - REDIRECT
    - -i
    - "*"      # 劫持所有出站流量
    - -b
    - "*"      # 劫持所有入站流量
    securityContext:
      capabilities:
        add:
        - NET_ADMIN
        - NET_RAW
      runAsNonRoot: false
      runAsUser: 0
  
  containers:
  # 主应用容器
  - name: app
    image: myapp:v1.2.0
    ports:
    - containerPort: 8080
    resources:
      requests:
        cpu: 500m
        memory: 512Mi
      limits:
        cpu: 1000m
        memory: 1Gi
  
  # Proxy 容器 (使用 Envoy)
  - name: envoy-proxy
    image: envoyproxy/envoy:v1.29.0
    ports:
    - containerPort: 15001
      name: proxy-inbound
    - containerPort: 15006
      name: proxy-outbound
    - containerPort: 15090
      name: metrics
    volumeMounts:
    - name: envoy-config
      mountPath: /etc/envoy
      readOnly: true
    resources:
      requests:
        cpu: 100m
        memory: 128Mi
      limits:
        cpu: 500m
        memory: 512Mi
    securityContext:
      runAsUser: 1337
      runAsGroup: 1337
  
  volumes:
  - name: envoy-config
    configMap:
      name: envoy-proxy-config
```

---

## 4. Pod Affinity/Anti-Affinity

Pod 亲和性/反亲和性控制 Pod 在节点上的分布，实现高可用、性能优化等目标。

### 4.1 Required 亲和性

硬性要求，必须满足才能调度。

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: cache-deployment
  namespace: production
spec:
  replicas: 3
  selector:
    matchLabels:
      app: cache
      tier: backend
  template:
    metadata:
      labels:
        app: cache
        tier: backend
        version: v1.0.0
    spec:
      affinity:
        # Pod 亲和性: 必须调度到运行了 app=web 的节点上
        podAffinity:
          requiredDuringSchedulingIgnoredDuringExecution:
          - labelSelector:
              matchExpressions:
              - key: app
                operator: In
                values:
                - web
              - key: tier
                operator: In
                values:
                - frontend
            # topologyKey 定义拓扑域
            # kubernetes.io/hostname: 同一节点
            # topology.kubernetes.io/zone: 同一可用区
            # topology.kubernetes.io/region: 同一区域
            topologyKey: kubernetes.io/hostname
          
          - labelSelector:
              matchLabels:
                app: database
                tier: backend
            # 必须与数据库 Pod 在同一可用区 (减少延迟)
            topologyKey: topology.kubernetes.io/zone
        
        # Pod 反亲和性: 必须不与其他 cache Pod 在同一节点
        podAntiAffinity:
          requiredDuringSchedulingIgnoredDuringExecution:
          - labelSelector:
              matchLabels:
                app: cache
                tier: backend
            # 确保每个节点只运行一个 cache Pod
            topologyKey: kubernetes.io/hostname
      
      containers:
      - name: redis
        image: redis:7.2-alpine
        ports:
        - containerPort: 6379
          name: redis
        resources:
          requests:
            cpu: 500m
            memory: 1Gi
          limits:
            cpu: 1000m
            memory: 2Gi
```

### 4.2 Preferred 亲和性

软性偏好，尽量满足但不强制。

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: web-deployment
  namespace: production
spec:
  replicas: 6
  selector:
    matchLabels:
      app: web
      tier: frontend
  template:
    metadata:
      labels:
        app: web
        tier: frontend
        version: v2.0.0
    spec:
      affinity:
        # Pod 亲和性偏好
        podAffinity:
          preferredDuringSchedulingIgnoredDuringExecution:
          # 权重 100: 优先与 cache Pod 在同一节点 (提高性能)
          - weight: 100
            podAffinityTerm:
              labelSelector:
                matchLabels:
                  app: cache
                  tier: backend
              topologyKey: kubernetes.io/hostname
          
          # 权重 50: 优先与 API 网关在同一可用区
          - weight: 50
            podAffinityTerm:
              labelSelector:
                matchExpressions:
                - key: component
                  operator: In
                  values:
                  - api-gateway
                  - ingress
              topologyKey: topology.kubernetes.io/zone
        
        # Pod 反亲和性偏好
        podAntiAffinity:
          preferredDuringSchedulingIgnoredDuringExecution:
          # 权重 100: 尽量不与同类 web Pod 在同一节点
          - weight: 100
            podAffinityTerm:
              labelSelector:
                matchLabels:
                  app: web
                  tier: frontend
              topologyKey: kubernetes.io/hostname
          
          # 权重 50: 尽量不与同类 web Pod 在同一可用区
          - weight: 50
            podAffinityTerm:
              labelSelector:
                matchLabels:
                  app: web
                  tier: frontend
              topologyKey: topology.kubernetes.io/zone
        
        # 节点亲和性: 优先选择高性能节点
        nodeAffinity:
          preferredDuringSchedulingIgnoredDuringExecution:
          - weight: 80
            preference:
              matchExpressions:
              - key: node.kubernetes.io/instance-type
                operator: In
                values:
                - c6i.2xlarge
                - c6i.4xlarge
          
          - weight: 60
            preference:
              matchExpressions:
              - key: topology.kubernetes.io/zone
                operator: In
                values:
                - us-west-2a  # 优先可用区
      
      containers:
      - name: nginx
        image: nginx:1.25-alpine
        ports:
        - containerPort: 80
          name: http
        resources:
          requests:
            cpu: 200m
            memory: 256Mi
          limits:
            cpu: 500m
            memory: 512Mi
```

### 4.3 反亲和性模式

```yaml
apiVersion: apps/v1
kind: StatefulSet
metadata:
  name: mongodb-statefulset
  namespace: database
spec:
  serviceName: mongodb
  replicas: 3
  selector:
    matchLabels:
      app: mongodb
      role: primary
  template:
    metadata:
      labels:
        app: mongodb
        role: primary
    spec:
      affinity:
        # 强制反亲和: 每个 MongoDB 副本必须在不同节点
        podAntiAffinity:
          requiredDuringSchedulingIgnoredDuringExecution:
          - labelSelector:
              matchLabels:
                app: mongodb
                role: primary
            topologyKey: kubernetes.io/hostname
          
          # 跨可用区分布 (高可用)
          - labelSelector:
              matchLabels:
                app: mongodb
            topologyKey: topology.kubernetes.io/zone
        
        # 节点亲和性: 必须调度到 SSD 节点
        nodeAffinity:
          requiredDuringSchedulingIgnoredDuringExecution:
            nodeSelectorTerms:
            - matchExpressions:
              - key: node.kubernetes.io/storage-type
                operator: In
                values:
                - ssd
                - nvme
              - key: node.kubernetes.io/instance-type
                operator: In
                values:
                - r6i.xlarge
                - r6i.2xlarge
                - r6i.4xlarge
      
      containers:
      - name: mongodb
        image: mongo:7.0
        ports:
        - containerPort: 27017
          name: mongodb
        volumeMounts:
        - name: data
          mountPath: /data/db
        resources:
          requests:
            cpu: 1000m
            memory: 2Gi
          limits:
            cpu: 2000m
            memory: 4Gi
  
  volumeClaimTemplates:
  - metadata:
      name: data
    spec:
      accessModes:
      - ReadWriteOnce
      storageClassName: fast-ssd
      resources:
        requests:
          storage: 100Gi
```

---

## 5. Topology Spread Constraints

拓扑分布约束提供更精细的 Pod 分布控制，替代复杂的亲和性规则。

### 5.1 基础拓扑分布

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: webapp-deployment
  namespace: production
spec:
  replicas: 12
  selector:
    matchLabels:
      app: webapp
  template:
    metadata:
      labels:
        app: webapp
        tier: frontend
    spec:
      # 拓扑分布约束
      topologySpreadConstraints:
      # 约束 1: 跨可用区均匀分布
      - maxSkew: 1
        # 拓扑域: 可用区
        topologyKey: topology.kubernetes.io/zone
        # 当无法满足约束时: DoNotSchedule (硬约束) / ScheduleAnyway (软约束)
        whenUnsatisfiable: DoNotSchedule
        # 匹配哪些 Pod 参与分布计算
        labelSelector:
          matchLabels:
            app: webapp
            tier: frontend
        # 最小域数量 (v1.30+)
        # 确保至少在 3 个可用区分布
        minDomains: 3
      
      # 约束 2: 跨节点均匀分布
      - maxSkew: 2
        topologyKey: kubernetes.io/hostname
        whenUnsatisfiable: ScheduleAnyway
        labelSelector:
          matchLabels:
            app: webapp
        # 节点亲和性策略 (v1.26+)
        # Honor: 只考虑满足 nodeAffinity 的节点 (默认)
        # Ignore: 忽略 nodeAffinity，所有节点都参与
        nodeAffinityPolicy: Honor
        # 节点污点策略 (v1.26+)
        # Honor: 只考虑 Pod 能容忍污点的节点 (默认)
        # Ignore: 忽略污点，所有节点都参与
        nodeTaintsPolicy: Honor
      
      containers:
      - name: webapp
        image: webapp:v1.0.0
        ports:
        - containerPort: 8080
        resources:
          requests:
            cpu: 500m
            memory: 512Mi
          limits:
            cpu: 1000m
            memory: 1Gi
```

### 5.2 高级特性 (v1.26+)

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: advanced-topology-deployment
  namespace: production
spec:
  replicas: 20
  selector:
    matchLabels:
      app: distributed-app
  template:
    metadata:
      labels:
        app: distributed-app
        version: v2.0.0
        environment: production
    spec:
      topologySpreadConstraints:
      # 多层拓扑分布
      - maxSkew: 1
        topologyKey: topology.kubernetes.io/region
        whenUnsatisfiable: DoNotSchedule
        labelSelector:
          matchLabels:
            app: distributed-app
            environment: production
        minDomains: 2  # 至少跨 2 个区域
      
      - maxSkew: 2
        topologyKey: topology.kubernetes.io/zone
        whenUnsatisfiable: DoNotSchedule
        labelSelector:
          matchLabels:
            app: distributed-app
            environment: production
        minDomains: 6  # 至少跨 6 个可用区 (每个区域 3 个)
        
        # nodeAffinityPolicy: 节点亲和性策略 (v1.26+)
        nodeAffinityPolicy: Honor
        # nodeTaintsPolicy: 节点污点策略 (v1.26+)
        nodeTaintsPolicy: Ignore  # 忽略污点，扩大候选节点范围
      
      - maxSkew: 3
        topologyKey: kubernetes.io/hostname
        whenUnsatisfiable: ScheduleAnyway  # 软约束
        labelSelector:
          matchLabels:
            app: distributed-app
        
        # matchLabelKeys: 动态标签匹配 (v1.27+)
        # 自动添加 Pod 的这些标签到 labelSelector
        matchLabelKeys:
        - version      # 相同版本的 Pod 一起考虑分布
        - environment  # 相同环境的 Pod 一起考虑分布
      
      # 节点亲和性 (配合 nodeAffinityPolicy 使用)
      affinity:
        nodeAffinity:
          requiredDuringSchedulingIgnoredDuringExecution:
            nodeSelectorTerms:
            - matchExpressions:
              - key: node.kubernetes.io/instance-type
                operator: In
                values:
                - m6i.xlarge
                - m6i.2xlarge
                - m6i.4xlarge
              - key: node.kubernetes.io/environment
                operator: In
                values:
                - production
      
      containers:
      - name: app
        image: distributed-app:v2.0.0
        ports:
        - containerPort: 8080
        resources:
          requests:
            cpu: 500m
            memory: 1Gi
          limits:
            cpu: 2000m
            memory: 4Gi
```

**拓扑分布约束关键概念**:

- **maxSkew**: 最大偏差值，越小分布越均匀
  - `maxSkew=1`: 任意两个拓扑域的 Pod 数量差值不超过 1
  - `maxSkew=2`: 任意两个拓扑域的 Pod 数量差值不超过 2

- **topologyKey**: 拓扑域标识
  - `kubernetes.io/hostname`: 节点级别
  - `topology.kubernetes.io/zone`: 可用区级别
  - `topology.kubernetes.io/region`: 区域级别
  - 自定义标签: `rack`, `datacenter` 等

- **whenUnsatisfiable**: 无法满足约束时的行为
  - `DoNotSchedule`: 不调度 (硬约束)
  - `ScheduleAnyway`: 仍然调度 (软约束)

- **minDomains** (v1.30+): 最小拓扑域数量
  - 确保 Pod 至少分布在指定数量的域中

- **nodeAffinityPolicy** (v1.26+): 节点亲和性策略
  - `Honor`: 只考虑满足 nodeAffinity 的节点
  - `Ignore`: 忽略 nodeAffinity

- **nodeTaintsPolicy** (v1.26+): 节点污点策略
  - `Honor`: 只考虑 Pod 能容忍的节点
  - `Ignore`: 忽略污点

- **matchLabelKeys** (v1.27+): 动态标签匹配
  - 自动添加 Pod 的指定标签到 labelSelector
  - 实现版本隔离、环境隔离等

---

## 6. Taints and Tolerations

污点 (Taint) 和容忍 (Toleration) 控制哪些 Pod 可以调度到特定节点。

```yaml
# 为节点添加污点 (通过 kubectl 执行):
# kubectl taint nodes node1 key1=value1:NoSchedule
# kubectl taint nodes node2 dedicated=gpu:NoSchedule
# kubectl taint nodes node3 environment=production:NoExecute

---
apiVersion: v1
kind: Pod
metadata:
  name: tolerations-pod
  namespace: production
spec:
  # 容忍配置
  tolerations:
  # 容忍 1: 精确匹配
  - key: "key1"
    operator: "Equal"
    value: "value1"
    effect: "NoSchedule"
  
  # 容忍 2: 存在性匹配 (不检查 value)
  - key: "dedicated"
    operator: "Exists"
    effect: "NoSchedule"
  
  # 容忍 3: 带超时的 NoExecute
  - key: "environment"
    operator: "Equal"
    value: "production"
    effect: "NoExecute"
    # tolerationSeconds: Pod 可以在节点上停留的时间
    # 节点添加污点后，Pod 将在 3600 秒后被驱逐
    tolerationSeconds: 3600
  
  # 容忍 4: 容忍所有污点 (慎用)
  - operator: "Exists"
  
  # 容忍 5: 容忍特定 key 的所有 effect
  - key: "node.kubernetes.io/not-ready"
    operator: "Exists"
    effect: "NoExecute"
    tolerationSeconds: 300  # 节点 NotReady 后 5 分钟才驱逐
  
  - key: "node.kubernetes.io/unreachable"
    operator: "Exists"
    effect: "NoExecute"
    tolerationSeconds: 300  # 节点 Unreachable 后 5 分钟才驱逐
  
  containers:
  - name: app
    image: myapp:v1.0.0
    resources:
      requests:
        cpu: 500m
        memory: 512Mi
      limits:
        cpu: 1000m
        memory: 1Gi
---
# GPU 节点专用 Pod
apiVersion: v1
kind: Pod
metadata:
  name: gpu-pod
  namespace: ml-workloads
spec:
  tolerations:
  # 容忍 GPU 节点污点
  - key: "nvidia.com/gpu"
    operator: "Exists"
    effect: "NoSchedule"
  
  # 节点选择器 (确保调度到 GPU 节点)
  nodeSelector:
    accelerator: nvidia-tesla-v100
  
  containers:
  - name: cuda-app
    image: nvidia/cuda:12.0.0-runtime-ubuntu22.04
    resources:
      requests:
        cpu: 4000m
        memory: 16Gi
        nvidia.com/gpu: 1  # 请求 1 个 GPU
      limits:
        cpu: 8000m
        memory: 32Gi
        nvidia.com/gpu: 1
---
# 生产环境专用 DaemonSet
apiVersion: apps/v1
kind: DaemonSet
metadata:
  name: monitoring-agent
  namespace: monitoring
spec:
  selector:
    matchLabels:
      app: monitoring-agent
  template:
    metadata:
      labels:
        app: monitoring-agent
    spec:
      tolerations:
      # DaemonSet 通常需要容忍所有节点污点
      - key: "node-role.kubernetes.io/control-plane"
        operator: "Exists"
        effect: "NoSchedule"
      
      - key: "node-role.kubernetes.io/master"
        operator: "Exists"
        effect: "NoSchedule"
      
      - key: "environment"
        operator: "Equal"
        value: "production"
        effect: "NoSchedule"
      
      # 容忍所有 NoExecute 污点 (确保监控覆盖所有节点)
      - operator: "Exists"
        effect: "NoExecute"
      
      containers:
      - name: node-exporter
        image: prom/node-exporter:v1.7.0
        ports:
        - containerPort: 9100
          name: metrics
        resources:
          requests:
            cpu: 100m
            memory: 128Mi
          limits:
            cpu: 200m
            memory: 256Mi
```

**污点效果 (Effect)**:

1. **NoSchedule**: 硬性限制，不容忍的 Pod 不会被调度到该节点
2. **PreferNoSchedule**: 软性限制，尽量不调度不容忍的 Pod，但不强制
3. **NoExecute**: 驱逐现有 Pod，不容忍的 Pod 会被立即驱逐 (或在 tolerationSeconds 后驱逐)

**常见污点场景**:

```yaml
# 1. 节点维护
kubectl taint nodes node1 maintenance=true:NoExecute

# 2. GPU 节点专用
kubectl taint nodes gpu-node nvidia.com/gpu=present:NoSchedule

# 3. 生产环境隔离
kubectl taint nodes prod-node environment=production:NoSchedule

# 4. 控制平面节点
kubectl taint nodes master-node node-role.kubernetes.io/control-plane:NoSchedule

# 5. 临时驱逐 Pod (保留节点)
kubectl taint nodes node1 drain=true:NoExecute
```

---

## 7. 探针最佳实践

Kubernetes 提供三种探针类型: Liveness (存活探针)、Readiness (就绪探针)、Startup (启动探针)。

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: probes-best-practices-pod
  namespace: production
spec:
  containers:
  - name: app
    image: myapp:v1.2.0
    ports:
    - containerPort: 8080
      name: http
    - containerPort: 8081
      name: management
    - containerPort: 9090
      name: grpc
    
    # Startup Probe (v1.18+): 启动探针
    # 用于慢启动应用，成功后才开始 Liveness/Readiness 探针
    startupProbe:
      httpGet:
        path: /healthz/startup
        port: 8081
        scheme: HTTP
        httpHeaders:
        - name: X-Probe-Type
          value: Startup
      # 初始延迟: 首次探测前等待时间
      initialDelaySeconds: 10
      # 探测周期: 每 10 秒探测一次
      periodSeconds: 10
      # 超时时间: 单次探测超时
      timeoutSeconds: 3
      # 成功阈值: 连续成功次数视为成功 (Startup/Liveness 必须为 1)
      successThreshold: 1
      # 失败阈值: 连续失败次数视为失败
      # 30 次 * 10 秒 = 300 秒启动时间
      failureThreshold: 30
    
    # Liveness Probe: 存活探针
    # 失败后重启容器
    livenessProbe:
      httpGet:
        path: /healthz/live
        port: 8081
        scheme: HTTP
      # Startup Probe 成功后立即开始
      initialDelaySeconds: 0
      periodSeconds: 10
      timeoutSeconds: 3
      successThreshold: 1
      failureThreshold: 3  # 连续 3 次失败后重启
    
    # Readiness Probe: 就绪探针
    # 失败后从 Service 移除，成功后重新加入
    readinessProbe:
      httpGet:
        path: /healthz/ready
        port: 8081
        scheme: HTTP
        httpHeaders:
        - name: X-Probe-Type
          value: Readiness
      initialDelaySeconds: 0
      periodSeconds: 5   # 更频繁的就绪检查
      timeoutSeconds: 3
      successThreshold: 1
      failureThreshold: 2  # 连续 2 次失败即移除
    
    resources:
      requests:
        cpu: 500m
        memory: 512Mi
      limits:
        cpu: 1000m
        memory: 1Gi
---
# TCP Socket 探针
apiVersion: v1
kind: Pod
metadata:
  name: tcp-probe-pod
  namespace: production
spec:
  containers:
  - name: redis
    image: redis:7.2-alpine
    ports:
    - containerPort: 6379
      name: redis
    
    startupProbe:
      tcpSocket:
        port: 6379
      initialDelaySeconds: 5
      periodSeconds: 5
      timeoutSeconds: 2
      failureThreshold: 10
    
    livenessProbe:
      tcpSocket:
        port: 6379
      initialDelaySeconds: 0
      periodSeconds: 10
      timeoutSeconds: 2
      failureThreshold: 3
    
    readinessProbe:
      # 使用 redis-cli 检查
      exec:
        command:
        - sh
        - -c
        - redis-cli ping | grep -q PONG
      initialDelaySeconds: 0
      periodSeconds: 5
      timeoutSeconds: 2
      failureThreshold: 2
---
# Exec 探针
apiVersion: v1
kind: Pod
metadata:
  name: exec-probe-pod
  namespace: production
spec:
  containers:
  - name: database
    image: postgres:16-alpine
    ports:
    - containerPort: 5432
    
    env:
    - name: POSTGRES_DB
      value: appdb
    - name: POSTGRES_USER
      valueFrom:
        secretKeyRef:
          name: db-credentials
          key: username
    - name: POSTGRES_PASSWORD
      valueFrom:
        secretKeyRef:
          name: db-credentials
          key: password
    
    startupProbe:
      exec:
        command:
        - /bin/sh
        - -c
        - pg_isready -U $POSTGRES_USER -d $POSTGRES_DB
      initialDelaySeconds: 10
      periodSeconds: 5
      timeoutSeconds: 3
      failureThreshold: 20  # 100 秒启动时间
    
    livenessProbe:
      exec:
        command:
        - /bin/sh
        - -c
        - pg_isready -U $POSTGRES_USER -d $POSTGRES_DB
      initialDelaySeconds: 0
      periodSeconds: 10
      timeoutSeconds: 3
      failureThreshold: 3
    
    readinessProbe:
      exec:
        command:
        - /bin/sh
        - -c
        - |
          # 检查数据库是否接受连接
          psql -U $POSTGRES_USER -d $POSTGRES_DB -c "SELECT 1" > /dev/null 2>&1
      initialDelaySeconds: 0
      periodSeconds: 5
      timeoutSeconds: 3
      failureThreshold: 2
---
# gRPC 探针 (v1.24+)
apiVersion: v1
kind: Pod
metadata:
  name: grpc-probe-pod
  namespace: production
spec:
  containers:
  - name: grpc-server
    image: grpc-server:v1.0.0
    ports:
    - containerPort: 9090
      name: grpc
    
    startupProbe:
      grpc:
        port: 9090
        # service 字段可选，实现 gRPC Health Checking Protocol
        service: my.service.v1.HealthCheck
      initialDelaySeconds: 5
      periodSeconds: 5
      timeoutSeconds: 3
      failureThreshold: 10
    
    livenessProbe:
      grpc:
        port: 9090
      initialDelaySeconds: 0
      periodSeconds: 10
      timeoutSeconds: 3
      failureThreshold: 3
    
    readinessProbe:
      grpc:
        port: 9090
      initialDelaySeconds: 0
      periodSeconds: 5
      timeoutSeconds: 3
      failureThreshold: 2
```

**探针配置最佳实践**:

1. **Startup Probe**:
   - 用于慢启动应用 (JVM、大型数据加载等)
   - `failureThreshold * periodSeconds` 应覆盖最长启动时间
   - 成功后禁用，不再探测

2. **Liveness Probe**:
   - 检查进程是否健康 (死锁、无响应等)
   - 失败后重启容器
   - `failureThreshold` 不宜过小，避免误杀
   - 探测路径应轻量级，避免消耗资源

3. **Readiness Probe**:
   - 检查应用是否准备好处理请求
   - 失败后从 Service 移除，成功后重新加入
   - `periodSeconds` 应较短，快速响应流量变化
   - 可包含依赖检查 (数据库、缓存连接等)

4. **时序配置**:
   ```yaml
   # 启动阶段
   startupProbe:
     initialDelaySeconds: 10
     periodSeconds: 10
     failureThreshold: 30  # 最长 310 秒启动时间
   
   # 运行阶段
   livenessProbe:
     initialDelaySeconds: 0  # Startup 成功后立即开始
     periodSeconds: 10
     failureThreshold: 3     # 30 秒无响应才重启
   
   readinessProbe:
     initialDelaySeconds: 0
     periodSeconds: 5        # 更频繁检查就绪状态
     failureThreshold: 2     # 10 秒未就绪即移除
   ```

---

## 8. 优雅终止与生命周期钩子

优雅终止确保 Pod 在删除前完成清理工作，避免数据丢失和请求中断。

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: graceful-shutdown-pod
  namespace: production
spec:
  # 终止宽限期: Pod 从接收 SIGTERM 到被强制杀死 (SIGKILL) 的时间
  # 默认 30 秒，根据应用需要调整
  terminationGracePeriodSeconds: 60
  
  containers:
  - name: app
    image: myapp:v1.2.0
    ports:
    - containerPort: 8080
      name: http
    
    # 生命周期钩子
    lifecycle:
      # PostStart: 容器启动后立即执行 (与 ENTRYPOINT 并发)
      postStart:
        exec:
          command:
          - /bin/sh
          - -c
          - |
            echo "Container started at $(date)" >> /var/log/lifecycle.log
            # 注册到服务发现
            curl -X POST http://service-registry:8080/register \
              -d "pod_name=${HOSTNAME}" \
              -d "pod_ip=${POD_IP}"
        # 也可以使用 HTTP 调用
        # httpGet:
        #   path: /lifecycle/post-start
        #   port: 8081
        #   scheme: HTTP
      
      # PreStop: 容器终止前执行 (在发送 SIGTERM 之前)
      # 用于优雅关闭、注销服务、完成请求等
      preStop:
        exec:
          command:
          - /bin/sh
          - -c
          - |
            echo "PreStop hook triggered at $(date)" >> /var/log/lifecycle.log
            
            # 1. 从服务注册中心注销
            echo "Deregistering from service registry..."
            curl -X DELETE http://service-registry:8080/deregister/${HOSTNAME}
            
            # 2. 等待现有连接完成 (配合 Readiness Probe 失败)
            echo "Waiting for existing connections to drain..."
            sleep 20
            
            # 3. 通知应用开始优雅关闭
            echo "Triggering graceful shutdown..."
            curl -X POST http://localhost:8081/shutdown
            
            # 4. 等待应用完成清理
            sleep 10
            
            echo "PreStop hook completed at $(date)" >> /var/log/lifecycle.log
    
    # 就绪探针: 确保 PreStop 期间流量停止
    readinessProbe:
      httpGet:
        path: /healthz/ready
        port: 8081
      initialDelaySeconds: 5
      periodSeconds: 5
      failureThreshold: 1  # 快速标记为 NotReady
    
    resources:
      requests:
        cpu: 500m
        memory: 512Mi
      limits:
        cpu: 1000m
        memory: 1Gi
    
    env:
    - name: POD_IP
      valueFrom:
        fieldRef:
          fieldPath: status.podIP
    - name: HOSTNAME
      valueFrom:
        fieldRef:
          fieldPath: metadata.name
---
# 生产级优雅终止示例
apiVersion: apps/v1
kind: Deployment
metadata:
  name: web-app-deployment
  namespace: production
spec:
  replicas: 3
  # 滚动更新策略
  strategy:
    type: RollingUpdate
    rollingUpdate:
      maxUnavailable: 1
      maxSurge: 1
  
  selector:
    matchLabels:
      app: web-app
  
  template:
    metadata:
      labels:
        app: web-app
        version: v2.0.0
    spec:
      terminationGracePeriodSeconds: 90  # 延长终止时间
      
      containers:
      - name: nginx
        image: nginx:1.25-alpine
        ports:
        - containerPort: 80
          name: http
        
        lifecycle:
          preStop:
            exec:
              command:
              - /bin/sh
              - -c
              - |
                # 1. 停止接受新连接 (修改就绪探针会失败)
                echo "Stopping new connections..."
                rm -f /tmp/healthy
                
                # 2. 等待 Endpoint 更新传播 (kube-proxy 更新 iptables)
                echo "Waiting for endpoint removal..."
                sleep 15
                
                # 3. 等待现有连接完成
                echo "Draining existing connections..."
                nginx_pid=$(cat /var/run/nginx.pid)
                
                # 发送 SIGQUIT 给 Nginx (优雅关闭)
                kill -QUIT $nginx_pid
                
                # 等待 Nginx 完成关闭 (最多 60 秒)
                timeout=60
                while kill -0 $nginx_pid 2>/dev/null && [ $timeout -gt 0 ]; do
                  sleep 1
                  timeout=$((timeout - 1))
                done
                
                echo "Graceful shutdown completed"
        
        readinessProbe:
          exec:
            command:
            - /bin/sh
            - -c
            - test -f /tmp/healthy
          initialDelaySeconds: 5
          periodSeconds: 2
          failureThreshold: 1
        
        livenessProbe:
          httpGet:
            path: /
            port: 80
          initialDelaySeconds: 10
          periodSeconds: 10
        
        # 创建健康检查文件
        postStart:
          exec:
            command:
            - /bin/sh
            - -c
            - touch /tmp/healthy
```

**优雅终止流程**:

1. Pod 标记为 Terminating
2. PreStop Hook 执行
3. 同时发送 SIGTERM 给容器主进程
4. 等待 `terminationGracePeriodSeconds` 秒
5. 如果仍未退出，发送 SIGKILL 强制杀死

**最佳实践**:

```yaml
# 推荐配置
spec:
  terminationGracePeriodSeconds: 60  # 根据应用调整
  
  containers:
  - lifecycle:
      preStop:
        exec:
          command:
          - /bin/sh
          - -c
          - |
            # Step 1: 标记为 NotReady (5-10 秒)
            rm -f /tmp/ready
            sleep 10
            
            # Step 2: 等待流量排空 (10-20 秒)
            sleep 15
            
            # Step 3: 应用清理逻辑 (10-30 秒)
            /app/cleanup.sh
            
            # 总时间: 35-60 秒 (留余量给 SIGTERM)
```

---

## 9. Projected Volumes 组合投射

Projected Volume 将多个卷源投射到同一目录，支持 Secret、ConfigMap、DownwardAPI、ServiceAccountToken。

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: projected-volume-pod
  namespace: production
spec:
  serviceAccountName: app-service-account
  
  containers:
  - name: app
    image: myapp:v1.2.0
    volumeMounts:
    - name: all-in-one
      mountPath: /var/run/configs
      readOnly: true
    
    env:
    - name: CONFIG_DIR
      value: /var/run/configs
  
  volumes:
  - name: all-in-one
    projected:
      # 默认权限: 0644
      defaultMode: 0440
      sources:
      # 1. Secret 投射
      - secret:
          name: db-credentials
          items:
          - key: username
            path: secrets/db-username
            mode: 0400  # 只读，仅所有者
          - key: password
            path: secrets/db-password
            mode: 0400
          # optional: true 表示 Secret 不存在时不报错
          optional: false
      
      # 2. ConfigMap 投射
      - configMap:
          name: app-config
          items:
          - key: app.yaml
            path: config/app.yaml
          - key: logging.yaml
            path: config/logging.yaml
          optional: false
      
      # 3. Downward API 投射 (Pod 元数据)
      - downwardAPI:
          items:
          # Pod 字段
          - path: metadata/pod-name
            fieldRef:
              fieldPath: metadata.name
          
          - path: metadata/pod-namespace
            fieldRef:
              fieldPath: metadata.namespace
          
          - path: metadata/pod-ip
            fieldRef:
              fieldPath: status.podIP
          
          - path: metadata/labels
            fieldRef:
              fieldPath: metadata.labels
          
          - path: metadata/annotations
            fieldRef:
              fieldPath: metadata.annotations
          
          # 容器资源字段
          - path: resources/cpu-request
            resourceFieldRef:
              containerName: app
              resource: requests.cpu
              divisor: 1m  # 转换为 millicores
          
          - path: resources/memory-limit
            resourceFieldRef:
              containerName: app
              resource: limits.memory
              divisor: 1Mi  # 转换为 MiB
      
      # 4. ServiceAccountToken 投射 (v1.20+)
      - serviceAccountToken:
          path: tokens/vault-token
          # 令牌受众 (aud 字段)
          audience: vault.example.com
          # 令牌过期时间 (秒)
          expirationSeconds: 3600
      
      - serviceAccountToken:
          path: tokens/api-token
          audience: api.example.com
          expirationSeconds: 7200
---
# 完整示例: 应用配置管理
apiVersion: v1
kind: Pod
metadata:
  name: config-management-pod
  namespace: production
  labels:
    app: myapp
    tier: backend
    version: v1.0.0
  annotations:
    prometheus.io/scrape: "true"
    prometheus.io/port: "9090"
spec:
  serviceAccountName: myapp-sa
  
  containers:
  - name: app
    image: myapp:v1.2.0
    ports:
    - containerPort: 8080
      name: http
    
    volumeMounts:
    - name: config-bundle
      mountPath: /etc/app
      readOnly: true
    
    command:
    - /bin/sh
    - -c
    - |
      echo "=== Configuration Files ==="
      ls -la /etc/app/
      
      echo -e "\n=== Secrets ==="
      echo "DB Username: $(cat /etc/app/secrets/db-username)"
      
      echo -e "\n=== Config ==="
      cat /etc/app/config/app.yaml
      
      echo -e "\n=== Pod Metadata ==="
      echo "Pod Name: $(cat /etc/app/metadata/pod-name)"
      echo "Pod Namespace: $(cat /etc/app/metadata/pod-namespace)"
      echo "Pod IP: $(cat /etc/app/metadata/pod-ip)"
      
      echo -e "\n=== Resource Limits ==="
      echo "CPU Request: $(cat /etc/app/resources/cpu-request) millicores"
      echo "Memory Limit: $(cat /etc/app/resources/memory-limit) MiB"
      
      echo -e "\n=== Service Account Tokens ==="
      ls -la /etc/app/tokens/
      
      # 启动应用
      exec /app/server --config=/etc/app/config/app.yaml
    
    resources:
      requests:
        cpu: 500m
        memory: 512Mi
      limits:
        cpu: 1000m
        memory: 1Gi
  
  volumes:
  - name: config-bundle
    projected:
      defaultMode: 0440
      sources:
      - secret:
          name: app-secrets
          items:
          - key: db-username
            path: secrets/db-username
          - key: db-password
            path: secrets/db-password
          - key: api-key
            path: secrets/api-key
      
      - configMap:
          name: app-config
          items:
          - key: app.yaml
            path: config/app.yaml
          - key: database.yaml
            path: config/database.yaml
      
      - downwardAPI:
          items:
          - path: metadata/pod-name
            fieldRef:
              fieldPath: metadata.name
          - path: metadata/pod-namespace
            fieldRef:
              fieldPath: metadata.namespace
          - path: metadata/pod-ip
            fieldRef:
              fieldPath: status.podIP
          - path: metadata/labels
            fieldRef:
              fieldPath: metadata.labels
          - path: metadata/annotations
            fieldRef:
              fieldPath: metadata.annotations
          - path: resources/cpu-request
            resourceFieldRef:
              containerName: app
              resource: requests.cpu
              divisor: 1m
          - path: resources/memory-limit
            resourceFieldRef:
              containerName: app
              resource: limits.memory
              divisor: 1Mi
      
      - serviceAccountToken:
          path: tokens/vault-token
          audience: https://vault.example.com
          expirationSeconds: 3600
```

---

## 10. Downward API 完整字段参考

Downward API 将 Pod/Container 元数据暴露给容器，支持环境变量和文件两种方式。

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: downward-api-pod
  namespace: production
  labels:
    app: myapp
    tier: backend
    version: v1.0.0
    environment: production
  annotations:
    build.version: "v1.0.0-20240101"
    git.commit: "a1b2c3d4"
    team: "platform-engineering"
spec:
  containers:
  - name: app
    image: myapp:v1.2.0
    
    # 方式 1: 环境变量
    env:
    # Pod 字段
    - name: POD_NAME
      valueFrom:
        fieldRef:
          fieldPath: metadata.name
    
    - name: POD_NAMESPACE
      valueFrom:
        fieldRef:
          fieldPath: metadata.namespace
    
    - name: POD_UID
      valueFrom:
        fieldRef:
          fieldPath: metadata.uid
    
    - name: POD_IP
      valueFrom:
        fieldRef:
          fieldPath: status.podIP
    
    - name: POD_IPS  # v1.20+ 支持多 IP
      valueFrom:
        fieldRef:
          fieldPath: status.podIPs
    
    - name: POD_SERVICE_ACCOUNT
      valueFrom:
        fieldRef:
          fieldPath: spec.serviceAccountName
    
    - name: NODE_NAME
      valueFrom:
        fieldRef:
          fieldPath: spec.nodeName
    
    - name: NODE_IP
      valueFrom:
        fieldRef:
          fieldPath: status.hostIP
    
    # 标签 (所有标签作为 JSON 字符串)
    - name: POD_LABELS
      valueFrom:
        fieldRef:
          fieldPath: metadata.labels
    
    # 单个标签
    - name: APP_LABEL
      valueFrom:
        fieldRef:
          fieldPath: metadata.labels['app']
    
    - name: VERSION_LABEL
      valueFrom:
        fieldRef:
          fieldPath: metadata.labels['version']
    
    # 注解 (所有注解作为 JSON 字符串)
    - name: POD_ANNOTATIONS
      valueFrom:
        fieldRef:
          fieldPath: metadata.annotations
    
    # 单个注解
    - name: BUILD_VERSION
      valueFrom:
        fieldRef:
          fieldPath: metadata.annotations['build.version']
    
    # 容器资源字段
    - name: CPU_REQUEST
      valueFrom:
        resourceFieldRef:
          containerName: app
          resource: requests.cpu
          divisor: 1m  # 结果单位: millicores
    
    - name: CPU_LIMIT
      valueFrom:
        resourceFieldRef:
          containerName: app
          resource: limits.cpu
          divisor: 1  # 结果单位: cores
    
    - name: MEMORY_REQUEST
      valueFrom:
        resourceFieldRef:
          containerName: app
          resource: requests.memory
          divisor: 1Mi  # 结果单位: MiB
    
    - name: MEMORY_LIMIT
      valueFrom:
        resourceFieldRef:
          containerName: app
          resource: limits.memory
          divisor: 1Gi  # 结果单位: GiB
    
    - name: EPHEMERAL_STORAGE_REQUEST
      valueFrom:
        resourceFieldRef:
          containerName: app
          resource: requests.ephemeral-storage
          divisor: 1Gi
    
    # 方式 2: 文件 (Volume)
    volumeMounts:
    - name: podinfo
      mountPath: /etc/podinfo
      readOnly: true
    
    resources:
      requests:
        cpu: 500m
        memory: 512Mi
        ephemeral-storage: 1Gi
      limits:
        cpu: 1000m
        memory: 1Gi
        ephemeral-storage: 2Gi
  
  volumes:
  - name: podinfo
    downwardAPI:
      defaultMode: 0644
      items:
      # Pod 元数据
      - path: name
        fieldRef:
          fieldPath: metadata.name
      
      - path: namespace
        fieldRef:
          fieldPath: metadata.namespace
      
      - path: uid
        fieldRef:
          fieldPath: metadata.uid
      
      - path: labels
        fieldRef:
          fieldPath: metadata.labels
      
      - path: annotations
        fieldRef:
          fieldPath: metadata.annotations
      
      # Pod 状态
      - path: ip
        fieldRef:
          fieldPath: status.podIP
      
      - path: ips  # v1.20+
        fieldRef:
          fieldPath: status.podIPs
      
      - path: host-ip
        fieldRef:
          fieldPath: status.hostIP
      
      # 节点信息
      - path: node-name
        fieldRef:
          fieldPath: spec.nodeName
      
      - path: service-account
        fieldRef:
          fieldPath: spec.serviceAccountName
      
      # 容器资源
      - path: cpu-request
        resourceFieldRef:
          containerName: app
          resource: requests.cpu
          divisor: 1m
      
      - path: cpu-limit
        resourceFieldRef:
          containerName: app
          resource: limits.cpu
          divisor: 1
      
      - path: memory-request
        resourceFieldRef:
          containerName: app
          resource: requests.memory
          divisor: 1Mi
      
      - path: memory-limit
        resourceFieldRef:
          containerName: app
          resource: limits.memory
          divisor: 1Mi
```

**Downward API 支持的字段总结**:

| 字段类型 | 环境变量 | 文件卷 | 字段路径 |
|---------|---------|--------|---------|
| Pod 名称 | ✅ | ✅ | `metadata.name` |
| Pod 命名空间 | ✅ | ✅ | `metadata.namespace` |
| Pod UID | ✅ | ✅ | `metadata.uid` |
| Pod IP | ✅ | ✅ | `status.podIP` |
| Pod IPs (v1.20+) | ✅ | ✅ | `status.podIPs` |
| 节点名称 | ✅ | ✅ | `spec.nodeName` |
| 节点 IP | ✅ | ✅ | `status.hostIP` |
| ServiceAccount | ✅ | ✅ | `spec.serviceAccountName` |
| 所有标签 | ✅ | ✅ | `metadata.labels` |
| 单个标签 | ✅ | ❌ | `metadata.labels['key']` |
| 所有注解 | ✅ | ✅ | `metadata.annotations` |
| 单个注解 | ✅ | ❌ | `metadata.annotations['key']` |
| CPU Request | ✅ | ✅ | `requests.cpu` |
| CPU Limit | ✅ | ✅ | `limits.cpu` |
| Memory Request | ✅ | ✅ | `requests.memory` |
| Memory Limit | ✅ | ✅ | `limits.memory` |
| Ephemeral Storage | ✅ | ✅ | `requests/limits.ephemeral-storage` |

**资源单位转换 (divisor)**:

```yaml
# CPU
divisor: 1    # 结果: 0.5 (表示 500m)
divisor: 1m   # 结果: 500

# Memory
divisor: 1    # 结果: 536870912 (bytes)
divisor: 1Ki  # 结果: 524288 (KiB)
divisor: 1Mi  # 结果: 512 (MiB)
divisor: 1Gi  # 结果: 0.5 (GiB)
```

---

## 11. Resource QoS 详解

Kubernetes 根据 Pod 的资源请求和限制自动分配 QoS 类别，影响调度优先级和驱逐顺序。

```yaml
# QoS Class 1: Guaranteed (最高优先级)
# 条件: 所有容器都设置了 requests 和 limits，且 requests == limits
apiVersion: v1
kind: Pod
metadata:
  name: guaranteed-pod
  namespace: production
spec:
  containers:
  - name: app
    image: myapp:v1.2.0
    resources:
      requests:
        cpu: 1000m      # 1 core
        memory: 1Gi     # 1 GiB
      limits:
        cpu: 1000m      # 必须等于 requests
        memory: 1Gi     # 必须等于 requests
  
  - name: sidecar
    image: sidecar:v1.0.0
    resources:
      requests:
        cpu: 100m
        memory: 128Mi
      limits:
        cpu: 100m       # 必须等于 requests
        memory: 128Mi   # 必须等于 requests
---
# QoS Class 2: Burstable (中等优先级)
# 条件: 至少一个容器设置了 requests 或 limits，但不满足 Guaranteed 条件
apiVersion: v1
kind: Pod
metadata:
  name: burstable-pod
  namespace: production
spec:
  containers:
  # 场景 1: 设置 requests，limits 更高
  - name: app
    image: myapp:v1.2.0
    resources:
      requests:
        cpu: 500m
        memory: 512Mi
      limits:
        cpu: 2000m      # 可以突发到 2 cores
        memory: 2Gi     # 可以使用到 2 GiB
  
  # 场景 2: 只设置 requests
  - name: worker
    image: worker:v1.0.0
    resources:
      requests:
        cpu: 200m
        memory: 256Mi
      # 没有 limits，可以使用节点剩余资源
  
  # 场景 3: 只设置 limits
  - name: batch
    image: batch:v1.0.0
    resources:
      # 没有 requests，调度器无法预留资源
      limits:
        cpu: 1000m
        memory: 1Gi
---
# QoS Class 3: BestEffort (最低优先级)
# 条件: 所有容器都没有设置 requests 和 limits
apiVersion: v1
kind: Pod
metadata:
  name: besteffort-pod
  namespace: development
spec:
  containers:
  - name: app
    image: myapp:v1.2.0
    # 没有 resources 字段
    # 可以使用节点上的任何可用资源
    # 资源不足时最先被驱逐
---
# 生产环境推荐配置
apiVersion: apps/v1
kind: Deployment
metadata:
  name: production-app
  namespace: production
spec:
  replicas: 3
  selector:
    matchLabels:
      app: production-app
  template:
    metadata:
      labels:
        app: production-app
        qos: guaranteed  # 标识 QoS 意图
    spec:
      # 优先级类 (影响调度和驱逐)
      priorityClassName: high-priority
      
      containers:
      - name: app
        image: myapp:v1.2.0
        
        # 策略 1: Guaranteed QoS (关键应用)
        resources:
          requests:
            cpu: 1000m
            memory: 2Gi
            ephemeral-storage: 5Gi
          limits:
            cpu: 1000m          # 相等
            memory: 2Gi         # 相等
            ephemeral-storage: 5Gi  # 相等
        
        # 策略 2: Burstable QoS (可突发应用)
        # resources:
        #   requests:
        #     cpu: 500m        # 基准
        #     memory: 1Gi      # 基准
        #   limits:
        #     cpu: 2000m       # 可突发 4x
        #     memory: 4Gi      # 可突发 4x
        
        # 资源配额建议
        # 1. requests: 应用正常运行所需
        # 2. limits: 应用峰值负载所需
        # 3. limits / requests 比例:
        #    - CPU: 2-4x (可突发)
        #    - Memory: 1-2x (内存突发风险大)
---
# PriorityClass 定义
apiVersion: scheduling.k8s.io/v1
kind: PriorityClass
metadata:
  name: high-priority
# 优先级值: 越高越优先 (0-10亿)
# 系统保留: > 10亿
value: 1000000
# 全局默认优先级
globalDefault: false
# 描述
description: "High priority for production critical services"
# preemptionPolicy: 抢占策略 (v1.19+)
# Never: 不抢占低优先级 Pod
# PreemptLowerPriority: 抢占低优先级 Pod (默认)
preemptionPolicy: PreemptLowerPriority
---
apiVersion: scheduling.k8s.io/v1
kind: PriorityClass
metadata:
  name: medium-priority
value: 100000
globalDefault: false
description: "Medium priority for standard applications"
---
apiVersion: scheduling.k8s.io/v1
kind: PriorityClass
metadata:
  name: low-priority
value: 10000
globalDefault: true  # 默认优先级
description: "Low priority for batch jobs and non-critical workloads"
preemptionPolicy: Never  # 不抢占其他 Pod
```

**QoS 类别对比**:

| QoS 类别 | 资源配置 | 调度优先级 | 驱逐顺序 | 适用场景 |
|---------|---------|-----------|---------|---------|
| **Guaranteed** | `requests == limits` | 最高 | 最后驱逐 | 关键生产应用、数据库 |
| **Burstable** | `requests < limits` 或部分设置 | 中等 | 中间驱逐 | 常规应用、可突发负载 |
| **BestEffort** | 无 `requests/limits` | 最低 | 最先驱逐 | 开发测试、批处理任务 |

**驱逐优先级** (资源不足时):
1. BestEffort Pod (使用量最高的先驱逐)
2. Burstable Pod 超出 requests 部分 (按超出比例驱逐)
3. Burstable Pod 在 requests 内
4. Guaranteed Pod (最后才驱逐)

**资源配置最佳实践**:

```yaml
# 1. Web 应用 (可突发)
resources:
  requests:
    cpu: 500m       # 正常负载
    memory: 512Mi   # 正常内存
  limits:
    cpu: 2000m      # 突发 4x
    memory: 1Gi     # 突发 2x (内存不建议超过 2x)

# 2. 数据库 (稳定资源)
resources:
  requests:
    cpu: 2000m
    memory: 4Gi
  limits:
    cpu: 2000m      # 不突发，保证性能
    memory: 4Gi     # 不突发，避免 OOM

# 3. 批处理 (低优先级)
resources:
  requests:
    cpu: 100m       # 最小请求
    memory: 256Mi
  limits:
    cpu: 4000m      # 可大量突发
    memory: 8Gi

# 4. 开发环境 (BestEffort)
# 不设置 resources，节省集群配额
```

---

## 12. 生产案例

### 案例 1: 微服务 Sidecar 模式

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: microservice-with-sidecar
  namespace: production
  labels:
    app: order-service
    version: v2.0.0
spec:
  replicas: 3
  selector:
    matchLabels:
      app: order-service
  
  template:
    metadata:
      labels:
        app: order-service
        version: v2.0.0
      annotations:
        prometheus.io/scrape: "true"
        prometheus.io/port: "9090"
    
    spec:
      # 服务账号
      serviceAccountName: order-service-sa
      
      # 优雅终止
      terminationGracePeriodSeconds: 60
      
      # Init Container: 等待依赖服务
      initContainers:
      - name: wait-for-dependencies
        image: busybox:1.36
        command:
        - sh
        - -c
        - |
          echo "Waiting for PostgreSQL..."
          until nc -z postgres.database.svc.cluster.local 5432; do
            sleep 2
          done
          
          echo "Waiting for Redis..."
          until nc -z redis.cache.svc.cluster.local 6379; do
            sleep 2
          done
          
          echo "All dependencies ready"
      
      containers:
      # 主应用容器
      - name: order-service
        image: order-service:v2.0.0
        ports:
        - containerPort: 8080
          name: http
          protocol: TCP
        - containerPort: 9090
          name: metrics
          protocol: TCP
        
        env:
        - name: DATABASE_URL
          valueFrom:
            secretKeyRef:
              name: order-service-secrets
              key: database-url
        - name: REDIS_URL
          value: "redis://redis.cache.svc.cluster.local:6379"
        - name: POD_NAME
          valueFrom:
            fieldRef:
              fieldPath: metadata.name
        - name: POD_NAMESPACE
          valueFrom:
            fieldRef:
              fieldPath: metadata.namespace
        
        volumeMounts:
        - name: config
          mountPath: /etc/order-service
          readOnly: true
        - name: logs
          mountPath: /var/log/app
        
        resources:
          requests:
            cpu: 500m
            memory: 1Gi
          limits:
            cpu: 2000m
            memory: 2Gi
        
        # 启动探针
        startupProbe:
          httpGet:
            path: /healthz/startup
            port: 8080
          initialDelaySeconds: 10
          periodSeconds: 5
          failureThreshold: 30
        
        # 存活探针
        livenessProbe:
          httpGet:
            path: /healthz/live
            port: 8080
          initialDelaySeconds: 0
          periodSeconds: 10
          failureThreshold: 3
        
        # 就绪探针
        readinessProbe:
          httpGet:
            path: /healthz/ready
            port: 8080
          initialDelaySeconds: 0
          periodSeconds: 5
          failureThreshold: 2
        
        # 生命周期钩子
        lifecycle:
          preStop:
            exec:
              command:
              - sh
              - -c
              - |
                echo "Starting graceful shutdown..."
                sleep 15  # 等待流量排空
                curl -X POST http://localhost:8080/shutdown
                sleep 10
      
      # Sidecar 1: Envoy 代理
      - name: envoy-proxy
        image: envoyproxy/envoy:v1.29.0
        ports:
        - containerPort: 15001
          name: proxy
        - containerPort: 15000
          name: admin
        
        volumeMounts:
        - name: envoy-config
          mountPath: /etc/envoy
          readOnly: true
        
        resources:
          requests:
            cpu: 100m
            memory: 128Mi
          limits:
            cpu: 500m
            memory: 512Mi
        
        livenessProbe:
          httpGet:
            path: /ready
            port: 15000
          initialDelaySeconds: 5
          periodSeconds: 10
      
      # Sidecar 2: Fluent Bit 日志收集
      - name: fluent-bit
        image: fluent/fluent-bit:2.2
        volumeMounts:
        - name: logs
          mountPath: /var/log/app
          readOnly: true
        - name: fluent-bit-config
          mountPath: /fluent-bit/etc
          readOnly: true
        
        env:
        - name: FLUENT_ELASTICSEARCH_HOST
          value: "elasticsearch.logging.svc.cluster.local"
        - name: FLUENT_ELASTICSEARCH_PORT
          value: "9200"
        
        resources:
          requests:
            cpu: 50m
            memory: 64Mi
          limits:
            cpu: 100m
            memory: 128Mi
      
      # Sidecar 3: Prometheus Exporter
      - name: prometheus-exporter
        image: prom/statsd-exporter:v0.26.0
        ports:
        - containerPort: 9102
          name: metrics
        
        resources:
          requests:
            cpu: 50m
            memory: 64Mi
          limits:
            cpu: 100m
            memory: 128Mi
      
      volumes:
      - name: config
        configMap:
          name: order-service-config
      - name: logs
        emptyDir: {}
      - name: envoy-config
        configMap:
          name: envoy-config
      - name: fluent-bit-config
        configMap:
          name: fluent-bit-config
      
      # 拓扑分布
      topologySpreadConstraints:
      - maxSkew: 1
        topologyKey: topology.kubernetes.io/zone
        whenUnsatisfiable: DoNotSchedule
        labelSelector:
          matchLabels:
            app: order-service
      
      - maxSkew: 2
        topologyKey: kubernetes.io/hostname
        whenUnsatisfiable: ScheduleAnyway
        labelSelector:
          matchLabels:
            app: order-service
      
      # 节点亲和性
      affinity:
        nodeAffinity:
          preferredDuringSchedulingIgnoredDuringExecution:
          - weight: 100
            preference:
              matchExpressions:
              - key: node.kubernetes.io/instance-type
                operator: In
                values:
                - c6i.2xlarge
                - c6i.4xlarge
```

### 案例 2: 跨可用区高可用部署

```yaml
apiVersion: apps/v1
kind: StatefulSet
metadata:
  name: ha-database
  namespace: database
spec:
  serviceName: ha-database
  replicas: 5
  
  # Pod 管理策略
  podManagementPolicy: Parallel  # 并行创建 Pod
  
  selector:
    matchLabels:
      app: ha-database
  
  template:
    metadata:
      labels:
        app: ha-database
        tier: database
    
    spec:
      # 优先级
      priorityClassName: high-priority
      
      # 服务账号
      serviceAccountName: ha-database-sa
      
      # 反亲和性: 跨节点和可用区分布
      affinity:
        podAntiAffinity:
          requiredDuringSchedulingIgnoredDuringExecution:
          # 必须在不同节点
          - labelSelector:
              matchLabels:
                app: ha-database
            topologyKey: kubernetes.io/hostname
          
          # 必须在不同可用区
          - labelSelector:
              matchLabels:
                app: ha-database
            topologyKey: topology.kubernetes.io/zone
        
        # 节点亲和性: 使用 SSD 节点
        nodeAffinity:
          requiredDuringSchedulingIgnoredDuringExecution:
            nodeSelectorTerms:
            - matchExpressions:
              - key: node.kubernetes.io/storage-type
                operator: In
                values:
                - ssd
                - nvme
              - key: topology.kubernetes.io/zone
                operator: In
                values:
                - us-west-2a
                - us-west-2b
                - us-west-2c
      
      # 拓扑分布: 确保均匀分布
      topologySpreadConstraints:
      - maxSkew: 1
        topologyKey: topology.kubernetes.io/zone
        whenUnsatisfiable: DoNotSchedule
        labelSelector:
          matchLabels:
            app: ha-database
        minDomains: 3  # 至少 3 个可用区
      
      - maxSkew: 1
        topologyKey: kubernetes.io/hostname
        whenUnsatisfiable: DoNotSchedule
        labelSelector:
          matchLabels:
            app: ha-database
      
      initContainers:
      # 初始化存储权限
      - name: init-permissions
        image: busybox:1.36
        command:
        - sh
        - -c
        - |
          chown -R 999:999 /var/lib/postgresql/data
          chmod 0700 /var/lib/postgresql/data
        volumeMounts:
        - name: data
          mountPath: /var/lib/postgresql/data
        securityContext:
          runAsUser: 0
      
      containers:
      - name: postgres
        image: postgres:16-alpine
        ports:
        - containerPort: 5432
          name: postgres
        
        env:
        - name: POSTGRES_DB
          value: appdb
        - name: POSTGRES_USER
          valueFrom:
            secretKeyRef:
              name: postgres-credentials
              key: username
        - name: POSTGRES_PASSWORD
          valueFrom:
            secretKeyRef:
              name: postgres-credentials
              key: password
        - name: PGDATA
          value: /var/lib/postgresql/data/pgdata
        - name: POD_NAME
          valueFrom:
            fieldRef:
              fieldPath: metadata.name
        
        volumeMounts:
        - name: data
          mountPath: /var/lib/postgresql/data
        - name: config
          mountPath: /etc/postgresql
          readOnly: true
        
        # Guaranteed QoS
        resources:
          requests:
            cpu: 2000m
            memory: 4Gi
          limits:
            cpu: 2000m
            memory: 4Gi
        
        startupProbe:
          exec:
            command:
            - /bin/sh
            - -c
            - pg_isready -U $POSTGRES_USER -d $POSTGRES_DB
          initialDelaySeconds: 10
          periodSeconds: 5
          failureThreshold: 30
        
        livenessProbe:
          exec:
            command:
            - /bin/sh
            - -c
            - pg_isready -U $POSTGRES_USER -d $POSTGRES_DB
          initialDelaySeconds: 0
          periodSeconds: 10
          failureThreshold: 3
        
        readinessProbe:
          exec:
            command:
            - /bin/sh
            - -c
            - psql -U $POSTGRES_USER -d $POSTGRES_DB -c "SELECT 1"
          initialDelaySeconds: 0
          periodSeconds: 5
          failureThreshold: 2
        
        lifecycle:
          preStop:
            exec:
              command:
              - /bin/sh
              - -c
              - |
                # PostgreSQL 优雅关闭
                pg_ctl stop -m smart -t 60
      
      # Sidecar: Postgres Exporter
      - name: postgres-exporter
        image: prometheuscommunity/postgres-exporter:v0.15.0
        ports:
        - containerPort: 9187
          name: metrics
        
        env:
        - name: DATA_SOURCE_NAME
          valueFrom:
            secretKeyRef:
              name: postgres-credentials
              key: exporter-dsn
        
        resources:
          requests:
            cpu: 100m
            memory: 128Mi
          limits:
            cpu: 200m
            memory: 256Mi
      
      volumes:
      - name: config
        configMap:
          name: postgres-config
  
  # 持久化存储
  volumeClaimTemplates:
  - metadata:
      name: data
    spec:
      accessModes:
      - ReadWriteOnce
      storageClassName: fast-ssd
      resources:
        requests:
          storage: 100Gi
```

### 案例 3: GPU 节点调度

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: gpu-training-pod
  namespace: ml-workloads
spec:
  # 优先级
  priorityClassName: high-priority
  
  # 容忍 GPU 节点污点
  tolerations:
  - key: "nvidia.com/gpu"
    operator: "Exists"
    effect: "NoSchedule"
  
  # GPU 节点选择器
  nodeSelector:
    accelerator: nvidia-tesla-v100
    node.kubernetes.io/instance-type: p3.2xlarge
  
  # 节点亲和性
  affinity:
    nodeAffinity:
      requiredDuringSchedulingIgnoredDuringExecution:
        nodeSelectorTerms:
        - matchExpressions:
          - key: nvidia.com/gpu.present
            operator: In
            values:
            - "true"
          - key: topology.kubernetes.io/zone
            operator: In
            values:
            - us-west-2a  # 指定可用区
    
    # Pod 反亲和性: 避免 GPU 竞争
    podAntiAffinity:
      preferredDuringSchedulingIgnoredDuringExecution:
      - weight: 100
        podAffinityTerm:
          labelSelector:
            matchExpressions:
            - key: workload-type
              operator: In
              values:
              - gpu-training
          topologyKey: kubernetes.io/hostname
  
  initContainers:
  # 下载训练数据
  - name: download-dataset
    image: amazon/aws-cli:2.15.0
    command:
    - sh
    - -c
    - |
      echo "Downloading training dataset..."
      aws s3 sync s3://my-bucket/datasets/ /data/datasets/
      echo "Dataset download completed"
    volumeMounts:
    - name: data
      mountPath: /data
    env:
    - name: AWS_REGION
      value: us-west-2
    - name: AWS_ACCESS_KEY_ID
      valueFrom:
        secretKeyRef:
          name: aws-credentials
          key: access-key-id
    - name: AWS_SECRET_ACCESS_KEY
      valueFrom:
        secretKeyRef:
          name: aws-credentials
          key: secret-access-key
    resources:
      requests:
        cpu: 1000m
        memory: 2Gi
      limits:
        cpu: 2000m
        memory: 4Gi
  
  containers:
  # GPU 训练任务
  - name: pytorch-training
    image: pytorch/pytorch:2.1.0-cuda12.1-cudnn8-runtime
    command:
    - python3
    - /app/train.py
    - --data-dir=/data/datasets
    - --output-dir=/data/models
    - --epochs=100
    - --batch-size=64
    
    volumeMounts:
    - name: data
      mountPath: /data
    - name: training-code
      mountPath: /app
      readOnly: true
    - name: shm
      mountPath: /dev/shm  # 共享内存，提高 DataLoader 性能
    
    env:
    - name: CUDA_VISIBLE_DEVICES
      value: "0"
    - name: NVIDIA_VISIBLE_DEVICES
      value: "all"
    - name: NVIDIA_DRIVER_CAPABILITIES
      value: "compute,utility"
    
    # GPU 资源请求
    resources:
      requests:
        cpu: 8000m
        memory: 32Gi
        nvidia.com/gpu: 1  # 请求 1 个 GPU
      limits:
        cpu: 16000m
        memory: 64Gi
        nvidia.com/gpu: 1
    
    # 健康检查
    livenessProbe:
      exec:
        command:
        - sh
        - -c
        - nvidia-smi && pgrep -f train.py
      initialDelaySeconds: 60
      periodSeconds: 30
      failureThreshold: 3
  
  # Sidecar: TensorBoard
  - name: tensorboard
    image: tensorflow/tensorflow:2.15.0
    command:
    - tensorboard
    - --logdir=/data/logs
    - --host=0.0.0.0
    - --port=6006
    
    ports:
    - containerPort: 6006
      name: tensorboard
    
    volumeMounts:
    - name: data
      mountPath: /data
      readOnly: true
    
    resources:
      requests:
        cpu: 500m
        memory: 1Gi
      limits:
        cpu: 1000m
        memory: 2Gi
  
  volumes:
  - name: data
    persistentVolumeClaim:
      claimName: ml-data-pvc
  - name: training-code
    configMap:
      name: training-code
  - name: shm
    emptyDir:
      medium: Memory  # 使用内存作为共享内存
      sizeLimit: 16Gi
  
  # 调度失败时不自动重启
  restartPolicy: OnFailure
---
# GPU 节点污点配置 (kubectl 命令)
# kubectl taint nodes gpu-node-1 nvidia.com/gpu=present:NoSchedule
# kubectl label nodes gpu-node-1 accelerator=nvidia-tesla-v100
# kubectl label nodes gpu-node-1 nvidia.com/gpu.present=true
```

---

## 总结

本文档涵盖了 Kubernetes 高级 Pod 模式与调度策略的全面配置：

1. **Init Container 模式**: 数据预加载、配置生成、依赖等待
2. **Sidecar Container 模式**: v1.29+ 原生支持、日志收集、代理注入
3. **多容器协作**: Adapter、Ambassador、Proxy 模式
4. **Pod Affinity/Anti-Affinity**: Required/Preferred 亲和性配置
5. **Topology Spread Constraints**: 精细的拓扑分布控制 (v1.26-v1.30+ 特性)
6. **Taints and Tolerations**: 污点与容忍的完整用法
7. **探针最佳实践**: Startup/Liveness/Readiness 探针配置
8. **优雅终止**: PreStop Hook 与生命周期管理
9. **Projected Volumes**: 多源卷组合投射
10. **Downward API**: 完整字段引用表
11. **Resource QoS**: Guaranteed/Burstable/BestEffort 详解
12. **生产案例**: 微服务 Sidecar、跨 AZ 高可用、GPU 调度

所有 YAML 配置均包含详细的中文注释，适用于 Kubernetes v1.25-v1.32 版本。

