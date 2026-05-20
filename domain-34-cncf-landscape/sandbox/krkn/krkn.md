---
title: Krkn (Kraken)
description: '## 项目概述'
category: cncf-landscape
tags:
- k8s
- cncf
- cloud-native
- ecosystem
- etcd
- prometheus
- elasticsearch
- ingress
- operator
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 架构师
- 技术决策者
- SRE
estimated_read_time: 5min
intent_queries:
- Krkn (Kraken) 是什么
- 如何 Krkn (Kraken)
- Kubernetes 34 cncf landscape 最佳实践
trigger_keywords:
- Krkn
- Kraken
- cncf
- landscape
---

# Krkn (Kraken)

> **成熟度**: Sandbox | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官网** | https://krkn-chaos.github.io/krkn/ |
| **GitHub** | https://github.com/krkn-chaos/krkn |
| **许可证** | Apache-2.0 |
| **开发语言** | Python |
| **CNCF 状态** | Sandbox |

---

## 项目概述

Krkn（原名 Kraken）是一个面向 Kubernetes 的混沌工程工具，通过向集群注入各种故障场景来测试系统的弹性和可靠性。它支持节点故障、Pod 中断、网络混沌、CPU/内存压力、时间偏移等多种混沌场景，并提供基于 Cerberus 的健康检查和告警机制，帮助团队在生产环境之前发现系统弱点。

### 核心特性

- **节点混沌**: 关机、重启、终止云平台节点 (AWS/GCP/Azure/OpenStack)
- **Pod 混沌**: 随机删除 Pod、Pod 网络隔离、容器资源压力
- **网络混沌**: 引入延迟、丢包、分区等网络故障
- **资源压力**: CPU、内存、IO 压力注入
- **时间偏移**: 容器级时间偏移，测试时间相关逻辑
- **应用混沌**: 针对特定应用（如 etcd、API Server）的故障注入
- **Cerberus 集成**: 持续监控集群健康状态，混沌测试期间检测异常
- **可观测性**: 集成 Prometheus/Elasticsearch 收集混沌测试指标

---

## 架构设计

```
┌─────────────────────────────────────────────┐
│              Krkn Engine                     │
│                                              │
│  ┌──────────────────────────────────┐       │
│  │         场景编排器                │       │
│  │  (YAML 配置 / 场景调度)          │       │
│  └──────────────┬───────────────────┘       │
│                 │                             │
│  ┌──────────────▼───────────────────┐       │
│  │       混沌场景插件               │       │
│  │  ┌────────┐ ┌────────┐ ┌──────┐ │       │
│  │  │节点混沌│ │Pod 混沌│ │网络  │ │       │
│  │  └────────┘ └────────┘ └──────┘ │       │
│  │  ┌────────┐ ┌────────┐ ┌──────┐ │       │
│  │  │CPU 压力│ │时间偏移│ │应用  │ │       │
│  │  └────────┘ └────────┘ └──────┘ │       │
│  └──────────────┬───────────────────┘       │
│                 │                             │
│  ┌──────────────▼───────────────────┐       │
│  │       Cerberus 健康检查          │       │
│  │  (集群健康 / SLO 监控)           │       │
│  └──────────────────────────────────┘       │
└──────────────────┬──────────────────────────┘
                   │
         ┌─────────▼─────────┐
         │ Kubernetes Cluster │
         │  (混沌注入目标)    │
         └───────────────────┘
```

---

## 快速开始

### 安装

```bash
# 克隆仓库
git clone https://github.com/krkn-chaos/krkn.git
cd krkn

# 安装依赖
pip install -r requirements.txt

# 或使用容器运行
podman run --rm \
  -v ~/.kube/config:/root/.kube/config:Z \
  -v ./scenarios:/root/kraken/scenarios:Z \
  quay.io/krkn-chaos/krkn:latest
```

### 配置混沌场景

