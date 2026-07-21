---
title: 灾备场景目录
description: '| 区域级网络中断 | 整个 Region | 30 分钟 | 0 |'
summary: '| 区域级网络中断 | 整个 Region | 30 分钟 | 0 |'
category: domain
tags:
- disaster-recovery
- dr
- scenarios
- sre
- etcd
tier: supporting
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 灾备场景目录 是什么
- 如何 灾备场景目录
- Kubernetes 09 reliability engineering 最佳实践
trigger_keywords:
- 灾备场景目录
- reliability
- engineering
prerequisites:
- kubectl-basics
- sre-practices
- etcd-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 灾备场景目录

## 场景分类

### 基础设施层

| 场景 | 影响范围 | RTO 目标 | RPO 目标 |
|------|---------|---------|---------|
| 单可用区问题 | 1 个 AZ | 5 分钟 | 0 |
| 区域级网络中断 | 整个 Region | 30 分钟 | 0 |
| [[Kubernetes|Kubernetes]] 控制面问题 | 集群管理 | 15 分钟 | 0 |
|  [[etcd|etcd]] 数据损坏 | 集群状态 | 30 分钟 | 5 分钟 |

### 应用层

| 场景 | 影响范围 | RTO 目标 | RPO 目标 |
|------|---------|---------|---------|
| 核心服务级联问题 | 多个服务 | 10 分钟 | 0 |
| 数据库主节点问题 | 数据服务 | 5 分钟 | 0 |
| 缓存集群完全失效 | 读性能 | 15 分钟 | 0 |
| 消息队列堆积 | 异步处理 | 30 分钟 | 5 分钟 |

### 外部依赖

| 场景 | 影响范围 | RTO 目标 | RPO 目标 |
|------|---------|---------|---------|
| 第三方支付服务中断 | 交易功能 | 60 分钟 | 0 |
| CDN 问题 | 静态资源 | 10 分钟 | 0 |
| DNS 服务商问题 | 全站访问 | 15 分钟 | 0 |
| 云厂商 API 限流 | 自动化运维 | 30 分钟 | 0 |

## 场景详细响应手册

### 场景 1: 单可用区故障

**检测信号**:
- 节点 NotReady 告警（同一 AZ 多个节点）
- Pod 重新调度到其仙 AZ
- 跨 AZ 延迟增加

**响应步骤**:
```bash
# 🟢 低风险：检查受影响节点
kubectl get nodes -l topology.kubernetes.io/zone=<affected-zone>
kubectl get pods -A --field-selector spec.nodeName=<node> -o wide

# 🟡 中风险：手动驱逐受影响节点上的 Pod
kubectl drain <node> --ignore-daemonsets --delete-emptydir-data --timeout=60s

# 🟢 低风险：验证 Pod 已重新调度
kubectl get pods -A -o wide | grep -v <affected-zone>
```

**自动化恢复**:
```yaml
# 节点自动修复 DaemonSet
apiVersion: apps/v1
kind: DaemonSet
metadata:
  name: node-auto-repair
  namespace: kube-system
spec:
  selector:
    matchLabels:
      app: node-auto-repair
  template:
    metadata:
      labels:
        app: node-auto-repair
    spec:
      tolerations:
        - operator: Exists
      containers:
        - name: repair
          image: bitnami/kubectl:latest
          command:
            - /bin/sh
            - -c
            - |
              while true; do
                # 检查节点健康状态
                NOT_READY=$(kubectl get nodes --no-headers | grep -c NotReady || true)
                if [ "$NOT_READY" -gt 0 ]; then
                  echo "检测到 $NOT_READY 个 NotReady 节点"
                  # 发送告警
                  curl -X POST -H 'Content-type: application/json' \
                    --data '{"text":"⚠️ 检测到节点故障"}' \
                    $SLACK_WEBHOOK
                fi
                sleep 60
              done
```

### 场景 2: etcd 数据损坏

**检测信号**:
- API Server 无法连接 etcd
- etcd 集群健康检查失败
- `etcdctl endpoint health` 返回错误

**响应步骤**:
```bash
# 🟢 低风险：检查 etcd 健康状态
ETCDCTL_API=3 etcdctl endpoint health --cluster
ETCDCTL_API=3 etcdctl endpoint status --cluster -w table

# 🔴 高风险：从快照恢复 etcd
# 1. 停止所有控制平面组件
systemctl stop kube-apiserver kube-controller-manager kube-scheduler

# 2. 停止 etcd
systemctl stop etcd

# 3. 备份当前数据
mv /var/lib/etcd /var/lib/etcd.bak.$(date +%Y%m%d)

# 4. 从快照恢复
ETCDCTL_API=3 etcdctl snapshot restore /backup/etcd-snapshot.db \
  --name etcd-1 \
  --initial-cluster etcd-1=https://10.0.0.1:2380 \
  --initial-cluster-token etcd-cluster \
  --initial-advertise-peer-urls https://10.0.0.1:2380 \
  --data-dir /var/lib/etcd

# 5. 启动 etcd
systemctl start etcd

# 6. 验证
ETCDCTL_API=3 etcdctl endpoint health --cluster

# 7. 启动控制平面
systemctl start kube-apiserver kube-controller-manager kube-scheduler
```

### 场景 3: 数据库主节点故障

**检测信号**:
- 数据库连接失败
- 主从复制中断
- 应用层 5xx 错误增加

