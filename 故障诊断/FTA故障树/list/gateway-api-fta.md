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

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




---
title: "Gateway API 异常故障树分析"
category: skills
summary: "<!-- condition: kubectl get gateway,httproute -A -o jsonpath='{range .items[?(@.status.conditions[?(@.type!=\'Ready\' && @.status!=\'Accepted\')])]} {.kind}/{.metadata.namespace..."
tags: ["k8s", "fta", "troubleshooting"]
sources: ["故障诊断/topic-fta/list/gateway-api-fta.md"]
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

## 生产案例

### 案例1: HTTPRoute 未被 Accepted - GatewayClass 未就绪

**时间线**:
- 14:00 创建 HTTPRoute 绑定到 Gateway
- 14:02 HTTPRoute 状态显示 `Accepted: False`，原因 `GatewayClassNotReady`
- 14:05 确认根因: GatewayClass 的 controllerName 与实际安装的控制器不匹配
- 14:10 修正 controllerName 后 Route 被接受

**根因链**:
```
HTTPRoute引用Gateway → Gateway引用GatewayClass
→ GatewayClass controllerName不匹配 → 无控制器处理
→ Route未被Accepted → 流量无法路由
```

**修复**:
```bash
# 🟢 检查 GatewayClass 状态
kubectl get gatewayclass -o wide
kubectl describe gatewayclass ${GC_NAME} | grep -A5 "Conditions"
# 🟢 检查 HTTPRoute 状态
kubectl get httproute ${ROUTE} -n ${NS} -o jsonpath='{.status.conditions}' | jq .
# 🟡 修正 controllerName
kubectl patch gatewayclass ${GC_NAME} -p '{"spec":{"controllerName":"gateway.envoyproxy.io/gatewayclass-controller"}}'
```

### 案例2: Gateway Listener 端口冲突

**现象**: Gateway 创建成功但 Listener 状态 `Conflicted`，流量无法进入

**根因**: 同一节点上多个 Gateway 使用相同端口，且未配置端口共享

**修复**:
```bash
# 🟢 检查 Gateway Listener 状态
kubectl get gateway ${GW} -n ${NS} -o jsonpath='{.status.listeners}' | jq .
# 🟡 调整端口或合并 Gateway
kubectl patch gateway ${GW} -n ${NS} --type=merge -p '{"spec":{"listeners":[{"name":"https","port":8443,"protocol":"HTTPS"}]}}'
```

## 预防与监控

### 告警规则

```yaml
groups:
- name: gateway-api-alerts
  rules:
  - alert: GatewayNotProgrammed
    expr: kube_gateway_status_condition{type="Programmed",status="True"} == 0
    for: 5m
    labels:
      severity: critical
  - alert: HTTPRouteNotAccepted
    expr: kube_httproute_status_condition{type="Accepted",status="True"} == 0
    for: 10m
    labels:
      severity: warning
```

### 预防措施

| 措施 | 说明 | 优先级 |
|------|------|--------|
| GatewayClass 验证 | 部署前确认 controllerName 正确 | P0 |
| 端口规划 | 避免多 Gateway 端口冲突 | P0 |
| RBAC 配置 | 确保控制器有足够权限 | P1 |
| 状态监控 | 监控 Gateway/Route 状态变化 | P1 |

## 面试要点

1. **Q: Gateway API 的核心资源关系？**
   A: GatewayClass → Gateway → HTTPRoute/TCPRoute；角色分离: 平台管理员管 GatewayClass，集群管理员管 Gateway，开发者管 Route

2. **Q: HTTPRoute 不被接受的排查步骤？**
   A: 检查 GatewayClass 状态 → 确认 Gateway Listener 配置 → 验证 Route parentRefs → 检查 RBAC 权限 → 查看控制器日志

3. **Q: Gateway API 相比 Ingress 的优势？**
   A: 支持多协议(HTTP/TCP/UDP/gRPC) → 角色分离 → 跨命名空间引用 → 丰富的流量管理(镜像/重试/超时) → 可扩展的 Policy Attachment

## 相关链接

- [[技能/fta-方法论/methodology/FTA Methodology and Core Principles.md|FTA 方法论]]
- [[技能/fta-方法论/execution-engine/FTA Diagnostic Execution Engine.md|FTA 诊断执行引擎]]

## See Also

- [[技能/etcd-fta.md|etcd-fta]]
- [[技能/flannel-fta.md|flannel-fta]]
- [[技能/gitops-argocd-fta.md|gitops-argocd-fta]]
- [[技能/gpu-fta.md|gpu-fta]]

---

> 📄 **完整版本**: 本文件为精简版故障树速查。完整的故障树分析（含详细根因推理和决策路径）请查阅：
> [[归档/troubleshooting-diagnostics/FTA故障树/list/gateway-api-fta.md|Gateway-Api FTA 完整版]]


<!-- risk-assessed -->
