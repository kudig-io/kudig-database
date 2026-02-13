# 03-边缘计算生产部署

> **适用范围**: Kubernetes v1.25-v1.32 | **维护状态**: 🔧 持续更新中 | **专家级别**: ⭐⭐⭐⭐⭐

## 📋 概述

边缘计算场景下的Kubernetes部署面临独特的挑战，包括资源受限、网络不稳定、地理位置分散等。本文档详细介绍边缘环境的生产部署策略和最佳实践。

## 🌐 边缘计算架构

### 边缘节点分类

#### 1. 微边缘节点 (Micro Edge)
```yaml
# 资源受限节点配置
apiVersion: v1
kind: Node
metadata:
  name: micro-edge-01
  labels:
    node-role.kubernetes.io/edge: ""
    edge.tier: micro
    location: factory-floor
spec:
  taints:
  - key: node-role.kubernetes.io/edge
    value: micro
    effect: NoSchedule
---
# 微边缘Deployment配置
apiVersion: apps/v1
kind: Deployment
metadata:
  name: micro-edge-app
spec:
  replicas: 1
  selector:
    matchLabels:
      app: edge-collector
  template:
    metadata:
      labels:
        app: edge-collector
        edge.tier: micro
    spec:
      nodeSelector:
        edge.tier: micro
      containers:
      - name: collector
        image: edge-collector:latest
        resources:
          requests:
            memory: "64Mi"
            cpu: "100m"
          limits:
            memory: "128Mi"
            cpu: "200m"
        volumeMounts:
        - name: data
          mountPath: /data
      volumes:
      - name: data
        emptyDir: {}
```

#### 2. 小边缘节点 (Mini Edge)
```yaml
# 小边缘节点配置
apiVersion: v1
kind: Node
metadata:
  name: mini-edge-01
  labels:
    node-role.kubernetes.io/edge: ""
    edge.tier: mini
    location: retail-store
spec:
  taints:
  - key: node-role.kubernetes.io/edge
    value: mini
    effect: NoSchedule
---
# 小边缘StatefulSet配置
apiVersion: apps/v1
kind: StatefulSet
metadata:
  name: mini-edge-db
spec:
  serviceName: mini-edge-db
  replicas: 1
  selector:
    matchLabels:
      app: edge-database
  template:
    metadata:
      labels:
        app: edge-database
        edge.tier: mini
    spec:
      nodeSelector:
        edge.tier: mini
      containers:
      - name: database
        image: sqlite:latest
        ports:
        - containerPort: 3306
        env:
        - name: SQLITE_DATA_DIR
          value: /data/db
        volumeMounts:
        - name: db-storage
          mountPath: /data
  volumeClaimTemplates:
  - metadata:
      name: db-storage
    spec:
      accessModes: ["ReadWriteOnce"]
      resources:
        requests:
          storage: 10Gi
```

### 网络架构设计

#### 1. 边缘网络拓扑
```yaml
# 边缘网络策略配置
apiVersion: cilium.io/v2
kind: CiliumNetworkPolicy
metadata:
  name: edge-network-policy
spec:
  endpointSelector:
    matchLabels:
      node-role.kubernetes.io/edge: ""
  ingress:
  - fromEndpoints:
    - matchLabels:
        k8s:io.kubernetes.pod.namespace: monitoring
    toPorts:
    - ports:
      - port: "9100"
        protocol: TCP
  egress:
  - toCIDR:
    - 10.0.0.0/8
    - 172.16.0.0/12
```

#### 2. 离线运行支持
```yaml
# 线运行DaemonSet配置
apiVersion: apps/v1
kind: DaemonSet
metadata:
  name: offline-agent
  namespace: edge-system
spec:
  selector:
    matchLabels:
      app: offline-agent
  template:
    metadata:
      labels:
        app: offline-agent
    spec:
      tolerations:
      - key: node-role.kubernetes.io/edge
        operator: Exists
        effect: NoSchedule
      containers:
      - name: agent
        image: offline-agent:latest
        env:
        - name: OFFLINE_MODE
          value: "true"
        - name: SYNC_INTERVAL
          value: "300"  # 5分钟同步间隔
        volumeMounts:
        - name: cache
          mountPath: /cache
      volumes:
      - name: cache
        emptyDir:
          sizeLimit: 1Gi
```

## 📦 镜像管理策略

### 轻量化镜像构建

