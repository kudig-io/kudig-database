---
title: 06 - 资源版本与并发控制 (Concurrency Control)
description: '## 专家解析：410 Gone 的终极治理'
summary: '在生产环境中，频繁出现 `410 Gone (Too old resource version)` 错误通常意味着你的 Watch 客户端跟不上 [[etcd|etcd]] 的压缩 (Compaction) 速度。'
category: design-principles
tags:
- k8s
- design
- principles
- etcd
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- 架构师
- SRE
estimated_read_time: 5min
intent_queries:
- 资源版本与并发控制 (Concurrency Control) 是什么
- 如何 资源版本与并发控制 (Concurrency Control)
- Kubernetes 2 design principles 最佳实践
trigger_keywords:
- 资源版本与并发控制
- Concurrency
- Control
- design
- principles
prerequisites:
- kubectl-basics
- kubernetes-concepts
- etcd-basics
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
authors:
- name: KUDIG Team
  role: contributor
cross_refs:
- type: domain
  path: ../集群基础/
  label: '相关知识域: 集群基础'
- type: domain
  path: ../集群基础/
  label: '相关知识域: 集群基础'
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 06 - 资源版本与并发控制 (Concurrency Control)

<!-- chunk: 专家解析：410 Gone 的终极治理 -->
## 专家解析：410 Gone 的终极治理

在生产环境中，频繁出现 `410 Gone (Too old resource version)` 错误通常意味着你的 Watch 客户端跟不上 [[etcd|etcd]] 的压缩 (Compaction) 速度。

### 根本原因分析
1. **etcd 压缩**: etcd 定期清理旧版本的 MVCC 数据。
2. **处理延迟**: 客户端处理事件太慢，导致其持有的 `ResourceVersion` 已经超出了 etcd 的保留窗口。

### 治理方案
* **优化 Handler 吞吐**: 使用工作队列并发处理。
* **合理设置 Resync**: 避免过短的 Resync 导致无意义的全量重新计算。
* **利用 Bookmarks**: 确保即使在无事件发生时，客户端的 RV 也能保持最新。

<!-- chunk: 核心概念 -->
## 核心概念

| 概念 | 英文 | 说明 |
|-----|-----|------|
| ResourceVersion | 资源版本 | 每个对象的版本号,每次修改递增 |
| Optimistic Concurrency | 乐观并发 | 假设冲突少,更新时检测冲突 |
| Pessimistic Concurrency | 悲观并发 | 假设冲突多,操作前加锁 |
| Conflict | 冲突 | 并发修改同一对象导致 |
| 409 Conflict | HTTP状态码 | 表示版本冲突 |

<!-- chunk: ResourceVersion来源 -->
## ResourceVersion来源

| 来源 | 说明 |
|-----|------|
| etcd revision | 全局递增的修订号 |
| 作用域 | 整个etcd集群全局唯一 |
| 递增时机 | 每次etcd事务提交 |
| 格式 | 字符串形式的数字 |

<!-- chunk: 乐观锁工作流程 -->
## 乐观锁工作流程

| 步骤 | 操作 | 说明 |
|-----|------|------|
| 1 | GET资源 | 获取当前resourceVersion |
| 2 | 修改对象 | 在内存中修改 |
| 3 | PUT/PATCH | 发送更新请求 |
| 4a | 成功 | RV匹配,更新成功,返回新RV |
| 4b | 409冲突 | RV不匹配,需要重试 |
| 5 | 重试 | 重新GET后再试 |

### 乐观锁示意图

```
Client A                    API Server                    Client B
   │                            │                            │
   │ GET pod (rv=100)           │                            │
   │◄───────────────────────────│                            │
   │                            │                            │
   │                            │         GET pod (rv=100)   │
   │                            │───────────────────────────►│
   │                            │                            │
   │                            │    PUT pod (rv=100) ─────►│
   │                            │    成功, 新 rv=101         │
   │                            │◄───────────────────────────│
   │                            │                            │
   │ PUT pod (rv=100) ─────────►│                            │
   │ 失败! 409 Conflict         │                            │
   │◄───────────────────────────│                            │
   │                            │                            │
   │ GET pod (rv=101) ─────────►│                            │
   │◄───────────────────────────│                            │
   │                            │                            │
   │ PUT pod (rv=101) ─────────►│                            │
   │ 成功, 新 rv=102            │                            │
   │◄───────────────────────────│                            │
```

