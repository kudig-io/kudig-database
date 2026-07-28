---
title: 生产环境日常巡检与值班手册
summary: 生产环境日常巡检与值班手册：生产环境的稳定性直接决定业务的可用性。作为远程顾问，我们需要建立标准化的巡检清单和值班响应机制，帮助客户在生产环境中主动发现问题、快速响应异常。
category: 生产运维
tags:
- domain-11
- SRE
- 运维
- 巡检
- 值班
- 变更管理
- visibility/public
tier: core
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




# 生产环境日常巡检与值班手册

## 概述

生产环境的稳定性直接决定业务的可用性。作为远程顾问，我们需要建立标准化的巡检清单和值班响应机制，帮助客户在生产环境中主动发现问题、快速响应异常。

## 日常巡检清单

### 节点巡检

| 检查项 | 命令/方法 | 正常标准 |
|---|---|---|
| 节点状态 | `kubectl get nodes` | All Ready |
| 资源使用率 | `kubectl top nodes` | CPU < 80%，内存 < 85% |
| 磁盘空间 | `df -h` | 根分区 < 80% |
| 系统负载 | `uptime` | load < CPU 核数 × 2 |
| 节点条件 | `kubectl describe node` | 无 DiskPressure、MemoryPressure |

### Pod 巡检

| 检查项 | 命令/方法 | 正常标准 |
|---|---|---|
| Pod 状态分布 | `kubectl get pods -A` | 无 CrashLoopBackOff、Error |
| 重启计数 | `kubectl get pods -A -o wide` | Restart Count < 3 |
| 资源使用 | `kubectl top pods -A` | 无资源耗尽 |
| 事件告警 | `kubectl get events --sort-by='.lastTimestamp'` | 无频繁 Warning |

### 存储巡检

- PVC 使用率：通过 [[persistent-volume-claim]] 检查绑定状态
- StorageClass 状态：确认 provisioner 正常运行
- 快照策略：检查备份任务执行历史

### 网络巡检

- Service 端点健康：确认 Endpoints 与 Pod 数量一致
- Ingress 状态：检查 TLS 证书有效期
- CoreDNS 响应：测试集群内部 DNS 解析延迟

## 值班手册结构

### 告警响应分级

| 级别 | 响应时间 | 升级路径 | 典型场景 |
|---|---|---|---|
| P0 | 5分钟内 | 立即通知值班负责人 + 团队负责人 | 集群不可用、核心服务中断 |
| P1 | 15分钟内 | 通知值班负责人 | 部分服务降级、节点异常 |
| P2 | 30分钟内 | 记录并安排处理 | 非核心功能异常、资源预警 |

### 告警响应流程

1. **确认告警**：核实告警真实性，排除误报
2. **初步判断**：根据现象定位问题域（节点/Pod/网络/存储）
3. **信息收集**：收集日志、指标、事件，联系相关团队
4. **决策执行**：根据预案执行回滚或修复操作
5. **事后复盘**：记录根因，更新 [[12-可靠性/05-事后复盘/02-postmortem-culture-guide.md|postmortem]] 和 [[35-元数据/metadata/KUDIG Knowledge Base Architecture.md|knowledge-base]]

## 变更管理流程

### 变更窗口

| 变更类型 | 窗口要求 | 审批层级 |
|---|---|---|
| 配置变更 | 任意时间 | 团队负责人 |
| 版本升级 | 低峰期 | 架构师 + 运维负责人 |
| 扩缩容 | 任意时间 | 自动化审批 |
| 架构调整 | 预定维护窗口 | 技术总监 |

### 灰度验证

- 金丝雀发布：先灰度 1% 流量，观察 15 分钟
- 蓝绿部署：并行部署，切换前做健康检查
- 回滚预案：确保回滚命令已验证，回滚时间 < 5 分钟

## 阿里云 ACK 特定巡检

### 节点池巡检

- 节点池状态：`ack-node-pool` 检查扩容/缩容状态
- 组件版本：确认 kube-proxy、CoreDNS 版本与 ACK 推荐版本一致
- 费用监控：检查按量付费节点资源利用率，识别闲置节点

### 组件升级检查

- 阿里云组件升级公告订阅
- 升级前在测试集群验证兼容性
- 升级后检查 [[22-概念/08-可靠性与运维/cluster-upgrade-paths.md|cluster-upgrade-paths]] 确认版本匹配

## 自动化巡检脚本

### 每日巡检脚本（CronJob）

