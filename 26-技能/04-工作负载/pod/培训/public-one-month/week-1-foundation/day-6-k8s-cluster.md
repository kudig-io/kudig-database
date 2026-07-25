---
title: 'Day 6: K8s 架构深化 + 集群配置'
description: '# Day 6: K8s 架构深化 + 集群配置'
summary: '本文在 Day 5 的架构基础上，深入集群配置参数、API 版本管理，并通过部署第一个 Deployment 来体验 K8s 声明式管理的完整工作流。你将理解 Deployment → [[ReplicaSet|ReplicaSet]] → Pod 的层级关系，掌握滚动更新和回滚操作，并学会创建 Service 暴露应用。'
category: learning
tags:
- k8s
- training
- hands-on
- etcd
- apiserver
- kubelet
- statefulset
- job
- cronjob
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
- 'Day 6: K8s 架构深化 + 集群配置 是什么'
- '如何 Day 6: K8s 架构深化 + 集群配置'
trigger_keywords:
- Day
- '6:'
- K8s
- 架构深化
- 集群配置
- learn
prerequisites:
- kubectl-basics
- gpu-ml-basics
- etcd-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Day 6: K8s 架构深化 + 集群配置

```yaml
---
id: LEARN-ONE-MONTH-W1-DAY6
title: Day 6 - K8s 架构深化 + 集群配置
topic: kubernetes
type: hands-on-guide
tags: [kubernetes, deployment, service, rolling-update, rollback, api-version, hands-on, week-1]
last_updated: 2026-05-18
difficulty: beginner
intent_queries:
  - "Deployment 完整配置怎么写"
  - "滚动更新怎么配置"
  - "回滚怎么做"
  - "Service 怎么暴露应用"
trigger_keywords:
  - Deployment
  - RollingUpdate
  - maxSurge
  - maxUnavailable
  - Rollback
  - RevisionHistoryLimit
  - Service
  - ClusterIP
  - NodePort
  - Endpoints
  - kubectl apply
  - 声明式管理
reading_level: beginner
audience:
  - sre
  - ops-engineer
  - developer
estimated_read_time: 45min
related_domains:
  - 集群基础
  - 工作负载
related_topics:
  - kubernetes
  - deployment
  - service
  - rollout
related:
  - 生产运维/topic-learn/public-training/one-month/week-1-foundation/day-5-k8s-architecture.md
  - 工作负载/02-deployment-production-patterns.md
---
```

> **学习时间**: 4-5 小时 | **主题**: 深入理解集群配置与声明式管理

---

## 概述

本文在 Day 5 的架构基础上，深入集群配置参数、API 版本管理，并通过部署第一个 Deployment 来体验 K8s 声明式管理的完整工作流。你将理解 Deployment → [[ReplicaSet|ReplicaSet]] → Pod 的层级关系，掌握滚动更新和回滚操作，并学会创建 Service 暴露应用。

### 学习目标

- 理解 K8s 集群配置参数（[[kubelet|kubelet]]、API Server、网络等关键配置）
- 掌握 API 版本演进（alpha/beta/stable）和特性门控（Feature Gate）
- 部署第一个 Deployment，体验声明式管理的完整流程
- 理解 Deployment → ReplicaSet → Pod 的层级关系和滚动更新机制

---

## 核心概念详解

### 集群配置参数

K8s 集群的配置涉及多个组件的参数。在 kind/minikube 本地集群中，这些参数已经预设好了，但了解它们对于理解集群行为和排障很重要。

**kube-apiserver 关键参数**:

| 参数 | 说明 | 典型值 |
|------|------|--------|
| `--etcd-servers` | [[etcd|etcd]] 连接地址 | `http://127.0.0.1:2379` |
| `--service-cluster-ip-range` | Service CIDR | `10.96.0.0/12` |
| `--enable-admission-plugins` | 启用的准入控制器 | `NodeRestriction,LimitRanger` |
| `--max-requests-inflight` | 最大并发请求数 | `400` |
| `--request-timeout` | 请求超时 | `60s` |

**kubelet 关键参数**:

| 参数 | 说明 | 典型值 |
|------|------|--------|
| `--max-[[Pods|pods]]` | 节点最大 Pod 数 | `110` |
| `--pod-cidr` | Pod IP 地址范围 | `10.244.0.0/24` |
| `--eviction-hard` | 硬驱逐阈值 | `memory.available<100Mi` |
| `--system-reserved` | 系统资源预留 | `cpu=500m,memory=512Mi` |

### API 版本与演进

K8s 的 API 资源有不同的版本阶段：

