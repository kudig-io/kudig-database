---
title: 32 - API聚合层配置
description: '# 32 - API聚合层配置'
summary: 'opts := options.NewSecureServingOptions()'
category: platform-ops
tags:
- k8s
- platform
- operations
- devops
- apiserver
- prometheus
- rbac
- crd
- operator
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- SRE
- 平台工程师
- 运维工程师
estimated_read_time: 5min
intent_queries:
- API聚合层配置 是什么
- 如何 API聚合层配置
- Kubernetes 9 platform ops 最佳实践
trigger_keywords:
- API聚合层配置
- platform
- ops
prerequisites:
- kubectl-basics
- platform-engineering-basics
- prometheus-basics
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
authors:
- name: KUDIG Team
  role: contributor
cross_refs:
- type: domain
  path: ../可观测性/
  label: '相关知识域: 可观测性'
- type: domain
  path: ../专项技术/
  label: '相关知识域: 专项技术'
- type: domain
  path: ../故障诊断/
  label: '相关知识域: 故障诊断'
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 32 - API聚合层配置

<!-- chunk: API聚合架构 -->
## API聚合架构

| 组件 | 功能 | 说明 |
|-----|------|------|
| kube-aggregator | API路由 | 内置于kube-apiserver |
| APIService | 服务注册 | 声明API组/版本 |
| Extension API Server | 扩展服务器 | 自定义API实现 |

<!-- chunk: APIService配置 -->
## APIService配置

| 字段 | 类型 | 说明 |
|-----|-----|------|
| `spec.group` | string | API组名 |
| `spec.version` | string | API版本 |
| `spec.[[Service|service]].name` | string | 后端服务名 |
| `spec.service.namespace` | string | 后端服务命名空间 |
| `spec.service.port` | int | 后端服务端口(默认443) |
| `spec.caBundle` | []byte | CA证书 |
| `spec.groupPriorityMinimum` | int | 组优先级最小值 |
| `spec.versionPriority` | int | 版本优先级 |
| `spec.insecureSkipTLSVerify` | bool | 跳过TLS验证(不推荐) |

<!-- chunk: APIService示例 -->
## APIService示例

```yaml
apiVersion: apiregistration.k8s.io/v1
kind: APIService
metadata:
  name: v1beta1.metrics.k8s.io
spec:
  service:
    name: metrics-server
    namespace: kube-system
    port: 443
  group: metrics.k8s.io
  version: v1beta1
  groupPriorityMinimum: 100
  versionPriority: 100
  caBundle: <base64-encoded-ca>
---
# 本地APIService(内置API)
apiVersion: apiregistration.k8s.io/v1
kind: APIService
metadata:
  name: v1.
spec:
  group: ""
  version: v1
  groupPriorityMinimum: 18000
  versionPriority: 1
  # 无service字段表示由kube-apiserver本地处理
```

<!-- chunk: 内置聚合API -->
## 内置聚合API

| APIService | 服务 | 功能 |
|-----------|------|------|
| `v1beta1.metrics.k8s.io` | metrics-server | 资源指标 |
| `v1.custom.metrics.k8s.io` | prometheus-adapter | 自定义指标 |
| `v1beta1.external.metrics.k8s.io` | - | 外部指标 |

<!-- chunk: Extension API Server开发 -->
## Extension API Server开发

| 步骤 | 说明 |
|-----|------|
| 1. 实现API处理 | REST handler |
| 2. 配置TLS | 服务器证书 |
| 3. 部署服务 | Deployment+Service |
| 4. 创建APIService | 注册到聚合层 |
| 5. 配置RBAC | 授权访问 |

<!-- chunk: Extension Server示例结构 -->
## Extension Server示例结构

```go
// 使用apiserver-builder或自定义实现
package main

import (
    "k8s.io/apiserver/pkg/server"
    "k8s.io/apiserver/pkg/server/options"
)

func main() {
    // 配置TLS
    opts := options.NewSecureServingOptions()
    
    // 注册API处理器
    apiGroupInfo := server.NewDefaultAPIGroupInfo(...)
    
    // 启动服务器
    server.PrepareRun().Run(stopCh)
}
```

<!-- chunk: 认证代理配置 -->
## 认证代理配置

