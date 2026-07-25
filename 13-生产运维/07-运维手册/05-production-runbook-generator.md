---
title: 生产 Runbook 编写规范与高频操作清单
summary: 生产 Runbook 编写规范与高频操作清单：Runbook（操作手册）是值班工程师在高压环境下的「救命稻草」。一份好的 Runbook 能够显著降低
  MTTR，避免人为失误。本文档提供编写规范和高频操作清单，帮助远程顾问指导客户建立可执行、可维护的操作文档。
category: 生产运维
tags:
- domain-11
- runbook
- 操作手册
- SRE
- 运维
- ACK
- visibility/public
tier: supporting
sources:
- KUDIG Gap Analysis 2026-05-21
created: 2026-05-21
updated: 2026-05-21
last_updated: 2026-05-21
status: reviewed
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 生产 Runbook 编写规范与高频操作清单

## 概述

Runbook（操作手册）是值班工程师在高压环境下的「救命稻草」。一份好的 Runbook 能够显著降低 MTTR，避免人为失误。本文档提供编写规范和高频操作清单，帮助远程顾问指导客户建立可执行、可维护的操作文档。

## Runbook 编写规范

### 一页一操作

每个 Runbook 只描述一个场景（如「重启 Pod」、「排空节点」），避免混排多个不相关操作。

### 命令可复制

所有命令必须可直接复制执行：

- ❌ `kubectl delete pod <pod-name>`
- ✅ `kubectl delete pod my-pod -n my-ns`

变量使用显式占位符：

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl delete`：删除资源（可由声明式清单重建）

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
NAMESPACE="default"
POD_NAME="my-app-xxx"
kubectl delete pod "$POD_NAME" -n "$NAMESPACE"
```
### 含验证步骤

每个操作后必须有验证命令和预期结果：

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl get pod "$POD_NAME" -n "$NAMESPACE" -o jsonpath='{.status.phase}'
# 预期输出：Running
```
### 前置条件与风险提示

| 要素 | 说明 |
|---|---|
| 前置条件 | 执行前需确认的状态 |
| 影响范围 | 会影响哪些服务或用户 |
| 回滚步骤 | 操作失败如何恢复 |
| 审批要求 | 是否需双人复核 |

## 高频操作清单

### Pod 重启

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl rollout undo/restart`：触发滚动变更，影响副本

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
kubectl rollout restart deployment/my-app -n my-ns
kubectl rollout status deployment/my-app -n my-ns
```
### 节点排空

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `kubectl drain`：驱逐节点所有 Pod，业务流量受影响

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
kubectl drain node-01 --ignore-daemonsets --delete-emptydir-data
kubectl get nodes  # 验证 SchedulingDisabled
kubectl uncordon node-01  # 恢复调度
```
### 证书更新

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `systemctl stop/restart`：停止/重启系统服务，影响节点上所有容器

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
kubeadm certs check-expiration
kubeadm certs renew all
systemctl restart kubelet
```
### 配置回滚

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl rollout undo/restart`：触发滚动变更，影响副本

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl rollout history deployment/my-app -n my-ns
kubectl rollout undo deployment/my-app -n my-ns
# 或回滚到指定版本
kubectl rollout undo deployment/my-app --to-revision=3 -n my-ns
```
## Runbook 维护机制

### 定期演练

- 每季度至少演练一次关键 Runbook
- 记录实际耗时与文档预期的差异
- 演练后更新命令和参数

### 版本控制

- Runbook 纳入 Git 管理，变更需 PR 审核
- 发布版本号，值班手册引用固定版本

### 过期检查

- 每月检查命令是否因集群升级失效
- 标记超过 6 个月未更新的为「待审阅」
- 删除已废弃组件的相关 Runbook

## 阿里云 ACK 常用操作

### 节点池扩容

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
ack-node-pool scale --cluster-id $CLUSTER_ID \
  --nodepool-id $NP_ID --count 5
kubectl get nodes -l alibabacloud.com/nodepool-id=$NP_ID
```
### 组件升级

- 在 ACK 控制台查看待升级组件列表
- 先在测试集群验证兼容性
- 升级后参考 [[22-概念/08-可靠性与运维/cluster-upgrade-paths.md|cluster-upgrade-paths]] 确认版本一致

### 集群备份

- etcd 备份：ACK 托管版自动备份，专有版需配置定时备份
- 应用配置备份：使用 Velero 定期备份命名空间
- 每季度执行一次恢复演练

