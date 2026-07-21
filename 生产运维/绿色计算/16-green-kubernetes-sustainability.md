---
title: Green Kubernetes — Sustainable Computing Practices
description: 绿色 Kubernetes 实践 — 碳足迹度量、能效优化、资源调度策略、可持续计算架构
summary: 云原生基础设施的可持续发展实践，涵盖碳排放度量、节能调度、硬件利用率优化
category: practice
tags:
- green-computing
- sustainability
- carbon-footprint
- energy-efficiency
- finops
tier: supporting
created: '2026-07-21'
last_updated: '2026-07-21'
difficulty: advanced
domain: operations
---
# 绿色 Kubernetes — 可持续计算实践

> 降低云原生基础设施的碳排放与能耗，实现可持续 IT 运营。

## 为什么关注绿色计算

| 维度 | 数据 |
|------|------|
| 全球数据中心能耗 | 占全球电力 1-1.5%（~460 TWh/年） |
| 碳排放 | 约占全球 CO₂ 排放 0.5-1% |
| 趋势 | AI/ML 工作负载推动能耗快速增长 |
| 企业压力 | ESG 报告、碳中和承诺、监管要求 |

## 碳足迹度量

### 度量框架

```
碳排放 = 能耗(kWh) × 电网碳强度(gCO₂/kWh)

K8s 工作负载碳排放 = 
  CPU 能耗 + 内存能耗 + 存储能耗 + 网络能耗
```

### 度量工具

| 工具 | 能力 | 粒度 |
|------|------|------|
| Cloud Carbon Footprint | 多云碳排放估算 | 服务/区域 |
| Kepler | 基于 eBPF 的 Pod 能耗 | Pod/容器 |
| Scaphandre | 进程级能耗监控 | 进程 |
| Green Metrics Tool | 端到端碳度量 | 应用 |

### Kepler 部署（eBPF 能耗监控）

```yaml
# Kepler DaemonSet 简化配置
apiVersion: apps/v1
kind: DaemonSet
metadata:
  name: kepler
  namespace: kepler
spec:
  template:
    spec:
      containers:
        - name: kepler
          image: quay.io/sustainable_computing_io/kepler:latest
          securityContext:
            privileged: true
          env:
            - name: NODE_IP
              valueFrom:
                fieldRef:
                  fieldPath: status.hostIP
          ports:
            - containerPort: 9103
              name: metrics
          volumeMounts:
            - name: proc
              mountPath: /proc
            - name: sys
              mountPath: /sys
      volumes:
        - name: proc
          hostPath:
            path: /proc
        - name: sys
          hostPath:
            path: /sys
```

```promql
# 按命名空间统计能耗
sum by (namespace) (
  rate(kepler_container_joules_total[5m])
) / 1000  # 转换为 kWh

# 碳排放估算（假设电网碳强度 500gCO₂/kWh）
sum(kepler_container_joules_total) / 3600000 * 500
```

## 能效优化策略

### 1. 资源 Right-Sizing

```yaml
# VPA 自动推荐资源
apiVersion: autoscaling.k8s.io/v1
kind: VerticalPodAutoscaler
metadata:
  name: app-vpa
spec:
  targetRef:
    apiVersion: apps/v1
    kind: Deployment
    name: my-app
  updatePolicy:
    updateMode: "Auto"
  resourcePolicy:
    containerPolicies:
      - containerName: app
        minAllowed:
          cpu: 100m
          memory: 128Mi
        maxAllowed:
          cpu: "4"
          memory: 8Gi
```

### 2.  bin-packing 调度优化

```yaml
# 优先 bin-packing（减少活跃节点数）
apiVersion: kubescheduler.config.k8s.io/v1
kind: KubeSchedulerConfiguration
profiles:
  - schedulerName: default-scheduler
    pluginConfig:
      - name: NodeResourcesFit
        args:
          scoringStrategy:
            type: MostAllocated  # bin-packing
            resources:
              - name: cpu
                weight: 1
              - name: memory
                weight: 1
```