| 参数 | 说明 |
|-----|------|
| `--requestheader-client-ca-file` | 代理客户端CA |
| `--requestheader-allowed-names` | 允许的CN |
| `--requestheader-extra-headers-prefix` | 额外头前缀 |
| `--requestheader-group-headers` | 组头名称 |
| `--requestheader-username-headers` | 用户名头名称 |
| `--proxy-client-cert-file` | 代理客户端证书 |
| `--proxy-client-key-file` | 代理客户端密钥 |

<!-- chunk: 聚合层故障排查 -->
## 聚合层故障排查

| 问题 | 诊断命令 | 解决方案 |
|-----|---------|---------|
| APIService不可用 | `kubectl get apiservices` | 检查后端服务 |
| 证书问题 | `kubectl describe apiservice` | 更新caBundle |
| 网络不通 | `kubectl logs kube-apiserver` | 检查服务连通性 |
| 权限不足 | `kubectl auth can-i` | 配置RBAC |

<!-- chunk: 状态检查命令 -->
## 状态检查命令

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看所有APIService
kubectl get apiservices

# 查看聚合API状态
kubectl get apiservices v1beta1.metrics.k8s.io -o yaml

# 检查可用性
kubectl api-resources --api-group=metrics.k8s.io

# 测试API
kubectl get --raw /apis/metrics.k8s.io/v1beta1/nodes
```
<!-- chunk: 版本变更记录 -->
## 版本变更记录

| 版本 | 变更内容 |
|------|---------|
| v1.25 | APIService状态条件改进 |
| v1.27 | 聚合发现API改进 |
| v1.28 | API优先级和公平性增强 |
| v1.29 | 聚合层性能优化 |

<!-- chunk: APIService 高可用部署 -->
## APIService 高可用部署

### 生产级 Extension API Server 部署

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: custom-metrics-apiserver
  namespace: monitoring
  labels:
    app: custom-metrics-apiserver
spec:
  replicas: 2  # 生产至少 2 副本
  selector:
    matchLabels:
      app: custom-metrics-apiserver
  template:
    spec:
      affinity:
        podAntiAffinity:
          preferredDuringSchedulingIgnoredDuringExecution:
            - weight: 100
              podAffinityTerm:
                labelSelector:
                  matchLabels:
                    app: custom-metrics-apiserver
                topologyKey: kubernetes.io/hostname
      serviceAccountName: custom-metrics-apiserver
      containers:
        - name: apiserver
          image: custom-metrics-apiserver:v1.0.0
          args:
            - --secure-port=6443
            - --tls-cert-file=/certs/tls.crt
            - --tls-private-key-file=/certs/tls.key
            - --v=4
          ports:
            - containerPort: 6443
              name: https
          resources:
            requests:
              cpu: 100m
              memory: 128Mi
            limits:
              cpu: 500m
              memory: 256Mi
          livenessProbe:
            httpGet:
              path: /healthz
              port: 6443
              scheme: HTTPS
            initialDelaySeconds: 30
            periodSeconds: 10
          readinessProbe:
            httpGet:
              path: /readyz
              port: 6443
              scheme: HTTPS
            initialDelaySeconds: 5
            periodSeconds: 5
          volumeMounts:
            - name: certs
              mountPath: /certs
              readOnly: true
      volumes:
        - name: certs
          secret:
            secretName: custom-metrics-apiserver-certs
---
apiVersion: v1
kind: Service
metadata:
  name: custom-metrics-apiserver
  namespace: monitoring
spec:
  selector:
    app: custom-metrics-apiserver
  ports:
    - port: 443
      targetPort: 6443
      protocol: TCP
```

### 证书管理（cert-manager）

```yaml
apiVersion: cert-manager.io/v1
kind: Certificate
metadata:
  name: custom-metrics-apiserver-certs
  namespace: monitoring
spec:
  secretName: custom-metrics-apiserver-certs
  dnsNames:
    - custom-metrics-apiserver.monitoring.svc
    - custom-metrics-apiserver.monitoring.svc.cluster.local
  issuerRef:
    name: kube-ca-issuer
    kind: ClusterIssuer
  duration: 8760h    # 1 年
  renewBefore: 720h  # 提前 30 天续期
```

<!-- chunk: 聚合层安全加固 -->
## 聚合层安全加固

### 安全配置检查清单

