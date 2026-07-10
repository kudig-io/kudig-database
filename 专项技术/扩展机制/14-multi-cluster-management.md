---
title: 14 - 多集群管理与联邦 (Multi-Cluster Management & Federation)
description: '# 14 - 多集群管理与联邦 (Multi-Cluster Management & Federation)'
summary: 'cluster.x-k8s.io/cluster-name: production-cluster'
category: extensions
tags:
- k8s
- extensions
- crd
- operator
- webhook
- apiserver
- kubelet
- scheduler
- prometheus
- istio
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 开发工程师
- 架构师
estimated_read_time: 5min
intent_queries:
- 多集群管理与联邦 (Multi-Cluster Management & Federation) 是什么
- 如何 多集群管理与联邦 (Multi-Cluster Management & Federation)
- Kubernetes 10 extensions 最佳实践
trigger_keywords:
- 多集群管理与联邦
- Multi-Cluster
- Management
- Federation
- extensions
prerequisites:
- kubectl-basics
- helm-basics
- service-mesh-basics
- prometheus-basics
- gitops-basics
- cilium-basics
- cni-basics
- policy-basics
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
authors:
- name: KUDIG Team
  role: contributor
cross_refs:
- type: domain
  path: ../平台工程/
  label: '相关知识域: 平台工程'
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 14 - 多集群管理与联邦 (Multi-Cluster Management & Federation)

