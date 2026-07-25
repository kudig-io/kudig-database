---
title: Litmus 混沌工程实践
description: '# Litmus 混沌工程实践'
summary: 'kubectl apply -f https://litmuschaos.github.io/litmus/3.12.0/litmus-3.12.0.yaml'
category: domain
tags:
- litmus
- chaos-engineering
- kubernetes
- ci-cd
- agent
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Litmus 混沌工程实践 是什么
- 如何 Litmus 混沌工程实践
- Kubernetes 09 reliability engineering 最佳实践
trigger_keywords:
- Litmus
- 混沌工程实践
- reliability
- engineering
prerequisites:
- kubectl-basics
- sre-practices
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# [[Litmus|Litmus]] 混沌工程实践

## Litmus vs Chaos Mesh

| 特性 | Litmus | Chaos Mesh |
|------|--------|-----------|
| 项目归属 | CNCF 孵化项目 | PingCAP |
| 实验编排 | [[Argo|Argo]]go Workflows|Argo Workflows]] 原生 | 自定义 Workflow |
| 多集群 | 原生支持 | 需额外配置 |
| GitOps | 原生支持 | 有限 |
| 社区 | 活跃，企业采用多 | 活跃，中国社区大 |

## 核心概念

```
Litmus 架构:
├── ChaosExperiment: 定义实验（问题类型、参数）
├── ChaosEngine: 将实验绑定到应用
├── ChaosResult: 实验结果
└── ChaosCenter: 控制平面（Web UI + API）
```

## 安装

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 安装 ChaosCenter
kubectl apply -f https://litmuschaos.github.io/litmus/3.12.0/litmus-3.12.0.yaml

# 安装 ChaosAgent（目标集群）
litmusctl agent connect \
  --agent-name="prod-cluster" \
  --project-id="$PROJECT_ID" \
  --installation-mode="namespace" \
  --namespace="litmus"
```
## 实验示例

```yaml
# ChaosEngine: 对 nginx 注入 Pod 删除
apiVersion: litmuschaos.io/v1alpha1
kind: ChaosEngine
metadata:
  name: nginx-chaos
  namespace: default
spec:
  appinfo:
    appns: 'default'
    applabel: 'app=nginx'
    appkind: 'deployment'
  annotationCheck: 'true'
  engineState: 'active'
  chaosServiceAccount: pod-delete-sa
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
          probe:
            - name: "check-nginx-access"
              type: "httpProbe"
              mode: "Continuous"
              runProperties:
                probeTimeout: "5s"
                retry: 2
                interval: "5s"
                probePollingInterval: "2s"
                initialDelay: "2s"
              httpProbe/inputs:
                url: "http://nginx.default.svc.cluster.local"
                insecureSkipVerify: false
                method:
                  get:
                    criteria: "=="
                    responseCode: "200"
```

## GitOps 集成

```yaml
# 与 Argo CD 集成，自动执行混沌实验
apiVersion: argoproj.io/v1alpha1
kind: Workflow
metadata:
  name: chaos-pipeline
spec:
  entrypoint: chaos-tests
  templates:
    - name: chaos-tests
      steps:
        - - name: deploy-app
            template: deploy
        - - name: run-chaos
            template: litmus-experiment
        - - name: verify-slo
            template: slo-check
```

## 生产环境部署

### Helm 安装 ChaosCenter

```bash
# 🟡 中风险：Helm 安装 Litmus ChaosCenter
helm repo add litmuschaos https://litmuschaos.github.io/litmus-helm/
helm repo update

# 创建命名空间
kubectl create namespace litmus

# 安装 ChaosCenter (生产配置)
helm install chaos litmuschaos/litmus \
  --namespace litmus \
  --set portal.frontend.service.type=LoadBalancer \
  --set portal.server.service.type=ClusterIP \
  --set mongodb.persistence.enabled=true \
  --set mongodb.persistence.size=20Gi \
  --version 3.12.0

# 验证安装
kubectl get pods -n litmus
kubectl get svc -n litmus
```

### 生产环境 Values

```yaml
# litmus-values.yaml
portal:
  frontend:
    replicas: 2
    resources:
      requests:
        cpu: 100m
        memory: 128Mi
      limits:
        cpu: 500m
        memory: 256Mi
  server:
    replicas: 2
    resources:
      requests:
        cpu: 200m
        memory: 256Mi
      limits:
        cpu: 1000m
        memory: 512Mi

