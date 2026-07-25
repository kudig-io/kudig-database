---
title: Chaos Mesh 企业级部署
description: '# Chaos Mesh 企业级部署'
summary: '# Chaos Mesh 企业级部署'
category: domain
tags:
- chaos-mesh
- chaos-engineering
- kubernetes
- deployment
- controller-manager
- helm
- containerd
- daemonset
- rbac
- crd
tier: supporting
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Chaos Mesh 企业级部署 是什么
- 如何 Chaos Mesh 企业级部署
- Kubernetes 09 reliability engineering 最佳实践
trigger_keywords:
- Chaos
- Mesh
- 企业级部署
- reliability
- engineering
prerequisites:
- kubectl-basics
- sre-practices
- helm-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Chaos Mesh 企业级部署

## 架构组件

```
Chaos Mesh 架构:
├── chaos-operator-manager: 管理 CRD 和控制器生命周期
├── chaos-daemon: DaemonSet，在每个节点上执行实际故障注入
├── chaos-dashboard: Web UI 和 API 服务
└── chaos-mesh-controller-manager: 核心控制器
```

## [[Helm|Helm]] 安装

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `helm upgrade/install`：部署/升级 release

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 添加 Helm repo
helm repo add chaos-mesh https://charts.chaos-mesh.org
helm repo update

# 安装（生产环境配置）
helm install chaos-mesh chaos-mesh/chaos-mesh \
  --namespace chaos-mesh \
  --create-namespace \
  --set chaosDaemon.runtime=containerd \
  --set chaosDaemon.socketPath=/run/containerd/containerd.sock \
  --set dashboard.securityMode=true \
  --set controllerManager.enableFilterNamespace=true
```
## 安全加固

```yaml
# 启用 RBAC 和多租户
apiVersion: rbac.authorization.k8s.io/v1
kind: Role
metadata:
  name: chaos-experimenter
rules:
- apiGroups: ["chaos-mesh.org"]
  resources: ["*"]
  verbs: ["get", "list", "create", "delete"]
---
apiVersion: rbac.authorization.k8s.io/v1
kind: RoleBinding
metadata:
  name: chaos-experimenter-binding
subjects:
- kind: ServiceAccount
  name: chaos-experimenter
  namespace: default
roleRef:
  kind: Role
  name: chaos-experimenter
  apiGroup: rbac.authorization.k8s.io
```

## 实验类型清单

| 类型 | 说明 | 安全级别 |
|------|------|---------|
| PodChaos | Pod 问题/终止/容器重启 | 中 |
| NetworkChaos | 网络延迟/丢包/分区 | 高 |
| IOChaos | 文件系统 I/O 问题 | 中 |
| StressChaos | CPU/内存压力测试 | 中 |
| DNSChaos | DNS 问题 | 高 |
| TimeChaos | 时间偏移 | 低 |
| HTTPChaos | HTTP 请求/响应篡改 | 高 |
| JVMChaos | JVM 级别问题 | 中 |

## 生产环境配置最佳实践

### Helm Values 生产配置

```yaml
# chaos-mesh-values.yaml
# 生产环境推荐配置

# 控制器配置
controllerManager:
  replicas: 2  # 高可用
  resources:
    requests:
      cpu: 500m
      memory: 512Mi
    limits:
      cpu: 1000m
      memory: 1Gi
  # 启用命名空间过滤
  enableFilterNamespace: true
  # 允许的命名空间
  targetNamespace: ""
  # 忽略的命名空间
  ignoredNamespaces:
    - kube-system
    - kube-public
    - monitoring

# Chaos Daemon 配置
chaosDaemon:
  runtime: containerd
  socketPath: /run/containerd/containerd.sock
  resources:
    requests:
      cpu: 250m
      memory: 256Mi
    limits:
      cpu: 500m
      memory: 512Mi
  # 特权模式（某些实验需要）
  privileged: false
  # 主机网络
  hostNetwork: false

# Dashboard 配置
dashboard:
  create: true
  replicas: 2
  securityMode: true  # 启用安全模式
  resources:
    requests:
      cpu: 250m
      memory: 256Mi
    limits:
      cpu: 500m
      memory: 512Mi
  # 持久化存储
  persistentVolume:
    enabled: true
    size: 10Gi

# 指标配置
metrics:
  enabled: true
  serviceMonitor:
    enabled: true
    interval: 30s

# Webhook 配置
webhook:
  timeoutSeconds: 5
  failurePolicy: Ignore  # 避免影响正常 Pod 创建
