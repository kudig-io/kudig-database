---
title: GKE 网络深度 — Dataplane V2 与高级网络配置
description: 'GKE Dataplane V2 (Cilium) 配置、Private Cluster、Private Service Connect、VPC Flow Logs 及 Network Policy'
summary: 'GKE Dataplane V2 (Cilium) 配置、Private Cluster、Private Service Connect、VPC Flow Logs 及 Network Policy'
category: cloud-providers
tags:
- cloud
- k8s
- gcp
- gke
- networking
- dataplane-v2
- cilium
tier: supporting
created: '2026-07-02'
last_updated: 2026-07
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 网络工程师
- 平台工程师
estimated_read_time: 15min
intent_queries:
- GKE Dataplane V2 是什么
- 如何配置 GKE 网络
trigger_keywords:
- dataplane-v2
- cilium
- private-cluster
- vpc-flow-logs
- network-policy
prerequisites:
- kubectl-basics
- cloud-basics
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
authors:
- name: KUDIG Team
  role: contributor
---

# GKE 网络深度 — Dataplane V2 与高级网络配置

## 1. Dataplane V2 (Cilium eBPF)

Dataplane V2 是 GKE 使用 Cilium eBPF 实现的数据平面，替代传统 iptables，提供更高性能和更细粒度的网络策略。

### 1.1 启用 Dataplane V2

```bash
# 创建集群时启用
gcloud container clusters create prod-cluster \
  --region=asia-southeast1 \
  --cluster-version=1.31 \
  --enable-dataplane-v2 \
  --release-channel=regular

# 现有集群升级到 Dataplane V2（不可回退）
gcloud container clusters update prod-cluster \
  --region=asia-southeast1 \
  --enable-dataplane-v2
```

```hcl
# Terraform 配置
resource "google_container_cluster" "prod" {
  name     = "prod-cluster"
  location = "asia-southeast1"

  datapath_provider = "ADVANCED_DATAPATH"  # Dataplane V2
  
  # 其他配置...
}
```

### 1.2 Dataplane V2 vs iptables

| 特性 | iptables (Standard) | Datapane V2 (eBPF) |
|------|--------------------|--------------------|
| Service 路由 | iptables 规则 | eBPF 程序 |
| 规则数量增长 | O(n²) | O(1) |
| Network Policy | 需额外安装 | 内置 |
| 可观测性 | 有限 | Hubble 集成 |
| 性能（1000+ Service） | 规则膨胀 | 稳定 |
| DDoS 防护 | 基础 | eBPF 层级 |

### 1.3 Hubble 可观测性

```bash
# 启用 Hubble
gcloud container clusters update prod-cluster \
  --region=asia-southeast1 \
  --enable-hubble

# 安装 Hubble CLI
curl -LO https://github.com/cilium/hubble/releases/latest/download/hubble-linux-amd64.tar.gz
tar xzf hubble-linux-amd64.tar.gz
sudo mv hubble /usr/local/bin/

# 端口转发 Hubble Relay
kubectl port-forward -n kube-system svc/hubble-relay 4245:80

# 查看网络流
hubble observe --namespace production --verdict DROPPED --since 1h

# 查看特定 Pod 的网络流
hubble observe --pod production/api-server --since 30m

# 查看 DNS 查询
hubble observe --type l7 --protocol dns --namespace production
```

### 1.4 Cilium Network Policy

```yaml
# 基于 L7 的策略（HTTP 级别）
apiVersion: cilium.io/v2
kind: CiliumNetworkPolicy
metadata:
  name: api-l7-policy
  namespace: production
spec:
  endpointSelector:
    matchLabels:
      app: api-server
  ingress:
    - fromEndpoints:
        - matchLabels:
            app: web-frontend
      toPorts:
        - ports:
            - port: "8080"
          rules:
            http:
              - method: GET
                path: "/api/v1/.*"
              - method: POST
                path: "/api/v1/orders"
              - method: GET
                path: "/healthz"

---
# DNS 策略（限制出站 DNS 查询）
apiVersion: cilium.io/v2
kind: CiliumNetworkPolicy
metadata:
  name: dns-policy
  namespace: production
spec:
  endpointSelector:
    matchLabels:
      app: backend
  egress:
    - toEndpoints:
        - matchLabels:
            k8s:io.kubernetes.pod.namespace: kube-system
            k8s-app: kube-dns
      toPorts:
        - ports:
            - port: "53"
              protocol: UDP
          rules:
            dns:
              - matchPattern: "*.prod.svc.cluster.local"
              - matchPattern: "*.googleapis.com"

---
# FQDN 策略（限制外部域名访问）
apiVersion: cilium.io/v2
kind: CiliumCiliumNetworkPolicy
metadata:
  name: external-egress
  namespace: production
spec:
  endpointSelector:
    matchLabels:
      app: data-processor
  egress:
    - toFQDNs:
        - matchName: "storage.googleapis.com"
        - matchName: "bigquery.googleapis.com"
      toPorts:
        - ports:
            - port: "443"
              protocol: TCP
```

