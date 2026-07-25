---
title: LitmusChaos
description: '## 概述'
summary: '该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[22-概念/01-核心架构/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]'
category: entities
tags:
- k8s
- cncf
- observability
- litmus
- prometheus
- grafana
- istio
- argocd
- crd
- operator
tier: peripheral
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- LitmusChaos 是什么
- 如何 LitmusChaos
trigger_keywords:
- LitmusChaos
prerequisites:
- kubectl-basics
- service-mesh-basics
- prometheus-basics
- monitoring-basics
- gitops-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# LitmusChaos

> **CNCF 状态**: Incubating | **类别**: Observability | **主要语言**: Go

## 概述

LitmusChaos 是由 Harness（原 MayaData）开源的云原生混沌工程平台，2020 年加入 CNCF Sandbox，后晋升为 Incubating。它提供完整的混沌实验编排和管理能力，帮助团队在受控环境中主动注入故障，测试系统弹性，发现潜在问题。LitmusChaos 提供预置的 ChaosHub 实验库，支持 Pod 杀死、网络延迟、CPU 压力等数十种故障注入场景，并通过 GitOps 方式管理混沌实验。

## 核心特性

- **ChaosHub**: 50+ 预置混沌实验（Pod Kill、Network Delay、CPU Hog、Disk Fill 等）
- **CRD 原生**: ChaosEngine、ChaosExperiment、ChaosResult CRD 声明式管理
- **GitOps 支持**: 混沌实验以 YAML 定义，通过 Git 版本控制管理
- **稳态假设**: 实验前后验证系统稳态指标（Hypothesis CRD）
- **多调度模式**: 支持 Cron、Manual、Automated 触发方式
- **可观测性**: Prometheus 指标导出和 Grafana 仪表盘

## 架构

LitmusChaos 采用控制平面-执行平面分离架构。控制平面（ChaosCenter）提供 Web UI 和 API，管理项目、用户和实验编排。执行平面由 ChaosOperator 和 ChaosRunner 组成——Operator 监听 ChaosEngine CRD，为每个实验创建 ChaosRunner Pod。ChaosRunner 注入混沌故障到目标 Pod/Node。实验执行使用 LitmusProbes（探针）验证稳态假设，结果写入 ChaosResult CRD。ChaosHub 提供实验模板，可克隆和自定义。

## Kubernetes 集成

LitmusChaos 完全基于 Kubernetes CRD 构建。ChaosEngine 定义实验目标和参数，ChaosExperiment 定义实验步骤，ChaosResult 记录执行结果。通过 ServiceAccount 和 RBAC 控制实验权限范围。支持命名空间级别的混沌隔离。实验可通过 ArgoCD 或 FluxCD 以 GitOps 方式部署。ChaosScheduler 支持 CronJob 式的定期实验。

## 生产使用场景

1. **弹性验证**: 在游戏日（Game Day）中注入 Pod 故障，验证自动恢复能力
2. **CI/CD 集成**: 在部署后自动运行混沌实验，确保新版本的弹性不退化
3. **多区域容灾**: 注入网络分区，验证跨区域故障切换
4. **容量规划**: 注入 CPU/内存压力，验证系统在高负载下的表现

## 安装与配置

```bash
# Helm 安装 LitmusChaos 控制平面
helm repo add litmuschaos https://litmuschaos.github.io/litmus-helm/
helm install litmus litmuschaos/litmus \
  --namespace litmus --create-namespace \
  --set portal.frontend.service.type=LoadBalancer
# 等待控制平面就绪
kubectl wait --for=condition=available deployment/litmus-frontend -n litmus --timeout=300s
# 安装混沌实验（从 ChaosHub）
kubectl apply -f https://hub.litmuschaos.io/api/chaos/3.6.0?file=charts/generic/pod-delete/experiment.yaml
kubectl apply -f https://hub.litmuschaos.io/api/chaos/3.6.0?file=charts/generic/network-delay/experiment.yaml
```

