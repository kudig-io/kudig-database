# 03 - 多集群网络联邦与跨集群通信

> **适用版本**: Kubernetes v1.25 - v1.32 | **最后更新**: 2026-02 | **难度**: 专家级

---

## 目录

1. [多集群网络架构模式](#1-多集群网络架构模式)
2. [Cluster Federation 核心组件](#2-cluster-federation-核心组件)
3. [跨集群服务发现](#3-跨集群服务发现)
4. [网络连通性实现](#4-网络连通性实现)
5. [安全策略管理](#5-安全策略管理)
6. [流量治理与路由](#6-流量治理与路由)
7. [监控与故障排查](#7-监控与故障排查)
8. [生产环境最佳实践](#8-生产环境最佳实践)

---

## 1. 多集群网络架构模式

### 1.1 架构模式对比

| 模式 | 适用场景 | 优势 | 劣势 | 复杂度 |
|------|----------|------|------|--------|
| **扁平网络** | 同一云厂商、同区域 | 配置简单、性能最优 | 网络隔离差、安全风险高 | 低 |
| **VPC对等连接** | 同云厂商、跨区域 | 原生性能、成本较低 | 配置复杂、管理困难 | 中 |
| **VPN隧道** | 混合云、跨云厂商 | 安全性强、兼容性好 | 性能损耗、延迟较高 | 高 |
| **专用线路** | 金融、政企客户 | 性能最优、安全最高 | 成本极高、部署周期长 | 高 |

### 1.2 扁平网络架构

```yaml
# 扁平网络配置示例
apiVersion: v1
kind: ConfigMap
metadata:
  name: flat-network-config
  namespace: kube-system
data:
  network-config: |
    # Pod CIDR 规划
    podCIDR: "10.244.0.0/16"
    serviceCIDR: "10.96.0.0/12"
    
    # 集群间 CIDR 分配
    cluster1:
      podCIDR: "10.244.0.0/18"      # 10.244.0.0 - 10.244.63.255
      serviceCIDR: "10.96.0.0/14"   # 10.96.0.0 - 10.99.255.255
      
    cluster2:
      podCIDR: "10.244.64.0/18"     # 10.244.64.0 - 10.244.127.255
      serviceCIDR: "10.100.0.0/14"  # 10.100.0.0 - 10.103.255.255
      
    cluster3:
      podCIDR: "10.244.128.0/18"    # 10.244.128.0 - 10.244.191.255
      serviceCIDR: "10.104.0.0/14"  # 10.104.0.0 - 10.107.255.255
```

### 1.3 VPC对等连接架构

```yaml
# AWS VPC Peering 配置
apiVersion: v1
kind: ConfigMap
metadata:
  name: vpc-peering-config
  namespace: network-operator
data:
  aws-vpc-peering.yaml: |
    # 集群1 VPC配置
    cluster1-vpc:
      vpc-id: vpc-xxxxxxxxx
      cidr-block: 10.0.0.0/16
      subnets:
        - az: us-west-2a
          cidr: 10.0.1.0/24
        - az: us-west-2b
          cidr: 10.0.2.0/24
    
    # 集群2 VPC配置
    cluster2-vpc:
      vpc-id: vpc-yyyyyyyyy
      cidr-block: 10.1.0.0/16
      subnets:
        - az: us-east-1a
          cidr: 10.1.1.0/24
        - az: us-east-1b
          cidr: 10.1.2.0/24
    
    # 对等连接配置
    peering-connections:
      - name: cluster1-to-cluster2
        requester-vpc: vpc-xxxxxxxxx
        accepter-vpc: vpc-yyyyyyyyy
        auto-accept: true
        route-tables:
          - vpc-xxxxxxxxx-rtb-private
          - vpc-yyyyyyyyy-rtb-private
```

---

## 2. 跨集群联邦方案 (Karmada & KubeFed)

### 2.1 Karmada 现代联邦架构

**Karmada** 已成为 CNCF 毕业项目，是多集群资源分发和治理的事实标准，取代了早期的 KubeFed。

```yaml
# Karmada 资源分发策略 (PropagationPolicy)
apiVersion: policy.karmada.io/v1alpha1
kind: PropagationPolicy
metadata:
  name: nginx-propagation
spec:
  resourceSelectors:
    - apiVersion: apps/v1
      kind: Deployment
      name: nginx
  placement:
    clusterAffinity:
      clusterNames:
        - cluster-east
        - cluster-west
    replicaScheduling:
      replicaSchedulingType: Divided
      replicaDivisionPreference: Weighted
      weightPreference:
        staticWeightList:
          - targetCluster:
              clusterNames:
                - cluster-east
            weight: 2
          - targetCluster:
              clusterNames:
                - cluster-west
            weight: 1
```

### 2.2 多集群服务 API (MCS)

MCS (Multi-cluster Service) 是 Kubernetes 官方定义的标准 API，用于跨集群服务导出和导入。

```yaml
# 1. 导出服务 (在源集群)
apiVersion: multicluster.x-k8s.io/v1alpha1
kind: ServiceExport
metadata:
  name: web-service
  namespace: production

---
# 2. 导入服务 (在消费集群 - 由控制器自动生成)
apiVersion: multicluster.x-k8s.io/v1alpha1
kind: ServiceImport
metadata:
  name: web-service
  namespace: production
spec:
  ips: ["10.100.1.1"]
  type: ClusterSetIP
  ports:
  - name: http
    port: 80
    protocol: TCP
```,old_str:

### 2.2 成员集群注册

```yaml
# 成员集群配置
apiVersion: core.kubefed.io/v1beta1
kind: KubeFedCluster
metadata:
  name: cluster-east
  namespace: kubefed-system
spec:
  apiEndpoint: https://cluster-east-api.example.com:6443
  caBundle: <base64-encoded-ca-cert>
  secretRef:
    name: cluster-east-secret
  disabledTLSValidations:
  - SubjectAltName

---
apiVersion: core.kubefed.io/v1beta1
kind: KubeFedCluster
metadata:
  name: cluster-west
  namespace: kubefed-system
spec:
  apiEndpoint: https://cluster-west-api.example.com:6443
  caBundle: <base64-encoded-ca-cert>
  secretRef:
    name: cluster-west-secret
```

### 2.3 联邦资源配置

```yaml
# 联邦化 Deployment
apiVersion: types.kubefed.io/v1beta1
kind: FederatedDeployment
metadata:
  name: federated-nginx
  namespace: default
spec:
  template:
    metadata:
      labels:
        app: nginx
    spec:
      replicas: 3
      selector:
        matchLabels:
          app: nginx
      template:
        metadata:
          labels:
            app: nginx
        spec:
          containers:
          - name: nginx
            image: nginx:1.21
            ports:
            - containerPort: 80
  placement:
    clusters:
    - name: cluster-east
    - name: cluster-west
  overrides:
  - clusterName: cluster-east
    clusterOverrides:
    - path: "/spec/replicas"
      value: 5
  - clusterName: cluster-west
    clusterOverrides:
    - path: "/spec/replicas"
      value: 2
```

---

## 3. 跨集群服务发现

### 3.1 DNS 联邦服务发现

```yaml
# 联邦服务配置
apiVersion: types.kubefed.io/v1beta1
kind: FederatedService
metadata:
  name: federated-web-service
  namespace: default
spec:
  template:
    spec:
      selector:
        app: web
      ports:
      - name: http
        port: 80
        targetPort: 8080
      type: LoadBalancer
  placement:
    clusters:
    - name: cluster-east
    - name: cluster-west
  overrides:
  - clusterName: cluster-east
    clusterOverrides:
    - path: "/spec/type"
      value: NodePort
  - clusterName: cluster-west
    clusterOverrides:
    - path: "/spec/type"
      value: LoadBalancer

---
# 跨集群 DNS 配置
apiVersion: v1
kind: ConfigMap
metadata:
  name: coredns-federation
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
        federation cluster.local {
            east us-east-1.cluster.example.com
            west us-west-1.cluster.example.com
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
```

### 3.2 服务网格跨集群发现

```yaml
# Istio 多集群服务发现
apiVersion: networking.istio.io/v1beta1
kind: ServiceEntry
metadata:
  name: cross-cluster-service
  namespace: istio-system
spec:
  hosts:
  - web.default.global
  location: MESH_INTERNAL
  ports:
  - name: http
    number: 80
    protocol: HTTP
  resolution: DNS
  endpoints:
  - address: web.default.svc.cluster-east.local
    locality: us-east-1
    ports:
      http: 80
  - address: web.default.svc.cluster-west.local
    locality: us-west-1
    ports:
      http: 80

---
# 集群本地服务导出
apiVersion: networking.istio.io/v1beta1
kind: ServiceEntry
metadata:
  name: export-local-service
  namespace: istio-system
spec:
  hosts:
  - web.default.svc.cluster-east.local
  location: MESH_EXTERNAL
  ports:
  - name: http
    number: 80
    protocol: HTTP
  resolution: STATIC
  endpoints:
  - address: 10.0.1.100  # LoadBalancer IP
    locality: us-east-1
    ports:
      http: 80
```

---

## 4. 网络连通性实现

### 4.1 Cilium ClusterMesh

```yaml
# Cilium ClusterMesh 配置
apiVersion: cilium.io/v2
kind: CiliumClusterwideNetworkPolicy
metadata:
  name: clustermesh-policy
spec:
  endpointSelector:
    matchLabels:
      io.cilium.k8s.policy.cluster: ""
  egress:
  - toEntities:
    - cluster
    toPorts:
    - ports:
      - port: "53"
        protocol: UDP
      - port: "53"
        protocol: TCP
      rules:
        dns:
        - matchPattern: "*"

---
# ClusterMesh 连接配置
apiVersion: v1
kind: Secret
metadata:
  name: cilium-clustermesh
  namespace: kube-system
type: Opaque
data:
  clustermesh-apiserver-remote-config: |
    ---
    endpoints:
    - https://clustermesh-apiserver.cluster-east.svc:2379
    - https://clustermesh-apiserver.cluster-west.svc:2379
    tls-config:
      ca-file: /var/lib/cilium/clustermesh/apiserver.crt
      cert-file: /var/lib/cilium/clustermesh/client.crt
      key-file: /var/lib/cilium/clustermesh/client.key
```

### 4.2 Submariner 网络连接

```yaml
# Submariner 部署配置
apiVersion: v1
kind: ConfigMap
metadata:
  name: submariner-config
  namespace: submariner-operator
data:
  submariner-cr.yaml: |
    apiVersion: submariner.io/v1alpha1
    kind: Submariner
    metadata:
      name: submariner
    spec:
      broker: k8s
      brokerK8sApiServer: broker-api.example.com:6443
      brokerK8sApiServerToken: <token>
      brokerK8sCA: <ca-cert>
      brokerK8sRemoteNamespace: submariner-k8s-broker
      ceIPSecDebug: false
      clusterCIDR: "10.244.0.0/16"
      serviceCIDR: "10.96.0.0/12"
      clusterID: cluster-east
      colorCodes: "blue"
      natEnabled: false
      debug: false
      cableDriver: libreswan

---
# 网关节点配置
apiVersion: apps/v1
kind: DaemonSet
metadata:
  name: submariner-gateway
  namespace: submariner-operator
spec:
  selector:
    matchLabels:
      app: submariner-gateway
  template:
    metadata:
      labels:
        app: submariner-gateway
    spec:
      nodeSelector:
        submariner.io/gateway: "true"
      containers:
      - name: submariner-gateway
        image: quay.io/submariner/submariner-gateway:0.15.0
        env:
        - name: SUBMARINER_NAMESPACE
          valueFrom:
            fieldRef:
              fieldPath: metadata.namespace
        - name: SUBMARINER_CLUSTERID
          value: cluster-east
        - name: SUBMARINER_DEBUG
          value: "false"
        securityContext:
          capabilities:
            add:
            - NET_ADMIN
            - SYS_MODULE
```

---

## 5. 安全策略管理

### 5.1 跨集群网络策略

```yaml
# 跨集群 NetworkPolicy
apiVersion: cilium.io/v2
kind: CiliumClusterwideNetworkPolicy
metadata:
  name: cross-cluster-security
spec:
  endpointSelector:
    matchLabels:
      app: critical-service
  ingress:
  - fromEndpoints:
    - matchLabels:
        app: frontend
        io.cilium.k8s.policy.cluster: cluster-east
    toPorts:
    - ports:
      - port: "8080"
        protocol: TCP
  - fromEndpoints:
    - matchLabels:
        app: monitoring
        io.cilium.k8s.policy.cluster: cluster-west
    toPorts:
    - ports:
      - port: "9090"
        protocol: TCP

---
# 集群间 TLS 认证
apiVersion: cert-manager.io/v1
kind: Certificate
metadata:
  name: cross-cluster-tls
  namespace: istio-system
spec:
  secretName: cross-cluster-tls-secret
  issuerRef:
    name: cluster-issuer
    kind: ClusterIssuer
  commonName: "*.cluster.example.com"
  dnsNames:
  - "cluster-east.cluster.example.com"
  - "cluster-west.cluster.example.com"
  usages:
  - server auth
  - client auth
```

### 5.2 身份认证与授权

```yaml
# SPIFFE/SPIRE 跨集群身份管理
apiVersion: spire.spiffe.io/v1alpha1
kind: ClusterSPIFFEID
metadata:
  name: cross-cluster-workload
spec:
  spiffeIDTemplate: "spiffe://{{.TrustDomain}}/ns/{{.PodMeta.Namespace}}/sa/{{.PodSpec.ServiceAccountName}}"
  podSelector:
    matchLabels:
      security-tier: high
  workloadSelectorTemplates:
  - "k8s:ns:production"
  - "k8s:sa:backend-service"
  federatesWith:
  - "spiffe://cluster-east.example.com"
  - "spiffe://cluster-west.example.com"
```

---

## 6. 流量治理与路由

### 6.1 多集群流量路由

```yaml
# Istio 多集群流量分发
apiVersion: networking.istio.io/v1beta1
kind: VirtualService
metadata:
  name: multi-cluster-routing
  namespace: istio-system
spec:
  hosts:
  - web.example.com
  gateways:
  - istio-ingressgateway
  http:
  - match:
    - headers:
        region:
          exact: east
    route:
    - destination:
        host: web.default.svc.cluster-east.local
        port:
          number: 80
      weight: 100
  - match:
    - headers:
        region:
          exact: west
    route:
    - destination:
        host: web.default.svc.cluster-west.local
        port:
          number: 80
      weight: 100
  - route:
    - destination:
        host: web.default.svc.cluster-east.local
        port:
          number: 80
      weight: 70
    - destination:
        host: web.default.svc.cluster-west.local
        port:
          number: 80
      weight: 30

---
# 故障转移配置
apiVersion: networking.istio.io/v1beta1
kind: DestinationRule
metadata:
  name: multi-cluster-failover
  namespace: istio-system
spec:
  host: web.default.global
  trafficPolicy:
    outlierDetection:
      consecutive5xxErrors: 5
      interval: 10s
      baseEjectionTime: 30s
    loadBalancer:
      localityLbSetting:
        distribute:
        - from: us-east-1/*
          to:
            "us-east-1/*": 80
            "us-west-1/*": 20
        - from: us-west-1/*
          to:
            "us-west-1/*": 80
            "us-east-1/*": 20
```

### 6.2 地理位置感知路由

```yaml
# 基于地理位置的路由
apiVersion: networking.istio.io/v1beta1
kind: EnvoyFilter
metadata:
  name: geo-aware-routing
  namespace: istio-system
spec:
  configPatches:
  - applyTo: HTTP_FILTER
    match:
      context: GATEWAY
      listener:
        filterChain:
          filter:
            name: "envoy.filters.network.http_connection_manager"
    patch:
      operation: INSERT_BEFORE
      value:
        name: envoy.filters.http.geoip
        typed_config:
          "@type": type.googleapis.com/envoy.extensions.filters.http.geoip.v3.Geoip
          geoip_db_path: "/etc/geoip/GeoLite2-City.mmdb"
          header_name: "X-Client-Location"
```

---

## 7. 监控与故障排查

### 7.1 跨集群监控

```yaml
# Prometheus 联邦监控
apiVersion: monitoring.coreos.com/v1
kind: Prometheus
metadata:
  name: federated-prometheus
  namespace: monitoring
spec:
  externalLabels:
    cluster: cluster-east
  remoteWrite:
  - url: http://prometheus-remote-write.monitoring.svc:9090/api/v1/write
    writeRelabelConfigs:
    - sourceLabels: [__name__]
      regex: 'up|prometheus_.*'
      action: keep
  additionalScrapeConfigs:
    name: additional-scrape-configs
    key: multi-cluster-config.yaml

---
# 跨集群指标聚合
apiVersion: v1
kind: ConfigMap
metadata:
  name: multi-cluster-scrape-config
  namespace: monitoring
data:
  multi-cluster-config.yaml: |
    - job_name: 'cluster-west-metrics'
      static_configs:
      - targets: ['prometheus-cluster-west.monitoring.svc:9090']
      honor_labels: true
      metrics_path: '/federate'
      params:
        'match[]':
        - '{job=~"kubernetes-.*"}'
        - '{job=~"istio-.*"}'
```

### 7.2 网络连通性测试

```bash
#!/bin/bash
# 跨集群网络连通性测试脚本

echo "=== 跨集群网络连通性测试 ==="
echo "测试时间: $(date)"
echo

CLUSTERS=("cluster-east" "cluster-west" "cluster-central")

# 测试 Pod 到 Pod 连通性
echo "1. Pod 间连通性测试:"
for src_cluster in "${CLUSTERS[@]}"; do
  for dst_cluster in "${CLUSTERS[@]}"; do
    if [ "$src_cluster" != "$dst_cluster" ]; then
      echo "  ${src_cluster} -> ${dst_cluster}:"
      
      # 获取目标集群 Pod IP
      DST_POD_IP=$(kubectl get pods -n default -l app=test-pod --context=$dst_cluster \
        -o jsonpath='{.items[0].status.podIP}' 2>/dev/null)
      
      if [ ! -z "$DST_POD_IP" ]; then
        # 从源集群测试连接
        RESULT=$(kubectl exec -n default -l app=test-pod --context=$src_cluster \
          -- curl -s -o /dev/null -w "%{http_code}" http://$DST_POD_IP:8080 2>&1)
        
        if [ "$RESULT" == "200" ]; then
          echo "    ✅ 连接成功"
        else
          echo "    ❌ 连接失败 ($RESULT)"
        fi
      else
        echo "    ⚠️  未找到目标 Pod"
      fi
    fi
  done
done

# 测试 Service 连通性
echo -e "\n2. Service 连通性测试:"
for cluster in "${CLUSTERS[@]}"; do
  echo "  测试 $cluster:"
  
  # 测试本地 Service
  LOCAL_RESULT=$(kubectl exec -n default -l app=test-pod --context=$cluster \
    -- curl -s -o /dev/null -w "%{http_code}" http://web-service:80 2>&1)
  
  if [ "$LOCAL_RESULT" == "200" ]; then
    echo "    本地 Service: ✅"
  else
    echo "    本地 Service: ❌ ($LOCAL_RESULT)"
  fi
  
  # 测试跨集群 Service
  CROSS_RESULT=$(kubectl exec -n default -l app=test-pod --context=$cluster \
    -- curl -s -o /dev/null -w "%{http_code}" http://web-service.global:80 2>&1)
  
  if [ "$CROSS_RESULT" == "200" ]; then
    echo "    跨集群 Service: ✅"
  else
    echo "    跨集群 Service: ❌ ($CROSS_RESULT)"
  fi
done
```

---

## 8. 生产环境最佳实践

### 8.1 部署策略

```yaml
# 渐进式多集群部署
apiVersion: batch/v1
kind: Job
metadata:
  name: multi-cluster-deployment-check
  namespace: deployment-validation
spec:
  template:
    spec:
      containers:
      - name: validator
        image: busybox:1.35
        command:
        - /bin/sh
        - -c
        - |
          # 1. 验证集群连通性
          for cluster in cluster-east cluster-west; do
            echo "Checking connectivity to $cluster..."
            kubectl --context=$cluster get nodes >/dev/null 2>&1 || exit 1
          done
          
          # 2. 验证网络策略
          echo "Validating network policies..."
          kubectl get networkpolicy -A --context=cluster-east | wc -l
          kubectl get networkpolicy -A --context=cluster-west | wc -l
          
          # 3. 验证服务发现
          echo "Testing service discovery..."
          nslookup web-service.default.svc.cluster.local
          nslookup web-service.default.global
          
          echo "✅ All validations passed"
        env:
        - name: CLUSTER_CONFIG
          valueFrom:
            configMapKeyRef:
              name: cluster-endpoints
              key: endpoints.yaml
      restartPolicy: Never
```

### 8.2 灾难恢复计划

```yaml
# 多集群灾难恢复配置
apiVersion: v1
kind: ConfigMap
metadata:
  name: disaster-recovery-plan
  namespace: operations
data:
  recovery-playbook.yaml: |
    failure_scenarios:
      - name: "单集群故障"
        detection:
          prometheus_alert: "ClusterDown"
          threshold: "持续5分钟"
        response:
          - step: "健康检查所有集群"
            command: "kubectl get nodes --context=all-clusters"
          - step: "重定向流量到健康集群"
            command: "kubectl patch virtualservice primary-service --patch ..."
          - step: "通知运维团队"
            command: "send-alert.sh 'Cluster failure detected'"
      
      - name: "网络分区"
        detection:
          metric: "cross_cluster_latency > 1000ms"
          duration: "持续10分钟"
        response:
          - step: "验证网络连通性"
            command: "ping-cluster-mesh.sh"
          - step: "重启网络组件"
            command: "kubectl rollout restart daemonset/cilium -n kube-system"
          - step: "验证服务恢复"
            command: "test-service-connectivity.sh"
```

通过以上完整的多集群网络联邦方案，可以实现跨地域、跨云厂商的 Kubernetes 集群统一管理和高效协同。