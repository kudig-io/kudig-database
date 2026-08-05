---
title: 变更窗口与审批流程
description: 面向阿里云/专有云 K8s 的变更窗口与审批流程设计，涵盖变更分级、时间窗口、审批链、ASO 集成与风险控制。
summary: 面向阿里云/专有云 K8s 的变更窗口与审批流程设计，涵盖变更分级、时间窗口、审批链、ASO 集成与风险控制。
category: release-management
tags:
- k8s
- change-management
- approval
- change-window
- rfc
- governance
- alicloud
- apsara-stack
tier: supporting
created: '2026-06-29'
updated: '2026-06-29'
last_updated: 2026-06
difficulty: intermediate
reading_level: intermediate
audience:
- SRE
- 变更经理
- 运维工程师
estimated_read_time: 20min
intent_queries:
- 变更窗口设计
- K8s 变更审批流程
- 阿里云专有云变更管理
trigger_keywords:
- 变更窗口
- 审批流程
- RFC
- change window
- approval
prerequisites:
- kubectl-basics
- gitops-basics
- sre-practices
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
authors:
- name: KUDIG Team
  role: contributor
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 变更窗口与审批流程

> **适用版本**: Kubernetes v1.28 - v1.32 | **最后更新**: 2026-06
> **文档定位**: 面向阿里云/专有云 K8s 环境，建立生产变更的窗口划分、分级审批与风险控制机制。

## 目录

