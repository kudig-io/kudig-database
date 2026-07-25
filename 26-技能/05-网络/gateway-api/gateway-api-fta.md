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
sources: ["故障诊断/FTA故障树/list/gateway-api-fta.md"]
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

### 案例 1: GatewayClass 未就绪导致 Gateway 不生效

| 时间 | 事件 |
|------|------|
| 09:00 | 创建 Gateway 资源但无外部 IP 分配 |
| 09:05 | `kubectl get gatewayclass` 显示 Accepted=False |
| 09:08 | Gateway Controller 未安装或版本不兼容 |
| 09:12 | 🟡 安装/升级 Gateway Controller(如 Envoy Gateway) |

**根因**: Gateway API 需要对应的 Controller 实现，未安装或版本不匹配。

### 案例 2: HTTPRoute 规则冲突导致流量路由错误

**现象**: 多个 HTTPRoute 匹配相同路径，流量被路由到错误后端。

**诊断**: `kubectl get httproute -o yaml` 检查规则优先级

**修复**: 🟢 调整 path 匹配精确度(Exact > PathPrefix)或设置不同 hostname

## 升级决策点

| 级别 | 条件 | 动作 |
|------|------|------|
| P0 | Gateway 完全不可用 | 检查 Controller Pod + GatewayClass |
| P1 | 部分路由规则失败 | 检查 HTTPRoute 配置 |
| P2 | 性能优化 | 调整路由规则精确度 |

## 面试要点

1. **Q: Gateway API 的角色分离模型？**
   A: ① Infrastructure Provider: 管理 GatewayClass ② Cluster Operator: 创建 Gateway 和监听器 ③ Application Developer: 创建 HTTPRoute 绑定到 Gateway。通过 ReferenceGrant 实现跨 namespace 引用。

2. **Q: Gateway API 相比 Ingress 的优势？**
   A: ① 原生支持 TCP/UDP/gRPC/TLS ② 角色分离，权限更清晰 ③ 原生流量分割(权重) ④ Header/Query 匹配 ⑤ 可扩展的 Policy Attachment ⑥ 无需 annotations。

3. **Q: Gateway API 的流量路由流程？**
   A: Client → Gateway(Listener) → HTTPRoute(规则匹配) → BackendRef(Service) → Pod。Controller watch Gateway+HTTPRoute 变更，转换为数据平面配置(如 Envoy xDS)。

## 相关链接

- [[26-技能/04-工作负载/pod/方法论/FTA Methodology and Core Principles.md|FTA 方法论]]
- [[26-技能/04-工作负载/pod/方法论/FTA Diagnostic Execution Engine.md|FTA 诊断执行引擎]]

## See Also

- [[26-技能/02-控制面/etcd/etcd-fta.md|etcd-fta]]
- [[26-技能/05-网络/cni/flannel-fta.md|flannel-fta]]
- [[26-技能/01-集群运维/gitops-argocd/gitops-argocd-fta.md|gitops-argocd-fta]]
- [[26-技能/03-节点/gpu/gpu-fta.md|gpu-fta]]


<!-- risk-assessed -->
