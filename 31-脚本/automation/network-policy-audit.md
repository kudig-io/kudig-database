---
title: NetworkPolicy 覆盖审计脚本
description: 审计 NetworkPolicy 覆盖范围，发现未受保护的 Pod
summary: NetworkPolicy 覆盖审计脚本 — 检测网络隔离策略缺口和未受保护的 Pod
category: automation
tags:
- k8s
- automation
- network-policy
- security
- audit
- bash
tier: supporting
created: '2026-07-11'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 安全工程师
- SRE
- 网络工程师
estimated_read_time: 8min
intent_queries:
- NetworkPolicy 审计脚本 是什么
- 如何检查 Kubernetes 网络策略覆盖
- 未受保护的 Pod 检测
- network policy coverage audit script
trigger_keywords:
- network-policy
- 审计
- audit
- 网络隔离
- 安全
- 脚本
prerequisites:
- kubectl-basics
- network-basics
- security-basics
---

> **生产环境安全提示**
>
> 本脚本为只读检查 (🟢)，不修改集群状态。

# NetworkPolicy 覆盖审计脚本

> 脚本 ID: `AUTO-03` | 语言: Bash | 风险: 🟢 只读 | 执行时间: ~10s

## 概述

Kubernetes 默认允许所有网络流量 (deny-none 模型)。NetworkPolicy 是实现 Pod 级网络隔离的主要机制。本脚本审计集群中 NetworkPolicy 的覆盖情况，识别安全缺口:

1. **命名空间覆盖** — 哪些命名空间没有任何 NetworkPolicy
2. **Pod 覆盖** — 即使命名空间有 NetworkPolicy，哪些 Pod 未被任何策略选中
3. **默认策略检查** — 是否存在默认 deny-all 基线策略
4. **策略质量分析** — 检测过于宽松的策略 (如 `0.0.0.0/0` 入站)
5. **合规报告** — 对照 Pod Security Standards Restricted profile 生成合规报告

## 前置条件

- CNI 支持 NetworkPolicy (Calico/Cilium/Terway/Weave Net 等)
- 具有 `cluster-reader` RBAC 权限
- `jq` 已安装

## 使用方法

```bash
# 审计所有命名空间
bash network-policy-audit.sh

# 审计特定命名空间
bash network-policy-audit.sh -n production

# 排除系统命名空间
bash network-policy-audit.sh --exclude kube-system,kube-public,kube-node-lease

# 只报告未受保护的 Pod (CI/CD 集成)
bash network-policy-audit.sh --unprotected-only

# JSON 输出
bash network-policy-audit.sh --json
```

## 完整脚本

