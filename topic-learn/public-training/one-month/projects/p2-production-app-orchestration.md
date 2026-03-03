# 项目 P2: 生产级应用全栈编排

> **所属周**: Week 2 | **预计时间**: 2.5 小时

---

## 项目目标

部署一套包含前端、后端、存储、网络的完整应用，体验生产级应用编排:
- 前端: Deployment + HPA
- 后端: StatefulSet + PVC
- 网络: Service + Ingress + NetworkPolicy
- 存储: StorageClass + PVC

---

## 前置条件

- 已完成 Week 2 Day 8-13 的学习
- 已安装 Nginx Ingress Controller
- 了解 PV/PVC/StorageClass

---

## 项目步骤

### Step 1: 创建 Namespace (5min)

```bash
kubectl create namespace production-app
```

### Step 2: 部署后端 StatefulSet (30min)

```bash
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
        readinessProbe:
          httpGet:
            path: /
            port: 80
          initialDelaySeconds: 3
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
```

### Step 3: 部署前端 Deployment + HPA (30min)

```bash
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
        readinessProbe:
          httpGet:
            path: /
            port: 80
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
```

### Step 4: 创建 Service (15min)

```bash
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
EOF

kubectl apply -f services.yaml
```

### Step 5: 创建 Ingress (20min)

```bash
cat > ingress.yaml << 'EOF'
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: app-ingress
  namespace: production-app
  annotations:
    nginx.ingress.kubernetes.io/rewrite-target: /
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
```

### Step 6: 创建 NetworkPolicy (20min)

```bash
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
---
# 后端: 只允许前端访问
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
EOF

kubectl apply -f networkpolicy.yaml
```

### Step 7: 验证和测试 (30min)

```bash
# 查看所有资源
kubectl get all -n production-app
kubectl get pvc -n production-app
kubectl get ingress -n production-app
kubectl get networkpolicy -n production-app

# 测试访问
INGRESS_IP=$(kubectl get svc -n ingress-nginx ingress-nginx-controller -o jsonpath='{.status.loadBalancer.ingress[0].ip}')
curl -H "Host: app.local" http://$INGRESS_IP/
curl -H "Host: app.local" http://$INGRESS_IP/api

# 测试 HPA
kubectl get hpa -n production-app

# 测试滚动更新
kubectl set image deployment/frontend web=nginx:1.25 -n production-app
kubectl rollout status deployment/frontend -n production-app
```

---

## 验收清单

- [ ] StatefulSet 部署成功，Pod 名称有序
- [ ] 每个 StatefulSet Pod 有独立的 PVC
- [ ] Deployment 部署成功，HPA 配置正常
- [ ] Service 和 Endpoints 正常
- [ ] Ingress 路由正常工作
- [ ] NetworkPolicy 隔离有效
- [ ] 滚动更新正常

---

## 清理资源

```bash
kubectl delete namespace production-app
```
