---
title: "Prometheus 与 ArgoCD 监控集成"
category: synthesis
tags: [synthesis, prometheus, argocd, monitoring]
sources: []
created: 2026-05-24
updated: 2026-05-24
last_updated: 2026-05-24
---

# Prometheus 与 ArgoCD 监控集成

> Prometheus 监控体系与 ArgoCD GitOps 部署的集成方案和监控策略。

## 集成维度

1. **监控 ArgoCD 本身**: ArgoCD 暴露 Prometheus metrics 端点
2. **通过 ArgoCD 部署 Prometheus**: 使用 GitOps 管理监控栈
3. **应用级监控**: ArgoCD 同步后自动发现 ServiceMonitor

## ArgoCD Metrics

ArgoCD Controller 和 Server 暴露以下关键指标:
- `argocd_app_info`: 应用状态
- `argocd_app_reconcile`: 同步耗时
- `argocd_cluster_api_*`: 集群 API 调用

## 监控栈 GitOps 部署

使用 ArgoCD Application 管理 Prometheus Operator + Grafana:
- ServiceMonitor CRD 自动发现
- PrometheusRule CRD 管理告警规则
- Grafana Dashboard ConfigMap 自动加载

## 相关页面

- [[prometheus]] — Prometheus 监控体系
- [[argocd]] — ArgoCD 持续部署
- [[observability]] — 可观测性架构
