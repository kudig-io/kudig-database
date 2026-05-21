---
title: 'Day 12: 网络栈 - CNI + Service + DNS'
description: '# Day 12: 网络栈 - CNI + Service + DNS'
category: learning
tags:
- k8s
- training
- hands-on
- coredns
- ingress
- networkpolicy
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 'Day 12: 网络栈 - CNI + Service + DNS 是什么'
- '如何 Day 12: 网络栈 - CNI + Service + DNS'
trigger_keywords:
- Day
- '12:'
- 网络栈
- CNI
- Service
- DNS
- learn
prerequisites:
- kubectl-basics
- gpu-ml-basics
---

# Day 12: 网络栈 - CNI + Service + DNS

```yaml
---
id: LEARN-ONE-MONTH-W2-DAY12
title: Day 12 - 网络栈 - CNI + Service + DNS
topic: kubernetes
type: hands-on-guide
tags: [cni, service, dns, coredns, iptables, ipvs, networking, hands-on, week-2]
last_updated: 2026-05-18
difficulty: intermediate
intent_queries:
  - "K8s 网络模型是什么"
  - "Service 四种类型怎么选"
  - "CoreDNS 怎么工作"
  - "iptables vs IPVS 区别"
trigger_keywords:
  - CNI
  - Container Network Interface
  - Service
  - ClusterIP
  - NodePort
  - LoadBalancer
  - DNS
  - CoreDNS
  - kube-proxy
  - iptables
  - IPVS
  - Endpoints
  - Service Discovery
reading_level: intermediate
audience:
  - sre
  - ops-engineer
  - developer
estimated_read_time: 45min
related_domains:
  - domain-03-networking-traffic
  - domain-10-troubleshooting-diagnostics
related_topics:
  - networking
  - service
  - dns
  - cni
related:
  - domain-11-production-operations/topic-learn/public-training/one-month/week-2-core-tech/day-13-networking-2.md
  - domain-03-networking-traffic/01-network-architecture-overview.md
---
```

> **学习时间**: 4-5 小时 | **主题**: K8s 网络基础架构

---

## 今日目标

- [ ] 理解 K8s 网络模型和 CNI 机制
- [ ] 掌握 Service 四种类型及实现原理
- [ ] 理解 CoreDNS 服务发现机制

---

## 理论学习 (2h)

### 必读文档

1. **网络架构总览**
   - 文件: `../../domain-03-networking-traffic/01-network-architecture-overview.md`
   - 重点: K8s 网络模型、三层扁平网络

2. **CNI 架构基础**
   - 文件: `../../domain-03-networking-traffic/02-cni-architecture-fundamentals.md`
   - 重点: CNI 插件机制、常见 CNI 实现

3. **Service 概念和类型**
   - 文件: `../../domain-03-networking-traffic/06-service-concepts-types.md`
   - 重点: ClusterIP/NodePort/LoadBalancer/ExternalName

4. **DNS 和服务发现**
   - 文件: `../../domain-03-networking-traffic/11-dns-service-discovery-coredns.md`
   - 重点: CoreDNS 配置、DNS 解析规则

---

## 实践任务 (2.5h)

### 任务 1: 探索 Pod 网络 (30min)

```bash
# 创建两个 Pod
kubectl run pod1 --image=nginx:alpine
kubectl run pod2 --image=busybox --command -- sleep 3600

# 获取 Pod IP
kubectl get pods -o wide

# Pod 间直接通信 (无需 Service)
POD1_IP=$(kubectl get pod pod1 -o jsonpath='{.status.podIP}')
kubectl exec pod2 -- wget -qO- $POD1_IP

# 查看 Pod 的网络配置
kubectl exec pod1 -- ip addr
kubectl exec pod1 -- ip route
kubectl exec pod1 -- cat /etc/resolv.conf

# 清理
kubectl delete pod pod1 pod2
```

### 任务 2: Service 类型实践 (1h)

```bash
# 创建测试 Deployment
kubectl create deployment web --image=nginx:alpine --replicas=3

# 1. ClusterIP Service (默认，集群内访问)
kubectl expose deployment web --port=80 --type=ClusterIP --name=web-clusterip
kubectl get svc web-clusterip
kubectl run curl --image=curlimages/curl -it --rm -- curl web-clusterip

# 2. NodePort Service (节点端口暴露)
kubectl expose deployment web --port=80 --type=NodePort --name=web-nodeport
kubectl get svc web-nodeport
# 访问: http://<node-ip>:<node-port>

# 3. LoadBalancer Service (云环境)
kubectl expose deployment web --port=80 --type=LoadBalancer --name=web-lb
kubectl get svc web-lb
# 在云环境会分配外部 IP

# 4. ExternalName Service (DNS 别名)
cat > external-svc.yaml << 'EOF'
apiVersion: v1
kind: Service
metadata:
  name: external-db
spec:
  type: ExternalName
  externalName: db.example.com
EOF
kubectl apply -f external-svc.yaml

# 查看 Endpoints
kubectl get endpoints web-clusterip
kubectl describe svc web-clusterip

# 清理
kubectl delete deployment web
kubectl delete svc web-clusterip web-nodeport web-lb external-db
```

