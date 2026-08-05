---
title: PostgreSQL StatefulSet + Patroni 高可用生产部署
description: 面向阿里云/专有云 K8s 的 PostgreSQL 高可用方案，使用 StatefulSet + Patroni + etcd 实现自动故障切换、流复制与备份恢复。
summary: 面向阿里云/专有云 K8s 的 PostgreSQL 高可用方案，使用 StatefulSet + Patroni + etcd 实现自动故障切换、流复制与备份恢复。
category: storage
tags:
- k8s
- statefulset
- postgresql
- patroni
- etcd
- ha
- alicloud
- apsara-stack
tier: supporting
created: '2026-06-29'
updated: '2026-06-29'
last_updated: 2026-06
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 数据库管理员
- 运维工程师
estimated_read_time: 25min
intent_queries:
- PostgreSQL StatefulSet Patroni 高可用
- K8s 上 PostgreSQL 自动故障切换
- 阿里云 K8s PostgreSQL 生产部署
trigger_keywords:
- PostgreSQL
- Patroni
- etcd
- 高可用
- 流复制
- 故障切换
prerequisites:
- kubectl-basics
- statefulset-basics
- postgresql-basics
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
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# PostgreSQL StatefulSet + Patroni 高可用生产部署

> **适用版本**: Kubernetes v1.28 - v1.32 | **最后更新**: 2026-06
> **文档定位**: 面向阿里云/专有云 K8s 环境，讲解 PostgreSQL 基于 StatefulSet + Patroni + etcd 的高可用生产部署、备份与故障切换。

## 目录

