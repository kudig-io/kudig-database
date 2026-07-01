---
title: 数据库 Operator 设计模式
description: '# 数据库 Operator 设计模式'
summary: '# 数据库 Operator 设计模式'
category: domain
tags:
- kubernetes
- operator
- database
- crd
- pattern
- redis
- mysql
- postgresql
- kafka
- statefulset
tier: supporting
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 数据库 Operator 设计模式 是什么
- 如何 数据库 Operator 设计模式
- Kubernetes 16 database middleware 最佳实践
trigger_keywords:
- 数据库
- Operator
- 设计模式
- database
- middleware
prerequisites:
- kubectl-basics
- kafka-basics
- redis-basics
- mysql-basics
---



# 数据库 Operator 设计模式

## 核心职责

```
数据库 Operator:
├── 生命周期管理
│   ├── 部署（单节点/主从/集群）
│   ├── 配置管理
│   └── 版本升级
├── 高可用管理
│   ├── 故障检测
│   ├── 自动故障转移
│   └── 拓扑维护
├── 数据保护
│   ├── 定时备份
│   ├── 按需恢复
│   └── 跨区域复制
└── 监控集成
    ├── 指标暴露
    ├── 告警规则
    └── 日志收集
```

## 状态管理

```yaml
# MySQL Operator 示例 CR
apiVersion: mysql.oracle.com/v2
kind: InnoDBCluster
metadata:
  name: mycluster
spec:
  instances: 3
  router:
    instances: 1
  secretName: mypwds
  tlsUseSelfSigned: true
status:
  clusterOnline: true
  onlineInstances: 3
  status: ONLINE
```

## 常见 Operator

| 数据库 | Operator | 成熟度 |
|--------|---------|--------|
| MySQL | Oracle MySQL Operator | GA |
| PostgreSQL | [[CloudNativePG|CloudNativePG]] / Zalando | GA |
| Redis | Redis Operator / Spotahome | GA |
| MongoDB | MongoDB Community Operator | GA |
| Cassandra | Cass Operator | GA |
| Kafka | [[Strimzi|Strimzi]] | GA |

## 反模式

```
❌ 在 Operator 中实现业务逻辑
✅ Operator 只管理基础设施状态

❌ 直接操作 Pod 而非 StatefulSet
✅ 使用 StatefulSet 管理有状态 Pod

❌ 忽略 finalizer 导致资源泄漏
✅ 正确使用 finalizer 进行清理
```

## 相关

- domain-15-specialized-tech/02-operator-development-patterns
- [[domain-16-database-middleware/05-operator-management/02-operator-comparison-mysql-postgres-redis.md|02 operator comparison mysql postgres redis]]
