---
title: FinOps 成本优化与云费用故障排查指南 [topic-structural-trouble-shooting]
description: 'title: FinOps 成本优化与云费用故障排查指南'
summary: 'title: FinOps 成本优化与云费用故障排查指南'
category: structural-troubleshooting
tags:
- troubleshooting
- guide
- daily-ops
- cost-optimization
- prometheus
- helm
- docker
- vpa
- job
- cronjob
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 技术支持
estimated_read_time: 25min
intent_queries:
- FinOps 成本优化与云费用故障排查指南 是什么
- 如何 FinOps 成本优化与云费用故障排查指南
- Kubernetes 10 troubleshooting diagnostics 最佳实践
- FinOps 成本优化与云费用故障排查指南 故障排查
- FinOps 成本优化与云费用故障排查指南 排障步骤
trigger_keywords:
- FinOps
- 成本优化与云费用故障排查指南
- troubleshooting
- diagnostics
- structural
- trouble
- shooting
prerequisites:
- kubectl-basics
- troubleshooting-methodology
- helm-basics
- prometheus-basics
- ebpf-basics
- gpu-scheduling-basics
- observability-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




title: FinOps 成本优化与云费用故障排查指南
description: '# FinOps 成本优化与云费用故障排查指南'
category: structural-troubleshooting
tags:
- k8s
- troubleshooting
- decision-tree
- [[Prometheus|prometheus]]
- [[Helm|helm]]
- vpa
- job
- [[CronJob|cronjob]]
- ingress
- gateway
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 技术支持
estimated_read_time: 5min
intent_queries:
- FinOps 成本优化与云费用故障排查指南 是什么
- 如何 FinOps 成本优化与云费用故障排查指南
- FinOps 成本优化与云费用故障排查指南 故障排查
- FinOps 成本优化与云费用故障排查指南 排障步骤
trigger_keywords:
- FinOps
- 成本优化与云费用故障排查指南
- structural
- trouble
- shooting
authors:
- name: KUDIG Team
  role: contributor
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
---

# FinOps 成本优化与云费用故障排查指南

> **适用版本**: Kubernetes v1.25 - v1.32 | Kubecost/OpenCost v1.108+ | **最后更新**: 2026-04 | **难度**: 中级

---

## 0. 10 分钟快速诊断

1. **成本总览**：`kubectl port-forward -n kubecost deployment/kubecost-cost-analyzer 9090`，访问 `/overview.html` 查看成本趋势。
2. **异常飙升检测**：对比本周与上周的 namespace/Deployment 成本，定位异常增长来源。
3. **闲置资源扫描**：查看 Kubecost 的 "Savings" 页面，识别未挂载的 PV、低利用率节点、过度配置的 Pod。
4. **Spot/Preemptible 利用率**：检查 Spot 实例占总计算成本的比例，评估优化空间。
5. **计费对齐**：将 Kubecost 的估算与云厂商账单对比，偏差 >20% 时需检查折扣、预留实例、分摊规则。
6. **快速缓解**：
   - 闲置资源：通过 Kubecost API 获取闲置资源列表并清理。
   - 过度配置：使用 VPA 推荐值调整 Pod requests/limits。
   - 异常飙升：找到导致成本激增的 Pod/Job 并限制资源。
7. **证据留存**：保存成本趋势截图、闲置资源报告、优化前后的资源配置对比。

---

## 1. 问题现象与影响分析

### 1.1 常见问题现象

#### 1.1.1 成本异常飙升

| 现象 | 报错信息 | 报错来源 | 查看方式 |
|------|----------|----------|----------|
| 单日成本翻倍 | `cost increased by 100%+` | Kubecost/OpenCost | Web UI / API |
|  namespace 成本异常 | `namespace cost spike detected` | Kubecost Alerts | 告警通知 |
|  GPU 成本激增 | `gpu cost unexpectedly high` | 云厂商账单 | 云控制台 |
|  存储费用暴涨 | `storage cost increased` | 云厂商账单 | 云控制台 |
|  网络出口费用异常 | `egress cost spike` | 云厂商账单 | 云控制台 |

#### 1.1.2 成本估算不准

