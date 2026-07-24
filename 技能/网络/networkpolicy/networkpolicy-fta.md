---
title: NetworkPolicy 异常故障树分析 (skills)
description: 'summary: "<!-- condition: kubectl get networkpolicy -A -o jsonpath=''{range
  .items[?(@.spec.policyTypes!=null)]} {.metadata.namespace}/{.metadata.name}{\''\n\''}{end}''
  显示有策略但存在网络不通问题 --> - *..."'
summary: 'summary: "<!-- condition: kubectl get networkpolicy -A -o jsonpath=''{range
  .items[?(@.spec.policyTypes!=null)]} {.metadata.namespace}/{.metadata.name}{\''\n\''}{end}''
  显示有策略但存在网络不通问题 --> - *..."'
category: general
tags:
- k8s
- cilium
- calico
- ingress
- networkpolicy
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- NetworkPolicy 异常故障树分析 是什么
- 如何 NetworkPolicy 异常故障树分析
trigger_keywords:
- NetworkPolicy
- 异常故障树分析
prerequisites:
- kubectl-basics
- cilium-basics
- cni-basics
fta_id: FTA-NETWORKPOLICY-001
component: Networkpolicy
severity: high
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




---
title: "NetworkPolicy 异常故障树分析"
category: skills
summary: "<!-- condition: kubectl get networkpolicy -A -o jsonpath='{range .items[?(@.spec.policyTypes!=null)]} {.metadata.namespace}/{.metadata.name}{\'\n\'}{end}' 显示有策略但存在网络不通问题 --> - *..."
tags: ["k8s", "fta", "troubleshooting"]
sources: ["故障诊断/FTA故障树/list/networkpolicy-fta.md"]
created: 2026-05-21
updated: 2026-05-21
lifecycle: reviewed
lifecycle_changed: "2026-05-21"
tier: supporting
base_confidence: 0.7
---

# NetworkPolicy 异常故障树分析

### 诊断命令快速参考

### 1. 策略配置诊断命令

| 节点 ID | 名称 | 诊断命令 | 预期输出模式 | 判定 |
|---------|------|----------|--------------|------|
| evt_podselector_error | podSelector 选择错误 | `kubectl get networkpolicy ${NP_NAME} -n ${NAMESPACE} -o jsonpath='{.spec.podSelector}'` | 返回 podSelector 配置 | 检查 selector 是否匹配目标 Pod 标签 |
| evt_nsselector_error | namespaceSelector 错误 | `kubectl get networkpolicy ${NP_NAME} -n ${NAMESPACE} -o jsonpath='{.spec.[[Ingress|ingress]][*].from[*].namespaceSelector}'` | 返回 namespaceSelector 配置 | 检查是否正确选择源命名空间 |
| evt_label_mismatch | 标签不匹配 | `kubectl get pods -n ${NAMESPACE} -l "${LABEL_SELECTOR}" --show-labels` | 列出匹配的 Pod | 无输出表示标签不匹配 |
| evt_ingress_missing | 入站规则缺失 | `kubectl get networkpolicy ${NP_NAME} -n ${NAMESPACE} -o jsonpath='{.spec.ingress}'` | 返回 ingress 规则 | null/空表示缺失入站规则 |
| evt_egress_missing | 出站规则缺失 | `kubectl get networkpolicy ${NP_NAME} -n ${NAMESPACE} -o jsonpath='{.spec.egress}'` | 返回 egress 规则 | null/空表示缺失出站规则 |
| evt_rule_logic_error | 规则逻辑错误 | `kubectl describe networkpolicy ${NP_NAME} -n ${NAMESPACE}` | 显示完整策略详情 | 审查规则逻辑是否正确 |
| evt_port_number_error | 端口号错误 | `kubectl get networkpolicy ${NP_NAME} -n ${NAMESPACE} -o jsonpath='{.spec..ports}'` | 返回端口配置 | 检查端口号是否正确 |
| evt_protocol_error | 协议类型错误 | `kubectl get networkpolicy ${NP_NAME} -n ${NAMESPACE} -o jsonpath='{.spec..ports[*].protocol}'` | 返回协议配置 | 检查协议类型是否正确 |

