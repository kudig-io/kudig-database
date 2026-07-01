---
title: '[2026-03-28] [P1] NetworkPolicy 误配导致服务间通信中断'
summary: '[2026-03-28] [P1] NetworkPolicy 误配导致服务间通信中断：15:20，客服收到大量投诉："已支付但显示未付款"。'
category: case-study
tags:
- production
- incident
- networking
- networkpolicy
- security
tier: supporting
created: '2026-05-23'
updated: '2026-05-23'
date: '2026-03-28'
severity: P1
mttr: 28min
status: resolved
last_updated: 2026-05-23
---



# [2026-03-28] NetworkPolicy 默认拒绝规则误拦截支付回调流量

## 工单信息
- **工单编号**: INC-2026-0328-007
- **发现时间**: 2026-03-28 15:20 UTC
- **恢复时间**: 2026-03-28 15:48 UTC
- **影响范围**: `prod-payment` namespace，支付回调链路（支付网关 → 订单服务）
- **业务影响**: 用户支付成功但订单状态未更新，15:20-15:48 期间 3,400 笔订单滞留

## 问题现象
15:20，客服收到大量投诉："已支付但显示未付款"。

监控显示：
- `payment-gateway` Pod 健康，但 `payment-callback-handler` Pod 日志中出现大量 `connection timeout`
- 订单服务 `order-api` 的 `/callback/payment` 接口无任何请求到达

## 诊断过程

**15:22** — 测试网络连通性：

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

```bash
# 从 payment-gateway Pod 测试到 order-api
kubectl exec -n prod-payment payment-gateway-xxx -- \
  curl -v http://order-api.prod-order.svc.cluster.local/callback/payment
# *   Trying 10.96.234.56...
# * connect to 10.96.234.56 port 80 failed: Connection timed out
```

**15:24** — 检查 Service 和 Endpoints：
```bash
kubectl get svc order-api -n prod-order
# NAME       TYPE        CLUSTER-IP     EXTERNAL-IP   PORT(S)   AGE
# order-api  ClusterIP   10.96.234.56   <none>        80/TCP    45d

kubectl get endpoints order-api -n prod-order
# NAME       ENDPOINTS                           AGE
# order-api  10.0.4.12:8080,10.0.4.13:8080       45d
```

Service 和 Endpoints 正常，问题在网络层。

**15:26** — 检查 `prod-order` namespace 的 NetworkPolicy：
```bash
kubectl get networkpolicy -n prod-order
# NAME                    POD-SELECTOR   AGE
# default-deny-all        <none>         25m
# allow-payment-ns        app=order-api  20m

kubectl get networkpolicy default-deny-all -n prod-order -o yaml
# spec:
#   podSelector: {}
#   policyTypes:
#   - Ingress
#   - Egress
```

**15:28** — 检查 `allow-payment-ns`：
```bash
kubectl get networkpolicy allow-payment-ns -n prod-order -o yaml
# spec:
#   podSelector:
#     matchLabels:
#       app: order-api
#   policyTypes:
#   - Ingress
#   ingress:
#   - from:
#     - namespaceSelector:
#         matchLabels:
#           name: prod-payment
#     ports:
#     - protocol: TCP
#       port: 8080
```

**15:30** — 发现 `allow-payment-ns` 只允许 `port: 8080`，但 `order-api` Service 的 `targetPort` 是 `8080`，而 `port` 是 `80`。NetworkPolicy 的 `port` 字段匹配的是容器端口（targetPort），即 `8080`。这个配置本身是对的。

**15:32** — 进一步排查，发现 `order-api` Pod 的 label 缺少 `app: order-api`：
```bash
kubectl get pod order-api-xxx -n prod-order --show-labels
# NAME           READY   STATUS    LABELS
# order-api-xxx  1/1     Running   app=order-api-v2,version=2.3.1
```

原来 14:55 部署的 v2.3.1 版本将 Pod label 从 `app: order-api` 改为 `app: order-api-v2`，导致 `allow-payment-ns` 的 `podSelector.matchLabels.app=order-api` 无法匹配到任何 Pod，支付回调流量被 `default-deny-all` 拦截。

## 根因
1. 14:55 部署的 `order-api` v2.3.1 误将 Pod label 从 `app: order-api` 改为 `app: order-api-v2`
2. `allow-payment-ns` NetworkPolicy 的 `podSelector` 仍匹配旧 label
3. 新 Pod 未被任何允许规则覆盖，被 `default-deny-all` 拦截
4. 支付网关到订单服务的回调流量被阻断，支付成功但订单状态不更新

## 修复动作

**15:35** — 紧急更新 NetworkPolicy：

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

```bash
cat <<'EOF' | kubectl apply -f -
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: allow-payment-ns
  namespace: prod-order
spec:
  podSelector:
    matchExpressions:
    - key: app
      operator: In
      values: ["order-api", "order-api-v2"]
  policyTypes:
  - Ingress
  ingress:
  - from:
    - namespaceSelector:
        matchLabels:
          name: prod-payment
    ports:
    - protocol: TCP
      port: 8080
EOF
```

**15:40** — 验证连通性：

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

```bash
kubectl exec -n prod-payment payment-gateway-xxx -- \
  curl -s http://order-api.prod-order.svc.cluster.local/callback/payment -X POST -d '{"order_id":"12345"}'
# {"status":"success","order_id":"12345"}
```

**15:42** — 批量处理滞留订单：
```bash
# 调用订单系统批量补偿接口
curl -s http://order-api.prod-order.svc.cluster.local/admin/reconcile-payments \
  -H "Authorization: Bearer $ADMIN_TOKEN" \
  -d '{"from":"2026-03-28T15:20:00Z","to":"2026-03-28T15:48:00Z"}'
# {"processed": 3400, "updated": 3400}
```

## 验证
- 15:45 — 新支付订单状态正常更新
- 15:48 — 3,400 笔滞留订单全部完成状态同步
- 客服投诉停止

## 复盘
- **直接原因**: `order-api` v2.3.1 修改 Pod label → NetworkPolicy podSelector 不匹配 → 支付回调被 default-deny-all 拦截
- **根本原因**: 
  1. 部署变更未检查 NetworkPolicy 依赖
  2. 缺少 NetworkPolicy 连通性冒烟测试
- **改进措施**:
  1. 部署 Pipeline 中添加 NetworkPolicy 连通性测试：部署后执行 `curl` 验证关键跨 namespace 调用
  2. NetworkPolicy 使用更稳定的 label 选择器（如 `app.kubernetes.io/name: order-api`），不依赖版本号 label
  3. 为所有 `default-deny-all` namespace 配置 NetworkPolicy 覆盖度检查脚本
  4. CI 中禁止修改 `app` label 的自动校验规则
- **相关 Skill**: [[k8s-network-security-guide]]
- **相关 FTA**: [[networkpolicy-fta]]
