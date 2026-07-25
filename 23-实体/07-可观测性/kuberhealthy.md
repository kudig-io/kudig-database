---
title: Kuberhealthy (entities)
description: '## 概述'
summary: 'Kuberhealthy 是一个 Kubernetes 综合健康检查和合成监控工具。'
category: entities
tags:
- k8s
- cncf
- observability
- kuberhealthy
- prometheus
- grafana
- daemonset
- job
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
- Kuberhealthy 是什么
- 如何 Kuberhealthy
trigger_keywords:
- Kuberhealthy
prerequisites:
- kubectl-basics
- prometheus-basics
- monitoring-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# Kuberhealthy

> **CNCF 状态**: Sandbox | **类别**: Observability | **主要语言**: Go

## 概述

Kuberhealthy 是一个 Kubernetes 综合健康检查和合成监控（Synthetic Monitoring）工具，由 Comcast 开发，2020 年加入 CNCF 沙箱。它通过运行 Kubernetes Job 来执行主动健康检查，将检查结果以 Prometheus 指标格式输出。与传统的被动监控不同，Kuberhealthy 采用合成监控方法——主动模拟用户行为来测试集群功能是否正常，如"创建一个 Deployment 并验证 Pod 是否 Running"、"解析一个 DNS 名称"、"挂载一个 PVC"等。这些检查以 khcheck CRD 声明式定义，支持自定义检查镜像，可以验证 DNS、部署、存储、网络等各方面的集群健康状态。

## 核心能力

- **合成监控**: 通过 Kubernetes Job 执行主动健康检查，模拟真实工作负载
- **丰富检查项**: 内置 DNS 解析、Deployment 创建、DaemonSet 部署、Pod 重启、PodStatus 等检查
- **自定义检查**: 使用任何容器镜像编写自定义检查逻辑
- **Prometheus 集成**: 检查结果直接导出为 Prometheus 指标（kuberhealthy_check）
- **CRD 配置**: 使用 khcheck/khstate CRD 声明式定义和管理检查
- **多命名空间**: 支持跨命名空间和集群范围的健康检查

## 架构

Kuberhealthy 采用 Controller + Check Job 模式：

- **Kuberhealthy Controller**: 核心控制器，管理所有 khcheck 资源的生命周期
- **khcheck CRD**: 声明式健康检查定义（检查镜像、运行频率、超时时间）
- **Check Pod (Job)**: Kuberhealthy Controller 根据 khcheck 创建的临时 Pod 执行检查
- **Check Protocol**: 检查 Pod 通过特定退出码（0=OK，1=Failure）和 stdout JSON 报告结果
- **State Storage**: khstate CRD 存储每个检查的当前状态（OK/Error/运行中）
- **Metrics Exporter**: 暴露 Prometheus 格式指标供 scrape

检查流程：`khcheck → Controller → Check Job (Pod) → 执行检查 → Exit Code → khstate → Prometheus`

## K8s 集成

Kuberhealthy 以 Helm Chart 部署在 Kubernetes 集群中。Controller 以 Deployment 运行，监听 khcheck CRD。每个 khcheck 定义了检查镜像和运行频率，Controller 定期创建 Check Pod（通过 Kubernetes Job）执行检查。Check Pod 执行完毕后通过退出码报告检查结果，Controller 更新对应的 khstate CRD。Prometheus 通过 scrape Kuberhealthy 的 metrics endpoint 获取所有检查的状态指标。与 [[22-概念/01-核心架构/kubernetes-architecture-overview.md|Kubernetes 架构]] 中的 Job、CronJob、ConfigMap 等原生资源深度集成。

## 生产场景

1. **集群功能验证**: 定期验证 DNS、网络、存储等关键集群功能是否正常
2. **SLA 合成监控**: 从用户视角主动测试"部署一个应用"是否成功，验证服务可用性
3. **自定义业务检查**: 编写自定义检查镜像验证特定业务逻辑（如"数据库连接是否正常"）
4. **多集群健康对比**: 在多个集群部署 Kuberhealthy，对比各集群的健康指标

## 安装与配置

```bash
# Helm 安装 Kuberhealthy
helm repo add kuberhealthy https://kuberhealthy.github.io/kuberhealthy/helm-repos
helm install kuberhealthy kuberhealthy/kuberhealthy \
  -n kuberhealthy --create-namespace \
  --set prometheus.enabled=true

# 部署内置检查
kubectl apply -f https://raw.githubusercontent.com/kuberhealthy/kuberhealthy/master/cmd/dns-resolution-check/dns-check.yaml
kubectl apply -f https://raw.githubusercontent.com/kuberhealthy/kuberhealthy/master/cmd/deployment-check/deployment-check.yaml
kubectl apply -f https://raw.githubusercontent.com/kuberhealthy/kuberhealthy/master/cmd/daemonset-check/daemonset-check.yaml

# 查看检查状态
kubectl get khstate -A
```

```yaml
# 自定义 khcheck CRD 示例
apiVersion: comcast.github.io/v1
kind: KuberhealthyCheck
metadata:
  name: database-connectivity
  namespace: kuberhealthy
spec:
  runInterval: 5m
  timeout: 2m
  podSpec:
    containers:
    - name: db-check
      image: my-registry.io/checks/db-connectivity:v1
      env:
      - name: DB_HOST
        value: postgres.production.svc.cluster.local
      - name: DB_PORT
        value: "5432"
      - name: DB_NAME
        valueFrom:
          secretKeyRef:
            name: db-check-credentials
            key: dbname
      resources:
        requests:
          cpu: 10m
          memory: 32Mi
        limits:
          cpu: 50m
          memory: 64Mi
    restartPolicy: Never
    serviceAccountName: kuberhealthy-check-sa
---
# DNS 检查示例
apiVersion: comcast.github.io/v1
kind: KuberhealthyCheck
metadata:
  name: dns-resolution-internal
  namespace: kuberhealthy
spec:
  runInterval: 2m
  timeout: 1m
  podSpec:
    containers:
    - name: dns-check
      image: kuberhealthy/dns-resolution-check:v1.5.0
      env:
      - name: HOSTNAME
        value: kubernetes.default.svc.cluster.local
      resources:
        requests:
          cpu: 10m
          memory: 16Mi
```

