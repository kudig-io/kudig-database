---
title: Database & Middleware 生产就绪运维指南
description: 面向 SRE 的数据库与中间件生产就绪检查、风险缓解、日常运维和故障排查综合指南
summary: 数据库与中间件生产就绪运维指南
category: database-middleware
tags:
- production
- best-practices
- database
- middleware
- operations
- sre
- readiness
tier: core
created: '2026-07-01'
last_updated: '2026-07'
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 平台工程师
- DBA
estimated_read_time: 20min
intent_queries:
- Database & Middleware 生产就绪运维指南是什么
- 如何按生产环境要求运维数据库与中间件
trigger_keywords:
- 生产就绪
- 运维指南
- database
- middleware
- 数据库
- 中间件
prerequisites:
- kubectl-basics
- helm-basics
- prometheus-basics
- etcd-basics
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
- '1.33'
authors:
- name: KUDIG Team
  role: contributor
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。


# Database & Middleware 生产就绪运维指南

> **适用场景**: 在 Kubernetes 上运行 MySQL、PostgreSQL、Redis、Kafka、MongoDB、Pulsar、Prometheus TSDB 等数据库与中间件的生产集群。
> **目标读者**: SRE、平台工程师、DBA 及需要为数据面服务做生产就绪评审的架构师。
> **最后更新**: 2026-07-01

---

## 1. 生产环境检查清单

在将任一数据库或中间件实例标记为 `Production Ready` 之前，建议逐项核对以下检查点。本清单基于 [[数据库/04-database-middleware-kubernetes.md|数据库中间件 Kubernetes 企业级实践]] 与 [[Operator管理/01-database-operator-patterns.md|数据库 Operator 设计模式]] 中的生产要求提炼而成。

建议将本清单嵌入平台工程团队的 Service Catalog 或 GitOps 模板中，作为数据库实例上架（onboarding）的强制门禁。每次重大变更（版本升级、存储扩容、网络策略调整）后，应重新执行相关检查项并留存审计记录。对于核心生产实例，建议每季度开展一次全面复评。

| 编号 | 检查项 | 验收标准 | 常用命令/工具 |
|---|---|---|---|
| 1 | **Operator/Chart 版本冻结** | 使用稳定版 Operator 或 Helm Chart，并校验镜像签名与 SBOM | `helm show values <chart>`、`cosign verify` |
| 2 | **持久化存储策略** | StorageClass 已指定，PV 回收策略符合业务要求，跨可用区分布 | `kubectl get sc`、`kubectl get pv -o yaml` |
| 3 | **高可用拓扑** | 主从/集群/分片实例跨节点、跨可用区部署，具备反亲和性 | `kubectl get pods -o wide -n <ns>` |
| 4 | **备份与可恢复性** | 存在定时备份 CronJob，且最近 30 天内执行过成功恢复演练 | `kubectl get cronjob`、`velero restore describe` |
| 5 | **网络隔离** | 默认拒绝入站流量，仅允许白名单 Namespace/标签访问数据库端口 | `kubectl get networkpolicy -n <ns>` |
| 6 | **资源 QoS** | 已设置合理的 requests/limits，数据库 Pod 优先为 Guaranteed 或 Burstable | `kubectl top pod`、`kubectl describe pod` |
| 7 | **监控告警覆盖** | 暴露关键指标（连接数、复制延迟、存储使用率、Leader 状态），PrometheusRule 已生效 | `kubectl get servicemonitor`、`kubectl get prometheusrule` |
| 8 | **安全基线** | TLS 加密通信、凭据由 External Secrets/Vault 注入、审计日志开启 | `kubectl get secret`、`kubectl get cert` |
| 9 | **优雅中断** | 已配置 PodDisruptionBudget，滚动升级时保证最小可用副本数 | `kubectl get pdb -n <ns>` |
| 10 | **升级与回滚方案** | 明确支持小版本滚动升级，具备镜像回滚与数据快照回退能力 | `helm history <release>`、`kubectl rollout undo` |
| 11 | **灾难恢复计划** | 跨区域/跨集群 DR 方案已文档化，RTO/RPO 已定义并通过演练 | DR Runbook、区域故障演练记录 |
| 12 | **容量规划基线** | 存储、连接数、CPU/内存 已建立增长趋势图和扩容阈值 | VPA/Metrics Dashboard |

---

## 2. 关键风险与缓解措施

以下风险基于过去一年的生产事件复盘与 数据库故障排查手册（待补充） 中的高频场景总结。每个风险均给出可落地的配置或命令示例，便于直接纳入运行手册。

### 2.1 数据丢失（未备份或备份不可恢复）

