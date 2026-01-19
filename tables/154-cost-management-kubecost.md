# 成本管理与 FinOps

## 概述

Kubernetes 成本管理是确保云原生基础设施经济高效运行的关键实践。本文档详细介绍成本构成分析、监控工具部署、优化策略和 FinOps 成熟度模型。

## 成本架构

### Kubernetes 成本构成模型

```
┌─────────────────────────────────────────────────────────────────────────────────────┐
│                           Kubernetes 成本构成模型                                    │
│                                                                                      │
│   ┌─────────────────────────────────────────────────────────────────────────────┐   │
│   │                          总成本 (Total Cost of Ownership)                    │   │
│   │                                                                              │   │
│   │   ┌─────────────────────────────────────────────────────────────────────┐   │   │
│   │   │                      计算成本 (50-70%)                               │   │   │
│   │   │                                                                      │   │   │
│   │   │   ┌─────────────┐   ┌─────────────┐   ┌─────────────┐               │   │   │
│   │   │   │   CPU 成本   │   │   内存成本  │   │   GPU 成本  │               │   │   │
│   │   │   │             │   │             │   │             │               │   │   │
│   │   │   │ • 按需实例  │   │ • 按需实例  │   │ • 训练任务  │               │   │   │
│   │   │   │ • 预留实例  │   │ • 高内存型  │   │ • 推理服务  │               │   │   │
│   │   │   │ • Spot实例  │   │             │   │ • Spot GPU  │               │   │   │
│   │   │   └─────────────┘   └─────────────┘   └─────────────┘               │   │   │
│   │   │                                                                      │   │   │
│   │   └──────────────────────────────────────────────────────────────────────┘   │   │
│   │                                                                              │   │
│   │   ┌────────────────────────┐   ┌────────────────────────┐                   │   │
│   │   │     存储成本 (15-25%)   │   │     网络成本 (10-20%)   │                   │   │
│   │   │                        │   │                        │                   │   │
│   │   │  • 块存储 (EBS/Disk)   │   │  • 跨 AZ 流量          │                   │   │
│   │   │  • 文件存储 (EFS/NFS)  │   │  • 跨区域流量          │                   │   │
│   │   │  • 对象存储 (S3)       │   │  • 公网出口流量        │                   │   │
│   │   │  • 快照和备份          │   │  • 负载均衡            │                   │   │
│   │   └────────────────────────┘   └────────────────────────┘                   │   │
│   │                                                                              │   │
│   │   ┌────────────────────────┐   ┌────────────────────────┐                   │   │
│   │   │     管理成本 (5-10%)    │   │     其他成本 (5-10%)    │                   │   │
│   │   │                        │   │                        │                   │   │
│   │   │  • 控制平面 (托管)     │   │  • 日志存储            │                   │   │
│   │   │  • 监控/可观测性       │   │  • 安全工具            │                   │   │
│   │   │  • 服务网格            │   │  • CI/CD 运行          │                   │   │
│   │   │  • 备份/DR             │   │  • 开发环境            │                   │   │
│   │   └────────────────────────┘   └────────────────────────┘                   │   │
│   │                                                                              │   │
│   └─────────────────────────────────────────────────────────────────────────────┘   │
│                                                                                      │
└──────────────────────────────────────────────────────────────────────────────────────┘
```

### 成本分配模型

