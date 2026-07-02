---
title: 'Day 28: 综合复习 + 毕业项目'
description: '- K8s 综合实践'
summary: '今天是整个一个月学习计划的最后一天，也是最重要的一天。前三周你学习了大量的知识点——Docker 基础、Linux 运维、K8s 架构、工作负载管理、网络存储、安全体系、监控告警、GitOps、故障排查方法论。今天是检验你是否真正掌握这些知识的时刻。'
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
- helm
- argocd
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 'Day 28: 综合复习 + 毕业项目 是什么'
- '如何 Day 28: 综合复习 + 毕业项目'
trigger_keywords:
- Day
- '28:'
- 综合复习
- 毕业项目
- learn
prerequisites:
- kubectl-basics
- gpu-ml-basics
- helm-basics
- prometheus-basics
- monitoring-basics
- gitops-basics
- etcd-basics
- policy-basics
- logging-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




---
title: Day 28: 综合复习 + 毕业项目
last_updated: 2026-05-18
difficulty: advanced
intent_queries:
  - [[Kubernetes|Kubernetes]] 毕业项目
  - K8s 综合实践
  - 费曼复述法
  - 知识图谱绘制
trigger_keywords:
  - 毕业项目
  - 综合复习
  - P5
  - 费曼复述
  - 知识图谱
  - 一个月学习总结
  - 综合实践
reading_level: advanced
audience:
  - sre-engineer
  - devops-engineer
  - platform-engineer
estimated_read_time: 240min
related_domains:
  - domain-01-cluster-fundamentals
  - domain-11-production-operations
related_topics:
  - domain-11-production-operations/topic-learn/public-training/one-month/projects/p5-graduation-project
  - domain-11-production-operations/topic-learn/public-training/one-month/week-4-enterprise/checkpoint
  - domain-11-production-operations/topic-learn/public-training/one-month/README
---

# Day 28: 综合复习 + 毕业项目

## 概述

今天是整个一个月学习计划的最后一天，也是最重要的一天。前三周你学习了大量的知识点——Docker 基础、Linux 运维、K8s 架构、工作负载管理、网络存储、安全体系、监控告警、GitOps、故障排查方法论。今天是检验你是否真正掌握这些知识的时刻。

今天的核心活动是完成毕业综合实践项目（P5），它要求你从头搭建一个生产级的 K8s 平台，涵盖应用部署、网络配置、存储管理、安全加固、监控告警、日志采集和 GitOps 流水线。这个项目是对一个月学习成果的全面检验。

### 学习目标

- 通过费曼复述法巩固整月学习内容
- 完成毕业综合实践项目，检验综合能力
- 通过终极自测评估学习成果
- 制定后续持续学习计划

---

## 核心概念详解

### 知识体系梳理

一个月的学习内容可以归纳为以下几个能力域：

**集群管理能力**: 能够创建、配置、升级和维护 K8s 集群。核心知识点包括：ACK 集群类型选择、网络规划、节点池管理、集群升级策略、证书管理。这个能力域是所有其他能力的基础——没有健康的集群，其他一切都无从谈起。

**应用部署能力**: 能够设计并实施生产级的应用编排方案。核心知识点包括：Deployment 滚动更新、StatefulSet 有状态应用管理、HPA 自动伸缩、资源配置（requests/limits）、健康检查（探针）。这是日常运维中最频繁的工作。

**网络与存储能力**: 能够配置 K8s 的网络和存储方案。核心知识点包括：CNI 网络插件选择与配置、Service 四种类型、Ingress 路由规则、NetworkPolicy 网络隔离、PV/PVC/StorageClass 存储管理。网络和存储是运行生产应用的基础设施。

**安全合规能力**: 能够实施 K8s 集群的安全加固。核心知识点包括：RBAC 权限模型、ServiceAccount 管理、Pod 安全标准、Secret 管理、NetworkPolicy、审计日志。安全是生产环境的底线。