### 3. 节点自动缩放

```yaml
# Cluster Autoscaler — 缩容空闲节点
apiVersion: apps/v1
kind: Deployment
metadata:
  name: cluster-autoscaler
spec:
  template:
    spec:
      containers:
        - name: cluster-autoscaler
          command:
            - ./cluster-autoscaler
            - --scale-down-enabled=true
            - --scale-down-delay-after-add=10m
            - --scale-down-unneeded-time=10m
            - --scale-down-utilization-threshold=0.5
            - --max-graceful-termination-sec=600
```

### 4. 工作负载时间调度

```yaml
# KEDA Cron 触发器 — 非工作时间缩容
apiVersion: keda.sh/v1alpha1
kind: ScaledObject
metadata:
  name: dev-env-scaler
spec:
  scaleTargetRef:
    name: dev-environment
  minReplicaCount: 0
  maxReplicaCount: 10
  triggers:
    - type: cron
      metadata:
        timezone: Asia/Shanghai
        start: "0 9 * * 1-5"   # 工作日 9 点扩容
        end: "0 18 * * 1-5"    # 18 点缩容
        desiredReplicas: "5"
```

### 5. 绿色区域选择

| 云厂商 | 绿色区域 | 碳强度 |
|--------|----------|--------|
| GCP | finland-west1 | ~25 gCO₂/kWh（水电/核电） |
| AWS | eu-north-1 (Stockholm) | ~10 gCO₂/kWh |
| Azure | Norway East | ~8 gCO₂/kWh（水电） |
| 阿里云 | 张北 | 风电/太阳能 |

## 可持续架构原则

### 设计检查清单

- [ ] 资源请求与实际使用匹配（避免过度配置）
- [ ] 启用节点自动缩放（减少空闲节点）
- [ ] 非关键工作负载使用 Spot/抢占式实例
- [ ] 开发/测试环境非工作时间缩容至零
- [ ] 选择低碳区域的云数据中心
- [ ] 使用 ARM 架构节点（能效比更高）
- [ ] 启用集群级能耗监控（Kepler）
- [ ] 定期清理未使用的 PVC/镜像/资源
- [ ] 批处理工作负载合并执行（减少峰值）
- [ ] 考虑碳感知调度（Carbon-Aware Scheduling）

### ARM 节点能效优势

| 架构 | 典型 TDP | 性能/瓦 | 适用 |
|------|----------|---------|------|
| x86 (Intel Xeon) | 150-250W | 基准 | 通用/高性能 |
| ARM (Graviton/Ampere) | 80-150W | 1.3-1.5x | Web/API/微服务 |
| GPU (A100) | 300-400W | N/A | AI/ML |

```yaml
# ARM 节点池（AWS Graviton）
apiVersion: eksctl.io/v1alpha5
kind: ClusterConfig
nodeGroups:
  - name: arm-workers
    instanceType: m7g.xlarge  # Graviton3
    desiredCapacity: 3
    labels:
      kubernetes.io/arch: arm64
    taints:
      - key: arch
        value: arm64
        effect: NoSchedule
```

## 碳感知调度

```yaml
# 基于碳强度的调度（概念）
# 通过 KEDA 外部触发器实现
apiVersion: keda.sh/v1alpha1
kind: TriggerAuthentication
metadata:
  name: carbon-aware-auth
spec:
  secretTargetRef:
    - parameter: apiKey
      name: carbon-aware-secret
      key: api-key
---
# 碳强度高时减少批处理任务
apiVersion: keda.sh/v1alpha1
kind: ScaledObject
metadata:
  name: carbon-aware-batch
spec:
  scaleTargetRef:
    name: batch-processor
  minReplicaCount: 1
  maxReplicaCount: 20
  triggers:
    - type: external
      metadata:
        scalerAddress: carbon-aware-scaler:6000
        region: ap-southeast-1
        threshold: "200"  # gCO₂/kWh 阈值
```