<!-- chunk: ResourceVersion使用场景 -->
## ResourceVersion使用场景

| 场景 | RV值 | 含义 |
|-----|------|------|
| 更新操作 | 具体值 | 必须匹配当前RV |
| List | 空 | 获取最新数据 |
| List | "0" | 可从缓存读取 |
| Watch | 具体值 | 从该版本开始监听 |
| Watch | "0" | 从任意版本开始 |

<!-- chunk: 冲突处理策略 -->
## 冲突处理策略

| 策略 | 说明 | 适用场景 |
|-----|------|---------|
| 重试 | 获取最新版本后重试 | 大多数场景 |
| 合并 | 三方合并变更 | 复杂更新 |
| 覆盖 | 强制更新(危险) | 紧急修复 |
| 放弃 | 返回错误给用户 | 用户操作 |

### 重试模式代码

```go
func updateWithRetry(client kubernetes.Interface, pod *v1.Pod) error {
    return retry.RetryOnConflict(retry.DefaultRetry, func() error {
        // 1. 获取最新版本
        current, err := client.CoreV1().Pods(pod.Namespace).Get(
            context.TODO(), pod.Name, metav1.GetOptions{})
        if err != nil {
            return err
        }
        
        // 2. 应用修改
        current.Spec = pod.Spec
        
        // 3. 尝试更新
        _, err = client.CoreV1().Pods(pod.Namespace).Update(
            context.TODO(), current, metav1.UpdateOptions{})
        return err
    })
}
```

<!-- chunk: Generation vs ResourceVersion -->
## Generation vs ResourceVersion

| 维度 | Generation | ResourceVersion |
|-----|-----------|-----------------|
| 作用域 | 单个对象 | 全局 |
| 递增时机 | spec变更时 | 任何变更时 |
| 用途 | 判断spec是否变化 | 乐观锁/Watch |
| 控制器使用 | 判断是否需要调谐 | 冲突检测 |

### Generation使用示例

```go
// 控制器中判断是否需要处理
func needsReconcile(deploy *appsv1.Deployment) bool {
    // 如果observedGeneration < generation,说明有新的spec变更需要处理
    return deploy.Status.ObservedGeneration < deploy.Generation
}
```

<!-- chunk: SSA (Server-Side Apply) -->
## SSA (Server-Side Apply)

| 特性 | 说明 |
|-----|------|
| 字段管理器 | 每个字段记录管理者 |
| 冲突检测 | 不同管理者修改同一字段时冲突 |
| force参数 | 强制获取字段所有权 |
| 部分更新 | 只需提供要修改的字段 |

### SSA请求示例

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
kubectl apply --server-side --field-manager=my-controller -f pod.yaml
```
### SSA API调用

```go
patchOptions := metav1.PatchOptions{
    FieldManager: "my-controller",
    Force:        pointer.Bool(true), // 强制接管
}
client.CoreV1().Pods(ns).Patch(
    ctx, name, types.ApplyPatchType, 
    patchData, patchOptions)
```

<!-- chunk: Managed Fields -->
## Managed Fields

```yaml
# kubectl get pod nginx -o yaml 中的managedFields
metadata:
  managedFields:
  - manager: kubectl-client-side-apply
    operation: Update
    apiVersion: v1
    time: "2024-01-15T10:00:00Z"
    fieldsType: FieldsV1
    fieldsV1:
      f:metadata:
        f:labels:
          f:app: {}
      f:spec:
        f:containers:
          k:{"name":"nginx"}:
            .: {}
            f:image: {}
