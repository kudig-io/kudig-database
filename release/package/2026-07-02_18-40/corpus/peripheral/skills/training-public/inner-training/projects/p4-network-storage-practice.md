---
title: 'P4: 网络与存储综合实践'
description: 'title: P4: 网络与存储综合实践'
summary: 'title: P4: 网络与存储综合实践'
category: learning
tags:
- k8s
- training
- hands-on
- redis
- mysql
- pdb
- statefulset
- ingress
- networkpolicy
- operator
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 'P4: 网络与存储综合实践 是什么'
- '如何 P4: 网络与存储综合实践'
trigger_keywords:
- 'P4:'
- 网络与存储综合实践
- learn
prerequisites:
- kubectl-basics
- gpu-ml-basics
- redis-basics
- mysql-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




---
title: P4: 网络与存储综合实践
last_updated: 2026-05-18
difficulty: advanced
intent_queries:
  - ACK microservice deployment network storage
  - [[Kubernetes|Kubernetes]] [[Ingress|Ingress]] DNS [[Service|service]] discovery
  - StatefulSet PVC persistent storage
  - CNI network policy verification
  - ACK storage CSI integration
trigger_keywords:
  - microservice
  - network
  - storage
  - Ingress
  - StatefulSet
  - PVC
  - DNS
  - service discovery
  - CNI
  - NetworkPolicy
reading_level: advanced
audience:
  - ACK operators
  - Platform engineers
  - DevOps engineers
estimated_read_time: 45min
related_domains:
  - domain-6-networking
  - domain-7-storage
  - domain-12-cloud-providers
  - domain-10-troubleshooting-diagnostics
related_topics:
  - service-networking
  - ingress
  - cni
  - storage
  - pvc
---

# P4: 网络与存储综合实践

> **对应周次**: Week 4 | **预计时间**: 3-4 小时 | **难度**: ⭐⭐⭐

---

## 项目目标

在 ACK 集群中部署一个完整的微服务应用，配置 Service 网络暴露、Ingress 路由、持久化存储，并验证 CNI 网络连通性。

## 前置条件

- [ ] 完成 Week 4 全部教案 (Day 22-28)
- [ ] 有运行中的 ACK 集群
- [ ] Nginx Ingress Controller 已安装
- [ ] 了解 Service / Ingress / PV/PVC 概念

---

## 实施步骤

### Step 1: 创建项目 Namespace 与存储 (30min)

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 1.1 创建 Namespace
kubectl create namespace microservice-demo

# 1.2 创建云盘 PVC (用于数据库)
cat <<EOF | kubectl apply -f -
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: db-data
  namespace: microservice-demo
spec:
  accessModes: [ReadWriteOnce]
  storageClassName: alicloud-disk-ssd
  resources:
    requests:
      storage: 20Gi
EOF

# 1.3 创建 ConfigMap (应用配置)
cat <<EOF | kubectl apply -f -
apiVersion: v1
kind: ConfigMap
metadata:
  name: app-config
  namespace: microservice-demo
data:
  DB_HOST: "db-svc"
  DB_PORT: "3306"
  CACHE_HOST: "cache-svc"
  APP_ENV: "production"
EOF

# 1.4 创建 Secret (敏感信息)
kubectl create secret generic db-credentials -n microservice-demo \
  --from-literal=MYSQL_ROOT_PASSWORD=training2024 \
  --from-literal=MYSQL_DATABASE=appdb
```
### Step 2: 部署后端服务 (数据库 + 缓存) (40min)

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 2.1 部署数据库 (StatefulSet + PVC)
cat <<EOF | kubectl apply -f -
apiVersion: apps/v1
kind: StatefulSet
metadata:
  name: db
  namespace: microservice-demo
spec:
  serviceName: db-svc
  replicas: 1
  selector:
    matchLabels:
      app: db
  template:
    metadata:
      labels:
        app: db
    spec:
      containers:
      - name: mysql
        image: registry.cn-hangzhou.aliyuncs.com/acs-sample/busybox:1.36
        command: ['sh', '-c', 'echo "DB Simulator running" && sleep 86400']
        ports:
        - containerPort: 3306
        volumeMounts:
        - name: data
          mountPath: /var/lib/mysql
        resources:
          requests:
            cpu: 200m
            memory: 256Mi
          limits:
            cpu: "1"
            memory: 1Gi
  volumeClaimTemplates:
  - metadata:
      name: data
    spec:
      accessModes: [ReadWriteOnce]
      storageClassName: alicloud-disk-ssd
      resources:
        requests:
          storage: 20Gi
---
apiVersion: v1
kind: Service
metadata:
  name: db-svc
  namespace: microservice-demo
spec:
  clusterIP: None
  selector:
    app: db
  ports:
  - port: 3306
EOF

# 2.2 部署缓存 (Deployment)
cat <<EOF | kubectl apply -f -
apiVersion: apps/v1
kind: Deployment
metadata:
  name: cache
  namespace: microservice-demo
spec:
  replicas: 1
  selector:
    matchLabels:
      app: cache
  template:
    metadata:
      labels:
        app: cache
    spec:
      containers:
      - name: redis
        image: registry.cn-hangzhou.aliyuncs.com/acs-sample/busybox:1.36
        command: ['sh', '-c', 'echo "Cache Simulator running" && sleep 86400']
        ports:
        - containerPort: 6379
        resources:
          requests:
            cpu: 100m
            memory: 128Mi
---
apiVersion: v1
kind: Service
metadata:
  name: cache-svc
  namespace: microservice-demo
spec:
  selector:
    app: cache
  ports:
  - port: 6379
EOF
```
### Step 3: 部署前端 Web 应用 (30min)

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 3.1 部署 Web 前端 (多副本 + 反亲和)
cat <<EOF | kubectl apply -f -
apiVersion: apps/v1
kind: Deployment
metadata:
  name: web-frontend
  namespace: microservice-demo
