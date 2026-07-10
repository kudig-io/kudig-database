---
title: 'Day 28: 综合复习与实践'
description: '# Day 28: 综合复习与实践'
summary: 'kubectl apply -f backend-deployment.yaml'
category: learning
tags:
- k8s
- training
- hands-on
- etcd
- kubelet
- scheduler
- prometheus
- grafana
- opa
- redis
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 'Day 28: 综合复习与实践 是什么'
- '如何 Day 28: 综合复习与实践'
trigger_keywords:
- Day
- '28:'
- 综合复习与实践
- learn
prerequisites:
- kubectl-basics
- gpu-ml-basics
- prometheus-basics
- monitoring-basics
- ebpf-basics
- cilium-basics
- cni-basics
- etcd-basics
- redis-basics
- mysql-basics
- policy-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Day 28: 综合复习与实践

> **日期**: Week 4 Day 7 | **主题**: 全流程实操与问题答疑 | **版本**: K8s 1.28-1.33

---

## 1. 综合实操项目

### 1.1 项目：部署 Web 应用完整栈

**要求**：使用所学知识，部署一个完整的 Web 应用栈

```
Frontend (Nginx) → Backend (Python API) → Database (MySQL) + Cache (Redis)
```

**步骤**：

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 1. 创建命名空间
kubectl create namespace production

# 2. 部署 MySQL StatefulSet
cat > mysql-statefulset.yaml <<'EOF'
apiVersion: apps/v1
kind: StatefulSet
metadata:
  name: mysql
  namespace: production
spec:
  selector:
    matchLabels:
      app: mysql
  serviceName: mysql-headless
  replicas: 1
  template:
    metadata:
      labels:
        app: mysql
    spec:
      containers:
        - name: mysql
          image: mysql:8.0
          env:
            - name: MYSQL_ROOT_PASSWORD
              valueFrom:
                secretKeyRef:
                  name: mysql-secret
                  key: root-password
          volumeMounts:
            - name: data
              mountPath: /var/lib/mysql
  volumeClaimTemplates:
    - metadata:
        name: data
      spec:
        accessModes: ["ReadWriteOnce"]
        storageClassName: standard
        resources:
          requests:
            storage: 10Gi
---
apiVersion: v1
kind: Service
metadata:
  name: mysql-headless
  namespace: production
spec:
  clusterIP: None
  selector:
    app: mysql
  ports:
    - port: 3306
---
apiVersion: v1
kind: Secret
metadata:
  name: mysql-secret
  namespace: production
type: Opaque
stringData:
  root-password: changeme123
EOF
kubectl apply -f mysql-statefulset.yaml

# 3. 部署 Backend API
cat > backend-deployment.yaml <<'EOF'
apiVersion: v1
kind: Service
metadata:
  name: backend-svc
  namespace: production
spec:
  selector:
    app: backend
  ports:
    - port: 8080
      targetPort: 8080
---
apiVersion: apps/v1
kind: Deployment
metadata:
  name: backend
  namespace: production
spec:
  replicas: 3
  selector:
    matchLabels:
      app: backend
  template:
    metadata:
      labels:
        app: backend
    spec:
      containers:
        - name: backend
          image: python:3.11-slim
          ports:
            - containerPort: 8080
          env:
            - name: DB_HOST
              value: "mysql-headless.production.svc.cluster.local"
            - name: DB_USER
              valueFrom:
                secretKeyRef:
                  name: mysql-secret
                  key: root-password
          readinessProbe:
            httpGet:
              path: /health
              port: 8080
          livenessProbe:
            httpGet:
              path: /health
              port: 8080
          resources:
            requests:
              cpu: "100m"
              memory: "256Mi"
            limits:
              cpu: "500m"
              memory: "512Mi"
EOF
kubectl apply -f backend-deployment.yaml

# 4. 部署 Frontend Nginx
cat > frontend-deployment.yaml <<'EOF'
apiVersion: v1
kind: Service
metadata:
  name: frontend-svc
  namespace: production
spec:
  type: NodePort
  selector:
    app: frontend
  ports:
    - port: 80
      targetPort: 80
      nodePort: 30080
---
apiVersion: apps/v1
kind: Deployment
metadata:
  name: frontend
  namespace: production
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
        - name: nginx
          image: nginx:1.25-alpine
          ports:
            - containerPort: 80
          resources:
            requests:
              cpu: "50m"
              memory: "64Mi"
            limits:
              cpu: "200m"
              memory: "128Mi"
---
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: web-ingress
  namespace: production
  annotations:
    nginx.ingress.kubernetes.io/proxy-pass: "http://backend-svc.production:8080"
spec:
  ingressClassName: nginx
  rules:
    - host: web.example.com
      http:
        paths:
          - path: /
            pathType: Prefix
            backend:
              service:
                name: frontend-svc
                port:
                  number: 80
EOF
kubectl apply -f frontend-deployment.yaml