```
┌─────────────────────────────────────────────────────────────────────────────────────┐
│                              成本分配架构                                            │
│                                                                                      │
│   ┌─────────────────────────────────────────────────────────────────────────────┐   │
│   │                          组织层级 (Organization)                             │   │
│   │                                                                              │   │
│   │   ┌──────────────────────────────────────────────────────────────────────┐  │   │
│   │   │                          公司 (Company)                               │  │   │
│   │   │                                                                       │  │   │
│   │   │   ┌─────────────────────────────────────────────────────────────┐    │  │   │
│   │   │   │                     部门 (Department)                        │    │  │   │
│   │   │   │                                                              │    │  │   │
│   │   │   │   ┌────────────────────────────────────────────────────┐    │    │  │   │
│   │   │   │   │                  团队 (Team)                        │    │    │  │   │
│   │   │   │   │                                                     │    │    │  │   │
│   │   │   │   │   ┌───────────────────────────────────────────┐    │    │    │  │   │
│   │   │   │   │   │              项目 (Project)                │    │    │    │  │   │
│   │   │   │   │   │                                            │    │    │    │  │   │
│   │   │   │   │   │   ┌────────────────────────────────────┐  │    │    │    │  │   │
│   │   │   │   │   │   │         环境 (Environment)          │  │    │    │    │  │   │
│   │   │   │   │   │   │   prod | staging | dev | test      │  │    │    │    │  │   │
│   │   │   │   │   │   └────────────────────────────────────┘  │    │    │    │  │   │
│   │   │   │   │   │                                            │    │    │    │  │   │
│   │   │   │   │   └───────────────────────────────────────────┘    │    │    │  │   │
│   │   │   │   │                                                     │    │    │  │   │
│   │   │   │   └────────────────────────────────────────────────────┘    │    │  │   │
│   │   │   │                                                              │    │  │   │
│   │   │   └─────────────────────────────────────────────────────────────┘    │  │   │
│   │   │                                                                       │  │   │
│   │   └──────────────────────────────────────────────────────────────────────┘  │   │
│   │                                                                              │   │
│   └─────────────────────────────────────────────────────────────────────────────┘   │
│                                                                                      │
│   ┌─────────────────────────────────────────────────────────────────────────────┐   │
│   │                          Kubernetes 层级                                     │   │
│   │                                                                              │   │
│   │   Cluster ──► Namespace ──► Deployment ──► Pod ──► Container               │   │
│   │      │            │              │           │          │                   │   │
│   │      │            │              │           │          │                   │   │
│   │   labels:      labels:       labels:     labels:    resources:             │   │
│   │   • env        • team        • app       • version  • requests             │   │
│   │   • region     • project     • component             • limits              │   │
│   │   • cost-center• owner                                                      │   │
│   │                                                                              │   │
│   └─────────────────────────────────────────────────────────────────────────────┘   │
│                                                                                      │
└──────────────────────────────────────────────────────────────────────────────────────┘
```

## 成本监控工具对比

| 工具 | 类型 | 核心功能 | 成本 | 适用场景 |
|-----|-----|---------|------|---------|
| **Kubecost** | 开源/商业 | 成本分配、优化建议、预算告警 | 免费版可用 | 中大型集群 |
| **OpenCost** | 开源 (CNCF) | 成本监控、Prometheus 集成 | 免费 | 任意规模 |
| **CloudHealth** | 商业 | 多云成本管理、治理 | 付费 | 企业级多云 |
| **Spot.io** | 商业 | 成本优化自动化、Spot 管理 | 付费 | 需要自动化 |
| **CAST AI** | 商业 | 自动优化、跨云调度 | 付费 | 多云/混合云 |
| **AWS Cost Explorer** | 云厂商 | AWS 原生成本分析 | 包含 | AWS 用户 |
| **阿里云成本分析** | 云厂商 | ACK 原生成本统计 | 包含 | 阿里云用户 |

## Kubecost 部署

### Helm 安装