## 报告与治理

### ESG 报告指标

| 指标 | 计算方式 | 目标 |
|------|----------|------|
| PUE | 总能耗/IT 能耗 | < 1.2 |
| 碳强度 | gCO₂/请求 | 逐季下降 |
| 利用率 | 实际使用/配置容量 | > 65% |
| 可再生能源比例 | 绿电/总电 | > 80% |

## 碳仪表盘与可视化

### Grafana Dashboard 配置

```json
{
  "title": "Green Kubernetes — 碳排放总览",
  "panels": [
    {
      "title": "集群总能耗 (kWh)",
      "type": "stat",
      "targets": [{
        "expr": "sum(rate(kepler_container_joules_total[1h])) / 3600000"
      }]
    },
    {
      "title": "碳排放趋势 (gCO₂/h)",
      "type": "timeseries",
      "targets": [{
        "expr": "sum(rate(kepler_container_joules_total[5m])) / 3600000 * 500"
      }]
    },
    {
      "title": "Top 10 高碳命名空间",
      "type": "bargauge",
      "targets": [{
        "expr": "topk(10, sum by (namespace) (rate(kepler_container_joules_total[1h])) / 3600000 * 500)"
      }]
    },
    {
      "title": "节点能效 (性能/瓦)",
      "type": "table",
      "targets": [{
        "expr": "sum by (instance) (rate(container_cpu_usage_seconds_total[5m])) / sum by (instance) (rate(kepler_node_joules_total[5m]))"
      }]
    }
  ]
}
```

### Kepler 高级配置

```yaml
# Kepler 带模型导出器配置
apiVersion: apps/v1
kind: DaemonSet
metadata:
  name: kepler
  namespace: kepler
spec:
  template:
    spec:
      containers:
        - name: kepler
          image: quay.io/sustainable_computing_io/kepler:latest
          env:
            - name: KEPLER_METRICS_PORT
              value: "9103"
            - name: MODEL_CONFIG
              value: /etc/kepler/models
            - name: REDSHIFT_URL
              valueFrom:
                secretKeyRef:
                  name: kepler-redshift
                  key: url
                  optional: true
          volumeMounts:
            - name: models
              mountPath: /etc/kepler/models
            - name: proc
              mountPath: /proc
              readOnly: true
            - name: sys
              mountPath: /sys
              readOnly: true
      volumes:
        - name: models
          configMap:
            name: kepler-models
        - name: proc
          hostPath:
            path: /proc
        - name: sys
          hostPath:
            path: /sys
```

## 碳预算告警

### PrometheusRule — 碳排放监控

```yaml
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: green-computing-alerts
  namespace: monitoring
spec:
  groups:
    - name: carbon-budget
      rules:
        - alert: CarbonIntensityHigh
          expr: |
            sum(rate(kepler_container_joules_total[15m])) / 3600000 * 500
            > 5000  # gCO₂/h 阈值
          for: 30m
          labels:
            severity: warning
            team: platform
          annotations:
            summary: "集群碳排放超过预算 ({{ $value | printf \"%.0f\" }} gCO₂/h)"
            runbook_url: "https://wiki.internal/runbooks/carbon-budget"

        - alert: NodeEnergyInefficiency
          expr: |
            sum by (instance) (rate(kepler_node_joules_total[1h]))
            / sum by (instance) (rate(container_cpu_usage_seconds_total[1h]))
            > 100  # 焦耳/秒CPU — 能效过低
          for: 2h
          labels:
            severity: warning
          annotations:
            summary: "节点 {{ $labels.instance }} 能效过低，考虑迁移负载或缩容"

        - alert: IdleNodeEnergyWaste
          expr: |
            sum by (instance) (rate(kepler_node_joules_total[1h])) > 0
            and
            sum by (instance) (kube_pod_info) < 3
          for: 1h
          labels:
            severity: info
          annotations:
            summary: "节点 {{ $labels.instance }} 几乎空闲但仍在耗电，建议缩容"

        - alert: WeeklyCarbonBudgetExceeded
          expr: |
            sum(increase(kepler_container_joules_total[7d])) / 3600000 * 500
            > 840000  # 周预算 840 kgCO₂
          labels:
            severity: critical
            team: leadership
          annotations:
            summary: "本周碳排放已超预算，需立即采取减排措施"
```

