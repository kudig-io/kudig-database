---
title: 'Day 22: Service 基础'
description: '- "kube-proxy配置"'
category: learning
tags:
- k8s
- training
- hands-on
- flannel
- coredns
- statefulset
- ingress
- rag
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 'Day 22: Service 基础 是什么'
- '如何 Day 22: Service 基础'
trigger_keywords:
- Day
- '22:'
- Service
- 基础
- learn
prerequisites:
- kubectl-basics
- gpu-ml-basics
created: "2026-05-23"
---

# Day 22: [[Service|Service]] 基础

```yaml
---
title: Day 22: Service 基础
last_updated: 2026-05-18
difficulty: intermediate
intent_queries:
  - "[[entities/kubernetes.md|kubernetes]] Service类型"
  - "ClusterIP NodePort LoadBalancer"
  - "Service Endpoints"
  - "kube-proxy配置"
  - "ACK SLB集成"
trigger_keywords:
  - "Service基础"
  - "ClusterIP"
  - "NodePort"
  - "LoadBalancer"
  - "Headless Service"
  - "Endpoints"
  - "kube-proxy"
  - "SLB负载均衡"
reading_level: intermediate
audience:
  - sre工程师
  - ops工程师
  - 运维工程师
estimated_read_time: 45min
related_domains:
  - domain-03-networking-traffic
  - domain-04-storage-data
  - domain-10-troubleshooting-diagnostics
related_topics:
  - domain-11-production-operations/topic-learn/inner-training/week-4-network-storage/day-23-ingress
  - domain-11-production-operations/topic-learn/inner-training/week-4-network-storage/day-24-terway-cni
  - domain-11-production-operations/topic-learn/inner-training/week-4-network-storage/day-25-flannel-cni
id: WEEK4-DAY22
topic: training
type: hands-on
tags: [week-4, day-22, service, networking, k8s, k8s-1.28-1.33]
---
```

> **学习时间**: 4-5 小时 | **主题**: Service 类型与配置实践

---

## 概述

本文深入讲解 Kubernetes Service 的核心概念和配置实践。Service 是 K8s 中最基础的服务暴露机制，它为一组 Pod 提供稳定的访问入口和负载均衡能力。理解 Service 的工作原理（kube-proxy、iptables/IPVS、Endpoints）对于部署和调试应用至关重要。在 ACK 环境中，LoadBalancer 类型 Service 与阿里云 SLB 的自动集成是生产环境暴露服务的标准方式。

### 学习目标

- 理解 Service 的作用与核心机制（kube-proxy / iptables / IPVS / Endpoints）
- 掌握 ClusterIP / NodePort / LoadBalancer / Headless 四种类型的配置和使用场景
- 能通过 ACK 控制台和 kubectl 创建和调试 Service
- 了解 ACK 中 SLB（负载均衡）与 LoadBalancer Service 的集成方式
- 掌握 Service DNS 解析规则和连通性测试方法

---

## 核心概念详解

### Service 的工作原理

在 K8s 中，Pod 的 IP 地址是临时的——每次 Pod 重建都会获得新的 IP。这使得直接使用 Pod IP 通信变得不可靠。Service 通过以下机制解决这个问题：

1. **稳定的虚拟 IP（ClusterIP）**: Service 分配一个固定的集群内部 IP，客户端通过这个 IP 访问服务
2. **Label Selector 自动发现**: Service 通过 Label Selector 选择匹配的 Pod，将其 IP 和端口记录在 Endpoints 对象中
3. **负载均衡**: kube-proxy 在每个节点上配置 iptables 或 IPVS 规则，将 ClusterIP 的流量均匀转发到 Endpoints 中的 Pod
4. **DNS 名称解析**: [[CoreDNS|CoreDNS]] 为每个 Service 创建 DNS 记录，格式为 `<service-name>.<namespace>.svc.cluster.local`

### Service 的四种类型

