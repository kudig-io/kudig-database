---
title: 值班手册与告警响应规范
summary: 值班手册与告警响应规范：值班（On-Call）是生产运维的第一道防线。一个结构化的值班手册能够确保告警被及时响应、问题被正确分级、升级路径清晰可控。本文档为远程顾问提供标准化值班框架，以便指导客户建立可执行的值班体系。
category: 生产运维
tags:
- domain-11
- on-call
- 告警
- 值班
- SRE
- 升级
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




# 值班手册与告警响应规范

## 概述

值班（On-Call）是生产运维的第一道防线。一个结构化的值班手册能够确保告警被及时响应、问题被正确分级、升级路径清晰可控。本文档为远程顾问提供标准化值班框架，以便指导客户建立可执行的值班体系。

## 值班生命周期

### 值班前准备

- [ ] 确认值班交接文档已阅读，了解上一班次遗留问题
- [ ] 检查告警渠道畅通（钉钉/企业微信/短信/电话）
- [ ] 验证 VPN / 堡垒机 / 集群访问凭证未过期
- [ ] 确认 Runbook 和应急联系人名单可访问
- [ ] 通知团队成员当前值班责任人

### 值班中响应

1. **接收告警**：通过统一告警平台（[[Prometheus]] Alertmanager / PagerDuty）接收通知
2. **确认告警**：在告警平台点击「确认」，停止重复通知
3. **初步分级**：根据影响范围判定 P0 / P1 / P2
4. **执行响应**：按分级时间要求启动排查
5. **记录过程**：在值班日志中记录关键时间点和操作

### 值班后交接

- 遗留问题清单与当前状态
- 未关闭告警的跟进计划
- 已执行变更的影响说明
- 建议优化的监控项或阈值

## 告警分级标准

| 级别 | 定义 | 响应时间 | 典型场景 |
|---|---|---|---|
| P0 | 核心业务中断 | 5 分钟内 | 集群全部不可用、支付链路中断、数据丢失 |
| P1 | 功能受损 | 15 分钟内 | 部分 Pod 持续重启、关键 API 延迟 > 5s |
| P2 | 轻微影响 | 1 小时内 | 非核心服务异常、资源利用率预警、单节点故障 |

> P0 告警需在确认后 1 分钟内启动语音电话通知。

## 升级路径

```
一线值班工程师
    ↓ （P0 或 15 分钟未定位）
二线技术专家
    ↓ （P0 或 30 分钟未恢复）
主管 / 团队负责人
    ↓ （涉及客户业务中断）
客户通知（通过客户成功或项目经理）
```

## 阿里云 ACK 值班特殊注意

### 节点池告警

- 节点池扩容失败：检查弹性伸缩配置、实例规格库存、VPC 网段余量
- 节点池缩容异常：确认 Pod 驱逐策略、DaemonSet 容忍度
- 节点 NotReady：参考 [[node-notready]] 排查流程

### 费用告警

- 按量付费节点突发账单：检查节点池自动伸缩触发条件
- 存储费用激增：排查是否有异常日志写入或快照策略配置错误
- 带宽费用告警：分析 Ingress 流量模式，识别异常请求来源

### 配额告警

- ACK 集群数量配额：提前评估业务增长，申请配额提升
- SLB 实例配额：服务暴露数量受限于账户配额
- EIP 配额：检查暴露到公网的服务数量

## 远程顾问指导要点

远程顾问无法直接接收客户告警，但可以通过以下方式帮助现场值班工程师定位问题：

1. **结构化提问模板**：
   - 告警内容是什么？（指标、阈值、持续时间）
   - 影响范围有多大？（涉及哪些命名空间、服务、节点）
   - 最近是否有变更？（发布、配置修改、扩缩容）
   - 日志中有什么异常？（ERROR、Panic、OOMKilled）

2. **输出审核**：要求工程师提供 `kubectl describe`、`kubectl logs`、`kubectl top` 的完整输出，逐项分析

3. **决策树引导**：按「基础设施 → 网络 → 存储 → 应用」的优先级逐层排查，避免盲目猜测

4. **升级判断**：当排查超过 10 分钟无进展时，建议启动升级流程，引入更多专家

> 远程顾问的核心价值在于提供系统性排查思路，帮助值班工程师避免在压力下做出错误决策。

## 值班轮换管理

### 轮换模式对比