## 远程顾问指导要点

远程顾问审核客户 Runbook 时，重点关注以下维度：

1. **可执行性**：要求工程师按 Runbook 实操一次，观察是否有歧义或遗漏
2. **完整性**：是否每个操作都有前置条件、执行步骤、验证步骤、回滚步骤
3. **时效性**：命令是否与当前集群版本匹配，参数是否过时
4. **权限合理性**：操作权限是否最小化，是否存在过度授权
5. **命名规范**：文件名是否按「系统_场景_操作」格式统一命名

> 建议为客户建立 Runbook 评分卡，从 5 个维度打分，持续优化。

---

## Runbook 标准模板

### 完整结构

```markdown
# [RB-XXX] 场景名称

> **风险等级**: 🟢/🟡/🔴 | **影响范围**: xxx | **审批要求**: 单人/双人
> **最后演练**: 2026-XX-XX | **负责人**: @xxx

## 触发条件

当以下告警触发时执行本 Runbook：
- `AlertName` — 描述

## 前置条件

- [ ] 确认当前集群: `kubectl config current-context`
- [ ] 确认命名空间: `kubectl config view --minify -o jsonpath='{.contexts[0].context.namespace}'`
- [ ] 确认 RBAC 权限: `kubectl auth can-i <verb> <resource> -n <ns>`
- [ ] 确认变更窗口: 当前是否在允许的变更时间内

## 执行步骤

### Step 1: 信息收集
```bash
# 🟢 低风险
kubectl get pods -n $NAMESPACE -l app=$APP
kubectl logs -n $NAMESPACE -l app=$APP --tail=100
```

### Step 2: 执行操作
```bash
# 🟡 中风险
kubectl rollout restart deployment/$APP -n $NAMESPACE
```

### Step 3: 验证
```bash
# 🟢 低风险
kubectl rollout status deployment/$APP -n $NAMESPACE --timeout=120s
kubectl get pods -n $NAMESPACE -l app=$APP
# 预期: 所有 Pod Running, READY 1/1
```

## 回滚步骤

```bash
# 🟡 中风险
kubectl rollout undo deployment/$APP -n $NAMESPACE
```

## 升级路径

如果 15 分钟内未解决：
1. 通知 Tech Lead
2. 开启事故响应流程
3. 参考 [[13-生产运维/03-事件响应/04-incident-response-template.md|事故响应模板]]
```

---

## 高频场景 Runbook 库

### RB-001: Pod OOMKilled 处理

```bash
# 🟢 信息收集
NAMESPACE="production"
POD_NAME="my-app-xxx"

# 1. 确认 OOMKilled
kubectl get pod $POD_NAME -n $NAMESPACE -o jsonpath='{.status.containerStatuses[0].lastState.terminated.reason}'
# 预期: OOMKilled

# 2. 查看当前内存使用
kubectl top pod $POD_NAME -n $NAMESPACE

# 3. 查看历史内存趋势（Prometheus）
# container_memory_working_set_bytes{pod="$POD_NAME"} / container_spec_memory_limit_bytes{pod="$POD_NAME"}

# 🟡 4. 临时增加内存限制
kubectl patch deployment my-app -n $NAMESPACE --type='json' \
  -p='[{"op":"replace","path":"/spec/template/spec/containers/0/resources/limits/memory","value":"2Gi"}]'

# 🟢 5. 验证
kubectl rollout status deployment/my-app -n $NAMESPACE
kubectl top pod -n $NAMESPACE -l app=my-app
```

### RB-002: PVC 容量不足

```bash
# 🟢 信息收集
NAMESPACE="production"
PVC_NAME="data-postgresql-0"

# 1. 查看 PVC 使用率
kubectl get pvc $PVC_NAME -n $NAMESPACE
kubectl exec -n $NAMESPACE postgresql-0 -- df -h /data

# 🟡 2. 扩容 PVC（需 StorageClass 支持 allowVolumeExpansion）
kubectl patch pvc $PVC_NAME -n $NAMESPACE --type='json' \
  -p='[{"op":"replace","path":"/spec/resources/requests/storage","value":"200Gi"}]'

# 🟢 3. 验证扩容
kubectl get pvc $PVC_NAME -n $NAMESPACE -o jsonpath='{.status.capacity.storage}'
kubectl exec -n $NAMESPACE postgresql-0 -- df -h /data
```

### RB-003: 节点磁盘压力 (DiskPressure)