| 现象 | 报错信息 | 报错来源 | 查看方式 |
|------|----------|----------|----------|
| Kubecost 与账单偏差大 | `cost mismatch > 30%` | 财务对账 | 对比报告 |
| 预留实例未计入折扣 | `RI/SP discount not applied` | Kubecost | 配置检查 |
| 分摊规则错误 | `shared cost allocation incorrect` | Kubecost | 配置检查 |
| 自定义定价未生效 | `custom pricing not reflected` | Kubecost | API/ConfigMap |
| Spot 价格估算错误 | `spot price estimation inaccurate` | OpenCost | 日志检查 |

#### 1.1.3 闲置与过度配置

| 现象 | 报错信息 | 报错来源 | 查看方式 |
|------|----------|----------|----------|
| CPU 利用率长期 <10% | `low cpu utilization` | Kubecost Savings | Web UI |
| 内存利用率长期 <20% | `low memory utilization` | Kubecost Savings | Web UI |
| 未挂载 PV 持续扣费 | `unmounted pv detected` | Kubecost Savings | Web UI |
| 节点空闲但未缩容 | `node not scaled down` | Cluster Autoscaler | CA 日志 |
| 重复部署导致资源浪费 | `duplicate deployments` | Kubecost | Web UI |

#### 1.1.4 生产环境典型场景

| 场景 | 典型现象 | 根本原因 | 解决方向 |
|------|----------|----------|----------|
| **CI/CD 构建节点持续运行** | 非工作时间节点仍运行，CPU 利用率接近 0 | Cluster Autoscaler 未配置或缩容阻塞 | 配置 CA + 节点亲和性 + 定时缩容 |
| **开发环境 GPU 实例未关闭** | 开发 namespace 的 GPU Pod 下班后仍在运行 | 缺少资源生命周期管理 | 配置 TTL + 定时任务清理 |
| **日志存储费用月增 50%** | 日志量激增但 retention 未调整 | 应用日志级别错误或审计日志全开 | 调整日志级别 + 分层存储 |
| **多租户成本归属不清** | 同一 namespace 多团队共享，无法拆分成本 | 缺少 labels/annotations 分摊规则 | 配置 team/project 标签 + Kubecost 分摊 |

### 1.2 报错查看方式汇总

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# Kubecost/OpenCost API 查询
curl -s http://kubecost-cost-analyzer.kubecost.svc.cluster.local:9090/model/allocation \
  -d 'window=7d' -d 'aggregate=namespace' -d 'accumulate=true'

# 闲置资源 API
curl -s http://kubecost-cost-analyzer.kubecost.svc.cluster.local:9090/model/savings/requestSizing

# 未挂载 PV
curl -s http://kubecost-cost-analyzer.kubecost.svc.cluster.local:9090/model/savings/unmountedVolumes

# 节点利用率
curl -s http://kubecost-cost-analyzer.kubecost.svc.cluster.local:9090/model/assets \
  -d 'window=7d' -d 'aggregate=type'

# OpenCost 基础查询
curl -s http://opencost.opencost.svc.cluster.local:9003/allocation/compute \
  -d 'window=7d' -d 'resolution=1d'

# 查看成本相关 Prometheus 指标
kubectl exec -it prometheus-pod -- wget -qO- localhost:9090/api/v1/query \
  --post-data 'query=node_cpu_utilization' 2>/dev/null