| 模式 | 周期 | 适用团队 | 优点 | 缺点 |
|------|------|----------|------|------|
| 主/备制 | 1 周 | 3-5 人 | 责任清晰 | 主值班压力大 |
| Follow-the-Sun | 8h 三班 | 跨时区 | 无夜间打扰 | 交接复杂 |
| 分层制 | 1 周 | > 10 人 | 专业分工 | 协调成本高 |
| 自愿+轮转 | 2 周 | 小团队 | 灵活性高 | 可能不均 |

### 轮换排班配置（PagerDuty/OpsGenie）

```yaml
# 排班策略示例
rotation:
  name: k8s-platform-oncall
  type: weekly
  start: monday 09:00 Asia/Shanghai
  layers:
    - name: primary
      members:
        - engineer-a
        - engineer-b
        - engineer-c
      rotation_interval: 1w
    - name: secondary
      members:
        - senior-a
        - senior-b
      rotation_interval: 2w
  escalation_policy:
    - level: 1
      target: primary
      timeout: 5m
    - level: 2
      target: secondary
      timeout: 10m
    - level: 3
      target: engineering-manager
      timeout: 15m
  override_rules:
    - max_consecutive_weeks: 2
    - min_rest_between: 48h
    - no_oncall_during_pto: true
```

### 值班补偿与健康管理

| 项目 | 标准 | 说明 |
|------|------|------|
| 夜间响应补偿 | 调休 0.5-1 天/次 | P0 夜间响应后 |
| 连续值班上限 | 2 周 | 避免疲劳 |
| 值班间隔 | ≥ 48h | 两次值班间休息 |
| 事件后休息 | P0 后可休半天 | 高压恢复 |
| 月度回顾 | 必须 | 值班质量改进 |

## 告警分诊（Triage）

### 快速分诊命令集

```bash
# 🟢 第一步：全景扫描（30 秒内完成）
kubectl get events -A --sort-by='.lastTimestamp' --field-selector type=Warning | tail -20
kubectl get pods -A --field-selector status.phase!=Running,status.phase!=Succeeded
kubectl get nodes
kubectl top nodes 2>/dev/null || echo "metrics-server 不可用"

# 🟢 第二步：定位影响范围
# 单服务问题
kubectl get pods -n <ns> -l app=<service> -o wide
kubectl logs -n <ns> -l app=<service> --tail=50 --all-containers
kubectl describe svc <service> -n <ns> | grep -A5 Endpoints

# 节点问题
kubectl describe node <node> | grep -A20 "Conditions"
kubectl get pods -A --field-selector spec.nodeName=<node>

# 集群级问题
kubectl -n kube-system get pods
kubectl get componentstatuses 2>/dev/null
kubectl get apiservice | grep -v True
```

### 分诊决策树

```
告警触发
├── 是否为误报？
│   ├── 是 → 记录 + 调整阈值/静默
│   └── 否 → 继续
├── 影响范围？
│   ├── 单 Pod → 检查重启/日志/资源
│   ├── 单服务 → 检查 Endpoints/依赖
│   ├── 单节点 → 检查 kubelet/磁盘/网络
│   └── 集群级 → 检查控制平面/etcd/网络
├── 最近有变更？
│   ├── 是 → 优先回滚验证
│   └── 否 → 按层排查
└── 能否 5 分钟内恢复？
    ├── 是 → 执行修复
    └── 否 → 升级 + 启动事件响应
```

## 常见告警处理 Runbook

### Pod CrashLoopBackOff

```bash
# 🟢 诊断
kubectl logs <pod> -n <ns> --previous --tail=100
kubectl describe pod <pod> -n <ns> | grep -A10 "Events"
kubectl get events -n <ns> --field-selector involvedObject.name=<pod>

# 常见原因与修复:
# 1. OOMKilled → 🟡 增加 memory limits
kubectl patch deploy <name> -n <ns> -p '{"spec":{"template":{"spec":{"containers":[{"name":"<c>","resources":{"limits":{"memory":"1Gi"}}}]}}}}'

# 2. 配置错误 → 🟡 修复 ConfigMap/Secret
kubectl get cm <config> -n <ns> -o yaml

# 3. 依赖不可用 → 检查下游服务
kubectl exec -it <pod> -n <ns> -- nslookup <dependency-svc>

# 4. 版本 Bug → 🔴 回滚
kubectl rollout undo deploy/<name> -n <ns>
kubectl rollout status deploy/<name> -n <ns>
```

### 节点 NotReady