**可观测性能力**: 能够构建完整的监控、日志和追踪体系。核心知识点包括：Prometheus 指标采集、PromQL 查询、Grafana 可视化、Alertmanager 告警、Loki 日志聚合、分布式追踪。可观测性是运维的"眼睛"。

**故障排查能力**: 能够系统化地排查和解决 K8s 问题。核心知识点包括：FTA 故障树分析、FEBM 取证循证方法、Pod 排障流程、网络排障流程、节点排障流程。故障排查能力决定了你应对生产事故的水平。

**GitOps 与自动化能力**: 能够实施声明式的持续部署。核心知识点包括：ArgoCD 工作原理、GitOps 工作流、Helm Chart 管理、CRD 和 Operator 模式。自动化能力决定了团队的运维效率。

### 毕业项目的意义

毕业项目 P5 不是简单的练习，而是一个接近真实生产场景的综合挑战。它要求你：

1. **整体设计**: 从零开始设计一个完整的 K8s 应用平台架构
2. **分步实施**: 按照模块化的方式逐步实现各个功能
3. **验证测试**: 通过实际操作验证每个功能是否正确工作
4. **文档输出**: 编写架构设计文档、操作手册和故障排查手册

通过毕业项目，你可以：

- 发现自己知识的薄弱环节，有针对性地补强
- 建立从"学知识"到"做项目"的实战能力
- 产出可以在工作中直接使用的架构设计和技术文档
- 获得独立完成 K8s 平台搭建的信心

---

## 实战演练

### 第一部分: 费曼复述 (1.5h)

用自己的语言向"虚拟初学者"解释以下概念（可以录音或写笔记）：

**Week 1 核心概念复述**:

```
# 🟢 低风险：只读/信息收集，通常无副作用
1. 容器和虚拟机的区别是什么？为什么 K8s 选择容器？
   提示: 从隔离方式、启动速度、资源开销、适用场景四个角度解释

2. Docker 镜像是分层的，这有什么好处？
   提示: 层共享节省空间、构建缓存加速、快速创建容器

3. K8s 的 Master 和 Node 各自运行什么组件？它们如何协作？
   提示: API Server 是中心，所有组件通过 Watch/List 与 API Server 交互

4. kubectl 常用的命令有哪些？各自的使用场景？
   提示: get/describe/logs/exec/apply/delete
```
**Week 2 核心概念复述**:

```
1. etcd 在 K8s 中的作用？为什么它对磁盘 IO 要求高？
   提示: 存储所有集群状态、Raft 协议需要 fsync 保证数据持久性

2. Deployment 滚动更新是如何工作的？如何控制更新速度？
   提示: maxSurge/maxUnavailable、readinessProbe 保证可用性

3. Service 和 Ingress 的区别？各自的使用场景？
   提示: 四层 vs 七层、TCP/UDP vs HTTP/HTTPS

4. PV/PVC/StorageClass 的关系？动态供给是如何工作的？
   提示: PVC 声明需求 → StorageClass 定义类型 → CSI 驱动自动创建 PV
```

**Week 3 核心概念复述**:

```
1. RBAC 的四种资源如何配合实现权限控制？
   提示: Role 定义权限 → RoleBinding 绑定到用户/SA

2. Prometheus 的数据模型是什么？四种指标类型各自的特点？
   提示: 时间序列、Counter/Gauge/Histogram/Summary

3. FTA 和 FEBM 分别解决什么问题？如何配合使用？
   提示: FTA 构建排查框架、FEBM 在框架内以证据驱动决策
```

**Week 4 核心概念复述**:

```
# 🟢 低风险：只读/信息收集，通常无副作用
1. GitOps 的核心理念是什么？与传统 CI/CD 有什么区别？
   提示: Git 作为唯一真实来源、Pull vs Push 模式

2. Kyverno 的三种策略类型分别做什么？
   提示: Validate（验证）、Mutate（变更）、Generate（生成）

3. Helm Chart 的核心组成部分？
   提示: Chart.yaml、values.yaml、templates/
```
### 第二部分: 知识图谱绘制 (30min)

