---
title: 阿里云SLB与Ingress
description: 阿里云SLB类型、ACK Ingress Controller配置、健康检查与多可用区高可用
summary: SLB/ALB/NLB负载均衡与ACK Ingress Controller的配置与故障处理。
category: cloud-provider
tags:
- alibaba-cloud
- ack
- slb
- ingress
- load-balancer
- alb
- nlb
- high-availability
- multi-az
tier: core
sources:
- 阿里云SLB产品文档
- ACK Ingress最佳实践
created: 2026-05-21
updated: 2026-05-21
last_updated: 2026-05-21
relationships:
- target: '[[domain-17-system-foundation/topic-dictionary/networking/ingress.md]]'
  type: uses
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# 阿里云SLB与Ingress

本文档覆盖阿里云专有云 ACK 集群的负载均衡与 [[domain-17-system-foundation/topic-dictionary/networking/ingress.md|Ingress]] 体系：SLB 类型选择、ACK Ingress Controller 配置、后端健康检查机制以及多可用区高可用架构。面向远程顾问场景，所有配置均可通过工单指导客户完成。

---

## 1. SLB类型

### 1.1 SLB分类体系

阿里云负载均衡在专有云中的类型映射：

| 类型 | 公网/内网 | 性能规格 | 协议支持 | 七层能力 | 专有云对应 |
|------|----------|----------|----------|----------|------------|
| **CLB（传统SLB）** | 公网/内网 | 标准/性能型 | TCP/UDP/HTTP/HTTPS | 基础 | SLB |
| **ALB** | 公网/内网 | 自动弹性 | HTTP/HTTPS/gRPC | 高级 | 部分支持 |
| **NLB** | 公网/内网 | 自动弹性 | TCP/UDP | 无 | 部分支持 |
| **金属LB** | 内网 | 物理设备 | L4/L7 | 视设备 | 专有云特有 |

### 1.2 公网SLB vs 内网SLB

```
公网SLB场景（专有云少见）:
Internet → EIP → SLB(公网) → ECS节点 → Pod

内网SLB场景（专有云主流）:
客户端内网 → 内网SLB → ECS节点 → Pod
                ↑
         通过客户内部网络访问
```

**专有云SLB限制**：

| 限制项 | 说明 | 应对方案 |
|--------|------|----------|
| 无公网EIP | 专无外网出口 | 使用内网SLB + 客户DMZ |
| SLB规格有限 | 物理设备容量 | 提前规划并发/吞吐 |
| 证书管理 | 需客户CA签发 | 对接客户PKI体系 |
| 健康检查源IP | 固定网段 | 安全组放行对应网段 |

```yaml
# 内网SLB Service 示例
apiVersion: v1
kind: Service
metadata:
  name: internal-app
  annotations:
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-address-type: "intranet"
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-spec: "slb.s1.small"
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-id: "lb-apsara-xxx"
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-force-override-listeners: "true"
spec:
  type: LoadBalancer
  selector:
    app: internal-app
  ports:
    - port: 80
      targetPort: 8080
      protocol: TCP
  sessionAffinity: ClientIP
  sessionAffinityConfig:
    clientIP:
      timeoutSeconds: 10800
```

### 1.3 性能型 vs 标准型

| 维度 | 标准型 | 性能型 |
|------|--------|--------|
| 最大连接数 | 5000 | 50万+ |
| 每秒新建连接 | 3000 | 10万+ |
| 吞吐量 | 小型 | 大型 |
| 适用场景 | 开发测试 | 生产高并发 |
| 专有云可用 | 是 | 视底座规格 |

```bash
# 远程检查SLB规格
aliyun slb DescribeLoadBalancerAttribute \
  --LoadBalancerId lb-apsara-xxx \
  --RegionId cn-apsara-local

# 检查监听状态
aliyun slb DescribeLoadBalancerListeners \
  --LoadBalancerId lb-apsara-xxx \
  --RegionId cn-apsara-local
```

---

## 2. ACK Ingress Controller配置

### 2.1 Ingress-Nginx Controller

ACK 专有版默认提供 Ingress-Nginx Controller：

