---
title: StatefulSets
description: '## 概述'
summary: '## 概述'
category: dictionary
tags:
- k8s
- glossary
- terminology
- etcd
- mysql
- postgresql
- kafka
- statefulset
- rag
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- StatefulSets 是什么
- 如何 StatefulSets
trigger_keywords:
- StatefulSets
- dictionary
prerequisites:
- kubectl-basics
- pod-lifecycle
- cloud-provider-basics
- etcd-basics
- kafka-basics
- mysql-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# StatefulSets

## 概述
[[StatefulSet|StatefulSet]] 是用于管理有状态应用的工作负载 API 对象。它管理一组基于相同容器规范运行的 Pod，并保证这些 Pod 的排序和唯一性。与 Deployment 不同，StatefulSet 为每个 Pod 维护一个粘性标识（sticky identity），即使 Pod 被重新调度，该标识也不会改变。

## 核心概念/原理
- **稳定网络标识**：每个 Pod 都有一个基于序号的唯一主机名，格式为 `$(statefulset-name)-$(ordinal)`。配合 Headless [[Service|Service]] 可提供稳定的 DNS 名称。
- **稳定存储**：通过 `volumeClaimTemplates` 为每个 Pod 自动创建 PersistentVolumeClaim。Pod 重新调度后，原有的 PVC 会重新挂载到新 Pod。
- **有序部署与扩缩容**：默认 `OrderedReady` 策略下，Pod 按序号 0 到 N-1 依次创建；缩容时按 N-1 到 0 依次删除。每个前置 Pod 必须 Running 且 Ready 后，才会继续下一步。
- **Pod 序号**：
  - 默认从 0 开始。
  - 自 v1.31 起可通过 `spec.ordinals.start` 自定义起始序号。
  - 控制器会自动添加标签 `apps.[[Kubernetes|kubernetes]].io/pod-index`（值为序号）。

## 关键机制或特性
- **Pod 管理策略**：
  - `OrderedReady`（默认）：严格按顺序创建和删除。
  - `Parallel`：并行创建和终止所有 Pod，不等待前一个就绪。
- **更新策略**：
  - `RollingUpdate`（默认）：按逆序逐个删除并重建 Pod。支持 `partition` 进行灰度更新；支持 `maxUnavailable`（Beta，默认 1）控制同时不可用的 Pod 数。
  - `OnDelete`：不自动更新，需手动删除 Pod 触发重建。
- **版本控制与回滚**：使用 ControllerRevision 保存历史配置，支持 `kubectl rollout history/undo` 回滚到指定版本。可通过 `revisionHistoryLimit` 控制保留数量。
- **PVC 保留策略（v1.32 Stable）**：通过 `persistentVolumeClaimRetentionPolicy` 配置 `whenDeleted` 和 `whenScaled` 策略（`Delete` 或 `Retain`），决定在 StatefulSet 删除或缩容时是否自动删除对应的 PVC。默认行为为 `Retain`。
- **最小就绪时间（`minReadySeconds`）**：Pod 就绪后需持续 healthy 的最短时间，才被视为可用。

## 使用场景
- 需要稳定网络标识和持久存储的数据库（如 MySQL、PostgreSQL、MongoDB）。
- 分布式协调服务（如 ZooKeeper、etcd）。
- 消息队列（如 Kafka、RabbitMQ）。

## 最佳实践/注意事项
- 必须为 StatefulSet 创建对应的 Headless Service，以控制 Pod 的网络域。
- StatefulSet 名称必须是有效的 DNS Label。
- 使用 `OnDelete` 策略时要特别注意，需要手动删除 Pod 才能应用更新。
- 使用 `partition` 进行灰度发布时，确保序号大于等于 partition 的 Pod 才会更新。
- 强烈建议不要设置 `pod.spec.terminationGracePeriodSeconds` 为 0，以确保安全终止。
- 设置 PVC 保留策略前需确保 API Server 和 Controller Manager 启用了 `StatefulSetAutoDeletePVC` 特性门控。

