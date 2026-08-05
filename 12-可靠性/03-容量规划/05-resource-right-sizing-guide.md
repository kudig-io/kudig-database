---
title: 资源配额右调优指南
description: 用 VPA + Goldilocks 实现资源 requests/limits 的数据驱动式右调优工作流
summary: VPA Off 模式采集 → Goldilocks 出建议 → GitOps 灰度落地 → 持续校准的右调优闭环
category: reliability
tags:
- slo
- sli
- reliability
- vpa
- goldilocks
- resource
- cost
tier: core
created: '2026-07-11'
last_updated: 2026-07
difficulty: advanced
audience:
- SRE
- 架构师
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。

# 资源配额右调优指南

> **核心原则**：资源右调优不是"凭感觉拍个数字"，而是**让真实负载数据说话**。`requests` 决定调度与成本，`limits` 决定稳定性——拍脑袋设的值要么浪费 30% 成本，要么在峰值 OOM。数据驱动的右调优能同时收回浪费、避免事故。

## 右调优闭环

```
部署(初始值) ──▶ VPA Off 采集(2w) ──▶ Goldilocks 建议 ──▶ GitOps 灰度落地
                         ▲                                          │
                         └────────── 持续校准(月度) ◀────────────────┘
```

## 为什么用 VPA + Goldilocks 组合

- **VPA**：内核级采集 CPU/内存真实用量，给出推荐值。但单独用只输出原始建议，无门槛分级，直接用容易激进。
- **Goldilocks**：在 VPA 之上加一层，按 `ensure` 策略输出不同激进程度的建议（本本分分 / 平衡 / 激进），适合不同稳定性需求的负载。

```
            VPA(原始建议)
                 │
        Goldilocks 分层
    ┌────────────┼────────────┐
    ▼            ▼            ▼
 保守(slack)   平衡(default)  紧凑(packing)
 低风险         一般服务       省成本/可容忍波动
```

## 第 1 步：启用 VPA 采集（仅推荐模式）

🟡 **中危**：起步必须 `updateMode: Off`，只采集不改 Pod，避免生产 Pod 被重启。

```yaml
apiVersion: autoscaling.k8s.io/v1
kind: VerticalPodAutoscaler
metadata: { name: api-vpa, namespace: prod }
spec:
  targetRef: { apiVersion: apps/v1, kind: Deployment, name: api }
  updatePolicy: { updateMode: "Off" }   # ★ 只采集，不自动改
  resourcePolicy:
    containerPolicies:
    - containerName: '*'
      controlledResources: ["cpu","memory"]
```

采集至少 2 周（覆盖一个完整业务周期，含峰值）。`kubectl describe vpa api-vpa` 看 recommendation。

## 第 2 步：部署 Goldilocks

```bash
# 🟡 中危：会安装集群级组件
helm repo add fairwinds-stable https://charts.fairwinds.com/stable
helm install goldilocks fairwinds-stable/goldilocks -n goldilocks --create-namespace

# 🟢 只读：给 namespace 打标签启用
kubectl label namespace prod goldilocks.fairwinds.com/enabled=true
```

```yaml
# 给 Deployment 加注解选择策略
apiVersion: apps/v1
kind: Deployment
metadata:
  name: api
  annotations:
    goldilocks.fairwinds.com/v1beta1-ensure: "balanced"   # conservative|balanced|projection
```

## 第 3 步：读取建议

```bash
# 🟢 只读
kubectl get vpa -n prod
# NAME       RECOMMENDED   TARGET
# api-vpa   True          Deployment/api

kubectl get -n goldilocks recommendation -o yaml
```

输出示例：
```yaml
recommendation:
  cpu:
    recommendation: 250m      # 当前 500m，可省一半
  memory:
    recommendation: 384Mi     # 当前 1Gi，可省 60%
```

## 第 4 步：GitOps 灰度落地

⚠️ **永远不要直接应用 Goldilocks 的自动 patch**——通过 GitOps 走 PR，人工评审后再灰度。

```yaml
# Git 仓库里的 Deployment，基于建议调小
spec:
  template:
    spec:
      containers:
      - name: api
        resources:
          requests: { cpu: 250m, memory: 384Mi }   # ← 调整
          limits:   { cpu: 1,    memory: 768Mi }    # limit = 2-3x request
        # limits 不调太死：CPU limit 会 throttle 导致延迟飙升
```

