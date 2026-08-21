---
title: Helm Chart 部署与回滚故障诊断
description: Kubernetes Helm Chart 部署与回滚故障的完整诊断-修复-验证工单处理 Skill
summary: Kubernetes Helm Chart 部署与回滚故障的完整诊断-修复-验证工单处理 Skill
category: skills
tags:
- k8s
- skills
- sop
- runbook
- helm
- chart
- deployment
- rollback
tier: supporting
created: '2026-05-23'
last_updated: '2026-05-21'
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
estimated_read_time: 15min
intent_queries:
- Helm Chart 部署与回滚故障诊断 是什么
- 如何 Helm Chart 部署与回滚故障诊断
trigger_keywords:
- helm
- chart
- release
- rollback
- upgrade failed
- template error
prerequisites:
- kubectl-basics
- troubleshooting-methodology
- helm-basics
skill_id: SKILL-HELM-001
skill_name: Helm Chart 部署与回滚故障诊断
version: 1.0.0
k8s_versions:
- 1.28.x
- 1.29.x
- 1.30.x
- 1.31.x
- 1.32.x
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Helm Chart 部署与回滚故障诊断

> **Skill ID**: `SKILL-HELM-001`  
> **严重级别**: P2  
> **执行模式**: L1  
> **来源**: FTA + Structural 文档派生

---

## 1. 概述

Helm Chart 部署与回滚故障诊断 是 [[kubernetes|Kubernetes]] 生产环境中 **P2 级问题**。

**典型触发条件**:
- `helm upgrade` 返回 template rendering error
- Release 状态为 `failed` 或 `pending-upgrade`
- 回滚后 Pod 仍为 CrashLoopBackOff
- Chart 依赖的子 chart 版本不兼容

**爆炸半径评估**:
- 影响范围: 单个 Release 对应的所有资源（Deployment/Service/ConfigMap等）
- 恢复时间目标 (RTO): 15分钟（回滚）/ 60分钟（修复前向）
- 是否需要人工审批: 视修复动作而定

---

## 2. 症状识别

| 症状模式 | 置信度 | 检查命令 |
|---------|--------|--------|
| helm upgrade 报 template error | 0.95 | `helm upgrade --dry-run --debug` |
| Release 状态 failed/pending | 0.90 | `helm status <release> -n <ns>` |
| Pod ImagePullBackOff after upgrade | 0.85 | `kubectl describe pod -l app.kubernetes.io/instance=<release>` |
| 回滚后配置未恢复 | 0.80 | `helm get values <release> --revision <n>` |

---

## 3. 快速检查（< 2 分钟）

```bash
# 🟢 低风险：确认问题范围
helm list -n <namespace> -a
helm status <release> -n <namespace>
helm history <release> -n <namespace> | tail -5
kubectl get pods -n <namespace> -l app.kubernetes.io/instance=<release>
```

**决策点**:
- 如果 Release 状态为 `failed` → 跳转到 Phase 2-A（模板/值错误）
- 如果 Release 状态为 `deployed` 但 Pod 异常 → 跳转到 Phase 2-B（运行时问题）
- 如果都不匹配 → 升级到 L2 人工诊断

---

## 4. Phase 1: 信息收集（2-5 分钟）

```bash
# 🟢 低风险：收集基础信息
helm get values <release> -n <ns> -o yaml > current-values.yaml
helm get manifest <release> -n <ns> > current-manifest.yaml
helm get notes <release> -n <ns>
kubectl get events -n <ns> --sort-by='.lastTimestamp' | tail -20
```

---

## 5. Phase 2: 根因定位（5-10 分钟）

| 根因假设 | 验证命令 | 验证标准 |
|---------|---------|--------|
| 模板渲染错误 | `helm template <chart> -f values.yaml --debug` | 无报错，输出完整 YAML |
| 依赖 chart 版本不兼容 | `helm dependency list <chart>` | 所有依赖状态为 ok |
| values 覆盖冲突 | `helm get values <release> --all` | 无意外覆盖值 |
| 资源配额不足 | `kubectl describe resourcequota -n <ns>` | 剩余配额足够 |
| 镜像不存在/tag错误 | `kubectl describe pod -l app.kubernetes.io/instance=<release>` | 无 ImagePullBackOff |

---

## 6. Phase 3: 修复操作

