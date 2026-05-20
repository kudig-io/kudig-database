---
title: Litmus
description: '## 项目概述'
category: cncf-landscape
tags:
- k8s
- cncf
- cloud-native
- ecosystem
- kubelet
- prometheus
- grafana
- helm
- crd
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
- Litmus 是什么
- 如何 Litmus
- Kubernetes 34 cncf landscape 最佳实践
trigger_keywords:
- Litmus
- cncf
- landscape
---


# Litmus

> **成熟度**: Incubating | **加入时间**: 2020-06 | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官方网站** | https://litmuschaos.io |
| **GitHub** | https://github.com/litmuschaos/litmus |
| **许可证** | Apache-2.0 |
| **主要语言** | Go |
| **CNCF 分类** | Observability & Chaos Engineering |

---

## 项目概述

Litmus 是云原生混沌工程平台，提供完整的混沌实验编排和管理能力。它帮助团队在受控环境中测试系统弹性，发现潜在故障点并提高系统可靠性。

## 核心特性

- **丰富的实验库**: ChaosHub 提供 50+ 预置混沌实验
- **Kubernetes 原生**: CRD 方式定义和管理混沌实验
- **GitOps 支持**: 混沌即代码，版本控制管理
- **可观测性集成**: Prometheus 指标和 Grafana 仪表盘
- **细粒度控制**: 支持命名空间、标签、注解级别的定向注入
- **多租户**: 支持多团队协作管理混沌实验

---

## 架构设计

```
┌─────────────────────────────────────────────────────────────────┐
│                     Litmus Architecture                          │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  ┌────────────────────────────────────────────────────────────┐ │
│  │                    Litmus Control Plane                    │ │
│  │                                                            │ │
│  │  ┌─────────────┐  ┌──────────────┐  ┌─────────────────┐  │ │
│  │  │  ChaosCenter│  │   GraphQL    │  │   MongoDB       │  │ │
│  │  │   Web UI    │  │   Server     │  │   (State)       │  │ │
│  │  └─────────────┘  └──────────────┘  └─────────────────┘  │ │
│  │                                                            │ │
│  └──────────────────────────┬─────────────────────────────────┘ │
│                             │                                    │
│            ┌────────────────┼────────────────┐                  │
│            │                │                │                  │
│            ▼                ▼                ▼                  │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐          │
│  │  Cluster 1   │  │  Cluster 2   │  │  Cluster N   │          │
│  │              │  │              │  │              │          │
│  │ ┌──────────┐ │  │ ┌──────────┐ │  │ ┌──────────┐ │          │
│  │ │Subscriber│ │  │ │Subscriber│ │  │ │Subscriber│ │          │
│  │ └──────────┘ │  │ └──────────┘ │  │ └──────────┘ │          │
│  │ ┌──────────┐ │  │ ┌──────────┐ │  │ ┌──────────┐ │          │
│  │ │ Workflow │ │  │ │ Workflow │ │  │ │ Workflow │ │          │
│  │ │Controller│ │  │ │Controller│ │  │ │Controller│ │          │
│  │ └──────────┘ │  │ └──────────┘ │  │ └──────────┘ │          │
│  │ ┌──────────┐ │  │ ┌──────────┐ │  │ ┌──────────┐ │          │
│  │ │  Chaos   │ │  │ │  Chaos   │ │  │ │  Chaos   │ │          │
│  │ │ Operator │ │  │ │ Operator │ │  │ │ Operator │ │          │
│  │ └──────────┘ │  │ └──────────┘ │  │ └──────────┘ │          │
│  └──────────────┘  └──────────────┘  └──────────────┘          │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

### 核心组件

| 组件 | 功能 |
|------|------|
| ChaosCenter | Web UI 和 API 控制中心 |
| Subscriber | 集群与控制中心的通信代理 |
| Workflow Controller | Argo Workflows 执行器 |
| Chaos Operator | 混沌实验 CRD 控制器 |
| Chaos Exporter | Prometheus 指标导出 |

---

## 快速开始

### 安装 Litmus 3.x

```bash
# 添加 Helm 仓库
helm repo add litmuschaos https://litmuschaos.github.io/litmus-helm/

# 安装 ChaosCenter
helm install litmus litmuschaos/litmus \
  --namespace litmus \
  --create-namespace

# 获取访问地址
kubectl get svc -n litmus litmus-frontend-service
```

### 连接集群（Self-Agent）

```bash
# 在 ChaosCenter UI 中创建环境
# 获取连接命令并在目标集群执行
kubectl apply -f https://litmus-server:port/api/file/AGENT_YAML
```

---

## 混沌实验定义

### Pod Delete 实验

```yaml
apiVersion: litmuschaos.io/v1alpha1
kind: ChaosEngine
metadata:
  name: pod-delete-chaos
  namespace: default
spec:
  appinfo:
    appns: 'production'
    applabel: 'app=nginx'
    appkind: 'deployment'
  chaosServiceAccount: litmus-admin
  experiments:
    - name: pod-delete
      spec:
        components:
          env:
            - name: TOTAL_CHAOS_DURATION
              value: '30'
            - name: CHAOS_INTERVAL
              value: '10'
            - name: FORCE
              value: 'false'
            - name: PODS_AFFECTED_PERC
              value: '50'