**ClusterIP** 是默认类型。它分配一个集群内部的虚拟 IP 地址，只能在集群内部访问。ClusterIP 来自创建集群时指定的 Service CIDR（如 10.96.0.0/12）。ClusterIP 适合集群内部服务间通信。

**NodePort** 在 ClusterIP 的基础上，在每个节点上开放一个固定的端口（默认范围 30000-32767）。外部流量可以通过 `<任意节点IP>:<NodePort>` 访问到 Service。NodePort 适合测试环境或小规模暴露服务。

**LoadBalancer** 在 NodePort 的基础上，自动创建一个外部负载均衡器。在 ACK 中，会自动创建阿里云 SLB 实例。LoadBalancer 分配一个外部 IP（或域名），流量经过 SLB → NodePort → kube-proxy → Pod 的路径到达目标。这是生产环境暴露 TCP/UDP 服务最常用的方式。

**Headless Service** 设置 `clusterIP: None`，不分配 ClusterIP。DNS 查询直接返回 Pod IP 列表（而不是 ClusterIP）。主要用于 [[StatefulSet|StatefulSet]]，为每个 Pod 提供独立的 DNS 名称（如 `app-0.headless-svc.namespace.svc.cluster.local`）。

### kube-proxy 模式

kube-proxy 负责在节点上实现 Service 的转发规则。有两种主要模式：

**iptables 模式**（默认）: 使用 iptables 规则实现 NAT 转发。每条 Service 和 Endpoints 组合对应一条 iptables 规则。在大规模集群中（数千 Service），iptables 规则数量线性增长，查找使用线性遍历，性能下降。

**IPVS 模式**: 使用 Linux 内核的 IPVS（IP Virtual Server）实现负载均衡。IPVS 使用哈希表查找，性能在大规模场景下优于 iptables。支持多种调度算法：rr（轮询）、lc（最少连接）、wrr（加权轮询）等。

### ACK 中 Service 与 SLB 的集成

在 ACK 中创建 LoadBalancer 类型 Service 时，Cloud Controller Manager 会自动调用阿里云 API 创建 SLB 实例。可以通过 annotation 自定义 SLB 的配置：

| Annotation | 说明 | 示例值 |
|-----------|------|--------|
| `service.beta.kubernetes.io/alibaba-cloud-loadbalancer-spec` | SLB 规格 | `slb.s1.small` |
| `service.beta.kubernetes.io/alibaba-cloud-loadbalancer-address-type` | 网络类型 | `internet` / `intranet` |
| `service.beta.kubernetes.io/alibaba-cloud-loadbalancer-charge-type` | 计费方式 | `paybytraffic` / `paybybandwidth` |
| `service.beta.kubernetes.io/alibaba-cloud-loadbalancer-bandwidth` | 带宽限制 | `100`（Mbps） |
| `service.beta.kubernetes.io/alibaba-cloud-loadbalancer-backend-label` | 后端节点标签 | `node-role=worker` |

---

## 实战演练

### 任务 1: ClusterIP Service (40min)

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

