# Kube-burner

> **成熟度**: Sandbox | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官网** | https://kube-burner.github.io/kube-burner/ |
| **GitHub** | https://github.com/kube-burner/kube-burner |
| **许可证** | Apache-2.0 |
| **开发语言** | Go |
| **CNCF 状态** | Sandbox |

---

## 项目概述

Kube-burner 是一个 Kubernetes 性能和规模测试工具，通过在集群中创建或删除大量对象来模拟各种负载场景，并收集详细的性能指标。它广泛用于 Kubernetes 发行版（如 OpenShift）的可扩展性测试和基准测试。

### 核心特性

- **负载生成**: 批量创建 Namespace、Deployment、Pod、Service 等资源
- **指标收集**: 集成 Prometheus 收集 API Server 延迟、etcd 性能等指标
- **索引存储**: 将测试结果写入 Elasticsearch/OpenSearch 进行分析
- **内置场景**: 提供 node-density、cluster-density 等预定义测试场景
- **告警规则**: 在测试期间检测性能回退，基于 PromQL 告警
- **可重复性**: 配置文件驱动，确保测试可重复执行

---

## 架构设计

```
┌──────────────────────────────────────────────┐
│              kube-burner CLI                  │
│                                              │
│  ┌─────────┐  ┌──────────┐  ┌────────────┐ │
│  │ Job      │  │ Metrics  │  │ Alerting   │ │
│  │ Engine   │  │ Collector│  │ Engine     │ │
│  └────┬─────┘  └────┬─────┘  └─────┬──────┘ │
│       │              │              │         │
└───────┼──────────────┼──────────────┼─────────┘
        │              │              │
        ▼              ▼              ▼
   K8s API Server  Prometheus    Elasticsearch
   (Create/Delete) (Metrics)    (Index Results)
```

---

## 快速开始

### 安装

```bash
# 下载最新版本
curl -sL https://github.com/kube-burner/kube-burner/releases/latest/download/kube-burner-linux-x86_64.tar.gz | tar xz
sudo mv kube-burner /usr/local/bin/

# 验证安装
kube-burner version
```

### 运行内置场景

```bash
# 节点密度测试 - 每节点创建大量 Pod
kube-burner ocp node-density --pods-per-node=100 --uuid=$(uuidgen)

# 集群密度测试
kube-burner ocp cluster-density-v2 --iterations=100 --churn=true
```

---

## 配置详解

### 自定义测试配置

```yaml
# config.yaml
global:
  writeToFile: true
  indexerConfig:
    type: opensearch
    esServers: ["https://opensearch:9200"]
    defaultIndex: kube-burner
    insecureSkipVerify: true

metricsEndpoints:
  - endpoint: http://prometheus:9090
    metrics:
      - metrics-profile.yaml
    alerts:
      - alerts-profile.yaml

jobs:
  - name: create-namespaces
    jobType: create
    jobIterations: 100
    qps: 20
    burst: 40
    namespacedIterations: true
    namespace: kube-burner-ns
    podWait: false
    waitWhenFinished: true
    cleanup: false
    objects:
      - objectTemplate: namespace.yaml
        replicas: 1

  - name: create-deployments
    jobType: create
    jobIterations: 100
    qps: 10
    burst: 20
    namespacedIterations: true
    namespace: kube-burner-ns
    podWait: true
    waitWhenFinished: true
    maxWaitTimeout: 5m
    objects:
      - objectTemplate: deployment.yaml
        replicas: 3
        inputVars:
          podReplicas: 2
          cpuRequest: 10m
          memoryRequest: 64Mi
      - objectTemplate: service.yaml
        replicas: 1
```

### 对象模板

```yaml
# deployment.yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: test-deployment-{{.Iteration}}
  labels:
    kube-burner-job: {{.JobName}}
    kube-burner-uuid: {{.UUID}}
spec:
  replicas: {{.podReplicas}}
  selector:
    matchLabels:
      app: test-{{.Iteration}}
  template:
    metadata:
      labels:
        app: test-{{.Iteration}}
    spec:
      containers:
        - name: app
          image: registry.k8s.io/pause:3.9
          resources:
            requests:
              cpu: "{{.cpuRequest}}"
              memory: "{{.memoryRequest}}"
      topologySpreadConstraints:
        - maxSkew: 1
          topologyKey: kubernetes.io/hostname
          whenUnsatisfiable: ScheduleAnyway
          labelSelector:
            matchLabels:
              app: test-{{.Iteration}}
```

### 指标采集配置

```yaml
# metrics-profile.yaml
- query: histogram_quantile(0.99, sum(rate(apiserver_request_duration_seconds_bucket{verb=~"POST|PUT|PATCH|DELETE"}[2m])) by (verb, resource, le))
  metricName: apiServerWriteLatency99th
  instant: true

- query: histogram_quantile(0.99, sum(rate(apiserver_request_duration_seconds_bucket{verb=~"GET|LIST"}[2m])) by (verb, resource, le))
  metricName: apiServerReadLatency99th
  instant: true

- query: sum(rate(etcd_request_duration_seconds_sum[2m])) by (operation, type)
  metricName: etcdRequestLatency
  instant: true

- query: process_resident_memory_bytes{job="apiserver"}
  metricName: apiServerMemory
  instant: true
```

### 告警规则

```yaml
# alerts-profile.yaml
- expr: histogram_quantile(0.99, sum(rate(apiserver_request_duration_seconds_bucket{verb=~"POST|PUT|PATCH|DELETE"}[5m])) by (le)) > 1
  description: "API Server write P99 latency > 1s"
  severity: critical

- expr: histogram_quantile(0.99, sum(rate(apiserver_request_duration_seconds_bucket{verb=~"LIST"}[5m])) by (le)) > 5
  description: "API Server LIST P99 latency > 5s"
  severity: warning
```

---

## 高级用法

### Churn 模式（对象替换）

```yaml
jobs:
  - name: cluster-density-with-churn
    jobType: create
    jobIterations: 500
    churn: true
    churnPercent: 10       # 每周期替换 10% 的对象
    churnDuration: 30m     # 持续 30 分钟
    churnDelay: 30s        # 每 30 秒一个替换周期
```

### 删除测试

```yaml
jobs:
  - name: cleanup
    jobType: delete
    waitForDeletion: true
    objects:
      - kind: Namespace
        labelSelector:
          kube-burner-job: create-namespaces
```

---

## 最佳实践

1. **基线测试**: 先在空集群运行获取基线数据，再对比优化后的结果
2. **渐进负载**: 从低 QPS 开始逐步提高，找到集群的吞吐瓶颈
3. **指标存储**: 使用 Elasticsearch 持久化结果，便于趋势分析和回归检测
4. **告警阈值**: 根据 SLO 设定合理的告警阈值，及时发现性能回退
5. **资源隔离**: 在专用测试集群运行，避免影响生产环境
6. **重复执行**: 每次测试多次运行取平均值，减少偶发因素影响

---

## 参考资源

- [Kube-burner 官方文档](https://kube-burner.github.io/kube-burner/)
- [Kube-burner GitHub](https://github.com/kube-burner/kube-burner)
- [CNCF Sandbox Projects](https://www.cncf.io/sandbox-projects/)

---

**维护者**: Kudig Team | **许可证**: MIT