#### 1. 多阶段构建优化
```dockerfile
# 多阶段构建Dockerfile
FROM golang:1.19-alpine AS builder
WORKDIR /app
COPY . .
RUN CGO_ENABLED=0 GOOS=linux go build -a -installsuffix cgo -o main .

FROM alpine:latest
RUN apk --no-cache add ca-certificates
WORKDIR /root/
COPY --from=builder /app/main .
CMD ["./main"]

# 镜像大小对比：原始 1.2GB → 优化后 15MB
```

#### 2. 镜像分层缓存
```yaml
# Kaniko缓存配置
apiVersion: batch/v1
kind: Job
metadata:
  name: kaniko-build
spec:
  template:
    spec:
      containers:
      - name: kaniko
        image: gcr.io/kaniko-project/executor:latest
        args:
        - "--dockerfile=Dockerfile"
        - "--context=dir:///workspace"
        - "--destination=my-registry.com/edge-app:latest"
        - "--cache=true"
        - "--cache-repo=my-registry.com/cache"
        volumeMounts:
        - name: workspace
          mountPath: /workspace
      volumes:
      - name: workspace
        configMap:
          name: build-context
```

### 镜像预推送到边缘

#### 1. 镜像预加载脚本
```bash
#!/bin/bash
# 边缘节点镜像预加载脚本

IMAGES=(
  "nginx:1.21-alpine"
  "redis:7.0-alpine"
  "busybox:1.35"
)

for image in "${IMAGES[@]}"; do
  echo "Loading image: $image"
  docker pull "$image"
  docker save "$image" | gzip > "/tmp/${image//\//_}.tar.gz"
done

# 将压缩包传输到边缘节点并加载
scp /tmp/*.tar.gz edge-node:/tmp/
ssh edge-node "for file in /tmp/*.tar.gz; do docker load -i \$file; done"
```

#### 2. 镜像仓库边缘缓存
```yaml
# Registry边缘缓存配置
apiVersion: apps/v1
kind: Deployment
metadata:
  name: edge-registry-cache
spec:
  replicas: 1
  selector:
    matchLabels:
      app: registry-cache
  template:
    metadata:
      labels:
        app: registry-cache
    spec:
      containers:
      - name: registry
        image: registry:2.8
        env:
        - name: REGISTRY_PROXY_REMOTEURL
          value: https://hub.docker.com
        - name: REGISTRY_STORAGE_FILESYSTEM_ROOTDIRECTORY
          value: /var/lib/registry
        volumeMounts:
        - name: cache-storage
          mountPath: /var/lib/registry
      volumes:
      - name: cache-storage
        persistentVolumeClaim:
          claimName: registry-cache-pvc
---
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: registry-cache-pvc
spec:
  accessModes:
  - ReadWriteOnce
  resources:
    requests:
      storage: 50Gi
```

## 🔧 资源管理优化

### 节点资源分配

#### 1. 系统保留资源
```yaml
# Kubelet资源配置
apiVersion: v1
kind: ConfigMap
metadata:
  name: kubelet-config
  namespace: kube-system
data:
  kubelet: |
    apiVersion: kubelet.config.k8s.io/v1beta1
    kind: KubeletConfiguration
    systemReserved:
      cpu: 500m
      memory: 1Gi
      ephemeral-storage: 1Gi
    kubeReserved:
      cpu: 200m
      memory: 256Mi
      ephemeral-storage: 1Gi
    evictionHard:
      memory.available: "100Mi"
      nodefs.available: "5%"
      imagefs.available: "5%"
    evictionMinimumReclaim:
      memory.available: "0Mi"
      nodefs.available: "500Mi"
      imagefs.available: "2Gi"
```

#### 2. 边缘工作负载优先级
```yaml
# PriorityClass配置
apiVersion: scheduling.k8s.io/v1
kind: PriorityClass
metadata:
  name: edge-critical
value: 1000000
globalDefault: false
description: "Priority class for edge critical workloads"
---
apiVersion: scheduling.k8s.io/v1
kind: PriorityClass
metadata:
  name: edge-normal
value: 1000
globalDefault: true
description: "Default priority class for edge workloads"
```

### 存储优化策略

#### 1. 本地存储管理
```yaml
# Local PV配置
apiVersion: v1
kind: PersistentVolume
metadata:
  name: local-pv-1
spec:
  capacity:
    storage: 100Gi
  accessModes:
  - ReadWriteOnce
  persistentVolumeReclaimPolicy: Retain
  storageClassName: local-storage
  local:
    path: /mnt/disks/ssd1
  nodeAffinity:
    required:
      nodeSelectorTerms:
      - matchExpressions:
        - key: kubernetes.io/hostname
          operator: In
          values:
          - edge-node-1
```