```yaml
# Ingress Nginx Controller 部署
apiVersion: apps/v1
kind: Deployment
metadata:
  name: nginx-ingress-controller
  namespace: kube-system
spec:
  replicas: 2
  selector:
    matchLabels:
      app: nginx-ingress-controller
  template:
    metadata:
      labels:
        app: nginx-ingress-controller
    spec:
      serviceAccountName: nginx-ingress-serviceaccount
      containers:
        - name: nginx-ingress-controller
          image: registry-vpc.cn-apsara-local.aliyuncs.com/acs/aliyun-ingress-controller:v1.8.0
          args:
            - /nginx-ingress-controller
            - --election-id=ingress-nginx-leader
            - --controller-class=k8s.io/ingress-nginx
            - --ingress-class=nginx
            - --configmap=$(POD_NAMESPACE)/ingress-nginx-controller
            - --validating-webhook=:8443
            - --validating-webhook-certificate=/usr/local/certificates/cert
            - --validating-webhook-key=/usr/local/certificates/key
            - --publish-service=$(POD_NAMESPACE)/ingress-nginx
            - --annotations-prefix=nginx.ingress.kubernetes.io
          securityContext:
            allowPrivilegeEscalation: false
            readOnlyRootFilesystem: true
            runAsNonRoot: true
          resources:
            requests:
              cpu: 500m
              memory: 512Mi
            limits:
              cpu: 2
              memory: 2Gi
          ports:
            - name: http
              containerPort: 80
            - name: https
              containerPort: 443
            - name: webhook
              containerPort: 8443
---
# Ingress Nginx Service（关联SLB）
apiVersion: v1
kind: Service
metadata:
  name: ingress-nginx
  namespace: kube-system
  annotations:
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-address-type: "intranet"
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-spec: "slb.s2.medium"
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-id: "lb-apsara-xxx"
spec:
  type: LoadBalancer
  selector:
    app: nginx-ingress-controller
  ports:
    - name: http
      port: 80
      targetPort: 80
    - name: https
      port: 443
      targetPort: 443
```

### 2.2 Ingress 规则配置

```yaml
# 基础Ingress规则
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: app-ingress
  namespace: default
  annotations:
    nginx.ingress.kubernetes.io/ssl-redirect: "true"
    nginx.ingress.kubernetes.io/proxy-body-size: "100m"
    nginx.ingress.kubernetes.io/proxy-connect-timeout: "30"
    nginx.ingress.kubernetes.io/proxy-read-timeout: "60"
    nginx.ingress.kubernetes.io/proxy-send-timeout: "60"
    nginx.ingress.kubernetes.io/rewrite-target: /
    nginx.ingress.kubernetes.io/rate-limit: "100"
spec:
  ingressClassName: nginx
  tls:
    - hosts:
        - app.corp.internal
      secretName: app-tls-secret
  rules:
    - host: app.corp.internal
      http:
        paths:
          - path: /
            pathType: Prefix
            backend:
              service:
                name: app-service
                port:
                  number: 8080
---

## 3. SLB后端健康检查

### 3.1 健康检查机制

```
┌─────────┐      health check       ┌─────────┐      readiness      ┌─────┐
│   SLB   │  ────────────────────→  │  Node   │  ────────────────→  │ Pod │
│         │  TCP/HTTP/HTTPS probe   │         │      probe          │     │
└─────────┘                         └─────────┘                     └─────┘
   ↑                                                                  ↑
   └──────────────────────── 业务流量 ────────────────────────────────┘
```

**三层健康检查体系**：

| 层级 | 检查目标 | 检查方式 | 失败处理 |
|------|----------|----------|----------|
| SLB层 | ECS节点+端口 | TCP/HTTP | 移除后端 |
| Node层 | Kube-proxy | iptables规则 | 自动切换 |
| Pod层 | 容器就绪状态 | ReadinessProbe | 从Endpoint移除 |

### 3.2 健康检查配置

```yaml
# Service + 健康检查配置
apiVersion: v1
kind: Service
metadata:
  name: health-checked-app
  annotations:
    # SLB健康检查协议
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-health-check-type: "http"
    # 健康检查路径
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-health-check-uri: "/health"
    # 健康检查端口
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-health-check-port: "8080"
    # 健康检查间隔（秒）
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-health-check-interval: "5"
    # 健康检查超时（秒）
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-health-check-timeout: "3"
    # 健康检查失败阈值
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-health-check-unhealthy-threshold: "3"
    # 健康检查成功阈值
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-health-check-healthy-threshold: "2"
    # 健康检查正常HTTP状态码
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-health-check-http-code: "http_2xx"
spec:
  type: LoadBalancer
  selector:
    app: health-checked-app
  ports:
    - port: 80
      targetPort: 8080