## 2. Private Cluster

### 2.1 创建 Private Cluster

```bash
gcloud container clusters create private-cluster \
  --region=asia-southeast1 \
  --enable-private-nodes \
  --enable-private-endpoint \
  --master-ipv4-cidr=172.16.0.0/28 \
  --enable-master-authorized-networks \
  --master-authorized-networks=10.0.0.0/8 \
  --no-enable-basic-auth \
  --no-issue-client-certificate
```

```hcl
# Terraform Private Cluster
resource "google_container_cluster" "private" {
  name     = "private-cluster"
  location = "asia-southeast1"

  private_cluster_config {
    enable_private_nodes    = true
    enable_private_endpoint = true  # 控制平面仅私有访问
    master_ipv4_cidr_block  = "172.16.0.0/28"
  }

  master_authorized_networks_config {
    gcp_public_cidrs_access_enabled = false
    
    cidr_blocks {
      cidr_block   = "10.0.0.0/8"
      display_name = "internal-vpc"
    }
    cidr_blocks {
      cidr_block   = "192.168.0.0/16"
      display_name = "vpn-range"
    }
  }

  # 禁用基本认证
  master_auth {
    client_certificate_config {
      issue_client_certificate = false
    }
  }
}
```

### 2.2 Private Cluster 网络要求

```
Private Cluster 网络拓扑:

VPC: 10.0.0.0/16
├── Subnet: 10.0.0.0/20        (节点)
│   └── Secondary Range: 10.4.0.0/14  (Pod)
│   └── Secondary Range: 10.8.0.0/20  (Service)
├── Subnet: 172.16.0.0/28      (控制平面)
└── Firewall Rules:
    ├── 允许 控制平面 → 节点 (TCP 10250, 443)
    ├── 允许 节点 → 控制平面 (TCP 443, 10250)
    └── 允许 Webhook 回调 (TCP 8443, 9443, 15017)
```

### 2.3 访问 Private Cluster

```bash
# 方式一: 通过 VPN/专线
# 配置 kubectl
gcloud container clusters get-credentials private-cluster \
  --region=asia-southeast1 \
  --internal-ip

# 方式二: 通过 IAP 隧道
gcloud compute ssh bastion-host \
  --zone=asia-southeast1-a \
  -- -L 8443:172.16.0.1:443

# 方式三: 通过 GKE Connect Gateway（推荐多集群）
gcloud container fleet memberships register prod-membership \
  --gke-cluster=asia-southeast1/private-cluster \
  --enable-workload-identity
```

## 3. Private Service Connect

### 3.1 暴露 GKE Service 到 PSC

```yaml
# 创建 PSC Service Attachment
apiVersion: networking.gke.io/v1
kind: ServiceAttachment
metadata:
  name: api-service-attachment
  namespace: production
spec:
  connectionPreference: ACCEPT_MANUAL
  natSubnets:
    - projects/my-project/regions/asia-southeast1/subnetworks/psc-nat-subnet
  resourceRef:
    apiVersion: v1
    kind: Service
    name: api-service
  proxyProtocol: false
  consumerAcceptList:
    - project: "123456789012"
      connectionLimit: 10
```

### 3.2 通过 PSC 访问 Google API

```yaml
# 创建 Private Service Connect Endpoint
apiVersion: networking.gke.io/v1
kind: Service
metadata:
  name: google-api-psc
  namespace: production
spec:
  type: ExternalName
  externalName: www.googleapis.com
```

```bash
# 创建 PSC 转发规则
gcloud compute forwarding-rules create psc-google-api \
  --region=asia-southeast1 \
  --network=my-vpc \
  --subnet=my-subnet \
  --address=psc-ip \
  --target-service-attachment=projects/google/regions/asia-southeast1/serviceAttachments/google-api
```

## 4. VPC Flow Logs

### 4.1 启用 VPC Flow Logs

```bash
# 为子网启用 Flow Logs
gcloud compute networks subnets update my-subnet \
  --region=asia-southeast1 \
  --enable-flow-logs \
  --flow-logs-interval=INTERVAL_5_SEC \
  --flow-logs-sampling=0.5 \
  --flow-logs-metadata=INCLUDE_ALL_METADATA \
  --flow-logs-filter="true"
```

```hcl
# Terraform
resource "google_compute_subnetwork" "subnet" {
  name          = "my-subnet"
  region        = "asia-southeast1"
  network       = google_compute_network.vpc.id
  ip_cidr_range = "10.0.0.0/20"

  log_config {
    aggregation_interval = "INTERVAL_5_SEC"
    flow_sampling        = 0.5
    metadata             = "INCLUDE_ALL_METADATA"
    filter_expr          = "true"
  }

  secondary_ip_range {
    range_name    = "pods"
    ip_cidr_range = "10.4.0.0/14"
  }
  secondary_ip_range {
    range_name    = "services"
    ip_cidr_range = "10.8.0.0/20"
  }
}
```