- **alpha**（如 `v1alpha1`）: 实验性功能，可能随时变更或移除，默认禁用
- **beta**（如 `v1beta1`）: 经过测试的功能，可能有小幅变更，默认启用
- **stable**（如 `v1`）: 稳定版本，保证向后兼容

常用资源的 API 版本：

| 资源 | API 版本 | API Group |
|------|---------|-----------|
| Pod, Service, ConfigMap | v1 | "" (核心组) |
| Deployment, StatefulSet | v1 | apps |
| Ingress | v1 | networking.k8s.io |
| CronJob | v1 | batch |
| HorizontalPodAutoscaler | v2 | autoscaling |

### Deployment 工作原理

Deployment 是 K8s 中管理无状态应用的核心资源。它通过 ReplicaSet 间接管理 Pod，提供滚动更新和回滚能力。

**Deployment → ReplicaSet → Pod 关系**:
- Deployment 定义应用的期望状态（镜像、副本数、更新策略等）
- Deployment 创建和管理 ReplicaSet（每个版本对应一个 ReplicaSet）
- ReplicaSet 负责维护指定数量的 Pod 副本

**滚动更新策略**:

| 参数 | 说明 | 推荐值 |
|------|------|--------|
| `maxSurge` | 更新时最多超出期望副本数 | `1` 或 `25%` |
| `maxUnavailable` | 更新时最多允许多少不可用 | `0` 或 `25%` |

maxSurge=1, maxUnavailable=0 意味着：先创建 1 个新 Pod，等新 Pod 就绪后再删除旧 Pod，始终保持所有副本可用。

---

## 实战演练

### 任务 1: 部署第一个 Deployment (45min)

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 创建测试 namespace
kubectl create namespace learn-k8s
# 预期输出: namespace/learn-k8s created

# 方式 1: 命令行创建
kubectl create deployment nginx --image=nginx:alpine -n learn-k8s
# 预期输出: deployment.apps/nginx created

# 查看创建的资源（注意层级关系）
kubectl get deployment -n learn-k8s
# NAME    READY   UP-TO-DATE   AVAILABLE   AGE
# nginx   1/1     1            1           30s

kubectl get replicaset -n learn-k8s
# NAME               DESIRED   CURRENT   READY   AGE
# nginx-6d4f7b8c9d   1         1         1       30s

kubectl get pods -n learn-k8s
# NAME                     READY   STATUS    RESTARTS   AGE
# nginx-6d4f7b8c9d-abc12   1/1     Running   0          30s

# 方式 2: YAML 文件创建（推荐）
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
  strategy:
    type: RollingUpdate
    rollingUpdate:
      maxSurge: 1
      maxUnavailable: 0
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
EOF

kubectl apply -f nginx-deployment.yaml
# 预期输出: deployment.apps/nginx-deployment created

# 查看 Deployment 详情
kubectl describe deployment nginx-deployment -n learn-k8s
# 重点关注:
# Replicas:           3 desired | 3 updated | 3 total | 3 available | 0 unavailable
# StrategyType:       RollingUpdate
# RollingUpdateStrategy:  max surge 1, max unavailable 0
# Pod Template:        Image: nginx:alpine
```
### 任务 2: 体验声明式管理 (45min)

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源
> - `kubectl rollout undo/restart`：触发滚动变更，影响副本

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 修改副本数（命令式）
kubectl scale deployment nginx-deployment --replicas=5 -n learn-k8s
# 预期输出: deployment.apps/nginx-deployment scaled

# 观察 Pod 变化
kubectl get pods -n learn-k8s -w
# 预期输出: 新 Pod 被创建
# NAME                                 READY   STATUS    RESTARTS   AGE
# nginx-deployment-6d4f7b8c9d-abc12    1/1     Running   0          5m
# nginx-deployment-6d4f7b8c9d-def34    1/1     Running   0          5m
# nginx-deployment-6d4f7b8c9d-ghi56    1/1     Running   0          5m
# nginx-deployment-6d4f7b8c9d-jkl78    0/1     Pending   0          1s
# nginx-deployment-6d4f7b8c9d-mno90    0/1     Pending   0          1s

# 修改 YAML 后重新 apply（声明式，推荐）
# 将 replicas 改为 2
sed -i '' 's/replicas: 3/replicas: 2/' nginx-deployment.yaml
kubectl apply -f nginx-deployment.yaml
# 预期输出: deployment.apps/nginx-deployment configured

# 查看 ReplicaSet 历史（每次更新会创建新的 ReplicaSet）
kubectl get replicaset -n learn-k8s
# NAME                           DESIRED   CURRENT   READY   AGE
# nginx-deployment-6d4f7b8c9d    2         2         2       10m

# 滚动更新（修改镜像版本）
kubectl set image deployment/nginx-deployment nginx=nginx:1.25 -n learn-k8s
# 预期输出: deployment.apps/nginx-deployment image updated

# 观察滚动更新过程
kubectl rollout status deployment/nginx-deployment -n learn-k8s
# 预期输出:
# Waiting for deployment "nginx-deployment" rollout to finish: 1 out of 2 new replicas have been updated...
# Waiting for deployment "nginx-deployment" rollout to finish: 1 old replicas are pending termination...
# deployment "nginx-deployment" successfully rolled out

# 查看更新历史
kubectl rollout history deployment/nginx-deployment -n learn-k8s
# 预期输出:
# REVISION  CHANGE-CAUSE
# 1         kubectl apply --filename=nginx-deployment.yaml --record=true
# 2         kubectl set image deployment/nginx-deployment nginx=nginx:1.25

# 查看两个 ReplicaSet（新旧版本）
kubectl get replicaset -n learn-k8s
# NAME                           DESIRED   CURRENT   READY   AGE
# nginx-deployment-6d4f7b8c9d    0         0         0       15m    (旧版本，副本数为 0)
# nginx-deployment-7e8a9c0d1e    2         2         2       2m     (新版本)

# 回滚到上一版本
kubectl rollout undo deployment/nginx-deployment -n learn-k8s
# 预期输出: deployment.apps/nginx-deployment rolled back

# 验证回滚
kubectl get replicaset -n learn-k8s
# 旧 ReplicaSet 重新变为 DESIRED=2
```
### 任务 3: 创建 Service 暴露应用 (30min)

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
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
# 预期输出: service/nginx-service created

