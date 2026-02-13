# 02-多云混合部署策略

> **适用范围**: Kubernetes v1.25-v1.32 | **维护状态**: 🔧 持续更新中 | **专家级别**: ⭐⭐⭐⭐⭐

## 📋 概述

多云混合部署是现代企业IT战略的重要组成部分。本文档详细介绍如何在多个云平台和本地环境中实现Kubernetes集群的统一管理和容灾部署。

## ☁️ 多云架构模式

### 多云部署拓扑

#### 1. 主备模式 (Active-Passive)
```yaml
# 主集群配置
apiVersion: cluster.x-k8s.io/v1beta1
kind: Cluster
metadata:
  name: primary-cluster
  labels:
    cluster-type: primary
spec:
  controlPlaneRef:
    apiVersion: controlplane.cluster.x-k8s.io/v1beta1
    kind: KubeadmControlPlane
    name: primary-control-plane
---
# 备用集群配置
apiVersion: cluster.x-k8s.io/v1beta1
kind: Cluster
metadata:
  name: standby-cluster
  labels:
    cluster-type: standby
spec:
  controlPlaneRef:
    apiVersion: controlplane.cluster.x-k8s.io/v1beta1
    kind: KubeadmControlPlane
    name: standby-control-plane
```

#### 2. 主主模式 (Active-Active)
```yaml
# 多区域负载均衡配置
apiVersion: v1
kind: Service
metadata:
  name: global-lb
  annotations:
    service.beta.kubernetes.io/aws-load-balancer-type: "nlb"
    service.beta.kubernetes.io/aws-load-balancer-cross-zone-load-balancing-enabled: "true"
spec:
  type: LoadBalancer
  ports:
  - port: 80
    targetPort: 8080
  selector:
    app: global-app
```

### 跨云网络互联

#### 1. VPN网关配置
```bash
# AWS VPN连接配置
aws ec2 create-vpn-connection \
  --type ipsec.1 \
  --customer-gateway-id cgw-12345678 \
  --vpn-gateway-id vgw-87654321 \
  --options TunnelInsideCidrList=["169.254.10.0/30","169.254.11.0/30"]
```

#### 2. 专线连接设置
```yaml
# Google Cloud Interconnect配置
apiVersion: compute.cnrm.cloud.google.com/v1beta1
kind: ComputeInterconnectAttachment
metadata:
  name: interconnect-attachment
spec:
  routerRef:
    name: cloud-router
  region: us-central1
  type: DEDICATED
  bandwidth: BPS_10G
  candidateSubnets:
  - 169.254.100.0/29
```

## 🔄 数据同步策略

### 容器镜像同步

#### 1. Harbor多实例同步
```yaml
# Harbor复制规则配置
apiVersion: goharbor.io/v1alpha1
kind: ReplicationPolicy
metadata:
  name: cross-cloud-sync
spec:
  srcRegistry:
    name: harbor-primary
  destRegistry:
    name: harbor-secondary
  trigger:
    type: scheduled
    cron: "0 2 * * *"
  filters:
  - name: "prod-*"
    type: name
  destNamespace: production
  override: true
  speed: 0
  copyByChunk: false
```

#### 2. 镜像仓库缓存策略
```yaml
# Registry缓存配置
apiVersion: apps/v1
kind: Deployment
metadata:
  name: registry-cache
spec:
  replicas: 3
  template:
    spec:
      containers:
      - name: registry
        image: registry:2.8
        env:
        - name: REGISTRY_PROXY_REMOTEURL
          value: https://registry-1.docker.io
        - name: REGISTRY_STORAGE_DELETE_ENABLED
          value: "true"
        volumeMounts:
        - name: cache-volume
          mountPath: /var/lib/registry
```

### 配置数据同步

#### 1. GitOps多集群同步
```yaml
# ArgoCD ApplicationSet配置
apiVersion: argoproj.io/v1alpha1
kind: ApplicationSet
metadata:
  name: multi-cluster-apps
spec:
  generators:
  - clusters:
      selector:
        matchLabels:
          environment: production
  template:
    metadata:
      name: '{{name}}-app'
    spec:
      project: default
      source:
        repoURL: https://github.com/org/apps.git
        targetRevision: HEAD
        path: charts/app
      destination:
        server: '{{server}}'
        namespace: app-namespace
```

