---
title: Kubernetes FinOps 成本治理 Runbook
description: 覆盖成本分配标签、Kubecost/OpenCost 部署、Showback/Chargeback、Right-sizing 工作流、Spot/可中断实例使用、异常检测的生产级成本治理手册
summary: 覆盖成本分配标签、Kubecost/OpenCost 部署、Showback/Chargeback、Right-sizing 工作流、Spot/可中断实例使用、异常检测的生产级成本治理手册
category: production-operations
tags:
- production
- best-practices
- playbook
- finops
- cost-governance
- kubecost
- opencost
- showback
- chargeback
- right-sizing
- spot
- anomaly-detection
tier: core
created: '2026-07-01'
last_updated: '2026-07'
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 平台工程师
estimated_read_time: 25min
intent_queries:
- Kubernetes FinOps 成本治理 Runbook 是什么
- Kubecost OpenCost 怎么部署
- 成本分配标签怎么设计
- Showback Chargeback 怎么做
- Right-sizing 工作流
- Spot 实例怎么用
trigger_keywords:
- finops
- cost governance
- kubecost
- opencost
- showback
- chargeback
- right-sizing
- spot
- anomaly detection
- cost allocation
prerequisites:
- kubectl-basics
- prometheus-basics
- helm-basics
- cloud-billing-basics
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
- '1.33'
authors:
- name: KUDIG Team
  role: contributor
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。


# Kubernetes FinOps 成本治理 Runbook

> **适用范围**: Kubernetes v1.28–v1.33 | **最后更新**: 2026-07 | **文档类型**: 生产运维 Runbook

本 Runbook 面向 SRE、平台工程师与 FinOps 负责人，系统阐述 Kubernetes 生产环境的成本治理体系。内容覆盖成本分配标签设计、Kubecost/OpenCost 部署与使用、Showback/Chargeback 机制、Right-sizing 闭环工作流、Spot/可中断实例策略，以及成本异常检测。成本治理不是一次性优化，而是“观测—分摊—优化—约束”的持续闭环。

---

## 1. 适用场景与范围

- **成本可见性**：按命名空间、部门、项目、环境拆解集群总成本。
- **成本分摊**：将节点、存储、网络、负载均衡费用公平分摊到业务团队。
- **资源优化**：通过 Request/Limit 分析与 VPA 推荐，持续 Right-size 工作负载。
- **算力成本优化**：引入 Spot/Preemptible/可中断实例，降低非关键负载成本。
- **异常检测**：识别费用突增、资源浪费、闲置 PV/LoadBalancer。

---

## 2. 前置条件与工具

### 2.1 标签与命名规范

所有生产资源必须携带以下标签：

| 标签键 | 示例值 | 用途 |
|--------|--------|------|
| `cost-center` | `cc-12345` | 财务成本中心 |
| `team` | `platform` | 负责团队 |
| `project` | `payment-gateway` | 项目或产品 |
| `environment` | `production` | 环境 |
| `app` | `api-server` | 应用名称 |
| `tier` | `critical` / `batch` | 服务等级 |

节点标签通过 kubelet `--node-labels` 或启动模板注入；工作负载标签通过 CI/CD 或 OPA/Gatekeeper 强制校验。

### 2.2 必备工具

| 工具 | 用途 | 推荐版本 |
|------|------|----------|
| OpenCost | CNCF 开源成本分摊 | v1.114+ |
| Kubecost | 企业级成本治理平台 | v2.7+ |
| Prometheus | 资源用量指标 | v2.55+ |
| VPA | Vertical Pod Autoscaler | 0.14+ |
| Karpenter / Cluster Autoscaler | 节点弹性与 Spot 整合 | 0.37+ / 1.31+ |
| OPA/Kyverno | 标签策略与资源约束 | 最新 |

---

## 3. 标准操作流程