# 查看 Service
kubectl get svc -n learn-k8s
# NAME             TYPE        CLUSTER-IP      EXTERNAL-IP   PORT(S)   AGE
# nginx-service    ClusterIP   10.96.123.456   <none>        80/TCP    10s

kubectl describe svc nginx-service -n learn-k8s
# Name:              nginx-service
# Selector:          app=nginx
# Type:              ClusterIP
# Endpoints:         10.244.1.2:80,10.244.2.2:80

# 查看 Endpoints（Service 关联的后端 Pod）
kubectl get endpoints nginx-service -n learn-k8s
# NAME             ENDPOINTS                      AGE
# nginx-service    10.244.1.2:80,10.244.2.2:80    1m

# 测试访问（在集群内）
kubectl run curl --image=curlimages/curl -it --rm --restart=Never -n learn-k8s -- \
  curl -s http://nginx-service.learn-k8s.svc.cluster.local
# 预期输出: nginx 默认欢迎页面 HTML
```
### 任务 4: 查看集群事件 (30min)

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl rollout undo/restart`：触发滚动变更，影响副本

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 查看 namespace 事件
kubectl get events -n learn-k8s --sort-by='.lastTimestamp'
# 预期输出:
# LAST SEEN   TYPE    REASON              OBJECT                                MESSAGE
# 2m          Normal  ScalingReplicaSet   deployment/nginx-deployment           Scaled up replica set to 3
# 1m          Normal  SuccessfulCreate    replicaset/nginx-deployment-xxx        Created pod: nginx-deployment-xxx-abc
# 30s         Normal  Pulled              pod/nginx-deployment-xxx-abc           Container image "nginx:alpine" already present
# 30s         Normal  Created             pod/nginx-deployment-xxx-abc           Created container nginx
# 30s         Normal  Started             pod/nginx-deployment-xxx-abc           Started container nginx

# 查看特定资源的事件
kubectl describe pod <pod-name> -n learn-k8s | grep -A 20 Events

# 实时监控事件
kubectl get events -n learn-k8s -w

# 模拟问题场景
kubectl set image deployment/nginx-deployment nginx=nginx:nonexistent -n learn-k8s
# 预期输出: deployment.apps/nginx-deployment image updated

# 观察事件
kubectl get events -n learn-k8s | grep -i error
# 预期输出: Failed to pull image "nginx:nonexistent": ...

# 查看 Pod 状态
kubectl get pods -n learn-k8s
# 新 Pod 处于 ImagePullBackOff 状态

# 回滚修复
kubectl rollout undo deployment/nginx-deployment -n learn-k8s
```
### 任务 5: API 资源探索 (30min)

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 查看所有 API 资源
kubectl api-resources | head -30

# 查看资源的 API 版本
kubectl api-resources | grep deployment
# DEPLOYMENTS   deploy   apps/v1         true         Deployment

# 使用 kubectl explain 查看资源结构
kubectl explain deployment
kubectl explain deployment.spec
kubectl explain deployment.spec.strategy.rollingUpdate

# 获取资源的完整 YAML
kubectl get deployment nginx-deployment -n learn-k8s -o yaml

# 使用 --dry-run 预览
kubectl create deployment test --image=nginx --dry-run=client -o yaml | head -30
```
---

