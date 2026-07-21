---
title: Kubernetes Cost Optimization — Kubecost, Resource Right-Sizing, and FinOps Automation
description: K8s 成本优化 — Kubecost 部署、资源 Right-Sizing 自动化、Spot/抢占实例、节点池优化、成本异常检测
summary: 构建 Kubernetes FinOps 自动化体系，实现成本可视化、优化推荐与异常告警
category: practice
tags:
- cost-optimization
- kubecost
- finops
- right-sizing
- spot-instances
tier: core
created: '2026-07-21'
last_updated: '2026-07-21'
difficulty: advanced
domain: operations
---
# Kubernetes 成本优化与 FinOps 自动化

> 构建成本可视化、优化推荐与自动化治理的 FinOps 体系。

## 成本构成分析

| 成本项 | 占比（典型） | 优化手段 |
|--------|-------------|----------|
| 计算（节点） | 60-70% | Right-Sizing、Spot、缩零 |
| 存储（PV） | 15-20% | 生命周期策略、压缩 |
| 网络（LB/带宽） | 5-10% | 内网通信、CDN |
| 托管服务费 | 5-10% | 预留实例、Savings Plan |
| 其他（快照/日志） | 5% | 保留策略、降采样 |

## Kubecost 部署

```bash
# Helm 安装 Kubecost
helm repo add kubecost https://kubecost.github.io/cost-analyzer/
helm install kubecost kubecost/cost-analyzer \
  --namespace kubecost --create-namespace \
  --set kubecostToken="your-token" \
  --set prometheus.server.persistentVolume.size=100Gi \
  --set kubecostProductConfigs.clusterName="production-cn-east" \
  --set kubecostProductConfigs.currencyCode="CNY"
```

### 成本分配配置

```yaml
# kubecost-values.yaml
kubecostProductConfigs:
  clusterName: production-cn-east
  # 自定义标签作为成本分配维度
  labelMappingConfigs:
    enabled: true
    configs:
      department_label: team
      environment_label: environment
      product_label: product
  # 共享成本分摊
  sharedNamespaces:
    - kube-system
    - monitoring
    - istio-system
  sharedOverhead: 15  # 15% 管理费用分摊

# 成本异常检测
costAnalyzer:
  realtime:
    enabled: true
  # 预算告警
  alerts:
    enabled: true
```

## 资源 Right-Sizing

### VPA 推荐模式

```yaml
# 全命名空间 VPA 推荐（不自动修改）
apiVersion: autoscaling.k8s.io/v1
kind: VerticalPodAutoscaler
metadata:
  name: recommendation-only
  namespace: production
spec:
  targetRef:
    apiVersion: apps/v1
    kind: Deployment
    name: api-server
  updatePolicy:
    updateMode: "Off"  # 仅推荐
  resourcePolicy:
    containerPolicies:
      - containerName: "*"
        minAllowed:
          cpu: 50m
          memory: 64Mi
        maxAllowed:
          cpu: "16"
          memory: 32Gi
```

### 自动化 Right-Sizing 脚本

```bash
#!/bin/bash
# right-sizing-report.sh — 生成优化建议
echo "=== 资源优化报告 $(date) ==="

echo "--- CPU 过度配置（使用率 < 20%）---"
kubectl top pods -A --no-headers | while read ns pod cpu mem; do
  cpu_val=${cpu%m}
  requested=$(kubectl get pod $pod -n $ns -o jsonpath='{.spec.containers[0].resources.requests.cpu}' 2>/dev/null)
  if [ -n "$requested" ]; then
    req_val=$(echo $requested | sed 's/m//')
    if [ "$req_val" -gt 0 ] 2>/dev/null; then
      usage_pct=$((cpu_val * 100 / req_val))
      if [ "$usage_pct" -lt 20 ]; then
        echo "  $ns/$pod: 使用 ${cpu_val}m / 请求 ${req_val}m (${usage_pct}%)"
      fi
    fi
  fi
done

echo "--- 未使用 PVC ---"
kubectl get pvc -A -o json | jq -r '.items[] | 
  select(.status.phase == "Bound") | 
  select(.metadata.annotations["pv.kubernetes.io/bind-completed"] != "true") |
  "\(.metadata.namespace)/\(.metadata.name) \(.spec.resources.requests.storage)"'

echo "--- 空闲节点（CPU < 10%）---"
kubectl top nodes --no-headers | while read node cpu mem; do
  cpu_pct=${cpu%\%}
  if [ "$cpu_pct" -lt 10 ]; then
    echo "  $node: CPU ${cpu_pct}%"
  fi
done
```

