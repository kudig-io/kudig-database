# Sidecar 容器模式

## 概述

Sidecar 容器是 Kubernetes 中的核心设计模式，用于扩展和增强主应用容器的功能。Kubernetes v1.28 引入了原生 Sidecar 容器支持，提供更好的生命周期管理。本文档详细介绍 Sidecar 模式的设计原理、实现方式和最佳实践。

## Sidecar 架构

### Sidecar 模式概念图

```
┌─────────────────────────────────────────────────────────────────────────────────────┐
│                              Sidecar 容器模式架构                                    │
│                                                                                      │
│   ┌─────────────────────────────────────────────────────────────────────────────┐   │
│   │                                Pod                                           │   │
│   │                                                                              │   │
│   │   ┌─────────────────────────────────────────────────────────────────────┐   │   │
│   │   │                         共享资源                                     │   │   │
│   │   │                                                                      │   │   │
│   │   │   ┌──────────────┐   ┌──────────────┐   ┌──────────────┐           │   │   │
│   │   │   │  Network     │   │  Storage     │   │  IPC         │           │   │   │
│   │   │   │  Namespace   │   │  Volumes     │   │  Namespace   │           │   │   │
│   │   │   │              │   │              │   │              │           │   │   │
│   │   │   │  共享 IP     │   │  emptyDir    │   │  共享内存    │           │   │   │
│   │   │   │  localhost   │   │  hostPath    │   │  信号量      │           │   │   │
│   │   │   └──────────────┘   └──────────────┘   └──────────────┘           │   │   │
│   │   │                                                                      │   │   │
│   │   └──────────────────────────────────────────────────────────────────────┘   │   │
│   │                                                                              │   │
│   │   ┌────────────────────────────┐   ┌────────────────────────────────────┐   │   │
│   │   │      主容器 (Main)          │   │         Sidecar 容器               │   │   │
│   │   │                            │   │                                    │   │   │
│   │   │   ┌────────────────────┐   │   │   ┌────────────────────────────┐  │   │   │
│   │   │   │   应用程序          │   │   │   │   辅助功能                  │  │   │   │
│   │   │   │                    │   │   │   │                            │  │   │   │
│   │   │   │   • 业务逻辑       │   │   │   │   • 日志收集 (Fluent Bit) │  │   │   │
│   │   │   │   • API 服务       │   │   │   │   • 代理 (Envoy)          │  │   │   │
│   │   │   │   • Web 应用       │   │   │   │   • 监控 (Exporter)       │  │   │   │
│   │   │   │                    │   │   │   │   • 配置刷新               │  │   │   │
│   │   │   │   Port: 8080       │   │   │   │   • Secret 注入           │  │   │   │
│   │   │   └────────────────────┘   │   │   └────────────────────────────┘  │   │   │
│   │   │                            │   │                                    │   │   │
│   │   │   写入日志 ─────────────────┼───┼──► 读取日志并转发                │   │   │
│   │   │                            │   │                                    │   │   │
│   │   │   localhost:8080 ◄─────────┼───┼─── 流量代理 (localhost:15001)    │   │   │
│   │   │                            │   │                                    │   │   │
│   │   └────────────────────────────┘   └────────────────────────────────────┘   │   │
│   │                                                                              │   │
│   └─────────────────────────────────────────────────────────────────────────────┘   │
│                                                                                      │
└──────────────────────────────────────────────────────────────────────────────────────┘
```

### 原生 Sidecar 生命周期 (v1.28+)

