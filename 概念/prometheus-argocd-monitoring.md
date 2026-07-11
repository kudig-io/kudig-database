---
title: Prometheus 与 ArgoCD 监控集成
summary: 'Prometheus 与 ArgoCD 监控集成：1. 监控 ArgoCD 本身: ArgoCD 暴露 Prometheus metrics 端点
  2. 通过 ArgoCD 部署 Prometheus: 使用 GitOps 管理监控栈 3. 应用级监控: ArgoCD 同步后自动发现 ServiceMonitor'
category: synthesis
tags:
- synthesis
- prometheus
- argocd
- monitoring
tier: supporting
sources: []
created: 2026-05-24
updated: 2026-07
last_updated: 2026-07
---



# Prometheus 与 ArgoCD 监控集成

> Prometheus 监控体系与 ArgoCD GitOps 部署的集成方案和监控策略。

## 概述

Prometheus 与 ArgoCD 的集成覆盖三个维度：监控 ArgoCD 自身健康、通过 GitOps 管理 Prometheus 监控栈、以及利用 ArgoCD 的同步事件驱动应用级监控发现。三者结合形成"监控即代码"的闭环体系。

## 集成维度

1. **监控 ArgoCD 本身**: ArgoCD 暴露 Prometheus metrics 端点，关注同步状态、控制器性能
2. **通过 ArgoCD 部署 Prometheus**: 使用 GitOps 管理 kube-prometheus-stack，实现监控基础设施的声明式管理
3. **应用级监控**: ArgoCD 同步后自动发现 ServiceMonitor，实现应用部署与监控配置的原子性

## ArgoCD 自身指标监控

### 关键指标

ArgoCD 的 Application Controller、API Server、Repo Server 和 Server 组件均暴露 Prometheus 指标：

| 指标 | 说明 | 告警阈值 |
|------|------|----------|
| `argocd_app_info` | 应用状态（Synced/OutOfSync/Healthy） | phase != Healthy |
| `argocd_app_sync_status` | 同步状态 | status != Synced 持续 > 5min |
| `argocd_app_reconcile` | 应用调谐耗时 | p99 > 30s |
| `argocd_cluster_api_resource_objects` | 集群 API 资源对象数 | 趋势异常增长 |
| `argocd_git_request_total` | Git 请求总数 | 错误率 > 5% |
| `argocd_repo_pending_request_total` | Repo Server 积压请求 | > 10 |

### 告警规则示例

```yaml
# PrometheusRule: ArgoCD 健康监控
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: argocd-alerts
  namespace: argocd
spec:
  groups:
    - name: argocd
      rules:
        # 应用不同步告警
        - alert: ArgoCDAppOutOfSync
          expr: argocd_app_info{sync_status!="Synced"} == 1
          for: 10m
          labels:
            severity: warning
          annotations:
            summary: "应用 {{ $labels.name }} 已不同步超过 10 分钟"

        # 同步失败告警
        - alert: ArgoCDAppSyncFailed
          expr: argocd_app_sync_status{status!="Synced"} == 1
          for: 5m
          labels:
            severity: critical

        # 控制器调谐过慢
        - alert: ArgoCDReconcileSlow
          expr: histogram_quantile(0.99, rate(argocd_app_reconcile_bucket[5m])) > 30
          labels:
            severity: warning
```

## 监控栈 GitOps 部署

### 通过 ArgoCD 管理 Prometheus Operator

使用 ArgoCD Application 管理 kube-prometheus-stack，实现监控基础设施的声明式部署：

```yaml
apiVersion: argoproj.io/v1alpha1
kind: Application
metadata:
  name: monitoring-stack
  namespace: argocd
spec:
  source:
    repoURL: https://github.com/prometheus-community/helm-charts
    chart: kube-prometheus-stack
    targetRevision: "58.0.0"
    helm:
      values: |
        prometheus:
          prometheusSpec:
            retention: 30d
            storageSpec:
              volumeClaimTemplate:
                spec:
                  storageClassName: fast-ssd
                  resources:
                    requests:
                      storage: 200Gi
        alertmanager:
          config:
            receivers:
              - name: oncall-team
                webhook_configs:
                  - url: http://alert-router:9093/alert
  destination:
    namespace: monitoring
    server: https://kubernetes.default.svc
  syncPolicy:
    automated:
      prune: true
      selfHeal: true
```

### ServiceMonitor 自动发现链路

```
开发者提交代码 → CI 构建镜像 → Git 仓库更新 Helm values
                                        ↓
                              ArgoCD 检测变更
                                        ↓
                              同步 Deployment + ServiceMonitor
                                        ↓
                              Prometheus Operator 发现新 ServiceMonitor
                                        ↓
                              Prometheus 自动添加 scrape target
```

这实现了**应用部署与监控配置的原子性**——新服务上线时监控自动生效，下线时监控自动移除。

## 最佳实践

- **监控 ArgoCD 控制器性能**：Application Controller 的调谐延迟直接影响 GitOps 流水线响应速度，配置 `argocd_app_reconcile` 的 P99 告警
- **为应用同步配置告警**：OutOfSync 持续超过 10 分钟通常意味着配置漂移或同步错误，需要人工介入
- **使用 PrometheusRule 管理 ArgoCD 告警**：通过 GitOps 管理告警规则本身，而非在 Grafana 或 Alertmanager 中手动配置
- **关注 Repo Server 性能**：大型 monorepo 场景下 Repo Server 容易成为瓶颈，监控 `argocd_repo_pending_request_total`
- **配置 ArgoCD Notifications 与 Prometheus 联动**：同步失败时自动通知 On-call 团队

## 常见陷阱

- **ServiceMonitor 未被 Prometheus 选中**：检查 ServiceMonitor 的 `namespaceSelector` 和 `labelSelector` 是否与 Prometheus CR 的选择器匹配，以及 Namespace 是否有 `monitoring: enabled` 标签
- **ArgoCD 同步状态被 ArgoCD 自身监控干扰**：如果监控栈的 ServiceMonitor 由 ArgoCD 管理，ArgoCD 不同步会导致监控失效——形成"鸡蛋问题"，建议核心监控用独立部署
- **大量 Application 导致 Prometheus 指标爆炸**：数百个 Application 会产生大量 `argocd_app_*` 时间序列，需要合理设置 `metric_relabel_configs` 过滤无用标签

## 相关页面

- [[prometheus]] — Prometheus 监控体系
- [[argocd]] — ArgoCD 持续部署
- [[概念/slo-monitoring-integration.md|SLO 与监控集成]] — SLO 驱动的告警
- [[概念/helm-argocd-gitops.md|Helm 与 ArgoCD GitOps]] — GitOps 工作流
- [[observability]] — 可观测性架构
