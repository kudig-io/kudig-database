---
title: StatefulSet
description: StatefulSet — Kubernetes 生产运维知识库
summary: StatefulSet — Kubernetes 生产运维知识库
category: entities
tags:
- k8s
- statefulset
- workload
- stateful
- ordered
- persistent-storage
- mysql
- postgresql
- kafka
- elasticsearch
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- StatefulSet 是什么
- 如何 StatefulSet
trigger_keywords:
- StatefulSet
prerequisites:
- kubectl-basics
- kafka-basics
- mysql-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# StatefulSet

## Role

StatefulSet manages stateful workloads that require stable identity and persistent storage, such as databases (MySQL, PostgreSQL, Elasticsearch, Kafka).

## Key Properties

| Property | Description |
|----------|-------------|
| **Stable Pod identity** | [[pods\|Pods]] named `{name}-{0}`, `{name}-{1}`, ... in order |
| **Ordered operations** | Pods created 0→N, terminated N→0 |
| **Persistent storage** | Each Pod gets its own PVC from `volumeClaimTemplates` |
| **Stable network** | DNS via Headless [[service\|Service]]: `pod-0.service.ns.svc.cluster.local` |
| **PVC retention** | PVCs survive Pod deletion (data persists) |

## Update Strategy

| Strategy | Behavior |
|----------|----------|
| **RollingUpdate** | Update Pods in reverse order (N→0), waiting for each to be Ready |
| **OnDelete** | Only update when Pods are manually deleted |
| **Partition** | Update only Pods with index >= partition (canary rollout) |

## Volume Claim Templates

Each Pod gets a dedicated PVC. Unlike Deployment where Pods share a volume template, StatefulSet creates unique PVCs per Pod, ensuring data isolation.

## Use Cases

Databases (MySQL, PostgreSQL, MongoDB), message brokers (Kafka, RabbitMQ), search engines (Elasticsearch), and any application requiring persistent identity and storage.

## 完整 YAML 示例

```yaml
apiVersion: apps/v1
kind: StatefulSet
metadata:
  name: postgresql
  namespace: database
spec:
  serviceName: postgresql-headless
  replicas: 3
  podManagementPolicy: OrderedReady  # 或 Parallel
  updateStrategy:
    type: RollingUpdate
    rollingUpdate:
      partition: 0  # 金丝雀: 设为 2 则只更新 pod-2
  selector:
    matchLabels:
      app: postgresql
  template:
    metadata:
      labels:
        app: postgresql
    spec:
      terminationGracePeriodSeconds: 60
      containers:
      - name: postgres
        image: postgres:16-alpine
        ports:
        - containerPort: 5432
          name: postgresql
        env:
        - name: POSTGRES_PASSWORD
          valueFrom:
            secretKeyRef:
              name: postgresql-secret
              key: password
        - name: PGDATA
          value: /var/lib/postgresql/data/pgdata
        resources:
          requests:
            cpu: 500m
            memory: 1Gi
          limits:
            cpu: 2
            memory: 4Gi
        volumeMounts:
        - name: data
          mountPath: /var/lib/postgresql/data
        livenessProbe:
          exec:
            command: ["pg_isready", "-U", "postgres"]
          initialDelaySeconds: 30
          periodSeconds: 10
        readinessProbe:
          exec:
            command: ["pg_isready", "-U", "postgres"]
          initialDelaySeconds: 5
          periodSeconds: 5
  volumeClaimTemplates:
  - metadata:
      name: data
    spec:
      accessModes: ["ReadWriteOnce"]
      storageClassName: fast-ssd
      resources:
        requests:
          storage: 100Gi
---
# Headless Service (必须)
apiVersion: v1
kind: Service
metadata:
  name: postgresql-headless
  namespace: database
spec:
  clusterIP: None  # Headless
  selector:
    app: postgresql
  ports:
  - port: 5432
    name: postgresql
```

## 运维操作

### 常用命令