1. [变更管理原则](#变更管理原则)
2. [变更分级](#变更分级)
3. [变更窗口设计](#变更窗口设计)
4. [审批链设计](#审批链设计)
5. [RFC 模板](#rfc-模板)
6. [ASO/工单系统集成](#aso工单系统集成)
7. [风险控制](#风险控制)
8. [最佳实践检查清单](#最佳实践检查清单)

---

## 1. 变更管理原则

### 1.1 核心原则

| 原则 | 说明 |
|:---|:---|
| 可回滚 | 每个变更必须有回滚方案 |
| 可观测 | 变更过程必须有监控与告警 |
| 最小范围 | 变更影响范围尽可能小 |
| 受控时间 | 高风险变更在指定窗口执行 |
| 双人复核 | 关键操作需第二人确认 |

### 1.2 变更类型

| 类型 | 示例 | 风险等级 |
|:---|:---|:---:|
| 应用发布 | Deployment 镜像更新 | 中 |
| 配置变更 | ConfigMap、Secret 更新 | 中 |
| 基础设施 | 节点升级、网络调整 | 高 |
| 安全补丁 | CVE 修复 | 高 |
| 数据变更 | 数据库 schema 变更 | 极高 |
| 容量变更 | 扩容、缩容 | 低 |

---

## 2. 变更分级

### 2.1 分级标准

| 级别 | 影响范围 | 响应要求 | 审批人 |
|:---:|:---|:---|:---|
| L1 标准 | 单个应用/命名空间 | 工作日窗口 | 团队负责人 |
| L2 重要 | 多个应用/核心服务 | 指定窗口 | 部门负责人 + SRE |
| L3 重大 | 集群/平台级 | 夜间窗口 + 值班长 | 架构师 + 运维总监 |
| L4 紧急 | 生产故障修复 | 立即执行，事后补单 | 值班长 |

### 2.2 分级判定表

| 判定条件 | 级别 |
|:---|:---:|
| 影响用户 < 1000 | L1 |
| 影响用户 1000-10000 或核心链路 | L2 |
| 影响用户 > 10000 或多集群 | L3 |
| 生产 P0 故障修复 | L4 |

---

## 3. 变更窗口设计

### 3.1 标准变更窗口

| 窗口类型 | 时间 | 适用级别 | 说明 |
|:---|:---|:---:|:---|
| 日常窗口 | 工作日 10:00-12:00, 14:00-17:00 | L1 | 低风险变更 |
| 晚间窗口 | 22:00-次日 02:00 | L2/L3 | 流量低谷 |
| 周末窗口 | 周六 00:00-06:00 | L3 | 重大变更 |
| 紧急窗口 | 即时 | L4 | 故障修复 |

### 3.2 阿里云/专有云特殊窗口

- 专有云版本发布窗口通常由 ASO 统一编排
- 云底座升级需避开业务高峰
- 跨 Region 变更需考虑时区差异

---

## 4. 审批链设计

### 4.1 审批矩阵

| 级别 | 提交人 | 技术审批 | 运维审批 | 安全审批 | 业务审批 |
|:---:|:---|:---:|:---:|:---:|:---:|
| L1 | 开发/SRE | ✓ | - | - | - |
| L2 | SRE | ✓ | ✓ | - | - |
| L3 | 架构师 | ✓ | ✓ | ✓ | ✓ |
| L4 | 值班长 | 事后 | 事后 | 事后 | 事后 |

### 4.2 自动化审批 Checklist

```yaml
# 变更请求 CRD 示例
apiVersion: change.example.com/v1
kind: RFCRequest
metadata:
  name: upgrade-ingress-controller
  namespace: change-management
spec:
  title: "升级 Ingress Controller 至 v1.10"
  changeType: "infrastructure"
  riskLevel: "medium"
  impact: "high"
  affectedComponents:
    - ingress-nginx
    - production-gateway
  rollbackPlan:
    procedure: "kubectl rollout undo deployment/ingress-nginx-controller -n ingress-nginx"
    timeframe: "5 minutes"
    validation: "curl -I https://api.example.com/health"
  approvalChain:
    - approver: "platform-architect"
      role: "technical-lead"
      required: true
    - approver: "sre-manager"
      role: "operations-lead"
      required: true
  schedule:
    plannedStart: "2026-07-01T22:00:00Z"
    plannedEnd: "2026-07-01T23:00:00Z"
    maintenanceWindow: "1 hour"
```

---

## 5. RFC 模板

### 5.1 标准 RFC 模板

```markdown
# 变更请求 (RFC)

## 基本信息
- **RFC 编号**: RFC-2026-0701-001
- **变更标题**: 升级 Ingress Controller 至 v1.10
- **提交人**: 张三
- **提交时间**: 2026-06-29
- **计划窗口**: 2026-07-01 22:00 - 23:00
- **风险等级**: 中
- **影响范围**: 生产环境所有入口流量

## 变更内容
1. 更新 ingress-nginx Deployment 镜像
2. 调整 controller 配置参数
3. 验证灰度流量

## 回滚方案

```bash
kubectl rollout undo deployment/ingress-nginx-controller -n ingress-nginx
```

## 验证步骤
1. Pod 全部 Running
2. 健康检查接口返回 200
3. 业务流量监控无异常

## 审批记录
| 审批人 | 角色 | 状态 | 时间 |
|---|---|---|---|
| 李四 | 技术负责人 | 已批准 | 2026-06-30 |
| 王五 | SRE 负责人 | 已批准 | 2026-06-30 |
```
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
---

## 6. ASO/工单系统集成

### 6.1 工单触发变更

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl label/annotate`：改元数据可能影响选择器/控制器

```bash
# 通过工单号关联变更记录
kubectl annotate rfc upgrade-ingress-controller \
  ticket-id="TICKET-20260701-001" \
  aso-change-id="ASO-CHANGE-12345"
```
### 6.2 变更状态同步

```bash
# 变更完成后更新工单状态
aliyun oos StartExecution \
  --TemplateName ChangeStatusSync \
  --Parameters '{"ticketId":"TICKET-20260701-001","status":"completed"}'
```

---

## 7. 风险控制

### 7.1 变更前检查

| 检查项 | 要求 |
|:---|:---|
| 测试环境验证 | 必须完成 |
| 回滚方案 | 必须可执行 |
| 监控告警 | 变更期间专人值守 |
| 备份状态 | 关键数据已备份 |
| 通知相关方 | 业务方已知情 |

### 7.2 变更中监控

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 实时查看变更资源状态
kubectl rollout status deployment/ingress-nginx-controller -n ingress-nginx --timeout=300s

# 监控 Pod 重启与错误率
kubectl get pods -n ingress-nginx -w
```
### 7.3 变更后验证

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 验证服务健康
kubectl get pods -n ingress-nginx
kubectl logs -l app.kubernetes.io/name=ingress-nginx -n ingress-nginx --tail=100

# 验证业务入口
for i in {1..10}; do curl -s -o /dev/null -w "%{http_code}\n" https://api.example.com/health; done
```
---

## 8. 最佳实践检查清单

| 检查项 | 要求 | 验证方式 |
|:---|:---|:---|
| RFC 已创建 | 所有 L1+ 变更 | 变更系统 |
| 审批已完成 | 对应级别审批人 | 审批记录 |
| 窗口已确认 | 不在业务高峰 | 变更日历 |
| 回滚方案 | 已验证可执行 | 测试记录 |
| 监控值守 | 变更期间在线 | 值班表 |
| 变更结果记录 | 成功/失败/回滚 | 变更系统 |

---

## 变更风险管理

变更前必须识别潜在风险并制定缓解措施。风险矩阵可帮助快速判定变更等级。

| 影响范围 \ 回滚难度 | 低 | 中 | 高 |
|:---|:---:|:---:|:---:|
| 单 Pod / 单服务 | 低风险 | 中风险 | 高风险 |
| 多服务 / 多命名空间 | 中风险 | 高风险 | 极高风险 |
| 控制平面 / 底座 | 高风险 | 极高风险 | 极高风险 |

### 变更通知示例

```markdown
【生产变更通知】
变更编号：RFC-20260629-001
变更内容：升级 nginx-ingress-controller 到 v1.10
窗口：2026-06-30 02:00 - 04:00
影响：ingress 控制面短暂重启，可能导致部分请求重试
负责人：sre-oncall
回滚方式：kubectl rollout undo deployment/ingress-nginx-controller -n ingress-nginx
```

### 变更事后复盘

变更完成后 24 小时内完成复盘，记录实际耗时、异常、改进项，并更新变更手册。

## 变更沟通与协作

变更管理的核心是透明沟通。所有变更应在变更日历中可见，相关方可以提前知晓潜在影响。

### 变更日历字段

| 字段 | 说明 |
|:---|:---|
| RFC 编号 | 唯一标识 |
| 变更标题 | 一句话描述 |
| 负责人 | 主负责人与备份 |
| 窗口时间 | 精确到分钟 |
| 影响范围 | 服务、命名空间、用户群 |
| 回滚方案 | 关键回滚命令 |
| 验证方式 | 验证指标与命令 |
| 状态 | 已计划 / 执行中 / 已完成 / 已回滚 |

### 变更复盘要点

1. 是否按时完成？偏差原因是什么？
2. 是否出现意外？如何处理？
3. 回滚方案是否有效？
4. 监控是否及时发现问题？
5. 下次同类变更可优化哪些点？

## 典型工单场景与处理

**场景**：用户反馈某变更未在窗口期执行，却影响了生产。

处理步骤：
1. 核查变更日历与审批记录。
2. 确认是否未按流程执行或越权操作。
3. 评估影响范围并启动回滚或修复。
4. 将事件纳入变更复盘并更新流程。

## 阿里云/专有云变更入口

| 平台 | 入口 | 用途 |
|:---|:---|:---|
| 阿里云 ACK | 控制台 → 集群运维 → 变更记录 | 查看集群级变更 |
| 专有云 ASO | ASO 控制台 → 变更管理 | 提交与审批 RFC |
| 天基 | 运维平台 → 变更中心 | 底座与产品变更 |

### 变更审批工单示例

```markdown
## RFC-20260629-001
- 变更人：张三
- 变更内容：升级 ingress-nginx-controller 到 v1.10
- 风险等级：中风险
- 审批人：李四（技术负责人）
- 审批结果：通过
- 备注：请在维护窗口执行，执行前确认灰度环境验证通过
```

### 变更失败快速判定

| 现象 | 判定 |
|:---|:---|
| 错误率 > SLO 阈值 | 可能需回滚 |
| 关键告警持续 5 分钟 | 启动回滚评估 |
| 用户投诉激增 | 立即通知变更负责人 |
| 监控无异常但业务反馈异常 | 延长观察期并加监控 |

## Related

- [[32-发布/package/2026-07-02_18-40/corpus/supporting/domain-08-release-change-management/03-change-management/05-change-management-process|变更管理流程]]
- [[32-发布/package/2026-07-02_18-40/corpus/core/domain-11-production-operations/03-change-management-guide|变更管理指南]]

## See Also

- [[domain-08-release-change-management/变更管理/02-canary-release-strategy.md|金丝雀发布策略与回滚]]
- [[domain-08-release-change-management/变更管理/03-change-rollback-playbook.md|变更回滚操作手册]]

```

<!-- risk-assessed -->