```
---

## 2. 排查方法与步骤

### 2.1 诊断原理说明

云原生成本管理架构：

```
# 🟢 低风险：只读/信息收集，通常无副作用
┌─────────────────────────────────────────────────────────────────┐
│                    云厂商计费层 (Cloud Billing)                   │
│  AWS Cost Explorer / Azure Cost Management / GCP Billing        │
├─────────────────────────────────────────────────────────────────┤
│                    成本汇聚层 (Cost Aggregation)                  │
│  Kubecost / OpenCost / CloudHealth / Vantage                    │
├─────────────────────────────────────────────────────────────────┤
│                    指标采集层 (Metrics Collection)                │
│  Prometheus (kube-state-metrics, cAdvisor, node-exporter)       │
├─────────────────────────────────────────────────────────────────┤
│                    Kubernetes 资源层                              │
│  Nodes | Pods | PVCs | Services | LoadBalancers | Ingress       │
└─────────────────────────────────────────────────────────────────┘
```
**关键概念**：
- **成本分摊 (Cost Allocation)**：将节点成本按 CPU/内存/GPU requests 分摊到各个 Pod/namespace
- **闲置成本 (Idle Cost)**：节点已分配但未使用的资源对应的成本
- **分摊成本 (Shared Cost)**：系统组件、监控、日志等公共资源的成本分摊方式
- **折扣归集**：预留实例 (RI)、Saving Plans、Spot 折扣在成本展示中的处理方式

### 2.2 排查逻辑决策树

```
云成本异常
    ├── 成本估算不准
    │   ├── Kubecost 与云账单偏差大？
    │   │   ├── 自定义定价未配置？──► 配置 cloud provider pricing
    │   │   ├── RI/SP 折扣未计入？──► 启用 discounts 配置
    │   │   └── 分摊规则不合理？──► 调整 sharedNamespaces/labels
    │   └── OpenCost 数据缺失？
    │       ├── Prometheus 指标缺失？──► 检查 kube-state-metrics
    │       └── Node 定价信息缺失？──► 配置自定义 pricing
    ├── 成本异常飙升
    │   ├── 计算资源激增？
    │   │   ├── 新部署了高配置 Pod？──► 检查最近的 Deployment/Job
    │   │   ├── 节点未缩容？──► 检查 CA 和节点利用率
    │   │   └── 使用了 On-Demand 而非 Spot？──► 迁移到 Spot
    │   ├── 存储费用激增？
    │   │   ├── 新创建了大量 PVC？──► 检查 PVC 增长趋势
    │   │   ├── 快照/备份保留期过长？──► 调整 retention
    │   │   └── 使用了高性能存储？──► 评估是否需要 SSD
    │   └── 网络费用激增？
    │       ├── 跨区/跨地域流量增加？──► 检查 Service topology
    │       └── 出口流量激增？──► 检查 NAT Gateway/LoadBalancer
    └── 资源闲置浪费
        ├── CPU 过度配置？
        │   └── requests >> 实际使用？──► 使用 VPA 调整
        ├── 内存过度配置？
        │   └── requests >> 实际使用？──► 使用 VPA 调整
        ├── 闲置节点？
        │   └── 节点利用率 <20%？──► 启用 CA 或手动缩容
        └── 未使用的 PV/LB？
            └── PVC 未绑定或 LB 无后端？──► 清理闲置资源
```

### 2.3 详细诊断命令

#### Kubecost 成本诊断

```bash
#!/bin/bash
# Kubecost 成本诊断脚本

KUBECOST_URL="http://kubecost-cost-analyzer.kubecost.svc.cluster.local:9090"

echo "=== Kubecost 成本诊断 ==="

# 1. 总成本概览
echo "1. 过去 7 天总成本概览:"
curl -s "$KUBECOST_URL/model/allocation" \
  -d 'window=7d' \
  -d 'aggregate=namespace' \
  -d 'accumulate=true' \
  -d 'shareIdle=true' \
  -d 'shareTenancyCosts=true' | jq -r '
  .data[]? | to_entries[] | "  \(.key): totalCost=\(.value.totalCost // 0 | tostring[0:6])"
' 2>/dev/null | sort -k2 -rn | head -15

# 2. 闲置资源
echo ""
echo "2. 闲置资源建议:"
curl -s "$KUBECOST_URL/model/savings" | jq -r '
  .data? | to_entries[] | select(.value.savings > 0) |
  "  \(.key): savings=\(.value.savings // 0 | tostring[0:6])"
' 2>/dev/null | sort -k2 -rn | head -10

# 3. 过度配置的 Deployment
echo ""
echo "3. 过度配置的 Deployment (CPU request > 2x usage):"
curl -s "$KUBECOST_URL/model/savings/requestSizing" \
  -d 'window=7d' | jq -r '
  .data[]? | select(.savingsCPU > 0 or .savingsRAM > 0) |
  "  \(.namespace)/\(.controllerName): cpuSavings=\(.savingsCPU // 0 | tostring[0:6]), ramSavings=\(.savingsRAM // 0 | tostring[0:6])"