```bash
#!/bin/bash
# deploy-kubecost.sh
# Kubecost 完整部署脚本

set -e

NAMESPACE="kubecost"
RELEASE_NAME="kubecost"

echo "=== 部署 Kubecost ==="

# 添加 Helm 仓库
helm repo add kubecost https://kubecost.github.io/cost-analyzer/
helm repo update

# 创建命名空间
kubectl create namespace $NAMESPACE --dry-run=client -o yaml | kubectl apply -f -

# 创建 values 文件
cat > kubecost-values.yaml << 'EOF'
# Kubecost Helm Values

# 全局配置
global:
  prometheus:
    enabled: true
    nodeExporter:
      enabled: true

# 产品配置
kubecostProductConfigs:
  # 集群名称
  clusterName: "production-cluster"
  
  # 货币设置
  currencyCode: "USD"
  
  # 共享成本分配
  sharedCostEnabled: true
  sharedNamespaces:
    - kube-system
    - monitoring
    - ingress-nginx
    
# Prometheus 配置
prometheus:
  server:
    retention: 30d
    persistentVolume:
      enabled: true
      size: 32Gi
    resources:
      requests:
        cpu: 500m
        memory: 2Gi
      limits:
        cpu: 2
        memory: 8Gi
        
  nodeExporter:
    enabled: true
    
  alertmanager:
    enabled: false

# 网络成本监控
networkCosts:
  enabled: true
  config:
    # 按区域定价
    zoneCost: 0.01
    regionCost: 0.02
    internetCost: 0.12

# 持久化
persistentVolume:
  enabled: true
  size: 32Gi
  storageClass: "gp3"

# 资源配置
kubecostModel:
  resources:
    requests:
      cpu: 200m
      memory: 512Mi
    limits:
      cpu: 1
      memory: 2Gi
      
# 前端配置
kubecostFrontend:
  resources:
    requests:
      cpu: 50m
      memory: 64Mi
    limits:
      cpu: 200m
      memory: 256Mi

# 服务配置
service:
  type: ClusterIP
  port: 9090

# Ingress 配置 (可选)
ingress:
  enabled: false
  # className: nginx
  # hosts:
  #   - host: kubecost.example.com
  #     paths:
  #       - path: /
  #         pathType: Prefix

# RBAC
rbac:
  create: true

# ServiceAccount
serviceAccount:
  create: true
  # annotations:
  #   eks.amazonaws.com/role-arn: arn:aws:iam::ACCOUNT:role/kubecost-role
EOF

# 部署 Kubecost
helm upgrade --install $RELEASE_NAME kubecost/cost-analyzer \
  --namespace $NAMESPACE \
  --values kubecost-values.yaml \
  --wait --timeout 10m

echo "=== 等待 Pod 就绪 ==="
kubectl wait --for=condition=Ready pod \
  -l app=cost-analyzer \
  -n $NAMESPACE \
  --timeout=300s

echo "=== 部署完成 ==="
echo "访问命令: kubectl port-forward -n $NAMESPACE svc/kubecost-cost-analyzer 9090:9090"
echo "浏览器访问: http://localhost:9090"
```

### OpenCost 部署 (开源替代)

```yaml
# opencost-deployment.yaml
apiVersion: v1
kind: Namespace
metadata:
  name: opencost

---
apiVersion: apps/v1
kind: Deployment
metadata:
  name: opencost
  namespace: opencost
spec:
  replicas: 1
  selector:
    matchLabels:
      app: opencost
  template:
    metadata:
      labels:
        app: opencost
    spec:
      serviceAccountName: opencost
      containers:
        - name: opencost
          image: ghcr.io/opencost/opencost:latest
          ports:
            - containerPort: 9003
          env:
            - name: PROMETHEUS_SERVER_ENDPOINT
              value: "http://prometheus-server.monitoring:9090"
            - name: CLOUD_PROVIDER_API_KEY
              valueFrom:
                secretKeyRef:
                  name: opencost-secrets
                  key: CLOUD_PROVIDER_API_KEY
                  optional: true
            - name: CLUSTER_ID
              value: "production-cluster"
          resources:
            requests:
              cpu: 100m
              memory: 256Mi
            limits:
              cpu: 500m
              memory: 1Gi
              
        - name: opencost-ui
          image: ghcr.io/opencost/opencost-ui:latest
          ports:
            - containerPort: 9090
          resources:
            requests:
              cpu: 50m
              memory: 64Mi

---
apiVersion: v1
kind: Service
metadata:
  name: opencost
  namespace: opencost
spec:
  selector:
    app: opencost
  ports:
    - name: api
      port: 9003
      targetPort: 9003
    - name: ui
      port: 9090
      targetPort: 9090

---
apiVersion: v1
kind: ServiceAccount
metadata:
  name: opencost
  namespace: opencost

---
apiVersion: rbac.authorization.k8s.io/v1
kind: ClusterRole
metadata:
  name: opencost
rules:
  - apiGroups: [""]
    resources:
      - configmaps
      - deployments
      - nodes
      - pods
      - services
      - resourcequotas
      - replicationcontrollers
      - limitranges
      - persistentvolumeclaims
      - persistentvolumes
      - namespaces
      - endpoints
    verbs: ["get", "list", "watch"]
  - apiGroups: ["extensions", "apps"]
    resources: ["daemonsets", "deployments", "replicasets"]
    verbs: ["get", "list", "watch"]
  - apiGroups: ["batch"]
    resources: ["cronjobs", "jobs"]
    verbs: ["get", "list", "watch"]
  - apiGroups: ["autoscaling"]
    resources: ["horizontalpodautoscalers"]
    verbs: ["get", "list", "watch"]
  - apiGroups: ["policy"]
    resources: ["poddisruptionbudgets"]
    verbs: ["get", "list", "watch"]
  - apiGroups: ["storage.k8s.io"]
    resources: ["storageclasses"]
    verbs: ["get", "list", "watch"]

---
apiVersion: rbac.authorization.k8s.io/v1
kind: ClusterRoleBinding
metadata:
  name: opencost
roleRef:
  apiGroup: rbac.authorization.k8s.io
  kind: ClusterRole
  name: opencost
subjects:
  - kind: ServiceAccount
    name: opencost
    namespace: opencost
```

