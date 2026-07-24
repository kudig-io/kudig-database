---
title: metrics-server 深度解析 (metrics-server Deep Dive)
description: 深入解析 Kubernetes metrics-server 的架构、部署、HPA/VPA 依赖关系、与自定义指标适配器的对比，以及生产级故障排查。
summary: 深入解析 Kubernetes metrics-server 的架构、部署、HPA/VPA 依赖关系、与自定义指标适配器的对比，以及生产级故障排查。
category: 集群基础
tags:
- k8s
- control-plane
- metrics-server
- monitoring
- metrics
- hpa
- vpa
- apiservice
- aggregator
- prometheus
- kubelet
tier: peripheral
created: '2026-07-23'
last_updated: 2026-07
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 平台工程师
- 运维工程师
estimated_read_time: 15min
intent_queries:
- metrics-server 深度解析 是什么
- 如何 metrics-server 深度解析
- Kubernetes metrics-server 生产部署与排障
trigger_keywords:
- metrics-server
- 深度解析
- HPA
- VPA
prerequisites:
- kubectl-basics
- kubernetes-concepts
- prometheus-basics
- autoscaling-basics
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
  path: ../集群基础/
  label: '相关知识域: 集群基础'
- type: domain
  path: ../可观测性/
  label: '相关知识域: 可观测性'
- type: domain
  path: ../工作负载/
  label: '相关知识域: 工作负载'
- type: cheatsheet
  path: ../系统基础/topic-cheat-sheet/kubectl-scene-cheatsheet.md
  label: '速查卡: kubectl-scene-cheatsheet'
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# metrics-server 深度解析 (metrics-server Deep Dive)

> metrics-server 是 Kubernetes 集群核心指标聚合器，为 `kubectl top`、HPA、VPA 提供节点与 Pod 级资源使用数据。

---

<!-- chunk: 1. 架构概述 -->
## 1. 架构概述

### 1.1 核心职责

| 职责 | 说明 |
|------|------|
| **资源指标聚合** | 通过 kubelet Summary API 收集节点/Pod CPU、内存指标 |
| **API 暴露** | 通过 Kubernetes aggregator 向 API Server 注册 `metrics.k8s.io` APIService |
| **HPA/VPA 数据源** | 为 Horizontal Pod Autoscaler 和 Vertical Pod Autoscaler 提供实时资源指标 |
| **kubectl top 数据源** | 支持 `kubectl top node` / `kubectl top pod` 命令 |

### 1.2 数据流

```
API Server ← Aggregator
                ↑
         metrics-server Pod
                ↑
         kubelet Summary API (10250)
                ↑
         cAdvisor / runtime cgroup stats
```

> metrics-server **不是**长期监控系统，默认只保留最近几分钟的指标。如需长期存储与告警，请使用 Prometheus。

---

<!-- chunk: 2. 部署与升级 -->
## 2. 部署与升级

### 2.1 官方 YAML 部署

```bash
# 🟡 中风险：会修改 kube-system 命名空间
kubectl apply -f https://github.com/kubernetes-sigs/metrics-server/releases/latest/download/components.yaml
```

### 2.2 关键启动参数

| 参数 | 作用 | 生产建议 |
|------|------|----------|
| `--kubelet-preferred-address-types` | 连接 kubelet 使用的地址类型 | `InternalIP,ExternalIP,Hostname` |
| `--kubelet-insecure-tls` | 跳过 kubelet 证书校验 | 仅测试使用；生产请使用有效 CA |
| `--requestheader-client-ca-file` | 校验 aggregator 请求 | 必须配置以启用 API 聚合 |
| `--metric-resolution` | 指标采集间隔 | `15s`（默认） |
| `--kubelet-use-node-status-port` | 使用节点 status 中的 port | 适用于 EKS/ACK 等托管集群 |
| `--enable-prometheus-endpoint` | 暴露 Prometheus 指标 | 建议开启 |

### 2.3 生产配置示例

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: metrics-server
  namespace: kube-system
spec:
  template:
    spec:
      containers:
      - name: metrics-server
        image: registry.k8s.io/metrics-server/metrics-server:v0.7.1
        args:
        - --cert-dir=/tmp
        - --secure-port=10250
        - --kubelet-preferred-address-types=InternalIP,ExternalIP,Hostname
        - --kubelet-use-node-status-port
        - --metric-resolution=15s
        - --enable-prometheus-endpoint
        resources:
          requests:
            cpu: 100m
            memory: 200Mi
          limits:
            cpu: 500m
            memory: 512Mi
```

---

<!-- chunk: 3. HPA/VPA 依赖关系 -->
## 3. HPA/VPA 依赖关系

### 3.1 metrics-server 与 HPA

```yaml
apiVersion: autoscaling/v2
kind: HorizontalPodAutoscaler
spec:
  metrics:
  - type: Resource
    resource:
      name: cpu
      target:
        type: Utilization
        averageUtilization: 50