```
┌─────────────────────────────────────────────────────────────────────────────────────┐
│                           原生 Sidecar 生命周期 (v1.28+)                             │
│                                                                                      │
│   Pod 启动流程:                                                                      │
│   ┌─────────────────────────────────────────────────────────────────────────────┐   │
│   │                                                                              │   │
│   │   阶段1: Init 容器 (顺序执行)                                               │   │
│   │   ┌────────────┐   ┌────────────┐   ┌────────────┐                         │   │
│   │   │ Init 1     │──►│ Init 2     │──►│ Init 3     │                         │   │
│   │   │ 完成后退出 │   │ 完成后退出 │   │ 完成后退出 │                         │   │
│   │   └────────────┘   └────────────┘   └────────────┘                         │   │
│   │         │                                  │                                │   │
│   │         ▼                                  ▼                                │   │
│   │   阶段2: Sidecar 容器启动 (restartPolicy: Always)                          │   │
│   │   ┌────────────────────────────────────────────────────────────────────┐   │   │
│   │   │  ┌─────────────┐   ┌─────────────┐   ┌─────────────┐               │   │   │
│   │   │  │ Sidecar 1   │   │ Sidecar 2   │   │ Sidecar 3   │               │   │   │
│   │   │  │ (日志收集)  │   │ (代理)      │   │ (监控)      │               │   │   │
│   │   │  │ 持续运行    │   │ 持续运行    │   │ 持续运行    │               │   │   │
│   │   │  └─────────────┘   └─────────────┘   └─────────────┘               │   │   │
│   │   │        ↓ 启动完成 (不阻塞)                                          │   │   │
│   │   └────────────────────────────────────────────────────────────────────┘   │   │
│   │         │                                                                   │   │
│   │         ▼                                                                   │   │
│   │   阶段3: 主容器启动                                                        │   │
│   │   ┌────────────────────────────────────────────────────────────────────┐   │   │
│   │   │  ┌─────────────┐   ┌─────────────┐                                 │   │   │
│   │   │  │ Main 1      │   │ Main 2      │                                 │   │   │
│   │   │  │ (应用)      │   │ (Worker)    │                                 │   │   │
│   │   │  └─────────────┘   └─────────────┘                                 │   │   │
│   │   └────────────────────────────────────────────────────────────────────┘   │   │
│   │                                                                              │   │
│   └─────────────────────────────────────────────────────────────────────────────┘   │
│                                                                                      │
│   Pod 终止流程:                                                                      │
│   ┌─────────────────────────────────────────────────────────────────────────────┐   │
│   │                                                                              │   │
│   │   1. 主容器收到 SIGTERM                                                     │   │
│   │      ┌─────────────┐                                                        │   │
│   │      │ Main 容器   │ ◄── SIGTERM                                           │   │
│   │      │ 优雅退出    │                                                        │   │
│   │      └─────────────┘                                                        │   │
│   │            │                                                                │   │
│   │            ▼ 主容器退出完成                                                 │   │
│   │                                                                              │   │
│   │   2. Sidecar 容器收到 SIGTERM (主容器退出后)                                │   │
│   │      ┌─────────────┐   ┌─────────────┐                                     │   │
│   │      │ Sidecar 1   │   │ Sidecar 2   │ ◄── SIGTERM                         │   │
│   │      │ 优雅退出    │   │ 优雅退出    │                                     │   │
│   │      └─────────────┘   └─────────────┘                                     │   │
│   │                                                                              │   │
│   └─────────────────────────────────────────────────────────────────────────────┘   │
│                                                                                      │
└──────────────────────────────────────────────────────────────────────────────────────┘
```

## Sidecar 类型对比

### 容器类型矩阵

| 类型 | 配置位置 | restartPolicy | 启动顺序 | 退出时机 | 适用场景 |
|-----|---------|---------------|---------|---------|---------|
| **Init 容器** | initContainers | - | 顺序启动,完成后退出 | 启动阶段 | 初始化任务 |
| **原生 Sidecar (v1.28+)** | initContainers | Always | 启动后持续运行,不阻塞 | 主容器退出后 | 辅助服务 |
| **普通容器** | containers | - | 与主容器并行 | 与主容器同时 | 传统方式 |

### 启动顺序对比

| 阶段 | v1.28 之前 | v1.28+ 原生 Sidecar |
|-----|-----------|-------------------|
| **1** | Init 容器顺序启动 | Init 容器顺序启动 |
| **2** | Init 完成后所有容器启动 | 原生 Sidecar 启动 (不等待完成) |
| **3** | - | 主容器启动 |
| **终止** | 所有容器同时收到 SIGTERM | 主容器先退出,Sidecar 后退出 |

## 配置示例

### 原生 Sidecar 配置 (v1.28+)