在纸上或白板上画出完整的 K8s 知识图谱：

```
K8s 知识图谱框架:

1. 架构层
   Master: API Server ← etcd, Scheduler, Controller Manager
   Node: kubelet ← Pod, kube-proxy ← Service
   关系: Master 通过 kubelet 管理 Node

2. 资源对象层
   Workloads: Pod → Deployment → StatefulSet → DaemonSet → Job
   Networking: Service → Ingress → NetworkPolicy → CNI
   Storage: PV → PVC → StorageClass → CSI
   Security: RBAC → SA → Secret → PodSecurity

3. 运维层
   Monitoring: Prometheus → Grafana → Alertmanager
   Logging: [[fluentd|Fluentd]]/Promtail → Loki/ELK
   CI/CD: Git → ArgoCD → Cluster
   Troubleshooting: FTA + FEBM
```

### 第三部分: 毕业项目 P5 (2.5h)

详细指南见: [../projects/p5-graduation-project.md](../projects/p5-graduation-project.md)

**项目: 生产级 K8s 平台完整搭建**

#### Step 1: 集群基础 (30min)

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 创建命名空间
kubectl create namespace production
kubectl create namespace monitoring
kubectl create namespace security

# 部署应用 (Deployment + StatefulSet)
cat > app-deployment.yaml << 'EOF'
apiVersion: apps/v1
kind: Deployment
metadata:
  name: web-app
  namespace: production
spec:
  replicas: 3
  selector:
    matchLabels:
      app: web-app
  strategy:
    type: RollingUpdate
    rollingUpdate:
      maxSurge: 1
      maxUnavailable: 0
  template:
    metadata:
      labels:
        app: web-app
    spec:
      containers:
      - name: nginx
        image: nginx:1.25-alpine
        ports:
        - containerPort: 80
        resources:
          requests:
            cpu: 100m
            memory: 128Mi
          limits:
            cpu: 200m
            memory: 256Mi
        readinessProbe:
          httpGet:
            path: /
            port: 80
          initialDelaySeconds: 5
          periodSeconds: 10
        livenessProbe:
          httpGet:
            path: /
            port: 80
          initialDelaySeconds: 15
          periodSeconds: 20
EOF

kubectl apply -f app-deployment.yaml
```
#### Step 2: 存储配置 (30min)

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 创建 StorageClass 和 PVC
cat > storage.yaml << 'EOF'
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: standard
provisioner: diskplugin.csi.alibabacloud.com
parameters:
  type: cloud_essd
  performanceLevel: PL1
reclaimPolicy: Retain
allowVolumeExpansion: true
---
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: data-pvc
  namespace: production
spec:
  accessModes:
  - ReadWriteOnce
  storageClassName: standard
  resources:
    requests:
      storage: 20Gi
EOF

kubectl apply -f storage.yaml
```
#### Step 3: 网络暴露 (30min)

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 创建 Ingress + TLS
cat > ingress.yaml << 'EOF'
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: web-app-ingress
  namespace: production
  annotations:
    nginx.ingress.kubernetes.io/ssl-redirect: "true"
spec:
  ingressClassName: nginx
  tls:
  - hosts:
    - app.example.com
    secretName: app-tls
  rules:
  - host: app.example.com
    http:
      paths:
      - path: /
        pathType: Prefix
        backend:
          service:
            name: web-app-svc
            port:
              number: 80
---
apiVersion: v1
kind: Service
metadata:
  name: web-app-svc
  namespace: production
spec:
  selector:
    app: web-app
  ports:
  - port: 80
    targetPort: 80
  type: ClusterIP
EOF