' 2>/dev/null | head -10

# 4. 未挂载的卷
echo ""
echo "4. 未挂载的卷:"
curl -s "$KUBECOST_URL/model/savings/unmountedVolumes" | jq -r '
  .data[]? | "  \(.namespace)/\(.volumeName): cost=\(.savings // 0 | tostring[0:6])"
' 2>/dev/null | head -10
```

#### 成本异常排查脚本

```bash
#!/bin/bash
# 成本异常排查脚本：对比本期与上期成本

NAMESPACE=${1:-""}
WINDOW_DAYS=${2:-7}

echo "=== 成本异常排查 (窗口: ${WINDOW_DAYS}天) ==="

KUBECOST_URL="http://kubecost-cost-analyzer.kubecost.svc.cluster.local:9090"

# 获取本期成本
echo "1. 本期成本 ($(date -d "-${WINDOW_DAYS} days" +%Y-%m-%d) 至 $(date +%Y-%m-%d)):"
CURRENT=$(curl -s "$KUBECOST_URL/model/allocation" \
  -d "window=${WINDOW_DAYS}d" \
  -d 'aggregate=namespace' \
  -d 'accumulate=true' | jq -r '.data[]? | to_entries[] | "\(.key):\(.value.totalCost)"')

# 获取上期成本
echo ""
echo "2. 上期成本 ($(date -d "-$((WINDOW_DAYS*2)) days" +%Y-%m-%d) 至 $(date -d "-${WINDOW_DAYS} days" +%Y-%m-%d)):"
PREVIOUS=$(curl -s "$KUBECOST_URL/model/allocation" \
  -d "window=$((WINDOW_DAYS*2))d,${WINDOW_DAYS}d" \
  -d 'aggregate=namespace' \
  -d 'accumulate=true' | jq -r '.data[]? | to_entries[] | "\(.key):\(.value.totalCost)"')

# 对比差异
echo ""
echo "3. 成本变化 (本期 vs 上期):"
echo "$CURRENT" | while IFS=: read ns cost; do
  prev_cost=$(echo "$PREVIOUS" | grep "^$ns:" | cut -d: -f2)
  prev_cost=${prev_cost:-0}
  if (( $(echo "$cost > $prev_cost * 1.5" | bc -l) )); then
    echo "  ⚠ $ns: 上期=$prev_cost, 本期=$cost (增长 >50%)"
  fi
done

# 4. 按 Deployment 细分最高成本的 namespace
if [ -n "$NAMESPACE" ]; then
  echo ""
  echo "4. Namespace $NAMESPACE 的 Deployment 成本:"
  curl -s "$KUBECOST_URL/model/allocation" \
    -d "window=${WINDOW_DAYS}d" \
    -d 'aggregate=deployment' \
    -d "filterNamespaces=$NAMESPACE" \
    -d 'accumulate=true' | jq -r '
    .data[]? | to_entries[] | "  \(.key): cost=\(.value.totalCost // 0 | tostring[0:6])"
  ' 2>/dev/null | sort -k2 -rn | head -10
fi
```

---

## 3. 解决方案与风险控制

### 3.1 Kubecost/OpenCost 配置优化

#### 方案一：Kubecost 自定义定价与折扣配置

```yaml
# Kubecost 自定义定价 ConfigMap
apiVersion: v1
kind: ConfigMap
metadata:
  name: pricing-configs
  namespace: kubecost
data:
  aws.json: |
    {
      "provider": "AWS",
      "description": "AWS Custom Pricing",
      "CPU": "0.031611",
      "spotCPU": "0.006655",
      "RAM": "0.004237",
      "spotRAM": "0.000892",
      "GPU": "2.000000",
      "spotGPU": "0.500000",
      "storage": "0.00013888889",
      "zoneNetworkEgress": "0.01",
      "regionNetworkEgress": "0.01",
      "internetNetworkEgress": "0.09"
    }
---
# Kubecost 分摊规则配置
apiVersion: v1
kind: ConfigMap
metadata:
  name: kubecost-cost-analyzer
  namespace: kubecost