```yaml
# native-sidecar-example.yaml
apiVersion: v1
kind: Pod
metadata:
  name: app-with-native-sidecars
  namespace: production
  labels:
    app: myapp
spec:
  # ==========================================================================
  # Init 容器和原生 Sidecar 容器
  # ==========================================================================
  initContainers:
    # 普通 Init 容器 - 完成后退出
    - name: init-config
      image: busybox:1.36
      command: ['sh', '-c', 'cp /config/* /app/config/']
      volumeMounts:
        - name: config-volume
          mountPath: /config
        - name: app-config
          mountPath: /app/config

    # 原生 Sidecar 容器 - 持续运行
    - name: log-collector
      image: fluent/fluent-bit:2.2
      restartPolicy: Always  # 关键: 标识为原生 Sidecar
      resources:
        requests:
          cpu: 50m
          memory: 64Mi
        limits:
          cpu: 200m
          memory: 256Mi
      volumeMounts:
        - name: app-logs
          mountPath: /var/log/app
        - name: fluent-bit-config
          mountPath: /fluent-bit/etc
      env:
        - name: NODE_NAME
          valueFrom:
            fieldRef:
              fieldPath: spec.nodeName

    # 原生 Sidecar - Envoy 代理
    - name: envoy-proxy
      image: envoyproxy/envoy:v1.28.0
      restartPolicy: Always
      resources:
        requests:
          cpu: 100m
          memory: 128Mi
        limits:
          cpu: 500m
          memory: 512Mi
      ports:
        - containerPort: 15001
          name: envoy-outbound
        - containerPort: 15006
          name: envoy-inbound
        - containerPort: 15090
          name: envoy-stats
      volumeMounts:
        - name: envoy-config
          mountPath: /etc/envoy
      readinessProbe:
        httpGet:
          path: /ready
          port: 15090
        initialDelaySeconds: 1
        periodSeconds: 5

  # ==========================================================================
  # 主应用容器
  # ==========================================================================
  containers:
    - name: app
      image: myapp:v1.0.0
      ports:
        - containerPort: 8080
          name: http
      resources:
        requests:
          cpu: 500m
          memory: 512Mi
        limits:
          cpu: 2
          memory: 2Gi
      volumeMounts:
        - name: app-logs
          mountPath: /var/log/app
        - name: app-config
          mountPath: /app/config
      env:
        - name: LOG_PATH
          value: /var/log/app/app.log
      livenessProbe:
        httpGet:
          path: /health
          port: 8080
        initialDelaySeconds: 10
        periodSeconds: 10
      readinessProbe:
        httpGet:
          path: /ready
          port: 8080
        initialDelaySeconds: 5
        periodSeconds: 5

  # ==========================================================================
  # 共享卷
  # ==========================================================================
  volumes:
    - name: app-logs
      emptyDir: {}
    - name: app-config
      emptyDir: {}
    - name: config-volume
      configMap:
        name: app-config
    - name: fluent-bit-config
      configMap:
        name: fluent-bit-config
    - name: envoy-config
      configMap:
        name: envoy-config

  # Pod 级别配置
  terminationGracePeriodSeconds: 30
  serviceAccountName: myapp-sa
```

### 传统 Sidecar 配置

```yaml
# traditional-sidecar-example.yaml
apiVersion: v1
kind: Pod
metadata:
  name: app-with-traditional-sidecars
spec:
  containers:
    # 主应用容器
    - name: app
      image: myapp:v1.0.0
      ports:
        - containerPort: 8080
      resources:
        requests:
          cpu: 500m
          memory: 512Mi
      volumeMounts:
        - name: shared-logs
          mountPath: /var/log/app
        - name: shared-data
          mountPath: /data

    # Sidecar: 日志收集
    - name: log-shipper
      image: fluent/fluent-bit:2.2
      resources:
        requests:
          cpu: 50m
          memory: 64Mi
        limits:
          cpu: 200m
          memory: 256Mi
      volumeMounts:
        - name: shared-logs
          mountPath: /var/log/app
          readOnly: true
        - name: fluent-config
          mountPath: /fluent-bit/etc

    # Sidecar: 服务网格代理
    - name: envoy-proxy
      image: envoyproxy/envoy:v1.28.0
      resources:
        requests:
          cpu: 100m
          memory: 128Mi
      ports:
        - containerPort: 15001
        - containerPort: 15006
      volumeMounts:
        - name: envoy-config
          mountPath: /etc/envoy

    # Sidecar: Prometheus 指标导出
    - name: prometheus-exporter
      image: prom/statsd-exporter:v0.26.0
      resources:
        requests:
          cpu: 20m
          memory: 32Mi
      ports:
        - containerPort: 9102
          name: metrics

  volumes:
    - name: shared-logs
      emptyDir: {}
    - name: shared-data
      emptyDir: {}
    - name: fluent-config
      configMap:
        name: fluent-bit-config
    - name: envoy-config
      configMap:
        name: envoy-config
```