## 运维操作

```bash
# 🟢 低风险：查看所有检查状态
kubectl get khcheck -A
kubectl get khstate -A
kubectl describe khstate dns-resolution-internal -n kuberhealthy

# 🟢 低风险：查看检查指标
kubectl port-forward svc/kuberhealthy -n kuberhealthy 8080:80 &
curl -s http://localhost:8080/metrics | grep kuberhealthy_check

# 🟡 中风险：手动触发检查
kubectl annotate khcheck database-connectivity -n kuberhealthy \
  comcast.github.io/check-run=$(date +%s) --overwrite

# 🟡 中风险：暂停检查
kubectl patch khcheck database-connectivity -n kuberhealthy \
  --type merge -p '{"spec":{"paused":true}}'

# 🔴 高风险：删除检查
kubectl delete khcheck database-connectivity -n kuberhealthy

# 🟢 低风险：查看检查 Pod 日志
kubectl logs -l kuberhealthy-check-name=database-connectivity -n kuberhealthy
```

## 故障排查

| 症状 | 可能原因 | 排查命令 | 修复方案 |
|------|----------|----------|----------|
| 检查持续失败 | 目标服务不可用 | `kubectl describe khstate <name>` | 检查目标服务和网络连通性 |
| Check Pod 未创建 | RBAC 权限不足 | `kubectl logs deploy/kuberhealthy -n kuberhealthy` | 检查 ServiceAccount 权限 |
| 检查超时 | Pod 启动慢/资源不足 | `kubectl get pods -n kuberhealthy` | 增加 timeout 或资源请求 |
| 指标未导出 | Prometheus 配置错误 | `curl svc/kuberhealthy:80/metrics` | 检查 ServiceMonitor/PodMonitor |
| 误报（假阳性） | 检查逻辑不严谨 | `kubectl logs <check-pod>` | 调整检查参数或增加重试 |

```
排查流程：
├── 检查失败？
│   ├── kubectl get khstate → 查看状态和错误信息
│   ├── kubectl logs <check-pod> → 查看检查日志
│   └── 手动执行检查命令验证
├── 检查未运行？
│   ├── kubectl get khcheck → 确认检查存在且未暂停
│   ├── kubectl logs deploy/kuberhealthy → 查看控制器日志
│   └── 检查 RBAC 和命名空间权限
└── 指标异常？
    ├── curl metrics endpoint → 确认指标导出
    ├── 检查 Prometheus scrape 配置
    └── 对比 khstate 与指标一致性
```

## 生产案例

### 案例 1：集群功能 SLA 监控

- **场景**：平台团队需要向业务团队提供集群功能 SLA 报告（DNS、存储、部署能力）
- **排查**：传统监控只能被动发现故障，无法证明"部署一个应用"是否成功
- **方案**：部署 Kuberhealthy 检查（DNS + Deployment + DaemonSet + PVC），每 2 分钟执行一次，结果接入 Grafana SLA 仪表盘
- **效果**：SLA 报告自动化，提前发现 3 次 DNS 故障（影响 < 5min）

### 案例 2：自定义业务健康检查

- **场景**：金融交易系统需要验证"数据库连接 + 消息队列 + 缓存"全链路可用
- **排查**：单一组件监控无法反映端到端业务可用性
- **方案**：编写自定义检查镜像，模拟完整交易流程（连接 DB → 发送 MQ → 读取缓存），作为 khcheck 部署
- **效果**：业务可用性从 99.5% 提升至 99.95%，故障发现时间从 10min 缩短至 2min

## 对比

| 特性 | Kuberhealthy | Prometheus | Blackbox Exporter | Synthetic Monitoring |
|------|-------------|-----------|-------------------|---------------------|
| 合成监控 | ✅ K8s 原生 | ❌ 被动 | ✅ 外部探测 | ✅ |
| K8s 资源检查 | ✅ | ⚠️ | ❌ | ❌ |
| 自定义检查 | ✅ 任意镜像 | ⚠️ | ❌ | ⚠️ |
| CNCF 状态 | Sandbox | Graduated | 非 CNCF | 非 CNCF |

## 架构定位

在 CNCF 生态中，Kuberhealthy 属于 **Observability** 类别，为云原生应用提供合成监控和综合健康检查能力。

## 参考链接

- [[23-实体/07-可观测性/prometheus-grafana.md|prometheus-grafana]]
- [[deployment]]
- [[23-实体/02-K8s核心组件/crd-custom-resources.md|crd-custom-resources]]
- [[22-概念/06-可观测性/observability-pillars.md|observability-pillars]]
- [[pod-lifecycle]]

## Related

- [[kubefleet]] — KubeFleet
- [[kuma]] — Kuma
- [[deployment]] — Deployment
- [[prometheus]] — Prometheus
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- kuberhealthy
- [[23-实体/15-参考与索引/cncf-infrastructure.md|CNCF 基础设施与混沌工程项目全景]] — Cross-reference
- [[21-生态参考/03-领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