data:
  # 共享 namespace 的成本分摊方式
  sharedNamespaces: "kube-system,monitoring,logging,ingress-nginx"
  sharedOverhead: "true"
  sharedLabels: "app.kubernetes.io/instance,app.kubernetes.io/part-of"
  
  # 折扣配置
  discount: "0.2"                    # 20% 的 negotiated 折扣
  negotiatedDiscount: "0.15"         # 15% 的 negotiated 折扣
  customDiscounts: |
    {
      "CPU": 0.30,
      "RAM": 0.30,
      "GPU": 0.10,
      "Storage": 0.20
    }
```

#### 方案二：自动资源优化（VPA + Goldilocks）

```yaml
# VPA 推荐配置（仅推荐模式，不自动修改）
apiVersion: autoscaling.k8s.io/v1
kind: VerticalPodAutoscaler
metadata:
  name: my-app-vpa
  namespace: default
spec:
  targetRef:
    apiVersion: apps/v1
    kind: Deployment
    name: my-app
  updatePolicy:
    updateMode: "Off"              # 仅推荐，不自动更新
  resourcePolicy:
    containerPolicies:
    - containerName: '*'
      minAllowed:
        cpu: "50m"
        memory: "100Mi"
      maxAllowed:
        cpu: "4"
        memory: "8Gi"
      controlledResources: ["cpu", "memory"]
---
# 使用 Goldilocks 批量生成 VPA 推荐
# helm install goldilocks fairwinds-stable/goldilocks --namespace goldilocks --create-namespace
# kubectl label ns default goldilocks.fairwinds.com/enabled=true
```

### 3.2 闲置资源清理自动化

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl delete`：删除资源（可由声明式清单重建）

> **🔴 高风险操作警告**
>
> 下方命令属于不可逆或高影响操作，执行前请确认：
> - 已备份关键数据与配置
> - 处于批准的变更窗口期
> - 已获得相关责任人授权
> - 已准备回滚或恢复方案
> - 目标集群、Namespace、节点/资源名称正确无误

``` bash
# 🔴 高风险：可能造成数据丢失或服务中断，执行前需备份、变更审批与回滚方案
#!/bin/bash
# 闲置资源自动清理脚本（dry-run 模式）
# 建议作为 CronJob 定期执行

DRY_RUN=${1:-"true"}
THRESHOLD_DAYS=${2:-7}

echo "=== 闲置资源清理 ==="
echo "模式: $([ "$DRY_RUN" = "true" ] && echo "模拟运行" || echo "实际执行")"

# 1. 未挂载的 PVC
echo ""
echo "1. 未挂载的 PVC:"
kubectl get pvc --all-namespaces -o json | jq -r '
  .items[] | select(.status.phase == "Bound") |
  "\(.metadata.namespace) \(.metadata.name) \(.spec.volumeName)"
' | while read ns name pv; do
  # 检查是否有 Pod 使用该 PVC
  IN_USE=$(kubectl get pods -n $ns -o json | jq -r --arg pvc "$name" '.items[].spec.volumes[]?.persistentVolumeClaim.claimName // "" | select(. == $pvc)' | wc -l)
  if [ "$IN_USE" -eq 0 ]; then
    AGE=$(kubectl get pvc $name -n $ns -o jsonpath='{.metadata.creationTimestamp}')
    AGE_DAYS=$(( ( $(date +%s) - $(date -d "$AGE" +%s) ) / 86400 ))
    if [ "$AGE_DAYS" -ge "$THRESHOLD_DAYS" ]; then
      echo "  发现未挂载 PVC: $ns/$name (存在 ${AGE_DAYS} 天)"
      if [ "$DRY_RUN" = "false" ]; then
        kubectl delete pvc $name -n $ns && echo "    ✓ 已删除" || echo "    ✗ 删除失败"
      fi
    fi
  fi
done

# 2. 空闲 Service (无 Endpoint)
echo ""
echo "2. 无 Endpoint 的 Service:"
kubectl get svc --all-namespaces -o json | jq -r '
  .items[] | select(.spec.type == "LoadBalancer" or .spec.type == "NodePort") |
  "\(.metadata.namespace) \(.metadata.name)"
' | while read ns name; do
  ENDPOINTS=$(kubectl get endpoints $name -n $ns -o jsonpath='{.subsets[*].addresses[*].ip}' | wc -w)
  if [ "$ENDPOINTS" -eq 0 ]; then
    echo "  发现空闲 Service: $ns/$name"
    if [ "$DRY_RUN" = "false" ]; then
      kubectl delete svc $name -n $ns && echo "    ✓ 已删除" || echo "    ✗ 删除失败"
    fi
  fi
done

# 3. 已完成的 Job
echo ""
echo "3. 已完成的 Job (超过 ${THRESHOLD_DAYS} 天):"
kubectl get jobs --all-namespaces -o json | jq -r '
  .items[] | select(.status.succeeded != null and .status.succeeded > 0) |
  "\(.metadata.namespace) \(.metadata.name) \(.status.completionTime // .metadata.creationTimestamp)"
' | while read ns name completion; do
  AGE_DAYS=$(( ( $(date +%s) - $(date -d "$completion" +%s) ) / 86400 ))
  if [ "$AGE_DAYS" -ge "$THRESHOLD_DAYS" ]; then
    echo "  发现旧 Job: $ns/$name (完成于 ${AGE_DAYS} 天前)"
    if [ "$DRY_RUN" = "false" ]; then
      kubectl delete job $name -n $ns && echo "    ✓ 已删除" || echo "    ✗ 删除失败"
    fi
  fi
done

echo ""
echo "清理检查完成"
```
### 3.3 Spot 实例与自动扩缩容优化