```bash
# 创建 Deployment
kubectl create deployment web --image=registry.cn-hangzhou.aliyuncs.com/acs-sample/nginx:1.24 --replicas=3
# 预期输出: deployment.apps/web created

# 查看 Pod IP
kubectl get pods -l app=web -o wide
# 预期输出:
# NAME                   READY   STATUS    RESTARTS   AGE   IP            NODE
# web-6d4f7b8c9d-abc12   1/1     Running   0          1m    172.20.0.10   node-1
# web-6d4f7b8c9d-def34   1/1     Running   0          1m    172.20.1.11   node-2
# web-6d4f7b8c9d-ghi56   1/1     Running   0          1m    172.20.2.12   node-3

# 创建 ClusterIP Service
cat <<EOF | kubectl apply -f -
apiVersion: v1
kind: Service
metadata:
  name: web-clusterip
spec:
  type: ClusterIP
  selector:
    app: web
  ports:
  - port: 80
    targetPort: 80
    protocol: TCP
EOF
# 预期输出: service/web-clusterip created

# 验证 Service
kubectl get svc web-clusterip
# 预期输出:
# NAME            TYPE        CLUSTER-IP      EXTERNAL-IP   PORT(S)   AGE
# web-clusterip   ClusterIP   10.96.123.456   <none>        80/TCP    10s

kubectl describe svc web-clusterip
# 预期输出:
# Name:              web-clusterip
# Namespace:         default
# Labels:            <none>
# Selector:          app=web
# Type:              ClusterIP
# IP Family Policy:  SingleStack
# IP Families:       IPv4
# IP:                10.96.123.456
# Port:              <unset>  80/TCP
# TargetPort:        80/TCP
# Endpoints:         172.20.0.10:80,172.20.1.11:80,172.20.2.12:80

# 查看 Endpoints（Service 关联的后端 Pod）
kubectl get endpoints web-clusterip
# 预期输出:
# NAME            ENDPOINTS                                      AGE
# web-clusterip   172.20.0.10:80,172.20.1.11:80,172.20.2.12:80   1m

# 集群内访问测试
kubectl run curl-test --image=registry.cn-hangzhou.aliyuncs.com/acs-sample/busybox:1.36 \
  --rm -it --restart=Never -- wget -qO- http://web-clusterip
# 预期输出: nginx 默认欢迎页面 HTML

# 测试 DNS 解析
kubectl run dns-test --image=registry.cn-hangzhou.aliyuncs.com/acs-sample/busybox:1.36 \
  --rm -it --restart=Never -- nslookup web-clusterip
# 预期输出:
# Server:    10.96.0.10
# Address 1: 10.96.0.10 kube-dns.kube-system.svc.cluster.local
# Name:      web-clusterip
# Address 1: 10.96.123.456 web-clusterip.default.svc.cluster.local
```

### 任务 2: NodePort Service (30min)

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

```bash
# 创建 NodePort Service
cat <<EOF | kubectl apply -f -
apiVersion: v1
kind: Service
metadata:
  name: web-nodeport
spec:
  type: NodePort
  selector:
    app: web
  ports:
  - port: 80
    targetPort: 80
    nodePort: 30080
EOF
# 预期输出: service/web-nodeport created

# 查看分配的 NodePort
kubectl get svc web-nodeport
# 预期输出:
# NAME           TYPE       CLUSTER-IP      EXTERNAL-IP   PORT(S)        AGE
# web-nodeport   NodePort   10.96.234.567   <none>        80:30080/TCP   10s

# 通过节点 IP + NodePort 访问
NODE_IP=$(kubectl get nodes -o jsonpath='{.items[0].status.addresses[?(@.type=="InternalIP")].address}')
echo "访问地址: http://${NODE_IP}:30080"
curl http://${NODE_IP}:30080
# 预期输出: nginx 默认欢迎页面

# 注意: 任意节点的 IP + NodePort 都可以访问
# 即使该节点上没有运行目标 Pod，kube-proxy 也会将流量转发到其他节点
```

### 任务 3: LoadBalancer Service（ACK + SLB）(40min)

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