## Spot/抢占实例策略

### Karpenter Spot 配置

```yaml
apiVersion: karpenter.sh/v1beta1
kind: NodePool
metadata:
  name: spot-workers
spec:
  template:
    spec:
      requirements:
        - key: karpenter.sh/capacity-type
          operator: In
          values: ["spot"]  # 仅 Spot
        - key: node.kubernetes.io/instance-type
          operator: In
          values:
            - m5.large
            - m5.xlarge
            - m5.2xlarge
            - m6g.large
            - m6g.xlarge
      taints:
        - key: karpenter.sh/capacity-type
          value: spot
          effect: NoSchedule
      nodeClassRef:
        name: default
  limits:
    cpu: "500"
    memory: 1000Gi
  disruption:
    consolidationPolicy: WhenUnderutilized
    expireAfter: 168h  # 7 天轮换
---
# 工作负载容忍 Spot
apiVersion: apps/v1
kind: Deployment
metadata:
  name: batch-processor
spec:
  template:
    spec:
      tolerations:
        - key: karpenter.sh/capacity-type
          value: spot
          operator: Equal
          effect: NoSchedule
      # Spot 中断处理
      terminationGracePeriodSeconds: 120
      containers:
        - name: processor
          lifecycle:
            preStop:
              exec:
                command: ["sh", "-c", "sleep 30 && /app/graceful-shutdown"]
```

### Spot 适用性矩阵

| 工作负载 | 适合 Spot | 理由 |
|----------|-----------|------|
| 批处理/CI | ✅ | 可重试、无状态 |
| 开发/测试 | ✅ | 中断可接受 |
| 无状态 Web（多副本） | ✅ | 副本冗余 |
| 消息消费者 | ✅ | 可重连 |
| 数据库 | ❌ | 有状态、不可中断 |
| 单副本关键服务 | ❌ | 中断影响大 |
| GPU 训练（有 Checkpoint） | ⚠️ | 需断点续训 |

## 成本异常检测

### Prometheus 告警

```yaml
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: cost-alerts
  namespace: monitoring
spec:
  groups:
    - name: cost
      rules:
        - alert: NamespaceCostSpike
          expr: |
            sum(kubecost_cluster_management_cost) by (namespace)
            > 1.5 * avg_over_time(sum(kubecost_cluster_management_cost) by (namespace)[7d:1h])
          for: 6h
          labels:
            severity: warning
          annotations:
            summary: "命名空间 {{ $labels.namespace }} 成本异常增长 > 50%"
        - alert: IdleResourceHigh
          expr: |
            sum(kubecost_cluster_idle_cost) / sum(kubecost_cluster_total_cost) > 0.3
          for: 24h
          labels:
            severity: warning
          annotations:
            summary: "集群空闲资源成本占比 > 30%"
```

## FinOps 运营节奏

| 频率 | 活动 | 参与者 |
|------|------|--------|
| 每周 | 成本报告 + 异常审查 | FinOps + 团队 Lead |
| 双周 | Right-Sizing 执行 | SRE + 开发 |
| 每月 | 预留实例/Savings Plan 评估 | FinOps + 财务 |
| 每月 | 未使用资源清理 | SRE |
| 季度 | 架构成本评审 | 架构师 + FinOps |

## 成本优化检查清单

- [ ] Kubecost/OpenCost 部署并配置成本分配
- [ ] 所有工作负载设置 requests/limits
- [ ] VPA 推荐模式覆盖所有 Deployment
- [ ] 非关键工作负载使用 Spot 实例
- [ ] 开发/测试环境非工作时间缩零（KEDA Cron）
- [ ] PVC 生命周期策略（未使用 30 天告警）
- [ ] 预留实例/Savings Plan 覆盖基线负载
- [ ] 成本标签（team/environment/product）100% 覆盖
- [ ] 月度成本报告自动生成
- [ ] 成本异常告警配置

---

## 多集群成本管理

### 架构设计