## 资源优化配置

### VPA 资源建议

```yaml
# vpa-recommendation.yaml
apiVersion: autoscaling.k8s.io/v1
kind: VerticalPodAutoscaler
metadata:
  name: myapp-vpa
  namespace: production
spec:
  targetRef:
    apiVersion: apps/v1
    kind: Deployment
    name: myapp
  updatePolicy:
    updateMode: "Off"  # 仅建议模式,不自动更新
  resourcePolicy:
    containerPolicies:
      - containerName: "*"
        minAllowed:
          cpu: 50m
          memory: 64Mi
        maxAllowed:
          cpu: 4
          memory: 8Gi
        controlledResources:
          - cpu
          - memory
        controlledValues: RequestsAndLimits

---
# VPA 推荐配置 - 自动更新模式
apiVersion: autoscaling.k8s.io/v1
kind: VerticalPodAutoscaler
metadata:
  name: backend-vpa-auto
  namespace: production
spec:
  targetRef:
    apiVersion: apps/v1
    kind: Deployment
    name: backend
  updatePolicy:
    updateMode: "Auto"
    minReplicas: 2  # 保证最小副本数
  resourcePolicy:
    containerPolicies:
      - containerName: backend
        minAllowed:
          cpu: 100m
          memory: 128Mi
        maxAllowed:
          cpu: 2
          memory: 4Gi
```

### Cluster Autoscaler 成本优化

