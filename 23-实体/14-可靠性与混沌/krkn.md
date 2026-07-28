---
title: Krkn
description: '## 概述'
summary: 'Krkn（原名 Kraken）是一个面向 Kubernetes 的混沌工程工具，通过向集群注入各种问题场景来测试系统的弹性和可靠性。它支持节点问题、Pod 中断、网络混沌、CPU/内存压力、时间偏移等多种混沌场景，并提供基于 Cerberus 的健康检查和告警机制，帮助团队在生产环境之前发现系统弱点。'
category: entities
tags:
- k8s
- cncf
- chaos
- krkn
- etcd
- prometheus
- grafana
- crd
- operator
tier: supporting
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Krkn 是什么
- 如何 Krkn
trigger_keywords:
- Krkn
prerequisites:
- kubectl-basics
- prometheus-basics
- monitoring-basics
- etcd-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# Krkn

> **CNCF 状态**: Sandbox | **类别**: Chaos | **主要语言**: Python

## 概述

Krkn（原 krkube）是一个 CNCF 沙箱项目，由 Red Hat 开源，是专为 OpenShift/Kubernetes 设计的混沌工程工具。它专注于基础设施级故障注入——模拟节点宕机、网络中断、API Server 压力等大规模故障场景。Krkn 特别适合验证 OpenShift/K8s 生产集群的容灾能力和恢复机制。与 Chaos Mesh 专注于 Pod 级故障不同，Krkn 更关注节点和集群级别的混沌实验。

## Key Features（核心能力）

- **节点混沌**：模拟节点 NotReady、关机、网络隔离等故障
- **网络混沌**：注入集群级网络延迟、丢包、DNS 故障
- **API Server 压力**：模拟 API Server 过载和响应延迟
- **Pod 混沌**：大规模 Pod kill 和 IO 干扰
- **Scenario 框架**：通过 YAML 定义可复用的混沌场景
- **与 Prow 集成**：支持 CI/CD 流水线中的自动化混沌测试

## 架构与工作原理

Krkn 采用 Python 实现的 Scenario 驱动架构：每个 Scenario 以 YAML 配置定义故障类型、目标范围和持续时间。Krkn 核心引擎解析 Scenario 配置，通过 K8s API（如 cordon/uncordon node、delete pod）或系统命令（如 iptables、tc）执行故障注入。执行完成后自动收集指标和日志用于分析。支持与 Chaos Mesh 互补使用。

## K8s 集成

Krkn 直接通过 Kubernetes API 执行混沌操作：通过 cordon/uncordon 模拟节点故障；通过 delete pod 验证工作负载韧性；通过 NetworkPolicy 和 iptables 规则注入网络故障。Krkn 以 Job 或 CronJob 方式在 K8s 集群中运行，通过 ServiceAccount 获取所需的 API 权限。与 Prometheus 集成收集故障期间的系统指标。

## 生产用例

- **集群容灾测试**：验证多节点故障场景下的集群可用性
- **OpenShift 认证测试**：PaaS 级混沌验证
- **API Server 韧性**：验证控制平面在高负载下的表现
- **灾备演练**：模拟数据中心级故障验证 DR 方案

## 安装与配置

```bash
# 🟢 安装 Krkn CLI
pip3 install krkn

# 🟢 验证安装
krkn --version

# 🟢 创建配置文件
cat > kraken-config.yaml << 'EOF'
kraken:
  distribution: kubernetes
  kubeconfig_path: ~/.kube/config
  exit_on_failure: false
  port: 8081
  signal_address: 0.0.0.0
  run_mode: normal
  wait_duration: 120
  iterations: 1
  daemon_mode: false
cerberus:
  cerberus_enabled: true
  cerberus_url: http://cerberus:8080
  check_applicaton_routes: false
performance_monitoring:
  deploy_dashboards: true
  repo: "https://github.com/cloud-bulldozer/performance-dashboards.git"
  capture_metrics: true
  prometheus_url: http://prometheus:9090
EOF

# 🟢 运行混沌场景
krkn --config kraken-config.yaml --scenario scenarios/node_scenario.yaml

# 🟢 K8s Job 方式部署
kubectl apply -f krkn-job.yaml
```

### 混沌场景配置示例