```
Cluster-A (CN)    Cluster-B (US)    Cluster-C (EU)
┌───────────┐  ┌───────────┐  ┌───────────┐
│ OpenCost  │  │ OpenCost  │  │ OpenCost  │
│ Agent     │  │ Agent     │  │ Agent     │
└─────┬─────┘  └─────┬─────┘  └─────┬─────┘
      │              │              │
      └──────────────┼──────────────┘
                     ▼
         ┌─────────────────┐
         │ Kubecost        │
         │ Enterprise      │
         │ (汇聚层)       │
         └─────────────────┘
                     │
                     ▼
         ┌─────────────────┐
         │ Grafana         │
         │ 成本看板       │
         └─────────────────┘
```

### 跨集群成本查询

```promql
# 按集群汇总成本
sum(kubecost_node_total_cost) by (cluster)

# 按团队汇总（跨集群）
sum(kubecost_cluster_management_cost * on(namespace) group_left(label_team)
  kube_namespace_labels{label_team!=""}) by (label_team, cluster)

# 成本效率（每 CPU 核成本）
avg(kubecost_node_total_cost / kube_node_status_capacity{resource="cpu"}) by (cluster)
```

---

## 存储成本优化

### PVC 生命周期管理

```yaml
# 自动化 PVC 清理 CronJob
apiVersion: batch/v1
kind: CronJob
metadata:
  name: pvc-cleanup
  namespace: kube-system
spec:
  schedule: "0 3 * * 0"  # 每周日 3:00
  jobTemplate:
    spec:
      template:
        spec:
          serviceAccountName: pvc-cleaner
          containers:
            - name: cleaner
              image: bitnami/kubectl:latest
              command:
                - /bin/sh
                - -c
                - |
                  echo "=== PVC 清理报告 $(date) ==="
                  # 查找未绑定 Pod 的 PVC
                  kubectl get pvc -A -o json | jq -r '.items[] |
                    select(.status.phase == "Bound") |
                    select(.metadata.annotations["pvc-cleanup/retain"] != "true") |
                    "\(.metadata.namespace)/\(.metadata.name)"' | while read pvc; do
                    NS=$(echo $pvc | cut -d/ -f1)
                    NAME=$(echo $pvc | cut -d/ -f2)
                    # 检查是否有 Pod 使用
                    IN_USE=$(kubectl get pods -n $NS -o json | jq --arg pvc "$NAME" \
                      '[.items[] | select(.spec.volumes[]?.persistentVolumeClaim.claimName == $pvc)] | length')
                    if [ "$IN_USE" -eq 0 ]; then
                      echo "⚠️ 未使用: $pvc"
                      # 添加标签标记，30 天后自动删除
                      kubectl label pvc $NAME -n $NS pvc-cleanup/orphaned=true --overwrite
                    fi
                  done
          restartPolicy: OnFailure
---
# StorageClass 配置回收策略
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: standard-retain
provisioner: kubernetes.io/aws-ebs
reclaimPolicy: Retain  # 防止误删
volumeBindingMode: WaitForFirstConsumer
allowVolumeExpansion: true
```

### 存储分层策略

| 数据类型 | 存储类型 | 保留期 | 成本 |
|----------|----------|--------|------|
| 数据库 | SSD (gp3/io2) | 永久 | 高 |
| 应用日志 | HDD (st1) | 30 天 | 中 |
| 监控指标 | 对象存储 (S3) | 90 天 | 低 |
| 备份/归档 | 冷存储 (Glacier) | 1-7 年 | 极低 |
| 临时数据 | emptyDir/local | Pod 生命周期 | 免费 |

---

## 网络成本优化

### 流量成本分析

```bash
# 🟢 分析跨 AZ 流量
kubectl get pods -A -o wide | awk '{print $1, $8}' | sort | uniq -c | sort -rn

# 检查跨 AZ 通信（云厂商通常收费）
# 使用 topology spread constraints 减少跨 AZ 流量
```

### 优化策略

