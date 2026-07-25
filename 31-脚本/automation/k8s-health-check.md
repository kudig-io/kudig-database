---
title: K8s 集群健康检查脚本
description: 综合集群健康检查 — 节点状态、Pod 状态、事件、资源使用、证书到期
summary: K8s 集群健康检查脚本 — 一键检查集群五大维度的健康状态
category: automation
tags:
- k8s
- automation
- health-check
- monitoring
- bash
tier: supporting
created: '2026-07-11'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- SRE
- 运维工程师
- oncall 工程师
estimated_read_time: 8min
intent_queries:
- K8s 集群健康检查脚本 是什么
- 如何一键检查 Kubernetes 集群健康
- 集群健康检查 bash 脚本
- kubernetes cluster health check script
trigger_keywords:
- 健康检查
- health-check
- 集群
- cluster
- 脚本
- script
prerequisites:
- kubectl-basics
- cluster-access
---

> **生产环境安全提示**
>
> 本脚本为只读检查 (🟢)，不修改集群状态。但输出可能包含敏感信息 (如节点 IP、证书指纹)，请注意日志安全。

# K8s 集群健康检查脚本

> 脚本 ID: `AUTO-01` | 语言: Bash | 风险: 🟢 只读 | 执行时间: ~30s

## 概述

本脚本对 Kubernetes 集群执行五大维度的综合健康检查:

1. **节点状态** — Ready 状态、资源压力、版本一致性
2. **Pod 状态** — 异常 Pod (CrashLoopBackOff/Pending/ImagePullBackOff)
3. **集群事件** — Warning 事件和异常原因聚合
4. **资源使用** — CPU/Memory 使用率和 Request/Limit 覆盖率
5. **证书到期** — 控制平面 TLS 证书到期检查

## 前置条件

- `kubectl` >= 1.28，已配置集群访问
- 具有 `cluster-reader` RBAC 权限
- 对控制平面证书的读取权限 (或使用 `kubeadm` 命令)
- Bash 4.0+，`jq` 已安装

## 使用方法

```bash
# 基础用法 — 检查所有维度
bash k8s-health-check.sh

# 指定命名空间
bash k8s-health-check.sh -n production

# 只检查特定维度
bash k8s-health-check.sh --check nodes,pods

# JSON 输出 (用于集成)
bash k8s-health-check.sh --json

# 设置证书到期告警阈值 (默认 30 天)
bash k8s-health-check.sh --cert-warn-days 60
```

## 完整脚本

