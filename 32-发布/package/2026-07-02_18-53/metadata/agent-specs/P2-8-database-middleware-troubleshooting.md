---
title: 数据库中间件问题排查指南
description: '# 数据库中间件问题排查指南'
summary: '# 数据库中间件问题排查指南'
category: general
tags:
- k8s
- etcd
- redis
- mysql
- postgresql
- operator
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 数据库中间件问题排查指南 是什么
- 如何 数据库中间件问题排查指南
- 数据库中间件问题排查指南 问题排查
- 数据库中间件问题排查指南 排障步骤
trigger_keywords:
- 数据库中间件问题排查指南
prerequisites:
- kubectl-basics
- etcd-basics
- redis-basics
- mysql-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 数据库中间件问题排查指南

> **版本**: v1.0
> **创建日期**: 2026-05-18
> **用途**: MySQL/PostgreSQL/Redis 常见问题的快速诊断与修复
> **覆盖**: MySQL Operator、Redis Cluster 脑裂、数据库备份恢复

---

## 1. MySQL 问题排查

### 1.1 连接问题

| 症状 | 诊断命令 | 根因 | 修复 |
|------|---------|------|------|
| 连接超时 | `mysqladmin ping -h <host>` | 网络/防火墙 | 检查安全组/防火墙规则 |
| 访问拒绝 | `mysql -u <user> -p` | 密码错误/权限不足 | 检查用户权限 |
| max_connections 满 | `show global status like 'Max_used_connections'` | 连接池泄漏 | 重启连接或增加 max_connections |
| 无法连接 | `netstat -tlnp | grep 3306` | MySQL 未运行 | `systemctl start mysql` 或检查 Pod 状态 |

### 1.2 主从复制问题

```bash
# 检查复制状态
mysql -e "SHOW SLAVE STATUS\G" | grep -E "Slave_IO_Running|Slave_SQL_Running|Seconds_Behind"

# 常见复制问题
# Slave_IO_Running: No → 主库 binlog 未传输到从库
# 解决: 检查主库网络、防火墙、binlog position

# Slave_SQL_Running: No → 从库执行 SQL 失败
# 解决: 查看 Last_Error，跳过错误事务
mysql -e "STOP SLAVE; SET GLOBAL sql_slave_skip_counter=1; START SLAVE;"

# Seconds_Behind_Master > 10 → 复制延迟
# 解决: 检查网络带宽、大事务、磁盘 IO

# 重置复制
mysql -e "RESET SLAVE ALL; CHANGE MASTER TO MASTER_HOST='<master>', MASTER_LOG_FILE='<binlog>', MASTER_LOG_POS=<pos>;"
```

### 1.3 InnoDB 问题

```bash
# 检查 InnoDB 状态
mysql -e "SHOW ENGINE INNODB STATUS\G"

# 常见问题
# 死锁 (Deadlock)
# → 查看 Latest Deadlock 输出
# → 优化事务，减少锁范围

# 缓冲池命中率低
# → 调大 innodb_buffer_pool_size (建议 70-80% 内存)
# → 检查缓存效率: SHOW ENGINE INNODB STATUS | grep "Buffer pool hit rate"

# 事务回滚
# → 查看 TRX_ROLLBACK_SEGMENT
# → 长事务需尽早提交
```

### 1.4 MySQL Operator 问题 ([[entities/kubernetes.md|k8s]])

| 症状 | 诊断命令 | 根因 | 修复 |
|------|---------|------|------|
| MySQL Pod 无法启动 | `kubectl describe pod -n mysql` | 配置文件错误/资源不足 | 检查 PVC、资源限制 |
| 主从切换失败 | `kubectl logs <pod> -n mysql` | Patroni/etcd 连接问题 | 检查 etcd 状态 |
| 数据不一致 | - | 主从复制中断未修复 | 重新同步数据或重建从库 |

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl delete`：删除资源（可由声明式清单重建）
> - `kubectl edit/patch`：修改运行中的资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# Percona Operator 检查
kubectl get pods -n mysql-operator
kubectl describe pxc db-cluster -n mysql

# 恢复失败的 Pod
kubectl delete pod <pod> -n mysql --grace-period=0

# 手动触发 failover
kubectl patch pxc db-cluster -n mysql -p '{"spec":{"forceStandalone":true}}' --type=merge
```
---

## 2. PostgreSQL 问题排查

### 2.1 连接与认证问题

| 症状 | 诊断命令 | 根因 | 修复 |
|------|---------|------|------|
| 连接拒绝 | `pg_isready -h <host> -p 5432` | PostgreSQL 未启动 | `systemctl start postgresql` |
| 认证失败 | `psql -h <host>` | pg_hba.conf 配置/密码错误 | 检查 pg_hba.conf 规则 |
| 连接数满 | `SELECT count(*) FROM pg_stat_activity` | max_connections 不足 | 增加 max_connections 或收缩连接池 |
| 连接泄漏 | `SELECT * FROM pg_stat_activity WHERE state='idle'` | 应用未释放连接 | 修复应用或使用 PgBouncer |

### 2.2 复制问题

