---
title: OpenCost
description: '## 项目概述'
category: cncf-landscape
tags:
- k8s
- cncf
- cloud-native
- ecosystem
- prometheus
- grafana
- helm
- vpa
- job
- gpu
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 架构师
- 技术决策者
- SRE
estimated_read_time: 5min
intent_queries:
- OpenCost 是什么
- 如何 OpenCost
- Kubernetes 34 cncf landscape 最佳实践
trigger_keywords:
- OpenCost
- cncf
- landscape
---


# OpenCost

> **成熟度**: Incubating | **加入时间**: 2022-06 | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官方网站** | https://opencost.io |
| **GitHub** | https://github.com/opencost/opencost |
| **许可证** | Apache-2.0 |
| **主要语言** | Go |
| **CNCF 分类** | Observability & FinOps |

---

## 项目概述

OpenCost 是 Kubernetes 成本监控的开源标准。它提供实时成本分配、多维度成本分析和优化建议，帮助团队了解和优化 Kubernetes 基础设施支出。

## 核心特性

- **实时成本监控**: 分钟级别的成本数据采集
- **多维度分析**: 按命名空间、标签、服务、团队分析成本
- **多云支持**: AWS、Azure、GCP、私有云定价集成
- **Prometheus 集成**: 以 Prometheus 指标格式暴露数据
- **闲置资源检测**: 识别未使用的资源
- **成本分配**: 支持共享资源成本分摊

---

## 架构设计

```
┌─────────────────────────────────────────────────────────────────┐
│                     OpenCost Architecture                        │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  ┌────────────────────────────────────────────────────────────┐ │
│  │                    Data Collection                         │ │
│  │                                                            │ │
│  │  ┌─────────────────┐    ┌─────────────────────────────┐  │ │
│  │  │  Kubernetes API │    │    Cloud Pricing APIs       │  │ │
│  │  │  (Nodes, Pods,  │    │  (AWS/Azure/GCP Cost Data)  │  │ │
│  │  │   Resources)    │    │                             │  │ │
│  │  └────────┬────────┘    └────────────┬────────────────┘  │ │
│  │           │                          │                    │ │
│  │           └──────────┬───────────────┘                    │ │
│  │                      ▼                                    │ │
│  │           ┌─────────────────────┐                        │ │
│  │           │   OpenCost Server   │                        │ │
│  │           │  ┌───────────────┐  │                        │ │
│  │           │  │ Cost Model    │  │                        │ │
│  │           │  │ Engine        │  │                        │ │
│  │           │  └───────────────┘  │                        │ │
│  │           │  ┌───────────────┐  │                        │ │
│  │           │  │ Allocation    │  │                        │ │
│  │           │  │ Engine        │  │                        │ │
│  │           │  └───────────────┘  │                        │ │
│  │           └─────────────────────┘                        │ │
│  └────────────────────────────────────────────────────────────┘ │
│                              │                                   │
│              ┌───────────────┼───────────────┐                  │
│              │               │               │                  │
│              ▼               ▼               ▼                  │
│  ┌──────────────┐ ┌──────────────┐ ┌────────────────────────┐  │
│  │  Prometheus  │ │   REST API   │ │     OpenCost UI        │  │
│  │   Metrics    │ │  (JSON)      │ │                        │  │
│  └──────────────┘ └──────────────┘ └────────────────────────┘  │
│         │                │                    │                 │
│         ▼                ▼                    ▼                 │
│  ┌──────────────┐ ┌──────────────┐ ┌────────────────────────┐  │
│  │   Grafana    │ │  CI/CD Tools │ │   Cost Dashboards      │  │
│  │  Dashboards  │ │  Integration │ │                        │  │
│  └──────────────┘ └──────────────┘ └────────────────────────┘  │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

---

## 快速开始

### Helm 安装

```bash
# 添加仓库
helm repo add opencost https://opencost.github.io/opencost-helm-chart

# 安装 OpenCost
helm install opencost opencost/opencost \
  --namespace opencost \
  --create-namespace

# 安装带 UI
helm install opencost opencost/opencost \
  --namespace opencost \
  --create-namespace \
  --set opencost.ui.enabled=true
```

### 配置云厂商定价

```yaml
# values.yaml - AWS
opencost:
  exporter:
    cloudProviderApiKey: ""
    aws:
      enabled: true
      spotDataRegion: us-east-1
      spotDataBucket: "my-spot-pricing-bucket"
      
# values.yaml - GCP
opencost:
  exporter:
    cloudProviderApiKey: ""
    gcp:
      enabled: true
      bigQueryBillingDataset: "billing_export"

# values.yaml - Azure
opencost:
  exporter:
    azure:
      enabled: true
      subscriptionId: "your-subscription-id"
      clientId: "your-client-id"
      clientSecret: "your-client-secret"
      tenantId: "your-tenant-id"
```

### 自定义定价（私有云）

```yaml
# custom-pricing.yaml
opencost:
  customPricing:
    enabled: true
    configmapName: custom-pricing
    
# ConfigMap
apiVersion: v1
kind: ConfigMap
metadata:
  name: custom-pricing
  namespace: opencost
