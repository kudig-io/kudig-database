---
title: 第四课：Service - 让应用可以被访问
description: 【解决方案】
summary: '2. 掌握 ClusterIP、NodePort、LoadBalancer 三种类型'
category: skills
tags:
- k8s
- learn
- fundamentals
- kubelet
- coredns
- ingress
- networkpolicy
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 第四课：Service - 让应用可以被访问 是什么
- 如何 第四课：Service - 让应用可以被访问
trigger_keywords:
- 第四课：Service
- 让应用可以被访问
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 第四课：[[Service|Service]] - 让应用可以被访问

> **章节**: 入门引导 | **难度**: 入门 | **时长**: 20 分钟

---

## 学习目标

1. 理解 Service 的概念和作用
2. 掌握 ClusterIP、NodePort、LoadBalancer 三种类型
3. 学会创建和使用 Service
4. 了解服务发现原理

---

## 1. Service 的概念

### 1.1 问题引入

```
【问题】

上节课我们学了 Deployment，可以保证 Pod 始终运行。
但有一个问题：Pod 的 IP 地址是变化的！

场景：
• Pod A  IP: 10.244.0.15
• Pod B  IP: 10.244.0.16
• Pod C  IP: 10.244.0.17

如果我访问 Pod A，过了一会儿 Pod A 挂了，
Deployment 重新创建了 Pod，IP 可能变成 10.244.0.25。
那我的应用怎么找到新的 Pod？

【解决方案】

这就引出了 Service 的概念！

Service 会：
• 给一组 Pod 起一个固定的名字（服务名）
• 分配一个固定的 ClusterIP（集群内部的 IP）
• 自动负载均衡，路由到可用的 Pod

就像餐厅前台客服电话：
不管里面有多少服务员在换，顾客只需要记住一个电话号码就行。
前台客服会把电话转给当前在岗的服务员。
```

### 1.2 Service 的定义

```
【核心概念】

Service 是 K8s 提供的一种访问 Pod 的方式。
它会：
1. 创建一个固定的服务名（DNS）
2. 分配一个固定 IP（ClusterIP）
3. 自动追踪 Pod 变化
4. 负载均衡到健康 Pod
```

---

## 2. Service 类型

### 2.1 ClusterIP（集群内部访问）

```
# 🟢 低风险：只读/信息收集，通常无副作用
【类型说明】

ClusterIP 是默认类型，只能在集群内部访问。

【YAML 示例】

apiVersion: v1
kind: Service
metadata:
  name: my-service
spec:
  type: ClusterIP
  selector:
    app: web
  ports:
  - port: 80          # Service 端口
    targetPort: 8080   # Pod 端口

【使用场景】

• 微服务之间互相调用
• 后端服务被前端服务访问
• 集群内部的 API 调用

【命令行创建】

kubectl expose deployment my-app --port=80 --target-port=8080
# 默认创建 ClusterIP 类型
```
---

### 2.2 NodePort（节点端口访问）

```
# 🟢 低风险：只读/信息收集，通常无副作用
【类型说明】

NodePort 通过节点端口访问，外部可以访问。

【YAML 示例】

apiVersion: v1
kind: Service
metadata:
  name: my-service
spec:
  type: NodePort
  selector:
    app: web
  ports:
  - port: 80          # Service 端口
    targetPort: 8080   # Pod 端口
    nodePort: 30080    # 节点端口 (30000-32767)

【访问方式】

http://<节点IP>:30080

【使用场景】

• 开发/测试环境快速验证
• 临时对外提供服务
• 不推荐生产环境使用

【命令行创建】

kubectl expose deployment my-app --port=80 --target-port=8080 --type=NodePort
```
---

### 2.3 LoadBalancer（云负载均衡器）

```
# 🟢 低风险：只读/信息收集，通常无副作用
【类型说明】

LoadBalancer 通常配合云厂商的负载均衡器使用，外部访问最方便。

【YAML 示例】

apiVersion: v1
kind: Service
metadata:
  name: my-service
spec:
  type: LoadBalancer
  selector:
    app: web
  ports:
  - port: 80
    targetPort: 8080

【使用场景】

• 生产环境对外提供服务
• 需要固定公网 IP
• 需要自动负载均衡

【注意事项】

需要云厂商支持：
• AWS: 需要安装 AWS LB Controller
• GCP: 需要 Cloud Load Balancing
• 阿里云: 需要阿里云 LB Controller

本地集群没有 LoadBalancer 功能。
```
---

### 2.4 类型对比

| 类型 | 访问范围 | 端口范围 | 生产可用 | 说明 |
|------|---------|---------|---------|------|
| ClusterIP | 集群内部 | 无 | ✓ | 默认类型，内部服务通信 |
| NodePort | 节点端口 | 30000-32767 | △ | 临时暴露，简单测试 |
| LoadBalancer | 外部 | 云厂商分配 | ✓ | 生产环境推荐 |
| Headless | 集群内部 | 无 | ✓ | 无负载均衡，直接 Pod DNS |

