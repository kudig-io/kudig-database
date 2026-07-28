---
title: etcd × Prometheus
summary: etcd × Prometheus：etcd与Prometheus是Kubernetes生产环境中的两个关键维度。理解它们之间的交互关系对于构建稳定、可观测的集群至关重要。
category: synthesis
tags:
- cross-domain
- cluster
- observability
tier: supporting
created: '2026-05-23'
updated: '2026-05-23'
last_updated: 2026-07
relationships:
- target: '[[23-实体/08-交付与制品/helm.md]]'
  type: uses
- target: '[[23-实体/08-交付与制品/argocd.md]]'
  type: related_to
status: reviewed
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息消费，无副作用）。

# etcd × Prometheus

## 概述
Prometheus 是 etcd 最核心的监控工具——etcd 的所有健康指标（Raft 一致性、磁盘 I/O、数据库大小、网络延迟）都通过 Prometheus 采集并存储。etcd 指标的异常往往是整个集群故障的前兆，因此基于 Prometheus 的 etcd 告警是 Kubernetes SRE 的最早期预警系统。这两个组件之间的关系是"被监控者"和"监控者"的直接关系，Prometheus 的正确配置决定了 etcd 故障能否被及时发现。

## 技术关联机制

1. **etcd metrics 暴露与采集**：etcd 在 `https://<etcd-node>:2379/metrics` 端点暴露 Prometheus 格式指标。由于 etcd 使用 mTLS 认证，Prometheus 需要 CA 证书和 client 证书配置。在生产环境中，Prometheus 的 scrape config 包含三个 etcd 节点的地址（或通过 headless service 发现），采集间隔通常为 15-30 秒。

2. **核心 etcd 指标分类**：
   - **Raft 健康类**：`etcd_server_has_leader`、`etcd_server_leader_changes_seen_total`、`etcd_server_proposals_committed_total`、`etcd_server_proposals_applied_total`、`etcd_server_proposals_pending`
   - **磁盘性能类**：`etcd_disk_wal_fsync_duration_seconds_bucket`（WAL 写入延迟）、`etcd_disk_backend_commit_duration_seconds_bucket`（DB 提交延迟）
   - **网络类**：`etcd_network_peer_round_trip_time_seconds`（节点间 RTT）、`etcd_network_client_grpc_received_bytes_total`
   - **存储类**：`etcd_mvcc_db_total_size_in_bytes`（数据库大小）、`etcd_mvcc_delete_total`、`etcd_mvcc_put_total`

3. **Recording Rules 预计算**：生产环境通常配置 Prometheus Recording Rules 预计算 etcd 的聚合指标（如 P99 fsync 延迟、proposal commit rate），减少查询时的实时计算开销。这些预计算结果也存储在 Prometheus TSDB 中。

4. **etcd 告警的 Alertmanager 路由**：etcd 相关告警通常设置为最高优先级（critical），通过 Alertmanager 路由到 PagerDuty/OnCall 通知渠道。关键告警规则：`etcd_server_has_leader == 0`（集群无 leader）、`histogram_quantile(0.99, etcd_disk_wal_fsync_duration_seconds_bucket) > 0.025`（fsync P99 > 25ms）、`etcd_mvcc_db_total_size_in_bytes > 1800000000`（DB 接近 2GB quota）。

## 实践场景

- **etcd 磁盘瓶颈预警**：监控 `etcd_disk_wal_fsync_duration_seconds` P99 趋势，在延迟超过 10ms 时预警，避免等到 25ms 告警时已影响 apiserver 性能
- **Raft 不一致检测**：监控 `etcd_server_proposals_pending` 持续 > 0，检测 Raft 提案积压指示的节点间不一致
- **数据库容量趋势分析**：通过 `etcd_mvcc_db_total_size_in_bytes` 长期趋势预测 etcd 存储增长，提前规划 compaction 或扩容
- **灾备验证**：监控 etcd snapshot 备份的大小和频率，验证备份任务正常执行

## 常见问题

### 问题1：Prometheus 无法采集 etcd metrics
**症状**：Prometheus targets 页面显示 etcd target 为 DOWN
**根因**：mTLS 证书过期或配置错误；或 etcd 监听地址未包含 Prometheus 所在节点
**修复**：检查证书有效期和配置；确认 etcd `--listen-client-urls` 包含 Prometheus 可达的地址

