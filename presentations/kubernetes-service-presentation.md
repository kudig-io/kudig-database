# Kubernetes Service 从入门到实战

> **适用环境**: 阿里云专有云 & 公共云 | **重点产品**: ACK | **版本**: Kubernetes v1.25-v1.32  
> **文档类型**: PPT演示文稿内容 | **目标受众**: 开发者、运维工程师、架构师  

---

## 目录

1. [Service 基础概念](#1-service-基础概念)
2. [Service 类型详解](#2-service-类型详解)
3. [Service 工作原理](#3-service-工作原理)
4. [阿里云环境实践](#4-阿里云环境实践)
5. [ACK 产品集成](#5-ack-产品集成)
6. [高级特性与最佳实践](#6-高级特性与最佳实践)
7. [故障排查与监控](#7-故障排查与监控)
8. [总结与Q&A](#8-总结与qa)

---

## 1. Service 基础概念

### 1.1 什么是 Service？

**核心定义**
- Kubernetes 中为 Pod 提供稳定网络访问入口的抽象
- 解耦服务消费者和服务提供者
- 自动负载均衡和故障转移

**关键特性**
- 稳定的虚拟IP (ClusterIP)
- 服务发现 (DNS)
- 负载均衡
- 生命周期独立于Pod

### 1.2 为什么需要 Service？

**Pod 的挑战**
```
❌ Pod IP 动态变化
❌ 直连Pod不安全
❌ 缺乏负载均衡
❌ 无法服务发现
```

**Service 的价值**
```
✅ 稳定访问入口
✅ 自动服务发现
✅ 内建负载均衡
✅ 流量治理能力
```

### 1.3 Service 与 Pod 的关系

```
[客户端] → [Service] → [Endpoint] → [Pod1]
                    ↘ [Pod2]
                    ↘ [Pod3]
```

**核心概念**
- **Service**: 虚拟服务对象
- **Endpoints**: 后端Pod的实际地址集合
- **Selector**: 标签选择器，关联Pod

---

## 2. Service 类型详解

### 2.1 四种 Service 类型对比

| 类型 | 访问范围 | 使用场景 | 特点 |
|------|----------|----------|------|
| **ClusterIP** | 集群内部 | 内部服务通信 | 默认类型，最安全 |
| **NodePort** | 集群外部 | 开发测试环境 | 每个节点开放端口 |
| **LoadBalancer** | 集群外部 | 生产环境 | 云厂商负载均衡器 |
| **ExternalName** | 集群内外 | 外部服务引用 | CNAME记录 |

### 2.2 ClusterIP (默认类型)

**特点**
- 仅集群内部可访问
- 自动分配虚拟IP
- 最安全的服务暴露方式

**YAML 示例**
```yaml
apiVersion: v1
kind: Service
metadata:
  name: my-app-service
spec:
  selector:
    app: my-app
  ports:
    - protocol: TCP
      port: 80
      targetPort: 8080
  type: ClusterIP
```

### 2.3 NodePort

**特点**
- 通过节点IP:端口访问
- 端口范围: 30000-32767
- 适用于开发测试

**YAML 示例**
```yaml
apiVersion: v1
kind: Service
metadata:
  name: my-app-nodeport
spec:
  selector:
    app: my-app
  ports:
    - protocol: TCP
      port: 80
      targetPort: 8080
      nodePort: 30080
  type: NodePort
```

### 2.4 LoadBalancer (云环境重点)

**特点**
- 云厂商提供外部负载均衡器
- 自动生成公网IP
- 生产环境首选

**阿里云ACK示例**
```yaml
apiVersion: v1
kind: Service
metadata:
  name: my-app-lb
  annotations:
    # 阿里云负载均衡器配置
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-id: "lb-xxxxxx"
spec:
  selector:
    app: my-app
  ports:
    - protocol: TCP
      port: 80
      targetPort: 8080
  type: LoadBalancer
```

### 2.5 ExternalName

**特点**
- 将服务映射到外部DNS名
- 不需要selector
- 适用于集成外部服务

**示例**
```yaml
apiVersion: v1
kind: Service
metadata:
  name: external-database
spec:
  type: ExternalName
  externalName: database.example.com
```

---

## 3. Service 工作原理

### 3.1 核心组件架构

```
[Service对象] 
    ↓
[kube-apiserver] 
    ↓
[kube-proxy] (每个节点)
    ↓
[iptables/IPVS] 
    ↓
[Pods]
```

### 3.2 kube-proxy 三种模式

#### iptables 模式
```
优点: 简单可靠，兼容性好
缺点: 规则多时性能下降
适用: 小规模集群
```

#### IPVS 模式
```
优点: 高性能，支持更多算法
缺点: 需要内核支持
适用: 大规模生产环境
```

#### nftables 模式 (v1.29+)
```
优点: 新一代规则引擎，性能更好
缺点: 较新，生态待完善
适用: 最新版K8s集群
```

### 3.3 服务发现机制

**DNS 解析流程**
```
my-service.default.svc.cluster.local
    ↑
[CoreDNS]
    ↑
[Endpoints]
    ↑
[Pod IPs]
```

**环境变量注入**
```bash
# Pod内自动注入的环境变量
MY_APP_SERVICE_HOST=10.96.0.10
MY_APP_SERVICE_PORT=80
```

---

## 4. 阿里云环境实践

### 4.1 专有云 vs 公共云差异

| 特性 | 专有云 (Apsara Stack) | 公共云 (ACK) |
|------|---------------------|-------------|
| 网络环境 | 私有网络 | 公网+私网 |
| 负载均衡 | SLB内网 | SLB公网/内网 |
| 安全管控 | 本地化策略 | 云安全中心 |
| 运维模式 | 本地运维 | 托管运维 |

### 4.2 网络规划建议

**专有云环境**
```yaml
# 推荐网络配置
VPC网段: 10.0.0.0/8
Pod网段: 172.20.0.0/16
Service网段: 172.21.0.0/16
```

**公共云环境**
```yaml
# ACK推荐配置
VPC: 自动创建或复用现有
Pod CIDR: 172.20.0.0/16
Service CIDR: 172.21.0.0/20
```

### 4.3 负载均衡器选择

**CLB (传统型负载均衡)**
```
适用场景: TCP/UDP协议
优势: 成熟稳定，成本较低
限制: 不支持HTTP高级特性
```

**NLB (网络型负载均衡)**
```
适用场景: 高性能TCP/UDP
优势: 超低延迟，超高并发
限制: 仅支持四层协议
```

**ALB (应用型负载均衡)**
```
适用场景: HTTP/HTTPS应用
优势: 七层路由，丰富特性
限制: 成本相对较高
```

---

## 5. ACK 产品集成

### 5.1 Service 注解配置

**基础负载均衡配置**
```yaml
apiVersion: v1
kind: Service
metadata:
  name: ack-service
  annotations:
    # 指定负载均衡器实例
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-id: "lb-xxxxxxxxx"
    
    # 负载均衡规格
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-spec: "slb.s1.small"
    
    # 带宽设置
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-bandwidth: "100"
spec:
  selector:
    app: my-app
  ports:
    - protocol: TCP
      port: 80
      targetPort: 8080
  type: LoadBalancer
```

**高级网络配置**
```yaml
apiVersion: v1
kind: Service
metadata:
  name: advanced-service
  annotations:
    # 指定可用区
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-zone-id: "cn-hangzhou-a"
    
    # 启用删除保护
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-delete-protection: "on"
    
    # 修改保护
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-modification-protection: "ConsoleProtection"
    
    # 健康检查配置
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-health-check-flag: "on"
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-health-check-type: "tcp"
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-health-check-connect-port: "8080"
spec:
  selector:
    app: my-app
  ports:
    - protocol: TCP
      port: 80
      targetPort: 8080
  type: LoadBalancer
```

### 5.2 多协议支持

**TCP/UDP 负载均衡**
```yaml
apiVersion: v1
kind: Service
metadata:
  name: tcp-udp-service
  annotations:
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-protocol-port: "tcp:80,udp:53"
spec:
  selector:
    app: mixed-protocol-app
  ports:
    - name: http
      protocol: TCP
      port: 80
      targetPort: 8080
    - name: dns
      protocol: UDP
      port: 53
      targetPort: 53
  type: LoadBalancer
```

### 5.3 安全组集成

**绑定安全组**
```yaml
apiVersion: v1
kind: Service
metadata:
  name: secure-service
  annotations:
    # 绑定安全组
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-security-group-id: "sg-xxxxxxxxx"
    
    # 允许访问的CIDR
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-access-control-enable: "on"
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-access-control-type: "white"
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-access-control-list: "192.168.0.0/16,10.0.0.0/8"
spec:
  selector:
    app: secure-app
  ports:
    - protocol: TCP
      port: 443
      targetPort: 8443
  type: LoadBalancer
```

---

## 6. 高级特性与最佳实践

### 6.1 会话亲和性 (Session Affinity)

**基于客户端IP的会话保持**
```yaml
apiVersion: v1
kind: Service
metadata:
  name: session-affinity-service
spec:
  selector:
    app: web-app
  ports:
    - protocol: TCP
      port: 80
      targetPort: 8080
  sessionAffinity: ClientIP
  sessionAffinityConfig:
    clientIP:
      timeoutSeconds: 10800  # 3小时超时
```

### 6.2 拓扑感知路由

**区域感知负载均衡 (v1.21+)**
```yaml
apiVersion: v1
kind: Service
metadata:
  name: topology-aware-service
spec:
  selector:
    app: app
  ports:
    - protocol: TCP
      port: 80
      targetPort: 8080
  topologyKeys:
    - "kubernetes.io/hostname"
    - "topology.kubernetes.io/zone"
    - "*"
```

### 6.3 Headless Service

**无头服务 - 直接访问Pod**
```yaml
apiVersion: v1
kind: Service
metadata:
  name: headless-service
spec:
  selector:
    app: database
  ports:
    - protocol: TCP
      port: 9090
      targetPort: 9090
  clusterIP: None  # 关键配置
```

**使用场景**
- StatefulSet应用
- 自定义服务发现
- 直接Pod访问需求

### 6.4 ExternalTrafficPolicy

**保留客户端源IP**
```yaml
apiVersion: v1
kind: Service
metadata:
  name: preserve-source-ip
spec:
  selector:
    app: app
  ports:
    - protocol: TCP
      port: 80
      targetPort: 8080
  type: LoadBalancer
  externalTrafficPolicy: Local  # 保留源IP
```

### 6.5 健康检查配置

**HTTP 健康检查**
```yaml
apiVersion: v1
kind: Service
metadata:
  name: health-check-service
  annotations:
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-health-check-flag: "on"
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-health-check-type: "http"
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-health-check-uri: "/health"
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-health-check-http-code: "http_2xx"
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-health-check-timeout: "5"
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-health-check-interval: "2"
spec:
  selector:
    app: healthy-app
  ports:
    - protocol: TCP
      port: 80
      targetPort: 8080
  type: LoadBalancer
```

---

## 7. 故障排查与监控

### 7.1 常见问题诊断

**Service 无法访问排查清单**

1. **检查Service配置**
```bash
kubectl get svc <service-name> -o yaml
kubectl describe svc <service-name>
```

2. **验证Endpoints**
```bash
kubectl get endpoints <service-name>
kubectl get pods -l <selector-labels>
```

3. **测试网络连通性**
```bash
# 在Pod内测试
kubectl exec -it <pod-name> -- curl <service-ip>:<port>

# 集群内测试
kubectl run debug --image=busybox --restart=Never --rm -it -- sh
```

### 7.2 kube-proxy 状态检查

**查看kube-proxy日志**
```bash
kubectl logs -n kube-system -l k8s-app=kube-proxy
```

**检查iptables规则**
```bash
# 登录节点检查
iptables-save | grep <service-name>
```

### 7.3 阿里云监控集成

**关键监控指标**
- Service连接数
- 后端Pod健康状态
- 负载均衡器状态
- 网络流量统计

**云监控配置**
```yaml
# 启用详细的监控
apiVersion: v1
kind: Service
metadata:
  name: monitored-service
  annotations:
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-enable-access-log: "on"
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-access-log-bucket: "slb-access-logs"
spec:
  # ... 其他配置
```

### 7.4 故障排除命令速查

```bash
# 查看所有Services
kubectl get services --all-namespaces

# 查看Service详细信息
kubectl describe service <service-name>

# 查看Endpoints
kubectl get endpoints <service-name>

# 测试Service DNS解析
kubectl run dns-test --image=busybox --restart=Never --rm -it -- nslookup <service-name>

# 查看kube-proxy状态
kubectl get daemonset kube-proxy -n kube-system

# 检查网络插件状态
kubectl get pods -n kube-system -l k8s-app=<cni-plugin-name>
```

---

## 8. 总结与Q&A

### 8.1 核心要点回顾

**Service 的价值**
- ✅ 提供稳定的网络访问入口
- ✅ 自动负载均衡和故障转移
- ✅ 内建服务发现机制
- ✅ 支持多种访问模式

**阿里云环境最佳实践**
- 🎯 专有云使用内网SLB
- 🎯 公共云根据需求选择CLB/NLB/ALB
- 🎯 合理配置安全组和访问控制
- 🎯 启用监控和日志收集

### 8.2 常见问题解答

**Q: Service IP冲突怎么办？**
A: 检查Service CIDR配置，确保不与其他网络段冲突

**Q: 如何优化Service性能？**
A: 使用IPVS模式，合理设置sessionAffinity，启用拓扑感知

**Q: 专有云环境下如何配置外部访问？**
A: 通过NodePort或配置内网SLB实现

**Q: 如何实现蓝绿部署？**
A: 结合Ingress和Service权重配置实现流量切换

### 8.3 学习资源推荐

**官方文档**
- Kubernetes Service文档: https://kubernetes.io/docs/concepts/services-networking/service/
- 阿里云ACK文档: https://help.aliyun.com/product/85222.html

**相关技术**
- Ingress控制器配置
- NetworkPolicy网络安全
- Service Mesh服务网格

---# Kubernetes Service ACK 补充技术文�?

## 3.2 负载均衡器选择策略（续�?

### 3.2.2 NLB (网络型负载均�?

**性能优势**
- 超低延迟 (<1ms)
- 超高并发 (百万级连�?
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

### 3.2.3 ALB (应用型负载均�?

**七层特�?*
- HTTP/HTTPS协议支持
- 基于内容的路�?
- 丰富的安全特�?

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

### 4.1 安全组集成配�?

**精细化安全控�?*
```yaml
apiVersion: v1
kind: Service
metadata:
  name: secure-service
  annotations:
    # 绑定安全�?
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

**跨AZ高可用配�?*
```yaml
apiVersion: v1
kind: Service
metadata:
  name: multi-az-service
  annotations:
    # 多可用区部署
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-master-zone-id: "cn-hangzhou-a"
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-slave-zone-id: "cn-hangzhou-b"
    
    # 健康检查增�?
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

## 5. 生产级配置模�?

### 5.1 标准Web服务配置

```yaml
apiVersion: v1
kind: Service
metadata:
  name: production-web-service
  namespace: production
  annotations:
    # 负载均衡器配�?
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-id: "lb-xxxxxxxxx"
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-spec: "slb.s2.medium"
    
    # 网络配置
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-bandwidth: "200"
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-charge-type: "paybybandwidth"
    
    # 安全配置
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-security-group-id: "sg-web-prod"
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-delete-protection: "on"
    
    # 健康检�?
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
    
    # 连接池优�?
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

## 6. 性能优化与调�?

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
# 增加conntrack表大�?
echo "net.netfilter.nf_conntrack_max = 1048576" >> /etc/sysctl.conf
echo "net.netfilter.nf_conntrack_buckets = 262144" >> /etc/sysctl.conf

# 调整TCP参数
echo "net.ipv4.tcp_fin_timeout = 30" >> /etc/sysctl.conf
echo "net.ipv4.tcp_keepalive_time = 1200" >> /etc/sysctl.conf
echo "net.core.somaxconn = 65535" >> /etc/sysctl.conf

sysctl -p
```

### 6.2 负载均衡器优�?

**连接复用配置**
```yaml
apiVersion: v1
kind: Service
metadata:
  name: optimized-service
  annotations:
    # 连接池配�?
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
    
    # HTTP重定�?
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

1. **检查Service状�?*
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

# 检查Pod状�?
kubectl get pods -l <selector-labels>

# 验证Pod就绪状�?
kubectl get pods -l <selector-labels> -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{.status.phase}{"\t"}{.status.conditions[?(@.type=="Ready")].status}{"\n"}{end}'
```

3. **网络连通性测�?*
```bash
# 在集群内测试
kubectl run debug --image=busybox --rm -it -- sh
# 在Pod内执�?
nslookup <service-name>.<namespace>
telnet <service-ip> <port>

# 测试负载均衡�?
curl -v http://<load-balancer-ip>:<port>
```

### 9.2 阿里云特定问�?

**负载均衡器相关问�?*

```bash
# 检查SLB实例状�?
aliyun slb DescribeLoadBalancers --LoadBalancerId lb-xxxxxxxxx

# 查看后端服务器状�?
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
- Service名称：`<应用�?-<环境>-svc`
- 端口名称：`<协议>-<用�?` (�?http-api, https-web)
- Annotation前缀：使用标准阿里云注解

**标签规范**
```yaml
metadata:
  labels:
    app: <应用名称>
    version: <版本�?
    env: <环境标识>
    tier: <层级标识>
```

### 10.2 运维建议

**定期检查清�?*
- [ ] Service健康状态监�?
- [ ] Endpoints可用性检�?
- [ ] 负载均衡器性能指标
- [ ] 安全组规则审�?
- [ ] SSL证书有效期检�?
- [ ] 访问日志分析

**自动化运�?*
```bash
#!/bin/bash
# Service健康检查脚�?

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

# 检查负载均衡器状�?
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
- 小型应用�?-10个Service，ClusterIP为主
- 中型应用�?0-100个Service，混合使用各种类�?
- 大型应用�?00+个Service，主要使用LoadBalancer

**资源建议**
- 每个Service：约0.1-0.5 CPU�?4-256MB内存
- kube-proxy：每节点50-200m CPU�?28-512MB内存
- CoreDNS：每实例100-500m CPU�?28-1GB内存

---
