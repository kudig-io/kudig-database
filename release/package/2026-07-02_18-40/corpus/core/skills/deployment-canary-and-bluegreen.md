---
title: 金丝雀与蓝绿发布
description: '## 概述'
summary: 'Kubernetes Deployment 原生支持多种发布策略。通过组合双 Deployment、[[Service|Service]] Selector 切换和 pause/resume 机制，可以实现金丝雀发布和蓝绿发布，满足不同场景的发布需求。'
category: skills
tags:
- k8s
- deployment
- canary
- blue-green
- progressive-delivery
- pause-resume
- traffic-split
- istio
- ingress
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 金丝雀与蓝绿发布 是什么
- 如何 金丝雀与蓝绿发布
trigger_keywords:
- 金丝雀与蓝绿发布
prerequisites:
- kubectl-basics
- service-mesh-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 金丝雀与蓝绿发布

## 概述

Kubernetes Deployment 原生支持多种发布策略。通过组合双 Deployment、[[Service|Service]] Selector 切换和 pause/resume 机制，可以实现金丝雀发布和蓝绿发布，满足不同场景的发布需求。

## 发布策略矩阵

| 策略 | 停机时间 | 流量控制粒度 | 实现复杂度 |
|------|---------|------------|----------|
| Recreate（原地替换） | 有 | 无 | 低 |
| RollingUpdate（滚动更新） | 零 | 副本数比例 | 低 |
| Canary（金丝雀） | 零 | 副本数/权重 | 中 |
| Blue-Green（蓝绿） | 秒级 | Service Selector | 中 |
| 进阶金丝雀（Argo/Flagger） | 零 | HTTP 权重/Header | 高 |

## 方案一：双 Deployment 金丝雀

### 原理

```
Service (selector: app=web)
    ├── Deployment-stable (replicas=9, labels: app=web, version=v1)
    └── Deployment-canary (replicas=1, labels: app=web, version=v2)
                              ↑
              10% 流量自动路由到金丝雀（基于副本比例）
```

### 操作

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源
> - `kubectl delete`：删除资源（可由声明式清单重建）

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 1. 当前稳定版本
kubectl get deployment web-stable

# 2. 创建金丝雀版本（10% 流量 = 1/10 副本）
kubectl apply -f web-canary.yaml

# 3. 观察金丝雀 Pod
kubectl get pods -l app=web -o wide

# 4. 确认无误后扩容金丝雀，缩容稳定版
kubectl scale deployment web-canary --replicas=5
kubectl scale deployment web-stable --replicas=5

# 5. 完全切换
kubectl scale deployment web-canary --replicas=10
kubectl delete deployment web-stable
```
### 配置

```yaml
# web-stable.yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: web-stable
  labels:
    app: web
    release: stable
spec:
  replicas: 9
  selector:
    matchLabels:
      app: web
      release: stable
  template:
    metadata:
      labels:
        app: web
        release: stable
    spec:
      containers:
      - name: web
        image: myapp:v1.0.0
---
# web-canary.yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: web-canary
  labels:
    app: web
    release: canary
spec:
  replicas: 1
  selector:
    matchLabels:
      app: web
      release: canary
  template:
    metadata:
      labels:
        app: web
        release: canary
    spec:
      containers:
      - name: web
        image: myapp:v2.0.0
---
# Service — 同时覆盖两个 Deployment
apiVersion: v1
kind: Service
metadata:
  name: web
spec:
  selector:
    app: web  # 同时匹配 stable 和 canary Pod
  ports:
  - port: 80
```

## 方案二：pause/resume 金丝雀

利用 `kubectl rollout pause` 将 RollingUpdate 暂停在中间状态：

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl rollout undo/restart`：触发滚动变更，影响副本

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 1. 触发更新，立即暂停
kubectl set image deployment/web web=myapp:v2.0.0
kubectl rollout pause deployment/web

# 此时：1 个新 Pod（金丝雀），4 个旧 Pod

# 2. 观察指标，确认无误后恢复
kubectl rollout resume deployment/web

# 如发现问题立即回滚
kubectl rollout undo deployment/web
```
pause 和 resume 的本质操作：

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
kubectl patch deployment web -p '{"spec":{"paused":true}}'
kubectl patch deployment web -p '{"spec":{"paused":false}}'
```
## 方案三：蓝绿发布（Service Selector 切换）

### 原理

```
Service (selector: 动态切换)
    ├── Deployment-blue  (当前生产，v1)
    └── Deployment-green (预发布，v2)
           ↑ 测试通过后，切换 Service selector 指向 green
```

### 操作

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源
> - `kubectl delete`：删除资源（可由声明式清单重建）
> - `kubectl edit/patch`：修改运行中的资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 1. 部署绿版本（不影响生产流量）
kubectl apply -f web-green.yaml

# 2. 验证绿版本（通过 port-forward）
kubectl port-forward svc/web-green 8080:80

# 3. 切换流量（原子操作）
kubectl patch svc web -p '{"spec":{"selector":{"version":"green"}}}'

# 4. 确认无误后清理蓝版本
kubectl delete deployment web-blue
```
## 方案四：[[Ingress|Ingress]] 权重金丝雀

```yaml
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: web-canary
  annotations:
    nginx.ingress.kubernetes.io/canary: "true"
    nginx.ingress.kubernetes.io/canary-weight: "20"  # 20% 流量
    # 或基于 Header 路由
    # nginx.ingress.kubernetes.io/canary-by-header: "X-Canary"
spec:
  rules:
  - host: api.example.com
    http:
      paths:
      - path: /
        pathType: Prefix
        backend:
          service:
            name: web-canary
            port:
              number: 80
```

## 方案对比

| 方案 | 适用场景 | 优点 | 缺点 |
|------|---------|------|------|
| 双 Deployment 金丝雀 | 流量比例控制 | 灵活、副本级别控制 | 需维护两个 Deployment |
| pause/resume 金丝雀 | 快速验证单个 Pod | 简单、原生支持 | 只能暂停 1 个新 RS |
| 蓝绿发布 | 需要原子切换 | 秒级切换，可快速回滚 | 资源消耗翻倍 |
| Ingress 权重 | 精确流量百分比 | 精确控制、支持 Header 路由 | 依赖 Ingress Controller |

## 常见错误

| 错误 | 现象 | 原因 | 解决方案 |
|------|------|------|---------|
| pause 后无法 resume | resume 命令不生效 | 同时存在 RollbackTo 注解 | 先回滚，再重新发布 |
| 蓝绿切换后旧 Pod 仍收到请求 | Service 缓存未刷新 | 等待 EndpointSlice 更新 | 通常 <1s |
| 金丝雀副本比例不准确 | 实际流量与预期不符 | kube-proxy 轮询非精确权重 | 使用 Ingress 权重或 Istio |
| 双 Deployment 标签冲突 | Pod 被错误覆盖 | selector 设计有重叠 | 使用唯一 release label 区分 |

## 相关技能

- [[skills/deployment-rolling-update.md|[[Deployment 滚动更新策略|Deployment 滚动更新策略]]]]
- [[skills/deployment-workload-selection.md|[[工作负载控制器选型|工作负载控制器选型]]]]
- [[deployment|Deployment]]

## Related

- [[skills/k8s-deployment-strategies-guide.md|k8s-deployment-strategies-guide]] — Kubernetes 部署策略最佳实践
- [[deployment]] — Deployment
- [[istio]] — Istio
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[argo]] — Argo Workflows

```

<!-- risk-assessed -->