| 检查项 | 配置 | 说明 |
|--------|------|------|
| TLS 强制 | `spec.caBundle` 必须配置 | 禁止使用 `insecureSkipTLSVerify` |
| 证书轮换 | cert-manager 自动续期 | 避免证书过期导致服务中断 |
| RBAC 最小权限 | 专用 ServiceAccount | 禁止使用 cluster-admin |
| 网络隔离 | NetworkPolicy 限制访问 | 仅允许 kube-apiserver 访问 |
| 审计日志 | 记录所有 API 请求 | 便于事后追溯 |

### RBAC 配置示例

```yaml
# Extension API Server 的 ServiceAccount 权限
apiVersion: rbac.authorization.k8s.io/v1
kind: ClusterRole
metadata:
  name: custom-metrics-apiserver
rules:
  # 允许访问自定义资源
  - apiGroups: ["custom.metrics.company.com"]
    resources: ["*"]
    verbs: ["get", "list", "watch"]
  # 允许访问核心资源（只读）
  - apiGroups: [""]
    resources: ["namespaces", "pods", "services"]
    verbs: ["get", "list", "watch"]
---
apiVersion: rbac.authorization.k8s.io/v1
kind: ClusterRoleBinding
metadata:
  name: custom-metrics-apiserver
roleRef:
  apiGroup: rbac.authorization.k8s.io
  kind: ClusterRole
  name: custom-metrics-apiserver
subjects:
  - kind: ServiceAccount
    name: custom-metrics-apiserver
    namespace: monitoring
```

### 用户访问授权

```yaml
# 允许用户访问聚合 API
apiVersion: rbac.authorization.k8s.io/v1
kind: ClusterRole
metadata:
  name: custom-metrics-reader
rules:
  - apiGroups: ["custom.metrics.company.com"]
    resources: ["*"]
    verbs: ["get", "list"]
---
apiVersion: rbac.authorization.k8s.io/v1
kind: ClusterRoleBinding
metadata:
  name: developer-custom-metrics
roleRef:
  apiGroup: rbac.authorization.k8s.io
  kind: ClusterRole
  name: custom-metrics-reader
subjects:
  - kind: Group
    name: developers
    apiGroup: rbac.authorization.k8s.io
```

<!-- chunk: 自定义指标 API 实战 -->
## 自定义指标 API 实战

### Prometheus Adapter 配置

```yaml
# 将 Prometheus 指标暴露为 K8s 自定义指标
apiVersion: v1
kind: ConfigMap
metadata:
  name: prometheus-adapter-config
  namespace: monitoring
data:
  config.yaml: |
    rules:
      # 将 http_requests_total 暴露为自定义指标
      - seriesQuery: 'http_requests_total{namespace!="",pod!=""}'
        resources:
          overrides:
            namespace: {resource: "namespace"}
            pod: {resource: "pod"}
        name:
          matches: "^(.*)$"
          as: "http_requests"
        metricsQuery: 'sum(rate(<<.Series>>{<<.LabelMatchers>>}[2m])) by (<<.GroupBy>>)'
      
      # 将队列长度暴露为外部指标（用于 KEDA）
      - seriesQuery: 'queue_length{queue!=""}'
        resources:
          overrides:
            namespace: {resource: "namespace"}
        name:
          as: "queue_length"
        metricsQuery: '<<.Series>>{<<.LabelMatchers>>}'
```

### 使用自定义指标进行 HPA

```yaml
apiVersion: autoscaling/v2
kind: HorizontalPodAutoscaler
metadata:
  name: web-app-hpa
spec:
  scaleTargetRef:
    apiVersion: apps/v1
    kind: Deployment
    name: web-app
  minReplicas: 2
  maxReplicas: 20
  metrics:
    # 基于自定义指标 http_requests
    - type: Pods
      pods:
        metric:
          name: http_requests
        target:
          type: AverageValue
          averageValue: "100"  # 每 Pod 100 QPS
    # 基于外部指标 queue_length
    - type: External
      external:
        metric:
          name: queue_length
          selector:
            matchLabels:
              queue: "processing"
        target:
          type: Value
          value: "50"  # 队列长度超过 50 时扩容
```

<!-- chunk: 聚合层监控告警 -->
## 聚合层监控告警

### PrometheusRule