---
# Pod ReadinessProbe
apiVersion: apps/v1
kind: Deployment
metadata:
  name: health-checked-app
spec:
  replicas: 3
  selector:
    matchLabels:
      app: health-checked-app
  template:
    metadata:
      labels:
        app: health-checked-app
    spec:
      containers:
        - name: app
          image: harbor.corp.internal/app:v1
          ports:
            - containerPort: 8080
          readinessProbe:
            httpGet:
              path: /health
              port: 8080
              httpHeaders:
                - name: X-Health-Check
                  value: "true"
            initialDelaySeconds: 10
            periodSeconds: 5
            timeoutSeconds: 3
            successThreshold: 1
            failureThreshold: 3
          livenessProbe:
            httpGet:
              path: /health
              port: 8080
            initialDelaySeconds: 30
            periodSeconds: 10
            timeoutSeconds: 5
            failureThreshold: 3
```

### 3.3 健康检查排查

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 健康检查远程诊断
aliyun slb DescribeHealthStatus --LoadBalancerId lb-apsara-xxx --RegionId cn-apsara-local
kubectl get pods -l app=health-checked-app -o wide
kubectl get endpoints health-checked-app -o yaml
kubectl describe pods -l app=health-checked-app | grep -A 5 "Events:"
kubectl run -it --rm test --image=busybox:1.36 --restart=Never -- \
  wget -qO- http://health-checked-app.default.svc.cluster.local:80/health
kubectl logs -n kube-system -l k8s-app=kube-proxy --tail=50
```
---

## 4. 多可用区SLB与高可用

### 4.1 多可用区架构

```
                    ┌─────────────────┐
                    │   内网SLB       │
                    │  (主可用区A)    │
                    │  (备可用区B)    │
                    └────────┬────────┘
                             │
            ┌────────────────┼────────────────┐
            │                │                │
     ┌──────┴──────┐  ┌─────┴──────┐  ┌─────┴──────┐
     │  可用区A    │  │  可用区B   │  │  可用区C   │
     │             │  │            │  │            │
     │ ┌─────────┐ │  │ ┌────────┐ │  │ ┌────────┐ │
     │ │Node 1   │ │  │ │Node 2  │ │  │ │Node 3  │ │
     │ │Node 2   │ │  │ │Node 3  │ │  │ │Node 4  │ │
     │ └─────────┘ │  │ └────────┘ │  │ └────────┘ │
     └─────────────┘  └────────────┘  └────────────┘
```

### 4.2 多可用区Service配置

```yaml
# 多可用区负载均衡Service
apiVersion: v1
kind: Service
metadata:
  name: multi-az-app
  annotations:
    # 指定主备可用区
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-master-zone: "cn-apsara-local-a"
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-slave-zone: "cn-apsara-local-b"
    # 多可用区调度策略
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-scheduler: "wrr"
    # 跨可用区后端调度
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-cross-zone-loadbalance: "on"
spec:
  type: LoadBalancer
  selector:
    app: multi-az-app
  ports:
    - port: 80
      targetPort: 8080
  sessionAffinity: None
---
# Pod 反亲和性：确保跨AZ分布
apiVersion: apps/v1
kind: Deployment
metadata:
  name: multi-az-app
spec:
  replicas: 6
  selector:
    matchLabels:
      app: multi-az-app
  template:
    metadata:
      labels:
        app: multi-az-app
    spec:
      affinity:
        podAntiAffinity:
          preferredDuringSchedulingIgnoredDuringExecution:
            - weight: 100
              podAffinityTerm:
                labelSelector:
                  matchExpressions:
                    - key: app
                      operator: In
                      values:
                        - multi-az-app
                topologyKey: topology.kubernetes.io/zone
      containers:
        - name: app
          image: harbor.corp.internal/app:v1
          ports:
            - containerPort: 8080
```

### 4.3 高可用检查清单