```

### Pod CPU Hog 实验

```yaml
apiVersion: litmuschaos.io/v1alpha1
kind: ChaosEngine
metadata:
  name: cpu-hog-chaos
spec:
  appinfo:
    appns: 'production'
    applabel: 'app=api-server'
    appkind: 'deployment'
  chaosServiceAccount: litmus-admin
  experiments:
    - name: pod-cpu-hog
      spec:
        components:
          env:
            - name: TOTAL_CHAOS_DURATION
              value: '60'
            - name: CPU_CORES
              value: '1'
            - name: CPU_LOAD
              value: '100'
            - name: PODS_AFFECTED_PERC
              value: '100'
```

### Network Chaos 实验

```yaml
apiVersion: litmuschaos.io/v1alpha1
kind: ChaosEngine
metadata:
  name: network-chaos
spec:
  appinfo:
    appns: 'production'
    applabel: 'app=frontend'
    appkind: 'deployment'
  chaosServiceAccount: litmus-admin
  experiments:
    - name: pod-network-latency
      spec:
        components:
          env:
            - name: TOTAL_CHAOS_DURATION
              value: '60'
            - name: NETWORK_INTERFACE
              value: 'eth0'
            - name: NETWORK_LATENCY
              value: '200'  # ms
            - name: JITTER
              value: '50'
```

---

## 混沌工作流

```yaml
apiVersion: argoproj.io/v1alpha1
kind: Workflow
metadata:
  name: chaos-workflow
spec:
  entrypoint: chaos-workflow
  templates:
    - name: chaos-workflow
      steps:
        - - name: install-experiment
            template: install-chaos-experiments
        - - name: run-pod-delete
            template: pod-delete
        - - name: run-cpu-hog
            template: cpu-hog
        - - name: verify-results
            template: verify-chaos-results

    - name: pod-delete
      inputs:
        artifacts:
          - name: pod-delete-engine
            raw:
              data: |
                apiVersion: litmuschaos.io/v1alpha1
                kind: ChaosEngine
                metadata:
                  name: pod-delete-chaos
                spec:
                  appinfo:
                    appns: 'production'
                    applabel: 'app=nginx'
                  experiments:
                    - name: pod-delete
      container:
        image: litmuschaos/litmus-checker:latest
        args:
          - -file=/tmp/pod-delete-engine.yaml
```

---

## ChaosHub 实验库

### 常用实验

| 实验 | 类别 | 说明 |
|------|------|------|
| pod-delete | Pod | 随机删除 Pod |
| pod-cpu-hog | Resource | CPU 压力测试 |
| pod-memory-hog | Resource | 内存压力测试 |
| pod-network-latency | Network | 网络延迟注入 |
| pod-network-loss | Network | 网络丢包注入 |
| node-drain | Node | 节点排空 |
| node-cpu-hog | Node | 节点 CPU 压力 |
| disk-fill | Storage | 磁盘填充 |
| kubelet-service-kill | Kubernetes | Kubelet 服务终止 |

### 添加自定义实验

```yaml
apiVersion: litmuschaos.io/v1alpha1
kind: ChaosExperiment
metadata:
  name: custom-experiment
spec:
  definition:
    scope: Namespaced
    permissions:
      - apiGroups: [""]
        resources: ["pods"]
        verbs: ["get", "list", "delete"]
    image: "my-registry/custom-chaos:latest"
    args:
      - -c
      - ./chaos
    env:
      - name: TARGET_PODS
        value: ''
      - name: CHAOS_DURATION
        value: '30'
```

---

## Probes（探针）

```yaml
experiments:
  - name: pod-delete
    spec:
      probe:
        - name: http-probe
          type: httpProbe
          mode: Continuous
          httpProbe/inputs:
            url: 'http://frontend-service:8080/health'
            method:
              get:
                criteria: '=='
                responseCode: '200'
          runProperties:
            probeTimeout: 5s
            interval: 2s
            retry: 3
        - name: cmd-probe
          type: cmdProbe
          mode: Edge
          cmdProbe/inputs:
            command: 'kubectl get pods -l app=nginx'
            comparator:
              type: string
              criteria: contains
              value: Running
```

---

## 监控与报告

### Prometheus 指标

```yaml
# 关键指标
- litmuschaos_experiment_run_count
- litmuschaos_experiment_success_rate
- litmuschaos_probe_success_percentage
- litmuschaos_experiment_verdict

# 告警规则
- alert: ChaosExperimentFailed
  expr: litmuschaos_experiment_verdict == 0
  for: 1m
  labels:
    severity: warning
  annotations:
    summary: "Chaos experiment failed"
```

---

## 最佳实践

1. **渐进式测试**: 从低强度实验开始，逐步增加复杂度
2. **稳态假设**: 实验前定义清晰的稳态指标和阈值
3. **最小爆炸半径**: 限制实验范围，避免影响生产环境
4. **自动化集成**: 将混沌实验纳入 CI/CD 流水线
5. **游戏日**: 定期组织全团队参与的混沌工程演练

---

## 参考资源

- [官方文档](https://litmuschaos.io/docs)
- [GitHub Repo](https://github.com/litmuschaos/litmus)
- [ChaosHub](https://hub.litmuschaos.io)
- [混沌工程原则](https://principlesofchaos.org/)

---

**维护者**: Kudig Team | **许可证**: MIT