#### 2. 数据持久化策略
```yaml
# 边缘数据库StatefulSet
apiVersion: apps/v1
kind: StatefulSet
metadata:
  name: edge-database
spec:
  serviceName: edge-database
  replicas: 1
  selector:
    matchLabels:
      app: database
  template:
    metadata:
      labels:
        app: database
    spec:
      containers:
      - name: postgres
        image: postgres:14-alpine
        env:
        - name: POSTGRES_DB
          value: edgedata
        - name: POSTGRES_USER
          valueFrom:
            secretKeyRef:
              name: db-secret
              key: username
        - name: POSTGRES_PASSWORD
          valueFrom:
            secretKeyRef:
              name: db-secret
              key: password
        volumeMounts:
        - name: postgres-storage
          mountPath: /var/lib/postgresql/data
        resources:
          requests:
            memory: "256Mi"
            cpu: "200m"
          limits:
            memory: "512Mi"
            cpu: "500m"
  volumeClaimTemplates:
  - metadata:
      name: postgres-storage
    spec:
      accessModes: ["ReadWriteOnce"]
      storageClassName: local-storage
      resources:
        requests:
          storage: 20Gi
```

## 🛡️ 安全加固措施

### 边缘节点安全

#### 1. 最小化攻击面
```yaml
# 边缘节点安全配置
apiVersion: apps/v1
kind: DaemonSet
metadata:
  name: edge-security-agent
  namespace: edge-system
spec:
  selector:
    matchLabels:
      app: security-agent
  template:
    metadata:
      labels:
        app: security-agent
    spec:
      hostPID: true
      hostNetwork: true
      containers:
      - name: falco
        image: falcosecurity/falco:latest
        securityContext:
          privileged: true
        env:
        - name: FALCO_ENGINE_VERSION
          value: "12"
        volumeMounts:
        - name: dev
          mountPath: /host/dev
        - name: proc
          mountPath: /host/proc
        - name: boot
          mountPath: /host/boot
        - name: lib-modules
          mountPath: /host/lib/modules
        - name: usr
          mountPath: /host/usr
        - name: etc
          mountPath: /host/etc
      volumes:
      - name: dev
        hostPath:
          path: /dev
      - name: proc
        hostPath:
          path: /proc
      - name: boot
        hostPath:
          path: /boot
      - name: lib-modules
        hostPath:
          path: /lib/modules
      - name: usr
        hostPath:
          path: /usr
      - name: etc
        hostPath:
          path: /etc
```

#### 2. 网络策略强化
```yaml
# 边缘网络安全策略
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: edge-isolation-policy
spec:
  podSelector:
    matchLabels:
      node-role.kubernetes.io/edge: ""
  policyTypes:
  - Ingress
  - Egress
  ingress:
  - from:
    - namespaceSelector:
        matchLabels:
          name: monitoring
    ports:
    - protocol: TCP
      port: 9100
  egress:
  - to:
    - ipBlock:
        cidr: 10.0.0.0/8
    ports:
    - protocol: TCP
      port: 53
    - protocol: UDP
      port: 53
```

### 身份认证增强

#### 1. 证书轮换自动化
```yaml
# Cert-Manager配置
apiVersion: cert-manager.io/v1
kind: Certificate
metadata:
  name: edge-node-cert
spec:
  secretName: edge-node-tls
  duration: 2160h  # 90天
  renewBefore: 360h  # 15天提前续期
  subject:
    organizations:
    - edge-cluster
  commonName: edge-node
  issuerRef:
    name: ca-issuer
    kind: ClusterIssuer
  privateKey:
    algorithm: RSA
    encoding: PKCS1
    size: 2048
```

#### 2. 边缘设备认证
```yaml
# SPIFFE/SPIRE配置
apiVersion: spire.spiffe.io/v1alpha1
kind: ClusterSPIFFEID
metadata:
  name: edge-workload
spec:
  className: "edge-cluster"
  spiffeIDTemplate: "spiffe://edge.example.org/workload/{{ .PodMeta.Name }}"
  podSelector:
    matchLabels:
      edge.workload/type: service
  workloadSelectorTemplates:
  - "k8s:ns:{{.PodMeta.Namespace}}"
  - "k8s:sa:{{.PodSpec.ServiceAccountName}}"
```

## 📊 监控与运维

### 边缘监控架构

