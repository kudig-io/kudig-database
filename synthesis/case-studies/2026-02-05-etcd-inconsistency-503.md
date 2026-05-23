---
title: "[2026-02-05] [P0] etcd 数据不一致导致 API Server 间歇性 503"
category: case-study
tags: [production, incident, control-plane, etcd, apiserver]
date: "2026-02-05"
severity: P0
mttr: "45min"
status: resolved
created: "2026-05-23"
updated: "2026-05-23"
---

# [2026-02-05] etcd 数据不一致导致 API Server 503 ServiceUnavailable

## 工单信息
- **工单编号**: INC-2026-0205-003
- **发现时间**: 2026-02-05 03:12 UTC
- **恢复时间**: 2026-02-05 03:57 UTC
- **影响范围**: 全集群 API 操作（kubectl、控制器、调度器）
- **业务影响**: 新 Pod 无法创建，HPA 无法扩容，CI/CD 流水线全部失败

## 问题现象
03:12，监控系统告警 `K8sAPIServerErrorRate` 飙升至 35%。症状：
- `kubectl get pods` 偶发返回 `Error from server (ServiceUnavailable): the server is currently unable to handle the request`
- `kubectl apply` 失败率 > 40%
- Deployment 控制器日志出现大量 `ListWatch` 超时

## 诊断过程

**03:15** — 检查 API Server 状态：
```bash
kubectl get --raw /healthz
# [+]ping ok
# [+]log ok
# [-]etcd failed: reason withheld
# healthz check failed
```

**03:17** — 查看 etcd Pod 状态：
```bash
kubectl get pods -n kube-system -l component=etcd
# NAME             READY   STATUS    RESTARTS   AGE
# etcd-master-0    1/1     Running   0          120d
# etcd-master-1    1/1     Running   0          120d
# etcd-master-2    1/1     Running   0          120d
```

**03:19** — 查看 etcd 日志，发现大量 `mvcc: database space exceeded`：
```bash
kubectl logs -n kube-system etcd-master-0 | tail -n 50
# 2026-02-05T03:11:33.112Z ... mvcc: database space exceeded (quota: 8589934592)
# 2026-02-05T03:11:33.113Z ... etcdserver: failed to apply request "..." with response "..." 
#   took too long (2.345s) to execute
```

**03:22** — 检查 etcd 数据库大小和一致性：
```bash
ETCDCTL_API=3 etcdctl --endpoints https://10.0.1.10:2379 \
  --cacert /etc/kubernetes/pki/etcd/ca.crt \
  --cert /etc/kubernetes/pki/etcd/server.crt \
  --key /etc/kubernetes/pki/etcd/server.key \
  endpoint status -w table
# +----------------+------------------+---------+---------+-----------+------------+-----------+------------+--------------------+--------+
# |   ENDPOINT     |        ID        | VERSION | DB SIZE | IS LEADER | IS LEARNER | RAFT TERM | RAFT INDEX | RAFT APPLIED INDEX | ERRORS |
# +----------------+------------------+---------+---------+-----------+------------+-----------+------------+--------------------+--------+
# | 10.0.1.10:2379 | 8e9e05c52164694d |  3.5.10 | 8.2 GB  |      true |      false |         7 |    892341 |             892341 |        |
# | 10.0.1.11:2379 | bc62ac0d21c8b4e3 |  3.5.10 | 4.1 GB  |     false |      false |         7 |    892341 |             892341 |        |
# | 10.0.1.12:2379 | cb5c2394fae19a02 |  3.5.10 | 4.1 GB  |     false |      false |         7 |    892341 |             892341 |        |
# +----------------+------------------+---------+---------+-----------+------------+-----------+------------+--------------------+--------+
```

**03:25** — 发现 etcd-master-0 的 DB SIZE 为 8.2GB（超过 8GB quota），其余节点为 4.1GB。Leader 节点因空间超限拒绝写入，导致 API Server 503。差异原因：master-0 在 01-20 的维护窗口中曾短暂脱离集群，期间 compaction/defrag 未执行，导致历史版本堆积。