```yaml
# ChaosEngine CRD 示例
apiVersion: litmuschaos.io/v1alpha1
kind: ChaosEngine
metadata:
  name: pod-kill-payment
  namespace: production
spec:
  appinfo:
    appns: production
    applabel: app=payment-service
  annotationCheck: 'true'
  engineState: active
  chaosServiceAccount: litmus-admin
  monitoring: true
  jobCleanUpPolicy: retain
  experiments:
  - name: pod-delete
    spec:
      probe:
      - name: check-payment-health
        type: httpProbe
        httpProbe/inputs:
          url: http://payment-service:8080/health
          insecureSkipVerify: false
          method:
            get:
              criteria: ==
              responseCode: "200"
        mode: Continuous
      components:
        env:
        - name: TOTAL_CHAOS_DURATION
          value: "30"
        - name: CHAOS_INTERVAL
          value: "10"
        - name: FORCE
          value: "true"
---
# ChaosSchedule（定期实验）
apiVersion: litmuschaos.io/v1alpha1
kind: ChaosSchedule
metadata:
  name: weekly-resilience-test
spec:
  schedule:
    repeat:
      timeRange:
        startTime: "2026-01-01T02:00:00Z"
        endTime: "2026-12-31T06:00:00Z"
      properties:
        minChaosInterval: 168h  # 每周一次
  engineTemplateSpec:
    appinfo:
      appns: production
      applabel: app=critical-service
    chaosServiceAccount: litmus-admin
    experiments:
    - name: pod-delete
      spec:
        components:
          env:
          - name: TOTAL_CHAOS_DURATION
            value: "60"
```

```yaml
# RBAC 配置（限制实验权限范围）
apiVersion: rbac.authorization.k8s.io/v1
kind: Role
metadata:
  name: chaos-limited-role
  namespace: production
rules:
- apiGroups: ["litmuschaos.io"]
  resources: ["chaosengines", "chaosexperiments", "chaosresults"]
  verbs: ["*"]
- apiGroups: [""]
  resources: ["pods", "events"]
  verbs: ["get", "list", "delete"]
- apiGroups: ["apps"]
  resources: ["deployments", "replicasets"]
  verbs: ["get", "list"]
```

## 运维操作

```bash
# 🟢 低风险：查看实验状态
kubectl get chaosengine -A
kubectl get chaosresult -A
kubectl describe chaosengine pod-kill-payment -n production

# 🟢 低风险：查看实验结果
kubectl get chaosresult pod-kill-payment-pod-delete -n production -o yaml
kubectl logs -l name=chaos-runner -n production

# 🟡 中风险：暂停/恢复实验
kubectl patch chaosengine pod-kill-payment -n production --type merge -p '{"spec":{"engineState":"stop"}}'
kubectl patch chaosengine pod-kill-payment -n production --type merge -p '{"spec":{"engineState":"active"}}'

# 🟡 中风险：删除实验（停止所有注入）
kubectl delete chaosengine pod-kill-payment -n production

# 🔴 高风险：强制清理所有混沌资源
kubectl delete chaosengine --all -A
kubectl delete chaosrunner --all -A

# 🟢 低风险：查看 ChaosHub 可用实验
kubectl get chaosexperiment -A
```

## 故障排查

| 症状 | 可能原因 | 排查命令 | 修复方案 |
|------|----------|----------|----------|
| ChaosEngine 卡在 Waiting | RBAC 权限不足 | `kubectl describe chaosengine <name>` | 检查 ServiceAccount 权限 |
| Runner Pod CrashLoopBackOff | 目标应用标签不匹配 | `kubectl logs <runner-pod>` | 修正 appinfo.applabel |
| 实验未注入故障 | annotationCheck 失败 | `kubectl get pods -l app=x -o jsonpath='{.items[*].metadata.annotations}'` | 添加 `litmuschaos.io/chaos: "true"` annotation |
| Probe 持续失败 | 健康检查 URL 不可达 | `kubectl exec -it <pod> -- curl <probe-url>` | 修正 probe URL 或 criteria |
| ChaosResult 显示 Fail | 稳态假设被违反 | `kubectl get chaosresult <name> -o yaml` | 分析失败探针，修复系统弹性问题 |
| 控制平面无法访问 | MongoDB 连接失败 | `kubectl logs deploy/litmus-server -n litmus` | 检查 MongoDB 连接串和 Secret |

