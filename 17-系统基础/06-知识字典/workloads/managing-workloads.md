---
title: Managing Workloads
description: '## 概述'
summary: '## 概述'
category: dictionary
tags:
- k8s
- glossary
- terminology
- hpa
- vpa
- pdb
- statefulset
- daemonset
- rag
tier: supporting
created: '2026-05-23'
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Managing Workloads 是什么
- 如何 Managing Workloads
trigger_keywords:
- Managing
- Workloads
- dictionary
prerequisites:
- kubectl-basics
- pod-lifecycle
- cloud-provider-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Managing Workloads

## 概述
本页介绍在 [[kubernetes|Kubernetes]] 中部署应用后，如何使用各种工具和实践来管理、更新和扩展工作负载，涵盖 kubectl 批量操作、应用更新、金丝雀发布、资源注解和扩缩容等内容。

## 核心概念/原理
- **资源配置组织**：将同一微服务的相关资源（如 Deployment + [[service|Service]]）放在同一个 YAML 文件中，用 `---` 分隔，便于统一管理。
- **kubectl 批量操作**：
  - `kubectl apply -f <dir> --recursive`：递归处理目录下的所有清单文件。
  - `kubectl delete -f <file>` 或 `kubectl delete <resource>/<name>`：删除资源。
  - 通过标签选择器 `-l` 进行批量过滤和操作。
  - 利用 `xargs` 或命令替换 `$()` 链式操作资源。
- **应用更新**：
  - 使用 Deployment、[[daemonset|DaemonSet]]、[[statefulset|StatefulSet]] 的滚动更新机制，逐步将流量切换到新版本的 Pod。
  - `kubectl rollout` 系列命令用于管理、暂停、恢复和查看更新进度。
  - `kubectl patch`、`kubectl edit`、`kubectl apply` 用于对资源进行原地更新。
  - 对于不可变字段的修改，可使用 `kubectl replace --force`（先删除再重建）。

## 关键机制或特性
- **金丝雀部署（Canary Deployment）**：通过为不同版本设置不同标签（如 `track: stable` 和 `track: canary`），让 Service 同时覆盖两组 Pod，逐步将流量导向新版本。
- **自动扩缩容**：
  - `kubectl scale`：手动调整副本数。
  - `kubectl autoscale`：创建 HorizontalPodAutoscaler，根据 CPU 利用率等指标自动扩缩容。
- **原地更新（In-place Updates）**：
  - `kubectl apply`：基于声明式配置进行差异更新，推荐与版本控制配合使用。
  - `kubectl edit`：交互式编辑资源。
  - `kubectl patch`：支持 JSON patch、JSON merge patch 和 strategic merge patch。

## 使用场景
- 日常应用的生命周期管理、版本发布和回滚。
- 需要零停机更新的生产环境。
- 根据负载动态调整应用容量的自动扩缩容场景。
- 通过金丝雀发布验证新版本稳定性。

## 最佳实践/注意事项
- 将同一应用的相关资源放在同一个目录或文件中，并使用版本控制管理清单。
- 使用 `kubectl apply` 而非直接 `replace`，以保留自动化字段（如 `resourceVersion`）。
- 滚动更新时，设置合理的 `maxSurge` 和 `maxUnavailable`，平衡可用性和更新速度。
- 使用 HPA 时，建议从 Deployment/StatefulSet 的 manifest 中移除 `spec.replicas`，避免 `kubectl apply` 与 HPA 冲突。
- 对于破坏性更新（需修改不可变字段），使用 `replace --force` 并确认业务影响。

## 生产 YAML 示例

### 同一文件管理多个关联资源

```yaml
# 推荐：将同一微服务的 Deployment + Service + HPA 放在同一文件
apiVersion: apps/v1
kind: Deployment
metadata:
  name: api-server
  namespace: production
  labels:
    app: api-server
    version: v3.2.1
spec:
  # 注意：使用 HPA 时，不要设置 replicas 字段
  selector:
    matchLabels:
      app: api-server
  strategy:
    type: RollingUpdate
    rollingUpdate:
      maxSurge: 1              # 每次多创建 1 个 Pod
      maxUnavailable: 0        # 零停机更新
  template:
    metadata:
      labels:
        app: api-server
        version: v3.2.1
    spec:
      containers:
      - name: api
        image: registry.example.com/apps/api-server:v3.2.1
        ports:
        - containerPort: 8080
        readinessProbe:
          httpGet:
            path: /healthz
            port: 8080
          initialDelaySeconds: 5
          periodSeconds: 5
        resources:
          requests:
            cpu: "500m"
            memory: "512Mi"
          limits:
            cpu: "1"
            memory: "1Gi"
---
apiVersion: v1
kind: Service
metadata:
  name: api-server
  namespace: production
spec:
  selector:
    app: api-server
  ports:
  - port: 80
    targetPort: 8080
---
apiVersion: autoscaling/v2
kind: HorizontalPodAutoscaler
metadata:
  name: api-server
  namespace: production
spec:
  scaleTargetRef:
    apiVersion: apps/v1
    kind: Deployment
    name: api-server
  minReplicas: 3
  maxReplicas: 20
  metrics:
  - type: Resource
    resource:
      name: cpu
      target:
        type: Utilization
        averageUtilization: 70
```

### 金丝雀部署配置