```bash
# 创建 LoadBalancer Service（ACK 自动创建 SLB）
cat <<EOF | kubectl apply -f -
apiVersion: v1
kind: Service
metadata:
  name: web-lb
  annotations:
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-spec: "slb.s1.small"
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-charge-type: "paybytraffic"
EOF
cat <<EOF | kubectl apply -f -
apiVersion: v1
kind: Service
metadata:
  name: web-lb
  annotations:
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-spec: "slb.s1.small"
spec:
  type: LoadBalancer
  selector:
    app: web
  ports:
  - port: 80
    targetPort: 80
EOF
# 预期输出: service/web-lb created

# 等待外部 IP 分配
kubectl get svc web-lb -w
# 预期输出（动态变化）:
# NAME     TYPE           CLUSTER-IP      EXTERNAL-IP      PORT(S)        AGE
# web-lb   LoadBalancer   10.96.100.200   <pending>        80:31234/TCP   5s
# web-lb   LoadBalancer   10.96.100.200   47.102.xxx.xxx   80:31234/TCP   30s

# 查看关联的 SLB 信息
kubectl describe svc web-lb | grep -A 5 "LoadBalancer Ingress"
# 预期输出:
# LoadBalancer Ingress:     47.102.xxx.xxx

# 通过 SLB 外部 IP 访问
EXTERNAL_IP=$(kubectl get svc web-lb -o jsonpath='{.status.loadBalancer.ingress[0].ip}')
echo "外部访问: http://${EXTERNAL_IP}"
curl http://${EXTERNAL_IP}
# 预期输出: nginx 默认欢迎页面

# 查看 Service 事件（了解 SLB 创建过程）
kubectl describe svc web-lb | grep -A 10 "Events:"
# 预期输出:
# Events:
#   Type    Reason                Age   From                Message
#   Normal  EnsuringLoadBalancer  60s   service-controller  Ensuring load balancer
#   Normal  EnsuredLoadBalancer   30s   service-controller  Ensured load balancer
```

### 任务 4: Headless Service 与 DNS (30min)

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源
> - `kubectl delete`：删除资源（可由声明式清单重建）

```bash
# 创建 Headless Service
cat <<EOF | kubectl apply -f -
apiVersion: v1
kind: Service
metadata:
  name: web-headless
spec:
  clusterIP: None
  selector:
    app: web
  ports:
  - port: 80
EOF
# 预期输出: service/web-headless created

# DNS 解析对比
kubectl run dns-test --image=registry.cn-hangzhou.aliyuncs.com/acs-sample/busybox:1.36 \
  --rm -it --restart=Never -- sh -c '
  echo "=== ClusterIP Service ==="
  nslookup web-clusterip
  echo "=== Headless Service ==="
  nslookup web-headless
  echo "=== Full DNS ==="
  nslookup web-clusterip.default.svc.cluster.local
'
# 预期输出:
# === ClusterIP Service ===
# Server:    10.96.0.10
# Name:      web-clusterip
# Address 1: 10.96.123.456 web-clusterip.default.svc.cluster.local
# (返回 ClusterIP)
#
# === Headless Service ===
# Server:    10.96.0.10
# Name:      web-headless
# Address 1: 172.20.0.10 web-6d4f7b8c9d-abc12
# Address 2: 172.20.1.11 web-6d4f7b8c9d-def34
# Address 3: 172.20.2.12 web-6d4f7b8c9d-ghi56
# (直接返回所有 Pod IP)

# 清理
kubectl delete svc web-clusterip web-nodeport web-lb web-headless
kubectl delete deploy web
```

---

## 配置示例

### LoadBalancer Service 完整配置

```yaml
apiVersion: v1
kind: Service
metadata:
  name: production-app
  annotations:
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-spec: "slb.s2.medium"
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-address-type: "internet"
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-charge-type: "paybytraffic"
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-bandwidth: "100"
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-backend-label: "node-role=worker"
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-health-check-flag: "on"
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-health-check-type: "http"
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-health-check-uri: "/healthz"
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-health-check-connect-port: "80"
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-healthy-threshold: "3"
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-unhealthy-threshold: "3"
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-health-check-interval: "5"
spec:
  type: LoadBalancer
  selector:
    app: production-app
  ports:
  - name: http
    port: 80
    targetPort: 8080
    protocol: TCP
  - name: https
    port: 443
    targetPort: 8443
    protocol: TCP
  externalTrafficPolicy: Local
  sessionAffinity: ClientIP
  sessionAffinityConfig:
    clientIP:
      timeoutSeconds: 10800
```

### ExternalName Service