### 2. CNI 实现诊断命令

| 节点 ID | 名称 | 诊断命令 | 预期输出模式 | 判定 |
|---------|------|----------|--------------|------|
| evt_cni_not_support | CNI 不支持 NetworkPolicy | `kubectl get pods -n kube-system -l k8s-app=calico-node -o name || kubectl get pods -n kube-system -l k8s-app=cilium -o name` | 返回 CNI Pod 名称 | 无输出表示可能使用不支持策略的 CNI |
| evt_policy_mode_disabled | 策略模式未启用 | `kubectl get configmap -n kube-system calico-config -o jsonpath='{.data.cni_network_config}' 2>/dev/null || kubectl get configmap -n kube-system cilium-config -o yaml` | 返回 CNI 配置 | 检查 policy 相关配置是否启用 |
| evt_sync_delay | 策略同步延迟 | `kubectl get networkpolicy ${NP_NAME} -n ${NAMESPACE} -o jsonpa
...(截断)

## 生产案例

### 案例 1: 默认拒绝策略导致服务间通信中断

| 时间 | 事件 |
|------|------|
| 13:00 | 安全团队应用 default-deny NetworkPolicy 到所有 namespace |
| 13:01 | 微服务间调用全部失败，业务 503 |
| 13:05 | `kubectl get networkpolicy -A` 发现 default-deny-all |
| 13:10 | 🟡 添加允许服务间通信的 ingress 规则 |
| 13:15 | 服务恢复 |

**根因**: 应用 default-deny 前未梳理服务间依赖关系，未提前配置白名单规则。

### 案例 2: CNI 不支持 NetworkPolicy 导致策略无效

**现象**: 配置了 NetworkPolicy 但流量未被拦截。

**诊断**: 集群使用 Flannel(不支持 NetworkPolicy)，需 Calico/Cilium

**修复**: 🟡 部署 Calico 或 Cilium 作为 NetworkPolicy 执行引擎

## 升级决策点

| 级别 | 条件 | 动作 |
|------|------|------|
| P0 | 策略导致核心业务中断 | 立即删除问题 NetworkPolicy |
| P1 | 部分服务不可达 | 检查策略规则和 Pod label |
| P2 | 策略未生效 | 确认 CNI 支持 NetworkPolicy |

## 面试要点

1. **Q: NetworkPolicy 的默认行为是什么？**
   A: 默认无任何 NetworkPolicy 时，所有 Pod 间流量允许。一旦某 Pod 被任何 NetworkPolicy 的 podSelector 匹配，则该 Pod 变为“白名单模式”，只允许策略明确允许的流量。

2. **Q: 哪些 CNI 支持 NetworkPolicy？**
   A: Calico(完整支持)、Cilium(支持+L7)、Weave Net(支持)；Flannel(不支持，需配合 Calico policy-only 模式)、AWS VPC CNI(需 Calico 插件)。

3. **Q: 如何安全地实施 default-deny？**
   A: ① 先梳理服务依赖图 ② 在测试环境验证 ③ 按 namespace 分批实施 ④ 先应用 deny + 立即应用已知白名单 ⑤ 监控拒绝日志。

## 相关链接

- [[技能/工作负载/pod/方法论/FTA Methodology and Core Principles.md|FTA 方法论]]
- [[技能/工作负载/pod/方法论/FTA Diagnostic Execution Engine.md|FTA 诊断执行引擎]]

## Related

- [[networkpolicy]] — NetworkPolicy
- [[cilium]] — Cilium
- [[cni]] — CNI (Container Network Interface)
- [[生态参考/领域索引/terway-index.md|Terway 知识图谱索引]]
- [[生态参考/领域索引/network-index.md|Network 网络知识图谱索引]]


<!-- risk-assessed -->
