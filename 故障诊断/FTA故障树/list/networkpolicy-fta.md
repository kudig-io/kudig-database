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
sources: ["故障诊断/topic-fta/list/networkpolicy-fta.md"]
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

### 案例1: NetworkPolicy 误配置导致服务间调用失败

**时间线**:
- 15:00 安全团队应用默认拒绝策略(default-deny-all)
- 15:02 多个微服务间调用失败，返回 connection refused
- 15:05 确认根因: default-deny 未配套放行策略
- 15:10 紧急添加允许策略，服务恢复

**根因链**:
```
应用default-deny-all → 所有Pod入站流量被拒绝
→ 未配套允许策略 → 服务间调用失败
```

**修复**:
```bash
# 🟢 查看当前 NetworkPolicy
kubectl get networkpolicy -n ${NS} -o wide
# 🟡 添加允许策略
kubectl apply -f - <<EOF
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: allow-internal
  namespace: ${NS}
spec:
  podSelector: {}
  policyTypes: [Ingress]
  ingress:
  - from:
    - namespaceSelector:
        matchLabels:
          name: ${NS}
EOF
```

### 案例2: CNI 不支持 NetworkPolicy 导致策略无效

**现象**: 应用了 NetworkPolicy 但流量未被拦截，策略似乎不生效

**根因**: 集群使用 Flannel CNI，不支持 NetworkPolicy 执行

**修复**:
```bash
# 🟢 确认 CNI 类型
kubectl get pods -n kube-system | grep -E "calico|cilium|weave|flannel"
# 解决方案: 安装 Calico Policy 组件或迁移到 Cilium
# 🟡 安装 calico-policy (与 Flannel 共存)
kubectl apply -f calico-policy-only.yaml
```

## 预防与监控

### 告警规则

```yaml
groups:
- name: networkpolicy-alerts
  rules:
  - alert: NetworkPolicyBlockingTraffic
    expr: rate(cilium_drop_count_total{direction="INGRESS", reason="POLICY_DENIED"}[5m]) > 10
    for: 5m
    labels:
      severity: warning
  - alert: DefaultDenyWithoutAllow
    expr: count(kube_networkpolicy_spec_ingress_default_deny) > 0 unless count(kube_networkpolicy_spec_ingress_allow) > 0
    for: 10m
    labels:
      severity: critical
```

### 预防措施

| 措施 | 说明 | 优先级 |
|------|------|--------|
| 策略变更审批 | NetworkPolicy 变更需安全+业务双方确认 | P0 |
| 先审计后执行 | 先用 Cilium audit 模式观察再强制 | P0 |
| CNI 能力确认 | 确保 CNI 支持 NetworkPolicy 执行 | P0 |
| 回滚预案 | 变更前备份现有策略 | P1 |

## 面试要点

1. **Q: NetworkPolicy 不生效的常见原因？**
   A: CNI 不支持(Flannel) → 策略选择器不匹配 → policyTypes 未正确指定 → 命名空间标签缺失 → 策略同步延迟

2. **Q: 如何安全地实施 default-deny？**
   A: 先审计模式观察流量 → 梳理服务依赖关系 → 配套允许策略 → 分批应用 → 监控拒绝事件

3. **Q: Calico 与 Cilium 实现 NetworkPolicy 的差异？**
   A: Calico 用 iptables/eBPF + BGP；Cilium 纯 eBPF 性能更优；Cilium 支持 L7 策略和 FQDN 过滤；Calico 生态更成熟

## 相关链接

- [[技能/fta-方法论/methodology/FTA Methodology and Core Principles.md|FTA 方法论]]
- [[技能/fta-方法论/execution-engine/FTA Diagnostic Execution Engine.md|FTA 诊断执行引擎]]

## Related

- [[networkpolicy]] — NetworkPolicy
- [[cilium]] — Cilium
- [[cni]] — CNI (Container Network Interface)
- [[生态参考/领域索引/terway-index.md|Terway 知识图谱索引]]
- [[生态参考/领域索引/network-index.md|Network 网络知识图谱索引]]

---

> 📄 **完整版本**: 本文件为精简版故障树速查。完整的故障树分析（含详细根因推理和决策路径）请查阅：
> [[归档/troubleshooting-diagnostics/FTA故障树/list/networkpolicy-fta.md|Networkpolicy FTA 完整版]]


<!-- risk-assessed -->