```bash
# 🟢 诊断
kubectl describe node <node> | grep -A5 "Conditions"
kubectl get events --field-selector involvedObject.name=<node>

# SSH 到节点后:
systemctl status kubelet
journalctl -u kubelet --since "10 min ago" --no-pager | tail -50
df -h /  # 磁盘满？
free -h  # 内存耗尽？
dmesg | tail -20  # 内核错误？

# 🟡 修复（kubelet 异常）
systemctl restart kubelet

# 🔴 修复（节点不可恢复）— 驱逐并替换
kubectl drain <node> --ignore-daemonsets --delete-emptydir-data --force
# 云控制台替换节点 / 节点池自动修复
```

### 高延迟/高错误率

```bash
# 🟢 诊断
# 检查 Pod 资源使用
kubectl top pods -n <ns> -l app=<service> --sort-by=cpu

# 检查 HPA 状态
kubectl get hpa -n <ns>
kubectl describe hpa <name> -n <ns>

# 检查连接池/线程池（应用日志）
kubectl logs -n <ns> -l app=<service> --tail=100 | grep -i "timeout\|connection\|pool"

# 🟡 紧急扩容
kubectl scale deploy/<service> -n <ns> --replicas=<N>

# 检查下游依赖
kubectl exec -it <pod> -n <ns> -- curl -s -o /dev/null -w '%{time_total}' http://<dependency>/health
```

## 沟通模板

### P0 事件初始通知

```markdown
🚨 [P0] <服务名> 不可用

⏰ 时间: 2026-07-21 14:32 CST
👤 值班: @engineer-a
📊 影响: <具体影响，如「订单服务 5xx 错误率 > 50%」>
🔍 当前状态: 排查中
📋 事件频道: #inc-20260721-order-api

下一步:
- [ ] 确认影响范围
- [ ] 检查最近变更
- [ ] 15 分钟内更新进展
```

### 进展更新（每 15 分钟）

```markdown
📢 [P0] 进展更新 #2 — 14:47

🔍 根因: 已定位 — 14:30 上线的 v2.3.1 引入数据库连接泄漏
🛠 行动: 正在回滚到 v2.3.0
⏱ 预计恢复: 15:00
👥 参与: @engineer-a @dba-b

下一步:
- [x] 定位根因
- [ ] 执行回滚
- [ ] 验证恢复
- [ ] 确认无数据影响
```

### 事件关闭通知

```markdown
✅ [P0] 已恢复 — <服务名>

⏰ 恢复时间: 15:03 CST
📊 总影响时长: 31 分钟
🔍 根因: v2.3.1 数据库连接泄漏导致连接池耗尽
🛠 修复: 回滚至 v2.3.0
📋 复盘: 将于 48h 内完成，负责人 @engineer-a

改进项:
- [ ] CI 添加连接池泄漏检测
- [ ] 金丝雀阶段增加 DB 连接数监控
- [ ] 告警阈值优化（连接池 > 80% 提前告警）
```

## 值班质量度量

### 核心 KPI

| 指标 | 目标 | 计算方式 | 改进方向 |
|------|------|----------|----------|
| MTTA（平均确认时间） | < 5 min | 告警触发 → 确认 | 通知渠道优化 |
| MTTR（平均恢复时间） | < 30 min | 确认 → 恢复 | Runbook 完善 |
| 告警准确率 | > 90% | 真实告警/总告警 | 阈值调优 |
| 升级率 | < 20% | 升级次数/总事件 | 一线能力建设 |
| 值班打扰次数 | < 3 次/晚 | 夜间非紧急通知 | 告警分级 |
| 事件复盘完成率 | 100% | P0/P1 必须复盘 | 流程强制 |

### 告警质量审查（月度）

```bash
# 统计告警数据（Alertmanager API）
# 按告警名分组统计
curl -s http://alertmanager:9093/api/v2/alerts | jq -r '.[].labels.alertname' | sort | uniq -c | sort -rn | head -20

# 识别噪音告警（频繁触发但无需处理）
# 标准: 同一告警 7 天内触发 > 10 次且无需操作 → 调整或静默

# 识别缺失告警（有事件但无告警）
# 对比事件日志与告警记录，找出未覆盖的故障模式
```

## 告警自动化

### 自动修复（Auto-Remediation）

