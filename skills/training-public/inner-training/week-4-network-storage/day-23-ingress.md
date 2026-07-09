---
title: 'Day 23: Ingress'
description: '- "Kubernetes Ingress配置"'
summary: '- "Kubernetes Ingress配置"'
category: learning
tags:
- k8s
- training
- hands-on
- ingress
- rag
tier: supporting
created: '2026-05-23'
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 'Day 23: Ingress 是什么'
- '如何 Day 23: Ingress'
trigger_keywords:
- Day
- '23:'
- Ingress
- learn
prerequisites:
- kubectl-basics
- gpu-ml-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Day 23: [[Ingress|Ingress]]

```yaml
---
title: Day 23: Ingress
last_updated: 2026-05-18
difficulty: intermediate
intent_queries:
  - "Kubernetes Ingress配置"
  - "Ingress Controller"
  - "Nginx Ingress"
  - "ALB Ingress"
  - "IngressClass"
trigger_keywords:
  - "Ingress"
  - "Ingress Controller"
  - "Nginx Ingress"
  - "ALB Ingress"
  - "IngressClass"
  - "TLS证书"
  - "灰度发布"
  - "金丝雀发布"
reading_level: intermediate
audience:
  - sre工程师
  - ops工程师
  - 运维工程师
estimated_read_time: 45min
related_domains:
  - 网络
  - 故障诊断
related_topics:
  - 生产运维/topic-learn/inner-training/week-4-network-storage/day-22-service-basics
  - 生产运维/topic-learn/inner-training/week-4-network-storage/day-24-terway-cni
  - 网络/19-ingress-fundamentals
id: WEEK4-DAY23
topic: training
type: hands-on
tags: [week-4, day-23, ingress, networking, k8s, k8s-1.28-1.33]
---
```

> **学习时间**: 4-5 小时 | **主题**: Ingress 路由规则与控制器配置

---

## 今日目标

- [ ] 理解 Ingress 资源与 IngressClass 概念
- [ ] 掌握 ACK 中 ALB Ingress Controller 和 Nginx Ingress Controller 的使用
- [ ] 能配置基于域名和路径的路由规则
- [ ] 了解 TLS 证书配置与灰度发布

---

## 理论学习 (2h)

### 必读文档

1. **K8S Ingress 基础**
   - 文件: `../../../domain-06-service-networking/03-ingress.md`
   - 重点: Ingress 规则、IngressClass、默认后端

2. **ACK Ingress 管理**
   - 文件: `../../../云厂商/04-alicloud-ack/260-ack-networking.md`
   - 重点: ALB Ingress vs Nginx Ingress 选型

### 阅读要点

- **Ingress**: L7 层流量路由，基于域名/路径分发到不同 [[Service|Service]]
- **IngressClass**: 指定使用哪个 Ingress Controller 处理
- **ALB Ingress Controller**: 阿里云 ALB (应用型负载均衡) 原生集成，推荐生产使用
- **Nginx Ingress Controller**: 社区方案，ACK 默认组件，灵活度高
- **TLS 终止**: 在 Ingress 层配置 HTTPS 证书
- **灰度发布**: 通过 annotation 实现金丝雀/蓝绿发布

---

## 实践任务 (2.5h)

### 任务 1: Nginx Ingress Controller 基础路由 (40min)

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 确认 Nginx Ingress Controller 已安装
kubectl get pods -n kube-system | grep nginx-ingress
kubectl get svc -n kube-system nginx-ingress-lb

# 创建两个测试应用
kubectl create deployment app-v1 --image=registry.cn-hangzhou.aliyuncs.com/acs-sample/nginx:1.24
kubectl create deployment app-v2 --image=registry.cn-hangzhou.aliyuncs.com/acs-sample/nginx:1.24
kubectl expose deployment app-v1 --port=80
kubectl expose deployment app-v2 --port=80

# 创建基于路径的 Ingress 路由
cat <<EOF | kubectl apply -f -
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: demo-ingress
  annotations:
    nginx.ingress.kubernetes.io/rewrite-target: /
spec:
  ingressClassName: nginx
  rules:
  - host: demo.example.com
    http:
      paths:
      - path: /v1
        pathType: Prefix
        backend:
          service:
            name: app-v1
            port:
              number: 80
      - path: /v2
        pathType: Prefix
        backend:
          service:
            name: app-v2
            port:
              number: 80
EOF

# 查看 Ingress
kubectl get ingress demo-ingress
kubectl describe ingress demo-ingress