```yaml
# cluster-autoscaler-cost-config.yaml
apiVersion: v1
kind: ConfigMap
metadata:
  name: cluster-autoscaler-config
  namespace: kube-system
data:
  config.yaml: |
    # 扩展器配置 - 优先使用最便宜的节点池
    expanders:
      - priority
      - least-waste
    
    # 节点池优先级 (数字越小优先级越高)
    priorities: |
      10:
        - .*spot.*           # 最优先 Spot 实例
      20:
        - .*preemptible.*    # 其次抢占式实例
      50:
        - .*ondemand.*       # 最后按需实例
    
    # 缩容配置
    scale-down-enabled: true
    scale-down-delay-after-add: 10m          # 扩容后等待缩容
    scale-down-delay-after-delete: 0s        # 删除节点后等待
    scale-down-delay-after-failure: 3m       # 失败后等待
    scale-down-unneeded-time: 10m            # 节点空闲时间
    scale-down-unready-time: 20m             # 不健康节点空闲时间
    scale-down-utilization-threshold: 0.5    # 利用率阈值
    
    # 节点组配置
    balance-similar-node-groups: true        # 平衡相似节点组
    
    # 性能配置
    scan-interval: 10s
    max-node-provision-time: 15m
    max-graceful-termination-sec: 600
    max-empty-bulk-delete: 10

---
# Karpenter 成本优化配置
apiVersion: karpenter.sh/v1beta1
kind: NodePool
metadata:
  name: cost-optimized
spec:
  template:
    spec:
      requirements:
        - key: kubernetes.io/arch
          operator: In
          values: ["amd64"]
        - key: karpenter.sh/capacity-type
          operator: In
          values: ["spot", "on-demand"]  # 优先 Spot
        - key: node.kubernetes.io/instance-type
          operator: In
          values:
            - m5.large
            - m5.xlarge
            - m5a.large
            - m5a.xlarge
            - m6i.large
            - m6i.xlarge
      nodeClassRef:
        name: default
  limits:
    cpu: 1000
    memory: 1000Gi
  disruption:
    consolidationPolicy: WhenUnderutilized
    consolidateAfter: 30s
  # 权重配置 - 优先选择 Spot
  weight: 100
```

## 成本标签体系

### 标签规范

```yaml
# cost-labeling-standard.yaml

---
# 命名空间级别标签
apiVersion: v1
kind: Namespace
metadata:
  name: team-backend
  labels:
    # 组织标签
    team: backend
    department: engineering
    cost-center: "CC-12345"
    
    # 环境标签
    environment: production
    
    # 管理标签
    owner: backend-team@company.com
    managed-by: terraform

---
# Deployment 级别标签
apiVersion: apps/v1
kind: Deployment
metadata:
  name: api-server
  namespace: team-backend
  labels:
    # 应用标签
    app: api-server
    app.kubernetes.io/name: api-server
    app.kubernetes.io/component: backend
    app.kubernetes.io/part-of: platform
    
    # 成本标签
    team: backend
    project: platform-api
    cost-center: "CC-12345"
    
    # 运维标签
    tier: critical
    sla: gold
spec:
  template:
    metadata:
      labels:
        app: api-server
        team: backend
        project: platform-api
      annotations:
        # Kubecost 注解
        cost.kubernetes.io/team: "backend"
        cost.kubernetes.io/project: "platform-api"
    spec:
      containers:
        - name: api-server
          resources:
            requests:
              cpu: 500m
              memory: 512Mi
            limits:
              cpu: 2
              memory: 2Gi
```

### 标签强制策略

```yaml
# cost-label-policy.yaml
# 使用 Kyverno 强制成本标签

apiVersion: kyverno.io/v1
kind: ClusterPolicy
metadata:
  name: require-cost-labels
  annotations:
    policies.kyverno.io/title: 强制成本标签
    policies.kyverno.io/description: 所有 Pod 必须包含成本分配标签
spec:
  validationFailureAction: Enforce
  background: true
  rules:
    - name: require-team-label
      match:
        any:
          - resources:
              kinds:
                - Pod
              namespaces:
                - "!kube-system"
                - "!kube-public"
                - "!monitoring"
      validate:
        message: "所有 Pod 必须包含 'team' 标签用于成本分配"
        pattern:
          metadata:
            labels:
              team: "?*"
              
    - name: require-project-label
      match:
        any:
          - resources:
              kinds:
                - Pod
              namespaces:
                - production
                - staging
      validate:
        message: "生产/预发环境 Pod 必须包含 'project' 标签"
        pattern:
          metadata:
            labels:
              project: "?*"
              
    - name: require-cost-center
      match:
        any:
          - resources:
              kinds:
                - Namespace
      validate:
        message: "命名空间必须包含 'cost-center' 标签"
        pattern:
          metadata:
            labels:
              cost-center: "?*"
```

## 成本查询与分析

### Prometheus 成本查询