## 配置示例

### 完整的 Deployment 模板

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: <name>
  namespace: <namespace>
  labels:
    app: <label>
spec:
  replicas: 3
  selector:
    matchLabels:
      app: <label>
  strategy:
    type: RollingUpdate
    rollingUpdate:
      maxSurge: 1
      maxUnavailable: 0
  minReadySeconds: 5
  revisionHistoryLimit: 10
  progressDeadlineSeconds: 300
  template:
    metadata:
      labels:
        app: <label>
    spec:
      containers:
      - name: <container-name>
        image: <image>:<tag>
        ports:
        - containerPort: <port>
        resources:
          requests:
            cpu: 100m
            memory: 128Mi
          limits:
            cpu: 200m
            memory: 256Mi
        livenessProbe:
          httpGet:
            path: /healthz
            port: <port>
          initialDelaySeconds: 10
          periodSeconds: 10
        readinessProbe:
          httpGet:
            path: /ready
            port: <port>
          initialDelaySeconds: 5
          periodSeconds: 5
        lifecycle:
          preStop:
            exec:
              command: ["/bin/sh", "-c", "sleep 5"]
      terminationGracePeriodSeconds: 30

```

---

## 常见问题

### Q1: Deployment、ReplicaSet、Pod 三者的关系是什么？

Deployment 管理 ReplicaSet，ReplicaSet 管理 Pod。每次 Deployment 更新（修改镜像等），都会创建一个新的 ReplicaSet，旧 ReplicaSet 的副本数逐渐减为 0。回滚就是将旧 ReplicaSet 的副本数恢复。`revisionHistoryLimit` 控制保留多少个旧 ReplicaSet（默认 10）。

### Q2: 什么是"声明式管理"？

声明式管理是"描述期望状态"而非"执行操作步骤"。你告诉 K8s "我要 3 个 nginx Pod"，而不是"先创建 Pod1，再创建 Pod2，再创建 Pod3"。Controller Manager 会自动将实际状态向期望状态收敛。这意味着你可以反复 `kubectl apply` 同一个文件，K8s 只会执行必要的变更。

### Q3: Service 是如何找到 Pod 的？

Service 通过 Label Selector 匹配 Pod 的 labels。匹配到的 Pod IP 和端口会自动记录在 Endpoints 对象中。kube-proxy 在每个节点上配置 iptables/IPVS 规则，将 Service ClusterIP 的流量转发到 Endpoints 中的 Pod IP。

### Q4: 滚动更新卡住怎么办？

检查 `kubectl rollout status` 的输出。常见原因：新 Pod 的 readinessProbe 失败（新 Pod 不就绪，不会替换旧 Pod）、镜像拉取失败、资源不足导致新 Pod Pending。`progressDeadlineSeconds`（默认 600 秒）可以自动检测更新卡住的情况。

### Q5: kubectl apply 和 kubectl create 有什么区别？

`kubectl create` 是命令式操作——如果资源已存在会报错。`kubectl apply` 是声明式操作——如果资源已存在会计算差异并更新，如果不存在则创建。推荐始终使用 `kubectl apply`。

### Q6: 如何查看 API Server 的配置参数？

在 kind 集群中：`kubectl describe pod -n kube-system -l component=kube-apiserver | grep -A 50 "Command:"`。在 ACK 专有版中：SSH 到 Master 节点查看 `/etc/kubernetes/manifests/kube-apiserver.yaml`。

---

## 要点总结

| 概念 | 说明 | 关键命令 |
|------|------|---------|
| Deployment | 管理无状态应用 | `kubectl apply -f deployment.yaml` |
| ReplicaSet | 维护 Pod 副本数 | 每次 Deployment 更新创建新 RS |
| 滚动更新 | 零停机更新 | `kubectl set image` + `rollout status` |
| 回滚 | 恢复到上一版本 | `kubectl rollout undo` |
| Service | 稳定的访问入口 | ClusterIP + Label Selector + Endpoints |
| 声明式管理 | 描述期望状态 | `kubectl apply`（推荐） |

---

## 延伸阅读

- [集群配置参数](../../集群基础/06-cluster-configuration-parameters.md)
- [API 版本与特性](../../集群基础/03-api-versions-features.md)
- [Deployment 生产模式](../../工作负载/02-deployment-production-patterns.md)
- [K8s 速查手册](../../系统基础/速查卡/k8s.md)

## Related

- [[21-生态参考/03-领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]

```

<!-- risk-assessed -->