```yaml
# Cluster Autoscaler + Spot 实例混合配置
apiVersion: apps/v1
kind: Deployment
metadata:
  name: cluster-autoscaler
  namespace: kube-system
spec:
  template:
    spec:
      containers:
      - name: cluster-autoscaler
        image: registry.k8s.io/autoscaling/cluster-autoscaler:v1.30.0
        command:
        - ./cluster-autoscaler
        - --cloud-provider=aws
        - --node-group-auto-discovery=asg:tag=k8s.io/cluster-autoscaler/enabled,k8s.io/cluster-autoscaler/my-cluster
        - --expander=least-waste
        - --skip-nodes-with-system-pods=false
        - --skip-nodes-with-local-storage=false
        - --scale-down-delay-after-add=5m
        - --scale-down-unneeded-time=2m
        - --scale-down-utilization-threshold=0.3   # 利用率 <30% 时考虑缩容
        env:
        - name: AWS_REGION
          value: "us-east-1"
---
# Spot 实例容忍度 Pod 示例
apiVersion: apps/v1
kind: Deployment
metadata:
  name: spot-workload
spec:
  replicas: 3
  template:
    spec:
      tolerations:
      - key: "node.kubernetes.io/lifecycle"
        operator: "Equal"
        value: "spot"
        effect: "NoSchedule"
      affinity:
        nodeAffinity:
          preferredDuringSchedulingIgnoredDuringExecution:
          - weight: 100
            preference:
              matchExpressions:
              - key: "node.kubernetes.io/lifecycle"
                operator: In
                values:
                - "spot"
      containers:
      - name: app
        image: my-app:v1
        resources:
          requests:
            cpu: "500m"
            memory: "1Gi"
```

### 3.4 风险控制与回滚

| 操作 | 风险等级 | 影响评估 | 回滚方案 |
|------|---------|---------|---------|
| 调整 Pod requests/limits | ⭐⭐ 中 | 可能导致 Pod 调度失败或 OOM | 恢复原始配置 |
| 清理未挂载 PVC | ⭐⭐ 中 | 数据可能仍被需要 | 从备份恢复或重新创建 |
| 启用 Spot 实例 | ⭐⭐ 中 | 实例可能随时被中断 | 迁移回 On-Demand 节点 |
| 修改 Kubecost 分摊规则 | ⭐ 低 | 影响成本归属展示 | 恢复原始 ConfigMap |
| 缩容闲置节点 | ⭐ 低 | 释放节点，不影响运行中的 Pod | 手动扩容节点组 |
| 删除空闲 Service | ⭐ 低 | 可能影响外部访问 | 重新创建 Service |