## 实战 YAML 示例

以下为生产级 PostgreSQL StatefulSet 配置：

```yaml
apiVersion: v1
kind: Service
metadata:
  name: postgres-headless
  namespace: prod
  labels:
    app: postgres
spec:
  ports:
  - port: 5432
    name: postgres
  clusterIP: None                            # Headless Service（必需）
  selector:
    app: postgres
---
apiVersion: apps/v1
kind: StatefulSet
metadata:
  name: postgres
  namespace: prod
spec:
  serviceName: postgres-headless             # 关联 Headless Service
  replicas: 3
  podManagementPolicy: OrderedReady          # 有序启停
  updateStrategy:
    type: RollingUpdate
    rollingUpdate:
      maxUnavailable: 1
      partition: 0                           # 灰度更新：设置 >0 可仅更新高序号 Pod
  selector:
    matchLabels:
      app: postgres
  template:
    metadata:
      labels:
        app: postgres
    spec:
      terminationGracePeriodSeconds: 120     # PostgreSQL 需要充足的终止时间
      serviceAccountName: postgres-sa
      securityContext:
        runAsUser: 999                       # postgres 用户
        fsGroup: 999
      containers:
      - name: postgres
        image: postgres:16.3
        ports:
        - containerPort: 5432
          name: postgres
        env:
        - name: POSTGRES_DB
          value: "mydb"
        - name: POSTGRES_USER
          valueFrom:
            secretKeyRef:
              name: postgres-secret
              key: username
        - name: POSTGRES_PASSWORD
          valueFrom:
            secretKeyRef:
              name: postgres-secret
              key: password
        - name: PGDATA
          value: "/var/lib/postgresql/data/pgdata"
        resources:
          requests:
            cpu: "500m"
            memory: "1Gi"
          limits:
            cpu: "2000m"
            memory: "4Gi"
        readinessProbe:
          exec:
            command:
            - pg_isready
            - -U
            - $(POSTGRES_USER)
          periodSeconds: 10
          timeoutSeconds: 5
        livenessProbe:
          exec:
            command:
            - pg_isready
            - -U
            - $(POSTGRES_USER)
          periodSeconds: 30
          timeoutSeconds: 5
          failureThreshold: 5
        volumeMounts:
        - name: data
          mountPath: /var/lib/postgresql/data
  volumeClaimTemplates:
  - metadata:
      name: data
    spec:
      accessModes: ["ReadWriteOnce"]
      storageClassName: fast-ssd              # 使用高性能存储类
      resources:
        requests:
          storage: 50Gi
  persistentVolumeClaimRetentionPolicy:
    whenDeleted: Retain                      # 删除 StatefulSet 时保留数据
    whenScaled: Retain                       # 缩容时保留 PVC
```

**DNS 解析规则**: Pod 的 DNS 名称为 `postgres-0.postgres-headless.prod.svc.cluster.local`

## 故障排查

### StatefulSet Pod 卡在 Pending（PVC 未绑定）
- **症状**: Pod 状态为 Pending，事件显示 `waiting for a volume to be created`。
- **常见原因**: StorageClass 不存在、存储配额不足、PV provisioner 未运行。
- **诊断命令**:
  ```bash
  # 查看 PVC 状态
  kubectl get pvc -n prod -l app=postgres
  # 查看 PVC 事件
  kubectl describe pvc data-postgres-0 -n prod
  # 检查 StorageClass
  kubectl get storageclass fast-ssd -o yaml
  # 查看 CSI 驱动状态
  kubectl get pods -n kube-system -l app=csi-provisioner
  ```

### 有序更新中某个 Pod 持续失败导致更新停滞
- **症状**: 滚动更新卡在某个序号的 Pod，后续 Pod 不再更新。
- **常见原因**: 新版本配置错误导致该序号 Pod 无法就绪。
- **诊断命令**:
  ```bash
  kubectl rollout status sts/postgres -n prod
  kubectl get pods -n prod -l app=postgres -o wide
  kubectl logs postgres-2 -n prod

  ```