**灰度策略**：先在 Staging 跑 3 天 → Prod 先改 1 个副本（金丝雀）→ 观察 P99 与 OOM 率 → 全量。

## limits 与 requests 的取舍

| 项目 | requests | limits |
|------|----------|--------|
| 作用 | 调度 + 成本 | 稳定性 + 防抢占 |
| CPU | 必须精确 | 建议 unset 或 2-3x；过紧会 CFS throttle |
| 内存 | 必须精确 | 建议 = request 或略高；超限 OOMKill |

⚠️ **CPU throttle 陷阱**：设了 CPU limit 且应用突发用 CPU 时，会被 cgroup 限流导致延迟尖刺。对延迟敏感服务，**不设 CPU limit** 或用 `cpu.cfs_quota_us` 谨慎。

## 验证与持续校准

```bash
# 🟢 只读：对比调优前后成本
kubectl cost namespace --show-cpu --show-memory -n prod

# 调优后 1 个月复查：实际用量是否仍贴合建议？
kubectl describe vpa api-vpa | grep -A5 Recommendation
```

每月跑一次"右调优日"，处理：
1. 新上线服务（无历史数据）的初始值
2. 流量模式变化的服务（促销季前后）
3. Goldilocks 建议漂移 > 30% 的服务

## 常见陷阱

1. **VPA Auto 模式 + HPA on CPU**：两者争抢 CPU 目标，Pod 反复重启。组合用时 HPA 必须用自定义指标。
2. **采集窗口太短**：只采 2 天就调，会漏掉周末/促销峰值，调完峰值就 OOM。
3. **limits = requests**：看似"精确"，实际是放弃了 burst 缓冲，瞬时峰值就 OOM。
4. **调一次就不管**：负载会漂移，右调优是持续工程，不是一次性项目。

## 自动化右调优脚本

### 批量采集 VPA 建议

```bash
#!/bin/bash
# 🟢 低风险：批量采集 VPA 建议
set -euo pipefail

NAMESPACE=${1:-production}
OUTPUT_FILE="/tmp/vpa-recommendations-$(date +%Y%m%d).csv"

echo "=== 采集 VPA 建议: $NAMESPACE ==="

echo "Deployment,Current_CPU,Recommended_CPU,Current_Memory,Recommended_Memory,CPU_Savings,Memory_Savings" > $OUTPUT_FILE

# 获取所有 Deployment
for deploy in $(kubectl get deploy -n $NAMESPACE -o name | cut -d'/' -f2); do
  # 获取当前配置
  CURRENT_CPU=$(kubectl get deploy $deploy -n $NAMESPACE -o jsonpath='{.spec.template.spec.containers[0].resources.requests.cpu}')
  CURRENT_MEM=$(kubectl get deploy $deploy -n $NAMESPACE -o jsonpath='{.spec.template.spec.containers[0].resources.requests.memory}')
  
  # 获取 VPA 建议
  VPA_NAME="${deploy}-vpa"
  REC_CPU=$(kubectl get vpa $VPA_NAME -n $NAMESPACE -o jsonpath='{.status.recommendation.containerRecommendations[0].target.cpu}' 2>/dev/null || echo "N/A")
  REC_MEM=$(kubectl get vpa $VPA_NAME -n $NAMESPACE -o jsonpath='{.status.recommendation.containerRecommendations[0].target.memory}' 2>/dev/null || echo "N/A")
  
  # 计算节省 (简化计算)
  CPU_SAVINGS="N/A"
  MEM_SAVINGS="N/A"
  
  echo "$deploy,$CURRENT_CPU,$REC_CPU,$CURRENT_MEM,$REC_MEM,$CPU_SAVINGS,$MEM_SAVINGS" >> $OUTPUT_FILE
done

echo "=== 采集完成: $OUTPUT_FILE ==="
cat $OUTPUT_FILE
```

### 自动生成调整 PR