```bash
# 🟢 查看 StatefulSet
kubectl get statefulset -A
kubectl describe statefulset postgresql -n database

# 🟢 查看 Pod 状态 (有序索引)
kubectl get pods -n database -l app=postgresql -o wide

# 🟢 查看 PVC
kubectl get pvc -n database -l app=postgresql

# 🟡 扩容
kubectl scale statefulset postgresql --replicas=5 -n database

# 🟡 缩容 (注意: PVC 不会自动删除)
kubectl scale statefulset postgresql --replicas=3 -n database

# 🔴 删除 StatefulSet (保留 PVC)
kubectl delete statefulset postgresql -n database --cascade=orphan

# 🔴 删除 StatefulSet 和 Pod (保留 PVC)
kubectl delete statefulset postgresql -n database

# 🔴 删除所有 PVC (数据丢失!)
kubectl delete pvc -l app=postgresql -n database

# 🟡 金丝雀更新 (partition)
kubectl patch statefulset postgresql -n database -p '{"spec":{"updateStrategy":{"rollingUpdate":{"partition":2}}}}'

# 🟢 查看更新状态
kubectl rollout status statefulset postgresql -n database

# 🟡 强制回滚
kubectl rollout undo statefulset postgresql -n database

# 🟢 查看 Pod DNS
kubectl exec -it postgresql-0 -n database -- nslookup postgresql-0.postgresql-headless.database.svc.cluster.local
```

## 故障排查

| 症状 | 可能原因 | 排查命令 | 修复方案 |
|------|----------|----------|----------|
| Pod Pending | PVC 未绑定/资源不足 | `kubectl describe pod <name>-0` | 检查 StorageClass 和节点资源 |
| Pod CrashLoopBackOff | 应用启动失败/数据损坏 | `kubectl logs <pod> --previous` | 检查应用日志和数据目录 |
| 更新卡住 | Pod 未 Ready/探针失败 | `kubectl rollout status sts <name>` | 检查 readinessProbe 配置 |
| PVC 未创建 | volumeClaimTemplates 错误 | `kubectl get pvc -l app=<name>` | 检查 storageClassName 是否存在 |
| 网络不通 | Headless Service 缺失 | `kubectl get svc <name>-headless` | 创建 clusterIP: None 的 Service |
| 缩容后 PVC 残留 | 设计行为 (PVC 不自动删除) | `kubectl get pvc` | 手动删除不需要的 PVC |
| Pod 顺序混乱 | podManagementPolicy=Parallel | `kubectl get pods -o wide` | 改为 OrderedReady (如需严格顺序) |

### 排查流程

```
1. kubectl get statefulset → 确认 replicas/ready 状态
2. kubectl get pods -l app=<name> → 查看各 Pod 状态
3. kubectl describe pod <name>-<N> → 查看 Events
4. kubectl get pvc → 确认 PVC 绑定状态
5. kubectl logs <pod> --previous → 查看崩溃日志
6. kubectl get svc <name>-headless → 确认 Headless Service
```

## 生产最佳实践

### 数据库 StatefulSet 检查清单

- [ ] 使用 Headless Service (clusterIP: None)
- [ ] volumeClaimTemplates 指定高性能 StorageClass
- [ ] 配置合理的 terminationGracePeriodSeconds (>= 60s)
- [ ] 设置 livenessProbe 和 readinessProbe
- [ ] 资源 requests/limits 已设置
- [ ] 使用 PodDisruptionBudget 保护可用性
- [ ] 配置 Pod 反亲和 (分散到不同节点)
- [ ] 定期备份 PVC 数据
- [ ] 更新使用 partition 金丝雀策略
- [ ] 监控 PVC 使用率和 Pod 状态

### StatefulSet vs Deployment 选型

| 维度 | StatefulSet | Deployment |
|------|-------------|------------|
| Pod 标识 | 稳定有序 (name-0,1,2) | 随机哈希 |
| 存储 | 每 Pod 独立 PVC | 共享或无状态 |
| 网络标识 | 稳定 DNS | 不稳定 |
| 扩缩容 | 有序 | 并行 |
| 适用场景 | 数据库/消息队列 | 无状态 Web/API |
| 更新顺序 | 严格有序 | 并行滚动 |

## 生产案例

