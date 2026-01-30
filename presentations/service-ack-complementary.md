# Kubernetes Service ACK 补充技术文档

## 3.2 负载均衡器选择策略（续）

### 3.2.2 NLB (网络型负载均衡)

**性能优势**
- 超低延迟 (<1ms)
- 超高并发 (百万级连接)
- 更好的网络性能

**配置示例**
```yaml
apiVersion: v1
kind: Service
metadata:
  name: nlb-service
  annotations:
    # 指定NLB实例
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-id: "nlb-xxxxxxxxx"
    
    # NLB规格
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-spec: "nlb.s1.small"
    
    # 地址类型
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-address-type: "Internet"
spec:
  selector:
    app: high-performance-app
  ports:
    - protocol: TCP
      port: 80
      targetPort: 8080
  type: LoadBalancer
```

### 3.2.3 ALB (应用型负载均衡)

**七层特性**
- HTTP/HTTPS协议支持
- 基于内容的路由
- 丰富的安全特性

**完整配置示例**
```yaml
apiVersion: v1
kind: Service
metadata:
  name: alb-service
  annotations:
    # ALB配置
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-id: "alb-xxxxxxxxx"
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-protocol-port: "https:443"
    
    # SSL证书
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-cert-id: "xxxxxx"
    
    # 访问控制
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-acl-status: "on"
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-acl-id: "acl-xxxxxx"
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-acl-type: "white"
spec:
  selector:
    app: web-application
  ports:
    - name: https
      protocol: TCP
      port: 443
      targetPort: 8443
  type: LoadBalancer
```

## 4. ACK产品深度集成

### 4.1 安全组集成配置

**精细化安全控制**
```yaml
apiVersion: v1
kind: Service
metadata:
  name: secure-service
  annotations:
    # 绑定安全组
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-security-group-id: "sg-xxxxxxxxx"
    
    # 访问控制列表
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-access-control-enable: "on"
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-access-control-type: "white"
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-access-control-list: "192.168.0.0/16,10.0.0.0/8"
    
    # 删除保护
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-delete-protection: "on"
spec:
  selector:
    app: secure-app
  ports:
    - protocol: TCP
      port: 443
      targetPort: 8443
  type: LoadBalancer
```

### 4.2 多可用区部署

**跨AZ高可用配置**
```yaml
apiVersion: v1
kind: Service
metadata:
  name: multi-az-service
  annotations:
    # 多可用区部署
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-master-zone-id: "cn-hangzhou-a"
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-slave-zone-id: "cn-hangzhou-b"
    
    # 健康检查增强
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-health-check-connect-timeout: "5"
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-health-check-interval: "2"
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-healthy-threshold: "3"
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-unhealthy-threshold: "3"
spec:
  selector:
    app: multi-az-app
  ports:
    - protocol: TCP
      port: 80
      targetPort: 8080
  type: LoadBalancer
```

## 5. 生产级配置模板

### 5.1 标准Web服务配置

```yaml
apiVersion: v1
kind: Service
metadata:
  name: production-web-service
  namespace: production
  annotations:
    # 负载均衡器配置
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-id: "lb-xxxxxxxxx"
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-spec: "slb.s2.medium"
    
    # 网络配置
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-bandwidth: "200"
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-charge-type: "paybybandwidth"
    
    # 安全配置
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-security-group-id: "sg-web-prod"
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-delete-protection: "on"
    
    # 健康检查
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-health-check-flag: "on"
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-health-check-type: "http"
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-health-check-uri: "/health"
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-health-check-http-code: "http_2xx"
    
    # 监控配置
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-enable-access-log: "on"
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-access-log-bucket: "prod-slb-logs"
spec:
  selector:
    app: web-application
    tier: frontend
    env: production
  ports:
    - name: http
      protocol: TCP
      port: 80
      targetPort: 8080
    - name: https
      protocol: TCP
      port: 443
      targetPort: 8443
  type: LoadBalancer
  externalTrafficPolicy: Local
  sessionAffinity: ClientIP
  sessionAffinityConfig:
    clientIP:
      timeoutSeconds: 10800
```

### 5.2 内部服务配置

```yaml
apiVersion: v1
kind: Service
metadata:
  name: internal-database-service
  namespace: backend
  annotations:
    # 内网负载均衡
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-address-type: "intranet"
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-network-type: "vpc"
    
    # 安全配置
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-security-group-id: "sg-backend"
    
    # 连接池优化
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-connection-drain: "on"
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-connection-drain-timeout: "300"
spec:
  selector:
    app: database
    tier: backend
  ports:
    - name: mysql
      protocol: TCP
      port: 3306
      targetPort: 3306
    - name: redis
      protocol: TCP
      port: 6379
      targetPort: 6379
  type: LoadBalancer
  externalTrafficPolicy: Cluster
```