## 常见 Sidecar 用例

### 用例矩阵

| 用例 | Sidecar 镜像 | 功能 | 通信方式 | 资源建议 |
|-----|------------|------|---------|---------|
| **日志收集** | Fluent Bit/Filebeat | 收集转发日志 | 共享 Volume | CPU: 50m, Mem: 64Mi |
| **服务网格** | Envoy/Istio Proxy | 流量管理、mTLS | localhost | CPU: 100m, Mem: 128Mi |
| **监控代理** | Prometheus Exporter | 暴露应用指标 | localhost | CPU: 20m, Mem: 32Mi |
| **配置刷新** | ConfigMap Reloader | 配置热更新 | 共享 Volume | CPU: 10m, Mem: 16Mi |
| **Secret 注入** | Vault Agent | 动态 Secret | 共享 Volume | CPU: 50m, Mem: 64Mi |
| **调试工具** | tcpdump/strace | 网络/进程调试 | 共享 Namespace | 按需分配 |
| **数据同步** | rsync/rclone | 数据同步 | 共享 Volume | CPU: 100m, Mem: 128Mi |

### 日志收集 Sidecar 完整配置

```yaml
# fluent-bit-sidecar.yaml
apiVersion: v1
kind: ConfigMap
metadata:
  name: fluent-bit-sidecar-config
  namespace: production
data:
  fluent-bit.conf: |
    [SERVICE]
        Flush         1
        Log_Level     info
        Daemon        off
        Parsers_File  parsers.conf
        HTTP_Server   On
        HTTP_Listen   0.0.0.0
        HTTP_Port     2020
        Health_Check  On

    [INPUT]
        Name              tail
        Path              /var/log/app/*.log
        Tag               app.*
        Read_from_Head    True
        Refresh_Interval  5
        Mem_Buf_Limit     50MB
        Skip_Long_Lines   On
        DB                /var/log/flb_app.db

    [FILTER]
        Name          parser
        Match         app.*
        Key_Name      log
        Parser        json
        Reserve_Data  True

    [FILTER]
        Name          modify
        Match         *
        Add           cluster ${CLUSTER_NAME}
        Add           namespace ${POD_NAMESPACE}
        Add           pod ${POD_NAME}
        Add           node ${NODE_NAME}

    [OUTPUT]
        Name          forward
        Match         *
        Host          fluentd-aggregator.logging
        Port          24224
        Retry_Limit   5

    [OUTPUT]
        Name          stdout
        Match         *
        Format        json_lines

  parsers.conf: |
    [PARSER]
        Name        json
        Format      json
        Time_Key    timestamp
        Time_Format %Y-%m-%dT%H:%M:%S.%L%z

    [PARSER]
        Name        docker
        Format      json
        Time_Key    time
        Time_Format %Y-%m-%dT%H:%M:%S.%L

---
apiVersion: apps/v1
kind: Deployment
metadata:
  name: app-with-logging
  namespace: production
spec:
  replicas: 3
  selector:
    matchLabels:
      app: myapp
  template:
    metadata:
      labels:
        app: myapp
    spec:
      initContainers:
        # 原生 Sidecar: Fluent Bit
        - name: fluent-bit
          image: fluent/fluent-bit:2.2
          restartPolicy: Always
          resources:
            requests:
              cpu: 50m
              memory: 64Mi
            limits:
              cpu: 200m
              memory: 256Mi
          env:
            - name: CLUSTER_NAME
              value: "production"
            - name: POD_NAMESPACE
              valueFrom:
                fieldRef:
                  fieldPath: metadata.namespace
            - name: POD_NAME
              valueFrom:
                fieldRef:
                  fieldPath: metadata.name
            - name: NODE_NAME
              valueFrom:
                fieldRef:
                  fieldPath: spec.nodeName
          volumeMounts:
            - name: app-logs
              mountPath: /var/log/app
            - name: fluent-bit-config
              mountPath: /fluent-bit/etc
            - name: fluent-bit-state
              mountPath: /var/log
          ports:
            - containerPort: 2020
              name: metrics

      containers:
        - name: app
          image: myapp:v1.0.0
          ports:
            - containerPort: 8080
          volumeMounts:
            - name: app-logs
              mountPath: /var/log/app
          env:
            - name: LOG_FILE
              value: /var/log/app/app.log
            - name: LOG_FORMAT
              value: json

      volumes:
        - name: app-logs
          emptyDir: {}
        - name: fluent-bit-config
          configMap:
            name: fluent-bit-sidecar-config
        - name: fluent-bit-state
          emptyDir: {}
```

