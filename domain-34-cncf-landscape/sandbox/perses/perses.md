# Perses

> **成熟度**: Sandbox | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官网** | https://perses.dev/ |
| **GitHub** | https://github.com/perses/perses |
| **许可证** | Apache-2.0 |
| **开发语言** | Go, TypeScript |
| **CNCF 状态** | Sandbox |

---

## 项目概述

Perses 是一个云原生的 Dashboard 即代码 (Dashboard-as-Code) 可视化平台，用于创建和管理可观测性仪表板。它旨在成为 Grafana 的开源替代方案之一，提供标准化的 Dashboard 定义规范，支持将仪表板作为代码进行版本控制和 GitOps 管理。

### 核心特性

- **Dashboard as Code**: 仪表板以 JSON/YAML 代码形式定义和管理
- **GitOps 就绪**: 仪表板定义可纳入 Git 版本控制
- **标准化规范**: 提供 Dashboard 定义的标准格式和 JSON Schema
- **多数据源**: Prometheus 原生支持，可扩展其他数据源
- **Kubernetes CRD**: 通过 PersesDashboard CRD 在 K8s 中管理仪表板
- **插件系统**: 可扩展的面板类型和数据源插件
- **协作编辑**: 支持多用户协作编辑仪表板
- **嵌入式**: 仪表板可嵌入到其他 Web 应用中

---

## 快速开始

### 安装

```bash
# Docker 运行
docker run -d --name perses -p 8080:8080 \
  persesdev/perses:latest

# Helm 安装
helm repo add perses https://perses.github.io/helm-charts
helm install perses perses/perses \
  --namespace perses \
  --create-namespace
```

### Dashboard 定义

```yaml
kind: Dashboard
metadata:
  name: kubernetes-overview
  project: monitoring
spec:
  display:
    name: "Kubernetes Overview"
  duration: "1h"
  refreshInterval: "30s"
  variables:
    - kind: ListVariable
      spec:
        display:
          name: "Namespace"
        plugin:
          kind: PrometheusLabelValuesVariable
          spec:
            datasource:
              kind: PrometheusDatasource
              name: prometheus
            labelName: namespace
  panels:
    cpu_usage:
      kind: Panel
      spec:
        display:
          name: "CPU Usage"
        plugin:
          kind: TimeSeriesChart
          spec: {}
        queries:
          - kind: TimeSeriesQuery
            spec:
              plugin:
                kind: PrometheusTimeSeriesQuery
                spec:
                  query: |
                    sum(rate(container_cpu_usage_seconds_total{namespace="$namespace"}[5m])) by (pod)
                  datasource:
                    kind: PrometheusDatasource
                    name: prometheus
    memory_usage:
      kind: Panel
      spec:
        display:
          name: "Memory Usage"
        plugin:
          kind: TimeSeriesChart
          spec: {}
        queries:
          - kind: TimeSeriesQuery
            spec:
              plugin:
                kind: PrometheusTimeSeriesQuery
                spec:
                  query: |
                    sum(container_memory_working_set_bytes{namespace="$namespace"}) by (pod)
  layouts:
    - kind: Grid
      spec:
        items:
          - x: 0
            y: 0
            width: 12
            height: 8
            content:
              "$ref": "#/spec/panels/cpu_usage"
          - x: 12
            y: 0
            width: 12
            height: 8
            content:
              "$ref": "#/spec/panels/memory_usage"
```

### Kubernetes CRD 部署

```yaml
apiVersion: perses.dev/v1alpha1
kind: PersesDashboard
metadata:
  name: kubernetes-overview
  namespace: monitoring
spec:
  # 与上面的 Dashboard 定义相同
  display:
    name: "Kubernetes Overview"
  # ...
```

---

## 与 Grafana 对比

| 特性 | Perses | Grafana |
|:---|:---|:---|
| **许可证** | Apache-2.0 | AGPL-3.0 |
| **Dashboard as Code** | 原生 | Provisioning |
| **标准化格式** | JSON Schema | 私有格式 |
| **K8s CRD** | 原生支持 | 需要 Operator |
| **嵌入式** | 原生支持 | 仅企业版 |
| **插件** | 可扩展 | 丰富生态 |

---

## 最佳实践

1. **代码化管理**: 将 Dashboard 定义存储在 Git 仓库，通过 CI/CD 部署
2. **变量化**: 使用变量实现 Dashboard 的环境通用性
3. **标准化**: 团队统一使用 Perses 定义规范，确保仪表板一致性
4. **CRD 集成**: 在 Kubernetes 中使用 PersesDashboard CRD 实现 GitOps
5. **数据源配置**: 集中管理 Prometheus 数据源配置

---

## 参考资源

- [Perses 官方文档](https://perses.dev/docs/)
- [Perses GitHub](https://github.com/perses/perses)
- [CNCF Sandbox Projects](https://www.cncf.io/sandbox-projects/)

---

**维护者**: Kudig Team | **许可证**: MIT
