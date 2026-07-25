---
title: 运维 Runbook 模板
description: 标准运维 Runbook 模板，用于操作手册编写
summary: 运维 Runbook 模板 — 标准化的操作手册模板，覆盖诊断、处置、验证全流程
category: template
tags:
- runbook
- template
- operations
- standard
- sre
tier: supporting
created: '2026-07-11'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- SRE
- 运维工程师
- oncall 工程师
estimated_read_time: 5min
intent_queries:
- Runbook 模板 是什么
- 如何编写 Kubernetes 运维手册
- 标准运维 Runbook 模板
- operations runbook template
trigger_keywords:
- runbook
- 运维手册
- 模板
- 操作
- operations
prerequisites:
- kubectl-basics
- incident-response-basics
---

> **生产环境安全提示**
>
> 本模板包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险、🟡 中风险、🟢 低风险/只读。

# 运维 Runbook 模板

> **模板版本**: 1.0 | **适用范围**: 所有运维操作场景 | **使用方式**: 复制此模板，替换所有 `[PLACEHOLDER]`

## 模板使用说明

1. 复制本模板到目标位置 (如 `故障诊断/runbooks/`)
2. 替换所有 `[PLACEHOLDER]` 为实际内容
3. 验证所有命令在目标环境中可执行
4. 由至少一名同事 Review 后发布
5. 定期演练验证 Runbook 的有效性

---

## [Runbook 标题 — 如: Pod OOM 排查手册]

> **Runbook ID**: RB-[NN]
> **服务/组件**: [服务名称]
> **严重级别**: [P0/P1/P2/P3]
> **最后更新**: [YYYY-MM-DD]
> **负责人**: [团队/个人]
> **Review 状态**: [Draft / Reviewed / Active / Deprecated]

### 1. 概述

[一句话描述本 Runbook 解决的问题。例如: "本手册用于排查和修复 Pod 因内存不足 (OOMKilled) 被驱逐的问题。"]

**触发条件**: [在什么告警/场景下应使用本 Runbook]
- 告警名称: `[alert_name]`
- 告警条件: `[PromQL 或描述]`
- 告警级别: `[critical/warning]`

**影响范围**:
- 受影响服务: [服务列表]
- 受影响用户: [用户比例/范围]
- 业务影响: [收入/体验影响描述]

### 2. 前置检查

执行任何操作前，先完成以下检查:

```bash
# 🟢 低风险：确认当前集群和命名空间
kubectl config current-context
kubectl get namespace [NAMESPACE]

# 🟢 低风险：确认有足够的权限
kubectl auth can-i patch deployments -n [NAMESPACE]
```

**所需权限**: [列出需要的 RBAC 权限，如 `edit` on namespace `[NAMESPACE]`]

**所需工具**:
- [ ] `kubectl` >= [版本]
- [ ] [其他工具，如 `jq`, `curl`, 特定 CLI]

### 3. 诊断步骤

#### 3.1 [诊断步骤名称 — 如: 确认 OOM 事件]

```bash
# 🟢 低风险：查看 Pod 状态和重启次数
kubectl get pods -n [NAMESPACE] -l app=[APP_NAME] -o wide

# 🟢 低风险：查看 Pod 详情，确认 Last State
kubectl describe pod [POD_NAME] -n [NAMESPACE] | grep -A5 "Last State"
```

**预期结果**: [描述正常和异常情况下的输出]
- ✅ 正常: [正常输出描述]
- 🔴 异常: [异常输出描述，如 `Reason: OOMKilled`]

**判定逻辑**:
| 观察结果 | 结论 | 下一步 |
|---------|------|--------|
| [观察1] | [结论1] | 跳转到 3.2 |
| [观察2] | [结论2] | 跳转到 3.3 |

#### 3.2 [诊断步骤名称]

```bash
# 🟢 低风险：[诊断命令]
[command]
```

[继续诊断步骤...]

### 4. 处置方案

根据诊断结果选择对应方案:

#### 方案 A: [方案名称 — 如: 提高 Memory Limit]

**适用场景**: [描述何时使用此方案]
**风险等级**: 🟡 中风险
**预计耗时**: [如 5 分钟]
**是否可逆**: [是/否]

```bash
# 🟡 中风险：修改资源限制 (需要确认目标 Deployment)
kubectl set resources deployment [DEPLOYMENT_NAME] -n [NAMESPACE] \
    --limits=memory=[NEW_LIMIT] \
    --requests=memory=[NEW_REQUEST]

# 🟡 中风险：监控滚动更新
kubectl rollout status deployment/[DEPLOYMENT_NAME] -n [NAMESPACE]
```

**回滚方案**:
```bash
# 🟡 中风险：回滚到上一个版本
kubectl rollout undo deployment/[DEPLOYMENT_NAME] -n [NAMESPACE]
```

#### 方案 B: [方案名称 — 如: 扩容实例]

**适用场景**: [描述]
**风险等级**: 🟢 低风险

```bash
# 🟢 低风险：扩容
kubectl scale deployment [DEPLOYMENT_NAME] -n [NAMESPACE] --replicas=[NEW_REPLICAS]
```

### 5. 验证

处置后执行以下验证:

```bash
# 🟢 低风险：验证 Pod 状态
kubectl get pods -n [NAMESPACE] -l app=[APP_NAME]

# 🟢 低风险：验证无新 OOM 事件 (等待 5 分钟后)
kubectl get events -n [NAMESPACE] --field-selector reason=OOMKilling
```

**验收标准**:
- [ ] 所有 Pod 处于 Running 状态
- [ ] 过去 10 分钟无重启
- [ ] [其他业务指标，如错误率 < 0.1%]

### 6. 升级路径

如果以上方案无法解决:

| 条件 | 升级到 | 通知方式 |
|------|--------|---------|
| 30 分钟内未恢复 | [高级 SRE / 团队负责人] | [电话/企业微信] |
| 数据丢失风险 | [DBA / 数据团队] | [电话/PagerDuty] |
| 安全事件嫌疑 | [安全团队] | [紧急通道] |

### 7. 事后行动

- [ ] 在 [时间] 内完成 [[31-脚本/templates/postmortem-template|事后总结]]
- [ ] 更新本 Runbook (补充新发现的诊断步骤)
- [ ] 检查告警阈值是否需要调整
- [ ] 考虑添加自动化修复 (参考 [[31-脚本/prompts/incident-diagnosis|事件诊断 Prompt]])

### 8. 参考资料

- [[19-故障诊断/08-技能体系/MOC|操作技能索引]] — 相关技能卡片
- [[19-故障诊断/06-FTA故障树/MOC|FTA 故障树]] — 根因分析参考
- [相关监控 Dashboard 链接]
- [相关架构设计文档]

---

## 版本历史

| 版本 | 日期 | 变更 | 作者 |
|------|------|------|------|
| v1.0 | [YYYY-MM-DD] | 初始版本 | [作者] |

<!-- risk-assessed -->