```bash
# 🟢 信息收集
NODE_NAME="worker-01"

# 1. 确认节点状态
kubectl describe node $NODE_NAME | grep -A 5 Conditions

# 2. 查看磁盘使用
kubectl debug node/$NODE_NAME -it --image=busybox -- df -h

# 3. 清理未使用镜像
kubectl debug node/$NODE_NAME -it --image=busybox -- \
  chroot /host crictl rmi --prune

# 🟡 4. 驱逐非必要 Pod
kubectl get pods -A --field-selector spec.nodeName=$NODE_NAME | grep -v kube-system

# 🟢 5. 验证恢复
kubectl get node $NODE_NAME -o jsonpath='{.status.conditions[?(@.type=="DiskPressure")].status}'
# 预期: False
```

### RB-004: Ingress 502/504 处理

```bash
# 🟢 信息收集
NAMESPACE="ingress-nginx"

# 1. 检查 Ingress Controller Pod
kubectl get pods -n $NAMESPACE -l app.kubernetes.io/name=ingress-nginx

# 2. 查看后端服务状态
kubectl get endpoints -n production my-service

# 3. 检查后端 Pod 健康检查
kubectl describe pod -n production -l app=my-service | grep -A 10 "Conditions"

# 4. 测试后端连接
kubectl exec -n $NAMESPACE deploy/ingress-nginx-controller -- \
  curl -s -o /dev/null -w "%{http_code}" http://my-service.production.svc:8080/health

# 🟡 5. 重启后端 Pod（如果健康检查失败）
kubectl rollout restart deployment/my-service -n production

# 🟢 6. 验证
kubectl rollout status deployment/my-service -n production
kubectl exec -n $NAMESPACE deploy/ingress-nginx-controller -- \
  curl -s -o /dev/null -w "%{http_code}" http://my-service.production.svc:8080/health
```

---

## 告警与 Runbook 集成

### PrometheusRule 中嵌入 Runbook 链接

```yaml
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: app-alerts
  namespace: monitoring
spec:
  groups:
    - name: app.rules
      rules:
        - alert: PodCrashLooping
          expr: |
            rate(kube_pod_container_status_restarts_total{
              namespace="production"
            }[15m]) > 0
          for: 5m
          labels:
            severity: warning
          annotations:
            summary: "Pod {{ $labels.namespace }}/{{ $labels.pod }} 频繁重启"
            description: "过去 15 分钟内重启 {{ $value | printf \"%.0f\" }} 次"
            runbook_url: "https://wiki.internal/runbooks/RB-001-pod-crashloop"
            dashboard_url: "https://grafana.internal/d/pod-overview?var-pod={{ $labels.pod }}"

        - alert: PVCSpaceCritical
          expr: |
            kubelet_volume_stats_used_bytes / kubelet_volume_stats_capacity_bytes > 0.9
          for: 10m
          labels:
            severity: critical
          annotations:
            summary: "PVC {{ $labels.namespace }}/{{ $labels.persistentvolumeclaim }} 容量 > 90%"
            runbook_url: "https://wiki.internal/runbooks/RB-002-pvc-capacity"
```

### Alertmanager 路由配置

```yaml
# 告警模板包含 Runbook 链接
templates:
  - '/etc/alertmanager/templates/*.tmpl'

route:
  receiver: default
  routes:
    - match:
        severity: critical
      receiver: pagerduty-critical
      continue: true
    - match:
        severity: critical
      receiver: slack-critical

receivers:
  - name: pagerduty-critical
    pagerduty_configs:
      - routing_key: '<key>'
        description: '{{ .CommonAnnotations.summary }}'
        details:
          runbook: '{{ .CommonAnnotations.runbook_url }}'
          dashboard: '{{ .CommonAnnotations.dashboard_url }}'
  - name: slack-critical
    slack_configs:
      - channel: '#oncall-critical'
        title: '🚨 {{ .CommonLabels.alertname }}'
        text: |
          {{ .CommonAnnotations.summary }}
          *Runbook*: {{ .CommonAnnotations.runbook_url }}
          *Dashboard*: {{ .CommonAnnotations.dashboard_url }}
```

---

## Runbook 质量评分卡