### 3.1 OpenCost 部署

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
helm install opencost opencost/opencost \
  --namespace opencost --create-namespace \
  --set opencost.exporter.defaultClusterId=<cluster-id> \
  --set opencost.prometheus.internal.namespaceName=monitoring \
  --set opencost.prometheus.internal.serviceName=prometheus \
  --set opencost.ui.enabled=true
```
访问 UI：

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
kubectl port-forward -n opencost service/opencost 9003:9090
```
### 3.2 云厂商账单集成（Kubecost）

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
helm install kubecost cost-analyzer \
  --repo https://kubecost.github.io/cost-analyzer/ \
  --namespace kubecost --create-namespace \
  --set kubecostToken="<token>" \
  --set prometheus.server.retention=15d \
  --set kubecostProductConfigs.cloudIntegrationJSON=<cloud-integration-secret>
```
集成后可在 UI 中查看实际账单与分摊成本对比。

### 3.3 Showback / Chargeback 机制

| 模式 | 说明 | 适用阶段 |
|------|------|----------|
| **Showback** | 向团队展示分摊成本，不实际收费 | 成本意识培养初期 |
| **Chargeback** | 按实际用量向团队内部结算 | 成本成熟、预算管理严格 |
| **Hybrid** | Showback 占 80%，Chargeback 对超标部分收费 | 平衡可见性与激励 |

月度成本报告应包含：

- 按 `team/project/environment` 聚合的 CPU/内存/GPU/存储/网络成本。
- 与上月及预算的对比。
- Top 10 成本增长工作负载。
- 闲置资源清单。

### 3.4 Right-sizing 工作流

#### 数据采集

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看 VPA 推荐
kubectl get vpa -A -o jsonpath='{range .items[*]}{.metadata.namespace}{"/"}{.metadata.name}{"\t"}{.status.recommendation.containerRecommendations[0].target}{"\n"}{end}'

# Kubecost Request Sizing
# 访问 /request-sizing 页面，按 namespace 导出 CSV
```
#### 决策矩阵

| 当前 Request | VPA 推荐 | 置信度 | 建议动作 |
|--------------|----------|--------|----------|
| CPU 1000m | 250m | 高 | 下调至 300m |
| 内存 4Gi | 6Gi | 中 | 上调并观察 |
| CPU 500m | 450m | 低 | 暂不调整，等待更多样本 |

#### 变更流程

1. 从 Kubecost/OpenCost 导出推荐。
2. 在 Git 中修改 Helm values 或 Kustomize overlay。
3. 在非生产环境验证 1 周。
4. 通过 Argo CD 灰度同步到生产，观察 SLO。
5. 成功后关闭优化项工单。

### 3.5 Spot / 可中断实例策略

#### 工作负载分级

| 等级 | 是否适合 Spot | 示例 |
|------|---------------|------|
| Tier 0 关键 | 否 | 支付、认证、实时推理 |
| Tier 1 重要 | 谨慎 | 订单处理，需 on-demand fallback |
| Tier 2 普通 | 是 | 批处理、CI/CD、数据清洗 |
| Tier 3 可延迟 | 是 | 报表、归档、测试 |

#### Karpenter NodePool 示例

```yaml
apiVersion: karpenter.sh/v1beta1
kind: NodePool
metadata:
  name: spot-batch
spec:
  template:
    spec:
      requirements:
      - key: karpenter.sh/capacity-type
        operator: In
        values: ["spot"]
      - key: node.kubernetes.io/instance-type
        operator: In
        values: ["c6i.xlarge", "c6i.2xlarge"]
  disruption:
    consolidationPolicy: WhenUnderutilized
    expireAfter: 720h
  limits:
    cpu: 1000
    memory: 4000Gi
```

工作负载需配置容忍与 PDB：

```yaml
tolerations:
- key: karpenter.sh/capacity-type
  operator: Equal
  value: spot
  effect: NoSchedule
```

### 3.6 成本异常检测

#### PrometheusRule 示例

