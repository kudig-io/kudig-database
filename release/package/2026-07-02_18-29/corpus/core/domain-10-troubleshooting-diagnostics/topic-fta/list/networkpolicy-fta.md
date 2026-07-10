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
sources: ["domain-10-troubleshooting-diagnostics/topic-fta/list/networkpolicy-fta.md"]
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

## 相关链接

- [[skills/FTA Methodology and Core Principles.md|FTA 方法论]]
- [[skills/FTA Diagnostic Execution Engine.md|FTA 诊断执行引擎]]

## Related

- [[networkpolicy]] — NetworkPolicy
- [[cilium]] — Cilium
- [[cni]] — CNI (Container Network Interface)
- [[domain-19-landscape-references/领域索引/terway-index.md|Terway 知识图谱索引]]
- [[domain-19-landscape-references/领域索引/network-index.md|Network 网络知识图谱索引]]

---

> 📄 **完整版本**: 本文件为精简版故障树速查。完整的故障树分析（含详细根因推理和决策路径）请查阅：
> [[_archives/troubleshooting-diagnostics/FTA故障树/list/networkpolicy-fta.md|Networkpolicy FTA 完整版]]


<!-- risk-assessed -->
