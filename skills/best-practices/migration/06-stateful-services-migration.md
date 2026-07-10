---
title: 06 - 有状态服务迁移 [migration]
description: 'description: ''#<!-- chunk: 2.1 方案 A: 迁移到阿里云 RDS（推荐）'' -->## 2.1 方案
  A: 迁移到阿里云 RDS（推荐）'''
summary: 'description: ''#<!-- chunk: 2.1 方案 A: 迁移到阿里云 RDS（推荐）'' -->## 2.1 方案 A: 迁移到阿里云
  RDS（推荐）'''
category: general
tags:
- migration
- upgrade
- etcd
- docker
- redis
- mysql
- kafka
- elasticsearch
- statefulset
- operator
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 15min
intent_queries:
- 有状态服务迁移 是什么
- 如何 有状态服务迁移
- Kubernetes 11 production operations 最佳实践
trigger_keywords:
- 有状态服务迁移
- production
- operations
- best
- practices
prerequisites:
- kubectl-basics
- gpu-ml-basics
- etcd-basics
- kafka-basics
- redis-basics
- mysql-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




title: 06 - 有状态服务迁移
description: '#<!-- chunk: 2.1 方案 A: 迁移到阿里云 RDS（推荐）' -->## 2.1 方案 A: 迁移到阿里云 RDS（推荐）'
category: migration
tags:
- k8s
- migration
- modernization
- [[etcd|etcd]]
- docker
- redis
- mysql
- kafka
- elasticsearch
- [[StatefulSet|statefulset]]
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- 架构师
- SRE
- 运维工程师
estimated_read_time: 5min
intent_queries:
- 有状态服务迁移 是什么
- 如何 有状态服务迁移
trigger_keywords:
- 有状态服务迁移
- migration
authors:
- name: KUDIG Team
  role: contributor
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
---

# 06 - 有状态服务迁移

> **文档版本**: v1.0 | **适用场景**: 自建 K8s → 阿里云 ACK | **更新日期**: 2026-03 | **关键词**: MySQL, Redis, Elasticsearch, Kafka, StatefulSet, RDS, 数据一致性

---

<!-- chunk: 目录 -->## 目录