## 碳感知调度进阶

### 碳强度数据源集成

| 数据源 | API | 覆盖 | 更新频率 |
|--------|-----|------|----------|
| WattTime | watttime.org/api | 北美/部分国际 | 5 min |
| Electricity Maps | electricitymaps.com | 全球 200+ 区域 | 1 h |
| Carbon Intensity API | carbonintensity.org.uk | 英国 | 30 min |
| 阿里云碳数据 | 内部 API | 中国区域 | 1 h |

### 碳感知 CronJob 调度器

```yaml
# 碳感知批处理调度器
apiVersion: batch/v1
kind: CronJob
metadata:
  name: carbon-aware-scheduler
  namespace: green-ops
spec:
  schedule: "*/15 * * * *"  # 每 15 分钟检查
  jobTemplate:
    spec:
      template:
        spec:
          containers:
            - name: scheduler
              image: registry.internal/green-ops/carbon-scheduler:1.2
              env:
                - name: ELECTRICITY_MAPS_TOKEN
                  valueFrom:
                    secretKeyRef:
                      name: carbon-api-creds
                      key: electricity-maps-token
                - name: REGION
                  value: "ap-southeast-1"
                - name: CARBON_THRESHOLD
                  value: "200"  # gCO₂/kWh
                - name: BATCH_QUEUE_URL
                  value: "https://batch-api.internal/queue"
              command:
                - /bin/sh
                - -c
                - |
                  CARBON=$(curl -s -H "auth-token: $ELECTRICITY_MAPS_TOKEN" \
                    "https://api.electricitymap.org/v3/carbon-intensity/latest?zone=$REGION" \
                    | jq '.carbonIntensity')
                  if [ "$CARBON" -lt "$CARBON_THRESHOLD" ]; then
                    echo "碳强度 ${CARBON}gCO₂/kWh < 阈值，释放批处理队列"
                    curl -X POST "$BATCH_QUEUE_URL/resume"
                  else
                    echo "碳强度 ${CARBON}gCO₂/kWh ≥ 阈值，暂停非紧急批处理"
                    curl -X POST "$BATCH_QUEUE_URL/pause"
                  fi
          restartPolicy: OnFailure
```

### 跨区域碳感知工作负载迁移

```yaml
# 多区域工作负载 — 优先调度到低碳区域
apiVersion: keda.sh/v1alpha1
kind: ScaledObject
metadata:
  name: carbon-aware-workload
  namespace: batch-processing
spec:
  scaleTargetRef:
    name: data-pipeline
  minReplicaCount: 2
  maxReplicaCount: 50
  triggers:
    - type: external
      metadata:
        scalerAddress: carbon-aware-scaler.green-ops:6000
        region: eu-north-1       # 低碳区域优先
        fallbackRegion: ap-southeast-1
        threshold: "150"        # gCO₂/kWh
        strategy: shift         # 迁移而非仅缩容
```

## GreenOps 成熟度模型

| 等级 | 名称 | 特征 | 关键动作 |
|------|------|------|----------|
| L1 | 无感知 | 无碳度量，纯成本驱动 | 部署 Kepler，建立基线 |
| L2 | 可视化 | 有碳仪表盘，被动观察 | Grafana Dashboard + 周报 |
| L3 | 目标驱动 | 设定碳预算，告警通知 | PrometheusRule + 预算告警 |
| L4 | 自动优化 | 碳感知调度，自动缩容 | KEDA + Carbon Scaler |
| L5 | 跨域协同 | 多集群/多区域碳调度 | 工作负载迁移 + 绿色区域 |
| L6 | 净零运营 | 碳中和/负碳，全链 ESG | 碳抵消 + 供应链 + 报告 |