```bash
# 检查复制状态
psql -c "SELECT * FROM pg_stat_replication;"

# 检查复制延迟
psql -c "SELECT now() - pg_last_xact_replay_timestamp() AS replication_delay;"

# 常见问题
# 复制断开 → 检查网络、防火墙、pg_hba.conf
# 复制延迟大 → WAL 堆积太多，优化网络或增加从库
# 槽位 (replication slot) 堆积 → 保留 WAL 不清理

# 重建复制
psql -h <primary> -c "SELECT pg_start_backup('sync');"
# 同步数据目录
psql -h <primary> -c "SELECT pg_stop_backup();"
# 启动从库后配置复制
psql -c "pg_ctl promote -D /var/lib/postgresql/data"
```

### 2.3 Patroni 高可用问题

| 症状 | 诊断命令 | 根因 | 修复 |
|------|---------|------|------|
| Leader 选举失败 | `patronictl list` | etcd 连接问题 | 检查 etcd 集群健康 |
| 自动 failover 未触发 | `patronictl list` 查看状态 | 确认 Patroni 配置 | 检查 `patronctl checkout` |
| 切换后数据丢失 | - | 异步复制配置 | 启用同步复制 (`synchronous_mode: on`) |

```bash
# Patroni 诊断
patronictl list
patronictl check-config
patronictl show-config

# 手动 switchover
patronictl switchover --cluster <name> --candidate <node>

# 强制重新初始化
patronictl reinit --cluster <name> <node>
```

### 2.4 慢查询与性能问题

```bash
# 查看慢查询
SELECT * FROM pg_stat_statements ORDER BY mean_exec_time DESC LIMIT 10;

# 查看锁等待
SELECT * FROM pg_stat_activity WHERE wait_event_type = 'Lock';

# 查看长事务
SELECT * FROM pg_stat_activity WHERE state = 'active' AND query_start < now() - interval '5 minutes';

# 杀掉阻塞进程
SELECT pg_terminate_backend(pid) FROM pg_stat_activity WHERE wait_event_type = 'Lock';
```

---

## 3. Redis 问题排查

### 3.1 连接与认证问题

| 症状 | 诊断命令 | 根因 | 修复 |
|------|---------|------|------|
| 连接失败 | `redis-cli -h <host> ping` | Redis 未运行/网络问题 | 检查 Redis 进程/防火墙 |
| 认证失败 | `redis-cli -h <host> AUTH <password>` | 密码错误 | 检查 requirepass 配置 |
| 连接数满 | `redis-cli INFO clients` | maxclients 限制 | 增加 maxclients 或收缩连接 |
| OOM | `redis-cli INFO memory` | 内存耗尽 | 增加 maxmemory 或清理数据 |

### 3.2 Redis Cluster 脑裂问题

```bash
# 检查集群状态
redis-cli cluster info
redis-cli cluster nodes

# 脑裂检测标志
# 1. 多个 master 节点宣称自己是主
# 2. 节点之间无法通信
# 3. 出现 "cluster down" 告警

# 解决方法
# 1. 确认网络恢复
redis-cli -h <node1> PING
redis-cli -h <node2> PING

# 2. 如果确认一个节点为正确的主，手动移除错误节点
redis-cli cluster forget <node_id>

# 3. 重新加入节点
redis-cli cluster meet <ip> <port>

# 4. 如果数据不一致，以最新数据为准，手动同步
# 从正确的主节点导出数据并导入到其他节点

# 预防措施
# 设置 min-slaves-to-write 和 min-slaves-max-lag
redis-cli CONFIG SET min-slaves-to-write 2
redis-cli CONFIG SET min-slaves-max-lag 10
```

### 3.3 大 Key 问题

```bash
# 查找大 Key
redis-cli --bigkeys
redis-cli --scan | head -1000 | xargs -I {} redis-cli DEBUG OBJECT {} | grep -v "not a string"

# 内存分析
redis-cli MEMORY USAGE <key>
redis-cli MEMORY STATS

# 大 Key 删除 (避免阻塞)
redis-cli UNLINK <key>  # 异步删除
redis-cli --scan --pattern "big:*" | xargs -I {} redis-cli UNLINK {}
```

### 3.4 持久化与备份问题

```bash
# 检查 RDB/AOF 状态
redis-cli INFO persistence

# RDB 配置检查
redis-cli CONFIG GET save
# 建议配置: save 900 1 save 300 10 save 60 10000

# AOF 损坏修复
redis-cli BGREWRITEAOF  # 重建 AOF

# 如果 AOF 严重损坏
redis-cli --pipe < /dev/null
# 或者重启 Redis 自动加载修复

# 手动 BGSAVE
redis-cli BGSAVE
redis-cli LASTSAVE  # 查看上次保存时间
```

### 3.5 Redis Sentinel 高可用问题

```bash
# 检查 Sentinel 状态
redis-cli -p 26379 INFO
redis-cli -p 26379 SENTINEL masters
redis-cli -p 26379 SENTINEL get-master-addr-by-name <master-name>

# 手动 failover
redis-cli -p 26379 SENTINEL failover <master-name>

# 常见问题
# Sentinel 认为 master 宕机 → 检查网络抖动
# 新 master 上线后从库未同步 → 检查 slave 配置
# 问题转移后应用无法连接 → 检查 Sentinel 公告 IP
```