- **风险**: 误删 Namespace、存储故障、应用 Bug 导致数据被覆盖，且没有可用的恢复点。
- **缓解**:
  - 使用 CSI 快照或 Velero 进行定时备份，保留至少 7 个每日快照和 3 个月异地副本。
  - 每月执行一次恢复演练，验证备份文件完整性。
  - 对关键数据库启用连续归档（WAL、oplog、AOF）以支持 PITR。

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 创建 PVC 快照示例
kubectl apply -f - <<EOF
apiVersion: snapshot.storage.k8s.io/v1
kind: VolumeSnapshot
metadata:
  name: postgres-prod-snap-$(date +%Y%m%d)
  namespace: db-prod
spec:
  volumeSnapshotClassName: csi-snapclass
  source:
    persistentVolumeClaimName: postgres-data-0
EOF
```
### 2.2 脑裂与复制延迟

- **风险**: 网络分区导致双主写入，或从库滞后引发读取脏数据。
- **缓解**:
  - 部署具备自动故障转移与仲裁机制的 Operator（如 MySQL InnoDB Cluster、CloudNativePG、Redis Sentinel）。
  - 配置复制延迟告警阈值（默认 ≥5s 告警，≥30s 自动切换只读）。
  - 使用 `PodDisruptionBudget` 与 `terminationGracePeriodSeconds` 保证切换窗口内数据一致性。

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 查看复制延迟示例（MySQL）
kubectl exec -n db-prod mysql-1 -- mysql -e "SHOW SLAVE STATUS\G" | grep Seconds_Behind_Master
```
### 2.3 PVC 容量耗尽

- **风险**: 数据文件持续增长，触发 DiskPressure，导致 Pod 被驱逐或数据库只读。
- **缓解**:
  - 启用 `AllowVolumeExpansion: true` 的 StorageClass，配置容量告警（≥75% 警告，≥85% 紧急）。
  - 对可收缩数据配置 TTL 与归档策略；时序数据库配置降采样与 retention。
  - 预先定义 PVC 扩容 SOP。

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 在线扩展 PVC
kubectl patch pvc postgres-data-0 -n db-prod -p '{"spec":{"resources":{"requests":{"storage":"200Gi"}}}}'
```
### 2.4 证书/TLS 过期

- **风险**: 数据库 mTLS 证书或 cert-manager 颁发的证书过期，导致客户端连接失败。
- **缓解**:
  - 使用 cert-manager 自动轮换，并设置证书过期前 30 天告警。
  - 对自签名证书建立备用根证书与 break-glass 轮换流程。
  - 在应用侧启用证书热加载，避免重启 Pod。

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 检查证书过期时间
kubectl get secret -n db-prod postgres-tls -o jsonpath='{.data.tls\.crt}' | base64 -d | openssl x509 -noout -dates
```
### 2.5 Operator 控制面单点故障

- **风险**: Operator 自身仅单副本运行，升级或节点故障时无法协调数据库集群。
- **缓解**:
  - Operator Deployment 设置至少 2 副本并配置反亲和性。
  - 限制 Operator RBAC 为最小权限，禁止 cluster-admin 绑定。
  - 升级 Operator 前先在非生产环境验证 CRD 变更。

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看 Operator 副本与事件
kubectl get deployment -n operators -l app.kubernetes.io/name=cloudnativepg
kubectl get events -n operators --field-selector reason=FailedBinding
```
---

## 3. 日常运维操作

### 3.1 健康巡检

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看所有数据库/中间件命名空间下的 Pod 状态
kubectl get pods -n db-prod -o wide

# 检查 PVC 使用率
kubectl top pvc -n db-prod

# 检查 Service 与 Endpoint 是否正常
kubectl get svc,ep -n db-prod

# 查看数据库 Pod 最近事件
kubectl get events -n db-prod --sort-by='.lastTimestamp' | tail -n 30
```
### 3.2 备份与恢复

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# Velero 备份
velero backup create db-prod-backup --include-namespaces db-prod --default-volumes-to-fs-backup --wait

# Velero 恢复演练（隔离命名空间）
velero restore create --from-backup db-prod-backup --restore-volumes --namespace-mappings db-prod:db-restore-test

# CloudNativePG 按需备份
kubectl cnpg backup cluster-example -n db-prod
```
### 3.3 滚动升级

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# Helm 升级前进行 diff 检查
helm diff upgrade mydb bitnami/postgresql -f values-prod.yaml -n db-prod

# 执行升级并记录 revision
helm upgrade mydb bitnami/postgresql -f values-prod.yaml -n db-prod --history-max 10

# 观察滚动进度
kubectl rollout status sts/mydb-postgresql -n db-prod
```
### 3.4 连接池与性能巡检

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 查看当前连接数（PostgreSQL）
kubectl exec -n db-prod pg-primary-0 -- psql -c "SELECT count(*), state FROM pg_stat_activity GROUP BY state;"