**响应步骤**:
```bash
# 🟢 低风险：检查数据库状态
kubectl exec -n database deploy/postgres -- pg_isready
kubectl exec -n database deploy/postgres -- psql -c "SELECT pg_is_in_recovery();"

# 🟡 中风险：手动触发主从切换
# PostgreSQL: 提升从库为主库
kubectl exec -n database sts/postgres-1 -- pg_ctl promote

# 更新 Service 指向新主库
kubectl patch svc postgres-primary -n database -p '{"spec":{"selector":{"statefulset.kubernetes.io/pod-name":"postgres-1"}}}'

# 🟢 低风险：验证切换成功
kubectl exec -n database deploy/postgres -- psql -c "SELECT pg_is_in_recovery();"  # 应返回 f
```

### 场景 4: 缓存集群完全失效

**检测信号**:
- Redis 连接失败
- 缓存命中率降为 0
- 数据库负载突然增加

**响应步骤**:
```bash
# 🟢 低风险：检查 Redis 状态
kubectl exec -n cache deploy/redis -- redis-cli ping
kubectl exec -n cache deploy/redis -- redis-cli info replication

# 🟡 中风险：重启 Redis 集群
kubectl rollout restart statefulset/redis -n cache

# 🟢 低风险：预热缓存
kubectl exec -n cache deploy/redis -- redis-cli --eval /scripts/warmup.lua

# 检查数据库负载
kubectl top pods -n database
```

## 检测与告警配置

### PrometheusRule 多场景告警

```yaml
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: dr-scenario-alerts
  namespace: monitoring
spec:
  groups:
    - name: dr.infrastructure.rules
      rules:
        # 单 AZ 多节点故障
        - alert: AZMultipleNodesDown
          expr: |
            count by (topology.kubernetes.io/zone) (
              kube_node_status_condition{condition="Ready", status="false"} == 1
            ) >= 2
          for: 2m
          labels:
            severity: critical
            scenario: az-failure
          annotations:
            summary: "可用区 {{ $labels.topology_kubernetes_io_zone }} 多个节点故障"

        # etcd 集群不健康
        - alert: EtcdClusterUnhealthy
          expr: |
            etcd_server_has_leader == 0 or etcd_server_leader_changes_seen_total > 3
          for: 1m
          labels:
            severity: critical
            scenario: etcd-failure
          annotations:
            summary: "etcd 集群不健康"

    - name: dr.application.rules
      rules:
        # 数据库主从复制中断
        - alert: DatabaseReplicationBroken
          expr: |
            pg_replication_lag_seconds == -1
          for: 1m
          labels:
            severity: critical
            scenario: db-failover
          annotations:
            summary: "数据库复制中断，可能需要主从切换"

        # 缓存命中率过低
        - alert: CacheHitRateLow
          expr: |
            rate(redis_keyspace_hits_total[5m]) 
            / (rate(redis_keyspace_hits_total[5m]) + rate(redis_keyspace_misses_total[5m])) < 0.5
          for: 10m
          labels:
            severity: warning
            scenario: cache-degradation
          annotations:
            summary: "缓存命中率低于 50%"

    - name: dr.external.rules
      rules:
        # 外部依赖超时
        - alert: ExternalDependencyTimeout
          expr: |
            rate(http_client_requests_seconds_count{status="timeout"}[5m]) > 0.1
          for: 5m
          labels:
            severity: warning
            scenario: external-dependency
          annotations:
            summary: "外部依赖 {{ $labels.service }} 超时率过高"
```

## 场景优先级矩阵

| 场景 | 影响程度 | 发生频率 | 优先级 | 演练频率 |
|-----|---------|---------|-------|----------|
| 单 AZ 故障 | 高 | 中 | **P0** | 月度 |
| etcd 数据损坏 | 极高 | 低 | **P0** | 季度 |
| 数据库主节点故障 | 高 | 中 | **P0** | 月度 |
| 区域级网络中断 | 极高 | 低 | **P1** | 季度 |
| 缓存集群失效 | 中 | 中 | **P1** | 季度 |
| 第三方支付中断 | 中 | 高 | **P1** | 月度 |
| CDN 故障 | 中 | 中 | **P2** | 半年度 |
| DNS 服务商故障 | 高 | 低 | **P2** | 半年度 |
| 云 API 限流 | 低 | 高 | **P3** | 年度 |

## 演练检查清单

### 演练前准备

| 序号 | 检查项 | 状态 |
|-----|--------|------|
| 1 | 演练场景已确定 | ☐ |
| 2 | 成功/失败标准已定义 | ☐ |
| 3 | 回滚方案已准备 | ☐ |
| 4 | 监控仪表盘已就绪 | ☐ |
| 5 | 通知已发送给相关团队 | ☐ |
| 6 | 备份已完成 | ☐ |
| 7 | 审批已获得 | ☐ |

### 演练后复盘

| 序号 | 检查项 | 状态 |
|-----|--------|------|
| 1 | RTO 实际值 vs 目标值 | ☐ |
| 2 | RPO 实际值 vs 目标值 | ☐ |
| 3 | 检测时间 (MTTD) | ☐ |
| 4 | 恢复时间 (MTTR) | ☐ |
| 5 | 发现的问题已记录 | ☐ |
| 6 | 改进行动已分配 | ☐ |
| 7 | 报告已归档 | ☐ |

## 相关

- 可靠性/02-disaster-recovery/README.md


<!-- risk-assessed -->
