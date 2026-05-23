---
title: "[2026-08-05] [P0] Istio mTLS 严格模式导致服务连通性中断"
category: case-study
tags: [production, incident, networking, service-mesh, istio, security]
date: "2026-08-05"
severity: P0
mttr: "27min"
status: resolved
created: "2026-05-23"
updated: "2026-05-23"
---

# [2026-08-05] Istio mTLS STRICT 模式误配导致全集群服务间调用失败

## 工单信息
- **工单编号**: INC-2026-0805-017
- **发现时间**: 2026-08-05 14:05 UTC
- **恢复时间**: 2026-08-05 14:32 UTC
- **影响范围**: 全集群（12 个 namespace，280+ Pod）
- **业务影响**: 微服务间调用全部失败，用户无法登录、下单、支付

## 问题现象
14:05，监控大盘全红，所有服务间调用返回 `connection reset by peer` 或 `upstream connect error`。用户反馈：
- 登录页面报错 "服务不可用"
- 已登录用户操作无响应
- 移动端 App 白屏

```bash
kubectl get pods -A | grep -v Running | grep -v Completed
# （大量 Pod 处于 Error 或 CrashLoopBackOff）
```

## 诊断过程

**14:07** — 检查一个问题 Pod 的 sidecar 日志：
```bash
kubectl logs -n prod-api order-api-xxx -c istio-proxy | tail -n 20
# 2026-08-05T14:04:55.112Z warn    envoy filter ...
#   tls_inspector: tls_error: TLS_ERROR_SECRET_NOT_FOUND
# 2026-08-05T14:04:55.113Z error   envoy filter ...
#   mcm TLS handshake error: 337047686:SSL routines:tls_process_client_certificate:certificate verify failed
```

**14:09** — 检查 PeerAuthentication：
```bash
kubectl get peerauthentication -A
# NAMESPACE     NAME           MODE         AGE
# istio-system  default        STRICT       5m
# prod-api      api-mtls       STRICT       5m
```

**14:11** — 检查根配置：
```bash
kubectl get peerauthentication default -n istio-system -o yaml
# spec:
#   mtls:
#     mode: STRICT
```

**14:13** — 检查变更历史：
```bash
# 14:00，安全团队执行了 "启用全集群 mTLS STRICT 模式" 的 GitOps 同步
# 意图：提升集群安全性，强制所有服务间通信使用 mTLS

# 但部分 namespace 的 Pod 未注入 Istio sidecar
kubectl get pods -n prod-legacy -l app=legacy-billing --show-labels
# NAME                    READY   STATUS
# legacy-billing-xxx      1/1     Running   （无 sidecar）
```

**14:15** — 验证：
```bash
# 从 order-api（有 sidecar）调用 legacy-billing（无 sidecar）
kubectl exec -n prod-api order-api-xxx -c istio-proxy -- \
  curl -v http://legacy-billing.prod-legacy.svc.cluster.local:8080/health
# * Connected to legacy-billing.prod-legacy.svc.cluster.local
# * ALPN: offers h2
# * TLS error: error:00000000:lib(0)::reason(0)
# * recv failure: Connection reset by peer
```

有 sidecar 的服务尝试用 mTLS 连接，但无 sidecar 的服务不支持 TLS，连接被重置。

## 根因
安全团队在 14:00 将 Istio PeerAuthentication 的 `mtls.mode` 从 `PERMISSIVE` 改为 `STRICT`，强制所有服务间通信使用 mTLS。但部分 `prod-legacy` namespace 的 Pod 未注入 Istio sidecar（因旧系统使用 hostNetwork，不支持 sidecar 注入），这些服务无法响应 mTLS 连接，导致：
1. 有 sidecar 的服务 → 无 sidecar 的服务：TLS 握手失败
2. 所有跨 namespace 调用中断
3. 依赖旧服务的核心业务全部失败

## 修复动作

**14:18** — 立即将全局 mTLS 回退到 PERMISSIVE：
```bash
kubectl patch peerauthentication default -n istio-system --type='merge' -p '
{
  "spec": {
    "mtls": {
      "mode": "PERMISSIVE"
    }
  }
}'
```

**14:20** — 为已注入 sidecar 的 namespace 单独配置 STRICT：
```bash
for ns in prod-api prod-order prod-payment prod-inventory; do
  cat <<EOF | kubectl apply -f -
apiVersion: security.istio.io/v1beta1
kind: PeerAuthentication
metadata:
  name: ${ns}-mtls
  namespace: ${ns}
spec:
  mtls:
    mode: STRICT
EOF
done
```

**14:25** — 验证服务恢复：
```bash
kubectl exec -n prod-api order-api-xxx -c istio-proxy -- \
  curl -s http://legacy-billing.prod-legacy.svc.cluster.local:8080/health
# {"status":"ok"}

kubectl exec -n prod-api order-api-xxx -c istio-proxy -- \
  curl -s http://auth-service.prod-auth.svc.cluster.local:8080/health
# {"status":"ok"}
```

**14:28** — 验证 mTLS 在已配置 namespace 中生效：
```bash
kubectl exec -n prod-api order-api-xxx -c istio-proxy -- \
  istioctl authn tls-check order-api.prod-api.svc.cluster.local
# HOST:PORT                                    STATUS     SERVER     CLIENT     AUTHN POLICY
# order-api.prod-api.svc.cluster.local:8080    OK         STRICT     -          prod-api/api-mtls
```

## 验证
- 14:30 — 用户登录、下单、支付全部恢复
- 14:32 — 5xx 错误率归零，业务指标正常

## 复盘
- **直接原因**: 全局 mTLS STRICT → 无 sidecar 的服务无法响应 TLS → 服务间调用全部失败
- **根本原因**: 安全团队未评估所有 Pod 的 sidecar 注入情况，未制定渐进式迁移计划
- **改进措施**:
  1. mTLS 迁移采用渐进策略：PERMISSIVE → STRICT（按 namespace 逐个启用）
  2. 部署前执行 sidecar 注入检查：`kubectl get pods -A -o json | jq '.items[] | select(.spec.containers | length == 1)'`
  3. 为无 sidecar 的服务配置 DestinationRule，关闭 mTLS 要求
  4. 添加 mTLS 兼容性测试 Pipeline：在 PERMISSIVE 模式下收集流量模式，确认所有服务对支持 mTLS 后再切换 STRICT
  5. STRICT 模式启用前，先对单个 namespace 进行 24h 金丝雀验证
- **相关 Skill**: [[k8s-network-security-guide]]
- **相关 FTA**: [[service-mesh-istio-fta]]
