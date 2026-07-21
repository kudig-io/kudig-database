---
title: 事故响应模板与流程规范
summary: 事故响应模板与流程规范：生产事故不可避免，但事故响应的质量决定了业务恢复速度和客户信任度。本文档提供标准化的事故响应流程、角色定义和沟通模板，帮助远程顾问指导客户建立高效的事故响应机制。
category: 生产运维
tags:
- domain-11
- 事故响应
- incident
- MTTR
- 复盘
- Commander
- visibility/public
tier: supporting
sources:
- KUDIG Gap Analysis 2026-05-21
created: 2026-05-21
updated: 2026-05-21
last_updated: 2026-05-21
status: reviewed
---



# 事故响应模板与流程规范

## 概述

生产事故不可避免，但事故响应的质量决定了业务恢复速度和客户信任度。本文档提供标准化的事故响应流程、角色定义和沟通模板，帮助远程顾问指导客户建立高效的事故响应机制。

## 事故响应六阶段

| 阶段 | 目标 | 关键动作 | 时间约束 |
|---|---|---|---|
| 发现 | 尽早感知异常 | 监控告警、用户反馈、巡检发现 | MTTD 目标 < 5 分钟 |
| 遏制 | 阻止影响扩大 | 流量切换、服务降级、回滚变更 | 越快越好 |
| 根因 | 定位根本原因 | 日志分析、链路追踪、变更比对 | 不阻塞恢复 |
| 修复 | 恢复业务正常 | 执行修复操作、验证恢复 | MTTR 目标因级别而异 |
| 验证 | 确认修复有效性 | 监控指标、业务测试、用户反馈 | 修复后 15 分钟 |
| 复盘 | 总结经验教训 | 时间线整理、根因分析、改进项 | 事故关闭后 48 小时内 |

> **原则**：遏制和修复优先于根因定位。先止血，后手术。

## 事故 Commander 职责

事故 Commander（指挥官）是事故响应的核心角色，不一定是技术最高的人，但必须具备以下能力：

### 信息收集

- 维护实时事故时间线
- 汇总各排查通道的发现（日志、指标、链路）
- 记录已尝试的方案和结果

### 决策

- 判断是否启动升级流程
- 决定采用哪种遏制方案（回滚 / 降级 / 切换）
- 授权执行可能影响数据的修复操作

### 沟通

- 每 15 分钟向内部群同步进展
- 评估是否需要通知客户
- 控制信息口径，避免未经确认的猜测对外传播

## 关键时间节点记录

| 指标 | 定义 | 计算方法 |
|---|---|---|
| MTTD | Mean Time To Detect | 故障发生时间 → 告警触发时间 |
| MTTA | Mean Time To Acknowledge | 告警触发时间 → 值班人员确认时间 |
| MTTR | Mean Time To Repair | 故障发生时间 → 业务恢复时间 |
| MTBF | Mean Time Between Failures | 两次故障之间的平均时间 |

```yaml
# 事故时间线示例（YAML 格式便于归档）
incident_id: INC-2026-0521-001
severity: P0
timeline:
  - time: "2026-05-21T14:03:00Z"
    event: "故障发生"
  - time: "2026-05-21T14:05:00Z"
    event: "Prometheus 告警触发"
  - time: "2026-05-21T14:06:00Z"
    event: "值班人员确认"
  - time: "2026-05-21T14:12:00Z"
    event: "执行回滚"
  - time: "2026-05-21T14:18:00Z"
    event: "业务指标恢复正常"
```

## 沟通模板

### 内部通知（即时通讯）

```
【P0 事故】支付服务异常
- 发现时间：14:03
- 影响范围：支付接口延迟 > 10s，成功率 < 50%
- 当前状态：正在排查
- Commander：张三
- 进度同步：每 15 分钟更新
```

### 客户通知（邮件）

```
主题：[事故通知] XX 服务异常 — 已恢复 / 处理中
尊敬的客户：
我们发现 XX 服务于 XX:XX 出现异常，影响范围包括 XXX。
当前状态：已定位 / 已恢复 / 预计 XX:XX 恢复
我们将于事故关闭后 24 小时内提供详细报告。
```

### 升级邮件

```
主题：[升级] P0 事故 INC-XXXX — 需要高层决策
事故已持续 XX 分钟，当前方案存在风险：
- 方案 A：回滚（预计 5 分钟恢复，可能丢失 3 分钟数据）
- 方案 B：修复（预计 30 分钟恢复，无数据丢失）
请确认采用哪种方案。
```

## 事故分级详细标准

| 级别 | 定义 | 影响范围 | 响应 | MTTR 目标 | 复盘 |
|------|------|----------|------|-----------|------|
| SEV-1 | 核心业务完全中断 | 所有用户 | 全员响应 | < 30 min | 必须 |
| SEV-2 | 核心功能严重降级 | > 30% 用户 | 团队响应 | < 1 h | 必须 |
| SEV-3 | 非核心功能异常 | < 30% 用户 | 值班处理 | < 4 h | 建议 |
| SEV-4 | 轻微影响/预警 | 极少用户 | 记录处理 | < 24 h | 可选 |