```promql
# cost-prometheus-queries.promql

# =============================================================================
# 按命名空间统计 CPU 成本
# =============================================================================
sum by (namespace) (
  rate(container_cpu_usage_seconds_total{container!="", namespace!="kube-system"}[5m])
) * on (node) group_left() 
(node_hourly_cost / 3600)

# =============================================================================
# 按命名空间统计内存成本
# =============================================================================
sum by (namespace) (
  container_memory_usage_bytes{container!="", namespace!="kube-system"}
) * on (node) group_left() 
(node_hourly_cost / 3600 / 1024 / 1024 / 1024)

# =============================================================================
# 按标签统计成本 (team)
# =============================================================================
sum by (label_team) (
  rate(container_cpu_usage_seconds_total{container!=""}[1h]) 
  * on (namespace, pod) group_left(label_team) 
  kube_pod_labels
) * on (node) group_left() 
node_hourly_cost

# =============================================================================
# 资源请求 vs 实际使用 (识别浪费)
# =============================================================================
# CPU 请求利用率
sum(rate(container_cpu_usage_seconds_total{container!=""}[5m])) by (namespace)
/
sum(kube_pod_container_resource_requests{resource="cpu"}) by (namespace)

# 内存请求利用率
sum(container_memory_usage_bytes{container!=""}) by (namespace)
/
sum(kube_pod_container_resource_requests{resource="memory"}) by (namespace)

# =============================================================================
# 闲置资源统计
# =============================================================================
# CPU 闲置量
(
  sum(kube_pod_container_resource_requests{resource="cpu"}) -
  sum(rate(container_cpu_usage_seconds_total{container!=""}[5m]))
) 
/ 
sum(kube_pod_container_resource_requests{resource="cpu"}) * 100

# 内存闲置量
(
  sum(kube_pod_container_resource_requests{resource="memory"}) -
  sum(container_memory_usage_bytes{container!=""})
) 
/ 
sum(kube_pod_container_resource_requests{resource="memory"}) * 100

# =============================================================================
# 节点利用率 (识别可缩容节点)
# =============================================================================
# CPU 利用率低于 50% 的节点
(
  sum(rate(container_cpu_usage_seconds_total{container!=""}[5m])) by (node)
  /
  sum(kube_node_status_allocatable{resource="cpu"}) by (node)
) < 0.5

# =============================================================================
# 跨 AZ 流量成本估算
# =============================================================================
sum(rate(container_network_transmit_bytes_total[5m])) by (namespace) 
* 0.01  # 假设跨 AZ 流量 $0.01/GB
```

### ResourceQuota 成本控制

```yaml
# resourcequota-cost-control.yaml

---
# 团队配额
apiVersion: v1
kind: ResourceQuota
metadata:
  name: team-backend-quota
  namespace: team-backend
spec:
  hard:
    # 计算资源
    requests.cpu: "100"
    requests.memory: 200Gi
    limits.cpu: "200"
    limits.memory: 400Gi
    
    # GPU 资源
    requests.nvidia.com/gpu: "4"
    
    # 存储资源
    persistentvolumeclaims: "50"
    requests.storage: 1Ti
    
    # 对象数量
    pods: "200"
    services: "50"
    secrets: "100"
    configmaps: "100"
    
    # 特定存储类配额
    gp3.storageclass.storage.k8s.io/requests.storage: 500Gi
    io2.storageclass.storage.k8s.io/requests.storage: 100Gi

---
# LimitRange 默认资源
apiVersion: v1
kind: LimitRange
metadata:
  name: team-backend-limits
  namespace: team-backend
spec:
  limits:
    # 容器默认值
    - type: Container
      default:
        cpu: 500m
        memory: 512Mi
      defaultRequest:
        cpu: 100m
        memory: 128Mi
      min:
        cpu: 50m
        memory: 64Mi
      max:
        cpu: 4
        memory: 8Gi
        
    # Pod 限制
    - type: Pod
      max:
        cpu: "16"
        memory: 32Gi
        
    # PVC 限制
    - type: PersistentVolumeClaim
      min:
        storage: 1Gi
      max:
        storage: 100Gi
```