## 6. 性能优化与调优

### 6.1 kube-proxy 性能优化

**IPVS模式配置**
```yaml
apiVersion: kubeproxy.config.k8s.io/v1alpha1
kind: KubeProxyConfiguration
mode: ipvs
ipvs:
  scheduler: "rr"
  excludeCIDRs: []
  strictARP: true
  tcpTimeout: 0s
  tcpFinTimeout: 0s
  udpTimeout: 0s
  minSyncPeriod: 0s
  syncPeriod: 30s
  masqueradeAll: false
  masqueradeBit: 14
```

**系统参数调优**
```bash
# 增加conntrack表大小
echo "net.netfilter.nf_conntrack_max = 1048576" >> /etc/sysctl.conf
echo "net.netfilter.nf_conntrack_buckets = 262144" >> /etc/sysctl.conf

# 调整TCP参数
echo "net.ipv4.tcp_fin_timeout = 30" >> /etc/sysctl.conf
echo "net.ipv4.tcp_keepalive_time = 1200" >> /etc/sysctl.conf
echo "net.core.somaxconn = 65535" >> /etc/sysctl.conf

sysctl -p
```

### 6.2 负载均衡器优化

**连接复用配置**
```yaml
apiVersion: v1
kind: Service
metadata:
  name: optimized-service
  annotations:
    # 连接池配置
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-persistence-timeout: "1800"
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-connection-drain: "on"
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-connection-drain-timeout: "300"
    
    # 性能优化
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-scheduler: "wrr"  # 加权轮询
spec:
  selector:
    app: optimized-app
  ports:
    - protocol: TCP
      port: 80
      targetPort: 8080
  type: LoadBalancer
```

## 7. 安全加固实践

### 7.1 网络安全配置

**网络安全策略**
```yaml
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: service-network-policy
  namespace: production
spec:
  podSelector:
    matchLabels:
      app: web-service
  policyTypes:
  - Ingress
  - Egress
  ingress:
  - from:
    - namespaceSelector:
        matchLabels:
          name: frontend
    - podSelector:
        matchLabels:
          role: api-gateway
    ports:
    - protocol: TCP
      port: 8080
  egress:
  - to:
    - namespaceSelector:
        matchLabels:
          name: database
    ports:
    - protocol: TCP
      port: 3306
```

### 7.2 TLS/SSL配置

**HTTPS服务配置**
```yaml
apiVersion: v1
kind: Service
metadata:
  name: tls-service
  annotations:
    # SSL证书配置
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-cert-id: "cert-xxxxxxxxx"
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-protocol-port: "https:443"
    
    # TLS安全策略
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-tls-cipher-policy: "tls_cipher_policy_1_2_strict"
    
    # HTTP重定向
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-redirect-http-to-https: "on"
spec:
  selector:
    app: tls-enabled-app
  ports:
    - name: https
      protocol: TCP
      port: 443
      targetPort: 8443
    - name: http
      protocol: TCP
      port: 80
      targetPort: 8080
  type: LoadBalancer
```

## 8. 监控告警配置

### 8.1 关键监控指标

**Service层级监控**
```yaml
# Prometheus监控配置
apiVersion: monitoring.coreos.com/v1
kind: ServiceMonitor
metadata:
  name: service-monitor
  namespace: monitoring
spec:
  selector:
    matchLabels:
      app: prometheus-operator
  endpoints:
  - port: metrics
    interval: 30s
    path: /metrics
  namespaceSelector:
    matchNames:
    - default
```

**关键指标列表**
- `kube_service_status_load_balancer_ingress`
- `kube_service_info`
- `kube_service_spec_type`
- `kube_endpoint_address_available`
- `kube_endpoint_address_not_ready`

### 8.2 告警规则配置

**Prometheus告警规则**
```yaml
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: service-alerts
  namespace: monitoring
spec:
  groups:
  - name: service.rules
    rules:
    - alert: ServiceDown
      expr: kube_service_status_load_balancer_ingress == 0
      for: 5m
      labels:
        severity: critical
      annotations:
        summary: "Service {{ $labels.service }} is down"
        description: "Service {{ $labels.service }} in namespace {{ $labels.namespace }} has no load balancer ingress"
        
    - alert: ServiceEndpointsMissing
      expr: kube_endpoint_address_available == 0
      for: 2m
      labels:
        severity: warning
      annotations:
        summary: "Service endpoints missing"
        description: "Service {{ $labels.service }} has no available endpoints"
```