```

<!-- chunk: 最佳实践 -->
## 最佳实践

| 实践 | 说明 |
|-----|------|
| 使用retry库 | RetryOnConflict处理409 |
| 不要忽略RV | 更新时必须带RV |
| 使用SSA | 多控制器场景 |
| 检查Generation | 避免无意义的调谐 |
| 合理重试 | 设置重试上限 |

<!-- chunk: 常见问题 -->
## 常见问题

| 问题 | 原因 | 解决 |
|-----|------|------|
| 409 Conflict | 并发修改 | 重试 |
| 410 Gone | RV过旧(Watch) | 重新List |
| 频繁冲突 | 高并发更新 | 减少更新频率/分散处理 |
| 死循环更新 | 控制器互相触发 | 检查Generation |

---

**表格底部标记**: Kusheet Project, 作者 Allen Galler (allengaller@gmail.com)

---

<!-- chunk: Obsidian 相关文档 -->
## Obsidian 相关文档

- 集群基础 KUDIG Database — Global MOC
- [[01-集群基础/README.md|Domain-2: Kubernetes 设计原则与核心机制]]
- index.md|Domain-2 设计原则 — 开源项目索引]]
- Kubernetes 设计原则与哲学
- 声明式 API 与面向终态设计
- 控制器模式与调谐循环
- 04 - List-Watch 机制深度解析 (List-Watch)
- 05 - Informer 架构与工作队列 (Informer & Workqueue)
- 07 - 分布式共识与 etcd 原理 (etcd & Raft)
- 08 - 高可用架构模式 (HA Patterns)
- 09 - Kubernetes 源码结构与阅读指南 (Source Code)
- 10 - CAP 定理与分布式系统基础 (CAP Theorem)

## etcd 压缩调优与 410 Gone 深度诊断

### etcd Compaction 机制

```
etcd MVCC 存储模型:

  Revision 100: pod/nginx spec.image = "nginx:1.25"
  Revision 101: pod/nginx spec.image = "nginx:1.26"  ← 当前
  Revision 102: pod/nginx status.phase = "Running"
  ...
  Revision 200: 当前最新 revision

  Compaction (revision < 150 删除):
  [X] Rev 100  [X] Rev 101  ...  [✓] Rev 150+ 保留

  如果客户端持有 RV=100 尝试 Watch:
  → 410 Gone: "too old resource version"
```

### etcd 压缩参数调优

```bash
# 🟢 只读：查看当前 etcd 配置
kubectl -n kube-system get pod etcd-master-0 -o yaml | grep -A5 "command"

# 关键参数:
# --auto-compaction-mode=periodic   # 或 revision
# --auto-compaction-retention=5m    # periodic 模式: 保留 5 分钟历史
# --auto-compaction-retention=10000 # revision 模式: 保留 10000 个版本
```

| 参数 | 默认值 | 建议值 | 说明 |
|------|--------|--------|------|
| auto-compaction-mode | periodic | periodic | 按时间压缩更可控 |
| auto-compaction-retention | 5m | 5m-15m | 增大可减少 410 Gone |
| quota-backend-bytes | 2GB | 4-8GB | 大集群建议增大 |
| snapshot-count | 10000 | 10000 | 快照触发阈值 |

### 410 Gone 故障排查流程

```bash
# 🟢 只读：检查 API Server 日志中的 410 错误
kubectl logs -n kube-system kube-apiserver-master-0 --tail=500 | grep "410"

# 🟢 只读：查看当前 etcd revision
kubectl exec -n kube-system etcd-master-0 -- etcdctl \
  --endpoints=https://127.0.0.1:2379 \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/server.crt \
  --key=/etc/kubernetes/pki/etcd/server.key \
  endpoint status --write-out=table

# 🟢 只读：检查哪些客户端触发了 410
kubectl logs -n kube-system kube-apiserver-master-0 --tail=2000 | \
  grep "too old resource version" | \
  awk '{print $NF}' | sort | uniq -c | sort -rn | head -10

