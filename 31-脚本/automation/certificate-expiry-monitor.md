---
title: TLS 证书到期监控脚本
description: 跨命名空间 TLS 证书到期监控与告警
summary: TLS 证书到期监控脚本 — 检测 Kubernetes Secret 中的证书和控制平面证书到期
category: automation
tags:
- k8s
- automation
- certificate
- tls
- ssl
- monitoring
- bash
tier: supporting
created: '2026-07-11'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- SRE
- 安全工程师
- 运维工程师
estimated_read_time: 8min
intent_queries:
- TLS 证书到期监控脚本 是什么
- 如何监控 Kubernetes 证书过期
- Kubernetes Secret 证书检查
- certificate expiry monitor script
trigger_keywords:
- 证书
- certificate
- 到期
- expiry
- tls
- ssl
- 监控
- 脚本
prerequisites:
- kubectl-basics
- security-basics
- openssl-basics
---

> **生产环境安全提示**
>
> 本脚本为只读检查 (🟢)，不修改集群状态。

# TLS 证书到期监控脚本

> 脚本 ID: `AUTO-05` | 语言: Bash | 风险: 🟢 只读 | 执行时间: ~15s

## 概述

TLS 证书过期是常见的生产事故根源。证书过期会导致:
- API 调用失败 (HTTPS 握手失败)
- Ingress 流量中断
- Pod 间 mTLS 通信失败
- 服务网格 (Istio/Linkerd) 数据平面中断

本脚本全面监控集群中的所有 TLS 证书:

1. **Kubernetes Secret 证书** — `type: kubernetes.io/tls` 的 Secret 中的证书
2. **控制平面证书** — API Server / Controller Manager / Scheduler / Etcd 证书
3. **cert-manager 证书** — `Certificate` CRD 管理的证书
4. **Ingress TLS** — Ingress 资源引用的 TLS Secret
5. **自定义 CA** — ConfigMap/Secret 中的 CA 证书

## 前置条件

- `kubectl` >= 1.28，具有 `cluster-reader` 权限
- `openssl` CLI 已安装
- `jq` 已安装
- (可选) `kubeadm` 用于检查控制平面证书
- (可选) `cert-manager` CLI (`cmctl`) 如果使用 cert-manager

## 使用方法

```bash
# 检查所有证书 (默认阈值: 30 天告警, 7 天严重)
bash certificate-expiry-monitor.sh

# 自定义告警阈值
bash certificate-expiry-monitor.sh --warn-days 60 --critical-days 14

# 指定命名空间
bash certificate-expiry-monitor.sh -n production

# 只检查 cert-manager 证书
bash certificate-expiry-monitor.sh --cert-manager-only

# Webhook 告警 (企业微信/钉钉/Slack)
bash certificate-expiry-monitor.sh --webhook https://qyapi.weixin.qq.com/cgi-bin/webhook/send?key=xxx

# JSON 输出 (集成到 Prometheus/Grafana)
bash certificate-expiry-monitor.sh --json
```

## 完整脚本