### 4.2 Flow Logs 分析查询

```sql
-- 查找被拒绝的流量
SELECT
  timestamp,
  jsonPayload.src_ip,
  jsonPayload.dest_ip,
  jsonPayload.dest_port,
  jsonPayload.protocol
FROM `my-project.global.compute_subnetwork_flows_all`
WHERE jsonPayload.reporter = "DEST"
  AND jsonPayload.connection.action = "DENY"
  AND timestamp > TIMESTAMP_SUB(CURRENT_TIMESTAMP(), INTERVAL 1 HOUR)
ORDER BY timestamp DESC
LIMIT 100

-- 按流量排名 Top 10
SELECT
  jsonPayload.src_ip,
  jsonPayload.dest_ip,
  SUM(CAST(jsonPayload.bytes_sent AS INT64)) as total_bytes,
  COUNT(*) as flow_count
FROM `my-project.global.compute_subnetwork_flows_all`
WHERE timestamp > TIMESTAMP_SUB(CURRENT_TIMESTAMP(), INTERVAL 1 HOUR)
GROUP BY jsonPayload.src_ip, jsonPayload.dest_ip
ORDER BY total_bytes DESC
LIMIT 10

-- 查找异常 DNS 流量
SELECT
  timestamp,
  jsonPayload.src_ip,
  jsonPayload.dest_ip,
  jsonPayload.dest_port
FROM `my-project.global.compute_subnetwork_flows_all`
WHERE jsonPayload.dest_port = 53
  AND jsonPayload.connection.action = "ALLOW"
  AND timestamp > TIMESTAMP_SUB(CURRENT_TIMESTAMP(), INTERVAL 15 MINUTE)
ORDER BY timestamp DESC
```

## 5. Network Policy 最佳实践

### 5.1 默认拒绝策略

```yaml
# 所有命名空间默认拒绝入站
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: default-deny-ingress
  namespace: production
spec:
  podSelector: {}
  policyTypes:
    - Ingress

---
# 所有命名空间默认拒绝出站
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: default-deny-egress
  namespace: production
spec:
  podSelector: {}
  policyTypes:
    - Egress
  egress:
    - to:  # 仅允许 DNS
        - namespaceSelector: {}
      ports:
        - protocol: UDP
          port: 53
```

### 5.2 微分段策略

```yaml
# 允许前端访问后端 API
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: allow-frontend-to-api
  namespace: production
spec:
  podSelector:
    matchLabels:
      tier: api
  policyTypes:
    - Ingress
  ingress:
    - from:
        - podSelector:
            matchLabels:
              tier: frontend
      ports:
        - protocol: TCP
          port: 8080

---
# 允许后端访问数据库
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: allow-api-to-database
  namespace: production
spec:
  podSelector:
    matchLabels:
      tier: database
  policyTypes:
    - Ingress
  ingress:
    - from:
        - podSelector:
            matchLabels:
              tier: api
      ports:
        - protocol: TCP
          port: 5432
```

### 5.3 Network Policy 调试

```bash
# 查看策略是否生效
kubectl get networkpolicy -A -o wide

# 使用 Hubble 验证策略
hubble observe --namespace production --verdict DROPPED --since 10m

# 测试连通性
kubectl run test-pod --image=nicolaka/netshoot --rm -it -n production -- \
  curl -s --max-time 5 http://api-service:8080/healthz
```

## 6. Service Mesh 集成

### 6.1 Anthos Service Mesh (Istio)

```bash
# 安装 Anthos Service Mesh
asmcli install \
  --cluster_name prod-cluster \
  --cluster_location asia-southeast1 \
  --project_id my-project \
  --enable_all \
  --output_dir ./asm-output
```

```yaml
# mTLS 策略
apiVersion: security.istio.io/v1beta1
kind: PeerAuthentication
metadata:
  name: default
  namespace: production
spec:
  mtls:
    mode: STRICT

---
# 授权策略
apiVersion: security.istio.io/v1beta1
kind: AuthorizationPolicy
metadata:
  name: api-authz
  namespace: production
spec:
  selector:
    matchLabels:
      app: api-server
  rules:
    - from:
        - source:
            principals: ["cluster.local/ns/production/sa/web-frontend"]
      to:
        - operation:
            methods: ["GET", "POST"]
            paths: ["/api/*"]
```

## Related

- [[02-gke-autopilot-serverless]]
- [[05-gke-workload-identity-security]]

## See Also

- GKE Dataplane V2 文档
- Cilium Network Policy
- GKE Private Cluster
