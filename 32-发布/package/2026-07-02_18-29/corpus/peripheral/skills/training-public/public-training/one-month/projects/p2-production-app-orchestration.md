---
title: '项目 P2: 生产级应用全栈编排'
description: '- kubernetes statefulset deployment hpa 完整部署示例'
summary: '- kubernetes statefulset deployment hpa 完整部署示例'
category: learning
tags:
- k8s
- training
- hands-on
- kubelet
- cilium
- flannel
- calico
- hpa
- statefulset
- ingress
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- '项目 P2: 生产级应用全栈编排 是什么'
- '如何 项目 P2: 生产级应用全栈编排'
trigger_keywords:
- 项目
- 'P2:'
- 生产级应用全栈编排
- learn
prerequisites:
- kubectl-basics
- gpu-ml-basics
- cilium-basics
- cni-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




---
title: 项目 P2: 生产级应用全栈编排
last_updated: 2026-05-18
difficulty: intermediate
intent_queries:
  - [[Kubernetes|kubernetes]] [[StatefulSet|statefulset]] deployment hpa 完整部署示例
  - 生产环境 k8s 应用架构怎么设计
  - k8s 网络策略 [[Ingress|ingress]] service 联动配置
  - pvc storageclass dynamic provisioning 配置
trigger_keywords:
  - StatefulSet
  - HPA
  - Ingress
  - NetworkPolicy
  - StorageClass
  - PVC
  - Headless Service
  - 滚动更新
  - 动态供给
  - 生产级部署
reading_level: intermediate
audience:
  - sre-engineer
  - devops-engineer
  - platform-engineer
estimated_read_time: 150min
related_domains:
  - domain-02-workloads-applications
  - domain-03-networking-traffic
  - domain-04-storage-data
  - domain-10-troubleshooting-diagnostics
related_topics:
  - domain-11-production-operations/topic-learn/public-training/one-month/projects/p1-k8s-cluster-setup
  - domain-11-production-operations/topic-learn/public-training/one-month/projects/p3-observability-fault-drill
  - domain-11-production-operations/topic-learn/public-training/one-month/projects/p5-graduation-project
---

# 项目 P2: 生产级应用全栈编排

> **所属周**: Week 2 | **预计时间**: 2.5 小时

---

## 概述

本实践项目要求你部署一套包含前端、后端、存储、网络的完整应用，体验生产级应用编排。你将综合运用 Week 2 所学的 Deployment、StatefulSet、HPA、Service、Ingress、NetworkPolicy 和 StorageClass 等知识，构建一个接近真实生产环境的应用架构。

### 项目目标

部署一套包含前端、后端、存储、网络的完整应用，体验生产级应用编排：
- 前端: Deployment + HPA（自动伸缩）
- 后端: StatefulSet + PVC（有状态存储）
- 网络: Service + Ingress + NetworkPolicy（服务暴露与隔离）
- 存储: StorageClass + PVC（动态供给）

### 前置条件

- 已完成 Week 2 Day 8-13 的学习
- 已安装 Nginx Ingress Controller
- 了解 PV/PVC/StorageClass

---

## 核心概念回顾

### 应用架构设计

```
                    ┌─────────────┐
                    │   Ingress   │
                    │  (TLS+路由)  │
                    └──────┬──────┘
                           │
              ┌────────────┼────────────┐
              │ /          │ /api       │
       ┌──────▼──────┐ ┌──▼───────────┐
       │  Frontend   │ │   Backend    │
       │ Deployment  │ │  StatefulSet │
       │  (HPA)      │ │  (PVC each)  │
       └─────────────┘ └──────────────┘
              ▲               ▲
              │               │
       ┌──────┴──────┐ ┌─────┴───────┐
       │  ClusterIP  │ │  ClusterIP  │
       │  Service    │ │  Service    │
       └─────────────┘ └─────────────┘
                           │
                    ┌──────▼──────┐
                    │NetworkPolicy│
                    │ (隔离控制)   │
                    └─────────────┘
```

### StatefulSet vs Deployment 选择

| 维度 | Deployment | StatefulSet |
|------|-----------|-------------|
| 应用类型 | 无状态 | 有状态 |
| Pod 名称 | 随机后缀 | 有序编号（app-0, app-1） |
| 存储 | 共享或无 | 每个 Pod 独立 PVC |
| 网络 | 通过 Service 负载均衡 | 通过 Headless Service 直接寻址 |
| 扩缩容 | 随机创建/删除 | 有序创建/删除 |
| 适用 | Web 服务、API | 数据库、消息队列 |

