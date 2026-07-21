---
title: 混沌工程概述与原则
description: '# 混沌工程概述与原则'
summary: '# 混沌工程概述与原则'
category: domain
tags:
- chaos-engineering
- reliability
- sre
- testing
- scheduler
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 混沌工程概述与原则 是什么
- 如何 混沌工程概述与原则
- Kubernetes 09 reliability engineering 最佳实践
trigger_keywords:
- 混沌工程概述与原则
- reliability
- engineering
prerequisites:
- kubectl-basics
- sre-practices
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 混沌工程概述与原则

> **定义**: 混沌工程是在分布式系统上进行实验的学科，目的是建立对系统抵御生产环境中失控条件能力的信心。

## 五大原则

### 1. 建立稳态假设 (Build a Hypothesis around Steady State Behavior)

```
稳态: 系统正常运行的可度量行为

示例假设:
  "当单个可用区问题时，订单服务的 P99 延迟增加不超过 50%"
  "当 30% 的 Pod 被随机终止时，API 错误率不超过 1%"
```

### 2. 引入真实世界事件 (Vary Real-world Events)

```
真实问题类型:
├── 基础设施层
│   ├── 节点问题 (Node failure)
│   ├── 网络分区 (Network partition)
│   └── 磁盘问题 (Disk failure)
├── Kubernetes 层
│   ├── Pod 随机终止 (Pod kill)
│   ├── 调度器问题 (Scheduler failure)
│   └── API Server 延迟 (API latency)
├── 应用层
│   ├── 依赖服务超时 (Dependency timeout)
│   ├── 数据库连接池耗尽 (Connection pool exhaust)
│   └── 内存泄漏 (Memory leak)
└── 运维层
    ├── 配置错误 (Config error)
    ├── 证书过期 (Certificate expiry)
    └── 人为操作失误 (Human error)
```

### 3. 生产环境运行 (Run Experiments in Production)

```
为什么必须在生产环境?
  - 测试环境 ≠ 生产环境（流量模式、数据规模、配置差异）
  - 只有在生产环境才能验证真实用户影响

安全措施:
  - 爆炸半径控制（见原则 5）
  - 快速回滚机制
  - 降级开关 (Kill switch)
```

### 4. 自动化持续执行 (Automate Experiments to Run Continuously)

```
手动执行 → 半自动 → 全自动

全自动混沌工程流水线:
  CI/CD → 部署 → 自动混沌实验 → 验证 SLO → 通过/回滚
```

### 5. 最小化爆炸半径 (Minimize Blast [[Radius|Radius]])

```
爆炸半径控制手段:
  - 只对特定用户/流量执行实验
  - 时间窗口限制（低峰期）
  - 快速终止机制
  - 金丝雀范围（1% → 5% → 25%）
```

## 混沌工程成熟度模型

| 级别 | 特征 | 工具 |
|------|------|------|
| **1. 萌芽** | 随机故障注入 | 手动 kubectl delete pod |
| **2. 基础** | 有计划的人工实验 | Chaos Mesh Dashboard |
| **3. 中级** | 自动化实验，事后分析 | [[Litmus|Litmus]] + CI/CD 集成 |
| **4. 高级** | 生产环境持续运行，自动回滚 | Gremlin / 自研平台 |
| **5. 专家** | 智能故障预测，AI 驱动 | 智能混沌平台 |

## Kubernetes 混沌实验分类

### 按故障层级分类

| 层级 | 实验类型 | 典型场景 | 工具支持 |
|-----|---------|---------|----------|
| **基础设施层** | 节点故障 | 节点宕机、磁盘满、CPU 压力 | Chaos Mesh / Litmus |
| **网络层** | 网络故障 | 延迟、丢包、分区、DNS 失败 | Chaos Mesh / Istio |
| **Pod 层** | Pod 故障 | Pod Kill、OOM、调度失败 | Chaos Mesh / kubectl |
| **应用层** | 应用故障 | 异常注入、资源耗尽、线程死锁 | Chaos Mesh / 自研 |
| **云平台层** | 云资源故障 | AZ 故障、API 限流、存储故障 | Litmus / 自研 |

### 混沌工具对比

| 工具 | 优势 | 劣势 | 适用场景 | K8s 版本 |
|-----|------|------|---------|----------|
| **Chaos Mesh** | CNCF 毕业、功能全面、UI 友好 | 资源占用较高 | 通用混沌实验 | v1.24+ |
| **Litmus** | 实验市场丰富、GitOps 友好 | 学习曲线稍陡 | 企业级混沌 | v1.24+ |
| **Gremlin** | 商业支持、安全控制强 | 付费、闭源 | 企业生产环境 | 全版本 |
| **kubectl + 脚本** | 零依赖、灵活 | 无 UI、无自动化 | 简单实验/学习 | 全版本 |
| **Istio 故障注入** | 与服务网格集成 | 仅限 HTTP/gRPC | 应用层故障 | v1.24+ |