## 成本告警规则

```yaml
# cost-alerting-rules.yaml
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: cost-alerts
  namespace: monitoring
spec:
  groups:
    # =================================================================
    # 成本阈值告警
    # =================================================================
    - name: cost.thresholds
      interval: 5m
      rules:
        - alert: DailyCostHigh
          expr: |
            sum(increase(kubecost_cluster_cost_total[24h])) > 1000
          for: 30m
          labels:
            severity: warning
          annotations:
            summary: "日成本超过 $1000"
            description: "过去 24 小时集群成本: ${{ $value | printf \"%.2f\" }}"
            
        - alert: NamespaceCostSpike
          expr: |
            (
              sum(increase(kubecost_namespace_cost_total[1h])) by (namespace)
              /
              avg_over_time(sum(increase(kubecost_namespace_cost_total[1h])) by (namespace)[24h:1h])
            ) > 2
          for: 15m
          labels:
            severity: warning
          annotations:
            summary: "命名空间 {{ $labels.namespace }} 成本激增"
            description: "成本比 24 小时平均值增长 {{ $value | printf \"%.1f\" }}x"
            
    # =================================================================
    # 资源效率告警
    # =================================================================
    - name: cost.efficiency
      interval: 5m
      rules:
        - alert: LowCPUUtilization
          expr: |
            (
              sum(rate(container_cpu_usage_seconds_total{container!=""}[1h])) by (namespace)
              /
              sum(kube_pod_container_resource_requests{resource="cpu"}) by (namespace)
            ) < 0.2
          for: 2h
          labels:
            severity: info
          annotations:
            summary: "命名空间 {{ $labels.namespace }} CPU 利用率低"
            description: "CPU 利用率仅 {{ $value | printf \"%.1f\" }}%,建议调整资源请求"
            
        - alert: LowMemoryUtilization
          expr: |
            (
              sum(container_memory_usage_bytes{container!=""}) by (namespace)
              /
              sum(kube_pod_container_resource_requests{resource="memory"}) by (namespace)
            ) < 0.3
          for: 2h
          labels:
            severity: info
          annotations:
            summary: "命名空间 {{ $labels.namespace }} 内存利用率低"
            description: "内存利用率仅 {{ $value | printf \"%.1f\" }}%"
            
        - alert: OverprovisionedResources
          expr: |
            (
              sum(kube_pod_container_resource_limits{resource="cpu"}) by (namespace)
              /
              sum(kube_pod_container_resource_requests{resource="cpu"}) by (namespace)
            ) > 3
          for: 1h
          labels:
            severity: info
          annotations:
            summary: "命名空间 {{ $labels.namespace }} 资源超额配置"
            description: "Limits/Requests 比率为 {{ $value | printf \"%.1f\" }}x"
            
    # =================================================================
    # 预算告警
    # =================================================================
    - name: cost.budget
      interval: 15m
      rules:
        - alert: MonthlyBudget80Percent
          expr: |
            (
              sum(kubecost_cluster_cost_total)
              /
              kubecost_budget_monthly_total
            ) > 0.8
          labels:
            severity: warning
          annotations:
            summary: "月度预算已使用 80%"
            
        - alert: MonthlyBudgetExceeded
          expr: |
            (
              sum(kubecost_cluster_cost_total)
              /
              kubecost_budget_monthly_total
            ) > 1.0
          labels:
            severity: critical
          annotations:
            summary: "月度预算已超支"
```

## 成本优化检查清单

### 优化策略矩阵

| 优化策略 | 节省比例 | 实现复杂度 | 适用场景 |
|---------|---------|-----------|---------|
| **抢占式/Spot 实例** | 50-90% | 低 | 可中断工作负载 |
| **预留实例** | 30-60% | 低 | 稳定基线负载 |
| **节点自动缩放** | 20-40% | 中 | 负载波动场景 |
| **VPA 资源调整** | 15-30% | 低 | 过度配置应用 |
| **节点池优化** | 10-30% | 中 | 多样化工作负载 |
| **Bin Packing** | 15-25% | 中 | 提高装箱率 |
| **存储分层** | 20-40% | 中 | 大量存储场景 |
| **网络优化** | 10-20% | 高 | 跨区域流量大 |