### 问题2：etcd 告警延迟过大
**症状**：etcd 故障后 Alertmanager 告警延迟数分钟才触发
**根因**：Prometheus scrape interval 过大；或 Alertmanager 的 group_wait 配置过长
**修复**：将 etcd scrape interval 调整为 15s；设置 etcd 告警的 `for: 30s` 短持续时间和优先路由

### 问题3：etcd 指标 cardinality 过高导致 Prometheus 存储/查询性能下降
**症状**：Prometheus 查询 etcd 指标响应慢，TSDB 占用大
**根因**：部分 etcd 指标（如 `etcd_network_peer_round_trip_time_seconds_bucket`）在多节点集群中 cardinality 较高
**修复**：使用 `metric_relabel_configs` 过滤不需要的 histogram bucket；调整 retention period

## 关键命令

```bash
# 🟢 直接查看 etcd metrics
curl -s --cacert <ca> --cert <cert> --key <key> https://127.0.0.1:2379/metrics | grep etcd_disk

# 🟢 查看 Prometheus 对 etcd 的 scrape 状态
kubectl -n monitoring exec prometheus-0 -- wget -qO- 'http://localhost:9090/api/v1/targets' | jq '.data.activeTargets[] | select(.labels.job|test("etcd"))'

# 🟢 查询 etcd fsync P99 延迟（Prometheus PromQL）
# 在 Prometheus UI 中执行:
# histogram_quantile(0.99, rate(etcd_disk_wal_fsync_duration_seconds_bucket[5m]))

# 🟢 检查 etcd alertmanager 规则
kubectl get prometheusrule -n monitoring | grep etcd

# 🟢 查看 etcd 数据库大小趋势
ETCDCTL_API=3 etcdctl --endpoints=<endpoints> endpoint status --write-out=table
```

## 权衡取舍

| 维度 | etcd 倾向 | Prometheus 倾向 | 权衡点 |
|------|----------|----------------|--------|
| Scrape 频率 | 低频减少 etcd 负载 | 高频提升监控精度 | etcd 性能 vs 监控精度 |
| 指标 cardinality | 低基数减少序列数 | 高基数精细分析 | 存储成本 vs 分析能力 |
| 证书管理 | mTLS 保证安全 | 证书配置增加复杂度 | 安全性 vs 运维复杂度 |
| 告警策略 | 高敏感快速发现 | 低敏感减少噪声 | 故障发现速度 vs 误报率 |

## 最佳实践
1. 配置专用的 Prometheus scrape job 采集 etcd metrics，使用 mTLS 证书认证，采集间隔 15s
2. 设置 etcd 关键告警：`has_leader == 0`（critical）、`fsync P99 > 25ms`（warning）、`db_size > 1.8GB`（warning）
3. 使用 Prometheus Recording Rules 预计算 etcd P99 延迟和 proposal rate，减少实时查询开销
4. 将 etcd 告警路由到最高优先级通知渠道（PagerDuty/电话），确保 SRE 在 etcd 异常时第一时间响应

## 工具推荐
- kubectl：基础诊断
- [[23-实体/08-交付与制品/helm.md|Helm]]/Kustomize：配置管理
- Prometheus/Grafana：联合监控
- [[23-实体/08-交付与制品/argocd.md|ArgoCD]]：GitOps同步

## 相关概念
- [[etcd]]
- [[prometheus|Prometheus]]
## Related

- [[23-实体/02-K8s核心组件/kubernetes.md|Kubernetes (CNCF Graduated)]]
- [[23-实体/08-交付与制品/argo.md|Argo Workflows]]
- [[17-系统基础/05-速查卡/git.md|Git 速查卡]]
- [[19-故障诊断/08-技能体系/skill-set/k8s-pvc-storage/DIALOGUE.md|DIALOGUE]]
- [[22-概念/11-交叉分析/etcd-×-PVC.md|etcd-×-PVC]]
- [[22-概念/11-交叉分析/apiserver-×-Pod诊断.md|apiserver-×-Pod诊断]]
- [[22-概念/11-交叉分析/etcd-×-灾难恢复.md|etcd-×-灾难恢复]]


<!-- risk-assessed -->