# 🟢 只读：检查 etcd 压缩历史
kubectl exec -n kube-system etcd-master-0 -- etcdctl \
  --endpoints=https://127.0.0.1:2379 \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/server.crt \
  --key=/etc/kubernetes/pki/etcd/server.key \
  get / --prefix --keys-only --limit=1 -w json | jq '.header.revision'
```

### 410 Gone 常见原因与修复

| 原因 | 症状 | 修复方案 |
|------|------|----------|
| 客户端处理太慢 | 单个控制器频繁 410 | 优化 Handler 吐吐量，增加并发 worker |
| 压缩窗口太短 | 多个客户端同时 410 | 增大 auto-compaction-retention |
| 网络分区 | 客户端恢复后 410 | 实现自动 Re-List 逻辑 |
| etcd 性能瓶颈 | 全局延迟增高 | 优化 etcd 磁盘 IOPS |
| 大量 List 请求 | API Server 压力大 | 使用 RV="0" 从缓存读取 |

## Watch Bookmark 机制详解

### Bookmark 工作原理

```
无 Bookmark 场景:
  Client RV=100 ──── Watch ────> [30min 无事件]
  etcd Compaction 删除 RV<150
  Client 尝试继续 Watch RV=100 → 410 Gone!

有 Bookmark 场景:
  Client RV=100 ──── Watch ────> [Bookmark RV=120]
                                 [Bookmark RV=140]
                                 [Bookmark RV=160]  ← 客户端更新 RV
  etcd Compaction 删除 RV<150
  Client Watch RV=160 → ✅ 成功继续
```

### 启用 Bookmark

```go
// 客户端启用 Bookmark
import "k8s.io/apimachinery/pkg/apis/meta/v1"

listOptions := metav1.ListOptions{
    ResourceVersion:      "",  // 从最新开始
    AllowWatchBookmarks:  true, // 启用 Bookmark
}

watcher, err := client.CoreV1().Pods("").Watch(ctx, listOptions)
for event := range watcher.ResultChan() {
    switch event.Type {
    case watch.Bookmark:
        // 更新本地 RV，不处理业务逻辑
        lastRV = event.Object.(*v1.Pod).ResourceVersion
        log.Debugf("Bookmark received, RV updated to %s", lastRV)
    case watch.Added, watch.Modified, watch.Deleted:
        // 正常业务处理
        handleEvent(event)
        lastRV = event.Object.(*v1.Pod).ResourceVersion
    }
}
```

### Bookmark 监控指标

```promql
# API Server Bookmark 发送率
rate(apiserver_watch_events_sizes_sum{resource="pods"}[5m])

# Watch 缓存命中率
apiserver_watch_cache_events_dispatched_total

# 410 Gone 错误率
rate(apiserver_request_total{code="410"}[5m])
```

## 大规模集群 RV 管理

### 性能基准

| 集群规模 | 对象数 | etcd Revision 增速 | 压缩窗口建议 |
|----------|--------|-------------------|------------|
| 小型 (<100 节点) | <50K | ~100 rev/s | 5m |
| 中型 (100-500 节点) | 50K-500K | ~500 rev/s | 10m |
| 大型 (500-2000 节点) | 500K-2M | ~2000 rev/s | 15m |
| 超大型 (>2000 节点) | >2M | >5000 rev/s | 15m + 分片 |

### 大规模集群优化策略

```yaml
# API Server 优化配置 (kubeadm)
apiVersion: kubeadm.k8s.io/v1beta4
kind: ClusterConfiguration
apiServer:
  extraArgs:
    watch-cache-sizes: "pods=1000,nodes=100,configmaps=200"
    default-watch-cache-size: "100"
    max-requests-inflight: "400"
    max-mutating-requests-inflight: "200"
    # 启用 API Priority and Fairness
    enable-priority-and-fairness: "true"
```

### APF 对 Watch 的保护

```yaml
# 为控制器 Watch 请求设置优先级
apiVersion: flowcontrol.apiserver.k8s.io/v1
kind: FlowSchema
metadata:
  name: controller-watch-protection
