---
title: OpenKruise 工作负载异常故障树分析 (skills)
description: '### 故障排查命令速查'
category: skills
tags:
- k8s
- fta
- troubleshooting
- job
- ingress
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
created: "2026-05-23"
---

# OpenKruise 工作负载异常故障树分析

### 故障排查命令速查

```bash
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

## 相关链接

- [[skills/FTA Methodology and Core Principles|FTA 方法论]]
- [[skills/FTA Diagnostic Execution Engine|[[FTA 诊断执行引擎|FTA 诊断执行引擎]]]]

## Related

- observability.md|ts-monitoring-observability]] — 监控可观测性排查
- [[skills/skill-k8s-node-notready-SKILL|SKILL]].md|skill-k8s-node-notready-SKILL]] — Skill
- [[kudig-prompts-catalog]] — KUDIG Prompt 模板集：故障排查、架构评审、配置生成与学习路径
- [[skills/learn-decision-tree-mermaid|learn-decision-tree-mermaid]] — 故障排查决策树 - Mermaid 可视化版
- [[openkruise]] — OpenKruise

- [[nginx-ingress-fta]]
- [[domain-10-troubleshooting-diagnostics/topic-fta/list/openkruise-fta|OpenKruise 工作负载异常故障树分析]]
- [[skills/learn-05-ingress-basics|第五课：Ingress - 外部 HTTP/HTTPS 访问]] — Cross-reference
- [[domain-19-landscape-references/topic-index/openkruise-index|OpenKruise 全局索引]]