# 获取 Ingress Controller 外部 IP 并测试
INGRESS_IP=$(kubectl get svc -n kube-system nginx-ingress-lb -o jsonpath='{.status.loadBalancer.ingress[0].ip}')
curl -H "Host: demo.example.com" http://${INGRESS_IP}/v1
curl -H "Host: demo.example.com" http://${INGRESS_IP}/v2
```
### 任务 2: TLS 证书配置 (30min)

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 创建自签名证书 (测试用)
openssl req -x509 -nodes -days 365 -newkey rsa:2048 \
  -keyout tls.key -out tls.crt \
  -subj "/CN=demo.example.com"

# 创建 TLS Secret
kubectl create secret tls demo-tls --cert=tls.crt --key=tls.key

# 更新 Ingress 添加 TLS
cat <<EOF | kubectl apply -f -
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: demo-ingress-tls
  annotations:
    nginx.ingress.kubernetes.io/ssl-redirect: "true"
spec:
  ingressClassName: nginx
  tls:
  - hosts:
    - demo.example.com
    secretName: demo-tls
  rules:
  - host: demo.example.com
    http:
      paths:
      - path: /
        pathType: Prefix
        backend:
          service:
            name: app-v1
            port:
              number: 80
EOF

# 测试 HTTPS
curl -k -H "Host: demo.example.com" https://${INGRESS_IP}/
```
### 任务 3: 灰度发布 (Canary) (40min)

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 创建灰度 Ingress (将 20% 流量导向 v2)
cat <<EOF | kubectl apply -f -
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: demo-ingress-canary
  annotations:
    nginx.ingress.kubernetes.io/canary: "true"
    nginx.ingress.kubernetes.io/canary-weight: "20"
spec:
  ingressClassName: nginx
  rules:
  - host: demo.example.com
    http:
      paths:
      - path: /
        pathType: Prefix
        backend:
          service:
            name: app-v2
            port:
              number: 80
EOF

# 基于 Header 的灰度 (精准控制)
cat <<EOF | kubectl apply -f -
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: demo-ingress-canary-header
  annotations:
    nginx.ingress.kubernetes.io/canary: "true"
    nginx.ingress.kubernetes.io/canary-by-header: "x-canary"
    nginx.ingress.kubernetes.io/canary-by-header-value: "true"
spec:
  ingressClassName: nginx
  rules:
  - host: demo.example.com
    http:
      paths:
      - path: /
        pathType: Prefix
        backend:
          service:
            name: app-v2
            port:
              number: 80
EOF

# 测试灰度
curl -H "Host: demo.example.com" -H "x-canary: true" http://${INGRESS_IP}/
```
### 任务 4: ALB Ingress Controller (30min)

> ⚠️ **🔴 灾难性操作** — 含不可逆命令，执行前必须满足变更窗口+双人复核+事前备份+回滚方案
> - `kubectl delete --all`：批量删除某类全部资源，波及面巨大
> - `kubectl apply/create/replace`：创建/变更集群资源
> - `kubectl delete`：删除资源（可由声明式清单重建）

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
# 确认 ALB Ingress Controller 是否安装
kubectl get pods -n kube-system | grep alb

# ALB Ingress 示例 (如已安装 ALB Ingress Controller)
cat <<EOF | kubectl apply -f -
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: alb-demo
  annotations:
    alb.ingress.kubernetes.io/address-type: internet
    alb.ingress.kubernetes.io/vswitch-ids: "<vswitch-id-1>,<vswitch-id-2>"
spec:
  ingressClassName: alb
  rules:
  - host: alb-demo.example.com
    http:
      paths:
      - path: /
        pathType: Prefix
        backend:
          service:
            name: app-v1
            port:
              number: 80
EOF

# 清理
kubectl delete ingress --all  # ⚠️ 批量删除，波及面大
kubectl delete secret demo-tls
kubectl delete svc app-v1 app-v2
kubectl delete deploy app-v1 app-v2
rm -f tls.key tls.crt
```
---

## 费曼复述 (0.5h)

1. **Ingress 和 Service LoadBalancer 有什么区别？各自适用什么场景？**
2. **ALB Ingress Controller 和 Nginx Ingress Controller 的核心区别是什么？**
3. **如何通过 Ingress 实现灰度发布？有哪几种流量分配方式？**

---

## 今日检验

- [ ] 能创建基于域名和路径的 Ingress 路由规则
- [ ] 能为 Ingress 配置 TLS 证书
- [ ] 了解灰度发布 (Canary) 的配置方式
- [ ] 理解 ALB Ingress 与 Nginx Ingress 的选型

---

## 核心概念总结

| Ingress Controller | 提供方 | 特点 | 适用场景 |
|-------------------|--------|------|---------|
| Nginx Ingress | 社区/ACK | 灵活、配置丰富 | 通用场景、自定义需求多 |
| ALB Ingress | 阿里云 | 云原生、高性能 | 生产环境、大流量 |

| 路由方式 | 配置方式 | 说明 |
|---------|---------|------|
| 域名路由 | `rules[].host` | 按域名分发 |
| 路径路由 | `rules[].http.paths[]` | 按 URL 路径分发 |
| 灰度发布 | canary annotation | 按权重/Header 分流 |
| TLS 终止 | `tls[]` + Secret | HTTPS 卸载 |

---

## 明日预告

Day 24 将深入学习 Terway CNI 架构与配置。

## Related

- index/nginx-ingress-index|nginx-ingress-controller 知识图谱索引]]


<!-- risk-assessed -->
