---
title: 工单样本去重与差异化审查报告（2026-06-26）
description: 本轮 50 个工单样本的重复主题识别与处理建议
summary: 本轮 50 个工单样本的重复主题识别与处理建议
category: reports
tags:
- ticket-agent
- audit
tier: supporting
created: '2026-06-26'
updated: '2026-06-26'
last_updated: 2026-06-26
relationships:
- target: _reports/ticket-agent-corpus-comprehensive-supplement-summary-2026-06-26.md
  type: related_to
- target: _reports/ticket-agent-corpus-round2-summary-2026-06-26.md
  type: related_to
- target: _reports/recent-wikilink-audit-2026-06-26.md
  type: related_to
---



# 工单样本去重与差异化审查报告

- 总样本数: 50
- 重复主题组: 7
- 重复样本数: 27

## 重复主题组详情

### ingress-controller-404-502

- 样本数量: 8
- 建议保留代表: `ticket-case-021-ingress-controller-pod-abnormal-404-502.md` (TC-2026-021)
- 代表标题: 阿里云专有云 Ingress 控制器 Pod 异常导致业务访问 404/502

| 文件 | incident_id | 优先级 | 字数 | 处理建议 |
|---|---|---|---|---|
| `ticket-case-021-ingress-controller-pod-abnormal-404-502.md` | TC-2026-021 | P0 | 9819 | 保留为代表 |
| `ticket-case-046-ingress-controller-404-502.md` | "INC-2026-ACK-046" | "P0" | 9627 | 标记为 duplicate_of 代表样本 |
| `ticket-case-016-ingress-controller-404-502.md` | "INC-2026-ACK-016" | "P0" | 7733 | 标记为 duplicate_of 代表样本 |
| `ticket-case-031-ingress-controller-oom-502-404.md` | "TC-2026-031" | "P1" | 7727 | 标记为 duplicate_of 代表样本 |
| `ticket-case-036-ingress-controller-pod-exception.md` | "TC-2026-036" | "P0" | 7291 | 标记为 duplicate_of 代表样本 |
| `ticket-case-041-ingress-controller-502.md` | "INC-2026-ACK-041" | "P0" | 7047 | 标记为 duplicate_of 代表样本 |
| `ticket-case-026-ingress-controller-pod-502.md` | "INC-2026-ACK-026" | "P0" | 6892 | 标记为 duplicate_of 代表样本 |
| `ticket-case-011-ingress-controller-pod-404-502.md` | "INC-2026-ACK-011" | "P1" | 6175 | 标记为 duplicate_of 代表样本 |

### pod-pending-resource

- 样本数量: 7
- 建议保留代表: `ticket-case-047-pod-pending-resource-taint-affinity.md` ("INC-2026-ACK-047")
- 代表标题: Pod 持续 Pending：资源不足、Taint 不匹配与亲和性冲突

| 文件 | incident_id | 优先级 | 字数 | 处理建议 |
|---|---|---|---|---|
| `ticket-case-047-pod-pending-resource-taint-affinity.md` | "INC-2026-ACK-047" | "P1" | 8329 | 保留为代表 |
| `ticket-case-022-pod-pending-resource-taint-affinity.md` | TC-2026-022 | P1 | 8228 | 标记为 duplicate_of 代表样本 |
| `ticket-case-027-pod-pending-resource-taint.md` | "INC-2026-ACK-027" | "P1" | 7557 | 标记为 duplicate_of 代表样本 |
| `ticket-case-017-pod-pending-resource-exhaustion.md` | "INC-2026-ACK-017" | "P1" | 6739 | 标记为 duplicate_of 代表样本 |
| `ticket-case-032-pod-pending-resources-taint-affinity.md` | "TC-2026-032" | "P1" | 6675 | 标记为 duplicate_of 代表样本 |
| `ticket-case-042-pod-pending-resource-taint.md` | "INC-2026-ACK-042" | "P1" | 6101 | 标记为 duplicate_of 代表样本 |
| `ticket-case-012-pod-pending-resource-exhaustion.md` | "INC-2026-ACK-012" | "P1" | 5687 | 标记为 duplicate_of 代表样本 |

### statefulset-pvc-unbound

- 样本数量: 6
- 建议保留代表: `ticket-case-048-statefulset-pvc-config-failure.md` ("INC-2026-ACK-048")
- 代表标题: StatefulSet Pod 启动失败：PVC 未绑定与配置错误