### Vault Agent Sidecar

```yaml
# vault-agent-sidecar.yaml
apiVersion: v1
kind: Pod
metadata:
  name: app-with-vault
  namespace: production
  annotations:
    # Vault Agent Injector 注解
    vault.hashicorp.com/agent-inject: "true"
    vault.hashicorp.com/agent-inject-status: "update"
    vault.hashicorp.com/role: "myapp-role"
    
    # 注入数据库凭证
    vault.hashicorp.com/agent-inject-secret-db-creds: "database/creds/myapp-db"
    vault.hashicorp.com/agent-inject-template-db-creds: |
      {{- with secret "database/creds/myapp-db" -}}
      export DB_USERNAME="{{ .Data.username }}"
      export DB_PASSWORD="{{ .Data.password }}"
      {{- end }}
    
    # 注入 API 密钥
    vault.hashicorp.com/agent-inject-secret-api-key: "secret/data/myapp/api-key"
    vault.hashicorp.com/agent-inject-template-api-key: |
      {{- with secret "secret/data/myapp/api-key" -}}
      {{ .Data.data.key }}
      {{- end }}
    
    # Agent 配置
    vault.hashicorp.com/agent-pre-populate-only: "false"
    vault.hashicorp.com/agent-cache-enable: "true"
    vault.hashicorp.com/agent-cache-use-auto-auth-token: "force"
spec:
  serviceAccountName: myapp-sa
  containers:
    - name: app
      image: myapp:v1.0.0
      command: ["/bin/sh", "-c"]
      args:
        - source /vault/secrets/db-creds && /app/start.sh
      env:
        - name: VAULT_SECRETS_PATH
          value: /vault/secrets
      volumeMounts:
        - name: vault-secrets
          mountPath: /vault/secrets
          readOnly: true
      resources:
        requests:
          cpu: 500m
          memory: 512Mi

---
# 手动配置 Vault Agent Sidecar
apiVersion: v1
kind: Pod
metadata:
  name: app-with-manual-vault
spec:
  initContainers:
    # Vault Agent 作为原生 Sidecar
    - name: vault-agent
      image: hashicorp/vault:1.15
      restartPolicy: Always
      args:
        - agent
        - -config=/etc/vault/agent-config.hcl
      resources:
        requests:
          cpu: 50m
          memory: 64Mi
        limits:
          cpu: 200m
          memory: 256Mi
      volumeMounts:
        - name: vault-config
          mountPath: /etc/vault
        - name: vault-secrets
          mountPath: /vault/secrets
        - name: vault-token
          mountPath: /home/vault

  containers:
    - name: app
      image: myapp:v1.0.0
      volumeMounts:
        - name: vault-secrets
          mountPath: /vault/secrets
          readOnly: true

  volumes:
    - name: vault-config
      configMap:
        name: vault-agent-config
    - name: vault-secrets
      emptyDir:
        medium: Memory
    - name: vault-token
      emptyDir:
        medium: Memory
```

### Envoy Sidecar (手动注入)