| 检查项 | 命令/方法 | 预期结果 |
|--------|-----------|----------|
| 多AZ节点分布 | `kubectl get nodes -L topology.kubernetes.io/zone` | 至少2个AZ有节点 |
| Pod跨AZ分布 | `kubectl get pods -o wide` | Pod分布在不同AZ |
| SLB多AZ配置 | `aliyun slb DescribeLoadBalancerAttribute` | MasterZoneId != SlaveZoneId |
| 后端健康状态 | `aliyun slb DescribeHealthStatus` | 所有后端normal |
| 故障转移测试 | 模拟AZ问题 | 流量自动切换 |
| 会话保持 | Service sessionAffinity配置 | 根据业务需要配置 |

### 4.4 故障切换流程

> **🔴 高风险操作警告**
>
> 下方命令属于不可逆或高影响操作，执行前请确认：
> - 已备份关键数据与配置
> - 处于批准的变更窗口期
> - 已获得相关责任人授权
> - 已准备回滚或恢复方案
> - 目标集群、Namespace、节点/资源名称正确无误

``` bash
# 🔴 高风险：可能造成数据丢失或服务中断，执行前需备份、变更审批与回滚方案
# AZ故障切换
kubectl cordon $(kubectl get nodes -l topology.kubernetes.io/zone=cn-apsara-local-a -o name)
kubectl drain $(kubectl get nodes -l topology.kubernetes.io/zone=cn-apsara-local-a -o name) \
  --ignore-daemonsets --delete-emptydir-data --force
kubectl get pods -l app=multi-az-app -o wide
aliyun slb DescribeHealthStatus --LoadBalancerId lb-apsara-xxx --RegionId cn-apsara-local
kubectl uncordon $(kubectl get nodes -l topology.kubernetes.io/zone=cn-apsara-local-a -o name)
```
---

## 5. 远程诊断流程

### 5.1 Ingress问题排查

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
#!/bin/bash
# Ingress 远程诊断
kubectl get pods -n kube-system -l app=nginx-ingress-controller
kubectl logs -n kube-system -l app=nginx-ingress-controller --tail=200
kubectl get ingress -A
kubectl get endpoints -A
kubectl get secrets -A | grep tls
kubectl run -it --rm test --image=busybox:1.36 --restart=Never -- \
  wget -qO- http://ingress-nginx.kube-system.svc.cluster.local/healthz
```
### 5.2 常见SLB/Ingress问题

| 症状 | 根因 | 远程处理 |
|------|------|----------|
| 404/502 | 后端Pod未就绪 | 检查ReadinessProbe、Pod日志 |
| 证书错误 | TLS Secret过期/不匹配 | 检查证书有效期、Secret配置 |
| 访问超时 | 安全组/健康检查失败 | 检查安全组规则、健康检查路径 |
| 流量不均 | 会话保持/权重配置 | 调整调度算法、权重 |
| 单点问题 | 单AZ部署 | 启用多AZ、Pod反亲和性 |
| Ingress不生效 | IngressClass未匹配 | 检查ingressClassName |
| 域名不通 | DNS未解析 | 检查内部DNS、CoreDNS |

---

## 相关文档

- [[domain-12-cloud-providers/01-alibaba-cloud/01-专有云架构概述.md|专有云架构概述]]
- [[domain-12-cloud-providers/01-alibaba-cloud/02-ACK集群运维.md|ACK集群运维]]
- [[domain-12-cloud-providers/01-alibaba-cloud/03-Terway-CNI网络.md|Terway-CNI网络]]
- [[domain-12-cloud-providers/01-alibaba-cloud/04-阿里云存储集成.md|阿里云存储集成]]
- [[domain-12-cloud-providers/01-alibaba-cloud/06-阿里云专有云远程顾问指南.md|阿里云专有云远程顾问指南]]
- [[241-ack-slb-nlb-alb|ACK SLB/NLB/ALB]]
- [[alicloud-ack-overview|阿里云ACK概述]]
## Related

- [[entities/coredns.md|CoreDNS (entities)]]
- [[entities/deployment.md|Deployment]]
- [[entities/kubernetes.md|Kubernetes (CNCF Graduated)]]
- [[domain-17-system-foundation/topic-dictionary/fundamentals/nodes.md|Nodes（节点）]]


<!-- risk-assessed -->
