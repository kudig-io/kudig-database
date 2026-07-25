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

## 源码实现分析

### Deployment Controller 滚动更新核心逻辑

```go
// k8s.io/kubernetes/pkg/controller/deployment/sync.go
func (dc *DeploymentController) syncDeployment(ctx context.Context, d *apps.Deployment) error {
    // 1. 获取所有关联的 ReplicaSet
    rsList := dc.getAllReplicaSets(d)
    // 2. 计算新 RS 的期望副本数
    newRS := dc.getNewReplicaSet(d, rsList)
    if d.Spec.Strategy.Type == apps.RollingUpdateDeploymentStrategyType {
        // 3. 滚动更新逻辑
        maxSurge := d.Spec.Strategy.RollingUpdate.MaxSurge       // 默认 25%
        maxUnavailable := d.Spec.Strategy.RollingUpdate.MaxUnavailable // 默认 25%
        // 4. 扩容新 RS（不超过 replicas + maxSurge）
        newReplicas := calculateNewReplicas(d, newRS, maxSurge)
        dc.scaleReplicaSet(newRS, newReplicas)
        // 5. 缩容旧 RS（不低于 replicas - maxUnavailable）
        for _, oldRS := range oldRSList {
            oldReplicas := calculateOldReplicas(d, oldRS, maxUnavailable)
            dc.scaleReplicaSet(oldRS, oldReplicas)
        }
    }
    // 6. 检查是否完成（新 RS 就绪 + 旧 RS 缩容到 0）
    if dc.deploymentComplete(d, newRS) {
        // 清理旧 RS（保留 revisionHistoryLimit 个）
        dc.cleanupOldReplicaSets(rsList, d.Spec.RevisionHistoryLimit)
    }
    return nil
}
```

### Deployment 滚动更新状态机

```
┌──────────────────────────────────────────────────────────┐
│          Deployment 滚动更新状态机                    │
├──────────────────────────────────────────────────────────┤
│  kubectl set image deployment/webapp app=v2              │
│         │                                                │
│         ▼                                                │
│  ┌─────────────┐     ┌─────────────┐              │
│  │ 创建新 RS    │────▶│ 扩容新 RS    │              │
│  │ (replicas=0) │     │ (+maxSurge)  │              │
│  └─────────────┘     └──────┬──────┘              │
│                              │ 新 Pod Ready?            │
│                              ▼                          │
│                       ┌─────────────┐              │
│                       │ 缩容旧 RS    │              │
│                       │ (-maxUnavail)│              │
│                       └──────┬──────┘              │
│                              │ 旧 Pod 全部终止?        │
│                              ▼                          │
│                       ┌─────────────┐              │
│                       │ 完成/清理    │              │
│                       │ 旧 RS 保留   │              │
│                       └─────────────┘              │
│  异常: progressDeadlineSeconds 超时 → Failed          │
└──────────────────────────────────────────────────────────┘
```

## 使用场景

### 场景一：零停机滚动更新配置

```yaml
# 🟡 中风险：修改 Deployment 策略影响发布行为
apiVersion: apps/v1
kind: Deployment
metadata:
  name: webapp
spec:
  replicas: 6
  revisionHistoryLimit: 5  # 保留 5 个版本用于回滚
  progressDeadlineSeconds: 600  # 10分钟超时标记 Failed
  strategy:
    type: RollingUpdate
    rollingUpdate:
      maxSurge: 25%        # 最多多创建 25% Pod
      maxUnavailable: 0    # 零不可用（严格零停机）
  template:
    spec:
      terminationGracePeriodSeconds: 60  # 优雅终止时间
      containers:
      - name: webapp
        image: registry/webapp:v2.0.0
        readinessProbe:  # 必须配置！否则 Pod 创建即 Ready
          httpGet:
            path: /ready
            port: 8080
          initialDelaySeconds: 5
          periodSeconds: 5
        lifecycle:
          preStop:  # 优雅关闭：等待连接排干
            exec:
              command: ["sh", "-c", "sleep 10"]
```

### 场景二：发布监控与自动回滚

```bash
# 🟡 中风险：生产发布操作
# 触发发布
kubectl set image deployment/webapp webapp=registry/webapp:v2.1.0 -n production
# 监控发布进度
kubectl rollout status deployment/webapp -n production --timeout=300s
# 发布期间观察关键指标
kubectl get pods -n production -w  # 观察 Pod 状态变化
# 异常时立即回滚
kubectl rollout undo deployment/webapp -n production  # 🟡 回滚到上一版本
kubectl rollout undo deployment/webapp --to-revision=3 -n production  # 回滚到指定版本
# 确认回滚成功
kubectl rollout status deployment/webapp -n production
kubectl get rs -n production  # 确认 RS 副本数正确
```

