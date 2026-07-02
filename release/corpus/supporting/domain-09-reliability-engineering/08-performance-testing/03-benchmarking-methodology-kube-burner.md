---
title: kube-burner 集群密度测试方法论
description: '使用 kube-burner 进行 Kubernetes 集群密度、负载与缩放基准测试的完整方法论'
summary: '使用 kube-burner 进行 Kubernetes 集群密度、负载与缩放基准测试的完整方法论'
category: reliability-engineering
tags:
- performance-testing
- benchmarking
- kube-burner
- density
- scalability
tier: critical
created: '2026-07-02'
last_updated: 2026-07
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 平台工程师
estimated_read_time: 15min
intent_queries:
- kube-burner 集群密度测试 是什么
- 如何使用 kube-burner 进行基准测试
trigger_keywords:
- kube-burner
- density
- benchmark
- scalability
- perf-test
prerequisites:
- kubectl-basics
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
authors:
- name: KUDIG Team
  role: contributor
---

# kube-burner 集群密度测试方法论

## 1. 概述

kube-burner 是 Red Hat 开源的 Kubernetes 性能与规模测试工具，专注于集群密度（Density）、负载（Load）和缩放（Scale）场景。与通用压测工具不同，kube-burner 以声明式 YAML 定义工作负载，自动采集 Prometheus 指标，并生成标准化的基准报告。

核心能力：
- 批量创建/删除 Kubernetes 资源（Pod、Service、ConfigMap 等）
- 内置 Metrics Profiling，自动查询 Prometheus 并关联资源生命周期
- 支持 Node Density、Cluster Density、Network Policy Density 等标准化场景
- 与 K8s perf-test 工具链（clusterloader2、sonobuoy）互补

## 2. 安装与配置

### 2.1 安装 kube-burner

```bash
# 二进制安装（推荐）
KUBE_BURNER_VERSION="v1.9"
curl -sL "https://github.com/kube-burner/kube-burner/releases/download/${KUBE_BURNER_VERSION}/kube-burner-${KUBE_BURNER_VERSION}-linux-x86_64.tar.gz" | tar xz -C /usr/local/bin/

# 验证安装
kube-burner version
```

### 2.2 配置文件结构

kube-burner 使用一个主配置文件引用多个 Job 定义：

```
benchmark/
├── config.yml          # 主配置（全局参数 + Job 列表）
├── metrics.yml         # Metrics 采集配置
├── jobs/
│   ├── cluster-density.yml
│   ├── node-density.yml
│   └── network-policy-density.yml
└── templates/          # 资源模板
    ├── deployment.yml
    └── service.yml
```

### 2.3 主配置文件

```yaml
# config.yml
global:
  measurements:
    - name: podLatency
      esIndex: kube-burner-pod-latency
    - name: serviceLatency
      esIndex: kube-burner-service-latency
  indexerConfig:
    type: local          # 或 elastic
    enabled: true

jobs:
  - name: cluster-density
    jobType: create
    qps: 20
    burst: 20
    namespace: cluster-density
    namespacedIterations: true
    cleanup: true
    iterations: 1000
    objects:
      - objectTemplate: templates/deployment.yml
        replicas: 1
      - objectTemplate: templates/service.yml
        replicas: 1
```

## 3. 测试场景设计

### 3.1 Cluster Density（集群密度）

衡量集群在大量命名空间和资源下的调度与管理能力。

```yaml
# jobs/cluster-density.yml
- name: cluster-density
  jobType: create
  namespace: cluster-density
  namespacedIterations: true
  iterations: 1000          # 每个迭代创建一个 namespace
  qps: 20
  burst: 20
  maxWaitTimeout: 1h
  waitFor: []
  cleanup: true
  objects:
    - objectTemplate: templates/deployment.yml
      replicas: 5           # 每 namespace 5 个 Deployment
      inputVars:
        containerImage: registry.k8s.io/pause:3.9
        replicas: 1
    - objectTemplate: templates/service.yml
      replicas: 5
    - objectTemplate: templates/configmap.yml
      replicas: 10
    - objectTemplate: templates/secret.yml
      replicas: 5
```