```yaml
# 自动重启 CrashLoop Pod（谨慎使用）
apiVersion: batch/v1
kind: CronJob
metadata:
  name: auto-remediation
  namespace: kube-system
spec:
  schedule: "*/5 * * * *"
  jobTemplate:
    spec:
      template:
        spec:
          serviceAccountName: auto-remediator
          containers:
            - name: remediate
              image: bitnami/kubectl:latest
              command:
                - /bin/sh
                - -c
                - |
                  # 自动删除 Evicted Pod
                  kubectl get pods -A --field-selector status.phase=Failed \
                    -o json | jq -r '.items[] | select(.status.reason=="Evicted") |
                    "\(.metadata.namespace) \(.metadata.name)"' | \
                    while read ns name; do
                      kubectl delete pod $name -n $ns
                      echo "Deleted evicted pod: $ns/$name"
                    done
                  
                  # 自动清理 Completed Job（超过 1h）
                  kubectl get jobs -A -o json | jq -r '.items[] |
                    select(.status.completionTime != null) |
                    select((now - (.status.completionTime | fromdate)) > 3600) |
                    "\(.metadata.namespace) \(.metadata.name)"' | \
                    while read ns name; do
                      kubectl delete job $name -n $ns
                    done
          restartPolicy: OnFailure
```

### Alertmanager 路由配置

```yaml
# Alertmanager 路由（分级通知）
route:
  receiver: default
  group_by: [alertname, namespace]
  group_wait: 30s
  group_interval: 5m
  repeat_interval: 4h
  routes:
    # P0: 电话 + 短信 + IM
    - match:
        severity: critical
      receiver: pagerduty-critical
      group_wait: 10s
      repeat_interval: 15m
      continue: true
    # P1: IM + 邮件
    - match:
        severity: warning
      receiver: slack-warning
      repeat_interval: 2h
    # 开发环境: 仅 IM
    - match:
        environment: dev
      receiver: slack-dev
      repeat_interval: 24h

receivers:
  - name: pagerduty-critical
    pagerduty_configs:
      - service_key: <key>
        severity: critical
  - name: slack-warning
    slack_configs:
      - channel: '#alerts-warning'
        send_resolved: true
  - name: slack-dev
    slack_configs:
      - channel: '#dev-alerts'
        send_resolved: false
```

## 值班工具链

| 工具 | 用途 | 替代方案 |
|------|------|----------|
| PagerDuty | 告警路由/升级 | OpsGenie, 自研 |
| Slack/钉钉 | 实时沟通 | 企业微信, Teams |
| Statuspage | 状态页 | Instatus, 自研 |
| Grafana | 指标看板 | Kibana, Datadog |
| k9s | 终端 K8s 管理 | Lens, kubectl |
| Runbook.md | 操作手册 | Notion, Confluence |
| Incident.io | 事件管理 | Rootly, FireHydrant |

## 值班培训与演练

### 新值班人员上手清单

- [ ] 完成 K8s 基础操作培训（kubectl 常用命令）
- [ ] 熟悉告警平台操作（确认/静默/升级）
- [ ] 阅读 Top 10 常见告警 Runbook
- [ ] 完成 1 次影子值班（跟随资深工程师）
- [ ] 完成 1 次独立值班（资深工程师待命）
- [ ] 通过值班能力评估（模拟告警场景）

### 季度 GameDay 演练

```markdown
## 值班 GameDay 设计

### 场景 1: 核心服务不可用
- 注入: 删除核心 Deployment 的所有 Pod + 设置 replicas=0
- 预期: 5 分钟内发现，15 分钟内恢复
- 评估: MTTA、MTTR、沟通质量

### 场景 2: 数据库连接耗尽
- 注入: 限制 DB 最大连接数为 5
- 预期: 告警触发，定位连接池问题
- 评估: 诊断准确性、修复方案

### 场景 3: 节点批量故障
- 注入: 同时 cordon 3 个节点
- 预期: Pod 重调度，服务不中断
- 评估: PDB 有效性、扩容响应
```

## 相关链接

- [[13-生产运维/07-运维手册/01-production-sre-daily-ops.md|production-sre-daily-ops]] — 日常巡检与值班手册
- [[13-生产运维/03-事件响应/04-incident-response-template.md|incident-response-template]] — 事故响应模板
- [[13-生产运维/07-运维手册/02-change-management-guide.md|change-management-guide]] — 变更管理指南
- [[node-notready]] — 节点异常排查
- [[09-可观测性/05-告警/index.md|告警管理]] — 告警体系设计
- [[12-可靠性/05-事后复盘/index.md|事后复盘]] — 复盘文化

## Related

- [[visibility-public|#visibility/public Hub]] — tag hub


<!-- risk-assessed -->