# 查看慢查询（MySQL）
kubectl exec -n db-prod mysql-0 -- mysql -e "SELECT * FROM performance_schema.events_statements_summary_by_digest ORDER BY SUM_TIMER_WAIT DESC LIMIT 10;"
```
### 3.5 凭据轮换

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 更新 Secret 后触发滚动重启
kubectl create secret generic db-credentials \
  --from-literal=password='$(openssl rand -base64 32)' \
  --dry-run=client -o yaml | kubectl apply -f - -n db-prod

kubectl rollout restart sts/mydb-postgresql -n db-prod
```
### 3.6 日志与审计巡检

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 实时查看数据库错误日志
kubectl logs -f -n db-prod -l app=postgresql --tail=500 | grep -E "ERROR|FATAL|WARN"

# 按时间窗口检索审计事件
kubectl get events -n db-prod --sort-by='.lastTimestamp' | awk '/Warning|Error/{print}'

# 使用 stern 聚合多副本日志
stern -n db-prod "postgresql-" --since 1h | grep -i "slow\|deadlock\|lock"
```
### 3.7 节点维护前的保护性操作

在对数据库 Pod 所在节点进行维护（内核升级、硬件更换）前，务必按以下顺序执行，避免误触发 Leader 切换或数据不一致：

1. 检查集群健康状态与复制延迟。
2. 根据 PDB 确认可安全驱逐的副本数。
3. 对目标节点执行 `cordon` + `drain`，观察数据库集群是否完成自动切换。
4. 维护完成后 `uncordon` 节点，观察副本重新加入集群。

> **🔴 高风险操作警告**
>
> 下方命令属于不可逆或高影响操作，执行前请确认：
> - 已备份关键数据与配置
> - 处于批准的变更窗口期
> - 已获得相关责任人授权
> - 已准备回滚或恢复方案
> - 目标集群、Namespace、节点/资源名称正确无误

``` bash
# 🔴 高风险：可能造成数据丢失或服务中断，执行前需备份、变更审批与回滚方案
kubectl drain <node> --ignore-daemonsets --delete-emptydir-data --pod-selector='app=postgresql' --dry-run=client
```
---

## 4. 故障排查速查

| 现象 | 可能原因 | 确认命令 | 修复措施 |
|---|---|---|---|
| Pod `CrashLoopBackOff` | 配置错误、初始化失败、资源不足、Liveness 探针误杀 | `kubectl logs -p <pod> -n db-prod`、`kubectl describe pod <pod>` | 修复配置、调整探针阈值、增加资源 |
| PVC `Pending` | StorageClass 不存在、Provisioner 故障、配额不足 | `kubectl describe pvc <pvc> -n db-prod` | 检查 SC/Provisioner、调整 ResourceQuota |
| 复制延迟高 | 网络抖动、从库 IO 饱和、大事务、binlog/WAL 积压 | `kubectl exec <pod> -- 查看复制状态`、Prometheus `mysql_slave_lag_seconds` | 优化查询、扩容从库 IO、调整同步模式 |
| 连接数打满 | 连接池泄漏、应用未复用连接、max_connections 过低 | `kubectl exec <pod> -- 查看活跃连接`、应用监控 | 修复应用连接池、提升 max_connections、引入 ProxySQL/PgBouncer |
| Pod `OOMKilled` | 内存请求不足、查询导致临时表溢出、缓存过大 | `kubectl describe pod <pod>`、数据库内存视图 | 增加 limits、优化 SQL、调整 shared_buffers/innodb_buffer_pool_size |
| 节点 NotReady 导致主库漂移失败 | PDB 限制、网络分区、Operator 未运行 | `kubectl get pdb -n db-prod`、`kubectl get events` | 确认 Operator 健康、必要时手动触发 failover |
| 证书过期连接失败 | cert-manager 未续期、Secret 未挂载 | `kubectl get certificate -n db-prod`、`openssl x509 -in ... -noout -dates` | 手动续期证书、重启 Pod 重载 Secret |
| 写入延迟突增 | 磁盘 IO 饱和、WAL/事务日志刷盘阻塞、并发锁竞争 | 节点 `iostat -x 1`、数据库 `pg_stat_activity`/`show engine innodb status` | 提升磁盘类型（HDD→SSD/NVMe）、优化索引、拆分热点表 |
| 缓存命中率下降 | 内存不足导致页换出、数据集增长超过缓存池 | 数据库缓存命中率指标、`kubectl top pod` | 增加缓存池内存、引入 Redis 二级缓存、归档冷数据 |
| Operator 无法协调 CR | CRD 版本不匹配、Operator 权限不足、Webhook 故障 | `kubectl describe <cr> -n db-prod`、`kubectl logs -n operators deployment/<operator>` | 回滚 Operator、修复 RBAC、检查 ValidatingWebhook |
| 消息队列消费堆积 | 消费者不足、分区倾斜、消费端 GC 停顿 | Kafka `consumer-lag`、Pulsar `msgBacklog` | 扩容消费者、检查分区分配、优化消费端性能 |

---

## 5. 与其他域的协作边界

数据库与中间件是 Kubernetes 平台的"数据面"，其稳定性高度依赖周边域的能力。明确边界可避免职责不清，也能在故障发生时快速定位责任团队。建议在各域的值班手册中相互引用，并建立跨域的定期联合演练机制。

- **[[存储/README.md|存储]]（存储与数据）**: 负责 StorageClass、CSI 驱动、PVC 扩容、快照与备份存储。数据库层应复用其存储治理与容量规划结论，不自行维护底层存储 SOP。
- **[[安全/README.md|安全]]（安全合规）**: 负责 NetworkPolicy 默认拒绝、Secret/凭据生命周期、Pod Security Standards、TLS/mTLS 策略。数据库安全加固的具体实践见 数据库安全加固（待补充）。
- **[[可观测性/README.md|可观测性]]（可观测性）**: 负责 Prometheus、Grafana、Loki、Jaeger 的统一采集与告警路由。数据库层需提供Exporter/ServiceMonitor，并定义自身的 SLO/SLI，详情参考 数据库 SLO 与容量规划（待补充）。
- **[[平台工程/README.md|平台工程]]（平台工程）**: 负责 Operator 注册、多租户命名空间模板、GitOps 交付、资源配额。数据库实例通过平台目录发布，避免业务团队自行部署。
- **[[可靠性/README.md|可靠性]]（可靠性工程）**: 负责跨域灾难恢复、PDB、混沌演练、RTO/RPO 定义。数据库的跨集群 DR 方案见 数据库多集群灾备手册（待补充）。
- **[[生产运维/README.md|生产运维]]（生产运维）**: 负责事件响应、值班手册、变更管理。数据库 P0/P1 故障应接入统一事件流程。
- **[[AI基础设施/README.md|AI基础设施]]（AI/ML 基础设施）**: 负责 AI 数据管道、特征存储、向量数据库集成。当数据库层为 AI 工作负载提供数据服务时，需协同评估高并发读取与向量索引性能。

---

## 6. 推荐阅读

### 本域核心资料

- [[README.md|Database & Middleware 目录]] — 域内结构与索引
- [[01-database-on-kubernetes-guide.md|Kubernetes 数据库部署指南]] — 通用部署模式
- [[数据库/01-mysql-enterprise-database.md|MySQL 企业级数据库运维管理]]
- [[数据库/02-postgresql-enterprise-database.md|PostgreSQL 企业级数据库运维管理]]
- [[数据库/04-database-middleware-kubernetes.md|数据库中间件 Kubernetes 企业级实践]]
- [[数据库/06-redis-enterprise-cache.md|Redis 企业级缓存]]
- [[数据库/08-kafka-kubernetes-strimzi.md|Kafka Kubernetes Strimzi 实践]]
- [[数据库/99-cloudnativepg-enterprise-guide.md|CloudNativePG 企业指南]]
- [[消息队列/03-message-queue-comparison.md|消息队列选型]]
- [[时序数据库/01-prometheus-tsdb-deep-dive.md|Prometheus TSDB 深度解析]]
- [[Operator管理/01-database-operator-patterns.md|数据库 Operator 设计模式]]
- [[Operator管理/03-operator-lifecycle-management.md|Operator 生命周期管理]]
- [[数据流/01-cdc-change-data-capture.md|CDC 变更数据捕获]]

### 计划补充的关键主题

以下内容已识别为当前域缺口，建议优先补充：

- 数据库安全加固（待补充）
- Kubernetes 上的 etcd（待补充）
- 数据库多集群灾备手册（待补充）
- 数据库 SLO 与容量规划（待补充）
- 数据库故障排查手册（待补充）
- 数据库迁移指南（待补充）
- 云托管数据库集成（待补充）
- Thanos/Cortex/Mimir 长期存储（待补充）

### 跨域参考

- [[存储/README.md|存储]] — 存储基础与 PVC 治理
- [[安全/README.md|安全]] — 安全基线与合规
- [[可观测性/README.md|可观测性]] — 监控告警与 SLO
- [[可靠性/README.md|可靠性]] — 灾难恢复与韧性设计
- [[生产运维/README.md|生产运维]] — 事件响应与值班手册

---

*本指南用于指导数据库与中间件在 Kubernetes 生产环境中的就绪评审与日常运维。实际执行前，请结合具体 Operator/Chart 版本与组织安全策略进行裁剪。*


<!-- risk-assessed -->