### K8s 典型事故场景映射

| 场景 | 级别 | 影响 | 遏制方案 |
|------|------|------|----------|
| API Server 不可用 | SEV-1 | 所有操作失败 | 恢复控制平面 |
| etcd 集群失败 | SEV-1 | 数据丢失风险 | 恢复 etcd 快照 |
| 核心服务 5xx > 50% | SEV-1 | 业务中断 | 回滚/扩容 |
| 单节点 NotReady | SEV-3 | Pod 重调度 | 修复/替换节点 |
| DNS 解析失败 | SEV-2 | 服务间通信中断 | 重启 CoreDNS |
| PVC 绑定失败 | SEV-3 | 新 Pod 无法启动 | 检查 SC/PV |
| 证书过期 | SEV-2 | TLS 连接失败 | 更新证书 |
| 资源配额耗尽 | SEV-3 | 新部署失败 | 清理/扩容 |

## 事故指挥体系（ICS）

### 角色定义

```
┌─────────────────────────────────────────────────────┐
│  Incident Commander (IC)                             │
│  职责: 全局协调、决策、升级、对外沟通          │
├─────────────────────────────────────────────────────┤
│                                                     │
│  ┌─────────────┐  ┌─────────────┐  ┌───────────┐  │
│  │ Tech Lead   │  │ Comms Lead  │  │ Ops Lead  │  │
│  │ 技术排查   │  │ 沟通协调   │  │ 执行操作  │  │
│  │ 根因分析   │  │ 进展同步   │  │ 回滚/修复 │  │
│  │ 方案制定   │  │ 客户通知   │  │ 变更执行  │  │
│  └─────────────┘  └─────────────┘  └───────────┘  │
│                                                     │
│  ┌─────────────────────────────────────────────┐  │
│  │ Subject Matter Experts (SME)                  │  │
│  │ DBA / 网络 / 安全 / 应用开发               │  │
│  └─────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────┘
```

### IC 决策框架

```markdown
## IC 决策检查单

### 启动时 (0-5 min)
- [ ] 确认事故级别（SEV-1/2/3/4）
- [ ] 开启事故频道/战争房间
- [ ] 分配角色（Tech/Comms/Ops）
- [ ] 确认影响范围（哪些服务/用户）

### 遏制时 (5-15 min)
- [ ] 最近有变更？→ 优先回滚
- [ ] 能否降级？→ 关闭非核心功能
- [ ] 能否切换？→ DNS/流量切换到备用
- [ ] 需要升级？→ 通知管理层

### 恢复时 (15-60 min)
- [ ] 修复方案确认（风险评估）
- [ ] 执行修复（Ops Lead）
- [ ] 验证恢复（监控 + 业务测试）
- [ ] 宜布恢复（Comms Lead）

### 关闭时
- [ ] 确认所有指标正常
- [ ] 确认无残留影响
- [ ] 安排复盘时间（48h 内）
- [ ] 发送关闭通知
```

## K8s 事故场景 Runbook

### 场景 1: 控制平面不可用

```bash
# 症状: kubectl 无法连接 API Server
# 级别: SEV-1

# 🟢 诊断
kubectl cluster-info  # 确认不可达
curl -k https://<api-server>:6443/healthz  # 直接检查

# 检查控制平面 Pod（如果能 SSH 到 master）
crictl ps | grep -E "kube-apiserver|etcd|kube-scheduler|kube-controller"
journalctl -u kubelet --since "10 min ago" | grep -i error

# 检查 etcd 健康
etcdctl endpoint health --cluster
etcdctl endpoint status --write-out=table

# 🟡 修复: API Server 崩溃
systemctl restart kubelet  # kubelet 会自动重启静态 Pod

# 🔴 修复: etcd 数据损坏（最后手段）
etcdctl snapshot restore /backup/etcd-snapshot.db \
  --data-dir=/var/lib/etcd-restored
# 然后更新 etcd 配置指向新数据目录
```

### 场景 2: 大规模 Pod 崩溃

```bash
# 症状: 多个服务同时 CrashLoopBackOff
# 级别: SEV-1/2

# 🟢 诊断 — 确认是否共同原因
kubectl get pods -A --field-selector status.phase!=Running -o wide | \
  awk '{print $1, $2, $4, $8}' | sort

# 检查共同因素:
# 1. 同一节点？ → 节点问题
# 2. 同一时间？ → 变更/依赖问题
# 3. 同一镜像？ → 镜像问题

# 检查最近变更
kubectl get events -A --sort-by='.lastTimestamp' | grep -i "pull\|create\|update" | tail -20

# 🟡 遏制: 回滚最近部署
kubectl rollout undo deploy/<name> -n <ns>

# 如果是配置变更导致
kubectl rollout undo deploy/<name> -n <ns> --to-revision=<N>
```

