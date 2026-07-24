---
title: apiserver × Grafana
summary: apiserver × Grafana：apiserver与Grafana是Kubernetes生产环境中的两个关键维度。理解它们之间的交互关系对于构建稳定、可观测的集群至关重要。
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

# apiserver × Grafana

## 概述
Grafana 本身不直接与 apiserver 强交互——它的核心数据源是 Prometheus。但 apiserver 的健康指标（如请求延迟、错误率、etcd 请求耗时）是 Grafana 控制面仪表盘的核心数据。同时，如果 Grafana 部署在集群内，其自身的部署、Service、Ingress 配置也要通过 apiserver 管理。理解这条间接链路有助于构建完整的 apiserver 可观测性方案。

## 技术关联机制

1. **apiserver 指标暴露**：apiserver 在 `/metrics` 端点以 Prometheus 格式暴露了数百个指标，包括 `apiserver_request_total`、`apiserver_request_duration_seconds_bucket`、`etcd_request_duration_seconds_bucket`、`apiserver_current_inflight_requests` 等。Prometheus 通过 ServiceMonitor 或 PodMonitor 配置 scrape apiserver 的 metrics 端点（通常通过 HTTPS + RBAC 认证），这些指标随后在 Grafana 中可视化。

2. **kube-apiserver 仪表盘**：社区标准的 Grafana 仪表盘（如 Grafana Dashboard ID 15760、12006）直接消费 apiserver 指标来展示：每秒请求数（QPS）、请求延迟分位数（P50/P90/P99）、按 verb+resource 分类的错误率、inflight 请求并发数、watch cache 命中率等。这些仪表盘是 apiserver 容量规划和故障排查的第一入口。

3. **Grafana 的集群内部署依赖 apiserver**：Grafana 通常以 Deployment 形式部署在集群中，其 ConfigMap（仪表盘配置）、Secret（数据源凭证）、Service、Ingress 均通过 apiserver 管理。apiserver 不可用时，Grafana 的配置更新和扩缩容操作都会失败。

4. **apiserver Audit Log → Grafana (Loki)**：部分生产环境将 apiserver 的 audit log 通过 Fluentbit/Promtail 发送到 Loki，在 Grafana 中查询。这形成了一条 "apiserver → audit → Grafana" 的可观测性链路，用于追踪谁在什么时间对什么资源做了什么操作。

## 实践场景

- **apiserver 容量规划**：通过 Grafana 仪表盘观察 apiserver QPS 趋势，决定是否需要水平扩展控制面节点或调整 `--max-requests-inflight`
- **发布期间监控**：大规模滚动更新时，在 Grafana 中实时观察 apiserver 的 inflight 请求和延迟，判断是否触发了 APF 限流
- **审计可视化**：将 apiserver audit log 导入 Grafana Loki，构建 "API 操作热力图" 识别异常调用模式（如某 SA 大量 delete 操作）
- **Grafana 自身运维**：Grafana 的 Provisioning 配置（数据源、仪表盘）通过 ConfigMap 注入，apiserver 异常时 ConfigMap 更新无法生效

## 常见问题

### 问题1：Grafana 仪表盘显示 apiserver 指标为空
**症状**：kube-apiserver 仪表盘所有面板无数据
**根因**：Prometheus 未正确配置对 apiserver `/metrics` 的 scrape；或 RBAC 权限不足导致 Prometheus 无法访问
**修复**：检查 Prometheus 的 ServiceMonitor 配置；确认 kube-system 中 Prometheus SA 拥有 `get /metrics` 的 ClusterRole 权限

### 问题2：Grafana 自身因 apiserver 不可用而无法访问
**症状**：Grafana UI 返回 502/503，或 Ingress 无法路由
**根因**：apiserver 宕机导致 Grafana Pod 无法被 kube-proxy 正确 endpoints 同步；或 Grafana Deployment 正在滚动更新但 apiserver 异常导致更新卡住
**修复**：优先恢复 apiserver（检查 etcd 健康）；在独立监控集群或物理机上部署 Grafana 作为灾备

### 问题3：Audit Log 在 Grafana Loki 中查询不到
**症状**：Loki 数据源正常但查询 audit log 为空
**根因**：apiserver 的 audit policy 未配置将事件发送到指定后端；或 Promtail/Fluentbit 采集配置错误
**修复**：检查 `--audit-policy-file` 配置；确认 audit log 后端（log file 或 webhook）正常工作

## 关键命令

```bash
# 🟢 查看 apiserver metrics 端点是否正常
kubectl get --raw /metrics | grep apiserver_request_total

# 🟢 查看 Grafana Deployment 状态
kubectl get deployment -n monitoring grafana

# 🟢 检查 Prometheus 对 apiserver 的 scrape 配置
kubectl get servicemonitor -n monitoring | grep apiserver

# 🟢 查看 apiserver audit policy
kubectl -n kube-system exec kube-apiserver-<node> -- cat /etc/kubernetes/audit/policy.yaml

# 🟡 重启 Grafana 以刷新配置
kubectl rollout restart deployment grafana -n monitoring
```

## 权衡取舍

| 维度 | apiserver 倾向 | Grafana 倾向 | 权衡点 |
|------|---------------|-------------|--------|
| Metrics 粒度 | 粗粒度减少采集开销 | 细粒度提升可视化精度 | apiserver 负载 vs 可观测性 |
| 审计日志级别 | 低级别减少 I/O | 高级别完整审计追踪 | 磁盘 I/O vs 安全合规 |
| 部署位置 | 集群内简化管理 | 集群外保障独立性 | 统一管理 vs 灾备隔离 |
| Dashboard 数量 | 少指标低负载 | 多面板全面覆盖 | 查询负载 vs 监控覆盖面 |

## 最佳实践
1. 在独立于生产集群的监控基础设施上部署 Grafana，避免 apiserver 故障时 "监控也挂了"
2. 配置 apiserver Audit Policy 时区分 LogLevel：Metadata 级别用于日常审计，RequestResponse 级别仅用于敏感资源
3. 使用 Grafana Alerting 基于 apiserver P99 延迟和 5xx 错误率设置告警
4. 定期导出和备份 Grafana Dashboard JSON 到 Git，防止 apiserver/etcd 故障导致仪表盘丢失

## 工具推荐
- kubectl：基础诊断
- [[实体/helm.md|Helm]]/Kustomize：配置管理
- [[实体/prometheus.md|Prometheus]]/Grafana：联合监控
- [[实体/argocd.md|ArgoCD]]：GitOps同步

## 相关概念
- apiserver
- Grafana
## Related

- [[实体/kubernetes.md|Kubernetes (CNCF Graduated)]]
- [[实体/argo.md|Argo Workflows]]
- [[故障诊断/技能体系/skill-set/k8s-pvc-storage/DIALOGUE.md|DIALOGUE]]
- [[概念/etcd-×-PVC.md|etcd-×-PVC]]
- [[概念/apiserver-×-Pod诊断.md|apiserver-×-Pod诊断]]
- [[概念/etcd-×-灾难恢复.md|etcd-×-灾难恢复]]


<!-- risk-assessed -->
