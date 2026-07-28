---
title: 'Day 22: Service 基础实操'
description: '**日期**: Week 4 Day 1 | **主题**: Service 类型与配置实践 | **版本**: K8s 1.28-1.33'
summary: '**日期**: Week 4 Day 1 | **主题**: Service 类型与配置实践 | **版本**: K8s 1.28-1.33'
category: learning
tags:
- k8s
- training
- hands-on
- coredns
- mysql
- statefulset
- ingress
- rag
- cilium
- flannel
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 'Day 22: Service 基础实操 是什么'
- '如何 Day 22: Service 基础实操'
trigger_keywords:
- Day
- '22:'
- Service
- 基础实操
- learn
prerequisites:
- kubectl-basics
- gpu-ml-basics
- ebpf-basics
- cilium-basics
- cni-basics
- mysql-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Day 22: [[service|Service]] 基础实操

> **日期**: Week 4 Day 1 | **主题**: Service 类型与配置实践 | **版本**: K8s 1.28-1.33

---

## 1. Service 核心概念

### 1.1 Service 类型

| 类型 | ClusterIP | 适用场景 |
|------|-----------|---------|
| `ClusterIP` | 集群内部 IP | 内部服务间调用 |
| `NodePort` | <nodeIP>:<port> | 开发/测试环境 |
| `LoadBalancer` | 外部负载均衡 IP | 生产环境（配合云厂商） |
| `ExternalName` | CNAME 映射 | 访问外部服务 |

### 1.2 Service 工作原理

```
Pod A → Service (10.96.0.1) → Endpoints (Pod B:8080, Pod C:8080)
                        ↑
                   kube-proxy (iptables/ipvs)
```

---

## 2. 创建 Service

### 2.1 ClusterIP Service

```yaml
# 基本 ClusterIP
apiVersion: v1
kind: Service
metadata:
  name: backend-svc
  namespace: production
spec:
  type: ClusterIP
  selector:
    app: backend
  ports:
    - name: http
      port: 80
      targetPort: 8080
    - name: grpc
      port: 50051
      targetPort: 50051
```

### 2.2 NodePort Service

```yaml
apiVersion: v1
kind: Service
metadata:
  name: web-nodeport
spec:
  type: NodePort
  selector:
    app: web
  ports:
    - name: http
      port: 80
      targetPort: 8080
      nodePort: 30080  # 可选，默认 30000-32767
```

### 2.3 Headless Service

```yaml
# 无负载均衡，直接返回 Pod IP
apiVersion: v1
kind: Service
metadata:
  name: statefulset-svc
spec:
  type: ClusterIP
  clusterIP: None  # Headless
  selector:
    app: mysql
  ports:
    - port: 3306
      targetPort: 3306
```

---

## 3. Service 发现

### 3.1 环境变量发现

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# Kube-proxy 在每个 Pod 中注入环境变量
# 自动生成: {SVCNAME}_SERVICE_HOST, {SVCNAME}_SERVICE_PORT
kubectl exec -it <pod-name> -- env | grep -E "_SERVICE_PORT|_SERVICE_HOST"

# 示例输出:
# BACKEND_SVC_SERVICE_HOST=10.96.0.1
# BACKEND_SVC_SERVICE_PORT=80
```
### 3.2 DNS 发现

```bash
# 集群内部 DNS 访问
# 格式: <svc-name>.<namespace>.svc.cluster.local
curl http://backend-svc.production.svc.cluster.local

# 同 namespace 可简写
curl http://backend-svc

# 跨 namespace 访问
curl http://backend-svc.other-namespace.svc.cluster.local
```

### 3.3 DNS 调试

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 测试 DNS 解析
kubectl run dnsutils --image=tutum/dnsutils --restart=Never -it -- nslookup backend-svc

# 查看 CoreDNS Pod
kubectl get pods -n kube-system -l k8s-app=kube-dns

# 查看 CoreDNS 配置
kubectl get configmap coredns -n kube-system -o yaml
```
---

## 4. Service 故障排查

### 4.1 无 Endpoints

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 1. 检查 Service 是否有 selector 匹配的 Pod
kubectl get svc backend-svc -o yaml
kubectl get pods -l app=backend

# 2. 检查 Endpoints
kubectl get endpoints backend-svc

# 3. 常见原因
# - selector 匹配不到 Pod
# - 所有 Pod 都不是 Running 状态
# - Pod 在不同 namespace
```
### 4.2 Service 无法访问

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 1. 检查 Pod 是否正常运行
kubectl get pods -l app=backend

# 2. 检查 targetPort 是否正确
kubectl describe svc backend-svc | grep -i port

# 3. 测试直接访问 Pod
kubectl exec -it <pod-a> -- curl -s http://<pod-b-ip>:8080/health

# 4. 检查 kube-proxy 状态
kubectl get pods -n kube-system -l k8s-app=kube-proxy
kubectl logs -n kube-system kube-proxy-xxx --tail=20
```
### 4.3 LoadBalancer 无外部 IP

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 1. 检查 Cloud Controller Manager
kubectl get pods -n kube-system | grep cloud