---

## 3. 创建和使用 Service

### 3.1 创建 Service

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

```
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
【方式一：从 Deployment 暴露】

kubectl expose deployment my-app --port=80 --target-port=8080

【方式二：YAML 创建】

kubectl apply -f service.yaml

【查看 Service】

kubectl get services
# 或
kubectl get svc
```
### 3.2 访问 Service

```
【集群内部访问】

方式一：DNS 名称
http://my-service
# 完整格式：my-service.default.svc.cluster.local

方式二：ClusterIP
http://<ClusterIP>:80

方式三：环境变量
应用会自动获得环境变量：
• MY_SERVICE_SERVICE_HOST
• MY_SERVICE_SERVICE_PORT
```

---

## 4. 服务发现

### 4.1 内部 DNS

```
【K8s DNS 架构】

K8s 集群内有一个 DNS 服务器（通常是 CoreDNS）。
Service 创建后，会自动在 DNS 中注册。

【DNS 解析规则】

<service-name>.<namespace>.svc.<cluster-domain>

示例：
my-service.default.svc.cluster.local

简化规则：
1. 同命名空间：直接用 <service-name>
2. 同集群不同命名空间：<service-name>.<namespace>
3. 外部：完整域名
```

### 4.2 环境变量

```
【自动环境变量】

当 Pod 启动时，Kubelet 会为每个 Service 添加环境变量。

示例：
MY_APP_SERVICE_HOST=10.96.0.123
MY_APP_SERVICE_PORT=80

注意：Pod 必须和 Service 在同一个命名空间！
```

---

## 5. 常见问题

### 5.1 Service 无法访问

```
# 🟢 低风险：只读/信息收集，通常无副作用
【排查步骤】

1. 检查 Service 是否存在
   kubectl get svc

2. 检查 Endpoints 是否存在
   kubectl get endpoints <service-name>

   Endpoints 为空 = 没有 Pod 匹配 Service 的 selector

3. 检查 Pod 是否运行
   kubectl get pods -l app=web

4. 检查 selector 是否匹配
   kubectl describe svc <service-name> | grep -A5 Selector

5. 检查网络策略
   如果有 NetworkPolicy，可能阻止访问
```
### 5.2 Pod 无法解析 Service DNS

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

```
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
【排查步骤】

1. 检查 CoreDNS 是否运行
   kubectl get pods -n kube-system -l k8s-app=kube-dns

2. 检查 /etc/resolv.conf
   kubectl exec -it <pod> -- cat /etc/resolv.conf

   确认 nameserver 指向 K8s DNS

3. 测试 DNS 解析
   kubectl run -it --rm dnsutils --image=tutum/dnsutils -- nslookup kubernetes
```
---

## 6. 总结

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl delete`：删除资源（可由声明式清单重建）

```
# 🟢 低风险：只读/信息收集，通常无副作用
【命令速查】

创建 Service：
kubectl expose deployment my-app --port=80 --target-port=8080

查看 Service：
kubectl get svc
kubectl describe svc <name>

删除 Service：
kubectl delete svc <name>

【核心要点】

1. Service 给 Pod 提供固定访问入口
2. ClusterIP：集群内部访问
3. NodePort：通过节点端口，外部可访问
4. LoadBalancer：配合云 LB，生产环境使用
5. 通过 DNS 名称或 ClusterIP 访问 Service

【下节课预告】

下节课我们会学习 Ingress：
• Ingress 是什么
• 如何配置基于域名和路径的路由
• 简化外部访问配置

有问题吗？"
```
---

**关联文档**:
- [../04-networking/04-ingress-basics.md](32-发布/package/2026-07-02_18-29/corpus/peripheral/skills/training-lecturer/04-networking/01-ingress-basics.md) — Ingress 基础
- [../../domain-10-troubleshooting-diagnostics/topic-skills/05-service-connectivity.md](../../domain-10-troubleshooting-diagnostics/技能体系/05-service-connectivity.md) — Service 连通性 [[SKILL|Skill]]
- [../../domain-03-networking-traffic/](../../domain-03-networking-traffic/) — Kubernetes 网络文档

## 相关概念

- [[domain-17-system-foundation/知识字典/networking/service.md|service]]
- 网络模型

## Related

- [[entities/networkpolicy.md|networkpolicy]] — NetworkPolicy
- [[deployment]] — Deployment
- [[entities/kubelet.md|kubelet]] — kubelet
- [[coredns]] — CoreDNS
- [[kubernetes]] — Kubernetes (CNCF Graduated)


<!-- risk-assessed -->