```yaml
# envoy-sidecar-manual.yaml
apiVersion: v1
kind: ConfigMap
metadata:
  name: envoy-sidecar-config
data:
  envoy.yaml: |
    admin:
      address:
        socket_address:
          address: 0.0.0.0
          port_value: 15000
    
    static_resources:
      listeners:
        # 入站监听器
        - name: inbound_listener
          address:
            socket_address:
              address: 0.0.0.0
              port_value: 15006
          filter_chains:
            - filters:
                - name: envoy.filters.network.http_connection_manager
                  typed_config:
                    "@type": type.googleapis.com/envoy.extensions.filters.network.http_connection_manager.v3.HttpConnectionManager
                    stat_prefix: inbound
                    codec_type: AUTO
                    route_config:
                      name: inbound_route
                      virtual_hosts:
                        - name: app
                          domains: ["*"]
                          routes:
                            - match:
                                prefix: "/"
                              route:
                                cluster: local_app
                    http_filters:
                      - name: envoy.filters.http.router
                        typed_config:
                          "@type": type.googleapis.com/envoy.extensions.filters.http.router.v3.Router
        
        # 出站监听器
        - name: outbound_listener
          address:
            socket_address:
              address: 0.0.0.0
              port_value: 15001
          filter_chains:
            - filters:
                - name: envoy.filters.network.http_connection_manager
                  typed_config:
                    "@type": type.googleapis.com/envoy.extensions.filters.network.http_connection_manager.v3.HttpConnectionManager
                    stat_prefix: outbound
                    codec_type: AUTO
                    route_config:
                      name: outbound_route
                      virtual_hosts:
                        - name: external
                          domains: ["*"]
                          routes:
                            - match:
                                prefix: "/"
                              route:
                                cluster: external_service
                    http_filters:
                      - name: envoy.filters.http.router
                        typed_config:
                          "@type": type.googleapis.com/envoy.extensions.filters.http.router.v3.Router
      
      clusters:
        - name: local_app
          connect_timeout: 5s
          type: STATIC
          lb_policy: ROUND_ROBIN
          load_assignment:
            cluster_name: local_app
            endpoints:
              - lb_endpoints:
                  - endpoint:
                      address:
                        socket_address:
                          address: 127.0.0.1
                          port_value: 8080
        
        - name: external_service
          connect_timeout: 5s
          type: STRICT_DNS
          lb_policy: ROUND_ROBIN
          load_assignment:
            cluster_name: external_service
            endpoints:
              - lb_endpoints:
                  - endpoint:
                      address:
                        socket_address:
                          address: external-service.default.svc.cluster.local
                          port_value: 80

---
apiVersion: apps/v1
kind: Deployment
metadata:
  name: app-with-envoy
spec:
  replicas: 2
  selector:
    matchLabels:
      app: myapp
  template:
    metadata:
      labels:
        app: myapp
    spec:
      initContainers:
        - name: envoy
          image: envoyproxy/envoy:v1.28.0
          restartPolicy: Always
          resources:
            requests:
              cpu: 100m
              memory: 128Mi
            limits:
              cpu: 500m
              memory: 512Mi
          ports:
            - containerPort: 15001
              name: outbound
            - containerPort: 15006
              name: inbound
            - containerPort: 15000
              name: admin
            - containerPort: 15090
              name: stats
          volumeMounts:
            - name: envoy-config
              mountPath: /etc/envoy
          readinessProbe:
            httpGet:
              path: /ready
              port: 15000
            initialDelaySeconds: 1
            periodSeconds: 2
          livenessProbe:
            httpGet:
              path: /server_info
              port: 15000
            initialDelaySeconds: 10
            periodSeconds: 10

      containers:
        - name: app
          image: myapp:v1.0.0
          ports:
            - containerPort: 8080
          env:
            - name: HTTP_PROXY
              value: "http://127.0.0.1:15001"

      volumes:
        - name: envoy-config
          configMap:
            name: envoy-sidecar-config
```

## Sidecar 通信模式

### 通信方式对比