```bash
#!/bin/bash
# 🟡 中风险：自动生成资源调整 PR
set -euo pipefail

NAMESPACE=${1:-production}
GIT_REPO=${2:-"git@github.com:org/k8s-configs.git"}

echo "=== 生成资源调整 PR ==="

# 1. 克隆仓库
TEMP_DIR=$(mktemp -d)
git clone $GIT_REPO $TEMP_DIR
cd $TEMP_DIR

# 2. 创建分支
git checkout -b right-sizing-$(date +%Y%m%d)

# 3. 遍历 VPA 建议
for vpa in $(kubectl get vpa -n $NAMESPACE -o name | cut -d'/' -f2); do
  DEPLOY=$(kubectl get vpa $vpa -n $NAMESPACE -o jsonpath='{.spec.targetRef.name}')
  REC_CPU=$(kubectl get vpa $vpa -n $NAMESPACE -o jsonpath='{.status.recommendation.containerRecommendations[0].target.cpu}')
  REC_MEM=$(kubectl get vpa $vpa -n $NAMESPACE -o jsonpath='{.status.recommendation.containerRecommendations[0].target.memory}')
  
  if [ -n "$REC_CPU" ] && [ -n "$REC_MEM" ]; then
    echo "调整 $DEPLOY: CPU=$REC_CPU, Memory=$REC_MEM"
    # 使用 yq 或 sed 修改 YAML 文件
    # yq -i ".spec.template.spec.containers[0].resources.requests.cpu = \"$REC_CPU\"" deployments/$DEPLOY.yaml
    # yq -i ".spec.template.spec.containers[0].resources.requests.memory = \"$REC_MEM\"" deployments/$DEPLOY.yaml
  fi
done

# 4. 提交并创建 PR
git add .
git commit -m "chore: right-sizing resource requests based on VPA recommendations"
git push origin right-sizing-$(date +%Y%m%d)

# 5. 创建 PR (使用 gh CLI)
gh pr create --title "Right-sizing resource requests" --body "基于 VPA 建议自动生成的资源调整"

echo "=== PR 创建完成 ==="
```

## 成本节省报告

### 月度报告生成

```bash
#!/bin/bash
# 🟢 低风险：生成成本节省报告
set -euo pipefail

REPORT_DATE=$(date +%Y-%m)
OUTPUT_FILE="/tmp/cost-savings-report-$REPORT_DATE.md"

echo "=== 生成成本节省报告 ==="

cat > $OUTPUT_FILE <<EOF
# 资源右调优成本节省报告

**报告月份**: $REPORT_DATE
**生成时间**: $(date)

## 总体节省

| 指标 | 调优前 | 调优后 | 节省 |
|-----|-------|-------|------|
| CPU 请求总量 | 100 cores | 65 cores | 35% |
| 内存请求总量 | 200 Gi | 140 Gi | 30% |
| 月度成本 | ¥50,000 | ¥35,000 | ¥15,000 |

## 服务级别明细

| 服务 | CPU 节省 | 内存节省 | 月度节省 |
|-----|---------|---------|----------|
| api-gateway | 40% | 35% | ¥3,000 |
| order-service | 30% | 25% | ¥2,500 |
| user-service | 45% | 40% | ¥2,000 |

## 建议

1. 继续监控调优后的服务稳定性
2. 下个月重点关注新上线服务
3. 考虑将节省的预算用于混沌工程实验

---
*本报告由自动化脚本生成*
EOF

echo "报告已生成: $OUTPUT_FILE"
cat $OUTPUT_FILE
```

## 多环境策略

### 环境差异化配置

| 环境 | 策略 | CPU 建议 | 内存 建议 |
|-----|------|---------|----------|
| **Development** | 激进节省 | P50 用量 | P50 用量 |
| **Staging** | 平衡 | P90 用量 | P90 用量 |
| **Production** | 保守稳定 | P99 用量 + 20% buffer | P99 用量 + 30% buffer |

### 环境特定 VPA 配置

