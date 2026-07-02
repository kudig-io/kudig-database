---
title: Gateway API 异常故障树分析 (skills)
description: '| evt_config_error | 配置参数错误 | `kubectl logs -n ${GW_NS} -l app=${CONTROLLER_LABEL}
  --tail=50 | grep -i error` | Controller 日志 | 配置错误信息 |'
summary: '| evt_config_error | 配置参数错误 | `kubectl logs -n ${GW_NS} -l app=${CONTROLLER_LABEL}
  --tail=50 | grep -i error` | Controller 日志 | 配置错误信息 |'
category: general
tags:
- k8s
- etcd
- flannel
- argocd
- gateway
- rbac
- gpu
tier: supporting
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Gateway API 异常故障树分析 是什么
- 如何 Gateway API 异常故障树分析
trigger_keywords:
- Gateway
- API
- 异常故障树分析
prerequisites:
- kubectl-basics
- gitops-basics
- etcd-basics
- gpu-scheduling-basics
fta_id: FTA-GATEWAY_API-001
component: Gateway Api
severity: critical
---



---
title: "Gateway API 异常故障树分析"
category: skills
summary: "<!-- condition: kubectl get gateway,httproute -A -o jsonpath='{range .items[?(@.status.conditions[?(@.type!=\'Ready\' && @.status!=\'Accepted\')])]} {.kind}/{.metadata.namespace..."
tags: ["k8s", "fta", "troubleshooting"]
sources: ["domain-10-troubleshooting-diagnostics/topic-fta/list/gateway-api-fta.md"]
created: 2026-05-21
updated: 2026-05-21
lifecycle: reviewed
lifecycle_changed: "2026-05-21"
tier: supporting
base_confidence: 0.7
---

# Gateway API 异常故障树分析

### 诊断命令快速参考

### 1. Gateway Controller 诊断命令

| 节点 ID | 名称 | 诊断命令 | 预期输出模式 | 判定 |
|---------|------|----------|--------------|------|
| evt_ctrl_crashloop | Pod CrashLoop | `kubectl get pods -n ${GW_NS} -l app=${CONTROLLER_LABEL} -o wide` | Pod 状态 | CrashLoopBackOff 表示崩溃循环 |
| evt_ctrl_resource | 资源不足 | `kubectl top pod -n ${GW_NS} -l app=${CONTROLLER_LABEL}` | CPU/内存使用量 | 接近 limits 表示资源不足 |
| evt_ctrl_image | 镜像拉取失败 | `kubectl describe pod -n ${GW_NS} -l app=${CONTROLLER_LABEL} | grep -A5 'Events:'` | 事件信息 | ImagePullBackOff 表示镜像问题 |
| evt_rbac_insufficient | RBAC 权限不足 | `kubectl auth can-i --list --as=system:serviceaccount:${GW_NS}:${SA_NAME}` | 权限列表 | 检查必要权限是否存在 |
| evt_config_error | 配置参数错误 | `kubectl logs -n ${GW_NS} -l app=${CONTROLLER_LABEL} --tail=50 | grep -i error` | Controller 日志 | 配置错误信息 |
| evt_gwclass_notexist | GatewayClass 不存在 | `kubectl get gatewayclass ${GWCLASS_NAME} -o wide 2>&1` | GatewayClass 状态 | NotFound 表示不存在 |
| evt_gwclass_notready | GatewayClass 未就绪 | `kubectl get gatewayclass ${GWCLASS_NAME} -o jsonpath='{.status.conditions}'` | 条件状态 | Accepted=False 表示未就绪 |

### 2. 路由配置诊断命令

| 节点 ID | 名称 | 诊断命令 | 预期输出模式 | 判定 |
|---------|------|----------|--------------|------|
| evt_hostname_mismatch | hostnames 不匹配 | `kubectl get httproute ${ROUTE_NAME} -n ${NAMESPACE} -o jsonpath='{.spec.hostnames}'` | hostname 列表 | 检查是否包含目标域名 |
| evt_path_error | path 规则错误 | `kubectl get httproute ${ROUTE_NAME} -n ${NAMESPACE} -o jsonpath='{.spec.rules[*].matches}'` | 匹配规则 | 检查 path 配置是否正确 |
| evt_header_fail | headers 匹配失败 | `kubectl get httproute ${ROUTE_NAME} -n ${NAMESPACE} -o yaml | grep -A10 'headers:'` | header 匹配规则 | 检查 header 配置 |
| evt_parent_notexist | parentRef Gateway 不存在 | `kubectl get gateway ${GW_NAME} -n ${GW_NS} 2>&1` | Gateway 状态 | NotFound 表示不存在 |
| evt_backend_notexist | backendRef Service 不存在 | `kubectl get svc ${SVC_NAME} -n ${NAMESPACE} 2>&1` | Service 状态 | NotFound 表示不存在 |
| evt_route_not_accepted | Route 未被 Accepted | `kubectl get httproute ${ROUTE_NA
...(截断)

## 相关链接

- [[skills/FTA Methodology and Core Principles.md|FTA 方法论]]
- [[skills/FTA Diagnostic Execution Engine.md|FTA 诊断执行引擎]]

## See Also

- [[skills/etcd-fta.md|etcd-fta]]
- [[skills/flannel-fta.md|flannel-fta]]
- [[skills/gitops-argocd-fta.md|gitops-argocd-fta]]
- [[skills/gpu-fta.md|gpu-fta]]

---

> 📄 **完整版本**: 本文件为精简版故障树速查。完整的故障树分析（含详细根因推理和决策路径）请查阅：
> [[_archives/troubleshooting-diagnostics/topic-fta/list/gateway-api-fta.md|Gateway-Api FTA 完整版]]