### 案例1: Elasticsearch 集群滚动更新卡住
- **场景**: ES 集群 5 节点，更新时 pod-3 一直 Pending
- **根因**: PVC 请求的 StorageClass 在新节点不可用 (zone 限制)
- **解决**: 使用 topology-aware StorageClass，确保 PV 跨 zone 可用

### 案例2: Kafka StatefulSet 缩容数据丢失
- **场景**: Kafka 从 5 缩容到 3，删除 PVC 后 topic 数据丢失
- **根因**: 直接删除 PVC 未先迁移 partition
- **解决**: 缩容前使用 kafka-reassign-partitions 迁移数据，确认无副本在待删 broker

## Related
- [[22-概念/11-交叉分析/Operator 模式 × Pod 生命周期.md|Operator 模式 × Pod 生命周期]] — 综合

- [[26-技能/04-工作负载/deployment/deployment-workload-selection.md|deployment-workload-selection]] — 工作负载控制器选型
- [[26-技能/04-工作负载/statefulset/skill-21-statefulset-failure.md|skill-21-statefulset-failure]] — StatefulSet 故障诊断与修复 / StatefulSet Failure Diagnosis & Remediation
- [[INDEX]] — Wiki Index
- [[deployment]] — Deployment
- [[22-概念/04-存储/storage-model.md|storage-model]] — Persistent Storage Model (PV/PVC/StorageClass)
- [[deployment|Deployment]]
- [[22-概念/04-存储/storage-model.md|Persistent Storage Model]]
- [[pod-lifecycle|Pod Lifecycle]]
- Headless Service

- 08-statefulset-daemonset-events
- 05-statefulset-reference
- 03-statefulset-advanced-operations
- [[19-故障诊断/02-资源排障/13-statefulset-troubleshooting.md|21-statefulset-troubleshooting]]
- [[19-故障诊断/06-FTA故障树/list/statefulset-fta.md|StatefulSet 异常故障树分析]]
- [[19-故障诊断/04-高级排障/structural-05-workloads/03-statefulset-troubleshooting.md|03-statefulset-troubleshooting]]
- [[26-技能/04-工作负载/statefulset/statefulset-fta.md|StatefulSet 异常故障树分析]] — Cross-reference