| 通信方式 | 实现 | 优点 | 缺点 | 适用场景 |
|---------|-----|------|------|---------|
| **共享 Volume** | emptyDir | 简单、高效 | 需要文件格式协商 | 日志收集、配置共享 |
| **localhost** | 127.0.0.1 | TCP/HTTP 标准 | 需要端口协调 | 代理、API 调用 |
| **Unix Socket** | 共享 Volume 中的 socket | 高性能、安全 | 需要两端支持 | 高性能 IPC |
| **共享内存** | emptyDir (medium: Memory) | 最高性能 | 复杂、需要锁机制 | 极高性能场景 |
| **环境变量** | Downward API | 简单 | 只能传递静态值 | 配置传递 |

### 通信配置示例

```yaml
# sidecar-communication-examples.yaml

---
# 共享 Volume 通信
apiVersion: v1
kind: Pod
metadata:
  name: volume-communication
spec:
  containers:
    - name: app
      image: myapp:v1
      volumeMounts:
        - name: shared-data
          mountPath: /data
      command: ['sh', '-c', 'while true; do echo "$(date)" >> /data/output.txt; sleep 5; done']
    
    - name: processor
      image: processor:v1
      volumeMounts:
        - name: shared-data
          mountPath: /data
          readOnly: true
      command: ['sh', '-c', 'tail -f /data/output.txt']
  
  volumes:
    - name: shared-data
      emptyDir: {}

---
# localhost 通信
apiVersion: v1
kind: Pod
metadata:
  name: localhost-communication
spec:
  containers:
    - name: app
      image: myapp:v1
      ports:
        - containerPort: 8080
      env:
        - name: METRICS_PORT
          value: "9090"
    
    - name: exporter
      image: prom/statsd-exporter:v0.26.0
      ports:
        - containerPort: 9102
      args:
        - --statsd.listen-udp=:9125
        - --web.listen-address=:9102
      # 应用通过 localhost:9125 发送 StatsD 指标

---
# Unix Socket 通信
apiVersion: v1
kind: Pod
metadata:
  name: socket-communication
spec:
  containers:
    - name: app
      image: myapp:v1
      volumeMounts:
        - name: socket-dir
          mountPath: /var/run/app
      env:
        - name: SOCKET_PATH
          value: /var/run/app/app.sock
    
    - name: proxy
      image: nginx:alpine
      volumeMounts:
        - name: socket-dir
          mountPath: /var/run/app
      # nginx 通过 Unix socket 转发请求
  
  volumes:
    - name: socket-dir
      emptyDir: {}
```

## 监控 Sidecar 状态

### 诊断命令

```bash
#!/bin/bash
# sidecar-diagnostics.sh
# Sidecar 容器诊断脚本

POD_NAME=$1
NAMESPACE=${2:-default}

if [ -z "$POD_NAME" ]; then
    echo "用法: $0 <pod-name> [namespace]"
    exit 1
fi

echo "=== Sidecar 容器诊断: $POD_NAME ==="
echo ""

# 获取 Pod 信息
echo "--- Pod 概览 ---"
kubectl get pod $POD_NAME -n $NAMESPACE -o wide
echo ""

# 列出所有容器状态
echo "--- 容器状态 ---"
kubectl get pod $POD_NAME -n $NAMESPACE -o jsonpath='{range .status.containerStatuses[*]}Container: {.name}
  Ready: {.ready}
  State: {.state}
  RestartCount: {.restartCount}
{end}'
echo ""

# 列出 Init 容器状态 (包括原生 Sidecar)
echo "--- Init 容器状态 ---"
kubectl get pod $POD_NAME -n $NAMESPACE -o jsonpath='{range .status.initContainerStatuses[*]}Init Container: {.name}
  Ready: {.ready}
  State: {.state}
  RestartCount: {.restartCount}
{end}'
echo ""

# 检查 Sidecar 资源使用
echo "--- 资源使用 ---"
kubectl top pod $POD_NAME -n $NAMESPACE --containers 2>/dev/null || echo "需要 metrics-server"
echo ""

# 获取 Sidecar 日志
echo "--- Sidecar 日志 (最近 20 行) ---"
for container in $(kubectl get pod $POD_NAME -n $NAMESPACE -o jsonpath='{.spec.initContainers[*].name}'); do
    echo "=== $container ==="
    kubectl logs $POD_NAME -n $NAMESPACE -c $container --tail=20 2>/dev/null || echo "无法获取日志"
    echo ""
done

# 检查就绪探针
echo "--- 探针状态 ---"
kubectl describe pod $POD_NAME -n $NAMESPACE | grep -A 5 "Readiness\|Liveness"
```

