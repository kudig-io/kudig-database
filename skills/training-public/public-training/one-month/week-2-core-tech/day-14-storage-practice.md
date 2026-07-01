---
title: 'Day 14: 存储体系 + 综合实践'
description: '# Day 14: 存储体系 + 综合实践'
category: learning
tags:
- k8s
- training
- hands-on
- etcd
- scheduler
- hpa
- statefulset
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
- 'Day 14: 存储体系 + 综合实践 是什么'
- '如何 Day 14: 存储体系 + 综合实践'
trigger_keywords:
- Day
- '14:'
- 存储体系
- 综合实践
- learn
prerequisites:
- kubectl-basics
- gpu-ml-basics
- etcd-basics
created: "2026-05-23"
---

# Day 14: 存储体系 + 综合实践

```yaml
---
id: LEARN-ONE-MONTH-W2-DAY14
title: Day 14 - 存储体系 + 综合实践
topic: [[entities/kubernetes.md|kubernetes]]
type: hands-on-guide
tags: [pv, pvc, storageclass, dynamic-provisioning, statefulset, csi, hands-on, week-2]
last_updated: 2026-05-18
difficulty: intermediate
intent_queries:
  - "PV/PVC 静态供应怎么配置"
  - "StorageClass 动态供应怎么用"
  - "NFS 存储怎么配置"
  - "StatefulSet 存储怎么管理"
trigger_keywords:
  - PersistentVolume
  - PersistentVolumeClaim
  - StorageClass
  - Dynamic Provisioning
  - CSI
  - NFS
  - volumeClaimTemplate
  - reclaimPolicy
  - accessModes
  - WaitForFirstConsumer
reading_level: intermediate
audience:
  - sre
  - ops-engineer
estimated_read_time: 45min
related_domains:
  - domain-04-storage-data
  - domain-10-troubleshooting-diagnostics
related_topics:
  - storage
  - pv
  - pvc
  - statefulset
related:
  - domain-11-production-operations/topic-learn/public-training/one-month/week-2-core-tech/day-12-networking-1.md
  - domain-04-storage-data/04-storageclass-dynamic-provisioning.md
---
```

> **学习时间**: 4-5 小时 | **主题**: K8s 存储与 Week 2 总结

---

## 今日目标

- [ ] 掌握 PV/PVC/StorageClass 机制
- [ ] 理解动态存储供应
- [ ] 完成综合实践项目 P2

---

## 理论学习 (2h)

### 必读文档

1. **存储架构总览**
   - 文件: `../../domain-04-storage-data/01-storage-architecture-overview.md`
   - 重点: 存储架构全貌

2. **PV 架构基础**
   - 文件: `../../domain-04-storage-data/02-pv-architecture-fundamentals.md`
   - 重点: PV/PVC 绑定机制

3. **StorageClass 动态供应**
   - 文件: `../../domain-04-storage-data/04-storageclass-dynamic-provisioning.md`
   - 重点: 动态供应、CSI 驱动

---

## 实践任务 (2.5h)

### 任务 1: PV/PVC 静态供应 (30min)

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源
> - `kubectl delete`：删除资源（可由声明式清单重建）
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

```bash
# 创建 PV (hostPath 类型，仅用于测试)
cat > pv-static.yaml << 'EOF'
apiVersion: v1
kind: PersistentVolume
metadata:
  name: pv-static
spec:
  capacity:
    storage: 1Gi
  accessModes:
    - ReadWriteOnce
  persistentVolumeReclaimPolicy: Retain
  hostPath:
    path: /tmp/pv-static
EOF

kubectl apply -f pv-static.yaml

# 查看 PV
kubectl get pv

# 创建 PVC
cat > pvc-static.yaml << 'EOF'
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: pvc-static
spec:
  accessModes:
    - ReadWriteOnce
  resources:
    requests:
      storage: 500Mi
EOF

kubectl apply -f pvc-static.yaml

# 查看绑定状态
kubectl get pv,pvc

# 使用 PVC
cat > pod-pvc.yaml << 'EOF'
apiVersion: v1
kind: Pod
metadata:
  name: pod-pvc-test
spec:
  containers:
  - name: app
    image: nginx:alpine
    volumeMounts:
    - name: data
      mountPath: /data
  volumes:
  - name: data
    persistentVolumeClaim:
      claimName: pvc-static
EOF

kubectl apply -f pod-pvc.yaml

# 验证存储
kubectl exec pod-pvc-test -- sh -c "echo 'Hello PVC' > /data/test.txt"
kubectl exec pod-pvc-test -- cat /data/test.txt

# 清理
kubectl delete pod pod-pvc-test
kubectl delete pvc pvc-static
kubectl delete pv pv-static
```

