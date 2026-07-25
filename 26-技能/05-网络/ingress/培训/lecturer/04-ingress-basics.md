---
title: 第五课：Ingress - 外部 HTTP/HTTPS 访问 [04-networking]
description: 2. 掌握 Ingress 的配置方法
summary: 2. 掌握 Ingress 的配置方法
category: k8s-lecturer
tags:
- k8s
- training
- lecturer
- istio
- helm
- ingress
- gateway
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- 培训师
- 技术经理
estimated_read_time: 5min
intent_queries:
- 第五课：Ingress - 外部 HTTP/HTTPS 访问 是什么
- 如何 第五课：Ingress - 外部 HTTP/HTTPS 访问
trigger_keywords:
- 第五课：Ingress
- 外部
- HTTP
- HTTPS
- 访问
- k8s
- lecturer
prerequisites:
- kubectl-basics
- gpu-ml-basics
- helm-basics
- service-mesh-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 第五课：[[Ingress|Ingress]] - 外部 HTTP/HTTPS 访问

> **章节**: 入门引导 | **难度**: 入门 | **时长**: 20 分钟

---

## 学习目标

1. 理解 Ingress 的概念和作用
2. 掌握 Ingress 的配置方法
3. 了解 Ingress Controller 的概念
4. 学会基于域名和路径的路由配置

---

## 1. Ingress 的概念

### 1.1 问题引入

```
【上节课的问题】

上节课我们学了 Service：
• ClusterIP - 集群内部访问
• NodePort - 通过节点端口访问
• LoadBalancer - 云负载均衡器

但这些都有局限：
• ClusterIP 外部无法访问
• NodePort 端口范围限制 (30000-32767)
• LoadBalancer 每个服务需要一个 LB（成本高）

【Ingress 的解决方案】

Ingress 是 K8s 中管理外部 HTTP/HTTPS 访问的资源。
它可以：
• 将外部请求路由到集群内部的服务
• 基于域名和路径的路由
• 提供 SSL/TLS 终止
• 一个 Ingress 控制器可以管理多个服务
```

### 1.2 Ingress 类比

```
【餐厅类比】

Ingress 就像酒店的大堂入口：

• 大堂入口是 Ingress
• 前台服务员是 Service
• 房间是 Pod

客人（用户请求）来到酒店：
1. 先到入口 (Ingress)
2. 告诉入口要去 "餐厅" (域名路由)
3. 入口指向前台 (Service)
4. 前台分配房间给客人 (Pod)

如果没有 Ingress：
客人需要记住每个服务员的房间号 (Pod IP)
这太麻烦了！

有了 Ingress：
客人只需要记住酒店的名字 (域名)，入口会帮你找到对应的服务。
```

---

## 2. Ingress 资源

### 2.1 基本 Ingress 配置

```
【YAML 示例】

apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: my-ingress
spec:
  rules:
  - host: myapp.example.com
    http:
      paths:
      - path: /
        pathType: Prefix
        backend:
          service:
            name: my-service
            port:
              number: 80

【解释】

• host: myapp.example.com → 基于域名的路由
• path: / → 根路径
• pathType: Prefix → 路径前缀匹配
• backend.service.name → 后端 Service 名称
• backend.service.port → Service 端口
```

### 2.2 基于路径的路由

```
【多路径配置】

apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: my-ingress
spec:
  rules:
  - host: myapp.example.com
    http:
      paths:
      - path: /api
        pathType: Prefix
        backend:
          service:
            name: api-service
            port:
              number: 8080
      - path: /web
        pathType: Prefix
        backend:
          service:
            name: web-service
            port:
              number: 80
```

### 2.3 基于域名的路由

```
【多域名配置】

apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: my-ingress
spec:
  rules:
  - host: api.example.com
    http:
      paths:
      - path: /
        pathType: Prefix
        backend:
          service:
            name: api-service
            port:
              number: 8080
  - host: web.example.com
    http:
      paths:
      - path: /
        pathType: Prefix
        backend:
          service:
            name: web-service
            port:
              number: 80
```

---

## 3. TLS 配置

### 3.1 基本配置

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