### Prometheus 监控规则

```yaml
# sidecar-monitoring-rules.yaml
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: sidecar-monitoring-rules
  namespace: monitoring
spec:
  groups:
    - name: sidecar.alerts
      interval: 30s
      rules:
        # Sidecar 容器重启告警
        - alert: SidecarContainerRestarting
          expr: |
            increase(kube_pod_container_status_restarts_total{container=~".*sidecar.*|envoy|fluent-bit|vault-agent"}[1h]) > 3
          for: 5m
          labels:
            severity: warning
          annotations:
            summary: "Sidecar 容器频繁重启"
            description: "Pod {{ $labels.namespace }}/{{ $labels.pod }} 的 Sidecar {{ $labels.container }} 在 1 小时内重启 {{ $value }} 次"
            
        # Sidecar 未就绪告警
        - alert: SidecarContainerNotReady
          expr: |
            kube_pod_container_status_ready{container=~".*sidecar.*|envoy|fluent-bit|vault-agent"} == 0
          for: 5m
          labels:
            severity: warning
          annotations:
            summary: "Sidecar 容器未就绪"
            description: "Pod {{ $labels.namespace }}/{{ $labels.pod }} 的 Sidecar {{ $labels.container }} 未就绪"
            
        # Envoy Sidecar 健康检查
        - alert: EnvoyProxyUnhealthy
          expr: |
            envoy_server_state{envoy_server_state!="LIVE"} == 1
          for: 2m
          labels:
            severity: critical
          annotations:
            summary: "Envoy 代理不健康"
            description: "Pod {{ $labels.pod }} 的 Envoy 代理状态: {{ $labels.envoy_server_state }}"
            
        # Fluent Bit 积压告警
        - alert: FluentBitBacklogHigh
          expr: |
            fluentbit_input_records_total - fluentbit_output_records_total > 10000
          for: 10m
          labels:
            severity: warning
          annotations:
            summary: "Fluent Bit 日志积压"
            description: "Fluent Bit 日志积压超过 10000 条"
```

## 版本变更记录

| 版本 | 变更内容 | 影响 |
|-----|---------|------|
| **v1.28** | 原生 Sidecar 容器 Alpha | initContainers 支持 restartPolicy: Always |
| **v1.29** | 原生 Sidecar 容器 Beta | 更稳定的生命周期管理 |
| **v1.30** | Sidecar 启动顺序改进 | 更好的依赖管理 |
| **v1.31** | 原生 Sidecar GA 准备 | 预计稳定版本 |

## 最佳实践总结

### Sidecar 设计检查清单

- [ ] 选择合适的 Sidecar 类型 (原生 vs 传统)
- [ ] 配置合理的资源请求和限制
- [ ] 设置适当的健康检查探针
- [ ] 使用共享 Volume 或 localhost 通信
- [ ] 配置日志和监控
- [ ] 考虑优雅终止和启动顺序
- [ ] 避免 Sidecar 成为单点故障

### 资源配置建议

| Sidecar 类型 | CPU Request | Memory Request | CPU Limit | Memory Limit |
|-------------|-------------|----------------|-----------|--------------|
| 日志收集 | 50m | 64Mi | 200m | 256Mi |
| 代理 (Envoy) | 100m | 128Mi | 500m | 512Mi |
| 监控导出 | 20m | 32Mi | 100m | 128Mi |
| Secret 注入 | 50m | 64Mi | 200m | 256Mi |
| 配置刷新 | 10m | 16Mi | 50m | 64Mi |

---

**参考资料**:
- [KEP-753: Sidecar Containers](https://github.com/kubernetes/enhancements/tree/master/keps/sig-node/753-sidecar-containers)
- [Kubernetes Sidecar 模式](https://kubernetes.io/blog/2023/08/25/native-sidecar-containers/)
- [Istio Sidecar 注入](https://istio.io/latest/docs/setup/additional-setup/sidecar-injection/)