```yaml
# Stable 版本
apiVersion: apps/v1
kind: Deployment
metadata:
  name: web-stable
  namespace: production
spec:
  replicas: 9                  # 90% 流量
  selector:
    matchLabels:
      app: web
      track: stable
  template:
    metadata:
      labels:
        app: web               # Service 通过此标签选择
        track: stable
        version: v2.0.0
    spec:
      containers:
      - name: web
        image: registry.example.com/apps/web:v2.0.0
---
# Canary 版本
apiVersion: apps/v1
kind: Deployment
metadata:
  name: web-canary
  namespace: production
spec:
  replicas: 1                  # 10% 流量
  selector:
    matchLabels:
      app: web
      track: canary
  template:
    metadata:
      labels:
        app: web               # 共享同一 Service
        track: canary
        version: v2.1.0-rc1
    spec:
      containers:
      - name: web
        image: registry.example.com/apps/web:v2.1.0-rc1
---
# Service 同时覆盖 stable 和 canary（通过 app: web 标签）
apiVersion: v1
kind: Service
metadata:
  name: web
  namespace: production
spec:
  selector:
    app: web                   # 匹配 stable + canary
  ports:
  - port: 80
    targetPort: 8080

```

## 常用 kubectl 操作速查

| 操作 | 命令 |
|------|------|
| 递归应用目录清单 | `kubectl apply -f manifests/ --recursive` |
| 查看更新状态 | `kubectl rollout status deployment/<name>` |
| 查看更新历史 | `kubectl rollout history deployment/<name>` |
| 暂停滚动更新 | `kubectl rollout pause deployment/<name>` |
| 恢复滚动更新 | `kubectl rollout resume deployment/<name>` |
| 回滚到上一版本 | `kubectl rollout undo deployment/<name>` |
| 回滚到指定版本 | `kubectl rollout undo deployment/<name> --to-revision=3` |
| 手动扩缩容 | `kubectl scale deployment/<name> --replicas=5` |
| JSON Patch | `kubectl patch deploy <name> -p '{"spec":{"replicas":5}}'` |
| Strategic Merge Patch | `kubectl patch deploy <name> --type=strategic -p '...'` |
| 破坏性更新（不可变字段） | `kubectl replace --force -f manifest.yaml` |

## 故障排查

| 症状 | 可能原因 | 排查步骤 |
|------|----------|----------|
| 滚动更新卡住，新 Pod Pending | 新镜像拉取失败或资源不足 | `kubectl rollout status`；`kubectl describe pod` 查看 Events |
| 回滚后版本不对 | revision 号码指定错误 | `kubectl rollout history` 确认目标 revision |
| HPA 与 kubectl apply 冲突 | manifest 中包含 `spec.replicas` | 从 manifest 中移除 `spec.replicas`，由 HPA 管理 |
| 金丝雀版本未收到流量 | Service selector 不匹配 canary Pod labels | 确认 Service selector 是两组 Pod 的公共标签 |
| `replace --force` 导致短暂停机 | 先删后建，中间无可用 Pod | 尽量使用 `apply` 或滚动更新；`replace --force` 仅用于修改不可变字段 |

## 生产检查清单

- [ ] 清单文件纳入 Git 版本控制
- [ ] 使用 `kubectl apply`（而非 `create` 或 `replace`）管理资源
- [ ] 滚动更新设置合理的 `maxSurge` 和 `maxUnavailable`
- [ ] 使用 HPA 时从 manifest 中移除 `spec.replicas`
- [ ] 金丝雀发布有独立的 Deployment + 共享 Service
- [ ] 所有 Deployment 设置 `revisionHistoryLimit`（推荐 5-10）
- [ ] 定期清理不再使用的 ConfigMap/Secret 版本
- [ ] CI/CD pipeline 中集成 `kubectl diff` 预览变更

## 命令快速参考

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `kubectl scale --replicas=0`：缩容到 0，立即停服
> - `kubectl apply/create/replace`：创建/变更集群资源
> - `kubectl delete`：删除资源（可由声明式清单重建）
> - `kubectl rollout undo/restart`：触发滚动变更，影响副本

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 预览变更（不实际应用）
kubectl diff -f manifests/

# 递归应用整个目录
kubectl apply -f manifests/ --recursive

# 按标签批量操作
kubectl get pods -l app=web -n production
kubectl delete pods -l version=v1.0 -n production

# 查看资源的最后应用配置
kubectl get deployment <name> -o jsonpath='{.metadata.annotations.kubectl\.kubernetes\.io/last-applied-configuration}' | jq .

# 批量重启所有 Deployment
kubectl rollout restart deployment -n production

# 使用 xargs 批量缩容
kubectl get deploy -n production -o name | xargs -I{} kubectl scale {} --replicas=0 -n production
```
## 交叉引用

- [Deployments](deployments.md) — 滚动更新和回滚的详细机制
- [水平 Pod 自动扩缩](horizontal-pod-autoscaling.md) — HPA 配置与 Deployment 的配合
- [自动扩缩工作负载](autoscaling-workloads.md) — HPA/VPA/KEDA 等全方位扩缩容方案
- [Disruptions](disruptions.md) — PDB 在更新过程中的保护作用

## 参考链接
- https://kubernetes.io/docs/concepts/workloads/management/

## Related

- [[21-生态参考/03-领域索引/pod-index.md|Pod 知识图谱索引]]
- [[21-生态参考/03-领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]

```

<!-- risk-assessed -->
