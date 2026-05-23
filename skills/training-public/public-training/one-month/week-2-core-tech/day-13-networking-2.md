---
title: 'Day 13: 网络栈 - Ingress + NetworkPolicy'
description: '# Day 13: 网络栈 - Ingress + NetworkPolicy'
category: learning
tags:
- k8s
- training
- hands-on
- helm
- ingress
- networkpolicy
- rag
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 'Day 13: 网络栈 - Ingress + NetworkPolicy 是什么'
- '如何 Day 13: 网络栈 - Ingress + NetworkPolicy'
trigger_keywords:
- Day
- '13:'
- 网络栈
- Ingress
- NetworkPolicy
- learn
prerequisites:
- kubectl-basics
- gpu-ml-basics
- helm-basics
created: "2026-05-23"
---

# Day 13: 网络栈 - [[Ingress|Ingress]] + [[NetworkPolicy|NetworkPolicy]]

```yaml
---
id: LEARN-ONE-MONTH-W2-DAY13
title: Day 13 - 网络栈 - Ingress + NetworkPolicy
topic: kubernetes
type: hands-on-guide
tags: [ingress, networkpolicy, nginx-ingress, tls, hostname, path, routing, hands-on, week-2]
last_updated: 2026-05-18
difficulty: intermediate
intent_queries:
  - "Ingress 怎么配置"
  - "Ingress Controller 怎么选"
  - "TLS 证书怎么配"
  - "NetworkPolicy 怎么写"
  - "L7 路由怎么实现"
trigger_keywords:
  - Ingress
  - Ingress Controller
  - Nginx Ingress
  - TLS
  - hostname
  - path
  - rewrite
  - canary
  - NetworkPolicy
  - egress
  - ingress
  - podSelector
  - namespaceSelector
  - ipBlock
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
  - ingress
  - networkpolicy
  - tls
related:
  - domain-11-production-operations/topic-learn/public-training/one-month/week-2-core-tech/day-14-storage-practice.md
  - domain-03-networking-traffic/19-ingress-fundamentals.md
---
```

> **学习时间**: 4-5 小时 | **主题**: HTTP 路由与网络隔离

---

## 今日目标

- [ ] 掌握 Ingress 资源和控制器配置
- [ ] 配置 TLS 证书实现 HTTPS
- [ ] 使用 NetworkPolicy 实现 Pod 间访问控制

---

## 理论学习 (2h)

### 必读文档

1. **Ingress 基础**
   - 文件: `../../domain-03-networking-traffic/19-ingress-fundamentals.md`
   - 重点: Ingress 资源和控制器的关系

2. **Nginx Ingress 完整指南**
   - 文件: `../../domain-03-networking-traffic/21-nginx-ingress-complete-guide.md`
   - 重点: 配置方法、常用注解

3. **Ingress TLS 证书**
   - 文件: `../../domain-03-networking-traffic/22-ingress-tls-certificate.md`
   - 重点: TLS 配置、证书管理

4. **NetworkPolicy 深入实践**
   - 文件: `../../domain-03-networking-traffic/16-networkpolicy-deep-practice.md`
   - 重点: 网络隔离策略编写

---

## 实践任务 (2.5h)

### 任务 1: 安装 Nginx Ingress Controller (30min)

```bash
# 方式 1: 使用 Helm (推荐)
helm repo add ingress-nginx https://kubernetes.github.io/ingress-nginx
helm repo update
helm install ingress-nginx ingress-nginx/ingress-nginx --namespace ingress-nginx --create-namespace

# 方式 2: 使用 YAML
kubectl apply -f https://raw.githubusercontent.com/kubernetes/ingress-nginx/controller-v1.8.2/deploy/static/provider/cloud/deploy.yaml

# 等待就绪
kubectl wait --namespace ingress-nginx \
  --for=condition=ready pod \
  --selector=app.kubernetes.io/component=controller \
  --timeout=120s

# 验证
kubectl get pods -n ingress-nginx
kubectl get svc -n ingress-nginx
```

### 任务 2: 配置 Ingress 路由 (45min)

```bash
# 创建测试应用
kubectl create deployment app1 --image=nginx:alpine
kubectl create deployment app2 --image=httpd:alpine
kubectl expose deployment app1 --port=80
kubectl expose deployment app2 --port=80

# 基于路径的路由
cat > ingress-path.yaml << 'EOF'
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: path-ingress
  annotations:
    nginx.ingress.kubernetes.io/rewrite-target: /
spec:
  ingressClassName: nginx
  rules:
  - host: demo.local
    http:
      paths:
      - path: /app1
        pathType: Prefix
        backend:
          service:
            name: app1
            port:
              number: 80
      - path: /app2
        pathType: Prefix
        backend:
          service:
            name: app2
            port:
              number: 80
EOF

kubectl apply -f ingress-path.yaml

# 基于主机名的路由
cat > ingress-host.yaml << 'EOF'
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: host-ingress
spec:
  ingressClassName: nginx
  rules:
  - host: app1.local
    http:
      paths:
      - path: /
        pathType: Prefix
        backend:
          service:
            name: app1
            port:
              number: 80
  - host: app2.local
    http:
      paths:
      - path: /
        pathType: Prefix
        backend:
          service:
            name: app2
            port:
              number: 80
EOF

kubectl apply -f ingress-host.yaml

# 获取 Ingress IP
kubectl get ingress

# 测试 (需要配置 hosts 或使用 --resolve)
INGRESS_IP=$(kubectl get svc -n ingress-nginx ingress-nginx-controller -o jsonpath='{.status.loadBalancer.ingress[0].ip}')
# 或者使用 NodePort
INGRESS_PORT=$(kubectl get svc -n ingress-nginx ingress-nginx-controller -o jsonpath='{.spec.ports[0].nodePort}')

curl -H "Host: demo.local" http://$INGRESS_IP/app1
curl -H "Host: app1.local" http://$INGRESS_IP/
```