| 维度 | 权重 | 评分标准 (1-5) | 说明 |
|------|------|--------------|------|
| 可执行性 | 30% | 5=新人可直接执行 | 无歧义、无遗漏 |
| 完整性 | 25% | 5=前置+执行+验证+回滚 | 四段式完整 |
| 时效性 | 20% | 5=本月内验证 | 命令与当前版本匹配 |
| 可发现性 | 15% | 5=告警直接链接 | 告警→Runbook 无缝 |
| 可维护性 | 10% | 5=Git 管理+定期审阅 | 有 Owner、有版本 |

### 评分计算

```
总分 = 可执行性×0.3 + 完整性×0.25 + 时效性×0.2 + 可发现性×0.15 + 可维护性×0.1

评级:
  4.5-5.0: ⭐ 优秀 — 可作为标杆
  3.5-4.4: ✅ 良好 — 小修即可
  2.5-3.4: ⚠️ 待改进 — 需安排优化
  < 2.5:   🔴 不合格 — 立即重写
```

---

## 自动化 Runbook 生成

### 从告警规则自动生成 Runbook 骨架

```bash
#!/bin/bash
# 🟢 generate-runbook-skeleton.sh
# 从 PrometheusRule 提取告警，生成 Runbook 骨架

RULES_FILE="${1:?Usage: $0 <prometheus-rules.yaml>}"
OUTPUT_DIR="${2:-./runbooks}"
mkdir -p "$OUTPUT_DIR"

# 提取告警名称和描述
yq '.spec.groups[].rules[] | select(.alert) | [.alert, .annotations.summary // ""] | @tsv' \
  "$RULES_FILE" | while IFS=$'\t' read -r alert summary; do

  FILENAME="RB-$(echo $alert | tr '[:upper:]' '[:lower:]' | tr '_' '-').md"
  cat > "$OUTPUT_DIR/$FILENAME" << EOF
# [RB-AUTO] $alert

> **风险等级**: 待评估 | **影响范围**: 待确认 | **审批要求**: 待定
> **最后演练**: 未演练 | **负责人**: @unassigned

## 触发条件

告警: \`$alert\`
描述: $summary

## 前置条件

- [ ] 确认集群与命名空间
- [ ] 确认 RBAC 权限
- [ ] 确认变更窗口

## 执行步骤

### Step 1: 信息收集
\`\`\`bash
# TODO: 添加诊断命令
\`\`\`

### Step 2: 处置操作
\`\`\`bash
# TODO: 添加修复命令
\`\`\`

### Step 3: 验证
\`\`\`bash
# TODO: 添加验证命令
\`\`\`

## 回滚步骤

\`\`\`bash
# TODO: 添加回滚命令
\`\`\`

## 升级路径

如果 15 分钟内未解决，通知 Tech Lead 并启动事故响应。
EOF

  echo "✅ 生成: $OUTPUT_DIR/$FILENAME"
done
```

---

## Runbook 演练机制

### GameDay 演练计划

| 频率 | 演练内容 | 参与者 | 输出 |
|------|----------|--------|------|
| 每月 | 1 个关键 Runbook 实操 | 值班团队 | 演练报告 + 更新 |
| 每季度 | 全量 Runbook 审阅 | SRE + 开发 | 评分卡 + 改进项 |
| 每半年 | 混沌工程 + Runbook 验证 | 全团队 | 新 Runbook + 修复 |

### 演练记录模板

```markdown
## Runbook 演练记录

- **日期**: 2026-XX-XX
- **Runbook**: RB-XXX
- **演练人**: @xxx
- **实际耗时**: XX 分钟（文档预期: XX 分钟）
- **发现问题**:
  1. Step 3 命令已过时（kubectl 版本变更）
  2. 缺少回滚步骤
- **修复措施**:
  1. 更新命令为 v1.33 语法
  2. 补充回滚步骤
- **评分**: 可执行性 4 / 完整性 3 / 时效性 2 / 可发现性 4 / 可维护性 3
```

## 相关链接

- [[13-生产运维/07-运维手册/01-production-sre-daily-ops.md|production-sre-daily-ops]] — 日常巡检与值班手册
- [[13-生产运维/03-事件响应/03-on-call-playbook.md|on-call-playbook]] — 值班手册与告警响应规范
- [[13-生产运维/03-事件响应/04-incident-response-template.md|incident-response-template]] — 事故响应模板
- [[22-概念/08-可靠性与运维/cluster-upgrade-paths.md|cluster-upgrade-paths]] — 集群升级路径与版本兼容性

## Related

- [[visibility-public|#visibility/public Hub]] — tag hub


<!-- risk-assessed -->