```yaml
apiVersion: batch/v1
kind: CronJob
metadata:
  name: daily-cluster-inspection
  namespace: kube-system
spec:
  schedule: "0 8 * * *"  # 每天早 8 点
  jobTemplate:
    spec:
      template:
        spec:
          serviceAccountName: cluster-inspector
          containers:
            - name: inspect
              image: bitnami/kubectl:latest
              command:
                - /bin/sh
                - -c
                - |
                  echo "=== 集群巡检 $(date '+%Y-%m-%d %H:%M') ==="
                  
                  # 1. 节点状态
                  echo "--- 节点状态 ---"
                  NOT_READY=$(kubectl get nodes --no-headers | grep -v Ready | wc -l)
                  if [ "$NOT_READY" -gt 0 ]; then
                    echo "⚠️ $NOT_READY 个节点异常"
                    kubectl get nodes | grep -v Ready
                  else
                    echo "✅ 所有节点正常"
                  fi
                  
                  # 2. 异常 Pod
                  echo "--- 异常 Pod ---"
                  BAD_PODS=$(kubectl get pods -A --no-headers | \
                    grep -v "Running\|Completed\|Succeeded" | wc -l)
                  if [ "$BAD_PODS" -gt 0 ]; then
                    echo "⚠️ $BAD_PODS 个异常 Pod"
                    kubectl get pods -A | grep -v "Running\|Completed\|Succeeded"
                  else
                    echo "✅ 所有 Pod 正常"
                  fi
                  
                  # 3. 资源使用率
                  echo "--- 资源使用 ---"
                  kubectl top nodes 2>/dev/null | awk 'NR>1{
                    cpu=$3+0; mem=$5+0;
                    if(cpu>80 || mem>85) print "⚠️ "$1" CPU:"$3" MEM:"$5
                  }'
                  
                  # 4. PVC 状态
                  echo "--- 存储状态 ---"
                  kubectl get pvc -A --no-headers | grep -v Bound | wc -l | \
                    xargs -I{} echo "{} 个 PVC 未绑定"
                  
                  # 5. 证书检查
                  echo "--- 证书有效期 ---"
                  kubectl get certificates -A -o json 2>/dev/null | \
                    jq -r '.items[] | select(.status.renewalTime != null) |
                    select((.status.renewalTime | fromdate) < (now + 604800)) |
                    "⚠️ \(.metadata.namespace)/\(.metadata.name) 将在 7 天内过期"'
                  
                  # 6. 事件统计
                  echo "--- 警告事件 ---"
                  kubectl get events -A --field-selector type=Warning \
                    --sort-by='.lastTimestamp' --no-headers | tail -10
                  
                  echo "=== 巡检完成 ==="
          restartPolicy: OnFailure
```

### 巡检结果通知（Webhook）

```bash
#!/bin/bash
# inspection-notify.sh — 巡检结果推送到 IM
RESULT=$(kubectl logs job/daily-cluster-inspection -n kube-system --tail=50)

# 检查是否有异常
if echo "$RESULT" | grep -q "⚠️"; then
  LEVEL="warning"
  TITLE="⚠️ 集群巡检发现异常"
else
  LEVEL="info"
  TITLE="✅ 集群巡检正常"
fi

# 推送到钉钉/企微/Slack
curl -s -X POST "$WEBHOOK_URL" \
  -H 'Content-Type: application/json' \
  -d "{
    \"msgtype\": \"markdown\",
    \"markdown\": {
      \"title\": \"$TITLE\",
      \"text\": \"## $TITLE\n\n\`\`\`\n$RESULT\n\`\`\`\"
    }
  }"
```

## 监控看板体系

### 分层看板设计

| 层级 | 受众 | 内容 | 刷新频率 |
|------|------|------|----------|
| L1: 业务总览 | 管理层/客户 | SLI/SLO、业务指标 | 5 min |
| L2: 服务健康 | 开发团队 | 四大黄金指标、依赖状态 | 1 min |
| L3: 基础设施 | SRE/运维 | 节点/网络/存储/控制平面 | 30s |
| L4: 深度诊断 | 专家 | 内核/运行时/应用内部 | 按需 |

### 必备 Grafana 看板

```yaml
# 看板清单（按优先级）
# 1. Kubernetes Cluster Overview
#    - 节点状态、Pod 分布、资源使用率
# 2. Namespace Resource Usage
#    - 按命名空间的 CPU/内存/网络/存储
# 3. Pod Performance (per service)
#    - 延迟 P50/P95/P99、错误率、QPS、饱和度
# 4. Network Overview
#    - 跨节点流量、DNS 延迟、Ingress 吐吐
# 5. Storage Performance
#    - PV 使用率、IOPS、延迟
# 6. Control Plane Health
#    - API Server 延迟、etcd 状态、调度器吐吐
```

## 容量规划

### 资源使用趋势分析