#### 2. Secret跨集群同步
```yaml
# Sealed Secrets配置
apiVersion: bitnami.com/v1alpha1
kind: SealedSecret
metadata:
  name: database-credentials
  namespace: production
spec:
  encryptedData:
    username: AgB2...
    password: AgC3...
  template:
    metadata:
      name: database-credentials
      namespace: production
    type: Opaque
```

## 🛡️ 安全管控策略

### 统一身份认证

#### 1. OIDC多云集成
```yaml
# Dex配置支持多云提供商
apiVersion: apps/v1
kind: Deployment
metadata:
  name: dex
spec:
  template:
    spec:
      containers:
      - name: dex
        image: dexidp/dex:v2.35.0
        args:
        - dex
        - serve
        - --web-http-addr=:5556
        - --telemetry-addr=:5558
        - --config=/etc/dex/config.yaml
        volumeMounts:
        - name: config
          mountPath: /etc/dex
      volumes:
      - name: config
        configMap:
          name: dex-config
---
apiVersion: v1
kind: ConfigMap
metadata:
  name: dex-config
data:
  config.yaml: |
    issuer: https://dex.example.com
    storage:
      type: kubernetes
      config:
        inCluster: true
    web:
      http: 0.0.0.0:5556
    connectors:
    - type: oidc
      id: aws
      name: AWS SSO
      config:
        issuer: https://oidc.eks.us-west-2.amazonaws.com/id/ABC123
        clientID: $AWS_CLIENT_ID
        clientSecret: $AWS_CLIENT_SECRET
        redirectURI: https://dex.example.com/callback
    - type: oidc
      id: gcp
      name: Google Cloud
      config:
        issuer: https://accounts.google.com
        clientID: $GCP_CLIENT_ID
        clientSecret: $GCP_CLIENT_SECRET
        redirectURI: https://dex.example.com/callback
```

#### 2. 统一RBAC策略
```yaml
# 跨集群RBAC同步
apiVersion: rbac.authorization.k8s.io/v1
kind: ClusterRole
metadata:
  name: cross-cluster-admin
rules:
- apiGroups: ["*"]
  resources: ["*"]
  verbs: ["*"]
---
apiVersion: rbac.authorization.k8s.io/v1
kind: ClusterRoleBinding
metadata:
  name: cross-cluster-admin-binding
subjects:
- kind: Group
  name: sso:admins
  apiGroup: rbac.authorization.k8s.io
roleRef:
  kind: ClusterRole
  name: cross-cluster-admin
  apiGroup: rbac.authorization.k8s.io
```

### 网络安全策略

#### 1. 统一网络策略
```yaml
# 全局网络策略模板
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: global-default-deny
spec:
  podSelector: {}
  policyTypes:
  - Ingress
  - Egress
---
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: allow-monitoring
spec:
  podSelector:
    matchLabels:
      app: monitoring
  ingress:
  - from:
    - namespaceSelector:
        matchLabels:
          name: monitoring
    ports:
    - protocol: TCP
      port: 9090
```

#### 2. 服务网格安全
```yaml
# Istio多集群安全配置
apiVersion: security.istio.io/v1beta1
kind: AuthorizationPolicy
metadata:
  name: cross-cluster-policy
  namespace: istio-system
spec:
  action: ALLOW
  rules:
  - from:
    - source:
        principals: ["cluster.local/ns/istio-system/sa/istio-reader-service-account"]
    to:
    - operation:
        ports: ["15012", "15017"]
```

## 💰 成本优化策略

### 资源调度优化