## Chaos Mesh 实战配置

### 安装与部署

```bash
# 🟡 中风险：安装 Chaos Mesh 到集群
# 使用 Helm 安装（生产环境建议独立命名空间）
helm repo add chaos-mesh https://charts.chaos-mesh.org
helm install chaos-mesh chaos-mesh/chaos-mesh \
  --namespace chaos-mesh \
  --create-namespace \
  --set chaosDaemon.runtime=containerd \
  --set chaosDaemon.socketPath=/run/containerd/containerd.sock \
  --set dashboard.create=true \
  --set dashboard.securityMode=false

# 验证安装
kubectl get pods -n chaos-mesh
kubectl port-forward -n chaos-mesh svc/chaos-dashboard 2333:2333
```

### Pod Kill 实验

```yaml
apiVersion: chaos-mesh.org/v1alpha1
kind: PodChaos
metadata:
  name: pod-kill-experiment
  namespace: production
spec:
  action: pod-kill
  mode: one  # 只影响一个 Pod
  selector:
    namespaces:
      - production
    labelSelectors:
      app: payment-api
  scheduler:
    cron: "@every 10m"  # 每 10 分钟执行一次
  duration: "30s"
---
# 网络延迟实验
apiVersion: chaos-mesh.org/v1alpha1
kind: NetworkChaos
metadata:
  name: network-delay-experiment
  namespace: production
spec:
  action: delay
  mode: all
  selector:
    namespaces:
      - production
    labelSelectors:
      app: order-service
  delay:
    latency: "200ms"
    correlation: "50"
    jitter: "50ms"
  direction: to
  duration: "60s"
---
# CPU 压力实验
apiVersion: chaos-mesh.org/v1alpha1
kind: StressChaos
metadata:
  name: cpu-stress-experiment
  namespace: production
spec:
  mode: one
  selector:
    namespaces:
      - production
    labelSelectors:
      app: inventory-service
  stressors:
    cpu:
      workers: 2
      load: 80
  duration: "120s"
```

### 实验工作流（Workflow）

```yaml
apiVersion: chaos-mesh.org/v1alpha1
kind: Workflow
metadata:
  name: az-failure-simulation
  namespace: chaos-mesh
spec:
  entry: the-entry
  templates:
    - name: the-entry
      templateType: Serial
      deadline: 10m
      children:
        - check-health-before
        - inject-failure
        - wait-and-observe
        - recover
        - check-health-after
    - name: check-health-before
      templateType: Suspend
      deadline: 1m
    - name: inject-failure
      templateType: PodChaos
      deadline: 5m
      podChaos:
        action: pod-kill
        mode: fixed-percent
        value: "30"  # 杀死 30% 的 Pod
        selector:
          namespaces: [production]
          labelSelectors:
            app: api-gateway
    - name: wait-and-observe
      templateType: Suspend
      deadline: 3m
    - name: recover
      templateType: PodChaos
      podChaos:
        action: pod-kill
        mode: none
    - name: check-health-after
      templateType: Suspend
      deadline: 1m
```

## 生产环境安全保障

### 实验前检查清单

| 序号 | 检查项 | 验证方法 | 通过标准 |
|-----|--------|---------|----------|
| 1 | 实验范围已明确 | 检查 selector 配置 | 仅影响目标服务 |
| 2 | 爆炸半径已控制 | 检查 mode/value | 影响比例合理 |
| 3 | 回滚方案已准备 | 确认实验可停止 | 有快速终止机制 |
| 4 | 监控告警已配置 | 检查 Grafana/Prometheus | 关键指标可观测 |
| 5 | 通知已发送 | Slack/邮件通知 | 相关团队已知晓 |
| 6 | 时间窗口合理 | 确认低峰期 | 避开业务高峰 |
| 7 | 数据已备份 | 检查备份状态 | 有状态服务已备份 |
| 8 | 审批已通过 | 检查审批记录 | 变更单已批准 |

### 紧急终止机制

```bash
# 🔴 高风险：紧急停止所有混沌实验
# 停止指定实验
kubectl delete podchaos pod-kill-experiment -n production

# 停止命名空间内所有实验
kubectl delete podchaos,networkchaos,stresschaos --all -n production

# 停止所有 Chaos Mesh 实验（紧急按钮）
kubectl delete workflows,podchaos,networkchaos,stresschaos,iochaos,timechaos,kernelchaos --all -A

# 禁用 Chaos Mesh（完全停止）
kubectl scale deployment chaos-controller-manager -n chaos-mesh --replicas=0
```

### 安全护栏配置

```yaml
# Chaos Mesh 安全配置
apiVersion: v1
kind: ConfigMap
metadata:
  name: chaos-mesh-config
  namespace: chaos-mesh
data:
  # 禁止实验的命名空间
  forbiddenNamespaces: "kube-system,kube-public,monitoring"
  # 最大影响 Pod 比例
  maxImpactPercent: "50"
  # 实验最大持续时间
  maxDuration: "30m"
  # 需要审批的实验类型
  approvalRequired: "kernelchaos,hostchaos"
```