```

### 安装命令

```bash
# 🟡 中风险：生产环境安装
helm install chaos-mesh chaos-mesh/chaos-mesh \
  --namespace chaos-mesh \
  --create-namespace \
  -f chaos-mesh-values.yaml \
  --wait \
  --timeout 10m

# 验证安装
kubectl get pods -n chaos-mesh
kubectl get crd | grep chaos-mesh
```

## 多租户配置

### 命名空间隔离

```yaml
# 为每个团队创建独立的实验命名空间
apiVersion: v1
kind: Namespace
metadata:
  name: chaos-team-a
  labels:
    team: team-a
    chaos-mesh.org/enable: "true"
---
apiVersion: v1
kind: Namespace
metadata:
  name: chaos-team-b
  labels:
    team: team-b
    chaos-mesh.org/enable: "true"
---
# 团队 A 的 RBAC
apiVersion: rbac.authorization.k8s.io/v1
kind: Role
metadata:
  name: chaos-experimenter
  namespace: chaos-team-a
rules:
  - apiGroups: ["chaos-mesh.org"]
    resources: ["*"]
    verbs: ["get", "list", "watch", "create", "update", "delete"]
  - apiGroups: [""]
    resources: ["pods", "pods/log"]
    verbs: ["get", "list", "watch"]
---
apiVersion: rbac.authorization.k8s.io/v1
kind: RoleBinding
metadata:
  name: chaos-experimenter-binding
  namespace: chaos-team-a
subjects:
  - kind: Group
    name: team-a-engineers
    apiGroup: rbac.authorization.k8s.io
roleRef:
  kind: Role
  name: chaos-experimenter
  apiGroup: rbac.authorization.k8s.io
```

### 实验审批流程

```yaml
# 使用 OPA/Gatekeeper 强制审批
apiVersion: constraints.gatekeeper.sh/v1beta1
kind: ChaosExperimentApproval
metadata:
  name: require-approval
spec:
  match:
    kinds:
      - apiGroups: ["chaos-mesh.org"]
        kinds: ["PodChaos", "NetworkChaos", "StressChaos"]
  parameters:
    # 需要审批的实验类型
    requireApproval:
      - NetworkChaos
      - KernelChaos
      - HostChaos
    # 允许的命名空间
    allowedNamespaces:
      - chaos-team-a
      - chaos-team-b
    # 最大影响比例
    maxImpactPercent: 50
```

## 实验配置示例详解

### PodChaos 完整示例

```yaml
apiVersion: chaos-mesh.org/v1alpha1
kind: PodChaos
metadata:
  name: pod-failure-example
  namespace: production
  annotations:
    # 实验元数据
    experiment.owner: "sre-team"
    experiment.ticket: "CHAOS-123"
    experiment.description: "测试 payment-api 在 Pod 故障时的恢复能力"
spec:
  # 故障类型
  action: pod-failure  # pod-kill | pod-failure | container-kill
  
  # 影响模式
  mode: fixed-percent  # one | all | fixed | fixed-percent | random-max-percent
  value: "30"  # 影响 30% 的 Pod
  
  # 目标选择器
  selector:
    namespaces:
      - production
    labelSelectors:
      app: payment-api
      tier: backend
    # 排除特定 Pod
    expressionSelectors:
      - key: chaos-exclude
        operator: DoesNotExist
  
  # 调度配置
  scheduler:
    cron: "@every 30m"  # 每 30 分钟执行
  
  # 持续时间
  duration: "60s"
  
  # 优雅终止
  gracePeriod: 30
```

### NetworkChaos 完整示例

```yaml
apiVersion: chaos-mesh.org/v1alpha1
kind: NetworkChaos
metadata:
  name: network-partition-example
  namespace: production
spec:
  # 故障类型
  action: partition  # delay | loss | duplicate | corrupt | partition | bandwidth
  
  # 影响模式
  mode: all
  
  # 目标选择器
  selector:
    namespaces:
      - production
    labelSelectors:
      app: order-service
  
  # 分区配置
  direction: both  # to | from | both
  target:
    selector:
      namespaces:
        - production
      labelSelectors:
        app: payment-api
    mode: all
  
  # 持续时间
  duration: "120s"
---
# 网络延迟示例
apiVersion: chaos-mesh.org/v1alpha1
kind: NetworkChaos
metadata:
  name: network-delay-example
  namespace: production
spec:
  action: delay
  mode: one
  selector:
    namespaces:
      - production
    labelSelectors:
      app: user-service
  delay:
    latency: "200ms"
    correlation: "50"
    jitter: "50ms"
  direction: to
  duration: "60s"
