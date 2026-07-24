---
title: etcd × Grafana
summary: etcd × Grafana：etcd与Grafana是Kubernetes生产环境中的两个关键维度。理解它们之间的交互关系对于构建稳定、可观测的集群至关重要。
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
- target: '[[实体/helm.md]]'
  type: uses
- target: '[[实体/prometheus.md]]'
  type: uses
- target: '[[实体/argocd.md]]'
  type: related_to
status: reviewed
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。

# etcd × Grafana

## 概述
etcd 是 Kubernetes 集群可用性的最后一道防线——etcd 故障意味着整个控制面不可用。Grafana 是 etcd 健康状况最核心的可视化工具，通过 Prometheus 采集 etcd 的 `/metrics` 端点指标，在仪表盘上展示 Raft 一致性、磁盘 I/O、leader 选举、存储压缩等关键健康指标。生产环境中的 etcd Grafana 仪表盘是 SRE 的首要监控入口，etcd 指标异常往往是集群故障的前兆。

## 技术关联机制

1. **etcd metrics 采集链路**：etcd 在 `https://<etcd-node>:2379/metrics` 端点暴露 Prometheus 格式指标。由于 etcd 通常使用私有证书和 mTLS 认证，Prometheus 需要配置专用的 CA 证书和 client 证书才能 scrape。在 managed Kubernetes（EKS/GKE）中，etcd 由云厂商管理，用户可能无法直接 scrape etcd metrics，需要依赖云厂商提供的控制面监控仪表盘。

2. **核心 etcd Grafana 指标**：
   - `etcd_server_has_leader`：是否有 leader（1=正常，0=集群不可用）
   - `etcd_server_leader_changes_seen_total`：leader 变更次数（应该接近 0，频繁变更指示网络问题）
   - `etcd_disk_wal_fsync_duration_seconds_bucket`：WAL 日志写入磁盘延迟（P99 < 25ms 为健康）
   - `etcd_disk_backend_commit_duration_seconds_bucket`：数据库提交延迟（P99 < 50ms 为健康）
   - `etcd_mvcc_db_total_size_in_bytes`：数据库总大小（监控增长趋势，预防磁盘满）
   - `etcd_network_peer_round_trip_time_seconds`：节点间网络延迟

3. **Grafana 告警规则**：基于 etcd 指标的告警是 SRE 的第一道防线。关键告警包括：etcd leader 丢失、磁盘 fsync 延迟 > 25ms（P99）、数据库大小接近 quota（默认 2GB）、etcd member 不可达。这些告警通常设置 1-2 分钟的 for 持续时间避免瞬时抖动误报。

4. **Grafana 自身的 etcd 依赖**：如果 Grafana 部署在集群内部，其 Deployment/ConfigMap/Secret 等配置存储在 etcd 中。etcd 不可用时虽然 Grafana Pod 继续运行（数据面不受影响），但配置更新和 Pod 重建会失败。这就是为什么生产环境推荐将 Grafana 部署在独立于生产集群的监控基础设施上。

## 实践场景

- **etcd 容量预警**：在 Grafana 中监控 `etcd_mvcc_db_total_size_in_bytes` 趋势，接近 2GB quota 时触发告警
- **磁盘 I/O 诊断**：当 apiserver 响应慢时，在 Grafana 中查看 etcd 的 fsync 延迟是否升高，定位是否为磁盘瓶颈
- **Raft 健康监控**：持续监控 leader changes 和 proposal failures，网络分区或多 etcd 节点不一致时第一时间告警
- **灾备验证可视化**：etcd 快照大小和频率在 Grafana 中追踪，验证备份策略执行正常

## 常见问题

### 问题1：Grafana 中 etcd metrics 为空
**症状**：etcd 仪表盘所有面板无数据
**根因**：Prometheus 未配置对 etcd 的 scrape；或 mTLS 证书配置错误；或 managed K8s 不支持直接 scrape etcd
**修复**：检查 Prometheus scrape config 中的 etcd job 配置；确认 CA 证书和 client 证书有效；managed K8s 使用云厂商提供的控制面监控