---

## 项目步骤

### Step 1: 创建 Namespace (5min)

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
kubectl create namespace production-app
# 预期输出: namespace/production-app created

kubectl config set-context --current --namespace=production-app
```
### Step 2: 部署后端 StatefulSet (30min)

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
cat > backend.yaml << 'EOF'
apiVersion: v1
kind: Service
metadata:
  name: backend-headless
  namespace: production-app
spec:
  clusterIP: None
  selector:
    app: backend
  ports:
  - port: 80
    name: http
---
apiVersion: apps/v1
kind: StatefulSet
metadata:
  name: backend
  namespace: production-app
spec:
  serviceName: backend-headless
  replicas: 2
  selector:
    matchLabels:
      app: backend
  template:
    metadata:
      labels:
        app: backend
    spec:
      containers:
      - name: api
        image: nginx:alpine
        ports:
        - containerPort: 80
        volumeMounts:
        - name: data
          mountPath: /data
        resources:
          requests:
            cpu: 100m
            memory: 128Mi
          limits:
            cpu: 200m
            memory: 256Mi
        livenessProbe:
          httpGet:
            path: /
            port: 80
          initialDelaySeconds: 5
          periodSeconds: 10
        readinessProbe:
          httpGet:
            path: /
            port: 80
          initialDelaySeconds: 3
          periodSeconds: 5
        lifecycle:
          preStop:
            exec:
              command: ["/bin/sh", "-c", "sleep 5"]
      terminationGracePeriodSeconds: 30
  volumeClaimTemplates:
  - metadata:
      name: data
    spec:
      accessModes: ["ReadWriteOnce"]
      resources:
        requests:
          storage: 500Mi
EOF

kubectl apply -f backend.yaml
# 预期输出:
# service/backend-headless created
# statefulset.apps/backend created

# 观察 StatefulSet 有序创建
kubectl get pods -l app=backend -w
# 预期输出:
# NAME        READY   STATUS    RESTARTS   AGE
# backend-0   0/1     Pending   0          0s
# backend-0   0/1     ContainerCreating   0          2s
# backend-0   1/1     Running   0          5s
# backend-1   0/1     Pending   0          0s
# backend-1   0/1     ContainerCreating   0          2s
# backend-1   1/1     Running   0          5s

# 验证 PVC 自动创建
kubectl get pvc
# 预期输出:
# NAME             STATUS   VOLUME             CAPACITY   ACCESS MODES   AGE
# data-backend-0   Bound    pvc-xxxxx          500Mi      RWO            1m
# data-backend-1   Bound    pvc-yyyyy          500Mi      RWO            1m

# 验证 Pod 有序命名
kubectl get pods -l app=backend
# NAME        READY   STATUS    RESTARTS   AGE
# backend-0   1/1     Running   0          2m
# backend-1   1/1     Running   0          1m

# 验证 Headless Service DNS
kubectl run dns-test --image=busybox --rm -it --restart=Never -- \
  nslookup backend-0.backend-headless.production-app.svc.cluster.local
# 预期输出: 返回 backend-0 的 Pod IP
```
### Step 3: 部署前端 Deployment + HPA (30min)

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
cat > frontend.yaml << 'EOF'
apiVersion: apps/v1
kind: Deployment
metadata:
  name: frontend
  namespace: production-app
spec:
  replicas: 2
  selector:
    matchLabels:
      app: frontend
  strategy:
    type: RollingUpdate
    rollingUpdate:
      maxSurge: 1
      maxUnavailable: 0
  template:
    metadata:
      labels:
        app: frontend
    spec:
      containers:
      - name: web
        image: nginx:alpine
        ports:
        - containerPort: 80
        resources:
          requests:
            cpu: 50m
            memory: 64Mi
          limits:
            cpu: 100m
            memory: 128Mi
        livenessProbe:
          httpGet:
            path: /
            port: 80
          initialDelaySeconds: 5
          periodSeconds: 10
        readinessProbe:
          httpGet:
            path: /
            port: 80
          initialDelaySeconds: 3
          periodSeconds: 5
---
apiVersion: autoscaling/v2
kind: HorizontalPodAutoscaler
metadata:
  name: frontend-hpa
  namespace: production-app