```yaml
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: api-aggregation-alerts
  namespace: monitoring
spec:
  groups:
    - name: api-aggregation
      rules:
        - alert: APIServiceUnavailable
          expr: |
            apiservice_available == 0
          for: 5m
          labels:
            severity: critical
          annotations:
            summary: "APIService {{ $labels.name }} 不可用"
            runbook: "检查后端服务状态、证书有效性、网络连通性"

        - alert: APIServiceHighLatency
          expr: |
            histogram_quantile(0.99,
              sum(rate(apiserver_request_duration_seconds_bucket{verb!="WATCH"}[5m])) by (le, group, version)
            ) > 1
          for: 5m
          labels:
            severity: warning
          annotations:
            summary: "API {{ $labels.group }}/{{ $labels.version }} P99 延迟 > 1s"

        - alert: APIServiceHighErrorRate
          expr: |
            sum(rate(apiserver_request_total{code=~"5.."}[5m])) by (group, version) /
            sum(rate(apiserver_request_total[5m])) by (group, version) > 0.05
          for: 5m
          labels:
            severity: critical
          annotations:
            summary: "API {{ $labels.group }}/{{ $labels.version }} 错误率 > 5%"
```

### 关键监控指标

| 指标 | 说明 | 告警阈值 |
|------|------|----------|
| `apiservice_available` | APIService 可用性 | == 0 |
| `apiserver_request_duration_seconds` | 请求延迟 | P99 > 1s |
| `apiserver_request_total{code=~"5.."}` | 5xx 错误 | > 5% |
| `apiserver_current_inflight_requests` | 并发请求数 | > 80% 限制 |
| `aggregator_openapi_v2_regeneration_count` | OpenAPI 重新生成 | 异常增长 |

<!-- chunk: 生产部署检查清单 -->
## 生产部署检查清单

### 上线前检查

| 检查项 | 命令/方法 | 期望结果 |
|--------|----------|----------|
| APIService 状态 | `kubectl get apiservices` | Available=True |
| 后端 Pod 就绪 | `kubectl get pods -n <ns>` | Ready |
| 证书有效性 | `openssl x509 -in tls.crt -noout -dates` | 未过期 |
| RBAC 配置 | `kubectl auth can-i --as=system:serviceaccount:<ns>:<sa>` | 权限正确 |
| 网络连通性 | `kubectl exec -it <pod> -- curl -k https://<svc>` | 200 OK |
| 资源限制 | `kubectl get deploy -o yaml` | 已配置 |
| PDB 配置 | `kubectl get pdb -n <ns>` | 已配置 |
| 监控告警 | Prometheus/Grafana | 已配置 |

### 故障恢复流程

```
APIService 不可用
├── 1. 检查后端 Pod 状态
│   └── kubectl get pods -n <ns> -l app=<apiserver>
├── 2. 检查证书
│   └── kubectl get secret <certs> -o yaml | grep ca.crt | base64 -d | openssl x509 -noout -dates
├── 3. 检查网络
│   └── kubectl exec -it <pod> -- curl -k https://<service>.<ns>.svc:443/healthz
├── 4. 检查 RBAC
│   └── kubectl auth can-i --list --as=system:serviceaccount:<ns>:<sa>
└── 5. 查看 kube-apiserver 日志
    └── kubectl logs -n kube-system kube-apiserver-<node> | grep -i "aggregat"
```

---

**表格底部标记**: Kusheet Project, 作者 Allen Galler (allengaller@gmail.com)

---

<!-- chunk: Obsidian 相关文档 -->
## Obsidian 相关文档

- 平台工程 KUDIG Database — Global MOC
- [[10-平台工程/README.md|[[Platform Ops Domain (平台运维领域)|Platform Ops Domain (平台运维领域)]]]]
- index.md|Domain-9 平台运维 — 开源项目索引]]
- 平台运维概述
- 集群生命周期管理
- 容量规划与资源评估 (Capacity Planning & Resource Assessment)
- 性能基准测试与调优 (Performance Benchmarking & Tuning)
- 运维指标体系建设 (Operations Metrics System)
- 监控告警体系
- GitOps配置管理 (GitOps Configuration Management)
- 运维自动化工具链 (Operations Automation Toolchain)
- 成本优化与FinOps实践 (Cost Optimization & FinOps)

## See Also

- 19-lease-leader-election
- 20-crd-operator-development
- 22-client-libraries
- 23-cli-enhancement-tools


<!-- risk-assessed -->