```yaml
# scenarios/node_scenario.yaml
apiVersion: krkn/v1
kind: Scenario
metadata:
  name: node-failure-test
spec:
  scenario_type: node_scenarios
  actions:
    - name: node-stop-start
      label_selector: node-role.kubernetes.io/worker
      instance_count: 1
      runs: 2
      sleep: 60
      timeout: 300
      cloud_type: aws
    - name: node-cordon-uncordon
      label_selector: node-role.kubernetes.io/worker
      instance_count: 2
      runs: 1
      sleep: 30
---
# scenarios/network_scenario.yaml
apiVersion: krkn/v1
kind: Scenario
metadata:
  name: network-chaos-test
spec:
  scenario_type: network_scenarios
  actions:
    - name: pod-network-latency
      namespace: production
      label_selector: app=web-frontend
      latency: 500
      jitter: 100
      duration: 120
      interfaces: [eth0]
    - name: node-network-loss
      label_selector: node-role.kubernetes.io/worker
      loss: 50
      duration: 60
---
# scenarios/pod_scenario.yaml
apiVersion: krkn/v1
kind: Scenario
metadata:
  name: pod-kill-test
spec:
  scenario_type: pod_scenarios
  actions:
    - name: delete-pods
      namespace: production
      label_selector: app=payment-service
      instance_count: 2
      runs: 3
      sleep: 30
```

## 运维操作

```bash
# 🟢 查看当前运行的混沌实验
kubectl get jobs -n krkn
kubectl get pods -n krkn

# 🟢 查看实验日志
kubectl logs -n krkn job/krkn-run-<id> --tail=100

# 🟢 检查 Cerberus 健康状态
curl -s http://cerberus:8080 | jq .

# 🟡 停止当前混沌实验
kubectl delete job -n krkn --all

# 🟡 恢复被 cordon 的节点
kubectl uncordon --all

# 🔴 紧急停止所有混沌操作
kubectl delete jobs -n krkn --all
kubectl uncordon --all
# 检查并清理 iptables/tc 规则
```

## 故障排查

| 症状 | 可能原因 | 诊断命令 | 修复方法 |
|------|----------|----------|----------|
| 场景执行失败 | RBAC 权限不足 | `kubectl logs job/krkn-*` | 检查 ServiceAccount 权限 |
| 节点未恢复 | uncordon 未执行 | `kubectl get nodes` | 手动 uncordon |
| 网络规则残留 | tc/iptables 未清理 | `tc qdisc show` | 手动清理规则 |
| Cerberus 报警 | 系统未恢复 | `curl cerberus:8080` | 检查受影响组件 |

```bash
# 排查流程
# 1. 检查 Krkn Job 状态
kubectl get jobs -n krkn
kubectl describe job krkn-run-<id> -n krkn

# 2. 检查节点状态
kubectl get nodes -o wide
kubectl describe node <affected-node> | grep -A5 Taints

# 3. 检查网络规则残留
ssh <node> "tc qdisc show dev eth0"
ssh <node> "iptables -L -n | grep DROP"

# 4. 检查应用恢复状态
kubectl get pods -n production -o wide
kubectl get endpoints -n production
```

## 生产案例

### 案例1：集群容灾能力验证
- **场景**：金融企业需要验证 K8s 集群在 2 个节点同时宕机时的服务可用性
- **方案**：Krkn node_scenarios 同时停止 2 个 Worker 节点；Cerberus 监控应用健康；Prometheus 记录恢复时间
- **效果**：发现 PDB 配置缺陷，修复后服务恢复时间从 5min 降到 30s

### 案例2：CI/CD 混沌门禁
- **场景**：每次发布前自动验证新版本的容错能力
- **方案**：Krkn 集成到 Prow CI；发布后自动执行 Pod Kill + 网络延迟场景；Cerberus 检查服务健康，失败则回滚
- **效果**：上线后故障率降低 70%，混沌测试成为发布门禁

## 对比替代方案

| 维度 | Krkn | Chaos Mesh | LitmusChaos | Gremlin |
|------|------|-----------|-------------|--------|
| 故障级别 | 节点/集群 | Pod/网络 | 多层 | 多层 |
| OpenShift | 原生 | 支持 | 支持 | 支持 |
| 开源 | 是 | 是 | 是 | 否 |
| CI/CD 集成 | Prow | 支持 | 支持 | 支持 |
| 学习曲线 | 中 | 中 | 中 | 低 |

## 检查清单

- [ ] Krkn 已安装且配置文件已验证
- [ ] 混沌场景已在测试集群验证
- [ ] Cerberus 健康检查已部署
- [ ] 回滚方案已准备（节点 uncordon、网络规则清理）
- [ ] RBAC 权限已正确配置
- [ ] 实验窗口已通知相关团队
- [ ] Prometheus 指标采集已配置

## Related

- [[devfile]] — Devfile
- [[cohdi]] — Cohdi
- [[koordinator]] — Koordinator
- [[oxia]] — Oxia
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- krkn
- [[23-实体/cncf-infrastructure.md|[[23-实体/15-参考与索引/cncf-infrastructure|CNCF 基础设施与混沌工程项目全景]]]] — Cross-reference
- index/etcd-index|etcd 知识图谱索引]]


<!-- risk-assessed -->
