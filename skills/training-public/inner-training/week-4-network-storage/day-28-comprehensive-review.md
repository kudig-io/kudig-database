---
title: Day 28: 综合复习与实践 [week-4-network-storage]
description: '# Day 28: 综合复习与实践'
category: learning
tags:
- k8s
- training
- hands-on
- prometheus
- flannel
- ingress
- rbac
- operator
- rag
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
created: "2026-05-23"
---

# Day 28: 综合复习与实践

```yaml
---
title: Day 28: 综合复习与实践
last_updated: 2026-05-18
difficulty: intermediate
intent_queries:
  - "Kubernetes四周培训复习"
  - "综合复习题目"
  - "培训自测"
  - "K8s知识点回顾"
trigger_keywords:
  - "综合复习"
  - "四周总结"
  - "自测"
  - "培训考核"
  - "K8s复习"
reading_level: intermediate
audience:
  - sre工程师
  - ops工程师
  - 运维工程师
estimated_read_time: 45min
related_domains:
  - domain-01-cluster-fundamentals
  - domain-02-workloads-applications
  - domain-03-networking-traffic
  - domain-04-storage-data
  - domain-10-troubleshooting-diagnostics
related_topics:
  - domain-11-production-operations/topic-learn/inner-training/week-4-network-storage/checkpoint
  - domain-11-production-operations/topic-learn/inner-training/week-4-network-storage/day-22-service-basics
  - domain-11-production-operations/topic-learn/inner-training/week-4-network-storage/day-26-storage-create-delete
id: WEEK4-DAY28
topic: training
type: review
tags: [week-4, day-28, review, checkpoint, k8s, k8s-1.28-1.33]
---
```

> **学习时间**: 4-5 小时 | **主题**: 全流程实操与问题答疑

---

## 今日目标

- [ ] 回顾 4 周核心知识点
- [ ] 完成端到端的综合实操演练
- [ ] 识别个人薄弱环节并制定补强计划
- [ ] 为毕业项目做准备

---

## 理论回顾 (1.5h)

### 四周知识图谱

```
Week 1: ACK/ACR 基础与集群生命周期
├── ACK/ACR 服务架构与管控
├── SDK & API 调用
├── 控制台操作
├── 集群创建 → 删除 → 升级 → 证书
└── 关键: aliyun CLI, OpenAPI

Week 2: 安全认证与监控运维
├── RBAC 权限模型
├── RAM 账号集成
├── 漏洞与风险防范
├── 审计日志 (SLS)
├── 监控告警 (Prometheus/ARMS)
└── 配额管理 (ResourceQuota/LimitRange)

Week 3: 节点与工作负载管理
├── Node 节点基础与进阶
├── 节点池管理与自动伸缩
├── Pod 生命周期与调度
├── 健康探针与资源管理
└── K8S 核心组件运维

Week 4: 网络与存储
├── Service (ClusterIP/NodePort/LoadBalancer)
├── Ingress (Nginx/ALB Controller)
├── Terway vs Flannel CNI
├── PV/PVC/StorageClass
└── 存储挂载与扩容
```

### 自检清单

请逐项回顾，标记掌握程度 (✅ 熟练 / ⚠️ 了解 / ❌ 需补强):

| 编号 | 核心技能 | 掌握度 |
|:---:|---------|:-----:|
| 1 | 使用 aliyun CLI 调用 ACK API | |
| 2 | 独立创建/升级/删除 ACK 集群 | |
| 3 | 配置 RBAC + RAM 权限 | |
| 4 | 搭建 Prometheus 监控 | |
| 5 | 管理节点池与自动伸缩 | |
| 6 | Pod 调度策略与探针配置 | |
| 7 | 排查 kube-system 组件异常 | |
| 8 | 配置 Service 和 Ingress | |
| 9 | Terway/Flannel 网络原理与排查 | |
| 10 | PV/PVC 创建、挂载与扩容 | |

---

## 综合实操演练 (2.5h)

### 演练: 从零到一部署完整应用

> 模拟真实场景：在 ACK 集群中部署一个 Web 应用，包含完整的网络暴露、存储持久化、监控配置。

#### Phase 1: 环境准备 (20min)