### 任务 3: Service 实现原理 - iptables (30min)

```bash
# 创建 Service
kubectl create deployment nginx --image=nginx:alpine --replicas=2
kubectl expose deployment nginx --port=80 --type=ClusterIP

# 获取 Service IP
SVC_IP=$(kubectl get svc nginx -o jsonpath='{.spec.clusterIP}')
echo "Service IP: $SVC_IP"

# 查看 iptables 规则 (在节点上执行)
# 注意: 以下命令需要在 K8s 节点上执行，或通过 nsenter

# 查看 KUBE-SERVICES 链
sudo iptables -t nat -L KUBE-SERVICES -n | grep nginx

# 查看 Service 对应的规则
sudo iptables -t nat -L -n | grep $SVC_IP

# 理解流程:
# 1. 目标地址匹配 Service IP -> KUBE-SERVICES
# 2. KUBE-SERVICES -> KUBE-SVC-xxx (Service 规则)
# 3. KUBE-SVC-xxx -> KUBE-SEP-xxx (Endpoint 规则)
# 4. KUBE-SEP-xxx -> DNAT 到 Pod IP

# 清理
kubectl delete deployment nginx
kubectl delete svc nginx
```

### 任务 4: CoreDNS 服务发现 (30min)

```bash
# 查看 CoreDNS 配置
kubectl get configmap coredns -n kube-system -o yaml

# 创建测试环境
kubectl create namespace dns-test
kubectl create deployment web --image=nginx:alpine -n dns-test
kubectl expose deployment web --port=80 -n dns-test

# DNS 解析测试
kubectl run dns-debug --image=busybox -it --rm -- sh

# 在容器内执行:
# 短域名 (同 namespace)
nslookup web

# 完整域名
nslookup web.dns-test.svc.cluster.local

# 查看 /etc/resolv.conf
cat /etc/resolv.conf

# DNS 格式解释:
# <service>.<namespace>.svc.<cluster-domain>
# web.dns-test.svc.cluster.local

# Headless Service DNS
cat > headless-svc.yaml << 'EOF'
apiVersion: v1
kind: Service
metadata:
  name: web-headless
  namespace: dns-test
spec:
  clusterIP: None
  selector:
    app: web
  ports:
  - port: 80
EOF

kubectl apply -f headless-svc.yaml

# Headless Service 返回所有 Pod IP
kubectl run dns-test2 --image=busybox -it --rm -- nslookup web-headless.dns-test

# 清理
kubectl delete namespace dns-test
```

---

## 费曼复述 (0.5h)

用自己的语言回答:

1. **K8s 网络模型的核心原则是什么？**
   - 所有 Pod 可以直接通信，不需要 NAT
   - Pod IP 在集群内全局唯一
   - CNI 插件负责实现网络

2. **ClusterIP Service 的流量是如何转发到 Pod 的？**
   - kube-proxy 配置 iptables/IPVS 规则
   - 请求到 Service IP 被 DNAT 到后端 Pod IP
   - 负载均衡在 iptables 层面实现

3. **CoreDNS 如何实现服务发现？**
   - 监听 K8s API 获取 Service 和 Endpoint
   - 为每个 Service 创建 DNS 记录
   - Pod 默认使用 CoreDNS 作为 DNS 服务器

---

## 今日检验

- [ ] 理解 K8s 网络模型
- [ ] 能够创建不同类型的 Service
- [ ] 理解 Service 的 iptables 实现原理
- [ ] 能够使用 DNS 进行服务发现

---

## Service 类型对比

| 类型 | 访问范围 | 使用场景 |
|------|----------|----------|
| ClusterIP | 集群内部 | 内部服务通信 |
| NodePort | 节点端口 | 开发测试、无 LB 环境 |
| LoadBalancer | 外部 IP | 云环境生产服务 |
| ExternalName | DNS 别名 | 外部服务引用 |

---

## 明日预告

Day 13 将学习 Ingress 和 NetworkPolicy，实现 HTTP 路由和网络隔离。

## Related

- [[domain-19-landscape-references/topic-index/dns-index|DNS 知识图谱索引]]