```yaml
groups:
- name: cost-anomaly
  rules:
  - alert: DailyCostSpike
    expr: |
      (
        sum by (team, namespace) (opencost_container_cpu_allocation * on() opencost_cpu_cost_per_vcpu)
        +
        sum by (team, namespace) (opencost_container_memory_allocation_bytes / 1024 / 1024 / 1024 * on() opencost_memory_cost_per_gigabyte)
      )
      > 1.5 * avg_over_time(
        same_expression[7d]
      )
    for: 1h
    labels:
      severity: warning
    annotations:
      summary: "{{ $labels.team }}/{{ $labels.namespace }} 日成本较 7 日均值上涨 50%"
```

#### 常见异常类型

- 闲置 LoadBalancer Service。
- 未绑定的 PVC。
- Request 远高于实际使用。
- 测试环境未按时间表缩容。
- GPU 资源长期空闲。

---

## 4. 关键检查点与验证命令

| 检查项 | 命令 | 合格标准 |
|--------|------|----------|
| 标签合规 | `kubectl get pods -A --show-labels` | 所有 Pod 含 cost-center/team/project |
| OpenCost 运行 | `kubectl get pods -n opencost` | Running/Ready |
| 成本分摊数据 | `curl http://opencost.opencost.svc:9003/allocation` | 返回 JSON 数据 |
| VPA 推荐 | `kubectl get vpa -A` | 无空 recommendation |
| Spot 节点 | `kubectl get nodes -L karpenter.sh/capacity-type` | 标签正确 |
| 闲置 PVC | `kubectl get pvc -A | grep Bound` | 业务 Owner 确认使用状态 |

---

## 5. 回滚/应急方案

- **Right-sizing 导致性能下降**：立即回滚 Request/Limit 调整。
  ```bash
  kubectl rollout undo deployment/<app> -n <ns>
  ```
- **Spot 实例大规模回收**：启用 on-demand fallback NodePool，调整 workload 亲和性。
- **成本异常无法解释**：导出 OpenCost allocation 数据，按 team/namespace 下钻，定位异常工作负载。
- **标签缺失导致分摊失败**：通过 Kyverno Policy 阻断无标签资源创建。
  ```yaml
  validationFailureAction: Enforce
  ```

---

## 6. 风险与注意事项

1. **成本数据有延迟**：OpenCost/Kubecost 基于 Prometheus 指标，通常延迟 1–5 分钟，不适合实时计费。
2. **Request 不等于实际使用**：按 Request 分摊会激励团队虚报，建议结合实际用量与账单权重。
3. **Spot 实例不适用于所有负载**：关键服务使用 Spot 可能导致 SLA 违约，需明确分级。
4. **存储与网络成本难分摊**：PVC 实际用量需 CSI 支持监控，跨区流量需云厂商账单明细。
5. **FinOps 是组织变革**：仅靠工具无法降本，需要与业务团队建立预算、考核与反馈机制。

---

## 7. 相关 Runbook / 推荐阅读

- [[domain-11-production-operations/99-production-readiness-operations-guide.md|生产运维 生产就绪运维指南]]
- [[domain-07-platform-engineering/99-production-readiness-operations-guide.md|平台工程 生产就绪运维指南]]
- [[domain-11-production-operations/成本治理/99-finops-cost-optimization-guide.md|K8s FinOps 成本优化实践指南]]
- [[domain-11-production-operations/成本治理/01-cost-allocation-chargeback.md|成本分摊与 Chargeback]]
- [[domain-11-production-operations/成本治理/02-idle-resource-right-sizing.md|闲置资源与 Right-sizing]]
- [[domain-11-production-operations/成本治理/03-spot-instance-strategy.md|Spot 实例策略]]
- [[domain-14-ai-ml-infra/基础设施/27-cost-management-kubecost.md|AI 场景 Kubecost 成本管理]]


<!-- risk-assessed -->