```

## 监控与告警集成

### ServiceMonitor 配置

```yaml
apiVersion: monitoring.coreos.com/v1
kind: ServiceMonitor
metadata:
  name: chaos-mesh-monitor
  namespace: monitoring
spec:
  selector:
    matchLabels:
      app.kubernetes.io/name: chaos-mesh
  namespaceSelector:
    matchNames:
      - chaos-mesh
  endpoints:
    - port: metrics
      interval: 30s
      path: /metrics
```

### PrometheusRule 告警

```yaml
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: chaos-mesh-alerts
  namespace: monitoring
spec:
  groups:
    - name: chaos-mesh.rules
      rules:
        # 实验运行时间过长
        - alert: ChaosExperimentRunningTooLong
          expr: |
            chaos_experiment_duration_seconds > 3600
          for: 5m
          labels:
            severity: warning
          annotations:
            summary: "混沌实验 {{ $labels.name }} 运行超过 1 小时"

        # 实验失败率高
        - alert: ChaosExperimentHighFailureRate
          expr: |
            rate(chaos_experiment_failed_total[5m]) > 0.1
          for: 5m
          labels:
            severity: warning
          annotations:
            summary: "混沌实验失败率过高"

        # 意外实验（无标签）
        - alert: UnlabeledChaosExperiment
          expr: |
            chaos_experiment_running{experiment_owner=""} == 1
          for: 1m
          labels:
            severity: critical
          annotations:
            summary: "检测到无主混沌实验: {{ $labels.name }}"
```

### Grafana Dashboard

| 面板 | 指标 | 用途 |
|-----|------|------|
| 活动实验数 | `chaos_experiment_running` | 当前运行的实验 |
| 实验历史 | `chaos_experiment_total` | 实验执行历史 |
| 实验成功率 | `chaos_experiment_success_total / chaos_experiment_total` | 实验成功率 |
| 受影响 Pod | `chaos_affected_pods` | 受影响的 Pod 数量 |

## 故障排查

### 常见问题

| 问题 | 可能原因 | 解决方案 |
|-----|---------|----------|
| 实验未生效 | selector 不匹配 | 检查 labelSelectors 和 namespaces |
| chaos-daemon 崩溃 | 权限不足 | 检查 privileged 和 RBAC |
| Dashboard 无法访问 | Service 类型 | 改为 LoadBalancer 或 port-forward |
| 实验无法停止 | finalizer 卡住 | 手动移除 finalizer |
| Webhook 拒绝 Pod | failurePolicy=Fail | 改为 Ignore 或修复 webhook |

### 调试命令

```bash
# 🟢 低风险：检查 Chaos Mesh 状态
kubectl get pods -n chaos-mesh
kubectl logs -n chaos-mesh deploy/chaos-controller-manager --tail=100
kubectl logs -n chaos-mesh ds/chaos-daemon --tail=100

# 🟢 低风险：检查实验状态
kubectl get podchaos,networkchaos,stresschaos -A
kubectl describe podchaos <name> -n <namespace>

# 🟡 中风险：强制停止实验
kubectl patch podchaos <name> -n <ns> -p '{"metadata":{"finalizers":null}}'
kubectl delete podchaos <name> -n <ns> --force --grace-period=0

# 🟢 低风险：检查 CRD
kubectl get crd | grep chaos-mesh
kubectl api-resources | grep chaos
```

## 升级与维护

### 升级流程

```bash
# 🟡 中风险：升级 Chaos Mesh
# 1. 备份当前配置
helm get values chaos-mesh -n chaos-mesh > chaos-mesh-values-backup.yaml

# 2. 停止所有实验
kubectl delete podchaos,networkchaos,stresschaos --all -A

# 3. 更新 Helm repo
helm repo update

# 4. 执行升级
helm upgrade chaos-mesh chaos-mesh/chaos-mesh \
  --namespace chaos-mesh \
  -f chaos-mesh-values.yaml \
  --wait

# 5. 验证
kubectl get pods -n chaos-mesh
kubectl get crd | grep chaos-mesh
```

### 版本兼容性

| Chaos Mesh | Kubernetes | containerd | 备注 |
|-----------|-----------|-----------|------|
| v2.5+ | v1.24+ | 1.6+ | 推荐版本 |
| v2.6+ | v1.26+ | 1.7+ | 支持新特性 |
| v2.7+ | v1.28+ | 2.0+ | 最新稳定版 |

## 相关

- [[12-可靠性/04-混沌工程/01-chaos-engineering-overview.md|01 chaos engineering overview]]
- [[12-可靠性/04-混沌工程/03-chaos-experiment-design.md|03 chaos experiment design]]

```

<!-- risk-assessed -->