### 任务 3: 配置 TLS (30min)

```bash
# 生成自签名证书
openssl req -x509 -nodes -days 365 -newkey rsa:2048 \
  -keyout tls.key -out tls.crt \
  -subj "/CN=demo.local"

# 创建 Secret
kubectl create secret tls demo-tls --key tls.key --cert tls.crt

# 配置 TLS Ingress
cat > ingress-tls.yaml << 'EOF'
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: tls-ingress
spec:
  ingressClassName: nginx
  tls:
  - hosts:
    - demo.local
    secretName: demo-tls
  rules:
  - host: demo.local
    http:
      paths:
      - path: /
        pathType: Prefix
        backend:
          service:
            name: app1
            port:
              number: 80
EOF

kubectl apply -f ingress-tls.yaml

# 测试 HTTPS
curl -k -H "Host: demo.local" https://$INGRESS_IP/

# 清理测试文件
rm tls.key tls.crt
```

### 任务 4: NetworkPolicy 实践 (45min)

```bash
# 创建测试 namespace
kubectl create namespace netpol-test

# 创建测试 Pod
kubectl run web --image=nginx:alpine -n netpol-test -l app=web
kubectl run api --image=nginx:alpine -n netpol-test -l app=api
kubectl run db --image=nginx:alpine -n netpol-test -l app=db

kubectl expose pod web --port=80 -n netpol-test
kubectl expose pod api --port=80 -n netpol-test
kubectl expose pod db --port=80 -n netpol-test

# 默认情况下，所有 Pod 可以互相访问
kubectl run test --image=busybox -n netpol-test -it --rm -- wget -qO- --timeout=2 web

# 1. 默认拒绝所有入站流量
cat > deny-all.yaml << 'EOF'
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: default-deny
  namespace: netpol-test
spec:
  podSelector: {}
  policyTypes:
  - Ingress
EOF

kubectl apply -f deny-all.yaml

# 验证无法访问
kubectl run test --image=busybox -n netpol-test -it --rm -- wget -qO- --timeout=2 web
# 应该超时

# 2. 允许 web 接收来自任何地方的流量
cat > allow-web.yaml << 'EOF'
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: allow-web
  namespace: netpol-test
spec:
  podSelector:
    matchLabels:
      app: web
  policyTypes:
  - Ingress
  ingress:
  - from: []
    ports:
    - port: 80
EOF

kubectl apply -f allow-web.yaml

# 验证 web 可以访问
kubectl run test --image=busybox -n netpol-test -it --rm -- wget -qO- --timeout=2 web

# 3. 只允许 api 访问 db
cat > db-policy.yaml << 'EOF'
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: db-access
  namespace: netpol-test
spec:
  podSelector:
    matchLabels:
      app: db
  policyTypes:
  - Ingress
  ingress:
  - from:
    - podSelector:
        matchLabels:
          app: api
    ports:
    - port: 80
EOF

kubectl apply -f db-policy.yaml

# 验证: api 可以访问 db
kubectl exec -n netpol-test api -- wget -qO- --timeout=2 db

# 验证: web 不能访问 db
kubectl exec -n netpol-test web -- wget -qO- --timeout=2 db
# 应该超时

# 清理
kubectl delete namespace netpol-test
```

---

## 费曼复述 (0.5h)

用自己的语言回答:

1. **Ingress 和 Service 的区别是什么？**
   - Service: L4 负载均衡，基于 IP:Port
   - Ingress: L7 路由，基于 Host/Path

2. **Ingress Controller 的作用是什么？**
   - 监听 Ingress 资源变化
   - 配置反向代理 (如 Nginx)
   - 实现 HTTP 路由和 TLS 终止

3. **NetworkPolicy 的默认行为是什么？**
   - 默认允许所有流量
   - 一旦有 NetworkPolicy 作用于 Pod，则变为默认拒绝

---

## 今日检验

- [ ] 能够部署 Ingress Controller
- [ ] 能够配置基于路径和主机名的路由
- [ ] 能够配置 TLS 证书
- [ ] 能够使用 NetworkPolicy 实现网络隔离

---

## NetworkPolicy 规则模板

```yaml
# 基本模板
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: policy-name
  namespace: target-namespace
spec:
  podSelector:
    matchLabels:
      app: target-app    # 作用于哪些 Pod
  policyTypes:
  - Ingress              # 入站规则
  - Egress               # 出站规则
  ingress:
  - from:
    - podSelector:       # 来源 Pod
        matchLabels:
          app: allowed-app
    - namespaceSelector: # 来源 Namespace
        matchLabels:
          name: allowed-ns
    - ipBlock:           # 来源 IP 段
        cidr: 10.0.0.0/8
    ports:
    - port: 80
      protocol: TCP
```

---

## 明日预告

Day 14 将学习存储体系 (PV/PVC/StorageClass)，并完成本周综合实践项目 P2。

## Related

- index/gitops-cicd-index|GitOps / CI-CD 全局索引]]