| 文件 | incident_id | 优先级 | 字数 | 处理建议 |
|---|---|---|---|---|
| `ticket-case-048-statefulset-pvc-config-failure.md` | "INC-2026-ACK-048" | "P1" | 9278 | 保留为代表 |
| `ticket-case-028-statefulset-pvc-unbound.md` | "INC-2026-ACK-028" | "P1" | 8761 | 标记为 duplicate_of 代表样本 |
| `ticket-case-023-statefulset-pvc-unbound-config-error.md` | TC-2026-023 | P1 | 8308 | 标记为 duplicate_of 代表样本 |
| `ticket-case-033-statefulset-pvc-unbound.md` | "TC-2026-033" | "P1" | 7660 | 标记为 duplicate_of 代表样本 |
| `ticket-case-038-statefulset-pvc-unbound.md` | "TC-2026-038" | "P1" | 7484 | 标记为 duplicate_of 代表样本 |
| `ticket-case-043-statefulset-pvc-unbound.md` | "INC-2026-ACK-043" | "P1" | 6787 | 标记为 duplicate_of 代表样本 |

### node-diskpressure

- 样本数量: 4
- 建议保留代表: `ticket-case-040-node-diskpressure-eviction.md` ("TC-2026-040")
- 代表标题: 节点磁盘压力 DiskPressure 导致 Pod 被驱逐

| 文件 | incident_id | 优先级 | 字数 | 处理建议 |
|---|---|---|---|---|
| `ticket-case-040-node-diskpressure-eviction.md` | "TC-2026-040" | "P0" | 7227 | 保留为代表 |
| `ticket-case-018-node-diskpressure.md` | "INC-2026-ACK-018" | "P1" | 6841 | 标记为 duplicate_of 代表样本 |
| `ticket-case-035-node-diskpressure-eviction.md` | "TC-2026-035" | "P1" | 6835 | 标记为 duplicate_of 代表样本 |
| `ticket-case-014-node-disk-pressure.md` | "INC-2026-ACK-014" | "P0" | 5995 | 标记为 duplicate_of 代表样本 |

### cronjob-job-failure

- 样本数量: 4
- 建议保留代表: `ticket-case-049-job-cronjob-execution-failure.md` ("INC-2026-ACK-049")
- 代表标题: Job/CronJob 执行失败：退避重试耗尽与镜像拉取异常

| 文件 | incident_id | 优先级 | 字数 | 处理建议 |
|---|---|---|---|---|
| `ticket-case-049-job-cronjob-execution-failure.md` | "INC-2026-ACK-049" | "P1" | 9476 | 保留为代表 |
| `ticket-case-024-cronjob-execution-failure.md` | TC-2026-024 | P1 | 8381 | 标记为 duplicate_of 代表样本 |
| `ticket-case-029-cronjob-fail.md` | "INC-2026-ACK-029" | "P1" | 8377 | 标记为 duplicate_of 代表样本 |
| `ticket-case-034-cronjob-stuck-job-skipped-schedule.md` | "TC-2026-034" | "P1" | 6747 | 标记为 duplicate_of 代表样本 |

### daemonset-not-ready

- 样本数量: 3
- 建议保留代表: `ticket-case-050-daemonset-not-running-all-nodes.md` ("INC-2026-ACK-050")
- 代表标题: DaemonSet 未在所有节点运行：日志采集 Agent 缺失

| 文件 | incident_id | 优先级 | 字数 | 处理建议 |
|---|---|---|---|---|
| `ticket-case-050-daemonset-not-running-all-nodes.md` | "INC-2026-ACK-050" | "P1" | 9181 | 保留为代表 |
| `ticket-case-025-daemonset-not-running-on-all-nodes.md` | TC-2026-025 | P1 | 8653 | 标记为 duplicate_of 代表样本 |
| `ticket-case-030-daemonset-not-ready-all-nodes.md` | "INC-2026-ACK-030" | "P1" | 8393 | 标记为 duplicate_of 代表样本 |

### kubeproxy-service-unreachable

- 样本数量: 2
- 建议保留代表: `ticket-case-019-kubeproxy-service-unreachable.md` ("INC-2026-ACK-019")
- 代表标题: Service 访问异常：kube-proxy 未同步 Endpoint 导致 ClusterIP 不通

| 文件 | incident_id | 优先级 | 字数 | 处理建议 |
|---|---|---|---|---|
| `ticket-case-019-kubeproxy-service-unreachable.md` | "INC-2026-ACK-019" | "P0" | 8248 | 保留为代表 |
| `ticket-case-044-kubeproxy-service-unreachable.md` | "INC-2026-ACK-044" | "P0" | 7059 | 标记为 duplicate_of 代表样本 |

## Related

- _reports/ticket-agent-corpus-comprehensive-supplement-summary-2026-06-26.md
- _reports/ticket-agent-corpus-round2-summary-2026-06-26.md
- _reports/recent-wikilink-audit-2026-06-26.md
- _reports/ticket-agent-corpus-round2-summary-2026-06-26.md
- _reports/recent-wikilink-audit-2026-06-26.md