1. [有状态服务迁移策略](#1-有状态服务迁移策略)
2. [MySQL 迁移](#2-mysql-迁移)
3. [Redis 迁移](#3-redis-迁移)
4. [Elasticsearch 迁移](#4-elasticsearch-迁移)
5. [Kafka/RocketMQ 迁移](#5-kafkarocketmq-迁移)
6. [etcd 数据迁移](#6-etcd-数据迁移)
7. [StatefulSet 通用迁移](#7-statefulset-通用迁移)
8. [数据一致性校验](#8-数据一致性校验)

---

<!-- chunk: 1. 有状态服务迁移策略 -->## 1. 有状态服务迁移策略

## 1.1 策略选择

| 策略 | 说明 | 停机时间 | 复杂度 | 适用场景 |
|------|------|---------|--------|---------|
| **A: 迁移到云托管服务** | K8s 自建 → 阿里云 RDS/Redis/ES | 短暂（切换窗口） | 中 | **推荐生产环境** |
| **B: 保持 K8s StatefulSet** | 源集群 → ACK StatefulSet | 需停写同步 | 高 | 需 K8s 内运行 |
| **C: 双写过渡** | 源集群 + ACK 同时写入 | 零停机 | 最高 | 金融级要求 |

## 1.2 决策矩阵

```
自建集群中的数据库类型
    |
    +-- MySQL
    |   +-- 业务关键 → 阿里云 RDS MySQL（推荐策略 A）
    |   +-- 开发测试 → ACK StatefulSet（策略 B）
    |
    +-- Redis
    |   +-- 持久化数据 → 阿里云 Redis 版（策略 A）
    |   +-- 纯缓存 → ACK Deployment + 重建缓存
    |
    +-- Elasticsearch
    |   +-- > 500GB → 阿里云 ES 版（策略 A）
    |   +-- < 500GB → ACK StatefulSet（策略 B）
    |
    +-- Kafka / RocketMQ
        +-- 生产环境 → 阿里云消息队列（策略 A）
        +-- 仅内部 → ACK StatefulSet（策略 B）
```

---

<!-- chunk: 2. MySQL 迁移 -->## 2. MySQL 迁移

## 2.1 方案 A: 迁移到阿里云 RDS（推荐）

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# Step 1: 创建 RDS 实例
aliyun rds CreateDBInstance \
  --Engine MySQL \
  --EngineVersion "8.0" \
  --DBInstanceClass "mysql.n4.medium.2c" \
  --DBInstanceStorage 100 \
  --DBInstanceNetType Intranet \
  --VPCId $VPC_ID \
  --VSwitchId "<vsw-id>" \
  --PayType Postpaid \
  --SecurityIPList "10.0.0.0/8" \
  --DBInstanceDescription "migration-mysql"

# Step 2: 创建数据库和账号
RDS_ID="<rds-instance-id>"
aliyun rds CreateAccount --DBInstanceId $RDS_ID --AccountName admin --AccountPassword "<password>" --AccountType Super
aliyun rds CreateDatabase --DBInstanceId $RDS_ID --DBName production --CharacterSetName utf8mb4

# Step 3: 使用 DTS 进行数据迁移
# 阿里云 DTS（数据传输服务）支持增量同步
# 控制台: DTS → 数据迁移 → 创建迁移任务
# 源: 自建 MySQL（通过 VPN/CEN/公网访问）
# 目标: RDS MySQL

# 或使用 mysqldump 手动迁移（适合小数据量）
# 在源集群 MySQL Pod 中导出
kubectl --context=source-cluster exec -n production mysql-0 -- \
  mysqldump -u root -p"$MYSQL_ROOT_PASSWORD" --all-databases --single-transaction \
  --routines --triggers --events > full-backup.sql

# 导入到 RDS
mysql -h <rds-endpoint> -u admin -p"<password>" < full-backup.sql

# Step 4: 验证数据一致性
mysql -h <rds-endpoint> -u admin -p"<password>" -e "
  SELECT table_schema, 
         COUNT(*) as table_count,
         SUM(table_rows) as total_rows
  FROM information_schema.tables 
  WHERE table_schema NOT IN ('mysql','information_schema','performance_schema','sys')
  GROUP BY table_schema;
"

# Step 5: 应用连接串更新
# 更新 ACK 集群中的 ConfigMap/Secret
kubectl --context=ack-cluster create secret generic mysql-secret \
  -n production \
  --from-literal=host=<rds-endpoint> \
  --from-literal=port=3306 \
  --from-literal=username=admin \
  --from-literal=password="<password>" \
  --from-literal=database=production
```
## 2.2 方案 B: 迁移到 ACK StatefulSet

```yaml
# MySQL StatefulSet on ACK
apiVersion: apps/v1
kind: StatefulSet
metadata:
  name: mysql
  namespace: production
spec:
  serviceName: mysql
  replicas: 1
  selector:
    matchLabels:
      app: mysql
  template:
    metadata:
      labels:
        app: mysql
    spec:
      nodeSelector:
        node-role: stateful
      tolerations:
      - key: "workload-type"
        operator: "Equal"
        value: "stateful"
        effect: "NoSchedule"
      containers:
      - name: mysql
        image: mysql:8.0
        ports:
        - containerPort: 3306
        env:
        - name: MYSQL_ROOT_PASSWORD
          valueFrom:
            secretKeyRef:
              name: mysql-secret
              key: password
        volumeMounts:
        - name: data
          mountPath: /var/lib/mysql
        resources:
          requests:
            cpu: "2"
            memory: "4Gi"
          limits:
            cpu: "4"
            memory: "8Gi"
        livenessProbe:
          exec:
            command: ["mysqladmin", "ping", "-h", "localhost"]
          initialDelaySeconds: 30
          periodSeconds: 10
        readinessProbe:
          exec:
            command: ["mysql", "-h", "localhost", "-e", "SELECT 1"]
          initialDelaySeconds: 5
          periodSeconds: 5
  volumeClaimTemplates:
  - metadata:
      name: data
    spec:
      accessModes: ["ReadWriteOnce"]
      storageClassName: alicloud-disk-essd
      resources:
        requests:
          storage: 100Gi
```

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 数据迁移步骤:
# 1. 创建上述 StatefulSet
kubectl --context=ack-cluster apply -f mysql-statefulset.yaml

# 2. 等待 MySQL 就绪
kubectl --context=ack-cluster wait --for=condition=ready pod/mysql-0 -n production --timeout=300s

# 3. 从源集群导出数据
kubectl --context=source-cluster exec -n production mysql-0 -- \
  mysqldump -u root -p"$MYSQL_ROOT_PASSWORD" --all-databases --single-transaction > dump.sql

# 4. 导入到 ACK MySQL
kubectl --context=ack-cluster cp dump.sql production/mysql-0:/tmp/dump.sql
kubectl --context=ack-cluster exec -n production mysql-0 -- \
  mysql -u root -p"$MYSQL_ROOT_PASSWORD" < /tmp/dump.sql
```
---

<!-- chunk: 3. Redis 迁移 -->## 3. Redis 迁移

## 3.1 方案 A: 迁移到阿里云 Redis

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 创建阿里云 Redis 实例
aliyun r-kvstore CreateInstance \
  --InstanceClass "redis.master.small.default" \
  --InstanceName "migration-redis" \
  --Password "<password>" \
  --VpcId $VPC_ID \
  --VSwitchId "<vsw-id>" \
  --EngineVersion "7.0" \
  --ChargeType PostPaid

# 数据迁移方式:
# 方式 1: redis-shake (推荐大数据量)
# 下载: https://github.com/tair-opensource/RedisShake
docker run --rm -v $(pwd):/data \
  redisshake/redis-shake:latest \
  /data/redis-shake.toml

# redis-shake.toml 配置:
cat > redis-shake.toml <<EOF
[source]
type = "standalone"
address = "<source-redis-host>:6379"
password = "<source-password>"

[target]
type = "standalone"
address = "<aliyun-redis-host>:6379"
password = "<target-password>"

[advanced]
log_file = "redis-shake.log"
EOF

# 方式 2: RDB 文件迁移（适合停机窗口）
# 在源 Redis Pod 中执行 BGSAVE
kubectl --context=source-cluster exec -n production redis-0 -- redis-cli -a "$REDIS_PASSWORD" BGSAVE

# 复制 RDB 文件
kubectl --context=source-cluster cp production/redis-0:/data/dump.rdb ./dump.rdb

# 通过 DTS 或手动导入到阿里云 Redis
# 控制台: Redis → 备份与恢复 → 从 RDB 文件恢复
```
## 3.2 纯缓存场景

```bash
# 如果 Redis 仅作为缓存，不需要数据迁移
# 在 ACK 部署新 Redis 或使用阿里云 Redis
# 应用启动后会自动重建缓存

# 注意事项:
# 1. 迁移后短期缓存命中率会降低（冷启动）
# 2. 建议提前预热：在切流前用脚本预加载热点数据
# 3. 监控缓存命中率，确认恢复正常水平

# 缓存预热脚本示例
redis-cli -h <ack-redis-host> -a "<password>" --pipe < warmup-commands.txt
```

---

<!-- chunk: 4. Elasticsearch 迁移 -->## 4. Elasticsearch 迁移

## 4.1 使用 Snapshot/Restore 迁移到阿里云 ES

```bash
# Step 1: 在源 ES 注册 OSS Repository
curl -X PUT "http://<source-es>:9200/_snapshot/migration_repo" -H 'Content-Type: application/json' -d'{
  "type": "oss",
  "settings": {
    "endpoint": "http://oss-cn-hangzhou-internal.aliyuncs.com",
    "access_key_id": "<access-key>",
    "secret_access_key": "<secret-key>",
    "bucket": "es-migration-snapshot",
    "base_path": "migration"
  }
}'

# Step 2: 创建快照
curl -X PUT "http://<source-es>:9200/_snapshot/migration_repo/snapshot_1?wait_for_completion=true" -H 'Content-Type: application/json' -d'{
  "indices": "*",
  "ignore_unavailable": true,
  "include_global_state": false
}'

# Step 3: 在阿里云 ES 注册同一个 OSS Repository
curl -X PUT "http://<aliyun-es>:9200/_snapshot/migration_repo" -H 'Content-Type: application/json' -d'{
  "type": "oss",
  "settings": {
    "endpoint": "http://oss-cn-hangzhou-internal.aliyuncs.com",
    "access_key_id": "<access-key>",
    "secret_access_key": "<secret-key>",
    "bucket": "es-migration-snapshot",
    "base_path": "migration"
  }
}'

# Step 4: 恢复快照
curl -X POST "http://<aliyun-es>:9200/_snapshot/migration_repo/snapshot_1/_restore" -H 'Content-Type: application/json' -d'{
  "indices": "*",
  "ignore_unavailable": true,
  "include_global_state": false
}'

# Step 5: 验证
curl "http://<aliyun-es>:9200/_cat/indices?v&s=index"
```

## 4.2 使用 Reindex 在线迁移

```bash
# 适合小数据量或需要在线迁移的场景
# 在目标 ES 配置远程源
# elasticsearch.yml: reindex.remote.whitelist: "<source-es>:9200"

curl -X POST "http://<aliyun-es>:9200/_reindex?wait_for_completion=false" -H 'Content-Type: application/json' -d'{
  "source": {
    "remote": {
      "host": "http://<source-es>:9200"
    },
    "index": "my-index-*"
  },
  "dest": {
    "index": "my-index"
  }
}'

# 查看 reindex 进度
curl "http://<aliyun-es>:9200/_tasks?actions=*reindex&detailed"
```

---

<!-- chunk: 5. Kafka/RocketMQ 迁移 -->## 5. Kafka/RocketMQ 迁移

## 5.1 Kafka 迁移策略

```
方案 A: MirrorMaker 2 双向复制
  源 Kafka ──── MirrorMaker2 ────► 阿里云 Kafka
  优点: 零停机，实时同步
  缺点: 需要网络互通

方案 B: 消费者切换法
  1. 在 ACK 部署新 Kafka (或阿里云消息队列)
  2. 生产者先切到新 Kafka
  3. 消费者消费完源 Kafka 积压后切到新 Kafka
  优点: 简单，无数据丢失
  缺点: 需要短暂双写

方案 C: 直接迁移
  1. 停止生产者
  2. 等待消费者消费完所有消息
  3. 在 ACK 启动新 Kafka
  4. 切换生产者和消费者
  缺点: 需要停机窗口
```

## 5.2 MirrorMaker 2 配置

```yaml
# mm2.properties
clusters = source, target
source.bootstrap.servers = <source-kafka>:9092
target.bootstrap.servers = <ack-kafka>:9092

source->target.enabled = true
source->target.topics = .*

# 复制配置
replication.factor = 3
checkpoints.topic.replication.factor = 3
heartbeats.topic.replication.factor = 3
offset-syncs.topic.replication.factor = 3

# 消费者偏移同步
sync.group.offsets.enabled = true
sync.group.offsets.interval.seconds = 10
```

---

<!-- chunk: 6. etcd 数据迁移 -->## 6. etcd 数据迁移

> 注意: 此处指业务使用的 etcd（如 etcd 作为配置中心），非 K8s 控制面 etcd。

> ⚠️ **🔴 灾难性操作** — 含不可逆命令，执行前必须满足变更窗口+双人复核+事前备份+回滚方案
> - `etcdctl snapshot restore`：用快照覆盖 etcd 数据目录，集群状态强制回退

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 导出 etcd 数据
ETCDCTL_API=3 etcdctl --endpoints=<source-etcd>:2379 \
  --cert=/etc/etcd/pki/server.crt \
  --key=/etc/etcd/pki/server.key \
  --cacert=/etc/etcd/pki/ca.crt \
  snapshot save etcd-backup.db

# 传输到 ACK 环境
scp etcd-backup.db <ack-bastion>:/tmp/

# 在 ACK 恢复（如果继续使用 etcd）
ETCDCTL_API=3 etcdctl snapshot restore etcd-backup.db \
  --data-dir=/var/lib/etcd-restored \
  --name=etcd-0 \
  --initial-cluster="etcd-0=https://etcd-0:2380" \
  --initial-advertise-peer-urls="https://etcd-0:2380"
```
---

<!-- chunk: 7. StatefulSet 通用迁移 -->## 7. StatefulSet 通用迁移

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
#!/bin/bash
# migrate-statefulset.sh
# StatefulSet 通用迁移流程

SOURCE_CONTEXT="source-cluster"
ACK_CONTEXT="ack-cluster"
NS="production"
STS_NAME="$1"  # 传入 StatefulSet 名称

echo "=== 迁移 StatefulSet: $STS_NAME ==="

# Step 1: 导出 StatefulSet
echo ">>> Step 1: 导出资源"
kubectl --context=$SOURCE_CONTEXT get sts $STS_NAME -n $NS -o yaml | kubectl neat > sts-$STS_NAME.yaml

# Step 2: 适配 StorageClass
echo ">>> Step 2: 适配 StorageClass"
sed -i '' 's/storageClassName: .*/storageClassName: alicloud-disk-essd/' sts-$STS_NAME.yaml

# Step 3: 先创建 PVC（预创建以便先灌数据）
echo ">>> Step 3: 创建 PVC"
REPLICAS=$(yq eval '.spec.replicas' sts-$STS_NAME.yaml)
VCT_NAME=$(yq eval '.spec.volumeClaimTemplates[0].metadata.name' sts-$STS_NAME.yaml)
STORAGE=$(yq eval '.spec.volumeClaimTemplates[0].spec.resources.requests.storage' sts-$STS_NAME.yaml)

for i in $(seq 0 $((REPLICAS-1))); do
  PVC_NAME="$VCT_NAME-$STS_NAME-$i"
  kubectl --context=$ACK_CONTEXT apply -f - <<EOF
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: $PVC_NAME
  namespace: $NS
spec:
  accessModes: ["ReadWriteOnce"]
  storageClassName: alicloud-disk-essd
  resources:
    requests:
      storage: $STORAGE
EOF
done

# Step 4: 数据迁移（具体方式取决于服务类型）
echo ">>> Step 4: 数据迁移（请根据服务类型手动执行）"
echo "  MySQL: mysqldump + mysql import"
echo "  Redis: redis-shake / RDB copy"
echo "  ES: snapshot/restore"
echo "  通用: tar + kubectl cp"

# Step 5: 部署 StatefulSet
echo ">>> Step 5: 部署 StatefulSet"
kubectl --context=$ACK_CONTEXT apply -f sts-$STS_NAME.yaml

# Step 6: 验证
echo ">>> Step 6: 验证"
kubectl --context=$ACK_CONTEXT rollout status sts/$STS_NAME -n $NS --timeout=600s
```
---

<!-- chunk: 8. 数据一致性校验 -->## 8. 数据一致性校验

## 8.1 MySQL 校验

```bash
# 使用 pt-table-checksum 校验
pt-table-checksum \
  --host=<source-mysql> \
  --user=root --password="$SRC_PASSWORD" \
  --replicate=percona.checksums \
  --databases=production

# 对比校验结果
pt-table-sync --print \
  --source h=<source-mysql>,u=root,p="$SRC_PASSWORD" \
  --dest h=<ack-mysql>,u=root,p="$ACK_PASSWORD" \
  --databases production

# 简易校验: 行数对比
echo "=== 表行数对比 ==="
mysql -h <source-mysql> -u root -p"$SRC_PASSWORD" -e "
  SELECT table_name, table_rows 
  FROM information_schema.tables 
  WHERE table_schema='production' 
  ORDER BY table_name;" | sort > /tmp/src_counts.txt

mysql -h <ack-mysql> -u root -p"$ACK_PASSWORD" -e "
  SELECT table_name, table_rows 
  FROM information_schema.tables 
  WHERE table_schema='production' 
  ORDER BY table_name;" | sort > /tmp/ack_counts.txt

diff /tmp/src_counts.txt /tmp/ack_counts.txt
```

## 8.2 Redis 校验

```bash
# Key 数量对比
SRC_KEYS=$(redis-cli -h <source-redis> -a "$SRC_PASSWORD" DBSIZE | awk '{print $2}')
ACK_KEYS=$(redis-cli -h <ack-redis> -a "$ACK_PASSWORD" DBSIZE | awk '{print $2}')
echo "源 Redis: $SRC_KEYS keys, ACK Redis: $ACK_KEYS keys"

# 抽样校验
redis-cli -h <source-redis> -a "$SRC_PASSWORD" RANDOMKEY | while read key; do
  src_val=$(redis-cli -h <source-redis> -a "$SRC_PASSWORD" GET "$key")
  ack_val=$(redis-cli -h <ack-redis> -a "$ACK_PASSWORD" GET "$key")
  if [ "$src_val" = "$ack_val" ]; then
    echo "OK: $key"
  else
    echo "MISMATCH: $key"
  fi
done
```

---

<!-- chunk: 检查清单 -->## 检查清单

- [ ] 有状态服务迁移策略已确定（托管服务 vs K8s StatefulSet）
- [ ] MySQL 数据已迁移并校验通过
- [ ] Redis 数据已迁移（或缓存重建策略已确认）
- [ ] Elasticsearch 索引已迁移并校验通过
- [ ] Kafka/MQ 消息同步完成
- [ ] 所有应用连接串已更新为新地址
- [ ] 有状态服务性能基线已对比
- [ ] 数据一致性校验通过

---

**上一步**: ← [05-网络迁移与流量切换](./05-network-migration-traffic-cutover.md)
**下一步**: → [07-可观测性与安全迁移](./07-observability-security-migration.md)

---

<!-- chunk: Obsidian 相关文档 -->## Obsidian 相关文档

- topic-migration KUDIG Database — Global MOC
- [[发布变更/迁移方案/README.md|自建 Kubernetes 迁移至阿里云 ACK 生产实践指南]]
- [[发布变更/迁移方案/01-migration-assessment-planning.md|01 - 迁移评估与规划]]
- [[发布变更/迁移方案/02-ack-target-cluster-design.md|02 - ACK 目标集群设计与搭建]]
- [[发布变更/迁移方案/03-application-workload-migration.md|03 - 应用工作负载迁移]]
- [[发布变更/迁移方案/04-storage-data-migration.md|04 - 存储与数据迁移]]
- [[发布变更/迁移方案/05-network-migration-traffic-cutover.md|05 - 网络迁移与流量切换]]
- [[发布变更/迁移方案/07-observability-security-migration.md|07 - 可观测性与安全迁移]]
- [[发布变更/迁移方案/08-validation-cutover-decommission.md|08 - 验收、切换与旧集群退役]]
- [[发布变更/迁移方案/09-migration-toolchain.md|09 - 迁移工具链参考]]
- [[发布变更/迁移方案/10-real-world-case-study.md|10 - 生产迁移实战案例]]

## See Also

- 04-storage-data-migration
- 05-network-migration-traffic-cutover
- 07-observability-security-migration
- 08-validation-cutover-decommission

## Related

- [[生态参考/领域索引/backup-dr-index.md|Backup & DR 备份与灾备知识图谱索引]]
- [[生态参考/领域索引/pvc-index.md|PVC 知识图谱索引]]


<!-- risk-assessed -->