### 3.5 验证与监控

#### 成本优化效果验证脚本

```bash
#!/bin/bash
# 成本优化效果验证脚本

BEFORE_DATE=$1
AFTER_DATE=$2

if [ -z "$BEFORE_DATE" ] || [ -z "$AFTER_DATE" ]; then
  echo "用法: $0 <优化前日期(YYYY-MM-DD)> <优化后日期(YYYY-MM-DD)>"
  exit 1
fi

KUBECOST_URL="http://kubecost-cost-analyzer.kubecost.svc.cluster.local:9090"

echo "=== 成本优化效果验证 ==="
echo "优化前: $BEFORE_DATE"
echo "优化后: $AFTER_DATE"

# 获取优化前 7 天成本
BEFORE_WINDOW="${BEFORE_DATE},$(date -d "$BEFORE_DATE +7 days" +%Y-%m-%d)"
BEFORE_COST=$(curl -s "$KUBECOST_URL/model/allocation" \
  -d "window=$BEFORE_WINDOW" \
  -d 'aggregate=cluster' \
  -d 'accumulate=true' | jq -r '.data[]? | to_entries[0].value.totalCost // 0')

# 获取优化后 7 天成本
AFTER_WINDOW="${AFTER_DATE},$(date -d "$AFTER_DATE +7 days" +%Y-%m-%d)"
AFTER_COST=$(curl -s "$KUBECOST_URL/model/allocation" \
  -d "window=$AFTER_WINDOW" \
  -d 'aggregate=cluster' \
  -d 'accumulate=true' | jq -r '.data[]? | to_entries[0].value.totalCost // 0')

echo ""
echo "优化前 7 天总成本: $BEFORE_COST"
echo "优化后 7 天总成本: $AFTER_COST"

if (( $(echo "$BEFORE_COST > 0" | bc -l) )); then
  SAVINGS=$(echo "$BEFORE_COST - $AFTER_COST" | bc)
  SAVINGS_PCT=$(echo "scale=2; ($SAVINGS / $BEFORE_COST) * 100" | bc)
  echo "节省金额: $SAVINGS ($SAVINGS_PCT%)"
fi
```

#### Prometheus 成本监控告警

```yaml
# 成本监控告警
groups:
- name: cost-optimization
  rules:
  - alert: NamespaceCostSpike
    expr: |
      (
        sum(kubecost_container_memory_working_set_bytes) by (namespace)
        -
        avg_over_time(sum(kubecost_container_memory_working_set_bytes) by (namespace)[7d:1h])
      ) / avg_over_time(sum(kubecost_container_memory_working_set_bytes) by (namespace)[7d:1h]) > 0.5
    for: 1h
    labels:
      severity: warning
    annotations:
      summary: "Namespace 成本异常增长"
      description: "Namespace {{ $labels.namespace }} 的资源使用量比 7 天均值增长超过 50%"

  - alert: HighIdleNodeCost
    expr: |
      (
        sum(node_cpu_seconds_total{mode="idle"}) by (instance)
        /
        sum(node_cpu_seconds_total) by (instance)
      ) > 0.8
    for: 1h
    labels:
      severity: info
    annotations:
      summary: "节点 CPU 空闲率过高"
      description: "节点 {{ $labels.instance }} CPU 空闲率超过 80%，建议评估缩容"

  - alert: UnmountedPVCost
    expr: |
      count(kube_persistentvolumeclaim_info unless on(persistentvolumeclaim, namespace) kube_pod_spec_volumes_persistentvolumeclaims_info) > 0
    for: 1h
    labels:
      severity: warning
    annotations:
      summary: "存在未挂载的 PVC"
      description: "有 PVC 未被任何 Pod 使用，可能产生不必要的存储费用"

  - alert: SpotInstanceInterruptionRate
    expr: |
      rate(spot_instance_interruptions_total[1h]) > 0.1
    for: 10m
    labels:
      severity: info
    annotations:
      summary: "Spot 实例中断率较高"
      description: "Spot 实例中断频繁，建议评估是否需要更多 On-Demand 实例"
```

### 3.6 最佳实践

