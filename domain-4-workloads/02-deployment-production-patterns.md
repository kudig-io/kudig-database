# 02 - Deployment 生产模式与最佳实践 (Deployment Production Patterns)

> **适用版本**: v1.25 - v1.32 | **最后更新**: 2026-02 | **参考**: [Kubernetes Deployment](https://kubernetes.io/docs/concepts/workloads/controllers/deployment/)

## Deployment 核心架构解析

### 1. Deployment 控制链路

```mermaid
graph LR
    A[Deployment] --> B[ReplicaSet]
    B --> C[Pod]
    A --> D[Controller Manager]
    D --> E[Deployment Controller]
    E --> F[Reconciliation Loop]
    
    subgraph "更新过程"
    G[创建新RS] --> H[缩放新RS]
    H --> I[缩放旧RS]
    I --> J[清理旧RS]
    end
```

### 2. 生产级 Deployment 模板库

#### 2.1 标准 Web 应用模板

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: web-application
  namespace: production
  labels:
    app: web-application
    version: v1.2.3
    tier: frontend
    env: production
  annotations:
    kubernetes.io/change-cause: "Release v1.2.3 - 性能优化和安全补丁"
    deployment.kubernetes.io/revision: "15"
spec:
  # 基础配置
  replicas: 6
  revisionHistoryLimit: 20  # 保留更多历史版本用于回滚
  progressDeadlineSeconds: 600  # 10分钟部署超时
  
  # 更新策略
  strategy:
    type: RollingUpdate
    rollingUpdate:
      maxSurge: 2           # 允许超出2个副本(约33%)
      maxUnavailable: 1     # 最多1个不可用
  
  # 选择器
  selector:
    matchLabels:
      app: web-application
  
  # Pod 模板
  template:
    metadata:
      labels:
        app: web-application
        version: v1.2.3
        tier: frontend
        env: production
      annotations:
        prometheus.io/scrape: "true"
        prometheus.io/port: "9090"
        prometheus.io/path: "/metrics"
        checksum/config: "sha256:abcdef123456..."
    
    spec:
      # 服务账户
      serviceAccountName: web-app-sa
      
      # 优先级类
      priorityClassName: high-priority
      
      # 节点选择
      nodeSelector:
        node-type: web-server
        kubernetes.io/arch: amd64
      
      # 污点容忍
      tolerations:
      - key: dedicated
        operator: Equal
        value: web-tier
        effect: NoSchedule
      - key: node.kubernetes.io/not-ready
        operator: Exists
        effect: NoExecute
        tolerationSeconds: 300
      
      # 拓扑分布约束
      topologySpreadConstraints:
      - maxSkew: 1
        topologyKey: topology.kubernetes.io/zone
        whenUnsatisfiable: DoNotSchedule
        labelSelector:
          matchLabels:
            app: web-application
      - maxSkew: 1
        topologyKey: kubernetes.io/hostname
        whenUnsatisfiable: ScheduleAnyway
        labelSelector:
          matchLabels:
            app: web-application
      
      # Pod 反亲和性
      affinity:
        podAntiAffinity:
          requiredDuringSchedulingIgnoredDuringExecution:
          - labelSelector:
              matchExpressions:
              - key: app
                operator: In
                values: ["web-application"]
            topologyKey: kubernetes.io/hostname
          preferredDuringSchedulingIgnoredDuringExecution:
          - weight: 100
            podAffinityTerm:
              labelSelector:
                matchLabels:
                  app: web-application
              topologyKey: topology.kubernetes.io/zone
      
      # 容器配置
      containers:
      - name: web-app
        image: registry.prod.local/web-app:v1.2.3@sha256:abcdef123456...
        imagePullPolicy: IfNotPresent
        
        # 端口配置
        ports:
        - name: http
          containerPort: 8080
          protocol: TCP
        - name: metrics
          containerPort: 9090
          protocol: TCP
        
        # 环境变量
        env:
        - name: ENVIRONMENT
          value: "production"
        - name: LOG_LEVEL
          value: "INFO"
        - name: DATABASE_URL
          valueFrom:
            secretKeyRef:
              name: db-credentials
              key: url
        - name: REDIS_HOST
          valueFrom:
            configMapKeyRef:
              name: redis-config
              key: host
        
        # 资源管理
        resources:
          requests:
            cpu: "200m"
            memory: "256Mi"
            ephemeral-storage: "1Gi"
          limits:
            cpu: "1"
            memory: "1Gi"
            ephemeral-storage: "2Gi"
        
        # 健康检查
        livenessProbe:
          httpGet:
            path: /health
            port: 8080
            scheme: HTTP
          initialDelaySeconds: 60
          periodSeconds: 10
          timeoutSeconds: 5
          successThreshold: 1
          failureThreshold: 3
        
        readinessProbe:
          httpGet:
            path: /ready
            port: 8080
            scheme: HTTP
          initialDelaySeconds: 10
          periodSeconds: 5
          timeoutSeconds: 3
          successThreshold: 1
          failureThreshold: 3
        
        startupProbe:
          httpGet:
            path: /startup
            port: 8080
            scheme: HTTP
          initialDelaySeconds: 10
          periodSeconds: 5
          timeoutSeconds: 3
          successThreshold: 1
          failureThreshold: 60  # 最长5分钟启动时间
        
        # 生命周期钩子
        lifecycle:
          postStart:
            exec:
              command: ["/bin/sh", "-c", "echo $(date): App started >> /var/log/app.log"]
          preStop:
            httpGet:
              path: /shutdown
              port: 8080
              scheme: HTTP
        
        # 安全上下文
        securityContext:
          allowPrivilegeEscalation: false
          readOnlyRootFilesystem: true
          runAsNonRoot: true
          runAsUser: 1000
          runAsGroup: 3000
          capabilities:
            drop: ["ALL"]
            add: ["NET_BIND_SERVICE"]
        
        # 存储挂载
        volumeMounts:
        - name: tmp-storage
          mountPath: /tmp
        - name: logs-storage
          mountPath: /var/log
        - name: config-volume
          mountPath: /etc/app/config.yaml
          subPath: config.yaml
          readOnly: true
        - name: timezone
          mountPath: /etc/localtime
          readOnly: true
      
      # Init 容器
      initContainers:
      - name: wait-for-db
        image: busybox:1.35
        command: ['sh', '-c', 'until nc -z db-host 5432; do echo waiting for db; sleep 2; done']
        env:
        - name: DB_HOST
          valueFrom:
            configMapKeyRef:
              name: db-config
              key: host
      
      # 存储卷
      volumes:
      - name: tmp-storage
        emptyDir: {}
      - name: logs-storage
        emptyDir: {}
      - name: config-volume
        configMap:
          name: app-config
      - name: timezone
        hostPath:
          path: /usr/share/zoneinfo/Asia/Shanghai
```

#### 2.2 微服务部署模板

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: microservice-user
  namespace: backend
  labels:
    app: user-service
    version: v2.1.0
    team: backend-team
spec:
  replicas: 4
  minReadySeconds: 30  # Pod 就绪后等待30秒再继续
  
  strategy:
    type: RollingUpdate
    rollingUpdate:
      maxSurge: 1
      maxUnavailable: 0  # 零停机更新
  
  selector:
    matchLabels:
      app: user-service
  
  template:
    metadata:
      labels:
        app: user-service
        version: v2.1.0
        team: backend-team
      annotations:
        prometheus.io/scrape: "true"
        prometheus.io/port: "8080"
    
    spec:
      terminationGracePeriodSeconds: 60  # 优雅终止时间
      
      containers:
      - name: user-service
        image: registry.prod.local/user-service:v2.1.0
        ports:
        - containerPort: 8080
        envFrom:
        - configMapRef:
            name: user-service-config
        - secretRef:
            name: user-service-secrets
        
        resources:
          requests:
            cpu: "300m"
            memory: "512Mi"
          limits:
            cpu: "1"
            memory: "1Gi"
        
        # 多路径健康检查
        livenessProbe:
          httpGet:
            path: /actuator/health/liveness
            port: 8080
          failureThreshold: 3
          periodSeconds: 10
        
        readinessProbe:
          httpGet:
            path: /actuator/health/readiness
            port: 8080
          failureThreshold: 3
          periodSeconds: 5
        
        # JVM 优化参数
        env:
        - name: JAVA_OPTS
          value: "-Xmx768m -Xms512m -XX:+UseG1GC -XX:MaxGCPauseMillis=200"
```

### 3. 部署策略详解

#### 3.1 蓝绿部署策略

```yaml
# 蓝色环境 (当前生产)
apiVersion: apps/v1
kind: Deployment
metadata:
  name: app-blue
  namespace: production
spec:
  replicas: 10
  selector:
    matchLabels:
      app: app
      color: blue
  template:
    metadata:
      labels:
        app: app
        color: blue
        version: v1.0.0

---
# 绿色环境 (新版本)
apiVersion: apps/v1
kind: Deployment
metadata:
  name: app-green
  namespace: production
spec:
  replicas: 0  # 初始为0
  selector:
    matchLabels:
      app: app
      color: green
  template:
    metadata:
      labels:
        app: app
        color: green
        version: v2.0.0

---
# 生产 Service (指向蓝色)
apiVersion: v1
kind: Service
metadata:
  name: app-service
  namespace: production
spec:
  selector:
    app: app
    color: blue  # 切换此标签即可实现蓝绿切换
  ports:
  - port: 80
    targetPort: 8080

---
# 切换脚本
#!/bin/bash
# 蓝绿切换脚本
kubectl patch service app-service -p '{"spec":{"selector":{"color":"green"}}}'
```

#### 3.2 金丝雀部署策略

```yaml
# 稳定版本 (90% 流量)
apiVersion: apps/v1
kind: Deployment
metadata:
  name: app-stable
  namespace: production
spec:
  replicas: 9
  selector:
    matchLabels:
      app: app
      track: stable
  template:
    metadata:
      labels:
        app: app
        track: stable
        version: v1.0.0

---
# 金丝雀版本 (10% 流量)
apiVersion: apps/v1
kind: Deployment
metadata:
  name: app-canary
  namespace: production
spec:
  replicas: 1
  selector:
    matchLabels:
      app: app
      track: canary
  template:
    metadata:
      labels:
        app: app
        track: canary
        version: v2.0.0

---
# 统一路由 Service
apiVersion: v1
kind: Service
metadata:
  name: app-service
  namespace: production
spec:
  selector:
    app: app  # 匹配所有 track
  ports:
  - port: 80
    targetPort: 8080

---
# Istio 金丝雀配置
apiVersion: networking.istio.io/v1alpha3
kind: VirtualService
metadata:
  name: app-virtualservice
  namespace: production
spec:
  hosts:
  - app.example.com
  http:
  - route:
    - destination:
        host: app-service
        subset: v1
      weight: 90  # 90% 流量到 v1
    - destination:
        host: app-service
        subset: v2
      weight: 10  # 10% 流量到 v2
```

### 4. 高级配置模式

#### 4.1 多环境配置管理

```yaml
# 使用 Kustomize 管理多环境
# base/deployment.yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: app
spec:
  replicas: 3
  template:
    spec:
      containers:
      - name: app
        image: app:latest
        envFrom:
        - configMapRef:
            name: app-config

---
# overlays/production/deployment.yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: app
spec:
  replicas: 10  # 生产环境更多副本
  template:
    spec:
      containers:
      - name: app
        resources:
          requests:
            cpu: "500m"
            memory: "1Gi"
          limits:
            cpu: "1"
            memory: "2Gi"

---
# overlays/staging/deployment.yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: app
spec:
  replicas: 3  # 预发环境较少副本
  template:
    spec:
      containers:
      - name: app
        resources:
          requests:
            cpu: "100m"
            memory: "256Mi"
          limits:
            cpu: "500m"
            memory: "512Mi"
```

#### 4.2 滚动更新优化

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: optimized-deployment
spec:
  # 精细化滚动更新控制
  strategy:
    type: RollingUpdate
    rollingUpdate:
      maxSurge: 25%         # 最大超出25%
      maxUnavailable: 25%   # 最大不可用25%
  
  # 分批更新配置
  minReadySeconds: 30     # Pod就绪后等待30秒
  
  template:
    spec:
      # 启动探针确保应用完全启动
      containers:
      - name: app
        startupProbe:
          httpGet:
            path: /health
            port: 8080
          failureThreshold: 60  # 最长5分钟启动时间
          periodSeconds: 5
      
      # 优雅终止配置
      terminationGracePeriodSeconds: 120
      
      # 预停止钩子实现优雅关闭
      containers:
      - name: app
        lifecycle:
          preStop:
            exec:
              # 先从负载均衡摘除，再等待连接处理完
              command: ["/bin/sh", "-c", "sleep 30"]
```

### 5. 监控与告警配置

#### 5.1 Deployment 状态监控

```yaml
# Prometheus 监控指标
groups:
- name: deployment_monitoring
  rules:
  # 部署不同步告警
  - alert: DeploymentReplicasMismatch
    expr: |
      kube_deployment_status_replicas_available != 
      kube_deployment_spec_replicas
    for: 10m
    labels:
      severity: warning
    annotations:
      summary: "Deployment {{ $labels.namespace }}/{{ $labels.deployment }} 副本数不匹配"
  
  # 滚动更新卡住
  - alert: DeploymentStuck
    expr: |
      kube_deployment_status_replicas_updated != 
      kube_deployment_spec_replicas
    for: 15m
    labels:
      severity: critical
    annotations:
      summary: "Deployment {{ $labels.namespace }}/{{ $labels.deployment }} 更新卡住"
  
  # 频繁重启
  - alert: DeploymentPodCrashLooping
    expr: |
      increase(kube_pod_container_status_restarts_total{container!="POD"}[10m]) > 5
    for: 5m
    labels:
      severity: critical
    annotations:
      summary: "Deployment {{ $labels.namespace }}/{{ $labels.deployment }} Pod频繁重启"
```

#### 5.2 性能监控仪表板

```yaml
# Grafana Dashboard 配置片段
dashboard:
  title: "Deployment Performance Overview"
  panels:
  - title: "副本状态"
    targets:
    - expr: kube_deployment_status_replicas{namespace="production"}
      legendFormat: "{{deployment}} desired"
    - expr: kube_deployment_status_replicas_available{namespace="production"}
      legendFormat: "{{deployment}} available"
  
  - title: "更新进度"
    targets:
    - expr: |
        kube_deployment_status_replicas_updated / 
        kube_deployment_spec_replicas * 100
      legendFormat: "{{deployment}} update progress %"
  
  - title: "资源使用率"
    targets:
    - expr: |
        sum(rate(container_cpu_usage_seconds_total{namespace="production", container!="POD"}[5m])) by (pod) /
        sum(container_spec_cpu_quota{namespace="production", container!="POD"}) by (pod) * 100
      legendFormat: "{{pod}} CPU %"
```

### 6. 故障排查与恢复

#### 6.1 常见问题诊断

```bash
# 1. 查看 Deployment 状态
kubectl describe deployment <deployment-name> -n <namespace>

# 2. 查看 ReplicaSet 状态
kubectl get rs -l app=<app-name> -n <namespace>
kubectl describe rs <rs-name> -n <namespace>

# 3. 查看 Pod 状态和事件
kubectl get pods -l app=<app-name> -n <namespace>
kubectl describe pod <pod-name> -n <namespace>

# 4. 查看滚动更新状态
kubectl rollout status deployment/<deployment-name> -n <namespace>

# 5. 查看历史版本
kubectl rollout history deployment/<deployment-name> -n <namespace>

# 6. 回滚到指定版本
kubectl rollout undo deployment/<deployment-name> --to-revision=3 -n <namespace>

# 7. 暂停/恢复滚动更新
kubectl rollout pause deployment/<deployment-name> -n <namespace>
kubectl rollout resume deployment/<deployment-name> -n <namespace>
```

#### 6.2 自动化恢复脚本

```bash
#!/bin/bash
# Deployment 自动恢复脚本

DEPLOYMENT_NAME=$1
NAMESPACE=${2:-default}

echo "检查 Deployment $DEPLOYMENT_NAME 状态..."

# 检查副本状态
AVAILABLE=$(kubectl get deployment $DEPLOYMENT_NAME -n $NAMESPACE -o jsonpath='{.status.availableReplicas}')
DESIRED=$(kubectl get deployment $DEPLOYMENT_NAME -n $NAMESPACE -o jsonpath='{.spec.replicas}')

if [ "$AVAILABLE" != "$DESIRED" ]; then
    echo "发现副本不匹配: available=$AVAILABLE, desired=$DESIRED"
    
    # 检查最近的事件
    kubectl get events -n $NAMESPACE --field-selector involvedObject.name=$DEPLOYMENT_NAME --sort-by='.lastTimestamp' | tail -5
    
    # 尝试重启
    echo "尝试重启 Deployment..."
    kubectl rollout restart deployment/$DEPLOYMENT_NAME -n $NAMESPACE
    
    # 等待恢复
    kubectl rollout status deployment/$DEPLOYMENT_NAME -n $NAMESPACE --timeout=300s
fi

# 检查 Pod 状态
kubectl get pods -l app=$DEPLOYMENT_NAME -n $NAMESPACE | grep -E "(CrashLoopBackOff|Error|Pending)"
```

### 7. 安全加固配置

#### 7.1 完整安全配置示例

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: secure-app
  namespace: production
spec:
  replicas: 3
  template:
    metadata:
      labels:
        app: secure-app
    
    spec:
      # 服务账户和安全策略
      serviceAccountName: restricted-sa
      automountServiceAccountToken: false
      
      # 安全上下文
      securityContext:
        runAsNonRoot: true
        runAsUser: 10001
        runAsGroup: 30001
        fsGroup: 20001
        seccompProfile:
          type: RuntimeDefault
      
      containers:
      - name: app
        image: secure-registry.local/app:v1.0.0
        imagePullPolicy: Always
        
        # 容器安全上下文
        securityContext:
          allowPrivilegeEscalation: false
          readOnlyRootFilesystem: true
          runAsNonRoot: true
          runAsUser: 10001
          capabilities:
            drop: ["ALL"]
            add: ["NET_BIND_SERVICE"]
        
        # 资源限制
        resources:
          requests:
            cpu: "100m"
            memory: "128Mi"
          limits:
            cpu: "500m"
            memory: "512Mi"
        
        # 端口最小化
        ports:
        - containerPort: 8080
          protocol: TCP
        
        # 只读文件系统挂载
        volumeMounts:
        - name: tmp-storage
          mountPath: /tmp
        - name: logs-storage
          mountPath: /var/log
        
      volumes:
      - name: tmp-storage
        emptyDir: {}
      - name: logs-storage
        emptyDir: {}
```

### 8. 成本优化实践

#### 8.1 资源优化配置

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: cost-optimized-app
  namespace: staging
spec:
  replicas: 2
  
  # HPA 配置实现弹性伸缩
  template:
    spec:
      containers:
      - name: app
        resources:
          requests:
            # 降低请求值以提高调度成功率
            cpu: "50m"
            memory: "64Mi"
          limits:
            # 合理设置限制值防止单个Pod耗尽资源
            cpu: "500m"
            memory: "512Mi"
        
        # 启用垂直扩缩容建议
        env:
        - name: VPA_ENABLED
          value: "true"

---
# 配合 VPA 实现垂直扩缩容
apiVersion: autoscaling.k8s.io/v1
kind: VerticalPodAutoscaler
metadata:
  name: cost-optimized-app-vpa
spec:
  targetRef:
    apiVersion: apps/v1
    kind: Deployment
    name: cost-optimized-app
  updatePolicy:
    updateMode: "Off"  # 先观察建议，再手动调整
  resourcePolicy:
    containerPolicies:
    - containerName: app
      maxAllowed:
        cpu: "1"
        memory: "1Gi"
      minAllowed:
        cpu: "50m"
        memory: "64Mi"
```

---

**部署原则**: 零停机更新是底线，可观测性是保障，安全性是前提，成本优化是目标。

---

**文档维护**: Kusheet Project | **作者**: Allen Galler (allengaller@gmail.com)