### 检查命令集

```bash
#!/bin/bash
# cost-optimization-checklist.sh
# 成本优化检查脚本

echo "=== Kubernetes 成本优化检查 ==="
echo ""

echo "1. 资源利用率检查"
echo "--- 无资源请求的 Pod ---"
kubectl get pods --all-namespaces -o json | jq -r '
  .items[] | 
  select(.spec.containers[].resources.requests == null) | 
  "\(.metadata.namespace)/\(.metadata.name)"'
echo ""

echo "2. 闲置 Pod 检查"
echo "--- CPU 使用率低于 10% 的 Pod ---"
kubectl top pods --all-namespaces --sort-by=cpu | head -20
echo ""

echo "3. 过期 PVC 检查"
echo "--- 未绑定的 PVC ---"
kubectl get pvc --all-namespaces --field-selector=status.phase!=Bound
echo ""

echo "4. 未使用 ConfigMap/Secret"
echo "--- 检查命令 ---"
echo "kubectl get configmaps --all-namespaces -o json | jq '.items[].metadata.name'"
echo ""

echo "5. 节点利用率"
kubectl top nodes
echo ""

echo "6. 跨 AZ Pod 分布"
kubectl get pods --all-namespaces -o wide | awk '{print $8}' | sort | uniq -c
echo ""

echo "=== 检查完成 ==="
```

## FinOps 成熟度模型

### 成熟度阶段

| 阶段 | 名称 | 核心能力 | 关键指标 | 目标 |
|-----|------|---------|---------|------|
| **Level 1** | Crawl (起步) | 成本可见性 | 能看到成本数据 | 知道花了多少钱 |
| **Level 2** | Walk (行走) | 成本分配 | 按团队/项目分配 | 知道谁花了钱 |
| **Level 3** | Run (奔跑) | 成本优化 | 主动优化措施 | 持续降低成本 |
| **Level 4** | Fly (飞翔) | 预测优化 | 预测性成本管理 | 前瞻性决策 |

### 云厂商特定功能

| 功能 | AWS EKS | Azure AKS | GCP GKE | 阿里云 ACK |
|-----|---------|-----------|---------|-----------|
| 成本分析 | Cost Explorer | Cost Management | Cloud Billing | 成本分析 |
| 资源建议 | Compute Optimizer | Advisor | Recommender | 资源画像 |
| Spot 节点 | Spot Instances | Spot VMs | Preemptible VMs | 抢占式实例 |
| 预留实例 | Reserved/Savings Plans | Reserved | CUDs | 预留实例 |
| 弹性配额 | Service Quotas | Quotas | Quotas | 弹性配额 |

## 版本变更记录

| 版本 | 变更内容 | 影响 |
|-----|---------|------|
| **Kubecost 2.0** | 新增 GPU 成本追踪 | 更精确的成本分析 |
| **OpenCost 1.0** | CNCF 毕业项目 | 开源标准化 |
| **v1.29** | VPA 增强 | 更好的资源建议 |
| **v1.28** | Karpenter GA | 更智能的节点管理 |

## 最佳实践总结

### 成本管理检查清单

- [ ] 部署成本监控工具 (Kubecost/OpenCost)
- [ ] 建立成本标签规范
- [ ] 配置 ResourceQuota 和 LimitRange
- [ ] 启用 VPA 资源建议
- [ ] 配置节点自动缩放
- [ ] 使用 Spot/抢占式实例
- [ ] 设置成本告警
- [ ] 定期进行成本审查

---

**参考资料**:
- [Kubecost 文档](https://docs.kubecost.com/)
- [OpenCost 项目](https://www.opencost.io/)
- [FinOps 基金会](https://www.finops.org/)
- [Kubernetes 成本优化](https://kubernetes.io/docs/concepts/cluster-administration/manage-deployment/)