```yaml
# Production VPA - 保守策略
apiVersion: autoscaling.k8s.io/v1
kind: VerticalPodAutoscaler
metadata:
  name: api-vpa-prod
  namespace: production
spec:
  targetRef:
    apiVersion: apps/v1
    kind: Deployment
    name: api
  updatePolicy:
    updateMode: "Off"  # 仅推荐
  resourcePolicy:
    containerPolicies:
      - containerName: '*'
        controlledResources: ["cpu", "memory"]
        minAllowed:
          cpu: 100m
          memory: 128Mi
        maxAllowed:
          cpu: 8
          memory: 16Gi
---
# Staging VPA - 平衡策略
apiVersion: autoscaling.k8s.io/v1
kind: VerticalPodAutoscaler
metadata:
  name: api-vpa-staging
  namespace: staging
spec:
  targetRef:
    apiVersion: apps/v1
    kind: Deployment
    name: api
  updatePolicy:
    updateMode: "Auto"  # 自动调整
  resourcePolicy:
    containerPolicies:
      - containerName: '*'
        controlledResources: ["cpu", "memory"]
```

## 特殊工作负载处理

### 批处理作业

```yaml
# 批处理作业使用固定资源，不用 VPA
apiVersion: batch/v1
kind: Job
metadata:
  name: data-processing
spec:
  template:
    spec:
      containers:
        - name: processor
          image: data-processor:latest
          resources:
            requests:
              cpu: 2
              memory: 4Gi
            limits:
              cpu: 4
              memory: 8Gi
      restartPolicy: OnFailure
```

### 有状态服务 (StatefulSet)

```yaml
# 数据库等 StatefulSet 需要更保守的配置
apiVersion: apps/v1
kind: StatefulSet
metadata:
  name: postgres
spec:
  template:
    spec:
      containers:
        - name: postgres
          resources:
            requests:
              cpu: 2
              memory: 8Gi
            limits:
              # 数据库不建议设 CPU limit
              memory: 16Gi
```

### GPU 工作负载

```yaml
# GPU 工作负载的资源配置
apiVersion: apps/v1
kind: Deployment
metadata:
  name: ml-inference
spec:
  template:
    spec:
      containers:
        - name: inference
          image: ml-inference:latest
          resources:
            requests:
              cpu: 4
              memory: 16Gi
              nvidia.com/gpu: 1
            limits:
              cpu: 8
              memory: 32Gi
              nvidia.com/gpu: 1
```

## 监控与告警

### PrometheusRule 资源告警

```yaml
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: resource-alerts
  namespace: monitoring
spec:
  groups:
    - name: resource.rules
      rules:
        # CPU 使用率过高
        - alert: ContainerCPUHigh
          expr: |
            rate(container_cpu_usage_seconds_total{container!="POD",container!=""}[5m])
            /
            kube_pod_container_resource_requests{resource="cpu"}
            > 0.9
          for: 10m
          labels:
            severity: warning
          annotations:
            summary: "容器 {{ $labels.container }} CPU 使用率超过 90%"

        # 内存使用率过高
        - alert: ContainerMemoryHigh
          expr: |
            container_memory_working_set_bytes{container!="POD",container!=""}
            /
            kube_pod_container_resource_requests{resource="memory"}
            > 0.9
          for: 10m
          labels:
            severity: warning
          annotations:
            summary: "容器 {{ $labels.container }} 内存使用率超过 90%"

        # OOMKilled 频繁
        - alert: ContainerOOMKilled
          expr: |
            increase(kube_pod_container_status_terminated_reason{reason="OOMKilled"}[1h]) > 0
          for: 0m
          labels:
            severity: critical
          annotations:
            summary: "容器 {{ $labels.container }} 发生 OOMKilled"

        # 资源请求与实际使用偏差过大
        - alert: ResourceRequestMismatch
          expr: |
            abs(
              kube_pod_container_resource_requests{resource="cpu"}
              -
              rate(container_cpu_usage_seconds_total[1h])
            ) / kube_pod_container_resource_requests{resource="cpu"} > 0.5
          for: 24h
          labels:
            severity: info
          annotations:
            summary: "容器 {{ $labels.container }} CPU 请求与实际使用偏差超过 50%，建议右调优"
```

## 相关

- [[12-可靠性/03-容量规划/04-autoscaling-best-practices.md|06 autoscaling best practices]]
- [[12-可靠性/03-容量规划/03-resource-quota-limitrange.md|03 resource quota limitrange]]
- [[12-可靠性/03-容量规划/06-capacity-planning-forecasting.md|24 capacity planning forecasting]]

<!-- risk-assessed -->
