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

---