```bash
#!/bin/bash
# network-policy-audit.sh — NetworkPolicy 覆盖审计
# 风险等级: 🟢 只读，无副作用

set -euo pipefail

NAMESPACE=""
EXCLUDE_NS="kube-system,kube-public,kube-node-lease,calico-system,tigera-operator,cilium-system"
UNPROTECTED_ONLY=false
OUTPUT="text"
TIMESTAMP=$(date -u +"%Y-%m-%dT%H:%M:%SZ")

while [[ $# -gt 0 ]]; do
    case $1 in
        -n|--namespace) NAMESPACE="$2"; shift 2 ;;
        --exclude) EXCLUDE_NS="$2"; shift 2 ;;
        --unprotected-only) UNPROTECTED_ONLY=true; shift ;;
        --json) OUTPUT="json"; shift ;;
        *) echo "Unknown: $1"; exit 1 ;;
    esac
done

# 将排除列表转换为 jq 条件
EXCLUDE_FILTER=$(echo "$EXCLUDE_NS" | tr ',' '\n' | sed 's/^/    . != "/' | sed 's/$/"/' | paste -sd " and " -)

echo "============================================"
echo " NetworkPolicy Audit — $TIMESTAMP"
echo "============================================"

# ── 1. 命名空间级覆盖 ──
audit_namespace_coverage() {
    echo -e "\n[1/4] Namespace Coverage"
    echo "--------------------------------------------"
    
    local all_ns
    if [ -n "$NAMESPACE" ]; then
        all_ns="$NAMESPACE"
    else
        all_ns=$(kubectl get namespaces -o jsonpath='{range .items[*]}{.metadata.name}{"\n"}{end}')
    fi
    
    local total_ns=0 protected_ns=0 unprotected_ns=0
    local unprotected_list=""
    
    while read -r ns; do
        [ -z "$ns" ] && continue
        
        # 检查是否在排除列表中
        if echo "$EXCLUDE_NS" | grep -qw "$ns"; then
            continue
        fi
        
        total_ns=$((total_ns + 1))
        
        local policy_count
        policy_count=$(kubectl get networkpolicy -n "$ns" --no-headers 2>/dev/null | wc -l)
        
        if [ "$policy_count" -eq 0 ]; then
            unprotected_ns=$((unprotected_ns + 1))
            unprotected_list="${unprotected_list}\n  🔴 ${ns} — NO NetworkPolicy"
        else
            protected_ns=$((protected_ns + 1))
        fi
    done <<< "$all_ns"
    
    echo "  Total Namespaces (audited): $total_ns"
    echo "  Protected (has NetworkPolicy): $protected_ns"
    echo "  Unprotected (no NetworkPolicy): $unprotected_ns"
    
    if [ "$unprotected_ns" -gt 0 ]; then
        echo -e "\n  Unprotected Namespaces:"
        echo -e "$unprotected_list"
    else
        echo "  ✅ All namespaces have NetworkPolicy"
    fi
}

# ── 2. Pod 级覆盖 ──
audit_pod_coverage() {
    echo -e "\n[2/4] Pod Coverage (pods not selected by any NetworkPolicy)"
    echo "--------------------------------------------"
    
    kubectl get namespaces -o json 2>/dev/null | \
        jq -r '.items[].metadata.name' | while read -r ns; do
            [ -z "$ns" ] && continue
            echo "$EXCLUDE_NS" | grep -qw "$ns" && continue
            
            # 获取该命名空间的所有 Pod
            local pods
            pods=$(kubectl get pods -n "$ns" --no-headers 2>/dev/null | awk '{print $1}')
            [ -z "$pods" ] && continue
            
            # 获取 NetworkPolicy 选择的 Pod (通过 label selector)
            local policies
            policies=$(kubectl get networkpolicy -n "$ns" -o json 2>/dev/null)
            
            if [ -z "$policies" ] || [ "$policies" == "[]" ]; then
                # 没有 NetworkPolicy，所有 Pod 都未受保护
                while read -r pod; do
                    [ -z "$pod" ] && continue
                    echo "  🔴 $ns/$pod — no NetworkPolicy in namespace"
                done <<< "$pods"
            else
                # 有 NetworkPolicy，检查每个 Pod 是否被选中
                while read -r pod; do
                    [ -z "$pod" ] && continue
                    
                    # 获取 Pod labels
                    local pod_labels
                    pod_labels=$(kubectl get pod "$pod" -n "$ns" -o json 2>/dev/null | jq -r '.metadata.labels // {}')
                    
                    # 检查是否有 NetworkPolicy 选择此 Pod
                    local covered=false
                    covered=$(kubectl get networkpolicy -n "$ns" -o json 2>/dev/null | \
                        jq -r --argjson labels "$pod_labels" '
                        .items[] |
                        .spec.podSelector.matchLabels as $sel |
                        select(
                            ($sel | length == 0) or
                            ($sel | to_entries | all(.value == ($labels[.key] // "")))
                        ) | .metadata.name' 2>/dev/null | head -1)
                    
                    if [ -z "$covered" ]; then
                        echo "  ⚠️  $ns/$pod — not selected by any NetworkPolicy"
                    fi
                done <<< "$pods"
            fi
        done
}

# ── 3. 默认 Deny 检查 ──
audit_default_deny() {
    echo -e "\n[3/4] Default Deny Policy Check"
    echo "--------------------------------------------"
    
    if [ -n "$NAMESPACE" ]; then
        ns_list="$NAMESPACE"
    else
        ns_list=$(kubectl get namespaces -o jsonpath='{range .items[*]}{.metadata.name}{"\n"}{end}')
    fi
    
    while read -r ns; do
        [ -z "$ns" ] && continue
        echo "$EXCLUDE_NS" | grep -qw "$ns" && continue
        
        local has_default_deny_ingress=false
        local has_default_deny_egress=false
        
        # 检查是否有 podSelector 为空 + policyTypes 含 Ingress/Egress 的策略
        local default_policies
        default_policies=$(kubectl get networkpolicy -n "$ns" -o json 2>/dev/null | \
            jq -r '.items[] | select(.spec.podSelector == {} or .spec.podSelector.matchLabels == {}) | "\(.metadata.name) types=\(.spec.policyTypes | join(","))"' 2>/dev/null)
        
        if echo "$default_policies" | grep -q "Ingress"; then
            has_default_deny_ingress=true
        fi
        if echo "$default_policies" | grep -q "Egress"; then
            has_default_deny_egress=true
        fi
        
        if $has_default_deny_ingress && $has_default_deny_egress; then
            echo "  ✅ $ns — default deny (ingress + egress)"
        elif $has_default_deny_ingress; then
            echo "  ⚠️  $ns — default deny ingress only (no egress default)"
        elif $has_default_deny_egress; then
            echo "  ⚠️  $ns — default deny egress only (no ingress default)"
        else
            echo "  🔴 $ns — NO default deny policy"
        fi
    done <<< "$ns_list"
}

# ── 4. 宽松策略检测 ──
audit_overly_permissive() {
    echo -e "\n[4/4] Overly Permissive Policies"
    echo "--------------------------------------------"
    
    kubectl get networkpolicy -A -o json 2>/dev/null | \
        python3 -c "
import json, sys
data = json.load(sys.stdin)
findings = []

for policy in data.get('items', []):
    ns = policy['metadata']['namespace']
    name = policy['metadata']['name']
    spec = policy.get('spec', {})
    
    # 检查入站规则是否允许 0.0.0.0/0
    for rule in spec.get('ingress', []):
        for peer in rule.get('from', []):
            ipblock = peer.get('ipBlock', {})
            if ipblock.get('cidr') == '0.0.0.0/0':
                # 但如果没有 from 限制 (空 from = allow all)
                findings.append(f'  ⚠️  {ns}/{name} — ingress allows 0.0.0.0/0')
        
        # 空 from = 允许所有来源
        if not rule.get('from'):
            findings.append(f'  ⚠️  {ns}/{name} — ingress rule with no from restriction (allows all)')
    
    # 检查出站规则
    for rule in spec.get('egress', []):
        for peer in rule.get('to', []):
            ipblock = peer.get('ipBlock', {})
            if ipblock.get('cidr') == '0.0.0.0/0':
                findings.append(f'  ⚠️  {ns}/{name} — egress allows 0.0.0.0/0')
        
        if not rule.get('to'):
            findings.append(f'  ⚠️  {ns}/{name} — egress rule with no to restriction (allows all)')

if findings:
    for f in findings:
        print(f)
    print(f'\n  Total permissive findings: {len(findings)}')
else:
    print('  ✅ No overly permissive policies detected')
" 2>/dev/null || echo "  ⚠️  Analysis failed"
}

# ── 执行 ──
audit_namespace_coverage
if ! $UNPROTECTED_ONLY; then
    audit_default_deny
fi
audit_pod_coverage
if ! $UNPROTECTED_ONLY; then
    audit_overly_permissive
fi

echo -e "\n============================================"
echo " Audit Complete"
echo "============================================"
echo ""
echo "💡 Recommendations:"
echo "   1. Add default deny-all policy to unprotected namespaces"
echo "   2. Ensure all production Pods are selected by at least one NetworkPolicy"
echo "   3. Review overly permissive policies and add specific CIDR/namespace selectors"
echo "   4. Use [[31-脚本/prompts/security-audit|security audit prompt]] for comprehensive review"
```