> **适用版本**: v1.25 - v1.32 | **最后更新**: 2026-02 | **参考**: [[entities/kubernetes.md|kubernetes]].io/docs/concepts/architecture/multicluster](https://kubernetes.io/docs/concepts/architecture/multicluster/)

<!-- chunk: 多集群架构模式 -->
## 多集群架构模式

### 集群管理模式对比

| 模式 | 适用场景 | 管理复杂度 | 数据同步 | 网络要求 |
|------|----------|------------|----------|----------|
| **独立集群** | 开发测试环境 | 低 | 无 | 独立网络 |
| **集群联邦** | 多地域部署 | 中 | 有限同步 | 跨区域网络 |
| **集群注册中心** | 统一管理 | 高 | 集中视图 | 网络可达 |
| **虚拟集群** | 租户隔离 | 中 | 完全隔离 | 共享底层 |

### 生产环境多集群拓扑

```
┌─────────────────────────────────────────────────────────────────────────┐
│                           多集群管理架构                                 │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                          │
│  ┌─────────────────┐    ┌─────────────────┐    ┌─────────────────┐     │
│  │  管理控制平面    │    │  注册中心集群    │    │  监控告警中心    │    │
│  │                 │    │                 │    │                 │    │
│  │ Cluster API     │◄──►│ Cluster Registry │◄──►│ Observability   │    │
│  │ ArgoCD          │    │ Fleet Manager   │    │ Central System  │    │
│  │ Rancher         │    │                 │    │                 │    │
│  └─────────────────┘    └─────────────────┘    └─────────────────┘     │
│           │                       │                       │              │
│           ▼                       ▼                       ▼              │
│  ┌─────────────────┐    ┌─────────────────┐    ┌─────────────────┐     │
│  │  开发集群        │    │  生产集群        │    │  灾备集群        │    │
│  │  dev-cluster     │    │  prod-cluster    │    │  dr-cluster      │    │
│  │                 │    │                 │    │                 │    │
│  │ Applications    │    │ Critical Apps   │    │ Backup Systems  │    │
│  │ CI/CD Pipeline  │    │ HA Services     │    │ DR Procedures   │    │
│  └─────────────────┘    └─────────────────┘    └─────────────────┘     │
│                                                                          │
└─────────────────────────────────────────────────────────────────────────┘
```

<!-- chunk: Cluster API 生产实践 -->
## Cluster API 生产实践

### 基础设施即代码配置

```yaml
# Cluster API 集群定义
apiVersion: cluster.x-k8s.io/v1beta1
kind: Cluster
metadata:
  name: production-cluster
  namespace: capi-system
spec:
  clusterNetwork:
    pods:
      cidrBlocks: ["192.168.0.0/16"]
    services:
      cidrBlocks: ["10.96.0.0/12"]
  infrastructureRef:
    apiVersion: infrastructure.cluster.x-k8s.io/v1beta1
    kind: AWSCluster
    name: production-cluster
  controlPlaneRef:
    apiVersion: controlplane.cluster.x-k8s.io/v1beta1
    kind: KubeadmControlPlane
    name: production-control-plane

---
# AWS 基础设施配置
apiVersion: infrastructure.cluster.x-k8s.io/v1beta1
kind: AWSCluster
metadata:
  name: production-cluster
  namespace: capi-system
spec:
  region: us-west-2
  sshKeyName: production-key
  network:
    vpc:
      availabilityZoneUsageLimit: 3
      availabilityZoneSelection: Ordered
  bastion:
    enabled: true
    allowedCIDRBlocks:
    - 10.0.0.0/8

---
# 控制平面配置
apiVersion: controlplane.cluster.x-k8s.io/v1beta1
kind: KubeadmControlPlane
metadata:
  name: production-control-plane
  namespace: capi-system
spec:
  replicas: 3
  machineTemplate:
    infrastructureRef:
      apiVersion: infrastructure.cluster.x-k8s.io/v1beta1
      kind: AWSMachineTemplate
      name: production-control-plane-machines
  kubeadmConfigSpec:
    clusterConfiguration:
      apiServer:
        extraArgs:
          audit-log-path: /var/log/apiserver/audit.log
          audit-policy-file: /etc/kubernetes/policies/audit-policy.yaml
        extraVolumes:
        - name: audit-policies
          hostPath: /etc/kubernetes/policies
          mountPath: /etc/kubernetes/policies
          readOnly: true
      controllerManager:
        extraArgs:
          horizontal-pod-autoscaler-sync-period: 10s
      scheduler:
        extraArgs:
          profiling: "false"
    initConfiguration:
      nodeRegistration:
        kubeletExtraArgs:
          cloud-provider: aws
    joinConfiguration:
      nodeRegistration:
        kubeletExtraArgs:
          cloud-provider: aws
```

### 节点组管理

```yaml
# Worker 节点池配置
apiVersion: cluster.x-k8s.io/v1beta1
kind: MachineDeployment
metadata:
  name: production-workers
  namespace: capi-system
spec:
  clusterName: production-cluster
  replicas: 6
  selector:
    matchLabels:
      cluster.x-k8s.io/cluster-name: production-cluster
      pool: workers
  template:
    spec:
      clusterName: production-cluster
      version: v1.28.0
      bootstrap:
        configRef:
          apiVersion: bootstrap.cluster.x-k8s.io/v1beta1
          kind: KubeadmConfigTemplate
          name: production-worker-bootstrap
      infrastructureRef:
        apiVersion: infrastructure.cluster.x-k8s.io/v1beta1
        kind: AWSMachineTemplate
        name: production-worker-machines

---
# Worker 节点机器模板
apiVersion: infrastructure.cluster.x-k8s.io/v1beta1
kind: AWSMachineTemplate
metadata:
  name: production-worker-machines
  namespace: capi-system
spec:
  template:
    spec:
      instanceType: m5.xlarge
      ami:
        id: ami-0abcdef1234567890
      iamInstanceProfile: nodes.cluster-api-provider-aws.sigs.k8s.io
      rootVolume:
        size: 100
        type: gp3
      cloudInit:
        insecureSkipSecretsManager: true
      spotMarketOptions:
        maxPrice: "0.10"  # Spot实例价格上限

---
# 节点启动配置
apiVersion: bootstrap.cluster.x-k8s.io/v1beta1
kind: KubeadmConfigTemplate
metadata:
  name: production-worker-bootstrap
  namespace: capi-system
spec:
  template:
    spec:
      joinConfiguration:
        nodeRegistration:
          kubeletExtraArgs:
            cloud-provider: aws
            rotate-certificates: "true"
            streaming-connection-idle-timeout: "5m"
            max-pods: "110"
            register-with-taints: ""
      preKubeadmCommands:
      - hostname "{{ ds.meta_data.hostname }}"
      - echo "::1         ipv6-localhost ipv6-loopback" >/etc/hosts
      - echo "127.0.0.1   localhost" >>/etc/hosts
      postKubeadmCommands:
      - systemctl daemon-reload
      - systemctl enable kubelet
      - systemctl start kubelet
```

<!-- chunk: 多集群注册与管理 -->
## 多集群注册与管理

### Rancher 多集群管理

```yaml
# Rancher Server 高可用部署
apiVersion: apps/v1
kind: Deployment
metadata:
  name: rancher
  namespace: cattle-system
spec:
  replicas: 3
  selector:
    matchLabels:
      app: rancher
  template:
    metadata:
      labels:
        app: rancher
    spec:
      serviceAccountName: rancher
      containers:
      - name: rancher
        image: rancher/rancher:v2.7.5
        args:
        - --http-port=80
        - --https-port=443
        - --audit-log-path=/var/log/auditlog/rancher-api-audit.log
        - --audit-level=2
        - --audit-log-maxage=30
        - --audit-log-maxbackup=10
        - --audit-log-maxsize=100
        - --features=multi-cluster-management=true
        - --features=fleet=false
        ports:
        - containerPort: 80
        - containerPort: 443
        volumeMounts:
        - name: audit-log
          mountPath: /var/log/auditlog
        readinessProbe:
          httpGet:
            path: /healthz
            port: 80
          initialDelaySeconds: 60
          periodSeconds: 30
      volumes:
      - name: audit-log
        emptyDir: {}

---
# 集群导入配置
apiVersion: management.cattle.io/v3
kind: Cluster
metadata:
  name: imported-prod-cluster
spec:
  displayName: "Production Cluster"
  description: "Main production Kubernetes cluster"
  importedConfig:
    kubeConfigSecret: prod-cluster-kubeconfig
  clusterAgentDeploymentCustomization:
    overrideAffinity:
      nodeAffinity:
        requiredDuringSchedulingIgnoredDuringExecution:
          nodeSelectorTerms:
          - matchExpressions:
            - key: node-role.kubernetes.io/control-plane
              operator: In
              values:
              - "true"
  fleetWorkspaceName: prod-workspace
```

### Cluster Registry 配置

```yaml
# 集群注册中心
apiVersion: apps/v1
kind: Deployment
metadata:
  name: cluster-registry
  namespace: multicluster-system
spec:
  replicas: 2
  selector:
    matchLabels:
      app: cluster-registry
  template:
    metadata:
      labels:
        app: cluster-registry
    spec:
      containers:
      - name: registry-server
        image: k8s.gcr.io/cluster-registry:v0.1.0
        ports:
        - containerPort: 8080
        env:
        - name: CLUSTER_REGISTRY_CONFIG
          value: "/etc/cluster-registry/config.yaml"
        volumeMounts:
        - name: config-volume
          mountPath: /etc/cluster-registry
        livenessProbe:
          httpGet:
            path: /healthz
            port: 8080
          initialDelaySeconds: 30
          periodSeconds: 10
      volumes:
      - name: config-volume
        configMap:
          name: cluster-registry-config

---
# 集群元数据配置
apiVersion: v1
kind: ConfigMap
metadata:
  name: cluster-registry-config
  namespace: multicluster-system
data:
  config.yaml: |
    clusters:
    - name: production-cluster
      endpoint: https://k8s-prod.example.com:6443
      auth:
        type: serviceaccount
        secretName: prod-cluster-sa-token
      labels:
        environment: production
        region: us-west-2
        purpose: customer-facing
      resources:
        cpu: 128
        memory: 512Gi
        nodes: 24
      
    - name: staging-cluster
      endpoint: https://k8s-staging.example.com:6443
      auth:
        type: certificate
        secretName: staging-cluster-cert
      labels:
        environment: staging
        region: us-east-1
        purpose: testing
      resources:
        cpu: 64
        memory: 256Gi
        nodes: 12
```

<!-- chunk: 跨集群应用部署 -->
## 跨集群应用部署

### ArgoCD 多集群部署

```yaml
# 多集群应用配置
apiVersion: argoproj.io/v1alpha1
kind: Application
metadata:
  name: multi-cluster-app
  namespace: argocd
spec:
  project: default
  source:
    repoURL: https://github.com/example/microservices.git
    targetRevision: HEAD
    path: manifests/
  destination:
    server: https://kubernetes.default.svc
    namespace: production
  syncPolicy:
    automated:
      prune: true
      selfHeal: true
  ignoreDifferences:
  - group: apps
    kind: Deployment
    jsonPointers:
    - /spec/replicas
  
  # 多集群目标配置
  destinations:
  - name: production-cluster
    namespace: production
    server: https://k8s-prod.example.com:6443
  - name: staging-cluster
    namespace: staging
    server: https://k8s-staging.example.com:6443
  - name: dr-cluster
    namespace: disaster-recovery
    server: https://k8s-dr.example.com:6443

---
# 集群凭证管理
apiVersion: v1
kind: Secret
metadata:
  name: cluster-credentials
  namespace: argocd
  labels:
    argocd.argoproj.io/secret-type: cluster
type: Opaque
stringData:
  name: production-cluster
  server: https://k8s-prod.example.com:6443
  config: |
    {
      "bearerToken": "<token>",
      "tlsClientConfig": {
        "insecure": false,
        "caData": "<base64-encoded-ca-cert>"
      }
    }
```

### Fleet 多集群管理

```yaml
# Fleet Bundle 配置
apiVersion: fleet.cattle.io/v1alpha1
kind: Bundle
metadata:
  name: monitoring-stack
  namespace: fleet-default
spec:
  resources:
  - content: |
      apiVersion: apps/v1
      kind: Deployment
      metadata:
        name: prometheus
        namespace: monitoring
      spec:
        replicas: 2
        selector:
          matchLabels:
            app: prometheus
        template:
          metadata:
            labels:
              app: prometheus
          spec:
            containers:
            - name: prometheus
              image: prom/prometheus:v3.2.1
  targets:
  - clusterSelector:
      matchLabels:
        environment: production
    replicaCount: 3
    kustomize:
      patches:
      - patch: |-
          apiVersion: apps/v1
          kind: Deployment
          metadata:
            name: prometheus
          spec:
            template:
              spec:
                containers:
                - name: prometheus
                  resources:
                    requests:
                      memory: "2Gi"
                      cpu: "1"
                    limits:
                      memory: "4Gi"
                      cpu: "2"
  - clusterSelector:
      matchLabels:
        environment: staging
    replicaCount: 1
    kustomize:
      patches:
      - patch: |-
          apiVersion: apps/v1
          kind: Deployment
          metadata:
            name: prometheus
          spec:
            template:
              spec:
                containers:
                - name: prometheus
                  resources:
                    requests:
                      memory: "1Gi"
                      cpu: "500m"
                    limits:
                      memory: "2Gi"
                      cpu: "1"
```

<!-- chunk: 集群间通信与服务发现 -->
## 集群间通信与服务发现

### 多集群服务网格

```yaml
# Istio 多集群配置
apiVersion: install.istio.io/v1alpha1
kind: IstioOperator
metadata:
  name: istio-multicluster
spec:
  profile: demo
  values:
    global:
      multiCluster:
        clusterName: production-cluster
      meshID: mesh1
      network: network1
    gateways:
      istio-ingressgateway:
        type: LoadBalancer
  components:
    ingressGateways:
    - name: istio-ingressgateway
      enabled: true
      k8s:
        service:
          ports:
          - port: 80
            targetPort: 8080
            name: http2
          - port: 443
            targetPort: 8443
            name: https

---
# 服务导出配置
apiVersion: networking.istio.io/v1beta1
kind: ServiceEntry
metadata:
  name: remote-services
  namespace: istio-system
spec:
  hosts:
  - "*.remote-cluster.example.com"
  location: MESH_EXTERNAL
  ports:
  - number: 80
    name: http
    protocol: HTTP
  resolution: DNS
  endpoints:
  - address: remote-cluster-gateway.example.com
    ports:
      http: 80
```

### 跨集群DNS配置

```yaml
# CoreDNS 跨集群配置
apiVersion: v1
kind: ConfigMap
metadata:
  name: coredns
  namespace: kube-system
data:
  Corefile: |
    .:53 {
        errors
        health {
           lameduck 5s
        }
        ready
        kubernetes cluster.local in-addr.arpa ip6.arpa {
           pods insecure
           fallthrough in-addr.arpa ip6.arpa
           ttl 30
        }
        prometheus :9153
        forward . /etc/resolv.conf {
           max_concurrent 1000
        }
        cache 30
        loop
        reload
        loadbalance
    }
    
    # 跨集群DNS转发
    remote-cluster.example.com:53 {
        forward . 10.100.10.10 10.100.20.10 {
            health_check 5s
        }
        cache 30
    }
```

<!-- chunk: 监控与告警统一 -->
## 监控与告警统一

### 多集群Prometheus配置

```yaml
# Prometheus 联邦配置
apiVersion: monitoring.coreos.com/v1
kind: Prometheus
metadata:
  name: federated-prometheus
  namespace: monitoring
spec:
  replicas: 2
  serviceAccountName: prometheus
  serviceMonitorSelector:
    matchLabels:
      team: frontend
  ruleSelector:
    matchLabels:
      team: frontend
  externalLabels:
    cluster: production-cluster
    region: us-west-2
  remoteWrite:
  - url: http://central-prometheus.monitoring.svc:9090/api/v1/write
    writeRelabelConfigs:
    - sourceLabels: [__name__]
      regex: (up|scrape_samples_scraped)
      action: keep
  additionalScrapeConfigs:
    name: additional-scrape-configs
    key: prometheus-additional.yaml

---
# 额外抓取配置
apiVersion: v1
kind: Secret
metadata:
  name: additional-scrape-configs
  namespace: monitoring
stringData:
  prometheus-additional.yaml: |
    - job_name: 'federate'
      scrape_interval: 15s
      honor_labels: true
      metrics_path: '/federate'
      params:
        'match[]':
          - '{job=~"kubernetes-.*"}'
          - '{__name__=~"node_.*"}'
      static_configs:
      - targets:
        - 'prometheus-us-east.monitoring.svc:9090'
        - 'prometheus-eu-west.monitoring.svc:9090'
        labels:
          cluster: remote-clusters
```

<!-- chunk: 安全与访问控制 -->
## 安全与访问控制

### 多集群RBAC管理

```yaml
# 集群间RBAC同步
apiVersion: rbac.authorization.k8s.io/v1
kind: ClusterRole
metadata:
  name: multicluster-admin
rules:
- apiGroups: [""]
  resources: ["pods", "services", "namespaces"]
  verbs: ["get", "list", "watch", "create", "update", "patch", "delete"]
- apiGroups: ["apps"]
  resources: ["deployments", "statefulsets"]
  verbs: ["get", "list", "watch", "create", "update", "patch", "delete"]
- apiGroups: ["networking.k8s.io"]
  resources: ["networkpolicies", "ingresses"]
  verbs: ["get", "list", "watch", "create", "update", "patch", "delete"]

---
# 跨集群角色绑定
apiVersion: rbac.authorization.k8s.io/v1
kind: ClusterRoleBinding
metadata:
  name: multicluster-admin-binding
roleRef:
  apiGroup: rbac.authorization.k8s.io
  kind: ClusterRole
  name: multicluster-admin
subjects:
- kind: User
  name: admin-user
  apiGroup: rbac.authorization.k8s.io
- kind: Group
  name: cluster-admins
  apiGroup: rbac.authorization.k8s.io
```

<!-- chunk: 问题排除与调试 -->
## 问题排除与调试

### 多集群诊断工具

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
#!/bin/bash
# multicluster-diagnostics.sh

CLUSTERS=("production-cluster" "staging-cluster" "dr-cluster")

diagnose_cluster_connectivity() {
    echo "=== 集群连接性诊断 ==="
    for cluster in "${CLUSTERS[@]}"; do
        echo "检查集群: $cluster"
        kubectl config use-context $cluster
        
        # 检查API Server可达性
        if kubectl cluster-info >/dev/null 2>&1; then
            echo "✅ $cluster API Server 可达"
        else
            echo "❌ $cluster API Server 不可达"
        fi
        
        # 检查节点状态
        ready_nodes=$(kubectl get nodes --no-headers | grep -c " Ready ")
        total_nodes=$(kubectl get nodes --no-headers | wc -l)
        echo "📊 $cluster 节点状态: $ready_nodes/$total_nodes 就绪"
    done
}

diagnose_cross_cluster_services() {
    echo "=== 跨集群服务诊断 ==="
    # 检查服务发现
    for cluster in "${CLUSTERS[@]}"; do
        echo "检查 $cluster 中的服务..."
        kubectl config use-context $cluster
        kubectl get svc --all-namespaces | grep -E "(LoadBalancer|ClusterIP)" | head -5
    done
}

diagnose_network_connectivity() {
    echo "=== 网络连通性诊断 ==="
    # 检查Pod间通信
    for cluster in "${CLUSTERS[@]}"; do
        echo "检查 $cluster 网络连通性..."
        kubectl config use-context $cluster
        kubectl run debug-pod --image=busybox --restart=Never --rm -it -- sh -c "
            ping -c 3 8.8.8.8
            nslookup kubernetes.default
        " 2>/dev/null || echo "网络测试失败"
    done
}

# 执行诊断
diagnose_cluster_connectivity
diagnose_cross_cluster_services
diagnose_network_connectivity

echo "=== 诊断完成 ==="
```
<!-- chunk: 生产环境最佳实践 -->
## 生产环境最佳实践

### 集群命名规范

```yaml
# 集群命名约定
clusters:
  # 环境-用途-区域-序号
  production-customer-us-west-01:  # 生产客户集群-美国西部-01
    purpose: customer-facing
    sla: 99.99%
    
  staging-testing-us-east-01:      # 预发布测试集群-美国东部-01
    purpose: testing
    sla: 99.9%
    
  development-dev-us-west-01:      # 开发环境集群-美国西部-01
    purpose: development
    sla: 99%
```

### 版本管理策略

```yaml
# 集群版本升级计划
version_management:
  upgrade_schedule:
    - time: "2024-02-15T02:00:00Z"
      clusters: ["staging-cluster"]
      target_version: "v1.28.2"
      
    - time: "2024-02-22T02:00:00Z"
      clusters: ["production-cluster"]
      target_version: "v1.28.2"
      
  compatibility_matrix:
    kubernetes_versions: ["1.26", "1.27", "1.28"]
    supported_cnis: ["calico", "cilium", "flannel"]
    certified_platforms: ["aws", "gcp", "azure"]
```

---

**表格底部标记**: Kusheet Project, 作者 Allen Galler (allengaller@gmail.com)

---

<!-- chunk: Obsidian 相关文档 -->
## Obsidian 相关文档

- 专项技术 KUDIG Database — Global MOC
- [[专项技术/README.md|Domain-10: Kubernetes 扩展生态]]
- index.md|Domain-10 扩展与自定义 — 开源项目索引]]
- CRD 自定义资源定义开发指南
- 02 - Operator开发模式与控制器实现
- 03 - 准入控制器(Webhook)配置与实现
- Kubernetes API 聚合扩展机制详解
- 包管理与应用分发工具
- 47 - Helm Chart开发与管理
- 129 - Helm 高级运维：复杂部署、CI/CD 集成与安全最佳实践
- CI/CD 管道
- 48 - GitOps工作流

## See Also

- 12-service-mesh-advanced
- 13-kubernetes-operations-fundamentals
- 15-monitoring-alerting-system
- 16-security-compliance-management


<!-- risk-assessed -->