<!-- risk-assessed -->
- [[01-集群基础/02-设计原则/03-declarative-api-pattern|02 - 声明式 API 与面向终态设计 (Declarative API)]]
- [[02-工作负载/01-核心工作负载/README-old|Domain-4: Kubernetes工作负载]]
- [[02-工作负载/01-核心工作负载/23-resource-management|16 - 资源管理表]]
- [[04-应用模式/02-行业架构/04-im-rtc-architecture|实时通信 (IM / RTC) Kubernetes 生产架构设计]]
- [[04-应用模式/02-行业架构/03-cms-architecture|内容管理系统 (CMS) Kubernetes 生产架构设计]]
- [[04-应用模式/02-行业架构/93-digital-twin-factory|数字孪生工厂架构设计 — 阿里云视角]]
- [[04-应用模式/02-行业架构/54-social-gaming-metaverse|社交游戏与元宇宙社交架构设计 — 阿里云视角]]
- [[04-应用模式/02-行业架构/31-instant-retail|即时零售架构设计 — 阿里云视角]]
- [[04-应用模式/02-行业架构/70-ecny-cbdc|数字人民币架构设计 — 阿里云视角]]
- [[04-应用模式/02-行业架构/42-secondhand-circular|二手交易与循环经济架构设计 — 阿里云视角]]
- [[04-应用模式/02-行业架构/26-aviation-travel|航空出行架构设计 — 阿里云视角]]
- [[04-应用模式/02-行业架构/43-enterprise-im|企业即时通讯架构设计 — 阿里云视角]]
- [[04-应用模式/02-行业架构/25-quantitative-trading|证券量化交易架构设计 — 阿里云视角]]
- [[04-应用模式/02-行业架构/48-vocational-edtech|职业教育培训架构设计 — 阿里云视角]]
- [[09-可观测性/00-总览/04-elastic-stack-enterprise-observability|Elastic Stack企业级可观测性平台深度实践]]
- [[09-可观测性/02-日志/07-splunk-enterprise-siem|Splunk企业级日志分析与安全智能平台深度实践]]
- [[11-发布变更/07-迁移方案/06-stateful-services-migration|06 - 有状态服务迁移 [migration]]]
- [[17-系统基础/04-K8s事件/09-job-cronjob-batch-events|09 - Job 与 CronJob 批处理事件]]
- [[17-系统基础/04-K8s事件/12-autoscaling-events|12 - 自动扩缩容事件 (HPA / VPA / Cluster Autoscaler)]]
- [[17-系统基础/04-K8s事件/08-statefulset-daemonset-events|08 - StatefulSet 与 DaemonSet 控制器事件]]
- [[17-系统基础/04-K8s事件/14-namespace-resource-gc-events|14 - Namespace、资源管理与垃圾回收事件]]
- [[17-系统基础/06-知识字典/fundamentals/kubernetes-self-healing|Kubernetes Self-Healing（Kubernetes 自愈能力）]]
- [[17-系统基础/06-知识字典/fundamentals/objects-in-kubernetes|Kubernetes 中的对象]]
- [[17-系统基础/06-知识字典/operations/node-shutdowns|节点关闭（Node Shutdowns）]]
- [[17-系统基础/06-知识字典/storage/dynamic-volume-provisioning|Dynamic Volume Provisioning（动态卷供给）]]
- [[17-系统基础/06-知识字典/storage/node-specific-volume-limits|Node-specific Volume Limits（节点特定卷限制）]]
- [[17-系统基础/06-知识字典/workloads/horizontal-pod-autoscaling|Horizontal Pod Autoscaling]]
- [[17-系统基础/06-知识字典/workloads/managing-workloads|Managing Workloads]]
- [[17-系统基础/06-知识字典/workloads/workload-management|Workload Management]]
- [[17-系统基础/06-知识字典/workloads/pod-hostname|Pod Hostname]]
- [[19-故障诊断/02-资源排障/07-ingress-troubleshooting|15 - Ingress 故障排查 (Ingress Troubleshooting)]]
- [[20-最佳实践/04-migration/06-stateful-services-migration|06 - 有状态服务迁移 [migration]]]
- [[22-概念/11-交叉分析/etcd-×-StatefulSet|etcd × StatefulSet]]
- [[22-概念/11-交叉分析/StatefulSet-×-Service|StatefulSet × Service]]
- [[22-概念/11-交叉分析/apiserver-×-StatefulSet|apiserver × StatefulSet]]
- [[22-概念/11-交叉分析/StatefulSet-×-Ingress|StatefulSet × Ingress]]
- [[22-概念/11-交叉分析/StatefulSet-×-NetworkPolicy|StatefulSet × NetworkPolicy]]
- [[22-概念/11-交叉分析/StatefulSet-×-PVC|StatefulSet × PVC]]
- [[22-概念/11-交叉分析/StatefulSet-×-RBAC|StatefulSet × RBAC]]
- [[26-技能/01-集群运维/migration/06-stateful-services-migration|06 - 有状态服务迁移 [migration]]]
- [[26-技能/02-控制面/scheduler/培训/learn-15-scheduling-basics|第15课：调度与亲和性]]
- [[26-技能/04-工作负载/hpa-vpa/培训/learn-09-hpa-basics|第九课：HPA - 自动伸缩]]
- [[26-技能/04-工作负载/job-cronjob/培训/learn-11-job-cronjob|第九课：Job 和 CronJob - 任务调度]]
- [[26-技能/04-工作负载/pod/资源与自动扩缩/字典-horizontal-pod-autoscaling|Horizontal Pod Autoscaling]]
- [[26-技能/04-工作负载/pod/配置与字典/pod-hostname|Pod Hostname]]
- [[26-技能/04-工作负载/statefulset/培训/learn-14-statefulset-basics|第14课：StatefulSet - 有状态应用管理]]
- [[26-技能/05-网络/service/培训/kubernetes-service-presentation|Kubernetes Service 全栈进阶培训 (从入门到专家) [topic-presentations]]]