1. **标签规范**：强制要求所有工作负载添加 `team`、`project`、`cost-center`、`environment` 标签
2. **分层展示**：Kubecost 中配置 namespace 级别的分摊规则，系统组件成本按 CPU/内存比例分摊到业务 namespace
3. **预算告警**：为每个 team/project 设置月度预算告警，超过 80% 时预警
4. **定时清理**：使用 CronJob 定期清理未挂载 PVC、已完成 Job、空闲 Service
5. **Spot 优先**：对无状态、可容忍中断的工作负载强制使用 Spot 实例，成本可降低 60-90%
6. **Right-sizing 周期**：每月使用 VPA/Goldilocks 分析一次资源请求推荐值
7. **账单对账**：每月将 Kubecost 估算与云厂商实际账单对比，偏差 >10% 时调查原因

### 典型问题案例

#### 案例一：开发环境 GPU 实例夜间持续扣费

**问题描述**：开发团队每天下班后的 GPU 成本仍占全天的 40%。

**根本原因**：Jupyter Notebook 和训练 Job 没有配置自动停止，GPU Pod 24 小时运行。

**解决方案**：
1. 为开发 namespace 配置 Kubecost 预算告警
2. 部署 kube-janitor 或 similar 工具，为 Pod 添加 `ttl` annotation 自动清理
3. 配置 Cluster Autoscaler 的 `scale-down-unneeded-time=10m`，快速缩容空闲 GPU 节点

#### 案例二：日志存储费用月增 100%

**问题描述**：EFS/NFS 日志存储费用从 $500/月飙升到 $1200/月。

**根本原因**：应用将 DEBUG 级别日志全部输出到 stdout，Fluent Bit 转发到持久化存储，且 retention 设为 90 天。

**解决方案**：
1. 将应用日志级别从 DEBUG 调整为 WARN（生产环境）
2. 配置日志 retention 为 30 天，归档到冷存储（如 S3 Glacier）
3. 在 Fluent Bit 中配置日志采样，对 DEBUG 日志只保留 10%

#### 案例三：Kubecost 与 AWS 账单偏差 40%

**问题描述**：财务部门反馈 Kubecost 显示的成本比 AWS Cost Explorer 低 40%。

**根本原因**：Kubecost 默认未配置 AWS 的 Reserved Instance 和 Saving Plans 折扣，也未计入 Enterprise Discount Program。

**解决方案**：
1. 在 Kubecost 的 values.yaml 中配置 `pricingConfig` 包含 negotiatedDiscount
2. 上传 AWS CUR (Cost and Usage Report) 到 S3 并让 Kubecost 读取
3. 配置 `awsSpotDataRegion` 和 `awsSpotDataBucket` 以准确计算 Spot 价格

## Related

- 08-docker-troubleshooting-guide
- 16-troubleshooting-guide
- [[domain-17-system-foundation/速查卡/go.md|go]]
- [[domain-17-system-foundation/速查卡/helm.md|helm]]
- [[domain-17-system-foundation/速查卡/k8s.md|k8s]]
- [[domain-19-landscape-references/领域索引/observability-index.md|Observability 可观测性知识图谱索引]]
- [[domain-19-landscape-references/领域索引/ai-gpu-index.md|AI / GPU 基础设施知识图谱索引]]
- [[domain-19-landscape-references/领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]

## See Also

- [[32-发布/package/2026-07-02_18-40/corpus/core/domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/15-monitoring-observability/02-opentelemetry-troubleshooting|02-opentelemetry-troubleshooting]]
- [[32-发布/package/2026-07-02_18-40/corpus/core/domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/15-monitoring-observability/03-ebpf-observability-troubleshooting|03-ebpf-observability-troubleshooting]]
- [[32-发布/package/2026-07-02_18-40/corpus/core/domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/15-monitoring-observability/01-monitoring-observability-troubleshooting|01-monitoring-observability-troubleshooting]]
- [[32-发布/package/2026-07-02_18-40/corpus/core/domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/15-monitoring-observability/02-opentelemetry-troubleshooting|02-opentelemetry-troubleshooting]]

```

<!-- risk-assessed -->