### 🟢 低风险（L0-L1，可自动执行）
| 动作 | 命令 | 预期结果 |
|------|------|--------|
| 查看 Release 历史 | `helm history <release> -n <ns>` | 显示各版本状态 |
| 模板干跑验证 | `helm upgrade --dry-run --debug <release> <chart>` | 无渲染错误 |
| 查看当前值 | `helm get values <release> -n <ns> --all` | 输出完整配置 |

### 🟡 中风险（L2，需确认后执行）
| 动作 | 命令 | 风险说明 | 确认方式 |
|------|------|---------|--------|
| 回滚到上一版本 | `helm rollback <release> <rev> -n <ns>` | 会变更所有资源 | 确认目标 revision |
| 修正 values 重新升级 | `helm upgrade <release> <chart> -f fixed-values.yaml` | 变更集群资源 | --dry-run 验证 |
| 修复依赖 | `helm dependency update <chart>` | 更新 Chart.lock | 检查版本兼容性 |

### 🔴 高风险（L3，必须人工审批）
| 动作 | 命令 | 风险说明 | 审批人 |
|------|------|---------|--------|
| 删除并重装 Release | `helm uninstall <release> && helm install` | 可能丢失 PVC 数据 | 技术主管 |
| 强制删除卡住的 Release | `helm uninstall <release> --no-hooks` | 跳过清理钩子 | SRE Lead |

---

## 7. 验证

```bash
# 🟢 验证修复结果
helm status <release> -n <ns>
helm test <release> -n <ns>  # 如果 chart 定义了 test hook
kubectl get pods -n <ns> -l app.kubernetes.io/instance=<release>
kubectl rollout status deployment -n <ns> -l app.kubernetes.io/instance=<release>
```

**验证标准**:
- [ ] 指标恢复正常
- [ ] Pod/节点状态正常
- [ ] 业务流量恢复

---

## 8. 回滚方案

如果修复后问题加剧，执行以下回滚：

```bash
# 🟡 中风险：回滚到上一个稳定版本
helm rollback <release> <last-good-revision> -n <namespace> --wait --timeout=5m

# 确认回滚成功
helm status <release> -n <namespace>
kubectl get pods -n <namespace> -l app.kubernetes.io/instance=<release>
```

---

## 9. 升级路径

如果以下情况发生，立即升级：
- Release 卡在 pending-upgrade/pending-install 超过 10min → 升级到 SRE On-Call
- 回滚后仍无法恢复，疑似 etcd 中 Release Secret 损坏 → 升级到平台架构师
- 多个 Release 同时失败，疑似集群级问题 → 升级到基础设施团队

---

## 10. 相关链接

- FTA 故障树: [[19-故障诊断/06-FTA故障树/list/helm-fta.md|Helm FTA]]
- 相关概念: [[22-概念/12-研究/gitops-tool-evolution|GitOps 部署]]
- Helm 官方文档: https://helm.sh/docs/troubleshooting/

---

## 11. 生产案例

### 案例：helm upgrade 卡住导致 Release 不可操作

**背景**：某团队执行 `helm upgrade` 时网络中断，Release 卡在 `pending-upgrade` 状态，后续所有 helm 操作报错。

**修复**：
``` bash
# 🟡 中风险：手动修正 Release 状态
kubectl patch secret -n <ns> -l name=<release>,version=<rev> -p '{"metadata":{"labels":{"status":"failed"}}}'
# 然后执行回滚
helm rollback <release> <last-good-rev> -n <ns>
```

## 12. 面试要点

1. **Q: Helm Release 存储在什么地方？如何工作？**
   A: Helm 3 将 Release 信息存储为目标 namespace 中的 Secret（类型 `helm.sh/release.v1`），包含渲染后的 manifest、values、状态等。每次 upgrade 创建新版本 Secret，rollback 标记旧版本为 deployed。

2. **Q: helm upgrade --atomic 的作用是什么？**
   A: 如果 upgrade 失败，自动回滚到上一个成功版本。配合 `--timeout` 使用，避免 Release 卡在 pending 状态。生产环境强烈建议始终添加 `--atomic --timeout=5m`。

3. **Q: 如何处理 Helm hooks 失败？**
   A: hooks（如 pre-upgrade migration job）失败会阻止 upgrade 继续。检查 hook Pod 日志，修复后重新 upgrade。若 hook 资源残留，手动删除后重试。使用 `--no-hooks` 可跳过 hooks（仅紧急情况）。

*本 Skill 已补充完整，达到 GA 状态。*


<!-- risk-assessed -->