```

- HPA 通过 `metrics.k8s.io` 获取 Pod CPU/内存使用率。
- 如果 metrics-server 不可用，HPA 无法获取指标，会保留上次状态或显示 `failed to get cpu utilization`。

### 3.2 metrics-server 与 VPA

- VPA Recommender 使用 metrics-server 的历史指标计算资源推荐。
- 建议同时部署 `kube-state-metrics` 补充对象状态指标。

---

<!-- chunk: 4. metrics-server vs 自定义指标适配器 -->
## 4. metrics-server vs 自定义指标适配器

| 能力 | metrics-server | Prometheus Adapter | KEDA |
|------|----------------|-------------------|------|
| 资源指标（CPU/内存） | ✅ 内置 | ✅ 支持 | ✅ 支持 |
| 自定义应用指标 | ❌ 不支持 | ✅ 支持 | ✅ 支持 |
| 外部指标（如云监控） | ❌ 不支持 | ❌ 不支持 | ✅ 支持 |
| 事件驱动伸缩 | ❌ 不支持 | ❌ 不支持 | ✅ 支持 |
| 长期存储 | ❌ 不保存 | ❌ 不保存 | ❌ 不保存 |
| 典型使用场景 | 基础 HPA/VPA | 自定义指标 HPA | 事件源触发伸缩 |

---

<!-- chunk: 5. 故障排查 -->
## 5. 故障排查

### 5.1 症状速查

| 症状 | 可能原因 | 诊断命令 | 修复方法 |
|------|----------|----------|----------|
| `kubectl top node` 无数据 | metrics-server 未运行 / APIService 异常 | `kubectl get apiservice v1beta1.metrics.k8s.io` | 重启 metrics-server / 修复证书 |
| `kubectl top pod` 报错 | kubelet 指标端点不可达 | `kubectl logs -n kube-system deploy/metrics-server` | 配置 `--kubelet-preferred-address-types` |
| HPA 显示 `unknown` | metrics-server 未提供指标 | `kubectl describe hpa <name>` | 检查 metrics-server 与 HPA 指标类型匹配 |
| APIService 状态 `MissingEndpoints` | metrics-server Service 无后端 | `kubectl get endpoints -n kube-system metrics-server` | 检查 Pod 状态与 selector |
| `x509: certificate signed by unknown authority` | kubelet 证书不被 metrics-server 信任 | `kubectl logs -n kube-system deploy/metrics-server` | 使用集群 CA 签名 kubelet 证书，或配置 `--kubelet-certificate-authority` |
| metrics-server OOM | 节点/Pod 数量过大 | `kubectl top pod -n kube-system` | 增加内存 limit / 开启分片 |

### 5.2 排查流程

```bash
# 1. 检查 APIService 状态
kubectl get apiservice v1beta1.metrics.k8s.io -o yaml

# 2. 检查 metrics-server Pod
kubectl get pods -n kube-system -l k8s-app=metrics-server

# 3. 查看 metrics-server 日志
kubectl logs -n kube-system -l k8s-app=metrics-server --tail=100

# 4. 验证 kubelet 指标端点
kubectl get --raw /api/v1/nodes/<node>/proxy/stats/summary

# 5. 验证 metrics API 是否可用
kubectl get --raw /apis/metrics.k8s.io/v1beta1/nodes
kubectl get --raw /apis/metrics.k8s.io/v1beta1/pods

# 6. 检查 Service/Endpoints
kubectl get svc -n kube-system metrics-server
kubectl get endpoints -n kube-system metrics-server
```

### 5.3 常见证书问题

```bash
# 检查 kubelet 服务端证书是否由集群 CA 签名
openssl x509 -in /var/lib/kubelet/pki/kubelet-server-current.pem -noout -issuer

# 临时绕过（仅测试环境）
# 在 metrics-server args 中添加 --kubelet-insecure-tls
```

---

<!-- chunk: 6. 生产最佳实践 -->
## 6. 生产最佳实践

1. **高可用部署**：使用 Deployment + PodDisruptionBudget，至少 2 副本跨可用区部署。
2. **资源限制**：根据集群节点数调整 CPU/内存，大规模集群建议 1CPU/1Gi+。
3. **证书安全**：生产环境务必使用集群 CA 签名的 kubelet 服务端证书，禁用 `--kubelet-insecure-tls`。
4. **监控告警**：
   - metrics-server 自身可用性
   - `metrics_server_storage_points` 积压
   - APIService `v1beta1.metrics.k8s.io` 状态
5. **与 Prometheus 互补**：metrics-server 用于调度伸缩，Prometheus 用于长期监控与告警。

---

<!-- chunk: 7. 检查清单 -->
## 7. 检查清单

- [ ] metrics-server Pod 正常运行且多副本可用
- [ ] `v1beta1.metrics.k8s.io` APIService 状态为 True
- [ ] `kubectl top node` / `kubectl top pod` 能返回数据
- [ ] kubelet 服务端证书由集群 CA 签名
- [ ] metrics-server 资源限制符合集群规模
- [ ] HPA/VPA 能获取资源指标
- [ ] metrics-server 日志与指标已接入可观测性平台

---

## Related

- [[实体/kube-apiserver.md|kube-apiserver]] — kube-apiserver
- [[实体/kubelet.md|kubelet]] — kubelet
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[可观测性/指标/19-kube-state-metrics-deep-dive.md|kube-state-metrics 深度解析]]
- [[平台工程/代码分析/cluster-create/23-scheduler.md|kube-scheduler 代码分析]]


<!-- risk-assessed -->
