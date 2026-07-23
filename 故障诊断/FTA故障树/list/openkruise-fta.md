---
title: OpenKruise 工作负载异常故障树分析 (skills)
description: '### 故障排查命令速查'
summary: '### 故障排查命令速查'
category: skills
tags:
- k8s
- fta
- troubleshooting
- job
- ingress
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- OpenKruise 工作负载异常故障树分析 是什么
- 如何 OpenKruise 工作负载异常故障树分析
trigger_keywords:
- OpenKruise
- 工作负载异常故障树分析
prerequisites:
- kubectl-basics
fta_id: FTA-OPENKRUISE-001
component: Openkruise
severity: high
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# OpenKruise 工作负载异常故障树分析

### 故障排查命令速查

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl label/annotate`：改元数据可能影响选择器/控制器

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 1. 检查 OpenKruise 组件状态
kubectl get pods -n kruise-system

# 2. 查看 CloneSet 列表
kubectl get cloneset -A

# 3. 查看 CloneSet 详情
kubectl describe cloneset <name> -n <namespace>

# 4. 查看 SidecarSet 列表
kubectl get sidecarset -A

# 5. 查看 SidecarSet 详情
kubectl describe sidecarset <name> -n <namespace>

# 6. 查看 PodUnavailableBudget
kubectl get pub -A

# 7. 查看镜像预热任务
kubectl get imagepulljob -A

# 8. 查看原地升级状态
kubectl get pod -n <namespace> -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{.metadata.labels.kruise\.io/workload-transition-mark\.version}{"\n"}{end}'

# 9. 测试 Sidecar 注入
kubectl debug -it <pod> -n <namespace> --image=<sidecar-image> -- /bin/sh

# 10. 手动触发原地升级
kubectl annotate pod <pod> -n <namespace> kruise.io/inplace-update-enabled="true"
```
---

## 生产案例

### 案例1: CloneSet 滚动更新卡住

**时间线**:
- 11:00 发布 CloneSet 新版本，partition 设为 50%
- 11:05 前 50% Pod 更新成功，但剩余 Pod 未继续更新
- 11:10 检查发现 kruise-controller-manager 日志报错: webhook 超时
- 11:15 确认根因: 自定义 Admission Webhook 服务不可用，阻塞了 Pod 创建
- 11:20 修复 Webhook 后更新继续

**根因链**:
```
CloneSet滚动更新 → 创建新Pod → Admission Webhook拦截
→ Webhook服务不可用(timeout) → Pod创建失败 → 更新卡住
```

**修复**:
```bash
# 🟢 检查 kruise-controller 状态
kubectl get pods -n kruise-system -l control-plane=controller-manager
kubectl logs -n kruise-system -l control-plane=controller-manager --tail=50 | grep -i error
# 🟡 检查并修复 Webhook
kubectl get validatingwebhookconfigurations | grep kruise
kubectl get mutatingwebhookconfigurations | grep kruise
```

### 案例2: DaemonSet 升级导致节点服务中断

**现象**: 使用 OpenKruise DaemonSet 滚动升级日志采集 Agent，部分节点采集中断超过 10 分钟

**根因**: surge 策略配置不当，同时升级节点过多，且新 Pod 启动慢(需加载大量配置)

**修复**:
```yaml
# 🟡 调整滚动策略
spec:
  updateStrategy:
    type: RollingUpdate
    rollingUpdate:
      maxUnavailable: 1
      partition: 0
```

## 预防与监控

### 告警规则

```yaml
groups:
- name: openkruise-alerts
  rules:
  - alert: KruiseControllerDown
    expr: up{job="kruise-controller-manager"} == 0
    for: 2m
    labels:
      severity: critical
  - alert: CloneSetUpdateStuck
    expr: kruise_cloneset_status_updated_replicas < kruise_cloneset_status_replicas
    for: 30m
    labels:
      severity: warning
```

### 预防措施

| 措施 | 说明 | 优先级 |
|------|------|--------|
| Webhook 高可用 | kruise webhook 至少 2 副本 | P0 |
| 分批发布 | partition 控制每批更新比例 | P0 |
| 回滚预案 | 保留旧版本镜像，快速回滚 | P1 |
| 资源预留 | 更新时预留足够资源给新 Pod | P1 |

## 面试要点

1. **Q: OpenKruise CloneSet 与原生 Deployment 的区别？**
   A: CloneSet 支持原地升级(in-place update)避免 Pod 重建；支持指定 Pod 删除；partition 控制更精细；支持流式扩容

2. **Q: OpenKruise 原地升级的原理？**
   A: 只更新容器镜像并重启容器，保持 Pod IP/挂载卷/节点不变；通过 CRI 接口重建容器而非删除 Pod

3. **Q: OpenKruise 更新卡住的排查思路？**
   A: 检查 kruise-controller 日志 → 验证 Webhook 可用性 → 确认资源是否充足 → 检查 partition/selector 配置 → 查看 Pod 事件

## 相关链接

- [[技能/fta-方法论/methodology/FTA Methodology and Core Principles.md|FTA 方法论]]
- [[技能/fta-方法论/execution-engine/FTA Diagnostic Execution Engine.md|[[FTA 诊断执行引擎|FTA 诊断执行引擎]]]]

## Related

- observability.md|ts-monitoring-observability]] — 监控可观测性排查
- [[技能/skill-k8s-node-notready-SKILL.md|SKILL]].md|skill-k8s-node-notready-SKILL]] — Skill
- [[kudig-prompts-catalog]] — KUDIG Prompt 模板集：故障排查、架构评审、配置生成与学习路径
- [[技能/learn-decision-tree-mermaid.md|learn-decision-tree-mermaid]] — 故障排查决策树 - Mermaid 可视化版
- [[openkruise]] — OpenKruise

- [[nginx-ingress-fta]]
- [[故障诊断/FTA故障树/list/openkruise-fta.md|OpenKruise 工作负载异常故障树分析]]
- [[技能/learn-05-ingress-basics.md|第五课：Ingress - 外部 HTTP/HTTPS 访问]] — Cross-reference
- [[生态参考/领域索引/openkruise-index.md|OpenKruise 全局索引]]


<!-- risk-assessed -->