spec:
  scaleTargetRef:
    apiVersion: apps/v1
    kind: Deployment
    name: frontend
  minReplicas: 2
  maxReplicas: 5
  metrics:
  - type: Resource
    resource:
      name: cpu
      target:
        type: Utilization
        averageUtilization: 70
EOF

kubectl apply -f frontend.yaml
# 预期输出:
# deployment.apps/frontend created
# horizontalpodautoscaler.autoscaling/frontend-hpa created

# 验证 Deployment
kubectl get deployment frontend
# NAME       READY   UP-TO-DATE   AVAILABLE   AGE
# frontend   2/2     2            2           1m

# 验证 HPA
kubectl get hpa frontend-hpa
# NAME            REFERENCE             TARGETS         MINPODS   MAXPODS   REPLICAS   AGE
# frontend-hpa    Deployment/frontend   <unknown>/70%   2         5         2          1m
```
### Step 4: 创建 Service (15min)

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
cat > services.yaml << 'EOF'
apiVersion: v1
kind: Service
metadata:
  name: frontend
  namespace: production-app
spec:
  selector:
    app: frontend
  ports:
  - port: 80
    targetPort: 80
---
apiVersion: v1
kind: Service
metadata:
  name: backend
  namespace: production-app
spec:
  selector:
    app: backend
  ports:
  - port: 80
    targetPort: 80
EOF

kubectl apply -f services.yaml
# 预期输出:
# service/frontend created
# service/backend created

# 验证 Service 和 Endpoints
kubectl get svc
# NAME       TYPE        CLUSTER-IP      EXTERNAL-IP   PORT(S)   AGE
# frontend   ClusterIP   10.96.100.1     <none>        80/TCP    10s
# backend    ClusterIP   10.96.100.2     <none>        80/TCP    10s

kubectl get endpoints
# NAME       ENDPOINTS                      AGE
# frontend   10.244.1.2:80,10.244.2.2:80    30s
# backend    10.244.1.3:80,10.244.2.3:80    30s
```
### Step 5: 创建 Ingress (20min)

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
cat > ingress.yaml << 'EOF'
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: app-ingress
  namespace: production-app
  annotations:
    nginx.ingress.kubernetes.io/rewrite-target: /
    nginx.ingress.kubernetes.io/ssl-redirect: "false"
spec:
  ingressClassName: nginx
  rules:
  - host: app.local
    http:
      paths:
      - path: /
        pathType: Prefix
        backend:
          service:
            name: frontend
            port:
              number: 80
      - path: /api
        pathType: Prefix
        backend:
          service:
            name: backend
            port:
              number: 80
EOF

kubectl apply -f ingress.yaml
# 预期输出: ingress.networking.k8s.io/app-ingress created

# 查看 Ingress
kubectl get ingress
# NAME           CLASS   HOSTS       ADDRESS          PORTS   AGE
# app-ingress    nginx   app.local   192.168.x.x      80      10s

# 测试路由
INGRESS_IP=$(kubectl get svc -n ingress-nginx ingress-nginx-controller -o jsonpath='{.status.loadBalancer.ingress[0].ip}' 2>/dev/null || \
  kubectl get svc -n ingress-nginx ingress-nginx-controller -o jsonpath='{.spec.clusterIP}')

curl -H "Host: app.local" http://$INGRESS_IP/
# 预期输出: 前端 nginx 欢迎页面