# 5. 验证部署
kubectl get all -n production
kubectl get ingress -n production
```
---

## 2. 综合故障排查

### 2.1 场景：Web 应用无法访问

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# ========== 故障排查 SOP ==========

echo "[1] 检查所有 Pod 状态"
kubectl get pods -n production

echo "[2] 检查前端 Service"
kubectl get svc -n production
kubectl describe svc frontend-svc -n production

echo "[3] 检查 Ingress"
kubectl get ingress -n production
kubectl describe ingress web-ingress -n production

echo "[4] 检查后端连通性"
kubectl run curl --image=curlimages/curl --restart=Never -it -- sh
# curl http://backend-svc.production:8080/health

echo "[5] 检查数据库"
kubectl exec -it mysql-0 -n production -- mysql -u root -p -e "SHOW DATABASES;"

echo "[6] 检查网络策略"
kubectl get networkpolicy -n production

echo "[7] 检查事件"
kubectl get events -n production --sort-by='.lastTimestamp' | tail -20

# ========== 修复步骤 ==========
# 如 Pod 未就绪：kubectl describe pod <pod> -n production
# 如 Service 无 Endpoints：kubectl get endpoints -n production
# 如 Ingress 404：检查 annotations 配置
```
---

## 3. 知识图谱回顾

### 3.1 K8s 核心概念

```
控制平面
├── API Server (6443) → 认证/授权/准入 → etcd
├── Scheduler (10259) → Pod 调度到节点
├── Controller Manager (10257) → 控制器循环
└── etcd (2379) → 数据存储

节点组件
├── kubelet (10250) → Pod 生命周期管理
├── kube-proxy (10249) → Service 网络代理
└── Container Runtime → 容器运行

核心资源
├── Pod → 最小调度单元
├── Deployment → 无状态应用管理
├── StatefulSet → 有状态应用管理
├── Service → 内部负载均衡
├── Ingress → 外部 HTTP/HTTPS 路由
├── PVC → 持久化存储
└── ConfigMap/Secret → 配置管理
```

### 3.2 故障排查路径

```
Pod 异常
  ├── Pending → 调度问题/资源不足/污点
  ├── CrashLoopBackOff → 应用错误/配置问题
  ├── ImagePullBackOff → 镜像问题/权限问题
  └── Running 但不可用 → 探针失败/网络问题

Service 异常
  ├── 无 Endpoints → selector 不匹配
  ├── 无法访问 → kube-proxy/CNI 问题
  └── 外部不可达 → Ingress/LoadBalancer 配置

节点异常
  ├── NotReady → kubelet/网络问题
  └── Ready 但 Pod 异常 → 资源压力/磁盘问题

控制平面异常
  ├── API Server 不响应 → etcd/内存问题
  ├── 调度失败 → Scheduler 异常
  └── 控制器不工作 → Controller Manager 异常
```

---

## 4. 自我评估检查清单

### 4.1 Week 1-4 核心技能

- [ ] **Week 1: 集群生命周期**
  - [ ] 能创建/升级/删除集群
  - [ ] 能管理节点池
  - [ ] 能续期证书

- [ ] **Week 2: 安全监控**
  - [ ] 能配置 RBAC 权限
  - [ ] 能配置审计日志
  - [ ] 能部署 [[Prometheus|Prometheus]] + Grafana
  - [ ] 能识别常见安全风险

- [ ] **Week 3: 节点与工作负载**
  - [ ] 能 cordon/drain/uncordon 节点
  - [ ] 能配置 Pod 调度策略
  - [ ] 能配置探针和资源限制
  - [ ] 能排查 Pod 问题

- [ ] **Week 4: 网络与存储**
  - [ ] 能创建和管理 Service
  - [ ] 能配置 Ingress
  - [ ] 能配置 PV/PVC
  - [ ] 能排查网络和存储问题

### 4.2 推荐学习路径

```
入门 → 进阶 → 精通
  ↓       ↓       ↓
节点管理 → 网络/存储 → 安全/监控
       → 调度策略 → 可观测性
                → 故障排查
```

---

## 5. 扩展学习资源

### 5.1 官方文档

- [Kubernetes 官方文档](https://kubernetes.io/docs/)
- [Kubernetes Blog](https://kubernetes.io/blog/)
- [CNCF Landscape](https://landscape.cncf.io/)

### 5.2 推荐书籍/课程

- "Kubernetes in Action" - Marko Lukša
- CKAD/CKA/CKS 认证课程
- Killer.sh 模拟题库

### 5.3 实践平台

- Killercoda（免费 K8s 环境）
- Play with Kubernetes
- Kind/Minikube 本地集群

---

```yaml
---
id: LEARN-WEEK4-DAY28
title: Day 28 - 综合复习与实践
topic: network-storage
type: comprehensive-review
tags: [comprehensive, review, hands-on, troubleshooting, k8s-1.28-1.33]
last_updated: 2026-05-18
difficulty: intermediate
intent_queries:
  - "K8s 综合练习"
  - "故障排查 SOP"
  - "自我评估清单"
  - "Web 应用部署完整流程"
  - "K8s 知识图谱"
trigger_keywords:
  - 综合复习
  - 故障排查
  - 知识图谱
  - 自我评估
  - 生产级部署
  - 微服务架构
  - 监控体系
  - 日志管理
  - 自测清单
  - 学习路径
reading_level: intermediate
audience:
  - sre
  - ops-engineer
  - developer
estimated_read_time: 60min
related_domains:
  - 故障诊断
  - 集群基础
related_topics:
  - troubleshooting
  - deployment
  - networking
  - storage
  - security
related:
  - 生产运维/topic-learn/public-training/[[存储/README.md|README]].md
  - 故障诊断/00-troubleshooting-overview.md
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


```

<!-- risk-assessed -->