kubectl apply -f ingress.yaml
```
#### Step 4: 监控告警 (30min)

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `helm upgrade/install`：部署/升级 release
> - `kubectl apply/create/replace`：创建/变更集群资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 部署 Prometheus + Grafana（如果尚未部署）
helm install prometheus prometheus-community/kube-prometheus-stack \
  --namespace monitoring --create-namespace

# 创建告警规则
cat > alert-rules.yaml << 'EOF'
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: production-alerts
  namespace: monitoring
spec:
  groups:
  - name: app-alerts
    rules:
    - alert: AppDown
      expr: kube_deployment_status_replicas_available{namespace="production"} < 2
      for: 2m
      labels:
        severity: critical
      annotations:
        summary: "Production app has less than 2 available replicas"
EOF

kubectl apply -f alert-rules.yaml
```
#### Step 5: 验收检查 (30min)

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 验证应用运行
kubectl get pods -n production
kubectl get svc -n production
kubectl get ingress -n production

# 验证监控
kubectl get pods -n monitoring
kubectl port-forward -n monitoring svc/prometheus-grafana 3000:80

# 验证存储
kubectl get pvc -n production
kubectl get pv

# 验收清单
echo "=== 验收检查 ==="
echo "Deployment replicas: $(kubectl get deployment web-app -n production -o jsonpath='{.status.availableReplicas}')"
echo "PVC bound: $(kubectl get pvc data-pvc -n production -o jsonpath='{.status.phase}')"
echo "Ingress created: $(kubectl get ingress -n production --no-headers | wc -l | tr -d ' ')"
echo "Prometheus running: $(kubectl get pods -n monitoring -l app.kubernetes.io/name=prometheus --no-headers | grep Running | wc -l | tr -d ' ')"
```
---

## 常见问题

### Q1: 毕业项目做不完怎么办？

毕业项目可以在课后继续完成。重点完成以下核心部分：Deployment 部署 → Service 暴露 → 监控部署。存储、安全加固和 GitOps 可以作为后续练习。

### Q2: 一个月学完能达到什么水平？

完成一个月的学习后，你应该具备：独立搭建和管理 K8s 集群的能力、部署和运维常见应用的能力、基本的故障排查能力。但要达到资深 K8s 运维工程师的水平，还需要在实际生产环境中持续积累经验。

### Q3: 后续应该怎么继续深入学习？

推荐路径：1) 在实际项目中应用所学知识；2) 考取 CKA（Certified Kubernetes Administrator）或 CKS（Certified Kubernetes Security Specialist）认证；3) 深入某个领域（网络、存储、安全、可观测性）；4) 参与 K8s 社区和开源项目。

---

## 要点总结

| 能力域 | 达成标准 | 对应学习周 |
|--------|----------|-----------|
| 集群管理 | 能独立搭建和维护 K8s 集群 | Week 1-2 |
| 应用部署 | 能设计和实施生产级应用编排 | Week 2 |
| 网络存储 | 能配置复杂的网络和存储方案 | Week 2 |
| 安全合规 | 能实施安全加固和合规检查 | Week 3-4 |
| 监控告警 | 能构建完整的可观测性体系 | Week 3-4 |
| 故障排查 | 能系统化排查和解决问题 | Week 3 |
| GitOps | 能实施声明式持续部署 | Week 4 |

---

## 延伸阅读

- [毕业项目详细指南](../projects/p5-graduation-project.md)
- [终极自测题](./checkpoint.md)
- [生产架构设计原则](../../domain-11-production-operations/01-production-architecture-design-principles.md)
- [变更管理流程](../../domain-11-production-operations/22-change-management-process.md)
- [事故响应处理](../../domain-11-production-operations/23-incident-response-handling.md)
- [容量规划预测](../../domain-11-production-operations/24-capacity-planning-forecasting.md)

---

恭喜完成一个月的 Kubernetes 全栈运维学习！


<!-- risk-assessed -->