- **解决方案**: 修复配置后等待自动恢复，或使用 `kubectl rollout undo` 回滚。也可使用 `partition` 参数将其设置为失败 Pod 序号+1，先稳定其他 Pod。

### 缩容后 PVC 残留
- **症状**: 缩容后旧 PVC 仍存在，占用存储资源。
- **常见原因**: `persistentVolumeClaimRetentionPolicy.whenScaled` 为 `Retain`（默认行为）。
- **诊断命令**:
  ```bash
  kubectl get pvc -n prod -l app=postgres
  ```
- **解决方案**: 手动删除不需要的 PVC，或将 `whenScaled` 设置为 `Delete`。

## 生产检查清单

- [ ] Headless Service 已创建（`clusterIP: None`）
- [ ] `volumeClaimTemplates` 已配置，使用合适的 StorageClass
- [ ] `terminationGracePeriodSeconds` 足够长（数据库至少 120 秒）
- [ ] `PodDisruptionBudget` 已创建，如 3 副本集群设置 `minAvailable: 2`
- [ ] 数据备份策略已建立（VolumeSnapshot 或外部备份工具）
- [ ] `persistentVolumeClaimRetentionPolicy` 已根据需求配置
- [ ] 探针使用应用级别的健康检查（如 `pg_isready`），而非简单的端口检查
- [ ] 敏感配置通过 Secret 挂载，而非环境变量明文
- [ ] 灰度更新策略已测试（`partition` 参数验证）

## 命令快速参考

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源
> - `kubectl rollout undo/restart`：触发滚动变更，影响副本

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 查看 StatefulSet 状态
kubectl get sts -n prod

# 查看滚动更新进度
kubectl rollout status sts/postgres -n prod

# 查看历史版本
kubectl rollout history sts/postgres -n prod

# 灰度更新（仅更新序号 >= 2 的 Pod）
kubectl patch sts postgres -n prod -p '{"spec":{"updateStrategy":{"rollingUpdate":{"partition":2}}}}'

# 完成灰度（更新所有 Pod）
kubectl patch sts postgres -n prod -p '{"spec":{"updateStrategy":{"rollingUpdate":{"partition":0}}}}'

# 回滚到上一版本
kubectl rollout undo sts/postgres -n prod

# 查看各 Pod 对应的 PVC
kubectl get pvc -n prod -l app=postgres
```
## 交叉引用

- [StatefulSet 高级运维](../../../02-%E5%B7%A5%E4%BD%9C%E8%B4%9F%E8%BD%BD/01-%E6%A0%B8%E5%BF%83%E5%B7%A5%E4%BD%9C%E8%B4%9F%E8%BD%BD/03-statefulset-advanced-operations.md)
- [工作负载概览与架构](../../../02-%E5%B7%A5%E4%BD%9C%E8%B4%9F%E8%BD%BD/01-%E6%A0%B8%E5%BF%83%E5%B7%A5%E4%BD%9C%E8%B4%9F%E8%BD%BD/01-workload-overview-architecture.md)
- [StatefulSet 故障树分析 (FTA)](../../../19-%E6%95%85%E9%9A%9C%E8%AF%8A%E6%96%AD/06-FTA%E6%95%85%E9%9A%9C%E6%A0%91/list/statefulset-fta.md)
- [存储 CSI 故障排查](../../[[19-故障诊断/01-核心排障/04-storage-csi-troubleshooting.md|04-storage-csi-troubleshooting]].md)
- [Pod Disruptions 中断管理](./disruptions.md)

## 参考链接
- https://kubernetes.io/docs/concepts/workloads/controllers/statefulset/

## Related

- [[21-生态参考/03-领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]

```

<!-- risk-assessed -->