data:
  default.json: |
    {
      "provider": "custom",
      "description": "On-premise cluster",
      "CPU": "0.031611",
      "spotCPU": "0.006655",
      "RAM": "0.004237",
      "spotRAM": "0.000892",
      "GPU": "0.95",
      "storage": "0.00005479452",
      "zoneNetworkEgress": "0.01",
      "regionNetworkEgress": "0.01",
      "internetNetworkEgress": "0.12"
    }
```

---

## API 使用

### 查询成本分配

```bash
# 按命名空间查询
curl "http://opencost:9003/allocation/compute?window=7d&aggregate=namespace"

# 按标签查询
curl "http://opencost:9003/allocation/compute?window=7d&aggregate=label:app"

# 按控制器查询
curl "http://opencost:9003/allocation/compute?window=7d&aggregate=controller"

# 按节点查询
curl "http://opencost:9003/allocation/compute?window=7d&aggregate=node"

# 多维度聚合
curl "http://opencost:9003/allocation/compute?window=7d&aggregate=namespace,label:team"
```

### 查询资产成本

```bash
# 查询集群资产
curl "http://opencost:9003/assets?window=7d"

# 按类型聚合
curl "http://opencost:9003/assets?window=7d&aggregate=type"
```

### API 响应示例

```json
{
  "code": 200,
  "data": [
    {
      "production": {
        "name": "production",
        "cpuCost": 125.45,
        "memoryCost": 89.23,
        "gpuCost": 0,
        "pvCost": 15.67,
        "networkCost": 8.92,
        "totalCost": 239.27,
        "cpuEfficiency": 0.45,
        "memoryEfficiency": 0.62,
        "totalEfficiency": 0.53
      }
    }
  ]
}
```

---

## Prometheus 指标

```yaml
# 关键指标
- opencost_cluster_total_cost
- opencost_namespace_total_cost
- opencost_pod_total_cost
- opencost_node_total_cost
- opencost_pv_total_cost

# PromQL 示例
# 命名空间月度成本
sum(opencost_namespace_total_cost{namespace="production"}) * 720

# CPU 利用效率
opencost_allocation_cpu_efficiency

# 闲置资源成本
opencost_allocation_idle_cost
```

### Grafana Dashboard

```json
{
  "panels": [
    {
      "title": "Namespace Cost (Daily)",
      "targets": [
        {
          "expr": "sum by (namespace) (opencost_namespace_total_cost) * 24",
          "legendFormat": "{{namespace}}"
        }
      ]
    },
    {
      "title": "CPU Efficiency",
      "targets": [
        {
          "expr": "avg(opencost_allocation_cpu_efficiency) by (namespace)",
          "legendFormat": "{{namespace}}"
        }
      ]
    }
  ]
}
```

---

## 成本优化建议

### 识别闲置资源

```bash
# 查询低效率的命名空间
curl "http://opencost:9003/allocation/compute?window=7d&aggregate=namespace" | \
  jq '.data[] | to_entries[] | select(.value.totalEfficiency < 0.3)'
```

### 资源调整建议

```yaml
# OpenCost 可以与 VPA 集成
# 基于实际使用提供资源建议
apiVersion: autoscaling.k8s.io/v1
kind: VerticalPodAutoscaler
metadata:
  name: my-app-vpa
spec:
  targetRef:
    apiVersion: apps/v1
    kind: Deployment
    name: my-app
  updatePolicy:
    updateMode: "Off"  # 仅建议模式
```

---

## 成本分摊配置

```yaml
# 共享成本分摊
opencost:
  exporter:
    extraEnv:
      SHARED_OVERHEAD_COST_ENABLED: "true"
      # 按比例分摊系统命名空间成本
      SHARED_NAMESPACES: "kube-system,monitoring"
```

---

## CI/CD 集成

```yaml
# GitHub Actions - 成本检查
name: Cost Check
on: [pull_request]

jobs:
  cost-check:
    runs-on: ubuntu-latest
    steps:
      - name: Check deployment cost
        run: |
          COST=$(curl -s "http://opencost/allocation/compute?window=1d&aggregate=namespace" | \
            jq '.data[0].production.totalCost')
          if (( $(echo "$COST > 100" | bc -l) )); then
            echo "Cost exceeds budget!"
            exit 1
          fi
```

---

## 最佳实践

1. **标签策略**: 使用一致的标签（team, app, env）便于成本分配
2. **定期审查**: 每周检查成本报告，识别异常增长
3. **预算告警**: 设置成本阈值告警
4. **资源配额**: 结合成本数据设置命名空间配额
5. **闲置资源**: 定期清理未使用的 PV 和负载均衡器

---

## 参考资源

- [官方文档](https://opencost.io/docs)
- [GitHub Repo](https://github.com/opencost/opencost)
- [OpenCost Spec](https://github.com/opencost/opencost/blob/develop/spec)
- [Grafana Dashboard](https://grafana.com/grafana/dashboards/opencost)

---

**维护者**: Kudig Team | **许可证**: MIT