```yaml
# Topology Spread 减少跨 AZ 流量
apiVersion: apps/v1
kind: Deployment
metadata:
  name: api-server
spec:
  template:
    spec:
      topologySpreadConstraints:
        - maxSkew: 1
          topologyKey: topology.kubernetes.io/zone
          whenUnsatisfiable: DoNotSchedule
          labelSelector:
            matchLabels:
              app: api-server
      # 优先同 AZ 通信
      affinity:
        podAffinity:
          preferredDuringSchedulingIgnoredDuringExecution:
            - weight: 100
              podAffinityTerm:
                labelSelector:
                  matchLabels:
                    app: api-server
                topologyKey: topology.kubernetes.io/zone
---
# 内网通信（避免公网 LB 费用）
apiVersion: v1
kind: Service
metadata:
  name: internal-api
  annotations:
    # AWS: 使用内网 NLB
    service.beta.kubernetes.io/aws-load-balancer-internal: "true"
    # 阿里云: 内网 SLB
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-address-type: "intranet"
spec:
  type: LoadBalancer
  selector:
    app: api-server
  ports:
    - port: 80
      targetPort: 8080
```

---

## FinOps 成熟度模型

| 级别 | 名称 | 特征 | 工具 | 建议时间 |
|------|------|------|------|----------|
| L1 | 无感知 | 无成本可视化，月底看账单 | 云账单 | - |
| L2 | 可视化 | 部署 Kubecost，按 NS 看成本 | Kubecost | 2 周 |
| L3 | 分配 | 标签分配、Showback 报告 | 标签 + 报告 | 1 月 |
| L4 | 优化 | Right-Sizing、Spot、缩零 | VPA + KEDA + Karpenter | 3 月 |
| L5 | 治理 | 预算告警、Chargeback、审批流 | 全套体系 | 6 月 |
| L6 | 自动化 | AI 推荐、自动执行、持续优化 | 自研/商业 | 12 月 |

### 快速启动路线图

```
第 1 周: 部署 Kubecost/OpenCost
    ├── 配置成本分配标签
    ├── 识别 Top 10 成本消耗者
    └── 生成基线报告

第 2 周: 快速优化（Low-Hanging Fruit）
    ├── 清理未使用 PVC/LB/EIP
    ├── 缩零开发环境（非工作时间）
    └── 调整过度配置的 requests

第 3-4 周: 系统性优化
    ├── VPA 推荐模式全覆盖
    ├── Spot 实例迁移（批处理/CI）
    └── 预留实例/Savings Plan 评估

第 2 月: 治理体系
    ├── 预算告警配置
    ├── 月度成本报告自动化
    └── 团队 Showback

第 3 月: 持续优化
    ├── Karpenter 节点优化
    ├── 存储分层策略
    └── 网络成本优化
```

---

## 自动化成本报告

### 月度报告生成脚本

```bash
#!/bin/bash
# 🟢 generate-cost-report.sh — 月度成本报告

REPORT_DATE=$(date +%Y-%m)
KUBECOST_URL="http://kubecost.kubecost:9090"

echo "══════════════════════════════════════════"
echo "  Kubernetes 成本报告 - $REPORT_DATE"
echo "══════════════════════════════════════════"

# 1. 总成本
echo -e "\n📊 集群总成本"
curl -s "$KUBECOST_URL/model/assets?window=month&accumulate=false" | \
  jq '.code, .data | to_entries[] | {cluster: .key, totalCost: .value.totalCost}'

# 2. 按命名空间
echo -e "\n📊 命名空间成本 Top 10"
curl -s "$KUBECOST_URL/allocation?window=month&aggregate=namespace" | \
  jq -r '.data[0] | to_entries | sort_by(-.value.totalCost) | .[:10][] |
    "  \(.key): $\(.value.totalCost | round)"'

# 3. 空闲资源
echo -e "\n📊 空闲资源成本"
curl -s "$KUBECOST_URL/allocation?window=month&aggregate=cluster" | \
  jq '.data[0][] | {cluster: .name, idleCost: .totalCost - .cpuCost - .ramCost}'

# 4. 优化建议
echo -e "\n💡 优化建议"
curl -s "$KUBECOST_URL/savings/rightSizing" | \
  jq -r '.data[] | "  \(.namespace)/\(.controllerName): 节省 $\(..monthlyRate | round)/月"' | head -20

echo -e "\n══════════════════════════════════════════"
```

## Related

- [[生产运维/成本治理/index.md|成本治理]]
- [[生产运维/成本治理/02-idle-resource-right-sizing.md|资源 Right-Sizing]]
- [[集群基础/性能调优/06-autoscaling-hpa-vpa-keda.md|自动缩放]]