### 场景 3: 网络分区

```bash
# 症状: 部分节点间 Pod 无法通信
# 级别: SEV-2

# 🟢 诊断
# 在问题 Pod 中测试
kubectl exec -it <pod-a> -- ping <pod-b-ip>  # 跨节点
kubectl exec -it <pod-a> -- nslookup kubernetes.default  # DNS

# 检查 CNI 状态
kubectl -n kube-system get pods -l k8s-app=calico-node -o wide
kubectl -n kube-system logs -l k8s-app=calico-node --tail=50

# 检查节点路由
ip route show | grep -E "10.244|cali|cilium"

# 🟡 修复: 重启 CNI Pod
kubectl -n kube-system delete pod <cni-pod-on-bad-node>

# 检查防火墙/安全组规则变更
iptables -L -n | grep DROP | head -20
```

## 复盘模板（Blameless Postmortem）

```markdown
# 事故复盘: [INC-XXXX] <标题>

## 基本信息
- 日期: 2026-XX-XX
- 级别: SEV-X
- 影响时长: XX 分钟
- 影响范围: <服务/用户数>
- Commander: <姓名>
- 参与者: <列表>

## 摘要
<一段话描述发生了什么、影响、根因、修复>

## 时间线
| 时间 | 事件 |
|------|------|
| 14:00 | 故障发生（实际） |
| 14:05 | 告警触发 |
| 14:06 | 值班确认 |
| 14:10 | 定位根因 |
| 14:15 | 执行回滚 |
| 14:20 | 业务恢复 |

## 根因分析 (5-Why)
1. Why: <直接原因>
2. Why: <深层原因>
3. Why: <流程缺陷>
4. Why: <根本原因>
5. Why: <系统性问题>

## 做得好的
- <列出响应中的亮点>

## 待改进的
- <列出不足>

## 行动项
| 行动 | 负责人 | 截止日期 | 优先级 |
|------|--------|----------|--------|
| <具体改进> | <人> | <日期> | P0/P1/P2 |

## 经验教训
<可复用的经验，沉淀到知识库>
```

## 事故度量与改进

### 月度事故报告指标

| 指标 | 目标 | 计算 | 改进方向 |
|------|------|------|----------|
| 事故总数 | 趋势下降 | 月度统计 | 预防 |
| MTTD | < 5 min | 发生→发现 | 监控覆盖 |
| MTTA | < 3 min | 发现→确认 | 通知优化 |
| MTTR | < 30 min | 发现→恢复 | Runbook/自动化 |
| 重复事故率 | < 10% | 同类事故复发 | 行动项落地 |
| 复盘完成率 | 100% | SEV-1/2 必须 | 流程强制 |
| 行动项完成率 | > 90% | 按时完成 | 跟踪机制 |

### 事故趋势分析

```bash
# 事故分类统计（按根因）
# 变更引入: 40% → 改进: 金丝雀/自动化测试
# 容量不足: 25% → 改进: 自动扩容/容量规划
# 依赖故障: 20% → 改进: 熔断/降级/多活
# 配置错误: 10% → 改进: 策略即代码/审查
# 其他: 5%
```

## 远程顾问在事故中的角色

远程顾问不是事故 Commander，但扮演关键的辅助角色：

1. **诊断建议**：基于经验提供排查方向，帮助缩小根因范围
2. **方案审核**：对客户提出的修复方案进行风险评估，指出潜在副作用
3. **事后复盘**：参与复盘会议，提供外部视角，帮助发现盲点和流程改进点
4. **知识沉淀**：将事故处理过程写入 [[可靠性/事后复盘/02-postmortem-culture-guide.md|postmortem]]，补充到 [[概念/KUDIG Knowledge Base Architecture.md|knowledge-base]]
5. **模式识别**：对比历史事故，识别重复模式，提出系统性改进

> 远程顾问应避免直接「接管」指挥权，而是帮助客户 Commander 做出更 informed 的决策。

## 相关链接

- [[生产运维/03-on-call-playbook.md|on-call-playbook]] — 值班手册与告警响应规范
- [[生产运维/02-change-management-guide.md|change-management-guide]] — 变更管理指南
- [[生产运维/01-production-sre-daily-ops.md|production-sre-daily-ops]] — 日常巡检与值班手册
- [[可靠性/事后复盘/index.md|事后复盘]] — 复盘文化与方法
- [[故障诊断/03-systematic-troubleshooting-methodology.md|系统化排障]] — 故障诊断方法论
- [[node-notready]] — 节点异常排查

## Related

- [[visibility-public|#visibility/public Hub]] — tag hub