#### 1. 跨云Spot实例利用
```yaml
# Spot实例节点组配置
apiVersion: eksctl.io/v1alpha5
kind: ClusterConfig
metadata:
  name: spot-cluster
  region: us-west-2
managedNodeGroups:
- name: spot-ng
  instanceTypes: ["m5.large", "m5.xlarge", "m5.2xlarge"]
  spot: true
  desiredCapacity: 10
  minSize: 5
  maxSize: 20
  labels:
    lifecycle: spot
  taints:
  - key: spot
    value: "true"
    effect: NoSchedule
```

#### 2. 混合实例类型调度
```yaml
# 混合实例调度器配置
apiVersion: apps/v1
kind: Deployment
metadata:
  name: cost-optimized-app
spec:
  template:
    spec:
      affinity:
        nodeAffinity:
          preferredDuringSchedulingIgnoredDuringExecution:
          - weight: 100
            preference:
              matchExpressions:
              - key: node.kubernetes.io/instance-type
                operator: In
                values: ["t3.medium", "t3.large"]
          - weight: 50
            preference:
              matchExpressions:
              - key: lifecycle
                operator: In
                values: ["spot"]
```

### 成本监控告警

#### 1. 多云成本聚合
```yaml
# Prometheus多云成本指标
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: cross-cloud-cost-alerts
spec:
  groups:
  - name: cost.rules
    rules:
    - alert: HighCloudCost
      expr: sum by(cloud_provider) (rate(kube_node_status_capacity_cpu_cores[1h]) * 0.1) > 1000
      for: 15m
      labels:
        severity: warning
      annotations:
        summary: "High cloud computing costs detected"
        description: "{{ $labels.cloud_provider }} costs exceeded threshold"
```

#### 2. 预算管理策略
```yaml
# Kubernetes资源配额与预算联动
apiVersion: v1
kind: ResourceQuota
metadata:
  name: monthly-budget-quota
spec:
  hard:
    requests.cpu: "100"
    requests.memory: 200Gi
    limits.cpu: "200"
    limits.memory: 400Gi
    persistentvolumeclaims: "50"
  scopeSelector:
    matchExpressions:
    - scopeName: PriorityClass
      operator: In
      values: ["high-cost", "medium-cost"]
```

## 🎯 故障切换策略

### 自动故障检测

#### 1. 集群健康检查
```yaml
# 集群健康探针配置
apiVersion: v1
kind: ConfigMap
metadata:
  name: cluster-health-check
data:
  health-check.sh: |
    #!/bin/bash
    set -e
    
    # 检查API Server可用性
    kubectl get nodes --request-timeout=5s >/dev/null 2>&1 || exit 1
    
    # 检查核心组件状态
    kubectl get pods -n kube-system -l tier=control-plane --no-headers | \
      grep -v Running && exit 1
    
    # 检查节点就绪状态
    kubectl get nodes --no-headers | grep -v Ready && exit 1
    
    echo "Cluster is healthy"
```

#### 2. 应用健康监控
```yaml
# 应用级健康检查配置
apiVersion: apps/v1
kind: Deployment
metadata:
  name: health-monitored-app
spec:
  replicas: 3
  template:
    spec:
      containers:
      - name: app
        image: myapp:latest
        readinessProbe:
          httpGet:
            path: /healthz
            port: 8080
          initialDelaySeconds: 30
          periodSeconds: 10
        livenessProbe:
          httpGet:
            path: /healthz
            port: 8080
          initialDelaySeconds: 60
          periodSeconds: 30
```

### 手动/自动切换

#### 1. DNS故障切换
```yaml
# ExternalDNS配置支持多集群
apiVersion: externaldns.k8s.io/v1alpha1
kind: DNSEndpoint
metadata:
  name: multi-cluster-dns
spec:
  endpoints:
  - dnsName: app.example.com
    recordTTL: 300
    recordType: A
    targets:
    - 203.0.113.1  # Primary cluster IP
    - 203.0.113.2  # Secondary cluster IP
    providerSpecific:
    - name: weight
      value: "100"   # Primary weight
    - name: weight
      value: "0"     # Secondary weight (standby)
```