mongodb:
  persistence:
    enabled: true
    size: 20Gi
    storageClass: alicloud-disk-ssd

# 认证配置
auth:
  adminPassword: "${LITMUS_ADMIN_PASSWORD}"
  adminEmail: "sre@example.com"
```

### RBAC 配置

```yaml
# 实验专用 ServiceAccount
apiVersion: v1
kind: ServiceAccount
metadata:
  name: chaos-experiment-sa
  namespace: production
---
apiVersion: rbac.authorization.k8s.io/v1
kind: Role
metadata:
  name: chaos-experiment-role
  namespace: production
rules:
  - apiGroups: [""]
    resources: ["pods", "pods/exec", "pods/log"]
    verbs: ["get", "list", "delete", "create"]
  - apiGroups: ["apps"]
    resources: ["deployments", "statefulsets", "replicasets"]
    verbs: ["get", "list", "patch"]
  - apiGroups: ["litmuschaos.io"]
    resources: ["*"]
    verbs: ["*"]
---
apiVersion: rbac.authorization.k8s.io/v1
kind: RoleBinding
metadata:
  name: chaos-experiment-binding
  namespace: production
subjects:
  - kind: ServiceAccount
    name: chaos-experiment-sa
    namespace: production
roleRef:
  kind: Role
  name: chaos-experiment-role
  apiGroup: rbac.authorization.k8s.io
```

## 高级实验示例

### 网络延迟注入

```yaml
apiVersion: litmuschaos.io/v1alpha1
kind: ChaosEngine
metadata:
  name: network-latency-experiment
  namespace: production
spec:
  appinfo:
    appns: 'production'
    applabel: 'app=api-service'
    appkind: 'deployment'
  engineState: 'active'
  chaosServiceAccount: chaos-experiment-sa
  experiments:
    - name: pod-network-latency
      spec:
        components:
          env:
            - name: TOTAL_CHAOS_DURATION
              value: '60'
            - name: NETWORK_LATENCY
              value: '200'  # ms
            - name: JITTER
              value: '50'   # ms
            - name: CONTAINER_RUNTIME
              value: 'containerd'
            - name: SOCKET_PATH
              value: '/run/containerd/containerd.sock'
          probe:
            - name: "check-api-latency"
              type: "httpProbe"
              mode: "Continuous"
              runProperties:
                probeTimeout: "10s"
                retry: 3
                interval: "5s"
              httpProbe/inputs:
                url: "http://api-service.production.svc.cluster.local/health"
                method:
                  get:
                    criteria: "=="
                    responseCode: "200"
```

### CPU/内存压力测试

```yaml
apiVersion: litmuschaos.io/v1alpha1
kind: ChaosEngine
metadata:
  name: stress-experiment
  namespace: production
spec:
  appinfo:
    appns: 'production'
    applabel: 'app=worker'
    appkind: 'deployment'
  engineState: 'active'
  chaosServiceAccount: chaos-experiment-sa
  experiments:
    - name: pod-cpu-hog
      spec:
        components:
          env:
            - name: TOTAL_CHAOS_DURATION
              value: '120'
            - name: CPU_CORES
              value: '2'
            - name: CPU_LOAD
              value: '80'  # 百分比
          probe:
            - name: "check-hpa-scaling"
              type: "cmdProbe"
              mode: "Continuous"
              runProperties:
                probeTimeout: "5s"
                interval: "10s"
              cmdProbe/inputs:
                command: "kubectl get hpa worker-hpa -n production -o jsonpath='{.status.currentReplicas}'"
                comparator:
                  type: "int"
                  criteria: ">="
                  value: "3"
```

### DNS 故障模拟

```yaml
apiVersion: litmuschaos.io/v1alpha1
kind: ChaosEngine
metadata:
  name: dns-chaos
  namespace: production
spec:
  appinfo:
    appns: 'production'
    applabel: 'app=api-service'
    appkind: 'deployment'
  engineState: 'active'
  chaosServiceAccount: chaos-experiment-sa
  experiments:
    - name: pod-dns-error
      spec:
        components:
          env:
            - name: TOTAL_CHAOS_DURATION
              value: '30'
            - name: TARGET_HOSTNAMES
              value: 'database.production.svc.cluster.local'
            - name: MATCH_SCHEME
              value: 'exact'
