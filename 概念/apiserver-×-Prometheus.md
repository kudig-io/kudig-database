---
title: apiserver × Prometheus
summary: apiserver × Prometheus：apiserver与Prometheus是Kubernetes生产环境中的两个关键维度。理解它们之间的交互关系对于构建稳定、可观测的集群至关重要。
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
- target: '[[实体/argocd.md]]'
  type: related_to
status: reviewed
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。

# apiserver × Prometheus

## 概述
Prometheus 是 apiserver 最核心的监控数据消费者。apiserver 在 `/metrics` 端点暴露了数百个 Prometheus 格式指标，是 Kubernetes 控制面可观测性的数据基础。同时 Prometheus 自身的 Service Discovery 也依赖 apiserver 来发现监控目标。这两者之间的双向依赖（Prometheus 消费 apiserver 指标 + Prometheus 通过 apiserver 发现目标）构成了 Kubernetes 可观测性的基石。

## 技术关联机制

1. **Prometheus scrape apiserver**：Prometheus 通过 Kubernetes SD（Service Discovery）配置，使用 apiserver 的 `in-cluster` ServiceAccount 凭据直接访问 `https://kubernetes.default.svc:443/metrics`。这条连接使用 HTTPS + Client Certificate（或 Bearer Token）认证。Prometheus 以固定间隔（通常 30s）发起 scrape，每次请求拉取全量指标数据（可能数十 MB）。在大规模集群中，这个请求的响应时间和数据量对 apiserver 造成持续负载。

2. **关键 apiserver 指标**：Prometheus 从 apiserver 采集的核心指标包括：`apiserver_request_total`（按 verb/resource/code 分类的请求计数）、`apiserver_request_duration_seconds`（请求延迟直方图）、`apiserver_current_inflight_requests`（当前并发请求数）、`etcd_request_duration_seconds`（etcd 操作延迟）、`apiserver_storage_objects`（各资源类型在 etcd 中的对象数量）、`apiserver_admission_webhook_admission_duration_seconds`（准入控制器延迟）。这些指标是容量规划和告警的基础。

3. **Prometheus 自身的 apiserver 依赖**：Prometheus 的 Kubernetes SD 模式通过 List+Watch apiserver 来发现 Pod、Service、Endpoint 等资源作为监控目标。Prometheus 需要一个拥有 `get/list/watch` 权限的 ClusterRole 来访问集群资源。如果 apiserver 不可用，Prometheus 无法发现新目标，也无法采集已有目标的指标，监控能力全面降级。

4. **Recording Rules 与 apiserver 告警**：生产环境通常配置 Prometheus Recording Rules 预计算 apiserver 的核心指标（如 P99 延迟、5xx 错误率），并设置 Alerting Rules 在 apiserver 异常时触发告警。常见的告警规则包括：`apiserver_request_5xx_rate > 0.01`、`apiserver_request_duration_P99 > 1s`、`up{job="kubernetes-apiservers"} == 0`。

## 实践场景

- **apiserver 容量预警**：监控 `apiserver_current_inflight_requests` 趋势，接近 `--max-requests-inflight` 阈值时提前扩容控制面
- **发布影响分析**：在 Deployment 滚动更新期间观察 apiserver 的请求 QPS 和延迟变化，评估发布对控制面的影响
- **API 错误率告警**：配置 Prometheus Alertmanager 在 apiserver 5xx 错误率超过阈值时自动告警，触发自动化回滚
- **审计准入控制器性能**：通过 `apiserver_admission_webhook_admission_duration_seconds` 指标识别慢 webhook，优化部署延迟

## 常见问题

### 问题1：Prometheus 无法采集 apiserver 指标
**症状**：Prometheus targets 页面显示 apiserver target 为 DOWN
**根因**：Prometheus 的 SA 缺少对 `/metrics` 的访问权限；或证书/TLS 配置错误
**修复**：检查 ClusterRole/ClusterRoleBinding 是否授予了 `nonResourceURLs: ["/metrics"]` 的 get 权限；确认 TLS 证书未过期

### 问题2：apiserver metrics 数据量过大导致 Prometheus 内存溢出
**症状**：Prometheus OOMKilled，重启后再次 OOM
**根因**：大规模集群中 apiserver 指标的 cardinality 过高（如按 namespace+verb+resource 分类的 request_total 指标可能数万条）
**修复**：使用 Prometheus 的 `metric_relabel_configs` 过滤不必要的标签；调整 scrape interval 至 60s；增加 Prometheus 内存

### 问题3：apiserver scrape 请求自身加剧 apiserver 负载
**症状**：开启 Prometheus 后 apiserver 延迟升高
**根因**：Prometheus 的 scrape 请求产生大量 `/metrics` GET 请求，在高负载集群中雪上加霜
**修复**：适当增大 scrape_interval（如 60s）；考虑使用 Pushgateway 或独立 metrics 聚合层减轻直连压力

## 关键命令

```bash
# 🟢 检查 Prometheus 是否成功 scrape apiserver
kubectl -n monitoring exec prometheus-0 -- wget -qO- http://localhost:9090/api/v1/targets | jq '.data.activeTargets[] | select(.labels.job=="kubernetes-apiservers")'

# 🟢 直接查看 apiserver 指标
kubectl get --raw /metrics | grep apiserver_request_duration

# 🟢 检查 Prometheus SA 权限
kubectl get clusterrolebinding -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{.subjects[*].kind}{"\n"}{end}' | grep -i prometheus

# 🟢 查看特定资源在 etcd 中的对象数量
kubectl get --raw /metrics | grep apiserver_storage_objects
```

## 权衡取舍

| 维度 | apiserver 倾向 | Prometheus 倾向 | 权衡点 |
|------|---------------|----------------|--------|
| Scrape 频率 | 低频减少负载 | 高频提升精度 | apiserver 压力 vs 监控粒度 |
| 指标 cardinality | 低基数减少开销 | 高基数精细分析 | 内存消耗 vs 分析能力 |
| SD 依赖 | 减少 List/Watch 负载 | 实时发现监控目标 | apiserver 负载 vs 自动化 |
| 告警敏感度 | 低敏感减少噪声 | 高敏感快速响应 | 误报率 vs 响应速度 |

## 最佳实践
1. 为 Prometheus 配置专用 ServiceAccount，授予对 `/metrics` 和 Kubernetes SD 所需资源的最小权限
2. 使用 `metric_relabel_configs` 过滤高 cardinality 指标的非必要标签，控制 Prometheus 内存使用
3. 配置 apiserver 延迟 P99 和 5xx 错误率的 Alerting Rules，并接入 Alertmanager 通知链路
4. 在大规模集群（>500 节点）考虑使用 Thanos 或 VictoriaMetrics 分散指标存储和查询负载

## 工具推荐
- kubectl：基础诊断
- [[实体/helm.md|Helm]]/Kustomize：配置管理
- Prometheus/Grafana：联合监控
- [[实体/argocd.md|ArgoCD]]：GitOps同步

## 相关概念
- apiserver
- [[Prometheus]]
## Related

- [[实体/kubernetes.md|Kubernetes (CNCF Graduated)]]
- [[实体/argo.md|Argo Workflows]]
- [[系统基础/速查卡/git.md|Git 速查卡]]
- [[故障诊断/技能体系/skill-set/k8s-pvc-storage/DIALOGUE.md|DIALOGUE]]
- [[概念/etcd-×-PVC.md|etcd-×-PVC]]
- [[概念/apiserver-×-Pod诊断.md|apiserver-×-Pod诊断]]
- [[概念/etcd-×-灾难恢复.md|etcd-×-灾难恢复]]


<!-- risk-assessed -->