```yaml
# templates/deployment.yml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: {{ .JobName }}-{{ .Iteration }}-{{ .Replica }}
spec:
  replicas: {{ .inputVars.replicas }}
  selector:
    matchLabels:
      app: {{ .JobName }}-{{ .Iteration }}-{{ .Replica }}
  template:
    metadata:
      labels:
        app: {{ .JobName }}-{{ .Iteration }}-{{ .Replica }}
    spec:
      containers:
        - name: placeholder
          image: {{ .inputVars.containerImage }}
          resources:
            requests:
              cpu: 1m
              memory: 10Mi
            limits:
              cpu: 10m
              memory: 20Mi
```

### 3.2 Node Density（节点密度）

衡量单节点在高 Pod 密度下的性能表现。

```yaml
- name: node-density
  jobType: create
  namespace: node-density
  iterations: 0                    # 自动计算：基于节点数和目标密度
  podDensity:
    podsPerNode: 250               # 目标 Pod 数/节点
  objects:
    - objectTemplate: templates/pod.yml
      replicas: 1
      inputVars:
        containerImage: registry.k8s.io/pause:3.9
```

### 3.3 Network Policy Density

衡量 NetworkPolicy 大规模部署对数据面性能的影响。

```yaml
- name: network-policy-density
  jobType: create
  namespace: netpol-density
  iterations: 100
  objects:
    - objectTemplate: templates/deployment.yml
      replicas: 10
    - objectTemplate: templates/networkpolicy.yml
      replicas: 50              # 每 namespace 50 条 NetworkPolicy
```

### 3.4 缩放测试

```yaml
- name: scale-up
  jobType: patch
  namespace: scale-test
  objects:
    - objectTemplate: templates/scale-deployment.yml
      replicas: 100
      patchType: merge
      inputVars:
        replicas: 500            # 从 100 扩展到 500
```

## 4. Metrics Profiling 配置

### 4.1 Prometheus 集成

```yaml
# metrics.yml
metrics:
  - query: sum(rate(container_cpu_usage_seconds_total{namespace=~"cluster-density.*"}[2m]))
    metricName: cpuUsage
    instant: false

  - query: sum(container_memory_working_set_bytes{namespace=~"cluster-density.*"})
    metricName: memoryUsage
    instant: false

  - query: sum(rate(apiserver_request_total{verb!="WATCH"}[2m]))
    metricName: apiserverRequestRate
    instant: false

  - query: histogram_quantile(0.99, sum(rate(apiserver_request_duration_seconds_bucket{verb!="WATCH"}[5m])) by (le, verb))
    metricName: apiserverLatencyP99
    instant: false

  - query: etcd_disk_wal_fsync_duration_seconds_count
    metricName: etcdWalFsync
    instant: false

  - query: scheduler_scheduling_algorithm_duration_seconds_bucket
    metricName: schedulerLatency
    instant: false

alerts:
  - expr: avg_over_time(apiserver_request_duration_seconds:histogram_quantile{quantile="0.99"}[1h]) > 1
    severity: warning
    description: "API Server P99 延迟超过 1s"
```

### 4.2 Pod Latency 内置指标

kube-burner 自动采集以下 Pod 生命周期指标：

| 指标 | 含义 |
|------|------|
| `podLatency(PodScheduled)` | 调度延迟 |
| `podLatency(Initialized)` | Init 容器完成时间 |
| `podLatency(ContainersReady)` | 所有容器就绪时间 |
| `podLatency(Ready)` | Pod Ready 时间 |

阈值配置：

```yaml
global:
  measurements:
    - name: podLatency
      thresholds:
        - metric: P99
          condition: "<"
          value: 5000          # P99 Pod Ready < 5s
        - metric: P99
          condition: "<"
          value: 1000          # P99 调度延迟 < 1s
```

## 5. 执行与结果分析

### 5.1 执行基准测试

```bash
# 使用本地 Prometheus
kube-burner init \
  --config config.yml \
  --metrics-profile metrics.yml \
  --prometheus-url http://prometheus.kubernetes-monitoring:9090 \
  --timeout 2h

# 使用外部 Elasticsearch 存储结果
kube-burner init \
  --config config.yml \
  --metrics-profile metrics.yml \
  --prometheus-url http://prometheus:9090 \
  --es-url https://elastic:9200 \
  --es-index kube-burner-results
```

### 5.2 结果文件

测试完成后在当前目录生成：