# 2. 检查 Service 事件
kubectl describe svc web-svc | grep -A10 "Events:"

# 3. 云厂商问题排查（AWS/GCE/阿里云）
# AWS: 检查 ELB 配额和安全组
# GCE: 检查 quota 和防火墙规则
```
---

## 5. Service 高级配置

### 5.1 Session Affinity

```yaml
# 客户端 IP 亲和性
apiVersion: v1
kind: Service
metadata:
  name: sticky-service
spec:
  type: ClusterIP
  sessionAffinity: ClientIP  # 同一 IP 始终路由到同一 Pod
  sessionAffinityConfig:
    clientIP:
      timeoutSeconds: 10800  # 3 小时超时
  selector:
    app: backend
  ports:
    - port: 80
      targetPort: 8080
```

### 5.2 外部流量策略

```yaml
# 只允许内部 Pod 访问
apiVersion: v1
kind: Service
metadata:
  name: internal-only
spec:
  type: ClusterIP
  externalTrafficPolicy: Local  # 保留源 IP
  selector:
    app: web
  ports:
    - port: 80
      targetPort: 8080

# externalTrafficPolicy: Cluster（默认）流量分布到所有节点
```

### 5.3 多端口 Service

```yaml
apiVersion: v1
kind: Service
metadata:
  name: multi-port-svc
spec:
  type: ClusterIP
  selector:
    app: api
  ports:
    - name: http
      port: 80
      targetPort: 8080
      protocol: TCP
    - name: https
      port: 443
      targetPort: 8443
      protocol: TCP
    - name: grpc
      port: 50051
      targetPort: 50051
      protocol: TCP
```

---

## 6. [[ingress|Ingress]] 与 Service

### 6.1 Ingress 配置

```yaml
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: web-ingress
  annotations:
    nginx.ingress.[[23-实体/02-K8s核心组件/kubernetes.md|kubernetes]].io/rewrite-target: /
spec:
  ingressClassName: nginx
  rules:
    - host: api.example.com
      http:
        paths:
          - path: /
            pathType: Prefix
            backend:
              service:
                name: web-svc
                port:
                  number: 80
```

---

## 7. 实战练习

**练习 1**: 创建 ClusterIP Service，验证 Pod 间通过 Service DNS 通信

**练习 2**: 创建 NodePort Service，通过节点 IP:nodePort 访问应用

**练习 3**: 模拟 Endpoints 为空，排查 Service 无法访问问题

**练习 4**: 配置 Session Affinity，验证同一客户端 IP 始终访问同一 Pod

---

```yaml
---
id: LEARN-WEEK4-DAY22
title: Day 22 - Service 基础实操
topic: network-storage
type: hands-on-guide
tags: [service, clusterip, nodeport, loadbalancer, dns, endpoints, hands-on, k8s-1.28-1.33]
last_updated: 2026-05-18
difficulty: intermediate
intent_queries:
  - "Service 类型有哪些"
  - "Service 怎么发现"
  - "Endpoints 为空怎么排查"
  - "Service DNS 怎么用"
  - "ClusterIP Service 配置"
trigger_keywords:
  - Service
  - ClusterIP
  - NodePort
  - LoadBalancer
  - Headless Service
  - ExternalName
  - Endpoints
  - kube-proxy
  - Session Affinity
  - externalTrafficPolicy
  - Service Discovery
  - DNS
reading_level: beginner
audience:
  - sre
  - ops-engineer
  - developer
estimated_read_time: 40min
related_domains:
  - 故障诊断
  - 网络
related_topics:
  - networking
  - service
  - dns
  - ingress
related:
  - 生产运维/topic-learn/public-training/week-4-network-storage/day-23-ingress/01-ingress-hands-on.md
  - 故障诊断/03-service-endpoints-troubleshooting.md
---
```

---

## 自测题 (Self-Check)

**1. ClusterIP 如何实现?**

<details><summary>答案</summary>

kube-proxy 通过 iptables/IPVS 将 ClusterIP DNAT 到后端 PodIP:TargetPort。

</details>

**2. Ingress vs Gateway API?**

<details><summary>答案</summary>

Ingress 仅 HTTP, 需注解扩展; Gateway API 支持 HTTP/gRPC/TCP, 原生流量分割, 角色分离。

</details>

**3. StatefulSet 稳定网络标识原理?**

<details><summary>答案</summary>

Pod 名 <sts>-<ordinal> + Headless Service → DNS <pod>.<svc>.<ns>.svc.cluster.local。

</details>

**4. 如何选 CNI?**

<details><summary>答案</summary>

Calico (通用 BGP/VXLAN) / Cilium (eBPF 高性能) / Flannel (简单无 Policy)。生产推荐 Cilium 或 Calico。

</details>

**5. PVC 三种访问模式?**

<details><summary>答案</summary>

ReadWriteOnce (单节点 RW) / ReadOnlyMany (多节点 RO) / ReadWriteMany (多节点 RW)。

</details>



<!-- risk-assessed -->