```

## 监控与告警

### ServiceMonitor 配置

```yaml
apiVersion: monitoring.coreos.com/v1
kind: ServiceMonitor
metadata:
  name: litmus-monitor
  namespace: litmus
spec:
  selector:
    matchLabels:
      app: chaos-exporter
  endpoints:
    - port: metrics
      interval: 30s
```

### PrometheusRule 告警

```yaml
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: litmus-alerts
  namespace: monitoring
spec:
  groups:
    - name: litmus.rules
      rules:
        # 实验失败告警
        - alert: ChaosExperimentFailed
          expr: |
            litmuschaos_experiment_status{status="Failed"} == 1
          for: 1m
          labels:
            severity: warning
          annotations:
            summary: "混沌实验 {{ $labels.experiment_name }} 失败"

        # 探针失败告警
        - alert: ChaosProbeFailed
          expr: |
            litmuschaos_experiment_probe_success_rate < 0.9
          for: 2m
          labels:
            severity: critical
          annotations:
            summary: "实验 {{ $labels.experiment_name }} 探针成功率低于 90%"

        # 实验超时
        - alert: ChaosExperimentTimeout
          expr: |
            litmuschaos_experiment_duration_seconds > 600
          for: 1m
          labels:
            severity: warning
          annotations:
            summary: "实验 {{ $labels.experiment_name }} 运行超过 10 分钟"
```

### Grafana Dashboard

```json
{
  "title": "Litmus Chaos Experiments",
  "panels": [
    {
      "title": "实验状态",
      "type": "stat",
      "targets": [{"expr": "litmuschaos_experiment_status"}]
    },
    {
      "title": "探针成功率",
      "type": "gauge",
      "targets": [{"expr": "litmuschaos_experiment_probe_success_rate"}]
    },
    {
      "title": "实验历史",
      "type": "table",
      "targets": [{"expr": "litmuschaos_experiment_count"}]
    }
  ]
}
```

## 多集群管理

### 连接远程集群

```bash
# 🟡 中风险：连接远程集群到 ChaosCenter
litmusctl agent connect \
  --agent-name="prod-cluster-01" \
  --project-id="$PROJECT_ID" \
  --installation-mode="namespace" \
  --namespace="litmus-agent" \
  --kubeconfig="$HOME/.kube/prod-cluster-01"

# 验证连接
litmusctl agent list
```

### 多集群实验编排

```yaml
apiVersion: litmuschaos.io/v1alpha1
kind: Workflow
metadata:
  name: multi-cluster-chaos
  namespace: litmus
spec:
  experiments:
    - name: cluster-01-pod-delete
      agent: prod-cluster-01
      spec:
        appinfo:
          appns: 'production'
          applabel: 'app=api'
    - name: cluster-02-network-latency
      agent: prod-cluster-02
      spec:
        appinfo:
          appns: 'production'
          applabel: 'app=api'
```

## 故障排查

### 常见问题

| 问题 | 原因 | 解决方案 |
|-----|------|----------|
| 实验卡在 Waiting | RBAC 权限不足 | 检查 ServiceAccount 权限 |
| 探针失败 | 目标服务不可达 | 检查网络连通性 |
| 实验超时 | 资源不足 | 检查节点资源 |
| Agent 断开 | 网络问题 | 检查 Agent 日志 |

### 调试命令

```bash
# 🟢 低风险：查看实验状态
kubectl get chaosengine -A
kubectl get chaosresult -A

# 🟢 低风险：查看实验日志
kubectl logs -n litmus -l app=chaos-operator -f

# 🟢 低风险：查看 Runner 日志
kubectl logs -n production -l app=chaos-runner -f

# 🟡 中风险：强制停止实验
kubectl patch chaosengine <name> -n <ns> --type='json' \
  -p='[{"op": "replace", "path": "/spec/engineState", "value": "stop"}]'
```

## 相关

- [[12-可靠性/04-混沌工程/01-chaos-engineering-overview.md|01 chaos engineering overview]]
- [[12-可靠性/04-混沌工程/02-chaos-mesh-deployment.md|02 chaos mesh deployment]]

```

<!-- risk-assessed -->