```bash
# 🟢 查看当前资源分配率
kubectl describe nodes | grep -A5 "Allocated resources" | \
  awk '/cpu/{print "CPU: "$3"/"$5} /memory/{print "MEM: "$3"/"$5}'

# 🟢 查看各命名空间资源使用
kubectl top pods -A --sort-by=memory | head -20

# 容量规划公式:
# 所需节点数 = (当前 Pod 总 requests × 1.3 安全系数) / 单节点可分配量
# 例: 当前 100 Pod，平均 request 0.5C/1G
#   总需求 = 100 × 0.5 × 1.3 = 65 CPU
#   单节点 (8C16G) 可分配 ≈ 6.5 CPU
#   所需节点 = 65 / 6.5 = 10 节点
```

### 扩容触发条件

| 指标 | 警告阈值 | 扩容阈值 | 操作 |
|------|----------|----------|------|
| CPU 分配率 | > 70% | > 85% | 添加节点 |
| 内存分配率 | > 75% | > 90% | 添加节点 |
| Pod 数/节点 | > 80 | > 100 | 添加节点 |
| PVC 使用率 | > 80% | > 90% | 扩容卷 |
| 待调度 Pod | > 0 持续 5min | > 5 | 紧急扩容 |

## 周期性运维任务

### 周度任务

| 任务 | 操作 | 负责人 |
|------|------|--------|
| 告警质量审查 | 清理噪音告警、调整阈值 | SRE |
| 资源使用报告 | 生成周报、识别浪费 | SRE |
| 安全扫描 | 镜像漏洞扫描、RBAC 审计 | 安全 |
| 备份验证 | 恢复测试（至少 1 个服务） | SRE |
| 依赖更新 | 检查组件版本 EOL | 平台 |

### 月度任务

| 任务 | 操作 | 负责人 |
|------|------|--------|
| 集群升级评估 | 检查 K8s 版本 EOL、规划升级 | 架构师 |
| 成本优化 | Right-Sizing、清理闲置资源 | FinOps |
| DR 演练 | 恢复流程验证 | SRE |
| 容量规划 | 下月资源需求预测 | SRE |
| 安全审计 | 全量 RBAC/NetworkPolicy 审查 | 安全 |
| 文档更新 | Runbook/架构图更新 | 全员 |

### 季度任务

| 任务 | 操作 | 负责人 |
|------|------|--------|
| GameDay | 故障注入演练 | SRE |
| 架构审查 | 单点故障/扩展性评估 | 架构师 |
| 工具链升级 | 监控/日志/CI 工具更新 | 平台 |
| 培训 | 新工具/流程培训 | 全员 |

## 运维成熟度模型

```
Level 1: 救火式 (Reactive)
└── 故障后响应，无标准流程，依赖个人英雄

Level 2: 标准化 (Standardized)
└── 有巡检清单、告警分级、基本 Runbook

Level 3: 自动化 (Automated)
└── 自动巡检、自动扩容、自动修复、GitOps

Level 4: 可观测 (Observable)
└── 全链路追踪、SLO 驱动、异常检测、根因分析

Level 5: 自愈 (Self-Healing)
└── 混沌工程、AIOps、预测性维护、零人工介入
```

## 远程顾问指导要点

作为远程顾问，无法直连集群，需通过以下方式指导现场执行：

1. **结构化提问**：按巡检清单逐项询问结果，避免遗漏
2. **输出审核**：要求客户提供命令输出截图或文本，逐项审核
3. **变更方案评审**：要求客户提交变更方案文档，包含：
   - 变更范围与影响分析
   - 回滚步骤（每一步需验证命令）
   - 验证方法与通过标准
4. **应急预案**：确认客户已演练回滚操作，而非仅书面记录
5. **巡检报告模板**：提供标准化报告格式，确保信息完整性
6. **定期回顾**：每周审查巡检结果趋势，识别潜在风险

> 远程顾问应建立标准话术模板，将巡检清单转化为可执行的对话流程，确保每次指导的一致性。

## 相关链接

- [[19-故障诊断/08-技能体系/01-node-notready|node-notready]] — 节点异常的排查方法
- [[22-概念/08-可靠性与运维/cluster-upgrade-paths.md|cluster-upgrade-paths]] — 集群升级路径与版本兼容性
- [[13-生产运维/07-运维手册/02-change-management-guide.md|change-management-guide]] — 变更管理的详细流程
- [[13-生产运维/03-事件响应/04-incident-response-template.md|incident-response-playbook]] — 事件响应操作手册
- [[13-生产运维/03-事件响应/03-on-call-playbook.md|值班手册]] — 告警响应与值班规范
- [[09-可观测性/02-指标/index.md|指标监控]] — 监控体系设计

## Related

- [[visibility-public|#visibility/public Hub]] — tag hub


<!-- risk-assessed -->
