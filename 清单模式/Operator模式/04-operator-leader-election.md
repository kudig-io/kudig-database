---
title: Operator Leader Election 高可用
description: Leader Election 机制、Lease 锁配置与多副本控制器部署
summary: 使用 Kubernetes Lease 实现控制器高可用，避免多副本同时写入导致冲突
category: manifests-patterns
tags:
- k8s
- manifests
- operator
- leader-election
- ha
- reliability
tier: core
created: '2026-07-11'
last_updated: 2026-07
difficulty: advanced
reading_level: advanced
audience:
- 平台工程师
- SRE
estimated_read_time: 10min
intent_queries:
- Operator 高可用部署
- Leader Election 配置
- controller-runtime leader election
trigger_keywords:
- leader-election
- lease
- ha
- controller
prerequisites:
- operator-basics
authors:
- name: KUDIG Team
  role: contributor
---

# Operator Leader Election 高可用

## 1. 为什么需要 Leader Election

当 Operator 部署多副本时，如果所有副本同时 Reconcile，会导致：
- 重复创建资源
- Status 写入冲突
- 外部资源重复操作

Leader Election 确保同一时刻**只有一个副本**执行调谐逻辑，其余副本处于待命状态。

## 2. Lease 锁机制

Kubernetes 使用 `coordination.k8s.io/v1` Lease 资源作为分布式锁：

```yaml
apiVersion: coordination.k8s.io/v1
kind: Lease
metadata:
  name: webapp-operator-leader
  namespace: webapp-system
spec:
  holderIdentity: webapp-controller-manager-7d4f6b-x2k9m
  leaseDurationSeconds: 15        # Leader 持锁时长
  acquireTime: "2026-07-11T08:00:00Z"
  renewTime: "2026-07-11T08:00:10Z"  # Leader 定期续约
  leaseTransitions: 3             # Leader 切换次数
```

## 3. controller-runtime 配置

```go
func main() {
    mgr, err := ctrl.NewManager(ctrl.GetConfigOrDie(), ctrl.Options{
        Scheme:                 scheme,
        Metrics:                server.Options{BindAddress: ":8080"},
        HealthProbeBindAddress: ":8081",
        LeaderElection:         true,
        LeaderElectionID:       "webapp-operator.platform.example.com",
        LeaderElectionNamespace: "webapp-system",
        LeaseDuration:          ptr.To(15 * time.Second),
        RenewDeadline:          ptr.To(10 * time.Second),
        RetryPeriod:            ptr.To(2 * time.Second),
    })
    if err != nil {
        setupLog.Error(err, "unable to start manager")
        os.Exit(1)
    }
    // ...
}
```

| 参数 | 推荐值 | 说明 |
|------|--------|------|
| `LeaseDuration` | 15s | Leader 持锁时长，超时后其他副本可抢占 |
| `RenewDeadline` | 10s | Leader 续约间隔，必须 < LeaseDuration |
| `RetryPeriod` | 2s | 非 Leader 重试获取锁的间隔 |

## 4. 部署清单

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: webapp-controller-manager
  namespace: webapp-system
spec:
  replicas: 2  # 多副本实现高可用
  selector:
    matchLabels:
      control-plane: controller-manager
  template:
    metadata:
      labels:
        control-plane: controller-manager
    spec:
      serviceAccountName: webapp-controller-manager
      containers:
        - name: manager
          image: registry.example.com/webapp-operator:v1.0.0
          args:
            - --leader-elect
            - --leader-election-id=webapp-operator.platform.example.com
            - --leader-election-namespace=webapp-system
          env:
            - name: POD_NAME
              valueFrom:
                fieldRef:
                  fieldPath: metadata.name
            - name: POD_NAMESPACE
              valueFrom:
                fieldRef:
                  fieldPath: metadata.namespace
          livenessProbe:
            httpGet:
              path: /healthz
              port: 8081
            initialDelaySeconds: 15
            periodSeconds: 20
          readinessProbe:
            httpGet:
              path: /readyz
              port: 8081
            initialDelaySeconds: 5
            periodSeconds: 10
          resources:
            requests:
              cpu: 100m
              memory: 128Mi
            limits:
              cpu: 500m
              memory: 256Mi
      terminationGracePeriodSeconds: 10
```

## 5. RBAC 权限

```yaml
apiVersion: rbac.authorization.k8s.io/v1
kind: Role
metadata:
  name: webapp-leader-election-role
  namespace: webapp-system
rules:
  - apiGroups: [""]
    resources: ["configmaps", "events"]
    verbs: ["create", "get", "list", "watch", "update", "patch"]
  - apiGroups: ["coordination.k8s.io"]
    resources: ["leases"]
    verbs: ["create", "get", "list", "watch", "update", "patch", "delete"]
```

## 6. 故障切换流程

```
Leader (Pod-A) 崩溃
      ↓
Lease 超过 leaseDurationSeconds 未续约 (15s)
      ↓
Pod-B 检测到 Lease 过期
      ↓
Pod-B 获取 Lease，成为新 Leader
      ↓
Pod-B 开始 Reconcile（新 Leader 需要 0-2s 获取锁）
      ↓
总故障切换时间 ≈ 15s (LeaseDuration) + 2s (RetryPeriod)
```

## 7. 生产建议

- **副本数 2-3 即可**，过多副本浪费资源且不缩短切换时间
- **LeaseDuration 不要太短**，避免网络抖动导致频繁切换
- 监控 `leader_election_master_status` 指标
- 在 liveness probe 中检查 leader election 状态，避免僵尸进程

## Related

- [[清单模式/Operator模式/07-operator-metrics-observability|Operator Metrics 可观测性]]
- [[清单模式/07-resilience-patterns/01-pdb-patterns|PDB 模式]]

## See Also

- [client-go Leader Election](https://pkg.go.dev/k8s.io/client-go/tools/leaderelection)
- [controller-runtime HA 文档](https://book.kubebuilder.io/reference/leader-election)

<!-- risk-assessed -->