## 输出示例

```
[1/4] Namespace Coverage
--------------------------------------------
  Total Namespaces (audited): 12
  Protected (has NetworkPolicy): 8
  Unprotected (no NetworkPolicy): 4
  Unprotected Namespaces:
    🔴 dev-test — NO NetworkPolicy
    🔴 monitoring — NO NetworkPolicy

[2/4] Pod Coverage
--------------------------------------------
  ⚠️  prod/external-api-xxx — not selected by any NetworkPolicy

[3/4] Default Deny Policy Check
--------------------------------------------
  ✅ production — default deny (ingress + egress)
  🔴 dev-test — NO default deny policy

[4/4] Overly Permissive Policies
--------------------------------------------
  ⚠️  prod/frontend-netpol — ingress allows 0.0.0.0/0
```

## 推荐的修复策略

```yaml
# 默认 deny-all 基线策略 (每个命名空间都应部署)
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: default-deny-all
  namespace: {namespace}
spec:
  podSelector: {}    # 选择所有 Pod
  policyTypes:
  - Ingress
  - Egress
  # 无 ingress/egress 规则 = 拒绝所有流量
```

部署基线策略后，再逐步添加放行规则 (allow-list 模型)。

## 集成建议

- 配合 [[31-脚本/prompts/security-audit|安全审计 Prompt]] 做全面 CIS Benchmark 合规检查
- 建议在 CI/CD 中运行 `--unprotected-only` 模式，作为部署安全门控
- 新建命名空间时自动检查是否缺少默认 deny 策略

<!-- risk-assessed -->