```
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
【创建 TLS Secret】

kubectl create secret tls my-tls \
  --cert=path/to/cert.pem \
  --key=path/to/key.pem

【Ingress TLS 配置】

apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: my-ingress
spec:
  tls:
  - hosts:
    - myapp.example.com
    secretName: my-tls
  rules:
  - host: myapp.example.com
    http:
      paths:
      - path: /
        pathType: Prefix
        backend:
          service:
            name: my-service
            port:
              number: 80
```
---

## 4. Ingress Controller

### 4.1 什么是 Ingress Controller？

```
# 🟢 低风险：只读/信息收集，通常无副作用
【重要概念】

Ingress 只是 API，需要 Ingress Controller 来实现。

Ingress Controller 是真正处理请求的组件：
• 监听 Ingress 资源变化
• 配置反向代理规则
• 处理流量

【常见 Ingress Controller】

1. nginx-ingress-controller
   最流行的 Ingress 控制器

2. Traefik
   轻量级，易于配置

3. Istio Ingress Gateway
   服务网格环境下使用

4. 云厂商提供的
   AWS ALB Ingress Controller
   阿里云 nginx-ingress-controller
```
### 4.2 安装 Ingress Controller

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `helm upgrade/install`：部署/升级 release

```
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
【使用 Helm 安装 nginx-ingress】

helm install ingress-nginx ingress-nginx/ingress-nginx \
  --namespace ingress-nginx \
  --create-namespace

【检查安装】

kubectl get pods -n ingress-nginx
```
---

## 5. 常见问题

### 5.1 Ingress 不生效

```
# 🟢 低风险：只读/信息收集，通常无副作用
【排查步骤】

1. 检查 Ingress Controller 是否运行
   kubectl get pods -n ingress-nginx

2. 检查 Ingress 资源
   kubectl get ingress
   kubectl describe ingress my-ingress

3. 检查 Ingress Class 是否正确
   apiVersion: networking.k8s.io/v1
   kind: IngressClass
   metadata:
     name: nginx
   spec:
     controller: k8s.io/ingress-nginx

4. 检查 DNS 解析
   确保域名解析到 Ingress Controller 的 IP

5. 检查后端 Service
   确保 Service 和 Pod 正常运行
```
### 5.2 404 错误

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

```
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
【原因】

1. 后端 Service 不存在
2. 后端 Pod 不健康
3. 路径匹配错误
4. DNS 解析问题

【排查】

1. 检查 Service 是否存在
   kubectl get svc

2. 检查 Endpoints
   kubectl get endpoints <service-name>

3. 检查 Pod 状态
   kubectl get pods -l app=<label>

4. 测试后端是否可达
   kubectl exec -it test-pod -- curl <service-name>:80
```
---

## 6. 总结

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源
> - `kubectl delete`：删除资源（可由声明式清单重建）

```
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
【命令速查】

创建 Ingress：
kubectl apply -f ingress.yaml

查看 Ingress：
kubectl get ingress
kubectl describe ingress my-ingress

TLS Secret：
kubectl create secret tls my-tls --cert=cert.pem --key=key.pem

删除 Ingress：
kubectl delete ingress my-ingress

【核心要点】

1. Ingress 管理外部 HTTP/HTTPS 访问
2. 基于域名和路径的路由
3. 需要 Ingress Controller 实现
4. 可以配置 TLS 加密
5. 一个 Ingress 管理多个服务

【下节课预告】

下节课我们会学习 ConfigMap 和 Secret：
• 如何管理应用配置
• 如何存储敏感信息
• 环境变量和 volume 挂载方式

有问题吗？"
```
---

**关联文档**:
- [../05-configuration/05-configmap-secret.md](../../../../04-%E5%B7%A5%E4%BD%9C%E8%B4%9F%E8%BD%BD/pod/%E5%9F%B9%E8%AE%AD/lecturer/05-configmap-secret.md) — 配置管理
- [../../故障诊断/topic-skills/13-ingress-gateway-failure.md](../../../../../19-%E6%95%85%E9%9A%9C%E8%AF%8A%E6%96%AD/08-%E6%8A%80%E8%83%BD%E4%BD%93%E7%B3%BB/13-ingress-gateway-failure.md) — Ingress 问题 [[SKILL|Skill]]
- [../../网络/](../../网络/) — [[23-实体/kubernetes.md|[[Kubernetes|kubernetes]]]] 网络文档

<!-- risk-assessed -->