spec:
  replicas: 3
  selector:
    matchLabels:
      app: web-frontend
  template:
    metadata:
      labels:
        app: web-frontend
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
                  values: [web-frontend]
              topologyKey: kubernetes.io/hostname
      containers:
      - name: nginx
        image: registry.cn-hangzhou.aliyuncs.com/acs-sample/nginx:1.24
        ports:
        - containerPort: 80
        envFrom:
        - configMapRef:
            name: app-config
        resources:
          requests:
            cpu: 100m
            memory: 128Mi
          limits:
            cpu: 500m
            memory: 256Mi
        livenessProbe:
          httpGet:
            path: /
            port: 80
          periodSeconds: 10
        readinessProbe:
          httpGet:
            path: /
            port: 80
          periodSeconds: 5
---
apiVersion: v1
kind: Service
metadata:
  name: web-frontend-svc
  namespace: microservice-demo
spec:
  type: ClusterIP
  selector:
    app: web-frontend
  ports:
  - port: 80
    targetPort: 80
EOF
```
### Step 4: 配置 Ingress 路由 (30min)

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 4.1 创建 Ingress 规则
cat <<EOF | kubectl apply -f -
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: app-ingress
  namespace: microservice-demo
  annotations:
    nginx.ingress.kubernetes.io/proxy-body-size: "10m"
    nginx.ingress.kubernetes.io/proxy-connect-timeout: "30"
spec:
  ingressClassName: nginx
  rules:
  - host: app.training.local
    http:
      paths:
      - path: /
        pathType: Prefix
        backend:
          service:
            name: web-frontend-svc
            port:
              number: 80
EOF

# 4.2 验证 Ingress
kubectl get ingress -n microservice-demo
INGRESS_IP=$(kubectl get svc -n kube-system nginx-ingress-lb -o jsonpath='{.status.loadBalancer.ingress[0].ip}')
curl -H "Host: app.training.local" http://${INGRESS_IP}/
```
### Step 5: 网络连通性验证 (20min)

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 5.1 服务发现测试
kubectl run net-test -n microservice-demo \
  --image=registry.cn-hangzhou.aliyuncs.com/acs-sample/busybox:1.36 \
  --rm -it --restart=Never -- sh -c '
  echo "=== DNS 解析 ==="
  nslookup web-frontend-svc
  nslookup db-svc
  nslookup cache-svc
  echo "=== 连通性 ==="
  wget -qO- --timeout=5 http://web-frontend-svc || echo "Web: OK (或超时)"
  echo "=== Endpoints ==="
  nslookup db-svc
'

# 5.2 跨 Namespace 访问测试
kubectl run cross-ns-test --rm -it --restart=Never \
  --image=registry.cn-hangzhou.aliyuncs.com/acs-sample/busybox:1.36 \
  -- wget -qO- --timeout=5 http://web-frontend-svc.microservice-demo.svc.cluster.local

# 5.3 全面状态检查
echo "=== Pods ==="
kubectl get pods -n microservice-demo -o wide
echo "=== Services ==="
kubectl get svc -n microservice-demo
echo "=== Endpoints ==="
kubectl get endpoints -n microservice-demo
echo "=== PVC ==="
kubectl get pvc -n microservice-demo
echo "=== Ingress ==="
kubectl get ingress -n microservice-demo
```
---

## 验收清单

- [ ] 数据库使用 StatefulSet + 独立 PVC 部署
- [ ] Web 前端 3 副本分布在不同节点
- [ ] Service 网络连通 (ClusterIP + Headless)
- [ ] Ingress 路由配置正确且可访问
- [ ] DNS 服务发现正常工作
- [ ] 跨 Namespace 访问正常

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
kubectl delete namespace microservice-demo  # ⚠️ 不可逆：永久删除命名空间及全部资源
```
## Related

- [[domain-19-landscape-references/topic-index/pvc-index.md|PVC 知识图谱索引]]


<!-- risk-assessed -->