curl -H "Host: app.local" http://$INGRESS_IP/api
# 预期输出: 后端 nginx 欢迎页面
```
### Step 6: 创建 NetworkPolicy (20min)

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
cat > networkpolicy.yaml << 'EOF'
# 前端: 允许外部访问
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: frontend-policy
  namespace: production-app
spec:
  podSelector:
    matchLabels:
      app: frontend
  policyTypes:
  - Ingress
  ingress:
  - from: []
    ports:
    - port: 80
      protocol: TCP
---
# 后端: 只允许前端和同 namespace 访问
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: backend-policy
  namespace: production-app
spec:
  podSelector:
    matchLabels:
      app: backend
  policyTypes:
  - Ingress
  ingress:
  - from:
    - podSelector:
        matchLabels:
          app: frontend
    - podSelector:
        matchLabels:
          app: backend
    ports:
    - port: 80
      protocol: TCP
---
# 默认拒绝: 禁止其他 ingress 流量
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: default-deny-ingress
  namespace: production-app
spec:
  podSelector: {}
  policyTypes:
  - Ingress
EOF

kubectl apply -f networkpolicy.yaml
# 预期输出:
# networkpolicy.networking.k8s.io/frontend-policy created
# networkpolicy.networking.k8s.io/backend-policy created
# networkpolicy.networking.k8s.io/default-deny-ingress created

# 验证 NetworkPolicy
kubectl get networkpolicy
# NAME                   POD-SELECTOR    AGE
# frontend-policy        app=frontend    10s
# backend-policy         app=backend     10s
# default-deny-ingress   <none>          10s
```
### Step 7: 验证和测试 (30min)

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl delete`：删除资源（可由声明式清单重建）
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 查看所有资源
kubectl get all -n production-app
kubectl get pvc -n production-app
kubectl get ingress -n production-app
kubectl get networkpolicy -n production-app
# 预期输出: 所有资源正常

# 测试 Service 连通性
kubectl run curl-test --image=curlimages/curl -it --rm --restart=Never -- \
  curl -s http://frontend.production-app.svc.cluster.local
# 预期输出: 前端 nginx 页面

# 测试滚动更新
kubectl set image deployment/frontend web=nginx:1.25 -n production-app
# 预期输出: deployment.apps/frontend image updated

kubectl rollout status deployment/frontend -n production-app
# 预期输出: deployment "frontend" successfully rolled out

# 验证 HPA
kubectl get hpa -n production-app
# NAME            REFERENCE             TARGETS   MINPODS   MAXPODS   REPLICAS   AGE
# frontend-hpa    Deployment/frontend   0%/70%    2         5         2          10m

# 测试 StatefulSet 有序性
kubectl scale statefulset backend --replicas=3 -n production-app
kubectl get pods -l app=backend -w
# 预期输出: backend-2 有序创建
# 注意: StatefulSet 按 0→1→2 顺序创建，按 2→1→0 顺序删除

# 验证 PVC 数据持久化
kubectl exec backend-0 -- sh -c 'echo "data from backend-0" > /data/test.txt'
kubectl delete pod backend-0
# 等待 backend-0 重新调度
kubectl exec backend-0 -- cat /data/test.txt
# 预期输出: data from backend-0（数据持久化验证成功）
```
---

## 配置示例

### 完整的应用清单（单文件）

```yaml
apiVersion: v1
kind: Namespace
metadata:
  name: production-app
---
apiVersion: v1
kind: Service
metadata:
  name: backend-headless
  namespace: production-app
spec:
  clusterIP: None
  selector:
    app: backend
  ports:
  - port: 80
---
apiVersion: apps/v1
kind: StatefulSet
metadata:
  name: backend
  namespace: production-app
spec:
  serviceName: backend-headless
  replicas: 2
  selector:
    matchLabels:
      app: backend
  template:
    metadata:
      labels:
        app: backend
    spec:
      containers:
      - name: api
        image: nginx:alpine
        ports:
        - containerPort: 80
        volumeMounts:
        - name: data
          mountPath: /data
        resources:
          requests:
            cpu: 100m
            memory: 128Mi
          limits:
            cpu: 200m
            memory: 256Mi
  volumeClaimTemplates:
  - metadata:
      name: data
    spec:
      accessModes: ["ReadWriteOnce"]
      resources:
        requests:
          storage: 500Mi
---
apiVersion: apps/v1
kind: Deployment
metadata:
  name: frontend
  namespace: production-app
spec:
  replicas: 2
  selector:
    matchLabels:
      app: frontend
  template:
    metadata:
      labels:
        app: frontend
    spec:
      containers:
      - name: web
        image: nginx:alpine
        ports:
        - containerPort: 80
        resources:
          requests:
            cpu: 50m
            memory: 64Mi
          limits:
            cpu: 100m
            memory: 128Mi
---
apiVersion: autoscaling/v2
kind: HorizontalPodAutoscaler
metadata:
  name: frontend-hpa
  namespace: production-app
spec:
  scaleTargetRef:
    apiVersion: apps/v1
    kind: Deployment
    name: frontend
  minReplicas: 2
  maxReplicas: 5
  metrics:
  - type: Resource
    resource:
      name: cpu
      target:
        type: Utilization
        averageUtilization: 70
---
apiVersion: v1
kind: Service
metadata:
  name: frontend
  namespace: production-app
spec:
  selector:
    app: frontend
  ports:
  - port: 80
---
apiVersion: v1
kind: Service
metadata:
  name: backend
  namespace: production-app
spec:
  selector:
    app: backend
  ports:
  - port: 80
---
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: app-ingress
  namespace: production-app
spec:
  ingressClassName: nginx
  rules:
  - host: app.local
    http:
      paths:
      - path: /
        pathType: Prefix
        backend:
          service:
            name: frontend
            port:
              number: 80
      - path: /api
        pathType: Prefix
        backend:
          service:
            name: backend
            port:
              number: 80
---
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: backend-policy
  namespace: production-app
spec:
  podSelector:
    matchLabels:
      app: backend
  policyTypes:
  - Ingress
  ingress:
  - from:
    - podSelector:
        matchLabels:
          app: frontend
    ports:
    - port: 80

```