### 成熟度评估脚本

```bash
#!/bin/bash
# 🟢 只读：GreenOps 成熟度快速评估
echo "=== GreenOps 成熟度评估 ==="

# L1: 是否有能耗监控
echo -n "[L1] Kepler 部署: "
kubectl get ds kepler -n kepler &>/dev/null && echo "✅" || echo "❌ 未部署"

# L2: 是否有碳仪表盘
echo -n "[L2] Grafana 碳仪表盘: "
curl -s http://grafana.monitoring:3000/api/search?query=carbon | jq length | \
  xargs -I{} sh -c '[ {} -gt 0 ] && echo "✅" || echo "❌"'

# L3: 是否有碳告警
echo -n "[L3] 碳预算告警规则: "
kubectl get prometheusrule -n monitoring -o name | grep -q green && echo "✅" || echo "❌"

# L4: 是否有碳感知缩放
echo -n "[L4] Carbon-Aware KEDA: "
kubectl get scaledobject -A -o yaml | grep -q carbon && echo "✅" || echo "❌"

# L5: 多区域调度
echo -n "[L5] 多区域碳调度: "
kubectl get scaledobject -A -o yaml | grep -q fallbackRegion && echo "✅" || echo "❌"

# 节点利用率
echo ""
echo "=== 当前集群能效指标 ==="
echo -n "节点平均 CPU 利用率: "
kubectl top nodes --no-headers | awk '{sum+=$2; n++} END {printf "%.1f%%\n", sum/n/10}'
echo -n "空闲节点数 (<10% CPU): "
kubectl top nodes --no-headers | awk '$2+0 < 100 {n++} END {print n+0}'
```

## 可持续 KPI 自动化报告

### 月度 ESG 报告 CronJob

```yaml
apiVersion: batch/v1
kind: CronJob
metadata:
  name: monthly-carbon-report
  namespace: green-ops
spec:
  schedule: "0 8 1 * *"  # 每月 1 日 08:00
  jobTemplate:
    spec:
      template:
        spec:
          containers:
            - name: reporter
              image: registry.internal/green-ops/carbon-reporter:2.0
              env:
                - name: PROMETHEUS_URL
                  value: "http://prometheus.monitoring:9090"
                - name: SLACK_WEBHOOK
                  valueFrom:
                    secretKeyRef:
                      name: report-webhooks
                      key: slack-url
                - name: EMAIL_RECIPIENTS
                  value: "cto@company.com,sustainability@company.com"
              command:
                - /bin/sh
                - -c
                - |
                  # 查询上月碳排放数据
                  START=$(date -d '1 month ago' +%Y-%m-01T00:00:00Z)
                  END=$(date +%Y-%m-01T00:00:00Z)
                  
                  TOTAL_KWH=$(curl -s "$PROMETHEUS_URL/api/v1/query" \
                    --data-urlencode "query=sum(increase(kepler_container_joules_total[30d]))/3600000" \
                    | jq -r '.data.result[0].value[1]')
                  
                  TOTAL_CO2=$(echo "$TOTAL_KWH * 500 / 1000" | bc)  # kgCO₂
                  
                  # 生成报告
                  cat > /tmp/report.json << EOF
                  {
                    "period": "$(date -d '1 month ago' +%Y-%m)",
                    "total_energy_kwh": $TOTAL_KWH,
                    "total_carbon_kg": $TOTAL_CO2,
                    "pue": 1.15,
                    "renewable_pct": 82,
                    "recommendations": [
                      "缩容空闲节点可节省 ~15% 能耗",
                      "迁移批处理到低碳区域可减少 ~20% 排放"
                    ]
                  }
                  EOF
                  
                  # 发送 Slack 通知
                  curl -X POST "$SLACK_WEBHOOK" \
                    -H 'Content-Type: application/json' \
                    -d "{\"text\": \"🌱 月度碳报告: ${TOTAL_CO2}kgCO₂, 能耗 ${TOTAL_KWH}kWh\"}"
          restartPolicy: OnFailure
```