```
排查流程：
├── ChaosEngine 状态异常？
│   ├── kubectl describe chaosengine → 查看 Events
│   ├── 检查 ServiceAccount 是否存在且权限足够
│   └── 检查目标 Namespace 的 RBAC
├── Runner Pod 异常？
│   ├── kubectl logs <runner-pod> → 查看注入日志
│   ├── 检查目标 Pod 是否存在且标签匹配
│   └── 检查网络策略是否阻止 Runner 访问目标
└── 实验结果异常？
    ├── kubectl get chaosresult → 查看探针结果
    ├── 对比实验前后系统指标
    └── 检查 Probe 配置是否合理
```

## 生产案例

### 案例 1：Game Day Pod Kill 验证自动恢复

- **场景**：电商平台大促前进行弹性验证，注入 Pod Kill 测试订单服务自动恢复
- **排查**：配置 ChaosEngine 目标为 `app=order-service`，设置 `TOTAL_CHAOS_DURATION=60s`，添加 HTTP Probe 持续检测 `/health` 端点
- **方案**：实验发现 Pod 被杀后 45s 才恢复（超过 SLO 30s），原因是 readinessProbe 的 initialDelaySeconds 过大（30s），调整为 5s 后恢复时间降至 12s
- **效果**：大促期间零故障，订单服务 P99 延迟保持在 200ms 以内

### 案例 2：网络分区验证跨区域容灾

- **场景**：金融系统双活架构，验证 AZ 间网络分区时的故障切换能力
- **排查**：使用 network-partition 实验注入 30s 网络隔离，观察流量切换和数据库一致性
- **方案**：发现网络分区后 DNS 切换耗时 60s（TTL 过大），将 CoreDNS TTL 从 300s 调整为 30s，同时启用 Istio 的 outlierDetection 实现秒级故障转移
- **效果**：故障切换时间从 60s 降至 8s，满足 RTO < 15s 的合规要求

## 替代方案

| 维度 | LitmusChaos | Chaos Mesh | Gremlin | Pumba |
|------|-------------|------------|---------|-------|
| CNCF 状态 | Incubating | Incubating | 商业 | 社区 |
| UI | Web Portal 完善 | Dashboard | 企业级 | 无 |
| 实验数量 | 50+ ChaosHub | 30+ | 20+ | 5+ |
| GitOps | 原生支持 | 支持 | 有限 | 不支持 |
| 资源开销 | 中等 | 较重 | Agent 轻量 | 极低 |
| 适用场景 | 企业级混沌平台 | 中文社区/云原生 | 商业合规 | 开发测试 |

## 架构定位

在 CNCF 生态中，LitmusChaos 属于 **Observability / Reliability Engineering** 类别，是云原生混沌工程的两大主流平台之一（与 Chaos Mesh 并列）。

## 参考链接

- [[23-实体/07-可观测性/prometheus-grafana.md|prometheus-grafana]]
- [[23-实体/08-交付与制品/argocd.md|argocd]]
- [[deployment]]
- [[23-实体/02-K8s核心组件/crd-custom-resources.md|crd-custom-resources]]
- [[operator-pattern]]

## Related

- [[openkruise]] — OpenKruise
- [[02-istio-advanced-traffic-management]] — Istio 高级流量管理
- [[vscode-kubernetes-tools]] — VS Code Kubernetes Tools
- [[prometheus]] — Prometheus
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- litmus
- [[23-实体/15-参考与索引/k8s-observability-ecosystem.md|可观测性体系：指标、日志、链路追踪与混沌工程]] — Cross-reference
- [[23-实体/15-参考与索引/operations-terms.md|K8s 运维运营术语参考]] — Cross-reference
- [[23-实体/15-参考与索引/cncf-infrastructure.md|CNCF 基础设施与混沌工程项目全景]] — Cross-reference
- [[21-生态参考/03-领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
