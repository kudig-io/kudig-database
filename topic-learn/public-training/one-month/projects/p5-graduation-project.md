---
title: '项目 P5: 毕业综合实践项目'
description: 'title: 项目 P5: 毕业综合实践项目'
category: learning
tags:
- k8s
- training
- hands-on
- prometheus
- grafana
- helm
- argocd
- hpa
- statefulset
- ingress
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- '项目 P5: 毕业综合实践项目 是什么'
- '如何 项目 P5: 毕业综合实践项目'
trigger_keywords:
- 项目
- 'P5:'
- 毕业综合实践项目
- learn
---


---
title: 项目 P5: 毕业综合实践项目
last_updated: 2026-05-18
difficulty: advanced
intent_queries:
  - kubernetes 毕业项目生产级平台搭建完整方案
  - k8s 全栈部署包含哪些组件
  - argocd gitops 完整项目实战
  - pvc prometheus grafana loki 一体化部署
trigger_keywords:
  - GitOps
  - ArgoCD
  - Prometheus
  - Grafana
  - Loki
  - 毕业项目
  - 生产级架构
  - 全栈部署
  - 故障排查手册
  - 变更管理
reading_level: advanced
audience:
  - sre-engineer
  - devops-engineer
  - platform-engineer
estimated_read_time: 180min
related_domains:
  - domain-4-workloads
  - domain-5-networking
  - domain-6-storage
  - domain-7-security
  - domain-8-observability
  - domain-12-troubleshooting
  - domain-18-production-operations
  - domain-23-gitops-ci-cd
related_topics:
  - topic-learn/public-training/one-month/projects/p1-k8s-cluster-setup
  - topic-learn/public-training/one-month/projects/p2-production-app-orchestration
  - topic-learn/public-training/one-month/projects/p3-observability-fault-drill
  - topic-learn/public-training/one-month/projects/p4-gitops-pipeline
---

# 项目 P5: 毕业综合实践项目

> **所属周**: Week 4 | **预计时间**: 2.5+ 小时

---

## 概述

本毕业项目是整个一个月学习计划的综合考核。你将搭建一个完整的生产级 K8s 平台，综合运用所学的所有知识：应用编排（Deployment + StatefulSet）、网络存储（Ingress + PVC + NetworkPolicy）、安全（RBAC + Pod Security）、可观测性（Prometheus + Loki）、GitOps（ArgoCD）和运维文档。完成此项目意味着你已经具备了独立管理生产级 K8s 集群的能力。

### 项目目标

搭建一个完整的生产级 K8s 平台，综合运用一个月所学的所有知识：
- 应用编排: Deployment + StatefulSet + HPA
- 网络存储: Ingress + PVC + NetworkPolicy
- 安全: RBAC + Pod Security
- 可观测性: Prometheus + Loki
- GitOps: ArgoCD
- 运维: 故障排查手册

### 前置条件

- 完成前四周全部课程
- 有运行中的 K8s 集群（kind/minikube/ACK 均可）
- 已安装 Helm、ArgoCD
- 已部署 kube-prometheus-stack

---

## 项目架构

```
┌─────────────────────────────────────────────────────────────┐
│                        Ingress                               │
│                    (TLS + 域名路由)                           │
├─────────────────────────────────────────────────────────────┤
│  ┌─────────────┐    ┌─────────────┐    ┌─────────────┐     │
│  │  Frontend   │    │   Backend   │    │   Backend   │     │
│  │ Deployment  │───▶│ StatefulSet │───▶│ StatefulSet │     │
│  │  (HPA)      │    │   (API-1)   │    │   (DB)      │     │
│  └─────────────┘    └─────────────┘    └─────────────┘     │
│         │                  │                  │             │
│         └──────────────────┴──────────────────┘             │
│                            │                                 │
│                   NetworkPolicy                              │
├─────────────────────────────────────────────────────────────┤
│                        Storage                               │
│               (StorageClass + PVC)                          │
├─────────────────────────────────────────────────────────────┤
│  ┌─────────────┐    ┌─────────────┐    ┌─────────────┐     │
│  │ Prometheus  │    │   Grafana   │    │    Loki     │     │
│  │ + Alertmgr  │    │             │    │ + Promtail  │     │
│  └─────────────┘    └─────────────┘    └─────────────┘     │
├─────────────────────────────────────────────────────────────┤
│                        ArgoCD                                │
│                   (GitOps Pipeline)                          │
└─────────────────────────────────────────────────────────────┘
```

---

## 验收清单