```yaml
# config.yaml - Krkn 主配置
kraken:
  kubeconfig_path: ~/.kube/config
  exit_on_failure: false
  chaos_scenarios:
    - pod_scenarios:
        - scenarios/pod-delete.yaml
    - node_scenarios:
        - scenarios/node-shutdown.yaml
    - network_scenarios:
        - scenarios/network-latency.yaml
  cerberus:
    cerberus_enabled: true
    cerberus_url: http://cerberus:8080
  performance_monitoring:
    deploy_dashboards: true
    prometheus_url: http://prometheus:9090
```

### Pod 删除场景

```yaml
# scenarios/pod-delete.yaml
input_list:
  - namespace: "default"
    name_pattern: ".*"
    label_selector: "app=my-service"
    pod_count: 1              # 每次删除 1 个 Pod
    kill_timeout: 60
    disruption_count: 3       # 连续执行 3 次
    wait_timeout: 120         # 等待 Pod 恢复
    expected_recovery_time: 30
```

### 节点关机场景

```yaml
# scenarios/node-shutdown.yaml
cloud_type: aws
node_scenarios:
  - actions:
      - stop_start_instances
    instance_count: 1
    label_selector: "node-role.kubernetes.io/worker="
    timeout: 300
    runs: 1
    cloud_credentials:
      aws_access_key: "${AWS_ACCESS_KEY_ID}"
      aws_secret_key: "${AWS_SECRET_ACCESS_KEY}"
      region: "us-east-1"
```

### 运行混沌测试

```bash
# 运行 Krkn
python run_kraken.py --config config.yaml

# 使用容器化 krkn-hub 运行特定场景
podman run --rm \
  -e KUBECONFIG=/root/.kube/config \
  -e SCENARIO_TYPE=pod-delete \
  -e NAMESPACE=default \
  -e POD_LABEL="app=nginx" \
  -v ~/.kube/config:/root/.kube/config:Z \
  quay.io/krkn-chaos/krkn-hub:pod-scenarios
```

---

## 高级场景

### 网络混沌

```yaml
# scenarios/network-latency.yaml
input_list:
  - namespace: "default"
    direction:
      - ingress
      - egress
    label_selector: "app=api-server"
    network_params:
      latency: 500ms
      loss: 10%              # 10% 丢包率
      bandwidth: 100mbit
    duration: 300             # 持续 5 分钟
    interfaces:
      - eth0
```

### 资源压力

```yaml
# scenarios/cpu-hog.yaml
input_list:
  - namespace: "default"
    label_selector: "app=compute-service"
    num_workers: 4            # CPU 压力线程数
    duration: 120             # 持续 2 分钟
    target_cpu_percent: 80    # 目标 CPU 使用率
```

---

## 与其他方案对比

| 特性 | Krkn | Chaos Mesh | Litmus | ChaosBlade |
|:---|:---|:---|:---|:---|
| 语言 | Python | Go | Go | Go/Java |
| 安装方式 | CLI/容器 | Operator | Operator | CLI/Operator |
| 云平台集成 | AWS/GCP/Azure | 有限 | AWS/GCP/Azure | 阿里云 |
| OpenShift 支持 | 原生支持 | 部分 | 部分 | 部分 |
| 节点场景 | 原生 | 需权限 | 原生 | 原生 |
| 健康检查 | Cerberus | 内置 | 内置 | 需自建 |
| 适用场景 | 大规模集群 SRE | 通用 | 通用 | 通用 |

---

## 最佳实践

1. **渐进式注入**: 从小范围、低强度开始，逐步扩大混沌范围
2. **健康检查**: 始终启用 Cerberus 监控集群状态，设置安全阀
3. **非生产先行**: 先在测试/预发环境验证混沌场景
4. **SLO 驱动**: 基于 SLO 定义验收标准，混沌测试通过=SLO 不受影响
5. **团队协作**: 提前通知相关团队，记录混沌测试的发现和改进措施

---

## 参考资源

- [Krkn 官方文档](https://krkn-chaos.github.io/krkn/)
- [Krkn GitHub](https://github.com/krkn-chaos/krkn)
- [krkn-hub 场景集](https://github.com/krkn-chaos/krkn-hub)
- [CNCF Sandbox Projects](https://www.cncf.io/sandbox-projects/)

---

**维护者**: Kudig Team | **许可证**: MIT