### 场景三：金丝雀发布（暂停/恢复）

```bash
# 🟡 中风险：金丝雀发布
# 更新镜像并立即暂停
kubectl set image deployment/webapp webapp=registry/webapp:v3.0.0 -n production
kubectl rollout pause deployment/webapp -n production
# 此时只有 maxSurge 数量的新 Pod 运行（金丝雀）
kubectl get rs -n production  # 观察新旧 RS 副本数
# 观察 15 分钟关键指标：错误率、延迟、资源使用
# 确认无异常后继续发布
kubectl rollout resume deployment/webapp -n production
# 或者发现问题回滚
kubectl rollout undo deployment/webapp -n production
```

## 常见误区

| # | 误区 | 正确理解 |
|---|------|----------|
| 1 | 不配 readinessProbe 也能零停机 | 无 readinessProbe 时 Pod 创建即 Ready，流量立即打入（应用可能未初始化完成） |
| 2 | maxUnavailable=0 就绝对零停机 | 还需 readinessProbe + preStop + 足够 terminationGracePeriod 配合 |
| 3 | 回滚总是安全的 | 若新版本有数据库 migration，回滚代码可能导致数据不兼容 |
| 4 | revisionHistoryLimit=0 节省资源 | 设为 0 则无法回滚！生产至少保留 3-5 个版本 |
| 5 | Recreate 策略无停机 | Recreate 先杀旧 Pod 再建新 Pod，必有 downtime；只适合单实例/开发环境 |
| 6 | HPA 和手动 scale 可以共存 | HPA 启用后手动 scale 会被立即覆盖；应通过 HPA 参数控制 |

## 面试要点

1. **Q: Deployment 滚动更新的完整流程是什么？**
   A: ① 用户更新 Pod Template（如镜像版本）；② Deployment Controller 创建新 ReplicaSet（replicas=0）；③ 按 maxSurge 扩容新 RS（如 25% → 最多 7-8 个 Pod）；④ 新 Pod 通过 readinessProbe 后加入 Service Endpoints；⑤ 按 maxUnavailable 缩容旧 RS（如 25% → 最少 4-5 个可用）；⑥ 重复③⑤直到新 RS 达到期望副本、旧 RS 缩容到 0；⑦ 清理超出 revisionHistoryLimit 的旧 RS。

2. **Q: 如何实现真正的零停机发布？**
   A: 五个必要条件：① maxUnavailable: 0（始终有足够副本）；② readinessProbe（确保新 Pod 真正就绪才接收流量）；③ preStop hook + sleep（等待 kube-proxy 更新 iptables，避免连接打到已终止 Pod）；④ 足够的 terminationGracePeriodSeconds（等待现有请求处理完成）；⑤ 应用支持优雅关闭（处理 SIGTERM，完成进行中的请求）。

3. **Q: Deployment 发布卡住（不收敛）如何排查？**
   A: ① kubectl rollout status 查看当前状态；② kubectl get rs 检查新旧 RS 副本数；③ kubectl get pods 查看新 Pod 状态（Pending/ImagePullBackOff/CrashLoopBackOff）；④ kubectl describe pod 查看事件（调度失败/资源不足/探针失败）；⑤ 检查 progressDeadlineSeconds 是否超时标记 Failed；⑥ 常见原因：资源不足、镜像拉取失败、readinessProbe 配置错误、PDB 阻止缩容。

4. **Q: Deployment 与 StatefulSet/DaemonSet 如何选择？**
   A: Deployment：无状态应用（Web 服务、API），Pod 可互换、随机调度、滚动更新。StatefulSet：有状态应用（数据库、消息队列），需要稳定网络标识、持久存储、有序部署/扩缩。DaemonSet：每节点一个（日志采集、监控 agent、网络插件），节点加入自动部署。关键区别：Deployment 的 Pod 是无状态可替换的；StatefulSet 的 Pod 有身份和状态。

## 相关概念

- [[22-概念/01-核心架构/kubernetes.md|Kubernetes]]
- [[22-概念/02-工作负载/pods.md|Pod]]
- [[22-概念/02-工作负载/replicaset.md|ReplicaSet]] — Deployment 的底层
- [[22-概念/02-工作负载/statefulset.md|StatefulSet]] — 有状态对照
- [[22-概念/09-平台与发布/blue-green-deployment.md|蓝绿发布]]
- [[22-概念/09-平台与发布/canary-deployment.md|金丝雀发布]]
- [[22-概念/01-核心架构/kubernetes-architecture-overview.md|Kubernetes 架构概览]]
- [[README]]

## Related

- [[visibility-public|#visibility/public Hub]] — tag hub