```yaml
apiVersion: v1
kind: Service
metadata:
  name: external-database
  namespace: production
spec:
  type: ExternalName
  externalName: database.internal.example.com
# 使用: 应用通过 external-database.production.svc.cluster.local 访问
# DNS 返回 CNAME 记录指向 database.internal.example.com
```

---

## 常见问题

### Q1: Service 的 selector 如何关联到后端 Pod？

Service 通过 Label Selector 匹配 Pod 的 labels。匹配到的 Pod IP 和端口会自动记录在 Endpoints 对象中。如果 Endpoints 为空，说明没有 Pod 匹配——检查 selector 中的标签是否与 Pod 的 labels 完全一致（注意 key 和 value 的拼写）。

### Q2: LoadBalancer 类型在 ACK 中创建时，背后发生了什么？

创建 LoadBalancer Service 后，ACK 的 Cloud Controller Manager 检测到新的 Service，调用阿里云 SLB API 创建实例。SLB 配置后端服务器（集群节点），设置监听规则（端口映射），然后返回 SLB 的外部 IP。整个过程通常需要 30 秒到 1 分钟。SLB 的生命周期与 Service 绑定——删除 Service 会自动删除 SLB。

### Q3: Headless Service 的 DNS 解析与普通 ClusterIP Service 有什么区别？

ClusterIP Service 的 DNS 查询返回 ClusterIP（虚拟 IP），然后由 kube-proxy 转发到后端 Pod。Headless Service 的 DNS 查询直接返回所有匹配 Pod 的 IP 地址。客户端可以自行选择连接哪个 Pod，也可以配合 StatefulSet 使用 `pod-name.headless-svc` 的格式访问特定 Pod。

### Q4: externalTrafficPolicy: Local 和 Cluster 有什么区别？

Cluster 模式（默认）：流量可以转发到任意节点上的 Pod，可能跨节点转发（二次跳转）。保留了源 IP（通过 SNAT），但增加了一跳延迟。Local 模式：流量只转发到接收节点上的 Pod，不跨节点。保留了真实客户端源 IP，但要求每个节点都有 Pod 运行（否则该节点不会加入 SLB 后端）。生产环境推荐 Local 模式以保留源 IP。

### Q5: Service 的 Endpoints 更新有延迟怎么办？

Endpoints 的更新依赖 kube-proxy 的 Watch 机制，通常在几秒内完成。但在高频率变更场景（如频繁扩缩容）中可能出现短暂的不一致。可以通过 `kubectl get endpoints <svc-name> -w` 实时观察 Endpoints 变化。readinessProbe 配置正确可以确保只有就绪的 Pod 被加入 Endpoints。

### Q6: 如何在不删除 Service 的情况下更换 SLB？

修改 Service 的 annotation 指定已有的 SLB ID：`service.beta.kubernetes.io/alibaba-cloud-loadbalancer-id: "lb-xxxxx"`。这样 Service 会使用指定的 SLB 而不是创建新的。注意：指定的 SLB 需要与集群在同一 VPC 中。

---

## 要点总结

| Service 类型 | 访问范围 | ACK 集成 | 适用场景 | DNS 返回 |
|-------------|---------|---------|---------|---------|
| ClusterIP | 集群内 | 无 | 内部微服务通信 | ClusterIP |
| NodePort | 节点IP:端口 | 无 | 测试/开发环境 | ClusterIP |
| LoadBalancer | 外部 IP | 自动创建 SLB | 生产环境暴露服务 | ClusterIP |
| Headless | DNS 直接解析到 Pod | 无 | StatefulSet / 服务发现 | Pod IP 列表 |

---

## 延伸阅读

- [Service 概念与类型](../../domain-06-service-networking/01-service-overview.md)
- [kube-proxy 模式详解](../../domain-06-service-networking/02-kube-proxy.md)
- [ACK 网络管理](../../domain-12-cloud-providers/04-alicloud-ack/260-ack-networking.md)
- [网络架构总览](../../domain-03-networking-traffic/01-network-architecture-overview.md)