spec:
  priorityLevelConfiguration:
    name: workload-high
  matchingPrecedence: 800
  rules:
    - subjects:
        - kind: ServiceAccount
          serviceAccount:
            name: "*"
            namespace: kube-system
      resourceRules:
        - verbs: ["watch", "list"]
          apiGroups: ["*"]
          resources: ["*"]
          namespaces: ["*"]
```

## controller-runtime 冲突处理模式

### 标准 Reconcile 冲突处理

```go
import (
    "context"
    ctrl "sigs.k8s.io/controller-runtime"
    "sigs.k8s.io/controller-runtime/pkg/client"
    "k8s.io/client-go/util/retry"
)

func (r *MyReconciler) Reconcile(ctx context.Context, req ctrl.Request) (ctrl.Result, error) {
    // 1. 获取当前状态
    var app v1.MyApp
    if err := r.Get(ctx, req.NamespacedName, &app); err != nil {
        return ctrl.Result{}, client.IgnoreNotFound(err)
    }

    // 2. 检查 Generation 是否需要调谐
    if app.Status.ObservedGeneration >= app.Generation {
        return ctrl.Result{}, nil // 无需处理
    }

    // 3. 执行业务逻辑
    desiredState := computeDesiredState(&app)

    // 4. 更新状态（带冲突重试）
    err := retry.RetryOnConflict(retry.DefaultRetry, func() error {
        var current v1.MyApp
        if err := r.Get(ctx, req.NamespacedName, &current); err != nil {
            return err
        }
        current.Status.ObservedGeneration = current.Generation
        current.Status.State = desiredState
        return r.Status().Update(ctx, &current)
    })
    if err != nil {
        return ctrl.Result{}, err
    }

    return ctrl.Result{}, nil
}
```

### 多控制器字段所有权管理

```go
// 控制器 A: 只管理 spec.replicas
func (r *ReplicaController) Reconcile(ctx context.Context, req ctrl.Request) (ctrl.Result, error) {
    patch := client.MergeFrom(original.DeepCopy())
    
    // 使用 SSA 避免与其他控制器冲突
    obj := &appsv1.Deployment{
        ObjectMeta: metav1.ObjectMeta{
            Name:      req.Name,
            Namespace: req.Namespace,
        },
        Spec: appsv1.DeploymentSpec{
            Replicas: pointer.Int32(desiredReplicas),
        },
    }
    
    err := r.Patch(ctx, obj, client.Apply, 
        client.FieldOwner("replica-controller"),
        client.ForceOwnership,
    )
    return ctrl.Result{}, err
}

// 控制器 B: 只管理 spec.template.spec.containers[].image
func (r *ImageController) Reconcile(ctx context.Context, req ctrl.Request) (ctrl.Result, error) {
    obj := &appsv1.Deployment{
        ObjectMeta: metav1.ObjectMeta{
            Name:      req.Name,
            Namespace: req.Namespace,
        },
        Spec: appsv1.DeploymentSpec{
            Template: corev1.PodTemplateSpec{
                Spec: corev1.PodSpec{
                    Containers: []corev1.Container{
                        {Name: "app", Image: newImage},
                    },
                },
            },
        },
    }
    
    err := r.Patch(ctx, obj, client.Apply,
        client.FieldOwner("image-controller"),
    )
    return ctrl.Result{}, err
}
```

### SSA 冲突检测与解决

```bash
# 🟢 只读：查看对象字段所有权
kubectl get deployment nginx -o jsonpath='{.metadata.managedFields}' | jq '.'

# 🟢 只读：查看特定字段的管理者
kubectl get deployment nginx -o json | jq '.metadata.managedFields[] | {manager, fieldsV1}'

# 🟡 中风险：强制接管字段所有权
kubectl apply --server-side --field-manager=my-controller --force-conflicts -f deploy.yaml

# 🟢 只读：检查 SSA 冲突（dry-run）
kubectl apply --server-side --field-manager=new-controller --dry-run=server -f deploy.yaml
```

## 诊断命令集

```bash
# 🟢 只读：查看对象当前 ResourceVersion
kubectl get pod nginx -o jsonpath='{.metadata.resourceVersion}'

