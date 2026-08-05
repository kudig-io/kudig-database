---
title: 可观测性体系最佳实践
description: 大规模 Kubernetes 集群可观测性体系建设：监控分层、控制面关键指标、日志与事件治理、告警分级与 SLO 告警、链路追踪、大规模 Prometheus 基数治理
summary: 覆盖监控四层模型、控制面黄金指标、日志采集规范、事件外送、告警风暴治理、burn rate 告警与大规模 TSDB 选型
category: references
tags:
- k8s
- observability
- monitoring
- prometheus
- alerting
- production
tier: core
created: '2026-08-03'
last_updated: '2026-08-03'
difficulty: advanced
audience:
- SRE
- 平台工程师
estimated_read_time: 20min
---

# 可观测性体系最佳实践

> 大规模集群的故障特征：影响面大、定位链路长、告警量爆炸。可观测性体系的目标不是"什么都采"，而是 **故障时 5 分钟内定位到层（控制面/节点/网络/存储/业务/依赖）**。

## 1. 监控分层模型

| 层 | 对象 | 核心指标 | 工具 |
|---|---|---|---|
| L1 控制面 | apiserver / etcd / scheduler / controller-manager | 见第 2 节 | Prometheus + 官方 mixin |
| L2 节点 | OS / 容器运行时 / kubelet | node-exporter 全集 + conntrack + inode + 磁盘 IO | node-exporter |
| L3 集群组件 | CoreDNS / Ingress / CSI / CNI | 各组件黄金指标 | 组件自带 /metrics |
| L4 业务 | 应用 Pod | RED（Rate/Error/Duration）或 USE | Prometheus / APM |
| L5 依赖 | DB / MQ / 缓存 / 云服务 | 中间件 exporter + 云监控 | 各 exporter |

> 原则：**故障定位从上往下切，容量规划从下往上看**。

## 2. 控制面黄金指标（大规模必配告警）

### 2.1 kube-apiserver

| 指标 | 告警参考 |
|---|---|
| `apiserver_request_duration_seconds` P99 | > 1s（对齐官方可伸缩性 SLO：99% API 调用 < 1s） |
| `apiserver_request_total{code=~"5.."}` 错误率 | > 1% 持续 5min |
| `apiserver_flowcontrol_rejected_requests_total` | 持续非零（APF 队列配置不合理） |
| `apiserver_longrunning_requests` | watch 连接数异常增长 |
| `apiserver_current_inflight_requests` | 逼近 max-requests-inflight 上限 |
| `etcd_request_duration_seconds` P99 | > 500ms 关注，> 1s 严重 |

### 2.2 etcd

| 指标 | 告警参考 |
|---|---|
| `etcd_disk_wal_fsync_duration_seconds` P99 | > 10ms（磁盘性能不足的直接信号） |
| `etcd_server_leader_changes_seen_total` | 1 天内 > 3 次 |
| `etcd_mvcc_db_total_size_in_bytes` / quota | > 70% |
| `etcd_server_has_leader` | = 0 即集群不可用，最高级告警 |
| `etcd_network_peer_round_trip_time_seconds` | 跨 AZ 网络质量 |

### 2.3 scheduler / controller-manager / kubelet

- `scheduler_scheduling_attempt_duration_seconds` P99 > 100ms → 调度吞吐瓶颈
- `scheduler_pending_pods` 持续积压 → 容量不足或调度器过载
- `workqueue_depth`（各 controller）持续上升 → reconcile 跟不上
- `kubelet_running_pods` 逼近 maxPods；`kubelet_evictions_total` 非零 → 节点压力

### 2.4 CoreDNS / NodeLocal

- `coredns_dns_request_duration_seconds` P99、NXDOMAIN/SERVFAIL 率
- NodeLocal：缓存命中率、`node_dns_hijacked_requests_total`

## 3. 日志体系

### 3.1 规范

- 应用日志**只写 stdout/stderr**，JSON 结构化，带 traceId；禁止写容器内文件（重启即丢）或随意 hostPath
- DaemonSet 采集（fluent-bit/vector/iLogtail），设置资源 requests/limits——采集 Agent 失控是大集群常见雪崩源
- 分级存储：热日志（3–7 天）ES/Loki，温日志（30–90 天）对象存储索引，冷日志（合规留存）归档