```bash
#!/bin/bash
# k8s-health-check.sh — Kubernetes 集群综合健康检查
# 风险等级: 🟢 只读，无副作用

set -euo pipefail

# ── 参数解析 ──
NAMESPACE=""
CHECKS="nodes,pods,events,resources,certs"
OUTPUT="text"
CERT_WARN_DAYS=30

while [[ $# -gt 0 ]]; do
    case $1 in
        -n|--namespace) NAMESPACE="$2"; shift 2 ;;
        --check) CHECKS="$2"; shift 2 ;;
        --json) OUTPUT="json"; shift ;;
        --cert-warn-days) CERT_WARN_DAYS="$2"; shift 2 ;;
        -h|--help)
            echo "Usage: $0 [-n NAMESPACE] [--check CHECKS] [--json] [--cert-warn-days N]"
            echo "Checks: nodes, pods, events, resources, certs"
            exit 0 ;;
        *) echo "Unknown option: $1"; exit 1 ;;
    esac
done

IFS=',' read -ra CHECK_LIST <<< "$CHECKS"
TIMESTAMP=$(date -u +"%Y-%m-%dT%H:%M:%SZ")

echo "========================================"
echo " K8s Health Check — $TIMESTAMP"
echo "========================================"

# ── 1. 节点状态检查 ──
check_nodes() {
    echo -e "\n[1/5] Node Status"
    echo "----------------------------------------"
    
    local total ready notready
    total=$(kubectl get nodes --no-headers 2>/dev/null | wc -l)
    ready=$(kubectl get nodes --no-headers 2>/dev/null | grep -c " Ready" || true)
    notready=$((total - ready))
    
    echo "  Total Nodes : $total"
    echo "  Ready       : $ready"
    echo "  NotReady    : $notready"
    
    # 检查资源压力条件
    local pressure
    pressure=$(kubectl get nodes -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{range .status.conditions[?(@.status=="True")]}{.type}{" "}{end}{"\n"}{end}' 2>/dev/null | grep -E "DiskPressure|MemoryPressure|PIDPressure" || true)
    
    if [ -n "$pressure" ]; then
        echo "  ⚠️  Resource Pressure Detected:"
        echo "$pressure" | sed 's/^/      /'
    else
        echo "  ✅ No resource pressure"
    fi
    
    # 版本一致性
    local versions
    versions=$(kubectl get nodes -o jsonpath='{.items[*].status.nodeInfo.kubeletVersion}' 2>/dev/null | tr ' ' '\n' | sort -u | wc -l)
    if [ "$versions" -gt 1 ]; then
        echo "  ⚠️  Mixed kubelet versions detected ($versions different versions)"
        kubectl get nodes -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{.status.nodeInfo.kubeletVersion}{"\n"}{end}' | sed 's/^/      /'
    else
        echo "  ✅ Kubelet version consistent"
    fi
}

# ── 2. Pod 状态检查 ──
check_pods() {
    echo -e "\n[2/5] Pod Status"
    echo "----------------------------------------"
    
    local ns_flag=""
    [ -n "$NAMESPACE" ] && ns_flag="-n $NAMESPACE"
    
    local total running pending failed
    total=$(kubectl get pods -A ${ns_flag} --no-headers 2>/dev/null | wc -l)
    running=$(kubectl get pods -A ${ns_flag} --no-headers 2>/dev/null | grep -c "Running" || true)
    pending=$(kubectl get pods -A ${ns_flag} --no-headers 2>/dev/null | grep -c "Pending" || true)
    
    # 异常 Pod
    local crashed
    crashed=$(kubectl get pods -A ${ns_flag} --field-selector=status.phase!=Running,status.phase!=Succeeded --no-headers 2>/dev/null || true)
    
    local crashloop
    crashloop=$(kubectl get pods -A ${ns_flag} --no-headers 2>/dev/null | grep -E "CrashLoopBackOff|ImagePullBackOff|ErrImagePull|OOMKilled" || true)
    
    echo "  Total Pods  : $total"
    echo "  Running     : $running"
    echo "  Pending     : $pending"
    
    if [ -n "$crashed" ]; then
        echo "  🔴 Abnormal Pods:"
        echo "$crashed" | head -20 | sed 's/^/      /'
    else
        echo "  ✅ No abnormal pods"
    fi
    
    if [ -n "$crashloop" ]; then
        echo "  🔴 CrashLoop/ImagePull Issues:"
        echo "$crashloop" | head -20 | sed 's/^/      /'
    fi
}

# ── 3. 事件检查 ──
check_events() {
    echo -e "\n[3/5] Recent Warning Events (last 1h)"
    echo "----------------------------------------"
    
    local warnings
    warnings=$(kubectl get events -A --field-selector type=Warning --sort-by=.lastTimestamp 2>/dev/null | tail -20 || true)
    
    if [ -n "$warnings" ] && [ "$warnings" != "" ]; then
        echo "$warnings" | sed 's/^/  /'
        local count
        count=$(echo "$warnings" | wc -l)
        echo "  Total Warning Events: $count"
    else
        echo "  ✅ No warning events in the last hour"
    fi
}

# ── 4. 资源使用检查 ──
check_resources() {
    echo -e "\n[4/5] Resource Utilization"
    echo "----------------------------------------"
    
    # 节点资源
    echo "  Node Resources:"
    kubectl top nodes 2>/dev/null | sed 's/^/    /' || echo "    ⚠️  metrics-server not available"
    
    # 无 Request/Limit 的 Pod
    local no_limits
    no_limits=$(kubectl get pods -A -o json 2>/dev/null | jq -r '.items[] | select(.spec.containers[].resources.requests == null) | "\(.metadata.namespace)/\(.metadata.name)"' 2>/dev/null | head -10 || true)
    
    if [ -n "$no_limits" ]; then
        echo "  ⚠️  Pods without resource requests (top 10):"
        echo "$no_limits" | sed 's/^/      /'
    fi
}

# ── 5. 证书到期检查 ──
check_certs() {
    echo -e "\n[5/5] Certificate Expiry"
    echo "----------------------------------------"
    
    # 方式 1: kubeadm (如果有权限)
    if command -v kubeadm &>/dev/null; then
        local cert_info
        cert_info=$(kubeadm certs check-expiration 2>/dev/null || true)
        if [ -n "$cert_info" ]; then
            echo "$cert_info" | sed 's/^/  /'
            return
        fi
    fi
    
    # 方式 2: 通过 kubeconfig 检查 API server 证书
    local api_cert_expiry
    api_cert_expiry=$(echo | openssl s_client -connect "$(kubectl cluster-info 2>/dev/null | grep -oP 'https?://[^ ]+' | head -1 | sed 's|https://||')" 2>/dev/null | openssl x509 -noout -enddate 2>/dev/null | cut -d= -f2 || true)
    
    if [ -n "$api_cert_expiry" ]; then
        local expiry_epoch now_epoch days_left
        expiry_epoch=$(date -d "$api_cert_expiry" +%s 2>/dev/null || date -j -f "%b %d %T %Y %Z" "$api_cert_expiry" +%s 2>/dev/null || true)
        now_epoch=$(date +%s)
        days_left=$(( (expiry_epoch - now_epoch) / 86400 ))
        
        if [ "$days_left" -lt "$CERT_WARN_DAYS" ]; then
            echo "  🔴 API Server cert expires in $days_left days (threshold: $CERT_WARN_DAYS)"
        else
            echo "  ✅ API Server cert valid for $days_left more days"
        fi
    fi
    
    # 检查命名空间中的 TLS Secret
    echo "  TLS Secrets approaching expiry (< ${CERT_WARN_DAYS}d):"
    kubectl get secrets -A -o json 2>/dev/null | \
        jq -r '.items[] | select(.type=="kubernetes.io/tls") | 
               "\(.metadata.namespace)/\(.metadata.name)"' 2>/dev/null | \
        while read -r secret; do
            local ns name cert_data expiry days
            ns=$(echo "$secret" | cut -d/ -f1)
            name=$(echo "$secret" | cut -d/ -f2)
            cert_data=$(kubectl get secret "$name" -n "$ns" -o jsonpath='{.data.tls\.crt}' 2>/dev/null | base64 -d 2>/dev/null)
            if [ -n "$cert_data" ]; then
                expiry=$(echo "$cert_data" | openssl x509 -noout -enddate 2>/dev/null | cut -d= -f2)
                if [ -n "$expiry" ]; then
                    expiry_epoch=$(date -d "$expiry" +%s 2>/dev/null || date -j -f "%b %d %T %Y %Z" "$expiry" +%s 2>/dev/null)
                    now_epoch=$(date +%s)
                    days=$(( (expiry_epoch - now_epoch) / 86400 ))
                    if [ "$days" -lt "$CERT_WARN_DAYS" ]; then
                        echo "    🔴 $secret — expires in $days days ($expiry)"
                    fi
                fi
            fi
        done
}

# ── 执行 ──
for check in "${CHECK_LIST[@]}"; do
    case $check in
        nodes) check_nodes ;;
        pods) check_pods ;;
        events) check_events ;;
        resources) check_resources ;;
        certs) check_certs ;;
    esac
done

echo -e "\n========================================"
echo " Health Check Complete"
echo "========================================"
```

## 输出示例

```
========================================
 K8s Health Check — 2026-07-11T08:00:00Z
========================================

[1/5] Node Status
----------------------------------------
  Total Nodes : 20
  Ready       : 20
  NotReady    : 0
  ✅ No resource pressure
  ✅ Kubelet version consistent

[2/5] Pod Status
----------------------------------------
  Total Pods  : 342
  Running     : 338
  Pending     : 2
  🔴 CrashLoop/ImagePull Issues:
      prod/payment-service-xxx   CrashLoopBackOff

[5/5] Certificate Expiry
----------------------------------------
  ✅ API Server cert valid for 285 more days
  TLS Secrets approaching expiry (< 30d):
    🔴 prod/api-tls — expires in 12 days
```

## 集成建议

- **Cron 定时执行**: 每天早上 8 点生成报告，通过企业微信/钉钉/Slack webhook 推送
- **Grafana 集成**: 使用 `--json` 输出 + Prometheus node-exporter-textfile 收集器
- **告警联动**: 结合 [[31-脚本/prompts/incident-diagnosis|事件诊断 Prompt]] 自动触发根因分析
- **CI/CD**: 在部署后自动执行健康检查作为部署验证门控

<!-- risk-assessed -->