## 多集群碳治理架构

```
┌────────────────────────────────────────────────────────┐
│              碳治理控制平面 (Global)                      │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐  │
│  │ Carbon API   │  │ 碳预算引擎    │  │ ESG 报告     │  │
│  │ (WattTime/   │  │ (Per-Region  │  │ Generator    │  │
│  │  ElecMaps)   │  │  Budget)     │  │              │  │
│  └──────┬───────┘  └──────┬───────┘  └──────────────┘  │
│         │                  │                             │
│  ┌──────▼──────────────────▼───────────────────────────┐ │
│  │          Thanos / VictoriaMetrics (全局指标)          │ │
│  └──────┬──────────────────┬──────────────────┬────────┘ │
└─────────┼──────────────────┼──────────────────┼──────────┘
          │                  │                  │
   ┌──────▼──────┐   ┌──────▼──────┐   ┌──────▼──────┐
   │ Cluster A   │   │ Cluster B   │   │ Cluster C   │
   │ (eu-north)  │   │ (ap-se-1)   │   │ (us-west)   │
   │ Kepler      │   │ Kepler      │   │ Kepler      │
   │ 碳强度:10g  │   │ 碳强度:400g │   │ 碳强度:250g │
   └─────────────┘   └─────────────┘   └─────────────┘
```

### 联邦碳指标采集

```yaml
# Thanos Ruler — 全局碳聚合规则
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: global-carbon-rules
  namespace: monitoring
spec:
  groups:
    - name: carbon-aggregation
      rules:
        - record: cluster:carbon_intensity:current
          expr: |
            sum by (cluster) (rate(kepler_container_joules_total[5m]))
            / 3600000 * 500
          labels:
            unit: gCO2_per_hour

        - record: global:total_carbon:monthly
          expr: |
            sum(increase(kepler_container_joules_total[30d]))
            / 3600000 * 500 / 1000
          labels:
            unit: kgCO2

        - record: cluster:energy_efficiency:ratio
          expr: |
            sum by (cluster) (rate(container_cpu_usage_seconds_total[1h]))
            /
            sum by (cluster) (rate(kepler_node_joules_total[1h]))
          labels:
            unit: cpu_seconds_per_joule
```

## 快速启动路线图

| 阶段 | 时间 | 目标 | 关键动作 |
|------|------|------|----------|
| Phase 1 | 第 1-2 周 | 建立基线 | 部署 Kepler + Grafana Dashboard |
| Phase 2 | 第 3-4 周 | 告警与预算 | PrometheusRule + 碳预算设定 |
| Phase 3 | 第 5-8 周 | 自动优化 | KEDA Cron 缩容 + VPA Right-Sizing |
| Phase 4 | 第 9-12 周 | 碳感知调度 | Carbon Scaler + 区域选择 |
| Phase 5 | 持续 | ESG 报告 | 月度自动化报告 + 成熟度评估 |

### 投资回报估算

| 优化措施 | 预期节能 | 实施难度 | 回收期 |
|----------|----------|----------|--------|
| 空闲节点缩容 | 20-30% | 低 | 即时 |
| VPA Right-Sizing | 15-25% | 低 | 1-2 周 |
| 非工作时间缩容 | 30-40%（dev） | 低 | 即时 |
| ARM 节点迁移 | 20-30% | 中 | 1-3 月 |
| 碳感知区域迁移 | 40-60%（碳强度） | 高 | 3-6 月 |
| 批处理合并执行 | 10-15% | 中 | 2-4 周 |

## Related

- [[生产运维/成本治理/index.md|成本治理 FinOps]]
- [[生产运维/集群治理/index.md|集群治理]]
- [[可观测性/指标/index.md|指标 Metrics]]