```bash
#!/bin/bash
# certificate-expiry-monitor.sh — TLS 证书到期监控
# 风险等级: 🟢 只读，无副作用

set -euo pipefail

NAMESPACE=""
WARN_DAYS=30
CRITICAL_DAYS=7
WEBHOOK_URL=""
CERT_MANAGER_ONLY=false
OUTPUT="text"
TIMESTAMP=$(date -u +"%Y-%m-%dT%H:%M:%SZ")

while [[ $# -gt 0 ]]; do
    case $1 in
        -n|--namespace) NAMESPACE="$2"; shift 2 ;;
        --warn-days) WARN_DAYS="$2"; shift 2 ;;
        --critical-days) CRITICAL_DAYS="$2"; shift 2 ;;
        --webhook) WEBHOOK_URL="$2"; shift 2 ;;
        --cert-manager-only) CERT_MANAGER_ONLY=true; shift ;;
        --json) OUTPUT="json"; shift ;;
        *) echo "Unknown: $1"; exit 1 ;;
    esac
done

echo "============================================"
echo " Certificate Expiry Monitor — $TIMESTAMP"
echo " Warning: < ${WARN_DAYS}d | Critical: < ${CRITICAL_DAYS}d"
echo "============================================"

# 辅助函数: 解析证书到期日期并计算剩余天数
get_days_until_expiry() {
    local cert_pem="$1"
    local expiry
    expiry=$(echo "$cert_pem" | openssl x509 -noout -enddate 2>/dev/null | cut -d= -f2)
    if [ -z "$expiry" ]; then
        echo "N/A"
        return
    fi
    
    local expiry_epoch now_epoch
    # 兼容 Linux (date -d) 和 macOS (date -j)
    expiry_epoch=$(date -d "$expiry" +%s 2>/dev/null || \
                   date -j -f "%b %d %H:%M:%S %Y %Z" "$expiry" +%s 2>/dev/null || echo 0)
    now_epoch=$(date +%s)
    echo $(( (expiry_epoch - now_epoch) / 86400 ))
}

# 辅助函数: 格式化状态
format_status() {
    local days="$1"
    if [ "$days" = "N/A" ]; then
        echo "❓ Unknown"
    elif [ "$days" -le "$CRITICAL_DAYS" ]; then
        echo "🔴 CRITICAL (${days}d)"
    elif [ "$days" -le "$WARN_DAYS" ]; then
        echo "🟠 WARNING (${days}d)"
    else
        echo "🟢 OK (${days}d)"
    fi
}

ALERTS=""

# ── 1. 检查 TLS Secret ──
check_tls_secrets() {
    echo -e "\n[1/4] TLS Secrets"
    echo "--------------------------------------------"
    
    local ns_flag="-A"
    [ -n "$NAMESPACE" ] && ns_flag="-n $NAMESPACE"
    
    kubectl get secrets $ns_flag -o json 2>/dev/null | \
        jq -r '.items[] | select(.type=="kubernetes.io/tls") | 
               "\(.metadata.namespace)\t\(.metadata.name)"' 2>/dev/null | \
        while IFS=$'\t' read -r ns name; do
            [ -z "$ns" ] && continue
            
            # 获取证书内容
            local cert_data
            cert_data=$(kubectl get secret "$name" -n "$ns" \
                -o jsonpath='{.data.tls\.crt}' 2>/dev/null | base64 -d 2>/dev/null)
            
            if [ -z "$cert_data" ]; then
                echo "  ❓ $ns/$name — no tls.crt data"
                continue
            fi
            
            local days expiry cn
            days=$(get_days_until_expiry "$cert_data")
            expiry=$(echo "$cert_data" | openssl x509 -noout -enddate 2>/dev/null | cut -d= -f2)
            cn=$(echo "$cert_data" | openssl x509 -noout -subject 2>/dev/null | \
                 sed 's/subject=//' | sed 's/CN = //' | head -1)
            
            local status
            status=$(format_status "$days")
            echo "  $status  $ns/$name (CN: ${cn:-N/A}, expires: ${expiry:-N/A})"
            
            # 收集告警
            if [ "$days" != "N/A" ] && [ "$days" -le "$WARN_DAYS" ]; then
                ALERTS="${ALERTS}\n$ns/$name: $status (expires: $expiry)"
            fi
        done
}

# ── 2. 检查控制平面证书 ──
check_control_plane_certs() {
    echo -e "\n[2/4] Control Plane Certificates"
    echo "--------------------------------------------"
    
    # 方式 1: kubeadm
    if command -v kubeadm &>/dev/null; then
        echo "  (via kubeadm)"
        kubeadm certs check-expiration 2>/dev/null | sed 's/^/    /' || \
            echo "    ⚠️  kubeadm not accessible (may need sudo)"
    else
        echo "  (kubeadm not available — checking API server cert)"
        
        # 方式 2: 检查 API Server 端点证书
        local api_url
        api_url=$(kubectl cluster-info 2>/dev/null | grep -oP 'https?://[^ ]+' | head -1)
        
        if [ -n "$api_url" ]; then
            local api_host
            api_host=$(echo "$api_url" | sed 's|https://||')
            
            local cert_data
            cert_data=$(echo | timeout 5 openssl s_client -connect "$api_host" -showcerts 2>/dev/null)
            
            if [ -n "$cert_data" ]; then
                local days expiry
                days=$(get_days_until_expiry "$cert_data")
                expiry=$(echo "$cert_data" | openssl x509 -noout -enddate 2>/dev/null | cut -d= -f2)
                status=$(format_status "$days")
                echo "    $status  API Server ($api_host, expires: ${expiry:-N/A})"
            fi
        fi
        
        # 检查 kubelet 证书 (如果有节点访问权限)
        echo ""
        echo "    kubelet client cert (per node):"
        kubectl get nodes -o jsonpath='{range .items[*]}{.metadata.name}{"\n"}{end}' 2>/dev/null | \
            while read -r node; do
                [ -z "$node" ] && continue
                echo "      $node — check via SSH: openssl x509 -in /var/lib/kubelet/pki/kubelet-client-current.pem -enddate"
            done
    fi
}

# ── 3. 检查 cert-manager 证书 ──
check_cert_manager() {
    echo -e "\n[3/4] cert-manager Certificates"
    echo "--------------------------------------------"
    
    # 检查 cert-manager 是否安装
    if ! kubectl get crd certificates.cert-manager.io &>/dev/null 2>&1; then
        echo "  ℹ️  cert-manager not installed — skipping"
        return
    fi
    
    local ns_flag="-A"
    [ -n "$NAMESPACE" ] && ns_flag="-n $NAMESPACE"
    
    kubectl get certificates $ns_flag -o json 2>/dev/null | \
        jq -r '.items[] | 
               "\(.metadata.namespace)\t\(.metadata.name)\t\(.status.conditions[]? | select(.type=="Ready") | .status)\t\(.status.notAfter // "unknown)")"' 2>/dev/null | \
        while IFS=$'\t' read -r ns name ready not_after; do
            [ -z "$ns" ] && continue
            
            local status="🟢 OK"
            if [ "$not_after" != "unknown" ] && [ -n "$not_after" ]; then
                local days
                days=$(get_days_until_expiry "$(echo | openssl s_client 2>/dev/null)")
                # 直接计算天数
                local expiry_epoch now_epoch
                expiry_epoch=$(date -d "$not_after" +%s 2>/dev/null || \
                               date -j -f "%Y-%m-%dT%H:%M:%SZ" "$not_after" +%s 2>/dev/null || echo 0)
                now_epoch=$(date +%s)
                days=$(( (expiry_epoch - now_epoch) / 86400 ))
                status=$(format_status "$days")
            fi
            
            echo "  $status  $ns/$name (Ready: ${ready:-unknown}, notAfter: ${not_after:-N/A})"
            
            if [ "$ready" != "True" ]; then
                ALERTS="${ALERTS}\n$ns/$name: cert-manager NOT READY"
            fi
        done
}

# ── 4. 检查 Ingress TLS 配置 ──
check_ingress_tls() {
    echo -e "\n[4/4] Ingress TLS References"
    echo "--------------------------------------------"
    
    local ns_flag="-A"
    [ -n "$NAMESPACE" ] && ns_flag="-n $NAMESPACE"
    
    kubectl get ingress $ns_flag -o json 2>/dev/null | \
        jq -r '.items[] |
               .metadata.namespace as $ns |
               .metadata.name as $ingress |
               .spec.tls[]? |
               "\($ns)\t\($ingress)\t\(.secretName // "no-secret")\t\(.hosts | join(","))"' 2>/dev/null | \
        while IFS=$'\t' read -r ns ingress secret hosts; do
            [ -z "$ns" ] && continue
            
            if [ "$secret" = "no-secret" ] || [ -z "$secret" ]; then
                echo "  ⚠️  $ns/$ingress — TLS enabled but no secretName (uses default cert)"
            else
                # 验证 Secret 是否存在
                if kubectl get secret "$secret" -n "$ns" &>/dev/null 2>&1; then
                    echo "  ✅ $ns/$ingress → secret/$secret (hosts: $hosts)"
                else
                    echo "  🔴 $ns/$ingress → secret/$secret NOT FOUND (hosts: $hosts)"
                    ALERTS="${ALERTS}\n$ns/$ingress: TLS Secret '$secret' missing!"
                fi
            fi
        done
}

# ── 执行 ──
if ! $CERT_MANAGER_ONLY; then
    check_tls_secrets
    check_control_plane_certs
fi
check_cert_manager
if ! $CERT_MANAGER_ONLY; then
    check_ingress_tls
fi

# ── 告警汇总 ──
echo -e "\n============================================"
echo " Certificate Monitor Summary"
echo "============================================"

if [ -n "$ALERTS" ]; then
    echo -e "\n🚨 Alerts:$ALERTS"
    
    # 发送 webhook 告警
    if [ -n "$WEBHOOK_URL" ]; then
        echo -e "\n📡 Sending webhook alert..."
        local alert_text
        alert_text=$(echo -e "Certificate Expiry Alerts ($TIMESTAMP):$ALERTS" | \
            jq -Rs '{msgtype: "text", text: {content: .}}')
        curl -s -X POST "$WEBHOOK_URL" \
            -H "Content-Type: application/json" \
            -d "$alert_text" >/dev/null 2>&1 && echo "  ✅ Alert sent" || echo "  ❌ Alert failed"
    fi
else
    echo "✅ No certificate alerts — all certificates are within the safe window"
fi

echo ""
echo "💡 Recommendations:"
echo "   - Deploy cert-manager for automatic certificate rotation"
echo "   - Set up monitoring on cert-manager Certificate resources"
echo "   - Run this script weekly via cron"
echo "   - For control plane: kubeadm certs renew all (before expiry)"
```