### 3.2 控制面与审计日志

- apiserver/controller/scheduler 日志必须采集（托管集群在控制台开启 control plane logging）——**这是事故复盘的第一手材料，成本极低，收益极高**
- 审计日志外送异地留存，防篡改（见 [[08-security-defense-checklist#6. 审计与溯源]]）

### 3.3 事件（Event）治理

- kubelet `eventRecordQPS` 限流（见 [[02-cluster-configuration]]）
- event-exporter 外送所有 Warning 事件到告警通道：OOMKilled、FailedScheduling、Unhealthy、BackOff 是最有价值的事件源
- 大集群下 `kubectl get events` 直查 etcd 既慢又伤控制面，一律走外送平台

## 4. 告警体系设计

### 4.1 告警分级与路由

| 级别 | 定义 | 通道 | 响应 SLA |
|---|---|---|---|
| P1 | 集群/核心业务不可用 | 电话 + IM | 5 分钟 |
| P2 | 功能受损有冗余兜底 | IM | 30 分钟 |
| P3 | 潜在风险/容量预警 | 工单 | 24 小时 |

### 4.2 防告警风暴（大规模关键）

- 单故障源告警收敛：Alertmanager `group_by` + inhibit 规则（节点 NotReady 抑制该节点所有 Pod 告警）
- 告警评审制度：新告警上线必须给出 Runbook 链接与处置动作，**没有处置动作的告警不上线**
- 每周告警回顾：触发次数 Top 10 的告警要么修复根因，要么调整阈值——告警疲劳比没有告警更危险

### 4.3 SLO 告警（burn rate）

业务告警从"阈值告警"升级为"错误预算消耗速率告警"：

```yaml
# 示例：1 小时窗口 fast burn（预算 2% 在 1 小时内烧完即 P1）
- alert: HighErrorRateFastBurn
  expr: |
    sum(rate(http_requests_total{code=~"5.."}[1h]))
      / sum(rate(http_requests_total[1h])) > 14.4 * 0.001
  for: 2m
```

详见 [[15-slo-chaos-engineering]]。

## 5. 链路追踪（Tracing）

- 核心调用链接入 OpenTelemetry，采样率按服务分级（核心 100%，边缘 1–10%）
- 大规模注意：Jaeger/Tempo 后端的存储成本随规模爆炸，必须配置 span 采样 + tail-based sampling
- traceId 贯通日志是排障效率的乘数项

## 6. 大规模 Prometheus 治理

| 问题 | 对策 |
|---|---|
| 基数（cardinality）爆炸 | 禁用高基数 label（pod_name、container_id、url 全路径）；每服务指标 budget 制度 |
| 单点容量不足 | 分片（按 namespace/服务 hash）或换用 VictoriaMetrics / Mimir / Thanos |
| 长期存储 | Thanos/Mimir + 对象存储降采样（5m:30d，1h:1y） |
| 多集群统一视图 | 每层集群 local Prometheus + 中心化查询层（Thanos Query / Grafana Mimir） |
| 指标缺失发现晚 | 对监控自身做元监控（target down 率、采集延迟） |

## 7. 上线验收标准

- [ ] 第 2 节控制面指标全部有 Dashboard + 告警
- [ ] 任一 P1 告警触发，值班人能在 5 分钟内从 Dashboard 定位到故障层
- [ ] 告警演练：注入测试故障，验证通知链路、升级机制、Runbook 有效性
- [ ] 监控自身高可用：Prometheus 双副本，监控集群与业务集群故障域隔离（监控不能死在它要监控的东西上）

## Related

- [[07-pre-production-checklist|生产上线前检查项（监控告警闭环）]]
- [[15-slo-chaos-engineering|SLO 体系与混沌工程]]
- [[02-cluster-configuration|集群配置最佳实践（事件治理）]]
- [[20-最佳实践/07-scenarios/monitoring-alerting|监控告警场景]]
