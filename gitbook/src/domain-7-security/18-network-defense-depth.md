# 18 - 网络安全纵深防御体系

> **适用版本**: Kubernetes v1.25 - v1.32 | **难度**: 高级 | **参考**: [NIST SP 800-207 Zero Trust Architecture](https://csrc.nist.gov/publications/detail/sp/800-207/final) | [CIS Kubernetes Benchmark](https://www.cisecurity.org/benchmark/kubernetes)

## 一、网络安全纵深防御架构

### 1.1 纵深防御分层模型

```
┌─────────────────────────────────────────────────────────────────────────────────────┐
│                      Kubernetes 网络安全纵深防御体系                                 │
├─────────────────────────────────────────────────────────────────────────────────────┤
│                                                                                      │
│  ┌────────────────────────────────────────────────────────────────────────────────┐ │
│  │                              Perimeter Layer (边界层)                           │ │
│  │  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐       │ │
│  │  │   Firewall   │  │   WAF        │  │   IDS/IPS    │  │   DDoS防护   │       │ │
│  │  │   边界防火墙  │  │   应用防火墙  │  │   入侵检测    │  │   流量清洗    │       │ │
│  │  └──────┬───────┘  └──────┬───────┘  └──────┬───────┘  └──────┬───────┘       │ │
│  │         │                 │                 │                 │                │ │
│  │         └─────────────────┼─────────────────┼─────────────────┘                │ │
│  │                           │                 │                                  │ │
│  │                    ┌──────▼─────────────────▼──────┐                          │ │
│  │                    │     Load Balancer Security    │                          │ │
│  │                    │      负载均衡器安全配置        │                          │ │
│  │                    └───────────────────────────────┘                          │ │
│  └────────────────────────────────────────────────────────────────────────────────┘ │
│                                    │                                                │
│  ┌─────────────────────────────────▼──────────────────────────────────────────────┐ │
│  │                            Cluster Layer (集群层)                              │ │
│  │  ┌─────────────────────────────────────────────────────────────────────────┐   │ │
│  │  │                         Network Policies                                │   │ │
│  │  │  ┌────────────┐  ┌────────────┐  ┌────────────┐  ┌────────────┐         │   │ │
│  │  │  │ CNI Plugin │  │  Namespace  │  │   Ingress   │  │  Egress     │         │   │ │
│  │  │  │ 网络插件    │  │  网络隔离    │  │   控制      │  │   控制      │         │   │ │
│  │  │  │ Calico/Cilium│ │  策略       │  │   策略      │  │   策略      │         │   │ │
│  │  │  └────────────┘  └────────────┘  └────────────┘  └────────────┘         │   │ │
│  │  └─────────────────────────────────────────────────────────────────────────┘   │ │
│  │  ┌─────────────────────────────────────────────────────────────────────────┐   │ │
│  │  │                         Service Mesh                                    │   │ │
│  │  │  ┌────────────┐  ┌────────────┐  ┌────────────┐  ┌────────────┐         │   │ │
│  │  │  │   Istio    │  │   Linkerd   │  │   Consul    │  │   OpenSergo │         │   │ │
│  │  │  │   mTLS     │  │   mTLS      │  │   mTLS      │  │   mTLS      │         │   │ │
│  │  │  │   流量治理  │  │   流量治理   │  │   服务发现   │  │   流量治理   │         │   │ │
│  │  │  └────────────┘  └────────────┘  └────────────┘  └────────────┘         │   │ │
│  │  └─────────────────────────────────────────────────────────────────────────┘   │ │
│  └────────────────────────────────────────────────────────────────────────────────┘ │
│                                    │                                                │
│  ┌─────────────────────────────────▼──────────────────────────────────────────────┐ │
│  │                           Node Layer (节点层)                                   │ │
│  │  ┌─────────────────────────────────────────────────────────────────────────┐   │ │
│  │  │                         Host Network Security                           │   │ │
│  │  │  ┌────────────┐  ┌────────────┐  ┌────────────┐  ┌────────────┐         │   │ │
│  │  │  │ iptables   │  │   firewalld │  │   ebtables  │  │   nftables  │         │   │ │
│  │  │  │ 包过滤     │  │   动态防火墙  │  │   二层过滤   │  │   新一代    │         │   │ │
│  │  │  └────────────┘  └────────────┘  └────────────┘  └────────────┘         │   │ │
│  │  └─────────────────────────────────────────────────────────────────────────┘   │ │
│  │  ┌─────────────────────────────────────────────────────────────────────────┐   │ │
│  │  │                         Container Network                               │   │ │
│  │  │  ┌────────────┐  ┌────────────┐  ┌────────────┐  ┌────────────┐         │   │ │
│  │  │  │ CNI配置    │  │ 网络命名空间 │  │   Veth Pair │  │   Bridge    │         │   │ │
│  │  │  │ 网络策略    │  │ 隔离        │  │   虚拟网卡   │  │   网桥      │         │   │ │
│  │  │  └────────────┘  └────────────┘  └────────────┘  └────────────┘         │   │ │
│  │  └─────────────────────────────────────────────────────────────────────────┘   │ │
│  └────────────────────────────────────────────────────────────────────────────────┘ │
│                                    │                                                │
│  ┌─────────────────────────────────▼──────────────────────────────────────────────┐ │
│  │                          Application Layer (应用层)                            │ │
│  │  ┌─────────────────────────────────────────────────────────────────────────┐   │ │
│  │  │                         Pod Security                                    │   │ │
│  │  │  ┌────────────┐  ┌────────────┐  ┌────────────┐  ┌────────────┐         │   │ │
│  │  │  │ Network    │  │   Runtime   │  │   Image     │  │   Process   │         │   │ │
│  │  │  │   Policy   │  │   Security  │  │   Scanning  │  │   Isolation │         │   │ │
│  │  │  │   网络策略  │  │   运行时    │  │   镜像扫描   │  │   进程隔离   │         │   │ │
│  │  │  └────────────┘  └────────────┘  └────────────┘  └────────────┘         │   │ │
│  │  └─────────────────────────────────────────────────────────────────────────┘   │ │
│  └────────────────────────────────────────────────────────────────────────────────┘ │
│                                                                                      │
└─────────────────────────────────────────────────────────────────────────────────────┘
```

### 1.2 生产环境网络分区策略

#### 网络区域划分标准

| 区域 | 用途 | 访问控制 | 监控级别 | 数据流方向 |
|-----|------|---------|---------|-----------|
| **DMZ** | 对外服务 | 严格限制 | 全流量监控 | Inbound Only |
| **Frontend** | 前端应用 | 有限访问 | 详细监控 | In/Out |
| **Backend** | 后端服务 | 最小权限 | 完整监控 | Internal Only |
| **Data** | 数据存储 | 最严格 | 审计级监控 | Outbound Only |
| **Management** | 管理平面 | 特权访问 | 安全审计 | Admin Only |

#### 多租户网络隔离方案

```yaml
# 01-multitenant-network-isolation.yaml
apiVersion: v1
kind: Namespace
metadata:
  name: tenant-a-production
  labels:
    tenant: "tenant-a"
    environment: "production"
    security-tier: "backend"
---
apiVersion: v1
kind: Namespace
metadata:
  name: tenant-b-staging
  labels:
    tenant: "tenant-b"
    environment: "staging"
    security-tier: "frontend"
---
# 默认拒绝所有跨租户流量
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: deny-cross-tenant-traffic
  namespace: tenant-a-production
spec:
  podSelector: {}
  policyTypes:
  - Ingress
  - Egress
  ingress:
  - from:
    - namespaceSelector:
        matchLabels:
          tenant: "tenant-a"  # 只允许同租户内通信
  egress:
  - to:
    - namespaceSelector:
        matchLabels:
          tenant: "tenant-a"
```

## 二、CNI插件安全配置

### 2.1 Calico生产级配置

#### 安全增强配置参数

```yaml
# 02-calico-secure-config.yaml
apiVersion: operator.tigera.io/v1
kind: Installation
metadata:
  name: default
spec:
  calicoNetwork:
    # 启用IP-in-IP隧道加密
    ipPools:
    - cidr: 10.244.0.0/16
      encapsulation: VXLANCrossSubnet  # 使用VXLAN替代IPIP
      natOutgoing: Enabled
      blockSize: 26
    
    # 启用BGP对等体认证
    bgp:
      enabled: true
      peers:
      - peerIP: 192.168.1.100
        asNumber: 64512
        password:
          secretKeyRef:
            name: bgp-password
            key: password
    
    # 启用日志记录
    logSeverityScreen: Info
    nodeAddressAutodetectionV4:
      interface: eth.*
  
  # 启用全局网络策略
  networkPolicy:
    enabled: true
    
  # 启用felix配置安全增强
  felixConfiguration:
    bpfEnabled: true  # 启用eBPF加速
    bpfLogLevel: Info
    reportingInterval: 0  # 禁用定期报告减少攻击面
    xdpEnabled: true      # 启用XDP加速
    routeTableRange: "100-120"
```

#### 生产级Calico网络策略

```yaml
# 03-calico-global-policies.yaml
apiVersion: projectcalico.org/v3
kind: GlobalNetworkPolicy
metadata:
  name: default-deny
spec:
  order: 1000
  selector: all()
  types:
  - Ingress
  - Egress
---
apiVersion: projectcalico.org/v3
kind: GlobalNetworkPolicy
metadata:
  name: allow-dns
spec:
  order: 100
  selector: all()
  types:
  - Egress
  egress:
  - action: Allow
    protocol: UDP
    destination:
      ports:
      - 53
    destination:
      nets:
      - 10.96.0.10/32  # CoreDNS Service IP
---
apiVersion: projectcalico.org/v3
kind: GlobalNetworkPolicy
metadata:
  name: allow-kubelet
spec:
  order: 200
  selector: has(node-role.kubernetes.io/control-plane)
  types:
  - Ingress
  ingress:
  - action: Allow
    protocol: TCP
    source:
      selector: all()
    destination:
      ports:
      - 10250  # Kubelet API
```

### 2.2 Cilium安全特性配置

#### 启用透明加密

```yaml
# 04-cilium-ipsec-config.yaml
apiVersion: v1
kind: ConfigMap
metadata:
  name: cilium-config
  namespace: kube-system
data:
  # 启用IPSec加密
  enable-ipsec: "true"
  ipsec-key-file: /etc/ipsec/keys
  
  # 启用WireGuard加密
  enable-wireguard: "true"
  
  # 启用Hubble可观测性
  enable-hubble: "true"
  hubble-listen-address: ":4244"
  hubble-metrics-server: ":9965"
  hubble-flow-buffer-size: "65536"
  
  # 启用带宽管理
  enable-bandwidth-manager: "true"
  
  # 启用本地重定向策略
  enable-local-redirect-policy: "true"
  
  # 启用服务网格
  enable-envoy-config: "true"
```

#### Cilium网络安全策略

```yaml
# 05-cilium-network-policy.yaml
apiVersion: cilium.io/v2
kind: CiliumNetworkPolicy
metadata:
  name: secure-backend-app
  namespace: production
spec:
  endpointSelector:
    matchLabels:
      app: backend-api
      
  # 入站规则
  ingress:
  - fromEndpoints:
    - matchLabels:
        app: frontend-web
    toPorts:
    - ports:
      - port: "8080"
        protocol: TCP
      rules:
        http:
        - method: "GET"
          path: "/api/*"
          
  # 出站规则
  egress:
  - toEntities:
    - cluster  # 只能访问集群内服务
  - toCIDR:
    - 10.0.0.0/8  # 允许访问内部网络
    except:
    - 10.0.0.0/32  # 排除特定IP
  - toServices:
    - k8sService:
        serviceName: database
        namespace: database
    toPorts:
    - ports:
      - port: "5432"
        protocol: TCP
        
  # L7策略示例
  - fromEndpoints:
    - matchLabels:
        app: monitoring
    toPorts:
    - ports:
      - port: "8080"
      rules:
        http:
        - method: "GET"
          path: "/metrics"
          headers:
          - "Authorization": "Bearer monitoring-token.*"
```

## 三、服务网格安全配置

### 3.1 Istio mTLS配置

#### 全局mTLS策略

```yaml
# 06-istio-mtls-policy.yaml
apiVersion: security.istio.io/v1beta1
kind: PeerAuthentication
metadata:
  name: default
  namespace: istio-system
spec:
  mtls:
    mode: STRICT  # 强制mTLS
---
apiVersion: security.istio.io/v1beta1
kind: PeerAuthentication
metadata:
  name: disable-mtls-for-monitoring
  namespace: monitoring
spec:
  selector:
    matchLabels:
      app: prometheus
  mtls:
    mode: DISABLE  # 监控组件禁用mTLS
---
# 命名空间级别mTLS配置
apiVersion: security.istio.io/v1beta1
kind: PeerAuthentication
metadata:
  name: permissive-mode
  namespace: legacy-apps
spec:
  mtls:
    mode: PERMISSIVE  # 渐进式迁移
```

#### 细粒度授权策略

```yaml
# 07-istio-authorization-policy.yaml
apiVersion: security.istio.io/v1beta1
kind: AuthorizationPolicy
metadata:
  name: backend-api-access
  namespace: production
spec:
  selector:
    matchLabels:
      app: backend-api
      
  # 允许规则
  rules:
  - from:
    - source:
        principals: ["cluster.local/ns/frontend/sa/frontend-sa"]
        namespaces: ["frontend"]
    to:
    - operation:
        methods: ["GET", "POST"]
        paths: ["/api/v1/*"]
        ports: ["8080"]
        
  - from:
    - source:
        principals: ["cluster.local/ns/monitoring/sa/prometheus"]
        namespaces: ["monitoring"]
    to:
    - operation:
        methods: ["GET"]
        paths: ["/metrics"]
        ports: ["8080"]
        
  # 拒绝规则（黑名单）
  - to:
    - operation:
        methods: ["DELETE"]
        paths: ["/admin/*"]
    when:
    - key: request.auth.claims[groups]
      notValues: ["admin"]
```

### 3.2 服务网格流量治理

#### 流量加密和证书管理

```yaml
# 08-istio-destination-rule.yaml
apiVersion: networking.istio.io/v1alpha3
kind: DestinationRule
metadata:
  name: secure-backend-service
  namespace: production
spec:
  host: backend-api.production.svc.cluster.local
  trafficPolicy:
    # TLS设置
    tls:
      mode: ISTIO_MUTUAL  # 使用Istio双向TLS
      sni: backend-api.production.svc.cluster.local
      
    # 连接池设置
    connectionPool:
      tcp:
        maxConnections: 100
        connectTimeout: 30ms
      http:
        http1MaxPendingRequests: 1000
        maxRequestsPerConnection: 10
        
    # 负载均衡
    loadBalancer:
      simple: LEAST_REQUEST
      localityLbSetting:
        enabled: true
        failover:
        - from: us-central1
          to: us-west1
          
    # 熔断器
    outlierDetection:
      consecutive5xxErrors: 5
      interval: 1m
      baseEjectionTime: 30s
```

## 四、网络安全监控与告警

### 4.1 网络流量异常检测

#### NetFlow采集配置

```yaml
# 09-netflow-collector.yaml
apiVersion: apps/v1
kind: DaemonSet
metadata:
  name: netflow-collector
  namespace: monitoring
spec:
  selector:
    matchLabels:
      app: netflow-collector
  template:
    metadata:
      labels:
        app: netflow-collector
    spec:
      hostNetwork: true
      containers:
      - name: collector
        image: networkstatic/nfcapd:latest
        args:
        - "-D"  # 后台运行
        - "-l"  # 监听端口
        - "/data"
        - "-p"
        - "9995"
        - "-S"
        - "1"   # 每分钟轮转
        volumeMounts:
        - name: data
          mountPath: /data
        securityContext:
          privileged: true
      volumes:
      - name: data
        hostPath:
          path: /var/lib/netflow
```

#### 网络异常Prometheus告警规则

```yaml
# 10-network-alerts.yaml
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: network-security-alerts
  namespace: monitoring
spec:
  groups:
  - name: network.security
    rules:
    # 异常连接数告警
    - alert: HighConnectionCount
      expr: sum(rate(container_network_tcp_connection_opened_total[5m])) > 1000
      for: 5m
      labels:
        severity: warning
      annotations:
        summary: "TCP连接数过高"
        description: "Pod {{ $labels.pod }} 的TCP连接数超过阈值"
        
    # 网络流量突增告警
    - alert: NetworkTrafficSurge
      expr: rate(container_network_receive_bytes_total[5m]) > 100e6
      for: 3m
      labels:
        severity: critical
      annotations:
        summary: "网络流量激增"
        description: "Pod {{ $labels.pod }} 接收流量超过100MB/s"
        
    # DNS查询异常
    - alert: SuspiciousDNSQueries
      expr: increase(dns_queries_total{rcode!="NOERROR"}[10m]) > 100
      for: 2m
      labels:
        severity: warning
      annotations:
        summary: "可疑DNS查询"
        description: "检测到大量失败的DNS查询"
        
    # 端口扫描检测
    - alert: PortScanDetected
      expr: rate(net_conntrack_dialer_conn_failed_total[5m]) > 50
      for: 1m
      labels:
        severity: critical
      annotations:
        summary: "端口扫描活动"
        description: "检测到可能的端口扫描行为"
```

### 4.2 网络可视化监控

#### NetworkPolicy可视化Dashboard

```json
{
  "dashboard": {
    "title": "Network Security Dashboard",
    "panels": [
      {
        "title": "Network Policy Compliance",
        "type": "gauge",
        "targets": [
          {
            "expr": "sum(kube_networkpolicy_status) / count(kube_networkpolicy) * 100",
            "legendFormat": "Compliance Rate"
          }
        ]
      },
      {
        "title": "Blocked Traffic by Policy",
        "type": "graph",
        "targets": [
          {
            "expr": "rate(networkpolicy_blocked_packets_total[5m])",
            "legendFormat": "{{policy}}"
          }
        ]
      },
      {
        "title": "Cross-Namespace Traffic",
        "type": "table",
        "targets": [
          {
            "expr": "sum by(source_namespace, destination_namespace) (network_traffic_bytes_total)",
            "format": "table"
          }
        ]
      }
    ]
  }
}
```

## 五、网络安全最佳实践

### 5.1 生产环境部署清单

#### 网络安全基线检查

```bash
#!/bin/bash
# 11-network-security-check.sh

echo "=== Kubernetes Network Security Baseline Check ==="

# 1. 检查默认网络策略
echo "1. Checking default deny policies..."
kubectl get networkpolicies --all-namespaces | grep -E "(default-deny|deny-all)"

# 2. 检查CNI插件安全配置
echo "2. Checking CNI security configuration..."
kubectl get cm -n kube-system calico-config -o yaml | grep -E "(ipip|vxlan|encryption)"

# 3. 检查服务网格mTLS状态
echo "3. Checking service mesh mTLS..."
kubectl exec -n istio-system deploy/istiod -- pilot-discovery request GET /debug/securityz

# 4. 检查网络隔离配置
echo "4. Checking namespace isolation..."
kubectl get ns -l 'tenant=*' --show-labels

# 5. 检查外部访问控制
echo "5. Checking external access controls..."
kubectl get ingress --all-namespaces -o wide

# 6. 检查网络策略覆盖率
echo "6. Checking network policy coverage..."
kubectl get pods --all-namespaces -o jsonpath='{range .items[*]}{.metadata.namespace}/{.metadata.name}{"\n"}{end}' | \
  while read pod; do
    ns=$(echo $pod | cut -d'/' -f1)
    name=$(echo $pod | cut -d'/' -f2)
    policies=$(kubectl get networkpolicy -n $ns --field-selector spec.podSelector.matchLabels."app"="$(kubectl get pod $name -n $ns -o jsonpath='{.metadata.labels.app}')" 2>/dev/null | wc -l)
    if [ "$policies" -eq 0 ]; then
      echo "WARNING: Pod $pod has no network policies"
    fi
  done
```

#### 网络安全加固脚本

```bash
#!/bin/bash
# 12-network-hardening.sh

echo "=== Network Security Hardening ==="

# 1. 启用网络策略默认拒绝
echo "1. Enabling default deny network policies..."
kubectl apply -f - <<EOF
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: default-deny-all
  namespace: default
spec:
  podSelector: {}
  policyTypes:
  - Ingress
  - Egress
EOF

# 2. 配置核心组件网络策略
echo "2. Configuring core component network policies..."
kubectl apply -f https://raw.githubusercontent.com/ahmetb/kubernetes-network-policy-recipes/master/01-deny-all-traffic-to-an-application.yaml

# 3. 启用CNI插件安全特性
echo "3. Enabling CNI security features..."
kubectl patch configmap calico-config -n kube-system --type merge -p '{"data":{"enable_ipip":"false"}}'

# 4. 配置iptables规则
echo "4. Configuring iptables rules..."
iptables -A INPUT -p tcp --dport 10250 -s 10.0.0.0/8 -j ACCEPT
iptables -A INPUT -p tcp --dport 10250 -j DROP

# 5. 启用网络监控
echo "5. Enabling network monitoring..."
kubectl apply -f https://raw.githubusercontent.com/coreos/kube-prometheus/main/manifests/network-metrics.yaml

echo "Network security hardening completed!"
```

### 5.2 网络安全运维规范

#### 日常巡检清单

```markdown
## 网络安全日常巡检清单

### 每日检查
- [ ] 网络策略生效情况检查
- [ ] 异常网络流量监控
- [ ] DNS查询异常检测
- [ ] 跨命名空间流量审计

### 每周检查
- [ ] 网络策略覆盖率统计
- [ ] 服务网格mTLS状态验证
- [ ] CNI插件版本更新检查
- [ ] 网络性能基准测试

### 每月检查
- [ ] 网络安全架构评审
- [ ] 网络隔离有效性测试
- [ ] 第三方安全扫描结果分析
- [ ] 网络安全策略优化
```

这份网络安全纵深防御文档从业务视角出发，涵盖了从边界防护到应用层的完整安全体系，特别强调了生产环境的实际部署和运维经验。