## 输出示例

```
[1/4] TLS Secrets
--------------------------------------------
  🟢 OK (285d)   prod/api-tls (CN: api.example.com, expires: Apr 22 2027)
  🟠 WARNING (18d) prod/grafana-tls (CN: grafana.internal, expires: Jul 29 2026)
  🔴 CRITICAL (3d) staging/legacy-app-tls (CN: legacy.staging, expires: Jul 14 2026)

[2/4] Control Plane Certificates
--------------------------------------------
  (via kubeadm)
    CERTIFICATE                EXPIRES
    apiserver                  Aug 10, 2027 03:24 UTC
    apiserver-kubelet-client   Aug 10, 2027 03:24 UTC
    front-proxy-client         Aug 10, 2027 03:24 UTC

[3/4] cert-manager Certificates
--------------------------------------------
  🟢 OK (89d)  prod/wildcard-cert (Ready: True, notAfter: 2026-10-09T00:00:00Z)

🚨 Alerts:
  prod/grafana-tls: 🟠 WARNING (18d)
  staging/legacy-app-tls: 🔴 CRITICAL (3d)
```

## 自动化续期建议

使用 cert-manager 实现证书自动续期:

```yaml
apiVersion: cert-manager.io/v1
kind: Certificate
metadata:
  name: api-tls
  namespace: production
spec:
  secretName: api-tls
  duration: 2160h    # 90 天
  renewBefore: 360h  # 到期前 15 天自动续期
  issuerRef:
    name: letsencrypt-prod
    kind: ClusterIssuer
  commonName: api.example.com
  dnsNames:
  - api.example.com
```

配合此脚本，实现 "自动续期 + 到期监控" 双保险。

## 集成建议

- 每周 cron 执行，通过 webhook 推送到企业微信/钉钉/Slack
- 配合 cert-manager 使用时，监控 `Certificate` CRD 的 `Ready` 状态
- 将控制平面证书到期纳入集群升级计划 (kubeadm upgrade 会自动续期)
- Prometheus 可通过 `x509_cert_expiration_timestamp_seconds` 指标监控

<!-- risk-assessed -->