## 9. 故障排查手册

### 9.1 常见问题诊断

**Service无法访问排查步骤**

1. **检查Service状态**
```bash
# 查看Service基本信息
kubectl get svc <service-name> -o wide

# 查看详细配置
kubectl describe svc <service-name>

# 检查YAML配置
kubectl get svc <service-name> -o yaml
```

2. **验证Endpoints**
```bash
# 查看Endpoints
kubectl get endpoints <service-name>

# 检查Pod状态
kubectl get pods -l <selector-labels>

# 验证Pod就绪状态
kubectl get pods -l <selector-labels> -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{.status.phase}{"\t"}{.status.conditions[?(@.type=="Ready")].status}{"\n"}{end}'
```

3. **网络连通性测试**
```bash
# 在集群内测试
kubectl run debug --image=busybox --rm -it -- sh
# 在Pod内执行
nslookup <service-name>.<namespace>
telnet <service-ip> <port>

# 测试负载均衡器
curl -v http://<load-balancer-ip>:<port>
```

### 9.2 阿里云特定问题

**负载均衡器相关问题**

```bash
# 检查SLB实例状态
aliyun slb DescribeLoadBalancers --LoadBalancerId lb-xxxxxxxxx

# 查看后端服务器状态
aliyun slb DescribeHealthStatus --LoadBalancerId lb-xxxxxxxxx

# 检查安全组规则
aliyun ecs DescribeSecurityGroupAttribute --SecurityGroupId sg-xxxxxxxxx
```

**日志分析命令**
```bash
# 查看kube-proxy日志
kubectl logs -n kube-system -l k8s-app=kube-proxy --tail=100

# 查看CCM日志
kubectl logs -n kube-system -l k8s-app=cloud-controller-manager --tail=100

# 查看CoreDNS日志
kubectl logs -n kube-system -l k8s-app=kube-dns --tail=100
```

## 10. 最佳实践总结

### 10.1 配置规范

**命名规范**
- Service名称：`<应用名>-<环境>-svc`
- 端口名称：`<协议>-<用途>` (如 http-api, https-web)
- Annotation前缀：使用标准阿里云注解

**标签规范**
```yaml
metadata:
  labels:
    app: <应用名称>
    version: <版本号>
    env: <环境标识>
    tier: <层级标识>
```

### 10.2 运维建议

**定期检查清单**
- [ ] Service健康状态监控
- [ ] Endpoints可用性检查
- [ ] 负载均衡器性能指标
- [ ] 安全组规则审查
- [ ] SSL证书有效期检查
- [ ] 访问日志分析

**自动化运维**
```bash
#!/bin/bash
# Service健康检查脚本

NAMESPACE=${1:-default}
SERVICE_NAME=$2

if [ -z "$SERVICE_NAME" ]; then
    echo "Usage: $0 <namespace> <service-name>"
    exit 1
fi

# 检查Service是否存在
if ! kubectl get svc $SERVICE_NAME -n $NAMESPACE >/dev/null 2>&1; then
    echo "ERROR: Service $SERVICE_NAME not found in namespace $NAMESPACE"
    exit 1
fi

# 检查Endpoints
ENDPOINTS=$(kubectl get endpoints $SERVICE_NAME -n $NAMESPACE -o jsonpath='{.subsets[*].addresses[*].ip}' | wc -w)
if [ $ENDPOINTS -eq 0 ]; then
    echo "WARNING: No endpoints available for service $SERVICE_NAME"
    exit 1
fi

# 检查负载均衡器状态
LB_STATUS=$(kubectl get svc $SERVICE_NAME -n $NAMESPACE -o jsonpath='{.status.loadBalancer.ingress[*].ip}')
if [ -z "$LB_STATUS" ]; then
    echo "WARNING: Load balancer not assigned to service $SERVICE_NAME"
    exit 1
fi

echo "OK: Service $SERVICE_NAME is healthy"
echo "Endpoints: $ENDPOINTS"
echo "Load Balancer: $LB_STATUS"
```

### 10.3 性能基准

**推荐配置基准**
- 小型应用：1-10个Service，ClusterIP为主
- 中型应用：10-100个Service，混合使用各种类型
- 大型应用：100+个Service，主要使用LoadBalancer

**资源建议**
- 每个Service：约0.1-0.5 CPU，64-256MB内存
- kube-proxy：每节点50-200m CPU，128-512MB内存
- CoreDNS：每实例100-500m CPU，128-1GB内存

---