#### 2. 流量切换脚本
```bash
#!/bin/bash
# 多集群流量切换脚本

PRIMARY_CLUSTER="https://primary-api.example.com"
STANDBY_CLUSTER="https://standby-api.example.com"
SERVICE_NAME="my-service"

switch_to_standby() {
    echo "Switching traffic to standby cluster..."
    
    # 更新DNS权重
    kubectl patch dnsendpoint multi-cluster-dns \
      -p '{"spec":{"endpoints":[{"dnsName":"app.example.com","recordTTL":300,"recordType":"A","targets":["203.0.113.1","203.0.113.2"],"providerSpecific":[{"name":"weight","value":"0"},{"name":"weight","value":"100"}]}]}}' \
      --type=merge
    
    # 验证切换
    sleep 30
    curl -f https://app.example.com/health || {
        echo "Health check failed after switch!"
        exit 1
    }
    
    echo "Traffic switched successfully to standby cluster"
}

# 使用示例
# switch_to_standby
```

## 📊 监控与可观测性

### 统一监控面板

#### 1. 多集群Prometheus联邦
```yaml
# Prometheus联邦配置
global:
  scrape_interval: 15s
  evaluation_interval: 15s

rule_files:
  - "rules/*.yml"

scrape_configs:
  - job_name: 'federate'
    scrape_interval: 15s
    honor_labels: true
    metrics_path: '/federate'
    params:
      'match[]':
        - '{job=~"kubernetes-.*"}'
        - '{__name__=~"job:.*"}'
    static_configs:
      - targets:
        - 'prometheus-primary:9090'
        - 'prometheus-secondary:9090'
        - 'prometheus-tertiary:9090'
```

#### 2. 跨集群Grafana仪表板
```json
{
  "dashboard": {
    "title": "Multi-Cluster Overview",
    "panels": [
      {
        "title": "Cluster Status Summary",
        "type": "stat",
        "targets": [
          {
            "expr": "count by(cluster) (up{job=\"kubernetes-nodes\"})",
            "legendFormat": "{{cluster}}"
          }
        ]
      },
      {
        "title": "Cross-Cluster Latency",
        "type": "graph",
        "targets": [
          {
            "expr": "histogram_quantile(0.95, rate(http_request_duration_seconds_bucket[5m]))",
            "legendFormat": "{{cluster}} - p95"
          }
        ]
      }
    ]
  }
}
```

### 日志集中收集

#### 1. 多云日志架构
```yaml
# Fluentd多集群日志收集
apiVersion: v1
kind: ConfigMap
metadata:
  name: fluentd-config
data:
  fluent.conf: |
    <source>
      @type forward
      port 24224
      bind 0.0.0.0
    </source>
    
    <filter kubernetes.**>
      @type kubernetes_metadata
      tag_to_kubernetes_name_regexp (?<pod_name>[a-z0-9]([-a-z0-9]*[a-z0-9])?(\.[a-z0-9]([-a-z0-9]*[a-z0-9])?)*)_(?<namespace>[^_]+)_(?<container_name>.+)-(?<docker_id>[a-z0-9]{64})
    </filter>
    
    <match **>
      @type elasticsearch
      host elasticsearch.logging.svc
      port 9200
      logstash_format true
      logstash_prefix ${record['kubernetes']['namespace']}-${ENV['CLUSTER_NAME']}
      include_tag_key true
      tag_key cluster
      flush_interval 10s
    </match>
```

## 🔧 实施最佳实践

### 部署前检查清单
- [ ] 确定业务连续性要求(RTO/RPO)
- [ ] 选择合适的多云架构模式
- [ ] 设计网络连通性和安全策略
- [ ] 制定数据同步和备份策略
- [ ] 建立统一的身份认证体系
- [ ] 配置监控告警和日志收集
- [ ] 制定故障切换和恢复流程

### 运营维护要点
- [ ] 定期进行故障切换演练
- [ ] 监控各云平台的成本变化
- [ ] 保持各集群版本同步
- [ ] 更新安全策略和访问控制
- [ ] 优化资源配置和调度策略
- [ ] 维护架构文档和操作手册

---

*本文档提供多云混合部署的全面指导，帮助企业构建高可用、安全的分布式Kubernetes架构*