### 1. 应用编排

- [ ] 前端 Deployment（至少 2 副本）
- [ ] 前端 HPA（CPU 阈值 70%）
- [ ] 后端 StatefulSet（至少 2 副本）
- [ ] 每个 StatefulSet Pod 有独立 PVC
- [ ] 配置了 liveness/readiness 探针
- [ ] 配置了合理的 resources

### 2. 网络

- [ ] ClusterIP Service 用于内部通信
- [ ] Ingress 配置路由规则
- [ ] Ingress 配置 TLS（可以是自签名）
- [ ] NetworkPolicy 限制 Pod 间访问

### 3. 存储

- [ ] 使用 StorageClass 动态供应
- [ ] PVC 成功绑定
- [ ] 数据在 Pod 重启后持久化

### 4. 安全

- [ ] 创建专用 ServiceAccount
- [ ] 配置 RBAC（最小权限）
- [ ] Pod 以非 root 用户运行
- [ ] 配置 securityContext

### 5. 可观测性

- [ ] Prometheus 采集应用指标
- [ ] 配置至少 3 条告警规则
- [ ] Grafana Dashboard 可视化
- [ ] Loki 收集应用日志
- [ ] Alertmanager 路由配置

### 6. GitOps

- [ ] 应用配置存储在 Git 仓库
- [ ] ArgoCD Application 配置完成
- [ ] 修改 Git 能触发同步

### 7. 文档

- [ ] 架构设计文档
- [ ] 部署操作手册
- [ ] 故障排查手册（基于 FTA/FEBM）
- [ ] 变更管理 SOP

---

## 实施步骤

### Phase 1: 基础设施 (30min)

```bash
# 创建 namespace
kubectl create namespace graduation-project
# 预期输出: namespace/graduation-project created

# 确认监控组件就绪
kubectl get pods -n monitoring
# 预期输出: Prometheus、Grafana、Loki 等 Pod Running

# 确认 ArgoCD 就绪
kubectl get pods -n argocd
# 预期输出: argocd-server、argocd-repo-server 等 Pod Running

# 查看集群资源
kubectl get nodes -o wide
kubectl get sc
```

### Phase 2: 应用部署 (45min)

#### 2.1 创建 ServiceAccount 和 RBAC

```bash
cat <<EOF | kubectl apply -f -
apiVersion: v1
kind: ServiceAccount
metadata:
  name: app-sa
  namespace: graduation-project
---
apiVersion: rbac.authorization.k8s.io/v1
kind: Role
metadata:
  name: app-role
  namespace: graduation-project
rules:
- apiGroups: [""]
  resources: ["configmaps"]
  verbs: ["get", "list"]
  resourceNames: ["app-config"]
- apiGroups: [""]
  resources: ["secrets"]
  verbs: ["get"]
  resourceNames: ["app-secret"]
---
apiVersion: rbac.authorization.k8s.io/v1
kind: RoleBinding
metadata:
  name: app-rolebinding
  namespace: graduation-project
subjects:
- kind: ServiceAccount
  name: app-sa
roleRef:
  kind: Role
  name: app-role
  apiGroup: rbac.authorization.k8s.io
EOF
```

#### 2.2 部署前端 Deployment + HPA

```bash
cat <<EOF | kubectl apply -f -
apiVersion: apps/v1
kind: Deployment
metadata:
  name: frontend
  namespace: graduation-project
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
      serviceAccountName: app-sa
      securityContext:
        runAsNonRoot: true
        runAsUser: 101
        fsGroup: 101
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
          initialDelaySeconds: 10
          periodSeconds: 10
        readinessProbe:
          httpGet:
            path: /
            port: 80
          initialDelaySeconds: 5
          periodSeconds: 5
        securityContext:
          allowPrivilegeEscalation: false
          readOnlyRootFilesystem: false
---
apiVersion: autoscaling/v2
kind: HorizontalPodAutoscaler
metadata:
  name: frontend-hpa
  namespace: graduation-project
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
```

#### 2.3 部署后端 StatefulSet + PVC

```bash
cat <<EOF | kubectl apply -f -
apiVersion: v1
kind: Service
metadata:
  name: backend-headless
  namespace: graduation-project
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
  namespace: graduation-project
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
      securityContext:
        runAsNonRoot: true
        runAsUser: 101
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
          storage: 1Gi
EOF
```

### Phase 3: 网络配置 (20min)