```bash
# 创建专用 Namespace
kubectl create namespace final-demo

# 设置 ResourceQuota
cat <<EOF | kubectl apply -f -
apiVersion: v1
kind: ResourceQuota
metadata:
  name: demo-quota
  namespace: final-demo
spec:
  hard:
    requests.cpu: "4"
    requests.memory: 4Gi
    limits.cpu: "8"
    limits.memory: 8Gi
    persistentvolumeclaims: "5"
    pods: "20"
EOF

# 创建 LimitRange
cat <<EOF | kubectl apply -f -
apiVersion: v1
kind: LimitRange
metadata:
  name: demo-limits
  namespace: final-demo
spec:
  limits:
  - default:
      cpu: 500m
      memory: 256Mi
    defaultRequest:
      cpu: 100m
      memory: 128Mi
    type: Container
EOF

kubectl get quota,limitrange -n final-demo
```

#### Phase 2: 应用部署 (30min)

```bash
# 创建 ConfigMap 和 Secret
kubectl create configmap app-config -n final-demo \
  --from-literal=APP_NAME="ACK Training Demo" \
  --from-literal=LOG_LEVEL=info

kubectl create secret generic app-secret -n final-demo \
  --from-literal=API_KEY=training-demo-key-2024

# 创建 PVC
cat <<EOF | kubectl apply -f -
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: app-logs
  namespace: final-demo
spec:
  accessModes: [ReadWriteOnce]
  storageClassName: alicloud-disk-ssd
  resources:
    requests:
      storage: 20Gi
EOF

# 创建 Deployment
cat <<EOF | kubectl apply -f -
apiVersion: apps/v1
kind: Deployment
metadata:
  name: web-app
  namespace: final-demo
spec:
  replicas: 3
  selector:
    matchLabels:
      app: web-app
  template:
    metadata:
      labels:
        app: web-app
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
                  values: [web-app]
              topologyKey: [[entities/kubernetes|kubernetes]].io/hostname
      containers:
      - name: web
        image: registry.cn-hangzhou.aliyuncs.com/acs-sample/nginx:1.24
        ports:
        - containerPort: 80
        env:
        - name: APP_NAME
          valueFrom:
            configMapKeyRef:
              name: app-config
              key: APP_NAME
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
        volumeMounts:
        - name: config
          mountPath: /etc/app
        - name: logs
          mountPath: /var/log/nginx
      volumes:
      - name: config
        configMap:
          name: app-config
      - name: logs
        persistentVolumeClaim:
          claimName: app-logs
EOF

kubectl get pods -n final-demo -w
```

#### Phase 3: 服务暴露 (20min)

```bash
# 创建 Service
cat <<EOF | kubectl apply -f -
apiVersion: v1
kind: Service
metadata:
  name: web-app-svc
  namespace: final-demo
spec:
  type: ClusterIP
  selector:
    app: web-app
  ports:
  - port: 80
    targetPort: 80
EOF

# 创建 Ingress
cat <<EOF | kubectl apply -f -
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: web-app-ingress
  namespace: final-demo
spec:
  ingressClassName: nginx
  rules:
  - host: final-demo.example.com
    http:
      paths:
      - path: /
        pathType: Prefix
        backend:
          service:
            name: web-app-svc
            port:
              number: 80
EOF

# 验证
kubectl get svc,ingress -n final-demo
```

#### Phase 4: 验证与清理 (20min)

```bash
# 全面检查
echo "=== Pod 状态 ==="
kubectl get pods -n final-demo -o wide

echo "=== Service 与 Endpoints ==="
kubectl get svc,endpoints -n final-demo

echo "=== Ingress ==="
kubectl describe ingress -n final-demo

echo "=== 资源使用 ==="
kubectl describe quota demo-quota -n final-demo

echo "=== 存储 ==="
kubectl get pvc -n final-demo

# 连通性测试
kubectl run test -n final-demo \
  --image=registry.cn-hangzhou.aliyuncs.com/acs-sample/busybox:1.36 \
  --rm -it --restart=Never -- wget -qO- http://web-app-svc

# 清理
kubectl delete namespace final-demo
```

---

## 费曼复述 (0.5h)

1. **回顾这 4 周学到的最核心的 3 个知识点是什么？**
2. **如果要向新同事讲解 ACK 集群的日常运维流程，你会怎么组织？**
3. **ACK 与自建 K8S 集群相比，最大的运维差异在哪里？**

---

## 今日检验

- [ ] 完成端到端应用部署演练
- [ ] 填写自检清单并识别薄弱环节
- [ ] 制定个人补强计划
- [ ] 准备开始毕业项目

---

## 下一步

1. 完成 [Week 4 自测](checkpoint.md)
2. 开始 [毕业综合项目](../projects/p5-graduation-project.md)
3. 参考 [知识图谱](../resources/knowledge-map.md) 进行系统回顾
