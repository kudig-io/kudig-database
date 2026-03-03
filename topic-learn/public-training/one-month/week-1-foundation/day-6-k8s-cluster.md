# Day 6: K8s 架构深化 + 集群配置

> **学习时间**: 4-5 小时 | **主题**: 深入理解集群配置与声明式管理

---

## 今日目标

- [ ] 理解 K8s 集群配置参数
- [ ] 掌握 API 版本和特性门控
- [ ] 部署第一个 Deployment，体验声明式管理

---

## 理论学习 (2h)

### 必读文档

1. **集群配置参数**
   - 文件: `../../domain-1-architecture-fundamentals/06-cluster-configuration-parameters.md`
   - 重点: 关键配置项含义，生产环境配置建议

2. **API 版本与特性**
   - 文件: `../../domain-1-architecture-fundamentals/03-api-versions-features.md`
   - 重点: API 版本演进 (alpha/beta/stable)、特性门控

### 补充阅读

3. **速查手册**
   - 文件: `../../topic-cheat-sheet/k8s.md`
   - 重点: 标记不理解的命令，稍后实践

---

## 实践任务 (2.5h)

### 任务 1: 部署第一个 Deployment (45min)

```bash
# 创建测试 namespace
kubectl create namespace learn-k8s

# 方式 1: 命令行创建
kubectl create deployment nginx --image=nginx:alpine -n learn-k8s

# 查看创建的资源
kubectl get deployment -n learn-k8s
kubectl get replicaset -n learn-k8s
kubectl get pods -n learn-k8s

# 方式 2: YAML 文件创建 (推荐)
cat > nginx-deployment.yaml << 'EOF'
apiVersion: apps/v1
kind: Deployment
metadata:
  name: nginx-deployment
  namespace: learn-k8s
  labels:
    app: nginx
spec:
  replicas: 3
  selector:
    matchLabels:
      app: nginx
  template:
    metadata:
      labels:
        app: nginx
    spec:
      containers:
      - name: nginx
        image: nginx:alpine
        ports:
        - containerPort: 80
        resources:
          requests:
            cpu: 100m
            memory: 128Mi
          limits:
            cpu: 200m
            memory: 256Mi
EOF

kubectl apply -f nginx-deployment.yaml

# 查看 Deployment 详情
kubectl describe deployment nginx-deployment -n learn-k8s
```

### 任务 2: 体验声明式管理 (45min)

```bash
# 修改副本数
kubectl scale deployment nginx-deployment --replicas=5 -n learn-k8s

# 观察 Pod 变化
kubectl get pods -n learn-k8s -w

# 修改 YAML 后重新 apply (声明式)
# 将 replicas 改为 2
sed -i 's/replicas: 3/replicas: 2/' nginx-deployment.yaml
kubectl apply -f nginx-deployment.yaml

# 查看 ReplicaSet 历史
kubectl get replicaset -n learn-k8s

# 滚动更新 (修改镜像版本)
kubectl set image deployment/nginx-deployment nginx=nginx:1.25 -n learn-k8s

# 观察滚动更新过程
kubectl rollout status deployment/nginx-deployment -n learn-k8s

# 查看更新历史
kubectl rollout history deployment/nginx-deployment -n learn-k8s

# 回滚到上一版本
kubectl rollout undo deployment/nginx-deployment -n learn-k8s
```

### 任务 3: 创建 Service 暴露应用 (30min)

```bash
# 创建 Service
cat > nginx-service.yaml << 'EOF'
apiVersion: v1
kind: Service
metadata:
  name: nginx-service
  namespace: learn-k8s
spec:
  selector:
    app: nginx
  ports:
  - port: 80
    targetPort: 80
  type: ClusterIP
EOF

kubectl apply -f nginx-service.yaml

# 查看 Service
kubectl get svc -n learn-k8s
kubectl describe svc nginx-service -n learn-k8s

# 查看 Endpoints
kubectl get endpoints nginx-service -n learn-k8s

# 测试访问 (在集群内)
kubectl run curl --image=curlimages/curl -it --rm -- curl nginx-service.learn-k8s.svc.cluster.local
```

### 任务 4: 查看集群事件 (30min)

```bash
# 查看 namespace 事件
kubectl get events -n learn-k8s --sort-by='.lastTimestamp'

# 查看特定资源的事件
kubectl describe pod <pod-name> -n learn-k8s | grep -A 20 Events

# 实时监控事件
kubectl get events -n learn-k8s -w

# 模拟故障场景
kubectl set image deployment/nginx-deployment nginx=nginx:nonexistent -n learn-k8s

# 观察事件
kubectl get events -n learn-k8s | grep -i error

# 回滚修复
kubectl rollout undo deployment/nginx-deployment -n learn-k8s
```

### 任务 5: API 资源探索 (30min)

```bash
# 查看所有 API 资源
kubectl api-resources

# 查看资源的 API 版本
kubectl api-resources | grep deployment

# 查看资源的 YAML 结构
kubectl explain deployment
kubectl explain deployment.spec
kubectl explain deployment.spec.template.spec.containers

# 获取资源的完整 YAML
kubectl get deployment nginx-deployment -n learn-k8s -o yaml

# 使用 --dry-run 预览
kubectl create deployment test --image=nginx --dry-run=client -o yaml
```

---

## 费曼复述 (0.5h)

用自己的语言回答:

1. **Deployment、ReplicaSet、Pod 三者的关系是什么？**
   - Deployment 管理 ReplicaSet
   - ReplicaSet 管理 Pod
   - 滚动更新时会创建新的 ReplicaSet

2. **什么是"声明式管理"？与"命令式管理"有什么区别？**
   - 声明式: 描述期望状态，系统自动 reconcile
   - 命令式: 直接执行操作

3. **Service 是如何找到 Pod 的？**
   - Label Selector 匹配

---

## 今日检验

- [ ] 能够编写 Deployment YAML 并部署应用
- [ ] 能够执行滚动更新和回滚操作
- [ ] 能够创建 Service 暴露应用
- [ ] 理解声明式管理的核心思想

---

## 关键 YAML 模板

```yaml
# Deployment 模板
apiVersion: apps/v1
kind: Deployment
metadata:
  name: <name>
  namespace: <namespace>
spec:
  replicas: <number>
  selector:
    matchLabels:
      app: <label>
  template:
    metadata:
      labels:
        app: <label>
    spec:
      containers:
      - name: <container-name>
        image: <image>
        ports:
        - containerPort: <port>
        resources:
          requests:
            cpu: <cpu>
            memory: <memory>
          limits:
            cpu: <cpu>
            memory: <memory>
```

---

## 明日预告

Day 7 是本周复习日，将完成综合实践项目 P1: 从零搭建一个可运行 nginx 的 K8s 集群。