# 🟢 只读：查看对象 Generation
kubectl get deployment nginx -o jsonpath='{.metadata.generation}'

# 🟢 只读：查看 ObservedGeneration
kubectl get deployment nginx -o jsonpath='{.status.observedGeneration}'

# 🟢 只读：检查控制器是否落后
kubectl get deployment nginx -o json | \
  jq '{generation: .metadata.generation, observed: .status.observedGeneration}'

# 🟢 只读：查看 etcd 当前 revision
kubectl exec -n kube-system etcd-master-0 -- etcdctl \
  --endpoints=https://127.0.0.1:2379 \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/server.crt \
  --key=/etc/kubernetes/pki/etcd/server.key \
  endpoint status -w table

# 🟢 只读：API Server Watch 缓存状态
kubectl get --raw /metrics | grep apiserver_watch

# 🟢 只读：检查 409/410 错误率
kubectl get --raw /metrics | grep -E "apiserver_request_total.*code=\"(409|410)\""

# 🟢 只读：查看对象 managedFields
kubectl get pod nginx -o jsonpath='{.metadata.managedFields[*].manager}'

# 🟡 中风险：强制重新触发调谐
kubectl annotate deployment nginx kubectl.kubernetes.io/restartedAt=$(date -u +%Y-%m-%dT%H:%M:%SZ) --overwrite
```

## 监控告警

### PrometheusRule — RV 与并发控制

```yaml
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: rv-concurrency-alerts
  namespace: monitoring
spec:
  groups:
    - name: resource-version
      rules:
        - alert: HighConflictRate
          expr: |
            rate(apiserver_request_total{code="409"}[5m]) > 10
          for: 10m
          labels:
            severity: warning
          annotations:
            summary: "API 冲突率过高 ({{ $value | printf \"%.1f\" }}/s)，检查控制器竞争"

        - alert: WatchGoneErrors
          expr: |
            rate(apiserver_request_total{code="410", resource="pods"}[5m]) > 1
          for: 5m
          labels:
            severity: warning
          annotations:
            summary: "Watch 410 Gone 错误频繁，检查 etcd 压缩配置或客户端性能"

        - alert: ControllerReconcileLag
          expr: |
            kube_deployment_status_observed_generation < kube_deployment_metadata_generation
          for: 15m
          labels:
            severity: warning
          annotations:
            summary: "Deployment {{ $labels.namespace }}/{{ $labels.deployment }} 控制器落后 >15min"

        - alert: EtcdHighRevisionGrowth
          expr: |
            rate(etcd_server_proposals_committed_total[5m]) > 5000
          for: 10m
          labels:
            severity: warning
          annotations:
            summary: "etcd revision 增速过快 ({{ $value }}/s)，检查是否有异常写入"
```

## 最佳实践总结

| 场景 | 推荐方案 | 避免 |
|------|----------|------|
| 单控制器更新 | 乐观锁 + RetryOnConflict | 忽略 409 错误 |
| 多控制器管理同一对象 | SSA + FieldOwner | 全量 Update 覆盖 |
| 高频 List | RV="0" 从缓存读 | 每次 List 都走 etcd |
| 长连接 Watch | 启用 Bookmark | 不处理 Bookmark 事件 |
| 控制器调谐 | 检查 Generation | 每次事件都全量计算 |
| 大规模集群 | APF + Watch Cache 调优 | 默认配置不变 |
| 紧急修复 | --force-conflicts | 生产环境随意 force |

## See Also

- 04-watch-list-mechanism
- 05-informer-workqueue
- 07-distributed-consensus-etcd
- 08-high-availability-patterns

## Related

- [[21-生态参考/03-领域索引/etcd-index.md|[[etcd 知识图谱索引|etcd 知识图谱索引]]]]


- [[10-平台工程/06-代码分析/kubernetes-core/05-etcd-storage-deep-dive.md|etcd 与存储链路源码剖析（resourceVersion ↔ etcd revision 的源码本体）]]

<!-- risk-assessed -->