#### 1. 轻量化监控代理
```yaml
# Node Exporter轻量化配置
apiVersion: apps/v1
kind: DaemonSet
metadata:
  name: node-exporter-edge
  namespace: monitoring
spec:
  selector:
    matchLabels:
      app: node-exporter
  template:
    metadata:
      labels:
        app: node-exporter
    spec:
      containers:
      - name: node-exporter
        image: quay.io/prometheus/node-exporter:v1.4.0
        args:
        - --collector.disable-defaults
        - --collector.cpu
        - --collector.meminfo
        - --collector.filesystem
        - --collector.netdev
        - --collector.stat
        - --web.listen-address=:9100
        - --web.telemetry-path=/metrics
        ports:
        - containerPort: 9100
        resources:
          requests:
            cpu: 50m
            memory: 64Mi
          limits:
            cpu: 100m
            memory: 128Mi
```

#### 2. 边缘日志收集
```yaml
# Fluent Bit边缘配置
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
        Tag               kube.*
        Path              /var/log/containers/*.log
        Parser            docker
        DB                /var/log/flb_kube.db
        Mem_Buf_Limit     5MB
        Skip_Long_Lines   On
        Refresh_Interval  10
    
    [FILTER]
        Name                kubernetes
        Match               kube.*
        Kube_URL            https://kubernetes.default.svc:443
        Kube_CA_File        /var/run/secrets/kubernetes.io/serviceaccount/ca.crt
        Kube_Token_File     /var/run/secrets/kubernetes.io/serviceaccount/token
        Kube_Tag_Prefix     kube.var.log.containers.
        Merge_Log           On
        Merge_Log_Key       log_processed
        K8S-Logging.Parser  On
        K8S-Logging.Exclude Off
    
    [OUTPUT]
        Name  forward
        Match *
        Host  central-logging.example.com
        Port  24224
```

### 远程运维支持

#### 1. SSH隧道配置
```yaml
# 反向SSH隧道Deployment
apiVersion: apps/v1
kind: Deployment
metadata:
  name: ssh-tunnel
  namespace: edge-system
spec:
  replicas: 1
  selector:
    matchLabels:
      app: ssh-tunnel
  template:
    metadata:
      labels:
        app: ssh-tunnel
    spec:
      containers:
      - name: autossh
        image: jnovack/autossh:latest
        env:
        - name: SSH_TARGET
          value: "bastion.example.com"
        - name: SSH_USER
          value: "edge-user"
        - name: SSH_PORT
          value: "2222"
        - name: LOCAL_PORT
          value: "22"
        - name: REMOTE_PORT
          value: "2201"
        securityContext:
          capabilities:
            add: ["NET_BIND_SERVICE"]
```

#### 2. 远程诊断工具
```bash
#!/bin/bash
# 边缘节点诊断脚本

NODE_NAME=$1
DIAG_DIR="/tmp/diag-$(date +%Y%m%d-%H%M%S)"

mkdir -p $DIAG_DIR

# 收集基本信息
echo "=== System Information ===" > $DIAG_DIR/system-info.txt
uname -a >> $DIAG_DIR/system-info.txt
cat /etc/os-release >> $DIAG_DIR/system-info.txt

# 收集Kubernetes信息
echo "=== Kubernetes Status ===" > $DIAG_DIR/k8s-status.txt
kubectl get nodes -o wide >> $DIAG_DIR/k8s-status.txt
kubectl get pods -A >> $DIAG_DIR/k8s-status.txt
kubectl describe node $NODE_NAME >> $DIAG_DIR/k8s-status.txt

# 收集资源使用情况
echo "=== Resource Usage ===" > $DIAG_DIR/resources.txt
free -h >> $DIAG_DIR/resources.txt
df -h >> $DIAG_DIR/resources.txt
top -bn1 | head -20 >> $DIAG_DIR/resources.txt

# 打包诊断信息
tar -czf "${DIAG_DIR}.tar.gz" $DIAG_DIR
echo "Diagnostic package created: ${DIAG_DIR}.tar.gz"
```

## 🔧 部署实施检查清单

### 预部署准备
- [ ] 评估边缘节点硬件规格和网络条件
- [ ] 设计适合的边缘架构拓扑
- [ ] 准备轻量化的容器镜像
- [ ] 配置安全策略和访问控制
- [ ] 设置监控告警机制
- [ ] 制定远程运维流程

### 部署实施
- [ ] 在边缘节点安装轻量化Kubernetes发行版
- [ ] 配置节点标签和污点
- [ ] 部署必要的系统组件
- [ ] 验证网络连通性和安全策略
- [ ] 测试应用部署和扩缩容
- [ ] 验证监控数据收集

### 运营维护
- [ ] 建立定期健康检查机制
- [ ] 实施自动化故障恢复
- [ ] 优化资源使用效率
- [ ] 保持安全补丁及时更新
- [ ] 完善文档和操作手册

---

*本文档为边缘计算环境下的Kubernetes生产部署提供全面指导*