---

## 常见问题

### Q1: StatefulSet Pod 创建顺序是怎样的？

StatefulSet 按 Pod 序号顺序创建（0→1→2...），只有前一个 Pod Ready 后才创建下一个。删除时按逆序（...→2→1→0）。这保证了有状态应用的启动依赖关系。

### Q2: HPA 显示 unknown 怎么办？

HPA 需要 metrics-server 才能获取 CPU/内存指标。安装：`kubectl apply -f https://github.com/kubernetes-sigs/metrics-server/releases/latest/download/components.yaml`。在 kind 中需要添加 `--kubelet-insecure-tls` 参数。

### Q3: Ingress 创建了但 ADDRESS 为空？

检查 Ingress Controller 是否正常运行：`kubectl get pods -n ingress-nginx`。如果是 LoadBalancer 类型，等待云服务商分配外部 IP。如果是 kind/minikube，可能需要使用 `kubectl port-forward` 访问。

### Q4: NetworkPolicy 不生效？

NetworkPolicy 需要支持它的 CNI 插件（如 Calico、Cilium、Terway）。Flannel 不支持 NetworkPolicy。检查 Pod 的 labels 是否与 NetworkPolicy 的 selector 匹配。

---

## 验收清单

- [ ] StatefulSet 部署成功，Pod 名称有序（backend-0, backend-1）
- [ ] 每个 StatefulSet Pod 有独立的 PVC
- [ ] Deployment 部署成功，HPA 配置正常
- [ ] Service 和 Endpoints 正常
- [ ] Ingress 路由正常工作（/ 和 /api 分别路由到不同 Service）
- [ ] NetworkPolicy 隔离有效（后端只允许前端访问）
- [ ] 滚动更新正常
- [ ] PVC 数据在 Pod 重启后持久化

---

## 要点总结

| 资源 | 用途 | 关键配置 |
|------|------|---------|
| StatefulSet | 有状态应用 | serviceName + volumeClaimTemplates |
| Deployment | 无状态应用 | strategy.rollingUpdate |
| HPA | 自动伸缩 | target.averageUtilization |
| Service | 服务发现 | selector + ClusterIP |
| Ingress | HTTP 路由 | rules + paths + host |
| NetworkPolicy | 网络隔离 | podSelector + ingress/egress |
| PVC | 存储申请 | accessModes + storage |

---

## 清理资源

> ⚠️ **🔴 灾难性操作** — 含不可逆命令，执行前必须满足变更窗口+双人复核+事前备份+回滚方案
> - `kubectl delete namespace`：永久删除命名空间及全部资源，不可恢复

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
kubectl delete namespace production-app  # ⚠️ 不可逆：永久删除命名空间及全部资源
```
---

## 延伸阅读

- [Deployment 生产模式](32-发布/package/2026-07-02_18-29/corpus/supporting/domain-02-workloads-applications/00-core-workloads/01-deployment-production-patterns.md)
- [StatefulSet 高级操作](32-发布/package/2026-07-02_18-29/corpus/core/domain-02-workloads-applications/00-core-workloads/01-statefulset-advanced-operations.md)
- [Service 概念与类型](32-发布/package/2026-07-02_18-29/corpus/peripheral/domain-03-networking-traffic/00-core-k8s-networking/05-service-concepts-types.md)
- [Ingress 基础](32-发布/package/2026-07-02_18-29/corpus/supporting/domain-03-networking-traffic/00-core-k8s-networking/05-ingress-fundamentals.md)
- [存储架构总览](../../domain-04-storage-data/01-storage-architecture-overview.md)

## Related

- [[domain-19-landscape-references/领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]

```

<!-- risk-assessed -->
