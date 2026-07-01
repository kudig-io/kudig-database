---
title: '[2026-05-15] [P2] ConfigMap 更新未触发滚动更新'
summary: '[2026-05-15] [P2] ConfigMap 更新未触发滚动更新：16:00，DBA 团队完成数据库主从切换，新主库 IP 为 10.0.20.50。运维人员更新了
  payment-db-config ConfigMap 中的 DB_HOST。'
category: case-study
tags:
- production
- incident
- workloads
- configmap
- deployment
tier: supporting
created: '2026-05-23'
updated: '2026-05-23'
date: '2026-05-15'
severity: P2
mttr: 55min
status: resolved
last_updated: 2026-05-23
---



# [2026-05-15] ConfigMap 更新后应用仍使用旧配置，数据库连接指向已下线实例

## 工单信息
- **工单编号**: INC-2026-0515-011
- **发现时间**: 2026-05-15 16:00 UTC
- **恢复时间**: 2026-05-15 16:55 UTC
- **影响范围**: `prod-api` namespace，`payment-api` Deployment（6 个 Pod）
- **业务影响**: 支付接口偶发超时，16:00-16:55 期间支付成功率跌至 87%

## 问题现象
16:00，DBA 团队完成数据库主从切换，新主库 IP 为 `10.0.20.50`。运维人员更新了 `payment-db-config` ConfigMap 中的 `DB_HOST`。

但 15 分钟后，监控显示仍有大量连接指向旧主库 IP `10.0.20.45`：

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

```bash
kubectl exec -n prod-api payment-api-xxx -- netstat -an | grep 10.0.20.45 | wc -l
# 48
```

## 诊断过程

**16:05** — 检查 ConfigMap：
```bash
kubectl get cm payment-db-config -n prod-api -o yaml
# data:
#   DB_HOST: "10.0.20.50"
#   DB_PORT: "5432"
#   DB_NAME: "payment"
```

ConfigMap 已更新。

**16:07** — 检查 Pod 环境变量：

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

```bash
kubectl exec -n prod-api payment-api-xxx -- env | grep DB_HOST
# DB_HOST=10.0.20.45
```

Pod 仍在使用旧配置！

**16:09** — 检查 Deployment：
```bash
kubectl get deployment payment-api -n prod-api
# NAME          READY   UP-TO-DATE   AVAILABLE   AGE
# payment-api   6/6     6            6           120d

kubectl get rs -n prod-api -l app=payment-api
# NAME                     DESIRED   CURRENT   READY   AGE
# payment-api-7d9f4b8c5a   6         6         6       120d
```

Deployment 的 ReplicaSet 未变化，说明 ConfigMap 更新未触发滚动更新。

**16:11** — 检查 Deployment 的 Pod template：
```bash
kubectl get deployment payment-api -n prod-api -o yaml | grep -A20 env
# env:
# - name: DB_HOST
#   valueFrom:
#     configMapKeyRef:
#       name: payment-db-config
#       key: DB_HOST
```

Deployment 确实引用了 ConfigMap，但 Pod template hash 未变化，因此 Deployment 控制器认为无需滚动更新。

**16:13** — 根本原因：
- ConfigMap 是独立资源，修改 ConfigMap 不会导致引用它的 Deployment 的 `spec.template` 发生变化
- Deployment 控制器通过 `spec.template` 的 hash 决定是否触发滚动更新
- 只有 Pod template 变化才会触发滚动更新，ConfigMap 内容变化不会

## 根因
ConfigMap 作为独立资源，其内容变更不会自动触发引用它的 Deployment 的滚动更新。运维人员更新了 ConfigMap 但忘记手动触发 Deployment 滚动更新，导致旧 Pod 继续使用旧的数据库连接配置，连接已下线的旧主库。

## 修复动作

**16:15** — 手动触发滚动更新：

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl rollout undo/restart`：触发滚动变更，影响副本

```bash
kubectl rollout restart deployment payment-api -n prod-api
kubectl get pods -n prod-api -l app=payment-api -w
# payment-api-7d9f4b8c5a-xxx   1/1   Running   0   30s
# ...
```

**16:20** — 验证新 Pod 使用新配置：

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

```bash
kubectl exec -n prod-api payment-api-new-xxx -- env | grep DB_HOST
# DB_HOST=10.0.20.50

kubectl exec -n prod-api payment-api-new-xxx -- netstat -an | grep 10.0.20.50 | wc -l
# 8
```

**16:25** — 压测验证支付接口：
```bash
k6 run --vus 50 --duration 2m payment-api-smoke-test.js
# http_req_duration..............: avg=85ms  p(95)=150ms  p(99)=220ms
# http_req_failed................: 0.00%
```

**16:30** — 清理旧数据库连接：
```bash
# 在旧主库上强制断开来自 payment-api 的连接
# （已通过 DBA 操作完成）
```

## 验证
- 16:50 — 支付成功率恢复至 99.8%
- 16:52 — 无旧主库连接残留
- 16:55 — 全部业务指标正常

## 复盘
- **直接原因**: ConfigMap 更新后未触发 Deployment 滚动更新 → 旧 Pod 使用旧 DB_HOST → 连接已下线数据库 → 支付超时
- **根本原因**: 运维团队不了解 ConfigMap 变更不会自动触发滚动更新的机制
- **改进措施**:
  1. **Reloader 方案**: 部署 Reloader 控制器，自动监听 ConfigMap/Secret 变化并触发关联 Deployment 的滚动更新
  2. **GitOps 方案**: 将 ConfigMap 与 Deployment 放在同一个 Helm Chart，任何配置变更都通过 Helm upgrade 触发滚动更新
  3. **CI 检查**: 在 CI Pipeline 中添加校验——修改 ConfigMap 时，若关联 Deployment 未同步修改 `metadata.annotations.configmap-checksum`，则阻断合并
  4. SOP 更新：任何数据库连接配置变更后，必须执行 `kubectl rollout restart`
- **相关 Skill**: [[k8s-deployment-strategies-guide]]
- **相关 FTA**: [[deployment-fta]]
