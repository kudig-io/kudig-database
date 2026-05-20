---
title: Kuberhealthy
description: '## 项目概述'
category: cncf-landscape
tags:
- k8s
- cncf
- cloud-native
- ecosystem
- prometheus
- helm
- daemonset
- job
- crd
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 架构师
- 技术决策者
- SRE
estimated_read_time: 5min
intent_queries:
- Kuberhealthy 是什么
- 如何 Kuberhealthy
- Kubernetes 34 cncf landscape 最佳实践
trigger_keywords:
- Kuberhealthy
- cncf
- landscape
---

# Kuberhealthy

> **成熟度**: Sandbox | **加入时间**: 2021-06 | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官方网站** | https://kuberhealthy.github.io/kuberhealthy |
| **GitHub** | https://github.com/kuberhealthy/kuberhealthy |
| **许可证** | Apache-2.0 |
| **开发语言** | Go |
| **CNCF 分类** | Observability |
| **适用场景** | Kubernetes 综合健康检查 |

---

## 项目概述

Kuberhealthy 是一个 Kubernetes 综合健康检查和合成监控工具。它通过运行 Kubernetes Job 来执行健康检查，将检查结果以 Prometheus 指标格式输出。支持自定义检查，可以验证 DNS、部署、存储、网络等各方面的集群健康状态。

---

## 核心特性

- **合成监控**: 通过 Kubernetes Job 执行主动健康检查
- **丰富检查项**: DNS、Deployment、DaemonSet、Pod 等
- **自定义检查**: 使用任何容器镜像编写自定义检查
- **Prometheus 集成**: 检查结果直接导出为指标
- **CRD 配置**: 使用 KuberhealthyCheck CRD 定义检查
- **多命名空间**: 支持跨命名空间检查
- **告警友好**: 与 Alertmanager 无缝集成

---

## 快速开始

### Helm 安装

```bash
helm repo add kuberhealthy https://kuberhealthy.github.io/kuberhealthy/helm-repos
helm install kuberhealthy kuberhealthy/kuberhealthy \
  --namespace kuberhealthy \
  --create-namespace
```

---

## 内置检查

### DNS 检查

```yaml
apiVersion: comcast.github.io/v1
kind: KuberhealthyCheck
metadata:
  name: dns-status-internal
  namespace: kuberhealthy
spec:
  runInterval: 2m
  timeout: 15m
  podSpec:
    containers:
      - name: dns-check
        image: kuberhealthy/dns-resolution-check:v1.5.0
        env:
          - name: HOSTNAME
            value: "kubernetes.default"
```

### Deployment 检查

```yaml
apiVersion: comcast.github.io/v1
kind: KuberhealthyCheck
metadata:
  name: deployment-check
  namespace: kuberhealthy
spec:
  runInterval: 10m
  timeout: 15m
  podSpec:
    containers:
      - name: deployment-check
        image: kuberhealthy/deployment-check:v1.9.0
        env:
          - name: CHECK_DEPLOYMENT_REPLICAS
            value: "2"
          - name: CHECK_DEPLOYMENT_ROLLING_UPDATE
            value: "true"
```

### DaemonSet 检查

```yaml
apiVersion: comcast.github.io/v1
kind: KuberhealthyCheck
metadata:
  name: daemonset-check
  namespace: kuberhealthy
spec:
  runInterval: 15m
  timeout: 12m
  podSpec:
    containers:
      - name: daemonset-check
        image: kuberhealthy/daemonset-check:v3.3.0
```

---

## Prometheus 指标

| 指标 | 说明 |
|:---|:---|
| `kuberhealthy_check` | 检查通过状态 (0/1) |
| `kuberhealthy_running` | 检查是否在运行 |

### 告警规则

```yaml
groups:
  - name: kuberhealthy
    rules:
      - alert: KuberhealthyCheckFailed
        expr: kuberhealthy_check == 0
        for: 5m
        labels:
          severity: warning
        annotations:
          summary: "Health check {{ $labels.check }} failed"
```

---

## 最佳实践

1. **检查频率**: 关键检查每 2-5 分钟，非关键 10-15 分钟
2. **超时设置**: 合理设置超时避免误报
3. **自定义检查**: 针对业务场景编写自定义检查
4. **告警集成**: 配置 Prometheus 告警规则

---

## 参考资源

- [GitHub Repo](https://github.com/kuberhealthy/kuberhealthy)
- [检查列表](https://github.com/kuberhealthy/kuberhealthy/tree/master/cmd)
- [自定义检查指南](https://github.com/kuberhealthy/kuberhealthy/blob/master/docs/EXTERNAL_CHECK_CREATION.md)

---

**维护者**: Kudig Team | **许可证**: MIT