## 根因
etcd 数据库大小超过 quota（8GB），Leader 节点拒绝新写入请求。根本原因是：
1. `auto-compaction-retention` 设置为 `1h`（过小），compaction 频率高但历史 revision 未清理
2. 缺少定期 defragmentation 作业
3. 事件对象（Event）未设置 TTL，大量 `FailedScheduling`、`ImagePullBackOff` 事件堆积

## 修复动作

**03:28** — 临时提升 quota 以恢复写入：
```bash
ETCDCTL_API=3 etcdctl --endpoints https://10.0.1.10:2379 \
  --cacert /etc/kubernetes/pki/etcd/ca.crt \
  --cert /etc/kubernetes/pki/etcd/server.crt \
  --key /etc/kubernetes/pki/etcd/server.key \
  alarm disarm
# all alarms disabled

ETCDCTL_API=3 etcdctl --endpoints https://10.0.1.10:2379 \
  --cacert /etc/kubernetes/pki/etcd/ca.crt \
  --cert /etc/kubernetes/pki/etcd/server.crt \
  --key /etc/kubernetes/pki/etcd/server.key \
  put /quota/bytes 17179869184
```

**03:32** — 执行 compaction 和 defragmentation：
```bash
# 获取当前 revision
ETCDCTL_API=3 etcdctl --endpoints https://10.0.1.10:2379 \
  --cacert /etc/kubernetes/pki/etcd/ca.crt \
  --cert /etc/kubernetes/pki/etcd/server.crt \
  --key /etc/kubernetes/pki/etcd/server.key \
  endpoint status --write-out=json | jq '.[].Status.header.revision'
# 892341

# 在所有节点顺序执行 compaction 和 defrag
for node in 10.0.1.10 10.0.1.11 10.0.1.12; do
  ETCDCTL_API=3 etcdctl --endpoints https://${node}:2379 \
    --cacert /etc/kubernetes/pki/etcd/ca.crt \
    --cert /etc/kubernetes/pki/etcd/server.crt \
    --key /etc/kubernetes/pki/etcd/server.key \
    compaction 892000
  ETCDCTL_API=3 etcdctl --endpoints https://${node}:2379 \
    --cacert /etc/kubernetes/pki/etcd/ca.crt \
    --cert /etc/kubernetes/pki/etcd/server.crt \
    --key /etc/kubernetes/pki/etcd/server.key \
    defrag
done
```

**03:45** — 验证 etcd 状态：
```bash
ETCDCTL_API=3 etcdctl ... endpoint status -w table
# | 10.0.1.10:2379 | ... |  3.5.10 | 2.1 GB  | ... |
# | 10.0.1.11:2379 | ... |  3.5.10 | 2.1 GB  | ... |
# | 10.0.1.12:2379 | ... |  3.5.10 | 2.1 GB  | ... |
```

**03:50** — 清理历史 Event：
```bash
kubectl delete events --all-namespaces --field-selector type=Warning
```

## 验证
- 03:52 — `kubectl get --raw /healthz` 全部通过
- 03:55 — CI/CD 流水线恢复，新 Pod 正常创建
- 03:57 — API Server p99 延迟从 8.5s 恢复至 120ms

## 复盘
- **直接原因**: etcd Leader DB 超过 quota → alarm: NOSPACE → 拒绝写入 → API Server 503
- **根本原因**: auto-compaction 配置不当 + 缺少 defrag CronJob + Event 未清理
- **改进措施**:
  1. etcd `auto-compaction-retention` 改为 `24h`，并添加每周 defrag CronJob
  2. 部署 etcd 监控告警：`etcd_db_size > 6GB` 提前预警
  3. 为 Event 对象设置 7 天 TTL：`event-ttl=168h0m0s`
  4. 所有 etcd 维护操作前执行 `etcdctl endpoint hashkv` 验证一致性
- **相关 Skill**: [[backup-restore-etcd]]
- **相关 FTA**: [[etcd-fta]]