```bash
cat <<EOF | kubectl apply -f -
apiVersion: v1
kind: Service
metadata:
  name: frontend
  namespace: graduation-project
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
  namespace: graduation-project
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
  namespace: graduation-project
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
  tls:
  - hosts:
    - app.local
    secretName: app-tls
---
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: frontend-policy
  namespace: graduation-project
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
  namespace: graduation-project
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
```

### Phase 4: 可观测性 (30min)

```bash
cat <<EOF | kubectl apply -f -
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: graduation-alerts
  namespace: monitoring
  labels:
    prometheus: kube-prometheus
    role: alert-rules
spec:
  groups:
  - name: graduation-alerts
    rules:
    - alert: GraduationAppDown
      expr: kube_deployment_status_replicas_available{namespace="graduation-project"} == 0
      for: 2m
      labels:
        severity: critical
      annotations:
        summary: "Graduation app has no available replicas"
    - alert: GraduationHighCPU
      expr: namespace_name:container_cpu_usage_seconds_total:sum_rate{namespace="graduation-project"} > 0.8
      for: 5m
      labels:
        severity: warning
      annotations:
        summary: "Graduation app CPU usage > 80%"
    - alert: GraduationHighMemory
      expr: namespace_name:container_memory_usage_bytes:sum{namespace="graduation-project"} / namespace_name:kube_pod_container_resource_limits_memory_bytes:sum{namespace="graduation-project"} > 0.8
      for: 5m
      labels:
        severity: warning
      annotations:
        summary: "Graduation app memory usage > 80%"
EOF
```

### Phase 5: GitOps (30min)

```bash
# 创建 ArgoCD Application
cat <<EOF | kubectl apply -f -
apiVersion: argoproj.io/v1alpha1
kind: Application
metadata:
  name: graduation-project
  namespace: argocd
spec:
  project: default
  source:
    repoURL: <your-repo-url>
    targetRevision: HEAD
    path: graduation-project
  destination:
    server: https://kubernetes.default.svc
    namespace: graduation-project
  syncPolicy:
    automated:
      prune: true
      selfHeal: true
    syncOptions:
    - CreateNamespace=true
EOF
```

### Phase 6: 文档编写 (30min)

创建以下文档：

1. **architecture.md**: 架构设计文档
2. **deployment-guide.md**: 部署手册
3. **troubleshooting-handbook.md**: 故障排查手册（基于 FTA/FEBM）
4. **change-management-sop.md**: 变更管理 SOP

---

## 演示要点

1. **架构讲解**
   - 能够清晰解释整体架构
   - 能够解释组件间的关系

2. **操作演示**
   - 通过 Ingress 访问应用
   - 演示 HPA 自动扩容
   - 演示滚动更新

3. **故障演练**
   - 注入一个故障
   - 按 FTA 方法定位
   - 修复并验证

4. **GitOps 演示**
   - 修改 Git 仓库
   - 观察自动同步

---

## 评分标准

| 项目 | 分值 | 评分标准 |
|------|------|----------|
| 应用编排 | 15 | 完成所有组件部署 |
| 网络存储 | 15 | Ingress/PVC/NetworkPolicy 正常 |
| 安全 | 10 | RBAC/SecurityContext 配置 |
| 可观测性 | 15 | 监控告警日志完整 |
| GitOps | 10 | ArgoCD 自动同步 |
| 文档 | 15 | 文档完整清晰 |
| 演示 | 20 | 能够清晰讲解和演示 |
| **总分** | **100** | |

---

## 要点总结

| 阶段 | 产出 | 关键技术 |
|------|------|---------|
| 基础设施 | Namespace + SA + RBAC | kubectl, RBAC |
| 应用部署 | Deployment + StatefulSet + HPA | YAML, kubectl apply |
| 网络配置 | Service + Ingress + NetworkPolicy | 网络策略, TLS |
| 可观测性 | PrometheusRule + Dashboard | PromQL, Grafana |
| GitOps | ArgoCD Application | Kustomize, Git |
| 文档 | 4 份运维文档 | FTA/FEBM |

---

恭喜完成毕业项目！

---

## 延伸阅读

- [生产架构设计原则](../../domain-18-production-operations/01-production-architecture-design-principles.md)
- [FTA 故障树分析](../../../topic-fta/04-fta-core-principles.md)
- [FEBM 取证循证方法](../../../topic-febm/01-febm-theory-foundations.md)
- [ArgoCD 企业级 GitOps](../../domain-23-gitops-ci-cd/01-argo-cd-enterprise-gitops.md)
- [SLO/SLI 体系](../../domain-8-observability/18-slo-sli-system.md)