```
collected-metrics/
├── podLatency-density.json          # Pod 延迟数据
├── cpuUsage-density.json            # CPU 使用率时序
├── memoryUsage-density.json         # 内存使用率时序
├── apiserverLatencyP99-density.json # API Server 延迟
└── indexer.json                     # 测试元数据
```

### 5.3 基线对比

```bash
# 提取 P99 Pod Ready 延迟
jq '.[] | select(.quantile=="P99") | .avg' collected-metrics/podLatency-density.json

# 与基线对比
BASELINE=3500    # 基线 P99 = 3500ms
CURRENT=$(jq -r '.[] | select(.metricName=="podLatency" and .quantile=="P99") | .avg' \
  collected-metrics/podLatency-density.json)

if (( $(echo "$CURRENT > $BASELINE * 1.1" | bc -l) )); then
  echo "FAIL: P99 延迟回归 > 10% (当前: ${CURRENT}ms, 基线: ${BASELINE}ms)"
  exit 1
fi
```

### 5.4 关键指标阈值

| 指标 | 基准值（1000 namespace） | 告警阈值 |
|------|-------------------------|---------|
| Pod Ready P99 | < 5s | > 10s |
| Pod 调度 P99 | < 1s | > 3s |
| API Server QPS | > 500 | < 200 |
| API Server P99 延迟 | < 500ms | > 1s |
| etcd fsync P99 | < 10ms | > 50ms |
| 调度器延迟 P99 | < 200ms | > 500ms |

## 6. K8s perf-test 工具链集成

### 6.1 clusterloader2 互补

clusterloader2 是 Kubernetes 官方的负载测试框架，适合测量 API 吞吐量和端到端延迟。kube-burner 侧重密度指标，二者互补：

```bash
# clusterloader2 测试 API 吞吐
go run cmd/clusterloader.go \
  --testconfig=testing/load/config.yaml \
  --provider=local \
  --kubeconfig=$KUBECONFIG
```

### 6.2 sonobuoy 集成

```bash
# 运行 Conformance + Performance 组合测试
sonobuoy run \
  --mode=certified-conformance \
  --plugin kube-burner-plugin.yml
```

### 6.3 CI/CD 集成

```yaml
# GitHub Actions 示例
- name: Run Density Benchmark
  run: |
    kube-burner init --config config.yml --metrics-profile metrics.yml \
      --prometheus-url $PROMETHEUS_URL --timeout 1h
    
    P99=$(jq -r '.[] | select(.metricName=="podLatency" and .quantile=="P99") | .avg' \
      collected-metrics/podLatency-density.json)
    
    if (( $(echo "$P99 > 10000" | bc -l) )); then
      echo "::error::Density P99 regression: ${P99}ms"
      exit 1
    fi
```

## 7. 最佳实践

### 7.1 环境隔离

- 使用独立的测试集群，避免影响生产环境
- 测试前清理无关资源，确保指标基线一致
- 记录集群规格（节点数、机型、K8s 版本）作为测试元数据

### 7.2 渐进式加压

```
100 namespace → 500 namespace → 1000 namespace → 2000 namespace
每级运行 30min，观察指标稳定性后再升级
```

### 7.3 测试结果版本化

将每次测试结果提交到 Git，建立历史趋势：

```bash
git add collected-metrics/ benchmark-metadata.json
git commit -m "density-baseline: k8s-1.31, 3x c5.4xlarge, 1000ns"
```

## 8. 常见问题排查

| 现象 | 可能原因 | 排查方法 |
|------|---------|---------|
| Pod 调度延迟飙升 | 调度器瓶颈或节点资源碎片 | 检查 scheduler 队列深度和节点资源分布 |
| API Server 超时 | etcd 磁盘 I/O 饱和 | 检查 etcd 磁盘延迟和 WAL fsync |
| 创建速率下降 | API Server 限流 | 检查 APF（Priority and Fairness）配置 |
| 内存持续增长 | Controller 内存泄漏 | 检查 controller-manager 内存 RSS |

## Related

- [[01-load-testing-methodology|负载测试方法论]]
- [[02-chaos-load-integration|混沌工程与负载集成]]

## See Also

- [kube-burner 官方文档](https://kube-burner.github.io/kube-burner/)
- [K8s perf-test 仓库](https://github.com/kubernetes/perf-tests)
- [clusterloader2 文档](https://github.com/kubernetes/perf-tests/tree/master/clusterloader2)