---

## 4. 数据库备份与恢复

### 4.1 MySQL 备份恢复

```bash
# 全量备份 (XtraBackup)
xtrabackup --backup --target-dir=/backup/full --user=root --password=

# 增量备份
xtrabackup --backup --target-dir=/backup/inc1 --incremental-basedir=/backup/full --user=root

# 恢复
xtrabackup --prepare --target-dir=/backup/full
xtrabackup --copy-back --target-dir=/backup/full

# 逻辑备份 (mysqldump)
mysqldump -h <host> -u root -p --all-databases > /backup/all.sql
mysqldump -h <host> -u root -p --single-transaction db_name > /backup/db.sql

# 恢复
mysql -h <host> -u root -p < /backup/db.sql
```

### 4.2 PostgreSQL 备份恢复

```bash
# 全量备份 (pg_basebackup)
pg_basebackup -h <host> -U replication -D /backup/full -Ft -z -P

# WAL 归档 (配合 Barman)
# 配置 barman.conf
[barman]
barman_home = /var/lib/barman
configuration_files_directory = /etc/barman.d
minimum_redundancy = 1

# 恢复 (Point-in-Time Recovery)
# 1. 解压全量备份
tar -xzf /backup/base.tar.gz -C /var/lib/pgsql/data

# 2. 配置恢复
cat >> /var/lib/pgsql/data/postgresql.conf <<EOF
restore_command = 'cp /archive/%f %p'
recovery_target_time = '2026-05-18 10:00:00'
EOF

# 3. 创建恢复信号
touch /var/lib/pgsql/data/recovery.signal

# 4. 启动 PostgreSQL
pg_ctl start -D /var/lib/pgsql/data

# 逻辑备份
pg_dump -h <host> -U postgres db_name > /backup/db.sql
pg_dumpall -h <host> -U postgres > /backup/all.sql

# 恢复
psql -h <host> -U postgres -d db_name < /backup/db.sql
```

### 4.3 Redis 备份恢复

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `systemctl stop/restart`：停止/重启系统服务，影响节点上所有容器

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# RDB 备份
redis-cli BGSAVE
# 检查: redis-cli LASTSAVE

# 复制到 S3
aws s3 cp /var/lib/redis/dump.rdb s3://bucket/redis/dump-$(date +%Y%m%d).rdb

# AOF 备份
redis-cli BGREWRITEAOF
aws s3 cp /var/lib/redis/appendonly.aof s3://bucket/redis/appendonly-$(date +%Y%m%d).aof

# 恢复
aws s3 cp s3://bucket/redis/dump-latest.rdb /var/lib/redis/dump.rdb
chown redis:redis /var/lib/redis/dump.rdb
systemctl restart redis
```
### 4.4 备份验证清单

| 备份类型 | 验证方法 | 验证频率 |
|---------|---------|---------|
| MySQL XtraBackup | `xtrabackup --prepare --target-dir=/backup/full` | 每周 |
| PostgreSQL 全量 | `pg_restore --list /backup/full.tar.zst` | 每月 |
| Redis RDB | `redis-server --test-memory 1024` (测试内存分配) | 每周 |
| 恢复演练 | 在测试环境执行完整恢复流程 | 每季度 |

---

## 5. 快速检查清单

### 数据库 on-call 速查

```bash
# MySQL 健康检查
mysqladmin ping && mysql -e "SHOW SLAVE STATUS\G" | grep -E "Running|Behind" && mysql -e "SHOW PROCESSLIST" | wc -l

# PostgreSQL 健康检查
pg_isready -h <host> && psql -c "SELECT pg_is_in_recovery(), pg_database_size('postgres')" && psql -c "SELECT count(*) FROM pg_stat_activity"

# Redis 健康检查
redis-cli PING && redis-cli INFO replication && redis-cli INFO memory | grep used_memory_human

# 连接池检查 (ProxySQL)
mysql -h 127.0.0.1 -u admin -padmin -e "SELECT * FROM stats_mysql_connection_pool" | head -20

# PgBouncer 检查
psql -h 127.0.0.1 -p 5432 -U pgbouncer -c "SHOW POOLS"
```

---

## 6. 升级条件

| 条件 | 操作 |
|------|------|
| 数据库主节点宕机且无法恢复 | 立即升级 DBA 团队 |
| 数据损坏/丢失 | 立即升级 DBA + 安全团队 |
| 主从复制中断超过 30 分钟 | 升级 DBA 团队 |
| 备份恢复失败 | 立即升级 DBA 团队 |

---

**关联文档**:
- [domain-16-database-middleware/](../domain-16-database-middleware/) — 数据库中间件完整文档
- [domain-10-troubleshooting-diagnostics/](../domain-10-troubleshooting-diagnostics/) — K8s 通用问题排查
- [domain-10-troubleshooting-diagnostics/topic-skills/](../domain-10-troubleshooting-diagnostics/技能体系/) — 通用运维 Skill

<!-- risk-assessed -->