### 问题2：etcd fsync 延迟间歇性飙高
**症状**：Grafana 显示 fsync 延迟从 5ms 偶尔飙到 100ms+
**根因**：etcd 所在磁盘与其他 I/O 密集型进程共享；或磁盘类型为普通 HDD；或 ext4 文件系统参数未优化
**修复**：为 etcd 使用专用 SSD/NVMe 磁盘；确保 `--data-dir` 独立挂载；调整内核 I/O scheduler

### 问题3：Grafana 告警在 etcd 故障时未触发
**症状**：etcd 宕机但 Grafana 未告警
**根因**：Prometheus 自身也依赖 etcd（通过 apiserver），etcd 宕机可能导致 Prometheus 无法评估告警规则；或 Alertmanager 也在同一集群内不可用
**修复**：部署独立的 Prometheus + Alertmanager（不依赖被监控集群的 etcd）；使用 Dead Man's Switch 告警模式

## 关键命令

```bash
# 🟢 直接查看 etcd metrics（在控制面节点执行）
curl -s --cacert <ca> --cert <cert> --key <key> https://127.0.0.1:2379/metrics | grep etcd_disk

# 🟢 检查 etcd 集群状态
ETCDCTL_API=3 etcdctl --endpoints=<endpoints> endpoint status --write-out=table

# 🟢 查看 etcd 数据库大小
ETCDCTL_API=3 etcdctl --endpoints=<endpoints> endpoint status --write-out=json | jq '.[].Status.dbSize'

# 🟢 检查 Prometheus 是否成功 scrape etcd
kubectl -n monitoring exec prometheus-0 -- wget -qO- 'http://localhost:9090/api/v1/targets' | jq '.data.activeTargets[] | select(.labels.job|test("etcd"))'

# 🟢 查看 Grafana etcd 相关 Dashboard
kubectl get configmap -n monitoring | grep etcd
```

## 权衡取舍

| 维度 | etcd 倾向 | Grafana 倾向 | 权衡点 |
|------|----------|-------------|--------|
| Metrics 暴露 | 限制 scrape 频率减少负载 | 高频采集提升精度 | etcd 性能 vs 监控精度 |
| 告警敏感度 | 低敏感减少噪声 | 高敏感快速发现问题 | 误报率 vs 故障发现速度 |
| 部署位置 | 集群内简化部署 | 集群外保障独立性 | 运维简单 vs 灾备能力 |
| 指标保留 | 短保留减少存储 | 长保留便于趋势分析 | 存储成本 vs 分析能力 |

## 最佳实践
1. 为 etcd 配置专用 Prometheus scrape job，使用 mTLS 证书认证，采集间隔 15-30s
2. 导入社区标准 etcd Grafana Dashboard（如 ID 3070、9733）作为基线，根据集群特点定制
3. 设置 etcd 关键指标告警：fsync P99 > 25ms、leader changes > 0、db size > 1.5GB
4. 将 Grafana 和 Prometheus 部署在独立监控集群中，避免 etcd 故障导致"监控也挂了"

## 工具推荐
- kubectl：基础诊断
- [[实体/helm.md|Helm]]/Kustomize：配置管理
- [[实体/prometheus.md|Prometheus]]/Grafana：联合监控
- [[实体/argocd.md|ArgoCD]]：GitOps同步

## 相关概念
- [[etcd]]
- Grafana
## Related

- [[实体/kubernetes.md|Kubernetes (CNCF Graduated)]]
- [[实体/argo.md|Argo Workflows]]
- [[故障诊断/技能体系/skill-set/k8s-pvc-storage/DIALOGUE.md|DIALOGUE]]
- [[概念/etcd-×-PVC.md|etcd-×-PVC]]
- [[概念/apiserver-×-Pod诊断.md|apiserver-×-Pod诊断]]
- [[概念/etcd-×-灾难恢复.md|etcd-×-灾难恢复]]


<!-- risk-assessed -->
