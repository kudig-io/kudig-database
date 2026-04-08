# 第23章：FTA 生产环境快速启动与 SRE 集成指南

> 本章面向需要在现有 Kubernetes 集群中快速落地 FTA 方法论的 SRE 和运维团队，提供从零到一的实施路线和日常工作流集成。

**适用对象**：
- SRE / DevOps 工程师
- 运维负责人
- 平台工程团队
- 需要降低 MTTR 和提升稳定性的技术团队

**前置条件**：
- 已有运行中的 Kubernetes 集群
- 基础的可观测性能力（Prometheus / Logging / Tracing）
- 事件管理流程（Incident Management / On-call）

---

## 目录

- [23.1 FTA 30天快速启动路线图](#231-fta-30天快速启动路线图)
- [23.2 快速构建你的第一棵 Kubernetes 故障树](#232-快速构建你的第一棵-kubernetes-故障树)
- [23.3 FTA 与 SRE On-Call 工作流集成](#233-fta-与-sre-on-call-工作流集成)
- [23.4 FTA 与 Postmortem 流程集成](#234-fta-与-postmortem-流程集成)
- [23.5 FTA 与 SLO/Error Budget 管理](#235-fta-与-sloerror-budget-管理)
- [23.6 FTA 与变更管理集成](#236-fta-与变更管理集成)
- [23.7 生产事件完整 FTA 案例演练](#237-生产事件完整-fta-案例演练)
- [23.8 FTA ROI 量化模型](#238-fta-roi-量化模型)
- [23.9 FTA 常用工具快速对比](#239-fta-常用工具快速对比)
- [23.10 总结与后续行动](#2310-总结与后续行动)

---

## 23.1 FTA 30天快速启动路线图

将 FTA 引入生产环境不需要一次性完成所有工作。以下是经过实践验证的 4 周渐进式路线图：

```
┌────────────────────────────────────────────────────────────────────┐
│                    FTA 快速启动 30 天路线图                         │
└────────────────────────────────────────────────────────────────────┘

Week 1: Foundation - 识别高频故障场景
├─ Day 1-2: 回顾过去 3 个月的生产事件
│   └─ 输出：Top 5 高频/高影响故障列表
├─ Day 3-4: 选择第一个场景，初步构建故障树
│   └─ 输出：一棵完整的故障树图（至少 3 层）
└─ Day 5:   团队评审，明确底事件和检测手段
    └─ 输出：底事件清单 + 对应的检测命令/指标

Week 2: Detection - 绑定监控和告警
├─ Day 6-7: 为每个底事件配置 Prometheus 告警规则
│   └─ 输出：新增告警规则 YAML 文件
├─ Day 8-9: 配置日志模式匹配（Log Pattern）
│   └─ 输出：日志告警规则（ELK / Loki 规则）
└─ Day 10:  部署和测试告警
    └─ 输出：告警测试报告（验证覆盖度）

Week 3: Response - 从故障树到 Runbook
├─ Day 11-13: 编写基于 FTA 的诊断 Runbook
│   └─ 输出：Markdown 格式 Runbook（含决策树）
├─ Day 14-15: 将 Runbook 集成到工单系统
│   └─ 输出：工单模板更新（含 FTA 诊断路径）
└─ Day 16:    On-call 团队培训
    └─ 输出：培训材料 + Q&A 文档

Week 4: Feedback Loop - 持续改进
├─ Day 17-18: 复盘上月故障，更新故障树
│   └─ 输出：故障树 v2（新增分支/底事件）
├─ Day 19-20: 建立 FTA 更新流程（纳入 Postmortem）
│   └─ 输出：Postmortem 模板更新
├─ Day 21-25: 扩展到第 2、3 个故障场景
│   └─ 输出：新增 2 棵故障树
└─ Day 26-30: 度量 FTA 效果（MTTR / 告警数量）
    └─ 输出：FTA ROI 报告

```

### 第一周详细任务清单

**Week 1 Deliverables**

#### 任务 1.1：识别 Top 5 高频故障场景（2天）

**输入材料**：
- 过去 3-6 个月的事件记录（Incident Tickets）
- On-call 报告
- 用户投诉记录

**执行步骤**：
```bash
# 1. 从工单系统导出事件数据
# 示例：使用 Jira API
curl -u user:token "https://jira.example.com/rest/api/2/search?jql=project=OPS&created>=-90d" \
  > incidents_last_90days.json

# 2. 统计事件类型频次
jq '.issues[] | .fields.summary' incidents_last_90days.json | sort | uniq -c | sort -rn

# 3. 计算每类事件的平均解决时间和影响范围
jq '.issues[] | {summary: .fields.summary, resolution_time: .fields.resolutiondate, priority: .fields.priority}' \
  incidents_last_90days.json
```

**输出模板**：

| Rank | 故障类型 | 发生次数 | 平均 MTTR | 影响用户数 | 业务影响 | 优先级 |
|------|---------|---------|-----------|-----------|----------|--------|
| 1 | Service 不可用 | 15 | 45min | 10000+ | Critical | P0 |
| 2 | 数据库连接超时 | 12 | 30min | 5000 | High | P1 |
| 3 | Pod OOMKilled | 10 | 20min | 1000 | Medium | P2 |
| 4 | 磁盘空间不足 | 8 | 60min | 500 | Medium | P2 |
| 5 | 间歇性网络抖动 | 6 | 90min | 2000 | High | P1 |

**选择标准**：
- ☑ 高频次（发生 ≥ 5次 / 季度）
- ☑ 高影响（P0/P1 级别）
- ☑ 高 MTTR（解决时间 > 30分钟）
- ☑ 根因不明确（多次发生但未彻底解决）

#### 任务 1.2：构建第一棵故障树（2天）

**选择场景**：Service 不可用

**故障树构建步骤**：

```
Step 1: 定义顶事件
┌─────────────────────────────────────┐
│    Service 不可用                    │
│    (HTTP 503 / Connection Refused)  │
└─────────────────────────────────────┘

Step 2: 第一层分解（OR 门）
           ┌────────── Service 不可用 ──────────┐
           │                                     │
        [OR]                                  [OR]
           │                                     │
    ┌──────┴──────┐                     ┌───────┴────────┐
    │ Pod 不可用   │                     │ Service 配置错误│
    └─────────────┘                     └────────────────┘

Step 3: 继续分解 Pod 不可用
           ┌──── Pod 不可用 ────┐
           │                    │
        [OR]                 [OR]
           │                    │
    ┌──────┴─────┐       ┌─────┴──────┐
    │ Pod Crash  │       │ Pod Pending │
    └────────────┘       └─────────────┘

Step 4: 定义底事件
    Pod Crash:
      - ImagePullBackOff
      - CrashLoopBackOff
      - OOMKilled
      - Liveness Probe Failed
    
    Pod Pending:
      - 资源不足 (CPU/Memory)
      - Node NotReady
      - PVC Bound 失败
      - 污点/容忍度不匹配
```

**完整故障树**：

```
                              ┌─────────────────────────────┐
                              │    Service 不可用            │
                              │    (Top Event)              │
                              └──────────┬──────────────────┘
                                         │
                                      [OR]
                                         │
                ┌────────────────────────┼────────────────────────┐
                │                        │                        │
         ┌──────┴──────┐          ┌─────┴────┐           ┌───────┴────────┐
         │ Pod 不可用   │          │ Ingress  │           │ Service 配置   │
         │             │          │ 配置错误  │           │ 错误           │
         └──────┬──────┘          └──────────┘           └────────────────┘
                │                      ▲                        ▲
             [OR]                     (E1)                     (E2)
                │
    ┌───────────┼───────────┐
    │           │           │
┌───┴────┐  ┌───┴────┐  ┌───┴────┐
│Pod     │  │Pod     │  │Pod     │
│Crash   │  │Pending │  │Evicted │
└───┬────┘  └───┬────┘  └────────┘
    │           │            ▲
 [OR]        [OR]          (E3)
    │           │
┌───┴───┬───┬───┴───┬───┐
│       │   │       │   │
(E4)   (E5)(E6)   (E7)(E8)

底事件编号：
E1: Ingress Rule 未配置 Backend
E2: Service Selector 与 Pod Label 不匹配
E3: Node 内存压力导致 Kubelet 驱逐
E4: ImagePullBackOff
E5: CrashLoopBackOff (应用启动失败)
E6: OOMKilled
E7: CPU/Memory 资源不足
E8: Node NotReady
```

#### 任务 1.3：为底事件绑定检测手段（1天）

**底事件检测映射表**：

| 底事件 ID | 故障现象 | 检测手段 | 检测命令/指标 | 告警阈值 |
|-----------|---------|---------|--------------|---------|
| E1 | Ingress 无后端 | kubectl describe | `kubectl describe ingress <name>` 查看 Backend | 人工检查 |
| E2 | Label 不匹配 | kubectl get svc | `kubectl get svc <name> -o yaml` 对比 selector | 人工检查 |
| E3 | Pod Evicted | Kube Event | `kubectl get events --field-selector reason=Evicted` | count > 0 |
| E4 | ImagePullBackOff | Pod Status | `kubectl get pods -o json \| jq '.items[] \| select(.status.containerStatuses[].state.waiting.reason=="ImagePullBackOff")'` | count > 0 |
| E5 | CrashLoopBackOff | Pod Status | `kube_pod_container_status_waiting_reason{reason="CrashLoopBackOff"} > 0` | Prometheus |
| E6 | OOMKilled | Container Status | `kube_pod_container_status_terminated_reason{reason="OOMKilled"} > 0` | Prometheus |
| E7 | 资源不足 | Scheduler Event | `kube_pod_status_phase{phase="Pending"}` + Event reason=FailedScheduling | Pending > 5min |
| E8 | Node NotReady | Node Condition | `kube_node_status_condition{condition="Ready",status="false"} > 0` | Prometheus |

**检测脚本示例**：

```bash
#!/bin/bash
# fta_service_unavailable_check.sh
# FTA 底事件自动检测脚本

NAMESPACE="production"
SERVICE_NAME="my-service"

echo "========== FTA Service Unavailable Diagnostic =========="
echo "Timestamp: $(date)"
echo ""

# E1: Check Ingress Backend
echo "[E1] Checking Ingress Backend..."
kubectl describe ingress $SERVICE_NAME -n $NAMESPACE | grep -A 5 "Backend"

# E2: Check Service Selector vs Pod Labels
echo "[E2] Checking Service Selector..."
SERVICE_SELECTOR=$(kubectl get svc $SERVICE_NAME -n $NAMESPACE -o jsonpath='{.spec.selector}')
echo "Service Selector: $SERVICE_SELECTOR"
POD_COUNT=$(kubectl get pods -n $NAMESPACE -l app=$SERVICE_NAME --no-headers | wc -l)
echo "Matching Pods: $POD_COUNT"

# E3: Check Evicted Pods
echo "[E3] Checking Evicted Pods..."
kubectl get pods -n $NAMESPACE --field-selector=status.phase==Failed -o json | \
  jq '.items[] | select(.status.reason=="Evicted") | {name: .metadata.name, reason: .status.reason, message: .status.message}'

# E4-E6: Check Pod Status
echo "[E4-E6] Checking Pod Container Status..."
kubectl get pods -n $NAMESPACE -o json | jq '.items[] | {
  name: .metadata.name,
  phase: .status.phase,
  containers: [.status.containerStatuses[]? | {
    name: .name,
    ready: .ready,
    restartCount: .restartCount,
    state: .state,
    lastState: .lastState
  }]
}'

# E7: Check Pending Pods (Resource Shortage)
echo "[E7] Checking Pending Pods..."
kubectl get pods -n $NAMESPACE --field-selector=status.phase==Pending -o json | \
  jq '.items[] | {name: .metadata.name, reason: .status.conditions[]? | select(.type=="PodScheduled") | .message}'

# E8: Check Node Status
echo "[E8] Checking Node Status..."
kubectl get nodes -o json | jq '.items[] | {
  name: .metadata.name,
  ready: (.status.conditions[] | select(.type=="Ready") | .status),
  memoryPressure: (.status.conditions[] | select(.type=="MemoryPressure") | .status),
  diskPressure: (.status.conditions[] | select(.type=="DiskPressure") | .status)
}'

echo ""
echo "========== Diagnostic Complete =========="
```

### 第二周：配置监控告警

**Week 2 Checklist**：

- [ ] 为每个底事件创建 Prometheus 告警规则
- [ ] 配置日志告警（ELK / Loki）
- [ ] 设置告警路由和分组
- [ ] 测试告警触发和恢复
- [ ] 文档化告警与 FTA 底事件的映射关系

**Prometheus 告警规则示例**：

```yaml
# fta_service_unavailable_alerts.yaml
groups:
  - name: fta_service_unavailable
    interval: 30s
    rules:
      # E5: CrashLoopBackOff
      - alert: FTA_E5_CrashLoopBackOff
        expr: kube_pod_container_status_waiting_reason{reason="CrashLoopBackOff"} > 0
        for: 2m
        labels:
          severity: critical
          fta_event: E5
          fta_tree: service_unavailable
        annotations:
          summary: "Pod {{ $labels.namespace }}/{{ $labels.pod }} is in CrashLoopBackOff"
          description: "FTA Event E5: Application startup failure detected"
          runbook_url: "https://wiki.example.com/runbook/fta/e5-crashloop"
      
      # E6: OOMKilled
      - alert: FTA_E6_OOMKilled
        expr: increase(kube_pod_container_status_terminated_reason{reason="OOMKilled"}[5m]) > 0
        labels:
          severity: critical
          fta_event: E6
          fta_tree: service_unavailable
        annotations:
          summary: "Pod {{ $labels.namespace }}/{{ $labels.pod }} was OOMKilled"
          description: "FTA Event E6: Container exceeded memory limit"
          runbook_url: "https://wiki.example.com/runbook/fta/e6-oom"
      
      # E7: Pod Pending due to Resource Shortage
      - alert: FTA_E7_PodPendingResource
        expr: |
          kube_pod_status_phase{phase="Pending"} == 1
          and
          kube_pod_status_scheduled{condition="false"} == 1
        for: 5m
        labels:
          severity: warning
          fta_event: E7
          fta_tree: service_unavailable
        annotations:
          summary: "Pod {{ $labels.namespace }}/{{ $labels.pod }} pending for 5+ minutes"
          description: "FTA Event E7: Insufficient cluster resources (CPU/Memory)"
          runbook_url: "https://wiki.example.com/runbook/fta/e7-resource-shortage"
      
      # E8: Node NotReady
      - alert: FTA_E8_NodeNotReady
        expr: kube_node_status_condition{condition="Ready",status="false"} > 0
        for: 2m
        labels:
          severity: critical
          fta_event: E8
          fta_tree: service_unavailable
        annotations:
          summary: "Node {{ $labels.node }} is NotReady"
          description: "FTA Event E8: Node failure detected, pods may be evicted"
          runbook_url: "https://wiki.example.com/runbook/fta/e8-node-notready"
```

**告警路由配置**（AlertManager）：

```yaml
# alertmanager.yaml
route:
  receiver: 'default'
  group_by: ['fta_tree', 'fta_event']  # 按 FTA 故障树和事件分组
  group_wait: 10s
  group_interval: 5m
  repeat_interval: 3h
  
  routes:
    # FTA Service Unavailable 专用路由
    - match:
        fta_tree: service_unavailable
      receiver: 'sre-oncall'
      continue: true
      group_by: ['fta_event']  # 按底事件聚合
    
    # 关键路径事件立即通知
    - match_re:
        fta_event: ^(E5|E6|E8)$  # CrashLoop, OOM, Node Failure
      receiver: 'pagerduty-critical'
      group_wait: 0s

receivers:
  - name: 'sre-oncall'
    slack_configs:
      - channel: '#sre-oncall'
        title: 'FTA Alert: {{ .GroupLabels.fta_tree }}'
        text: |
          *Event ID*: {{ .GroupLabels.fta_event }}
          *Count*: {{ .Alerts | len }}
          *Details*: {{ range .Alerts }}{{ .Annotations.description }}{{ end }}
          *Runbook*: {{ (index .Alerts 0).Annotations.runbook_url }}
```

### 第三周：构建 Runbook

**Week 3 Checklist**：

- [ ] 编写基于 FTA 的诊断 Runbook（Markdown）
- [ ] 为每个分支添加决策树
- [ ] 包含快速诊断命令和修复步骤
- [ ] 集成到 Wiki / Confluence
- [ ] 在工单系统中添加 Runbook 链接模板

**Runbook 模板**：

```markdown
# Runbook: Service 不可用故障诊断

## 基本信息
- **Runbook ID**: RB-001
- **FTA Tree**: service_unavailable
- **Owner**: SRE Team
- **Last Updated**: 2024-01-15

## 快速诊断决策树

当收到 "Service 不可用" 告警时，按以下决策树进行诊断：

┌─────────────────────────┐
│ Service 返回 503/Timeout │
└────────┬────────────────┘
         │
         ▼
    检查 Pod 状态
         │
    ┌────┴────┐
    │         │
    ▼         ▼
Running   Not Running ────────┐
    │                         │
    │                    ┌────┴────┐
    │                    │         │
    │                    ▼         ▼
    │                 Pending   Crash/Failed
    │                    │         │
    │               执行 E7   执行 E4-E6
    │               诊断流程   诊断流程
    │
    ▼
检查 Service/Ingress 配置
    │
执行 E1-E2 诊断流程

## 诊断步骤

### Step 1: 确认问题范围

```bash
# 1.1 检查 Service Endpoint 是否就绪
kubectl get endpoints <service-name> -n <namespace>

# 1.2 检查 Pod 数量和状态
kubectl get pods -l app=<service-name> -n <namespace>

# 1.3 检查最近的事件
kubectl get events -n <namespace> --sort-by='.lastTimestamp' | tail -20
```

**预期结果**：
- Endpoints 应有 ≥ 1 个 Ready 的 IP
- Pods 应处于 Running 状态
- 无 Error/Warning 事件

**如果异常**：继续 Step 2

### Step 2: Pod 不可用诊断（E4-E6）

#### E4: ImagePullBackOff

**检测**：
```bash
kubectl describe pod <pod-name> -n <namespace> | grep -A 10 "Events"
```

**常见原因**：
1. 镜像不存在或 Tag 错误
2. 镜像仓库认证失败
3. 镜像仓库网络不可达

**修复步骤**：
```bash
# 检查 Image 名称和 Tag
kubectl get pod <pod-name> -n <namespace> -o jsonpath='{.spec.containers[0].image}'

# 检查 ImagePullSecret
kubectl get pod <pod-name> -n <namespace> -o jsonpath='{.spec.imagePullSecrets}'

# 验证 Secret 是否存在
kubectl get secret <secret-name> -n <namespace>

# 在 Node 上手动拉取镜像测试
docker pull <image-name>
```

**快速修复**：
- 更正 Deployment 中的 image 字段
- 重新创建 ImagePullSecret
- 回滚到上一个可用版本

#### E5: CrashLoopBackOff

**检测**：
```bash
kubectl logs <pod-name> -n <namespace> --previous  # 查看上一次运行的日志
kubectl describe pod <pod-name> -n <namespace>
```

**常见原因**：
1. 应用启动失败（配置错误、依赖不可用）
2. Liveness Probe 失败
3. 应用代码 Bug

**修复步骤**：
```bash
# 查看容器启动日志
kubectl logs <pod-name> -n <namespace> -c <container-name>

# 检查 ConfigMap/Secret 是否正确挂载
kubectl describe pod <pod-name> -n <namespace> | grep -A 5 "Mounts"

# 检查环境变量
kubectl exec <pod-name> -n <namespace> -- env

# 检查 Liveness Probe 配置
kubectl get pod <pod-name> -n <namespace> -o jsonpath='{.spec.containers[0].livenessProbe}'
```

**快速修复**：
- 修正 ConfigMap/Secret 配置
- 调整 Liveness Probe 阈值
- 回滚到稳定版本

#### E6: OOMKilled

**检测**：
```bash
kubectl describe pod <pod-name> -n <namespace> | grep -i "oom"
kubectl get pod <pod-name> -n <namespace> -o jsonpath='{.status.containerStatuses[0].lastState.terminated.reason}'
```

**常见原因**：
1. Memory Limit 设置过低
2. 内存泄漏
3. 流量突增

**修复步骤**：
```bash
# 查看当前内存限制
kubectl get pod <pod-name> -n <namespace> -o jsonpath='{.spec.containers[0].resources.limits.memory}'

# 查看历史内存使用情况（Prometheus）
# container_memory_usage_bytes{pod="<pod-name>"}

# 临时增加内存限制（紧急情况）
kubectl set resources deployment/<deployment-name> -n <namespace> --limits=memory=2Gi
```

**长期修复**：
- 分析内存使用趋势，调整 Request/Limit
- 排查内存泄漏（Heap Dump / Profiling）
- 实施水平扩容（HPA）

### Step 3: Pod Pending 诊断（E7）

**检测**：
```bash
kubectl describe pod <pod-name> -n <namespace> | grep -A 10 "Events"
# 查找 "FailedScheduling" 事件
```

**常见原因**：
1. 集群资源不足（CPU/Memory）
2. Node Selector / Affinity 无法满足
3. PVC 无法绑定

**修复步骤**：
```bash
# 检查集群可用资源
kubectl top nodes
kubectl describe nodes | grep -A 5 "Allocated resources"

# 检查 Pod 资源请求
kubectl get pod <pod-name> -n <namespace> -o jsonpath='{.spec.containers[0].resources.requests}'

# 检查调度约束
kubectl get pod <pod-name> -n <namespace> -o jsonpath='{.spec.nodeSelector}'
kubectl get pod <pod-name> -n <namespace> -o jsonpath='{.spec.affinity}'
```

**快速修复**：
- 删除不必要的 Pod 释放资源
- 扩容集群（添加 Node）
- 调整 Pod 资源请求
- 放宽调度约束

### Step 4: Service/Ingress 配置诊断（E1-E2）

#### E2: Service Selector 不匹配

**检测**：
```bash
# 查看 Service Selector
kubectl get svc <service-name> -n <namespace> -o jsonpath='{.spec.selector}'

# 查看 Pod Labels
kubectl get pods -n <namespace> --show-labels | grep <service-name>

# 对比是否匹配
```

**快速修复**：
```bash
# 方法1: 修正 Service Selector
kubectl edit svc <service-name> -n <namespace>

# 方法2: 修正 Pod Labels（修改 Deployment）
kubectl edit deployment <deployment-name> -n <namespace>
```

#### E1: Ingress Backend 未配置

**检测**：
```bash
kubectl describe ingress <ingress-name> -n <namespace>
# 查看 "Backend" 字段
```

**快速修复**：
```bash
kubectl edit ingress <ingress-name> -n <namespace>
# 确保 backend.service.name 和 backend.service.port 正确
```

## 升级路径

如果以上步骤均无法解决问题：

1. **联系应用负责人**：可能是应用层 Bug
2. **检查依赖服务**：数据库、缓存、消息队列
3. **升级到 L2 支持**：平台基础设施问题

## Postmortem 检查清单

事件解决后，更新 FTA：

- [ ] 此次故障是否已覆盖在 FTA 中？
- [ ] 是否需要新增底事件？
- [ ] 检测手段是否生效？
- [ ] Runbook 是否需要更新？
- [ ] 告警是否需要优化（减少噪音）？

## 相关链接

- FTA 故障树图：https://wiki.example.com/fta/service-unavailable
- 告警规则：https://github.com/example/monitoring/blob/main/prometheus/fta_alerts.yaml
- 历史事件：https://jira.example.com/issues/?jql=labels=service-unavailable
```

### 第四周：反馈循环与扩展

**Week 4 Checklist**：

- [ ] 复盘上月所有生产事件
- [ ] 识别 FTA 未覆盖的新故障模式
- [ ] 更新故障树（新增分支/底事件）
- [ ] 将 FTA 更新纳入 Postmortem 流程
- [ ] 扩展到第 2、3 个高频故障场景
- [ ] 生成 FTA ROI 初步报告

**Postmortem 模板更新**（添加 FTA 章节）：

```markdown
## FTA 分析

### 故障在 FTA 中的位置
- [ ] 此故障已在现有 FTA 中覆盖
  - 故障树: _______________
  - 底事件: _______________
- [ ] 此故障为新发现的故障模式（需要更新 FTA）

### FTA 检测有效性
- [ ] 告警及时触发（检测延迟 < 1分钟）
- [ ] 告警准确（无误报）
- [ ] Runbook 指导有效（快速定位根因）

### FTA 改进行动项
- [ ] 新增底事件: _______________
- [ ] 更新检测手段: _______________
- [ ] 优化告警规则: _______________
- [ ] 更新 Runbook: _______________

### FTA 更新 PR
- PR 链接: _______________
- Review 负责人: _______________
```

---

## 23.2 快速构建你的第一棵 Kubernetes 故障树

本节将手把手带你构建一棵完整的 Kubernetes 故障树，以 **"Service 不可用"** 为例。

### 23.2.1 顶事件定义

**顶事件 (Top Event)**：
- **现象**：用户访问服务时收到 HTTP 503 / 504 错误，或连接超时
- **业务影响**：服务完全不可用，影响所有用户
- **SLI 指标**：`probe_success{service="my-service"} == 0`（外部探测失败）

### 23.2.2 第一层分解：确定主要故障路径

使用 **OR 门** 分解，表示任意一个分支发生都会导致顶事件。

```
                    ┌───────────────────────┐
                    │  Service 不可用       │
                    │  (Top Event)          │
                    └──────────┬────────────┘
                               │
                            [OR]
                               │
        ┌──────────────────────┼──────────────────────┐
        │                      │                      │
  ┌─────┴─────┐         ┌──────┴──────┐      ┌───────┴────────┐
  │ Backend   │         │ Kubernetes  │      │ Network/       │
  │ Pod       │         │ Service     │      │ Ingress        │
  │ 不可用     │         │ 层故障       │      │ 层故障          │
  └───────────┘         └─────────────┘      └────────────────┘
```

**第一层决策逻辑**：

1. **Backend Pod 不可用**：没有健康的 Pod 提供服务
2. **Kubernetes Service 层故障**：Service、Endpoint 配置错误
3. **Network/Ingress 层故障**：Ingress、负载均衡器配置错误或网络不通

### 23.2.3 第二层分解：Backend Pod 不可用

```
            ┌─────────────────────┐
            │ Backend Pod 不可用   │
            └──────────┬──────────┘
                       │
                    [OR]
                       │
        ┌──────────────┼──────────────┐
        │              │              │
  ┌─────┴─────┐  ┌─────┴─────┐  ┌─────┴─────┐
  │ Pod       │  │ Pod       │  │ Pod       │
  │ Crash     │  │ Pending   │  │ Evicted   │
  └───────────┘  └───────────┘  └───────────┘
```

### 23.2.4 第三层分解：Pod Crash 的具体原因

```
            ┌─────────────────────┐
            │ Pod Crash           │
            └──────────┬──────────┘
                       │
                    [OR]
                       │
        ┌──────────────┼──────────────┬──────────────┐
        │              │              │              │
  ┌─────┴─────┐  ┌─────┴─────┐  ┌─────┴─────┐  ┌─────┴─────┐
  │ Image     │  │ App       │  │ OOMKilled │  │ Liveness  │
  │ Pull      │  │ Startup   │  │           │  │ Probe     │
  │ Failed    │  │ Failed    │  │           │  │ Failed    │
  └───────────┘  └───────────┘  └───────────┘  └───────────┘
     (E4)            (E5)            (E6)            (E9)
```

### 23.2.5 第三层分解：Pod Pending 的具体原因

```
            ┌─────────────────────┐
            │ Pod Pending         │
            └──────────┬──────────┘
                       │
                    [OR]
                       │
        ┌──────────────┼──────────────┬──────────────┐
        │              │              │              │
  ┌─────┴─────┐  ┌─────┴─────┐  ┌─────┴─────┐  ┌─────┴─────┐
  │ CPU/Mem   │  │ Node      │  │ PVC Bind  │  │ Taint/    │
  │ 资源不足   │  │ NotReady  │  │ Failed    │  │ Toleration│
  └───────────┘  └───────────┘  └───────────┘  └───────────┘
     (E7)            (E8)           (E10)           (E11)
```

### 23.2.6 完整故障树图

```
                              ┌──────────────────────────┐
                              │   Service 不可用          │
                              │   (HTTP 503/Timeout)     │
                              └────────────┬─────────────┘
                                           │
                                        [OR]
                                           │
                ┌──────────────────────────┼──────────────────────────┐
                │                          │                          │
         ┌──────┴──────┐            ┌──────┴──────┐          ┌────────┴────────┐
         │ Backend Pod │            │ K8s Service │          │ Network/Ingress │
         │ 不可用       │            │ 层故障       │          │ 层故障           │
         └──────┬──────┘            └──────┬──────┘          └────────┬────────┘
                │                          │                          │
             [OR]                       [OR]                        [OR]
                │                          │                          │
    ┌───────────┼───────────┐              │              ┌───────────┴────────┐
    │           │           │              │              │                    │
┌───┴────┐  ┌───┴────┐  ┌───┴────┐   ┌─────┴─────┐   ┌────┴────┐       ┌─────┴─────┐
│Pod     │  │Pod     │  │Pod     │   │Service    │   │Ingress  │       │Network    │
│Crash   │  │Pending │  │Evicted │   │Selector   │   │Backend  │       │Policy/    │
│        │  │        │  │        │   │Mismatch   │   │Missing  │       │Firewall   │
└───┬────┘  └───┬────┘  └────────┘   └───────────┘   └─────────┘       └───────────┘
    │           │            ▲              (E2)           (E1)               (E12)
 [OR]        [OR]           (E3)
    │           │
┌───┴───┬───┬───┴───┬───┬───┐
│       │   │       │   │   │
(E4)   (E5)(E6)   (E7)(E8)(E10)

底事件汇总：
E1:  Ingress Backend 未配置
E2:  Service Selector 与 Pod Label 不匹配
E3:  Pod Evicted (Node 资源压力)
E4:  ImagePullBackOff
E5:  CrashLoopBackOff (应用启动失败)
E6:  OOMKilled
E7:  CPU/Memory 资源不足 (Pending)
E8:  Node NotReady
E9:  Liveness Probe Failed
E10: PVC Bound Failed
E11: Taint/Toleration 不匹配
E12: Network Policy / Firewall 阻断
```

### 23.2.7 底事件详细定义与检测

| ID | 底事件名称 | 检测指标/命令 | 告警条件 | 修复 SOP |
|----|-----------|--------------|---------|---------|
| E1 | Ingress Backend 未配置 | `kubectl describe ingress` | Backend 字段为空 | 更新 Ingress 规则 |
| E2 | Service Selector 不匹配 | `kubectl get endpoints` | Endpoints 为空 | 修正 Selector 或 Pod Labels |
| E3 | Pod Evicted | `kube_pod_status_reason{reason="Evicted"}` | > 0 | 释放 Node 资源，调整 QoS |
| E4 | ImagePullBackOff | `kube_pod_container_status_waiting_reason{reason="ImagePullBackOff"}` | > 0 | 检查镜像名称、Secret、网络 |
| E5 | CrashLoopBackOff | `kube_pod_container_status_waiting_reason{reason="CrashLoopBackOff"}` | > 0 | 查看日志，修正配置或代码 |
| E6 | OOMKilled | `kube_pod_container_status_terminated_reason{reason="OOMKilled"}` | > 0 | 增加 Memory Limit，排查泄漏 |
| E7 | 资源不足 (Pending) | `kube_pod_status_phase{phase="Pending"}` + Event | Pending > 5min | 扩容集群或调整资源请求 |
| E8 | Node NotReady | `kube_node_status_condition{condition="Ready",status="false"}` | > 0 | 检查 Node 组件（kubelet, CNI） |
| E9 | Liveness Probe Failed | Container Restarts | Restart > 3 in 5min | 调整 Probe 参数或修复应用 |
| E10 | PVC Bound Failed | `kube_persistentvolumeclaim_status_phase{phase="Pending"}` | Pending > 10min | 检查 StorageClass、PV 可用性 |
| E11 | Taint/Toleration 不匹配 | Pod Event: FailedScheduling | 包含 "taint" | 添加 Toleration 或调整 Node Taint |
| E12 | Network Policy 阻断 | 连通性测试失败 | Connection Refused | 检查 NetworkPolicy, Security Group |

### 23.2.8 故障树到诊断命令的映射

**快速诊断脚本**：

```bash
#!/bin/bash
# fta_diagnose_service_unavailable.sh
# 基于 FTA 的自动化诊断脚本

NAMESPACE=${1:-default}
SERVICE=${2:-my-service}

echo "==========================================="
echo "FTA Diagnosis: Service Unavailable"
echo "Service: $SERVICE"
echo "Namespace: $NAMESPACE"
echo "==========================================="

# Step 1: Check Service/Endpoint (E1, E2)
echo ""
echo "[Step 1] Checking Service & Endpoints..."
kubectl get svc $SERVICE -n $NAMESPACE -o wide
kubectl get endpoints $SERVICE -n $NAMESPACE

EP_COUNT=$(kubectl get endpoints $SERVICE -n $NAMESPACE -o jsonpath='{.subsets[*].addresses}' | jq '. | length')
if [ "$EP_COUNT" == "0" ]; then
  echo "❌ No Endpoints found! Checking E2 (Selector Mismatch)..."
  SELECTOR=$(kubectl get svc $SERVICE -n $NAMESPACE -o jsonpath='{.spec.selector}' | jq -r 'to_entries | map("\(.key)=\(.value)") | join(",")')
  echo "Service Selector: $SELECTOR"
  POD_COUNT=$(kubectl get pods -n $NAMESPACE -l $SELECTOR --no-headers | wc -l)
  echo "Matching Pods: $POD_COUNT"
  if [ "$POD_COUNT" == "0" ]; then
    echo "❌ E2 Confirmed: No pods match the selector!"
  fi
fi

# Step 2: Check Ingress (E1)
echo ""
echo "[Step 2] Checking Ingress..."
kubectl describe ingress -n $NAMESPACE | grep -A 3 "Backend"

# Step 3: Check Pod Status (E3-E11)
echo ""
echo "[Step 3] Checking Pod Status..."
kubectl get pods -n $NAMESPACE -l app=$SERVICE -o wide

# Check for specific failure patterns
for pod in $(kubectl get pods -n $NAMESPACE -l app=$SERVICE -o jsonpath='{.items[*].metadata.name}'); do
  echo ""
  echo "Analyzing Pod: $pod"
  
  PHASE=$(kubectl get pod $pod -n $NAMESPACE -o jsonpath='{.status.phase}')
  echo "Phase: $PHASE"
  
  # E3: Evicted
  if [ "$PHASE" == "Failed" ]; then
    REASON=$(kubectl get pod $pod -n $NAMESPACE -o jsonpath='{.status.reason}')
    if [ "$REASON" == "Evicted" ]; then
      echo "❌ E3 Detected: Pod was evicted"
      kubectl describe pod $pod -n $NAMESPACE | grep -A 5 "Message"
    fi
  fi
  
  # E4-E6: Container Status
  if [ "$PHASE" == "Running" ] || [ "$PHASE" == "Pending" ]; then
    kubectl get pod $pod -n $NAMESPACE -o json | jq '.status.containerStatuses[]? | {
      name: .name,
      ready: .ready,
      restartCount: .restartCount,
      state: .state,
      lastTerminationReason: .lastState.terminated.reason
    }'
    
    # E4: ImagePullBackOff
    IMAGE_PULL=$(kubectl get pod $pod -n $NAMESPACE -o jsonpath='{.status.containerStatuses[0].state.waiting.reason}')
    if [ "$IMAGE_PULL" == "ImagePullBackOff" ]; then
      echo "❌ E4 Detected: ImagePullBackOff"
    fi
    
    # E5: CrashLoopBackOff
    if [ "$IMAGE_PULL" == "CrashLoopBackOff" ]; then
      echo "❌ E5 Detected: CrashLoopBackOff"
      echo "Last 20 lines of logs:"
      kubectl logs $pod -n $NAMESPACE --tail=20 --previous 2>/dev/null || kubectl logs $pod -n $NAMESPACE --tail=20
    fi
    
    # E6: OOMKilled
    LAST_TERM=$(kubectl get pod $pod -n $NAMESPACE -o jsonpath='{.status.containerStatuses[0].lastState.terminated.reason}')
    if [ "$LAST_TERM" == "OOMKilled" ]; then
      echo "❌ E6 Detected: OOMKilled"
      kubectl get pod $pod -n $NAMESPACE -o jsonpath='{.spec.containers[0].resources}'
    fi
  fi
  
  # E7, E8, E10, E11: Pending Reasons
  if [ "$PHASE" == "Pending" ]; then
    echo "❌ Pod is Pending, checking reasons..."
    kubectl describe pod $pod -n $NAMESPACE | grep -A 10 "Events:"
    
    # Check for specific reasons
    kubectl get events -n $NAMESPACE --field-selector involvedObject.name=$pod --sort-by='.lastTimestamp' | tail -5
  fi
done

# Step 4: Check Node Status (E8)
echo ""
echo "[Step 4] Checking Node Status..."
kubectl get nodes -o wide
kubectl get nodes -o json | jq '.items[] | {
  name: .metadata.name,
  ready: (.status.conditions[] | select(.type=="Ready") | .status),
  memoryPressure: (.status.conditions[] | select(.type=="MemoryPressure") | .status),
  diskPressure: (.status.conditions[] | select(.type=="DiskPressure") | .status)
}'

# Step 5: Check NetworkPolicy (E12)
echo ""
echo "[Step 5] Checking NetworkPolicy..."
kubectl get networkpolicies -n $NAMESPACE

echo ""
echo "==========================================="
echo "Diagnosis Complete"
echo "==========================================="
```

---

## 23.3 FTA 与 SRE On-Call 工作流集成

### 23.3.1 On-Call 工作流现状痛点

传统 On-Call 工作流的常见问题：

```
传统 On-Call 流程：
┌──────────────┐
│ 告警触发      │
└──────┬───────┘
       │
       ▼
┌──────────────┐
│ SRE 被唤醒    │  ❌ 告警过多，疲劳
└──────┬───────┘
       │
       ▼
┌──────────────┐
│ 查看告警内容  │  ❌ 告警信息不足，需要额外调查
└──────┬───────┘
       │
       ▼
┌──────────────┐
│ 尝试各种诊断  │  ❌ 缺乏系统性诊断路径
│ 命令 (摸黑)   │
└──────┬───────┘
       │
       ▼
┌──────────────┐
│ 查找 Runbook  │  ❌ Runbook 过时或不存在
└──────┬───────┘
       │
       ▼
┌──────────────┐
│ 升级或瞎猜    │  ❌ MTTR 长，用户体验差
└──────────────┘
```

### 23.3.2 FTA 增强的 On-Call 工作流

```
FTA-Enhanced On-Call 流程：
┌──────────────┐
│ 告警触发      │
└──────┬───────┘
       │
       ▼
┌──────────────────────┐
│ 告警携带 FTA 上下文   │  ✅ fta_tree: service_unavailable
│ - FTA Tree ID        │  ✅ fta_event: E6 (OOMKilled)
│ - Bottom Event ID    │  ✅ runbook_url: https://wiki.../e6
│ - Runbook Link       │
└──────┬───────────────┘
       │
       ▼
┌──────────────────────┐
│ SRE 打开 Runbook     │  ✅ 直接跳转到 E6 OOMKilled 章节
└──────┬───────────────┘
       │
       ▼
┌──────────────────────┐
│ 执行 FTA 决策树       │  ✅ 系统性诊断，避免遗漏
│ 按步骤检查            │
└──────┬───────────────┘
       │
       ▼
┌──────────────────────┐
│ 快速定位根因并修复    │  ✅ MTTR 降低 40-60%
└──────┬───────────────┘
       │
       ▼
┌──────────────────────┐
│ 更新 FTA (如需要)     │  ✅ 持续改进
└──────────────────────┘
```

### 23.3.3 告警聚合与降噪

**问题**：一个顶事件（如 Service 不可用）可能触发多个底事件告警，导致告警风暴。

**FTA 解决方案**：

```yaml
# AlertManager 配置：按 FTA Tree 聚合
route:
  group_by: ['fta_tree']  # 同一故障树的告警聚合为一条
  group_wait: 30s         # 等待 30s 收集所有相关告警
  group_interval: 5m
  
  routes:
    - match:
        fta_tree: service_unavailable
      receiver: 'sre-oncall'
      continue: false
```

**效果示例**：

传统告警（10 条独立告警）：
```
🔴 Alert: Pod my-service-abc CrashLoopBackOff
🔴 Alert: Pod my-service-def CrashLoopBackOff
🔴 Alert: Pod my-service-ghi CrashLoopBackOff
🔴 Alert: Service my-service has no endpoints
🔴 Alert: HTTP probe failing for my-service
🔴 Alert: High error rate on my-service
... (4 more)
```

FTA 聚合后（1 条综合告警）：
```
🔴 FTA Alert: Service Unavailable (service_unavailable)
   
   📊 Triggered Events:
   - E5: CrashLoopBackOff (3 pods)
   - E2: Service has no endpoints
   
   🔍 Root Cause Analysis:
   ├─ Top Event: Service Unavailable
   ├─ Primary Branch: Backend Pod 不可用
   └─ Bottom Events:
      └─ E5: CrashLoopBackOff (应用启动失败)
   
   📖 Runbook: https://wiki.example.com/fta/service-unavailable#e5
   
   🛠️ Quick Actions:
   1. Check pod logs: kubectl logs <pod> --previous
   2. Verify ConfigMap/Secret
   3. Check dependency services
```

### 23.3.4 On-Call Playbook 决策树

**基于 FTA 的 On-Call 决策流程**：

```
┌─────────────────────────────┐
│ 收到 FTA Alert              │
└────────┬────────────────────┘
         │
         ▼
    检查 fta_tree 标签
         │
    ┌────┴─────┐
    │          │
    ▼          ▼
Known Tree  Unknown Tree
    │          │
    │          └─> 创建新 FTA 分析任务
    │
    ▼
检查 fta_event (底事件)
    │
    ├─ 单一底事件
    │  └─> 直接跳转到对应 Runbook 章节
    │
    └─ 多个底事件
       └─> 判断主路径 (Critical Path)
           │
           ├─ 关键路径事件 (E5, E6, E8)
           │  └─> 立即处理关键事件
           │
           └─ 次要事件 (E4, E7)
              └─> 创建工单，非紧急处理

执行 Runbook 诊断步骤
    │
    ├─ 快速修复成功
    │  └─> 监控恢复 -> 创建 Postmortem -> 结束
    │
    └─ 无法快速修复
       └─> 升级决策
           │
           ├─ 应用层问题
           │  └─> 联系应用 Owner
           │
           ├─ 平台层问题
           │  └─> 升级到 L2 SRE
           │
           └─ 外部依赖问题
              └─> 联系供应商 / 切换备用方案
```

### 23.3.5 On-Call Handoff 模板

**交接清单（包含 FTA 上下文）**：

```markdown
## On-Call Handoff Report

**交接时间**: 2024-01-15 09:00 AM
**On-Call**: Alice -> Bob
**时间段**: 2024-01-14 09:00 AM ~ 2024-01-15 09:00 AM

### 事件汇总

**总事件数**: 3
**P0/P1 事件**: 1
**平均 MTTR**: 25 分钟

### 详细事件

#### Event 1: Service Unavailable - my-service

- **触发时间**: 2024-01-14 14:32
- **解决时间**: 2024-01-14 14:55
- **MTTR**: 23 分钟
- **FTA Tree**: service_unavailable
- **Bottom Event**: E6 (OOMKilled)
- **根因**: Memory Limit 设置过低 (512Mi)，流量突增导致 OOM
- **修复措施**: 
  - 临时：增加 Memory Limit 到 1Gi
  - 长期：分析内存使用趋势，配置 HPA
- **FTA 更新**: 无需更新（已覆盖）
- **Postmortem**: [LINK]

#### Event 2: Disk Space Warning - node-xyz

- **触发时间**: 2024-01-14 22:15
- **解决时间**: 2024-01-14 22:40
- **MTTR**: 25 分钟
- **FTA Tree**: node_failure
- **Bottom Event**: E13 (Disk Pressure)
- **根因**: 日志未自动清理
- **修复措施**: 
  - 清理旧日志
  - 配置 logrotate
- **FTA 更新**: 无需更新
- **Postmortem**: 无需（常规运维）

### 待跟进事项

- [ ] Event 1 的 HPA 配置（Owner: Alice, Due: 2024-01-17）
- [ ] 所有 Node 的 logrotate 配置审计（Owner: Bob, Due: 2024-01-20）

### FTA 改进建议

- [ ] 考虑为 "Disk Space" 场景创建独立的故障树
- [ ] E6 (OOMKilled) 的告警阈值可以降低（当前 1 次触发，建议改为 0 次容忍）

### 当前告警状态

**活跃告警**: 0
**静默告警**: 2
  - NetworkPolicy audit (计划内维护)
  - Test cluster alert (开发环境)

### 资源状态

**集群健康度**: ✅ Healthy
**Node 可用性**: 100% (12/12 Ready)
**关键 Service SLO 达成率**: 99.95%

### 备注

本周计划内变更：
- 2024-01-16 02:00 AM: Kubernetes 升级 (1.28 -> 1.29)
- 2024-01-17 10:00 AM: Database 主从切换演练
```

### 23.3.6 PagerDuty / OpsGenie 集成

**PagerDuty Event 格式（包含 FTA 信息）**：

```json
{
  "routing_key": "YOUR_INTEGRATION_KEY",
  "event_action": "trigger",
  "payload": {
    "summary": "FTA Alert: Service Unavailable (my-service)",
    "severity": "critical",
    "source": "prometheus",
    "custom_details": {
      "fta_tree": "service_unavailable",
      "fta_event": "E6",
      "fta_event_description": "OOMKilled",
      "runbook_url": "https://wiki.example.com/fta/service-unavailable#e6",
      "affected_pods": "my-service-abc, my-service-def",
      "namespace": "production",
      "cluster": "us-west-2-prod"
    }
  },
  "links": [
    {
      "href": "https://wiki.example.com/fta/service-unavailable",
      "text": "FTA Fault Tree"
    },
    {
      "href": "https://grafana.example.com/d/service-dashboard",
      "text": "Service Dashboard"
    }
  ]
}
```

---

## 23.4 FTA 与 Postmortem 流程集成

### 23.4.1 传统 Postmortem 的局限性

**常见问题**：

1. **缺乏系统性分析框架**：根因分析依赖个人经验，容易遗漏
2. **知识无法沉淀**：Postmortem 写完就束之高阁，未转化为可执行的改进
3. **重复故障频发**：相同或类似故障反复发生
4. **改进措施不落地**：Action Items 执行率低

### 23.4.2 FTA-Enhanced Postmortem 模板

```markdown
# Postmortem: [事件标题]

## 基本信息

| 项目 | 内容 |
|------|------|
| 事件ID | INC-2024-001 |
| 发生时间 | 2024-01-15 14:32 UTC |
| 解决时间 | 2024-01-15 15:05 UTC |
| 持续时长 | 33 分钟 |
| 影响范围 | 全部用户 (10,000+) |
| 业务影响 | 服务完全不可用 |
| 严重级别 | P0 (Critical) |
| On-Call SRE | Alice, Bob |

## 执行摘要 (Executive Summary)

**一句话总结**：生产环境 `my-service` 因 Memory Limit 配置过低，在流量突增时触发 OOMKilled，导致服务不可用 33 分钟。

**业务影响**：
- 用户无法访问服务，收到 HTTP 503 错误
- 预估损失：约 $5,000 收入损失，100+ 用户投诉

**根因**：
- 直接原因：Pod 因 OOMKilled 反复重启
- 根本原因：Memory Limit 设置不合理（512Mi），未根据实际使用情况调整
- 触发因素：营销活动导致流量突增 3 倍

## FTA 分析

### 故障在 FTA 中的定位

- ✅ **此故障已在现有 FTA 中覆盖**
  - **故障树**: `service_unavailable`
  - **顶事件**: Service 不可用
  - **故障路径**: Backend Pod 不可用 -> Pod Crash -> OOMKilled
  - **底事件**: E6 (OOMKilled)

### FTA 决策树演练

```
Service 不可用
    │
    ▼
Backend Pod 不可用? ✅ 是
    │
    ▼
Pod Crash? ✅ 是
    │
    ▼
OOMKilled? ✅ 是 (E6)
```

### FTA 检测有效性评估

| 检查项 | 状态 | 说明 |
|--------|------|------|
| 告警及时触发 | ✅ 是 | Prometheus 在 1 分钟内触发告警 |
| 告警准确性 | ✅ 是 | 无误报，准确识别为 E6 |
| Runbook 指导有效 | ⚠️ 部分 | Runbook 缺少流量突增场景的快速扩容步骤 |
| 根因定位时间 | ✅ 快 | 5 分钟内定位到 OOM 根因 |
| 修复时间 | ⚠️ 偏长 | 修复耗时 28 分钟（等待 Pod 重启 + 调整配置 + 再次重启） |

### FTA 覆盖度分析

- ✅ **底事件 E6 已覆盖**
- ⚠️ **触发因素未覆盖**："流量突增"作为一个重要触发因素，未在 FTA 中体现
- ❌ **缺少预防性检测**：未配置 "内存使用率持续高位" 的预警

## 时间线 (Timeline)

| 时间 (UTC) | 事件 | 负责人 | FTA 阶段 |
|-----------|------|--------|---------|
| 14:30 | 营销活动开始，流量开始上升 | - | 触发因素 |
| 14:32 | 第一个 Pod OOMKilled | - | E6 发生 |
| 14:33 | Prometheus 触发告警 "FTA_E6_OOMKilled" | Prometheus | 检测 |
| 14:34 | PagerDuty 通知 On-Call SRE Alice | PagerDuty | - |
| 14:35 | Alice 查看告警，打开 FTA Runbook | Alice | 诊断开始 |
| 14:37 | 通过 Runbook 确认 OOM 根因 | Alice | 根因定位 |
| 14:40 | 决定增加 Memory Limit 到 1Gi | Alice | 修复决策 |
| 14:42 | 提交 Deployment 配置变更 | Alice | 修复执行 |
| 14:45 | 等待新 Pod 启动 | - | - |
| 14:50 | 部分 Pod 恢复，但流量仍高导致再次 OOM | Alice | 修复失败 |
| 14:52 | 决定进一步增加到 2Gi，并手动扩容到 10 副本 | Alice + Bob | 修复调整 |
| 14:55 | Pod 稳定，Endpoints 恢复 | - | - |
| 15:00 | 外部探测恢复正常 | - | 恢复 |
| 15:05 | 确认服务完全恢复，解除告警 | Alice | 事件结束 |

## 根因分析 (Root Cause Analysis)

### 5 Whys 分析

1. **为什么服务不可用？**
   - 因为所有 Pod 处于 CrashLoopBackOff 状态

2. **为什么 Pod Crash？**
   - 因为 Container 被 OOMKilled

3. **为什么 Container 被 OOMKilled？**
   - 因为内存使用超过了 512Mi 的 Limit

4. **为什么内存使用会超过 512Mi？**
   - 因为流量突增导致并发请求增加，内存消耗上升

5. **为什么流量突增时内存 Limit 不够用？**
   - 因为初始配置时使用的是低流量时的数据，未进行容量规划和压力测试

### FTA Fault Path Analysis

```
顶事件: Service 不可用
    ↓
中间事件1: Backend Pod 不可用
    ↓
中间事件2: Pod Crash
    ↓
底事件: E6 (OOMKilled)
    ↓
根本原因:
  1. Memory Limit 配置不合理 (512Mi)
  2. 缺少基于流量的自动扩容 (HPA)
  3. 未进行容量规划和压力测试
    ↓
触发因素:
  - 营销活动导致流量突增 3 倍
```

### 贡献因素 (Contributing Factors)

| 因素 | 分类 | 影响程度 |
|------|------|---------|
| Memory Limit 设置过低 | 配置问题 | 🔴 高 |
| 缺少 HPA | 架构问题 | 🔴 高 |
| 未进行压力测试 | 流程问题 | 🟡 中 |
| 缺少流量预警 | 监控问题 | 🟡 中 |
| 修复过程中的等待时间 | 流程问题 | 🟢 低 |

## 改进措施 (Action Items)

### 立即行动 (Immediate Actions - 已完成)

- [x] 将 `my-service` Memory Limit 增加到 2Gi
- [x] 手动扩容到 10 副本应对当前流量
- [x] 监控内存使用情况，确保稳定

### 短期行动 (Short-term - 1-2 周)

| ID | 行动项 | 负责人 | 截止日期 | FTA 关联 |
|----|--------|--------|---------|---------|
| AI-1 | 为 `my-service` 配置 HPA (基于 CPU 和 Memory) | Alice | 2024-01-22 | 预防 E6 |
| AI-2 | 对所有生产服务进行 Memory Limit 审计，识别类似风险 | Bob | 2024-01-25 | 预防 E6 |
| AI-3 | 在 Runbook 中添加 "流量突增" 场景的快速扩容步骤 | Alice | 2024-01-20 | 改进 Runbook |
| AI-4 | 配置 "内存使用率 > 80% 持续 5 分钟" 的预