## 混沌实验自动化流水线

### CI/CD 集成架构

```
代码提交 → CI 构建 → 部署 Staging → 自动混沌实验 → SLO 验证 → 生产发布
                                        │
                                        ├─ Pod Kill 实验
                                        ├─ 网络延迟实验
                                        ├─ 依赖故障实验
                                        └─ 资源压力实验
                                        │
                                        ▼
                                  实验报告生成
                                  ├─ SLO 达标率
                                  ├─ 恢复时间
                                  └─ 异常事件
```

### GitHub Actions 集成示例

```yaml
name: Chaos Engineering Pipeline
on:
  workflow_dispatch:
  schedule:
    - cron: '0 2 * * 0'  # 每周日凌晨 2 点

jobs:
  chaos-experiment:
    runs-on: ubuntu-latest
    steps:
      - name: Checkout
        uses: actions/checkout@v4

      - name: Setup kubectl
        uses: azure/setup-kubectl@v3

      - name: Run Chaos Experiment
        run: |
          # 应用实验配置
          kubectl apply -f chaos/experiments/pod-kill.yaml
          
          # 等待实验完成
          sleep 300
          
          # 收集实验结果
          kubectl get podchaos -n production -o json > chaos-results.json

      - name: Validate SLO
        run: |
          # 检查错误率
          ERROR_RATE=$(curl -s 'http://prometheus:9090/api/v1/query?query=sum(rate(http_requests_total{status=~"5.."}[5m]))/sum(rate(http_requests_total[5m]))' | jq -r '.data.result[0].value[1]')
          
          if (( $(echo "$ERROR_RATE > 0.01" | bc -l) )); then
            echo "❌ SLO 未达标: 错误率 $ERROR_RATE > 1%"
            exit 1
          fi
          echo "✅ SLO 达标: 错误率 $ERROR_RATE"

      - name: Generate Report
        run: |
          echo "# 混沌实验报告 $(date)" > chaos-report.md
          echo "## 实验结果" >> chaos-report.md
          cat chaos-results.json | jq '.' >> chaos-report.md
```

## 实验结果分析与报告

### 关键评估指标

| 指标 | 定义 | 目标值 | 计算方法 |
|-----|------|-------|----------|
| **MTTD** | 平均检测时间 | < 1min | 故障注入到告警触发 |
| **MTTR** | 平均恢复时间 | < 5min | 故障注入到服务恢复 |
| **SLO 达标率** | 实验期间 SLO 保持 | > 99% | 成功请求/总请求 |
| **影响范围** | 受影响用户比例 | < 5% | 受影响用户/总用户 |
| **自动恢复率** | 无需人工干预恢复 | > 80% | 自动恢复次数/总故障次数 |

### 实验报告模板

```markdown
# 混沌实验报告

## 基本信息
- 实验日期: 2026-07-21
- 实验类型: Pod Kill (30%)
- 目标服务: payment-api
- 持续时间: 10 分钟

## 实验结果
| 指标 | 目标 | 实际 | 状态 |
|-----|------|------|------|
| MTTD | < 1min | 45s | ✅ |
| MTTR | < 5min | 2m30s | ✅ |
| SLO 达标率 | > 99% | 99.2% | ✅ |
| 错误率峰值 | < 5% | 3.2% | ✅ |

## 发现的问题
1. 第 3 分钟出现短暂 5xx 峰值（8%），持续 15s
2. 部分客户端未配置重试，导致用户可见错误

## 改进行动
- [ ] 优化 HPA 扩容速度（当前 60s → 30s）
- [ ] 客户端 SDK 添加重试逻辑
- [ ] 增加 PDB minAvailable 到 3
```

## 混沌工程检查清单

### 团队就绪度评估

| 维度 | 检查项 | 状态 |
|-----|--------|------|
| **文化** | 团队理解混沌工程目的（不是制造故障） | ☐ |
| **文化** | 管理层支持混沌实验 | ☐ |
| **工具** | 混沌平台已部署（Chaos Mesh/Litmus） | ☐ |
| **工具** | 监控告警覆盖关键服务 | ☐ |
| **流程** | 实验审批流程已建立 | ☐ |
| **流程** | 紧急终止机制已验证 | ☐ |
| **实践** | 已从简单实验开始（Pod Kill） | ☐ |
| **实践** | 已进行生产环境实验 | ☐ |
| **度量** | 实验结果有量化报告 | ☐ |
| **度量** | 改进项有跟踪闭环 | ☐ |

## 相关

- deployment]]
- [[可靠性/混沌工程/03-chaos-experiment-design.md|03 chaos experiment design]]


<!-- risk-assessed -->