### 任务 2: StorageClass 动态供应 (30min)

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源
> - `kubectl delete`：删除资源（可由声明式清单重建）

```bash
# 查看现有 StorageClass
kubectl get storageclass

# 创建 StorageClass (以 hostpath 为例，生产环境使用云存储 CSI)
cat > storageclass.yaml << 'EOF'
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: fast-storage
provisioner: rancher.io/local-path  # 或其他 CSI 驱动
reclaimPolicy: Delete
volumeBindingMode: WaitForFirstConsumer
EOF

# 注意: 需要安装对应的 CSI 驱动
# 对于 kind 集群，可以使用默认的 standard StorageClass

# 使用动态供应创建 PVC
cat > pvc-dynamic.yaml << 'EOF'
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: pvc-dynamic
spec:
  storageClassName: standard  # 使用默认或指定的 StorageClass
  accessModes:
    - ReadWriteOnce
  resources:
    requests:
      storage: 1Gi
EOF

kubectl apply -f pvc-dynamic.yaml

# 查看动态创建的 PV
kubectl get pv,pvc

# 清理
kubectl delete pvc pvc-dynamic
```

### 任务 3: 综合实践项目 P2 (1.5h)

**项目: 生产级应用全栈编排**

详细指南见: [../projects/p2-production-app-orchestration.md](../projects/p2-production-app-orchestration.md)

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

```bash
# 创建 namespace
kubectl create namespace production-app

# 1. 创建后端 StatefulSet + PVC
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

# 2. 创建前端 Deployment + HPA
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
          initialDelaySeconds: 5
        readinessProbe:
          httpGet:
            path: /
            port: 80
          initialDelaySeconds: 3
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

# 3. 创建 Service
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

# 4. 创建 Ingress
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

# 5. 创建 NetworkPolicy
cat > networkpolicy.yaml << 'EOF'
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

# 验证部署
kubectl get all -n production-app
kubectl get pvc -n production-app
kubectl get ingress -n production-app
kubectl get networkpolicy -n production-app
```

---

## 费曼复述 (0.5h)

用自己的语言回答:

1. **PV、PVC、StorageClass 三者的关系是什么？**
   - PV: 实际存储资源
   - PVC: 存储请求，与 PV 绑定
   - StorageClass: 动态供应的模板

2. **accessModes (RWO/ROX/RWX) 分别表示什么？**
   - RWO: 单节点读写
   - ROX: 多节点只读
   - RWX: 多节点读写

3. **Reclaim Policy (Retain/Delete) 的区别？**
   - Retain: PVC 删除后保留 PV 和数据
   - Delete: PVC 删除后自动删除 PV 和数据

---

## 自测检验

完成 checkpoint.md](./checkpoint.md) 中的 Week 2 自测题。

---

## Week 2 总结

### 学习路径

```
Day 8-9:  控制平面 (etcd, API Server, Scheduler, Controller)
Day 10-11: 工作负载 (Deployment, StatefulSet, Pod 生命周期, HPA)
Day 12-13: 网络栈 (CNI, Service, DNS, Ingress, NetworkPolicy)
Day 14:   存储体系 (PV, PVC, StorageClass) + 综合实践
```

### 关键收获

1. **控制平面**: 理解 K8s 大脑如何工作
2. **工作负载**: 掌握不同应用类型的管理方式
3. **网络**: 理解 Pod 通信和服务发现机制
4. **存储**: 掌握持久化存储的配置方法

---

## 清理资源

> ⚠️ **🔴 灾难性操作** — 含不可逆命令，执行前必须满足变更窗口+双人复核+事前备份+回滚方案
> - `kubectl delete namespace`：永久删除命名空间及全部资源，不可恢复

```bash
kubectl delete namespace production-app  # ⚠️ 不可逆：永久删除命名空间及全部资源
```

恭喜完成 Week 2 的学习!
