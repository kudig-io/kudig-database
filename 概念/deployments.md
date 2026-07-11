---
title: Deployments
summary: Deployments：Deployment 是 Kubernetes 中用于声明式管理 Pod 和 ReplicaSet 的控制器。
category: concepts
tags:
- core-concept
- k8s
- workloads
- visibility/public
tier: supporting
sources:
- KUDIG Stub Generation 2026-05-24
created: 2026-05-24
updated: 2026-07-11
last_updated: 2026-07
status: stable
---



# Deployments

## 概述

Deployment 是 Kubernetes 中用于声明式管理无状态应用的工作负载控制器。它通过管理 ReplicaSet 来维持指定数量的 Pod 副本，并在其之上提供滚动更新（RollingUpdate）和回滚（Rollback）能力。用户只需声明期望的 Pod 模板与副本数，Deployment 控制器会持续收敛实际状态，是生产环境运行无状态服务（Web API、微服务、前端）的首选对象。

## 架构与工作原理

```
Deployment (apps/v1)
   │ spec.replicas / spec.template
   ▼
ReplicaSet (rs-a)  ←─ 历史 ReplicaSet (rs-b, rs-c …)
   │ 维持副本数                     │ 用于回滚
   ▼
Pod × N
```

**工作流**：
1. 用户 apply Deployment 清单，控制器创建一个 ReplicaSet（以 Pod 模板的 hash 命名）。
2. ReplicaSet 通过 selector 匹配 Pod，缺多少补多少。
3. 当 Pod 模板变化（如镜像 tag 改变），Deployment 创建**新** ReplicaSet，按 `maxSurge`/`maxUnavailable` 策略逐步扩新、缩旧，实现滚动更新。
4. 旧 ReplicaSet 保留（replicas=0），保留最近 `revisionHistoryLimit`（默认 10）条历史，用于一键回滚。
5. 更新过程中始终满足：`已就绪副本数 ≥ desired - maxUnavailable`，从而保证服务可用性。

**更新策略（spec.strategy.type）**：
- `RollingUpdate`（默认）：渐进式替换，可配 `maxSurge` / `maxUnavailable`。
- `Recreate`：先杀全部旧 Pod 再起新 Pod，会有短暂停机，仅用于不支持多版本并存的场景。

## 关键组件与特性

| 字段 | 作用 |
|------|------|
| `spec.replicas` | 期望副本数 |
| `spec.selector` | 必须匹配 template.labels，且创建后不可改 |
| `spec.template` | Pod 模板，改动即触发 rollout |
| `spec.strategy` | RollingUpdate / Recreate |
| `spec.minReadySeconds` | Pod 就绪后多久才被视为可用，平滑发布 |
| `spec.progressDeadlineSeconds` | 超时判定发布失败（ProgressDeadlineExceeded） |
| `spec.revisionHistoryLimit` | 保留的旧 ReplicaSet 数量 |
| `spec.paused` | 暂停 rollout（多次改模板后统一发布） |

## 配置示例

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: webapp
  namespace: production
  labels:
    app: webapp
spec:
  replicas: 4
  revisionHistoryLimit: 10
  progressDeadlineSeconds: 600
  minReadySeconds: 10
  strategy:
    type: RollingUpdate
    rollingUpdate:
      maxSurge: 1           # 滚动期间可超出期望 1 个
      maxUnavailable: 0     # 滚动期间不允许不可用（零停机）
  selector:
    matchLabels:
      app: webapp
  template:
    metadata:
      labels:
        app: webapp
    spec:
      containers:
      - name: webapp
        image: registry.example.com/webapp:v2.0.0
        ports:
        - containerPort: 8080
        resources:
          requests:
            cpu: 250m
            memory: 256Mi
          limits:
            cpu: "1"
            memory: 512Mi
        readinessProbe:
          httpGet:
            path: /ready
            port: 8080
          periodSeconds: 5
```

## 常用操作与命令

```bash
# 创建 / 伸缩
kubectl create deployment webapp --image=webapp:v1 --replicas=3
kubectl scale deployment webapp --replicas=6

# 滚动更新与状态
kubectl set image deployment/webapp webapp=webapp:v2
kubectl rollout status deployment/webapp
kubectl rollout pause deployment/webapp     # 暂停
kubectl rollout resume deployment/webapp    # 恢复

# 回滚
kubectl rollout undo deployment/webapp
kubectl rollout undo deployment/webapp --to-revision=2
kubectl rollout history deployment/webapp

# 重启（触发重新拉取镜像 / 重置状态）
kubectl rollout restart deployment/webapp
```

## 最佳实践

1. **零停机发布三件套**：`maxUnavailable: 0` + readinessProbe + `terminationGracePeriodSeconds` 充足。
2. **minReadySeconds 设小值（如 10-30s）**：避免 Pod 刚就绪就被加入流量但实际不稳定。
3. **progressDeadlineSeconds**：让卡住的发布自动标记 Failed，便于告警与自动回滚（配合 Argo Rollouts / Flagger）。
4. **不要直接用 ReplicaSet**：Deployment 提供版本管理与回滚，手动管理 RS 会丢失这些能力。
5. **镜像 tag 可追溯**：用 git commit 或语义化版本，禁用 `:latest`。
6. **HPA 协同**：让 HPA 管理 replicas，Deployment 模板里不硬写副本数（或设初始值）。

## 常见陷阱

- **发布卡住不收敛**：通常是 readinessProbe 失败或资源不足新 Pod 一直 Pending，检查 `kubectl rollout status` 与 events。
- **selector 不可变**：尝试修改 selector 会直接报错，需重建 Deployment。
- **rollout history 缺失**：revisionHistoryLimit 设为 0 则无法回滚；手工 `kubectl apply` 改非模板字段不产生新 revision。
- **Recreate 策略导致停机**：单实例或不支持并行的应用用 Recreate 必有 downtime，建议改 RollingUpdate 并配 PDB。
- **HPA 与手动 scale 冲突**：HPA 启用后不要手动 `kubectl scale`，否则下一次伸缩会被 HPA 覆盖。

## 相关概念

- [[概念/kubernetes.md|Kubernetes]]
- [[概念/pods.md|Pod]]
- [[概念/replicaset.md|ReplicaSet]] — Deployment 的底层
- [[概念/statefulset.md|StatefulSet]] — 有状态对照
- [[概念/blue-green-deployment.md|蓝绿发布]]
- [[概念/canary-deployment.md|金丝雀发布]]
- [[概念/kubernetes-architecture-overview.md|Kubernetes 架构概览]]
- [[README]]

## Related

- [[visibility-public|#visibility/public Hub]] — tag hub