1. [架构概述](#架构概述)
2. [etcd 集群准备](#etcd-集群准备)
3. [Patroni 配置](#patroni-配置)
4. [StatefulSet 部署](#statefulset-部署)
5. [读写分离与 Service 设计](#读写分离与-service-设计)
6. [备份与 WAL 归档](#备份与-wal-归档)
7. [故障切换演练](#故障切换演练)
8. [监控告警](#监控告警)
9. [常见问题与优化](#常见问题与优化)
10. [最佳实践检查清单](#最佳实践检查清单)

---

## 1. 架构概述

### 1.1 组件职责

PostgreSQL 在 Kubernetes 上实现高可用通常采用 Patroni 方案，这是一种经过大规模生产验证的架构。其核心组件及职责如下表所示：

| 组件 | 职责 | 推荐部署方式 |
|:---|:---|:---|
| PostgreSQL | 数据存储与查询 | StatefulSet，每 Pod 一个实例 |
| Patroni | 集群管理、leader 选举、故障切换 | 与 PostgreSQL 同容器或 Sidecar |
| etcd | 分布式配置与 leader 锁 | 独立 etcd 集群，避免与 K8s etcd 混用 |
| pgBackRest / WAL-G | 物理备份与归档 | CronJob 或 Sidecar |
| Service | 读写分离入口 | ClusterIP + selector |

### 1.2 部署模式对比

在选择 PostgreSQL 高可用方案时，需要综合考虑数据一致性、自动切换能力、运维复杂度和成本。

| 模式 | 高可用 | 自动切换 | 运维复杂度 | 适用场景 |
|:---|:---:|:---:|:---:|:---|
| 单实例 | 否 | 否 | 低 | 开发测试 |
| Patroni + etcd | 是 | 是 | 中 | 生产推荐 |
| PostgreSQL 原生流复制 | 是 | 否 | 中 | 需配合外部切换工具 |
| 阿里云 PolarDB | 是 | 是 | 低 | 核心生产优先 |

Patroni 通过 etcd 存储集群元数据和 leader 锁，当 leader 故障时，Patroni 会自动在剩余节点中选举新 leader，并更新相关标签，从而实现应用无感知的故障切换。对于阿里云/专有云环境，核心业务仍然推荐优先使用阿里云 PolarDB，以获得更好的托管体验和自动运维能力。

### 1.3 为什么需要独立 etcd

虽然 Kubernetes 本身也使用 etcd，但将 Patroni 的 etcd 与 K8s 控制平面的 etcd 分离非常重要。原因包括：
- 避免数据库集群的负载影响 K8s 控制平面稳定性。
- 便于独立备份、恢复和升级 etcd。
- 在 K8s 控制平面异常时，数据库集群仍能维持高可用决策。

---

## 2. etcd 集群准备

Patroni 依赖 etcd 存储 leader 锁。强烈建议部署独立的 etcd 集群，避免与 Kubernetes 控制平面 etcd 混用，防止数据库集群状态影响 K8s 控制平面稳定性。

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 创建 etcd 命名空间
kubectl create namespace postgres-etcd

# 部署三节点 etcd（生产必须奇数节点）
kubectl apply -f etcd-statefulset.yaml
```
```yaml
apiVersion: apps/v1
kind: StatefulSet
metadata:
  name: etcd
  namespace: postgres-etcd
spec:
  serviceName: etcd
  replicas: 3
  selector:
    matchLabels:
      app: etcd
  template:
    metadata:
      labels:
        app: etcd
    spec:
      containers:
        - name: etcd
          image: quay.io/coreos/etcd:v3.5.15
          command:
            - /usr/local/bin/etcd
          args:
            - --name=$(POD_NAME)
            - --data-dir=/var/lib/etcd
            - --initial-advertise-peer-urls=http://$(POD_NAME).etcd.postgres-etcd.svc.cluster.local:2380
            - --listen-peer-urls=http://0.0.0.0:2380
            - --advertise-client-urls=http://$(POD_NAME).etcd.postgres-etcd.svc.cluster.local:2379
            - --listen-client-urls=http://0.0.0.0:2379
            - --initial-cluster=etcd-0=http://etcd-0.etcd.postgres-etcd.svc.cluster.local:2380,etcd-1=http://etcd-1.etcd.postgres-etcd.svc.cluster.local:2380,etcd-2=http://etcd-2.etcd.postgres-etcd.svc.cluster.local:2380
            - --initial-cluster-token=postgres-etcd-cluster
            - --initial-cluster-state=new
          env:
            - name: POD_NAME
              valueFrom:
                fieldRef:
                  fieldPath: metadata.name
          volumeMounts:
            - name: data
              mountPath: /var/lib/etcd
  volumeClaimTemplates:
    - metadata:
        name: data
      spec:
        accessModes: ["ReadWriteOnce"]
        storageClassName: alicloud-disk-essd
        resources:
          requests:
            storage: 20Gi
```

etcd 集群的节点数必须为奇数，通常生产环境使用 3 节点或 5 节点。3 节点集群可以容忍 1 个节点故障，5 节点集群可以容忍 2 个节点故障。部署完成后，需要验证集群健康状态：

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
kubectl exec -it etcd-0 -n postgres-etcd -- etcdctl endpoint health --cluster
```
---

## 3. Patroni 配置

### 3.1 ConfigMap 配置

Patroni 的配置文件 `patroni.yml` 需要包含 etcd 地址、PostgreSQL 监听地址、认证信息和复制参数。以下是一个生产可用的配置模板：

```yaml
apiVersion: v1
kind: ConfigMap
metadata:
  name: patroni-config
  namespace: production
data:
  patroni.yml: |
    scope: postgres-ha
    namespace: /service/
    name: 
    restapi:
      listen: 0.0.0.0:8008
      connect_address: 
    etcd3:
      hosts: etcd-0.etcd.postgres-etcd.svc.cluster.local:2379,etcd-1.etcd.postgres-etcd.svc.cluster.local:2379,etcd-2.etcd.postgres-etcd.svc.cluster.local:2379
    bootstrap:
      dcs:
        ttl: 30
        loop_wait: 10
        retry_timeout: 10
        maximum_lag_on_failover: 1048576
        postgresql:
          use_pg_rewind: true
          use_slots: true
          parameters:
            wal_level: replica
            hot_standby: "on"
            max_connections: 500
            max_wal_senders: 10
            max_replication_slots: 10
            wal_keep_size: 1GB
      initdb:
        - encoding: UTF8
        - data-checksums
      pg_hba:
        - host replication replicator 0.0.0.0/0 md5
        - host all all 0.0.0.0/0 md5
    postgresql:
      listen: 0.0.0.0:5432
      connect_address: 
      data_dir: /var/lib/postgresql/data
      pgpass: /tmp/pgpass
      authentication:
        replication:
          username: replicator
          password: $REPL_PASSWORD
        superuser:
          username: postgres
          password: $POSTGRES_PASSWORD
      parameters:
        shared_buffers: 2GB
        effective_cache_size: 6GB
        maintenance_work_mem: 512MB
        checkpoint_completion_target: 0.9
        wal_buffers: 16MB
        default_statistics_target: 100
        random_page_cost: 1.1
        effective_io_concurrency: 200
        work_mem: 10485kB
        min_wal_size: 1GB
        max_wal_size: 4GB
```

### 3.2 关键参数说明

| 参数 | 说明 |
|:---|:---|
| ttl | Patroni 在 etcd 中 key 的生存时间，影响故障检测速度 |
| loop_wait | Patroni 主循环间隔 |
| retry_timeout | 操作重试超时时间 |
| use_pg_rewind | 使用 pg_rewind 快速重新同步 |
| maximum_lag_on_failover | 允许参与故障切换的最大延迟 |

### 3.3 动态注入 Pod 名称与地址

由于每个 Pod 需要不同的 `name` 和 `connect_address`，建议在启动时通过 init 容器或 entrypoint 脚本动态注入。

```yaml
initContainers:
  - name: init-patroni
    image: busybox:1.36
    command:
      - sh
      - -c
      - |
        POD_NAME=${HOSTNAME}
        POD_IP=$(hostname -i)
        sed -e "s/name: .*/name: ${POD_NAME}/" \
            -e "s/connect_address: .*/connect_address: ${POD_IP}:5432/" \
            -e "s#connect_address: .*#connect_address: ${POD_IP}:8008#" \
            /etc/patroni/patroni.yml > /etc/patroni-runtime/patroni.yml
    volumeMounts:
      - name: patroni-config
        mountPath: /etc/patroni
      - name: patroni-runtime
        mountPath: /etc/patroni-runtime
```

---

## 4. StatefulSet 部署

```yaml
apiVersion: apps/v1
kind: StatefulSet
metadata:
  name: postgres-ha
  namespace: production
spec:
  serviceName: postgres-ha
  replicas: 3
  selector:
    matchLabels:
      app: postgres-ha
  template:
    metadata:
      labels:
        app: postgres-ha
    spec:
      affinity:
        podAntiAffinity:
          preferredDuringSchedulingIgnoredDuringExecution:
            - weight: 100
              podAffinityTerm:
                labelSelector:
                  matchExpressions:
                    - key: app
                      operator: In
                      values:
                        - postgres-ha
                topologyKey: topology.kubernetes.io/zone
      containers:
        - name: patroni
          image: registry.cn-hangzhou.aliyuncs.com/acs/patroni:3.2.2
          env:
            - name: PATRONI_KUBERNETES_POD_IP
              valueFrom:
                fieldRef:
                  fieldPath: status.podIP
            - name: POSTGRES_PASSWORD
              valueFrom:
                secretKeyRef:
                  name: postgres-secret
                  key: postgres-password
            - name: REPL_PASSWORD
              valueFrom:
                secretKeyRef:
                  name: postgres-secret
                  key: replication-password
          ports:
            - containerPort: 5432
              name: postgres
            - containerPort: 8008
              name: patroni
          volumeMounts:
            - name: data
              mountPath: /var/lib/postgresql/data
            - name: patroni-runtime
              mountPath: /etc/patroni
          readinessProbe:
            httpGet:
              path: /health
              port: 8008
            initialDelaySeconds: 10
            periodSeconds: 5
          livenessProbe:
            httpGet:
              path: /health
              port: 8008
            initialDelaySeconds: 30
            periodSeconds: 10
      volumes:
        - name: patroni-config
          configMap:
            name: patroni-config
        - name: patroni-runtime
          emptyDir: {}
  volumeClaimTemplates:
    - metadata:
        name: data
      spec:
        accessModes: ["ReadWriteOnce"]
        storageClassName: alicloud-disk-essd
        resources:
          requests:
            storage: 500Gi
```

---

## 5. 读写分离与 Service 设计

Patroni 会自动在 leader Pod 上打上 `role=master` 标签，在 replica Pod 上打上 `role=replica` 标签。利用这一机制，可以创建两个 Service 分别指向读写节点和只读节点。

```yaml
# 写服务：只指向 leader
apiVersion: v1
kind: Service
metadata:
  name: postgres-primary
  namespace: production
  labels:
    app: postgres-ha
spec:
  selector:
    app: postgres-ha
    role: master
  ports:
    - port: 5432
      targetPort: 5432
      name: postgres
---
# 读服务：指向所有 replica
apiVersion: v1
kind: Service
metadata:
  name: postgres-replica
  namespace: production
  labels:
    app: postgres-ha
spec:
  selector:
    app: postgres-ha
    role: replica
  ports:
    - port: 5432
      targetPort: 5432
      name: postgres
```

---

## 6. 备份与 WAL 归档

### 6.1 WAL-G 归档配置

WAL 归档是实现按时间点恢复的基础。建议在 Patroni 配置中开启归档，并将 WAL 文件推送到对象存储。

```yaml
# 在 patroni.yml 的 postgresql.parameters 中增加
archive_mode: "on"
archive_command: 'envdir /etc/wal-g/env /usr/local/bin/wal-g wal-push %p'
archive_timeout: 60s
```

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 配置 WAL-G 环境变量
kubectl create secret generic wal-g-env -n production \
  --from-literal=AWS_ACCESS_KEY_ID=xxx \
  --from-literal=AWS_SECRET_ACCESS_KEY=xxx \
  --from-literal=WALE_S3_ENDPOINT=https+path://oss-cn-hangzhou-internal.aliyuncs.com \
  --from-literal=AWS_S3_BUCKET=pg-backup-bucket
```
### 6.2 每日全量备份 CronJob

```yaml
apiVersion: batch/v1
kind: CronJob
metadata:
  name: postgres-full-backup
  namespace: production
spec:
  schedule: "0 3 * * *"
  concurrencyPolicy: Forbid
  jobTemplate:
    spec:
      template:
        spec:
          containers:
            - name: wal-g
              image: registry.cn-hangzhou.aliyuncs.com/acs/wal-g:latest
              command:
                - /bin/sh
                - -c
                - |
                  # 找到当前 leader
                  LEADER=$(curl -s http://postgres-primary.production.svc.cluster.local:8008/cluster | jq -r '.leader')
                  kubectl exec -it ${LEADER} -n production -- pg_basebackup -D /tmp/basebackup -Ft -z -P
                  ossutil cp /tmp/basebackup.tar.gz oss://pg-backup-bucket/full/
              volumeMounts:
                - name: wal-g-env
                  mountPath: /etc/wal-g/env
          restartPolicy: OnFailure
          volumes:
            - name: wal-g-env
              secret:
                secretName: wal-g-env
```

---

## 7. 故障切换演练

### 7.1 查看当前集群状态

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 查看 Patroni 集群成员与 leader
kubectl exec -it postgres-ha-0 -n production -- patronictl list
```
### 7.2 模拟主库故障

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl delete`：删除资源（可由声明式清单重建）

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 删除 leader Pod，观察 Patroni 是否自动切换
kubectl delete pod -n production -l app=postgres-ha,role=master
```
### 7.3 验证切换结果

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 等待 StatefulSet 重新调度并加入集群
kubectl exec -it postgres-ha-0 -n production -- patronictl list

# 验证新 leader 可写
kubectl exec -it postgres-ha-1 -n production -- psql -U postgres -c "CREATE TABLE switchover_test(id int); DROP TABLE switchover_test;"
```
---

## 8. 监控告警

```yaml
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: postgres-alerts
  namespace: monitoring
spec:
  groups:
    - name: postgres.rules
      rules:
        - alert: PostgresLeaderMissing
          expr: |
            patroni_postgres_running{role="master"} == 0
          for: 1m
          labels:
            severity: critical
          annotations:
            summary: "PostgreSQL 集群无 leader"
        - alert: PostgresReplicationLagHigh
          expr: |
            patroni_postgresql_replication_lag_bytes > 104857600
          for: 5m
          labels:
            severity: warning
          annotations:
            summary: "PostgreSQL 复制延迟超过 100MB"
        - alert: PostgresBackupStale
          expr: |
            time() - postgres_backup_last_success > 90000
          for: 5m
          labels:
            severity: warning
          annotations:
            summary: "PostgreSQL 备份超过 25 小时未成功"
```

---

## 9. 常见问题与优化

### 9.1 Patroni 无法连接 etcd

可能原因包括 etcd 地址错误、网络策略限制或 etcd 证书问题。排查方法：

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 检查 etcd 健康状态
kubectl exec -it etcd-0 -n postgres-etcd -- etcdctl endpoint health --cluster

# 检查 Patroni 日志
kubectl logs postgres-ha-0 -n production
```
### 9.2 复制延迟过高

常见原因包括网络带宽不足、从库 IO 瓶颈或大事务。优化建议：
- 使用更高性能的云盘类型，如 ESSD PL3。
- 调整 `max_wal_size` 和 `checkpoint_completion_target`。
- 避免单个大事务，拆分为多个小事务。

### 9.3 脑裂问题

当网络分区发生时，可能出现多个节点同时认为自己是 leader。Patroni 通过 etcd 的 TTL 和 leader 锁机制防止脑裂。建议：
- 确保 etcd 集群稳定，节点数为奇数。
- 合理设置 `ttl`、`loop_wait` 和 `retry_timeout`。
- 配置 watchdog，在极端情况下触发节点重启。

---

## 10. 阿里云/专有云生产实践

### 10.1 存储选择建议

在阿里云/专有云环境中部署 PostgreSQL 时，存储性能直接影响数据库响应速度和稳定性。建议为 PostgreSQL 数据盘选择 ESSD 云盘，性能等级根据业务负载选择 PL1、PL2 或 PL3。对于写入密集型应用，如高频交易系统或日志分析平台，应优先选择 PL3 以获得更高的 IOPS 和更低的延迟。如果业务对成本敏感且读写压力适中，PL1 或 PL2 也可以满足需求。

### 10.2 网络与可用区规划

Patroni + etcd 方案对网络稳定性要求较高。建议将 PostgreSQL 集群的 Pod 分布在不同的可用区，以提升容灾能力。同时，etcd 集群节点也应跨可用区部署，避免单可用区故障导致整个数据库高可用机制失效。在专有云环境中，需要提前规划好 VPC、子网和安全组，确保 Pod 之间、Pod 与 etcd 之间的通信畅通。

### 10.3 备份与恢复策略

阿里云/专有云环境通常提供 OSS 对象存储服务，PostgreSQL 的 WAL 归档和全量备份应优先存储到 OSS。建议配置生命周期策略，对超过 30 天的备份自动转存到低频访问存储，超过 180 天的备份自动删除或归档到更冷的存储介质。此外，每季度至少进行一次恢复演练，验证备份文件的完整性和恢复流程的可操作性。

---

## 11. 最佳实践检查清单

| 检查项 | 要求 | 验证方式 |
|:---|:---|:---|
| etcd 独立部署 | 与 K8s etcd 隔离 | `kubectl get pods -n postgres-etcd` |
| 奇数副本 | PostgreSQL 3 节点 | `kubectl get sts -n production postgres-ha` |
| 跨可用区部署 | Pod 反亲和性 zone | `kubectl get pod -o wide -n production` |
| WAL 归档开启 | archive_mode=on | `SHOW archive_mode;` |
| 每日全量备份 | CronJob 成功 | `kubectl get cj -n production` |
| 自动故障切换 | 演练通过 | 演练报告 |
| 读写分离 Service | primary/replica | `kubectl get svc -n production` |
| 监控告警覆盖 | leader、延迟、备份 | PrometheusRule |

---

## Related

- [[32-发布/package/2026-07-02_18-29/corpus/supporting/domain-04-storage-data/05-stateful-app-storage/01-stateful-app-storage-patterns|有状态应用 Kubernetes 存储模式]]
- [[32-发布/package/2026-07-02_18-29/corpus/supporting/domain-04-storage-data/04-distributed-storage/01-velero-backup-recovery|Velero 阿里云专有云备份恢复实战]]

## See Also

- [[32-发布/package/2026-07-02_18-29/corpus/core/domain-09-reliability-engineering/01-backup-recovery/01-enterprise-backup-strategy|企业级备份策略]]
- [[domain-10-troubleshooting-diagnostics/核心排障/02-control-plane-etcd-troubleshooting.md|etcd 故障诊断]]

```

<!-- risk-assessed -->
